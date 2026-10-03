/********************************************************************************
 *  IRTReaderLogic.h  --  the testable half of lsIRTReader (no IRT SDK)
 *
 *  lsIRTReader records what Investor/RT knows about the order flow of the chart's market, so the LRA analytics can read it
 *  the way it reads MenthorQ (Rassul 2026-10-02 19:31 "read my footprint, dom, time and sales"; 20:08 "start building the irt
 *  reader indicator ... get the footprint charts for the markets i trade and start backtesting them"):
 *     footprint   every bar's volume at each price, split bought (at the ask) / sold (at the bid), trades, delta swings
 *     trades      every trade: time, price, size, bid / ask at that moment -> who was the aggressor
 *     dom         the top bid / ask levels and sizes, a snapshot when they change (every N s)
 *     dbo         order-by-order depth events (new / modify / delete / fill) when the feed carries them
 *  Files: <folder>\<session yyyy-mm-dd>\<MKT>\fp_<N>m.csv, trades.csv, dom.csv, dbo.csv  (| separated, a header row first).
 *  The session is the futures trading day: 17:00 CT on, it is the next weekday's session.
 ********************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>

namespace irl {

// the trading session (yyyy-mm-dd) a local CT time belongs to: from 17:00 it is the next weekday's session
inline std::string sessionOf(int y, int mo, int d, int hh)
{
    struct tm t; memset(&t, 0, sizeof(t));
    t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = 12;
    time_t s = mktime(&t);
    if (hh >= 17) s += 86400;
    struct tm u = *localtime(&s);
    while (u.tm_wday == 6 || u.tm_wday == 0) { s += 86400; u = *localtime(&s); }   // Saturday / Sunday -> Monday
    char b[40]; snprintf(b, sizeof(b), "%04d-%02d-%02d", u.tm_year + 1900, u.tm_mon + 1, u.tm_mday);
    return b;
}

// who traded: 'B' bought (at / above the ask), 'S' sold (at / below the bid), 'M' between (or no quote)
inline char aggressor(double price, double bid, double ask)
{
    if (ask > 0 && price >= ask - 1e-9) return 'B';
    if (bid > 0 && price <= bid + 1e-9) return 'S';
    return 'M';
}

inline std::string stamp(int y, int mo, int d, int hh, int mi, int ss)
{
    char b[24]; snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d", y, mo, d, hh, mi, ss);
    return b;
}

// a price as text with enough decimals for the market's tick (6E 0.00005, HG 0.0005, ES 0.25 ...)
inline std::string px(double v, double tick)
{
    int dec = 2;
    if (tick > 0) { dec = 0; double t = tick; while (dec < 6 && std::fabs(t - std::round(t)) > 1e-9) { t *= 10; dec++; } }
    char b[32]; snprintf(b, sizeof(b), "%.*f", dec, v);
    return b;
}

// trades that share a second: the feed stamps trades to the second, so a restart resumes after the last written second and
// after the number of trades already written in it (no duplicate, no gap)
struct TickCursor {
    long long lastSec = -1; int doneInSec = 0;
    // true = write this trade (and move the cursor)
    bool take(long long sec)
    {
        if (sec < lastSec) return false;
        if (sec == lastSec) { if (seen < doneInSec) { seen++; return false; } doneInSec++; seen++; return true; }
        lastSec = sec; doneInSec = 1; seen = 1; return true;
    }
    void rewind() { seen = 0; }            // call before re-reading the trades from lastSec
    int seen = 0;
};

// a DOM snapshot is written only when a level changed: a cheap fingerprint of the book
inline unsigned long long bookHash(const std::vector<double>& v)
{
    unsigned long long h = 1469598103934665603ULL;
    for (size_t i = 0; i < v.size(); i++) {
        long long q = (long long)std::llround(v[i] * 100000.0);
        for (int k = 0; k < 8; k++) { h ^= (unsigned long long)((q >> (8 * k)) & 0xFF); h *= 1099511628211ULL; }
    }
    return h;
}

}  // namespace irl
