#ifndef LS_STOPRUN_LOGIC_H
#define LS_STOPRUN_LOGIC_H
// StopRunLogic.h (1.2.0, 2026-10-22) -- SDK-free rules of lsStopRun, the "St" / "Sta" STOP-RUN mark on the PRICE chart.
// Rassul 2026-10-22 15:33-15:34 CT (MBO Roadmap row "Were stops triggered - and were they absorbed after?", T-20261022-15) "show yes";
// 15:37 CT "Sta can indicate the stop run absorbed"; 1.1.0 approved 16:13 CT (real levels only, live decision).
// 1.2.0 (Rassul approved 17:39 CT the Sr mockup v4 rule "and the SAME swing rule on St / Sta"; 18:05 CT "St / Sta use the SAME
// reference as Sr - last 3 swings, not most recent only"): the reference = SwingRef.h (an intact swing among the last 3 swings of the
// 1-bar ZigZag from the RTH open, or a key / MenthorQ level; 5-candle extremes no longer count). The St / Sta validation
// (VALIDATE-STOPRUN-SWEEP-2026-10-22.md, iteration 3, held-out tested) per market from lsFlexLevels\LRA-StopSweep-<MKT>.csv:
// burst >= stRatio (3.0x) normal; Sta decided LIVE at burst end + staReclaimSec (NQ 10 s) = the last trade is back across the level
// AND >= 1 resting order at / beyond the level filled >= staBigLots lots (NQ 5); the 1.1 "50% eaten" and "no new extreme" rules are
// gone; staDecideLive 0 = decided at candle closes (a close back across + the big resting fill within staAbsorbBars candles).
// Drawn per verdict: SHOW, ? ("Sta?" / "St?"), LOG = recorded only (today: NQ green Sta "?", GC grey St "?", everything else LOG).
// On a candle that also has an Sr (lsSweepRefill) only the Sr is drawn (the host reads the Sr record).
//
// WHAT IS DRAWN (when its verdict is not LOG)
//   * grey "St" where stops fired through a reference, on that candle (decided once, the moment the burst ends);
//   * the decision upgrades it once to "Sta": green = stops BELOW a low absorbed (bullish), red = ABOVE a high (bearish);
//     not absorbed = grey "St" (continuation). Never changed again. Both lines (ST fire, SA decision) are recorded.
//
// DETECTION (ATAS Stops Tracker groups a burst by a ms window + min ticks + min count; Bookmap Stops & Icebergs uses the MBO event
// order; CME MDP elects stops as aggressor orders that never rested, in the same match event):
//   1. BURST: aggressive fills on ONE side, each within `windowMs` (50 ms) of the burst's first fill and in the same or next second.
//   2. THROUGH A REFERENCE (SwingRef.h, the same rule as Sr; incl. the volume-profile POC / VAH / VAL, prior day, naked POCs): the burst ENDS beyond the swing / level, STARTS <= 0.5 ATR past it and
//      at most `nearTicks` before it, and no candle before the last one had traded through it; the deepest one wins.
//   3. SIZE: >= `minFills` fills AND lots >= `ratio` (3.0x) x the NORMAL STOP BURST = the median lots of the bursts with >= minFills
//      fills in the same 30-min slot this session (>= 20 of them), else the last 300 such bursts of any slot (>= 20), else the seed.
//   4. MANY ORDERS: when the feed names aggressor orders, >= `minIds` (2) distinct aggressor ids.
//   St price = the first fill price beyond the level.
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "SwingRef.h"

namespace stl {

static const char* const VERSION = "1.2.1";
enum Side { DOWN = -1, UP = 1 };            // DOWN = sellers hit bids through a low; UP = buyers lift offers through a high
enum Outcome { PENDING = 0, ABSORBED = 1, NOT_ABSORBED = 2 };
enum RefKind { REF_KEY = 0, REF_X5 = 1, REF_PRIOR = 2, REF_SWING = 3, REF_MQ = 4, REF_VP = 5 };   // REF_X5 / REF_PRIOR: 1.0 / 1.1 records only
static const int SLOT_MIN = 20, ANY_MAX = 300, IDS_CAP = 64, PX_CAP = 512, REST_CAP = 4096, PENDING_CAP = 64;

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }

