// SessionVWAPLogic.h -- the session rules and the VWAP arithmetic of lsSessionVWAP (1.2.0, 2026-10-08). NO SDK, so the
// whole calculation is tested on plain arrays (plugin/test_sessionvwap_logic.cpp + tests_sessionvwap/reference.py).
//
// Rassul 2026-10-08 22:13-22:16: "can you automatically switch the vwap during different sessions?" -> mockup -> "build it,
// make sure it uses the right day and overnight for all the markets i trade and the calculations are correct."
//
// THE SESSIONS (Central time, the chart's clock; a bar belongs by its START = its close stamp - the bar size):
//   overnight (O/N)  17:00 (Globex reopen) -> the market's RTH open. A bar starting at or after 17:00 belongs to the NEXT
//                    day's session (Sunday 17:00 -> Monday); after midnight it belongs to that day.
//   RTH              bar START >= open and bar END <= close (lra.markets): ES / NQ 08:30-15:00, CL / NG 08:00-13:30,
//                    GC / HG 07:20-12:30, 6E 07:20-14:00. Mon-Fri only.
//   after the close  until 17:00 the RTH lines stay FROZEN at their final value (no new O/N session until the reopen).
// THE MATH (both sessions): typical price (H + L + C) / 3 weighted by volume from the session's first traded bar;
// SD = sqrt(sum v (tp - VWAP)^2 / sum v) by weighted Welford (no cancellation on large volumes); bands VWAP +-1 / +-2 SD.
// A zero-volume bar carries the running values; before the first trade nothing is drawn (0 = no line).
// pRTH = the final RTH VWAP of the latest earlier session that had one, drawn on overnight bars only.
#pragma once
#include <cmath>
#include "OnTouchLogic.h"
#include <vector>

namespace svl {

enum Kind { NONE = 0, ON = 1, RTH = 2, POST = 3 };

// days since 1970-01-01 of a civil date (proleptic Gregorian) and back - no time zone involved
inline long daysFromCivil(int y, int m, int d)
{
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}
inline int weekdayOf(long days) { long w = (days + 4) % 7; return (int)(w < 0 ? w + 7 : w); }   // 0 = Sunday (1970-01-01 = Thu)

inline bool rthOf(const char* m, int& openMin, int& closeMin)
{
    auto is = [&](const char* a) { const char* p = m; const char* q = a; while (*p && *q && *p == *q) { p++; q++; } return *p == 0 && *q == 0; };
    if (!m) return false;
    if (is("ES") || is("NQ")) { openMin = 8 * 60 + 30; closeMin = 15 * 60; return true; }
    if (is("CL") || is("NG")) { openMin = 8 * 60;      closeMin = 13 * 60 + 30; return true; }
    // GC / HG / 6E open 07:20. A 3-min chart's bars (07:18-07:21, 07:21-07:24) give the same first bar as lra.markets'
    // 07:21 start; a 1-min chart now also keeps the 07:20 bar.
    if (is("GC") || is("HG")) { openMin = 7 * 60 + 20; closeMin = 12 * 60 + 30; return true; }
    if (is("EU"))             { openMin = 7 * 60 + 20; closeMin = 14 * 60; return true; }
    return false;
}

struct Cls { int kind = NONE; long sess = 0; int startSec = 0; };

// days = the civil day of the bar's CLOSE stamp, endSec = its second of the day, per = bar size in seconds
inline Cls classify(long days, int endSec, int per, int openMin, int closeMin)
{
    Cls c;
    long d = days; int s = endSec - per;
    while (s < 0) { s += 86400; d -= 1; }                  // a bar closing at 00:00-00:02 started the evening before
    c.startSec = s;
    if (s >= 17 * 3600) { c.kind = ON; c.sess = d + 1; }   // the evening: next day's session
    else if (s < openMin * 60) { c.kind = ON; c.sess = d; }
    else if (s + per <= closeMin * 60) { c.kind = RTH; c.sess = d; }
    else { c.kind = POST; c.sess = d; }
    int wd = weekdayOf(c.sess);
    if (wd == 0 || wd == 6) c.kind = NONE;                 // no Saturday / Sunday session (Sunday evening is Monday's)
    return c;
}

struct Acc {
    double sv = 0, vw = 0, m2 = 0;
    void reset() { sv = vw = m2 = 0; }
    void add(double tp, double v)
    {
        if (!(v > 0) || !std::isfinite(tp) || !std::isfinite(v)) return;
        double nsv = sv + v, delta = tp - vw;
        vw += delta * (v / nsv);
        m2 += v * delta * (tp - vw);
        sv = nsv;
    }
    bool ok() const { return sv > 0 && std::isfinite(vw) && std::isfinite(m2); }
    double sd() const { double var = m2 / sv; return (std::isfinite(var) && var > 0) ? std::sqrt(var) : 0.0; }
    void out(float* o) const
    {
        if (!ok()) { for (int k = 0; k < 5; k++) o[k] = 0; return; }
        double s = sd();
        o[0] = (float)vw; o[1] = (float)(vw + s); o[2] = (float)(vw - s); o[3] = (float)(vw + 2 * s); o[4] = (float)(vw - 2 * s);
    }
};

struct Bar { long days; int endSec; float h, l, c; double v; };

// 11 outputs per bar: [0..4] RTH VWAP, +1, -1, +2, -2 (frozen after the close), [5..9] O/N the same, [10] pRTH VWAP.
// Returns the number of RTH bars; sets invalid when a bar had a non-finite price.
struct Result { int nRth = 0, nOn = 0; bool invalid = false; };
inline Result run(const std::vector<Bar>& B, int per, int openMin, int closeMin, std::vector<float>& out /* 11 * n */)
{
    Result R;
    size_t n = B.size();
    out.assign(n * 11, 0.0f);
    Acc on, rth;
    long cur = -1;
    float rthFinal[5] = { 0, 0, 0, 0, 0 }; bool curHasRth = false;
    float prevRthVwap = 0;
    for (size_t i = 0; i < n; i++) {
        Cls c = classify(B[i].days, B[i].endSec, per, openMin, closeMin);
        float* o = &out[i * 11];
        if (c.kind == NONE) continue;
        if (c.sess != cur) {
            if (curHasRth && rthFinal[0] > 0) prevRthVwap = rthFinal[0];
            on.reset(); rth.reset(); curHasRth = false; for (int k = 0; k < 5; k++) rthFinal[k] = 0;
            cur = c.sess;
        }
        bool good = std::isfinite(B[i].h) && std::isfinite(B[i].l) && std::isfinite(B[i].c);
        if (!good) R.invalid = true;
        double tp = good ? ((double)B[i].h + (double)B[i].l + (double)B[i].c) / 3.0 : 0.0;
        if (c.kind == ON) {
            R.nOn++;
            if (!good) continue;
            on.add(tp, B[i].v);
            on.out(o + 5);
            o[10] = prevRthVwap;
        } else if (c.kind == RTH) {
            R.nRth++;
            if (!good) continue;
            rth.add(tp, B[i].v);
            rth.out(o);
            if (o[0] > 0) { for (int k = 0; k < 5; k++) rthFinal[k] = o[k]; curHasRth = true; }
        } else {                                            // POST: the day's RTH lines, frozen
            if (curHasRth) for (int k = 0; k < 5; k++) o[k] = rthFinal[k];
        }
    }
    return R;
}

using otl::onSigmaMin;
using otl::onTouch;

} // namespace svl
