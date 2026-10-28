// test_columnboard_180.cpp - ColumnBoard 1.8.0 (2026-10-28 06:42 CT, Rassul option R "keep the liquidity profile behind the delta profile"): on the right,
// out from the candles: gap, BIG (LMT F), Delta (READ strip + profile), Levels (liquidity top 10), Dealer, axis; the left side is free.   g++ -std=c++17 -I. test_columnboard_180.cpp && ./a.out
#include "ColumnBoard.h"
#include <cstdio>
static int P = 0, F = 0;
static void ok(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
int main()
{
    cb::In in; in.paneL = 0; in.paneT = 0; in.paneB = 600; in.axisR = 1600;
    in.levels = true; in.levelsW = 170; in.delta = true; in.deltaW = 150; in.dealer = true; in.dealerW = 80; in.big = true; in.bigW = 64;
    cb::Layout L = cb::layout(in);
    ok(!cb::DELTA_LEFT && !cb::LEVELS_LEFT && cb::LEVELS_OUTER, "flags: Delta right, Levels right outside Delta");
    ok(L.dealer.r == 1600 && L.levels.r == L.dealer.l && L.delta.r == L.levels.l && L.big.r == L.delta.l, "right side: BIG | Delta | Levels | Dealer abut, axis last");
    ok(L.levels.w() == 170 && L.delta.w() == 150 && L.big.w() == 64, "widths kept");
    ok(L.big.l - L.priceRight == cb::GAP_PX, "the gap sits between the last bar and LMT F");
    ok(L.margin == cb::GAP_PX + 64 + 150 + 170 + 80, "margin = gap + BIG + Delta + Levels + Dealer");
    ok(cb::columnsClean(L), "clean");
    ok(cb::leftKeep("ESZ6", 180, "c", 0, 1000) == 0, "the left side is free: leftKeep = the pane edge");
    in.dealer = false; L = cb::layout(in);
    ok(L.levels.r == 1600 && L.delta.r == L.levels.l && L.big.r == L.delta.l && cb::columnsClean(L), "no Dealer: Levels against the axis");
    in.levels = false; L = cb::layout(in);
    ok(L.delta.r == 1600 && L.big.r == L.delta.l && cb::columnsClean(L), "no Levels: Delta against the axis");
    in.levels = true; in.dealer = true; in.axisR = 500; L = cb::layout(in);
    ok(L.squeezed && (cb::columnsClean(L) || !L.ok), "narrow chart: squeezed, still clean (or not drawn)");
    std::printf("ColumnBoard %s: %d passed, %d failed\n", cb::VERSION, P, F); return F ? 1 : 0;
}
