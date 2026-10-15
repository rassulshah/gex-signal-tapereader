#ifndef LS_BIGORDERLINES_H
#define LS_BIGORDERLINES_H
// BigOrderLines.h 1.0.0 (2026-10-14 night) -- the BIGGEST RESTING ORDER LINES of lsLiquidityProfile 0.18.0. SDK-free; tested by
// tests_lqp/test_bigorders_logic.cpp (+ the host suite test_bigorders_host.cpp) and replayed on his recorded order-by-order files
// (tests_lqp/replay_bigorders.cpp).
//
// Rassul approved 2026-10-14 22:11 CT (mockup v2 "mockup-big-resting-orders.png"): on the PRICE chart, at most 2 live lines -
//   the single biggest DISPLAYED resting BID order within 2 ATR below price (green) and the single biggest resting OFFER within 2 ATR
//   above (red). ATR = the 14-bar ATR of the chart's own closed bars. "Order" = one MBO order id (its displayed size), never a price
//   level total. A per-market minimum size: the session's own top 0.1% of displayed order sizes so far (past-only) with a hard floor
//   per market (a setting). Each line starts at the bar when that order appeared / became the biggest within reach, at its price,
//   labelled at its left end "Bid 120" / "Ask 95" (the size when the line started), extends bar by bar while the order rests and ends
//   ONCE: filled dot (it traded away), X (pulled / moved), arrow (still resting), a plain end (a bigger order took over, it left the
//   reach, the feed lost it, IRT restarted, the session ended). NEVER repaints: a line only extends, then ends once; ended lines stay
//   (the last N per side are drawn). Hover: size, price, time appeared (12-hour CT), how long it rested, lots traded vs cancelled,
//   filled / pulled / resting, what it answers / how to use it / an example, and the limit: displayed size only.
//
// THE FEED (as recorded in data\mbo\live, and as OrderFlowFeed.h reads it): NEW / MOD / DEL / FILL per order id. A FILL row carries the
// AGGRESSOR's side (ES 10/14: 197,924 + 206,018 fills, every one on the side opposite the order's own NEW), so the order's side is
// always taken from the book, never from the fill. A fill is followed by a MOD with the remaining size (a size sync, not a cancel);
// a full fill ends with FILL(remaining) + DEL(remaining). An order first seen by a MOD rested before the chart / IRT was opened: its
// start is NOT known and the hover says so ("since restart" / "since the chart opened") - an age is never invented.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <map>
#include <set>
#include <string>
#include <deque>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bol {
static const char* const VERSION = "1.3.0";   // (1.3.0) biggest(): the biggest order each side within reach for the BIG column (display only, sticky +20%);   // (1.2.0) self-tuning big size: k-th largest new order near price in the last 60 min (~3 per side per hour), nightly prior at cold start, +/-20% per minute   // (1.1.0) depthMin: large = also >= 3x the typical displayed size within reach
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
// how a line ended. FILLED = a dot, PULLED / MOVED = an X, RESTING = the arrow (live); the rest are plain ends
enum End { RESTING = 0, FILLED = 1, PULLED = 2, MOVED = 3, OVER = 4, LEFT = 5, LOST = 6, RESTART = 7, SESSION = 8 };
inline bool endOk(long long e) { return e >= RESTING && e <= SESSION; }
inline const char* endWord(int e) {
    switch (e) { case FILLED: return "FILLED"; case PULLED: return "PULLED"; case MOVED: return "MOVED"; case OVER: return "TAKEN OVER";
    case LEFT: return "LEFT REACH"; case LOST: return "NOT SEEN"; case RESTART: return "RESTART"; case SESSION: return "SESSION END"; default: return "RESTING"; }
}
inline const char* endCode(int e) {
    switch (e) { case FILLED: return "FILL"; case PULLED: return "PULL"; case MOVED: return "MOVE"; case OVER: return "OVER"; case LEFT: return "LEFT";
    case LOST: return "LOST"; case RESTART: return "RESTART"; case SESSION: return "SESSION"; default: return "REST"; }
}
inline int endOf(const std::string& c) {
    for (int e = RESTING; e <= SESSION; ++e) if (c == endCode(e)) return e;
    return -1;
}
inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }

// ---- per-market defaults: the hard floor (lots) under the session's own top-0.1% line --------------------------------------------
// Chosen from his data (2026-10-14 night): displayed NEW order sizes in data\mbo\live (ES 10/14 2.39M orders: p99.9 = 15, p99.99 =
// 50, 950 orders >= 30 in 9.8 h; NQ 10/12 1.55M: p99.9 5, 378 >= 10; GC 10/12 0.74M: p99.9 5, 418 >= 10; HG 10/12 0.19M: p99.9 4,
// 76 >= 8; 6E 10/12 0.19M: p99.9 32, 46 >= 50; CL: too little recorded (p99.9 3 on 2.4k orders) - 15 from the makeup files' top
// order (16)); the Liquidity Profile makeup files' "top order" in a zone (ES median 20, max 53; GC max 20; HG max 20; NQ max 10).
// Then replayed through this engine (10 s dwell, 60 s leave): ES 10/14 floor 30 = 133 lines in 9.8 h (<= 20 an hour); NQ 10/12 floor
// 10 = 105; GC 10/12 floor 10 = 391 (70 in the 07:00 hour, 168 ended by algos re-pricing) -> 15 = 78; HG floor 8 = 125 (43 in one
// hour, one 10-lot order re-pricing 68 times) -> 12 = 24; 6E floor 50 = 24. CL / NG: too little recorded - their floors are guesses
// until the nightly measures them (the setting overrides).
inline long long floorFor(const std::string& m) {
    if (m == "ES") return 30;
    if (m == "NQ") return 10;
    if (m == "GC") return 15;
    if (m == "HG") return 12;
    if (m == "CL") return 15;
    if (m == "NG") return 10;
    if (m == "EU") return 50;
    return 20;
}
static const long long FLOOR_MIN = 1, FLOOR_MAX = 100000;
static const double REACH_MIN = 0.5, REACH_MAX = 5.0, REACH_DEF = 2.0;
static const int KEEP_MIN = 0, KEEP_MAX = 50, KEEP_DEF = 10;
static const int FONT_MIN = 7, FONT_MAX = 16, FONT_DEF = 10;   // (LP 0.23.0, Rassul 12:01 "for big trades also add font size ... make sure it works") 7-16, default 10

