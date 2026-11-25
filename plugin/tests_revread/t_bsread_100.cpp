#include "../BsRead.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
int main() {
    using namespace bsr;
    // the real GC 9:30 AM candle: buys 909 (1.9x), sells 598 (1.3x), delta +311, swing +284 (3.2x)
    In x; x.ok = true; x.buy = 909; x.sell = 598; for (int k = 0; k < 20; ++k) { x.pb.push_back(478); x.ps.push_back(460); x.pswing.push_back(89); } x.prevOk = true; x.prevDelta = 27; x.nB = 8; x.nS = 0;
    Out o = read(x);
    CHECK(o.ok && o.verdict == 1 && o.head == "Buyers 909 vs sellers 598" && o.ex.size() == 3);
    CHECK(o.ex[0] == "Buyers 1.9x normal, sellers 1.3x - delta +311.");
    CHECK(o.ex[1] == "Delta swung +284 from the candle before (3.2x) ?");
    x.nB = 10; o = read(x); CHECK(o.head == "Buyers 909 vs sellers 598 - the most buying in 10 candles");
    x.nB = 150; o = read(x); CHECK(o.head.find("in 99+ candles") != std::string::npos);
    // sellers bigger, no big swing
    In y = x; y.buy = 802; y.sell = 922; y.prevDelta = -120; y.nS = 5; o = read(y);
    CHECK(o.verdict == -1 && o.head == "Sellers 922 vs buyers 802" && o.ex[0].compare(0, 22, "Sellers 2.0x normal, b") == 0 && o.ex[0].find("delta -120.") != std::string::npos);
    CHECK(o.ex[1] == "Delta: no big swing from the candle before (0.0x).");
    // even within 10%, thousands with commas, no normal yet, no candle before
    In z; z.ok = true; z.buy = 1050; z.sell = 1000; o = read(z);
    CHECK(o.verdict == 0 && o.head == "Even - buyers 1,050 vs sellers 1,000" && o.ex.size() == 2 && o.ex[0] == "Normal not known yet - delta +50.");
    In e; e.ok = true; o = read(e); CHECK(o.head == "No trades" && o.verdict == 0);
    In n9 = x; n9.pb.resize(9); o = read(n9); CHECK(o.ex[0].compare(0, 10, "Normal not") == 0);
    In bad; CHECK(!read(bad).ok);
    // (1.1.0) the big-order line and the questions
    In g = x; g.nB = 8; o = read(g); CHECK(o.ex.back() == "Big orders: no order feed for this candle." && o.exQ.size() == o.ex.size() && o.exQ.back() == std::string(Q4) && o.exQ[0] == std::string(Q2) && o.headQ == Q2);
    g.bigOk = true; g.thr = 32; g.allBuy = 909; g.allSell = 598; o = read(g); CHECK(o.ex.back() == "Big orders (32+ lots): none - all small orders.");
    g.bigBuy = 282; g.bigSell = 0; o = read(g); CHECK(o.ex.back() == "Big orders (32+ lots): 31% of the buying, no big selling.");
    g.bigSell = 72; o = read(g); CHECK(o.ex.back() == "Big orders (32+ lots): 31% of the buying, 12% of the selling.");
    g.thr = 1250; o = read(g); CHECK(o.ex.back().find("(1,250+ lots)") != std::string::npos);
    std::printf("t_bs: %d checks, %d fails\n", checks, fails); return fails ? 1 : 0; }
