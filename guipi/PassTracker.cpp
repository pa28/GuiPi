//
// Created by richard on 2020-10-09.
//

#include <sdlgui/Image.h>
#include <guipi/GfxPrimitives.h>
#include <guipi/GeoChrono.h>
#include "PassTracker.h"

guipi::PassTracker::PassTracker(sdlgui::Widget *parent, const sdlgui::Vector2i &position,
                                const sdlgui::Vector2i &fixedSize) : Widget(parent),
                                mTimer(*this, &PassTracker::timerCallback, 2000){
    setPosition(position);
    setFixedSize(fixedSize);
}


guipi::PassTracker::PassTracker(sdlgui::Widget *parent) : Widget(parent),
        mTimer(*this, &PassTracker::timerCallback, 2000) {

}


Uint32 guipi::PassTracker::timerCallback(Uint32 interval) {
    DateTime now{true};
    for (auto & plot : mPassPlotMap) {
        plot.second.satellite.predict(now);
        auto [elevation, azimuth, range, rangeRate] = plot.second.satellite.topo(mObserver);
        plot.second.elevation = elevation;
        plot.second.azimuth = azimuth;
        if (!plot.second.passStarted && elevation > 0.)
            plot.second.passStarted = true;
        auto az = RADIANS(plot.second.azimuth);
        auto el = RADIANS(plot.second.elevation);
        auto r = (M_PI_2 - el) / M_PI_2 * 150.;
        plot.second.x = roundToInt(r * sin(az)) + 165;
        plot.second.y = roundToInt( -r * cos(az)) + 165;
    }

    for (auto & pass : mPassPlotMap) {
        if (pass.second.passStarted && pass.second.elevation < 0.) {
            mPassPlotMap.erase(pass.first);
            break;
        }
    }

    if (mPassPlotMap.empty()) {
        dynamic_cast<GeoChrono*>(parent())->invalidateMapCoordinates();
        setVisible(false);
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
        dst = clip_rects(dst,clipRect);

        ImageRepository::ImageStoreIndex imageStoreIndex{0, 1};
        if (mBackground) {
            SDL_RenderCopy(renderer, mBackground.get(), &src, &dst);
            for (auto & plot : mPassPlotMap) {
                if (plot.second.imageData.dirty) {
                    plot.second.imageData.set(mTheme->getTexAndRectUtf8(renderer, 0, 0, plot.first.c_str(),
                                              mTheme->mBoldFont.c_str(), 15, mTheme->mTextColor));
                }
                auto iconSize = mImageRepository->imageSize(imageStoreIndex);
                SDL_Rect iconSrc{0, 0, iconSize.x, iconSize.y};
                SDL_Rect iconDst{ ax + plot.second.x - iconSize.x/2, ay + plot.second.y - iconSize.y/2, iconSize.x, iconSize.y };
                mImageRepository->renderCopy(renderer, imageStoreIndex, iconSrc, iconDst);

                SDL_Rect labelSrc{ 0, 0, plot.second.imageData.w, plot.second.imageData.h};
                // Default location below and right.
                SDL_Rect labelDst{ ax + plot.second.x - iconSize.x/2, ay + plot.second.y + iconSize.y/4,
                                   plot.second.imageData.w, plot.second.imageData.h};
                // If on right side, pull it back left
                if (plot.second.x > mSize.x/2)
                    labelDst.x -= plot.second.imageData.w - iconSize.x;
                // If on the bottom half, pull it up.
                if (plot.second.y > mSize.y/2)
                    labelDst.y -= iconSize.y/2 + plot.second.imageData.h;
                SDL_RenderCopy(renderer, plot.second.imageData.get(), &labelSrc, &labelDst);
            }
        } else {
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

void guipi::PassTracker::addSatellite(Satellite &satellite) {
    bool found = false;
    string name = std::string(satellite.getName());
    mPassPlotMap[name] = PassPlot{name, 0, 0, 0, 0, false, satellite};

    auto tracking = dynamic_cast<GeoChrono*>(parent())->satelliteDisplay();
    if (tracking && !mPassPlotMap.empty()) {
        setVisible(true);
        dynamic_cast<GeoChrono*>(parent())->invalidateMapCoordinates();
    }
}
