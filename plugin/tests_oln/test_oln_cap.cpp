// test_oln_cap.cpp -- lsLiquidityProfile 0.60.1 THE LIVE-LINE CAP (Rassul 13:52 GC 'a total disaster to read'): only the 3 biggest live lines per side are drawn; was test_oln_host.cpp 0.55.1 (0.55.0) THE LIMIT-ORDER LINES on the mock host (order feed -> OrderLines engine -> draw / hover /
// status / record / settings / restart). ES 3-min; a 100-lot bid rolled 7,675 -> 7,680, filled, green close above = HELD; a 50-lot offer
// pulled = very dark line; no LMT column; no RE-BID? / ACCEPTED / X.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_oln/test_oln_host.cpp -o tolnh && ASAN_OPTIONS=detect_leaks=0 ./tolnh
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static bool hasText(mock::Host& h, const std::string& t, COLOR c = 0xFFFFFFFF) { for (auto& d : h.draws) if (d.type == "text" && d.text == t && (c == 0xFFFFFFFF || d.color == c)) return true; return false; }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static void bar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; b.rows.push_back(mock::Price(c, 50, 40, 90)); h.bars.push_back(b); }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/olncXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 22) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26";
    h.priceBase = 7660.0f; h.pxPer = 20.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 12 * 3600 + 30 * 60;
    for (int i = 0; i < 31; ++i) bar(h, t0 - (30 - i) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(7689.75 - i * 0.25); L.ask = (float)(7690.0 + i * 0.25); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    const int lots[6] = { 60, 110, 70, 100, 80, 90 };
    for (int k = 0; k < 6; ++k) mock::feed.push(0, 0, 7700.0f + 0.5f * k, lots[k], (RTDATE)(t0 - 60 + 2 * k), 10 + k);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 5; ++s) { h.now = t0 - 50 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    for (int i = 31; i < 35; ++i) bar(h, t0 + (i - 30) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    for (int s = 0; s < 4; ++s) { h.now = t0 + 601 + s; p->calc(0); usleep(5000); }
    CHECK(S->ol.tally.drawn == 6, (std::string("six offers tracked + drawable (") + std::to_string(S->ol.tally.drawn) + ")").c_str());
    h.draws.clear(); p->draw();
    const COLOR RED = 0xEF4444; int on[6] = { 0, 0, 0, 0, 0, 0 };
    for (int k = 0; k < 6; ++k) { const int y = (int)(h.paneBottom - (7700.0f + 0.5f * k - h.priceBase) * h.pxPer);
        for (auto& d : h.draws) if (d.type == "line" && d.pen == RED && std::abs(d.y - y) <= 1) on[k] = 1; }
    CHECK(on[1] && on[3] && on[5], "the 3 biggest (110, 100, 90) are drawn");
    CHECK(!on[0] && !on[2] && !on[4], "the smaller three (60, 70, 80) are not drawn");
    CHECK(S->olCutNow == 3, (std::string("status: 3 live lines cut (") + std::to_string(S->olCutNow) + ")").c_str());
    std::printf("lsLiquidityProfile 0.60.1 live-line cap: %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