struct Params {
    double reachAtr = REACH_DEF;       // the reach: this many ATRs from price
    long long floor = 20;              // hard minimum, lots
    double pct = 0.999;                // the session's own top 0.1% of displayed order sizes (past-only)
    long long pctMinN = 1000;          // the percentile is used once this many orders were seen this session
    double keepX = 1.25;               // a live line leaves the reach only past 1.25 x the reach (no flapping at the edge)
    long long lostMs = 5000;           // the depth book showed less than the order at its price for this long: the feed lost it
    long long dwellMs = 10000;         // a candidate must stay the biggest this long (or trade first) before its line is drawn
    long long leaveMs = 60000;         // a live line ends LEFT only after its order stayed out of reach this long
    int keep = KEEP_DEF;               // ended lines drawn per side
    size_t bookCap = 200000;           // orders tracked (far ones are dropped first; ES rests ~20-50k at a time)
    size_t linesCap = 20000;           // lines kept in memory per session (the file keeps all)
    // (1.2.0, Rassul 2026-10-15 "optimizing the size based on recent history automatically") the SELF-TUNING big size
    bool roll = true;                  // off when he sets his own "min lots"
    long long rollMs = 3600000;        // learn from the last 60 min of new orders near price
    double rollTarget = 3.0;           // about this many qualifying orders per side per hour
    size_t rollMinN = 50;              // orders in the window before the rolling size is used (cold start: the nightly prior)
    double rollStep = 0.20;            // the size moves at most 20% per minute (no flicker)
    long long rollEveryMs = 60000;
};

// ---- the size distribution (past-only) ----------------------------------------------------------------------------------------
struct Hist {
    static const int BINS = 4096;      // sizes 1..4095 one bin each, the rest in the last
    std::vector<long long> c; long long n = 0; long long cached = 0, cachedAt = -1;
    Hist() : c(BINS, 0) {}
    void add(long long s) { if (s <= 0) return; c[(size_t)std::min<long long>(s, BINS - 1)]++; ++n; }
    // the smallest size s with at least (1 - q) of the orders at s or above; recomputed every 256 orders (a function of n only)
    long long pct(double q) {
        if (n <= 0) return 0;
        const long long at = n / 256;
        if (at == cachedAt) return cached;
        const long long want = std::max<long long>(1, (long long)std::ceil((1.0 - q) * (double)n - 1e-9));   // (1 - 0.999 is 0.0010000000000000009)
        long long acc = 0, s = BINS - 1;
        for (; s > 0; --s) { acc += c[(size_t)s]; if (acc >= want) break; }
        cached = std::max<long long>(1, s); cachedAt = at; return cached;
    }
    void clear() { std::fill(c.begin(), c.end(), 0); n = 0; cached = 0; cachedAt = -1; }
};

struct Ord {
    int side = BID; Tick px = 0;
    long long size = 0, orig = 0, traded = 0, cancelled = 0;
    long long tNew = 0;                // when it appeared (CT ms), or first seen
    bool startKnown = true;            // false: first seen by a MOD (rested before the chart / IRT opened)
    bool filledLast = false;           // the last event was a fill (the next MOD / DEL is its sync, not a cancel)
};

struct Line {
    long long key = 0, oid = 0; int side = BID; Tick px = 0;
    long long size0 = 0;               // the size when the line started (the label)
    long long tStart = 0, tEnd = 0;    // CT ms; tEnd 0 while resting
    long long tNew = 0;                // when the order appeared / was first seen
    bool startKnown = true, sinceRestart = false;
    int end = RESTING;
    long long traded = 0, cancelled = 0, left = 0;   // at the end (live: now)
    long long thr = 0;                 // the minimum size in force when it started
    long long overBy = 0;              // OVER: the size of the order that took over
    Tick movedTo = 0;                  // MOVED: the new price
    bool restored = false;             // read back from the session file
};

// ---- text helpers --------------------------------------------------------------------------------------------------------------
inline int decimals(double tick) {
    if (!(tick > 0) || !finiteD(tick)) return 2;
    for (int d = 0; d <= 6; ++d) { const double p = std::pow(10.0, d), v = tick * p; if (std::fabs(v - std::round(v)) < 1e-7) return d; }
    return 6;
}
inline std::string price(Tick t, double tick) {
    const int d = decimals(tick); char b[48]; std::snprintf(b, sizeof(b), "%.*f", d, (double)t * tick);
    std::string r = b; const size_t dot = r.find('.'); int i = (int)(dot == std::string::npos ? r.size() : dot);
    const int stop = r[0] == '-' ? 1 : 0;
    while ((i -= 3) > stop) r.insert((size_t)i, ",");
    return r;
}
// "6:57 AM" from Central ms (epoch of the Central wall clock); "?" when unknown
inline std::string clock12(long long ms) {
    if (ms <= 0) return "?";
    long long s = (ms / 1000) % 86400; if (s < 0) s += 86400;
    int h = (int)(s / 3600), m = (int)(s / 60 % 60); const bool pm = h >= 12; h %= 12; if (h == 0) h = 12;
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h, m, pm ? "PM" : "AM"); return b;
}
inline std::string dur(long long ms) {
    if (ms < 0) ms = 0;
    const long long s = ms / 1000; char b[32];
    if (s < 60) std::snprintf(b, sizeof(b), "%lld s", s);
    else if (s < 3600) std::snprintf(b, sizeof(b), "%lld min %lld s", s / 60, s % 60);
    else std::snprintf(b, sizeof(b), "%lld h %lld min", s / 3600, s / 60 % 60);
    return b;
}
inline std::string label(const Line& L) { return std::string(L.side == BID ? "Bid " : "Ask ") + std::to_string(L.size0); }

