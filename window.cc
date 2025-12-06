#include "window.h"

#include <cassert>
#include <cmath>
#include <string>
#include <unordered_map>

#include "character.h"
#include "logger.h"
#include "misc.h"
#include "sstp.h"

#define MOUSE_BUTTON_LEFT 1
#define MOUSE_BUTTON_MIDDLE 2
#define MOUSE_BUTTON_RIGHT 3

#define KEY2S(key) { SDLK_ ## key, #key }
namespace {
    std::unordered_map<SDL_Keycode, int> key_count;
    std::unordered_map<SDL_Keycode, std::string> key2s = {
        KEY2S(0),
        KEY2S(1),
        KEY2S(2),
        KEY2S(3),
        KEY2S(4),
        KEY2S(5),
        KEY2S(6),
        KEY2S(7),
        KEY2S(8),
        KEY2S(9),
        KEY2S(A),
        KEY2S(B),
        KEY2S(C),
        KEY2S(D),
        KEY2S(E),
        KEY2S(F),
        KEY2S(G),
        KEY2S(H),
        KEY2S(I),
        KEY2S(J),
        KEY2S(K),
        KEY2S(L),
        KEY2S(M),
        KEY2S(N),
        KEY2S(O),
        KEY2S(P),
        KEY2S(Q),
        KEY2S(R),
        KEY2S(S),
        KEY2S(T),
        KEY2S(U),
        KEY2S(V),
        KEY2S(W),
        KEY2S(X),
        KEY2S(Y),
        KEY2S(Z),
    };
}
#undef KEY2S

Window::Window(Character *parent, SDL_DisplayID id)
    : window_(nullptr), size_({0, 0}),
    position_({0, 0}), parent_(parent),
    adjust_(false),
    counter_(0), offset_({0, 0}), renderer_(nullptr) {
    if (util::isWayland()) {
        SDL_Rect r;
        SDL_GetDisplayBounds(id, &r);
        monitor_rect_ = { r.x, r.y, r.w, r.h };
    }
    else {
        monitor_rect_ = { 0, 0, 1, 1 };
    }
    if (util::isWayland()) {
        SDL_PropertiesID p = SDL_CreateProperties();
        assert(p);
        SDL_SetStringProperty(p, SDL_PROP_WINDOW_CREATE_TITLE_STRING, parent_->name().c_str());
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN, true);
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_UNDEFINED_DISPLAY(id));
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_UNDEFINED_DISPLAY(id));
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, monitor_rect_.width);
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, monitor_rect_.height);
        window_ = SDL_CreateWindowWithProperties(p);
    }
#if 0
    else if (util::isWayland()) {
        int w = 0, h = 0;
        if (w == 0 || h == 0) {
            w = h = 200;
        }
        window_ = SDL_CreateWindow(parent_->name().c_str(), w, h, SDL_WINDOW_TRANSPARENT);
        //SDL_MaximizeWindow(window_);
        SDL_SyncWindow(window_);
        SDL_GetWindowSize(window_, &w, &h);
        monitor_rect_ = { 0, 0, w, h };
    }
#endif
    else {
        window_ = SDL_CreateWindow(parent_->name().c_str(), 200, 200, SDL_WINDOW_TRANSPARENT);
    }
    renderer_ = SDL_CreateRenderer(window_, NULL);
}

Window::~Window() {
    if (window_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
    }
}

void Window::resize(int width, int height) {
    std::unique_lock<std::mutex> lock(mutex_);
    size_ = {width, height};
}

void Window::focus(int focused) {
    //focused_ = (focused == GLFW_TRUE);
}

void Window::position(int x, int y) {
    if (!util::isWayland()) {
        SDL_SetWindowPosition(window_, x, y);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        position_ = {x, y};
    }
}

