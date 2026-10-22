#ifndef LS_QUEUEMETER_H
#define LS_QUEUEMETER_H
// QueueMeter.h 1.0.0 (2026-10-22) -- the QUEUE METER: "Is the queue at the level running out?" (MBO roadmap). SDK-free; tested by
// tests_qm/test_qm.cpp (logic), tests_qm/replay_qm.cpp (NQ 2026-10-09 real full-depth MBO) and the two host suites.
//
// Rassul approved the design 2026-10-22 13:51-13:52 CT (mockup queue-placement.png, panel A) and ordered the build at 15:20 CT:
//   - The LEVELS column doubles as the meter column: the level badge at the tested price fills left to right with the QUEUE LEFT vs the
//     size resting there when the test began (green = bids defending a low, red = offers defending a high), text "G1 Q76%". Warning
//     when the ORIGINAL defenders are gone: "Q33% · orig 0%" with a yellow outline / text. Also at a price with NO key level where
//     defenders show up (a recent pivot, or support that suddenly appears): a small unlabeled badge "Q33%".
//   - TIME TO EMPTY counts PULLED + EATEN and subtracts only refills that STAY (rest >= 1 s). The net-drain version subtracted refills
//     that were pulled again at once and gave 21 s when the truth was ~1 s (NQ 9:42:26).
//   - ORIGINALS = the defender order ids resting in the zone when the test began, each counted once, with a lifetime filter: an order
//     resting < 1 s at the start is re-price flicker (~60% of added size lives < 1 s) and is not an original; pulls after price has left
//     (the clean-up) are not counted.
//   - LIVE like the forming candle; the DECIDED signal is a Q record at a candle close (G = refilled / held, R = drained / broke),
//     append-only, never changed (first line per side + anchor + start candle wins).
//   - Per-market normal "eaten lots/s" by 30-min CT slot from the nightly params file (LRA-QueueNorms-<MKT>.csv), else this session's.
// Who uses it: lsLiquidityProfile (owns the chart's MBO book) runs the Engine and writes lsFlexLevels\LRA-QueueMeter-<MKT>.csv about once
// a second; lsDealerProfile (draws the Levels column, has no order feed) reads that file and draws the meter in its badges.
// No look-ahead: everything is computed from events up to "now"; a refill counts only once it has rested 1 s.
// 1.1.0 (2026-10-22 evening) THE VALIDATION VERDICT (research-artifacts\learning\VALIDATE-DEFENDER-SIZE-2026-10-22.md, out-of-sample on
// NQ / ES / GC / CL): the LIVE call 10 s after contact works, the candle-close Q has no edge (47-54%) -> kept as a record only.
//   - CALL at t0 + CALL_AT_S (10 s), only if the test has not broken / left: RED when queue left < RED_PCT (40%); GREEN when queue left
//     >= GREEN_PCT (100%) and holding (time to empty = holding), only where GREEN_SHOW = 1. Decided once, append-only C line, never repainted.
//   - Per market (LRA-QueueMeter-<MKT>.csv PARAM lines, defaults = the verdict): ES / CL SHOW red + green; NQ / GC SHOW? red only;
//     other markets LOG (not tested: no call drawn).
//   OOS: ES red broke 76% (base 30%), green held 90% (base 70%); CL red 80% (46%), green 74% (54%); NQ red 75% (60%); GC red 73% (53%).
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace qm {
static const char* const VERSION = "1.1.0";   // 1.1.0 (2026-10-22 evening, VALIDATE-DEFENDER-SIZE-2026-10-22.md): the 10-s CALL (red < 40% left / green >= 100% + holding) per market, C lines; the candle-close Q is a record only
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Kind { K_LEVEL = 0, K_PIVOT = 1, K_DEFENDED = 2 };
enum FlowK { F_EAT = 0, F_PULL = 1, F_ADD = 2 };

inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }
inline const char* kindCode(int k) { return k == K_LEVEL ? "L" : k == K_PIVOT ? "P" : k == K_DEFENDED ? "D" : "?"; }
inline int kindOf(const std::string& s) { return s == "L" ? K_LEVEL : s == "P" ? K_PIVOT : s == "D" ? K_DEFENDED : -1; }

struct Params {
    double zoneAtr = 0.12;      // the zone reaches this far from the anchor on the defended side (x ATR) ...
    int innerTicks = 2;         // ... and this many ticks toward price
    double leaveAtr = 0.25;     // HELD: price left the zone this far on the defenders' side
    int breakTicks = 2;         // BROKE: traded this many ticks past the zone's far edge
    double endAtr = 2.0;        // a test is dropped once price is this far away (the display reach is the Dealer Profile's setting)
    long long winMs = 10000;    // rate window
    long long restMs = 1000;    // lifetime filter: a refill counts once it rested 1 s; a pull counts only for an order that rested 1 s
    double suddenX = 2.5;       // a defended price with no level: the zone holds >= 2.5x the typical size per tick there (1 ATR)
    long long lookMs = 120000;  // ... at a new 2-min low (bids) / high (offers)
    int maxWaitBars = 3;        // undecided after 3 closes in contact: decided by the queue left
    double holdPct = 0.5;       // ... >= 50% left and originals not all gone = G
    double riskTte = 10.0, riskPct = 0.4;   // Break risk HIGH: empty within 10 s, or under 40% left, or broke
    int pivotL = 1, pivotR = 2, pivotKeep = 6;
    int atrN = 14;
    int origTicks = 2;          // ORIGINALS = the orders resting within 2 ticks of the defended price (the anchor) when the test began
};

struct Anchor { Tick px = 0; int side = -1; int kind = K_LEVEL; std::string name; };   // side -1 = either (a level), BID = pivot low

