// TurnLogic.h 1.2.1 (2026-11-23; 1.2.0 and 1.1.0 the same day; 1.0.1 2026-11-22; 1.0.0 the same day) -- the logic of lsTurn "Turn v1.2" (Turn.cpp). SDK-free, unit tested (tests_turn).
// (1.2.1, Rassul 2026-11-23 11:47 CT "go" on the Stop Run fault: CL counted 81% of its fills as stops, NQ 42% in the session before) THE
// ORDER-ID CLOCK IS NO LONGER STOPPED BY ONE OLD ORDER. IRT's order IDs are an 8-digit counter that starts again at 0 after 99,999,999
// (CL: 99.99 M on Friday 2:27 PM, 27.5 M on Monday 9:00 AM), so an order from the lap before can carry a HIGHER number than every new
// order. 1.0.0 - 1.2.0 took any ID up to 16.7 M above the newest as the newest: on CL 2026-11-23 9:49:52 AM one such order (ID
// 44,965,432 while new orders were at 30.4 M) stopped the clock at 431 points for the hours the counter needs to get there, every
// aggressor looked 60+ s old and was counted as a stop. Now (IdClock): an ID above the newest moves the clock only when the counter can
// have advanced that far since (20,000 + 5,000 a second; measured on IRT's own feed: never over 5,000 + 5,000 a second); a farther one
// is a candidate that needs 32 rising IDs of its own with no ID of the running count in between; and 64 new orders in a row far below
// the newest (the counter started again at 0, or the newest was not real) set the clock back to them. Nothing else changes.
// (1.2.0, Rassul 2026-11-23 CT 11:00 "show me this indicator with just effort .. show mockup"; 11:04 "meke values that are less than 1.5 gray";
// 11:06 "this look simple and easy  i like it.   lets use this for now.. save other setting because i might change my mind and add it
// pieces later"): THE PANE SHOWS JUST EFFORT - one bar a side on every closed candle, coloured only when the number written on it is 1.5 or
// more. The other pieces (the Stop Run bar, the Exhaustion bar, the Answer bar, the read strip with level / verdict / odds) are kept whole
// and switched by a small display file (Show, parseShow; no file = Effort only): the saved full setting brings the 1.1.0 pane back, piece
// by piece. Nothing that is read, decided or recorded changes - only what is drawn.
// (1.1.0, Rassul 2026-11-23 CT 09:45 "does the turn indicator only show values when there is a level"; 09:47 "can you show the indicator on
// all bars ?"; 09:51 "go -  for now just do effort"): Effort on EVERY closed candle - the buyers' Effort at the top of the candle (up, green)
// and the sellers' Effort at its bottom (down, red), the same ratio and the same right-hand column as on a candle at a level (effOf); the
// strip of a plain candle under the mouse (stripOfEffort); pickLabels made fast for a pane where every candle carries bars (same answer).
// Stop Run, Exhaustion, the Answer and the verdict are unchanged: at named levels and on sweep candles only.
// (1.0.1, Rassul 22:50-23:00 CT "indicator should update when my mouse hovers over different candles"; "top strip should be moved higher to the
// top of the pane, right now there is blackspace at the top . i cant see the ratios either"; "go"): the strip of a candle at a named level
// (stripOfCand, levelNameAt, candSide) and one ratio label per candle side when the candles are too narrow for two (pickLabels); the
// ratio on a bar is written without the x (labelText; 23:07 CT "get rid of the x and make it smaller").
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

