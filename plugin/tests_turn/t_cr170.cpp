// t_cr170.cpp -- lsTurn 1.7.0 the compact read (who won at the pushed end; Attack + Absorbed / Not absorbed) and its record file
#include "../TurnLogic.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    using namespace turn;
    CHECK(pushedEnd(2900, 600) == -1 && pushedEnd(600, 3100) == 1 && pushedEnd(1400, 1499) == 0 && pushedEnd(-1, 1500) == 1 && pushedEnd(2000, 2000) == -1);
    AbsCalc c; c.feed = true; c.attQ = 558; c.defShow = 48; c.putBack = 30; c.iceN = 2; c.big.lots = 12; c.big.lo = c.big.hi = 35642;
    CText t = compactText(-1, 2889, true, c, "9:42 AM", "", 0, 0.1);
    CHECK(t.title == "9:42 AM  Passive defense won - buyers absorbed at the top" && t.win == -1 && t.rows.size() == 2);
    CHECK(t.rows[0].first == "Attack" && t.rows[0].second == "Buyers 558 lots into the top 2.9x, biggest print 12.");
    CHECK(t.rows[1].first == "Absorbed" && t.rows[1].second == "Offers showing up to 48 took them and put back 30 (refilled 2x); the top held.");
    AbsCalc d = c; d.putBack = -120; d.big.lo = 35400; d.big.hi = 35403;
    t = compactText(1, 3300, false, d, "9:18 AM", "", 0, 0.1);
    CHECK(t.title == "9:18 AM  Aggressor won - sellers got through the bottom" && t.win == -1);
    CHECK(t.rows[0].second.find("biggest print 12 (swept 4 ticks)") != std::string::npos && t.rows[1].first == "Not absorbed" && t.rows[1].second == "Bids pulled 120 lots ahead of them; closed in the bottom quarter.");
    t = compactText(1, 3100, true, c, "9:21 AM", "ONL", 71050, 0.1); CHECK(t.win == 1 && t.title.find("sellers absorbed at the bottom (ONL ") != std::string::npos);
    t = compactText(-1, 2000, false, AbsCalc(), "", "", 0, 0.25); CHECK(t.win == 1 && t.rows[0].second.find("no order feed") != std::string::npos && t.rows[1].second == "closed in the top quarter.");
    for (const auto& r : t.rows) CHECK(r.first != "Result" && r.first != "Next" && r.first != "Ex");
    t = quietText("10:18 AM", 600, 1000); CHECK(t.quiet && t.rows.empty() && t.title == "10:18 AM  Quiet - no push into either end (top 0.6x / bottom 1.0x)");
    // the record file: both ends per candle, round trip, wrong market / candle size refused, garbage skipped
    std::map<std::pair<long long, int>, AbsCalc> m; m[std::make_pair(100LL, 1)] = c; m[std::make_pair(100LL, -1)] = d;
    const std::string tx = crecText(m, "GC", 180); std::map<std::pair<long long, int>, AbsCalc> r;
    CHECK(parseCrec(tx + "C|x|1|1|2|3|4|5|6|7|8|9\nC|200|0|1|2|3|4|5|6|7|8|9\n", "GC", 180, r) && r.size() == 2);
    CHECK(r[std::make_pair(100LL, -1)].putBack == -120 && r[std::make_pair(100LL, 1)].iceN == 2 && r[std::make_pair(100LL, 1)].big.lots == 12 && r[std::make_pair(100LL, -1)].big.hi == 35403);
    CHECK(!parseCrec(tx, "HG", 180, r) && !parseCrec(tx, "GC", 60, r));
    CHECK(std::string(VERSION) == "1.10.0");
    std::printf("t_cr170: %d checks, %d fails\n", checks, fails); return fails ? 1 : 0; }
