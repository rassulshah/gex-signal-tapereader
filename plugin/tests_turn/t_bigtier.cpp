#include "../BigTier.h"
#include <cstdio>
static int c = 0, f = 0;
#define CHECK(x) do { ++c; if (!(x)) { ++f; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
int main() {
    using namespace btr;
    CHECK(tier(31, "GC", true) == "" && tier(32, "GC", true) == "S20" && tier(43, "GC", true) == "S10" && tier(56, "GC", true) == "S5" && tier(500, "GC", true) == "S5");
    CHECK(tier(38, "GC", false) == "" && tier(39, "GC", false) == "S20");
    CHECK(tier(273, "ES", true) == "S20" && tier(124, "ES", false) == "S20" && tier(272, "ES", true) == "");
    CHECK(tier(100, "EU", true) == "S20" && tier(100, "6E", true) == "S20" && tier(1000, "ZZ", true) == "" && tier(0, "ES", true) == "");
    CHECK(lotsText(41, "GC", true) == "41 (S20)" && lotsText(11, "GC", true) == "11");
    // 2026-11-20 9:30 AM CT candle (ends 9:30, starts 9:27) = RTH; 8:30 end (starts 8:27) = overnight; 15:00 end (starts 14:57) = RTH; 15:03 end = not
    auto L = [](int h, int m) { return 3878000000LL - (3878000000LL % 86400) + h * 3600 + m * 60; };
    CHECK(rthOf(L(9, 30), 180) && !rthOf(L(8, 30), 180) && rthOf(L(8, 33), 180) && rthOf(L(15, 0), 180) && !rthOf(L(15, 3), 180) && !rthOf(L(20, 0), 180));
    std::printf("t_bigtier: %d checks, %d fails\n", c, f); return f ? 1 : 0; }