// the nightly norms: eaten lots / s per 30-min CT slot and side (0 = none)
struct Norms {
    double slot[48][2];
    bool ok = false; std::string src = "none";
    Norms() { for (int i = 0; i < 48; ++i) slot[i][0] = slot[i][1] = 0; }
};
inline bool numTok(const std::string& s, double& v) {
    if (s.empty() || s.size() > 32) return false;
    char* e = nullptr; errno = 0; const double x = std::strtod(s.c_str(), &e);
    if (!e || *e || errno || !finiteD(x)) return false;
    v = x; return true;
}
inline std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> o; std::string cur;
    for (size_t i = 0; i < s.size(); ++i) { if (s[i] == c) { o.push_back(cur); cur.clear(); } else if (s[i] != '\r') cur += s[i]; }
    o.push_back(cur); return o;
}
// LRA-QueueNorms-<MKT>.csv: "VERSION,1" first, "MARKET,<MKT>", then "SLOT,<HHMM>,<bid eaten lots/s>,<ask eaten lots/s>" (HHMM = slot
// start, CT, on the 30-min grid). Garbled / wrong market = not used (ok false, src says why).
inline Norms parseNorms(const std::vector<std::string>& lines, const std::string& mkt) {
    Norms n; bool ver = false, mk = false; int rows = 0;
    for (size_t i = 0; i < lines.size() && i < 512; ++i) {
        const std::vector<std::string> c = split(lines[i], ',');
        if (c.empty() || c[0].empty()) continue;
        if (!ver) { if (c[0] != "VERSION" || c.size() < 2) { n.src = "garbled (no VERSION first)"; return Norms(); } ver = true; continue; }
        if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != mkt) { Norms z; z.src = "wrong market"; return z; } mk = true; continue; }
        if (c[0] == "SLOT") {
            double hm = 0, b = 0, a = 0;
            if (c.size() < 4 || !numTok(c[1], hm) || !numTok(c[2], b) || !numTok(c[3], a) || b < 0 || a < 0 || b > 1e6 || a > 1e6) { Norms z; z.src = "garbled SLOT row"; return z; }
            const int h = (int)hm / 100, m = (int)hm % 100;
            if (hm != std::floor(hm) || h < 0 || h > 23 || (m != 0 && m != 30)) { Norms z; z.src = "garbled SLOT time"; return z; }
            const int s = h * 2 + m / 30; n.slot[s][BID] = b; n.slot[s][ASK] = a; ++rows;
        }
    }
    if (!ver || !mk || rows == 0) { Norms z; z.src = !ver ? "empty" : !mk ? "no MARKET" : "no SLOT rows"; return z; }
    n.ok = true; n.src = "nightly (" + std::to_string(rows) + " slots)"; return n;
}

// the per-market CALL rule (LRA-QueueMeter-<MKT>.csv: VERSION / MARKET / PARAM,<name>,<value>; unknown names ignored)
enum Mark { M_LOG = 0, M_SHOW = 1, M_SHOWQ = 2 };
struct CallRule { int mark = M_LOG; int callAtS = 10; double redPct = 0.40, greenPct = 1.00; bool greenNeedsHolding = true, greenShow = false; std::string src = "defaults"; };
inline CallRule callRuleFor(const std::string& m) {
    CallRule r;
    if (m == "ES" || m == "CL") { r.mark = M_SHOW; r.greenShow = true; }
    else if (m == "NQ" || m == "GC") { r.mark = M_SHOWQ; r.greenShow = false; }
    r.src = "defaults (the 2026-10-22 verdict)"; return r;
}
inline CallRule parseCallRule(const std::vector<std::string>& lines, const std::string& mkt, bool& ok) {
    CallRule r = callRuleFor(mkt); ok = false; bool ver = false, mk = false; int n = 0;
    for (size_t i = 0; i < lines.size() && i < 256; ++i) {
        std::vector<std::string> c; { std::string cur; for (char ch : lines[i]) { if (ch == ',') { c.push_back(cur); cur.clear(); } else if (ch != '\r') cur += ch; } c.push_back(cur); }
        if (c.empty() || c[0].empty() || c[0][0] == '#') continue;
        if (!ver) { if (c[0] != "VERSION") { r = callRuleFor(mkt); r.src = "garbled (no VERSION first) - defaults"; return r; } ver = true; continue; }
        if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != mkt) { r = callRuleFor(mkt); r.src = "wrong market - defaults"; return r; } mk = true; continue; }
        if (c[0] != "PARAM" || c.size() < 3) continue;
        const std::string& k = c[1]; const std::string& v = c[2]; double d = 0; char* e = nullptr;
        if (k == "MARK") { if (v == "SHOW") r.mark = M_SHOW; else if (v == "SHOW?") r.mark = M_SHOWQ; else if (v == "LOG") r.mark = M_LOG; else { r = callRuleFor(mkt); r.src = "garbled MARK - defaults"; return r; } ++n; continue; }
        errno = 0; d = std::strtod(v.c_str(), &e);
        const bool num = !v.empty() && e && !*e && !errno && d == d;
        if (k == "CALL_AT_S") { if (!num || d < 2 || d > 120) { r = callRuleFor(mkt); r.src = "garbled CALL_AT_S - defaults"; return r; } r.callAtS = (int)d; ++n; }
        else if (k == "RED_PCT") { if (!num || d <= 0 || d >= 1) { r = callRuleFor(mkt); r.src = "garbled RED_PCT - defaults"; return r; } r.redPct = d; ++n; }
        else if (k == "GREEN_PCT") { if (!num || d < 0.5 || d > 5) { r = callRuleFor(mkt); r.src = "garbled GREEN_PCT - defaults"; return r; } r.greenPct = d; ++n; }
        else if (k == "GREEN_NEEDS_HOLDING") { if (!num || (d != 0 && d != 1)) { r = callRuleFor(mkt); r.src = "garbled GREEN_NEEDS_HOLDING - defaults"; return r; } r.greenNeedsHolding = d == 1; ++n; }
        else if (k == "GREEN_SHOW") { if (!num || (d != 0 && d != 1)) { r = callRuleFor(mkt); r.src = "garbled GREEN_SHOW - defaults"; return r; } r.greenShow = d == 1; ++n; }
    }
    if (!ver || !mk) { r = callRuleFor(mkt); r.src = !ver ? "empty - defaults" : "no MARKET - defaults"; return r; }
    ok = true; r.src = "params file (" + std::to_string(n) + " rows)"; return r;
}
// the out-of-sample odds behind each call (the hover)
inline std::string callOdds(const std::string& m, int call) {
    if (call == 0) {
        if (m == "ES") return "ES: queue left under 40% at 10 s broke 76% of the time (base 30%)";
        if (m == "CL") return "CL: queue left under 40% at 10 s broke 80% of the time (base 46%)";
        if (m == "NQ") return "NQ: queue left under 40% at 10 s broke 75% of the time (base 60%) ?";
        if (m == "GC") return "GC: queue left under 40% at 10 s broke 73% of the time (base 53%) ?";
    } else if (call == 1) {
        if (m == "ES") return "ES: 100%+ left and holding at 10 s held 90% of the time (base 70%)";
        if (m == "CL") return "CL: 100%+ left and holding at 10 s held 74% of the time (base 54%)";
    }
    return std::string();
}

