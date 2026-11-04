#ifndef LS_BOOK_LEVEL_BOX_H
#define LS_BOOK_LEVEL_BOX_H
// BookLevelBox.h 1.1.0 (2026-11-04; 1.1.0 = + the WAVE POC levels, Rassul "add wave poc as one of the levels for bookflow") -- the SDK-free LEVEL BOX of lsBookFlow 1.34.0 (Rassul "just build it", 2026-11-04: the BookFlow
// upgrade he called "clean and it looks good" on bf_mockup.html / bf_*.png / bf3_*.png). One line, only while price is near a level:
//   "<LEVEL> | Defenders stacking 2.1x? | Behind stacking 3.3x? | Attackers normal?"
// and two rows of squares under the bars of each candle that spent time at a level (Defenders row, Behind row).
//
// DEFINITIONS = the nightly lra.book_level_combo 1.1.1 (BLC111; prototype replay3.py / reads.py / figs2.smooth), line for line:
//   zone      level +-4 ticks (Pb test zone: the bottom half of the BO candle / the top half of the BD candle)
//   side      support when price came from above (defenders = bids), resistance from below (defenders = offers): the side of the last
//             trade >= 8 ticks beyond the zone edge (AWAY)
//   arrival   the first trade in the zone after price was >= 8 ticks beyond its edge; the zone of a moving level (VWAP / bands) is fixed
//             at the arrival
//   DEFENDERS rolling 30 s: durable adds in the zone (not cancelled / moved within 5 s) vs pulls of orders that had rested >= 5 s;
//             stacking adds / pulls >= 1.5 (green at a support), pulling pulls / adds >= 1.5 (red), else steady; quiet when adds + pulls <
//             the market's floor (ES 40 lots; others from the nightly's export, else scaled from this session's own arrivals as
//             book_level_combo.quiet_floor does: max(5, 40 x median(adds + pulls at -60..-30 s) / 60))
//   BEHIND    defender-side resting size 1-12 ticks beyond the zone (the book at the start of each second) vs its mean at -150..-91 s
//             before the arrival: stacking >= 2.0x, thinning <= 0.67x, else steady
//   ATTACKERS opposite-side resting orders within 8 ticks of price (offers best ask .. +8 at a support, bids best bid -8 .. at a
//             resistance): 30-s durable adds and rested pulls vs their own median over the 30-s windows ending -271 .. -31 s:
//             stacking >= 1.4x (and >= the pull ratio), pulling >= 1.5x, else normal; always shown with "?"
//   PERSIST   a new state shows only after it held 15 s (figs2.smooth: a change of ratio / colour inside the same word is immediate),
//             walked from -270 s before the anchor
// LIVE: a second's adds are final once 5 s have passed (the durable rule) -> the box shows second E = the newest complete second - 5
// (the replay's exact value for E, 5 s late). Anchor = the arrival; before an arrival (price within the band, side known) the anchor
// is E itself ("as if arriving now"). The box shows only while price is within the BAND = 4 ticks of the zone (= 2 ES points of the
// level for a +-4 tick zone; every distance is in ticks, as the nightly's zone / away rules, so it scales per market by its tick).
// Squares: per candle, seconds the box showed: red if red >= 20 s, else green if green >= 20 s, else gray; decided once every second of
// the candle was evaluated, never changed (the host records them and reloads them after a restart).
// Per event O(1) (one hash lookup / insert, one bucket add); per new trade PRICE O(levels) (the away / arrival rule); per second one
// book snapshot (2 x 321 ticks) and the smoothing step of the active case only.
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace blb {

static const char* const VERSION = "1.1.0";   // (1.1.0) + L_WPOC: lsWavePOC's drawn wave POC lines (wpocLevels)   // was "1.0.0"
static const int HALF = 4, AWAY = 8, ABAND = 8, MINREST = 5, BEHIND = 12, PERSIST = 15, WIN = 30, PRE = 300, BAND = 4;
static const int HIST = 700;                 // seconds kept in the per-second rings (a case can be rebuilt up to ~+390 s after its anchor)
static const int SNAP_R = 160;               // book snapshot half-width around the last trade (ticks)
static const int DEPTH_N = 1 << 16;          // the flat book: ticks around the first price seen
static const int PEND_MAX = 256;             // pending (not yet durable) adds kept per order (an order re-sized many times inside 5 s; 8 overflowed 1,243 times on ES 10-30)
static const int SQ_SECS = 20;               // a square's colour needs >= 20 s of that colour
static const double ES_REF_ACT = 60.0;       // book_level_combo.ES_REF_ACT

enum Col { GRAY = 0, GREEN = 1, RED = 2 };
enum LType { L_KEY = 0, L_MQ = 1, L_PROFILE = 2, L_VWAP = 3, L_PB = 4, L_WPOC = 5 };
static const int L_TYPES = 6;
inline const char* ltypeName(int t) { return t == L_KEY ? "key" : t == L_MQ ? "mq" : t == L_PROFILE ? "profile" : t == L_VWAP ? "vwap" : t == L_WPOC ? "wave_poc" : "pb"; }   // (1.1.0) "wave_poc" = the nightly's ltype

struct Params {
    double defX = 1.5, quiet = -1, behStack = 2.0, behThin = 0.67, attStack = 1.4, attPull = 1.5;   // quiet < 0 = not from the file
    bool showD = false, showB = false;       // the nightly says SHOW for that line (else "?")
    bool file = false; std::string why = "defaults (no LRA-BookLevel file)";
};

// one read: word, ratio, colour (the RAW colour of the nightly: D stacking green / pulling red; B stacking green / thinning red;
// A stacking red / pulling green - as at a support; the box flips them at a resistance)
struct Read { int word = 0; double x = 0; int col = GRAY; };
enum Word { W_QUIET = 0, W_STACK = 1, W_PULL = 2, W_STEADY = 3, W_THIN = 4, W_NORMAL = 5 };
inline bool sameState(const Read& a, const Read& b) { return a.word == b.word && a.col == b.col; }

// the levels the host gives each second
struct Level { std::string name; int type = L_KEY; long long lo = 0, hi = 0; bool fixedZone = false; long long tFrom = 0, tTo = 0; };   // ticks; Pb: fixed zone + window [tFrom, tTo) (0 = open)

inline long long zoneLo(long long c) { return c - HALF; }
inline long long zoneHi(long long c) { return c + HALF; }

struct Pend { long long s = 0, pt = 0, q = 0; int side = 0, a = -1; };

struct Engine {
    // ---- the book (the replay's book / depth) ----
    struct Ord { int side = 0; long long pt = 0, sz = 0, tr = 0; bool wf = false; };
    std::unordered_map<long long, Ord> book;
    std::vector<long long> dep[2]; long long base = 0; bool based = false; long long outside = 0;
    long long cb = 0, ca = 0, lastTrade = 0; bool haveTrade = false;
    // ---- per-second rings ----
    struct Cell { long long key = 0, add = 0, pull = 0; };
    struct Sec { long long s = -1; std::vector<Cell> cells; long long aAdd[2] = {0, 0}, aPull[2] = {0, 0}; long long center = 0; bool snap = false; std::vector<int> d[2]; long long px = 0; bool pxOk = false; };
    std::vector<Sec> ring;
    long long cur = -1;                      // the newest event second
    std::deque<std::pair<long long, long long> > pendQ;      // (second, order id) of every pending add, time order
    std::unordered_map<long long, std::vector<Pend> > pend;
    long long events = 0, pendOver = 0, garbage = 0;
    // ---- the levels ----
    struct LState { Level lv; bool away = false; int sideFrom = -1; bool arrived = false; long long tArr = 0, aLo = 0, aHi = 0; int ds = 0; };
    std::vector<LState> L;
    long long resetAt = 0;                   // the second the away states were last reset (RTH open - 60 min, as find_touches)
    std::vector<double> arrAct;              // adds + pulls at -60..-30 s of each arrival (the quiet floor of other markets)
    long long arrivals = 0;
    Params P; std::string market;

    Engine() : ring(HIST) { dep[0].assign(DEPTH_N, 0); dep[1].assign(DEPTH_N, 0); }
    void reset() { book.clear(); for (int s = 0; s < 2; ++s) std::fill(dep[s].begin(), dep[s].end(), 0); based = false; cb = ca = 0; haveTrade = false;
                   for (Sec& x : ring) { x = Sec(); }
                   cur = -1; pendQ.clear(); pend.clear(); resetAway(); sm = Smooth(); cache.clear(); }
    double quietFloor() const {
        if (P.quiet > 0) return P.quiet;
        if (market == "ES") return 40.0;
        if (arrAct.empty()) return 40.0;     // no arrival yet this session: the ES floor
        std::vector<double> v(arrAct); std::sort(v.begin(), v.end());
        const size_t n = v.size(); const double med = n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
        return std::max(5.0, std::round(40.0 * med / ES_REF_ACT * 10.0) / 10.0);
    }
    Sec& at(long long s) { return ring[(size_t)(((s % HIST) + HIST) % HIST)]; }
    const Sec* get(long long s) const { if (s < 0) return nullptr; const Sec& x = ring[(size_t)(((s % HIST) + HIST) % HIST)]; return x.s == s ? &x : nullptr; }
    long long& depthAt(int side, long long pt) { static long long dummy; const long long i = pt - base; if (i < 0 || i >= DEPTH_N) { dummy = 0; ++outside; return dummy; } return dep[side][(size_t)i]; }
    long long depthOf(int side, long long pt) const { const long long i = pt - base; if (i < 0 || i >= DEPTH_N) return 0; return dep[side][(size_t)i]; }
    static long long keyOf(int side, long long pt) { return pt * 2 + side; }
    Cell* cell(long long s, int side, long long pt, bool make) {
        Sec& x = at(s); if (x.s != s) return nullptr;
        const long long k = keyOf(side, pt);
        for (Cell& c : x.cells) if (c.key == k) return &c;
        if (!make) return nullptr;
        if (haveTrade && std::llabs(pt - lastTrade) > 400) return nullptr;   // far from price: never in a zone near price
        Cell c; c.key = k; x.cells.push_back(c); return &x.cells.back();
    }
    // the snapshot of second ss = the book before its events (the replay's snap())
    void snapTo(long long ss) {
        Sec& x = at(ss);
        if (x.s != ss) { x = Sec(); x.s = ss; }
        x.center = haveTrade ? lastTrade : (cb > 0 ? cb : ca); x.snap = true;
        for (int s = 0; s < 2; ++s) { x.d[s].assign(2 * SNAP_R + 1, 0); for (int k = -SNAP_R; k <= SNAP_R; ++k) { const long long v = depthOf(s, x.center + k); x.d[s][(size_t)(k + SNAP_R)] = v > 2000000000LL ? 2000000000 : (int)v; } }
        // the last trade carried forward (px_last ffill)
        const Sec* p = get(ss - 1); if (haveTrade) { x.px = lastTrade; x.pxOk = true; } else if (p && p->pxOk) { x.px = p->px; x.pxOk = true; }
    }
    void newSecond(long long s) {
        if (cur >= 0 && s <= cur) return;
        if (cur < 0) { snapTo(s); cur = s; return; }
        long long from = cur + 1; if (s - from >= HIST) from = s - HIST + 1;
        for (long long ss = from; ss <= s; ++ss) snapTo(ss);
        cur = s;
        while (!pendQ.empty() && pendQ.front().first <= s - MINREST) {             // adds 5 s old are durable: forget their pending entry
            const long long o = pendQ.front().second; pendQ.pop_front();
            std::unordered_map<long long, std::vector<Pend> >::iterator it = pend.find(o);
            if (it != pend.end()) { std::vector<Pend>& v = it->second; while (!v.empty() && v.front().s <= s - MINREST) v.erase(v.begin()); if (v.empty()) pend.erase(it); }
        }
    }
    int inA(int side, long long p) const { if (side == 1 && ca > 0 && p >= ca && p <= ca + ABAND) return 0; if (side == 0 && cb > 0 && p >= cb - ABAND && p <= cb) return 1; return -1; }
    void logAdd(long long s, int side, long long p, long long q, long long o) {
        if (q <= 0) return;
        Cell* c = cell(s, side, p, true); if (c) c->add += q;
        const int a = inA(side, p); if (a >= 0) at(s).aAdd[a] += q;
        if (!c && a < 0) return;
        Pend pd; pd.s = s; pd.pt = c ? p : LLONG_MIN; pd.q = q; pd.side = side; pd.a = a;   // pt LLONG_MIN = counted in no zone bucket
        std::vector<Pend>& v = pend[o]; if ((int)v.size() >= PEND_MAX) { v.erase(v.begin()); ++pendOver; }
        v.push_back(pd); pendQ.push_back(std::make_pair(s, o));
    }
    void logPull(long long s, int side, long long p, long long q, bool rested) {
        if (!rested || q <= 0) return;
        Cell* c = cell(s, side, p, true); if (c) c->pull += q;
        const int a = inA(side, p); if (a >= 0) at(s).aPull[a] += q;
    }
    // a leave (moved away / cancelled, not filled): its adds of the last 5 s were not durable - take them back
    void leave(long long s, long long o) {
        std::unordered_map<long long, std::vector<Pend> >::iterator it = pend.find(o); if (it == pend.end()) return;
        for (const Pend& d : it->second) {
            if (s - d.s >= MINREST) continue;
            if (d.pt != LLONG_MIN) { Cell* c = cell(d.s, d.side, d.pt, false); if (c) c->add -= d.q; }
            if (d.a >= 0) { Sec& x = at(d.s); if (x.s == d.s) x.aAdd[d.a] -= d.q; }
        }
        pend.erase(it);
    }
    // one order event. act 0 NEW / 1 MOD / 2 DEL / 3 FILL; side 0 bid / 1 offer (the event's side); bid / ask in ticks (0 = none)
    void onEvent(long long s, int act, int side, long long p, long long q, long long o, long long bidPt, long long askPt) {
        ++events;
        if (s <= 0 || act < 0 || act > 3) { ++garbage; return; }
        if (cur >= 0 && s < cur) s = cur;                                       // a late stamp counts in the newest second
        newSecond(s);
        if (bidPt > 0) cb = bidPt;
        if (askPt > 0) ca = askPt;
        if (!based && p > 0) { base = p - DEPTH_N / 2; based = true; }
        if (p <= 0) { ++garbage; return; }
        if (act == 0) {
            if (side != 0 && side != 1) { ++garbage; return; }
            Ord d; d.side = side; d.pt = p; d.sz = q; d.tr = s; book[o] = d; depthAt(side, p) += q;
            logAdd(s, side, p, q, o);
        } else if (act == 1) {
            if (side != 0 && side != 1) { ++garbage; return; }
            std::unordered_map<long long, Ord>::iterator it = book.find(o);
            if (it == book.end()) { Ord d; d.side = side; d.pt = p; d.sz = q; d.tr = s; book[o] = d; depthAt(side, p) += q; return; }
            Ord& b = it->second; const int osd = b.side; const long long op = b.pt, oq = b.sz, tr = b.tr;
            depthAt(osd, op) -= oq; depthAt(side, p) += q;
            if (op != p) {
                logPull(s, osd, op, oq, s - tr >= MINREST); leave(s, o);
                b.side = side; b.pt = p; b.sz = q; b.tr = s; b.wf = false;
                logAdd(s, side, p, q, o);
            } else {
                b.side = side; b.sz = q; b.wf = false;
                if (q > oq) logAdd(s, side, p, q - oq, o);
                else if (q < oq) logPull(s, side, p, oq - q, s - tr >= MINREST);
            }
        } else if (act == 2) {
            std::unordered_map<long long, Ord>::iterator it = book.find(o); if (it == book.end()) return;
            const Ord b = it->second; book.erase(it);
            depthAt(b.side, b.pt) -= b.sz;
            if (!b.wf && b.sz > 0) { logPull(s, b.side, b.pt, b.sz, s - b.tr >= MINREST); leave(s, o); }
        } else {
            std::unordered_map<long long, Ord>::iterator it = book.find(o);
            if (it != book.end()) { Ord& b = it->second; const long long d = std::min(q, b.sz); depthAt(b.side, b.pt) -= d; b.sz -= d; b.wf = true; }
            trade(s, p);
        }
    }
    // a trade print: the last price, and the away / arrival rule of every level (only when the trade price changed: same price = same state)
    void trade(long long s, long long p) {
        const bool changed = !haveTrade || p != lastTrade;
        lastTrade = p; haveTrade = true;
        Sec& x = at(s); if (x.s == s) { x.px = p; x.pxOk = true; }
        if (changed) for (LState& l : L) step(l, s, p);
    }
    bool zoneOf(const LState& l, long long& lo, long long& hi) const { lo = l.lv.lo; hi = l.lv.hi; return hi >= lo && lo > 0; }
    void step(LState& l, long long s, long long p) {
        if (l.lv.fixedZone && ((l.lv.tFrom > 0 && s < l.lv.tFrom) || (l.lv.tTo > 0 && s >= l.lv.tTo))) return;
        long long lo, hi; if (!zoneOf(l, lo, hi)) return;
        if (p >= hi + AWAY) { l.away = true; l.sideFrom = 0; l.arrived = false; }
        else if (p <= lo - AWAY) { l.away = true; l.sideFrom = 1; l.arrived = false; }
        else if (p >= lo && p <= hi && l.away) {
            l.away = false; l.arrived = true; l.tArr = s; l.aLo = lo; l.aHi = hi; l.ds = l.sideFrom; ++arrivals;
            pendingArr.push_back((int)(&l - &L[0]));
        }
    }
    std::vector<int> pendingArr;             // arrivals whose -60..-30 activity is measured at the next evaluation
    // replace the level list (same name + type keep their state; a moved zone of a level that has not arrived just moves)
    void setLevels(const std::vector<Level>& v) {
        std::vector<LState> n; n.reserve(v.size());
        for (const Level& lv : v) {
            LState st; st.lv = lv;
            for (const LState& o : L) if (o.lv.name == lv.name && o.lv.type == lv.type && o.lv.tFrom == lv.tFrom) { st = o; st.lv = lv; break; }
            n.push_back(st);
        }
        L.swap(n); pendingArr.clear();
    }
    void resetAway() { for (LState& l : L) { l.away = false; l.sideFrom = -1; l.arrived = false; } }

    // ---- the reads (book_level_combo.read_at) for a case ----
    struct Case { int li = -1; std::string name; int type = 0; long long lo = 0, hi = 0, anchor = 0; int ds = 0; bool arrived = false; };
    // per-second inputs of a case: d adds / pulls in the zone, attacker adds / pulls, behind
    bool inputs(const Case& c, long long s, double& dA, double& dP, double& aA, double& aP, double& bh) const {
        const Sec* x = get(s); if (!x) return false;
        dA = dP = 0;
        for (const Cell& k : x->cells) { const int side = (int)(k.key & 1); const long long pt = (k.key - side) / 2; if (side == c.ds && pt >= c.lo && pt <= c.hi) { dA += (double)k.add; dP += (double)k.pull; } }
        aA = (double)x->aAdd[c.ds]; aP = (double)x->aPull[c.ds];
        bh = 0;
        if (x->snap) {
            const long long a = c.ds == 0 ? c.lo - BEHIND : c.hi + 1, b = c.ds == 0 ? c.lo - 1 : c.hi + BEHIND;
            for (long long t = a; t <= b; ++t) { const long long i = t - x->center + SNAP_R; if (i >= 0 && i <= 2 * SNAP_R) bh += (double)x->d[c.ds][(size_t)i]; }
        }
        return true;
    }
    struct Series { long long s0 = 0; std::vector<double> dA, dP, aA, aP, bh; std::vector<char> ok; };
    // the series for seconds [s0, s1] (missing seconds = 0, as the replay's zero arrays)
    void series(const Case& c, long long s0, long long s1, Series& S) const {
        const size_t n = (size_t)(s1 - s0 + 1); S.s0 = s0;
        S.dA.assign(n, 0); S.dP.assign(n, 0); S.aA.assign(n, 0); S.aP.assign(n, 0); S.bh.assign(n, 0); S.ok.assign(n, 0);
        for (size_t i = 0; i < n; ++i) { double a, b, e, f, g; if (inputs(c, s0 + (long long)i, a, b, e, f, g)) { S.dA[i] = a; S.dP[i] = b; S.aA[i] = e; S.aP[i] = f; S.bh[i] = g; S.ok[i] = 1; } }
    }
    static double r30(const std::vector<double>& v, long long i) { double t = 0; for (long long j = i - 29; j <= i; ++j) if (j >= 0 && j < (long long)v.size()) t += v[(size_t)j]; return t; }
    static double median(std::vector<double> v) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); const size_t n = v.size(); return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0; }
    struct Base { double b0 = 0, na = 0, np = 0; bool ok = false; };
    // the baselines of a case (behind mean -150..-91; attackers' medians of the 30-s windows ending -271..-31) from S (S.s0 = anchor - PRE)
    static Base baseOf(const Series& S) {
        Base B; if (S.dA.size() < (size_t)PRE) return B;
        double t = 0; for (int k = PRE - 150; k < PRE - 90; ++k) t += S.bh[(size_t)k]; B.b0 = t / 60.0;
        std::vector<double> wa, wp; for (int j = 29; j < PRE - 30; ++j) { wa.push_back(r30(S.aA, j)); wp.push_back(r30(S.aP, j)); }
        B.na = median(wa); B.np = median(wp); B.ok = true; return B;
    }
    void readAt(const Series& S, const Base& B, long long i, double floor, Read out[3]) const {
        const double A = r30(S.dA, i), Pu = r30(S.dP, i);
        Read d; if (A + Pu < floor) { d.word = W_QUIET; d.x = 0; d.col = GRAY; }
        else if (A >= Pu) { d.x = A / std::max(Pu, 1.0); d.word = W_STACK; d.col = d.x >= P.defX ? GREEN : GRAY; }
        else { d.x = Pu / std::max(A, 1.0); d.word = W_PULL; d.col = d.x >= P.defX ? RED : GRAY; }
        const double bn = i >= 0 && i < (long long)S.bh.size() ? S.bh[(size_t)i] : 0, bx = bn / std::max(B.b0, 1.0);
        Read b; if (bx >= P.behStack) { b.word = W_STACK; b.x = bx; b.col = GREEN; } else if (bx <= P.behThin) { b.word = W_THIN; b.x = 1.0 / std::max(bx, 1e-3); b.col = RED; } else { b.word = W_STEADY; b.x = bx; b.col = GRAY; }
        const double xa = r30(S.aA, i) / std::max(B.na, 1.0), xp = r30(S.aP, i) / std::max(B.np, 1.0);
        Read a; if (xa >= P.attStack && xa >= xp) { a.word = W_STACK; a.x = xa; a.col = RED; } else if (xp >= P.attPull) { a.word = W_PULL; a.x = xp; a.col = GREEN; } else { a.word = W_NORMAL; a.x = xa; a.col = GRAY; }
        out[0] = d; out[1] = b; out[2] = a;
    }
    // ---- the shown state (live) ----
    struct Shown { bool on = false; Case c; long long E = 0, k = 0; Read r[3]; Read raw[3]; bool settled = false; };
    // smoothing state of the active case: the shown reads + the last PERSIST raw reads per line
    struct Smooth { long long anchor = 0, lastK = 0; int li = -1; long long lo = 0, hi = 0; int ds = 0; bool arrived = false; Base B; Read cur[3]; std::deque<Read> win[3]; bool ok = false; };
    Smooth sm; long long smoothRebuilds = 0;
    void smoothStep(Read raw[3]) {
        for (int q = 0; q < 3; ++q) {
            sm.win[q].push_back(raw[q]); while ((int)sm.win[q].size() > PERSIST) sm.win[q].pop_front();
            bool all = (int)sm.win[q].size() == PERSIST; for (const Read& w : sm.win[q]) all = all && sameState(w, raw[q]);
            if (all) sm.cur[q] = raw[q]; else if (sm.cur[q].word == raw[q].word) sm.cur[q] = raw[q];
        }
    }
    // rebuild the smoothing of case c from k = -270 up to k (the nightly's walk), from the rings
    bool rebuild(const Case& c, long long kTo) {
        Series S; series(c, c.anchor - PRE, c.anchor + std::max(kTo, 0LL), S);   // the baselines need -300 .. -31 (kTo < 0 only in tests)
        sm = Smooth(); sm.anchor = c.anchor; sm.li = c.li; sm.lo = c.lo; sm.hi = c.hi; sm.ds = c.ds; sm.arrived = c.arrived; sm.B = baseOf(S);
        if (!sm.B.ok) return false;
        for (int q = 0; q < 3; ++q) { sm.cur[q] = Read(); sm.cur[q].word = W_QUIET; }   // every line starts ('quiet', 0, 'gray') as smooth()
        const double fl = quietFloor();
        for (long long k = -270; k <= kTo; ++k) { Read r[3]; readAt(S, sm.B, PRE + k, fl, r); smoothStep(r); }
        sm.lastK = kTo; sm.ok = true; ++smoothRebuilds; return true;
    }
    std::map<std::pair<int, long long>, Smooth> cache;      // arrived cases' smoothing (level index, anchor): continue after a switch
    bool advance(const Case& c, long long kTo) {
        const bool same = sm.ok && sm.anchor == c.anchor && sm.li == c.li && sm.lo == c.lo && sm.hi == c.hi && sm.ds == c.ds;
        if (!same) {
            if (sm.ok && sm.arrived) { cache[std::make_pair(sm.li, sm.anchor)] = sm; while (cache.size() > 32) cache.erase(cache.begin()); }
            std::map<std::pair<int, long long>, Smooth>::iterator it = cache.find(std::make_pair(c.li, c.anchor));
            if (c.arrived && it != cache.end() && it->second.lo == c.lo && it->second.hi == c.hi && it->second.ds == c.ds) { sm = it->second; cache.erase(it); }
            else return rebuild(c, kTo);
        }
        if (kTo < sm.lastK) return rebuild(c, kTo);
        if (kTo == sm.lastK) return true;
        if (!get(c.anchor + sm.lastK + 1 - 29)) return rebuild(c, kTo);          // the gap is older than the rings: rebuild from what is kept
        Series S; series(c, c.anchor + sm.lastK + 1 - 29, c.anchor + kTo, S);
        const double fl = quietFloor();
        for (long long k = sm.lastK + 1; k <= kTo; ++k) { Read r[3]; readAt(S, sm.B, (k - (sm.lastK + 1 - 29)), fl, r); smoothStep(r); }
        sm.lastK = kTo; return true;
    }
    // which level the box is about at second E (price at E): within BAND of the zone, side known; an arrived case first (latest arrival),
    // else the nearest. false = no box.
    bool pick(long long E, Case& out) const {
        const Sec* x = get(E); if (!x || !x->pxOk) return false;
        const long long p = x->px;
        int best = -1; long long bestT = -1, bestD = 0;
        for (size_t i = 0; i < L.size(); ++i) {
            const LState& l = L[i];
            if (l.lv.fixedZone && ((l.lv.tFrom > 0 && E < l.lv.tFrom) || (l.lv.tTo > 0 && E >= l.lv.tTo)) && !(l.arrived && l.tArr <= E)) continue;
            long long lo, hi; bool arr = l.arrived && l.tArr <= E;
            if (arr) { lo = l.aLo; hi = l.aHi; } else { if (l.sideFrom < 0 || !zoneOf(l, lo, hi)) continue; }
            const long long d = p < lo ? lo - p : (p > hi ? p - hi : 0);
            if (d > BAND) continue;
            const long long t = arr ? l.tArr : 0;
            if (best < 0 || t > bestT || (t == bestT && d < bestD)) { best = (int)i; bestT = t; bestD = d; }
        }
        if (best < 0) return false;
        const LState& l = L[(size_t)best];
        out = Case(); out.li = best; out.name = l.lv.name; out.type = l.lv.type;
        if (l.arrived && l.tArr <= E) { out.lo = l.aLo; out.hi = l.aHi; out.anchor = l.tArr; out.ds = l.ds; out.arrived = true; }
        else { zoneOf(l, out.lo, out.hi); out.anchor = E; out.ds = l.sideFrom; out.arrived = false; }
        return true;
    }
    // the activity of new arrivals at -60..-30 s (quiet floor of markets with no file)
    void measureArrivals() {
        for (int li : pendingArr) {
            if (li < 0 || li >= (int)L.size()) { continue; }
            const LState& l = L[(size_t)li]; Case c; c.lo = l.aLo; c.hi = l.aHi; c.ds = l.ds;
            double t = 0; bool any = false;
            for (long long s = l.tArr - 60; s < l.tArr - 30; ++s) { double a, b, e, f, g; if (inputs(c, s, a, b, e, f, g)) { t += a + b; any = true; } }
            if (any) { arrAct.push_back(t); if (arrAct.size() > 400) arrAct.erase(arrAct.begin()); }
        }
        pendingArr.clear();
    }
    // the box at second E (call once per second with E = newest complete second - MINREST)
    Shown evaluate(long long E) {
        Shown sh; sh.E = E; measureArrivals();
        Case c; if (!pick(E, c)) { sm.ok = false; return sh; }
        const long long k = E - c.anchor;
        if (!advance(c, k)) return sh;
        sh.on = true; sh.c = c; sh.k = k; for (int q = 0; q < 3; ++q) sh.r[q] = sm.cur[q];
        sh.settled = c.arrived;
        return sh;
    }
    long long newestComplete() const { return cur - 1; }
};

