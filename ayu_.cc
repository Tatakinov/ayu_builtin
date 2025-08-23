#include "ayu_.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

#if defined(USE_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif // USE_WAYLAND

#include "ayu.h"
#include "logger.h"
#include "sstp.h"
#include "util.h"
#include "window.h"

namespace {
    std::unordered_map<int, std::unique_ptr<Character>> characters;

    void errorCallback(int code, const char *message) {
        Logger::log("Error(", code, "): ", message);
    }
#if defined(USE_WAYLAND)
    wl_compositor *compositor = nullptr;
    zxdg_output_manager_v1 *manager = nullptr;
#endif // USE_WAYLAND
}

Ayu::~Ayu() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        alive_ = false;
    }
#if defined(USE_WAYLAND)
    if (compositor) {
    }
    if (manager) {
    }
#endif // USE_WAYLAND
    th_send_->join();
    th_recv_->join();
    characters.clear();
    glfwTerminate();
}

bool Ayu::init() {
    glfwSetErrorCallback(errorCallback);
    assert(glfwInit() != GLFW_FALSE);

    glfwSetMonitorCallback([](auto *monitor, int event) {
        if (event == GLFW_CONNECTED) {
            for (auto &[_, v] : characters) {
                v->create(monitor);
            }
        }
        else if (event == GLFW_DISCONNECTED) {
            for (auto &[_, v] : characters) {
                v->destroy(monitor);
            }
        }
    });

#if defined(USE_WAYLAND)
    wl_display *display = glfwGetWaylandDisplay();
    const wl_registry_listener listener = {
        [](void *data, wl_registry *reg, uint32_t id, const char *interface, uint32_t version) {
            std::string s = interface;
            if (s == "wl_compositor") {
                compositor = static_cast<wl_compositor *>(wl_registry_bind(reg, id, &wl_compositor_interface, 1));
            }
            if (s == "zxdg_output_manager_v1") {
                manager = static_cast<zxdg_output_manager_v1 *>(wl_registry_bind(reg, id, &zxdg_output_manager_v1_interface, 1));
            }
        },
        [](void *data, wl_registry *reg, uint32_t id) {
        },
    };
    wl_registry *r = wl_display_get_registry(display);
    wl_registry_add_listener(r, &listener, nullptr);
    wl_display_roundtrip(display);
    assert(compositor);
#endif // USE_WAYLAND

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);

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
                cond_.notify_one();
            }
            else if (event == "Endpoint" && req(0) && req(1)) {
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    path_ = req(0).value();
                    uuid_ = req(1).value();
                }
                cond_.notify_one();
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
                        info_[value.substr(0, pos)] = value.substr(pos + 1);
                    }
                }
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
        cond_.wait(lock, [this] { return !ayu_dir_.empty(); });
    }
#endif // DEBUG

#if !defined(DEBUG)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !path_.empty(); });
    }
#endif // DEBUG

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

#if defined(USE_WAYLAND)
wl_compositor *Ayu::getCompositor() {
    return compositor;
}
zxdg_output_manager_v1 *Ayu::getManager() {
    return manager;
}
#endif // USE_WAYLAND

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
    if (characters.contains(side)) {
        return;
    }
    auto s = util::side2str(side);
    auto name = getInfo(s + ".name", true);
    characters.try_emplace(side, std::make_unique<Character>(this, side, name.c_str(), surfaces_->getSeriko()));
    // TODO dynamic
    int count = 0;
    auto **monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; i++) {
        characters.at(side)->create(monitors[i]);
    }
    return;
}

void Ayu::show(int side) {
    if (!characters.contains(side)) {
        return;
    }
    characters.at(side)->show();
}

void Ayu::hide(int side) {
    if (!characters.contains(side)) {
        return;
    }
    characters.at(side)->hide();
}

void Ayu::setSurface(int side, int id) {
    if (!characters.contains(side)) {
        return;
    }
    characters.at(side)->setSurface(id);
}

void Ayu::startAnimation(int side, int id) {
    if (!characters.contains(side)) {
        return;
    }
    characters.at(side)->startAnimation(id);
}

bool Ayu::isPlayingAnimation(int side, int id) {
    if (!characters.contains(side)) {
        return false;
    }
    return characters.at(side)->isPlayingAnimation(id);
}

void Ayu::draw() {
    glfwPollEvents();
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    std::queue<std::vector<std::string>> queue;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue.push(queue_.front());
            queue_.pop();
        }
    }
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
    }
    std::vector<int> keys;
    for (auto &[k, _] : characters) {
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    for (auto k : keys) {
        characters[k]->draw();
    }
}

Rect Ayu::getRect(int side) {
    if (!characters.contains(side)) {
        return {0, 0, 0, 0};
    }
    return characters.at(side)->getRect();
}

void Ayu::resetBalloonPosition() {
    for (auto &[k, v] : characters) {
        v->resetBalloonPosition();
    }
}

Offset Ayu::getCharacterOffset(int side) {
    if (!characters.contains(side)) {
        return {0, 0};
    }
    return characters.at(side)->getOffset();
}

void Ayu::setBalloonOffset(int side, int x, int y) {
    if (!characters.contains(side)) {
        return;
    }
    return characters.at(side)->setBalloonOffset(x, y);
}

Offset Ayu::getBalloonOffset(int side) {
    if (!characters.contains(side)) {
        return {0, 0};
    }
    return characters.at(side)->getBalloonOffset();
}

GLFWcursor *Ayu::getCursor(CursorType type) {
    if (!cursors_.contains(type)) {
        switch (type) {
            case CursorType::Default:
                cursors_.emplace(type, glfwCreateStandardCursor(GLFW_ARROW_CURSOR));
                assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
                break;
            case CursorType::Hand:
                cursors_.emplace(type, glfwCreateStandardCursor(GLFW_HAND_CURSOR));
                assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
                break;
            default:
                assert(false);
                break;
        }
    }
    return cursors_.at(type);
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
    if (write(soc, request.c_str(), request.size()) != request.size()) {
        close(soc);
        return res;
    }
    char buffer[BUFFER_SIZE] = {};
    std::string data;
    while (true) {
        int ret = read(soc, buffer, BUFFER_SIZE);
        if (ret == -1) {
            close(soc);
            return res;
        }
        if (ret == 0) {
            close(soc);
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

std::unique_ptr<Seriko> Ayu::getSeriko() const {
    return surfaces_->getSeriko();
}
