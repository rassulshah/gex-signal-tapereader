// VRRead.h  --  the reads of the Volume and Range panes (lsVolRead / lsRangeRead 1.4.0). SDK-free.
// (1.0.0, 2026-11-17 Rassul 06:54-07:02 CT "now lets move to the the Volume", "for volume can you look ino the bar to see if there are large lots , stop
// run, etc", "ok, i like it. what about range", "ok, build all , make sure that as i move my mouse on different bars i get that bars / waves
// information"; mockups claude.ai/artifact/RLzNyNZo4rDHuJA2vKwdfY (Volume) and claude.ai/artifact/NdxMKEmswYJHTCjYKW57bi (Range))
// One candle (under the mouse, else the last closed one) and the candles before it:
//   line 1  the verdict + up to two reasons     line 2  the candle     lines 3-4  the two most important behaviours     then the numbers
// VOLUME looks inside the candle (lsBookFlow's record: the biggest market order each way, the levels one order swept, iceberg lots traded) and
// reads volume against range; RANGE reads the candle's shape (rejection, erased, strong close, narrowing, wide, no progress).
// Every verdict carries "?" - UNPROVEN: the cut-offs are starting values, scored nightly before the "?" may go.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include "PlainRead.h"     // (2026-11-17 21:17) the plain three-line read - toPlain() at the end

namespace vrd {

static const char* const VERSION = "1.3.0";   // (1.3.0, Rassul 2026-11-24 19:28-19:44 CT "i'll let you determine the display and the read", "i would read the volume, then the buy sell vol, then the absorbtion volume", "Go" on panes-read-mockup.html) volPlain(): the VOLUME read answers layers 1, 4 and 5 of his volume model only - how much (size), the biggest orders / sweeps, icebergs, and a neutral note - with NO bullish / bearish verdict (the Turn pane calls the winner; the buy / sell pane says who hit whom). The Range read is unchanged
// was static const char* const VERSION = "1.2.0";   // (1.2.0, 2026-11-17 21:17) plain() / toPlain(): the three-line read for the pane   // was "1.1.2";   // (1.1.2, his 11:18 CT screenshot) HG decimals, "big" candle at 1.5x, iceberg reason = hidden buyers / sellers (was "absorbed" next to "volume absorbed")
// was "1.1.1";   // (1.1.1, his 09:32 CT screenshot: "narrowing" was the verdict but the sentence under it was about the 9:21 candle) the sentences behind the verdict come first, the side notes after   // (1.1.0, 2026-11-17 Rassul 08:35-08:44 CT "a split with the left having a verdict and human readable explanation and the right side to have the data points": Out::ex = the plain-English sentences, most important first)
static const int NORM_N = 20, NORM_MIN = 5, EXT_N = 10;
static const double BIG_X = 2.0;        // volume / range "big": 2x the median of the 20 candles before
static const double SMALL_X = 0.5;      // "drying up": under half of it
static const double SWEEP_LV = 3;       // a sweep: one market order took 3 price levels or more
static const double ONE_SIDED = 2.0;    // the biggest order one way is 2x the biggest the other way
static const double ICE_SHARE = 0.04;   // hidden orders matter when their traded lots are 4% of the candle's volume or more
static const double WICK_SHARE = 1.0 / 3.0;

struct Cd { double o = 0, h = 0, l = 0, c = 0, vol = 0; std::string when; bool green() const { return c >= o; } double rng() const { return h - l; } };
struct Inside { bool ok = false; long long bigBuy = 0, bigSell = 0; int swBuy = 0, swSell = 0; long long hidSell = 0, hidBuy = 0; };   // hidSell = iceberg OFFERS traded (they filled the buying)
struct Ctx { std::vector<Cd> c;            // oldest -> newest; the LAST one is the candle the read is about
             int cntAll = 0, cntSame = 0;  // bars in a row before it with a smaller value: all / its own colour (the pane's own count)
             double atr = 0; int cx = 0;   // Wilder ATR(13) of the candle before; cx: +1 a climax at a pivot LOW on it or the candle before, -1 at a pivot HIGH
             Inside in; int dec = 2; };
struct Row { std::string lab, a, b; int ca = 0, cb = 0; };   // +1 green, -1 red, 0 white, 2 grey
struct Out { bool ok = false; int verdict = 0; std::string head, why, candle, b1, b2; std::vector<Row> rows; std::vector<std::string> ex; std::vector<std::string> notes; };   // (1.3.0) notes = neutral facts for the Volume read (no side)   // ex (1.1.0) = the explanation sentences   // verdict +1 / -1 / 2 mixed / 0 quiet

inline std::string commas(long long v) { std::string s = std::to_string(v < 0 ? -v : v); for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ","); return v < 0 ? "-" + s : s; }
inline std::string xx(double v) { char b[24]; std::snprintf(b, sizeof b, v > 0 && v < 0.095 ? "%.2fx" : "%.1fx", (std::min)(v, 99.9)); return b; }   // (1.1.1) 0.01x, not 0.0x
inline std::string px(double v, int dec) { char b[40]; std::snprintf(b, sizeof b, "%.*f", dec < 0 ? 0 : dec > 6 ? 6 : dec, v); return b; }
inline std::string cnt(int n) { return n >= 99 ? std::string("99+") : std::to_string(n); }
inline int decOf(double tick) { if (!(tick > 0)) return 2; int d = 0; double t = std::round(tick * 1e9) / 1e9;   // (1.1.2) IRT hands 0.000500000023749 for HG: rounded first (was 6 decimals)
    while (d < 6 && std::fabs(t - std::round(t)) > 1e-7) { t *= 10; ++d; } return (std::max)(d, tick < 1 && d == 1 ? 1 : d); }
