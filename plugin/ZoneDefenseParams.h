// ZoneDefenseParams.h  --  lsZoneDefense per-market / per-period thresholds.
// WRITTEN BY the jumpstart (zd/gen_params.py) and then ONLY by the nightly self-improvement part zone_defense:nightly
// (lra.zone_defense DEPLOY, after its JUDGE passes on held-out days). Do not hand-edit: the nightly overwrites it.
// PARAMS_STAMP: 2026-11-08 16:21 (71 market-days of raw MBO)
// Signs (decided on held-out halves, |diff| >= 5 pts, same sign in both halves, else 0 = drawn white):
//   reloadSign  -1 = heavy selling into the zone in the 60 s after the touch (sold >= reloadX x shown) means it breaks MORE (red)
//   pullSign    -1 = the queue pulling before the touch means it breaks more (red); 0 = drawn white
//   sweepSign   +1 = a swept stop run (lift >= 1.0x, back in the zone) holds more than a weak one (green / red triangles)
// The locked design values (zone geometry, 60-s reload, lift 1.0x, 5-s lift window, 30-s queue rest) live in ZoneDefenseLogic.h.
#pragma once
#include <string>

namespace zdp {
enum { P_ASIA, P_LONDON, P_PREOPEN, P_OPEN, P_MORNING, P_LUNCH, P_AFTERNOON, P_CLOSE, P_POST, P_N };
static const char* const PER_NAME[P_N] = { "asia", "london", "preopen", "open", "morning", "lunch", "afternoon", "close", "post" };
struct Per { long long minLots; double reloadX; double cancelNorm; };
struct Mkt {
    const char* name; int openMin, closeMin;                    // CT minute of the RTH open / close
    long long bigLots;                                          // one big-order line per zone at >= this size
    int reloadSign, pullSign, sweepSign;
    Per per[P_N];                                               // minLots = the period's zone-size p60 (what is drawn), reloadX, the queue's normal cancel share
};
inline const Mkt& forMarket(const std::string& m) {
    static const Mkt T[] = {
        { "ES", 510, 900, 100, -1, 0, 1, { {198,2.00,0.019},{203,2.00,0.011},{247,2.00,0.015},{252,2.00,0.009},{299,2.00,0.022},{330,2.00,0.014},{348,2.00,0.021},{363,2.00,0.023},{251,2.00,0.025} } },   // 10 days, 53.5 touched/day; reload 2.0 held9 -0.20; pull +0.03; swept +0.09
        { "NQ", 510, 900, 20, -1, 0, 1, { {20,2.00,0.000},{19,2.00,0.000},{19,2.00,0.000},{24,2.00,0.000},{25,2.00,0.043},{27,2.00,0.043},{30,2.00,0.040},{27,2.00,0.035},{23,2.00,0.029} } },   // 11 days, 242.6 touched/day; reload 2.0 held9 -0.17; pull -0.03; swept +0.09
        { "CL", 480, 810, 30, -1, 0, 1, { {48,0.25,0.000},{47,0.25,0.033},{42,0.25,0.036},{41,0.25,0.051},{48,0.25,0.039},{50,0.25,0.039},{53,0.25,0.039},{44,0.25,0.027},{42,0.25,0.032} } },   // 10 days, 101.0 touched/day; reload 0.25 held9 -0.15; pull +0.08; swept +0.10
        { "NG", 480, 810, 30, -1, -1, 1, { {59,0.50,0.000},{63,0.50,0.024},{68,0.50,0.038},{88,0.50,0.034},{96,0.50,0.023},{111,0.50,0.008},{111,0.50,0.000},{116,0.50,0.019},{95,0.50,0.016} } },   // 10 days, 43.4 touched/day; reload 0.5 held9 -0.29; pull -0.07; swept +0.15
        { "GC", 440, 750, 20, -1, -1, 1, { {30,1.00,0.000},{33,1.00,0.000},{35,1.00,0.000},{32,1.00,0.000},{36,1.00,0.000},{38,1.00,0.000},{32,1.00,0.000},{43,1.00,0.000},{33,1.00,0.000} } },   // 10 days, 165.7 touched/day; reload 1.0 held9 -0.18; pull -0.20; swept +0.12
        { "HG", 430, 720, 20, -1, -1, 1, { {28,0.50,0.000},{35,0.50,0.000},{37,0.50,0.000},{35,0.50,0.000},{39,0.50,0.000},{44,0.50,0.000},{34,0.50,0.000},{44,0.50,0.000},{33,0.50,0.000} } },   // 10 days, 63.3 touched/day; reload 0.5 held9 -0.16; pull -0.12; swept +0.18
        { "EU", 440, 840, 100, 0, 0, 0, { {678,1.00,0.000},{808,1.00,0.000},{838,1.00,0.007},{784,1.00,0.000},{763,1.00,0.000},{856,1.00,0.000},{834,1.00,0.008},{700,1.00,0.000},{739,1.00,0.019} } },   // 10 days, 3.8 touched/day; reload 1.0 held9 +0.00; pull +0.10; swept +0.11
    };
    for (size_t i = 0; i < sizeof(T) / sizeof(T[0]); ++i) if (m == T[i].name) return T[i];
    return T[0];
}
} // namespace zdp
