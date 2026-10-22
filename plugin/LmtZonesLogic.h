#ifndef LS_LMTZONESLOGIC_H
#define LS_LMTZONESLOGIC_H
// LmtZonesLogic.h 1.0.0 (2026-10-22) -- LMT ZONES of lsLiquidityProfile 0.48.0. SDK-free; tested by tests_lmz/test_lmz.cpp (logic),
// tests_lmz/test_lmz_host.cpp (mock host) and replayed on a recorded MBO session (tests_lmz/replay_lmz.cpp).
//
// Rassul approved mockup v3 on 2026-10-22 09:34 CT ("Ok build it") with two fixes: (1) gamma lines GREY (not purple), the yellow
// confluence diamond kept, ABC internal-breakout zones grey; (2) far fewer small order lines - only the zone orders and the far orders are
// drawn, every other order stays in the LMT column. Rules = PULLBACK-ZONES-FINAL-2026-10-22.md (research-artifacts\learning):
//   - A zone = ONE real large resting LIMIT order: one MBO order id >= Big Lot S20 (market, RTH / overnight), rested >= 30 s at its price,
//     re-priced <= 1x. The zone ends the moment the order is pulled (X). Re-priced again (2nd move) or out of its band = ends (X); a first
//     re-price inside the band = the zone follows it (a new segment; the old one stays as drawn).
//   - Kinds (offers above price; bids mirror, LOG ONLY by default): PRINT = in the print area of a WITH-TREND breakdown (BD: the first RED
//     3-min candle closing below a confirmed 1-ATR zigzag low; area = BD close - 0.1 ATR .. broken level + 0.25 ATR) in the last 3 h;
//     ABC = the same in an INTERNAL (counter-trend) break - grey, context only; ORIGIN = within 0.25 ATR of the BD wave's origin high;
//     NEAR = the biggest other real order within 2 ATR; FAR = 2-3 ATR, thin line, a map only (no edge).
//   - Gamma confluence: within 0.25 ATR of a MenthorQ gamma level -> the yellow diamond.
//   - Per market (study section 5; the nightly LRA-LmtZones-<MKT>.csv overrides): NG / HG offers drawn; NQ offers "?"; CL offers only on
//     a gamma level; ES only the with-trend BD-print offer "?"; GC / 6E log only; bids never drawn (LMT / LMT F columns only).
//   - Decided on CLOSED bars, recorded append-only (Z / M / F / E lines), read back after a restart - a zone only extends, then ends ONCE.
// No look-ahead: the zigzag, trend and breakouts use closed bars only; an order counts only once it has rested 30 s by the bar's end.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace lmz {
static const char* const VERSION = "1.0.0";
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Kind { K_PRINT = 0, K_ABC = 1, K_ORIGIN = 2, K_NEAR = 3, K_FAR = 4, K_N = 5 };
enum End { LIVE = 0, FILLED = 1, PULLED = 2, MOVED = 3, BROKEN = 4, EXPIRED = 5, SESSION = 6, LEFT = 7, NEARED = 8 };
enum Ink { INK_WHITE = 0, INK_GREEN = 1, INK_RED = 2, INK_GREY = 3, INK_YELLOW = 4 };

inline const char* kindCode(int k) { static const char* c[K_N] = { "PRINT", "ABC", "ORIGIN", "NEAR", "FAR" }; return k >= 0 && k < K_N ? c[k] : "?"; }
inline int kindOf(const std::string& s) { for (int k = 0; k < K_N; ++k) if (s == kindCode(k)) return k; return -1; }
inline const char* endCode(int e) {
    static const char* c[] = { "LIVE", "FILLED", "PULLED", "MOVED", "BROKEN", "EXPIRED", "SESSION", "LEFT", "NEARED" };
    return e >= 0 && e <= NEARED ? c[e] : "?";
}
inline int endOf(const std::string& s) { for (int e = LIVE; e <= NEARED; ++e) if (s == endCode(e)) return e; return -1; }
inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }

// ---- per-market rules ---------------------------------------------------------------------------------------------------------
// draw: 0 = log only, 1 = drawn, 2 = drawn with "?" (LEARNING)
struct Rule {
    int print = 2, origin = 2, nearO = 1;  // offers: with-trend BD-print, origin, the biggest near order
    bool gammaOnly = false;                // CL: offers drawn only with gamma confluence
    bool bids = false;                     // bids never drawn by default (study: worse than random)
    bool farOn = true;                       // far orders: thin map lines
    bool abc = true;                       // internal breaks: grey context
    long long s20Rth = 20, s20On = 20;     // fallback Big Lot S20 (resting-order pool, study table) when the live footer is still learning
    int restS = 30, maxMoves = 1;
    double nearAtr = 2.0, farAtr = 3.0, confAtr = 0.25, brokenAtr = 1.0;
    int expireMin = 240;
    bool s20Set = false;                   // the nightly params file set S20_RTH / S20_ON (then it wins over the live footer)
    bool ok = true; std::string src = "defaults";
};
inline Rule ruleFor(const std::string& m) {
    Rule r;
    if (m == "NG") { r.print = 2; r.origin = 2; r.nearO = 1; r.s20Rth = 10; r.s20On = 10; }
    else if (m == "HG") { r.print = 2; r.origin = 2; r.nearO = 1; r.s20Rth = 5; r.s20On = 5; }
    else if (m == "NQ") { r.print = 2; r.origin = 2; r.nearO = 2; r.s20Rth = 20; r.s20On = 15; }
    else if (m == "CL") { r.print = 2; r.origin = 2; r.nearO = 1; r.gammaOnly = true; r.s20Rth = 15; r.s20On = 20; }
    else if (m == "ES") { r.print = 2; r.origin = 0; r.nearO = 0; r.s20Rth = 45; r.s20On = 25; }
    else if (m == "GC") { r.print = 0; r.origin = 0; r.nearO = 0; r.abc = false; r.s20Rth = 9; r.s20On = 12; }
    else if (m == "EU") { r.print = 0; r.origin = 0; r.nearO = 0; r.abc = false; r.farOn = false; r.s20Rth = 42; r.s20On = 41; }
    else { r.print = 0; r.origin = 0; r.nearO = 0; r.abc = false; r.farOn = false; }
    return r;
}
// any market with nothing drawn near price
inline bool logOnly(const Rule& r) { return r.print == 0 && r.origin == 0 && r.nearO == 0; }

