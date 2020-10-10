//
// Created by richard on 2020-10-09.
//

#include <sdlgui/Image.h>
#include <guipi/GfxPrimitives.h>
#include "PassTracker.h"

guipi::PassTracker::PassTracker(sdlgui::Widget *parent, const sdlgui::Vector2i &position,
                                const sdlgui::Vector2i &fixedSize) : Widget(parent){
    setPosition(position);
    setFixedSize(fixedSize);
}

Uint32 guipi::PassTracker::timerCallback(Uint32 interval) {
    if (visible()) {
        for (auto &plot : mPlotPackage) {
            // Compute Azimuth-Elevation.
        }
    }

    return interval;
}

void guipi::PassTracker::draw(SDL_Renderer *renderer) {
    Widget::draw(renderer);

    if (visible()) {
        int ax = getAbsoluteLeft();
        int ay = getAbsoluteTop();

        PntRect clip = getAbsoluteCliprect();
        SDL_Rect clipRect = pntrect2srect(clip);

        // If the asynchronous drawing thread is done, join it.
        if (mBackgroundThread.joinable()) {
            mBackgroundThread.join();
        }

        SDL_Rect src{0, 0, mSize.x, mSize.y};
        SDL_Rect dst{ ax, ay, mSize.x, mSize.y};
        if (mBackground) {
            SDL_RenderCopy(renderer, mBackground.get(), &src, &dst);
            SDL_RenderPresent(renderer);
        } else {
//            mBackgroundThread = thread([this, renderer, ax, ay]() {
//                lock_guard<mutex> lockGuard(mBackgroundMutex);
//                drawBackground(renderer, ax, ay);
//            });
                drawBackground(renderer, ax, ay);
        }
    }
}

void guipi::PassTracker::drawBackground(SDL_Renderer *renderer, int ax, int ay) {
    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mSize.x, mSize.y);
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    boxRGBA(renderer, 0, 0, mSize.x, mSize.y, 0x00, 0x00, 0x00, 0x00);
    filledCircleRGBA(renderer, mSize.x/2, mSize.y/2, mSize.x/2, 0, 0, 0, 255);
    aacircleRGBA(renderer, mSize.x/2, mSize.y/2, 150, 0, 255, 0, 255);
    aacircleRGBA(renderer, mSize.x/2, mSize.y/2, 100, 0, 255, 0, 255);
    aacircleRGBA(renderer, mSize.x/2, +mSize.y/2, 50, 0, 255, 0, 255);
    aalineRGBA( renderer, mSize.x/2 - 150, mSize.y/2, mSize.x/2 + 150, mSize.y/2, 0, 255, 0, 255);
    aalineRGBA( renderer, mSize.x/2, mSize.y/2 - 150, mSize.x/2, mSize.y/2 + 150, 0, 255, 0, 255);
    auto quad = roundToInt(cos(M_PI_4) * 150.);
    aalineRGBA( renderer, mSize.x/2 - quad, mSize.y/2 - quad, mSize.x/2 + quad, mSize.y/2 + quad, 0, 255, 0, 255);
    aalineRGBA( renderer, mSize.x/2 + quad, mSize.y/2 - quad, mSize.x/2 - quad, mSize.y/2 + quad, 0, 255, 0, 255);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderPresent(renderer);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    mBackground.set(texture);
}
