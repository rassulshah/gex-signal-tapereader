#ifndef LS_SWEEPREFILL_LOGIC_H
#define LS_SWEEPREFILL_LOGIC_H
// SweepRefillLogic.h (1.0.0, 2026-10-22) -- SDK-free rules of lsSweepRefill, the "Sr" (SWEEP REFILLED) mark on the PRICE chart.
// MBO Roadmap row "Did the swept prices refill fast after a sweep?". Rassul approved the build 16:16 CT 2026-10-22, with his filter
// "Sr only happens when a level or pivot high or low was taken out" and "you can put it on top of the bar for a bullish reclaim";
// 16:18 CT "it should not be on adjacent candles" (mockup v2, sweep-refill-mockup-v2.png).
//
// WHAT IS DRAWN
//   * BULLISH Sr (green, ABOVE the candle): sellers swept DOWN through >= minPrices bid prices in one burst, the sweep TOOK OUT a
//     pivot low still in play or a key / MenthorQ level below, at a local (15-min) low, and the bids came back: >= refillPct of the
//     resting size that sat on the swept prices just before the sweep is resting there again refillMs (3 s) after the sweep.
//   * BEARISH mirror (red, BELOW the candle): buyers swept UP through offer prices through a level / pivot high and the offers came
//     back. Drawn by default (Rassul 16:28: "draw bearish Sr as well"); the "Show bearish Sr" checkbox turns it off.
//   * never on two adjacent candles (per side): a refilled sweep on a candle within gapBars of the last DRAWN one is recorded only.
//     The first Sr of a run is the one drawn; it never moves or disappears.
//   * decided ONCE, refillMs + flickerMs (4 s) after the sweep ended; never repainted. Every candidate (refilled or not) is recorded for the nightly.
//
// DETECTION
//   1. SWEEP: fills of RESTING orders on one side (resting bids hit = a sell sweep), each within windowMs (50) of the burst's first
//      fill and in the same or next exchange second, moving one way (a sell sweep never fills higher than its last fill).
//   2. SIZE: >= minPrices distinct prices AND >= minLots lots resting on the swept prices just before the sweep ("pre").
//      pre = for each swept price, at its FIRST fill in the burst, the resting lots there from orders that had rested >= flickerMs
//      (1 s) at the burst start (orders living < 1 s are flicker: 76% of the added size on NQ 10-09).
//   3. LOCAL EXTREME: the sweep's low <= the lowest trade of the last lowMin (15) minutes + lowAtr (0.25) x ATR (mirror for highs).
//   4. TAKEN OUT (Rassul 16:16 "Sr only happens when a level or pivot high or low was taken out"; 16:28 correction - the v1 red Sr at
//      NQ 9:55 / 10:03 are valid): the sweep's low is below (>= 1 tick) a reference that was still INTACT before this move, i.e.
//      taken out by the forming candle or the last (takeoutBars - 1) closed candles. The lookBars (5) closed candles before that
//      window = the pre-move candles. References, by priority (nearest to the sweep's low within a priority):
//        a key level (ONL / PDL / PFL / LonLO - joined names count) or any MenthorQ level that no pre-move candle traded below;
//        a confirmed 5-candle pivot low (2 candles each side) not taken out before the window;
//        the 5-candle low = the lowest low of the pre-move candles.
//      Mirror for highs (ONH / PDH / PFH / LonHI). (v2 needed the sweep itself to cross the reference and a confirmed pivot: it
//      dropped 9:55 - the 9:33 pivot high was taken out by the 9:51 candle just before the sweep - and 10:03 - the high it took
//      out was 1 candle old.)
//   5. REFILL: post = the lots resting on the swept prices at sweep end + refillMs (3 s) from orders that live >= flickerMs (1 s);
//      known for certain flickerMs later, so the decision is final at end + refillMs + flickerMs (4 s). refill = post / pre;
//      refilled = refill >= refillPct. Time to refill = when the surviving orders had arrived enough lots for
//      refillPct (0 = the pre-sweep orders that survived already make it).
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace swr {

static const char* const VERSION = "1.0.0";
enum Side { BID = 0, ASK = 1 };                    // the RESTING side: a sweep of BIDs = sellers sweeping down = a bullish Sr candidate
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum RefKind { REF_PIVOT = 0, REF_KEY = 1, REF_MQ = 2, REF_X5 = 3 };
static const int PX_CAP = 256, PEND_CAP = 64, PIV_CAP = 64, BARS_CAP = 600, SEC_CAP = 1800;
static const size_t BOOK_CAP = 400000;

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }

struct Params {
    int windowMs = 50, minPrices = 8, minLots = 30, refillMs = 3000, flickerMs = 1000, lowMin = 15, gapBars = 1, spb = 180, takeoutBars = 2, lookBars = 5;
    double refillPct = 70, lowAtr = 0.25;
};

// ---- inputs --------------------------------------------------------------------------------------------------------------------
struct Level { std::string name; long long px = 0; int kind = REF_KEY; int dir = 0; };   // dir -1 = a low (KEY), +1 = a high, 0 = either (MQ)
// a key-level file name -> -1 low / +1 high / 0 not one of his session levels ("PFH/PDH" -> +1; "VAL", "PWL", "DPoc" -> 0)
inline int keyDir(const std::string& n) {
    static const char* const H[4] = { "ONH", "PDH", "PFH", "LonHI" }, * const L[4] = { "ONL", "PDL", "PFL", "LonLO" };
    size_t a = 0; int d = 0;
    for (;;) {
        const size_t p = n.find('/', a); std::string t = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        while (!t.empty() && t[0] == ' ') t.erase(0, 1);
        while (!t.empty() && t.back() == ' ') t.pop_back();
        for (int i = 0; i < 4; ++i) { if (t == H[i]) d = d == 0 || d == 1 ? 1 : d; if (t == L[i]) d = d == 0 || d == -1 ? -1 : d; }
        if (p == std::string::npos) break;
        a = p + 1;
    }
    return d;
}
struct Bar { long long t = 0, h = 0, l = 0, c = 0; long long session = 0; };   // a CLOSED candle in ticks; t = its END (IRT bar time)
struct Ev { int action = NEW; int side = -1; long long px = 0, size = 0, id = 0, ms = 0, sec = 0; };

// ---- the hybrid event clock (IRT stamps whole seconds; arrival ms orders events inside a second, never across one) -------------
struct Clock {
    long long cur = -1, secArr = 0, last = 0;
    long long at(long long sec, long long arrMs) {
        if (sec > cur) { cur = sec; secArr = arrMs; }
        long long d = arrMs - secArr; if (d < 0) d = 0; if (d > 999) d = 999;
        const long long m = cur * 1000 + d; if (m > last) last = m; return last;
    }
    long long idle(long long arrMs) {                 // no event: time still passes on the arrival clock
        if (cur < 0) return last;
        long long d = arrMs - secArr; if (d < 0) d = 0; if (d > 600000) d = 600000;
        const long long m = cur * 1000 + d; if (m > last) last = m; return last;
    }
};

// ---- the record of one candidate ----------------------------------------------------------------------------------------------
struct Event {
    std::string key;                // "<start sec>-<b|s>-<low or high px ticks>"
    long long barT = 0, sec = 0, endMs = 0, startMs = 0;
    int side = BID;                 // BID = sell sweep through a low (bullish candidate), ASK = buy sweep through a high (bearish)
    std::string ref; int refKind = REF_PIVOT; long long refPx = 0;
    long long hiPx = 0, loPx = 0; int prices = 0; long long lots = 0, pre = 0, post = 0;
    double refill = 0;              // percent
    long long ttrMs = -1;           // time to refill (ms after the sweep end; -1 = never reached refillPct)
    bool refilled = false, gapOk = false;
    double atrTicks = 0;
    std::string ct;                 // 12-hour Central time of the sweep, for the hover / record (host)
};
inline bool bullish(const Event& e) { return e.side == BID; }

struct Ord { int side = 0; long long px = 0, size = 0, addMs = 0; };
struct Burst {
    bool on = false; int side = 0; long long startMs = 0, lastMs = 0, startSec = 0, lastPx = 0, firstPx = 0, lo = 0, hi = 0, lots = 0;
    long long loBefore = 0, hiBefore = 0;            // the forming candle's lowest / highest trade BEFORE the burst (pivot still in play)
    std::vector<long long> pxs; std::map<long long, long long> preAt;   // price -> resting lots (aged) at its first fill in the burst
};
struct Watch { long long id = 0, size = 0, addMs = 0; };
struct Pending { Event e; std::vector<long long> pxs; bool snapped = false; std::vector<Watch> w; };   // w = the orders resting on the swept prices at end + refillMs
struct Pivot { long long px = 0, barT = 0; int dir = 0; long long brokenAt = 0; };   // brokenAt = the END of the closed candle that took it out (0 = in play)