static const char* const VERSION = "1.8.0";   // (1.8.0, Rassul 2026-11-24 17:52 CT "the absorbtion indicator can be updated to have two bars per candle , 1 for absorbtion and 1 for initiative", 18:01 "i want more detail for the intiative buyers", 18:53 "build" on turn-initiative-mockup.html v3): INITIATIVE = the counter side's heaviest single price in the end quarter (buys at the low, sells at the high) / its 20-candle mean, on when >= 1.5, bigger than the attackers' heaviest price there and the candle closed outside that quarter. Each candle: left bar = Absorption, right bar = Initiative, each at its stronger end. The read: Initiative / How (burst, sweep, biggest order) / Against. Research 2026-11-24 (new 10-candle lows / highs, 7 markets): initiative only 66% went 1 ATR first (n 711, halves 66 / 67) vs 57% ordinary, 61% absorption only; burst at the market's 90th percentile of the prior hour's pace 83% vs 67% (n 30, "?"), swept 4+ prices 76% vs 67% (n 55, "?")
// was static const char* const VERSION = "1.7.0";   // (1.7.0, Rassul 2026-11-24 17:28 CT "i think absorbtion read should be in the absorption indicator but it should be more compact without the result and next", 17:30 "lets go with this for now" / "build" on turn-read-mockup.html): THE COMPACT READ - for the candle under the mouse (else the last closed one): who won at the end that was pushed (Passive defense / Aggressor), then Attack and Absorbed / Not absorbed only; an Abs names its level in the heading. Research VERDICT 2026-11-24: 12,406 candles 1.5x+, passive defense held 40% (both halves 40 / 40) vs aggressor 15% (16 / 13); aggressor ran on 1 ATR 42% vs 26%, all 7 markets
// was static const char* const VERSION = "1.6.0";   // (1.6.0, Rassul 2026-11-24 12:51 CT "2. do the test" on tst-line3-mockup.html): RevRead\\turnexq-<MKT>-<spb>.txt = the sellers' / buyers' Ex? (last minute <= 0.5x the first 2 minutes' pace) of EVERY closed candle this pane saw whole - the E of the Tst badge's line 3 (lsRevReadMarks 0.28.0); nothing drawn changed
// was static const char* const VERSION = "1.5.0";   // (1.5.0, Rassul 2026-11-24 11:12 CT "ok. build this", abs-badge-mockup.html v15) THE ABS READ: the order feed now keeps the book per price (added / pulled / most showing / refills) and the biggest single order a side; the read right of the last candle = the newest Abs of lsRevRead (Attack / Absorbed / Result / Ex or Next); RevRead\\turnabs-<MKT>-<spb>.txt = the P and Ex? of each Abs for the price-chart badge
// was static const char* const VERSION = "1.4.2";   // (1.4.2, Rassul 2026-11-24 11:21 CT "first color the bars differently , top should be green and bottom should be red") host only: the Absorption bars in the attackers' colour (top green, bottom red)
// was static const char* const VERSION = "1.4.1";   // (1.4.1, 2026-11-23 22:40 "fix everything") the top line shows a sweep of the current Globex session only (sessionOf); before, the newest sweep of an earlier session stayed there all evening
// was static const char* const VERSION = "1.4.0";   // (1.4.0, Rassul 2026-11-23 20:47 CT "build the read to the right llike you do for all the other indicators", 20:48 "i need something more specific because i already have the ratios on the bars", 20:52 "what else can you add that would be a bullish or bearish indication or telling me about direciton", 20:53 "lets go with this"): THE ABSORPTION READ right of the last candle - absRead() below: the direction from where price stands against the absorbed prices that still hold (or the one that just failed), the price, the lots, held / tested, rising lows / falling highs, and what to watch
// was static const char* const VERSION = "1.3.0";   // (1.3.0, Rassul 2026-11-23 18:15 CT "can you update my turn indicator to this .  so update effort to absrobtion also", 18:28 "i want you to do the absorbtion the right way", 18:38 "Research first before building so you do it right", 19:48 "build" on turn-four-pieces-mockup.html v3): the four pieces Stop Run / Absorption / Exhaustion / Initiative, all shown by default. ABSORPTION replaces Effort on the bars and in the strip: the lots at the heaviest single price in the end quarter / its average over the 20 closed candles before; coloured (the winner: red at a high, green at a low) from 2.0 AND only when the candle closes outside that end quarter. "Answer" is called Initiative. The record file and its Effort column are unchanged
// was static const char* const VERSION = "1.2.1";
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
// (1.0.1, Rassul 23:07 CT on the mockup: "the font for the ratio looks big , it may overlap on the other bar next to it, maybe get rid of the x
// and make it smaller") the ratio ON a bar: no x, one decimal under 10, none from 10 up ("2.9", "14"; 99+ cap). The strip keeps ratioText.
inline std::string labelText(double x) {
    if (!std::isfinite(x) || x < 0) return std::string();
    char b[24]; if (x >= 99.5) return "99+";
    std::snprintf(b, sizeof b, x >= 9.95 ? "%.0f" : "%.1f", x); return b;
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
struct Bars { bool topOk = false, botOk = false; double top = 0, bot = 0; long long levels = 0, zone = 0; long long topMax = -1, botMax = -1; long long topMaxTick = 0, botMaxTick = 0; long long topIni = -1, botIni = -1, topIniTick = 0, botIniTick = 0; };   // (1.8.0) topIni / botIni = the COUNTER side's heaviest single price in the end quarter (sells at the top, buys at the bottom) - Initiative   // (1.4.0) + the price (tick) of that heaviest price: the highest such price at the top, the lowest at the bottom   // (1.3.0) topMax / botMax = the lots at the heaviest single price in the end quarter (buys at the top, sells at the bottom); -1 = no candle
inline long long zoneOf(long long L) { const long long z = (L + 2) / 4; return z < 1 ? 1 : z; }   // round(0.25 L), half up
inline Bars barRatios(const std::vector<Row>& rows, long long lo, long long hi) {
    Bars b; if (hi < lo) return b;
    const long long L = hi - lo + 1; if (L < 4 || L > 200000) return b;
    const long long z = zoneOf(L); b.levels = L; b.zone = z;
    long long bt = 0, br = 0, st = 0, sr = 0, bmx = 0, smx = 0, bmt = 0, smt = 0, tix = 0, bix = 0, tit = 0, bit = 0;
    for (const Row& r : rows) {
        if (r.tick < lo || r.tick > hi || r.buy < 0 || r.sell < 0) continue;
        if (r.tick > hi - z) bt += r.buy; else br += r.buy;
        if (r.tick < lo + z) st += r.sell; else sr += r.sell;
        if (r.tick > hi - z && (r.buy > bmx || (r.buy == bmx && bmx > 0 && r.tick > bmt))) { bmx = r.buy; bmt = r.tick; }
        if (r.tick < lo + z && (r.sell > smx || (r.sell == smx && smx > 0 && r.tick < smt))) { smx = r.sell; smt = r.tick; }
        if (r.tick > hi - z && (r.sell > tix || (r.sell == tix && tix > 0 && r.tick > tit))) { tix = r.sell; tit = r.tick; }   // (1.8.0) sellers at the top
        if (r.tick < lo + z && (r.buy > bix || (r.buy == bix && bix > 0 && r.tick < bit))) { bix = r.buy; bit = r.tick; }      // (1.8.0) buyers at the bottom
    }
    b.topIni = tix; b.botIni = bix; b.topIniTick = tix > 0 ? tit : hi; b.botIniTick = bix > 0 ? bit : lo;
    const double nz = (double)z, nr = (double)(L - z);
    b.topMax = bmx; b.botMax = smx; b.topMaxTick = bmx > 0 ? bmt : hi; b.botMaxTick = smx > 0 ? smt : lo;
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
// order IDs: IRT's are ints that count up; unwrapped as 32-bit against the newest so a 32-bit wrap never breaks the clock.
// (1.2.1) THE COUNT IS NOT ONE LINE: (a) it is an 8-digit counter - after 99,999,999 it starts again near 0, so orders of the lap before
// have higher numbers than the new ones; (b) such an old order shows up among the new ones - as a NEW order (a stop-limit that was
// triggered, an order re-announced) or as an aggressor (a triggered stop) - 15 to 35 times a session a market, one ID or a handful of
// neighbours. The clock follows the RUNNING count only:
//   near      an ID above the newest by no more than BASE + RATE x (seconds since the newest moved): the count went on. Moves the clock.
//   far above a candidate lane. It becomes the clock only after LANE_VOTES rising IDs of that lane with no ID of the running count
//             (above it, or less than BASE below it) in between - a real jump (the feed was away, the pane started on an old order).
//             One running ID drops the candidate: it was an old order.
//   far below (more than BASE under the newest) from a NEW order: says nothing by itself (an old order announced again), but LOW_RUN
//             of them in a row with no running ID in between = the newest is not the count (the counter started again at 0, or the
//             clock was caught by old orders after all): the clock is set back to where MOST of them are - the newest ID within BASE
//             of their median (an old order among them never becomes the newest again); points above it are dropped.
//   An old aggressor below the newest never counts for or against: that is what a stop looks like.
// After a lane change or a set-back the clock is "known since" that second: a candle that began less than 60 s later has no Stop Run.
struct IdClock {
    struct P { long long sec, id; };
    std::deque<P> pts; long long maxId = 0, maxSec = 0, since = -1; bool have = false;
    long long candId = 0, candSec = 0; int candVotes = 0; bool candOn = false;
    long long lowIds[64]; int lowRun = 0;                                    // the far-below NEW IDs of the running streak (LOW_RUN of them)
    long long jumps = 0, strays = 0, lanes = 0, setBacks = 0;                // jumps = candidates opened; strays = dropped; lanes = accepted
    static const long long BASE = 20000, RATE = 5000, GAP_CAP = 3600;
    static const int LANE_VOTES = 32, LOW_RUN = 64;
    long long unwrap(long long raw) const {
        const uint32_t u = (uint32_t)(raw & 0xFFFFFFFFLL);
        if (!have) return (long long)u;
        const int32_t d = (int32_t)(u - (uint32_t)(maxId & 0xFFFFFFFFLL));
        return maxId + (long long)d;
    }
    static long long reach(long long sec, long long from) { const long long dt = sec - from; return BASE + RATE * (dt < 0 ? 0 : dt > GAP_CAP ? GAP_CAP : dt); }
    void push(long long id, long long sec) {
        maxId = id; maxSec = sec; have = true;
        if (!pts.empty() && pts.back().sec >= sec) pts.back().id = id;
        else { P p; p.sec = sec; p.id = id; pts.push_back(p); if (since < 0) since = sec; if (pts.size() > CLOCK_CAP) pts.pop_front(); }
    }
    void dropCand() { if (candOn) { candOn = false; ++strays; } }
    // fromNew = the ID of a NEW order (else: the aggressor of a fill)
    void note(long long id, long long sec, bool fromNew) {
        if (!have) { push(id, sec); return; }
        if (id > maxId) {
            if (id <= maxId + reach(sec, maxSec)) { dropCand(); lowRun = 0; push(id, sec); return; }   // the count went on
            if (!candOn || id < candId - BASE || id > candId + reach(sec, candSec)) { dropCand(); ++jumps; candOn = true; candId = id; candSec = sec; candVotes = 1; return; }
            if (id <= candId) return;                                     // the same old order again, or a neighbour below it: no vote
            candId = id; candSec = sec;
            if (++candVotes < LANE_VOTES) return;
            candOn = false; ++lanes; lowRun = 0;                          // the count IS there now: the older points stay (they are below)
            push(id, sec); since = sec;
            return;
        }
        if (id >= maxId - BASE) { dropCand(); lowRun = 0; return; }       // the running count, a moment behind
        if (!fromNew) return;                                             // an old aggressor: a stop, not a word about the count
        lowIds[lowRun] = id;
        if (++lowRun < LOW_RUN) return;
        long long srt[LOW_RUN]; for (int k = 0; k < LOW_RUN; ++k) srt[k] = lowIds[k];
        std::sort(srt, srt + LOW_RUN); const long long med = srt[LOW_RUN / 2];
        long long to = med; for (int k = 0; k < LOW_RUN; ++k) if (srt[k] <= med + BASE && srt[k] > to) to = srt[k];
        ++setBacks; lowRun = 0; candOn = false;
        while (!pts.empty() && pts.back().id > to) pts.pop_back();        // what was noted above it was not the count
        maxId = to; maxSec = sec; { P p; p.sec = sec; p.id = to; if (!pts.empty() && pts.back().sec >= sec) pts.back().id = to; else pts.push_back(p); }
        since = sec;
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
    void clear() { pts.clear(); maxId = 0; maxSec = 0; since = -1; have = false; candOn = false; candVotes = 0; lowRun = 0; }
    IdClock() { for (int k = 0; k < 64; ++k) lowIds[k] = 0; }
};
struct Fill { long long sec = 0, px = 0, size = 0, aid = 0; signed char ag = 0; bool fresh = false, stop = false; };
struct Cell { long long buy = 0, sell = 0, stopB = 0, stopS = 0; };      // aggressor lots at the price; stop = from old orders never seen resting
struct Turn { bool ok = false; long long extPx = 0, extSec = 0, hard = 0, big = 0; bool fresh = false; size_t ie = 0; };
struct Exh { bool ok = false, hasDecay = false; double late = 0, decay = 0, after = 0; long long push = 0; };
// (1.5.0) THE ABS DETAIL (Rassul 2026-11-24 "In the indicator read do you mention how many the absorbtion was done like iceber lots were absorbed",
// "I also want to know how big the print was", "Attack and defense should be more focused on how 1 side absorbed the other", "ok. build this"):
// per candle and price the RESTING side's book changes ([0] bids, [1] offers) - lots added, lots pulled, the most showing when hit, refills
// (an order topped up after it filled = an iceberg tranche) - and per side the biggest single aggressive order (fills grouped by aggressor ID)
struct BookCell { long long add[2] = { 0, 0 }, pull[2] = { 0, 0 }, shownMax[2] = { 0, 0 }, refills[2] = { 0, 0 }, refillLots[2] = { 0, 0 }, filled[2] = { 0, 0 }; };
struct BigPr { long long lots = 0, lo = 0, hi = 0; };
// (1.8.0) how the counter side took an end quarter: [0] = buyers at the bottom, [1] = sellers at the top. big = the biggest single order (fills grouped
// by aggressor ID) with a fill in the quarter; sweep = the most ticks one such order spanned; burst = its most lots in one 5-second step in the quarter
struct IniQ { BigPr big; long long sweep = 0, burst = 0; };
static const size_t BOOK_CAP = 60000, HIST_P = 400;
struct Sum {
    long long key = 0, start = 0; bool complete = false, overflow = false, stopOk = false, thin = true;
    std::map<long long, BookCell> book;                                   // (1.5.0) the prices that traded this candle: their book changes
    long long bigAny[2] = { -1, -1 };                                     // (1.5.0) the biggest single order of the sellers [0] / buyers [1] anywhere in the candle (-1 = not known)
    BigPr qBig[2];                                                        // (1.5.0) the biggest single SELL order wholly inside the bottom quarter [0], BUY order inside the top quarter [1]
    IniQ iq[2];                                                           // (1.8.0) [0] buyers at the bottom, [1] sellers at the top
    long long sideLots[2] = { 0, 0 };                                     // (1.8.0) sellers [0] / buyers [1] lots in the whole candle (the pace of the hour before)
    long long early[2] = { 0, 0 }, late[2] = { 0, 0 };                    // (1.5.0) sellers [0] / buyers [1] lots in the first 2/3 of the candle and in its last 1/3 (the last minute of 3)
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
    struct Hist { long long key = 0, big[2] = { -1, -1 }, lots[2] = { -1, -1 }; };   // (1.8.0) lots = the side's lots in the candle (-1 = not known)
    std::deque<Hist> hist;                                                // (1.5.0) every closed candle's biggest order a side (-1 = not known), oldest first, HIST_P kept
    // a fresh start (new chart / symbol / reconnect): everything open is dropped and the next candle cannot be complete
    void reset(int barSecs) {
        spb = barSecs; fills_.clear(); std::vector<Fill>().swap(fills_); ords_.clear(); clock_.clear();
        curKey_ = 0; liveSince_ = -1; haveRef_ = false; ref_ = 0; overflow_ = false; sums.clear(); lastClosed_ = 0; limitKey_ = 0; implied_ = 0;
        bk_.clear(); depth_.clear(); curBook_.clear(); hist.clear();
    }
    // the newest candle the chart allows: an event dated past it is counted and never opens a candle
    void setLimit(long long chartFormingKey) { limitKey_ = chartFormingKey > 0 ? chartFormingKey + spb : 0; }
    void gap() {                                                           // the stream was interrupted: open candle and the clock are not whole
        fills_.clear(); curKey_ = 0; liveSince_ = -1; overflow_ = false; clock_.clear(); ords_.clear(); implied_ = 0;
        bk_.clear(); depth_.clear(); curBook_.clear();   // (1.5.0) the book is not whole after a gap
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
            if (oid) { if (action == 0) clock_.note(oid, sec, true); shown(oid, px); bookPut(oid, px, size, buySell == 1 ? 0 : buySell == 0 ? 1 : -1); }
            break; }
        case 2: if (oid) { ords_.erase(oid); bookDel(oid); } break;
        case 3: {
            ++n.fills; ref_ = px; haveRef_ = true;
            const long long aid = aidRaw ? clock_.unwrap(aidRaw) : 0;
            if (oid && oid == aid) { ++n.bad; break; }                    // an order filling itself is not a trade between two sides
            if (oid) bookFill(oid, size);                                  // (1.5.0) the resting order that was hit
            Fill f; f.sec = sec; f.px = px; f.size = size;
            if (buySell != 0 && buySell != 1) { ++n.implied; ++implied_; }   // implied (no aggressor): kept for the extreme and the print order only
            else {
                f.ag = (signed char)(buySell == 1 ? 1 : -1); f.aid = aid;
                if (aid) {
                    const long long a = clock_.age(aid, sec);
                    f.fresh = a >= 0 && a < FRESH_S;
                    f.stop = a >= OLD_S && ords_.find(aid) == ords_.end();   // issued 60+ s ago and never seen resting: a stop / hidden order
                    if (f.stop) ++n.stopFills;
                    clock_.note(aid, sec, false);
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
    long long idStrays() const { return clock_.strays; }                 // (1.2.1) old orders that showed up above the count and were left out
    long long idLanes() const { return clock_.lanes; }                   //         real jumps of the count taken over
    long long idSetBacks() const { return clock_.setBacks; }             //         times the clock was set back to the running count
    long long forming() const { return curKey_; }
private:
    std::vector<Fill> fills_; std::unordered_map<long long, long long> ords_; IdClock clock_;   // ords_: resting order ID -> its price
    struct BO { long long px = 0, size = 0, filled = 0; int side = 0; };
    std::unordered_map<long long, BO> bk_; std::map<long long, std::pair<long long, long long> > depth_; std::map<long long, BookCell> curBook_;   // (1.5.0) resting orders near price, lots resting per price (bids, offers), this candle's changes
    long long& dep(long long px, int side) { std::pair<long long, long long>& d = depth_[px]; return side == 0 ? d.first : d.second; }
    BookCell* cell(long long px) { std::map<long long, BookCell>::iterator it = curBook_.find(px); if (it != curBook_.end()) return &it->second; if (curBook_.size() >= CELL_CAP) return nullptr; return &curBook_[px]; }
    void note(long long px, int side) { BookCell* c = cell(px); if (!c) return; const long long d = dep(px, side); if (d > c->shownMax[side]) c->shownMax[side] = d; }
    void bookPut(long long oid, long long px, long long size, int side) {
        std::unordered_map<long long, BO>::iterator it = bk_.find(oid);
        if (it == bk_.end()) {
            if (side < 0 || size <= 0) return;
            if (haveRef_ && std::llabs(px - ref_) > NEAR_TICKS) return;
            if (bk_.size() >= BOOK_CAP) bookPrune();
            if (bk_.size() >= BOOK_CAP) return;
            BO o; o.px = px; o.size = size; o.side = side; bk_[oid] = o;
            dep(px, side) += size; if (BookCell* c = cell(px)) c->add[side] += size; note(px, side); return;
        }
        BO& o = it->second;
        if (px != o.px) {                                                  // moved: pulled there, added here
            dep(o.px, o.side) -= o.size; if (BookCell* c = cell(o.px)) c->pull[o.side] += o.size;
            o.px = px; o.size = size; o.filled = 0; dep(px, o.side) += size; if (BookCell* c = cell(px)) c->add[o.side] += size; note(px, o.side); return;
        }
        const long long d = size - o.size; o.size = size; dep(px, o.side) += d;
        if (d > 0) { if (BookCell* c = cell(px)) { c->add[o.side] += d; if (o.filled > 0) { ++c->refills[o.side]; c->refillLots[o.side] += d; } } o.filled = 0; note(px, o.side); }
        else if (d < 0) { if (BookCell* c = cell(px)) c->pull[o.side] += -d; }
    }
    void bookDel(long long oid) {
        std::unordered_map<long long, BO>::iterator it = bk_.find(oid); if (it == bk_.end()) return;
        const BO& o = it->second; if (o.size > 0) { dep(o.px, o.side) -= o.size; if (BookCell* c = cell(o.px)) c->pull[o.side] += o.size; }
        bk_.erase(it);
    }
    void bookFill(long long oid, long long size) {
        std::unordered_map<long long, BO>::iterator it = bk_.find(oid); if (it == bk_.end() || size <= 0) return;
        BO& o = it->second; note(o.px, o.side);                            // the most showing at the moment it was hit
        const long long k = std::min(size, o.size); o.size -= k; o.filled += size; dep(o.px, o.side) -= k;
        if (BookCell* c = cell(o.px)) c->filled[o.side] += size;
    }
    void bookPrune() {
        for (std::unordered_map<long long, BO>::iterator it = bk_.begin(); it != bk_.end();) { if (!haveRef_ || std::llabs(it->second.px - ref_) > NEAR_TICKS) { dep(it->second.px, it->second.side) -= it->second.size; it = bk_.erase(it); } else ++it; }
        for (std::map<long long, std::pair<long long, long long> >::iterator it = depth_.begin(); it != depth_.end();) { if (it->second.first <= 0 && it->second.second <= 0) it = depth_.erase(it); else ++it; }
    }
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
        for (std::map<long long, BookCell>::const_iterator it = curBook_.begin(); it != curBook_.end(); ++it) if (s.cells.count(it->first)) s.book.insert(*it);   // (1.5.0) the traded prices only (as the research store)
        curBook_.clear(); if (depth_.size() > 4096) bookPrune();
        { Hist h; h.key = key; if (s.complete) { h.big[0] = std::max(0LL, s.bigAny[0]); h.big[1] = std::max(0LL, s.bigAny[1]); h.lots[0] = s.sideLots[0]; h.lots[1] = s.sideLots[1]; } hist.push_back(h); while (hist.size() > HIST_P) hist.pop_front(); }
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
        {   // (1.5.0) the biggest single order a side (fills grouped by aggressor ID), the quarters' biggest, the first 2/3 vs the last 1/3
            struct G { long long lots = 0, lo = 0, hi = 0; int sd = 0; }; std::unordered_map<long long, G> g;
            const long long cut = s.start + (long long)spb * 2 / 3;
            for (size_t k = 0; k < nf; ++k) { const Fill& f = F[k]; if (f.ag == 0) continue; const int sd = f.ag > 0 ? 1 : 0;
                (f.sec < cut ? s.early[sd] : s.late[sd]) += f.size; s.sideLots[sd] += f.size;
                if (!f.aid) continue;
                std::unordered_map<long long, G>::iterator gi = g.find(f.aid);
                if (gi == g.end()) { if (g.size() >= 200000) continue; G x; x.lo = x.hi = f.px; x.sd = sd; gi = g.insert(std::make_pair(f.aid, x)).first; }
                G& x = gi->second; x.lots += f.size; if (f.px < x.lo) x.lo = f.px; if (f.px > x.hi) x.hi = f.px; }
            s.bigAny[0] = s.bigAny[1] = 0;
            const long long L = s.hiPx - s.loPx + 1, z = L >= 4 ? zoneOf(L) : 0;
            for (std::unordered_map<long long, G>::const_iterator it = g.begin(); it != g.end(); ++it) { const G& x = it->second;
                if (x.lots > s.bigAny[x.sd]) s.bigAny[x.sd] = x.lots;
                if (z <= 0) continue;
                const bool in = x.sd == 0 ? x.hi < s.loPx + z : x.lo > s.hiPx - z;
                if (in && x.lots > s.qBig[x.sd].lots) { s.qBig[x.sd].lots = x.lots; s.qBig[x.sd].lo = x.lo; s.qBig[x.sd].hi = x.hi; }
                const int e = x.sd == 1 ? 0 : 1; const bool touch = x.sd == 1 ? x.lo < s.loPx + z : x.hi > s.hiPx - z;   // (1.8.0) the counter side's orders with a fill in the quarter
                if (touch) { IniQ& q = s.iq[e]; if (x.lots > q.big.lots) { q.big.lots = x.lots; q.big.lo = x.lo; q.big.hi = x.hi; } if (x.hi - x.lo + 1 > q.sweep) q.sweep = x.hi - x.lo + 1; } }
            if (z > 0) for (int e = 0; e < 2; ++e) {                     // (1.8.0) the burst: the counter side's most lots in one 5-second step inside the quarter
                std::map<long long, long long> st; const int sd = e == 0 ? 1 : 0;
                for (size_t k = 0; k < nf; ++k) { const Fill& f = F[k]; if (f.ag == 0 || (f.ag > 0 ? 1 : 0) != sd) continue;
                    if (e == 0 ? !(f.px < s.loPx + z) : !(f.px > s.hiPx - z)) continue;
                    const long long b5 = f.sec >= 0 ? f.sec / 5 : (f.sec - 4) / 5; long long& v = st[b5]; v += f.size; if (v > s.iq[e].burst) s.iq[e].burst = v; } } }
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
    b.show = true; b.v = (double)v1000 / 1000.0; b.on = b.v >= 0.5; b.label = labelText(b.v); return b;
}
inline BarSet barsOf(const Read& r) {
    BarSet s;
    s.stop = ratioBar(r.stopX1000, 50);                                   // under 0.05x: nothing to draw
    s.eff = ratioBar(r.eff1000, 0);
    if (r.dem1e4 >= 0) { s.exh.show = true; s.exh.v = exhBar((double)r.dem1e4 / 1e4); s.exh.on = r.exh > 0; s.exh.label = labelText(s.exh.v); }
    if (r.marks >= 0 && r.ansOk) { s.ans.show = true; s.ans.v = r.marks > 0 ? (double)r.marks : 0.06; s.ans.on = r.marks >= 1; s.ans.label = std::to_string(r.marks) + "/3"; }
    return s;
}
// bar height in pixels: full height (half the pane) at BAR_CAP; never under 2 px
inline int barPx(double v, int half) {
    if (!std::isfinite(v) || v < 0 || half <= 0) return 0;
    const int h = (int)std::lround(std::min(v, BAR_CAP) / BAR_CAP * half); return h < 2 ? 2 : h;
}
enum { T_TEXT = 0, T_MUTED = 1, T_DIR = 2, T_AMBER = 3, T_BUY = 4, T_SELL = 5 };   // T_DIR = the colour of the turn: red at a swept high, green at a swept low; (1.1.0) T_BUY green, T_SELL red
struct Seg { std::string text; int tone = T_TEXT; bool bold = false; };
struct Strip { std::vector<Seg> segs; std::string pill, flat; int pillTone = T_MUTED, edgeTone = T_MUTED; };
// the number WRITTEN on a bar, in tenths ("1.5" -> 15, "0.5" -> 5, "14" -> 140, "99+" -> 990); -1 = no number. The colour of an Effort
// bar follows what the bar says: a bar that reads 1.5 is coloured, whatever the digits behind it.
inline long long shown10(const std::string& label) {
    if (label.empty()) return -1;
    char* e = nullptr; const double x = std::strtod(label.c_str(), &e);
    if (e == label.c_str() || !std::isfinite(x) || x < 0) return -1;
    return (long long)std::llround(x * 10.0);
}
// ---------------------------------------------------------------------------------------------------------------------------------
// (1.3.0) ABSORPTION. Research 2026-11-23 (603 first RTH tests of a major level, 7 markets, 2026-09-10 .. 11-20; turned = 2 ATR away
// before 0.5 ATR through, within 10 candles): the CLOSE test separates (closes outside the tested end quarter 73% turned, 319 / 435; in
// it 46%, 78 / 168); Effort does not (AUC 0.48); volume adds only after the close test, and the heaviest single price does it best
// (2.0x or more: 80%, 126 / 157, against 69%). Per market the volume step is under 20 a side: LEARNING.
//   number  = the lots at the heaviest single price in the candle's end quarter (buys at the top, sells at the bottom) / the average of
//             the same over the ABS_AVG closed candles before it that have 4+ price levels (ABS_MIN at least, else no bar)
//   away    = the candle closed OUTSIDE that end quarter (top: close <= high - zone; bottom: close >= low + zone): no result for the push
//   counts  = number written >= from10 (2.0) AND away. Colour = the winner: absorbed at the high = red, at the low = green.
static const int ABS_AVG = 20, ABS_MIN = 10, ABS_BACK = 80; static const long long ABS_FROM10 = 20;
struct Abs { long long x1000 = -1; bool away = false; };                 // x1000 -1 = not known / no bar
inline bool absAway(long long hi, long long lo, long long cl, long long zone, bool top) { return zone > 0 && hi >= lo && (top ? cl <= hi - zone : cl >= lo + zone); }
// cur = this candle's heaviest-price lots; prior = the same of the candles before it, newest first (only candles with 4+ levels)
inline long long absX1000(long long cur, const std::vector<long long>& prior) {
    double sum = 0; int n = 0;
    if (cur < 0) return -1;
    for (size_t k = 0; k < prior.size() && n < ABS_AVG; ++k) { if (prior[k] < 0) continue; sum += (double)prior[k]; ++n; }
    if (n < ABS_MIN || !(sum > 0)) return -1;
    return x1000((double)cur / (sum / n));
}
// No / Light / Moderate / Heavy by the number written (tenths): No when the candle closed in that end quarter or the number is under 1.0;
// Heavy = it counts (from10 or more)
inline int absClass10(long long t, bool away, long long from10) { return !away || t < 10 ? 0 : t >= from10 ? 3 : t < 15 ? 1 : 2; }
inline Bar1 absBar(const Abs& a, long long from10) { Bar1 b = ratioBar(a.x1000, 0); b.on = b.show && a.away && shown10(b.label) >= from10; return b; }
// ---------------------------------------------------------------------------------------------------------------------------------
// (1.4.0) THE ABSORPTION READ - three plain lines right of the last candle (the panes' PlainRead format: head / why / watch).
// It says what the bars cannot: the PRICE where the push was absorbed, the LOTS there, what price did SINCE, and the DIRECTION.
//   event   = a closed candle side whose Absorption counts (number >= from10 AND closed outside that end quarter), in the last
//             ARD_WIN closed candles: low side = selling absorbed (bullish), high side = buying absorbed (bearish); px = the heaviest price.
//   failed  = a later candle CLOSED beyond px (below an absorbed low / above an absorbed high). tested = a later candle traded back
//             to px and closed on the right side. held = candles since.
//   head    = the NEWEST information: an absorbed low that holds = "Bullish ?", an absorbed high that holds = "Bearish ?", both hold
//             = "Range ?", the newest thing is a failure = the other way ("the absorbed low at P failed" = Bearish ?). "?" = LEARNING:
//             only the close test at major levels is measured (73% held); the rest is not. Nothing = "Quiet".
//   why     = "<lots> lots sold there, 5.2x normal. Held N candles, tested K times." (+ "Absorbed lows are rising." / "highs are falling.")
//   watch   = the close that changes the read.
// (1.4.1) the Globex session of an IRT time (CT clock seconds since 1904): sessions start at 17:00 CT
inline long long sessionOf(long long rt) { const long long x = rt - 17LL * 3600; return x >= 0 ? x / 86400 : -((-x + 86399) / 86400); }
static const int ARD_WIN = 20;
struct ACandle { long long key = 0, hi = 0, lo = 0, cl = 0; Abs up, dn; long long upPx = 0, upLots = -1, dnPx = 0, dnLots = -1; std::string lvUp, lvDn, swUp, swDn; };   // lv = a named level in that end quarter; sw = the level the candle swept at that end
struct AEvent { int i = 0, side = 0; long long px = 0, lots = 0, x1000 = 0; bool failed = false; int failAt = -1, tested = 0, held = 0; std::string lv, sw; };   // side +1 = an absorbed low (bullish), -1 = an absorbed high
struct ARead { int verdict = 0; std::string head, why, watch; std::vector<AEvent> ev; };   // verdict +1 bullish / -1 bearish / 2 range / 0 quiet
inline std::string pxText(long long ticks, double tick) {
    if (!(tick > 0) || !std::isfinite(tick)) return std::to_string(ticks);
    int d = 0; double t = tick; while (d < 7 && (std::round(t) < 1.0 || std::fabs(t - std::round(t)) > 1e-3)) { t *= 10.0; ++d; }
    double sc = 1.0; for (int k = 0; k < d; ++k) sc *= 10.0;
    const long long v = std::llround((double)ticks * tick * sc); const long long a = v < 0 ? -v : v; long long p = 1; for (int k = 0; k < d; ++k) p *= 10;   // whole units of the last decimal: no 1.16804 for 1.16805
    std::string fr = std::to_string(a % p); while ((int)fr.size() < d) fr.insert(fr.begin(), '0');
    return (v < 0 ? std::string("-") : std::string()) + std::to_string(a / p) + (d > 0 ? "." + fr : std::string());
}
inline std::string plural(long long n, const char* one, const char* many) { return std::to_string(n) + " " + (n == 1 ? one : many); }
inline ARead absRead(const std::vector<ACandle>& all, long long from10, double tick) {
    ARead R; const int total = (int)all.size(); const int a0 = std::max(0, total - ARD_WIN);
    std::vector<ACandle> C(all.begin() + a0, all.end()); const int n = (int)C.size();
    for (int i = 0; i < n; ++i) for (int sd = -1; sd <= 1; sd += 2) {
        const Abs& a = sd < 0 ? C[(size_t)i].up : C[(size_t)i].dn; if (!absBar(a, from10).on) continue;
        AEvent e; e.i = i; e.side = sd; e.px = sd < 0 ? C[(size_t)i].upPx : C[(size_t)i].dnPx; e.lots = sd < 0 ? C[(size_t)i].upLots : C[(size_t)i].dnLots; e.x1000 = a.x1000;
        e.lv = sd < 0 ? C[(size_t)i].lvUp : C[(size_t)i].lvDn; e.sw = sd < 0 ? C[(size_t)i].swUp : C[(size_t)i].swDn; if (e.px <= 0) continue;
        for (int j = i + 1; j < n; ++j) { const ACandle& q = C[(size_t)j];
            if (sd > 0 ? q.cl < e.px : q.cl > e.px) { e.failed = true; e.failAt = j; break; }
            if (sd > 0 ? q.lo <= e.px : q.hi >= e.px) ++e.tested; }
        e.held = (e.failed ? e.failAt : n) - 1 - i; R.ev.push_back(e);
    }
    const AEvent *hl = nullptr, *hh = nullptr, *fl = nullptr; int flAt = -1;   // the newest holding low / high, the newest failure
    for (const AEvent& e : R.ev) { if (!e.failed) { if (e.side > 0) hl = &e; else hh = &e; } else if (e.failAt >= flAt) { fl = &e; flAt = e.failAt; } }
    const int holdAt = std::max(hl ? hl->i : -1, hh ? hh->i : -1);
    auto what = [&](const AEvent& e) { return std::to_string(e.lots) + (e.side > 0 ? " lots sold there, " : " lots bought there, ") + ratioText((double)e.x1000 / 1000.0) + " normal."; };
    auto since = [&](const AEvent& e) { return e.held <= 0 ? std::string(" On the last candle.") : " Held " + plural(e.held, "candle", "candles") + (e.tested > 0 ? ", tested " + plural(e.tested, "time", "times") : std::string()) + "."; };
    auto at = [&](const AEvent& e) { return pxText(e.px, tick) + (e.lv.empty() ? std::string() : " (" + e.lv + ")"); };
    auto swp = [&](const AEvent& e) { return e.sw.empty() ? std::string() : "At the " + e.sw + " sweep. "; };
    if (!hl && !hh && !fl) { R.verdict = 0; R.head = "Quiet"; R.why = "No absorption in the last " + std::to_string(ARD_WIN) + " candles."; R.watch = "Nothing to act on."; return R; }
    if (fl && flAt > holdAt) {                                            // the newest thing is a failure: the side that absorbed is trapped
        const AEvent& e = *fl; R.verdict = e.side > 0 ? -1 : 1;
        R.head = std::string(e.side > 0 ? "Bearish ? - the absorbed low at " : "Bullish ? - the absorbed high at ") + at(e) + " failed";
        { const int ago = n - 1 - e.failAt; R.why = swp(e) + what(e) + (e.side > 0 ? " Closed below it " : " Closed above it ") + (ago <= 0 ? std::string("on the last candle.") : plural(ago, "candle", "candles") + " ago."); }
        const AEvent* o = e.side > 0 ? hh : hl;                          // an absorbed price of the same direction that still holds
        R.watch = std::string("Watch: a close back ") + (e.side > 0 ? "above " : "below ") + pxText(e.px, tick) + " = the failure is undone." + (o ? std::string(e.side > 0 ? " The absorbed high at " : " The absorbed low at ") + pxText(o->px, tick) + " still holds." : std::string());
        return R; }
    if (hl && hh) {                                                       // both hold: price is between them
        R.verdict = 2; R.head = "Range ? - absorbed low " + at(*hl) + " / absorbed high " + at(*hh);
        const AEvent& nw = hl->i >= hh->i ? *hl : *hh; const AEvent& od = hl->i >= hh->i ? *hh : *hl;
        R.why = std::string(nw.side > 0 ? "Newest, the low: " : "Newest, the high: ") + swp(nw) + what(nw) + since(nw) + (od.side > 0 ? " The low: " : " The high: ") + std::to_string(od.lots) + " lots, " + ratioText((double)od.x1000 / 1000.0) + ".";
        R.watch = "Watch: a close above " + pxText(hh->px, tick) + " = bullish; a close below " + pxText(hl->px, tick) + " = bearish."; return R; }
    const AEvent& e = hl ? *hl : *hh; R.verdict = e.side;
    R.head = std::string(e.side > 0 ? "Bullish ? - above the absorbed low at " : "Bearish ? - below the absorbed high at ") + at(e);
    R.why = swp(e) + what(e) + since(e);
    { int k = 0; long long last = 0; bool mono = true; for (const AEvent& q : R.ev) { if (q.failed || q.side != e.side) continue; if (k > 0 && !(e.side > 0 ? q.px > last : q.px < last)) mono = false; last = q.px; ++k; }
      if (k >= 2 && mono) R.why += e.side > 0 ? " Absorbed lows are rising." : " Absorbed highs are falling."; }
    R.watch = std::string("Watch: a close ") + (e.side > 0 ? "below " : "above ") + pxText(e.px, tick) + (e.side > 0 ? " = failed, bearish." : " = failed, bullish.");
    return R;
}
// word-wrap for the read: whole words, lines no wider than width px (tw = text width); at most maxLines (the last one ends in "..." when cut)
template <class TW> std::vector<std::string> wrapRead(const std::string& s, int width, int maxLines, TW tw) {
    std::vector<std::string> L; if (maxLines <= 0 || width <= 0 || s.empty()) return L; std::string line, w; bool cut = false;
    auto flush = [&](const std::string& word) { if (cut) return; const std::string t = line.empty() ? word : line + " " + word;
        if (!line.empty() && tw(t) > width) { if ((int)L.size() + 1 >= maxLines) { cut = true; return; } L.push_back(line); line = word; } else line = t; };
    for (size_t i = 0; i <= s.size(); ++i) { if (i == s.size() || s[i] == ' ') { if (!w.empty()) flush(w); w.clear(); } else w += s[i]; }
    if (cut) { while (!line.empty() && tw(line + "...") > width) { const size_t sp = line.find_last_of(' '); if (sp == std::string::npos) break; line.erase(sp); } line += "..."; }
    if (!line.empty()) L.push_back(line);
    return L;
}
// ---------------------------------------------------------------------------------------------------------------------------------
// (1.5.0) THE ABS READ (Rassul 2026-11-24, abs-badge-mockup.html v15, "ok. build this"). lsRevRead decides the Abs at its candle's close
// (LevelMarks 1.18.0: effort without result at the first test of a Bk level) and writes "stamp|Abs|dir|<pos>|<level px>|<level>" and, a
// candle later, "stamp|Ex|dir" into RevRead\wy-<MKT>-<spb>.txt. This pane adds what only the order feed knows and says it in four lines:
//   Attack   - who pushed into the absorbed end quarter, how many lots, the biggest single order (P = biggest of its side in P candles) and
//              what it hit (a sweep of N ticks / an iceberg / an order showing on the book)
//   Absorbed - how the other side took it: the most showing at those prices, all the lots taken, lots put back (added - pulled), iceberg lots
//   Result   - volume / range vs the 20 candles before, which half it closed in; the attackers' last minute vs their first two (Ex? at 0.5x or
//              less: 19% exhausted vs 11%, ABS-EXHAUSTION 2026-11-24)
//   Ex/Next  - the candle after: the attackers' lots vs the Abs candle, and the book (defenders rebuilt / pulling, the attackers' own
//              resting side pulled / stacked; thirds of 1,295 absorption candles: rebuilt > +15.1%, pulling < +7.2%, pulled < +5.9%,
//              stacked > +14.6% - rebuilt AND pulled held 81% vs 59%)
// Numbers only from candles this pane saw whole (the feed has no history); none = the line says so.
struct AbsLine { long long t = 0; int dir = 0; std::string pos, lv; long long lvPx = 0; bool ex = false; long long exT = 0; };
inline std::vector<AbsLine> parseAbs(const std::string& txt, const std::string& mkt, int spb) {
    std::vector<AbsLine> out; const std::vector<std::string> L = lines(txt, true); if (L.empty()) return out;
    const std::vector<std::string> h = split(L[0], '|'); long long s = 0;
    if (h.size() < 4 || h[0] != "WY" || h[1] != "1" || h[2] != mkt || !toLL(h[3], s) || s != spb) return out;
    std::map<long long, AbsLine> m; std::map<long long, int> ex;
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i], '|');
        long long t = 0, d = 0; if (c.size() < 3 || !toLL(c[0], t) || t <= 0 || !toLL(c[2], d) || (d != 1 && d != -1)) continue;
        if (c[1] == "Ex" && c.size() == 3) { ex[t] = (int)d; continue; }
        if (c[1] != "Abs" || c.size() != 6) continue;
        AbsLine a; a.t = t; a.dir = (int)d; a.pos = c[3].substr(0, 12); long long px = 0; if (!toLL(c[4], px) || px <= 0) continue; a.lvPx = px; a.lv = c[5].substr(0, 12); m[t] = a; }
    for (std::map<long long, AbsLine>::iterator it = m.begin(); it != m.end(); ++it) {   // its Ex = the first Ex line of its side after it, before the next Abs
        std::map<long long, int>::const_iterator e = ex.upper_bound(it->first); std::map<long long, AbsLine>::const_iterator nx = std::next(it);
        if (e != ex.end() && e->second == it->second.dir && (nx == m.end() || e->first <= nx->first) && e->first - it->first <= 2LL * spb) { it->second.ex = true; it->second.exT = e->first; }
        out.push_back(it->second); }
    return out;
}
struct AbsCalc { bool feed = false; int p = -1; bool plus = false; bool exq = false; long long attQ = 0, defShow = 0, putBack = 0, iceLots = 0, iceN = 0, hitShow = 0, hitRef = 0, early = 0, late = 0; BigPr big; };
inline AbsCalc absCalc(const Live& Lv, long long key, int dir) {
    AbsCalc c; const Sum* s = Lv.find(key); if (!s || !s->complete || s->thin || s->overflow) return c;
    const int a = dir > 0 ? 0 : 1, d = dir > 0 ? 0 : 1;                 // attackers: sellers [0] after a bullish Abs; the resting side they hit: bids [0]
    const long long lo = s->loPx, hi = s->hiPx, L = hi - lo + 1, z = L >= 4 ? zoneOf(L) : 0; if (z <= 0) return c;
    c.feed = true; c.early = s->early[a]; c.late = s->late[a]; c.exq = c.early > 0 && c.late * 4 <= c.early;
    for (std::map<long long, Cell>::const_iterator it = s->cells.begin(); it != s->cells.end(); ++it) { const bool in = dir > 0 ? it->first < lo + z : it->first > hi - z; if (in) c.attQ += dir > 0 ? it->second.sell : it->second.buy; }
    for (std::map<long long, BookCell>::const_iterator it = s->book.begin(); it != s->book.end(); ++it) { const bool in = dir > 0 ? it->first < lo + z : it->first > hi - z; if (!in) continue;
        c.defShow += it->second.shownMax[d]; c.putBack += it->second.add[d] - it->second.pull[d]; c.iceLots += it->second.refillLots[d]; c.iceN += it->second.refills[d]; }
    c.big = s->qBig[a];
    if (c.big.lots > 0) {
        if (c.big.lo == c.big.hi) { const std::map<long long, BookCell>::const_iterator b = s->book.find(c.big.lo); if (b != s->book.end()) { c.hitShow = b->second.shownMax[d]; c.hitRef = b->second.refills[d]; } }
        c.p = 0; bool stop = false;
        for (std::deque<Live::Hist>::const_reverse_iterator it = Lv.hist.rbegin(); it != Lv.hist.rend(); ++it) { if (it->key >= key) continue;
            if (it->big[a] < 0) { c.plus = true; stop = true; break; }
            if (it->big[a] >= c.big.lots) { stop = true; break; }
            ++c.p; }
        if (!stop) c.plus = true;
    } else c.p = 0;
    return c;
}
struct NextCalc { bool feed = false; long long att = 0, attAbs = 0; double dNet = 0, rNet = 0; bool dOk = false, rOk = false; };
inline NextCalc nextCalc(const Live& Lv, long long keyAbs, long long keyNext, int dir) {
    NextCalc n; const Sum* s0 = Lv.find(keyAbs); const Sum* s1 = Lv.find(keyNext); if (!s0 || !s1 || !s0->complete || !s1->complete) return n;
    const int d = dir > 0 ? 0 : 1, r = 1 - d;
    for (std::map<long long, Cell>::const_iterator it = s0->cells.begin(); it != s0->cells.end(); ++it) n.attAbs += dir > 0 ? it->second.sell : it->second.buy;
    for (std::map<long long, Cell>::const_iterator it = s1->cells.begin(); it != s1->cells.end(); ++it) n.att += dir > 0 ? it->second.sell : it->second.buy;
    long long ad = 0, pd = 0, ar = 0, pr = 0;
    for (std::map<long long, BookCell>::const_iterator it = s1->book.begin(); it != s1->book.end(); ++it) { ad += it->second.add[d]; pd += it->second.pull[d]; ar += it->second.add[r]; pr += it->second.pull[r]; }
    if (ad > 0) { n.dNet = (double)(ad - pd) / (double)ad; n.dOk = true; } if (ar > 0) { n.rNet = (double)(ar - pr) / (double)ar; n.rOk = true; }
    n.feed = n.attAbs > 0; return n;
}
static const double BOOK_REBUILT = 0.151, BOOK_PULLING = 0.072, ATT_PULLED = 0.059, ATT_STACKED = 0.146;
inline std::string pctText(double x) { char b[16]; std::snprintf(b, sizeof b, "%+d%%", (int)std::lround(x * 100.0)); return b; }
inline std::string xText(double x) { char b[16]; std::snprintf(b, sizeof b, "%.1fx", x < 0 ? 0.0 : x > 99 ? 99.0 : x); return b; }
inline std::string xText2(double x) { char b[16]; std::snprintf(b, sizeof b, "%.2fx", x < 0 ? 0.0 : x > 99 ? 99.0 : x); return b; }
struct AbsText { std::string title; int dir = 0; std::vector<std::pair<std::string, std::string> > rows; };
// volr / rngr < 0 = not known; nextT = "" while the candle after is still forming
inline AbsText absText(const AbsLine& a, const AbsCalc& c, const NextCalc* nx, const std::string& when, const std::string& nextWhen, double volr, double rngr, double tick) {
    AbsText o; o.dir = a.dir; const bool bull = a.dir > 0;
    const std::string Att = bull ? "Sellers" : "Buyers", att = bull ? "sellers" : "buyers", Def = bull ? "Bids" : "Offers", def = bull ? "bids" : "offers";
    const std::string end = bull ? "bottom" : "top", ord = bull ? "sell" : "buy", hitWord = bull ? "bid" : "offer", ownRest = bull ? "offers" : "bids";
    const std::string pos = a.pos.empty() ? std::string() : " (" + a.pos + ")";
    o.title = (when.empty() ? std::string() : when + "  ") + (bull ? "Bullish ? - absorbed at " : "Bearish ? - absorbed at ") + a.lv + " " + pxText(a.lvPx, tick);
    std::string at;
    if (!c.feed) at = Att + (bull ? " hit" : " lifted") + " the " + end + " 25%" + pos + " - no order feed for this candle.";
    else if (c.big.lots > 0 && c.p >= 5) {
        std::string hit;
        if (c.big.lo != c.big.hi) hit = "it swept " + std::to_string(c.big.hi - c.big.lo + 1) + " ticks to " + pxText(bull ? c.big.lo : c.big.hi, tick);
        else if (c.hitRef > 0) hit = "it hit an iceberg at " + pxText(c.big.lo, tick) + " (refilled " + std::to_string(c.hitRef) + "x)";
        else if (c.hitShow > 0) hit = "it hit a " + std::to_string(c.hitShow) + "-lot " + hitWord + " at " + pxText(c.big.lo, tick) + " that was showing";
        else hit = "it hit the " + hitWord + " at " + pxText(c.big.lo, tick);
        at = Att + (bull ? " hit " : " lifted ") + std::to_string(c.attQ) + " lots into the " + end + " 25%" + pos + ", led by one " + std::to_string(c.big.lots) + "-lot " + ord
           + " (biggest in " + std::to_string(c.p) + (c.plus ? "+" : "") + " bars) - " + hit + ".";
    } else at = Att + (bull ? " hit " : " lifted ") + std::to_string(c.attQ) + " lots into the " + end + " 25%" + pos + " in small orders (biggest " + std::to_string(c.big.lots) + " - no big print).";
    o.rows.push_back(std::make_pair("Attack", at));
    if (c.feed) {
        std::string ab = Def + " showing up to " + std::to_string(c.defShow) + " lots took all " + std::to_string(c.attQ);
        ab += c.putBack >= 0 ? " and put back " + std::to_string(c.putBack) : " and pulled " + std::to_string(-c.putBack);
        if (c.iceLots > 0) ab += " (iceberg " + std::to_string(c.iceLots) + ")";
        o.rows.push_back(std::make_pair("Absorbed", ab + "."));
    }
    std::string rs; if (volr >= 0 && rngr >= 0) rs = "Volume " + xText(volr) + ", range " + xText(rngr) + ", closed in the " + (bull ? "upper" : "lower") + " half"; else rs = std::string("Closed in the ") + (bull ? "upper" : "lower") + " half";
    if (c.feed && c.early > 0) rs += "; last minute " + att + " " + xText(2.0 * (double)c.late / (double)c.early) + (c.exq ? " - Ex?" : " - no Ex");
    o.rows.push_back(std::make_pair("Result", rs + "."));
    if (!nextWhen.empty()) {
        std::string nt;
        if (nx && nx->feed) {
            const double r = (double)nx->att / (double)nx->attAbs;
            nt = nextWhen + " - " + att + (a.ex ? (r <= 0.5 ? " traded only " : " traded ") : " came back with ") + std::to_string(nx->att) + " lots (" + xText2(r) + ")" + (a.ex ? " and the candle closed further away" : "");
            bool rebuilt = false, pulled = false, pulling = false;
            if (nx->dOk) { if (nx->dNet > BOOK_REBUILT) { rebuilt = true; nt += "; " + def + " rebuilt (net " + pctText(nx->dNet) + ")"; } else if (nx->dNet < BOOK_PULLING) { pulling = true; nt += "; " + def + " thinning (net " + pctText(nx->dNet) + ")"; } else nt += "; " + def + " steady (net " + pctText(nx->dNet) + ")"; }
            if (nx->rOk) { if (nx->rNet < ATT_PULLED) { pulled = true; nt += ", " + att + " pulled their " + ownRest + " (net " + pctText(nx->rNet) + ")"; } else if (nx->rNet > ATT_STACKED) nt += ", " + att + " stacked " + ownRest + " (net " + pctText(nx->rNet) + ")"; }
            if (a.ex) nt += std::string(" - ") + att + " exhausted" + (rebuilt && pulled ? std::string(", ") + (bull ? "buyers" : "sellers") + " in control" : std::string());
            else nt += pulling ? " - no exhaustion, warning" : " - no exhaustion";
        } else nt = nextWhen + (a.ex ? " - " + att + " traded 0.5x or less and the candle closed further away - " + att + " exhausted" : " - no exhaustion");
        o.rows.push_back(std::make_pair(a.ex ? "Ex" : "Next", nt + "."));
    }
    return o;
}

// (1.5.0) the read's numbers, kept once decided (TurnPane\absread-<MKT>-<spb>.txt) so a restart draws the same read (the feed has no history):
//   "A|stamp|feed|p|plus|exq|attQ|defShow|putBack|iceLots|iceN|hitShow|hitRef|early|late|bigLots|bigLo|bigHi" and "N|stamp|feed|att|attAbs|dOk|dNet x1e6|rOk|rNet x1e6"
inline std::string absRecText(const std::map<long long, AbsCalc>& A, const std::map<long long, NextCalc>& N, const std::string& mkt, int spb) {
    std::string t = "ABR|1|" + mkt + "|" + std::to_string(spb) + "\n"; char b[400];
    size_t skip = A.size() > HIST_P ? A.size() - HIST_P : 0;
    for (std::map<long long, AbsCalc>::const_iterator it = A.begin(); it != A.end(); ++it) { if (skip) { --skip; continue; } const AbsCalc& c = it->second;
        std::snprintf(b, sizeof b, "A|%lld|%d|%d|%d|%d|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld\n", it->first, c.feed ? 1 : 0, c.p, c.plus ? 1 : 0, c.exq ? 1 : 0, c.attQ, c.defShow, c.putBack, c.iceLots, c.iceN, c.hitShow, c.hitRef, c.early, c.late, c.big.lots, c.big.lo, c.big.hi); t += b; }
    skip = N.size() > HIST_P ? N.size() - HIST_P : 0;
    for (std::map<long long, NextCalc>::const_iterator it = N.begin(); it != N.end(); ++it) { if (skip) { --skip; continue; } const NextCalc& n = it->second;
        std::snprintf(b, sizeof b, "N|%lld|%d|%lld|%lld|%d|%lld|%d|%lld\n", it->first, n.feed ? 1 : 0, n.att, n.attAbs, n.dOk ? 1 : 0, (long long)std::llround(n.dNet * 1e6), n.rOk ? 1 : 0, (long long)std::llround(n.rNet * 1e6)); t += b; }
    return t;
}
inline bool parseAbsRec(const std::string& txt, const std::string& mkt, int spb, std::map<long long, AbsCalc>& A, std::map<long long, NextCalc>& N) {
    A.clear(); N.clear(); const std::vector<std::string> L = lines(txt, true); if (L.empty()) return false;
    const std::vector<std::string> h = split(L[0], '|'); long long s = 0; if (h.size() < 4 || h[0] != "ABR" || h[1] != "1" || h[2] != mkt || !toLL(h[3], s) || s != spb) return false;
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i], '|'); std::vector<long long> v;
        for (size_t k = 1; k < c.size(); ++k) { long long x = 0; if (!toLL(c[k], x)) { v.clear(); break; } v.push_back(x); }
        if (c[0] == "A" && v.size() == 17 && v[0] > 0) { AbsCalc a; a.feed = v[1] != 0; a.p = (int)std::max(-1LL, std::min(100000LL, v[2])); a.plus = v[3] != 0; a.exq = v[4] != 0; a.attQ = v[5]; a.defShow = v[6]; a.putBack = v[7]; a.iceLots = v[8]; a.iceN = v[9]; a.hitShow = v[10]; a.hitRef = v[11]; a.early = v[12]; a.late = v[13]; a.big.lots = v[14]; a.big.lo = v[15]; a.big.hi = v[16]; A[v[0]] = a; }
        else if (c[0] == "N" && v.size() == 8 && v[0] > 0) { NextCalc n; n.feed = v[1] != 0; n.att = v[2]; n.attAbs = v[3]; n.dOk = v[4] != 0; n.dNet = (double)v[5] / 1e6; n.rOk = v[6] != 0; n.rNet = (double)v[7] / 1e6; if (n.feed && n.attAbs <= 0) n.feed = false; N[v[0]] = n; } }
    return true;
}

