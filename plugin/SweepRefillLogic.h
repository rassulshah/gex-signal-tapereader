#ifndef LS_SWEEPREFILL_LOGIC_H
#define LS_SWEEPREFILL_LOGIC_H
// SweepRefillLogic.h (1.1.0, 2026-10-22) -- SDK-free rules of lsSweepRefill, the "Sr" (SWEEP REFILLED) mark on the PRICE chart.
// MBO Roadmap row "Did the swept prices refill fast after a sweep?". 1.0.0: Rassul approved the build 16:16 CT 2026-10-22 ("Sr only
// happens when a level or pivot high or low was taken out"), 16:18 CT "it should not be on adjacent candles", 16:28 CT bearish drawn.
// 1.1.0 (Rassul approved 17:39 CT, mockup v4 sweep-refill-mockup-v4.png): the reference is an INTACT ZigZag swing (last 3) or a key /
// MenthorQ level - SwingRef.h, the SAME rule lsStopRun 1.2.0 uses; green Sr UNDER the swept wick, red Sr OVER the swept high; a short
// line from the swing taken out to the sweep candle; the hover names the swing; the 1-bar ZigZag drawn as a thin line (setting).
//
// 1.1.0 also applies the St / Sr validation (research-artifacts\learning\VALIDATE-STOPRUN-SWEEP-2026-10-22.md, iteration 3 locked per
// market, held-out tested): the refill rule 50% within 5 s and still there 5 s later (NQ; GC 2 s), and per-market SHOW / ? / LOG from
// lsFlexLevels\LRA-StopSweep-<MKT>.csv (NQ bull SHOW 68% held-out vs 47%, NQ bear "?", GC bull SHOW 71% vs 50%; else LOG = recorded only).
//
// WHAT IS DRAWN
//   * BULLISH Sr (green, UNDER the candle's low): sellers swept DOWN through >= minPrices bid prices in one burst, the sweep TOOK OUT
//     an intact swing low (or a key / MenthorQ level below), and the bids came back (REFILL below).
//   * BEARISH mirror (red, OVER the candle's high): buyers swept UP through offer prices through a swing high / level, offers came back.
//   * per market and side from the params file: SHOW = "Sr", ? = "Sr?" (strong but not proven), LOG = recorded, not drawn; the
//     "Show bullish / bearish Sr" checkboxes can still turn a side off.
//   * never on two adjacent candles (per side): a refilled sweep on a candle within gapBars of the last refilled one is recorded only.
//     The first Sr of a run is the one kept; it never moves or disappears.
//   * decided ONCE at sweep end + windowS + stayS (NQ 10 s); never repainted. Every candidate (refilled or not) is recorded.
//
// DETECTION
//   1. SWEEP: fills of RESTING orders on one side (resting bids hit = a sell sweep), each within windowMs (50) of the burst's first
//      fill and in the same or next exchange second, moving one way (a sell sweep never fills higher than its last fill).
//   2. SIZE: >= minPrices distinct prices (NQ 8, GC 3, ES 4, CL 6) and some resting size there before the sweep ("pre").
//      pre = for each swept price, at its FIRST fill in the burst, the resting lots there from orders that had rested >= minRestS (1 s).
//   3. TAKEN OUT (SwingRef.h): an intact swing among the last refPivots (3) ZigZag swings on that side (1 back always; 2 back if
//      beyond 1; 3 back if beyond 1 and 2), else a key level (ONL PDL PFL LonLO PWL AsiaLO / the highs), a MenthorQ level or a
//      volume-profile level (POC / VAH / VAL today and the prior day, naked prior POCs - Rassul 18:07 CT); the sweep
//      ends beyond it, starts <= 0.5 ATR past it, and no candle before the last one had traded through it; the deepest one wins.
//   4. REFILL: share(T) = the lots resting on the swept prices at T from orders that have rested >= minRestS, / pre. Checked every
//      second after the sweep end: refilled = share >= refillPct (50%) at some check within windowS (5 s) AND still >= refillPct at
//      windowS + stayS (NQ 10 s, GC 7 s). Time to refill = the first check that reached it.
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "SwingRef.h"

