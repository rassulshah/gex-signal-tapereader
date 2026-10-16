#ifndef LS_BIGFLOW_H
#define LS_BIGFLOW_H
// BigFlow.h 1.1.0 (sweep: Mark.lo / hi) / 1.0.0 (2026-10-15 evening, Rassul 19:55 "build out the mkt column and 1,2,3") -- SDK-free; tested by test_bigflow.cpp.
// Two trackers fed by the same order-by-order events the BIG column already gets (lsLiquidityProfile):
//   Mkt - BIG MARKET ORDERS: every FILL row carries the AGGRESSOR's order id; all fills of one aggressor id = ONE market order
//         (its lots, the prices it took, its side = the FILL row's side). An order is complete 2 s after its last fill. Yardstick: the
//         session's own market orders - start = the 99th percentile of every completed market order (>= 3 lots), the "big" ones are
//         those at or above the start; Mkt mu / 1SD / 2SD / 3SD = the log mean / SD of the big ones (30 needed). Checked on his ES
//         10/14 recording: 199,809 market orders, p99 27 lots, 2,101 big -> mu 39 / 1SD 56 / 2SD 81 / 3SD 117.
//   Ice - ICEBERGS: CME keeps an iceberg's order id and reloads its shown size by a MOD after a fill (a MOD to a BIGGER size at the
//         same price right after a fill = a reload; a smaller one is only the size sync). An order is an iceberg once it has 2+
//         reloads and has traded at least 2x the most it ever showed. ES 10/14: 4,342 same-id reloads, 49 icebergs traded 56+ lots,
//         13 traded 117+ (the biggest: 259 lots traded showing 5).
// Times = ms on the host's Central clock. Nothing here draws; the host lays the marks out in the BIG column.
#include <algorithm>
#include <cmath>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace bfl {
static const char* const VERSION = "1.2.1";
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };              // FILL side = the aggressor: BID = a buyer lifted the offer, ASK = a seller hit the bid
static const long long DONE_MS = 2000;      // a market order is complete 2 s after its last fill
static const long long KEEP_MS = 3600000;   // marks: the last 60 min
static const int MIN_N = 30;

struct Mark { int side = BID; Tick px = 0; long long lots = 0; int n = 0; long long last = 0; Tick lo = 0, hi = 0; };   // lo / hi = the lowest / highest price taken (a sweep: hi > lo)

inline bool logStats(const std::vector<long long>& v, double& mu, double out[3]) {
    if ((int)v.size() < MIN_N) return false;
    double s1 = 0, s2 = 0; for (size_t i = 0; i < v.size(); ++i) { const double x = std::log((double)v[i]); s1 += x; s2 += x * x; }
    const double n = (double)v.size(), m = s1 / n, sd = std::sqrt(std::max(0.0, s2 / n - m * m));
    mu = std::exp(m); for (int k = 0; k < 3; ++k) out[k] = std::exp(m + (k + 1) * sd);
    return true;
}
// merge by side + price, the biggest `per` per side, newest-first ties
inline std::vector<Mark> topMarks(const std::vector<Mark>& in, int per) {
    std::vector<Mark> m;
    for (size_t i = 0; i < in.size(); ++i) {
        bool hit = false;
        for (size_t j = 0; j < m.size(); ++j) if (m[j].side == in[i].side && m[j].px == in[i].px) { m[j].lots += in[i].lots; m[j].n += in[i].n; m[j].last = std::max(m[j].last, in[i].last); m[j].lo = std::min(m[j].lo, in[i].lo); m[j].hi = std::max(m[j].hi, in[i].hi); hit = true; break; }
        if (!hit) m.push_back(in[i]);
    }
    std::vector<Mark> out;
    for (int s = 0; s < 2; ++s) {
        std::vector<Mark> v; for (size_t i = 0; i < m.size(); ++i) if (m[i].side == s) v.push_back(m[i]);
        std::sort(v.begin(), v.end(), [](const Mark& a, const Mark& b) { return a.lots != b.lots ? a.lots > b.lots : a.last > b.last; });
        for (size_t i = 0; i < v.size() && (int)i < per; ++i) out.push_back(v[i]);
    }
    return out;
}