struct Detector {
    Params P;
    // book
    std::unordered_map<long long, Ord> book; std::map<long long, std::set<long long> > at[2];   // price -> order ids resting there
    long long bookResets = 0, unknownFills = 0;
    // flow
    Burst b; std::vector<Pending> pending; std::vector<Event> decided;   // the host drains `decided`
    long long nowMs = 0;
    // context
    std::vector<Level> levels; std::vector<Pivot> pivots; std::vector<Bar> win; std::vector<long long> barEnds;
    long long session = 0; double atrTicks = 0;
    long long formLo = 0, formHi = 0;                 // trades in the forming candle (reset at each close)
    std::map<long long, std::pair<long long, long long> > secRange;   // exchange second -> (low, high) trade, the last lowMin minutes
    long long lastDrawnBar[2] = { 0, 0 };
    // counters (status)
    long long fills = 0, bursts = 0, sweeps = 0, bigSweeps = 0, atExtreme = 0, tookOut = 0, candidates = 0, refilledN = 0, drawnN = 0;

    // ---- book -----------------------------------------------------------------------------------------------------------------
    void restOn(const Ord& o, long long id) { if (o.side == BID || o.side == ASK) at[o.side][o.px].insert(id); }
    void restOff(const Ord& o, long long id) {
        if (o.side != BID && o.side != ASK) return;
        std::map<long long, std::set<long long> >::iterator it = at[o.side].find(o.px);
        if (it == at[o.side].end()) return;
        it->second.erase(id);
        if (it->second.empty()) at[o.side].erase(it);
    }
    // resting lots at (side, px) from orders added at or before `bornBy` (ms)
    long long restingAged(int side, long long px, long long bornBy) const {
        std::map<long long, std::set<long long> >::const_iterator it = at[side].find(px); if (it == at[side].end()) return 0;
        long long s = 0;
        for (std::set<long long>::const_iterator k = it->second.begin(); k != it->second.end(); ++k) {
            std::unordered_map<long long, Ord>::const_iterator o = book.find(*k); if (o == book.end()) continue;
            if (o->second.addMs <= bornBy && o->second.size > 0) s += o->second.size;
        }
        return s;
    }
    void clearBook() { book.clear(); at[0].clear(); at[1].clear(); ++bookResets; }

