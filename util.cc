#include "util.h"

#include <cassert>
#include <cstdlib>
#include <random>

namespace {
    std::random_device rd;
    std::mt19937 mt(rd());
}

namespace util {
    double random() {
        std::uniform_real_distribution<> dist(0, 1);
        return dist(mt);
    }

    int random(int a, int b) {
        std::uniform_int_distribution<> dist(a, b);
        return dist(mt);
    }

    std::string side2str(int side) {
        if (side == 0) {
            return "sakura";
        }
        else if (side == 1) {
            return "kero";
        }
        else if (side >= 2) {
            std::ostringstream oss;
            oss << "char" << side;
            return oss.str();
        }
        // unreachable
        assert(false);
    }

    bool isWayland() {
        std::string wayland = "wayland";
        return (wayland == getenv("XDG_SESSION_TYPE"));
    }

    bool isEnableMultiMonitor() {
        return !!getenv("NINIX_ENABLE_MULTI_MONITOR");
    }
}
