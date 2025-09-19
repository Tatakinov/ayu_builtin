#include "texture_cache.h"

#include <algorithm>
#include "glad/glad.h"
#include <glm/gtc/matrix_transform.hpp>

#include "image_cache.h"
#include "logger.h"

namespace {
    const int inf = 1000000;
    // OpenGLはテクスチャを上下反転で保持するので
    // フレームバッファを用いてのテクスチャへの描画も反転させる
    glm::mat4 view = glm::lookAt(
            glm::vec3(0, 0, -1),
            glm::vec3(0, 0, 0),
            glm::vec3(0, -1, 0)
            );
}

class FrameBuffer {
    private:
        GLuint id_;
    public:
        FrameBuffer() {
            glGenFramebuffers(1, &id_);
            assert(glGetError() == GL_NO_ERROR);
            bind();
        }
        void bind() {
            glBindFramebuffer(GL_FRAMEBUFFER, id_);
            assert(glGetError() == GL_NO_ERROR);
        }
        ~FrameBuffer() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            assert(glGetError() == GL_NO_ERROR);
            glDeleteFramebuffers(1, &id_);
            assert(glGetError() == GL_NO_ERROR);
        }
};

std::unique_ptr<Texture> &TextureCache::get(std::unique_ptr<ImageCache> &cache, const Element &e, const std::unique_ptr<Program> &program, const bool use_self_alpha, bool &regenerate) {
    bool load_required = true;
    if (elements_.contains(e)) {
        auto &t = elements_.at(e);
        auto &info = cache->get(e.filename);
        Logger::log("texture: ", t->isUpconverted());
        Logger::log("info   : ", info->isUpconverted());
        if (t->isUpconverted() == info->isUpconverted()) {
            load_required = false;
        }
        else {
            regenerate = true;
        }
    }
    if (load_required) {
        auto t = std::make_unique<Texture>(cache, e, use_self_alpha);
        if (!*t) {
            Logger::log("invalid texture");
            elements_[e] = std::make_unique<Texture>();
        }
        else {
            Logger::log("t1     : ", t->isUpconverted());
            FrameBuffer fb;
            auto texture = std::make_unique<Texture>(t->rect(), t->region());
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture->id(), 0);
            assert(glGetError() == GL_NO_ERROR);
            GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, buffers);
            assert(glGetError() == GL_NO_ERROR);
            fb.bind();
            program->use(view);
            glClearColor(0.0, 0.0, 0.0, 0.0);
            assert(glGetError() == GL_NO_ERROR);
            glClear(GL_COLOR_BUFFER_BIT);
            assert(glGetError() == GL_NO_ERROR);
            auto [x, y, w, h] = t->rect();
            // テクスチャは上下反転で保持されているので
            // 素直に引数を取ってよい(左上原点とした座標変換はいらない)
            glViewport(x, y, w, h);
            assert(glGetError() == GL_NO_ERROR);
            program->set(t->id());
            switch (e.method) {
                case Method::Base:
                case Method::Add:
                case Method::Overlay:
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                    break;
                case Method::OverlayFast:
                    glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                    break;
                case Method::OverlayMultiply:
                    // FIXME
                    glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ONE, GL_ONE);
                    break;
                case Method::Replace:
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ZERO, GL_ONE, GL_ONE);
                    break;
                case Method::Interpolate:
                    glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                    break;
                case Method::Reduce:
                    // FIXME
                    glBlendFuncSeparate(GL_ZERO, GL_SRC_ALPHA, GL_ZERO, GL_ONE);
                    break;
                default:
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                    break;
            }
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            assert(glGetError() == GL_NO_ERROR);
            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            Logger::log("t2     : ", t->isUpconverted());
            if (t->isUpconverted()) {
                texture->upconverted();
            }
            elements_[e] = std::move(texture);
        }
    }
    Logger::log("return texture: ", elements_.at(e)->isUpconverted());
    return elements_.at(e);
}

