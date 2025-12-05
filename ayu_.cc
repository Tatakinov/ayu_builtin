#include "ayu_.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#include <ws2tcpip.h>
#include <afunix.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef max
#undef min
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif // WIN32

#include "ayu.h"
#include "logger.h"
#include "misc.h"
#include "sstp.h"
#include "util.h"
#include "window.h"

namespace {
#ifndef IS_WINDOWS
    inline int closesocket(int fd) {
        return close(fd);
    }
#endif
}

Ayu::~Ayu() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        alive_ = false;
    }
    th_send_->join();
    th_recv_->join();
    characters_.clear();
#ifdef IS_WINDOWS
    WSACleanup();
#endif // Windows
}

bool Ayu::init() {
#ifdef IS_WINDOWS
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif // Windows

    th_recv_ = std::make_unique<std::thread>([&]() {
        uint32_t len;
        while (true) {
            std::cin.read(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            if (std::cin.eof() || len == 0) {
                break;
            }
            char *buffer = new char[len];
            std::cin.read(buffer, len);
            std::string request(buffer, len);
            delete[] buffer;
            if (std::cin.gcount() < len) {
                break;
            }
            auto req = ayu::Request::parse(request);
            Logger::log(request);
            auto event = req().value();

            ayu::Response res {204, "No Content"};

            if (event == "Initialize" && req(0)) {
                std::string tmp;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    tmp = req(0).value();
                }
                std::u8string dir(tmp.begin(), tmp.end());
                ayu_dir_ = dir;
            }
            else if (event == "Endpoint" && req(0) && req(1)) {
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    path_ = req(0).value();
                    uuid_ = req(1).value();
                }
            }
            else if (event == "UpdateInfo" && req(0)) {
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    int n;
                    util::to_x(req(0).value(), n);
                    for (int i = 1; i <= n; i++) {
                        auto &value = req(i).value();
                        auto pos = value.find(',');
                        if (pos == std::string::npos) {
                            continue;
                        }
                        auto key = value.substr(0, pos);
                        value = value.substr(pos + 1);
                        info_[key] = value;
                        do {
                            std::string tmp, group, name, category, part;
                            int side = -1, id = -1;
                            {
                                std::istringstream iss(key);
                                std::getline(iss, tmp, '.');
                                if (tmp == "sakura") {
                                    side = 0;
                                }
                                else if (tmp == "kero") {
                                    side = 1;
                                }
                                else if (tmp.starts_with("char")) {
                                    util::to_x(tmp.substr(4), side);
                                }
                                else {
                                    break;
                                }
                                std::getline(iss, tmp, '.');
                                if (!tmp.starts_with("bindgroup")) {
                                    break;
                                }
                                util::to_x(tmp.substr(9), id);
                                std::getline(iss, tmp, '.');
                                if (tmp != "name") {
                                    break;
                                }
                            }
                            {
                                std::istringstream iss(value);
                                std::getline(iss, category, ',');
                                if (category.empty()) {
                                    break;
                                }
                                std::getline(iss, part, ',');
                                if (part.empty()) {
                                    break;
                                }
                            }
                            key = category + "," + part;
                            bind_id_[side][key] = id;
                        } while (false);
                    }
                    loaded_ = true;
                }
                cond_.notify_one();
            }
            else if (event == "Position" && req(0)) {
                int side;
                std::istringstream iss(req(0).value());
                iss >> side;
                res = {200, "OK"};
                Rect r = getRect(side);
                res(0) = r.x;
                res(1) = r.y;
            }
            else if (event == "Size" && req(0)) {
                int side;
                std::istringstream iss(req(0).value());
                iss >> side;
                res = {200, "OK"};
                Rect r = getRect(side);
                res(0) = r.width;
                res(1) = r.height;
            }
            else if (event == "ResetBalloonPosition") {
                resetBalloonPosition();
            }
            else if (event == "SetBalloonOffset" && req(0) && req(1) && req(2)) {
                int side, x = 0, y = 0;
                util::to_x(req(0).value(), side);
                util::to_x(req(1).value(), x);
                util::to_x(req(2).value(), y);
                setBalloonOffset(side, x, y);
            }
            else if (event == "GetBalloonOffset" && req(0)) {
                int side;
                std::istringstream iss(req(0).value());
                iss >> side;
                res = {200, "OK"};
                auto offset = getBalloonOffset(side);
                res(0) = static_cast<int>(offset.x);
                res(1) = static_cast<int>(offset.y);
            }
            else if (event == "StartAnimation" && req(0) && req(1)) {
                int side, id;
                util::to_x(req(0).value(), side);
                util::to_x(req(1).value(), id);
                startAnimation(side, id);
            }
            else if (event == "IsPlayingAnimation" && req(0) && req(1)) {
                int side, id;
                util::to_x(req(0).value(), side);
                util::to_x(req(1).value(), id);
                res = {200, "OK"};
                bool playing = isPlayingAnimation(side, id);
                res(0) = static_cast<int>(playing);
            }
            else if (event == "Bind" && req(0) && req(1) && req(2) && req(3) && req(4)) {
                int side;
                BindFlag flag = BindFlag::Toggle;
                auto arg = req(4).value();
                if (arg == "true") {
                    flag = BindFlag::True;
                }
                else if (arg == "false") {
                    flag = BindFlag::False;
                }
                util::to_x(req(0).value(), side);
                do {
                    auto key = req(1).value() + "," + req(2).value();
                    if (!bind_id_.contains(side)) {
                        break;
                    }
                    if (!bind_id_.at(side).contains(key)) {
                        break;
                    }
                    int id = bind_id_.at(side).at(key);
                    bind(side, id, req(3).value(), flag);
                } while (false);
            }
            else {
                std::vector<std::string> args;
                args.push_back(event);
                for (int i = 0; ; i++) {
                    if (req(i)) {
                        args.push_back(req(i).value());
                    }
                    else {
                        break;
                    }
                }
                std::unique_lock<std::mutex> lock(mutex_);
                queue_.push(args);
            }

            res["Charset"] = "UTF-8";

            std::string response = res;
            Logger::log(response);
            len = response.size();
            std::cout.write(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            std::cout.write(response.c_str(), len);
        }
        {
            std::unique_lock<std::mutex> lock(mutex_);
            alive_ = false;
            event_queue_.push({{"", "", {}}});
        }
        cond_.notify_one();
    });

