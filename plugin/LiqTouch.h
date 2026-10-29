#ifndef LS_LIQTOUCH_H
#define LS_LIQTOUCH_H
// LiqTouch.h 1.1.0 (2026-10-28 14:04 CT, Rassul approved the new liquidity-column candidate rule): with Params.zoneGate (lsLiquidityProfile
// sets it live) a BIG LEVEL is no longer "size >= the p97 tier" but a ZONE CANDIDATE (pickZones below, one rule shared with lsDealerProfile):
//   real orders only (rested >= 30 s at their price, re-priced < 2x in 5 min - quoting / market-maker style orders excluded); zone = the real
//   lots at a price + its +-1 tick neighbours (same side), at the price with the most lots; passes when zone >= 2.0x the MEDIAN real level
//   (non-zero, same side, within 3 ATR; the median recomputed every 60 s); each node its own distribution (>= 3 ticks from a bigger node and
//   the smallest raw level between them <= 0.7x the smaller, else the bigger is kept); bids within 3 ATR below the last trade, offers above.
//   Every passing node is followed at once (its orders already rested 30 s); a touch = the first trade inside the zone (price +-1 tick).
//   The p97 tier is kept as a LOG field only in that mode. zoneGate false = the 1.0.0 study engine exactly (parity tests).
// LiqTouch.h 1.0.0 (2026-10-28, Rassul approved the liquidity-column build 11:05 CT; HELD added 11:08 CT "i also like the held signal ...
// i like to show it when it held") -- THE TOUCH TAG of a big resting level, decided natively from the order-by-order feed (IRT
// DEPTH_BY_ORDER) inside lsLiquidityProfile. SDK-free, C++11, no files except the params parse.
//
// Ported EXACTLY from the 10-28 liquidity study (scratchpad liqfull: eng2.py wall episodes at the market's p97 tier + touch.py reads):
//   BIG LEVEL   a price whose resting size (all orders there) is >= the market tier (p97 of book levels, lots) at every 1-s boundary for
//               30 s while 2 ticks .. 3 ATR from the last trade; followed until the first trade at it (the TOUCH), a pull (under 30% of
//               its max for 10 s), a drift (price > 6 ATR away) or 60 min.
//   AT 15 s     after the first trade (decided only while price has neither held 0.5 ATR away nor broken 0.25 ATR through - else EARLY):
//               STAYS  = size left >= 50% of the size at the touch AND added >= half of what was eaten          (holds 54% vs 43%)
//               STAYS+ = STAYS, price spent <= 4 s at the level (within 1 tick) and churn (added + pulled) / size < 15x (NG: no churn rule)
//                                                                                                                (64% vs 43%)
//               STAYS weak = STAYS but price sat 14-15 s at the level (grey, no vote)                             (46% vs 37%)
//               GONE   = size left < 30% of the size at the touch                                                (25% vs 42%)
//               GONE+  = GONE with added < 25% of eaten, or more than its max size pulled in the last minute before the touch (23.5%)
//               MID    = neither (no tag, logged)
//   VOTES       (Tape Reader / combiner, once per touch) STAYS+ +2 defender, STAYS +1, weak 0, defenders' side also aggressive (net
//               aggression <= -0.2 toward the defenders) +1 extra, GONE +1 attacker, GONE+ or an all-in one-sided attack (net >= 0.6) into
//               a non-STAYS level +2 attacker; the king's state before the touch votes 0. Posted signed: + bullish / - bearish.
//   OUTCOME     HELD = price went 0.5 ATR away on the defended side before 0.25 ATR through (study 'hold', from +15 s, 10 min);
//               BROKE = 0.25 ATR through first; EXPIRED = neither in 10 min. Final once decided (never repaints).
// ATR = this engine's own 3-min ATR in ticks (20 bars of true range from its 1-s series, >= 10 bars, floor 2 ticks); before 10 bars the
// host's ATR (ticks) is used. Times: milliseconds on one clock (the host passes Central wall-clock ms); 1-s boundaries on whole seconds.
// Event semantics (IRT): NEW adds, MOD changes size / re-prices (re-price = the whole old size pulled + the new size added), DEL pulls
// what is left, FILL = a trade against a resting order: the level loses min(fill, order size) as EATEN (Databento's F + C/M split is the
// same thing). A FILL is also the trade print: last / high / low, and the aggressor = the side that hit the resting order.
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <map>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <sstream>
namespace ltc {
static const char* const VERSION = "1.2.0";   // (1.2.0, 2026-10-29) + the KING nodes above / below (kingStep)
enum Side { BID = 0, ASK = 1 };
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Tag { T_PENDING = 0, T_STAYS_PLUS = 1, T_STAYS = 2, T_STAYS_WEAK = 3, T_GONE = 4, T_GONE_PLUS = 5, T_MID = 6, T_EARLY = 7 };
enum Outcome { O_OPEN = 0, O_HELD = 1, O_BROKE = 2, O_EXPIRED = 3 };
inline const char* tagText(int t) { return t == T_STAYS_PLUS ? "STAYS+" : t == T_STAYS ? "STAYS" : t == T_STAYS_WEAK ? "STAYS" : t == T_GONE ? "GONE" : t == T_GONE_PLUS ? "GONE+" : ""; }
inline const char* tagName(int t) { return t == T_STAYS_PLUS ? "STAYS+" : t == T_STAYS ? "STAYS" : t == T_STAYS_WEAK ? "STAYS_WEAK" : t == T_GONE ? "GONE" : t == T_GONE_PLUS ? "GONE+" : t == T_MID ? "MID" : t == T_EARLY ? "EARLY" : "PENDING"; }
inline const char* outName(int o) { return o == O_HELD ? "HELD" : o == O_BROKE ? "BROKE" : o == O_EXPIRED ? "EXPIRED" : "OPEN"; }
inline bool drawnTag(int t) { return t >= T_STAYS_PLUS && t <= T_GONE_PLUS; }
// colour by MEANING: +1 green (good for buyers), -1 red, 0 grey. A bid that STAYS / an offer that is GONE = green; an offer that STAYS /
// a bid that is GONE = red; STAYS weak = grey.
inline int tagColour(int tag, int side) {
    if (tag == T_STAYS_WEAK) return 0;
    const bool stays = tag == T_STAYS_PLUS || tag == T_STAYS, gone = tag == T_GONE || tag == T_GONE_PLUS;
    if (!stays && !gone) return 0;
    const bool bid = side == BID;
    return (stays == bid) ? 1 : -1;
}
// HELD colour: a bid that held = green, an offer that held = red (as the order-line HELD word)
inline int heldColour(int side) { return side == BID ? 1 : -1; }

// ------------------------------------------------------------------------------------------------------------- per-market params
struct Params {
    std::string market; long long tier[2] = { 72, 72 };   // [0] overnight, [1] RTH - lots
    int farWait[2] = { 60, 60 };                            // far-king (> 3 ATR) bright wait, seconds (0 = none) - read by lsDealerProfile
    int dwellPlus = 4, dwellWeak = 14; double churnCut = 15.0;   // churnCut 0 = no churn rule (NG)
    bool fromFile = false; std::string why = "defaults (no params file)";
    // (1.1.0) the zone candidate rule (zoneGate false = the 1.0.0 tier gate)
    bool zoneGate = false; double zoneX = 2.0, sepX = 0.7; int sepTicks = 3, perSide = 3; double reachAtr = 3.0;
    long long realMs = 30000, moveWinMs = 300000, medEveryMs = 60000; int maxMoves = 2;
};
// (1.1.0) ONE candidate rule for the liquidity column and the touch engine. real[sd]: price (ticks) -> REAL resting lots (already filtered:
// rested >= realMs, re-priced < maxMoves in moveWinMs). Bids are taken at last-R .. last-1, offers at last+1 .. last+R. med[sd] = the median
// real level of that side (caller recomputes it every 60 s). Returns every passing, distinct node per side, biggest first (not capped: the
// caller shows perSide per side; the rest are challengers for the steady board).
struct Zone { int side = BID; long long px = 0, lots = 0, raw = 0; };
inline double medianOf(std::vector<long long> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end()); const size_t n = v.size();
    return n % 2 ? (double)v[n / 2] : 0.5 * ((double)v[n / 2 - 1] + (double)v[n / 2]); }
