// test_vwapodds_logic.cpp -- VwapOddsLogic.h (VW100). No SDK. g++ -std=c++17 -Wall -Wextra test_vwapodds_logic.cpp
// With an argument (a CSV from vw100_odds_parity.py: m,kind,line,p,dist,tr,via,endMin,dsd,p_python) it also checks the C++
// layer against lra.vwap_odds.apply_calib on real rows.
#ifdef VW_WINDOWS_H
#include <windows.h>
#endif
#ifdef VW_WINMACROS
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define near
#define far
#endif
#include "SessionVWAPLogic.h"
#include "VwapOddsLogic.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) passes++; else { fails++; printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static double ex(double z) { return 1.0 / (1.0 + std::exp(-z)); }

int main(int argc, char** argv)
{
    // a hand-made row
    VOCal c = { "XX", 2, 1, 0, { 0.1, 0.2, 0.3, 0.4, 0.5 }, { 1.0, 0.9, 1.1, 1.2, 0.8 }, -0.2, -0.3, 0.25, 0.15, { 0.6, -0.1, 0.4 }, 0.0 };
    double p = 0.3, lg = std::log(0.3 / 0.7);
    CHECK(std::fabs(vol::calibrate(&c, 0, p, 1.0, 0, 0, -1, 1.0) - ex(0.1 + lg)) < 1e-12, "no trend, reference time: a + b logit p");
    CHECK(std::fabs(vol::calibrate(&c, 3, p, 2.0, 1, 2, 0, 1.0) - ex(0.4 + 1.2 * lg - 0.2 + 0.6)) < 1e-12, "+2SD above price in an Uptrend via 2SD, first hour");
    CHECK(std::fabs(vol::calibrate(&c, 4, p, -2.0, 1, 1, 1, 1.0) - ex(0.5 + 0.8 * lg + 0.15 - 0.1)) < 1e-12, "-2SD below price in an Uptrend via majority = against the trend");
    CHECK(std::fabs(vol::calibrate(&c, 2, p, -1.0, -1, 2, 2, 1.0) - ex(0.3 + 1.1 * lg - 0.2 + 0.4)) < 1e-12, "-1SD below price in a Dntrend = with the trend");
    CHECK(std::fabs(vol::calibrate(&c, 1, p, 0.0, -1, 2, -1, 1.0) - ex(0.2 + 0.9 * lg - 0.2)) < 1e-12, "a line exactly at price counts as below (Python: dist > 0 is above)");
    CHECK(std::fabs(vol::calibrate(&c, 0, 0.0, 1, 0, 0, -1, 1) - ex(0.1 + std::log(1e-4 / (1 - 1e-4)))) < 1e-12, "p = 0 is clipped to 1e-4");
    CHECK(vol::calibrate(&c, 5, p, 1, 0, 0, -1, 1) == p && vol::calibrate(&c, -1, p, 1, 0, 0, -1, 1) == p, "bad line index: unchanged");
    CHECK(std::isnan(vol::calibrate(&c, 0, std::numeric_limits<double>::quiet_NaN(), 1, 0, 0, -1, 1)), "NaN p stays NaN (no badge)");
    CHECK(vol::calibrate(&c, 0, p, std::numeric_limits<double>::infinity(), 0, 0, -1, 1) == p, "non-finite distance: unchanged");
    CHECK(vol::calibrate(nullptr, 0, p, 1, 0, 0, -1, 1) == p, "no row: unchanged");
    VOCal off = c; off.calib = 0;
    CHECK(vol::calibrate(&off, 0, p, 1, 1, 2, 0, 1) == p, "calib 0: the law's own number");
    VOCal dd = c; dd.dsd = 0.5;
    CHECK(std::fabs(vol::calibrate(&dd, 0, p, 1, 0, 0, -1, 10.0) - ex(0.1 + lg + 0.5 * std::log(6.0))) < 1e-12, "dsd clipped at 6");
    CHECK(!vol::proven(&c) && !vol::proven(nullptr), "proven flag");
    // time of day buckets (ES 08:30-15:00; overnight Central)
    CHECK(vol::todIndex(2, 9 * 60 + 15, 510, 900) == 0 && vol::todIndex(2, 9 * 60 + 30, 510, 900) == 0, "RTH open 30-60 min");
    CHECK(vol::todIndex(2, 9 * 60 + 45, 510, 900) == 1 && vol::todIndex(2, 10 * 60 + 30, 510, 900) == 1, "RTH 60-120 min");
    CHECK(vol::todIndex(2, 12 * 60, 510, 900) == -1, "RTH midday = reference");
    CHECK(vol::todIndex(2, 14 * 60, 510, 900) == 2 && vol::todIndex(2, 14 * 60 - 3, 510, 900) == -1, "RTH last hour");
    CHECK(vol::todIndex(1, 17 * 60 + 45, 510, 900) == 0 && vol::todIndex(1, 0, 510, 900) == 0 && vol::todIndex(1, 59, 510, 900) == 0, "O/N evening 17-01");
    CHECK(vol::todIndex(1, 60, 510, 900) == 1 && vol::todIndex(1, 119, 510, 900) == 1, "O/N 01-02");
    CHECK(vol::todIndex(1, 120, 510, 900) == 2 && vol::todIndex(1, 419, 510, 900) == 2, "O/N London 02-07");
    CHECK(vol::todIndex(1, 420, 510, 900) == -1 && vol::todIndex(1, 1440, 510, 900) == -1 && vol::todIndex(1, -1, 510, 900) == -1, "pre-open reference / bad minute");
    // the generated table: every market, both sessions, finite numbers
    const char* M[7] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
    bool all = true, fin = true;
    for (int i = 0; i < 7; i++) for (int k = 1; k <= 2; k++) {
        const VOCal* r = vol::find(M[i], k);
        if (!r) { all = false; continue; }
        for (int j = 0; j < 5; j++) fin = fin && std::isfinite(r->a[j]) && std::isfinite(r->b[j]) && r->b[j] > 0.2 && r->b[j] < 3;
        fin = fin && std::isfinite(r->tw2) && std::isfinite(r->ta2) && std::isfinite(r->tod[0]);
    }
    CHECK(all && VO_N == 14, "a row for every market and session");
    CHECK(fin, "generated coefficients finite and sane");
    CHECK(vol::find("XX", 2) == nullptr && vol::find(nullptr, 2) == nullptr, "unknown market");
    // parity with the Python layer on real rows
    if (argc > 1) {
        std::ifstream in(argv[1]); std::string line; int n = 0; double worst = 0;
        std::getline(in, line);
        while (std::getline(in, line)) {
            std::stringstream ss(line); std::string f[10]; for (int k = 0; k < 10; k++) std::getline(ss, f[k], ',');
            int kind = atoi(f[1].c_str()), o = 0, cl = 0; svl::rthOf(f[0].c_str(), o, cl);
            const VOCal* r = vol::find(f[0].c_str(), kind);
            double q = vol::calibrate(r, atoi(f[2].c_str()), atof(f[3].c_str()), atof(f[4].c_str()), atoi(f[5].c_str()), atoi(f[6].c_str()),
                                      vol::todIndex(kind, atoi(f[7].c_str()), o, cl), atof(f[8].c_str()));
            double d = std::fabs(q - atof(f[9].c_str())); if (d > worst) worst = d; n++;
        }
        printf("parity rows %d, worst |C++ - Python| = %.3g\n", n, worst);
        CHECK(n > 1000 && worst < 1e-6, "C++ layer == lra.vwap_odds.apply_calib on real rows");
    }
    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
