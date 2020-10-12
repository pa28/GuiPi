//
// Created by richard on 2020-10-11.
//

#include <vector>
#include <iostream>
#include <sstream>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include "EphemerisModel.h"

namespace guipi {

    /**
     * Compute the sub-solar geographic coordinates, used in plotting the solar ilumination.
     * @return a tuple with the latitude, longitude in radians
     */
    std::tuple<double, double> subSolar() {
        using namespace std::chrono;
        auto epoch = system_clock::now();
        time_t tt = system_clock::to_time_t(epoch);

        double JD = (tt / 86400.0) + 2440587.5;
        double D = JD - 2451545.0;
        double g = 357.529 + 0.98560028 * D;
        double q = 280.459 + 0.98564736 * D;
        double L = q + 1.915 * sin(M_PI / 180 * g) + 0.020 * sin(M_PI / 180 * 2 * g);
        double e = 23.439 - 0.00000036 * D;
        double RA = 180 / M_PI * atan2(cos(M_PI / 180 * e) * sin(M_PI / 180 * L), cos(M_PI / 180 * L));
        auto lat = asin(sin(M_PI / 180 * e) * sin(M_PI / 180 * L));
        auto lat_d = rad2deg(lat);
        double GMST = fmod(15 * (18.697374558 + 24.06570982441908 * D), 360.0);
        auto lng_d = fmod(RA - GMST + 36000.0 + 180.0, 360.0) - 180.0;
        auto lng = deg2rad(lng_d);

        return std::make_tuple(lat, lng);
    }

    SatelliteEphemeris SatelliteEphemerisFetch::fetchNamed(const std::string &name) {
        std::ostringstream url;
        url << URL_FETCH_NAME << name;
        auto data = curl_process(url.str());

        if (data.size() != 1)
            throw curlpp::LogicError("Should be one, and only one, ephemeris");

        return std::move(data.begin()->second);
    }

    SatelliteEphemerisMap SatelliteEphemerisFetch::fetchAll() {
        return std::move(curl_process((std::string) URL_FETCH_ALL));
    }

    SatelliteEphemerisMap SatelliteEphemerisFetch::curl_process(const std::string &url) {
        SatelliteEphemerisMap ephemerisMap;

        try {
            // Set the URL.
            curlpp::options::Url myUrl(url);
            curlpp::Easy myRequest;

            std::stringstream response;

            myRequest.setOpt(new curlpp::options::Url(url));
            myRequest.setOpt(new curlpp::options::WriteStream(&response));

            // Send request and get a result.
            myRequest.perform();

            std::vector<std::string> input;
            std::string line;
            while (getline(response, line))
                input.push_back(line);

            if (input.size() % 3)
                throw curlpp::LogicError("Input lines must be a multiple of 3");

            SatelliteEphemeris ephemeris;
            for (auto i = input.begin(); i != input.end(); ++i) {
                ephemeris[0] = *i++;
                ephemeris[1] = *i++;
                ephemeris[2] = *i;
                ephemerisMap[ephemeris[0]] = ephemeris;
            }
        }

        catch (curlpp::RuntimeError &e) {
            std::cout << e.what() << std::endl;
        }

        catch (curlpp::LogicError &e) {
            std::cout << e.what() << std::endl;
        }

        return std::move(ephemerisMap);
    }

    void EphemerisModel::loadEphemerisLibrary() {
        mEphemerisLibaryLoad = std::thread([this]() {
            std::lock_guard<std::mutex> libraryLock(mEphmerisLibraryMutex);
            mEphmerisLibrary.loadEphemeris();
        });
    }

    std::optional<Satellite> EphemerisModel::getSatellite(const std::string &name) {
        if (mEphmerisLibrary.haveEphemeris(name))
            return mEphmerisLibrary.satellite(name);
        return std::nullopt;
    }

