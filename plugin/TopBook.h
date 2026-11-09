#ifndef LS_TOPBOOK_H
#define LS_TOPBOOK_H
// TopBook.h 1.0.0 (2026-11-09, Rassul 12:55 CT "do it" on the top 3 bids / top 3 offers with the fake / market-maker filters;
// mockup claude.ai/artifact/E9QjUViBERJrEpJruVPKie, real ES book Nov 6 09:45 CT). SDK-free, C++11, no files, no clocks of its own.
// THE NATIVE TOP BOOK for the Limit Profile's Liquidity column: fed every market-by-order event (lsBookFlow's order feed), asked once a
// second for the 3 biggest REAL resting bid levels below price and the 3 biggest REAL offer levels above it.
//   REAL lots at a price = the orders that rested there REST_MS (30 s) or more (an order's clock restarts when it moves to a new price)
//     and did NOT re-price QUOTE_MOVES (2) or more times in the last 5 min (quoting / market-maker size follows price)
//   SKIP the first INSIDE_SKIP (2) price levels of each side (the best bid / offer and the next tick = market-maker quotes)
//   REACH = REACH_ATR (3) x the 3-min ATR (the engine's own, from its fills) from the inside
//   BIG ENOUGH = real lots >= NORM_X (2.0x) the median real level of that side within the reach (the liquidity-analyst measured the
//     hold rate flat from 1.5x to 3x: 2.0x is a screen budget, not an edge)
//   PULLED ON APPROACH = a SHOWN level that lost >= 50% of its real lots by cancels (not fills) while the inside was within APPROACH (3)
//     ticks -> dropped at that price for the rest of the session (a reset / reconnect clears the list)
//   MOSTLY ONE ORDER = the biggest single order >= 70% of the level: NOT removed; flagged "?" (measured: pulled less, held no better -
//     "may not reload")
//   S20 TAG = the level holds one order >= the market's Big Lot S20, never re-priced, resting >= 2 min (the tested LMT order); ranking
//     stays by size, the tag shows on the row ("172 S20"); GC / 6E never tagged (the study: no reaction / invalid null)
//   STEADY = one Top10Stable board per side, n = 3 (in after 30 s + 1.20x the weakest, out 10 s after it is gone, swap at 1.25x)
//   HELD / BROKE (log only, never drawn): a shown level the inside reached; HELD = price moved HELD_ATR (0.5 ATR) back away while it was
//     still resting before going BROKE_ATR (0.25 ATR) through it; a refill alone is never a hold. PULLED_WHILE_HIT = its defenders
//     cancelled >= 30% of it while it was being filled (the measured break warning, 78% vs 54%) - counted / logged only.
// No look-ahead: every call uses only the events fed so far. Bounded: MAX_ORDERS orders (beyond = the oldest-touched dropped and counted),
// the evaluation walks the order table once (<= ~50k orders, well under 1 ms).
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <algorithm>
#include <cmath>
#include <string>
#include <cstdio>
#include "Top10Stable.h"
namespace tbk {
static const char* const VERSION = "1.0.0";
enum Act { ADD = 0, MOD = 1, DEL = 2, FILL = 3, CLEAR = 9 };
struct Params {
    long long restMs = 30000; int quoteMoves = 2; long long quoteWinMs = 300000; int insideSkip = 2; double reachAtr = 3.0; double normX = 2.0;
    int approach = 3; double pulledFrac = 0.5; double singleShare = 0.70; double heldAtr = 0.5, brokeAtr = 0.25; double pullHitFrac = 0.30;
    int perSide = 3; size_t maxOrders = 400000; long long barMs = 180000; int atrBars = 14; double atrDefaultTicks = 20;
    // the LMT order (INSTITUTIONAL-ORDERS-LMT-2026-10-22: one order >= the market's Big Lot S20, never re-priced, resting >= 2 min, still
    // there when price arrives -> turn AT the order 37% vs 27% random); s20 = 0 -> no tag (GC: not reaction levels; 6E: S20 = quote size)
    long long s20Rth = 0, s20On = 0; long long stationaryMs = 120000; bool rth = true;
};
// the per-market Big Lot S20 of the resting-order pool (RTH / overnight medians of the 2026-10-22 study); GC and 6E carry no tag
inline void s20For(const std::string& m, Params& p) {
    struct R { const char* m; long long rth, on; };
    static const R T[] = { {"ES", 45, 25}, {"NQ", 20, 15}, {"CL", 15, 20}, {"HG", 5, 5}, {"NG", 10, 10}, {"GC", 0, 0}, {"EU", 0, 0}, {"6E", 0, 0} };
    p.s20Rth = p.s20On = 0; for (const R& r : T) if (m == r.m) { p.s20Rth = r.rth; p.s20On = r.on; } }
struct In { int act = ADD; int side = 0; long long px = 0; long long sz = 0; long long oid = 0; long long tMs = 0; };   // side 0 bid / 1 offer, px in ticks
struct Lvl { int side = 0; long long px = 0; long long lots = 0; int orders = 0; long long big = 0; double x = 0; int rank = 0; bool single = false; int dist = 0; bool s20 = false; int why = 0; };   // why (removed list only): 1 inside 2, 2 young, 3 quoting, 4 pulled earlier
struct Why { int inside = 0, young = 0, quoting = 0, small = 0, pulled = 0, kept = 0; };
struct Outcome { int side; long long px; long long lots; bool held; bool pulledWhileHit; long long tTouch, tEnd; };
class Engine {
public:
    explicit Engine(const Params& p = Params()) : P(p) { boardsInit(); }
    void setRth(bool on) { P.rth = on; }
    void setS20(long long rthLots, long long onLots) { P.s20Rth = rthLots; P.s20On = onLots; }
    const Params& params() const { return P; }
    // ---- the feed -------------------------------------------------------------------------------------------------------------
    void feed(const In& e) {
        ++events; lastT = std::max(lastT, e.tMs);
        switch (e.act) {
        case ADD: add(e); break;
        case MOD: mod(e); break;
        case DEL: end(e.oid, true); break;
        case FILL: fill(e); break;
        case CLEAR: reset(); break;
        default: ++garbage; break;
        }
    }
    void reset() { O.clear(); banned.clear(); cxl[0].clear(); cxl[1].clear(); fills[0].clear(); fills[1].clear(); boardsInit(); shown.clear(); watch.clear(); bars.clear(); lastPx = 0; ++resets; }
    // ---- once a second: the steady top 3 per side ---------------------------------------------------------------------------------
    std::vector<Lvl> evaluate(long long now) {
        ++evals; why = Why(); cands.clear(); removed.clear();
        if (lastPx <= 0 || O.empty()) { shown.clear(); return shown; }
        // the inside: best bid at / below the last trade, best offer at / above it (a missed delete never crosses the book this way)
        long long bb = 0, ba = 0;
        for (const auto& kv : O) { const Ord& o = kv.second; if (o.sz <= 0) continue;
            if (o.side == 0 && o.px <= lastPx && (bb == 0 || o.px > bb)) bb = o.px;
            if (o.side == 1 && o.px >= lastPx && (ba == 0 || o.px < ba)) ba = o.px; }
        insideB = bb; insideA = ba;
        const double atr = atrTicks(); atrNow = atr; const long long reach = (long long)std::llround(P.reachAtr * atr);
        struct Agg { long long real = 0, young = 0, quote = 0, big = 0; int n = 0; bool s20 = false; };
        std::unordered_map<long long, Agg> A; A.reserve(512);
        for (const auto& kv : O) { const Ord& o = kv.second; if (o.sz <= 0) continue;
            const long long ins = o.side == 0 ? bb : ba; if (ins <= 0) continue;
            const long long d = o.side == 0 ? ins - o.px : o.px - ins; if (d < 0 || d > reach) continue;
            Agg& g = A[key(o.side, o.px)];
            if (now - o.tRest < P.restMs) { g.young += o.sz; continue; }
            int mv = 0; for (long long t : o.moves) if (now - t <= P.quoteWinMs) ++mv;
            if (mv >= P.quoteMoves) { g.quote += o.sz; continue; }
            g.real += o.sz; ++g.n; g.big = std::max(g.big, o.sz);
            const long long s20 = P.rth ? P.s20Rth : P.s20On;
            if (s20 > 0 && o.sz >= s20 && o.nMoves == 0 && now - o.tRest >= P.stationaryMs) g.s20 = true; }
        double med[2] = { 0, 0 };
        for (int s = 0; s < 2; ++s) { std::vector<long long> v; for (const auto& kv : A) if (sideOf(kv.first) == s && kv.second.real > 0) v.push_back(kv.second.real);
            if (!v.empty()) { std::sort(v.begin(), v.end()); const size_t m = v.size() / 2; med[s] = v.size() % 2 ? (double)v[m] : 0.5 * (double)(v[m - 1] + v[m]); } }
        medNow[0] = med[0]; medNow[1] = med[1];
        std::vector<t10::In> tin[2];
        for (const auto& kv : A) { const int s = sideOf(kv.first); const long long px = pxOf(kv.first); const Agg& g = kv.second;
            const long long ins = s == 0 ? bb : ba; const int d = (int)(s == 0 ? ins - px : px - ins);
            const long long tot = g.real + g.young + g.quote;
            auto rm = [&](int w) { if (med[s] > 0 && (double)tot >= P.normX * med[s] && removed.size() < 400) { Lvl R; R.side = s; R.px = px; R.lots = tot; R.dist = d; R.why = w; removed.push_back(R); } };
            if (banned.count(key(s, px))) { ++why.pulled; rm(4); continue; }
            if (d < P.insideSkip) { ++why.inside; rm(1); continue; }
            if (g.real <= 0) { if (g.young >= g.quote) { ++why.young; rm(2); } else { ++why.quoting; rm(3); } continue; }
            if (med[s] > 0 && (double)g.real < P.normX * med[s]) { ++why.small; continue; }
            ++why.kept;
            Lvl L; L.side = s; L.px = px; L.lots = g.real; L.orders = g.n; L.big = g.big; L.x = med[s] > 0 ? (double)g.real / med[s] : 0; L.single = g.real > 0 && (double)g.big >= P.singleShare * (double)g.real; L.dist = d; L.s20 = g.s20;
            cands[key(s, px)] = L;
            tin[s].push_back(t10::In{ key(s, px), (double)px, g.real, s == 0, true }); }
        std::vector<Lvl> out;
        for (int s = 0; s < 2; ++s) { const std::vector<t10::Out> st = board[s].update(now, tin[s]);
            for (const t10::Out& o : st) { auto it = cands.find(o.key); Lvl L; if (it != cands.end()) L = it->second; else { L.side = s; L.px = (long long)std::llround(o.px); L.lots = o.lots; }
                L.rank = o.rank; out.push_back(L); } }
        std::sort(out.begin(), out.end(), [](const Lvl& a, const Lvl& b) { return a.lots != b.lots ? a.lots > b.lots : a.dist < b.dist; });
        for (size_t i = 0; i < out.size(); ++i) out[i].rank = (int)i + 1;   // the column's rank: 1 = the biggest of the (up to) 6 = K
        approachCheck(now, out);
        touchCheck(now, out, atr);
        shown = out; return shown;
    }
    // ---- read-outs (status / tests) ------------------------------------------------------------------------------------------
    const std::vector<Lvl>& lastShown() const { return shown; }
    const Why& lastWhy() const { return why; }
    const std::map<long long, Lvl>& lastCands() const { return cands; }
    const std::vector<Lvl>& lastRemoved() const { return removed; }   // big-enough levels a filter removed (the check / log)
    long long last() const { return lastPx; }
    double atr() const { return atrNow; }
    std::vector<Outcome> takeOutcomes() { std::vector<Outcome> v; v.swap(done); return v; }
    size_t orders() const { return O.size(); }
    long long ev() const { return events; }
    std::string statusLine() const {
        char b[600]; std::snprintf(b, sizeof b, "TOP_BOOK,TopBook %s,S20 %lld (%s),orders %zu,events %lld,inside %lld/%lld,atr %.1f ticks,median real bid %.0f / offer %.0f,shown %zu,kept %d,skipped inside %d young %d quoting %d small %d pulled-banned %d,banned prices %zu,held %d,broke %d,pulled while hit %d,single-order shown %d,dropped orders %lld,resets %lld",
            VERSION, P.rth ? P.s20Rth : P.s20On, P.rth ? "RTH" : "overnight", O.size(), events, insideB, insideA, atrNow, medNow[0], medNow[1], shown.size(), why.kept, why.inside, why.young, why.quoting, why.small, why.pulled, banned.size(), nHeld, nBroke, nPullHit, nSingle, dropped, resets);
        return b; }
    int nHeld = 0, nBroke = 0, nPullHit = 0, nSingle = 0, nBanned = 0;
private:
    struct Ord { int side = 0; long long px = 0, sz = 0, tRest = 0, tTouch = 0; int nMoves = 0; std::vector<long long> moves; };
    struct Watch { int side; long long px; long long lots0; long long cxl0; long long fill0; long long tFirst; bool touched = false, near = false; long long lotsNear = 0, cxlNear = 0, fillNear = 0, cxlPrev = 0, fillPrev = 0; long long tTouch = 0; long long lotsTouch = 0; long long cxlTouch = 0, fillTouch = 0; bool pullHit = false; };
    Params P; std::unordered_map<long long, Ord> O; std::unordered_set<long long> banned;
    std::unordered_map<long long, long long> cxl[2], fills[2];   // per price: cumulative cancelled / filled lots (approach + hit checks)
    t10::Board board[2]; std::vector<Lvl> shown; std::map<long long, Lvl> cands; std::vector<Lvl> removed; Why why; std::map<long long, Watch> watch; std::vector<Outcome> done;
    std::map<long long, std::pair<long long, long long> > bars;   // bar index -> (high, low) in ticks, from fills
    long long lastPx = 0, lastT = 0, events = 0, garbage = 0, dropped = 0, resets = 0, evals = 0, insideB = 0, insideA = 0; double atrNow = 0, medNow[2] = { 0, 0 };
    static long long key(int s, long long px) { return (s == 0 ? 1LL : 2LL) * 1000000000LL + px; }
    static int sideOf(long long k) { return k >= 2000000000LL ? 1 : 0; }
    static long long pxOf(long long k) { return k - (k >= 2000000000LL ? 2000000000LL : 1000000000LL); }
    void boardsInit() { t10::Params bp; bp.n = P.perSide; board[0] = t10::Board(bp); board[1] = t10::Board(bp); }
    void add(const In& e) {
        if (e.side != 0 && e.side != 1) { ++garbage; return; }
        if (O.size() >= P.maxOrders) { trim(); }
        Ord o; o.side = e.side; o.px = e.px; o.sz = e.sz; o.tRest = e.tMs; o.tTouch = e.tMs; O[e.oid] = o; }
    void mod(const In& e) {
        auto it = O.find(e.oid); if (it == O.end()) { In a = e; a.act = ADD; add(a); return; }
        Ord& o = it->second; o.tTouch = e.tMs;
        if (e.px > 0 && e.px != o.px) { cxl[o.side][o.px] += o.sz; ++o.nMoves; o.moves.push_back(e.tMs); if (o.moves.size() > 8) o.moves.erase(o.moves.begin()); o.px = e.px; o.tRest = e.tMs; }
        else if (e.sz > 0 && e.sz < o.sz) cxl[o.side][o.px] += o.sz - e.sz;   // a size cut = a partial cancel
        if (e.sz > 0) o.sz = e.sz; }
    void end(long long oid, bool cancel) {
        auto it = O.find(oid); if (it == O.end()) return;
        if (cancel) cxl[it->second.side][it->second.px] += it->second.sz;
        O.erase(it); }
    void fill(const In& e) {
        auto it = O.find(e.oid);
        long long px = e.px;
        if (it != O.end()) { Ord& o = it->second; if (px <= 0) px = o.px; fills[o.side][o.px] += std::min(e.sz, o.sz); o.sz -= e.sz; o.tTouch = e.tMs; if (o.sz <= 0) O.erase(it); }
        if (px > 0) { lastPx = px; const long long b = e.tMs / std::max(1LL, P.barMs); auto& h = bars[b]; if (h.first == 0 && h.second == 0) h = std::make_pair(px, px); else { h.first = std::max(h.first, px); h.second = std::min(h.second, px); }
            while (bars.size() > (size_t)(P.atrBars + 2)) bars.erase(bars.begin()); } }
    void trim() {   // bounded memory: drop the 10% least recently touched orders (a feed that misses deletes) - counted
        std::vector<std::pair<long long, long long> > v; v.reserve(O.size()); for (const auto& kv : O) v.push_back(std::make_pair(kv.second.tTouch, kv.first));
        std::sort(v.begin(), v.end()); const size_t n = v.size() / 10 + 1; for (size_t i = 0; i < n && i < v.size(); ++i) { O.erase(v[i].second); ++dropped; } }
    double atrTicks() const {
        if (bars.size() < 3) return P.atrDefaultTicks;
        double s = 0; int n = 0; for (auto it = bars.rbegin(); it != bars.rend() && n < P.atrBars; ++it) { s += (double)(it->second.first - it->second.second); ++n; }
        return std::max(4.0, n > 0 ? s / n : P.atrDefaultTicks); }
    long long realAt(int s, long long px) const { auto it = cands.find(key(s, px)); return it == cands.end() ? 0 : it->second.lots; }
    // a shown level the inside came within APPROACH ticks of: lost >= half by cancels (not fills) -> banned at that price this session
    void approachCheck(long long now, const std::vector<Lvl>& out) {
        for (auto it = watch.begin(); it != watch.end();) {
            Watch& w = it->second; const long long ins = w.side == 0 ? insideB : insideA;
            const long long d = w.side == 0 ? ins - w.px : w.px - ins;
            if (w.touched) { ++it; continue; }
            const long long nowLots = realAt(w.side, w.px);
            if (d >= 0 && d <= P.approach) {
                // (the approach window) the lots and the cancel / fill counters when the inside first came within APPROACH ticks
                if (!w.near) { w.near = true; w.lotsNear = std::max(nowLots, w.lots0); w.cxlNear = w.cxlPrev; w.fillNear = w.fillPrev; }   // from the previous second: a pull in the arrival second counts
                const long long cx = cxl[w.side][w.px] - w.cxlNear, fl = fills[w.side][w.px] - w.fillNear;
                if (w.lotsNear > 0 && (double)(w.lotsNear - nowLots) >= P.pulledFrac * (double)w.lotsNear && cx > fl && (double)cx >= P.pulledFrac * (double)w.lotsNear) {
                    banned.insert(it->first); ++nBanned; it = watch.erase(it); continue; }
            } else if (d > P.approach) w.near = false;   // price left: the next approach starts a new window
            if (nowLots > 0 && !w.near) w.lots0 = nowLots;
            w.cxlPrev = cxl[w.side][w.px]; w.fillPrev = fills[w.side][w.px];
            ++it; }
        for (const Lvl& L : out) { const long long k = key(L.side, L.px); auto it = watch.find(k);
            if (it == watch.end()) { Watch w; w.side = L.side; w.px = L.px; w.lots0 = L.lots; w.cxl0 = w.cxlPrev = cxl[L.side][L.px]; w.fill0 = w.fillPrev = fills[L.side][L.px]; w.tFirst = now; watch[k] = w; if (L.single) ++nSingle; }
            else if (!it->second.touched && !it->second.near) it->second.lots0 = L.lots; }
        (void)now; }
    // touched = the inside reached it (or traded at it); outcome HELD / BROKE decided once, logged, never drawn
    void touchCheck(long long now, const std::vector<Lvl>&, double atr) {
        for (auto it = watch.begin(); it != watch.end();) {
            Watch& w = it->second; const long long ins = w.side == 0 ? insideB : insideA;
            if (!w.touched) {
                const bool reached = w.side == 0 ? (lastPx > 0 && lastPx <= w.px) : (lastPx > 0 && lastPx >= w.px);
                if (reached) { w.touched = true; w.tTouch = now; w.lotsTouch = w.lots0; w.cxlTouch = cxl[w.side][w.px]; w.fillTouch = fills[w.side][w.px]; }
                else if (now - w.tFirst > 3600000) { it = watch.erase(it); continue; }
                ++it; continue; }
            const long long cx = cxl[w.side][w.px] - w.cxlTouch, fl = fills[w.side][w.px] - w.fillTouch;
            if (!w.pullHit && fl > 0 && w.lotsTouch > 0 && (double)cx >= P.pullHitFrac * (double)w.lotsTouch) { w.pullHit = true; ++nPullHit; }
            const double away = w.side == 0 ? (double)(lastPx - w.px) : (double)(w.px - lastPx);
            const double through = -away; (void)ins;
            if (through >= P.brokeAtr * atr) { done.push_back(Outcome{ w.side, w.px, w.lotsTouch, false, w.pullHit, w.tTouch, now }); ++nBroke; it = watch.erase(it); continue; }
            if (away >= P.heldAtr * atr) { const bool still = realAt(w.side, w.px) > 0 || cands.count(key(w.side, w.px)) || anyResting(w.side, w.px);
                done.push_back(Outcome{ w.side, w.px, w.lotsTouch, still, w.pullHit, w.tTouch, now }); if (still) ++nHeld; else ++nBroke; it = watch.erase(it); continue; }
            if (now - w.tTouch > 3600000) { it = watch.erase(it); continue; }
            ++it; }
        while (done.size() > 500) done.erase(done.begin()); }
    bool anyResting(int s, long long px) const { for (const auto& kv : O) if (kv.second.side == s && kv.second.px == px && kv.second.sz > 0) return true; return false; }
};
} // namespace tbk
#endif