// ---- what the box shows -----------------------------------------------------------------------------------------------------------
// the colour by implication: at a support the raw colours; at a resistance green <-> red (sellers stacking = red)
inline int implCol(int raw, int ds) { if (raw == GRAY || ds == 0) return raw; return raw == GREEN ? RED : GREEN; }
inline std::string wordText(int line, const Read& r) {
    switch (r.word) {
        case W_QUIET: return "quiet";
        case W_STEADY: return "steady";
        case W_NORMAL: return "normal";
        default: break;
    }
    if (line == 0 && r.col == GRAY) return "steady";                  // defenders under 1.5x either way
    const char* w = r.word == W_STACK ? "stacking" : r.word == W_PULL ? "pulling" : "thinning";
    char b[48]; if (r.x >= 20.0 || !(r.x == r.x)) std::snprintf(b, sizeof b, "%s 20x+", w); else std::snprintf(b, sizeof b, "%s %.1fx", w, r.x);
    return b;
}
inline std::string lineText(int line, const Read& r, bool proven) {
    static const char* const LAB[3] = { "Defenders ", "Behind ", "Attackers " };
    return std::string(LAB[line]) + wordText(line, r) + (proven && line < 2 ? "" : "?");
}
struct BoxText { std::string level; std::string part[3]; int col[3] = {GRAY, GRAY, GRAY}; };
inline BoxText boxText(const Engine::Shown& sh, const Params& P) {
    BoxText t; if (!sh.on) return t; t.level = sh.c.name;
    for (int q = 0; q < 3; ++q) { t.part[q] = lineText(q, sh.r[q], q == 0 ? P.showD : (q == 1 ? P.showB : false)); t.col[q] = implCol(sh.r[q].col, sh.c.ds); }
    return t;
}

