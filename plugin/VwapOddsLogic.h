// VwapOddsLogic.h -- the calibration layer on lsSessionVWAP's touch-odds badges (VW100, 2026-10-10). No SDK, no file at run time:
// the coefficients are compiled in from VwapOddsParams.h, which lra.vwap_odds rewrites only when its nightly judge adopts a better
// fit on sessions that did not pick it (walk-forward + a 20-session holdout).
//
//   p' = expit( a[line] + b[line] * logit(p) + trend terms + time-of-day term + dsd term )        (calib = 1)
//   p' = p                                                                                          (calib = 0: the law as is)
//   p        the law's own chance (RTH: TouchParams.h forecast; overnight: OnTouchLogic.h)
//   trend    vwapTrend of the same session (VwapTrendLogic.h): tw2 / tw1 when the line is on the trend's side (via 2SD touch /
//            via majority), ta2 / ta1 when it is on the other side
//   tod      RTH: open 30-60 min, 60-120 min, last hour (midday = 0); overnight: 17:00-01:00, 01:00-02:00, 02:00-07:00 (pre-open = 0)
//   dsd      log(distance in SD, clipped 0.05..6)
// The arithmetic is the same as lra.vwap_odds.apply_calib (tested: test_vwapodds_logic.cpp + the oracle parity on 17 months).
// "proven" = the market / session passed the '?' rule out of sample; otherwise the badge keeps its "?".
#pragma once
#include <cmath>
#include <cstring>
#include "VwapOddsParams.h"

namespace vol {

inline const VOCal* find(const char* m, int kind)
{
    if (!m) return nullptr;
    for (int i = 0; i < VO_N; i++) if (VO_CAL[i].kind == kind && std::strcmp(VO_CAL[i].m, m) == 0) return &VO_CAL[i];
    return nullptr;
}

// kind 2 = RTH, 1 = overnight; endMin = minute of day of the bar's END stamp (the decision time)
inline int todIndex(int kind, int endMin, int openMin, int closeMin)
{
    if (endMin < 0 || endMin >= 1440) return -1;
    if (kind == 2) {
        int since = endMin - openMin;
        if (since <= 60) return 0;
        if (since <= 120) return 1;
        if (closeMin - endMin <= 60) return 2;
        return -1;
    }
    if (endMin >= 17 * 60 || endMin < 60) return 0;
    if (endMin < 120) return 1;
    if (endMin < 7 * 60) return 2;
    return -1;
}

// line 0 VWAP, 1 +1SD, 2 -1SD, 3 +2SD, 4 -2SD; dist = line - price; trendState +1 / -1 / 0, trendVia 2 (2SD) / 1 (majority)
// returns p unchanged when there is no layer or an input is not finite
inline double calibrate(const VOCal* c, int line, double p, double dist, int trendState, int trendVia, int tod, double dsd)
{
    if (!c || !c->calib || line < 0 || line > 4 || !std::isfinite(p) || !std::isfinite(dist)) return p;
    double q = p < 1e-4 ? 1e-4 : (p > 1 - 1e-4 ? 1 - 1e-4 : p);
    double z = c->a[line] + c->b[line] * std::log(q / (1 - q));
    bool above = dist > 0;
    bool tw = (trendState == 1 && above) || (trendState == -1 && !above);
    bool ta = (trendState == 1 && !above) || (trendState == -1 && above);
    if (tw && trendVia == 2) z += c->tw2;
    if (tw && trendVia == 1) z += c->tw1;
    if (ta && trendVia == 2) z += c->ta2;
    if (ta && trendVia == 1) z += c->ta1;
    if (tod >= 0 && tod < 3) z += c->tod[tod];
    if (c->dsd != 0 && std::isfinite(dsd)) { double x = dsd < 0.05 ? 0.05 : (dsd > 6 ? 6 : dsd); z += c->dsd * std::log(x); }
    if (!std::isfinite(z)) return p;
    double r = 1.0 / (1.0 + std::exp(-z));
    return r < 0 ? 0 : (r > 1 ? 1 : r);
}

inline bool proven(const VOCal* c) { return c && c->proven != 0; }

} // namespace vol
