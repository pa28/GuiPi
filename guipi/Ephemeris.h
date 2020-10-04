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
#include <vector>
#include <optional>
#include <sdlgui/common.h>
#include <sdlgui/Image.h>
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
            for (auto & ephemeris : satelliteEphemerisMap) {
                cout << ephemeris.first << '\n';
            }
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

    enum class PlotItemType {
        GEO_LOCATION,
        CELESTIAL_BODY,
        EARTH_SATELLITE,
    };

    class PlotPackage {
    protected:
        PlotItemType plotItemType{};
        string name;
        ImageData imageData;
        Vector2f geoCoordinates;
        vector<Vector2f> coastingCoords;
        long coastingInterval{};
        Vector2i mapCoordinates;
        bool mapCoordinatesValid{};

    public:
        PlotPackage() = default;

        ~PlotPackage() = default;

        PlotPackage(PlotPackage &) = delete;

        PlotPackage(const PlotPackage &) = delete;

        auto operator=(PlotPackage &) = delete;

        auto operator=(const PlotPackage &) = delete;

        PlotPackage(PlotPackage &&other) noexcept;

        PlotPackage &operator=(PlotPackage &&other) noexcept;

        PlotPackage(const Ephemeris &ephemeris, const string &plotName, ImageData iconImageData, PlotItemType itemType, uint32_t coastInterval=0, size_t coastCount=0);

        bool rePredict(const Ephemeris &ephemeris);

        bool predict(const Ephemeris &ephemeris, size_t coastingCount);

        [[nodiscard]] PlotItemType getPlotItemType() const { return plotItemType; }
        [[nodiscard]] bool getMapCoordinatesValid() const { return mapCoordinatesValid; }
        [[nodiscard]] Vector2f getGeoCoord() const { return geoCoordinates; }
        [[nodiscard]] Vector2i getMapCoord() const { return mapCoordinates; }
        [[nodiscard]] SDL_Texture *texture() { return imageData.get(); }
        [[nodiscard]] int iconWidth() const { return imageData.w; }
        [[nodiscard]] int iconHeight() const { return imageData.h; }
        void setMapCoordValid(bool valid) { mapCoordinatesValid = valid; }
        void setMapCoord(const Vector2i &mapCoord) { mapCoordinates = mapCoord; mapCoordinatesValid = true; }
        void setGeoCoord(const Vector2f &geoCoord) { geoCoordinates = geoCoord; mapCoordinatesValid = false;}

        PlotPackage(const string &name, ImageData imageData, PlotItemType itemType, Vector2f geoCoord);
    };
}

