// DayTypeLogic.h 1.0.0 (2026-10-22 evening, Rassul approved the build 17:39 CT): the DAY TYPE folded into the HOD / LOD box's trend
// text - "Uptrend . Trend day ?" (green) / "Dntrend . Trend day ?" (red) / "Balance . Rotation day ?" (plain). SDK-free, unit-tested.
// FIRST VERSION = PRICE ONLY, decided live from the CLOSED RTH bars (no look-ahead), after the initial balance (IB, first 60 min):
// Dalton's day types (Mind Over Markets; the auction-market-theory skill): a TREND day extends one side of the IB only, keeps going (range
// >= EXT_X x the IB), holds VWAP on its side and trades near its extreme; a ROTATION / balance day stays near its IB (range <= ROT_X x IB,
// or both sides extended = neutral) and keeps crossing VWAP. "?" = not proven: the nightly scores every call (logged in the status file).
// The MBO tells (defences failing on one side, one-sided trails, no Sr on the losing side, one-sided big market orders, the losing side
// keeps pulling) are tested by the nightly first; HOOK: PARAM,MBO_TELLS,0 in lsFlexLevels\LRA-DayType-<MKT>.csv (0 = price only;
// the nightly sets 1 once proven and a later build adds them to Inputs::mbo).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace dtl {
static const char* const VERSION = "1.0.0";
enum Kind { NONE = 0, TREND_UP = 1, TREND_DN = -1, ROTATION = 2 };

struct Bar { double h = 0, l = 0, c = 0, v = 0; int endMin = 0; };   // endMin = minutes from the RTH open to the bar's END
struct Params {
    int ibMin = 60;               // the initial balance
    double extX = 1.5;            // trend: range >= 1.5 x IB
    double vwapHold = 0.80;       // trend: >= 80% of the bars after the IB closed on the trend side of VWAP (and the last one)
    double closePct = 0.35;       // trend: the last close in the top / bottom 35% of the day's range
    double rotX = 1.25;           // rotation: range <= 1.25 x IB (or both IB sides extended)
    int rotCross = 4;             // rotation: >= 4 VWAP crosses after the IB
    int rotMin = 120;             // rotation: not called before 2 hours into RTH
    int mboTells = 0;             // the hook (0 = price only)
    int show = 1;                 // 0 = log only (the trend word stays as before)
};
struct Check { std::string what; bool ok; };
struct Result { int kind = NONE; bool ready = false; std::string text; std::vector<Check> checks; std::string why;
                double ib = 0, range = 0, vwap = 0, holdUp = 0, holdDn = 0, closeLoc = 0; int crosses = 0, bars = 0; bool extUp = false, extDn = false; };

inline std::string x1(double v) { char b[32]; std::snprintf(b, sizeof b, "%.1f", v); return b; }
inline std::string pct(double v) { char b[32]; std::snprintf(b, sizeof b, "%d%%", (int)std::lround(v * 100)); return b; }

