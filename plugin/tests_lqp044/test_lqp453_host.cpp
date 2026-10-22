// test_lqp453_host.cpp -- lsLiquidityProfile 0.45.3 (Rassul 23:37 CT 2026-10-18 "get rid of the hover over on the gex levels and other
// places. make the colors in the mkt and lmt f lighter, i can barely see them"): on the mock host with the 0.45.2 fixture,
//   1) mouse events over every text drawn and a price grid across the pane produce NO hover: no invalidate, the next draw is
//      byte-identical to the one before, status "hover off". NOTE: this fixture draws no near zone / big-order line, so with LQ_HOVERS
//      true only the LQ_HOVERS + status checks fail (proved 2026-10-18) - the switch itself is the guard; check_hovers_off.py holds the
//      source contract (mouse() returns first, draw gated, no TRACK_MOUSE);
//   2) every MKT / LMT F text is drawn in one of the four lighter inks (lqp::colInk), never the old dim 0x157A3D / 0x933030 or 0x22C55E /
//      0xEF4444, and every ink is >= 4.5:1 (WCAG) on the column background; the bigger tier is the brighter ink; green stays green, red red.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_lqp044/test_lqp453_host.cpp -o th453 && ./th453
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

// (fixture from test_lqp452_host.cpp) lsLiquidityProfile 0.45.2 on the mock host: the MKT column must not blink between redraws (Rassul 20:04 2026-10-18).
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
    for (int r = 0; r < 3; ++r) { h.now = t0 + 46 + r; p->calc(0); h.draws.clear(); p->draw(); }
    // ---- 2) colours
    const unsigned inks[4] = { lqp::COL_INK_G, lqp::COL_INK_R, lqp::COL_INK_G_DIM, lqp::COL_INK_R_DIM };
    for (int k = 0; k < 4; ++k) CHECK(lqp::contrast(inks[k], lqp::COL_BG) >= 4.5, "every column ink >= 4.5:1 on the column background");
    CHECK(lqp::relLum(lqp::COL_INK_G) > lqp::relLum(lqp::COL_INK_G_DIM) && lqp::relLum(lqp::COL_INK_R) > lqp::relLum(lqp::COL_INK_R_DIM), "the bigger tier is the brighter ink");
    auto ch = [](unsigned c, int s) { return (int)((c >> s) & 255u); };
    CHECK(ch(lqp::COL_INK_G, 8) > ch(lqp::COL_INK_G, 16) && ch(lqp::COL_INK_G_DIM, 8) > ch(lqp::COL_INK_G_DIM, 16), "green inks stay green (G > R)");
    CHECK(ch(lqp::COL_INK_R, 16) > ch(lqp::COL_INK_R, 8) && ch(lqp::COL_INK_R_DIM, 16) > ch(lqp::COL_INK_R_DIM, 8), "red inks stay red (R > G)");
    CHECK(lqp::colInk(true, false) == lqp::COL_INK_G && lqp::colInk(false, true) == lqp::COL_INK_R_DIM, "colInk maps side / tier");
    CHECK(lqp::contrast(0x0022C55Eu, lqp::COL_BG) < lqp::contrast(lqp::COL_INK_G, lqp::COL_BG) && lqp::contrast(0x00933030u, lqp::COL_BG) < 4.5, "the old inks were dimmer (the dim red failed 4.5:1)");
    int mkt = 0, mktOk = 0;
    for (auto& d : h.draws) if (d.type == "text" && d.text.size() > 2 && (d.text.compare(0, 2, "+ ") == 0 || d.text.compare(0, 2, "- ") == 0)) {
        ++mkt; const unsigned c = (unsigned)d.color; bool ok = false; for (int k = 0; k < 4; ++k) if (c == inks[k]) ok = true;
        const bool buyer = d.text[0] == '+'; if (ok && (buyer ? (c == lqp::COL_INK_G || c == lqp::COL_INK_G_DIM) : (c == lqp::COL_INK_R || c == lqp::COL_INK_R_DIM))) ++mktOk;
    }
    std::printf("  MKT texts %d, in the new inks with the right side %d\n", mkt, mktOk);
    CHECK(mkt >= 4 && mktOk == mkt, "every MKT text: buyers ('+') a green ink, sellers ('-') a red ink, all from colInk");
    int old = 0; for (auto& d : h.draws) if (d.type == "text") { const unsigned c = (unsigned)d.color; if (c == 0x00157A3Du || c == 0x00933030u) ++old; }
    CHECK(old == 0, "no column text in the old dim inks 0x157A3D / 0x933030");
    // ---- 1) no hover anywhere
    auto sig = [&]() { std::string o; for (auto& d : h.draws) o += d.type + "|" + d.text + "|" + std::to_string(d.x) + "," + std::to_string(d.y) + "," + std::to_string(d.r) + "," + std::to_string(d.b) + "#" + std::to_string(d.color) + "/" + std::to_string(d.border) + "\n"; return o; };
    h.now = t0 + 50; p->calc(0); h.draws.clear(); p->draw(); const std::string before = sig();
    std::vector<std::pair<int, float> > spots;                      // (bar, price) over every text drawn and a price grid across the pane
    const int nb = (int)h.bars.size();
    for (auto& d : h.draws) if (d.type == "text") for (int b = 0; b < nb; b += 4) spots.push_back(std::make_pair(b, (float)(h.priceBase + (h.paneBottom - d.y) / h.pxPer)));
    for (int b = 0; b < nb; ++b) for (int k = 0; k <= 60; ++k) spots.push_back(std::make_pair(b, 79.70f + 0.01f * (float)k));
    const int inv0 = h.invalidates; int rc = 0;
    for (size_t i = 0; i < spots.size(); ++i) for (int ty = 0; ty < 2; ++ty) {
        RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = ty ? E_MOUSE_HOVER : E_MOUSE_MOVE; e.v.mouse.bar = spots[i].first; e.v.mouse.price = spots[i].second;
        e.v.mouse.h = (short)(h.barX0 + spots[i].first * h.barW); e.v.mouse.v = (short)(h.paneBottom - (spots[i].second - h.priceBase) * h.pxPer);
        if (p->mouse(&e) != RTX_FAIL) ++rc;
    }
    std::printf("  mouse events %zu\n", spots.size() * 2);
    CHECK(rc == 0, "mouse() never swallows an event");
    CHECK(h.invalidates == inv0, "no mouse event asks for a redraw (nothing to show)");
    CHECK(S->hovId == 0 && S->boHovKey == 0, "no hovered zone / order line is remembered");
    h.draws.clear(); p->draw();
    CHECK(sig() == before, "the draw after the mouse is identical to the draw before it (no hover box)");
    CHECK(!LQ_HOVERS, "LQ_HOVERS is off");
    { const std::string st = readAll(flex + "\\LiquidityProfile.status-CL-180.txt");
      CHECK(st.compare(0, 8 + std::string(lqp::VERSION).size() + 1, "VERSION," + std::string(lqp::VERSION) + "\n") == 0, "status VERSION = lqp::VERSION");   // (2026-10-22) the version moves on with each approved build (0.47.1 now): at least this one
      CHECK(st.find(",hover off") != std::string::npos, "status: the hover is reported off"); }
    delete p;
    std::printf("lqp 0.45.3 host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
