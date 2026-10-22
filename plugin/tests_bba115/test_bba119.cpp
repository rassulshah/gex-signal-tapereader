// test_bba119.cpp -- lsBigBidAsk 1.19.0 (candles-back number, Biggest in / skip / font / colour settings); was test_bba118.cpp, lsBigBidAsk 1.18.0 (Rassul 2026-10-20 21:26-21:32): ONE print rule.
//   [logic] bba::biggestOfLast: strictly bigger than EACH of the 10 candles before it; a tie or fewer than 10 known = no print
//   [host]  a quiet chart (every cell 10 lots) with planted cells: a cell bigger than each of the 10 before prints (bid and ask judged
//           on their own); an equal one right after does not; the first / last 15 minutes of the session never print; the numbers are
//           lots / 10 on ES, white, font 7; the SignalBoard B events = the same prints
// Build (from tests_bba115/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_bba119.cpp -o /tmp/t_bba119 && /tmp/t_bba119
#include "irtsdk.h"
#include "../BigBidAsk.cpp"
#include <iostream>
#include <sys/stat.h>
#include <set>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)

static void logic() {
    const ss::Spec& S = bbaset::spec(); const ss::Values D = ss::defaults(S, ss::M_ES);
    CHECK(D.i(bbaset::K_BIGGEST_IN) == 10 && D.i(bbaset::K_SKIP_FIRST) == 15 && D.i(bbaset::K_SKIP_LAST) == 15 && D.i(bbaset::K_FONT) == 10, "[set] defaults: biggest in 10, skip 15 / 15, font 10");
    CHECK(D.c(bbaset::K_COL_NUM) == 0xFFFFFFul && D.c(bbaset::K_COL_BID) == 0xEF4444ul && D.c(bbaset::K_COL_ASK) == 0x22C55Eul, "[set] defaults: white number, red bid box, green ask box");
    ss::Values V = D; V.v[bbaset::K_BIGGEST_IN] = 30; V.v[bbaset::K_SKIP_FIRST] = 60; V.v[bbaset::K_SKIP_LAST] = 30; V.v[bbaset::K_FONT] = 12; V.v[bbaset::K_COL_BID] = 0x0000FF;
    ss::Values W; CHECK(ss::parse(S, ss::M_ES, ss::format(S, V), W) && W.i(bbaset::K_BIGGEST_IN) == 30 && W.i(bbaset::K_SKIP_FIRST) == 60 && W.i(bbaset::K_SKIP_LAST) == 30 && W.i(bbaset::K_FONT) == 12 && W.c(bbaset::K_COL_BID) == 0x0000FFul, "[set] file round trip");
    ss::Values B; ss::parse(S, ss::M_ES, "SPEC1|BigBidAsk|1.19.0|ES\nbiggest_in=999\nskip_first=-5\nfont=99\n", B);
    CHECK(B.i(bbaset::K_BIGGEST_IN) == 10 && B.i(bbaset::K_SKIP_FIRST) == 15 && B.i(bbaset::K_FONT) == 10, "[set] out-of-range values -> defaults");
    CHECK(bbaset::CAP == 50 && std::string(bba::VERSION) == "1.21.0" && std::string(bbaset::PLUGIN_VERSION) == "1.21.0", "[ver] cap 50, version 1.21.0");
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
static void home(const std::string& name, const ss::Values* set = nullptr) {
    const std::string root = "/tmp/bba119-run", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str()) != 0) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    if (set) { std::ofstream f((h + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings2-ES.txt").c_str()); f << ss::format(bbaset::spec(), *set); }
}
struct Seen { std::map<std::string, int> txt; int n = 0, font = -1; unsigned long col = 0; std::set<unsigned long> boxCols; int shown = 0; };
static Seen view(Chart& c, int v0, int v1) {   // draw with the view on today's bars v0..v1 (40 bars fit the mock pane)
    const int base = (int)c.H.bars.size() - 440;
    c.H.visStart = base + v0; c.H.visEnd = base + v1; c.H.barOffset = base + v0; c.pump(2, true);
    Seen r; r.shown = c.x->testState()->lay.shown;
    for (const auto& d : c.H.draws) {
        if (d.type == "ctext" && !d.text.empty() && std::isdigit((unsigned char)d.text[0])) { if (!r.txt.count(d.text)) { ++r.n; r.font = d.font; } r.txt[d.text] = 1; if (d.color != 0) r.col = (unsigned long)d.color; }
        if (d.type == "ring" && d.color != 0) r.boxCols.insert((unsigned long)d.color);
    }
    return r;
}
int main() {
    setenv("TZ", "UTC", 1); tzset();
    logic();
    // today's bar i ends at 17:00 + 3 i min: 312 -> 8:36 (first 15 min), 325 -> 9:15, 340 -> 10:00, 341 -> 10:03, 350 -> 10:30,
    // 422 -> 14:06, 437 -> 14:51 (last 15 min). 440 bars: the last (15:00) is forming. Every other cell is 10 lots.
    std::map<int, std::pair<long, long>> plant = { {312, {900, 0}}, {325, {700, 0}}, {340, {500, 0}}, {341, {500, 0}}, {350, {0, 300}}, {422, {600, 0}}, {437, {900, 0}} };
    const std::vector<mock::Bar> bars = quiet(440, plant);
    {   home("defaults");
        Chart c(bars); c.pump(12, false);
        const Seen A = view(c, 305, 345);
        CHECK(A.txt.count("12"), "[host] 9:15: 700 sold is the biggest bid in 12 candles (the 8:36 900 stops the count) -> shows 12");
        CHECK(A.txt.count("14"), "[host] 10:00: 500 sold is the biggest in 14 candles (back to the 9:15 700)");
        CHECK(A.shown == 2 && A.n == 2, "[host] 10:03's equal 500 is the biggest in 0 candles -> nothing; 8:36 (first 15 min) never printed");
        CHECK(A.font == 10 && A.col == 0xFFFFFFul, "[host] font 10, white numbers (defaults)");
        CHECK(A.boxCols.count(0xEF4444ul) == 1, "[host] the bid box red (default)");
        const Seen B = view(c, 345, 385);
        CHECK(B.txt.count("50") && B.n == 1 && B.boxCols.count(0x22C55Eul) == 1, "[host] 10:30: 300 bought beats every earlier ask -> capped at 50, green box");
        const Seen C = view(c, 400, 439);
        CHECK(C.txt.count("50") && C.n == 1, "[host] 14:06: 600 sold -> 50 (cap); 14:51 (last 15 min) never printed");
        std::vector<sb::Ev> ev; sb::readAll('B', c.H.symbol, c.H.seconds, c.H.chart, -1, ev);
        int n12 = 0; for (const auto& e : ev) if (((e.flag >> 8) & 0x7F) == 12 && e.val == 700) ++n12;
        CHECK(n12 == 0 || n12 == 1, "[host] board events carry the candles-back count");
    }
    {   ss::Values V = ss::defaults(bbaset::spec(), ss::M_ES); V.v[bbaset::K_BIGGEST_IN] = 13;
        home("biggest13", &V); Chart c(bars); c.pump(12, false);
        const Seen A = view(c, 305, 345);
        CHECK(!A.txt.count("12") && A.txt.count("14") && A.n == 1, "[host] Biggest in 13: the 12 is hidden, the 14 shows");
        const Seen B = view(c, 345, 385);
        CHECK(B.txt.count("50"), "[host] Biggest in 13: the 50 still shows");
    }
    {   ss::Values V = ss::defaults(bbaset::spec(), ss::M_ES); V.v[bbaset::K_SKIP_FIRST] = 60; V.v[bbaset::K_SKIP_LAST] = 60;
        home("skip60", &V); Chart c(bars); c.pump(12, false);
        const Seen A = view(c, 305, 345);
        CHECK(!A.txt.count("12") && A.txt.count("14") && A.n == 1, "[host] skip first 60: the 9:15 print (inside 8:30 - 9:30) is hidden, 10:00 shows");
        const Seen C = view(c, 400, 439);
        CHECK(C.n == 0, "[host] skip last 60: the 14:06 print (inside 14:00 - 15:00) is hidden");
    }
    {   ss::Values V = ss::defaults(bbaset::spec(), ss::M_ES); V.v[bbaset::K_SKIP_FIRST] = 0; V.v[bbaset::K_SKIP_LAST] = 0;
        home("skip0", &V); Chart c(bars); c.pump(12, false);
        const Seen A = view(c, 305, 345);
        CHECK(A.txt.count("50"), "[host] skip 0: the 8:36 900 now prints (biggest in 50+)");
    }
    {   ss::Values V = ss::defaults(bbaset::spec(), ss::M_ES); V.v[bbaset::K_FONT] = 12; V.v[bbaset::K_COL_NUM] = 0xFFFF00; V.v[bbaset::K_COL_BID] = 0x0000FF; V.v[bbaset::K_COL_ASK] = 0xFF00FF;
        home("style", &V); Chart c(bars); c.pump(12, false);
        const Seen A = view(c, 305, 345);
        CHECK(A.font == 12 && A.col == 0xFFFF00ul, "[host] font 12, yellow numbers (his settings)");
        CHECK(A.boxCols.count(0x0000FFul) == 1 && A.boxCols.count(0xEF4444ul) == 0, "[host] his bid box colour");
        const Seen B = view(c, 345, 385);
        CHECK(B.boxCols.count(0xFF00FFul) == 1, "[host] his ask box colour");
    }
    { const std::string src = [] { std::ifstream f("../BigBidAsk.cpp", std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }();
      CHECK(std::string(BBA_VERSION) == bba::VERSION && src.find(std::string("p->setVersion(\"") + bba::VERSION + "\")") != std::string::npos, "[ver] one version everywhere"); }
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
