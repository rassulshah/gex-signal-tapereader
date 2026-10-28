#ifndef LS_COLUMNBOARD_H
#define LS_COLUMNBOARD_H
// ColumnBoard.h 1.6.0 (1.6.0, 2026-10-28: Rassul 19:08 / 19:25 CT layout - the Liquidity (Levels) column at the pane's LEFT edge; on the right, out from the
// candles: gap, BIG = LMT F, Delta (READ strip + profile), Dealer, axis) 1.5.0 (1.5.0, 2026-10-27: the Delta column at the pane's LEFT edge) 1.0.0 (2026-10-14, CR5200) -- the right-side columns of the price chart, shared by lsDealerProfile 2.11.0,
// lsDeltaProfile 2.8.0 and lsLiquidityProfile 0.16.0. SDK-free; tested by test_columnboard_logic.cpp.
//
// Rassul approved 2026-10-14 11:00 CT (mockup-columns.png, 10:53): right of the last bar, left to right: a 20 px gap, the Levels
// column (default 40 px), the Delta Profile column (80), the Dealer Profile = gamma column (80), then the price axis. Price never
// enters the columns: the chart's right margin = gap + levels + delta + dealer, set by ONE plugin (the Dealer Profile; the Delta
// Profile only when no Dealer Profile is on the chart) and recomputed when any width changes. The Liquidity Profile stays on the LEFT
// in its own solid column (60). Every column clamps to its box - no column ever draws into another.
//
//   widths   each plugin's own Width setting (its settings window, kept per market in its own settings file); the ranges are here.
//   board    every plugin publishes its column width for THIS chart in the process environment (as BannerBoard.h / HoverBoard.h):
//            LRA_COL<c>_<symbol>_<spb>_<chart> = "<ms>|<width>"; the others read it each draw. bb::Beat decides whether the other
//            plugin is still on the chart (stamp moving, < 5 s old, or < 3 own draws without a new stamp), so a removed column
//            closes within seconds and an idle chart never flickers on its first repaint.
//   layout   the boxes from the axis edge leftward; on a chart too narrow for them the columns shrink in proportion (never under
//            their minimum) so at least MIN_PRICE_PX of price stays visible.
//   margin   the right margin in IRT's unit: pixels, or bars (px / pixels-per-bar, rounded up). The unit is checked against the
//            chart (the measured gap between the last bar and the axis vs the margin value) and logged; it grows at once and
//            shrinks only after 60 s (bp::StableW), so nothing shakes.
//   stack    1-D stacking of the level badges: each badge as near its line as it can be, in price order, never overlapping, kept
//            inside the pane; a badge that cannot fit is dropped (counted), never drawn over another.
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "BoxPlace.h"
#include "BannerBoard.h"

