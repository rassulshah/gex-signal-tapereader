#ifndef LS_BOXDOCK_H
#define LS_BOXDOCK_H
// BoxDock.h (1.2.0 2026-10-21 the keep-out; 1.0.0, 2026-10-18) -- one small box DOCKED to another indicator's box on the same chart (SDK-free, Linux-tested).
// Rassul 2026-10-18 (approved 15:03 CT): "make it simple like BL 5 1434 and just have a compact box of the averages connected to the
// hod lod box when it is added to the chart so it doesn't overlap"; "no, just the avg".
//   lsHodLod (2.4.1+) publishes the rectangle of its HOD/LOD table on every draw:
//     LRA_HLB_<symbol>_<spb>_<chart>   "<ms>|<l>|<t>|<r>|<b>"   (BannerBoard.h encoding; empty = no table now)
//   lsBigBidAsk (1.16.0+) reads it (bb::Beat heartbeat: present while the stamp keeps moving) and attaches its Big Lot box to the
//   table's edge with a small gap: beside it on the right, else on the left (bottom-aligned when the table is in the lower half of
//   the pane, top-aligned otherwise), else above / below it (right edge, then left edge aligned). It moves with the table and never
//   overlaps the table, the other avoid boxes (Tape Reader banner) or anything right of the LAST BAR (the area ends there).
//   No HOD/LOD table on the chart (or no docked spot fits): the box's own 9-spot Position, else the first free spot of the 9.
#include <chrono>
#include <cmath>
#include <string>
#include <vector>
#include "BannerBoard.h"
namespace bdk {
static const char* const VERSION = "1.2.0";   // 1.2.0 (2026-10-21): the KEEP-OUT (TableWatch / KeepScope / hide / clip) - no chart mark paints over the HOD/LOD table; 1.1.0 (2026-10-18): LRA_HLT_ (the table alone) + placeMatched (the Big Lot box = the table's height)
static const int GAP = 4;          // px between the docked box and the table
static const int MARGIN = 4;       // px kept inside the area edges (the 9-spot placement)
struct Box { int l = 0, t = 0, r = 0, b = 0; };
inline Box mk(int l, int t, int r, int b) { Box x; x.l = l; x.t = t; x.r = r; x.b = b; return x; }
inline bool valid(const Box& a) { return a.r > a.l && a.b > a.t; }
// touching edges do not overlap; gap = extra clearance demanded around b
inline bool overlap(const Box& a, const Box& b, int gap = 0) { return a.l < b.r + gap && b.l - gap < a.r && a.t < b.b + gap && b.t - gap < a.b; }
inline bool inside(const Box& outer, const Box& in) { return in.l >= outer.l && in.t >= outer.t && in.r <= outer.r && in.b <= outer.b; }
// the 9 spots: 1 top left .. 3 top right, 4 - 6 middle, 7 bottom left .. 9 bottom right, MARGIN inside the area
inline Box spot(const Box& area, int w, int h, int pos) {
    if (pos < 1 || pos > 9) pos = 9;
    const int col = (pos - 1) % 3, row = (pos - 1) / 3;
    const int L0 = area.l + MARGIN, R0 = area.r - MARGIN, T0 = area.t + MARGIN, B0 = area.b - MARGIN;
    const int l = col == 0 ? L0 : col == 1 ? (L0 + R0 - w) / 2 : R0 - w;
    const int t = row == 0 ? T0 : row == 1 ? (T0 + B0 - h) / 2 : B0 - h;
    return mk(l, t, l + w, t + h);
}
inline bool clear(const Box& c, const Box& area, const std::vector<Box>& avoid) {
    if (!inside(area, c)) return false;
    for (const Box& a : avoid) if (valid(a) && overlap(c, a, 2)) return false;
    return true;
}
enum Mode { NONE = 0, DOCKED = 1, OWN = 2, OWN_MOVED = 3, OWN_BLOCKED = 4 };
enum Side { S_NONE = 0, S_RIGHT = 1, S_LEFT = 2, S_ABOVE = 3, S_BELOW = 4 };
struct Result { Box b; int mode = NONE; int side = S_NONE; std::string why; };
inline const char* sideName(int s) { return s == S_RIGHT ? "right" : s == S_LEFT ? "left" : s == S_ABOVE ? "above" : s == S_BELOW ? "below" : "-"; }
// the candidate spots round the host, in order of preference
inline std::vector<std::pair<int, Box>> dockCandidates(const Box& host, int w, int h, const Box& area) {
    std::vector<std::pair<int, Box>> v;
    const bool lower = (host.t + host.b) / 2 >= (area.t + area.b) / 2;
    const int t = lower ? host.b - h : host.t;                    // the edge nearest the pane edge is shared
    v.push_back({ S_RIGHT, mk(host.r + GAP, t, host.r + GAP + w, t + h) });
    v.push_back({ S_LEFT,  mk(host.l - GAP - w, t, host.l - GAP, t + h) });
    const int tA = host.t - GAP - h, tB = host.b + GAP;
    if (lower) {   // a table at the bottom: above it first (below is the pane edge)
        v.push_back({ S_ABOVE, mk(host.r - w, tA, host.r, tA + h) }); v.push_back({ S_ABOVE, mk(host.l, tA, host.l + w, tA + h) });
        v.push_back({ S_BELOW, mk(host.r - w, tB, host.r, tB + h) }); v.push_back({ S_BELOW, mk(host.l, tB, host.l + w, tB + h) });
    } else {
        v.push_back({ S_BELOW, mk(host.r - w, tB, host.r, tB + h) }); v.push_back({ S_BELOW, mk(host.l, tB, host.l + w, tB + h) });
        v.push_back({ S_ABOVE, mk(host.r - w, tA, host.r, tA + h) }); v.push_back({ S_ABOVE, mk(host.l, tA, host.l + w, tA + h) });
    }
    return v;
}
// area = where the box may go (left limit .. the LAST BAR, below the IRT title row); avoid = other boxes (Tape Reader banner ...)
inline Result place(bool hostOn, const Box& host, int w, int h, const Box& area, int pos, std::vector<Box> avoid) {
    Result R;
    if (w <= 0 || h <= 0 || !valid(area) || w > area.r - area.l - 2 * MARGIN || h > area.b - area.t - 2 * MARGIN) { R.why = "area too small"; return R; }
    if (hostOn && valid(host)) {
        for (const auto& c : dockCandidates(host, w, h, area))
            if (clear(c.second, area, avoid) && !overlap(c.second, host)) { R.b = c.second; R.mode = DOCKED; R.side = c.first; R.why = std::string("docked ") + sideName(c.first); return R; }
        avoid.push_back(host);                                      // no docked spot fits: still never on the table
        R.why = "no docked spot fits: ";
    }
    const Box own = spot(area, w, h, pos);
    if (clear(own, area, avoid)) { R.b = own; R.mode = OWN; R.why += "own position"; return R; }
    for (int p = 1; p <= 9; ++p) { if (p == pos) continue; const Box c = spot(area, w, h, p); if (clear(c, area, avoid)) { R.b = c; R.mode = OWN_MOVED; R.why += "spot " + std::to_string(p) + " (own position blocked)"; return R; } }
    R.mode = NONE; R.why += "every spot blocked: not drawn"; return R;   // never drawn over another box
}
// ---- the board (process environment block, as BannerBoard.h) -------------------------------------------------------------------
inline void publishHodLod(const std::string& sym, long spb, const std::string& chart, long long ms, const Box& r) {
    if (sym.empty()) return;
    bb::Rect x; x.l = r.l; x.t = r.t; x.r = r.r; x.b = r.b;
    bb::bbSet(bb::key("LRA_HLB_", sym, spb, chart), bb::encBanner(ms, x));
}
inline void clearHodLod(const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) bb::bbSet(bb::key("LRA_HLB_", sym, spb, chart), std::string()); }
// the entry at any age + its stamp (the reader's bb::Beat decides presence); false = none / malformed / cleared
inline bool peekHodLod(const std::string& sym, long spb, const std::string& chart, Box& r, long long& ms) {
    std::string v; ms = -1; bb::Rect x;
    if (sym.empty() || !bb::bbGet(bb::key("LRA_HLB_", sym, spb, chart), v) || !bb::decBanner(v, -1, x)) return false;
    ms = bb::stampOf(v); r = mk(x.l, x.t, x.r, x.b); return ms >= 0;
}
// ---- (1.1.0, Rassul 2026-10-18 "the BLS box should be the same height as the hod lod box") ----------------------------------------
// lsHodLod 2.4.4 draws the Gex S/R strip ABOVE its table: LRA_HLB_ = table + strip (+ header) - what nothing may cover; LRA_HLT_ = the
// table alone - what the Big Lot box matches: beside it (right, else left), the table's top and bottom, never the strip's.
inline void publishHodLodTable(const std::string& sym, long spb, const std::string& chart, long long ms, const Box& r) {
    if (sym.empty()) return;
    bb::Rect x; x.l = r.l; x.t = r.t; x.r = r.r; x.b = r.b;
    bb::bbSet(bb::key("LRA_HLT_", sym, spb, chart), bb::encBanner(ms, x));
}
inline void clearHodLodTable(const std::string& sym, long spb, const std::string& chart) { if (!sym.empty()) bb::bbSet(bb::key("LRA_HLT_", sym, spb, chart), std::string()); }
inline bool peekHodLodTable(const std::string& sym, long spb, const std::string& chart, Box& r, long long& ms) {
    std::string v; ms = -1; bb::Rect x;
    if (sym.empty() || !bb::bbGet(bb::key("LRA_HLT_", sym, spb, chart), v) || !bb::decBanner(v, -1, x)) return false;
    ms = bb::stampOf(v); r = mk(x.l, x.t, x.r, x.b); return ms >= 0;
}
// the docked box at the TABLE's height (full = table + strip, never covered; table = the part whose top / bottom it shares).
// minH = the box's smallest readable height: a table shorter than that keeps the old place() (own height).
inline Result placeMatched(bool hostOn, const Box& full, const Box& table, int w, int minH, const Box& area, int pos, std::vector<Box> avoid, int* hOut = nullptr) {
    if (hostOn && valid(table) && valid(full) && table.b - table.t >= minH) {
        const int h = table.b - table.t;
        const Box c[2] = { mk(table.r + GAP, table.t, table.r + GAP + w, table.b), mk(table.l - GAP - w, table.t, table.l - GAP, table.b) };
        for (int i = 0; i < 2; ++i)
            if (clear(c[i], area, avoid) && !overlap(c[i], full)) { Result R; R.b = c[i]; R.mode = DOCKED; R.side = i ? S_LEFT : S_RIGHT; R.why = std::string("docked ") + sideName(R.side) + ", table height"; if (hOut) *hOut = h; return R; }
    }
    if (hOut) *hOut = minH;
    return place(hostOn, full, w, minH, area, pos, avoid);
}
// ---- (1.2.0) THE KEEP-OUT: chart marks never paint over the HOD/LOD table ------------------------------------------------------
// Rassul 16:05 CT 2026-10-21 (ES 3-min): "fix this, it should be solid" - Exhaustion "E ?" / "R ?" labels, swing lines with their
// labels, key-level lines and circles of OTHER indicators were painted on top of the lsHodLod table. IRT draws the indicators in
// chart order, so the table cannot simply paint last: every plugin that draws on the price pane asks here before it paints.
//   TableWatch (one per chart, in the plugin's HostSlot state): reads LRA_HLB_ (table + strip + header, as published by lsHodLod)
//     and decides "the table is there" with the bb::Beat heartbeat, or the stamp under bb::FRESH_MS (lsHodLod clears the entry when
//     its table is off or it is removed, so an old stamp only means an idle chart). A different chart / symbol / bar size = another key.
//   KeepScope (one per draw call): makes that chart's table the ACTIVE keep-out for the plugin's drawing helpers on this thread;
//     the destructor clears it, so nothing carries from one chart's draw to another's.
//   hide(l, t, r, b): a mark / label / circle / badge that would touch the table (KEEP_MARGIN px around it) is NOT painted.
//   segParts / hParts: a line is cut at the table's edges; only the parts outside are painted.
//   rectParts: a plain fill (band, bar) loses the part under the table (up to 4 pieces remain).
// Screen only: the plugin's records, signals, status files and no-repaint rules never ask here.
static const int KEEP_MARGIN = 2;
inline Box grow(const Box& a, int m) { return mk(a.l - m, a.t - m, a.r + m, a.b + m); }
// any (inclusive) overlap of a mark's rectangle with the table + margin; rectangles given in any corner order
inline bool hitsTable(const Box& table, int l, int t, int r, int b, int m = KEEP_MARGIN) {
    if (!valid(table)) return false;
    if (r < l) { const int k = l; l = r; r = k; } if (b < t) { const int k = t; t = b; b = k; }
    const Box g = grow(table, m);
    return l <= g.r && r >= g.l && t <= g.b && b >= g.t;
}
// a segment against the table + margin: the visible parts (0, 1 or 2) - Liang-Barsky on the grown box
struct Seg { int x1 = 0, y1 = 0, x2 = 0, y2 = 0; };
inline int segParts(const Box& table, int x1, int y1, int x2, int y2, Seg out[2], int m = KEEP_MARGIN) {
    Seg whole; whole.x1 = x1; whole.y1 = y1; whole.x2 = x2; whole.y2 = y2;
    if (!valid(table)) { out[0] = whole; return 1; }
    const Box g = grow(table, m);
    const double dx = (double)x2 - x1, dy = (double)y2 - y1;
    double t0 = 0.0, t1 = 1.0;
    const double p[4] = { -dx, dx, -dy, dy }, q[4] = { (double)x1 - g.l, (double)g.r - x1, (double)y1 - g.t, (double)g.b - y1 };
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0.0) { if (q[i] < 0.0) { out[0] = whole; return 1; } continue; }   // parallel and outside: all visible
        const double r = q[i] / p[i];
        if (p[i] < 0.0) { if (r > t1) { out[0] = whole; return 1; } if (r > t0) t0 = r; }
        else { if (r < t0) { out[0] = whole; return 1; } if (r < t1) t1 = r; }
    }
    // [t0, t1] is inside the grown box: keep [0, t0) and (t1, 1], one pixel clear of it
    int n = 0;
    auto at = [&](double t, int& x, int& y) { x = (int)(x1 + dx * t + (dx >= 0 ? 0.5 : -0.5)); y = (int)(y1 + dy * t + (dy >= 0 ? 0.5 : -0.5)); };
    const double len = std::sqrt(dx * dx + dy * dy), eps = len > 0.0 ? 1.0 / len : 1.0;
    for (int k = 1; k <= 3; ++k) {   // the cut end one pixel clear of the box (a step further when rounding lands on its edge)
        if (t0 - k * eps <= 0.0) break;
        Seg s; s.x1 = x1; s.y1 = y1; at(t0 - k * eps, s.x2, s.y2);
        if (!hitsTable(table, s.x2, s.y2, s.x2, s.y2, m)) { out[n++] = s; break; }
    }
    for (int k = 1; k <= 3; ++k) {
        if (t1 + k * eps >= 1.0) break;
        Seg s; at(t1 + k * eps, s.x1, s.y1); s.x2 = x2; s.y2 = y2;
        if (!hitsTable(table, s.x1, s.y1, s.x1, s.y1, m)) { out[n++] = s; break; }
    }
    return n;
}
// a horizontal line [x1, x2] at y: the visible x ranges (0, 1 or 2)
inline int hParts(const Box& table, int x1, int x2, int y, int out[2][2], int m = KEEP_MARGIN) {
    if (x2 < x1) { const int k = x1; x1 = x2; x2 = k; }
    if (!valid(table) || y < table.t - m || y > table.b + m || x2 < table.l - m || x1 > table.r + m) { out[0][0] = x1; out[0][1] = x2; return 1; }
    int n = 0;
    if (x1 < table.l - m) { out[n][0] = x1; out[n][1] = table.l - m - 1; ++n; }
    if (x2 > table.r + m) { out[n][0] = table.r + m + 1; out[n][1] = x2; ++n; }
    return n;
}
// a filled rectangle minus the table + margin (0 .. 4 pieces; the whole one when it does not touch)
inline int rectParts(const Box& table, const Box& a, Box out[4], int m = KEEP_MARGIN) {
    if (!valid(a)) return 0;
    if (!hitsTable(table, a.l, a.t, a.r, a.b, m)) { out[0] = a; return 1; }
    const Box g = grow(table, m); int n = 0;
    if (a.t < g.t) out[n++] = mk(a.l, a.t, a.r, g.t);                                       // above
    if (a.b > g.b + 1) out[n++] = mk(a.l, g.b + 1, a.r, a.b);                               // below
    const int t = a.t > g.t ? a.t : g.t, b = a.b < g.b + 1 ? a.b : g.b + 1;
    if (b > t) {
        if (a.l < g.l) out[n++] = mk(a.l, t, g.l, b);                                       // left
        if (a.r > g.r + 1) out[n++] = mk(g.r + 1, t, a.r, b);                               // right
    }
    return n;
}
// one per chart (HostSlot state): is lsHodLod's table on THIS chart now, and where
struct TableWatch {
    bb::Beat beat; Box box; bool on = false;
    bool refresh(const std::string& sym, long spb, const std::string& chart, long long nowMs) {
        Box r; long long ms = -1;
        const bool have = peekHodLod(sym, spb, chart, r, ms);
        const bool beatOn = beat.step(have, ms, nowMs);
        on = have && valid(r) && (beatOn || (nowMs >= ms && nowMs - ms <= bb::FRESH_MS));
        box = on ? r : Box();
        return on;
    }
    bool refresh(const char* sym, long spb, const char* chart) {   // the plugin's call (lsHodLod stamps with the same steady clock)
        const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        return refresh(std::string(sym ? sym : ""), spb, std::string(chart ? chart : ""), ms);
    }
};
// the active keep-out of THIS draw call (thread-local, set by KeepScope, cleared when the draw returns)
struct Active { bool on = false; Box box; long long hidden = 0, clipped = 0; };
inline Active& active() { static thread_local Active a; return a; }
struct KeepScope {
    Active prev;
    explicit KeepScope(const TableWatch& w) : prev(active()) { Active& a = active(); a.on = w.on && valid(w.box); a.box = w.box; a.hidden = 0; a.clipped = 0; }
    ~KeepScope() { active() = prev; }
    KeepScope(const KeepScope&) = delete; KeepScope& operator=(const KeepScope&) = delete;
};
// the drawing helpers' questions (no active scope = nothing hidden, nothing cut)
inline bool hide(int l, int t, int r, int b) { Active& a = active(); if (!a.on || !hitsTable(a.box, l, t, r, b)) return false; ++a.hidden; return true; }
inline int seg(int x1, int y1, int x2, int y2, Seg out[2]) {
    Active& a = active();
    if (!a.on) { out[0].x1 = x1; out[0].y1 = y1; out[0].x2 = x2; out[0].y2 = y2; return 1; }
    const int n = segParts(a.box, x1, y1, x2, y2, out);
    if (n != 1 || out[0].x1 != x1 || out[0].y1 != y1 || out[0].x2 != x2 || out[0].y2 != y2) ++a.clipped;
    return n;
}
inline int rects(const Box& r, Box out[4]) {
    Active& a = active();
    if (!a.on) { if (!valid(r)) return 0; out[0] = r; return 1; }
    const int n = rectParts(a.box, r, out);
    if (n != 1 || out[0].l != r.l || out[0].t != r.t || out[0].r != r.r || out[0].b != r.b) ++a.clipped;
    return n;
}
inline std::string keepLine() { const Active& a = active(); return std::string("UNDER_HODLOD,") + (a.on ? "table on" : "no table") + ",hidden " + std::to_string(a.hidden) + ",clipped " + std::to_string(a.clipped) + "\n"; }
} // namespace bdk
#endif
