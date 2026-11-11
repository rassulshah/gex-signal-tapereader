#ifndef LS_STEPTREND_PARAMS_H
#define LS_STEPTREND_PARAMS_H
// StepTrendParams.h (1.0.0, 2026-11-11) -- per-market values of lsStepTrend. Written by hand from Rassul's rule (2026-11-10 22:28 - 23:53 CT:
// "the bar breaks out with a higher close and high above the prior bar high", "did you put the zig zag wave rotation requirement",
// clock hours; 2026-11-11 00:11 the breakout hour is a green / red candle). The nightly lra.step_trend_score scores this rule against its challengers (rotation off, any candle colour, rule B, hours from the session
// open) on days that did not choose them and REPORTS; the rule is his definition, so a change is brought to him and written here by hand.
//   rule 0 = the hour CLOSES beyond the prior hour's high (up) / low (down)      rule 1 = a higher high and a higher close (mirror down)
//   body 1 = the breakout hour itself must close green for a step up / red for a step down (Rassul 2026-11-11 00:11 "the bar closing
//            higher needs to be green and vice versa")   0 = any candle
//   rot  1 = the hourly ZigZag (High / Low, 1 bar) must be rotating the breakout's way (or have no rotation yet)   0 = every breakout
#include <string>
namespace stpp {
struct Mkt { const char* name; int rule, rot, body; };
inline const Mkt& forMarket(const std::string& m) {
    static const Mkt T[] = { { "ES", 0, 1, 1 }, { "NQ", 0, 1, 1 }, { "CL", 0, 1, 1 }, { "GC", 0, 1, 1 }, { "HG", 0, 1, 1 }, { "NG", 0, 1, 1 }, { "EU", 0, 1, 1 }, { "", 0, 1, 1 } };
    for (int i = 0; i < 7; ++i) if (m == T[i].name) return T[i];
    return T[7];
}
} // namespace stpp
#endif
