// tests_turn/test_turn.cpp -- unit tests of TurnLogic.h (lsTurn 1.1.0; 1.1.0: effOf, stripOfEffort, candPlainSide, the fast pickLabels = the plain one;
// 1.0.1: labelText, stripOfCand, levelNameAt, candSide, pickLabels). g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror
#include "../TurnLogic.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <ctime>
#include <random>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static bool near(double a, double b, double e = 1e-9) { return std::fabs(a - b) <= e; }
using namespace turn;

static void effort() {
    CHECK(zoneOf(4) == 1); CHECK(zoneOf(5) == 1); CHECK(zoneOf(6) == 2); CHECK(zoneOf(10) == 3); CHECK(zoneOf(12) == 3); CHECK(zoneOf(14) == 4); CHECK(zoneOf(1) == 1);
    std::vector<Row> r = { {100, 10, 40}, {101, 10, 10}, {102, 10, 10}, {103, 30, 5} };
    Bars b = barRatios(r, 100, 103);
    CHECK(b.topOk && near(b.top, (30.0 / 1) / (30.0 / 3)));     // 3.0
    CHECK(b.botOk && near(b.bot, (40.0 / 1) / (25.0 / 3)));     // 4.8
    CHECK(!barRatios(r, 100, 102).topOk);                       // 3 levels: no ratio
    CHECK(!barRatios(r, 103, 100).topOk);                       // inverted
    std::vector<Row> onlyTop = { {100, 0, 5}, {103, 9, 0} };
    Bars c = barRatios(onlyTop, 100, 103); CHECK(!c.topOk); CHECK(!c.botOk);
    std::vector<Row> g = { {200, 4, 4}, {205, 4, 4} };
    Bars d = barRatios(g, 200, 205); CHECK(d.levels == 6 && d.zone == 2); CHECK(near(d.top, (4.0 / 2) / (4.0 / 4))); CHECK(near(d.bot, 2.0));
    std::vector<Row> neg = { {100, -5, 1}, {101, 1, 1}, {102, 1, 1}, {103, 1, 1} };
    CHECK(barRatios(neg, 100, 103).topOk);                      // a negative row is skipped, not summed
    std::vector<Row> big = { {100, 3000000000LL, 1}, {101, 3000000000LL, 1}, {102, 3000000000LL, 1}, {103, 9000000000LL, 1} };
    CHECK(near(barRatios(big, 100, 103).top, 3.0));             // volumes over 2^31
}
static void words() {
    CHECK(ratioText(1.74) == "1.7x"); CHECK(ratioText(120) == "99x+"); CHECK(ratioText(-1).empty()); CHECK(ratioText(NAN).empty()); CHECK(ratioText(0) == "0.0x");
    CHECK(stopClass(0.49) == 0 && stopClass(0.5) == 1 && stopClass(0.99) == 1 && stopClass(1.0) == 2 && stopClass(2.99) == 2 && stopClass(3.0) == 3 && stopClass(NAN) == 0);
    CHECK(effClass(0.49) == 0 && effClass(0.5) == 1 && effClass(1.0) == 2 && effClass(1.49) == 2 && effClass(1.5) == 3 && effClass(NAN) == 0);
    CHECK(exhClassOf(0.60, Q1_DEF, Q2_DEF) == 0 && exhClassOf(0.59, Q1_DEF, Q2_DEF) == 2 && exhClassOf(0.46, Q1_DEF, Q2_DEF) == 2 && exhClassOf(0.45, Q1_DEF, Q2_DEF) == 3 && exhClassOf(NAN, Q1_DEF, Q2_DEF) == 0);
    CHECK(std::string(word4(0)) == "No" && std::string(word4(1)) == "Light" && std::string(word4(2)) == "Moderate" && std::string(word4(3)) == "Heavy" && std::string(word4(9)) == "Heavy" && std::string(word4(-1)) == "No");
    CHECK(near(exhBar(0.5), 1.0) && near(exhBar(0.0), 2.0) && near(exhBar(1.0), 0.0));
    CHECK(clockText(9, 3) == "9:03 AM" && clockText(0, 0) == "12:00 AM" && clockText(12, 30) == "12:30 PM" && clockText(13, 0) == "1:00 PM" && clockText(24, 0).empty() && clockText(1, 60).empty());
    CHECK(sideOfName("ONH") == -1 && sideOfName("LonHI") == -1 && sideOfName("PhrH") == -1 && sideOfName("1hH") == -1 && sideOfName("VAH") == -1);
    CHECK(sideOfName("ONL") == 1 && sideOfName("LonLO") == 1 && sideOfName("AsiaLO") == 1 && sideOfName("PhrL") == 1 && sideOfName("VAL") == 1 && sideOfName("PHL") == 1);
    CHECK(sideOfName("POC") == 0 && sideOfName("VWAP") == 0 && sideOfName("") == 0);
    CHECK(keyOf(1200, 180) == 1260 && keyOf(1259, 180) == 1260 && keyOf(1260, 180) == 1440);
    CHECK(barPx(3.0, 100) == 100 && barPx(9.0, 100) == 100 && barPx(1.5, 100) == 50 && barPx(0.001, 100) == 2 && barPx(NAN, 100) == 0 && barPx(1.0, 0) == 0);
}
static void wy() {
    std::vector<Sw> v; std::vector<Level> lv; std::string why;
    const std::string t = "WY|1|ES|180|0.21.7\nL|28420|ONL/PHL\nL|28733|LonHI\nL|28797|ONH\nL|28771|VAL\nL|28500|POC\nL|abc|ONH\nL|28800|O N H\nL|28900|PDH/ONL\n"
                          "3878010180|Sw|-1|ONH|28797|R99+\n3878009460|Sw|-1|LHI|28733|R99+\n3877953480|Bk|1|PDL|28768|R10\n"
                          "bad|Sw|-1|ONH|1|\n3878010000|Sw|2|ONH|28797|\n3878010001|Sw|1|O N H|28797|\n3878020000|Sw|1|PDL|28768";   // last line unterminated
    CHECK(parseWy(t, "ES", 180, v, lv, why)); CHECK(v.size() == 2);
    CHECK(v.size() == 2 && v[0].stamp == 3878009460LL && v[0].lv == "LonHI" && v[0].dir == -1 && v[0].lvt == 28733);
    CHECK(v.size() == 2 && v[1].stamp == 3878010180LL && v[1].lv == "ONH");
    CHECK(lv.size() == 5);                                       // POC (no side), the garbled and the spaced name are left out
    CHECK(lv.size() == 5 && lv[0].tick == 28420 && lv[0].lo && !lv[0].hi && lv[0].name == "ONL/PHL");
    CHECK(lv.size() == 5 && lv[1].tick == 28733 && lv[1].hi && !lv[1].lo && lv[2].tick == 28771 && lv[2].lo && lv[3].tick == 28797 && lv[3].hi);
    CHECK(lv.size() == 5 && lv[4].tick == 28900 && lv[4].hi && lv[4].lo);     // a high and a low at one price: both sides
    long long hiL = -1, loL = -1;
    levelsIn(lv, 28700, 28800, 28700, hiL, loL); CHECK(hiL == 28797 && loL == 0);       // up from the low: the highest high reached, no low (it opened under them)
    levelsIn(lv, 28700, 28800, 28800, hiL, loL); CHECK(hiL == 0 && loL == 28771);       // down from the high: the lowest low reached, no high
    levelsIn(lv, 28700, 28800, 28780, hiL, loL); CHECK(hiL == 28797 && loL == 28771);   // from the middle both ways: both sides
    levelsIn(lv, 28700, 28800, 28797, hiL, loL); CHECK(hiL == 0);                        // opened AT the high: not reached from below
    levelsIn(lv, 28734, 28770, 28750, hiL, loL); CHECK(hiL == 0 && loL == 0);
    levelsIn(lv, 28790, 28797, 28790, hiL, loL); CHECK(hiL == 28797 && loL == 0);       // the edges count
    levelsIn(lv, 28000, 29000, 28500, hiL, loL); CHECK(hiL == 28900 && loL == 28420);
    levelsIn(lv, 28000, 29000, 28950, hiL, loL); CHECK(hiL == 0 && loL == 28420);
    CHECK(!parseWy(t, "NQ", 180, v, lv, why) && v.empty() && lv.empty());       // foreign market
    CHECK(!parseWy(t, "ES", 60, v, lv, why));                    // bar size
    CHECK(!parseWy("garbage\n", "ES", 180, v, lv, why));
    CHECK(!parseWy(std::string(WY_MAX_BYTES + 1, 'x'), "ES", 180, v, lv, why));
    std::string many = "WY|1|ES|180|x\n"; for (int i = 0; i < 1000; ++i) many += std::to_string(3878000000LL + i * 180) + "|Sw|1|PDL|28768|\n";
    for (int i = 0; i < 200; ++i) many += "L|" + std::to_string(20000 + i) + "|PDH\n";
    CHECK(parseWy(many, "ES", 180, v, lv, why) && v.size() == SW_KEEP && v.back().stamp == 3878000000LL + 999 * 180 && lv.size() == LEVEL_KEEP);
    // the same price twice: one level, the names joined
    CHECK(parseWy("WY|1|ES|180|x\nL|100|ONH\nL|100|PDL\n", "ES", 180, v, lv, why) && lv.size() == 1 && lv[0].hi && lv[0].lo && lv[0].name == "ONH/PDL");
}
static std::string sampleRead(long long stamp, char src) {
    Read r; r.stamp = stamp; r.dir = -1; r.lv = "ONH"; r.lvt = 28797; r.stopLots = 112; r.stopX1000 = 5209; r.eff1000 = 1800; r.late1e4 = 8224; r.decay1e4 = 50538; r.after1e4 = 8818;
    r.dem1e4 = 4667; r.exh = 2; r.F = 1; r.H = 1; r.B = 0; r.marks = 2; r.ansOk = 1; r.hard100 = 420; r.big100 = 1250; r.extInto = 30; r.extLeft = 150; r.verdict = V_BEST; r.why = 0; r.odds = -1; r.src = src;
    return readText(r, ',');
}
static std::string paramsText(const std::string& mkt) {
    std::string s = "VERSION,1\nMARKET," + mkt + "\nSTOP_TYP,20\nHARD_CUT,3\nBIG_CUT,10\nEXH_CUT,0.45,0.6\nODDS,-1,25,50,-1\nBEST_PROVEN,0\nN15";
    for (int i = 0; i < 48; ++i) { s += ",10"; }
    s += "\nAVG_ORDER";
    for (int i = 0; i < 48; ++i) { s += ",4"; }
    s += "\nRANK_LATE,2.5,0.5,1,1.5,2\nRANK_DECAY,0.5,0.8,1.0,1.2,3\nRANK_AFTER,0.1,0.2,0.3,0.4,0.5\n";
    s += "SW," + sampleRead(3878010180LL, 'N') + "\nSW," + sampleRead(3878009460LL, 'N') + "\nFUTURE_KEY,1\nEND\n"; return s;
}
static void params() {
    Params p = parseParams(paramsText("ES"), "ES");
    CHECK(p.fromFile && p.why == "ok" && near(p.stopTyp, 20) && near(p.hardCut, 3) && near(p.bigCut, 10) && near(p.q1, 0.45) && near(p.q2, 0.6) && near(p.n15[47], 10) && near(p.avgOrd[0], 4));
    CHECK(p.odds[0] == -1 && p.odds[1] == 25 && p.odds[2] == 50 && p.odds[3] == -1 && !p.bestProven);
    CHECK(p.rkLate.size() == 5 && near(p.rkLate[0], 0.5) && near(p.rkLate[4], 2.5));   // sorted
    CHECK(p.hist.size() == 2 && p.hist[0].stamp == 3878009460LL && p.hist[1].verdict == V_BEST && p.hist[1].src == 'N' && p.hist[1].lv == "ONH" && p.hist[1].stopLots == 112);
    CHECK(near(rankOf(p.rkLate, 1.8), 0.6) && near(rankOf(p.rkLate, 0.5), 0.2) && near(rankOf(p.rkLate, 0.49), 0.0) && near(rankOf(p.rkLate, 99), 1.0));   // share of past values <= x
    CHECK(rankOf(p.rkLate, NAN) < 0); std::vector<double> four = { 1, 2, 3, 4 }; CHECK(rankOf(four, 2) < 0);   // under 5 past values: no rank
    Params w = parseParams(paramsText("ES"), "NQ"); CHECK(!w.fromFile && near(w.stopTyp, 24) && w.why.find("market") != std::string::npos && w.rkLate.empty());
    Params df = defaults("GC"); CHECK(near(df.stopTyp, 8) && near(df.hardCut, 3.059) && near(df.bigCut, 3.309) && near(df.q2, Q2_DEF) && df.odds[1] == 26 && df.odds[2] == 52 && df.odds[3] == -1);
    CHECK(defaults("XX").stopTyp == 0);
    const char* bad[][2] = { { "HARD_CUT,3", "HARD_CUT,x" }, { "STOP_TYP,20", "STOP_TYP,0" }, { "EXH_CUT,0.45,0.6", "EXH_CUT,0.6,0.45" }, { "EXH_CUT,0.45,0.6", "EXH_CUT,0.45,1.2" },
                             { "ODDS,-1,25,50,-1", "ODDS,-1,25,500,-1" }, { "ODDS,-1,25,50,-1", "ODDS,-1,25,50" }, { "BEST_PROVEN,0", "BEST_PROVEN,2" }, { "RANK_LATE,2.5", "RANK_LATE,nan" },
                             { "RANK_AFTER,0.1", "RANK_AFTER,-0.1" }, { "N15,10", "N15,-10" }, { "AVG_ORDER,4", "AVG_ORDER,0.001" }, { "SW,3878010180,-1", "SW,3878010180,3" }, { "SW,3878010180", "SW,-5" } };
    for (const auto& b : bad) { std::string g = paramsText("ES"); const size_t at = g.find(b[0]); CHECK(at != std::string::npos); if (at == std::string::npos) continue;
        g.replace(at, std::strlen(b[0]), b[1]); const Params q = parseParams(g, "ES"); CHECK(!q.fromFile && near(q.hardCut, 3.010) && q.hist.empty() && q.rkLate.empty() && q.why.find("garbled") != std::string::npos); }
    { std::string g = paramsText("ES"); g.replace(g.find(",N\nSW"), 2, ",L"); CHECK(!parseParams(g, "ES").fromFile); }   // a nightly row must say N
    CHECK(!parseParams(std::string(PARAMS_MAX_BYTES + 5, 'a'), "ES").fromFile);
    CHECK(!parseParams("", "ES").fromFile); CHECK(!parseParams("VERSION,2\nMARKET,ES\n", "ES").fromFile);
    std::string proven = paramsText("ES"); proven.replace(proven.find("BEST_PROVEN,0"), 13, "BEST_PROVEN,1"); proven.replace(proven.find("ODDS,-1,25,50,-1"), 16, "ODDS,-1,25,50,70");
    Params pp = parseParams(proven, "ES"); CHECK(pp.fromFile && pp.bestProven && pp.odds[3] == 70);
    std::string trunc = paramsText("ES"); trunc = trunc.substr(0, trunc.find("AVG_ORDER") + 20); CHECK(!parseParams(trunc, "ES").fromFile || parseParams(trunc, "ES").avgOrd[0] == 0);
    std::string manyRanks = "VERSION,1\nMARKET,ES\nRANK_LATE"; for (size_t i = 0; i < RANK_KEEP + 1; ++i) manyRanks += ",1"; manyRanks += "\n"; CHECK(!parseParams(manyRanks, "ES").fromFile);
}
// the worked candle of the header: spb 60, key 1260 = [1200, 1260), the high side
static void feedCandle(Live& L, bool mirror) {
    const long long K = 400;                                          // mirror: price -> K - price, the sides swapped
    auto ev = [&](long long sec, int act, int bs, long long px, long long sz, long long oid, long long aid) { L.onEvent(sec, act, mirror && bs != 2 ? 1 - bs : bs, mirror ? K - px : px, sz, oid, aid); };
    ev(1100, 3, 1, 99, 1, 1, 900);                                    // the feed and the ID clock were live 100 s before the candle
    ev(1150, 0, 1, 110, 5, 850, 0);                                   // order 850 rests (known), issued before the clock's newest ID
    ev(1205, 3, 1, 100, 10, 11, 1001);                                // new high 1 (the first print)
    ev(1210, 3, 1, 101, 10, 12, 1002);                                // new high 2
    ev(1215, 3, 0, 100, 5, 13, 1003);
    ev(1220, 3, 1, 102, 20, 14, 1004);                                // new high 3
    ev(1228, 3, 1, 103, 4, 15, 850);                                  // new high 4: an OLD order, but a known resting one (not a stop)
    ev(1230, 3, 1, 104, 6, 16, 800);                                  // new high 5 = the extreme: an OLD order never seen resting = a stop
    ev(1231, 3, 0, 103, 30, 17, 2001);                                // the sellers answer: a fresh order through 3 prices
    ev(1232, 3, 0, 102, 10, 18, 2001);
    ev(1233, 3, 0, 101, 10, 19, 2001);
    ev(1240, 3, 1, 102, 8, 20, 1007);                                 // buying after the extreme
    ev(1250, 3, 0, 100, 12, 21, 2002);                                // 20 s after the extreme: outside the 15-s window, inside the 30-s one
    ev(1261, 3, 1, 101, 1, 22, 2003);                                 // the next candle's first print closes it
}
static Params testParams() { return parseParams(paramsText("ES"), "ES"); }
static void readings() {
    Live L; L.reset(60); feedCandle(L, false);
    const Sum* s = L.find(1260); CHECK(s != nullptr); if (!s) return;
    CHECK(s->complete && s->stopOk && !s->thin && s->start == 1200 && s->hiPx == 104 && s->loPx == 100 && s->fills == 11 && s->lots == 125);
    // the Answer (as lsEffortRewardPane): first print at the extreme, FAST / HARD / BIG
    CHECK(s->hi.ok && s->hi.extPx == 104 && s->hi.extSec == 1230 && s->hi.fresh && s->hi.hard == 42 && s->hi.big == 50);
    // Exhaustion: B 50 lots in 30 s; the last 10 s 30 lots; 5 new highs, second half from the 102 print
    CHECK(s->exHi.ok && s->exHi.push == 50);
    CHECK(near(s->exHi.late, (30.0 / 10.0) / (50.0 / 30.0)));          // 1.8
    CHECK(s->exHi.hasDecay && near(s->exHi.decay, (30.0 / 3.0) / (20.0 / 2.0)));   // 1.0
    CHECK(near(s->exHi.after, (8.0 / 30.0) / (50.0 / 30.0)));          // 0.16
    // Stop Run: only the 104 print (old ID, never resting); the 103 print's order was resting
    CHECK(stopLots(*s, -1, 103) == 6 && stopLots(*s, -1, 104) == 6 && stopLots(*s, -1, 105) == 0 && stopLots(*s, -1, 100) == 6 && stopLots(*s, 1, 104) == 0);
    CHECK(L.n.stopFills == 1);
    const Bars b = sumBars(*s); CHECK(b.levels == 5 && b.zone == 1 && b.topOk && near(b.top, (6.0 / 1) / (52.0 / 4)));
    // the read
    const Params P = testParams(); Sw w; w.stamp = 1260; w.dir = -1; w.lv = "ONH"; w.lvt = 103;
    const Dem d = demOf(s->exHi, P); CHECK(d.ok && d.n == 3 && near(d.dem, (0.6 + 0.6 + 0.2) / 3.0));
    Read r = decideLive(*s, w, P, 17, true, 1.8);
    CHECK(r.src == 'L' && r.stopLots == 6 && r.stopX1000 == 300 && r.eff1000 == 1800 && r.late1e4 == 18000 && r.decay1e4 == 10000 && r.after1e4 == 1600 && r.dem1e4 == 4667 && r.exh == 2);
    CHECK(r.F == 1 && r.H == 1 && r.B == 1 && r.marks == 3 && r.ansOk == 1 && r.hard100 == 420 && r.big100 == 1250 && r.extInto == 30 && r.extLeft == 30);
    CHECK(r.verdict == V_BEST && r.why == WHY_NONE && r.odds == -1);
    // the chart's Effort missing: the feed's own
    Read r2 = decideLive(*s, w, P, 17, false, 0); CHECK(r2.eff1000 == x1000((6.0 / 1) / (52.0 / 4)));
    // no normal at that half hour: the Answer is not available -> WAIT (no upgrade), FAST still known
    Params Pn = P; Pn.n15[17] = 0; Read r3 = decideLive(*s, w, Pn, 17, true, 1.8); CHECK(r3.H == -1 && r3.B == 1 && r3.ansOk == 0 && r3.marks == 2 && r3.verdict == V_WAIT && r3.odds == 50);
    Read r3b = decideLive(*s, w, P, -1, true, 1.8); CHECK(r3b.ansOk == 0 && r3b.verdict == V_WAIT);
    // demand still strong -> STAND ASIDE with its odds
    Params Ps = P; Ps.q2 = 0.40; Ps.q1 = 0.30; Read r4 = decideLive(*s, w, Ps, 17, true, 1.8); CHECK(r4.verdict == V_ASIDE && r4.exh == 0 && r4.odds == 25);
    // one mark only -> WAIT
    Params Pw = P; Pw.hardCut = 5; Pw.bigCut = 20; Read r5 = decideLive(*s, w, Pw, 17, true, 1.8); CHECK(r5.marks == 1 && r5.verdict == V_WAIT && r5.odds == 50);
    // no rank tables -> no exhaustion value -> UNCLEAR
    Params Pe = P; Pe.rkLate.clear(); Pe.rkDecay.clear(); Pe.rkAfter.clear(); Read r6 = decideLive(*s, w, Pe, 17, true, 1.8); CHECK(r6.verdict == V_UNCLEAR && r6.why == WHY_NOEXH && r6.dem1e4 == -1 && r6.exh == -1 && r6.odds == -1);
    // BEST ODDS proven by the nightly: the odds are shown
    Params Pp = P; Pp.bestProven = true; Pp.odds[3] = 70; CHECK(decideLive(*s, w, Pp, 17, true, 1.8).odds == 70);
    // the level was never traded through on this chart (another contract): no read
    Sw far = w; far.lvt = 104; CHECK(decideLive(*s, far, P, 17, true, 1.8).src == '-'); far.lvt = 500; CHECK(decideLive(*s, far, P, 17, true, 1.8).src == '-');
    Sw other = w; other.stamp = 1320; CHECK(decideLive(*s, other, P, 17, true, 1.8).src == '-');
    // the low side of the same candle: sellers push, buyers answer (the first print at the low is the 100 buy at 1205)
    CHECK(s->lo.ok && s->lo.extPx == 100 && s->lo.extSec == 1205);
    CHECK(!s->exLo.ok);                                                // no sell before the low print: no reading
}
static void mirror() {
    // the mirrored stream must give the same readings on the other side (exactly: the same arithmetic)
    Live A, B; A.reset(60); B.reset(60); feedCandle(A, false); feedCandle(B, true);
    const Sum* a = A.find(1260); const Sum* b = B.find(1260); CHECK(a && b); if (!a || !b) return;
    CHECK(b->lo.extPx == 400 - a->hi.extPx && b->lo.extSec == a->hi.extSec && b->lo.fresh == a->hi.fresh && b->lo.hard == a->hi.hard && b->lo.big == a->hi.big);
    CHECK(b->exLo.ok && b->exLo.late == a->exHi.late && b->exLo.decay == a->exHi.decay && b->exLo.after == a->exHi.after && b->exLo.hasDecay == a->exHi.hasDecay);
    CHECK(stopLots(*b, 1, 400 - 103) == stopLots(*a, -1, 103) && stopLots(*b, 1, 400 - 104) == 6 && stopLots(*b, -1, 400 - 103) == 0);
    const Bars ba = sumBars(*a), bb = sumBars(*b); CHECK(bb.botOk == ba.topOk && bb.bot == ba.top && bb.topOk == ba.botOk);
    const Params P = testParams(); Sw wa; wa.stamp = 1260; wa.dir = -1; wa.lv = "ONH"; wa.lvt = 103; Sw wb = wa; wb.dir = 1; wb.lv = "ONL"; wb.lvt = 400 - 103;
    Read ra = decideLive(*a, wa, P, 17, false, 0), rb = decideLive(*b, wb, P, 17, false, 0);
    ra.dir = rb.dir = 0; ra.lv = rb.lv = "X"; ra.lvt = rb.lvt = 1; CHECK(readText(ra, '|') == readText(rb, '|'));
    // a random candle: 3,000 events, both ways
    Live C, D; C.reset(60); D.reset(60); unsigned long long z = 88172645463325252ULL;
    auto rnd = [&]() { z ^= z << 13; z ^= z >> 7; z ^= z << 17; return z; };
    long long px = 5000, id = 100000; C.onEvent(1000, 3, 1, px, 1, 1, id); D.onEvent(1000, 3, 0, 10000 - px, 1, 1, id);
    for (int i = 0; i < 3000; ++i) {
        const long long sec = 1200 + (long long)i * 60 / 3000; const int kind = (int)(rnd() % 10);
        if (kind < 2) { const long long o = ++id, p = px + (long long)(rnd() % 21) - 10; C.onEvent(sec, 0, (int)(rnd() % 2), p, 3, o, 0); D.onEvent(sec, 0, 1, 10000 - p, 3, o, 0); continue; }
        px += (long long)(rnd() % 5) - 2; const int bs = (int)(rnd() % 20) == 0 ? 2 : (int)(rnd() % 2); const long long sz = 1 + (long long)(rnd() % 9);
        const long long aid = (rnd() % 4 == 0) ? id - (long long)(rnd() % 3000) : ++id;       // some old order IDs (stops), some new
        C.onEvent(sec, 3, bs, px, sz, 7, bs == 2 ? 0 : aid); D.onEvent(sec, 3, bs == 2 ? 2 : 1 - bs, 10000 - px, sz, 7, bs == 2 ? 0 : aid);
    }
    C.onEvent(1265, 3, 1, px, 1, 7, ++id); D.onEvent(1265, 3, 0, 10000 - px, 1, 7, id);
    const Sum* c = C.find(1260); const Sum* d = D.find(1260); CHECK(c && d); if (!c || !d) return;
    CHECK(c->complete && d->complete && c->stopOk && d->stopOk && c->fills == d->fills && c->lots == d->lots && c->hiPx == 10000 - d->loPx && c->loPx == 10000 - d->hiPx);
    CHECK(c->hi.extPx == 10000 - d->lo.extPx && c->hi.extSec == d->lo.extSec && c->hi.hard == d->lo.hard && c->hi.big == d->lo.big && c->hi.fresh == d->lo.fresh);
    CHECK(c->lo.extPx == 10000 - d->hi.extPx && c->lo.hard == d->hi.hard && c->lo.big == d->hi.big && c->lo.fresh == d->hi.fresh);
    CHECK(c->exHi.ok == d->exLo.ok && c->exHi.late == d->exLo.late && c->exHi.decay == d->exLo.decay && c->exHi.after == d->exLo.after && c->exHi.hasDecay == d->exLo.hasDecay);
    CHECK(c->exLo.ok == d->exHi.ok && c->exLo.late == d->exHi.late && c->exLo.decay == d->exHi.decay && c->exLo.after == d->exHi.after);
    for (long long lv = c->loPx; lv <= c->hiPx; ++lv) { CHECK(stopLots(*c, -1, lv) == stopLots(*d, 1, 10000 - lv)); CHECK(stopLots(*c, 1, lv) == stopLots(*d, -1, 10000 - lv)); }
    CHECK(stopLots(*c, -1, c->loPx) > 0 && C.n.stopFills > 0);        // the stream does hold old orders
}
static std::string dump(const Sum& s) {
    char b[512]; std::snprintf(b, sizeof b, "%lld %d %d %d %lld %lld %lld %lld %d %lld %lld %.17g %.17g %.17g %.17g %zu", s.key, (int)s.complete, (int)s.stopOk, (int)s.thin, s.hi.extPx, s.hi.extSec, s.hi.hard, s.hi.big,
        (int)s.hi.fresh, s.lo.hard, s.lo.big, s.exHi.late, s.exHi.decay, s.exHi.after, s.exLo.late, s.cells.size());
    std::string o = b; for (const auto& c : s.cells) o += " " + std::to_string(c.first) + ":" + std::to_string(c.second.buy) + "/" + std::to_string(c.second.sell) + "/" + std::to_string(c.second.stopB) + "/" + std::to_string(c.second.stopS);
    return o;
}
static void noLookAhead() {
    Live L; L.reset(60); feedCandle(L, false);
    const Sum* s = L.find(1260); CHECK(s != nullptr); if (!s) return; const std::string before = dump(*s);
    // a wild next candle and a third one: the closed candle's values never move
    for (int i = 0; i < 500; ++i) L.onEvent(1262 + i % 50, 3, i % 2, 90 + i % 40, 50, 30, 5000 + i);
    L.onEvent(1330, 3, 1, 150, 9, 30, 9000); L.closeThrough(1380);
    const Sum* s2 = L.find(1260); CHECK(s2 != nullptr && dump(*s2) == before);
    CHECK(L.find(1320) != nullptr && L.find(1380) != nullptr);
    // an event stamped inside the closed candle arriving late: that candle is no longer whole (never silently changed)
    L.onEvent(1259, 3, 1, 104, 5, 30, 9500); const Sum* s3 = L.find(1260); CHECK(s3 && !s3->complete && !s3->stopOk && L.n.late == 1);
    Sw w; w.stamp = 1260; w.dir = -1; w.lv = "ONH"; w.lvt = 103; if (s3) CHECK(decideLive(*s3, w, testParams(), 17, true, 1.0).src == '-');
}
static void clockAndCaps() {
    // the ID clock: ages, the 32-bit wrap, a garbage jump, the 60-s rule for Stop Run
    IdClock c; CHECK(c.age(5, 100) == -1 && c.since == -1);
    c.note(1000, 100); c.note(1500, 105); c.note(1400, 106); c.note(2000, 110);
    CHECK(c.since == 100 && c.age(900, 200) == 100 && c.age(1000, 200) == 100 && c.age(1001, 200) == 95 && c.age(1500, 200) == 95 && c.age(1501, 200) == 90 && c.age(2500, 200) == 0);
    c.note(2000 + (1LL << 30), 111); CHECK(c.maxId == 2000 && c.jumps == 1);                  // one garbage ID: ignored
    for (int i = 0; i < 8; ++i) { c.note(2000 + (1LL << 30) + i, 112 + i); }
    CHECK(c.maxId > 2000 && c.since >= 112);   // 8 in a row: believed, the clock restarts
    IdClock w; w.note(0xFFFFFFF0LL, 10); const long long u = w.unwrap(5); CHECK(u == 0x100000005LL); w.note(u, 11); CHECK(w.age(u, 20) == 9 && w.unwrap(0xFFFFFFF1LL) == 0xFFFFFFF1LL);
    for (int i = 0; i < 10000; ++i) { w.note(u + 1 + i, 12 + i); }
    CHECK(w.pts.size() == CLOCK_CAP);
    c.clear(); CHECK(c.pts.empty() && c.since == -1 && !c.have);
    // loaded 30 s before the candle: complete but no Stop Run yet; the next candle has it
    { Live L; L.reset(60); L.onEvent(1170, 3, 1, 100, 1, 1, 900);
      for (int i = 0; i < 6; ++i) L.onEvent(1201 + i, 3, 1, 100 + i, 2, 2, 950 + i);
      for (int i = 0; i < 6; ++i) L.onEvent(1261 + i, 3, 1, 100 + i, 2, 2, 800);
      L.onEvent(1321, 3, 1, 100, 1, 2, 990);
      const Sum* a = L.find(1260); const Sum* b = L.find(1320); CHECK(a && a->complete && !a->stopOk); CHECK(b && b->complete && b->stopOk && stopLots(*b, -1, 100) == 12);
      Sw w2; w2.stamp = 1260; w2.dir = -1; w2.lv = "ONH"; w2.lvt = 102; if (a) { const Read r = decideLive(*a, w2, testParams(), 17, true, 1.0); CHECK(r.src == 'L' && r.stopLots == -1 && r.stopX1000 == -1); }
      const std::vector<Level> lv = { Level{ 102, "ONH", true, false } };
      if (a) { const Cand ca = candOf(1260, lv, 100, 105, 100, a, testParams()); CHECK(ca.hiL == 102 && ca.hiStop == -1 && ca.hiX1000 == -1); }
      if (b) { const Cand cb = candOf(1320, lv, 100, 105, 100, b, testParams()); CHECK(cb.hiL == 102 && cb.hiStop == 8 && cb.hiX1000 == 400 && cb.loL == 0); }
      if (b) { const Cand cn = candOf(1320, lv, 100, 105, 100, nullptr, testParams()); CHECK(cn.hiL == 102 && cn.hiStop == -1); const Cand op = candOf(1320, lv, 100, 105, 104, b, testParams()); CHECK(op.hiL == 0 && op.loL == 0); const Cand no = candOf(1320, lv, 103, 105, 103, b, testParams()); CHECK(no.hiL == 0 && no.loL == 0); } }
    // the first candle after the load is never complete; a gap drops the open candle and the clock
    { Live L; L.reset(60); for (int i = 0; i < 6; ++i) L.onEvent(1201 + i, 3, 1, 100 + i, 2, 2, 950 + i); L.onEvent(1261, 3, 1, 100, 1, 2, 990);
      const Sum* a = L.find(1260); CHECK(a && !a->complete && !a->stopOk);
      L.gap(); CHECK(L.openFills() == 0 && L.clockPoints() == 0 && L.orders() == 0 && L.clockSince() == -1);
      L.onEvent(1290, 3, 1, 100, 1, 2, 991); L.onEvent(1321, 3, 1, 100, 1, 2, 992); const Sum* b = L.find(1320); CHECK(b && !b->complete); }
    // a thin candle (under 5 prints): no exhaustion reading, UNCLEAR
    { Live L; L.reset(60); L.onEvent(1100, 3, 1, 100, 1, 1, 900); L.onEvent(1205, 3, 1, 100, 5, 2, 901); L.onEvent(1225, 3, 1, 104, 5, 2, 902); L.onEvent(1228, 3, 0, 101, 5, 2, 903); L.onEvent(1261, 3, 1, 100, 1, 2, 990);
      const Sum* a = L.find(1260); CHECK(a && a->complete && a->thin && !a->exHi.ok);
      Sw w2; w2.stamp = 1260; w2.dir = -1; w2.lv = "ONH"; w2.lvt = 102; if (a) { const Read r = decideLive(*a, w2, testParams(), 17, true, 1.0); CHECK(r.src == 'L' && r.verdict == V_UNCLEAR && r.why == WHY_NOEXH); } }
    // the extreme in the first 20 s / the last 30 s: UNCLEAR whatever the bars say
    { Live L; L.reset(60); L.onEvent(1100, 3, 1, 100, 1, 1, 900);
      for (int i = 0; i < 8; ++i) L.onEvent(1201 + i, 3, 1, 100 + i, 2, 2, 950 + i);          // the high at 1208: 8 s into the candle
      for (int i = 0; i < 8; ++i) L.onEvent(1215 + i, 3, 0, 106 - i, 2, 2, 960 + i);
      for (int i = 0; i < 8; ++i) L.onEvent(1261 + i * 4, 3, 1, 100 + i, 2, 2, 970 + i);       // next candle: the high at 1289 = 31 s before the close
      for (int i = 0; i < 4; ++i) L.onEvent(1291 + i, 3, 1, 108 + i, 2, 2, 980 + i);           // ... no: a higher high at 1294 = 26 s before the close
      L.onEvent(1321, 3, 1, 100, 1, 2, 995);
      const Sum* a = L.find(1260); const Sum* b = L.find(1320); CHECK(a && b); Sw w2; w2.dir = -1; w2.lv = "ONH"; w2.lvt = 102;
      if (a) { w2.stamp = 1260; const Read r = decideLive(*a, w2, testParams(), 17, true, 1.0); CHECK(r.extInto == 8 && r.verdict == V_UNCLEAR && r.why == WHY_EARLY && r.odds == -1 && r.dem1e4 >= 0); }
      if (b) { w2.stamp = 1320; const Read r = decideLive(*b, w2, testParams(), 17, true, 1.0); CHECK(r.extLeft == 26 && r.verdict == V_UNCLEAR && r.why == WHY_LATE); } }
    // too many fills in one candle: not decided live, memory stays bounded
    { Live L; L.reset(60); L.onEvent(1100, 3, 1, 100, 1, 1, 900);
      for (size_t i = 0; i < FILL_CAP + 50; ++i) L.onEvent(1201 + (long long)(i % 50), 3, 1, 100 + (long long)(i % 7), 1, 2, 1000 + (long long)i);
      CHECK(L.openFills() == FILL_CAP); L.onEvent(1261, 3, 1, 100, 1, 2, 5);
      const Sum* a = L.find(1260); CHECK(a && a->overflow && !a->complete && !a->stopOk && L.n.overflows == 1 && L.openFills() == 1); }
    // bad / future events are counted, never stored; self-fills are not trades
    { Live L; L.reset(60); L.setLimit(1260); L.onEvent(0, 3, 1, 100, 1, 1, 2); L.onEvent(1201, 9, 1, 100, 1, 1, 2); L.onEvent(1201, 3, 1, -5, 1, 1, 2); CHECK(L.n.bad == 3);
      L.onEvent(1201, 3, 1, 100, 1, 1, 2); L.onEvent(1500, 3, 1, 100, 1, 1, 3); CHECK(L.n.future == 1 && L.forming() == 1260);
      L.onEvent(1202, 3, 1, 100, 1, 7, 7); CHECK(L.n.bad == 4 && L.openFills() == 1); }
    // resting orders: tracked near the last trade only, capped
    { Live L; L.reset(60); L.onEvent(1201, 3, 1, 1000, 1, 1, 2);
      L.onEvent(1201, 0, 1, 1000 + NEAR_TICKS, 1, 50, 0); L.onEvent(1201, 0, 1, 1000 + NEAR_TICKS + 1, 1, 51, 0); CHECK(L.orders() == 1);
      L.onEvent(1201, 1, 1, 1000 + NEAR_TICKS + 5, 1, 50, 0); CHECK(L.orders() == 0);            // modified away: dropped
      L.onEvent(1201, 0, 1, 1001, 1, 52, 0); L.onEvent(1201, 2, 1, 1001, 1, 52, 0); CHECK(L.orders() == 0);   // deleted
      for (size_t i = 0; i < SHOWN_CAP + 500; ++i) L.onEvent(1202, 0, 1, 1000 + (long long)(i % 60), 1, 100 + (long long)i, 0);
      CHECK(L.orders() <= SHOWN_CAP); }
    // candle sums kept: SUMS_KEEP
    { Live L; L.reset(60); for (int i = 0; i < 40; ++i) L.onEvent(1201 + 60 * i, 3, 1, 100, 1, 1, 2 + i); CHECK(L.sums.size() == SUMS_KEEP && L.sums.back().key == 1260 + 60 * 38); }
}
static void record() {
    Rec r; Read a; CHECK(readParse(split(sampleRead(3878010180LL, 'L'), ','), 0, a)); a.lv = "ONH"; r.reads[a.stamp] = a;
    Read u; u.stamp = 3878009460LL; u.dir = 1; u.lv = "PDL"; u.lvt = 28768; u.src = 'L'; u.verdict = V_UNCLEAR; u.why = WHY_EARLY; u.F = 0; u.marks = 0; u.extInto = 6; u.extLeft = 174; r.reads[u.stamp] = u;
    Cand c; c.key = 3878010360LL; c.hiL = 28797; c.hiStop = 15; c.hiX1000 = 698; r.cands[c.key] = c;
    Cand c2; c2.key = 3878010540LL; c2.loL = 28768; r.cands[c2.key] = c2;
    const std::string t = recFileText(r, "ES", 180);
    CHECK(t.find("TRN|1|ES|180\n") == 0 && t.find("T|3878010180|-1|ONH|28797|112|5209|1800|8224|50538|8818|4667|2|1|1|0|2|1|420|1250|30|150|3|0|-1|L\n") != std::string::npos);
    CHECK(t.find("C|3878010360|28797|15|698|0|-1|-1\n") != std::string::npos && t.find("C|3878010540|0|-1|-1|28768|-1|-1\n") != std::string::npos);
    Rec back; CHECK(parseRecFile(t, "ES", 180, back) && back.reads.size() == 2 && back.cands.size() == 2 && recFileText(back, "ES", 180) == t);   // byte-identical round trip
    CHECK(back.reads[3878009460LL].verdict == V_UNCLEAR && back.reads[3878009460LL].stopLots == -1 && back.cands[3878010360LL].hiX1000 == 698);
    Rec x; CHECK(!parseRecFile(t, "NQ", 180, x) && x.reads.empty()); CHECK(!parseRecFile(t, "ES", 60, x)); CHECK(!parseRecFile("", "ES", 180, x)); CHECK(!parseRecFile(std::string(REC_MAX_BYTES + 1, 'T'), "ES", 180, x));
    // garbled / hostile lines are skipped one by one; a torn last line is dropped; a nightly-marked line is not a live record
    const std::string g = "TRN|1|ES|180\nT|3878010180|-1|ONH|28797|112\nT|x|-1|ONH|28797|112|5209|1800|8224|50538|8818|4667|2|1|1|0|2|1|420|1250|30|150|3|0|-1|L\n"
                          "T|3878010180|-1|ONH|28797|112|5209|1800|8224|50538|8818|4667|9|1|1|0|2|1|420|1250|30|150|3|0|-1|L\nT|3878010181|-1|ONH|28797|112|5209|1800|8224|50538|8818|4667|2|1|1|0|2|1|420|1250|30|150|3|0|-1|N\n"
                          "C|5|0|-1|-1|0|-1|-1\nC|5|a|1|1|1|1|1\nC|6|10|-2|0|0|-1|-1\nZ|whatever\n" + std::string("T|3878010182|-1|ON H|28797|112|5209|1800|8224|50538|8818|4667|2|1|1|0|2|1|420|1250|30|150|3|0|-1|L\n")
                          + "C|3878010360|28797|15|698|0|-1|-1\nT|3878010900|-1|ONH|28797|112|5209|1800|8224|50538|8818|4667|2|1|1|0|2|1|420|1250|30|150|3|0|-1|L";
    CHECK(parseRecFile(g, "ES", 180, x) && x.reads.empty() && x.cands.size() == 1);
    // bounded: the newest REC_KEEP reads and CAND_KEEP candles
    Rec m; for (size_t i = 0; i < REC_KEEP + 40; ++i) { Read q = a; q.stamp = 1000 + (long long)i * 180; m.reads[q.stamp] = q; }
    for (size_t i = 0; i < CAND_KEEP + 40; ++i) { Cand q = c; q.key = 1000 + (long long)i * 180; m.cands[q.key] = q; }
    const std::string mt = recFileText(m, "ES", 180); CHECK(mt.size() < REC_MAX_BYTES); Rec mb; CHECK(parseRecFile(mt, "ES", 180, mb) && mb.reads.size() == REC_KEEP && mb.cands.size() == CAND_KEEP);
    CHECK(mb.reads.begin()->first == 1000 + 40 * 180 && mb.cands.begin()->first == 1000 + 40 * 180);
    trimRec(m); CHECK(m.reads.size() == REC_KEEP && m.cands.size() == CAND_KEEP);
    // every field at its limits survives
    Read e; e.stamp = 1; e.dir = 1; e.lv = "A"; e.lvt = 1; e.src = '-'; Read e2; CHECK(readParse(split(readText(e, '|'), '|'), 0, e2) && readText(e2, '|') == readText(e, '|'));
    CHECK(!readParse(split("1|1|A", '|'), 0, e2)); CHECK(!nameOk("") && !nameOk("A|B") && !nameOk("A,B") && nameOk("ONL/PHL") && !nameOk(std::string(33, 'A')));
}
static void drawing() {
    Read a; CHECK(readParse(split(sampleRead(3878010180LL, 'L'), ','), 0, a));
    const BarSet bs = barsOf(a);
    CHECK(bs.stop.show && bs.stop.on && bs.stop.label == "5.2" && near(bs.stop.v, 5.209));
    CHECK(bs.eff.show && bs.eff.on && bs.eff.label == "1.8");
    CHECK(bs.exh.show && bs.exh.on && bs.exh.label == "1.1" && near(bs.exh.v, (1 - 0.4667) / 0.5));
    CHECK(bs.ans.show && bs.ans.on && bs.ans.label == "2/3" && near(bs.ans.v, 2.0));
    Strip s = stripOf(a, false, false, "ONH 9:03 AM");
    CHECK(s.flat == "ONH 9:03 AM   Heavy Stop Run / Heavy Effort / Moderate Exhaustion / Moderate Answer   \xBB BEST ODDS ?");
    CHECK(s.pill == "?" && s.pillTone == T_DIR && s.edgeTone == T_DIR && s.segs.size() == 10 && s.segs[1].tone == T_TEXT && s.segs[5].tone == T_DIR && s.segs[7].tone == T_DIR && s.segs[9].tone == T_DIR && s.segs[9].bold);
    CHECK(stripOf(a, false, true, "").flat == "Heavy Stop Run 5.2x / Heavy Effort 1.8x / Moderate Exhaustion 1.1x / Moderate Answer 2/3   \xBB BEST ODDS ?");
    Strip sp = stripOf(a, true, false, ""); CHECK(sp.flat.find("BEST ODDS ?") == std::string::npos && sp.flat.find("BEST ODDS") != std::string::npos && sp.pill == "?");   // proven but no odds number: still "?"
    a.odds = 70; CHECK(stripOf(a, true, false, "").pill == "70% turn" && stripOf(a, false, false, "").pill == "?");
    Read w = a; w.verdict = V_WAIT; w.marks = 1; w.odds = 52; Strip sw = stripOf(w, false, false, "");
    CHECK(sw.flat == "Heavy Stop Run / Heavy Effort / Moderate Exhaustion / Light Answer   \xBB WAIT" && sw.pill == "52% turn" && sw.pillTone == T_AMBER && sw.edgeTone == T_AMBER);
    Read n = a; n.verdict = V_ASIDE; n.exh = 0; n.dem1e4 = 7000; n.marks = 0; n.odds = 26; n.stopX1000 = 300; n.eff1000 = 400; Strip sn = stripOf(n, false, false, "");
    CHECK(sn.flat == "No Stop Run / No Effort / No Exhaustion / No Answer   \xBB STAND ASIDE" && sn.pill == "26% turn" && sn.pillTone == T_MUTED);
    for (const Seg& g : sn.segs) CHECK(g.tone == T_MUTED);              // everything that reads No is grey
    const BarSet bn = barsOf(n); CHECK(bn.stop.show && !bn.stop.on && bn.eff.show && !bn.eff.on && bn.exh.show && !bn.exh.on && near(bn.exh.v, 0.6) && bn.ans.show && !bn.ans.on && near(bn.ans.v, 0.06) && bn.ans.label == "0/3");
    Read uu = a; uu.verdict = V_UNCLEAR; uu.why = WHY_EARLY; uu.odds = -1; Strip su = stripOf(uu, false, false, "");
    CHECK(su.flat == "Heavy Stop Run / Heavy Effort / Moderate Exhaustion ? / Moderate Answer   \xBB UNCLEAR" && su.pill == "\x96" && su.pillTone == T_MUTED);
    // nothing known but the chart's Effort (a sweep from before the pane was loaded)
    Read z; z.stamp = 5; z.dir = 1; z.lv = "PDL"; z.lvt = 100; z.eff1000 = 1200; Strip sz = stripOf(z, false, false, "PDL 9:03 AM");
    CHECK(sz.flat == "PDL 9:03 AM   \x96 Stop Run / Moderate Effort / \x96 Exhaustion / \x96 Answer   \xBB NO READ" && sz.pill == "\x96");
    const BarSet bz = barsOf(z); CHECK(!bz.stop.show && bz.eff.show && !bz.exh.show && !bz.ans.show);
    // a Stop Run under 0.05x is not drawn; the Answer without its normals is not drawn
    Read t = a; t.stopX1000 = 49; t.ansOk = 0; const BarSet bt = barsOf(t); CHECK(!bt.stop.show && !bt.ans.show); CHECK(stripOf(t, false, false, "").flat.find("\x96 Answer") != std::string::npos);
    CHECK(ratioBar(-1, 0).show == false && ratioBar(0, 0).show && !ratioBar(0, 0).on && ratioBar(500, 0).on);
}
// (1.0.1) the ratio on a bar, the strip of a candle at a named level, and which labels narrow candles keep
static void mouseAndLabels() {
    CHECK(std::string(VERSION) == "1.1.0");
    // the ratio ON a bar: no x, one decimal under 10, none from 10 up, 99+ cap; the strip's ratioText is unchanged
    CHECK(labelText(0) == "0.0" && labelText(0.522) == "0.5" && labelText(2.435) == "2.4" && labelText(2.87) == "2.9" && labelText(9.94) == "9.9" && labelText(9.95) == "10" && labelText(13.652) == "14");
    CHECK(labelText(99.4) == "99" && labelText(99.5) == "99+" && labelText(1e9) == "99+" && labelText(-1).empty() && labelText(std::nan("")).empty() && labelText(1.0 / 0.0).empty());
    CHECK(ratioText(2.87) == "2.9x" && ratioText(13.652) == "13.7x");
    for (double v = 0; v < 120; v += 0.013) { const std::string t = labelText(v); CHECK(!t.empty() && t.size() <= 3 && t.find('x') == std::string::npos); }   // never wider than three characters
    // the level's name
    std::vector<Level> lv; { Level a; a.tick = 8328; a.name = "VAH"; a.hi = true; lv.push_back(a); Level b; b.tick = 8359; b.name = "PDH"; b.hi = true; lv.push_back(b); Level c; c.tick = 8078; c.name = "PFL/PWL"; c.lo = true; lv.push_back(c); }
    CHECK(levelNameAt(lv, 8328) == "VAH" && levelNameAt(lv, 8359) == "PDH" && levelNameAt(lv, 8078) == "PFL/PWL" && levelNameAt(lv, 8329).empty() && levelNameAt(lv, 0).empty() && levelNameAt(lv, -5).empty() && levelNameAt(std::vector<Level>(), 8328).empty());
    // which side of the candle: the one it has; both: the side the mouse is on
    { Cand h; h.hiL = 8328; Cand l; l.loL = 8078; Cand b; b.hiL = 8328; b.loL = 8078; Cand n;
      CHECK(candSide(h, true) == -1 && candSide(h, false) == -1 && candSide(l, true) == 1 && candSide(l, false) == 1 && candSide(b, true) == -1 && candSide(b, false) == 1 && candSide(n, true) == 0 && candSide(n, false) == 0); }
    // the strip of a candle at a level: his CL chart 2026-11-22 (VAH / PDH, the record's Stop Run and the Effort read off his screenshot)
    { const Strip a = stripOfCand(2435, 160, true, "VAH 9:36 PM"); CHECK(a.flat == "VAH 9:36 PM   Moderate Stop Run 2.4x / No Effort 0.2x" && a.pill == "\x96" && a.pillTone == T_MUTED && a.edgeTone == T_MUTED);
      CHECK(a.segs.size() == 4 && a.segs[0].tone == T_MUTED && !a.segs[0].bold && a.segs[1].tone == T_TEXT && a.segs[1].bold && a.segs[2].tone == T_MUTED && a.segs[3].tone == T_MUTED && a.segs[3].bold);   // what reads No is grey
      CHECK(stripOfCand(13652, 440, true, "VAH 9:57 PM").flat == "VAH 9:57 PM   Heavy Stop Run 13.7x / No Effort 0.4x");
      CHECK(stripOfCand(2870, 2780, true, "VAH 10:00 PM").flat == "VAH 10:00 PM   Moderate Stop Run 2.9x / Heavy Effort 2.8x");
      CHECK(stripOfCand(522, 1640, true, "PDH 10:12 PM").flat == "PDH 10:12 PM   Light Stop Run 0.5x / Heavy Effort 1.6x");
      CHECK(stripOfCand(522, 1640, false, "PDH 10:12 PM").flat == "PDH 10:12 PM   Light Stop Run / Heavy Effort");                 // wide candles: the bars carry the numbers
      CHECK(stripOfCand(522, 1640, false, "").flat == "Light Stop Run / Heavy Effort" && stripOfCand(522, 1640, false, "").segs.size() == 3);
      CHECK(stripOfCand(-1, -1, true, "PDL 9:00 AM").flat == "PDL 9:00 AM   \x96 Stop Run / \x96 Effort");                       // not known: a dash, never a number
      CHECK(stripOfCand(0, 0, true, "").flat == "No Stop Run 0.0x / No Effort 0.0x");
      for (const Strip& q : { stripOfCand(2435, 160, true, "VAH"), stripOfCand(-1, 900, false, "") }) { CHECK(q.flat.find("Exhaustion") == std::string::npos && q.flat.find("Answer") == std::string::npos && q.flat.find("\xBB") == std::string::npos);
          std::string f; for (const Seg& g : q.segs) { CHECK(g.tone == T_TEXT || g.tone == T_MUTED); f += g.text; } CHECK(f == q.flat); } }   // never a turn colour, never a verdict
    // which bar of a candle side carries the label
    CHECK(tallerOf(10, 11) == 1 && tallerOf(11, 10) == 0 && tallerOf(10, 10) == 0 && tallerOf(0, 0) == 0);
    // labels: boxes are [l, r) x [t, b)
    { LabBox a; a.l = 0; a.r = 10; a.t = 0; a.b = 10; LabBox b = a; b.l = 10; b.r = 20; CHECK(!boxesTouch(a, b, 0) && boxesTouch(a, b, 1) && boxesTouch(b, a, 1)); b.l = 9; CHECK(boxesTouch(a, b, 0));
      LabBox c = a; c.t = 10; c.b = 20; CHECK(!boxesTouch(a, c, 0) && !boxesTouch(a, c, 5)); }
    const auto bar = [](int x, int w, int h, int axis, int owner) { BarBox b; b.owner = owner; b.box.l = x; b.box.r = x + w; b.box.t = axis - h; b.box.b = axis; return b; };
    const auto lab = [](int cx, int lw, int h, int axis, int owner) { LabCand c; c.owner = owner; c.h = h; c.box.l = cx - lw / 2; c.box.r = c.box.l + lw; c.box.t = axis - h - 12; c.box.b = axis - h - 1; return c; };
    // his CL chart at 22 px a candle, axis 87, half 51: bars 9 px wide left / right of the centre. Labels of 9 pt without the x are 18 px:
    // each stays inside its own candle, all four are kept (the mockup after "get rid of the x and make it smaller")
    { const int ax = 87; std::vector<BarBox> bars; std::vector<LabCand> c; const int X[4] = { 121, 275, 297, 385 }, HS[4] = { 41, 51, 49, 9 }, HE[4] = { 3, 7, 47, 28 };
      for (int i = 0; i < 4; ++i) { bars.push_back(bar(X[i] - 10, 9, HS[i], ax, i)); bars.push_back(bar(X[i] + 1, 9, HE[i], ax, i)); c.push_back(lab(X[i], 18, tallerOf(HS[i], HE[i]) ? HE[i] : HS[i], ax, i)); }
      const std::vector<size_t> k = pickLabels(c, bars); CHECK(k.size() == 4 && k[0] == 0 && k[1] == 1 && k[2] == 2 && k[3] == 3);
      // with the 10 pt "13.7x" / "2.9x" labels of the first mockup (33 / 26 px) the two neighbours collide: the taller bar (9:57) keeps its label
      std::vector<LabCand> w; const int LW[4] = { 26, 33, 26, 26 }; for (int i = 0; i < 4; ++i) w.push_back(lab(X[i], LW[i], tallerOf(HS[i], HE[i]) ? HE[i] : HS[i], ax, i));
      const std::vector<size_t> kw = pickLabels(w, bars); CHECK(kw.size() == 3 && kw[0] == 0 && kw[1] == 1 && kw[2] == 3); }
    // a label is never drawn over a bar of ANOTHER candle: the short candle next to a tall one loses its label, the tall one keeps it
    { const int ax = 100; std::vector<BarBox> bars; bars.push_back(bar(40, 9, 60, ax, 0)); bars.push_back(bar(62, 9, 20, ax, 1));
      std::vector<LabCand> c; c.push_back(lab(45, 18, 60, ax, 0)); c.push_back(lab(55, 30, 20, ax, 1));       // candle 1's label box reaches under candle 0's bar top
      const std::vector<size_t> k = pickLabels(c, bars); CHECK(k.size() == 1 && k[0] == 0);
      std::vector<LabCand> own; own.push_back(lab(45, 18, 30, ax, 0)); CHECK(pickLabels(own, bars).size() == 1);   // its own bars never block it (here: a label inside its own taller bar's column)
      std::vector<LabCand> other = own; other[0].owner = 7; CHECK(pickLabels(other, bars).empty()); }
    // equal heights: the newer candle is asked first; an empty box is never kept; no candidates, no labels
    { const int ax = 100; std::vector<BarBox> none; std::vector<LabCand> c; c.push_back(lab(50, 30, 40, ax, 0)); c.push_back(lab(60, 30, 40, ax, 1));
      const std::vector<size_t> k = pickLabels(c, none); CHECK(k.size() == 1 && k[0] == 1);
      LabCand e = lab(200, 0, 40, ax, 2); c.push_back(e); CHECK(pickLabels(c, none).size() == 1); CHECK(pickLabels(std::vector<LabCand>(), none).empty()); }
    // property (20,000 random panes): kept labels touch no kept label and no foreign bar; every label left out touches a kept TALLER-or-equal
    // label or a foreign bar (nothing is dropped without a reason); the answer does not depend on the order the candidates come in
    std::mt19937 rng(20261122);
    for (int rep = 0; rep < 20000; ++rep) {
        const int ax = 150, n = 1 + (int)(rng() % 12), bw = 3 + (int)(rng() % 40), w = std::max(1, std::min((bw - 4) / 2, 20)), lw = 14 + (int)(rng() % 22);
        std::vector<BarBox> bars; std::vector<LabCand> c; int x = 30;
        for (int i = 0; i < n; ++i) { x += bw * (1 + (int)(rng() % 3)); const int hs = 2 + (int)(rng() % 120), he = 2 + (int)(rng() % 120);
            bars.push_back(bar(x - 1 - w, w, hs, ax, i)); bars.push_back(bar(x + 1, w, he, ax, i)); c.push_back(lab(x, lw, tallerOf(hs, he) ? he : hs, ax, i)); }
        const std::vector<size_t> k = pickLabels(c, bars); std::vector<bool> in(c.size(), false); for (size_t q : k) in[q] = true;
        for (size_t a = 0; a < k.size(); ++a) { CHECK(a == 0 || k[a - 1] < k[a]);
            for (size_t b = a + 1; b < k.size(); ++b) CHECK(!boxesTouch(c[k[a]].box, c[k[b]].box, 1));
            for (const BarBox& bb : bars) CHECK(bb.owner == c[k[a]].owner || !boxesTouch(c[k[a]].box, bb.box, 0)); }
        for (size_t a = 0; a < c.size(); ++a) { if (in[a]) continue; bool why = false;
            for (size_t q : k) if (boxesTouch(c[a].box, c[q].box, 1) && c[q].h >= c[a].h) why = true;
            for (const BarBox& bb : bars) if (bb.owner != c[a].owner && boxesTouch(c[a].box, bb.box, 0)) why = true;
            CHECK(why); }
        std::vector<LabCand> r(c.rbegin(), c.rend()); const std::vector<size_t> kr = pickLabels(r, bars); CHECK(kr.size() == k.size());
        for (size_t a = 0; a < kr.size(); ++a) CHECK(in[c.size() - 1 - kr[a]]);
    }
}
// (1.1.0) Effort on every candle: the two bars of a plain candle, its strip, which half of a level candle is plain, the fast label picker
static void effortEveryCandle() {
    // the two bars: buyers at the top (up), sellers at the bottom (down); the same numbers as the Effort of a candle at a level
    { Bars b; b.topOk = true; b.top = 1.957; b.botOk = true; b.bot = 0.49;                      // ES 2026-11-20 9:51 AM
      const EffPair e = effOf(b); CHECK(e.up.show && e.up.on && e.up.label == "2.0" && std::fabs(e.up.v - 1.957) < 1e-9); CHECK(e.dn.show && !e.dn.on && e.dn.label == "0.5" && std::fabs(e.dn.v - 0.49) < 1e-9);
      CHECK(e.up.v == ratioBar(x1000(b.top), 0).v && e.dn.v == ratioBar(x1000(b.bot), 0).v); }  // = the level candle's Effort bar
    { Bars b; b.topOk = true; b.top = 0.5; b.botOk = true; b.bot = 0.4996; const EffPair e = effOf(b); CHECK(e.up.on && e.up.label == "0.5" && e.dn.on && e.dn.label == "0.5"); }   // coloured from 0.5x; a ratio is kept to 3 decimals, as at a level (0.4996 = 0.500: on)
    { Bars b; b.topOk = true; b.top = 0.4994; const EffPair e = effOf(b); CHECK(e.up.show && !e.up.on && !e.dn.show); }
    { Bars b; const EffPair e = effOf(b); CHECK(!e.up.show && !e.dn.show && e.up.label.empty()); }                                   // under 4 price levels / nothing in the rest: no bar
    { Bars b; b.topOk = true; b.top = std::nan(""); b.botOk = true; b.bot = -1; const EffPair e = effOf(b); CHECK(!e.up.show && !e.dn.show); }
    { Bars b; b.topOk = true; b.top = 1.0 / 0.0; b.botOk = true; b.bot = 0; const EffPair e = effOf(b); CHECK(!e.up.show && e.dn.show && !e.dn.on && e.dn.label == "0.0"); }
    { Bars b; b.topOk = true; b.top = 250; b.botOk = true; b.bot = 12.34; const EffPair e = effOf(b); CHECK(e.up.label == "99+" && e.dn.label == "12" && barPx(e.up.v, 60) == 60 && barPx(e.dn.v, 60) == 60); }
    // from volume at price: a candle of 8 prices, the buyers heavy in its top two, the sellers thin in its bottom two
    { std::vector<Row> rows; for (long long t = 100; t <= 107; ++t) { Row r; r.tick = t; r.buy = t >= 106 ? 60 : 10; r.sell = t <= 101 ? 2 : 20; rows.push_back(r); }
      const EffPair e = effOf(barRatios(rows, 100, 107)); CHECK(e.up.show && e.up.on && e.up.label == "6.0" && e.dn.show && !e.dn.on && e.dn.label == "0.1");
      const EffPair f = effOf(barRatios(rows, 100, 102)); CHECK(!f.up.show && !f.dn.show); }     // 3 price levels: none
    // the strip of a plain candle: buyers green, sellers red, No = grey; a dash where there is no bar; never a verdict, a level or a pill
    { const Strip a = stripOfEffort(610, 1800, true, "9:27 AM"); CHECK(a.flat == "9:27 AM   Buyers Light Effort 0.6x / Sellers Heavy Effort 1.8x" && a.pill == "\x96" && a.pillTone == T_MUTED && a.edgeTone == T_MUTED);
      CHECK(a.segs.size() == 4 && a.segs[0].tone == T_MUTED && !a.segs[0].bold && a.segs[1].tone == T_BUY && a.segs[1].bold && a.segs[2].tone == T_MUTED && a.segs[3].tone == T_SELL && a.segs[3].bold);
      CHECK(stripOfEffort(610, 1800, false, "9:27 AM").flat == "9:27 AM   Buyers Light Effort / Sellers Heavy Effort");
      CHECK(stripOfEffort(1957, 490, true, "").flat == "Buyers Heavy Effort 2.0x / Sellers No Effort 0.5x" && stripOfEffort(1957, 490, true, "").segs.size() == 3 && stripOfEffort(1957, 490, true, "").segs[2].tone == T_MUTED);
      CHECK(stripOfEffort(1000, 1499, true, "").flat == "Buyers Moderate Effort 1.0x / Sellers Moderate Effort 1.5x" && stripOfEffort(1500, 999, true, "").flat == "Buyers Heavy Effort 1.5x / Sellers Light Effort 1.0x");
      CHECK(stripOfEffort(-1, 3160, true, "10:06 AM").flat == "10:06 AM   Buyers \x96 Effort / Sellers Heavy Effort 3.2x" && stripOfEffort(230, -1, true, "").flat == "Buyers No Effort 0.2x / Sellers \x96 Effort");
      for (const Strip& q : { stripOfEffort(610, 1800, true, "9:27 AM"), stripOfEffort(-1, -1, false, ""), stripOfEffort(0, 99999000, true, "x") }) {
          CHECK(q.flat.find("Stop Run") == std::string::npos && q.flat.find("Exhaustion") == std::string::npos && q.flat.find("Answer") == std::string::npos && q.flat.find("\xBB") == std::string::npos && q.flat.find("%") == std::string::npos);
          std::string f; for (const Seg& g : q.segs) { CHECK(g.tone == T_BUY || g.tone == T_SELL || g.tone == T_MUTED); f += g.text; } CHECK(f == q.flat); } }
    // a candle at a level under the mouse: the half that has a level keeps the level's strip; the half with only the other side's Effort is plain
    { Cand h; h.hiL = 8328; Cand l; l.loL = 8078; Cand b; b.hiL = 8328; b.loL = 8078; Cand n;
      CHECK(!candPlainSide(h, true) && candPlainSide(h, false) && candPlainSide(l, true) && !candPlainSide(l, false) && !candPlainSide(b, true) && !candPlainSide(b, false) && candPlainSide(n, true) && candPlainSide(n, false)); }
    // THE FAST LABEL PICKER = the plain one (1.0.1's double loop), on panes where every candle carries bars on both sides
    std::mt19937 rng(20261123); long long bigPanes = 0, keptAll = 0;
    const auto same = [](const std::vector<size_t>& a, const std::vector<size_t>& b) { return a == b; };
    for (int rep = 0; rep < 600; ++rep) {
        const int ax = 150, n = rep < 40 ? 1 + (int)(rng() % 6) : 60 + (int)(rng() % 1400), bw = 2 + (int)(rng() % 44), w = std::max(1, std::min((bw - 4) / 2, 20)), lw = 14 + (int)(rng() % 22);
        const int x0 = rep % 7 == 3 ? -5000 : 30;                                                // panes left of zero too
        std::vector<BarBox> bars; std::vector<LabCand> c; int x = x0, owner = 0;
        for (int i = 0; i < n; ++i) { x += bw * (rng() % 9 == 0 ? 2 : 1);
            for (int side = 0; side < 2; ++side) {                                               // side 0: the bars going up, 1: going down
                const bool two = rng() % 3 == 0; const int hs = two ? 2 + (int)(rng() % 70) : 0, he = rng() % 11 == 0 ? 0 : 2 + (int)(rng() % 70); if (!hs && !he) continue;
                const int h = std::max(hs, he);
                if (hs) { BarBox b; b.owner = owner; b.box.l = x - 1 - w; b.box.r = x - 1; if (side == 0) { b.box.t = ax - hs; b.box.b = ax; } else { b.box.t = ax + 1; b.box.b = ax + 1 + hs; } bars.push_back(b); }
                if (he) { BarBox b; b.owner = owner; b.box.l = x + 1; b.box.r = x + 1 + w; if (side == 0) { b.box.t = ax - he; b.box.b = ax; } else { b.box.t = ax + 1; b.box.b = ax + 1 + he; } bars.push_back(b); }
                LabCand lc; lc.owner = owner; lc.h = h; lc.box.l = x - lw / 2; lc.box.r = lc.box.l + lw; if (side == 0) { lc.box.t = ax - h - 12; lc.box.b = ax - h - 1; } else { lc.box.t = ax + 1 + h + 1; lc.box.b = ax + 1 + h + 12; } c.push_back(lc);
                ++owner; } }
        if (rep % 13 == 5 && !c.empty()) { c[rng() % c.size()].box.r = c[0].box.l - 3; BarBox inv; inv.owner = -1; inv.box.l = x0 + 400; inv.box.r = x0 + 380; inv.box.t = 0; inv.box.b = 400; bars.push_back(inv); }   // an empty label, an inverted bar
        const std::vector<size_t> a = pickLabels(c, bars), b = pickLabelsPlain(c, bars); CHECK(same(a, b));
        if (c.size() * (bars.size() + c.size()) > 4096) ++bigPanes;
        keptAll += (long long)a.size();
        std::vector<LabCand> r(c.rbegin(), c.rend()); CHECK(pickLabels(r, bars).size() == a.size());   // the order the candidates come in does not matter
    }
    CHECK(bigPanes > 400 && keptAll > 1000);
    // boxes far apart (more columns than the picker keeps): the plain loop answers, the same way
    { std::vector<BarBox> bars; std::vector<LabCand> c;
      for (int i = 0; i < 80; ++i) { LabCand lc; lc.owner = i; lc.h = 10 + i % 7; lc.box.l = (int)((long long)i * 2250000LL); lc.box.r = lc.box.l + 18; lc.box.t = 10; lc.box.b = 21; c.push_back(lc); BarBox b; b.owner = i + 1; b.box = lc.box; b.box.l += (i % 2) * 40; b.box.r += (i % 2) * 40; bars.push_back(b); }
      const std::vector<size_t> a = pickLabels(c, bars); CHECK(a == pickLabelsPlain(c, bars) && a.size() == 40); }
    // a pane of 1,500 candles with bars on both sides: one pick stays far under a millisecond's worth of work per candle
    { const int ax = 150; std::vector<BarBox> bars; std::vector<LabCand> c;
      for (int i = 0; i < 1500; ++i) for (int side = 0; side < 2; ++side) { const int x = 20 + i * 3, h = 2 + (i * 37 + side * 11) % 70; BarBox b; b.owner = i * 2 + side; b.box.l = x; b.box.r = x + 1; b.box.t = side ? ax + 1 : ax - h; b.box.b = side ? ax + 1 + h : ax; bars.push_back(b);
          LabCand lc; lc.owner = i * 2 + side; lc.h = h; lc.box.l = x - 9; lc.box.r = x + 9; lc.box.t = side ? ax + 2 + h : ax - h - 12; lc.box.b = lc.box.t + 11; c.push_back(lc); }
      const std::clock_t t0 = std::clock(); std::vector<size_t> a; for (int k = 0; k < 20; ++k) a = pickLabels(c, bars); const double ms = 1000.0 * (double)(std::clock() - t0) / CLOCKS_PER_SEC / 20;
      const std::clock_t t1 = std::clock(); const std::vector<size_t> b = pickLabelsPlain(c, bars); const double msPlain = 1000.0 * (double)(std::clock() - t1) / CLOCKS_PER_SEC;
      CHECK(a == b); CHECK(ms < 60.0); std::printf("pickLabels, 1,500 candles x 2 sides (3,000 labels asked, %zu kept): %.2f ms (the plain loop %.1f ms)\n", a.size(), ms, msPlain); }
}
int main() {
    effort(); words(); wy(); params(); readings(); mirror(); noLookAhead(); clockAndCaps(); record(); drawing(); mouseAndLabels(); effortEveryCandle();
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
