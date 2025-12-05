#include "element.h"

#include <cassert>

#include "logger.h"

namespace {
    const int kInf = 1000000;
}

std::unique_ptr<WrapTexture> ElementWithChildren::getTexture(SDL_Renderer *renderer, std::unique_ptr<ImageCache> &cache) const {
    int w = 0, h = 0;
    std::vector<std::unique_ptr<WrapTexture>> list;
    for (auto &element : children) {
        std::visit([&](const auto &e) {
            auto t = e.getTexture(renderer, cache);
            if (!t) {
                Logger::log("invalid texture");
                return;
            }
            if (w < t->width()) {
                w = t->width();
            }
            if (h < t->height()) {
                h = t->height();
            }
            list.push_back(std::move(t));
        }, element);
    }
    if (w == 0 || h == 0) {
        std::unique_ptr<WrapTexture> invalid;
        Logger::log("no valid children");
        return invalid;
    }
    auto texture = std::make_unique<WrapTexture>(renderer, w, h);
#if 0
    SDL_BlendMode mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
    switch (method) {
        case Method::Base:
        case Method::Add:
        case Method::Overlay:
            SDL_SetRenderDrawBlendMode(renderer, mode);
            break;
        case Method::OverlayFast:
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetRenderDrawBlendMode(renderer, mode);
            break;
        case Method::OverlayMultiply:
            // FIXME
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetRenderDrawBlendMode(renderer, mode);
            break;
        case Method::Replace:
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetRenderDrawBlendMode(renderer, mode);
            break;
        case Method::Interpolate:
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE_MINUS_DST_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetRenderDrawBlendMode(renderer, mode);
            break;
        case Method::Reduce:
            // FIXME
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetRenderDrawBlendMode(renderer, mode);
            break;
        default:
            SDL_SetRenderDrawBlendMode(renderer, mode);
            break;
    }
#endif
    SDL_SetRenderTarget(renderer, texture->texture());
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    for (auto &t : list) {
        SDL_FRect r = { x, y, t->width(), t->height() };
        SDL_RenderTexture(renderer, t->texture(), nullptr, &r);
    }
    return texture;
}

std::unique_ptr<WrapSurface> ElementWithChildren::getSurface(std::unique_ptr<ImageCache> &cache) const {
    int w = 0, h = 0;
    std::vector<std::unique_ptr<WrapSurface>> list;
    for (auto &element : children) {
        std::visit([&](const auto &e) {
            auto t = e.getSurface(cache);
            if (!t) {
                Logger::log("invalid surface");
                return;
            }
            if (w < t->width()) {
                w = t->width();
            }
            if (h < t->height()) {
                h = t->height();
            }
            list.push_back(std::move(t));
        }, element);
    }
    if (w == 0 || h == 0) {
        std::unique_ptr<WrapSurface> invalid;
        Logger::log("no valid children");
        return invalid;
    }
    auto surface = std::make_unique<WrapSurface>(w, h);
    SDL_BlendMode mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
    switch (method) {
        case Method::Base:
        case Method::Add:
        case Method::Overlay:
            SDL_SetSurfaceBlendMode(surface->surface(), mode);
            break;
        case Method::OverlayFast:
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetSurfaceBlendMode(surface->surface(), mode);
            break;
        case Method::OverlayMultiply:
            // FIXME
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetSurfaceBlendMode(surface->surface(), mode);
            break;
        case Method::Replace:
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetSurfaceBlendMode(surface->surface(), mode);
            break;
        case Method::Interpolate:
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE_MINUS_DST_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetSurfaceBlendMode(surface->surface(), mode);
            break;
        case Method::Reduce:
            // FIXME
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
            SDL_SetSurfaceBlendMode(surface->surface(), mode);
            break;
        default:
            SDL_SetSurfaceBlendMode(surface->surface(), mode);
            break;
    }
    for (auto &t : list) {
        SDL_Rect r = { x, y, t->width(), t->height() };
        SDL_BlitSurface(t->surface(), nullptr, surface->surface(), &r);
    }
    return surface;
}
