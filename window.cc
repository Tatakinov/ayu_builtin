#include "window.h"

#include <glm/gtc/matrix_transform.hpp>

#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef max
#undef min
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif // Windows

#if defined(USE_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif // USE_WAYLAND

#if defined(USE_X11)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif // USE_X11

#include "character.h"
#include "logger.h"
#include "sstp.h"

namespace {
    glm::mat4 view = glm::lookAt(
            glm::vec3(0, 0, 1),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
            );
}

Window::Window(Character *parent, GLFWmonitor *monitor)
    : window_(nullptr), size_({0, 0}),
    position_({0, 0}), parent_(parent),
    cache_(std::make_unique<TextureCache>()), adjust_(false),
    counter_(0), offset_({0, 0}) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    //glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    //assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "io.github.tatakinov.ayu_builtin");
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "io.github.tatakinov.ayu_builtin");

    auto *mode = glfwGetVideoMode(monitor);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwGetMonitorWorkarea(monitor, &monitor_rect_.x, &monitor_rect_.y, &monitor_rect_.width, &monitor_rect_.height);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
#if !defined(USE_WAYLAND)
    glfwGetMonitorPos(monitor, &monitor_rect_.x, &monitor_rect_.y);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
#else
    zxdg_output_v1 *output = zxdg_output_manager_v1_get_xdg_output(parent_->getManager(), glfwGetWaylandMonitor(monitor));
    assert(output);
    zxdg_output_v1_listener listener = {
        [](void *data, zxdg_output_v1 *o, int32_t x, int32_t y) {
            Window *w = static_cast<Window *>(data);
            w->setPosition(x, y);
        },
        [](void *data, zxdg_output_v1 *o, int32_t width, int32_t height) {
        },
        [](void *data, zxdg_output_v1 *o) {
        },
        [](void *data, zxdg_output_v1 *o, const char *name) {
        },
        [](void *data, zxdg_output_v1 *o, const char *description) {
        }
    };
    wl_display *display = glfwGetWaylandDisplay();
    zxdg_output_v1_add_listener(output, &listener, this);
    wl_callback *callback = wl_display_sync(display);
    wl_callback_listener callback_listener = {
        [](void *data, wl_callback *c, uint32_t time) {
            Window *w = static_cast<Window *>(data);
            w->decrement();
        }
    };
    increment();
    wl_callback_add_listener(callback, &callback_listener, this);
    while (wl_display_dispatch(display) != -1 && counter_ > 0) {}
    zxdg_output_v1_destroy(output);
#endif
    if (util::isWayland() && util::isCompatibleRendering()) {
        window_ = glfwCreateWindow(mode->width, mode->height, parent_->name().c_str(), monitor, nullptr);
    }
    else {
        window_ = glfwCreateWindow(200, 200, parent_->name().c_str(), nullptr, nullptr);
    }
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);

    glfwMakeContextCurrent(window_);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);

    assert(gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
    program_ = std::make_unique<Program>();

    glfwSwapInterval(1);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);

    glfwSetWindowUserPointer(window_, this);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);

    glfwSetWindowSizeCallback(window_, resizeCallback);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwSetWindowPosCallback(window_, positionCallback);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwSetWindowFocusCallback(window_, focusCallback);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwSetCursorPosCallback(window_, cursorPositionCallback);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwSetKeyCallback(window_, keyCallback);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);

    glEnable(GL_BLEND);

    if (util::isWayland() && !util::isCompatibleRendering()) {
        glfwMaximizeWindow(window_);
    }
    else {
        resizeCallback(window_, mode->width, mode->height);
    }
}

Window::~Window() {
    if (window_ != nullptr) {
        glfwMakeContextCurrent(window_);
        assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
        cache_.reset();
        program_.reset();
        glfwMakeContextCurrent(nullptr);
        assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
        glfwDestroyWindow(window_);
        assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    }
}

void Window::resize(int width, int height) {
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glViewport(0, 0, w, h);
    assert(glGetError() == GL_NO_ERROR);
    std::unique_lock<std::mutex> lock(mutex_);
    size_ = {width, height};
}

void Window::focus(int focused) {
    focused_ = (focused == GLFW_TRUE);
}

void Window::position(int x, int y) {
    if (!util::isWayland()) {
        glfwSetWindowPos(window_, x, y);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        position_ = {x, y};
    }
}

void Window::mouseButton(int button, int action, int mods) {
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
    if (action == GLFW_RELEASE) {
        parent_->sendDirectSSTP("EXECUTE", "RaiseBalloon", {util::to_s(parent_->side())});
    }
}

