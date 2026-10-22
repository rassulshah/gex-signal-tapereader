#ifndef LS_DEFENSE_TURN_H
#define LS_DEFENSE_TURN_H
// DefenseTurn.h 1.2.0 (14:19 CT: the close beyond the R line) (2026-10-20, Rassul 12:22 CT "first build the lines, do it now") - SDK-free logic shared by lsBookFlow (decides,
// records) and lsDefenseLines (draws on the price chart). Approved design (11:37-12:22 CT, defense-refill-level-pane-mockup.png):
//   TURN = his pivot rule (1-bar ZigZag pivotOk, DeltaProfileCore.h): the candle that makes the new high / low, or the NEXT candle when it
//          trades back into that pivot candle's range; + the DEFENDING side's Defense bar (Refill Effort = added - pulled near price) is
//          bigger than the attacking side's (offers red at a high, bids green at a low); + the candle CLOSES in the defending colour (red at a
//          high, green at a low). Aggression is NOT required (Rassul 12:19 "the green close would indicate that absorption occurred").
//   "?" while the candle forms: pivot rule + the defending Defense bar bigger so far. Confirmed at the close, else gone (forming candle only).
//   LINE price: within 2 points (8 ticks) of the pivot extreme, the price where the defenders re-posted the most lots after a hit
//          (pivot candle .. turn candle); none -> the extreme itself.
//   Tags: E = the attack DIED at the extreme: after the pivot's high / low printed, the attackers hit at half or less of their pace
//          before it (12:25 CT "like when you indicated attackers died"; attackDied below). A = absorption: the attackers out-hit the
//          defenders on this candle and still lost the close.
// Test 2026-10-20 (TURN-TEST.md, 7 markets, 4,075 pivot candles, +1.5R before beyond the extreme): no edge over a plain defending close yet,
// so every mark is "?" until the nightly proves it (his show-only-what-works rule). No look-ahead: only candles up to the turn candle.
#include <string>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <cerrno>
#include <limits>

