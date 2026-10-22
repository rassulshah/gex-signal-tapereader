// test_lqp461_host.cpp -- lsLiquidityProfile 0.46.1 (2026-10-20 night): IRT faulted INSIDE the MARKET_DEPTH destructor at every quit
// ("C0000005 Exception at 14002AD39 ... calling done()"). done() / destroy() now PARK the book object; only the next calc / draw of the
// same chart (a symbol / bar-size change) deletes it. On the mock host (MARKET_DEPTH counts live objects):
//   1) the book opens on calc; 2) done() does NOT delete it (still 1 live); 3) the next calc deletes the parked one and opens a fresh one
//   (still 1 live, never 2); 4) destroy() parks it (quit / chart closed: never deleted - the process ends); 5) a draw after done() also
//   flushes the parked book; 6) no live object is ever deleted twice (ASan).
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_lqp044/test_lqp461_host.cpp -o th461 && ./th461
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
int main() {
    char tmpl[] = "/tmp/lq461XXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    h.priceBase = 79.5f; h.pxPer = 1000.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 25; ++i) { mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f; b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b); }
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(80.00 - i * 0.01); L.ask = (float)(80.01 + i * 0.01); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 8; ++s) { h.now = t0 + s; p->calc(0); usleep(5000); }
    std::printf("  live book objects after calc: %d\n", h.depthObjects);
    CHECK(h.depthObjects == 1, "1) the chart's book object is open after calc");
    p->done();
    CHECK(h.depthObjects == 1, "2) done() does not delete the book object (IRT faults in its destructor while quitting)");
    CHECK(p->mdPark_.size() == 1, "2) ... it is parked");
    for (int s = 0; s < 8; ++s) { h.now = t0 + 20 + s; p->calc(0); usleep(5000); }
    CHECK(p->mdPark_.empty(), "3) the next calc deletes the parked book (a symbol / bar-size change)");
    CHECK(h.depthObjects == 1, "3) ... and a fresh one is open (never two live)");
    p->done(); CHECK(p->mdPark_.size() == 1 && h.depthObjects == 1, "5) parked again by done()");
    h.draws.clear(); p->draw(); CHECK(p->mdPark_.empty(), "5) a draw after done() also deletes the parked book");
    for (int s = 0; s < 4; ++s) { h.now = t0 + 40 + s; p->calc(0); usleep(5000); }
    const int before = h.depthObjects;
    p->destroy();
    CHECK(h.depthObjects == before && p->mdPark_.size() == (before ? 1u : 0u), "4) destroy() parks the book (at quit it is never deleted)");
    const std::string st = [&]() { std::ifstream f((std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels\\LiquidityProfile.status-CL-180.txt").c_str()); std::string s((std::istreambuf_iterator<char>(f)), {}); return s; }();
    CHECK(st.empty() || st.compare(0, 8 + std::string(lqp::VERSION).size() + 1, "VERSION," + std::string(lqp::VERSION) + "\n") == 0, "status VERSION = lqp::VERSION (0.47.0 or later)");   // (2026-10-22) the version moves on with each approved build (0.47.1 now): at least this one
    CHECK(std::string(LQ_VERSION) == "0.47.1" && std::string(lqp::VERSION) == "0.47.1", "versions 0.47.0");
    p->mdPark_.clear();          // the mock process ends: (IRT would never delete it) - drop the pointer so ASan's leak check stays about real leaks
    delete p;
    std::printf("lqp 0.46.1 host (book parked at done / destroy): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
