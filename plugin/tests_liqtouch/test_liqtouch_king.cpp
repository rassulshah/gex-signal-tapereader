// test_liqtouch_king.cpp -- LiqTouch 1.2.0 THE KING NODES (Rassul approved 2026-10-29 11:18 CT): per side the biggest zone of REAL orders
// (rested >= 10 min, re-priced < 2x in 5 min) BEYOND the 3-ATR candidate reach, inside today's reachable range [kLo, kHi], >= 3.0x the
// side's median real level; it changes only to a challenger >= 1.25x.
// From plugin\:  g++ -std=c++17 -O1 -Wall -Wextra -fsanitize=address,undefined -I. tests_liqtouch/test_liqtouch_king.cpp -o tltk && ./tltk
#include "LiqTouch.h"
#include <cstdio>
static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace ltc;
int main() {
    Params P = defaults("ES"); P.zoneGate = true;
    Engine e(P); e.hostAtr = 20; const long long T = 1000000000LL; e.begin(T, 1000);
    long long id = 1;
    for (int i = 0; i < 12; ++i) e.onEvent(NEW, BID, 995 - 4 * i, 10, id++, T + 10);   // the median real bid level: 10 lots
    e.onEvent(NEW, BID, 900, 100, 900, T + 10);                                          // far (beyond 3 ATR), 10x the median
    e.onEvent(NEW, BID, 880, 40, 901, T + 10);                                           // far, 4x
    e.kLo = 850; e.kHi = 1100;
    e.advance(T + 300000);
    CHECK(e.king[BID].lots == 0, "no king before the orders rested 10 min");
    e.advance(T + 601000);
    std::printf("  king bid %lld at %lld (%.1fx), median %.1f\n", e.king[BID].lots, e.king[BID].px, e.kingX[BID], e.median(BID));
    CHECK(e.king[BID].lots == 100 && e.king[BID].px == 900 && e.kingX[BID] >= 3.0, "king = the biggest far zone (100 lots at 900), >= 3x the median");
    CHECK(e.king[ASK].lots == 0, "no offer king (no far offers)");
    e.onEvent(NEW, BID, 870, 120, 902, T + 602000); e.advance(T + 1203000);
    CHECK(e.king[BID].px == 900, "a challenger at 1.2x does not take over (swap needs 1.25x)");
    e.onEvent(MOD, BID, 870, 130, 902, T + 1204000); e.advance(T + 1206000);
    CHECK(e.king[BID].px == 870 && e.king[BID].lots == 130, "a challenger at 1.3x takes over");
    e.onEvent(DEL, BID, 870, 0, 902, T + 1207000); e.advance(T + 1209000);
    CHECK(e.king[BID].px == 900, "the king pulled -> the next qualifying zone");
    e.kLo = 950; e.advance(T + 1212000);
    CHECK(e.king[BID].lots == 0, "outside today's reachable range: no king");
    std::printf("LiqTouch %s king nodes: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
