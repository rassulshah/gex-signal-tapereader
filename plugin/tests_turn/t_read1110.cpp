// lsTurn 1.11.0: the read that concludes - turnRead() strings (Rassul 2026-11-24 22:39 "everything else is a go", reads-conclude-mockup.html v30)
#include "../TurnLogic.h"
#include <iostream>
static int n = 0, f = 0;
#define CHECK(c) do { ++n; if (!(c)) { ++f; std::cout << "FAIL line " << __LINE__ << ": " #c "\n"; } } while (0)
static std::string all(const turn::CText& c) { std::string s = c.title + "|"; for (auto& r : c.rows) s += r.first + "=" + r.second + "|"; return s; }
int main() {
    using namespace turn;
    Ini in; in.lots = 135; in.att = 24; in.x1000 = 6800; in.away = true; IniCalc nc; nc.feed = true; nc.sweep = 4; nc.burst1000 = 6500; AbsCalc ac; XCalc xc; xc.feed = true; xc.thin1000 = 30; xc.stops = 0;
    CText c = turnRead(1, 1, in, nc, ac, xc, "", 8, "GC"); const std::string a = all(c); std::cout << a << "\n";
    CHECK(c.title == "Initiative - buyers lifted the offer at the bottom" && c.win == 1);
    CHECK(a.find("3 Who won at the end?=Buyers won the low about 5 to 1.") != std::string::npos);
    CHECK(a.find("Sellers ran out at the low.") != std::string::npos);
    CHECK(a.find("5 Tactics?=Urgent: one buy order swept 4 prices.") != std::string::npos);
    CHECK(a.find("?") == a.find("3 Who won at the end?") + 20 && a.find(" ?") == std::string::npos && a.find("x)") == std::string::npos);
    in.lots = 213; in.att = 15; nc.sweep = 8; CHECK(all(turnRead(1, 1, in, nc, ac, xc, "", 8, "GC")).find("Buyers won the low - sellers barely there.") != std::string::npos);
    in.att = 200; CHECK(all(turnRead(1, 1, in, nc, ac, xc, "", 8, "GC")).find("Buyers won the low narrowly.") != std::string::npos);
    nc.sweep = 1; nc.burst1000 = 11000; CHECK(all(turnRead(1, 1, in, nc, ac, xc, "", 8, "GC")).find("Urgent: buyers fired at 11.0x their normal pace.") != std::string::npos);   // GC burst level 10.4
    nc.burst1000 = 9000; CHECK(all(turnRead(1, 1, in, nc, ac, xc, "", 8, "GC")).find("Urgent") == std::string::npos);
    // level in the title, stops
    xc.stops = 9; CText d = turnRead(1, 1, in, nc, ac, xc, "LonLO", 8, "GC"); CHECK(d.title == "Initiative at LonLO - buyers lifted the offer at the bottom");
    CHECK(all(d).find("Stops ran below LonLO, and buyers lifted straight back.") != std::string::npos);
    xc.stops = 7; CHECK(all(turnRead(1, 1, in, nc, ac, xc, "", 8, "GC")).find("Stops ran") == std::string::npos);
    // passive defense, iceberg, exhaustion off when not thin
    xc.stops = 8; xc.thin1000 = 150; ac.feed = true; ac.iceN = 3; ac.iceLots = 20; CText p = turnRead(2, 1, in, nc, ac, xc, "", 8, "GC"); const std::string pa = all(p); std::cout << pa << "\n";
    CHECK(p.title == "Passive defense won - sellers absorbed at the bottom" && p.win == 1);
    CHECK(pa.find("Bids took the selling - the bottom held at the close.") != std::string::npos && pa.find("ran out") == std::string::npos);
    CHECK(pa.find("Stops ran below the low, then bids absorbed them.") != std::string::npos && pa.find("Iceberg: bids refilled 3 times (20 lots) at the low.") != std::string::npos);
    ac.iceLots = 15; CHECK(all(turnRead(2, 1, in, nc, ac, xc, "", 8, "GC")).find("Iceberg") == std::string::npos);
    // aggressor (the top): buyers got through, offers pulled
    ac.putBack = -40; CText g = turnRead(3, -1, in, nc, ac, xc, "PDH", 8, "GC"); const std::string ga = all(g); std::cout << ga << "\n";
    CHECK(g.title == "Aggressor won at PDH - buyers got through the top" && g.win == 1 && ga.find("Offers pulled 40 lots ahead of them - no defense.") != std::string::npos);
    // bearish initiative at the top
    CText b = turnRead(1, -1, in, nc, ac, xc, "", 8, "GC"); CHECK(b.title == "Initiative - sellers hit the bid at the top" && b.win == -1 && all(b).find("Sellers won the high") != std::string::npos);
    // quiet
    CText q = turnRead(0, 0, in, nc, ac, xc, "", 8, "GC"); CHECK(q.quiet && q.title == "No absorption or initiative at the ends" && q.rows.empty());
    // the level in the end quarter: closest to the extreme; none outside
    std::vector<Level> lv(3); lv[0].tick = 100; lv[0].name = "ONL"; lv[1].tick = 101; lv[1].name = "LonLO"; lv[2].tick = 140; lv[2].name = "PDH";
    CHECK(levelInZone(lv, 100, 120, 1) == "ONL" && levelInZone(lv, 100, 140, -1) == "PDH" && levelInZone(lv, 90, 98, 1).empty() && levelInZone(lv, 100, 120, -1).empty());
    // the X record round trip
    std::map<std::pair<long long, int>, XCalc> m, m2; XCalc x; x.feed = true; x.thin1000 = 42; x.stops = 9; m[std::make_pair(1795000000LL, 1)] = x;
    parseXrec("CRD|1|GC|180|1.11.0\n" + xrecText(m), m2); CHECK(m2.size() == 1 && m2.begin()->second.thin1000 == 42 && m2.begin()->second.stops == 9);
    parseXrec("CRD|1|GC|180\nX|1|2|1|5|6\nX|abc|1|1|5|6\nX|5|1|1|-2|6\n", m2); CHECK(m2.empty());
    CHECK(std::string(VERSION) == "1.11.0");
    std::cout << "t_read1110: " << n << " checks, " << f << " fails\n"; return f ? 1 : 0;
}
