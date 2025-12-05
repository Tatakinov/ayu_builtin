#include "surface.h"

#include "image_cache.h"
#include "logger.h"

std::unique_ptr<WrapTexture> Element::getTexture(SDL_Renderer *renderer, std::unique_ptr<ImageCache> &cache) const {
    auto &info = cache->get(filename);
    if (!info) {
        Logger::log("invalid info");
        std::unique_ptr<WrapTexture> invalid;
        return invalid;
    }
    WrapSurface surface(info.value());
    auto src = std::make_unique<WrapTexture>(renderer, surface.surface());
    auto dst = std::make_unique<WrapTexture>(renderer, x + surface.width(), y + surface.height());
    SDL_BlendMode mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
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
            mode = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
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
    SDL_SetRenderTarget(renderer, dst->texture());
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    SDL_FRect r = { x, y, src->width(), src->height() };
    SDL_RenderTexture(renderer, src->texture(), nullptr, &r);
    return dst;
}