// bars = today's CLOSED RTH bars in time order
inline Result classify(const std::vector<Bar>& bars, const Params& P) {
    Result R; R.bars = (int)bars.size();
    if (bars.empty()) { R.why = "no RTH bars yet"; return R; }
    double ibH = -1e300, ibL = 1e300, dayH = -1e300, dayL = 1e300, pv = 0, vv = 0; bool ibDone = false;
    for (size_t i = 0; i < bars.size(); ++i) {
        const Bar& b = bars[i];
        if (!std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(b.c) || b.h < b.l) { R.why = "bad bar"; return R; }
        if (b.endMin <= P.ibMin) { ibH = std::max(ibH, b.h); ibL = std::min(ibL, b.l); }
        if (b.endMin >= P.ibMin) ibDone = true;
        dayH = std::max(dayH, b.h); dayL = std::min(dayL, b.l);
    }
    if (!ibDone || !(ibH >= ibL)) { R.why = "initial balance not complete"; return R; }
    R.ib = ibH - ibL; R.range = dayH - dayL;
    if (!(R.ib > 0)) { R.why = "zero initial balance"; return R; }
    R.extUp = dayH > ibH; R.extDn = dayL < ibL;
    int after = 0, up = 0, dn = 0, lastSide = 0;
    for (size_t i = 0; i < bars.size(); ++i) {
        const Bar& b = bars[i]; const double w = (std::isfinite(b.v) && b.v > 0) ? b.v : 1.0;
        pv += (b.h + b.l + b.c) / 3.0 * w; vv += w;
        const double vw = pv / vv;
        if (b.endMin <= P.ibMin) continue;
        ++after; const int side = b.c > vw ? 1 : b.c < vw ? -1 : 0;
        if (side > 0) ++up; else if (side < 0) ++dn;
        if (side != 0) { if (lastSide != 0 && side != lastSide) ++R.crosses; lastSide = side; }
    }
    R.vwap = vv > 0 ? pv / vv : 0;
    R.holdUp = after ? (double)up / after : 0; R.holdDn = after ? (double)dn / after : 0;
    const double lastC = bars.back().c; R.closeLoc = R.range > 0 ? (lastC - dayL) / R.range : 0.5;
    R.ready = true;
    const double rx = R.range / R.ib; const int nowMin = bars.back().endMin;
    const bool lastUp = lastC > R.vwap, lastDn = lastC < R.vwap;
    // trend up / down: all four
    const bool upOne = R.extUp && !R.extDn, dnOne = R.extDn && !R.extUp, wide = rx >= P.extX;
    const bool tUp = upOne && wide && R.holdUp >= P.vwapHold && lastUp && R.closeLoc >= 1.0 - P.closePct;
    const bool tDn = dnOne && wide && R.holdDn >= P.vwapHold && lastDn && R.closeLoc <= P.closePct;
    const bool rot = nowMin >= P.rotMin && (rx <= P.rotX || (R.extUp && R.extDn)) && R.crosses >= P.rotCross;
    if (tUp || (!tDn && upOne)) {
        R.checks.push_back(Check{ "IB extended up only (" + std::string(R.extDn ? "also down" : "not down") + ")", upOne });
        R.checks.push_back(Check{ "range " + x1(rx) + "x the IB (needs " + x1(P.extX) + "x)", wide });
        R.checks.push_back(Check{ pct(R.holdUp) + " of bars after the IB closed above VWAP (needs " + pct(P.vwapHold) + "), last " + (lastUp ? "above" : "not above"), R.holdUp >= P.vwapHold && lastUp });
        R.checks.push_back(Check{ "last close at " + pct(R.closeLoc) + " of the range (needs top " + pct(P.closePct) + ")", R.closeLoc >= 1.0 - P.closePct });
    } else if (tDn || dnOne) {
        R.checks.push_back(Check{ "IB extended down only (" + std::string(R.extUp ? "also up" : "not up") + ")", dnOne });
        R.checks.push_back(Check{ "range " + x1(rx) + "x the IB (needs " + x1(P.extX) + "x)", wide });
        R.checks.push_back(Check{ pct(R.holdDn) + " of bars after the IB closed below VWAP (needs " + pct(P.vwapHold) + "), last " + (lastDn ? "below" : "not below"), R.holdDn >= P.vwapHold && lastDn });
        R.checks.push_back(Check{ "last close at " + pct(R.closeLoc) + " of the range (needs bottom " + pct(P.closePct) + ")", R.closeLoc <= P.closePct });
    }
    R.checks.push_back(Check{ "range " + x1(rx) + "x the IB (rotation: <= " + x1(P.rotX) + "x, or both sides extended)", rx <= P.rotX || (R.extUp && R.extDn) });
    R.checks.push_back(Check{ std::to_string(R.crosses) + " VWAP crosses after the IB (rotation: needs " + std::to_string(P.rotCross) + ")", R.crosses >= P.rotCross });
    R.checks.push_back(Check{ std::to_string(nowMin) + " min into RTH (rotation: from " + std::to_string(P.rotMin) + ")", nowMin >= P.rotMin });
    if (tUp) { R.kind = TREND_UP; R.text = "Uptrend \xB7 Trend day ?"; R.why = "trend up"; }
    else if (tDn) { R.kind = TREND_DN; R.text = "Dntrend \xB7 Trend day ?"; R.why = "trend down"; }
    else if (rot) { R.kind = ROTATION; R.text = "Balance \xB7 Rotation day ?"; R.why = "rotation"; }
    else R.why = "no day type yet";
    return R;
}

// the hover: the question, what it is, why it matters, how to use it, an example, then what fired ("+" passed / "-" not)
inline std::vector<std::string> hover(const Result& R) {
    std::vector<std::string> o;
    o.push_back(std::string("Day type ?  ") + (R.kind == TREND_UP ? "Trend day up" : R.kind == TREND_DN ? "Trend day down" : R.kind == ROTATION ? "Rotation (balance) day" : "not called yet"));
    o.push_back("Question: is today a one-way TREND day (fade nothing) or a ROTATION day (fade the edges)?");
    o.push_back("What it is: Dalton's day types from price only - the first hour (IB), how far and to which side the day extended, VWAP, the close.");
    o.push_back("Why it matters: on a trend day reversals at key levels fail more often; on a rotation day the IB edges and VWAP turn price.");
    o.push_back("How to use: Trend day = take only with-trend turns (pullbacks); Rotation day = fade the extremes back to VWAP. '?' = not proven yet.");
    o.push_back("Example: ES extends above the IB only, range 1.8x the IB, 90% of closes above VWAP, close in the top 20% -> Uptrend . Trend day ?");
    o.push_back(std::string("What fired (") + std::to_string(R.bars) + " closed RTH bars, IB " + x1(R.ib) + " pts, range " + x1(R.range) + " pts):");
    for (size_t i = 0; i < R.checks.size(); ++i) o.push_back(std::string(R.checks[i].ok ? "  + " : "  - ") + R.checks[i].what);
    o.push_back("MBO tells (defences failing, one-sided trails / big market orders, pulling) are tested by the nightly first.");
    return o;
}
// the status / log line (the nightly scores each call)
inline std::string statusLine(const Result& R, const Params& P) {
    return std::string("DAYTYPE,") + VERSION + "," + (R.kind == TREND_UP ? "TREND_UP" : R.kind == TREND_DN ? "TREND_DN" : R.kind == ROTATION ? "ROTATION" : "NONE") +
           ",ib " + x1(R.ib) + ",range " + x1(R.range) + ",ext " + (R.extUp ? "U" : "") + (R.extDn ? "D" : "") + ",holdUp " + pct(R.holdUp) + ",holdDn " + pct(R.holdDn) +
           ",crosses " + std::to_string(R.crosses) + ",closeLoc " + pct(R.closeLoc) + ",bars " + std::to_string(R.bars) + ",show " + std::to_string(P.show) + ",mbo_tells " + std::to_string(P.mboTells) + "," + R.why;
}
} // namespace dtl
