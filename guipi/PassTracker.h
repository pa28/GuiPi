//
// Created by richard on 2020-10-09.
//


/*
    Another significant redesign to update the coding standards to C++17,
    reduce the amount of bare pointer handling (especially in user code),
    and focus on the RaspberryPi environment.
    
    License terms for the changes as well as the base nanogui-sdl code are
    contained int the LICENSE.txt file.
    
    A significant redesign of this code was contributed by Christian Schueller.

    Based on NanoGUI by Wenzel Jakob <wenzel@inf.ethz.ch>.
    Adaptation for SDL by Dalerank <dalerankn8@gmail.com>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include <thread>
#include <sdlgui/common.h>
#include <sdlgui/widget.h>
#include <sdlgui/Image.h>
#include <sdlgui/TimeBox.h>
#include <guipi/Ephemeris.h>

namespace guipi {
    using namespace sdlgui;
    using namespace std;
    /**
     * @class PassTracker
     * A helper Widget for GeoChrono that provides Azimuth-Elevation tracking for
     * statellites in view of the station.
     */
    class PassTracker : public Widget {
    protected:

        mutex mBackgroundMutex;
        thread mBackgroundThread;
        ImageData mForeground;      //< The foreground image - items being tracked
        ImageData mBackground;      //< The background image - the Az-El grid

        Observer mObserver{};       //< Location of the observer

        vector<PlotPackage> mPlotPackage{};     //< A list of items being tracked

    public:

        ~PassTracker() override = default;
        PassTracker() = delete;
        explicit PassTracker(Widget *parent) : Widget(parent) {}
        PassTracker(Widget *parent, const Vector2i &position, const Vector2i &fixedSize);

        void trackObject(const PlotPackage &plotPackage) { mPlotPackage.push_back(plotPackage); }

        void setObserver(const Observer &observer) { mObserver = observer; }
        sdlgui::ref<PassTracker> withObserver(const Observer &observer) { setObserver(observer); return sdlgui::ref{this}; }
        Observer observer() const { return mObserver; }

        /**
         * The timer callback
         * @param interval
         * @return the new interval
         */
        Uint32 timerCallback(Uint32 interval);

        /**
         * Override the Widget draw method.
         * @param renderer the renderer to draw to.
         */
        void draw(SDL_Renderer *renderer) override;

        void drawBackground(SDL_Renderer *renderer, int ax, int ay);
    };
}



