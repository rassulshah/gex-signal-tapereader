#ifndef LS_LMTZONESLOGIC_H
#define LS_LMTZONESLOGIC_H
// LmtZonesLogic.h 1.3.0 (2026-10-25) -- LMT ZONES of lsLiquidityProfile 0.54.0. SDK-free; tested by tests_lmz/test_lmz.cpp (logic),
// tests_lmz/test_lmz_host.cpp (mock host) and replayed on recorded MBO sessions (tests_lmz/replay_lmz.cpp, tests_lmz/fill_parity.py).
//
// 1.3.0 (Rassul's go 2026-10-25; research iter1024/iter6 ITER6-HARDENING.md 4.2 + Card F1, the PROVEN read H6-FILL, 17,476 fills strict
// dedup, 7 / 7 markets): the drawn HELD? tag is gone (HELD is still decided and logged as T lines). NEW: the AFTER-FILL READ. When a
// zone's order is FULLY filled (traded >= the most it ever showed) inside the market's RTH window (the study's RTH0 .. 15:00 CT), the next
// 30 s are watched (window [fill, fill + 30 s), exactly as lmt6_indep.py):
//   - stack = NEW lots minus DEL lots of DEFENDER-side orders (the filled order's side) at a FIXED band 1 .. floor(max(1, 0.25 ATR)) ticks
//     BEYOND the filled price (below a bid / above an offer); a DEL counts the order's last shown size (= the Databento C row size);
//     nb = stack / (adds + cancels) >= the market's top-third line (strict-dedup q2/3: CL 0.0102 ES 0.0144 6E 0.0114 GC -0.0076 HG 0.0149
//     NG 0.0414 NQ -0.0068);
//   - delta = defender-side aggressor lots minus attacker aggressor lots in [fill + 10 s, fill + 30 s) > 0 (the aggressor = the opposite of
//     the resting order's side in the book; implied / unknown-side fills not counted);
//   - BOTH -> "RE-BID" (the sweep likely fails: 53% failed vs 39%, research), else "ACCEPTED" (likely continuation).
// Decided ONCE at fill + 30 s (the first event at / after it; a bar close 60 s later if the feed is idle), recorded as an R line
// (R|id|fill_ms|decide_ms|RE-BID or ACCEPTED|stack_lots|delta_30s|adds|cancels), read back after a restart, never repainted. Strict dedup:
// one read per side and price per session (the study's headline). FAR zones never. A read whose 30 s were not seen (an IRT restart inside
// the window) is never decided (counted as lost). A 1.2.0 reader skips R lines (unknown type, not "bad").
// 1.2.0 (Rassul approved option A + the v0.49 mockup, LIMIT-PROFILE.md section 7): a big resting order is a MAGNET / TARGET, not a wall
// (zones reverse 30.1% vs 34.7% at random prices; 57% of large orders at pivot candidates end FILLED and price goes through).
//   - Every drawn kind (PRINT / ABC / ORIGIN / NEAR offers) is a thin GREY MAP LINE, label "LMT <lots> (<KIND>)" left of its start; FAR as
//     before (a thin line, the size only). Per-market draw exceptions removed (pooled, heterogeneity p 0.85): the kinds are drawn in all 7
//     markets with one rule; bids never drawn; per-market floors and the 3x / 2.5x median entry / keep lines unchanged. The nightly
//     PRINT / ORIGIN / NEAR / GAMMA_ONLY / BIDS / ABC params are ignored (counted in the params source text).
//   - STATE TAGS, decided on CLOSED bars, recorded (T lines), never repainted: FILLING = the zone order traded during the bar while price
//     was in its band (grey); HELD = the order still resting at the close of a 3-min bar that traded into the band (order price +/- 0.25 ATR)
//     and closed back on the defended side (below an offer's band / above a bid's band) with the extreme since contact inside the tolerance
//     (+0.25 ATR beyond the far edge) - drawn "HELD?" (red offer / green bid), the only retest-place mark. At most one of each per zone; a
//     bar where the order traded is FILLING, never HELD. PULLED = the existing X (E line).
//   - LOG ONLY (A / W / S lines, never drawn): FAST / DRIFT arrival (ATR travelled in the 60 s before the first print in the band, market
//     terciles of the study), EATEN (> 30 s inside the band), STEPPED (the same id re-priced toward price after the close back).
//   - B lines: the last bar judged, so a restart never decides a tag again on a bar already judged. Unknown line types are skipped.
// 1.1.0 / 1.0.0 history:
// Rassul approved mockup v3 on 2026-10-22 09:34 CT ("Ok build it") with two fixes: (1) gamma lines GREY (not purple), the yellow
// confluence diamond kept, ABC internal-breakout zones grey; (2) far fewer small order lines - only the zone orders and the far orders are
// drawn, every other order stays in the LMT column. Rules = PULLBACK-ZONES-FINAL-2026-10-22.md (research-artifacts\learning):
//   - A zone = ONE real large resting LIMIT order (1.1.0: >= 3x the live median resting order, per-market floor), rested >= 30 s at its
//     price, re-priced <= 1x. The zone ends the moment the order is pulled (X). Re-priced again (2nd move) or out of its band = ends (X); a
//     first re-price inside the band = the zone follows it (a new segment; the old one stays as drawn).
//   - Kinds (offers above price; bids mirror, LOG ONLY): PRINT = in the print area of a WITH-TREND breakdown (BD: the first RED
//     3-min candle closing below a confirmed 1-ATR zigzag low; area = BD close - 0.1 ATR .. broken level + 0.25 ATR) in the last 3 h;
//     ABC = the same in an INTERNAL (counter-trend) break; ORIGIN = within 0.25 ATR of the BD wave's origin high;
//     NEAR = the biggest other real order within 2 ATR; FAR = 2-3 ATR, thin line, a map only (no edge).
//   - Gamma confluence: within 0.25 ATR of a MenthorQ gamma level -> the yellow diamond.
//   - Decided on CLOSED bars, recorded append-only (Z / M / F / E lines), read back after a restart - a zone only extends, then ends ONCE.
// No look-ahead: the zigzag, trend and breakouts use closed bars only; an order counts only once it has rested 30 s by the bar's end.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace lmz {
static const char* const VERSION = "1.3.0";   // 1.3.0 (2026-10-25) the AFTER-FILL READ RE-BID / ACCEPTED (H6-FILL, R lines), HELD? no longer drawn (still logged);   // 1.2.0 (2026-10-25) grey map lines, pooled draw rule, FILLING / HELD? state tags (T lines), FAST / DRIFT / EATEN / STEPPED log only; 1.1.0 (2026-10-22 evening): (2026-10-22 evening fix build; Rassul 19:19 CT "the big lot methodology is causing too much filtering" - zones 0 in ES / NQ all evening, ES S20 194 = the RTH tail at night): an LMT = ONE resting order >= MULT (3.0x) the MEDIAN resting order size in the live book within +/- 2 ATR of price (computed at each bar close, so RTH / overnight adapt by themselves), above a per-market floor (ES 10, NQ 5, CL 5, GC 3, HG 3, NG 3, 6E 5 lots), rested >= 30 s; HYSTERESIS: a live zone ends (SHRANK) only when its order falls under KEEP_MULT (2.5x) the median, or is pulled / filled; S5 / S10 / S20 stay only as context (the LMT / LMT F column footer, the s20 field of the record); params MULT / KEEP_MULT / FLOOR / MED_ATR in LRA-LmtZones-<MKT>.csv
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
enum Kind { K_PRINT = 0, K_ABC = 1, K_ORIGIN = 2, K_NEAR = 3, K_FAR = 4, K_N = 5 };
enum End { LIVE = 0, FILLED = 1, PULLED = 2, MOVED = 3, BROKEN = 4, EXPIRED = 5, SESSION = 6, LEFT = 7, NEARED = 8, SHRANK = 9, END_N = 10 };   // (1.1.0) SHRANK = under KEEP_MULT x the median
enum Ink { INK_WHITE = 0, INK_GREEN = 1, INK_RED = 2, INK_GREY = 3, INK_YELLOW = 4 };

inline const char* kindCode(int k) { static const char* c[K_N] = { "PRINT", "ABC", "ORIGIN", "NEAR", "FAR" }; return k >= 0 && k < K_N ? c[k] : "?"; }
inline int kindOf(const std::string& s) { for (int k = 0; k < K_N; ++k) if (s == kindCode(k)) return k; return -1; }
inline const char* endCode(int e) {
    static const char* c[] = { "LIVE", "FILLED", "PULLED", "MOVED", "BROKEN", "EXPIRED", "SESSION", "LEFT", "NEARED", "SHRANK" };
    return e >= 0 && e < END_N ? c[e] : "?";
}
inline int endOf(const std::string& s) { for (int e = LIVE; e < END_N; ++e) if (s == endCode(e)) return e; return -1; }
inline bool finiteD(double v) { return v == v && v < 1e300 && v > -1e300; }