// ---- per-market defaults. seed = the median lots of a >= 5-fill burst in the replay (Databento MBO: ES 10-06..09 = 14-16, NQ 10-09 = 10,
//      CL 10-08 = 11, GC 10-08 = 11; HG / NG / 6E not replayed yet = 6 / 6 / 8). Used only until 20 bursts of the session are seen.
struct MarketDefaults { int nearTicks; int bigLots; double seedLots; };
inline MarketDefaults marketDefaults(const std::string& m) {
    if (m == "ES") return { 4, 20, 15 };
    if (m == "NQ") return { 8, 5, 10 };
    if (m == "CL") return { 5, 5, 11 };
    if (m == "GC") return { 5, 3, 11 };
    if (m == "HG") return { 3, 3, 6 };
    if (m == "NG") return { 3, 3, 6 };
    if (m == "EU" || m == "6E") return { 3, 5, 8 };
    return { 4, 10, 8 };
}

struct Params {
    int windowMs = 50, minFills = 5, nearTicks = 4, bigLots = 5, minIds = 2;
    int reclaimS = 10, absorbBars = 1, refPivots = 3; bool decideLive = true, useKey = true, useMq = true;
    double ratio = 3.0, seedLots = 14;
    int showBull = swg::LOG, showBear = swg::LOG, showGrey = swg::LOG;   // the verdicts (params file), stamped on each mark
};
// the per-market params file -> the detector (stRefPivots is NOT applied: Rassul 18:05 "the SAME reference as Sr", the last 3 swings)
inline void applyParams(Params& P, const swg::StopSweep& X) {
    P.windowMs = X.stWindowMs; P.minFills = X.stMinFills; P.ratio = X.stRatio; P.nearTicks = X.stNearTicks; P.bigLots = X.staBigLots;
    P.decideLive = X.staDecideLive != 0; P.reclaimS = X.staReclaimSec; P.absorbBars = X.staAbsorbBars; P.refPivots = 3;
    P.useKey = X.stKey != 0; P.useMq = X.stMq != 0; P.showBull = X.staBull; P.showBear = X.staBear; P.showGrey = X.stGrey;
}

// ---- inputs ------------------------------------------------------------------------------------------------------------------
struct Ref { std::string name; long long px = 0; int dir = 0; int kind = REF_KEY; int rank = 9; };   // dir +1 = a high, -1 = a low
// a session level: dir +1 / -1, rank 0-5. A MenthorQ level (either side can run through it): dir 2, rank 6-14. Else 0 (not used).
inline int keyDir(const std::string& n, int& rank) {
    static const char* const H[6] = { "PFH", "PDH", "PWH", "ONH", "LonHI", "AsiaHI" }, * const L[6] = { "PFL", "PDL", "PWL", "ONL", "LonLO", "AsiaLO" };
    for (int i = 0; i < 6; ++i) { if (n == H[i]) { rank = i; return 1; } if (n == L[i]) { rank = i; return -1; } }
    static const char* const MQ[9] = { "CR", "PS", "HVL", "GW0", "CR0", "PS0", "HVL0", "1DH", "1DL" };
    for (int i = 0; i < 9; ++i) if (n == MQ[i]) { rank = 6 + i; return 2; }
    return 0;
}
inline bool isMq(const std::string& n) { int r = 0; return keyDir(n, r) == 2; }
struct Bar { long long t = 0, o = 0, h = 0, l = 0, c = 0; long long day = 0; bool rth = false; };   // a CLOSED candle in ticks; t = its END; day / rth from the host (CT)

// ---- the record of one stop run ------------------------------------------------------------------------------------------------
struct Event {
    std::string key;                 // "<fired_sec>-<side>-<st_px ticks>" unique per run
    long long barT = 0, firedSec = 0, firedMs = 0;
    int side = 0;                    // DOWN / UP
    std::string ref; int refKind = REF_KEY; long long refPx = 0, stPx = 0;
    long long lots = 0, fills = 0, ids = -1;   // ids -1 = the feed did not name aggressor orders
    double norm = 0, ratio = 0; int slot = -1;
    bool orphan = false;             // host: reloaded undecided after a restart (its order flow is gone - it can no longer be decided)
    bool drawn = true;               // false = the candle already had 2 marks (recorded, not drawn)
    int outcome = PENDING; long long decidedT = 0; int bars = 0; int bigIds = 0; double excAtr = 0; bool reclaimed = false;
    long long decidedSec = 0, decidedMs = 0;   // the exchange second / feed ms of the LIVE decision (decidedSec 0 = a 1.0.0 record)
    int sharePct = -1;                         // 1.1 records: % of the burst's lots taken by big resting orders (-1 = not known)
    bool newExt = false;                       // price made a new extreme beyond the burst before the decision
    long long refT = 0; int refBack = 0; std::string refCt;   // the swing's candle (END; 0 = a level), swings back, host "8:48 AM"
    int bigLotsMax = 0;                        // the biggest resting order filled at / beyond the level (lots)
    int showFire = swg::SHOW, show = swg::SHOW;   // the verdicts stamped when fired (grey St) and when decided (Sta / St); old records = SHOW
    std::string ct;                  // 12-hour Central time of the fire, for the hover / record
};
inline int bullish(const Event& e) { return e.side == DOWN ? 1 : 0; }   // an absorbed DOWN run = trapped sellers = bullish

