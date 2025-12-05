#include "texture.h"

#include <cassert>

#include "image_cache.h"

WrapSurface::WrapSurface(int w, int h) {
    surface_ = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ARGB8888);
}

WrapSurface::WrapSurface(ImageInfo &info) {
    surface_ = SDL_CreateSurfaceFrom(info.width(), info.height(), SDL_PIXELFORMAT_ARGB8888, info.get().data(), info.width() * 4);
}

WrapSurface::~WrapSurface() {
    if (surface_ != nullptr) {
        SDL_DestroySurface(surface_);
    }
}


WrapTexture::WrapTexture(SDL_Renderer *renderer, int w, int h) {
    texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
}

WrapTexture::WrapTexture(SDL_Renderer *renderer, SDL_Surface *surface) {
    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
}

WrapTexture::~WrapTexture() {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
    }
}