// ---- per-market rules ---------------------------------------------------------------------------------------------------------
// draw: 0 = log only, 1 = drawn, 2 = drawn with "?" (LEARNING)
struct Rule {
    int print = 2, origin = 2, nearO = 1;  // offers: with-trend BD-print, origin, the biggest near order
    bool gammaOnly = false;                // CL: offers drawn only with gamma confluence
    bool bids = false;                     // bids never drawn by default (study: worse than random)
    bool farOn = true;                       // far orders: thin map lines
    bool abc = true;                       // internal breaks: grey context
    long long s20Rth = 20, s20On = 20;     // fallback Big Lot S20 (resting-order pool, study table) when the live footer is still learning
    int restS = 30, maxMoves = 1;
    double nearAtr = 2.0, farAtr = 3.0, confAtr = 0.25, brokenAtr = 1.0;
    int expireMin = 240;
    bool s20Set = false;                   // the nightly params file set S20_RTH / S20_ON (context only since 1.1.0)
    double mult = 3.0, keepMult = 2.5, medAtr = 2.0;   // (1.1.0) LMT = >= mult x the median resting order within +/- medAtr ATR; kept while >= keepMult x
    long long floorLots = 5;               // (1.1.0) per-market floor (lots)
    double tagAtr = 0.25;                  // (1.2.0) the tag band: order price +/- tagAtr x ATR (the study's w 0.25); tolerance = one more band
    double fastAtr = 0.73, driftAtr = 0.40;   // (1.2.0) arrival terciles (ATR travelled in the 60 s before contact; study, all contacts w 0.25)
    int eatenS = 30;                       // (1.2.0) EATEN = more than this many seconds inside the band
    double nbMin = 0.0;                    // (1.3.0) after-fill read: net defender adds beyond / (adds + cancels) >= this (the study's top third)
    int rth0Min = 510, rth1Min = 900;      // (1.3.0) the after-fill read is decided only for fills from rth0Min to rth1Min (CT; the study's RTH .. 15:00)
    int afterS = 30, afterDeltaS = 10;     // (1.3.0) the window [fill, fill + afterS); delta counted from fill + afterDeltaS
    bool ok = true; std::string src = "defaults";
};
inline long long floorFor(const std::string& m) {   // (1.1.0) Rassul-approved floors
    if (m == "ES") return 10;
    if (m == "NQ" || m == "CL" || m == "EU" || m == "6E") return 5;
    if (m == "GC" || m == "HG" || m == "NG") return 3;
    return 5;
}
// (1.2.0) ONE draw rule for all 7 markets (pooled, heterogeneity p 0.85): PRINT / ABC / ORIGIN / NEAR offers drawn as grey map lines, no "?",
// no gamma-only rule, bids never. FAR stays as it was (on, except 6E). Per market only: the floor, the S20 context, the arrival terciles.
inline Rule ruleFor(const std::string& m) {
    Rule r; r.floorLots = floorFor(m);
    r.print = 1; r.origin = 1; r.nearO = 1; r.gammaOnly = false; r.bids = false; r.abc = true;
    // (1.3.0) nbMin = the strict-dedup top-third line of nb per market; rth0Min = the study's RTH start (lmt6_indep.py RTH0)
    if (m == "NG") { r.s20Rth = 10; r.s20On = 10; r.fastAtr = 0.75; r.driftAtr = 0.40; r.nbMin = 0.0414; r.rth0Min = 480; }
    else if (m == "HG") { r.s20Rth = 5; r.s20On = 5; r.fastAtr = 0.67; r.driftAtr = 0.37; r.nbMin = 0.0149; r.rth0Min = 430; }
    else if (m == "NQ") { r.s20Rth = 20; r.s20On = 15; r.fastAtr = 0.73; r.driftAtr = 0.40; r.nbMin = -0.0068; r.rth0Min = 510; }
    else if (m == "CL") { r.s20Rth = 15; r.s20On = 20; r.fastAtr = 0.78; r.driftAtr = 0.43; r.nbMin = 0.0102; r.rth0Min = 480; }
    else if (m == "ES") { r.s20Rth = 45; r.s20On = 25; r.fastAtr = 0.77; r.driftAtr = 0.43; r.nbMin = 0.0144; r.rth0Min = 510; }
    else if (m == "GC") { r.s20Rth = 9; r.s20On = 12; r.fastAtr = 0.71; r.driftAtr = 0.41; r.nbMin = -0.0076; r.rth0Min = 440; }
    else if (m == "EU" || m == "6E") { r.farOn = false; r.s20Rth = 42; r.s20On = 41; r.fastAtr = 0.70; r.driftAtr = 0.37; r.nbMin = 0.0114; r.rth0Min = 440; }
    else { r.print = 0; r.origin = 0; r.nearO = 0; r.abc = false; r.farOn = false; }   // not one of the 7 markets: log only
    return r;
}
// any market with nothing drawn near price
inline bool logOnly(const Rule& r) { return r.print == 0 && r.origin == 0 && r.nearO == 0; }

// strict full-token number
inline bool numLL(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 20) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || !e || *e || v < lo || v > hi) return false;
    out = v; return true;
}
inline bool numD(const std::string& t, double lo, double hi, double& out) {
    if (t.empty() || t.size() > 32) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (errno || !e || *e || !finiteD(v) || v < lo || v > hi) return false;
    out = v; return true;
}
inline std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> o; std::string c;
    for (char ch : s) { if (ch == d) { o.push_back(c); c.clear(); } else if (ch != '\r' && ch != '\n') c.push_back(ch); }
    o.push_back(c); return o;
}
// lsFlexLevels\LRA-LmtZones-<MKT>.csv (nightly): "VERSION,1" first, "MARKET,<MKT>", then "PARAM,<NAME>,<value>" rows. Garbled / wrong
// market / wrong version = the defaults, with the reason. Unknown names are ignored (newer nightly).
inline Rule parseParams(const std::vector<std::string>& lines, const std::string& market) {
    Rule r = ruleFor(market), z = r;
    if (lines.empty()) { r.src = "no params file: defaults"; return r; }
    const std::vector<std::string> v0 = split(lines[0], ',');
    if (v0.size() != 2 || v0[0] != "VERSION" || v0[1] != "1") { r.src = "params: bad VERSION line - defaults"; return r; }
    bool mk = false; int used = 0, pooled = 0;
    for (size_t i = 1; i < lines.size() && i < 200; ++i) {
        const std::vector<std::string> f = split(lines[i], ',');
        if (f.size() == 2 && f[0] == "MARKET") { if (f[1] != market) { r.src = "params: MARKET " + f[1] + " is not " + market + " - defaults"; return r; } mk = true; continue; }
        if (f.size() != 3 || f[0] != "PARAM") continue;
        const std::string& n = f[1]; long long v = 0; double d = 0; bool ok = true;
        if (n == "PRINT" || n == "ORIGIN" || n == "NEAR" || n == "GAMMA_ONLY" || n == "BIDS" || n == "ABC") { ++pooled; continue; }   // (1.2.0) one pooled draw rule: ignored
        else if (n == "FAR" && numLL(f[2], 0, 1, v)) z.farOn = v == 1;
        else if (n == "TAG_ATR" && numD(f[2], 0.05, 1.0, d)) z.tagAtr = d;                 // (1.2.0)
        else if (n == "FAST_ATR" && numD(f[2], 0.01, 10.0, d)) z.fastAtr = d;
        else if (n == "DRIFT_ATR" && numD(f[2], -10.0, 10.0, d)) z.driftAtr = d;
        else if (n == "EATEN_S" && numLL(f[2], 1, 3600, v)) z.eatenS = (int)v;
        else if (n == "FILL_NB" && numD(f[2], -1.0, 1.0, d)) z.nbMin = d;                 // (1.3.0) the after-fill read's stack line (nightly)
        else if (n == "S20_RTH" && numLL(f[2], 1, 100000, v)) { z.s20Rth = v; z.s20Set = true; }
        else if (n == "S20_ON" && numLL(f[2], 1, 100000, v)) { z.s20On = v; z.s20Set = true; }
        else if (n == "REST_S" && numLL(f[2], 0, 3600, v)) z.restS = (int)v;
        else if (n == "MAX_MOVES" && numLL(f[2], 0, 10, v)) z.maxMoves = (int)v;
        else if (n == "NEAR_ATR" && numD(f[2], 0.25, 5.0, d)) z.nearAtr = d;
        else if (n == "FAR_ATR" && numD(f[2], 0.5, 10.0, d)) z.farAtr = d;
        else if (n == "CONF_ATR" && numD(f[2], 0.0, 2.0, d)) z.confAtr = d;
        else if (n == "EXPIRE_MIN" && numLL(f[2], 15, 1440, v)) z.expireMin = (int)v;
        else if (n == "MULT" && numD(f[2], 1.5, 20.0, d)) z.mult = d;                  // (1.1.0)
        else if (n == "KEEP_MULT" && numD(f[2], 1.0, 20.0, d)) z.keepMult = d;
        else if (n == "FLOOR" && numLL(f[2], 1, 100000, v)) z.floorLots = v;
        else if (n == "MED_ATR" && numD(f[2], 0.5, 10.0, d)) z.medAtr = d;
        else ok = false;
        if (ok) ++used;
    }
    if (!mk) { r.src = "params: no MARKET line - defaults"; return r; }
    if (z.farAtr <= z.nearAtr) z.farAtr = z.nearAtr + 1.0;
    if (z.keepMult > z.mult) z.keepMult = z.mult;   // (1.1.0) the keep line is never above the entry line
    if (z.driftAtr > z.fastAtr) z.driftAtr = z.fastAtr;   // (1.2.0) DRIFT is never above FAST
    z.src = "params file (" + std::to_string(used) + " rows" + (pooled ? ", " + std::to_string(pooled) + " per-market draw rows ignored (pooled rule)" : std::string()) + ")"; return z;
}

// ---- closed bars, the causal 1-ATR zigzag and the breakouts (tags.py of the study, made incremental) ---------------------------
struct Bar { long long endMs = 0; Tick o = 0, h = 0, l = 0, c = 0; };
struct Pivot { int type = 0; Tick px = 0; int iExt = 0, iConf = 0; bool broke = false; };   // type +1 high / -1 low
struct Break { int dir = 0; Tick level = 0, close = 0; int bar = 0; long long endMs = 0; Tick origin = 0; bool haveOrigin = false; bool with = false; };

