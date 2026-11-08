// ZoneDefenseLogic.h  --  lsZoneDefense 1.0.0 (2026-11-08) - the SDK-free Zone Defense engine (one copy: the plugin, the zdrun CLI used
// by the jumpstart / nightly (lra.zone_defense) and the mock-SDK tests all compile THIS file, so the research and the chart agree).
//
// Rassul 2026-11-08 "Zone Defense is good. lets build it" (design: claude.ai/artifact/MJ9YYwjeMAUQztNZrCiP1G).
// Input: the market-by-order feed (order ids): ADD / MOD / DEL / FILL (+ CLEAR). A FILL of a resting BID = a sell aggressor hit it.
// What it decides (each decided ONCE, at decision time, never changed afterwards):
//   ZONE  (at a minute close)  the densest 4-tick band of resting orders aged >= 2 min, 2..40 ticks behind the last trade, >= densX x the
//                               median band and >= minLots; bids below price (support), offers above (resistance). One active zone
//                               per side at a time near a price (a new band overlapping the active one only refreshes it).
//   TOUCH (first trade into it, within 60 min)   + QUEUE: the share of the orders that had rested >= 30 s at the zone (or 1 tick
//                               behind it) cancelled in the 60 s before the touch vs its normal (the same filter in the 2..16 earlier
//                               60-s windows) - "pulling" = >= pullX x normal and >= normal + 10 pts (zone study fix).
//   RELOAD (touch + 60 s)       lots sold into the zone vs what was shown at the touch, the refills (an add at a price <= 250 ms after a
//                               fill there), what is left, and whether price broke >= 2 ticks through inside the 60 s:
//                               broke = "Bid exhausted" (bearish), reload >= reloadX and held = "Buyers holding" (bullish), else "Quiet".
//   SWEEP (first stop run per touch: the first burst of attacker fills beyond the zone within 15 min, decided 5 s after its end)
//                               lift = opposite lots in the 5 s after / run lots; swept = lift >= liftX AND the last trade back in or
//                               above the zone (green up-triangle), else weak (red). One per touch, never changes.
//   END  the zone ends: broken (a trade >= 2 ticks through), faded (resting < 50% of its peak), or 60 min with no touch.
// Truth (for the nightly only, never drawn): held9 / held30 after the RELOAD decision and after the SWEEP decision.
#pragma once
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "ZoneDefenseParams.h"

