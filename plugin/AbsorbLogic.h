#ifndef LS_ABSORBLOGIC_H
#define LS_ABSORBLOGIC_H
// AbsorbLogic.h 1.0.0 (2026-10-21, MBO Roadmap Top Defender #1 "Is the level absorbing the attack?", Rassul approved mockup v2 08:57 CT
// "approved"). SDK-free, unit-tested (tests_absorb/test_absorb.cpp). Shared by lsDeltaProfile (wick A) and lsBigBidAsk (absorbed circle).
//
// What the testing showed (research_absorption.md, research_testing.md; 7 markets x 12 sessions, ~20,000 candles at a local high / low
// checked against the CME order book): the footprint alone cannot tell real absorption - at 1-4 marks a session only 10-20% were real
// in the order book. Its best sign is the attackers' lots in the WICK vs the same half hour on earlier sessions (real 3.1x, other 0.9x).
// So a mark needs BOTH:
//   1. the footprint candidate: the candle is at a local extreme (its high >= the 5 highs before it / low <= the 5 lows before it), it
//      has a wick beyond its body, and the attackers' lots IN the wick are >= X x this market's normal for that half hour (slot norm);
//   2. the order-book truth (lsBidAskStats, SignalBoard kind 'O'): at prices inside the wick the DEFENDERS' resting orders traded
//      >= 2x the most ever shown there and were reloaded >= 2 times, and those prices took >= 25% of the attackers' lots in the wick.
// Absorption may trade through a little - the wick IS the trade-through; there is no fixed-tick limit.
// Decided once at the candle's close (after lsBidAskStats has decided it); never repaints. No order feed on the chart = not drawn.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace absl {
static const char* const VERSION = "1.0.1";   // (1.0.1, 2026-10-21 reverse engineering) the half-hour norm has a floor: 0.5x the market's median over all slots (quiet slots gave 150x - 1500x junk ratios in 6E / overnight)
static const int SLOT_MIN = 30;           // half-hour slots
static const int LOOKBACK_SESSIONS = 10;  // slot norm = median over the last 10 sessions with data in that slot
static const int MIN_OBS = 8;             // fewer observations in a slot = no norm (no mark, logged as learning)

