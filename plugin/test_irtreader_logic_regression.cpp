// Regression tests for finite-safe IRTReader DOM book hashing.
#include "IRTReaderLogic.h"
#include <cstdio>
#include <limits>
#include <vector>

static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("  ok    %s\n", m); } else { ++fails; std::printf("  FAIL  %s\n", m); } } while (0)

int main()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const std::vector<double> normal = { 0.0, 1.0, 2.0, 3.0 };
    const std::vector<double> nanBook = { nan, 1.0, 2.0, 3.0 };
    const std::vector<double> nanBookAgain = { nan, 1.0, 2.0, 3.0 };
    const std::vector<double> plusInf = { inf, 1.0, 2.0, 3.0 };
    const std::vector<double> minusInf = { -inf, 1.0, 2.0, 3.0 };
    const std::vector<double> huge = { (std::numeric_limits<double>::max)(), 1.0, 2.0, 3.0 };

    CHECK(irl::bookHash(nanBook) == irl::bookHash(nanBookAgain), "NaN-containing snapshots have a deterministic fingerprint");
    CHECK(irl::bookHash(nanBook) != irl::bookHash(normal), "NaN cannot be mistaken for an ordinary zero level");
    CHECK(irl::bookHash(plusInf) != irl::bookHash(minusInf), "positive and negative infinity remain distinguishable");
    CHECK(irl::bookHash(huge) == irl::bookHash(huge), "out-of-range finite input is clamped deterministically");
    std::printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
