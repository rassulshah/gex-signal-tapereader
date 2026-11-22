// ErPaneLogic.h 0.2.0 (2026-11-21) -- the logic of lsEffortRewardPane "Effort Reward v0.2" (EffortRewardPane.cpp). SDK-free, unit tested
// (tests_erpane). Rassul 2026-11-20 / 21: "lets stop here and make this just the effort reward indicator. it calculates the effort at the top 25%
// of the bar (1st bar) and the bottom 25% of the bar (2nd bar)"; "put the red bar on the opposite side of the horzontal axis"; "from this
// indicator remove the 1.0 lines"; "no i dont want signals on this indicator.. we need to have a read instead maybe a t the top a strip that
// gives the read"; "make sure you mention magnitude"; "The word comes from the quantifier only", "0.0x is No absorbtion", "i would say .5 >=
// and < 1 is light", "moderate is 1-1.5, 1.5 and higher is heavy"; "build it. make sure you do an independed code review and no memory
// leasks etc... then build". Design: the doc "Effort Reward v0.2 - IRT design"; mockup effort-reward-final.html.
//
//   BARS (every candle, IRT volume at price): L = (high - low) / tick + 1 price levels, zone = max(1, round(0.25 L)) levels (half rounds up).
//     green (up)  = (offer-traded lots in the top zone / its levels) / (offer-traded lots in the other levels / their count)
//     red (down)  = (bid-traded lots in the bottom zone / its levels) / (bid-traded lots in the other levels / their count)
//     fewer than 4 levels, or nothing traded in the rest of the bar = no ratio.
//   READ STRIP (the newest Sw of lsRevRead's wy file, RevRead\wy-<MKT>-<spb>.txt line "stamp|Sw|dir|level|px|strength", dir -1 = a high swept):
//     strict absorption = aggressive buy lots traded above the swept level at prices that never traded higher later in the candle (a low
//       sweep: sell lots below the level that never traded lower). Implied fills (no aggressor) are left out.
//     x = those lots / the median over earlier sweeps of that market (params file ABS_MED); word from x only: NO < 0.5, LIGHT < 1.0,
//       MODERATE < 1.5, HEAVY 1.5+. "thin poke" = the sweep side's bar under 1.0x.
//     hidden = of those lots, the fills on resting orders beyond the largest size the order ever showed (live order feed only).
//     turn pill = the sellers' score (a high; the buyers' for a low): F a fresh counter order (ID issued < 2 s before) through 3+ prices
//       within 15 s after the extreme + H net counter delta in those 15 s / the normal 15-s delta of the half hour >= HARD_CUT + B the biggest
//       counter order in 30 s / the average order of the half hour > BIG_CUT; % = TURN_PCT[score]. Windows stop at the candle close; an
//       extreme in the last 30 s = "late".
//   Decided once (at / after the candle close) and recorded; a recorded read is never recomputed (no repaint).
//   Memory: fills kept for the FORMING candle only (FILL_CAP), summed per price at its close; resting orders tracked within NEAR_TICKS of
//   the last trade (SHOWN_CAP); the order-ID clock is a ring of CLOCK_CAP points; SUMS_KEEP closed candles kept. No per-tick history.
#pragma once
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace erp {

static const char* const VERSION = "0.2.0";
static const size_t FILL_CAP = 150000;      // fills of one candle (more = the candle is not decided live)
static const size_t SHOWN_CAP = 50000;      // resting orders tracked for the hidden (iceberg) lots
static const long long NEAR_TICKS = 64;     // only orders within this many ticks of the last trade are tracked
static const size_t CLOCK_CAP = 4096;       // order-ID clock points (one per second the newest ID moved)
static const size_t SUMS_KEEP = 12;         // closed candles kept for a Sw that is marked up to 3 candles later
static const size_t CELL_CAP = 4096;        // prices per candle summary
static const size_t WY_MAX_BYTES = 512 * 1024, PARAMS_MAX_BYTES = 64 * 1024, REC_MAX_BYTES = 64 * 1024;
static const size_t SW_KEEP = 400, REC_KEEP = 300, HIST_KEEP = 400;
static const int FRESH_S = 2, WIN_F = 15, WIN_B = 30, LATE_S = 30;

// ---------------------------------------------------------------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------------------------------------------------------------
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> v; size_t a = 0;
    for (;;) { const size_t b = s.find(d, a); v.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    return v;
}
inline bool toLL(const std::string& s, long long& v) {
    if (s.empty() || s.size() > 19) return false;
    char* e = nullptr; errno = 0; const long long x = std::strtoll(s.c_str(), &e, 10);
    if (!e || *e || errno) return false;
    v = x; return true;
}
inline bool toD(const std::string& s, double& v) {
    if (s.empty() || s.size() > 32) return false;
    char* e = nullptr; const double x = std::strtod(s.c_str(), &e);
    if (!e || *e || !std::isfinite(x)) return false;
    v = x; return true;
}
inline std::vector<std::string> lines(const std::string& txt, bool dropUnterminatedLast) {
    std::vector<std::string> v; size_t a = 0;
    while (a < txt.size()) {
        size_t b = txt.find('\n', a);
        if (b == std::string::npos) { if (dropUnterminatedLast) break; b = txt.size(); }
        std::string ln = txt.substr(a, b - a); if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (!ln.empty()) v.push_back(ln);
        a = b + 1;
    }
    return v;
}
inline std::string thousands(long long v) {
    const bool neg = v < 0; unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v;
    std::string d = std::to_string(u), o; int n = 0;
    for (size_t i = d.size(); i-- > 0;) { o.insert(o.begin(), d[i]); if (++n % 3 == 0 && i > 0) o.insert(o.begin(), ','); }
    return neg ? "-" + o : o;
}
inline std::string ratioText(double x) {            // "1.7x"; 99x cap
    if (!std::isfinite(x) || x < 0) return std::string();
    char b[24]; if (x >= 99.95) return "99x+";
    std::snprintf(b, sizeof b, "%.1fx", x); return b;
}
inline const char* wordOf(double x) {                // Rassul 11-21: the word comes from the quantifier only
    if (!std::isfinite(x) || x < 0.5) return "NO";
    if (x < 1.0) return "LIGHT";
    if (x < 1.5) return "MODERATE";
    return "HEAVY";
}
inline std::string levelName(const std::string& s) { // LevelMarks short names -> his names
    return s == "LHI" ? "LonHI" : s == "LLO" ? "LonLO" : s == "AHI" ? "AsiaHI" : s == "ALO" ? "AsiaLO" : s;
}
inline bool nameOk(const std::string& s) {
    if (s.size() > 16) return false;
    for (char c : s) if (!(std::isalnum((unsigned char)c) || c == '/' || c == '_' || c == '-' || c == '+')) return false;
    return true;
}
inline long long keyOf(long long sec, int spb) { return (sec / spb + 1) * (long long)spb; }   // IRT bar time = the bar END

// ---------------------------------------------------------------------------------------------------------------------------------
// BARS
// ---------------------------------------------------------------------------------------------------------------------------------
struct Row { long long tick = 0, buy = 0, sell = 0; };       // buy = traded at the offer, sell = traded at the bid
struct Bars { bool topOk = false, botOk = false; double top = 0, bot = 0; long long levels = 0, zone = 0; };
inline long long zoneOf(long long L) { const long long z = (L + 2) / 4; return z < 1 ? 1 : z; }   // round(0.25 L), half up
inline Bars barRatios(const std::vector<Row>& rows, long long lo, long long hi) {
    Bars b; if (hi < lo) return b;
    const long long L = hi - lo + 1; if (L < 4 || L > 200000) return b;
    const long long z = zoneOf(L); b.levels = L; b.zone = z;
    long long bt = 0, br = 0, st = 0, sr = 0;
    for (const Row& r : rows) {
        if (r.tick < lo || r.tick > hi || r.buy < 0 || r.sell < 0) continue;
        if (r.tick > hi - z) bt += r.buy; else br += r.buy;
        if (r.tick < lo + z) st += r.sell; else sr += r.sell;
    }
    const double nz = (double)z, nr = (double)(L - z);
    if (br > 0) { b.top = ((double)bt / nz) / ((double)br / nr); b.topOk = std::isfinite(b.top); }
    if (sr > 0) { b.bot = ((double)st / nz) / ((double)sr / nr); b.botOk = std::isfinite(b.bot); }
    return b;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// SWEEPS (lsRevRead's wy file)
// ---------------------------------------------------------------------------------------------------------------------------------
struct Sw { long long stamp = 0; int dir = 0; std::string lv; long long lvt = 0; };
// header WY|1|<MKT>|<spb>|<ver>; keeps the newest SW_KEEP Sw lines, sorted by stamp; false + why on a foreign / garbled header
inline bool parseWy(const std::string& txt, const std::string& mkt, int spb, std::vector<Sw>& out, std::string& why) {
    out.clear(); why.clear();
    if (txt.size() > WY_MAX_BYTES) { why = "sweep file too big"; return false; }
    const std::vector<std::string> L = lines(txt, true);
    if (L.empty()) { why = "sweep file empty"; return false; }
    const std::vector<std::string> h = split(L[0], '|');
    if (h.size() < 4 || h[0] != "WY" || h[1] != "1") { why = "sweep file header"; return false; }
    if (h[2] != mkt) { why = "sweep file is " + h[2].substr(0, 8); return false; }
    long long s = 0; if (!toLL(h[3], s) || s != spb) { why = "sweep file bar size"; return false; }
    std::map<long long, Sw> m;
    for (size_t i = 1; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], '|');
        if (c.size() < 5 || c[1] != "Sw") continue;
        Sw w; long long d = 0;
        if (!toLL(c[0], w.stamp) || w.stamp <= 0 || !toLL(c[2], d) || (d != 1 && d != -1) || !toLL(c[4], w.lvt) || w.lvt <= 0 || !nameOk(c[3])) continue;
        w.dir = (int)d; w.lv = levelName(c[3]); m[w.stamp] = w;
    }
    for (std::map<long long, Sw>::const_iterator it = m.begin(); it != m.end(); ++it) out.push_back(it->second);
    if (out.size() > SW_KEEP) out.erase(out.begin(), out.end() - (std::ptrdiff_t)SW_KEEP);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// PARAMS (lsFlexLevels\LRA-EffortReward-<MKT>.csv, written by the nightly; defaults when missing / garbled)
// ---------------------------------------------------------------------------------------------------------------------------------
struct Hist { long long stamp = 0; int dir = 0; long long lvt = 0, strict = -1, tot = -1, hid = -1; };
struct Params {
    std::string mkt; bool fromFile = false; std::string why = "defaults";
    double absMed = 0, hardCut = 0, bigCut = 0; double pct[4] = { 28, 38, 56, 67 };
    double n15[48], avgOrd[48];
    std::vector<Hist> hist;                     // sweeps the nightly measured from the order store (history candles, no pill)
    Params() { for (int i = 0; i < 48; ++i) { n15[i] = 0; avgOrd[i] = 0; } }
};
// built-in defaults (research 2026-11-21: 156 sweeps, 7 markets; strict medians over all sweeps; per-market thirds, pooled for < 6)
inline Params defaults(const std::string& mkt) {
    Params p; p.mkt = mkt;
    struct D { const char* m; double med, hard, big; };
    static const D T[] = { { "ES", 244, 3.010, 12.094 }, { "NQ", 55.5, 3.838, 13.374 }, { "CL", 53, 3.154, 4.283 }, { "GC", 24.5, 3.059, 3.309 },
                           { "HG", 8.5, 4.911, 2.316 }, { "NG", 78, 4.276, 4.967 }, { "EU", 112, 3.439, 6.999 } };
    for (const D& d : T) if (mkt == d.m) { p.absMed = d.med; p.hardCut = d.hard; p.bigCut = d.big; }
    return p;
}
inline bool fillVals(const std::vector<std::string>& c, double* v, size_t n, bool allowZero) {
    if (c.size() != n + 1) return false;
    for (size_t i = 0; i < n; ++i) { double x = 0; if (!toD(c[i + 1], x) || x < 0 || x > 1e9 || (!allowZero && x <= 0) || (x > 0 && x < 0.01)) return false; v[i] = x; }   // (review #11) no tiny divisors
    return true;
}
// VERSION,1 first, MARKET must match; any bad line = defaults for everything + why (never a half-read file)
inline Params parseParams(const std::string& txt, const std::string& mkt) {
    Params d = defaults(mkt);
    if (txt.empty()) { d.why = "no params file (defaults)"; return d; }
    if (txt.size() > PARAMS_MAX_BYTES) { d.why = "params file too big (defaults)"; return d; }
    const std::vector<std::string> L = lines(txt, true);
    if (L.size() < 2 || L[0] != "VERSION,1") { d.why = "params VERSION (defaults)"; return d; }
    if (L[1] != "MARKET," + mkt) { d.why = "params market (defaults)"; return d; }
    Params p = d; std::map<long long, Hist> hs; bool haveN = false, haveA = false;
    for (size_t i = 2; i < L.size(); ++i) {
        const std::vector<std::string> c = split(L[i], ',');
        double x = 0; bool ok = true;
        if (c[0] == "ABS_MED") ok = c.size() == 2 && toD(c[1], x) && x > 0 && x < 1e9 && (p.absMed = x, true);
        else if (c[0] == "HARD_CUT") ok = c.size() == 2 && toD(c[1], x) && x > 0 && x < 1e6 && (p.hardCut = x, true);
        else if (c[0] == "BIG_CUT") ok = c.size() == 2 && toD(c[1], x) && x > 0 && x < 1e6 && (p.bigCut = x, true);
        else if (c[0] == "TURN_PCT") { ok = fillVals(c, p.pct, 4, true); for (int k = 0; ok && k < 4; ++k) if (p.pct[k] > 100) ok = false; }
        else if (c[0] == "N15") ok = (haveN = fillVals(c, p.n15, 48, true));
        else if (c[0] == "AVG_ORDER") ok = (haveA = fillVals(c, p.avgOrd, 48, true));
        else if (c[0] == "SW") {
            Hist h; long long dd = 0;
            ok = c.size() == 7 && toLL(c[1], h.stamp) && h.stamp > 0 && toLL(c[2], dd) && (dd == 1 || dd == -1) && toLL(c[3], h.lvt) && h.lvt > 0
                 && toLL(c[4], h.strict) && h.strict >= 0 && toLL(c[5], h.tot) && h.tot >= h.strict && toLL(c[6], h.hid) && h.hid >= -1;
            if (ok) { h.dir = (int)dd; hs[h.stamp] = h; }
        }
        else if (c[0] == "END") continue;
        else continue;                                                    // unknown keys are ignored (a newer nightly)
        if (!ok) { d.why = "params line " + std::to_string(i + 1) + " garbled (defaults)"; return d; }
    }
    (void)haveN; (void)haveA;
    for (std::map<long long, Hist>::const_iterator it = hs.begin(); it != hs.end(); ++it) p.hist.push_back(it->second);
    if (p.hist.size() > HIST_KEEP) p.hist.erase(p.hist.begin(), p.hist.end() - (std::ptrdiff_t)HIST_KEEP);
    p.fromFile = true; p.why = "ok"; return p;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// THE LIVE ORDER FEED -> per-candle sums
// ---------------------------------------------------------------------------------------------------------------------------------
// order IDs: IRT's are increasing ints; unwrapped as 32-bit against the newest so a wrap never breaks the clock
struct IdClock {
    struct P { long long sec, id; };
    std::deque<P> pts; long long maxId = 0; bool have = false; int jumpRun = 0; long long jumps = 0;
    static const long long JUMP = 1LL << 24;                              // (review #9) a bigger jump needs 8 IDs in a row to be believed
    long long unwrap(long long raw) const {
        const uint32_t u = (uint32_t)(raw & 0xFFFFFFFFLL);
        if (!have) return (long long)u;
        const int32_t d = (int32_t)(u - (uint32_t)(maxId & 0xFFFFFFFFLL));
        return maxId + (long long)d;
    }
    void note(long long id, long long sec) {
        if (have && id <= maxId) return;
        if (have && id > maxId + JUMP) { ++jumps; if (++jumpRun < 8) return; pts.clear(); }   // a garbage ID never freezes the clock
        jumpRun = 0;
        maxId = id; have = true;
        if (!pts.empty() && pts.back().sec >= sec) pts.back().id = id;
        else { P p; p.sec = sec; p.id = id; pts.push_back(p); if (pts.size() > CLOCK_CAP) pts.pop_front(); }
    }
    // seconds since the ID was first reached; -1 = no clock yet
    long long age(long long id, long long sec) const {
        if (pts.empty()) return -1;
        if (id > maxId) return 0;
        size_t lo = 0, hi = pts.size();                                   // the first point whose newest ID >= id
        while (lo < hi) { const size_t m = (lo + hi) / 2; if (pts[m].id >= id) hi = m; else lo = m + 1; }
        if (lo >= pts.size()) return 0;
        const long long a = sec - pts[lo].sec; return a < 0 ? 0 : a;
    }
    void clear() { pts.clear(); maxId = 0; have = false; jumpRun = 0; }
};
struct Fill { long long sec = 0, px = 0, size = 0, aid = 0, hid = 0; int ag = 0; bool known = false, fresh = false; };
struct Cell { long long allB = 0, npB = 0, hidB = 0, unkB = 0, allS = 0, npS = 0, hidS = 0, unkS = 0; };
struct Turn { bool ok = false; long long extPx = 0, extSec = 0, hard = 0, big = 0; bool fresh = false, late = false; };
struct Sum { long long key = 0; bool complete = false, overflow = false; std::map<long long, Cell> cells; Turn hi, lo; long long fills = 0, implied = 0, lots = 0; };   // lots = every filled lot read (implied too): the coverage check vs the chart's bar volume
struct Counters { long long events = 0, fills = 0, implied = 0, late = 0, bad = 0, overflows = 0, shownResets = 0, candles = 0, complete = 0, noAggr = 0, future = 0; };

class Live {
public:
    int spb = 0;
    Counters n;
    std::deque<Sum> sums;                                                 // closed candles, oldest first
    // a fresh start (new chart / symbol / reconnect): everything open is dropped and the next candle cannot be complete
    void reset(int barSecs) {
        spb = barSecs; fills_.clear(); std::vector<Fill>().swap(fills_); ords_.clear(); clock_.clear();
        curKey_ = 0; liveSince_ = -1; haveRef_ = false; ref_ = 0; overflow_ = false; sums.clear(); lastClosed_ = 0; limitKey_ = 0;
    }
    // (review #4) the newest candle the chart allows: an event dated past it is counted bad and never opens a candle
    void setLimit(long long chartFormingKey) { limitKey_ = chartFormingKey > 0 ? chartFormingKey + spb : 0; }
    void gap() {                                                           // the stream was interrupted: open candle and the clock are not whole
        fills_.clear(); curKey_ = 0; liveSince_ = -1; overflow_ = false; clock_.clear(); ords_.clear();
    }
    // one order event: sec = its RTDATE second, px in ticks, action 0 NEW 1 MOD 2 DEL 3 FILL, buySell 1 buy / 0 sell / 2 unknown
    void onEvent(long long sec, int action, int buySell, long long px, long long size, long long oidRaw, long long aidRaw) {
        ++n.events;
        if (spb <= 0 || sec <= 0 || px <= 0 || size < 0 || action < 0 || action > 3) { ++n.bad; return; }
        const long long key = keyOf(sec, spb);
        if ((curKey_ != 0 && key < curKey_) || key <= lastClosed_) {     // an event of a candle already closed: that candle is not whole (review #2)
            ++n.late; for (Sum& x : sums) if (x.key == key) x.complete = false;
            return;
        }
        if (limitKey_ > 0 && key > limitKey_) { ++n.future; return; }
        if (liveSince_ < 0 && action == 3) liveSince_ = sec;              // (review #6) the live clock starts at the first FILL (a book snapshot may carry old times)
        if (key > curKey_) { if (curKey_ != 0) close(curKey_); open(key); }
        const long long oid = oidRaw ? clock_.unwrap(oidRaw) : 0;
        switch (action) {
        case 0: case 1: {
            if (oid) { if (action == 0) clock_.note(oid, sec); shown(oid, px, size); }
            break; }
        case 2: if (oid) ords_.erase(oid); break;
        case 3: {
            ++n.fills; ref_ = px; haveRef_ = true;
            Ord* o = nullptr; if (oid) { std::unordered_map<long long, Ord>::iterator it = ords_.find(oid); if (it != ords_.end()) o = &it->second; }
            if (oid && oid == (aidRaw ? clock_.unwrap(aidRaw) : 0)) { ++n.bad; break; }   // an order filling itself is not a trade between two sides
            if (buySell != 0 && buySell != 1) {                           // implied (no aggressor): kept for the extreme only (as the research)
                ++n.implied; ++implied_; if (o) o->filled += size;
                Fill f; f.sec = sec; f.px = px; f.size = size; f.ag = 0;
                if (fills_.size() < FILL_CAP) fills_.push_back(f); else if (!overflow_) { overflow_ = true; ++n.overflows; }
                break;
            }
            Fill f; f.sec = sec; f.px = px; f.size = size; f.ag = buySell == 1 ? 1 : -1;
            const long long aid = aidRaw ? clock_.unwrap(aidRaw) : 0; f.aid = aid;
            if (aid) { const long long a = clock_.age(aid, sec); f.fresh = a >= 0 && a < FRESH_S; clock_.note(aid, sec); } else ++n.noAggr;
            if (o) { const long long b = std::max(0LL, o->filled - o->maxShown); o->filled += size; const long long a2 = std::max(0LL, o->filled - o->maxShown); f.hid = a2 - b; f.known = true; }
            if (fills_.size() < FILL_CAP) fills_.push_back(f); else if (!overflow_) { overflow_ = true; ++n.overflows; }
            break; }
        default: break;
        }
    }
    // the chart says candles up to `lastClosed` are closed: a quiet feed still closes its candle
    void closeThrough(long long lastClosed) { if (curKey_ != 0 && curKey_ <= lastClosed) { close(curKey_); curKey_ = 0; } }
    const Sum* find(long long key) const { for (const Sum& s : sums) if (s.key == key) return &s; return nullptr; }
    size_t openFills() const { return fills_.size(); }
    size_t orders() const { return ords_.size(); }
    size_t clockPoints() const { return clock_.pts.size(); }
    long long forming() const { return curKey_; }
private:
    struct Ord { long long px = 0, maxShown = 0, filled = 0; };
    std::vector<Fill> fills_; std::unordered_map<long long, Ord> ords_; IdClock clock_;
    long long curKey_ = 0, liveSince_ = -1, ref_ = 0, implied_ = 0, lastClosed_ = 0, limitKey_ = 0; bool haveRef_ = false, overflow_ = false;
    void shown(long long oid, long long px, long long size) {
        std::unordered_map<long long, Ord>::iterator it = ords_.find(oid);
        if (it == ords_.end()) {
            if (haveRef_ && std::llabs(px - ref_) > NEAR_TICKS) return;
            if (ords_.size() >= SHOWN_CAP) prune();
            if (ords_.size() >= SHOWN_CAP) return;
            Ord o; o.px = px; o.maxShown = size; ords_[oid] = o; return;
        }
        it->second.px = px; if (size > it->second.maxShown) it->second.maxShown = size;
        if (haveRef_ && std::llabs(px - ref_) > NEAR_TICKS) ords_.erase(it);
    }
    void prune() {
        for (std::unordered_map<long long, Ord>::iterator it = ords_.begin(); it != ords_.end();) { if (!haveRef_ || std::llabs(it->second.px - ref_) > NEAR_TICKS) it = ords_.erase(it); else ++it; }
        if (ords_.size() >= SHOWN_CAP * 9 / 10) { ords_.clear(); ++n.shownResets; }
    }
    void open(long long key) { curKey_ = key; fills_.clear(); overflow_ = false; implied_ = 0; }
    void close(long long key) {
        Sum s; s.key = key; s.overflow = overflow_; s.fills = (long long)fills_.size(); s.implied = implied_;
        for (const Fill& f : fills_) s.lots += f.size;
        s.complete = !overflow_ && liveSince_ >= 0 && liveSince_ < key - spb;
        if (key > lastClosed_) lastClosed_ = key;
        ++n.candles; if (s.complete) ++n.complete;
        summarize(s);
        sums.push_back(s); while (sums.size() > SUMS_KEEP) sums.pop_front();
        fills_.clear(); if (fills_.capacity() > 65536) std::vector<Fill>().swap(fills_);
        if (ords_.size() > SHOWN_CAP / 2) prune();
        overflow_ = false; implied_ = 0;
    }
    void summarize(Sum& s) const {
        const std::vector<Fill>& F = fills_; const size_t nf = F.size(); if (nf == 0) return;
        // no progress: nothing printed beyond this price from the NEXT fill on (the last fill counts as no progress)
        long long runMax = 0, runMin = 0; bool any = false;
        for (size_t k = nf; k-- > 0;) {
            const Fill& f = F[k]; if (f.ag == 0) continue;                 // implied prints: no side, not a price that was "traded beyond" (research)
            const long long aMax = any ? runMax : f.px, aMin = any ? runMin : f.px;
            {
                std::map<long long, Cell>::iterator ci = s.cells.find(f.px);
                if (ci == s.cells.end()) { if (s.cells.size() >= CELL_CAP) { s.overflow = true; s.complete = false; } else ci = s.cells.insert(std::make_pair(f.px, Cell())).first; }
                if (ci != s.cells.end()) {
                    Cell& c = ci->second;
                    if (f.ag > 0) { c.allB += f.size; if (aMax <= f.px) { c.npB += f.size; c.hidB += f.hid; if (!f.known) c.unkB += f.size; } }
                    else { c.allS += f.size; if (aMin >= f.px) { c.npS += f.size; c.hidS += f.hid; if (!f.known) c.unkS += f.size; } }
                }
            }
            if (!any) { runMax = runMin = f.px; any = true; } else { if (f.px > runMax) runMax = f.px; if (f.px < runMin) runMin = f.px; }
        }
        s.hi = turnOf(s.key, -1); s.lo = turnOf(s.key, 1);
    }
    // the counter side's answer after the extreme. dir -1 = the high (counter = sellers), +1 = the low (counter = buyers)
    Turn turnOf(long long key, int dir) const {
        Turn t; const std::vector<Fill>& F = fills_; if (F.empty()) return t;
        size_t ie = 0; for (size_t k = 1; k < F.size(); ++k) if (dir < 0 ? F[k].px > F[ie].px : F[k].px < F[ie].px) ie = k;   // the FIRST fill at the extreme (implied prints too, as the research)
        t.ok = true; t.extPx = F[ie].px; t.extSec = F[ie].sec; t.late = key - t.extSec < LATE_S;
        const int ctr = dir < 0 ? -1 : 1;
        struct G { long long lots = 0, p[3] = { 0, 0, 0 }; int np = 0; bool fresh = false, first = true; };
        std::unordered_map<long long, G> g15, g30;
        for (size_t k = ie + 1; k < F.size(); ++k) {
            const Fill& f = F[k]; const long long dt = f.sec - t.extSec; if (dt > WIN_B) break;
            if (dt <= WIN_F) t.hard += (long long)ctr * f.ag * f.size;    // net counter delta, counter side positive
            if (f.ag != ctr || !f.aid) continue;
            { std::unordered_map<long long, G>::iterator gi = g30.find(f.aid);
              if (gi == g30.end() && g30.size() < 20000) gi = g30.insert(std::make_pair(f.aid, G())).first;
              if (gi != g30.end()) { gi->second.lots += f.size; if (gi->second.lots > t.big) t.big = gi->second.lots; } }
            if (dt <= WIN_F && (g15.count(f.aid) || g15.size() < 20000)) {
                G& a = g15[f.aid]; if (a.first) { a.fresh = f.fresh; a.first = false; }
                bool seen = false; for (int q = 0; q < a.np; ++q) if (a.p[q] == f.px) seen = true;
                if (!seen && a.np < 3) a.p[a.np++] = f.px;
                if (a.np >= 3 && a.fresh) t.fresh = true;
            }
        }
        return t;
    }
};

// ---------------------------------------------------------------------------------------------------------------------------------
// THE READ (decided once, recorded)
// ---------------------------------------------------------------------------------------------------------------------------------
struct Read {
    long long stamp = 0; int dir = 0; std::string lv; long long lvt = 0;
    long long strict = -1, tot = -1, hid = -1, unk = -1;                  // -1 = not known
    int F = -1, H = -1, B = -1, score = -1; bool late = false; char src = '-';   // src L live order feed, N nightly, - none
    long long hard100 = -1, big100 = -1;                                  // the ratios x100 (record / status)
};
struct Strict { long long strict = 0, tot = 0, hid = 0, unk = 0; };
inline Strict strictOf(const Sum& s, int dir, long long lvt) {
    Strict r;
    for (std::map<long long, Cell>::const_iterator it = s.cells.begin(); it != s.cells.end(); ++it) {
        const Cell& c = it->second;
        if (dir < 0 && it->first > lvt) { r.tot += c.allB; r.strict += c.npB; r.hid += c.hidB; r.unk += c.unkB; }
        if (dir > 0 && it->first < lvt) { r.tot += c.allS; r.strict += c.npS; r.hid += c.hidS; r.unk += c.unkS; }
    }
    return r;
}
// tod = the half hour (CT clock) of the extreme: hour * 2 + minute / 30
inline Read decideLive(const Sum& s, const Sw& w, const Params& P, int tod) {
    Read r; r.stamp = w.stamp; r.dir = w.dir; r.lv = w.lv; r.lvt = w.lvt;
    if (!s.complete || s.key != w.stamp) return r;
    const Strict x = strictOf(s, w.dir, w.lvt);
    r.strict = x.strict; r.tot = x.tot; r.hid = x.hid; r.unk = x.unk; r.src = 'L';
    const Turn& t = w.dir < 0 ? s.hi : s.lo;
    if (!t.ok) return r;
    if (w.dir < 0 ? t.extPx <= w.lvt : t.extPx >= w.lvt) { Read z; z.stamp = w.stamp; z.dir = w.dir; z.lv = w.lv; z.lvt = w.lvt; return z; }   // (review #5) this chart never traded through the level: another contract
    r.late = t.late; r.F = t.fresh ? 1 : 0;
    if (tod >= 0 && tod < 48 && P.n15[tod] > 0 && P.hardCut > 0) { const double h = (double)t.hard / P.n15[tod]; r.H = h >= P.hardCut ? 1 : 0; r.hard100 = (long long)std::llround(std::max(-1e6, std::min(1e6, h)) * 100); }
    if (tod >= 0 && tod < 48 && P.avgOrd[tod] > 0 && P.bigCut > 0) { const double b = (double)t.big / P.avgOrd[tod]; r.B = b > P.bigCut ? 1 : 0; r.big100 = (long long)std::llround(std::min(1e6, b) * 100); }
    if (r.H >= 0 && r.B >= 0) r.score = r.F + r.H + r.B;
    return r;
}
inline Read fromHist(const Hist& h, const Sw& w) {
    Read r; r.stamp = w.stamp; r.dir = w.dir; r.lv = w.lv; r.lvt = w.lvt;
    if (h.stamp != w.stamp || h.dir != w.dir || h.lvt != w.lvt) return r;
    r.strict = h.strict; r.tot = h.tot; r.hid = h.hid; r.src = 'N'; return r;
}
// record line: R|stamp|dir|lv|lvt|strict|tot|hid|unk|F|H|B|score|late|src|hard100|big100
inline std::string recLine(const Read& r) {
    char b[512];
    std::snprintf(b, sizeof b, "R|%lld|%d|%s|%lld|%lld|%lld|%lld|%lld|%d|%d|%d|%d|%d|%c|%lld|%lld", r.stamp, r.dir, r.lv.c_str(), r.lvt, r.strict, r.tot, r.hid, r.unk,
                  r.F, r.H, r.B, r.score, r.late ? 1 : 0, r.src, r.hard100, r.big100);
    return b;
}
inline bool parseRec(const std::string& ln, Read& r) {
    const std::vector<std::string> c = split(ln, '|'); if (c.size() != 17 || c[0] != "R") return false;
    long long v[16]; const int idx[] = { 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16 };
    for (int k = 0; k < 14; ++k) if (!toLL(c[(size_t)idx[k]], v[k])) return false;
    if (v[0] <= 0 || (v[1] != 1 && v[1] != -1) || v[2] <= 0 || !nameOk(c[3]) || c[14].size() != 1 || (c[14][0] != 'L' && c[14][0] != 'N' && c[14][0] != '-')) return false;
    for (int k = 3; k <= 6; ++k) if (v[k] < -1) return false;
    for (int k = 7; k <= 9; ++k) if (v[k] < -1 || v[k] > 1) return false;
    if (v[10] < -1 || v[10] > 3 || (v[11] != 0 && v[11] != 1)) return false;
    r = Read(); r.stamp = v[0]; r.dir = (int)v[1]; r.lv = c[3]; r.lvt = v[2]; r.strict = v[3]; r.tot = v[4]; r.hid = v[5]; r.unk = v[6];
    r.F = (int)v[7]; r.H = (int)v[8]; r.B = (int)v[9]; r.score = (int)v[10]; r.late = v[11] == 1; r.src = c[14][0]; r.hard100 = v[12]; r.big100 = v[13];
    return true;
}
inline std::string recHeader(const std::string& mkt, int spb) { return "ERR|1|" + mkt + "|" + std::to_string(spb); }
inline std::map<long long, Read> parseRecFile(const std::string& txt, const std::string& mkt, int spb) {
    std::map<long long, Read> m; if (txt.size() > REC_MAX_BYTES) return m;
    const std::vector<std::string> L = lines(txt, true);
    if (L.empty() || L[0] != recHeader(mkt, spb)) return m;
    for (size_t i = 1; i < L.size(); ++i) { Read r; if (parseRec(L[i], r)) m[r.stamp] = r; }
    while (m.size() > REC_KEEP) m.erase(m.begin());
    return m;
}
inline std::string recFileText(const std::map<long long, Read>& m, const std::string& mkt, int spb) {
    std::string s = recHeader(mkt, spb) + "\n"; size_t skip = m.size() > REC_KEEP ? m.size() - REC_KEEP : 0;
    for (std::map<long long, Read>::const_iterator it = m.begin(); it != m.end(); ++it) { if (skip) { --skip; continue; } s += recLine(it->second) + "\n"; }
    return s;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// THE STRIP TEXT
// ---------------------------------------------------------------------------------------------------------------------------------
struct Strip { std::string text; std::string pill; int pillTone = 0; int word = -1; };   // pillTone 2 green / 1 amber / 0 red / -1 grey; word 0..3 NO..HEAVY
inline Strip stripOf(const Read& r, const Params& P, bool sideBarOk, double sideBar) {
    Strip s; const bool hi = r.dir < 0;
    if (r.strict < 0 || r.tot < 0) { s.text = r.lv + " sweep \xB7 absorption \x96"; s.pill = "\x96"; s.pillTone = -1; return s; }
    const double x = (double)r.strict / std::max(1.0, P.absMed);
    const char* w = wordOf(x); s.word = x < 0.5 ? 0 : x < 1.0 ? 1 : x < 1.5 ? 2 : 3;
    s.text = std::string(w) + " ABSORPTION " + ratioText(x) + " \x96 " + thousands(r.strict) + " of " + thousands(r.tot) + (hi ? " buy lots above " : " sell lots below ") + r.lv + " got no further";
    if (r.hid > 0) s.text += " (" + thousands(r.hid) + " hidden)";
    if (sideBarOk && sideBar < 1.0) s.text += " \xB7 thin poke";
    if (r.late && r.score >= 0) { s.pill = "late"; s.pillTone = -1; }
    else if (r.score >= 0 && r.score <= 3) { char b[24]; std::snprintf(b, sizeof b, "%.0f%% turn", P.pct[r.score]); s.pill = b; s.pillTone = r.score >= 2 ? 2 : r.score == 1 ? 1 : 0; }
    else { s.pill = "\x96"; s.pillTone = -1; }
    return s;
}

}  // namespace erp
