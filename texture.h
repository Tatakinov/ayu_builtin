#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

#include "glad/glad.h"
#include "misc.h"
#include "surface.h"

class ImageInfo {
    private:
        unsigned char *p_;
        int width_, height_, bpp_;
    public:
        ImageInfo(std::string filename);
        ~ImageInfo();
        unsigned char *get() const {
            return p_;
        }
        int width() const {
            return width_;
        }
        int height() const {
            return height_;
        }
};

class Texture {
    private:
        GLuint id_;
        Rect r_;
        bool valid_;
        std::vector<Rect> region_;
    public:
        Texture() : valid_(false) {}
        Texture(Rect r, const std::vector<Rect> &region);
        Texture(const Element &e);
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
        ~Texture() {
            glDeleteTextures(1, &id_);
        }
};

#endif // TEXTURE_H_
