#include "GammaProfileLogic.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;
static int passes = 0;

#define CHECK(condition, message) do { \
    if (condition) { ++passes; std::printf("  ok    %s\n", message); } \
    else { ++failures; std::printf("  FAIL  %s (line %d)\n", message, __LINE__); } \
} while (0)

int main()
{
    std::printf("=== GammaProfile audit regression ===\n");

    float f = 0.0f;
    CHECK(gpl::parseFiniteFloat(" \t7614.25 \r", f) && std::fabs(f - 7614.25f) < 0.001f,
          "finite float accepts a complete price with surrounding whitespace");
    CHECK(!gpl::parseFiniteFloat("", f) && !gpl::parseFiniteFloat("7614x", f) &&
          !gpl::parseFiniteFloat("nan", f) && !gpl::parseFiniteFloat("inf", f) &&
          !gpl::parseFiniteFloat("1e9999", f),
          "finite float rejects empty, partial, NaN, infinity, and overflow tokens");

    int i = 0;
    CHECK(gpl::parseInt(" -42 ", i) && i == -42, "integer parser accepts a complete signed integer");
    CHECK(!gpl::parseInt("5.5", i) && !gpl::parseInt("7rank", i) &&
          !gpl::parseInt("999999999999999999999", i),
          "integer parser rejects partial and overflowing rank tokens");
    CHECK(gpl::boundedText("GammaProfile", 5) == "Gamma" && gpl::boundedText("IF", 5) == "IF",
          "untrusted display fields are deterministically bounded without changing short values");
    CHECK(gpl::validProfilePrice(7614.25f) && gpl::validProfilePrice(gpl::MAX_PROFILE_PRICE) &&
          !gpl::validProfilePrice(0.0f) && !gpl::validProfilePrice(gpl::MAX_PROFILE_PRICE + 1.0f),
          "profile price is bounded before chart mapping and integer display conversion");

    std::vector<int> sparseThenDense;
    sparseThenDense.push_back(200);
    sparseThenDense.push_back(160);
    sparseThenDense.push_back(155);
    sparseThenDense.push_back(150);
    CHECK(gpl::autoBarHForRows(sparseThenDense) == gpl::autoBarH(5) &&
          gpl::autoBarHForRows(sparseThenDense) == 3,
          "Auto thickness follows the nearest adjacent row, not the first sparse gap");
    std::vector<int> oneRow(1, 200);
    CHECK(gpl::autoBarHForRows(oneRow) == 6, "Auto thickness retains the safe single-row fallback");

    std::printf("=== %d passed, %d failed ===\n", passes, failures);
    return failures;
}