inline double med(std::vector<double> v) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); const size_t n = v.size(); return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0; }
// x the median of the (up to) 20 candles before index i; 0 = not known
inline double volX(const std::vector<Cd>& c, int i) { std::vector<double> v; for (int j = i - 1; j >= 0 && (int)v.size() < NORM_N; --j) v.push_back(c[(size_t)j].vol); const double m = (int)v.size() >= NORM_MIN ? med(v) : 0; return m > 0 ? c[(size_t)i].vol / m : 0; }
inline double rngX(const std::vector<Cd>& c, int i) { std::vector<double> v; for (int j = i - 1; j >= 0 && (int)v.size() < NORM_N; --j) v.push_back(c[(size_t)j].rng()); const double m = (int)v.size() >= NORM_MIN ? med(v) : 0; return m > 0 ? c[(size_t)i].rng() / m : 0; }
inline bool newHigh(const std::vector<Cd>& c, int i) { if (i < 3) return false; for (int j = i - 1; j >= 0 && j >= i - EXT_N; --j) if (c[(size_t)j].h >= c[(size_t)i].h) return false; return true; }
inline bool newLow(const std::vector<Cd>& c, int i) { if (i < 3) return false; for (int j = i - 1; j >= 0 && j >= i - EXT_N; --j) if (c[(size_t)j].l <= c[(size_t)i].l) return false; return true; }

inline std::string biggestIn(int n) { return n >= 5 ? ", the biggest in " + cnt(n) + " candles" : std::string(); }
struct Why { int s; std::string t; };
inline void verdict(Out& o, const std::vector<Why>& Y) {
    if (Y.empty()) { o.verdict = 0; o.head = "Quiet"; return; }
    int s = 0; bool up = false, dn = false; for (const Why& y : Y) { s += y.s; if (y.s > 0) up = true; if (y.s < 0) dn = true; }
    if (!up && !dn) { o.verdict = 0; o.head = "Quiet:"; o.why = Y[0].t; return; }
    o.verdict = s > 0 ? 1 : s < 0 ? -1 : 2; o.head = s > 0 ? "Bullish ?:" : s < 0 ? "Bearish ?:" : "Mixed ?:";
    std::vector<std::string> R;
    if (s != 0) { for (const Why& y : Y) if (y.s != 0 && (y.s > 0) == (s > 0) && R.size() < 2) R.push_back(y.t); }
    else { for (const Why& y : Y) if (y.s > 0) { R.push_back(y.t); break; } for (const Why& y : Y) if (y.s < 0) { R.push_back(y.t); break; } }
    for (size_t i = 0; i < R.size(); ++i) o.why += (i ? ", " : "") + R[i];
}

