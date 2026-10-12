// test_audit_257.cpp -- lsDeltaProfile 2.5.7: the 2026-10-12 audit's F1 (node edges in whole ticks) on the five markets it reproduced.
// g++ -std=c++17 -I. test_audit_257.cpp -o t && ./t   (from tests_delta)
#include "../DeltaProfileCore.h"
namespace dp = delta_profile;
#include <cstdio>
static int fails = 0, passes = 0;
static void check(bool c, const char* m) { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } }
// a selling node at one price (the native float price), then a GREEN bar closing exactly ON that price: no close beyond the node -> A?
static void onEdge(const char* name, float nodePrice, float tickF) {
    const double tick = static_cast<double>(tickF);
    const dp::Tick k = dp::priceKey(static_cast<double>(nodePrice), tick);
    std::vector<dp::Bar> bars(3);
    for (int i = 0; i < 3; ++i) { bars[i].index = i; bars[i].time = 1000 + 180 * i; bars[i].closed = true; }
    dp::Row r; r.key = k; r.delta = -500; r.volume = 900;                      // heavy selling at the node
    bars[0].rows.push_back(r);
    bars[0].open = bars[0].close = bars[0].high = bars[0].low = nodePrice;
    const float below = nodePrice - 2 * tickF;
    bars[1].open = below; bars[1].low = below; bars[1].close = nodePrice; bars[1].high = nodePrice;   // green, closes ON the node's edge
    bars[2].open = bars[2].close = bars[2].high = bars[2].low = nodePrice;
    const dp::State st = dp::classify(bars, k, k, -500, tick);
    char m[160]; std::snprintf(m, sizeof(m), "%s: a green close ON the node (%.10g, key %lld) is not beyond it -> A? (got %s)", name, (double)nodePrice, (long long)k, st.code.c_str());
    check(st.code == "A?", m);
    // one tick above IS beyond: A
    bars[1].close = nodePrice + tickF; bars[1].high = bars[1].close;
    const dp::State st2 = dp::classify(bars, k, k, -500, tick);
    std::snprintf(m, sizeof(m), "%s: a green close one tick above the selling node -> A (got %s)", name, st2.code.c_str());
    check(st2.code == "A", m);
}
int main() {
    onEdge("GC", 4214.5f, 0.1f); onEdge("HG", 5.0f, 0.0005f); onEdge("NG", 3.0f, 0.001f); onEdge("CL", 75.0f, 0.01f); onEdge("EU", 1.1f, 0.00005f);
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
