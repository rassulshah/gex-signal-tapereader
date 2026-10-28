// test_lft.cpp -- LmtFTiers.h 1.0.0 (SDK-free): session parts, the tiers file (PART / TIER / FLOOR), the print = lots filled at a price in a
// candle summed over orders, the adaptive live floor.  From plugin\:  g++ -std=c++17 -O1 -I. tests_lmtf/test_lft.cpp -o tlft && ./tlft
#include "LmtFTiers.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
int main() {
    const int o = 510, c = 900;
    const int mins[] = { 1020, 1139, 1140, 0, 119, 120, 509, 510, 539, 540, 869, 870, 899, 900, 1019 };
    const int want[] = { lft::EVE, lft::EVE, lft::ASIA, lft::ASIA, lft::ASIA, lft::LON, lft::LON, lft::OPEN, lft::OPEN, lft::MID, lft::MID, lft::CLOSE, lft::CLOSE, lft::POST, lft::POST };
    bool okp = true; for (int i = 0; i < 15; ++i) okp = okp && lft::partOf(mins[i], o, c) == want[i];
    CHECK(okp, "partOf = the nightly part_of (same boundaries)");
    lft::Tiers T;
    for (const char* l : { "VERSION,1", "MARKET,ES", "TIER,RTH,533,658,798", "TIER,ON,143,196,338", "FLOOR,RTH,179", "FLOOR,ON,33", "PART,ASIA,73,97,124", "PART,EVE,20,25,30", "garbage,1" }) T.line(l);
    long long x[3];
    CHECK(T.ok && T.cuts(lft::ASIA, x) && x[0] == 73 && x[1] == 97 && x[2] == 124, "ASIA cuts from its PART row");
    CHECK(T.cuts(lft::EVE, x) && x[0] == 33 && x[1] == 33 && x[2] == 33, "EVE row under the ON floor -> raised to the floor 33");
    CHECK(T.cuts(lft::LON, x) && x[0] == 143 && x[2] == 338, "no LON row -> the TIER,ON row");
    CHECK(T.cuts(lft::MID, x) && x[0] == 533, "no MID row -> the TIER,RTH row");
    lft::Tiers old; old.line("TIER,RTH,100,200,300"); old.line("TIER,ON,50,100,150");
    CHECK(old.cuts(lft::ASIA, x) && x[0] == 50 && old.cuts(lft::OPEN, x) && x[0] == 100, "an old 1.x file (TIER rows only) still works");
    lft::Prints P; P.spb = 180;
    const long long day = 20000LL * 86400, t = day + 20 * 3600;   // 20:00 Central (ASIA)
    P.on(t + 10, 0, 1000, 10, 1); P.on(t + 20, 0, 1000, 20, 2); P.on(t + 30, 0, 1000, 5, 1);   // one candle, one price, two orders
    P.on(t + 200, 0, 1000, 7, 3);                                                                // the next candle
    P.on(t + 10, 1, 1001, 4, 9);                                                                 // the offer side
    CHECK(P.cells.size() == 3, "three prints (candle x side x price)");
    const lft::Cell& a = P.cells[0];
    CHECK(a.lots == 35 && a.orders == 2 && a.big == 20, "the print sums the orders (35 lots, 2 orders, biggest single 20)");
    // adaptive: 12 closed quiet candles in ASIA, every print under the file's S20 73
    lft::Prints Q; Q.spb = 180; const long long ps = day + 19 * 3600;   // ASIA starts 19:00
    for (int b = 0; b < 12; ++b) for (int k = 0; k < 3; ++k) Q.on(ps + b * 180 + 5, k % 2, 2000 + b * 3 + k, 10 + b + k, 100 + b * 3 + k);
    const long long nowEnd = lft::Prints::barEnd(ps + 12 * 180 + 30, 180);
    bool ad = false; CHECK(lft::liveCuts(T, Q, lft::ASIA, nowEnd, ps, 420, x, ad) && ad, "quiet part, 12 candles, nothing >= S20 -> adaptive");
    CHECK(x[0] >= 33, "adaptive cut never under the floor 33");
    {   lft::Prints R; R.spb = 180; for (int b = 0; b < 12; ++b) for (int k = 0; k < 3; ++k) R.on(ps + b * 180 + 5, 0, 3000 + b * 3 + k, 30 + 3 * b + k, 500 + b * 3 + k);
        bool ad2 = false; lft::liveCuts(T, R, lft::ASIA, nowEnd, ps, 420, x, ad2);
        // prints 30..65 (all under S20 73); 12 candles of 420 min -> K20 = round(20 x 36/420) = 2 -> the 2nd biggest closed print (64)
        CHECK(ad2 && x[0] == 64 && x[2] >= x[1] && x[1] >= x[0], "adaptive S20 = the part's K-th biggest closed print (K = 20 x share elapsed)"); }
    {   lft::Prints R; R.spb = 180; R.on(ps + 5, 0, 1, 80, 1); for (int b = 1; b < 12; ++b) R.on(ps + b * 180 + 5, 0, 2 + b, 20, 2 + b);
        bool ad3 = true; lft::liveCuts(T, R, lft::ASIA, nowEnd, ps, 420, x, ad3);
        CHECK(!ad3 && x[0] == 73, "one print >= S20 in the part -> the file cuts stay"); }
    {   lft::Prints R; R.spb = 180; for (int b = 0; b < 5; ++b) R.on(ps + b * 180 + 5, 0, 1, 20, 1);
        const long long e5 = lft::Prints::barEnd(ps + 5 * 180 + 30, 180); bool ad4 = true; lft::liveCuts(T, R, lft::ASIA, e5, ps, 420, x, ad4);
        CHECK(!ad4, "under 10 closed candles -> no adaptive yet"); }
    {   lft::Prints R; R.spb = 180; R.on(ps + 5, 0, 1, 50, 1); R.on(ps + 4000, 0, 2, 50, 2); R.prune(ps + 3600);
        CHECK(R.cells.size() == 1 && R.perOrd.size() == 1, "prune drops older candles and their order tallies"); }
    std::printf("LmtFTiers %s: %d passed, %d failed\n", lft::VERSION, passes, fails);
    return fails ? 1 : 0;
}
