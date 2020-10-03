//
// Created by richard on 2020-08-26.
//

#include <tuple>
#include "p13.h"

//
// P13.cpp
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
// VE9QRP started the qrpTracker project to fufill many of the same goals,
// but I thought that the code could be made more compact and more modular,
// and could serve not just the embedded targets but could be of more
// use for more general applications.  And, I like the BSD License a bit
// better too.
//
// So, here it is!
//

// Added a few select DateTime overloaded operators and _DATETIME_UNITTEST	-- ECD

// here are a bunch of constants that will be used throughout the
// code, but which will probably not be helpful outside.

// Updated with 2014 values from
// http://www.amsat.org/amsat/articles/g3ruh/111.html

long DateTime::fnday(long y, uint8_t m, uint8_t d) {
    if (m < 3) {
        m += 12;
        y--;
    }
    return (long) ((double) y * P13::YM) + (long) (((double) m + 1) * 30.6f) + (long) d - 428L;
}

std::tuple<int, uint8_t, uint8_t> DateTime::fndate(long dt) {
    dt += 428L;
    auto y = (int) (((double) dt - 122.1) / 365.25);
    dt -= (long) ((double) y * 365.25);
    auto m = (uint8_t) ((double) dt / 30.61);
    dt -= (long) ((double) m * 30.6);
    m--;
    if (m > 12) {
        m -= 12;
        y++;
    }
    auto d = (uint8_t) dt;
    return std::make_tuple(y, m, d);
}

// overload <
bool DateTime::operator<(const DateTime &rhs) const {
    return (rhs - *this > 0);
}

// overload + seconds
DateTime DateTime::operator+(long seconds) {
    DateTime t(*this);
    t += seconds;
    return (t);
}

// overload += seconds
DateTime &DateTime::operator+=(long seconds) {
    TN += (double) seconds / (24.0 * 3600.0);
    DN += (long) P13::to_integer(TN);
    TN -= P13::to_integer(TN);
    return (*this);
}

// overload + days
DateTime DateTime::operator+(double days) {
    DateTime t(*this);
    t += days;
    return (t);
}

// overload += days
DateTime &DateTime::operator+=(double days) {
    TN += days;
    DN += (long) P13::to_integer(TN);
    TN -= P13::to_integer(TN);
    return (*this);
}

// overload - to yield difference in days
double DateTime::operator-(const DateTime &rhs) const {
    long ddn = DN - rhs.DN;
    double dtn = TN - rhs.TN;
    return ((double) ddn + dtn);
}


[[maybe_unused]] std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>
DateTime::gettime() const {
    uint8_t h, m, s;
    auto[year, month, day] = fndate(DN);
    double t = TN;
    t *= 24;
    h = (uint8_t) t;
    t -= h;
    t *= 60;
    m = (uint8_t) t;
    t -= m;
    t *= 60;
    s = (uint8_t) lround(t + 0.5);
    if (s == 60)
        s = 59;

    return std::make_tuple(year, month, day, h, m, s);
}

void
DateTime::settime(int year, uint8_t month, uint8_t day, uint8_t h, uint8_t m, uint8_t s) {
    DN = fnday(year, month, day);
    TN = ((double) h + (double) m / 60. + (double) s / 3600.) / 24.;
}

Observer::Observer(double const &latitude, double const &longitude, double const &elevation) : Observer() {
    LA = RADIANS(latitude);
    LO = RADIANS(longitude);
    HT = elevation / 1000;

    U[0] = cos(LA) * cos(LO);
    U[1] = cos(LA) * sin(LO);
    U[2] = sin(LA);

    E[0] = -sin(LO);
    E[1] = cos(LO);
    E[2] = 0.F;

    N[0] = -sin(LA) * cos(LO);
    N[1] = -sin(LA) * sin(LO);
    N[2] = cos(LA);

    auto RP = P13::RE * (1 - P13::FL);
    auto XX = P13::RE * P13::RE;
    auto ZZ = RP * RP;
    auto D = sqrt(XX * cos(LA) * cos(LA) +
                  ZZ * sin(LA) * sin(LA));
    auto Rx = XX / D + HT;
    auto Rz = ZZ / D + HT;

    O[0] = Rx * U[0];
    O[1] = Rx * U[1];
    O[2] = Rz * U[2];

    V[0] = -O[1] * P13::W0;
    V[1] = O[0] * P13::W0;
    V[2] = 0;
}