// strict full-token number
inline bool numLL(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || !e || *e || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool numD(const std::string& t, double lo, double hi, double& out) {
    if (t.empty() || t.size() > 32) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (errno || !e || *e || !finiteD(v) || v < lo || v > hi) return false;
    out = v; return true;
}
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; std::string c;
    for (char ch : s) { if (ch == d) { o.push_back(c); c.clear(); } else if (ch != '\r' && ch != '\n') c.push_back(ch); }
    o.push_back(c); return o;
}
// lsFlexLevels\LRA-LmtZones-<MKT>.csv (nightly): "VERSION,1" first, "MARKET,<MKT>", then "PARAM,<NAME>,<value>" rows. Garbled / wrong
// market / wrong version = the defaults, with the reason. Unknown names are ignored (newer nightly).
inline Rule parseParams(const std::vector<std::string>& lines, const std::string& market) {
    Rule r = ruleFor(market), z = r;
    if (lines.empty()) { r.src = "no params file: defaults"; return r; }
    const std::vector<std::string> v0 = split(lines[0], ',');
    if (v0.size() != 2 || v0[0] != "VERSION" || v0[1] != "1") { r.src = "params: bad VERSION line - defaults"; return r; }
    bool mk = false; int used = 0;
    for (size_t i = 1; i < lines.size() && i < 200; ++i) {
        const std::vector<std::string> f = split(lines[i], ',');
        if (f.size() == 2 && f[0] == "MARKET") { if (f[1] != market) { r.src = "params: MARKET " + f[1] + " is not " + market + " - defaults"; return r; } mk = true; continue; }
        if (f.size() != 3 || f[0] != "PARAM") continue;
        const std::string& n = f[1]; long long v = 0; double d = 0; bool ok = true;
        if (n == "PRINT" && numLL(f[2], 0, 2, v)) z.print = (int)v;
        else if (n == "ORIGIN" && numLL(f[2], 0, 2, v)) z.origin = (int)v;
        else if (n == "NEAR" && numLL(f[2], 0, 2, v)) z.nearO = (int)v;
        else if (n == "GAMMA_ONLY" && numLL(f[2], 0, 1, v)) z.gammaOnly = v == 1;
        else if (n == "BIDS" && numLL(f[2], 0, 1, v)) z.bids = v == 1;
        else if (n == "FAR" && numLL(f[2], 0, 1, v)) z.farOn = v == 1;
        else if (n == "ABC" && numLL(f[2], 0, 1, v)) z.abc = v == 1;
        else if (n == "S20_RTH" && numLL(f[2], 1, 100000, v)) { z.s20Rth = v; z.s20Set = true; }
        else if (n == "S20_ON" && numLL(f[2], 1, 100000, v)) { z.s20On = v; z.s20Set = true; }
        else if (n == "REST_S" && numLL(f[2], 0, 3600, v)) z.restS = (int)v;
        else if (n == "MAX_MOVES" && numLL(f[2], 0, 10, v)) z.maxMoves = (int)v;
        else if (n == "NEAR_ATR" && numD(f[2], 0.25, 5.0, d)) z.nearAtr = d;
        else if (n == "FAR_ATR" && numD(f[2], 0.5, 10.0, d)) z.farAtr = d;
        else if (n == "CONF_ATR" && numD(f[2], 0.0, 2.0, d)) z.confAtr = d;
        else if (n == "EXPIRE_MIN" && numLL(f[2], 15, 1440, v)) z.expireMin = (int)v;
        else ok = false;
        if (ok) ++used;
    }
    if (!mk) { r.src = "params: no MARKET line - defaults"; return r; }
    if (z.farAtr <= z.nearAtr) z.farAtr = z.nearAtr + 1.0;
    z.src = "params file (" + std::to_string(used) + " rows)"; return z;
}

// ---- closed bars, the causal 1-ATR zigzag and the breakouts (tags.py of the study, made incremental) ---------------------------
struct Bar { long long endMs = 0; Tick o = 0, h = 0, l = 0, c = 0; };
struct Pivot { int type = 0; Tick px = 0; int iExt = 0, iConf = 0; bool broke = false; };   // type +1 high / -1 low
struct Break { int dir = 0; Tick level = 0, close = 0; int bar = 0; long long endMs = 0; Tick origin = 0; bool haveOrigin = false; bool with = false; };

