#include "texture.h"

#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "logger.h"

ImageInfo::ImageInfo(std::string filename) {
    p_ = stbi_load(filename.c_str(), &width_, &height_, &bpp_, 4);
}

ImageInfo::~ImageInfo() {
    if (p_ != nullptr) {
        stbi_image_free(p_);
    }
}

Texture::Texture(Rect r, const std::vector<Rect> &region) : r_(r), valid_(true), region_(region) {
    glGenTextures(1, &id_);
    assert(glGetError() == GL_NO_ERROR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    assert(glGetError() == GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_2D, id_);
    assert(glGetError() == GL_NO_ERROR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, r_.width, r_.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    assert(glGetError() == GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    assert(glGetError() == GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    assert(glGetError() == GL_NO_ERROR);
}

Texture::Texture(const Element &e) : valid_(true) {
    Logger::log("filename: ", e.filename.string());
    ImageInfo info(e.filename.string());
    if (info.get() == nullptr) {
        valid_ = false;
        return;
    }
    auto dir = e.filename.parent_path();
    // TODO seriko.use_alpha
    auto pna_filename = dir / e.filename.stem();
    pna_filename += ".pna";
    ImageInfo pna(pna_filename.string());
    if (pna.get() != nullptr && info.width() == pna.width() && info.height() == pna.height()) {
        Logger::log("use pna file");
        unsigned char *dest = info.get();
        unsigned char *src = pna.get();
        for (int y = 0; y < pna.height(); y++) {
            for (int x = 0; x < pna.width(); x++) {
                int alpha = src[4 * (y * pna.width() + x)];
                dest[4 * (y * pna.width() + x) + 3] = alpha;
            }
        }
    }
    else {
        Logger::log("use left-top transparent");
        unsigned char *p = info.get();
        int transparent_r = p[0];
        int transparent_g = p[1];
        int transparent_b = p[2];
        for (int y = 0; y < info.height(); y++) {
            for (int x = 0; x < info.width(); x++) {
                int r = p[4 * (y * info.width() + x) + 0];
                int g = p[4 * (y * info.width() + x) + 1];
                int b = p[4 * (y * info.width() + x) + 2];
                if (r == transparent_r && g == transparent_g && b == transparent_b) {
                    p[4 * (y * info.width() + x) + 3] = 0;
                }
            }
        }
    }
    r_ = { e.x, e.y, info.width(), info.height() };
    unsigned char *p = info.get();
    for (int y = 0; y < info.height(); y++) {
        int begin = -1;
        for (int x = 0; x < info.width(); x++) {
            int alpha = p[4 * (y * info.width() + x) + 3];
            if (alpha) {
                if (begin == -1) {
                    begin = x;
                }
            }
            else {
                if (begin != -1) {
                    region_.push_back({r_.x + begin, r_.y + y, x - begin, 1});
                    begin = -1;
                }
            }
        }
        if (begin != -1) {
            region_.push_back({r_.x + begin, r_.y + y, info.width() - begin, 1});
        }
    }
    glGenTextures(1, &id_);
    assert(glGetError() == GL_NO_ERROR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    assert(glGetError() == GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_2D, id_);
    assert(glGetError() == GL_NO_ERROR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width(), info.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, info.get());
    assert(glGetError() == GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    assert(glGetError() == GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    assert(glGetError() == GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    assert(glGetError() == GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(glGetError() == GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_2D, 0);
    assert(glGetError() == GL_NO_ERROR);
}
