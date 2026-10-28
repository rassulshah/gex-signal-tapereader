// test_columnboard_191.cpp - ColumnBoard 1.9.1 (2026-10-28 12:50 CT, Rassul "the hod lod should be on the right of the level column not in it. and it should
// be on the bottom left next to the levels column"): the HOD/LOD table dock = BOTTOM-LEFT of the price area, immediately RIGHT of the Key Levels column
// (left = column right + DOCK_GAP, bottom = pane bottom - DOCK_GAP - 2); the column is a keep-out for every plugin again (lsHodLod too); the rest of 1.9.0.
// g++ -std=c++17 -I. test_columnboard_191.cpp && ./a.out
// was: // test_columnboard_190.cpp - ColumnBoard 1.9.0 (2026-10-28 12:16-12:18 CT, Rassul "a column for levels ... move the hod lod box aligned with the liquidity
// profile on the left"): the KEY LEVELS column at the pane's LEFT edge (lsDealerProfile 2.33.0) - board key, freshness, leftKeep with / without it,
// the HOD/LOD table dock (top-left inside the column, under the IRT title row), and the 1.8.0 right side unchanged.   g++ -std=c++17 -I. test_columnboard_190.cpp && ./a.out
#include "ColumnBoard.h"
#include <cstdio>
static int P = 0, F = 0;
static void ok(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
int main()
{
    ok(std::string(cb::VERSION) == "1.9.1" && cb::KEYS_LEFT && !cb::LEVELS_LEFT && cb::LEVELS_OUTER, "1.9.1: Key Levels left, liquidity right outside Delta");
    // the right side is unchanged (1.8.0 layout)
    cb::In in; in.paneL = 0; in.paneT = 0; in.paneB = 600; in.axisR = 1600;
    in.levels = true; in.levelsW = 170; in.delta = true; in.deltaW = 150; in.dealer = true; in.dealerW = 80; in.big = true; in.bigW = 64;
    cb::Layout L = cb::layout(in);
    ok(L.big.r <= L.delta.l && L.delta.r <= L.levels.l && L.levels.r <= L.dealer.l && cb::columnsClean(L), "right side LMT F | Delta | Liquidity | Dealer unchanged");
    // board: nothing posted = no keep-out
    ok(cb::leftKeep("ESZ6", 180, "k", 0, 1000) == 0, "no Key Levels column: leftKeep = pane edge");
    int w = 0;
    ok(!cb::keysFresh("ESZ6", 180, "k", 1000, w) && w == 0, "no column: not fresh");
    cb::publishKeys("ESZ6", 180, "k", 1000, 230);
    ok(cb::keysFresh("ESZ6", 180, "k", 1500, w) && w == 230, "posted 230: fresh, width 230");
    ok(cb::leftKeep("ESZ6", 180, "k", 0, 1500) == 0 + 1 + 230 + 2, "leftKeep = paneL + 1 + 230 + 2 for every other plugin");
    ok(cb::leftKeep("ESZ6", 180, "k", 0, 1500, false) == 0, "withKeys false still ignores the column (no caller since lsHodLod 2.11.0)");
    ok(cb::leftKeep("ESZ6", 180, "k", 0, 12001) == 0, "stale after 10 s: the keep-out is gone");
    ok(cb::leftKeep("ESZ6", 60, "k", 0, 1500) == 0 && cb::leftKeep("NQZ6", 180, "k", 0, 1500) == 0 && cb::leftKeep("ESZ6", 180, "other", 0, 1500) == 0, "another bar size / symbol / chart: no effect");
    cb::publishKeys("ESZ6", 180, "k", 2000, 9999); ok(cb::keysFresh("ESZ6", 180, "k", 2000, w) && w == cb::keysRange().def, "an out-of-range width is cleaned to the default");
    cb::publishKeys("ESZ6", 180, "k", 2000, 300); ok(cb::keysFresh("ESZ6", 180, "k", 2000, w) && w == 300, "300 (the table's width) accepted");
    cb::clearKeys("ESZ6", 180, "k"); ok(!cb::keysFresh("ESZ6", 180, "k", 2000, w), "cleared: gone");
    ok(cb::cleanKeysW(50) == 230 && cb::cleanKeysW(460) == 460 && cb::cleanKeysW(120) == 120, "keys range 120-460, default 230");
    // (1.9.1) the dock: a 270 x 300 table + strip somewhere mid-chart -> bottom-left, its left edge right of the 230 px column
    cb::Dock d = cb::dockKeys(700, 300, 970, 600, 0, 0, 820, 230);
    ok(d.ok && 700 + d.dx == 1 + 230 + cb::DOCK_GAP && 600 + d.dy == 820 - cb::DOCK_GAP - 2, "dock: table at the bottom-left, right of the column (235, bottom 814)");
    ok(d.ok && 700 + d.dx > 1 + 230 && 300 + d.dy > 22, "dock: never inside the column, never in the title row");
    d = cb::dockKeys(235, 514, 505, 814, 0, 0, 820, 230); ok(d.ok && d.dx == 0 && d.dy == 0, "already docked: no move (no jitter)");
    d = cb::dockKeys(100, 10, 370, 900, 0, 0, 820, 230); ok(!d.ok, "a table taller than the pane: not docked (keeps its Position)");
    d = cb::dockKeys(100, 10, 100, 50, 0, 0, 820, 230); ok(!d.ok, "an empty table: not docked");
    d = cb::dockKeys(500, 100, 760, 300, 40, 10, 150, 230); ok(d.ok == false, "a short pane (140 px) with a 200 px table: not docked");
    d = cb::dockKeys(500, 100, 760, 200, 40, 10, 190, 230); ok(d.ok && 500 + d.dx == 40 + 1 + 230 + cb::DOCK_GAP && 200 + d.dy == 190 - cb::DOCK_GAP - 2, "a pane under 200 px: no title row reserve, still bottom-left");
    d = cb::dockKeys(700, 300, 970, 600, 0, 0, 820, 300); ok(d.ok && 700 + d.dx == 1 + 300 + cb::DOCK_GAP, "follows the column's width (300)");
    d = cb::dockKeys(700, 300, 970, 600, 0, 0, 820, 0); ok(!d.ok, "no column width: not docked");
    std::printf("ColumnBoard 1.9.1: %d passed, %d failed\n", P, F);
    return F ? 1 : 0;
}
