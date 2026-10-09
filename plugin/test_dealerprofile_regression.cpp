/********************************************************************************
 * test_dealerprofile_regression.cpp -- SDK-free regressions for DealerProfile
 *
 * Linux:  g++ -std=c++14 -Wall -Wextra -Werror -pedantic \
 *             test_dealerprofile_regression.cpp -o /tmp/test_dp && /tmp/test_dp
 ********************************************************************************/
#include "DealerProfileAuditLogic.h"

#include <cmath>
#include <cstdio>
#include <string>

static int failures = 0;
static int passes = 0;
#define CHECK(condition, message) do { \
    if (condition) { ++passes; std::printf("  ok    %s\n", message); } \
    else { ++failures; std::printf("  FAIL  %s (line %d)\n", message, __LINE__); } \
} while (0)

int main()
{
    std::printf("test_dealerprofile_regression\n");

    float price = 0.0f;
    CHECK(dpaudit::parsePositiveFinite("4201.50", price) && std::fabs(price - 4201.5f) < 0.001f,
          "valid external price is retained");
    CHECK(dpaudit::parsePositiveFinite(" \t6.5970 \r", price) && std::fabs(price - 6.597f) < 0.0001f,
          "surrounding CSV whitespace is accepted");
    CHECK(!dpaudit::parsePositiveFinite("4200junk", price),
          "partially numeric external price is rejected instead of silently drawn");
    CHECK(!dpaudit::parsePositiveFinite("NaN", price) && !dpaudit::parsePositiveFinite("inf", price),
          "non-finite external prices are rejected");
    CHECK(!dpaudit::parsePositiveFinite("1e999", price) && !dpaudit::parsePositiveFinite("0", price) &&
              !dpaudit::parsePositiveFinite("-1", price) && !dpaudit::parsePositiveFinite("", price),
          "overflow, non-positive, and empty external prices are rejected");

    const std::string target = "C:\\InvestorRT\\rtx\\lsFlexLevels\\LRA-IRT-Bars-GC.csv";
    const std::string first = dpaudit::exportTempPath(target, (std::uintptr_t)0x10);
    const std::string second = dpaudit::exportTempPath(target, (std::uintptr_t)0x20);
    CHECK(first != target && second != target && first != second,
          "simultaneous chart states use distinct temporary export paths");
    CHECK(first.find(target + ".tmp.") == 0 && second.find(target + ".tmp.") == 0,
          "temporary exports remain beside the intended target");

    std::printf("\n%d passed, %d failed\n", passes, failures);
    return failures;
}
