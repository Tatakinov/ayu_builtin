#ifndef GL_AYU_H_
#define GL_AYU_H_

#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "character.h"
#include "image_cache.h"
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
        std::unordered_map<int, std::unordered_map<std::string, int>> bind_id_;
        std::unordered_map<int, std::unique_ptr<Character>> characters_;
        std::unique_ptr<Surfaces> surfaces_;
        std::unique_ptr<ImageCache> cache_;
        std::string path_;
        std::string uuid_;
        bool alive_;
        int scale_;
        bool loaded_;
        bool redrawn_;

    public:
        Ayu() : alive_(true), scale_(100), loaded_(false), redrawn_(false) {
            init();
#if defined(DEBUG)
            ayu_dir_ = "./shell/master";
            surfaces_ = std::make_unique<Surfaces>(ayu_dir_);
#endif // DEBUG
        }
        ~Ayu();

        bool init();

        void load();

        std::string getInfo(std::string key, bool fallback);

        void create(int side);

        Rect getRect(int side);

        void resetBalloonPosition();

        void setBalloonOffset(int side, int x, int y);
        Offset getBalloonOffset(int side);

        std::optional<Offset> getCharacterOffset(int side);

        void show(int side);

        void hide(int side);

        void setSurface(int side, int id);

        void startAnimation(int side, int id);

        bool isPlayingAnimation(int side, int id);

        void bind(int side, int id, std::string from, BindFlag flag);

        void clearCache();

        operator bool() {
            return alive_;
        }

        void draw();

        std::string sendDirectSSTP(std::string method, std::string command, std::vector<std::string> args);

        void enqueueDirectSSTP(std::vector<Request> list);
};

#endif // GL_AYU_H_