void Window::mouseButton(int button, int action, int mods) {
#if 0
    mouse_state_[button].press = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_LEFT && !mouse_state_[button].press) {
        parent_->resetDrag();
    }
    if (!mouse_state_[button].press && !mouse_state_[button].drag) {
        int x = cursor_position_.x;
        int y = cursor_position_.y;
        int b = 0;
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            b = 0;
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            b = 1;
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            b = 2;
        }

        if (util::isWayland()) {
            auto r = getMonitorRect();
            x = x + r.x;
            y = y + r.y;
        }
        auto name = parent_->getHitBoxName(x, y);

        std::vector<std::string> args;
        Offset offset = parent_->getOffset();
        x = x - offset.x;
        y = y - offset.y;
        args = {util::to_s(x), util::to_s(y), util::to_s(0), util::to_s(parent_->side()), name, util::to_s(b)};

        auto now = std::chrono::system_clock::now();
        auto prev = mouse_state_[button].prev;
        mouse_state_[button].prev = now;

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - prev).count() < 300) {
            Request req = {"NOTIFY", "OnMouseDoubleClick", args};
            parent_->enqueueDirectSSTP({req});
        }
        else if (button != GLFW_MOUSE_BUTTON_RIGHT) {
            Request up = {"NOTIFY", "OnMouseUp", args};
            Request click = {"NOTIFY", "OnMouseClick", args};
            parent_->enqueueDirectSSTP({up, click});
        }
        else {
            Request up = {"NOTIFY", "OnMouseUp", args};
            Request click = {"NOTIFY", "OnMouseClick", args};
#if 0
            // 右クリックメニューを呼び出す
            args = {util::to_s(parent_->side()), util::to_s(x), util::to_s(y)};
            Request menu = {"EXECUTE", "OpenMenu", args};
            parent_->enqueueDirectSSTP({up, click, menu});
#else
            parent_->enqueueDirectSSTP({up, click});
#endif
        }
    }
    for (auto &[k, v] : mouse_state_) {
        if (!v.press) {
            v.drag = false;
        }
    }
#if 0
    if (action == GLFW_RELEASE) {
        parent_->sendDirectSSTP("EXECUTE", "RaiseBalloon", {util::to_s(parent_->side())});
    }
#endif
#endif
}

void Window::cursorPosition(double x, double y) {
#if 0
    if (!parent_->drag().has_value()) {
        int xi = x, yi = y;
        if (util::isWayland()) {
            auto r = getMonitorRect();
            xi = xi + r.x;
            yi = yi + r.y;
        }
        auto name = parent_->getHitBoxName(xi, yi);
        if (name.empty()) {
            parent_->setCursor(CursorType::Default);
        }
        else {
            parent_->setCursor(CursorType::Hand);
        }
    }
    if (!parent_->drag().has_value() && mouse_state_[GLFW_MOUSE_BUTTON_LEFT].press) {
        if (util::isWayland() && util::isEnableMultiMonitor()) {
            parent_->setDrag(cursor_position_.x + monitor_rect_.x, cursor_position_.y + monitor_rect_.y);
        }
        else {
            parent_->setDrag(cursor_position_.x, cursor_position_.y);
        }
    }
    cursor_position_ = {x, y};
    if (parent_->drag().has_value()) {
        auto [dx, dy, px, py] = parent_->drag().value();
        if (util::isWayland() && util::isEnableMultiMonitor()) {
            x = x + monitor_rect_.x;
            y = y + monitor_rect_.y;
        }
        parent_->setOffset(px + x - dx, py + y - dy);
    }
    for (auto &[k, v] : mouse_state_) {
        if (v.press) {
            v.drag = true;
        }
    }
#endif
}

void dump(const std::vector<RenderInfo> &infos) {
    for (auto &info : infos) {
        if (std::holds_alternative<Element>(info)) {
            auto elem = std::get<Element>(info);
            Logger::log(elem.filename.string());
        }
    }
}

bool Window::draw(std::unique_ptr<ImageCache> &image_cache, Offset offset, const ElementWithChildren &element, const bool use_self_alpha) {
    auto surface = element.getSurface(image_cache);
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer_);
    auto m = getMonitorRect();
    if (surface) {
        auto texture = std::make_unique<WrapTexture>(renderer_, surface->surface());
        while (adjust_) {
            int side = parent_->side();
            int origin_x = m.x + m.width;
            if (side > 0) {
                auto o = parent_->getCharacterOffset(side - 1);
                if (!o) {
                    return false;
                }
                if (o->x < origin_x) {
                    origin_x = o->x;
                }
            }
            origin_x -= texture->width();
            if (origin_x < m.x) {
                origin_x = m.x;
            }
            int origin_y = m.y + m.height;
            origin_y -= texture->height();
            parent_->setOffset(origin_x, origin_y);
            offset = {origin_x, origin_y};

            adjust_ = false;
        }
        if (!util::isWayland()) {
            SDL_SetWindowSize(window_, texture->width(), texture->height());
        }
        parent_->setSize(texture->width(), texture->height());

        SDL_BlendMode mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
        SDL_SetRenderDrawBlendMode(renderer_, mode);
        SDL_FRect r = { offset.x - m.x, offset.y - m.y, texture->width(), texture->height() };
        SDL_RenderTexture(renderer_, texture->texture(), nullptr, &r);

        std::vector<int> shape;
        {
            SDL_LockSurface(surface->surface());
            for (int i = 0; i < surface->width() * surface->height(); i++) {
                unsigned char *p = static_cast<unsigned char *>(surface->surface()->pixels);
                if (p[4 * i + 3]) {
                    shape.push_back(i);
                }
            }
            SDL_UnlockSurface(surface->surface());
        }
        //if (!parent_->isInDragging() && (!shape_ || shape_ != shape || !(offset_ == offset))) {
        if ((!shape_ || shape_ != shape || !(offset_ == offset))) {
            auto s = std::make_unique<WrapSurface>(m.width, m.height);
            SDL_ClearSurface(s->surface(), 0, 0, 0, 0);
            Logger::log("surface.rect: ", surface->width(), ",", surface->height());
            SDL_Rect r = { offset.x - m.x, offset.y - m.y, surface->width(), surface->height() };
            SDL_BlitSurface(surface->surface(), nullptr, s->surface(), &r);
            offset_ = offset;
            SDL_SetWindowShape(window_, s->surface());
            shape_ = shape;
        }
    }
    else {
        if (!shape_ || shape_->size() > 0) {
            shape_ = std::make_optional<std::vector<int>>();
            auto s = std::make_unique<WrapSurface>(m.width, m.height);
            SDL_ClearSurface(s->surface(), 0, 0, 0, 0);
            SDL_SetWindowShape(window_, s->surface());
        }
    }
