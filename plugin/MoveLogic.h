#ifndef LS_MOVE_LOGIC_H
#define LS_MOVE_LOGIC_H
// MoveLogic.h 1.2.0 (2026-10-22 15:20 CT Rassul ordered "ROLLING BIDS / OFFERS DURING THE MOVE", MBO Roadmap "Is a big order following price
// during the move?" T-20261022-17: TRAILS - one big order (by id, >= the market's Big Lot S20 of resting orders) re-priced again and again
// BEHIND price during the wave: a step = a bid re-priced UP (an offer DOWN) after price ran that way since its last price, still behind the
// trade and inside the near band (NQ 10-09 replay: without these, band-keeping quotes 300+ ticks away and oscillating quotes made 112 junk
// trails; with them 19); 2 steps = a trail; it ends once (traded / pulled / stepped back here, stopped following on the chart side - TrailSet).
// The extreme reads below are unchanged); 1.1.2 (2026-10-21; 1.1.2 23:30 CT: counter `small` renamed smallSz - Windows rpcndr.h #define small char broke MSVC builds; 1.1.0 14:18 CT Rassul "i want you to draw them" - all FOUR cases drawn, mockup v4: P = pressing in /
// B = stepping back, the letter and the price boxes in the colour of the SIDE that moved (bids green, offers red); "they may be weak
// for other reasons and may require additional confluences" - drawn with "?" while the confluence study + the nightly score them),
// 1.0.0 (2026-10-21, MBO Roadmap TOP DEFENDER #3 "are the defenders stepping back or pressing in?" - Rassul approved
// defender3-mockup-v3 11:50 CT "ok.. lets do it" + 11:59 "the other scenario would be offers going down ... and vice versa").
// SDK-free: which resting orders MOVED, toward price (pressing in) or away (stepping back), both sides of the book.
// Design + test: /home/claude/pt/d3/Defender3-design-research.md, Defender3-phase1-findings.md (7 markets, Oct 6-8 MBO):
//   PATH M (exact, 55-89% of moves): the SAME order id re-priced (CME keeps the id on a price modify; Rithmic CHANGE / DBO MOD).
//   PATH R (backup): a cancel (not right after a fill) + a NEW on the same side, same size (10% tolerance at 10+ lots), a DIFFERENT
//          price, within 250 ms - a UNIQUE match only (a new order that matches 2+ cancels, or a cancel matched by 2+ new orders = none).
//   The move is judged by its DISTANCE to the last trade (the old distance from when the order was placed / last moved): smaller =
//   pressing in (+1), larger = stepping back (-1), unchanged = following the market (dropped).
//   Dropped: ladder shifts (5+ moves on one side, same tick change, same 5 ms), orders that do not rest 0.5 s after the move (any later
//   event on the order inside 500 ms), sizes under the market's q90 of moves (CL 3, ES 6, EU 8, GC 2, HG 2, NG 7, NQ 1).
//   A move is booked to the candle in which it is CONFIRMED (its 0.5 s rest done): decided at that candle's close, no look-ahead.
// The per-candle read (moveRead): moves in the zone (max(2 ticks, 0.5 ATR) around the extreme) in the candle and the one before;
// per side the lots moved in / out; a case QUALIFIES when its lots >= 2.0x that side's normal (30-min slot mean of moved lots per
// candle) and >= 65% of that side's moved lots went that way. Only P (the DEFENDERS pressing in) is drawn ('?'); the other three
// cases are recorded for the nightly (his "i just want what works": the test found only P positive, +28 at supports, LEARNING).
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace mvl {
static const char* const VERSION = "1.2.0";   // (1.2.0, 2026-10-22) TRAILS: Tracker::fill + the trail lines (TS / TP / TF / TX / TB / RS) + TrailSet (the stop-following B? decision);   // (1.1.1, 2026-10-21) letterOf / movedText: a side whose case flag is set but moved no lots that way shows "-" and no letter (the box showed "In 0t 0.0x")
static const long long PAIR_MS = 250, REST_MS = 500, LADDER_MS = 5;
static const int LADDER_N = 5;
static const double BADGE_X = 2.0, ONE_WAY = 0.65, ZONE_ATR = 0.5;
static const long long ZONE_MIN = 2;
enum { BIDS = 0, OFFERS = 1 };                       // = OrderFlowFeed BID / ASK
inline long long minLotsOf(const std::string& m) {    // the market's q90 of moves (phase 1, refit by the nightly later)
    return m == "CL" ? 3 : m == "ES" ? 6 : (m == "EU" || m == "6E") ? 8 : m == "GC" ? 2 : m == "HG" ? 2 : m == "NG" ? 7 : m == "NQ" ? 1 : 3;
}
// distance from the last trade, positive on the order's own side (a bid below the trade, an offer above it)
inline long long distOf(int side, long long px, long long ref) { return side == BIDS ? ref - px : px - ref; }
inline int dirOf(long long dOld, long long dNew) { return dNew < dOld ? 1 : (dNew > dOld ? -1 : 0); }
inline bool sizeMatch(long long a, long long b) {
    if (a <= 0 || b <= 0) return false;
    const long long m = std::max(a, b);
    return m >= 10 ? std::llabs(a - b) * 10 <= m : a == b;
}

struct Move { int side = BIDS; long long oldPx = 0, newPx = 0, size = 0; int dir = 0; long long ms = 0; long long id = 0; bool viaR = false; };

class Tracker {
public:
    long long minLots = 3;
    // counters (status / tests)
    long long seenM = 0, seenR = 0, following = 0, ladder = 0, shortRest = 0, smallSz = 0, ambiguous = 0, confirmed = 0;
    // PATH M: a known order re-priced. ref = the last trade now; refAt = the last trade when it was placed / last moved
    void modify(long long id, int side, long long oldPx, long long newPx, long long size, long long refAt, long long ref, long long ms) {
        if (oldPx == newPx || side < 0 || side > 1) return;
        ++seenM; ladderNote(side, newPx - oldPx, ms);
        if (trailMin > 0) trailNote(id, side, oldPx, newPx, size, refAt, ref, ms);   // (1.2.0) a trail step (judged after its 5 ms ladder window)
        candidate(id, side, oldPx, newPx, size, distOf(side, oldPx, refAt), distOf(side, newPx, ref), ms, false, 0);
    }
    // PATH R step 1: a resting order cancelled (not because it traded)
    void cancel(long long id, int side, long long px, long long size, long long refAt, long long ms) {
        if (trailMin > 0) trailEnd(id, 'X', px, 0, ms);                  // (1.2.0) a trailing order pulled
        if (side < 0 || side > 1 || size <= 0) return;
        prune(ms);
        Cx c; c.id = id; c.side = side; c.px = px; c.size = size; c.dOld = distOf(side, px, refAt); c.ms = ms;
        cancels.push_back(c);
        if (cancels.size() > 4000) cancels.pop_front();
    }
    // PATH R step 2: a new order - a unique same-size cancel on its side within 250 ms at another price makes it a candidate
    void added(long long id, int side, long long px, long long size, long long ref, long long ms) {
        if (side < 0 || side > 1 || size <= 0) return;
        prune(ms);
        Cx* one = nullptr; int n = 0;
        for (size_t k = 0; k < cancels.size(); ++k) {
            Cx& c = cancels[k];
            if (c.side != side || c.px == px || ms - c.ms > PAIR_MS || ms < c.ms || !sizeMatch(c.size, size)) continue;
            ++n; one = &c;
        }
        if (n == 0) return;
        if (n > 1) { for (size_t k = 0; k < cancels.size(); ++k) { Cx& c = cancels[k]; if (c.side == side && c.px != px && ms - c.ms <= PAIR_MS && ms >= c.ms && sizeMatch(c.size, size)) c.matches = 99; } ++ambiguous; return; }
        if (++one->matches > 1) { ++ambiguous; return; }
        ++seenR; ladderNote(side, px - one->px, ms);
        candidate(id, side, one->px, px, size, one->dOld, distOf(side, px, ref), ms, true, one->id);
    }
    // any later event on an order (fill, cancel, another modify): a candidate that has not rested 0.5 s is dropped
    void touched(long long id, long long ms) {
        std::unordered_map<long long, size_t>::iterator it = byId.find(id);
        if (it == byId.end()) return;
        if (it->second >= base && it->second - base < pend.size()) { Pend& p = pend[it->second - base]; if (!p.dead && ms - p.m.ms < REST_MS) { p.dead = true; ++shortRest; } }
        byId.erase(it);
    }
    // the moves whose 0.5 s rest is done by `ms` (in time order) -> out; ladder shifts / ambiguous pairs / small sizes dropped here
    void confirm(long long ms, std::vector<Move>& out) {
        if (trailMin > 0) trailRun(ms, 0);                                  // (1.2.0) trail steps whose ladder window closed (before prune: it reads the ladder buckets)
        prune(ms);
        while (!pend.empty() && (pend.front().dead || ms - pend.front().m.ms >= REST_MS)) {
            Pend p = pend.front(); pend.pop_front(); ++base;
            std::unordered_map<long long, size_t>::iterator it = byId.find(p.m.id);
            if (it != byId.end() && it->second == base - 1) byId.erase(it);
            if (p.dead) continue;
            if (ladderCount(p.m.side, p.m.newPx - p.m.oldPx, p.m.ms) >= LADDER_N) { ++ladder; continue; }
            if (p.m.viaR && cancelMatches(p.cancelId) != 1) { ++ambiguous; continue; }
            if (p.m.size < minLots) { ++smallSz; continue; }
            ++confirmed; out.push_back(p.m);
        }
        rebase();
    }
    size_t pending() const { return pend.size(); }
    // ---- (1.2.0) TRAILS: one big order (by id, PATH M only - the exact id) re-priced again and again BEHIND price -------------------
    // A bid stepping UP (an offer stepping DOWN) after price ran that way, still behind the trade and inside the near band, is a FOLLOW step. An order >= trailMin lots with 2 follow steps (ladder shifts never
    // count) becomes a TRAIL; every later step extends it; it ENDS once: traded (TF, a fill on it), pulled (TX, cancelled), or moved
    // AWAY from price (TB, stepped back). The stop-following "B?" (price ran on, the order did not follow) needs candles + ATR: it is
    // decided by the chart overlay from these lines (TrailSet below). Lines (prices in ticks), appended to the trail file:
    //   RS|key|tick                      a new order stream (a restart: trails open before it are lost - MBO has no history)
    //   TS|id|side|size|key|ms|px        trail started (px = the price before its first step; key / ms = its first step)
    //   TP|id|key|ms|px|flag|dist|ran    a step to px (flag 1 = not counted: a ladder shift / not after a run; dist = ticks behind the
    //                                    last trade at the new price; ran = ticks price moved its way since the order's last price)
    //   TF|id|key|ms|px|lots   TX|id|key|ms|px   TB|id|key|ms|px
    long long trailMin = 0;                         // 0 = trails off (lsBookFlow's engine sets trailMinOf(market))
    long long key = 0;                              // the candle key of the event being applied (set by the engine)
    long long trailNear = 0;                        // the near band in ticks (the engine's: max(1 ATR, the zone)); a step must stay this close behind price (0 = no cap)
    std::vector<std::string> trailOut;              // lines waiting for the file (flushTrails)
    long long trStarted = 0, trSteps = 0, trLadder = 0, trFilled = 0, trPulled = 0, trBack = 0, trFileFails = 0, trCands = 0;
    // a fill on a resting order (any size): a trail that traded ends with the dot
    void fill(long long id, long long px, long long lots, long long ms) { if (trailMin > 0) trailEnd(id, 'F', px, lots, ms); }
    void setTrailFile(const std::string& path, double tick) {
        if (path == trailPath) return;
        trailPath = path; trailTick = tick;
        { std::ifstream in(path.c_str(), std::ios::binary | std::ios::ate); if (in.is_open() && (long long)in.tellg() > 3LL * 1024 * 1024) { in.close(); const std::string old = path + ".old"; std::remove(old.c_str()); std::rename(path.c_str(), old.c_str()); } }
        char tb[40]; std::snprintf(tb, sizeof tb, "%.10g", tick);
        trailOut.push_back("RS|" + std::to_string(key) + "|" + tb);
    }
    const std::string& trailFile() const { return trailPath; }
    // append the waiting lines; on a failed append they are kept (bounded) and tried again
    bool flushTrails() {
        if (trailOut.empty() || trailPath.empty()) return true;
        std::ofstream f(trailPath.c_str(), std::ios::binary | std::ios::app);
        if (f.is_open()) { for (size_t k = 0; k < trailOut.size(); ++k) f << trailOut[k] << "\n"; f.flush(); if (f.good()) { trailOut.clear(); return true; } }
        ++trFileFails; if (trailOut.size() > 4000) trailOut.erase(trailOut.begin(), trailOut.begin() + (trailOut.size() - 4000));
        return false;
    }
    size_t trailsOpen() const { size_t n = 0; for (std::unordered_map<long long, Tr>::const_iterator it = trails.begin(); it != trails.end(); ++it) n += it->second.active ? 1 : 0; return n; }
private:
    struct TStep { long long id = 0; int side = 0; long long oldPx = 0, newPx = 0, size = 0, ms = 0, key = 0, ran = 0, dNew = 0; };   // ran: ticks price moved its way since the order's last price; dNew: its new distance behind the trade
    struct Tr { int side = 0; long long size = 0, firstPx = 0, lastPx = 0, lastMs = 0; int steps = 0; bool active = false; std::vector<TStep> st; };
    std::deque<TStep> tsteps;                           // steps waiting for their ladder window (5 ms)
    std::unordered_map<long long, Tr> trails;           // order id -> candidate (1 step) / active trail
    std::string trailPath; double trailTick = 0; long long trLastPrune = 0;
    void trailNote(long long id, int side, long long oldPx, long long newPx, long long size, long long refAt, long long ref, long long ms) {
        TStep t; t.id = id; t.side = side; t.oldPx = oldPx; t.newPx = newPx; t.size = size; t.ms = ms; t.key = key;
        t.ran = side == BIDS ? ref - refAt : refAt - ref; t.dNew = distOf(side, newPx, ref);
        tsteps.push_back(t); if (tsteps.size() > 20000) tsteps.pop_front();
    }
    static std::string num(long long v) { return std::to_string(v); }
    void emitStep(long long id, const TStep& s, bool lad) { trailOut.push_back("TP|" + num(id) + "|" + num(s.key) + "|" + num(s.ms) + "|" + num(s.newPx) + "|" + (lad ? "1" : "0") + "|" + num(s.dNew) + "|" + num(s.ran)); }
    void stepNow(const TStep& s) {
        const bool lad = ladderCount(s.side, s.newPx - s.oldPx, s.ms) >= LADDER_N;
        // FOLLOW = re-priced in the wave's direction (a bid up / an offer down) AFTER price moved that way since its last price, still
        // behind price (not crossing it) A re-price that way without price moving first is quote management (it oscillates): no step.
        const bool toward = s.side == BIDS ? s.newPx > s.oldPx : s.newPx < s.oldPx;
        // and NEAR: within the near band behind the trade (NQ 10-09: band-keeping quotes 300+ ticks away stepped with price - not support)
        const bool follow = toward && s.ran > 0 && s.dNew >= 0 && (trailNear <= 0 || s.dNew <= trailNear);
        std::unordered_map<long long, Tr>::iterator it = trails.find(s.id);
        if (it == trails.end()) {
            if (lad) { ++trLadder; return; }
            if (!follow || s.size < trailMin) return;
            if (trails.size() >= 20000) return;
            Tr t; t.side = s.side; t.size = s.size; t.firstPx = s.oldPx; t.lastPx = s.newPx; t.lastMs = s.ms; t.steps = 1; t.st.push_back(s);
            trails[s.id] = t; ++trCands; return;
        }
        Tr& t = it->second;
        if (lad) { ++trLadder; if (!t.active) { trails.erase(it); return; } t.lastPx = s.newPx; t.lastMs = s.ms; emitStep(s.id, s, true); return; }
        if (toward && !follow) { if (t.active) { t.lastPx = s.newPx; t.lastMs = s.ms; emitStep(s.id, s, true); } return; }   // toward price but not after a run: drawn (price kept), never counted
        if (!follow) {                                                 // moved away from price: stepped back
            if (t.active) { trailOut.push_back("TB|" + num(s.id) + "|" + num(s.key) + "|" + num(s.ms) + "|" + num(s.newPx)); ++trBack; }
            trails.erase(it); return;
        }
        t.steps++; t.lastPx = s.newPx; t.lastMs = s.ms; ++trSteps;
        if (t.active) { emitStep(s.id, s, false); return; }
        t.st.push_back(s);
        if (t.steps >= 2) {                                            // 2 follow steps: a trail
            t.active = true; ++trStarted;
            const TStep& f = t.st.front();
            trailOut.push_back("TS|" + num(s.id) + "|" + num(t.side) + "|" + num(t.size) + "|" + num(f.key) + "|" + num(f.ms) + "|" + num(t.firstPx));
            for (size_t k = 0; k < t.st.size(); ++k) emitStep(s.id, t.st[k], false);
            t.st.clear(); t.st.shrink_to_fit();
        }
    }
    // the steps whose ladder window closed by `ms` (all of one order's steps when `forceId` != 0: it is being filled / pulled now)
    void trailRun(long long ms, long long forceId) {
        if (forceId != 0) {
            for (std::deque<TStep>::iterator it = tsteps.begin(); it != tsteps.end();) { if (it->id == forceId) { const TStep s = *it; it = tsteps.erase(it); stepNow(s); } else ++it; }
        }
        while (!tsteps.empty() && ms >= (tsteps.front().ms / LADDER_MS + 1) * LADDER_MS) { const TStep s = tsteps.front(); tsteps.pop_front(); stepNow(s); }
        if (ms - trLastPrune > 10000 || ms < trLastPrune) {             // candidates idle 10 min / trails idle 2 h are forgotten
            trLastPrune = ms;
            for (std::unordered_map<long long, Tr>::iterator it = trails.begin(); it != trails.end();) { const long long idle = ms - it->second.lastMs; if (idle > (it->second.active ? 7200000LL : 600000LL)) it = trails.erase(it); else ++it; }
        }
    }
    void trailEnd(long long id, char why, long long px, long long lots, long long ms) {
        trailRun(ms, id);
        std::unordered_map<long long, Tr>::iterator it = trails.find(id);
        if (it == trails.end()) return;
        if (it->second.active) {
            if (why == 'F') { trailOut.push_back("TF|" + num(id) + "|" + num(key) + "|" + num(ms) + "|" + num(px) + "|" + num(lots)); ++trFilled; }
            else { trailOut.push_back("TX|" + num(id) + "|" + num(key) + "|" + num(ms) + "|" + num(it->second.lastPx)); ++trPulled; }
        }
        trails.erase(it);
    }
    struct Cx { long long id = 0; int side = 0; long long px = 0, size = 0, dOld = 0, ms = 0; int matches = 0; };
    struct Pend { Move m; long long cancelId = 0; bool dead = false; };
    std::deque<Cx> cancels;
    std::deque<Pend> pend; size_t base = 0;              // pend[k] has the global index base + k
    std::unordered_map<long long, size_t> byId;          // order id -> its pending global index
    std::map<std::pair<long long, std::pair<int, long long> >, int> lad;   // (5 ms bucket, (side, tick change)) -> moves
    void candidate(long long id, int side, long long oldPx, long long newPx, long long size, long long dOld, long long dNew, long long ms, bool viaR, long long cancelId) {
        const int d = dirOf(dOld, dNew);
        { std::unordered_map<long long, size_t>::iterator it = byId.find(id); if (it != byId.end()) { if (it->second >= base && it->second - base < pend.size()) { Pend& q = pend[it->second - base]; if (!q.dead && ms - q.m.ms < REST_MS) { q.dead = true; ++shortRest; } } byId.erase(it); } }
        if (d == 0) { ++following; return; }
        Pend p; p.m.side = side; p.m.oldPx = oldPx; p.m.newPx = newPx; p.m.size = size; p.m.dir = d; p.m.ms = ms; p.m.id = id; p.m.viaR = viaR; p.cancelId = cancelId;
        pend.push_back(p); byId[id] = base + pend.size() - 1;
        if (pend.size() > 20000) { Pend& f = pend.front(); f.dead = true; }
    }
    void ladderNote(int side, long long change, long long ms) { lad[std::make_pair(ms / LADDER_MS, std::make_pair(side, change))]++; }
    int ladderCount(int side, long long change, long long ms) const {
        std::map<std::pair<long long, std::pair<int, long long> >, int>::const_iterator it = lad.find(std::make_pair(ms / LADDER_MS, std::make_pair(side, change)));
        return it == lad.end() ? 0 : it->second;
    }
    int cancelMatches(long long cid) const { for (size_t k = 0; k < cancels.size(); ++k) if (cancels[k].id == cid) return cancels[k].matches; return 1; }   // aged out: its window closed with the count it had
    void prune(long long ms) {
        // a cancel is kept until every candidate that refers to it is confirmed (its window + the rest time)
        while (!cancels.empty() && ms - cancels.front().ms > PAIR_MS + REST_MS + 50) {
            const Cx& c = cancels.front();
            if (c.matches > 1) for (size_t k = 0; k < pend.size(); ++k) if (pend[k].m.viaR && pend[k].cancelId == c.id && !pend[k].dead) { pend[k].dead = true; ++ambiguous; }
            cancels.pop_front();
        }
        while (!lad.empty() && lad.begin()->first.first * LADDER_MS < ms - 2000) lad.erase(lad.begin());
    }
    void rebase() { if (pend.empty() && base > 1000000000) { base = 0; byId.clear(); } }
};

// ---- the per-candle read -----------------------------------------------------------------------------------------------------
// one candle's confirmed moves (prices in ticks), as the pane keeps them
struct Sum { long long in[2] = {0, 0}, out[2] = {0, 0}; long long bigSize[2] = {0, 0}, bigOld[2] = {0, 0}, bigNew[2] = {0, 0}; long long ticksIn[2] = {0, 0}, ticksOut[2] = {0, 0};
             long long outSize[2] = {0, 0}, outOld[2] = {0, 0}, outNew[2] = {0, 0}; };   // big*: the largest move IN (toward price) per side; (1.1.0) out*: the largest move OUT (away) per side
inline long long zoneOf(double atrTicks) { const long long z = (long long)std::llround(ZONE_ATR * (std::isfinite(atrTicks) ? atrTicks : 0.0)); return std::max(z, ZONE_MIN); }
// the moves of the candle and the one before (window) whose old or new price is in [ext - zone, ext + zone]
inline Sum sumIn(const std::vector<Move>& mv, long long ext, long long zone) {
    Sum s;
    for (size_t k = 0; k < mv.size(); ++k) {
        const Move& m = mv[k];
        if (std::llabs(m.oldPx - ext) > zone && std::llabs(m.newPx - ext) > zone) continue;
        const long long t = std::llabs(m.newPx - m.oldPx);
        if (m.dir > 0) { s.in[m.side] += m.size; s.ticksIn[m.side] = std::max(s.ticksIn[m.side], t);
                         if (m.size > s.bigSize[m.side]) { s.bigSize[m.side] = m.size; s.bigOld[m.side] = m.oldPx; s.bigNew[m.side] = m.newPx; } }
        else { s.out[m.side] += m.size; s.ticksOut[m.side] = std::max(s.ticksOut[m.side], t);
               if (m.size > s.outSize[m.side]) { s.outSize[m.side] = m.size; s.outOld[m.side] = m.oldPx; s.outNew[m.side] = m.newPx; } }
    }
    return s;
}
// the four cases at an extreme. ext side: LOW (support: bids defend, offers attack) / HIGH (resistance: offers defend, bids attack)
enum { C_DEF_IN = 0, C_DEF_OUT = 1, C_ATT_IN = 2, C_ATT_OUT = 3 };
struct Read { long long t = 0; int ext = 0; long long px = 0; double tick = 0; Sum s; double norm[2] = {0, 0}; bool q[4] = {false, false, false, false}; double x[4] = {0, 0, 0, 0}; bool learning = false; };
// does `lots` one way qualify against that side's normal, with >= 65% of the side's moved lots going that way?
inline bool qualifies(long long lots, long long other, double norm, double& x) {
    x = norm > 0 ? (double)lots / norm : 0.0;
    const long long tot = lots + other;
    return norm > 0 && lots > 0 && x >= BADGE_X - 1e-9 && (double)lots >= ONE_WAY * (double)tot - 1e-9;
}
inline Read readAt(long long t, int ext, long long px, const Sum& s, const double norm[2]) {
    Read r; r.t = t; r.ext = ext; r.px = px; r.s = s; r.norm[0] = norm[0]; r.norm[1] = norm[1];
    r.learning = !(norm[0] > 0) || !(norm[1] > 0);
    const int def = ext == 0 ? BIDS : OFFERS, att = 1 - def;
    r.q[C_DEF_IN] = qualifies(s.in[def], s.out[def], norm[def], r.x[C_DEF_IN]);
    r.q[C_DEF_OUT] = qualifies(s.out[def], s.in[def], norm[def], r.x[C_DEF_OUT]);
    r.q[C_ATT_IN] = qualifies(s.in[att], s.out[att], norm[att], r.x[C_ATT_IN]);
    r.q[C_ATT_OUT] = qualifies(s.out[att], s.in[att], norm[att], r.x[C_ATT_OUT]);
    return r;
}
inline bool drawP(const Read& r) { return r.q[C_DEF_IN]; }          // the defenders pressed in (1.0.0: the only case drawn)
// (1.1.0) every qualified case is drawn. Per BOOK side (BIDS / OFFERS) at most one letter: 'P' (pressed in) or 'B' (stepped back), 0 = none
inline char letterOf(const Read& r, int side) {
    const int def = r.ext == 0 ? BIDS : OFFERS;
    const int cin = side == def ? C_DEF_IN : C_ATT_IN, cout = side == def ? C_DEF_OUT : C_ATT_OUT;
    return r.q[cin] && r.s.in[side] > 0 ? 'P' : (r.q[cout] && r.s.out[side] > 0 ? 'B' : 0);   // (1.1.1) a flag with no lots moved that way = nothing
}
inline bool anyDrawn(const Read& r) { return r.q[0] || r.q[1] || r.q[2] || r.q[3]; }

// the normal: the mean of moved lots per candle per side, per 30-min slot of the day (>= 5 candles in the slot), else all slots (>= 20)
struct Norm {
    std::map<int, std::pair<long long, double> > slot[2];   // slot -> (candles, sum lots)
    long long n[2] = {0, 0}; double sum[2] = {0, 0};
    void add(int sl, const long long lots[2]) {
        for (int s = 0; s < 2; ++s) { std::pair<long long, double>& p = slot[s][sl]; if (p.first >= 400) { p.second *= 399.0 / 400.0; p.first = 399; } p.first++; p.second += (double)lots[s];
            if (n[s] >= 4000) { sum[s] *= 3999.0 / 4000.0; n[s] = 3999; } n[s]++; sum[s] += (double)lots[s]; }
    }
    double of(int s, int sl) const {
        std::map<int, std::pair<long long, double> >::const_iterator it = slot[s].find(sl);
        if (it != slot[s].end() && it->second.first >= 5) return it->second.second / (double)it->second.first;
        return n[s] >= 20 ? sum[s] / (double)n[s] : 0.0;
    }
    std::string text() const {
        std::ostringstream o; o << "MN|1";
        for (int s = 0; s < 2; ++s) for (std::map<int, std::pair<long long, double> >::const_iterator it = slot[s].begin(); it != slot[s].end(); ++it) o << '|' << s << ':' << it->first << ':' << it->second.first << ':' << (long long)std::llround(it->second.second);
        o << "|T:" << n[0] << ':' << (long long)std::llround(sum[0]) << ':' << n[1] << ':' << (long long)std::llround(sum[1]);
        return o.str();
    }
    bool parse(const std::string& line) {
        if (line.compare(0, 5, "MN|1|") != 0 && line != "MN|1") return false;
        Norm z; std::stringstream ss(line.substr(4)); std::string tok;
        while (std::getline(ss, tok, '|')) {
            if (tok.empty()) continue;
            if (tok.compare(0, 2, "T:") == 0) { long long a = 0, b = 0, c = 0, d = 0; if (std::sscanf(tok.c_str() + 2, "%lld:%lld:%lld:%lld", &a, &b, &c, &d) != 4 || a < 0 || c < 0) return false; z.n[0] = a; z.sum[0] = (double)b; z.n[1] = c; z.sum[1] = (double)d; continue; }
            int s = 0, sl = 0; long long k = 0, v = 0;
            if (std::sscanf(tok.c_str(), "%d:%d:%lld:%lld", &s, &sl, &k, &v) != 4 || s < 0 || s > 1 || sl < 0 || sl > 47 || k < 0) return false;
            z.slot[s][sl] = std::make_pair(k, (double)v);
        }
        *this = z; return true;
    }
};
inline int slotOf(int minuteOfDay) { return std::max(0, std::min(47, minuteOfDay / 30)); }

// the record line (prices in ticks; the tick size last): M|t|ext|px|bidIn|bidOut|offIn|offOut|normB*100|normO*100|flags|defBigOld|defBigNew|defBigSize|tInB|tOutB|tInO|tOutO (flags: P B a b = def in /
// def out / att in / att out qualified; L = learning, no normal yet)
inline std::string mLineOf(const Read& r) {
    std::ostringstream o;
    std::string f; if (r.q[C_DEF_IN]) f += 'P'; if (r.q[C_DEF_OUT]) f += 'B'; if (r.q[C_ATT_IN]) f += 'a'; if (r.q[C_ATT_OUT]) f += 'b'; if (r.learning) f += 'L'; if (f.empty()) f = "-";
    o << "M|" << r.t << '|' << r.ext << '|' << r.px << '|' << r.s.in[0] << '|' << r.s.out[0] << '|' << r.s.in[1] << '|' << r.s.out[1] << '|'
      << (long long)std::llround(r.norm[0] * 100.0) << '|' << (long long)std::llround(r.norm[1] * 100.0) << '|' << f << '|' << r.s.bigOld[r.ext == 0 ? BIDS : OFFERS] << '|' << r.s.bigNew[r.ext == 0 ? BIDS : OFFERS] << '|' << r.s.bigSize[r.ext == 0 ? BIDS : OFFERS]
      << '|' << r.s.ticksIn[0] << '|' << r.s.ticksOut[0] << '|' << r.s.ticksIn[1] << '|' << r.s.ticksOut[1];
    char tb[40]; std::snprintf(tb, sizeof tb, "%.10g", r.tick); o << '|' << tb;
    for (int sd = 0; sd < 2; ++sd) o << '|' << r.s.bigOld[sd] << '|' << r.s.bigNew[sd] << '|' << r.s.bigSize[sd] << '|' << r.s.outOld[sd] << '|' << r.s.outNew[sd] << '|' << r.s.outSize[sd];   // (1.1.0) both sides, in + out
    return o.str();
}
inline bool parseM(const std::string& line, Read& r) {
    if (line.compare(0, 2, "M|") != 0) return false;
    std::vector<std::string> c; std::stringstream ss(line); std::string tok; while (std::getline(ss, tok, '|')) c.push_back(tok);
    if (c.size() != 19 && c.size() != 31) return false;              // (1.1.0) 31 = + both sides' largest moves in / out; 19 = a 1.0.0 line
    char* e = nullptr; std::vector<long long> v;
    for (size_t k = 1; k < 19; ++k) { if (k == 10 || k == 18) continue; errno = 0; const long long x = std::strtoll(c[k].c_str(), &e, 10); if (c[k].empty() || !e || *e) return false; v.push_back(x); }
    Read z; { char* te = nullptr; z.tick = std::strtod(c[18].c_str(), &te); if (c[18].empty() || !te || *te || !std::isfinite(z.tick) || z.tick <= 0) return false; }
    z.t = v[0]; z.ext = (int)v[1]; if (z.ext != 0 && z.ext != 1) return false; z.px = v[2];
    z.s.in[0] = v[3]; z.s.out[0] = v[4]; z.s.in[1] = v[5]; z.s.out[1] = v[6]; z.norm[0] = (double)v[7] / 100.0; z.norm[1] = (double)v[8] / 100.0;
    { const int d = z.ext == 0 ? BIDS : OFFERS; z.s.bigOld[d] = v[9]; z.s.bigNew[d] = v[10]; z.s.bigSize[d] = v[11]; }
    z.s.ticksIn[0] = v[12]; z.s.ticksOut[0] = v[13]; z.s.ticksIn[1] = v[14]; z.s.ticksOut[1] = v[15];
    if (z.s.in[0] < 0 || z.s.out[0] < 0 || z.s.in[1] < 0 || z.s.out[1] < 0 || v[11] < 0) return false;
    if (c.size() == 31) {
        long long w[12];
        for (int k = 0; k < 12; ++k) { const std::string& cs = c[19 + k]; errno = 0; char* ee = nullptr; w[k] = std::strtoll(cs.c_str(), &ee, 10); if (cs.empty() || !ee || *ee) return false; }
        for (int sd = 0; sd < 2; ++sd) { const long long* q = w + 6 * sd; if (q[2] < 0 || q[5] < 0) return false;
            z.s.bigOld[sd] = q[0]; z.s.bigNew[sd] = q[1]; z.s.bigSize[sd] = q[2]; z.s.outOld[sd] = q[3]; z.s.outNew[sd] = q[4]; z.s.outSize[sd] = q[5]; }
    }
    const std::string& f = c[10];
    for (size_t k = 0; k < f.size(); ++k) { const char ch = f[k]; if (ch == 'P') z.q[C_DEF_IN] = true; else if (ch == 'B') z.q[C_DEF_OUT] = true; else if (ch == 'a') z.q[C_ATT_IN] = true; else if (ch == 'b') z.q[C_ATT_OUT] = true; else if (ch == 'L') z.learning = true; else if (ch != '-') return false; }
    const int def = z.ext == 0 ? BIDS : OFFERS, att = 1 - def;
    auto xr = [](long long l, double n) { return n > 0 ? (double)l / n : 0.0; };
    z.x[C_DEF_IN] = xr(z.s.in[def], z.norm[def]); z.x[C_DEF_OUT] = xr(z.s.out[def], z.norm[def]); z.x[C_ATT_IN] = xr(z.s.in[att], z.norm[att]); z.x[C_ATT_OUT] = xr(z.s.out[att], z.norm[att]);
    r = z; return true;
}
// the box row "Moved", per side (column): "In 2t 2.3x" / "Back 5t 2.6x" when that side's move qualified, else "-"
inline std::string movedText(const Read& r, int side) {
    const int def = r.ext == 0 ? BIDS : OFFERS;
    const int cin = side == def ? C_DEF_IN : C_ATT_IN, cout = side == def ? C_DEF_OUT : C_ATT_OUT;
    char b[48];
    if (r.q[cin] && r.s.in[side] > 0) { std::snprintf(b, sizeof b, "In %lldt %.1fx", r.s.ticksIn[side], r.x[cin]); return b; }   // (1.1.1) no lots that way = "-" (was "In 0t 0.0x")
    if (r.q[cout] && r.s.out[side] > 0) { std::snprintf(b, sizeof b, "Out %lldt %.1fx", r.s.ticksOut[side], r.x[cout]); return b; }   // (1.1.0) "Out" (mockup v4)
    return "-";
}
// the price-chart boxes: drawn only on a P candle, for the largest move on the defenders' side (hollow at the old price, solid at the new)
inline bool chartBoxes(const Read& r) { const int d = r.ext == 0 ? BIDS : OFFERS; return drawP(r) && r.s.bigSize[d] > 0 && r.s.bigOld[d] != r.s.bigNew[d]; }
// (1.1.0) the box of one BOOK side: the largest move in the direction that side qualified (P: in, B: out); false = nothing to draw
inline bool chartBoxOf(const Read& r, int side, long long& oldPx, long long& newPx, long long& size) {
    const char L = letterOf(r, side);
    if (L == 'P') { oldPx = r.s.bigOld[side]; newPx = r.s.bigNew[side]; size = r.s.bigSize[side]; }
    else if (L == 'B') { oldPx = r.s.outOld[side]; newPx = r.s.outNew[side]; size = r.s.outSize[side]; }
    else return false;
    return size > 0 && oldPx != newPx;
}

// ==== (1.2.0) TRAILS: the chart side (SDK-free; lsDefenseLines reads the trail file, decides the stop-following B?, records, draws) ====
// The big-order size for a trail: Big Lot S20 of RESTING orders per market (LmtZonesLogic fallback table, the smaller of RTH / overnight;
// COPIED, not included, so the two plugins never break each other's build). Always >= the q90 of moves above.
inline long long trailMinOf(const std::string& m) {
    return m == "ES" ? 25 : m == "NQ" ? 15 : m == "CL" ? 15 : m == "GC" ? 9 : m == "HG" ? 5 : m == "NG" ? 10 : (m == "EU" || m == "6E") ? 41 : 20;
}
inline std::string trailPathOf(const std::string& flex, const std::string& mkt, int spb) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\BookFlow\\" + mkt + "-" + std::to_string(spb) + "-trails.csv"; }
inline std::string trailRecPathOf(const std::string& flex, const std::string& mkt, int spb) { return flex.empty() || mkt.empty() ? std::string() : flex + "\\BookFlow\\" + mkt + "-" + std::to_string(spb) + "-trails-rec.csv"; }
static const double TRAIL_GAP_ATR = 0.5;   // default follow gap: price ran >= 0.5 ATR past the candle of the last step
static const int TRAIL_N = 2;              // default N: ... and the order did not follow within N candles
static const int TRAIL_SHOW_PER_SIDE = 2;  // at most 2 trails per side on screen
struct TStepR { long long key = 0, ms = 0, px = 0; bool lad = false; };
// end: 0 open, 'F' traded (dot), 'X' pulled (x), 'B' stopped following (B?), 'b' stepped back - moved away (B?), 'R' lost (a restart)
struct Trail {
    long long id = 0; int side = BIDS; long long size = 0, key0 = 0, ms0 = 0, px0 = 0;
    std::vector<TStepR> steps;
    char end = 0; long long endKey = 0, endMs = 0, endPx = 0, endLots = 0;
    bool fromRec = false, recorded = false;
    double atrAt = 0, gapUsed = 0; int nUsed = 0;    // the B? decision's numbers (ticks)
    int moves() const { int n = 0; for (size_t k = 0; k < steps.size(); ++k) n += steps[k].lad ? 0 : 1; return n; }
    long long lastPx() const { return steps.empty() ? px0 : steps.back().px; }
    long long ticks() const { return std::llabs(lastPx() - px0); }
    long long followMs() const { return steps.empty() ? 0 : std::max(0LL, steps.back().ms - ms0); }
    bool stopLetter() const { return end == 'B' || end == 'b'; }
    void cutAfterEnd() { if (end != 'B') return; size_t k = steps.size(); while (k > 0 && steps[k - 1].key > endKey) --k; steps.resize(k); }   // steps after a B? never count
};
inline bool splitInts(const std::string& ln, size_t from, std::vector<long long>& out) {
    std::vector<std::string> c; std::stringstream ss(ln); std::string tok; while (std::getline(ss, tok, '|')) c.push_back(tok);
    out.clear();
    for (size_t k = from; k < c.size(); ++k) { errno = 0; char* e = nullptr; const long long x = std::strtoll(c[k].c_str(), &e, 10); if (c[k].empty() || !e || *e || errno) return false; out.push_back(x); }
    return true;
}
// the trail file, read in order (append-only; a second chart on the same market writes the same lines: duplicates are ignored)
class TrailSet {
public:
    std::vector<Trail> v; std::map<long long, size_t> open; long long bad = 0, restarts = 0; double tick = 0;
    void clear() { v.clear(); open.clear(); bad = 0; restarts = 0; tick = 0; }
    bool line(const std::string& raw) {
        std::string ln = raw; while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n')) ln.pop_back();
        if (ln.empty()) return true;
        if (ln.size() < 3 || ln[2] != '|') { ++bad; return false; }
        const std::string tag = ln.substr(0, 2); std::vector<long long> x;
        if (tag == "RS") {
            std::vector<std::string> c; std::stringstream ss(ln); std::string tok; while (std::getline(ss, tok, '|')) c.push_back(tok);
            char* e = nullptr; const double t = c.size() == 3 ? std::strtod(c[2].c_str(), &e) : 0.0;
            if (c.size() != 3 || !e || *e || !std::isfinite(t) || t <= 0) { ++bad; return false; }
            tick = t; ++restarts;
            for (std::map<long long, size_t>::iterator it = open.begin(); it != open.end(); ++it) { Trail& T = v[it->second];
                if (!T.end) { T.end = 'R'; T.endKey = T.steps.empty() ? T.key0 : T.steps.back().key; T.endMs = T.steps.empty() ? T.ms0 : T.steps.back().ms; T.endPx = T.lastPx(); } }
            open.clear(); return true;
        }
        if (!splitInts(ln, 1, x)) { ++bad; return false; }
        if (tag == "TS") {
            if (x.size() != 6 || (x[1] != BIDS && x[1] != OFFERS) || x[2] <= 0) { ++bad; return false; }
            std::map<long long, size_t>::iterator it = open.find(x[0]);
            if (it != open.end() && v[it->second].key0 == x[3] && v[it->second].ms0 == x[4]) return true;   // the same start twice (2 charts)
            Trail T; T.id = x[0]; T.side = (int)x[1]; T.size = x[2]; T.key0 = x[3]; T.ms0 = x[4]; T.px0 = x[5];
            if (it != open.end()) { Trail& O = v[it->second]; if (!O.end) { O.end = 'R'; O.endKey = O.steps.empty() ? O.key0 : O.steps.back().key; O.endMs = O.steps.empty() ? O.ms0 : O.steps.back().ms; O.endPx = O.lastPx(); } }
            v.push_back(T); open[T.id] = v.size() - 1; return true;
        }
        std::map<long long, size_t>::iterator it = x.empty() ? open.end() : open.find(x[0]);
        if (tag == "TP") {
            if ((x.size() != 5 && x.size() != 7) || (x[4] != 0 && x[4] != 1)) { ++bad; return false; }   // 7: + dist / ran (1.2.0 final)
            if (it == open.end()) return true;                        // a step of a trail whose start is not in this file (rotated): ignored
            Trail& T = v[it->second]; if (T.end) return true;
            for (size_t k = 0; k < T.steps.size(); ++k) if (T.steps[k].ms == x[2] && T.steps[k].px == x[3]) return true;   // duplicate
            TStepR s; s.key = x[1]; s.ms = x[2]; s.px = x[3]; s.lad = x[4] == 1; T.steps.push_back(s); return true;
        }
        if (tag == "TF" || tag == "TX" || tag == "TB") {
            if (x.size() != (tag == "TF" ? 5u : 4u)) { ++bad; return false; }
            if (it == open.end()) return true;
            Trail& T = v[it->second];
            if (!T.end) { T.end = tag == "TF" ? 'F' : tag == "TX" ? 'X' : 'b'; T.endKey = x[1]; T.endMs = x[2]; T.endPx = x[3]; T.endLots = tag == "TF" ? x[4] : 0; }
            open.erase(it); return true;
        }
        ++bad; return false;
    }
};
// the record line for the nightly (one per ended trail, written once; prices in ticks, tick size included):
// TR|1|id|side|size|key0|ms0|px0|lastPx|moves|ticks|followMs|end|endKey|endMs|endPx|endLots|atr*100|gap*100|N|tick
inline std::string trLineOf(const Trail& T, double tick) {
    std::ostringstream o; char tb[40]; std::snprintf(tb, sizeof tb, "%.10g", tick);
    o << "TR|1|" << T.id << '|' << T.side << '|' << T.size << '|' << T.key0 << '|' << T.ms0 << '|' << T.px0 << '|' << T.lastPx() << '|' << T.moves() << '|' << T.ticks() << '|' << T.followMs()
      << '|' << (T.end ? T.end : '-') << '|' << T.endKey << '|' << T.endMs << '|' << T.endPx << '|' << T.endLots << '|' << (long long)std::llround(T.atrAt * 100.0) << '|' << (long long)std::llround(T.gapUsed * 100.0) << '|' << T.nUsed << '|' << tb;
    return o.str();
}
struct TrEnd { char end = 0; long long endKey = 0, endMs = 0, endPx = 0, endLots = 0; double atr = 0, gap = 0; int n = 0; };
inline bool parseTR(const std::string& raw, long long& id, long long& key0, TrEnd& e) {
    std::string ln = raw; while (!ln.empty() && (ln.back() == '\r' || ln.back() == '\n')) ln.pop_back();
    if (ln.compare(0, 5, "TR|1|") != 0) return false;
    std::vector<std::string> c; std::stringstream ss(ln); std::string tok; while (std::getline(ss, tok, '|')) c.push_back(tok);
    if (c.size() != 21 || c[12].size() != 1) return false;
    { char* te = nullptr; const double tk = std::strtod(c[20].c_str(), &te); if (c[20].empty() || !te || *te || !std::isfinite(tk) || tk <= 0) return false; }
    const char en = c[12][0]; if (en != 'F' && en != 'X' && en != 'B' && en != 'b' && en != 'R') return false;
    std::vector<long long> x;
    for (size_t k = 2; k < 20; ++k) { if (k == 12) { x.push_back(0); continue; } errno = 0; char* q = nullptr; const long long y = std::strtoll(c[k].c_str(), &q, 10); if (c[k].empty() || !q || *q || errno) return false; x.push_back(y); }
    id = x[0]; key0 = x[3]; e.end = en; e.endKey = x[11]; e.endMs = x[12]; e.endPx = x[13]; e.endLots = x[14]; e.atr = (double)x[15] / 100.0; e.gap = (double)x[16] / 100.0; e.n = (int)x[17];
    return true;
}
// one chart candle in ticks
struct TBar { long long key = 0, hi = 0, lo = 0, cl = 0; };
inline int barIdx(const std::vector<TBar>& b, long long key) {     // the candle with this key, else the first one after it (-1 = none)
    size_t a = 0, z = b.size(); while (a < z) { const size_t m = (a + z) / 2; if (b[m].key < key) a = m + 1; else z = m; }
    return a < b.size() ? (int)a : -1;
}
inline double atrAtIdx(const std::vector<TBar>& b, int i, int n = 14) {
    double s = 0; int k = 0;
    for (int j = i; j >= 1 && k < n; --j, ++k) s += (double)std::max(b[j].hi - b[j].lo, std::max(std::llabs(b[j].hi - b[j - 1].cl), std::llabs(b[j].lo - b[j - 1].cl)));
    if (k == 0 && i >= 0 && i < (int)b.size()) return (double)(b[i].hi - b[i].lo);
    return k ? s / k : 0.0;
}
// THE STOP-FOLLOWING B?: after each step (in its candle s), price RAN on - a bid: a high >= gapAtr x ATR above candle s's high (an offer:
// a low that far below its low) first in candle m - and the order made no step through candle e = m + N - 1: B? at e's close. A traded /
// pulled / stepped-back end in an earlier candle wins; the same candle: the order's own end wins. lastDecidable = the last candle allowed
// to decide (closed, with the trail file caught up). A trail already decided (from the record) never changes. Returns the trails ended now.
inline int decideStops(TrailSet& T, const std::vector<TBar>& bars, int lastDecidable, double gapAtr, int N) {
    if (bars.empty() || !(gapAtr > 0) || N < 1) return 0;
    if (lastDecidable >= (int)bars.size()) lastDecidable = (int)bars.size() - 1;
    int ended = 0;
    for (size_t t = 0; t < T.v.size(); ++t) {
        Trail& R = T.v[t]; if (R.fromRec || R.end == 'B') continue;
        if (R.end == 'R') continue;                                       // lost at a restart: never judged
        const int fe = R.end ? barIdx(bars, R.endKey) : -1;                // the candle of the order's own end (traded / pulled / back)
        if (R.end && (fe < 0 || bars[(size_t)fe].key != R.endKey)) continue;   // its end is not on this chart: keep it as it is
        const int limit = R.end ? std::min(lastDecidable, fe - 1) : lastDecidable;
        bool decided = false;
        // the step candles (the first step's candle starts it)
        std::vector<int> si; std::vector<long long> spx;
        for (size_t k = 0; k < R.steps.size(); ++k) { const int b = barIdx(bars, R.steps[k].key); si.push_back(b); spx.push_back(R.steps[k].px); }
        for (size_t k = 0; k < si.size() && !decided; ++k) {
            const int s = si[k]; if (s < 0 || bars[(size_t)s].key != R.steps[k].key) continue;
            const int next = k + 1 < si.size() ? (si[k + 1] < 0 ? (int)bars.size() : si[k + 1]) : INT_MAX;
            if (next == s) continue;                                       // another step in the same candle: judge from the last one
            const double atr = atrAtIdx(bars, s), gap = std::max(1.0, gapAtr * atr);
            const long long ref = R.side == BIDS ? bars[(size_t)s].hi : bars[(size_t)s].lo;
            long long run = ref;
            for (int j = s + 1; j < (int)bars.size() && j <= limit && j < next; ++j) {
                run = R.side == BIDS ? std::max(run, bars[(size_t)j].hi) : std::min(run, bars[(size_t)j].lo);
                if ((double)(R.side == BIDS ? run - ref : ref - run) + 1e-9 < gap) continue;
                const int e = j + N - 1;
                if (e >= next || e > limit) break;                         // it followed in time / not decidable yet / its own end first
                R.end = 'B'; R.endKey = bars[(size_t)e].key; R.endMs = 0; R.endPx = spx[k]; R.endLots = 0; R.atrAt = atr; R.gapUsed = gap; R.nUsed = N;
                R.cutAfterEnd();
                decided = true; ++ended; break;
            }
        }
    }
    return ended;
}
// a trail's end is FINAL (can be recorded) when it cannot change any more: B? / R, or the order's own end with every candle before it judged
inline bool finalEnd(const Trail& R, const std::vector<TBar>& bars, int lastDecidable) {
    if (!R.end) return false;
    if (R.fromRec || R.end == 'B' || R.end == 'R') return true;
    const int fe = barIdx(bars, R.endKey);
    if (fe < 0 || bars[(size_t)fe].key != R.endKey) return true;     // not on this chart (older): as the file says
    return fe - 1 <= lastDecidable;
}
// the trails to draw: per side the TRAIL_SHOW_PER_SIDE latest started among those that touch [keyLo, keyHi]
inline std::vector<size_t> shownTrails(const TrailSet& T, long long keyLo, long long keyHi, int perSide = TRAIL_SHOW_PER_SIDE) {
    std::vector<size_t> out; int n[2] = {0, 0};
    for (size_t k = T.v.size(); k-- > 0;) {
        const Trail& R = T.v[k]; if (R.side != BIDS && R.side != OFFERS) continue;
        if (R.key0 > keyHi || (R.end && R.endKey < keyLo)) continue;
        if (n[R.side] >= perSide) continue;
        ++n[R.side]; out.push_back(k);
    }
    std::reverse(out.begin(), out.end()); return out;
}
inline std::string durText(long long ms) {
    const long long s = std::max(0LL, ms / 1000); char b[32];
    if (s >= 3600) std::snprintf(b, sizeof b, "%lldh %02lldm", s / 3600, s / 60 % 60); else if (s >= 60) std::snprintf(b, sizeof b, "%lldm %02llds", s / 60, s % 60); else std::snprintf(b, sizeof b, "%llds", s);
    return b;
}
// the hover (rows <= ~115 characters): numbers first, then question / what / why / use / example, "?" last
inline std::vector<std::string> trailHover(const Trail& R, const std::string& pxFrom, const std::string& pxTo, const std::string& pxEnd, const std::string& clkFrom, const std::string& clkEnd) {
    const bool bid = R.side == BIDS; const std::string mid = " \xB7 ";
    std::vector<std::string> L; char b[200];
    std::snprintf(b, sizeof b, "Trailing %s %lld lots%sstepped %dx%s%lld ticks %s%sfollowed %s", bid ? "BID" : "OFFER", R.size, mid.c_str(), R.moves(), mid.c_str(), R.ticks(), bid ? "up" : "down", mid.c_str(), durText(R.followMs()).c_str());
    L.push_back(b);
    std::string end;
    if (R.end == 'B') { std::snprintf(b, sizeof b, "End: B? stopped following - price ran %.0f ticks (%.1f ATR) on and it did not follow in %d candle%s (%s)", R.gapUsed, R.atrAt > 0 ? R.gapUsed / R.atrAt : 0.0, R.nUsed, R.nUsed == 1 ? "" : "s", clkEnd.c_str()); end = b; }
    else if (R.end == 'b') end = "End: B? stepped back - re-priced AWAY from price to " + pxEnd + " (" + clkEnd + ")";
    else if (R.end == 'F') { std::snprintf(b, sizeof b, "End: traded at %s (%lld lots hit, %s) - price came back to it", pxEnd.c_str(), R.endLots, clkEnd.c_str()); end = b; }
    else if (R.end == 'X') end = "End: pulled (cancelled) at " + pxEnd + " (" + clkEnd + ")";
    else if (R.end == 'R') end = "End: lost - IRT / the order feed restarted (MBO has no history)";
    else end = std::string("Still following - last step to ") + pxTo;
    L.push_back("From " + pxFrom + " to " + pxTo + ", since " + clkFrom);
    L.push_back(end);
    L.push_back(bid ? "Question: is a big bid following price up during the rally - and has it stopped?" : "Question: is a big offer following price down during the selloff - and has it stopped?");
    L.push_back(bid ? "What: ONE resting bid (one order id) re-priced UP behind price again and again - the stepped green line." : "What: ONE resting offer (one order id) re-priced DOWN above price again and again - the stepped red line.");
    L.push_back("Why: each re-price gives up its queue place - a trader paying that to stay close to price is supporting the move.");
    L.push_back(bid ? "Use: a bid that stops following under a rally = buyers quitting - watch for the turn." : "Use: an offer that stops following over a selloff = sellers quitting - watch for the turn.");
    L.push_back(bid ? "Dot = it traded (defended) \xB7 x = pulled \xB7 B? = stopped following / stepped back (support quitting)." : "Dot = it traded (defended) \xB7 x = pulled \xB7 B? = stopped following / stepped back (resistance quitting).");
    L.push_back(bid ? "Example: an 85-lot NQ bid steps 4x from 21,300 to 21,312 under the rally, then stops: B?; the high prints 2 candles later."
                    : "Example: a 60-lot NQ offer steps 5x from 21,340 to 21,322 over the selloff, then stops: B?; the low prints next candle.");
    L.push_back("? = not proven yet: the nightly scores whether B? comes before the turn.");
    return L;
}
} // namespace mvl
#endif