static double
getfloat(const std::string_view &c, int i0, int i1) {
    char buf[20];
    int i;
    for (i = 0; i0 + i < i1; i++)
        buf[i] = c[i0 + i];
    buf[i] = '\0';
    return strtod(buf, nullptr);
}

static long
getlong(const std::string_view &c, int i0, int i1) {
    char buf[20];
    int i;
    for (i = 0; i0 + i < i1; i++)
        buf[i] = c[i0 + i];
    buf[i] = '\0';
    return strtol(buf, nullptr, 10);
}

Satellite::Satellite(const SatelliteEphemeris &ephemeris)
        : Satellite() {
    name = ephemeris[0];
    isMoon = name == "Moon";
    tle(ephemeris[1], ephemeris[2]);
}

void
Satellite::tle(const std::string_view &l1, const std::string_view &l2) {
    // direct quantities from the orbital elements

    N = getlong(l2, 2, 7);
    YE = getlong(l1, 18, 20);
    if (YE < 58)
        YE += 2000;
    else
        YE += 1900;

    TE = getfloat(l1, 20, 32);
    M2 = RADIANS(getfloat(l1, 33, 43));

    IN = RADIANS(getfloat(l2, 8, 16));
    RA = RADIANS(getfloat(l2, 17, 25));
    EC = getfloat(l2, 26, 33) / 1e7f;
    WP = RADIANS(getfloat(l2, 34, 42));
    MA = RADIANS(getfloat(l2, 43, 51));
    MM = 2.0 * M_PI * getfloat(l2, 52, 63);
    RV = getfloat(l2, 63, 68);

    // derived quantities from the orbital elements

    // convert TE to DE and TE
    DE = DateTime::fnday(YE, 1, 0) + (long) TE;
    TE -= P13::to_integer(TE);
    N0 = MM / 86400;
    A_0 = pow(P13::GM / (N0 * N0), 1. / 3.);
    B_0 = A_0 * sqrt(1.F - EC * EC);
    PC = P13::RE * A_0 / (B_0 * B_0);
    PC = 1.5 * P13::J2 * PC * PC * MM;
    double CI = cos(IN);
    QD = -PC * CI;
    WD = PC * (5 * CI * CI - 1) / 2;
    DC = -2 * M2 / (3 * MM);
}

