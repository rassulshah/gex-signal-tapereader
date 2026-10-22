// test_bba118.cpp -- lsBigBidAsk 1.18.0 (Rassul 2026-10-20 21:26-21:32): ONE print rule.
//   [logic] bba::biggestOfLast: strictly bigger than EACH of the 10 candles before it; a tie or fewer than 10 known = no print
//   [host]  a quiet chart (every cell 10 lots) with planted cells: a cell bigger than each of the 10 before prints (bid and ask judged
//           on their own); an equal one right after does not; the first / last 15 minutes of the session never print; the numbers are
//           lots / 10 on ES, white, font 7; the SignalBoard B events = the same prints
// Build (from tests_bba115/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_bba118.cpp -o /tmp/t_bba118 && /tmp/t_bba118
#include "irtsdk.h"
#include "../BigBidAsk.cpp"
#include <iostream>
#include <sys/stat.h>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)

static void logic() {
    std::vector<long long> p(10, 100);
    CHECK(bba::biggestOfLast(p, 101), "[logic] 101 beats ten 100s");
    CHECK(!bba::biggestOfLast(p, 100), "[logic] a tie is not bigger");
    p[9] = 500; CHECK(!bba::biggestOfLast(p, 400), "[logic] one bigger candle 10 back blocks it");
    CHECK(bba::biggestOfLast(p, 501), "[logic] beats all ten");
    std::vector<long long> q(9, 1); CHECK(!bba::biggestOfLast(q, 999), "[logic] fewer than 10 known = no print");
    CHECK(!bba::biggestOfLast(std::vector<long long>(10, 0), 0), "[logic] zero lots never prints");
    std::vector<long long> r(12, 100); r[10] = r[11] = 9999;          // only the 10 just before count
    CHECK(bba::biggestOfLast(r, 101), "[logic] only the last 10 candles count");
    CHECK(bba::TOP_N == 10 && std::string(bba::VERSION) >= "1.18.0", "[logic] look-back 10, version 1.18.0 or later");   // (2026-10-22) the version moves on with each approved build (0.47.1 now): at least this one
}
static const unsigned long DAY = 86400, START = 1791133200UL;   // Sun 2026-10-04 17:00
static std::vector<mock::Bar> quiet(int todayBars, std::map<int, std::pair<long, long>> plant) {
    std::vector<mock::Bar> v;
    auto add = [&](unsigned long open, int n, bool today) {
        for (int i = 1; i <= n; ++i) {
            mock::Bar b; b.time = open + (unsigned long)i * 180; b.open = 7700; b.low = 7699; b.high = 7701; b.close = 7700.5f;
            for (int k = 0; k <= 8; ++k) { mock::VRow x; x.price = b.low + 0.25f * (float)k; x.ask = 10; x.bid = 10; b.rows.push_back(x); }
            if (today && plant.count(i)) { b.rows[2].bid = plant[i].first ? plant[i].first : 10; b.rows[6].ask = plant[i].second ? plant[i].second : 10; }
            for (const auto& x : b.rows) b.volume += (unsigned long)(x.ask + x.bid);
            v.push_back(b);
        }
    };
    unsigned long open = START;
    for (int d = 0; d < 12; ++d, open += DAY) { const int wd = d % 7; if (wd == 5 || wd == 6) continue; add(open, 460, false); }
    add(START + 14 * DAY, todayBars, true);
    return v;
}
struct Chart {
    mock::Host H; BigBidAsk* x = nullptr;
    explicit Chart(const std::vector<mock::Bar>& b) { H.bars = b; H.now = b.back().time; H.symbol = "ESZ6"; H.root = "ES"; H.chart = "ES 3m";
        mock::current = &H; x = static_cast<BigBidAsk*>(CreateExtension()); x->setup(); }
    ~Chart() { mock::current = &H; x->destroy(); delete x; mock::current = nullptr; }
    void pump(int n, bool draw) {
        for (int k = 0; k < n; ++k) { mock::current = &H; BBState* s = x->testState();
            if (s) { s->builtMs = -1; s->boardMs = -1; s->sdCheckMs = -1; s->statsMs = -1; s->lastStatusMs = -1; s->logMs = -1; }
            if (draw) { H.draws.clear(); x->draw(); } else x->calc(0); }
    }
};
static void home(const std::string& name) {
    const std::string root = "/tmp/bba118-run", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str()) != 0) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
}
int main() {
    setenv("TZ", "UTC", 1); tzset();
    logic();
    // today's bar i ends at 17:00 + 3 i min: i = 312 -> 8:36 (first 15 min of RTH), 325 -> 9:15, 340 -> 10:00, 341 -> 10:03,
    // 350 -> 10:30, 437 -> 14:51 (last 15 min). 440 bars: the last (15:00) is forming.
    std::map<int, std::pair<long, long>> plant = { {312, {900, 0}}, {325, {700, 0}}, {340, {500, 0}}, {341, {500, 0}}, {350, {0, 300}}, {437, {900, 0}} };
    const std::vector<mock::Bar> bars = quiet(440, plant);
    home("rule");
    Chart c(bars); c.pump(12, false); c.pump(2, true);
    auto view = [&](int v0, int v1) {   // draw with the view on today's bars v0..v1 (40 bars fit the mock pane)
        const int base = (int)c.H.bars.size() - 440;
        c.H.visStart = base + v0; c.H.visEnd = base + v1; c.H.barOffset = base + v0; c.pump(2, true);
        std::map<std::string, int> seen; int font7 = 0, numbers = 0;
        for (const auto& d : c.H.draws) if (d.type == "ctext" && !d.text.empty() && std::isdigit((unsigned char)d.text[0])) { if (!seen.count(d.text)) { ++numbers; if (d.font == 7) ++font7; } seen[d.text] = 1; }
        if (getenv("DBG")) for (const auto& d : c.H.draws) if (d.type.find("text") != std::string::npos) std::cerr << "  " << d.type << " [" << d.text << "] f" << d.font << "\n";
        if (getenv("DBG")) { BBState* S = c.x->testState(); std::cerr << v0 << ".." << v1 << " shown " << S->lay.shown << " dropped " << S->lay.dropped << " clL " << S->clL << " clR " << S->clR << "\n"; }
        seen["#n"] = numbers; seen["#f7"] = font7; return seen; };
    auto A = view(305, 345);
    CHECK(A.count("70") == 1, "[host] 9:15: 700 sold beats the 10 candles before -> 70 (lots / 10 on ES; the skipped 8:36 candle is 11 back)");
    CHECK(A.count("50") == 1 && c.x->testState()->lay.shown == 2, "[host] 10:00: 500 prints; 10:03's equal 500 does not (a tie is not bigger): 2 prints in view");
    CHECK(A.count("90") == 0, "[host] 8:36 (first 15 min of RTH): never printed");
    CHECK(A["#n"] == 2 && A["#f7"] == 2, "[host] only those 2 numbers in view, font 7");
    auto B = view(345, 385);
    CHECK(B.count("30") == 1 && B["#n"] == 1, "[host] 10:30: the ask side judged on its own (300 bought)");
    auto C = view(400, 439);
    CHECK(C.count("90") == 0 && C["#n"] == 0, "[host] 14:51 (last 15 min): never printed");
    std::vector<sb::Ev> ev; sb::readAll('B', c.H.symbol, c.H.seconds, c.H.chart, -1, ev);
    int b50 = 0, a30 = 0, b90 = 0; for (const auto& e : ev) { if (e.side < 0 && e.val == 500) ++b50; if (e.side > 0 && e.val == 300) ++a30; if (e.val == 900) ++b90; }
    CHECK(b50 == 1 && a30 == 1 && b90 == 0, "[host] SignalBoard B events = the same prints");
    { const std::string src = [] { std::ifstream f("../BigBidAsk.cpp", std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }();
      CHECK(std::string(BBA_VERSION) == bba::VERSION && src.find(std::string("p->setVersion(\"") + bba::VERSION + "\")") != std::string::npos, "[ver] one version everywhere"); }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