// (1.7.0) THE COMPACT READ (Rassul 2026-11-24 17:28-17:30 CT, turn-read-mockup.html, "build"). The candle under the mouse (else the last closed
// candle in view). The END that was pushed = the bigger of its two Absorption numbers (top = buyers, bottom = sellers); counted from 1.5.
//   heading  - time, who won, where: "Passive defense won - buyers absorbed at the top" (the candle closed OUTSIDE that end quarter) or
//              "Aggressor won - buyers got through" (closed inside it); an Abs of lsRevRead on the candle adds its level. Colour = the side that
//              won (passive at the top = red, at the bottom = green; aggressor buyers = green, sellers = red).
//   Attack   - the attackers' lots in that quarter, the Absorption number, the biggest single order (a sweep: how many ticks)
//   Absorbed / Not absorbed - what the resting side there did: most showing, put back or pulled, refills (icebergs)
// Research VERDICT 2026-11-24 (12,406 RTH candles 1.5x+, 7 markets): the next 5 candles stayed within 1 tick of the pushed extreme after
// passive defense 40% (both halves) vs aggressor 15%; price ran 1 ATR on past it 26% vs 42%. Refills did NOT change it (40% vs 41%):
// they describe, they do not grade. Under 1.5 at both ends = one quiet line.
static const long long CR_FROM1000 = 1500;
struct CText { std::string title; int win = 0; std::vector<std::pair<std::string, std::string> > rows; bool quiet = false; };
inline CText compactText(int dir, long long x1000, bool passive, const AbsCalc& c, const std::string& when, const std::string& lv, long long lvPx, double tick) {
    CText o; const bool bull = dir > 0;   // dir +1 = sellers pushed into the bottom, -1 = buyers into the top
    const std::string Att = bull ? "Sellers" : "Buyers", Def = bull ? "Bids" : "Offers", end = bull ? "bottom" : "top";
    o.win = passive ? (bull ? 1 : -1) : (bull ? -1 : 1);
    o.title = (when.empty() ? std::string() : when + "  ") + (passive ? std::string("Passive defense won - ") + (bull ? "sellers" : "buyers") + " absorbed at the " + end
                                                                       : std::string("Aggressor won - ") + (bull ? "sellers" : "buyers") + " got through the " + end);
    if (!lv.empty()) o.title += " (" + lv + (lvPx > 0 ? " " + pxText(lvPx, tick) : std::string()) + ")";
    const std::string xs = xText((double)x1000 / 1000.0);
    std::string at;
    if (!c.feed) at = Att + " into the " + end + " " + xs + " - no order feed for this candle.";
    else { at = Att + " " + std::to_string(c.attQ) + " lots into the " + end + " " + xs;
        if (c.big.lots > 0) { at += ", biggest print " + std::to_string(c.big.lots); if (c.big.lo != c.big.hi) at += " (swept " + std::to_string(c.big.hi - c.big.lo + 1) + " ticks)"; }
        else { at += ", no big print"; }
        at += "."; }
    o.rows.push_back(std::make_pair("Attack", at));
    std::string ab;
    if (passive) {
        if (!c.feed) ab = std::string("closed back out of the ") + end + " quarter; the " + end + " held.";
        else { ab = Def + " showing up to " + std::to_string(c.defShow) + " took them and " + (c.putBack >= 0 ? "put back " + std::to_string(c.putBack) : "pulled " + std::to_string(-c.putBack));
            if (c.iceN > 0) { ab += " (refilled " + std::to_string(c.iceN) + "x)"; }
            ab += "; the " + end + " held."; }
        o.rows.push_back(std::make_pair("Absorbed", ab));
    } else {
        if (!c.feed) ab = std::string("closed in the ") + end + " quarter.";
        else { ab = c.putBack < 0 ? Def + " pulled " + std::to_string(-c.putBack) + " lots ahead of them" : Def + " showing up to " + std::to_string(c.defShow) + " were taken";
            ab += "; closed in the " + end + " quarter."; }
        o.rows.push_back(std::make_pair("Not absorbed", ab));
    }
    return o;
}
inline CText quietText(const std::string& when, long long up1000, long long dn1000) {
    CText o; o.quiet = true; o.title = (when.empty() ? std::string() : when + "  ") + "Quiet - no push into either end";
    if (up1000 >= 0 || dn1000 >= 0) o.title += " (top " + (up1000 >= 0 ? xText((double)up1000 / 1000.0) : std::string("-")) + " / bottom " + (dn1000 >= 0 ? xText((double)dn1000 / 1000.0) : std::string("-")) + ")";
    return o;
}
// ---------------------------------------------------------------------------------------------------------------------------------
// (1.8.0) INITIATIVE - the counter side TAKING an end: buyers lifting the offer in the bottom quarter, sellers hitting the bid in the top one
// ---------------------------------------------------------------------------------------------------------------------------------
//   number = the counter side's heaviest single price in the end quarter / its average over the 20 candles before (as Absorption)
//   on     = number >= 1.5 AND those lots > the attackers' heaviest price there (buyers bigger than sellers at the low) AND the candle closed
//            outside that quarter. Colour = the side that attacked: buyers at the low green, sellers at the high red.
// Research 2026-11-24 (RTH candles at a new 10-candle low / high, 7 markets, decided at the close; went 1 ATR the turn's way before 1 tick past
// the extreme within 10 candles): initiative only 66% (n 711, halves 66 / 67), absorption only 61%, both 61%, ordinary 57%; better in all 7
// markets (ES weakest 61 vs 59). Initiative >= 1.5 with the attackers bigger there: 54% (no better than ordinary) - hence the "bigger" rule.
static const long long INI_FROM1000 = 1500, SWEEP_MIN = 4;
struct Ini { long long x1000 = -1, attX1000 = -1, lots = -1, att = -1, tick = 0; bool away = false; };
inline bool iniOn(const Ini& i) { return i.x1000 >= INI_FROM1000 && i.away && i.lots > i.att && i.lots > 0; }
inline Bar1 iniBar(const Ini& i) { Bar1 b = ratioBar(i.x1000, 0); b.on = b.show && iniOn(i); return b; }
// the end whose initiative is on: +1 the bottom (buyers), -1 the top (sellers), 0 = none; both on = the bigger number (ties: the bottom)
inline int iniEnd(const Ini& up, const Ini& dn) { const bool u = iniOn(up), d = iniOn(dn); if (!u && !d) return 0; if (u && d) return dn.x1000 >= up.x1000 ? 1 : -1; return d ? 1 : -1; }
// burst level per market (tenths): the 90th percentile of the burst against the prior hour's pace among initiative candles, research 2026-11-24
// (267 candles with the order-by-order trades; at it 83% vs 67%, n 30 - LEARNING, shown with "?")
inline long long burstLevel10(const std::string& mkt) {
    static const char* const M[] = { "CL", "ES", "EU", "6E", "GC", "HG", "NG", "NQ" }; static const long long V[] = { 86, 54, 121, 121, 104, 130, 95, 77 };
    for (int k = 0; k < 8; ++k) if (mkt == M[k]) return V[k];
    return 100;
}
struct IniCalc { bool feed = false; long long bigLots = 0, bigLo = 0, bigHi = 0, sweep = 0, burst = 0, burst1000 = -1; };
// end +1 = the bottom (buyers), -1 = the top (sellers); the pace = the side's lots per 5 s over the 20 candles before (10+ known, else no burst number)
inline IniCalc iniCalc(const Live& Lv, long long key, int end, int spb) {
    IniCalc c; const Sum* s = Lv.find(key); if (!s || !s->complete || s->thin || s->overflow || spb <= 0) return c;
    const int e = end > 0 ? 0 : 1, sd = end > 0 ? 1 : 0; const IniQ& q = s->iq[e];
    c.feed = true; c.bigLots = q.big.lots; c.bigLo = q.big.lo; c.bigHi = q.big.hi; c.sweep = q.sweep; c.burst = q.burst;
    long long sum = 0; int n = 0;
    for (std::deque<Live::Hist>::const_reverse_iterator it = Lv.hist.rbegin(); it != Lv.hist.rend() && n < 20; ++it) { if (it->key >= key) continue; if (it->lots[sd] < 0) continue; sum += it->lots[sd]; ++n; }
    if (n >= 10 && sum > 0) { const double pace = (double)sum / ((double)n * (double)spb / 5.0); if (pace > 0) c.burst1000 = x1000((double)c.burst / pace); }
    return c;
}
// the read: heading, Initiative, How (burst / sweep / biggest order), Against
inline CText iniText(int end, const Ini& in, const IniCalc& c, const std::string& when, double tick, const std::string& mkt) {
    CText o; const bool low = end > 0; o.win = low ? 1 : -1;
    const std::string who = low ? "buyers" : "sellers", other = low ? "sellers" : "buyers";
    o.title = (when.empty() ? std::string() : when + "  ") + "Initiative - " + who + (low ? " lifted the offer at the bottom" : " hit the bid at the top");
    o.rows.push_back(std::make_pair("Initiative", who + " " + std::to_string(in.lots) + " lots at " + pxText(in.tick, tick) + " (" + xText((double)in.x1000 / 1000.0) + ")"));
    if (!c.feed) o.rows.push_back(std::make_pair("How", std::string("no order feed for this candle")));
    else {
        const long long bl = burstLevel10(mkt); char lv[48]; std::snprintf(lv, sizeof lv, "%lld.%lldx", bl / 10, bl % 10);
        std::string h = c.burst1000 >= 0 ? "Burst " + xText((double)c.burst1000 / 1000.0) + " pace (" + lv + ") ?" : std::string("Burst - (no pace yet)");
        h += "  |  Swept " + std::to_string(c.sweep) + (c.sweep == 1 ? " price" : " prices") + " (" + std::to_string(SWEEP_MIN) + ") ?";
        if (c.bigLots > 0) h += "  |  Biggest " + std::to_string(c.bigLots);
        o.rows.push_back(std::make_pair("How", h));
    }
    std::string ag = other + " " + (in.att >= 0 ? std::to_string(in.att) : std::string("-")) + " lots there";
    if (in.attX1000 >= 0) ag += " (" + xText((double)in.attX1000 / 1000.0) + ")";
    ag += " - " + who + " won the " + (low ? "low" : "high");
    o.rows.push_back(std::make_pair("Against", ag));
    return o;
}
// the end that was pushed: +1 bottom (sellers), -1 top (buyers), 0 = neither 1.5+; ties go to the top
inline int pushedEnd(long long up1000, long long dn1000) { const long long u = up1000 >= CR_FROM1000 ? up1000 : -1, d = dn1000 >= CR_FROM1000 ? dn1000 : -1; if (u < 0 && d < 0) return 0; return u >= d ? -1 : 1; }
// (1.7.0) the order-book numbers of every closed candle this pane read whole, both ends, kept once decided (TurnPane\cread-<MKT>-<spb>.txt) so
// the read of an older candle (or after a restart) is the same: "C|stamp|dir|feed|attQ|defShow|putBack|iceLots|iceN|bigLots|bigLo|bigHi"
static const size_t CR_KEEP = 900;
inline std::string crecText(const std::map<std::pair<long long, int>, AbsCalc>& m, const std::string& mkt, int spb) {
    std::string t = "CRD|1|" + mkt + "|" + std::to_string(spb) + "|" + VERSION + "\n"; size_t skip = m.size() > CR_KEEP ? m.size() - CR_KEEP : 0;
    for (const auto& kv : m) { if (skip) { --skip; continue; } const AbsCalc& a = kv.second;
        t += "C|" + std::to_string(kv.first.first) + "|" + std::to_string(kv.first.second) + "|" + (a.feed ? "1" : "0") + "|" + std::to_string(a.attQ) + "|" + std::to_string(a.defShow) + "|" + std::to_string(a.putBack)
           + "|" + std::to_string(a.iceLots) + "|" + std::to_string(a.iceN) + "|" + std::to_string(a.big.lots) + "|" + std::to_string(a.big.lo) + "|" + std::to_string(a.big.hi) + "\n"; }
    return t;
}
// (1.8.0) + the Initiative numbers: "N|stamp|end|feed|bigLots|bigLo|bigHi|sweep|burst|burst1000" (older readers skip the N lines)
inline std::string crecText(const std::map<std::pair<long long, int>, AbsCalc>& m, const std::map<std::pair<long long, int>, IniCalc>& n, const std::string& mkt, int spb) {
    std::string t = crecText(m, mkt, spb); size_t skip = n.size() > CR_KEEP ? n.size() - CR_KEEP : 0;
    for (const auto& kv : n) { if (skip) { --skip; continue; } const IniCalc& a = kv.second;
        t += "N|" + std::to_string(kv.first.first) + "|" + std::to_string(kv.first.second) + "|" + (a.feed ? "1" : "0") + "|" + std::to_string(a.bigLots) + "|" + std::to_string(a.bigLo) + "|" + std::to_string(a.bigHi)
           + "|" + std::to_string(a.sweep) + "|" + std::to_string(a.burst) + "|" + std::to_string(a.burst1000) + "\n"; }
    return t;
}
inline bool parseCrec(const std::string& txt, const std::string& mkt, int spb, std::map<std::pair<long long, int>, AbsCalc>& out) {
    out.clear(); const std::vector<std::string> L = lines(txt, true); if (L.empty()) return false;
    const std::vector<std::string> h = split(L[0], '|'); long long s = 0; if (h.size() < 4 || h[0] != "CRD" || h[1] != "1" || h[2] != mkt || !toLL(h[3], s) || s != spb) return false;
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i], '|'); if (c.size() != 12 || c[0] != "C") continue; std::vector<long long> v;
        for (size_t k = 1; k < c.size(); ++k) { long long x = 0; if (!toLL(c[k], x)) { v.clear(); break; } v.push_back(x); }
        if (v.size() != 11 || v[0] <= 0 || (v[1] != 1 && v[1] != -1)) continue;
        AbsCalc a; a.feed = v[2] != 0; a.attQ = v[3]; a.defShow = v[4]; a.putBack = v[5]; a.iceLots = v[6]; a.iceN = v[7]; a.big.lots = v[8]; a.big.lo = v[9]; a.big.hi = v[10]; out[std::make_pair(v[0], (int)v[1])] = a; }
    return true;
}
inline bool parseCrec(const std::string& txt, const std::string& mkt, int spb, std::map<std::pair<long long, int>, AbsCalc>& out, std::map<std::pair<long long, int>, IniCalc>& nout) {
    nout.clear(); if (!parseCrec(txt, mkt, spb, out)) return false;
    const std::vector<std::string> L = lines(txt, true);
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i], '|'); if (c.size() != 10 || c[0] != "N") continue; std::vector<long long> v;
        for (size_t k = 1; k < c.size(); ++k) { long long x = 0; if (!toLL(c[k], x)) { v.clear(); break; } v.push_back(x); }
        if (v.size() != 9 || v[0] <= 0 || (v[1] != 1 && v[1] != -1) || v[3] < 0 || v[6] < 0 || v[7] < 0 || v[8] < -1) continue;
        IniCalc a; a.feed = v[2] != 0; a.bigLots = v[3]; a.bigLo = v[4]; a.bigHi = v[5]; a.sweep = v[6]; a.burst = v[7]; a.burst1000 = v[8]; nout[std::make_pair(v[0], (int)v[1])] = a; }
    return true;
}
// (1.5.0) lsTurn's line per Abs candle for lsRevReadMarks (RevRead\turnabs-<MKT>-<spb>.txt): "stamp|dir|P|plus|exq" (P -1 = not known)
struct TabRec { int dir = 0, p = -1; bool plus = false, exq = false; };
inline std::string tabFileText(const std::map<long long, TabRec>& m, const std::string& mkt, int spb) {
    std::string t = "TAB|1|" + mkt + "|" + std::to_string(spb) + "|" + VERSION + "\n"; size_t skip = m.size() > HIST_P ? m.size() - HIST_P : 0;
    for (std::map<long long, TabRec>::const_iterator it = m.begin(); it != m.end(); ++it) { if (skip) { --skip; continue; }
        t += std::to_string(it->first) + "|" + std::to_string(it->second.dir) + "|" + std::to_string(it->second.p) + "|" + (it->second.plus ? "1" : "0") + "|" + (it->second.exq ? "1" : "0") + "\n"; }
    return t;
}
inline bool parseTabFile(const std::string& txt, const std::string& mkt, int spb, std::map<long long, TabRec>& out) {
    out.clear(); const std::vector<std::string> L = lines(txt, true); if (L.empty()) return false;
    const std::vector<std::string> h = split(L[0], '|'); long long s = 0; if (h.size() < 4 || h[0] != "TAB" || h[1] != "1" || h[2] != mkt || !toLL(h[3], s) || s != spb) return false;
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i], '|'); long long t = 0, d = 0, p = 0;
        if (c.size() != 5 || !toLL(c[0], t) || t <= 0 || !toLL(c[1], d) || (d != 1 && d != -1) || !toLL(c[2], p) || p < -1 || p > 100000 || (c[3] != "0" && c[3] != "1") || (c[4] != "0" && c[4] != "1")) continue;
        TabRec r; r.dir = (int)d; r.p = (int)p; r.plus = c[3] == "1"; r.exq = c[4] == "1"; out[t] = r; }
    return true;
}

