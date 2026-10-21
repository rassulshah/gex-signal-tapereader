#ifndef LS_MOVE_LOGIC_H
#define LS_MOVE_LOGIC_H
// MoveLogic.h 1.1.0 (2026-10-21; 1.1.0 14:18 CT Rassul "i want you to draw them" - all FOUR cases drawn, mockup v4: P = pressing in /
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
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace mvl {
static const char* const VERSION = "1.1.1";   // (1.1.1, 2026-10-21) letterOf / movedText: a side whose case flag is set but moved no lots that way shows "-" and no letter (the box showed "In 0t 0.0x")
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
    long long seenM = 0, seenR = 0, following = 0, ladder = 0, shortRest = 0, small = 0, ambiguous = 0, confirmed = 0;
    // PATH M: a known order re-priced. ref = the last trade now; refAt = the last trade when it was placed / last moved
    void modify(long long id, int side, long long oldPx, long long newPx, long long size, long long refAt, long long ref, long long ms) {
        if (oldPx == newPx || side < 0 || side > 1) return;
        ++seenM; ladderNote(side, newPx - oldPx, ms);
        candidate(id, side, oldPx, newPx, size, distOf(side, oldPx, refAt), distOf(side, newPx, ref), ms, false, 0);
    }
    // PATH R step 1: a resting order cancelled (not because it traded)
    void cancel(long long id, int side, long long px, long long size, long long refAt, long long ms) {
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
        prune(ms);
        while (!pend.empty() && (pend.front().dead || ms - pend.front().m.ms >= REST_MS)) {
            Pend p = pend.front(); pend.pop_front(); ++base;
            std::unordered_map<long long, size_t>::iterator it = byId.find(p.m.id);
            if (it != byId.end() && it->second == base - 1) byId.erase(it);
            if (p.dead) continue;
            if (ladderCount(p.m.side, p.m.newPx - p.m.oldPx, p.m.ms) >= LADDER_N) { ++ladder; continue; }
            if (p.m.viaR && cancelMatches(p.cancelId) != 1) { ++ambiguous; continue; }
            if (p.m.size < minLots) { ++small; continue; }
            ++confirmed; out.push_back(p.m);
        }
        rebase();
    }
    size_t pending() const { return pend.size(); }
private:
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
} // namespace mvl
#endif
