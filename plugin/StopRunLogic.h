#ifndef LS_STOPRUN_LOGIC_H
#define LS_STOPRUN_LOGIC_H
// StopRunLogic.h (1.1.0, 2026-10-22) -- SDK-free rules of lsStopRun, the "St" / "Sta" STOP-RUN mark on the PRICE chart.
// Rassul 2026-10-22 15:33-15:34 CT (MBO Roadmap row "Were stops triggered - and were they absorbed after?", T-20261022-15) "show yes";
// 15:37 CT "Sta can indicate the stop run absorbed". 1.1.0 (approved 16:13 CT): FEWER (real pivots + key levels only) and an EARLIER,
// LIVE absorbed decision (seconds after the burst, not at a later candle close).
//
// WHAT IS DRAWN
//   * every time stops get triggered at a REAL level: a small GREY "St" at the price where the stops fired, on that candle (decided once,
//     the moment the burst ends; never moved, never removed).
//   * once, within `decideS` seconds (default 10) of the burst, the outcome is decided LIVE:
//       ABSORBED = big resting orders (>= `bigLots` each, per market) at / beyond the level took >= `sharePct` % of the burst's lots AND
//       price traded back across the level - or (unless `requireReclaim`) made no new extreme beyond the burst for the whole window.
//       A reclaim with the absorption already in decides at once; otherwise the decision is taken when the window ends.
//       -> the same mark becomes "Sta": green = stops BELOW a low absorbed (bullish), red = stops ABOVE a high (bearish).
//       NOT absorbed -> it stays grey "St"; the hover says "not absorbed - continuation".
//     The upgrade happens at most once and is never changed again. Both the fire (ST line) and the decision (SA line + its second) are recorded.
//   * the hover (header "Stop run?" - "?" = not proven yet): question / what it is / why it matters / how to use / example.
//
// DETECTION (how the platforms do it: ATAS Stops Tracker groups a burst by a ms window + min ticks + min count; Bookmap Stops & Icebergs
// uses the MBO event order; CME MDP elects stops as aggressor orders that never rested, in the same match event):
//   1. BURST: aggressive fills on ONE side, each within `windowMs` (default 50 ms) of the burst's first fill and in the same or next
//      exchange second (a drained backlog arriving in one ms can never glue seconds of trading into one burst).
//   2. THROUGH A REAL LEVEL: the burst starts at most `nearTicks` before a reference and trades >= 1 tick through it. References only
//      (1.1.0 dropped the prior candle's high / low): a KEY level - his session levels PFH/PFL PDH/PDL PWH/PWL ONH/ONL LonHI/LonLO
//      AsiaHI/AsiaLO and the MenthorQ levels CR PS HVL CR0 PS0 HVL0 GW0 1DH 1DL (setting on / off) - or the 5-candle PIVOT extreme
//      (the lowest low / highest high of the last 5 closed candles, ZigZag-style swing).
//   3. SIZE: >= `minFills` fills AND lots >= `ratio` (2.0x) x the NORMAL STOP BURST = the median lots of the bursts with >= minFills
//      fills in the same 30-min slot this session (>= 20 of them), else the last 300 such bursts of any slot (>= 20), else the market seed.
//   4. MANY ORDERS: when the feed names aggressor orders, >= `minIds` (2) distinct aggressor ids (stops elect as separate orders).
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
#include <vector>

namespace stl {

static const char* const VERSION = "1.1.0";
enum Side { DOWN = -1, UP = 1 };            // DOWN = sellers hit bids through a low; UP = buyers lift offers through a high
enum Outcome { PENDING = 0, ABSORBED = 1, NOT_ABSORBED = 2 };
enum RefKind { REF_KEY = 0, REF_X5 = 1, REF_PRIOR = 2 };   // REF_PRIOR: 1.0.0 records only (1.1.0 never fires on it)
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
    int windowMs = 50, minFills = 5, nearTicks = 4, bigLots = 20, minIds = 2;
    int decideS = 10, sharePct = 50; bool requireReclaim = false, useKeys = true;
    double ratio = 2.0, seedLots = 14;
};

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
struct Bar { long long t = 0; long long h = 0, l = 0, c = 0; };   // a CLOSED candle in ticks; t = its key (IRT: the bar END time)

