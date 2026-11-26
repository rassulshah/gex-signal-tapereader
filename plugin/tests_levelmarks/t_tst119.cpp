// t_tst119.cpp -- LevelMarks 1.19.0 tstL3: the Tst badge's 3rd line A+E / E / A! / "" (research TST-REVERSE 2026-11-24)
#include "../LevelMarks.h"
#include "../WyMarks.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static wyr::Bar bar(long long t, long long o, long long h, long long l, long long c, long long buy, long long sell, long long qt = 10, long long qb = 10) {
    wyr::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; b.buy = buy; b.sell = sell; b.qTopLots = qt; b.qBotLots = qb; return b; }
static std::map<long long, std::vector<lvm::SMark> > tstAt(long long t, int dir) { std::map<long long, std::vector<lvm::SMark> > s; lvm::SMark m; m.code = 'T'; m.dir = dir; m.lv = "ONL"; s[t].push_back(m); return s; }
int main() {
    const long long T0 = 3870000000LL;
    // 25 quiet candles: range 10 ticks, volume 100 (50 / 50), 10 lots at the heaviest quarter price; then the Tst pivot (index 25) and the candle after
    auto base = [&]() { std::vector<wyr::Bar> B; for (int k = 0; k < 25; ++k) B.push_back(bar(T0 + 180 * k, 1005, 1010, 1000, 1005, 50, 50)); return B; };
    const long long tp = T0 + 180 * 25, tn = tp + 180;
    // 1) bullish Tst, effort without result (vol 2x, range 1.2x, closed high) + next candle exhausted (sellers 40 -> 15, closed higher, held) = A+E
    { std::vector<wyr::Bar> B = base(); B.push_back(bar(tp, 1000, 1002, 990, 999, 80, 120)); B.push_back(bar(tn, 999, 1003, 991, 1001, 60, 40));
      const auto o = lvm::tstL3(B, tstAt(tp, 1)); CHECK(o.count(tp) && o.at(tp) == "A+E"); }
    // 2) the same, the next candle NOT exhausted (sellers 70 > 0.5 x 120) = A!
    { std::vector<wyr::Bar> B = base(); B.push_back(bar(tp, 1000, 1002, 990, 999, 80, 120)); B.push_back(bar(tn, 999, 1003, 991, 1001, 60, 70));
      CHECK(lvm::tstL3(B, tstAt(tp, 1)).at(tp) == "A!"); }
    // 3) next candle traded 2 ticks under the pivot low = not held -> A!; 1 tick under = held -> A+E
    { std::vector<wyr::Bar> B = base(); B.push_back(bar(tp, 1000, 1002, 990, 999, 80, 120)); B.push_back(bar(tn, 999, 1003, 988, 1001, 60, 40));
      CHECK(lvm::tstL3(B, tstAt(tp, 1)).at(tp) == "A!"); B.back().l = 989; CHECK(lvm::tstL3(B, tstAt(tp, 1)).at(tp) == "A+E"); }
    // 4) no effort (vol 1x) but the Absorption bar: 25 lots at the heaviest bottom-quarter sell price = 2.5x, closed outside the quarter -> A
    { std::vector<wyr::Bar> B = base(); B.push_back(bar(tp, 1000, 1002, 990, 999, 50, 50, 10, 25)); B.push_back(bar(tn, 999, 1003, 991, 1001, 60, 50));
      CHECK(lvm::tstL3(B, tstAt(tp, 1)).at(tp) == "A!");
      B[25].c = 992; CHECK(lvm::tstL3(B, tstAt(tp, 1)).at(tp) == "");                  // closed inside the bottom quarter (13 levels -> z 3: 990..992): no A
      B[25].c = 999; B[25].qBotLots = 19; CHECK(lvm::tstL3(B, tstAt(tp, 1)).at(tp) == ""); }   // 1.9x: no A
    // 5) exhaustion only from the order feed (lsTurn's Ex?): sellers' flag for a bullish Tst, buyers' for a bearish one
    { std::vector<wyr::Bar> B = base(); B.push_back(bar(tp, 1005, 1010, 1000, 1004, 50, 50)); B.push_back(bar(tn, 1004, 1006, 999, 1002, 60, 50));
      std::map<long long, std::pair<int, int> > ex; ex[tp] = std::make_pair(1, 0);
      CHECK(lvm::tstL3(B, tstAt(tp, 1)).at(tp) == ""); CHECK(lvm::tstL3(B, tstAt(tp, 1), &ex).at(tp) == "E"); CHECK(lvm::tstL3(B, tstAt(tp, -1), &ex).at(tp) == ""); }
    // 6) bearish mirror: effort without result at the high (closed low), buyers faded next candle, held under the high + 1 tick
    { std::vector<wyr::Bar> B = base(); B.push_back(bar(tp, 1010, 1020, 1008, 1011, 120, 80)); B.push_back(bar(tn, 1011, 1019, 1006, 1009, 40, 60));
      CHECK(lvm::tstL3(B, tstAt(tp, -1)).at(tp) == "A+E"); }
    // 7) not decided before the candle after the pivot closed (unless decideLast); a non-Tst mark is ignored
    { std::vector<wyr::Bar> B = base(); B.push_back(bar(tp, 1000, 1002, 990, 999, 80, 120));
      CHECK(lvm::tstL3(B, tstAt(tp, 1)).empty()); CHECK(lvm::tstL3(B, tstAt(tp, 1), nullptr, nullptr, nullptr, nullptr, true).at(tp) == "A!");
      auto s = tstAt(tp, 1); s[tp][0].code = 'R'; B.push_back(bar(tn, 999, 1003, 991, 1001, 60, 40)); CHECK(lvm::tstL3(B, s).empty()); }
    // 8) fewer than 10 candles before: no A from the averages; the candles before the session (preV / preR / preQ) count
    { std::vector<wyr::Bar> B; for (int k = 0; k < 5; ++k) B.push_back(bar(T0 + 180 * k, 1005, 1010, 1000, 1005, 50, 50));
      const long long t5 = T0 + 900; B.push_back(bar(t5, 1000, 1002, 990, 999, 80, 120)); B.push_back(bar(t5 + 180, 999, 1003, 991, 1001, 60, 70));
      CHECK(lvm::tstL3(B, tstAt(t5, 1)).at(t5) == "");
      std::vector<double> pv(15, 100.0), pr(15, 10.0); std::vector<lvm::PreQ> pq(15); for (auto& q : pq) { q.top = 10; q.bot = 10; }
      CHECK(lvm::tstL3(B, tstAt(t5, 1), nullptr, &pv, &pr, &pq).at(t5) == "A!"); }
    CHECK(lvm::l3Text(true, true) == "A+E" && lvm::l3Text(false, true) == "E" && lvm::l3Text(true, false) == "A!" && lvm::l3Text(false, false).empty());
    // the files: lsTurn's turnexq (WyMarks parseTeq) and the decisions file (tl3Text / parseTl3) - first decision kept, "-" = blank, junk skipped
    { std::map<long long, std::pair<int, int> > q; CHECK(wyi::parseTeq("TEQ|1|ES|180|1.6.0\n100|1|0\n200|0|1\nbad|1|1\n300|2|0\n", "ES", 180, q) && q.size() == 2 && q[100].first == 1 && q[200].second == 1);
      CHECK(!wyi::parseTeq("TEQ|1|ES|60|1.6.0\n100|1|0\n", "ES", 180, q) && q.empty()); CHECK(!wyi::parseTeq("TAB|1|ES|180|x\n", "ES", 180, q));
      std::map<long long, std::string> m; m[100] = "A+E"; m[200] = ""; m[300] = "A!"; const std::string t = wyi::tl3Text(m, "GC", 180, "0.28.0");
      CHECK(t == "TL3|1|GC|180|0.28.0\n100|A+E\n200|-\n300|A!\n");
      std::map<long long, std::string> r; CHECK(wyi::parseTl3(t + "400|ZZ\nx|E\n", "GC", 180, r) && r == m); CHECK(!wyi::parseTl3(t, "HG", 180, r)); CHECK(!wyi::parseTl3(t, "GC", 60, r)); }
    CHECK(std::string(lvm::VERSION) == "1.29.0" && std::string(wyi::VERSION) == "2.8.0");
    std::printf("t_tst119: %d checks, %d fails\n", checks, fails); return fails ? 1 : 0; }
