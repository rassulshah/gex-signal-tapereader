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
    // (2.8.4 night verification) rewritten to the 2.6.2 rule: the node's OWN PEAK BAR (bar 0) decides at its close. It is GREEN, its
    // low two ticks below the node, high two ticks above (the node sits exactly at its halfway mark -> the 2.8.2 rule passes), and it
    // closes exactly ON the node: not beyond it -> not A.
    const float below = nodePrice - 2 * tickF, above = nodePrice + 2 * tickF;
    bars[0].open = below; bars[0].low = below; bars[0].high = above; bars[0].close = nodePrice;
    for (int i = 1; i < 3; ++i) bars[i].open = bars[i].close = bars[i].high = bars[i].low = nodePrice;
    const dp::State st = dp::classify(bars, k, k, -500, tick);
    char m[200]; std::snprintf(m, sizeof(m), "%s: a green peak-bar close ON the node (%.10g, key %lld) is not beyond it -> not A (got %s)", name, (double)nodePrice, (long long)k, st.code.c_str());
    check(st.code != "A", m);
    // one tick above IS beyond: A
    bars[0].close = nodePrice + tickF;
    const dp::State st2 = dp::classify(bars, k, k, -500, tick);
    std::snprintf(m, sizeof(m), "%s: a green peak-bar close one tick above the selling node -> A (got %s)", name, st2.code.c_str());
    check(st2.code == "A", m);
    // the float price one ULP below the next tick still keys to the node + 1 (whole ticks, llround)
    std::snprintf(m, sizeof(m), "%s: priceKey(node + 1 tick) = key + 1", name);
    check(dp::priceKey(static_cast<double>(static_cast<float>(nodePrice + tickF)), tick) == k + 1, m);
}
int main() {
    onEdge("GC", 4214.5f, 0.1f); onEdge("HG", 5.0f, 0.0005f); onEdge("NG", 3.0f, 0.001f); onEdge("CL", 75.0f, 0.01f); onEdge("EU", 1.1f, 0.00005f); onEdge("ES", 6712.25f, 0.25f); onEdge("NQ", 24931.75f, 0.25f);
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