class Waves {
public:
    std::vector<Bar> bars; std::vector<Pivot> piv; std::vector<Break> brk;
    int d = 0; Tick ext = 0; int ie = 0; bool haveExt = false;
    double atr = 0;   // ticks, 14-bar ATR of closed bars including the last
    double fixedAtr = 0;   // tests only: > 0 replaces the computed ATR
    static double atrAt(const std::vector<Bar>& b, size_t i) {
        if (i < 14 || i >= b.size()) return 0;
        double s = 0;
        for (size_t k = i - 13; k <= i; ++k) { const Tick pc = b[k - 1].c; s += (double)(std::max(b[k].h, pc) - std::min(b[k].l, pc)); }
        return s / 14.0;
    }
    int trendAt(int k) const {   // +1 up (HH + HL), -1 down (LH + LL), 0 range: last 2 confirmed highs / lows with conf < k
        Tick hs[2] = { 0, 0 }, ls[2] = { 0, 0 }; int nh = 0, nl = 0;
        for (size_t i = piv.size(); i-- > 0 && (nh < 2 || nl < 2);) {
            const Pivot& p = piv[i]; if (p.iConf >= k) continue;
            if (p.type > 0 && nh < 2) hs[nh++] = p.px; else if (p.type < 0 && nl < 2) ls[nl++] = p.px;
        }
        if (nh < 2 || nl < 2) return 0;
        if (hs[0] > hs[1] && ls[0] > ls[1]) return 1;
        if (hs[0] < hs[1] && ls[0] < ls[1]) return -1;
        return 0;
    }
    void add(const Bar& b) {
        bars.push_back(b); const int i = (int)bars.size() - 1;
        atr = fixedAtr > 0 ? fixedAtr : atrAt(bars, (size_t)i);
        const double a = atr > 0 ? atr : 0;
        // breakouts against the pivots confirmed BEFORE this bar (first close beyond; colour must match, else the pivot is spent)
        for (size_t p = 0; p < piv.size(); ++p) {
            Pivot& P = piv[p]; if (P.broke || P.iConf >= i) continue;
            if (P.type > 0 && b.c > P.px) { P.broke = true; if (b.c > b.o) addBreak(+1, P.px, b, i); }
            else if (P.type < 0 && b.c < P.px) { P.broke = true; if (b.c < b.o) addBreak(-1, P.px, b, i); }
        }
        // the zigzag (1 ATR reversal), confirmed at this bar
        if (!(a > 0)) { if (!haveExt) { haveExt = true; ext = b.h; ie = i; d = 1; } return; }
        if (!haveExt) { haveExt = true; ext = b.h; ie = i; d = 1; return; }
        if (d == 1) {
            if (b.h >= ext) { ext = b.h; ie = i; }
            if ((double)b.l <= (double)ext - a) { Pivot P; P.type = 1; P.px = ext; P.iExt = ie; P.iConf = i; piv.push_back(P); d = -1; ext = b.l; ie = i; }
        } else {
            if (b.l <= ext) { ext = b.l; ie = i; }
            else if ((double)b.h >= (double)ext + a) { Pivot P; P.type = -1; P.px = ext; P.iExt = ie; P.iConf = i; piv.push_back(P); d = 1; ext = b.h; ie = i; }
        }
        if (piv.size() > 400) piv.erase(piv.begin(), piv.begin() + 100);
        if (brk.size() > 400) brk.erase(brk.begin(), brk.begin() + 100);
        if (bars.size() > 6000) trim();
    }
    // breakouts in the last `ms` before t
    std::vector<Break> recent(long long t, long long ms) const {
        std::vector<Break> o; for (size_t i = brk.size(); i-- > 0;) { if (t - brk[i].endMs > ms) break; if (brk[i].endMs <= t) o.push_back(brk[i]); }
        return o;   // newest first
    }
private:
    void addBreak(int dir, Tick level, const Bar& b, int i) {
        Break B; B.dir = dir; B.level = level; B.close = b.c; B.bar = i; B.endMs = b.endMs;
        for (size_t q = piv.size(); q-- > 0;) if (piv[q].iConf <= i && piv[q].type == (dir > 0 ? -1 : 1)) { B.origin = piv[q].px; B.haveOrigin = true; break; }
        B.with = trendAt(i) == dir;
        brk.push_back(B);
    }
    void trim() {   // keep the last 3000 bars; indexes shift
        const int cut = (int)bars.size() - 3000;
        bars.erase(bars.begin(), bars.begin() + cut);
        std::vector<Pivot> np; for (auto p : piv) { p.iExt -= cut; p.iConf -= cut; if (p.iConf >= 0) np.push_back(p); } piv.swap(np);
        std::vector<Break> nb; for (auto b : brk) { b.bar -= cut; if (b.bar >= 0) nb.push_back(b); } brk.swap(nb);
        ie -= cut; if (ie < 0) ie = 0;
    }
};

// ---- the order book (one entry per MBO order id) ------------------------------------------------------------------------------
struct Ord {
    int side = BID; Tick px = 0; long long size = 0, maxShown = 0, traded = 0;
    long long tFirst = 0, tAtPx = 0;   // first seen; when it arrived at this price (CT ms)
    int moves = 0; bool startKnown = true; bool filledLast = false; int zone = -1;
    long long shown = 0;               // (1.3.0) the size at its last NEW / MOD (a DEL counts this: the Databento C row size)
    long long tBig = 0, tFill1 = 0;    // (1.3.0) when it first showed >= the market floor; its first fill (the study: hit only after resting 30 s)
    Tick pxBig = 0; bool movedBig = false;   // (1.3.0) its price then; re-priced since (the study ends such an order MOVED: no fill read)
};
struct Seg { long long t = 0; Tick px = 0, lo = 0, hi = 0; };
struct Zone {
    int id = 0; long long oid = 0; int side = ASK, kind = K_NEAR;
    long long t0 = 0, t1 = 0; int end = LIVE;
    long long size0 = 0, sizeNow = 0, traded = 0, maxShown = 0;
    long long tFill0 = 0, tFill = 0; Tick fillPx = 0;
    std::vector<Seg> segs;               // the zone's price over time; segs.back() = now
    bool drawn = false, q = false, conf = false;
    Tick confPx = 0; long long s20 = 0; double atr = 0;   // atr in ticks at the start
    long long restMs = 0; bool startKnown = true; int moves = 0;
    long long boMs = 0; Tick level = 0, bclose = 0;        // PRINT / ABC: the breakout bar, the broken level, its close; ORIGIN: level = the origin
    bool restored = false;
    long long med = 0;                     // (1.2.0) the median resting order (lots) when the zone started (0 = not known: an older record)
    // (1.2.0) STATE TAGS (bar closes; recorded as T lines, read back): the bar end of the FILLING / HELD bar, 0 = none
    long long tagFillMs = 0, tagHeldMs = 0;
    long long contactBarMs = 0; Tick extQ = 0;   // the first closed bar that traded into the band; the extreme since (q = side x price)
    long long turnMs = 0;                  // the first close back on the defended side after contact (STEPPED is judged after it)
    // (1.2.0) LOG ONLY (A / W / S lines): the first print in the band, the arrival speed (ATR in the prior 60 s), time inside, stepped
    long long tContact = 0; double speed = 0; int arrival = 0;   // arrival 0 not yet, 1 FAST, 2 MID, 3 DRIFT, 4 unknown (no print 60 s before)
    long long dwellMs = -1; bool eaten = false, stepped = false;
    // (1.3.0) THE AFTER-FILL READ (R line): the full-fill time, the decision time (fill + 30 s), 1 = RE-BID, 2 = ACCEPTED, the numbers
    long long afFillMs = 0, afDecMs = 0; int afRead = 0, afSkip = 0; long long afStack = 0, afDelta = 0, afAdds = 0, afCans = 0;   // afSkip (not recorded; replay / status only): 1 dedup, 2 outside RTH, 3 hit < 30 s, 4 re-priced
    Tick px() const { return segs.empty() ? 0 : segs.back().px; }
    Tick pxAt(long long ms) const { Tick p = segs.empty() ? 0 : segs.front().px; for (size_t i = 0; i < segs.size(); ++i) if (segs[i].t <= ms) p = segs[i].px; return p; }   // (1.2.0) the order price in force at ms
    // the tag band half-width in ticks: max(1, tagAtr x the zone's ATR) - from the ATR rounded to 0.1 tick as the record stores it, so a
    // restored zone gets exactly the band the live one had
    Tick w(double tagAtr) const { const double a10 = (double)std::llround(atr * 10.0) / 10.0; return std::max<Tick>(1, (Tick)std::llround(tagAtr * a10)); }
    int sg() const { return side == ASK ? 1 : -1; }   // q = sg x price: an offer's defended side is BELOW (smaller q)
};
inline const char* arrivalCode(int a) { return a == 1 ? "FAST" : a == 2 ? "MID" : a == 3 ? "DRIFT" : a == 4 ? "NA" : "-"; }
inline int arrivalOf(const std::string& s) { for (int a = 1; a <= 4; ++a) if (s == arrivalCode(a)) return a; return -1; }
struct Fill { int zone = 0; long long t = 0; long long lots = 0; Tick px = 0; };

struct Tally { long long made[2][K_N] = { { 0 } }, drawn = 0, ends[END_N] = { 0 }, events = 0, unknownFill = 0, bookDrop = 0;
    long long filling = 0, held = 0, fast = 0, mid = 0, drift = 0, eaten = 0, stepped = 0, unknownLines = 0;
    long long rebid = 0, accepted = 0, afDup = 0, afLost = 0, afOff = 0, afYoung = 0, afMoved = 0; };   // afYoung: first hit < 30 s after it showed the floor   // (1.3.0) after-fill reads; dedup skips; 30 s not seen; outside RTH
inline const char* afterCode(int r) { return r == 1 ? "RE-BID" : r == 2 ? "ACCEPTED" : "-"; }
// (1.3.0) one after-fill read being watched (the 30 s after a full fill)
struct Pend { int zone = -1; int side = BID; Tick px = 0, wb = 1; long long f = 0, dec = 0, adds = 0, cans = 0, delta = 0; };   // (1.2.0) tags + log-only states

class Engine {
public:
    Rule R; std::string market;
    Waves W;
    std::unordered_map<long long, Ord> book;
    std::vector<Zone> zones;
    std::vector<int> live;                  // indexes of live zones
    std::set<long long> usedOid;            // an order id gets at most one zone per session
    std::vector<std::string> rows;          // record lines not yet written
    std::vector<Tick> gamma;                // gamma level ticks (MenthorQ, this contract)
    long long feedStartMs = 0, lastT = 0;   // the first event's time (the book is only known from then)
    int nextId = 1; long long session = -1;
    Tally tally; size_t bookCap = 300000; int farKeep = 1;   // far: the single biggest per side (replay HG 10-20: 2 per side = 48 far lines a day)
    long long lastBarMs = 0;
    long long medLots = 0, thrLots = 0, keepLots = 0; size_t medN = 0;   // (1.1.0) the live median resting order (+/- medAtr ATR), the entry / keep lines
    long long judgedMs = 0;                 // (1.2.0) the last bar end whose tags were decided (B / T lines): never decided again after a restart
    std::deque<std::pair<long long, Tick> > prints;   // (1.2.0) the trade prints of the last 90 s (log-only arrival speed)
    std::vector<Pend> pend;                 // (1.3.0) after-fill reads inside their 30 s
    std::set<std::pair<int, Tick> > afDone; // (1.3.0) strict dedup: one after-fill read per side / price per session

