// TurnLogic.h 1.0.0 (2026-11-22) -- the logic of lsTurn "Turn v1.0" (Turn.cpp). SDK-free, unit tested (tests_turn).
// Rassul 2026-11-22 CT: "Put the red bars below the green bars"; "sanity check and get independent sanity check , code and design review ,
// no memory leaks and give me a final mockup before I approve to build"; "Let's rename the indicator to Turn since it describes the process
// of a turn"; "build". Spec: SPEC-TURN-1.0.md; mockup turn-final.html; reference calculation turnref.py (whole-second mode); the golden
// candles tests_turn/golden_turn_v1.json. Upgrade of ErPaneLogic.h 0.2.0 (lsEffortRewardPane): Effort and the Answer are that plugin's.
//
//   Written for the HIGH side (a key high is being swept: buyers push, sellers answer); the LOW side is the exact mirror.
//   Candle = [bar end - spb, bar end). Prints in feed order, whole seconds. extreme print = the FIRST print at the candle's high (implied
//   prints count), es = its second.
//   1 STOP RUN   buy-aggressor lots at price >= the level from order IDs issued 60+ s earlier that are not a known resting order,
//                / STOP_TYP[market].                                       No < 0.5x, Light < 1x, Moderate < 3x, Heavy 3x+
//   2 EFFORT     (offer-traded lots per price level in the top 25% of the candle) / (per level in the rest), IRT volume at price.
//                Under 4 levels or nothing in the rest = none.             No < 0.5x, Light < 1x, Moderate < 1.5x, Heavy 1.5x+
//   3 EXHAUSTION = lack of demand. B = buys up to the extreme print, dur = max(es - candle start, 1):
//                late  = (buys in the last 10 s before es / min(dur, 10)) / (B / dur)
//                decay = buys per new-high print in the second half of the climb / in the first half (4+ new highs, else missing)
//                after = (buys after the extreme print / max(bar end - es, 1)) / (B / dur)
//                dem = mean of the ranks of the three among the market's past sweeps (share of past values <= x; nightly tables).
//                No (demand still strong) dem > Q2, Moderate, Heavy dem < Q1. Bar = (1 - dem) / 0.5.
//   4 ANSWER     marks out of 3 after the extreme print: FAST a counter order ID issued < 2 s before through 3+ prices within 15 s;
//                HARD net counter delta in 15 s / the half hour's normal >= HARD_CUT; BIG the biggest counter order in 30 s / the half
//                hour's average order > BIG_CUT. Windows stop at the candle close. A missing normal = the Answer is not available.
//   VERDICT (lsRevRead's Sw candle only): UNCLEAR no exhaustion value, or the extreme in the first 20 s or the last 30 s of the candle;
//                STAND ASIDE dem > Q2; BEST ODDS ? exhaustion and marks >= 2; WAIT otherwise.
//   OTHER CANDLES AT A NAMED LEVEL (the highest named high the candle reached from below, the lowest named low it reached from above):
//                Stop Run and Effort of that side only.
//   Decided once (after the candle close), recorded, never recomputed (no repaint).
//   Memory: fills kept for the FORMING candle only (FILL_CAP), summed per price at its close; resting orders tracked within NEAR_TICKS of
//   the last trade (SHOWN_CAP); the order-ID clock is a ring of CLOCK_CAP points; SUMS_KEEP closed candles kept. No per-tick history.
#pragma once
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace turn {

static const char* const VERSION = "1.0.0";
static const size_t FILL_CAP = 150000;      // fills of one candle (more = the candle is not decided live)
static const size_t SHOWN_CAP = 50000;      // resting orders tracked (is the aggressor a known resting order?)
static const long long NEAR_TICKS = 64;     // only orders within this many ticks of the last trade are tracked
static const size_t CLOCK_CAP = 4096;       // order-ID clock points (one per second the newest ID moved)
static const size_t SUMS_KEEP = 12;         // closed candles kept for a Sw that is marked up to 3 candles later
static const size_t CELL_CAP = 4096;        // prices per candle summary
static const size_t WY_MAX_BYTES = 512 * 1024, PARAMS_MAX_BYTES = 256 * 1024, REC_MAX_BYTES = 256 * 1024;
static const size_t SW_KEEP = 400, LEVEL_KEEP = 64, REC_KEEP = 300, CAND_KEEP = 1500, HIST_KEEP = 400, RANK_KEEP = 600, RANK_MIN = 5;
static const int FRESH_S = 2, OLD_S = 60, WIN_F = 15, WIN_B = 30, LATE_S = 30, EARLY_S = 20, LAST_S = 10, MIN_PRINTS = 5;
static const double Q1_DEF = 0.4562289562289562, Q2_DEF = 0.5931537598204265;   // the thirds of dem over the 110 past sweeps (in-sample)
static const double BAR_CAP = 3.0;          // a bar is full height at 3.0x (the Answer at 3 of 3)

// ---------------------------------------------------------------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------------------------------------------------------------
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> v; size_t a = 0;
    for (;;) { const size_t b = s.find(d, a); v.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    return v;
}
inline bool toLL(const std::string& s, long long& v) {
    if (s.empty() || s.size() > 19) return false;
    char* e = nullptr; errno = 0; const long long x = std::strtoll(s.c_str(), &e, 10);
    if (!e || *e || errno) return false;
    v = x; return true;
}
inline bool toD(const std::string& s, double& v) {
    if (s.empty() || s.size() > 32) return false;
    char* e = nullptr; const double x = std::strtod(s.c_str(), &e);
    if (!e || *e || !std::isfinite(x)) return false;
    v = x; return true;
}
inline std::vector<std::string> lines(const std::string& txt, bool dropUnterminatedLast) {
    std::vector<std::string> v; size_t a = 0;
    while (a < txt.size()) {
        size_t b = txt.find('\n', a);
        if (b == std::string::npos) { if (dropUnterminatedLast) break; b = txt.size(); }
        std::string ln = txt.substr(a, b - a); if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (!ln.empty()) v.push_back(ln);
        a = b + 1;
    }
    return v;
}
inline std::string ratioText(double x) {            // "1.7x"; 99x cap
    if (!std::isfinite(x) || x < 0) return std::string();
    char b[24]; if (x >= 99.95) return "99x+";
    std::snprintf(b, sizeof b, "%.1fx", x); return b;
}
inline const char* word4(int k) { static const char* const W[] = { "No", "Light", "Moderate", "Heavy" }; return W[k < 0 ? 0 : k > 3 ? 3 : k]; }
inline int stopClass(double x) { return (!std::isfinite(x) || x < 0.5) ? 0 : x < 1.0 ? 1 : x < 3.0 ? 2 : 3; }
inline int effClass(double x) { return (!std::isfinite(x) || x < 0.5) ? 0 : x < 1.0 ? 1 : x < 1.5 ? 2 : 3; }
inline int exhClassOf(double dem, double q1, double q2) { return (!std::isfinite(dem) || dem > q2) ? 0 : dem < q1 ? 3 : 2; }   // No / Moderate / Heavy
inline double exhBar(double dem) { return (1.0 - dem) / 0.5; }
inline std::string levelName(const std::string& s) { // LevelMarks short names -> his names
    return s == "LHI" ? "LonHI" : s == "LLO" ? "LonLO" : s == "AHI" ? "AsiaHI" : s == "ALO" ? "AsiaLO" : s;
}
inline bool nameOk(const std::string& s) {
    if (s.empty() || s.size() > 32) return false;
    for (char c : s) if (!(std::isalnum((unsigned char)c) || c == '/' || c == '_' || c == '-' || c == '+')) return false;
    return true;
}
inline long long keyOf(long long sec, int spb) { return (sec / spb + 1) * (long long)spb; }   // IRT bar time = the bar END
// -1 = a high (buyers push through it), +1 = a low, 0 = not a high / low name. As lsSR (SRLogic.h sideOfName).
inline int sideOfName(const std::string& n) {
    const size_t z = n.size();
    if (z >= 2 && n.compare(z - 2, 2, "HI") == 0) return -1;
    if (z >= 2 && n.compare(z - 2, 2, "LO") == 0) return 1;
    if (z >= 1 && n[z - 1] == 'H') return -1;
    if (z >= 1 && n[z - 1] == 'L') return 1;
    return 0;
}
inline std::string clockText(int hour, int minute) {   // 12-hour clock: "9:03 AM"
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return std::string();
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", hour % 12 == 0 ? 12 : hour % 12, minute, hour < 12 ? "AM" : "PM"); return b;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// EFFORT (IRT volume at price), as lsEffortRewardPane 0.2.0
// ---------------------------------------------------------------------------------------------------------------------------------
struct Row { long long tick = 0, buy = 0, sell = 0; };       // buy = traded at the offer, sell = traded at the bid
struct Bars { bool topOk = false, botOk = false; double top = 0, bot = 0; long long levels = 0, zone = 0; };
inline long long zoneOf(long long L) { const long long z = (L + 2) / 4; return z < 1 ? 1 : z; }   // round(0.25 L), half up
inline Bars barRatios(const std::vector<Row>& rows, long long lo, long long hi) {
    Bars b; if (hi < lo) return b;
    const long long L = hi - lo + 1; if (L < 4 || L > 200000) return b;
    const long long z = zoneOf(L); b.levels = L; b.zone = z;
    long long bt = 0, br = 0, st = 0, sr = 0;
    for (const Row& r : rows) {
        if (r.tick < lo || r.tick > hi || r.buy < 0 || r.sell < 0) continue;
        if (r.tick > hi - z) bt += r.buy; else br += r.buy;
        if (r.tick < lo + z) st += r.sell; else sr += r.sell;
    }
    const double nz = (double)z, nr = (double)(L - z);
    if (br > 0) { b.top = ((double)bt / nz) / ((double)br / nr); b.topOk = std::isfinite(b.top); }
    if (sr > 0) { b.bot = ((double)st / nz) / ((double)sr / nr); b.botOk = std::isfinite(b.bot); }
    return b;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// SWEEPS AND NAMED LEVELS (lsRevRead's wy file)
// ---------------------------------------------------------------------------------------------------------------------------------
struct Sw { long long stamp = 0; int dir = 0; std::string lv; long long lvt = 0; };
struct Level { long long tick = 0; std::string name; bool hi = false, lo = false; };
// header WY|1|<MKT>|<spb>|<ver>; keeps the newest SW_KEEP "stamp|Sw|dir|level|px|strength" lines (sorted by stamp) and the named levels
// "L|ticks|name" whose name says high or low (ONH, LonHI, PDL, ONL/PHL ...); false + why on a foreign / garbled header
inline bool parseWy(const std::string& txt, const std::string& mkt, int spb, std::vector<Sw>& out, std::vector<Level>& lv, std::string& why) {
    out.clear(); lv.clear(); why.clear();
    if (txt.size() > WY_MAX_BYTES) { why = "sweep file too big"; return false; }
    const std::vector<std::string> L = lines(txt, true);
    if (L.empty()) { why = "sweep file empty"; return false; }
    const std::vector<std::string> h = split(L[0], '|');
    if (h.size() < 4 || h[0] != "WY" || h[1] != "1") { why = "sweep file header"; return false; }
    if (h[2] != mkt) { why = "sweep file is " + h[2].substr(0, 8); return false; }
    long long s = 0; if (!toLL(h[3], s) || s != spb) { why = "sweep file bar size"; return false; }
    std::map<long long, Sw> m; std::map<long long, Level> lm;
    for (size_t i = 1; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], '|');
        if (c.size() >= 3 && c[0] == "L") {
            long long t = 0; if (!toLL(c[1], t) || t <= 0 || !nameOk(c[2])) continue;
            bool hi = false, lo = false;
            for (const std::string& part : split(c[2], '/')) { const int sd = sideOfName(levelName(part)); if (sd < 0) hi = true; if (sd > 0) lo = true; }
            if (!hi && !lo) continue;
            Level& x = lm[t];
            if (x.tick == 0) { x.tick = t; x.name = c[2]; } else if (x.name.size() + 1 + c[2].size() <= 32) x.name += "/" + c[2];
            x.hi = x.hi || hi; x.lo = x.lo || lo;
            continue;
        }
        if (c.size() < 5 || c[1] != "Sw") continue;
        Sw w; long long d = 0;
        if (!toLL(c[0], w.stamp) || w.stamp <= 0 || !toLL(c[2], d) || (d != 1 && d != -1) || !toLL(c[4], w.lvt) || w.lvt <= 0 || !nameOk(c[3])) continue;
        w.dir = (int)d; w.lv = levelName(c[3]); m[w.stamp] = w;
    }
    for (std::map<long long, Sw>::const_iterator it = m.begin(); it != m.end(); ++it) out.push_back(it->second);
    if (out.size() > SW_KEEP) out.erase(out.begin(), out.end() - (std::ptrdiff_t)SW_KEEP);
    for (std::map<long long, Level>::const_iterator it = lm.begin(); it != lm.end() && lv.size() < LEVEL_KEEP; ++it) lv.push_back(it->second);
    return true;
}
// the named levels a candle pushed into: the highest high it reached from below (open < level <= high) and the lowest low it reached
// from above (open > level >= low); 0 = none. The level nearest the candle's extreme, as a sweep's level is.
inline void levelsIn(const std::vector<Level>& lv, long long lo, long long hi, long long open, long long& hiL, long long& loL) {
    hiL = 0; loL = 0;
    for (const Level& x : lv) {
        if (x.tick < lo || x.tick > hi) continue;
        if (x.hi && open < x.tick && x.tick > hiL) hiL = x.tick;
        if (x.lo && open > x.tick && (loL == 0 || x.tick < loL)) loL = x.tick;
    }
}

// ---------------------------------------------------------------------------------------------------------------------------------
// THE READ of one sweep candle (decided once; the same fields in the record file and in the nightly's SW rows)
// ---------------------------------------------------------------------------------------------------------------------------------
enum { V_NONE = -1, V_UNCLEAR = 0, V_ASIDE = 1, V_WAIT = 2, V_BEST = 3 };
enum { WHY_NONE = 0, WHY_EARLY = 1, WHY_LATE = 2, WHY_NOEXH = 3 };
struct Read {
    long long stamp = 0; int dir = 0; std::string lv; long long lvt = 0;
    long long stopLots = -1, stopX1000 = -1, eff1000 = -1;                       // -1 = not known
    long long late1e4 = -1, decay1e4 = -1, after1e4 = -1, dem1e4 = -1; int exh = -1;   // exh 0 No / 2 Moderate / 3 Heavy
    int F = -1, H = -1, B = -1, marks = -1, ansOk = 0; long long hard100 = 0, big100 = -1;
    long long extInto = -1, extLeft = -1; int verdict = V_NONE, why = WHY_NONE, odds = -1;
    char src = '-';                                                              // L live order feed, N nightly (the store), - none
};
static const size_t READ_FIELDS = 25;
inline std::string readText(const Read& r, char sep) {
    char b[640];
    std::snprintf(b, sizeof b, "%lld%c%d%c%s%c%lld%c%lld%c%lld%c%lld%c%lld%c%lld%c%lld%c%lld%c%d%c%d%c%d%c%d%c%d%c%d%c%lld%c%lld%c%lld%c%lld%c%d%c%d%c%d%c%c",
                  r.stamp, sep, r.dir, sep, r.lv.c_str(), sep, r.lvt, sep, r.stopLots, sep, r.stopX1000, sep, r.eff1000, sep, r.late1e4, sep, r.decay1e4, sep, r.after1e4, sep,
                  r.dem1e4, sep, r.exh, sep, r.F, sep, r.H, sep, r.B, sep, r.marks, sep, r.ansOk, sep, r.hard100, sep, r.big100, sep, r.extInto, sep, r.extLeft, sep,
                  r.verdict, sep, r.why, sep, r.odds, sep, r.src);
    return b;
}
// the READ_FIELDS fields from c[at]; false on any field out of range (the line is skipped, never half-read)
inline bool readParse(const std::vector<std::string>& c, size_t at, Read& out) {
    if (c.size() != at + READ_FIELDS) return false;
    long long v[READ_FIELDS]; for (size_t k = 0; k < READ_FIELDS; ++k) v[k] = 0;
    for (size_t k = 0; k < READ_FIELDS; ++k) { if (k == 2 || k == 24) continue; if (!toLL(c[at + k], v[k])) return false; }
    const std::string& nm = c[at + 2]; const std::string& sc = c[at + 24];
    if (v[0] <= 0 || (v[1] != 1 && v[1] != -1) || !nameOk(nm) || v[3] <= 0) return false;
    if (sc.size() != 1 || (sc[0] != 'L' && sc[0] != 'N' && sc[0] != '-')) return false;
    for (size_t k = 4; k <= 9; ++k) if (v[k] < -1 || v[k] > 1000000000000LL) return false;      // lots and ratios
    if (v[10] < -1 || v[10] > 10000) return false;                                              // dem x 10000
    if (v[11] != -1 && v[11] != 0 && v[11] != 2 && v[11] != 3) return false;                    // exhaustion class
    for (size_t k = 12; k <= 14; ++k) if (v[k] < -1 || v[k] > 1) return false;                  // F H B
    if (v[15] < -1 || v[15] > 3 || (v[16] != 0 && v[16] != 1)) return false;                    // marks, the answer available
    if (v[17] < -1000000000000LL || v[17] > 1000000000000LL || v[18] < -1 || v[18] > 1000000000000LL) return false;
    if (v[19] < -1 || v[19] > 86400 || v[20] < -1 || v[20] > 86400) return false;
    if (v[21] < -1 || v[21] > 3 || v[22] < 0 || v[22] > 3 || v[23] < -1 || v[23] > 100) return false;
    Read r; r.stamp = v[0]; r.dir = (int)v[1]; r.lv = nm; r.lvt = v[3]; r.stopLots = v[4]; r.stopX1000 = v[5]; r.eff1000 = v[6];
    r.late1e4 = v[7]; r.decay1e4 = v[8]; r.after1e4 = v[9]; r.dem1e4 = v[10]; r.exh = (int)v[11]; r.F = (int)v[12]; r.H = (int)v[13]; r.B = (int)v[14];
    r.marks = (int)v[15]; r.ansOk = (int)v[16]; r.hard100 = v[17]; r.big100 = v[18]; r.extInto = v[19]; r.extLeft = v[20];
    r.verdict = (int)v[21]; r.why = (int)v[22]; r.odds = (int)v[23]; r.src = sc[0];
    out = r; return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// PARAMS (lsFlexLevels\LRA-TurnPane-<MKT>.csv, written by the nightly lra.turn_pane; defaults when missing / garbled)
// ---------------------------------------------------------------------------------------------------------------------------------
struct Params {
    std::string mkt; bool fromFile = false; std::string why = "defaults";
    double stopTyp = 0, hardCut = 0, bigCut = 0, q1 = Q1_DEF, q2 = Q2_DEF;
    int odds[4] = { -1, 26, 52, -1 };             // by verdict: UNCLEAR, STAND ASIDE, WAIT, BEST ODDS; -1 = no number (under 20 cases)
    bool bestProven = false;                      // the nightly's judge: BEST ODDS confirmed on 20+ new cases a side (then no "?")
    double n15[48], avgOrd[48];
    std::vector<double> rkLate, rkDecay, rkAfter; // the market's past sweep values, ascending (the rank tables)
    std::vector<Read> hist;                       // sweeps the nightly read from the order store (src N), by stamp
    Params() { for (int i = 0; i < 48; ++i) { n15[i] = 0; avgOrd[i] = 0; } }
};
// built-in defaults (research 2026-11-22: 110 sweeps, 7 markets). No rank tables: without the nightly file every read is UNCLEAR.
inline Params defaults(const std::string& mkt) {
    Params p; p.mkt = mkt;
    struct D { const char* m; double stop, hard, big; };
    static const D T[] = { { "ES", 21.5, 3.010, 12.094 }, { "NQ", 24, 3.838, 13.374 }, { "CL", 11.5, 3.154, 4.283 }, { "GC", 8, 3.059, 3.309 },
                           { "HG", 6, 4.911, 2.316 }, { "NG", 5, 4.276, 4.967 }, { "EU", 6, 3.439, 6.999 } };
    for (const D& d : T) if (mkt == d.m) { p.stopTyp = d.stop; p.hardCut = d.hard; p.bigCut = d.big; }
    return p;
}
inline bool fillVals(const std::vector<std::string>& c, double* v, size_t n) {
    if (c.size() != n + 1) return false;
    for (size_t i = 0; i < n; ++i) { double x = 0; if (!toD(c[i + 1], x) || x < 0 || x > 1e9 || (x > 0 && x < 0.01)) return false; v[i] = x; }   // no tiny divisors
    return true;
}
inline bool fillRank(const std::vector<std::string>& c, std::vector<double>& v) {
    v.clear(); if (c.size() > RANK_KEEP + 1) return false;
    for (size_t i = 1; i < c.size(); ++i) { double x = 0; if (!toD(c[i], x) || x < 0 || x > 1e12) return false; v.push_back(x); }
    std::sort(v.begin(), v.end()); return true;
}
// VERSION,1 first, MARKET must match; any bad line = defaults for everything + why (never a half-read file)
inline Params parseParams(const std::string& txt, const std::string& mkt) {
    Params d = defaults(mkt);
    if (txt.empty()) { d.why = "no params file (defaults)"; return d; }
    if (txt.size() > PARAMS_MAX_BYTES) { d.why = "params file too big (defaults)"; return d; }
    const std::vector<std::string> L = lines(txt, true);
    if (L.size() < 2 || L[0] != "VERSION,1") { d.why = "params VERSION (defaults)"; return d; }
    if (L[1] != "MARKET," + mkt) { d.why = "params market (defaults)"; return d; }
    Params p = d; std::map<long long, Read> hs;
    for (size_t i = 2; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], ',');
        double x = 0; bool ok = true;
        if (c[0] == "STOP_TYP") ok = c.size() == 2 && toD(c[1], x) && x >= 0.01 && x < 1e9 && (p.stopTyp = x, true);
        else if (c[0] == "HARD_CUT") ok = c.size() == 2 && toD(c[1], x) && x > 0 && x < 1e6 && (p.hardCut = x, true);
        else if (c[0] == "BIG_CUT") ok = c.size() == 2 && toD(c[1], x) && x > 0 && x < 1e6 && (p.bigCut = x, true);
        else if (c[0] == "EXH_CUT") { double a = 0, b = 0; ok = c.size() == 3 && toD(c[1], a) && toD(c[2], b) && a > 0 && a < b && b < 1; if (ok) { p.q1 = a; p.q2 = b; } }
        else if (c[0] == "ODDS") { long long o[4] = { 0, 0, 0, 0 }; ok = c.size() == 5;
            for (int k = 0; ok && k < 4; ++k) ok = toLL(c[(size_t)k + 1], o[k]) && o[k] >= -1 && o[k] <= 100;
            if (ok) for (int k = 0; k < 4; ++k) p.odds[k] = (int)o[k]; }
        else if (c[0] == "BEST_PROVEN") { long long b = 0; ok = c.size() == 2 && toLL(c[1], b) && (b == 0 || b == 1); if (ok) p.bestProven = b == 1; }
        else if (c[0] == "N15") ok = fillVals(c, p.n15, 48);
        else if (c[0] == "AVG_ORDER") ok = fillVals(c, p.avgOrd, 48);
        else if (c[0] == "RANK_LATE") ok = fillRank(c, p.rkLate);
        else if (c[0] == "RANK_DECAY") ok = fillRank(c, p.rkDecay);
        else if (c[0] == "RANK_AFTER") ok = fillRank(c, p.rkAfter);
        else if (c[0] == "SW") { Read r; ok = readParse(c, 1, r) && r.src == 'N'; if (ok) hs[r.stamp] = r; }
        else continue;                                                    // END and unknown keys (a newer nightly) are ignored
        if (!ok) { d.why = "params line " + std::to_string(i + 1) + " garbled (defaults)"; return d; }
    }
    for (std::map<long long, Read>::const_iterator it = hs.begin(); it != hs.end(); ++it) p.hist.push_back(it->second);
    if (p.hist.size() > HIST_KEEP) p.hist.erase(p.hist.begin(), p.hist.end() - (std::ptrdiff_t)HIST_KEEP);
    p.fromFile = true; p.why = "ok"; return p;
}
// share of the past values <= x; -1 = no table yet (under RANK_MIN values) or no value
inline double rankOf(const std::vector<double>& sortedPool, double x) {
    if (sortedPool.size() < RANK_MIN || !std::isfinite(x)) return -1;
    const size_t k = (size_t)(std::upper_bound(sortedPool.begin(), sortedPool.end(), x) - sortedPool.begin());
    return (double)k / (double)sortedPool.size();
}