namespace swr {

static const char* const VERSION = "1.1.3";   // (1.1.1, 2026-10-22 evening, Rassul 17:16 CT IMPLIED FILLS option B) an implied fill (Ev::implied, the feed's side "?" FILL - ImpliedFill.h) is never a sweep trade: it only takes its lots off the resting order
enum Side { BID = 0, ASK = 1 };                    // the RESTING side: a sweep of BIDs = sellers sweeping down = a bullish Sr candidate
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum RefKind { REF_PIVOT = 0, REF_KEY = 1, REF_MQ = 2, REF_X5 = 3, REF_VP = 4 };   // REF_PIVOT = a ZigZag swing (1.1.0); REF_VP = POC / VAH / VAL (1.1.0); REF_X5 = 1.0.0 records only
static const int PX_CAP = 256, PEND_CAP = 64, BARS_CAP = 600;
static const size_t BOOK_CAP = 400000;

inline bool finite(double v) { return v == v && v < 1e300 && v > -1e300; }

struct Params {
    int windowMs = 50, minPrices = 8, windowS = 5, stayS = 5, minRestS = 1, gapBars = 1, spb = 180, lookBars = 5, refPivots = 3;
    double refillPct = 50, nearAtr = 0.5; bool useKey = true, useMq = true;
    int showBull = swg::LOG, showBear = swg::LOG;    // the verdict per side (params file); stamped on each mark when it is decided
};
// the per-market params file -> the detector's rule (a change applies to sweeps decided from now on; decided marks never change)
inline void applyParams(Params& P, const swg::StopSweep& X) {
    P.minPrices = X.srMinPrices; P.windowMs = X.srWindowMs; P.refillPct = X.srRefillPct; P.windowS = X.srWindowSec; P.stayS = X.srStaySec;
    P.minRestS = X.srMinRestSec; P.refPivots = X.srRefPivots; P.useKey = X.srKey != 0; P.useMq = X.srMq != 0; P.showBull = X.srBull; P.showBear = X.srBear;
}

// ---- inputs --------------------------------------------------------------------------------------------------------------------
typedef swg::Lvl Level;                             // name, px (ticks), dir (-1 low / +1 high / 0 either), kind (swg::KEY / swg::MQ)
inline int keyDir(const std::string& n) { return swg::keyDir(n); }   // ONL PDL PFL LonLO PWL AsiaLO -1, the highs +1, else 0
struct Bar { long long t = 0, o = 0, h = 0, l = 0, c = 0; long long session = 0; long long day = 0; bool rth = false; };   // a CLOSED candle in ticks; t = its END (IRT bar time); day / rth from the host (CT)
struct Ev { int action = NEW; int side = -1; long long px = 0, size = 0, id = 0, ms = 0, sec = 0; bool implied = false; };   // implied: (1.1.1) a FILL with raw side "?"

// ---- the hybrid event clock (IRT stamps whole seconds; arrival ms orders events inside a second, never across one) -------------
struct Clock {
    long long cur = -1, secArr = 0, last = 0;
    long long at(long long sec, long long arrMs) {
        if (sec > cur) { cur = sec; secArr = arrMs; }
        long long d = arrMs - secArr; if (d < 0) d = 0; if (d > 999) d = 999;
        const long long m = cur * 1000 + d; if (m > last) last = m; return last;
    }
    long long idle(long long arrMs) {                 // no event: time still passes on the arrival clock
        if (cur < 0) return last;
        long long d = arrMs - secArr; if (d < 0) d = 0; if (d > 600000) d = 600000;
        const long long m = cur * 1000 + d; if (m > last) last = m; return last;
    }
};

// ---- the record of one candidate ----------------------------------------------------------------------------------------------
struct Event {
    std::string key;                // "<start sec>-<b|s>-<low or high px ticks>"
    long long barT = 0, sec = 0, endMs = 0, startMs = 0;
    int side = BID;                 // BID = sell sweep through a low (bullish candidate), ASK = buy sweep through a high (bearish)
    std::string ref; int refKind = REF_PIVOT; long long refPx = 0;
    long long refT = 0; int refBack = 0;   // the swing's candle (END; 0 = a level) and how many swings back (1-3)
    std::string refCt;                     // host: the swing candle's start, "8:48 AM"
    long long hiPx = 0, loPx = 0; int prices = 0; long long lots = 0, pre = 0, post = 0;
    double refill = 0;              // percent
    long long ttrMs = -1;           // time to refill (ms after the sweep end, whole checks; -1 = never reached refillPct)
    double peak = 0;                // the best share (percent) within the window
    int show = swg::SHOW;           // the verdict when decided: SHOW "Sr", QUERY "Sr?", LOG not drawn (1.0.0 records = SHOW)
    bool refilled = false, gapOk = false;
    double atrTicks = 0;
    std::string ct;                 // 12-hour Central time of the sweep, for the hover / record (host)
};
inline bool bullish(const Event& e) { return e.side == BID; }
inline const char* markText(const Event& e) { return e.show == swg::QUERY ? "Sr?" : "Sr"; }

struct Ord { int side = 0; long long px = 0, size = 0, addMs = 0; };
struct Burst {
    bool on = false; int side = 0; long long startMs = 0, lastMs = 0, startSec = 0, lastPx = 0, firstPx = 0, lo = 0, hi = 0, lots = 0;
    std::vector<long long> pxs; std::map<long long, long long> preAt;   // price -> resting lots (aged) at its first fill in the burst
};
struct Pending { Event e; std::vector<long long> pxs; int k = 0; bool hit = false; };   // k = the last check done (seconds after the end)

struct Detector {
    Params P;
    // book
    std::unordered_map<long long, Ord> book; std::map<long long, std::set<long long> > at[2];   // price -> order ids resting there
    long long bookResets = 0, unknownFills = 0;
    bool skipImplied = true; long long impliedFills = 0, impliedLots = 0;   // (1.1.1) LRA-Implied-<MKT>.csv exclude (default on)
    // flow
    Burst b; std::vector<Pending> pending; std::vector<Event> decided;   // the host drains `decided`
    long long nowMs = 0;
    // context
    std::vector<Level> levels; swg::ZigZag Z; swg::BarWin W; swg::VolProf V; std::vector<long long> barEnds;
    long long session = 0; double atrTicks = 0;
    long long lastDrawnBar[2] = { 0, 0 };
    std::string lastKeyBase; int keySeq = 1;
    // counters (status)
    long long fills = 0, bursts = 0, sweeps = 0, bigSweeps = 0, tookOut = 0, candidates = 0, refilledN = 0, drawnN = 0;

