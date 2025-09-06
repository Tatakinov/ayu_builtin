#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

#include "glad/glad.h"
#include "image_cache.h"
#include "misc.h"
#include "surface.h"

class Texture {
    private:
        GLuint id_;
        Rect r_;
        bool valid_;
        std::vector<Rect> region_;
        bool is_upconverted_;
    public:
        Texture() : valid_(false), is_upconverted_(false) {}
        Texture(Rect r, const std::vector<Rect> &region);
        Texture(std::unique_ptr<ImageCache> &cache, const Element &e, const bool use_self_alpha);
        ~Texture() {
            glDeleteTextures(1, &id_);
        }
        GLuint id() const {
            assert(valid_);
            return id_;
        }
        Rect rect() const {
            assert(valid_);
            return r_;
        }
        operator bool() const {
            return valid_;
        }
        std::vector<Rect> &region() {
            return region_;
        }
        void upconverted() {
            is_upconverted_ = true;
        }
};

#endif // TEXTURE_H_
