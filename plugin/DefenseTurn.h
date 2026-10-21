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
static const char* const VERSION = "1.4.0";   // (1.4.0, 2026-10-21 Rassul approved defender4-mockup-v1 + L / X 11:32 CT) Top Defender #4: the RE-TEST of the refill level (retestStep): price moved >= 0.5 ATR away, then came back into the zone within 15 min; the defenders' RESTING lots in the zone at the close BEFORE it came back vs the market normal -> L (reloaded) / X (empty) / N; Q lines in the turns file; badge "A|H|L"; box row "Re-test";   // (1.3.0, 2026-10-21 Rassul approved defender5-mockup-v2 11:25 CT "build this") H = the defenders HIT BACK after an A (counterDecide), the H line in the turns file, the pane badge text "A|H" (badgeText), the Counter row text;   // (1.1.0, 2026-10-20 13:54 CT Rassul "okay build", defense-E-R-mockup.png) the gray EXHAUSTION line (attack died AT the price, touch rule) on pivots that have an R; T lines carry the pivot candle; lines at most 3 candles from the candle they refer to (yellow dot)
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