// ---- the engine ----------------------------------------------------------------------------------------------------------------
class Engine {
public:
    Params P; double tick = 0.25;
    std::unordered_map<long long, Ord> book;
    std::set<long long> big[2];        // ids resting with size >= the floor (the only candidates)
    Hist hist;
    Tick px = 0; bool havePx = false;  // the last trade (a FILL's price), else the chart's close
    bool pxFromFill = false;
    Tick reach = 0;                    // ticks; 0 = no ATR yet: nothing is drawn
    double atrPrice = 0;
    std::vector<Line> lines; int live[2] = {-1, -1};
    long long seq = 0, lineN[2] = {0, 0};
    bool restarted = false;            // this session's file existed: orders first seen now rested before the restart
    long long openedMs = 0;            // when this engine started listening (the restart / chart open time)
    std::vector<std::string> rows;     // record rows waiting to be written (S / E)
    long long events = 0, unknown = 0, garbage = 0, drops = 0, lostEnds = 0;
    long long lastT = 0;
    long long depthMin = 0;
    std::deque<std::pair<long long, long long>> recent;   // (1.2.0) (time, size) of new orders within reach, last rollMs
    long long rollThr = 0, rollAt = 0, prior = 0;            // the tuned size, when it last moved, the nightly prior for this hour
    void roll(long long t) {
        while (!recent.empty() && recent.front().first < t - P.rollMs) recent.pop_front();
        if (!P.roll) { rollThr = 0; return; }
        if (rollAt > 0 && t >= rollAt && t - rollAt < P.rollEveryMs) return;
        rollAt = t; long long target = 0;
        if (recent.size() >= P.rollMinN) {
            const double cover = std::min(1.0, std::max(1.0 / 60.0, (double)(t - recent.front().first) / (double)P.rollMs));
            const size_t k = (size_t)std::max(1.0, std::floor(P.rollTarget * 2.0 * cover + 0.5));
            std::vector<long long> v; v.reserve(recent.size()); for (size_t i = 0; i < recent.size(); ++i) v.push_back(recent[i].second);
            const size_t kk = std::min(k, v.size()) - 1;
            std::nth_element(v.begin(), v.begin() + (long)kk, v.end(), std::greater<long long>()); target = v[kk];
        } else target = prior;                                    // cold start: the nightly hourly norm (0 = none)
        if (target <= 0) return;
        if (rollThr <= 0) { rollThr = target; return; }
        const double lo = rollThr * (1.0 - P.rollStep), hi = rollThr * (1.0 + P.rollStep);
        rollThr = (long long)std::llround(std::max(lo, std::min(hi, (double)target)));
        if (rollThr < 1) rollThr = 1;
    }            // (1.1.0, Rassul 10-15) also >= 3x the typical displayed size within reach (set live by the host)