// ---- big market orders ----------------------------------------------------------------------------------------------------------
class Mkt {
public:
    struct Ord { int side = BID; long long lots = 0; double pxLots = 0; Tick lo = 0, hi = 0; long long t0 = 0, tLast = 0; bool big = false; };
    std::vector<std::pair<long long, Ord> > fresh;    // (1.2.0) completed orders of LOG_MIN+ lots, for the plugin to write to the tracking file (raw, whatever the yardstick says)
    static const long long LOG_MIN = 10;
    std::unordered_map<long long, Ord> open;          // aggressor id -> its fills so far
    std::deque<std::pair<long long, Ord>> done;       // completed in the last 60 min (any size at or above the start when completed)
    std::vector<long long> big;                       // completed sizes >= the start (the yardstick sample, this session)
    std::vector<long long> cnt;                       // completed sizes 1..1023 (+ the rest in the last) for the start
    long long nAll = 0, fills = 0, noAggr = 0;
    Mkt() : cnt(1024, 0) {}
    void clear() { open.clear(); done.clear(); fresh.clear(); big.clear(); std::fill(cnt.begin(), cnt.end(), 0); nAll = fills = noAggr = 0; }
    long long start() const {                         // the 99th percentile of every completed market order, never under 3
        if (nAll <= 0) return 3;
        const long long want = std::max(1LL, (long long)std::ceil(0.01 * (double)nAll));
        long long acc = 0; int s = (int)cnt.size() - 1;
        for (; s > 1; --s) { acc += cnt[(size_t)s]; if (acc >= want) break; }
        return std::max(3, s);
    }
    void onFill(long long aggr, int side, Tick px, long long lots, long long t) {
        if (lots <= 0 || px <= 0 || (side != BID && side != ASK)) return;
        ++fills;
        if (aggr == 0) { ++noAggr; return; }
        Ord& o = open[aggr];
        if (o.lots == 0) { o.side = side; o.lo = o.hi = px; o.t0 = t; }
        o.lots += lots; o.pxLots += (double)px * (double)lots; o.lo = std::min(o.lo, px); o.hi = std::max(o.hi, px); o.tLast = std::max(o.tLast, t);
    }
    // complete the orders quiet for 2 s; drop marks older than 60 min
    void step(long long now) {
        const long long st = start();
        for (std::unordered_map<long long, Ord>::iterator it = open.begin(); it != open.end();) {
            if (now - it->second.tLast < DONE_MS) { ++it; continue; }
            Ord o = it->second; o.big = o.lots >= st;
            if (o.lots >= std::min((long long)LOG_MIN, st) && fresh.size() < 5000) fresh.push_back(std::make_pair(it->first, o));
            ++nAll; cnt[(size_t)std::min<long long>(o.lots, (long long)cnt.size() - 1)]++;
            if (o.lots >= st) { big.push_back(o.lots); done.push_back(std::make_pair(it->first, o)); }
            it = open.erase(it);
        }
        while (!done.empty() && done.front().second.tLast < now - KEEP_MS) done.pop_front();
        if (open.size() > 20000) open.clear();          // a stuck feed never grows without bound
    }
    bool stats(double& mu, double v[3]) const { return logStats(big, mu, v); }
    // the marks at or above `floor` lots (completed + still filling), at each order's volume-weighted price
    std::vector<Mark> marks(long long now, long long floor, int per = 5) const {
        std::vector<Mark> in;
        auto add = [&](const Ord& o) { if (o.lots < floor || o.tLast < now - KEEP_MS) return;
            Mark m; m.side = o.side; m.px = (Tick)std::llround(o.pxLots / (double)o.lots); m.lots = o.lots; m.n = 1; m.last = o.tLast; m.lo = o.lo; m.hi = o.hi; in.push_back(m); };
        for (size_t i = 0; i < done.size(); ++i) add(done[i].second);
        for (std::unordered_map<long long, Ord>::const_iterator it = open.begin(); it != open.end(); ++it) add(it->second);
        return topMarks(in, per);
    }
};

