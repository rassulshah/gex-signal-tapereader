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
    printf("scale cap: %d checks, %d failed\n", p + f, f); return f ? 1 : 0;
}
