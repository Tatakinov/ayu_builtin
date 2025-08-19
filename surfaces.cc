#include "surfaces.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <optional>
#include <unordered_map>

#include "logger.h"
#include "util.h"

enum class State { Root, Descript, Surface, None };

namespace {
    std::string trimSpace(const std::string &str) {
        int pos = 0;
        for (int i = 0; i < str.length(); i++) {
            if (str[i] == ' ' || str[i] == '\t') {
                pos++;
            }
            else {
                break;
            }
        }
        int n = str.length() - pos;
        for (int i = str.length() - 1; i >= 0; i--) {
            if (str[i] == ' ' || str[i] == '\t') {
                n--;
            }
            else {
                break;
            }
        }
        return str.substr(pos, n);
    }

    const std::unordered_map<std::string, Interval> s2interval = {
        {"sometimes", Interval::Sometimes},
        {"rarely", Interval::Rarely},
        {"random", Interval::Random},
        {"periodic", Interval::Periodic},
        {"always", Interval::Always},
        {"runonce", Interval::Runonce},
        {"never", Interval::Never},
        {"yen-e", Interval::YenE},
        {"talk", Interval::Talk},
        {"bind", Interval::Bind},
    };
    const std::unordered_map<std::string, Method> s2method = {
        {"base", Method::Base},
        {"overlay", Method::Overlay},
        {"overlayfast", Method::OverlayFast},
        {"overlaymultiply", Method::OverlayMultiply},
        {"replace", Method::Replace},
        {"interpolate", Method::Interpolate},
        {"asis", Method::Asis},
        {"move", Method::Move},
        {"bind", Method::Bind},
        {"add", Method::Add},
        {"reduce", Method::Reduce},
        {"insert", Method::Insert},
        {"start", Method::Start},
        {"stop", Method::Stop},
        {"alternativestart", Method::AlternativeStart},
        {"alternativestop", Method::AlternativeStop},
        {"parallelstart", Method::ParallelStart},
        {"parallelstop", Method::ParallelStop},
    };
    const std::unordered_map<std::string, Method> s2method_synthesize = {
        {"base", Method::Base},
        {"overlay", Method::Overlay},
        {"overlayfast", Method::OverlayFast},
        {"overlaymultiply", Method::OverlayMultiply},
        {"replace", Method::Replace},
        {"interpolate", Method::Interpolate},
        {"asis", Method::Asis},
        {"bind", Method::Bind},
        {"add", Method::Add},
        {"reduce", Method::Reduce},
    };
    const std::unordered_set<Method> synthesize = {
        Method::Base, Method::Overlay, Method::OverlayFast,
        Method::OverlayMultiply, Method::Replace,
        Method::Interpolate, Method::Asis, Method::Move,
        Method::Bind, Method::Add, Method::Reduce,
    };

    const std::unordered_map<std::string, CollisionType> s2collision = {
        {"rect", CollisionType::Rect},
        {"ellipse", CollisionType::Ellipse},
        {"circle", CollisionType::Circle},
        {"polygon", CollisionType::Polygon},
        {"region", CollisionType::Region}
    };
}

void parseSurfaceID(const std::string &line, std::unordered_set<int> &inclusive, std::unordered_set<int> & exclusive) {
    std::string tmp;
    std::istringstream l1(line);
    while (std::getline(l1, tmp, ',')) {
        bool in = true;
        if (tmp[0] == '!') {
            tmp = tmp.substr(1);
            in = false;
        }
        if (tmp.find('-') != std::string::npos) {
            std::istringstream l2(tmp);
            int begin, end;
            std::getline(l2, tmp, '-');
            util::to_x(tmp, begin);
            std::getline(l2, tmp, '-');
            util::to_x(tmp, end);
            if (begin > end) {
                continue;
            }
            for (int i = begin; i <= end; i++) {
                if (in) {
                    inclusive.emplace(i);
                }
                else {
                    exclusive.emplace(i);
                }
            }
        }
        else {
            int id;
            util::to_x(tmp, id);
            if (in) {
                inclusive.emplace(id);
            }
            else {
                exclusive.emplace(id);
            }
        }
    }
}

Surfaces::Surfaces(const std::filesystem::path &ayu_dir) {
    std::vector<std::filesystem::path> list;
    assert(std::filesystem::is_directory(ayu_dir));
    for (const auto &e : std::filesystem::directory_iterator(ayu_dir)) {
        if (e.is_regular_file()) {
            std::string path = e.path().filename().string();
            if (path.starts_with("surface") && path.ends_with(".png")) {
                std::string s;
                std::istringstream iss(path.substr(7));
                std::getline(iss, s, '.');
                bool valid = true;
                for (int i = 0; i < s.size(); i++) {
                    if (!isdigit(s[i])) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    int n;
                    util::to_x(s, n);
                    addSurface(n, e.path());
                }
            }
            else if (path.starts_with("surfaces") && path.ends_with(".txt")) {
                list.push_back(e);
            }
        }
    }
    std::sort(list.begin(), list.end(), std::less<std::filesystem::path>());
    for (auto &p : list) {
        parse(p);
    }
}

