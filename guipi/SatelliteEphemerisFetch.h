//
// Created by richard on 2020-08-29.
//

#pragma once

#include "guipi/p13.h"

class SatelliteEphemerisFetch {

#if __cplusplus == 201703L
    constexpr static std::string_view URL_FETCH_NAME = "http://clearskyinstitute.com/ham/HamClock/esats.pl?tlename=";
    constexpr static std::string_view URL_FETCH_ALL = "http://clearskyinstitute.com/ham/HamClock/esats.pl?getall=";
#else
#define URL_FETCH_NAME "http://clearskyinstitute.com/ham/HamClock/esats.pl?tlename="
#define URL_FETCH_ALL "http://clearskyinstitute.com/ham/HamClock/esats.pl?getall="
#endif

public:

    static SatelliteEphemerisMap fetchAll();

    static SatelliteEphemeris fetchNamed(const std::string &name);

    static SatelliteEphemerisMap curl_process(const std::string &url);
};