    // (1.1.0) the median resting order size within +/- medAtr ATR of price c (ticks), from the live book; 0 = no orders there
    static long long medianOf(std::vector<long long>& v) { if (v.empty()) return 0; std::nth_element(v.begin(), v.begin() + (long)(v.size() / 2), v.end()); return v[v.size() / 2]; }
    void bookMedian(Tick c, double a) {
        std::vector<long long> v; v.reserve(4096);
        const double reach = R.medAtr * a;
        for (auto& kv : book) { const Ord& o = kv.second; if (o.size <= 0) continue; if (std::fabs((double)(o.px - c)) <= reach) v.push_back(o.size); }
        medN = v.size(); medLots = medianOf(v);
        thrLots = std::max(R.floorLots, (long long)std::ceil(R.mult * (double)medLots - 1e-9));
        keepLots = std::max(R.floorLots, (long long)std::ceil(R.keepMult * (double)medLots - 1e-9));
    }
    long long threshold() const { return thrLots > 0 ? thrLots : R.floorLots; }

    void setGamma(const std::vector<Tick>& g) { gamma = g; std::sort(gamma.begin(), gamma.end()); }
    // one MBO event. side is used only on NEW (a FILL row carries the aggressor's side, never the order's)
    void onEvent(int act, int side, Tick px, long long size, long long oid, long long t) {
        if (t <= 0 || oid == 0) return;
        if (!pend.empty()) decideDue(t, false);   // (1.3.0) reads whose 30 s are over: decided before this event (window [fill, fill + 30 s))
        ++tally.events; if (feedStartMs == 0) feedStartMs = t; if (t > lastT) lastT = t;
        auto it = book.find(oid);
        if (act == NEW) {
            if (px <= 0 || size <= 0 || (side != BID && side != ASK)) return;
            if (!pend.empty()) afBook(side, px, size, t, true);   // (1.3.0) every NEW row counts (as the study's A rows)
            if (it != book.end()) { Ord& q = it->second; if (q.tBig && px != q.pxBig) q.movedBig = true; q.px = px; q.size = size; q.shown = size; q.tAtPx = t; if (!q.tBig && size >= R.floorLots) { q.tBig = t; q.pxBig = px; } return; }
            if (book.size() >= bookCap) prune();
            Ord o; o.side = side; o.px = px; o.size = size; o.shown = size; o.maxShown = size; o.tFirst = o.tAtPx = t; if (size >= R.floorLots) { o.tBig = t; o.pxBig = px; } book[oid] = o; return;
        }
        if (act == MOD) {
            if (px <= 0) return;
            if (it == book.end()) {   // rested before the chart opened: the age is NOT known (counted from now)
                if (size <= 0 || (side != BID && side != ASK)) return;
                if (book.size() >= bookCap) prune();
                Ord o; o.side = side; o.px = px; o.size = size; o.shown = size; o.maxShown = size; o.tFirst = o.tAtPx = t; o.startKnown = false; if (size >= R.floorLots) { o.tBig = t; o.pxBig = px; } book[oid] = o; return;
            }
            Ord& o = it->second; o.shown = size; if (o.tBig && px != o.pxBig) o.movedBig = true; if (!o.tBig && size >= R.floorLots) { o.tBig = t; o.pxBig = px; }
            if (px != o.px) {
                o.moves++; o.px = px; o.tAtPx = t; o.size = size; o.filledLast = false;
                if (o.zone >= 0) zoneMoved(o.zone, o, t);
            } else { o.size = size; o.filledLast = false; }
            if (size > o.maxShown) o.maxShown = size;
            return;
        }
        if (act == FILL) {
            if (it == book.end()) { ++tally.unknownFill; return; }
            Ord& o = it->second; if (size <= 0) return;
            if (!pend.empty() && (side == BID || side == ASK)) afDeltaAdd(o.side, size, t);   // (1.3.0) aggressor = opposite of the resting side; implied (side unknown) not counted
            o.traded += size; o.size = std::max(0LL, o.size - size); o.filledLast = true; if (!o.tFill1) o.tFill1 = t;
            if (o.zone >= 0) { Zone& z = zones[(size_t)o.zone]; if (z.end == LIVE) {
                z.traded += size; if (!z.tFill0) z.tFill0 = t; z.tFill = t; z.fillPx = px > 0 ? px : o.px;
                char b[160]; std::snprintf(b, sizeof(b), "F|%d|%lld|%lld|%lld", z.id, t, z.traded, (long long)z.fillPx); rows.push_back(b);
                if (o.traded >= std::max(o.maxShown, z.maxShown)) afStart(o.zone, t, o); } }   // (1.3.0) FULLY filled: watch the next 30 s
            onPrint(t, px > 0 ? px : o.px);   // (1.2.0) the print (log-only arrival / dwell)
            return;
        }
        if (act == DEL) {
            if (it == book.end()) return;
            Ord& o = it->second;
            if (!pend.empty()) afBook(o.side, o.px, o.shown, t, false);   // (1.3.0) a cancel row (also the one after a full fill, as the study)
            if (o.zone >= 0) { Zone& z = zones[(size_t)o.zone]; if (z.end == LIVE) finish(o.zone, o.size <= 0 ? FILLED : PULLED, t); }
            book.erase(it);
        }
    }
    // the bar has CLOSED: zones end / follow, then new zones. s20 = the size tier in force at this bar's end (lots).
    void closeBar(const Bar& b, long long s20) {
        if (b.endMs <= lastBarMs) return;   // once per bar, in order
        const long long prevEnd = lastBarMs;
        lastBarMs = b.endMs;
        if (!pend.empty()) decideDue(b.endMs, true);   // (1.3.0) an idle feed: a read is decided at a bar close >= 60 s after its 30 s
        W.add(b);
        judgeTags(b, prevEnd);   // (1.2.0) FILLING / HELD on this closed bar (zones that were live in it), before any zone ends here
        const double a = W.atr;
        if (!(a > 0)) return;
        const long long t = b.endMs;
        bookMedian(b.c, a);   // (1.1.0) the live entry / keep lines
        // 1) the price / time ends of live zones
        const bool bookKnown = feedStartMs > 0 && t >= feedStartMs + (long long)R.restS * 1000LL + 30000LL;   // (1.2.0) found by the restart test
        for (size_t k = 0; k < live.size(); ++k) {
            Zone& z = zones[(size_t)live[k]]; if (z.end != LIVE || z.t0 >= t) continue;   // (a restored zone never judged on a bar before its start)
            const Tick p = z.px(); const double dist = z.side == ASK ? (double)(p - b.c) : (double)(b.c - p);   // > 0 = still on its side
            if (-dist > R.brokenAtr * a) { finish(live[k], BROKEN, t); continue; }
            if (medN > 0 && bookKnown && liveSize(z) < keepLots) { finish(live[k], SHRANK, t); continue; }   // (1.1.0) hysteresis: under KEEP_MULT x the median; (1.2.0) only on a bar the live book saw (a restart re-feeds old bars before the book refills)
            if (t - z.t0 > (long long)R.expireMin * 60000LL) { finish(live[k], EXPIRED, t); continue; }
            if (z.kind == K_FAR && dist > (R.farAtr + 0.5) * a) { finish(live[k], LEFT, t); continue; }
            if (z.kind == K_FAR && dist > 0 && dist <= R.nearAtr * a) { finish(live[k], NEARED, t); continue; }   // the near rules take it
        }
        compact();
        // 2) new zones: only once the book has been watched long enough to know an order rested
        if (feedStartMs == 0 || t < feedStartMs + (long long)R.restS * 1000LL + 30000LL) return;
        const long long restMs = (long long)R.restS * 1000LL;
        struct C { long long oid; const Ord* o; double dist; int kind; Break br; bool hasBr; };
        std::vector<C> nearC[2], farC[2];
        const std::vector<Break> rb = W.recent(t, 3LL * 3600000LL);
        for (auto& kv : book) {
            const Ord& o = kv.second;
            if (o.zone >= 0 || o.size < threshold() || o.moves > R.maxMoves || t - o.tAtPx < restMs || usedOid.count(kv.first)) continue;
            const double dist = o.side == ASK ? (double)(o.px - b.c) : (double)(b.c - o.px);
            if (!(dist > 0)) continue;
            C c; c.oid = kv.first; c.o = &o; c.dist = dist; c.kind = K_NEAR; c.hasBr = false;
            if (dist > R.farAtr * a) continue;
            if (dist > R.nearAtr * a) { c.kind = K_FAR; farC[o.side].push_back(c); continue; }
            // the case tag (study order: a retest of a recent breakout print first, then the wave origin)
            const int want = o.side == ASK ? -1 : +1;
            for (size_t i = 0; i < rb.size() && !c.hasBr; ++i) {
                const Break& e = rb[i]; if (e.dir != want) continue;
                const bool in = want < 0 ? ((double)o.px >= (double)e.close - 0.1 * a && (double)o.px <= (double)e.level + 0.25 * a)
                                         : ((double)o.px >= (double)e.level - 0.25 * a && (double)o.px <= (double)e.close + 0.1 * a);
                if (in) { c.kind = e.with ? K_PRINT : K_ABC; c.br = e; c.hasBr = true; }
            }
            for (size_t i = 0; i < rb.size() && !c.hasBr; ++i) {
                const Break& e = rb[i]; if (e.dir != want || !e.haveOrigin) continue;
                if (std::fabs((double)(o.px - e.origin)) <= 0.25 * a) { c.kind = K_ORIGIN; c.br = e; c.hasBr = true; }
            }
            nearC[o.side].push_back(c);
        }
        auto bigger = [](const C& x, const C& y) { return x.o->size != y.o->size ? x.o->size > y.o->size : x.dist != y.dist ? x.dist < y.dist : x.oid < y.oid; };
        for (int s = 0; s < 2; ++s) {
            std::sort(nearC[s].begin(), nearC[s].end(), bigger);
            std::sort(farC[s].begin(), farC[s].end(), bigger);
            bool have[K_N] = { false, false, false, false, false }; int nFar = 0;
            for (size_t k = 0; k < live.size(); ++k) { const Zone& z = zones[(size_t)live[k]]; if (z.side != s) continue; if (z.kind == K_FAR) ++nFar; else have[z.kind] = true; }
            // the biggest near order -> its own kind; then the biggest of each tagged kind not yet live
            if (!nearC[s].empty()) { const C& c = nearC[s][0]; const int k = c.kind; if (!have[k]) { start(c.oid, *c.o, k, c.hasBr ? &c.br : nullptr, t, s20, a, restMs); have[k] = true; } }
            for (int k = K_PRINT; k <= K_ORIGIN; ++k) {
                if (have[k]) continue;
                for (size_t i = 0; i < nearC[s].size(); ++i) if (nearC[s][i].kind == k && book.count(nearC[s][i].oid) && book[nearC[s][i].oid].zone < 0) {
                    const C& c = nearC[s][i]; start(c.oid, *c.o, k, &c.br, t, s20, a, restMs); have[k] = true; break; }
            }
            for (size_t i = 0; i < farC[s].size() && nFar < farKeep; ++i) { const C& c = farC[s][i]; start(c.oid, *c.o, K_FAR, nullptr, t, s20, a, restMs); ++nFar; }
        }
    }
    void endSession(long long t) {
        if (!pend.empty()) decideDue(t, false);
        tally.afLost += (long long)pend.size(); pend.clear();   // (1.3.0) a 30 s window not finished by the session end: never decided
        for (size_t k = 0; k < live.size(); ++k) if (zones[(size_t)live[k]].end == LIVE) finish(live[k], SESSION, t);
        compact();
    }
    void clearSession() {
        for (auto& kv : book) kv.second.zone = -1;
        zones.clear(); live.clear(); usedOid.clear(); nextId = 1; judgedMs = 0; pend.clear(); afDone.clear();
    }
    // ---- (1.3.0) THE AFTER-FILL READ --------------------------------------------------------------------------------------------------
    bool inRth(long long t) const {
        long long d = t % 86400000LL; if (d < 0) d += 86400000LL;
        return d >= (long long)R.rth0Min * 60000LL && d <= (long long)R.rth1Min * 60000LL;
    }
    // a zone's order was FULLY filled at t: watch [t, t + afterS) - one read per side / price per session, never on a far zone
    void afStart(int idx, long long t, const Ord& o) {
        Zone& z = zones[(size_t)idx];
        if (z.kind == K_FAR || z.afFillMs || z.afRead || z.afSkip) return;
        if (!inRth(t)) { ++tally.afOff; z.afSkip = 2; return; }
        if (!o.tBig || o.tFill1 - o.tBig < (long long)R.restS * 1000LL) { ++tally.afYoung; z.afSkip = 3; return; }   // the study: first hit >= 30 s after it showed the floor size
        if (o.movedBig) { ++tally.afMoved; z.afSkip = 4; return; }   // the study: an order re-priced after it showed the floor size ended MOVED (no fill row)
        const std::pair<int, Tick> key(z.side, z.px());
        bool dup = afDone.count(key) > 0; for (size_t i = 0; i < pend.size() && !dup; ++i) dup = pend[i].side == z.side && pend[i].px == z.px();
        if (dup) { ++tally.afDup; z.afSkip = 1; return; }
        Pend p; p.zone = idx; p.side = z.side; p.px = z.px(); p.f = t; p.dec = t + (long long)R.afterS * 1000LL;
        const double a = z.atr > 0 ? z.atr : 0; p.wb = std::max<Tick>(1, (Tick)std::floor(std::max(1.0, R.tagAtr * a) + 1e-9));   // the study: int(max(1, 0.25 ATR))
        z.afFillMs = t; pend.push_back(p);
    }
    // a NEW (add) or DEL (cancel) row of `lots` at px on `side`: counted for every read watching that side, at its FIXED band beyond
    void afBook(int side, Tick px, long long lots, long long t, bool add) {
        if (lots <= 0) return;
        for (size_t i = 0; i < pend.size(); ++i) {
            Pend& p = pend[i]; if (p.side != side || t < p.f || t >= p.dec) continue;
            const bool in = side == BID ? (px >= p.px - p.wb && px <= p.px - 1) : (px >= p.px + 1 && px <= p.px + p.wb);
            if (in) { if (add) p.adds += lots; else p.cans += lots; }
        }
    }
    // a fill of `lots` against a resting order of side restSide: the aggressor is the other side; + for the read's defender side
    void afDeltaAdd(int restSide, long long lots, long long t) {
        const long long buy = restSide == ASK ? lots : -lots;   // buyers lift offers, sellers hit bids
        for (size_t i = 0; i < pend.size(); ++i) { Pend& p = pend[i]; if (t >= p.f + (long long)R.afterDeltaS * 1000LL && t < p.dec) p.delta += p.side == BID ? buy : -buy; }
    }
    // reads whose window is over at t (idle: only at a bar close >= 60 s after the window, so a slow feed is not cut short)
    void decideDue(long long t, bool idle) {
        for (size_t i = 0; i < pend.size();) {
            const long long need = pend[i].dec + (idle ? 60000LL : 0LL);
            if (t >= need) { afDecide(pend[i]); pend.erase(pend.begin() + (long)i); } else ++i;
        }
    }
    void afDecide(const Pend& p) {
        if (p.zone < 0 || p.zone >= (int)zones.size()) return;
        Zone& z = zones[(size_t)p.zone]; if (z.afRead) return;
        const long long tot = p.adds + p.cans; const double nb = (double)(p.adds - p.cans) / (double)std::max(tot, 1LL);
        const bool rebid = nb >= R.nbMin - 1e-12 && p.delta > 0;
        z.afRead = rebid ? 1 : 2; z.afDecMs = p.dec; z.afStack = p.adds - p.cans; z.afDelta = p.delta; z.afAdds = p.adds; z.afCans = p.cans;
        if (rebid) ++tally.rebid; else ++tally.accepted;
        afDone.insert(std::make_pair(p.side, p.px));
        rows.push_back(afterRow(z));
    }
    static std::string afterRow(const Zone& z) {
        char r[220]; std::snprintf(r, sizeof(r), "R|%d|%lld|%lld|%s|%lld|%lld|%lld|%lld", z.id, z.afFillMs, z.afDecMs, afterCode(z.afRead), z.afStack, z.afDelta, z.afAdds, z.afCans);
        return r;
    }
    // ---- (1.2.0) the STATE TAGS on a closed bar --------------------------------------------------------------------------------------
    // For every non-FAR zone that was live during the bar (still live, or ended inside it): contact = the bar traded into the band
    // (q >= L - w, q = side x price, L = side x the order price); the extreme since contact is kept from the bars (rebuilt the same way
    // after a restart). FILLING = the order traded during this bar (its last fill inside the bar, at a price in the band). HELD = no fill in
    // this bar, the order still resting at the close, the bar traded into the band, closed back on the defended side (q close < L - w) and
    // the extreme since contact stayed inside the tolerance (<= L + 2w). One of each per zone; bars already judged (judgedMs) only track.
    void judgeTags(const Bar& b, long long prevEnd) {
        bool any = false;
        for (size_t i = 0; i < zones.size(); ++i) {
            Zone& z = zones[i];
            if (z.kind == K_FAR || z.segs.empty() || z.t0 >= b.endMs) continue;
            if (z.end != LIVE && z.t1 <= prevEnd) continue;            // ended before this bar
            any = true;
            const int sg = z.sg(); const Tick L = (Tick)sg * z.pxAt(b.endMs), w = z.w(R.tagAtr);
            const Tick hiQ = sg > 0 ? b.h : -b.l, cQ = (Tick)sg * b.c;
            const bool touched = hiQ >= L - w;
            if (touched && !z.contactBarMs) { z.contactBarMs = b.endMs; z.extQ = hiQ; }
            else if (z.contactBarMs) z.extQ = std::max(z.extQ, hiQ);
            const bool inTol = z.contactBarMs && z.extQ <= L + 2 * w;
            const bool closeBack = touched && inTol && cQ < L - w;
            if (closeBack && !z.turnMs) z.turnMs = b.endMs;
            if (b.endMs <= judgedMs) continue;                          // judged before a restart: track only, decide nothing
            const Tick fq = (Tick)sg * z.fillPx;
            const bool tradedHere = z.tFill > prevEnd && z.tFill <= b.endMs && z.traded > 0 && fq >= L - w && fq <= L + w;
            if (tradedHere) {
                if (!z.tagFillMs) { z.tagFillMs = b.endMs; ++tally.filling; tagRow(z, "FILLING", b.endMs); }
            } else if (!z.tagHeldMs && closeBack && z.end == LIVE && liveSize(z) > 0) {
                z.tagHeldMs = b.endMs; ++tally.held; tagRow(z, "HELD", b.endMs);
            }
        }
        if (any && b.endMs > judgedMs) { judgedMs = b.endMs; char r[48]; std::snprintf(r, sizeof(r), "B|%lld", b.endMs); rows.push_back(r); }
    }
    // ---- (1.2.0) LOG ONLY: every trade print -> the first print in a zone's band (arrival speed), the time inside (EATEN) -------------
    void onPrint(long long t, Tick px) {
        if (px <= 0 || t <= 0) return;
        prints.push_back(std::make_pair(t, px));
        while (!prints.empty() && prints.front().first < t - 90000) prints.pop_front();
        if (prints.size() > 200000) prints.pop_front();
        for (size_t k = 0; k < live.size(); ++k) {
            Zone& z = zones[(size_t)live[k]];
            if (z.end != LIVE || z.kind == K_FAR || t <= z.t0 || z.segs.empty()) continue;
            const int sg = z.sg(); const Tick L = (Tick)sg * z.px(), w = z.w(R.tagAtr), q = (Tick)sg * px;
            const double a10 = (double)std::llround(z.atr * 10.0) / 10.0;
            if (!z.tContact) {
                if (q < L - w) continue;
                z.tContact = t;
                size_t j = 0; while (j < prints.size() && prints[j].first < t - 60000) ++j;   // the first print at / after 60 s before
                if (j + 1 < prints.size() && a10 > 0) {                    // a print before this one exists in the window
                    z.speed = (double)(L - w - (Tick)sg * prints[j].second) / a10;
                    z.arrival = z.speed >= R.fastAtr ? 1 : z.speed <= R.driftAtr ? 3 : 2;
                } else z.arrival = 4;
                if (z.arrival == 1) ++tally.fast; else if (z.arrival == 3) ++tally.drift; else if (z.arrival == 2) ++tally.mid;
                char r[160]; std::snprintf(r, sizeof(r), "A|%d|%lld|%lld|%s", z.id, t, (long long)std::llround(z.speed * 100.0), arrivalCode(z.arrival)); rows.push_back(r);
                continue;
            }
            if (z.dwellMs < 0 && (double)q < (double)(L - w) - 0.1 * a10) dwellEnd(z, t);   // left the band: the time inside is known
        }
    }
    // the session record header (1.2.0 adds T / B and the log-only A / W / S lines)
    static const char* header() { return "Z|id|oid|side|kind|t0|px|lo|hi|size|drawn|q|conf|s20|atr10|bo_ms|level|close|rest_s|start_known|ver|med  /  M|id|t|px|lo|hi  /  F|id|t|traded|px  /  E|id|t|end|traded  /  T|id|bar_end|FILLING or HELD|traded|resting  /  B|bar_end  /  A|id|t|speed_atr100|FAST/MID/DRIFT/NA  /  W|id|t|dwell_ms|EATEN or IN  /  S|id|t|px (stepped)  /  R|id|fill_ms|decide_ms|RE-BID or ACCEPTED|stack_lots|delta_30s|adds|cancels"; }
    // ---- the session record -----------------------------------------------------------------------------------------------------
    static std::string zRow(const Zone& z) {
        const Seg& g = z.segs.front(); char b[400];
        std::snprintf(b, sizeof(b), "Z|%d|%lld|%d|%s|%lld|%lld|%lld|%lld|%lld|%d|%d|%d|%lld|%lld|%lld|%lld|%lld|%lld|%d|%s", z.id, z.oid, z.side, kindCode(z.kind), z.t0,
            (long long)g.px, (long long)g.lo, (long long)g.hi, z.size0, z.drawn ? 1 : 0, z.q ? 1 : 0, z.conf ? 1 : 0, z.s20, (long long)std::llround(z.atr * 10.0),
            z.boMs, (long long)z.level, (long long)z.bclose, z.restMs / 1000, z.startKnown ? 1 : 0, VERSION);
        return std::string(b) + "|" + std::to_string(z.med);   // (1.2.0) + the median resting order at the start (22 fields; 21 = older)
    }
    // read back after a restart; live zones are re-linked to their order id (the book learns the order again from its next event)
    size_t restore(const std::vector<std::string>& in, long long* bad = nullptr) {
        size_t n = 0; long long nb = 0; std::map<int, int> idx;
        for (size_t i = 0; i < in.size(); ++i) {
            const std::vector<std::string> f = split(in[i], '|');
            if (f.empty() || f[0].empty()) continue;
            long long v[20] = { 0 };
            if (f[0] == "Z" && (f.size() == 21 || f.size() == 22)) {   // (1.2.0) 22 = + med; 21 = a 1.0 / 1.1 line
                bool ok = true; for (int k = 1; k <= 3 && ok; ++k) ok = numLL(f[(size_t)k], -1, 9223372036854775807LL, v[k]);
                for (int k = 5; k <= 19 && ok; ++k) ok = numLL(f[(size_t)k], -9223372036854775807LL, 9223372036854775807LL, v[k]);
                const int kind = kindOf(f[4]);
                long long med = 0; if (ok && f.size() == 22) ok = numLL(f[21], 0, 100000000, med);
                if (!ok || kind < 0 || (v[3] != BID && v[3] != ASK) || v[9] <= 0 || idx.count((int)v[1])) { ++nb; continue; }
                Zone z; z.id = (int)v[1]; z.oid = v[2]; z.side = (int)v[3]; z.kind = kind; z.t0 = v[5];
                Seg g; g.t = v[5]; g.px = v[6]; g.lo = v[7]; g.hi = v[8]; z.segs.push_back(g);
                z.size0 = z.sizeNow = z.maxShown = v[9]; z.drawn = v[10] == 1; z.q = v[11] == 1; z.conf = v[12] == 1; z.s20 = v[13]; z.atr = (double)v[14] / 10.0;
                z.boMs = v[15]; z.level = v[16]; z.bclose = v[17]; z.restMs = v[18] * 1000; z.startKnown = v[19] == 1; z.restored = true; z.med = med;
                idx[z.id] = (int)zones.size(); zones.push_back(z); usedOid.insert(z.oid); nextId = std::max(nextId, z.id + 1); ++n;
            } else if ((f[0] == "M" && f.size() == 6) || (f[0] == "F" && f.size() == 5) || (f[0] == "E" && f.size() == 5)) {
                long long id = 0, t = 0; if (!numLL(f[1], 0, 2000000000, id) || !numLL(f[2], 0, 9223372036854775807LL, t) || !idx.count((int)id)) { ++nb; continue; }
                Zone& z = zones[(size_t)idx[(int)id]];
                if (f[0] == "M") { long long p = 0, lo = 0, hi = 0; if (!numLL(f[3], -9223372036854775807LL, 9223372036854775807LL, p) || !numLL(f[4], -9223372036854775807LL, 9223372036854775807LL, lo) || !numLL(f[5], -9223372036854775807LL, 9223372036854775807LL, hi) || z.end != LIVE) { ++nb; continue; }
                    Seg g; g.t = t; g.px = p; g.lo = lo; g.hi = hi; z.segs.push_back(g); z.moves++; }
                else if (f[0] == "F") { long long tr = 0, p = 0; if (!numLL(f[3], 0, 100000000, tr) || !numLL(f[4], -9223372036854775807LL, 9223372036854775807LL, p)) { ++nb; continue; }
                    if (!z.tFill0) { z.tFill0 = t; }
                    z.tFill = t; z.traded = std::max(z.traded, tr); z.fillPx = p; }
                else { const int e = endOf(f[3]); long long tr = 0; if (e <= LIVE || z.end != LIVE || !numLL(f[4], 0, 100000000, tr)) { ++nb; continue; }
                    z.end = e; z.t1 = t; z.traded = std::max(z.traded, tr); }
            } else if (f[0] == "B" && f.size() == 2) {                 // (1.2.0) the last bar judged
                long long t = 0; if (!numLL(f[1], 0, 9223372036854775807LL, t)) { ++nb; continue; }
                judgedMs = std::max(judgedMs, t);
            } else if ((f[0] == "T" && f.size() == 6) || (f[0] == "A" && f.size() == 5) || (f[0] == "W" && f.size() == 5) || (f[0] == "S" && f.size() == 4)) {   // (1.2.0)
                long long id = 0, t = 0; if (!numLL(f[1], 0, 2000000000, id) || !numLL(f[2], 0, 9223372036854775807LL, t) || !idx.count((int)id)) { ++nb; continue; }
                Zone& z = zones[(size_t)idx[(int)id]];
                if (f[0] == "T") {
                    long long tr = 0, rs = 0; if (!numLL(f[4], 0, 100000000, tr) || !numLL(f[5], 0, 100000000, rs)) { ++nb; continue; }
                    if (f[3] == "FILLING" && !z.tagFillMs) { z.tagFillMs = t; ++tally.filling; }
                    else if (f[3] == "HELD" && !z.tagHeldMs) { z.tagHeldMs = t; ++tally.held; }
                    else { ++nb; continue; }
                    judgedMs = std::max(judgedMs, t);
                } else if (f[0] == "A") {
                    long long sp = 0; const int a = arrivalOf(f[4]); if (!numLL(f[3], -100000000, 100000000, sp) || a < 1 || z.tContact) { ++nb; continue; }
                    z.tContact = t; z.speed = (double)sp / 100.0; z.arrival = a; if (a == 1) ++tally.fast; else if (a == 3) ++tally.drift; else if (a == 2) ++tally.mid;
                } else if (f[0] == "W") {
                    long long d = 0; if (!numLL(f[3], 0, 9223372036854775807LL, d) || (f[4] != "EATEN" && f[4] != "IN") || z.dwellMs >= 0) { ++nb; continue; }
                    z.dwellMs = d; z.eaten = f[4] == "EATEN"; if (z.eaten) ++tally.eaten;
                } else {
                    long long p = 0; if (!numLL(f[3], -9223372036854775807LL, 9223372036854775807LL, p) || z.stepped) { ++nb; continue; }
                    z.stepped = true; ++tally.stepped;
                }
            } else if (f[0] == "R" && f.size() == 9) {                 // (1.3.0) the after-fill read
                long long id = 0, tf = 0, td = 0, st = 0, de = 0, ad = 0, ca = 0;
                const int rd = f[4] == "RE-BID" ? 1 : f[4] == "ACCEPTED" ? 2 : 0;
                if (!numLL(f[1], 0, 2000000000, id) || !numLL(f[2], 1, 9223372036854775807LL, tf) || !numLL(f[3], 1, 9223372036854775807LL, td) || rd == 0
                    || !numLL(f[5], -100000000000LL, 100000000000LL, st) || !numLL(f[6], -100000000000LL, 100000000000LL, de) || !numLL(f[7], 0, 100000000000LL, ad)
                    || !numLL(f[8], 0, 100000000000LL, ca) || td < tf || !idx.count((int)id)) { ++nb; continue; }
                Zone& z = zones[(size_t)idx[(int)id]];
                if (z.afRead) { ++nb; continue; }
                z.afFillMs = tf; z.afDecMs = td; z.afRead = rd; z.afStack = st; z.afDelta = de; z.afAdds = ad; z.afCans = ca;
                if (rd == 1) ++tally.rebid; else ++tally.accepted;
                afDone.insert(std::make_pair(z.side, z.pxAt(tf)));
            } else if (f[0].size() <= 2 && f[0][0] >= 'A' && f[0][0] <= 'Z' && std::string("ZMFEBTAWSR").find(f[0][0]) == std::string::npos) ++tally.unknownLines;   // (1.2.0) a line type from a newer build: skipped, not "bad"
            else ++nb;
        }
        retrack();
        live.clear(); usedOid.clear();
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].end != NEARED) usedOid.insert(zones[i].oid);
        for (size_t i = 0; i < zones.size(); ++i) if (zones[i].end == LIVE) {
            live.push_back((int)i); Zone& z = zones[i];
            auto it = book.find(z.oid);
            if (it == book.end()) { Ord o; o.side = z.side; o.px = z.px(); o.size = z.sizeNow; o.maxShown = z.maxShown; o.tFirst = z.t0 - z.restMs; o.tAtPx = z.segs.back().t; o.startKnown = false; o.moves = z.moves; o.zone = (int)i; book[z.oid] = o; }
            else it->second.zone = (int)i;
        }
        if (bad) *bad = nb;
        return n;
    }
    // (1.2.0) the contact / extreme / close-back tracking of every zone from the closed bars this engine already holds (the replay's
    // restart keeps its bars; in IRT the bars come again after the restore and judgeTags rebuilds the same tracking without deciding)
    void retrack() {
        for (size_t i = 0; i < zones.size(); ++i) {
            Zone& z = zones[i]; if (z.kind == K_FAR || z.segs.empty()) continue;
            z.contactBarMs = 0; z.extQ = 0; z.turnMs = 0;
            long long prev = 0;
            for (size_t k = 0; k < W.bars.size(); ++k) {
                const Bar& b = W.bars[k]; const long long pe = prev; prev = b.endMs;
                if (b.endMs <= z.t0 || (z.end != LIVE && z.t1 <= pe)) continue;
                const int sg = z.sg(); const Tick L = (Tick)sg * z.pxAt(b.endMs), w = z.w(R.tagAtr);
                const Tick hiQ = sg > 0 ? b.h : -b.l, cQ = (Tick)sg * b.c;
                const bool touched = hiQ >= L - w;
                if (touched && !z.contactBarMs) { z.contactBarMs = b.endMs; z.extQ = hiQ; }
                else if (z.contactBarMs) z.extQ = std::max(z.extQ, hiQ);
                if (touched && z.contactBarMs && z.extQ <= L + 2 * w && cQ < L - w && !z.turnMs) z.turnMs = b.endMs;
            }
        }
    }
    // ---- reads for the host -----------------------------------------------------------------------------------------------------
    // LMT F: every zone order (drawn or log-only) that traded, filled within the last `ms` (or still live)
    std::vector<int> filledZones(long long now, long long ms) const {
        std::vector<int> o; for (size_t i = 0; i < zones.size(); ++i) { const Zone& z = zones[i]; if (z.traded > 0 && (z.end == LIVE || now - z.tFill <= ms)) o.push_back((int)i); }
        return o;
    }
    long long liveSize(const Zone& z) const { auto it = book.find(z.oid); return it == book.end() ? z.sizeNow : it->second.size; }