    // ---- one order-book event (IRT DEPTH_BY_ORDER semantics; ms = the event clock) --------------------------------------------
    void onEvent(const Ev& e) {
        if (e.ms > nowMs) nowMs = e.ms;
        decide(nowMs);
        if (e.action < NEW || e.action > FILL || e.px <= 0 || e.size < 0 || e.size > 10000000) return;
        std::unordered_map<long long, Ord>::iterator it = book.find(e.id);
        switch (e.action) {
        case NEW: {
            if (e.side != BID && e.side != ASK) return;
            if (it != book.end()) { restOff(it->second, e.id); book.erase(it); }
            if (book.size() >= BOOK_CAP) clearBook();
            Ord o; o.side = e.side; o.px = e.px; o.size = e.size; o.addMs = nowMs; book[e.id] = o; restOn(o, e.id);
            break; }
        case MOD: {
            if (it == book.end()) {                     // first sight (rested before load): from now on, born now
                if (e.side != BID && e.side != ASK) return;
                if (book.size() >= BOOK_CAP) clearBook();
                Ord o; o.side = e.side; o.px = e.px; o.size = e.size; o.addMs = nowMs; book[e.id] = o; restOn(o, e.id); break;
            }
            Ord& o = it->second;
            if (e.px != o.px) { left(e.id); restOff(o, e.id); o.px = e.px; o.size = e.size; o.addMs = nowMs; restOn(o, e.id); }   // re-priced = a new arrival there
            else { if (e.size > o.size) o.addMs = nowMs; o.size = e.size; }   // a size increase = new lots (priority lost at CME)
            break; }
        case DEL: {
            left(e.id);
            if (it != book.end()) { restOff(it->second, e.id); book.erase(it); }
            break; }
        case FILL: {
            int side = e.side;
            if (it != book.end()) side = it->second.side; else ++unknownFills;
            if (side != BID && side != ASK) return;
            fill(side, e.px, e.size, e.sec);            // the sweep sees the book BEFORE this fill takes its lots
            if (it != book.end()) { Ord& o = it->second; o.size -= std::min(o.size, e.size); if (o.size <= 0) left(e.id); }
            break; }
        default: break;
        }
    }
    // ---- trades ---------------------------------------------------------------------------------------------------------------
    void trade(long long px, long long sec) {
        if (formLo == 0 || px < formLo) formLo = px;
        if (formHi == 0 || px > formHi) formHi = px;
        std::map<long long, std::pair<long long, long long> >::iterator r = secRange.find(sec);
        if (r == secRange.end()) { secRange[sec] = std::make_pair(px, px); while ((int)secRange.size() > SEC_CAP) secRange.erase(secRange.begin()); }
        else { if (px < r->second.first) r->second.first = px; if (px > r->second.second) r->second.second = px; }
    }
    void fill(int side, long long px, long long lots, long long sec) {
        ++fills;
        const bool join = b.on && side == b.side && nowMs - b.startMs <= P.windowMs && sec - b.startSec <= 1 && sec >= b.startSec
                          && (side == BID ? px <= b.lastPx : px >= b.lastPx);
        if (!join) {
            close();
            b = Burst(); b.on = true; b.side = side; b.startMs = nowMs; b.startSec = sec; b.firstPx = px; b.lo = b.hi = px;
            b.loBefore = formLo; b.hiBefore = formHi;
        }
        b.lastPx = px; b.lastMs = nowMs; b.lots += lots; if (px < b.lo) b.lo = px; if (px > b.hi) b.hi = px;
        if (!b.preAt.count(px) && (int)b.pxs.size() < PX_CAP) { b.pxs.push_back(px); b.preAt[px] = restingAged(side, px, b.startMs - P.flickerMs); }
        trade(px, sec);
    }
    // time passes (event clock): a burst older than the window is over; pending refills whose time has come are decided
    void advance(long long ms) { if (ms > nowMs) nowMs = ms; if (b.on && nowMs - b.startMs > P.windowMs) close(); decide(nowMs); }