#if 0
    bool regenerate = false;
    std::unique_ptr<Texture> &texture = cache_->get(image_cache, list, program_, use_self_alpha);
    if (*texture) {
        auto [x, y, w, h] = texture->rect();
        auto r = getMonitorRect();
        while (adjust_) {
            int side = parent_->side();
            int origin_x = r.x + r.width;
            if (side > 0) {
                auto o = parent_->getCharacterOffset(side - 1);
                if (!o) {
                    return false;
                }
                if (o->x < origin_x) {
                    origin_x = o->x;
                }
            }
            origin_x -= x + w;
            if (origin_x < r.x) {
                origin_x = r.x;
            }
            int origin_y = r.y + r.height;
            origin_y -= y + h;
            parent_->setOffset(origin_x, origin_y);
            offset = {origin_x, origin_y};

            adjust_ = false;
        }
        if (!util::isWayland()) {
            SDL_SetWindowSize(window_, x + w, y + h);
        }
        parent_->setSize(x + w, y + h);
    }
    else {
        parent_->setSize(0, 0);
    }
    if (*texture) {
        if (!parent_->isInDragging() && (!region_ || !(region_.value() == texture->region()) || !(offset_ == offset))) {
            offset_ = offset;
            region_ = texture->region();
            SDL_SetWindowShape();
        }
    }
    else {
        if (!region_ || region_.value().size() > 0) {
            region_ = std::make_optional<std::vector<Rect>>();
            SDL_SetWindowShape();
        }
    }
    return texture->isUpconverted();
#else
    return true;
#endif
}

void Window::swapBuffers() {
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_RenderPresent(renderer_);
}

double Window::distance(int x, int y) const {
    if (monitor_rect_.x <= x && monitor_rect_.x + monitor_rect_.width >= x &&
            monitor_rect_.y <= y && monitor_rect_.y + monitor_rect_.height >= y) {
        return 0;
    }
    if (monitor_rect_.x <= x && monitor_rect_.x + monitor_rect_.width >= x) {
        return std::min(std::abs(monitor_rect_.x - x), std::abs(monitor_rect_.x + monitor_rect_.width - x));
    }
    if (monitor_rect_.y <= y && monitor_rect_.y + monitor_rect_.height >= y) {
        return std::min(std::abs(monitor_rect_.y - y), std::abs(monitor_rect_.y + monitor_rect_.height - y));
    }
    double d;
    {
        int dx = monitor_rect_.x - x;
        int dy = monitor_rect_.y - y;
        d = sqrt(dx * dx + dy * dy);
    }
    {
        int dx = monitor_rect_.x + monitor_rect_.width - x;
        int dy = monitor_rect_.y - y;
        d = std::min(d, sqrt(dx * dx + dy * dy));
    }
    {
        int dx = monitor_rect_.x + monitor_rect_.width - x;
        int dy = monitor_rect_.y + monitor_rect_.height - y;
        d = std::min(d, sqrt(dx * dx + dy * dy));
    }
    {
        int dx = monitor_rect_.x - x;
        int dy = monitor_rect_.y + monitor_rect_.height - y;
        d = std::min(d, sqrt(dx * dx + dy * dy));
    }
    return d;
}

void Window::clearCache() {
}

void Window::key(const SDL_KeyboardEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    // TODO stub
    std::vector<std::string> args = { key2s[event.key], util::to_s(event.key) };
    Request req = {"NOTIFY", "OnKeyPress", args};
    parent_->enqueueDirectSSTP({req});
}

