#ifndef ELEMENT_H_
#define ELEMENT_H_

#include <memory>
#include <variant>

#include "surface.h"
#include "texture.h"

struct Element;
struct ElementWithChildren;

using RenderInfo = std::variant<Element, ElementWithChildren>;

struct ElementWithChildren {
    Method method;
    int x, y;
    std::vector<std::variant<Element, ElementWithChildren>> children;
    bool operator==(const ElementWithChildren &rhs) const {
        const auto &lhs = *this;
        if (!(lhs.method == rhs.method && lhs.x == rhs.x && lhs.y == rhs.y)) {
            return false;
        }
        if (rhs.children.size() != lhs.children.size()) {
            return false;
        }
        for (int i = 0; i < rhs.children.size(); i++) {
            if (std::holds_alternative<Element>(rhs.children[i]) && 
                    std::holds_alternative<Element>(lhs.children[i])) {
                auto &a = std::get<Element>(rhs.children[i]);
                auto &b = std::get<Element>(lhs.children[i]);
                if (!(a == b)) {
                    return false;
                }
            }
            else if (std::holds_alternative<ElementWithChildren>(rhs.children[i]) && 
                    std::holds_alternative<ElementWithChildren>(lhs.children[i])) {
                auto &a = std::get<ElementWithChildren>(rhs.children[i]);
                auto &b = std::get<ElementWithChildren>(lhs.children[i]);
                if (!(a == b)) {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        return true;
    }
    std::unique_ptr<WrapTexture> getTexture(SDL_Renderer *renderer, std::unique_ptr<ImageCache> &cache) const;
};

template<>
struct std::hash<std::vector<RenderInfo>> {
    size_t operator()(const std::vector<RenderInfo> &infos) const {
        std::ostringstream oss;
        for (auto &info : infos) {
            if (std::holds_alternative<Element>(info)) {
                auto &e = std::get<Element>(info);
                oss << std::hash<Element>()(e);
            }
            else if (std::holds_alternative<ElementWithChildren>(info)) {
                auto &e = std::get<ElementWithChildren>(info);
                oss << std::hash<Method>()(e.method);
                oss << std::hash<int>()(e.x);
                oss << std::hash<int>()(e.y);
                oss << (*this)(e.children);
            }
        }
        return std::hash<std::string>()(oss.str());
    }
};

#endif // ELEMENT_H_
