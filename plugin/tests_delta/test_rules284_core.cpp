// test_rules284_core.cpp -- lsDeltaProfile 2.8.4: the SDK-free half of test_rules284.cpp (pivotOk / halfwayOk / classify), so
// run-logic-tests.bat can run it with MSVC (cl) as well as g++. The host half (mock SDK, record files) is test_rules284.cpp.
// Build: g++ -std=c++17 -I. test_rules284_core.cpp -o t && ./t      or      cl /nologo /EHsc /W3 tests_delta\test_rules284_core.cpp
#include "../DeltaProfileCore.h"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
namespace dp = delta_profile;
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) { ++passed; std::cout << "PASS " << why << '\n'; } else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
int main()
{
    // ---- 2(d) pivotOk unit tests (pure function, plain arrays) ----
    {
        //            0    1    2    3    4    5    6
        std::vector<double> H = { 110, 109, 108, 107, 109, 111, 113 };
        std::vector<double> L = { 108, 107, 106, 104, 106, 108, 110 };
        int piv = -2;
        check(dp::pivotOk(H, L, 3, true, &piv) && piv == 3, "bullish: the bar that makes the down leg's low creates the down pivot");
        check(dp::pivotOk(H, L, 4, true, &piv) && piv == 3, "bullish: bar 4 low 106 < down-pivot bar 3 high 107 -> pass");
        check(!dp::pivotOk(H, L, 5, true, &piv) && piv == 3, "bullish: bar 5 low 108 >= down-pivot bar 3 high 107 -> fail");
        check(!dp::pivotOk(H, L, 6, true), "bullish: bar 6 (low 110) mid up-leg -> fail");
        // later bars never change the answer at bar 5 (only bars 0..pk are read)
        std::vector<double> H2 = H, L2 = L; H2.push_back(105); L2.push_back(100); H2.push_back(104); L2.push_back(99);
        check(!dp::pivotOk(H2, L2, 5, true) && dp::pivotOk(H2, L2, 4, true), "bars after the peak never change the pivot answer");
        // bearish mirror: a rally to bar 3, then down
        std::vector<double> Hb = { 100, 101, 102, 105, 103, 101, 99 };
        std::vector<double> Lb = { 98,  99,  100, 103, 101, 99,  97 };
        check(dp::pivotOk(Hb, Lb, 3, false, &piv) && piv == 3, "bearish: the bar that makes the up leg's high creates the up pivot");
        check(!dp::pivotOk(Hb, Lb, 4, false, &piv) && piv == 3, "bearish: bar 4 high 103 is NOT above up-pivot bar 3 low 103 (equal) -> fail");
        check(!dp::pivotOk(Hb, Lb, 5, false, &piv) && piv == 3, "bearish: bar 5 high 101 <= up-pivot bar 3 low 103 -> fail");
        std::vector<double> Hn = { 1, 2 }, Ln = { 0, std::nan("") };
        check(!dp::pivotOk(Hn, Ln, 1, true), "NaN low -> fail (never A on bad data)");
        check(!dp::pivotOk(H, L, 0, true), "bar 0 has no ZigZag history -> fail");
        // parity with CobImpulseLogic::legPivot's walk: an up leg's start is the down pivot
        check(dp::pivotOk(H, L, 6, false, &piv) && piv == 6, "bearish at bar 6: it makes the up leg's high = creates the up pivot");
    }
    // ---- 2(c) halfwayOk unit tests ----
    {
        // a bar 100.00 - 102.00 (tick 0.25): mid 101.00 = key 404
        check(dp::halfwayOk(400, 401, 102, 100, 0.25, true), "bullish: node 100.00-100.25 in the lower half -> pass");
        check(dp::halfwayOk(403, 405, 102, 100, 0.25, true), "bullish: node middle exactly at the halfway mark -> pass (at or below)");
        check(!dp::halfwayOk(405, 406, 102, 100, 0.25, true), "bullish: node above the halfway mark -> fail");
        check(dp::halfwayOk(406, 407, 102, 100, 0.25, false), "bearish: node in the upper half -> pass");
        check(dp::halfwayOk(404, 404, 102, 100, 0.25, false), "bearish: node exactly at the halfway mark -> pass (at or above)");
        check(!dp::halfwayOk(400, 401, 102, 100, 0.25, false), "bearish: node in the lower half -> fail");
        check(!dp::halfwayOk(400, 401, std::nan(""), 100, 0.25, true), "unknown high -> fail");
        // float ticks that are not exact in binary (IRT hands float prices / tick): a node exactly AT the halfway mark passes
        { const float t = 0.1f, lo = 4214.3f, hi = 4214.7f; const dp::Tick k = dp::priceKey(4214.5f, t);
          check(dp::halfwayOk(k, k, hi, lo, t, true) && dp::halfwayOk(k, k, hi, lo, t, false), "GC float tick 0.1: node AT the halfway mark passes both ways (2.8.3 refused it)"); }
        { const float t = 0.0005f, lo = 4.999f, hi = 5.001f; const dp::Tick k = dp::priceKey(5.0f, t);
          check(dp::halfwayOk(k, k, hi, lo, t, true) && dp::halfwayOk(k, k, hi, lo, t, false), "HG float tick 0.0005: node AT the halfway mark passes both ways"); }
    }
    // ---- 2(c) on the core: classify() with the halfway rule ----
    {
        std::vector<dp::Bar> b(2);
        for (int i = 0; i < 2; ++i) { b[i].index = i; b[i].time = 60 * i; b[i].closed = true; }
        b[0].low = 100; b[0].high = 104; b[0].open = 100.5; b[0].close = 103.5; b[0].rows.push_back(dp::Row(404, -500, 500));   // node 101.00 (lower half)
        b[1].low = b[1].high = b[1].open = b[1].close = 103;
        check(dp::classify(b, 404, 404, -500, 0.25).code == "A", "core: green peak bar, close above, node in the lower half -> A");
        b[0].rows[0].key = 412;                                   // node 103.00: upper half of a 100-104 bar
        check(dp::classify(b, 412, 412, -500, 0.25).code == "A", "(2.15.0, Rassul 2026-10-17 no halfway rule) core: node in the upper half of the green peak bar -> still A");
    }

    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
