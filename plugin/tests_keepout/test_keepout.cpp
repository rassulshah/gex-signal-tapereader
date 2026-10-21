// test_keepout.cpp -- BoxDock.h 1.2.0 keep-out (Rassul 16:05 CT 2026-10-21 "fix this, it should be solid"): no chart mark paints over
// the lsHodLod table. SDK-free: intersect, segment / horizontal clip, rect pieces, the TableWatch heartbeat (fresh / stale / cleared),
// another chart / symbol / bar size = no effect, the KeepScope (active only inside a draw, nested scopes restore).
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_keepout.cpp -o t && ./t
#include "BoxDock.h"
#include <cstdio>
#include <cstdlib>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static bool insideGrown(const bdk::Box& T, int x, int y) { return bdk::hitsTable(T, x, y, x, y); }
int main() {
    const bdk::Box T = bdk::mk(100, 400, 500, 520);
    // 1. intersect (2 px margin, any corner order, touching the margin counts)
    CHECK(bdk::hitsTable(T, 150, 450, 170, 462), "a label inside the table");
    CHECK(bdk::hitsTable(T, 90, 390, 101, 401), "a mark across the corner");
    CHECK(bdk::hitsTable(T, 502, 450, 520, 460), "a mark 2 px right of the edge (the margin)");
    CHECK(!bdk::hitsTable(T, 503, 450, 520, 460), "3 px right: clear");
    CHECK(!bdk::hitsTable(T, 150, 380, 170, 397), "above the table: clear");
    CHECK(bdk::hitsTable(T, 170, 462, 150, 450), "corner order does not matter");
    CHECK(!bdk::hitsTable(bdk::Box(), 150, 450, 170, 462), "no table: never hits");
    // 2. horizontal clip
    {   int o[2][2]; int n = bdk::hParts(T, 0, 900, 450, o);
        CHECK(n == 2 && o[0][0] == 0 && o[0][1] == 97 && o[1][0] == 503 && o[1][1] == 900, "a level line through the table: two parts, stopping at its edges");
        n = bdk::hParts(T, 200, 900, 450, o); CHECK(n == 1 && o[0][0] == 503 && o[0][1] == 900, "starts inside: the right part only");
        n = bdk::hParts(T, 200, 300, 450, o); CHECK(n == 0, "inside: nothing");
        n = bdk::hParts(T, 0, 900, 300, o); CHECK(n == 1 && o[0][0] == 0 && o[0][1] == 900, "above: whole");
        n = bdk::hParts(T, 900, 0, 450, o); CHECK(n == 2 && o[0][0] == 0, "reversed x");
    }
    // 3. any segment: parts never inside, ends kept
    {   bdk::Seg s[2]; int n = bdk::segParts(T, 0, 450, 900, 450, s);
        CHECK(n == 2 && s[0].x1 == 0 && s[1].x2 == 900 && s[0].x2 <= 97 && s[0].x2 >= 95 && s[1].x1 >= 503 && s[1].x1 <= 505, "horizontal through");
        n = bdk::segParts(T, 50, 300, 600, 650, s); CHECK(n == 2 && s[0].x1 == 50 && s[1].x2 == 600 && !insideGrown(T, s[0].x2, s[0].y2) && !insideGrown(T, s[1].x1, s[1].y1), "diagonal through: both ends outside");
        n = bdk::segParts(T, 50, 300, 90, 350, s); CHECK(n == 1 && s[0].x1 == 50 && s[0].x2 == 90 && s[0].y2 == 350, "a miss: whole");
        n = bdk::segParts(T, 200, 450, 300, 460, s); CHECK(n == 0, "inside: nothing");
        n = bdk::segParts(T, 300, 450, 300, 700, s); CHECK(n == 1 && s[0].x1 == 300 && s[0].y2 == 700 && s[0].y1 > 522, "vertical starting inside: the part below");
        n = bdk::segParts(T, 300, 100, 300, 300, s); CHECK(n == 1 && s[0].y2 == 300, "vertical above: whole");
        n = bdk::segParts(T, 300, 450, 300, 450, s); CHECK(n == 0, "a point inside: nothing");
        srand(7); int bad = 0;
        for (int k = 0; k < 20000; ++k) {   // fuzz: no visible part ever passes through the grown box (sampled every pixel)
            const int x1 = rand() % 700 - 50, y1 = rand() % 800 - 50, x2 = rand() % 700 - 50, y2 = rand() % 800 - 50;
            n = bdk::segParts(T, x1, y1, x2, y2, s);
            for (int i = 0; i < n; ++i) { const int L = std::abs(s[i].x2 - s[i].x1) + std::abs(s[i].y2 - s[i].y1) + 1;
                for (int q = 0; q <= L; ++q) { const double t = (double)q / L; const int x = (int)(s[i].x1 + (s[i].x2 - s[i].x1) * t + 0.5), y = (int)(s[i].y1 + (s[i].y2 - s[i].y1) * t + 0.5); if (bdk::hitsTable(T, x, y, x, y, 1) && !(q == 0 || q == L)) { if (bad < 3) std::printf("seg %d,%d-%d,%d part %d,%d-%d,%d px %d,%d\n", x1,y1,x2,y2,s[i].x1,s[i].y1,s[i].x2,s[i].y2,x,y); ++bad; break; } } }
        }
        CHECK(bad == 0, "fuzz 20,000 segments: no visible pixel within 1 px of the table (rounding at the 2 px margin's corners allowed)");
    }
    // 4. rect pieces
    {   bdk::Box o[4]; int n = bdk::rectParts(T, bdk::mk(0, 380, 900, 600), o);
        int area = 0; bool hit = false; for (int i = 0; i < n; ++i) { area += (o[i].r - o[i].l) * (o[i].b - o[i].t); if (bdk::hitsTable(T, o[i].l, o[i].t, o[i].r - 1, o[i].b - 1)) hit = true; }
        CHECK(n == 4 && !hit, "a band round the table: 4 pieces, none on it");
        CHECK(area == 900 * 220 - 405 * 125, "the pieces cover the band minus the table + margin");
        n = bdk::rectParts(T, bdk::mk(0, 0, 50, 50), o); CHECK(n == 1 && o[0].r == 50, "clear: whole");
        n = bdk::rectParts(T, bdk::mk(200, 420, 300, 500), o); CHECK(n == 0, "inside: nothing");
    }
    // 5. TableWatch: published by lsHodLod for one chart, read with the heartbeat
    {   bdk::TableWatch w; const std::string sym = "ESZ6", chart = "ES 3 min";
        CHECK(!w.refresh(sym, 180, chart, 1000), "nothing published: no table");
        bdk::publishHodLod(sym, 180, chart, 1000, T);
        CHECK(w.refresh(sym, 180, chart, 1001) && w.box.l == 100 && w.box.b == 520, "published: the table");
        bdk::TableWatch other; CHECK(!other.refresh(sym, 180, "NQ 3 min", 1001), "another chart: no table");
        CHECK(!other.refresh("NQZ6", 180, chart, 1001), "another symbol: no table");
        CHECK(!other.refresh(sym, 60, chart, 1001), "another bar size: no table");
        CHECK(w.refresh(sym, 180, chart, 600000) && w.refresh(sym, 180, chart, 600001), "an idle chart (10 min, no new stamp): kept for the first draws");
        bool on = true; for (int k = 0; k < 5; ++k) on = w.refresh(sym, 180, chart, 700000 + k);
        CHECK(!on, "stale: the stamp stopped for 3+ draws and is over 60 s old -> no table");
        bdk::publishHodLod(sym, 180, chart, 700010, T); CHECK(w.refresh(sym, 180, chart, 700011), "it drew again: the table is back");
        for (int k = 0; k < 6; ++k) on = w.refresh(sym, 180, chart, 700020 + k);
        CHECK(on, "stamp unchanged but under 60 s old: still the table (a reader drawing more often than lsHodLod)");
        bdk::clearHodLod(sym, 180, chart); CHECK(!w.refresh(sym, 180, chart, 700030), "cleared (table off / removed): no table at once");
    }
    // 6. KeepScope: active only inside the draw; nested scopes restore; counts
    {   bdk::TableWatch w; w.on = true; w.box = T; bdk::TableWatch none;
        CHECK(!bdk::hide(150, 450, 160, 460), "no scope: nothing hidden");
        {   bdk::KeepScope k(w);
            CHECK(bdk::hide(150, 450, 160, 460) && !bdk::hide(10, 10, 20, 20), "in the scope: a mark on the table is hidden, one elsewhere is not");
            bdk::Seg s[2]; CHECK(bdk::seg(0, 450, 900, 450, s) == 2, "in the scope: a line is cut");
            bdk::Box o[4]; CHECK(bdk::rects(bdk::mk(0, 380, 900, 600), o) == 4, "in the scope: a fill is cut");
            {   bdk::KeepScope k2(none); CHECK(!bdk::hide(150, 450, 160, 460), "a nested scope with no table (another chart's draw)"); }
            CHECK(bdk::hide(150, 450, 160, 460), "the outer scope is restored");
            CHECK(bdk::active().hidden == 2 && bdk::active().clipped == 2, "hidden / clipped counted");
            CHECK(bdk::keepLine().find("UNDER_HODLOD,table on,hidden 2,clipped 2") == 0, "status line");
        }
        CHECK(!bdk::hide(150, 450, 160, 460) && !bdk::active().on, "after the draw: no keep-out");
    }
    CHECK(std::string(bdk::VERSION) == "1.2.0", "BoxDock 1.2.0");
    std::printf("keep-out: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
