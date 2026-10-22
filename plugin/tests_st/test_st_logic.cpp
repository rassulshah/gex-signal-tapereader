// lsStopRun 1.0.0 logic tests (StopRunLogic.h): burst detection, absorbed vs not, no repaint, record.
#include "../StopRunLogic.h"
#include <cstdio>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace stl;
static Detector base() {
    Detector D; D.P.windowMs = 50; D.P.minFills = 5; D.P.ratio = 2.0; D.P.nearTicks = 4; D.P.bigLots = 20; D.P.absorbBars = 3; D.P.minIds = 2; D.P.seedLots = 10;
    D.prior.t = 100; D.prior.h = 1010; D.prior.l = 1000; D.prior.c = 1005; D.havePrior = true; D.formingT = 280; D.minuteOfDay = 600; D.atrTicks = 8;
    return D;
}
// a down burst: n fills of `lots` from price `p0` stepping down 1 tick every `step` fills, distinct aggressor ids
static void downBurst(Detector& D, long long ms, long long sec, long long p0, int n, long long lots, int step, bool ids = true, long long bidId = 0, long long bidLots = 0) {
    long long px = p0;
    for (int i = 0; i < n; ++i) { if (step > 0 && i > 0 && i % step == 0) --px; D.aggFill(ms, sec, DOWN, px, lots, ids ? 500 + i : 0); D.restFill(ms, DOWN, px, lots, 9000 + i); }
    if (bidId) { D.aggFill(ms + 1, sec, DOWN, px, bidLots, ids ? 777 : 0); D.restFill(ms + 1, DOWN, px, bidLots, bidId); }
}
int main() {
    {   // 1. a 2x burst just through the prior candle low fires ONE St at the first price beyond
        Detector D = base();
        downBurst(D, 1000, 50, 1001, 10, 3, 3);       // 30 lots, 1001 -> 998, through 1000
        D.tick(2000, 52);
        CHECK(D.fired.size() == 1, "fires on a 3x burst through the prior low");
        if (!D.fired.empty()) { const Event& e = D.fired[0]; CHECK(e.side == DOWN && e.refPx == 1000 && e.stPx == 999, "St at the first price beyond the level");
            CHECK(e.ref == "prior candle low" && e.refKind == REF_PRIOR, "reference = prior candle low"); CHECK(e.ratio > 2.9 && e.ratio < 3.1, "ratio 3.0x vs seed 10");
            CHECK(e.ids == 10, "10 distinct aggressor orders"); CHECK(e.barT == 280 && e.drawn, "on the forming candle, drawn"); CHECK(markText(e) == "St" && markColour(e) == 0, "grey St when fired"); }
    }
    {   // 2. no fire: too few fills / below ratio / started too far above / not through / one aggressor
        Detector D = base(); downBurst(D, 1000, 50, 1001, 4, 10, 2); D.tick(2000, 52); CHECK(D.fired.empty(), "4 fills < min 5");
        Detector E = base(); downBurst(E, 1000, 50, 1001, 6, 3, 3); E.tick(2000, 52); CHECK(E.fired.empty(), "18 lots = 1.8x < 2.0x");
        Detector F = base(); downBurst(F, 1000, 50, 1006, 30, 3, 5); F.tick(2000, 52); CHECK(F.fired.empty(), "started 6 ticks above (near = 4)");
        Detector G = base(); downBurst(G, 1000, 50, 1003, 10, 3, 0); G.tick(2000, 52); CHECK(G.fired.empty(), "never traded through the level");
        Detector H = base(); for (int i = 0; i < 10; ++i) { H.aggFill(1000, 50, DOWN, 1001 - i / 3, 3, 42); } H.tick(2000, 52); CHECK(H.fired.empty(), "one aggressor order (a sweep, not stops)");
        Detector I = base(); downBurst(I, 1000, 50, 1001, 10, 3, 3, false); I.tick(2000, 52); CHECK(I.fired.size() == 1, "aggressor ids not named by the feed: the ids rule is skipped");
    }
    {   // 3. the window: fills 60 ms apart are separate bursts (none big enough alone); a second boundary +2 s splits too
        Detector D = base();
        for (int i = 0; i < 10; ++i) { D.aggFill(1000 + 60 * i, 50, DOWN, 1001 - i / 3, 3, 500 + i); }
        D.tick(5000, 55); CHECK(D.fired.empty(), "60 ms apart = 10 one-fill bursts");
        Detector E = base(); for (int i = 0; i < 10; ++i) { E.aggFill(1000, 50 + 2 * (i / 5), DOWN, 1001 - i / 3, 3, 500 + i); } E.tick(9000, 60);
        CHECK(E.fired.empty(), "same arrival ms but exchange seconds 2 apart = two bursts (a drained backlog is never one burst)");
    }
    {   // 4. key level beats the candle extremes; 5-candle low beats the prior candle low
        Detector D = base(); Ref k; k.name = "LonLO"; k.px = 1000; k.dir = -1; keyDir(k.name, k.rank); D.keys.push_back(k);
        downBurst(D, 1000, 50, 1001, 10, 3, 3); D.tick(2000, 52);
        CHECK(D.fired.size() == 1 && D.fired[0].ref == "LonLO" && D.fired[0].refKind == REF_KEY, "key level named");
        Detector E = base(); E.haveX5 = true; E.x5lo = 999; E.x5hi = 1020; downBurst(E, 1000, 50, 1001, 12, 3, 3); E.tick(2000, 52);
        CHECK(E.fired.size() == 1 && E.fired[0].refKind == REF_X5 && E.fired[0].refPx == 999, "5-candle low preferred over the prior low");
    }
    {   // 5. ABSORBED: a big resting bid id at/below the level + a close back above in candle 2 -> Sta green, decided once
        Detector D = base(); downBurst(D, 1000, 50, 1001, 10, 3, 3, true, 4242, 25); D.tick(2000, 52);
        CHECK(D.fired.size() == 1, "fired"); Bar c1; c1.t = 280; c1.h = 1004; c1.l = 997; c1.c = 999; D.barClose(c1);
        CHECK(D.decided.empty(), "candle 1 closed below: still deciding");
        Bar c2; c2.t = 460; c2.h = 1006; c2.l = 998; c2.c = 1003; D.barClose(c2);
        CHECK(D.decided.size() == 1, "decided at candle 2");
        if (!D.decided.empty()) { const Event& e = D.decided[0]; CHECK(e.outcome == ABSORBED && e.bigIds == 1 && e.bars == 2 && e.reclaimed, "absorbed by 1 bid id in 2 candles");
            CHECK(markText(e) == "Sta" && markColour(e) == 1, "Sta green (stops below a low absorbed = bullish)");
            CHECK(outcomeText(e, 0.25, 3) == "Absorbed by 1 bid id, back above in 2 candles", "outcome text"); }
        Bar c3; c3.t = 640; c3.h = 1000; c3.l = 990; c3.c = 991; D.barClose(c3); CHECK(D.decided.size() == 1 && D.pending.empty(), "never decided again (no repaint)");
    }
    {   // 6. NOT absorbed: no close back in 3 candles -> stays St, "kept going X ATR"
        Detector D = base(); downBurst(D, 1000, 50, 1001, 10, 3, 3, true, 4242, 25); D.tick(2000, 52);
        Bar b; b.t = 280; b.h = 1001; b.l = 996; b.c = 997; D.barClose(b); b.t = 460; b.l = 990; b.c = 991; D.barClose(b); b.t = 640; b.l = 988; b.c = 989; D.barClose(b);
        CHECK(D.decided.size() == 1 && D.decided[0].outcome == NOT_ABSORBED && !D.decided[0].reclaimed && D.decided[0].bars == 3, "not absorbed after 3 candles");
        if (!D.decided.empty()) { CHECK(std::fabs(D.decided[0].excAtr - 1.5) < 1e-9, "kept going 12 ticks = 1.5 ATR"); CHECK(markText(D.decided[0]) == "St" && markColour(D.decided[0]) == 0, "stays grey St");
            CHECK(outcomeText(D.decided[0], 0.25, 3) == "Not absorbed - continuation: price kept going 1.5 ATR", "continuation text"); }
    }
    {   // 7. closed back but NO big resting order -> not absorbed (reclaimed)
        Detector D = base(); downBurst(D, 1000, 50, 1001, 10, 3, 3); D.tick(2000, 52);
        Bar b; b.t = 280; b.h = 1004; b.l = 998; b.c = 1002; D.barClose(b);
        CHECK(D.decided.size() == 1 && D.decided[0].outcome == NOT_ABSORBED && D.decided[0].reclaimed && D.decided[0].bigIds == 0, "reclaim without a big id = not absorbed");
    }
    {   // 8. UP run absorbed = red Sta; big OFFER ids only count at/above the level
        Detector D = base();
        for (int i = 0; i < 10; ++i) { D.aggFill(1000, 50, UP, 1009 + i / 3, 3, 600 + i); D.restFill(1000, UP, 1009 + i / 3, 3, 8000 + i); }
        D.aggFill(1001, 50, UP, 1011, 30, 699); D.restFill(1001, UP, 1011, 30, 5151); D.restFill(1001, DOWN, 1005, 50, 6161);
        D.tick(2000, 52);
        CHECK(D.fired.size() == 1 && D.fired[0].side == UP && D.fired[0].refPx == 1010 && D.fired[0].stPx == 1011, "up run through the prior high");
        Bar b; b.t = 280; b.h = 1013; b.l = 1004; b.c = 1006; D.barClose(b);
        CHECK(D.decided.size() == 1 && D.decided[0].outcome == ABSORBED && D.decided[0].bigIds == 1 && markColour(D.decided[0]) == 2, "red Sta; the 50-lot BID below never counts as an offer absorber");
    }
    {   // 9. at most 2 marks per candle; the 3rd is recorded, not drawn
        Detector D = base(); D.havePrior = false; Ref k1; k1.name = "PDL"; k1.px = 1000; k1.dir = -1; keyDir("PDL", k1.rank); D.keys.push_back(k1);
        for (int r = 0; r < 3; ++r) { downBurst(D, 1000 + 500 * r, 50 + 3 * r, 1001, 10, 3, 3); D.tick(1300 + 500 * r, 51 + 3 * r); }
        CHECK(D.fired.size() == 3 && D.fired[0].drawn && D.fired[1].drawn && !D.fired[2].drawn, "3rd mark in a candle recorded only");
    }
    {   // 10. the normal burst: the slot median after 20 qualifying bursts, no look-ahead (the burst itself is added after)
        Detector D = base(); for (int i = 0; i < 20; ++i) { for (int k = 0; k < 5; ++k) D.aggFill(100000 + 1000 * i, 1000 + 3 * i, UP, 2000, 4, 1 + k); D.tick(100000 + 1000 * i + 500, 1002 + 3 * i); }
        CHECK(std::fabs(D.normNow(20) - 20.0) < 1e-9, "slot median of 20 bursts of 20 lots"); CHECK(std::fabs(D.normNow(5) - 20.0) < 1e-9, "another slot: the any-slot median");
    }
    {   // 11. record round trip; the first SA per key wins; garbled lines rejected
        Event e; e.key = "50-d-999"; e.barT = 280; e.firedSec = 50; e.ct = "9:41:05 AM"; e.side = DOWN; e.ref = "LonLO"; e.refPx = 30690; e.stPx = 30689; e.lots = 412; e.fills = 38; e.ids = 9; e.norm = 133; e.ratio = 3.1; e.drawn = true;
        std::vector<std::string> L; L.push_back(REC_HEAD); L.push_back(stLine(e, 0.25));
        Event d = e; d.outcome = ABSORBED; d.decidedT = 460; d.bars = 1; d.bigIds = 2; d.reclaimed = true; L.push_back(saLine(d));
        Event x = e; x.outcome = NOT_ABSORBED; x.decidedT = 640; x.bars = 3; L.push_back(saLine(x));
        L.push_back("ST|bad"); L.push_back("SA|nokey|1|absorbed|1|1|0|1"); L.push_back("ST|k2|abc|1|t|down|PDL|1.0|1.0|1|1|1|1|1|1");
        long long bad = 0; std::vector<Event> v = parseRecord(L, 0.25, &bad);
        CHECK(v.size() == 1 && bad == 3, "1 event, 3 bad lines");
        if (v.size() == 1) { CHECK(v[0].outcome == ABSORBED && v[0].bigIds == 2 && v[0].bars == 1 && v[0].stPx == 30689 && v[0].refPx == 30690 && v[0].ratio > 3.09 && v[0].ct == "9:41:05 AM", "round trip");
            CHECK(markText(v[0]) == "Sta" && markColour(v[0]) == 1, "the later SA line never changes a decided mark"); }
        CHECK(stLine(e, 0.25).find("|7672.50|7672.25|") != std::string::npos, "prices with the tick's decimals");
    }
    {   // 12. hover rows: header with ?, the run, outcome, then question / what / why / how / example
        Event e; e.key = "k"; e.side = DOWN; e.ref = "LonLO"; e.refPx = 30690; e.stPx = 30689; e.lots = 412; e.fills = 38; e.ids = 9; e.norm = 133; e.ratio = 3.1; e.ct = "9:41:05 AM";
        std::vector<std::string> r = hoverRows(e, 0.25, 3);
        CHECK(r.size() == 9 && r[0].find("Stop run?") != std::string::npos, "9 rows, header Stop run?");
        CHECK(r[1] == "Stop run at 7672.25 below LonLO 7672.50: 3.1x normal stop burst", "headline");
        CHECK(r[3].find("deciding") != std::string::npos && r[4].find("Question:") == 0 && r[5].find("What it is:") == 0 && r[6].find("Why it matters:") == 0 && r[7].find("How to use:") == 0 && r[8].find("Example:") == 0, "structure");
        e.orphan = true; CHECK(hoverRows(e, 0.25, 3)[3].find("restarted") != std::string::npos, "orphan text after a restart");
        for (size_t k = 0; k < r.size(); ++k) CHECK(r[k].size() <= 110, "row fits a hover line");
    }
    {   // 13. prefix replay identical: the same fills fed in one pass vs with tick() calls between every fill give the same fires
        Detector A = base(), B = base();
        for (int i = 0; i < 10; ++i) { A.aggFill(1000, 50, DOWN, 1001 - i / 3, 3, 500 + i); B.aggFill(1000, 50, DOWN, 1001 - i / 3, 3, 500 + i); B.tick(1000, 50); }
        A.tick(2000, 52); B.tick(2000, 52);
        CHECK(A.fired.size() == 1 && B.fired.size() == 1 && A.fired[0].key == B.fired[0].key && A.fired[0].stPx == B.fired[0].stPx, "tick between fills never splits a burst inside its window");
    }
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
