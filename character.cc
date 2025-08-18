#include "character.h"

#include "sstp.h"

Character::Character(Ayu *parent, int side, const std::string &name, std::unique_ptr<Seriko> seriko)
    : parent_(parent), side_(side), name_(name),
    seriko_(std::move(seriko)),
    rect_({0, 0, 0, 0}), balloon_offset_({0, 0}),
    balloon_direction_(false), id_(-1), once_(true),
    reset_balloon_position_(false) {
    seriko_->setParent(this);
}

Character::~Character() {
}

void Character::create(GLFWmonitor *monitor) {
    windows_.try_emplace(monitor, std::make_unique<Window>(this, monitor));
}

void Character::destroy(GLFWmonitor *monitor) {
    if (windows_.contains(monitor)) {
        windows_.erase(monitor);
    }
}

void Character::draw() {
    auto list = seriko_->get(id_);
    for (auto &[_, v] : windows_) {
        v->draw({rect_.x, rect_.y}, list);
    }
    for (auto &[_, v] : windows_) {
        v->swapBuffers();
    }
}

void Character::show() {
    for (auto &[_, v] : windows_) {
        v->show();
    }
}

void Character::hide() {
    for (auto &[_, v] : windows_) {
        v->hide();
    }
}

void Character::setSurface(int id) {
    id_ = id;
    if (id_ >= 0 && once_) {
        once_ = false;
#if 0
        for (auto &[_, v] : windows_) {
            Rect r = v->getMonitorRect();
            if (r.x <= rect_.x && r.x + r.width >= rect_.x &&
                    r.y <= rect_.y && r.y + r.height >= rect_.y) {
                v->requestAdjust();
                return;
            }
        }
#endif
        for (auto &[_, v] : windows_) {
            v->requestAdjust();
            break;
        }
    }
}

void Character::startAnimation(int id) {
    std::unique_lock<std::mutex> lock(mutex_);
    seriko_->activate(From::User, id, 0);
}

bool Character::isPlayingAnimation(int id) {
    std::unique_lock<std::mutex> lock(mutex_);
    return seriko_->active(id);
}

void Character::resetBalloonPosition() {
    int ox = rect_.x, oy = rect_.y;
    if (drag_) {
        auto v = drag_.value();
        ox += v.ox - v.x;
        oy += v.oy - v.y;
    }
    GLFWmonitor *key = nullptr;
    {
        double distance = -1;
        for (auto &[k, v] : windows_) {
            double d = v->distance(ox, oy);
            if (d < distance || distance == -1) {
                distance = d;
                key = k;
            }
        }
    }
    Rect monitor_rect = windows_[key]->getMonitorRect();
    int x, y;
    int w = 0, h = 0;
    {
        auto res = sstp::Response::parse(parent_->sendDirectSSTP("EXECUTE", "GetBalloonSize", {util::to_s(side_)}));
        std::string content = res.getContent();
        char delim;
        if (!content.empty()) {
            std::istringstream iss(content);
            iss >> w;
            iss >> delim;
            iss >> h;
        }
    }
    int rx = rect_.x + rect_.width - balloon_offset_.x;
    int lx = rect_.x - w + balloon_offset_.x;
    // 右
    if (balloon_direction_) {
        if (rx + w > monitor_rect.x + monitor_rect.width) {
            if (lx >= monitor_rect.x) {
                balloon_direction_ = !balloon_direction_;
            }
        }
    }
    // 左
    else {
        if (lx < monitor_rect.x) {
            if (rx + w <= monitor_rect.x + monitor_rect.width) {
                balloon_direction_ = !balloon_direction_;
            }
        }
    }
    if (balloon_direction_) {
        if (rx + w > monitor_rect.x + monitor_rect.width) {
            rx -= rx + w - monitor_rect.x - monitor_rect.width;
        }
        x = rx;
    }
    else {
        if (lx < monitor_rect.x) {
            lx -= lx - monitor_rect.x;
        }
        x = lx;
    }
    y = rect_.y + balloon_offset_.y;
    Request req = {"EXECUTE", "SetBalloonPosition", {util::to_s(side_), util::to_s(x), util::to_s(y)}};
    parent_->enqueueDirectSSTP({req});
    req = {"EXECUTE", "SetBalloonDirection", {util::to_s(side_), util::to_s((balloon_direction_) ? (1) : (0))}};
    parent_->enqueueDirectSSTP({req});
}

void Character::setBalloonOffset(int x, int y) {
    std::unique_lock<std::mutex> lock(mutex_);
    balloon_offset_ = {x, y};
}

