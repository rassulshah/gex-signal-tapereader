#ifndef LS_DELTA_PROFILE_CORE_H
#define LS_DELTA_PROFILE_CORE_H

// DeltaProfileCore.h 2.21.0 (2.21.0: host only - the trapped traders read lives in TrapLogic.h) 2.20.0 (2.20.0: host only - the wick absorption A lives in AbsorbLogic.h) 2.19.1 (2.19.1: host colour only) 2.19.0 (2.19.0, Rassul 2026-10-19: grayPick - one gray circle per candle for a big node with no coloured A) 2.18.3 (2026-10-18: host settings only - font default 10, core unchanged) / 2.18.2 (settings audit) / 2.18.0 (2.18.0, Rassul 2026-10-18: aStep / nextPivotOk - the A close may come from the node's own candle OR the next candle, a next-candle A needs the lows (bullish) / highs (bearish) of BOTH bars beyond the pivot bar's high / low; swingPivot also returns each bar's pivot bar; labelKeys - Rassul 12:49 CT: the ratio label only on the 3 biggest profile bars by abs delta, at any ratio) 2.17.0 (2.17.0, 2026-10-18: tentStep - the tentative-A life cycle (final / shown / hidden / failed), boardFlag (SignalBoard A flag 1 final / 2 tentative), dpFacts - the Market State "DP" facts of one closed bar) 2.16.0 (2.16.0, Rassul 2026-10-17: the pivot rule against the peak bar's own swing - swingPivot - may repaint until the pivot is confirmed) 2.15.0 (2.15.0, Rassul 2026-10-17: the 2.6.0 rules - top 3 rows, no halfway rule, more than one bubble per candle - plus the pivot rule and the same-candle close) 2.13.0 (2.13.0, Rassul 2026-10-16: absorption only - every row >= 2.0x the average |delta| is a candidate (Snapshot::cands, no top-3 cap), no 10%-of-net gate in classify, ONE bubble per candle (the higher ratio)) 2.8.5 (2.8.5, 2026-10-14 23:05 CT: versionBefore / recheckA / oneBubblePerCandle - display only, records unchanged) 2.8.4 (2.8.4: halfwayOk / pivotOk shared with the host; 2.8.2 halfway rule in classify) 2.7.0 (absorption unchanged since 2.6.2: the peak bar's own close; 2.7.0 adds exh:: the thin-extreme exhaustion candidate) -- SDK-independent C++11 calculation layer. No drawing, files, static chart state,
// wall clock, market aliases, or pixel coordinates influence these results.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <map>   // (2.19.0) grayPick
#include <set>