inline double sideMedian(const std::map<long long, long long>& real, int sd, long long last, long long R) {
    std::vector<long long> v; for (std::map<long long, long long>::const_iterator i = real.begin(); i != real.end(); ++i) {
        if (i->second <= 0) continue;
        const long long d = sd == BID ? last - i->first : i->first - last; if (d >= 1 && d <= R) v.push_back(i->second); }
    return medianOf(v); }
inline std::vector<Zone> pickZones(const std::map<long long, long long>& real, int sd, long long last, long long R, double med, const Params& P) {
    std::vector<Zone> c; if (!(med > 0) || last <= 0 || R <= 0) return c;
    auto at = [&](long long p) -> long long { std::map<long long, long long>::const_iterator i = real.find(p); return i == real.end() ? 0 : std::max(0LL, i->second); };
    std::map<long long, Zone> byPx;                                   // the zone price -> the best window that names it
    for (long long j = 1; j <= R; ++j) {
        const long long p = sd == BID ? last - j : last + j; if (p <= 0) break;
        const long long a = at(p - 1), m = at(p), b = at(p + 1), z = a + m + b; if (m <= 0 && a <= 0 && b <= 0) continue;
        if ((double)z < P.zoneX * med) continue;
        long long zp = p, zm = m; if (a > zm) { zp = p - 1; zm = a; } if (b > zm) { zp = p + 1; zm = b; }   // ties keep the window centre
        const long long dz = sd == BID ? last - zp : zp - last; if (dz < 1 || dz > R) continue;
        std::map<long long, Zone>::iterator h = byPx.find(zp);
        if (h == byPx.end() || z > h->second.lots) { Zone Z; Z.side = sd; Z.px = zp; Z.lots = z; Z.raw = zm; byPx[zp] = Z; }
    }
    std::vector<Zone> all; for (std::map<long long, Zone>::const_iterator i = byPx.begin(); i != byPx.end(); ++i) all.push_back(i->second);
    std::sort(all.begin(), all.end(), [](const Zone& x, const Zone& y) { return x.lots != y.lots ? x.lots > y.lots : x.px < y.px; });
    for (const Zone& z : all) {
        bool ok = true;
        for (const Zone& k : c) {
            const long long lo = std::min(k.px, z.px), hi = std::max(k.px, z.px);
            if (hi - lo < P.sepTicks) { ok = false; break; }
            long long mn = -1; for (long long p = lo + 1; p < hi; ++p) { const long long v = at(p); if (mn < 0 || v < mn) mn = v; }
            if ((double)mn > P.sepX * (double)z.lots) { ok = false; break; }   // no valley between them: the same distribution - keep the bigger
        }
        if (ok) c.push_back(z);
    }
    return c;
}
inline Params defaults(const std::string& m) {
    Params p; p.market = m;
    long long t = 72; int w = 60; double ch = 15.0;
    if (m == "ES") { t = 72; w = 60; } else if (m == "NQ") { t = 12; w = 300; } else if (m == "CL") { t = 16; w = 300; }
    else if (m == "GC") { t = 10; w = 300; } else if (m == "HG") { t = 10; w = 60; } else if (m == "NG") { t = 30; w = 60; ch = 0.0; }
    else if (m == "EU" || m == "6E") { t = 128; w = 0; }
    p.tier[0] = p.tier[1] = t; p.farWait[0] = p.farWait[1] = w; p.churnCut = ch;
    return p;
}
// RTH window per market, Central minutes [first, last)
inline void rthOf(const std::string& m, int& a, int& b) {
    a = 8 * 60 + 30; b = 15 * 60;
    if (m == "CL" || m == "NG") { a = 8 * 60; b = 13 * 60 + 30; }
    else if (m == "GC") { a = 7 * 60 + 20; b = 12 * 60 + 30; }
    else if (m == "HG") { a = 7 * 60 + 20; b = 12 * 60; }
    else if (m == "EU" || m == "6E") { a = 7 * 60 + 20; b = 14 * 60; }
}
inline int rthIdx(const std::string& m, long long ctMs) {
    if (ctMs <= 0) return 1;
    const long long s = ((ctMs / 1000) % 86400 + 86400) % 86400; const int mod = (int)(s / 60);
    int a, b; rthOf(m, a, b); return (mod >= a && mod < b) ? 1 : 0;
}
namespace detail {
inline bool numI(const std::string& s, long long& v) { if (s.empty() || s.size() > 12) return false; char* e = nullptr; v = std::strtoll(s.c_str(), &e, 10); return e && *e == 0; }
inline bool numD(const std::string& s, double& v) { if (s.empty() || s.size() > 20) return false; char* e = nullptr; v = std::strtod(s.c_str(), &e); return e && *e == 0 && std::isfinite(v); }
inline std::vector<std::string> split(const std::string& s) { std::vector<std::string> o; std::string c; for (char ch : s) { if (ch == ',') { o.push_back(c); c.clear(); } else c += ch; } o.push_back(c); return o; }
}
// lsFlexLevels\LRA-LiqTouch-<MKT>.csv (written nightly by lra.liq_touch_fit):
//   VERSION,1 / MARKET,<MKT> / TIER,RTH|ON,<lots> / FARWAIT,RTH|ON,<s> / DWELL_PLUS,<s> / DWELL_WEAK,<s> / CHURN,<x or 0> / # comments / FIT,... (info)
// Strict: VERSION first, MARKET must match, every value in range; anything wrong = the defaults + the reason (said in the status line).
inline Params parse(const std::string& text, const std::string& mkt) {
    Params d = defaults(mkt);
    if (text.size() > 65536) { d.why = "params file too big - defaults"; return d; }
    Params p = d; bool ver = false, mk = false; int tiers = 0; std::stringstream ss(text); std::string line; int n = 0;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (++n > 512) { d.why = "params file too long - defaults"; return d; }
        const std::vector<std::string> f = detail::split(line);
        if (!ver) { if (f.size() == 2 && f[0] == "VERSION" && f[1] == "1") { ver = true; continue; } d.why = "params file: VERSION,1 must come first - defaults"; return d; }
        if (f[0] == "MARKET") { if (f.size() != 2 || (f[1] != mkt && !(mkt == "EU" && f[1] == "6E"))) { d.why = "params file: wrong market - defaults"; return d; } mk = true; continue; }
        long long v = 0; double x = 0;
        if (f[0] == "TIER" && f.size() == 3 && (f[1] == "RTH" || f[1] == "ON")) {
            if (!detail::numI(f[2], v) || v < 1 || v > 100000) { d.why = "params file: bad TIER - defaults"; return d; }
            p.tier[f[1] == "RTH" ? 1 : 0] = v; ++tiers;
        } else if (f[0] == "FARWAIT" && f.size() == 3 && (f[1] == "RTH" || f[1] == "ON")) {
            if (!detail::numI(f[2], v) || v < 0 || v > 3600) { d.why = "params file: bad FARWAIT - defaults"; return d; }
            p.farWait[f[1] == "RTH" ? 1 : 0] = (int)v;
        } else if (f[0] == "DWELL_PLUS" && f.size() == 2) {
            if (!detail::numI(f[1], v) || v < 0 || v > 15) { d.why = "params file: bad DWELL_PLUS - defaults"; return d; } p.dwellPlus = (int)v;
        } else if (f[0] == "DWELL_WEAK" && f.size() == 2) {
            if (!detail::numI(f[1], v) || v < 1 || v > 15) { d.why = "params file: bad DWELL_WEAK - defaults"; return d; } p.dwellWeak = (int)v;
        } else if (f[0] == "CHURN" && f.size() == 2) {
            if (!detail::numD(f[1], x) || x < 0 || x > 1000) { d.why = "params file: bad CHURN - defaults"; return d; } p.churnCut = x;
        } else if (f[0] == "FIT") { /* information only */ }
        else { d.why = "params file: unknown row '" + f[0].substr(0, 20) + "' - defaults"; return d; }
    }
    if (!ver || !mk || tiers < 2) { d.why = "params file incomplete (VERSION / MARKET / both TIER rows) - defaults"; return d; }
    if (p.dwellPlus >= p.dwellWeak) { d.why = "params file: DWELL_PLUS must be under DWELL_WEAK - defaults"; return d; }
    p.fromFile = true; p.why = "nightly params"; return p;
}