    // ---- book -----------------------------------------------------------------------------------------------------------------
    void restOn(const Ord& o, long long id) { if (o.side == BID || o.side == ASK) at[o.side][o.px].insert(id); }
    void restOff(const Ord& o, long long id) {
        if (o.side != BID && o.side != ASK) return;
        std::map<long long, std::set<long long> >::iterator it = at[o.side].find(o.px);
        if (it == at[o.side].end()) return;
        it->second.erase(id);
        if (it->second.empty()) at[o.side].erase(it);
    }
    // resting lots at (side, px) from orders added at or before `bornBy` (ms)
    long long restingAged(int side, long long px, long long bornBy) const {
        std::map<long long, std::set<long long> >::const_iterator it = at[side].find(px); if (it == at[side].end()) return 0;
        long long s = 0;
        for (std::set<long long>::const_iterator k = it->second.begin(); k != it->second.end(); ++k) {
            std::unordered_map<long long, Ord>::const_iterator o = book.find(*k); if (o == book.end()) continue;
            if (o->second.addMs <= bornBy && o->second.size > 0) s += o->second.size;
        }
        return s;
    }
    // A reset makes both the burst's pre-liquidity and any pending refill observation incomplete.
    // Keep already-decided marks, but never bridge an MBO data gap with a new Sr decision.
    void clearBook() { book.clear(); at[0].clear(); at[1].clear(); b = Burst(); pending.clear(); ++bookResets; }

