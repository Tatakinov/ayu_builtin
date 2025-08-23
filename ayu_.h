#ifndef GL_AYU_H_
#define GL_AYU_H_

#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(USE_WAYLAND)
#include <wayland-client.h>
#include "xdg-output-client-protocol.h"
#endif // USE_WAYLAND

#include "character.h"
#include "misc.h"
#include "surfaces.h"
#include "util.h"
#include "window.h"

class Character;
class Surfaces;

class Ayu {
    private:
        std::mutex mutex_;
        std::condition_variable cond_;
        std::queue<std::vector<std::string>> queue_;
        std::queue<std::vector<Request>> event_queue_;
        std::unique_ptr<std::thread> th_recv_;
        std::unique_ptr<std::thread> th_send_;
        std::filesystem::path ayu_dir_;
        std::unordered_map<std::string, std::string> info_;
        std::unique_ptr<Surfaces> surfaces_;
        std::unordered_map<CursorType, GLFWcursor *> cursors_;
#if !defined(_WIN32) && !defined(WIN32)
        std::string path_;
        std::string uuid_;
#endif // WIN32
        bool alive_;

    public:
        Ayu() : alive_(true) {
            init();
#if defined(DEBUG)
            ayu_dir_ = ".";
            surfaces_ = std::make_unique<Surfaces>(ayu_dir_);
#endif // DEBUG
        }
        ~Ayu();

        bool init();

        void load();

        std::string getInfo(std::string, bool fallback);

#if defined(USE_WAYLAND)
        wl_compositor *getCompositor();
        zxdg_output_manager_v1 *getManager();
#endif

        void create(int side);

        Rect getRect(int side);

        void resetBalloonPosition();

        void setBalloonOffset(int side, int x, int y);
        Offset getBalloonOffset(int side);

        Offset getCharacterOffset(int side);

        void show(int side);

        void hide(int side);

        void setSurface(int side, int id);

        void startAnimation(int side, int id);

        bool isPlayingAnimation(int side, int id);

        operator bool() {
            return alive_;
        }

        void draw();

        GLFWcursor *getCursor(CursorType type);

        std::string sendDirectSSTP(std::string method, std::string command, std::vector<std::string> args);

        void enqueueDirectSSTP(std::vector<Request> list);

        std::unique_ptr<Seriko> getSeriko() const;
};

#endif // GL_AYU_H_
