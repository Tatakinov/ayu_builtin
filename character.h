#ifndef CHARACTER_H_
#define CHARACTER_H_

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <memory>

#if defined(USE_WAYLAND)
#include <wayland-client.h>
#include "xdg-output-client-protocol.h"
#endif // USE_WAYLAND

#include "ayu_.h"
#include "misc.h"
#include "seriko.h"
#include "window.h"

class Ayu;
class Window;

class Character {
    private:
        Ayu *parent_;
        int side_;
        std::string name_;
        std::unique_ptr<Seriko> seriko_;
        std::unordered_map<GLFWmonitor *, std::unique_ptr<Window>> windows_;
        Rect rect_;
        Offset balloon_offset_;
        bool balloon_direction_;
        std::optional<DragPosition> drag_;
        int id_;
        bool once_;
        bool reset_balloon_position_;
        CursorType current_cursor_type_;
        std::mutex mutex_;
    public:
        Character(Ayu *parent, int side, const std::string &name, std::unique_ptr<Seriko> seriko);
        ~Character();
        void create(GLFWmonitor *monitor);
        void destroy(GLFWmonitor *monitor);
        void draw();
        int side() const {
            return side_;
        }
        const std::string &name() const {
            return name_;
        }
        void show();
        void hide();
        void setSurface(int id);
        void startAnimation(int id);
        bool isPlayingAnimation(int id);
        Rect getRect() {
            Rect r;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                r = rect_;
            }
            return r;
        }
        void setSize(int w, int h) {
            std::unique_lock<std::mutex> lock(mutex_);
            rect_.width = w;
            rect_.height = h;
        }
        void setBalloonOffset(int x, int y);
        Offset getBalloonOffset();
        std::optional<DragPosition> drag() {
            return drag_;
        }
        void resetDrag() {
            drag_ = std::nullopt;
        }
        void setDrag(double x, double y) {
            drag_ = {x, y, rect_.x, rect_.y};
        }
        void setOffset(int x, int y);
        void resetBalloonPosition();
        Offset getOffset() {
            Offset offset;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                offset = {rect_.x, rect_.y};
            }
            return offset;
        }
        Offset getCharacterOffset(int side);
        std::string sendDirectSSTP(std::string method, std::string command, std::vector<std::string> args);
        void enqueueDirectSSTP(std::vector<Request> list);
        void bind(int id, bool enable);
        bool isBinding(int id);
        std::string getHitBoxName(int x, int y);
        void setCursor(CursorType type);
#if defined(USE_WAYLAND)
        wl_compositor *getCompositor();
        zxdg_output_manager_v1 *getManager();
#endif // USE_WAYLAND
};

#endif // CHARACTER_H_