// ---- the detector --------------------------------------------------------------------------------------------------------------
struct Burst {
    bool on = false; int side = 0; long long startMs = 0, lastMs = 0, startSec = 0; long long firstPx = 0, lo = 0, hi = 0;
    long long lots = 0, fills = 0; std::vector<long long> ids; bool idsKnown = false; std::vector<long long> pxs;
    // Resting executions retain their price: a single order id can have fills before and through the reference.  At close, only
    // its through-reference lots are transferred to Pending, so pre-level partial fills cannot satisfy the Sta big-order rule.
    std::map<std::pair<long long, long long>, long long> rest;   // (resting order id, fill price) -> lots; bounded by REST_CAP
    void clear() { *this = Burst(); }
};
// a fired run waiting for its live decision: resting fills at / beyond the level since the burst started, the burst's extreme, reclaim
struct Pending { Event e; std::map<long long, long long> rest; long long ext = 0; int k = 0; long long endMs = 0, endSec = 0, extPx = 0, lastPx = 0; bool reclaimed = false, newExt = false, closeBack = false; };

inline long long median(std::vector<long long> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end()); const size_t n = v.size();
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2] + 1) / 2;
}
inline int slotOfMinute(int minuteOfDay) { return minuteOfDay < 0 || minuteOfDay >= 1440 ? -1 : minuteOfDay / 30; }

struct Detector {
    Params P;
    Burst b;
    std::map<int, std::vector<long long> > slotLots;    // qualifying-burst lots per 30-min slot (this session)
    std::vector<long long> anyLots;                     // the last ANY_MAX qualifying bursts
    std::vector<Pending> pending;
    std::vector<Event> fired, decided;                  // output queues (the host drains them)
    long long bursts = 0, qualifying = 0, nearLevel = 0, sized = 0, runs = 0, lastCloseT = 0;
    // the context the host keeps current (known data only: closed candles, the forming candle's key)
    std::vector<swg::Lvl> levels; swg::ZigZag Z; swg::BarWin W; swg::VolProf V;
    long long formingT = 0; int minuteOfDay = -1; double atrTicks = 0;
    void setRth(int openMin, int closeMin, int spb) { Z.rthLenSec = closeMin > openMin ? (closeMin - openMin) * 60 : 0; Z.spb = spb > 0 ? spb : 180; }
    std::map<long long, int> marksPerBar;
    std::set<std::pair<int, long long> > used;           // (side, level px): the stops at a level fire ONCE per session
    void markUsed(int side, long long px) { used.insert(std::make_pair(side, px)); }

