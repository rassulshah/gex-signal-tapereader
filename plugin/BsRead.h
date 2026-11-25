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
#include <algorithm>
#include <map>

namespace bsr {

static const char* const VERSION = "2.0.0";   // (2.0.0, Rassul 2026-11-24 21:23-22:39 CT "i dont want repetitive information in the read ... it should be either an important point, a conclusion or a verdict", "everything else is a go" on reads-conclude-mockup.html v30): the read CONCLUDES - who is in control, delta against price (a trapped side), strong side + delta flip, selling / buying drying up; no lots, no ratios the bars show, no "?", the big-order share moved to the Volume read (How big)
// was static const char* const VERSION = "1.1.0";
static const double SWING_X = 1.33, EVEN = 1.10, STRONG_X = 1.5, DELTA_BIG_X = 1.5, TRAP_ATR = 0.3; static const int NORM_N = 20, NORM_MIN = 10, BIG_N = 10, EXT_N = 10;

struct Prev { long long hi = 0, lo = 0, close = 0, buy = 0, sell = 0; bool ok = false; };
struct In { bool ok = false; long long buy = 0, sell = 0; std::vector<long long> pb, ps;   // pb / ps = buys / sells of the candles before, newest first
            bool prevOk = false; long long prevDelta = 0; std::vector<long long> pswing;  // the |delta swing| of the candles before, newest first
            int nB = 0, nS = 0;
            long long open = 0, close = 0, hi = 0, lo = 0; std::vector<Prev> prev;        // (2.0.0) this candle in ticks (0 = not known) and the candles before, newest first (up to 40)
            bool bigOk = false; long long bigBuy = 0, bigSell = 0, allBuy = 0, allSell = 0, thr = 0; };   // kept for the caller (not used since 2.0.0)
static const char* const Q2 = "2 Who hit whom?"; static const char* const Q4 = "4 How big?";
struct Out { bool ok = false; int verdict = 0; std::string head; std::vector<std::string> ex; std::string headQ = Q2; std::vector<std::string> exQ; double buyX = -1, sellX = -1, swingX = -1, deltaX = -1; long long delta = 0, swing = 0, atr = 0; int newExt = 0; };

inline std::string commas(long long v) { std::string s = std::to_string(v < 0 ? -v : v); for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ","); return v < 0 ? "-" + s : s; }
inline std::string sgn(long long v) { return (v > 0 ? "+" : v < 0 ? "-" : "") + commas(v < 0 ? -v : v); }
inline std::string xs(double x) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", x < 0 ? 0.0 : x > 99.9 ? 99.9 : x); return b; }
inline double meanOf(const std::vector<long long>& v) { double s = 0; int n = 0; for (size_t k = 0; k < v.size() && n < NORM_N; ++k) { if (v[k] < 0) continue; s += (double)v[k]; ++n; } return n >= NORM_MIN && s > 0 ? s / n : -1; }
inline std::string cnt(int n) { return n >= 99 ? std::string("99+") : std::to_string(n); }
// Wilder ATR(13) in ticks from the candles before (newest first); 0 = under 14 known
inline long long atrOf(const std::vector<Prev>& p) {
    std::vector<const Prev*> v; for (const Prev& q : p) { if (!q.ok) break; v.push_back(&q); } if (v.size() < 14) { return 0; }
    std::vector<const Prev*> o(v.rbegin(), v.rend()); double a = 0;
    for (size_t k = 1; k <= 13; ++k) { const long long h = o[k]->hi, l = o[k]->lo, pc = o[k - 1]->close; a += (double)std::max(h - l, std::max(std::llabs(h - pc), std::llabs(l - pc))); } a /= 13.0;
    for (size_t k = 14; k < o.size(); ++k) { const long long h = o[k]->hi, l = o[k]->lo, pc = o[k - 1]->close; a = (a * 12.0 + (double)std::max(h - l, std::max(std::llabs(h - pc), std::llabs(l - pc)))) / 13.0; }
    return (long long)std::llround(a);
}
// +1 = a new EXT_N-candle low, -1 = a new high, 0 = neither / not known
inline int newExtOf(const In& x) {
    if (x.hi <= 0 || x.lo <= 0 || (int)x.prev.size() < EXT_N) { return 0; } long long mn = 0, mx = 0;
    for (int k = 0; k < EXT_N; ++k) { const Prev& q = x.prev[(size_t)k]; if (!q.ok) return 0; if (k == 0 || q.lo < mn) mn = q.lo; if (k == 0 || q.hi > mx) mx = q.hi; }
    const bool l = x.lo < mn, h = x.hi > mx; return l && !h ? 1 : h && !l ? -1 : 0;
}
// THE READ (2.0.0), each line answers 2 Who hit whom?:
//   head   "Buyers took over at the low" (a side 1.5x+ normal and bigger, or the delta flipped 1.33x+ its way; "at the low / high" at a new 10-candle
//          extreme) | "Buyers took over - the delta flipped 3.7x bullish" (the flip without a strong side) | "Sellers hit harder but could not keep the low"
//          (more selling at a new low, closed in the upper half) | "Two-sided - no one in control"
//   2      delta against price, only when |delta| is 1.5x+ its normal: "Delta and price agreed." / "Buyers hit hard but price fell - buyers trapped." (closed
//          0.3 ATR+ the other way)
//   3      "Strong buyers flipped delta 3.2x bullish." (a strong side AND the flip its way)
//   4      "Selling dried up over 3 candles." (at a new low: the sellers' lots fell 3 candles in a row into this one; mirror at a high)
inline Out read(const In& x) {
    Out o; if (!x.ok || x.buy < 0 || x.sell < 0) return o; o.ok = true;
    const long long b = x.buy, s = x.sell, hi = b > s ? b : s, lo = b > s ? s : b; o.delta = b - s;
    if (hi == 0) { o.head = "No trades"; return o; }
    const bool even = lo > 0 && (double)hi / (double)lo < EVEN; const int big = even ? 0 : (b > s ? 1 : -1);
    const double mb = meanOf(x.pb), ms = meanOf(x.ps); if (mb > 0) o.buyX = (double)b / mb; if (ms > 0) o.sellX = (double)s / ms;
    int flip = 0; if (x.prevOk) { o.swing = o.delta - x.prevDelta; const double ma = meanOf(x.pswing); if (ma > 0) { o.swingX = (double)std::llabs(o.swing) / ma; if (o.swingX >= SWING_X && o.swing != 0) flip = o.swing > 0 ? 1 : -1; } }
    int strong = 0; if (big > 0 && o.buyX >= STRONG_X) strong = 1; else if (big < 0 && o.sellX >= STRONG_X) strong = -1;
    o.newExt = newExtOf(x); o.atr = atrOf(x.prev);
    std::vector<long long> ad; for (const Prev& q : x.prev) { if (!q.ok) break; ad.push_back(std::llabs(q.buy - q.sell)); } const double mdl = meanOf(ad); if (mdl > 0) o.deltaX = (double)std::llabs(o.delta) / mdl;
    const std::string atEx = o.newExt > 0 ? " at the low" : o.newExt < 0 ? " at the high" : "";
    const int took = strong ? strong : flip;
    const bool closeUpper = x.hi > x.lo && x.close > 0 && (x.close - x.lo) * 2 >= (x.hi - x.lo), closeLower = x.hi > x.lo && x.close > 0 && (x.close - x.lo) * 2 <= (x.hi - x.lo);
    const bool lostLow = o.newExt > 0 && closeUpper && (took < 0 || (!took && big < 0)), lostHigh = o.newExt < 0 && closeLower && (took > 0 || (!took && big > 0));   // the attackers could not keep the new extreme
    if (lostLow) { o.verdict = 0; o.head = "Sellers hit harder but could not keep the low"; }
    else if (lostHigh) { o.verdict = 0; o.head = "Buyers hit harder but could not keep the high"; }
    else if (took) { o.verdict = took; const std::string w = took > 0 ? "Buyers" : "Sellers";
        if (strong) o.head = w + " took over" + atEx; else o.head = w + " took over - the delta flipped " + xs(o.swingX) + (took > 0 ? " bullish" : " bearish"); }
    else { o.verdict = 0; o.head = "Two-sided - no one in control"; }
    // 2: delta against price
    if (o.deltaX >= DELTA_BIG_X && o.delta != 0 && x.close > 0 && x.open > 0) { const long long mv = x.close - x.open;
        if ((mv > 0 && o.delta > 0) || (mv < 0 && o.delta < 0)) { o.ex.push_back("Delta and price agreed."); o.exQ.push_back(Q2); }
        else if (o.atr > 0 && (double)std::llabs(mv) >= TRAP_ATR * (double)o.atr && mv != 0) { o.ex.push_back(o.delta > 0 ? "Buyers hit hard but price fell - buyers trapped." : "Sellers hit hard but price rose - sellers trapped."); o.exQ.push_back(Q2); } }
    // 3: a strong side that also flipped the delta its way
    if (strong && flip == strong && !lostLow && !lostHigh) { o.ex.push_back(std::string(strong > 0 ? "Strong buyers" : "Strong sellers") + " flipped delta " + xs(o.swingX) + (strong > 0 ? " bullish." : " bearish.")); o.exQ.push_back(Q2); }
    // 4: the attacking side drying up into a new extreme
    if (o.newExt != 0 && x.prev.size() >= 3 && x.prev[0].ok && x.prev[1].ok && x.prev[2].ok) {
        const auto lots = [&](const Prev& q) { return o.newExt > 0 ? q.sell : q.buy; }; const long long c0 = o.newExt > 0 ? s : b;
        if (lots(x.prev[2]) > lots(x.prev[1]) && lots(x.prev[1]) > lots(x.prev[0]) && lots(x.prev[0]) > c0) { o.ex.push_back(o.newExt > 0 ? "Selling dried up over 3 candles." : "Buying dried up over 3 candles."); o.exQ.push_back(Q2); } }
    return o;
}
// (2.0.0) RevRead\bsv-<MKT>-<spb>.txt, written by lsRevRead 0.31.0 for the Volume read's overall call: "BSV|1|MKT|spb|ver", then "stamp|verdict|trap"
// (verdict +1 / -1 / 0; trap +1 = buyers trapped, -1 = sellers trapped, 0 = none). An unterminated last line (a torn write) is dropped.
inline bool parseBsv(const std::string& txt, const std::string& mkt, int spb, std::map<long long, std::pair<int, int> >& out) {
    out.clear(); std::vector<std::string> L; { std::string cur; for (char c : txt) { if (c == '\n') { L.push_back(cur); cur.clear(); } else if (c != '\r') cur += c; } }
    if (L.empty()) return false;
    auto split = [](const std::string& x) { std::vector<std::string> v; std::string cur; for (char c : x) { if (c == '|') { v.push_back(cur); cur.clear(); } else cur += c; } v.push_back(cur); return v; };
    auto num = [](const std::string& x, long long& v) { if (x.empty() || x.size() > 20) return false; char* e = nullptr; v = std::strtoll(x.c_str(), &e, 10); return e && *e == 0; };
    const std::vector<std::string> h = split(L[0]); long long sp = 0; if (h.size() < 4 || h[0] != "BSV" || h[1] != "1" || h[2] != mkt || !num(h[3], sp) || sp != spb) return false;
    for (size_t i = 1; i < L.size(); ++i) { const std::vector<std::string> c = split(L[i]); long long t = 0, v = 0, tr = 0; if (c.size() != 3 || !num(c[0], t) || !num(c[1], v) || !num(c[2], tr) || t <= 0 || v < -1 || v > 1 || tr < -1 || tr > 1) continue; out[t] = std::make_pair((int)v, (int)tr); }
    return true;
}
} // namespace bsr
