#ifndef LS_PULLING_LOGIC_H
#define LS_PULLING_LOGIC_H
// PullingLogic.h (0.2.1, 2026-10-12) -- the SDK-free half of lsPulling: WHO PULLED their resting orders near price, per bar, from
// the market-by-order feed (Rithmic DBO through IRT's DEPTH_BY_ORDER). Rassul 2026-10-11 20:50-20:57: "a simple two line approach that
// is looking at pulling of orders by buyers and sellers ... it would show why price went up or down and who pulled orders", "call it
// pulling", "it should use mbo data"; design A (per bar, x normal) + the B cross (the 10-bar leader changed), mockup 20:50.
//
// The feed's meaning, checked on his own recordings (GC 19:00-19:57 2026-10-11: the FILL sizes add up to the traded volume minute by
// minute, 2,370 vs 2,462 lots over 31 minutes and exact in every fully drained minute; ES 17:00: 931 vs 931):
//   NEW  an order rests (side = its side: 1 buy = a bid, 0 sell = an offer)
//   FILL part or all of a resting order traded (size = lots filled); the order is then DELeted (all filled) or MODified (the rest)
//   DEL  the order left the book - a PULL unless its last event was a FILL that emptied it
//   MOD  the order changed: a new price (moved - a pull at the old price when it moved AWAY from the market) or a smaller size
//        (a partial cancel - a pull, unless it only follows a fill)
// Only orders NEAR price count: a bid within `near` ticks under the best bid, an offer within `near` ticks over the best offer.
// x normal = this bar's pulled lots / the average pulled per side per bar over the 20 bars before it (both sides together, so the two
// lines share one scale). The cross: the side that pulled more over the last 10 bars changed.
#include <cmath>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <climits>

namespace pl {

static const char* const VERSION = "0.3.0";   // 0.3.0 (2026-10-14 night): the header readout -> one zone read line (ZoneRead.h);      // keep equal to PL_VERSION / setVersion in Pulling.cpp
typedef long long Tick;
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Side { OFFER = 0, BID = 1 };                  // the SDK's buySell: 0 sell (an offer), 1 buy (a bid)

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }
inline bool toTick(double price, double tick, Tick& out) {
    if (!finite(price) || !finite(tick) || tick <= 0) return false;
    const double q = price / tick;
    // std::llround is only defined for a representable integral result.  Do not
    // turn a malformed native float into a wrapped order-book price.
    if (!finite(q) || q < (double)LLONG_MIN + 0.5 || q > (double)LLONG_MAX - 0.5) return false;
    out = (Tick)std::llround(q);
    return true;
}

struct Ev { int action = NEW; int side = BID; Tick px = 0, prev = 0; double size = 0; long long id = 0; Tick bid = 0, ask = 0; };
struct Bar { double pull[2] = {0, 0}, stack[2] = {0, 0}, filled = 0; long long events = 0; };
struct Params {
    int near = 10;          // ticks from the best price on each side that count
    double line = 1.5;      // the x-normal line (drawn, and a value is labelled at or over it)
    int normBars = 20, minNorm = 10, runBars = 10;
};
// per-market defaults until the nightly study sets them (LRA-PullNorms-<MKT>.csv)
inline void defaults(const std::string& m, Params& p) {
    p.near = m == "NQ" ? 20 : m == "ES" ? 8 : m == "EU" ? 6 : 10;
    p.line = 1.5;
}