// ---- the 5-candle PIVOT (ZigZag-style swing) from the closed candles, oldest first: the lowest low / highest high of the last 5
//      closed candles counts only when it is CONFIRMED - at least `confirm` (2) later candles did not go beyond it. A running extreme
//      (the last candle making a new low in a slide) is not a pivot, so a trend does not fire St at every new candle low.
inline void pivot5(const std::vector<Bar>& c, bool& haveLo, long long& lo, bool& haveHi, long long& hi, int confirm = 2) {
    haveLo = haveHi = false; const int n = (int)c.size(); if (n < 5) return;
    int il = n - 5, ih = n - 5;
    for (int k = n - 5; k < n; ++k) { if (c[k].l <= c[il].l) il = k; if (c[k].h >= c[ih].h) ih = k; }   // ties: the latest
    if (il <= n - 1 - confirm) { haveLo = true; lo = c[il].l; }
    if (ih <= n - 1 - confirm) { haveHi = true; hi = c[ih].h; }
}

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
    int sharePct = -1;                         // % of the burst's lots taken by big resting orders (-1 = not known: reloaded record)
    bool newExt = false;                       // price made a new extreme beyond the burst inside the window
    std::string ct;                  // 12-hour Central time of the fire, for the hover / record
};
inline int bullish(const Event& e) { return e.side == DOWN ? 1 : 0; }   // an absorbed DOWN run = trapped sellers = bullish

