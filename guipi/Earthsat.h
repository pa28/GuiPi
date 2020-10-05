//
// Created by richard on 2020-08-27.
//

#pragma once

#include "p13.h"

namespace guipi {

    class Earthsat {
    private:
        bool set_ok = false, rise_ok = false, ever_up = false, ever_down = false;
        DateTime set_time, rise_time;
        double set_az, rise_az;

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
    };

}