// the live numbers of one test (what the badge and its hover show)
struct Live {
    int id = 0, side = BID, kind = K_LEVEL; Tick anchor = 0, lo = 0, hi = 0; std::string name;
    long long t0 = 0, q0 = 0, q = 0; double pct = 0;          // queue left = q / q0
    double origPct = -1;                                      // originals still resting (-1 = no original order known)
    double eat = 0, pull = 0, refill = 0, norm = 0;           // lots / s (norm = this slot's normal eaten lots / s, 0 = unknown)
    double tte = -1;                                          // seconds to empty, -1 = holding (refills that stay >= drain)
    bool high = false, warn = false, contact = false, broke = false, left = false;
    std::string why;                                          // "pulled" / "eaten" / "refilling"
    int decided = -1;                                         // -1 live, 0 = red Q, 1 = green Q (the candle-close RECORD, not drawn)
    int call = -1;                                            // the 10-s call: -1 pending, 0 RED, 1 GREEN, 2 none (no call / too late)
    int mark = M_LOG;                                         // this market's rule: LOG / SHOW / SHOW?
};

class Engine {
public:
    Params P; std::string market; double tick = 0.25;
    struct Ord { int side = BID; Tick px = 0; long long size = 0, tAt = 0; bool filledLast = false; };
    struct Flow { long long t; int k; long long lots, oid; };
    struct Test {
        int id = 0, side = BID, kind = K_LEVEL; Tick anchor = 0, lo = 0, hi = 0; std::string name;
        long long t0 = 0, q0 = 0, tBroke = 0, tLeft = 0, startBar = 0, tDecided = 0; int barsSeen = 0, decided = -1; std::string why;
        std::unordered_map<long long, long long> orig; long long origSum = 0;
        std::deque<Flow> flows; bool broke = false, left = false, contact = true;
        double pctAtDecision = 0, origAtDecision = -1;
        int call = -1; long long tCall = 0;   // (1.1.0) the 10-s call, decided once
        Live atBreak; bool haveBreak = false;   // the numbers the moment it broke (what the Q record keeps for a break)
    };
    std::unordered_map<long long, Ord> book;
    std::map<Tick, long long> lv[2];          // resting lots per price per side
    std::vector<Anchor> levels;               // host: key + gamma levels (this contract, ticks)
    std::vector<Anchor> pivots;               // from closed bars
    std::vector<Test> tests;
    Norms norms;
    double atr = 0;
    long long lastT = 0, lastTradeT = 0; Tick lastPx = 0, prevPx = 0;
    std::deque<std::pair<long long, Tick> > minQ, maxQ;   // monotonic windows of trade prices (new local low / high)
    std::vector<std::string> rows;            // decided Q lines waiting to be appended (and, 1.1.0, the C call lines)
    CallRule rule;                            // (1.1.0) this market's call rule (host sets it from the params file / defaults)
    long long nCalls[3] = { 0, 0, 0 };        // red / green / none
    std::set<std::string> keys;               // decided keys (from the record too): never decided twice
    long long session = -1; int nextId = 1; long long lastSuddenT[2] = { 0, 0 };
    // this session's eaten lots and active seconds per slot / side (normal fallback)
    long long eatLots[48][2], eatSecs[48][2], lastEatSec[2] = { -1, -1 };
    size_t bookCap = 400000;
    struct Bar { long long endMs; double h, l, c; };
    std::deque<Bar> bars;
    long long lastBarMs = 0;
    long long nUnknownFill = 0, nEvents = 0, nStarted = 0;

    Engine() { resetNorms(); }
    void resetNorms() { for (int i = 0; i < 48; ++i) eatLots[i][0] = eatLots[i][1] = eatSecs[i][0] = eatSecs[i][1] = 0; lastEatSec[0] = lastEatSec[1] = -1; }
    static int slotOf(long long tMs) { long long s = (tMs / 1000) % 86400; if (s < 0) s += 86400; return (int)(s / 1800); }
    static long long sessionOf(long long tMs) { return (tMs / 1000 + 7 * 3600) / 86400; }   // 17:00 CT starts a session
    Tick zoneW() const { const double w = atr > 0 && tick > 0 ? P.zoneAtr * atr / tick : 0; return std::max<Tick>(2, (Tick)std::llround(w)); }
    Tick ticksOf(double atrMult) const { return atr > 0 && tick > 0 ? std::max<Tick>(1, (Tick)std::llround(atrMult * atr / tick)) : 1; }

    void setLevels(const std::vector<Anchor>& L) { levels = L; }
    long long zoneSum(int side, Tick lo, Tick hi) const {
        long long s = 0; for (std::map<Tick, long long>::const_iterator it = lv[side].lower_bound(lo); it != lv[side].end() && it->first <= hi; ++it) s += it->second; return s;
    }
    double normal(int side, long long tMs) const {
        const int s = slotOf(tMs);
        if (norms.ok && norms.slot[s][side] > 0) return norms.slot[s][side];
        if (eatSecs[s][side] >= 30) return (double)eatLots[s][side] / (double)eatSecs[s][side];
        long long L = 0, S = 0; for (int i = 0; i < 48; ++i) { L += eatLots[i][side]; S += eatSecs[i][side]; }
        return S >= 30 ? (double)L / (double)S : 0;
    }
    std::string normSrc(int side, long long tMs) const {
        const int s = slotOf(tMs);
        if (norms.ok && norms.slot[s][side] > 0) return "nightly";
        if (eatSecs[s][side] >= 30) return "this session, this half hour";
        long long S = 0; for (int i = 0; i < 48; ++i) S += eatSecs[i][side];
        return S >= 30 ? "this session" : "learning";
    }

