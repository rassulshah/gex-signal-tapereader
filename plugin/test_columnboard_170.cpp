// test_columnboard_170.cpp -- ColumnBoard 1.7.0 (2026-10-28, live 22:06: the HOD/LOD box, the Big Lot number boxes and the fill dots drew over the LEFT
// Liquidity column): cb::leftKeep = the one keep-out x every plugin uses - the left Levels column's right edge + 2 while its producer posts, the
// pane's left edge when it is gone or stale; a published width outside the range is refused (the default is not invented).
// g++ -std=c++17 -O1 -I. test_columnboard_170.cpp -o /tmp/tcb170 && /tmp/tcb170
#include "ColumnBoard.h"
#include <cstdio>
static int P = 0, F = 0; static void check(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
int main() {
    const std::string sym = "ESZ6", ch = "ES 3m"; const long spb = 180;
    check(cb::LEVELS_LEFT && !cb::DELTA_LEFT, "layout: Levels left, Delta right (1.6.0)");
    check(cb::leftKeep(sym, spb, ch, 0, 100000) == 0, "no Levels producer: keep = the pane's left edge");
    cb::publish(cb::LEVELS, sym, spb, ch, 100000, 170);
    check(cb::leftKeep(sym, spb, ch, 0, 101000) == 173, "Levels column 170 px from pane left 0: keep right of x 173 (1 + 170 + 2)");
    check(cb::leftKeep(sym, spb, ch, 10, 101000) == 183, "the pane's own left edge is added");
    check(cb::leftKeep(sym, spb, ch, 0, 100000 + cb::LEFT_FRESH_MS + 1) == 0, "a producer that stopped posting (> 10 s) keeps nothing");
    check(cb::leftKeep(sym, spb, "other chart", 0, 101000) == 0, "another chart's column does not count");
    cb::Layout L; { cb::In in; in.paneL = 0; in.paneT = 0; in.paneB = 500; in.axisR = 1400; in.levels = true; in.levelsW = 170; in.delta = true; in.deltaW = 110; in.big = true; in.bigW = 60; L = cb::layout(in); }
    check(L.levels.r + 2 == 173, "the Layout's levels box agrees with leftKeep (a Peers reader uses it)");
    std::printf("ColumnBoard %s: %d passed, %d failed\n", cb::VERSION, P, F); return F ? 1 : 0;
}
