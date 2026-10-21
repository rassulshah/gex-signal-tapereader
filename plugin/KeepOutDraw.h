#ifndef LS_KEEPOUTDRAW_H
#define LS_KEEPOUTDRAW_H
// KeepOutDraw.h (1.0.0, 2026-10-21) -- the SDK side of the BoxDock.h 1.2.0 keep-out: the drawing calls a price-pane plugin uses
// instead of the raw PNT / RCT ones, so nothing it paints shows inside the lsHodLod table.
// Rassul 16:05 CT 2026-10-21 (ES 3-min, the HOD/LOD table with Exhaustion / Defense "E ?" / "R ?" labels, lines and circles on it):
// "fix this, it should be solid".
//   Include AFTER irtsdk.h (and windows.h before it). Each draw(): bdk::TableWatch::refresh(...) on the chart's own state, then a
//   bdk::KeepScope for the rest of the draw; with no scope (or no table) every call below draws exactly as the raw call did.
//   ko::line       a line: cut at the table's edges (0, 1 or 2 parts drawn with the current pen)
//   ko::linePts    the same for two PNTs already set by bar / price (drawn through them when nothing is cut)
//   ko::text       RCT::drawText: not drawn when its rectangle touches the table
//   ko::mark       RCT::draw of a badge / box with a frame or text in it: not drawn when it touches the table
//   ko::fill       RCT::draw of a plain fill (band, bar, profile row): the part under the table is not painted
//   ko::oval       RCT::drawOval (a circle / bubble): not drawn when it touches the table
// Screen only: records, signals and status files never depend on these calls.
#include "BoxDock.h"
#ifndef KO_PNT
#define KO_PNT cppExtension::PNT   // the SDK's point class (a test mock that declares PNT outside cppExtension defines KO_PNT PNT)
#endif
namespace ko {
static const char* const VERSION = "1.0.0";
// templates: a call is compiled only where a plugin uses it (the test mocks do not all declare every RCT / PNT member)
template <class P = KO_PNT> inline void rawLine(int x1, int y1, int x2, int y2) {
    P a; a.set(0, 0.0f); a.h = (short)x1; a.v = (short)y1; a.setDrawPosition();
    P b; b.set(0, 0.0f); b.h = (short)x2; b.v = (short)y2; b.drawLineTo();
}
template <class P = KO_PNT> inline void line(int x1, int y1, int x2, int y2) {
    bdk::Seg s[2]; const int n = bdk::seg(x1, y1, x2, y2, s);
    for (int i = 0; i < n; ++i) rawLine<P>(s[i].x1, s[i].y1, s[i].x2, s[i].y2);
}
// a line between two PNTs already set (bar / price): drawn with them unchanged when nothing is cut, else the visible parts
template <class P> inline void linePts(P& a, P& b) {
    bdk::Seg s[2]; const int n = bdk::seg(a.h, a.v, b.h, b.v, s);
    if (n == 1 && s[0].x1 == a.h && s[0].y1 == a.v && s[0].x2 == b.h && s[0].y2 == b.v) { a.setDrawPosition(); b.drawLineTo(); return; }
    for (int i = 0; i < n; ++i) rawLine<P>(s[i].x1, s[i].y1, s[i].x2, s[i].y2);
}
template <class R> inline bool off(const R& r) { return bdk::hide(r.left, r.top, r.right, r.bottom); }
template <class R, class B> inline void text(R& r, const char* s, B centred, B right) { if (!off(r)) r.drawText(s, centred, right); }
template <class R, class C, class D, class S> inline void mark(R& r, short w, C border, C fillc, D mode, S st) { if (!off(r)) r.draw(w, border, fillc, mode, st); }
template <class R, class C, class D, class S> inline void fill(R& r, short w, C border, C fillc, D mode, S st) {
    bdk::Box p[4]; const int n = bdk::rects(bdk::mk(r.left, r.top, r.right, r.bottom), p);
    if (n == 1 && p[0].l == r.left && p[0].t == r.top && p[0].r == r.right && p[0].b == r.bottom) { r.draw(w, border, fillc, mode, st); return; }
    for (int i = 0; i < n; ++i) { R q; q.set((short)p[i].l, (short)p[i].t, (short)p[i].r, (short)p[i].b); q.draw(0, fillc, fillc, mode, st); }   // pieces: fill only (no frame through the cut)
}
template <class R, class D> inline void oval(R& r, D mode) { if (!off(r)) r.drawOval(mode); }
} // namespace ko
#endif