void Surfaces::parse(const std::filesystem::path &path) {
    std::filesystem::path shell_dir = path.parent_path();
    std::ifstream ifs(path);
    std::string data;
    std::getline(ifs, data, '\0');
    std::erase_if(data, [](char c) { return c == '\x0d'; });
    data = trimSpace(data);
    std::string charset = "UTF-8";
    if (data.starts_with("\xef\xbb\xbf")) {
        data = data.substr(3);
    }
    std::istringstream iss(data);
    std::string line;
    bool once = true;
    State state = State::Root;
    State next = State::None;
    std::optional<Surface> surface;
    std::unordered_set<int> inclusive;
    std::unordered_set<int> exclusive;
    bool append;
    for (int line_count = 1; std::getline(iss, line, '\x0a'); line_count++) {
        line = trimSpace(line);
        if (once && line.starts_with("charset,")) {
            // TODO
        }
        once = false;
        switch (state) {
            case State::Root:
                if (line == "descript") {
                    next = State::Descript;
                }
                else if (line.starts_with("surface.append")) {
                    inclusive.clear();
                    exclusive.clear();
                    append = true;
                    next = State::Surface;
                    surface = std::make_optional<Surface>();
                    parseSurfaceID(line.substr(14), inclusive, exclusive);
                    if (line.ends_with("{")) {
                        state = next;
                        next = State::None;
                    }
                }
                else if (line.starts_with("surface")) {
                    inclusive.clear();
                    exclusive.clear();
                    append = false;
                    next = State::Surface;
                    surface = std::make_optional<Surface>();
                    parseSurfaceID(line.substr(7), inclusive, exclusive);
                    if (line.ends_with("{")) {
                        state = next;
                        next = State::None;
                    }
                }
                else if (line.starts_with("//") || line.empty()) {
                }
                else if (line == "{") {
                    if (next == State::None || next == State::Root) {
                        // TODO error
                        continue;
                    }
                    state = next;
                    next = State::None;
                }
                else {
                    // TODO error
                }
                break;
            case State::Descript:
                if (line.starts_with("version,")) {
                    util::to_x(line.substr(8), version_);
                }
                else if (line == "}") {
                    state = State::Root;
                }
                else {
                    // TODO error
                }
                break;
            case State::Surface:
                if (line.starts_with("element")) {
                    Element element;
                    std::string tmp;
                    std::istringstream l(line.substr(7));
                    int id;
                    std::getline(l, tmp, ',');
                    util::to_x(tmp, id);
                    std::getline(l, tmp, ',');
                    if (!s2method_synthesize.contains(tmp)) {
                        Logger::log("Error(", line_count, "): invalid method in element");
                        continue;
                    }
                    element.method = s2method_synthesize.at(tmp);
                    std::getline(l, tmp, ',');
                    std::u8string u(tmp.begin(), tmp.end());
                    element.filename = shell_dir / u;
                    std::getline(l, tmp, ',');
                    util::to_x(tmp, element.x);
                    std::getline(l, tmp, ',');
                    util::to_x(tmp, element.y);
                    surface->element[id] = element;
                }
                else if (line.starts_with("animation")) {
                    std::string tmp;
                    std::istringstream l(line.substr(9));
                    std::getline(l, tmp, '.');
                    int id;
                    util::to_x(tmp, id);
                    std::getline(l, tmp, ',');
                    if (tmp == "interval") {
                        if (surface->animation.contains(id)) {
                            Logger::log("Error(", line_count, "): invalid method in animation");
                            continue;
                        }
                        Animation animation;
                        std::getline(l, tmp, ',');
                        std::istringstream l2(tmp);
                        while (std::getline(l2, tmp, '+')) {
                            if (!s2interval.contains(tmp)) {
                                Logger::log("Error(", line_count, "): invalid interval in animation");
                                continue;
                            }
                            animation.interval.emplace(s2interval.at(tmp));
                        }
                        std::getline(l, tmp, ',');
                        util::to_x(tmp, animation.interval_factor);
                        if (animation.interval_factor < 1) {
                            animation.interval_factor = 1;
                        }
                        surface->animation[id] = animation;
                    }
                    else if (tmp.starts_with("pattern")) {
                        if (!surface->animation.contains(id)) {
                            Logger::log("Error(", line_count, "): animation id not found");
                            continue;
                        }
                        int n;
                        util::to_x(tmp.substr(7), n);
                        if (surface->animation[id].pattern.size() < n) {
                            Logger::log("Error(", line_count, "): invalid pattern id");
                            continue;
                        }
                        Pattern p;
                        std::getline(l, tmp, ',');
                        if (surface->animation[id].interval.size() == 1 && surface->animation[id].interval.contains(Interval::Bind) && !s2method_synthesize.contains(tmp)) {
                            Logger::log("Error(", line_count, "): invalid method in bind");
                            continue;
                        }
                        if (!s2method.contains(tmp)) {
                            Logger::log("Error(", line_count, "): invalid method");
                            continue;
                        }
                        p.method = s2method.at(tmp);
                        if (synthesize.contains(p.method)) {
                            std::getline(l, tmp, ',');
                            util::to_x(tmp, p.id);
                            std::getline(l, tmp, ',');
                            if (tmp.find('-') != std::string::npos) {
                                std::istringstream l2(tmp);
                                std::getline(l2, tmp, '-');
                                util::to_x(tmp, p.wait_min);
                                std::getline(l2, tmp, '-');
                                util::to_x(tmp, p.wait_max);
                            }
                            else {
                                util::to_x(tmp, p.wait_min);
                                p.wait_max = p.wait_min;
                            }
                            std::getline(l, tmp, ',');
                            util::to_x(tmp, p.x);
                            std::getline(l, tmp, ',');
                            util::to_x(tmp, p.y);
                        }
                        else if (p.method == Method::Insert ||
                                p.method == Method::Start ||
                                p.method == Method::Stop) {
                            int id;
                            std::getline(l, tmp, ',');
                            util::to_x(tmp, id);
                            p.ids.push_back(id);
                        }
                        else if (p.method == Method::AlternativeStart ||
                                p.method == Method::AlternativeStop ||
                                p.method == Method::ParallelStart ||
                                p.method == Method::ParallelStop) {
                            // 末尾まで読み込みたい
                            std::getline(l, tmp, '\0');
                            if (!tmp.starts_with("(") || !tmp.ends_with(")")) {
                                // TODO error;
                                continue;
                            }
                            tmp = tmp.substr(1, tmp.length() - 2);
                            std::istringstream l2(tmp);
                            while (std::getline(l2, tmp, ',')) {
                                int id;
                                util::to_x(tmp, id);
                                p.ids.push_back(id);
                            }
                            if (p.ids.size() == 0) {
                                // TODO error
                                continue;
                            }
                        }
                        else {
                            // unreachable
                            assert(false);
                        }
                        if (surface->animation[id].pattern.size() > n) {
                            surface->animation[id].pattern[n] = p;
                        }
                        else {
                            surface->animation[id].pattern.push_back(p);
                        }
                    }
                }
                else if (line.starts_with("collisionex")) {
                    int id;
                    Collision collision;
                    std::string tmp;
                    std::istringstream l(line.substr(11));
                    collision.factor = line_count;
                    collision.type = CollisionType::Rect;
                    std::getline(l, tmp, ',');
                    util::to_x(tmp, id);
                    std::getline(l, tmp, ',');
                    collision.id = tmp;
                    std::getline(l, tmp, ',');
                    if (!s2collision.contains(tmp)) {
                        // TODO error
                        continue;
                    }
                    collision.type = s2collision.at(tmp);
                    while (std::getline(l, tmp, ',')) {
                        int point;
                        util::to_x(tmp, point);
                        collision.point.push_back(point);
                    }
                    if (surface->collision.contains(id)) {
                        // TODO error
                    }
                    else {
                        surface->collision[id] = collision;
                    }
                }
                else if (line.starts_with("collision")) {
                    int id;
                    Collision collision;
                    std::string tmp;
                    std::istringstream l(line.substr(9));
                    collision.factor = line_count;
                    collision.type = CollisionType::Rect;
                    std::getline(l, tmp, ',');
                    util::to_x(tmp, id);
                    for (int i = 0; i < 4; i++) {
                        int point;
                        std::getline(l, tmp, ',');
                        util::to_x(tmp, point);
                        collision.point.push_back(point);
                    }
                    std::getline(l, tmp, ',');
                    collision.id = tmp;
                    if (surface->collision.contains(id)) {
                        // TODO error
                    }
                    else {
                        surface->collision[id] = collision;
                    }
                }
                else if (line.starts_with("sakura.balloon.offset")) {
                    // TODO stub
                }
                else if (line.starts_with("kero.balloon.offset")) {
                    // TODO stub
                }
                else if (line.starts_with("balloon")) {
                    // TODO stub
                }
                else if (line.starts_with("point")) {
                    // TODO stub
                }
                else if (line == "}") {
                    state = State::Root;
                    for (auto e : inclusive) {
                        if (exclusive.contains(e)) {
                            continue;
                        }
                        if (append && !surfaces_.contains(e)) {
                            continue;
                        }
                        surfaces_[e].merge(*surface);
                    }
                }
                break;
            default:
                // unreachable
                assert(false);
        }
    }
    if (state != State::Root) {
        Logger::log("Error: invalid state");
    }
}

std::unique_ptr<Seriko> Surfaces::getSeriko() const {
    return std::make_unique<Seriko>(surfaces_);
}

void Surfaces::dump() const {
    for (auto &[k, v] : surfaces_) {
        Logger::log("surface: ", k);
        for (auto &[k, e] : v.element) {
            Logger::log("  element", k);
        }
        for (auto &[k, a] : v.animation) {
            Logger::log("  animation: ", k);
            Logger::log("    pattern: " , a.pattern.size());
        }
        for (auto &[k, c] : v.collision) {
            Logger::log("  collision: ", k);
        }
    }
}