    // a new floor (the setting changed): the candidate sets are rebuilt from the book
    void setFloor(long long f) {
        f = std::max(FLOOR_MIN, std::min(FLOOR_MAX, f));
        if (f == P.floor) return;
        P.floor = f; big[0].clear(); big[1].clear();
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) rebig(it->first, it->second);
    }
    long long threshold() {
        long long t = P.floor;
        if (P.roll && rollThr > 0) t = std::max(t, rollThr);        // (1.2.0) the self-tuned size replaces the session percentile
        else if (hist.n >= P.pctMinN) t = std::max(t, hist.pct(P.pct));
        return std::max(t, depthMin);
    }
    void setAtr(double a) {
        atrPrice = a > 0 && finiteD(a) ? a : 0;
        reach = atrPrice > 0 && tick > 0 ? (Tick)std::llround(P.reachAtr * atrPrice / tick) : 0;
        if (reach < 0) reach = 0;
    }
    void setPrice(Tick p, bool fromFill) {
        if (p <= 0) return;
        if (!fromFill && pxFromFill) return;           // the chart's close only until the feed has traded
        px = p; havePx = true; pxFromFill = pxFromFill || fromFill;
    }
    // distance from price on the line's own side: a bid below price / an offer above it is >= 0
    Tick dist(int side, Tick p) const { return side == BID ? px - p : p - px; }
    bool inReach(int side, Tick p, double x = 1.0) const {
        if (!havePx || reach <= 0) return false;
        const Tick d = dist(side, p);
        return d >= -1 && (double)d <= (double)reach * x + 1e-9;   // 1 tick of tolerance on the wrong side (the spread)
    }

    // one event of the feed. side = the row's side (ignored for an order already in the book: a FILL carries the aggressor's)
    void onEvent(int action, int side, Tick p, long long size, long long id, long long t) {
        if (t > 0) lastT = std::max(lastT, t);
        if (action < NEW || action > FILL || size < 0 || size > 100000000LL || p <= 0) { garbage++; return; }
        events++;
        std::unordered_map<long long, Ord>::iterator it = book.find(id);
        switch (action) {
        case NEW: {
            if (side != BID && side != ASK) { garbage++; return; }
            if (it != book.end()) {                       // a replayed NEW: a fresh appearance at its (maybe new) price
                if (it->second.px != p) moved(id, it->second, p, t);
                unbig(id, it->second);
                book.erase(it);
            }
            Ord o; o.side = side; o.px = p; o.size = size; o.orig = size; o.tNew = t; o.startKnown = true;
            book[id] = o; rebig(id, book[id]);
            evaluate(t);
            hist.add(size);                               // past-only: its own size joins the distribution after it was judged
            if (inReach(side, p)) recent.push_back(std::make_pair(t, size));   // (1.2.0) near-price arrivals feed the self-tuned size
            break; }
        case MOD: {
            if (it == book.end()) {                       // first sight: it rested before we were listening
                if (side != BID && side != ASK) { garbage++; return; }
                unknown++;
                Ord o; o.side = side; o.px = p; o.size = size; o.orig = size; o.tNew = t; o.startKnown = false;
                book[id] = o; rebig(id, book[id]);
                evaluate(t);
                break;
            }
            Ord& o = it->second;
            if (p != o.px) {                              // moved: an X at the old price, a fresh order at the new one
                moved(id, o, p, t);
                unbig(id, o);
                o.px = p; o.size = size; o.orig = size; o.traded = 0; o.cancelled = 0; o.tNew = t; o.startKnown = true; o.filledLast = false;
                rebig(id, o); evaluate(t);
                break;
            }
            const long long d = size - o.size; const bool afterFill = o.filledLast;
            if (d < 0 && !afterFill) o.cancelled += -d;   // after a fill a smaller MOD is the size sync, not a cancel
            o.size = size; o.filledLast = false;
            if (size == 0) { ended(id, o, afterFill ? FILLED : PULLED, t); unbig(id, o); book.erase(it); evaluate(t); break; }
            rebig(id, o); evaluate(t);
            break; }
        case FILL: {
            setPrice(p, true);
            if (it == book.end()) { unknown++; evaluate(t); break; }   // rested before we listened: nothing known to attribute
            Ord& o = it->second;
            const long long take = std::min(size, o.size);
            o.traded += size; o.size -= take; o.filledLast = true;
            if (o.size <= 0) { ended(id, o, FILLED, t); unbig(id, o); book.erase(it); evaluate(t); break; }
            rebig(id, o); evaluate(t);
            break; }
        case DEL: {
            if (it == book.end()) { unknown++; break; }
            Ord& o = it->second;
            if (o.size > 0) o.cancelled += o.size;        // what was still showing was cancelled (after a partial fill too)
            ended(id, o, o.size > 0 ? PULLED : FILLED, t);
            unbig(id, o); book.erase(it); evaluate(t);
            break; }
        }
        if (book.size() > P.bookCap) trim();
    }

    // (1.3.0, Rassul 2026-10-15 13:51-13:55 "biggest above and below within 2 ATR ... dimmed", mockup-big-column-tiers-v1) DISPLAY ONLY
    // (no line, no record): the biggest single resting order on side s within the reach, at least minShow lots. Sticky: the shown
    // order stays until another is 20% bigger, or it is filled / pulled / out of reach - no jumping box.
    long long showId[2] = {0, 0};
    bool biggest(int s, long long minShow, Tick& outPx, long long& outSize) {
        if (s < 0 || s > 1 || !havePx || reach <= 0) { if (s >= 0 && s <= 1) showId[s] = 0; return false; }
        if (minShow < 1) minShow = 1;
        const Ord* cur = nullptr;
        if (showId[s] != 0) {
            std::unordered_map<long long, Ord>::const_iterator it = book.find(showId[s]);
            if (it != book.end() && it->second.side == s && it->second.size >= minShow && inReach(s, it->second.px)) cur = &it->second; else showId[s] = 0;
        }
        long long bestId = 0; const Ord* best = nullptr;
        for (std::set<long long>::const_iterator i = big[s].begin(); i != big[s].end(); ++i) {
            std::unordered_map<long long, Ord>::const_iterator it = book.find(*i);
            if (it == book.end()) continue;
            const Ord& o = it->second;
            if (o.size < minShow || !inReach(s, o.px)) continue;
            if (!best || o.size > best->size || (o.size == best->size && *i < bestId)) { best = &o; bestId = *i; }
        }
        if (cur && (!best || (double)best->size < 1.2 * (double)cur->size)) { best = cur; bestId = showId[s]; }
        if (!best) { showId[s] = 0; return false; }
        showId[s] = bestId; outPx = best->px; outSize = best->size; return true;
    }
    // the selection: the biggest order on each side within the reach (size >= the minimum), else nothing
    void evaluate(long long t) {
        if (!havePx || reach <= 0) return;
        const long long thr = threshold();
        for (int s = 0; s < 2; ++s) {
            if (live[s] >= 0) {                           // the live line: out of the reach (past keepX x reach) for leaveMs = LEFT
                Line& L = lines[(size_t)live[s]];
                std::unordered_map<long long, Ord>::const_iterator it = book.find(L.oid);
                if (it == book.end()) finish(s, LOST, t, 0);  // (trimmed away: cannot normally happen, ended() runs first)
                else if (!inReach(s, L.px, P.keepX)) { if (outSince[s] <= 0) outSince[s] = t; else if (t - outSince[s] >= P.leaveMs) finish(s, LEFT, t, 0); }
                else outSince[s] = 0;
            }
            long long bestId = 0; const Ord* best = nullptr;
            for (std::set<long long>::const_iterator i = big[s].begin(); i != big[s].end(); ++i) {
                if (live[s] >= 0 && *i == lines[(size_t)live[s]].oid) continue;
                std::unordered_map<long long, Ord>::const_iterator it = book.find(*i);
                if (it == book.end()) continue;
                const Ord& o = it->second;
                if (o.size < thr || !inReach(s, o.px)) continue;
                if (!best || o.size > best->size || (o.size == best->size && (o.tNew < best->tNew || (o.tNew == best->tNew && *i < bestId)))) { best = &o; bestId = *i; }
            }
            if (best && live[s] >= 0) {                   // a bigger one only takes over when strictly bigger than what the line's order shows
                std::unordered_map<long long, Ord>::const_iterator cur = book.find(lines[(size_t)live[s]].oid);
                if (cur != book.end() && best->size <= cur->second.size) best = nullptr;
            }
            if (!best) { pendId[s] = 0; pendSince[s] = 0; continue; }
            // DWELL: the candidate must stay the biggest for dwellMs (or trade before that) - flashed orders never draw
            if (pendId[s] != bestId) { pendId[s] = bestId; pendSince[s] = t; pendTraded[s] = best->traded; pendSize[s] = best->size; }
            const bool hit = best->traded > pendTraded[s];
            if (t - pendSince[s] < P.dwellMs && !hit) continue;
            if (live[s] >= 0) finish(s, OVER, t, best->size);   // ends NOW (never shortened: no repaint)
            start(s, bestId, *best, pendSince[s], thr, pendSize[s]);
            pendId[s] = 0; pendSince[s] = 0;
        }
    }
    // the depth book (MARKET_DEPTH) at a live line's price: less than the order there for lostMs = the order feed lost its exit
    // clk = a steady clock (ms) for the lostMs wait; t = the Central time the line ends at
    void checkBook(int s, Tick p, double levelLots, bool known, long long clk, long long t) {
        if (live[s] < 0 || !known) { lowSince[s] = 0; return; }
        Line& L = lines[(size_t)live[s]];
        if (L.px != p) return;
        std::unordered_map<long long, Ord>::const_iterator it = book.find(L.oid);
        if (it == book.end()) return;
        if (levelLots + 0.5 < (double)it->second.size) {
            if (lowSince[s] <= 0) lowSince[s] = clk;
            else if (clk - lowSince[s] >= P.lostMs) {
                Ord o = it->second; lostEnds++;
                finish(s, LOST, t, 0);
                unbig(L.oid, o); book.erase(L.oid); lowSince[s] = 0; evaluate(t);
            }
        } else lowSince[s] = 0;
    }
    // a new session (17:00 CT): endSession() ends the live lines plainly (their E rows go to the OLD session's file), then
    // clearSession() starts the lines / distribution over; the book stays (orders keep resting across the boundary)
    void endSession(long long t) { for (int s = 0; s < 2; ++s) if (live[s] >= 0) finish(s, SESSION, t, 0); }
    void clearSession() {
        lines.clear(); live[0] = live[1] = -1; hist.clear(); seq = 0; lineN[0] = lineN[1] = 0; restarted = false; rows.clear();
        for (int q = 0; q < 2; ++q) { pendId[q] = pendSince[q] = pendTraded[q] = pendSize[q] = outSince[q] = lowSince[q] = 0; }
    }
    // ---- the session record (append-only; read back after a restart: no repaint) ---------------------------------------------
    // S|key|oid|side|px|size0|tStart|tNew|known|sinceRestart|thr     E|key|tEnd|end|traded|cancelled|left|overBy|movedTo     H|t
    static std::string header() { return "BOL1 S|key|oid|side|px_ticks|size0|t_start|t_new|start_known|since_restart|min_lots  E|key|t_end|end|traded|cancelled|left|over_by|moved_to  H|t (Central ms)"; }
    static std::string sRow(const Line& L) {
        char b[256]; std::snprintf(b, sizeof(b), "S|%lld|%lld|%d|%lld|%lld|%lld|%lld|%d|%d|%lld", L.key, L.oid, L.side, L.px, L.size0, L.tStart, L.tNew,
                                   L.startKnown ? 1 : 0, L.sinceRestart ? 1 : 0, L.thr);
        return b;
    }
    static std::string eRow(const Line& L) {
        char b[256]; std::snprintf(b, sizeof(b), "E|%lld|%lld|%s|%lld|%lld|%lld|%lld|%lld", L.key, L.tEnd, endCode(L.end), L.traded, L.cancelled, L.left, L.overBy, L.movedTo);
        return b;
    }
    static std::string hRow(long long t) { return "H|" + std::to_string(t); }
    // read back a session's rows: lines that never ended in the file end at the last heartbeat after their start (IRT closed then:
    // what happened later is not known - a plain end). Returns the number of lines restored; bad rows are skipped and counted.
    size_t restore(const std::vector<std::string>& in, long long* bad = nullptr) {
        std::map<long long, Line> got; long long lastH = 0, nbad = 0;
        for (size_t i = 0; i < in.size(); ++i) {
            if (in[i].compare(0, 4, "BOL1") == 0) continue;   // the header
            const std::vector<std::string> f = split(in[i]);
            if (f.empty() || (f.size() == 1 && f[0].empty())) continue;
            if (f[0] == "H" && f.size() == 2) { long long v; if (num(f[1], 0, 9e15, v)) lastH = std::max(lastH, v); else ++nbad; continue; }
            if (f[0] == "S" && f.size() == 11) {
                Line L; long long sd = 0, kn = 0, sr = 0;
                if (!num(f[1], 1, 1e12, L.key) || !num(f[2], -9e18, 9e18, L.oid) || !num(f[3], 0, 1, sd) || !num(f[4], 1, 9e15, L.px) || !num(f[5], 1, 1e9, L.size0) ||
                    !num(f[6], 1, 9e15, L.tStart) || !num(f[7], 0, 9e15, L.tNew) || !num(f[8], 0, 1, kn) || !num(f[9], 0, 1, sr) || !num(f[10], 0, 1e9, L.thr)) { ++nbad; continue; }
                L.side = (int)sd; L.startKnown = kn != 0; L.sinceRestart = sr != 0; L.restored = true; L.left = L.size0;
                if (!got.count(L.key)) got[L.key] = L;
                continue;
            }
            if (f[0] == "E" && f.size() == 9) {
                long long key = 0, tEnd = 0, tr = 0, ca = 0, le = 0, ob = 0, mt = 0; const int e = endOf(f[3]);
                if (!num(f[1], 1, 1e12, key) || !num(f[2], 1, 9e15, tEnd) || e <= RESTING || !num(f[4], 0, 1e12, tr) || !num(f[5], 0, 1e12, ca) ||
                    !num(f[6], 0, 1e12, le) || !num(f[7], 0, 1e12, ob) || !num(f[8], 0, 9e15, mt)) { ++nbad; continue; }
                std::map<long long, Line>::iterator g = got.find(key);
                if (g == got.end() || g->second.end != RESTING || tEnd < g->second.tStart) { ++nbad; continue; }   // first E wins
                g->second.end = e; g->second.tEnd = tEnd; g->second.traded = tr; g->second.cancelled = ca; g->second.left = le; g->second.overBy = ob; g->second.movedTo = mt;
                continue;
            }
            ++nbad;
        }
        lines.clear(); live[0] = live[1] = -1; seq = 0; lineN[0] = lineN[1] = 0;
        for (std::map<long long, Line>::iterator g = got.begin(); g != got.end(); ++g) {
            Line L = g->second;
            if (L.end == RESTING) {                        // never ended in the file: IRT closed while it rested
                L.end = RESTART; L.tEnd = std::max(L.tStart, lastH);
                rows.push_back(eRow(L));
            }
            seq = std::max(seq, L.key); lineN[L.side]++; lines.push_back(L);
        }
        restarted = !in.empty();
        if (bad) *bad = nbad;
        return lines.size();
    }
    // the lines to draw: every live line + the last `keep` ended lines per side (newest first)
    std::vector<size_t> shown() const {
        std::vector<size_t> v; int n[2] = {0, 0};
        for (int s = 0; s < 2; ++s) if (live[s] >= 0) v.push_back((size_t)live[s]);
        for (size_t k = lines.size(); k-- > 0;) {
            const Line& L = lines[k]; if (L.end == RESTING) continue;
            if (n[L.side] >= P.keep) continue;
            n[L.side]++; v.push_back(k);
        }
        return v;
    }
    // a live line's numbers as of now (its order in the book)
    Line now(size_t k) const {
        Line L = lines[k];
        if (L.end == RESTING) {
            std::unordered_map<long long, Ord>::const_iterator it = book.find(L.oid);
            if (it != book.end()) { L.traded = it->second.traded; L.cancelled = it->second.cancelled; L.left = it->second.size; }
        }
        return L;
    }

    // ---- parsing helpers ----
    static std::vector<std::string> split(const std::string& s) {
        std::vector<std::string> f; size_t a = 0; std::string t = s;
        while (!t.empty() && (t.back() == '\r' || t.back() == '\n')) t.pop_back();
        for (size_t i = 0; i <= t.size(); ++i) if (i == t.size() || t[i] == '|') { f.push_back(t.substr(a, i - a)); a = i + 1; }
        return f;
    }
    static bool num(const std::string& t, double lo, double hi, long long& out) {
        if (t.empty() || t.size() > 20 || !((t[0] >= '0' && t[0] <= '9') || (t[0] == '-' && t.size() > 1))) return false;   // no spaces / signs
        errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
        if (errno || e != t.c_str() + t.size() || (double)v < lo || (double)v > hi) return false;
        out = v; return true;
    }