Offset Character::getBalloonOffset() {
    Offset offset;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        offset = balloon_offset_;
    }
    return offset;
}

Offset Character::getCharacterOffset(int side) {
    return parent_->getCharacterOffset(side);
}

void Character::setOffset(int x, int y) {
    if (x != rect_.x || y != rect_.y) {
        std::unique_lock<std::mutex> lock(mutex_);
        rect_.x = x;
        rect_.y = y;
        reset_balloon_position_ = true;
        resetBalloonPosition();
    }
}

std::string Character::sendDirectSSTP(std::string method, std::string command, std::vector<std::string> args) {
    return parent_->sendDirectSSTP(method, command, args);
}

void Character::enqueueDirectSSTP(std::vector<Request> list) {
    parent_->enqueueDirectSSTP(list);
}

void Character::bind(int id, bool enable) {
    seriko_->bind(id, enable);
}

bool Character::isBinding(int id) {
    std::string key = util::side2str(side_) + ".bindgroup" + util::to_s(id) + ".default";
    std::string value = parent_->getInfo(key, false);
    if (value.empty()) {
        return false;
    }
    int binding;
    util::to_x(value, binding);
    return binding == 1;
}

std::string Character::getHitBoxName(int x, int y) {
    x -= rect_.x;
    y -= rect_.y;
    auto list = seriko_->getCollision(id_);
    for (auto &info : list) {
        for (auto &c : info.list) {
            if (c.type == CollisionType::Rect) {
                if (c.point.size() != 4) {
                    Logger::log("invalid collision type: rect");
                    continue;
                }
                int x1 = c.point[0];
                int y1 = c.point[1];
                int x2 = c.point[2];
                int y2 = c.point[3];
                if (x1 <= x && x2 >= x && y1 <= y && y2 >= y) {
                    return c.id;
                }
            }
            else if (c.type == CollisionType::Ellipse) {
                if (c.point.size() != 4) {
                    Logger::log("invalid collision type: ellipse");
                    continue;
                }
                int x1 = c.point[0];
                int y1 = c.point[1];
                int x2 = c.point[2];
                int y2 = c.point[3];
                double xr = std::abs(x1 - x2);
                double xo = (x1 + x2) / 2.0 - x;
                double yr = std::abs(y1 - y2);
                double yo = (y1 + y2) / 2.0 - y;
                assert(xr);
                assert(yr);
                if (xo * xo / xr * xr + yo * yo / yr * yr) {
                    return c.id;
                }
            }
            else if (c.type == CollisionType::Circle) {
                if (c.point.size() != 3) {
                    Logger::log("invalid collision type: circle");
                    continue;
                }
                int cx = c.point[0] - x;
                int cy = c.point[1] - y;
                int cr = c.point[2];
                if (cx * cx + cy * cy <= cr * cr) {
                    return c.id;
                }
            }
            else if (c.type == CollisionType::Polygon) {
                if (c.point.size() % 2 == 1 && c.point.size() >= 6) {
                    Logger::log("invalid collision type: polygon");
                    continue;
                }
                c.point.push_back(c.point[0]);
                c.point.push_back(c.point[1]);
                int count = 0;
                while (c.point.size() >= 4) {
                    double x1 = c.point[0];
                    double y1 = c.point[1];
                    double x2 = c.point[2];
                    double y2 = c.point[3];
                    c.point.erase(c.point.begin());
                    c.point.erase(c.point.begin());
                    if (y1 == y2) {
                        continue;
                    }
                    if (y1 > y2) {
                        if (y == y1) {
                            continue;
                        }
                        else if (y == y2 && x <= x2) {
                            count++;
                            continue;
                        }
                    }
                    else {
                        if (y == y1 && x < x1) {
                            count++;
                            continue;
                        }
                        else if (y == y2) {
                            continue;
                        }
                    }
                    if (y < y1 && y < y2) {
                        continue;
                    }
                    else if (y > y1 && y > y2) {
                        continue;
                    }
                    double intersection_x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
                    if (intersection_x > x) {
                        count++;
                    }
                }
                if (count % 2 == 1) {
                    return c.id;
                }
            }
            else if (c.type == CollisionType::Region) {
                // TODO stub
            }
        }
    }
    return "";
}

#if defined(USE_WAYLAND)
wl_compositor *Character::getCompositor() {
    return parent_->getCompositor();
}

zxdg_output_manager_v1 *Character::getManager() {
    return parent_->getManager();
}
#endif // USE_WAYLAND