    // ---- one order-book event (IRT DEPTH_BY_ORDER semantics; ms = the event clock) --------------------------------------------
    void onEvent(const Ev& e) {
        if (e.ms > nowMs) nowMs = e.ms;
        decide(nowMs);
        if (e.action < NEW || e.action > FILL || e.px <= 0 || e.size < 0 || e.size > 10000000) return;
        std::unordered_map<long long, Ord>::iterator it = book.find(e.id);
        switch (e.action) {
        case NEW: {
            if (e.side != BID && e.side != ASK) return;
            if (it != book.end()) { restOff(it->second, e.id); book.erase(it); }
            if (book.size() >= BOOK_CAP) clearBook();
            Ord o; o.side = e.side; o.px = e.px; o.size = e.size; o.addMs = nowMs; book[e.id] = o; restOn(o, e.id);
            break; }
        case MOD: {
            if (it == book.end()) {                     // first sight (rested before load): from now on, born now
                if (e.side != BID && e.side != ASK) return;
                if (book.size() >= BOOK_CAP) clearBook();
                Ord o; o.side = e.side; o.px = e.px; o.size = e.size; o.addMs = nowMs; book[e.id] = o; restOn(o, e.id); break;
            }
            Ord& o = it->second;
            if (e.px != o.px) { restOff(o, e.id); o.px = e.px; o.size = e.size; o.addMs = nowMs; restOn(o, e.id); }   // re-priced = a new arrival there
            else { if (e.size > o.size) o.addMs = nowMs; o.size = e.size; }   // a size increase = new lots (priority lost at CME)
            break; }
        case DEL: {
            if (it != book.end()) { restOff(it->second, e.id); book.erase(it); }
            break; }
        case FILL: {
            if (e.size == 0) return;                       // a zero-lot FILL is not an execution / swept price
            if (e.implied && skipImplied) {             // (1.1.1) implied: not a sweep / refill trade; the resting order still loses the lots
                ++impliedFills; impliedLots += e.size;
                if (it != book.end()) { Ord& o = it->second; o.size -= std::min(o.size, e.size); }
                break;
            }
            if (it == book.end()) { ++unknownFills; ++fills; return; }  // keep feed accounting, never infer aged liquidity from an unmapped ID
            const int side = it->second.side;
            if (side != BID && side != ASK) return;
            fill(side, e.px, e.size, e.sec);             // verified resting ID; observes the book BEFORE this execution takes its lots
            { Ord& o = it->second; o.size -= std::min(o.size, e.size); }
            break; }
        default: break;
        }
    }
    // ---- trades ---------------------------------------------------------------------------------------------------------------
    void fill(int side, long long px, long long lots, long long sec) {
        ++fills;
        const bool join = b.on && side == b.side && nowMs - b.startMs <= P.windowMs && sec - b.startSec <= 1 && sec >= b.startSec
                          && (side == BID ? px <= b.lastPx : px >= b.lastPx);
        if (!join) {
            close();
            b = Burst(); b.on = true; b.side = side; b.startMs = nowMs; b.startSec = sec; b.firstPx = px; b.lo = b.hi = px;
        }
        b.lastPx = px; b.lastMs = nowMs; b.lots += lots; if (px < b.lo) b.lo = px; if (px > b.hi) b.hi = px;
        if (!b.preAt.count(px) && (int)b.pxs.size() < PX_CAP) { b.pxs.push_back(px); b.preAt[px] = restingAged(side, px, b.startMs - 1000LL * P.minRestS); }
    }
    // time passes (event clock): a burst older than the window is over; pending refills whose time has come are decided
    void advance(long long ms) { if (ms > nowMs) nowMs = ms; if (b.on && nowMs - b.startMs > P.windowMs) close(); decide(nowMs); }
    // the market's RTH (CT minutes): the ZigZag runs on today's RTH candles only
    void setRth(int openMin, int closeMin) { Z.rthLenSec = closeMin > openMin ? (closeMin - openMin) * 60 : 0; }

