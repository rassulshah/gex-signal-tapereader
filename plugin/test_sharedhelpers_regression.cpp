#include "DealerLogic.h"
#include "ContractOffsetLogic.h"
#include <cmath>
#include <cstdio>
#include <limits>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("  ok    %s\n", m); } else { ++fails; std::printf("  FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

int main()
{
    std::printf("test_sharedhelpers_regression\n");
    {
        dl::Data d;
        CHECK(!dl::parseLine(d, "ASOF|nan|2026-10-01") && d.asofSo < 0, "non-finite ASOF is rejected");
        CHECK(!dl::parseLine(d, "PRICE|inf|55") && !d.hasPrice, "non-finite PRICE is rejected");
        CHECK(!dl::parseLine(d, "NODE|4200|nan|1|2|3|4|0") && d.nodes.empty(), "non-finite node geometry is rejected");
        CHECK(!dl::parseLine(d, "NODE|4200|1|1|2|3|4|0||||20|") && d.nodes.empty(), "partial net-GEX pair is rejected");
        CHECK(!dl::parseLine(d, "WALLROW|SPEED|nan|x|ok|why") && d.wall.empty(), "non-finite meter is rejected");
        CHECK(!dl::parseLine(d, "FSUM|10:00|10:05|1|2|3|4|nan|1|2|3|4") && !d.hasFsum, "non-finite forced-flow metric is rejected");
        CHECK(!dl::parseLine(d, "GSTR|10:00|4200|1|2|3|4|5|6|7|nan") && !d.hasGstr, "non-finite gamma-strength metric is rejected");
        CHECK(!dl::parseLine(d, "SIG|2026-10-01 10:00:00|SC|B|nan|bad") && d.sigs.empty(), "non-finite signal value is rejected");
    }
    {
        col::Bar timed[2] = {
            { 2026, 10, 1, 9 * 3600 + 27 * 60, 100.0f },
            { 2026, 10, 1, 9 * 3600 + 30 * 60, 0.0f }
        };
        CHECK(col::anchorIndex(timed, 2, -1, 9 * 3600 + 30 * 60, 2026, 10, 1) == 0,
              "timed anchor skips a zero-close bar and uses the prior valid bar");
        col::Bar rth[2] = {
            { 2026, 10, 1, 14 * 3600 + 57 * 60, 100.0f },
            { 2026, 10, 1, 15 * 3600, 0.0f }
        };
        CHECK(col::anchorIndex(rth, 2, 17 * 3600, -1, 0, 0, 0) == 0,
              "RTH fallback skips a zero-close bar and uses the prior valid bar");
        col::Bar nonfinite[2] = {
            { 2026, 10, 1, 9 * 3600 + 27 * 60, 100.0f },
            { 2026, 10, 1, std::numeric_limits<double>::quiet_NaN(), 110.0f }
        };
        CHECK(col::anchorIndex(nonfinite, 2, -1, 9 * 3600 + 30 * 60, 2026, 10, 1) == 0,
              "non-finite bar time is not selected as an anchor");
        CHECK(col::anchorIndex(0, 1, 0, -1, 0, 0, 0) == -1, "null bar array is safe");
        float off = 17.0f;
        CHECK(!col::offsetFor(0.0f, 100.0f, off) && off == 0.0f, "failed offset clears stale output");
        off = 17.0f;
        CHECK(!col::offsetFor(std::numeric_limits<float>::infinity(), 100.0f, off) && off == 0.0f,
              "non-finite offset input is refused and cleared");
        off = 17.0f;
        CHECK(!dl::offsetFor(100.0f, std::numeric_limits<float>::infinity(), off) && off == 0.0f,
              "DealerLogic refuses a non-finite file price and clears stale output");
    }
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails;
}
