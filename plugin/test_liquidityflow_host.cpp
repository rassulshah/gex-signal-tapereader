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
    {   // (0.4.0) the study confirmed LoF for this market (drawn without "?"); RpB is still testing
        std::ofstream nf((std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-LiqNorms-CL.csv").c_str());
        nf << "MARKET,CL\r\nZONE,0.25,4,8\r\nSIGNAL,LoF,confirmed\r\nSIGNAL,RpB,testing\r\nSIGNAL,X,confirmed\r\n";
    }
    mock::Host h; mock::current = &h; h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL); unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 25; ++i) { mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 20, 20, 40)); h.bars.push_back(b); }   // normal pace: 20 a side per bar
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }
    setDepth(h, 80.00, 12, 79.95, 200);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); }
    LQState* S = static_cast<LQState*>(h.data);
    // (0.4.1) six quiet live bars first, so the book has a normal pace (the volume pace is seeded from the history bars)
    unsigned long T = t0;
    for (int k = 0; k < 6; ++k) {
        h.bars.back().rows.push_back(mock::Price(80.03f, 5, 5, 10));
        for (int s = 0; s < 180; s += 20) { setDepth(h, 80.00, 12, 79.95, 200 + ((s / 20 + k) % 2) * 6); h.now = T + 45 + s; p->calc(0); }
        mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 80.03f; n.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(n);
        T += 180; h.now = T + 41; p->calc(0);
    }
    setDepth(h, 80.00, 12, 79.95, 200); h.now = T + 44; p->calc(0);
    mock::Bar& fb = h.bars.back();
    fb.rows.push_back(mock::Price(79.97f, 0, 40, 40)); setDepth(h, 79.97, 12, 79.95, 120); h.now = T + 50; p->calc(0);
    fb.rows.push_back(mock::Price(79.96f, 3, 30, 33)); h.now = T + 90; p->calc(0);
    { mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 79.96f; n.rows.push_back(mock::Price(79.96f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = T + 181; p->calc(0);
    mock::Bar& nb = h.bars.back();
    setDepth(h, 79.96, 12, 79.95, 300); h.now = T + 200; p->calc(0);
    nb.rows[0] = mock::Price(79.96f, 90, 4, 94); h.now = T + 260; p->calc(0);
    { mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 80.00f; n.rows.push_back(mock::Price(80.00f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = T + 361; p->calc(0);
    h.draws.clear(); p->draw();
    bool head = false, lines = false; int nl = 0; for (auto& d : h.draws) { if (d.type == "text" && d.text.find("AGGRESSION") == 0) head = true; if (d.type == "line") ++nl; }
    lines = nl >= 8;
    CHECK(head, "the pane is labelled AGGRESSION / DEFENSE");
    CHECK(lines, "buyers / sellers aggression and both net resting lines are drawn");
    const lqp::Attempt* a = !S->e.finished.empty() ? &S->e.finished.back() : (!S->e.attempts.empty() ? &S->e.attempts.back() : nullptr);
    CHECK(a && a->flowBars.size() >= 2, "the touch has per-bar flow");
    CHECK(a && a->barO >= 0 && a->barR >= 0, "buyers took over the trading and the bids' net crossed above the offers'");
    bool lof = false, rpb = false, old = false;
    for (auto& d : h.draws) if (d.type == "text") { if (d.text.find("LoF ") == 0 && d.text.find("x") != std::string::npos) lof = true; if (d.text.find("RpB? ") == 0) rpb = true; if (d.text == "X?" || d.text == "E?" || d.text == "O?" || d.text == "R?") old = true; }
    CHECK(lof, "LoF drawn without '?' (confirmed for this market)");
    CHECK(rpb, "RpB? drawn with '?' (still testing)");
    CHECK(!old, "the retired letters (E? O? R? X?) are gone");
    const std::string rec = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LiquidityProfile\\CL-180-panes-" + std::to_string(S->session) + ".csv";
    CHECK(readAll(rec).find("|LoF|") != std::string::npos && readAll(rec).find("|RpB|") != std::string::npos, "the pane signals are recorded for the session");
    std::string attempts = flex_attempts(tmp);
    CHECK(attempts.empty(), "the pane build writes no attempt log (the profile owns it)");
    p->destroy();
    LiquidityProfile* q = static_cast<LiquidityProfile*>(CreateExtension());
    h.draws.clear(); h.now = T + 400; q->calc(0); q->draw();
    bool again = false; for (auto& d : h.draws) if (d.type == "text" && d.text.find("LoF ") == 0) again = true;
    CHECK(again, "after a restart the same signals are drawn again (no repaint)");
    p = q;
    p->destroy();
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