// (1.6.0) THE EX? OF EVERY CANDLE for the Tst badge's 3rd line (LevelMarks 1.19.0 tstL3, lsRevReadMarks 0.28.0): the attackers' last minute fell to
// 0.5x or less of their first 2 minutes' pace = late x 4 <= early (the same rule as the Abs badge's Ex?, research TST-REVERSE: E with or without
// absorption). One line per closed candle this pane read whole: "stamp|sellers|buyers" (1 = Ex?, 0 = not); a candle it did not see whole has no
// line (the Tst then decides E from the next candle only). Header "TEQ|1|<MKT>|<spb>|<ver>"; the newest HIST_P candles are kept.
inline std::pair<int, int> exqOf(const Sum& s) {
    return std::make_pair(s.early[0] > 0 && s.late[0] * 4 <= s.early[0] ? 1 : 0, s.early[1] > 0 && s.late[1] * 4 <= s.early[1] ? 1 : 0); }
inline std::string teqFileText(const std::map<long long, std::pair<int, int> >& m, const std::string& mkt, int spb) {
    std::string t = "TEQ|1|" + mkt + "|" + std::to_string(spb) + "|" + VERSION + "\n"; size_t skip = m.size() > HIST_P ? m.size() - HIST_P : 0;
    for (std::map<long long, std::pair<int, int> >::const_iterator it = m.begin(); it != m.end(); ++it) { if (skip) { --skip; continue; }
        t += std::to_string(it->first) + "|" + std::to_string(it->second.first) + "|" + std::to_string(it->second.second) + "\n"; }
    return t;
}
inline bool parseTeqFile(const std::string& txt, const std::string& mkt, int spb, std::map<long long, std::pair<int, int> >& out) {
    out.clear(); const std::vector<std::string> L = lines(txt, true); if (L.empty()) return false;
    const std::vector<std::string> h = split(L[0], '|'); long long s = 0; if (h.size() < 4 || h[0] != "TEQ" || h[1] != "1" || h[2] != mkt || !toLL(h[3], s) || s != spb) return false;
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i], '|'); long long t = 0;
        if (c.size() != 3 || !toLL(c[0], t) || t <= 0 || (c[1] != "0" && c[1] != "1") || (c[2] != "0" && c[2] != "1")) continue;
        out[t] = std::make_pair(c[1] == "1" ? 1 : 0, c[2] == "1" ? 1 : 0); }
    return true;
}
inline const char* verdictText(int v, bool bestProven) {
    return v == V_UNCLEAR ? "UNCLEAR" : v == V_ASIDE ? "STAND ASIDE" : v == V_WAIT ? "WAIT" : v == V_BEST ? (bestProven ? "BEST ODDS" : "BEST ODDS ?") : "NO READ";
}
// prefix = which sweep ("PhrH 9:03 AM"); numbers = add the ratios to the words (when the bars are too narrow for labels)
inline Strip stripOf(const Read& r, bool bestProven, bool numbers, const std::string& prefix, const Abs& ab = Abs(), long long from10 = ABS_FROM10, const Ini* ini = nullptr) {
    Strip s;
    struct Add { Strip& s; void operator()(const std::string& t, int tone, bool bold) const { Seg g; g.text = t; g.tone = tone; g.bold = bold; s.segs.push_back(g); s.flat += t; } } add = { s };
    const std::string sep = " / ";
    if (!prefix.empty()) add(prefix + "   ", T_MUTED, false);
    if (r.stopX1000 >= 0) { const double x = (double)r.stopX1000 / 1000.0; const int k = stopClass(x);
        add(std::string(word4(k)) + " Stop Run" + (numbers ? " " + ratioText(x) : std::string()), k == 0 ? T_MUTED : T_TEXT, true); }
    else add("\x96 Stop Run", T_MUTED, true);
    add(sep, T_MUTED, false);
    if (ab.x1000 >= 0) { const double x = (double)ab.x1000 / 1000.0; const long long t = shown10(labelText(x)); const int k = absClass10(t, ab.away, from10);   // (1.3.0) Absorption, the turn's colour when it counts
        add(std::string(word4(k)) + " Absorption" + (numbers ? " " + ratioText(x) : std::string()), k == 0 ? T_MUTED : k == 3 ? T_DIR : T_TEXT, true); }
    else add("\x96 Absorption", T_MUTED, true);
    add(sep, T_MUTED, false);
    if (r.dem1e4 >= 0 && r.exh >= 0) {
        add(std::string(word4(r.exh)) + " Exhaustion" + (numbers ? " " + ratioText(exhBar((double)r.dem1e4 / 1e4)) : std::string()) + (r.verdict == V_UNCLEAR ? " ?" : ""), r.exh == 0 ? T_MUTED : T_DIR, true); }
    else add("\x96 Exhaustion", T_MUTED, true);
    add(sep, T_MUTED, false);
    if (ini) {   // (1.8.0) the new Initiative of the turn's end (the old Answer 0-3 is no longer drawn)
        if (ini->x1000 >= 0) { const double x = (double)ini->x1000 / 1000.0; const int k = iniOn(*ini) ? 3 : (ini->away && ini->lots > ini->att && x >= 1.0) ? 1 : 0;
            add(std::string(word4(k)) + " Initiative" + (numbers ? " " + ratioText(x) : std::string()), k == 0 ? T_MUTED : k == 3 ? T_DIR : T_TEXT, true); }
        else add("\x96 Initiative", T_MUTED, true); }
    else if (r.marks >= 0 && r.ansOk) add(std::string(word4(r.marks)) + " Initiative" + (numbers ? " " + std::to_string(r.marks) + "/3" : std::string()), r.marks == 0 ? T_MUTED : T_DIR, true);
    else add("\x96 Initiative", T_MUTED, true);
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

// ---------------------------------------------------------------------------------------------------------------------------------
// (1.0.1) THE CANDLE UNDER THE MOUSE and the ratio labels of narrow candles
// ---------------------------------------------------------------------------------------------------------------------------------
// the name of the named level at this tick ("" = not in the list any more)
inline std::string levelNameAt(const std::vector<Level>& lv, long long tick) {
    if (tick <= 0) return std::string();
    for (const Level& x : lv) if (x.tick == tick) return x.name;
    return std::string();
}
// which side of a candle at a named level the strip is about: -1 = the high it pushed into (bars up), 1 = the low (bars down), 0 = none.
// Both sides at a level: the side the mouse is on (above the axis = the high).
inline int candSide(const Cand& c, bool mouseAboveAxis) {
    if (c.hiL && c.loL) return mouseAboveAxis ? -1 : 1;
    return c.hiL ? -1 : c.loL ? 1 : 0;
}
// the strip of a candle at a named level that is not a sweep: what is drawn on it - its Stop Run and its Effort, nothing else (no
// Exhaustion, no Answer, no verdict: those are read on a sweep candle only). -1 = not known. prefix = "VAH 9:57 PM".
inline Strip stripOfCand(long long stopX1000, const Abs& ab, int side, bool numbers, const std::string& prefix, long long from10 = ABS_FROM10) {   // (1.3.0) side -1 = the candle's high (absorbed there = red), +1 = its low (green)
    Strip s;
    struct Add { Strip& s; void operator()(const std::string& t, int tone, bool bold) const { Seg g; g.text = t; g.tone = tone; g.bold = bold; s.segs.push_back(g); s.flat += t; } } add = { s };
    if (!prefix.empty()) add(prefix + "   ", T_MUTED, false);
    if (stopX1000 >= 0) { const double x = (double)stopX1000 / 1000.0; const int k = stopClass(x);
        add(std::string(word4(k)) + " Stop Run" + (numbers ? " " + ratioText(x) : std::string()), k == 0 ? T_MUTED : T_TEXT, true); }
    else add("\x96 Stop Run", T_MUTED, true);
    add(" / ", T_MUTED, false);
    if (ab.x1000 >= 0) { const double x = (double)ab.x1000 / 1000.0; const long long t = shown10(labelText(x)); const int k = absClass10(t, ab.away, from10);
        add(std::string(word4(k)) + " Absorption" + (numbers ? " " + ratioText(x) : std::string()), k == 0 ? T_MUTED : k == 3 ? (side < 0 ? T_SELL : T_BUY) : T_TEXT, true); }
    else add("\x96 Absorption", T_MUTED, true);
    s.pill = "\x96"; s.pillTone = T_MUTED; s.edgeTone = T_MUTED;
    return s;
}
// (1.2.0) WHAT THE PANE SHOWS besides Effort. lsFlexLevels\TurnPane\display.txt, written by hand / by Claude when Rassul asks for a piece:
//   TURNSHOW|1
//   stop|0            the Stop Run bar (left column, at a named level / on a sweep)
//   exhaustion|0      the Exhaustion bar (left column, the turn half of a sweep candle)
//   answer|0          the Answer bar (right column, the turn half of a sweep candle)
//   read|0            the read strip: level, the four words, the verdict, the odds pill (off = the strip tells the candle's two Efforts)
//   effort_from|1.5   an Effort bar is coloured when the number written on it is this or more (grey under it)
//   END
// No file = the defaults = Effort only, coloured from 1.5. The first line and END must be there (a half-written file is never used); a
// key that is missing keeps its default, an unknown key is ignored, a bad value = the defaults for everything + why.
static const size_t SHOW_MAX_BYTES = 4096;
struct Show {
    bool stop = true, exh = true, ans = true, read = true;                // (1.3.0) the four pieces are the default
    long long effFrom10 = ABS_FROM10;                                     // tenths: an Absorption bar is coloured from this number
    bool fromFile = false; std::string why = "no display file (the defaults)";
    bool twoCol() const { return stop || exh || ans; }                    // two columns a candle; else ONE centred Effort bar a side
};
inline Show parseShow(const std::string& txt) {
    Show d; if (txt.empty()) return d;
    if (txt.size() > SHOW_MAX_BYTES) { d.why = "display file too big (the defaults)"; return d; }
    const std::vector<std::string> L = lines(txt, false);
    if (L.empty() || L[0] != "TURNSHOW|1") { d.why = "display file header (the defaults)"; return d; }
    if (L.size() < 2 || L.back() != "END") { d.why = "display file not whole (the defaults)"; return d; }
    Show s;
    for (size_t i = 1; i + 1 < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], '|'); bool ok = true; bool* f = nullptr;
        if (c[0] == "stop") f = &s.stop; else if (c[0] == "exhaustion") f = &s.exh; else if (c[0] == "answer") f = &s.ans; else if (c[0] == "read") f = &s.read;
        if (f) { long long b = 0; ok = c.size() == 2 && toLL(c[1], b) && (b == 0 || b == 1); if (ok) *f = b == 1; }
        else if (c[0] == "absorption_from" || c[0] == "effort_from") { double x = 0; ok = c.size() == 2 && toD(c[1], x) && x >= 0 && x <= 99; if (ok) s.effFrom10 = (long long)std::llround(x * 10.0); }
        else continue;
        if (!ok) { d.why = "display line " + std::to_string(i + 1) + " garbled (the defaults)"; return d; }
    }
    s.fromFile = true; s.why = "ok"; return s;
}
inline std::string showText(const Show& s) {                             // the status file: "effort stop exhaustion answer read|effort coloured from 1.5"
    char b[56]; std::snprintf(b, sizeof b, "|absorption coloured from %lld.%lld", s.effFrom10 / 10, s.effFrom10 % 10);
    return std::string("absorption") + (s.stop ? " stop" : "") + (s.exh ? " exhaustion" : "") + (s.ans ? " initiative" : "") + (s.read ? " read" : "") + b;
}
inline bool effOn(const Bar1& b, long long from10) { return b.show && shown10(b.label) >= from10; }
inline int effClass10(long long t) { return t < 5 ? 0 : t < 10 ? 1 : t < 15 ? 2 : 3; }   // No / Light / Moderate / Heavy by the number written (tenths)
// (1.1.0) EFFORT ON EVERY CANDLE. The two Effort bars of a closed candle from its volume at price: up = the buyers' Effort at the top
// of the candle, dn = the sellers' Effort at its bottom (barRatios: per price level in the 25% at that end / per level in the rest).
// No bar where the candle has under 4 price levels or nothing traded in the rest. (1.2.0) Coloured when the number written on the bar is
// from10 tenths or more (15 = "1.5"), grey under it.
struct EffPair { Bar1 up, dn; };
inline EffPair effOf(const Bars& b, long long from10) {
    EffPair e;
    if (b.topOk && std::isfinite(b.top) && b.top >= 0) e.up = ratioBar(x1000(b.top), 0);
    if (b.botOk && std::isfinite(b.bot) && b.bot >= 0) e.dn = ratioBar(x1000(b.bot), 0);
    e.up.on = effOn(e.up, from10); e.dn.on = effOn(e.dn, from10);
    return e;
}
// (1.3.0) the strip of a plain candle: "<time>   High: <word> Absorption 8.6x / Low: <word> Absorption 4.3x" - what is drawn on it. The
// word and the colour follow the number written on the bar AND the close test: red at the high / green at the low when the number is
// from10 tenths or more and the candle closed outside that end quarter; "No" and grey when it closed in it. x1000 -1 = no bar (a dash).
inline Strip stripOfAbs(const Abs& up, const Abs& dn, bool numbers, const std::string& prefix, long long from10) {
    Strip s;
    struct Add { Strip& s; void operator()(const std::string& t, int tone, bool bold) const { Seg g; g.text = t; g.tone = tone; g.bold = bold; s.segs.push_back(g); s.flat += t; } } add = { s };
    if (!prefix.empty()) add(prefix + "   ", T_MUTED, false);
    if (up.x1000 >= 0) { const double x = (double)up.x1000 / 1000.0; const long long t = shown10(labelText(x)); const int k = absClass10(t, up.away, from10);
        add(std::string("High: ") + word4(k) + " Absorption" + (numbers ? " " + ratioText(x) : std::string()), k == 3 ? T_SELL : T_MUTED, true); }
    else add("High: \x96 Absorption", T_MUTED, true);
    add(" / ", T_MUTED, false);
    if (dn.x1000 >= 0) { const double x = (double)dn.x1000 / 1000.0; const long long t = shown10(labelText(x)); const int k = absClass10(t, dn.away, from10);
        add(std::string("Low: ") + word4(k) + " Absorption" + (numbers ? " " + ratioText(x) : std::string()), k == 3 ? T_BUY : T_MUTED, true); }
    else add("Low: \x96 Absorption", T_MUTED, true);
    s.pill = "\x96"; s.pillTone = T_MUTED; s.edgeTone = T_MUTED;
    return s;
}
// which strip a candle at a named level shows under the mouse: its level's side (candSide) while the mouse is on a half of the candle
// that has a level; (1.1.0) the mouse on the half with no level (only the other side's Effort bar is there) = the plain candle's strip
inline bool candPlainSide(const Cand& c, bool mouseAboveAxis) {
    if (c.hiL && c.loL) return false;
    if (c.hiL) return !mouseAboveAxis;
    if (c.loL) return mouseAboveAxis;
    return true;
}
// Ratio labels when a candle is too narrow for two labels side by side: each candle side offers ONE label (its tallest bar's ratio,
// centred on the candle). A label is kept when it touches no label already kept and no bar of another candle; the taller bar is asked
// first (then the newer candle), so of two neighbours the bigger keeps its label. Rectangles are [l, r) x [t, b).
struct LabBox { int l = 0, t = 0, r = 0, b = 0; };
struct LabCand { LabBox box; int h = 0, owner = 0; };                    // h = its bar's height in pixels; owner = which candle side it belongs to
struct BarBox { LabBox box; int owner = 0; };
inline bool boxesTouch(const LabBox& a, const LabBox& b, int padX) { return a.l < b.r + padX && b.l < a.r + padX && a.t < b.b && b.t < a.b; }
// of two bars on one candle side the taller carries the label; equal heights: the left one (Stop Run / Exhaustion)
inline int tallerOf(int hLeft, int hRight) { return hRight > hLeft ? 1 : 0; }
// (1.1.0) the same answer as the plain double loop (pickLabelsPlain, kept for the tests), found through columns of 64 px: with bars on
// every candle a pane asks about hundreds of labels against hundreds of bars on every draw.
inline std::vector<size_t> pickLabelsPlain(const std::vector<LabCand>& c, const std::vector<BarBox>& bars) {
    std::vector<size_t> order(c.size()); for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&c](size_t a, size_t b) { return c[a].h != c[b].h ? c[a].h > c[b].h : c[a].owner > c[b].owner; });
    std::vector<size_t> kept;
    for (size_t k = 0; k < order.size(); ++k) {
        const LabCand& x = c[order[k]]; bool ok = x.box.r > x.box.l && x.box.b > x.box.t;
        for (size_t j = 0; ok && j < kept.size(); ++j) if (boxesTouch(x.box, c[kept[j]].box, 1)) ok = false;
        for (size_t j = 0; ok && j < bars.size(); ++j) if (bars[j].owner != x.owner && boxesTouch(x.box, bars[j].box, 0)) ok = false;
        if (ok) kept.push_back(order[k]);
    }
    std::sort(kept.begin(), kept.end());
    return kept;
}
inline std::vector<size_t> pickLabels(const std::vector<LabCand>& c, const std::vector<BarBox>& bars) {
    if (c.size() * (bars.size() + c.size()) <= 4096) return pickLabelsPlain(c, bars);
    static const long long CELL = 64, MAX_CELLS = 65536;
    long long lo = 0, hi = 0; bool any = false;
    const auto span = [&](const LabBox& b) { const long long a = std::min(b.l, b.r) - 2LL, z = std::max(b.l, b.r) + 2LL; if (!any) { lo = a; hi = z; any = true; } else { lo = std::min(lo, a); hi = std::max(hi, z); } };
    for (const LabCand& x : c) span(x.box);
    for (const BarBox& b : bars) span(b.box);
    const long long cells = any ? (hi - lo) / CELL + 1 : 0;
    if (cells <= 0 || cells > MAX_CELLS) return pickLabelsPlain(c, bars);
    const auto cellOf = [&](long long v) { const long long k = (v - lo) / CELL; return (size_t)(k < 0 ? 0 : k >= cells ? cells - 1 : k); };
    std::vector<std::vector<size_t> > barAt((size_t)cells), labAt((size_t)cells);
    for (size_t j = 0; j < bars.size(); ++j) { const size_t a = cellOf(std::min(bars[j].box.l, bars[j].box.r) - 1LL), z = cellOf(std::max(bars[j].box.l, bars[j].box.r) + 1LL); for (size_t q = a; q <= z; ++q) barAt[q].push_back(j); }
    std::vector<size_t> order(c.size()); for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&c](size_t a, size_t b) { return c[a].h != c[b].h ? c[a].h > c[b].h : c[a].owner > c[b].owner; });
    std::vector<size_t> kept;
    for (size_t k = 0; k < order.size(); ++k) {
        const LabCand& x = c[order[k]]; bool ok = x.box.r > x.box.l && x.box.b > x.box.t; if (!ok) continue;
        const size_t a = cellOf(x.box.l - 2LL), z = cellOf(x.box.r + 2LL);
        for (size_t q = a; ok && q <= z; ++q) {
            for (size_t j = 0; ok && j < labAt[q].size(); ++j) if (boxesTouch(x.box, c[labAt[q][j]].box, 1)) ok = false;
            for (size_t j = 0; ok && j < barAt[q].size(); ++j) { const BarBox& b = bars[barAt[q][j]]; if (b.owner != x.owner && boxesTouch(x.box, b.box, 0)) ok = false; }
        }
        if (!ok) continue;
        kept.push_back(order[k]);
        const size_t ka = cellOf(x.box.l - 1LL), kz = cellOf(x.box.r + 1LL); for (size_t q = ka; q <= kz; ++q) labAt[q].push_back(order[k]);
    }
    std::sort(kept.begin(), kept.end());
    return kept;
}

}  // namespace turn
