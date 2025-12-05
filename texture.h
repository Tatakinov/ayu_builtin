#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <cassert>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>

class ImageInfo;

class WrapSurface {
    private:
        SDL_Surface *surface_;
    public:
        WrapSurface(int w, int h);
        WrapSurface(ImageInfo &info);
        ~WrapSurface();
        SDL_Surface *surface() {
            return surface_;
        }
        int width() const {
            assert(surface_);
            return surface_->w;
        }
        int height() const {
            assert(surface_);
            return surface_->h;
        }
};

class WrapTexture {
    private:
        SDL_Texture *texture_;
    public:
        WrapTexture(SDL_Renderer *renderer, int w, int h);
        WrapTexture(SDL_Renderer *renderer, SDL_Surface *surface);
        ~WrapTexture();
        SDL_Texture *texture() {
            return texture_;
        }
        int width() const {
            assert(texture_);
            return texture_->w;
        }
        int height() const {
            assert(texture_);
            return texture_->h;
        }
};

#endif // TEXTURE_H_
