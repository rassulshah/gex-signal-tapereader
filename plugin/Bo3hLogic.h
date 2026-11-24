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
static const char* const VERSION = "1.0.0";
static const int PERIOD = 10800, ATR_LEN = 14, TST_MAX = 2, MAX_BARS = 1500;
static const double ATR_FRAC = 1.0 / 3.0;
struct Bar { double h = 0, l = 0, c = 0; };
struct Mark { int bar = 0; char code = 0; int dir = 0; bool live = false; double lv = 0; };   // code 'B' = BO / BD, 'T' = Tst; dir +1 bullish; lv = the broken pivot's price
struct Piv { int bar = 0, side = 0, cf = 0; bool live = false; };
inline const char* text(const Mark& m) { return m.code == 'T' ? "Tst" : m.dir > 0 ? "BO" : "BD"; }
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
