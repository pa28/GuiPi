//
// Created by richard on 2020-10-15.
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
#include <string_view>
#include <soci/soci.h>
#include <sqlite3/soci-sqlite3.h>
#include <exception>
#include <iostream>

#define XSTR(arg) STR(arg)
#define STR(arg) #arg

#define SETTING_VALUES   \
    X(CallSign, std::string, "CALLSIGN") \
    X(Latitude, float, 0.f)      \
    X(Longitude, float, 0.f)     \
    X(Elevation, float, 0.f)     \
    X(SideBarActiveTab, int, 0)  \
    X(SatelliteTracking, int, 0) \
    X(CelestialTracking, int, 0) \
    X(AzimuthalDisplay, int, 0)  \
    X(GeoPositions, int, 0)

namespace guipi {
    class Settings {
    protected:
        std::string mDbFileName;
    public:
        std::string mHomeDir;
#define X(name,type,default) type m ## name;
        SETTING_VALUES
#undef X

        Settings() = delete;
        explicit Settings(const std::string &db_file_name);

        void initializeSettingsDatabase();

        template<typename T>
        std::optional<T> getDatabaseValue(soci::session &sql, const std::string_view &name);

        template<typename T>
        void setDatabaseValue(soci::session &sql, const std::string_view &name, T value);

        template<typename T>
        void writeValue(const std::string &name, T value) {
            try {
                soci::session sql(soci::sqlite3, mDbFileName);
                setDatabaseValue(sql, name, value);
            }
            catch (std::exception const &e) {
                std::cerr << e.what() << '\n';
            }
        }

        void readAllValues(soci::session &sql);

        void writeAllValues(soci::session &sql);

#define X(name,type,default) void set ## name(type value) { m ## name = value; writeValue(# name, m ## name); }
        SETTING_VALUES
#undef X
    };
}



