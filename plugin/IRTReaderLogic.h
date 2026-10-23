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
#include <limits>
#include <fstream>
#include <map>
#include <set>

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

// Market-data prices originate as floats.  Compare them on the instrument's
// native tick grid rather than with an arbitrary absolute epsilon, which is
// too small for some float magnitudes and too large for some small ticks.
inline bool toTicks(double price, double tick, long long& out)
{
    if (!std::isfinite(price) || !std::isfinite(tick) || tick <= 0.0) return false;
    const double q = price / tick;
    const double lo = (double)(std::numeric_limits<long long>::min)();
    const double hi = (double)(std::numeric_limits<long long>::max)();
    if (q < lo || q > hi) return false;
    out = (long long)std::llround(q);
    return true;
}

inline char aggressor(double price, double bid, double ask, double tick)
{
    long long p = 0, q = 0;
    if (ask > 0.0 && toTicks(price, tick, p) && toTicks(ask, tick, q) && p >= q) return 'B';
    if (bid > 0.0 && toTicks(price, tick, p) && toTicks(bid, tick, q) && p <= q) return 'S';
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
    const long long lo = (std::numeric_limits<long long>::min)();
    const long long hi = (std::numeric_limits<long long>::max)();
    unsigned long long h = 1469598103934665603ULL;
    for (size_t i = 0; i < v.size(); i++) {
        // Feed data can be unavailable/invalid during reconnects.  Never pass a
        // non-finite or out-of-range value to llround/cast; encode each class
        // distinctly so a bad snapshot is neither undefined behavior nor equal
        // to an ordinary zero level.
        long long q;
        if (std::isnan(v[i])) q = lo;
        else if (v[i] == std::numeric_limits<double>::infinity()) q = hi;
        else if (v[i] == -std::numeric_limits<double>::infinity()) q = lo + 1;
        else {
            const double scaled = v[i] * 100000.0;
            q = scaled >= (double)hi ? hi : scaled <= (double)lo ? lo : (long long)std::llround(scaled);
        }
        for (int k = 0; k < 8; k++) { h ^= (unsigned long long)((q >> (8 * k)) & 0xFF); h *= 1099511628211ULL; }
    }
    return h;
}

// Publish a complete replacement without exposing a truncated cursor/status
// file.  std::rename does not replace an existing target on Windows, so retain
// the previous good file as .bak until the new file is in place.
inline bool atomicWriteText(const std::string& path, const std::string& text)
{
    const std::string tmp = path + ".tmp";
    const std::string bak = path + ".bak";
    {
        std::ifstream target(path.c_str(), std::ios::binary);
        if (!target.good()) {
            std::ifstream backup(bak.c_str(), std::ios::binary);
            if (backup.good()) {
                backup.close();
                (void)std::rename(bak.c_str(), path.c_str());
            }
        }
    }
    {
        std::ofstream out(tmp.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!out.is_open()) return false;
        out << text;
        out.flush();
        if (!out.good()) { out.close(); std::remove(tmp.c_str()); return false; }
    }
    std::ifstream existing(path.c_str(), std::ios::binary);
    const bool hadExisting = existing.good();
    existing.close();
    if (hadExisting) {
        std::remove(bak.c_str());
        if (std::rename(path.c_str(), bak.c_str()) != 0) { std::remove(tmp.c_str()); return false; }
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        if (hadExisting) (void)std::rename(bak.c_str(), path.c_str());
        std::remove(tmp.c_str());
        return false;
    }
    if (hadExisting) std::remove(bak.c_str());
    return true;
}

// ======================================================================================================================
// (0.8.0, 2026-10-14 evening) the depth recorder's rules - DOM snapshots, order-by-order, coverage, stuck footprint bars.
// ======================================================================================================================
static const char* const DEPTH_VERSION = "0.8.0";

// a DOM snapshot is written only when it is a real book: v = [bid, bid_size, ask, ask_size] per level from level 0. Levels with no
// price on a side are ignored on that side; the prices present must be on the tick grid, bids strictly falling and asks strictly
// rising with the level, and the best bid under the best ask. (10-14 08:26:06 ES: level 1 bid 7814.00 below level 5 bid 7825.50 -
// a book read while IRT rebuilt it.) why = the first problem found.
inline bool domValid(const std::vector<double>& v, double tick, std::string& why)
{
    why.clear();
    if (v.size() % 4 != 0) { why = "odd level data"; return false; }
    if (!std::isfinite(tick) || tick <= 0) { why = "no tick size"; return false; }
    long long lastBid = 0, lastAsk = 0, bestBid = 0, bestAsk = 0; int nb = 0, na = 0;
    for (size_t i = 0; i + 3 < v.size(); i += 4) {
        const double b = v[i], bs = v[i + 1], a = v[i + 2], as = v[i + 3];
        if (!std::isfinite(b) || !std::isfinite(a) || !std::isfinite(bs) || !std::isfinite(as)) { why = "non-finite level"; return false; }
        if (bs < 0 || as < 0) { why = "negative size"; return false; }
        const int lvl = (int)(i / 4);
        if (b > 0 && bs > 0) {
            long long t = 0; if (!toTicks(b, tick, t) || std::fabs(b / tick - (double)t) > 1e-3) { why = "bid off the tick grid at level " + std::to_string(lvl); return false; }
            if (nb && t >= lastBid) { why = "bids out of order at level " + std::to_string(lvl); return false; }
            if (!nb) bestBid = t;
            lastBid = t; ++nb;
        }
        if (a > 0 && as > 0) {
            long long t = 0; if (!toTicks(a, tick, t) || std::fabs(a / tick - (double)t) > 1e-3) { why = "ask off the tick grid at level " + std::to_string(lvl); return false; }
            if (na && t <= lastAsk) { why = "asks out of order at level " + std::to_string(lvl); return false; }
            if (!na) bestAsk = t;
            lastAsk = t; ++na;
        }
    }
    if (!nb && !na) { why = "empty book"; return false; }
    if (nb && na && bestBid >= bestAsk) { why = "crossed book"; return false; }
    return true;
}