namespace delta_profile {
typedef std::int64_t Tick;
struct Row {
    Tick key;
    double delta, volume;
    Row(Tick k = 0, double d = 0, double v = 0) : key(k), delta(d), volume(v) {}
};
struct Bar {
    int index;
    std::int64_t time; // chart-local epoch seconds; must be nondecreasing
    bool closed;      // an unverified/forming candle cannot confirm a signal
    double open, high, low, close;
    std::vector<Row> rows; // normalized, unique tick keys, ascending
    Bar() : index(-1), time(0), closed(false), open(0), high(0), low(0), close(0) {}
};
struct State {
    std::string code;
    bool support;
    int peakBar;
    int decidedBar;   // (2.5.0) the bar whose close decided it (-1 = undecided)
    int formedBar;    // last contributing formation bar; never substitute the peak for this
    long long eventId; // adapter-owned identity shared by profile label and historical bubble
    State() : support(false), peakBar(-1), decidedBar(-1), formedBar(-1), eventId(-1) {}
    bool valid() const { return !code.empty(); }
};
struct Bucket {
    Tick key, low, high;
    double delta, volume, ratio;
    Bucket() : key(0), low(0), high(0), delta(0), volume(0), ratio(0) {}
};
struct Node { Tick bucket; double price, ratio; State state; };
struct Zone { Tick low, high; std::string code; bool support; int share; int peakBar = -1;   // (2.4.3) peakBar: where the zone's absorption happened, for its circle
              double net = 0; int decidedBar = -1; int formedBar = -1; long long eventId = -1; };
struct Snapshot {
    double tick, meanAbsDelta, maxAbsDelta, maxVolume;
    Tick ticksPerRow;
    std::vector<Bucket> buckets;
    std::vector<Node> nodes; // at most three, descending magnitude, deterministic ties (display)
    std::vector<Node> cands; // (2.13.0) EVERY row at 2.0x or more of the average |delta| that classifies - the absorption candidates (no top-3 cap)
    std::vector<Zone> zones;
    Snapshot() : tick(0), meanAbsDelta(0), maxAbsDelta(0), maxVolume(0), ticksPerRow(1) {}
};
inline bool finite(double x) { return std::isfinite(x); }
inline Tick priceKey(double price, double tick) {
    if (!finite(price) || !finite(tick) || tick <= 0)
        throw std::invalid_argument("invalid price/tick");
    const double q = price / tick;
    // Conservative bound also leaves headroom for group-edge arithmetic.
    if (!finite(q) || std::fabs(q) >= 9.0e15)
        throw std::overflow_error("price/tick outside supported integer range");
    return static_cast<Tick>(std::llround(q));
}
inline Tick floorDiv(Tick a, Tick b) {
    if (b <= 0) throw std::invalid_argument("nonpositive group size");
    Tick q = a / b, r = a % b;
    return q - (r < 0 ? 1 : 0);
}
inline void normalize(Bar& bar) {
    std::map<Tick, Row> merged;
    for (std::size_t i = 0; i < bar.rows.size(); ++i) {
        const Row& r = bar.rows[i];
        if (!finite(r.delta) || !finite(r.volume) || r.volume < 0 ||
            std::fabs(r.delta) > r.volume + 1e-6 * std::max(1.0, r.volume))
            throw std::invalid_argument("invalid volume-at-price row");
        Row& m = merged[r.key]; m.key = r.key;
        m.delta += r.delta; m.volume += r.volume;
        if (!finite(m.delta) || !finite(m.volume))
            throw std::overflow_error("volume aggregation overflow");
    }
    bar.rows.clear(); bar.rows.reserve(merged.size());
    for (std::map<Tick, Row>::const_iterator i = merged.begin(); i != merged.end(); ++i)
        bar.rows.push_back(i->second);
}
inline double regionalDelta(const Bar& b, Tick low, Tick high) {
    std::vector<Row>::const_iterator i = std::lower_bound(
        b.rows.begin(), b.rows.end(), low,
        [](const Row& r, Tick k) { return r.key < k; });
    double d = 0;
    for (; i != b.rows.end() && i->key <= high; ++i) d += i->delta;
    return d;
}
// (2.8.4) the two 2.8.2 / 2.8.3 A rules as pure functions, shared by the host's decision loop, classify() and the tests.
// halfwayOk (2.8.2, Rassul 2026-10-14 19:44 "for bullish circle the absorption needs to be at or below the half way mark of the
// candle and vice versa"): the node's middle (in tick keys) at / below the peak bar's (high+low)/2 for a SELLING node (bullish A),
// at / above it for a BUYING node (bearish A). Unknown high / low = fails.
// (2.8.4) compared in WHOLE TICKS (twice the midpoints, integers): 2.8.2 compared the node's integer key midpoint with the bar's raw
// (high + low) / 2 / tick in double, so with a float tick that is not exact in binary (GC 0.1, HG 0.0005, NG 0.001) a node exactly AT
// the halfway mark read ~0.0006 tick ABOVE it and a valid bullish A was refused (test_audit_257 GC / HG / NG).
inline bool halfwayOk(Tick low, Tick high, double barHigh, double barLow, double tick, bool sellNode) {
    if (!finite(barHigh) || !finite(barLow) || !finite(tick) || tick <= 0) return false;
    const double qh = barHigh / tick, ql = barLow / tick;
    if (!finite(qh) || !finite(ql) || std::fabs(qh) >= 9.0e15 || std::fabs(ql) >= 9.0e15) return false;
    const Tick node2 = low + high, bar2 = static_cast<Tick>(std::llround(qh)) + static_cast<Tick>(std::llround(ql));
    return sellNode ? (node2 <= bar2) : (node2 >= bar2);
}
// pivotOk (2.8.3, Rassul 2026-10-14 19:47): his 1-bar ZigZag (the same walk as CobImpulseLogic legPivot / trade_def zigzag) over
// chart bars 0 .. pk ONLY (decided once at the peak bar's close; later bars can never change it). Bullish A: bar pk CREATES the
// down pivot (it is the current down leg's low) or its LOW is below the HIGH of the latest down-pivot bar. Bearish A: mirror (pk
// creates the up pivot, or its HIGH is above the LOW of the latest up-pivot bar). H / L: any indexable arrays (RTARRAY, vector).
template <class H, class L>
inline bool pivotOk(H& high, L& low, int pk, bool bullish, int* pivotBar = nullptr) {
    if (pivotBar) *pivotBar = -1;
    if (pk < 1) return false;
    auto hi = [&](int i) { return static_cast<double>(high[i]); };
    auto lo = [&](int i) { return static_cast<double>(low[i]); };
    for (int i = 0; i <= pk; ++i) if (!finite(hi(i)) || !finite(lo(i)) || hi(i) < lo(i)) return false;
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0, start = 0;
    for (int i = 1; i <= pk; ++i) {
        if (dirn == 1) {
            if (hi(i) > hi(ext)) ext = i;
            else if (lo(i) < lo(i - 1) && i != ext) { start = ext; ext = i; dirn = -1; }
        } else {
            if (lo(i) < lo(ext)) ext = i;
            else if (hi(i) > hi(i - 1) && i != ext) { start = ext; ext = i; dirn = 1; }
        }
    }
    if (bullish) {
        const int piv = dirn == -1 ? ext : start;          // the down pivot (the current down leg's low, else the up leg's start)
        if (pivotBar) *pivotBar = piv;
        return piv == pk || lo(pk) < hi(piv);
    }
    const int piv = dirn == 1 ? ext : start;               // the up pivot (the current up leg's high, else the down leg's start)
    if (pivotBar) *pivotBar = piv;
    return piv == pk || hi(pk) > lo(piv);
}
// swingPivot (2.16.0, Rassul 2026-10-17 "it is ok to repaint when the pivot requirement is there"; "1. yes" for the Delta Profile):
// the pivot rule judged against the peak bar's OWN swing - his 1-bar ZigZag over ALL the chart's bars, the bar belongs to the leg
// (A, B] between two turns; its down pivot = the low end of that leg, its up pivot = the high end. Bullish: the bar is its down pivot
// or its LOW is below that pivot bar's HIGH; bearish mirror. The last leg ends at the extreme so far (not confirmed): bull / bear may
// change until conf[i] = 1 (the turn after it is confirmed). Same walk as BigBidAskLogic.h bba::swing (tests compare them).
// ---- (2.17.0, 2026-10-18, audit B finding 1) the tentative A and the shared facts -------------------------------------------------
// One A candidate on its CLOSED peak bar: closeOk = the peak bar closed through the node in its own colour; pivNow = the pivot rule holds
// on the bars so far (the peak bar's own swing); pivFinal = that swing pivot is confirmed (cannot change any more).
enum TentStep { TS_RULED_OUT = 0, TS_FINAL = 1, TS_SHOWN = 2, TS_HIDDEN = 3, TS_FAILED = 4, TS_PENDING = 5 };   // (2.18.0) PENDING = own close failed, next candle not closed yet
inline TentStep tentStep(bool closeOk, bool pivNow, bool pivFinal) {
    if (!closeOk) return TS_RULED_OUT;                  // never an A (the same-candle close failed)
    if (pivFinal) return pivNow ? TS_FINAL : TS_FAILED; // decided: final A, or a shown A whose confirmed pivot went against it
    return pivNow ? TS_SHOWN : TS_HIDDEN;               // undecided: the tentative A shows / is hidden for now (may come back)
}
// (2.18.0, Rassul 2026-10-18 "the close can be from the same candle or the close of the next candle as long as the low of both bars is
// below the high of the pivot bar which may or may not be either bar for bullish absorption and vice versa for the bearish absorption")
// closeSame = the node's own candle closed beyond the node in its colour (the 2.6.2 rule, unchanged, wins); otherwise the NEXT candle
// decides: nextClosed = it has closed, closeNext = it closed beyond the node in its colour (bullish green above the node's high, bearish
// red below its low), pivBoth = the lows (bullish) / highs (bearish) of BOTH bars are beyond the pivot bar's high / low (nextPivotOk).
// A node whose own candle failed is PENDING for exactly one candle, then ruled out for good - a third candle never confirms it.
// (2.18.0, Rassul 2026-10-18 12:49 CT) the profile's ratio labels: ONLY the 3 biggest bars by abs(delta), even one under 2.0x; no other bar,
// even at 2.0x or more. Non-finite deltas are skipped; ties go to the lower key (stable, load-order independent). Returns the keys.
inline std::vector<Tick> labelKeys(const std::vector<Bucket>& buckets, std::size_t n = 3) {
    std::vector<const Bucket*> v;
    for (const Bucket& b : buckets) if (finite(b.delta)) v.push_back(&b);
    std::sort(v.begin(), v.end(), [](const Bucket* a, const Bucket* b) {
        const double x = std::fabs(a->delta), y = std::fabs(b->delta); return x != y ? x > y : a->key < b->key; });
    std::vector<Tick> out;
    for (std::size_t i = 0; i < v.size() && i < n; ++i) out.push_back(v[i]->key);
    return out;
}
enum ConfBy { CONF_NONE = 0, CONF_SAME = 1, CONF_NEXT = 2 };
struct AStep { TentStep step; ConfBy by; };
inline AStep aStep(bool closeSame, bool pivSame, bool nextClosed, bool closeNext, bool pivBoth, bool pivFinal) {
    AStep r; r.by = CONF_NONE;
    if (closeSame) { r.step = tentStep(true, pivSame, pivFinal); r.by = CONF_SAME; return r; }
    if (!nextClosed) { r.step = TS_PENDING; return r; }
    if (!closeNext) { r.step = TS_RULED_OUT; return r; }
    r.step = tentStep(true, pivBoth, pivFinal); r.by = CONF_NEXT; return r;
}
// (2.18.0) the next-candle pivot part: bar nb (the confirming candle) against the node candle's pivot bar piv (from swingPivot's dPiv /
// uPiv for the node candle). Bullish: nb IS the pivot bar or its LOW is below the pivot bar's HIGH; bearish: nb is it or its HIGH is
// above the pivot bar's LOW. The node candle's own part is swingPivot's bull / bear flag (same rule, same bad-bar guard).
template <class H, class L>
inline bool nextPivotOk(H& high, L& low, int n, int nb, int piv, bool bullish) {
    if (nb < 0 || nb >= n || piv < 0 || piv >= n) return false;
    if (nb == piv) return true;
    const double h = static_cast<double>(high[piv]), l = static_cast<double>(low[piv]);
    const double bh = static_cast<double>(high[nb]), bl = static_cast<double>(low[nb]);
    if (!finite(h) || !finite(l) || !finite(bh) || !finite(bl) || h < l || bh < bl) return false;
    return bullish ? bl < h : bh > l;
}
// SignalBoard kind 'A' flag (SignalBoard.h 1.1.0): 1 = final, 2 = tentative
inline int boardFlag(bool tentative) { return tentative ? 2 : 1; }
// ---- (2.19.0, Rassul 2026-10-19 17:09-17:13 CT "the circle is just a gray circle and you dont know bullish or bearish until candle
// close"; "stay gray, keep the pivot rule and the next candle rule too"; "the idea is to see if volume is trapped and which way the
// close occurred at a pivot") GRAY circles: a big node (one of the 3 biggest rows, 2.0x+) that is NOT a coloured A - still forming,
// waiting for the next candle, its pivot not holding (yet), or decided no winner - is drawn as ONE gray circle on its candle. A candle
// that has a coloured A shows only that (colour wins); of several gray nodes on one candle the biggest multiple, then the earliest.
struct GrayCand { long long id = -1; long long candle = 0; double mult = 0; bool coloured = false; bool open = false; };   // open = code A? / I?
inline std::vector<long long> grayPick(const std::vector<GrayCand>& c) {
    std::map<long long, const GrayCand*> best; std::set<long long> col;
    for (const GrayCand& g : c) if (g.coloured) col.insert(g.candle);
    for (const GrayCand& g : c) {
        if (g.coloured || !g.open || col.count(g.candle)) continue;
        std::map<long long, const GrayCand*>::iterator it = best.find(g.candle);
        if (it == best.end() || g.mult > it->second->mult + 1e-12 || (std::fabs(g.mult - it->second->mult) <= 1e-12 && g.id < it->second->id)) best[g.candle] = &g;
    }
    std::vector<long long> out; for (std::map<long long, const GrayCand*>::const_iterator it = best.begin(); it != best.end(); ++it) out.push_back(it->second->id);
    return out;
}
// Market State "DP" facts (MARKET-STATE-SPEC.md) of one closed bar. Prices with the market's decimals (from the tick), ratios 1 decimal.
struct FactNode { double px = 0, x = 0; bool buy = false; };   // a top delta node: price, ratio (x the average |delta|), net buying?
struct FactA { double px = 0, x = 0; bool bull = false, fin = false; long long bar = 0; };   // an A: price, ratio, side, final?, peak bar
inline int tickDecimals(double tick) {
    if (!(tick > 0) || !finite(tick)) return 2;
    for (int d = 0; d <= 8; ++d) { const double m = tick * std::pow(10.0, d); if (std::fabs(m - std::floor(m + 0.5)) < 1e-6) return d; }
    return 8;
}
inline std::string fixedText(double v, int decimals) {
    if (!finite(v)) return "?";
    char b[64]; std::snprintf(b, sizeof(b), "%.*f", decimals < 0 ? 0 : decimals > 8 ? 8 : decimals, v); return b;
}
// top = the profile's top nodes (biggest first); onBar = the A events whose peak bar IS this bar (final or tentative, as they stand at
// its close); confirmed / failed = A events (any earlier peak bar) that turned final / failed since the previous posted bar.
inline std::map<std::string, std::string> dpFacts(const std::vector<FactNode>& top, const std::vector<FactA>& onBar,
                                                  const std::vector<FactA>& confirmed, const std::vector<FactA>& failed, double tick) {
    std::map<std::string, std::string> kv; const int d = tickDecimals(tick);
    if (!top.empty()) {
        kv["node_px"] = fixedText(top[0].px, d); kv["node_x"] = fixedText(top[0].x, 1); kv["node_side"] = top[0].buy ? "buy" : "sell";
        std::string t3; for (std::size_t i = 0; i < top.size() && i < 3; ++i) { if (i) t3 += "/"; t3 += fixedText(top[i].px, d); }
        kv["top3"] = t3;
    } else { kv["node_px"] = "?"; kv["node_x"] = "?"; kv["node_side"] = "?"; kv["top3"] = "?"; }
    // the A on this bar: a final one wins over a tentative one, then the stronger
    const FactA* best = nullptr;
    for (const FactA& a : onBar) {
        if (!finite(a.px) || !finite(a.x)) continue;
        if (!best || (a.fin && !best->fin) || (a.fin == best->fin && a.x > best->x)) best = &a;
    }
    kv["a"] = best ? (best->fin ? "final" : "tentative") : "none";
    kv["a_side"] = best ? (best->bull ? "bull" : "bear") : "?";
    kv["a_px"] = best ? fixedText(best->px, d) : "?";
    kv["a_x"] = best ? fixedText(best->x, 1) : "?";
    auto strongest = [](const std::vector<FactA>& v) { const FactA* b = nullptr; for (const FactA& a : v) if (finite(a.px) && finite(a.x) && (!b || a.x > b->x)) b = &a; return b; };
    const FactA* c = strongest(confirmed); const FactA* f = strongest(failed);
    kv["a_conf_n"] = std::to_string(confirmed.size());
    kv["a_conf_px"] = c ? fixedText(c->px, d) : "?"; kv["a_conf_side"] = c ? (c->bull ? "bull" : "bear") : "?"; kv["a_conf_bar"] = c ? std::to_string(c->bar) : "?";
    kv["a_fail_n"] = std::to_string(failed.size());
    kv["a_fail_px"] = f ? fixedText(f->px, d) : "?"; kv["a_fail_side"] = f ? (f->bull ? "bull" : "bear") : "?"; kv["a_fail_bar"] = f ? std::to_string(f->bar) : "?";
    return kv;
}
template <class H, class L>
inline void swingPivot(H& high, L& low, int n, std::vector<char>& bull, std::vector<char>& bear, std::vector<char>& conf,
                       std::vector<int>* dPiv = nullptr, std::vector<int>* uPiv = nullptr) {   // (2.18.0) + each bar's down / up pivot bar (-1 none)
    const std::size_t N = n > 0 ? static_cast<std::size_t>(n) : 0;
    bull.assign(N, 0); bear.assign(N, 0); conf.assign(N, 0);
    if (dPiv) dPiv->assign(N, -1);
    if (uPiv) uPiv->assign(N, -1);
    if (n < 3) return;
    auto hi = [&](int i) { return static_cast<double>(high[i]); };
    auto lo = [&](int i) { return static_cast<double>(low[i]); };
    auto good = [&](int i) { return finite(hi(i)) && finite(lo(i)) && hi(i) >= lo(i); };
    std::vector<int> piv; std::vector<signed char> typ;
    int dirn = hi(1) >= hi(0) ? 1 : -1, ext = 0, bad = -1;
    for (int i = 1; i < n; ++i) {
        if (!good(i)) { bad = i; continue; }
        if (dirn == 1) {
            if (hi(i) > hi(ext)) ext = i;
            else if (lo(i) < lo(i - 1) && i != ext) { piv.push_back(ext); typ.push_back(1); ext = i; dirn = -1; }
        } else {
            if (lo(i) < lo(ext)) ext = i;
            else if (hi(i) > hi(i - 1) && i != ext) { piv.push_back(ext); typ.push_back(-1); ext = i; dirn = 1; }
        }
    }
    const std::size_t nConf = piv.size();
    piv.push_back(ext); typ.push_back(dirn == 1 ? 1 : -1);
    auto mark = [&](int i, int D, int U, bool c) {
        conf[i] = c ? 1 : 0;
        if (!good(i) || (bad >= 0 && i > bad && i - bad < 50)) return;
        if (dPiv) (*dPiv)[i] = D;
        if (uPiv) (*uPiv)[i] = U;
        bull[i] = D >= 0 && (D == i || lo(i) < hi(D)) ? 1 : 0;
        bear[i] = U >= 0 && (U == i || hi(i) > lo(U)) ? 1 : 0;
    };
    for (std::size_t k = 0; k < piv.size(); ++k) {
        const int A = k ? piv[k - 1] : -1, B = piv[k];
        const int D = typ[k] < 0 ? B : (k ? piv[k - 1] : -1), U = typ[k] > 0 ? B : (k ? piv[k - 1] : -1);
        for (int i = A + 1; i <= B && i < n; ++i) mark(i, D, U, k < nConf);
    }
    const int E = piv.back();
    if (E + 1 < n) {
        const bool down = typ.back() > 0;
        int T = E + 1; for (int i = E + 2; i < n; ++i) if (good(i) && (down ? lo(i) < lo(T) : hi(i) > hi(T))) T = i;
        for (int i = E + 1; i < n; ++i) { const bool first = i <= T;
            mark(i, down ? T : (first ? E : -1), down ? (first ? E : -1) : T, false); }
    }
}
// ---- (2.8.5, Rassul approved 2026-10-14 23:05 CT) display-only rules for the A bubbles. The records are never changed. ----
// versionBefore: true when the record's version text is earlier than a.b.c, or cannot be read (an unreadable version is treated
// as old, so it is re-checked rather than trusted). "2.8.0" < "2.8.4" <= "2.8.4" < "2.8.10".
inline bool versionBefore(const std::string& v, int a, int b, int c) {
    long part[3] = { -1, -1, -1 }; std::size_t i = 0; int n = 0;
    while (n < 3) {
        if (i >= v.size() || v[i] < '0' || v[i] > '9') return true;
        long x = 0; int digits = 0;
        while (i < v.size() && v[i] >= '0' && v[i] <= '9') { if (++digits > 6) return true; x = x * 10 + (v[i] - '0'); ++i; }
        part[n++] = x;
        if (n < 3) { if (i >= v.size() || v[i] != '.') return true; ++i; }
    }
    if (i != v.size()) return true;
    if (part[0] != a) return part[0] < a;
    if (part[1] != b) return part[1] < b;
    return part[2] < c;
}
// recheckA (2.8.5 rule 2): the 2.8.4 A rules applied ONCE to a record decided by an earlier version, on the chart's bars 0 .. pk only:
// the same-bar close beyond the node (selling node: green close above the node's top tick; buying node: red close below its bottom
// tick), the 2.8.2 halfway rule (halfwayOk) and the 2.8.3 pivot rule (pivotOk). bars = false when the bars needed are not on the chart
// (peak bar missing / still forming / no bar before it / a non-finite or inverted bar in 0 .. pk): the record is then left undrawn.
struct Recheck { bool bars = false, close = false, halfway = false, pivot = false; int pivotBar = -1;
                 bool pass() const { return bars && close && pivot; } };   // (2.15.0) halfway no longer required (still reported)
template <class O, class H, class L, class C>
inline Recheck recheckA(O& open, H& high, L& low, C& close, int count, int pk, Tick nodeLow, Tick nodeHigh, double tick, bool sellNode) {
    Recheck r;
    if (pk < 1 || pk >= count - 1 || !finite(tick) || tick <= 0) return r;
    for (int i = 0; i <= pk; ++i) {
        const double h = static_cast<double>(high[i]), l = static_cast<double>(low[i]);
        if (!finite(h) || !finite(l) || h < l) return r;
    }
    const double po = static_cast<double>(open[pk]), pc = static_cast<double>(close[pk]);
    if (!finite(po) || !finite(pc) || !finite(pc / tick) || std::fabs(pc / tick) >= 9.0e15) return r;
    r.bars = true;
    const Tick pck = static_cast<Tick>(std::llround(pc / tick));
    r.close = sellNode ? (pc > po && pck > nodeHigh) : (pc < po && pck < nodeLow);
    r.halfway = halfwayOk(nodeLow, nodeHigh, static_cast<double>(high[pk]), static_cast<double>(low[pk]), tick, sellNode);
    r.pivot = pivotOk(high, low, pk, sellNode, &r.pivotBar);
    return r;
}
// oneBubblePerCandle (2.8.5 rule 1): ONE bubble per candle per side. Among the drawable confirmed A events whose peak bar is the same
// candle and side, the bubble goes to the event with the LOWEST price for a bullish (support) A and the HIGHEST price for a bearish
// (resistance) A - but only among the events decided at the group's FIRST decision moment (its smallest t_decided: the candle's
// close when IRT was running). An event decided later (a re-track after the row size changed, a node first seen later) can never
// move a bubble that is already drawn; it joins it. A lower (bullish) / higher (bearish) event decided at that same first moment
// does take the bubble - at that moment every candidate exists together. Ties (same price): the lowest id. Pure function of the
// set: independent of load order, the same after a restart, unchanged by later bars. Returns event id -> the id that holds its group's
// bubble (itself for the chosen one).
struct BubbleCand { long long id = 0, candle = 0, tDecided = 0; bool support = false; double price = 0, mult = 0; };   // (2.13.0) mult = the row's ratio
inline std::map<long long, long long> oneBubblePerCandle(const std::vector<BubbleCand>& v, double tick) {
    const double t = finite(tick) && tick > 0 ? tick : 0;
    auto halfTicks = [t](double p) -> long long { return t > 0 && finite(p / t) && std::fabs(p / t) < 4.0e15 ? std::llround(p / t * 2.0) : 0; };
    auto better = [&](const BubbleCand& a, const BubbleCand& b) {   // (2.13.0) a beats b inside one candle: the HIGHER ratio, then the lower id
        (void)halfTicks;
        if (a.mult != b.mult) return a.mult > b.mult;
        return a.id < b.id;
    };
    std::map<std::pair<long long, bool>, std::vector<const BubbleCand*> > groups;
    for (std::size_t i = 0; i < v.size(); ++i) groups[std::make_pair(v[i].candle, false)].push_back(&v[i]);   // (2.13.0) ONE bubble per candle, either side
    std::map<long long, long long> out;
    for (std::map<std::pair<long long, bool>, std::vector<const BubbleCand*> >::const_iterator g = groups.begin(); g != groups.end(); ++g) {
        long long first = g->second[0]->tDecided;
        for (const BubbleCand* c : g->second) first = std::min(first, c->tDecided);
        const BubbleCand* best = nullptr;
        for (const BubbleCand* c : g->second) if (c->tDecided == first && (!best || better(*c, *best))) best = c;
        for (const BubbleCand* c : g->second) out[c->id] = best->id;
    }
    return out;
}
inline State classify(const std::vector<Bar>& bars, Tick low, Tick high,
                      double net, double tick) {
    State out;
    if (bars.empty() || net == 0 || !finite(net) || !finite(tick) || tick <= 0 || low > high)
        return out;
    const bool sell = net < 0;
    int formed = -1, peak = -1;
    double peakMagnitude = -1;
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const double d = regionalDelta(bars[i], low, high);
        if (d != 0 && (d < 0) == sell) {   // (2.13.0) no 10%-of-net gate: the peak is simply the biggest same-side bar
            formed = static_cast<int>(i);
            if (std::fabs(d) > peakMagnitude) { peakMagnitude = std::fabs(d); peak = formed; }
        }
    }
    if (formed < 0 || peak < 0) return out;
    out.peakBar = bars[static_cast<std::size_t>(peak)].index;
    out.formedBar = bars[static_cast<std::size_t>(formed)].index;
    // (2.5.7, audit F1) beyond the node is decided in WHOLE TICKS (IRT prices and tick are float: a close ON the node's edge could
    // read a hair beyond it in double); a price's own tick key against the node's keys [low, high]
    auto key = [tick](double price) -> Tick { const double q = price / tick; return finite(q) && std::fabs(q) < 9.0e15 ? static_cast<Tick>(std::llround(q)) : 0; };
    // (2.6.2, Rassul 2026-10-13 05:16 "the same bar has to have a green close above the absorption") absorption is decided by
    // the node's OWN PEAK BAR (the bar holding the largest share of its delta - the bar its circle is drawn on), once that bar
    // has CLOSED (a forming bar never decides):
    //   a SELLING node is A (support, bullish)     only if its peak bar closes GREEN (close > open) ABOVE the node (tick key > high);
    //   a BUYING  node is A (resistance, bearish)  only if its peak bar closes RED   (close < open) BELOW the node (tick key < low).
    // A doji (close == open) is neither colour -> not A. A later bar can never make a node A.
    // A node whose peak bar did not confirm is not absorption: it can still become I (initiative) when the FIRST closed bar after
    // the node formed closes through it the other way in its own colour (selling node: red close below = sellers won;
    // buying node: green close above = buyers won); a close beyond the absorption side no longer decides anything.
    // While the peak bar is still forming: A? (or I? when price already trades a full tick through the other way, as before).
    // After the peak bar failed and before any I close: I? (no circle; the label no longer promises absorption).
    const Bar& pb = bars[static_cast<std::size_t>(peak)];
    const bool peakKnown = pb.closed && finite(pb.open) && finite(pb.close);
    if (peakKnown) {
        const Tick pk = key(pb.close);
        // (2.8.2, Rassul 2026-10-14 19:44) bullish A: the node at / below the peak bar's halfway mark; bearish A: at / above it
        // (2.8.4) the window holds only the last 90 min, so the 2.8.3 pivot rule (chart bars 0 .. peak) is NOT applied here: the
        // host never publishes this A as decided - a new event starts undecided and its own decision loop applies every rule
        const bool half = true;   // (2.15.0, Rassul 2026-10-17 13:25 "it doesn't need the halfway rule") halfwayOk kept for the record re-check only
        const bool confirmed = half && (sell ? (pb.close > pb.open && pk > high) : (pb.close < pb.open && pk < low));
        if (confirmed) { out.code = "A"; out.support = sell; out.decidedBar = pb.index; return out; }
        for (std::size_t i = static_cast<std::size_t>(formed + 1); i < bars.size(); ++i) {
            const Bar& b = bars[i];
            if (!b.closed || !finite(b.close) || !finite(b.open)) continue;
            const Tick ck = key(b.close);
            if (sell && ck < low && b.close < b.open) { out.code = "I"; out.support = false; out.decidedBar = b.index; return out; }
            if (!sell && ck > high && b.close > b.open) { out.code = "I"; out.support = true; out.decidedBar = b.index; return out; }
        }
        out.code = "I?"; out.support = !sell;
        return out;
    }
    out.code = "A?"; out.support = sell;
    {
        const Bar& last = bars.back();
        const bool initiative = sell ? (finite(last.low) && key(last.low) < low) : (finite(last.high) && key(last.high) > high);   // a full tick beyond
        if (static_cast<std::size_t>(formed + 1) < bars.size() && initiative) {
            out.code = "I?"; out.support = !sell;
        }
    }
    return out;
}
inline Snapshot build(const std::vector<Bar>& bars, double tick, bool allowConfirmation = true) {
    if (!finite(tick) || tick <= 0) throw std::invalid_argument("native tick size unavailable");
    Snapshot s; s.tick = tick;
    if (bars.empty()) return s;
    std::map<Tick, Row> profile;
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const Bar& b = bars[i];
        if ((i && b.time < bars[i - 1].time) || !finite(b.open) || !finite(b.high) ||
            !finite(b.low) || !finite(b.close) || b.high < b.low ||
            b.open < b.low || b.open > b.high || b.close < b.low || b.close > b.high)
            throw std::invalid_argument("invalid/nonchronological OHLC bars");
        for (std::size_t j = 0; j < b.rows.size(); ++j) {
            const Row& r = b.rows[j];
            if ((j && r.key <= b.rows[j - 1].key) || !finite(r.delta) || !finite(r.volume) || r.volume < 0 ||
                std::fabs(r.delta) > r.volume + 1e-6 * std::max(1.0, r.volume))
                throw std::invalid_argument("rows must be valid and normalized");
            Row& p = profile[r.key]; p.key = r.key; p.delta += r.delta; p.volume += r.volume;
            if (!finite(p.delta) || !finite(p.volume)) throw std::overflow_error("profile overflow");
        }
    }
    if (profile.empty()) return s;
    const Tick span = profile.rbegin()->first - profile.begin()->first;
    s.ticksPerRow = std::max<Tick>(1, span / 60 + (span % 60 != 0));
    std::map<Tick, Bucket> grouped;
    for (std::map<Tick, Row>::const_iterator i = profile.begin(); i != profile.end(); ++i) {
        const Tick key = floorDiv(i->first, s.ticksPerRow);
        Bucket& b = grouped[key];
        if (b.volume == 0) { b.low = i->first; b.high = i->first; }
        b.key = key; b.low = std::min(b.low, i->first); b.high = std::max(b.high, i->first);
        b.delta += i->second.delta; b.volume += i->second.volume;
    }
    std::size_t nonzero = 0;
    for (std::map<Tick, Bucket>::const_iterator i = grouped.begin(); i != grouped.end(); ++i) {
        s.buckets.push_back(i->second);
        const double a = std::fabs(i->second.delta);
        if (a > 0) { s.meanAbsDelta += a; ++nonzero; }
        s.maxAbsDelta = std::max(s.maxAbsDelta, a); s.maxVolume = std::max(s.maxVolume, i->second.volume);
    }
    s.meanAbsDelta = nonzero ? s.meanAbsDelta / static_cast<double>(nonzero) : 0;
    std::vector<std::size_t> ranked;
    for (std::size_t i = 0; i < s.buckets.size(); ++i) {
        s.buckets[i].ratio = s.meanAbsDelta > 0 ? std::fabs(s.buckets[i].delta) / s.meanAbsDelta : 0;
        ranked.push_back(i);
    }
    std::sort(ranked.begin(), ranked.end(), [&s](std::size_t a, std::size_t b) {
        const double x = std::fabs(s.buckets[a].delta), y = std::fabs(s.buckets[b].delta);
        return x != y ? x > y : s.buckets[a].key < s.buckets[b].key;
    });
    for (std::size_t i = 0; i < ranked.size() && i < 10 && s.nodes.size() < 3; ++i) {
        const Bucket& b = s.buckets[ranked[i]];
        if (b.ratio < 2.0) break;
        State state = classify(bars, b.low, b.high, b.delta, tick);
        if (!state.valid()) continue;
        if (!allowConfirmation && (state.code == "A" || state.code == "I")) state.code += "?";
        Node n; n.bucket = b.key; n.price = (static_cast<double>(b.low) + static_cast<double>(b.high)) * 0.5 * tick;
        n.ratio = b.ratio; n.state = state; s.nodes.push_back(n);
    }
    // (2.13.0) every row at 2.0x or more was a candidate; (2.15.0, Rassul 2026-10-17 13:25 "keep it as 2.6 has it") back to the 2.6.0
    // rule: only the 3 biggest rows (2.0x or more, from the 10 biggest) are absorption candidates
    for (std::size_t i = 0; i < ranked.size() && i < 10 && s.cands.size() < 3; ++i) {
        const Bucket& b = s.buckets[ranked[i]];
        if (b.ratio < 2.0) break;
        State state = classify(bars, b.low, b.high, b.delta, tick);
        if (!state.valid()) continue;
        if (!allowConfirmation && (state.code == "A" || state.code == "I")) state.code += "?";
        Node n; n.bucket = b.key; n.price = (static_cast<double>(b.low) + static_cast<double>(b.high)) * 0.5 * tick;
        n.ratio = b.ratio; n.state = state; s.cands.push_back(n);
    }
    // Zones use exactly the SAME 60-minute subset for both net delta and state.
    std::vector<Bar> zoneBars;
    std::map<Tick, double> zdelta;
    const std::int64_t cutoff = bars.back().time - 3600;
    for (std::size_t i = 0; i < bars.size(); ++i) if (bars[i].time > cutoff) {
        zoneBars.push_back(bars[i]);
        for (std::size_t j = 0; j < bars[i].rows.size(); ++j)
            zdelta[bars[i].rows[j].key] += bars[i].rows[j].delta;
    }
    if (zdelta.size() < 5) return s;
    double largest = 0, sellTotal = 0, buyTotal = 0;
    for (std::map<Tick, double>::const_iterator i = zdelta.begin(); i != zdelta.end(); ++i) {
        largest = std::max(largest, std::fabs(i->second));
        if (i->second < 0) sellTotal += -i->second; else buyTotal += i->second;
    }
    if (largest == 0) return s;
    const Tick bottom = zdelta.begin()->first, top = zdelta.rbegin()->first;
    const double range = std::max(1.0, static_cast<double>(top - bottom));
    std::vector<std::pair<Tick, double> > run;
    const auto finish = [&]() {
        if (run.size() >= 2) {
            const bool sell = run.front().second < 0;
            double net = 0; for (std::size_t i = 0; i < run.size(); ++i) net += run[i].second;
            const double total = sell ? sellTotal : buyTotal;
            const double share = total > 0 ? std::fabs(net) / total : 0;
            const bool edge = sell ? static_cast<double>(run.back().first) >= static_cast<double>(top) - 0.25 * range :
                static_cast<double>(run.front().first) <= static_cast<double>(bottom) + 0.25 * range;
            if (share >= 0.4 && edge) {
                const State state = classify(zoneBars, run.front().first, run.back().first, net, tick);
                const std::string name = sell ? "Dst" : "Acc";
                Zone z; z.low = run.front().first; z.high = run.back().first;
                z.share = static_cast<int>(std::floor(share * 100 + 0.5));
                z.peakBar = state.peakBar;
                z.net = net; z.decidedBar = state.decidedBar;   // (2.5.4) recorded, never drawn
                z.formedBar = state.formedBar;
                z.code = state.code == "A" ? "A" : state.code == "I" ? name : name + "?";
                z.support = (state.code == "A" || state.code == "I") ? state.support : !sell;
                if (!allowConfirmation && z.code.back() != '?') z.code += "?";
                s.zones.push_back(z);
            }
        }
        run.clear();
    };
    for (std::map<Tick, double>::const_iterator i = zdelta.begin(); i != zdelta.end(); ++i) {
        const bool heavy = i->second != 0 && std::fabs(i->second) >= 0.25 * largest;
        if (!heavy || (!run.empty() && ((i->second < 0) != (run.back().second < 0) || i->first - run.back().first > 2)))
            finish();
        if (heavy) run.push_back(*i);
    }
    finish();
    std::sort(s.zones.begin(), s.zones.end(), [](const Zone& a, const Zone& b) {
        return a.share != b.share ? a.share > b.share : a.low < b.low;
    });
    if (s.zones.size() > 2) s.zones.resize(2);
    return s;
}