namespace dt {
static const char* const VERSION = "1.8.0";   // (1.8.0, 2026-10-21 19:48 CT Rassul) no "?" on the badge (badgeSegs / badgeSegs7); the badge hover per letter (hoverA / H / Tr / G / W / Move / Q, statusText, hoverText) built from the records; tipSpot places it inside the pane, never over the info box;   // (1.7.0, 2026-10-21 Rassul approved 14:20-14:22 CT) G = the ATTACKERS giving up (giveUpDecide: their own resting orders pulled >= 2x normal after the extreme, net of moves, more pulled than added, attack died; no G on an initiative candle; G lines), W = the DEFENDERS withdrew at a key level (LevelWatch: touch + 60 s, net pulled >= 2x traded; supports drawn; W lines), Tr hover text (the Delta Profile's TR record), badgeSegs7 (A, H, Tr, G, W, P/B, L/X - at most 4 letters, the rest in the hover), attackersText / atLevelText for the box rows;   // (1.6.0, 2026-10-21 14:16-14:18 CT Rassul "i want you to draw them" - B (stepping back) and both offer cases, mockup v4) badgeSegs: every move letter drawn (P / B per BOOK side) in the colour of the side that moved (bids green = LOW colour, offers red = HIGH colour), each letter its own colour in one combined badge;   // (1.5.0, 2026-10-21 Rassul approved defender3-mockup-v3 11:50 CT "lets do it" + 11:59 both sides) Top Defender #3: badgeText5 adds P (the DEFENDERS PRESSING IN: their resting orders moved toward price, MoveLogic.h) - the only move case drawn; the other three cases recorded as M lines;   // (1.4.0, 2026-10-21 Rassul approved defender4-mockup-v1 + L / X 11:32 CT) Top Defender #4: the RE-TEST of the refill level (retestStep): price moved >= 0.5 ATR away, then came back into the zone within 15 min; the defenders' RESTING lots in the zone at the close BEFORE it came back vs the market normal -> L (reloaded) / X (empty) / N; Q lines in the turns file; badge "A|H|L"; box row "Re-test";   // (1.3.0, 2026-10-21 Rassul approved defender5-mockup-v2 11:25 CT "build this") H = the defenders HIT BACK after an A (counterDecide), the H line in the turns file, the pane badge text "A|H" (badgeText), the Counter row text;   // (1.1.0, 2026-10-20 13:54 CT Rassul "okay build", defense-E-R-mockup.png) the gray EXHAUSTION line (attack died AT the price, touch rule) on pivots that have an R; T lines carry the pivot candle; lines at most 3 candles from the candle they refer to (yellow dot)
// the market's median slowdown at a fresh-price touch ((lots/s 20-60 s) / (lots/s 0-20 s + 0.05)), Databento MBO Oct 5 / 7 / 9 (wall4.py)
inline double adMedianOf(const std::string& m) {
    return m == "ES" ? 0.095 : m == "NQ" ? 0.029 : m == "CL" ? 0.057 : m == "GC" ? 0.0 : m == "HG" ? 0.0 : m == "NG" ? 0.032 : (m == "EU" || m == "6E") ? 0.067 : 0.05;
}
static const int NEAR_TICKS = 8;                 // 2 points in ES: how far from the extreme the refill price may be

struct Candle {                                  // one candle (prices in ticks)
    long long t = 0; bool ok = false;
    long long o = 0, h = 0, l = 0, c = 0;
    long long net[2] = {0, 0};                   // Defense bars: [0] bids (green), [1] offers (red) = added - pulled near price
    long long aggr[2] = {0, 0};                  // Aggression lots by the side HIT: [0] = sellers hitting bids, [1] = buyers lifting offers
    long long refPx[2] = {0, 0}, refLots[2] = {0, 0};   // the price with the most re-posted lots after a hit, per resting side
    // the attack at the extreme (order feed): [LOW] = sellers hitting bids around the candle's low, [HIGH] = buyers lifting offers at its high
    long long before[2] = {0, 0}, secsBefore[2] = {0, 0}, after[2] = {0, 0}, secsAfter[2] = {0, 0};   // lots hit up to / after the extreme printed
    long long hit[2] = {0, 0}, span = 0;          // lots hit per resting side in the whole candle; seconds from the first to the last fill
    // (1.7.0, Top Attacker #2 G) [LOW / HIGH extreme]: the ATTACKERS' own resting orders near price AFTER the extreme printed - at a high the
    // buyers' bids, at a low the sellers' offers: lots pulled, lots added, lots of their confirmed moves (MoveLogic; booked as pull + add)
    long long apull[2] = {0, 0}, aadd[2] = {0, 0}, amov[2] = {0, 0}; bool gOk[2] = {false, false};
};
enum { LOW = 0, HIGH = 1 };                      // side of the turn: LOW = bids defend a low (bullish), HIGH = offers defend a high

struct Turn {
    long long t = 0;                             // the turn candle (its key)
    long long pivotT = 0;                        // the pivot candle (dedupe: one turn per pivot)
    int side = -1;
    long long pivotPx = 0, linePx = 0;           // ticks
    long long defNet = 0, attNet = 0;            // the two Defense bars
    int marginPct = 0;                           // how much bigger the defending Defense bar is (capped 999)
    bool E = false, A = false;
    bool live = false;                           // the forming candle's "?"
    long long attAggr = 0;                       // (1.3.0) the attackers' aggressive lots on the turn candle (the attack H is measured against)
};

// the pivot rule for candle i (no look-ahead): LOW / HIGH candidate and the pivot candle's index, or false
inline bool pivotOf(const std::vector<Candle>& c, size_t i, int side, size_t& piv) {
    if (i < 2 || i >= c.size() || !c[i].ok || !c[i - 1].ok || !c[i - 2].ok) return false;
    const Candle &a = c[i - 2], &b = c[i - 1], &x = c[i];
    if (side == HIGH) {
        if (x.h > b.h) { piv = i; return true; }                                       // the candle making the new high
        if (b.h > a.h && x.h <= b.h && x.h > b.l) { piv = i - 1; return true; }        // the next one, trading back into it
    } else {
        if (x.l < b.l) { piv = i; return true; }
        if (b.l < a.l && x.l >= b.l && x.l < b.h) { piv = i - 1; return true; }
    }
    return false;
}
inline int marginOf(long long def, long long att) {
    if (att <= 0) return 999;                                                          // the other side pulled more than it added
    const double m = ((double)def / (double)att - 1.0) * 100.0;
    return m >= 999.0 ? 999 : (int)std::floor(m + 0.5);
}
// the refill price near the pivot (defending side), from the pivot candle to the turn candle; 0 = none
inline long long refillNear(const std::vector<Candle>& c, size_t piv, size_t i, int side, long long ext) {
    const int s = side == HIGH ? 1 : 0;
    long long best = 0, lots = 0;
    for (size_t k = piv; k <= i && k < c.size(); ++k) {
        const Candle& q = c[k];
        if (!q.ok || q.refLots[s] <= 0 || q.refPx[s] <= 0) continue;
        if (std::llabs(q.refPx[s] - ext) > NEAR_TICKS) continue;
        if (q.refLots[s] > lots) { lots = q.refLots[s]; best = q.refPx[s]; }
    }
    return best;
}
// E (12:24-12:25 CT "E = attackers gave up ... like when you indicated attackers died"): after the pivot candle printed its extreme, the
// attackers' pace (lots per second hitting the defending side) fell to HALF or less of their pace before it - through the rest of the pivot
// candle plus the turn candle when that is the next one. Needs >= 10 s after the extreme and a real attack before it.
inline bool attackDied(const Candle& p, const Candle& x, bool nextCandle, int side) {
    const int s = side == HIGH ? 1 : 0;                                                // the resting side being hit
    if (p.secsBefore[side] <= 0 || p.before[side] <= 0) return false;
    long long lots = p.after[side], secs = p.secsAfter[side];
    if (nextCandle) { lots += x.hit[s]; secs += x.span > 0 ? x.span : 0; }
    if (secs < 10) return false;
    return (double)lots / (double)secs <= 0.5 * (double)p.before[side] / (double)p.secsBefore[side];
}
// decide candle i (closed: live = false; forming: live = true, the close is not required). `used` = pivot keys already turned (per side).
template <class Used>
inline bool decide(const std::vector<Candle>& c, size_t i, int side, bool live, const Used& used, Turn& out) {
    size_t piv = 0;
    if (!pivotOf(c, i, side, piv)) return false;
    if (used(side, c[piv].t)) return false;                                            // one turn per pivot
    const Candle& x = c[i];
    const int ds = side == HIGH ? 1 : 0, as = 1 - ds;                                  // defending / attacking side of the book
    if (!(x.net[ds] > x.net[as]) || x.net[ds] <= 0) return false;                      // the defending Defense bar is bigger (and arriving)
    if (!live && !(side == HIGH ? x.c < x.o : x.c > x.o)) return false;               // the close in the defending colour
    Turn t; t.t = x.t; t.pivotT = c[piv].t; t.side = side; t.live = live;
    t.pivotPx = side == HIGH ? c[piv].h : c[piv].l;
    const long long ext = side == HIGH ? x.h : x.l;
    const long long rp = refillNear(c, piv, i, side, t.pivotPx);
    t.linePx = rp > 0 ? rp : (side == HIGH ? std::max(t.pivotPx, ext) : std::min(t.pivotPx, ext));
    // (1.2.0, Rassul 14:19 CT "the close of the candle has to be above the refill line"): the close beyond the R line (above at a low, below
    // at a high; on the line = no). Forming candle: the price now must be beyond it, so the "?" goes when price is back through the line.
    if (!(side == HIGH ? x.c < t.linePx : x.c > t.linePx)) return false;
    t.defNet = x.net[ds]; t.attNet = x.net[as]; t.marginPct = marginOf(x.net[ds], x.net[as]);
    // attackers: buyers lifting offers at a high (aggr[1]), sellers hitting bids at a low (aggr[0])
    const long long att = x.aggr[side == HIGH ? 1 : 0], def = x.aggr[side == HIGH ? 0 : 1];
    t.E = attackDied(c[piv], x, piv != i, side);
    t.A = att > def; t.attAggr = att;
    out = t; return true;
}
static const bool SHOW_E = false;   // (1.0.1, Rassul 13:37 CT "remove the exhaustion indicators for now"): E is still decided and recorded (nightly), not drawn
inline std::string tags(const Turn& t) { std::string s = "R"; if (SHOW_E && t.E) s += " E"; if (t.A) s += " A"; return s + " ?"; }
inline std::string marginText(int m) { return m >= 999 ? std::string("999%") : std::to_string(m) + "%"; }   // (1.2.0, Rassul 14:28 "get rid of the + sign before the percents")

// ---- the turns file (written by lsBookFlow, read by lsDefenseLines) --------------------------------------------------------------------
// one line per decided turn: T|bar_key|side|pivot_px|line_px|margin|E|A|def_net|att_net|tick|pivot_key  (prices as doubles, tick = tick size;
// pivot_key since 1.1.0 - older 10-field lines read pivot_key = 0)
// (1.1.0) one line per exhaustion decided for a turn: X|turn_bar_key|side|price|tick|bar_key_where_it_traded
// live file: L|bar_key|side|pivot_px|line_px|margin|E|A|def_net|att_net|tick  or "N" (no "?" now)
struct Line { char kind = 'T'; long long t = 0; int side = -1; double pivotPx = 0, linePx = 0, tick = 0; int margin = 0; bool E = false, A = false; long long defNet = 0, attNet = 0, pivotT = 0; };
inline std::string lineOf(char kind, const Turn& t, double tick) {
    std::ostringstream o; o.precision(12);
    o << kind << '|' << t.t << '|' << t.side << '|' << (double)t.pivotPx * tick << '|' << (double)t.linePx * tick << '|' << t.marginPct << '|'
      << (t.E ? 1 : 0) << '|' << (t.A ? 1 : 0) << '|' << t.defNet << '|' << t.attNet << '|' << tick << '|' << t.pivotT;
    return o.str();
}
inline bool decimalToken(const std::string& s, bool decimalPoint) {
    if (s.empty()) return false;
    size_t k = s[0] == '-' ? 1 : 0;             // lineOf emits a minus only for negative net values
    if (k == s.size()) return false;
    bool digits = false;
    while (k < s.size() && s[k] >= '0' && s[k] <= '9') { digits = true; ++k; }
    if (decimalPoint && k < s.size() && s[k] == '.') {
        ++k;
        while (k < s.size() && s[k] >= '0' && s[k] <= '9') { digits = true; ++k; }
    }
    if (!digits) return false;
    if (decimalPoint && k < s.size() && (s[k] == 'e' || s[k] == 'E')) {
        ++k;
        if (k < s.size() && (s[k] == '+' || s[k] == '-')) ++k;
        const size_t exponent = k;
        while (k < s.size() && s[k] >= '0' && s[k] <= '9') ++k;
        if (k == exponent) return false;
    }
    return k == s.size();
}
inline bool parseLongLong(const std::string& s, long long& v) {
    if (!decimalToken(s, false)) return false;
    char* e = nullptr; errno = 0;
    const long long n = std::strtoll(s.c_str(), &e, 10);
    if (errno == ERANGE || e == s.c_str() || *e) return false;
    v = n; return true;
}
inline bool parseInt(const std::string& s, int& v) {
    long long n = 0;
    if (!parseLongLong(s, n) || n < (long long)std::numeric_limits<int>::min() || n > (long long)std::numeric_limits<int>::max()) return false;
    v = (int)n; return true;
}
inline bool parseFiniteDouble(const std::string& s, double& v) {
    if (!decimalToken(s, true)) return false;
    char* e = nullptr; errno = 0;
    const double n = std::strtod(s.c_str(), &e);
    if (errno == ERANGE || e == s.c_str() || *e || !std::isfinite(n)) return false;
    v = n; return true;
}
inline bool parseLine(const std::string& s, Line& L) {
    if (s.size() < 3 || (s[0] != 'T' && s[0] != 'L') || s[1] != '|') return false;
    std::vector<std::string> f; std::string cur;
    const size_t end = s.back() == '\r' ? s.size() - 1 : s.size();
    for (size_t k = 2; k < end; ++k) { const char ch = s[k]; if (ch == '|') { f.push_back(cur); cur.clear(); } else cur += ch; }
    f.push_back(cur);
    if (f.size() != 10 && f.size() != 11) return false;
    Line out; out.kind = s[0];
    if (!parseLongLong(f[0], out.t) || out.t <= 0) return false;
    if (!parseInt(f[1], out.side) || (out.side != LOW && out.side != HIGH)) return false;
    if (!parseFiniteDouble(f[2], out.pivotPx) || !(out.pivotPx > 0)) return false;
    if (!parseFiniteDouble(f[3], out.linePx) || !(out.linePx > 0)) return false;
    if (!parseInt(f[4], out.margin) || out.margin < 0) return false;
    if ((f[5] != "0" && f[5] != "1") || (f[6] != "0" && f[6] != "1")) return false;
    out.E = f[5] == "1"; out.A = f[6] == "1";
    if (!parseLongLong(f[7], out.defNet) || !parseLongLong(f[8], out.attNet)) return false;
    if (!parseFiniteDouble(f[9], out.tick) || !(out.tick > 0)) return false;
    if (f.size() == 11 && (!parseLongLong(f[10], out.pivotT) || out.pivotT < 0)) return false;
    L = out;
    return true;
}
// (1.1.0) the exhaustion line of a turn
struct XLine { long long t = 0, at = 0; int side = -1; double px = 0, tick = 0; };
inline std::string xLineOf(long long turnT, int side, long long pxTicks, double tick, long long atKey) {
    std::ostringstream o; o.precision(12); o << "X|" << turnT << '|' << side << '|' << (double)pxTicks * tick << '|' << tick << '|' << atKey; return o.str();
}
inline bool parseX(const std::string& s, XLine& X) {
    if (s.size() < 3 || s[0] != 'X' || s[1] != '|') return false;
    std::vector<std::string> f; std::string cur;
    const size_t end = s.back() == '\r' ? s.size() - 1 : s.size();
    for (size_t k = 2; k < end; ++k) { const char ch = s[k]; if (ch == '|') { f.push_back(cur); cur.clear(); } else cur += ch; }
    f.push_back(cur);
    if (f.size() != 5) return false;
    XLine out;
    if (!parseLongLong(f[0], out.t) || out.t <= 0) return false;
    if (!parseInt(f[1], out.side) || (out.side != LOW && out.side != HIGH)) return false;
    if (!parseFiniteDouble(f[2], out.px) || !(out.px > 0)) return false;
    if (!parseFiniteDouble(f[3], out.tick) || !(out.tick > 0)) return false;
    if (!parseLongLong(f[4], out.at) || out.at <= 0) return false;
    X = out;
    return true;
}
// (1.1.0) the exhaustion price of a turn from the finished touches: the defending side's touches on the pivot candle or the turn candle,
// within NEAR_TICKS of the pivot extreme, where the attack died; the one closest to the extreme. false = none.
struct TouchV { int s = 0; long long px = 0, key = 0; bool ad = false; };
inline bool exhaustionOf(const std::vector<TouchV>& T, int side, long long pivotT, long long turnT, long long pivotPx, long long& px, long long& atKey) {
    const int rs = side == HIGH ? 1 : 0;                                  // the resting side the attackers hit (offers at a high)
    bool found = false;
    for (size_t k = 0; k < T.size(); ++k) {
        const TouchV& t = T[k];
        if (!t.ad || t.s != rs || (t.key != pivotT && t.key != turnT) || std::llabs(t.px - pivotPx) > NEAR_TICKS) continue;
        if (!found || (side == HIGH ? t.px > px : t.px < px)) { px = t.px; atKey = t.key; found = true; }
    }
    return found;
}
inline std::string tagsOf(const Line& L) { std::string s = "R"; if (SHOW_E && L.E) s += " E"; if (L.A) s += " A"; return s + " ?"; }

// ---- (1.3.0) H: did the defenders HIT BACK after the absorption (A)? -----------------------------------------------------------------
// MBO roadmap Top Defender #5 (Rassul 2026-10-21, defender5-mockup-v2.png). Research + test: /home/claude/pt/d5/Defender5-findings.md -
// 7 markets Sep 24 - Oct 9, 1,813 Defense A turns: H held 54% vs absorbed-only 49% (control 51%), costs ~0.5 ATR of waiting, GC / NQ / HG
// best, CL / ES / EU / NG no difference -> a READ of the defenders ("are they defending?"), shown with "?" until the nightly proves it.
// Rule: in the COUNTER_BARS candles after the A candle, the defenders' aggressive lots (buyers lifting offers at a low, sellers hitting bids
// at a high) >= COUNTER_X x the attackers' lots on the A candle, AND the last of them closes >= COUNTER_K ATR away from the pivot extreme,
// with no close beyond the extreme in between (that ends it: absorbed only). Decided once, at the close of the last window candle.
static const int COUNTER_BARS = 2;
static const double COUNTER_X = 1.0, COUNTER_K = 0.25;
struct Counter { long long t = 0; int side = -1; bool H = false; long long lots = 0, attack = 0, at = 0; };
// 0 = wait for more candles, 1 = decided (out), -1 = not applicable (no A / no attack), -2 = a window candle has no order feed (no decision)
inline int counterDecide(const Turn& t, const std::vector<Candle>& after, double atrTicks, Counter& out) {
    if (!t.A || t.attAggr <= 0 || t.side < 0) return -1;
    Counter c; c.t = t.t; c.side = t.side; c.attack = t.attAggr;
    const int dAggr = t.side == HIGH ? 0 : 1;                                          // aggr index of the defenders' aggression
    for (size_t k = 0; k < after.size() && (int)k < COUNTER_BARS; ++k) {
        const Candle& q = after[k];
        if (!q.ok) return -2;
        c.lots += q.aggr[dAggr];
        if (t.side == HIGH ? q.c > t.pivotPx : q.c < t.pivotPx) { c.H = false; c.at = q.t; out = c; return 1; }   // closed beyond: absorbed only
        if ((int)k == COUNTER_BARS - 1) {
            const double away = (double)(t.side == HIGH ? t.pivotPx - q.c : q.c - t.pivotPx);
            c.H = (double)c.lots >= COUNTER_X * (double)c.attack && atrTicks > 0 && away >= COUNTER_K * atrTicks;
            c.at = q.t; out = c; return 1;
        }
    }
    return 0;
}
// the true-range average of candles (ticks), the last `n` of them
inline double atrOf(const std::vector<Candle>& c, int n = 14) {
    double s = 0; int k = 0;
    for (size_t i = c.size(); i-- > 1 && k < n;) { if (!c[i].ok || !c[i - 1].ok) continue;
        const double tr = (double)std::max(c[i].h - c[i].l, std::max(std::llabs(c[i].h - c[i - 1].c), std::llabs(c[i].l - c[i - 1].c))); s += tr; ++k; }
    return k ? s / k : 0.0;
}
// the H line in the turns file: H|turn_key|side|H(0/1)|lots|attack|decided_at_key
inline std::string hLineOf(const Counter& c) { std::ostringstream o; o << "H|" << c.t << '|' << c.side << '|' << (c.H ? 1 : 0) << '|' << c.lots << '|' << c.attack << '|' << c.at; return o.str(); }
inline bool parseH(const std::string& s, Counter& C) {
    if (s.size() < 3 || s[0] != 'H' || s[1] != '|') return false;
    std::vector<std::string> f; std::string cur;
    const size_t end = s.back() == '\r' ? s.size() - 1 : s.size();
    for (size_t k = 2; k < end; ++k) { const char ch = s[k]; if (ch == '|') { f.push_back(cur); cur.clear(); } else cur += ch; }
    f.push_back(cur);
    if (f.size() != 6) return false;
    Counter o;
    if (!parseLongLong(f[0], o.t) || o.t <= 0 || !parseInt(f[1], o.side) || (o.side != LOW && o.side != HIGH)) return false;
    if (f[2] != "0" && f[2] != "1") return false;
    o.H = f[2] == "1";
    if (!parseLongLong(f[3], o.lots) || o.lots < 0 || !parseLongLong(f[4], o.attack) || o.attack < 0 || !parseLongLong(f[5], o.at) || o.at <= 0) return false;
    C = o; return true;
}
// the pane's badge on one candle: the behaviours decided ON that candle, joined "A|H" (A = the turn candle absorbed, H = the hit back
// decided on this candle, E only while SHOW_E); at most 3; "" = none. The "?" stays in the hover / box until proven (no numbers here).
inline std::string badgeText(bool A, bool H, bool E) {
    std::string s; auto add = [&](const char* x) { if (!s.empty()) s += "|"; s += x; };
    if (A) add("A");
    if (H) add("H");
    if (SHOW_E && E) add("E");
    return s;
}
// the biggest font (pt) from fs0 down to fsMin whose text fits maxW px (w(fs) = the text width at fs); fsMin when nothing fits
template <class W> inline int fitFont(W w, int maxW, int fs0, int fsMin) { for (int fs = fs0; fs > fsMin; --fs) if (w(fs) <= maxW) return fs; return fsMin; }
// the box's Counter row (in the defending side's column): "Hit back 2.4x" (the defenders' lots / the attack) / "Absorbed only"; the lots
// themselves are in the turns file and the status (the box columns keep their width: the mockup's "640 lots" would widen both columns)
inline std::string counterText(const Counter& c) {
    if (!c.H) return "Absorbed only";
    const double r = c.attack > 0 ? (double)c.lots / (double)c.attack : 0.0; char b[48]; std::snprintf(b, sizeof b, "%.1fx", r);
    return std::string("Hit back ") + b;
}
inline std::string tagsOfH(const Line& L, bool H) { std::string s = "R"; if (SHOW_E && L.E) s += " E"; if (L.A) s += " A"; if (H && L.A) s += " H"; return s + " ?"; }

// ---- (1.4.0) L / X: is the defended level REFILLED on the RE-TEST? -------------------------------------------------------------------
// MBO roadmap Top Defender #4 (Rassul 2026-10-21, defender4-mockup-v1.png; letters L = reloaded, X = empty, 11:32 CT). Research + test:
// /home/claude/pt/d4/Defender4-findings.md. A RE-TEST = after the turn, price first moves RT_AWAY ATR away from the R line (within RT_SEC
// of the turn), then trades back into the zone (within max(RT_ZONE ATR, 2 ticks) of the line) within RT_SEC of leaving. The read: the
// defenders' RESTING lots in the zone (the line +- the zone, RT_DEPTH ticks toward price) at the CLOSE BEFORE the re-test candle (price was
// still away - no look-ahead) vs the market's normal resting lots per price near the top of that side x RT_PRICES:
// ratio >= RT_L -> L (reloaded), <= RT_X -> X (empty), else N (shown as the ratio only). Decided once at the re-test candle's close,
// never repainted, "?" until proven. It answers "are they defending?" (the size is there or not); the test found it does NOT predict the
// hold (heavier waiting size went with MORE breaks on the one full-book day) - a read of the defenders, not an entry trigger.
static const double RT_AWAY = 0.5, RT_ZONE = 0.1, RT_L = 1.0, RT_X = 0.35;
static const int RT_ZONE_MIN = 2, RT_DEPTH = 4, RT_PRICES = 5, RT_SEC = 900;
struct Retest {
    long long t = 0; int side = -1; long long linePx = 0;      // the turn (its key), its side, the R line (ticks)
    int state = 0;                                              // 0 = waiting for price to leave, 1 = left (watching for the return), 2 = decided
    long long leftAt = 0;                                       // the candle (key) where price had left
    long long restBefore = -1;                                  // the defenders' resting lots in the zone at the last close (-1 = unknown)
    char res = 0; double ratio = 0; long long rest = 0, at = 0; // the decision: 'L' / 'X' / 'N', the ratio, the lots, the re-test candle
};
inline long long rtZone(double atrTicks) { const long long z = (long long)std::llround(RT_ZONE * atrTicks); return std::max<long long>(z, RT_ZONE_MIN); }
// the price range (ticks) whose resting lots count: LOW (bids) [line - zone, line + RT_DEPTH], HIGH (offers) [line - RT_DEPTH, line + zone]
inline void rtRange(const Retest& r, double atrTicks, long long& lo, long long& hi) {
    const long long z = rtZone(atrTicks);
    if (r.side == HIGH) { lo = r.linePx - RT_DEPTH; hi = r.linePx + z; } else { lo = r.linePx - z; hi = r.linePx + RT_DEPTH; }
}
inline char rtClass(double ratio) { return ratio >= RT_L ? 'L' : (ratio <= RT_X ? 'X' : 'N'); }
// one CLOSED candle q (sec = seconds since the turn candle's key, or since leaving once state == 1); restNow = the resting lots in the zone
// measured at THIS close (stored for the next candle), normal = the market's normal resting lots per price (> 0).
// returns 1 = decided (r.res set), 0 = keep watching, -1 = over (expired / closed through before a re-test), -2 = no resting size known
inline int retestStep(Retest& r, const Candle& q, double atrTicks, long long restNow, double normal) {
    if (r.state == 2 || !q.ok || atrTicks <= 0) return r.state == 2 ? 1 : 0;
    const bool low = r.side != HIGH;
    if (r.state == 0) {
        if (q.t - r.t > RT_SEC) return -1;
        if (low ? q.c < r.linePx : q.c > r.linePx) return -1;                       // closed through before it ever left
        const double away = (double)(low ? q.h - r.linePx : r.linePx - q.l);   // (not "far": a Windows macro)
        if (away >= RT_AWAY * atrTicks) { r.state = 1; r.leftAt = q.t; }
        r.restBefore = restNow; return 0;
    }
    if (q.t - r.leftAt > RT_SEC) return -1;
    const long long z = rtZone(atrTicks);
    const bool back = low ? q.l <= r.linePx + z : q.h >= r.linePx - z;
    if (!back) { if (low ? q.c < r.linePx : q.c > r.linePx) return -1; r.restBefore = restNow; return 0; }
    if (r.restBefore < 0 || !(normal > 0)) return -2;
    r.rest = r.restBefore; r.ratio = (double)r.restBefore / (normal * RT_PRICES); r.res = rtClass(r.ratio); r.at = q.t; r.state = 2;
    return 1;
}
// the Q line in the turns file: Q|turn_key|side|L/X/N|resting_lots|ratio_x100|re-test_key
inline std::string qLineOf(const Retest& r) { std::ostringstream o; o << "Q|" << r.t << '|' << r.side << '|' << (r.res ? r.res : 'N') << '|' << r.rest << '|' << (long long)std::llround(r.ratio * 100.0) << '|' << r.at; return o.str(); }
inline bool parseQ(const std::string& s, Retest& R) {
    if (s.size() < 2 || s[0] != 'Q' || s[1] != '|') return false;
    std::vector<std::string> f; std::string cur; for (size_t i = 0; i <= s.size(); ++i) { if (i == s.size() || s[i] == '|') { f.push_back(cur); cur.clear(); } else if (s[i] != '\r') cur += s[i]; }
    if (f.size() != 7 || f[3].size() != 1 || (f[3][0] != 'L' && f[3][0] != 'X' && f[3][0] != 'N')) return false;
    Retest o; int side = -1; long long r100 = 0;
    if (!parseLongLong(f[1], o.t) || !parseInt(f[2], side) || (side != LOW && side != HIGH) || !parseLongLong(f[4], o.rest) || o.rest < 0 || !parseLongLong(f[5], r100) || r100 < 0 || !parseLongLong(f[6], o.at)) return false;
    o.side = side; o.res = f[3][0]; o.ratio = (double)r100 / 100.0; o.state = 2; R = o; return true;
}
// the box's Re-test row (defending side's column): "Reloaded 1.9x" / "Empty 0.2x" / "1.0x" (between: the ratio only)
inline std::string retestText(const Retest& r) {
    char b[32]; std::snprintf(b, sizeof b, "%.1fx", r.ratio);
    return r.res == 'L' ? std::string("Reloaded ") + b : (r.res == 'X' ? std::string("Empty ") + b : std::string(b));
}
// (1.4.0) the badge with the re-test letter: "A|H|L"; at most 3 letters
inline std::string badgeText4(bool A, bool H, char Q, bool E) {
    std::string s = badgeText(A, H, E);
    if (Q == 'L' || Q == 'X') { if (!s.empty()) s += "|"; s += Q; }
    return s;
}

// (1.5.0) + P (the defenders pressed in, MoveLogic.h): "A|H|L|P"; the font shrinks to fit (fitFont)
inline std::string badgeText5(bool A, bool H, char Q, bool P, bool E) {
    std::string s = badgeText4(A, H, Q, E);
    if (P) { if (!s.empty()) s += "|"; s += 'P'; }
    return s;
}

// (1.6.0) the combined badge as coloured parts: A / H / L / X in the defending side's colour (X alone: the opposite colour, as 1.4.0), then
// the move letters per BOOK side - bids first ('P' pressed in / 'B' stepped back, LOW = green), offers second (HIGH = red); "|" between
// letters in the colour of the letter before it; the "?" in the colour of the last letter. colourKey: LOW / HIGH (the pane's two colours).
struct BadgeSeg { std::string s; int colourKey = LOW; };
inline std::vector<BadgeSeg> badgeSegs(bool A, bool H, char Q, int defSide, char bidMove, char offerMove, bool E = false) {
    std::vector<BadgeSeg> o; std::string base = badgeText4(A, H, Q, E);
    if (!base.empty()) { BadgeSeg b; b.s = base; b.colourKey = (Q == 'X' && !A && !H && !bidMove && !offerMove) ? 1 - defSide : defSide; o.push_back(b); }
    auto add = [&](char L, int key) { if (!L) return; BadgeSeg b; b.s = std::string(o.empty() ? "" : "|") + L; b.colourKey = key; if (!o.empty()) { o.back().s += "|"; b.s = std::string(1, L); } o.push_back(b); };
    add(bidMove, LOW); add(offerMove, HIGH);
    return o;                                    // (1.8.0, Rassul 19:48 CT) no "?" on the badge: proof status is in the hover
}
inline std::string segsText(const std::vector<BadgeSeg>& v) { std::string s; for (size_t k = 0; k < v.size(); ++k) s += v[k].s; return s; }

// ==== (1.7.0, 2026-10-21, Rassul approved 14:20-14:22 CT) TOP ATTACKER #2 G "giving up", MIDDLE ATTACKER #1 Tr on the badge, TOP DEFENDER #2 W
// "withdrew at the level". Findings: /home/claude/pt/roadmap-mockups/attacker2-giving-up-findings.md, middle-attacker1-trapped-findings.md,
// defender2-pulled-or-filled-findings.md (+ the build's veto tests in the BookFlow 1.19.0 notes). Every decision once, never repainted, "?".

// ---- G: the ATTACKERS are giving up. At a 5-candle high / low (decided at that candle's close): from the extreme to the close the attackers
// pull their OWN resting orders near price (at a high the buyers' bids, at a low the sellers' offers) at >= G_PULL_X x that market + side's
// normal pull rate (lots / s, NET of their confirmed moves - a moved order is not one that left), they pull more than they add, and their
// attack died (attackDied: pace after the extreme <= half the pace before). Veto (tested 2026-10-21: G+E 283 cases +9.4 pts; without the 16
// on initiative candles +9.4 and steadier halves 8.5 / 10.2; on initiative 16 cases, halves +21 / -2 = noise): no G on an INITIATIVE candle
// (the candle's delta with the move >= 2x the median |delta| of the 20 before it and the close in the outer quarter of its range).
static const double G_PULL_X = 2.0;
static const long long G_MIN_SECS = 20;          // the research measured the pace only with >= 20 s left after the extreme
static const bool G_VETO_INITIATIVE = true;
// the market's normal: the median of the attackers' post-extreme pull rate (lots / s, near price) at 5-candle extremes, 7 markets
// Sep 24 - Oct 9 MBO (attk_scored.pkl apull_rate by market and side); [LOW] = sellers' offers after a low, [HIGH] = buyers' bids after a high
inline double giveUpNormal(const std::string& m, int side) {
    const bool h = side == HIGH;
    if (m == "ES") return h ? 17.82 : 17.60;
    if (m == "NQ") return h ? 32.56 : 35.22;
    if (m == "CL") return h ? 6.66 : 6.48;
    if (m == "GC") return h ? 14.59 : 14.02;
    if (m == "HG") return h ? 1.88 : 1.76;
    if (m == "NG") return h ? 2.67 : 1.88;
    if (m == "EU" || m == "6E") return h ? 17.84 : 17.94;
    return 0.0;                                  // unknown market: no normal, nothing decided
}
struct GiveUp { long long t = 0; int side = -1; bool G = false; long long pull = 0, add = 0, mov = 0, secs = 0; double x = 0, pace = -1, normal = 0; bool init = false; };
// initiative: delta = buyers lifting - sellers hitting (aggr[1] - aggr[0]); priorDelta = the deltas of up to 20 candles before (>= 5 needed)
inline bool initiativeOf(const Candle& x, int side, const std::vector<long long>& priorDelta) {
    if (priorDelta.size() < 5) return false;
    std::vector<long long> a; for (size_t k = 0; k < priorDelta.size(); ++k) a.push_back(std::llabs(priorDelta[k]));
    std::sort(a.begin(), a.end());
    const double med = a.size() % 2 ? (double)a[a.size() / 2] : 0.5 * (double)(a[a.size() / 2 - 1] + a[a.size() / 2]);
    const double bd = (double)(x.aggr[1] - x.aggr[0]) * (side == HIGH ? 1.0 : -1.0);
    const double rng = (double)std::max<long long>(x.h - x.l, 1);
    const double pos = side == HIGH ? (double)(x.c - x.l) / rng : (double)(x.h - x.c) / rng;
    return bd >= 2.0 * std::max(med, 1.0) && pos >= 0.75;
}
// decide G on candle x at its extreme `side` (LOW / HIGH). false = not decidable (no order feed for the candle, < 20 s after the extreme, no
// normal): nothing recorded, never guessed
inline bool giveUpDecide(const Candle& x, int side, double normal, bool initiative, GiveUp& out) {
    if (side != LOW && side != HIGH) return false;
    const long long secs = x.secsAfter[side];
    if (!x.gOk[side] || secs < G_MIN_SECS || !(normal > 0)) return false;
    GiveUp g; g.t = x.t; g.side = side; g.pull = x.apull[side]; g.add = x.aadd[side]; g.mov = x.amov[side]; g.secs = secs; g.normal = normal; g.init = initiative;
    const long long net = std::max<long long>(0, g.pull - g.mov);
    g.x = ((double)net / (double)secs) / normal;
    if (x.before[side] > 0 && x.secsBefore[side] > 0) g.pace = ((double)x.after[side] / (double)secs) / ((double)x.before[side] / (double)x.secsBefore[side]);
    const bool died = attackDied(x, x, false, side);
    g.G = g.x >= G_PULL_X - 1e-9 && g.pull > g.add && died && !(G_VETO_INITIATIVE && initiative);
    out = g; return true;
}
// the G line in the turns file: G|candle_key|side|G(0/1)|pulled|added|moved|secs|x*100|pace*100 (-100 = n/a)|initiative(0/1)|normal*100
inline std::string gLineOf(const GiveUp& g) {
    std::ostringstream o; o << "G|" << g.t << '|' << g.side << '|' << (g.G ? 1 : 0) << '|' << g.pull << '|' << g.add << '|' << g.mov << '|' << g.secs << '|'
      << (long long)std::llround(g.x * 100.0) << '|' << (long long)std::llround(g.pace * 100.0) << '|' << (g.init ? 1 : 0) << '|' << (long long)std::llround(g.normal * 100.0);
    return o.str();
}
inline std::vector<std::string> barFields(const std::string& s) {
    std::vector<std::string> f; std::string cur; const size_t end = !s.empty() && s.back() == '\r' ? s.size() - 1 : s.size();
    for (size_t k = 0; k < end; ++k) { if (s[k] == '|') { f.push_back(cur); cur.clear(); } else cur += s[k]; }
    f.push_back(cur); return f;
}
inline bool parseG(const std::string& s, GiveUp& G) {
    if (s.size() < 2 || s[0] != 'G' || s[1] != '|') return false;
    const std::vector<std::string> f = barFields(s); if (f.size() != 12) return false;
    GiveUp o; long long x100 = 0, p100 = 0, n100 = 0;
    if (!parseLongLong(f[1], o.t) || o.t <= 0 || !parseInt(f[2], o.side) || (o.side != LOW && o.side != HIGH)) return false;
    if ((f[3] != "0" && f[3] != "1") || (f[10] != "0" && f[10] != "1")) return false;
    o.G = f[3] == "1"; o.init = f[10] == "1";
    if (!parseLongLong(f[4], o.pull) || o.pull < 0 || !parseLongLong(f[5], o.add) || o.add < 0 || !parseLongLong(f[6], o.mov) || o.mov < 0) return false;
    if (!parseLongLong(f[7], o.secs) || o.secs <= 0 || !parseLongLong(f[8], x100) || x100 < 0 || !parseLongLong(f[9], p100) || !parseLongLong(f[11], n100) || n100 <= 0) return false;
    o.x = (double)x100 / 100.0; o.pace = (double)p100 / 100.0; o.normal = (double)n100 / 100.0;
    G = o; return true;
}
// the box row "Attackers" (in the attackers' column, coloured by meaning): "Pulling 2.9x" (G) / "Still in 0.6x" (decided, no G)
inline std::string attackersText(const GiveUp& g) { char b[48]; std::snprintf(b, sizeof b, "%s %.1fx", g.G ? "Pulling" : "Still in", g.x); return b; }

// ---- Tr (Middle Attacker #1): the Delta Profile's TRAPPED decision (TrapLogic.h TR line) - read only, never re-decided here. The hover text:
inline std::string trapHover(bool top, double lots, double x) {
    char b[96]; std::snprintf(b, sizeof b, "%s trapped: %.0f lots (%.1fx) %s", top ? "Buyers" : "Sellers", lots, x, top ? "above the old high" : "below the old low");
    return b;
}

// ---- W: the DEFENDERS WITHDREW at a key level (Top Defender #2). A TOUCH of a key level (PDH / PDL / PFH / PFL / ONH / ONL / LonHI / LonLO):
// a trade within W_TOL ticks of it after price was >= the re-arm distance away (from above = a support test, from below = a resistance
// test; the study's level_events rule), one per level per 15 min. In the first 60 s from the touch, in the zone (a support: L-2 .. L+1, a
// resistance: L-1 .. L+2 ticks - one tick on the approach side, two beyond) on the DEFENDING side (bids at a support, offers at a resistance):
// pulled (cancels / size cuts that were not fills; a moved order = a pull at its old price), added, filled. Decided at touch + 60 s:
//   W      = net pulled (pulled - added) > 0 and >= 2x what traded there
//   Filled = traded >= the market's median and >= the net pulled (shown grey: on its own it is not support)
// Drawn at SUPPORTS only (resistances: -3 pts = nothing; logged). The "already bounced" veto was tested 2026-10-21 (1,275 touches, matched on
// distance at the decision): W not bounced -6.3 (n 95), W already >= 1 tick back off the level -34.9 (n 71) - the veto would drop the
// strongest warnings, so it is NOT applied (W_VETO_BOUNCE false); the bounce is recorded (flag b) for the nightly.
static const long long W_SEC = 60, W_DEDUPE = 900;
static const bool W_VETO_BOUNCE = false;
inline bool wKeyName(const std::string& n) {
    static const char* const K[] = { "PDH", "PDL", "PFH", "PFL", "ONH", "ONL", "LonHI", "LonLO" };
    std::string cur; for (size_t i = 0; i <= n.size(); ++i) {
        if (i == n.size() || n[i] == '/' || n[i] == ' ') { for (size_t k = 0; k < sizeof(K) / sizeof(K[0]); ++k) if (cur == K[k]) return true; cur.clear(); }
        else cur += n[i];
    }
    return false;
}
inline int wTolOf(const std::string& m) { return m == "NQ" ? 4 : m == "GC" ? 3 : (m == "NG" || m == "EU" || m == "6E") ? 1 : 2; }   // the tight zone's min (touch tolerance)
// the market's median lots traded in the zone in the first minute ([LOW] = support / bids, [HIGH] = resistance / offers), rm2 B_scored f60
inline double wFillMedian(const std::string& m, int side) {
    const bool r = side == HIGH;
    if (m == "ES") return r ? 215 : 426;
    if (m == "CL") return r ? 24 : 20;
    if (m == "GC") return 8;
    if (m == "HG") return r ? 4 : 3;
    if (m == "NG") return r ? 59 : 33.5;
    if (m == "EU" || m == "6E") return r ? 40.5 : 36.5;
    return 0.0;                                   // NQ was not run (raw too large): no Filled read (W still decided)
}
struct WLevel { std::string name; long long px = 0; };
struct WRead {
    long long t0 = 0, key = 0, px = 0; std::string name; int side = LOW;   // touch second, candle, level (ticks); LOW = support (bids defend)
    long long pulled = 0, added = 0, filled = 0, pxEnd = 0; bool partial = false;
    bool W = false, F = false, bounced = false; double fillX = 0;
};
inline void wDecide(WRead& r, const std::string& market) {
    const long long net = r.pulled - r.added;
    r.W = net > 0 && net >= 2 * r.filled;
    const double med = wFillMedian(market, r.side);
    r.fillX = med > 0 ? (double)r.filled / med : 0.0;
    r.F = med > 0 && (double)r.filled >= med && r.filled >= std::max<long long>(net, 0);
    r.bounced = r.pxEnd > 0 && (r.side == LOW ? r.pxEnd - r.px >= 1 : r.px - r.pxEnd >= 1);
}
inline bool wDrawn(const WRead& r) { return r.side == LOW && r.W && !(W_VETO_BOUNCE && r.bounced); }
// the box row "At level" (defending side's column): "Pulled 88%" (net pulled share of net pulled + traded; W) / "Filled 2.1x" (grey) / "-"
inline std::string atLevelText(const WRead& r) {
    char b[48]; const long long net = std::max<long long>(0, r.pulled - r.added);
    if (r.W) { std::snprintf(b, sizeof b, "Pulled %.0f%%", net + r.filled > 0 ? 100.0 * (double)net / (double)(net + r.filled) : 0.0); return b; }
    if (r.F) { std::snprintf(b, sizeof b, "Filled %.1fx", r.fillX); return b; }
    return "-";
}
// the W line in the turns file: W|touch_candle_key|side|level_ticks|pulled|added|filled|px_end_ticks|flags|touch_second|name|tick
// flags: W (withdrew), F (filled), b (already >= 1 tick back off the level at the decision), p (book seen < 60 s before the touch), - none
inline std::string wLineOf(const WRead& r, double tick) {
    std::string f; if (r.W) f += 'W'; if (r.F) f += 'F'; if (r.bounced) f += 'b'; if (r.partial) f += 'p'; if (f.empty()) f = "-";
    std::string nm; for (size_t i = 0; i < r.name.size() && nm.size() < 24; ++i) { const char c = r.name[i]; nm += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; }
    std::ostringstream o; o.precision(12);
    o << "W|" << r.key << '|' << r.side << '|' << r.px << '|' << r.pulled << '|' << r.added << '|' << r.filled << '|' << r.pxEnd << '|' << f << '|' << r.t0 << '|' << nm << '|' << tick;
    return o.str();
}
inline bool parseW(const std::string& s, WRead& R, const std::string& market) {
    if (s.size() < 2 || s[0] != 'W' || s[1] != '|') return false;
    const std::vector<std::string> f = barFields(s); if (f.size() != 12) return false;
    WRead o; double tk = 0;
    if (!parseLongLong(f[1], o.key) || o.key <= 0 || !parseInt(f[2], o.side) || (o.side != LOW && o.side != HIGH) || !parseLongLong(f[3], o.px) || o.px <= 0) return false;
    if (!parseLongLong(f[4], o.pulled) || o.pulled < 0 || !parseLongLong(f[5], o.added) || o.added < 0 || !parseLongLong(f[6], o.filled) || o.filled < 0) return false;
    if (!parseLongLong(f[7], o.pxEnd) || o.pxEnd < 0 || !parseLongLong(f[9], o.t0) || o.t0 < 0 || !parseFiniteDouble(f[11], tk) || !(tk > 0)) return false;
    const std::string& fl = f[8]; if (fl.empty()) return false;
    for (size_t k = 0; k < fl.size(); ++k) { const char c = fl[k]; if (c == 'W') o.W = true; else if (c == 'F') o.F = true; else if (c == 'b') o.bounced = true; else if (c == 'p') o.partial = true; else if (c != '-') return false; }
    o.name = f[10];
    { const double med = wFillMedian(market, o.side); o.fillX = med > 0 ? (double)o.filled / med : 0.0; }   // the decision as written (flags), never re-decided
    R = o; return true;
}
// the level watch: fed every classified near-price book event in order (OrderFlowFeed BookSink: kind 0 add / 1 pull / 2 fill, the RESTING
// side 0 bids / 1 offers, the event second, the candle key), the key levels (ticks) and the clock; finished reads wait in `done`
class LevelWatch {
public:
    int tol = 2; long long rearm = 6;
    std::vector<WLevel> lv; std::vector<WRead> open, done;
    std::vector<int> armed;                         // per level: +1 = was away above (a support test next), -1 = away below, 0 = not armed
    std::vector<long long> lastTouch;               // per level: the last touch second (15-min dedupe)
    long long lastPx = 0, firstTs = -1, lastTs = 0, touchesSeen = 0, deduped = 0;
    void setLevels(const std::vector<WLevel>& L) {
        std::vector<int> a; std::vector<long long> t;
        for (size_t i = 0; i < L.size(); ++i) { int aa = 0; long long tt = -1;
            for (size_t k = 0; k < lv.size(); ++k) if (lv[k].px == L[i].px) { aa = armed[k]; tt = lastTouch[k]; }
            if (aa == 0 && tt < 0 && lastPx > 0) aa = lastPx > L[i].px + tol ? 1 : (lastPx < L[i].px - tol ? -1 : 0);   // the study's start state
            a.push_back(aa); t.push_back(tt); }
        lv = L; armed.swap(a); lastTouch.swap(t);
    }
    void onBook(int kind, int side, long long px, long long size, long long ts, long long key) {
        if (px <= 0 || size <= 0 || ts <= 0 || (side != 0 && side != 1)) return;
        if (firstTs < 0) firstTs = ts;
        if (ts > lastTs) lastTs = ts;
        if (kind == 2) trade(px, ts, key);
        for (size_t k = 0; k < open.size(); ++k) {
            WRead& r = open[k];
            if (ts < r.t0 || ts - r.t0 >= W_SEC) continue;
            const int def = r.side == LOW ? 0 : 1;
            if (side != def) continue;
            const long long lo = r.side == LOW ? r.px - 2 : r.px - 1, hi = r.side == LOW ? r.px + 1 : r.px + 2;
            if (px < lo || px > hi) continue;
            if (kind == 0) r.added += size; else if (kind == 1) r.pulled += size; else if (kind == 2) r.filled += size;
        }
    }
    // the clock (the newest event second): reads 60 s old are finished (price = the last trade then)
    void step(long long nowTs) {
        if (nowTs > lastTs) lastTs = nowTs;
        for (size_t k = 0; k < open.size();) {
            if (lastTs - open[k].t0 >= W_SEC) { open[k].pxEnd = lastPx; done.push_back(open[k]); open.erase(open.begin() + (long)k); }
            else ++k;
        }
        if (done.size() > 200) done.erase(done.begin(), done.begin() + 100);
    }
private:
    void trade(long long px, long long ts, long long key) {
        if (lastPx <= 0) for (size_t i = 0; i < lv.size(); ++i) if (armed[i] == 0 && lastTouch[i] < 0) armed[i] = px > lv[i].px + tol ? 1 : (px < lv[i].px - tol ? -1 : 0);
        lastPx = px;
        for (size_t i = 0; i < lv.size(); ++i) {
            const long long L = lv[i].px;
            if (px >= L + rearm) armed[i] = 1; else if (px <= L - rearm) armed[i] = -1;
            int side = -1;
            if (armed[i] == 1 && px <= L + tol) side = LOW;            // came down to it: a support test
            else if (armed[i] == -1 && px >= L - tol) side = HIGH;     // came up to it: a resistance test
            if (side < 0) continue;
            armed[i] = 0; ++touchesSeen;
            if (lastTouch[i] >= 0 && ts - lastTouch[i] < W_DEDUPE) { ++deduped; continue; }
            lastTouch[i] = ts;
            WRead r; r.t0 = ts; r.key = key; r.px = L; r.name = lv[i].name; r.side = side; r.partial = firstTs < 0 || ts - firstTs < W_SEC;
            if (open.size() < 32) open.push_back(r);
        }
    }
};

// ---- the badge (1.7.0): every letter decided on the candle, the most important first - A, H, Tr, G, W, P / B (bids, then offers), L / X -
// at most BADGE_MAX letters on the badge (the font shrinks to fit the slot), the rest named in the badge's hover. Colours: A / H / L / X the
// defending side's (X alone: the opposite, as 1.4.0); Tr / G by meaning at their extreme (red at a high, green at a low); W red (a support
// at risk); P / B the side that moved (bids green, offers red). Adjacent A / H / L / X of one colour share one part ("A|H|L"); every other
// letter is its own part; "|" ends the part before the next letter, "?" ends the last.
static const int BADGE_MAX = 4;
struct BadgeIn { bool A = false, H = false, E = false; char Q = 0; int defSide = LOW; char bidMove = 0, offerMove = 0; int tr = -1, g = -1; bool w = false; };
struct BadgeLetter { std::string s; int colourKey = LOW; bool base = false; };
inline std::vector<BadgeLetter> badgeLetters(const BadgeIn& b) {
    std::vector<BadgeLetter> v; auto add = [&](const std::string& s, int key, bool base) { BadgeLetter L; L.s = s; L.colourKey = key; L.base = base; v.push_back(L); };
    if (b.A) add("A", b.defSide, true);
    if (b.H) add("H", b.defSide, true);
    if (SHOW_E && b.E) add("E", b.defSide, true);
    if (b.tr == LOW || b.tr == HIGH) add("Tr", b.tr, false);
    if (b.g == LOW || b.g == HIGH) add("G", b.g, false);
    if (b.w) add("W", HIGH, false);
    if (b.bidMove) add(std::string(1, b.bidMove), LOW, false);
    if (b.offerMove) add(std::string(1, b.offerMove), HIGH, false);
    if (b.Q == 'L' || b.Q == 'X') add(std::string(1, b.Q), (b.Q == 'X' && v.empty()) ? 1 - b.defSide : b.defSide, true);
    return v;
}
inline std::vector<BadgeSeg> badgeSegs7(const BadgeIn& b, std::vector<std::string>* dropped = nullptr, int maxLetters = BADGE_MAX) {
    std::vector<BadgeLetter> v = badgeLetters(b);
    if (dropped) dropped->clear();
    if ((int)v.size() > maxLetters) { if (dropped) for (size_t k = (size_t)maxLetters; k < v.size(); ++k) dropped->push_back(v[k].s); v.resize((size_t)maxLetters); }
    std::vector<BadgeSeg> o; bool lastBase = false;
    for (size_t k = 0; k < v.size(); ++k) {
        if (!o.empty() && lastBase && v[k].base && o.back().colourKey == v[k].colourKey) { o.back().s += "|" + v[k].s; continue; }
        if (!o.empty()) o.back().s += "|";
        BadgeSeg s; s.s = v[k].s; s.colourKey = v[k].colourKey; o.push_back(s); lastBase = v[k].base;
    }
    return o;                                    // (1.8.0, Rassul 19:48 CT "remove the ? infront of them") no "?": the hover says "still learning"
}

// ---- (1.8.0, 2026-10-21 19:48 CT Rassul "implement hover over on the defense letters ... that give you detail about what happened") the
// badge's HOVER: one block per letter decided on the candle (every letter, also the ones with no room on the badge), each in his hover
// format - the question it answers, what happened (the record's own numbers), how to use it, the proof status. Built only from the records
// lsBookFlow already keeps (T / H / Q / M / G / W lines, the Delta Profile's TR record) - no new detection. Rows <= ~100 characters (lesson 83).
inline std::string lotsText(long long v) {                                  // 1540 -> "1,540"
    const bool neg = v < 0; unsigned long long u = neg ? 0ULL - (unsigned long long)v : (unsigned long long)v;
    std::string d = std::to_string(u), o; for (size_t k = 0; k < d.size(); ++k) { if (k && (d.size() - k) % 3 == 0) o += ','; o += d[k]; }
    return neg ? "-" + o : o;
}
inline std::string priceText(double px, double tick) {                       // 7751.25 -> "7,751.25"; the tick's decimals; ".00" dropped
    int d = 2; if (tick > 0) { d = 0; double x = tick; while (d < 8 && std::fabs(x - std::floor(x + 0.5)) > 1e-9) { x *= 10; ++d; } }
    if (!std::isfinite(px)) return "n/a";
    char b[64]; std::snprintf(b, sizeof b, "%.*f", d, std::fabs(px)); std::string r = b;
    const size_t dot = r.find('.'); std::string ip = dot == std::string::npos ? r : r.substr(0, dot), fp = dot == std::string::npos ? std::string() : r.substr(dot);
    bool zero = true; for (size_t k = 1; k < fp.size(); ++k) if (fp[k] != '0') zero = false; if (zero) fp.clear();
    std::string o; for (size_t k = 0; k < ip.size(); ++k) { if (k && (ip.size() - k) % 3 == 0) o += ','; o += ip[k]; }
    return (px < 0 ? "-" : "") + o + fp;
}
inline std::string xText(double x) { char b[24]; std::snprintf(b, sizeof b, "%.1fx", std::isfinite(x) ? x : 0.0); return b; }
// the status row: "proven" once the nightly proves the letter for this market, else "still learning" with how many this chart has decided
inline std::string statusText(bool proven, long long n) { return proven ? std::string("   Status: proven") : "   Status: still learning, n = " + std::to_string(n) + " (this chart; scored nightly)"; }
struct HoverBlock { std::string letter; std::vector<std::string> rows; };
// A: the turn candle absorbed (Turn / T line). side LOW = bids defend a low (sellers attack), HIGH = offers defend a high (buyers attack)
inline HoverBlock hoverA(const Turn& t, double tick, bool proven, long long n) {
    const bool low = t.side != HIGH; HoverBlock b; b.letter = "A";
    b.rows.push_back(std::string("A  Absorbed - did the ") + (low ? "sellers' attack fail against the bids?" : "buyers' attack fail against the offers?"));
    if (t.attAggr > 0) b.rows.push_back(std::string("   ") + (low ? "Sellers hit " : "Buyers lifted ") + lotsText(t.attAggr) + (low ? " lots into the bids at the low, and the bids held:" : " lots into the offers at the high, and the offers held:"));
    else b.rows.push_back(std::string("   ") + (low ? "Sellers out-hit the bids at the low (lots not in the record), and the bids held:" : "Buyers out-lifted the offers at the high (lots not in the record), and the offers held:"));   // a T line read back has no attack size
    b.rows.push_back(std::string("   refill effort ") + (low ? "bids " : "offers ") + lotsText(t.defNet) + (low ? " vs offers " : " vs bids ") + lotsText(t.attNet) + " net lots (" + marginText(t.marginPct) + " bigger), close "
                     + (low ? "above" : "below") + " the R line " + priceText((double)t.linePx * tick, tick));
    b.rows.push_back(std::string("   Use: the ") + (low ? "buyers held the low - look for the turn up; it fails on a close below " : "sellers held the high - look for the turn down; it fails on a close above ") + priceText((double)t.linePx * tick, tick));
    b.rows.push_back(statusText(proven, n)); return b;
}
// H: the defenders hit back after the A (Counter / H line)
inline HoverBlock hoverH(const Counter& c, bool proven, long long n) {
    const bool low = c.side != HIGH; HoverBlock b; b.letter = "H";
    b.rows.push_back(std::string("H  Hit back - did the ") + (low ? "buyers" : "sellers") + " strike back after absorbing the attack?");
    b.rows.push_back(std::string("   Within ") + std::to_string(COUNTER_BARS) + " candles " + (low ? "buyers lifted " : "sellers hit ") + lotsText(c.lots) + " lots = "
                     + xText(c.attack > 0 ? (double)c.lots / (double)c.attack : 0.0) + " the " + lotsText(c.attack) + "-lot attack; the close got 0.25+ ATR " + (low ? "above the low" : "below the high"));
    b.rows.push_back(std::string("   Use: the defenders turned attackers - the turn has follow-through (no H = absorbed only)"));
    b.rows.push_back(statusText(proven, n)); return b;
}
// Tr: the Delta Profile's trapped-traders record (top = trapped buyers above the old high)
inline HoverBlock hoverTr(bool top, double lots, double x, bool proven, long long n) {
    HoverBlock b; b.letter = "Tr";
    b.rows.push_back(std::string("Tr  Trapped - are ") + (top ? "buyers" : "sellers") + " stuck on the wrong side of the old " + (top ? "high?" : "low?"));
    b.rows.push_back(std::string("   ") + (top ? "Buyers" : "Sellers") + " trapped " + lotsText((long long)std::llround(lots)) + " lots (" + xText(x) + " normal) " + (top ? "above the old high" : "below the old low") + " (Delta Profile)");
    b.rows.push_back(std::string("   Use: they must ") + (top ? "sell" : "buy") + " back to get out - fuel for the move " + (top ? "down" : "up") + "; it fails if price holds " + (top ? "above the old high" : "below the old low"));
    b.rows.push_back(statusText(proven, n)); return b;
}
// G: the attackers giving up (GiveUp / G line). side = the extreme: LOW = sellers (offers) after a low, HIGH = buyers (bids) after a high
inline HoverBlock hoverG(const GiveUp& g, bool proven, long long n) {
    const bool high = g.side == HIGH; HoverBlock b; b.letter = "G";
    b.rows.push_back(std::string("G  Giving up - are the ") + (high ? "buyers" : "sellers") + " (the attackers) pulling their own orders?");
    std::string pace = g.pace < 0 ? std::string("pace n/a") : "pace down to " + xText(g.pace);
    b.rows.push_back(std::string("   After the ") + (high ? "high the buyers pulled their bids at " : "low the sellers pulled their offers at ") + xText(g.x) + " normal ("
                     + lotsText(g.pull) + " pulled vs " + lotsText(g.add) + " added in " + std::to_string(g.secs) + " s); " + pace);
    b.rows.push_back(std::string("   Use: the attack is leaving - less pressure on the ") + (high ? "high" : "low") + "; weaker if they come back with size");
    b.rows.push_back(statusText(proven, n)); return b;
}
// W: the defenders withdrew at a key-level support (WRead / W line); the price is the level's
inline HoverBlock hoverW(const WRead& r, double tick, bool proven, long long n) {
    HoverBlock b; b.letter = "W"; const long long net = std::max<long long>(0, r.pulled - r.added);
    const double pct = net + r.filled > 0 ? 100.0 * (double)net / (double)(net + r.filled) : 0.0; char p[16]; std::snprintf(p, sizeof p, "%.0f%%", pct);
    b.rows.push_back("W  Withdrew - did the bids leave the key level when price touched it?");
    b.rows.push_back("   In the first " + std::to_string(W_SEC) + " s at " + r.name + " " + priceText((double)r.px * tick, tick) + " the bids pulled " + p + ": "
                     + lotsText(net) + " lots net pulled vs " + lotsText(r.filled) + " traded");
    b.rows.push_back("   Use: a warning - the support may not hold; wait for the bids to come back before buying it");
    b.rows.push_back(statusText(proven, n)); return b;
}
// P / B for one BOOK side (0 bids, 1 offers): the moved lots, ticks and x normal of the move that qualified (MoveLogic Read / M line)
inline HoverBlock hoverMove(char letter, int bookSide, long long lots, long long ticks, double x, bool proven, long long n) {
    const bool bids = bookSide == 0, in = letter == 'P'; HoverBlock b; b.letter = std::string(1, letter);
    b.rows.push_back(std::string(1, letter) + (in ? "  Pressed in - did the " : "  Stepped back - did the ") + (bids ? "bids" : "offers") + (in ? " move toward price?" : " move away from price?"));
    b.rows.push_back(std::string("   ") + (bids ? "Bids (buyers) moved " : "Offers (sellers) moved ") + lotsText(lots) + " lots up to " + std::to_string(ticks) + (ticks == 1 ? " tick " : " ticks ")
                     + (in ? "toward" : "away from") + " price = " + xText(x) + " normal");
    b.rows.push_back(in ? std::string("   Use: they lean on price with resting size - it backs their side of the level")
                        : std::string("   Use: they make room - less resting size where price is; their side is weaker here"));
    b.rows.push_back(statusText(proven, n)); return b;
}
// L / X: the re-test of the R line (Retest / Q line); linePx = the turn's R line price (0 = unknown)
inline HoverBlock hoverQ(const Retest& r, double linePx, double tick, bool proven, long long n) {
    const bool low = r.side != HIGH, L = r.res == 'L'; HoverBlock b; b.letter = std::string(1, r.res);
    const char* who = low ? "bids" : "offers";
    b.rows.push_back(std::string(L ? "L  Reloaded - were the " : "X  Empty - were the ") + who + (L ? " back at the level on the re-test?" : " gone from the level on the re-test?"));
    b.rows.push_back(std::string("   Re-test of the R line") + (linePx > 0 ? " " + priceText(linePx, tick) : std::string()) + ": " + lotsText(r.rest) + " lots of " + who + " waited in the zone = "
                     + xText(r.ratio) + " normal (" + (L ? "reloaded at 1.0x or more)" : "empty at 0.35x or less)"));
    b.rows.push_back(L ? std::string("   Use: the defenders are still there - the level can hold again (a read of them, not an entry)")
                       : std::string("   Use: nobody is waiting at the level - a re-test is more likely to break"));
    b.rows.push_back(statusText(proven, n)); return b;
}
// the whole hover: the badge, then one block per letter (in badge order), the letters with no room named in the header
// a row longer than HOVER_WRAP characters continues on the next line (indented 5), broken at a space: narrow columns fit the pane
static const size_t HOVER_WRAP = 56;
inline std::string wrapRow(const std::string& row, size_t width = HOVER_WRAP) {
    std::string o, rest = row; bool first = true;
    while (rest.size() > width) {
        size_t cut = rest.rfind(' ', width); if (cut == std::string::npos || cut <= 6) cut = width;
        o += (first ? "" : "\n") + rest.substr(0, cut); first = false;
        size_t k = cut; while (k < rest.size() && rest[k] == ' ') ++k;
        rest = "     " + rest.substr(k);
    }
    return o + (first ? "" : "\n") + rest;
}
inline std::string hoverText(const std::string& badge, const std::vector<std::string>& dropped, const std::vector<HoverBlock>& blocks) {
    std::string t = "Defense " + badge; if (!dropped.empty()) { t += "  (+ "; for (size_t k = 0; k < dropped.size(); ++k) t += (k ? ", " : "") + dropped[k]; t += ": no room on the badge)"; }
    for (size_t k = 0; k < blocks.size(); ++k) { t += "\n"; for (size_t r = 0; r < blocks[k].rows.size(); ++r) t += "\n" + wrapRow(blocks[k].rows[r]); }
    return t;
}
// the fit cascade when the pane is too small for every row: level 1 drops the "Use:" rows, level 2 also the "Status:" rows (the question
// and the numbers always stay)
inline std::string hoverCompact(const std::string& t, int level) {
    if (level <= 0) return t;
    std::string o, cur; bool first = true, dropping = false;
    for (size_t i = 0; i <= t.size(); ++i) {
        if (i < t.size() && t[i] != '\n') { cur += t[i]; continue; }
        const bool cont = cur.compare(0, 5, "     ") == 0;                 // a wrapped row's continuation follows its row
        const bool drop = cont ? dropping : (cur.compare(0, 7, "   Use:") == 0 || (level >= 2 && cur.compare(0, 10, "   Status:") == 0));
        if (!cont) dropping = drop;
        if (!drop) { if (!first) o += '\n'; o += cur; first = false; }
        cur.clear();
    }
    return o;
}
// where the hover box goes: below the badge (top = badge bottom + 3), starting at the badge's left, inside the pane, never over the info box
// (>= 4 px clear); scanning left / right from there; if no spot below the badge, below the box. sizeAt(top, w, h) = the box size at that top.
struct TipRect { int l = 0, t = 0, r = -1, b = -1; };
template <class SizeAt> inline bool tipSpot(int L, int T, int R, int B, const TipRect& badge, const TipRect* box, SizeAt sizeAt, TipRect& out) {
    std::vector<int> tops; tops.push_back(badge.b + 3); if (box && box->r > box->l) tops.push_back(box->b + 4);
    for (size_t q = 0; q < tops.size(); ++q) {
        const int top = tops[q]; if (top < T) continue;
        int w = 0, h = 0; sizeAt(top, w, h);
        if (w <= 0 || h <= 0 || top + h > B - 2 || w > R - L - 4) continue;
        const int lo = L + 2, hi = R - 2 - w, l0 = std::max(lo, std::min(hi, badge.l));
        auto clear = [&](int l) { return !box || box->r <= box->l || l + w + 4 <= box->l || l >= box->r + 4 || top + h + 4 <= box->t || top >= box->b + 4; };
        for (int d = 0; d <= R - L; d += 2) {
            if (l0 - d >= lo && clear(l0 - d)) { out.l = l0 - d; out.t = top; out.r = l0 - d + w; out.b = top + h; return true; }
            if (d && l0 + d <= hi && clear(l0 + d)) { out.l = l0 + d; out.t = top; out.r = l0 + d + w; out.b = top + h; return true; }
            if (l0 - d < lo && l0 + d > hi) break;
        }
    }
    return false;
}

// ---- the line's life on the chart (lsDefenseLines) --------------------------------------------------------------------------------------
// from the turn candle forward: ends at the first candle that CLOSES through the line (broken, drawn faded with x), when the next line
// starts, or after 15 minutes - whichever comes first. closes[k] / times[k] = the candles after the turn candle (k = 0 is the next one).
struct Life { int bars = 0; bool broken = false; };
// (1.1.0, Rassul 13:48 "no more than 3 candles or till the next lines") from the candle the line refers to: at most maxBars candles in all
// (that candle + the ones after it), until the next line starts, or a close through it. closes / times = the candles AFTER the start candle.
inline Life lifeBars(int side, double px, const std::vector<double>& closes, const std::vector<long long>& times, long long nextLineT, int maxBars) {
    Life L;
    for (size_t k = 0; k < closes.size() && k < times.size() && (int)k + 1 < maxBars; ++k) {
        if (nextLineT > 0 && times[k] >= nextLineT) break;
        L.bars = (int)k + 1;
        if (side == HIGH ? closes[k] > px : closes[k] < px) { L.broken = true; break; }
    }
    return L;
}
inline Life lifeOf(int side, double linePx, long long turnT, const std::vector<double>& closes, const std::vector<long long>& times, long long nextLineT, int maxSec) {
    Life L;
    for (size_t k = 0; k < closes.size() && k < times.size(); ++k) {
        if (times[k] - turnT > maxSec) break;
        if (nextLineT > 0 && times[k] >= nextLineT) break;
        L.bars = (int)k + 1;
        if (side == HIGH ? closes[k] > linePx : closes[k] < linePx) { L.broken = true; break; }
    }
    return L;
}
}  // namespace dt
#endif
