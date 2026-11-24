#ifndef LS_BO3H_LOGIC_H
#define LS_BO3H_LOGIC_H
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>
// =================================================================================================================================
// Bo3hLogic.h -- b3h 1.0.0: the 3-HOUR BO / Tst of the indicator "3h BO" (ls3hBO 1.0.0, Bo3h.cpp). SDK-free. Rassul 2026-11-23 19:23-19:34 CT on a GC 180-minute chart: "the 3hr bo just looks
// for breaks of pivots and then their test . so the idea is to look for an expanding range break through a pivot and then its test and
// label"; "1. bigger than the prior bar"; "actually you are right , the close should take out the pivot"; "3. test is like 3min test
// rules"; "it should have been drawn" (the Tst on the leg still running); "38.2% minimum pullback is not required here because of the atr
// rule" ... "the test needs to be .33 at of the pivot break"; "yes both" (overlap AND 1/3 ATR); "build"; 20:17 "rename the bo indicator to  3h BO", 20:20 "im talking about the htf bo right .. dont mess up
// the 3min bo indicator", 20:25 "1" (its own indicator). Mockup breakout-3h-mockup.html v5.
// Bullish shown, bearish = mirror. Used ONLY on a 10800-second chart. lsBreakout (the 3-minute BO) is NOT involved: it is as it was (1.29.0).
//   ZigZag  his 1-bar High / Low ZigZag (the walk of HtfSr.h / lsCobSignal): pivots confirmed by CLOSED candles.
//   BO      a closed candle CLOSES above a confirmed pivot high (one of the last two not yet closed through) and its range is bigger than
//           the candle before. A pivot stays live until a candle closes beyond it; closed through by a candle that is not bigger = no BO.
//   Tst     a pivot LOW after the BO move topped (the first pivot high at / after the BO candle): its candle OVERLAPS the BO candle; it
//           comes back to the broken pivot or stops less than 1/3 of the 14-bar Wilder ATR (at the BO candle) short of it; it does not go
//           beyond the move's origin (the last pivot low at / before the BO); it never takes out the BO candle's low (then that BO gets no
//           more Tsts); at most 2 per BO, the 2nd lower than the 1st; none after a close below the origin or a BD. One Tst per pivot.
//           NO 38.2% minimum and NO follow-through candle. Drawn LIVE: the low of the leg still running counts as a pivot while it
//           lasts (live = true: it can move or vanish); final once a closed candle confirms the pivot.
namespace b3h {
static const char* const VERSION = "1.1.0";   // (1.1.0, Rassul 2026-11-23 23:06 CT "update the bo on the htf that we did to include sp and ut .  the code can be found in these two indicators" - his IRT signals sot_mom_div_up_basic (UT) / sot_mom_div_dn_basic (SP), 23:10 "go"): + SP / UT, see spUt() below
// was static const char* const VERSION = "1.0.0";
static const int PERIOD = 10800, ATR_LEN = 14, TST_MAX = 2, MAX_BARS = 1500;
static const double ATR_FRAC = 1.0 / 3.0;
struct Bar { double h = 0, l = 0, c = 0; };
struct Mark { int bar = 0; char code = 0; int dir = 0; bool live = false; double lv = 0; };   // code 'B' = BO / BD, 'T' = Tst, (1.1.0) 'S' = SP (dir +1), 'U' = UT (dir -1); dir +1 bullish; lv = the broken / tested pivot's price
struct Piv { int bar = 0, side = 0, cf = 0; bool live = false; };
inline const char* text(const Mark& m) { return m.code == 'T' ? "Tst" : m.code == 'S' ? "SP" : m.code == 'U' ? "UT" : m.dir > 0 ? "BO" : "BD"; }
inline bool above(const Mark& m) { return m.code == 'B' ? m.dir > 0 : m.dir < 0; }   // BO above, BD below; Tst / SP / UT on the pivot's own side
// ---------------------------------------------------------------------------------------------------------------------------------
// (1.1.0) SP / UT = his IRT signals sot_mom_div_dn_basic (SP, bullish) / sot_mom_div_up_basic (UT, bearish), on the same 1-bar ZigZag,
// decided on the CLOSED candle that confirms the pivot (his "Uppivot" = Ups#1 = 1), drawn on the pivot candle, never repainted.
// UT at a new pivot high H1 (the pivot highs before it H2, H3, H4 newest first; L1 = the pivot low between H2 and H1, L2 the one before;
// C / TR = the close / true range of the confirming candle; M(x) = the highest MOM(MOM_LEN) from the pivot low before pivot x to x):
//   1. H1 >= H2, the H1 candle's low <= H2, and (H1 - H2 < 0.5 x (H2 - L1)  or  C < H2  or  M(H1) < M(H2))
//   2. H1 >= H3, the H1 candle's low <= H3, and (H1 - H3 < 0.5 x (H3 - L2)  or  C < H3  or  M(H1) < M(H3)), with H1 > H2 and H2 < H3
//   3. H1 >= H4, the H1 candle's low <= H4, H1 - H4 <= TR, with H4 > H2 and H4 > H3
// SP = the mirror at pivot lows (lows lower, MOM's lowest, C > the low).
static const int MOM_LEN = 10;   // IRT's MOM = close - close MOM_LEN bars back (his MOM setting: to confirm)
// B = the chart's candles, oldest first; the LAST one is the forming candle (never decides a BO; it only walks the live leg).
inline std::vector<Mark> run(const std::vector<Bar>& B) {
    std::vector<Mark> out; const int n = (int)B.size(); if (n < 4) return out;
    for (const Bar& b : B) if (!std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(b.c) || b.h < b.l) return out;
    const int closed = n - 1;                                   // bars [0, closed) are closed
    std::vector<Piv> piv; int d = 0, ext = 0;
    auto step = [&](int i, int& dd, int& ee, std::vector<Piv>& to, bool live) {
        if (dd == 0) { dd = B[1].h >= B[0].h ? 1 : -1; ee = 0; }
        if (dd == 1) { if (B[(size_t)i].h > B[(size_t)ee].h) ee = i; else if (B[(size_t)i].l < B[(size_t)i - 1].l && i != ee) { Piv p; p.bar = ee; p.side = 1; p.cf = i; p.live = live; to.push_back(p); ee = i; dd = -1; } }
        else         { if (B[(size_t)i].l < B[(size_t)ee].l) ee = i; else if (B[(size_t)i].h > B[(size_t)i - 1].h && i != ee) { Piv p; p.bar = ee; p.side = -1; p.cf = i; p.live = live; to.push_back(p); ee = i; dd = 1; } }
    };
    for (int i = 1; i < closed; ++i) step(i, d, ext, piv, false);
    {   // (1.1.0) SP / UT on the confirmed pivots (piv holds confirmed pivots only here, alternating, oldest first)
        std::vector<double> mom((size_t)n, NAN); for (int i = MOM_LEN; i < closed; ++i) mom[(size_t)i] = B[(size_t)i].c - B[(size_t)i - MOM_LEN].c;
        auto legMax = [&](int from, int to, int sd) {    // highest (sd +1) / lowest (sd -1) MOM over (from, to]; NAN if none
            double v = NAN; for (int k = std::max(0, from + 1); k <= to; ++k) { const double x = mom[(size_t)k]; if (!std::isfinite(x)) continue; if (!std::isfinite(v) || (sd > 0 ? x > v : x < v)) v = x; } return v; };
        const int np = (int)piv.size();
        for (int k = 0; k < np; ++k) {
            const Piv& P = piv[(size_t)k]; const int sd = P.side;           // +1 pivot high -> UT, -1 pivot low -> SP
            std::vector<int> same, opp;                                      // newest first: same-side pivots before P, opposite pivots before P
            for (int j = k - 1; j >= 0; --j) (piv[(size_t)j].side == sd ? same : opp).push_back(j);
            if (same.empty() || opp.empty()) continue;
            auto px = [&](int j) { return sd > 0 ? B[(size_t)piv[(size_t)j].bar].h : B[(size_t)piv[(size_t)j].bar].l; };
            auto prevOpp = [&](int j) { int r = -1; for (int q = j - 1; q >= 0; --q) if (piv[(size_t)q].side != sd) { r = piv[(size_t)q].bar; break; } return r; };
            const double X1 = sd > 0 ? B[(size_t)P.bar].h : B[(size_t)P.bar].l, far1 = sd > 0 ? B[(size_t)P.bar].l : B[(size_t)P.bar].h;
            const Bar& Cf = B[(size_t)P.cf]; const double C = Cf.c;
            const double TR = std::max(Cf.h - Cf.l, std::max(std::fabs(Cf.h - B[(size_t)P.cf - 1].c), std::fabs(Cf.l - B[(size_t)P.cf - 1].c)));
            const double M1 = legMax(prevOpp(k), P.bar, sd);
            auto beyond = [&](double a, double b) { return sd > 0 ? a >= b : a <= b; };       // a at / beyond b on P's side
            auto overl = [&](double lvl) { return sd > 0 ? far1 <= lvl : far1 >= lvl; };      // P's candle trades back to lvl
            auto back = [&](double lvl) { return sd > 0 ? C < lvl : C > lvl; };              // the confirming candle closes back inside
            auto weaker = [&](int j) { const double Mj = legMax(prevOpp(j), piv[(size_t)j].bar, sd); return std::isfinite(M1) && std::isfinite(Mj) && (sd > 0 ? M1 < Mj : M1 > Mj); };
            auto oppPx = [&](int q) { const int b = piv[(size_t)opp[(size_t)q]].bar; return sd > 0 ? B[(size_t)b].l : B[(size_t)b].h; };
            bool hit = false; double lv = 0;
            if (same.size() >= 1) { const int j = same[0]; const double X2 = px(j);
                if (beyond(X1, X2) && overl(X2) && (std::fabs(X1 - X2) < 0.5 * std::fabs(X2 - oppPx(0)) || back(X2) || weaker(j))) { hit = true; lv = X2; } }
            if (!hit && same.size() >= 2 && opp.size() >= 2) { const int j = same[1]; const double X2 = px(same[0]), X3 = px(j);
                if (beyond(X1, X3) && overl(X3) && (std::fabs(X1 - X3) < 0.5 * std::fabs(X3 - oppPx(1)) || back(X3) || weaker(j))
                    && (sd > 0 ? X1 > X2 && X2 < X3 : X1 < X2 && X2 > X3)) { hit = true; lv = X3; } }
            if (!hit && same.size() >= 3) { const double X2 = px(same[0]), X3 = px(same[1]), X4 = px(same[2]);
                if (beyond(X1, X4) && overl(X4) && std::fabs(X1 - X4) <= TR && (sd > 0 ? X4 > X2 && X4 > X3 : X4 < X2 && X4 < X3)) { hit = true; lv = X4; } }
            if (hit) { Mark m; m.bar = P.bar; m.code = sd > 0 ? 'U' : 'S'; m.dir = -sd; m.lv = lv; out.push_back(m); }
        }
    }
    std::vector<double> atr((size_t)n, NAN);
    { bool have = false; double y = 0; for (int i = 1; i < closed; ++i) { double tr = B[(size_t)i].h - B[(size_t)i].l; const double pc = B[(size_t)i - 1].c; tr = std::max(tr, std::max(std::fabs(B[(size_t)i].h - pc), std::fabs(B[(size_t)i].l - pc)));
        if (!have) { y = tr; have = true; } else y += (tr - y) / ATR_LEN; atr[(size_t)i] = i >= ATR_LEN ? y : NAN; } }
    struct Bo { int i = 0, dir = 0; double lv = 0; int n = 0; bool dead = false, has = false; double last = 0; };
    std::vector<Bo> bos;
    {   std::vector<std::pair<int, int>> alive; size_t pi = 0;      // (pivot bar, side): confirmed and not yet closed through
        for (int i = 1; i < closed; ++i) {
            while (pi < piv.size() && piv[pi].cf < i) { alive.push_back(std::make_pair(piv[pi].bar, piv[pi].side)); ++pi; }
            for (int sd = 1; sd >= -1; sd -= 2) {
                int c1 = -1, c2 = -1; for (const auto& a : alive) if (a.second == sd) { c2 = c1; c1 = a.first; }   // the last two of this side (alive is in bar order)
                bool hit = false; double lv = 0;
                for (int q = 0; q < 2; ++q) { const int pb = q ? c2 : c1; if (pb < 0) continue; const double px = sd > 0 ? B[(size_t)pb].h : B[(size_t)pb].l;
                    if (sd > 0 ? B[(size_t)i].c > px : B[(size_t)i].c < px) { lv = !hit ? px : sd > 0 ? std::max(lv, px) : std::min(lv, px); hit = true; } }
                for (size_t a = alive.size(); a-- > 0;) if (alive[a].second == sd) { const double px = sd > 0 ? B[(size_t)alive[a].first].h : B[(size_t)alive[a].first].l;
                    if (sd > 0 ? B[(size_t)i].c > px : B[(size_t)i].c < px) alive.erase(alive.begin() + (long)a); }
                if (hit && (B[(size_t)i].h - B[(size_t)i].l) > (B[(size_t)i - 1].h - B[(size_t)i - 1].l)) {
                    Bo b; b.i = i; b.dir = sd; b.lv = lv; bos.push_back(b); Mark m; m.bar = i; m.code = 'B'; m.dir = sd; m.lv = lv; out.push_back(m); }
            }
        }
    }
    {   int d2 = d, e2 = ext; step(closed, d2, e2, piv, true);     // the forming candle, as the live chart walks it
        if (d2 != 0) { Piv p; p.bar = e2; p.side = d2; p.cf = closed; p.live = true; piv.push_back(p); } }   // the leg still running: its extreme so far
    std::vector<int> used;                                          // pivot bars that already carry a Tst
    for (const Piv& P : piv) {
        for (Bo& b : bos) {
            if (b.dead || P.side != -b.dir || P.bar <= b.i) continue;
            const int sd = b.dir, i0 = b.i, p = P.bar;
            bool other = false; for (const Bo& q : bos) if (q.i > i0 && q.dir == -sd && q.i <= P.cf) { other = true; break; }
            if (other) { if (!P.live) b.dead = true; continue; }
            int top = -1, org = -1;
            for (const Piv& q : piv) { if (q.side == sd && q.bar >= i0 && q.bar < p && top < 0) top = q.bar; if (q.side == -sd && q.bar <= i0) org = q.bar; }
            if (top < 0 || org < 0) continue;
            const Bar& X = B[(size_t)p]; const Bar& K = B[(size_t)i0];
            bool brokeOrg = false; for (int k = i0; k <= p && !brokeOrg; ++k) brokeOrg = sd > 0 ? B[(size_t)k].c < B[(size_t)org].l : B[(size_t)k].c > B[(size_t)org].h;
            const bool tookOut = sd > 0 ? X.l < K.l : X.h > K.h;
            if (brokeOrg || tookOut) { if (!P.live) b.dead = true; continue; }
            const double px = sd > 0 ? X.l : X.h, a = atr[(size_t)i0];
            const bool overl = X.l <= K.h && X.h >= K.l;
            const bool inMove = sd > 0 ? px >= B[(size_t)org].l : px <= B[(size_t)org].h;
            const bool nearPiv = std::isfinite(a) && (sd > 0 ? px - b.lv : b.lv - px) <= a * ATR_FRAC;
            const bool deeper = !b.has || (sd > 0 ? px < b.last : px > b.last);
            bool taken = false; for (int u : used) if (u == p) taken = true;
            if (!(overl && inMove && nearPiv && deeper) || b.n >= TST_MAX || taken) continue;
            Mark m; m.bar = p; m.code = 'T'; m.dir = sd; m.live = P.live; m.lv = b.lv; out.push_back(m); used.push_back(p);
            if (!P.live) { ++b.n; b.has = true; b.last = px; }
            break;
        }
    }
    return out;
}
} // namespace b3h
#endif