// ---------------------------------------------------------------------------------------------------------------------------------
// THE LIVE ORDER FEED -> per-candle sums
// ---------------------------------------------------------------------------------------------------------------------------------
// order IDs: IRT's are increasing ints; unwrapped as 32-bit against the newest so a wrap never breaks the clock
struct IdClock {
    struct P { long long sec, id; };
    std::deque<P> pts; long long maxId = 0, since = -1; bool have = false; int jumpRun = 0; long long jumps = 0;
    static const long long JUMP = 1LL << 24;                              // a bigger jump needs 8 IDs in a row to be believed
    long long unwrap(long long raw) const {
        const uint32_t u = (uint32_t)(raw & 0xFFFFFFFFLL);
        if (!have) return (long long)u;
        const int32_t d = (int32_t)(u - (uint32_t)(maxId & 0xFFFFFFFFLL));
        return maxId + (long long)d;
    }
    void note(long long id, long long sec) {
        if (have && id <= maxId) return;
        if (have && id > maxId + JUMP) { ++jumps; if (++jumpRun < 8) return; pts.clear(); since = -1; }   // a garbage ID never freezes the clock
        jumpRun = 0;
        maxId = id; have = true;
        if (!pts.empty() && pts.back().sec >= sec) pts.back().id = id;
        else { P p; p.sec = sec; p.id = id; pts.push_back(p); if (since < 0) since = sec; if (pts.size() > CLOCK_CAP) pts.pop_front(); }
    }
    // seconds since the ID was first reached (a lower bound for IDs older than the clock); -1 = no clock yet
    long long age(long long id, long long sec) const {
        if (pts.empty()) return -1;
        if (id > maxId) return 0;
        size_t lo = 0, hi = pts.size();                                   // the first point whose newest ID >= id
        while (lo < hi) { const size_t m = (lo + hi) / 2; if (pts[m].id >= id) hi = m; else lo = m + 1; }
        if (lo >= pts.size()) return 0;
        const long long a = sec - pts[lo].sec; return a < 0 ? 0 : a;
    }
    void clear() { pts.clear(); maxId = 0; since = -1; have = false; jumpRun = 0; }
};
struct Fill { long long sec = 0, px = 0, size = 0, aid = 0; signed char ag = 0; bool fresh = false, stop = false; };
struct Cell { long long buy = 0, sell = 0, stopB = 0, stopS = 0; };      // aggressor lots at the price; stop = from old orders never seen resting
struct Turn { bool ok = false; long long extPx = 0, extSec = 0, hard = 0, big = 0; bool fresh = false; size_t ie = 0; };
struct Exh { bool ok = false, hasDecay = false; double late = 0, decay = 0, after = 0; long long push = 0; };
struct Sum {
    long long key = 0, start = 0; bool complete = false, overflow = false, stopOk = false, thin = true;
    std::map<long long, Cell> cells; long long hiPx = 0, loPx = 0;
    Turn hi, lo; Exh exHi, exLo;
    long long fills = 0, implied = 0, lots = 0;                           // lots = every filled lot read (implied too): the coverage check vs the chart's bar volume
};
struct Counters { long long events = 0, fills = 0, implied = 0, late = 0, bad = 0, overflows = 0, shownResets = 0, candles = 0, complete = 0, noAggr = 0, future = 0, stopFills = 0; };

