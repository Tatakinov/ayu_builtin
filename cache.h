#ifndef CACHE_H_
#define CACHE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "element.h"
#include "program.h"
#include "seriko.h"
#include "texture.h"

class Cache {
    private:
        std::unordered_map<Element, std::unique_ptr<Texture>> elements_;
        std::unordered_map<std::vector<RenderInfo>, std::unique_ptr<Texture>> cache_;
    public:
        Cache() {}
        ~Cache() {
            cache_.clear();
        }
        std::unique_ptr<Texture> &get(const Element &e, const bool use_self_alpha);
        std::unique_ptr<Texture> &get(const std::vector<RenderInfo> &key, const std::unique_ptr<Program> &program, const bool use_self_alpha);
};

#endif // CACHE_H_
