#ifndef LS_LIQTOUCH_H
#define LS_LIQTOUCH_H
// LiqTouch.h 1.3.0 (2026-11-01, Rassul 10:12-10:21 CT "it should be the whole book, no more 3 ATR"): + THE WHOLE-BOOK NODES (bookNodes):
//   real orders = rested >= 10 min at their price and re-priced < 2x in 5 min (the king rule), ANY distance from price (bids below the last
//   trade, offers above); node = a price +-1 tick (same side) at the price with the most lots; each node its own distribution (>= 3 ticks
//   from a bigger node and the smallest raw level between them <= 0.7x the smaller); the biggest BOOK_PER_SIDE per side, biggest first.
//   lsLiquidityProfile 0.70.0 posts them (SignalBoard B code 4); lsDealerProfile 2.40.0 ranks them (top 5 + level-linked >= 50% of the king).
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
static const char* const VERSION = "1.5.0";   // (1.5.0, Rassul 2026-11-02 13:31 CT "it should not be 1 atr, it should be bar close") HELD = a 3-min bar CLOSES beyond the zone on the defenders' side   // was "1.4.0":   // (1.4.0, 2026-11-02) + THE REAL-LIQUIDITY GRADE R S \xACP F (Grade below)   // was "1.3.0";   // (1.3.0, 2026-11-01) + the whole-book nodes (bookNodes)  // was "1.2.0";   // (1.2.0, 2026-10-29) + the KING nodes above / below (kingStep)
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
// (1.3.0) the WHOLE-BOOK nodes of one side: real[p] = real lots at tick p (already filtered); bids at p < last, offers at p > last, no reach.
// Same node rule as pickZones (price +-1 tick, distinct distributions) but no size gate; sparse (only priced ticks are visited), capped.
inline std::vector<Zone> bookZones(const std::map<long long, long long>& real, int sd, long long last, int sepTicks, double sepX, size_t cap) {
    std::vector<Zone> c; if (last <= 0 || real.empty() || cap == 0) return c;
    auto at = [&](long long p) -> long long { std::map<long long, long long>::const_iterator i = real.find(p); return i == real.end() ? 0 : std::max(0LL, i->second); };
    std::map<long long, Zone> byPx;
    for (std::map<long long, long long>::const_iterator i = real.begin(); i != real.end(); ++i) {
        if (i->second <= 0) continue;
        for (long long p = i->first - 1; p <= i->first + 1; ++p) {
            if (p <= 0) continue;
            const long long a = at(p - 1), m = at(p), b = at(p + 1), z = a + m + b; if (z <= 0) continue;
            long long zp = p, zm = m; if (a > zm) { zp = p - 1; zm = a; } if (b > zm) { zp = p + 1; zm = b; }
            const long long dz = sd == BID ? last - zp : zp - last; if (dz < 1) continue;
            std::map<long long, Zone>::iterator h = byPx.find(zp);
            if (h == byPx.end() || z > h->second.lots) { Zone Z; Z.side = sd; Z.px = zp; Z.lots = z; Z.raw = zm; byPx[zp] = Z; }
        }
    }
    std::vector<Zone> all; for (std::map<long long, Zone>::const_iterator i = byPx.begin(); i != byPx.end(); ++i) all.push_back(i->second);
    std::sort(all.begin(), all.end(), [](const Zone& x, const Zone& y) { return x.lots != y.lots ? x.lots > y.lots : x.px < y.px; });
    for (const Zone& z : all) {
        if (c.size() >= cap) break;
        bool ok = true;
        for (const Zone& k : c) {
            const long long lo = std::min(k.px, z.px), hi = std::max(k.px, z.px);
            if (hi - lo < sepTicks) { ok = false; break; }
            // the smallest raw level strictly between them: 0 when any tick there has no real lots (sparse map)
            long long mn = -1, n = 0;
            for (std::map<long long, long long>::const_iterator j = real.upper_bound(lo); j != real.end() && j->first < hi; ++j) { ++n; const long long v = std::max(0LL, j->second); if (mn < 0 || v < mn) mn = v; }
            if (n < hi - lo - 1) mn = 0;
            if ((double)mn > sepX * (double)z.lots) { ok = false; break; }
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

// ------------------------------------------------------------------------------------------------------------- (1.4.0) the grade
// (1.4.0, Rassul 2026-11-02 10:39-10:59 CT: study REAL-LIQUIDITY-2026-11-02 "go", mockup real_liq v4 "lets go with this") THE REAL-LIQUIDITY
// GRADE of a touched level, from the order feed, decided 180 s after the touch and never changed. The zone = the level +- hw ticks
// (the study's medium zone: hw = clamp(round(k x ATR), min, max) per market). Defenders = the touched side's orders in the zone.
//   R  REBUILT     (end - low) / (start - low) of the defenders' size in the zone (1-s samples) >= 0.84; never lost size = passed
//   S  STACKED     the size ADDED to the zone in [touch - 300 s, decision - 30 s] that STAYED 30 s+ (or was filled) / all added >= 0.18
//                  (adds of the last 30 s are left out of both: not judgeable live); no adds = not passed
//   NP NOT PULLED  of the defenders resting at the touch, lots pulled (cancelled / moved out of the zone) by the decision / shown < 0.5
//   F  FADED       the attackers' volume into the defenders in the 2nd 90 s < 0.5 x the 1st 90 s (no attack at all = passed)
// Study (7 markets, 11,929 touches, both halves): 0-1 checks held ~45-58%, 4 checks 83%; NQ flat (graded, not drawn by the readers).
// OUTCOME: (1.5.0, Rassul 2026-11-02 13:31 CT) HELD = the first 3-min bar that CLOSES beyond the zone on the defenders' side (was: price 1 ATR away);
// BROKE = that close first (also between the touch and the decision); EXPIRED = neither in 60 min. Final once decided.
enum GradeBit { G_R = 1, G_S = 2, G_NP = 4, G_F = 8 };
struct ZoneW { double k; int lo, hi; };
inline ZoneW zoneW(const std::string& m) {
    return m == "ES" ? ZoneW{ 0.30, 4, 6 } : m == "NQ" ? ZoneW{ 0.11, 8, 12 } : m == "GC" ? ZoneW{ 0.25, 8, 15 } : m == "HG" ? ZoneW{ 0.30, 2, 5 }
         : m == "CL" ? ZoneW{ 0.25, 4, 8 } : m == "NG" ? ZoneW{ 0.30, 2, 4 } : (m == "EU" || m == "6E") ? ZoneW{ 0.40, 2, 3 } : ZoneW{ 0.30, 2, 6 };
}
inline long long zoneHw(const std::string& m, double atrTicks) { const ZoneW z = zoneW(m); const double a = atrTicks > 0 ? atrTicks : 15.0;
    long long h = (long long)std::llround(z.k * a); if (h < z.lo) h = z.lo; if (h > z.hi) h = z.hi; return h; }
struct Grade {
    int side = BID; long long px = 0, lo = 0, hi = 0, ta = 0, tdc = 0; double A = 0;
    long long shown = 0, pulled = 0, s0 = -1, minS = -1, endS = -1, v1 = 0, v2 = 0, added = 0, quick = 0;
    double res = -1, dur = -1, pull = -1, exh = -1; int mask = 0; bool done = false, lost = false, through = false;   // through = price beyond the far edge at the decision (or a block closed there): the study left these out - shown as broken
    int outcome = O_OPEN; long long outMs = 0, mfe = 0, lastBlk = -1, lastClose = -1; bool brokeEarly = false, heldEarly = false;   // (1.5.0) heldEarly: the held close came before the decision
    std::unordered_map<long long, long long> coh;   // order id -> lots resting in the zone at the touch (the defenders)
    int checks() const { int n = 0; for (int b = 1; b <= 8; b <<= 1) n += (mask & b) ? 1 : 0; return n; }
};
inline std::string gradeLetters(int mask) { std::string o; o += (mask & G_R) ? 'R' : 'r'; o += (mask & G_S) ? 'S' : 's'; o += (mask & G_NP) ? 'P' : 'p'; o += (mask & G_F) ? 'F' : 'f'; return o; }
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
    void reset() { gLive.clear(); gDone.clear(); gOut.clear(); adds.clear(); quick.clear(); cand.clear(); book.clear(); nBookReal = 0; medAt = -1; med[0] = med[1] = 0; ord.clear(); lev.clear(); eps.clear(); live.clear(); out.clear(); hist.clear(); blocks.clear(); nextT = 0; last = hi = lo = -1; bv = sv = 0; lastSec = -1; started = false; }
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
            if (it != ord.end()) { cohortPull(id, it->second.size, px, side); logQuick(it->second.side, it->second.px, it->second.size, it->second.t0, ms); Lev& o = L(it->second.side, it->second.px); o.size -= it->second.size; it->second = Ord{ side, px, size, ms }; L(side, px).size += size; logAdd(side, px, size, ms); return; }
            ord[id] = Ord{ side, px, size, ms }; { Lev& l = L(side, px); l.size += size; l.cAdd += size; } logAdd(side, px, size, ms); return;
        case MOD:
            if (it == ord.end()) { if (side != BID && side != ASK) return; ord[id] = Ord{ side, px, size, ms }; Lev& l = L(side, px); l.size += size; l.cAdd += size; logAdd(side, px, size, ms); return; }
            {   Ord& o = it->second;
                if (px != o.px) { cohortPull(id, o.size, px, o.side); logQuick(o.side, o.px, o.size, o.t0, ms); Lev& a = L(o.side, o.px); a.size -= o.size; a.cPull += o.size; Lev& b = L(o.side, px); b.size += size; b.cAdd += size; o.px = px; o.size = size; o.mv1 = o.mv0; o.mv0 = ms; o.t0 = ms; logAdd(o.side, px, size, ms); return; }
                const long long ch = size - o.size; Lev& l = L(o.side, o.px); l.size += ch; if (ch < 0) { l.cPull += -ch; cohortPull(id, -ch, -1, o.side); logQuick(o.side, o.px, -ch, o.t0, ms); } else { l.cAdd += ch; logAdd(o.side, o.px, ch, ms); } o.size = size; }
            return;
        case DEL:
            if (it == ord.end()) return;
            { cohortPull(id, it->second.size, -1, it->second.side); logQuick(it->second.side, it->second.px, it->second.size, it->second.t0, ms); Lev& l = L(it->second.side, it->second.px); l.size -= it->second.size; l.cPull += it->second.size; } ord.erase(it); return;
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
            if (it != ord.end()) { Ord& o = it->second; const long long take = std::min(size, o.size); Lev& l = L(o.side, o.px); l.size -= take; l.cEat += take; o.size -= take;
                { for (size_t gi = 0; gi < gLive.size(); ++gi) { std::unordered_map<long long, long long>::iterator c = gLive[gi].coh.find(id); if (c != gLive[gi].coh.end()) c->second = std::max(0LL, c->second - take); } }   // filled cohort lots are not pullable
                gradeFill(o.side, o.px, take, ms); }
            else { ++nUnknown; if (!implied && (side == BID || side == ASK)) gradeFill(side, px, size, ms); }
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
    // (1.3.0) the whole-book real nodes (rested >= 10 min, re-priced < 2x in 5 min, any distance), biggest first per side
    const std::vector<Zone>& bookNodes() const { return book; }
    long long bookReal() const { return nBookReal; }
    static const size_t BOOK_PER_SIDE = 20;
    // (1.2.0, Rassul approved 11:18 CT) THE KING NODES: per side, the biggest zone (price +-1 tick of REAL orders: rested >= 10 min,
    // re-priced < 2x in 5 min) BEYOND the 3-ATR candidate reach but inside today's reachable range [kLo, kHi] (host: today's high - ADR ..
    // today's low + ADR, in ticks), >= 3.0x that side's median real level; the king changes only to a challenger >= 1.25x its size (or when
    // it stops qualifying). king[sd].lots == 0 = none. kingX[sd] = its size / the median. kingSince[sd] = when it became king (ms).
    long long kLo = 0, kHi = 0; Zone king[2]; double kingX[2] = { 0, 0 }; long long kingSince[2] = { 0, 0 };
    static const long long KING_REST_MS = 600000; static constexpr double KING_X = 3.0, KING_SWAP = 1.25;
    double median(int sd) const { return med[sd & 1]; }
    long long realNearCount() const { return nReal; }
    long long levelSize(int sd, long long px) const { std::unordered_map<long long, Lev>::const_iterator i = lev.find(key(sd, px)); return i == lev.end() ? 0 : i->second.size; }
    // (1.4.0) the real-liquidity grades: testing (not decided yet) and decided (newest last, the last 6 h, at most 400)
    const std::vector<Grade>& gradesLive() const { return gLive; }
    const std::vector<Grade>& gradesDone() const { return gDone; }
    struct GRec { char kind; Grade g; };                  // 'G' decided, 'H' outcome
    std::vector<GRec> drainGrades() { std::vector<GRec> r; r.swap(gOut); return r; }
    long long nGrades = 0, addLog = 0;
    static const long long GRADE_MS = 180000, STACK_PRE_MS = 300000, DURABLE_MS = 30000, GRADE_OUT_MS = 3600000, ADD_KEEP_MS = 510000, ADD_REACH = 400;
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
    std::vector<Zone> book; long long nBookReal = 0;   // (1.3.0)
    // (1.4.0) the grade: live / decided grades, the add log and the quick-cancel log (adds that left within 30 s), both 8.5 min
    struct AddRec { long long t, px, size; int side; };
    std::vector<Grade> gLive, gDone; std::vector<GRec> gOut; std::deque<AddRec> adds, quick;
    void logAdd(int sd, long long px, long long sz, long long ms) { if (sz <= 0 || (last > 0 && std::llabs(px - last) > ADD_REACH)) return; adds.push_back(AddRec{ ms, px, sz, sd }); ++addLog; }
    void logQuick(int sd, long long px, long long sz, long long addedAt, long long ms) { if (sz <= 0 || addedAt <= 0 || ms - addedAt >= DURABLE_MS || (last > 0 && std::llabs(px - last) > ADD_REACH)) return; quick.push_back(AddRec{ addedAt, px, sz, sd }); }
    // a defender order (cohort) leaves the zone / is cut: the pulled lots of every live grade that holds it
    void cohortPull(long long id, long long lots, long long newPx, int sd) {
        for (size_t i = 0; i < gLive.size(); ++i) { Grade& g = gLive[i]; if (g.done) continue;
            std::unordered_map<long long, long long>::iterator c = g.coh.find(id); if (c == g.coh.end()) continue;
            if (newPx >= g.lo && newPx <= g.hi && sd == g.side) continue;   // moved inside the zone: not pulled
            const long long q = std::min(lots, c->second); if (q <= 0) continue; g.pulled += q; c->second -= q; }
    }
    long long zoneSize(int sd, long long lo_, long long hi_) const { long long s = 0; for (long long p = lo_; p <= hi_; ++p) { std::unordered_map<long long, Lev>::const_iterator i = lev.find(key(sd, p)); if (i != lev.end() && i->second.size > 0) s += i->second.size; } return s; }
    void startGrade(int sd, long long px, long long ms, double A) {
        if (gLive.size() >= 200) return;
        for (const Grade& g : gLive) if (g.side == sd && std::llabs(g.px - px) <= 1) return;   // one grade per zone at a time
        Grade g; g.side = sd; g.px = px; g.A = A; const long long hw = zoneHw(P.market, A); g.lo = px - hw; g.hi = px + hw; g.ta = ms; g.tdc = ms + GRADE_MS;
        for (std::unordered_map<long long, Ord>::const_iterator i = ord.begin(); i != ord.end(); ++i) { const Ord& o = i->second;
            if (o.side == sd && o.size > 0 && o.px >= g.lo && o.px <= g.hi) { g.coh[i->first] = o.size; g.shown += o.size; } }
        g.s0 = g.minS = zoneSize(sd, g.lo, g.hi); ++nGrades; gLive.push_back(g);
    }
    void decideGrade(Grade& g) {
        g.endS = zoneSize(g.side, g.lo, g.hi); if (g.endS < g.minS) g.minS = g.endS;
        g.lost = g.s0 - g.minS > 0;
        g.res = g.lost ? std::max(0.0, (double)(g.endS - g.minS) / (double)(g.s0 - g.minS)) : 1.0;
        const long long from = g.ta - STACK_PRE_MS, to = g.tdc - DURABLE_MS; long long A_ = 0, Q_ = 0;
        for (size_t i = 0; i < adds.size(); ++i) { const AddRec& a = adds[i]; if (a.t < from) continue; if (a.t > to) break; if (a.side == g.side && a.px >= g.lo && a.px <= g.hi) A_ += a.size; }
        for (size_t i = 0; i < quick.size(); ++i) { const AddRec& a = quick[i]; if (a.t >= from && a.t <= to && a.side == g.side && a.px >= g.lo && a.px <= g.hi) Q_ += a.size; }
        g.added = A_; g.quick = std::min(Q_, A_); g.dur = A_ > 0 ? (double)(A_ - g.quick) / (double)A_ : -1;
        g.pull = g.shown > 0 ? (double)g.pulled / (double)g.shown : -1;
        g.exh = g.v1 > 0 ? (double)g.v2 / (double)g.v1 : (g.v2 > 0 ? 99.0 : 0.0);
        g.mask = 0; if (g.res >= 0.84) g.mask |= G_R; if (g.dur >= 0.18) g.mask |= G_S; if (g.pull >= 0 && g.pull < 0.5) g.mask |= G_NP; if (g.exh < 0.5) g.mask |= G_F;
        g.through = g.brokeEarly || (last > 0 && ((g.side == BID && last < g.lo) || (g.side == ASK && last > g.hi)));
        g.done = true; gOut.push_back(GRec{ 'G', g });
    }
    // the grades at a 1-s boundary: depth sample, the decision, the outcome (sh / sLo = the second's high / low, blk = its 3-min block, cl = last)
    void gradeStep(long long sh, long long sLo, long long sl) {
        const long long blk = blockIdx(nextT);
        while (!adds.empty() && adds.front().t < nextT - ADD_KEEP_MS) adds.pop_front();
        while (!quick.empty() && quick.front().t < nextT - ADD_KEEP_MS) quick.pop_front();
        for (size_t i = 0; i < gLive.size();) {
            Grade& g = gLive[i];
            const bool sup = g.side == BID;
            if (sl > 0) {   // a 3-min block closed beyond the far edge?
                if (g.lastBlk >= 0 && blk != g.lastBlk && g.lastClose > 0 && ((sup && g.lastClose < g.lo) || (!sup && g.lastClose > g.hi))) {
                    if (!g.done) { if (!g.heldEarly) g.brokeEarly = true; } else if (g.outcome == O_OPEN) { g.outcome = O_BROKE; g.outMs = nextT; } }
                // (1.5.0, Rassul 13:31 CT) HELD = the first 3-min bar that CLOSES beyond the zone on the defenders' side (bid: above its top,
                // offer: below its bottom) - was: price 1 ATR away. The first of held / broke wins.
                else if (g.lastBlk >= 0 && blk != g.lastBlk && g.lastClose > 0 && ((sup && g.lastClose > g.hi) || (!sup && g.lastClose < g.lo))) {
                    if (!g.done) { if (!g.brokeEarly) g.heldEarly = true; } else if (g.outcome == O_OPEN) { g.outcome = O_HELD; g.outMs = nextT; } }
                g.lastBlk = blk; g.lastClose = sl;
            }
            if (!g.done) {
                const long long z = zoneSize(g.side, g.lo, g.hi); if (z < g.minS) g.minS = z;
                if (nextT >= g.tdc) { decideGrade(g); if (g.brokeEarly) { g.outcome = O_BROKE; g.outMs = nextT; } else if (g.heldEarly) { g.outcome = O_HELD; g.outMs = nextT; } }
            } else if (g.outcome == O_OPEN && sl > 0) {
                const long long ex = sup ? sh - g.px : g.px - sLo; if (ex > g.mfe) g.mfe = ex;
                if (nextT - g.tdc >= GRADE_OUT_MS) { g.outcome = O_EXPIRED; g.outMs = nextT; }
            }
            if (g.done && g.outcome != O_OPEN) {
                gOut.push_back(GRec{ 'H', g }); g.coh.clear();
                gDone.push_back(g); if (gDone.size() > 400) gDone.erase(gDone.begin());
                gLive.erase(gLive.begin() + (long)i); continue; }
            ++i;
        }
        while (!gDone.empty() && nextT - gDone.front().ta > 21600000LL) gDone.erase(gDone.begin());
    }
    void gradeFill(int restSide, long long px, long long size, long long ms) {
        for (size_t i = 0; i < gLive.size(); ++i) { Grade& g = gLive[i]; if (g.done || restSide != g.side || px < g.lo || px > g.hi || ms < g.ta || ms >= g.tdc) continue;
            if (ms - g.ta < GRADE_MS / 2) g.v1 += size; else g.v2 += size; }
    }
    void bookStep(long long sl) {
        book.clear(); nBookReal = 0; if (sl <= 0) return;
        std::map<long long, long long> real[2];
        for (std::unordered_map<long long, Ord>::const_iterator i = ord.begin(); i != ord.end(); ++i) {
            const Ord& o = i->second; if (o.size <= 0 || (o.side != BID && o.side != ASK)) continue;
            const long long d = o.side == BID ? sl - o.px : o.px - sl; if (d < 0) continue;
            if (nextT - o.t0 < KING_REST_MS) continue;                                                  // resting >= 10 min
            const int mv = (o.mv0 >= 0 && nextT - o.mv0 <= P.moveWinMs ? 1 : 0) + (o.mv1 >= 0 && nextT - o.mv1 <= P.moveWinMs ? 1 : 0);
            if (mv >= P.maxMoves) continue;
            real[o.side][o.px] += o.size; ++nBookReal;
        }
        for (int sd = 0; sd < 2; ++sd) { std::vector<Zone> z = bookZones(real[sd], sd, sl, P.sepTicks, P.sepX, BOOK_PER_SIDE); book.insert(book.end(), z.begin(), z.end()); }
    }
    void zoneStep(long long sl, double atr) {
        cand.clear(); bookStep(sl);   // (1.3.0) the whole book needs no ATR
        if (sl <= 0 || !(atr > 0)) return;
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
        std::vector<Zone> follow = cand;   // (1.4.0) the whole-book nodes within reach are followed too (the Liquidity column's nodes get graded)
        for (const Zone& z : book) { const long long dz = z.side == BID ? sl - z.px : z.px - sl; if (dz >= 1 && dz <= R) follow.push_back(z); }
        for (const Zone& z : follow) { Lev& l = L(z.side, z.px); if (l.ep >= 0 || eps.size() >= 3000) continue;
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
        live.clear(); gLive.clear(); adds.clear(); quick.clear(); hi = lo = -1; bv = sv = 0; nextT = (ms / 1000) * 1000 + 1000;
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
        startGrade(sd, px, ms, t.A);   // (1.4.0) the real-liquidity grade starts at the touch
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
        gradeStep(sh, sLo, sl);   // (1.4.0)
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
