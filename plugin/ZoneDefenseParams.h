// ZoneDefenseParams.h  --  lsZoneDefense per-market / per-period thresholds.
// WRITTEN BY the jumpstart (zd/gen_params.py) and then ONLY by the nightly self-improvement part zone_defense:nightly
// (lra.zone_defense DEPLOY, after its JUDGE passes on held-out days). Do not hand-edit: the nightly overwrites it.
// PARAMS_STAMP: 2026-11-08 16:21 (71 market-days of raw MBO); LIVE floors 2026-11-09 (lsZoneDefense 1.0.5, zone_defense 1.5.0)
// (1.0.5) minLots = the LIVE-book floor: p60 of the zones the recorded live book shows (last 10 sessions; ES 10, NQ/HG 2, GC/CL/EU 3 on hand),
// never below 0.3x the research floor, the research floor where < 20 live zones, raised together to ~60 zones/day (NQ x1.55, GC x1.20, CL x1.05).
// NG: no live recordings - research floors. The research floors are kept in data/zone_defense/live/research_floors.json.
// (1.0.3) at-level floor = max(levelFloor, levelFloorX x the period floor); levelFloorX 1.00 = capped (off) for noise: the 2026-11-08 noise
// check added > 10 zones/day at 0.5-0.8 in ES NQ CL GC HG NG (13 fixture days); EU on at 0.5. The nightly re-checks it.
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
    double levelReach = 1.0;                                    // (1.0.2 levels) AT a level = overlapping it or out to this x the 3-min ATR beyond it (his 10-27 rule; nightly 0.5 / 1.0 / 1.5)
    long long levelFloor = 0;                                   // (1.0.3) the at-level size floor = max(this, levelFloorX x the period floor); 0 = off (the chart floor)
    double levelFloorX = 0.5;                                   // (1.0.3) the share of the chart floor an at-level zone needs (0.5 = his rule; a market capped for noise: higher)
};
inline const Mkt& forMarket(const std::string& m) {
    static const Mkt T[] = {
        { "ES", 510, 900, 100, -1, 0, 1, { {135,2.00,0.019},{149,2.00,0.011},{173,2.00,0.015},{225,2.00,0.009},{308,2.00,0.022},{285,2.00,0.014},{320,2.00,0.021},{342,2.00,0.023},{260,2.00,0.025} }, 1.0, 100, 1.00 },   // 10 days, 53.5 touched/day; reload 2.0 held9 -0.20; pull +0.03; swept +0.09; live floors (10 sessions)
        { "NQ", 510, 900, 20, -1, 0, 1, { {19,2.00,0.000},{19,2.00,0.000},{21,2.00,0.000},{36,2.00,0.000},{42,2.00,0.043},{42,2.00,0.043},{24,2.00,0.040},{42,2.00,0.035},{19,2.00,0.029} }, 1.0, 15, 1.00 },   // 11 days, 242.6 touched/day; reload 2.0 held9 -0.17; pull -0.03; swept +0.09; live floors (2 sessions)
        { "CL", 480, 810, 30, -1, 0, 1, { {31,0.25,0.000},{33,0.25,0.033},{31,0.25,0.036},{44,0.25,0.051},{63,0.25,0.039},{44,0.25,0.039},{56,0.25,0.039},{47,0.25,0.027},{40,0.25,0.032} }, 1.0, 25, 1.00 },   // 10 days, 101.0 touched/day; reload 0.25 held9 -0.15; pull +0.08; swept +0.10; live floors (3 sessions)
        { "NG", 480, 810, 30, -1, -1, 1, { {59,0.50,0.000},{63,0.50,0.024},{68,0.50,0.038},{88,0.50,0.034},{96,0.50,0.023},{111,0.50,0.008},{111,0.50,0.000},{116,0.50,0.019},{95,0.50,0.016} }, 1.0, 40, 1.00 },   // 10 days, 43.4 touched/day; reload 0.5 held9 -0.29; pull -0.07; swept +0.15
        { "GC", 440, 750, 20, -1, -1, 1, { {26,1.00,0.000},{38,1.00,0.000},{33,1.00,0.000},{39,1.00,0.000},{39,1.00,0.000},{46,1.00,0.000},{39,1.00,0.000},{52,1.00,0.000},{30,1.00,0.000} }, 1.0, 15, 1.00 },   // 10 days, 165.7 touched/day; reload 1.0 held9 -0.18; pull -0.20; swept +0.12; live floors (3 sessions)
        { "HG", 430, 720, 20, -1, -1, 1, { {26,0.50,0.000},{36,0.50,0.000},{37,0.50,0.000},{35,0.50,0.000},{64,0.50,0.000},{44,0.50,0.000},{34,0.50,0.000},{44,0.50,0.000},{33,0.50,0.000} }, 1.0, 18, 1.00 },   // 10 days, 63.3 touched/day; reload 0.5 held9 -0.16; pull -0.12; swept +0.18; live floors (2 sessions)
        { "EU", 440, 840, 100, 0, 0, 0, { {532,1.00,0.000},{640,1.00,0.000},{838,1.00,0.007},{784,1.00,0.000},{692,1.00,0.000},{856,1.00,0.000},{834,1.00,0.008},{700,1.00,0.000},{739,1.00,0.019} }, 1.0, 400, 0.50 },   // 10 days, 3.8 touched/day; reload 1.0 held9 +0.00; pull +0.10; swept +0.11; live floors (3 sessions)
    };
    for (size_t i = 0; i < sizeof(T) / sizeof(T[0]); ++i) if (m == T[i].name) return T[i];
    return T[0];
}
} // namespace zdp