void
Satellite::predict(const DateTime &dt) {
    long DN = dt.DN;
    double TN = dt.TN;

    double TEG = (double) DE - (double) DateTime::fnday((long) P13::YG, 1, 0) + TE;

    double GHAE = RADIANS(P13::G0) + TEG * P13::WE;

    double T = (double) (DN - DE) + (TN - TE);
    double DT = DC * T / 2.F;
    double KD = 1.F + 4.F * DT;
    double KDP = 1.F - 7.F * DT;

    double M = MA + MM * T * (1.F - 3.F * DT);
    double DR = P13::to_integer(M / (2. * M_PI));
    M -= DR * 2. * M_PI;
    double EA = M;

    double DNOM, C_EA, S_EA;

    for (;;) {
        C_EA = cos(EA);
        S_EA = sin(EA);
        DNOM = 1.F - EC * C_EA;
        double D = (EA - EC * S_EA - M) / DNOM;
        EA -= D;
        if (fabs(D) < 1e-5)
            break;
    }

    double A = A_0 * KD;
    double B = B_0 * KD;
    RS = A * DNOM;

    double Vx, Vy;
    double Sx, Sy;
    Sx = A * (C_EA - EC);
    Sy = B * S_EA;

    Vx = -A * S_EA / DNOM * N0;
    Vy = B * C_EA / DNOM * N0;

    double AP = WP + WD * T * KDP;
    double CW = cos(AP);
    double SW = sin(AP);

    double RAAN = RA + QD * T * KDP;

    double CQ = cos(RAAN);
    double SQ = sin(RAAN);

    double CI = cos(IN);
    double SI = sin(IN);

    // CX, CY, and CZ form a 3x3 matrix
    // that converts between orbit coordinates,
    // and celestial coordinates.

    Vec3 CX, CY, CZ;

    CX[0] = CW * CQ - SW * CI * SQ;
    CX[1] = -SW * CQ - CW * CI * SQ;
    CX[2] = SI * SQ;

    CY[0] = CW * SQ + SW * CI * CQ;
    CY[1] = -SW * SQ + CW * CI * CQ;
    CY[2] = -SI * CQ;

    CZ[0] = SW * SI;
    CZ[1] = CW * SI;
    CZ[2] = CI;

    // satellite in celestial coords

    SAT[0] = Sx * CX[0] + Sy * CX[1];
    SAT[1] = Sx * CY[0] + Sy * CY[1];
    SAT[2] = Sx * CZ[0] + Sy * CZ[1];

    VEL[0] = Vx * CX[0] + Vy * CX[1];
    VEL[1] = Vx * CY[0] + Vy * CY[1];
    VEL[2] = Vx * CZ[0] + Vy * CZ[1];

    // and in geocentric coordinates

    double GHAA = (GHAE + P13::WE * T);
    double CG = cos(-GHAA);
    double SG = sin(-GHAA);

    S[0] = SAT[0] * CG - SAT[1] * SG;
    S[1] = SAT[0] * SG + SAT[1] * CG;
    S[2] = SAT[2];

    V[0] = VEL[0] * CG - VEL[1] * SG;
    V[1] = VEL[0] * SG + VEL[1] * CG;
    V[2] = VEL[2];
}

/* find local apparent circumstances
 * alt, azimuth, range, range_rate
 */
std::tuple<double, double, double, double>
Satellite::topo(const Observer &obs) {
    Vec3 R;
    R[0] = S[0] - obs.O[0];
    R[1] = S[1] - obs.O[1];
    R[2] = S[2] - obs.O[2];
    auto range = sqrt(R[0] * R[0] + R[1] * R[1] + R[2] * R[2]);
    R[0] /= range;
    R[1] /= range;
    R[2] /= range;

    auto range_rate = 1000 * ((V[0] - obs.V[0]) * R[0] + (V[1] - obs.V[1]) * R[1] + V[2] * R[2]);    // m/s

    double u = R[0] * obs.U[0] + R[1] * obs.U[1] + R[2] * obs.U[2];
    double e = R[0] * obs.E[0] + R[1] * obs.E[1] + R[2] * obs.E[2];
    double n = R[0] * obs.N[0] + R[1] * obs.N[1] + R[2] * obs.N[2];

    auto az = DEGREES(atan2(e, n));
    if (az < 0) az += 360.F;

    auto alt = DEGREES(asin(u));

    // Saemundson refraction, true to apparent, 10C 1000 mbar (29.5 inch Hg)
    alt += (1000.0F / 1010.0F) * (283.0F / (273.0F + 10.0F)) * 1.02F / tan(RADIANS(alt + 10.3F / (alt + 5.11))) / 60.0F;

    return std::make_tuple(alt, az, range, range_rate);
}

// subsat location
std::tuple<double, double>
Satellite::geo() {
    double r = sqrt(S[0] * S[0] + S[1] * S[1]);
    double lat = atan2(S[2], r);
    double lng = atan2(S[1], S[0]);
    return std::make_tuple(lat, lng);
}

// celestial coords
std::tuple<double, double>
Satellite::celest() {
    double r = sqrt(SAT[0] * SAT[0] + SAT[1] * SAT[1]);
    double lat = atan2(SAT[2], r);
    double lng = atan2(SAT[1], SAT[0]);
    return std::make_tuple(lat, lng);
}

