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
    dp::BubbleCand c; c.mult = 1.0; c.id = id; c.candle = candle; c.support = support; c.price = price; c.tDecided = tDecided; return c;
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

    // ---- rule 1 (2.13.0): oneBubblePerCandle - ONE bubble per candle (either side): the HIGHER ratio, only among the events decided at the first moment ----
    const double T = 0.25;
    {   // the same peak bar recorded 3 times: ONE bubble, the highest ratio
        std::vector<dp::BubbleCand> v = { cand(15, 1000, true, 7746.25, 50), cand(17, 1000, true, 7745.5, 50), cand(20, 1000, true, 7745.625, 50) };
        v[0].mult = 2.1; v[1].mult = 3.4; v[2].mult = 2.9;
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[15] == 17 && h[17] == 17 && h[20] == 17, "three A on one candle -> ONE bubble, the HIGHEST ratio (3.4x)");
    }
    {   // bearish: same rule, price does not matter
        std::vector<dp::BubbleCand> v = { cand(1, 1000, false, 110.0, 50), cand(2, 1000, false, 111.0, 50), cand(3, 1000, false, 110.5, 50) };
        v[0].mult = 5.0; v[1].mult = 2.0; v[2].mult = 2.5;
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[1] == 1 && h[2] == 1 && h[3] == 1, "bearish: the highest ratio holds the bubble whatever the price");
    }
    {
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50), cand(2, 1180, true, 99, 60) };
        v[0].mult = v[1].mult = 2.0;
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[1] == 1 && h[2] == 2, "two candles -> two bubbles");
    }
    {   // one candle, both sides -> ONE bubble (2.13.0: per candle, not per side)
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50), cand(2, 1000, false, 101, 50) };
        v[0].mult = 2.2; v[1].mult = 3.0;
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[1] == 2 && h[2] == 2, "one candle, both sides -> ONE bubble (the higher ratio)");
    }
    {   // a later duplicate never moves a drawn bubble, even with a higher ratio
        std::vector<dp::BubbleCand> v = { cand(15, 1000, true, 7746.25, 50), cand(17, 1000, true, 7745.5, 53), cand(20, 1000, true, 7744.0, 80) };
        v[0].mult = 2.0; v[1].mult = 4.0; v[2].mult = 9.0;
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[15] == 15 && h[17] == 15 && h[20] == 15, "later duplicates (decided after the first decision) join the drawn bubble, never move it");
        v.push_back(cand(30, 1000, true, 7745.0, 50)); v.back().mult = 2.5;
        h = dp::oneBubblePerCandle(v, T);
        check(h[15] == 30 && h[30] == 30 && h[17] == 30 && h[20] == 30, "a higher ratio decided at the SAME first moment takes the bubble");
    }
    {   // ties: the same ratio -> the lowest id
        std::vector<dp::BubbleCand> v = { cand(9, 1000, true, 100.0, 50), cand(4, 1000, true, 99.0, 50), cand(7, 1000, true, 100.0, 50) };
        v[0].mult = v[1].mult = v[2].mult = 2.5;
        std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
        check(h[9] == 4 && h[7] == 4 && h[4] == 4, "same ratio -> the lowest id holds the bubble");
    }
    {   // load-order independence: every permutation of 6 events gives the same answer
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50), cand(2, 1000, true, 99.5, 50), cand(3, 1000, true, 99.0, 70),
                                          cand(4, 1180, false, 105, 60), cand(5, 1180, false, 105.25, 60), cand(6, 1180, true, 101, 60) };
        v[0].mult = 2.0; v[1].mult = 3.0; v[2].mult = 9.0; v[3].mult = 2.0; v[4].mult = 2.0; v[5].mult = 4.0;
        const std::map<long long, long long> ref = dp::oneBubblePerCandle(v, T);
        std::vector<int> idx = { 0, 1, 2, 3, 4, 5 }; int perms = 0, same = 0;
        do {
            std::vector<dp::BubbleCand> w; for (int i : idx) w.push_back(v[static_cast<std::size_t>(i)]);
            ++perms; if (dp::oneBubblePerCandle(w, T) == ref) ++same;
        } while (std::next_permutation(idx.begin(), idx.end()));
        check(perms == 720 && same == 720, "all 720 load orders give the same bubbles (" + std::to_string(same) + "/" + std::to_string(perms) + ")");
        check(ref.at(1) == 2 && ref.at(3) == 2 && ref.at(4) == 6 && ref.at(6) == 6, "the reference answer: candle 1000 -> ratio 3.0 (id 2), candle 1180 -> ratio 4.0 (id 6)");
    }
    {   // incremental growth = the live path: later-decided events never change an existing holder
        std::vector<dp::BubbleCand> v = { cand(1, 1000, true, 100, 50) }; v[0].mult = 2.0;
        bool stable = true;
        for (int k = 0; k < 20; ++k) {
            v.push_back(cand(100 + k, 1000, true, 100 - 0.25 * (k + 1), 51 + k)); v.back().mult = 3.0 + k;   // each higher, each decided later
            const std::map<long long, long long> h = dp::oneBubblePerCandle(v, T);
            for (std::map<long long, long long>::const_iterator i = h.begin(); i != h.end(); ++i) if (i->second != 1) stable = false;
        }
        check(stable, "20 later, ever-bigger duplicates: the bubble never moves (no repaint)");
    }
    check(dp::oneBubblePerCandle(std::vector<dp::BubbleCand>(), T).empty(), "no events -> no bubbles");
    {   // 2.13.0 candidates: build() lists EVERY row >= 2.0x the average |delta| (not only 3)
        using namespace dp;
        std::vector<Bar> bars; Bar b; b.index = 0; b.time = 60; b.closed = true; b.open = 100; b.high = 110; b.low = 90; b.close = 105;
        for (int k = 0; k < 40; ++k) b.rows.push_back(Row(90 + k / 2, (k % 7 == 0) ? -400.0 : -5.0, 500));
        for (int k = 0; k < 40; ++k) b.rows[(std::size_t)k].key = 80 + k;
        bars.push_back(b);
        Snapshot s2 = build(bars, 1.0, true);
        int big = 0; for (const Bucket& x : s2.buckets) if (x.ratio >= 2.0) ++big;
        check(big > 3 && s2.nodes.size() <= 3, "the fixture has more than 3 rows at 2x or more; the display nodes stay at most 3");
        // (2.15.0, Rassul 2026-10-17 "the 2.6.0 rules - top 3 rows") the 2.13.0 uncapped candidate list is capped at the top 3 again
        check(s2.cands.size() == 3 && s2.cands.size() <= (std::size_t)big, "(2.15.0) cands = the top 3 qualifying rows: " + std::to_string(s2.cands.size()) + " of " + std::to_string(big));
    }

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
            check(r.bars && r.close && !r.halfway && r.pivot && r.pass(), "(2.15.0 no halfway rule) re-check: node in the UPPER half of a bullish peak bar -> halfway reported false, still shown");
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