// ---- (1.1.0) WAVE POC levels: the wave POC lines lsWavePOC 1.8.3 draws (WavePOCLogic.h drawPocLines), from its own record ----------
// Rassul 2026-11-04: "add wave poc as one of the levels for bookflow". Source = lsFlexLevels\WavePOC\<MKT>-180-nodes.csv (append-only:
// WAVE,ver,N,dir,t0,t1,tConf,... + NODE,t0,t1,rank,price,...; rank 0 = the wave's volume POC = the cyan line). Times = bar END, local
// (Central) wall-clock seconds (WavePOC::localSec). The rule = drawPocLines line for line, on the chart's CLOSED bars:
//   waves     every recorded wave of the chart's reversal-bars N (the N of the newest WAVE line) confirmed by the last closed bar
//             (tConf <= its time); an extended leg (same direction and start, later end) replaces the earlier one (Engine::leg)
//   day       only waves whose extreme bar is in the last bar's CME trading day (17:00 CT starts the next; tday)
//   tested    the first closed bar AFTER the extreme bar whose range contains the POC; a tested line is drawn (to that bar) only until
//             a newer wave of the same direction exists
//   untested  only the 2 nearest at / above the last close and the 2 nearest below it (distance, then confirmation order)
// Every drawn line (untested + tested) is a level: zone = POC +-4 ticks, named "WPOC <price>". Not used (documented differences): the
// LIVE provisional wave's POC line (never recorded until confirmed); its "newer wave" effect on tested lines; whether the line is in
// the visible price range / wide enough to draw in px.
struct WpRec { int n = 0, dir = 0; long long t0 = 0, t1 = 0, tConf = 0; bool havePoc = false; long long poc = 0; long long seq = 0; };
struct WpBar { long long t = 0, h = 0, l = 0, c = 0; };   // bar END local seconds, prices in ticks
static const int WP_EACH_SIDE = 2;                        // = wpc::POC_EXT_EACH_SIDE
inline long long wpDay(long long t) { const long long a = t + 7 * 3600; return a >= 0 ? a / 86400 : -((-a + 86399) / 86400); }   // = wpc::tday
inline bool wpTicks(const std::string& s, double tick, long long& out) { if (s.empty() || s.size() > 40) return false; char* e = nullptr; const double v = std::strtod(s.c_str(), &e); if (!e || *e || !(tick > 0) || !(v > 0) || !(v < 1e12)) return false; const double q = v / tick; if (!(q < 4e15)) return false; out = (long long)std::llround(q); return true; }
inline bool wpLL(const std::string& s, long long& out) { if (s.empty() || s.size() > 20) return false; char* e = nullptr; const long long v = std::strtoll(s.c_str(), &e, 10); if (!e || *e) return false; out = v; return true; }
// the record, fed line by line (incremental: the host feeds only the new complete lines of the append-only file)
struct WpFeed {
    std::map<std::pair<long long, long long>, WpRec> w;   // (t0, t1) -> wave (+ its POC)
    int nNewest = 0; long long seq = 0, bad = 0, lines = 0;
    void line(const std::string& raw, double tick) {
        if (raw.size() < 5 || raw[0] == '#') return;
        const bool wave = raw.compare(0, 5, "WAVE,") == 0, node = raw.compare(0, 5, "NODE,") == 0; if (!wave && !node) return;   // ROWS / STOP / LNODE ...: skipped unparsed
        ++lines; std::vector<std::string> f; { std::string c; for (char ch : raw) { if (ch == ',') { f.push_back(c); c.clear(); } else if (ch != '\r' && ch != '\n') c += ch; } f.push_back(c); }
        if (wave) {
            long long n = 0, t0 = 0, t1 = 0, tc = 0;
            if (f.size() < 19 || !wpLL(f[2], n) || n < 1 || n > 50 || (f[3] != "up" && f[3] != "down") || !wpLL(f[4], t0) || !wpLL(f[5], t1) || !wpLL(f[6], tc) || t0 <= 0 || t1 < t0 || tc < t1) { ++bad; return; }
            WpRec& r = w[std::make_pair(t0, t1)]; r.n = (int)n; r.dir = f[3] == "up" ? 1 : -1; r.t0 = t0; r.t1 = t1; r.tConf = tc; r.seq = ++seq; nNewest = (int)n;
        } else {
            long long t0 = 0, t1 = 0, rk = 0, tk = 0;
            if (f.size() < 7 || !wpLL(f[1], t0) || !wpLL(f[2], t1) || !wpLL(f[3], rk) || !wpTicks(f[4], tick, tk)) { ++bad; return; }
            if (rk != 0) return;
            std::map<std::pair<long long, long long>, WpRec>::iterator it = w.find(std::make_pair(t0, t1)); if (it == w.end()) return;   // a torn / missing WAVE line
            it->second.havePoc = true; it->second.poc = tk;
        }
    }
    void prune(long long keepFrom) { for (std::map<std::pair<long long, long long>, WpRec>::iterator it = w.begin(); it != w.end();) { if (it->second.t1 < keepFrom) it = w.erase(it); else ++it; } }
    std::vector<WpRec> waves() const { std::vector<WpRec> v; for (const auto& kv : w) v.push_back(kv.second); return v; }
};
inline std::string wpPx(long long tk, double tick) {
    int d = 0; for (double x = tick; d < 6 && std::fabs(x - std::round(x)) > 1e-9; x *= 10) ++d;
    char b[48]; std::snprintf(b, sizeof b, "%.*f", d, (double)tk * tick); return b;
}
struct WpLine { long long tick = 0; bool open = true; long long t0 = 0, t1 = 0; int hit = -1; };
// the drawn lines at the last bar of `bars` (closed bars, time order); n = the chart's reversal bars (0 = the newest WAVE line's)
inline std::vector<WpLine> wpocLines(const std::vector<WpRec>& recs, const std::vector<WpBar>& bars, int n) {
    std::vector<WpLine> out; if (bars.empty()) return out;
    const WpBar& L = bars.back();
    // the engine's waves at this bar: confirmed by it, this N, an extended leg replaces the earlier one (same dir + start, later end)
    std::map<std::pair<int, long long>, WpRec> best;
    for (const WpRec& r : recs) { if (r.n != n || r.tConf > L.t) continue; const std::pair<int, long long> k(r.dir, r.t0);
        std::map<std::pair<int, long long>, WpRec>::iterator it = best.find(k); if (it == best.end() || r.t1 > it->second.t1) best[k] = r; }
    std::vector<WpRec> ws; for (const auto& kv : best) ws.push_back(kv.second);
    std::stable_sort(ws.begin(), ws.end(), [](const WpRec& a, const WpRec& b) { return a.tConf != b.tConf ? a.tConf < b.tConf : a.seq < b.seq; });   // confirmation order
    struct C { WpLine l; double d; };
    std::vector<C> up, dn; std::vector<WpLine> tested;
    const long long day = wpDay(L.t);
    for (const WpRec& w : ws) {
        if (!w.havePoc || wpDay(w.t1) != day) continue;
        int hit = -1; for (int k = 0; k < (int)bars.size(); ++k) { const WpBar& b = bars[(size_t)k]; if (b.t <= w.t1) continue; if (b.l <= w.poc && w.poc <= b.h) { hit = k; break; } }
        WpLine l; l.tick = w.poc; l.t0 = w.t0; l.t1 = w.t1;
        if (hit < 0) { C c; c.l = l; c.d = (double)std::llabs(w.poc - L.c); (w.poc >= L.c ? up : dn).push_back(c); continue; }
        bool newer = false; for (const WpRec& o : ws) if (!(o.t0 == w.t0 && o.t1 == w.t1) && o.dir == w.dir && o.t0 > w.t0) { newer = true; break; }
        if (!newer) { l.open = false; l.hit = hit; tested.push_back(l); }
    }
    auto near = [](std::vector<C>& a) { std::stable_sort(a.begin(), a.end(), [](const C& x, const C& y) { return x.d < y.d; }); if ((int)a.size() > WP_EACH_SIDE) a.resize((size_t)WP_EACH_SIDE); };
    near(up); near(dn);
    for (const C& c : up) out.push_back(c.l);
    for (const C& c : dn) out.push_back(c.l);
    for (const WpLine& l : tested) out.push_back(l);
    return out;
}
// the box levels: one per distinct POC price, zone +-HALF, "WPOC <price>"
inline std::vector<Level> wpocLevels(const std::vector<WpLine>& lines, double tick) {
    std::vector<Level> v; std::vector<long long> seen;
    for (const WpLine& l : lines) {
        if (l.tick <= HALF || std::find(seen.begin(), seen.end(), l.tick) != seen.end()) continue;
        seen.push_back(l.tick); Level lv; lv.name = "WPOC " + wpPx(l.tick, tick); lv.type = L_WPOC; lv.lo = l.tick - HALF; lv.hi = l.tick + HALF; v.push_back(lv);
    }
    return v;
}