#if !defined(DEBUG)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [&] { return loaded_; });
    }
#endif // DEBUG

    std::string exe_path;
    exe_path.resize(1024);
#ifdef IS_WINDOWS
    GetModuleFileName(NULL, exe_path.data(), 1024);
#else
    readlink("/proc/self/exe", exe_path.data(), 1024);
#endif // OS
    std::filesystem::path exe_dir = exe_path;
    exe_dir = exe_dir.parent_path();
    bool use_self_alpha = (getInfo("seriko.use_self_alpha", false) == "1");
    cache_ = std::make_unique<ImageCache>(exe_dir, use_self_alpha);

    th_send_ = std::make_unique<std::thread>([&]() {
        while (true) {
            std::vector<Request> list;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait(lock, [this] { return !event_queue_.empty(); });
                if (!alive_) {
                    break;
                }
                list = event_queue_.front();
                event_queue_.pop();
            }
            for (auto &request : list) {
                auto res = sstp::Response::parse(sendDirectSSTP(request.method, request.command, request.args));
                if (res.getStatusCode() != 204) {
                    break;
                }
            }
        }
    });

#if !defined(DEBUG)
    surfaces_ = std::make_unique<Surfaces>(ayu_dir_);
    surfaces_->dump();
#endif // DEBUG

    return true;
}

std::string Ayu::getInfo(std::string key, bool fallback) {
    if (info_.contains(key)) {
        return info_.at(key);
    }
    if (fallback) {
        auto res = sstp::Response::parse(Ayu::sendDirectSSTP("EXECUTE", "GetSurfaceInfo", {key}));
        std::string content = res.getContent();
        if (content.empty()) {
            Logger::log("info(", key, "): not found");
            return "";
        }
        return content;
    }
    return "";
}


void Ayu::create(int side) {
    if (characters_.contains(side)) {
        return;
    }
    auto s = util::side2str(side);
    auto name = getInfo(s + ".name", true);
    characters_.emplace(side, std::make_unique<Character>(this, side, name.c_str(), surfaces_->getSeriko()));
    // TODO dynamic
    if (util::isWayland()) {
        int count = 0;
        auto *monitors = SDL_GetDisplays(&count);
        for (int i = 0; i < count; i++) {
            characters_.at(side)->create(monitors[i]);
        }
        SDL_free(monitors);
    }
    else {
        characters_.at(side)->create(0);
    }
    return;
}

void Ayu::show(int side) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->show();
}

void Ayu::hide(int side) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->hide();
}

void Ayu::setSurface(int side, int id) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->setSurface(id);
}

void Ayu::startAnimation(int side, int id) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->startAnimation(id);
}

bool Ayu::isPlayingAnimation(int side, int id) {
    if (!characters_.contains(side)) {
        return false;
    }
    return characters_.at(side)->isPlayingAnimation(id);
}

