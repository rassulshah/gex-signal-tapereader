// test_lmtf_split_host.cpp (0.65.2) split column + test_lmtf_place_host.cpp (0.65.1) LMT F labels sit EXACTLY on their price row (the same PNT price->y the Delta Profile rows use); two prints one tick apart -> the bigger is drawn at its own row, the smaller is not drawn (never pushed down); facts for both at their true prices.
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
    mock::feed.push(0, 1, 7687.75f, 300, (RTDATE)(t0 - 60), 11);   // bid 300 @ 7687.75 (one tick below, same side)
    mock::feed.push(0, 0, 7688.0f, 260, (RTDATE)(t0 - 60), 12);    // offer 260 @ 7688.00 (SAME price as the bid - ES 10-28 7773.00 case)
    mock::feed.push(0, 0, 7692.0f, 250, (RTDATE)(t0 - 60), 13);    // offer 250 @ 7692.00
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 5; ++s) { h.now = t0 - 50 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    mock::feed.push(3, 0, 7688.0f, 350, (RTDATE)(t0 + 30), 10, 901);  mock::feed.push(2, 1, 7688.0f, 0, (RTDATE)(t0 + 30), 10);
    mock::feed.push(3, 0, 7687.75f, 250, (RTDATE)(t0 + 35), 11, 902); mock::feed.push(2, 1, 7687.75f, 0, (RTDATE)(t0 + 35), 11);
    mock::feed.push(3, 1, 7688.0f, 210, (RTDATE)(t0 + 38), 12, 904);  mock::feed.push(2, 0, 7688.0f, 0, (RTDATE)(t0 + 38), 12);
    mock::feed.push(3, 1, 7692.0f, 150, (RTDATE)(t0 + 40), 13, 903);  mock::feed.push(1, 0, 7692.0f, 100, (RTDATE)(t0 + 40), 13);
    for (int s = 0; s < 4; ++s) { h.now = t0 + 60 + s; p->calc(0); usleep(5000); }
    bar(h, t0 + 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    h.draws.clear(); p->draw();
    for (auto& c : S->lfp.cells) std::printf("   cell bar %lld side %d px %.2f lots %lld\n", c.bar, c.side, c.px * 0.25, c.lots);
    const int lastBar = (int)h.bars.size() - 1;
    auto yOf = [&](double px) { cppExtension::PNT q; q.set(lastBar, (float)px); return (int)q.v; };
    const mock::Draw* d350 = text(h, "350"); const mock::Draw* d250 = text(h, "250"); const mock::Draw* d210 = text(h, "210"); const mock::Draw* d150 = text(h, "150");
    for (auto& d : h.draws) if (d.type == "text" && (d.text == "350" || d.text == "250" || d.text == "210" || d.text == "150")) std::printf("   text %s x %d y %d l %d r %d\n", d.text.c_str(), d.x, d.y, d.l, d.r);
    CHECK(d350 != nullptr, "the 350 bid print is drawn");
    CHECK(d210 != nullptr, "the 210 OFFER print at the SAME price 7688.00 is drawn too (split column)");
    if (d350) CHECK(std::abs(d350->y - yOf(7688.0)) <= 1, "350 bid exactly on 7688.00's row");
    if (d210) CHECK(std::abs(d210->y - yOf(7688.0)) <= 1, "210 offer exactly on 7688.00's row");
    if (d350 && d210) CHECK(d350->x < d210->x, "bid in the LEFT half, offer in the RIGHT half");
    if (d350 && d210) CHECK(d350->r <= d210->l || d350->r < d210->x, "the bid half ends before the offer half starts (no overlap)");
    CHECK(d250 == nullptr, "the smaller 250 bid one tick below is NOT drawn (same side, overlapping row) - never pushed down");
    CHECK(d150 != nullptr && std::abs(d150->y - yOf(7692.0)) <= 1, "the 150 offer print is on 7692.00's row");
    if (d150 && d210) CHECK(std::abs(d150->x - d210->x) <= 2, "both offers centred in the same (right) half");
    // nothing ever moved: every drawn LMT F number sits within 1 px of a true print price row
    { const double pxs[] = { 7688.0, 7687.75, 7692.0 }; bool allOn = true; const char* names[] = { "350", "250", "210", "150" };
      for (auto& d : h.draws) { if (d.type != "text") continue; bool isN = false; for (auto n : names) if (d.text == n) isN = true; if (!isN) continue;
          bool on = false; for (double q : pxs) if (std::abs(d.y - yOf(q)) <= 1) on = true; if (!on) allOn = false; }
      CHECK(allOn, "no LMT F label drawn off its price row"); }
    std::vector<sb::Fact> P; sb::readFacts('P', S->symbol, S->spb, S->chartKey, -1, P, false);
    bool f250 = false, f350 = false, f210 = false; for (auto& f : P) { if (f.val == 250 && std::fabs(f.lo - 7687.75) < 1e-6) f250 = true; if (f.val == 350 && std::fabs(f.lo - 7688.0) < 1e-6) f350 = true; if (f.val == 210 && std::fabs(f.lo - 7688.0) < 1e-6 && f.side < 0) f210 = true; }
    CHECK(f350 && f250 && f210, "SignalBoard P facts for all prints at their TRUE prices (the undrawn 250 still posted)");
    std::printf("lsLiquidityProfile %s LMT F split-column host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