    double normNow(int slot) const {
        std::map<int, std::vector<long long> >::const_iterator it = slotLots.find(slot);
        if (it != slotLots.end() && (int)it->second.size() >= SLOT_MIN) return (double)median(it->second);
        if ((int)anyLots.size() >= SLOT_MIN) return (double)median(anyLots);
        return P.seedLots > 0 ? P.seedLots : 1;
    }
    // ---- the reference this burst ran through (SwingRef.h, the same rule as Sr) + at most nearTicks before it; none = no stop run --
    bool pickRef(const Burst& x, Ref& out, long long& fromT, int& back) const {
        swg::Sweep w; w.down = x.side == DOWN; w.ext = w.down ? x.lo : x.hi; w.start = x.firstPx; w.barT = formingT;
        swg::Params sp; sp.maxBack = P.refPivots; sp.useKey = P.useKey; sp.useMq = P.useMq; sp.maxBefore = P.nearTicks;
        std::vector<swg::Lvl> all = levels; V.levels(all);                              // + the volume-profile levels (closed candles)
        const swg::Ref r = swg::pick(Z, W, all, w, Z.spb, sp);
        if (!r.ok || used.count(std::make_pair(x.side, r.px))) return false;     // the stops at a reference fire once per session
        out.name = r.name; out.px = r.px; out.dir = w.down ? -1 : 1; out.kind = r.kind == swg::SWING ? REF_SWING : r.kind == swg::MQ ? REF_MQ : r.kind == swg::VP ? REF_VP : REF_KEY;
        fromT = r.fromT; back = r.back; return true;
    }
    void close(long long nowMs) {
        if (!b.on) return;
        Burst x = b; b.clear(); (void)nowMs;
        ++bursts;
        const int slot = slotOfMinute(minuteOfDay);
        if (x.fills < P.minFills) return;
        ++qualifying;
        const double norm = normNow(slot);
        std::vector<long long>& sl = slotLots[slot]; sl.push_back(x.lots); if (sl.size() > 2000) sl.erase(sl.begin());   // AFTER the norm: no look-ahead
        anyLots.push_back(x.lots); if ((int)anyLots.size() > ANY_MAX) anyLots.erase(anyLots.begin());
        Ref r; long long fromT = 0; int back = 0; if (!pickRef(x, r, fromT, back)) return;
        ++nearLevel;
        const double ratio = norm > 0 ? (double)x.lots / norm : 0;
        if (ratio + 1e-9 < P.ratio) return;
        ++sized;
        if (x.idsKnown && (int)x.ids.size() < P.minIds) return;
        // the first fill price beyond the level
        long long st = x.side == DOWN ? r.px - 1 : r.px + 1;
        for (size_t i = 0; i < x.pxs.size(); ++i) if (x.side == DOWN ? x.pxs[i] < r.px : x.pxs[i] > r.px) { st = x.pxs[i]; break; }
        Event e;
        e.barT = formingT; e.firedSec = x.startSec; e.firedMs = x.startMs; e.side = x.side; e.ref = r.name; e.refKind = r.kind; e.refPx = r.px; e.stPx = st; e.refT = fromT; e.refBack = back; e.showFire = P.showGrey;
        e.lots = x.lots; e.fills = x.fills; e.ids = x.idsKnown ? (long long)x.ids.size() : -1; e.norm = norm; e.ratio = ratio; e.slot = slot;
        char k[64]; std::snprintf(k, sizeof k, "%lld-%s-%lld", x.startSec, x.side == DOWN ? "d" : "u", st); e.key = k;
        int& m = marksPerBar[e.barT]; e.drawn = m < 2; if (e.drawn) ++m;
        ++runs; markUsed(x.side, r.px);
        Pending p; p.e = e; p.ext = 0; p.endMs = x.lastMs; p.endSec = x.startSec; p.extPx = x.side == DOWN ? x.lo : x.hi; p.lastPx = x.side == DOWN ? x.lo : x.hi;
        for (std::map<std::pair<long long, long long>, long long>::const_iterator it = x.rest.begin(); it != x.rest.end(); ++it)
            if (x.side == DOWN ? it->first.second <= r.px : it->first.second >= r.px) p.rest[it->first.first] += it->second;
        if (pending.size() >= (size_t)PENDING_CAP) pending.erase(pending.begin());
        pending.push_back(p);
        fired.push_back(e);
    }
    // ---- feed: one AGGRESSIVE fill. side: DOWN = a seller hit a bid, UP = a buyer lifted an offer. aggId 0 = not named ----------
    void aggFill(long long ms, long long sec, int side, long long px, long long lots, long long aggId) {
        lastMs = ms; lastSec = sec;
        if ((side != DOWN && side != UP) || lots <= 0 || lots > 10000000 || px <= 0) return;
        if (b.on && (side != b.side || ms - b.startMs > P.windowMs || ms < b.lastMs || sec - b.startSec > 1 || sec < b.startSec)) close(ms);
        if (!b.on) { b.on = true; b.side = side; b.startMs = ms; b.startSec = sec; b.firstPx = px; b.lo = px; b.hi = px; }
        b.lastMs = ms; b.lots += lots; ++b.fills; if (px < b.lo) b.lo = px; if (px > b.hi) b.hi = px;
        if ((int)b.pxs.size() < PX_CAP) b.pxs.push_back(px);
        if (aggId != 0) { b.idsKnown = true; if ((int)b.ids.size() < IDS_CAP && std::find(b.ids.begin(), b.ids.end(), aggId) == b.ids.end()) b.ids.push_back(aggId); }
        trade(ms, sec, px);
    }
    // every trade after a run fired: back across the level? a new extreme beyond the burst? then decide what can be decided now
    void trade(long long ms, long long sec, long long px) {
        for (size_t i = 0; i < pending.size(); ++i) {
            Pending& p = pending[i];
            p.lastPx = px;
            if (p.e.side == DOWN) { if (px > p.e.refPx) p.reclaimed = true; if (px < p.extPx) { p.extPx = px; p.newExt = true; } }
            else { if (px < p.e.refPx) p.reclaimed = true; if (px > p.extPx) { p.extPx = px; p.newExt = true; } }
        }
        decide(ms, sec);
    }
    // the resting orders at / beyond the level filled >= bigLots (count) and the biggest one
    int bigCount(const Pending& p, int& maxLots) const {
        int big = 0; maxLots = 0;
        for (std::map<long long, long long>::const_iterator it = p.rest.begin(); it != p.rest.end(); ++it) { if (it->second >= P.bigLots) ++big; if (it->second > maxLots) maxLots = (int)std::min(it->second, 1000000LL); }
        return big;
    }
    void finish(size_t i, bool absorbed, long long ms, long long sec) {
        Pending& p = pending[i]; Event e = p.e; int mx = 0;
        e.bigIds = bigCount(p, mx); e.bigLotsMax = mx; e.reclaimed = p.reclaimed; e.newExt = p.newExt; e.bars = p.k + 1; e.decidedT = formingT; e.decidedSec = sec; e.decidedMs = ms;
        const long long x = p.e.side == DOWN ? p.e.refPx - p.extPx : p.extPx - p.e.refPx;
        e.excAtr = atrTicks > 0 && x > 0 ? (double)x / atrTicks : 0;
        e.outcome = absorbed ? ABSORBED : NOT_ABSORBED;
        e.show = absorbed ? (p.e.side == DOWN ? P.showBull : P.showBear) : P.showGrey;
        decided.push_back(e);
        pending.erase(pending.begin() + (long)i);
    }
    // LIVE (staDecideLive): at burst end + reclaimS = the last trade is back across the level AND a big resting order (>= bigLots)
    // at / beyond the level was filled. Candle mode: decided at candle closes (barClose).
    void decide(long long ms, long long sec) {
        if (!P.decideLive) return;
        for (size_t i = 0; i < pending.size();) {
            Pending& p = pending[i];
            const bool due = sec - p.endSec > P.reclaimS || ms - p.endMs >= (long long)P.reclaimS * 1000;
            if (!due) { ++i; continue; }
            int mx = 0; const bool back = p.e.side == DOWN ? p.lastPx > p.e.refPx : p.lastPx < p.e.refPx;
            finish(i, back && bigCount(p, mx) >= 1, ms, sec);
        }
    }
    // one RESTING fill (the order that was hit). restSide: DOWN = a resting BID (hit by sellers), UP = a resting offer.
    void restFill(long long ms, int restSide, long long px, long long lots, long long restId) {
        (void)ms;
        if (restId == 0 || lots <= 0 || lots > 10000000) return;
        const std::pair<long long, long long> k = std::make_pair(restId, px);
        if (b.on && restSide == b.side && ((int)b.rest.size() < REST_CAP || b.rest.count(k))) b.rest[k] += lots;
        for (size_t i = 0; i < pending.size(); ++i) {
            Pending& p = pending[i]; if (p.e.side != restSide) continue;
            if (p.e.side == DOWN ? px > p.e.refPx : px < p.e.refPx) continue;
            if ((int)p.rest.size() < REST_CAP || p.rest.count(restId)) p.rest[restId] += lots;
        }
    }
    // time passes with no fill: a burst older than the window is over
    void tick(long long ms, long long sec) { if (b.on && (ms - b.startMs > P.windowMs || sec - b.startSec > 1)) close(ms); decide(ms, sec); }
    long long lastMs = 0, lastSec = 0;
    // ---- a CLOSED candle (strictly in time order): the ZigZag / ATR context; candle mode decides here --------------------------
    // rows = the candle's volume at price (tick -> lots) for the volume-profile levels; session = its 17:00 CT trading day
    void barClose(const Bar& c, const std::vector<std::pair<long long, double> >* rows = nullptr, long long session = 0) {
        if (c.t <= lastCloseT) return;
        lastCloseT = c.t;
        if (rows) V.add(c.t, session, c.h, c.l, *rows);
        swg::ZBar z; z.t = c.t; z.o = c.o; z.h = c.h; z.l = c.l; z.c = c.c; z.day = c.day; z.rth = c.rth;
        Z.add(z); W.add(z); atrTicks = W.atr(c.t + 1);
        for (size_t i = 0; i < pending.size();) {
            Pending& p = pending[i];
            if (c.t < p.e.barT) { ++i; continue; }
            ++p.k;
            if (P.decideLive) { ++i; continue; }
            int mx = 0;
            const bool back = p.e.side == DOWN ? c.c > p.e.refPx : c.c < p.e.refPx;
            if (back && bigCount(p, mx) >= 1) { finish(i, true, lastMs, lastSec); continue; }
            if (p.k >= (P.absorbBars < 1 ? 1 : P.absorbBars)) { finish(i, false, lastMs, lastSec); continue; }
            ++i;
        }
    }
};