void Window::motion(const SDL_MouseMotionEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    if (!parent_->drag().has_value()) {
        int xi = event.x, yi = event.y;
        if (util::isWayland()) {
            auto r = getMonitorRect();
            xi = xi + r.x;
            yi = yi + r.y;
        }
        auto name = parent_->getHitBoxName(xi, yi);
        if (name.empty()) {
            SDL_Cursor *cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
            SDL_SetCursor(cursor);
        }
        else {
            SDL_Cursor *cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
            SDL_SetCursor(cursor);
        }
    }
    if (!parent_->drag().has_value() && mouse_state_[MOUSE_BUTTON_LEFT].press) {
        if (util::isWayland()) {
            parent_->setDrag(cursor_position_.x + monitor_rect_.x, cursor_position_.y + monitor_rect_.y);
        }
        else {
            parent_->setDrag(cursor_position_.x, cursor_position_.y);
        }
    }
    cursor_position_ = {event.x, event.y};
    if (parent_->drag().has_value()) {
        auto [dx, dy, px, py] = parent_->drag().value();
        auto x = event.x;
        auto y = event.y;
        if (util::isWayland()) {
            x = x + monitor_rect_.x;
            y = y + monitor_rect_.y;
        }
        parent_->setOffset(px + x - dx, py + y - dy);
    }
    for (auto &[k, v] : mouse_state_) {
        if (v.press) {
            v.drag = true;
        }
    }
}

void Window::button(const SDL_MouseButtonEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    mouse_state_[event.button].press = event.down;
    if (event.button == MOUSE_BUTTON_LEFT && !mouse_state_[event.button].press) {
        parent_->resetDrag();
    }
    if (!mouse_state_[event.button].press && !mouse_state_[event.button].drag) {
        int x = cursor_position_.x;
        int y = cursor_position_.y;
        int b = -1;
        switch (event.button) {
            case 1:
                b = 0;
                break;
            case 2:
                b = 2;
                break;
            case 3:
                b = 1;
                break;
            default:
                break;
        }

        if (util::isWayland()) {
            auto r = getMonitorRect();
            x = x + r.x;
            y = y + r.y;
        }
        auto name = parent_->getHitBoxName(x, y);

        std::vector<std::string> args;
        Offset offset = parent_->getOffset();
        x = x - offset.x;
        y = y - offset.y;
        args = {util::to_s(x), util::to_s(y), util::to_s(0), util::to_s(parent_->side()), name, util::to_s(b)};

        if (event.clicks % 2 == 0) {
            Request req = {"NOTIFY", "OnMouseDoubleClick", args};
            parent_->enqueueDirectSSTP({req});
        }
        else if (event.button != MOUSE_BUTTON_RIGHT) {
            Request up = {"NOTIFY", "OnMouseUp", args};
            Request click = {"NOTIFY", "OnMouseClick", args};
            parent_->enqueueDirectSSTP({up, click});
        }
        else {
            Request up = {"NOTIFY", "OnMouseUp", args};
            Request click = {"NOTIFY", "OnMouseClick", args};
#if 0
            // 右クリックメニューを呼び出す
            args = {util::to_s(parent_->side()), util::to_s(x), util::to_s(y)};
            Request menu = {"EXECUTE", "OpenMenu", args};
            parent_->enqueueDirectSSTP({up, click, menu});
#else
            parent_->enqueueDirectSSTP({up, click});
#endif
        }
    }
    else if (event.down) {
        int x = cursor_position_.x;
        int y = cursor_position_.y;
        int b = -1;
        switch (event.button) {
            case 1:
                b = 0;
                break;
            case 2:
                b = 2;
                break;
            case 3:
                b = 1;
                break;
            default:
                break;
        }

        if (util::isWayland()) {
            auto r = getMonitorRect();
            x = x + r.x;
            y = y + r.y;
        }
        auto name = parent_->getHitBoxName(x, y);

        std::vector<std::string> args;
        Offset offset = parent_->getOffset();
        x = x - offset.x;
        y = y - offset.y;
        args = {util::to_s(x), util::to_s(y), util::to_s(0), util::to_s(parent_->side()), name, util::to_s(b)};
        Request req = {"NOTIFY", "OnMouseDown", args};
        parent_->enqueueDirectSSTP({req});
    }
    for (auto &[k, v] : mouse_state_) {
        if (!v.press) {
            v.drag = false;
        }
    }
}

void Window::wheel(const SDL_MouseWheelEvent &event) {
    if (event.windowID != SDL_GetWindowID(window_)) {
        return;
    }
    // TODO stub
}
