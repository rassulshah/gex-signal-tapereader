// test_liqtouch_book.cpp -- LiqTouch 1.3.0 THE WHOLE-BOOK NODES (Rassul 2026-11-01 10:12-10:21 CT "it should be the whole book, no more 3 ATR"):
// real orders (rested >= 10 min, re-priced < 2x in 5 min) at ANY distance, node = price +-1 tick, one node per distribution, biggest first.
// From plugin\:  g++ -std=c++17 -O1 -Wall -Wextra -fsanitize=address,undefined -I. tests_liqtouch/test_liqtouch_book.cpp -o tltb && ./tltb
#include "LiqTouch.h"
#include <cstdio>
static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace ltc;
static const Zone* find(const std::vector<Zone>& v, int sd, long long px) { for (const Zone& z : v) if (z.side == sd && z.px == px) return &z; return nullptr; }
int main() {
    {   // sparse-map node rule == the dense pickZones rule (no gate) on a small book
        std::map<long long, long long> r; r[990] = 5; r[989] = 30; r[988] = 4; r[970] = 50; r[960] = 20; r[959] = 20; r[958] = 1;
        const std::vector<Zone> z = bookZones(r, BID, 1000, 3, 0.7, 20);
        CHECK(z.size() == 3, "three distributions");
        CHECK(!z.empty() && z[0].px == 970 && z[0].lots == 50, "biggest first: 50 at 970");
        CHECK(find(z, BID, 989) && find(z, BID, 989)->lots == 39, "node = price +-1 tick at the biggest price (5 + 30 + 4 at 989)");
        CHECK(find(z, BID, 959) && find(z, BID, 959)->lots == 41, "959: 20 + 20 + 1 (ties keep the window centre)");
        const std::vector<Zone> c1 = bookZones(r, BID, 1000, 3, 0.7, 1);
        CHECK(c1.size() == 1 && c1[0].px == 970, "cap per side");
        const std::vector<Zone> a = bookZones(r, ASK, 1000, 3, 0.7, 20);
        CHECK(a.empty(), "bids are never offer nodes (offers only above the last trade)");
    }
    Params P = defaults("ES"); P.zoneGate = true;
    Engine e(P); e.hostAtr = 4; const long long T = 1000000000LL; e.begin(T, 1000);
    long long id = 1;
    e.onEvent(NEW, BID, 400, 300, id++, T + 10);   // 600 ticks away: far beyond any ATR reach
    e.onEvent(NEW, ASK, 1500, 200, id++, T + 10);
    e.onEvent(NEW, ASK, 1004, 50, id++, T + 10);
    e.advance(T + 300000);
    CHECK(e.bookNodes().empty(), "nothing before the orders rested 10 min");
    e.onEvent(NEW, ASK, 1010, 80, 77, T + 300500);   // placed later: rests 10 min later
    e.advance(T + 601000);
    CHECK(find(e.bookNodes(), BID, 400) && find(e.bookNodes(), BID, 400)->lots == 300, "a bid 600 ticks away is a node (whole book)");
    CHECK(find(e.bookNodes(), ASK, 1500) != nullptr && find(e.bookNodes(), ASK, 1004) != nullptr, "offers anywhere above");
    CHECK(find(e.bookNodes(), ASK, 1010) == nullptr, "rested < 10 min: not real yet");
    e.advance(T + 901000);
    CHECK(find(e.bookNodes(), ASK, 1010) != nullptr, "after 10 min it counts");
    // re-priced twice in 5 min = quoting, out
    e.onEvent(MOD, ASK, 1011, 80, 77, T + 902000); e.onEvent(MOD, ASK, 1012, 80, 77, T + 903000); e.advance(T + 905000);
    CHECK(find(e.bookNodes(), ASK, 1012) == nullptr && find(e.bookNodes(), ASK, 1011) == nullptr, "re-priced 2x in 5 min: not real");
    e.onEvent(DEL, BID, 400, 0, 1, T + 906000); e.advance(T + 908000);
    CHECK(find(e.bookNodes(), BID, 400) == nullptr, "pulled: gone");
    std::printf("LiqTouch %s book nodes: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
