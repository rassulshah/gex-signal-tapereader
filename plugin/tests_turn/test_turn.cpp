// tests_turn/test_turn.cpp -- unit tests of TurnLogic.h (lsTurn 1.0.0). g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror
#include "../TurnLogic.h"
#include <cassert>
#include <cstdio>
#include <iostream>
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
    CHECK(bs.stop.show && bs.stop.on && bs.stop.label == "5.2x" && near(bs.stop.v, 5.209));
    CHECK(bs.eff.show && bs.eff.on && bs.eff.label == "1.8x");
    CHECK(bs.exh.show && bs.exh.on && bs.exh.label == "1.1x" && near(bs.exh.v, (1 - 0.4667) / 0.5));
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
int main() {
    effort(); words(); wy(); params(); readings(); mirror(); noLookAhead(); clockAndCaps(); record(); drawing();
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
