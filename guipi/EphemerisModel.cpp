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

    void Earthsat::FindNextPass(const Satellite &satellite, const Observer &observer) {
        DateTime t_now{};
        Satellite localSat{satellite};
        Observer localObs{observer};

        t_now.userNow();

        double prevElevation{0};
        auto dt = COARSE_DT;
        DateTime t_srch = t_now + -FINE_DT;    // search time, start beyond any previous solution

        // init pel and make first step
        localSat.predict(t_srch);
#if __cplusplus == 201703L
        auto[tel, taz, trange, trate] = localSat.topo(observer);
#else
        auto _t = localSat.topo(observer);
        auto tel = std::get<0>(_t);
        auto taz = std::get<1>(_t);
        auto trange = std::get<2>(_t);
        auto trate = std::get<3>(_t);
#endif
        t_srch += dt;

        // search up to a few days ahead for next rise and set times (for example for moon)
        while ((!set_ok || !rise_ok) && t_srch < t_now + 2.0F) {
            std::cerr << __PRETTY_FUNCTION__ << '\n'
            << localSat.period() << '\n'
            << t_srch << '\n'
            << t_now + 2.0 << '\n';
            // find circumstances at time t_srch
            localSat.predict(t_srch);
#if __cplusplus == 201703L
            auto[tel, taz, trange, trate] = localSat.topo(observer);
#else
            _t = localSat.topo(observer);
            tel = std::get<0>(_t);
            taz = std::get<1>(_t);
            trange = std::get<2>(_t);
            trate = std::get<3>(_t);
#endif

            // check for rising or setting events
            if (tel >= SAT_MIN_EL) {
                ever_up = true;
                if (prevElevation < SAT_MIN_EL) {
                    if (dt == FINE_DT) {
                        // found a refined set event (recall we are going backwards),
                        // record and resume forward time.
                        set_time = t_srch;
                        set_az = taz;
                        set_ok = true;
                        dt = COARSE_DT;
                        prevElevation = tel;
                    } else if (!rise_ok) {
                        // found a coarse rise event, go back slower looking for better set
                        dt = FINE_DT;
                        prevElevation = tel;
                    }
                }
            } else {
                ever_down = true;
                if (prevElevation > SAT_MIN_EL) {
                    if (dt == FINE_DT) {
                        // found a refined rise event (recall we are going backwards).
                        // record and resume forward time but skip if set is within COARSE_DT because we
                        // would jump over it and find the NEXT set.
                        DateTime check_set = t_srch + COARSE_DT;
                        localSat.predict(check_set);
#if __cplusplus == 201703L
                        auto[check_tel, check_taz, check_trange, check_trate] = localSat.topo(observer);
#else
                        _t = localSat.topo(observer);
                        auto check_tel = std::get<0>(_t);
                        auto check_taz = std::get<1>(_t);
                        auto check_trange = std::get<2>(_t);
                        auto check_trate = std::get<3>(_t);
#endif
                        if (check_tel >= SAT_MIN_EL) {
                            rise_time = t_srch;
                            rise_az = taz;
                            rise_ok = true;
                        }
                        // regardless, resume forward search
                        dt = COARSE_DT;
                        prevElevation = tel;
                    } else if (!set_ok) {
                        // found a coarse set event, go back slower looking for better rise
                        dt = FINE_DT;
                        prevElevation = tel;
                    }
                }
            }
            t_srch += dt;
            prevElevation = tel;
        }
    }

    void Earthsat::roundPassTimes() {
        rise_time.TN = round(rise_time.TN * 86400.) / 86400.;
        set_time.TN = round(set_time.TN * 86400.) / 86400.;
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
        std::string url_fetch_moon = std::string{URL_FETCH_NAME} + "Moon";
        auto rest = std::move(curl_process((std::string) URL_FETCH_ALL));
        auto moon = std::move(curl_process(url_fetch_moon));
        rest["Moon"] = moon.at("Moon");
        return std::move(rest);
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

                // Normalize the satellite names
                auto l = ephemeris[0].find_first_of('(') + 1;
                auto r = ephemeris[0].find_last_of(')');
                if (l != std::string::npos && r != std::string::npos && l < r) {
                    ephemeris[0] = ephemeris[0].substr(l,r);
                }
                r = ephemeris[0].find_last_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") + 1;
                if (r != std::string::npos)
                    ephemeris[0] = ephemeris[0].substr(0, r);

                if (ephemeris[0] == "ZARYA")
                    ephemeris[0] = "ISS";

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

        if (satelliteNameList.empty()) {
            for (auto & sat : mEphmerisLibrary)
                if (sat.first != "Moon")
                    mSatellitesOfInterest[sat.first] = Satellite{sat.second};
        } else{
                std::stringstream strm;
                strm << satelliteNameList;
                std::string name;
                while (getline(strm, name, ',')) {
                    auto sat = getSatellite(name);
                    if (sat)
                        mSatellitesOfInterest[name] = sat.value();
                }
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
                        if (fmod(sat.second.period(), 1.0) < 0.9) {
                            Earthsat earthsat{};
                            earthsat.FindNextPass(sat.second, mObserver);
                            earthsat.roundPassTimes();
                            if (earthsat.isEverUp())
                                mSatellitePassData.emplace_back(sat.first, earthsat.riseTime(), earthsat.setTime());
                        }
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