class Waves {
public:
    std::vector<Bar> bars; std::vector<Pivot> piv; std::vector<Break> brk;
    int d = 0; Tick ext = 0; int ie = 0; bool haveExt = false;
    double atr = 0;   // ticks, 14-bar ATR of closed bars including the last
    double fixedAtr = 0;   // tests only: > 0 replaces the computed ATR
    static double atrAt(const std::vector<Bar>& b, size_t i) {
        if (i < 14 || i >= b.size()) return 0;
        double s = 0;
        for (size_t k = i - 13; k <= i; ++k) { const Tick pc = b[k - 1].c; s += (double)(std::max(b[k].h, pc) - std::min(b[k].l, pc)); }
        return s / 14.0;
    }
    int trendAt(int k) const {   // +1 up (HH + HL), -1 down (LH + LL), 0 range: last 2 confirmed highs / lows with conf < k
        Tick hs[2] = { 0, 0 }, ls[2] = { 0, 0 }; int nh = 0, nl = 0;
        for (size_t i = piv.size(); i-- > 0 && (nh < 2 || nl < 2);) {
            const Pivot& p = piv[i]; if (p.iConf >= k) continue;
            if (p.type > 0 && nh < 2) hs[nh++] = p.px; else if (p.type < 0 && nl < 2) ls[nl++] = p.px;
        }
        if (nh < 2 || nl < 2) return 0;
        if (hs[0] > hs[1] && ls[0] > ls[1]) return 1;
        if (hs[0] < hs[1] && ls[0] < ls[1]) return -1;
        return 0;
    }
    void add(const Bar& b) {
        bars.push_back(b); const int i = (int)bars.size() - 1;
        atr = fixedAtr > 0 ? fixedAtr : atrAt(bars, (size_t)i);
        const double a = atr > 0 ? atr : 0;
        // breakouts against the pivots confirmed BEFORE this bar (first close beyond; colour must match, else the pivot is spent)
        for (size_t p = 0; p < piv.size(); ++p) {
            Pivot& P = piv[p]; if (P.broke || P.iConf >= i) continue;
            if (P.type > 0 && b.c > P.px) { P.broke = true; if (b.c > b.o) addBreak(+1, P.px, b, i); }
            else if (P.type < 0 && b.c < P.px) { P.broke = true; if (b.c < b.o) addBreak(-1, P.px, b, i); }
        }
        // the zigzag (1 ATR reversal), confirmed at this bar
        if (!(a > 0)) { if (!haveExt) { haveExt = true; ext = b.h; ie = i; d = 1; } return; }
        if (!haveExt) { haveExt = true; ext = b.h; ie = i; d = 1; return; }
        if (d == 1) {
            if (b.h >= ext) { ext = b.h; ie = i; }
            if ((double)b.l <= (double)ext - a) { Pivot P; P.type = 1; P.px = ext; P.iExt = ie; P.iConf = i; piv.push_back(P); d = -1; ext = b.l; ie = i; }
        } else {
            if (b.l <= ext) { ext = b.l; ie = i; }
            else if ((double)b.h >= (double)ext + a) { Pivot P; P.type = -1; P.px = ext; P.iExt = ie; P.iConf = i; piv.push_back(P); d = 1; ext = b.h; ie = i; }
        }
        if (piv.size() > 400) piv.erase(piv.begin(), piv.begin() + 100);
        if (brk.size() > 400) brk.erase(brk.begin(), brk.begin() + 100);
        if (bars.size() > 6000) trim();
    }
    // breakouts in the last `ms` before t
    std::vector<Break> recent(long long t, long long ms) const {
        std::vector<Break> o; for (size_t i = brk.size(); i-- > 0;) { if (t - brk[i].endMs > ms) break; if (brk[i].endMs <= t) o.push_back(brk[i]); }
        return o;   // newest first
    }
private:
    void addBreak(int dir, Tick level, const Bar& b, int i) {
        Break B; B.dir = dir; B.level = level; B.close = b.c; B.bar = i; B.endMs = b.endMs;
        for (size_t q = piv.size(); q-- > 0;) if (piv[q].iConf <= i && piv[q].type == (dir > 0 ? -1 : 1)) { B.origin = piv[q].px; B.haveOrigin = true; break; }
        B.with = trendAt(i) == dir;
        brk.push_back(B);
    }
    void trim() {   // keep the last 3000 bars; indexes shift
        const int cut = (int)bars.size() - 3000;
        bars.erase(bars.begin(), bars.begin() + cut);
        std::vector<Pivot> np; for (auto p : piv) { p.iExt -= cut; p.iConf -= cut; if (p.iConf >= 0) np.push_back(p); } piv.swap(np);
        std::vector<Break> nb; for (auto b : brk) { b.bar -= cut; if (b.bar >= 0) nb.push_back(b); } brk.swap(nb);
        ie -= cut; if (ie < 0) ie = 0;
    }
};

// ---- the order book (one entry per MBO order id) ------------------------------------------------------------------------------
struct Ord {
    int side = BID; Tick px = 0; long long size = 0, maxShown = 0, traded = 0;
    long long tFirst = 0, tAtPx = 0;   // first seen; when it arrived at this price (CT ms)
    int moves = 0; bool startKnown = true; bool filledLast = false; int zone = -1;
};
struct Seg { long long t = 0; Tick px = 0, lo = 0, hi = 0; };
struct Zone {
    int id = 0; long long oid = 0; int side = ASK, kind = K_NEAR;
    long long t0 = 0, t1 = 0; int end = LIVE;
    long long size0 = 0, sizeNow = 0, traded = 0, maxShown = 0;
    long long tFill0 = 0, tFill = 0; Tick fillPx = 0;
    std::vector<Seg> segs;               // the zone's price over time; segs.back() = now
    bool drawn = false, q = false, conf = false;
    Tick confPx = 0; long long s20 = 0; double atr = 0;   // atr in ticks at the start
    long long restMs = 0; bool startKnown = true; int moves = 0;
    long long boMs = 0; Tick level = 0, bclose = 0;        // PRINT / ABC: the breakout bar, the broken level, its close; ORIGIN: level = the origin
    bool restored = false;
    Tick px() const { return segs.empty() ? 0 : segs.back().px; }
};
struct Fill { int zone = 0; long long t = 0; long long lots = 0; Tick px = 0; };

struct Tally { long long made[2][K_N] = { { 0 } }, drawn = 0, ends[NEARED + 1] = { 0 }, events = 0, unknownFill = 0, bookDrop = 0; };

class Engine {
public:
    Rule R; std::string market;
    Waves W;
    std::unordered_map<long long, Ord> book;
    std::vector<Zone> zones;
    std::vector<int> live;                  // indexes of live zones
    std::set<long long> usedOid;            // an order id gets at most one zone per session
    std::vector<std::string> rows;          // record lines not yet written
    std::vector<Tick> gamma;                // gamma level ticks (MenthorQ, this contract)
    long long feedStartMs = 0, lastT = 0;   // the first event's time (the book is only known from then)
    int nextId = 1; long long session = -1;
    Tally tally; size_t bookCap = 300000; int farKeep = 1;   // far: the single biggest per side (replay HG 10-20: 2 per side = 48 far lines a day)
    long long lastBarMs = 0;

