/*******************************************************************************
 * Focused audit regressions for KingTrackerLogic.h.
 * Run with: g++ -std=c++14 -Wall -Wextra -Wpedantic -I. test_kingtracker_logic_audit.cpp
 ******************************************************************************/
#include "KingTrackerLogic.h"
#include <cmath>
#include <cstdio>
#include <limits>
#include <sstream>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) { ++passes; std::printf("  ok    %s\n", msg); } else { ++fails; std::printf("  FAIL  %s (line %d)\n", msg, __LINE__); } } while (0)

static std::vector<std::string> split(const std::string& line)
{
    std::vector<std::string> fields;
    std::stringstream input(line);
    std::string field;
    while (std::getline(input, field, ',')) fields.push_back(field);
    return fields;
}

int main()
{
    std::printf("test_kingtracker_logic_audit — external-input and ordering regressions\n");

    std::string family, book;
    ktl::Step step;
    CHECK(ktl::parseTrack(split("KINGTRACK,ES,SPX,34200.5,6000.25,5995,-100\r"), family, book, step) &&
          family == "ES" && book == "SPX" && std::fabs(step.so - 34200.5) < 0.001 && step.strike == 5995 && step.pct == -100,
          "well-formed KINGTRACK accepts finite values and CRLF trailing whitespace");
    ktl::Step preserved = step;
    CHECK(!ktl::parseTrack(split("KINGTRACK,ES,SPX,not-a-clock,6000,5995"), family, book, step) && step.strike == preserved.strike,
          "malformed KINGTRACK clock is rejected without mutating its output step");
    CHECK(!ktl::parseTrack(split("KINGTRACK,ES,SPX,86400,6000,5995"), family, book, step),
          "out-of-day KINGTRACK clock is rejected");
    CHECK(!ktl::parseTrack(split("KINGTRACK,ES,SPX,10,nan,5995"), family, book, step) &&
          !ktl::parseTrack(split("KINGTRACK,ES,SPX,10,6000x,5995"), family, book, step) &&
          !ktl::parseTrack(split("KINGTRACK,ES,SPX,10,6000,0"), family, book, step),
          "NaN, numeric junk, and zero-strike KINGTRACK rows are rejected");
    CHECK(!ktl::parseTrack(split("KINGTRACK,RTY,SPX,10,6000,5995"), family, book, step),
          "unsupported KINGTRACK family is rejected");

    ktl::Book now;
    CHECK(ktl::parseNow(split("KINGNOW,NQ,IFQ,21000.5,20950,"), family, book, now) && now.hasNow && now.nowPct == 0,
          "KINGNOW accepts an omitted optional polarity");
    const float originalNow = now.nowPx;
    CHECK(!ktl::parseNow(split("KINGNOW,NQ,IFQ,inf,20950,-100"), family, book, now) && now.nowPx == originalNow,
          "infinite KINGNOW price is rejected without replacing the last valid state");

    int y = 0, m = 0, d = 0;
    CHECK(ktl::parseYmd("2028-02-29", y, m, d) && y == 2028 && m == 2 && d == 29 &&
          !ktl::parseYmd("2027-02-29", y, m, d) && !ktl::parseYmd("2026-13-01", y, m, d),
          "SCALEREF dates require a real calendar day");

    ktl::Book unordered;
    ktl::Step late; late.so = 500; late.strike = 5100; late.px = 5101.0f; late.pct = 1;
    ktl::Step early; early.so = 100; early.strike = 5000; early.px = 5001.0f; early.pct = -1;
    ktl::Step tieHigh; tieHigh.so = 500; tieHigh.strike = 5200; tieHigh.px = 5201.0f; tieHigh.pct = 1;
    unordered.steps.push_back(late); unordered.steps.push_back(tieHigh); unordered.steps.push_back(early);
    ktl::sortSteps(unordered);
    CHECK(unordered.steps[0].so == 100 && unordered.steps[1].strike == 5100 && unordered.steps[2].strike == 5200,
          "out-of-order and equal-clock steps get a deterministic chronological order");

    float offset = 0.0f;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    CHECK(!ktl::offsetFor(nan, 6000.0f, offset) && !ktl::offsetFor(6000.0f, nan, offset) &&
          !ktl::anchorPrice(false, 0.0f, true, nan, offset),
          "non-finite close or anchor cannot create a contract offset");
    CHECK(!ktl::bookDrawn("UNKNOWN", "ES", "ES", false), "unknown book cannot be reported as a Skylit drawing");
    CHECK(ktl::staleAge(std::numeric_limits<double>::quiet_NaN(), 60.0) < 0.0 && ktl::staleAge(60.0, 86400.0) < 0.0,
          "invalid stale-clock values remain unavailable");

    std::printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
