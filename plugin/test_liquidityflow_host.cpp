// test_liquidityflow_host.cpp -- the pane build (lsLiquidityFlow) (LiquidityProfile.cpp) driven by a mock Investor/RT host (mock/irtsdk.h):
// depth + VAP in, a zone touch, an LX decided on the closed bar, the record written, a restart redrawing the same mark.
//   g++ -std=c++17 -Imock -I. test_liquidityprofile_host.cpp -o th && ./th
#define LQ_FLOW 1
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>

namespace mock { Host* current = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)

static void setDepth(mock::Host& h, double bestBid, int levels, double extraPx = 0, double extra = 0) {
    h.depth.clear();
    for (int i = 0; i < levels; ++i) {
        DEPTH_LEVEL L; L.bid = (float)(bestBid - i * 0.01); L.ask = (float)(bestBid + 0.01 + i * 0.01);
        L.bidsize = 10; L.asksize = 10;
        if (extra > 0 && std::fabs(L.bid - extraPx) < 0.001) L.bidsize = (unsigned short)(10 + extra);
        h.depth.push_back(L);
    }
}
static std::string flex_attempts(const char* tmp) { std::ifstream f((std::string(tmp) + "\\\\InvestorRT\\\\rtx\\\\lsFlexLevels\\\\LiquidityProfile\\\\CL-180-attempts-" + std::to_string(0) + ".csv").c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }

int main() {
    char tmpl[] = "/tmp/lqfXXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    mock::Host h; mock::current = &h; h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL); unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 25; ++i) { mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b); }
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }
    setDepth(h, 80.00, 12, 79.95, 200);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); }
    LQState* S = static_cast<LQState*>(h.data);
    mock::Bar& fb = h.bars.back();
    fb.rows.push_back(mock::Price(79.97f, 0, 40, 40)); setDepth(h, 79.97, 12, 79.95, 120); h.now = t0 + 50; p->calc(0);
    fb.rows.push_back(mock::Price(79.96f, 3, 30, 33)); h.now = t0 + 90; p->calc(0);
    { mock::Bar n; n.time = t0 + 360; n.open = n.high = n.low = n.close = 79.96f; n.rows.push_back(mock::Price(79.96f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = t0 + 181; p->calc(0);
    mock::Bar& nb = h.bars.back();
    setDepth(h, 79.96, 12, 79.95, 300); h.now = t0 + 200; p->calc(0);
    nb.rows[0] = mock::Price(79.96f, 60, 4, 64); h.now = t0 + 260; p->calc(0);
    { mock::Bar n; n.time = t0 + 540; n.open = n.high = n.low = n.close = 80.00f; n.rows.push_back(mock::Price(80.00f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = t0 + 361; p->calc(0);
    h.draws.clear(); p->draw();
    bool head = false, lines = false; int nl = 0; for (auto& d : h.draws) { if (d.type == "text" && d.text.find("AGGRESSION") == 0) head = true; if (d.type == "line") ++nl; }
    lines = nl >= 8;
    CHECK(head, "the pane is labelled AGGRESSION / DEFENSE");
    CHECK(lines, "attack / defend / replenished / pulled lines are drawn");
    const lqp::Attempt* a = !S->e.finished.empty() ? &S->e.finished.back() : (!S->e.attempts.empty() ? &S->e.attempts.back() : nullptr);
    CHECK(a && a->flowBars.size() >= 2, "the touch has per-bar flow");
    CHECK(a && a->barO >= 0, "defenders took over the trading (O)");
    CHECK(a && a->barR >= 0 && a->barX >= 0, "replenished crossed above pulled (R) and both crosses = X");
    bool xs = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "X?") xs = true;
    CHECK(xs, "X? is drawn in the pane");
    std::string attempts = flex_attempts(tmp);
    CHECK(attempts.empty(), "the pane build writes no records (the profile owns them)");
    p->destroy();
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
