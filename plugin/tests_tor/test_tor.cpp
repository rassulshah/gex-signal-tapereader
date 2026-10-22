// test_tor.cpp -- TurnOrderLogic.h 1.0.0 (lsTurnOrder): order selection, FILLED vs TOO THICK, the 0.5 ATR confirmation gate, one per pivot,
// no repaint (restart = same marks, no new lines), the record round trip / strict parse, the later P line, the feed-watch rule, the hover.
#include "../TurnOrderLogic.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace tor;
static const long long T0 = 1791500000000LL / 180000 * 180000, BAR = 180000;
static Bar mk(int k, Tick o, Tick h, Tick l, Tick c) { Bar b; b.startMs = T0 + k * BAR; b.endMs = b.startMs + BAR; b.o = o; b.h = h; b.l = l; b.c = c; return b; }
// 20 quiet candles (range 8 ticks around 4000 -> ATR 8), a selloff to the low 3980 on candle 20, candles 21 / 22 hold above it
struct Sc { std::vector<Bar> bars; };
static Sc scenario(Tick c21, Tick c22) {
    Sc s; for (int k = 0; k < 18; ++k) s.bars.push_back(mk(k, 4000, 4004, 3996, 4000));
    s.bars.push_back(mk(18, 4000, 4002, 3992, 3994)); s.bars.push_back(mk(19, 3994, 3995, 3986, 3987));
    s.bars.push_back(mk(20, 3987, 3988, 3980, 3983));              // the low
    s.bars.push_back(mk(21, 3983, c21 + 1, 3982, c21)); s.bars.push_back(mk(22, c21, c22 + 1, 3982, c22));
    return s;
}
// the book: A 50 @3980 (at the low), B 80 @3979 (1 tick beyond), C 200 @3975 (too far), D 300 @3980 arrived 10 s before the extreme,
// E 120 @3981 (inside: price traded through it), all bids. The extreme prints at candle 20 + 100 s.
static const long long TEXT = T0 + 20 * BAR + 100000;
static void book(Engine& E, bool withB = true, long long fillA = 0) {
    E.onEvent(NEW, ASK, 4010, 5, 999, T0 + 1000);                   // the feed starts early
    E.onEvent(NEW, BID, 3980, 50, 1, T0 + 18 * BAR);
    if (withB) E.onEvent(NEW, BID, 3979, 80, 2, T0 + 18 * BAR + 5000);
    E.onEvent(NEW, BID, 3975, 200, 3, T0 + 18 * BAR);
    E.onEvent(NEW, BID, 3980, 300, 4, TEXT - 10000);
    E.onEvent(NEW, BID, 3981, 120, 5, T0 + 18 * BAR);
    E.onEvent(FILL, ASK, 3981, 120, 5, TEXT - 2000); E.onEvent(DEL, BID, 3981, 0, 5, TEXT - 2000);
    E.onEvent(FILL, ASK, 3980, 300, 4, TEXT); E.onEvent(DEL, BID, 3980, 0, 4, TEXT);   // the late 300 gets eaten at the extreme
    if (fillA > 0) { E.onEvent(FILL, ASK, 3980, fillA, 1, TEXT + 500); E.onEvent(MOD, BID, 3980, 50 - fillA, 1, TEXT + 500); }
}
static void run(Engine& E, const Sc& s, size_t upto = 1000) { for (size_t k = 0; k < s.bars.size() && k < upto; ++k) E.closeBar(s.bars[k]); }
int main() {
    { // selection: the largest real order AT or just BEYOND the low, rested 30 s: B (80 @3979); C too far, D too young, E inside
        Engine E; E.P.minSize = 20; book(E); run(E, scenario(3990, 3992));
        CHECK(E.marks.size() == 1, "one mark at the pivot low");
        if (!E.marks.empty()) { const Mark& m = E.marks[0];
            CHECK(m.oid == 2 && m.px == 3979 && m.side == BID && m.shown == 80, "picked B: 80 lots 1 tick beyond the low (not C 5 ticks away, not the 10 s old 300, not the 120 inside)");
            CHECK(!m.absorbed && m.filled == 0, "B never traded: TOO THICK");
            CHECK(m.pivotEnd == T0 + 21 * BAR && m.tExt == T0 + 20 * BAR + 100000 && m.ext == 3980, "keyed by the extreme candle's END, the extreme's time and price"); }
        CHECK(E.tally.pivots >= 1 && E.rows.size() >= 1 && E.rows[0].compare(0, 2, "M|") == 0, "an M line written");
    }
    { // FILLED: no B, A (50 @3980) took 30 lots at the low -> 60% >= 30%
        Engine E; E.P.minSize = 20; book(E, false, 30); run(E, scenario(3990, 3992));
        CHECK(E.marks.size() == 1 && E.marks[0].oid == 1 && E.marks[0].filled == 30 && E.marks[0].absorbed, "A at the low filled 30 of 50: FILLED (absorbed)");
        CHECK(E.marks.size() == 1 && E.marks[0].shown == 50, "shown = what it showed in the minute before the extreme (50)");
    }
    { // filled below the 30% line -> TOO THICK
        Engine E; E.P.minSize = 20; book(E, false, 10); run(E, scenario(3990, 3992));
        CHECK(E.marks.size() == 1 && E.marks[0].filled == 10 && !E.marks[0].absorbed, "10 of 50 (20%): TOO THICK");
    }
    { // min size: 100 lots -> nothing qualifies (B 80) -> N line, no mark
        Engine E; E.P.minSize = 100; book(E); run(E, scenario(3990, 3992));
        CHECK(E.marks.empty() && E.nos.size() == 1 && E.nos[0].why == "no real order at the extreme", "below the min size: no mark, an N line");
    }
    { // the gate: 0.5 ATR (8 ticks ATR -> 4 ticks from 3979 = a close >= 3983). Closes 3982 / 3982 -> waiting; 3984 on candle 23 -> decided there
        Engine E; E.P.minSize = 20; book(E); Sc s = scenario(3982, 3982); s.bars.push_back(mk(23, 3982, 3985, 3981, 3982)); s.bars.push_back(mk(24, 3982, 3986, 3981, 3984));
        run(E, s, 23);
        CHECK(E.marks.empty() && E.pend.size() == 1, "closes 3 ticks away: NOT drawn yet (pending)");
        run(E, s, 24);
        CHECK(E.marks.empty(), "3982 again: still pending");
        E.closeBar(s.bars[24]);
        CHECK(E.marks.size() == 1 && E.marks[0].decideMs == T0 + 25 * BAR && E.marks[0].run100 >= 50, "the 3984 close (5 ticks = 0.62 ATR) confirms: decided at that candle");
    }
    { // gate setting 1.0 ATR: 3984 is not enough
        Engine E; E.P.minSize = 20; E.P.gateAtr = 1.0; book(E); Sc s = scenario(3982, 3982); s.bars.push_back(mk(23, 3982, 3986, 3981, 3984)); run(E, s);
        CHECK(E.marks.empty() && E.pend.size() == 1, "gate 1.0 ATR: 0.62 ATR away is not a confirmation");
    }
    { // broken before the confirmation: a later candle trades under the low -> never drawn
        Engine E; E.P.minSize = 20; book(E); Sc s = scenario(3982, 3982); s.bars.push_back(mk(23, 3982, 3983, 3978, 3979)); run(E, s);
        CHECK(E.marks.empty() && E.pend.empty() && E.tally.broken == 1, "a new low before the 0.5 ATR: dropped (the order did not make the pivot)");
    }
    { // not confirmed in 20 candles -> N line
        Engine E; E.P.minSize = 20; book(E); Sc s = scenario(3982, 3982); for (int k = 23; k < 45; ++k) s.bars.push_back(mk(k, 3982, 3983, 3981, 3982)); run(E, s);
        CHECK(E.marks.empty() && E.tally.noConfirm == 1 && !E.nos.empty() && E.nos.back().why == "not confirmed in 20 candles", "20 candles without the gate: an N line, no mark");
    }
    { // one per pivot: closing the same candles again changes nothing
        Engine E; E.P.minSize = 20; book(E); const Sc s = scenario(3990, 3992); run(E, s); const size_t r = E.rows.size(); run(E, s);
        CHECK(E.marks.size() == 1 && E.rows.size() == r, "the same candles again: still one mark, no new line");
    }
    { // the feed must have watched the book 30 s before the pivot candle: a feed that started late -> silent (no mark, no line)
        Engine E; E.P.minSize = 20; E.onEvent(NEW, BID, 3979, 80, 2, T0 + 20 * BAR + 20000); run(E, scenario(3990, 3992));
        CHECK(E.marks.empty() && E.nos.empty() && E.tally.noFeed >= 1, "feed started inside the pivot candle: nothing decided");
    }
    { // an order first seen by a MOD (rested before the feed): its age is unknown, counted from first seen; marked "at least"
        Engine E; E.P.minSize = 20; E.onEvent(NEW, ASK, 4010, 5, 999, T0 + 1000); E.onEvent(MOD, BID, 3979, 90, 7, T0 + 19 * BAR);
        E.onEvent(FILL, ASK, 3980, 1, 999999, TEXT); run(E, scenario(3990, 3992));
        CHECK(E.marks.size() == 1 && E.marks[0].oid == 7 && !E.marks[0].startKnown, "a MOD-first order counts (start unknown)");
        const std::vector<Row> h = hoverRows(E.marks[0], 0.25, 45, 90);
        CHECK(h.size() == 9 && h[2].text.find("at least") != std::string::npos, "the hover says 'rested at least'");
    }
    { // re-priced order: a new segment, the rest counts again (moved 20 s before the extreme -> too young)
        Engine E; E.P.minSize = 20; E.onEvent(NEW, ASK, 4010, 5, 999, T0 + 1000); E.onEvent(NEW, BID, 3970, 90, 8, T0 + 10 * BAR); E.onEvent(MOD, BID, 3979, 90, 8, TEXT - 20000);
        E.onEvent(FILL, ASK, 3980, 1, 999999, TEXT); run(E, scenario(3990, 3992));
        CHECK(E.marks.empty(), "re-priced 20 s before the extreme: not a 30-s resting order there");
    }
    { // NO REPAINT: restart after every candle from the record = the straight pass; the restart writes no new line
        Engine A; A.P.minSize = 20; book(A); const Sc s = scenario(3990, 3992); run(A, s);
        std::vector<std::string> rec = A.rows;
        Engine B; B.P.minSize = 20; long long bad = 0; B.restore(rec, &bad); run(B, s);
        CHECK(bad == 0 && B.marks.size() == A.marks.size() && B.rows.empty(), "restored from the record: the same mark, nothing re-decided");
        CHECK(B.marks.size() == 1 && B.marks[0].px == A.marks[0].px && B.marks[0].oid == A.marks[0].oid && B.marks[0].shown == A.marks[0].shown && B.marks[0].absorbed == A.marks[0].absorbed, "identical fields after the restart");
        // the feed restarted too (same events again): still nothing new
        Engine C; C.P.minSize = 20; C.restore(rec); book(C); run(C, s);
        CHECK(C.marks.size() == 1 && C.rows.empty(), "restart with the feed replayed: the decided pivot is never decided again");
    }
    { // the later fate: the marked order is pulled after the decision -> P line; restore keeps it
        Engine E; E.P.minSize = 20; book(E); const Sc s = scenario(3990, 3992); run(E, s);
        E.onEvent(DEL, BID, 3979, 80, 2, T0 + 24 * BAR);
        CHECK(E.marks.size() == 1 && E.marks[0].later == 'P' && E.rows.back().compare(0, 2, "P|") == 0, "pulled later: P line");
        Engine R; R.restore(E.rows);
        CHECK(R.marks.size() == 1 && R.marks[0].later == 'P' && R.marks[0].laterMs == T0 + 24 * BAR, "P line read back");
        const std::vector<Row> h = hoverRows(R.marks[0], 0.25, 45, 0);
        CHECK(h[6].text.find("PULLED") != std::string::npos, "the hover says PULLED later");
    }
    { // record strictness
        Mark m; CHECK(!Engine::parseM("M|1.0.0|B|1|2|3", m), "short M line rejected");
        CHECK(!Engine::parseM("M|1.0.0|B|1791500000000|1791500000000|3980|2|3981|995.25|80|0|60|1|80|62|1791500000000|THICK", m), "a bid ABOVE the low it stopped is rejected");
        CHECK(Engine::parseM("M|1.0.0|B|1791500000000|1791500000000|3980|2|3979|994.75|80|0|60|1|80|62|1791500000000|THICK", m) && m.px == 3979 && m.shown == 80, "a good M line parses");
        CHECK(!Engine::parseM("M|1.0.0|B|1791500000000|1791500000000|3980|2|3979|994.75|80|0|60|1|80|62|1791500000000|MAYBE", m), "unknown result rejected");
        CHECK(!Engine::parseM("M|1.0.0|B|1791500000000|17915x0000000|3980|2|3979|994.75|80|0|60|1|80|62|1791500000000|THICK", m), "garbled number rejected");
        Engine R; long long bad = 0; R.restore({ "M|garbage", "", "# header", "N|1.0.0|A|1791500000000|4000|no real order at the extreme|1791500180000" }, &bad);
        CHECK(bad == 1 && R.marks.empty() && R.done.size() == 1, "garbage counted, header / blank skipped, N key kept (never re-decided)");
    }
    { // the hover: offer at a high, question / what / result / why / how / later / example / ?, title in the offer colour
        Mark m; m.side = ASK; m.px = 30912; m.ext = 30912; m.shown = 240; m.filled = 180; m.absorbed = true; m.restS = 130; m.tExt = (9 * 3600 + 41 * 60) * 1000LL; m.run100 = 60; m.maxRun100 = 140;
        const std::vector<Row> h = hoverRows(m, 0.25, 45, 60);
        CHECK(h[0].text == "Offer 240 lots at 7728.00 stopped the rally - 180 filled, price turned 1.4 ATR ?" && h[0].ink == INK_RED, "title: 'Offer 240 lots at 7728.00 stopped the rally - 180 filled, price turned 1.4 ATR ?'");
        CHECK(h[1].text.compare(0, 9, "Question:") == 0 && h[2].text.compare(0, 5, "What:") == 0 && h[3].text.compare(0, 7, "FILLED:") == 0 && h[4].text.compare(0, 15, "Why it matters:") == 0
              && h[5].text.compare(0, 11, "How to use:") == 0 && h[6].text.find("still resting, 60") != std::string::npos && h[7].text.compare(0, 8, "Example:") == 0 && h[8].text.compare(0, 1, "?") == 0, "rows in order");
        CHECK(h[2].text.find("9:41 AM") != std::string::npos && h[2].text.find("2 min 10 s") != std::string::npos, "12-hour time and the resting time");
        bool shortOk = true; for (auto& r : h) shortOk = shortOk && r.text.size() <= 130; CHECK(shortOk, "rows <= 130 chars");
    }
    { // a high pivot mirrors (offer at / just above the high)
        Engine E; E.P.minSize = 20; E.onEvent(NEW, BID, 3900, 5, 999, T0 + 1000);
        std::vector<Bar> b; for (int k = 0; k < 18; ++k) b.push_back(mk(k, 4000, 4004, 3996, 4000));
        b.push_back(mk(18, 4000, 4008, 3999, 4006)); b.push_back(mk(19, 4006, 4014, 4005, 4013)); b.push_back(mk(20, 4013, 4020, 4012, 4017));
        b.push_back(mk(21, 4017, 4018, 4009, 4010)); b.push_back(mk(22, 4010, 4011, 4006, 4008));
        E.onEvent(NEW, ASK, 4021, 60, 11, T0 + 17 * BAR); E.onEvent(NEW, ASK, 4020, 40, 12, T0 + 17 * BAR);
        E.onEvent(FILL, BID, 4020, 15, 12, TEXT); E.onEvent(MOD, ASK, 4020, 25, 12, TEXT);
        for (auto& x : b) E.closeBar(x);
        CHECK(E.marks.size() == 1 && E.marks[0].side == ASK && E.marks[0].oid == 11 && E.marks[0].px == 4021 && !E.marks[0].absorbed, "high: the 60-lot offer 1 tick above the high (bigger than the 40 at it), untouched = TOO THICK");
    }
    std::printf("test_tor: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
