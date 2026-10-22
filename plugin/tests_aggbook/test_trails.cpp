// test_trails.cpp -- MoveLogic 1.2.0 TRAILS (2026-10-22, Rassul "ROLLING BIDS / OFFERS DURING THE MOVE", T-20261022-17):
// Tracker side (follow / pulled / filled / stepped back / ladder shift ignored / small orders / restart line) and the chart side
// (TrailSet parse, the stop-following B? on candles, the record round-trip, no repaint, at most 2 per side, the hover).
#include "MoveLogic.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace mvl;
static int count(const std::vector<std::string>& v, const char* tag) { int n = 0; for (auto& s : v) n += s.compare(0, 2, tag) == 0; return n; }
static void feed(Tracker& t, long long ms) { std::vector<Move> o; t.confirm(ms, o); }
// a trail-capable tracker (ES floor 25 lots)
static Tracker mk() { Tracker t; t.minLots = 6; t.trailMin = trailMinOf("ES"); t.key = 100; return t; }
static std::vector<TBar> barsUp(int n, long long start, long long step) {   // a rally: each candle 4 ticks range, rising by `step`
    std::vector<TBar> b; for (int i = 0; i < n; ++i) { TBar x; x.key = 100 + i; x.lo = start + i * step; x.hi = x.lo + 4; x.cl = x.hi - 1; b.push_back(x); } return b;
}
int main() {
    CHECK(std::string(VERSION) == "1.2.0", "version 1.2.0");
    CHECK(trailMinOf("ES") == 25 && trailMinOf("NQ") == 15 && trailMinOf("GC") == 9 && trailMinOf("EU") == 41, "S20 resting floor per market");
    for (const char* m : {"CL", "ES", "EU", "GC", "HG", "NG", "NQ"}) CHECK(trailMinOf(m) >= minLotsOf(m), "the trail floor is >= the q90 of moves");
    // 1. FOLLOW: a 40-lot bid stepping up 3 times -> TS after the 2nd step, TP x3
    {   Tracker t = mk();
        t.modify(7, BIDS, 1000, 1002, 40, 1003, 1005, 1000); feed(t, 1010);
        CHECK(t.trailOut.empty(), "one step: no trail yet");
        t.key = 101; t.modify(7, BIDS, 1002, 1004, 40, 1005, 1007, 5000); feed(t, 5010);
        CHECK(count(t.trailOut, "TS") == 1 && count(t.trailOut, "TP") == 2, "2 follow steps: TS + both steps");
        CHECK(t.trailOut[0] == "TS|7|0|40|100|1000|1000", "TS carries side, size, first step key / ms and the price before it");
        CHECK(t.trailOut[1] == "TP|7|100|1000|1002|0|3|2" && t.trailOut[2] == "TP|7|101|5000|1004|0|3|2", "TP lines in order (+ ticks behind the trade, ticks price ran first)");
        t.key = 102; t.modify(7, BIDS, 1004, 1006, 40, 1007, 1009, 9000); feed(t, 9010);
        CHECK(count(t.trailOut, "TP") == 3 && t.trStarted == 1 && t.trailsOpen() == 1, "a 3rd step extends it");
        // the extreme-read path is unchanged: these moves are 'following' (same distance) and never reach the P / B read
        CHECK(t.following == 3 && t.confirmed == 0, "old extreme reads unchanged (following moves dropped there)");
        // 2. FILLED: a fill on it ends it with TF
        t.key = 103; t.fill(7, 1006, 12, 12000);
        CHECK(count(t.trailOut, "TF") == 1 && t.trailOut.back() == "TF|7|103|12000|1006|12" && t.trailsOpen() == 0, "traded -> TF (dot)");
        t.fill(7, 1006, 5, 12100); CHECK(count(t.trailOut, "TF") == 1, "ends once");
    }
    // 3. PULLED
    {   Tracker t = mk();
        t.modify(8, OFFERS, 2000, 1998, 30, 1995, 1994, 1000); t.modify(8, OFFERS, 1998, 1996, 30, 1994, 1992, 2000); feed(t, 2100);
        CHECK(count(t.trailOut, "TS") == 1, "an offer stepping DOWN is a trail");
        t.cancel(8, OFFERS, 1996, 30, 1992, 3000);
        CHECK(count(t.trailOut, "TX") == 1 && t.trailOut.back() == "TX|8|100|3000|1996", "pulled -> TX (x) at its last price");
    }
    // 4. STEPPED BACK: a trail moved away from price
    {   Tracker t = mk();
        t.modify(9, BIDS, 1000, 1001, 50, 1003, 1004, 1000); t.modify(9, BIDS, 1001, 1002, 50, 1004, 1005, 2000);
        t.modify(9, BIDS, 1002, 999, 50, 1005, 1005, 3000); feed(t, 3100);
        CHECK(count(t.trailOut, "TB") == 1 && t.trailOut.back() == "TB|9|100|3000|999", "moved away -> TB (B?)");
    }
    // 5. LADDER SHIFT: 5 orders moved by the same tick change in the same 5 ms never start a trail
    {   Tracker t = mk();
        for (int r = 0; r < 2; ++r) { for (long long id = 20; id < 26; ++id) t.modify(id, BIDS, 1000 + r, 1001 + r, 40, 1002, 1003, 1000 + 1000 * r + (id - 20) / 3); feed(t, 1100 + 1000 * r); }
        CHECK(count(t.trailOut, "TS") == 0 && t.trLadder >= 12, "ladder shifts ignored (no trail, counted)");
    }
    // 6. SMALL: under the S20 floor never trails; and a stream start writes RS
    {   Tracker t = mk();
        t.modify(30, BIDS, 1000, 1001, 10, 1002, 1003, 1000); t.modify(30, BIDS, 1001, 1002, 10, 1003, 1004, 2000); feed(t, 2100);
        CHECK(t.trailOut.empty(), "10 lots < ES 25: no trail");
        Tracker off; off.modify(31, BIDS, 1000, 1001, 90, 1002, 1003, 1000); off.modify(31, BIDS, 1001, 1002, 90, 1003, 1004, 2000); feed(off, 2100);
        CHECK(off.trailOut.empty() && off.trailMin == 0, "trailMin 0 (BidAskStats / LiquidityProfile engines): trails off");
        t.setTrailFile("/tmp/mvl_trail_test.csv", 0.25); CHECK(!t.trailOut.empty() && t.trailOut.back().compare(0, 3, "RS|") == 0, "a new stream writes RS");
        std::remove("/tmp/mvl_trail_test.csv"); CHECK(t.flushTrails() && t.trailOut.empty(), "flushed to the file");
        std::remove("/tmp/mvl_trail_test.csv");
    }
    // 6b. (NQ 10-09 replay) QUOTE MANAGEMENT: a re-price toward price with NO run first is not a step; FAR orders (beyond the near band) never trail
    {   Tracker t = mk();
        t.modify(40, BIDS, 1000, 1002, 40, 1005, 1005, 1000); t.modify(40, BIDS, 1002, 1004, 40, 1005, 1005, 2000); feed(t, 2100);
        CHECK(t.trailOut.empty(), "stepping up while price did not move: quote management, no trail");
        Tracker f = mk(); f.trailNear = 40;
        f.modify(41, BIDS, 700, 702, 40, 1003, 1005, 1000); f.modify(41, BIDS, 702, 704, 40, 1005, 1007, 2000); feed(f, 2100);
        CHECK(f.trailOut.empty(), "a bid 300 ticks under price stepping with it (band keeping): beyond the near band, no trail");
        f.modify(42, BIDS, 990, 992, 40, 1003, 1005, 3000); f.modify(42, BIDS, 992, 994, 40, 1005, 1007, 4000); feed(f, 4100);
        CHECK(count(f.trailOut, "TS") == 1, "13 ticks behind (inside the near band 40): a trail");
        f.modify(42, BIDS, 994, 997, 40, 1007, 1007, 5000); feed(f, 5100);
        CHECK(count(f.trailOut, "TP") == 3 && f.trailOut.back() == "TP|42|100|5000|997|1|10|0", "an active trail's re-price without a run: drawn (flag 1), not counted");
        TrailSet S; for (auto& l : f.trailOut) S.line(l); CHECK(S.v.size() == 1 && S.v[0].moves() == 2 && S.v[0].lastPx() == 997 && S.bad == 0, "parsed: 2 counted moves, the price kept");
    }
    // ---- the chart side ----
    // 7. B? STOP FOLLOWING: steps in candles 0, 1, 2; price keeps rallying 3 ticks a candle; ATR ~4-5; gap 0.5 ATR ~2.5 ticks
    {   std::vector<TBar> b = barsUp(12, 1000, 3);
        TrailSet T; CHECK(T.line("RS|0|0.25"), "RS parsed");
        T.line("TS|7|0|40|100|1000|990"); T.line("TP|7|100|1000|994|0"); T.line("TP|7|101|2000|997|0"); T.line("TP|7|102|3000|1000|0");
        CHECK(T.v.size() == 1 && T.v[0].moves() == 3 && T.v[0].ticks() == 10, "parsed: 3 moves, 10 ticks");
        // candle 3 runs >= gap above candle 2's high; N = 2 -> B? at candle 4 once candle 4 can be decided
        CHECK(decideStops(T, b, 3, 0.5, 2) == 0 && T.v[0].end == 0, "not decidable yet (candle 4 not closed)");
        CHECK(decideStops(T, b, 4, 0.5, 2) == 1 && T.v[0].end == 'B' && T.v[0].endKey == 104, "B? at the close of candle 4 (ran in 3, no step in 3-4)");
        CHECK(finalEnd(T.v[0], b, 4), "B? is final");
        // it followed in time: a step in candle 4 -> no B? at 4
        TrailSet U; U.line("RS|0|0.25"); U.line("TS|7|0|40|100|1000|990"); U.line("TP|7|100|1000|994|0"); U.line("TP|7|101|2000|997|0"); U.line("TP|7|102|3000|1000|0"); U.line("TP|7|104|5000|1009|0");
        decideStops(U, b, 6, 0.5, 2); CHECK(U.v[0].end == 'B' && U.v[0].endKey == 106, "a step in candle 4 keeps it alive; it stops after that: B? at 6");
        // N = 3 waits a candle longer
        TrailSet W; W.line("TS|7|0|40|100|1000|990"); W.line("TP|7|100|1000|994|0"); W.line("TP|7|101|2000|997|0"); W.line("TP|7|102|3000|1000|0");
        decideStops(W, b, 4, 0.5, 3); CHECK(W.v[0].end == 0, "N=3: not yet at 4"); decideStops(W, b, 5, 0.5, 3); CHECK(W.v[0].endKey == 105, "N=3: B? at 5");
        // a flat market: price never runs -> no B?
        std::vector<TBar> flat = barsUp(12, 1000, 0); TrailSet F; F.line("TS|7|0|40|100|1000|990"); F.line("TP|7|100|1000|994|0"); F.line("TP|7|101|2000|997|0");
        decideStops(F, flat, 11, 0.5, 2); CHECK(F.v[0].end == 0, "price did not run on: still following, no B?");
        // an offer in a selloff
        std::vector<TBar> dn = barsUp(12, 1100, -3); TrailSet O; O.line("TS|8|1|30|100|1000|1110"); O.line("TP|8|100|1000|1106|0"); O.line("TP|8|101|2000|1103|0");
        decideStops(O, dn, 11, 0.5, 2); CHECK(O.v[0].end == 'B' && O.v[0].endKey == 103, "an offer that stops following a selloff: B?");
    }
    // 8. PULLED / FILLED on the chart side: the order's own end wins when it is in the same candle or before the B? candle
    {   std::vector<TBar> b = barsUp(12, 1000, 3);
        TrailSet T; T.line("TS|7|0|40|100|1000|990"); T.line("TP|7|100|1000|994|0"); T.line("TP|7|101|2000|997|0"); T.line("TP|7|102|3000|1000|0"); T.line("TX|7|104|4000|1000");
        decideStops(T, b, 10, 0.5, 2); CHECK(T.v[0].end == 'X', "pulled in the B? candle: the pull wins");
        TrailSet F; F.line("TS|7|0|40|100|1000|990"); F.line("TP|7|100|1000|994|0"); F.line("TP|7|101|2000|997|0"); F.line("TP|7|102|3000|1000|0"); F.line("TF|7|106|9000|1000|40");
        decideStops(F, b, 10, 0.5, 2); CHECK(F.v[0].end == 'B' && F.v[0].endKey == 104, "B? at 4 comes before the fill at 6: B? wins (ends once)");
        CHECK(!finalEnd(T.v[0], b, 2), "an own end is not final until the candles before it are judged");
        TrailSet D; D.line("TS|7|0|40|100|1000|990"); D.line("TP|7|100|1000|994|0"); D.line("TF|7|100|1500|994|3"); D.line("TP|7|101|2000|997|0");
        CHECK(D.v[0].end == 'F' && D.v[0].steps.size() == 1, "steps after the end are ignored");
    }
    // 9. RESTART: RS ends the open trails as lost (R), never B?
    {   std::vector<TBar> b = barsUp(12, 1000, 3);
        TrailSet T; T.line("RS|0|0.25"); T.line("TS|7|0|40|100|1000|990"); T.line("TP|7|100|1000|994|0"); T.line("TP|7|101|2000|997|0"); T.line("RS|105|0.25");
        decideStops(T, b, 11, 0.5, 2); CHECK(T.v[0].end == 'R' && T.restarts == 2, "a feed restart: lost, not judged");
        CHECK(T.line("TS|7|0|40|100|1000|990") && T.v.size() == 2, "a new start after a restart is a new trail");
        CHECK(!T.line("TP|x|1|2|3|0") && !T.line("ZZ|1") && !T.line("TS|1|5|40|1|1|1") && T.bad == 3, "malformed lines rejected and counted");
        TrailSet D; D.line("TS|7|0|40|100|1000|990"); D.line("TS|7|0|40|100|1000|990"); D.line("TP|7|100|1000|994|0"); D.line("TP|7|100|1000|994|0");
        CHECK(D.v.size() == 1 && D.v[0].steps.size() == 1, "duplicate lines (2 charts) ignored");
    }
    // 10. RECORD round trip + NO REPAINT: a decided end is recorded; re-decided with other settings after a restart it stays the same
    {   std::vector<TBar> b = barsUp(12, 1000, 3);
        TrailSet T; T.line("TS|7|0|40|100|1000|990"); T.line("TP|7|100|1000|994|0"); T.line("TP|7|101|2000|997|0"); T.line("TP|7|102|3000|1000|0"); T.line("TP|7|108|9000|1020|0");
        decideStops(T, b, 6, 0.5, 2);
        const std::string rec = trLineOf(T.v[0], 0.25);
        long long id = 0, k0 = 0; TrEnd E; CHECK(parseTR(rec, id, k0, E) && id == 7 && k0 == 100 && E.end == 'B' && E.endKey == 104 && E.n == 2, "TR line round trip");
        CHECK(T.v[0].moves() == 3 && T.v[0].lastPx() == 1000, "steps after the B? candle are cut (the hover / record count only what came before)");
        // a restart: the file is read again, the record applied first, then decide with N = 5 -> unchanged
        TrailSet R2; R2.line("TS|7|0|40|100|1000|990"); R2.line("TP|7|100|1000|994|0"); R2.line("TP|7|101|2000|997|0"); R2.line("TP|7|102|3000|1000|0"); R2.line("TP|7|108|9000|1020|0");
        Trail& Q = R2.v[0]; Q.end = E.end; Q.endKey = E.endKey; Q.fromRec = true; Q.cutAfterEnd();
        decideStops(R2, b, 11, 0.5, 5); CHECK(Q.end == 'B' && Q.endKey == 104 && Q.moves() == 3, "recorded end never changes (no repaint)");
        CHECK(!parseTR("TR|1|7|0|40", id, k0, E) && !parseTR(rec.substr(0, rec.size() - 2) + "|Q", id, k0, E), "malformed TR rejected");
        // bar-by-bar: deciding after every candle = deciding once at the end
        TrailSet A, Bq; for (TrailSet* S : {&A, &Bq}) { S->line("TS|7|0|40|100|1000|990"); S->line("TP|7|100|1000|994|0"); S->line("TP|7|101|2000|997|0"); S->line("TP|7|102|3000|1000|0"); }
        for (int i = 0; i < 12; ++i) decideStops(A, b, i, 0.5, 2);
        decideStops(Bq, b, 11, 0.5, 2); CHECK(A.v[0].end == Bq.v[0].end && A.v[0].endKey == Bq.v[0].endKey, "candle-by-candle = one pass");
    }
    // 11. at most 2 per side on screen, the latest started
    {   TrailSet T; for (int k = 0; k < 4; ++k) { T.line("TS|" + std::to_string(k) + "|0|40|" + std::to_string(100 + k) + "|1|1"); T.line("TS|" + std::to_string(10 + k) + "|1|40|" + std::to_string(100 + k) + "|1|1"); }
        const std::vector<size_t> s = shownTrails(T, 100, 120); int b = 0, o = 0; for (size_t i : s) { (T.v[i].side == BIDS ? b : o)++; }
        CHECK(b == 2 && o == 2 && T.v[s[0]].key0 == 102, "2 bids + 2 offers, the latest");
    }
    // 12. hover rows
    {   TrailSet T; T.line("TS|7|0|85|100|1000|990"); T.line("TP|7|100|1000|994|0"); T.line("TP|7|101|381000|1000|0");
        std::vector<TBar> b = barsUp(12, 1000, 3); decideStops(T, b, 11, 0.5, 2);
        const std::vector<std::string> L = trailHover(T.v[0], "21,300.00", "21,312.00", "21,312.00", "9:30 AM", "9:41 AM");
        CHECK(L.size() <= 12 && L[0].find("Trailing BID 85 lots") == 0 && L[0].find("stepped 2x") != std::string::npos && L[0].find("6m 20s") != std::string::npos, "hover row 1: size, moves, ticks, time");
        bool q = false, use = false, ex = false, unp = false, endB = false; size_t mx = 0;
        for (auto& r : L) { q |= r.find("Question:") == 0; use |= r.find("buyers quitting - watch for the turn") != std::string::npos; ex |= r.find("Example:") == 0; unp |= r.find("? = not proven yet") == 0; endB |= r.find("End: B? stopped following") == 0; mx = std::max(mx, r.size()); }
        CHECK(q && use && ex && unp && endB && mx <= 125, "hover: question / use / example / not proven / the end reason, rows <= 125 chars");
    }
    std::printf("MoveLogic trails: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