    // ---- the order feed. side is used only on NEW (and on a MOD of an order seen for the first time); a FILL row's side is ignored.
    void onEvent(int act, int side, Tick px, long long size, long long oid, long long t) {
        if (t <= 0 || oid == 0) return;
        ++nEvents;
        if (t > lastT) lastT = t;
        if (!tests.empty()) stepCalls(t);   // (1.1.0) before this event: the call sees the book as it was at its moment
        const long long ses = sessionOf(t);
        if (session != ses) { if (session >= 0) { tests.clear(); pivots.clear(); minQ.clear(); maxQ.clear(); } session = ses; resetNorms(); }   // candles kept (ATR)
        std::unordered_map<long long, Ord>::iterator it = book.find(oid);
        if (act == FILL) {
            if (it == book.end()) { ++nUnknownFill; if (px > 0) trade(px, t); return; }
            if (size <= 0) return;
            const Tick fp = px > 0 ? px : it->second.px;
            trade(fp, t);                              // the price first: a test starting on this fill counts this order in q0
            it = book.find(oid); if (it == book.end()) return;
            Ord& o = it->second; const long long q = std::min(size, o.size);
            flow(o.side, o.px, F_EAT, size, oid, t);
            eaten(o.side, size, t);
            adj(o.side, o.px, -q); o.size -= q; o.filledLast = true;
            return;
        }
        if (act == NEW) {
            if (px <= 0 || size <= 0 || (side != BID && side != ASK)) return;
            if (it != book.end()) { remove(it, t, false); }
            if (book.size() >= bookCap) prune();
            Ord o; o.side = side; o.px = px; o.size = size; o.tAt = t; book[oid] = o; adj(side, px, size);
            flow(side, px, F_ADD, size, oid, t);
            return;
        }
        if (act == MOD) {
            if (px <= 0) return;
            if (it == book.end()) {   // rested before the feed opened: its age is unknown (counted from now)
                if (size <= 0 || (side != BID && side != ASK)) return;
                if (book.size() >= bookCap) prune();
                Ord o; o.side = side; o.px = px; o.size = size; o.tAt = t; book[oid] = o; adj(side, px, size); return;
            }
            Ord& o = it->second;
            if (px != o.px) {
                if (o.size > 0 && t - o.tAt >= P.restMs) flow(o.side, o.px, F_PULL, o.size, oid, t);
                adj(o.side, o.px, -o.size);
                o.px = px; o.size = std::max(0LL, size); o.tAt = t; o.filledLast = false;
                adj(o.side, o.px, o.size);
                if (o.size > 0) flow(o.side, o.px, F_ADD, o.size, oid, t);
            } else {
                const long long d = size - o.size;
                if (d > 0) flow(o.side, o.px, F_ADD, d, oid, t);
                else if (d < 0 && t - o.tAt >= P.restMs) flow(o.side, o.px, F_PULL, -d, oid, t);
                adj(o.side, o.px, d); o.size = std::max(0LL, size); o.filledLast = false;
            }
            return;
        }
        if (act == DEL) { if (it != book.end()) remove(it, t, true); }
    }

    // (1.1.0) the 10-s CALL: decided once at the first moment >= t0 + CALL_AT_S (called on every event and by the host each pass)
    void stepCalls(long long now) {
        for (size_t i = 0; i < tests.size(); ++i) {
            Test& T = tests[i];
            if (T.call >= 0 || now < T.t0 + (long long)rule.callAtS * 1000) continue;
            T.tCall = now; int c = 2;
            const Live v = snapshot(T, now);
            if (!T.broke && !T.left) {
                if (v.pct < rule.redPct) c = 0;
                else if (rule.greenShow && v.pct >= rule.greenPct && (!rule.greenNeedsHolding || v.tte < 0)) c = 1;
            }
            T.call = c; ++nCalls[c];
            char b[256];
            std::snprintf(b, sizeof(b), "C|%lld|%lld|%s|%lld|%s|%d|%d|%.1f|%s|%s", T.t0, now, T.side == BID ? "B" : "A", (long long)T.anchor, c == 0 ? "R" : c == 1 ? "G" : "N",
                          (int)std::lround(v.pct * 100), v.origPct < 0 ? -1 : (int)std::lround(v.origPct * 100), v.tte < 0 ? -1.0 : std::min(v.tte, 9999.0), kindCode(T.kind), VERSION);
            rows.push_back(b);
        }
    }
    // a trade price (a FILL, or the host's last price when there is no feed row). Starts / updates tests.
    void trade(Tick p, long long t) {
        if (p <= 0) return;
        prevPx = lastPx; lastPx = p; lastTradeT = t; if (t > lastT) lastT = t;
        const bool newLow = !minQ.empty() && p < minQ.front().second, newHigh = !maxQ.empty() && p > maxQ.front().second;
        while (!minQ.empty() && minQ.back().second >= p) minQ.pop_back();
        minQ.push_back(std::make_pair(t, p));
        while (!maxQ.empty() && maxQ.back().second <= p) maxQ.pop_back();
        maxQ.push_back(std::make_pair(t, p));
        while (!minQ.empty() && minQ.front().first < t - P.lookMs) minQ.pop_front();
        while (!maxQ.empty() && maxQ.front().first < t - P.lookMs) maxQ.pop_front();
        if (!(atr > 0)) return;
        const Tick lea = ticksOf(P.leaveAtr), endT = ticksOf(P.endAtr);
        for (size_t i = 0; i < tests.size(); ++i) {
            Test& T = tests[i];
            if (T.side == BID) {
                if (!T.broke && p < T.lo - P.breakTicks) { T.broke = true; T.tBroke = t; T.atBreak = snapshot(T, t); T.haveBreak = true; }
                if (!T.left && !T.broke && p > T.hi + lea) { T.left = true; T.tLeft = t; }
                T.contact = p >= T.lo - P.breakTicks && p <= T.hi + zoneW();
            } else {
                if (!T.broke && p > T.hi + P.breakTicks) { T.broke = true; T.tBroke = t; T.atBreak = snapshot(T, t); T.haveBreak = true; }
                if (!T.left && !T.broke && p < T.lo - lea) { T.left = true; T.tLeft = t; }
                T.contact = p <= T.hi + P.breakTicks && p >= T.lo - zoneW();
            }
        }
        // drop decided tests far away / long done
        for (size_t i = tests.size(); i-- > 0;) {
            const Test& T = tests[i];
            const Tick d = T.side == BID ? (p > T.hi ? p - T.hi : (p < T.lo ? T.lo - p : 0)) : (p < T.lo ? T.lo - p : (p > T.hi ? p - T.hi : 0));
            if (T.decided >= 0 && (d > endT || t - T.tDecided > 1800000LL)) tests.erase(tests.begin() + (long)i);
        }
        if (prevPx <= 0) return;
        // new tests: levels, then pivots (price entering the zone from the price side), then a defended price at a new low / high
        const Tick w = zoneW();
        for (int pass = 0; pass < 2; ++pass) {
            const std::vector<Anchor>& A = pass == 0 ? levels : pivots;
            for (size_t k = 0; k < A.size(); ++k) {
                const Anchor& a = A[k];
                for (int sd = 0; sd < 2; ++sd) {
                    if (a.side >= 0 && a.side != sd) continue;
                    const Tick lo = sd == BID ? a.px - w : a.px - P.innerTicks, hi = sd == BID ? a.px + P.innerTicks : a.px + w;
                    const bool enter = sd == BID ? (prevPx > hi && p <= hi && p >= lo - P.breakTicks) : (prevPx < lo && p >= lo && p <= hi + P.breakTicks);
                    if (enter) start(sd, a.kind, a.px, lo, hi, a.name, t);
                }
            }
        }
        for (int sd = 0; sd < 2; ++sd) {
            const bool fresh = sd == BID ? newLow : newHigh;
            if (!fresh || t - lastSuddenT[sd] < 1000) continue;
            if (covered(sd, p)) continue;
            lastSuddenT[sd] = t;
            const Tick lo = sd == BID ? p - w : p - P.innerTicks, hi = sd == BID ? p + P.innerTicks : p + w;
            const long long q = zoneSum(sd, lo, hi);
            // typical resting lots per tick on this side within 1 ATR of price (the median of the non-empty ticks)
            const Tick r = ticksOf(1.0);
            std::vector<long long> per;
            for (std::map<Tick, long long>::const_iterator it = lv[sd].lower_bound(sd == BID ? p - r : p); it != lv[sd].end() && it->first <= (sd == BID ? p : p + r); ++it) if (it->second > 0) per.push_back(it->second);
            if (per.size() < 5) continue;
            std::nth_element(per.begin(), per.begin() + (long)(per.size() / 2), per.end());
            const double med = (double)per[per.size() / 2];
            const double nTicks = (double)(hi - lo + 1);
            if ((double)q >= P.suddenX * med * nTicks) start(sd, K_DEFENDED, p, lo, hi, "", t);
        }
    }