    void setGamma(const std::vector<Tick>& g) { gamma = g; std::sort(gamma.begin(), gamma.end()); }
    // one MBO event. side is used only on NEW (a FILL row carries the aggressor's side, never the order's)
    void onEvent(int act, int side, Tick px, long long size, long long oid, long long t) {
        if (t <= 0 || oid == 0) return;
        ++tally.events; if (feedStartMs == 0) feedStartMs = t; if (t > lastT) lastT = t;
        auto it = book.find(oid);
        if (act == NEW) {
            if (px <= 0 || size <= 0 || (side != BID && side != ASK)) return;
            if (it != book.end()) { it->second.px = px; it->second.size = size; it->second.tAtPx = t; return; }
            if (book.size() >= bookCap) prune();
            Ord o; o.side = side; o.px = px; o.size = size; o.maxShown = size; o.tFirst = o.tAtPx = t; book[oid] = o; return;
        }
        if (act == MOD) {
            if (px <= 0) return;
            if (it == book.end()) {   // rested before the chart opened: the age is NOT known (counted from now)
                if (size <= 0 || (side != BID && side != ASK)) return;
                if (book.size() >= bookCap) prune();
                Ord o; o.side = side; o.px = px; o.size = size; o.maxShown = size; o.tFirst = o.tAtPx = t; o.startKnown = false; book[oid] = o; return;
            }
            Ord& o = it->second;
            if (px != o.px) {
                o.moves++; o.px = px; o.tAtPx = t; o.size = size; o.filledLast = false;
                if (o.zone >= 0) zoneMoved(o.zone, o, t);
            } else { o.size = size; o.filledLast = false; }
            if (size > o.maxShown) o.maxShown = size;
            return;
        }
        if (act == FILL) {
            if (it == book.end()) { ++tally.unknownFill; return; }
            Ord& o = it->second; if (size <= 0) return;
            o.traded += size; o.size = std::max(0LL, o.size - size); o.filledLast = true;
            if (o.zone >= 0) { Zone& z = zones[(size_t)o.zone]; if (z.end == LIVE) {
                z.traded += size; if (!z.tFill0) z.tFill0 = t; z.tFill = t; z.fillPx = px > 0 ? px : o.px;
                char b[160]; std::snprintf(b, sizeof(b), "F|%d|%lld|%lld|%lld", z.id, t, z.traded, (long long)z.fillPx); rows.push_back(b); } }
            return;
        }
        if (act == DEL) {
            if (it == book.end()) return;
            Ord& o = it->second;
            if (o.zone >= 0) { Zone& z = zones[(size_t)o.zone]; if (z.end == LIVE) finish(o.zone, o.size <= 0 ? FILLED : PULLED, t); }
            book.erase(it);
        }
    }
    // the bar has CLOSED: zones end / follow, then new zones. s20 = the size tier in force at this bar's end (lots).
    void closeBar(const Bar& b, long long s20) {
        if (b.endMs <= lastBarMs) return;   // once per bar, in order
        lastBarMs = b.endMs;
        W.add(b);
        const double a = W.atr;
        if (!(a > 0)) return;
        const long long t = b.endMs;
        // 1) the price / time ends of live zones
        for (size_t k = 0; k < live.size(); ++k) {
            Zone& z = zones[(size_t)live[k]]; if (z.end != LIVE || z.t0 >= t) continue;   // (a restored zone never judged on a bar before its start)
            const Tick p = z.px(); const double dist = z.side == ASK ? (double)(p - b.c) : (double)(b.c - p);   // > 0 = still on its side
            if (-dist > R.brokenAtr * a) { finish(live[k], BROKEN, t); continue; }
            if (t - z.t0 > (long long)R.expireMin * 60000LL) { finish(live[k], EXPIRED, t); continue; }
            if (z.kind == K_FAR && dist > (R.farAtr + 0.5) * a) { finish(live[k], LEFT, t); continue; }
            if (z.kind == K_FAR && dist > 0 && dist <= R.nearAtr * a) { finish(live[k], NEARED, t); continue; }   // the near rules take it
        }
        compact();
        // 2) new zones: only once the book has been watched long enough to know an order rested
        if (feedStartMs == 0 || t < feedStartMs + (long long)R.restS * 1000LL + 30000LL) return;
        const long long restMs = (long long)R.restS * 1000LL;
        struct C { long long oid; const Ord* o; double dist; int kind; Break br; bool hasBr; };
        std::vector<C> nearC[2], farC[2];
        const std::vector<Break> rb = W.recent(t, 3LL * 3600000LL);
        for (auto& kv : book) {
            const Ord& o = kv.second;
            if (o.zone >= 0 || o.size < s20 || o.moves > R.maxMoves || t - o.tAtPx < restMs || usedOid.count(kv.first)) continue;
            const double dist = o.side == ASK ? (double)(o.px - b.c) : (double)(b.c - o.px);
            if (!(dist > 0)) continue;
            C c; c.oid = kv.first; c.o = &o; c.dist = dist; c.kind = K_NEAR; c.hasBr = false;
            if (dist > R.farAtr * a) continue;
            if (dist > R.nearAtr * a) { c.kind = K_FAR; farC[o.side].push_back(c); continue; }
            // the case tag (study order: a retest of a recent breakout print first, then the wave origin)
            const int want = o.side == ASK ? -1 : +1;
            for (size_t i = 0; i < rb.size() && !c.hasBr; ++i) {
                const Break& e = rb[i]; if (e.dir != want) continue;
                const bool in = want < 0 ? ((double)o.px >= (double)e.close - 0.1 * a && (double)o.px <= (double)e.level + 0.25 * a)
                                         : ((double)o.px >= (double)e.level - 0.25 * a && (double)o.px <= (double)e.close + 0.1 * a);
                if (in) { c.kind = e.with ? K_PRINT : K_ABC; c.br = e; c.hasBr = true; }
            }
            for (size_t i = 0; i < rb.size() && !c.hasBr; ++i) {
                const Break& e = rb[i]; if (e.dir != want || !e.haveOrigin) continue;
                if (std::fabs((double)(o.px - e.origin)) <= 0.25 * a) { c.kind = K_ORIGIN; c.br = e; c.hasBr = true; }
            }
            nearC[o.side].push_back(c);
        }
        auto bigger = [](const C& x, const C& y) { return x.o->size != y.o->size ? x.o->size > y.o->size : x.dist != y.dist ? x.dist < y.dist : x.oid < y.oid; };
        for (int s = 0; s < 2; ++s) {
            std::sort(nearC[s].begin(), nearC[s].end(), bigger);
            std::sort(farC[s].begin(), farC[s].end(), bigger);
            bool have[K_N] = { false, false, false, false, false }; int nFar = 0;
            for (size_t k = 0; k < live.size(); ++k) { const Zone& z = zones[(size_t)live[k]]; if (z.side != s) continue; if (z.kind == K_FAR) ++nFar; else have[z.kind] = true; }
            // the biggest near order -> its own kind; then the biggest of each tagged kind not yet live
            if (!nearC[s].empty()) { const C& c = nearC[s][0]; const int k = c.kind; if (!have[k]) { start(c.oid, *c.o, k, c.hasBr ? &c.br : nullptr, t, s20, a, restMs); have[k] = true; } }
            for (int k = K_PRINT; k <= K_ORIGIN; ++k) {
                if (have[k]) continue;
                for (size_t i = 0; i < nearC[s].size(); ++i) if (nearC[s][i].kind == k && book.count(nearC[s][i].oid) && book[nearC[s][i].oid].zone < 0) {
                    const C& c = nearC[s][i]; start(c.oid, *c.o, k, &c.br, t, s20, a, restMs); have[k] = true; break; }
            }
            for (size_t i = 0; i < farC[s].size() && nFar < farKeep; ++i) { const C& c = farC[s][i]; start(c.oid, *c.o, K_FAR, nullptr, t, s20, a, restMs); ++nFar; }
        }
    }
    void endSession(long long t) {
        for (size_t k = 0; k < live.size(); ++k) if (zones[(size_t)live[k]].end == LIVE) finish(live[k], SESSION, t);
        compact();
    }
    void clearSession() {
        for (auto& kv : book) kv.second.zone = -1;
        zones.clear(); live.clear(); usedOid.clear(); nextId = 1;
    }
    // ---- the session record -----------------------------------------------------------------------------------------------------
    static const char* header() { return "Z|id|oid|side|kind|t0|px|lo|hi|size|drawn|q|conf|s20|atr10|bo_ms|level|close|rest_s|start_known|ver  /  M|id|t|px|lo|hi  /  F|id|t|traded|px  /  E|id|t|end|traded"; }
    static std::string zRow(const Zone& z) {
        const Seg& g = z.segs.front(); char b[400];
        std::snprintf(b, sizeof(b), "Z|%d|%lld|%d|%s|%lld|%lld|%lld|%lld|%lld|%d|%d|%d|%lld|%lld|%lld|%lld|%lld|%lld|%d|%s", z.id, z.oid, z.side, kindCode(z.kind), z.t0,
            (long long)g.px, (long long)g.lo, (long long)g.hi, z.size0, z.drawn ? 1 : 0, z.q ? 1 : 0, z.conf ? 1 : 0, z.s20, (long long)std::llround(z.atr * 10.0),
            z.boMs, (long long)z.level, (long long)z.bclose, z.restMs / 1000, z.startKnown ? 1 : 0, VERSION);
        return b;
    }
    // read back after a restart; live zones are re-linked to their order id (the book learns the order again from its next event)
    size_t restore(const std::vector<std::string>& in, long long* bad = nullptr) {
        size_t n = 0; long long nb = 0; std::map<int, int> idx;
        for (size_t i = 0; i < in.size(); ++i) {
            const std::vector<std::string> f = split(in[i], '|');
            if (f.empty() || f[0].empty()) continue;
            long long v[20] = { 0 };
            if (f[0] == "Z" && f.size() == 21) {
                bool ok = true; for (int k = 1; k <= 3 && ok; ++k) ok = numLL(f[(size_t)k], -1, 9223372036854775807LL, v[k]);
                for (int k = 5; k <= 19 && ok; ++k) ok = numLL(f[(size_t)k], -9223372036854775807LL, 9223372036854775807LL, v[k]);
                const int kind = kindOf(f[4]);
                if (!ok || kind < 0 || (v[3] != BID && v[3] != ASK) || v[9] <= 0 || idx.count((int)v[1])) { ++nb; continue; }
                Zone z; z.id = (int)v[1]; z.oid = v[2]; z.side = (int)v[3]; z.kind = kind; z.t0 = v[5];
                Seg g; g.t = v[5]; g.px = v[6]; g.lo = v[7]; g.hi = v[8]; z.segs.push_back(g);
                z.size0 = z.sizeNow = z.maxShown = v[9]; z.drawn = v[10] == 1; z.q = v[11] == 1; z.conf = v[12] == 1; z.s20 = v[13]; z.atr = (double)v[14] / 10.0;
                z.boMs = v[15]; z.level = v[16]; z.bclose = v[17]; z.restMs = v[18] * 1000; z.startKnown = v[19] == 1; z.restored = true;
                idx[z.id] = (int)zones.size(); zones.push_back(z); usedOid.insert(z.oid); nextId = std::max(nextId, z.id + 1); ++n;
            } else if ((f[0] == "M" && f.size() == 6) || (f[0] == "F" && f.size() == 5) || (f[0] == "E" && f.size() == 5)) {
                long long id = 0, t = 0; if (!numLL(f[1], 0, 2000000000, id) || !numLL(f[2], 0, 9223372036854775807LL, t) || !idx.count((int)id)) { ++nb; continue; }
                Zone& z = zones[(size_t)idx[(int)id]];
                if (f[0] == "M") { long long p = 0, lo = 0, hi = 0; if (!numLL(f[3], -9223372036854775807LL, 9223372036854775807LL, p) || !numLL(f[4], -9223372036854775807LL, 9223372036854775807LL, lo) || !numLL(f[5], -9223372036854775807LL, 9223372036854775807LL, hi) || z.end != LIVE) { ++nb; continue; }
                    Seg g; g.t = t; g.px = p; g.lo = lo; g.hi = hi; z.segs.push_back(g); z.moves++; }
                else if (f[0] == "F") { long long tr = 0, p = 0; if (!numLL(f[3], 0, 100000000, tr) || !numLL(f[4], -9223372036854775807LL, 9223372036854775807LL, p)) { ++nb; continue; }
                    if (!z.tFill0) { z.tFill0 = t; }
                    z.tFill = t; z.traded = std::max(z.traded, tr); z.fillPx = p; }
                else { const int e = endOf(f[3]); long long tr = 0; if (e <= LIVE || z.end != LIVE || !numLL(f[4], 0, 100000000, tr)) { ++nb; continue; }
                    z.end = e; z.t1 = t; z.traded = std::max(z.traded, tr); }
            } else ++nb;
        }
        live.clear(); usedOid.clear();
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].end != NEARED) usedOid.insert(zones[i].oid);
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].end == LIVE) {
            live.push_back((int)i); Zone& z = zones[i];
            auto it = book.find(z.oid);
            if (it == book.end()) { Ord o; o.side = z.side; o.px = z.px(); o.size = z.sizeNow; o.maxShown = z.maxShown; o.tFirst = z.t0 - z.restMs; o.tAtPx = z.segs.back().t; o.startKnown = false; o.moves = z.moves; o.zone = (int)i; book[z.oid] = o; }
            else it->second.zone = (int)i;
        }
        if (bad) *bad = nb;
        return n;
    }
    // ---- reads for the host -----------------------------------------------------------------------------------------------------
    // LMT F: every zone order (drawn or log-only) that traded, filled within the last `ms` (or still live)
    std::vector<int> filledZones(long long now, long long ms) const {
        std::vector<int> o; for (size_t i = 0; i < zones.size(); ++i) { const Zone& z = zones[i]; if (z.traded > 0 && (z.end == LIVE || now - z.tFill <= ms)) o.push_back((int)i); }
        return o;
    }
    long long liveSize(const Zone& z) const { auto it = book.find(z.oid); return it == book.end() ? z.sizeNow : it->second.size; }
