//
// Created by richard on 2020-10-03.
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

#include <string>
#include <optional>
#include <sdlgui/common.h>
#include <guipi/p13.h>
#include <guipi/SatelliteEphemerisFetch.h>

namespace guipi {
    using namespace sdlgui;
    using namespace std;

    class Ephemeris {
    protected:
        SatelliteEphemerisMap satelliteEphemerisMap;

    public:
        void loadEphemeris() {
            satelliteEphemerisMap = SatelliteEphemerisFetch::fetchAll();
        }

        [[nodiscard]] bool haveEphemeris(const string &name) const {
            return satelliteEphemerisMap.find(name) != satelliteEphemerisMap.cend();
        }

        [[nodiscard]] optional<Vector2f> predict(const string &name) const {
            DateTime predictionTime;
            predictionTime.userNow();
            return predict(name, predictionTime);
        }

        [[nodiscard]] optional<Vector2f> predict(const string &name, const DateTime &predictionTime) const {
            try {
                Satellite satellite{satelliteEphemerisMap.at(name)};
                satellite.predict(predictionTime);
                auto[lat, lon] = satellite.geo();
                return Vector2f{(float) lon, (float) lat};
            }

            catch (const out_of_range &e) {
                return nullopt;
            }
        }
    };
}

