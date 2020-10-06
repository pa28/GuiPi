//
// Created by richard on 2020-10-06.
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

#include <sdlgui/widget.h>
#include <guipi/GeoChrono.h>
#include <sdlgui/TimeBox.h>

namespace guipi {
    using namespace sdlgui;
    using sdlgui::ref;

    class SatelliteDataDisplay : public Widget {
    protected:
        class Satellite : public Widget {
        public:
            Satellite() = delete;
            explicit Satellite(Widget *parent) : Widget(parent) {}

            Satellite(Widget *parent, const PlotPackage &plotPackage);

            void update();

            void update(const PlotPackage &plotPackage);

            static string timeToString(const DateTime &time, const DateTime &now);

            void draw(SDL_Renderer* renderer) override;

        private:
            ref<Widget> mIcon;
            ref<Label> mName;
            ref<Label> mInfo;
            ref<ImageRepository> mImageRepository;
            ImageRepository::ImageStoreIndex mImageIndex;
            DateTime riseTime;
            DateTime setTime;
        };

        Timer<SatelliteDataDisplay> mTimer;
        time_point <system_clock> mEpoch;
        duration<double> mElapsedSeconds{};

    public:
        SatelliteDataDisplay() = delete;

        explicit SatelliteDataDisplay(Widget *parent);

        SatelliteDataDisplay(Widget *parent, const ref <GeoChrono> &geoChrono);

        void updateSatelliteData();

        Uint32 timerCallback(Uint32 interval);

    private:
        ref<GeoChrono> mGeoChrono;
    };
}