// ---- text ------------------------------------------------------------------------------------------------------------------------
inline std::string ratioText(double r) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", !finite(r) || r < 0 ? 0.0 : r > 999 ? 999.0 : r); return b; }
inline std::string pxText(long long ticks, double tick) {
    char b[32]; const double p = (double)ticks * tick;
    int dec = 0; for (double t = tick; dec < 6 && std::fabs(t - std::floor(t + 1e-9)) > 1e-9; t *= 10) ++dec;
    std::snprintf(b, sizeof b, "%.*f", dec, p); return b;
}
// the mark now: decided -> Sta (absorbed) / St (not); pending -> St. Its verdict: decided -> show; pending -> showFire
inline int verdict(const Event& e) { return e.outcome == PENDING ? e.showFire : e.show; }
inline std::string markText(const Event& e) { return std::string(e.outcome == ABSORBED ? "Sta" : "St") + (verdict(e) == swg::QUERY ? "?" : ""); }
inline bool drawn(const Event& e) { return e.drawn && verdict(e) != swg::LOG; }
// 0 = grey, 1 = green (bullish), 2 = red (bearish)
inline int markColour(const Event& e) { return e.outcome == ABSORBED ? (bullish(e) ? 1 : 2) : 0; }
inline std::string secs(long long n) { return n == 1 ? "1 s" : std::to_string(n < 0 ? 0 : n) + " s"; }
inline std::string refText(const Event& e, double tick) {
    if (e.refKind == REF_SWING && e.refT > 0) return "the " + e.ref + " " + pxText(e.refPx, tick) + (e.refCt.empty() ? std::string() : " from " + e.refCt) + swg::backText(e.refBack);
    return e.ref + " " + pxText(e.refPx, tick);
}
inline std::string outcomeText(const Event& e, double tick, int decideS, int bigLots) {
    const std::string idSide = e.side == DOWN ? "bid" : "offer", dir = e.side == DOWN ? "above" : "below";
    if (e.outcome == PENDING && e.orphan) return "Absorbed? not decided - Investor/RT restarted inside the " + secs(decideS) + " window";
    if (e.outcome == PENDING) return "Absorbed? deciding - at " + secs(decideS) + ": back " + dir + " " + pxText(e.refPx, tick) + " and a resting " + idSide + " of " + std::to_string(bigLots) + "+ lots filled there?";
    const long long took = e.decidedSec > 0 && e.firedSec > 0 ? e.decidedSec - e.firedSec : -1;
    const std::string when = took >= 0 ? " at " + secs(took) : std::string();
    if (e.outcome == ABSORBED) return "Absorbed" + when + ": back " + dir + " the level, " + std::to_string(e.bigIds) + " big resting " + idSide + (e.bigIds == 1 ? "" : "s") + " filled (biggest " + std::to_string(e.bigLotsMax) + " lots)";
    if (!e.reclaimed || (e.side == DOWN ? false : false)) return "Not absorbed - continuation: price stayed " + std::string(e.side == DOWN ? "below" : "above") + " the level" + when;
    return "Not absorbed - continuation: no resting " + idSide + " of " + std::to_string(bigLots) + "+ lots filled at the level" + when;
}
inline std::string headline(const Event& e, double tick) {
    return "Stop run at " + pxText(e.stPx, tick) + (e.side == DOWN ? " below " : " above ") + refText(e, tick) + ": " + ratioText(e.ratio) + " normal stop burst";
}
// the hover: header + the question / what it is / why it matters / how to use / example (Rassul's hover structure)
inline std::vector<std::string> hoverRows(const Event& e, double tick, int decideS, int bigLots) {
    std::vector<std::string> r;
    const int v = verdict(e);
    r.push_back(markText(e) + "  Stop run?  " + e.ct + (v == swg::QUERY ? "   (? = promising, not proven yet)" : v == swg::SHOW ? "   (proven on held-out days)" : "   (recorded only)"));
    r.push_back(headline(e, tick));
    r.push_back(std::to_string(e.lots) + " lots, " + std::to_string(e.fills) + " fills" + (e.ids >= 0 ? ", " + std::to_string(e.ids) + " orders" : std::string()) + " in one burst (normal " + std::to_string((long long)std::llround(e.norm)) + " lots)");
    r.push_back(outcomeText(e, tick, decideS, bigLots));
    r.push_back("Question: were stops triggered here - and were they absorbed after?");
    r.push_back("What it is: many small market orders fired at once just through an intact swing (last 3) or a key / MenthorQ level - the stops parked there.");
    r.push_back("Why it matters: absorbed = reversal fuel (trapped traders); not absorbed = continuation fuel - don't fade it.");
    r.push_back("How to use: green Sta at a low / red Sta at a high = look for the turn with your reversal bar; grey St = trade with the break.");
    r.push_back("Example: St at 7672.25 below the swing low from 8:48 AM, 3.4x; 10 s later back above and a 12-lot bid filled -> green Sta.");
    return r;
}

