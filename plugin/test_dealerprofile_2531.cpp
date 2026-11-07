// test_dealerprofile_2531.cpp -- DealerProfile 2.53.1: the once-per-session key-level history (LRA-KeyLevels-<MKT>-hist.csv)
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_dealerprofile_2531.cpp -o t && ./t
#include "DealerProfileAuditLogic.h"
#include <cstdio>
#include <string>
#include <vector>
static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
int main()
{
    // due: 3-min chart only, 10:00 <= bar end < 17:00 CT
    CHECK(dpaudit::histDue(180, 10 * 3600) && dpaudit::histDue(180, 16 * 3600 + 57 * 60), "3-min, 10:00 and 16:57 due");
    CHECK(!dpaudit::histDue(180, 9 * 3600 + 57 * 60) && !dpaudit::histDue(180, 17 * 3600) && !dpaudit::histDue(180, 2 * 3600), "before 10:00 / from 17:00 not due");
    CHECK(!dpaudit::histDue(60, 11 * 3600) && !dpaudit::histDue(3600, 11 * 3600) && !dpaudit::histDue(0, 11 * 3600), "other bar sizes never write");
    // dates: civil days -> YYYY-MM-DD (1970-01-01 = 0)
    CHECK(dpaudit::histDate(0) == "1970-01-01", "epoch day");
    CHECK(dpaudit::histDate(20764) == "2026-11-07", "2026-11-07");
    CHECK(dpaudit::histDate(20513) == "2026-03-01" && dpaudit::histDate(20512) == "2026-02-28", "month edge");
    CHECK(dpaudit::histDate(-5).empty(), "before 1970 = empty");
    // the line
    std::vector<dpaudit::KeyLv> v = { { "ONH/LonHI", 4062.2, false }, { "PDL", 3933.0, false }, { "bad|name", 1.0, false }, { "NaN", NAN, false }, { "Neg", -1.0, false } };
    const std::string ln = dpaudit::histLine("2026-11-06", 1794000000LL, v);
    CHECK(ln == "2026-11-06|1794000000|ONH/LonHI=4062.200000;PDL=3933.000000\n", "line: names / prices, bad entries dropped");
    CHECK(dpaudit::histLine("2026-11-6", 1, v).empty() && dpaudit::histLine("2026-11-06", 1, {}).empty(), "bad session / no levels = no line");
    // dedupe on the tail
    CHECK(dpaudit::histHas("2026-11-05|1|PDL=1\n2026-11-06|2|PDL=2\n", "2026-11-06"), "session in tail");
    CHECK(!dpaudit::histHas("2026-11-05|1|PDL=1\n", "2026-11-06") && !dpaudit::histHas("", "2026-11-06"), "not in tail / empty file");
    CHECK(!dpaudit::histHas("2026-11-066|x\n", "2026-11-06") && !dpaudit::histHas("x2026-11-06|1\n", "2026-11-06"), "prefix must be the whole field");
    CHECK(dpaudit::histHas("torn line without newline 2026\n2026-11-06|5|A=1", "2026-11-06"), "last line without newline still counts");
    std::printf("RESULT %d passed, %d failed\n", p, f);
    return f ? 1 : 0;
}