private:
    void tagRow(const Zone& z, const char* tag, long long barEnd) {
        char r[160]; std::snprintf(r, sizeof(r), "T|%d|%lld|%s|%lld|%lld", z.id, barEnd, tag, z.traded, std::max(0LL, liveSize(z))); rows.push_back(r);
    }
    void dwellEnd(Zone& z, long long t) {
        z.dwellMs = std::max(0LL, t - z.tContact); z.eaten = z.dwellMs > (long long)R.eatenS * 1000LL; if (z.eaten) ++tally.eaten;
        char r[120]; std::snprintf(r, sizeof(r), "W|%d|%lld|%lld|%s", z.id, t, z.dwellMs, z.eaten ? "EATEN" : "IN"); rows.push_back(r);
    }
    void prune() {   // the book is full: drop the smallest half of the orders that hold no zone (counted)
        std::vector<long long> sz; sz.reserve(book.size()); for (auto& kv : book) if (kv.second.zone < 0) sz.push_back(kv.second.size);
        if (sz.empty()) return;
        std::nth_element(sz.begin(), sz.begin() + sz.size() / 2, sz.end()); const long long cut = sz[sz.size() / 2];
        for (auto it = book.begin(); it != book.end();) { if (it->second.zone < 0 && it->second.size <= cut) { it = book.erase(it); ++tally.bookDrop; } else ++it; }
    }
    void compact() { std::vector<int> n; for (size_t k = 0; k < live.size(); ++k) if (zones[(size_t)live[k]].end == LIVE) n.push_back(live[k]); live.swap(n); }
    Tick halfBand(double a) const { return std::max<Tick>(1, (Tick)std::llround(0.05 * a)); }
    int drawRule(int side, int kind, bool conf, bool& q) const {   // (1.2.0) one pooled rule: no "?", no gamma-only, bids never
        q = false; (void)conf;
        if (side == BID) return 0;
        if (kind == K_FAR) return R.farOn ? 1 : 0;
        if (kind == K_ABC) return R.abc ? 1 : 0;
        const int v = kind == K_PRINT ? R.print : kind == K_ORIGIN ? R.origin : R.nearO;
        return v == 0 ? 0 : 1;
    }
    void start(long long oid, const Ord& o, int kind, const Break* br, long long t, long long s20, double a, long long restMs) {
        Zone z; z.id = nextId++; z.oid = oid; z.side = o.side; z.kind = kind; z.t0 = t;
        z.size0 = z.sizeNow = o.size; z.maxShown = o.maxShown; z.traded = 0; z.s20 = s20; z.atr = a; z.restMs = t - o.tAtPx; z.startKnown = o.startKnown; z.moves = o.moves; z.med = medLots;
        const Tick hb = halfBand(a); Seg g; g.t = t; g.px = o.px; g.lo = o.px - hb; g.hi = o.px + hb;
        if (br && (kind == K_PRINT || kind == K_ABC)) { z.boMs = br->endMs; z.level = br->level; z.bclose = br->close;
            g.lo = std::min(g.lo, std::min(br->level, br->close)); g.hi = std::max(g.hi, std::max(br->level, br->close)); }
        if (br && kind == K_ORIGIN) { z.boMs = br->endMs; z.level = br->origin; }
        for (size_t i = 0; i < gamma.size(); ++i) if (std::fabs((double)(gamma[i] - o.px)) <= R.confAtr * a + 1e-9) { z.conf = true; z.confPx = gamma[i]; break; }
        bool q = false; z.drawn = drawRule(z.side, kind, z.conf, q) == 1; z.q = q;
        z.segs.push_back(g);
        const int idx = (int)zones.size(); zones.push_back(z); live.push_back(idx); usedOid.insert(oid);
        book[oid].zone = idx;
        tally.made[z.side][kind]++; if (z.drawn) tally.drawn++;
        rows.push_back(zRow(z));
        (void)restMs;
    }
    void finish(int idx, int how, long long t) {
        Zone& z = zones[(size_t)idx]; if (z.end != LIVE) return;
        z.end = how; z.t1 = t;
        auto it = book.find(z.oid); if (it != book.end()) { z.sizeNow = it->second.size; it->second.zone = -1; }
        tally.ends[how]++;
        if (z.tContact && z.dwellMs < 0) dwellEnd(z, t);   // (1.2.0) still inside the band when it ended
        if (how == NEARED) usedOid.erase(z.oid);   // a far order that came within 2 ATR may become a near zone (one live zone per order)
        char b[160]; std::snprintf(b, sizeof(b), "E|%d|%lld|%s|%lld", z.id, t, endCode(how), z.traded); rows.push_back(b);
    }
    void zoneMoved(int idx, const Ord& o, long long t) {
        Zone& z = zones[(size_t)idx]; if (z.end != LIVE) return;
        z.moves++;
        const Seg& g = z.segs.back();
        if (z.turnMs && !z.stepped && t > z.turnMs && (Tick)z.sg() * o.px < (Tick)z.sg() * g.px) {   // (1.2.0) LOG ONLY: re-priced toward price after the close back
            z.stepped = true; ++tally.stepped; char sb[120]; std::snprintf(sb, sizeof(sb), "S|%d|%lld|%lld", z.id, t, (long long)o.px); rows.push_back(sb); }
        const bool inBand = o.px >= g.lo && o.px <= g.hi;
        if (o.moves > R.maxMoves || !inBand) { finish(idx, MOVED, t); return; }
        Seg n = g; const Tick hb = halfBand(z.atr);
        if (z.kind == K_PRINT || z.kind == K_ABC) { n.lo = std::min(g.lo, o.px - hb); n.hi = std::max(g.hi, o.px + hb); } else { n.lo = o.px - hb; n.hi = o.px + hb; }
        n.t = t; n.px = o.px; z.segs.push_back(n);
        char b[200]; std::snprintf(b, sizeof(b), "M|%d|%lld|%lld|%lld|%lld", z.id, t, (long long)n.px, (long long)n.lo, (long long)n.hi); rows.push_back(b);
    }
};

