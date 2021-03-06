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

#include <atomic>
#include <thread>
#include <sdlgui/common.h>
#include <sdlgui/widget.h>
#include <sdlgui/Image.h>
#include <sdlgui/TimeBox.h>

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

        ImageRepository::ImageStoreIndex mBaseIndex;

//        Timer<PassTracker> mTimer;

        EphemerisModel::PassTrackingData mNewTrackingData;
        std::atomic<bool> mNewTrackingDataFlag{false};
        bool mActiveTracking;

        struct PassPlot {
            int x;
            int y;
            double elevation;
            double azimuth;
            ImageData imageData;
        };

        map<string,PassPlot> mPassPlotMap;
        sdlgui::ref<ImageRepository> mImageRepository;

    public:

        ~PassTracker() override = default;
        PassTracker() = delete;
        explicit PassTracker(Widget *parent);
        PassTracker(Widget *parent, const Vector2i &position, const Vector2i &fixedSize);

        /**
         * The timer callback
         * @param interval
         * @return the new interval
         */
        Uint32 timerCallback(Uint32 interval);

        void setObserver(const Observer &observer) { mObserver = observer; }
        sdlgui::ref<PassTracker> withObserver(const Observer &observer) { setObserver(observer); return sdlgui::ref{this}; }
        Observer observer() const { return mObserver; }

        void setImageRepository(sdlgui::ref<ImageRepository> imageRepository, ImageRepository::ImageStoreIndex &baseIndex) {
            mImageRepository = move(imageRepository); mBaseIndex = baseIndex; }
        sdlgui::ref<PassTracker> withIconRepository(sdlgui::ref<ImageRepository> imageRepository, ImageRepository::ImageStoreIndex &baseIndex ) {
            setImageRepository(move(imageRepository), baseIndex);
            return sdlgui::ref<PassTracker>{this};
        }

        bool activeTracking() const { return mActiveTracking; }

        void setPassTrackingData(EphemerisModel::PassTrackingData data);

        bool empty() const { return mPassPlotMap.empty(); }

        /**
         * Override the Widget draw method.
         * @param renderer the renderer to draw to.
         */
        void draw(SDL_Renderer *renderer) override;

        void drawBackground(SDL_Renderer *renderer, int ax, int ay);

        void setPassTrackerVisible(bool v);
    };
}



