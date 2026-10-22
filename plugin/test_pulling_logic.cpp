// test_pulling_logic.cpp -- lsPulling engine tests (no Investor/RT). g++ -std=c++17 -Wall -Wextra -I. test_pulling_logic.cpp -o t && ./t [dbo.csv tick]
#include "PullingLogic.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
using namespace pl;
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static Ev ev(int a, int s, Tick px, double sz, long long id, Tick prev = 0) { Ev e; e.action = a; e.side = s; e.px = px; e.size = sz; e.id = id; e.prev = prev; return e; }
static Engine fresh() { Engine e; e.P.near = 5; e.setBest(100, 101); e.setBar(0); return e; }

static void test_fill_then_delete_is_not_a_pull() {
    Engine e = fresh();
    e.onEvent(ev(NEW, BID, 100, 10, 1));
    e.onEvent(ev(FILL, BID, 100, 10, 1)); e.onEvent(ev(DEL, BID, 100, 10, 1));
    CHECK(e.bars[0].pull[BID] == 0 && e.bars[0].filled == 10, "a bid filled in full then deleted: traded, not pulled");
    e.onEvent(ev(NEW, BID, 100, 10, 2)); e.onEvent(ev(FILL, BID, 100, 4, 2)); e.onEvent(ev(MOD, BID, 100, 6, 2));
    CHECK(e.bars[0].pull[BID] == 0, "a partial fill then MOD to the rest: not a pull");
    e.onEvent(ev(DEL, BID, 100, 6, 2));
    CHECK(e.bars[0].pull[BID] == 6, "the rest cancelled later: 6 pulled");
}
static void test_cancel_near_vs_far() {
    Engine e = fresh();
    e.onEvent(ev(NEW, OFFER, 103, 20, 1)); e.onEvent(ev(NEW, OFFER, 120, 50, 2));
    e.onEvent(ev(DEL, OFFER, 103, 20, 1)); e.onEvent(ev(DEL, OFFER, 120, 50, 2));
    CHECK(e.bars[0].pull[OFFER] == 20, "an offer 2 ticks over the best counts, 19 ticks away does not");
    CHECK(e.bars[0].stack[OFFER] == 20, "the near NEW is stacking");
}
static void test_moves() {
    Engine e = fresh();
    e.onEvent(ev(NEW, BID, 99, 8, 1)); e.onEvent(ev(MOD, BID, 96, 8, 1, 99));
    CHECK(e.bars[0].pull[BID] == 8, "a bid moved DOWN (away from price) = pulled");
    e.onEvent(ev(NEW, BID, 97, 5, 2)); e.onEvent(ev(MOD, BID, 99, 5, 2, 97));
    CHECK(e.bars[0].pull[BID] == 8 && e.bars[0].stack[BID] == 8 + 5 + 5, "a bid moved UP (toward price) = stacked, not pulled");
    e.onEvent(ev(MOD, BID, 99, 2, 2));
    CHECK(e.bars[0].pull[BID] == 11, "a smaller size at the same price = 3 pulled");
}
static void test_unknown_orders_and_x() {
    Engine e = fresh();
    e.onEvent(ev(DEL, OFFER, 101, 7, 99));
    CHECK(e.bars[0].pull[OFFER] == 7 && e.unknown == 1, "an order resting before load: its DEL size counts");
    Engine f = fresh();
    for (int b = 0; b < 20; ++b) { f.setBar(b); f.onEvent(ev(DEL, BID, 100, 10, 1000 + b)); f.onEvent(ev(DEL, OFFER, 101, 10, 2000 + b)); }
    f.setBar(20); f.onEvent(ev(DEL, OFFER, 101, 25, 3000)); f.onEvent(ev(DEL, BID, 100, 5, 3001));
    CHECK(std::fabs(f.x(20, OFFER) - 2.5) < 1e-9 && std::fabs(f.x(20, BID) - 0.5) < 1e-9, "sellers 2.5x, buyers 0.5x of the 10 lots a side normal");
    CHECK(f.x(5, OFFER) < 0, "under 10 bars before it: no number");
}
static void test_leader_cross() {
    Engine e = fresh();
    for (int b = 0; b < 12; ++b) { e.setBar(b); e.onEvent(ev(DEL, BID, 100, 20, 10 + b)); e.onEvent(ev(DEL, OFFER, 101, 5, 100 + b)); }
    CHECK(e.leader(11) == BID && !e.cross(11), "buyers pulled more over 10 bars: buyers lead, no cross yet");
    for (int b = 12; b < 30; ++b) { e.setBar(b); e.onEvent(ev(DEL, OFFER, 101, 30, 200 + b)); e.onEvent(ev(DEL, BID, 100, 2, 300 + b)); }
    int crosses = 0, at = -1; for (int b = 12; b < 30; ++b) if (e.cross(b)) { ++crosses; at = b; }
    CHECK(crosses == 1 && e.leader(at) == OFFER, "one cross where sellers took over the pulling");
}
static void test_no_best_no_count() {
    Engine e; e.setBar(0); e.onEvent(ev(NEW, BID, 100, 5, 1)); e.onEvent(ev(DEL, BID, 100, 5, 1));
    CHECK(e.bars[0].pull[BID] == 0, "no best bid / offer known yet: nothing counted");
}
// replay a recorded dbo_HH.csv (time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask), 3-minute bars
static void replay(const char* path, double tick) {
    std::ifstream in(path); std::string line; std::getline(in, line);
    Engine e; defaults("GC", e.P); int bar = -1; std::string lastBarKey;
    const char* A[4] = {"NEW", "MOD", "DEL", "FILL"};
    while (std::getline(in, line)) {
        std::vector<std::string> c; std::stringstream ss(line); std::string f; while (std::getline(ss, f, '|')) c.push_back(f);
        if (c.size() < 10) continue;
        const int mm = std::atoi(c[0].substr(14, 2).c_str()), hh = std::atoi(c[0].substr(11, 2).c_str());
        const int b = (hh * 60 + mm) / 3; if (b != bar) { bar = b; e.setBar(b); }
        Ev x; x.action = -1; for (int k = 0; k < 4; ++k) if (c[1] == A[k]) x.action = k; if (x.action < 0) continue;
        // (PullingLogic toTick(price, tick, out) -> bool: a malformed price is refused, never wrapped - the replay skips that event)
        auto tk = [&](const std::string& v, Tick& out) { return toTick(std::atof(v.c_str()), tick, out); };
        if (!tk(c[2], x.px) || !tk(c[8], x.bid) || !tk(c[9], x.ask)) continue;
        x.size = std::atof(c[4].c_str()); x.id = std::atoll(c[5].c_str());
        x.side = c[7] == "B" ? BID : OFFER; 
        e.onEvent(x);
    }
    std::printf("replay %s: %lld events, %lld unknown, %zu bars\n", path, e.events, e.unknown, e.bars.size());
    for (auto& kv : e.bars) std::printf("  bar %d  bids pulled %6.0f (%.1fx)  offers pulled %6.0f (%.1fx)  filled %5.0f  leader %d%s\n", kv.first,
        kv.second.pull[BID], e.x(kv.first, BID), kv.second.pull[OFFER], e.x(kv.first, OFFER), kv.second.filled, e.leader(kv.first), e.cross(kv.first) ? "  X" : "");
}
int main(int argc, char** argv) {
    test_fill_then_delete_is_not_a_pull(); test_cancel_near_vs_far(); test_moves(); test_unknown_orders_and_x(); test_leader_cross(); test_no_best_no_count();
    if (argc > 2) replay(argv[1], std::atof(argv[2]));
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