namespace cb {
static const char* const VERSION = "1.9.1";   // (1.9.1, 2026-10-28 12:50 CT Rassul "the hod lod should be on the right of the level column not in it. and it should be on the bottom left next to the levels column") dockKeys: lsHodLod's table docks at the BOTTOM-LEFT of the price area, immediately RIGHT of the Key Levels column (left = column right + 4, bottom = pane bottom - 6); the column keeps its full height
// was 1.9.0: // (1.9.0, 2026-10-28 12:16-12:18 CT Rassul "i want a column for levels and move all the level badges in the levels column because the chart is too messy. also move the hod lod box aligned with the liquidity profile on the left") the KEY LEVELS column (lsDealerProfile 2.33.0) at the pane's LEFT edge: its own board key LRA_COLV_, leftKeep keeps every plugin right of it (lsHodLod docks INSIDE it, leftKeep(..., false))
// was 1.8.0:   // (1.8.0, 2026-10-28 06:42 CT Rassul "keep the liquidity profile behind the delta profile", option R) the Levels / liquidity column moves to the RIGHT as its own column after the Delta Profile: gap, BIG = LMT F, Delta (READ + bars), Levels (liquidity top 10), Dealer, axis; the left side is free (leftKeep = the pane edge)   // (1.7.0, 2026-10-28 overnight, live 22:06 overlap on the left column) leftKeep: the one rule every plugin uses to keep its marks / boxes right of the left columns;   // (1.6.0, 2026-10-28) Levels (liquidity) LEFT, Delta back on the right after LMT F;   // (1.5.0, 2026-10-27 14:00 CT) the Delta column at the pane's LEFT edge (DELTA_LEFT);   // (1.4.0, 2026-10-27 DP 2.28.0) Levels column up to 460 px (the combined Levels + Liquidity column);   // (1.3.0, 2026-10-21 DP 2.19.0) Levels column up to 250 px: the level % block "33%  /  66%" sits after the liquidity ratio;   // (1.2.0) order: gap, Levels, BIG, Delta, Dealer (BIG next to the Delta Profile);   // (1.1.0, Rassul 2026-10-15) the BIG column (Liquidity Profile big orders) first after the last bar: gap, BIG, Levels, Delta, Dealer
static const int GAP_PX = 20;            // between the last bar and the Levels column
static const int MIN_PRICE_PX = 160;     // price area kept on a narrow chart
// (1.5.0, 2026-10-27 14:00 CT Rassul "to save space move the delta profile to the left") the Delta Profile column sits at the LEFT
// edge of the price pane (over the oldest bars, solid background) and no longer widens the right margin
static const bool DELTA_LEFT = false;   // (1.6.0) the Delta column is back on the right (READ strip + bars pointing toward price)
// (1.6.0, 2026-10-28, Rassul 19:08 / 19:25 CT "on the left we have liquidity profile top 10 levels ... on the right we have LMTF then delta profile")
// the Levels column (lsDealerProfile's liquidity top 10) sits at the LEFT edge of the price pane on a solid background, bars toward price
static const bool LEVELS_LEFT = false;   // (1.8.0) back on the right
// (1.8.0, option R) the Levels (liquidity top 10) column sits OUTSIDE the Delta column, next to the Dealer column / axis
static const bool LEVELS_OUTER = true;
enum Col { LIQ = 0, LEVELS = 1, DELTA = 2, DEALER = 3, BIG = 4, NCOL = 5 };
struct Range { int lo, hi, def; };
inline Range range(int c)
{
    switch (c) {
    case LIQ: return Range{ 20, 200, 60 };
    case LEVELS: return Range{ 30, 460, 78 };   // (1.4.0, DP 2.28.0) hi 250 -> 460: the combined Levels + Liquidity column (rank | liquidity | level chips | price | % block) publishes 420   // (1.3.0) hi 150 -> 250 (DP 2.19.0 publishes 150 + the % block); the setting stays 30-150   // (Rassul 11:39 CT) 40 -> 78 so "LonLO | HL 1.5K" fits
    case DELTA: return Range{ 40, 250, 80 };
    case BIG: return Range{ 30, 180, 44 };   // (LP 0.37.0, Rassul 13:23) three narrow columns LMT F | MKT | LMT R = 168
    default: return Range{ 40, 250, 80 };
    }
}
inline bool widthOk(int c, long long v) { const Range r = range(c); return v >= r.lo && v <= r.hi; }
inline int cleanWidth(int c, long long v) { return widthOk(c, v) ? (int)v : range(c).def; }
// a whole number 1-4 digits, nothing else (garbage, signs, spaces, decimals refused)
inline bool parseWidth(int c, const std::string& t, int& out)
{
    if (t.empty() || t.size() > 4) return false;
    for (size_t i = 0; i < t.size(); ++i) if (t[i] < '0' || t[i] > '9') return false;
    const long long v = std::strtoll(t.c_str(), nullptr, 10);
    if (!widthOk(c, v)) return false;
    out = (int)v; return true;
}

// ---- layout -------------------------------------------------------------------------------------------------------------------
struct In {
    int paneL = 0, paneT = 0, paneB = 0, axisR = 0;     // axisR = the price axis' left edge (pane right without the scale) - 2
    bool levels = false, delta = false, dealer = false;  // which columns are on this chart
    int levelsW = 40, deltaW = 80, dealerW = 80;
    bool big = false; int bigW = 44;                     // (1.1.0) the BIG column
};
struct Layout {
    bool ok = false, squeezed = false;
    int axisR = 0, priceRight = 0, margin = 0, paneL = 0;           // priceRight: the last bar ends left of it (the gap starts there)
    int w[NCOL] = { 0, 0, 0, 0, 0 };
    bp::Box levels, delta, dealer, big;                       // zero-width boxes for absent columns
};
inline Layout layout(const In& in)
{
    Layout L; L.axisR = in.axisR; L.paneL = in.paneL;
    int wl = in.levels ? cleanWidth(LEVELS, in.levelsW) : 0, wd = in.delta ? cleanWidth(DELTA, in.deltaW) : 0,
        wg = in.dealer ? cleanWidth(DEALER, in.dealerW) : 0, wb = in.big ? cleanWidth(BIG, in.bigW) : 0;
    const int sum = (LEVELS_LEFT ? 0 : wl) + (DELTA_LEFT ? 0 : wd) + wg + wb;
    const int avail = in.axisR - in.paneL - GAP_PX - MIN_PRICE_PX;
    if (sum > 0 && sum > avail) {                        // a narrow chart: shrink in proportion, never under each minimum
        L.squeezed = true;
        const double f = avail > 0 ? (double)avail / sum : 0.0;
        if (in.levels && !LEVELS_LEFT) wl = std::max(range(LEVELS).lo, (int)std::floor(wl * f));
        if (in.delta && !DELTA_LEFT) wd = std::max(range(DELTA).lo, (int)std::floor(wd * f));
        if (in.dealer) wg = std::max(range(DEALER).lo, (int)std::floor(wg * f));
        if (in.big) wb = std::max(range(BIG).lo, (int)std::floor(wb * f));
    }
    const int t = in.paneT, b = in.paneB;
    L.dealer = bp::mk(in.axisR - wg, t, in.axisR, b);
    const int outerL = (LEVELS_OUTER && !LEVELS_LEFT) ? L.dealer.l - wl : L.dealer.l;   // (1.8.0) the Levels column between Delta and Dealer
    if (LEVELS_OUTER && !LEVELS_LEFT) L.levels = bp::mk(outerL, t, L.dealer.l, b);
    L.delta = DELTA_LEFT ? bp::mk(in.paneL + 1, t, in.paneL + 1 + wd, b) : bp::mk(outerL - wd, t, outerL, b);
    const int rightOfBig = DELTA_LEFT ? outerL : L.delta.l;
    // (1.2.0, Rassul 18:57 "note the big column and how far it is from the delta profile .. fix alignment") the BIG column sits right
    // next to the Delta Profile: gap, Levels, BIG, Delta, Dealer (was gap, BIG, Levels, Delta, Dealer)
    L.big = bp::mk(rightOfBig - wb, t, rightOfBig, b);
    if (!(LEVELS_OUTER && !LEVELS_LEFT)) L.levels = LEVELS_LEFT ? bp::mk(in.paneL + 1, t, in.paneL + 1 + wl, b) : bp::mk(L.big.l - wl, t, L.big.l, b);
    const int used = (LEVELS_LEFT ? 0 : wl) + (DELTA_LEFT ? 0 : wd) + wg + wb;
    const int rightMost = (LEVELS_LEFT || LEVELS_OUTER) ? L.big.l : L.levels.l;   // (1.6.0 / 1.8.0) the leftmost right-side column   // (1.6.0) the leftmost right-side column
    L.priceRight = used > 0 ? rightMost - GAP_PX : in.axisR;
    L.margin = in.axisR - L.priceRight;
    L.w[LEVELS] = wl; L.w[DELTA] = wd; L.w[DEALER] = wg; L.w[BIG] = wb;
    L.ok = L.priceRight >= in.paneL + 40 && b > t;      // a chart with no room left for price: the columns are not drawn
    return L;
}
// no two column boxes overlap and none crosses the axis or the price area (the test's invariant, also used in the status line)
inline bool columnsClean(const Layout& L)
{
    const bp::Box* c[4] = { &L.levels, &L.delta, &L.dealer, &L.big };
    for (int i = 0; i < 4; ++i) {
        const bool leftCol = (DELTA_LEFT && c[i] == &L.delta) || (LEVELS_LEFT && c[i] == &L.levels);   // (1.5.0 / 1.6.0) a left column: inside the pane, left of the price room
        if (c[i]->r < c[i]->l || c[i]->r > L.axisR) return false;
        if (leftCol ? (c[i]->w() > 0 && (c[i]->l < L.paneL || c[i]->r > L.priceRight)) : (c[i]->w() > 0 && c[i]->l < L.priceRight + GAP_PX)) return false;
        for (int j = i + 1; j < 4; ++j) if (c[i]->w() > 0 && c[j]->w() > 0 && bp::overlap(*c[i], *c[j])) return false;
    }
    return true;
}

// ---- the right margin -----------------------------------------------------------------------------------------------------------
enum Units { PIXELS = 0, BARS = 1 };
inline int marginValue(int px, int ppb, int units)
{
    if (px < 0) px = 0;
    if (units == PIXELS) return px;
    ppb = std::max(1, std::min(400, ppb));
    return (px + ppb - 1) / ppb;                         // rounded UP: price never reaches the columns
}
inline int marginPx(int value, int ppb, int units) { if (units == PIXELS) return value; return value * (ppb < 1 ? 1 : ppb); }
// which unit IRT uses for OPT_RIGHT_MARGIN: compare the measured gap (axis - last bar's right edge, last bar on screen) with the
// value read back. Two agreeing observations in a row switch the unit; ambiguous ones (1 px per bar, value 0) are ignored.
struct UnitProbe {
    int units = BARS, votePx = 0, voteBars = 0; long long seen = 0;
    void observe(int value, int ppb, int gap)
    {
        if (value <= 0 || ppb <= 1 || gap < 0) return;
        const int dPx = std::abs(gap - value), dBars = std::abs(gap - value * ppb), tol = std::max(4, ppb);
        ++seen;
        if (dPx <= tol && dPx < dBars) { ++votePx; voteBars = 0; }
        else if (dBars <= tol + ppb && dBars < dPx) { ++voteBars; votePx = 0; }
        else { votePx = voteBars = 0; }
        if (votePx >= 2) units = PIXELS;
        if (voteBars >= 2) units = BARS;
    }
};
// the margin this chart needs: grows at once, shrinks only after 60 s (no shake); -1 = leave the chart's value alone
struct MarginCtl {
    bp::StableW stab; int lastSet = -1; long long lastMs = -1;
    int want(int needPx, int ppb, int units, int current, long long nowMs)
    {
        const int px = stab.step(needPx < 0 ? 0 : needPx, nowMs);
        const int v = marginValue(px, ppb, units);
        if (v == current) { lastSet = v; return -1; }
        if (v == lastSet && lastMs >= 0 && nowMs >= lastMs && nowMs - lastMs < 2000) return -1;   // asked already: give IRT time
        lastSet = v; lastMs = nowMs; return v;
    }
};

// ---- the board (process environment, per chart) -------------------------------------------------------------------------------
inline const char* prefix(int c) { return c == LIQ ? "LRA_COLL_" : c == LEVELS ? "LRA_COLK_" : c == DELTA ? "LRA_COLT_" : c == BIG ? "LRA_COLB_" : "LRA_COLD_"; }
inline std::string enc(long long ms, int w) { return std::to_string(ms) + "|" + std::to_string(w); }
inline bool dec(int c, const std::string& s, int& w, long long& ms)
{
    w = 0; ms = -1;
    if (s.empty() || s.size() > 40) return false;
    const size_t k = s.find('|'); if (k == std::string::npos) return false;
    long long m = 0; if (!bb::num(s.substr(0, k), m) || m < 0) return false;
    int v = 0; if (!parseWidth(c, s.substr(k + 1), v)) return false;
    w = v; ms = m; return true;
}
inline void publish(int c, const std::string& sym, long spb, const std::string& chart, long long ms, int w)
{ if (!sym.empty()) bb::bbSet(bb::key(prefix(c), sym, spb, chart), enc(ms, cleanWidth(c, w))); }
inline void clear(int c, const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) bb::bbSet(bb::key(prefix(c), sym, spb, chart), std::string()); }
inline bool peek(int c, const std::string& sym, long spb, const std::string& chart, int& w, long long& ms)
{
    std::string v; w = 0; ms = -1;
    return !sym.empty() && bb::bbGet(bb::key(prefix(c), sym, spb, chart), v) && dec(c, v, w, ms);
}
// (1.7.0, 2026-10-28, Rassul 22:06 CT screenshot: the HOD/LOD box, the Big Lot number boxes and the fill dots drew over the LEFT Liquidity
// column) THE LEFT KEEP-OUT: the x every plugin keeps its marks, labels and boxes right of - the right edge of the left columns (the Levels /
// liquidity column when LEVELS_LEFT, the Delta column when DELTA_LEFT) + 2 px, while their producer is posting (stamp within LEFT_FRESH_MS of
// nowMs, the steady clock every poster uses); else the pane's left edge. Stateless (a reader with Peers uses Layout.levels.r + 2 instead).
static const long long LEFT_FRESH_MS = 10000;
inline bool keysFresh(const std::string& sym, long spb, const std::string& chart, long long nowMs, int& w);
// (1.9.0) withKeys = false: the Key Levels column is NOT a keep-out (lsHodLod: its table docks inside that column)
inline int leftKeep(const std::string& sym, long spb, const std::string& chart, int paneL, long long nowMs, bool withKeys = true)
{
    int x = paneL, w = 0; long long ms = -1;
    if (withKeys && keysFresh(sym, spb, chart, nowMs, w)) x = std::max(x, paneL + 1 + w + 2);
    if (LEVELS_LEFT && peek(LEVELS, sym, spb, chart, w, ms) && nowMs >= ms && nowMs - ms <= LEFT_FRESH_MS) x = std::max(x, paneL + 1 + w + 2);
    if (DELTA_LEFT && peek(DELTA, sym, spb, chart, w, ms) && nowMs >= ms && nowMs - ms <= LEFT_FRESH_MS) x = std::max(x, paneL + 1 + w + 2);
    return x;
}
// the other plugins' columns on this chart, kept per chart by the reader; step() once per own draw
struct Peers {
    bb::Beat beat[NCOL]; bool on[NCOL] = { false, false, false, false, false }; int w[NCOL] = { 0, 0, 0, 0, 0 };
    void step(const std::string& sym, long spb, const std::string& chart, long long nowMs, unsigned selfMask)
    {
        for (int c = 0; c < NCOL; ++c) {
            if (selfMask & (1u << c)) continue;
            int v = 0; long long ms = -1; const bool have = peek(c, sym, spb, chart, v, ms);
            on[c] = beat[c].step(have, ms, nowMs);
            w[c] = on[c] ? v : 0;
        }
    }
};

