// test_liqtouch_zone.cpp -- LiqTouch 1.1.0 the zone candidate rule (Rassul approved 14:04 CT 2026-10-28)
// From plugin\:  g++ -std=c++17 -O1 -Wall -Wextra -I. tests_liqtouch/test_liqtouch_zone.cpp -o tltz && ./tltz
#include "LiqTouch.h"
#include <cstdio>
static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace ltc;
static Params zp() { Params p = defaults("ES"); p.zoneGate = true; return p; }
static bool has(const std::vector<Zone>& v, int sd, long long px, long long lots = -1) { for (const Zone& z : v) if (z.side == sd && z.px == px && (lots < 0 || z.lots == lots)) return true; return false; }
int main() {
    const Params P = zp();
    {   // pure rule: a ladder of 3 x 20 lots on adjacent ticks = one 60-lot zone; passes with median 22 (gate 44)
        std::map<long long, long long> r; r[990] = 20; r[989] = 20; r[988] = 20; r[980] = 22; r[975] = 22; r[970] = 25; r[965] = 21;
        const double med = sideMedian(r, BID, 1000, 40);
        (void)med;
        std::vector<Zone> z = pickZones(r, BID, 1000, 40, 22.0, P);
        CHECK(z.size() == 1 && z[0].px == 989 && z[0].lots == 60, "ladder 3 x 20 on adjacent ticks = ONE 60-lot zone at the middle price, passes (median 22, gate 44)");
        std::vector<Zone> z2 = pickZones(r, BID, 1000, 40, 31.0, P);
        CHECK(z2.empty(), "the same ladder fails when the median is 31 (gate 62)");
        CHECK(sideMedian(r, BID, 1000, 40) == 21.0, "median of 20 20 20 21 22 22 25 = 21");
    }
    {   // 2x gate with the computed median
        std::map<long long, long long> r; for (int i = 1; i <= 9; ++i) r[1000 - 4 * i] = 10; r[945] = 19; r[950] = 20;
        const double med = sideMedian(r, BID, 1000, 60);
        CHECK(med == 10.0, "median real level = 10");
        std::vector<Zone> z = pickZones(r, BID, 1000, 60, med, P);
        CHECK(has(z, BID, 950, 20) && !has(z, BID, 945), "2.0x gate: 20 passes (>= 20), 19 does not");
    }
    {   // distinct distributions: >= 3 ticks apart AND a valley <= 0.7x the smaller; else the bigger is kept
        std::map<long long, long long> r; r[990] = 100; r[989] = 60; r[988] = 70; r[960] = 90; r[959] = 10; r[958] = 10; r[957] = 10;
        for (int i = 0; i < 10; ++i) r[930 - 2 * i] = 10;
        std::vector<Zone> z = pickZones(r, BID, 1000, 80, 10.0, P);
        CHECK(has(z, BID, 990) && !has(z, BID, 989) && !has(z, BID, 988), "a node inside the bigger one's distribution (no valley) is dropped - the bigger kept");
        CHECK(has(z, BID, 960), "a separate node (valley 0 between) is its own distribution");
    }
    {   // top 3 per side is the caller's cap; fewer when fewer pass; offers above
        std::map<long long, long long> a; for (int i = 0; i < 8; ++i) a[1010 + 5 * i] = 100 + i; for (int i = 0; i < 20; ++i) a[1002 + 5 * i] += 0;
        for (int i = 0; i < 30; ++i) a[1003 + 5 * i] += 5;
        const double m = sideMedian(a, ASK, 1000, 80);
        std::vector<Zone> z = pickZones(a, ASK, 1000, 80, m, P);
        CHECK(z.size() == 8 && z[0].lots == 107 + 5 && z[0].px == 1045, "all 8 offers pass, biggest first (caller shows the top 3)");
        std::map<long long, long long> b; b[995] = 50; for (int i = 0; i < 10; ++i) b[980 - 3 * i] = 10;
        std::vector<Zone> zb = pickZones(b, BID, 1000, 80, sideMedian(b, BID, 1000, 80), P);
        CHECK(zb.size() == 1, "fewer than 3 when fewer pass (1 bid)");
        CHECK(pickZones(b, BID, 1000, 80, 0.0, P).empty(), "no median (no real levels) = nothing");
    }
    {   // the engine: real orders only (rested 30 s, re-priced < 2x in 5 min)
        Engine e(P); e.hostAtr = 20; const long long T = 1000000000LL;
        e.begin(T, 1000);
        long long id = 1;
        for (int i = 0; i < 9; ++i) e.onEvent(NEW, BID, 980 - 3 * i, 10, id++, T + 10);
        e.onEvent(NEW, BID, 990, 100, 500, T + 10);               // big real bid
        e.onEvent(NEW, ASK, 1010, 10, 600, T + 10); e.onEvent(NEW, ASK, 1014, 10, 601, T + 10); e.onEvent(NEW, ASK, 1018, 10, 602, T + 10);
        e.onEvent(NEW, ASK, 1005, 200, 700, T + 10);               // a quoting offer: re-prices twice
        e.advance(T + 20000);
        CHECK(e.candidates().empty(), "nothing before the orders rested 30 s");
        e.onEvent(MOD, ASK, 1006, 200, 700, T + 25000); e.onEvent(MOD, ASK, 1005, 200, 700, T + 26000);
        e.onEvent(NEW, BID, 970, 300, 800, T + 25000);             // a big bid that has rested only 6 s at T+31 s
        e.advance(T + 31000);
        CHECK(has(e.candidates(), BID, 990, 100), "the 100-lot real bid is a candidate after 30 s");
        CHECK(!has(e.candidates(), ASK, 1005) && !has(e.candidates(), ASK, 1006), "the re-quoting offer (2 re-prices in 5 min) is excluded (market-maker style)");
        CHECK(!has(e.candidates(), BID, 970), "a big bid rested < 30 s is not yet real");
        CHECK(e.followed() >= 1, "candidates are followed by the touch engine");
        e.advance(T + 57000);
        CHECK(has(e.candidates(), BID, 970), "the 300 bid becomes a candidate once rested 30 s");
        // touch inside the zone (+-1 tick): a trade at 991 touches the 990 zone
        e.onEvent(NEW, ASK, 1001, 50, 900, T + 57100);
        e.onEvent(FILL, BID, 991, 1, 999999, T + 57200);
        bool touched = false; for (const Touch& t : e.pending()) if (t.side == BID && t.px == 990) touched = true;
        CHECK(touched, "a trade 1 tick inside the zone touches it (zone touch)");
    }
    {   // zoneGate off = the 1.0.0 tier engine (parity)
        Params p = defaults("ES"); CHECK(!p.zoneGate, "defaults keep the tier gate (the study parity); lsLiquidityProfile turns the zone gate on");
    }
    std::printf("LiqTouch %s zone rule: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
