//
// Created by richard on 2020-08-26.
//

#ifndef HAMUTILITIES_P13_H
#define HAMUTILITIES_P13_H


//
// Plan13.cpp
//
// An implementation of Plan13 in C++ by Mark VandeWettering
//
// Plan13 is an algorithm for satellite orbit prediction first formulated
// by James Miller G3RUH.  I learned about it when I saw it was the basis
// of the PIC based antenna rotator project designed by G6LVB.
//
// http://www.g6lvb.com/Articles/LVBTracker2/index.htm
//
// I ported the algorithm to Python, and it was my primary means of orbit
// prediction for a couple of years while I operated the "Easy Sats" with
// a dual band hand held and an Arrow antenna.
//
// I've long wanted to redo the work in C++ so that I could port the code
// to smaller processors including the Atmel AVR chips.  Bruce Robertson,
// VE9QRP started the qrpTracker project to fulfill many of the same goals,
// but I thought that the code could be made more compact and more modular,
// and could serve not just the embedded targets but could be of more
// use for more general applications.  And, I like the BSD License a bit
// better too.
//
// So, here it is!
//


//----------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <tuple>
#include <array>
#include <iostream>
#include <iomanip>
#include <map>
#include "constexpertrig.h"

//----------------------------------------------------------------------

constexpr double RADIANS(double deg) {
    return deg * M_PI / 180.;
}

constexpr double DEGREES(double rad) {
    return rad * 180. / M_PI;
}

typedef std::array<std::string, 3> SatelliteEphemeris;

inline std::ostream& operator<<(std::ostream& os, const SatelliteEphemeris &satelliteEphemeris) {
    return os << satelliteEphemeris[0] << '\n'
        << satelliteEphemeris[1] << '\n'
        << satelliteEphemeris[2] << '\n';
}

typedef std::map<std::string, SatelliteEphemeris> SatelliteEphemerisMap;

struct P13 {
    constexpr static double RE = 6378.137;
    constexpr static double FL = 1. / 298.257224;
    constexpr static double GM = 3.986E5;
    constexpr static double J2 = 1.08263E-3;
    constexpr static double YM = 365.25;
    constexpr static double YT = 365.2421874;
    constexpr static double WW = 2. * M_PI / YT;
    constexpr static double WE = 2. * M_PI + WW;
    constexpr static double W0 = WE / 86400.;
    constexpr static double YG = 2014.;
    constexpr static double G0 = 99.5828;
    constexpr static double MAS0 = 356.4105;
    constexpr static double MASD = 0.98560028;
    constexpr static double EQC1 = 0.03340;
    constexpr static double EQC2 = 0.00035;
    constexpr static double INS = RADIANS(23.4375);
    constexpr static double CNS = math::cos(INS);
    constexpr static double SNS = math::sin(INS);

    /*
    constexpr static SatelliteEphemeris ISS =
            {"ISS",
             "1 25544U 98067A   20239.80208397  .00000993  00000-0  26004-4 0  9990",
             "2 25544  51.6471 359.0049 0001779  59.1240  72.0472 15.49189559242962"};
    constexpr static SatelliteEphemeris Moon =
            { "Moon",
              "1     1U     1A   20241.93195602  .00000000  00000-0  0000000 0  0019",
              "2     1 335.6972 191.4324 0362000   0.1506  91.1318  0.03660000    14"};
    */

    constexpr static double MAX_TLE_AGE = 7.0;        // max age to use a TLE, days (except moon)
    constexpr static double MAX_TLE_AGE_MOON = 1.5; // max age to use a TLE, days for the moon

    template<typename T>
    [[nodiscard]] static constexpr std::tuple<T, T> mod(const T x) {
        T i, f;
        f = modf(x, &i);
        return std::make_tuple(f, i);
    }

    template<typename T>
    [[maybe_unused]] [[nodiscard]] static constexpr T to_integer(const T x) {
        auto[f, i] = mod(x);
        return i;
    }

    template<typename T>
    [[maybe_unused]] [[nodiscard]] static constexpr T to_fraction(const T x) {
        auto[f, i] = mod(x);
        return f;
    }
};

//------------------------------------------------------------------------

// the original BASIC code used three variables (e.g. Ox, Oy, Oz) to
// represent a vector quantity.  I think that makes for slightly more
// obtuse code, so I going to collapse them into a single variable
// which is an array of three elements

//typedef double Vec3[3];

template<typename T>
class Vec3Base : public std::array<T, 3> {
};

typedef Vec3Base<double> Vec3;


//----------------------------------------------------------------------

class DateTime {
public:
    long DN;
    double TN;

    /**
     * Compute the day-in-space give the calendar year, month and day.
     * @param year
     * @param month
     * @param day
     * @return the day-in-space
     */
    [[nodiscard]] static long fnday(long year, uint8_t month, uint8_t day);

    /**
     * Compute the calendar date from the day-in-space
     * @param dt the day-in space
     * @return a tuple with the calendar year, month, day
     */
    [[nodiscard]] static std::tuple<int, uint8_t, uint8_t> fndate(long dt);

    /**
     * Constructor to initialize to a calendar date and clock time
     * @param year
     * @param month
     * @param day
     * @param hour
     * @param minute
     * @param seconds
     */
    DateTime(int year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t seconds)
            : DateTime() { settime(year, month, day, hour, minute, seconds); }

    /**
     * Default constructor with the option to initialzie to current time.
     * @param setToNow if true, initialize to the current time.
     */
    explicit DateTime(bool setToNow = false) : DN{0}, TN{0} {
        if (setToNow)
            userNow();
    }

    DateTime(const DateTime &) = default;

    ~DateTime() = default;