// ---- the detector --------------------------------------------------------------------------------------------------------------
struct Burst {
    bool on = false; int side = 0; long long startMs = 0, lastMs = 0, startSec = 0; long long firstPx = 0, lo = 0, hi = 0;
    long long lots = 0, fills = 0; std::vector<long long> ids; bool idsKnown = false; std::vector<long long> pxs;
    std::map<long long, long long> rest;   // resting order id -> lots filled in this burst (both sides; filtered at evaluation)
    std::map<long long, long long> restPx; // resting order id -> its fill price (ticks)
    void clear() { *this = Burst(); }
};
// a fired run waiting for its live decision: resting fills at / beyond the level since the burst started, the burst's extreme, reclaim
struct Pending { Event e; std::map<long long, long long> rest; long long ext = 0; int k = 0; long long endMs = 0, endSec = 0, extPx = 0; bool reclaimed = false, newExt = false; };

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
    std::vector<Ref> keys; Bar prior; bool havePrior = false; long long x5hi = 0, x5lo = 0; bool haveX5 = false, haveX5lo = true, haveX5hi = true;
    long long formingT = 0; int minuteOfDay = -1; double atrTicks = 0;
    std::map<long long, int> marksPerBar;
    std::set<std::pair<int, long long> > used;           // (side, level px): the stops at a level fire ONCE per session
    void markUsed(int side, long long px) { used.insert(std::make_pair(side, px)); }

    double normNow(int slot) const {
        std::map<int, std::vector<long long> >::const_iterator it = slotLots.find(slot);
        if (it != slotLots.end() && (int)it->second.size() >= SLOT_MIN) return (double)median(it->second);
        if ((int)anyLots.size() >= SLOT_MIN) return (double)median(anyLots);
        return P.seedLots > 0 ? P.seedLots : 1;
    }
    // ---- the reference this burst ran through (priority key > 5-candle extreme > prior candle); none = no stop run --------
    bool pickRef(const Burst& x, Ref& out) const {
        const int side = x.side;
        const long long thr = side == DOWN ? x.lo : x.hi;
        auto through = [&](long long px) {
            if (used.count(std::make_pair(side, px))) return false;
            if (side == DOWN) return x.firstPx >= px && x.firstPx - px <= P.nearTicks && thr <= px - 1;
            return x.firstPx <= px && px - x.firstPx <= P.nearTicks && thr >= px + 1;
        };
        bool got = false; Ref best;
        if (P.useKeys) for (size_t i = 0; i < keys.size(); ++i) {
            const Ref& r = keys[i];
            if (!through(r.px)) continue;
            if (!got || r.rank < best.rank || (r.rank == best.rank && std::llabs(r.px - x.firstPx) < std::llabs(best.px - x.firstPx))) { best = r; got = true; }
        }
        if (got) { best.kind = REF_KEY; out = best; return true; }
        if (haveX5 && (side == DOWN ? haveX5lo : haveX5hi)) { const long long px = side == DOWN ? x5lo : x5hi; if (through(px)) { out.name = side == DOWN ? "5-candle low" : "5-candle high"; out.px = px; out.kind = REF_X5; out.dir = side == DOWN ? -1 : 1; return true; } }
        return false;
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
        Ref r; if (!pickRef(x, r)) return;
        ++nearLevel;
        const double ratio = norm > 0 ? (double)x.lots / norm : 0;
        if (ratio + 1e-9 < P.ratio) return;
        ++sized;
        if (x.idsKnown && (int)x.ids.size() < P.minIds) return;
        // the first fill price beyond the level
        long long st = x.side == DOWN ? r.px - 1 : r.px + 1;
        for (size_t i = 0; i < x.pxs.size(); ++i) if (x.side == DOWN ? x.pxs[i] < r.px : x.pxs[i] > r.px) { st = x.pxs[i]; break; }
        Event e;
        e.barT = formingT; e.firedSec = x.startSec; e.firedMs = x.startMs; e.side = x.side; e.ref = r.name; e.refKind = r.kind; e.refPx = r.px; e.stPx = st;
        e.lots = x.lots; e.fills = x.fills; e.ids = x.idsKnown ? (long long)x.ids.size() : -1; e.norm = norm; e.ratio = ratio; e.slot = slot;
        char k[64]; std::snprintf(k, sizeof k, "%lld-%s-%lld", x.startSec, x.side == DOWN ? "d" : "u", st); e.key = k;
        int& m = marksPerBar[e.barT]; e.drawn = m < 2; if (e.drawn) ++m;
        ++runs; markUsed(x.side, r.px);
        Pending p; p.e = e; p.ext = 0; p.endMs = x.lastMs; p.endSec = x.startSec; p.extPx = x.side == DOWN ? x.lo : x.hi;
        for (std::map<long long, long long>::const_iterator it = x.rest.begin(); it != x.rest.end(); ++it) {
            const long long px = x.restPx[it->first];
            if (x.side == DOWN ? px <= r.px : px >= r.px) p.rest[it->first] += it->second;
        }
        if (pending.size() >= (size_t)PENDING_CAP) pending.erase(pending.begin());
        pending.push_back(p);
        fired.push_back(e);
    }
    // ---- feed: one AGGRESSIVE fill. side: DOWN = a seller hit a bid, UP = a buyer lifted an offer. aggId 0 = not named ----------
    void aggFill(long long ms, long long sec, int side, long long px, long long lots, long long aggId) {
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
            if (p.e.side == DOWN) { if (px > p.e.refPx) p.reclaimed = true; if (px < p.extPx) { p.extPx = px; p.newExt = true; } }
            else { if (px < p.e.refPx) p.reclaimed = true; if (px > p.extPx) { p.extPx = px; p.newExt = true; } }
        }
        decide(ms, sec);
    }
    int share(const Pending& p, int& big) const {
        long long took = 0; big = 0;
        for (std::map<long long, long long>::const_iterator it = p.rest.begin(); it != p.rest.end(); ++it) if (it->second >= P.bigLots) { ++big; took += it->second; }
        if (p.e.lots <= 0) return 0;
        const long long pc = took * 100 / p.e.lots; return (int)(pc > 999 ? 999 : pc);
    }
    // the LIVE decision: absorbed at once on a reclaim once big resting orders took >= sharePct of the burst; at the window's end
    // (decideS after the burst, by exchange second or feed ms) absorbed = that share AND (reclaimed OR no new extreme unless requireReclaim)
    void decide(long long ms, long long sec) {
        for (size_t i = 0; i < pending.size();) {
            Pending& p = pending[i];
            int big = 0; const int sh = share(p, big);
            const bool took = big >= 1 && sh >= P.sharePct;
            const bool due = sec - p.endSec > P.decideS || ms - p.endMs >= (long long)P.decideS * 1000;
            if ((took && p.reclaimed) || due) {
                Event e = p.e;
                e.bigIds = big; e.sharePct = sh; e.reclaimed = p.reclaimed; e.newExt = p.newExt; e.bars = p.k + 1; e.decidedT = formingT; e.decidedSec = sec; e.decidedMs = ms;
                const long long x = p.e.side == DOWN ? p.e.refPx - p.extPx : p.extPx - p.e.refPx;
                e.excAtr = atrTicks > 0 && x > 0 ? (double)x / atrTicks : 0;
                e.outcome = took && (p.reclaimed || (!P.requireReclaim && !p.newExt)) ? ABSORBED : NOT_ABSORBED;
                decided.push_back(e);
                pending.erase(pending.begin() + (long)i);
                continue;
            }
            ++i;
        }
    }
    // one RESTING fill (the order that was hit). restSide: DOWN = a resting BID (hit by sellers), UP = a resting offer.
    void restFill(long long ms, int restSide, long long px, long long lots, long long restId) {
        (void)ms;
        if (restId == 0 || lots <= 0 || lots > 10000000) return;
        if (b.on && restSide == b.side && (int)b.rest.size() < REST_CAP) { b.rest[restId] += lots; b.restPx[restId] = px; }
        for (size_t i = 0; i < pending.size(); ++i) {
            Pending& p = pending[i]; if (p.e.side != restSide) continue;
            if (p.e.side == DOWN ? px > p.e.refPx : px < p.e.refPx) continue;
            if ((int)p.rest.size() < REST_CAP || p.rest.count(restId)) p.rest[restId] += lots;
        }
    }
    // time passes with no fill: a burst older than the window is over
    void tick(long long ms, long long sec) { if (b.on && (ms - b.startMs > P.windowMs || sec - b.startSec > 1)) close(ms); decide(ms, sec); }
    // ---- a CLOSED candle (strictly in time order): counts the candles a pending run has waited (the decision itself is live) -------
    void barClose(const Bar& c) {
        if (c.t <= lastCloseT) return;
        lastCloseT = c.t;
        for (size_t i = 0; i < pending.size(); ++i) if (c.t >= pending[i].e.barT) ++pending[i].k;
    }
};