private:
    void prune() {   // the book is full: drop the smallest half of the orders that hold no zone (counted)
        std::vector<long long> sz; sz.reserve(book.size()); for (auto& kv : book) if (kv.second.zone < 0) sz.push_back(kv.second.size);
        if (sz.empty()) return;
        std::nth_element(sz.begin(), sz.begin() + sz.size() / 2, sz.end()); const long long cut = sz[sz.size() / 2];
        for (auto it = book.begin(); it != book.end();) { if (it->second.zone < 0 && it->second.size <= cut) { it = book.erase(it); ++tally.bookDrop; } else ++it; }
    }
    void compact() { std::vector<int> n; for (size_t k = 0; k < live.size(); ++k) if (zones[(size_t)live[k]].end == LIVE) n.push_back(live[k]); live.swap(n); }
    Tick halfBand(double a) const { return std::max<Tick>(1, (Tick)std::llround(0.05 * a)); }
    int drawRule(int side, int kind, bool conf, bool& q) const {
        q = false;
        if (side == BID && !R.bids) return 0;
        if (kind == K_FAR) return R.farOn ? 1 : 0;
        if (kind == K_ABC) return R.abc ? 1 : 0;
        const int v = kind == K_PRINT ? R.print : kind == K_ORIGIN ? R.origin : R.nearO;
        if (v == 0) return 0;
        if (R.gammaOnly && !conf) return 0;
        q = v == 2; return 1;
    }
    void start(long long oid, const Ord& o, int kind, const Break* br, long long t, long long s20, double a, long long restMs) {
        Zone z; z.id = nextId++; z.oid = oid; z.side = o.side; z.kind = kind; z.t0 = t;
        z.size0 = z.sizeNow = o.size; z.maxShown = o.maxShown; z.traded = 0; z.s20 = s20; z.atr = a; z.restMs = t - o.tAtPx; z.startKnown = o.startKnown; z.moves = o.moves;
        const Tick hb = halfBand(a); Seg g; g.t = t; g.px = o.px; g.lo = o.px - hb; g.hi = o.px + hb;
        if (br && (kind == K_PRINT || kind == K_ABC)) { z.boMs = br->endMs; z.level = br->level; z.bclose = br->close;
            g.lo = std::min(g.lo, std::min(br->level, br->close)); g.hi = std::max(g.hi, std::max(br->level, br->close)); }
        if (br && kind == K_ORIGIN) { z.boMs = br->endMs; z.level = br->origin; }
        for (size_t i = 0; i < gamma.size(); ++i) if (std::fabs((double)(gamma[i] - o.px)) <= R.confAtr * a + 1e-9) { z.conf = true; z.confPx = gamma[i]; break; }
        bool q = false; z.drawn = drawRule(z.side, kind, z.conf, q) == 1; z.q = q;
        z.segs.push_back(g);
        const int idx = (int)zones.size(); zones.push_back(z); live.push_back(idx); usedOid.insert(oid);
        book[oid].zone = idx;
        tally.made[z.side][kind]++; if (z.drawn) tally.drawn++;
        rows.push_back(zRow(z));
        (void)restMs;
    }
    void finish(int idx, int how, long long t) {
        Zone& z = zones[(size_t)idx]; if (z.end != LIVE) return;
        z.end = how; z.t1 = t;
        auto it = book.find(z.oid); if (it != book.end()) { z.sizeNow = it->second.size; it->second.zone = -1; }
        tally.ends[how]++;
        if (how == NEARED) usedOid.erase(z.oid);   // a far order that came within 2 ATR may become a near zone (one live zone per order)
        char b[160]; std::snprintf(b, sizeof(b), "E|%d|%lld|%s|%lld", z.id, t, endCode(how), z.traded); rows.push_back(b);
    }
    void zoneMoved(int idx, const Ord& o, long long t) {
        Zone& z = zones[(size_t)idx]; if (z.end != LIVE) return;
        z.moves++;
        const Seg& g = z.segs.back();
        const bool inBand = o.px >= g.lo && o.px <= g.hi;
        if (o.moves > R.maxMoves || !inBand) { finish(idx, MOVED, t); return; }
        Seg n = g; const Tick hb = halfBand(z.atr);
        if (z.kind == K_PRINT || z.kind == K_ABC) { n.lo = std::min(g.lo, o.px - hb); n.hi = std::max(g.hi, o.px + hb); } else { n.lo = o.px - hb; n.hi = o.px + hb; }
        n.t = t; n.px = o.px; z.segs.push_back(n);
        char b[200]; std::snprintf(b, sizeof(b), "M|%d|%lld|%lld|%lld|%lld", z.id, t, (long long)n.px, (long long)n.lo, (long long)n.hi); rows.push_back(b);
    }
};