// ---- (1.9.0) the KEY LEVELS column at the pane's LEFT edge (lsDealerProfile 2.33.0): the level badges; (1.9.1) the HOD/LOD table docks RIGHT of it at the bottom
static const bool KEYS_LEFT = true;
inline Range keysRange() { return Range{ 120, 460, 230 }; }
inline int cleanKeysW(long long v) { const Range r = keysRange(); return (v >= r.lo && v <= r.hi) ? (int)v : r.def; }
inline void publishKeys(const std::string& sym, long spb, const std::string& chart, long long ms, int w)
{ if (!sym.empty()) bb::bbSet(bb::key("LRA_COLV_", sym, spb, chart), enc(ms, cleanKeysW(w))); }
inline void clearKeys(const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) bb::bbSet(bb::key("LRA_COLV_", sym, spb, chart), std::string()); }
inline bool peekKeys(const std::string& sym, long spb, const std::string& chart, int& w, long long& ms)
{
    std::string v; w = 0; ms = -1;
    if (sym.empty() || !bb::bbGet(bb::key("LRA_COLV_", sym, spb, chart), v) || v.empty() || v.size() > 40) return false;
    const size_t k = v.find('|'); if (k == std::string::npos) return false;
    long long m = 0; if (!bb::num(v.substr(0, k), m) || m < 0) return false;
    const std::string t = v.substr(k + 1); if (t.empty() || t.size() > 4) return false;
    for (size_t i = 0; i < t.size(); ++i) if (t[i] < '0' || t[i] > '9') return false;
    const long long x = std::strtoll(t.c_str(), nullptr, 10); const Range r = keysRange(); if (x < r.lo || x > r.hi) return false;
    w = (int)x; ms = m; return true;
}
// the Key Levels column is on THIS chart now (its producer posted within LEFT_FRESH_MS of nowMs): its box = [paneL + 1, paneL + 1 + w]
inline bool keysFresh(const std::string& sym, long spb, const std::string& chart, long long nowMs, int& w)
{
    long long ms = -1; if (!KEYS_LEFT || !peekKeys(sym, spb, chart, w, ms) || nowMs < ms || nowMs - ms > 10000) { w = 0; return false; }
    return true;
}

