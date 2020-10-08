//
// Created by richard on 2020-08-27.
//

#include "Earthsat.h"

namespace guipi {

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
}
