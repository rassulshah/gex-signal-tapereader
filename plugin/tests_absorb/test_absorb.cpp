// AbsorbLogic.h 1.0.0 + SignalBoard 1.2.0 kind 'O' + OrderFlowFeed 1.7.0 absorbedPx (2026-10-21, MBO Roadmap Top Defender #1)
// g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_absorb.cpp (run from tests_absorb; mock BannerBoard env via setenv)
#include "../AbsorbLogic.h"
#include "../SignalBoard.h"
#include "../OrderFlowFeed.h"
#include <cstdio>
static int P = 0, F = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++F; std::printf("FAIL line %d: %s\n", __LINE__, m); } } while (0)
using namespace absl;
static Candle topCandle() {   // ES 10-08 11:18 shape, keys in ticks: open 31302, close 31368 (green), high 31400, low 31299
    Candle c; c.open = 31302; c.close = 31368; c.high = 31400; c.low = 31299;
    for (long long k = c.low; k <= c.high; ++k) { Row r; r.key = k; r.bought = k > c.close ? 250 : 100; r.sold = 90; c.rows.push_back(r); }
    return c;   // 32 wick prices x 250 = 8,000 bought in the wick
}
int main() {
    // --- slot / obs / norm (no look-ahead, last 10 sessions)
    CHECK(slotOf(0) == 0 && slotOf(675) == 22 && slotOf(1439) == 47 && slotOf(1440) == -1, "half-hour slots");
    std::vector<Obs> obs;
    for (int d = 1; d <= 15; ++d) for (int k = 0; k < 2; ++k) { Obs o; o.session = 20000 + d; o.slot = 22; o.top = 1; o.lots = d <= 5 ? 5000 : 900; obs.push_back(o); }
    Obs future; future.session = 20030; future.slot = 22; future.top = 1; future.lots = 1e6; obs.push_back(future);
    double norm = 0; int n = 0;
    CHECK(slotNorm(obs, 20016, 22, 1, norm, n) && n == 20 && norm == 900, "norm = median of the 10 sessions before (old 5000s and the future row ignored)");
    CHECK(!slotNorm(obs, 20016, 23, 1, norm, n) && n == 0, "no data in that slot = no norm");
    CHECK(!slotNorm(obs, 20003, 22, 1, norm, n) && n == 4, "fewer than 8 observations = learning");
    Obs o2; CHECK(parseObs("20500,22,1,8119", o2) && o2.lots == 8119 && obsLine(o2) == "20500,22,1,8119", "obs line round trip");
    CHECK(!parseObs("20500,48,1,5", o2) && !parseObs("20500,3,2,5", o2) && !parseObs("x,1,1,1", o2) && !parseObs("20500,3,1,-4", o2), "bad obs lines rejected");
    int bad = 0; std::vector<Obs> pv = parseObsText("# seed\n20500,22,1,10\r\nbad\n20501,22,0,7\n", &bad);
    CHECK(pv.size() == 2 && bad == 1, "obs text: comments skipped, bad counted, CRLF ok");
    // --- wick zone / attack / local extreme
    Candle c = topCandle(); long long lo = 0, hi = 0;
    CHECK(wickZone(c, true, lo, hi) && lo == 31369 && hi == 31400, "top wick = above the body to the high");
    CHECK(wickZone(c, false, lo, hi) && lo == 31299 && hi == 31301, "bottom wick = low to below the body");
    CHECK(wickAttack(c, true) == 32 * 250, "attackers' lots in the top wick (bought)");
    CHECK(wickAttack(c, false) == 3 * 90, "attackers' lots in the bottom wick (sold)");
    Candle doji; doji.open = doji.close = doji.high = 10; doji.low = 5; CHECK(!wickZone(doji, true, lo, hi), "no top wick when the body touches the high");
    std::vector<long long> H = {10, 12, 11, 13, 12, 14}, L = {5, 6, 4, 7, 6, 8};
    CHECK(localExtreme(H, L, 5, true) && !localExtreme(H, L, 5, false), "a new 5-candle high; not a low");
    H[5] = 13; CHECK(localExtreme(H, L, 5, true), "an equal high counts (double top)");
    H[5] = 12; CHECK(!localExtreme(H, L, 5, true), "below a prior high: not an extreme");
    CHECK(!localExtreme(H, L, 3, true), "needs 5 candles before");
    // --- the wick A: footprint + order book
    const double tick = 0.25; Params Pm; Pm.x = 3.0;
    std::vector<ObEv> ev; ObEv a; a.px = 31390 * tick; a.side = 1; a.traded = 2500; a.shown = 52; a.refills = 4; ev.push_back(a);
    ObEv b2 = a; b2.px = 31380 * tick; b2.traded = 1000; ev.push_back(b2);
    ObEv wrongSide = a; wrongSide.side = -1; wrongSide.traded = 99999; ev.push_back(wrongSide);
    ObEv outside = a; outside.px = 31360 * tick; outside.traded = 99999; ev.push_back(outside);   // inside the body: not the wick
    WickResult R = wickA(c, true, true, 900, 20, true, true, ev, tick, Pm);
    CHECK(R.candidate && R.absorbed && R.absorbedLots == 3500 && std::fabs(R.x - 8000.0 / 900) < 1e-9, "absorbed: 8.9x attack, 3,500 of 8,000 lots absorbed in the wick (44% >= 25%)");
    CHECK(!wickA(c, true, false, 900, 20, true, true, ev, tick, Pm).candidate, "not at an extreme: nothing");
    WickResult nf = wickA(c, true, true, 900, 20, true, false, ev, tick, Pm);
    CHECK(nf.candidate && !nf.absorbed && nf.why.find("no order feed") == 0, "no order feed: candidate only, never drawn");
    WickResult ln = wickA(c, true, true, 0, 3, false, true, ev, tick, Pm);
    CHECK(!ln.candidate && ln.why.find("learning") == 0, "no half-hour norm yet: learning, nothing drawn");
    WickResult small = wickA(c, true, true, 3000, 20, true, true, ev, tick, Pm);
    CHECK(!small.candidate && small.x < 3.0, "attack 2.7x < 3.0x: not a candidate");
    std::vector<ObEv> thin; ObEv t1 = a; t1.traded = 1000; thin.push_back(t1);
    WickResult th = wickA(c, true, true, 900, 20, true, true, thin, tick, Pm);
    CHECK(th.candidate && !th.absorbed && th.share < 0.25, "order book: only 12.5% absorbed -> no A (the footprint alone is not enough)");
    std::vector<ObEv> none; CHECK(!wickA(c, true, true, 900, 20, true, true, none, tick, Pm).absorbed, "decided candle with no absorbed price: no A");
    // --- the Big Bid / Ask circle
    CHECK(printAbsorbed(c, +1, 31390, true, ev, tick), "ask print in the top wick at an absorbed price: circle");
    CHECK(!printAbsorbed(c, +1, 31385, true, ev, tick), "ask print in the wick but that price not absorbed: no circle");
    CHECK(!printAbsorbed(c, +1, 31360, true, ev, tick), "print inside the body: no circle (even if absorbed there)");
    CHECK(!printAbsorbed(c, +1, 31390, false, ev, tick), "no order feed: no circle");
    CHECK(!printAbsorbed(c, -1, 31390, true, ev, tick), "a bid print is judged on the bottom wick / resting bids");
    // --- SignalBoard kind O round trip
    CHECK(std::string(sb::prefix('O')) == "LRA_SGO_" && sb::capOf('O') == 160 && sb::capOf('A') == 40, "board kind O prefix / cap");
    std::vector<sb::Ev> v; for (int i = 0; i < 200; ++i) { sb::Ev e; e.bar = 1000 + i; e.px = 7847.75; e.side = i % 2 ? 1 : -1; e.val = 428; e.flag = sb::obFlag(52, 4); v.push_back(e); }
    sb::publish('O', "EPZ26", 180, "ES 3m", 5000, v);
    std::vector<sb::Ev> got; CHECK(sb::readAll('O', "EPZ26", 180, "ES 3m", 6000, got) && got.size() == 160 && got.front().bar == 1040 && got.back().bar == 1199, "O: newest 160 kept, read back");
    CHECK(sb::obShown(got.back().flag) == 52 && sb::obRefills(got.back().flag) == 4 && got.back().val == 428 && got.back().side == 1, "O: shown / refills / traded / side survive");
    CHECK(sb::obFlag(5, 250) % 100 == 99 && sb::obFlag(-3, -1) == 0, "O flag clamps");
    CHECK(!sb::readAll('O', "EPZ26", 180, "ES 3m", 5000 + 61000, got), "O older than 60 s ignored");
    std::vector<sb::Ev> fortyA(v.begin(), v.begin() + 50); sb::publish('A', "EPZ26", 180, "ES 3m", 5000, fortyA);
    CHECK(sb::readAll('A', "EPZ26", 180, "ES 3m", 6000, got) && got.size() == 40, "kind A keeps its 40 cap");
    // --- OrderFlowFeed absorbedPx on a hand-made candle accumulator
    off::Acc acc;
    off::Ice i1; i1.traded = 428; i1.shown = 52; acc.ice[off::ASK][31390] = i1; acc.refAt[off::ASK][31390] = 4;
    off::Ice i2; i2.traded = 428; i2.shown = 52; acc.ice[off::ASK][31391] = i2; acc.refAt[off::ASK][31391] = 1;      // 1 refill: no
    off::Ice i3; i3.traded = 90; i3.shown = 52; acc.ice[off::ASK][31392] = i3; acc.refAt[off::ASK][31392] = 9;       // 1.7x: no
    off::Ice i4; i4.traded = 500; i4.shown = 0; acc.ice[off::ASK][31393] = i4; acc.refAt[off::ASK][31393] = 9;       // never shown: no
    off::Ice i5; i5.traded = 500; i5.shown = 10; i5.unknown = true; acc.ice[off::ASK][31394] = i5; acc.refAt[off::ASK][31394] = 9;   // unknown: no
    off::Ice i6; i6.traded = 200; i6.shown = 50; acc.ice[off::BID][31300] = i6; acc.refAt[off::BID][31300] = 2;
    std::vector<off::ObPx> ob; off::absorbedPx(acc, off::ASK, ob);
    CHECK(ob.size() == 1 && ob[0].px == 31390 && ob[0].traded == 428 && ob[0].shown == 52 && ob[0].refills == 4, "absorbedPx: >= 2x shown AND >= 2 refills only");
    off::absorbedPx(acc, off::BID, ob); CHECK(ob.size() == 1 && ob[0].px == 31300, "the bid side on its own");
    off::absorbedPx(acc, 7, ob); CHECK(ob.empty(), "bad side: nothing");
    CHECK(std::string(absl::VERSION) == "1.0.0" && std::string(sb::VERSION) == "1.2.0", "versions");
    std::printf("AbsorbLogic / board O / absorbedPx: %d checks, %d failed\n", P + F, F);
    return F ? 1 : 0;
}
