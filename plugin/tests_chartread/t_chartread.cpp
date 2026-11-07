// t_chartread.cpp - ChartRead.h 1.0.0 unit tests (Linux): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined
#include "../ChartRead.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include "gc1106_bars.h"
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static long long T(int y, int mo, int d, int h, int mi) { (void)y; (void)mo; return (long long)(20763 + (d - 6)) * 86400 + h * 3600 + mi * 60; }   // 2026-11-06 = day 20763
static crd::In gcIn()
{
    crd::In I; I.mkt = "GC"; I.openMin = 440; I.dec = 1; I.per = 180;
    for (const crd::Bar& b : GC1106) I.bars.push_back(b);
    I.adr = 49.064; I.onOk = true; I.onHi = 4116.2; I.onLo = 4056.4; I.side = 1; I.lod = 4035.1; I.lodMod = 7 * 60 + 33; I.sweptLod = "LonLO / AsiaLO";
    const std::string params = "VERSION,read_grade test\nMARKET,GC\nCOB,0.57,35\nON_CUTS,1.2,1.74\nBIG_DAY,0.09,0.19,0.47\nAGREE30,0.62\n";
    I.P = crd::parseParams(params, "GC");
    const std::string live = "COBSIGNAL,1.6.4\nCOB," + std::to_string(T(2026, 11, 6, 8, 0)) + ",BULL,Cob,LonLO/AsiaLO/PHL,2.31,4035.1,49.06," +
                             std::to_string(T(2026, 11, 6, 7, 33)) + "," + std::to_string(T(2026, 11, 6, 7, 33)) + ",0,0,SHOW\n";
    const std::string rec = "ZZO," + std::to_string(T(2026, 11, 6, 8, 0)) + ",BULL,14,OK,0\n";
    I.cobs = crd::parseCobs(live, rec);
    return I;
}
int main()
{
    CHECK(GC1106[0].t == T(2026, 11, 6, 6, 3));
    crd::In I = gcIn();
    CHECK(I.P.ok); CHECK(I.cobs.size() == 1); CHECK(I.cobs[0].zzo == 14);
    crd::Read R = crd::build(I);
    std::cout << R.l1 << "\n" << R.l2 << "\n" << R.l3 << "\n";
    CHECK(R.l1 == "GC 8:03 AM  \xB7  LOD candidate:  sellers swept LonLO / AsiaLO / PhrL to 4035.1 at 7:33, Cob (14) at 8:00 - buyers back above PhrL.");
    CHECK(R.l2 == "Buy the 50% pullback ~4053.5  ->  target 4084.0 (1.33)  \xB7  cancel on a close below 4035.1  \xB7  size down: first 30 min closed down, normal-range day.");
    CHECK(R.l3 == "Odds: GC Cob 57% (n 35)  \xB7  big day 19%  \xB7  my GC read grade: starts after the first graded session");
    CHECK(R.tone == crd::GREEN && R.side == 1 && std::fabs(R.entry - 4053.5) < 1e-6 && std::fabs(R.cancel - 4035.1) < 1e-9);
    // log row: quoted, one line, version tagged
    const std::string row = crd::logRow(R, 1, "lsHodLod 2.20.0");
    CHECK(row.find('\n') == std::string::npos); CHECK(row.rfind("8:03 AM,\"LOD candidate\",1,4053.5,4084.0,4035.1,", 0) == 0);
    CHECK(crd::ymd(R.at) == "20261106");
    // a close below the origin later -> Cob cancelled, grey wait
    { crd::In J = I; crd::Bar b = J.bars.back(); b.t += 180; b.c = 4030.0; b.l = 4029.0; J.bars.push_back(b); crd::Read Q = crd::build(J);
      CHECK(Q.sit == "Cob done" && Q.tone == crd::GREY && Q.l2.rfind("Wait:", 0) == 0); }
    // target reached -> done
    { crd::In J = I; crd::Bar b = J.bars.back(); b.t += 180; b.h = 4090.0; b.c = 4088.0; J.bars.push_back(b); crd::Read Q = crd::build(J);
      CHECK(Q.sit == "Cob done" && Q.l2.rfind("Done:", 0) == 0); }
    // after 11:00 -> no new trades
    { crd::In J = I; crd::Bar b = J.bars.back(); b.t = T(2026, 11, 6, 11, 3); J.bars.push_back(b); crd::Read Q = crd::build(J);
      CHECK(Q.l2.find("after 11:00 AM") != std::string::npos && Q.tone == crd::GREY); }
    // opening (no CoB yet, 7:36)
    { crd::In J = I; J.cobs.clear(); while (!J.bars.empty() && crd::modOf(J.bars.back().t) > 7 * 60 + 36) J.bars.pop_back(); crd::Read Q = crd::build(J);
      CHECK(Q.sit == "Opening" && Q.l2.find("62%") != std::string::npos); std::cout << Q.l1 << "\n"; }
    // no CoB, 8:03, side known -> LOD candidate wait
    { crd::In J = I; J.cobs.clear(); crd::Read Q = crd::build(J); CHECK(Q.sit == "LOD candidate" && Q.l2.rfind("Wait for a Cob", 0) == 0); std::cout << Q.l1 << "\n"; }
    // trend day: big odds high + first 30 down, no side
    { crd::In J = I; J.cobs.clear(); J.side = 0; J.P.big[1] = 0.45; crd::Read Q = crd::build(J); CHECK(Q.sit == "Trend day?" && Q.l2.find("short") != std::string::npos); }
    // big day -> hold for 1.618
    { crd::In J = I; J.P.big[1] = 0.45; crd::Read Q = crd::build(J); CHECK(Q.l2.find("hold for 1.618 (big day 45%)") != std::string::npos); }
    // pre-open
    { crd::In J = I; J.cobs.clear(); while (!J.bars.empty() && crd::modOf(J.bars.back().t) > 7 * 60 + 18) J.bars.pop_back(); crd::Read Q = crd::build(J); CHECK(Q.sit == "Pre-open"); }
    // params: missing / wrong market / garbage -> not ok, odds shown as "-"
    CHECK(!crd::parseParams("", "GC").ok); CHECK(!crd::parseParams("VERSION,1\nMARKET,ES\n", "GC").ok);
    { crd::Params P = crd::parseParams("VERSION,1\nMARKET,GC\nCOB,1.7,35\nBIG_DAY,x,1,2\nGRADE,65% right \xB7 +0.31R \xB7 last 20\n", "GC");
      CHECK(P.ok && !std::isfinite(P.cobWin) && !std::isfinite(P.big[0]) && P.grade.find("65%") == 0); }
    { crd::In J = I; J.P = crd::Params(); crd::Read Q = crd::build(J); CHECK(Q.l3 == "Odds: GC Cob -  \xB7  big day -  \xB7  my GC read grade: starts after the first graded session"); }
    // CoB parse: LOG lines and garbage ignored; bear side
    { auto C = crd::parseCobs("COB,1,BEAR,Cob+R,PDH,2,100,5,0,0,0,0,LOG\nCOB,x\nCOB,2,BEAR,Cob+R,PDH/PHH,2,100,5,1,1,0,0,SHOW\n", ""); CHECK(C.size() == 1 && C[0].side == -1 && C[0].label == "Cob+R"); }
    // fit
    { auto m = [](const std::string& s) { return (int)s.size() * 7; }; std::string f = crd::fit(std::string("one two three four five"), 70, m); CHECK((int)f.size() * 7 <= 70 && f.size() > 3 && f.substr(f.size() - 3) == "..."); }
    // determinism: same input -> same text (no repaint)
    { crd::Read A = crd::build(I), B2 = crd::build(I); CHECK(A.l1 == B2.l1 && A.l2 == B2.l2 && crd::key(A) == crd::key(B2)); }
    std::printf(fails ? "t_chartread: %d FAIL\n" : "t_chartread: all passed\n", fails);
    return fails ? 1 : 0;
}
