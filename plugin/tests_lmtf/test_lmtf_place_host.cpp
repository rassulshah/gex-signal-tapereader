// test_lmtf_place_host.cpp (0.65.1) LMT F labels sit EXACTLY on their price row (the same PNT price->y the Delta Profile rows use); two prints one tick apart -> the bigger is drawn at its own row, the smaller is not drawn (never pushed down); facts for both at their true prices.
// was: test_lmtf_host.cpp -- lsLiquidityProfile 0.62.0 LMT F on the mock host: the tiers from LRA-LmtF-ES.csv (RTH S20 100 / S10 200 / S5 300),
// a 400-lot bid filled 350 = boxed (S5, a climax fill), a 250-lot offer filled 150 and STILL RESTING = bold (S10) + the white dot,
// a 60-lot fill = not shown (under S20); SignalBoard 1.4.0 facts P (code = tier + 16 resting, state) and B (the resting order); the status line.
// From plugin\:  g++ -std=c++17 -O1 -Itests_lqp044 -I. tests_lmtf/test_lmtf_host.cpp -o tlmtf && ./tlmtf
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static void bar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; b.rows.push_back(mock::Price(c, 50, 40, 90)); h.bars.push_back(b); }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/lmtfXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    { std::ofstream f((flex + "\\LRA-LmtF-ES.csv").c_str()); f << "VERSION,1\nMARKET,ES\nTIER,RTH,100,200,300\nTIER,ON,50,100,150\n"; }
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 22) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26";
    h.priceBase = 7660.0f; h.pxPer = 20.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 12 * 3600 + 30 * 60;   // 12:30 CT = RTH
    for (int i = 0; i < 31; ++i) bar(h, t0 - (30 - i) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(7689.75 - i * 0.25); L.ask = (float)(7690.0 + i * 0.25); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }

    mock::feed.push(0, 1, 7688.0f, 400, (RTDATE)(t0 - 60), 10);    // bid 400 @ 7688.00
    mock::feed.push(0, 1, 7687.75f, 300, (RTDATE)(t0 - 60), 11);   // bid 300 @ 7687.75 (one tick below)
    mock::feed.push(0, 0, 7692.0f, 250, (RTDATE)(t0 - 60), 12);    // offer 250 @ 7692.00
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 5; ++s) { h.now = t0 - 50 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    mock::feed.push(3, 0, 7688.0f, 350, (RTDATE)(t0 + 30), 10, 901);  mock::feed.push(2, 1, 7688.0f, 0, (RTDATE)(t0 + 30), 10);
    mock::feed.push(3, 0, 7687.75f, 250, (RTDATE)(t0 + 35), 11, 902); mock::feed.push(2, 1, 7687.75f, 0, (RTDATE)(t0 + 35), 11);
    mock::feed.push(3, 1, 7692.0f, 150, (RTDATE)(t0 + 40), 12, 903);  mock::feed.push(1, 0, 7692.0f, 100, (RTDATE)(t0 + 40), 12);
    for (int s = 0; s < 4; ++s) { h.now = t0 + 60 + s; p->calc(0); usleep(5000); }
    bar(h, t0 + 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    h.draws.clear(); p->draw();
    for (auto& c : S->lfp.cells) std::printf("   cell bar %lld side %d px %.2f lots %lld\n", c.bar, c.side, c.px * 0.25, c.lots);
    const int lastBar = (int)h.bars.size() - 1;
    auto yOf = [&](double px) { cppExtension::PNT q; q.set(lastBar, (float)px); return (int)q.v; };   // DeltaProfile::yOf - the same mapping its rows use
    const mock::Draw* d350 = text(h, "350"); const mock::Draw* d250 = text(h, "250"); const mock::Draw* d150 = text(h, "150");
    CHECK(d350 != nullptr, "the 350 bid print is drawn");
    if (d350) { std::printf("   350 label centre y %d, yOf(7688.00) %d, DP row %d..%d\n", d350->y, yOf(7688.0), yOf(7688.125), yOf(7687.875));
        CHECK(std::abs(d350->y - yOf(7688.0)) <= 1, "350 label centred on 7688.00's row (+-1 px)");
        CHECK(d350->y >= yOf(7688.125) && d350->y <= yOf(7687.875), "350 label centre inside the Delta Profile's 7688.00 row band"); }
    CHECK(d250 == nullptr, "the smaller 250 print one tick below is NOT drawn (0.65.0 drew it pushed a row lower)");
    CHECK(d150 != nullptr && std::abs(d150->y - yOf(7692.0)) <= 1, "the 150 offer print is drawn at 7692.00's row");
    for (auto& d : h.draws) if (d.type == "text" && (d.text == "350" || d.text == "250" || d.text == "150")) std::printf("   text %s y %d\n", d.text.c_str(), d.y);
    std::vector<sb::Fact> P; sb::readFacts('P', S->symbol, S->spb, S->chartKey, -1, P, false);
    bool f250 = false, f350 = false; for (auto& f : P) { if (f.val == 250 && std::fabs(f.lo - 7687.75) < 1e-6) f250 = true; if (f.val == 350 && std::fabs(f.lo - 7688.0) < 1e-6) f350 = true; }
    CHECK(f350 && f250, "SignalBoard P facts for both bids at their TRUE prices (the undrawn 250 still posted)");
    std::printf("lsLiquidityProfile %s LMT F placement host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
