// tests_zd/test_mboquiet.cpp - MboQuiet.h 1.0.0 (g++ -std=c++17 -I.. test_mboquiet.cpp -o tq && ./tq)
#include "MboQuiet.h"
#include <cstdio>
static int fails = 0;
#define CHECK(c, m) do { if (c) std::printf("PASS %s\n", m); else { std::printf("FAIL %s\n", m); ++fails; } } while (0)
int main() {
    const int MON = 1, FRI = 5, SAT = 6, SUN = 0;
    auto T = [](int h, int m) { return h * 60 + m; };
    const long long base = 1794000000;   // any epoch; only differences matter
    // today: the feed went silent at 7:33, he looked at 7:45 -> "MBO QUIET 12m"
    long long last = base, now = base + 12 * 60;
    CHECK(mbq::text(last, now, T(7, 45), MON) == "MBO QUIET 12m", "7:33 silence seen at 7:45 = QUIET 12m");
    CHECK(mbq::text(base, base + 60, T(7, 34), MON).empty(), "1 min silence in the day = nothing");
    CHECK(mbq::text(base, base + 120, T(7, 35), MON) == "MBO QUIET 2m", "2 min in the day = quiet");
    CHECK(mbq::text(base, base + 4 * 60, T(2, 0), MON).empty(), "4 min overnight = nothing (NG / GC can pause ~3 min)");
    CHECK(mbq::text(base, base + 5 * 60, T(2, 0), MON) == "MBO QUIET 5m", "5 min overnight = quiet");
    CHECK(mbq::text(base, base + 30 * 60, T(16, 30), MON).empty(), "daily halt: no check");
    CHECK(mbq::text(base, base + 99 * 60, T(12, 0), SAT).empty(), "Saturday: no check");
    CHECK(mbq::text(base, base + 99 * 60, T(16, 30), FRI).empty(), "Friday after 16:00: no check");
    CHECK(mbq::text(base, base + 99 * 60, T(10, 0), SUN).empty(), "Sunday before the open: no check");
    // reopen at 17:00: the last event was 15:59 -> at 17:03 only 3 min count (overnight threshold 5) = nothing; at 17:06 = 6m
    CHECK(mbq::text(base, base + 64 * 60, T(17, 3), MON).empty(), "17:03 after the halt: counted from the 17:00 open");
    CHECK(mbq::text(base, base + 67 * 60, T(17, 6), MON) == "MBO QUIET 6m", "17:06 with nothing since the open = QUIET 6m");
    CHECK(mbq::thresholdMin(T(7, 0), MON) == 2 && mbq::thresholdMin(T(6, 59), MON) == 5 && mbq::thresholdMin(T(15, 0), MON) == 5, "day window 07:00-15:00");
    CHECK(mbq::minutesOpen(T(18, 0), SUN) == 60 && mbq::minutesOpen(T(8, 0), MON) == 900, "minutes open");
    CHECK(mbq::text(0, base, T(8, 0), MON) == "MBO QUIET 900m", "no event ever since the open (feed dead from the start)");
    std::printf(fails ? "FAILED %d\n" : "ALL PASSED\n", fails);
    return fails ? 1 : 0;
}