class Engine {
public:
    Params P;
    struct Ord { int side = BID; Tick px = 0; double size = 0; bool filledLast = false; };
    std::unordered_map<long long, Ord> book;           // every order seen since load (the ones resting before load are unknown)
    Tick bestBid = 0, bestAsk = 0; bool haveBest = false;
    std::map<int, Bar> bars;                           // chart bar index -> what happened in it
    int cur = -1;                                      // the bar events go to (the forming bar)
    long long events = 0, unknown = 0, suppressed = 0, bookResets = 0; // unknown native events; post-reset unknowns are not pulls
    bool countUnseenDeletes = true;                    // true only at initial subscription, before a known book was discarded
    void setBest(Tick b, Tick a) { if (b > 0 && a > 0 && a > b) { bestBid = b; bestAsk = a; haveBest = true; } }
    void setBar(int bar) {
        if (bar < 0) return;
        cur = bar;
        bars[bar];                                     // zero-event bars are part of a per-bar normal, not absent observations
        while (bars.size() > 1500) bars.erase(bars.begin());
    }
    // The supplied SDK has no sequence number or snapshot/reset callback.  Once
    // a known book is lost, an unseen DEL/MOD could be a reset artifact, so only
    // subsequently observed NEW orders may contribute until the next instance.
    void invalidateBook() {
        book.clear(); haveBest = false; countUnseenDeletes = false; ++bookResets;
    }
    bool near(int side, Tick px) const {
        if (!haveBest) return false;
        return side == BID ? (px <= bestBid && px >= bestBid - P.near) : (px >= bestAsk && px <= bestAsk + P.near);
    }
    void onEvent(const Ev& e) {
        if (cur < 0 || !(e.size >= 0) || !finite(e.size)) return;
        events++;
        if (e.bid > 0 && e.ask > e.bid) setBest(e.bid, e.ask);
        Bar& b = bars[cur]; b.events++;
        const int s = e.side == OFFER ? OFFER : BID;
        std::unordered_map<long long, Ord>::iterator it = book.find(e.id);
        switch (e.action) {
        case NEW: {
            if (it != book.end()) {                    // duplicate/replayed ADD: refresh identity, do not double stack it
                it->second.side = s; it->second.px = e.px; it->second.size = e.size; it->second.filledLast = false;
                break;
            }
            Ord o; o.side = s; o.px = e.px; o.size = e.size; book[e.id] = o;
            if (near(s, e.px)) b.stack[s] += e.size;
            break; }
        case FILL: {
            b.filled += e.size;
            if (it != book.end()) { it->second.size = std::max(0.0, it->second.size - e.size); it->second.filledLast = true; } else unknown++;
            break; }
        case DEL: {
            if (it == book.end()) {
                unknown++;
                if (countUnseenDeletes && near(s, e.px)) b.pull[s] += e.size; // resting before the initial subscription
                else if (!countUnseenDeletes) suppressed++;
                break;
            }
            const Ord o = it->second; book.erase(it);
            if (o.filledLast && o.size <= 0) break;                        // all filled: the trade took it, nobody pulled
            if (near(o.side, o.px)) b.pull[o.side] += o.size;              // cancelled (or the rest after a partial fill)
            break; }
        case MOD: {
            if (it == book.end()) {
                unknown++;
                // An unseen MOD after a reset does not establish a reliable
                // resting order; a later DEL must not become a synthetic pull.
                if (countUnseenDeletes) { Ord o; o.side = s; o.px = e.px; o.size = e.size; book[e.id] = o; }
                else suppressed++;
                break;
            }
            Ord& o = it->second;
            if (e.px != o.px) {
                const bool away = o.side == BID ? e.px < o.px : e.px > o.px;
                if (away && near(o.side, o.px)) b.pull[o.side] += o.size;   // moved out of the way
                else if (!away && near(o.side, e.px)) b.stack[o.side] += e.size;
            } else {
                const double d = e.size - o.size;
                if (d < 0 && near(o.side, o.px)) b.pull[o.side] += -d;     // a partial cancel (after a fill the size already matches)
                else if (d > 0 && near(o.side, o.px)) b.stack[o.side] += d;
            }
            o.px = e.px; o.size = e.size; o.filledLast = false;
            break; }
        default: break;
        }
        if (book.size() > 400000) invalidateBook();                         // bounded memory without converting later unknown DELs into pulls
    }
    // the average pulled per side per bar over the normBars before `bar` (both sides), -1 until minNorm bars are known
    double normal(int bar) const {
        double s = 0; int n = 0;
        for (std::map<int, Bar>::const_iterator i = bars.lower_bound(bar - P.normBars); i != bars.end() && i->first < bar; ++i) { s += i->second.pull[0] + i->second.pull[1]; ++n; }
        if (n < P.minNorm || s <= 0) return -1;
        return s / (2.0 * n);
    }
    double x(int bar, int side) const {
        std::map<int, Bar>::const_iterator i = bars.find(bar); if (i == bars.end()) return -1;
        const double nrm = normal(bar); if (nrm <= 0) return -1;
        return i->second.pull[side] / nrm;
    }
    // the side that pulled more over the runBars ending at `bar`: 1 bids (buyers), 0 offers (sellers), -1 even / not known
    int leader(int bar) const {
        double t[2] = {0, 0}; int n = 0;
        for (std::map<int, Bar>::const_iterator i = bars.lower_bound(bar - P.runBars + 1); i != bars.end() && i->first <= bar; ++i) { t[0] += i->second.pull[0]; t[1] += i->second.pull[1]; ++n; }
        if (n < std::min(P.runBars, 5) || t[0] + t[1] <= 0) return -1;
        if (t[1] > t[0] * 1.2) return BID;
        if (t[0] > t[1] * 1.2) return OFFER;
        return -1;
    }
    // a cross at `bar`: the leader there differs from the last bar before it that had a leader
    bool cross(int bar) const {
        const int now = leader(bar); if (now < 0) return false;
        for (std::map<int, Bar>::const_iterator i = bars.lower_bound(bar); i != bars.begin();) {
            --i; const int l = leader(i->first); if (l >= 0) return l != now;
            if (bar - i->first > 40) break;
        }
        return false;
    }
};

}  // namespace pl
#endif
