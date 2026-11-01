// test_exhaustion_040.cpp -- lsExhaustion 0.4.0 THE NET DISPLAY + THE T MARK (on the net ratio) (Rassul approved 2026-10-29 10:23 CT) through the REAL Exhaustion.cpp on the
// tests_exhaustion mock: an order-line HELD (SignalBoard G, as lsLiquidityProfile 0.68.0 posts it) + buyers' fade >= 2.0x within the 5 bars
// before + a close back below the offer within 3 bars -> a red T at the bottom of the pane; recorded; read back after a restart (no
// repaint); no T without the close back / without exhaustion. Plus ex::tMark unit checks.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. test_exhaustion_040.cpp -o t && ./t
#include "Exhaustion.cpp"
#include <cstdio>
#include <unistd.h>
#include <sstream>
#include <dirent.h>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static mock::Bar bar(unsigned long t, float o, float h, float l, float c, long buy, long sell) { mock::Bar b; b.time = t; b.high = h; b.low = l; b.open = o; b.close = c; b.rows.push_back(mock::Price(h, buy, sell, buy + sell)); return b; }
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
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
    {   // (0.4.0) the NET logic
        // wrap: no line over 60 characters, every word kept
        const std::vector<std::string> W = ex::wrap(ex::INFO_TEXT, 60); size_t mxl = 0, chars = 0; for (auto& x : W) { mxl = std::max(mxl, x.size()); chars += x.size(); }
        CHECK(mxl <= 60 && W.size() >= 2 && W[0].rfind("What it answers:", 0) == 0, "info text wraps to lines of <= 60 characters, several lines");
        { std::vector<std::string> w2 = ex::wrap(std::string(130, 'x'), 60); CHECK(w2.size() == 3 && w2[0].size() == 60, "a 130-character word is cut into 60 / 60 / 10"); }
        // a push up (net buying shrinking) then a push down (net selling shrinking) - the sellers side mirrors on NEGATIVE bars
        std::vector<double> h, l, b, s;
        for (int i = 0; i < 25; ++i) { h.push_back(100 + 0.25 * (i % 2)); l.push_back(99); b.push_back(300); s.push_back(300); }
        const double up[6] = { 900, 900, 700, 400, 150, 60 };   // net +600 .. -240
        for (int i = 0; i < 6; ++i) { h.push_back(101 + i); l.push_back(100 + i); b.push_back(up[i]); s.push_back(300); }
        for (int i = 0; i < 6; ++i) { h.push_back(105 - i); l.push_back(104 - i - 3); b.push_back(300); s.push_back(up[i]); }   // mirror: net -600 .. +240
        ex::NetOut o = ex::computeNet(h, l, b, s);
        int labB = 0, labS = 0, lastB = -1, lastS = -1; for (size_t k = 0; k < h.size(); ++k) { if (o.labB[k]) { ++labB; lastB = (int)k; } if (o.labS[k]) { ++labS; lastS = (int)k; } }
        std::printf("  net: fb up-run %.2f %.2f %.2f; fs down-run %.2f %.2f %.2f; labels B %d @%d S %d @%d; med %.0f\n", o.fb[28], o.fb[29], o.fb[30], o.fs[34], o.fs[35], o.fs[36], labB, lastB, labS, lastS, o.med[30]);
        CHECK(labB == 1 && lastB == 30, "ONE buyers label for the run, at its newest bar");
        CHECK(labS == 1 && std::fabs(o.fs[lastS] - o.fb[30]) < 1e-9 && o.net[lastS] == -o.net[30], "sellers side symmetric on the negative bars (same ratio, mirrored net)");
        CHECK(o.fb[30] <= ex::NET_CAP && o.fb[30] >= 2.0, "ratio capped at 10x, >= 2.0x labelled");
        CHECK(o.runEndB[30] == 1, "the buyers run is final once a later bar does not continue it");
        // noise cap: the same push scaled down to tiny lots under the median |net| -> logged, not labelled
        std::vector<double> b2 = b, s2 = s; for (int i = 0; i < 25; ++i) { b2[i] = i % 2 ? 2000 : 300; s2[i] = i % 2 ? 300 : 2000; }   // chop with |net| 1700 both ways: median |net| 1700, no push
        ex::NetOut q = ex::computeNet(h, l, b2, s2); int qb = 0, qn = 0; for (size_t k = 25; k < 31; ++k) { qb += q.labB[k]; qn += q.noisyB[k]; }
        CHECK(qb == 0 && qn == 1, "noise cap: a push peak under the median |net| of the last 100 bars is logged, not labelled");
        // no repaint for a final label: every prefix that contains the run's end gives the same label
        bool same = true; for (size_t n = 32; n <= h.size(); ++n) { std::vector<double> a(h.begin(), h.begin() + n), c(l.begin(), l.begin() + n), d(b.begin(), b.begin() + n), e(s.begin(), s.begin() + n);
            ex::NetOut p = ex::computeNet(a, c, d, e); for (int k = 0; k <= 30; ++k) same = same && p.labB[k] == o.labB[k] && p.fb[k] == o.fb[k]; }
        CHECK(same, "no repaint: a final label and its ratio never change as bars arrive");
    }
    char tmpl[] = "/tmp/ex40XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
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
    {   // (0.4.0) the display: header, histogram, the (i) box (separate lines <= 60 chars, inside the pane), the bar hover, the net record
        h.draws.clear(); p->draw();
        bool head = false, tip = false; int bars = 0; for (auto& x : h.draws) { if (x.type == "text" && x.text == "EXHAUSTION (net)   ") head = true; if (x.type == "box") ++bars; }
        CHECK(head && bars >= 30, "header 'EXHAUSTION (net)' and one histogram bar per visible closed candle");
        RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((S->iconL + S->iconR) / 2); e.v.mouse.v = (short)((S->iconT + S->iconB) / 2); e.v.mouse.bar = -1;
        CHECK(S->iconR > S->iconL, "the (i) icon is drawn"); p->mouse(&e); h.draws.clear(); p->draw();
        int lines = 0; size_t longest = 0; for (auto& x : h.draws) if (x.type == "text" && x.text.size() > 25) { ++lines; longest = std::max(longest, x.text.size()); if (x.text.rfind("What it answers", 0) == 0) tip = true; }
        std::printf("  info box: %d long text draws, longest %zu chars\n", lines, longest);
        CHECK(tip && lines >= 8 && longest <= 60, "info box: many separate draw calls, none over 60 characters");
        bool inPane = true; for (auto& x : h.draws) if (x.type == "box" && x.r - x.x > 100) inPane = inPane && x.x >= 0 && x.r <= 2000 && x.y >= 0 && x.b <= 1000;
        CHECK(inPane, "the box stays inside the pane");
        e.v.mouse.h = 300; e.v.mouse.v = 500; e.v.mouse.bar = 30; p->mouse(&e); h.draws.clear(); p->draw();
        bool hb = false, hn = false; for (auto& x : h.draws) if (x.type == "text") { if (x.text.rfind("Bought at the ask:", 0) == 0) hb = true; if (x.text.rfind("Net aggression:", 0) == 0) hn = true; }
        CHECK(hb && hn, "bar hover: buy lots, sell lots, net on separate lines");
        std::string net; { const std::string base = std::string(tmp).substr(5); DIR* dd = opendir("/tmp"); if (dd) { while (dirent* de = readdir(dd)) { const std::string n = de->d_name; if (n.rfind(base, 0) == 0 && n.find("-net-") != std::string::npos) net += readAll("/tmp/" + n); } closedir(dd); } }
        std::printf("  net record: %s\n", net.substr(0, 120).c_str());
        CHECK(!net.empty() && net.find("|top|") != std::string::npos, "final net labels recorded with the gross ratio beside them");
    }
    p->destroy(); h.data = nullptr;
    Exhaustion* q = static_cast<Exhaustion*>(CreateExtension());
    sb::postFacts('G', "ESZ6", 180, h.chart, steadyMs() - 120000, std::vector<sb::Fact>());   // no producer any more
    q->calc(0); EXState* R = q->testState();
    CHECK(R->tmarks.size() == 1 && R->tmarks.count((long long)h.bars[d].time), "after a restart the same T (read back from the record)");
    CHECK(std::string(EX_VERSION) == "0.5.0" && std::string(ex::VERSION) == "0.5.0" && readAll("Exhaustion.cpp").find("p->setVersion(\"0.5.0\")") != std::string::npos, "versions 0.5.0 agree");
    q->destroy();
    std::printf("lsExhaustion %s NET + T: %d passed, %d failed\n", EX_VERSION, passes, fails);
    return fails ? 1 : 0;
}