// ---- icebergs (one resting order id that reloads) -------------------------------------------------------------------------------
class Ice {
public:
    struct Ord { int side = BID; Tick px = 0; long long cur = 0, shown = 0, traded = 0; int reloads = 0; bool afterFill = false; long long tLast = 0; };
    std::unordered_map<long long, Ord> ord;          // resting orders that have traded (others are not kept)
    std::unordered_map<long long, std::pair<int, std::pair<Tick, long long> > > seen;   // every resting order: side, px, size (for the shown size)
    std::deque<std::pair<long long, Ord> > done;      // icebergs that ended (filled away / pulled) in the last 60 min
    long long reloadsSeen = 0;
    std::vector<std::pair<long long, Ord> > fresh;    // (1.2.0) icebergs that ended, for the tracking file
    void clear() { ord.clear(); seen.clear(); done.clear(); fresh.clear(); reloadsSeen = 0; }
    static bool is(const Ord& o) { return o.reloads >= 2 && o.shown > 0 && o.traded >= 2 * o.shown; }
    void onNew(long long id, int side, Tick px, long long size) {
        if (side != BID && side != ASK || px <= 0 || size < 0) return;
        seen[id] = std::make_pair(side, std::make_pair(px, size));
        if (seen.size() > 600000) seen.clear();
    }
    void onMod(long long id, int side, Tick px, long long size, long long t) {
        std::unordered_map<long long, Ord>::iterator it = ord.find(id);
        if (it == ord.end()) { onNew(id, side, px, size); return; }
        Ord& o = it->second;
        if (px != o.px) { end(id, t); onNew(id, side, px, size); return; }   // moved: a new order for this purpose
        if (o.afterFill && size > o.cur) { o.reloads++; ++reloadsSeen; o.afterFill = false; }   // a reload
        else if (size > o.cur) o.afterFill = false;                   // added size with no fill before it: not a reload
        // (a MOD to the same / a smaller size after a fill is the size sync: the reload may still follow)
        o.cur = size; o.shown = std::max(o.shown, size); o.tLast = t;
        seen[id] = std::make_pair(o.side, std::make_pair(px, size));
    }
    void onFill(long long id, Tick px, long long lots, long long t) {
        if (lots <= 0) return;
        std::unordered_map<long long, Ord>::iterator it = ord.find(id);
        if (it == ord.end()) {
            std::unordered_map<long long, std::pair<int, std::pair<Tick, long long> > >::const_iterator s = seen.find(id);
            if (s == seen.end()) return;                  // rested before we listened: its shown size is not known
            Ord o; o.side = s->second.first; o.px = s->second.second.first; o.cur = s->second.second.second; o.shown = o.cur;
            it = ord.insert(std::make_pair(id, o)).first;
            if (ord.size() > 200000) { ord.clear(); return; }
        }
        Ord& o = it->second;
        (void)px;
        o.traded += lots; o.cur = std::max(0LL, o.cur - lots); o.afterFill = true; o.tLast = t;
    }
    void end(long long id, long long t) {
        std::unordered_map<long long, Ord>::iterator it = ord.find(id);
        if (it != ord.end()) { if (is(it->second)) { Ord o = it->second; o.tLast = t; done.push_back(std::make_pair(id, o)); if (fresh.size() < 5000) fresh.push_back(std::make_pair(id, o)); } ord.erase(it); }
        seen.erase(id);
    }
    void step(long long now) { while (!done.empty() && done.front().second.tLast < now - KEEP_MS) done.pop_front(); }
    // live icebergs at or above `floor` traded lots, the biggest `per` per side
    std::vector<std::pair<long long, Ord> > live(long long floor, int per = 2) const {
        std::vector<std::pair<long long, Ord> > v[2];
        for (std::unordered_map<long long, Ord>::const_iterator it = ord.begin(); it != ord.end(); ++it)
            if (is(it->second) && it->second.traded >= floor) v[it->second.side].push_back(*it);
        std::vector<std::pair<long long, Ord> > out;
        for (int s = 0; s < 2; ++s) {
            std::sort(v[s].begin(), v[s].end(), [](const std::pair<long long, Ord>& a, const std::pair<long long, Ord>& b) { return a.second.traded > b.second.traded; });
            for (size_t i = 0; i < v[s].size() && (int)i < per; ++i) out.push_back(v[s][i]);
        }
        return out;
    }
    // ended icebergs as fill marks (side = the RESTING side: BID = a bid iceberg)
    std::vector<Mark> ended(long long now, long long floor, int per = 5) const {
        std::vector<Mark> in;
        for (size_t i = 0; i < done.size(); ++i) { const Ord& o = done[i].second; if (o.traded < floor || o.tLast < now - KEEP_MS) continue;
            Mark m; m.side = o.side; m.px = o.px; m.lots = o.traded; m.n = 1; m.last = o.tLast; m.lo = m.hi = o.px; in.push_back(m); }
        return topMarks(in, per);
    }
};
}   // namespace bfl
#endif
