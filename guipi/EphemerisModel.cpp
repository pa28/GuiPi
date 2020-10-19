//
// Created baggressive on 2020-10-11.
//

#include <vector>
#include <iostream>
#include <sstream>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <algorithm>
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
        max_elevation = 0.;

        t_now.userNow();

        double prevElevation{0};
        auto dt = COARSE_DT;
        DateTime t_srch = t_now + -FINE_DT;    // search time, start beyond any previous solution

        // init pel and make first step
        localSat.predict(t_srch);
        auto[tel, taz, trange, trate] = localSat.topo(observer);
        t_srch += dt;

        // search up to a few days ahead for next rise and set times (for example for moon)
        while ((!set_ok || !rise_ok) && t_srch < t_now + 2.0F) {
            // find circumstances at time t_srch
            localSat.predict(t_srch);
            auto[tel, taz, trange, trate] = localSat.topo(observer);
            max_elevation = std::max(max_elevation, tel);

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
                        auto[check_tel, check_taz, check_trange, check_trate] = localSat.topo(observer);
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

    SatelliteEphemerisMap curl_process(const std::string &url) {
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
                    ephemeris[0] = ephemeris[0].substr(l, r);
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

    SatelliteEphemerisMap fetchAll(int source) {
        SatelliteEphemerisMap result;
        SatelliteEphemerisMap moon;
        std::string url_fetch_moon = std::string{URL_FETCH_NAME} + "Moon";
        switch (source) {
            case 0:
                result = std::move(curl_process((std::string) URL_FETCH_ALL));
                break;
            case 1:
                result = std::move(curl_process((std::string) CT_AMATEUR));
                moon = std::move(curl_process(url_fetch_moon));
                result["Moon"] = moon.at("Moon");
                break;
            case 2:
                result = std::move(curl_process((std::string) CT_BRIGHT));
                moon = std::move(curl_process(url_fetch_moon));
                result["Moon"] = moon.at("Moon");
                break;
            case 3:
                result = std::move(curl_process((std::string) CT_CUBESAT));
                moon = std::move(curl_process(url_fetch_moon));
                result["Moon"] = moon.at("Moon");
                break;
            default:
                throw std::logic_error("SatelliteEphemerisFetch, source not handled.");
        }
        return std::move(result);
    }

    bool EphemerisModel::asyncEphemerisFetch(EphemerisModel *self, int source) {
        self->mNewSatelliteEphemerisMap = std::move(fetchAll(source));
        self->mDivider = 0;
        self->mInitialize = true;
        return true;
    }

    void EphemerisModel::loadEphemerisLibrary(int source) {
        mEphemerisLibaryLoad = std::async(asyncEphemerisFetch, this, source);
    }

    void EphemerisModel::loadEphemerisLibraryWait(int source) {
        std::lock_guard<std::mutex> lockGuard(mEphemerisLibraryMutex);
        mSatelliteEphemerisMap = std::move(fetchAll(source));
        mDivider = 0;
        mInitialize = true;
    }

    std::optional<Satellite> EphemerisModel::getSatellite(const std::string &name) {
        if (auto sat = mSatelliteEphemerisMap.find(name); sat != mSatelliteEphemerisMap.end())
            return Satellite(sat->second);
        return std::nullopt;
    }

    int EphemerisModel::setSatellitesOfInterestImpl(const std::string &satelliteNameList) {
        mSatelliteNameList = satelliteNameList;
        mSatellitesOfInterest.clear();

        if (satelliteNameList.empty()) {
            for (auto &sat : mSatelliteEphemerisMap)
                if (sat.first != "Moon")
                    mSatellitesOfInterest[sat.first] = Satellite{sat.second};
        } else {
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

    int EphemerisModel::setSatellitesOfInterest(const std::string &satelliteNameList) {
        std::lock_guard<std::mutex> lockGuard(mEphemerisLibraryMutex);
        return setSatellitesOfInterestImpl(satelliteNameList);
    }

    EphemerisModel::EphemerisModel()
        : mPredictionTimer(*this, &EphemerisModel::timerCallback, 5000) {
        mDivider = 0;
        mInitialize = true;
    }

    void EphemerisModel::setSettings(sdlgui::ref<Settings> settings) {
        mSettings = std::move(settings);
        mSettings->addCallback([this](guipi::Settings::Parameter parameter) {
            switch (parameter) {
                case Settings::Parameter::Latitude:
                case Settings::Parameter::Longitude:
                case Settings::Parameter::Elevation:
                    mDivider = 0;
                    mInitialize = true;
                    timerCallback(0);
                default:
                    break;
            }
        });
    }

    Uint32 EphemerisModel::timerCallback(Uint32 interval) {
        // If model is locked return.
        if (!mEphemerisLibraryMutex.try_lock()) {
            return interval;
        }

        // If a new library is waiting load it and reset calculations.
        if (mEphemerisLibaryLoad.valid()) {
            if (mEphemerisLibaryLoad.get()) {
                mSatelliteEphemerisMap = std::move(mNewSatelliteEphemerisMap);
                setSatellitesOfInterestImpl(mSatelliteNameList);
            }
        }

        // If the library is empty return
        if (mSatelliteEphemerisMap.empty()) {
            mEphemerisLibraryMutex.unlock();
            return interval;
        }

        mDivider += interval;

        Observer observer{mSettings->mLatitude, mSettings->mLongitude, mSettings->mElevation};
        DateTime now{true};
        if (mInitialize || mDivider >= 60000) {
            mSatellitePassData.clear();
            for (auto &sat : mSatellitesOfInterest) {
                sat.second.predict(now);
                if (fmod(sat.second.period(), 1.0) < 0.9) {
                    Earthsat earthsat{};
                    earthsat.FindNextPass(sat.second, observer);
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
                if ((std::get<1>(track) - now) * 86400. < 60. && (std::get<2>(track) - now) * 86400. > -60.) {
                    auto[el, az, range, rate] = sat.topo(observer);
                    mSatelliteTrackData.emplace_back(std::get<0>(track), el, az, range, rate);
                }
            }

            if (mPassTrackingCallback)
                mPassTrackingCallback(PassTrackingData{mSatelliteTrackData});
        }

        mInitialize = false;
        mEphemerisLibraryMutex.unlock();
        return interval;
    }

}
