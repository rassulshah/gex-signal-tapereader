// lsBigBidAsk 1.0.0 host tests: BigBidAsk.cpp on the mock SDK (tests_bigba/irtsdk.h). Build from tests_bigba/:
//   g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I. -I.. test_bigba_host.cpp
#include "irtsdk.h"
namespace mock { Host* current = nullptr; }
#include "../BigBidAsk.cpp"
#include <cstdio>
#include <fstream>
#include <random>
#include <set>
#include <sys/stat.h>
static int fails = 0, passes = 0;
#define CHECK(c) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
static std::string HOME;
static const unsigned long T0 = 1792058400UL;   // 2026-10-14 ~09:00 "CT wall" in the mock's gmtime convention (inside one session)

struct Chart {
    mock::Host H; BigBidAsk* x = nullptr;
    Chart() { mock::current = &H; x = static_cast<BigBidAsk*>(CreateExtension()); x->setup(); }
    ~Chart() { use(); x->destroy(); delete x; mock::current = nullptr; }
    void use() { mock::current = &H; }
    void pass() { use(); H.now = H.bars.empty() ? T0 : H.bars.back().time; x->calc(0); H.draws.clear(); x->draw(); }
    BBState* S() { use(); return x->testState(); }
    void restart() { use(); x->destroy(); delete x; H.data = nullptr; x = static_cast<BigBidAsk*>(CreateExtension()); x->setup(); }
};
// a candle with rows at prices: each row (price, ask lots, bid lots)
static mock::Bar bar(unsigned long t, float o, float h, float l, float c, std::vector<mock::VRow> rows) {
    mock::Bar b; b.time = t; b.open = o; b.high = h; b.low = l; b.close = c; b.rows = rows; for (auto& r : rows) b.volume += (unsigned long)(r.ask + r.bid); return b;
}
// a normal candle: biggest bid 'bid' at its low + 0.25, biggest ask 'ask' at its high - 0.25
static mock::Bar normal(unsigned long t, float base, long bid = 60, long ask = 60) {
    return bar(t, base, base + 1.0f, base, base + 0.5f, { { base, 5, 10 }, { base + 0.25f, 8, bid }, { base + 0.5f, 9, 9 }, { base + 0.75f, ask, 7 }, { base + 1.0f, 6, 4 } });
}
static std::vector<mock::Draw> texts(const mock::Host& H) { std::vector<mock::Draw> v; for (const auto& d : H.draws) if (d.type == "ctext") v.push_back(d); return v; }
static std::vector<mock::Draw> rings(const mock::Host& H) { std::vector<mock::Draw> v; for (const auto& d : H.draws) if (d.type == "ring") v.push_back(d); return v; }
static int yOf(const mock::Host& H, float price) { return (int)(short)(H.paneBottom - 100 - (price - H.priceBase) / H.tick * H.pxPerTick); }
static int xL(const mock::Host& H, int i) { return 20 + (i - H.barOffset) * H.barW; }
static const mock::Draw* textAtBar(const std::vector<mock::Draw>& v, const mock::Host& H, int i, int side) {
    for (const auto& d : v) { if (d.color != (side < 0 ? C_SELL : C_BUY)) continue; if (side < 0 && d.r < xL(H, i) && d.r >= xL(H, i) - 12) return &d; if (side > 0 && d.l > xL(H, i) + H.barW / 2 && d.l <= xL(H, i) + H.barW / 2 + 12) return &d; }
    return nullptr;
}
static std::string key(const mock::Draw& d) { char b[160]; std::snprintf(b, sizeof b, "%s|%d|%d|%d|%d|%lx|%d|%d", d.text.c_str(), d.l, d.t, d.r, d.b, d.color, d.font, (int)d.bold); return b; }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); return s; }
static void wipe() { std::string cmd = "rm -rf '" + HOME + "/InvestorRT/rtx/lsFlexLevels' && mkdir -p '" + HOME + "/InvestorRT/rtx/lsFlexLevels'"; if (std::system(cmd.c_str()) != 0) std::printf("wipe failed\n"); }

