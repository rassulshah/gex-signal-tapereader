// test_moves.cpp (2026-10-21) - MBO Roadmap TOP DEFENDER #3: MoveLogic.h 1.0.0 (order MOVES: pressing in / stepping back, both sides) and
// its wiring into the shared engine (OrderFlowFeed.h 1.11.0 Engine.trackMoves). Every detection rule of Defender3-phase1-findings.md:
// same-id modify (PATH M), cancel + re-post (PATH R: unique / ambiguous / after a fill), following dropped, ladder shift dropped, short
// rest dropped, the size floor, the zone + window, the 2.0x / 65% badge rule, the M line round trip (no repaint after a restart), the
// combined badge "A|H|L|P", the box text, the price-chart boxes, the normal file.
#include "BidAskStatsLogic.h"
#include "BookFlowLogic.h"
#include "DefenseTurn.h"
#include "MoveLogic.h"
#include <cstdio>
#include <string>
#include <vector>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, m); } } while (0)

static std::vector<mvl::Move> confirmAll(mvl::Tracker& t, long long ms) { std::vector<mvl::Move> v; t.confirm(ms, v); return v; }

int main() {
    // ---- the tracker ---------------------------------------------------------------------------------------------------------
    {   // PATH M: a bid 4 ticks below the trade re-priced 2 ticks up (toward price) = pressing in; confirmed after 0.5 s of rest
        mvl::Tracker t; t.minLots = 6;
        t.modify(1, mvl::BIDS, 996, 998, 10, 1000, 1000, 10000);
        CHECK(confirmAll(t, 10499).empty(), "M: not confirmed before 0.5 s of rest");
        std::vector<mvl::Move> v = confirmAll(t, 10500);
        CHECK(v.size() == 1 && v[0].dir == 1 && v[0].oldPx == 996 && v[0].newPx == 998 && v[0].size == 10 && !v[0].viaR, "M: same id re-priced toward price -> pressing in, after 0.5 s");
        t.modify(2, mvl::OFFERS, 1004, 1007, 8, 1000, 1000, 20000);
        v = confirmAll(t, 20600); CHECK(v.size() == 1 && v[0].dir == -1, "M: an offer moved further above the trade -> stepping back");
        t.modify(3, mvl::BIDS, 996, 998, 10, 1000, 1002, 30000);   // the trade moved up 2 as well: same distance = following
        v = confirmAll(t, 30600); CHECK(v.empty() && t.following == 1, "M: same distance to the trade = following the market: dropped");
        t.modify(4, mvl::BIDS, 996, 998, 5, 1000, 1000, 40000);    // 5 < the ES floor 6
        v = confirmAll(t, 40600); CHECK(v.empty() && t.smallSz == 1, "size under the market's q90 floor: dropped");
        t.modify(5, mvl::BIDS, 996, 998, 6, 1000, 1000, 50000);
        v = confirmAll(t, 50600); CHECK(v.size() == 1, "size AT the floor counts");
    }
    {   // short rest: any later event on the order within 0.5 s drops the move
        mvl::Tracker t; t.minLots = 1;
        t.modify(1, mvl::BIDS, 996, 998, 10, 1000, 1000, 1000); t.touched(1, 1300);
        CHECK(confirmAll(t, 2000).empty() && t.shortRest == 1, "cancelled / filled / moved again 300 ms later: dropped (did not rest)");
        t.modify(2, mvl::BIDS, 996, 998, 10, 1000, 1000, 3000); t.touched(2, 3500);
        CHECK(confirmAll(t, 4000).size() == 1, "an event exactly 0.5 s later: it rested 0.5 s (kept)");
        t.modify(3, mvl::BIDS, 996, 998, 10, 1000, 1000, 5000); t.modify(3, mvl::BIDS, 998, 999, 10, 998, 1000, 5100);
        std::vector<mvl::Move> v = confirmAll(t, 5700);
        CHECK(v.size() == 1 && v[0].oldPx == 998 && v[0].newPx == 999, "moved twice within 0.5 s: only the last move (which rested) counts");
    }
    {   // ladder shift: 5 bids moving 1 tick down in the same 5 ms = a market maker re-quoting the ladder
        mvl::Tracker t; t.minLots = 1;
        for (int k = 0; k < 5; ++k) t.modify(10 + k, mvl::BIDS, 998 - k, 997 - k, 10, 1000, 1000, 7001);
        CHECK(confirmAll(t, 8000).empty() && t.ladder == 5, "5 orders, same tick change, same 5 ms: a ladder shift, all dropped");
        for (int k = 0; k < 4; ++k) t.modify(20 + k, mvl::BIDS, 998 - k, 997 - k, 10, 1000, 1000, 9001);
        CHECK(confirmAll(t, 10000).size() == 4, "4 orders: not a ladder (kept)");
    }
    {   // PATH R: a cancel + a same-size NEW at another price within 250 ms, unique
        mvl::Tracker t; t.minLots = 1;
        t.cancel(1, mvl::BIDS, 996, 20, 1000, 1000);
        t.added(2, mvl::BIDS, 998, 20, 1000, 1100);
        std::vector<mvl::Move> v = confirmAll(t, 1700);
        CHECK(v.size() == 1 && v[0].viaR && v[0].dir == 1 && v[0].oldPx == 996 && v[0].newPx == 998, "R: unique cancel + re-post 2 ticks closer -> pressing in");
        t.cancel(3, mvl::BIDS, 996, 20, 1000, 2000);
        t.added(4, mvl::BIDS, 998, 20, 1000, 2300);
        CHECK(confirmAll(t, 3000).empty(), "R: 300 ms apart (> 250 ms): no pair");
        t.cancel(5, mvl::BIDS, 996, 20, 1000, 4000); t.cancel(6, mvl::BIDS, 995, 20, 1000, 4010);
        t.added(7, mvl::BIDS, 998, 20, 1000, 4100);
        CHECK(confirmAll(t, 5000).empty() && t.ambiguous >= 1, "R: a new order matching 2 cancels = ambiguous, none");
        t.cancel(8, mvl::BIDS, 996, 20, 1000, 6000);
        t.added(9, mvl::BIDS, 998, 20, 1000, 6050); t.added(10, mvl::BIDS, 997, 20, 1000, 6060);
        CHECK(confirmAll(t, 7000).empty(), "R: one cancel matched by 2 new orders = ambiguous, none");
        t.cancel(11, mvl::BIDS, 996, 20, 1000, 8000); t.added(12, mvl::BIDS, 996, 20, 1000, 8050);
        CHECK(confirmAll(t, 9000).empty(), "R: re-posted at the SAME price is not a move");
        t.cancel(13, mvl::BIDS, 996, 100, 1000, 10000); t.added(14, mvl::BIDS, 998, 108, 1000, 10050);
        CHECK(confirmAll(t, 11000).size() == 1, "R: 100 vs 108 lots (within 10% at 10+ lots) matches");
        t.cancel(15, mvl::BIDS, 996, 100, 1000, 12000); t.added(16, mvl::BIDS, 998, 115, 1000, 12050);
        CHECK(confirmAll(t, 13000).empty(), "R: 100 vs 115 lots (15%) does not match");
        t.cancel(17, mvl::BIDS, 996, 5, 1000, 14000); t.added(18, mvl::BIDS, 998, 6, 1000, 14050);
        CHECK(confirmAll(t, 15000).empty(), "R: under 10 lots the sizes must be equal");
        t.cancel(19, mvl::OFFERS, 1004, 20, 1000, 16000); t.added(20, mvl::BIDS, 998, 20, 1000, 16050);
        CHECK(confirmAll(t, 17000).empty(), "R: the other side never pairs");
    }
    // ---- the read at an extreme ------------------------------------------------------------------------------------------------
    {
        std::vector<mvl::Move> mv;
        auto M = [](int side, long long o, long long n, long long sz, int dir) { mvl::Move m; m.side = side; m.oldPx = o; m.newPx = n; m.size = sz; m.dir = dir; return m; };
        mv.push_back(M(mvl::BIDS, 996, 998, 30, 1)); mv.push_back(M(mvl::BIDS, 995, 997, 20, 1)); mv.push_back(M(mvl::BIDS, 999, 997, 10, -1));
        mv.push_back(M(mvl::BIDS, 900, 902, 500, 1));   // far from the extreme: outside the zone
        mv.push_back(M(mvl::OFFERS, 1003, 1001, 12, 1));
        const long long zone = mvl::zoneOf(8.0);        // 0.5 ATR = 4 ticks
        CHECK(zone == 4 && mvl::zoneOf(1.0) == 2 && mvl::zoneOf(NAN) == 2, "zone = max(2 ticks, 0.5 ATR)");
        mvl::Sum s = mvl::sumIn(mv, 997, zone);
        CHECK(s.in[0] == 50 && s.out[0] == 10 && s.in[1] == 12 && s.ticksIn[0] == 2 && s.bigSize[0] == 30 && s.bigOld[0] == 996 && s.bigNew[0] == 998, "the zone keeps the moves near the extreme (the far 500 lots out; the offer 1003 -> 1001 ends 4 ticks away: in)");
        const double norm[2] = { 20.0, 10.0 };
        mvl::Read r = mvl::readAt(100, 0, 997, s, norm);
        CHECK(r.q[mvl::C_DEF_IN] && !r.q[mvl::C_DEF_OUT] && std::fabs(r.x[mvl::C_DEF_IN] - 2.5) < 1e-9 && mvl::drawP(r), "support: bids moved in 50 lots = 2.5x the normal 20 and 83% one way -> P");
        const double low[2] = { 30.0, 10.0 }; CHECK(!mvl::readAt(100, 0, 997, s, low).q[mvl::C_DEF_IN], "1.67x the normal: under 2.0x -> no P");
        mvl::Sum s2 = s; s2.out[0] = 40; CHECK(!mvl::readAt(100, 0, 997, s2, norm).q[mvl::C_DEF_IN], "in 50 vs out 40 (56% one way < 65%): no P");
        const double none[2] = { 0, 0 }; mvl::Read rl = mvl::readAt(100, 0, 997, s, none); CHECK(!rl.q[0] && rl.learning, "no normal yet: learning, nothing qualifies");
        // resistance: offers are the defenders, bids the attackers
        mvl::Sum h; h.in[1] = 40; h.out[1] = 0; h.in[0] = 0; h.out[0] = 30;
        const double n10[2] = { 10.0, 10.0 }; mvl::Read rh = mvl::readAt(200, 1, 1003, h, n10);
        CHECK(rh.q[mvl::C_DEF_IN] && rh.q[mvl::C_ATT_OUT] && !rh.q[mvl::C_ATT_IN], "resistance: offers pressed in (defenders, P) + bids stepped back (attackers, recorded)");
        CHECK(mvl::movedText(rh, mvl::OFFERS) == "In 0t 4.0x" && mvl::movedText(rh, mvl::BIDS) == "Out 0t 3.0x", "box text per side: offers 'In ... 4.0x' (40 / 10), bids 'Out ... 3.0x' (30 / 10)");
        mvl::Read rt = r; rt.tick = 0.25;
        const std::string line = mvl::mLineOf(rt); mvl::Read back;
        CHECK(mvl::parseM(line, back) && back.t == 100 && back.ext == 0 && back.s.in[0] == 50 && back.q[mvl::C_DEF_IN] && std::fabs(back.tick - 0.25) < 1e-12 && back.s.bigOld[0] == 996 && back.s.bigNew[0] == 998 && back.s.bigSize[0] == 30, "the M line round trip (a restart redraws the same: no repaint)");
        CHECK(mvl::movedText(back, mvl::BIDS) == "In 2t 2.5x" && mvl::movedText(back, mvl::OFFERS) == "-", "box row Moved: bids 'In 2t 2.5x', offers '-'");
        CHECK(mvl::chartBoxes(back), "a P candle draws the price-chart boxes (largest move in: 30 lots 996 -> 998)");
        CHECK(!mvl::parseM("M|1|0|997|x|0|0|0|0|0|P|0|0|0|0|0|0|0|0.25", back) && !mvl::parseM("M|1|0|997|1|0|0|0|0|0|Z|0|0|0|0|0|0|0|0.25", back) && !mvl::parseM(line + "|9", back), "garbled M lines are rejected");
        mvl::Read nb = back; nb.q[mvl::C_DEF_IN] = false; nb.q[mvl::C_DEF_OUT] = true; CHECK(!mvl::drawP(nb) && !mvl::chartBoxes(nb), "B (stepping back) is recorded but not drawn");
    }
    {   // the normal: 30-min slot mean after 5 candles, else the all-slot mean after 20; saved / read back
        mvl::Norm n; const long long a[2] = { 10, 4 };
        for (int k = 0; k < 4; ++k) n.add(20, a);
        CHECK(n.of(0, 20) == 0.0, "under 5 candles in the slot and under 20 overall: no normal (learning)");
        n.add(20, a); CHECK(std::fabs(n.of(0, 20) - 10.0) < 1e-9 && std::fabs(n.of(1, 20) - 4.0) < 1e-9, "5 candles in the slot: its mean");
        for (int k = 0; k < 15; ++k) n.add(21, a);
        CHECK(std::fabs(n.of(0, 30) - 10.0) < 1e-9, "an empty slot: the all-slot mean once 20 candles were seen");
        mvl::Norm z; CHECK(z.parse(n.text()) && std::fabs(z.of(0, 20) - 10.0) < 1e-9 && z.n[0] == 20, "the normal file round trip");
        CHECK(!z.parse("MN|1|9:1:1:1") && !z.parse("garbage"), "a garbled normal file is rejected");
        CHECK(mvl::slotOf(8 * 60 + 45) == 17 && mvl::minLotsOf("ES") == 6 && mvl::minLotsOf("GC") == 2 && mvl::minLotsOf("EU") == 8 && mvl::minLotsOf("NQ") == 1, "slots and the per-market size floors (phase 1 q90)");
    }
    // ---- the badge ---------------------------------------------------------------------------------------------------------------
    CHECK(dt::badgeText5(true, true, 'L', true, false) == "A|H|L|P" && dt::badgeText5(false, false, 0, true, false) == "P" && dt::badgeText5(true, false, 0, false, false) == "A", "the combined badge A|H|L|P");
    CHECK(std::string(dt::VERSION) == "1.8.0" && std::string(mvl::VERSION) == "1.2.0" && std::string(off::VERSION).compare(0, 2, "1.") == 0 && std::string(off::VERSION) >= "1.13.", "versions");   // (MoveLogic 1.2.0) off: 1.13.x or later (1.14.0 implied fills) (other plugins bump it)
    // ---- (1.1.0 / DefenseTurn 1.6.0, Rassul 14:16 "i want you to draw them") all four cases drawn ----------------------------------
    {   mvl::Read r; r.ext = 0; r.t = 100; r.px = 4000; r.tick = 0.25; r.norm[0] = 5; r.norm[1] = 5;
        r.s.in[0] = 12; r.s.bigSize[0] = 12; r.s.bigOld[0] = 3990; r.s.bigNew[0] = 3992;                 // bids pressed in
        r.s.out[1] = 15; r.s.outSize[1] = 15; r.s.outOld[1] = 4004; r.s.outNew[1] = 4008;              // offers stepped back (up, away)
        r.q[mvl::C_DEF_IN] = true; r.q[mvl::C_ATT_OUT] = true;
        CHECK(mvl::letterOf(r, mvl::BIDS) == 'P' && mvl::letterOf(r, mvl::OFFERS) == 'B' && mvl::anyDrawn(r), "letters per book side: bids P (pressed in), offers B (stepped back)");
        long long o = 0, n = 0, z = 0;
        CHECK(mvl::chartBoxOf(r, mvl::BIDS, o, n, z) && o == 3990 && n == 3992 && z == 12, "the bids box: the largest move IN");
        CHECK(mvl::chartBoxOf(r, mvl::OFFERS, o, n, z) && o == 4004 && n == 4008 && z == 15, "the offers box: the largest move OUT");
        mvl::Read q; CHECK(mvl::parseM(mvl::mLineOf(r), q) && q.s.outSize[1] == 15 && q.s.outNew[1] == 4008 && q.s.bigSize[0] == 12 && mvl::letterOf(q, mvl::OFFERS) == 'B', "the 31-field M line round trip keeps both sides' moves");
        std::string old = mvl::mLineOf(r); for (int k = 0; k < 12; ++k) old = old.substr(0, old.rfind('|'));
        CHECK(mvl::parseM(old, q) && q.s.outSize[1] == 0 && mvl::letterOf(q, mvl::BIDS) == 'P', "a 1.0.0 line (19 fields) still reads");
        CHECK(!mvl::parseM(old + "|1|2", q), "a line with a wrong field count is rejected");
        mvl::Read e; e.ext = 1; CHECK(!mvl::anyDrawn(e) && mvl::letterOf(e, mvl::BIDS) == 0 && !mvl::chartBoxOf(e, mvl::BIDS, o, n, z), "nothing qualified: no letter, no box");
        r.q[mvl::C_ATT_OUT] = false; r.q[mvl::C_ATT_IN] = true;   // offers flagged "pressed in", but no offer lots moved in
        CHECK(mvl::letterOf(r, mvl::OFFERS) == 0 && !mvl::chartBoxOf(r, mvl::OFFERS, o, n, z) && mvl::movedText(r, mvl::OFFERS) == "-", "(1.1.1) a flag with no lots moved that way: no letter, no box, Moved '-' (was 'In 0t 0.0x')");
        r.s.in[1] = 7; CHECK(mvl::letterOf(r, mvl::OFFERS) == 'P', "(1.1.1) the same flag with 7 offer lots moved in: P");
    }
    {   std::vector<dt::BadgeSeg> v = dt::badgeSegs(true, true, 'L', dt::LOW, 'P', 'B');
        CHECK(dt::segsText(v) == "A|H|L|P|B" && v.size() == 3 && v[0].colourKey == dt::LOW && v[1].s == "P|" && v[1].colourKey == dt::LOW && v[2].s == "B" && v[2].colourKey == dt::HIGH, "badge parts: A|H|L| defending colour, P| bids green, B offers red");
        v = dt::badgeSegs(false, false, 0, dt::HIGH, 0, 'P');
        CHECK(dt::segsText(v) == "P" && v.size() == 1 && v[0].colourKey == dt::HIGH, "offers alone: P red");
        v = dt::badgeSegs(false, false, 'X', dt::LOW, 0, 0);
        CHECK(dt::segsText(v) == "X" && v[0].colourKey == dt::HIGH, "X alone: the opposite colour (as 1.4.0)");
        CHECK(dt::badgeSegs(false, false, 0, dt::LOW, 0, 0).empty(), "nothing: no badge");
        v = dt::badgeSegs(true, false, 0, dt::HIGH, 'B', 0);
        CHECK(dt::segsText(v) == "A|B" && v[0].colourKey == dt::HIGH && v[1].colourKey == dt::LOW, "A at a high (red) + bids stepped back (green)");
    }
    // ---- the shared engine (OrderFlowFeed.h 1.11.0) ---------------------------------------------------------------------------------
    {
        off::Engine g; g.near = 40; g.trackMoves = true; g.mvt.minLots = 6;
        auto ev = [](int act, int side, long long px, long long size, long long id, long long ms) { off::Ev e; e.action = act; e.side = side; e.px = px; e.size = size; e.id = id; e.arrMs = ms; e.stamp = ms / 1000; return e; };
        off::Acc a1, a2;
        g.apply(a1, ev(off::NEW, off::ASK, 1000, 5, 900, 1000)); g.apply(a1, ev(off::FILL, off::ASK, 1000, 1, 900, 1001));   // a trade at 1000 (the reference)
        g.apply(a1, ev(off::NEW, off::BID, 996, 10, 1, 1100));                     // a bid 4 below the trade
        g.apply(a1, ev(off::MOD, off::BID, 998, 10, 1, 2000));                     // re-priced 2 closer (same id)
        CHECK(g.mvt.seenM == 1 && g.mvt.pending() == 1, "engine: the same-id re-price reaches the tracker (PATH M)");
        CHECK(a1.pullC[0] == 10 && a1.addC[0] == 20, "engine: PullStats / StackStats numbers unchanged (the move still books as a pull + an add)");
        g.apply(a2, ev(off::NEW, off::ASK, 1010, 2, 901, 2600));                   // any later event: the rest is done -> into the candle open now
        CHECK(a2.moves.size() == 1 && a2.moves[0].dir == 1 && a2.moves[0].size == 10 && a1.moves.empty(), "engine: confirmed after 0.5 s, booked to the candle open at confirmation (no look-ahead)");
        g.apply(a2, ev(off::NEW, off::BID, 990, 20, 2, 3000));
        g.apply(a2, ev(off::DEL, off::BID, 990, 20, 2, 4000));                     // a real cancel (no fill before it)
        g.apply(a2, ev(off::NEW, off::BID, 995, 20, 3, 4100));                     // re-posted 5 closer, 100 ms later
        g.apply(a2, ev(off::NEW, off::ASK, 1011, 2, 902, 4700));
        CHECK(a2.moves.size() == 2 && a2.moves[1].viaR && a2.moves[1].oldPx == 990 && a2.moves[1].newPx == 995, "engine: cancel + unique re-post (PATH R)");
        g.apply(a2, ev(off::NEW, off::BID, 991, 20, 4, 5000));
        g.apply(a2, ev(off::FILL, off::BID, 991, 20, 4, 5100)); g.apply(a2, ev(off::DEL, off::BID, 991, 20, 4, 5100));   // filled, then removed: not a cancel
        g.apply(a2, ev(off::NEW, off::BID, 993, 20, 5, 5200)); g.apply(a2, ev(off::NEW, off::ASK, 1012, 2, 903, 5900));
        CHECK(a2.moves.size() == 2, "engine: a DEL right after a fill is the trade, never paired as a move");
        off::Engine q; q.near = 40; q.apply(a1, ev(off::NEW, off::BID, 996, 10, 1, 1100)); q.apply(a1, ev(off::MOD, off::BID, 998, 10, 1, 2000));
        CHECK(q.mvt.seenM == 0, "engine: trackMoves off (every other pane) = no move work at all");
    }
    {   // (MoveLogic 1.2.0 TRAILS) the engine wiring: candle key, same-id steps, the fill hook; trails off unless trailMin is set
        off::Engine g; g.near = 40; g.trackMoves = true; g.mvt.minLots = 6; g.mvt.trailMin = 25;
        auto ev = [](int act, int side, long long px, long long size, long long id, long long ms) { off::Ev e; e.action = act; e.side = side; e.px = px; e.size = size; e.id = id; e.arrMs = ms; e.stamp = ms / 1000; return e; };
        off::Acc a;
        g.curKey = 500; g.apply(a, ev(off::NEW, off::ASK, 1000, 5, 900, 1000)); g.apply(a, ev(off::FILL, off::ASK, 1000, 1, 900, 1001));
        g.apply(a, ev(off::NEW, off::BID, 996, 40, 1, 1100));
        g.apply(a, ev(off::NEW, off::ASK, 1002, 5, 910, 1500)); g.apply(a, ev(off::FILL, off::ASK, 1002, 1, 910, 1501));   // price runs up 2 first
        g.curKey = 501; g.apply(a, ev(off::MOD, off::BID, 998, 40, 1, 2000));
        g.apply(a, ev(off::NEW, off::ASK, 1004, 5, 911, 2500)); g.apply(a, ev(off::FILL, off::ASK, 1004, 1, 911, 2501));
        g.curKey = 502; g.apply(a, ev(off::MOD, off::BID, 1000, 40, 1, 3000));
        g.apply(a, ev(off::NEW, off::ASK, 1010, 2, 901, 3100));                   // a later event closes the 5 ms ladder window
        CHECK(g.mvt.trStarted == 1 && !g.mvt.trailOut.empty() && g.mvt.trailOut[0] == "TS|1|0|40|501|2000|996", "engine: a 40-lot bid stepping up twice after price ran = a trail (key = the candle of its first step)");
        CHECK(g.mvt.trailOut.size() >= 3 && g.mvt.trailOut[1] == "TP|1|501|2000|998|0|4|2", "engine: the step carries ticks behind the trade (4) and the run before it (2); the near band reaches the tracker");
        g.curKey = 503; g.apply(a, ev(off::FILL, off::BID, 1000, 7, 1, 4000));
        CHECK(g.mvt.trFilled == 1 && g.mvt.trailOut.back() == "TF|1|503|4000|1000|7", "engine: a fill on it = TF (the dot)");
        off::Engine q; q.near = 40; q.trackMoves = true; q.mvt.minLots = 6; off::Acc b;
        q.apply(b, ev(off::NEW, off::ASK, 1000, 5, 900, 1000)); q.apply(b, ev(off::FILL, off::ASK, 1000, 1, 900, 1001));
        q.apply(b, ev(off::NEW, off::BID, 996, 40, 1, 1100)); q.apply(b, ev(off::FILL, off::ASK, 1002, 1, 900, 1501)); q.apply(b, ev(off::MOD, off::BID, 998, 40, 1, 2000)); q.apply(b, ev(off::FILL, off::ASK, 1004, 1, 900, 2501)); q.apply(b, ev(off::MOD, off::BID, 1000, 40, 1, 3000)); q.apply(b, ev(off::NEW, off::ASK, 1010, 2, 901, 3100));
        CHECK(q.mvt.trailOut.empty() && q.mvt.trStarted == 0, "engine: trailMin 0 = no trail work");
    }
    std::printf("Moves (Top Defender #3): %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
