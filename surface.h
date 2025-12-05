#ifndef SURFACE_H_
#define SURFACE_H_

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "misc.h"
#include "texture.h"

class ImageCache;

struct Element {
    Method method;
    int x, y;
    std::filesystem::path filename;
    bool operator==(const Element &rhs) const {
        const auto &lhs = *this;
        return lhs.method == rhs.method && lhs.x == rhs.x && lhs.y == rhs.y && lhs.filename == rhs.filename;
    }
    std::unique_ptr<WrapTexture> getTexture(SDL_Renderer *renderer, std::unique_ptr<ImageCache> &cache) const;
};

template<>
struct std::hash<Element> {
    size_t operator()(const Element &e) const {
        size_t ret = 0;
        ret ^= std::hash<Method>()(e.method);
        ret ^= std::hash<int>()(e.x);
        ret ^= std::hash<int>()(e.y);
        ret ^= std::hash<std::string>()(e.filename.string());
        return ret;
    }
};

struct Pattern {
    Method method;
    int id, wait_min, wait_max, x, y;
    std::vector<int> ids;
};

struct Animation {
    std::unordered_set<Interval> interval;
    int interval_factor;
    std::vector<Pattern> pattern;
    std::optional<std::vector<int>> exclusive;
    bool background;
    bool shared_index;
};

struct Collision {
    int factor;
    CollisionType type;
    std::string id;
    std::vector<int> point;
};

struct Surface {
    std::unordered_map<int, Element> element;
    std::unordered_map<int, Animation> animation;
    std::unordered_map<int, Collision> collision;
    void merge(const Surface &other) {
        for (auto &[k, v] : other.element) {
            element[k] = v;
        }
        for (auto &[k, v] : other.animation) {
            animation[k] = v;
        }
        for (auto &[k, v] : other.collision) {
            collision[k] = v;
        }
    }
};

#endif // SURFACE_H_