inline Out volumeRead(const Ctx& x) {
    Out o; const int n = (int)x.c.size(); if (n < 1) return o; const int i = n - 1; const Cd& k = x.c[(size_t)i]; if (!(k.h > 0) || k.h < k.l) return o;
    o.ok = true; const bool g = k.green(); const int cd = g ? 1 : -1; const double vx = volX(x.c, i), rx = rngX(x.c, i);
    o.candle = std::string(g ? "green, " : "red, ") + commas((long long)std::llround(k.vol)) + " lots" + (vx > 0 ? ", " + xx(vx) : std::string()) + (x.cntAll >= 5 ? ", biggest in " + cnt(x.cntAll) : std::string());
    std::vector<std::string> B, E, N; std::vector<Why> Y; const Inside& q = x.in;   // E = sentences behind the verdict, N = side notes (1.1.1)
    const bool hiSide = newHigh(x.c, i) || (i >= 1 && newHigh(x.c, i - 1)), loSide = newLow(x.c, i) || (i >= 1 && newLow(x.c, i - 1));
    if (q.ok) {
        const bool bBuy = q.bigBuy > 0 && (double)q.bigBuy >= ONE_SIDED * (double)q.bigSell && q.swBuy >= SWEEP_LV, bSell = q.bigSell > 0 && (double)q.bigSell >= ONE_SIDED * (double)q.bigBuy && q.swSell >= SWEEP_LV;
        if (bBuy) { E.push_back("One " + commas(q.bigBuy) + "-lot buy order swept " + std::to_string(q.swBuy) + " offer levels" + (hiSide ? " at the high" : "") +
                                (g ? " - buyers are pushing." : hiSide ? ", but the candle closed red - a buying climax that sellers absorbed." : ", but the candle closed red - those buyers may be trapped."));
                    B.push_back(commas(q.bigBuy) + "-lot buy swept " + std::to_string(q.swBuy) + " offer levels" + (hiSide ? " at the high" : "") + (g ? "" : "; candle closed red ?"));
                    if (g) Y.push_back(Why{ 1, "big buyers lifting" }); else Y.push_back(Why{ -2, hiSide ? "buying climax at the high" : "big buyers trapped" }); }
        if (bSell) { E.push_back("One " + commas(q.bigSell) + "-lot sell order swept " + std::to_string(q.swSell) + " bid levels" + (loSide ? " at the low" : "") +
                                 (!g ? " - sellers are pushing." : loSide ? ", but the candle closed green - a selling climax that buyers absorbed." : ", but the candle closed green - those sellers may be trapped."));
                     B.push_back(commas(q.bigSell) + "-lot sell swept " + std::to_string(q.swSell) + " bid levels" + (loSide ? " at the low" : "") + (g ? "; candle closed green ?" : ""));
                     if (!g) Y.push_back(Why{ -1, "big sellers hitting" }); else Y.push_back(Why{ 2, loSide ? "selling climax at the low" : "big sellers trapped" }); }
        const double need = ICE_SHARE * k.vol;
        if (!bBuy && !bSell && (q.swBuy >= SWEEP_LV || q.swSell >= SWEEP_LV) && q.swBuy != q.swSell) {
            const bool sb = q.swSell > q.swBuy; N.push_back(std::string(sb ? "Sellers" : "Buyers") + " swept " + std::to_string(sb ? q.swSell : q.swBuy) + " levels in one order, " + (sb ? "buyers" : "sellers") + " only " + std::to_string(sb ? q.swBuy : q.swSell) + "."); }
        if (q.hidSell > 0 && (double)q.hidSell >= need && (double)q.hidSell >= 1.2 * (double)q.hidBuy) { E.push_back("Hidden sellers: " + commas(q.hidSell) + " lots of iceberg offers soaked up the buying without showing on the book."); B.push_back("Hidden sellers: " + commas(q.hidSell) + " lots of iceberg offers filled the buying"); Y.push_back(Why{ -1, "hidden sellers" }); }
        else if (q.hidBuy > 0 && (double)q.hidBuy >= need && (double)q.hidBuy >= 1.2 * (double)q.hidSell) { E.push_back("Hidden buyers: " + commas(q.hidBuy) + " lots of iceberg bids soaked up the selling without showing on the book."); B.push_back("Hidden buyers: " + commas(q.hidBuy) + " lots of iceberg bids filled the selling"); Y.push_back(Why{ 1, "hidden buyers" }); }
    }
    for (int j = i; j >= 0 && j >= i - 2; --j) {   // heavy volume that went nowhere, on this candle or the two before
        const double v2 = volX(x.c, j), r2 = rngX(x.c, j); if (!(v2 >= BIG_X && r2 > 0 && r2 < 1.0)) continue;
        E.push_back((j == i ? std::string("Big volume (") : "At " + x.c[(size_t)j].when + ", big volume (") + xx(v2) + ") moved price only " + xx(r2) + " - someone absorbed it.");
        B.push_back((j == i ? std::string() : x.c[(size_t)j].when + ": ") + xx(v2) + " volume moved price only " + xx(r2) + " - absorbed ?"); Y.push_back(Why{ x.c[(size_t)j].green() ? -1 : 1, "volume absorbed" });
        o.notes.push_back((j == i ? std::string("Heavy volume (") : "At " + x.c[(size_t)j].when + ", heavy volume (") + xx(v2) + ") moved price only " + xx(r2) + "."); break; }
    if (x.cx) { o.notes.push_back(std::string("Climax volume at the ") + (x.cx < 0 ? "high" : "low") + "."); E.push_back(std::string("Climax volume at the ") + (x.cx < 0 ? "high" : "low") + " - a common place for the move to turn."); B.push_back(std::string("Climax volume at the ") + (x.cx < 0 ? "high" : "low") + " ?"); Y.push_back(Why{ x.cx * 2, std::string("climax at the ") + (x.cx < 0 ? "high" : "low") }); }
    { int run = 0; for (int j = i; j >= 0; --j) { const double v2 = volX(x.c, j); if (v2 >= BIG_X) ++run; else break; } if (run >= 3) { o.notes.push_back(std::to_string(run) + " heavy candles in a row."); E.push_back(std::to_string(run) + " heavy candles in a row - a strong push is under way."); B.push_back("Volume run: " + std::to_string(run) + " candles in a row at 2x or more"); Y.push_back(Why{ 0, "volume run" }); } }
    if (vx > 0 && vx < SMALL_X && i >= 1) { const double v1 = volX(x.c, i - 1); if (v1 > 0 && v1 < SMALL_X) { o.notes.push_back("Volume drying up two candles in a row."); E.push_back("Volume is drying up two candles in a row - the move is losing interest."); B.push_back("Volume drying up: " + xx(vx)); Y.push_back(Why{ 0, "volume drying up" }); } }
    if (rx >= BIG_X && vx > 0 && vx < 1.2) { E.push_back("Price moved " + xx(rx) + " its normal range on normal volume - little stood in the way."); B.push_back("Easy movement: range " + xx(rx) + " on normal volume"); Y.push_back(Why{ cd, "easy movement" }); }
    if (!B.empty()) o.b1 = B[0];
    if (B.size() > 1) o.b2 = B[1];
    verdict(o, Y);
    {   // (1.1.0) the size sentence: last when something stood out, first (with "nothing unusual") when not
        std::string sz = vx >= BIG_X ? "Heavy volume (" + xx(vx) + ")" + biggestIn(x.cntAll) + "." : vx > 0 && vx < SMALL_X ? "Light volume (" + xx(vx) + ")." : vx > 0 ? "Volume was normal (" + xx(vx) + ")." : "";
        if (E.empty() && N.empty()) { if (!sz.empty()) o.ex.push_back(sz); o.ex.push_back(q.ok ? "Nothing stood out inside this candle." : "Nothing stood out (no order detail for this candle)."); }
        else { o.ex = E; o.ex.insert(o.ex.end(), N.begin(), N.end()); if (!sz.empty()) o.ex.push_back(sz); } }
    { Row r; r.lab = "Volume"; r.a = vx > 0 ? xx(vx) : "-"; r.ca = cd; r.b = commas((long long)std::llround(k.vol)) + " lots"; o.rows.push_back(r); }
    { Row r; r.lab = "Range"; r.a = rx > 0 ? xx(rx) : "-"; r.ca = cd; r.b = px(k.rng(), x.dec) + " pts"; o.rows.push_back(r); }
    { Row r; r.lab = "Biggest in"; r.a = cnt(x.cntAll) + " all"; r.b = cnt(x.cntSame) + (g ? " green" : " red"); r.cb = cd; o.rows.push_back(r); }
    if (q.ok) { { Row r; r.lab = "Biggest order"; r.a = commas(q.bigBuy) + " buy"; r.ca = 1; r.b = commas(q.bigSell) + " sell"; r.cb = -1; o.rows.push_back(r); }
                { Row r; r.lab = "Sweep"; r.a = std::to_string(q.swBuy) + " levels up"; r.ca = 1; r.b = std::to_string(q.swSell) + " down"; r.cb = -1; o.rows.push_back(r); } }
    return o;
}

