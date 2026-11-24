// t_abs118.cpp -- LevelMarks 1.18.0 absMarks: the Abs badge rule (effort without result at the first RTH test of a Bk level) + the next-candle Ex
#include "../LevelMarks.h"
#include "../WyMarks.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long buy, long long sell) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = buy; b.sell = sell; return b; }
int main() {
    // GC: RTH window 7:23-12:30 (candle END). Day 0 = 1904-01-01 style seconds: t = day*86400 + minutes*60
    const long long D = 86400LL * 45000;
    auto T = [&](int h, int m) { return D + h * 3600 + m * 60; };
    std::vector<wyr::Bar> B;
    // 6:00 .. 8:00: 41 quiet candles, range 10 ticks (1000..1010), volume 100
    for (int k = 0; k < 41; ++k) B.push_back(bar(T(6, 0) + k * 180, 1005, 1010, 1000, 1005, 50, 50));
    // 8:03: the test candle - low 990 (the AsiaLO at 992 inside the bottom quarter), range 12 (<= 1.3x), volume 200 (2x), closed 999 (upper half)
    B.push_back(bar(T(8, 3), 1000, 1002, 990, 999, 80, 120)); B.back().qBot = 992;
    // 8:06: the candle after - sellers 40 (0.33x of 120), closed 1001 (further up), low 991 (not past 990 - 1)
    B.push_back(bar(T(8, 6), 999, 1003, 991, 1001, 60, 40));
    // 8:09 .. : more quiet candles; 8:30 a second test of 992 (not the first: no Abs)
    for (int k = 0; k < 6; ++k) B.push_back(bar(T(8, 9) + k * 180, 1005, 1010, 1000, 1005, 50, 50));
    B.push_back(bar(T(8, 27), 1000, 1002, 990, 999, 80, 120));
    std::vector<wyr::Lvl> key; { wyr::Lvl z; z.px = 992; z.name = "AsiaLO"; key.push_back(z); }
    const std::map<long long, lvm::AbsMk> M = lvm::absMarks(B, key, "GC");
    CHECK(M.size() == 1);
    const auto it = M.find(T(8, 3)); CHECK(it != M.end());
    if (it != M.end()) { const lvm::AbsMk& a = it->second; CHECK(a.dir == 1 && a.lv == "ALO" && a.lvPx == 992); CHECK(a.pos == 17 && lvm::absPos(a) == "Low 17%"); CHECK(a.ex && a.exT == T(8, 6)); }
    // effort WITH result (range 1.5x) = no Abs; volume 1.4x = no Abs; closed in the lower half = no Abs
    { std::vector<wyr::Bar> C = B; C[41] = bar(T(8, 3), 1000, 1005, 990, 1001, 80, 120); CHECK(lvm::absMarks(C, key, "GC").empty()); }
    { std::vector<wyr::Bar> C = B; C[41] = bar(T(8, 3), 1000, 1002, 990, 999, 60, 80); const auto m = lvm::absMarks(C, key, "GC"); for (const auto& kv : m) std::printf("fired %lld (8:03 = %lld)\n", (kv.first - D) / 60, (T(8,3) - D) / 60); CHECK(m.empty()); }
    { std::vector<wyr::Bar> C = B; C[41] = bar(T(8, 3), 1000, 1002, 990, 995, 80, 120); const auto m = lvm::absMarks(C, key, "GC");   // closed in its lower half: no bullish Abs at ALO - but a BEARISH one at the hour low 1000 under its 2-hours-back name (the 1-back name was touched by the 7 AM candles, as the research keyed tests by name) (1000, inside its top quarter, close below it), as the research rule
      for (const auto& kv : m) std::printf("  case 3: %lld dir %d lv %s px %lld\n", (kv.first - D) / 60, kv.second.dir, kv.second.lv.c_str(), kv.second.lvPx);
      CHECK(m.size() == 1 && m.begin()->second.dir == -1 && m.begin()->second.lv == "2hL" && m.begin()->second.lvPx == 1000); }
    // the level above the close for a bullish test = no Abs; a level outside the end quarter + tol = no Abs
    { std::vector<wyr::Lvl> k2; wyr::Lvl z; z.px = 1000; z.name = "AsiaLO"; k2.push_back(z); CHECK(lvm::absMarks(B, k2, "GC").empty()); }
    // the Ex fails when the candle after trades past the low by 2 ticks, closes lower, or the sellers trade more than half
    { std::vector<wyr::Bar> C = B; C[42].l = 988; const auto m = lvm::absMarks(C, key, "GC"); CHECK(m.size() == 1 && !m.begin()->second.ex); }
    { std::vector<wyr::Bar> C = B; C[42].c = 998; const auto m = lvm::absMarks(C, key, "GC"); CHECK(m.size() == 1 && !m.begin()->second.ex); }
    { std::vector<wyr::Bar> C = B; C[42].sell = 61; const auto m = lvm::absMarks(C, key, "GC"); CHECK(m.size() == 1 && !m.begin()->second.ex); }
    // outside the research RTH window (GC 7:23-12:30) = no Abs; an unknown market = none
    { std::vector<wyr::Bar> C; for (const wyr::Bar& b : B) { wyr::Bar x = b; x.t -= 3 * 3600; C.push_back(x); } CHECK(lvm::absMarks(C, key, "GC").empty()); }
    CHECK(lvm::absMarks(B, key, "ZZ").empty());
    // a touch earlier in the day uses the level up (the first test only)
    { std::vector<wyr::Bar> C = B; C[30] = bar(T(7, 30), 1000, 1005, 992, 1003, 50, 50); const auto m = lvm::absMarks(C, key, "GC");   // ALO is used up; the same price is now the 7 AM hour low - a new level by name (the research keyed tests by name)
      CHECK(m.size() == 1 && m.begin()->second.lv == "1hL"); }
    // the overlay: the Abs / Ex lines parse, the Turn file adds Ex? and P (WyMarks 2.8.0: always shown once known; -1 = not known = blank)
    { std::map<long long, std::vector<wyi::M> > m; const int n = wyi::parseAll("WY|1|GC|180|0.27.0\n100|Abs|1|Low 17%|992|ALO\n280|Ex|1\n", m);
      CHECK(n == 2 && m[100].size() == 1 && m[100][0].code == 'Q' && m[100][0].lv == "Low 17%" && m[100][0].st == "ALO" && m[280][0].code == 'E');
      CHECK(wyi::sideOf(m[100][0]) == -1 && wyi::sideOf(m[280][0]) == -1);
      std::map<long long, wyi::TurnAbs> ta; CHECK(wyi::parseTurnAbs("TAB|1|GC|180|1.5.0\n100|1|29|0|0\n200|-1|3|1|1\n300|-1|12|1|1\n", "GC", ta) && ta.size() == 3);
      CHECK(wyi::absLine1(&ta[100]) == "Abs" && wyi::absLine3(&ta[100]) == "P29"); CHECK(wyi::absLine1(&ta[200]) == "Abs/Ex?" && wyi::absLine3(&ta[200]) == "P3+");
      { wyi::TurnAbs z0; z0.p = 0; wyi::TurnAbs zu; zu.p = -1; CHECK(wyi::absLine3(&z0) == "P0" && wyi::absLine3(&zu).empty()); }
      CHECK(wyi::absLine3(&ta[300]) == "P12+" && wyi::absLine1(nullptr) == "Abs" && wyi::absLine3(nullptr).empty());
      CHECK(!wyi::parseTurnAbs("TAB|1|ES|180|x\n", "GC", ta)); }
    std::printf("%d checks, %d failed\n", checks, fails); return fails ? 1 : 0;
}
