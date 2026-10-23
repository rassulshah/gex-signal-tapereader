#ifndef LS_TURNORDERLOGIC_H
#define LS_TURNORDERLOGIC_H
// TurnOrderLogic.h 1.0.0 (2026-10-22) -- the TURN ORDER mark of lsTurnOrder 1.0.0. SDK-free; tested by tests_tor/test_tor.cpp (logic),
// tests_tor/test_tor_host.cpp (mock host) and replayed on real MBO (tests_tor/replay_tor.cpp: NQ 2026-10-09, ES 2026-10-06..09).
//
// MBO Roadmap "Did the limit order at the turn fill, or was it too thick to get through?" (ticket T-20261022-25). Rassul 2026-10-22
// 15:45 CT: "I'll go with your recommendation". THE DESIGN:
//   - At each local PIVOT (a 5-candle extreme: the candle's low below the 2 candles before it and not above the 2 after it; mirrored for
//     a high), find THE resting limit order that stopped the move: the LARGEST real resting order - one MBO order id, rested >= 30 s at
//     its price when the extreme printed, at least the market's Big Lot S20 (or the Min size setting) - AT or just BEYOND the extreme
//     (a bid at the low or up to max(2 ticks, 0.1 ATR) under it; an offer at / above the high), alive when the extreme printed.
//   - DRAWN ONLY AFTER THE TURN IS CONFIRMED: a later closed candle closes >= 0.5 ATR (ATR(14) of the chart's candles; setting) away
//     from the order's price, before any candle trades through the extreme and within 20 candles. Iteration 1 found big resting size
//     that price still sits on is often a TRAP, so the mark always means "this order made the pivot", never "this might hold".
//   - FILLED (absorbed) vs TOO THICK (barely touched) goes in the HOVER only: FILLED = its own fills from the pivot candle's start to the
//     confirmation >= 30% of what it showed in the minute before the extreme, else TOO THICK.
//   - At most ONE per pivot. Decided once at a candle close, recorded append-only (M / N / P lines), never repainted.
// No look-ahead: a pivot is only seen 2 candles after its extreme (the 5-candle rule), the order must have rested 30 s BEFORE the
// extreme printed, and the book is only trusted 30 s after the order feed started (orders first seen by a MOD count from then).
// A known feed discontinuity invalidates the live book and selected-but-unconfirmed pivots: missing MBO fills/pulls must never become
// a FILLED or TOO THICK assertion. The next mark waits for a new 30-second observed feed interval.
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace tor {
static const char* const VERSION = "1.0.0";
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Ink { INK_WHITE = 0, INK_GREEN = 1, INK_RED = 2, INK_GREY = 3 };
static const int PIVOT_N = 2;               // candles each side: a 5-candle extreme
static const int ATR_N = 14;
static const int MAX_WAIT = 20;             // candles after the extreme to confirm, else dropped (N line)
static const double FILL_SHARE = 0.30;      // FILLED when its fills >= 30% of what it showed
static const long long SHOWN_LOOKBACK_MS = 60000;
static const size_t HIST_CAP = 256;         // size changes kept per big order

inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }
inline bool numLL(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || !e || *e || v < lo || v > hi) return false;
    out = v; return true;
}
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; std::string c;
    for (char ch : s) { if (ch == d) { o.push_back(c); c.clear(); } else if (ch != '\r' && ch != '\n') c.push_back(ch); }
    o.push_back(c); return o;
}

struct Params {
    double gateAtr = 0.5;      // the confirmation: a close this many ATR away from the order
    long long minSize = 20;    // lots (the host passes the Min size setting, or the market's Big Lot S20 when it is 0)
    int restS = 30;            // seconds rested at its price before the extreme
    int beyondTicks = 2;       // "just beyond": at least this many ticks (or 0.1 ATR, the bigger)
    double beyondAtr = 0.1;
};

struct Bar { long long endMs = 0, startMs = 0; Tick o = 0, h = 0, l = 0, c = 0; };