class Live {
public:
    int spb = 0;
    Counters n;
    std::deque<Sum> sums;                                                 // closed candles, oldest first
    // a fresh start (new chart / symbol / reconnect): everything open is dropped and the next candle cannot be complete
    void reset(int barSecs) {
        spb = barSecs; fills_.clear(); std::vector<Fill>().swap(fills_); ords_.clear(); clock_.clear();
        curKey_ = 0; liveSince_ = -1; haveRef_ = false; ref_ = 0; overflow_ = false; sums.clear(); lastClosed_ = 0; limitKey_ = 0; implied_ = 0;
    }
    // the newest candle the chart allows: an event dated past it is counted and never opens a candle
    void setLimit(long long chartFormingKey) { limitKey_ = chartFormingKey > 0 ? chartFormingKey + spb : 0; }
    void gap() {                                                           // the stream was interrupted: open candle and the clock are not whole
        fills_.clear(); curKey_ = 0; liveSince_ = -1; overflow_ = false; clock_.clear(); ords_.clear(); implied_ = 0;
    }
    // one order event: sec = its RTDATE second, px in ticks, action 0 NEW 1 MOD 2 DEL 3 FILL, buySell 1 buy / 0 sell / 2 unknown
    void onEvent(long long sec, int action, int buySell, long long px, long long size, long long oidRaw, long long aidRaw) {
        ++n.events;
        if (spb <= 0 || sec <= 0 || px <= 0 || size < 0 || action < 0 || action > 3) { ++n.bad; return; }
        const long long key = keyOf(sec, spb);
        if ((curKey_ != 0 && key < curKey_) || key <= lastClosed_) {     // an event of a candle already closed: that candle is not whole
            ++n.late; for (Sum& x : sums) if (x.key == key) { x.complete = false; x.stopOk = false; }
            return;
        }
        if (limitKey_ > 0 && key > limitKey_) { ++n.future; return; }
        if (liveSince_ < 0 && action == 3) liveSince_ = sec;              // the live clock starts at the first FILL (a book snapshot may carry old times)
        if (key > curKey_) { if (curKey_ != 0) close(curKey_); open(key); }
        const long long oid = oidRaw ? clock_.unwrap(oidRaw) : 0;
        switch (action) {
        case 0: case 1: {
            if (oid) { if (action == 0) clock_.note(oid, sec); shown(oid, px); }
            break; }
        case 2: if (oid) ords_.erase(oid); break;
        case 3: {
            ++n.fills; ref_ = px; haveRef_ = true;
            const long long aid = aidRaw ? clock_.unwrap(aidRaw) : 0;
            if (oid && oid == aid) { ++n.bad; break; }                    // an order filling itself is not a trade between two sides
            Fill f; f.sec = sec; f.px = px; f.size = size;
            if (buySell != 0 && buySell != 1) { ++n.implied; ++implied_; }   // implied (no aggressor): kept for the extreme and the print order only
            else {
                f.ag = (signed char)(buySell == 1 ? 1 : -1); f.aid = aid;
                if (aid) {
                    const long long a = clock_.age(aid, sec);
                    f.fresh = a >= 0 && a < FRESH_S;
                    f.stop = a >= OLD_S && ords_.find(aid) == ords_.end();   // issued 60+ s ago and never seen resting: a stop / hidden order
                    if (f.stop) ++n.stopFills;
                    clock_.note(aid, sec);
                } else ++n.noAggr;
            }
            if (fills_.size() < FILL_CAP) fills_.push_back(f); else if (!overflow_) { overflow_ = true; ++n.overflows; }
            break; }
        default: break;
        }
    }
    // the chart says candles up to `lastClosed` are closed: a quiet feed still closes its candle
    void closeThrough(long long lastClosed) { if (curKey_ != 0 && curKey_ <= lastClosed) { close(curKey_); curKey_ = 0; } }
    const Sum* find(long long key) const { for (const Sum& s : sums) if (s.key == key) return &s; return nullptr; }
    size_t openFills() const { return fills_.size(); }
    size_t orders() const { return ords_.size(); }
    size_t clockPoints() const { return clock_.pts.size(); }
    long long clockSince() const { return clock_.since; }
    long long forming() const { return curKey_; }
private:
    std::vector<Fill> fills_; std::unordered_map<long long, long long> ords_; IdClock clock_;   // ords_: resting order ID -> its price
    long long curKey_ = 0, liveSince_ = -1, ref_ = 0, implied_ = 0, lastClosed_ = 0, limitKey_ = 0; bool haveRef_ = false, overflow_ = false;
    void shown(long long oid, long long px) {
        std::unordered_map<long long, long long>::iterator it = ords_.find(oid);
        if (it == ords_.end()) {
            if (haveRef_ && std::llabs(px - ref_) > NEAR_TICKS) return;
            if (ords_.size() >= SHOWN_CAP) prune();
            if (ords_.size() >= SHOWN_CAP) return;
            ords_[oid] = px; return;
        }
        it->second = px;
        if (haveRef_ && std::llabs(px - ref_) > NEAR_TICKS) ords_.erase(it);
    }
    void prune() {
        for (std::unordered_map<long long, long long>::iterator it = ords_.begin(); it != ords_.end();) { if (!haveRef_ || std::llabs(it->second - ref_) > NEAR_TICKS) it = ords_.erase(it); else ++it; }
        if (ords_.size() >= SHOWN_CAP * 9 / 10) { ords_.clear(); ++n.shownResets; }
    }
    void open(long long key) { curKey_ = key; fills_.clear(); overflow_ = false; implied_ = 0; }
    void close(long long key) {
        sums.push_back(Sum()); Sum& s = sums.back();
        s.key = key; s.start = key - spb; s.overflow = overflow_; s.fills = (long long)fills_.size(); s.implied = implied_;
        for (const Fill& f : fills_) s.lots += f.size;
        s.complete = !overflow_ && liveSince_ >= 0 && liveSince_ < s.start;
        s.stopOk = s.complete && clock_.since >= 0 && clock_.since + OLD_S <= s.start;   // the ID clock was 60+ s old when the candle began
        if (key > lastClosed_) lastClosed_ = key;
        ++n.candles; if (s.complete) ++n.complete;
        summarize(s);
        while (sums.size() > SUMS_KEEP) sums.pop_front();
        fills_.clear(); if (fills_.capacity() > 65536) std::vector<Fill>().swap(fills_);
        if (ords_.size() > SHOWN_CAP / 2) prune();
        overflow_ = false; implied_ = 0;
    }
    void summarize(Sum& s) const {
        const std::vector<Fill>& F = fills_; const size_t nf = F.size(); if (nf == 0) return;
        long long aggr = 0; s.hiPx = s.loPx = F[0].px;
        for (size_t k = 0; k < nf; ++k) {
            const Fill& f = F[k];
            if (f.px > s.hiPx) s.hiPx = f.px;
            if (f.px < s.loPx) s.loPx = f.px;
            if (f.ag == 0) continue;                                      // implied prints: no side
            ++aggr;
            std::map<long long, Cell>::iterator ci = s.cells.find(f.px);
            if (ci == s.cells.end()) {
                if (s.cells.size() >= CELL_CAP) { s.overflow = true; s.complete = false; s.stopOk = false; continue; }
                ci = s.cells.insert(std::make_pair(f.px, Cell())).first;
            }
            Cell& c = ci->second;
            if (f.ag > 0) { c.buy += f.size; if (f.stop) c.stopB += f.size; }
            else { c.sell += f.size; if (f.stop) c.stopS += f.size; }
        }
        s.thin = nf < (size_t)MIN_PRINTS || aggr < MIN_PRINTS;            // under 5 prints: no reading (as the reference)
        s.hi = turnOf(-1); s.lo = turnOf(1);
        if (!s.thin) { s.exHi = exhOf(s, -1, s.hi); s.exLo = exhOf(s, 1, s.lo); }
    }
    // the counter side's answer after the extreme. dir -1 = the high (counter = sellers), +1 = the low (counter = buyers)
    Turn turnOf(int dir) const {
        Turn t; const std::vector<Fill>& F = fills_; if (F.empty()) return t;
        size_t ie = 0; for (size_t k = 1; k < F.size(); ++k) if (dir < 0 ? F[k].px > F[ie].px : F[k].px < F[ie].px) ie = k;   // the FIRST fill at the extreme (implied prints too)
        t.ok = true; t.ie = ie; t.extPx = F[ie].px; t.extSec = F[ie].sec;
        const int ctr = dir < 0 ? -1 : 1;
        struct G { long long lots = 0, p[3] = { 0, 0, 0 }; int np = 0; bool fresh = false, first = true; };
        std::unordered_map<long long, G> g15, g30;
        for (size_t k = ie + 1; k < F.size(); ++k) {
            const Fill& f = F[k]; const long long dt = f.sec - t.extSec; if (dt > WIN_B) break;
            if (dt <= WIN_F) t.hard += (long long)ctr * f.ag * f.size;    // net counter delta, counter side positive
            if (f.ag != ctr || !f.aid) continue;
            { std::unordered_map<long long, G>::iterator gi = g30.find(f.aid);
              if (gi == g30.end() && g30.size() < 20000) gi = g30.insert(std::make_pair(f.aid, G())).first;
              if (gi != g30.end()) { gi->second.lots += f.size; if (gi->second.lots > t.big) t.big = gi->second.lots; } }
            if (dt <= WIN_F && (g15.count(f.aid) || g15.size() < 20000)) {
                G& a = g15[f.aid]; if (a.first) { a.fresh = f.fresh; a.first = false; }
                bool seen = false; for (int q = 0; q < a.np; ++q) if (a.p[q] == f.px) seen = true;
                if (!seen && a.np < 3) a.p[a.np++] = f.px;
                if (a.np >= 3 && a.fresh) t.fresh = true;
            }
        }
        return t;
    }
    // the pushing side's demand around the extreme (the high: buyers). Positions are print order; times whole seconds.
    Exh exhOf(const Sum& s, int dir, const Turn& t) const {
        Exh e; const std::vector<Fill>& F = fills_; if (!t.ok || F.empty()) return e;
        const int sg = dir < 0 ? 1 : -1;                                  // the pusher's aggressor sign; pushed price = sg * px
        const size_t ie = t.ie; const long long es = t.extSec;
        const double dur = (double)std::max(es - s.start, 1LL);
        long long B = 0, last = 0, aft = 0;
        for (size_t k = 0; k < F.size(); ++k) {
            const Fill& f = F[k]; if (f.ag != sg) continue;
            if (k <= ie) { B += f.size; if (f.sec >= es - LAST_S) last += f.size; } else aft += f.size;
        }
        if (B <= 0) return e;
        e.ok = true; e.push = B;
        const double pace = (double)B / dur;
        e.late = ((double)last / std::min(dur, (double)LAST_S)) / pace;
        e.after = ((double)aft / (double)std::max(s.key - es, 1LL)) / pace;
        // new-high prints: positions where the running extreme moves (the first print counts)
        size_t nh = 0; long long mx = 0;
        for (size_t k = 0; k <= ie; ++k) { const long long p = sg * F[k].px; if (k == 0 || p > mx) { ++nh; mx = p; } }
        if (nh >= 4) {
            const size_t half = nh / 2; size_t seen = 0, kpos = 0; long long m2 = 0;   // kpos = the position of new-high number `half` (0-based)
            for (size_t k = 0; k <= ie; ++k) { const long long p = sg * F[k].px; if (k == 0 || p > m2) { if (seen == half) { kpos = k; break; } ++seen; m2 = p; } }
            long long a1 = 0, a2 = 0;
            for (size_t k = 0; k <= ie; ++k) { const Fill& f = F[k]; if (f.ag != sg) continue; if (k < kpos) a1 += f.size; else a2 += f.size; }
            if (a1 > 0) { e.decay = ((double)a2 / (double)(nh - half)) / ((double)a1 / (double)half); e.hasDecay = std::isfinite(e.decay); }
        }
        return e;
    }
};

