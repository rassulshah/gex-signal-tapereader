// test_liqtouch_warm.cpp -- LiqTouch 1.6.0 (2026-11-03 night, Rassul "fix all" after the Liquidity Profile audit): THE WARM-UP and THE DEPTH
// SEED. After an IRT restart the order-by-order book only knows orders placed since; a 300-lot bid resting from before the restart is only
// in the depth book (MBP). Replay: restart -> warming for 10 min; the new 30-lot orders become nodes at 10 min; the old 300 bid is seeded
// from depth once DISPLAYED 10 min (rank 1 of the whole book and the king), never double-counted once its order shows up in the feed;
// a stale depth book or a remainder that vanishes restarts / drops the seed; coverage = known / displayed lots.
// From plugin\:  g++ -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_liqtouch/test_liqtouch_warm.cpp -o tltw && ./tltw
#include "LiqTouch.h"
#include <cstdio>
static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace ltc;
static const Zone* find(const std::vector<Zone>& v, int sd, long long px) { for (const Zone& z : v) if (z.side == sd && z.px == px) return &z; return nullptr; }
int main() {
    Params P = defaults("ES"); P.zoneGate = true;
    Engine e(P); e.hostAtr = 4; const long long T = 1000000000LL;   // the restart: the engine's first event at T + 10
    CHECK(e.warming(T) && e.trackedMs(T) == 0 && e.firstMs < 0, "no event yet: warming, tracked 0");
    long long id = 1;
    // after the restart: new orders the feed sees (30-lot bids near price, a few offers); the 300-lot bid at 940 is OLD (never in the feed)
    e.onEvent(NEW, ASK, 1001, 20, id++, T + 10);
    for (int k = 0; k < 4; ++k) e.onEvent(NEW, BID, 990 - 3 * k, 30, id++, T + 10);
    e.onEvent(FILL, ASK, 1001, 1, 1, T + 20);                                      // last trade = 1001
    CHECK(e.firstMs == T + 10, "the warm-up clock starts at the first event");
    // the depth book: what the feed knows at 990 / 987 / 984 / 981 (30 each), the old 300 at 940, the 20 offer at 1001
    std::map<long long, double> db, da; for (int k = 0; k < 4; ++k) db[990 - 3 * k] = 30; db[940] = 300; da[1001] = 19;   // 20 offered, 1 filled
    long long t = T + 1000;
    for (; t <= T + 300000; t += 1000) { e.setDepth(db, da, t); e.advance(t); }
    CHECK(e.warming(t) && e.trackedMs(t) > 290000, "5 min after the restart: still warming");
    CHECK(e.bookNodes().empty(), "5 min: no whole-book node yet (nothing rested 10 min, the old bid not displayed 10 min yet)");
    CHECK(e.coverage() > 0.31 && e.coverage() < 0.33, "coverage = known 139 (120 bids + 19 offer) of displayed 439 lots");
    CHECK(e.seedNow == 0 && e.seedLots(BID, 940, t) == 0, "the old bid is not seeded before 10 min of display");
    for (; t <= T + 601000; t += 1000) { e.setDepth(db, da, t); e.advance(t); }
    CHECK(!e.warming(t), "10 min after the first event: ready");
    CHECK(e.seedLots(BID, 940, t) == 300 && e.seedNow == 300 && e.seedLevels == 1, "the old 300 bid is seeded once displayed 10 min");
    const Zone* old = find(e.bookNodes(), BID, 940);
    CHECK(old && old->lots == 300, "whole-book node: the old 300 bid at 940 (from depth)");
    CHECK(!e.bookNodes().empty() && e.bookNodes()[0].px == 940, "it ranks first (biggest), not an ordinary 30-lot order");
    CHECK(find(e.bookNodes(), BID, 990) != nullptr, "the 30-lot orders placed after the restart are nodes too (rested 10 min)");
    // the king: beyond 3 ATR (R = 13 ticks) inside today's range, >= 3x the bid median (30)
    e.kLo = 900; e.kHi = 1100; e.advance(t + 1000); t += 1000;
    for (int k = 0; k < 70; ++k) { t += 1000; e.setDepth(db, da, t); e.advance(t); }
    CHECK(e.king[BID].lots == 300 && e.king[BID].px == 940, "the king = the seeded 300 bid at 940");
    // its order shows up in the feed (a size change): known now, the seed shrinks to 0 - never counted twice
    e.onEvent(MOD, BID, 940, 300, 777, t + 10);
    t += 1000; e.setDepth(db, da, t); e.advance(t);
    CHECK(e.seedLots(BID, 940, t) == 0, "known order: no remainder (not double-counted)");
    const Zone* old2 = find(e.bookNodes(), BID, 940);
    CHECK(!old2 || old2->lots <= 300, "the node never exceeds what is displayed (300)");
    // depth stale (no setDepth for 6 s): no seed at all
    db[930] = 200; e.setDepth(db, da, t + 1000);
    long long t2 = t + 1000; for (int k = 0; k < 700; ++k) { t2 += 1000; e.setDepth(db, da, t2); }
    CHECK(e.seedLots(BID, 930, t2) == 200, "a second old bid seeded after 10 min of display");
    CHECK(e.seedLots(BID, 930, t2 + 6000) == 0, "depth older than 5 s: no seed");
    // a remainder that vanishes restarts its clock
    db.erase(930); e.setDepth(db, da, t2 + 1000); db[930] = 200; e.setDepth(db, da, t2 + 2000);
    CHECK(e.seedLots(BID, 930, t2 + 2000) == 0, "gone and back: its 10-min clock restarts");
    // garbage depth values are ignored
    std::map<long long, double> bad; bad[950] = -5; bad[951] = std::nan(""); bad[-3] = 50; e.setDepth(bad, da, t2 + 3000);
    CHECK(e.seedLots(BID, 950, t2 + 3000) == 0 && e.seedLots(BID, 951, t2 + 3000) == 0, "negative / NaN / bad price depth rows ignored");
    // reset clears the warm-up
    e.reset(); CHECK(e.firstMs < 0 && e.warming(T) && e.coverage() < 0, "reset: warming again, no coverage");
    std::printf("LiqTouch %s warm-up / depth seed: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
