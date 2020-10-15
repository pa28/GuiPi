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

#define XSTR(arg) STR(arg)
#define STR(arg) #arg

#define SETTING_VALUES   \
    X(mCallSign, std::string, "CALLSIGN") \
    X(mLatitude, float, 0.f)    \
    X(mLongitude, float, 0.f)   \
    X(mElevation, float, 0.f)   \

namespace guipi {
    class Settings {
    protected:
        std::string mDbFileName;
    public:
        std::string mHomeDir;
#define X(name,type,default) type name;
        SETTING_VALUES
#undef X

        Settings() = delete;
        explicit Settings(const std::string &db_file_name);

        void initializeSettingsDatabase();

        template<typename T>
        std::optional<T> getDatabaseValue(soci::session &sql, const std::string_view &name);

        template<typename T>
        void setDatabaseValue(soci::session &sql, const std::string_view &name, T value);

        void readAllValues(soci::session &sql);

        void writeAllValues(soci::session &sql);
    };
}