// ---- text ---------------------------------------------------------------------------------------------------------------------
inline int decimals(double tick) {
    if (!(tick > 0) || !finiteD(tick)) return 2;
    for (int d = 0; d <= 6; ++d) { const double p = std::pow(10.0, d), v = tick * p; if (std::fabs(v - std::round(v)) < 1e-7) return d; }
    return 6;
}
inline std::string price(Tick t, double tick) { char b[48]; std::snprintf(b, sizeof(b), "%.*f", decimals(tick), (double)t * tick); return b; }
inline std::string clock12(long long ms) {
    if (ms <= 0) return "?";
    long long s = (ms / 1000) % 86400; if (s < 0) s += 86400;
    int h = (int)(s / 3600), m = (int)(s / 60 % 60); const bool pm = h >= 12; h %= 12; if (h == 0) h = 12;
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h, m, pm ? "PM" : "AM"); return b;
}
inline std::string dur(long long ms) {
    if (ms < 0) { ms = 0; }
    const long long s = ms / 1000; char b[32];
    if (s < 60) std::snprintf(b, sizeof(b), "%lld s", s); else if (s < 3600) std::snprintf(b, sizeof(b), "%lld min", s / 60);
    else std::snprintf(b, sizeof(b), "%lld h %lld min", s / 3600, s / 60 % 60);
    return b;
}
// the chart label: short, "?" while LEARNING
inline std::string label(const Zone& z) {
    std::string k;
    if (z.kind == K_PRINT || z.kind == K_ABC) k = z.side == ASK ? "BD print" : "BO print";
    else if (z.kind == K_ORIGIN) k = "Origin";
    else if (z.kind == K_NEAR) k = z.side == ASK ? "Offer" : "Bid";
    if (z.kind == K_ABC) k = "ABC";
    std::string s = z.kind == K_FAR ? std::to_string(z.size0) : k + " " + std::to_string(z.size0);
    if (z.q) s += "?";
    return s;
}
// the study numbers for the hover (PULLBACK-ZONES-FINAL-2026-10-22, lift = order hold % minus random-price hold %, in points)
inline std::string studyText(const std::string& m, const Zone& z) {
    if (z.side == BID) {
        if (z.kind == K_FAR) return m == "GC" ? "Far bids GC: +16.9 vs random (n 25, later half +7.2) - watch list, not proven." : "Far bids, all markets: +3.8 vs random (n 232) - no edge; a map of big size only.";
        if (z.kind == K_PRINT) return "Bids at a with-trend BO print: -11.8 vs random (n 81) - WORSE. Shown in LMT / LMT F only.";
        return "Bids within 2 ATR, all markets: 52.7% held vs 60.2% random (-7.4, n 427). Not a zone - LMT / LMT F only.";
    }
    switch (z.kind) {
    case K_PRINT: return m == "ES" ? "ES BD-print offers: +28.4 vs random (n 11) LEARNING. All markets: 77% held vs 62% (+14.8, n 52)."
                                   : "With-trend BD-print offers, all markets: 77% held vs 62% random (+14.8, range +1..+29, n 52). LEARNING.";
    case K_ABC: return "Internal (ABC) break offers: -5.3 vs random (n 68) - no edge, it is just a pullback. Grey = context only.";
    case K_ORIGIN: return m == "NG" ? "Wave-origin offers: +7.7 vs random (n 68), no proven edge yet; NG +43.7 (n 14) LEARNING."
                                    : "Wave-origin offers, all markets: 62% held vs 55% random (+7.7, range -4..+20, n 68) - no proven edge yet.";
    case K_FAR: return m == "NQ" ? "Far offers NQ: +27.1 vs random (n 15) LEARNING; all markets +2.5 (n 275) - a map, no edge." : "Far offers (2-3 ATR), all markets: +2.5 vs random (n 275) - no edge; a map of where big size rests.";
    default: break;
    }
    if (m == "NG") return "NG offers within 2 ATR: +15.4 vs random (range +3..+28, n 98, later half +6.7). EDGE.";
    if (m == "HG") return "HG offers within 2 ATR: +14.6 vs random (range +2.5..+29, n 94, later half +15.1). EDGE.";
    if (m == "NQ") return "NQ offers: +27.4 vs random (n 25, later half +44.6, n 10) - 6 sessions only, LEARNING.";
    if (m == "CL") return "CL offers ON a gamma level: +32.2 vs random (range +8.5..+49, n 36, later half +31.7). Alone: +2.9 (n 99).";
    if (m == "GC") return "GC offers: +13.5 vs random (range -0.4..+39, n 32) - not proven. Log only.";
    if (m == "ES") return "ES offers within 2 ATR alone: -1.9 vs random (n 89) - no edge. Log only.";
    if (m == "EU") return "6E offers: +7.8 vs random (range -12..+26, n 30) - no edge. Log only.";
    return "Offers within 2 ATR, all markets: 65.0% held vs 56.5% random (+8.5, range +3.4..+13.5, n 431).";
}
struct Row { std::string text; int ink = INK_WHITE; };
struct HoverCtx { std::string market; double tick = 0.25; long long nowMs = 0; long long sizeLive = 0; };
// <= 12 rows: question / what it is / why it matters / how to use / study / example / now
inline std::vector<Row> hoverRows(const Zone& z, const HoverCtx& c) {
    std::vector<Row> v; const bool ask = z.side == ASK; const int ink = z.kind == K_ABC ? INK_GREY : ask ? INK_RED : INK_GREEN;
    const std::string who = ask ? "seller" : "buyer", pxs = price(z.px(), c.tick);
    const char* nm[K_N] = { ask ? "BD print zone" : "BO print zone", "ABC break (context)", "Origin zone", ask ? "Real offer" : "Real bid", ask ? "Far offer (map)" : "Far bid (map)" };
    std::string title = std::string(nm[z.kind >= 0 && z.kind < K_N ? z.kind : K_NEAR]) + ": " + std::to_string(z.size0) + " lots @ " + pxs
        + (z.q ? "  ? LEARNING" : "") + (z.drawn ? "" : "  (log only)");
    v.push_back(Row{ title, ink });
    if (z.kind == K_FAR) v.push_back(Row{ "Question: where is the next big real " + who + " if price keeps going?", INK_WHITE });
    else if (z.kind == K_ABC) v.push_back(Row{ "Question: is this break just a pullback inside the bigger wave?", INK_WHITE });
    else v.push_back(Row{ std::string("Question: is a real ") + who + " waiting here, so a " + (ask ? "rally / retest can fail" : "pullback can hold") + "?", INK_WHITE });
    std::string what = "What: ONE resting " + std::string(ask ? "sell" : "buy") + " order of " + std::to_string(z.size0) + " lots (Big Lot S20 " + std::to_string(z.s20) + "), rested "
        + (z.startKnown ? "" : "at least ") + dur(z.restMs) + ", moved " + std::to_string(z.moves) + "x.";
    v.push_back(Row{ what, INK_WHITE });
    if (z.kind == K_PRINT) v.push_back(Row{ "It sits where the with-trend " + std::string(ask ? "breakdown" : "breakout") + " printed (" + clock12(z.boMs) + ", level " + price(z.level, c.tick) + ").", INK_WHITE });
    else if (z.kind == K_ABC) v.push_back(Row{ "The break at " + clock12(z.boMs) + " went AGAINST the trend (an ABC inside the wave).", INK_WHITE });
    else if (z.kind == K_ORIGIN) v.push_back(Row{ "It sits at the origin of the breakdown wave (" + price(z.level, c.tick) + ").", INK_WHITE });
    v.push_back(Row{ "Why: a real order that rested 30 s+ and is not pulled is the reason price can stop; pulled ones = random.", INK_WHITE });
    if (z.conf) v.push_back(Row{ "Diamond: also on a gamma level (" + price(z.confPx, c.tick) + "): offers there held 71% vs 60% (+10.5, n 147).", INK_YELLOW });
    v.push_back(Row{ "How: WAIT for the close back (reversal bar / CoB), stop 1 tick beyond the pivot. Never at the touch:", INK_WHITE });
    v.push_back(Row{ "   touch-close entries were random (-0.8R, n 171). X = the order was pulled: the zone is void.", INK_WHITE });
    v.push_back(Row{ "Study: " + studyText(c.market, z), INK_WHITE });
    if (z.kind == K_PRINT) v.push_back(Row{ "Example: ES Oct 1 9:41 - retest stopped at 7707.75, 1 tick under a 100-lot offer at 7708; then -35 pts.", INK_WHITE });
    else if (z.kind == K_ORIGIN) v.push_back(Row{ "Example: ES Oct 1 10:27 - held at the 100-lot offer at 7698.75 (origin of wave 2).", INK_WHITE });
    else if (z.kind != K_ABC) v.push_back(Row{ "Example: ES Oct 1 12:31 - 200 lots of offers at 7728 / 7733 filled; wick 7737, closed back 7728.50.", INK_WHITE });
    std::string now;
    if (z.end == LIVE) now = "Now: resting " + std::to_string(c.sizeLive) + " lots" + (z.traded > 0 ? ", filled " + std::to_string(z.traded) + " (LMT F)" : std::string()) + ", since " + clock12(z.t0) + ".";
    else now = std::string("Ended ") + clock12(z.t1) + ": " + (z.end == FILLED ? "FILLED (ring)" : z.end == PULLED ? "PULLED (X) - void" : z.end == MOVED ? "MOVED away (X) - void"
        : z.end == BROKEN ? "price closed 1 ATR through it" : z.end == EXPIRED ? "4 h old" : z.end == NEARED ? "price came within 2 ATR" : z.end == LEFT ? "price moved away" : "session end")
        + (z.traded > 0 ? ", filled " + std::to_string(z.traded) + " lots" : std::string()) + ".";
    v.push_back(Row{ now, INK_WHITE });
    if (v.size() > 12) v.resize(12);
    for (size_t i = 0; i < v.size(); ++i) if (v[i].text.size() > 118) v[i].text = v[i].text.substr(0, 115) + "...";
    return v;
}