// ---- squares: per candle, the seconds the box showed and their colours ------------------------------------------------------------
struct SqAcc { long long secs = 0, red[2] = {0, 0}, green[2] = {0, 0}; std::string level; };
struct Square { int col[2] = {GRAY, GRAY}; long long secs = 0; std::string level; };
inline Square squareOf(const SqAcc& a) {
    Square s; s.secs = a.secs; s.level = a.level;
    for (int r = 0; r < 2; ++r) s.col[r] = a.red[r] >= SQ_SECS ? RED : (a.green[r] >= SQ_SECS ? GREEN : GRAY);
    return s;
}
// the candle (bar END key) second E belongs to: [K - spb, K)
inline long long candleOf(long long E, int spb) { if (spb <= 0) return 0; const long long q = E >= 0 ? E / spb : -((-E + spb - 1) / spb); return (q + 1) * spb; }

// ---- the params file (lsFlexLevels\LRA-BookLevel-<MKT>.csv, written by lra.book_level_combo's export) ------------------------------
//   VERSION|1 / MARKET|<MKT> / AT|<CT> / PARAM|<name>|<value> / SHOW|D|0/1 / SHOW|B|0/1 / PROFILE|<YYYY-MM-DD>|POC|<px> (VAH, VAL)
struct Profile { std::string day; double poc = 0, vah = 0, val = 0; bool ok = false; };
inline bool num(const std::string& s, double& v) { if (s.empty() || s.size() > 40) return false; char* e = nullptr; const double x = std::strtod(s.c_str(), &e); if (!e || *e || !(x == x) || x > 1e12 || x < -1e12) return false; v = x; return true; }
inline std::vector<std::string> split(const std::string& ln) { std::vector<std::string> f; std::string c; for (char ch : ln) { if (ch == '|') { f.push_back(c); c.clear(); } else if (ch != '\r' && ch != '\n') c += ch; } f.push_back(c); return f; }
inline bool parseParams(const std::vector<std::string>& lines, const std::string& market, Params& out, Profile& prof) {
    Params P; Profile pr; bool ver = false, mkt = false;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::vector<std::string> f = split(lines[i]); if (f.empty() || f[0].empty() || f[0][0] == '#') continue;
        if (!ver) { if (f.size() == 2 && f[0] == "VERSION" && f[1] == "1") { ver = true; continue; } out.why = "garbled (VERSION first)"; return false; }
        if (f[0] == "MARKET") { if (f.size() != 2 || f[1] != market) { out.why = "wrong market"; return false; } mkt = true; continue; }
        if (f[0] == "PARAM" && f.size() == 3) {
            double v; if (!num(f[2], v) || v <= 0 || v > 100000) { out.why = "bad PARAM " + f[1]; return false; }
            if (f[1] == "def_x") P.defX = v; else if (f[1] == "quiet_lots") P.quiet = v; else if (f[1] == "behind_stack_x") P.behStack = v;
            else if (f[1] == "behind_thin_x") P.behThin = v; else if (f[1] == "att_stack_x") P.attStack = v; else if (f[1] == "att_pull_x") P.attPull = v;
            continue;
        }
        if (f[0] == "SHOW" && f.size() == 3 && (f[2] == "0" || f[2] == "1")) { if (f[1] == "D") P.showD = f[2] == "1"; if (f[1] == "B") P.showB = f[2] == "1"; continue; }
        if (f[0] == "PROFILE" && f.size() == 4) { double v; if (!num(f[3], v) || v <= 0) continue; pr.day = f[1]; if (f[2] == "POC") pr.poc = v; else if (f[2] == "VAH") pr.vah = v; else if (f[2] == "VAL") pr.val = v; continue; }
    }
    if (!ver || !mkt) { out.why = !ver ? "garbled (no VERSION)" : "no MARKET line"; return false; }
    if (!(P.behThin < 1.0) || !(P.defX >= 1.0) || !(P.behStack >= 1.0) || !(P.attStack >= 1.0) || !(P.attPull >= 1.0)) { out.why = "thresholds out of range"; return false; }
    pr.ok = pr.poc > 0 && pr.vah > 0 && pr.val > 0 && pr.val <= pr.poc && pr.poc <= pr.vah;
    P.file = true; P.why = "file"; out = P; prof = pr; return true;
}