private:
    long long lowSince[2] = {0, 0}, outSince[2] = {0, 0};
    long long pendId[2] = {0, 0}, pendSince[2] = {0, 0}, pendTraded[2] = {0, 0}, pendSize[2] = {0, 0};
    void rebig(long long id, const Ord& o) { if (o.size >= P.floor) big[o.side].insert(id); else big[o.side].erase(id); }
    void unbig(long long id, const Ord& o) { big[o.side].erase(id); }
    int liveOf(long long id, int side) const { return live[side] >= 0 && lines[(size_t)live[side]].oid == id ? side : -1; }
    void start(int s, long long id, const Ord& o, long long t, long long thr, long long size0) {
        Line L; L.key = ++seq; L.oid = id; L.side = s; L.px = o.px; L.size0 = size0 > 0 ? size0 : o.size;   // the size when it became the biggest
        L.tStart = std::max(t, o.tNew); L.tNew = o.tNew;
        L.startKnown = o.startKnown; L.sinceRestart = !o.startKnown && restarted; L.thr = thr; L.left = o.size; L.traded = o.traded; L.cancelled = o.cancelled;
        if (lines.size() >= P.linesCap) {                  // bounded memory: forget the oldest ENDED line (the file keeps it)
            for (size_t k = 0; k < lines.size(); ++k) if (lines[k].end != RESTING) { lines.erase(lines.begin() + (long)k); drops++; break; }
            for (int q = 0; q < 2; ++q) { live[q] = -1; for (size_t k = 0; k < lines.size(); ++k) if (lines[k].end == RESTING && lines[k].side == q) live[q] = (int)k; }
        }
        lines.push_back(L); live[s] = (int)lines.size() - 1; lineN[s]++;
        rows.push_back(sRow(L));
    }
    void finish(int s, int how, long long t, long long overBy) {
        if (live[s] < 0) return;
        Line& L = lines[(size_t)live[s]];
        std::unordered_map<long long, Ord>::const_iterator it = book.find(L.oid);
        if (it != book.end()) { L.traded = it->second.traded; L.cancelled = it->second.cancelled; L.left = it->second.size; }
        L.end = how; L.tEnd = std::max(t, L.tStart); L.overBy = overBy;
        rows.push_back(eRow(L));
        live[s] = -1; lowSince[s] = 0; outSince[s] = 0;
    }
    // the order behind a live line left the book (filled / pulled)
    void ended(long long id, const Ord& o, int how, long long t) {
        const int s = liveOf(id, o.side); if (s < 0) return;
        Line& L = lines[(size_t)live[s]];
        L.traded = o.traded; L.cancelled = o.cancelled; L.left = how == FILLED ? 0 : o.size;
        L.end = how; L.tEnd = std::max(t, L.tStart); L.overBy = 0;
        if (how == PULLED) L.left = 0;
        rows.push_back(eRow(L));
        live[s] = -1; lowSince[s] = 0; outSince[s] = 0;
    }
    void moved(long long id, const Ord& o, Tick to, long long t) {
        const int s = liveOf(id, o.side); if (s < 0) return;
        Line& L = lines[(size_t)live[s]];
        L.traded = o.traded; L.cancelled = o.cancelled + o.size; L.left = 0; L.movedTo = to;
        L.end = MOVED; L.tEnd = std::max(t, L.tStart);
        rows.push_back(eRow(L));
        live[s] = -1; lowSince[s] = 0; outSince[s] = 0;
    }
    void trim() {
        const Tick far = reach > 0 ? 6 * reach : 4000;
        for (std::unordered_map<long long, Ord>::iterator it = book.begin(); it != book.end();) {
            const long long id = it->first; const Ord& o = it->second;
            const bool isLive = liveOf(id, o.side) >= 0;
            if (!isLive && (!havePx || std::llabs(o.px - px) > far)) { big[o.side].erase(id); it = book.erase(it); drops++; }
            else ++it;
        }
        if (book.size() > P.bookCap * 9 / 10) {            // still too many: keep only the live lines' orders
            std::unordered_map<long long, Ord> keepB;
            for (int s = 0; s < 2; ++s) if (live[s] >= 0) { std::unordered_map<long long, Ord>::iterator it = book.find(lines[(size_t)live[s]].oid); if (it != book.end()) keepB.insert(*it); }
            drops += (long long)(book.size() - keepB.size()); book.swap(keepB); big[0].clear(); big[1].clear();
            for (std::unordered_map<long long, Ord>::iterator it = book.begin(); it != book.end(); ++it) rebig(it->first, it->second);
        }
    }
};