// one order at one price (a re-price starts a new segment). Only orders that reached minSize keep a size history.
struct Seg {
    long long oid = 0; int side = BID; Tick px = 0;
    long long tAt = 0, tEnd = 0;           // arrived at this price / left it (0 = still resting)
    bool startKnown = true; int how = 0;    // how it ended: 0 live, 'F' filled out, 'P' pulled, 'M' moved
    long long size = 0, traded = 0;
    std::vector<std::pair<long long, long long> > hist;   // (t, size) changes once it was big
    std::vector<std::pair<long long, long long> > fills;  // (t, lots)
    long long sizeAt(long long t) const {  // its size at time t (-1 = not big then / unknown)
        long long v = -1; for (size_t i = 0; i < hist.size(); ++i) { if (hist[i].first > t) break; v = hist[i].second; } return v;
    }
    long long maxIn(long long t0, long long t1) const {   // the most it showed in [t0, t1]
        long long v = sizeAt(t0); for (size_t i = 0; i < hist.size(); ++i) if (hist[i].first > t0 && hist[i].first <= t1) v = std::max(v, hist[i].second); return v;
    }
    long long filledIn(long long t0, long long t1) const { long long s = 0; for (size_t i = 0; i < fills.size(); ++i) if (fills[i].first >= t0 && fills[i].first <= t1) s += fills[i].second; return s; }
};

// a decided mark (the M line) and what happened to its order later (P lines)
struct Mark {
    int side = BID;                 // BID = a bid that stopped a selloff (pivot LOW), ASK = an offer that stopped a rally (pivot HIGH)
    long long pivotEnd = 0;         // the extreme candle's END (CT ms) = the chart key
    long long tExt = 0;             // when the extreme first printed
    Tick ext = 0, px = 0; long long oid = 0;
    long long shown = 0, filled = 0, restS = 0; bool startKnown = true;
    long long atr10 = 0;            // ATR in tenths of a tick at the pivot
    long long run100 = 0;           // how far price had left the order at the confirmation, ATR x100
    long long decideMs = 0;         // the confirming candle's end
    bool absorbed = false;
    std::string ver = VERSION;
    // later (P lines / live): 'P' pulled, 'M' moved, 'F' filled out, 0 = still resting / unknown
    int later = 0; long long laterMs = 0, laterTraded = 0;
    long long maxRun100 = 0;        // the furthest close away from the order since (ATR x100), from the chart's candles
};
struct NoMark { int side = BID; long long pivotEnd = 0; Tick ext = 0; std::string why; long long decideMs = 0; };

struct Tally { long long pivots = 0, marks = 0, absorbed = 0, thick = 0, noOrder = 0, noConfirm = 0, broken = 0, noFeed = 0, feedGaps = 0, events = 0, unknownFill = 0, pruned = 0; };

inline std::string keyOf(int side, long long pivotEnd) { return std::to_string(side) + ":" + std::to_string(pivotEnd); }

class Engine {
public:
    Params P;
    std::vector<Bar> bars;                                   // closed candles, in order
    std::unordered_map<long long, Seg> live;                 // order id -> its current segment
    std::deque<Seg> gone;                                    // ended big segments (kept 3 h)
    std::deque<std::pair<long long, std::pair<Tick, Tick> > > secs;   // per second: (t, lowest / highest trade) from FILL rows
    std::vector<Mark> marks; std::vector<NoMark> nos;
    std::set<std::string> done;                              // pivot keys already decided (record or engine)
    std::vector<std::string> rows;                           // record lines not yet written
    long long feedStartMs = 0, lastT = 0, lastBarEnd = 0;
    Tally tally; size_t liveCap = 400000; double tick = 0.25;   // tick: the chart's tick size (record prices)
    struct Pend { int side; size_t idx; Tick ext; long long endMs, startMs; double atr; bool picked = false; long long oid = 0; Tick px = 0;
                  long long shown = 0, tExt = 0, restS = 0; bool startKnown = true; Seg seg; std::string why; };
    std::vector<Pend> pend;

