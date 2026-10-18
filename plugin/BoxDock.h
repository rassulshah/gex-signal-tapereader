#ifndef LS_BOXDOCK_H
#define LS_BOXDOCK_H
// BoxDock.h (1.0.0, 2026-10-18) -- one small box DOCKED to another indicator's box on the same chart (SDK-free, Linux-tested).
// Rassul 2026-10-18 (approved 15:03 CT): "make it simple like BL 5 1434 and just have a compact box of the averages connected to the
// hod lod box when it is added to the chart so it doesn't overlap"; "no, just the avg".
//   lsHodLod (2.4.1+) publishes the rectangle of its HOD/LOD table on every draw:
//     LRA_HLB_<symbol>_<spb>_<chart>   "<ms>|<l>|<t>|<r>|<b>"   (BannerBoard.h encoding; empty = no table now)
//   lsBigBidAsk (1.16.0+) reads it (bb::Beat heartbeat: present while the stamp keeps moving) and attaches its Big Lot box to the
//   table's edge with a small gap: beside it on the right, else on the left (bottom-aligned when the table is in the lower half of
//   the pane, top-aligned otherwise), else above / below it (right edge, then left edge aligned). It moves with the table and never
//   overlaps the table, the other avoid boxes (Tape Reader banner) or anything right of the LAST BAR (the area ends there).
//   No HOD/LOD table on the chart (or no docked spot fits): the box's own 9-spot Position, else the first free spot of the 9.
#include <string>
#include <vector>
#include "BannerBoard.h"
namespace bdk {
static const char* const VERSION = "1.1.0";   // 1.1.0 (2026-10-18): LRA_HLT_ (the table alone) + placeMatched (the Big Lot box = the table's height)
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
} // namespace bdk
#endif
