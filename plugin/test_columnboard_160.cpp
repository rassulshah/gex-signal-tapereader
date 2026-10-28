// test_columnboard_160.cpp - ColumnBoard 1.6.0 (2026-10-28, Rassul 19:08 / 19:25 CT): Liquidity (Levels) at the pane's LEFT edge; on the right,
// out from the candles: gap, BIG (LMT F), Delta (READ strip + profile), Dealer, axis.   g++ -std=c++17 -I. test_columnboard_160.cpp && ./a.out
#include "ColumnBoard.h"
#include <cstdio>
static int P = 0, F = 0;
static void ok(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
int main()
{
    cb::In in; in.paneL = 0; in.paneT = 0; in.paneB = 600; in.axisR = 1600;
    in.levels = true; in.levelsW = 170; in.delta = true; in.deltaW = 150; in.dealer = true; in.dealerW = 80; in.big = true; in.bigW = 64;
    cb::Layout L = cb::layout(in);
    ok(!cb::DELTA_LEFT && cb::LEVELS_LEFT, "flags: Delta right, Levels left");
    ok(L.levels.l == 1 && L.levels.r == 171, "Levels (liquidity) at the pane's left edge, its width");
    ok(L.dealer.r == 1600 && L.delta.r == L.dealer.l && L.big.r == L.delta.l, "right side: BIG | Delta | Dealer abut, axis last");
    ok(L.big.l > L.priceRight && L.big.l - L.priceRight == cb::GAP_PX, "the gap sits between the last bar and LMT F");
    ok(L.margin == cb::GAP_PX + 64 + 150 + 80, "margin = gap + BIG + Delta + Dealer (Levels not in it)");
    ok(cb::columnsClean(L), "clean: no overlap, the left column left of the price room");
    in.dealer = false; L = cb::layout(in);
    ok(L.delta.r == 1600 && L.big.r == L.delta.l, "no Dealer: Delta against the axis, LMT F next to it");
    ok(cb::columnsClean(L), "no Dealer: clean");
    in.big = false; L = cb::layout(in);
    ok(L.priceRight == L.delta.l - cb::GAP_PX, "no LMT F: the gap before the Delta column");
    in.big = true; in.axisR = 520; L = cb::layout(in);
    ok(L.levels.w() == 170, "narrow chart: the left column is not squeezed by the right ones");
    ok(L.priceRight - L.paneL >= cb::MIN_PRICE_PX - 40 || !L.ok, "narrow chart: price room kept or not drawn");
    std::printf("ColumnBoard %s: %d passed, %d failed\n", cb::VERSION, P, F); return F ? 1 : 0;
}
