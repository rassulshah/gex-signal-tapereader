// t_bkside120.cpp -- LevelMarks 1.20.0 levelSide: a bullish Bk is of a high, a bearish Bk of a low (Rassul 2026-11-24 16:01 CT)
#include "../LevelMarks.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long v) { wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = v / 2; b.sell = v - v / 2; return b; }
int main() {
    for (const char* n : { "ONH", "PDH", "PFH", "PWH", "LonHI", "AsiaHI", "PHH", "PhrH", "VAH", "R1", "R2", "HpH", "4hH", "ONH/AsiaHI" }) CHECK(lvm::levelSide(n, false, 0) == 1);
    for (const char* n : { "ONL", "PDL", "PFL", "PWL", "LonLO", "AsiaLO", "PHL", "PhrL", "VAL", "S1", "S2", "HpL", "4hL", "ONL/LonLO" }) CHECK(lvm::levelSide(n, false, 0) == -1);
    for (const char* n : { "Open", "DPoc", "WPoc", "VWAP", "ONH/ONL" }) CHECK(lvm::levelSide(n, false, 0) == 0);
    CHECK(lvm::levelSide("2 PM hour low", false, 2) == -1 && lvm::levelSide("2 PM hour high", true, 2) == 1);
    // a candle closing UP through a LOW (ONL 1000) is not a Bk; closing DOWN through it is
    const long long T0 = 3870000000LL; std::vector<wyr::Bar> B; for (int k = 0; k < 12; ++k) B.push_back(bar(T0 + 180 * k, 995, 998, 992, 995, 100));
    std::vector<wyr::Lvl> key; { wyr::Lvl z; z.px = 1000; z.name = "ONL"; key.push_back(z); }
    std::vector<wyr::Bar> U = B; U.push_back(bar(T0 + 180 * 12, 995, 1008, 994, 1006, 300));
    for (const auto& kv : lvm::run(U, key)) CHECK(!(kv.second.code == 'B' && kv.second.lv == "ONL"));
    std::vector<wyr::Bar> D; for (int k = 0; k < 12; ++k) D.push_back(bar(T0 + 180 * k, 1005, 1008, 1002, 1005, 100));
    D.push_back(bar(T0 + 180 * 12, 1005, 1006, 992, 994, 300)); bool bk = false;
    for (const auto& kv : lvm::run(D, key)) if (kv.second.code == 'B' && kv.second.lv == "ONL" && kv.second.dir == -1) bk = true;
    CHECK(bk);
    CHECK(std::string(lvm::VERSION) == "1.22.0");
    std::printf("t_bkside120: %d checks, %d fails\n", checks, fails); return fails ? 1 : 0; }
