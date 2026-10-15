// test_rules285_core.cpp -- lsDeltaProfile 2.8.5 (Rassul approved 2026-10-14 23:05 CT): the SDK-free half of the 2.8.5 rules, so
// run-logic-tests.bat can run it with MSVC (cl) as well as g++. The host half (mock SDK, record files, bubbles) is test_rules285.cpp.
//   rule 1  dp::oneBubblePerCandle - ONE bubble per candle per side (bullish lowest, bearish highest), first decision moment only
//   rule 2  dp::recheckA / dp::versionBefore - the one-time re-check of A records written before 2.8.4
// Build: g++ -std=c++17 -I. test_rules285_core.cpp -o t && ./t      or      cl /nologo /EHsc /W3 tests_delta\test_rules285_core.cpp
#include "../DeltaProfileCore.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
namespace dp = delta_profile;
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) { ++passed; std::cout << "PASS " << why << '\n'; } else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static dp::BubbleCand cand(long long id, long long candle, bool support, double price, long long tDecided) {
    dp::BubbleCand c; c.id = id; c.candle = candle; c.support = support; c.price = price; c.tDecided = tDecided; return c;
}
int main()
{
    // ---- versionBefore ----
    check(dp::versionBefore("2.8.0", 2, 8, 4), "2.8.0 is before 2.8.4");
    check(dp::versionBefore("2.6.1", 2, 8, 4) && dp::versionBefore("1.9.9", 2, 8, 4), "2.6.1 / 1.9.9 are before 2.8.4");
    check(!dp::versionBefore("2.8.4", 2, 8, 4) && !dp::versionBefore("2.8.5", 2, 8, 4), "2.8.4 and 2.8.5 are not before 2.8.4");
    check(!dp::versionBefore("2.8.10", 2, 8, 4) && !dp::versionBefore("3.0.0", 2, 8, 4) && !dp::versionBefore("2.9.0", 2, 8, 4), "2.8.10 / 2.9.0 / 3.0.0 compare as numbers, not text");
    check(dp::versionBefore("", 2, 8, 4) && dp::versionBefore("2.8", 2, 8, 4) && dp::versionBefore("2.8.4x", 2, 8, 4) && dp::versionBefore("v2.8.5", 2, 8, 4) &&
          dp::versionBefore("2..4", 2, 8, 4) && dp::versionBefore("9999999.0.0", 2, 8, 4), "unreadable versions count as OLD (re-checked, never trusted)");

    // ---- rule 1: oneBubblePerCandle ----
    const double T = 0.25;
    {   // tonight's ES pattern: the same peak bar recorded 3 times at slightly different prices (row size changed), all decided together
        std::vector<dp::BubbleCand> v = { cand(15, 1000, true, 7746.25, 50), cand(17, 1000, true, 7745.5, 50), cand(20, 1000, true, 7745.625, 50) };
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[15] == 17 && h[17] == 17 && h[20] == 17, "bullish: three A on one candle -> ONE bubble, at the LOWEST price (7745.50)");
    }
    {
        std::vector<dp::BubbleCand> v = { cand(1, 1000, false, 110.0, 50), cand(2, 1000, false, 111.0, 50), cand(3, 1000, false, 110.5, 50) };
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[1] == 2 && h[2] == 2 && h[3] == 2, "bearish: three A on one candle -> ONE bubble, at the HIGHEST price (111.00)");
    }
    {
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50), cand(2, 1180, true, 99, 60) };
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[1] == 1 && h[2] == 2, "two candles -> two bubbles");
    }
    {
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50), cand(2, 1000, false, 101, 50) };
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[1] == 1 && h[2] == 2, "one candle, both sides -> one bubble per side");
    }
    {   // a later duplicate (decided at a later moment) never moves a drawn bubble, even when it is lower
        std::vector<dp::BubbleCand> v = { cand(15, 1000, true, 7746.25, 50), cand(17, 1000, true, 7745.5, 53), cand(20, 1000, true, 7744.0, 80) };
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[15] == 15 && h[17] == 15 && h[20] == 15, "later duplicates (decided after the candle's first decision) join the drawn bubble, never move it");
        // ...but a lower one decided at that SAME first moment does take it
        v.push_back(cand(30, 1000, true, 7745.0, 50));
        h = dp::oneBubblePerCandle(v, T);
        check(h[15] == 30 && h[30] == 30 && h[17] == 30 && h[20] == 30, "a lower A decided at the same first moment takes the bubble (all candidates exist together)");
        // bearish mirror
        std::vector<dp::BubbleCand> b = { cand(1, 1000, false, 110, 50), cand(2, 1000, false, 112, 60) };
        h = dp::oneBubblePerCandle(b, T);
        check(h[2] == 1, "bearish: a later HIGHER duplicate never moves the drawn bubble");
        b.push_back(cand(3, 1000, false, 111, 50));
        h = dp::oneBubblePerCandle(b, T);
        check(h[1] == 3 && h[2] == 3 && h[3] == 3, "bearish: a higher A decided at the same first moment takes the bubble");
    }
    {   // ties: the same price -> the lowest id; prices that differ only by float noise (< 1/2 tick) are the same price
        std::vector<dp::BubbleCand> v = { cand(9, 1000, true, 100.0, 50), cand(4, 1000, true, 100.0 + 1e-9, 50), cand(7, 1000, true, 100.0, 50) };
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[9] == 4 && h[7] == 4 && h[4] == 4, "same price -> the lowest id holds the bubble (float noise is not a lower price)");
        std::vector<dp::BubbleCand> f = { cand(1, 1000, true, 4214.5f, 50), cand(2, 1000, true, 4214.4f, 50) };
        h = dp::oneBubblePerCandle(f, 0.1f);
        check(h[1] == 2, "GC float tick 0.1: 4214.4 is lower than 4214.5 (half-tick keys)");
    }
    {   // load-order independence: every permutation of 6 events gives the same answer
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50), cand(2, 1000, true, 99.5, 50), cand(3, 1000, true, 99.0, 70),
                                          cand(4, 1180, false, 105, 60), cand(5, 1180, false, 105.25, 60), cand(6, 1180, true, 101, 60) };
        const std::map<long long, long long> ref = dp::oneBubblePerCandle(v, T);
        std::vector<int> idx = { 0, 1, 2, 3, 4, 5 }; int perms = 0, same = 0;
        do {
            std::vector<dp::BubbleCand> w; for (int i : idx) w.push_back(v[static_cast<std::size_t>(i)]);
            ++perms; if (dp::oneBubblePerCandle(w, T) == ref) ++same;
        } while (std::next_permutation(idx.begin(), idx.end()));
        check(perms == 720 && same == 720, "all 720 load orders give the same bubbles (" + std::to_string(same) + "/" + std::to_string(perms) + ")");
        check(ref.at(1) == 2 && ref.at(3) == 2 && ref.at(4) == 5 && ref.at(6) == 6, "the reference answer: candle 1000 bullish -> 99.50, 1180 bearish -> 105.25, 1180 bullish -> its own");
    }
    {   // incremental growth = the live path: adding later-decided events one at a time never changes an existing holder
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50) };
        bool stable = true;
        for (int k = 0; k < 20; ++k) {
            v.push_back(cand(100 + k, 1000, true, 100 - 0.25 * (k + 1), 51 + k));      // each one lower, each decided later
            const std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
            for (std::map<long long, long long>::const_iterator i = h.begin(); i != h.end(); ++i) if (i->second != 1) stable = false;
        }
        check(stable, "20 later, ever-lower duplicates: the bubble never moves (no repaint)");
    }
    check(dp::oneBubblePerCandle(std::vector<dp::BubbleCand>(), T).empty(), "no events -> no bubbles");

    // ---- rule 2: recheckA ----
    {
        // bars 0..19 down (low 120-i, high low+2), bar 19 = the down pivot (low 101, high 103); 20..29 up (low 102 + (i-20)); bar 30 = the peak
        auto chart = [](double pl, double ph, double po, double pc) {
            std::vector<double> O, H, L, C;
            for (int i = 0; i < 40; ++i) {
                double lo = i < 20 ? 120 - i : i < 30 ? 102 + (i - 20) : ph - 3 + 0.25 * (i - 30);
                O.push_back(lo + 0.5); H.push_back(lo + 2); L.push_back(lo); C.push_back(lo + 1.5);
            }
            O[30] = po; H[30] = ph; L[30] = pl; C[30] = pc;
            return std::vector<std::vector<double> >{ O, H, L, C };
        };
        auto key = [](double p) { return dp::priceKey(p, 0.25); };
        {   // passes everything: green, closes above the node, node in the lower half, its low (101.5) is below the down-pivot bar 19's high (103)
            auto b = chart(101.5, 116, 102, 115.5);
            const dp::Recheck r = dp::recheckA(b[0], b[1], b[2], b[3], 40, 30, key(101.5), key(101.75), 0.25, true);
            check(r.bars && r.close && r.halfway && r.pivot && r.pass() && r.pivotBar == 19, "re-check passes: close / halfway / pivot all ok -> shown");
        }
        {   // halfway fails: node at 115.00 (above the 108.75 midpoint); close 115.50 still above it, pivot still ok
            auto b = chart(101.5, 116, 102, 115.5);
            const dp::Recheck r = dp::recheckA(b[0], b[1], b[2], b[3], 40, 30, key(115.0), key(115.0), 0.25, true);
            check(r.bars && r.close && !r.halfway && r.pivot && !r.pass(), "re-check: node in the UPPER half of a bullish peak bar -> fails halfway -> hidden");
        }
        {   // pivot fails: the peak bar's low 112 stays above the down-pivot bar 19's high 103 and makes no new down pivot
            auto b = chart(112, 116, 112.5, 115.5);
            const dp::Recheck r = dp::recheckA(b[0], b[1], b[2], b[3], 40, 30, key(112.0), key(112.0), 0.25, true);
            check(r.bars && r.close && r.halfway && !r.pivot && !r.pass(), "re-check: no down pivot and low above the pivot bar's high -> fails pivot -> hidden");
        }
        {   // close fails: red peak bar
            auto b = chart(101.5, 116, 115.5, 102);
            const dp::Recheck r = dp::recheckA(b[0], b[1], b[2], b[3], 40, 30, key(101.5), key(101.5), 0.25, true);
            check(r.bars && !r.close && !r.pass(), "re-check: RED peak bar for a selling node -> fails the same-bar close rule");
        }
        {   // bars not on the chart: the peak bar forming, bar 0, NaN inside 0..pk
            auto b = chart(101.5, 116, 102, 115.5);
            check(!dp::recheckA(b[0], b[1], b[2], b[3], 31, 30, key(101.5), key(101.5), 0.25, true).bars, "peak bar still forming -> bars not available (undrawn, retried)");
            check(!dp::recheckA(b[0], b[1], b[2], b[3], 40, 0, key(101.5), key(101.5), 0.25, true).bars, "peak at bar 0 (no bar before it) -> bars not available");
            b[2][5] = std::nan("");
            check(!dp::recheckA(b[0], b[1], b[2], b[3], 40, 30, key(101.5), key(101.5), 0.25, true).bars, "a non-finite bar in 0..peak -> bars not available");
        }
        {   // only bars 0..peak are read: later bars (a lower low) never change the answer
            auto b = chart(101.5, 116, 102, 115.5), c = b;
            for (int i = 31; i < 40; ++i) { c[2][static_cast<std::size_t>(i)] = 90 - i; c[1][static_cast<std::size_t>(i)] = 92 - i; }
            const dp::Recheck r1 = dp::recheckA(b[0], b[1], b[2], b[3], 40, 30, key(101.5), key(101.75), 0.25, true);
            const dp::Recheck r2 = dp::recheckA(c[0], c[1], c[2], c[3], 40, 30, key(101.5), key(101.75), 0.25, true);
            check(r1.pass() == r2.pass() && r1.pivotBar == r2.pivotBar, "bars after the peak never change the re-check (no repaint)");
        }
        {   // bearish mirror: rally to the peak bar 30 (a new high), red close below a buying node in its upper half
            std::vector<double> O, H, L, C;
            for (int i = 0; i < 40; ++i) { const double lo = i <= 30 ? 100 + i : 130 - (i - 30); O.push_back(lo + 1.5); H.push_back(lo + 2); L.push_back(lo); C.push_back(lo + 0.5); }
            O[30] = 131.5; C[30] = 130.25;                               // red; node 131.75 (upper half of 130..132), close below it
            const dp::Recheck r = dp::recheckA(O, H, L, C, 40, 30, key(131.75), key(131.75), 0.25, false);
            check(r.bars && r.close && r.halfway && r.pivot && r.pass(), "bearish: red close below a buying node in the upper half of the up-pivot bar -> passes");
            const dp::Recheck q = dp::recheckA(O, H, L, C, 40, 30, key(130.0), key(130.0), 0.25, false);
            check(q.bars && !q.halfway && !q.close, "bearish: node at the bar's LOW (lower half) fails halfway (and the close is not below it)");
        }
    }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
