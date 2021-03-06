//
// Created by richard on 2020-10-11.
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

#include <array>
#include <future>
#include <mutex>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <sdlgui/TimeBox.h>
#include <guipi/p13.h>

namespace guipi {
    template<typename T>
    constexpr T deg2rad(T deg) { return deg * M_PI / 180.; }

    template<typename T>
    constexpr T rad2deg(T rad) { return rad * 180. / M_PI; }

    std::tuple<double, double> subSolar();

    class Earthsat {
    private:
        bool set_ok = false, rise_ok = false, ever_up = false, ever_down = false;
        DateTime set_time, rise_time;
        double set_az, rise_az, max_elevation;

    public:
        constexpr static double SAT_MIN_EL = 1.0;   // minimum sat elevation for event
        constexpr static long COARSE_DT = 90;        // seconds/step forward for fast search
        constexpr static long FINE_DT = (-2L);    // seconds/step backward for refined search

        void FindNextPass(Satellite const &satellite, Observer const &observer);

        [[nodiscard]] auto passFound() const { return rise_ok && set_ok; }

        [[nodiscard]] auto riseTime() const { return rise_time; }

        [[nodiscard]] auto setTime() const { return set_time; }

        [[nodiscard]] auto riseAzimuth() const { return rise_az; }

        [[nodiscard]] auto setAzimuth() const { return set_az; }

        operator bool() const { return rise_ok && set_ok && ever_down && ever_up; }

        bool isEverUp() const { return ever_up; }

        double maxElevation() const { return max_elevation; }

        void roundPassTimes();
    };

    constexpr static std::string_view URL_FETCH_NAME = "http://clearskyinstitute.com/ham/HamClock/esats.pl?tlename=";
    constexpr static std::string_view URL_FETCH_ALL = "http://clearskyinstitute.com/ham/HamClock/esats.pl?getall=";
    constexpr static std::string_view CT_AMATEUR = "https://www.celestrak.com/NORAD/elements/amateur.txt";
    constexpr static std::string_view CT_BRIGHT = "https://www.celestrak.com/NORAD/elements/visual.txt";
    constexpr static std::string_view CT_CUBESAT = "https://www.celestrak.com/NORAD/elements/cubesat.txt";

    typedef std::array<std::string, 3> SatelliteEphemeris;

    inline std::ostream& operator<<(std::ostream& os, const SatelliteEphemeris &satelliteEphemeris) {
        return os << satelliteEphemeris[0] << '\n'
                  << satelliteEphemeris[1] << '\n'
                  << satelliteEphemeris[2] << '\n';
    }

    typedef std::map<std::string, SatelliteEphemeris> SatelliteEphemerisMap;

    class EphemerisModel {
    public:
        typedef std::tuple<std::string, DateTime, DateTime> PassData;
        typedef std::vector<PassData> PassMonitorData;
        typedef std::tuple<std::string, double, double> OrbitData;
        typedef std::vector<OrbitData> OrbitTrackingData;
        typedef std::tuple<std::string, double, double, double, double> TrackData;
        typedef std::vector<TrackData> PassTrackingData;
        typedef std::tuple<float, float, std::pair<size_t, size_t>> CelestialData;
        typedef std::vector<CelestialData> CelestialTrackingData;

        typedef std::function<void(PassMonitorData)> PassMonitorCallback;
        typedef std::function<void(OrbitTrackingData)> OrbitTrackingCallback;
        typedef std::function<void(PassTrackingData)> PassTrackingCallback;
        typedef std::function<void(CelestialTrackingData)> CelestialTrackingCallback;

    protected:
        size_t mDivider;
        bool mInitialize;
        std::string mSatelliteNameList;

        sdlgui::ref<Settings> mSettings;

        SatelliteEphemerisMap mSatelliteEphemerisMap{};
        SatelliteEphemerisMap mNewSatelliteEphemerisMap{};
        std::map<std::string,Satellite> mSatellitesOfInterest{};

        PassMonitorData mSatellitePassData{};
        OrbitTrackingData mSatelliteOrbitData{};
        PassTrackingData mSatelliteTrackData{};

        std::mutex mEphemerisLibraryMutex;
        std::future<bool> mEphemerisLibaryLoad{};
        static bool asyncEphemerisFetch(EphemerisModel *self, int source);

        sdlgui::Timer<EphemerisModel> mPredictionTimer;

        PassMonitorCallback mPassMonitorCallback{};
        PassTrackingCallback mPassTrackingCallback{};
        OrbitTrackingCallback mOrbitTrackingCallback{};
        CelestialTrackingCallback mCelestialTrackingCallback{};

        std::optional<Satellite> getSatellite(const std::string &name);

        int setSatellitesOfInterestImpl(const std::string &satelliteNameList);

    public:
        EphemerisModel();

        void setSettings(sdlgui::ref<Settings> settings);

        void loadEphemerisLibrary(int source = 0);

        void loadEphemerisLibraryWait(int source = 0);

        int setSatellitesOfInterest(const std::string &satelliteNameList = "");

        Uint32 timerCallback(Uint32 interval);

        void setPassMonitorCallback(PassMonitorCallback callback) { mPassMonitorCallback = move(callback); }

        void setPassTrackingCallback(PassTrackingCallback callback) { mPassTrackingCallback = move(callback); }

        void setOrbitTrackingCallback(OrbitTrackingCallback callback) { mOrbitTrackingCallback = move(callback); }

        void setCelestialTrackingCallback(CelestialTrackingCallback callback) { mCelestialTrackingCallback = move(callback); }

        [[nodiscard]] PassMonitorData getPassMonitorData() const { return mSatellitePassData; }

        [[nodiscard]] SatelliteEphemerisMap getSatelliteEphemerisMap() const { return mSatelliteEphemerisMap; }

        SatelliteEphemerisMap fetchAll(int source);
    };
}



