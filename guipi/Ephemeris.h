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
#include <sdlgui/ImageRepository.h>
#include <guipi/p13.h>
#include <guipi/SatelliteEphemerisFetch.h>
#include <guipi/Earthsat.h>

namespace guipi {
    using namespace sdlgui;
    using namespace std;
    using sdlgui::ref;

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

        [[nodiscard]] Earthsat nextPass(const string &name, const Observer &observer) const {
            Earthsat earthsat{};
            earthsat.FindNextPass(Satellite{satelliteEphemerisMap.at(name)}, observer);
            return earthsat;
        }
    };

    enum PlotItemType {
        GEO_LOCATION,
        GEO_LOCATION_QTH,
        GEO_LOCATION_ANTIPODE,
        CELESTIAL_BODY,
        CELESTIAL_BODY_SUN,
        CELESTIAL_BODY_MOON,
        EARTH_SATELLITE,
    };

    class PlotPackage {
    public:
        PlotItemType mPlotItemType{};
        string mName;
        Vector2f mGeoCoord;
        Vector2i mMapCoord;
        Vector2i mDrawSize;
        bool mMapCoordValid{};
        Earthsat mEarthsat;

        ref<ImageRepository> mImageRepository;
        ImageRepository::ImageStoreIndex mImageIndex;

        PlotPackage() = default;

        ~PlotPackage() = default;

        PlotPackage(PlotPackage &) = default;

        PlotPackage(const PlotPackage &) = default;

        PlotPackage(string plotName, PlotItemType itemType);

        PlotPackage(string_view plotName, PlotItemType itemType) : PlotPackage(string(plotName), itemType) {}

        PlotPackage(string name, PlotItemType itemType, const Vector2f& geoCoord);

        PlotPackage(string_view name, PlotItemType itemType, const Vector2f& geoCoord)
            : PlotPackage(string(name), itemType, geoCoord) {}

        PlotPackage &operator=(const PlotPackage &) = default;

        PlotPackage(PlotPackage &&other) noexcept = default;

        PlotPackage &operator=(PlotPackage &&other) noexcept = default;

        void predict(const Ephemeris &ephemeris);

        void predictPass(const Ephemeris &ephemeris, const Observer &observer);

        [[nodiscard]] bool compareLt(const PlotPackage &o) const {
            if (mPlotItemType == o.mPlotItemType) {
                if (mEarthsat.passFound()) {
                    return mEarthsat.riseTime() < o.mEarthsat.riseTime();
                } else
                    return true;
            } else {
                return mPlotItemType < o.mPlotItemType;
            }
        }

        [[nodiscard]] bool compareGt(const PlotPackage &o) const {
            if (mPlotItemType == o.mPlotItemType) {
                if (mEarthsat.passFound()) {
                    return mEarthsat.riseTime() > o.mEarthsat.riseTime();
                } else
                    return true;
            } else {
                return mPlotItemType > o.mPlotItemType;
            }
        }
    };
}

