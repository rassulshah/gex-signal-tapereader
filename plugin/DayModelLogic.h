// DayModelLogic.h — lsDayModel's decisions on plain arrays, no SDK. Pinned by plugin/test_daymodel_logic.cpp.
//
//   · the bar-stamp convention (start- vs end-stamped 3-minute bars) and the RTH / evening / pre-open windows under it
//     (v0.16: the swept PDH was 7690 on IRT and 7687 on the chart until the window followed the stamp)
//   · the chart-native reference levels PDH / PDL / ONH / ONL from the bars themselves
//   · the ACTUAL day measured from the most recent RTH day on the chart (v0.15: not IRT's rolling session — at 8 PM it
//     was measuring the evening session's low)
//   · the open-anchored re-alignment of the expected candle and the swept levels onto the chart's contract
//   · the stale age with the overnight wrap
#pragma once
#include <cstddef>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cctype>

namespace dml {

struct Bar { int y, m, d; int sod; float o, h, l, c; };

inline bool finiteFloat(float value) { return std::isfinite(static_cast<double>(value)); }
inline bool validSecondsOfDay(double value) { return std::isfinite(value) && value >= 0.0 && value < 86400.0; }

// Parse externally supplied numeric CSV fields completely.  atof() would turn malformed
// fields into zero and accept NaN/Inf, both of which can corrupt price-to-pixel rendering.
inline bool parseFiniteDouble(const char* text, double& value)
{
    if (!text) return false;
    char* end = 0;
    errno = 0;
    const double parsed = std::strtod(text, &end);
    if (end == text || errno == ERANGE || !std::isfinite(parsed)) return false;
    while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (*end) return false;
    value = parsed;
    return true;
}
inline bool parseFiniteFloat(const char* text, float& value)
{
    double parsed = 0.0;
    if (!parseFiniteDouble(text, parsed)) return false;
    const float narrowed = static_cast<float>(parsed);
    if (!finiteFloat(narrowed)) return false;
    value = narrowed;
    return true;
}
inline bool validOhlc(float o, float h, float l, float c)
{
    return finiteFloat(o) && finiteFloat(h) && finiteFloat(l) && finiteFloat(c) &&
           h >= l && h >= o && h >= c && l <= o && l <= c;
}
inline bool validBar(const Bar& bar)
{
    return finiteFloat(bar.o) && finiteFloat(bar.h) && finiteFloat(bar.l) && finiteFloat(bar.c);
}

// (v0.16) end-stamped = a bar stamped at the 16:00 session CLOSE exists and none stamped at the 17:00 OPEN
inline bool endStamped(const Bar* b, int n)
{
    if (!b || n < 2) return false;
    int seenStart = 0, seenEnd = 0, to = n - 2000; if (to < 0) to = 0;
    for (int i = n - 1; i >= to; i--) { if (b[i].sod == 17 * 3600) seenStart++; if (b[i].sod == 16 * 3600) seenEnd++; }
    return seenEnd > 0 && seenStart == 0;
}
inline bool inRth(int sod, bool endStamped)     { const int O = 8 * 3600 + 30 * 60, S = 15 * 3600; return endStamped ? (sod > O && sod <= S) : (sod >= O && sod < S); }
inline bool inEvening(int sod, bool endStamped) { const int E = 17 * 3600; return endStamped ? (sod > E) : (sod >= E); }
inline bool inPreOpen(int sod, bool endStamped) { const int O = 8 * 3600 + 30 * 60; return endStamped ? (sod <= O) : (sod < O); }

inline int dateKey(int y, int m, int d) { return y * 10000 + m * 100 + d; }
// days since 1970-01-01 for a civil date (proleptic Gregorian) — so "the calendar day N days back" needs no mktime
inline long dayNumber(int y, int m, int d)
{
    y -= m <= 2; long era = (y >= 0 ? y : y - 399) / 400; long yoe = y - era * 400;
    long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

struct Levels { bool hp, ho; float pdh, pdl, onh, onl; };
// PDH/PDL = the prior RTH day's high/low; ONH/ONL = that day's evening (17:00 on) through today's pre-open (to 08:30)
inline Levels chartLevels(const Bar* b, int n, bool es)
{
    Levels L; L.hp = L.ho = false; L.pdh = -1e9f; L.pdl = 1e9f; L.onh = -1e9f; L.onl = 1e9f;
    if (!b || n < 2) return L;
    int todayKey = dateKey(b[n - 1].y, b[n - 1].m, b[n - 1].d), priorKey = 0;
    for (int i = n - 1; i >= 0; i--) {
        const int k = dateKey(b[i].y, b[i].m, b[i].d);
        if (k < todayKey && inRth(b[i].sod, es) && validBar(b[i])) {
            priorKey = k;
            break;
        }
    }
    for (int i = 0; i < n; i++) {
        const int k = dateKey(b[i].y, b[i].m, b[i].d);
        if (!validBar(b[i])) continue;
        if (priorKey && k == priorKey && inRth(b[i].sod, es)) {
            if (b[i].h > L.pdh) L.pdh = b[i].h;
            if (b[i].l < L.pdl) L.pdl = b[i].l;
            L.hp = true;
        }
        if ((priorKey && k == priorKey && inEvening(b[i].sod, es)) || (k == todayKey && inPreOpen(b[i].sod, es))) {
            if (b[i].h > L.onh) L.onh = b[i].h;
            if (b[i].l < L.onl) L.onl = b[i].l;
            L.ho = true;
        }
    }
    return L;
}

struct DayMeasure { bool ok; float o, h, l, c; long dayNum; };
// The most recent valid RTH day represented by the chart. Search bars rather than
// limiting calendar lookback: holiday clusters can be longer than a weekend.
inline DayMeasure measureDay(const Bar* b, int n, bool es)
{
    DayMeasure D; D.ok = false; D.o = D.h = D.l = D.c = 0; D.dayNum = 0;
    if (!b || n < 1) return D;
    long want = 0;
    bool found = false;
    for (int i = n - 1; i >= 0; --i) {
        if (inRth(b[i].sod, es) && validBar(b[i])) {
            want = dayNumber(b[i].y, b[i].m, b[i].d);
            found = true;
            break;
        }
    }
    if (!found) return D;

    float o = 0.0f, h = 0.0f, l = 0.0f, c = 0.0f;
    bool have = false;
    for (int i = 0; i < n; ++i) {
        if (dayNumber(b[i].y, b[i].m, b[i].d) != want || !inRth(b[i].sod, es) ||
            !validBar(b[i])) continue;
        if (!have) {
            o = b[i].o;
            h = b[i].h;
            l = b[i].l;
            have = true;
        } else {
            if (b[i].h > h) h = b[i].h;
            if (b[i].l < l) l = b[i].l;
        }
        c = b[i].c;
    }
    if (have && h >= l) {
        D.ok = true;
        D.o = o;
        D.h = h;
        D.l = l;
        D.c = c;
        D.dayNum = want;
    }
    return D;
}

// the open-anchored spread that re-aligns the expected candle and the swept levels onto the measured day
inline float dayOffset(float chartOpen, bool csvActValid, float csvActOpen)
{
    if (!finiteFloat(chartOpen) || !csvActValid || !finiteFloat(csvActOpen)) return 0.0f;
    const float offset = chartOpen - csvActOpen;
    return finiteFloat(offset) ? offset : 0.0f;
}

// the contract offset the pre-open path uses (chart close − SPOT), with its clamp and "already aligned" short-cut
inline bool contractOffset(float chartClose, float spotPx, float& off)
{
    if (!finiteFloat(chartClose) || !finiteFloat(spotPx) || !(chartClose > 0)) return false;
    off = chartClose - spotPx;
    if (!finiteFloat(off)) return false;
    if (off < -300.0f || off > 300.0f) return false;
    if (off > -0.01f && off < 0.01f) return false;
    return true;
}

// minutes since the panel's ASOF write, handling the overnight wrap; <0 when ASOF is unknown
inline double staleAge(double asofSo, double localSo)
{
    if (!validSecondsOfDay(asofSo) || !validSecondsOfDay(localSo)) return -1.0;
    return (asofSo > localSo + 300.0) ? ((86400.0 - asofSo) + localSo) / 60.0 : (localSo - asofSo) / 60.0;
}
inline bool staleBadgeShown(double ageMin) { return ageMin > 4.0; }

} // namespace dml
