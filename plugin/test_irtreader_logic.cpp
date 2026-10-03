// Tests for IRTReaderLogic.h (lsIRTReader) - g++ -std=c++17 test_irtreader_logic.cpp && ./a.out
#include "IRTReaderLogic.h"
#include <cstdio>
static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) { passes++; printf("  ok    %s\n", m); } else { fails++; printf("  FAIL  %s\n", m); } } while (0)
int main()
{
    CHECK(irl::sessionOf(2026, 10, 2, 10) == "2026-10-02", "a Friday morning is Friday's session");
    CHECK(irl::sessionOf(2026, 10, 1, 17) == "2026-10-02", "Thursday 17:00 = Friday's session");
    CHECK(irl::sessionOf(2026, 10, 2, 18) == "2026-10-05", "Friday evening rolls to Monday");
    CHECK(irl::sessionOf(2026, 10, 4, 17) == "2026-10-05", "Sunday's Globex open = Monday's session");
    CHECK(irl::aggressor(4150.2, 4150.1, 4150.2) == 'B' && irl::aggressor(4150.1, 4150.1, 4150.2) == 'S', "at the ask = bought, at the bid = sold");
    CHECK(irl::aggressor(4150.15, 4150.1, 4150.2) == 'M' && irl::aggressor(5, 0, 0) == 'M', "between / no quote = M");
    CHECK(irl::px(6.5325, 0.0005) == "6.5325" && irl::px(4150.1, 0.1) == "4150.1" && irl::px(7754.25, 0.25) == "7754.25" && irl::px(1.16505, 0.00005) == "1.16505", "prices keep the tick's decimals");
    irl::TickCursor c;
    c.rewind(); int w = 0; long long secs[] = {100, 100, 101}; for (long long s : secs) w += c.take(s);
    CHECK(w == 3 && c.lastSec == 101 && c.doneInSec == 1, "first read writes all");
    c.rewind(); w = 0; long long again[] = {100, 100, 101, 101, 102}; for (long long s : again) w += c.take(s);
    CHECK(w == 2 && c.lastSec == 102, "a re-read writes only the new trades (the 2nd trade at 101 and 102)");
    std::vector<double> a = {4150.1, 12, 4150.2, 9}, b = {4150.1, 13, 4150.2, 9};
    CHECK(irl::bookHash(a) != irl::bookHash(b) && irl::bookHash(a) == irl::bookHash(a), "a size change changes the book's fingerprint");
    printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