// ---- text ------------------------------------------------------------------------------------------------------------------------
inline std::string ratioText(double r) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", !finite(r) || r < 0 ? 0.0 : r > 999 ? 999.0 : r); return b; }
inline std::string pxText(long long ticks, double tick) {
    char b[32]; const double p = (double)ticks * tick;
    int dec = 0; for (double t = tick; dec < 6 && std::fabs(t - std::floor(t + 1e-9)) > 1e-9; t *= 10) ++dec;
    std::snprintf(b, sizeof b, "%.*f", dec, p); return b;
}
inline std::string markText(const Event& e) { return e.outcome == ABSORBED ? "Sta" : "St"; }
// 0 = grey, 1 = green (bullish), 2 = red (bearish)
inline int markColour(const Event& e) { return e.outcome == ABSORBED ? (bullish(e) ? 1 : 2) : 0; }
inline std::string secs(long long n) { return n == 1 ? "1 s" : std::to_string(n < 0 ? 0 : n) + " s"; }
inline std::string outcomeText(const Event& e, double tick, int decideS) {
    const std::string idSide = e.side == DOWN ? "bid" : "offer", dir = e.side == DOWN ? "above" : "below", ext = e.side == DOWN ? "low" : "high";
    if (e.outcome == PENDING && e.orphan) return "Absorbed? not decided - Investor/RT restarted inside the " + secs(decideS) + " window";
    if (e.outcome == PENDING) return "Absorbed? deciding - watching " + secs(decideS) + " for big resting " + idSide + "s and a move back " + dir + " " + pxText(e.refPx, tick);
    const long long took = e.decidedSec > 0 && e.firedSec > 0 ? e.decidedSec - e.firedSec : -1;
    const std::string when = took >= 0 ? " in " + secs(took) : std::string();
    const std::string pct = e.sharePct >= 0 ? " took " + std::to_string(e.sharePct > 100 ? 100 : e.sharePct) + "%" : std::string();
    char a[32]; std::snprintf(a, sizeof a, "%.1f", e.excAtr);
    if (e.outcome == ABSORBED) return "Absorbed" + when + ": " + std::to_string(e.bigIds) + " big " + idSide + (e.bigIds == 1 ? " order" : " orders") + pct + ", " + (e.reclaimed ? "back " + dir + " the level" : "no new " + ext);
    if (e.bigIds == 0) return "Not absorbed - continuation: no big resting " + idSide + " took the burst";
    if (e.sharePct >= 0 && !e.reclaimed && e.newExt) return std::string("Not absorbed - continuation: new ") + ext + ", " + a + " ATR past the level";
    return "Not absorbed - continuation: big " + idSide + "s took too little of the burst";
}
inline std::string headline(const Event& e, double tick) {
    std::string s = "Stop run at " + pxText(e.stPx, tick) + (e.side == DOWN ? " below " : " above ") + e.ref + " " + pxText(e.refPx, tick) + ": " + ratioText(e.ratio) + " normal stop burst";
    return s;
}
// the hover: header + the question / what it is / why it matters / how to use / example (Rassul's hover structure)
inline std::vector<std::string> hoverRows(const Event& e, double tick, int decideS) {
    std::vector<std::string> r;
    r.push_back(std::string(e.outcome == ABSORBED ? "Sta" : "St") + "  Stop run?  " + e.ct + "   (? = not proven yet)");
    r.push_back(headline(e, tick));
    r.push_back(std::to_string(e.lots) + " lots, " + std::to_string(e.fills) + " fills" + (e.ids >= 0 ? ", " + std::to_string(e.ids) + " orders" : std::string()) + " in one burst (normal " + std::to_string((long long)std::llround(e.norm)) + " lots)");
    r.push_back(outcomeText(e, tick, decideS));
    r.push_back("Question: were stops triggered here - and were they absorbed after?");
    r.push_back("What it is: many small market orders fired at once just through a level - the stops parked there.");
    r.push_back("Why it matters: absorbed = reversal fuel (trapped traders); not absorbed = continuation fuel - don't fade it.");
    r.push_back("How to use: green Sta at a low / red Sta at a high = look for the turn; grey St = trade with the break.");
    r.push_back("Example: St at 7672.25 below LonLO, 3.1x; 4 s later 2 big bids took 70%, back above -> green Sta.");
    return r;
}

