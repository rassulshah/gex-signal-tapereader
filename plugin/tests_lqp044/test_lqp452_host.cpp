// test_liquidityhover_host.cpp -- Liquidity Profile 0.15.0 v2 hover, the WHOLE plugin (LiquidityProfile.cpp) on the mock host with the
// order-by-order feed: a near support zone, its orders, the mouse over it, the box's rows / colours / place (never over the column or
// the Delta Profile), 'as price came' building, the Hit line only while price is in the zone, nothing drawn without the mouse, the
// status MAKEUP line, the makeup record at the decision (the new columns), fonts 10 pt+.
//   g++ -std=c++17 -Imock -I. -I<of140/out> test_liquidityhover_host.cpp -o thh && ./thh
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>
#include <sstream>

namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const std::string DOT = "\xB7";
static const COLOR WHITE = 0x00E2E8F0;

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
// the hover box (grey border, dark fill) and the text rows inside it
struct Box { bool found = false; int l = 0, t = 0, r = 0, b = 0; std::vector<std::pair<std::string, COLOR>> rows; int minFont = 99; };

// test_lqp452_host.cpp -- lsLiquidityProfile 0.45.2 on the mock host: the MKT column must not blink between redraws (Rassul 20:04 2026-10-18).
// Pool files of 4 finished sessions (much bigger orders) + this session's own big market orders on the order feed; then calc / draw
// alternate quickly (the pool is recomputed only every 5 s) and the MKT texts of every redraw must be identical. LMT F colour by resting side.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_lqp044/test_lqp452_host.cpp -o th452 && ./th452
static std::vector<std::string> mktTexts(mock::Host& h) {
    std::vector<std::string> v;
    for (auto& d : h.draws) if (d.type == "text" && d.text.size() > 2 && (d.text.compare(0, 2, "+ ") == 0 || d.text.compare(0, 2, "- ") == 0))
        v.push_back(d.text + "@" + std::to_string(d.y) + "#" + std::to_string(d.color));
    return v;
}
static std::vector<std::string> footer(mock::Host& h) {
    std::vector<std::string> v; for (auto& d : h.draws) if (d.type == "text" && (d.text == "S20" || d.text == "S10" || d.text == "S5")) v.push_back(d.text + "@" + std::to_string(d.y));
    return v;
}
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/lq452XXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    const long long ses0 = (long long)(day / 86400);
    {   // 4 finished sessions: 100 orders each in the 94-108 lot bucket (S20 ~ 100+), for all three channels
        const char* sfx[3] = { "", "-RTH", "-ON" };
        for (int k = 0; k < 3; ++k) { std::ofstream f((flex + "\\LiquidityProfile.mktlots-CL" + sfx[k] + ".txt").c_str());
            for (int s = 1; s <= 4; ++s) { bol::LogDist d; d.addN(100, 100); f << d.line(ses0 - s) << "\n"; } }
    }
    mock::Host h; mock::current = &h;
    h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    h.priceBase = 79.5f; h.pxPer = 1000.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 25; ++i) {
        mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f;
        b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b);
    }
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }
    setDepth(h, 80.00, 80, 79.95, 50);
    // this session's market orders: 4000 one-lot + 60 of 20..79 lots (buyers even, sellers odd), each its own aggressor id, spread over prices
    int id = 1000, ag = 500000;
    for (int i = 0; i < 4000; ++i) mock::feed.push(3, (unsigned char)(i % 2), 80.00f + 0.01f * (float)(i % 5), 1, (RTDATE)(t0 - 900 + i / 10), id++, ag++);
    for (int s = 20; s < 80; ++s) mock::feed.push(3, (unsigned char)(s % 2 ? 0 : 1), 79.90f + 0.01f * (float)(s % 20), (unsigned long)s, (RTDATE)(t0 - 300 + s), id++, ag++);
    for (int s = 0; s < 4; ++s) mock::feed.push(3, (unsigned char)(s % 2), 79.80f + 0.02f * (float)s, (unsigned long)(120 + 10 * s), (RTDATE)(t0 - 120 + s), id++, ag++);   // 4 orders over the pool's S20
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); usleep(10000); }
    LQState* S = static_cast<LQState*>(h.data);
    if (!S) { std::printf("no state\n"); return 1; }
#ifndef LQP_OLD_CODE   // (built once against 0.45.1 with -DLQP_OLD_CODE to prove the test catches the flicker)
    std::printf("  mkt: orders %lld fills %lld big %zu bfNow %lld pool %d mktOk %d cut S20 %.0f\n", S->mkt.nAll, S->mkt.fills, S->mkt.big.size(), S->bfNow, S->mkPool.ok ? 1 : 0, S->mktOk ? 1 : 0, S->mktV[0]);
#endif
    CHECK(S->mkt.nAll >= 4000, "fixture: the market orders were read from the feed");
    double lmu = 0, lv[3] = { 0, 0, 0 }; const bool lok = S->mkt.stats(lmu, lv);
#ifndef LQP_OLD_CODE
    CHECK(lok && S->mkPool.ok && lv[0] < S->mkPool.v[0] && S->mkPool.v[0] < 120, "fixture: the session's own 1SD is under the pool's S20 (0.45.1 alternated these)");
#else
    (void)lok;
#endif
    // alternate calc / draw 45 times over ~7 s (the pool is recomputed every 5 s inside pump(), which calc AND draw run; 0.45.1 used it on that pass only, the session's own 1SD on all others): every redraw must print the same MKT items and footer
    std::vector<std::vector<std::string> > seen, foot; std::vector<double> cuts;
    for (int r = 0; r < 45; ++r) {
        h.now = t0 + 46 + r; if (r == 0) p->calc(0);                 // IRT redraws without a calc too: draw() runs pump() itself
        h.draws.clear(); p->draw();
        seen.push_back(mktTexts(h)); foot.push_back(footer(h)); cuts.push_back(S->mktV[0]);
        usleep(150000);
    }
    bool same = true, fsame = true, csame = true;
    for (size_t i = 1; i < seen.size(); ++i) { if (seen[i] != seen[0]) same = false; if (foot[i] != foot[0]) fsame = false; if (cuts[i] != cuts[0]) csame = false; }
    std::printf("  redraw 1: %zu MKT items, S20 cut %.0f\n", seen[0].size(), cuts[0]);
    CHECK(seen[0].size() == 4, "the 4 orders over the pool's S20 are printed");
    CHECK(csame, "the MKT cut is the same on every calc");
    CHECK(same, "every redraw prints identical MKT items (no blink)");
    CHECK(fsame, "the S5 / S10 / S20 footer is stable");
    // a later recompute (past 5 s) keeps them too
    usleep(5200000); h.now = t0 + 70; p->calc(0); h.draws.clear(); p->draw();
    CHECK(mktTexts(h) == seen[0], "after the pool's 5-s recompute: still the same MKT items");
    const std::string st = [&]() { std::ifstream f((flex + "\\LiquidityProfile.status-CL-180.txt").c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }();
    CHECK(st.find(",tiers 5-session pool") != std::string::npos && st.find(",held ") != std::string::npos, "status: MKT tiers source + held count");
    CHECK(st.compare(0, 15, "VERSION,0.45.2\n") == 0, "status VERSION 0.45.2");
    delete p;
    std::printf("lqp 0.45.2 host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
