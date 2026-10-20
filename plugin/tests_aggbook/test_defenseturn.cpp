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
    h[2].net[0] = -20; CHECK(dt::decide(h, 2, dt::HIGH, false, U, t) && dt::marginText(t.marginPct) == "+999%", "attacking side pulled: +999%");
    // file lines
    dt::Turn q; q.t = 3875279220LL; q.side = dt::HIGH; q.pivotPx = 31376; q.linePx = 31374; q.marginPct = 23; q.E = true; q.defNet = 3847; q.attNet = 3363;
    const std::string ln = dt::lineOf('T', q, 0.25); dt::Line L;
    CHECK(dt::parseLine(ln, L) && L.t == q.t && L.side == dt::HIGH && L.linePx == 7843.5 && L.pivotPx == 7844 && L.margin == 23 && L.E && !L.A && dt::tagsOf(L) == "R E ?", "file line round trip");
    CHECK(!dt::parseLine("T|1|2|3", L) && !dt::parseLine("X|1|0|1|1|0|0|0|0|0|0.25", L) && !dt::parseLine(ln.substr(0, ln.size() - 6), L), "garbage / half lines refused");
    // line life: 3-min bars, closes after a high line at 100
    std::vector<double> cl = { 99, 99.5, 98, 101, 97 }; std::vector<long long> tm = { 180, 360, 540, 720, 900 };
    dt::Life lf = dt::lifeOf(dt::HIGH, 100, 0, cl, tm, 0, 900); CHECK(lf.bars == 4 && lf.broken, "closed above on the 4th candle: broken there");
    lf = dt::lifeOf(dt::HIGH, 100, 0, std::vector<double>{99, 99, 99, 99, 99, 99, 99}, std::vector<long long>{180, 360, 540, 720, 900, 1080, 1260}, 0, 900);
    CHECK(lf.bars == 5 && !lf.broken, "15 minutes = 5 candles");
    lf = dt::lifeOf(dt::HIGH, 100, 0, cl, tm, 360, 900); CHECK(lf.bars == 1 && !lf.broken, "ends when the next line starts");
    lf = dt::lifeOf(dt::LOW, 98.5, 0, cl, tm, 0, 900); CHECK(lf.bars == 3 && lf.broken, "a low line: closed below on the 3rd candle");
    printf("DefenseTurn: %d passed, %d failed\n", p, f);
    return f ? 1 : 0;
}