    // ---- the reference the sweep took out (pivot first, then key, then MenthorQ; nearest to the sweep start) ------------------
    bool pickRef(const Burst& x, Level& out) const {
        const bool down = x.side == BID; const long long ext = down ? x.lo : x.hi;
        const int tk = P.takeoutBars < 1 ? 1 : P.takeoutBars, lk = P.lookBars < 1 ? 1 : P.lookBars;
        const int preEndIdx = (int)win.size() - tk;                  // the last pre-move candle (the window = the forming + tk - 1 closed)
        if (preEndIdx < 0 || preEndIdx + 1 < lk) return false;      // not enough closed candles yet
        long long pLo = 0, pHi = 0;
        for (int i = preEndIdx - lk + 1; i <= preEndIdx; ++i) { if (pLo == 0 || win[i].l < pLo) pLo = win[i].l; if (win[i].h > pHi) pHi = win[i].h; }
        const long long preEnd = win[preEndIdx].t;
        auto beyond = [&](long long L) { return down ? ext < L : ext > L; };
        bool got = false; Level best; int bestRank = 9;
        auto offer = [&](const Level& L, int rank) {
            if (!got || rank < bestRank || (rank == bestRank && std::llabs(L.px - ext) < std::llabs(best.px - ext))) { best = L; got = true; bestRank = rank; }
        };
        for (size_t i = 0; i < levels.size(); ++i) {
            const Level& L = levels[i];
            if (L.kind == REF_KEY && L.dir != (down ? -1 : 1)) continue;
            if (!beyond(L.px) || (down ? pLo < L.px : pHi > L.px)) continue;   // a pre-move candle had already traded beyond it
            offer(L, 0);
        }
        for (size_t i = 0; i < pivots.size(); ++i) {
            const Pivot& p = pivots[i]; if (p.dir != (down ? -1 : 1) || !beyond(p.px)) continue;
            if (p.brokenAt != 0 && p.brokenAt <= preEnd) continue;           // taken out before this move
            Level L; L.name = down ? "pivot low" : "pivot high"; L.px = p.px; L.kind = REF_PIVOT; L.dir = p.dir; offer(L, 1);
        }
        { Level L; L.name = down ? "5-candle low" : "5-candle high"; L.px = down ? pLo : pHi; L.kind = REF_X5; L.dir = down ? -1 : 1; if (beyond(L.px)) offer(L, 2); }
        if (got) out = best;
        return got;
    }
    bool atLocalExtreme(const Burst& x) const {
        const long long from = x.startSec - (long long)P.lowMin * 60;
        long long lo = x.lo, hi = x.hi;
        for (std::map<long long, std::pair<long long, long long> >::const_iterator it = secRange.lower_bound(from); it != secRange.end(); ++it) {
            if (it->second.first < lo) lo = it->second.first;
            if (it->second.second > hi) hi = it->second.second;
        }
        const double slack = P.lowAtr * (atrTicks > 0 ? atrTicks : 0);
        return x.side == BID ? (double)x.lo <= (double)lo + slack + 1e-9 : (double)x.hi >= (double)hi - slack - 1e-9;
    }
    long long barOf(long long sec) const {
        for (size_t i = barEnds.size(); i > 0; --i) if (barEnds[i - 1] <= sec) {    // the newest closed END <= sec: the next END holds sec
            if (i < barEnds.size()) return barEnds[i];
            const long long sp = P.spb > 0 ? P.spb : 180;
            return barEnds[i - 1] + ((sec - barEnds[i - 1]) / sp + 1) * sp;
        }
        if (!barEnds.empty()) return barEnds.front();
        const long long sp = P.spb > 0 ? P.spb : 180; return (sec / sp + 1) * sp;
    }
    void close() {
        if (!b.on) return;
        Burst x = b; b = Burst(); ++bursts;
        const int n = (int)x.pxs.size();
        if (n < 3) return;
        ++sweeps;
        long long pre = 0; for (std::map<long long, long long>::const_iterator it = x.preAt.begin(); it != x.preAt.end(); ++it) pre += it->second;
        if (n < P.minPrices || pre < P.minLots) return;
        ++bigSweeps;
        if (!atLocalExtreme(x)) return;
        ++atExtreme;
        Level r; if (!pickRef(x, r)) return;
        ++tookOut;
        Pending p; Event& e = p.e;
        e.sec = x.startSec; e.startMs = x.startMs; e.endMs = x.lastMs; e.side = x.side; e.ref = r.name; e.refKind = r.kind; e.refPx = r.px;
        e.hiPx = x.hi; e.loPx = x.lo; e.prices = n; e.lots = x.lots; e.pre = pre; e.atrTicks = atrTicks; e.barT = barOf(x.startSec);
        char k[64]; std::snprintf(k, sizeof k, "%lld-%s-%lld", x.startSec, x.side == BID ? "b" : "s", x.side == BID ? x.lo : x.hi); e.key = k;
        p.pxs = x.pxs;
        if ((int)pending.size() >= PEND_CAP) pending.erase(pending.begin());
        pending.push_back(p); ++candidates;
    }
    // (study rule, no look-ahead) the refill is MEASURED at sweep end + refillMs (3 s): the lots resting then on the swept prices from
    // orders that live >= flickerMs (1 s) - an order still resting flickerMs later certainly did; one that left counts if it had
    // lived flickerMs. So the decision is final at end + refillMs + flickerMs (4 s); nothing changes it after that.
    std::unordered_map<long long, long long> deaths;   // watched order id -> the ms it left the book (DEL / re-priced away / filled out)
    std::set<long long> watched;
    void left(long long id) { if (watched.count(id) && !deaths.count(id)) deaths[id] = nowMs; }
    void decide(long long ms) {
        for (size_t i = 0; i < pending.size();) {
            Pending& p = pending[i];
            const long long t3 = p.e.endMs + P.refillMs, t4 = t3 + P.flickerMs;
            if (!p.snapped && ms >= t3) {
                p.snapped = true;
                for (size_t k = 0; k < p.pxs.size(); ++k) {
                    std::map<long long, std::set<long long> >::const_iterator it = at[p.e.side].find(p.pxs[k]); if (it == at[p.e.side].end()) continue;
                    for (std::set<long long>::const_iterator q = it->second.begin(); q != it->second.end(); ++q) {
                        std::unordered_map<long long, Ord>::const_iterator o = book.find(*q);
                        if (o == book.end() || o->second.size <= 0 || p.w.size() >= 4096) continue;
                        Watch w; w.id = *q; w.size = o->second.size; w.addMs = o->second.addMs; p.w.push_back(w); watched.insert(*q);
                    }
                }
            }
            if (ms < t4) { ++i; continue; }
            Event e = p.e;
            std::vector<std::pair<long long, long long> > arr;   // the counted orders: (arrival ms, lots)
            long long post = 0;
            for (size_t k = 0; k < p.w.size(); ++k) {
                const Watch& w = p.w[k];
                std::unordered_map<long long, long long>::const_iterator d = deaths.find(w.id);
                const long long life = (d == deaths.end() ? t4 : d->second) - w.addMs;
                if (life < P.flickerMs) continue;
                post += w.size; arr.push_back(std::make_pair(w.addMs, w.size));
            }
            e.post = post; e.refill = e.pre > 0 ? 100.0 * (double)post / (double)e.pre : 0;
            e.refilled = e.pre > 0 && e.refill + 1e-9 >= P.refillPct;
            e.ttrMs = -1;
            if (e.refilled) {
                std::sort(arr.begin(), arr.end());
                const double need = P.refillPct / 100.0 * (double)e.pre; long long cum = 0;
                for (size_t k = 0; k < arr.size(); ++k) { cum += arr[k].second; if ((double)cum + 1e-9 >= need) { e.ttrMs = std::max(0LL, arr[k].first - e.endMs); break; } }
            }
            if (e.refilled) {
                ++refilledN;
                const long long sp = P.spb > 0 ? P.spb : 180, last = lastDrawnBar[e.side];
                e.gapOk = last == 0 || e.barT - last > (long long)P.gapBars * sp;   // gapBars 1 = never the very next candle (nor the same one)
                if (e.gapOk) { lastDrawnBar[e.side] = e.barT; ++drawnN; }
            }
            decided.push_back(e);
            pending.erase(pending.begin() + (long)i);
            if (pending.empty()) { watched.clear(); deaths.clear(); }
        }
        if (watched.size() > 200000) { watched.clear(); deaths.clear(); }
    }
    // ---- a CLOSED candle (strictly in time order; history first, then live): pivots, ATR, the forming candle's range resets -------
    void barClose(const Bar& c) {
        if (!barEnds.empty() && c.t <= barEnds.back()) return;
        if (c.session != session) { session = c.session; pivots.clear(); win.clear(); }
        barEnds.push_back(c.t); if ((int)barEnds.size() > BARS_CAP) barEnds.erase(barEnds.begin());
        const long long sp = P.spb > 0 ? P.spb : 180;
        for (size_t i = 0; i < pivots.size();) {           // taken out by this candle: remembered for the move window, then dropped
            Pivot& v = pivots[i];
            if (v.brokenAt == 0 && ((v.dir < 0 && c.l < v.px) || (v.dir > 0 && c.h > v.px))) v.brokenAt = c.t;
            if (v.brokenAt != 0 && c.t - v.brokenAt > (long long)(P.takeoutBars + 2) * sp) pivots.erase(pivots.begin() + (long)i); else ++i;
        }
        win.push_back(c); if (win.size() > 32) win.erase(win.begin());
        if (win.size() >= 5) {
            const size_t m = win.size() - 3; const Bar& a = win[m - 2]; const Bar& bb = win[m - 1]; const Bar& p = win[m]; const Bar& d = win[m + 1]; const Bar& f = win[m + 2];
            if (p.l <= bb.l && p.l <= a.l && p.l < d.l && p.l < f.l) { Pivot v; v.px = p.l; v.barT = p.t; v.dir = -1; pivots.push_back(v); }
            if (p.h >= bb.h && p.h >= a.h && p.h > d.h && p.h > f.h) { Pivot v; v.px = p.h; v.barT = p.t; v.dir = 1; pivots.push_back(v); }
            if ((int)pivots.size() > PIV_CAP) pivots.erase(pivots.begin());
        }
        double s = 0; int m = 0; for (size_t i = win.size() > 14 ? win.size() - 14 : 0; i < win.size(); ++i) { s += (double)(win[i].h - win[i].l); ++m; }
        atrTicks = m ? s / (double)m : 0;                  // ATR = the mean range of the last 14 closed candles
        formLo = formHi = 0;
    }
};