    // ---- the order feed: one MBO event (CT ms). side is used only on NEW / an unknown MOD (a FILL row carries the aggressor's side)
    void onEvent(int act, int side, Tick px, long long size, long long oid, long long t) {
        if (t <= 0 || oid == 0) return;
        ++tally.events; if (feedStartMs == 0) feedStartMs = t; if (t > lastT) lastT = t;
        if (act == FILL && px > 0 && size > 0) trade(px, t);               // every print times the extremes (known order or not)
        auto it = live.find(oid);
        if (act == NEW || (act == MOD && it == live.end())) {
            if (px <= 0 || size <= 0 || (side != BID && side != ASK)) return;
            if (it != live.end()) { endSeg(it->second, 'M', t); live.erase(it); }
            if (live.size() >= liveCap) prune();
            Seg s; s.oid = oid; s.side = side; s.px = px; s.tAt = t; s.size = size; s.startKnown = act == NEW;
            if (size >= P.minSize) s.hist.push_back(std::make_pair(t, size));
            live[oid] = s; return;
        }
        if (it == live.end()) { if (act == FILL) ++tally.unknownFill; return; }
        Seg& s = it->second;
        if (act == MOD) {
            if (px <= 0) return;
            if (px != s.px) {                                   // re-priced: the old segment ends ('M'), a new one starts (rest counted again)
                Seg n; n.oid = oid; n.side = s.side; n.px = px; n.tAt = t; n.size = size > 0 ? size : 0; n.startKnown = s.startKnown;
                endSeg(s, 'M', t);
                if (n.size >= P.minSize) n.hist.push_back(std::make_pair(t, n.size));
                it->second = n; return;
            }
            setSize(s, size, t); return;
        }
        if (act == FILL) {
            if (size <= 0) return;
            if (px <= 0) trade(s.px, t);
            s.traded += size; if (!s.hist.empty() || s.size >= P.minSize) { if (s.fills.size() < HIST_CAP) s.fills.push_back(std::make_pair(t, size)); }
            setSize(s, std::max(0LL, s.size - size), t);
            return;
        }
        if (act == DEL) { endSeg(s, s.size <= 0 ? 'F' : 'P', t); live.erase(it); }
    }
    // a trade print without a resting order known (tests / replays that carry the tape separately)
    void trade(Tick px, long long t) {
        const long long sec = t / 1000 * 1000;
        if (!secs.empty() && secs.back().first == sec) { auto& v = secs.back().second; v.first = std::min(v.first, px); v.second = std::max(v.second, px); }
        else if (secs.empty() || sec > secs.back().first) secs.push_back(std::make_pair(sec, std::make_pair(px, px)));
        while (!secs.empty() && sec - secs.front().first > 4LL * 3600000LL) secs.pop_front();
    }
    static double atrAt(const std::vector<Bar>& b, size_t i) {
        if (i < (size_t)ATR_N || i >= b.size()) return 0;
        double s = 0;
        for (size_t k = i - ATR_N + 1; k <= i; ++k) { const Tick pc = b[k - 1].c; s += (double)(std::max(b[k].h, pc) - std::min(b[k].l, pc)); }
        return s / ATR_N;
    }
    // ---- a candle CLOSED (in order, once). New pivots are seen; pending ones are confirmed / dropped; marks get their later run.
    void closeBar(const Bar& b) {
        if (b.endMs <= lastBarEnd) return;
        lastBarEnd = b.endMs; bars.push_back(b);
        const size_t n = bars.size();
        // 1) pending pivots: broken / confirmed / too old
        for (size_t k = 0; k < pend.size();) {
            Pend& p = pend[k]; bool drop = false;
            const bool broke = p.side == BID ? b.l < p.ext : b.h > p.ext;
            if (broke) { ++tally.broken; drop = true; }
            else if (p.picked) {
                const double away = p.side == BID ? (double)(b.c - p.px) : (double)(p.px - b.c);
                if (p.atr > 0 && away >= P.gateAtr * p.atr) { decide(p, b, away / p.atr); drop = true; }
            }
            if (!drop && n - 1 - p.idx >= (size_t)MAX_WAIT) { if (p.picked) { noMark(p, "not confirmed in 20 candles", b.endMs); ++tally.noConfirm; } drop = true; }
            if (drop) pend.erase(pend.begin() + (long)k); else ++k;
        }
        // 2) a new pivot at n - 1 - PIVOT_N
        if (n >= (size_t)(2 * PIVOT_N + 1)) {
            const size_t i = n - 1 - PIVOT_N; const Bar& c = bars[i];
            bool lo = true, hi = true;
            for (int d = 1; d <= PIVOT_N; ++d) { if (!(c.l < bars[i - d].l) || c.l > bars[i + d].l) lo = false; if (!(c.h > bars[i - d].h) || c.h < bars[i + d].h) hi = false; }
            if (lo) newPivot(BID, i);
            if (hi) newPivot(ASK, i);
        }
        // 3) marks: the furthest close away since (hover only; recomputed from the candles after a restart)
        for (size_t k = 0; k < marks.size(); ++k) {
            Mark& m = marks[k]; if (b.endMs <= m.pivotEnd || m.atr10 <= 0) continue;
            const long long r = (long long)std::llround((m.side == BID ? (double)(b.c - m.px) : (double)(m.px - b.c)) * 1000.0 / (double)m.atr10);
            if (r > m.maxRun100) m.maxRun100 = r;
        }
        if (bars.size() > 4000) { bars.erase(bars.begin(), bars.begin() + 1000); shift(1000); }
        const long long cut = b.endMs - 3LL * 3600000LL;
        while (!gone.empty() && gone.front().tEnd < cut) gone.pop_front();
    }
    // the record's marks and decided keys (after a restart); a later P line updates the order's fate
    size_t restore(const std::vector<std::string>& in, long long* bad = nullptr) {
        size_t n = 0; long long nb = 0;
        for (size_t i = 0; i < in.size(); ++i) {
            if (in[i].empty() || in[i][0] == '#') continue;
            Mark m; NoMark z; int side = 0; long long pe = 0, t = 0, tr = 0; int how = 0;
            if (parseM(in[i], m)) { if (done.count(keyOf(m.side, m.pivotEnd))) continue; done.insert(keyOf(m.side, m.pivotEnd)); marks.push_back(m); ++n; }
            else if (parseN(in[i], z)) done.insert(keyOf(z.side, z.pivotEnd));
            else if (parseP(in[i], side, pe, how, t, tr)) { for (size_t k = 0; k < marks.size(); ++k) if (marks[k].side == side && marks[k].pivotEnd == pe && !marks[k].later) { marks[k].later = how; marks[k].laterMs = t; marks[k].laterTraded = tr; } }
            else ++nb;
        }
        // the later run from the candles already closed
        for (size_t k = 0; k < marks.size(); ++k) { Mark& m = marks[k]; if (m.atr10 <= 0) continue;
            for (size_t j = 0; j < bars.size(); ++j) if (bars[j].endMs > m.pivotEnd) { const long long r = (long long)std::llround((m.side == BID ? (double)(bars[j].c - m.px) : (double)(m.px - bars[j].c)) * 1000.0 / (double)m.atr10); if (r > m.maxRun100) m.maxRun100 = r; } }
        if (bad) *bad = nb;
        return n;
    }
    void clearSession() { marks.clear(); nos.clear(); done.clear(); pend.clear(); }
    // Conn explicitly reports only two recoverable discontinuities to this engine: its queue dropped events, or its watchdog
    // destroyed/reopened the DBO subscription. In either case the identity book and any selected result spanning the gap are partial.
    void onFeedGap() {
        for (size_t k = 0; k < pend.size(); ++k) { noMark(pend[k], "order feed gap before confirmation", lastBarEnd); ++tally.noFeed; }
        pend.clear(); live.clear(); gone.clear(); secs.clear(); feedStartMs = 0; lastT = 0; ++tally.feedGaps;
    }
    // ---- the record lines ----------------------------------------------------------------------------------------------------
    static const char* header() { return "# lsTurnOrder record: M|ver|side(B/A)|pivot_end_ms|t_ext_ms|ext_tick|oid|px_tick|price|shown|filled|rest_s|start_known|atr_x10|run_x100|decide_ms|result(FILLED/THICK)  /  N|ver|side|pivot_end_ms|ext_tick|why|decide_ms  /  P|side|pivot_end_ms|oid|t_ms|how(P pulled/M moved/F filled out)|traded"; }
    static std::string mLine(const Mark& m, double tick) {
        char b[420]; std::snprintf(b, sizeof(b), "M|%s|%c|%lld|%lld|%lld|%lld|%lld|%s|%lld|%lld|%lld|%d|%lld|%lld|%lld|%s", m.ver.c_str(), m.side == BID ? 'B' : 'A', m.pivotEnd, m.tExt,
            (long long)m.ext, m.oid, (long long)m.px, priceText(m.px, tick).c_str(), m.shown, m.filled, m.restS, m.startKnown ? 1 : 0, m.atr10, m.run100, m.decideMs, m.absorbed ? "FILLED" : "THICK");
        return b;
    }
    static bool parseM(const std::string& ln, Mark& m) {
        const std::vector<std::string> f = split(ln, '|');
        if (f.size() != 17 || f[0] != "M" || (f[2] != "B" && f[2] != "A") || (f[16] != "FILLED" && f[16] != "THICK")) return false;
        long long v[17] = { 0 }; static const int idx[] = { 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15 };
        for (int k : idx) if (!numLL(f[(size_t)k], k == 5 || k == 7 ? -4000000000000LL : 0, 9000000000000000000LL, v[k])) return false;
        if (v[3] <= 0 || v[9] <= 0 || v[10] < 0 || v[12] > 1 || v[6] == 0) return false;
        Mark x; x.ver = f[1]; x.side = f[2] == "B" ? BID : ASK; x.pivotEnd = v[3]; x.tExt = v[4]; x.ext = v[5]; x.oid = v[6]; x.px = v[7]; x.shown = v[9]; x.filled = v[10];
        x.restS = v[11]; x.startKnown = v[12] == 1; x.atr10 = v[13]; x.run100 = v[14]; x.decideMs = v[15]; x.absorbed = f[16] == "FILLED";
        if (x.side == BID ? x.px > x.ext : x.px < x.ext) return false;     // a bid is never above the low it stopped (offer: below the high)
        m = x; return true;
    }
    static std::string nLine(const NoMark& z) {
        char b[260]; std::snprintf(b, sizeof(b), "N|%s|%c|%lld|%lld|%s|%lld", VERSION, z.side == BID ? 'B' : 'A', z.pivotEnd, (long long)z.ext, z.why.c_str(), z.decideMs); return b;
    }
    static bool parseN(const std::string& ln, NoMark& z) {
        const std::vector<std::string> f = split(ln, '|'); long long pe = 0, ex = 0, dm = 0;
        if (f.size() != 7 || f[0] != "N" || (f[2] != "B" && f[2] != "A") || !numLL(f[3], 1, 9000000000000000000LL, pe) || !numLL(f[4], -4000000000000LL, 4000000000000LL, ex) || !numLL(f[6], 0, 9000000000000000000LL, dm)) return false;
        z.side = f[2] == "B" ? BID : ASK; z.pivotEnd = pe; z.ext = ex; z.why = f[5]; z.decideMs = dm; return true;
    }
    static std::string pLine(const Mark& m) {
        char b[200]; std::snprintf(b, sizeof(b), "P|%c|%lld|%lld|%lld|%c|%lld", m.side == BID ? 'B' : 'A', m.pivotEnd, m.oid, m.laterMs, (char)m.later, m.laterTraded); return b;
    }
    static bool parseP(const std::string& ln, int& side, long long& pe, int& how, long long& t, long long& tr) {
        const std::vector<std::string> f = split(ln, '|'); long long oid = 0;
        if (f.size() != 7 || f[0] != "P" || (f[1] != "B" && f[1] != "A") || !numLL(f[2], 1, 9000000000000000000LL, pe) || !numLL(f[3], -9000000000000000000LL, 9000000000000000000LL, oid)
            || !numLL(f[4], 0, 9000000000000000000LL, t) || f[5].size() != 1 || (f[5] != "P" && f[5] != "M" && f[5] != "F") || !numLL(f[6], 0, 100000000, tr)) return false;
        side = f[1] == "B" ? BID : ASK; how = f[5][0]; return true;
    }
    static std::string priceText(Tick t, double tick) {
        int d = 2; if (tick > 0 && finiteD(tick)) for (d = 0; d <= 6; ++d) { const double p = std::pow(10.0, d), v = tick * p; if (std::fabs(v - std::round(v)) < 1e-7) break; }
        char b[48]; std::snprintf(b, sizeof(b), "%.*f", d, (double)t * tick); return b;
    }
private:
    void shift(size_t cut) { for (size_t k = 0; k < pend.size(); ++k) pend[k].idx = pend[k].idx >= cut ? pend[k].idx - cut : 0; }
    void setSize(Seg& s, long long size, long long t) {
        if (size < 0) size = 0;
        s.size = size;
        if ((size >= P.minSize || !s.hist.empty()) && s.hist.size() < HIST_CAP) s.hist.push_back(std::make_pair(t, size));
    }
    void endSeg(Seg& s, int how, long long t) {
        s.tEnd = t; s.how = how;
        // a marked order's fate (P line, once)
        for (size_t k = 0; k < marks.size(); ++k) { Mark& m = marks[k]; if (m.oid == s.oid && m.px == s.px && !m.later && t >= m.decideMs) { m.later = how; m.laterMs = t; m.laterTraded = s.traded; rows.push_back(pLine(m)); } }
        if (!s.hist.empty()) gone.push_back(s);
    }
    void prune() {   // the book is full: drop the smallest orders that never were big (counted)
        for (auto it = live.begin(); it != live.end();) { if (it->second.hist.empty() && it->second.size < std::max(2LL, P.minSize / 4)) { it = live.erase(it); ++tally.pruned; } else ++it; }
    }
    long long extTime(int side, Tick ext, long long t0, long long t1) const {   // the first second in [t0, t1) a trade printed at the extreme
        for (size_t i = 0; i < secs.size(); ++i) {
            const long long s = secs[i].first; if (s + 999 < t0) continue; if (s >= t1) break;
            if (side == BID ? secs[i].second.first <= ext : secs[i].second.second >= ext) return std::max(s, t0);
        }
        return 0;
    }
    void newPivot(int side, size_t i) {
        const Bar& c = bars[i]; const std::string key = keyOf(side, c.endMs);
        if (done.count(key)) return;
        ++tally.pivots;
        Pend p; p.side = side; p.idx = i; p.ext = side == BID ? c.l : c.h; p.endMs = c.endMs; p.startMs = c.startMs; p.atr = atrAt(bars, i);
        if (!(p.atr > 0)) return;                                           // not enough candles for the ATR yet
        if (feedStartMs == 0 || c.startMs < feedStartMs + (long long)P.restS * 1000LL) { ++tally.noFeed; return; }   // the book was not watched long enough: silent
        const long long tx = extTime(side, p.ext, c.startMs, c.endMs);
        if (tx <= 0) { p.why = "no trade at the extreme in the feed"; noMark(p, p.why, bars.back().endMs); return; }
        p.tExt = tx;
        // the candidates: every segment alive when the extreme printed, at / just beyond it, rested restS before, big then
        const Tick beyond = std::max<Tick>((Tick)P.beyondTicks, (Tick)std::llround(P.beyondAtr * p.atr));
        const long long restMs = (long long)P.restS * 1000LL;
        const Seg* best = nullptr; long long bestShown = 0;
        auto consider = [&](const Seg& s) {
            if (s.side != side || s.hist.empty()) return;
            const bool at = side == BID ? (s.px <= p.ext && s.px >= p.ext - beyond) : (s.px >= p.ext && s.px <= p.ext + beyond);
            if (!at || s.tAt > tx - restMs || (s.tEnd != 0 && s.tEnd < tx)) return;
            const long long shown = s.maxIn(tx - SHOWN_LOOKBACK_MS, tx);
            if (shown < P.minSize) return;
            if (!best || shown > bestShown || (shown == bestShown && (std::llabs(s.px - p.ext) < std::llabs(best->px - p.ext) || (std::llabs(s.px - p.ext) == std::llabs(best->px - p.ext) && s.tAt < best->tAt)))) { best = &s; bestShown = shown; }
        };
        for (auto& kv : live) consider(kv.second);
        for (size_t k = 0; k < gone.size(); ++k) consider(gone[k]);
        if (!best) { noMark(p, "no real order at the extreme", bars.back().endMs); ++tally.noOrder; return; }
        p.picked = true; p.oid = best->oid; p.px = best->px; p.shown = bestShown; p.restS = (tx - best->tAt) / 1000; p.startKnown = best->startKnown; p.seg = *best;
        // the candles after the extreme (the 2 that made it a pivot) may already have left the order by the gate: decided now
        double away = -1e300;
        for (size_t j = i + 1; j < bars.size(); ++j) away = std::max(away, p.side == BID ? (double)(bars[j].c - p.px) : (double)(p.px - bars[j].c));
        if (away >= P.gateAtr * p.atr) { decide(p, bars.back(), away / p.atr); return; }
        pend.push_back(p);
    }
    const Seg* segOf(const Pend& p) const {   // the order's segment now (live, or ended)
        auto it = live.find(p.oid); if (it != live.end() && it->second.px == p.px && it->second.tAt == p.seg.tAt) return &it->second;
        for (size_t k = gone.size(); k-- > 0;) if (gone[k].oid == p.oid && gone[k].px == p.px && gone[k].tAt == p.seg.tAt) return &gone[k];
        return nullptr;
    }
    void decide(const Pend& p, const Bar& b, double runAtr) {
        const std::string key = keyOf(p.side, p.endMs); if (done.count(key)) return; done.insert(key);
        const Seg* s = segOf(p); const Seg& S = s ? *s : p.seg;
        Mark m; m.side = p.side; m.pivotEnd = p.endMs; m.tExt = p.tExt; m.ext = p.ext; m.oid = p.oid; m.px = p.px; m.shown = p.shown;
        m.filled = S.filledIn(p.startMs, b.endMs); m.restS = p.restS; m.startKnown = p.startKnown;
        m.atr10 = (long long)std::llround(p.atr * 10.0); m.run100 = (long long)std::llround(runAtr * 100.0); m.maxRun100 = m.run100; m.decideMs = b.endMs;
        m.absorbed = m.filled > 0 && (double)m.filled >= FILL_SHARE * (double)m.shown;
        if (S.tEnd != 0 && S.tEnd <= b.endMs) { m.later = S.how; m.laterMs = S.tEnd; m.laterTraded = S.traded; }
        marks.push_back(m); rows.push_back(mLine(m, tick));
        if (m.later) rows.push_back(pLine(m));
        ++tally.marks; if (m.absorbed) ++tally.absorbed; else ++tally.thick;
    }
    void noMark(const Pend& p, const std::string& why, long long t) {
        const std::string key = keyOf(p.side, p.endMs); if (done.count(key)) return; done.insert(key);
        NoMark z; z.side = p.side; z.pivotEnd = p.endMs; z.ext = p.ext; z.why = why; z.decideMs = t; nos.push_back(z); rows.push_back(nLine(z));
    }
};