// ---- the hover rows (<= 12 rows of <= 115 characters, by importance: lesson 83) --------------------------------------------------
enum Ink { INK_WHITE = 0, INK_GREEN = 1, INK_RED = 2, INK_GREY = 3 };
struct Row { std::string text; int ink = INK_WHITE; };
struct HoverCtx { double tick = 0.25; double reachAtr = 2.0; double atrPrice = 0; long long nowMs = 0; long long minLots = 0, floor = 0, pctLots = 0; std::string replacedBy; };
inline std::vector<Row> hoverRows(const Line& L, const HoverCtx& c) {
    std::vector<Row> v; const bool bid = L.side == BID; const int ink = bid ? INK_GREEN : INK_RED;
    const std::string who = bid ? "bid" : "offer";
    {   // 1 what and how it ended
        std::string h = (bid ? "Bid " : "Ask ") + std::to_string(L.size0) + " at " + price(L.px, c.tick) + " - ";
        if (L.end == RESTING) h += "still resting";
        else if (L.end == FILLED) h += "FILLED (traded away)";
        else if (L.end == PULLED) h += "PULLED (cancelled)";
        else if (L.end == MOVED) h += "PULLED (moved to " + price(L.movedTo, c.tick) + ")";
        else if (L.end == OVER) h += "a bigger " + who + " took over";
        else if (L.end == LEFT) h += "price moved away (left the reach)";
        else if (L.end == LOST) h += "no longer in the book (feed missed its exit)";
        else if (L.end == RESTART) h += "IRT closed - after " + clock12(L.tEnd) + " not known";
        else h += "session ended";
        v.push_back(Row{h, ink});
    }
    {   // 2 when / how long
        const long long tEnd = L.end == RESTING ? std::max(c.nowMs, L.tStart) : L.tEnd;
        std::string t;
        if (L.startKnown) t = "Appeared " + clock12(L.tNew) + " - rested " + dur(tEnd - L.tNew);
        else t = std::string("Seen since ") + (L.sinceRestart ? "restart " : "the chart opened ") + clock12(L.tNew) + " - start not known, rested " + dur(tEnd - L.tNew) + "+";
        if (L.startKnown && L.tStart > L.tNew + 999) t += " (biggest from " + clock12(L.tStart) + ")";
        v.push_back(Row{t, INK_WHITE});
    }
    {   // 3 traded vs cancelled
        std::string t = "Traded " + std::to_string(L.traded) + " lots - cancelled " + std::to_string(L.cancelled) + " lots";
        if (L.end == RESTING || L.end == OVER || L.end == LEFT) t += " - " + std::to_string(L.left) + " still showing";
        if (L.end == OVER && L.overBy > 0) t += " (bigger: " + std::to_string(L.overBy) + ")";
        v.push_back(Row{t, INK_WHITE});
    }
    {   // 4 why it is drawn (the rule and the numbers)
        char b[160]; const double r = c.reachAtr * c.atrPrice;
        std::snprintf(b, sizeof(b), "The biggest single %s within %.1f ATR (%s) %s price; big now = %lld+ lots (self-tuned from the last 60 min, ~3 per side per hour, and >= 3x the book near price; floor %lld, tuned %lld)",
                      who.c_str(), c.reachAtr, r > 0 ? price((Tick)std::llround(r / c.tick), c.tick).c_str() : "?", bid ? "below" : "above", c.minLots, c.floor, c.pctLots);
        v.push_back(Row{b, INK_GREY});
    }
    v.push_back(Row{bid ? "Answers: where is the biggest buyer waiting under price - and did it get hit or pull out?"
                        : "Answers: where is the biggest seller waiting over price - and did it get lifted or pull out?", INK_WHITE});
    v.push_back(Row{bid ? "How: thicker line = size being added (+), thin = being pulled (-); once filled it stays in the BIG column for 60 min"
                        : "How: thicker line = size being added (+), thin = being pulled (-); once filled it stays in the BIG column for 60 min", INK_WHITE});
    v.push_back(Row{bid ? "Use: price into a big bid that FILLS and holds = selling absorbed; an X before price got there = support left"
                        : "Use: price into a big offer that FILLS and holds = buying absorbed; an X before price got there = resistance left", INK_WHITE});
    v.push_back(Row{bid ? "e.g. Bid 120 at 29,905 rested 14 min, filled at 7:11 AM and price turned up: a buyer absorbed the selling"
                        : "e.g. Ask 95 at 29,962 rested 9 min, filled at 9:42 AM and price turned down: a seller absorbed the buying", INK_WHITE});
    v.push_back(Row{"Displayed size only: hidden / iceberg size cannot be seen by anyone, and one order is not intent", INK_GREY});
    return v;
}

