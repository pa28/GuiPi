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

    }
}
