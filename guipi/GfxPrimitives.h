//
// Created by richard on 2020-10-09.
//


/*

GfxPrimitives.h graphic primitives for SDL

Copyright (C) 2020 Richard Buckley for adaptations to C++

SDL2_gfxPrimitives.h: graphics primitives for SDL

Copyright (C) 2012-2014  Andreas Schiffler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.

Andreas Schiffler -- aschiffler at ferzkopp dot net

*/

#pragma once

#include <memory>
#include <variant>
#include <SDL.h>

namespace guipi {
    using namespace std;

    struct SurfaceDeleter {
        void operator()(SDL_Surface *surface) {
            SDL_FreeSurface(surface);
        }
    };

    struct Surface : public unique_ptr<SDL_Surface,SurfaceDeleter> {
        auto &pixel(int x, int y) {
            auto *pixels = (Uint32 *) get()->pixels;
            return pixels[(y * get()->w) + x];
        }
    };

    class SurfaceLock {
    protected:
        SDL_Surface *mSurface;

    public:
        ~SurfaceLock() {
            SDL_UnlockSurface(mSurface);
        }

        SurfaceLock() = delete;
        explicit SurfaceLock(SDL_Surface *surface) : mSurface(surface) {
            SDL_LockSurface(mSurface);
        }
    };

    class RendererDeleter {
        void operator()(SDL_Renderer *renderer) {
            SDL_DestroyRenderer(renderer);
        }
    };

    class Renderer : public unique_ptr<SDL_Renderer,RendererDeleter> {

    };

    typedef variant<Surface, Renderer> Drawable;

    int pixel(Drawable &drawable, Sint16 x, Sint16 y);
    int pixelColor(Drawable &drawable, Sint16 x, Sint16 y, Uint32 color);
    int pixelRGBA(Drawable &drawable, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);



}



