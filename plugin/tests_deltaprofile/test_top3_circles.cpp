// test_top3_circles.cpp - dp::topCircles (lsDeltaProfile 2.37.0): one circle per top-3 row, on its peak candle, in the row's colour.
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. test_top3_circles.cpp -o t3 && ./t3
#include "DeltaProfileCore.h"
#include <iostream>
namespace dp = delta_profile;
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static dp::Bar bar(int idx, std::initializer_list<dp::Row> rows) { dp::Bar b; b.index = idx; b.time = 1000 + idx * 180; b.closed = true; b.open = b.close = 100; b.high = 101; b.low = 99; b.rows = rows; return b; }
static dp::Bucket bk(dp::Tick k, double d) { dp::Bucket b; b.key = k; b.low = k; b.high = k; b.delta = d; b.volume = std::fabs(d) * 2; return b; }
int main() {
    // rows (1 tick each): 400 = +500 (peak on bar 12), 404 = -900 (peak on bar 11), 408 = +300 (bar 13), 412 = -100 (4th: no circle)
    std::vector<dp::Bar> B = { bar(10, { {400, 100, 300}, {404, -200, 400} }), bar(11, { {400, 50, 200}, {404, -600, 900}, {412, -100, 200} }),
                               bar(12, { {400, 350, 500}, {404, -100, 300}, {408, 100, 200} }), bar(13, { {400, -40, 90}, {408, 200, 300} }) };
    std::vector<dp::Bucket> K = { bk(400, 460), bk(404, -900), bk(408, 300), bk(412, -100) };
    const std::vector<dp::TopCircle> T = dp::topCircles(B, K, 0.25);
    CHECK(T.size() == 3, "three circles for the three biggest rows (" << T.size() << ")");
    if (T.size() == 3) {
        CHECK(T[0].key == 404 && !T[0].buy && T[0].peakBar == 11 && T[0].price == 101.0, "the biggest row: a sell row, its circle on the candle with its biggest selling, at its price (" << T[0].peakBar << ", " << T[0].price << ")");
        CHECK(T[1].key == 400 && T[1].buy && T[1].peakBar == 12, "a buy row: the candle with its biggest BUYING (the -40 bar is the other side) (" << T[1].peakBar << ")");
        CHECK(T[2].key == 408 && T[2].buy && T[2].peakBar == 13, "the third row (" << T[2].peakBar << ")"); }
    K[3].delta = -5000; const std::vector<dp::TopCircle> U = dp::topCircles(B, K, 0.25);
    CHECK(U.size() == 3 && U[0].key == 412 && U[2].key == 400, "live: a new top-3 row takes a circle, the smallest loses it");
    CHECK(dp::topCircles(B, K, 0).empty() && dp::topCircles(B, {}, 0.25).empty() && dp::topCircles({}, K, 0.25).empty(), "no tick / no rows / no candles = no circles");
    std::vector<dp::Bucket> Z = { bk(400, 0), bk(404, -900) }; CHECK(dp::topCircles(B, Z, 0.25).size() == 1, "a row with no net delta has no colour and no circle");
    std::vector<dp::Bucket> W = { bk(400, 460) }; W[0].low = 400; W[0].high = 403; const std::vector<dp::TopCircle> V = dp::topCircles(B, W, 0.25);
    CHECK(V.size() == 1 && V[0].price == (400 + 403) / 2.0 * 0.25, "a wider row: the circle at its middle price");
    std::cout << "test_top3_circles: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
