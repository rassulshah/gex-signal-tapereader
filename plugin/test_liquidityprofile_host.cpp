// test_liquidityprofile_host.cpp -- the whole plugin (LiquidityProfile.cpp) driven by a mock Investor/RT host (mock/irtsdk.h):
// depth + VAP in, a zone touch, an LX decided on the closed bar, the record written, a restart redrawing the same mark.
//   g++ -std=c++17 -Imock -I. test_liquidityprofile_host.cpp -o th && ./th
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
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }

int main() {
    char tmpl[] = "/tmp/lqpXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mock::Host h; mock::current = &h;
    h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);   // Monday, CT clock as epoch
    unsigned long t0 = day + 9 * 3600;                                              // 9:00 AM
    for (int i = 0; i < 25; ++i) {                                                  // history: 3-min bars ending 7:48 .. 9:00
        mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f;
        b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b);
    }
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }  // forming bar
    setDepth(h, 80.00, 12, 79.95, 200);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    h.now = t0;
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); usleep(260 * 1000 / 26); }   // the zone rests 45 s
    LQState* S = static_cast<LQState*>(h.data);
    CHECK(S != nullptr, "per-chart state lives in the host slot");
    if (S) std::printf("state=%s reads=%lld books=%lld bad=%lld why=%s md=%p avail=%d tick=%f\n", S->state.c_str(), S->depthReads, S->e.books, S->e.badBooks, S->e.why.c_str(), (void*)S->md, (int)S->depthAvail, S->tick);
    CHECK(S && S->e.haveBook && S->e.books > 3, "depth snapshots accepted");
    bool zone = false; for (auto& z : S->e.zones) { std::printf("zone %d c=%lld [%lld,%lld] %.0f\n", z.side, z.center, z.lo, z.hi, z.size); if (z.side == lqp::BID && z.lo <= 7995 && z.hi >= 7995 && z.size >= 200) zone = true; }
    CHECK(zone, "the 200-lot bid at 79.95 is a zone");
    CHECK(S->e.trades == 0, "volume already on the bar at load is history, not new trades");
    // price comes down: sellers hit 79.97 (arrival) then eat 300 at 79.95, then trade 79.92
    mock::Bar& fb = h.bars.back();
    fb.rows.push_back(mock::Price(79.97f, 0, 5, 5)); fb.low = 79.97f; fb.close = 79.97f;
    setDepth(h, 79.97, 12, 79.95, 200);
    h.now = t0 + 46; usleep(300000); p->calc(0);
    CHECK(S->e.attempts.size() == 1, "arrival into the zone starts an attempt");
    fb.rows.push_back(mock::Price(79.95f, 0, 300, 300)); fb.low = 79.95f; fb.close = 79.95f;
    setDepth(h, 79.95, 12, 79.95, 0);
    h.now = t0 + 60; usleep(300000); p->calc(0);
    fb.rows.push_back(mock::Price(79.92f, 0, 20, 20)); fb.low = 79.89f; fb.close = 79.89f;
    setDepth(h, 79.91, 12);
    h.now = t0 + 90; usleep(300000); p->calc(0);
    CHECK(S->e.attempts.size() == 1 && S->e.attempts[0].attackVol >= 300, "attackers' volume counted from the VAP increase");
    // the bar closes (a new bar appears) below the zone
    { mock::Bar n; n.time = t0 + 360; n.open = n.high = n.low = n.close = 79.91f; n.rows.push_back(mock::Price(79.91f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = t0 + 181; usleep(300000); p->calc(0);
    CHECK(S->e.marks.size() == 1 && S->e.marks[0].code == "LX", "eaten 1x+ and the close through = LX on the closed bar");
    CHECK(S->marksWritten == 1, "the mark is written to the session record");
    const std::string rec = flex + "\\LiquidityProfile\\CL-180-signals-" + std::to_string(S->session) + ".csv";
    CHECK(readAll(rec).find("|LX|") != std::string::npos, "record file holds the LX line");
    const std::string att = flex + "\\LiquidityProfile\\CL-180-attempts-" + std::to_string(S->session) + ".csv";
    CHECK(readAll(att).find("LX") != std::string::npos, "the attempt is logged for the nightly scoring");
    p->draw();
    bool drawn = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "LX") drawn = true;
    CHECK(drawn, "LX is drawn");
    bool header = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "LIQ TESTING") header = true;
    CHECK(header, "header says TESTING");
    const std::string st = flex + "\\LiquidityProfile.status-CL-180.txt";
    CHECK(readAll(st).find("VERSION,0.1.2") != std::string::npos, "status file written");
    // restart: a new object and an empty slot redraw exactly the recorded mark
    p->destroy(); CHECK(h.data == nullptr, "destroy frees the slot");
    LiquidityProfile* q = static_cast<LiquidityProfile*>(CreateExtension());
    h.draws.clear(); h.now = t0 + 200; q->calc(0); q->draw();
    LQState* R = static_cast<LQState*>(h.data);
    CHECK(R && R->e.marks.size() == 1 && R->e.marks[0].code == "LX", "after a restart the record is re-read");
    drawn = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "LX") drawn = true;
    CHECK(drawn, "and the same LX is drawn again (no repaint)");
    CHECK(R->e.trades == 0, "a restart does not replay old volume as trades");
    // no depth from the feed: says so, draws no book, no crash
    h.depthOn = false; h.draws.clear(); LiquidityProfile* z = static_cast<LiquidityProfile*>(CreateExtension()); z->destroy();
    q->destroy(); h.data = nullptr;
    LiquidityProfile* w = static_cast<LiquidityProfile*>(CreateExtension());
    h.now = t0 + 300; w->calc(0); w->draw();
    bool noDepth = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "LIQ no depth") noDepth = true;
    CHECK(noDepth, "no depth from this feed is shown as LIQ no depth");
    w->destroy();
    CHECK(h.depthObjects == 0, "every depth object is released (unsubscribed)");
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