// ---- Pb zones from lsBreakout's BBO records ----------------------------------------------------------------------------------------
struct BoRec { long long t = 0, t0 = 0, hk = 0, lk = 0; int dir = 0; };
// one BBO line (27 fields, fired, the zz version) -> BoRec; false = not a fired 3-bar line
inline bool parseBbo(const std::string& ln, BoRec& r) {
    const std::vector<std::string> f = split(ln);
    if (f.size() != 27 || f[0] != "BBO" || f[4] != "1" || f[23].find("zz") == std::string::npos) return false;
    auto ll = [](const std::string& s, long long& v) { if (s.empty() || s.size() > 20) return false; char* e = nullptr; const long long x = std::strtoll(s.c_str(), &e, 10); if (!e || *e) return false; v = x; return true; };
    long long d = 0; if (!ll(f[1], r.t) || !ll(f[2], r.t0) || !ll(f[3], d) || !ll(f[8], r.hk) || !ll(f[9], r.lk)) return false;
    if ((d != 1 && d != -1) || r.t <= 0 || r.t0 <= 0 || r.hk < r.lk || r.lk <= 0) return false;
    r.dir = (int)d; return true;
}
// Python's round() (half to even) of (hk + lk) / 2 - bo_pb.pb_windows' mid
inline long long midTicks(long long hk, long long lk) { const long long s = hk + lk; if (s % 2 == 0) return s / 2; const long long lo = (s - 1) / 2; return lo % 2 == 0 ? lo : lo + 1; }
// pivots of the 3-bar High / Low ZigZag for the origin rule: (pivot bar time, confirm bar time, type +1 high / -1 low, price ticks)
struct Piv { long long t = 0, tc = 0; int type = 0; long long h = 0, l = 0; };
struct PbIn { long long t = 0, h = 0, l = 0; };   // the chart's closed bars (END stamps, ticks)
// the Pb windows (bo_pb.pb_windows): zone, from the fire bar until the next fired BO / BD fires or a bar trades beyond the wave origin
inline std::vector<Level> pbLevels(const std::vector<BoRec>& F0, const std::vector<PbIn>& bars, const std::vector<Piv>& piv) {
    std::vector<BoRec> F(F0); std::sort(F.begin(), F.end(), [](const BoRec& a, const BoRec& b) { return a.t0 < b.t0; });
    std::vector<Level> out;
    for (size_t q = 0; q < F.size(); ++q) {
        const BoRec& D = F[q]; const int dr = D.dir > 0 ? 1 : -1, want = dr > 0 ? -1 : 1;
        bool haveOrg = false; long long org = 0;
        for (const Piv& p : piv) { if (p.t >= D.t0) break; if (p.type == want && p.tc <= D.t) { haveOrg = true; org = dr > 0 ? p.l : p.h; } }
        long long tTo = 0;
        for (size_t r = q + 1; r < F.size(); ++r) if (F[r].t0 > D.t0) tTo = tTo == 0 ? F[r].t : std::min(tTo, F[r].t);
        if (haveOrg) for (const PbIn& b : bars) { if (b.t <= D.t0) continue; if (dr > 0 ? b.l < org : b.h > org) { tTo = tTo == 0 ? b.t : std::min(tTo, b.t); break; } }
        const long long mid = midTicks(D.hk, D.lk);
        Level lv; lv.type = L_PB; lv.fixedZone = true; lv.lo = dr > 0 ? D.lk : mid; lv.hi = dr > 0 ? mid : D.hk; lv.tFrom = D.t; lv.tTo = tTo;
        lv.name = dr > 0 ? "Pb BO" : "Pb BD";
        out.push_back(lv);
    }
    return out;
}