    // ---- the reference the sweep took out (SwingRef.h: an intact swing among the last 3, else a key / MenthorQ level) -------------
    bool pickRef(const Burst& x, Level& out, long long& fromT, int& back) const {
        swg::Sweep w; w.down = x.side == BID; w.ext = w.down ? x.lo : x.hi; w.start = w.down ? x.hi : x.lo; w.barT = barOf(x.startSec);
        swg::Params sp; sp.nearAtr = P.nearAtr; sp.look = P.lookBars < 1 ? 1 : P.lookBars; sp.maxBack = P.refPivots; sp.useKey = P.useKey; sp.useMq = P.useMq;
        std::vector<Level> all = levels; V.levels(all);                                  // + the volume-profile levels (closed candles)
        const swg::Ref r = swg::pick(Z, W, all, w, P.spb > 0 ? P.spb : 180, sp);
        if (!r.ok) return false;
        out.name = r.name; out.px = r.px; out.kind = r.kind == swg::SWING ? REF_PIVOT : r.kind == swg::MQ ? REF_MQ : r.kind == swg::VP ? REF_VP : REF_KEY; out.dir = w.down ? -1 : 1;
        fromT = r.fromT; back = r.back;
        return true;
    }
    long long barOf(long long sec) const {
        for (size_t i = barEnds.size(); i > 0; --i) if (barEnds[i - 1] <= sec) {    // the newest closed END <= sec: the next END holds sec
            if (i < barEnds.size()) return barEnds[i];
            const long long sp = P.spb > 0 ? P.spb : 180;
            return barEnds[i - 1] + ((sec - barEnds[i - 1]) / sp + 1) * sp;
        }
        if (!barEnds.empty()) return barEnds.front();
        const long long sp = P.spb > 0 ? P.spb : 180; return (sec / sp + 1) * sp;
    }
    void close() {
        if (!b.on) return;
        Burst x = b; b = Burst(); ++bursts;
        const int n = (int)x.pxs.size();
        if (n < 3) return;
        ++sweeps;
        long long pre = 0; for (std::map<long long, long long>::const_iterator it = x.preAt.begin(); it != x.preAt.end(); ++it) pre += it->second;
        if (n < P.minPrices || pre <= 0) return;
        ++bigSweeps;
        Level r; long long fromT = 0; int back = 0; if (!pickRef(x, r, fromT, back)) return;
        ++tookOut;
        Pending p; Event& e = p.e;
        e.sec = x.startSec; e.startMs = x.startMs; e.endMs = x.lastMs; e.side = x.side; e.ref = r.name; e.refKind = r.kind; e.refPx = r.px; e.refT = fromT; e.refBack = back;
        e.hiPx = x.hi; e.loPx = x.lo; e.prices = n; e.lots = x.lots; e.pre = pre; e.barT = barOf(x.startSec); e.atrTicks = W.atr(e.barT);
        char k[64]; std::snprintf(k, sizeof k, "%lld-%s-%lld", x.startSec, x.side == BID ? "b" : "s", x.side == BID ? x.lo : x.hi); e.key = k;
        if (e.key == lastKeyBase) { ++keySeq; e.key += "-" + std::to_string(keySeq); } else { lastKeyBase = e.key; keySeq = 1; }   // two sweeps, same second + price: unique keys
        p.pxs = x.pxs;
        if ((int)pending.size() >= PEND_CAP) pending.erase(pending.begin());
        pending.push_back(p); ++candidates;
    }
    // the share at a check: lots resting now on the swept prices from orders that have rested >= minRestS (no look-ahead)
    // (the book is as it was at checkT: decide() runs before each event is applied, so nothing changed between checkT and now)
    long long restingAt(const Pending& p, long long checkT) const {
        long long s = 0; const long long bornBy = checkT - 1000LL * P.minRestS;
        for (size_t k = 0; k < p.pxs.size(); ++k) s += restingAged(p.e.side, p.pxs[k], bornBy);
        return s;
    }
    // checks every second after the sweep end; final at windowS + stayS (the validation's locked rule; nothing changes it after that)
    void decide(long long ms) {
        const int Wn = P.windowS < 1 ? 1 : P.windowS, St = P.stayS < 0 ? 0 : P.stayS, last = Wn + St;
        for (size_t i = 0; i < pending.size();) {
            Pending& p = pending[i];
            bool done = false;
            while (p.k < last && ms >= p.e.endMs + 1000LL * (p.k + 1)) {
                ++p.k;
                const long long post = restingAt(p, p.e.endMs + 1000LL * p.k);
                const double sh = p.e.pre > 0 ? 100.0 * (double)post / (double)p.e.pre : 0;
                if (p.k <= Wn) { if (sh > p.e.peak) p.e.peak = sh; if (!p.hit && sh + 1e-9 >= P.refillPct) { p.hit = true; p.e.ttrMs = 1000LL * p.k; } }
                if (p.k == last) { p.e.post = post; p.e.refill = sh; done = true; }
            }
            if (!done) { ++i; continue; }
            Event e = p.e;
            e.refilled = p.hit && e.refill + 1e-9 >= P.refillPct;
            e.show = bullish(e) ? P.showBull : P.showBear;
            if (!e.refilled) e.ttrMs = p.hit ? e.ttrMs : -1;
            if (e.refilled) {
                ++refilledN;
                const long long sp = P.spb > 0 ? P.spb : 180, lastB = lastDrawnBar[e.side];
                e.gapOk = lastB == 0 || e.barT - lastB > (long long)P.gapBars * sp;   // gapBars 1 = never the very next candle (nor the same one)
                if (e.gapOk) { lastDrawnBar[e.side] = e.barT; ++drawnN; }
            }
            decided.push_back(e);
            pending.erase(pending.begin() + (long)i);
        }
    }
    // ---- a CLOSED candle (strictly in time order; history first, then live): ZigZag, ATR, the forming candle's range resets --------
    // rows = the candle's volume at price (tick -> lots) for the volume-profile levels (nullptr = not known: the profile skips it)
    void barClose(const Bar& c, const std::vector<std::pair<long long, double> >* rows = nullptr) {
        if (!barEnds.empty() && c.t <= barEnds.back()) return;
        if (rows) V.add(c.t, c.session, c.h, c.l, *rows);
        if (c.session != session) session = c.session;
        barEnds.push_back(c.t); if ((int)barEnds.size() > BARS_CAP) barEnds.erase(barEnds.begin());
        swg::ZBar z; z.t = c.t; z.o = c.o; z.h = c.h; z.l = c.l; z.c = c.c; z.day = c.day; z.rth = c.rth;
        Z.spb = P.spb > 0 ? P.spb : 180;
        Z.add(z); W.add(z);
        atrTicks = W.atr(c.t + 1);                         // ATR = the mean true range of the last 14 closed candles
    }
};

// ---- text -----------------------------------------------------------------------------------------------------------------------
inline std::string pxText(long long ticks, double tick) {
    char b[32]; const double p = (double)ticks * tick;
    int dec = 0; for (double t = tick; dec < 6 && std::fabs(t - std::floor(t + 1e-9)) > 1e-9; t *= 10) ++dec;
    std::snprintf(b, sizeof b, "%.*f", dec, p); return b;
}
inline std::string pctText(double v) { char b[16]; std::snprintf(b, sizeof b, "%.0f%%", !finite(v) || v < 0 ? 0.0 : v > 999 ? 999.0 : v); return b; }
inline std::string secText(long long ms) { if (ms < 0) return "not within the window"; char b[24]; std::snprintf(b, sizeof b, "%.1f s", (double)ms / 1000.0); return b; }
// "the swing low 30997.25 from 8:48 AM, 2 swings back" / "LonLO 31178.50"
inline std::string refText(const Event& e, double tick) {
    if (e.refKind == REF_PIVOT && e.refT > 0) return "the " + e.ref + " " + pxText(e.refPx, tick) + (e.refCt.empty() ? std::string() : " from " + e.refCt) + swg::backText(e.refBack);
    return e.ref + " " + pxText(e.refPx, tick);
}
inline std::string readLine(const Event& e, double tick, int windowS) {
    const std::string side = bullish(e) ? "bids" : "offers";
    std::string s = "Swept " + std::to_string(e.prices) + " prices (" + pxText(bullish(e) ? e.hiPx : e.loPx, tick) + " - " + pxText(bullish(e) ? e.loPx : e.hiPx, tick)
                    + ") through " + refText(e, tick) + " - refilled " + pctText(e.refill) + " at " + std::to_string(windowS) + " s";
    s += e.refilled ? " - " + side + " defending" : " - still empty";
    return s;
}
// the hover: header + the read + the numbers + question / what it is / why it matters / how to use / example (his hover structure)
inline std::vector<std::string> hoverRows(const Event& e, double tick, int windowS, int stayS, double pct) {
    std::vector<std::string> r;
    char pb[16]; std::snprintf(pb, sizeof pb, "%.0f%%", pct);
    r.push_back(std::string(markText(e)) + "  Sweep refilled?  " + e.ct + (bullish(e) ? "  (bullish)" : "  (bearish)") + (e.show == swg::QUERY ? "   (? = promising, not proven yet)" : e.show == swg::SHOW ? "   (proven on held-out days)" : "   (recorded only)"));
    r.push_back(readLine(e, tick, windowS + stayS));
    r.push_back("Took out " + refText(e, tick) + "  -  " + std::to_string(e.lots) + " lots traded, "
                + std::to_string(e.pre) + " resting before, " + std::to_string(e.post) + " back at the end (" + pb + " needed within " + std::to_string(windowS) + " s: " + secText(e.ttrMs) + ", still there after " + std::to_string(stayS) + " s more)");
    r.push_back(std::string("Question: did the ") + (bullish(e) ? "bids" : "offers") + " that were just swept through the " + (bullish(e) ? "swing low" : "swing high") + " come right back?");
    r.push_back(std::string("What it is: one ") + (bullish(e) ? "sell" : "buy") + " burst emptied many " + (bullish(e) ? "bid" : "offer") + " prices through an intact ZigZag swing (last 3) or a key / MenthorQ level; orders resting 1 s+ brought back half the size within 5 s and stayed.");
    r.push_back(std::string("Why it matters: the stops ") + (bullish(e) ? "under the swing low" : "over the swing high") + " were run and absorbed - the defenders reload; still empty = an air pocket, price keeps going.");
    r.push_back(std::string("How to use: ") + (bullish(e) ? "green Sr under the turn candle backs a long" : "red Sr over the turn candle backs a short") + " with your reversal bar / CoB; no Sr = don't fade it.");
    r.push_back("Example: NQ Oct 9 8:55 AM: 19 bid prices swept through the swing low from 8:48 AM, the bids back within 1 s and still there 10 s later -> green Sr under the wick.");
    return r;
}

// ---- the session record ----------------------------------------------------------------------------------------------------------
// SR|key|bar_t|sec|ct|side|ref|ref_kind|ref_px|hi|lo|prices|lots|pre|post|refill_x10|ttr_ms|refilled|gap_ok|atr_x100|ref_t|ref_back|show
// (1.1.0 adds ref_t = the swing candle's END, 0 = a level, ref_back = swings back 1-3, show = SHOW / ? / LOG; 1.0.0 lines of 20 fields still read)
static const char* const REC_HEAD = "# SR|key|bar_t|sec|ct|side|ref|ref_kind|ref_px|hi|lo|prices|lots|pre|post|refill_x10|ttr_ms|refilled|gap_ok|atr_x100|ref_t|ref_back|show";
inline std::string clean(const std::string& s, size_t n) { std::string o; for (char c : s) { if (o.size() >= n) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline std::string recLine(const Event& e, double tick) {
    char b[640];
    std::snprintf(b, sizeof b, "SR|%s|%lld|%lld|%s|%s|%s|%s|%s|%s|%s|%d|%lld|%lld|%lld|%lld|%lld|%d|%d|%lld|%lld|%d|%s", clean(e.key, 48).c_str(), e.barT, e.sec, clean(e.ct, 12).c_str(),
                  e.side == BID ? "bull" : "bear", clean(e.ref, 32).c_str(), e.refKind == REF_PIVOT ? "swing" : e.refKind == REF_KEY ? "key" : e.refKind == REF_X5 ? "x5" : e.refKind == REF_VP ? "vp" : "mq",
                  pxText(e.refPx, tick).c_str(), pxText(e.hiPx, tick).c_str(), pxText(e.loPx, tick).c_str(), e.prices, e.lots, e.pre, e.post,
                  (long long)std::llround((finite(e.refill) ? std::min(e.refill, 99999.0) : 0) * 10), e.ttrMs, e.refilled ? 1 : 0, e.gapOk ? 1 : 0,
                  (long long)std::llround((finite(e.atrTicks) ? std::min(e.atrTicks, 1e7) : 0) * 100), e.refT, e.refBack, swg::showText(e.show).c_str());
    return b;
}
inline std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; size_t a = 0; for (;;) { size_t p = s.find('|', a); o.push_back(s.substr(a, p == std::string::npos ? std::string::npos : p - a)); if (p == std::string::npos) break; a = p + 1; } return o; }
inline bool whole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    char* e = nullptr; errno = 0; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (!e || *e || errno == ERANGE || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool real(const std::string& t, double& out) { if (t.empty() || t.size() > 32) return false; char* e = nullptr; const double v = std::strtod(t.c_str(), &e); if (!e || *e || !finite(v)) return false; out = v; return true; }
inline bool toTicks(double px, double tick, long long& out) { if (!(tick > 0) || !finite(px) || px <= 0) return false; const double t = px / tick; if (t > 9e15) return false; out = (long long)std::llround(t); return std::fabs(t - (double)out) < 1e-3; }
// the record -> the events (the FIRST line per key wins; nothing is ever changed)
inline std::vector<Event> parseRecord(const std::vector<std::string>& lines, double tick, long long* bad = nullptr) {
    std::vector<Event> v; std::set<std::string> seen; long long nb = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i]; if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = split(ln);
        if ((c.size() != 20 && c.size() != 23) || c[0] != "SR" || c[1].empty() || c[1].size() > 48) { ++nb; continue; }
        if (seen.count(c[1])) continue;
        Event e; e.key = c[1]; long long x = 0; double d = 0;
        if (!whole(c[2], 1, 4000000000000LL, e.barT) || !whole(c[3], 0, 4000000000000LL, e.sec)) { ++nb; continue; }
        e.ct = c[4];
        if (c[5] == "bull") e.side = BID; else if (c[5] == "bear") e.side = ASK; else { ++nb; continue; }
        e.ref = c[6];
        if (c[7] == "swing" || c[7] == "pivot") e.refKind = REF_PIVOT; else if (c[7] == "key") e.refKind = REF_KEY; else if (c[7] == "mq") e.refKind = REF_MQ; else if (c[7] == "x5") e.refKind = REF_X5; else if (c[7] == "vp") e.refKind = REF_VP; else { ++nb; continue; }
        if (!real(c[8], d) || !toTicks(d, tick, e.refPx) || !real(c[9], d) || !toTicks(d, tick, e.hiPx) || !real(c[10], d) || !toTicks(d, tick, e.loPx)) { ++nb; continue; }
        if (!whole(c[11], 3, 100000, x)) { ++nb; continue; } e.prices = (int)x;
        if (!whole(c[12], 0, 100000000, e.lots) || !whole(c[13], 0, 100000000, e.pre) || !whole(c[14], 0, 100000000, e.post)) { ++nb; continue; }
        if (!whole(c[15], 0, 1000000, x)) { ++nb; continue; } e.refill = (double)x / 10.0;
        if (!whole(c[16], -1, 100000000, e.ttrMs)) { ++nb; continue; }
        if ((c[17] != "0" && c[17] != "1") || (c[18] != "0" && c[18] != "1")) { ++nb; continue; }
        e.refilled = c[17] == "1"; e.gapOk = c[18] == "1";
        if (!whole(c[19], 0, 1000000000000LL, x)) { ++nb; continue; } e.atrTicks = (double)x / 100.0;
        if (c.size() == 23) {
            if (!whole(c[20], 0, 4000000000000LL, e.refT) || !whole(c[21], 0, 3, x)) { ++nb; continue; }
            e.refBack = (int)x;
            if (c[22] == "SHOW") e.show = swg::SHOW; else if (c[22] == "?") e.show = swg::QUERY; else if (c[22] == "LOG") e.show = swg::LOG; else { ++nb; continue; }
        }
        seen.insert(e.key); v.push_back(e);
    }
    if (bad) *bad = nb;
    return v;
}
// drawn on the chart? (per side checkbox; the gap decision was made once, when the sweep was decided)
inline bool shown(const Event& e, bool showBull, bool showBear) { return e.refilled && e.gapOk && e.show != swg::LOG && (bullish(e) ? showBull : showBear); }

} // namespace swr
#endif
