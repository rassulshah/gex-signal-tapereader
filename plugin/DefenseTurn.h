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
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cmath>

namespace dt {
static const char* const VERSION = "1.1.0";   // (1.1.0, 2026-10-20 13:54 CT Rassul "okay build", defense-E-R-mockup.png) the gray EXHAUSTION line (attack died AT the price, touch rule) on pivots that have an R; T lines carry the pivot candle; lines at most 3 candles from the candle they refer to (yellow dot)
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
    t.A = att > def;
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
inline bool parseLine(const std::string& s, Line& L) {
    if (s.size() < 3 || (s[0] != 'T' && s[0] != 'L') || s[1] != '|') return false;
    std::vector<std::string> f; std::string cur;
    for (size_t k = 2; k < s.size(); ++k) { const char ch = s[k]; if (ch == '\r') continue; if (ch == '|') { f.push_back(cur); cur.clear(); } else cur += ch; }
    f.push_back(cur);
    if (f.size() != 10 && f.size() != 11) return false;
    char* e = nullptr;
    L.kind = s[0];
    L.t = std::strtoll(f[0].c_str(), &e, 10); if (*e || L.t <= 0) return false;
    L.side = (int)std::strtol(f[1].c_str(), &e, 10); if (*e || (L.side != LOW && L.side != HIGH)) return false;
    L.pivotPx = std::strtod(f[2].c_str(), &e); if (*e || !(L.pivotPx > 0)) return false;
    L.linePx = std::strtod(f[3].c_str(), &e); if (*e || !(L.linePx > 0)) return false;
    L.margin = (int)std::strtol(f[4].c_str(), &e, 10); if (*e || L.margin < 0) return false;
    L.E = f[5] == "1"; L.A = f[6] == "1";
    L.defNet = std::strtoll(f[7].c_str(), &e, 10); if (*e) return false;
    L.attNet = std::strtoll(f[8].c_str(), &e, 10); if (*e) return false;
    L.tick = std::strtod(f[9].c_str(), &e); if (*e || !(L.tick > 0)) return false;
    L.pivotT = 0; if (f.size() == 11) { L.pivotT = std::strtoll(f[10].c_str(), &e, 10); if (*e || L.pivotT < 0) return false; }
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
    for (size_t k = 2; k < s.size(); ++k) { const char ch = s[k]; if (ch == '\r') continue; if (ch == '|') { f.push_back(cur); cur.clear(); } else cur += ch; }
    f.push_back(cur);
    if (f.size() != 5) return false;
    char* e = nullptr;
    X.t = std::strtoll(f[0].c_str(), &e, 10); if (*e || X.t <= 0) return false;
    X.side = (int)std::strtol(f[1].c_str(), &e, 10); if (*e || (X.side != LOW && X.side != HIGH)) return false;
    X.px = std::strtod(f[2].c_str(), &e); if (*e || !(X.px > 0)) return false;
    X.tick = std::strtod(f[3].c_str(), &e); if (*e || !(X.tick > 0)) return false;
    X.at = std::strtoll(f[4].c_str(), &e, 10); if (*e || X.at <= 0) return false;
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
