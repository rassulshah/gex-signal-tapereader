// test_liqtouch_lp_host.cpp -- lsLiquidityProfile 0.66.0 THE TOUCH TAG on the mock host (tests_lqp044/irtsdk.h, the real LiquidityProfile.cpp):
// a 150-lot bid at 7688.00 (ES tier 72) rests 40 s two ticks+ under price; the first trade at it (30 lots), a 40-lot refill, price back up
// within 2 s -> 15 s later STAYS+ (record T line, SignalBoard S fact code 1, vote +2 posted ONCE); price 3 points up -> HELD (O line, fact 11);
// a 50-lot bid (under the tier) touched -> never followed, no tag; a RESTART reads the record back and posts the SAME facts (no repaint);
// the per-market params file (tier) and a garbled one (defaults + the reason in the status line LIQ_TOUCH).
// From plugin\:  g++ -std=c++17 -O1 -Itests_lqp044 -I. tests_liqtouch/test_liqtouch_lp_host.cpp -o tlth && ./tlth
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#define private public
#include "LiquidityProfile.cpp"
#undef private
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void bar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; b.rows.push_back(mock::Price(c, 50, 40)); h.bars.push_back(b); }
// on Linux the plugin's "\\"-joined paths are flat names in /tmp ("lthXXXX\\InvestorRT\\...\\file"): find by the temp name + a part
static std::string findFile(const std::string& tmpDir, const std::string& part) {
    const std::string base = tmpDir.substr(tmpDir.rfind('/') + 1); std::string cmd = "ls /tmp 2>/dev/null | grep -a '^" + base + "'"; FILE* p = popen(cmd.c_str(), "r"); char b[512]; std::string r;
    while (p && fgets(b, sizeof(b), p)) { std::string n(b); if (!n.empty() && n.back() == '\n') n.pop_back(); if (n.find(part) != std::string::npos && n.size() > part.size() && n.compare(n.size() - 4, 4, ".bak") != 0) r = "/tmp/" + n; }
    if (p) pclose(p); return r; }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/lthXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 22) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26";
    h.priceBase = 7660.0f; h.pxPer = 20.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 12 * 3600 + 30 * 60;   // 12:30 CT = RTH
    for (int i = 0; i < 31; ++i) bar(h, t0 - (30 - i) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(7689.75 - i * 0.25); L.ask = (float)(7690.0 + i * 0.25); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    auto run = [&](unsigned long from, unsigned long to) { for (unsigned long s = from; s <= to; ++s) { h.now = s; p->calc(0); usleep(2000); } h.draws.clear(); p->draw(); };
    for (int s = 0; s < 3; ++s) { h.now = t0 - 70 + s; p->calc(0); usleep(2000); }   // the feed opens, the session starts
    bar(h, t0 + 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);   // a new candle: the chart ATR (6 pts = 24 ticks) - the host ATR the engine uses before its own 10 blocks
    int id = 100;
    mock::feed.push(0, 1, 7688.0f, 150, (RTDATE)(t0 - 60), 10);   // the big bid 150 @ 7688.00
    mock::feed.push(0, 1, 7686.0f, 50, (RTDATE)(t0 - 60), 11);    // a 50-lot bid (under the 72 tier) @ 7686.00
    for (int i = 0; i < 5; ++i) mock::feed.push(0, 1, 7685.0f - 1.0f * i, 30, (RTDATE)(t0 - 60), 300 + i);   // (LiqTouch 1.1.0) five 30-lot bids 4 ticks apart: bid median 30 -> the 2.0x gate = 60 lots
    mock::feed.push(0, 0, 7690.0f, 400, (RTDATE)(t0 - 60), 12);   // offers at 7690 to trade against
    for (int i = 1; i <= 12; ++i) mock::feed.push(0, 0, 7690.0f + 0.25f * i, 5, (RTDATE)(t0 - 60), 200 + i);   // a book of 12 small offers (the per-minute book sample needs 10+ levels)
    for (unsigned long s = t0 - 60; s < t0 - 18; s += 3) mock::feed.push(3, 0, 7690.0f, 1, (RTDATE)s, 12, ++id);   // prints at 7690 (last = 7690, 8 ticks above the bid)
    run(t0 - 67, t0 - 18);
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    {   bool c150 = false, c50 = false; for (const ltc::Zone& z : S->lt.candidates()) { if (z.side == ltc::BID && z.px == 30752) c150 = z.lots == 150; if (z.px == 30744) c50 = true; }
        CHECK(c150 && !c50 && std::fabs(S->lt.median(ltc::BID) - 30.0) < 1e-9, "(1.1.0) zone candidates: the 150 bid in, the 50 bid out (median 30, gate 60)");
        std::vector<sb::Fact> B; sb::readFacts('B', S->symbol, S->spb, S->chartKey, -1, B, false); bool b2 = false; for (auto& f : B) if (f.code == 2 && std::fabs(f.lo - 7688.0) < 1e-6 && f.val == 150 && f.side == 1) b2 = true;
        bool onlyZones = !B.empty(); for (auto& f : B) if (f.code != 2) onlyZones = false;
        CHECK(b2 && onlyZones, "(0.67.0) B facts = the zone candidates only (code 2, 150 @ 7688.00 bid)"); }
    CHECK(S->lt.followed() >= 1, "the 150-lot bid is followed (zone 150 >= 2.0x the bid median 30, real after 30 s)");
    // the touch: 30 lots of the bid traded, a 40-lot refill, price back to 7690 within 2 s
    const unsigned long tt = t0 - 17;
    mock::feed.push(3, 1, 7688.0f, 30, (RTDATE)tt, 10, ++id); mock::feed.push(1, 1, 7688.0f, 120, (RTDATE)tt, 10);
    mock::feed.push(0, 1, 7688.0f, 40, (RTDATE)(tt + 1), 20);
    for (unsigned long s = tt + 2; s < tt + 20; s += 2) mock::feed.push(3, 0, 7690.0f, 1, (RTDATE)s, 12, ++id);
    run(t0 - 17, t0 + 4);
    std::vector<sb::Fact> F; sb::readFacts('S', S->symbol, S->spb, S->chartKey, -1, F, false);
    const sb::Fact* tag = nullptr; for (auto& f : F) if (f.code >= 1 && f.code <= 7 && std::fabs(f.lo - 7688.0) < 1e-6) tag = &f;
    CHECK(tag && tag->code == ltc::T_STAYS_PLUS && tag->side == 1 && tag->val == 2 && tag->state == 1 && tag->proven == 1, "15 s after the touch: STAYS+ (bid, vote +2, decided) on SignalBoard S");
    int n1 = 0; for (auto& f : F) if (f.code >= 1 && f.code <= 7 && std::fabs(f.lo - 7688.0) < 1e-6) ++n1;
    CHECK(n1 == 1, "the vote is posted once per touch");
    const std::string recPath = findFile(tmp, "-liqtouch-");
    std::string rec = readAll(recPath);
    CHECK(rec.find("kind,touch_ms,sec_end,side,px_tick") == 0 && rec.find("\nT,") != std::string::npos && rec.find(",BID,30752,7688,STAYS+,2,2,0,150,") != std::string::npos, "record: header + the T line (bid 7688.00, STAYS+, vote +2, def 2, att 0, 150 lots at the touch)");
    CHECK(rec.find(",7686,") == std::string::npos, "the 50-lot bid (zone 50 < 2.0x median = 60) is never followed / recorded");
    // the outcome: price 3 points up (0.5 ATR away first) -> HELD
    mock::feed.push(0, 0, 7692.0f, 100, (RTDATE)(t0 + 5), 30);
    for (unsigned long s = t0 + 6; s < t0 + 14; ++s) mock::feed.push(3, 0, 7692.0f, 1, (RTDATE)s, 30, ++id);
    run(t0 + 5, t0 + 18);
    F.clear(); sb::readFacts('S', S->symbol, S->spb, S->chartKey, -1, F, false);
    const sb::Fact* out = nullptr; for (auto& f : F) if (f.code >= 11 && std::fabs(f.lo - 7688.0) < 1e-6) out = &f;
    CHECK(out && out->code == 10 + ltc::O_HELD, "the bid HELD (fact code 11)");
    rec = readAll(recPath);
    CHECK(rec.find("\nO,") != std::string::npos && rec.find(",HELD,") != std::string::npos, "record: the O line HELD");
    CHECK(rec.find("\nD,") != std::string::npos, "record: the 30-s log line D");
    CHECK(rec.find("\nB,") != std::string::npos, "record: the per-minute book sample B (p97 of level sizes within 3 ATR) for the nightly tier refit");
    {   const std::string s2 = readAll(findFile(tmp, "LiquidityProfile.status-ES-180.txt")); const size_t a = s2.find("LIQ_TOUCH");
        CHECK(a != std::string::npos && s2.find("tier RTH 72") != std::string::npos && s2.find("no LRA-LiqTouch-ES.csv - defaults") != std::string::npos, "status LIQ_TOUCH: ES tier 72, defaults (no params file)");
        if (a != std::string::npos) std::printf("  %s\n", s2.substr(a, s2.find('\n', a) - a).c_str()); }
    std::vector<sb::Fact> before; for (auto& f : F) if (f.code != 0) before.push_back(f);
    // RESTART: the record is read back -> the same facts (tag + outcome), nothing re-decided
    p->destroy(); p->slot_.release(p); p->flushPark(); delete p; h.data = nullptr; mock::feed.ev.clear(); mock::feed.at = 0;
    p = static_cast<LiquidityProfile*>(CreateExtension());
    run(t0 + 19, t0 + 23); S = static_cast<LQState*>(h.data);
    std::vector<sb::Fact> F2; sb::readFacts('S', S->symbol, S->spb, S->chartKey, -1, F2, false);
    bool same = !before.empty(); for (auto& b : before) { bool hit = false; for (auto& f : F2) if (f.bar == b.bar && f.lo == b.lo && f.side == b.side && f.code == b.code && f.val == b.val) hit = true; if (!hit) same = false; }
    CHECK(same, "after a restart the same S facts (STAYS+ and HELD) are posted from the record (no repaint)");
    // the params file: a nightly tier, then a garbled one
    { std::ofstream f((flex + "\\LRA-LiqTouch-ES.csv").c_str()); f << "VERSION,1\nMARKET,ES\nTIER,RTH,500\nTIER,ON,400\nFARWAIT,RTH,60\nFARWAIT,ON,60\nDWELL_PLUS,4\nDWELL_WEAK,14\nCHURN,15\n"; }
    S->ltParamsMs = -100000; run(t0 + 24, t0 + 25);
    CHECK(S->lt.P.tier[1] == 500 && S->lt.P.tier[0] == 400 && S->lt.P.fromFile && S->ltWhy == "nightly params", "the per-market params file is read (tier RTH 500 / ON 400)");
    { std::ofstream f((flex + "\\LRA-LiqTouch-ES.csv").c_str()); f << "VERSION,1\nMARKET,NQ\nTIER,RTH,5\nTIER,ON,5\n"; }
    S->ltParamsMs = -100000; run(t0 + 26, t0 + 27);
    CHECK(S->lt.P.tier[1] == 72 && !S->lt.P.fromFile && S->ltWhy.find("wrong market") != std::string::npos, "a wrong-market / garbled params file = the defaults + the reason");
    p->destroy(); p->slot_.release(p); p->flushPark(); delete p;
    std::printf("lsLiquidityProfile %s touch-tag host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
