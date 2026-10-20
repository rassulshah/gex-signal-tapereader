// Aggression 1.17.0 / Book Flow 1.7.0: the scale cap (OrderFlowFeed capTop)
#include "OrderFlowFeed.h"
#include <cstdio>
static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; printf("FAIL %s\n", m); } } while (0)
int main() {
    std::vector<long long> v; for (int i = 1; i <= 19; ++i) v.push_back(1000); v.push_back(16000);   // 19 normal bars + one 16,000 open
    const long long c = off::capTop(v);
    CHECK(c == 1500, "cap = 1.5x the 90th percentile (1,000) = 1,500 - the outlier no longer sets the scale");
    CHECK(off::capped(16000, c) == 1500 && off::capped(800, c) == 800, "capped / not capped");
    CHECK(off::capTop(std::vector<long long>(9, 500)) == LLONG_MAX, "fewer than 10 bars: no cap");
    std::vector<long long> z(30, 0); z[3] = 100; CHECK(off::capTop(z) == LLONG_MAX, "zeros are not bars");
    std::vector<long long> u; for (int i = 1; i <= 100; ++i) u.push_back(i * 10);
    CHECK(off::capTop(u) == (long long)std::llround(1.5 * 900), "100 bars 10..1000: p90 = 900 -> 1,350");
    // a capped bar fills its side and sits at the edge
    off::NetGeo n = off::netGeoFor(200, 4, 1500, 1500);
    CHECK(off::barHeight(16000, n.upTop, n.maxPx) == n.maxPx && off::barHeight(1500, n.upTop, n.maxPx) == n.maxPx && off::barHeight(750, n.upTop, n.maxPx) < n.maxPx, "capped bars stop at the edge");
    // (1.17.0) the feed watchdog rule
    CHECK(off::feedSilent(true, 1, 1000, 40000, 40000), "open, subscribed, 39 s without an event while the chart traded: silent");
    CHECK(!off::feedSilent(true, 1, 1000, 4000, 40000), "no trade after the last event (+5 s): quiet market, not silent");
    CHECK(!off::feedSilent(true, 1, 20000, 40000, 40000), "20 s: not yet");
    CHECK(!off::feedSilent(false, 1, 1000, 40000, 40000) && !off::feedSilent(true, 0, 1000, 40000, 40000) && !off::feedSilent(true, 1, -1, 40000, 40000), "closed / not subscribed / never had an event: not judged here");
    CHECK(off::reconnectDue(-1, 5) && !off::reconnectDue(1000, 30000) && off::reconnectDue(1000, 61000), "reconnect at most once a minute");
    CHECK(off::silentRead(0, 2).find("Order feed silent") == 0 && off::silentRead(0, 2).find("(2)") != std::string::npos, "the silent read text");
    {   // (Effort Reward 1.24.0 / Defense 1.14.0, Rassul 14:22 "if i move it dont the bars move also ?") the y-axis drives the bars
        const int T = 0, Bm = 200, lo = 21, hi = 199;                    // pane rows 0..200, 20 rows reserved on top
        off::ValY m = off::valYOf(199, 99, 1000, T, Bm);                 // IRT: value 0 at row 199, 1,000 at row 99 -> 0.1 px a lot
        CHECK(m.ok && m.ppu == 0.1 && m.yOf(500) == 149, "the mapping: 500 lots = row 149");
        int yt = 0, yb = 0; bool cap = false;
        CHECK(off::axisBar(m, lo, hi, 500, yt, yb, cap) == 51 && yt == 149 && yb == 199 && !cap, "a 500 bar: 51 px from the zero row");
        off::ValY z = off::valYOf(199, 149, 1000, T, Bm);                // he drags the axis: 1,000 now at row 149 (zoomed out by 2)
        CHECK(z.ok && off::axisBar(z, lo, hi, 500, yt, yb, cap) == 26, "dragged: the same 500 bar is half as tall (26 px)");
        off::ValY zi = off::valYOf(199, -101, 1000, T, Bm);              // zoomed in: 1,000 above the pane
        CHECK(zi.ok && off::axisBar(zi, lo, hi, 1000, yt, yb, cap) > 0 && cap && yt == lo, "zoomed in: a bar past the top is clipped with the white tip");
        off::ValY up = off::valYOf(150, 50, 1000, T, Bm);                // dragged up: zero at row 150
        CHECK(up.ok && off::axisBar(up, lo, hi, 500, yt, yb, cap) == 51 && yb == 150, "the zero row moves with the axis");
        CHECK(!off::valYOf(199, 250, 1000, T, Bm).ok && !off::valYOf(140000, 139900, 1000, T, Bm).ok && !off::valYOf(199, 99, 0, T, Bm).ok, "upside down / zero far off the pane / no scale: the old pixel bars"); }
    printf("scale cap + watchdog: %d checks, %d failed\n", p + f, f); return f ? 1 : 0;
}