// ---- the line as drawn (one row, after the pane's Defending? line): level | Defenders .. | Behind .. | Attackers .. --------------------
// colours 0x00RRGGBB: green / red as his panes, gray = nothing, the level name white, the separators dim
static const unsigned long BC_GREEN = 0x0022C55E, BC_RED = 0x00EF4444, BC_GRAY = 0x009CA3AF, BC_NAME = 0x00FFFFFF, BC_SEP = 0x006B7280, BC_BG = 0x00141414;
static const int BOX_PT = 10, BOX_PAD = 4, BOX_GAP = 6, BOX_H = 16, BOX_AFTER = 6;   // one row = the pane's question strip (16 px)
inline unsigned long colOf(int c) { return c == GREEN ? BC_GREEN : c == RED ? BC_RED : BC_GRAY; }
struct Seg { std::string text; unsigned long color = 0; int x = 0, w = 0; };
struct Lay { bool ok = false; int l = 0, t = 0, r = 0, b = 0; std::vector<Seg> segs; int dropped = 0; };
// left = where the row starts (after the Defending? line), right = the last x allowed (the price axis edge); parts drop from the right
// (Attackers, then Behind, then Defenders); the level name alone is never drawn
template <class M> inline Lay layout(const BoxText& t, int left, int top, int right, M measure) {
    Lay L; L.l = left; L.t = top; L.b = top + BOX_H;
    if (t.level.empty()) return L;
    for (int keep = 3; keep >= 1; --keep) {
        std::vector<Seg> v; Seg n; n.text = t.level; n.color = BC_NAME; v.push_back(n);
        for (int q = 0; q < keep; ++q) { Seg sp; sp.text = "|"; sp.color = BC_SEP; v.push_back(sp); Seg p; p.text = t.part[q]; p.color = colOf(t.col[q]); v.push_back(p); }
        int x = left + BOX_PAD;
        for (size_t k = 0; k < v.size(); ++k) { v[k].w = measure(v[k].text); v[k].x = x; x += v[k].w + (k + 1 < v.size() ? BOX_GAP : 0); }
        const int r = x + BOX_PAD;
        if (r <= right) { L.ok = true; L.r = r; L.segs = v; L.dropped = 3 - keep; return L; }
    }
    return L;
}
// the two square rows under a candle's bars: rows of SQ_H px with a 1 px gap, starting SQ_TOP px under the zero row
static const int SQ_TOP = 2, SQ_H = 3, SQ_GAP = 1, SQ_RESERVE = SQ_TOP + 2 * SQ_H + SQ_GAP + 2;   // 10 px kept under the bars (only while squares are in view)
inline int sqRowTop(int zeroRow, int row) { return zeroRow + SQ_TOP + row * (SQ_H + SQ_GAP); }
// the record lines (lsFlexLevels\BookFlow\<MKT>-<SYM>-<spb>-<session>-levelbox.csv)
inline char colChar(int c) { return c == GREEN ? 'g' : c == RED ? 'r' : 'n'; }
inline int charCol(char c) { return c == 'g' ? GREEN : c == 'r' ? RED : GRAY; }
inline std::string sqLine(long long key, const Square& q, const SqAcc& a) {
    char b[200]; std::snprintf(b, sizeof b, "C|%lld|%c|%c|%lld|%lld|%lld|%lld|%lld|", key, colChar(q.col[0]), colChar(q.col[1]), q.secs, a.red[0], a.green[0], a.red[1], a.green[1]);
    std::string n; for (char ch : q.level) n += (ch == '|' || ch == '\n' || ch == '\r') ? ' ' : ch; return b + n;
}
inline bool parseSq(const std::string& ln, long long& key, Square& q) {
    const std::vector<std::string> f = split(ln); if (f.size() != 10 || f[0] != "C" || f[2].size() != 1 || f[3].size() != 1) return false;
    char* e = nullptr; const long long k = std::strtoll(f[1].c_str(), &e, 10); if (!e || *e || k <= 0) return false;
    const long long sc = std::strtoll(f[4].c_str(), &e, 10); if (!e || *e || sc < 0) return false;
    const char a = f[2][0], b = f[3][0]; if ((a != 'g' && a != 'r' && a != 'n') || (b != 'g' && b != 'r' && b != 'n')) return false;
    key = k; q = Square(); q.col[0] = charCol(a); q.col[1] = charCol(b); q.secs = sc; q.level = f[9]; return true;
}
inline std::string stateLine(long long E, const std::string& clock, const Engine::Shown& sh, const BoxText& t) {
    std::string o = "S|" + std::to_string(E) + "|" + clock + "|";
    if (!sh.on) return o + "off";
    char b[160]; std::snprintf(b, sizeof b, "%s|%s|%lld|%lld|%lld|%lld|", ltypeName(sh.c.type), sh.c.ds == 0 ? "support" : "resistance", sh.c.arrived ? sh.c.anchor : 0LL, sh.k, sh.c.lo, sh.c.hi);
    std::string n; for (char ch : sh.c.name) n += (ch == '|') ? ' ' : ch;
    o += n + "|" + b;
    for (int q = 0; q < 3; ++q) { char x[64]; std::snprintf(x, sizeof x, "%d|%.2f|%c|", sh.r[q].word, sh.r[q].x, colChar(t.col[q])); o += x; }
    return o + t.part[0] + " ; " + t.part[1] + " ; " + t.part[2];
}
// a state worth a record line: the level, the side, the words and colours (not the ratio - it moves every second)
inline std::string stateKey(const Engine::Shown& sh) {
    if (!sh.on) return "off";
    std::string k = sh.c.name + "/" + std::to_string(sh.c.ds) + "/" + std::to_string(sh.c.anchor * (sh.c.arrived ? 1 : 0));
    for (int q = 0; q < 3; ++q) k += "/" + std::to_string(sh.r[q].word) + std::to_string(sh.r[q].col);
    return k;
}

} // namespace blb
#endif