// ---- (2.7.0, Rassul 2026-10-13 "the thin extreme seems promising for exhaustion signals especially after absorbtion or trapped
// detla" / "this looks good, lets implement this") the THIN EXTREME exhaustion candidate "Ex?". SDK-independent: closed bars,
// decision-time data only (every input is at or before the extreme bar; the print is decided by later CLOSED bars only).
namespace exh {
struct Params {
    int N = 10;            // new high / low of the prior N bars
    int L = 4;             // within L ticks of a key / gamma level
    int T = 2;             // top (bottom) T profile rows of the extreme are measured for thinness
    double thinX = 0.35;   // thin: mean |delta| of those rows < thinX x the profile's median |delta| per row
    double fadeX = 0.5;    // fading: aggressor delta of the last 2 bars < fadeX x the 2 bars before
    int M = 40;            // the move starts at the last opposite swing within M bars
    double trapX = 2.0;    // trapped node: >= trapX x the median per-bar node |delta|
    int K = -1;            // ... and price advanced no more than K ticks past it (-1 = auto: 2 profile rows)
    int B = 3;             //     for the next B bars
    int W = 3;             // print on the first bar closing back within W bars
    bool requireLevel = true;
    int show = 0;          // 0 none (log only), 1 ctx (only with A / T context), 2 all
    bool proven = false;   // false "Ex?", true "Ex"
    double aeFadeX = 0.5;  // (AE) later-half attacker delta < aeFadeX x the earlier half
    int aeMinBars = 3;     // (AE) at least this many bars traded in the node
    int aeMaxBars = 20;    // (AE) watched for this many bars after the A's peak bar, then decided N
    int showAe = 0;        // (AE) 0 none (log only, label stays A), 1 "AE?", 2 "AE"
};
struct Level { std::string name; double price; };
struct AEvent { int peakIndex; double price; bool bearish; };   // a confirmed A (chart bar index of its peak bar), decided by the extreme
struct Candidate {
    bool top = true; double extreme = 0;
    std::string level; double levelPx = 0; long long levelTicks = -1;
    double thin = 0, fade = -1; bool thinOk = false, fadeOk = false;
    char ctx = 'N'; double ctxPx = 0; int ctxIndex = -1;
    Tick ticksPerRow = 1;
    bool qualifies() const { return thinOk && fadeOk; }
};
inline Tick keyOf(double price, double tick) { return priceKey(price, tick); }
inline double barDelta(const Bar& b) { double d = 0; for (std::size_t i = 0; i < b.rows.size(); ++i) d += b.rows[i].delta; return d; }
inline double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
// Evaluate bars[p] (must be CLOSED) as a thin extreme on one side. false = not a candidate (no new extreme, no level in reach,
// not enough contiguous history, no profile). true = a candidate; out.qualifies() says whether thin AND fading both hold.
inline bool evaluate(const std::vector<Bar>& bars, std::size_t p, bool top, const Params& P,
                     const std::vector<Level>& levels, const std::vector<AEvent>& absorptions, double tick, Candidate& out) {
    out = Candidate(); out.top = top;
    if (p >= bars.size() || !finite(tick) || tick <= 0 || P.N < 1 || P.M < 1 || P.T < 1 || P.B < 1) return false;
    const Bar& e = bars[p];
    if (!e.closed) return false;
    const std::size_t H = static_cast<std::size_t>(std::max(std::max(P.N, P.M), 3));
    if (p < H) return false;
    for (std::size_t i = p - H; i < p; ++i)                       // contiguous chart bars only (a VAP gap = not evaluated)
        if (bars[i + 1].index != bars[i].index + 1 || !bars[i].closed) return false;
    const double ext = top ? e.high : e.low;
    if (!finite(ext)) return false;
    out.extreme = ext;
    for (std::size_t i = p - static_cast<std::size_t>(P.N); i < p; ++i)
        if (top ? !(e.high > bars[i].high) : !(e.low < bars[i].low)) return false;
    const Tick ek = keyOf(ext, tick);
    long long best = -1;
    for (std::size_t i = 0; i < levels.size(); ++i) {
        if (!finite(levels[i].price) || levels[i].price <= 0) continue;
        Tick lk; try { lk = keyOf(levels[i].price, tick); } catch (...) { continue; }
        const long long d = static_cast<long long>(lk > ek ? lk - ek : ek - lk);
        if (d <= P.L && (best < 0 || d < best)) { best = d; out.level = levels[i].name; out.levelPx = levels[i].price; }
    }
    out.levelTicks = best;
    if (best < 0) { if (P.requireLevel) return false; out.level = "none"; out.levelPx = 0; }
    // the profile AS OF the extreme's close: bars in (extreme time - 90 min, extreme time]; its whole span must be on hand
    const std::int64_t cutoff = e.time - 5400;
    bool covered = bars.front().index == 0;
    std::size_t first = p;
    for (std::size_t i = p + 1; i-- > 0;) { if (bars[i].time <= cutoff) { covered = true; break; } first = i; }
    if (!covered) return false;
    std::map<Tick, double> profile;
    for (std::size_t i = first; i <= p; ++i) for (std::size_t j = 0; j < bars[i].rows.size(); ++j) profile[bars[i].rows[j].key] += bars[i].rows[j].delta;
    if (profile.empty()) return false;
    const Tick span = profile.rbegin()->first - profile.begin()->first;
    const Tick tpr = std::max<Tick>(1, span / 60 + (span % 60 != 0));          // the same row size rule as build()
    out.ticksPerRow = tpr;
    std::map<Tick, double> band;
    for (std::map<Tick, double>::const_iterator i = profile.begin(); i != profile.end(); ++i) band[floorDiv(i->first, tpr)] += i->second;
    std::vector<double> mags;
    for (std::map<Tick, double>::const_iterator i = band.begin(); i != band.end(); ++i) if (i->second != 0) mags.push_back(std::fabs(i->second));
    const double med = median(mags);
    if (!(med > 0)) return false;
    const Tick eb = floorDiv(ek, tpr);
    double sum = 0;
    for (int r = 0; r < P.T; ++r) {
        const Tick k = top ? eb - r : eb + r;
        const std::map<Tick, double>::const_iterator at = band.find(k);
        if (at != band.end()) sum += std::fabs(at->second);
    }
    out.thin = sum / static_cast<double>(P.T) / med;
    out.thinOk = out.thin < P.thinX;
    const auto aggressor = [top](const Bar& b) { const double d = barDelta(b); return top ? std::max(0.0, d) : std::max(0.0, -d); };
    const double now2 = aggressor(bars[p]) + aggressor(bars[p - 1]), before2 = aggressor(bars[p - 2]) + aggressor(bars[p - 3]);
    out.fade = before2 > 0 ? now2 / before2 : -1;
    out.fadeOk = before2 > 0 && out.fade < P.fadeX;
    // the move: from the last opposite swing (the most recent lowest low for a top) within M bars
    std::size_t s = p - 1;
    for (std::size_t i = p - static_cast<std::size_t>(P.M); i < p; ++i)
        if (top ? bars[i].low <= bars[s].low : bars[i].high >= bars[s].high) s = i;
    // (a) a confirmed A of the opposite side's defence inside the move, beyond-the-extreme side excluded
    for (std::size_t i = 0; i < absorptions.size(); ++i) {
        const AEvent& a = absorptions[i];
        if (a.bearish != top || a.peakIndex < bars[s].index || a.peakIndex >= e.index || !finite(a.price)) continue;
        if (top ? !(a.price < ext) : !(a.price > ext)) continue;
        if (out.ctx != 'A' || a.peakIndex > out.ctxIndex) { out.ctx = 'A'; out.ctxPx = a.price; out.ctxIndex = a.peakIndex; }
    }
    if (out.ctx == 'A') return true;
    // (b) a trapped-delta node: a big node on the move's side that price could not advance past (K ticks) for the next B bars
    std::vector<double> nodes;
    auto nodeOf = [tpr](const Bar& b, bool wantBuy, double& mag, Tick& at) {
        std::map<Tick, double> bb;
        for (std::size_t j = 0; j < b.rows.size(); ++j) bb[floorDiv(b.rows[j].key, tpr)] += b.rows[j].delta;
        mag = 0; at = 0; double any = 0;
        for (std::map<Tick, double>::const_iterator i = bb.begin(); i != bb.end(); ++i) {
            any = std::max(any, std::fabs(i->second));
            const double side = wantBuy ? i->second : -i->second;
            if (side > mag) { mag = side; at = i->first; }
        }
        return any;
    };
    for (std::size_t i = first; i <= p; ++i) { double m; Tick a; const double any = nodeOf(bars[i], top, m, a); if (any > 0) nodes.push_back(any); }
    const double nodeMed = median(nodes);
    const Tick K = P.K >= 0 ? static_cast<Tick>(P.K) : 2 * tpr;
    if (nodeMed > 0) {
        for (std::size_t j = p; j-- > s;) {
            if (j + static_cast<std::size_t>(P.B) >= p) continue;                     // its B bars must all close before the extreme
            double m; Tick b;
            nodeOf(bars[j], top, m, b);
            if (!(m >= P.trapX * nodeMed) || m <= 0) continue;
            const Tick edgeKey = top ? (b + 1) * tpr - 1 : b * tpr;                   // the node's far edge in ticks
            bool trapped = true;
            for (std::size_t k = j + 1; k <= j + static_cast<std::size_t>(P.B) && trapped; ++k) {
                Tick hk; try { hk = keyOf(top ? bars[k].high : bars[k].low, tick); } catch (...) { trapped = false; break; }
                if (top ? hk - edgeKey > K : edgeKey - hk > K) trapped = false;
            }
            if (!trapped) continue;
            out.ctx = 'T'; out.ctxPx = (static_cast<double>(b * tpr) + static_cast<double>(b * tpr + tpr - 1)) * 0.5 * tick; out.ctxIndex = bars[j].index;
            break;                                                                    // the latest trapped node in the move
        }
    }
    return true;
}
// After the extreme at chart bar e: 0 = pending, 1 = printed at `at` (first bar closing back: red for a top, green for a bottom),
// 2 = cancelled at `at` (a later bar went beyond the extreme first), 3 = expired at `at` (no close back within W bars).
// o/h/l/c are chart arrays; lastClosed = the last CLOSED chart bar index. Only closed bars decide.
template <class A>
inline int resolve(A& o, A& h, A& l, A& c, int lastClosed, int e, bool top, int W, int& at) {
    at = -1;
    if (e < 0 || W < 1) return 0;
    const double ext = top ? static_cast<double>(h[e]) : static_cast<double>(l[e]);
    for (int f = e + 1; f <= e + W && f <= lastClosed; ++f) {
        const double hi = static_cast<double>(h[f]), lo = static_cast<double>(l[f]), op = static_cast<double>(o[f]), cl = static_cast<double>(c[f]);
        if (!finite(hi) || !finite(lo) || !finite(op) || !finite(cl)) { at = f; return 3; }
        if (top ? hi > ext : lo < ext) { at = f; return 2; }
        if (top ? cl < op : cl > op) { at = f; return 1; }
    }
    if (lastClosed >= e + W) { at = e + W; return 3; }
    return 0;
}
// ---- (2.7.0, scope update Rassul 2026-10-13 10:33 "too many signals .. focus on the absorbtion + exhaustion") AE: after a CONFIRMED A,
// did the attackers exhaust INSIDE the node? At the close of a bar d that traded in the node's price range (a bar AFTER the A's
// peak bar - the peak bar holds the node's biggest attack by construction, so a fade can only show on a later re-test), take
// every closed bar up to and including d that traded in the node: the attackers' delta into it (buyers' +delta for a buying
// node, sellers' -delta for a selling node; a bar's net in the range, never below 0), split into an earlier and a later half
// (the middle bar of an odd count left out): fade = later / earlier. AE when earlier > 0 and fade < fadeX. Only bars in the hour
// before d (time > since) count, so the same bar decides the same way live and after a restart.
// false = not decidable at d (d not on hand / open / did not trade in the node / fewer than minBars bars).
struct AeResult { int bars = 0; double early = 0, late = 0, fade = -1; bool found = false; };
inline bool attackerFade(const std::vector<Bar>& bars, int upTo, std::int64_t since, Tick low, Tick high, bool sellNode, double fadeX, int minBars, AeResult& out) {
    out = AeResult();
    bool lastTraded = false, have = false;
    std::vector<double> att;
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const Bar& b = bars[i];
        if (b.index > upTo) break;
        if (b.time <= since) continue;                                                // only the last hour before the decision bar
        bool traded = false; double d = 0;
        for (std::size_t j = 0; j < b.rows.size(); ++j)
            if (b.rows[j].key >= low && b.rows[j].key <= high && b.rows[j].volume > 0) { traded = true; d += b.rows[j].delta; }
        if (b.index == upTo) { if (!b.closed) return false; have = true; lastTraded = traded; }
        if (traded) att.push_back(std::max(0.0, sellNode ? -d : d));
    }
    if (!have || !lastTraded) return false;
    out.bars = static_cast<int>(att.size());
    if (out.bars < std::max(2, minBars)) return false;
    const std::size_t half = att.size() / 2;
    for (std::size_t i = 0; i < half; ++i) { out.early += att[i]; out.late += att[att.size() - 1 - i]; }
    out.fade = out.early > 0 ? out.late / out.early : -1;
    out.found = out.early > 0 && out.fade < fadeX;
    return true;
}
} // namespace exh
} // namespace delta_profile
#endif