// [1] the sign: red = sold AT THE BID (sellVolume) left of the candle at its price, green = bought AT THE ASK (buyVolume) right of it
static void t1_sign() {
    wipe(); Chart c;
    for (int i = 0; i < 24; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700));
    // candle 24: sellers hit 7700.25 hardest (bid 90), buyers lifted 7700.75 hardest (ask 70); a big ASK at 7700.25 must not make it red
    c.H.bars.push_back(bar(T0 + 180 * 24, 7700, 7701, 7700, 7700.5f, { { 7700.0f, 3, 20 }, { 7700.25f, 50, 90 }, { 7700.5f, 4, 4 }, { 7700.75f, 70, 30 }, { 7701.0f, 2, 2 } }));
    c.H.bars.push_back(normal(T0 + 180 * 25, 7700));
    c.pass();
    const auto T = texts(c.H);
    const mock::Draw* r = textAtBar(T, c.H, 24, -1); const mock::Draw* g = textAtBar(T, c.H, 24, +1);
    CHECK(r && r->text == "90" && r->color == C_SELL && std::abs((r->t + r->b) / 2 - yOf(c.H, 7700.25f)) <= 1);
    CHECK(g && g->text == "70" && g->color == C_BUY && std::abs((g->t + g->b) / 2 - yOf(c.H, 7700.75f)) <= 1);
    CHECK(r && r->r < xL(c.H, 24) && g && g->l > xL(c.H, 24) + c.H.barW / 2);
    for (const auto& d : T) CHECK(d.font == 11);                                    // default 11 pt
    CHECK(rings(c.H).empty());
    // no dash / tick: nothing but text and (here none) rings are drawn
    bool other = false; for (const auto& d : c.H.draws) if (d.type != "ctext" && d.type != "ring") other = true;
    CHECK(!other);
}
// [2] ties -> nearest the close, through the host; the 6E tick (0.00005) and float prices -> whole-tick keys
static void t2_ties_tick() {
    wipe(); Chart c; c.H.symbol = "6EZ6"; c.H.root = "EU"; c.H.tick = 0.00005f; c.H.priceBase = 1.1700f; c.H.pxPerTick = 10;
    for (int i = 0; i < 22; ++i) c.H.bars.push_back(bar(T0 + 180 * i, 1.1701f, 1.17025f, 1.17005f, 1.1702f, { { 1.17005f, 5, 40 }, { 1.1701f, 3, 40 }, { 1.17015f, 6, 7 }, { 1.1702f, 40, 2 }, { 1.17025f, 40, 1 } }));
    c.H.bars.push_back(normal(T0 + 180 * 22, 1.17f));
    c.pass();
    const auto T = texts(c.H);
    const mock::Draw* r = textAtBar(T, c.H, 5, -1); const mock::Draw* g = textAtBar(T, c.H, 5, +1);
    // bid tie 1.17005 / 1.1701 (close 1.1702): 1.1701 is nearer; ask tie 1.1702 / 1.17025: 1.1702 (the close itself)
    CHECK(r && std::abs((r->t + r->b) / 2 - yOf(c.H, 1.1701f)) <= 1);
    CHECK(g && std::abs((g->t + g->b) / 2 - yOf(c.H, 1.1702f)) <= 1);
    BBState* S = c.S(); CHECK(S && S->market == "EU" && std::fabs(S->tick - 0.00005) < 1e-12);
    const auto it = S->fin.find(T0 + 180 * 5);
    CHECK(it != S->fin.end() && it->second.bid.key == 23402 && it->second.ask.key == 23404);
}
// [3] bold = past only; later candles never change it
static void t3_bold() {
    wipe(); Chart c;
    for (int i = 0; i < 25; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700, 100, 50));
    c.H.bars.push_back(normal(T0 + 180 * 25, 7700, 250, 99));      // 2.5x bid, 1.98x ask
    c.H.bars.push_back(normal(T0 + 180 * 26, 7700, 100, 50));
    c.pass();
    auto T = texts(c.H);
    const mock::Draw* r = textAtBar(T, c.H, 25, -1); const mock::Draw* g = textAtBar(T, c.H, 25, +1);
    CHECK(r && r->text == "250" && r->bold); CHECK(g && g->text == "99" && !g->bold);
    const mock::Draw* r10 = textAtBar(T, c.H, 10, -1); CHECK(r10 && !r10->bold);
    // the first 9 candles have under 10 priors: never bold even when big
    // later huge candles do not touch candle 25
    for (int i = 27; i < 40; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700, 5000, 5000));
    c.pass(); T = texts(c.H);
    r = textAtBar(T, c.H, 25, -1); CHECK(r && r->text == "250" && r->bold);
    const auto it = c.S()->fin.find(T0 + 180 * 25); CHECK(it != c.S()->fin.end() && it->second.bidQ == 250 && it->second.askQ == 198);
    // the hover line 1 for that candle
    std::vector<std::string> L = bba::hover(it->second, 0.25, 200, "9:15 AM", -1);
    CHECK(L[0] == "Biggest bid 7,700.25: 250 lots sold at the bid (2.5x normal) \xB7 biggest ask 7,700.75: 99 lots (2.0x)");
}
// [4] absorption circles: bullish lower half + green close above; upper half / doji / red close / forming -> none; bearish mirror
static void t4_absorption() {
    wipe(); Chart c;
    for (int i = 0; i < 22; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700, 100, 100));
    // 22: bullish: bid 410 at 7700.25 (lower half of 7700-7702), opens 7700.5 closes 7701.5 green
    c.H.bars.push_back(bar(T0 + 180 * 22, 7700.5f, 7702, 7700, 7701.5f, { { 7700.0f, 5, 30 }, { 7700.25f, 10, 410 }, { 7701.0f, 60, 20 }, { 7702.0f, 40, 5 } }));
    // 23: bid 270 low but the candle closes red -> bold, no circle
    c.H.bars.push_back(bar(T0 + 180 * 23, 7701.5f, 7702, 7700, 7700.5f, { { 7700.25f, 10, 270 }, { 7701.0f, 60, 20 } }));
    // 24: upper half bid, green close -> none
    c.H.bars.push_back(bar(T0 + 180 * 24, 7700.5f, 7702, 7700, 7701.75f, { { 7701.5f, 10, 400 }, { 7701.0f, 60, 20 } }));
    // 25: doji -> none
    c.H.bars.push_back(bar(T0 + 180 * 25, 7701, 7702, 7700, 7701, { { 7700.25f, 10, 400 }, { 7701.0f, 60, 20 } }));
    // 26: bearish: ask 320 at 7701.75 (upper half), opens 7701.5 closes 7700.5 red
    c.H.bars.push_back(bar(T0 + 180 * 26, 7701.5f, 7702, 7700, 7700.5f, { { 7701.75f, 320, 10 }, { 7700.5f, 20, 60 } }));
    // 27: forming, would be bullish if closed -> no circle
    c.H.bars.push_back(bar(T0 + 180 * 27, 7700.5f, 7702, 7700, 7701.5f, { { 7700.25f, 10, 410 }, { 7701.0f, 60, 20 } }));
    c.pass();
    auto R = rings(c.H); auto T = texts(c.H);
    CHECK(R.size() == 2);
    const mock::Draw* r22 = textAtBar(T, c.H, 22, -1); const mock::Draw* r23 = textAtBar(T, c.H, 23, -1); const mock::Draw* g26 = textAtBar(T, c.H, 26, +1);
    CHECK(r22 && r22->text == "410" && r22->bold && r23 && r23->text == "270" && r23->bold && g26 && g26->text == "320" && g26->bold);
    bool ring22 = false, ring26 = false;
    for (const auto& q : R) {
        if (r22 && q.color == C_SELL && q.l == r22->l - 3 && q.r == r22->r + 3 && q.t == r22->t - 1 && q.b == r22->b + 1) ring22 = true;   // hugs the number, its colour
        if (g26 && q.color == C_BUY && q.l == g26->l - 3 && q.r == g26->r + 3) ring26 = true;
    }
    CHECK(ring22 && ring26);
    // the forming candle closes (a new bar arrives): it gets its circle now, and nothing earlier changes
    std::set<std::string> before; for (const auto& d : c.H.draws) if (d.type == "ring") before.insert(key(d));
    c.H.bars.push_back(normal(T0 + 180 * 28, 7700, 100, 100));
    c.pass(); R = rings(c.H);
    CHECK(R.size() == 3);
    std::set<std::string> after; for (const auto& d : R) after.insert(key(d));
    for (const auto& k : before) CHECK(after.count(k));
    // circle setting off -> no rings, numbers stay
    std::ofstream(HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings-ES.txt") << "BBA1|11|200|0|1\n";
    c.S()->setCheckMs = -1; c.H.bars.push_back(normal(T0 + 180 * 29, 7700, 100, 100)); c.pass();
    CHECK(rings(c.H).empty() && textAtBar(texts(c.H), c.H, 22, -1) != nullptr);
}
// [5] no repaint: bar-by-bar replay with a restart after every bar == one straight pass; a VAP change after the close changes nothing
static std::vector<mock::Bar> series(unsigned seed, int n) {
    std::mt19937 rng(seed); std::vector<mock::Bar> v; float p = 7700;
    for (int i = 0; i < n; ++i) {
        const float o = p, c = p + 0.25f * (float)((int)(rng() % 9) - 4);
        const float h = std::max(o, c) + 0.25f * (float)(rng() % 4), l = std::min(o, c) - 0.25f * (float)(rng() % 4);
        std::vector<mock::VRow> rows;
        for (float q = l; q <= h + 0.001f; q += 0.25f) rows.push_back({ q, (long)(rng() % 80 + (rng() % 15 == 0 ? 300 : 0)), (long)(rng() % 80 + (rng() % 15 == 0 ? 300 : 0)) });
        v.push_back(bar(T0 + 180 * i, o, h, l, c, rows)); p = c;
    }
    return v;
}
static void t5_norepaint() {
    const std::vector<mock::Bar> all = series(11, 70);
    wipe();
    std::set<std::string> straight;
    { Chart c; c.H.barW = 40; c.H.bars = all; c.pass(); for (const auto& d : c.H.draws) if (d.type != "rect") straight.insert(key(d)); }
    int steps = 0, ok = 1;
    wipe();
    { Chart c; c.H.barW = 40;
      for (size_t k = 1; k <= all.size(); ++k) {
          c.H.bars.assign(all.begin(), all.begin() + (long)k);
          if (k > 1) { auto& f = c.H.bars.back(); auto rows = f.rows; f.rows.resize(f.rows.size() / 2 + 1); c.pass(); f.rows = rows; }   // seen half-formed first
          c.pass(); ++steps;
          // every closed candle's number / circle drawn now is drawn identically at the end
          for (const auto& d : c.H.draws) if (d.type != "rect" && d.r < xL(c.H, (int)k - 1) - 1 && d.l < xL(c.H, (int)k - 1) - 40 && !straight.count(key(d))) ok = 0;
          c.restart();
      }
      c.pass();
      std::set<std::string> end; for (const auto& d : c.H.draws) if (d.type != "rect") end.insert(key(d));
      CHECK(end == straight);
      // IRT corrects a closed candle's volume at price afterwards: the record wins, nothing moves
      c.H.bars[30].rows[0].bid += 5000; c.H.bars.push_back(normal(T0 + 180 * 70, 7700)); c.pass(); c.H.bars.pop_back(); c.H.bars[30].rows[0].bid -= 5000;
      std::set<std::string> after; for (const auto& d : c.H.draws) if (d.type != "rect" && d.l < xL(c.H, 69) - 40) after.insert(key(d));
      std::set<std::string> base; for (const auto& k : straight) base.insert(k);
      bool sub = true; for (const auto& k : after) if (!base.count(k)) sub = false;
      CHECK(sub);
    }
    CHECK(ok == 1 && steps == 70);
    // the record exists with one line per closed candle
    const std::string rec = slurp(HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk/ES-ESZ6-180-" + std::to_string((long long)((T0 - 17 * 3600) / 86400 + 1)) + ".csv");
    size_t lines = 0; for (char ch : rec) if (ch == '\n') ++lines;
    CHECK(rec.compare(0, std::strlen(bba::REC_HEADER), bba::REC_HEADER) == 0 && lines == 71);   // header + 70 (the 71st bar closed the 70th)
}
// [6] the overlap rule on a dense chart: no two numbers touch, none on a candle unless bold-without-room, non-bold dropped first
static void t6_overlap() {
    for (int bw : { 40, 24, 14, 8 }) {
        wipe(); Chart c; c.H.barW = bw; c.H.bars = series(5 + (unsigned)bw, 90); c.pass();
        const auto T = texts(c.H); BBState* S = c.S();
        bool clean = true;
        for (size_t i = 0; i < T.size(); ++i) for (size_t j = i + 1; j < T.size(); ++j)
            if (T[i].l <= T[j].r && T[j].l <= T[i].r && T[i].t <= T[j].b && T[j].t <= T[i].b) clean = false;
        CHECK(clean);
        // inside the pane, under the title row, left of the price axis
        for (const auto& d : T) CHECK(d.t >= 20 && d.b <= c.H.paneBottom && d.r < c.H.paneRight - c.H.scaleW - 2 && d.l >= 0);
        // a number that is not bold never touches a candle (a bold one may, only when it had no room - "show only the bold ones")
        int onCandle = 0, boldOnCandle = 0;
        for (const auto& d : T) for (size_t i = 0; i < c.H.bars.size(); ++i) {
            const int l = xL(c.H, (int)i), r = l + c.H.barW / 2, t = yOf(c.H, c.H.bars[i].high), b = yOf(c.H, c.H.bars[i].low);
            if (d.l <= r && l <= d.r && d.t <= b && t <= d.b) { if (d.bold) ++boldOnCandle; else ++onCandle; break; }
        }
        CHECK(onCandle == 0);
        CHECK(boldOnCandle <= S->lay.crowded);
        CHECK(S->lay.shown == (int)T.size());
        if (bw == 8) CHECK(S->lay.dropped > S->lay.shown);                          // a crowded chart: most plain numbers give way
        if (bw == 40) CHECK(S->lay.dropped < S->lay.shown);
    }
}
// [7] settings: written ONLY from the settings window, read by the chart from the per-market file; every setting does what it says
static void t7_settings() {
    wipe(); Chart c;
    for (int i = 0; i < 25; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700, 100, 100));
    c.H.bars.push_back(normal(T0 + 180 * 25, 7700, 250, 100));
    c.H.bars.push_back(normal(T0 + 180 * 26, 7700, 100, 100));
    const std::string file = HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings-ES.txt";
    c.pass();
    // the chart copy's Apply (IRT calls it with every box unchecked / 0): writes nothing
    c.use(); c.H.inDialog = false; c.x->parmsApply(); CHECK(slurp(file).empty());
    // the dialog opens: filled from the defaults (no file), nothing saved by opening
    c.H.inDialog = true; c.x->parmsLoad();
    CHECK(c.H.ints[0] == 11 && std::fabs(c.H.floats[1] - 2.0f) < 1e-6 && c.H.boxes[2] && c.H.boxes[3] && slurp(file).empty());
    // he sets font 14, bold 2.5x, circle off, non-bold off, OK
    c.H.ints[0] = 14; c.H.floats[1] = 2.5f; c.H.boxes[2] = false; c.H.boxes[3] = false; c.x->parmsApply();
    CHECK(slurp(file) == "BBA1|14|250|0|0\n");
    c.H.inDialog = false; c.S()->setCheckMs = -1; c.H.bars.push_back(normal(T0 + 180 * 27, 7700, 100, 100)); c.pass();
    auto T = texts(c.H);
    CHECK(T.size() == 1 && T[0].text == "250" && T[0].font == 14 && T[0].bold);    // only the bold one (2.5x >= 2.5x), at 14 pt
    // bold 3.0x: 250 no longer bold -> nothing drawn (non-bold off)
    c.H.inDialog = true; c.x->parmsLoad(); CHECK(c.H.ints[0] == 14 && std::fabs(c.H.floats[1] - 2.5f) < 1e-6 && !c.H.boxes[2] && !c.H.boxes[3]);
    c.H.floats[1] = 3.0f; c.x->parmsApply(); usleep(20000); CHECK(slurp(file) == "BBA1|14|300|0|0\n");
    c.H.inDialog = false; c.S()->setCheckMs = -1; c.H.bars.push_back(normal(T0 + 180 * 28, 7700, 100, 100)); c.pass();
    CHECK(texts(c.H).empty());
    // show non-bold on again: every number back, font 14
    c.H.inDialog = true; c.x->parmsLoad(); c.H.boxes[3] = true; c.x->parmsApply(); usleep(20000); CHECK(slurp(file) == "BBA1|14|300|0|1\n");
    c.H.inDialog = false; c.S()->setCheckMs = -1; c.H.bars.push_back(normal(T0 + 180 * 29, 7700, 100, 100)); c.pass();
    T = texts(c.H); CHECK(T.size() > 20); for (const auto& d : T) CHECK(d.font == 14);
    // garbage typed (font 99, bold 0.1x) never reaches the file; a control IRT handed back unchanged keeps the file's value
    c.H.inDialog = true; c.x->parmsLoad(); c.H.ints[0] = 99; c.H.floats[1] = 0.1f; c.x->parmsApply(); usleep(20000); CHECK(slurp(file) == "BBA1|14|300|0|1\n");
    c.x->parmsLoad(); std::ofstream(file) << "BBA1|12|200|1|1\n";                    // the file changed while the dialog was open
    c.x->parmsApply(); usleep(20000); CHECK(slurp(file) == "BBA1|12|200|1|1\n");                     // untouched controls keep the file's values
    // a garbled file = defaults, said in the status
    std::ofstream(file) << "nonsense\n"; c.H.inDialog = false; c.S()->setCheckMs = -1; c.H.bars.push_back(normal(T0 + 180 * 30, 7700, 100, 100)); c.pass();
    CHECK(c.S()->set.font == 11 && c.S()->settingsWhy.find("garbled") != std::string::npos);
    // per market: an NQ chart does not read the ES file
    std::ofstream(file) << "BBA1|16|200|1|1\n";
    { Chart n; n.H.symbol = "NQZ6"; n.H.root = "NQ"; n.H.chart = "NQ 3m"; n.H.bars = c.H.bars; n.pass(); CHECK(n.S()->set.font == 11); for (const auto& d : texts(n.H)) CHECK(d.font == 11); }
}
// [8] two charts on one DLL object kind (separate slots): no cross-talk; symbol change resets
static void t8_twocharts() {
    wipe(); Chart a, b;
    b.H.symbol = "NQZ6"; b.H.root = "NQ"; b.H.chart = "NQ 3m"; b.H.priceBase = 20000;
    for (int i = 0; i < 30; ++i) { a.H.bars.push_back(normal(T0 + 180 * i, 7700, 100, 100)); b.H.bars.push_back(normal(T0 + 180 * i, 20000, 70, 90)); }
    a.pass(); b.pass(); a.pass();
    CHECK(a.S() != b.S() && a.S()->market == "ES" && b.S()->market == "NQ");
    bool aok = true; for (const auto& d : texts(a.H)) if (d.text != "100") aok = false; CHECK(aok && !texts(a.H).empty());
    bool bok = true; for (const auto& d : texts(b.H)) if (d.text != "70" && d.text != "90") bok = false; CHECK(bok && !texts(b.H).empty());
    // symbol change on chart a: old facts gone
    a.H.symbol = "CLZ6"; a.H.root = "CL"; a.H.tick = 0.01f; a.H.priceBase = 65; a.H.bars.clear();
    for (int i = 0; i < 30; ++i) a.H.bars.push_back(normal(T0 + 180 * i, 65, 30, 40));
    a.pass(); CHECK(a.S()->market == "CL"); for (const auto& d : texts(a.H)) CHECK(d.text == "30" || d.text == "40");
}
// [9] the right-side columns (ColumnBoard.h) and the left Liquidity column: nothing drawn in them; the Tape Reader banner avoided
static void t9_columns() {
    wipe(); Chart c; c.H.barW = 30; c.H.paneRight = 1100;
    for (int i = 0; i < 30; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700, 100, 100));
    c.pass(); const int lastR = (int)texts(c.H).back().r;
    // a Dealer Profile (80) + Delta (80) + Levels (78) publish their columns for this chart
    const long long now = steadyMs();
    cb::publish(cb::DEALER, "ESZ6", 180, "ES 3m", now, 80); cb::publish(cb::DELTA, "ESZ6", 180, "ES 3m", now, 80); cb::publish(cb::LEVELS, "ESZ6", 180, "ES 3m", now, 78);
    cb::publish(cb::LIQ, "ESZ6", 180, "ES 3m", now, 60);
    c.pass();
    const int axisR = c.H.paneRight - c.H.scaleW - 2, priceRight = axisR - 80 - 80 - 78 - cb::GAP_PX;
    CHECK(c.S()->priceRight == priceRight && c.S()->leftLimit == 62);
    bool ok = true; for (const auto& d : texts(c.H)) if (d.r >= priceRight || d.l < 62) ok = false;
    CHECK(ok && lastR >= priceRight);                                              // without the columns it went further right
    // the Tape Reader banner over the top left: no number under it
    bb::Rect r; r.l = 0; r.t = 0; r.r = 1100; r.b = 640; bb::publishBanner("ESZ6", 180, "ES 3m", steadyMs(), r);
    c.pass(); ok = true; for (const auto& d : texts(c.H)) if (d.t <= 640) ok = false; CHECK(ok);
    cb::clear(cb::DEALER, "ESZ6", 180, "ES 3m"); cb::clear(cb::DELTA, "ESZ6", 180, "ES 3m"); cb::clear(cb::LEVELS, "ESZ6", 180, "ES 3m"); cb::clear(cb::LIQ, "ESZ6", 180, "ES 3m");
    bb::clearBanner("ESZ6", 180, "ES 3m");
}
// [10] hover: over a number -> the tooltip (line 1 Rassul's format, the why / how / example rows); elsewhere -> none
static void t10_hover() {
    wipe(); Chart c;
    for (int i = 0; i < 22; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700, 100, 100));
    c.H.bars.push_back(bar(T0 + 180 * 22, 7700.5f, 7702, 7700, 7701.5f, { { 7700.25f, 10, 410 }, { 7701.0f, 60, 20 } }));
    c.H.bars.push_back(normal(T0 + 180 * 23, 7700, 100, 100));
    c.pass();
    const auto T0v = texts(c.H); const mock::Draw* r = textAtBar(T0v, c.H, 22, -1); CHECK(r != nullptr); if (!r) return;
    const mock::Draw rr = *r;
    hb::fakeCursor() = 77;
    RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((rr.l + rr.r) / 2); e.v.mouse.v = (short)((rr.t + rr.b) / 2);
    c.use(); CHECK(c.x->mouse(&e) == RTX_FAIL);                                    // never swallows the event
    c.pass();
    std::vector<std::string> tip; for (const auto& d : c.H.draws) if (d.type == "text") tip.push_back(d.text);
    CHECK(tip.size() == 10 && tip[0].find("Biggest bid 7,700.25: 410 lots sold at the bid (4.1x normal)") == 0 && tip[2].find("ABSORPTION") == 0);
    bool bg = false; for (const auto& d : c.H.draws) if (d.type == "rect") bg = true; CHECK(bg);
    e.v.mouse.h = 5; e.v.mouse.v = 5; c.x->mouse(&e); c.pass();
    int n = 0; for (const auto& d : c.H.draws) if (d.type == "text") ++n; CHECK(n == 0);
    hb::fakeCursor() = 0;
}
// [11] a candle IRT has no volume at price for: no numbers, never recorded; numbers once it arrives
static void t11_novap() {
    wipe(); Chart c;
    for (int i = 0; i < 25; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700, 100, 100));
    c.H.bars[20].noVap = true;
    c.pass();
    CHECK(textAtBar(texts(c.H), c.H, 20, -1) == nullptr && !c.S()->fin.count(T0 + 180 * 20));
    c.H.bars[20].noVap = false; c.H.bars.push_back(normal(T0 + 180 * 25, 7700, 100, 100)); c.pass();
    CHECK(textAtBar(texts(c.H), c.H, 20, -1) != nullptr && c.S()->fin.count(T0 + 180 * 20));
    // candles after a missing one wait for it (their 20 priors must be known) - then decided once
    CHECK(c.S()->fin.count(T0 + 180 * 21));
}
// [12] the status file and the version everywhere (BUILD-LESSONS #70)
static void t12_status_version() {
    wipe(); Chart c; for (int i = 0; i < 25; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700));
    c.pass();
    const std::string st = slurp(HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.status-ES-ESZ6-180.txt");
    CHECK(st.find("VERSION,1.0.0\n") == 0 && st.find("SETTINGS,font 11,bold 2.0x,circle 1,non-bold 1") != std::string::npos && st.find("DRAWN,numbers") != std::string::npos);
    const std::string src = slurp("../BigBidAsk.cpp");
    CHECK(src.find("#define BBA_VERSION \"1.0.0\"") != std::string::npos && src.find("p->setVersion(\"1.0.0\");") != std::string::npos &&
          src.find("v1.0.0:") != std::string::npos && std::string(bba::VERSION) == "1.0.0" && std::string(BBA_VERSION) == bba::VERSION);
}
// [13] volumes above 2^31 lots in the facts / record path, and a pane too small draws nothing
static void t13_edges() {
    wipe(); Chart c; c.H.paneBottom = 50;
    for (int i = 0; i < 25; ++i) c.H.bars.push_back(normal(T0 + 180 * i, 7700));
    c.pass();
    CHECK(texts(c.H).empty() && c.S()->state.find("pane too small") != std::string::npos);
    bba::Facts F; std::vector<bba::Row> rows(1); rows[0].key = 5; rows[0].bid = 4000000000LL; F = bba::facts(1, true, 4, 6, 4, 6, rows, true);
    bba::Facts G; CHECK(bba::parseRec(bba::recLine(F), G) && G.bid.vol == 4000000000LL);
}

int main() {
    char tmpl[] = "/tmp/claude-0/bbaXXXXXX"; mkdir("/tmp/claude-0", 0755);
    const char* d = mkdtemp(tmpl); if (!d) { std::printf("no temp dir\n"); return 1; }
    HOME = d; setenv("USERPROFILE", d, 1);
    t1_sign(); t2_ties_tick(); t3_bold(); t4_absorption(); t5_norepaint(); t6_overlap(); t7_settings(); t8_twocharts(); t9_columns(); t10_hover(); t11_novap(); t12_status_version(); t13_edges();
    std::string cmd = std::string("rm -rf '") + d + "'"; if (std::system(cmd.c_str()) != 0) std::printf("cleanup failed\n");
    std::printf("bigba host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
