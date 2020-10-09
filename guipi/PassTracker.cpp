//
// Created by richard on 2020-10-09.
//

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

        if (!mBackground) {
            mBackgroundThread = thread([this, renderer]() {
                lock_guard<mutex> lockGuard(mBackgroundMutex);
                drawBackground();
            });
        } else {
            lock_guard<mutex> lockGuard(mBackgroundMutex);
        }
    }
}

void guipi::PassTracker::drawBackground() {

}
