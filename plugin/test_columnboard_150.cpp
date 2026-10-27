// test_columnboard_150.cpp - ColumnBoard 1.5.0: the Delta column at the pane's LEFT edge (Rassul 2026-10-27 14:00 CT)
#include "ColumnBoard.h"
#include <cstdio>
static int P = 0, F = 0;
static void ok(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
int main()
{
    cb::In in; in.paneL = 0; in.paneT = 0; in.paneB = 600; in.axisR = 1600;
    in.levels = true; in.levelsW = 420; in.delta = true; in.deltaW = 110; in.dealer = true; in.dealerW = 80;
    cb::Layout L = cb::layout(in);
    ok(cb::DELTA_LEFT, "DELTA_LEFT on");
    ok(L.delta.l == 1 && L.delta.r == 111, "Delta at the pane's left edge, its width");
    ok(L.dealer.r == 1600 && L.big.r == L.dealer.l && L.levels.r == L.big.l, "right side: Levels | BIG | Dealer abut, no Delta between");
    ok(L.margin == cb::GAP_PX + 420 + 80, "margin = gap + Levels + Dealer (Delta not in it)");
    ok(cb::columnsClean(L), "clean: no overlap, left column left of the price room");
    in.levels = false; in.dealer = false; L = cb::layout(in);
    ok(L.margin == 0 && L.priceRight == 1600, "Delta alone: no right margin");
    ok(cb::columnsClean(L), "Delta alone clean");
    in.levels = true; in.dealer = true; in.axisR = 500; L = cb::layout(in);
    ok(L.delta.w() == 110, "narrow chart: Delta not squeezed by the right columns");
    std::printf("RESULT %d passed, %d failed\n", P, F); return F ? 1 : 0;
}
