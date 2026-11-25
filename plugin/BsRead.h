// BsRead.h -- the Reversal Read pane's BUY / SELL read (lsRevRead 0.29.0). SDK-free.
// Rassul 2026-11-24 19:31 CT "currently we show the change in delta, should this be updated to buy and sell volume instead"; 19:33 "lets do that ...
// i would also hide the book flow ... i would read the volume, then the buy sell vol, then the absorbtion volume"; 19:44 "Go" (panes-read-mockup.html).
// The pane answers layer 2 of his volume model - WHO HIT WHOM over the whole candle; the Volume pane above says how much, the Turn pane below who won.
//   line 1  "Buyers 909 vs sellers 598" (+ "- the most buying in N candles" from 10) - green when buyers hit more, red when sellers did, grey when
//           within 10% of each other ("Even")
//   line 2  "Buyers 1.9x normal, sellers 1.3x - delta +311."   normal = the mean of the up to 20 candles before (10 at least)
//   line 3  "Delta swung +284 from the candle before (3.2x) ?" when the swing is 1.33x+ its normal (the mean |swing| of the 20 before), else
//           "Delta: no big swing from the candle before (0.4x)."
// Research 2026-11-24 (new 10-candle lows / highs that closed out of the end zone, 7 markets, n 7,947; 1 ATR the turn's way first): the turning
// side's volume 1.5x+ 69% vs 55% (both halves 69); the delta swing 1.33x+ the turn's way 66% vs 57% (halves 68 / 65, all 7 markets); both 72%,
// neither 54% - the swing keeps its "?" (not yet in R after costs).
#pragma once
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace bsr {

static const char* const VERSION = "1.0.0";
static const double SWING_X = 1.33, EVEN = 1.10; static const int NORM_N = 20, NORM_MIN = 10, BIG_N = 10;

struct In { bool ok = false; long long buy = 0, sell = 0; std::vector<long long> pb, ps;   // pb / ps = buys / sells of the candles before, newest first
            bool prevOk = false; long long prevDelta = 0; std::vector<long long> pswing;  // the |delta swing| of the candles before, newest first
            int nB = 0, nS = 0; };                                                         // biggest-in counts (the bar labels)
struct Out { bool ok = false; int verdict = 0; std::string head; std::vector<std::string> ex; double buyX = -1, sellX = -1, swingX = -1; long long delta = 0, swing = 0; };

inline std::string commas(long long v) { std::string s = std::to_string(v < 0 ? -v : v); for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ","); return v < 0 ? "-" + s : s; }
inline std::string sgn(long long v) { return (v > 0 ? "+" : v < 0 ? "-" : "") + commas(v < 0 ? -v : v); }
inline std::string xs(double x) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", x < 0 ? 0.0 : x > 99.9 ? 99.9 : x); return b; }
// the mean of the first NORM_N values (NORM_MIN at least), else -1
inline double meanOf(const std::vector<long long>& v) { double s = 0; int n = 0; for (size_t k = 0; k < v.size() && n < NORM_N; ++k) { if (v[k] < 0) continue; s += (double)v[k]; ++n; } return n >= NORM_MIN && s > 0 ? s / n : -1; }
inline std::string cnt(int n) { return n >= 99 ? std::string("99+") : std::to_string(n); }

inline Out read(const In& x) {
    Out o; if (!x.ok || x.buy < 0 || x.sell < 0) return o; o.ok = true;
    const long long b = x.buy, s = x.sell, hi = b > s ? b : s, lo = b > s ? s : b;
    o.delta = b - s;
    const bool even = hi == 0 || (lo > 0 && (double)hi / (double)lo < EVEN);
    o.verdict = even ? 0 : (b > s ? 1 : -1);
    if (hi == 0) o.head = "No trades";
    else if (even) o.head = "Even - buyers " + commas(b) + " vs sellers " + commas(s);
    else if (b > s) { o.head = "Buyers " + commas(b) + " vs sellers " + commas(s); if (x.nB >= BIG_N) o.head += " - the most buying in " + cnt(x.nB) + " candles"; }
    else { o.head = "Sellers " + commas(s) + " vs buyers " + commas(b); if (x.nS >= BIG_N) o.head += " - the most selling in " + cnt(x.nS) + " candles"; }
    const double mb = meanOf(x.pb), ms = meanOf(x.ps);
    if (mb > 0) o.buyX = (double)b / mb;
    if (ms > 0) o.sellX = (double)s / ms;
    std::string l2;
    if (o.buyX >= 0 && o.sellX >= 0) l2 = b >= s ? "Buyers " + xs(o.buyX) + " normal, sellers " + xs(o.sellX) : "Sellers " + xs(o.sellX) + " normal, buyers " + xs(o.buyX);
    else l2 = "Normal not known yet";
    l2 += " - delta " + sgn(o.delta) + ".";
    o.ex.push_back(l2);
    if (x.prevOk) {
        o.swing = o.delta - x.prevDelta; const double ma = meanOf(x.pswing);
        if (ma > 0) { o.swingX = (double)std::llabs(o.swing) / ma;
            if (o.swingX >= SWING_X) o.ex.push_back("Delta swung " + sgn(o.swing) + " from the candle before (" + xs(o.swingX) + ") ?");
            else o.ex.push_back("Delta: no big swing from the candle before (" + xs(o.swingX) + ")."); }
    }
    return o;
}
} // namespace bsr
