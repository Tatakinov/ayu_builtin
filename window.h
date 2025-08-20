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
#include "cache.h"
#include "logger.h"
#include "misc.h"
#include "program.h"

class Cache;
class Character;

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
        std::unique_ptr<Cache> cache_;
        bool adjust_;
        int counter_;

        static void resizeCallback(GLFWwindow *window, int width, int height) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->resize(width, height);
        }

        void resize(int width, int height);

        static void focusCallback(GLFWwindow *window, int focused) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->focus(focused);
        }

        void focus(int focused);

        static void positionCallback(GLFWwindow *window, int x, int y) {
            auto instance = static_cast<Window *>(glfwGetWindowUserPointer(window));
            if (instance == NULL) {
                return;
            }
            instance->position(x, y);
        }

        void position(int x, int y);

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
        virtual ~Window() {
            if (window_ != nullptr) {
                glfwDestroyWindow(window_);
            }
        }

        operator bool() {
            return !glfwWindowShouldClose(window_);
        }

        void draw(Offset offset, const std::vector<RenderInfo> &list, const bool use_self_alpha);
        void swapBuffers();

        void setPosition(int x, int y) {
            monitor_rect_.x = x;
            monitor_rect_.y = y;
        }

        GLfloat getScale() const {
            return scale_;
        }

        void show() {
            glfwShowWindow(window_);
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
            return monitor_rect_;
        }

        void requestAdjust() {
            adjust_ = true;
        }

        double distance(int x, int y) const;

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
