// lsDeltaProfile 2.19.0: the gray circle - dp::grayPick (DeltaProfileCore.h)
#include "DeltaProfileCore.h"
#include <cstdio>
#include <algorithm>
static int passes = 0, fails = 0;
static void check(bool ok, const char* m) { if (ok) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } }
using namespace delta_profile;
typedef delta_profile::GrayCand dp_gc;
static dp_gc G(long long id, long long candle, double mult, bool col, bool open) { dp_gc g; g.id = id; g.candle = candle; g.mult = mult; g.coloured = col; g.open = open; return g; }
int main()
{
    std::vector<dp_gc> c;
    c.push_back(G(1, 100, 2.5, false, true));      // forming node, candle 100
    c.push_back(G(2, 100, 3.1, false, true));      // a bigger one on the same candle -> the one gray circle
    c.push_back(G(3, 200, 2.2, true, false));      // candle 200: a coloured A
    c.push_back(G(4, 200, 4.0, false, true));      //   ... its gray node is not drawn (colour wins)
    c.push_back(G(5, 300, 2.0, false, true));      // candle 300: no winner -> gray
    c.push_back(G(6, 400, 2.0, false, false));     // an old decided I / Acc record: never gray
    c.push_back(G(7, 500, 2.4, false, true)); c.push_back(G(8, 500, 2.4, false, true));   // tie -> the earliest
    std::vector<long long> p = grayPick(c);
    auto has = [&](long long id) { return std::find(p.begin(), p.end(), id) != p.end(); };
    check(p.size() == 3, "one gray circle per candle that has no coloured A");
    check(has(2) && !has(1), "the biggest multiple on a candle");
    check(!has(4) && !has(3), "a coloured A owns its candle: no gray there");
    check(has(5), "no winner: stays gray");
    check(!has(6), "a decided old record (not A? / I?) is never gray");
    check(has(7) && !has(8), "a tie: the earliest node");
    check(grayPick(std::vector<dp_gc>()).empty(), "nothing: nothing");
    std::printf("dp 2.19.0 gray: %d checks, %d failures\n", passes + fails, fails);
    return fails ? 1 : 0;
}