    // a closed candle (CT end time ms): ATR, pivots, and the Q decisions. Returns the number of new Q rows.
    int closeBar(long long endMs, double h, double l, double c) {
        if (endMs <= lastBarMs || !finiteD(h) || !finiteD(l) || !finiteD(c) || h < l) return 0;
        const long long ses = sessionOf(endMs - 1);
        if (session >= 0 && ses != session) { /* the feed has not reached the new session yet: keep */ }
        lastBarMs = endMs;
        Bar b; b.endMs = endMs; b.h = h; b.l = l; b.c = c; bars.push_back(b); while (bars.size() > 80) bars.pop_front();
        // ATR (the mean true range of the last atrN closed candles)
        if (bars.size() >= 6) {
            double s = 0; int n = 0;
            for (size_t i = bars.size() - 1; i >= 1 && n < P.atrN; --i, ++n) { const Bar& x = bars[i]; const Bar& pv = bars[i - 1]; s += std::max(x.h, pv.c) - std::min(x.l, pv.c); }
            if (n > 0) atr = s / n;
        }
        // pivots: candle i with a lower low (higher high) than the pivotL before and the pivotR after it
        const int L = P.pivotL, R = P.pivotR, n = (int)bars.size();
        if (n >= L + R + 1 && tick > 0) {
            const int i = n - 1 - R; const Bar& x = bars[(size_t)i];
            bool lowP = true, highP = true;
            for (int j = i - L; j <= i + R; ++j) { if (j == i) continue; if (!(x.l < bars[(size_t)j].l)) lowP = false; if (!(x.h > bars[(size_t)j].h)) highP = false; }
            if (lowP) addPivot(BID, x.l);
            if (highP) addPivot(ASK, x.h);
        }
        // a pivot price closed through is no longer defended
        for (size_t k = pivots.size(); k-- > 0;) {
            const Tick px = pivots[k].px; const Tick ct = tick > 0 ? (Tick)std::llround(c / tick) : 0;
            if ((pivots[k].side == BID && ct < px - P.breakTicks) || (pivots[k].side == ASK && ct > px + P.breakTicks)) pivots.erase(pivots.begin() + (long)k);
        }
        int made = 0;
        for (size_t i = 0; i < tests.size(); ++i) {
            Test& T = tests[i];
            if (T.decided >= 0 || T.t0 >= endMs) continue;
            if (T.startBar == 0) T.startBar = endMs;
            int g = -1; std::string why;
            if (T.broke && T.tBroke < endMs) { g = 0; why = "broke"; }
            else if (T.left && T.tLeft < endMs) { g = 1; why = "held"; }
            else if (++T.barsSeen >= P.maxWaitBars) {
                const Live v = snapshot(T, endMs);
                if (v.pct >= P.holdPct && !v.warn) { g = 1; why = "refilled"; } else { g = 0; why = "drained"; }
            }
            if (g < 0) continue;
            const Live v = g == 0 && why == "broke" && T.haveBreak ? T.atBreak : snapshot(T, endMs);
            T.decided = g; T.why = why; T.tDecided = endMs; T.pctAtDecision = v.pct; T.origAtDecision = v.origPct;
            const std::string k = key(T.side, T.anchor, T.startBar);
            if (keys.count(k)) continue;
            keys.insert(k);
            rows.push_back(recordLine(T, endMs, v));
            ++made;
        }
        return made;
    }

    std::vector<Live> live(long long now) const {
        std::vector<Live> out;
        for (size_t i = 0; i < tests.size(); ++i) out.push_back(snapshot(tests[i], now));
        return out;
    }
    Live snapshot(const Test& T, long long now) const {
        Live v; v.id = T.id; v.side = T.side; v.kind = T.kind; v.anchor = T.anchor; v.lo = T.lo; v.hi = T.hi; v.name = T.name;
        v.t0 = T.t0; v.q0 = T.q0; v.q = zoneSum(T.side, T.lo, T.hi); v.pct = T.q0 > 0 ? (double)v.q / (double)T.q0 : 0;
        v.contact = T.contact; v.broke = T.broke; v.left = T.left; v.decided = T.decided; v.call = T.call; v.mark = rule.mark;
        if (T.origSum > 0) {
            long long left = 0;
            for (std::unordered_map<long long, long long>::const_iterator o = T.orig.begin(); o != T.orig.end(); ++o) {
                std::unordered_map<long long, Ord>::const_iterator b = book.find(o->first);
                if (b == book.end() || b->second.side != T.side || b->second.px < T.lo || b->second.px > T.hi) continue;
                left += std::min(b->second.size, o->second);
            }
            v.origPct = (double)left / (double)T.origSum;
        }
        const long long span = std::max(1000LL, std::min(P.winMs, now - T.t0));
        long long e = 0, pu = 0, rf = 0;
        for (std::deque<Flow>::const_iterator f = T.flows.begin(); f != T.flows.end(); ++f) {
            if (f->t < now - span || f->t > now) continue;
            if (f->k == F_EAT) e += f->lots;
            else if (f->k == F_PULL) pu += f->lots;
            else if (now - f->t >= P.restMs) {     // a refill that STAYED: still resting at this price, rested >= 1 s
                std::unordered_map<long long, Ord>::const_iterator b = book.find(f->oid);
                if (b != book.end() && b->second.side == T.side && b->second.px >= T.lo && b->second.px <= T.hi && b->second.tAt <= f->t)
                    rf += std::min(f->lots, b->second.size);
            }
        }
        const double s = (double)span / 1000.0;
        v.eat = (double)e / s; v.pull = (double)pu / s; v.refill = (double)rf / s; v.norm = normal(T.side, now);
        const double net = v.eat + v.pull - v.refill;
        v.tte = net > 0.05 ? (double)v.q / net : -1;
        v.why = (v.refill > 0 && v.refill >= v.eat + v.pull) ? "refilling" : (v.pull >= v.eat ? "pulled" : "eaten");
        if (v.eat + v.pull + v.refill <= 0) v.why = "quiet";
        v.warn = T.origSum > 0 && v.origPct <= 0.005;
        v.high = T.broke || (v.tte >= 0 && v.tte <= P.riskTte) || v.pct < P.riskPct;
        return v;
    }