namespace zd {
static const char* const VERSION = "1.0.0";
enum Act { ADD = 0, MOD = 1, DEL = 2, FILL = 3, CLEAR = 9 };
enum Side { BID = 0, ASK = 1 };
static const long long MIN_MS = 60000;
// the LOCKED design values (approved design 2026-11-08; lra.consistency_check PARITY rows zd_* read these lines and lra.zone_defense)
static const int MIN_DIST = 2, MAX_DIST = 40, BAND = 4, MIN_AGE_MIN = 2;   // zone: ticks behind the last trade, band width, order age
static const double DENS_X = 1.5;                                          // the band vs the median band
static const int BROKE_TICKS = 2;                                          // broken = a trade 2+ ticks through
static const int RELOAD_SEC = 60;                                          // the reload window after the touch
static const double LIFT_X = 1.0;                                          // swept = lift >= 1.0x (and back in the zone)
static const int SWEEP_LIFT_S = 5;                                         // the lift window after the run
static const int QUEUE_REST_S = 30;                                        // queue: orders rested 30 s+ at the window start
static const double PULL_X = 2.0;                                          // pulling = cancels >= 2x normal and normal + 10 pts

struct In { long long t = 0; int act = ADD, side = -1; long long px = 0, sz = 0, oid = 0; };   // t = ms since the epoch (CT wall clock used for periods)

inline int periodOf(int ctMin, const zdp::Mkt& m) {            // minute of the CT day -> period index (zdp::P_*)
    const int o = m.openMin, c = m.closeMin, le = std::min(750, c - 30);
    if (ctMin >= 1020 || ctMin < 60) return zdp::P_ASIA;
    if (ctMin < o - 60) return zdp::P_LONDON;
    if (ctMin < o) return zdp::P_PREOPEN;
    if (ctMin < o + 30) return zdp::P_OPEN;
    if (ctMin < std::min(660, c - 30)) return zdp::P_MORNING;
    if (ctMin < std::min(le, c - 30)) return zdp::P_LUNCH;
    if (ctMin < c - 30) return zdp::P_AFTERNOON;
    if (ctMin < c) return zdp::P_CLOSE;
    return zdp::P_POST;
}

struct Zone {
    int id = 0, side = BID, per = 0; long long T = 0, lo = 0, hi = 0, size = 0, peak = 0; double dens = 0, oldestMin = 0;
    // touch
    long long touchT = -1; long long atTouch = 0; double cancel60 = -1, cnorm = -1; int n60 = 0; bool pulling = false;
    // reload window
    long long sold = 0, refills = 0, left = 0, zeroT = -1, decT = -1; bool brokeEarly = false; long long brokeT = -1; int reload = -1;   // reload verdict: 0 exhausted, 1 quiet, 2 holding
    double reloadX = 0;
    // sweep
    bool runOpen = false; int sweep = -1; long long runT = -1, runEnd = -1, runLots = 0, runExt = 0, liftLots = 0, rebid = 0, sweepDecT = -1; double lift = 0; bool backIn = false;
    // end
    int endCode = 0; long long endT = -1;                        // 1 broken, 2 faded, 3 expired (60 min, no touch), 4 session end
    // truth (nightly)
    int held9 = -1, held30 = -1, sHeld9 = -1;
    // the bars (per candle key): sold into the zone, resting left at the candle's end
    std::map<long long, long long> barSold, barLeft;
    // single big order (at most one line per zone): the largest resting order in the zone when it formed
    long long bigOid = 0, bigSz = 0, bigPx = 0;
    bool active() const { return endCode == 0; }
};

// one decision record line (the journal the overlay draws from and a restart reloads)
struct Line { long long t = 0; std::string text; };

struct Ended { long long tAdd, tEnd; bool cancel; };
struct Ord { int side; long long px, sz, tAdd; };

class Engine {
public:
    zdp::Mkt M; int ctOffsetMin = 0;                            // CT minutes = UTC minutes + ctOffsetMin (set by the host from the clock)
    std::vector<Zone> zones; std::vector<Line> out;             // all zones of the session, decision lines in order
    long long lastPx = 0, lastT = 0, nextMin = 0, events = 0, fills = 0, minutes = 0;
    int nextId = 1;
    long long barSec = 180;
    long long minLotsOverride = 0;                              // research runs only (zdrun): one floor for every period                                     // the chart's candle (keys the pane bars)
    explicit Engine(const zdp::Mkt& m) : M(m) {}

    void feed(const In& e) {
        if (e.t < lastT) { /* out of order: keep time monotone */ } else lastT = e.t;
        const long long t = lastT; ++events;
        if (nextMin == 0) nextMin = (t / MIN_MS + 1) * MIN_MS;
        while (t >= nextMin) { minuteClose(nextMin); nextMin += MIN_MS; }
        timers(t);
        switch (e.act) {
        case ADD: onAdd(e, t); break;
        case MOD: onMod(e, t); break;
        case DEL: onEnd(e.oid, t, true); break;
        case FILL: onFill(e, t); break;
        case CLEAR: book.clear(); break;
        default: break;
        }
    }
    void finish(long long t) {                                  // session over: close what is open (truth stays -1 where not known)
        timers(t);
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].active()) endZone(zones[i], 4, t);
    }
    int ctMinute(long long t) const { long long m = (t / MIN_MS + ctOffsetMin) % 1440; if (m < 0) m += 1440; return (int)m; }
    long long candleKey(long long t) const { const long long b = barSec * 1000; return (t / b + 1) * b / 1000; }   // bar END (s), as the chart

    // the most recent zone with a touch (what the pane's 4 lines read), -1 = none
    int readZone() const { for (size_t i = zones.size(); i-- > 0;) if (zones[i].touchT >= 0) return (int)i; return -1; }