// ------------------------------------------------------------------------------------------------------------- the touch record
struct Touch {
    long long id = 0;                 // touch time ms (unique with side + px)
    int side = BID; long long px = 0; // level (ticks)
    long long tMs = 0, secEnd = 0;    // the touch event time; the end of the second it fell in (the study's stt)
    long long t0 = 0, sz0 = 0, szT = 0, szMax = 0, sz60 = 0, pull60 = 0;
    long long sa = 0, se = 0, sp = 0; // add / eat / pull counters at the touch
    long long a15 = -1, e15 = 0, p15 = 0, s15 = 0, a30 = -1, e30 = 0, p30 = 0, s30 = 0;
    int dwell15 = 0, dwell30 = 0; long long g15 = 0, opp15 = 0, g30 = 0, opp30 = 0;
    long long hi15 = 0, lo15 = 0;     // price extremes in the 15 decision seconds
    double A = 0;                     // ATR (ticks) at the touch second
    int tag = T_PENDING, tag30 = T_PENDING, vote = 0, defV = 0, attV = 0; double nd = 0, churn = 0, refill = 0; bool decided = false, decided30 = false;
    int outcome = O_OPEN; long long outMs = 0; bool outQueued = false;
    int tap = 0; bool king = false; double dom = 0; long long kingSz = 0, secondSz = 0; bool onMq = false; std::string mqName;
};
// STAYS / MID / GONE of a snapshot (study: stays = s >= 0.5 szT and a >= 0.5 max(e, 1); GONE = s < 0.3 szT)
inline int stateOf(long long szT, long long a, long long e, long long s) {
    if ((double)s >= 0.5 * (double)szT && (double)a >= 0.5 * (double)std::max(e, 1LL)) return 1;   // STAYS
    if ((double)s < 0.3 * (double)szT) return -1;                                                      // GONE
    return 0;
}
// the 15-s tag, votes and the signed (bullish +) total
inline void decide(const Params& P, Touch& t) {
    const int st = stateOf(t.szT, t.a15, t.e15, t.s15);
    t.churn = (double)(t.a15 + t.p15) / (double)std::max(t.szT, 1LL);
    t.refill = (double)t.a15 / (double)std::max(t.e15, 1LL);
    t.nd = (double)(t.g15 - t.opp15) / (double)std::max(t.g15 + t.opp15, 1LL);
    const double pullX = (double)t.pull60 / (double)std::max(t.szMax, 1LL);
    if (st == 1) {
        if (t.dwell15 <= P.dwellPlus && (P.churnCut <= 0 || t.churn < P.churnCut)) t.tag = T_STAYS_PLUS;
        else if (t.dwell15 >= P.dwellWeak) t.tag = T_STAYS_WEAK;
        else t.tag = T_STAYS;
    } else if (st == -1) t.tag = (t.refill < 0.25 || pullX > 1.0) ? T_GONE_PLUS : T_GONE;
    else t.tag = T_MID;
    int def = t.tag == T_STAYS_PLUS ? 2 : t.tag == T_STAYS ? 1 : 0;
    if (st == 1 && t.nd < -0.2) def += 1;
    int att = t.tag == T_GONE_PLUS ? 2 : t.tag == T_GONE ? 1 : 0;
    if (st != 1 && t.nd > 0.6) att = 2;
    t.defV = def; t.attV = att;
    const int net = def - att; t.vote = t.side == BID ? net : -net;
    t.decided = true;
}

