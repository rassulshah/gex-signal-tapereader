// AccumLogic.h (1.0.0, 2026-11-16) - accumulation / distribution ZONES for lsRevRead 0.9.0 (the pane finds them, lsRevReadMarks draws the boxes).
// Rassul 2026-11-16 10:53-10:58 CT: "i like the way you are analyzing accumulation", "can you create an accumulation indicator like you did with
// the boxes", "would you highlight the accumualation boxes like you did with the green and amber boxes", "build now create read on the right side".
// SDK-free. UNPROVEN: every zone is a "?" until the nightly has scored it (status RULES line says so).
//
// (2.0.0: the rule below is RETIRED - see "THE SIMPLE RULE" in the Engine.)  1.x: ACCUMULATION (at a low; DISTRIBUTION = the mirror at a high). Over the last 5..14 closed candles:
//   1. the sellers were the net aggressors: sell - buy >= NET_SHARE (2%) of the lots traded in those candles
//   2. price made no progress: the lowest low is at most PROG_X (0.25) x the candles' mean range below the first candle's low,
//      and it is the lowest low of the last LOW_LOOK (20) candles (the selling is AT a low)
//   3. a passive buyer was seen: lsRevRead's "Absorbed" (more traded at a price than was ever shown there, ratio >= ABS_X 1.2) inside
//      those candles, at or just above that low
//   TESTED 2026-11-16 (accstudy, order-by-order store): 1.0.0's rule held no better than any low under net selling (55% vs 59%). What DID separate,
//   on both halves and in all 4 markets: a tight hold (67%), a QUIET END - the defended prices no longer being hit in the last third (67% vs 42%) -
//   and offers above NOT building (68% vs 51%); together 76% (n 456). None of them yet finds the BIG turns (2 ranges before 1: 29-31%).
//   The quiet end and the offers need the order book inside the indicator: not built yet.
//   -> a zone OPENS (amber): from the first candle to now, low .. high of those candles, the absorbed price, the numbers of the read.
// While open it grows with every candle. It is DECIDED once, at a candle's close, and never changes afterwards:
//   CONFIRMED (green / red)  a candle with delta on the zone's side (buyers for accumulation) closes beyond the highest high (lowest low)
//                            of the 3 candles before it
//   BROKEN                   a candle closes through the absorbed price (below it for accumulation)        -> not drawn any more
//   EXPIRED                  MAX_OPEN (30) candles without a decision                                       -> not drawn any more
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <string>
#include <vector>