// ---- text -----------------------------------------------------------------------------------------------------------------------
inline std::string pxText(long long ticks, double tick) {
    char b[32]; const double p = (double)ticks * tick;
    int dec = 0; for (double t = tick; dec < 6 && std::fabs(t - std::floor(t + 1e-9)) > 1e-9; t *= 10) ++dec;
    std::snprintf(b, sizeof b, "%.*f", dec, p); return b;
}
inline std::string pctText(double v) { char b[16]; std::snprintf(b, sizeof b, "%.0f%%", !finite(v) || v < 0 ? 0.0 : v > 999 ? 999.0 : v); return b; }
inline std::string secText(long long ms) { if (ms < 0) return "not within the window"; char b[24]; std::snprintf(b, sizeof b, "%.1f s", (double)ms / 1000.0); return b; }
inline std::string refText(const Event& e, double tick) {
    if (e.refKind == REF_PIVOT) return e.ref + " " + pxText(e.refPx, tick);
    return e.ref + " " + pxText(e.refPx, tick);
}
inline std::string readLine(const Event& e, double tick, int windowS) {
    const std::string side = bullish(e) ? "bids" : "offers";
    std::string s = "Swept " + std::to_string(e.prices) + " prices (" + pxText(bullish(e) ? e.hiPx : e.loPx, tick) + " - " + pxText(bullish(e) ? e.loPx : e.hiPx, tick)
                    + ") through " + refText(e, tick) + " - refilled " + pctText(e.refill) + " in " + std::to_string(windowS) + " s";
    s += e.refilled ? " - " + side + " defending" : " - still empty";
    return s;
}
// the hover: header + the read + the numbers + question / what it is / why it matters / how to use / example (his hover structure)
inline std::vector<std::string> hoverRows(const Event& e, double tick, int windowS, double pct) {
    std::vector<std::string> r;
    char pb[16]; std::snprintf(pb, sizeof pb, "%.0f%%", pct);
    r.push_back(std::string("Sr  Sweep refilled?  ") + e.ct + (bullish(e) ? "  (bullish)" : "  (bearish)") + "   (? = not proven yet)");
    r.push_back(readLine(e, tick, windowS));
    r.push_back("Taken out: " + refText(e, tick) + (bullish(e) ? " (below)" : " (above)") + "  -  " + std::to_string(e.lots) + " lots traded, "
                + std::to_string(e.pre) + " resting before, " + std::to_string(e.post) + " back after " + std::to_string(windowS) + " s (" + pb + " needed in " + secText(e.ttrMs) + ")");
    r.push_back(std::string("Question: did the ") + (bullish(e) ? "bids" : "offers") + " that were just swept through the level come right back?");
    r.push_back(std::string("What it is: one ") + (bullish(e) ? "sell" : "buy") + " burst emptied many " + (bullish(e) ? "bid" : "offer") + " prices through a level / pivot; new orders (resting 1 s+) refilled them fast.");
    r.push_back("Why it matters: a fast refill = the sweep was absorbed and the defenders reload; still empty = an air pocket, price keeps going.");
    r.push_back(std::string("How to use: at the level, Sr on the turn candle backs a ") + (bullish(e) ? "long" : "short") + " with your reversal bar / CoB; no Sr = don't fade it.");
    r.push_back("Example: NQ 8:55 AM, 19 prices swept through G1 31000, 87% back in 3 s -> green Sr; price went 1 ATR up first.");
    return r;
}

