// DefenseTurn.h 1.0.0 - the pivot turn rule, the refill line price, E (attack died), A (absorption), the file lines, the line's life
#include "DefenseTurn.h"
#include <cstdio>
#include <set>
#include <utility>
static int f = 0, p = 0;
#define CHECK(c, m) do { if (c) ++p; else { ++f; printf("FAIL %s\n", m); } } while (0)
static dt::Candle C(long long t, long long o, long long h, long long l, long long c, long long nb, long long na) {
    dt::Candle x; x.t = t; x.ok = true; x.o = o; x.h = h; x.l = l; x.c = c; x.net[0] = nb; x.net[1] = na; return x;
}
int main() {
    std::set<std::pair<int, long long> > used;
    auto U = [&](int s, long long t) { return used.count(std::make_pair(s, t)) > 0; };
    // a low: 100 -> 96 (pivot candle 2 makes the new low and closes green, bids bigger)
    std::vector<dt::Candle> c = { C(1, 110, 111, 104, 105, 50, 60), C(2, 105, 106, 100, 101, 40, 70), C(3, 101, 103, 96, 102, 90, 60) };
    dt::Turn t;
    CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && t.side == dt::LOW && t.pivotT == 3 && t.pivotPx == 96 && t.linePx == 96 && t.marginPct == 50, "pivot low, green close, bids 90 vs 60 = +50%, line at the low");
    CHECK(!dt::decide(c, 2, dt::HIGH, false, U, t), "not a high");
    c[2].c = 99; CHECK(!dt::decide(c, 2, dt::LOW, false, U, t), "red close: no turn");
    CHECK(dt::decide(c, 2, dt::LOW, true, U, t) && t.live, "forming candle: the ? needs no close");
    c[2].c = 102; c[2].net[0] = 50; CHECK(!dt::decide(c, 2, dt::LOW, false, U, t), "bids smaller: no turn");
    c[2].net[0] = 90;
    // the next candle trading back into the pivot candle
    std::vector<dt::Candle> d = { C(1, 110, 111, 104, 105, 50, 60), C(2, 105, 106, 96, 99, 40, 70), C(3, 99, 101, 97, 100, 80, 20) };
    CHECK(dt::decide(d, 2, dt::LOW, false, U, t) && t.pivotT == 2 && t.t == 3 && t.pivotPx == 96 && t.marginPct == 300, "next candle: pivot = the candle before, +300%");
    used.insert(std::make_pair(dt::LOW, 2LL)); CHECK(!dt::decide(d, 2, dt::LOW, false, U, t), "one turn per pivot");
    used.clear();
    d[2].h = 95; d[2].l = 94; CHECK(!dt::decide(d, 2, dt::LOW, false, U, t) || t.pivotT == 3, "a lower low is its own pivot candle");
    // refill price within 8 ticks of the pivot: the most re-posted lots wins
    c[1].refPx[0] = 100; c[1].refLots[0] = 900;           // the candle before the pivot: not in pivot..turn
    c[2].refPx[0] = 98; c[2].refLots[0] = 300; CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && t.linePx == 98, "line at the refill price 98");
    c[2].refPx[0] = 120; CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && t.linePx == 96, "a refill 24 ticks away is ignored: the extreme");
    // (1.2.0) the close must be beyond the R line: above it at a low (green close 102 under a refill line at 103 = no turn)
    c[2].refPx[0] = 103; c[2].refLots[0] = 300; c[2].h = 104;
    CHECK(!dt::decide(c, 2, dt::LOW, false, U, t), "(1.2.0) green close 102 below the R line 103: no turn");
    c[2].c = 103; CHECK(!dt::decide(c, 2, dt::LOW, false, U, t), "(1.2.0) close ON the line: no turn");
    c[2].c = 104; CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && t.linePx == 103, "(1.2.0) close 104 above the line 103: turn");
    c[2].c = 102; CHECK(!dt::decide(c, 2, dt::LOW, true, U, t), "(1.2.0) forming: price back under the line = no '?'");
    c[2].c = 102; c[2].h = 103; c[2].refPx[0] = 0;
    // E: the attack died - 600 lots in 60 s before the low (10/s), 40 lots in 60 s after (0.67/s)
    c[2].refPx[0] = 0; c[2].before[dt::LOW] = 600; c[2].secsBefore[dt::LOW] = 60; c[2].after[dt::LOW] = 40; c[2].secsAfter[dt::LOW] = 60;
    CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && t.E, "pace after the low 0.67/s <= half of 10/s: E");
    c[2].after[dt::LOW] = 400; CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && !t.E, "6.7/s after: not E");
    c[2].after[dt::LOW] = 10; c[2].secsAfter[dt::LOW] = 5; CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && !t.E, "only 5 s after the low: not E");
    // E across the next candle
    d[2] = C(3, 99, 101, 97, 100, 80, 20); d[1].before[dt::LOW] = 600; d[1].secsBefore[dt::LOW] = 100; d[1].after[dt::LOW] = 20; d[1].secsAfter[dt::LOW] = 20; d[2].hit[0] = 100; d[2].span = 170;
    CHECK(dt::decide(d, 2, dt::LOW, false, U, t) && t.E, "120 lots in 190 s after the low vs 6/s before: E");
    d[2].hit[0] = 900; CHECK(dt::decide(d, 2, dt::LOW, false, U, t) && !t.E, "920 lots in 190 s: not E");
    // A: absorption = sellers out-hit buyers on the turn candle (at a low)
    c[2].aggr[0] = 700; c[2].aggr[1] = 500; CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && t.A, "sellers 700 > buyers 500, green close: A");
    c[2].aggr[0] = 300; CHECK(dt::decide(c, 2, dt::LOW, false, U, t) && !t.A, "buyers bigger: not A");
    // a high
    std::vector<dt::Candle> h = { C(1, 100, 104, 99, 103, 50, 40), C(2, 103, 108, 102, 107, 60, 50), C(3, 107, 110, 105, 106, 30, 80) };
    h[2].aggr[1] = 900; h[2].aggr[0] = 400;
    CHECK(dt::decide(h, 2, dt::HIGH, false, U, t) && t.pivotPx == 110 && t.A && t.marginPct == 167, "pivot high, red close, offers 80 vs 30 (+167%), buyers out-hit: A");
    CHECK(dt::tags(t) == "R A ?", "tags R A ?");
    h[2].net[0] = -20; CHECK(dt::decide(h, 2, dt::HIGH, false, U, t) && dt::marginText(t.marginPct) == "999%", "attacking side pulled: 999% (no + sign)");
    // file lines
    dt::Turn q; q.t = 3875279220LL; q.side = dt::HIGH; q.pivotPx = 31376; q.linePx = 31374; q.marginPct = 23; q.E = true; q.defNet = 3847; q.attNet = 3363;
    const std::string ln = dt::lineOf('T', q, 0.25); dt::Line L;
    CHECK(dt::parseLine(ln, L) && L.t == q.t && L.side == dt::HIGH && L.linePx == 7843.5 && L.pivotPx == 7844 && L.margin == 23 && L.E && !L.A && dt::tagsOf(L) == "R ?", "file line round trip (E recorded, not shown)");
    CHECK(!dt::parseLine("T|1|2|3", L) && !dt::parseLine("X|1|0|1|1|0|0|0|0|0|0.25", L) && !dt::parseLine(ln.substr(0, ln.size() - 6), L), "garbage / half lines refused");
    CHECK(dt::parseLine(ln + "\r", L) && !dt::parseLine("T|1||1|1|0|0|0|0|0|1", L) && !dt::parseLine("T|\r1|0|1|1|0|0|0|0|1", L) && !dt::parseLine("T|1|0|inf|1|0|0|0|0|0|1", L) && !dt::parseLine("T|999999999999999999999999|0|1|1|0|0|0|0|0|1", L) && !dt::parseLine("T|1|0|1|1|0|2|0|0|0|1", L), "Windows line ending accepted; malformed T fields refused");
    CHECK(!dt::parseLine("T| 1|0|1|1|0|0|0|0|0|1", L) && !dt::parseLine("T|+1|0|1|1|0|0|0|0|0|1", L) && !dt::parseLine("T|1|0|0x1p0|1|0|0|0|0|0|1", L) && !dt::parseLine("T|1|0|1e-999|1|0|0|0|0|0|1", L), "whitespace, noncanonical, hex, and underflowed T numeric fields refused");
    // line life: 3-min bars, closes after a high line at 100
    std::vector<double> cl = { 99, 99.5, 98, 101, 97 }; std::vector<long long> tm = { 180, 360, 540, 720, 900 };
    dt::Life lf = dt::lifeOf(dt::HIGH, 100, 0, cl, tm, 0, 900); CHECK(lf.bars == 4 && lf.broken, "closed above on the 4th candle: broken there");
    lf = dt::lifeOf(dt::HIGH, 100, 0, std::vector<double>{99, 99, 99, 99, 99, 99, 99}, std::vector<long long>{180, 360, 540, 720, 900, 1080, 1260}, 0, 900);
    CHECK(lf.bars == 5 && !lf.broken, "15 minutes = 5 candles");
    lf = dt::lifeOf(dt::HIGH, 100, 0, cl, tm, 360, 900); CHECK(lf.bars == 1 && !lf.broken, "ends when the next line starts");
    lf = dt::lifeOf(dt::LOW, 98.5, 0, cl, tm, 0, 900); CHECK(lf.bars == 3 && lf.broken, "a low line: closed below on the 3rd candle");
    // (1.1.0) the pivot key on T lines (old 10-field lines still read), X lines, the exhaustion choice, the 3-candle life, the medians
    {   dt::Turn q2; q2.t = 3875279400LL; q2.pivotT = 3875279220LL; q2.side = dt::LOW; q2.pivotPx = 27996; q2.linePx = 27997; dt::Line L2;
        CHECK(dt::parseLine(dt::lineOf('T', q2, 0.25), L2) && L2.pivotT == 3875279220LL && L2.linePx == 6999.25, "1.1.0 T line carries the pivot key");
        CHECK(dt::parseLine("T|5|0|7000|7000|10|0|0|5|3|0.25", L2) && L2.pivotT == 0, "an old 10-field T line still reads (pivot key 0)");
        dt::XLine X; CHECK(dt::parseX(dt::xLineOf(3875279400LL, dt::LOW, 27995, 0.25, 3875279220LL), X) && X.t == 3875279400LL && X.px == 6998.75 && X.at == 3875279220LL && X.side == dt::LOW, "X line round trip");
        CHECK(!dt::parseX("X|1|0|7000", X) && !dt::parseX("T|1|0|7000|0.25|1", X), "garbage X lines refused");
        CHECK(!dt::parseX("X|1||1|1|1", X) && !dt::parseX("X|1|0|nan|1|1", X) && !dt::parseX("X|1|0|1|1|999999999999999999999999", X), "empty, non-finite, and overflowed X fields refused");
        CHECK(!dt::parseX("X| 1|0|1|1|1", X) && !dt::parseX("X|1|0|0x1p0|1|1", X), "whitespace and hex X numeric fields refused");
        std::vector<dt::TouchV> T; auto add = [&](int s_, long long px, long long key, bool ad) { dt::TouchV v; v.s = s_; v.px = px; v.key = key; v.ad = ad; T.push_back(v); };
        add(0, 27997, 10, true); add(0, 27995, 11, true); add(0, 27994, 11, false); add(0, 27990, 9, true); add(1, 27993, 11, true); add(0, 27980, 11, true);
        long long px = 0, at = 0;
        CHECK(dt::exhaustionOf(T, dt::LOW, 10, 11, 27994, px, at) && px == 27995 && at == 11, "low: the bid touch nearest the low where the attack died (not the non-AD one, not other candles / offers / 14 ticks away)");
        T.clear(); add(0, 27997, 12, true); CHECK(!dt::exhaustionOf(T, dt::LOW, 10, 11, 27994, px, at), "a touch on another candle: none");
        T.clear(); add(1, 28010, 10, true); add(1, 28012, 11, true); CHECK(dt::exhaustionOf(T, dt::HIGH, 10, 11, 28012, px, at) && px == 28012, "high: the offer touch nearest the high");
        dt::Life lb = dt::lifeBars(dt::HIGH, 100, std::vector<double>{99, 99, 99, 99}, std::vector<long long>{1, 2, 3, 4}, 0, 3); CHECK(lb.bars == 2 && !lb.broken, "3 candles: the start + 2 after");
        lb = dt::lifeBars(dt::HIGH, 100, std::vector<double>{101, 99}, std::vector<long long>{1, 2}, 0, 3); CHECK(lb.bars == 1 && lb.broken, "closed through on the next candle");
        lb = dt::lifeBars(dt::LOW, 100, std::vector<double>{101, 101}, std::vector<long long>{5, 6}, 6, 3); CHECK(lb.bars == 1 && !lb.broken, "ends at the next line");
        CHECK(dt::adMedianOf("ES") == 0.095 && dt::adMedianOf("GC") == 0.0 && dt::adMedianOf("6E") == 0.067 && dt::adMedianOf("ZZ") == 0.05, "medians per market"); }
    {   std::vector<dt::Candle> hh = { C(1, 100, 104, 99, 103, 50, 40), C(2, 103, 108, 102, 107, 60, 50), C(3, 107, 110, 105, 109, 30, 80) };
        hh[2].refPx[1] = 108; hh[2].refLots[1] = 200; hh[2].o = 109.5;
        CHECK(!dt::decide(hh, 2, dt::HIGH, false, U, t), "(1.2.0) high: red close 109 above the R line 108 = no turn");
        hh[2].c = 107; CHECK(dt::decide(hh, 2, dt::HIGH, false, U, t) && t.linePx == 108, "(1.2.0) high: red close 107 below the line 108 = turn"); }
    {   // (1.3.0, Top Defender #5) H: the defenders hit back after an A
        dt::Turn T; T.t = 100; T.side = dt::LOW; T.pivotPx = 1000; T.A = true; T.attAggr = 300;
        auto K = [](long long t, long long c, long long buy, long long sell) { dt::Candle q; q.t = t; q.ok = true; q.o = c; q.h = c + 2; q.l = c - 2; q.c = c; q.aggr[1] = buy; q.aggr[0] = sell; return q; };
        dt::Counter C;
        CHECK(dt::counterDecide(T, std::vector<dt::Candle>{ K(101, 1003, 200, 50) }, 8.0, C) == 0, "1 of 2 candles: wait");
        CHECK(dt::counterDecide(T, std::vector<dt::Candle>{ K(101, 1003, 200, 50), K(102, 1004, 150, 40) }, 8.0, C) == 1 && C.H && C.lots == 350 && C.at == 102, "buyers lift 350 >= the 300 attack, close 4 ticks >= 0.25 ATR (2) above the low: H");
        CHECK(dt::counterDecide(T, std::vector<dt::Candle>{ K(101, 1003, 100, 50), K(102, 1004, 150, 40) }, 8.0, C) == 1 && !C.H, "250 < 300: absorbed only");
        CHECK(dt::counterDecide(T, std::vector<dt::Candle>{ K(101, 1003, 200, 50), K(102, 1001, 150, 40) }, 8.0, C) == 1 && !C.H, "close only 1 tick away (< 0.25 ATR): absorbed only");
        CHECK(dt::counterDecide(T, std::vector<dt::Candle>{ K(101, 999, 900, 50) }, 8.0, C) == 1 && !C.H && C.at == 101, "a close beyond the low ends it at once: absorbed only");
        dt::Candle nf = K(101, 1003, 200, 50); nf.ok = false;
        CHECK(dt::counterDecide(T, std::vector<dt::Candle>{ nf, K(102, 1004, 150, 40) }, 8.0, C) == -2, "no order feed in the window: no decision");
        dt::Turn N = T; N.A = false; CHECK(dt::counterDecide(N, std::vector<dt::Candle>{ K(101, 1003, 200, 50), K(102, 1004, 150, 40) }, 8.0, C) == -1, "no A: not applicable");
        dt::Turn Hh = T; Hh.side = dt::HIGH; Hh.pivotPx = 1010;   // at a high the defenders are the sellers (aggr[0])
        CHECK(dt::counterDecide(Hh, std::vector<dt::Candle>{ K(101, 1007, 50, 200), K(102, 1006, 40, 150) }, 8.0, C) == 1 && C.H && C.lots == 350, "high: sellers hit back 350, close 4 below the high: H");
        dt::Counter R; CHECK(dt::parseH(dt::hLineOf(C), R) && R.H && R.lots == 350 && R.attack == 300 && R.at == 102 && R.side == dt::HIGH, "H line round trip (restart redraws the same)");
        CHECK(!dt::parseH("H|1|0|2|5|5|5", R) && !dt::parseH("H|x", R) && !dt::parseH("T|1", R), "garbled H lines refused");
        CHECK(dt::counterText(C) == "Hit back 1.2x" && dt::counterText(dt::Counter()) == "Absorbed only", "the Counter row text");
        CHECK(dt::badgeText(true, false, false) == "A" && dt::badgeText(true, true, false) == "A|H" && dt::badgeText(false, true, false) == "H" && dt::badgeText(false, false, true) == "", "badge text A / A|H / H; E not shown while SHOW_E is off");
        auto w = [](const std::string& s) { return [s](int pt) { return (int)(s.size() * pt * 0.62) + 4; }; };
        CHECK(dt::fitFont(w("A?"), 20, 9, 6) == 9 && dt::fitFont(w("A|H?"), 25, 9, 6) == 8 && dt::fitFont(w("A|H|E?"), 25, 9, 6) == 6, "the font shrinks so 1 / 2 / 3 letters fit the candle");
        std::vector<dt::Candle> at = { K(1, 100, 0, 0), K(2, 100, 0, 0) }; at[1].h = 106; at[1].l = 98; CHECK(std::fabs(dt::atrOf(at) - 8.0) < 1e-9, "ATR from the candles");
        dt::Line L; L.A = true; CHECK(dt::tagsOfH(L, true) == "R A H ?" && dt::tagsOfH(L, false) == "R A ?", "the line label adds H");
        CHECK(std::string(dt::VERSION) == "1.5.0", "DefenseTurn 1.5.0");
    }
    {   // (1.4.0) Top Defender #4: the RE-TEST of the R line -> L (reloaded) / X (empty) / N, decided once at the re-test candle's close
        auto C3 = [](long long t, long long h, long long l, long long c) { dt::Candle q; q.t = t; q.o = c; q.h = h; q.l = l; q.c = c; q.ok = true; return q; };
        const double atr = 8.0;                                 // zone = max(0.8, 2) = 2 ticks; away = 4 ticks
        dt::Retest r; r.t = 0; r.side = dt::LOW; r.linePx = 1000;
        CHECK(dt::rtZone(atr) == 2, "zone = max(0.1 ATR, 2 ticks)");
        long long lo = 0, hi = 0; dt::rtRange(r, atr, lo, hi); CHECK(lo == 998 && hi == 1004, "LOW: the bids from line - zone to line + 4 ticks count");
        CHECK(dt::retestStep(r, C3(180, 1003, 1001, 1002), atr, 500, 100) == 0 && r.state == 0, "not yet 0.5 ATR away: waiting");
        CHECK(dt::retestStep(r, C3(360, 1006, 1002, 1005), atr, 900, 100) == 0 && r.state == 1 && r.leftAt == 360, "6 ticks above (>= 4): price LEFT");
        CHECK(dt::retestStep(r, C3(540, 1008, 1004, 1007), atr, 600, 100) == 0 && r.restBefore == 600, "still away: the resting lots at this close are kept (600)");
        CHECK(dt::retestStep(r, C3(720, 1006, 1001, 1003), atr, 50, 100) == 1 && r.res == 'L' && r.rest == 600 && std::fabs(r.ratio - 1.2) < 1e-9 && r.at == 720,
              "back within the zone: decided from the lots BEFORE the return (600 / (100 x 5) = 1.2x -> L), not the 50 at this close");
        dt::Retest x; x.side = dt::LOW; x.linePx = 1000; dt::retestStep(x, C3(180, 1006, 1001, 1005), atr, 100, 100);
        CHECK(dt::retestStep(x, C3(360, 1004, 999, 1001), atr, 80, 100) == 1 && x.res == 'X' && std::fabs(x.ratio - 0.2) < 1e-9, "100 / 500 = 0.2x <= 0.35: X (empty)");
        dt::Retest n; n.side = dt::LOW; n.linePx = 1000; dt::retestStep(n, C3(180, 1006, 1001, 1005), atr, 300, 100);
        CHECK(dt::retestStep(n, C3(360, 1004, 999, 1001), atr, 80, 100) == 1 && n.res == 'N', "0.6x: between (N, ratio only)");
        dt::Retest b; b.side = dt::LOW; b.linePx = 1000;
        CHECK(dt::retestStep(b, C3(180, 1002, 997, 998), atr, 100, 100) == -1, "closed through the line before it left: over");
        dt::Retest e; e.side = dt::LOW; e.linePx = 1000; dt::retestStep(e, C3(180, 1006, 1001, 1005), atr, 100, 100);
        CHECK(dt::retestStep(e, C3(180 + dt::RT_SEC + 1, 1006, 1004, 1005), atr, 100, 100) == -1, "no return within 15 min of leaving: over");
        dt::Retest u; u.side = dt::LOW; u.linePx = 1000; dt::retestStep(u, C3(180, 1006, 1001, 1005), atr, 100, 0);
        CHECK(dt::retestStep(u, C3(360, 1004, 999, 1001), atr, 80, 0) == -2, "no market normal yet: no decision (never guessed)");
        dt::Retest h; h.side = dt::HIGH; h.linePx = 2000; dt::rtRange(h, atr, lo, hi); CHECK(lo == 1996 && hi == 2002, "HIGH: the offers from line - 4 to line + zone");
        dt::retestStep(h, C3(180, 1999, 1994, 1995), atr, 700, 100);
        CHECK(h.state == 1 && dt::retestStep(h, C3(360, 1999, 1995, 1997), atr, 10, 100) == 1 && h.res == 'L', "HIGH: left 6 below, back within 2 ticks: L from the 700 before");
        dt::Retest R2; CHECK(dt::parseQ(dt::qLineOf(r), R2) && R2.res == 'L' && R2.rest == 600 && std::fabs(R2.ratio - 1.2) < 1e-9 && R2.at == 720 && R2.side == dt::LOW, "Q line round trip (restart redraws the same)");
        CHECK(!dt::parseQ("Q|1|0|Z|5|5|5", R2) && !dt::parseQ("Q|x", R2) && !dt::parseQ("H|1|0|1|1|1|1", R2), "garbled Q lines refused");
        CHECK(dt::retestText(r) == "Reloaded 1.2x" && dt::retestText(x) == "Empty 0.2x" && dt::retestText(n) == "0.6x", "the Re-test row text");
        CHECK(dt::badgeText4(true, true, 'L', false) == "A|H|L" && dt::badgeText4(false, false, 'X', false) == "X" && dt::badgeText4(true, false, 'N', false) == "A", "badge A|H|L / X; N adds no letter");
        auto w2 = [](const std::string& s) { return [s](int pt) { return (int)(s.size() * pt * 0.62) + 4; }; };
        CHECK(dt::fitFont(w2("A|H|L?"), 26, 9, 6) <= 7, "3 letters + ? shrink the font to fit");
    }
    printf("DefenseTurn: %d passed, %d failed\n", p, f);
    return f ? 1 : 0;
}