// ---- text ---------------------------------------------------------------------------------------------------------------------
inline std::string clock12(long long ms) {
    if (ms <= 0) return "?";
    long long s = (ms / 1000) % 86400; if (s < 0) s += 86400;
    int h = (int)(s / 3600), m = (int)(s / 60 % 60); const bool pm = h >= 12; h %= 12; if (h == 0) h = 12;
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h, m, pm ? "PM" : "AM"); return b;
}
inline std::string dur(long long s) {
    if (s < 0) { s = 0; }
    char b[32];
    if (s < 60) std::snprintf(b, sizeof(b), "%lld s", s); else if (s < 3600) std::snprintf(b, sizeof(b), "%lld min %lld s", s / 60, s % 60);
    else std::snprintf(b, sizeof(b), "%lld h %lld min", s / 3600, s / 60 % 60);
    return b;
}
inline std::string x100(long long v) { char b[24]; std::snprintf(b, sizeof(b), "%.1f", (double)v / 100.0); return b; }
struct Row { std::string text; int ink = INK_WHITE; };
// the hover: title, question, what, result, why, how, later, example, "?" (<= 10 rows, <= 118 chars each)
inline std::vector<Row> hoverRows(const Mark& m, double tick, long long s20, long long liveSize) {
    std::vector<Row> v; const bool bid = m.side == BID;
    const std::string who = bid ? "Bid" : "Offer", px = Engine::priceText(m.px, tick), move = bid ? "selloff" : "rally";
    v.push_back(Row{ who + " " + std::to_string(m.shown) + " lots at " + px + " stopped the " + move + " - " + std::to_string(m.filled) + " filled, price turned "
        + x100(m.maxRun100 > m.run100 ? m.maxRun100 : m.run100) + " ATR ?", bid ? INK_GREEN : INK_RED });
    v.push_back(Row{ "Question: which resting order made this pivot - did it fill (absorb the attack) or was it too thick to get through?", INK_WHITE });
    v.push_back(Row{ "What: ONE real resting " + std::string(bid ? "buy" : "sell") + " order (one order id), " + std::to_string(m.shown) + " lots" + (s20 > 0 ? " (S20 " + std::to_string(s20) + ")" : std::string())
        + ", rested " + (m.startKnown ? "" : "at least ") + dur(m.restS) + " before the " + (bid ? "low" : "high") + " at " + clock12(m.tExt) + ".", INK_WHITE });
    const long long pct = m.shown > 0 ? (long long)std::llround(100.0 * (double)m.filled / (double)m.shown) : 0;
    if (m.absorbed) v.push_back(Row{ "FILLED: " + std::to_string(m.filled) + " of " + std::to_string(m.shown) + " lots traded (" + std::to_string(pct) + "%) - it ABSORBED the " + (bid ? "sellers" : "buyers") + " and price still turned.", bid ? INK_GREEN : INK_RED });
    else v.push_back(Row{ "TOO THICK: only " + std::to_string(m.filled) + " of " + std::to_string(m.shown) + " lots traded (" + std::to_string(pct) + "%) - the " + (bid ? "sellers" : "buyers") + " stalled at it and gave up.", bid ? INK_GREEN : INK_RED });
    v.push_back(Row{ "Why it matters: drawn only after price left it by 0.5 ATR - big size price still sits on is often a trap; this one made the turn.", INK_WHITE });
    v.push_back(Row{ std::string("How to use: the defended price of this pivot. A retest that holds ") + (bid ? "above" : "below") + " it = the same defender; a close through = it is gone.", INK_WHITE });
    std::string later;
    if (m.later == 'P') later = "Later: PULLED at " + clock12(m.laterMs) + " (" + std::to_string(m.laterTraded) + " traded) - the defender left: the level is weaker now.";
    else if (m.later == 'M') later = "Later: MOVED away at " + clock12(m.laterMs) + " - the defender re-priced: the level is weaker now.";
    else if (m.later == 'F') later = "Later: FILLED OUT at " + clock12(m.laterMs) + " (" + std::to_string(m.laterTraded) + " traded) - all of it was taken.";
    else if (liveSize > 0) later = "Later: still resting, " + std::to_string(liveSize) + " lots now.";
    else later = "Later: not seen since (feed restarted or out of view).";
    v.push_back(Row{ later, INK_WHITE });
    v.push_back(Row{ "Example: ES Oct 7 11:02 AM low - bid 100 at 7838.00 FILLED all 100 and held; price turned 1.6 ATR.", INK_WHITE });
    v.push_back(Row{ "? = not proven yet: the nightly scores every mark (lsFlexLevels\\TurnOrder).", INK_GREY });
    for (size_t i = 0; i < v.size(); ++i) if (v[i].text.size() > 130) v[i].text = v[i].text.substr(0, 127) + "...";
    return v;
}
} // namespace tor
#endif
