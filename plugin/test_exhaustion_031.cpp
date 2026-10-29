// test_exhaustion_031.cpp -- lsExhaustion 0.3.1 THE T MARK (Rassul approved 2026-10-29 10:23 CT) through the REAL Exhaustion.cpp on the
// tests_exhaustion mock: an order-line HELD (SignalBoard G, as lsLiquidityProfile 0.68.0 posts it) + buyers' fade >= 2.0x within the 5 bars
// before + a close back below the offer within 3 bars -> a red T at the bottom of the pane; recorded; read back after a restart (no
// repaint); no T without the close back / without exhaustion. Plus ex::tMark unit checks.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_exhaustion -I. test_exhaustion_031.cpp -o t && ./t
#include "Exhaustion.cpp"
#include <cstdio>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static mock::Bar bar(unsigned long t, float o, float h, float l, float c, long buy, long sell) { mock::Bar b; b.time = t; b.high = h; b.low = l; b.open = o; b.close = c; b.rows.push_back(mock::Price(h, buy, sell, buy + sell)); return b; }
static long long ctOf(unsigned long t) { time_t v = (time_t)t; struct tm m; gmtime_r(&v, &m); return civilDays(m.tm_year + 1900, (unsigned)(m.tm_mon + 1), (unsigned)m.tm_mday) * 86400LL + m.tm_hour * 3600LL + m.tm_min * 60LL + m.tm_sec; }
int main() {
    {   // the rule (logic)
        ex::Out o; o.fb.assign(12, 1.0); o.fs.assign(12, 1.0); std::vector<double> c(12, 100.0);
        o.fb[6] = 2.4; c[7] = 101; c[8] = 99.5;   // exhaustion at 6, close back below 100 at 8
        ex::Held H; H.bar = 8; H.side = -1; H.px = 100.0; ex::TMark M;
        CHECK(ex::tMark(c, o, H, 11, M) && M.bar == 8 && M.side == -1 && M.exhBar == 6, "offer HELD at 8 + buyers 2.4x at 6 + close below at 8 = red T at 8");
        H.bar = 12; CHECK(!ex::tMark(c, o, H, 11, M), "a HELD bar outside the data: none");
        H.bar = 9; c[8] = 100.5; CHECK(!ex::tMark(c, o, H, 11, M), "no close back below within 3 bars of the exhaustion: none");
        c[8] = 99.5; o.fb[6] = 1.9; H.bar = 8; CHECK(!ex::tMark(c, o, H, 11, M), "exhaustion under 2.0x: none");
        o.fb[6] = 1.0; o.fs[4] = 2.0; c[5] = 101; H.side = 1; H.bar = 7; CHECK(ex::tMark(c, o, H, 11, M) && M.side == 1 && M.bar == 7, "bid HELD + sellers 2.0x + close back above = green T, at the HELD bar (never before it)");
        o.fs[4] = 2.0; H.bar = 10; CHECK(!ex::tMark(c, o, H, 11, M), "exhaustion more than 5 bars before the HELD: none");
    }
    char tmpl[] = "/tmp/ex31XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.chart = "ES 3m #1";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    unsigned long t = day + 8 * 3600 + 30 * 60 + 180;
    for (int i = 0; i < 25; ++i, t += 180) h.bars.push_back(bar(t, 6999.5f, 7000.0f + 0.25f * (i % 2), 6999.0f, 6999.5f, 500, 500));
    const long buyers[8] = {2000, 2000, 1800, 1500, 1200, 900, 700, 500};
    for (int i = 0; i < 8; ++i, t += 180) h.bars.push_back(bar(t, 7000.5f + i, 7001.0f + i, 7000.0f + i, 7000.8f + i, buyers[i], 300));
    // offer at 7008.00: bar 32 traded to 7008 (the HELD decision bar closes red below it), bar 33 closes back below
    const int d = 32; h.bars[d] = bar(h.bars[d].time, 7007.9f, 7008.0f, 7006.0f, 7006.5f, 500, 300);
    h.bars.push_back(bar(t, 7006.5f, 7007.0f, 7005.0f, 7005.5f, 300, 300)); t += 180;   // 33
    h.bars.push_back(bar(t, 7005.5f, 7006.0f, 7005.0f, 7005.5f, 10, 10));               // forming
    h.now = t;
    {   sb::Fact f; f.bar = ctOf(h.bars[d].time); f.lo = f.hi = 7008.0; f.side = -1; f.val = 146; f.code = 2; f.state = 1;
        sb::postFacts('G', "ESZ6", 180, h.chart, steadyMs(), std::vector<sb::Fact>{ f }); }
    Exhaustion* p = static_cast<Exhaustion*>(CreateExtension()); p->calc(0);
    EXState* S = p->testState();
    {   double mx = 0; int at = -1; for (int k = 25; k <= d; ++k) { const double v = S->out.fb[(size_t)(k - S->outFirst)]; if (v > mx) { mx = v; at = k; } }
        std::printf("  max buyers' fade bars 25..%d: %.2fx at %d; T marks %zu\n", d, mx, at, S->tmarks.size()); }
    CHECK(S->tmarks.size() == 1 && S->tmarks.count((long long)h.bars[d].time) && S->tmarks[(long long)h.bars[d].time] == -1, "one red T at the HELD decision bar");
    h.draws.clear(); p->draw();
    bool tm = false; for (auto& x : h.draws) if (x.type == "text" && x.text == "T" && x.color == C_SELL) tm = true;
    CHECK(tm, "T drawn in red at the bottom of the pane");
    p->destroy(); h.data = nullptr;
    Exhaustion* q = static_cast<Exhaustion*>(CreateExtension());
    sb::postFacts('G', "ESZ6", 180, h.chart, steadyMs() - 120000, std::vector<sb::Fact>());   // no producer any more
    q->calc(0); EXState* R = q->testState();
    CHECK(R->tmarks.size() == 1 && R->tmarks.count((long long)h.bars[d].time), "after a restart the same T (read back from the record)");
    CHECK(std::string(EX_VERSION) == "0.3.1" && std::string(ex::VERSION) == "0.3.1", "versions 0.3.1");
    q->destroy();
    std::printf("lsExhaustion %s T mark: %d passed, %d failed\n", EX_VERSION, passes, fails);
    return fails ? 1 : 0;
}
