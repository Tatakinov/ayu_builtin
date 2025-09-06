#ifndef TEXTURE_CACHE_H_
#define TEXTURE_CACHE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "element.h"
#include "program.h"
#include "seriko.h"
#include "texture.h"

class TextureCache {
    private:
        std::unordered_map<Element, std::unique_ptr<Texture>> elements_;
        std::unordered_map<std::vector<RenderInfo>, std::unique_ptr<Texture>> cache_;
    public:
        TextureCache() {}
        ~TextureCache() {
            clearCache();
        }
        std::unique_ptr<Texture> &get(std::unique_ptr<ImageCache> &cache, const Element &e, const bool use_self_alpha);
        std::unique_ptr<Texture> &get(std::unique_ptr<ImageCache> &cache, const std::vector<RenderInfo> &key, const std::unique_ptr<Program> &program, const bool use_self_alpha);
        void clearCache();
};

#endif // TEXTURE_CACHE_H_