// ---------------------------------------------------------------------------------------------------------------------------------
// DECIDING (once, recorded)
// ---------------------------------------------------------------------------------------------------------------------------------
// stop lots of a closed candle at a level: old never-resting buy lots at / above a high (sell lots at / below a low)
inline long long stopLots(const Sum& s, int dir, long long lvt) {
    long long v = 0;
    for (std::map<long long, Cell>::const_iterator it = s.cells.begin(); it != s.cells.end(); ++it) {
        if (dir < 0 && it->first >= lvt) v += it->second.stopB;
        if (dir > 0 && it->first <= lvt) v += it->second.stopS;
    }
    return v;
}
inline long long x1000(double x) { return (long long)std::llround(std::max(0.0, std::min(1e9, x)) * 1000.0); }
// the candle's own Effort from the order feed (aggressor lots per price): what the chart's volume at price shows, used when that is missing
inline Bars sumBars(const Sum& s) {
    std::vector<Row> rows; rows.reserve(s.cells.size());
    for (std::map<long long, Cell>::const_iterator it = s.cells.begin(); it != s.cells.end(); ++it) { Row r; r.tick = it->first; r.buy = it->second.buy; r.sell = it->second.sell; rows.push_back(r); }
    return barRatios(rows, s.loPx, s.hiPx);
}
struct Dem { bool ok = false; double dem = 0; int n = 0; };
inline Dem demOf(const Exh& e, const Params& P) {
    Dem d; if (!e.ok) return d;
    double sum = 0; int n = 0;
    const double r1 = rankOf(P.rkLate, e.late); if (r1 >= 0) { sum += r1; ++n; }
    if (e.hasDecay) { const double r2 = rankOf(P.rkDecay, e.decay); if (r2 >= 0) { sum += r2; ++n; } }
    const double r3 = rankOf(P.rkAfter, e.after); if (r3 >= 0) { sum += r3; ++n; }
    if (n == 0) return d;
    d.ok = true; d.n = n; d.dem = sum / n; return d;
}
inline int verdictOf(bool demOk, double dem, bool early, bool late, bool ansOk, int marks, double q2, int& why) {
    why = WHY_NONE;
    if (early) { why = WHY_EARLY; return V_UNCLEAR; }
    if (late) { why = WHY_LATE; return V_UNCLEAR; }
    if (!demOk || !std::isfinite(dem)) { why = WHY_NOEXH; return V_UNCLEAR; }
    if (dem > q2) return V_ASIDE;
    return (ansOk && marks >= 2) ? V_BEST : V_WAIT;
}
inline int oddsOf(const Params& P, int verdict) {
    if (verdict < 0 || verdict > 3) return -1;
    if (verdict == V_UNCLEAR) return -1;
    if (verdict == V_BEST && !P.bestProven) return -1;
    return P.odds[verdict];
}
// tod = the half hour (CT clock) of the extreme: hour * 2 + minute / 30. effOk / eff = the chart's Effort bar of the sweep side.
inline Read decideLive(const Sum& s, const Sw& w, const Params& P, int tod, bool effOk, double eff) {
    Read r; r.stamp = w.stamp; r.dir = w.dir; r.lv = w.lv; r.lvt = w.lvt;
    if (!s.complete || s.key != w.stamp) return r;
    const Turn& t = w.dir < 0 ? s.hi : s.lo; const Exh& e = w.dir < 0 ? s.exHi : s.exLo;
    if (!t.ok) return r;
    if (w.dir < 0 ? t.extPx <= w.lvt : t.extPx >= w.lvt) return r;     // this chart never traded through the level: another contract
    r.src = 'L';
    if (s.stopOk && P.stopTyp > 0) { r.stopLots = stopLots(s, w.dir, w.lvt); r.stopX1000 = x1000((double)r.stopLots / P.stopTyp); }
    if (!effOk) { const Bars b = sumBars(s); effOk = w.dir < 0 ? b.topOk : b.botOk; eff = w.dir < 0 ? b.top : b.bot; }
    if (effOk && std::isfinite(eff) && eff >= 0) r.eff1000 = x1000(eff);
    r.extInto = std::max(0LL, t.extSec - s.start); r.extLeft = std::max(0LL, s.key - t.extSec);
    const bool early = r.extInto < EARLY_S, late = r.extLeft < LATE_S;
    Dem d;
    if (!s.thin && e.ok) {
        r.late1e4 = (long long)std::llround(std::min(1e8, e.late) * 1e4); r.after1e4 = (long long)std::llround(std::min(1e8, e.after) * 1e4);
        if (e.hasDecay) r.decay1e4 = (long long)std::llround(std::min(1e8, e.decay) * 1e4);
        d = demOf(e, P);
        if (d.ok) { r.dem1e4 = (long long)std::llround(d.dem * 1e4); r.exh = exhClassOf(d.dem, P.q1, P.q2); }
    }
    r.F = t.fresh ? 1 : 0;
    if (tod >= 0 && tod < 48 && P.n15[tod] > 0 && P.hardCut > 0) { const double h = (double)t.hard / P.n15[tod]; r.H = h >= P.hardCut ? 1 : 0; r.hard100 = (long long)std::llround(std::max(-1e9, std::min(1e9, h)) * 100); }
    if (tod >= 0 && tod < 48 && P.avgOrd[tod] > 0 && P.bigCut > 0) { const double b = (double)t.big / P.avgOrd[tod]; r.B = b > P.bigCut ? 1 : 0; r.big100 = (long long)std::llround(std::min(1e9, b) * 100); }
    r.ansOk = (r.H >= 0 && r.B >= 0) ? 1 : 0;
    r.marks = r.F + (r.H > 0 ? 1 : 0) + (r.B > 0 ? 1 : 0);
    r.verdict = verdictOf(d.ok, d.dem, early, late, r.ansOk == 1, r.marks, P.q2, r.why);
    r.odds = oddsOf(P, r.verdict);
    return r;
}
// a candle that traded at a named level (not a sweep read): its Stop Run at that level, decided once and recorded
struct Cand { long long key = 0, hiL = 0, hiStop = -1, hiX1000 = -1, loL = 0, loStop = -1, loX1000 = -1; };
// s = the candle's sum when it is whole (complete, covered, stopOk), else null: the candle is recorded without a Stop Run
inline Cand candOf(long long key, const std::vector<Level>& lv, long long lo, long long hi, long long open, const Sum* s, const Params& P) {
    Cand c; c.key = key; levelsIn(lv, lo, hi, open, c.hiL, c.loL);
    if (s && s->key == key && s->complete && s->stopOk && P.stopTyp > 0) {
        if (c.hiL) { c.hiStop = stopLots(*s, -1, c.hiL); c.hiX1000 = x1000((double)c.hiStop / P.stopTyp); }
        if (c.loL) { c.loStop = stopLots(*s, 1, c.loL); c.loX1000 = x1000((double)c.loStop / P.stopTyp); }
    }
    return c;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// THE RECORD FILE (lsFlexLevels\TurnPane\turn-<MKT>-<spb>.txt): T = a sweep read, C = a candle at a named level
// ---------------------------------------------------------------------------------------------------------------------------------
struct Rec { std::map<long long, Read> reads; std::map<long long, Cand> cands; };
inline std::string recHeader(const std::string& mkt, int spb) { return "TRN|1|" + mkt + "|" + std::to_string(spb); }
inline std::string candLine(const Cand& c) {
    char b[256]; std::snprintf(b, sizeof b, "C|%lld|%lld|%lld|%lld|%lld|%lld|%lld", c.key, c.hiL, c.hiStop, c.hiX1000, c.loL, c.loStop, c.loX1000); return b;
}
inline bool parseCand(const std::vector<std::string>& c, Cand& out) {
    if (c.size() != 8 || c[0] != "C") return false;
    long long v[7]; for (int k = 0; k < 7; ++k) if (!toLL(c[(size_t)k + 1], v[k])) return false;
    if (v[0] <= 0 || v[1] < 0 || v[4] < 0 || (v[1] == 0 && v[4] == 0)) return false;
    if (v[2] < -1 || v[3] < -1 || v[5] < -1 || v[6] < -1) return false;
    Cand x; x.key = v[0]; x.hiL = v[1]; x.hiStop = v[2]; x.hiX1000 = v[3]; x.loL = v[4]; x.loStop = v[5]; x.loX1000 = v[6];
    out = x; return true;
}
inline void trimRec(Rec& r) {
    while (r.reads.size() > REC_KEEP) r.reads.erase(r.reads.begin());
    while (r.cands.size() > CAND_KEEP) r.cands.erase(r.cands.begin());
}
// false = not our file (foreign header / too big): the caller keeps what it has
inline bool parseRecFile(const std::string& txt, const std::string& mkt, int spb, Rec& out) {
    out = Rec(); if (txt.size() > REC_MAX_BYTES) return false;
    const std::vector<std::string> L = lines(txt, true);
    if (L.empty() || L[0] != recHeader(mkt, spb)) return false;
    for (size_t i = 1; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], '|');
        if (c[0] == "T") { Read r; if (readParse(c, 1, r) && r.src == 'L') out.reads[r.stamp] = r; }
        else if (c[0] == "C") { Cand x; if (parseCand(c, x)) out.cands[x.key] = x; }
    }
    trimRec(out); return true;
}
inline std::string recFileText(const Rec& r, const std::string& mkt, int spb) {
    std::string s = recHeader(mkt, spb) + "\n";
    size_t skip = r.reads.size() > REC_KEEP ? r.reads.size() - REC_KEEP : 0;
    for (std::map<long long, Read>::const_iterator it = r.reads.begin(); it != r.reads.end(); ++it) { if (skip) { --skip; continue; } s += "T|" + readText(it->second, '|') + "\n"; }
    skip = r.cands.size() > CAND_KEEP ? r.cands.size() - CAND_KEEP : 0;
    for (std::map<long long, Cand>::const_iterator it = r.cands.begin(); it != r.cands.end(); ++it) { if (skip) { --skip; continue; } s += candLine(it->second) + "\n"; }
    return s;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// WHAT IS DRAWN: the bars of a candle and the read strip
// ---------------------------------------------------------------------------------------------------------------------------------
struct Bar1 { bool show = false, on = false; double v = 0; std::string label; };   // on = false: the reading says No (drawn grey)
struct BarSet { Bar1 stop, eff, exh, ans; };
inline Bar1 ratioBar(long long v1000, long long minShow1000) {
    Bar1 b; if (v1000 < 0 || v1000 < minShow1000) return b;
    b.show = true; b.v = (double)v1000 / 1000.0; b.on = b.v >= 0.5; b.label = ratioText(b.v); return b;
}
inline BarSet barsOf(const Read& r) {
    BarSet s;
    s.stop = ratioBar(r.stopX1000, 50);                                   // under 0.05x: nothing to draw
    s.eff = ratioBar(r.eff1000, 0);
    if (r.dem1e4 >= 0) { s.exh.show = true; s.exh.v = exhBar((double)r.dem1e4 / 1e4); s.exh.on = r.exh > 0; s.exh.label = ratioText(s.exh.v); }
    if (r.marks >= 0 && r.ansOk) { s.ans.show = true; s.ans.v = r.marks > 0 ? (double)r.marks : 0.06; s.ans.on = r.marks >= 1; s.ans.label = std::to_string(r.marks) + "/3"; }
    return s;
}
// bar height in pixels: full height (half the pane) at BAR_CAP; never under 2 px
inline int barPx(double v, int half) {
    if (!std::isfinite(v) || v < 0 || half <= 0) return 0;
    const int h = (int)std::lround(std::min(v, BAR_CAP) / BAR_CAP * half); return h < 2 ? 2 : h;
}
enum { T_TEXT = 0, T_MUTED = 1, T_DIR = 2, T_AMBER = 3 };               // T_DIR = the colour of the turn: red at a swept high, green at a swept low
struct Seg { std::string text; int tone = T_TEXT; bool bold = false; };
struct Strip { std::vector<Seg> segs; std::string pill, flat; int pillTone = T_MUTED, edgeTone = T_MUTED; };
inline const char* verdictText(int v, bool bestProven) {
    return v == V_UNCLEAR ? "UNCLEAR" : v == V_ASIDE ? "STAND ASIDE" : v == V_WAIT ? "WAIT" : v == V_BEST ? (bestProven ? "BEST ODDS" : "BEST ODDS ?") : "NO READ";
}
// prefix = which sweep ("PhrH 9:03 AM"); numbers = add the ratios to the words (when the bars are too narrow for labels)
inline Strip stripOf(const Read& r, bool bestProven, bool numbers, const std::string& prefix) {
    Strip s;
    struct Add { Strip& s; void operator()(const std::string& t, int tone, bool bold) const { Seg g; g.text = t; g.tone = tone; g.bold = bold; s.segs.push_back(g); s.flat += t; } } add = { s };
    const std::string sep = " / ";
    if (!prefix.empty()) add(prefix + "   ", T_MUTED, false);
    if (r.stopX1000 >= 0) { const double x = (double)r.stopX1000 / 1000.0; const int k = stopClass(x);
        add(std::string(word4(k)) + " Stop Run" + (numbers ? " " + ratioText(x) : std::string()), k == 0 ? T_MUTED : T_TEXT, true); }
    else add("\x96 Stop Run", T_MUTED, true);
    add(sep, T_MUTED, false);
    if (r.eff1000 >= 0) { const double x = (double)r.eff1000 / 1000.0; const int k = effClass(x);
        add(std::string(word4(k)) + " Effort" + (numbers ? " " + ratioText(x) : std::string()), k == 0 ? T_MUTED : T_TEXT, true); }
    else add("\x96 Effort", T_MUTED, true);
    add(sep, T_MUTED, false);
    if (r.dem1e4 >= 0 && r.exh >= 0) {
        add(std::string(word4(r.exh)) + " Exhaustion" + (numbers ? " " + ratioText(exhBar((double)r.dem1e4 / 1e4)) : std::string()) + (r.verdict == V_UNCLEAR ? " ?" : ""), r.exh == 0 ? T_MUTED : T_DIR, true); }
    else add("\x96 Exhaustion", T_MUTED, true);
    add(sep, T_MUTED, false);
    if (r.marks >= 0 && r.ansOk) add(std::string(word4(r.marks)) + " Answer" + (numbers ? " " + std::to_string(r.marks) + "/3" : std::string()), r.marks == 0 ? T_MUTED : T_DIR, true);
    else add("\x96 Answer", T_MUTED, true);
    const int vt = r.verdict == V_BEST ? T_DIR : r.verdict == V_WAIT ? T_AMBER : T_MUTED;
    add("   \xBB ", T_MUTED, false);
    add(verdictText(r.verdict, bestProven), vt, true);
    s.edgeTone = vt; s.pillTone = vt;
    char b[24];
    if ((r.verdict == V_ASIDE || r.verdict == V_WAIT || (r.verdict == V_BEST && bestProven)) && r.odds >= 0) { std::snprintf(b, sizeof b, "%d%% turn", r.odds); s.pill = b; }
    else if (r.verdict == V_BEST) s.pill = "?";
    else s.pill = "\x96";
    return s;
}

}  // namespace turn