// ---- the layout: lines -> draw ops, inside [xMin, xMax] x [yTop, yBot], labels never overlapping -------------------------------
enum OpKind { OP_LINE = 0, OP_LABEL = 1, OP_DOT = 2, OP_X = 3, OP_ARROW = 4 };
struct Op { int kind = OP_LINE; int side = BID; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; std::string text; size_t line = 0; };
struct Seg { size_t line = 0; int side = BID; int x0 = 0, x1 = 0, y = 0; bool live = false; int end = RESTING; std::string text; int textW = 0; };
struct View { int xMin = 0, xMax = 0, yTop = 0, yBot = 0, rowH = 18; };
static const int ARROW_W = 9, MARK_R = 4, LABEL_GAP = 4;
// Returns the ops; the space a drawn line / mark / label takes never goes left of xMin (the Liquidity Profile's column) or right of xMax
// (the last bar + the arrow, before the right-side columns). Labels are placed in TIME order (lesson 49): an older line's label is
// decided first and a newer line never takes its spot - the newer one moves above / below its line or goes without; the same order's
// label at the same price again (it left the reach / was taken over and came back) is not repeated.
inline std::vector<Op> layout(std::vector<Seg> segs, const View& V) {
    std::stable_sort(segs.begin(), segs.end(), [](const Seg& a, const Seg& b) { return a.line < b.line; });
    std::vector<Op> ops; std::vector<std::pair<std::string, int> > texts;
    struct R { int l, t, r, b; };
    std::vector<R> placed;
    auto clash = [&](const R& a) { for (size_t i = 0; i < placed.size(); ++i) { const R& b = placed[i]; if (a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b) return true; } return false; };
    for (size_t i = 0; i < segs.size(); ++i) {
        const Seg& s = segs[i];
        if (s.y < V.yTop || s.y > V.yBot) continue;
        const int markRoom = s.live ? ARROW_W : (s.end == FILLED || s.end == PULLED || s.end == MOVED) ? MARK_R : 0;
        int x0 = std::max(s.x0, V.xMin), x1 = std::min(s.x1, V.xMax - markRoom);
        if (s.x1 < V.xMin || x0 > V.xMax - markRoom) continue;         // scrolled out of the drawable band
        if (x1 < x0) x1 = x0;
        Op ln; ln.kind = OP_LINE; ln.side = s.side; ln.x0 = x0; ln.x1 = x1; ln.y0 = ln.y1 = s.y; ln.line = s.line; ops.push_back(ln);
        Op mk; mk.side = s.side; mk.line = s.line; mk.y0 = s.y - MARK_R; mk.y1 = s.y + MARK_R;
        if (s.live) { mk.kind = OP_ARROW; mk.x0 = x1; mk.x1 = x1 + ARROW_W; ops.push_back(mk); }
        else if (s.end == FILLED && x1 + MARK_R <= V.xMax && x1 - MARK_R >= V.xMin) { mk.kind = OP_DOT; mk.x0 = x1 - MARK_R; mk.x1 = x1 + MARK_R; ops.push_back(mk); }
        else if ((s.end == PULLED || s.end == MOVED) && x1 + MARK_R <= V.xMax && x1 - MARK_R >= V.xMin) { mk.kind = OP_X; mk.x0 = x1 - MARK_R; mk.x1 = x1 + MARK_R; ops.push_back(mk); }
        bool again = false; for (size_t q = 0; q < texts.size(); ++q) if (texts[q].first == s.text && std::abs(texts[q].second - s.y) <= 1) again = true;
        if (again) continue;                                           // the same order's label is already on this price
        // the label at the left end: left of the start when there is room, else just above the line's start
        R a{ x0 - LABEL_GAP - s.textW, s.y - V.rowH / 2, x0 - LABEL_GAP, s.y - V.rowH / 2 + V.rowH };
        const bool leftOk = s.x0 >= V.xMin && a.l >= V.xMin && a.t >= V.yTop && a.b <= V.yBot;
        if (!leftOk || clash(a)) {
            R b{ x0, s.y - V.rowH - 1, x0 + s.textW, s.y - 1 };
            if (b.r > V.xMax || b.t < V.yTop || clash(b)) {
                R c2{ x0, s.y + 1, x0 + s.textW, s.y + 1 + V.rowH };
                if (c2.r > V.xMax || c2.b > V.yBot || clash(c2)) continue; // no free spot: the line without its label
                a = c2;
            } else a = b;
        }
        placed.push_back(a); texts.push_back(std::make_pair(s.text, s.y));
        Op lb; lb.kind = OP_LABEL; lb.side = s.side; lb.x0 = a.l; lb.y0 = a.t; lb.x1 = a.r; lb.y1 = a.b; lb.text = s.text; lb.line = s.line; ops.push_back(lb);
    }
    return ops;
}