    static std::string key(int side, Tick anchor, long long startBar) { return std::string(side == BID ? "B" : "A") + "|" + std::to_string(anchor) + "|" + std::to_string(startBar); }
    static std::string header() { return "Q|t0_ms|decided_ms|side|anchor_tick|G/R|why|pct|orig_pct|q0|kind|name|start_bar_ms  (record only) ; C|t0_ms|call_ms|side|anchor_tick|R/G/N|pct|orig_pct|tte_s|kind|ver  (the 10-s call)"; }
    std::string recordLine(const Test& T, long long endMs, const Live& v) const {
        char b[256];
        std::snprintf(b, sizeof(b), "Q|%lld|%lld|%s|%lld|%s|%s|%d|%d|%lld|%s|%s|%lld", T.t0, endMs, T.side == BID ? "B" : "A", (long long)T.anchor,
                      T.decided == 1 ? "G" : "R", T.why.c_str(), (int)std::lround(v.pct * 100), v.origPct < 0 ? -1 : (int)std::lround(v.origPct * 100),
                      T.q0, kindCode(T.kind), clean(T.name).c_str(), T.startBar);
        return b;
    }
    static std::string clean(const std::string& s) { std::string o; for (size_t i = 0; i < s.size() && o.size() < 24; ++i) { const char c = s[i]; if (c != '|' && c != ',' && c != '\n' && c != '\r') o += c; } return o; }
    // the record read back (restart): every decided key is known, so nothing is decided twice. Torn / garbled lines are skipped.
    struct QRec { long long t0 = 0, decided = 0, q0 = 0, startBar = 0; int side = BID, g = 0, pct = 0, orig = -1, kind = 0; Tick anchor = 0; std::string why, name; };
    static bool parseRec(const std::string& ln, QRec& r) {
        const std::vector<std::string> c = split(ln, '|');
        if (c.size() != 13 || c[0] != "Q") return false;
        double t0, dd, an, pc, og, q0, sb;
        if (!numTok(c[1], t0) || !numTok(c[2], dd) || !numTok(c[4], an) || !numTok(c[7], pc) || !numTok(c[8], og) || !numTok(c[9], q0) || !numTok(c[12], sb)) return false;
        if ((c[3] != "B" && c[3] != "A") || (c[5] != "G" && c[5] != "R") || kindOf(c[10]) < 0 || t0 <= 0 || dd <= 0 || sb <= 0 || an <= 0 || q0 < 0) return false;
        r.t0 = (long long)t0; r.decided = (long long)dd; r.side = c[3] == "B" ? BID : ASK; r.anchor = (Tick)an; r.g = c[5] == "G" ? 1 : 0; r.why = c[6];
        r.pct = (int)pc; r.orig = (int)og; r.q0 = (long long)q0; r.kind = kindOf(c[10]); r.name = c[11]; r.startBar = (long long)sb; return true;
    }
    int adopt(const std::vector<std::string>& lines) {   // returns the lines adopted
        int n = 0; for (size_t i = 0; i < lines.size(); ++i) { QRec r; if (!parseRec(lines[i], r)) continue; keys.insert(key(r.side, r.anchor, r.startBar)); ++n; }
        // a live test whose key is recorded is already decided (restart in the middle of a test)
        for (size_t i = 0; i < tests.size(); ++i) if (tests[i].startBar > 0 && keys.count(key(tests[i].side, tests[i].anchor, tests[i].startBar)) && tests[i].decided < 0) tests[i].decided = 2;
        return n;
    }

private:
    void adj(int side, Tick px, long long d) {
        if (d == 0 || (side != BID && side != ASK)) return;
        std::map<Tick, long long>::iterator it = lv[side].find(px);
        if (it == lv[side].end()) { if (d > 0) lv[side][px] = d; return; }
        it->second += d; if (it->second <= 0) lv[side].erase(it);
    }
    void remove(std::unordered_map<long long, Ord>::iterator it, long long t, bool cancel) {
        Ord& o = it->second;
        if (cancel && o.size > 0 && t - o.tAt >= P.restMs) flow(o.side, o.px, F_PULL, o.size, it->first, t);
        adj(o.side, o.px, -o.size);
        book.erase(it);
    }
    void prune() {   // the oldest half of the far-away orders (bounded memory; MBO books stay well under the cap in practice)
        std::vector<long long> drop;
        for (std::unordered_map<long long, Ord>::iterator it = book.begin(); it != book.end(); ++it) {
            const Tick d = it->second.px > lastPx ? it->second.px - lastPx : lastPx - it->second.px;
            if (lastPx > 0 && d > ticksOf(3.0)) drop.push_back(it->first);
        }
        for (size_t i = 0; i < drop.size(); ++i) { std::unordered_map<long long, Ord>::iterator it = book.find(drop[i]); if (it != book.end()) { adj(it->second.side, it->second.px, -it->second.size); book.erase(it); } }
    }
    void eaten(int side, long long lots, long long t) {
        const int s = slotOf(t); eatLots[s][side] += lots;
        const long long sec = t / 1000; if (sec != lastEatSec[side]) { lastEatSec[side] = sec; ++eatSecs[s][side]; }
    }
    void flow(int side, Tick px, int k, long long lots, long long oid, long long t) {
        if (lots <= 0) return;
        for (size_t i = 0; i < tests.size(); ++i) {
            Test& T = tests[i];
            if (T.side != side || px < T.lo || px > T.hi) continue;
            if (k != F_EAT && !T.contact) continue;            // the clean-up after price left is not a pull / refill
            T.flows.push_back(Flow{ t, k, lots, oid });
            while (!T.flows.empty() && T.flows.front().t < t - P.winMs - P.restMs) T.flows.pop_front();
            if (T.flows.size() > 20000) T.flows.pop_front();
        }
    }
    bool covered(int side, Tick p) const {
        for (size_t i = 0; i < tests.size(); ++i) if (tests[i].side == side && tests[i].decided < 0 && p >= tests[i].lo - P.breakTicks && p <= tests[i].hi + P.breakTicks) return true;
        return false;
    }
    void addPivot(int side, double px) {
        if (!(tick > 0) || !finiteD(px)) return;
        Anchor a; a.px = (Tick)std::llround(px / tick); a.side = side; a.kind = K_PIVOT;
        for (size_t i = 0; i < pivots.size(); ++i) if (pivots[i].side == side && pivots[i].px == a.px) return;
        pivots.push_back(a);
        int n = 0; for (size_t i = pivots.size(); i-- > 0;) if (pivots[i].side == side && ++n > P.pivotKeep) pivots.erase(pivots.begin() + (long)i);
    }
    void start(int side, int kind, Tick anchor, Tick lo, Tick hi, const std::string& name, long long t) {
        // one meter per place: an undecided test of this side over the price wins; a decided test of the same anchor is replaced
        for (size_t i = tests.size(); i-- > 0;) {
            const Test& T = tests[i];
            if (T.anchor == anchor && T.side != side && T.decided < 0) return;   // a level is support OR resistance while its test is live
            if (T.side != side) continue;
            const bool overlap = lo <= T.hi && hi >= T.lo;
            if (!overlap) continue;
            if (T.decided < 0) return;
            tests.erase(tests.begin() + (long)i);
        }
        Test T; T.id = nextId++; T.side = side; T.kind = kind; T.anchor = anchor; T.lo = lo; T.hi = hi; T.name = name; T.t0 = t;
        T.q0 = zoneSum(side, lo, hi);
        if (T.q0 <= 0) return;
        for (std::unordered_map<long long, Ord>::const_iterator it = book.begin(); it != book.end(); ++it) {
            const Ord& o = it->second;
            if (o.side != side || o.px < lo || o.px > hi || o.size <= 0 || t - o.tAt < P.restMs) continue;   // < 1 s = flicker, not an original
            if (P.origTicks >= 0 && (o.px < anchor - P.origTicks || o.px > anchor + P.origTicks)) continue;   // at the defended price
            T.orig[it->first] = o.size; T.origSum += o.size;
        }
        T.contact = true; ++nStarted;
        tests.push_back(T);
    }
};