    int EphemerisModel::setSatellitesOfInterest(const std::string &satelliteNameList) {
        if (mEphemerisLibaryLoad.joinable())
            mEphemerisLibaryLoad.join();

        std::lock_guard<std::mutex> libraryLock(mEphmerisLibraryMutex);

        mSatellitesOfInterest.clear();
        std::stringstream strm;
        strm << satelliteNameList;
        std::string name;
        while (getline(strm, name, ',')) {
            auto sat = getSatellite(name);
            if (sat)
                mSatellitesOfInterest[name] = sat.value();
        }

        mDivider = 0;
        mInitialize = true;

        return mSatellitesOfInterest.size();
    }

    EphemerisModel::EphemerisModel() : mPredictionTimer(*this, &EphemerisModel::timerCallback, 5000),
                                       mObserver() {
        mDivider = 0;
        mInitialize = true;
    }

    Uint32 EphemerisModel::timerCallback(Uint32 interval) {
        if (mEphmerisLibraryMutex.try_lock()) {
            if (!mEphmerisLibrary.empty()) {

                if (mEphemerisLibaryLoad.joinable())
                    mEphemerisLibaryLoad.join();

                mDivider += interval;

                DateTime now{true};
                if (mInitialize || mDivider >= 60000) {
                    mSatellitePassData.clear();
                    for (auto &sat : mSatellitesOfInterest) {
                        sat.second.predict(now);
                        Earthsat earthsat{};
                        earthsat.FindNextPass(sat.second, mObserver);
                        mSatellitePassData.emplace_back(sat.first, earthsat.riseTime(), earthsat.setTime());
                    }

                    std::sort(mSatellitePassData.begin(), mSatellitePassData.end(), [](auto &p0, auto &p1) {
                        return std::get<1>(p0) < std::get<1>(p1);
                    });

                    if (mPassMonitorCallback) {
                        mPassMonitorCallback(PassMonitorData{mSatellitePassData});
                    }

                    if (mCelestialTrackingCallback) {
                        CelestialTrackingData celestialData;
                        auto[sLat, sLon] = subSolar();
                        celestialData.emplace_back(sLat, sLon, std::pair{1, 0});
                        auto moon = getSatellite("Moon");
                        if (moon) {
                            moon->predict(now);
                            auto[mLat, mLon] = moon->geo();
                            celestialData.emplace_back(mLat, mLon, std::pair{1, 1});
                        }
                        mCelestialTrackingCallback(celestialData);
                    }
                }

                if (mInitialize || mDivider >= 10000) {
                    mSatelliteOrbitData.clear();
                    for (auto &pass : mSatellitePassData) {
                        auto sat = mSatellitesOfInterest.at(std::get<0>(pass));
                        if (abs(now - sat.mPrediction) > 10. / 86400.)
                            sat.predict(now);
                        auto[lat, lon] = sat.geo();
                        mSatelliteOrbitData.emplace_back(std::get<0>(pass), lat, lon);
                    }

                    if (mOrbitTrackingCallback) {
                        mOrbitTrackingCallback(OrbitTrackingData{mSatelliteOrbitData});
                    }
                }

                if (mInitialize || mDivider >= 5000) {
                    mSatelliteTrackData.clear();
                    for (auto &track : mSatellitePassData) {
                        auto sat = mSatellitesOfInterest.at(std::get<0>(track));
                        if (abs(now - sat.mPrediction) > 5. / 86400.)
                            sat.predict(now);
                        if ((std::get<1>(track) - now) * 86400. < 60. && (std::get<2>(track) - now) * 86400. > -60. ) {
                            auto[el, az, range, rate] = sat.topo(mObserver);
                            mSatelliteTrackData.emplace_back(std::get<0>(track), el, az, range, rate);
                        }
                    }

                    if (mPassTrackingCallback)
                        mPassTrackingCallback(PassTrackingData{mSatelliteTrackData});
                }

                mInitialize = false;
            }
            mEphmerisLibraryMutex.unlock();
        }
        return interval;
    }

    void EphemerisModel::setObserver(const Observer &observer) {
        std::lock_guard<std::mutex> libraryLock(mEphmerisLibraryMutex);
        if (mEphemerisLibaryLoad.joinable())
            mEphemerisLibaryLoad.join();

        mObserver = observer;
    }
}