inline Out rangeRead(const Ctx& x) {
    Out o; const int n = (int)x.c.size(); if (n < 1) return o; const int i = n - 1; const Cd& k = x.c[(size_t)i]; if (!(k.h > 0) || k.h < k.l) return o;
    o.ok = true; const bool g = k.green(); const int cd = g ? 1 : -1; const double rx = rngX(x.c, i), R = k.rng();
    const double top = k.h - (std::max)(k.o, k.c), bot = (std::min)(k.o, k.c) - k.l; const int at = R > 0 ? (int)std::lround((k.c - k.l) / R * 100.0) : 50;
    o.candle = std::string(g ? "green, " : "red, ") + px(R, x.dec) + " pts" + (rx > 0 ? ", " + xx(rx) : std::string()) + (x.cntSame >= 5 ? std::string(", biggest ") + (g ? "green" : "red") + " in " + cnt(x.cntSame) : std::string());
    std::vector<std::string> B, E, N; std::vector<Why> Y; bool erased = false;   // E = sentences behind the verdict, N = side notes (1.1.1)
    if (i >= 1) { const Cd& p = x.c[(size_t)i - 1]; const double pr = rngX(x.c, i - 1);
        const bool er = p.green() != g && p.rng() > 0 && pr >= 1.0 && (g ? k.c >= p.o : k.c <= p.o);
        if (er) { erased = true; const bool failHi = !g && newHigh(x.c, i - 1), failLo = g && newLow(x.c, i - 1);
            E.push_back("It erased the whole " + p.when + " candle" + (failHi ? " - the new high failed and sellers took control." : failLo ? " - the new low failed and buyers took control." : g ? " - buyers took back all the selling." : " - sellers took back all the buying."));
            B.push_back("Erased the whole " + p.when + " candle" + (failHi ? ": the new high failed" : failLo ? ": the new low failed" : ""));
            Y.push_back(Why{ cd * 2, failHi ? "new high rejected, then erased" : failLo ? "new low rejected, then erased" : std::string("prior ") + (g ? "selling" : "buying") + " erased" }); } }
    if (R > 0 && newHigh(x.c, i) && top >= WICK_SHARE * R) { E.push_back("It made a new high but closed " + px(k.h - k.c, x.dec) + " pts off it - sellers rejected the high."); B.push_back("New high rejected: closed " + px(k.h - k.c, x.dec) + " pts off it"); Y.push_back(Why{ -2, "new high rejected" }); }
    else if (R > 0 && newLow(x.c, i) && bot >= WICK_SHARE * R) { E.push_back("It made a new low but closed " + px(k.c - k.l, x.dec) + " pts off it - buyers rejected the low."); B.push_back("New low rejected: closed " + px(k.c - k.l, x.dec) + " pts off it"); Y.push_back(Why{ 2, "new low rejected" }); }
    if (i >= 1) { const Cd& p = x.c[(size_t)i - 1]; const double pR = p.rng(), pt = p.h - (std::max)(p.o, p.c), pbm = (std::min)(p.o, p.c) - p.l;
        if (pR > 0 && newHigh(x.c, i - 1) && pt >= WICK_SHARE * pR && k.h <= p.h) { N.push_back("The " + p.when + " candle made the high but closed " + px(p.h - p.c, x.dec) + " pts off it, and this one did not take it out."); B.push_back(p.when + " made the high but closed " + px(p.h - p.c, x.dec) + " pts off it"); }
        else if (pR > 0 && newLow(x.c, i - 1) && pbm >= WICK_SHARE * pR && k.l >= p.l) { N.push_back("The " + p.when + " candle made the low but closed " + px(p.c - p.l, x.dec) + " pts off it, and this one did not take it out."); B.push_back(p.when + " made the low but closed " + px(p.c - p.l, x.dec) + " pts off it"); } }
    if (rx >= 1.5 && (g ? at >= 75 : at <= 25)) { E.push_back(std::string("A wide candle that closed near its ") + (g ? "high - buyers were in control to the close." : "low - sellers were in control to the close.")); B.push_back(std::string("Strong close: wide candle closed near its ") + (g ? "high" : "low")); Y.push_back(Why{ cd, "strong close" }); }
    else if (rx >= BIG_X && !erased) { E.push_back("A wide candle (" + xx(rx) + " its normal range) - a strong push."); B.push_back("Wide candle: " + xx(rx) + " range"); Y.push_back(Why{ cd, "wide candle" }); }   // an erasing candle is wide by nature: said once
    if (i >= 2 && rx > 0 && rx < 0.7 && R < x.c[(size_t)i - 1].rng() && x.c[(size_t)i - 1].rng() < x.c[(size_t)i - 2].rng()) { E.push_back("Three candles in a row, each smaller than the last - the market is coiling before a move."); B.push_back("Narrowing: 3 candles, each smaller than the last"); Y.push_back(Why{ 0, "narrowing" }); }
    if (i >= 2 && rx >= 1.2 && rngX(x.c, i - 1) >= 1.2 && rngX(x.c, i - 2) >= 1.2) { const double net = std::fabs(k.c - x.c[(size_t)i - 2].o), avg = (R + x.c[(size_t)i - 1].rng() + x.c[(size_t)i - 2].rng()) / 3.0;
        if (net < 0.5 * avg) { E.push_back("Wide candles that overlap and go nowhere - both sides are fighting."); B.push_back("No progress: wide candles overlapping"); Y.push_back(Why{ 0, "no progress" }); } }
    if (!B.empty()) o.b1 = B[0];
    if (B.size() > 1) o.b2 = B[1];
    verdict(o, Y);
    {   // (1.1.0) the shape sentence: size, colour and where it closed
        const char* sz = rx > 0 && rx < 0.7 ? "A small " : rx >= BIG_X ? "A wide " : rx >= 1.5 ? "A big " : "A normal-size ";   // (1.1.2) 1.5x-2x = big
        std::string sh = std::string(sz) + (g ? "green" : "red") + " candle" + (rx > 0 ? " (" + xx(rx) + " its normal range)" : std::string()) + ", closed " +
                         (at >= 75 ? "near its high" : at <= 25 ? "near its low" : "mid-range") + " (" + std::to_string(at) + "%)";
        if (R > 0 && at <= 15 && !g && bot <= 0.1 * R) sh += " with almost no lower wick";
        else if (R > 0 && at >= 85 && g && top <= 0.1 * R) sh += " with almost no upper wick";
        sh += ".";
        if (E.empty() && N.empty()) { o.ex.push_back(sh); o.ex.push_back("Nothing unusual about its shape."); } else { o.ex = E; o.ex.push_back(sh); o.ex.insert(o.ex.end(), N.begin(), N.end()); } }
    { Row r; r.lab = "Range"; r.a = rx > 0 ? xx(rx) : "-"; r.ca = cd; r.b = px(R, x.dec) + " pts"; o.rows.push_back(r); }
    { Row r; r.lab = "Against ATR"; r.a = x.atr > 0 ? xx(R / x.atr) : "-"; r.ca = cd; o.rows.push_back(r); }
    { Row r; r.lab = "Closed at"; r.a = std::to_string(at) + "%"; r.b = "of its range"; r.cb = 2; o.rows.push_back(r); }
    { Row r; r.lab = "Wicks"; r.a = "top " + px(top, x.dec); r.b = "bottom " + px(bot, x.dec); o.rows.push_back(r); }
    { Row r; r.lab = "Biggest in"; r.a = cnt(x.cntAll) + " all"; r.b = cnt(x.cntSame) + (g ? " green" : " red"); r.cb = cd; o.rows.push_back(r); }
    return o;
}