// period, days
double
Satellite::period() const {
    // MM is radians per day -> 1/MM is days/radian -> mult by 2PI to get days/rev
    return ((2 * M_PI) / MM);
}

// return great-circle radius from subsat point to viewing circle at given altitude
double
Satellite::viewingRadius(double alt) {
    double h = sqrt(S[0] * S[0] + S[1] * S[1] + S[2] * S[2]);
    return (acos(P13::RE / h * cos(alt)) - alt);
}

bool
Satellite::eclipsed(const Sun &sp) {
    double CUA = -(SAT[0] * sp.SUN[0] + SAT[1] * sp.SUN[1] + SAT[2] * sp.SUN[2]) / RS;
    double UMD = RS * sqrt(1 - CUA * CUA) / P13::RE;
    return (UMD <= 1 && CUA >= 0);
}

DateTime Satellite::epoch() const {
    DateTime dt{};
    dt.DN = DE;
    dt.TN = TE;
    return (dt);
}

//----------------------------------------------------------------------

void
Sun::predict(const DateTime &dt) {
    long DN = dt.DN;
    double TN = dt.TN;

    double T = (double) (DN - DateTime::fnday((long) P13::YG, 1, 0)) + TN;
    double GHAE = RADIANS(P13::G0) + T * P13::WE;
    double MRSE = RADIANS(P13::G0) + T * P13::WW + M_PI;
    double MASE = RADIANS(P13::MAS0 + T * P13::MASD);
    double TAS = MRSE + P13::EQC1 * sin(MASE) + P13::EQC2 * sin(2.F * MASE);
    double C, S;

    C = cos(TAS);
    S = sin(TAS);
    SUN[0] = C;
    SUN[1] = S * P13::CNS;
    SUN[2] = S * P13::SNS;
    C = cos(-GHAE);
    S = sin(-GHAE);
    H[0] = SUN[0] * C - SUN[1] * S;
    H[1] = SUN[0] * S + SUN[1] * C;
    H[2] = SUN[2];
}


#ifdef _DATETIME_UNITTEST

#include <stdio.h>

#include "P13.h"

int main (int ac, char *av[])
{
    DateTime t0(2019, 1, 1, 0, 0, 0);
    DateTime t1 = t0;
    DateTime t2(2019, 1, 1, 12, 0, 0);

    printf ("t1 < t2:    1 =?= %d\n", t1 < t2);
    printf ("t1 > t2:    0 =?= %d\n", t1 > t2);
    printf ("t2 - t1:  0.5 =?= %g\n", t2 - t1);
    printf ("t1 - t2: -0.5 =?= %g\n", t1 - t2);

    printf ("\nt1 += 2 days\n");
    t1 += 2.0F;

    printf ("t1 < t2:    0 =?= %d\n", t1 < t2);
    printf ("t1 > t2:    1 =?= %d\n", t1 > t2);
    printf ("t1 - t0:    2 =?= %g\n", t1 - t0);
    printf ("t1 - t2:  1.5 =?= %g\n", t1 - t2);
    printf ("t2 - t1: -1.5 =?= %g\n", t2 - t1);

    printf ("\nt2 += %ld seconds\n", 2*24*3600L);
    t2 += 2*24*3600L;

    printf ("t1 < t2:    1 =?= %d\n", t1 < t2);
    printf ("t1 > t2:    0 =?= %d\n", t1 > t2);
    printf ("t1 - t2: -0.5 =?= %g\n", t1 - t2);
    printf ("t2 - t0:  2.5 =?= %g\n", t2 - t0);
    printf ("t2 - t1:  0.5 =?= %g\n", t2 - t1);

    printf ("\nt2 < t1 + -1.0F: 0 =?= %d\n", t2 < t1 + -1.0F);

    return (0);
}

#endif // _DATETIME_UNITTEST
