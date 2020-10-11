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
        std::lock_guard<std::mutex> libraryLock(mEphmerisLibraryMutex);
        if (mEphemerisLibaryLoad.joinable())
            mEphemerisLibaryLoad.join();

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
                        Earthsat earthsat;
                        earthsat.FindNextPass(sat.second, mObserver);
                        mSatellitePassData.emplace_back(sat.first, earthsat);
                    }

                    std::sort(mSatellitePassData.begin(), mSatellitePassData.end(), [](auto &p0, auto &p1) {
                        return p0.second.riseTime() < p1.second.riseTime();
                    });

                    if (mPassMonitorCallback) {
                        mPassMonitorCallback(PassMonitorData{mSatellitePassData});
                    }
                }

                if (mInitialize || mDivider >= 10000) {
                    mSatelliteOrbitData.clear();
                    for (auto &pass : mSatellitePassData) {
                        auto sat = mSatellitesOfInterest.at(pass.first);
                        if (abs(now - sat.mPrediction) > 10. / 86400.)
                            sat.predict(now);
                        auto[lat, lon] = sat.geo();
                        mSatelliteOrbitData.emplace_back(pass.first, lat, lon);
                    }

                    if (mOrbitTrackingCallback) {
                        mOrbitTrackingCallback(OrbitTrackingData{mSatelliteOrbitData});
                    }
                }

                if (mInitialize || mDivider >= 5000) {
                    mSatelliteTrackData.clear();
                    for (auto &track : mSatelliteOrbitData) {
                        auto sat = mSatellitesOfInterest.at(std::get<0>(track));
                        if (abs(now - sat.mPrediction) > 5. / 86400.)
                            sat.predict(now);
                        auto[el, az, range, rate] = sat.topo(mObserver);
                        mSatelliteTrackData.emplace_back(std::get<0>(track), el, az, range, rate);
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