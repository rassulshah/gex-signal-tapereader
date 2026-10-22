// lsStopRun 1.1.0 logic tests (StopRunLogic.h): burst detection at REAL levels only, the LIVE absorbed decision, no repaint, record.
#include "../StopRunLogic.h"
#include <cstdio>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace stl;
// the 5-candle pivot low = 1000 (high 1020); prior candle low ALSO 1000 (1.1.0 must not name it)
static Detector base() {
    Detector D; D.P.windowMs = 50; D.P.minFills = 5; D.P.ratio = 2.0; D.P.nearTicks = 4; D.P.bigLots = 20; D.P.minIds = 2; D.P.seedLots = 10;
    D.P.decideS = 10; D.P.sharePct = 50; D.P.requireReclaim = false;
    D.prior.t = 100; D.prior.h = 1010; D.prior.l = 1000; D.prior.c = 1005; D.havePrior = true; D.haveX5 = true; D.x5lo = 1000; D.x5hi = 1020;
    D.formingT = 280; D.minuteOfDay = 600; D.atrTicks = 8;
    return D;
}
// a down burst: n fills of `lots` from `p0` stepping down 1 tick every `step` fills, distinct aggressor ids; optional big bid id
static void downBurst(Detector& D, long long ms, long long sec, long long p0, int n, long long lots, int step, bool ids = true, long long bidId = 0, long long bidLots = 0) {
    long long px = p0;
    for (int i = 0; i < n; ++i) { if (step > 0 && i > 0 && i % step == 0) --px; D.aggFill(ms, sec, DOWN, px, lots, ids ? 500 + i : 0); D.restFill(ms, DOWN, px, lots, 9000 + i); }
    if (bidId) { D.aggFill(ms + 1, sec, DOWN, px, bidLots, ids ? 777 : 0); D.restFill(ms + 1, DOWN, px, bidLots, bidId); }
}
static Ref key(const char* n, long long px) { Ref k; k.name = n; k.px = px; k.dir = keyDir(n, k.rank); return k; }
int main() {
    {   // 1. a 3x burst through the 5-candle PIVOT low fires ONE St at the first price beyond
        Detector D = base();
        downBurst(D, 1000, 50, 1001, 10, 3, 3);       // 30 lots, 1001 -> 998, through 1000
        D.tick(1100, 50);
        CHECK(D.fired.size() == 1, "fires on a 3x burst through the pivot low");
        if (!D.fired.empty()) { const Event& e = D.fired[0]; CHECK(e.side == DOWN && e.refPx == 1000 && e.stPx == 999, "St at the first price beyond the level");
            CHECK(e.ref == "5-candle low" && e.refKind == REF_X5, "reference = the 5-candle pivot low"); CHECK(e.ratio > 2.9 && e.ratio < 3.1, "ratio 3.0x vs seed 10");
            CHECK(e.ids == 10, "10 distinct aggressor orders"); CHECK(e.barT == 280 && e.drawn, "on the forming candle, drawn"); CHECK(markText(e) == "St" && markColour(e) == 0, "grey St when fired"); }
    }
    {   // 2. FEWER: the prior candle low alone (no pivot, no key level there) never fires in 1.1.0
        Detector D = base(); D.x5lo = 990;            // the pivot low is far below; only the prior candle low 1000 is crossed
        downBurst(D, 1000, 50, 1001, 10, 3, 3); D.tick(1100, 50);
        CHECK(D.fired.empty() && D.nearLevel == 0, "prior candle low dropped");
    }
    {   // 3. no fire: too few fills / below ratio / started too far above / not through / one aggressor
        Detector D = base(); downBurst(D, 1000, 50, 1001, 4, 10, 2); D.tick(2000, 52); CHECK(D.fired.empty(), "4 fills < min 5");
        Detector E = base(); downBurst(E, 1000, 50, 1001, 6, 3, 3); E.tick(2000, 52); CHECK(E.fired.empty(), "18 lots = 1.8x < 2.0x");
        Detector F = base(); downBurst(F, 1000, 50, 1006, 30, 3, 5); F.tick(2000, 52); CHECK(F.fired.empty(), "started 6 ticks above (near = 4)");
        Detector G = base(); downBurst(G, 1000, 50, 1003, 10, 3, 0); G.tick(2000, 52); CHECK(G.fired.empty(), "never traded through the level");
        Detector H = base(); for (int i = 0; i < 10; ++i) { H.aggFill(1000, 50, DOWN, 1001 - i / 3, 3, 42); } H.tick(2000, 52); CHECK(H.fired.empty(), "one aggressor order (a sweep, not stops)");
        Detector I = base(); downBurst(I, 1000, 50, 1001, 10, 3, 3, false); I.tick(2000, 52); CHECK(I.fired.size() == 1, "aggressor ids not named by the feed: the ids rule is skipped");
    }
    {   // 4. the window: fills 60 ms apart are separate bursts; a second boundary +2 s splits too
        Detector D = base();
        for (int i = 0; i < 10; ++i) { D.aggFill(1000 + 60 * i, 50, DOWN, 1001 - i / 3, 3, 500 + i); }
        D.tick(5000, 55); CHECK(D.fired.empty(), "60 ms apart = 10 one-fill bursts");
        Detector E = base(); for (int i = 0; i < 10; ++i) { E.aggFill(1000, 50 + 2 * (i / 5), DOWN, 1001 - i / 3, 3, 500 + i); } E.tick(9000, 60);
        CHECK(E.fired.empty(), "same arrival ms but exchange seconds 2 apart = two bursts");
    }
    {   // 5. key levels: a session level beats the pivot; a MenthorQ level fires either side; useKeys off = pivots only
        Detector D = base(); D.keys.push_back(key("LonLO", 1000));
        downBurst(D, 1000, 50, 1001, 10, 3, 3); D.tick(1100, 50);
        CHECK(D.fired.size() == 1 && D.fired[0].ref == "LonLO" && D.fired[0].refKind == REF_KEY, "key level named");
        Detector E = base(); E.haveX5 = false; E.keys.push_back(key("HVL", 1000)); downBurst(E, 1000, 50, 1001, 10, 3, 3); E.tick(1100, 50);
        CHECK(E.fired.size() == 1 && E.fired[0].ref == "HVL", "MenthorQ HVL as a level (down run)");
        Detector F = base(); F.haveX5 = false; F.keys.push_back(key("HVL", 1010));
        for (int i = 0; i < 10; ++i) { F.aggFill(1000, 50, UP, 1009 + i / 3, 3, 600 + i); }
        F.tick(1100, 50);
        CHECK(F.fired.size() == 1 && F.fired[0].ref == "HVL" && F.fired[0].side == UP, "the same MenthorQ level on an up run");
        Detector G = base(); G.haveX5 = false; G.P.useKeys = false; G.keys.push_back(key("LonLO", 1000)); downBurst(G, 1000, 50, 1001, 10, 3, 3); G.tick(1100, 50);
        CHECK(G.fired.empty(), "key levels off: no key-level St");
        int r = 0; CHECK(keyDir("PDH", r) == 1 && keyDir("PDL", r) == -1 && keyDir("CR", r) == 2 && r == 6 && keyDir("G 7900", r) == 0 && keyDir("1DL", r) == 2, "level names");
    }
    {   // 6. ABSORBED LIVE: a big resting bid takes >= 50% and a trade back above the level -> decided at THAT moment (no candle close)
        Detector D = base(); downBurst(D, 1000, 50, 1001, 10, 3, 3, true, 4242, 25);   // 55 lots, the 25-lot bid = 45%
        D.tick(1100, 50); CHECK(D.fired.size() == 1 && D.decided.empty(), "fired, deciding");
        D.aggFill(2000, 51, DOWN, 998, 10, 801); D.restFill(2000, DOWN, 998, 10, 4242);          // 35 / 55 = 63% into the same bid
        CHECK(D.decided.empty(), "share in, but no reclaim / window not over: still deciding");
        D.aggFill(4000, 53, UP, 1001, 2, 802);                                                    // back above 1000
        CHECK(D.decided.size() == 1, "decided on the reclaim, 3 s after the burst");
        if (!D.decided.empty()) { const Event& e = D.decided[0]; CHECK(e.outcome == ABSORBED && e.bigIds == 1 && e.reclaimed && e.sharePct == 63 && e.decidedSec == 53 && e.decidedMs == 4000, "absorbed: 1 big bid took 63%");
            CHECK(markText(e) == "Sta" && markColour(e) == 1, "Sta green (stops below a low absorbed = bullish)");
            CHECK(outcomeText(e, 0.25, 10) == "Absorbed in 3 s: 1 big bid order took 63%, back above the level", "outcome text"); }
        D.aggFill(30000, 80, DOWN, 990, 50, 803); D.tick(40000, 90); CHECK(D.decided.size() == 1 && D.pending.empty(), "never decided again (no repaint)");
    }
    {   // 7. ABSORBED by HOLD: share in, no reclaim, no new low for the whole window -> absorbed when the window ends (default mode)
        Detector D = base(); downBurst(D, 1000, 50, 1001, 10, 3, 3, true, 4242, 40); D.tick(1100, 50);   // 40 / 70 = 57%
        D.aggFill(5000, 54, DOWN, 999, 1, 900); D.tick(10900, 60);
        CHECK(D.decided.empty(), "9.9 s: not yet");
        D.tick(11001, 60);
        CHECK(D.decided.size() == 1 && D.decided[0].outcome == ABSORBED && !D.decided[0].reclaimed && !D.decided[0].newExt, "held: absorbed at the window's end");
        if (!D.decided.empty()) CHECK(outcomeText(D.decided[0], 0.25, 10) == "Absorbed in 10 s: 1 big bid order took 57%, no new low", "hold text");
        Detector E = base(); E.P.requireReclaim = true; downBurst(E, 1000, 50, 1001, 10, 3, 3, true, 4242, 40); E.tick(1100, 50); E.tick(11001, 60);
        CHECK(E.decided.size() == 1 && E.decided[0].outcome == NOT_ABSORBED, "reclaim required: a hold alone is not absorbed");
    }
    {   // 8. NOT absorbed: a new low in the window; no big bid; big bids took too little
        Detector D = base(); downBurst(D, 1000, 50, 1001, 10, 3, 3, true, 4242, 40); D.tick(1100, 50);
        D.aggFill(3000, 52, DOWN, 990, 5, 901); D.tick(11001, 60);
        CHECK(D.decided.size() == 1 && D.decided[0].outcome == NOT_ABSORBED && D.decided[0].newExt && std::fabs(D.decided[0].excAtr - 1.25) < 1e-9, "new low 10 ticks = 1.25 ATR: not absorbed");
        if (!D.decided.empty()) { CHECK(markText(D.decided[0]) == "St" && markColour(D.decided[0]) == 0, "stays grey St");
            CHECK(outcomeText(D.decided[0], 0.25, 10) == "Not absorbed - continuation: new low, 1.2 ATR past the level", "continuation text"); }
        Detector E = base(); downBurst(E, 1000, 50, 1001, 10, 3, 3); E.tick(1100, 50); E.aggFill(2000, 51, UP, 1002, 1, 902); E.tick(11001, 60);
        CHECK(E.decided.size() == 1 && E.decided[0].outcome == NOT_ABSORBED && E.decided[0].reclaimed && E.decided[0].bigIds == 0, "reclaim without a big id = not absorbed");
        if (!E.decided.empty()) CHECK(outcomeText(E.decided[0], 0.25, 10) == "Not absorbed - continuation: no big resting bid took the burst", "no big bid text");
        Detector F = base(); downBurst(F, 1000, 50, 1001, 10, 3, 3, true, 4242, 20); F.tick(1100, 50); F.aggFill(2000, 51, UP, 1002, 1, 903); F.tick(11001, 60);
        CHECK(F.decided.size() == 1 && F.decided[0].outcome == NOT_ABSORBED && F.decided[0].sharePct == 40, "big bid took 40% < 50%");
    }
    {   // 9. UP run absorbed = red Sta; big OFFER ids only count at/above the level
        Detector D = base();
        for (int i = 0; i < 10; ++i) { D.aggFill(1000, 50, UP, 1019 + i / 3, 3, 600 + i); D.restFill(1000, UP, 1019 + i / 3, 3, 8000 + i); }
        D.aggFill(1001, 50, UP, 1021, 30, 699); D.restFill(1001, UP, 1021, 30, 5151); D.restFill(1001, DOWN, 1015, 50, 6161);
        D.tick(1100, 50);
        CHECK(D.fired.size() == 1 && D.fired[0].side == UP && D.fired[0].refPx == 1020 && D.fired[0].stPx == 1021, "up run through the pivot high");
        D.aggFill(3000, 52, DOWN, 1019, 1, 904);
        CHECK(D.decided.size() == 1 && D.decided[0].outcome == ABSORBED && D.decided[0].bigIds == 1 && markColour(D.decided[0]) == 2, "red Sta; the 50-lot BID below never counts as an offer absorber");
    }
    {   // 10. at most 2 marks per candle; the 3rd is recorded, not drawn
        Detector D = base(); D.haveX5 = false; D.keys.push_back(key("PDL", 1000)); D.keys.push_back(key("ONL", 995)); D.keys.push_back(key("LonLO", 990));
        for (int r = 0; r < 3; ++r) { downBurst(D, 1000 + 500 * r, 50 + 3 * r, 1001 - 5 * r, 10, 3, 3); D.tick(1300 + 500 * r, 51 + 3 * r); }
        CHECK(D.fired.size() == 3 && D.fired[0].drawn && D.fired[1].drawn && !D.fired[2].drawn, "3rd mark in a candle recorded only");
    }
    {   // 11. the normal burst: the slot median after 20 qualifying bursts, no look-ahead
        Detector D = base(); for (int i = 0; i < 20; ++i) { for (int k = 0; k < 5; ++k) D.aggFill(100000 + 1000 * i, 1000 + 3 * i, UP, 2000, 4, 1 + k); D.tick(100000 + 1000 * i + 500, 1002 + 3 * i); }
        CHECK(std::fabs(D.normNow(20) - 20.0) < 1e-9, "slot median of 20 bursts of 20 lots"); CHECK(std::fabs(D.normNow(5) - 20.0) < 1e-9, "another slot: the any-slot median");
    }
    {   // 12. record round trip (+ decided_sec); the first SA per key wins; 1.0.0 8-field SA lines still read; garbled rejected
        Event e; e.key = "50-d-999"; e.barT = 280; e.firedSec = 50; e.ct = "9:41:05 AM"; e.side = DOWN; e.ref = "LonLO"; e.refPx = 30690; e.stPx = 30689; e.lots = 412; e.fills = 38; e.ids = 9; e.norm = 133; e.ratio = 3.1; e.drawn = true;
        std::vector<std::string> L; L.push_back(REC_HEAD); L.push_back(stLine(e, 0.25));
        Event d = e; d.outcome = ABSORBED; d.decidedT = 280; d.bars = 1; d.bigIds = 2; d.reclaimed = true; d.decidedSec = 54; L.push_back(saLine(d));
        CHECK(saLine(d) == "SA|50-d-999|280|absorbed|1|2|0|1|54", "SA line with the decision second");
        Event x = e; x.outcome = NOT_ABSORBED; x.decidedT = 640; x.bars = 3; L.push_back(saLine(x));
        Event o = e; o.key = "old"; L.push_back(stLine(o, 0.25)); L.push_back("SA|old|460|not|3|0|150|0");
        L.push_back("ST|bad"); L.push_back("SA|nokey|1|absorbed|1|1|0|1|5"); L.push_back("ST|k2|abc|1|t|down|PDL|1.0|1.0|1|1|1|1|1|1"); L.push_back("SA|old2|1|absorbed|1|1|0|1|x");
        long long bad = 0; std::vector<Event> v = parseRecord(L, 0.25, &bad);
        CHECK(v.size() == 2 && bad == 4, "2 events, 4 bad lines");
        if (v.size() == 2) { CHECK(v[0].outcome == ABSORBED && v[0].bigIds == 2 && v[0].decidedSec == 54 && v[0].stPx == 30689 && v[0].refPx == 30690 && v[0].ratio > 3.09 && v[0].ct == "9:41:05 AM", "round trip");
            CHECK(markText(v[0]) == "Sta" && markColour(v[0]) == 1, "the later SA line never changes a decided mark");
            CHECK(v[1].outcome == NOT_ABSORBED && v[1].decidedSec == 0 && v[1].bars == 3, "a 1.0.0 SA line (no decision second) still reads"); }
        CHECK(stLine(e, 0.25).find("|7672.50|7672.25|") != std::string::npos, "prices with the tick's decimals");
    }
    {   // 13. hover rows: header with ?, the run, outcome, then question / what / why / how / example
        Event e; e.key = "k"; e.side = DOWN; e.ref = "LonLO"; e.refPx = 30690; e.stPx = 30689; e.lots = 412; e.fills = 38; e.ids = 9; e.norm = 133; e.ratio = 3.1; e.ct = "9:41:05 AM";
        std::vector<std::string> r = hoverRows(e, 0.25, 10);
        CHECK(r.size() == 9 && r[0].find("Stop run?") != std::string::npos, "9 rows, header Stop run?");
        CHECK(r[1] == "Stop run at 7672.25 below LonLO 7672.50: 3.1x normal stop burst", "headline");
        CHECK(r[3] == "Absorbed? deciding - watching 10 s for big resting bids and a move back above 7672.50", "deciding text");
        CHECK(r[4].find("Question:") == 0 && r[5].find("What it is:") == 0 && r[6].find("Why it matters:") == 0 && r[7].find("How to use:") == 0 && r[8].find("Example:") == 0, "structure");
        e.orphan = true; CHECK(hoverRows(e, 0.25, 10)[3].find("restarted") != std::string::npos, "orphan text after a restart");
        Event f = e; f.orphan = false; f.outcome = ABSORBED; f.bigIds = 2; f.reclaimed = true; f.sharePct = -1; f.decidedSec = 0;
        CHECK(outcomeText(f, 0.25, 10) == "Absorbed: 2 big bid orders, back above the level", "reloaded 1.0.0 decision: no time / share shown");
        for (size_t k = 0; k < r.size(); ++k) CHECK(r[k].size() <= 110, "row fits a hover line");
    }
    {   // 14. prefix replay identical: tick() between every fill changes nothing
        Detector A = base(), B = base();
        for (int i = 0; i < 10; ++i) { A.aggFill(1000, 50, DOWN, 1001 - i / 3, 3, 500 + i); B.aggFill(1000, 50, DOWN, 1001 - i / 3, 3, 500 + i); B.tick(1000, 50); }
        A.tick(2000, 52); B.tick(2000, 52);
        CHECK(A.fired.size() == 1 && B.fired.size() == 1 && A.fired[0].key == B.fired[0].key && A.fired[0].stPx == B.fired[0].stPx, "tick between fills never splits a burst inside its window");
        A.tick(20000, 70); B.tick(15000, 65); B.tick(20000, 70);
        CHECK(A.decided.size() == 1 && B.decided.size() == 1 && A.decided[0].outcome == B.decided[0].outcome, "same decision with extra ticks");
    }
    {   // 15. candle closes no longer decide; they count the candles waited
        Detector D = base(); D.P.decideS = 60; downBurst(D, 1000, 50, 1001, 10, 3, 3, true, 4242, 40); D.tick(1100, 50);
        Bar c; c.t = 280; c.h = 1004; c.l = 999; c.c = 1003; D.barClose(c);
        CHECK(D.decided.empty() && D.pending.size() == 1 && D.pending[0].k == 1, "a close back above alone decides nothing");
    }
    {   // 16. the 5-candle pivot: confirmed only when 2 later candles stayed inside; a running extreme is not a pivot
        std::vector<Bar> c(5); for (int i = 0; i < 5; ++i) { c[i].t = 180 * (i + 1); c[i].h = 1010; c[i].l = 1000; }
        c[2].l = 995; c[1].h = 1015; bool hl = false, hh = false; long long lo = 0, hi = 0;
        pivot5(c, hl, lo, hh, hi); CHECK(hl && lo == 995 && hh && hi == 1015, "pivot low at candle 3 / high at candle 2: confirmed");
        c[4].l = 990; pivot5(c, hl, lo, hh, hi); CHECK(!hl && hh, "the last candle made a new low: running extreme, no pivot low");
        c[4].l = 1000; c[3].l = 995; pivot5(c, hl, lo, hh, hi); CHECK(!hl, "equal low again 1 candle ago: the latest counts, not confirmed");
        std::vector<Bar> d(4); pivot5(d, hl, lo, hh, hi); CHECK(!hl && !hh, "fewer than 5 candles: none");
        Detector D = base(); D.haveX5lo = false; downBurst(D, 1000, 50, 1001, 10, 3, 3); D.tick(1100, 50); CHECK(D.fired.empty(), "no confirmed pivot low: no St");
    }
    {   // 17. a level's stops fire ONCE per session: a second burst through the same level is not a new St
        Detector D = base(); downBurst(D, 1000, 50, 1001, 10, 3, 3); D.tick(1100, 50);
        downBurst(D, 60000, 110, 1001, 10, 3, 3); D.tick(60100, 110);
        CHECK(D.fired.size() == 1 && D.runs == 1, "same level, same side: once");
        Detector E = base(); E.markUsed(DOWN, 1000); downBurst(E, 1000, 50, 1001, 10, 3, 3); E.tick(1100, 50);
        CHECK(E.fired.empty(), "used from the record after a restart: not again");
    }
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