namespace acc {
static const char* const VERSION = "2.0.0";   // (2.0.0, 2026-11-16 Rassul 14:32-14:43 CT "are you overcomplicating it .. you are suppose to look for accumulation / distibution", "the volume at the pivot (or 1 bar before) is a clue", "high volume on the pivot or right before it should be considered as part of the accum or dsit process", "go"): THE SIMPLE RULE replaces 1.x - climax volume at the pivot or the candle before (1.4x: ES 1:57 PM, the second case he circled, measured 1.43x), a push, 3+ candles stalling at the extreme, delta turned against the push, with the 60-candle trend; confirmed on a close beyond the stall. No order-book condition. Tested the same day on 80 sessions: with the trend 37% follow-through (n 217; ES 56%, n 39), the climax lifts HOLDING (60% -> 70% at 3x), not the move after: UNPROVEN, "?"   // was "1.2.1"
static const int LOOKP = 8, STALL_MIN = 3, TREND_BARS = 60, MAX_OPEN = 10, KEEP_BARS = 400, KEEP_ZONES = 300;
static const double CLIMAX_X = 1.4, PUSH_X = 1.5, STALL_X = 0.5, TREND_X = 2.0;   // (2.0.0)
static const double NET_SHARE = 0.02, PROG_X = 0.25, ABS_X = 1.2, OFF_MAX = 0.30;
static const int BK_SEC = 30, BK_KEEP = 300;                    // the book cells: 30 s each, 150 min kept (a zone spans at most 14 + 30 candles)
enum State { OPEN = 0, CONFIRMED = 1, BROKEN = 2, EXPIRED = 3 };
struct Bar { long long t = 0, lo = 0, hi = 0, close = 0, buy = 0, sell = 0; };          // t = the candle's END (seconds); prices in ticks
struct Abs { long long t = 0; int side = 0; long long px = 0, shown = 0, lots = 0; double x = 0; };   // side +1 = a BUYER absorbed sellers (at a low)
struct Zone {
    int id = 0, side = 0, state = OPEN;                  // side +1 accumulation, -1 distribution
    long long t0 = 0, t1 = 0, tDec = 0;                  // first candle's end, last candle's end, the deciding candle's end
    long long lo = 0, hi = 0, absPx = 0, confPx = 0;     // the box, the absorbed price, the price a close beyond confirms (as of the last candle)
    long long net = 0, total = 0, drift = 0;             // the aggressors' net lots / all lots since t0; ticks the extreme moved since the first candle
    long long absShown = 0, absLots = 0; double absX = 0; int bars = 0;
    long long band = 0, lateFill = 0, offNet = 0; int book = 0;   // (1.2.0) half the mean range (ticks); defender fills near the extreme in the last third; the other side's adds - pulls against the zone; book 1 = the order book covered the whole zone
    long long first = 0;                                 // the first candle's low (high): where "no progress" is measured from
};
inline bool quiet(const Zone& z) { return z.book == 1 && z.lateFill == 0; }
inline bool thin(const Zone& z) { return z.book == 1 && (double)z.offNet <= OFF_MAX * (double)z.total; }
inline int grade(const Zone& z) { return (quiet(z) ? 1 : 0) + (thin(z) ? 1 : 0); }
inline bool drawn(const Zone& z) { return z.state == OPEN || z.state == CONFIRMED; }

struct Engine {
    int spb = 180; std::deque<Bar> B; std::vector<Abs> A; std::vector<Zone> Z; int nextId = 1; long long lastT = 0;
    struct Cell { long long fill = 0, add = 0, pull = 0; };
    struct Bk { long long b = 0; std::map<long long, Cell> bid, ask; };
    std::deque<Bk> K;                                    // (1.2.0) the live book per 30 s and price: resting lots filled, added, pulled
    void reset() { B.clear(); A.clear(); Z.clear(); K.clear(); nextId = 1; lastT = 0; }
    // an order event: kind 0 = a resting order ADDED, 1 = PULLED, 2 = FILLED; bid = the resting order's side
    void onBook(long long tSec, int kind, bool bid, long long px, long long lots) {
        if (tSec <= 0 || px <= 0 || lots <= 0 || kind < 0 || kind > 2) return;
        const long long b = tSec / BK_SEC;
        if (K.empty() || b > K.back().b) { K.push_back(Bk()); K.back().b = b; while ((int)K.size() > BK_KEEP) K.pop_front(); }
        else if (b < K.back().b) return;                                             // older than the open cell: not counted
        Cell& c = (bid ? K.back().bid : K.back().ask)[px];
        if (kind == 0) c.add += lots; else if (kind == 1) c.pull += lots; else c.fill += lots;
    }
    // the two readings over the zone's window (tFrom = the first candle's START, tTo = the last candle's end)
    void measure(Zone& z, long long tFrom, long long tTo) const {
        z.book = 0; z.lateFill = 0; z.offNet = 0;
        if (K.empty() || (K.front().b + 1) * BK_SEC > tFrom + BK_SEC || tTo <= tFrom) return;   // the book does not reach back to the zone's start (a restart)
        const long long edge = z.side > 0 ? z.lo : z.hi, band = std::max(2LL, z.band), tl = tTo - (tTo - (tFrom + spb)) / 3;
        for (const Bk& k : K) { const long long te = (k.b + 1) * BK_SEC; if (te <= tFrom || te > tTo + BK_SEC - 1) continue;
            const std::map<long long, Cell>& D = z.side > 0 ? k.bid : k.ask; const std::map<long long, Cell>& O = z.side > 0 ? k.ask : k.bid;
            if (te > tl) { const long long a = z.side > 0 ? edge - 2 : edge - band, b = z.side > 0 ? edge + band : edge + 2;
                for (std::map<long long, Cell>::const_iterator it = D.lower_bound(a); it != D.end() && it->first <= b; ++it) z.lateFill += it->second.fill; }
            const long long a = z.side > 0 ? edge + 1 : edge - 3 * band, b = z.side > 0 ? edge + 3 * band : edge - 1;
            for (std::map<long long, Cell>::const_iterator it = O.lower_bound(a); it != O.end() && it->first <= b; ++it) z.offNet += it->second.add - it->second.pull; }
        z.book = 1;
    }
    // a restart: the zones as the file has them (decided ones never change), detection goes on after `last`; candles up to `last` only refill the history
    long long resume = 0;
    // (1.2.1) the same zone twice (same side, same first candle): only the first is kept - a restart that replayed candles it had already decided wrote every zone again (ES 2026-11-16: 12 lines for 6 zones)
    bool has(int side, long long t0) const { for (const Zone& z : Z) if (z.side == side && z.t0 == t0) return true; return false; }
    void load(const std::vector<Zone>& zones, long long last) { Z.clear(); for (const Zone& z : zones) if (!has(z.side, z.t0)) Z.push_back(z); nextId = 1; for (const Zone& z : Z) nextId = std::max(nextId, z.id + 1); resume = last; }
    void prime(const Bar& b) { if (b.t <= lastT || b.hi < b.lo || b.lo <= 0) return; lastT = b.t; B.push_back(b); while ((int)B.size() > KEEP_BARS) B.pop_front(); }
    int openOf(int side) const { for (int i = (int)Z.size() - 1; i >= 0; --i) if (Z[(size_t)i].side == side && Z[(size_t)i].state == OPEN) return i; return -1; }
    // lsRevRead's "Absorbed": bottom = the buyers absorbed (a low)
    void onAbsorb(long long tSec, bool bottom, long long px, long long shown, long long lots, double x) {
        if (!(x >= ABS_X) || px <= 0 || tSec <= 0 || !std::isfinite(x)) return;
        Abs a; a.t = tSec; a.side = bottom ? 1 : -1; a.px = px; a.shown = shown; a.lots = lots; a.x = x; A.push_back(a);
        if (A.size() > 200) A.erase(A.begin());
        const int k = openOf(a.side); if (k >= 0) { Zone& z = Z[(size_t)k]; if (px >= z.lo - 2 && px <= z.hi + 2 && x > z.absX && tSec > z.t0 - spb) { z.absX = x; z.absShown = shown; z.absLots = lots; } }   // a stronger read of the same zone; its price stays
    }
    // a CLOSED candle, in time order, each once
    void onCandle(const Bar& b) {
        if (b.t <= lastT || b.hi < b.lo || b.lo <= 0) return;
        lastT = b.t; B.push_back(b); while ((int)B.size() > KEEP_BARS) B.pop_front();
        const int n = (int)B.size();
        for (int side = 1; side >= -1; side -= 2) {
            const int k = openOf(side);
            if (k >= 0) { step(Z[(size_t)k], n); continue; }
            tryOpen(side, n);
        }
        while ((int)Z.size() > KEEP_ZONES) Z.erase(Z.begin());
    }
    // ---- helpers (side-mirrored: for a top every "low" is a high)
    static long long ext(const Bar& b, int side) { return side > 0 ? b.lo : b.hi; }
    static long long far(const Bar& b, int side) { return side > 0 ? b.hi : b.lo; }
    // ---- (2.0.0) THE SIMPLE RULE, written for DISTRIBUTION at a high (accumulation = the mirror at a low):
    //   1. CLIMAX   the pivot candle (the highest high of the last LOOKP candles) or the candle before it has CLIMAX_X x the average volume of the 20 before
    //   2. PUSH     that high is PUSH_X mean ranges or more above the low of the 6 candles into it
    //   3. STALL    STALL_MIN or more candles (from the one before the pivot on) have their high within STALL_X mean ranges of it, 2+ candles after the pivot, no close above it
    //   4. DELTA    the aggressors since the pivot are net AGAINST the push (NET_SHARE of the lots)
    //   5. TREND    the turn is WITH the trend of the last TREND_BARS candles (TREND_X mean ranges)
    //   -> the zone OPENS (amber). CONFIRMED when a candle closes beyond the stall (under its lowest low); BROKEN on a close above the high; EXPIRED after MAX_OPEN.
    double meanRange(int n) const { double r = 0; int k = 0; for (int q = std::max(0, n - 21); q < n - 1; ++q) { r += (double)(B[(size_t)q].hi - B[(size_t)q].lo); ++k; } return k ? r / k : 0; }
    void step(Zone& z, int n) {
        const Bar& b = B[(size_t)n - 1]; const int s = z.side;
        z.t1 = b.t; ++z.bars; z.net += s > 0 ? b.sell - b.buy : b.buy - b.sell; z.total += b.buy + b.sell;
        const bool through = s > 0 ? b.close < z.absPx : b.close > z.absPx;                 // absPx = the stall's extreme (the invalidation line)
        const bool out = s > 0 ? b.close > z.confPx : b.close < z.confPx;                   // confPx = the stall's far side as of the candle before
        if (through) { z.state = BROKEN; z.tDec = b.t; }
        else if (out) { z.state = CONFIRMED; z.tDec = b.t; }
        else if (z.bars >= MAX_OPEN) { z.state = EXPIRED; z.tDec = b.t; }
        if (z.state == OPEN) { z.lo = std::min(z.lo, b.lo); z.hi = std::max(z.hi, b.hi); z.confPx = s > 0 ? z.hi : z.lo; ++z.drift; }   // drift = the stall's candles
        else if (z.state == CONFIRMED) { z.lo = std::min(z.lo, b.lo); z.hi = std::max(z.hi, b.hi); }
    }
    void tryOpen(int side, int n) {
        if (n < TREND_BARS + LOOKP + 2) return;
        const int e = n - 1; const double rng = meanRange(n); if (!(rng > 0)) return;
        int ih = e - LOOKP + 1; for (int q = ih + 1; q <= e; ++q) if (side > 0 ? B[(size_t)q].lo < B[(size_t)ih].lo : B[(size_t)q].hi > B[(size_t)ih].hi) ih = q;   // the pivot: the first candle at the extreme
        const long long H = ext(B[(size_t)ih], side);
        int is = ih; for (int q = e - LOOKP + 1; q < ih; ++q) if ((double)(side > 0 ? B[(size_t)q].lo - H : H - B[(size_t)q].hi) <= STALL_X * rng) { is = q; break; }   // the stall's first candle: the first one at the extreme (the pivot itself, or an earlier candle within STALL_X of it)
        if (e - is < 2 || is < 22) return;
        // 5. with the trend
        const double tr = (double)(B[(size_t)e - 8].close - B[(size_t)e - TREND_BARS].close) / rng; if (side > 0 ? tr < TREND_X : tr > -TREND_X) return;
        // 1. the climax: the biggest candle from the one before the stall to the pivot, against the 20 candles before those
        double vm = 0; for (int q = is - 21; q < is - 1; ++q) vm += (double)(B[(size_t)q].buy + B[(size_t)q].sell); vm /= 20.0; if (!(vm > 0)) return;
        long long vc = 0; int ic = is - 1; for (int q = is - 1; q <= ih; ++q) { const long long v = B[(size_t)q].buy + B[(size_t)q].sell; if (v > vc) { vc = v; ic = q; } }
        const int ds = std::max(is, std::min(ic + 1, e - 1));   // the delta, the box and the far side are read AFTER the climax candle (its own delta is the push)
        const double cx = (double)vc / vm; if (cx < CLIMAX_X) return;
        // 2. the push
        long long base = far(B[(size_t)is], side); for (int q = is - 5; q <= is; ++q) base = side > 0 ? std::max(base, B[(size_t)q].hi) : std::min(base, B[(size_t)q].lo);
        if ((double)(side > 0 ? base - H : H - base) < PUSH_X * rng) return;
        // 3. the stall
        int st = 0; long long lo = B[(size_t)ds].lo, hi = B[(size_t)ds].hi, net = 0, tot = 0;
        for (int q = is; q <= e; ++q) { const Bar& b = B[(size_t)q]; if ((double)(side > 0 ? b.lo - H : H - b.hi) <= STALL_X * rng) ++st;
            if (q < ds) continue;
            lo = std::min(lo, b.lo); hi = std::max(hi, b.hi); net += side > 0 ? b.sell - b.buy : b.buy - b.sell; tot += b.buy + b.sell; }
        if (side > 0) lo = std::min(lo, H); else hi = std::max(hi, H);                       // the box always reaches the extreme
        if (st < STALL_MIN || tot <= 0) return;
        // 4. the delta has turned against the push: for a top the SELLERS are net (net here = the pushing side's net, so it must be negative)
        if ((double)net > -NET_SHARE * (double)tot) return;
        // still stalling: this candle has not closed beyond the candles before it
        long long farSide = far(B[(size_t)ds], side); for (int q = ds; q < e; ++q) farSide = side > 0 ? std::max(farSide, B[(size_t)q].hi) : std::min(farSide, B[(size_t)q].lo);
        const Bar& last = B[(size_t)e]; if (side > 0 ? last.close > farSide : last.close < farSide) return;
        Zone z; z.id = nextId++; z.side = side; z.t0 = B[(size_t)ds].t; z.t1 = last.t; z.lo = lo; z.hi = hi; z.absPx = H; z.absShown = 0; z.absLots = vc; z.absX = cx;
        z.net = net; z.total = tot; z.drift = e - is + 1; z.first = H; z.bars = 0; z.band = std::max(2LL, (long long)std::llround(rng * 0.5)); z.confPx = side > 0 ? hi : lo;
        if (has(z.side, z.t0)) { --nextId; return; }   /* (1.2.1) already decided once: never a second copy */
        Z.push_back(z);
    }
    // the zone the read is about now: an open one (the newer), else one decided CONFIRMED within the last `keep` candles
    const Zone* current(int keep = 10) const {
        const Zone* o = nullptr; for (const Zone& z : Z) if (z.state == OPEN && (!o || z.t0 > o->t0)) o = &z;
        if (o) return o;
        for (int i = (int)Z.size() - 1; i >= 0; --i) { const Zone& z = Z[(size_t)i]; if (z.state == CONFIRMED && lastT - z.tDec <= (long long)keep * spb) return &z; }
        return nullptr;
    }
    // (1.2.0) the newest zone of any state: what the read shows when nothing is open
    const Zone* last() const { const Zone* o = nullptr; for (const Zone& z : Z) if (!o || z.t0 >= o->t0) o = &z; return o; }
};

// ---- the read (right of the pane), the record lines and the boxes
struct Line { std::string text; int col = 0; };            // col: +1 green, -1 red, 2 white, 3 amber
inline std::string px(long long ticks, double tick) {
    const double v = (double)ticks * tick; int d = 0; double t = tick; while (d < 6 && std::fabs(t - std::round(t)) > 1e-7) { t *= 10; ++d; }
    char b[40]; std::snprintf(b, sizeof b, "%.*f", d, v); return b;
}
inline std::string commas(long long v) { const bool neg = v < 0; std::string d = std::to_string(neg ? -v : v), o; for (size_t k = 0; k < d.size(); ++k) { if (k && (d.size() - k) % 3 == 0) o += ','; o += d[k]; } return (neg ? "-" : "+") + o; }
inline std::vector<Line> readLines(const Zone& z, double tick, int spb, const std::string& decidedClock) {
    std::vector<Line> L; const bool up = z.side > 0, open = z.state == OPEN; const int zc = up ? 1 : -1; char b[96]; (void)spb;
    L.push_back(Line{ std::string(up ? "Accum" : "Distrib") + (open ? "? " : " ") + px(z.absPx, tick), open ? 3 : z.state == CONFIRMED ? zc : z.state == BROKEN ? -zc : 2 });
    std::snprintf(b, sizeof b, "1. Climax %.1fx (%.1fx)", z.absX, CLIMAX_X); L.push_back(Line{ b, 2 });
    std::snprintf(b, sizeof b, "2. Stall %lld bars", z.drift); L.push_back(Line{ b, 2 });
    L.push_back(Line{ "3. Delta " + commas(up ? -z.net : z.net), zc });          // net = the pushing side's net (negative): shown as the candles' delta (buy - sell)
    if (open) L.push_back(Line{ std::string(up ? "Above " : "Below ") + px(z.confPx, tick) + " confirms", 2 });
    else if (z.state == CONFIRMED) L.push_back(Line{ "Confirmed " + decidedClock, zc });
    else if (z.state == BROKEN) L.push_back(Line{ "Broken " + decidedClock, -zc });
    else L.push_back(Line{ "Expired " + decidedClock, 2 });
    return L;
}
// (1.2.0) the same read in 3 lines, for a pane too low for the full one
inline std::vector<Line> readCompact(const Zone& z, double tick, int spb, const std::string& decidedClock) {
    const std::vector<Line> F = readLines(z, tick, spb, decidedClock); std::vector<Line> L; if (F.empty()) return L;
    const bool up = z.side > 0; char b[96]; L.push_back(F.front());
    std::snprintf(b, sizeof b, "Climax %.1fx  Stall %lld  Delta %s", z.absX, z.drift, commas(up ? -z.net : z.net).c_str());
    L.push_back(Line{ b, 2 }); L.push_back(F.back()); return L;
}
// one line per zone: Z|ver|id|side|state|t0|t1|tDec|lo|hi|absPx|confPx|net|total|drift|absShown|absLots|absX|bars|first|band|lateFill|offNet|book   (1.2.0: the last four; a 1.1.0 line of 20 fields still reads)
inline std::string zLine(const Zone& z) {
    char b[320]; std::snprintf(b, sizeof b, "Z|%s|%d|%d|%d|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%.4f|%d|%lld|%lld|%lld|%lld|%d", VERSION, z.id, z.side, z.state, z.t0, z.t1, z.tDec, z.lo, z.hi, z.absPx, z.confPx,
        z.net, z.total, z.drift, z.absShown, z.absLots, z.absX, z.bars, z.first, z.band, z.lateFill, z.offNet, z.book); return b;
}
inline bool parseZ(const std::string& l, Zone& z) {
    if (l.compare(0, 2, "Z|") != 0) return false;
    std::vector<std::string> c; size_t a = 0; while (a <= l.size()) { size_t q = l.find('|', a); if (q == std::string::npos) q = l.size(); c.push_back(l.substr(a, q - a)); a = q + 1; }
    if (c.size() != 20 && c.size() != 24) return false;
    auto num = [&](size_t i, long long& v) { if (c[i].empty()) return false; char* e = nullptr; v = std::strtoll(c[i].c_str(), &e, 10); return e && *e == 0; };
    long long v[20] = { 0 }; for (size_t i = 2; i < 20; ++i) { if (i == 17) continue; if (!num(i, v[i])) return false; }
    char* e = nullptr; const double x = std::strtod(c[17].c_str(), &e); if (!e || *e != 0 || !std::isfinite(x) || x < 0) return false;
    if ((v[3] != 1 && v[3] != -1) || v[4] < OPEN || v[4] > EXPIRED || v[5] <= 0 || v[6] < v[5] || v[8] <= 0 || v[9] < v[8] || v[10] <= 0) return false;
    z.id = (int)v[2]; z.side = (int)v[3]; z.state = (int)v[4]; z.t0 = v[5]; z.t1 = v[6]; z.tDec = v[7]; z.lo = v[8]; z.hi = v[9]; z.absPx = v[10]; z.confPx = v[11]; z.net = v[12]; z.total = v[13]; z.drift = v[14];
    z.absShown = v[15]; z.absLots = v[16]; z.absX = x; z.bars = (int)v[18]; z.first = v[19];
    if (c.size() == 24) { long long w[4] = { 0 }; for (size_t i = 0; i < 4; ++i) if (!num(20 + i, w[i])) return false; if (w[0] < 0 || w[1] < 0 || w[3] < 0 || w[3] > 1) return false; z.band = w[0]; z.lateFill = w[1]; z.offNet = w[2]; z.book = (int)w[3]; }
    return true;
}
inline std::string fileText(const Engine& E, const std::string& mkt, double tick) {
    char h[200]; std::snprintf(h, sizeof h, "ACCUM|%s|%s|%d|tick %.10g|last %lld|zones %d|UNPROVEN - every zone is a ? until the nightly has scored it\n", VERSION, mkt.c_str(), E.spb, tick, E.lastT, (int)E.Z.size());
    std::string o = h; for (const Zone& z : E.Z) o += zLine(z) + "\n"; return o;
}
inline std::vector<Zone> parseFile(const std::string& text) {
    std::vector<Zone> v; size_t a = 0;
    while (a < text.size()) { size_t b = text.find('\n', a); if (b == std::string::npos) break;   /* a line without its newline is torn: not read */
        std::string l = text.substr(a, b - a); a = b + 1; if (!l.empty() && l.back() == '\r') l.pop_back(); Zone z; if (parseZ(l, z)) v.push_back(z); }
    return v;
}
// the header's "last <t>" (0 = none): the newest candle the zones were decided up to
inline long long parseLast(const std::string& text) {
    if (text.compare(0, 6, "ACCUM|") != 0) return 0;
    const size_t e = text.find('\n'), p = text.find("|last ");
    if (e == std::string::npos || p == std::string::npos || p > e) return 0;
    char* q = nullptr; const long long v = std::strtoll(text.c_str() + p + 6, &q, 10);
    return q && *q == '|' && v > 0 ? v : 0;
}
// lsRevRead's record: "S|ver|id|bottom or top|n|A|<t ms>|...|anchor <price>|shown <n>|abs <n>|absx <x>|..." = a story reached Absorbed
inline bool parseAbsLine(const std::string& l, double tick, Abs& a) {
    if (l.compare(0, 2, "S|") != 0 || !(tick > 0)) return false;
    std::vector<std::string> c; size_t i0 = 0; while (i0 <= l.size()) { size_t q = l.find('|', i0); if (q == std::string::npos) q = l.size(); c.push_back(l.substr(i0, q - i0)); i0 = q + 1; }
    if (c.size() < 12 || c[5] != "A" || (c[3] != "bottom" && c[3] != "top")) return false;
    char* e = nullptr; const long long t = std::strtoll(c[6].c_str(), &e, 10); if (!e || *e != 0 || t <= 0) return false;
    double an = 0, ax = 0; long long sh = -1, ab = -1;
    for (const std::string& q : c) { if (q.compare(0, 7, "anchor ") == 0) an = std::atof(q.c_str() + 7); else if (q.compare(0, 6, "shown ") == 0) sh = std::atoll(q.c_str() + 6); else if (q.compare(0, 4, "abs ") == 0) ab = std::atoll(q.c_str() + 4); else if (q.compare(0, 5, "absx ") == 0) ax = std::atof(q.c_str() + 5); }
    if (!(an > 0) || sh < 0 || ab < 0 || !std::isfinite(ax)) return false;
    a.t = t; a.side = c[3] == "bottom" ? 1 : -1; a.px = (long long)std::llround(an / tick); a.shown = sh; a.lots = ab; a.x = ax; return true;
}
// the boxes on the price chart: an OUTLINE only (IRT has no see-through fill; a filled box would cover the candles) + one line at the absorbed price
static const unsigned C_AMBER = 0x00F5B301, C_UP = 0x0022C55E, C_DN = 0x00EF4444;                  // 0x00RRGGBB
struct Box { int x0 = 0, y0 = 0, x1 = 0, y1 = 0, yAbs = 0; unsigned col = 0; bool open = false; int id = 0; };
template <class XofT, class YofTick> inline std::vector<Box> boxes(const std::vector<Zone>& Z, long long tFirst, long long tLast, int spb, XofT X, YofTick Y, int half) {
    std::vector<Box> out;
    for (const Zone& z : Z) { if (!drawn(z)) continue; const long long te = z.state == OPEN ? z.t1 : z.tDec; if (te < tFirst || z.t0 > tLast) continue;
        Box b; b.id = z.id; b.open = z.state == OPEN; b.col = b.open ? C_AMBER : z.side > 0 ? C_UP : C_DN;
        b.x0 = X(std::max(z.t0, tFirst)) - half; b.x1 = X(std::min(te, tLast)) + half; b.y0 = Y(z.hi); b.y1 = Y(z.lo); b.yAbs = Y(z.absPx);
        if (b.y0 > b.y1) std::swap(b.y0, b.y1);
        if (b.x1 > b.x0) out.push_back(b); }
    (void)spb; return out;
}
} // namespace acc
