// test_columnboard_190.cpp - ColumnBoard 1.9.0 (2026-10-28 12:16-12:18 CT, Rassul "a column for levels ... move the hod lod box aligned with the liquidity
// profile on the left"): the KEY LEVELS column at the pane's LEFT edge (lsDealerProfile 2.33.0) - board key, freshness, leftKeep with / without it,
// the HOD/LOD table dock (top-left inside the column, under the IRT title row), and the 1.8.0 right side unchanged.   g++ -std=c++17 -I. test_columnboard_190.cpp && ./a.out
#include "ColumnBoard.h"
#include <cstdio>
static int P = 0, F = 0;
static void ok(bool c, const char* m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m); } }
int main()
{
    ok(std::string(cb::VERSION) == "1.9.0" && cb::KEYS_LEFT && !cb::LEVELS_LEFT && cb::LEVELS_OUTER, "1.9.0: Key Levels left, liquidity right outside Delta");
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
    ok(cb::leftKeep("ESZ6", 180, "k", 0, 1500, false) == 0, "lsHodLod (withKeys false): the column is not a keep-out");
    ok(cb::leftKeep("ESZ6", 180, "k", 0, 12001) == 0, "stale after 10 s: the keep-out is gone");
    ok(cb::leftKeep("ESZ6", 60, "k", 0, 1500) == 0 && cb::leftKeep("NQZ6", 180, "k", 0, 1500) == 0 && cb::leftKeep("ESZ6", 180, "other", 0, 1500) == 0, "another bar size / symbol / chart: no effect");
    cb::publishKeys("ESZ6", 180, "k", 2000, 9999); ok(cb::keysFresh("ESZ6", 180, "k", 2000, w) && w == cb::keysRange().def, "an out-of-range width is cleaned to the default");
    cb::publishKeys("ESZ6", 180, "k", 2000, 300); ok(cb::keysFresh("ESZ6", 180, "k", 2000, w) && w == 300, "300 (the table's width) accepted");
    cb::clearKeys("ESZ6", 180, "k"); ok(!cb::keysFresh("ESZ6", 180, "k", 2000, w), "cleared: gone");
    ok(cb::cleanKeysW(50) == 230 && cb::cleanKeysW(460) == 460 && cb::cleanKeysW(120) == 120, "keys range 120-460, default 230");
    // the dock: a 270 x 300 table + strip somewhere mid-chart -> top-left of the column, under the title row
    cb::Dock d = cb::dockKeys(700, 300, 970, 600, 0, 0, 820);
    ok(d.ok && 700 + d.dx == 3 && 300 + d.dy == 24, "dock: table top-left at (3, 24) inside the column (title row 22 px clear)");
    d = cb::dockKeys(3, 24, 273, 324, 0, 0, 820); ok(d.ok && d.dx == 0 && d.dy == 0, "already docked: no move (no jitter)");
    d = cb::dockKeys(100, 10, 370, 900, 0, 0, 820); ok(!d.ok, "a table taller than the pane: not docked (keeps its Position)");
    d = cb::dockKeys(100, 10, 100, 50, 0, 0, 820); ok(!d.ok, "an empty table: not docked");
    d = cb::dockKeys(500, 100, 760, 300, 40, 10, 150); ok(d.ok == false, "a short pane (140 px) with a 200 px table: not docked");
    d = cb::dockKeys(500, 100, 760, 200, 40, 10, 190); ok(d.ok && 500 + d.dx == 43 && 100 + d.dy == 12, "a pane under 200 px: no title row reserve");
    std::printf("ColumnBoard 1.9.0: %d passed, %d failed\n", P, F);
    return F ? 1 : 0;
}