// ---- the slot norm: attackers' wick lots at local-extreme candles, by market, half-hour slot and side -------------------------------
inline int slotOf(int minuteOfDay) { if (minuteOfDay < 0 || minuteOfDay >= 1440) return -1; return minuteOfDay / SLOT_MIN; }
struct Obs { long long session = 0; int slot = -1; int top = 0; double lots = 0; };   // session = civil day of the 17:00 session
inline double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
// the norm for (slot, side) from observations of sessions BEFORE `session` (no look-ahead), the last LOOKBACK_SESSIONS sessions
inline bool slotNorm(const std::vector<Obs>& obs, long long session, int slot, int top, double& norm, int& n) {
    std::map<long long, std::vector<double> > by;
    for (const Obs& o : obs) if (o.session < session && o.slot == slot && o.top == top && o.lots >= 0) by[o.session].push_back(o.lots);
    std::vector<double> v; int used = 0;
    for (std::map<long long, std::vector<double> >::const_reverse_iterator r = by.rbegin(); r != by.rend() && used < LOOKBACK_SESSIONS; ++r, ++used)
        v.insert(v.end(), r->second.begin(), r->second.end());
    n = (int)v.size(); norm = median(v);
    return n >= MIN_OBS && norm > 0;
}
// (1.0.1) the floor under a slot norm: half the median of ALL slots' observations of that side before `session` (last 10 sessions)
inline double floorNorm(const std::vector<Obs>& obs, long long session, int top) {
    std::map<long long, std::vector<double> > by;
    for (const Obs& o : obs) if (o.session < session && o.top == top && o.lots >= 0) by[o.session].push_back(o.lots);
    std::vector<double> v; int used = 0;
    for (std::map<long long, std::vector<double> >::const_reverse_iterator r = by.rbegin(); r != by.rend() && used < LOOKBACK_SESSIONS; ++r, ++used)
        v.insert(v.end(), r->second.begin(), r->second.end());
    return 0.5 * median(v);
}
// file lines: "session,slot,top,lots" (the observations file the plugin appends; the seed file has the same format)
inline std::string obsLine(const Obs& o) { char b[96]; std::snprintf(b, sizeof(b), "%lld,%d,%d,%.0f", o.session, o.slot, o.top, o.lots); return b; }
inline bool parseObs(const std::string& line, Obs& o) {
    long long s = 0; int sl = -1, t = -1; double l = -1; char extra = 0;
    if (std::sscanf(line.c_str(), "%lld,%d,%d,%lf%c", &s, &sl, &t, &l, &extra) < 4) return false;
    if (s <= 0 || sl < 0 || sl >= 48 || (t != 0 && t != 1) || !std::isfinite(l) || l < 0 || l > 1e8) return false;
    o.session = s; o.slot = sl; o.top = t; o.lots = l;
    return true;
}
inline std::vector<Obs> parseObsText(const std::string& text, int* bad = nullptr) {
    std::vector<Obs> v; std::istringstream in(text); std::string ln; int b = 0;
    while (std::getline(in, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (ln.empty() || ln[0] == '#') continue; Obs o; if (parseObs(ln, o)) v.push_back(o); else ++b; }
    if (bad) *bad = b;
    return v;
}

// ---- one candle's footprint: rows = (tick key, bought at the ask, sold at the bid) ---------------------------------------------------
struct Row { long long key = 0; double bought = 0, sold = 0; };
struct Candle { long long open = 0, high = 0, low = 0, close = 0; std::vector<Row> rows; };   // whole-tick keys
// the wick zone of a side: top = (max(open, close), high], bottom = [low, min(open, close))
inline bool wickZone(const Candle& c, bool top, long long& lo, long long& hi) {
    if (top) { lo = std::max(c.open, c.close) + 1; hi = c.high; } else { lo = c.low; hi = std::min(c.open, c.close) - 1; }
    return hi >= lo;
}
inline double wickAttack(const Candle& c, bool top) {
    long long lo = 0, hi = 0; if (!wickZone(c, top, lo, hi)) return 0;
    double a = 0; for (const Row& r : c.rows) if (r.key >= lo && r.key <= hi) a += top ? r.bought : r.sold;
    return a;
}
// local extreme: high >= the `n` highs before it (top) / low <= the `n` lows before it; needs n earlier candles
inline bool localExtreme(const std::vector<long long>& hi, const std::vector<long long>& lo, int i, bool top, int n = 5) {
    if (i < n || i >= (int)hi.size() || i >= (int)lo.size()) return false;
    for (int k = i - n; k < i; ++k) { if (top ? hi[k] > hi[i] : lo[k] < lo[i]) return false; }
    return true;
}

// ---- the order-book truth from the SignalBoard 'O' events of one candle ------------------------------------------------------------
struct ObEv { double px = 0; int side = 0; double traded = 0; long long shown = 0; int refills = 0; };   // side = resting (+1 ask / -1 bid)
// lots the DEFENDERS absorbed inside [lo, hi] (keys): top -> resting asks (+1), bottom -> resting bids (-1)
inline double absorbedIn(const std::vector<ObEv>& ev, double tick, bool top, long long lo, long long hi) {
    double s = 0;
    for (const ObEv& e : ev) {
        if (e.side != (top ? 1 : -1) || !(tick > 0)) continue;
        const long long k = (long long)std::llround(e.px / tick);
        if (k >= lo && k <= hi) s += e.traded;
    }
    return s;
}
inline const ObEv* absorbedAt(const std::vector<ObEv>& ev, double tick, int restingSide, long long key) {
    for (const ObEv& e : ev) if (e.side == restingSide && tick > 0 && (long long)std::llround(e.px / tick) == key) return &e;
    return nullptr;
}

// ---- the wick A ----------------------------------------------------------------------------------------------------------------------
struct Params { double x = 3.0; double share = 0.25; };
// per-market starting X (attack vs the half-hour norm), from the 2026-10-21 test: at X = 3 about half of the real (order-book)
// absorptions at extremes pass the footprint step; the order book then decides. Overridden by the settings window.
inline double defaultX(const std::string& m) { (void)m; return 3.0; }
struct WickResult {
    bool candidate = false, absorbed = false; double attack = 0, norm = 0, x = 0, absorbedLots = 0, share = 0; int normN = 0;
    long long lo = 0, hi = 0; std::string why;
};
// decide one closed candle side. obPresent = lsBidAskStats decided this candle (its marker is on the board)
inline WickResult wickA(const Candle& c, bool top, bool atExtreme, double norm, int normN, bool normOk, bool obPresent,
                        const std::vector<ObEv>& ev, double tick, const Params& P) {
    WickResult r; r.norm = norm; r.normN = normN;
    if (!atExtreme) { r.why = "not at a local extreme"; return r; }
    if (!wickZone(c, top, r.lo, r.hi)) { r.why = "no wick"; return r; }
    r.attack = wickAttack(c, top);
    if (r.attack <= 0) { r.why = "no attack in the wick"; return r; }
    if (!normOk) { r.why = "learning: no half-hour norm yet"; return r; }
    r.x = r.attack / norm;
    if (r.x < P.x) { r.why = "attack under the norm threshold"; return r; }
    r.candidate = true;
    if (!obPresent) { r.why = "no order feed for this candle (add Aggression / lsBidAskStats)"; return r; }
    r.absorbedLots = absorbedIn(ev, tick, top, r.lo, r.hi);
    r.share = r.absorbedLots / r.attack;
    if (r.share < P.share) { r.why = "order book: defenders did not reload enough"; return r; }
    r.absorbed = true; r.why = "absorbed"; return r;
}

// ---- the Big Bid / Ask circle: a drawn print in the wick at a price the order book shows absorbed ---------------------------------
// printSide -1 = sold at the bid (sellers hitting, bid absorbs -> resting -1), +1 = bought at the ask (resting +1)
inline bool printAbsorbed(const Candle& c, int printSide, long long key, bool obPresent, const std::vector<ObEv>& ev, double tick) {
    if (!obPresent) return false;
    const bool top = printSide > 0; long long lo = 0, hi = 0;
    if (!wickZone(c, top, lo, hi) || key < lo || key > hi) return false;
    return absorbedAt(ev, tick, printSide > 0 ? 1 : -1, key) != nullptr;
}
}  // namespace absl
#endif