// ---- the per-market settings file: lsFlexLevels\LiquidityProfile.lmtzones-<MKT>.txt -----------------------------------------
// "LMZ1,on,print,origin,near,far,abc,gamma_lines,diamond,offer_rgb,bid_rgb,grey_rgb,gamma_rgb" (colours as decimal 0xRRGGBB)
static const long long C_OFFER = 0xEF4444, C_BID = 0x22C55E, C_GREYZ = 0x8A8F98, C_GAMMA = 0x6B7280;   // IRT COLOR = 0xRRGGBB (as C_BUY 0x22C55E)
static const long long C_DIAMOND = 0xFACC15;
struct Settings { bool on = true, print = true, origin = true, nearO = true, farOn = true, abc = true, gammaLines = true, diamond = true;
    long long cOffer = C_OFFER, cBid = C_BID, cGrey = C_GREYZ, cGamma = C_GAMMA; bool ok = false; std::string why; };
inline bool parseSettings(const std::string& line, Settings& out) {
    const std::vector<std::string> f = split(line, ',');
    if (f.size() != 13 || f[0] != "LMZ1") return false;
    Settings s; long long v = 0; bool* b[8] = { &s.on, &s.print, &s.origin, &s.nearO, &s.farOn, &s.abc, &s.gammaLines, &s.diamond };
    for (int i = 0; i < 8; ++i) { if (!numLL(f[(size_t)i + 1], 0, 1, v)) return false; *b[i] = v == 1; }
    long long* c[4] = { &s.cOffer, &s.cBid, &s.cGrey, &s.cGamma };
    for (int i = 0; i < 4; ++i) { if (!numLL(f[(size_t)i + 9], 0, 0xFFFFFF, v)) return false; *c[i] = v; }
    s.ok = true; out = s; return true;
}
inline std::string settingsLine(const Settings& s) {
    char b[200]; std::snprintf(b, sizeof(b), "LMZ1,%d,%d,%d,%d,%d,%d,%d,%d,%lld,%lld,%lld,%lld", s.on, s.print, s.origin, s.nearO, s.farOn, s.abc, s.gammaLines, s.diamond, s.cOffer, s.cBid, s.cGrey, s.cGamma);
    return b;
}
// does the chart's setting let this zone be drawn (the market rule already decided z.drawn)
inline bool shown(const Zone& z, const Settings& s) {
    if (!s.on || !z.drawn) return false;
    switch (z.kind) { case K_PRINT: return s.print; case K_ABC: return s.abc; case K_ORIGIN: return s.origin; case K_FAR: return s.farOn; default: return s.nearO; }
}
} // namespace lmz
#endif
