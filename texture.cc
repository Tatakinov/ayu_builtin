#include "texture.h"

#include <iostream>

#include "logger.h"

Texture::Texture(Rect r, const std::vector<Rect> &region) : r_(r), valid_(true), region_(region) {
    glGenTextures(1, &id_);
    assert(glGetError() == GL_NO_ERROR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
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

Texture::Texture(std::unique_ptr<ImageCache> &cache, const Element &e, const bool use_self_alpha) : valid_(true) {
    Logger::log("filename: ", e.filename.string());
    auto &info = cache->get(e.filename);
    if (!info) {
        valid_ = false;
        return;
    }
    r_ = { e.x, e.y, info->width(), info->height() };
    const unsigned char *p = info->get().data();
    for (int y = 0; y < info->height(); y++) {
        int begin = -1;
        for (int x = 0; x < info->width(); x++) {
            int index = 4 * (y * info->width() + x);
            int alpha = p[index + 3];
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
            region_.push_back({r_.x + begin, r_.y + y, info->width() - begin, 1});
        }
    }
    glGenTextures(1, &id_);
    assert(glGetError() == GL_NO_ERROR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    assert(glGetError() == GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_2D, id_);
    assert(glGetError() == GL_NO_ERROR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info->width(), info->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, info->get().data());
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