// ---- the session record ----------------------------------------------------------------------------------------------------
// ST|key|bar_t|fired_sec|ct|side|ref|ref_px|st_px|lots|fills|ids|norm_x10|ratio_x100|drawn
// SA|key|decided_t|outcome|bars|big_ids|exc_atr_x100|reclaimed|decided_sec      (1.1.0 adds decided_sec; 1.0.0 lines without it still read)
static const char* const REC_HEAD = "# ST|key|bar_t|fired_sec|ct|side|ref|ref_px|st_px|lots|fills|ids|norm_x10|ratio_x100|drawn  /  SA|key|decided_t|outcome|bars|big_ids|exc_atr_x100|reclaimed|decided_sec";
inline std::string clean(const std::string& s, size_t n) { std::string o; for (char c : s) { if (o.size() >= n) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline std::string stLine(const Event& e, double tick) {
    char b[512];
    std::snprintf(b, sizeof b, "ST|%s|%lld|%lld|%s|%s|%s|%s|%s|%lld|%lld|%lld|%lld|%lld|%d", clean(e.key, 48).c_str(), e.barT, e.firedSec, clean(e.ct, 12).c_str(),
                  e.side == DOWN ? "down" : "up", clean(e.ref, 24).c_str(), pxText(e.refPx, tick).c_str(), pxText(e.stPx, tick).c_str(), e.lots, e.fills, e.ids,
                  (long long)std::llround(e.norm * 10), (long long)std::llround(e.ratio * 100), e.drawn ? 1 : 0);
    return b;
}
inline std::string saLine(const Event& e) {
    char b[256];
    std::snprintf(b, sizeof b, "SA|%s|%lld|%s|%d|%d|%lld|%d|%lld", clean(e.key, 48).c_str(), e.decidedT, e.outcome == ABSORBED ? "absorbed" : "not", e.bars, e.bigIds,
                  (long long)std::llround((finite(e.excAtr) ? e.excAtr : 0) * 100), e.reclaimed ? 1 : 0, e.decidedSec);
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
            if (c.size() != 15 || c[1].empty() || at.count(c[1])) { ++nb; continue; }
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
            at[e.key] = v.size(); v.push_back(e);
        } else if (c[0] == "SA") {
            if (c.size() != 8 && c.size() != 9) { ++nb; continue; }
            std::map<std::string, size_t>::iterator it = at.find(c[1]); if (it == at.end()) { ++nb; continue; }
            Event& e = v[it->second]; if (e.outcome != PENDING) continue;   // decided once
            long long t = 0, bars = 0, big = 0, exc = 0;
            long long dsec = 0;
            if (!whole(c[2], 0, 4000000000000LL, t) || (c[3] != "absorbed" && c[3] != "not") || !whole(c[4], 0, 10000, bars) || !whole(c[5], 0, 100000, big) || !whole(c[6], 0, 100000000, exc) || (c[7] != "0" && c[7] != "1")
                || (c.size() == 9 && !whole(c[8], 0, 4000000000000LL, dsec))) { ++nb; continue; }
            e.decidedT = t; e.decidedSec = dsec; e.outcome = c[3] == "absorbed" ? ABSORBED : NOT_ABSORBED; e.bars = (int)bars; e.bigIds = (int)big; e.excAtr = (double)exc / 100.0; e.reclaimed = c[7] == "1";
        } else ++nb;
    }
    if (bad) *bad = nb;
    return v;
}

} // namespace stl
#endif