// ---- the session record ----------------------------------------------------------------------------------------------------
// ST|key|bar_t|fired_sec|ct|side|ref|ref_px|st_px|lots|fills|ids|norm_x10|ratio_x100|drawn|ref_kind|ref_t|ref_back|show
// SA|key|decided_t|outcome|bars|big_ids|exc_atr_x100|reclaimed|decided_sec|big_max|show
// (1.1.0 added decided_sec; 1.2.0 adds ref_kind / ref_t / ref_back / show to ST and big_max / show to SA; older lines still read = SHOW)
static const char* const REC_HEAD = "# ST|key|bar_t|fired_sec|ct|side|ref|ref_px|st_px|lots|fills|ids|norm_x10|ratio_x100|drawn|ref_kind|ref_t|ref_back|show  /  SA|key|decided_t|outcome|bars|big_ids|exc_atr_x100|reclaimed|decided_sec|big_max|show";
inline const char* kindText(int k) { return k == REF_SWING ? "swing" : k == REF_MQ ? "mq" : k == REF_VP ? "vp" : k == REF_X5 ? "x5" : k == REF_PRIOR ? "prior" : "key"; }
inline int showOf(const std::string& t) { return t == "SHOW" ? swg::SHOW : t == "?" ? swg::QUERY : t == "LOG" ? swg::LOG : -1; }
inline std::string clean(const std::string& s, size_t n) { std::string o; for (char c : s) { if (o.size() >= n) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline std::string stLine(const Event& e, double tick) {
    char b[512];
    std::snprintf(b, sizeof b, "ST|%s|%lld|%lld|%s|%s|%s|%s|%s|%lld|%lld|%lld|%lld|%lld|%d|%s|%lld|%d|%s", clean(e.key, 48).c_str(), e.barT, e.firedSec, clean(e.ct, 12).c_str(),
                  e.side == DOWN ? "down" : "up", clean(e.ref, 24).c_str(), pxText(e.refPx, tick).c_str(), pxText(e.stPx, tick).c_str(), e.lots, e.fills, e.ids,
                  (long long)std::llround(e.norm * 10), (long long)std::llround(e.ratio * 100), e.drawn ? 1 : 0, kindText(e.refKind), e.refT, e.refBack, swg::showText(e.showFire).c_str());
    return b;
}
inline std::string saLine(const Event& e) {
    char b[256];
    std::snprintf(b, sizeof b, "SA|%s|%lld|%s|%d|%d|%lld|%d|%lld|%d|%s", clean(e.key, 48).c_str(), e.decidedT, e.outcome == ABSORBED ? "absorbed" : "not", e.bars, e.bigIds,
                  (long long)std::llround((finite(e.excAtr) ? e.excAtr : 0) * 100), e.reclaimed ? 1 : 0, e.decidedSec, e.bigLotsMax, swg::showText(e.show).c_str());
    return b;
}
inline std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; size_t a = 0; for (;;) { size_t p = s.find('|', a); o.push_back(s.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; } return o; }
inline bool whole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    char* e = nullptr; errno = 0;
    const long long v = std::strtoll(t.c_str(), &e, 10);
    if (!e || *e || errno == ERANGE || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool real(const std::string& t, double& out) {
    if (t.empty() || t.size() > 32) return false;
    char* e = nullptr;
    const double v = std::strtod(t.c_str(), &e);
    if (!e || *e || !finite(v)) return false;
    out = v; return true;
}
inline bool toTicks(double px, double tick, long long& out) { if (!(tick > 0) || !finite(px) || px <= 0) return false; const double t = px / tick; if (t > 9e15) return false; out = (long long)std::llround(t); return std::fabs(t - (double)out) < 1e-3; }
// the record -> the events (fire + the FIRST decision line per key wins; a later SA for the same key is ignored = never changed again)
inline std::vector<Event> parseRecord(const std::vector<std::string>& lines, double tick, long long* bad = nullptr) {
    std::vector<Event> v; std::map<std::string, size_t> at; long long nb = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i];
        if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = split(ln);
        long long x = 0; double d = 0;
        if (c[0] == "ST") {
            if ((c.size() != 15 && c.size() != 19) || c[1].empty() || at.count(c[1])) { ++nb; continue; }
            Event e; e.key = c[1];
            if (!whole(c[2], 1, 4000000000000LL, e.barT) || !whole(c[3], 0, 4000000000000LL, e.firedSec)) { ++nb; continue; }
            e.ct = c[4];
            if (c[5] == "down") e.side = DOWN; else if (c[5] == "up") e.side = UP; else { ++nb; continue; }
            e.ref = c[6];
            if (!real(c[7], d) || !toTicks(d, tick, e.refPx) || !real(c[8], d) || !toTicks(d, tick, e.stPx)) { ++nb; continue; }
            if (!whole(c[9], 1, 10000000, e.lots) || !whole(c[10], 1, 10000000, e.fills) || !whole(c[11], -1, 100000, e.ids)) { ++nb; continue; }
            if (!whole(c[12], 0, 1000000000, x)) { ++nb; continue; } e.norm = (double)x / 10.0;
            if (!whole(c[13], 0, 100000000, x)) { ++nb; continue; } e.ratio = (double)x / 100.0;
            if (c[14] != "0" && c[14] != "1") { ++nb; continue; } e.drawn = c[14] == "1";
            if (c.size() == 19) {
                e.refKind = c[15] == "swing" ? REF_SWING : c[15] == "mq" ? REF_MQ : c[15] == "vp" ? REF_VP : c[15] == "key" ? REF_KEY : c[15] == "x5" ? REF_X5 : c[15] == "prior" ? REF_PRIOR : -1;
                if (e.refKind < 0 || !whole(c[16], 0, 4000000000000LL, e.refT) || !whole(c[17], 0, 3, x) || showOf(c[18]) < 0) { ++nb; continue; }
                e.refBack = (int)x; e.showFire = showOf(c[18]);
            }
            at[e.key] = v.size(); v.push_back(e);
        } else if (c[0] == "SA") {
            if (c.size() != 8 && c.size() != 9 && c.size() != 11) { ++nb; continue; }
            std::map<std::string, size_t>::iterator it = at.find(c[1]); if (it == at.end()) { ++nb; continue; }
            Event& e = v[it->second]; if (e.outcome != PENDING) continue;   // decided once
            long long t = 0, bars = 0, big = 0, exc = 0;
            long long dsec = 0;
            if (!whole(c[2], 0, 4000000000000LL, t) || (c[3] != "absorbed" && c[3] != "not") || !whole(c[4], 0, 10000, bars) || !whole(c[5], 0, 100000, big) || !whole(c[6], 0, 100000000, exc) || (c[7] != "0" && c[7] != "1")
                || (c.size() >= 9 && !whole(c[8], 0, 4000000000000LL, dsec))) { ++nb; continue; }
            long long bm = 0; int sh = swg::SHOW;
            if (c.size() == 11) { if (!whole(c[9], 0, 1000000, bm) || showOf(c[10]) < 0) { ++nb; continue; } sh = showOf(c[10]); }
            e.bigLotsMax = (int)bm; e.show = sh;
            e.decidedT = t; e.decidedSec = dsec; e.outcome = c[3] == "absorbed" ? ABSORBED : NOT_ABSORBED; e.bars = (int)bars; e.bigIds = (int)big; e.excAtr = (double)exc / 100.0; e.reclaimed = c[7] == "1";
        } else ++nb;
    }
    if (bad) *bad = nb;
    return v;
}

} // namespace stl
#endif