// ---- text ---------------------------------------------------------------------------------------------------------------------
inline int decimals(double tick) {
    if (!(tick > 0) || !finiteD(tick)) return 2;
    for (int d = 0; d <= 6; ++d) { const double p = std::pow(10.0, d), v = tick * p; if (std::fabs(v - std::round(v)) < 1e-7) return d; }
    return 6;
}
inline std::string price(Tick t, double tick) { char b[48]; std::snprintf(b, sizeof(b), "%.*f", decimals(tick), (double)t * tick); return b; }
inline std::string clock12(long long ms) {
    if (ms <= 0) return "?";
    long long s = (ms / 1000) % 86400; if (s < 0) s += 86400;
    int h = (int)(s / 3600), m = (int)(s / 60 % 60); const bool pm = h >= 12; h %= 12; if (h == 0) h = 12;
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", h, m, pm ? "PM" : "AM"); return b;
}
inline std::string dur(long long ms) {
    if (ms < 0) { ms = 0; }
    const long long s = ms / 1000; char b[32];
    if (s < 60) std::snprintf(b, sizeof(b), "%lld s", s); else if (s < 3600) std::snprintf(b, sizeof(b), "%lld min", s / 60);
    else std::snprintf(b, sizeof(b), "%lld h %lld min", s / 3600, s / 60 % 60);
    return b;
}
// (1.2.0) the chart label, LEFT of the line start: "LMT <lots> (<KIND>)"; a far order keeps its size only (as before)
inline std::string label(const Zone& z) {
    if (z.kind == K_FAR) return std::to_string(z.size0);
    return "LMT " + std::to_string(z.size0) + " (" + kindCode(z.kind) + ")";
}
// (1.2.0) the state tag texts; (1.3.0) HELD is logged only - the drawn tags are FILLING (grey) and the after-fill read
inline const char* tagText(bool held) { return held ? "HELD?" : "FILLING"; }
inline const char* afterText(int read) { return read == 1 ? "RE-BID" : read == 2 ? "ACCEPTED" : ""; }
struct Row { std::string text; int ink = INK_WHITE; };
// (1.3.0) the hover's example row (real cases from the replay of the research MBO; see NOTES-LP0540.md)
static const char* const EXAMPLE_ROW = "Example: HG Oct 22 12:20 PM offer 10 filled -> RE-BID, back 5 ATR. NG Oct 14 bid 40 filled -> ACCEPTED, fell 6 ATR.";
struct HoverCtx { std::string market; double tick = 0.25; long long nowMs = 0; long long sizeLive = 0; };
// (1.2.0) <= 12 rows, <= 118 chars: what it is (a target / magnet, not a wall) / question / what / why / the honest odds / how / example /
// now (lots, x median, age at contact, traded so far with % of shown, status resting / filling / pulled / held)
inline std::vector<Row> hoverRows(const Zone& z, const HoverCtx& c) {
    std::vector<Row> v; const bool ask = z.side == ASK;
    const std::string pxs = price(z.px(), c.tick), lots = std::to_string(z.size0);
    v.push_back(Row{ label(z) + (z.kind == K_FAR ? " lots (FAR)" : "") + ": " + (ask ? "sell" : "buy") + " order @ " + pxs + " - a TARGET / magnet, not a wall"
        + (z.drawn ? "" : "  (log only)"), INK_WHITE });
    v.push_back(Row{ "Question: where is big size resting that price is drawn to - and is it still there once price trades in?", INK_WHITE });
    std::string xm = z.med > 0 ? " = " + std::to_string((long long)std::llround((double)z.size0 / (double)z.med)) + "x the median resting order (" + std::to_string(z.med) + " lots)" : " (>= 3x the median resting order)";
    std::string age;
    const long long born = z.t0 - z.restMs;
    if (z.tContact > 0) age = "age at contact " + std::string(z.startKnown ? "" : "at least ") + dur(z.tContact - born);
    else age = "not reached yet, resting " + std::string(z.startKnown ? "" : "at least ") + dur((c.nowMs > z.t0 ? c.nowMs : z.t0) - born);
    v.push_back(Row{ "What: ONE resting " + std::string(ask ? "sell" : "buy") + " order of " + lots + " lots" + xm + "; " + age + ".", INK_WHITE });
    if (z.kind == K_PRINT) v.push_back(Row{ "It sits where the with-trend " + std::string(ask ? "breakdown" : "breakout") + " printed (" + clock12(z.boMs) + ", level " + price(z.level, c.tick) + ").", INK_WHITE });
    else if (z.kind == K_ABC) v.push_back(Row{ "It sits in an internal (ABC) break at " + clock12(z.boMs) + " - a break against the trend, inside the wave.", INK_WHITE });
    else if (z.kind == K_ORIGIN) v.push_back(Row{ "It sits at the origin of the breakdown wave (" + price(z.level, c.tick) + ").", INK_WHITE });
    v.push_back(Row{ "Why: 57% of big orders at turns end FILLED and price goes through - the size is the liquidity the attack wants.", INK_WHITE });
    v.push_back(Row{ "Odds: reverses here 30% vs 35% at a random price. Once filled: RE-BID sweeps failed 53%, ACCEPTED 39%.", INK_WHITE });
    v.push_back(Row{ "How: never fade the touch. FILLING = being filled. X = pulled before contact - no read either way.", INK_WHITE });
    v.push_back(Row{ "   30 s after it fills: RE-BID = new size stacked beyond + delta back = sweep failing; else ACCEPTED = through.", INK_WHITE });
    if (z.conf) v.push_back(Row{ "Diamond: the order also sits on a gamma level (" + price(z.confPx, c.tick) + ").", INK_YELLOW });
    v.push_back(Row{ EXAMPLE_ROW, INK_WHITE });
    const long long shownMax = std::max(z.maxShown, z.size0), pct = shownMax > 0 ? (long long)std::llround(100.0 * (double)z.traded / (double)shownMax) : 0;
    const std::string traded = "traded " + std::to_string(z.traded) + " (" + std::to_string(pct) + "% of " + std::to_string(shownMax) + " shown)";
    std::string tags; if (z.tagFillMs) tags += " | FILLING " + clock12(z.tagFillMs);
    if (z.afRead) tags += std::string(" | ") + afterText(z.afRead) + " " + clock12(z.afDecMs) + " (stack " + std::to_string(z.afStack) + ", delta " + std::to_string(z.afDelta) + ")";
    else if (z.afFillMs) tags += " | filled " + clock12(z.afFillMs) + " - read at +30 s";
    std::string now; int ink = INK_WHITE;
    if (z.end == LIVE) {
        const char* st = z.tagFillMs ? "FILLING" : "RESTING";
        now = "Now: " + std::string(st) + " - resting " + std::to_string(c.sizeLive) + " lots | " + traded + tags + ".";
        ink = st[0] == 'F' ? INK_GREY : INK_WHITE;
    } else {
        now = std::string("Ended ") + clock12(z.t1) + ": " + (z.end == FILLED ? "FILLED - price went through" : z.end == PULLED ? (z.tContact ? "PULLED (X)" : "PULLED before contact (X) - no read")
            : z.end == MOVED ? "MOVED away (X) - void" : z.end == BROKEN ? "price closed 1 ATR through it" : z.end == EXPIRED ? "4 h old" : z.end == NEARED ? "price came within 2 ATR"
            : z.end == SHRANK ? "shrank under 2.5x the median" : z.end == LEFT ? "price moved away" : "session end") + " | " + traded + tags + ".";
        ink = z.end == PULLED || z.end == MOVED || z.end == FILLED ? INK_GREY : INK_WHITE;
        if (z.afRead == 1) ink = ask ? INK_RED : INK_GREEN;   // (1.3.0) RE-BID: the reversal side's colour
    }
    v.push_back(Row{ now, ink });
    if (v.size() > 12) v.resize(12);
    for (size_t i = 0; i < v.size(); ++i) if (v[i].text.size() > 118) v[i].text = v[i].text.substr(0, 115) + "...";
    return v;
}