// "20261117" for civil days since 1970-01-01 (the session's RTH date)
inline std::string ymd(long long z) { z += 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097); const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long y = (long long)yoe + era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153; const unsigned d = doy - (153 * mp + 2) / 5 + 1; const unsigned m = mp < 10 ? mp + 3 : mp - 9;
    char b[64]; std::snprintf(b, sizeof b, "%04lld%02u%02u", y + (m <= 2 ? 1 : 0), m % 100u, d % 100u); return b; }
inline std::string clock12(long long localSec) { long long sod = ((localSec % 86400) + 86400) % 86400; int h = (int)(sod / 3600); const int m = (int)((sod % 3600) / 60); const bool pm = h >= 12; h %= 12; if (h == 0) h = 12; char b[24]; std::snprintf(b, sizeof b, "%d:%02d %s", h, m, pm ? "PM" : "AM"); return b; }
// lsBookFlow's session record ('|' separated, a header line naming the columns): per candle (bar_time = the chart's bar stamp) the biggest market
// order each way (aggr_max_a = a BUY into the offers, aggr_max_b = a SELL into the bids), the price levels one order swept, the iceberg lots traded
// (ice_traded_a = hidden OFFERS that filled buying). Returns the candles read; unknown / garbled lines are skipped.
inline int parseBookFlow(const std::string& text, std::map<long long, Inside>& out) {
    int iT = -1, iAb = -1, iAa = -1, iSb = -1, iSa = -1, iIb = -1, iIa = -1, nCol = 0, got = 0; size_t a = 0;
    while (a < text.size()) {
        size_t e = text.find('\n', a); if (e == std::string::npos) e = text.size();
        std::string l = text.substr(a, e - a); a = e + 1; if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.empty() || l[0] == '#') continue;
        std::vector<std::string> f; size_t b = 0; for (;;) { const size_t c = l.find('|', b); f.push_back(l.substr(b, c == std::string::npos ? std::string::npos : c - b)); if (c == std::string::npos) break; b = c + 1; }
        if (f[0] == "bar_time") { nCol = (int)f.size(); iT = 0; iAb = iAa = iSb = iSa = iIb = iIa = -1;
            for (int k = 0; k < nCol; ++k) { const std::string& h = f[(size_t)k]; if (h == "aggr_max_b") iAb = k; else if (h == "aggr_max_a") iAa = k; else if (h == "sweep_lv_b") iSb = k; else if (h == "sweep_lv_a") iSa = k; else if (h == "ice_traded_b") iIb = k; else if (h == "ice_traded_a") iIa = k; }
            continue; }
        if (iT < 0 || iAb < 0 || iAa < 0 || iSb < 0 || iSa < 0 || iIb < 0 || iIa < 0 || (int)f.size() < nCol) continue;
        auto ll = [&](int k, long long& v) { const std::string& t = f[(size_t)k]; if (t.empty() || t.size() > 18) return false; char* end = nullptr; v = std::strtoll(t.c_str(), &end, 10); return end && *end == 0 && v >= 0; };
        long long t = 0, ab = 0, aa = 0, sb = 0, sa = 0, ib = 0, ia = 0;
        if (!ll(iT, t) || !ll(iAb, ab) || !ll(iAa, aa) || !ll(iSb, sb) || !ll(iSa, sa) || !ll(iIb, ib) || !ll(iIa, ia) || t <= 0 || sb > 10000 || sa > 10000) continue;
        Inside q; q.ok = true; q.bigSell = ab; q.bigBuy = aa; q.swSell = (int)sb; q.swBuy = (int)sa; q.hidBuy = ib; q.hidSell = ia; out[t] = q; ++got;
        if (out.size() > 20000) out.erase(out.begin());
    }
    return got;
}


