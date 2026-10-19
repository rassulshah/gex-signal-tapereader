// lsBookFlow 1.5.1 (Rassul 2026-10-19 13:37 "a lot of black space"): the zero line where the data needs it (OrderFlowFeed.h netGeoFor)
#include "OrderFlowFeed.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) passes++; else { fails++; printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace off;
int main()
{
    const int H = 160, R = 4;
    NetGeo old = netGeoFor(H, R);
    CHECK(old.y0 == (old.yTop + old.yBot) / 2 && old.downPx == old.maxPx, "not given: the old even split (other panes unchanged)");
    NetGeo a = netGeoFor(H, R, 4000, 0);
    CHECK(a.y0 >= a.yBot - LABEL_LH - 16 && a.y0 <= a.yBot - LABEL_LH - 12, "no negative net: zero line at the bottom, only the marker strip below");
    CHECK(a.maxPx > old.maxPx * 2 - 10, "no negative net: the bars get about twice the height");
    CHECK(barHeight(4000, a.upTop, a.maxPx) == a.maxPx, "the biggest bar fills the up area");
    CHECK(a.yTop + 1 < a.y0 - a.maxPx - LABEL_ROOM + 3, "label room kept above the tallest bar");
    NetGeo b = netGeoFor(H, R, 3000, 1000);
    CHECK(b.downPx > 0 && b.maxPx > b.downPx * 2, "some negative net: the split follows the sizes (3:1)");
    const int hu = barHeight(1000, b.upTop, b.maxPx), hd = barHeight(1000, b.downTop, b.downPx);
    CHECK(std::abs(hu - hd) <= 1, "one scale: 1,000 up as tall as 1,000 down");
    CHECK(b.y0 + b.downPx <= b.yBot - LABEL_LH - 10, "the marker strip stays under the deepest down bar");
    NetGeo c = netGeoFor(H, R, 0, 2000);
    CHECK(c.maxPx >= 6 && c.downPx > 50, "only negatives: most of the room below");
    NetGeo d = netGeoFor(H, R, 5000, 3);
    CHECK(d.downPx >= 6 && barHeight(3, d.downTop, d.downPx) >= 1, "a tiny negative still drawn (min 6 px area)");
    NetGeo e = netGeoFor(30, 0, 100, 0);
    CHECK(e.maxPx >= 1 && e.y0 > e.yTop && e.y0 <= e.yBot, "a short pane stays sane");
    double mn, mx; netAxisFor(4000, H, R, mn, mx, 4000, 0);
    CHECK(mx > 4000 && mn < 0 && -mn < mx * 0.4, "the IRT axis: mostly positive, 0 near the bottom");
    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
