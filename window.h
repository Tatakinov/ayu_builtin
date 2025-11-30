#ifndef WINDOW_H_
#define WINDOW_H_

#include <chrono>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "ayu_.h"
#include "image_cache.h"
#include "logger.h"
#include "misc.h"
#include "program.h"
#include "texture_cache.h"
#include "util.h"

class Character;
class TextureCache;

class Window {
    struct State {
        bool press;
        bool drag;
        std::chrono::system_clock::time_point prev;
    };
    private:
        std::mutex mutex_;
        GLFWwindow *window_;
        std::unique_ptr<Program> program_;
        Position<int> size_;
        GLfloat scale_;
        Position<int> position_;
        bool focused_;
        std::unordered_map<int, State> mouse_state_;
        Position<double> cursor_position_;
        Character *parent_;
        Rect monitor_rect_;
        std::unique_ptr<TextureCache> cache_;
        bool adjust_;
        int counter_;
        Offset offset_;
        std::optional<std::vector<Rect>> region_;

        static void resizeCallback(GLFWwindow *window, int width, int height) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->resize(width, height);
        }

        static void focusCallback(GLFWwindow *window, int focused) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->focus(focused);
        }

        static void positionCallback(GLFWwindow *window, int x, int y) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->position(x, y);
        }

        void focus(int focused);

        static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->mouseButton(button, action, mods);
        }

        void mouseButton(int button, int action, int mods);

        static void cursorPositionCallback(GLFWwindow *window, double x, double y) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->cursorPosition(x, y);
        }

        void cursorPosition(double x, double y);

        static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->key(key, scancode, action, mods);
        }

        void key(int key, int scancode, int action, int mods);

    public:
        Window(Character *parent, GLFWmonitor *monitor);
        virtual ~Window();

        operator bool() {
            return !glfwWindowShouldClose(window_);
        }

        void resize(int width, int height);
        void position(int x, int y);

        bool draw(std::unique_ptr<ImageCache> &image_cache, Offset offset, const std::vector<RenderInfo> &list, const bool use_self_alpha);
        void swapBuffers();

        void setPosition(int x, int y) {
            monitor_rect_.x = x;
            monitor_rect_.y = y;
        }

        GLfloat getScale() const {
            return scale_;
        }

        void show(bool force) {
            if (force || glfwGetWindowAttrib(window_, GLFW_VISIBLE) == GLFW_FALSE) {
                glfwShowWindow(window_);
            }
            assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
        }

        void hide() {
            glfwHideWindow(window_);
            assert(glfwGetError(nullptr) == GLFW_NO_ERROR);
        }

        Position<int> getPosition() {
            std::unique_lock<std::mutex> lock(mutex_);
            Position<int> p = {position_.x, position_.y};
            return p;
        }

        Rect getMonitorRect() const {
            if (util::isWayland() && !util::isCompatibleRendering()) {
                Rect r = {0, 0, 0, 0};
                glfwGetWindowSize(window_, &r.width, &r.height);
                return r;
            }
            else {
                return monitor_rect_;
            }
        }

        void requestAdjust() {
            adjust_ = true;
        }

        bool isAdjusted() const {
            return !adjust_;
        }

        double distance(int x, int y) const;

        void setCursor(GLFWcursor *cursor);

        void clearCache();

#if defined(USE_WAYLAND)
        void increment() {
            counter_++;
        }
        void decrement() {
            counter_--;
        }
#endif // USE_WAYLAND
};

#endif // WINDOW_H_