// ---- the per-market settings file: lsFlexLevels\LiquidityProfile.lmtzones-<MKT>.txt -----------------------------------------
// "LMZ3,on,print,origin,near,far,abc,gamma_lines,diamond,offer_rgb,bid_rgb,grey_rgb,gamma_rgb,after" (colours as decimal 0xRRGGBB);
// (1.3.0) the last field = the after-fill tags (RE-BID / ACCEPTED). An older LMZ2 (its last field was the 1.2.0 state tags) or LMZ1 line
// still loads with the after-fill tags ON (the default; the box means something new). Colours since 1.3.0: offer / bid = the RE-BID text
// (red offer swept above = bearish, green bid swept below = bullish), grey = every zone line, its label, FILLING, ACCEPTED and the X.
static const long long C_OFFER = 0xEF4444, C_BID = 0x22C55E, C_GREYZ = 0x8A8F98, C_GAMMA = 0x6B7280;   // IRT COLOR = 0xRRGGBB (as C_BUY 0x22C55E)
static const long long C_DIAMOND = 0xFACC15;
struct Settings { bool on = true, print = true, origin = true, nearO = true, farOn = true, abc = true, gammaLines = true, diamond = true, tags = true;
    long long cOffer = C_OFFER, cBid = C_BID, cGrey = C_GREYZ, cGamma = C_GAMMA; bool ok = false; std::string why; };