void Ayu::bind(int side, int id, std::string from, BindFlag flag) {
    if (!characters_.contains(side)) {
        return;
    }
    characters_.at(side)->bind(id, from, flag);
}

void Ayu::clearCache() {
    cache_->clearCache();
    for (auto &[_, v] : characters_) {
        v->clearCache();
    }
}

void Ayu::draw() {
    SDL_Event event;
    if (redrawn_) {
        SDL_PollEvent(&event);
    }
    else {
        SDL_WaitEventTimeout(&event, 1);
    }
    if (event.type == SDL_EVENT_QUIT) {
        alive_ = false;
        return;
    }
    std::queue<std::vector<std::string>> queue;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue.push(queue_.front());
            queue_.pop();
        }
    }
    bool changed = false;
    while (!queue.empty()) {
        std::vector<std::string> args = queue.front();
        queue.pop();
        if (args[0] == "Create") {
            int side;
            std::istringstream iss(args[1]);
            iss >> side;
            create(side);
        }
        else if (args[0] == "Show") {
            int side;
            std::istringstream iss(args[1]);
            iss >> side;
            show(side);
        }
        else if (args[0] == "Surface") {
            int side, id;
            util::to_x(args[1], side);
            util::to_x(args[2], id);
            setSurface(side, id);
        }
        else if (args[0] == "Scale") {
            int scale;
            util::to_x(args[1], scale);
            clearCache();
            cache_->setScale(scale);
            changed = true;
        }
    }
    std::vector<int> keys;
    for (auto &[k, _] : characters_) {
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    redrawn_ = false;
    for (auto k : keys) {
        redrawn_ = characters_.at(k)->draw(cache_, changed) || redrawn_;
    }
}

Rect Ayu::getRect(int side) {
    if (!characters_.contains(side)) {
        return {0, 0, 0, 0};
    }
    return characters_.at(side)->getRect();
}

void Ayu::resetBalloonPosition() {
    for (auto &[k, v] : characters_) {
        v->resetBalloonPosition();
    }
}

std::optional<Offset> Ayu::getCharacterOffset(int side) {
    int s = side;
    std::optional<Offset> ret = std::nullopt;
    for (; s >= 0; s--) {
        if (characters_.contains(s)) {
            break;
        }
    }
    if (s == -1) {
        ret = {0, 0};
    }
    else if (characters_.at(s)->isAdjusted()) {
        ret = characters_.at(s)->getOffset();
    }
    return ret;
}

void Ayu::setBalloonOffset(int side, int x, int y) {
    if (!characters_.contains(side)) {
        return;
    }
    return characters_.at(side)->setBalloonOffset(x, y);
}

Offset Ayu::getBalloonOffset(int side) {
    if (!characters_.contains(side)) {
        return {0, 0};
    }
    return characters_.at(side)->getBalloonOffset();
}

std::string Ayu::sendDirectSSTP(std::string method, std::string command, std::vector<std::string> args) {
    sstp::Request req {method};
    sstp::Response res {500, "Internal Server Error"};
    req["Charset"] = "UTF-8";
    req["Ayu"] = uuid_;
    if (path_.empty()) {
        return res;
    }
    req["Sender"] = "AYU_PoC";
    req["Option"] = "nodescript";
    if (req.getCommand() == "EXECUTE") {
        req["Command"] = command;
    }
    else if (req.getCommand() == "NOTIFY") {
        req["Event"] = command;
    }
    for (int i = 0; i < args.size(); i++) {
        req(i) = args[i];
    }
    sockaddr_un addr;
    if (path_.length() >= sizeof(addr.sun_path)) {
        return res;
    }
    int soc = socket(AF_UNIX, SOCK_STREAM, 0);
    if (soc == -1) {
        return res;
    }
    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    // null-terminatedも書き込ませる
    strncpy(addr.sun_path, path_.c_str(), path_.length() + 1);
    if (connect(soc, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
        return res;
    }
    std::string request = req;
    if (send(soc, request.c_str(), request.size(), 0) != request.size()) {
        closesocket(soc);
        return res;
    }
    char buffer[BUFFER_SIZE] = {};
    std::string data;
    while (true) {
        int ret = recv(soc, buffer, BUFFER_SIZE, 0);
        if (ret == -1) {
            closesocket(soc);
            return res;
        }
        if (ret == 0) {
            closesocket(soc);
            break;
        }
        data.append(buffer, ret);
    }
    return data;
}

void Ayu::enqueueDirectSSTP(std::vector<Request> list) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        event_queue_.push(list);
    }
    cond_.notify_one();
}