// ---- the live file lsLiquidityProfile writes and lsDealerProfile reads: lsFlexLevels\LRA-QueueMeter-<MKT>.csv
// VERSION,<qm version> / MARKET,<MKT> / ASOF,<ct ms> / TICK,<tick> / LAST,<price> / ATR,<points> /
// T,<B|A>,<anchor px>,<lo px>,<hi px>,<kind L|P|D>,<name>,<q0>,<q>,<pct%>,<orig% | -1>,<eat>,<norm>,<pull>,<refill>,<tte s | -1>,
//   <H|L>,<why>,<warn 0|1>,<contact 0|1>,<decided -1|0|1>,<t0 ms>,<call -1 pending|0 red|1 green|2 none>,<mark 0 LOG|1 SHOW|2 SHOW?>
// (1.1.0: the live file is lsFlexLevels\LRA-QueueLive-<MKT>.csv - LRA-QueueMeter-<MKT>.csv is the nightly PARAMS file; 22-field rows of
// 1.0.0 are still read, with no call)
struct FileT { int side = BID, kind = K_LEVEL; double anchor = 0, lo = 0, hi = 0; std::string name; long long q0 = 0, q = 0, t0 = 0;
               double pct = 0, origPct = -1, eat = 0, norm = 0, pull = 0, refill = 0, tte = -1; bool high = false, warn = false, contact = false; int decided = -1; std::string why;
               int call = -1, mark = M_LOG; };
struct File { bool ok = false; std::string version, market, why = "not read"; long long asof = 0; double tick = 0, last = 0, atr = 0; std::vector<FileT> t; };

inline std::string fmtPx(double v, double tick) {
    int dec = 0; if (tick > 0) { double t = tick; while (dec < 6 && std::fabs(t - std::floor(t + 1e-9)) > 1e-9) { t *= 10; ++dec; } }
    char b[48]; std::snprintf(b, sizeof(b), "%.*f", dec, v); return b;
}
inline std::string fileText(const Engine& E, long long now, double lastPx) {
    std::string o = std::string("VERSION,") + VERSION + "\nMARKET," + E.market + "\nASOF," + std::to_string(now) + "\nTICK," + fmtPx(E.tick, 0.000001) +
                    "\nLAST," + fmtPx(lastPx, E.tick) + "\nATR," + fmtPx(E.atr, 0.01) + "\n";
    const std::vector<Live> L = E.live(now);
    for (size_t i = 0; i < L.size(); ++i) {
        const Live& v = L[i]; char b[512];
        std::snprintf(b, sizeof(b), "T,%s,%s,%s,%s,%s,%s,%lld,%lld,%d,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%s,%s,%d,%d,%d,%lld,%d,%d\n", v.side == BID ? "B" : "A",
                      fmtPx(v.anchor * E.tick, E.tick).c_str(), fmtPx(v.lo * E.tick, E.tick).c_str(), fmtPx(v.hi * E.tick, E.tick).c_str(), kindCode(v.kind), Engine::clean(v.name).c_str(),
                      v.q0, v.q, (int)std::lround(v.pct * 100), v.origPct < 0 ? -1 : (int)std::lround(v.origPct * 100), v.eat, v.norm, v.pull, v.refill, v.tte < 0 ? -1.0 : std::min(v.tte, 9999.0),
                      v.high ? "H" : "L", v.why.c_str(), v.warn ? 1 : 0, v.contact ? 1 : 0, v.decided > 1 ? -1 : v.decided, v.t0, v.call, v.mark);
        o += b;
    }
    return o;
}
inline File parseFile(const std::vector<std::string>& lines, const std::string& mkt) {
    File f; bool ver = false;
    for (size_t i = 0; i < lines.size() && i < 200; ++i) {
        const std::vector<std::string> c = split(lines[i], ',');
        if (c.empty() || c[0].empty()) continue;
        if (!ver) { if (c[0] != "VERSION" || c.size() < 2) { f.why = "garbled (no VERSION first)"; return f; } f.version = c[1]; ver = true; continue; }
        double v = 0;
        if (c[0] == "MARKET") { if (c.size() < 2 || c[1] != mkt) { File z; z.why = "wrong market"; return z; } f.market = c[1]; continue; }
        if (c[0] == "ASOF" || c[0] == "TICK" || c[0] == "LAST" || c[0] == "ATR") {
            if (c.size() < 2 || !numTok(c[1], v) || v < 0) { File z; z.why = "garbled " + c[0]; return z; }
            if (c[0] == "ASOF") f.asof = (long long)v; else if (c[0] == "TICK") f.tick = v; else if (c[0] == "LAST") f.last = v; else f.atr = v;
            continue;
        }
        if (c[0] == "T") {
            if (c.size() != 22 && c.size() != 24) { File z; z.why = "garbled T row"; return z; }
            FileT t; double n[16]; const int idx[16] = { 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 18, 19, 20, 21 };
            for (int k = 0; k < 16; ++k) if (!numTok(c[(size_t)idx[k]], n[k])) { File z; z.why = "garbled T number"; return z; }
            if ((c[1] != "B" && c[1] != "A") || kindOf(c[5]) < 0 || (c[16] != "H" && c[16] != "L") || n[0] <= 0 || n[1] <= 0 || n[2] < n[1] || n[3] < 0 || n[4] < 0) { File z; z.why = "garbled T field"; return z; }
            t.side = c[1] == "B" ? BID : ASK; t.anchor = n[0]; t.lo = n[1]; t.hi = n[2]; t.kind = kindOf(c[5]); t.name = c[6]; t.q0 = (long long)n[3]; t.q = (long long)n[4];
            t.pct = n[5] / 100.0; t.origPct = n[6] < 0 ? -1 : n[6] / 100.0; t.eat = n[7]; t.norm = n[8]; t.pull = n[9]; t.refill = n[10]; t.tte = n[11];
            t.high = c[16] == "H"; t.why = c[17]; t.warn = n[12] != 0; t.contact = n[13] != 0; t.decided = (int)n[14]; t.t0 = (long long)n[15];
            if (c.size() == 24) { double cc = 0, mm = 0; if (!numTok(c[22], cc) || !numTok(c[23], mm) || cc < -1 || cc > 2 || mm < 0 || mm > 2) { File z; z.why = "garbled T call"; return z; } t.call = (int)cc; t.mark = (int)mm; }
            f.t.push_back(t);
        }
    }
    if (!ver || f.market.empty()) { File z; z.why = !ver ? "empty" : "no MARKET"; return z; }
    f.ok = true; f.why = "ok"; return f;
}

