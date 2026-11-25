// t_ini180.cpp -- lsTurn 1.8.0 Initiative: the counter side's heaviest end-quarter price, the on rule, the end picked, the read, the record lines
#include "../TurnLogic.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    using namespace turn;
    // the real GC 11-20 9:30 AM candle (research rows: tick, buys, sells): buyers 135 at 35299 in the bottom quarter, sellers' heaviest there 24
    static const long long R[][3] =
#include "gc930.inc"
    ;
    std::vector<Row> rows; for (const auto& x : R) { Row r; r.tick = x[0]; r.buy = x[1]; r.sell = x[2]; rows.push_back(r); }
    const Bars b = barRatios(rows, 35277, 35375);
    CHECK(b.zone == 25 && b.botIni == 135 && b.botIniTick == 35299 && b.botMax == 24);
    CHECK(b.topIni >= 0 && b.topMax >= 0);
    // the on rule: >= 1.5, bigger than the attackers there, closed outside the quarter
    Ini i; i.x1000 = 6800; i.lots = 135; i.att = 24; i.away = true; i.tick = 35299; i.attX1000 = 1100;
    CHECK(iniOn(i)); Ini j = i; j.x1000 = 1499; CHECK(!iniOn(j)); j = i; j.att = 135; CHECK(!iniOn(j)); j = i; j.away = false; CHECK(!iniOn(j)); j = i; j.lots = 0; j.att = -1; CHECK(!iniOn(j));
    CHECK(iniBar(i).on && iniBar(i).show && iniBar(i).label == "6.8"); j = i; j.away = false; CHECK(iniBar(j).show && !iniBar(j).on); CHECK(!iniBar(Ini()).show);
    // the end: the one on; both on = the bigger (ties the bottom)
    Ini up = i; up.x1000 = 2000; CHECK(iniEnd(Ini(), i) == 1 && iniEnd(up, Ini()) == -1 && iniEnd(up, i) == 1); up.x1000 = 9000; CHECK(iniEnd(up, i) == -1); up.x1000 = 6800; CHECK(iniEnd(up, i) == 1);
    CHECK(iniEnd(Ini(), Ini()) == 0);
    // burst levels per market (tenths)
    CHECK(burstLevel10("GC") == 104 && burstLevel10("ES") == 54 && burstLevel10("NQ") == 77 && burstLevel10("EU") == 121 && burstLevel10("6E") == 121 && burstLevel10("ZZ") == 100);
    // the read
    IniCalc c; c.feed = true; c.burst1000 = 9500; c.sweep = 4; c.bigLots = 11;
    CText t = iniText(1, i, c, "9:30 AM", 0.1, "GC");
    CHECK(t.title == "9:30 AM  Initiative - buyers lifted the offer at the bottom" && t.win == 1 && t.rows.size() == 3);
    CHECK(t.rows[0].first == "Initiative" && t.rows[0].second == "buyers 135 lots at 3529.9 (6.8x)");
    CHECK(t.rows[1].first == "How" && t.rows[1].second == "Burst 9.5x pace (10.4x) ?  |  Swept 4 prices (4) ?  |  Biggest 11");
    CHECK(t.rows[2].first == "Against" && t.rows[2].second == "sellers 24 lots there (1.1x) - buyers won the low");
    Ini k = i; k.tick = 35642; t = iniText(-1, k, IniCalc(), "", 0.1, "GC");
    CHECK(t.title == "Initiative - sellers hit the bid at the top" && t.win == -1 && t.rows[1].second == "no order feed for this candle" && t.rows[2].second.find("sellers won the high") != std::string::npos);
    IniCalc c2 = c; c2.burst1000 = -1; c2.sweep = 1; c2.bigLots = 0; t = iniText(1, i, c2, "", 0.25, "ES");
    CHECK(t.rows[1].second == "Burst - (no pace yet)  |  Swept 1 price (4) ?");
    // the record: N lines round trip next to the C lines; an older reader (the 3-argument parse) skips them
    std::map<std::pair<long long, int>, AbsCalc> m; AbsCalc a; a.feed = true; a.attQ = 7; m[std::make_pair(100LL, 1)] = a;
    std::map<std::pair<long long, int>, IniCalc> n; c.bigLo = 35299; c.bigHi = 35302; c.burst = 60; n[std::make_pair(100LL, 1)] = c; n[std::make_pair(100LL, -1)] = IniCalc();
    const std::string tx = crecText(m, n, "GC", 180);
    std::map<std::pair<long long, int>, AbsCalc> r; std::map<std::pair<long long, int>, IniCalc> rn;
    CHECK(parseCrec(tx + "N|x|1|1|2|3|4|5|6|7\nN|200|0|1|2|3|4|5|6|7\nN|300|1|1|-5|3|4|5|6|7\n", "GC", 180, r, rn) && r.size() == 1 && rn.size() == 2);
    const IniCalc& g = rn[std::make_pair(100LL, 1)]; CHECK(g.feed && g.bigLots == 11 && g.bigLo == 35299 && g.bigHi == 35302 && g.sweep == 4 && g.burst == 60 && g.burst1000 == 9500);
    CHECK(!rn[std::make_pair(100LL, -1)].feed && rn[std::make_pair(100LL, -1)].burst1000 == -1);
    std::map<std::pair<long long, int>, AbsCalc> old; CHECK(parseCrec(tx, "GC", 180, old) && old.size() == 1);
    CHECK(!parseCrec(tx, "HG", 180, r, rn));
    // the sweep strip uses the new Initiative when given, the old marks otherwise
    Read rd; rd.marks = 2; rd.ansOk = 1; Strip s0 = stripOf(rd, false, true, "", Abs(), ABS_FROM10); CHECK(s0.flat.find("Initiative 2/3") != std::string::npos);
    Strip s1 = stripOf(rd, false, true, "", Abs(), ABS_FROM10, &i); CHECK(s1.flat.find("Heavy Initiative 6.8x") != std::string::npos && s1.flat.find("2/3") == std::string::npos);
    Ini q = i; q.x1000 = 1200; Strip s2 = stripOf(rd, false, false, "", Abs(), ABS_FROM10, &q); CHECK(s2.flat.find("Light Initiative") != std::string::npos);
    Strip s3 = stripOf(rd, false, false, "", Abs(), ABS_FROM10, &j); CHECK(s3.flat.find("No Initiative") != std::string::npos);
    CHECK(std::string(VERSION) == "1.8.0");
    std::printf("t_ini180: %d checks, %d fails\n", checks, fails); return fails ? 1 : 0; }
