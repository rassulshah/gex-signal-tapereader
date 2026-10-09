#include "DayStatsLogic.h"
#include <cmath>
#include <cstdio>
#include <limits>
#include <sstream>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(condition, message) do { if (condition) { ++passes; std::printf("  ok    %s\n", message); } else { ++fails; std::printf("  FAIL  %s (line %d)\n", message, __LINE__); } } while (0)

static std::vector<std::string> split(const std::string& line)
{
    std::vector<std::string> fields; std::stringstream ss(line); std::string field;
    while (std::getline(ss, field, ',')) fields.push_back(field);
    return fields;
}

int main()
{
    std::printf("test_daystats_regressions — external input validation\n");
    const std::string row = "DAYSA,HOD,7741.00,38880,138,12,50,33600,18,201,LOD,7617.00,52200,222,124.0,6200,,";

    double value = 0.0;
    CHECK(dsl::parseFinite(" \t12.5\r", value) && value == 12.5, "finite parser accepts whitespace/CR-delimited CSV values");
    CHECK(!dsl::parseFinite("12.5junk", value) && !dsl::parseFinite("nan", value) && !dsl::parseFinite("inf", value), "finite parser refuses suffixes and nonfinite values");

    unsigned int sameTimestampSkips = 0;
    CHECK(dsl::cacheRefreshDue(false, sameTimestampSkips) && sameTimestampSkips == 0, "a new SDK timestamp loads immediately and resets the cache gate");
    bool cached = true;
    for (unsigned int i = 0; i < dsl::kMaxSameDateCacheSkips; ++i) cached = cached && !dsl::cacheRefreshDue(true, sameTimestampSkips);
    CHECK(cached && sameTimestampSkips == dsl::kMaxSameDateCacheSkips, "same-timestamp repaint bursts are cached for the bounded skip count");
    CHECK(dsl::cacheRefreshDue(true, sameTimestampSkips) && sameTimestampSkips == 0, "a same-timestamp CSV correction is reloaded after the bounded cache interval");

    dsl::StatRow stats;
    CHECK(dsl::parseStatRow(split(row), stats) && stats.valid, "well-formed DAYSA row still parses");
    const std::string oldFirst = stats.first;
    std::vector<std::string> badNumber = split(row); badNumber[3] = "nan";
    CHECK(!dsl::parseStatRow(badNumber, stats) && stats.valid && stats.first == oldFirst, "bad numeric row is rejected without clobbering caller state");
    std::vector<std::string> badClock = split(row); badClock[12] = "86400";
    CHECK(!dsl::parseStatRow(badClock, stats), "out-of-day clock is rejected");
    std::vector<std::string> badTag = split(row); badTag[0] = "OTHER";
    CHECK(!dsl::parseStatRow(badTag, stats), "unknown row tag is rejected");
    std::vector<std::string> spaced = split(row); spaced[1] = " HOD "; spaced[10] = " LOD "; spaced[15] = "6200\r";
    dsl::StatRow normalized;
    CHECK(dsl::parseStatRow(spaced, normalized) && normalized.first == "HOD" && normalized.second == "LOD" && normalized.rngUsd == "6200", "valid whitespace/CR fields normalize safely");

    CHECK(dsl::clk(86400.0) == "--" && dsl::clk(std::numeric_limits<double>::quiet_NaN()) == "--", "invalid clocks do not wrap or convert nonfinite values");
    CHECK(dsl::dur(std::numeric_limits<double>::infinity()) == "--" && dsl::px1("not-a-price", 0.0) == "--" && dsl::pct(101.0, false) == "--", "invalid display numerics are unavailable rather than coerced");
    CHECK(dsl::usd(std::numeric_limits<double>::infinity()) == "--" && dsl::rngCell("not-money", "nan", false) == "--", "currency/range formatters reject nonfinite or malformed input");

    dsl::Cond cond;
    CHECK(!dsl::parseCond(split("CONDE,pos60,1,2,3,4,39junk,29"), cond) && cond.lastHr == -1, "CONDE does not coerce malformed ladder percentages to zero");
    CHECK(!dsl::parseCond(split("CONDE,pos60,1,2,3,4,39,29,1441"), cond), "CONDE rejects impossible percentile minute values");

    dsl::Read2 read2;
    CHECK(!dsl::parseRead2(split("READ2,LOD,50junk,0.2,20,1,0.2,9,4,baked"), read2), "READ2 rejects partial integer probabilities");
    CHECK(!dsl::parseRead2(split("READ2,LOD,50,nan,20,1,0.2,9,4,baked"), read2), "READ2 rejects nonfinite model fields");
    CHECK(!dsl::parseRead2(split("READ2,LOD,50,0.2,20,1,1.2,9,4,baked"), read2), "READ2 rejects a share outside zero to one");
    CHECK(dsl::parseRead2(split("READ2,LOD,25,0.2,20,1,0.2,9,4,baked"), read2), "well-formed READ2 remains accepted");
    CHECK(dsl::secondLine(read2, std::numeric_limits<double>::quiet_NaN(), false, -1) == "LOD IN 25%", "invalid ASOF cannot manufacture an arrival clock");

    dsl::Read read;
    CHECK(dsl::parseRead(split("READ,HOD,61.5,47,12,IN\r"), read) && read.valid && read.call == "IN", "READ parser validates and trims a valid classifier row");
    CHECK(!dsl::parseRead(split("READ,HOD,61junk,47,12,IN"), read) && !read.valid, "READ parser rejects a malformed probability");
    CHECK(!dsl::parseRead(split("READ,HOD,61,47,12,MAYBE"), read), "READ parser rejects an unknown call state");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    int yy[2] = {2026, 2026}, mm[2] = {9, 9}, dd[2] = {18, 18};
    double sod[2] = {30600.0, 30780.0}; float op[2] = {7700.0f, 7701.0f};
    float hi[2] = {(float)nan, 7710.0f}, lo[2] = {7690.0f, 7695.0f};
    dsl::Ext ext;
    CHECK(dsl::chartSession(yy, mm, dd, sod, op, hi, lo, 2, 2026, 9, 18, false, ext) && ext.open == 7701.0 && ext.hi == 7710.0 && ext.lo == 7695.0, "chart session skips a nonfinite SDK bar before aggregation");
    double stamps[3] = {57600.0, nan, 50000.0};
    CHECK(dsl::endStampedSods(stamps, 3), "end-stamp detection ignores a nonfinite timestamp without conversion");

    std::printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