// ---- the settings file: lsFlexLevels\LiquidityProfile.bigorders-<MKT>.txt  "BOL1,on,reach x10,min lots (0 = market default),keep,font"
struct Settings { bool on = true; int reach10 = 20; long long minLots = 0; int keep = KEEP_DEF; int font = FONT_DEF; bool ok = false; std::string why; };
inline bool parseSettings(const std::string& line, Settings& out) {
    Settings z; std::string ln = line; while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n' || ln.back() == ' ')) ln.pop_back();
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= ln.size(); ++i) if (i == ln.size() || ln[i] == ',') { f.push_back(ln.substr(a, i - a)); a = i + 1; }
    long long on = 0, r = 0, m = 0, k = 0, ft = 0;
    if (f.size() != 6 || f[0] != "BOL1" || !Engine::num(f[1], 0, 1, on) || !Engine::num(f[2], (double)(REACH_MIN * 10), (double)(REACH_MAX * 10), r) ||
        !Engine::num(f[3], 0, (double)FLOOR_MAX, m) || !Engine::num(f[4], KEEP_MIN, KEEP_MAX, k) || !Engine::num(f[5], FONT_MIN, FONT_MAX, ft)) {
        out.ok = false; out.why = "big-orders file garbled: '" + ln.substr(0, 40) + "' - last good kept"; return false;
    }
    z.on = on != 0; z.reach10 = (int)r; z.minLots = m; z.keep = (int)k; z.font = (int)ft; z.ok = true; out = z; return true;
}
inline std::string settingsLine(const Settings& s) {
    char b[96]; std::snprintf(b, sizeof(b), "BOL1,%d,%d,%lld,%d,%d", s.on ? 1 : 0, s.reach10, s.minLots, s.keep, s.font); return b;
}
// the dialog's values -> a clean Settings (garbage never reaches the file): out of range = the file's (or default) value
inline Settings cleanSettings(bool on, double reach, long long minLots, long long keep, long long font, const Settings& fileVal) {
    Settings s = fileVal; s.on = on;
    if (finiteD(reach) && reach >= REACH_MIN - 1e-9 && reach <= REACH_MAX + 1e-9) s.reach10 = (int)std::llround(reach * 10.0);
    if (minLots >= 0 && minLots <= FLOOR_MAX) s.minLots = minLots;
    if (keep >= KEEP_MIN && keep <= KEEP_MAX) s.keep = (int)keep;
    if (font >= FONT_MIN && font <= FONT_MAX) s.font = (int)font;
    s.ok = true; s.why.clear(); return s;
}

// ---- the 14-bar ATR of closed bars (true range, simple mean); 0 = fewer than 15 bars ---------------------------------------------
inline double atr14(const std::vector<double>& hi, const std::vector<double>& lo, const std::vector<double>& cl) {
    const size_t n = std::min(hi.size(), std::min(lo.size(), cl.size()));
    if (n < 15) return 0;
    double sum = 0; int m = 0;
    for (size_t i = n - 14; i < n; ++i) {
        const double h = hi[i], l = lo[i], pc = cl[i - 1];
        if (!finiteD(h) || !finiteD(l) || !finiteD(pc) || h < l) return 0;
        sum += std::max(h, pc) - std::min(l, pc); ++m;
    }
    return m == 14 ? sum / 14.0 : 0;
}
// the bar a Central-ms time falls in: bars stamp their END; bar i covers [end(i-1), end(i)). -1 = before the first bar
inline int barOf(const std::vector<long long>& endMs, long long t) {
    if (endMs.empty() || t <= 0) return -1;
    const std::vector<long long>::const_iterator it = std::upper_bound(endMs.begin(), endMs.end(), t);
    if (it == endMs.end()) return (int)endMs.size() - 1;     // the forming bar (its end is in the future)
    const int i = (int)(it - endMs.begin());
    if (i == 0 && endMs.size() > 1 && t < endMs[0] - (endMs[1] - endMs[0])) return -1;
    return i;
}

} // namespace bol
#endif