// ---- the badge text, fitted: "G1 Q76%" / "Q33% . orig 0%" / "Q33%" (IRT text is ASCII: the approved middle dot is drawn as " . ", as the merged level names)
inline std::string badgeText(const std::string& name, const FileT& t, int variant) {
    // variant 0 = full, 1 = no " orig 0%", 2 = just Q
    const int p = (int)std::lround(std::max(0.0, std::min(9.99, t.pct)) * 100);
    std::string q = "Q" + std::to_string(p) + "%";
    std::string s = name.empty() ? q : name + " " + q;
    if (variant == 0 && t.warn) s += " . orig 0%";
    if (variant == 2) s = q;
    if (t.mark == M_SHOWQ && (t.call == 0 || t.call == 1)) s += "?";   // (1.1.0) the call is "?" in NQ / GC
    return s;
}
inline double fillFrac(const FileT& t) { return std::max(0.0, std::min(1.0, t.pct)); }
// (1.1.0) the badge outline follows the CALL: 0 red, 1 green, -1 none (neutral) - only where the market's rule draws it
inline int outlineCall(const FileT& t) { return t.mark != M_LOG && (t.call == 0 || t.call == 1) ? t.call : -1; }

// ---- the hover: question / what it is / why it matters / how to use / example, then the live numbers
struct Hover { std::string title; std::vector<std::string> lines; };
inline std::string f1(double v) { char b[32]; std::snprintf(b, sizeof(b), "%.1f", v); return b; }
inline Hover hover(const FileT& t, const std::string& levelName, const std::string& pxTxt, const std::string& market = std::string()) {
    Hover h; const bool bid = t.side == BID;
    const bool proven = t.mark == M_SHOW;
    h.title = std::string("Queue meter") + (proven ? "" : " ?") + " - " + (levelName.empty() ? std::string(bid ? "bids at " : "offers at ") + pxTxt : levelName + " " + pxTxt) + (proven ? "" : t.mark == M_SHOWQ ? " (red call only, not fully proven)" : " (not proven yet)");
    char b[200];
    std::snprintf(b, sizeof(b), "Queue left: %lld of %lld lots (%d%%)", t.q, t.q0, (int)std::lround(t.pct * 100)); h.lines.push_back(b);
    h.lines.push_back(t.origPct < 0 ? std::string("Originals left: not known (no order rested 1 s+ when the test began)")
                                    : "Originals left: " + std::to_string((int)std::lround(t.origPct * 100)) + "% of the lots resting when the test began");
    h.lines.push_back("Eaten: " + f1(t.eat) + " lots/s" + (t.norm > 0 ? " (" + f1(t.eat / t.norm) + "x normal " + f1(t.norm) + "/s)" : " (normal: learning)"));
    h.lines.push_back("Pulled: " + f1(t.pull) + " lots/s");
    h.lines.push_back("Refill that stayed: " + f1(t.refill) + " lots/s");
    h.lines.push_back(t.tte < 0 ? std::string("Time to empty: holding (refills that stay keep up)") : "Time to empty: about " + f1(t.tte) + " s (pulled + eaten - refills that stay)");
    h.lines.push_back(std::string("Break risk: ") + (t.high ? "HIGH" : "LOW") + " - defenders " + (t.why == "refilling" ? "refilling" : t.why == "pulled" ? "pulled" : t.why == "eaten" ? "eaten" : "quiet"));
    if (t.call == 0 || t.call == 1) h.lines.push_back(std::string("10-s call: ") + (t.call == 0 ? "RED - expect the break" : "GREEN - defenders holding") + (t.mark == M_SHOWQ ? " ?" : ""));
    else if (t.call == -1) h.lines.push_back("10-s call: pending (made 10 s after price first traded into the zone)");
    else h.lines.push_back("10-s call: none (neither under 40% nor 100%+ and holding, or price already broke / left)");
    if ((t.call == 0 || t.call == 1) && !callOdds(market, t.call).empty()) h.lines.push_back(callOdds(market, t.call));
    if (t.decided >= 0) h.lines.push_back(std::string("Candle-close Q (a record for the nightly, not a forecast): ") + (t.decided == 1 ? "held / refilled" : "drained / broke"));
    h.lines.push_back(std::string("Question: is the queue ") + (bid ? "of bids defending this low" : "of offers defending this high") + " running out?");
    h.lines.push_back("What it is: the lots resting in the zone now vs when price first traded into it, from every order in the book (MBO).");
    h.lines.push_back("Why it matters: a level breaks when its queue empties - pulled orders break it faster than eaten ones.");
    h.lines.push_back("How to use: act on the 10-s call - a RED outline (under 40% left 10 s after contact) = expect the break; a GREEN outline (100%+ left and refilling) = defenders holding. The candle-close Q is only a record.");
    h.lines.push_back("Example: NQ Oct 9 9:42:26 AM - Q33%, orig 0%, 21 lots / (5.5 eaten + 14.5 pulled) = about 1 s to empty; it broke 2 s later.");
    return h;
}
} // namespace qm
#endif