void Window::cursorPosition(double x, double y) {
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
        if (util::isWayland() && util::isCompatibleRendering()) {
            parent_->setDrag(cursor_position_.x + monitor_rect_.x, cursor_position_.y + monitor_rect_.y);
        }
        else {
            parent_->setDrag(cursor_position_.x, cursor_position_.y);
        }
    }
    cursor_position_ = {x, y};
    if (parent_->drag().has_value()) {
        auto [dx, dy, px, py] = parent_->drag().value();
        if (util::isWayland() && util::isCompatibleRendering()) {
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

void Window::key(int key, int scancode, int action, int mods) {
    // TODO stub
}

void dump(const std::vector<RenderInfo> &infos) {
    for (auto &info : infos) {
        if (std::holds_alternative<Element>(info)) {
            auto elem = std::get<Element>(info);
            Logger::log(elem.filename.string());
        }
    }
}

bool Window::draw(std::unique_ptr<ImageCache> &image_cache, Offset offset, const std::vector<RenderInfo> &list, const bool use_self_alpha) {
    if (size_.x == 0 || size_.y == 0) {
        return false;
    }
    bool regenerate = false;
    glfwMakeContextCurrent(window_);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    cache_->clearCache(false);
    std::unique_ptr<Texture> &texture = cache_->get(image_cache, list, program_, use_self_alpha);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    assert(glGetError() == GL_NO_ERROR);
    glClear(GL_COLOR_BUFFER_BIT);
    assert(glGetError() == GL_NO_ERROR);
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
            glfwSetWindowSize(window_, x + w, y + h);
        }
        parent_->setSize(x + w, y + h);
        // OpenGLは左下が原点なのでyは上下逆にする
        if (util::isWayland()) {
            glViewport(offset.x - r.x + x, r.height - (offset.y - r.y + y + h), w, h);
        }
        else {
            glViewport(x, size_.y - (y + h), w, h);
        }
        assert(glGetError() == GL_NO_ERROR);
        program_->use(view);
        program_->set(texture->id());
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        assert(glGetError() == GL_NO_ERROR);
    }
    else {
        parent_->setSize(0, 0);
    }
#if defined(_WIN32) || defined(WIN32)
    if (*texture) {
        if (!region_ || !(region_.value() == texture->region()) || !(offset_ == offset)) {
            HWND window = glfwGetWin32Window(window_);
            offset_ = offset;
            region_ = texture->region();
            std::vector<POINT> points;
            std::vector<int> counts;
            int num = 0;
            for (auto &r : region_.value()) {
                // 矩形内部が有効な領域になるので矩形を1まわり大きくする
                int x = offset.x - monitor_rect_.x + r.x;
                int y = offset.y - monitor_rect_.y + r.y;
                points.push_back({x - 1, y - 1});
                points.push_back({x - 1, y + r.height + 1});
                points.push_back({x + r.width + 1, y + r.height + 1});
                points.push_back({x + r.width + 1, y - 1});
                counts.push_back(4);
                num += 1;
            }
            HRGN region = CreatePolyPolygonRgn(&points[0], &counts[0], num, WINDING);
            SetWindowRgn(window, region, TRUE);
        }
    }
    else {
        if (!region_ || region_.value().size() > 0) {
            region_ = std::make_optional<std::vector<Rect>>();
            HWND window = glfwGetWin32Window(window_);
            HRGN region = CreateRectRgn(0, 0, 1, 1);
            SetWindowRgn(window, region, TRUE);
        }
    }
#endif // Windows
#if defined(USE_WAYLAND)
    if (*texture) {
        if (!parent_->isInDragging() && (!region_ || !(region_.value() == texture->region()) || !(offset_ == offset))) {
            offset_ = offset;
            region_ = texture->region();
            wl_surface *surface = glfwGetWaylandWindow(window_);
            wl_compositor *compositor = parent_->getCompositor();
            wl_region *region = wl_compositor_create_region(compositor);
            //auto [_x, _y, _w, h] = texture->rect();
            for (auto &r : texture->region()) {
                // wl_regionは左上が原点
                if (util::isWayland() && util::isCompatibleRendering()) {
                    wl_region_add(region, offset.x - monitor_rect_.x + r.x, offset.y - monitor_rect_.y + r.y, r.width, r.height);
                }
                else {
                    wl_region_add(region, offset.x + r.x, offset.y + r.y, r.width, r.height);
                }
            }
            wl_surface_set_input_region(surface, region);
            wl_region_destroy(region);
            Logger::log("update region.");
        }
    }
    else {
        if (!region_ || region_.value().size() > 0) {
            region_ = std::make_optional<std::vector<Rect>>();
            wl_surface *surface = glfwGetWaylandWindow(window_);
            wl_compositor *compositor = parent_->getCompositor();
            wl_region *region = wl_compositor_create_region(compositor);
            wl_surface_set_input_region(surface, region);
            wl_region_destroy(region);
        }
    }
#endif // USE_WAYLAND
#if defined(USE_X11)
    if (*texture) {
        if (!parent_->isInDragging() && (!region_ || !(region_.value() == texture->region()) || !(offset_ == offset))) {
            offset_ = offset;
            region_ = texture->region();
            Display *display = glfwGetX11Display();
            Window window = glfwGetX11Window(window_);
            std::vector<XRectangle> rect;
            rect.reserve(texture->region().size());
            for (auto &r : texture->region()) {
                rect.push_back({r.x, r.y, r.width, r.height});
            }
            XShapeCombineRectangles(display, window, ShapeBounding, 0, 0, rect.data(), rect.size(), ShapeSet, Unsorted);
        }
    }
    else {
        if (!region_ || region_.value().size() > 0) {
            region_ = std::make_optional<std::vector<Rect>>();
            Display *display = glfwGetX11Display();
            Window window = glfwGetX11Window(window_);
            XShapeCombineRectangles(display, window, ShapeBounding, 0, 0, nullptr, 0, ShapeSet, Unsorted);
        }
    }
#endif // USE_X11
    glfwMakeContextCurrent(nullptr);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    return texture->isUpconverted();
}

void Window::swapBuffers() {
    glfwMakeContextCurrent(window_);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwSwapBuffers(window_);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    glfwMakeContextCurrent(nullptr);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
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

void Window::setCursor(GLFWcursor *cursor) {
    glfwSetCursor(window_, cursor);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
}

void Window::clearCache() {
    glfwMakeContextCurrent(window_);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    cache_->clearCache();
    glfwMakeContextCurrent(nullptr);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
}