std::unique_ptr<Texture> &TextureCache::get(std::unique_ptr<ImageCache> &cache, const std::vector<RenderInfo> &key, const std::unique_ptr<Program> &program, const bool use_self_alpha, bool &regenerate) {
    bool generate_required = true;
    if (cache_.contains(key)) {
        auto &t = cache_.at(key);
        generate_required = false;
        if (!t->isUpconverted()) {
            for (auto &info : key) {
                bool generated = false;
                if (std::holds_alternative<Element>(info)) {
                    get(cache, std::get<Element>(info), program, use_self_alpha, generated);
                }
                else {
                    get(cache, std::get<ElementWithChildren>(info).children, program, use_self_alpha, generated);
                }
                if (generated) {
                    generate_required = true;
                    regenerate = true;
                    break;
                }
            }
        }
    }
    if (generate_required) {
        bool _ = false;
        Rect r = {inf, inf, 0, 0};
        std::vector<Rect> region_sum;
        for (auto &info : key) {
            if (std::holds_alternative<Element>(info)) {
                auto &e = std::get<Element>(info);
                auto &t = get(cache, e, program, use_self_alpha, _);
                if (*t) {
                    auto [x, y, w, h] = t->rect();
                    r.x = std::min(r.x, x);
                    r.y = std::min(r.y, y);
                    r.width = std::max(r.width, x + w);
                    r.height = std::max(r.height, y + h);
                    auto &region = t->region();
                    region_sum.reserve(region_sum.size() + region.size());
                    std::copy(region.begin(), region.end(), std::back_inserter(region_sum));
                }
            }
            else if (std::holds_alternative<ElementWithChildren>(info)) {
                auto &e = std::get<ElementWithChildren>(info);
                if (e.children.size() > 0) {
                    auto &t = get(cache, e.children, program, use_self_alpha, _);
                    if (*t) {
                        auto [x, y, w, h] = t->rect();
                        r.x = std::min(r.x, x);
                        r.y = std::min(r.y, y);
                        r.width = std::max(r.width, x + w);
                        r.height = std::max(r.height, y + h);
                        auto &region = t->region();
                        region_sum.reserve(region_sum.size() + region.size());
                        std::copy(region.begin(), region.end(), std::back_inserter(region_sum));
                    }
                }
            }
        }
        if (region_sum.size() > 0) {
            std::vector<Rect> region;
            std::sort(region_sum.begin(), region_sum.end(), [](const auto &a, const auto &b) {
                if (a.y != b.y) {
                    return a.y < b.y;
                }
                if (a.x != b.x) {
                    return a.x < b.x;
                }
                return a.width < b.width;
            });
            for (auto &r : region_sum) {
                assert(r.height == 1);
                if (region.size() == 0) {
                    region.push_back(r);
                }
                else {
                    auto &last = region.back();
                    if (last.y != r.y) {
                        region.push_back(r);
                    }
                    else if (last.x + last.width >= r.x) {
                        if (last.x + last.width < r.x + r.width) {
                            last.width = r.x + r.width - last.x;
                        }
                    }
                    else {
                        region.push_back(r);
                    }
                }
            }
            bool is_upconverted = true;
            FrameBuffer fb;
            std::unique_ptr<Texture> texture = std::make_unique<Texture>(r, region);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture->id(), 0);
            assert(glGetError() == GL_NO_ERROR);
            GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, buffers);
            assert(glGetError() == GL_NO_ERROR);
            fb.bind();
            program->use(view);
            glClearColor(0.0, 0.0, 0.0, 0.0);
            assert(glGetError() == GL_NO_ERROR);
            glClear(GL_COLOR_BUFFER_BIT);
            assert(glGetError() == GL_NO_ERROR);
            for (auto &info : key) {
                std::unique_ptr<Texture> &t = (std::holds_alternative<Element>(info)) ? (get(cache, std::get<Element>(info), program, use_self_alpha, _)) : (get(cache, std::get<ElementWithChildren>(info).children, program, use_self_alpha, _));
                if (*t) {
                    is_upconverted = is_upconverted && t->isUpconverted();
                    auto [x, y, w, h] = t->rect();
                    // patternの場合のoffsetを考慮。
                    Offset offset;
                    if (std::holds_alternative<Element>(info)) {
                        // すでにoffset考慮済みなので(0, 0)とする
                        offset = {0, 0};
                    }
                    else {
                        auto &e = std::get<ElementWithChildren>(info);
                        offset = {e.x, e.y};
                    }
                    // テクスチャは上下反転で保持されているので
                    // 素直に引数を取ってよい(左上原点とした座標変換はいらない)
                    glViewport(offset.x + x, offset.y + y, w, h);
                    assert(glGetError() == GL_NO_ERROR);
                    program->set(t->id());
                    std::visit([](const auto &e) {
                        switch (e.method) {
                            case Method::Base:
                            case Method::Add:
                            case Method::Overlay:
                                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                                break;
                            case Method::OverlayFast:
                                glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                                break;
                            case Method::OverlayMultiply:
                                // FIXME
                                glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ONE, GL_ONE);
                                break;
                            case Method::Replace:
                                glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ONE);
                                break;
                            case Method::Interpolate:
                                glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                                break;
                            case Method::Reduce:
                                // FIXME
                                glBlendFuncSeparate(GL_ZERO, GL_SRC_ALPHA, GL_ZERO, GL_ONE);
                                break;
                            default:
                                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                                break;
                        }
                    }, info);
                    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                    assert(glGetError() == GL_NO_ERROR);
                    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
                }
            }
            if (is_upconverted) {
                Logger::log("texture upconverted!");
                texture->upconverted();
            }
            cache_[key] = std::move(texture);
            Logger::log("upcon: ", cache_.at(key)->isUpconverted());
        }
        else {
            cache_[key] = std::make_unique<Texture>();
        }
    }
    return cache_.at(key);
}

std::unique_ptr<Texture> &TextureCache::get(std::unique_ptr<ImageCache> &cache, const std::vector<RenderInfo> &key, const std::unique_ptr<Program> &program, const bool use_self_alpha) {
    bool _ = false;
    return get(cache, key, program, use_self_alpha, _);
}

void TextureCache::clearCache(bool full) {
    if (full) {
        elements_.clear();
    }
    cache_.clear();
}
