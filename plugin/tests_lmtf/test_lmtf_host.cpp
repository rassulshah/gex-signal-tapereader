// test_lmtf_host.cpp -- lsLiquidityProfile 0.62.0 LMT F on the mock host: the tiers from LRA-LmtF-ES.csv (RTH S20 100 / S10 200 / S5 300),
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
    mock::feed.push(0, 1, 7688.0f, 400, (RTDATE)(t0 - 60), 10);   // a 400-lot bid
    mock::feed.push(0, 0, 7692.0f, 250, (RTDATE)(t0 - 60), 11);   // a 250-lot offer
    mock::feed.push(0, 1, 7687.0f, 70, (RTDATE)(t0 - 60), 12);    // a 70-lot bid
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 5; ++s) { h.now = t0 - 50 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    // fills: the bid 350 then the rest cancelled (filled-out by the C right after), the offer 150 (still resting 100), the small bid 60
    mock::feed.push(3, 0, 7688.0f, 350, (RTDATE)(t0 + 30), 10, 901);
    mock::feed.push(2, 1, 7688.0f, 0, (RTDATE)(t0 + 30), 10);
    mock::feed.push(3, 1, 7692.0f, 150, (RTDATE)(t0 + 40), 11, 902);
    mock::feed.push(1, 0, 7692.0f, 100, (RTDATE)(t0 + 40), 11);
    mock::feed.push(3, 0, 7687.0f, 60, (RTDATE)(t0 + 50), 12, 903);
    mock::feed.push(2, 1, 7687.0f, 0, (RTDATE)(t0 + 50), 12);
    for (int s = 0; s < 4; ++s) { h.now = t0 + 60 + s; p->calc(0); usleep(5000); }
    // the engine's own line tracking is unchanged and tested in tests_oln; here the LMT F column is fed its lines directly
    {   const long long nowCt = (long long)(t0 + 64) * 1000LL; S->bo.lastT = nowCt; S->bo.lines.clear(); S->bo.live[0] = S->bo.live[1] = -1;
        bol::Line a; a.side = bol::BID; a.px = (bol::Tick)(7688.0 / 0.25); a.end = bol::FILLED; a.tStart = nowCt - 90000; a.tEnd = nowCt - 14000; a.traded = 350; a.size0 = 400; S->bo.lines.push_back(a);
        bol::Line b2; b2.side = bol::ASK; b2.px = (bol::Tick)(7692.0 / 0.25); b2.end = bol::RESTING; b2.tStart = nowCt - 90000; b2.tEnd = 0; b2.traded = 150; b2.left = 100; b2.size0 = 250; S->bo.lines.push_back(b2);
        bol::Line c; c.side = bol::BID; c.px = (bol::Tick)(7687.0 / 0.25); c.end = bol::FILLED; c.tStart = nowCt - 90000; c.tEnd = nowCt - 14000; c.traded = 60; c.size0 = 70; S->bo.lines.push_back(c); }
    bar(h, t0 + 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    h.draws.clear(); p->draw();
    std::printf("  bo lines %zu, tiers ok %d rth %d S20 %lld\n", S->bo.lines.size(), (int)S->lmtFOk, S->lmtFRth, S->lmtF[1][0]);
    for (auto& L : S->bo.lines) std::printf("   line px %lld side %d end %d traded %lld left %lld\n", (long long)L.px, L.side, L.end, L.traded, L.left);
    CHECK(S->lmtFOk && S->lmtFRth == 1 && S->lmtF[1][0] == 100, "the LRA-LmtF file read, RTH tiers in force");
    CHECK(text(h, "350") != nullptr, "the 350-lot fill is shown (>= S20)");
    CHECK(text(h, "150") != nullptr, "the 150-lot fill of the still-resting offer is shown");
    CHECK(text(h, "60") == nullptr, "the 60-lot fill is not shown (under S20 100)");
    bool dot = false; for (auto& d : h.draws) if (d.type == "text" && d.text == "\x95") dot = true;
    CHECK(dot && S->lmtFRest >= 1, "a white dot after the still-resting order's print");
    std::vector<sb::Fact> P, B;

    CHECK(sb::readFacts('P', S->symbol, S->spb, S->chartKey, -1, P, false) && P.size() == 2, "SignalBoard P: two prints");
    bool s5 = false, rest = false; for (auto& f : P) { if ((f.code & 7) == 3 && f.state == 1 && f.side == 1 && f.val == 350) s5 = true; if ((f.code & 7) == 1 && (f.code & 16) && f.state == 0 && f.side == -1) rest = true; }
    CHECK(s5, "P: the 350 bid = tier 3 (S5), decided, bullish side");
    CHECK(rest, "P: the 150 offer = tier 1 (S20 <= 150 < S10 200), still resting (code + 16), live");
    CHECK(sb::readFacts('B', S->symbol, S->spb, S->chartKey, -1, B, false) && !B.empty() && B[0].val == 100 && B[0].side == -1, "SignalBoard B: the resting 100-lot offer");
    usleep(1100000); h.now = t0 + 200; p->calc(0); h.draws.clear(); p->draw(); usleep(1100000); h.now = t0 + 202; p->calc(0);
    const std::string st = readAll(flex + "\\LiquidityProfile.status-ES-180.txt");
    CHECK(st.find("LMT_F,tiers LRA-LmtF file,RTH S20 100 S10 200 S5 300") != std::string::npos, "status LMT_F line");
    std::printf("lsLiquidityProfile %s LMT F host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
