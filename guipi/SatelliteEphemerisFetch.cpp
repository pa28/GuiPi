//
// Created by richard on 2020-08-29.
//

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <vector>
#include "SatelliteEphemerisFetch.h"


SatelliteEphemeris SatelliteEphemerisFetch::fetchNamed(const std::string &name) {
    std::ostringstream url;
    url << URL_FETCH_NAME << name;
    auto data = curl_process(url.str());

    if (data.size() != 1)
        throw curlpp::LogicError("Should be one, and only one, ephemeris");

    return std::move(data.begin()->second);
}

SatelliteEphemerisMap SatelliteEphemerisFetch::fetchAll() {
    return std::move(curl_process((std::string)URL_FETCH_ALL));
}

SatelliteEphemerisMap SatelliteEphemerisFetch::curl_process(const std::string &url) {
    SatelliteEphemerisMap ephemerisMap;

    try {
        // Set the URL.
        curlpp::options::Url myUrl(url);
        curlpp::Easy myRequest;

        std::stringstream response;

        myRequest.setOpt( new curlpp::options::Url( url ) );
        myRequest.setOpt( new curlpp::options::WriteStream( &response ) );

        // Send request and get a result.
        myRequest.perform();

        std::vector<std::string> input;
        std::string line;
        while (getline(response, line))
            input.push_back(line);

        if (input.size() % 3)
            throw curlpp::LogicError("Input lines must be a multiple of 3");

        SatelliteEphemeris ephemeris;
        for( auto i = input.begin(); i != input.end(); ++i ) {
            ephemeris[0] = *i++;
            ephemeris[1] = *i++;
            ephemeris[2] = *i;
            ephemerisMap[ephemeris[0]] = ephemeris;
        }
    }

    catch(curlpp::RuntimeError & e)
    {
        std::cout << e.what() << std::endl;
    }

    catch(curlpp::LogicError & e)
    {
        std::cout << e.what() << std::endl;
    }

    return std::move(ephemerisMap);
}
