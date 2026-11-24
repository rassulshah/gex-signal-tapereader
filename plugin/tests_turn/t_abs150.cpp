// t_abs150.cpp -- lsTurn 1.5.0: the order-feed book per price, the biggest single order, P / Ex?, the Abs read text, the wy Abs lines, the badge file
#include "../TurnLogic.h"
#include <cstdio>
static int checks = 0, fails = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
using namespace turn;
static long long oid = 1000;
int main() {
    // ---- parseAbs ----
    { const std::string w = "WY|1|HG|180|0.27.0\n100|Bk|1|LLO|50|\n3600|Abs|-1|Top 21%|11389|LHI\n3780|Ex|-1\n7200|Abs|1||11000|1hL\n7380|Ex|-1\n9000|Abs|1|Low 5%|11000\nbad|Abs|1|x|1|y\n";
      const std::vector<AbsLine> a = parseAbs(w, "HG", 180);
      CHECK(a.size() == 2); CHECK(a[0].t == 3600 && a[0].dir == -1 && a[0].pos == "Top 21%" && a[0].lvPx == 11389 && a[0].lv == "LHI" && a[0].ex && a[0].exT == 3780);
      CHECK(a[1].t == 7200 && a[1].dir == 1 && a[1].pos.empty() && !a[1].ex);          // an Ex of the other side is not its Ex
      CHECK(parseAbs(w, "GC", 180).empty() && parseAbs(w, "HG", 60).empty() && parseAbs("garbage", "HG", 180).empty());
      const std::vector<AbsLine> b = parseAbs("WY|1|HG|180|x\n3600|Abs|1|Low 3%|5|PDL\n4500|Ex|1\n", "HG", 180); CHECK(b.size() == 1 && !b[0].ex); }   // an Ex 5 candles later is not its Ex
    // ---- the feed: three quiet candles, then the Abs candle (buyers attack the high), then the candle after ----
    Live L; L.reset(180); const long long K0 = 180 * 1000;               // candle keys K0, K0+180, ...
    auto fill = [&](long long sec, int buy, long long px, long long sz, long long passive, long long aggr) { L.onEvent(sec, 3, buy, px, sz, passive, aggr); };
    long long ag = 500000;
    // candle 1 (K0 - 180 .. K0): the feed starts (not complete)
    for (int k = 0; k < 6; ++k) fill(K0 - 170 + k, 1, 90, 1, 0, ++ag);
    // candles 2..4: small buy prints (5 lots max), some sells
    for (int c = 1; c <= 3; ++c) { const long long st = K0 + (c - 1) * 180; for (int k = 0; k < 8; ++k) fill(st + 5 + k * 10, k % 2, 88 + k, k == 3 ? 5 : 2, 0, ++ag); }
    // the Abs candle: start S = K0 + 3*180, key S + 180. Offers resting 100..105 (10 lots each), bids 80..85
    const long long S = K0 + 3 * 180, KA = S + 180;
    std::vector<long long> offer(106, 0);
    for (long long px = 100; px <= 105; ++px) { offer[(size_t)px] = ++oid; L.onEvent(S + 1, 0, 0, px, 10, offer[(size_t)px], 0); }
    for (long long px = 80; px <= 85; ++px) L.onEvent(S + 1, 0, 1, px, 10, ++oid, 0);
    for (int k = 0; k < 10; ++k) fill(S + 2 + k * 10, 1, 86 + k, 3, 0, ++ag);   // buyers push up in the first 2 minutes (30 lots)
    fill(S + 105, 0, 85, 2, 0, ++ag); fill(S + 106, 0, 84, 2, 0, ++ag);
    const long long big = ++ag; for (long long px = 100; px <= 105; ++px) fill(S + 110, 1, px, 8, offer[(size_t)px], big);   // one 48-lot buy sweeping 100 -> 105
    L.onEvent(S + 111, 1, 0, 105, 12, offer[105], 0);                   // the 105 offer tops up after its fill: an iceberg tranche (+10)
    L.onEvent(S + 112, 2, 0, 104, 0, offer[104], 0);                    // the 104 offer's rest (2) is pulled
    fill(S + 150, 1, 104, 1, 0, ++ag); fill(S + 160, 0, 95, 4, 0, ++ag);  // the last minute: 1 buy lot
    fill(S + 170, 0, 92, 3, 0, ++ag);
    // the candle after (key KA + 180): buyers 2 lots, offers added at 99
    for (int k = 0; k < 3; ++k) L.onEvent(KA + 2 + k, 0, 0, 99, 20, ++oid, 0);
    for (int k = 0; k < 6; ++k) fill(KA + 10 + k * 20, k < 1 ? 1 : 0, 96 - k, k < 1 ? 2 : 3, 0, ++ag);
    fill(KA + 140, 0, 99, 1, 0, ++ag);                                   // 99 traded: its book counts (traded prices only, as the research store)
    fill(KA + 175, 0, 90, 1, 0, ++ag); L.onEvent(KA + 176, 2, 1, 80, 0, 0, 0);
    L.closeThrough(KA + 180);
    const Sum* sa = L.find(KA); CHECK(sa != nullptr);
    if (sa) {
        CHECK(sa->complete && !sa->thin); CHECK(sa->hiPx == 105 && sa->loPx == 84);
        CHECK(sa->bigAny[1] == 48 && sa->qBig[1].lots == 48 && sa->qBig[1].lo == 100 && sa->qBig[1].hi == 105);
        CHECK(sa->early[1] == 30 + 48 && sa->late[1] == 1);               // the sweep at S+110 is in the first 2 minutes (cut at S+120)
        const auto b105 = sa->book.find(105); CHECK(b105 != sa->book.end());
        if (b105 != sa->book.end()) { CHECK(b105->second.shownMax[1] == 12 && b105->second.refills[1] == 1 && b105->second.refillLots[1] == 10 && b105->second.filled[1] == 8); }
        const auto b104 = sa->book.find(104); CHECK(b104 != sa->book.end() && b104->second.pull[1] == 2);
        CHECK(!sa->book.count(80));                                       // a price that did not trade is not kept
    }
    if (sa) { const auto b = sa->book.find(105); if (b != sa->book.end()) std::printf("105: show %lld ref %lld refl %lld filled %lld add %lld pull %lld\n", b->second.shownMax[1], b->second.refills[1], b->second.refillLots[1], b->second.filled[1], b->second.add[1], b->second.pull[1]); }
    const AbsCalc c = absCalc(L, KA, -1);
    CHECK(c.feed && c.big.lots == 48 && c.p == 3 && c.plus);            // bigger than the 3 candles before; the history ran out (the first candle was not whole)
    CHECK(c.exq);                                                         // 1 lot in the last minute vs 78 before: 0.03x <= 0.5x
    CHECK(c.attQ == 48 + 1);                                              // buys in the top quarter (z = round(22/4) = 6: prices > 99)
    CHECK(c.iceLots == 10 && c.iceN == 1);
    const NextCalc nx = nextCalc(L, KA, KA + 180, -1); std::printf("nx feed %d att %lld attAbs %lld dOk %d dNet %.3f rOk %d\n", nx.feed, nx.att, nx.attAbs, nx.dOk, nx.dNet, nx.rOk);
    CHECK(nx.feed && nx.att == 2 && nx.attAbs == 30 + 48 + 1 && nx.dOk && nx.dNet > BOOK_REBUILT);
    const AbsLine al = parseAbs("WY|1|HG|180|x\n" + std::to_string(KA) + "|Abs|-1|Top 0%|105|LHI\n" + std::to_string(KA + 180) + "|Ex|-1\n", "HG", 180).at(0);
    AbsCalc c16 = c; c16.p = 16; c16.plus = false;
    const AbsText t = absText(al, c16, &nx, "9:09 AM", "9:12 AM", 1.6, 1.0, 0.0005);
    CHECK(t.title == "9:09 AM  Bearish ? - absorbed at LHI 0.0525");
    std::printf("%s\n", t.rows[0].second.c_str());
    CHECK(t.rows.size() == 4 && t.rows[0].first == "Attack" && t.rows[3].first == "Ex");
    CHECK(t.rows[0].second.find("Buyers lifted 49 lots into the top 25% (Top 0%), led by one 48-lot buy (biggest in 16 bars) - it swept 6 ticks to 0.0525.") == 0);
    CHECK(t.rows[1].second.find("Offers showing up to") == 0 && t.rows[1].second.find("(iceberg 10)") != std::string::npos);
    CHECK(t.rows[2].second == "Volume 1.6x, range 1.0x, closed in the lower half; last minute buyers 0.0x - Ex?.");
    CHECK(t.rows[3].second.find("9:12 AM - buyers traded only 2 lots (0.03x) and the candle closed further away") == 0 && t.rows[3].second.find("buyers exhausted") != std::string::npos);
    // no feed: the read says so and has no Absorbed line
    { AbsCalc none; const AbsText u = absText(al, none, nullptr, "9:09 AM", "", -1, -1, 0.0005);
      CHECK(u.rows.size() == 2 && u.rows[0].second == "Buyers lifted the top 25% (Top 0%) - no order feed for this candle." && u.rows[1].second == "Closed in the lower half."); }
    // a small print: "in small orders"
    { AbsCalc sm = c; sm.p = 2; const AbsText u = absText(al, sm, nullptr, "", "", 1.6, 1.0, 0.0005); CHECK(u.rows[0].second.find("in small orders (biggest 48 - no big print)") != std::string::npos); }
    // the badge file
    { std::map<long long, TabRec> m; TabRec r; r.dir = -1; r.p = 16; r.exq = true; m[KA] = r; r.dir = 1; r.p = -1; r.exq = false; m[KA + 900] = r;
      const std::string f = tabFileText(m, "HG", 180); std::map<long long, TabRec> back; CHECK(parseTabFile(f, "HG", 180, back) && back.size() == 2 && back[KA].p == 16 && back[KA].exq && back[KA + 900].p == -1);
      CHECK(!parseTabFile(f, "GC", 180, back)); CHECK(f.compare(0, 13, "TAB|1|HG|180|") == 0); }
    // reset clears the book and the history
    L.reset(180); CHECK(L.hist.empty() && L.sums.empty());
    std::printf("%d checks, %d failed\n", checks, fails); return fails ? 1 : 0;
}
