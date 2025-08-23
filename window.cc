#include "window.h"

#include <glm/gtc/matrix_transform.hpp>

#if defined(USE_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif // USE_WAYLAND

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
    cache_(std::make_unique<Cache>()), adjust_(false),
    counter_(0) {
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
    zxdg_output_v1 *output = zxdg_output_manager_v1_get_xdg_output(parent->getManager(), glfwGetWaylandMonitor(monitor));
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
    window_ = glfwCreateWindow(mode->width, mode->height, parent_->name().c_str(), monitor, NULL);
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

    resizeCallback(window_, mode->width, mode->height);
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
        int x = position_.x + cursor_position_.x;
        int y = position_.y + cursor_position_.y;
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

        auto name = parent_->getHitBoxName(x + monitor_rect_.x, y +monitor_rect_.y);

        std::vector<std::string> args;
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
    if (!parent_->drag().has_value() && mouse_state_[GLFW_MOUSE_BUTTON_LEFT].press) {
        parent_->setDrag(cursor_position_.x + monitor_rect_.x, cursor_position_.y + monitor_rect_.y);
    }
    cursor_position_ = {x, y};
    if (parent_->drag().has_value()) {
        auto [dx, dy, px, py] = parent_->drag().value();
        parent_->setOffset(px + (x + monitor_rect_.x) - dx, py + (y + monitor_rect_.y) - dy);
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

void Window::draw(Offset offset, const std::vector<RenderInfo> &list, const bool use_self_alpha) {
    glfwMakeContextCurrent(window_);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
    std::unique_ptr<Texture> &texture = cache_->get(list, program_, use_self_alpha);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    assert(glGetError() == GL_NO_ERROR);
    glClear(GL_COLOR_BUFFER_BIT);
    assert(glGetError() == GL_NO_ERROR);
    if (*texture) {
        auto [x, y, w, h] = texture->rect();
        if (adjust_) {
            adjust_ = false;
            int side = parent_->side();
            int origin_x = monitor_rect_.x + monitor_rect_.width;
            if (side > 0) {
                Offset o = parent_->getCharacterOffset(side - 1);
                if (o.x < origin_x) {
                    origin_x = o.x;
                }
            }
            origin_x -= x + w;
            if (origin_x < monitor_rect_.x) {
                origin_x = monitor_rect_.x;
            }
            int origin_y = monitor_rect_.y + monitor_rect_.height;
            origin_y -= y + h;
            parent_->setOffset(origin_x, origin_y);
            offset = {origin_x, origin_y};
        }
        parent_->setSize(x + w, y + h);
        // OpenGLは左下が原点なのでyは上下逆にする
        glViewport(offset.x - monitor_rect_.x + x, monitor_rect_.height - (offset.y - monitor_rect_.y + y + h), w, h);
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
#if defined(USE_WAYLAND)
    wl_surface *surface = glfwGetWaylandWindow(window_);
    wl_compositor *compositor = parent_->getCompositor();
    wl_region *region = wl_compositor_create_region(compositor);
    if (*texture) {
        //auto [_x, _y, _w, h] = texture->rect();
        for (auto &r : texture->region()) {
            // wl_regionは左上が原点
            wl_region_add(region, offset.x - monitor_rect_.x + r.x, offset.y - monitor_rect_.y + r.y, r.width, r.height);
        }
    }
    wl_surface_set_input_region(surface, region);
    wl_region_destroy(region);
#endif // USE_WAYLAND
    glfwMakeContextCurrent(nullptr);
    assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
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