    DateTime &operator=(const DateTime &source) = default;

    /**
     * Set the DateTime from calendar date and clock time.
     * @param year
     * @param month
     * @param day
     * @param hour
     * @param minute
     * @param seconds
     */
    void settime(int year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t seconds);

    /**
     * Compute the calendar date and clock time
     * @return a tuple with year, month, day, hour, minute, seconds
     */
    [[nodiscard]] std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>
    gettime() const;

    bool operator<(const DateTime &rhs) const;

    bool operator>(const DateTime &rhs) const { return (rhs < *this); }

    DateTime operator+(long seconds);

    DateTime &operator+=(long seconds);

    DateTime operator+(double days);

    DateTime &operator+=(double days);

    double operator-(const DateTime &rhs) const;

    /* return a DateTime for the current time
    */
    void userNow() {
        struct tm *tm;
        auto t = time(nullptr);
        tm = gmtime(&t);
        int yr = tm->tm_year + 1900;
        uint8_t mo = tm->tm_mon + 1;
        uint8_t dy = tm->tm_mday;
        uint8_t hr = tm->tm_hour;
        uint8_t mn = tm->tm_min;
        uint8_t sc = tm->tm_sec;

        settime(yr, mo, dy, hr, mn, sc);
    }

    [[maybe_unused]] std::ostream &print_on(std::ostream &os) const {
        auto[yr, mo, da, h, m, s] = gettime();
        os << yr << '-' << (long) mo << '-' << (long) da << ' ' << (long) h << ':' << (long) m << ':' << (long) s;
        return os;
    }
};

inline std::ostream &operator<<(std::ostream &os, const DateTime &dateTime) { return dateTime.print_on(os); }

//----------------------------------------------------------------------

class Observer {
public:
    double LA;
    double LO;
    double HT;
    Vec3 U, E, N, O, V;

    Observer() = default;

    /**
     * Constructor
     * @param latitude in degrees, North positive, South negative.
     * @param longitude in degrees, East positive, West negative.
     * @param elevation above sea level in meters?
     */
    Observer(double const &latitude, double const &longitude, double const &elevation);

    ~Observer() = default;
};

//----------------------------------------------------------------------


class Sun {
public:
    Vec3 SUN, H;

    Sun() = default;

    ~Sun() = default;

    void predict(const DateTime &dt);
};

//----------------------------------------------------------------------

class Satellite {
    bool isMoon{};
    std::string_view name;
    long N{};
    long YE{};
    double IN{};
    double RA{};
    double EC{};
    double WP{};
    double MA{};
    double MM{};
    double M2{};
    double RV{};


    // these values are stored, but could be calculated on the fly
    // during calls to predict()
    // classic space/time tradeoff

    double N0{}, A_0{}, B_0{};
    double PC{};
    double QD{}, WD{}, DC{};
    double RS{};

    /**
     * Initialize satellite data from two line ephemeris.
     * @param l1 line 1
     * @param l2 line 2
     */
    void tle(const std::string_view &l1, const std::string_view &l2);

public:
    long DE{};
    double TE{};

    Vec3 SAT{}, VEL{};        // celestial coordinates
    Vec3 S{}, V{};        // geocentric coordinates

    Satellite() = default;

    /**
     * Initialize satellite from two line ephemeris data
     * @param ephemeris An array containing the name, line 1, and line 2
     */
    explicit Satellite(const SatelliteEphemeris &ephemeris);

    ~Satellite() = default;

    /**
     * Predict the satelite position at given DateTime.
     * @param dateTime
     */
    void predict(const DateTime &dateTime);

    /**
     * Access the satellite name.
     * @return
     */
    [[nodiscard]] std::string_view getName() const {
        return name;
    }

    /**
     * Determine if the sat epoch is known to be good.
     * @return true if good.
     */
    [[nodiscard]] bool checkSatEpoch() {
        DateTime t_now(true);
        DateTime t_epo = epoch();
        if (isMoon)
            return (t_epo + P13::MAX_TLE_AGE_MOON > t_now && t_now + P13::MAX_TLE_AGE_MOON > t_epo);
        else
            return (t_epo + P13::MAX_TLE_AGE > t_now && t_now + P13::MAX_TLE_AGE > t_epo);
    }

    /**
     * Determine if the sun is eclipsed by the current predicted position.
     * @param sp The Sun object.
     * @return true if eclipsed.
     */
    bool eclipsed(const Sun &sp);

    /**
     * Convert satellite co-ordinates to an observer frame.
     * @param obs the Observer object
     * @return a tuple containing: altitude (the angle of elevation),
     *  azimuth, range, range_rate
     */
    [[nodiscard]] std::tuple<double, double, double, double> topo(const Observer &obs);

    /**
     * Access the sub-satellite geographical co-ordinates.
     * @return a tuple containing latitude and longitude in Radians. North and East hemispheres have
     * positive values.
     */
    [[nodiscard]] std::tuple<double, double> geo();

    /**
     * Access the satellite celestial co-ordinates.
     * @return a tuple containing latitude and longitude in Radians. North and East hemispheres have
     * positive values.
     */
    [[nodiscard]] std::tuple<double, double> celest();

    /**
     * Access the satellite orbital period.
     * @return the period in days per revolution.
     */
    [[nodiscard]] double period() const;

    /** Compute the viewing radius from the sub-satellite geographical location for a given
     * altitude (angle of elevation).
     * @param alt - angle of elevation in Radians
     * @return the great-circle viewing radius in Radians.
     */
    double viewingRadius(double alt);

    /**
     * Access the epoch of the satellite.
     * @return the DateTime of the epoch.
     */
    DateTime epoch() const;
};

#endif //HAMUTILITIES_P13_H
