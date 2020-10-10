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
#include <utility>
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

#if __cplusplus == 201703L

#else
    template<typename T>
    class optional {
    protected:
        bool valid{false};
        T content{};

    public:
        ~optional() = default;
        optional() = default;
        explicit optional(const T &v) : valid(true), content(v) {}
        explicit optional(T &&v) : valid(true), content(move(v)) {}
        optional(const optional<T> &o) : valid(o.valid), content(o.content) {}
        optional(optional<T> &&o)  noexcept : valid(o.valid), content(move(o.content)) {}
        optional<T> &operator=(const optional<T> &o) { valid = o.valid; content = o.content; return *this; }
        optional<T> &operator=(optional<T> &&o)  noexcept { valid = o.valid; content = move(o.content); return *this; }

        explicit operator bool() { return valid; }
        T value() { return content; }
        T value() const { return content; }
    };
#endif

    class Ephemeris {
    protected:
        SatelliteEphemerisMap satelliteEphemerisMap;

    public:
        void loadEphemeris() {
            satelliteEphemerisMap = SatelliteEphemerisFetch::fetchAll();
        }

        auto begin() noexcept { return satelliteEphemerisMap.begin(); }

        auto end() noexcept { return satelliteEphemerisMap.end(); }

        [[nodiscard]] bool haveEphemeris(const std::string &name) const {
            return satelliteEphemerisMap.find(name) != satelliteEphemerisMap.cend();
        }

        [[nodiscard]] optional<Vector2f> predict(const string &name) const {
            DateTime predictionTime;
            predictionTime.userNow();
            return predict(name, predictionTime);
        }

        [[nodiscard]] optional<Satellite> satellite(const string &name) const {
            if (haveEphemeris(name)) {
                return Satellite{satelliteEphemerisMap.at(name)};
            }
            return nullopt;
        }

        [[nodiscard]] optional<Vector2f> predict(const string &name, const DateTime &predictionTime) const {
            if (haveEphemeris(name)) {
                Satellite satellite{satelliteEphemerisMap.at(name)};
                satellite.predict(predictionTime);
#if __cplusplus == 201703L
                auto[lat, lon] = satellite.geo();
#else
                auto _t = satellite.geo();
                auto lat = std::get<0>(_t);
                auto lon = std::get<1>( _t);
#endif
                return optional<Vector2f>{Vector2f{(float) lon, (float) lat}};
            }
            return nullopt;
        }

        [[nodiscard]] Earthsat nextPass(const string &name, const Observer &observer) const {
            Earthsat earthsat{};
            earthsat.FindNextPass(Satellite{satelliteEphemerisMap.at(name)}, observer);
            return earthsat;
        }

        [[nodiscard]] bool empty() const { return satelliteEphemerisMap.empty(); }
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

#if __cplusplus == 201703L
        PlotPackage(string_view plotName, PlotItemType itemType) : PlotPackage(string(plotName), itemType) {}
#else
        PlotPackage(const char *plotName, PlotItemType itemType) : PlotPackage(string(plotName), itemType) {}
#endif

        PlotPackage(string name, PlotItemType itemType, const Vector2f& geoCoord);

#if __cplusplus == 201703L
        PlotPackage(string_view name, PlotItemType itemType, const Vector2f& geoCoord)
#else
        PlotPackage(const char *name, PlotItemType itemType, const Vector2f& geoCoord)
#endif
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