// ---- the session record ----------------------------------------------------------------------------------------------------------
// SR|key|bar_t|sec|ct|side|ref|ref_kind|ref_px|hi|lo|prices|lots|pre|post|refill_x10|ttr_ms|refilled|gap_ok|atr_x100
static const char* const REC_HEAD = "# SR|key|bar_t|sec|ct|side|ref|ref_kind|ref_px|hi|lo|prices|lots|pre|post|refill_x10|ttr_ms|refilled|gap_ok|atr_x100";
inline std::string clean(const std::string& s, size_t n) { std::string o; for (char c : s) { if (o.size() >= n) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline std::string recLine(const Event& e, double tick) {
    char b[640];
    std::snprintf(b, sizeof b, "SR|%s|%lld|%lld|%s|%s|%s|%s|%s|%s|%s|%d|%lld|%lld|%lld|%lld|%lld|%d|%d|%lld", clean(e.key, 48).c_str(), e.barT, e.sec, clean(e.ct, 12).c_str(),
                  e.side == BID ? "bull" : "bear", clean(e.ref, 32).c_str(), e.refKind == REF_PIVOT ? "pivot" : e.refKind == REF_KEY ? "key" : e.refKind == REF_X5 ? "x5" : "mq",
                  pxText(e.refPx, tick).c_str(), pxText(e.hiPx, tick).c_str(), pxText(e.loPx, tick).c_str(), e.prices, e.lots, e.pre, e.post,
                  (long long)std::llround((finite(e.refill) ? std::min(e.refill, 99999.0) : 0) * 10), e.ttrMs, e.refilled ? 1 : 0, e.gapOk ? 1 : 0,
                  (long long)std::llround((finite(e.atrTicks) ? std::min(e.atrTicks, 1e7) : 0) * 100));
    return b;
}
inline std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; size_t a = 0; for (;;) { size_t p = s.find('|', a); o.push_back(s.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; } return o; }
inline bool whole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    char* e = nullptr; errno = 0; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (!e || *e || errno == ERANGE || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool real(const std::string& t, double& out) { if (t.empty() || t.size() > 32) return false; char* e = nullptr; const double v = std::strtod(t.c_str(), &e); if (!e || *e || !finite(v)) return false; out = v; return true; }
inline bool toTicks(double px, double tick, long long& out) { if (!(tick > 0) || !finite(px) || px <= 0) return false; const double t = px / tick; if (t > 9e15) return false; out = (long long)std::llround(t); return std::fabs(t - (double)out) < 1e-3; }
// the record -> the events (the FIRST line per key wins; nothing is ever changed)
inline std::vector<Event> parseRecord(const std::vector<std::string>& lines, double tick, long long* bad = nullptr) {
    std::vector<Event> v; std::set<std::string> seen; long long nb = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i]; if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = split(ln);
        if (c.size() != 20 || c[0] != "SR" || c[1].empty() || c[1].size() > 48) { ++nb; continue; }
        if (seen.count(c[1])) continue;
        Event e; e.key = c[1]; long long x = 0; double d = 0;
        if (!whole(c[2], 1, 4000000000000LL, e.barT) || !whole(c[3], 0, 4000000000000LL, e.sec)) { ++nb; continue; }
        e.ct = c[4];
        if (c[5] == "bull") e.side = BID; else if (c[5] == "bear") e.side = ASK; else { ++nb; continue; }
        e.ref = c[6];
        if (c[7] == "pivot") e.refKind = REF_PIVOT; else if (c[7] == "key") e.refKind = REF_KEY; else if (c[7] == "mq") e.refKind = REF_MQ; else if (c[7] == "x5") e.refKind = REF_X5; else { ++nb; continue; }
        if (!real(c[8], d) || !toTicks(d, tick, e.refPx) || !real(c[9], d) || !toTicks(d, tick, e.hiPx) || !real(c[10], d) || !toTicks(d, tick, e.loPx)) { ++nb; continue; }
        if (!whole(c[11], 3, 100000, x)) { ++nb; continue; } e.prices = (int)x;
        if (!whole(c[12], 0, 100000000, e.lots) || !whole(c[13], 0, 100000000, e.pre) || !whole(c[14], 0, 100000000, e.post)) { ++nb; continue; }
        if (!whole(c[15], 0, 1000000, x)) { ++nb; continue; } e.refill = (double)x / 10.0;
        if (!whole(c[16], -1, 100000000, e.ttrMs)) { ++nb; continue; }
        if ((c[17] != "0" && c[17] != "1") || (c[18] != "0" && c[18] != "1")) { ++nb; continue; }
        e.refilled = c[17] == "1"; e.gapOk = c[18] == "1";
        if (!whole(c[19], 0, 1000000000000LL, x)) { ++nb; continue; } e.atrTicks = (double)x / 100.0;
        seen.insert(e.key); v.push_back(e);
    }
    if (bad) *bad = nb;
    return v;
}
// drawn on the chart? (per side checkbox; the gap decision was made once, when the sweep was decided)
inline bool shown(const Event& e, bool showBull, bool showBear) { return e.refilled && e.gapOk && (bullish(e) ? showBull : showBear); }

} // namespace swr
#endif