// (1.9.1) where lsHodLod's table (its union with the strip / Q header / rows: [ul, ut, ur, ub]) docks while the Key Levels column is on the
// chart: BOTTOM-LEFT of the price area, its left edge DOCK_GAP px right of the column ([paneL + 1, paneL + 1 + keysW]), its bottom DOCK_GAP + 2 px
// above the pane bottom (Rassul 12:50 CT "on the right of the level column not in it ... on the bottom left next to the levels column");
// ok = false when the table is taller than the pane under the IRT title row (then it keeps its own Position)
struct Dock { int dx = 0, dy = 0; bool ok = false; };
static const int DOCK_GAP = 4;
inline Dock dockKeys(int ul, int ut, int ur, int ub, int paneL, int paneT, int paneB, int keysW)
{
    Dock d; const int title = (paneB - paneT) >= 200 ? 22 : 0;
    if (ur <= ul || ub <= ut || keysW <= 0) return d;
    const int h = ub - ut, l = paneL + 1 + keysW + DOCK_GAP, b = paneB - DOCK_GAP - 2;
    if (b - h < paneT + title + 2) return d;
    d.dx = l - ul; d.dy = (b - h) - ut; d.ok = true; return d;
}

// ---- the level badges: stacking and the name that fits ------------------------------------------------------------------------
struct Item { int want = 0; int h = 0; int y = 0; bool shown = false; };   // want = the line's y; y = the badge's centre
// in price order (top first); each badge as near its line as possible, never overlapping (1 px apart), inside [top, bottom].
// Badges that do not fit (more badge height than pane) are dropped from the side farthest from 'focusY' (the last price).
inline int stack(std::vector<Item>& v, int top, int bottom, int focusY)
{
    std::vector<size_t> idx; for (size_t i = 0; i < v.size(); ++i) { v[i].shown = false; idx.push_back(i); }
    std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return v[a].want < v[b].want; });
    // drop the farthest from the focus until the total height fits
    long long need = 0; for (size_t i = 0; i < idx.size(); ++i) need += v[idx[i]].h + 1;
    std::vector<size_t> keep = idx; int dropped = 0;
    while (!keep.empty() && need > (long long)(bottom - top)) {
        const int a = std::abs(v[keep.front()].want - focusY), z = std::abs(v[keep.back()].want - focusY);
        const size_t k = a > z ? 0 : keep.size() - 1;
        need -= v[keep[k]].h + 1; keep.erase(keep.begin() + (long)k); ++dropped;
    }
    // forward pass: push down; backward pass: pull up into the pane; then forward again for the top edge
    int prevB = top - 1;
    for (size_t i = 0; i < keep.size(); ++i) {
        Item& it = v[keep[i]]; int t = it.want - it.h / 2; if (t < prevB + 1) t = prevB + 1; if (t < top) t = top;
        it.y = t + it.h / 2; prevB = t + it.h; it.shown = true;
    }
    int nextT = bottom + 1;
    for (size_t i = keep.size(); i-- > 0;) {
        Item& it = v[keep[i]]; int t = it.y - it.h / 2;
        if (t + it.h > nextT - 1) t = nextT - 1 - it.h;
        it.y = t + it.h / 2; nextT = t;
    }
    return dropped;
}
// the badge name that fits 'room' px: the name, else the first of a merged name + "+", else cut (at least 2 characters), else ""
template <class W> std::string fitName(const std::string& name, int room, W widthOf)
{
    if (name.empty() || room <= 0) return std::string();
    if (widthOf(name) <= room) return name;
    std::string first = name;
    const size_t cut = first.find_first_of("/\xB7");
    if (cut != std::string::npos) { first = first.substr(0, cut); while (!first.empty() && first.back() == ' ') first.pop_back(); first += "+"; if (widthOf(first) <= room) return first; }
    std::string s = first;
    while (s.size() > 2 && widthOf(s) > room) s.pop_back();
    return widthOf(s) <= room ? s : std::string();
}
} // namespace cb
#endif