// (1.2.0, 2026-11-17 Rassul 21:15-21:17 CT "more human readable and simplified ... i need something practical", "i like it. lets do this") the PLAIN read:
// three lines - who is winning / why in plain words / what to watch - and no table (the numbers stay in the status file and the nightly).
inline std::string plainReason(const std::string& r) {
    static const char* const M[][2] = {
        {"big buyers lifting", "A big buyer swept up through the offers."}, {"big sellers hitting", "A big seller swept down through the bids."},
        {"buying climax at the high", "A big buyer at the high got absorbed - a buying climax."}, {"selling climax at the low", "A big seller at the low got absorbed - a selling climax."},
        {"big buyers trapped", "A big buyer pushed up but the candle closed red - buyers trapped."}, {"big sellers trapped", "A big seller pushed down but the candle closed green - sellers trapped."},
        {"hidden sellers", "Hidden sellers soaked up the buying."}, {"hidden buyers", "Hidden buyers soaked up the selling."},
        {"volume absorbed", "Heavy volume barely moved price - someone absorbed it."}, {"climax at the high", "Climax volume at the high - often the last push."},
        {"climax at the low", "Climax volume at the low - often the last push."}, {"volume run", "Several heavy candles in a row - a strong push."},
        {"volume drying up", "Volume is drying up - the move is losing interest."}, {"easy movement", "Price moved easily on normal volume."},
        {"new high rejected, then erased", "The new high failed and this candle erased the one before."}, {"new low rejected, then erased", "The new low failed and this candle erased the one before."},
        {"prior selling erased", "Buyers erased the whole candle before."}, {"prior buying erased", "Sellers erased the whole candle before."},
        {"new high rejected", "Sellers rejected the new high."}, {"new low rejected", "Buyers rejected the new low."},
        {"wide candle", "A wide candle - a strong push."}, {"narrowing", "Candles getting smaller - coiling before a move."},
        {"no progress", "Wide candles going nowhere - both sides fighting."} };
    for (const auto& m : M) if (r == m[0]) return m[1];
    return plr::cap(r) + ".";
}
inline plr::Plain plain(const Out& o, bool range) {
    plr::Plain p; const int v = o.verdict; const std::vector<std::string> R = plr::reasons(o.why);
    auto one = [&](const std::string& r) { if (r == "strong close") return std::string(v > 0 ? "A wide candle closed at its high - buyers in control." : "A wide candle closed at its low - sellers in control."); return plainReason(r); };
    const bool q = o.head.find('?') != std::string::npos;
    if (v == 1 || v == -1) { p.head = std::string(v > 0 ? (range ? "Buyers in control" : "Buyers pushing") : (range ? "Sellers in control" : "Sellers pushing")) + (q ? " ?" : "");
        p.why = R.empty() ? std::string() : one(R[0]);
        p.watch = v > 0 ? "Watch: holds above this candle's low -> buyers stay in control." : "Watch: stays under this candle's high -> sellers stay in control."; }
    else if (v == 2) { p.head = q ? "Mixed ?" : "Mixed"; p.why = R.size() >= 2 ? plr::butJoin(one(R[0]), one(R[1])) : R.empty() ? std::string() : one(R[0]);
        p.watch = "Watch: the next candle's close decides."; }
    else { p.head = "Quiet";
        if (!R.empty()) { p.why = one(R[0]); p.watch = R[0] == "narrowing" ? "Watch: the break out of the coil." : "Nothing to act on."; }
        else { p.why = o.ex.empty() ? std::string("Nothing unusual.") : plr::noParens(o.ex[0]); p.watch = "Nothing to act on."; } }
    return p;
}
// (1.3.0) THE VOLUME READ: line 1 how much (Heavy 1.5x+ / Above normal 1.2x+ / Normal / Below normal under 0.8x / Light under 0.5x, + "the biggest
// (green / red) bar in N candles" from 20), coloured by the candle only when it is the biggest in 20+; line 2 the biggest order each way and the longest
// sweep (3+ prices); line 3 icebergs traded (bids / offers); line 4 the first neutral note (climax, absorbed volume, a run, drying up)
static const double HEAVY_X = 1.5, ABOVE_X = 1.2, BELOW_X = 0.8; static const int BIG_IN = 20;
inline Out volPlain(const Ctx& x, Out o) {
    if (!o.ok) { return o; }
    const int n = (int)x.c.size();
    if (n < 1) { return o; }
    const Cd& k = x.c[(size_t)n - 1]; const double vx = volX(x.c, n - 1); const bool g = k.green();
    std::string sz = vx >= HEAVY_X ? "Heavy" : vx >= ABOVE_X ? "Above normal" : vx > 0 && vx < SMALL_X ? "Light" : vx > 0 && vx < BELOW_X ? "Below normal" : vx > 0 ? "Normal" : "Not rated yet";
    const bool bigSame = x.cntSame >= BIG_IN && x.cntSame >= x.cntAll, bigAll = x.cntAll >= BIG_IN;
    if (bigAll) sz += " - the biggest bar in " + cnt(x.cntAll) + " candles";
    else if (bigSame) sz += std::string(" - the biggest ") + (g ? "green" : "red") + " bar in " + cnt(x.cntSame) + " candles";
    if (vx > 0) sz += " (" + xx(vx) + ")";
    o.head = sz; o.why.clear(); o.verdict = (bigAll || bigSame) ? (g ? 1 : -1) : 0;
    std::vector<std::string> L; const Inside& q = x.in;
    if (q.ok) {
        std::string l = "Biggest orders: buyers " + commas(q.bigBuy) + ", sellers " + commas(q.bigSell);
        const int sw = q.swBuy > q.swSell ? q.swBuy : q.swSell;
        if (sw >= (int)SWEEP_LV) { const bool sb = q.swSell > q.swBuy; l += std::string("; one ") + (sb ? "sell" : "buy") + " order swept " + std::to_string(sw) + " prices, " + (sb ? "buyers " : "sellers ") + std::to_string(sb ? q.swBuy : q.swSell); }
        L.push_back(l + ".");
        if (q.hidBuy > 0 || q.hidSell > 0) L.push_back("Icebergs: bids refilled " + commas(q.hidBuy) + " lots, offers " + commas(q.hidSell) + ".");
    } else L.push_back("No order detail for this candle.");
    if (!o.notes.empty()) L.push_back(o.notes[0]);
    o.ex = L; o.rows.clear(); o.b1.clear(); o.b2.clear(); return o;
}
inline Out toPlain(Out o, bool range) { if (!o.ok) return o; const plr::Plain p = plain(o, range); o.head = p.head; o.why.clear(); o.ex = plr::lines(p); o.rows.clear(); o.b1.clear(); o.b2.clear(); return o; }
} // namespace vrd