inline bool parseSettings(const std::string& line, Settings& out) {
    const std::vector<std::string> f = split(line, ',');
    const bool v1 = f.size() == 13 && f[0] == "LMZ1", v2 = f.size() == 14 && f[0] == "LMZ2", v3 = f.size() == 14 && f[0] == "LMZ3";
    if (!v1 && !v2 && !v3) return false;
    Settings s; long long v = 0; bool* b[8] = { &s.on, &s.print, &s.origin, &s.nearO, &s.farOn, &s.abc, &s.gammaLines, &s.diamond };
    for (int i = 0; i < 8; ++i) { if (!numLL(f[(size_t)i + 1], 0, 1, v)) return false; *b[i] = v == 1; }
    long long* c[4] = { &s.cOffer, &s.cBid, &s.cGrey, &s.cGamma };
    for (int i = 0; i < 4; ++i) { if (!numLL(f[(size_t)i + 9], 0, 0xFFFFFF, v)) return false; *c[i] = v; }
    if (v2 || v3) { if (!numLL(f[13], 0, 1, v)) return false; if (v3) s.tags = v == 1; }   // (1.3.0) LMZ2's box was the old state tags: after-fill = on
    s.ok = true; out = s; return true;
}
inline std::string settingsLine(const Settings& s) {
    char b[220]; std::snprintf(b, sizeof(b), "LMZ3,%d,%d,%d,%d,%d,%d,%d,%d,%lld,%lld,%lld,%lld,%d", s.on, s.print, s.origin, s.nearO, s.farOn, s.abc, s.gammaLines, s.diamond, s.cOffer, s.cBid, s.cGrey, s.cGamma, s.tags);
    return b;
}
// does the chart's setting let this zone be drawn (the market rule already decided z.drawn)
inline bool shown(const Zone& z, const Settings& s) {
    if (!s.on || !z.drawn) return false;
    switch (z.kind) { case K_PRINT: return s.print; case K_ABC: return s.abc; case K_ORIGIN: return s.origin; case K_FAR: return s.farOn; default: return s.nearO; }
}
// (1.2.0) FILLING is drawn with the zone line (1.3.0: no longer behind the tags box - that box is the after-fill read now); never on a far line
inline bool tagsShown(const Zone& z, const Settings& s) { return z.kind != K_FAR && shown(z, s); }
// (1.3.0) is this zone's after-fill read drawn: decided, the after-fill box ticked, the zone's kind box ticked; a drawn zone (offers) OR a
// BID zone (bids have no line - the read is proven on both sides, 54% vs 39% bids / 51% vs 39% offers) in a market with a draw rule
inline bool afterShown(const Zone& z, const Settings& s, bool bidsOk) {
    if (!s.on || !s.tags || z.kind == K_FAR || z.afRead == 0) return false;
    if (shown(z, s)) return true;
    if (z.side != BID || !bidsOk) return false;
    switch (z.kind) { case K_PRINT: return s.print; case K_ABC: return s.abc; case K_ORIGIN: return s.origin; default: return s.nearO; }
}
} // namespace lmz
#endif