// ------------------------------------------------------------------------------------------------------------- the engine
struct MqLevel { long long px = 0; std::string name; };
class Engine {
public:
    Params P;
    long long origin = -1;            // ATR block origin (ms); -1 = clock-aligned 3-min blocks
    double hostAtr = 0;               // ticks, used before 10 own blocks exist (0 = none)
    std::vector<std::pair<long long, double> > atrOverride;   // tests: (from ms, ATR ticks) - replaces the own ATR when set
    std::vector<MqLevel> mq;          // MenthorQ levels (ticks) for the log only
    long long nEvents = 0, nTouches = 0, nEpisodes = 0, nUnknown = 0, nGap = 0;
    explicit Engine(const Params& p = Params()) : P(p) {}
    void newSession() { live.clear(); out.clear(); }   // a new 17:00 session: touches start again, the book and the followed levels go on
    void reset() { cand.clear(); medAt = -1; med[0] = med[1] = 0; ord.clear(); lev.clear(); eps.clear(); live.clear(); out.clear(); hist.clear(); blocks.clear(); nextT = 0; last = hi = lo = -1; bv = sv = 0; lastSec = -1; started = false; }
    // tests: start the 1-s clock at a boundary and seed the last trade (the live engine starts at its first event)
    void begin(long long boundaryMs, long long lastPx) { nextT = boundaryMs; started = true; if (lastPx > 0) last = lastPx; }
    // one MBO event. side: the event's raw side (BID / ASK, -1 unknown); implied: a spread-driven fill (no aggressor)
    void onEvent(int act, int side, long long px, long long size, long long id, long long ms, bool implied = false) {
        if (ms <= 0 || px <= 0 || size < 0 || act < NEW || act > FILL) return;
        if (!started) { nextT = (ms / 1000) * 1000 + 1000; started = true; }
        advance(ms);
        ++nEvents;
        std::unordered_map<long long, Ord>::iterator it = ord.find(id);
        switch (act) {
        case NEW:
            if (side != BID && side != ASK) return;
            if (it != ord.end()) { Lev& o = L(it->second.side, it->second.px); o.size -= it->second.size; it->second = Ord{ side, px, size, ms }; L(side, px).size += size; return; }
            ord[id] = Ord{ side, px, size, ms }; { Lev& l = L(side, px); l.size += size; l.cAdd += size; } return;
        case MOD:
            if (it == ord.end()) { if (side != BID && side != ASK) return; ord[id] = Ord{ side, px, size, ms }; Lev& l = L(side, px); l.size += size; l.cAdd += size; return; }
            {   Ord& o = it->second;
                if (px != o.px) { Lev& a = L(o.side, o.px); a.size -= o.size; a.cPull += o.size; Lev& b = L(o.side, px); b.size += size; b.cAdd += size; o.px = px; o.size = size; o.mv1 = o.mv0; o.mv0 = ms; o.t0 = ms; return; }
                const long long ch = size - o.size; Lev& l = L(o.side, o.px); l.size += ch; if (ch < 0) l.cPull += -ch; else l.cAdd += ch; o.size = size; }
            return;
        case DEL:
            if (it == ord.end()) return;
            { Lev& l = L(it->second.side, it->second.px); l.size -= it->second.size; l.cPull += it->second.size; } ord.erase(it); return;
        case FILL: {
            const int rs = it != ord.end() ? it->second.side : side;
            last = px; if (hi < 0 || px > hi) hi = px; if (lo < 0 || px < lo) lo = px;
            if (!implied) { if (rs == BID) sv += size; else if (rs == ASK) bv += size; }
            for (int sd = 0; sd < 2; ++sd) for (int dj = 0; dj <= (P.zoneGate ? 2 : 0); ++dj) {   // the first trade at a followed level = its touch ((1.1.0) zone: inside +-1 tick)
                const long long q = px + (dj == 0 ? 0 : dj == 1 ? -1 : 1);
                std::unordered_map<long long, Lev>::iterator lv = lev.find(key(sd, q));
                if (lv == lev.end() || lv->second.ep < 0) continue;
                touch(sd, q, lv->second, ms);
            }
            if (it != ord.end()) { Ord& o = it->second; const long long take = std::min(size, o.size); Lev& l = L(o.side, o.px); l.size -= take; l.cEat += take; o.size -= take; }
            else ++nUnknown;
            return; }
        }
    }
    // close the 1-s boundaries up to ms (the host calls it with its clock minus a grace so a quiet feed still decides)
    void advance(long long ms) {
        if (!started) return;
        int guard = 0;
        while (ms >= nextT) {
            if (++guard > 900) { ++nGap; jump(ms); return; }   // a long gap (feed stopped): forget the followed levels, restart the clock
            boundary();
            nextT += 1000;
        }
    }
    struct Rec { char kind; Touch t; };                   // 'P' touched (live), 'T' 15-s decision, 'D' 30-s log, 'O' outcome
    std::vector<Rec> drain() { std::vector<Rec> r; r.swap(out); return r; }
    const std::vector<Touch>& pending() const { return live; }
    // after a restart: a decided touch read back from the record whose outcome was still open - keep watching it
    void resume(const Touch& t) { if (t.outcome == O_OPEN && t.decided && live.size() < 400) live.push_back(t); }
    double atrAt(long long secEndMs) const { return atrFor(secEndMs); }
    size_t followed() const { return eps.size(); }
    // the book sample for the nightly tier refit: every non-zero level size within R ticks of the last trade (both sides)
    std::vector<long long> sizesNear(long long R) const {
        std::vector<long long> v; if (last <= 0 || R <= 0) return v;
        for (std::unordered_map<long long, Lev>::const_iterator i = lev.begin(); i != lev.end(); ++i) { const long long px = i->first >> 1; if (i->second.size > 0 && px != last && std::llabs(px - last) <= R) v.push_back(i->second.size); }
        return v; }
    long long lastPx() const { return last; }
    // (1.1.0) the zone candidates of the last 1-s boundary (zoneGate), biggest first per side; the medians used
    const std::vector<Zone>& candidates() const { return cand; }
    // (1.2.0, Rassul approved 11:18 CT) THE KING NODES: per side, the biggest zone (price +-1 tick of REAL orders: rested >= 10 min,
    // re-priced < 2x in 5 min) BEYOND the 3-ATR candidate reach but inside today's reachable range [kLo, kHi] (host: today's high - ADR ..
    // today's low + ADR, in ticks), >= 3.0x that side's median real level; the king changes only to a challenger >= 1.25x its size (or when
    // it stops qualifying). king[sd].lots == 0 = none. kingX[sd] = its size / the median. kingSince[sd] = when it became king (ms).
    long long kLo = 0, kHi = 0; Zone king[2]; double kingX[2] = { 0, 0 }; long long kingSince[2] = { 0, 0 };
    static const long long KING_REST_MS = 600000; static constexpr double KING_X = 3.0, KING_SWAP = 1.25;
    double median(int sd) const { return med[sd & 1]; }
    long long realNearCount() const { return nReal; }
    long long levelSize(int sd, long long px) const { std::unordered_map<long long, Lev>::const_iterator i = lev.find(key(sd, px)); return i == lev.end() ? 0 : i->second.size; }
private:
    struct Ord { int side; long long px, size; long long t0 = 0, mv0 = -1, mv1 = -1; };   // (1.1.0) t0 = at this price since; the last two re-price times
    struct Lev { long long size = 0, cAdd = 0, cEat = 0, cPull = 0, qsince = 0; int ep = -1; };
    struct Ep { int side; long long px, t0, sz0, mx, hist60, sz60, lowSince; };
    struct Sec { long long end, last, hi, lo; };
    struct Block { long long idx, h, l, c; };
    std::unordered_map<long long, Ord> ord; std::unordered_map<long long, Lev> lev;
    std::map<int, Ep> eps; int epSeq = 0;
    std::vector<Touch> live; std::vector<Rec> out;
    std::deque<Sec> hist;                                 // 6 h of 1-s last / high / low (tap number)
    std::deque<Block> blocks;                             // 3-min blocks (ATR)
    long long nextT = 0, last = -1, hi = -1, lo = -1, bv = 0, sv = 0, lastSec = -1; bool started = false;
    std::vector<Zone> cand; double med[2] = { 0, 0 }; long long medAt = -1, nReal = 0;   // (1.1.0)
    void zoneStep(long long sl, double atr) {
        cand.clear(); if (sl <= 0 || !(atr > 0)) return;
        const long long R = (long long)(P.reachAtr * atr) + 1;
        std::map<long long, long long> real[2]; nReal = 0;
        for (std::unordered_map<long long, Ord>::const_iterator i = ord.begin(); i != ord.end(); ++i) {
            const Ord& o = i->second; if (o.size <= 0 || (o.side != BID && o.side != ASK)) continue;
            const long long d = o.side == BID ? sl - o.px : o.px - sl; if (d < 0 || d > R + 1) continue;
            if (nextT - o.t0 < P.realMs) continue;                                                     // rested < 30 s
            const int mv = (o.mv0 >= 0 && nextT - o.mv0 <= P.moveWinMs ? 1 : 0) + (o.mv1 >= 0 && nextT - o.mv1 <= P.moveWinMs ? 1 : 0);
            if (mv >= P.maxMoves) continue;                                                            // quoting / re-pricing
            real[o.side][o.px] += o.size; ++nReal;
        }
        if (medAt < 0 || nextT - medAt >= P.medEveryMs || nextT < medAt || !(med[0] > 0) || !(med[1] > 0)) {   // every 60 s (sooner while a side has no real level yet)
            for (int sd = 0; sd < 2; ++sd) med[sd] = sideMedian(real[sd], sd, sl, R);
            medAt = nextT; }
        for (int sd = 0; sd < 2; ++sd) { std::vector<Zone> z = pickZones(real[sd], sd, sl, R, med[sd], P); cand.insert(cand.end(), z.begin(), z.end()); }
        kingStep(sl, R);
        for (const Zone& z : cand) { Lev& l = L(z.side, z.px); if (l.ep >= 0 || eps.size() >= 3000) continue;
            Ep e; e.side = z.side; e.px = z.px; e.t0 = nextT; e.sz0 = l.size; e.mx = l.size; e.hist60 = 0; e.sz60 = l.size; e.lowSince = 0;
            l.ep = ++epSeq; eps[l.ep] = e; ++nEpisodes; }
    }
    void kingStep(long long sl, long long R) {
        std::map<long long, long long> far[2];
        for (std::unordered_map<long long, Ord>::const_iterator i = ord.begin(); i != ord.end(); ++i) {
            const Ord& o = i->second; if (o.size <= 0 || (o.side != BID && o.side != ASK)) continue;
            const long long d = o.side == BID ? sl - o.px : o.px - sl; if (d <= R) continue;               // beyond the candidate reach only
            if (kLo > 0 && o.px < kLo - 1) continue;                                                    // today's reachable range
            if (kHi > 0 && o.px > kHi + 1) continue;
            if (nextT - o.t0 < KING_REST_MS) continue;                                                  // resting >= 10 min
            const int mv = (o.mv0 >= 0 && nextT - o.mv0 <= P.moveWinMs ? 1 : 0) + (o.mv1 >= 0 && nextT - o.mv1 <= P.moveWinMs ? 1 : 0);
            if (mv >= P.maxMoves) continue;
            far[o.side][o.px] += o.size;
        }
        for (int sd = 0; sd < 2; ++sd) {
            auto zoneAt = [&](long long p) { long long z = 0; for (long long q = p - 1; q <= p + 1; ++q) { std::map<long long, long long>::const_iterator f = far[sd].find(q); if (f != far[sd].end()) z += f->second; } return z; };
            Zone best; for (std::map<long long, long long>::const_iterator f = far[sd].begin(); f != far[sd].end(); ++f) {
                if (kLo > 0 && f->first < kLo) continue;
                if (kHi > 0 && f->first > kHi) continue;
                const long long z = zoneAt(f->first); if (z > best.lots || (z == best.lots && z > 0 && f->second > best.raw)) { best.side = sd; best.px = f->first; best.lots = z; best.raw = f->second; } }
            const double m = med[sd]; const bool bestOk = m > 0 && (double)best.lots >= KING_X * m;
            Zone& K = king[sd];
            if (K.lots > 0) {   // the current king: still there and still qualifying?
                const long long dz = sd == BID ? sl - K.px : K.px - sl;
                const long long now = (dz > R && (kLo <= 0 || K.px >= kLo) && (kHi <= 0 || K.px <= kHi)) ? zoneAt(K.px) : 0;
                if (now > 0 && m > 0 && (double)now >= KING_X * m) { K.lots = now;
                    if (bestOk && best.px != K.px && (double)best.lots >= KING_SWAP * (double)now) { K = best; kingSince[sd] = nextT; } }
                else { K = bestOk ? best : Zone(); kingSince[sd] = bestOk ? nextT : 0; }
            } else if (bestOk) { K = best; kingSince[sd] = nextT; }
            kingX[sd] = (K.lots > 0 && m > 0) ? (double)K.lots / m : 0;
        }
    }
    static long long key(int sd, long long px) { return (px << 1) | (long long)(sd & 1); }
    Lev& L(int sd, long long px) { return lev[key(sd, px)]; }
    long long blockIdx(long long secEnd) const {
        if (origin >= 0) return (secEnd - origin) / 1000 / 180;           // the study: second i -> block i // 180 from the series start
        return (secEnd - 1000) / 180000;                                    // live: clock 3-min blocks [hh:mm:00, +3 min)
    }
    double atrFor(long long secEnd) const {
        if (!atrOverride.empty()) { double a = 0; for (size_t i = 0; i < atrOverride.size(); ++i) if (atrOverride[i].first <= secEnd) a = atrOverride[i].second; return a; }
        const long long b = blockIdx(secEnd);
        // true range of each completed block before b (the 20 most recent), previous close from the block before it
        std::vector<double> tr; const Block* prev = nullptr;
        for (size_t i = 0; i < blocks.size(); ++i) {
            const Block& k = blocks[i]; if (k.idx >= b) break;
            const double pc = prev ? (double)prev->c : (double)k.c;
            tr.push_back(std::max((double)(k.h - k.l), std::max(std::fabs((double)k.h - pc), std::fabs((double)k.l - pc)))); prev = &k;
        }
        if (tr.size() >= 10) { double s = 0; const size_t from = tr.size() > 20 ? tr.size() - 20 : 0; for (size_t i = from; i < tr.size(); ++i) s += tr[i]; return std::max(2.0, s / (double)(tr.size() - from)); }
        return hostAtr > 0 ? std::max(2.0, hostAtr) : 0.0;
    }
    void jump(long long ms) {
        for (std::map<int, Ep>::iterator e = eps.begin(); e != eps.end(); ++e) { Lev& l = L(e->second.side, e->second.px); l.ep = -1; l.qsince = 0; }
        eps.clear(); for (std::unordered_map<long long, Lev>::iterator i = lev.begin(); i != lev.end(); ++i) i->second.qsince = 0;
        for (size_t i = 0; i < live.size(); ++i) if (live[i].outcome == O_OPEN) { live[i].outcome = O_EXPIRED; live[i].outMs = ms; if (live[i].decided) out.push_back(Rec{ 'O', live[i] }); }
        live.clear(); hi = lo = -1; bv = sv = 0; nextT = (ms / 1000) * 1000 + 1000;
    }
    void touch(int sd, long long px, Lev& l, long long ms) {
        std::map<int, Ep>::iterator e = eps.find(l.ep); if (e == eps.end()) { l.ep = -1; return; }
        Touch t; t.id = ms; t.side = sd; t.px = px; t.tMs = ms; t.secEnd = nextT; t.t0 = e->second.t0; t.sz0 = e->second.sz0;
        t.szT = l.size; t.szMax = e->second.mx; t.sz60 = e->second.sz60; t.pull60 = l.cPull - e->second.hist60;
        t.sa = l.cAdd; t.se = l.cEat; t.sp = l.cPull; t.lo15 = t.hi15 = 0;
        t.A = atrFor(nextT);
        t.tap = visits(px) + 1;
        {   // log only: the king (biggest level, both sides, within 3 ATR of the last trade) at the touch
            const long long R = (long long)(3.0 * t.A) + 1; long long b1 = 0, b2 = 0, bp = -1; int bs = -1;
            if (last > 0 && t.A > 0) for (int s2 = 0; s2 < 2; ++s2) for (long long j = 1; j <= R; ++j) {
                const long long p = s2 == BID ? last - j : last + j; std::unordered_map<long long, Lev>::const_iterator q = lev.find(key(s2, p));
                const long long z = q == lev.end() ? 0 : (p == px && s2 == sd ? l.size : q->second.size);
                if (z > b1) { b2 = b1; b1 = z; bp = p; bs = s2; } else if (z > b2) b2 = z; }
            if (px == last && l.size > b1) { b2 = b1; b1 = l.size; bp = px; bs = sd; }
            t.king = bp == px && bs == sd; t.kingSz = b1; t.secondSz = b2; t.dom = t.king ? (double)b1 / (double)std::max(b2, 1LL) : 0.0;
        }
        for (size_t i = 0; i < mq.size(); ++i) if (std::fabs((double)(mq[i].px - px)) <= std::max(1.0, 0.1 * t.A)) { t.onMq = true; t.mqName = mq[i].name; break; }
        eps.erase(e); l.ep = -1; l.qsince = 0; ++nTouches;
        if (live.size() < 400) { live.push_back(t); out.push_back(Rec{ 'P', t }); }
    }
    int visits(long long p) const {                        // separate visits to price p in the 6 h before (gap > 60 s = a new visit)
        int n = 0; long long prev = -1;
        for (size_t i = 0; i < hist.size(); ++i) { const Sec& s = hist[i]; if (s.lo <= p && s.hi >= p) { if (prev < 0 || (s.end - prev) / 1000 > 60) ++n; prev = s.end; } }
        return n;
    }
    void boundary() {
        // 1) the second that just ended: last / high / low / aggression
        const long long sl = last, sh = hi >= 0 ? hi : last, sLo = lo >= 0 ? lo : last;
        const long long sbv = bv, ssv = sv; hi = lo = last; if (last < 0) hi = lo = -1; bv = sv = 0;
        if (sl > 0) {
            hist.push_back(Sec{ nextT, sl, sh, sLo }); while (!hist.empty() && hist.front().end <= nextT - 21600000LL) hist.pop_front();
            const long long b = blockIdx(nextT);
            if (blocks.empty() || blocks.back().idx != b) { blocks.push_back(Block{ b, sh, sLo, sl }); while (blocks.size() > 40) blocks.pop_front(); }
            else { Block& k = blocks.back(); k.h = std::max(k.h, sh); k.l = std::min(k.l, sLo); k.c = sl; }
        }
        const double atr = atrFor(nextT);
        const long long tier = P.tier[rthIdx(P.market, nextT)];
        // 2) big levels: 2 ticks .. 3 ATR from the last trade, at the tier for 30 s
        if (P.zoneGate) zoneStep(sl, atr);                            // (1.1.0) the zone candidates (tier = log only)
        else if (sl > 0 && atr > 0) {
            const long long R = (long long)(3.0 * atr) + 1;
            for (int sd = 0; sd < 2; ++sd) for (long long j = 2; j <= R; ++j) {
                const long long p = sd == BID ? sl - j : sl + j; if (p <= 0) break;
                std::unordered_map<long long, Lev>::iterator q = lev.find(key(sd, p));
                if (q == lev.end()) continue;                       // never seen = size 0 (its qsince is 0 by construction)
                Lev& l = q->second; if (l.ep >= 0) continue;
                if (l.size >= tier) {
                    if (l.qsince == 0) l.qsince = nextT;
                    else if (nextT - l.qsince >= 30000 && eps.size() < 3000) {
                        Ep e; e.side = sd; e.px = p; e.t0 = nextT; e.sz0 = l.size; e.mx = l.size; e.hist60 = 0; e.sz60 = l.size; e.lowSince = 0;
                        l.ep = ++epSeq; eps[l.ep] = e; ++nEpisodes;
                    }
                } else l.qsince = 0;
            }
        }
        // 3) followed levels (untouched): max, the minute marks, pulled / drift / timeout
        for (std::map<int, Ep>::iterator it = eps.begin(); it != eps.end();) {
            Ep& e = it->second; Lev& l = L(e.side, e.px); const long long sz = l.size;
            if (sz > e.mx) e.mx = sz;
            const long long dist = e.side == BID ? sl - e.px : e.px - sl;
            if ((nextT - e.t0) % 60000 < 1000) { e.hist60 = l.cPull; e.sz60 = sz; }
            bool end = false;
            if ((double)sz < 0.3 * (double)e.mx) { if (e.lowSince == 0) e.lowSince = nextT; else if (nextT - e.lowSince >= 10000) end = true; }
            else e.lowSince = 0;
            if (!end && sl > 0 && atr > 0 && (double)dist > 6.0 * atr) end = true;
            if (!end && nextT - e.t0 >= 3600000) end = true;
            if (end) { l.ep = -1; l.qsince = 0; it = eps.erase(it); } else ++it;
        }
        // 4) touched levels: the decision windows, the snapshots at +15 / +30 s, the outcome
        for (size_t i = 0; i < live.size();) {
            Touch& t = live[i]; const Lev& l = L(t.side, t.px);
            const long long k = (nextT - t.secEnd) / 1000;       // 0 = the touch's own second
            if (sl > 0 && k >= 0) {
                const long long g = t.side == BID ? ssv : sbv, o = t.side == BID ? sbv : ssv;
                const bool at = std::llabs(sl - t.px) <= 1;
                if (k < 15) { if (at) ++t.dwell15; t.g15 += g; t.opp15 += o; if (k == 0) { t.hi15 = sh; t.lo15 = sLo; } else { t.hi15 = std::max(t.hi15, sh); t.lo15 = std::min(t.lo15, sLo); } }
                if (k < 30) { if (at) ++t.dwell30; t.g30 += g; t.opp30 += o; }
            }
            if (t.a15 < 0 && !t.decided && nextT - t.tMs >= 15000) {
                t.a15 = l.cAdd - t.sa; t.e15 = l.cEat - t.se; t.p15 = l.cPull - t.sp; t.s15 = l.size;
                const double away = std::max(4.0, 0.5 * t.A), brk = std::max(2.0, 0.25 * t.A);
                const bool resolved = t.side == BID ? ((double)t.hi15 >= (double)t.px + away || (double)t.lo15 <= (double)t.px - brk)
                                                    : ((double)t.lo15 <= (double)t.px - away || (double)t.hi15 >= (double)t.px + brk);
                if (resolved || !(t.A > 0)) { t.tag = T_EARLY; t.decided = true; t.vote = 0; } else decide(P, t);
                out.push_back(Rec{ 'T', t });
            }
            if (t.a30 < 0 && nextT - t.tMs >= 30000) {
                t.a30 = l.cAdd - t.sa; t.e30 = l.cEat - t.se; t.p30 = l.cPull - t.sp; t.s30 = l.size;
                const int st = stateOf(t.szT, t.a30, t.e30, t.s30); t.tag30 = st == 1 ? T_STAYS : st == -1 ? T_GONE : T_MID; t.decided30 = true;
                out.push_back(Rec{ 'D', t });
            }
            // outcome (study touch_out from the touch second: through first = BROKE, away first = HELD, nothing in 10 min = EXPIRED)
            if (t.outcome == O_OPEN && sl > 0 && k >= 0 && t.A > 0) {
                const double away = std::max(4.0, 0.5 * t.A), brk = std::max(2.0, 0.25 * t.A);
                const double P0 = (double)t.px;
                if (t.side == BID) { if ((double)sLo <= P0 - brk) t.outcome = O_BROKE; else if ((double)sh >= P0 + away) t.outcome = O_HELD; }
                else { if ((double)sh >= P0 + brk) t.outcome = O_BROKE; else if ((double)sLo <= P0 - away) t.outcome = O_HELD; }
                if (t.outcome == O_OPEN && k >= 15 + 600) t.outcome = O_EXPIRED;
                if (t.outcome != O_OPEN) t.outMs = nextT;
            }
            if (t.outcome != O_OPEN && t.decided && !t.outQueued) { t.outQueued = true; out.push_back(Rec{ 'O', t }); }
            if (t.outQueued && (t.decided30 || t.tag == T_EARLY || nextT - t.tMs >= 30000)) { live.erase(live.begin() + (long)i); continue; }
            ++i;
        }
    }
};
}  // namespace ltc
#endif