// hour file name of a CT hour: dom_09.csv / dbo_09.csv
inline std::string hourFile(const char* kind, int hh) { char b[32]; snprintf(b, sizeof(b), "%s_%02d.csv", kind, hh < 0 ? 0 : hh > 23 ? 23 : hh); return b; }

// DBO arrival ordering belongs to the subscribed market/contract, not the
// chart period.  A period-only chart rebind recreates the SDK subscription;
// retaining the arrival clock keeps a resent resting order with an older
// timestamp from being written before the last row already recorded.
inline bool sameMboStream(const std::string& oldMarket, const std::string& oldSymbol,
                          const std::string& newMarket, const std::string& newSymbol)
{
    return !oldMarket.empty() && !oldSymbol.empty() && oldMarket == newMarket && oldSymbol == newSymbol;
}

// (0.8.0) a footprint bar whose volume-at-price IRT never loads (HG 2026-10-09 16:00, the maintenance boundary: the cursor stayed
// there 5 days, 2,006 "not loaded yet" passes, nothing written) is skipped - and logged - once it failed `tries` passes AND it is
// either 10+ minutes old or on the 16:00-17:00 CT boundary. A bar that loads late is still written if it loads before that.
inline bool skipStuckBar(int failures, long long ageSec, int hh, int tries = 5)
{
    if (failures < tries) return false;
    return ageSec >= 600 || hh == 16 || (hh == 17 && ageSec >= 120);
}

// (0.8.0) COVERAGE SELF-CHECK of the order-by-order record: the FILL lots read in a closed chart bar vs its volume. Bars are judged
// once, >= 5 s after they close; the bar the stream opened in is partial and skipped by the host.
struct FillCoverage {
    std::map<long long, long long> fillsAt;          // event second (RTDATE) -> FILL lots read
    std::map<long long, long long> pend;             // bar end -> ms first seen closed
    std::set<long long> done;
    long long lastEnd = 0, lastFills = 0, lastVol = 0; int lastPct = -1;
    long long lowEnd = 0, lowFills = 0, lowVol = 0; int lowPct = -1;
    long long checked = 0, sumFills = 0, sumVol = 0, recentF[10] = {0}, recentV[10] = {0}; int recentN = 0;
    void addFill(long long stamp, long long lots) { if (stamp <= 0 || lots <= 0 || lots > 10000000) return; fillsAt[stamp] += lots; if (fillsAt.size() > 20000) fillsAt.erase(fillsAt.begin()); }
    long long fillsIn(long long from, long long to) const { long long t = 0; for (std::map<long long, long long>::const_iterator i = fillsAt.lower_bound(from); i != fillsAt.end() && i->first < to; ++i) t += i->second; return t; }
    static int pctOf(long long f, long long v) { if (v <= 0) return -1; const long long p = (f * 100 + v / 2) / v; return (int)(p > 999 ? 999 : p); }
    bool offer(long long end, int spb, long long vol, long long nowMs, long long waitMs = 5000) {
        if (end <= 0 || spb <= 0 || done.count(end)) return false;
        std::map<long long, long long>::iterator p = pend.find(end);
        if (p == pend.end()) { pend[end] = nowMs; if (pend.size() > 50) pend.erase(pend.begin()); return false; }
        if (nowMs >= p->second && nowMs - p->second < waitMs) return false;
        pend.erase(p); done.insert(end); if (done.size() > 3000) done.erase(done.begin());
        const long long f = fillsIn(end - spb, end); const int pct = pctOf(f, vol);
        if (pct < 0) return true;
        lastEnd = end; lastFills = f; lastVol = vol; lastPct = pct;
        recentF[recentN % 10] = f; recentV[recentN % 10] = vol; ++recentN;
        ++checked; sumFills += f; sumVol += vol;
        if (lowPct < 0 || (pct < lowPct && vol >= 20)) { lowEnd = end; lowFills = f; lowVol = vol; lowPct = pct; }
        return true;
    }
    int recentPct() const { long long f = 0, v = 0; for (int i = 0; i < 10 && i < recentN; ++i) { f += recentF[i]; v += recentV[i]; } return pctOf(f, v); }
};

// 12-hour Central clock text for the status file ("3:24:05 PM")
inline std::string clock12(int hh, int mm, int ss)
{
    char b[24]; const int h = hh % 12 == 0 ? 12 : hh % 12;
    snprintf(b, sizeof(b), "%d:%02d:%02d %s", h, mm, ss, hh < 12 ? "AM" : "PM"); return b;
}

}  // namespace irl