private:
    std::unordered_map<long long, Ord> book;
    std::vector<int> live;                                            // zones active or ended < 31 min ago (truth windows)
    std::map<std::pair<int, long long>, std::deque<Ended> > ended;    // (side, px) -> orders that left there (last 17 min)
    std::map<std::pair<int, long long>, long long> lastFillAt;        // (side, px) -> last fill ms (refills)
    // trades after each touch / sweep for the truth windows: kept compact as (t, px)
    std::deque<std::pair<long long, long long> > tape;                // last 31 min of trades (t, px)
    struct Burst { int side = -1; long long t0 = -1, tl = -1, lots = 0, ext = 0; };
    long long tradeAggrSide = -1;

    const zdp::Per& P(int per) const { return M.per[per]; }

    void onAdd(const In& e, long long t) {
        if (e.side < 0 || e.px <= 0) return;
        Ord o; o.side = e.side; o.px = e.px; o.sz = e.sz; o.tAdd = t; book[e.oid] = o;
        // refills: an add at a price that just traded on the same side
        std::map<std::pair<int, long long>, long long>::const_iterator f = lastFillAt.find(std::make_pair(e.side, e.px));
        const bool refill = f != lastFillAt.end() && t - f->second <= 250;
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]]; if (!z.active()) { continue; }
            if (z.side != e.side) continue;
            if (z.touchT >= 0 && z.decT < 0 && e.px >= z.lo && e.px <= z.hi && refill) ++z.refills;
            if (z.runEnd >= 0 && z.sweep < 0 && t <= z.runEnd + SWEEP_LIFT_S * 1000 && (z.side == BID ? e.px >= z.runExt && e.px <= z.hi : e.px <= z.runExt && e.px >= z.lo)) z.rebid += e.sz;
        }
    }
    void onMod(const In& e, long long t) {
        std::unordered_map<long long, Ord>::iterator it = book.find(e.oid);
        if (it == book.end()) { In a = e; a.act = ADD; onAdd(a, t); return; }
        if (e.px > 0 && e.px != it->second.px) { it->second.px = e.px; it->second.tAdd = t; }   // a new price = a new queue place
        if (e.sz > 0) it->second.sz = e.sz;
    }
    void onEnd(long long oid, long long t, bool cancel) {
        std::unordered_map<long long, Ord>::iterator it = book.find(oid); if (it == book.end()) return;
        const Ord& o = it->second;
        std::deque<Ended>& d = ended[std::make_pair(o.side, o.px)];
        Ended x; x.tAdd = o.tAdd; x.tEnd = t; x.cancel = cancel; d.push_back(x);
        while (!d.empty() && t - d.front().tEnd > 17 * MIN_MS) d.pop_front();
        book.erase(it);
    }
    void onFill(const In& e, long long t) {
        std::unordered_map<long long, Ord>::iterator it = book.find(e.oid);
        int side = e.side; long long px = e.px;
        if (it != book.end()) { side = it->second.side; if (px <= 0) px = it->second.px; it->second.sz -= e.sz; if (it->second.sz <= 0) onEnd(e.oid, t, false); }
        if (side < 0 || px <= 0) return;
        ++fills; lastFillAt[std::make_pair(side, px)] = t;
        trade(t, px, e.sz, side == BID ? 1 : 0);                  // a resting bid filled = a SELL aggressor (1)
    }
    // the tape: one call per fill. sellAggr 1 = hit the bid
    void trade(long long t, long long px, long long lots, int sellAggr) {
        lastPx = px; tape.push_back(std::make_pair(t, px)); while (!tape.empty() && t - tape.front().first > 31 * MIN_MS) tape.pop_front();
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]];
            if (!z.active()) { truth(z, t, px); continue; }
            const bool bid = z.side == BID; const bool attacker = bid ? sellAggr == 1 : sellAggr == 0;
            if (z.touchT < 0) {
                if (bid ? px <= z.hi : px >= z.lo) touch(z, t);
                else continue;
            }
            truth(z, t, px);
            // the reload window
            if (z.decT < 0) {
                if (attacker && px >= z.lo && px <= z.hi) { z.sold += lots; z.barSold[candleKey(t)] += lots; }
                if (!z.brokeEarly && (bid ? px <= z.lo - BROKE_TICKS : px >= z.hi + BROKE_TICKS)) { z.brokeEarly = true; z.brokeT = t; }
            } else if (attacker && px >= z.lo && px <= z.hi) z.barSold[candleKey(t)] += lots;
            // the sweep: the first attacker burst beyond the zone (within 15 min of the touch)
            if (z.sweep < 0) {
                const bool beyond = bid ? px < z.lo : px > z.hi;
                if (z.runT < 0) {
                    if (attacker && beyond && t - z.touchT <= 15 * MIN_MS) { z.runT = t; z.runEnd = t; z.runLots = lots; z.runExt = px; z.runOpen = true; }
                } else if (z.runOpen && attacker && t - z.runEnd <= 200 && t - z.runT <= 2000) {
                    z.runEnd = t; z.runLots += lots; z.runExt = bid ? std::min(z.runExt, px) : std::max(z.runExt, px);
                } else {
                    z.runOpen = false;
                    if (!attacker && t <= z.runEnd + SWEEP_LIFT_S * 1000) z.liftLots += lots;
                }
            }
            // end: broken (after the reload and sweep decisions; before them the decisions read it)
            if (z.active() && z.decT >= 0 && z.sweep >= 0 && (bid ? px <= z.lo - BROKE_TICKS : px >= z.hi + BROKE_TICKS)) endZone(z, 1, t);
        }
    }
    void touch(Zone& z, long long t) {
        z.touchT = t; z.atTouch = restingIn(z.side, z.lo, z.hi); z.left = z.atTouch;
        queueRead(z, t);
        char b[200]; std::snprintf(b, sizeof(b), "ZT,%d,%lld,%lld,%.3f,%.3f,%d,%d", z.id, t, z.atTouch, z.cancel60, z.cnorm, z.n60, z.pulling ? 1 : 0);
        Line l; l.t = t; l.text = b; out.push_back(l);
    }
    // queue at the touch: orders at the zone (or 1 tick behind) that had rested >= 30 s at the window start; cancelled share
    void queueRead(Zone& z, long long tt) {
        const long long qlo = z.side == BID ? z.lo - 1 : z.lo, qhi = z.side == BID ? z.hi : z.hi + 1;
        struct OL { long long tAdd, tEnd; bool c; };
        std::vector<OL> v;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it)
            if (it->second.side == z.side && it->second.px >= qlo && it->second.px <= qhi) { OL o = { it->second.tAdd, LLONG_MAX, false }; v.push_back(o); }
        for (long long p = qlo; p <= qhi; ++p) {
            std::map<std::pair<int, long long>, std::deque<Ended> >::const_iterator d = ended.find(std::make_pair(z.side, p)); if (d == ended.end()) continue;
            for (size_t k = 0; k < d->second.size(); ++k) { OL o = { d->second[k].tAdd, d->second[k].tEnd, d->second[k].cancel }; v.push_back(o); }
        }
        int n = 0, c = 0;
        for (size_t k = 0; k < v.size(); ++k) if (v[k].tAdd <= tt - 60000 - QUEUE_REST_S * 1000 && v[k].tEnd > tt - 60000) { ++n; if (v[k].tEnd <= tt && v[k].c) ++c; }
        z.n60 = n; z.cancel60 = n ? (double)c / n : -1;
        std::vector<double> norm;
        for (int w = 2; w <= 16; ++w) {
            const long long w0 = tt - w * MIN_MS; int nn = 0, cc = 0;
            for (size_t k = 0; k < v.size(); ++k) if (v[k].tAdd <= w0 - QUEUE_REST_S * 1000 && v[k].tEnd > w0) { ++nn; if (v[k].tEnd <= w0 + MIN_MS && v[k].c) ++cc; }
            if (nn >= 3) norm.push_back((double)cc / nn);
        }
        if (!norm.empty()) { std::sort(norm.begin(), norm.end()); const size_t m = norm.size(); z.cnorm = m % 2 ? norm[m / 2] : (norm[m / 2 - 1] + norm[m / 2]) / 2; }
        else z.cnorm = P(z.per).cancelNorm;
        z.pulling = z.n60 >= 3 && z.cancel60 >= 0 && z.cancel60 >= std::max(PULL_X * z.cnorm, z.cnorm + 0.10);
    }
    long long restingIn(int side, long long lo, long long hi) const {
        long long s = 0;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) if (it->second.side == side && it->second.px >= lo && it->second.px <= hi) s += it->second.sz;
        return s;
    }
    // time-driven decisions (run before each event and at the session end)
    void timers(long long t) {
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]];
            if (!z.active()) { if (z.endT >= 0) truthTime(z, t); continue; }
            if (z.touchT < 0) { if (t - z.T > 60 * MIN_MS) endZone(z, 3, z.T + 60 * MIN_MS); continue; }
            truthTime(z, t);
            if (z.decT < 0 && t >= z.touchT + RELOAD_SEC * 1000) decideReload(z, z.touchT + RELOAD_SEC * 1000);
            if (z.sweep < 0 && z.runT >= 0 && t > z.runEnd + SWEEP_LIFT_S * 1000) decideSweep(z, z.runEnd + SWEEP_LIFT_S * 1000);
            if (z.sweep < 0 && z.runT < 0 && t - z.touchT > 15 * MIN_MS) sweepNone(z, z.touchT + 15 * MIN_MS);   // no run within 15 min: "Sweep: none"
            if (z.decT >= 0 && z.sweep >= 0 && z.active() && lastBeyond(z)) endZone(z, 1, t);
        }
    }
    bool lastBeyond(const Zone& z) const { return z.side == BID ? lastPx <= z.lo - BROKE_TICKS : lastPx >= z.hi + BROKE_TICKS; }
    void decideReload(Zone& z, long long dt) {
        z.decT = dt; z.left = restingIn(z.side, z.lo, z.hi);
        z.reloadX = z.atTouch > 0 ? (double)z.sold / z.atTouch : (z.sold > 0 ? 99.0 : 0.0);
        z.reload = z.brokeEarly ? 0 : (z.reloadX >= P(z.per).reloadX ? 2 : 1);
        char b[240]; std::snprintf(b, sizeof(b), "ZR,%d,%lld,%d,%lld,%lld,%lld,%lld,%.3f,%lld,%lld", z.id, dt, z.reload, z.sold, z.refills, z.atTouch, z.left, z.reloadX, z.zeroT, z.brokeT);
        Line l; l.t = dt; l.text = b; out.push_back(l);
    }
    void decideSweep(Zone& z, long long dt) {
        z.sweepDecT = dt; z.lift = z.runLots > 0 ? (double)z.liftLots / z.runLots : 0;
        z.backIn = z.side == BID ? lastPx >= z.lo : lastPx <= z.hi;
        z.sweep = (z.lift >= LIFT_X && z.backIn) ? 2 : 1;
        char b[240]; std::snprintf(b, sizeof(b), "ZS,%d,%lld,%d,%lld,%lld,%lld,%.3f,%d,%lld,%lld", z.id, dt, z.sweep, z.runT, z.runLots, z.runExt, z.lift, z.backIn ? 1 : 0, z.rebid, z.liftLots);
        Line l; l.t = dt; l.text = b; out.push_back(l);
    }
    void sweepNone(Zone& z, long long dt) {
        z.sweep = 0; z.sweepDecT = -1;
        char b[120]; std::snprintf(b, sizeof(b), "ZS,%d,%lld,0,-1,0,0,0.000,0,0,0", z.id, dt);
        Line l; l.t = dt; l.text = b; out.push_back(l);
    }
    void endZone(Zone& z, int code, long long t) {
        if (!z.active()) return;
        if (z.touchT >= 0 && z.decT < 0) decideReload(z, std::min(t, z.touchT + RELOAD_SEC * 1000));
        if (z.runT >= 0 && z.sweep < 0) decideSweep(z, z.runEnd + SWEEP_LIFT_S * 1000);
        if (z.touchT >= 0 && z.sweep < 0) sweepNone(z, t);
        z.endCode = code; z.endT = t;
        char b[120]; std::snprintf(b, sizeof(b), "ZE,%d,%lld,%d,%lld", z.id, t, code, restingIn(z.side, z.lo, z.hi));
        Line l; l.t = t; l.text = b; out.push_back(l);
    }
    // truth after the reload decision (held9 / held30) and after the sweep decision (sHeld9); trade-driven breaks
    void truth(Zone& z, long long t, long long px) {
        const bool bid = z.side == BID;
        if (z.decT >= 0 && !z.brokeEarly) {
            const bool br = bid ? px <= z.lo - BROKE_TICKS : px >= z.hi + BROKE_TICKS;
            if (br && t > z.decT) { if (z.held9 < 0 && t <= z.decT + 9 * MIN_MS) z.held9 = 0; if (z.held30 < 0 && t <= z.decT + 30 * MIN_MS) z.held30 = 0; }
        }
        if (z.sweepDecT >= 0 && z.sHeld9 < 0 && t > z.sweepDecT && t <= z.sweepDecT + 9 * MIN_MS && (bid ? px <= z.runExt - BROKE_TICKS : px >= z.runExt + BROKE_TICKS)) z.sHeld9 = 0;
    }
    void truthTime(Zone& z, long long t) {
        if (z.decT >= 0 && !z.brokeEarly) { if (z.held9 < 0 && t > z.decT + 9 * MIN_MS) z.held9 = 1; if (z.held30 < 0 && t > z.decT + 30 * MIN_MS) z.held30 = 1; }
        if (z.sweepDecT >= 0 && z.sHeld9 < 0 && t > z.sweepDecT + 9 * MIN_MS) z.sHeld9 = 1;
    }
    // the minute close: fade check, the pane's "bid left" per candle, new zones
    void minuteClose(long long T) {
        ++minutes;
        { std::vector<int> k; for (size_t q = 0; q < live.size(); ++q) { const Zone& z = zones[live[q]]; if (z.active() || T - z.endT <= 31 * MIN_MS) k.push_back(live[q]); } live.swap(k); }
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]]; if (!z.active()) { continue; }
            const long long r = restingIn(z.side, z.lo, z.hi);
            if (r > z.peak) z.peak = r;
            if (z.touchT >= 0) { z.barLeft[candleKey(T - 1)] = r; if (z.zeroT < 0 && r == 0) z.zeroT = T; }
            if (z.peak > 0 && r * 2 < z.peak && (z.touchT < 0 || (z.decT >= 0 && z.sweep >= 0))) endZone(z, 2, T);   // touched: only after its reload + sweep decisions
        }
        if (barSec > 0 && (T / 1000) % barSec == 0) barLines(T);
        if (lastPx <= 0) return;
        const int per = periodOf(ctMinute(T), M);
        for (int side = 0; side < 2; ++side) detect(side, T, per);
    }
    // the candle closed at T: one ZB line per touched zone that was live in it (sold into the zone in that candle, resting left)
    void barLines(long long T) {
        const long long key = candleKey(T - 1);
        for (size_t li = 0; li < live.size(); ++li) {
            Zone& z = zones[live[li]]; if (z.touchT < 0 || z.touchT >= T) continue;
            if (!z.active() && z.endT < T - barSec * 1000) continue;
            std::map<long long, long long>::const_iterator s = z.barSold.find(key);
            const long long sold = s == z.barSold.end() ? 0 : s->second, left = z.active() ? restingIn(z.side, z.lo, z.hi) : 0;
            char b[120]; std::snprintf(b, sizeof(b), "ZB,%d,%lld,%lld,%lld", z.id, key, sold, left);
            Line l; l.t = T; l.text = b; out.push_back(l);
        }
    }
    void detect(int side, long long T, int per) {
        const zdp::Per& p = P(per);
        long long tot[16] = { 0 }; const int NB = (MAX_DIST - MIN_DIST + 1 + BAND - 1) / BAND;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) {
            const Ord& o = it->second; if (o.side != side || o.tAdd > T - MIN_AGE_MIN * MIN_MS) continue;
            const long long d = side == BID ? lastPx - o.px : o.px - lastPx;
            if (d < MIN_DIST || d > MAX_DIST) continue;
            const int b = (int)((d - MIN_DIST) / BAND); if (b >= 0 && b < NB && b < 16) tot[b] += o.sz;
        }
        int bi = 0; std::vector<long long> nz;
        for (int b = 0; b < NB && b < 16; ++b) { if (tot[b] > tot[bi]) bi = b; if (tot[b] > 0) nz.push_back(tot[b]); }
        const long long best = tot[bi]; if (best <= 0) return;
        double med = (double)best;
        if (nz.size() > 2) { std::sort(nz.begin(), nz.end()); const size_t m = nz.size(); med = m % 2 ? (double)nz[m / 2] : (nz[m / 2 - 1] + nz[m / 2]) / 2.0; }
        const double dens = med > 0 ? best / med : 0;
        if (dens < DENS_X || best < (minLotsOverride > 0 ? minLotsOverride : p.minLots)) return;
        const long long loD = MIN_DIST + (long long)BAND * bi, hiD = loD + BAND - 1;
        const long long lo = side == BID ? lastPx - hiD : lastPx + loD, hi = side == BID ? lastPx - loD : lastPx + hiD;
        // one active zone per side near a price: an overlapping active zone only refreshes
        for (size_t li = live.size(); li-- > 0;) {
            Zone& z = zones[live[li]]; if (!z.active()) { continue; }
            if (z.side == side && lo <= z.hi + BAND && hi >= z.lo - BAND) { if (z.touchT < 0 && best > z.size) z.size = best; return; }
        }
        Zone z; z.id = nextId++; z.side = side; z.per = per; z.T = T; z.lo = lo; z.hi = hi; z.size = best; z.peak = best; z.dens = dens;
        double oldest = 0; long long bo = 0, bs = 0, bp = 0;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) {
            const Ord& o = it->second; if (o.side != side || o.px < lo || o.px > hi || o.tAdd > T - MIN_AGE_MIN * MIN_MS) continue;
            oldest = std::max(oldest, (T - o.tAdd) / 60000.0);
            if (o.sz > bs) { bs = o.sz; bo = it->first; bp = o.px; }
        }
        z.oldestMin = oldest;
        if (bs >= M.bigLots) { z.bigOid = bo; z.bigSz = bs; z.bigPx = bp; }
        zones.push_back(z); live.push_back((int)zones.size() - 1);
        char b[240]; std::snprintf(b, sizeof(b), "ZN,%d,%lld,%d,%lld,%lld,%lld,%.2f,%.1f,%d,%lld,%lld", z.id, T, side, lo, hi, best, dens, oldest, per, z.bigSz, z.bigPx);
        Line l; l.t = T; l.text = b; out.push_back(l);
    }
};
} // namespace zd
