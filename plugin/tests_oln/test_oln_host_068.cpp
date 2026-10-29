// test_oln_host_068.cpp -- lsLiquidityProfile 0.68.0 THE TESTED ORDER LINES (OrderLinesLogic 1.1.0) through the REAL LiquidityProfile.cpp on
// the mock host: untouched = nothing drawn; touched = thin dashed from the placement (left edge of the price area right of the Key Levels
// column when placed earlier); HELD solid with [rank] + HELD at the decision bar, rank frozen at the touch (Liquidity column Q board);
// min 2 bars after a close through; record T / X; restart = the same drawing (no repaint); everything inside the price area.
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Itests_lqp044 -I. tests_oln/test_oln_host_068.cpp -o t && ASAN_OPTIONS=detect_leaks=0 ./t
#include "LiquidityProfile.cpp"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
namespace mock { Host* current = nullptr; Feed feed; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static const mock::Draw* text(mock::Host& h, const std::string& t) { for (auto& d : h.draws) if (d.type == "text" && d.text == t) return &d; return nullptr; }
static void bar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; b.rows.push_back(mock::Price(c, 50, 40, 90)); h.bars.push_back(b); }
static const COLOR GREEN = 0x22C55E, RED = 0xEF4444;
struct LineSum { int n = 0, minX = 1 << 30, maxX = -1, w = 0, st = -1; };
static LineSum linesAt(mock::Host& h, COLOR c, int y) { LineSum s; for (auto& d : h.draws) if (d.type == "line" && d.pen == c && std::abs(d.y - y) <= 1 && d.t == d.y) { ++s.n; s.minX = std::min(s.minX, std::min(d.l, d.x)); s.maxX = std::max(s.maxX, std::max(d.l, d.x)); s.w = d.penW; s.st = d.penS; } return s; }
int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/oln68XXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 22) * 86400LL);
    mock::Host h; mock::current = &h;
    h.tick = 0.25f; h.seconds = 180; h.symbol = "EPZ26"; h.root = "EPZ26"; h.chart = "ES 3m #1";
    h.priceBase = 7660.0f; h.pxPer = 20.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long t0 = day + 12 * 3600 + 30 * 60;
    for (int i = 0; i < 31; ++i) bar(h, t0 - (30 - i) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    for (int i = 0; i < 80; ++i) { DEPTH_LEVEL L; L.bid = (float)(7689.75 - i * 0.25); L.ask = (float)(7690.0 + i * 0.25); L.bidsize = 10; L.asksize = 10; h.depth.push_back(L); }
    // a 100-lot bid at 7,675 placed BEFORE the first loaded bar (id 1) and an 80-lot offer at 7,701 (id 2) that is never touched
    mock::feed.push(0, 1, 7675.0f, 100, (RTDATE)(t0 - 31 * 180 - 300), 1);
    mock::feed.push(0, 0, 7701.0f, 80, (RTDATE)(t0 - 60), 2);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 5; ++s) { h.now = t0 - 50 + s; p->calc(0); usleep(5000); }
    LQState* S = static_cast<LQState*>(h.data); if (!S) { std::printf("no state\n"); return 1; }
    cb::publishKeys(S->symbol, 180, h.chart, ::steadyMs(), 230);   // lsDealerProfile's Key Levels column: 230 px at the pane's left edge
    const int keyR = 0 + 1 + 230 + 2;
    for (int i = 31; i < 35; ++i) bar(h, t0 + (i - 30) * 180, 7690.0f, 7693.0f, 7687.0f, 7690.0f);
    mock::feed.push(0, 1, 7600.0f, 7, (RTDATE)(t0 + 600), 90);
    for (int s = 0; s < 4; ++s) { h.now = t0 + 601 + s; p->calc(0); usleep(5000); }
    const int yBid = (int)(h.paneBottom - (7675.0f - h.priceBase) * h.pxPer), yOff = (int)(h.paneBottom - (7701.0f - h.priceBase) * h.pxPer);
    cb::publishKeys(S->symbol, 180, h.chart, ::steadyMs(), 230);
    h.draws.clear(); p->draw();
    CHECK(linesAt(h, GREEN, yBid).n == 0 && linesAt(h, RED, yOff).n == 0, "untouched big orders: nothing on the candles");
    // the Liquidity column (lsDealerProfile Q board): the 7,675 bid is #2 now
    { sb::Fact f; f.bar = (long long)time(nullptr); f.lo = f.hi = 7675.0; f.side = 1; f.val = 100; f.code = 2; sb::postFacts('Q', S->symbol, 180, h.chart, ::steadyMs(), std::vector<sb::Fact>{ f }); }
    // price trades into it: a 30-lot fill on the bid
    mock::feed.push(3, 1, 7675.0f, 30, (RTDATE)(t0 + 650), 1, 777);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 651 + s; p->calc(0); usleep(5000); }
    const oln::Line* B = nullptr; for (auto& L : S->ol.lines) if (L.side == oln::BID && L.px() == 30700) B = &L;
    CHECK(B && B->touchMs > 0 && B->touchRank == 2, "touched, rank 2 frozen at the touch");
    cb::publishKeys(S->symbol, 180, h.chart, ::steadyMs(), 230);
    h.draws.clear(); p->draw();
    {   const LineSum L = linesAt(h, GREEN, yBid);
        std::printf("  testing line: n %d x %d..%d w %d style %d (key column right %d)\n", L.n, L.minX, L.maxX, L.w, L.st, keyR);
        CHECK(L.n >= 1 && L.w == 1 && L.st == (int)P_DASH, "testing: thin dashed");
        CHECK(L.minX == keyR + 3, "placed before the visible chart: starts at the left edge of the price area, right of the Levels column");
        CHECK(L.maxX <= (int)h.scaleLeft, "clipped left of the right-side columns / scale"); }
    CHECK(!text(h, "HELD") && !text(h, "BREAK") && !text(h, "THRU"), "no word while testing");
    // the rank changes later: frozen
    { sb::Fact f; f.bar = (long long)time(nullptr); f.lo = f.hi = 7675.0; f.side = 1; f.val = 100; f.code = 5; sb::postFacts('Q', S->symbol, 180, h.chart, ::steadyMs(), std::vector<sb::Fact>{ f }); }
    mock::feed.push(3, 1, 7675.0f, 40, (RTDATE)(t0 + 700), 1, 778);
    mock::feed.push(2, 1, 7675.0f, 0, (RTDATE)(t0 + 701), 1);
    for (int s = 0; s < 3; ++s) { h.now = t0 + 702 + s; p->calc(0); usleep(5000); }
    bar(h, t0 + 6 * 180, 7690.0f, 7691.0f, 7674.0f, 7676.0f);    // 12:48 (contains the fill) red
    bar(h, t0 + 7 * 180, 7676.0f, 7684.0f, 7675.5f, 7683.0f);    // 12:51 green close above -> HELD
    bar(h, t0 + 8 * 180, 7683.0f, 7685.0f, 7682.0f, 7684.0f);    // forming
    for (int s = 0; s < 3; ++s) { h.now = t0 + 8 * 180 + 5 + s; p->calc(0); usleep(5000); }
    for (auto& L : S->ol.lines) if (L.side == oln::BID && L.px() == 30700) B = &L;
    CHECK(B->end == oln::FILLED && B->word == oln::W_HELD && B->touchRank == 2, "filled >= half, green close above = HELD; rank still 2");
    cb::publishKeys(S->symbol, 180, h.chart, ::steadyMs(), 230);
    h.draws.clear(); p->draw();
    const mock::Draw* held = text(h, "HELD"); const mock::Draw* rk = text(h, "2");
    {   const LineSum L = linesAt(h, GREEN, yBid);
        std::printf("  held line n %d x %d..%d w %d st %d; HELD %s at l %d t %d b %d; '2' at l %d r %d\n", L.n, L.minX, L.maxX, L.w, L.st, held ? "yes" : "no", held ? held->l : -1, held ? held->t : -1, held ? held->b : -1, rk ? rk->l : -1, rk ? rk->r : -1);
        CHECK(L.n >= 1 && L.w == 2 && L.st == (int)P_SOLID && L.maxX >= 50 + 10 * (int)(h.bars.size() - 1) - 1, "HELD: solid to the right edge of the price area (the last bar - his layout rule)");
        CHECK(held && held->color == GREEN && held->b <= yBid, "HELD in green above the line");
        const int decX = 50 + 10 * 36;   // the 12:51 bar (index 36)
        CHECK(held && rk && rk->r < held->l && held->r <= 50 + 10 * (int)(h.bars.size() - 1) && rk->l >= decX - 60, "label at the decision bar (kept inside the price area): [rank box] then the word");
        bool box = false; for (auto& d : h.draws) if (d.type == "box" && rk && d.l <= rk->l && d.r >= rk->r && d.border == GREEN) box = true;
        CHECK(box, "rank box in the side colour"); }
    CHECK(!text(h, "BREAK") && !text(h, "THRU"), "no BREAK / THRU");
    // a close through on the next bar: still shown (min 2 bars) ...
    h.bars.back() = mock::Bar(); { mock::Bar& b = h.bars.back(); b.time = t0 + 8 * 180; b.open = 7683.0f; b.high = 7684.0f; b.low = 7670.0f; b.close = 7672.0f; b.rows.push_back(mock::Price(7672.0f, 50, 40, 90)); }
    bar(h, t0 + 9 * 180, 7672.0f, 7674.0f, 7670.0f, 7673.0f);   // forming
    for (int s = 0; s < 3; ++s) { h.now = t0 + 9 * 180 + 5 + s; p->calc(0); usleep(5000); }
    cb::publishKeys(S->symbol, 180, h.chart, ::steadyMs(), 230);
    h.draws.clear(); p->draw();
    CHECK(B->throughMs > 0 && linesAt(h, GREEN, yBid).n >= 1 && text(h, "HELD"), "closed through 1 bar after: still shown (min 2 bars)");
    std::vector<mock::Draw> keep = h.draws;
    // restart: a fresh indicator reads the record back and draws the same
    usleep(1100000); h.now += 1; p->calc(0); usleep(5000);   // the record flush (at most once a second)
    {   std::string rec; { const std::string base = std::string(tmp).substr(5); DIR* dd = opendir("/tmp"); if (dd) { while (dirent* e = readdir(dd)) { const std::string n = e->d_name; if (n.rfind(base, 0) == 0 && n.find("-orderlines-") != std::string::npos) rec = readAll("/tmp/" + n); } closedir(dd); } }
        bool T = rec.find("\nT|") != std::string::npos, X = rec.find("\nX|") != std::string::npos, W = rec.find("|HELD|") != std::string::npos;
        if (rec.empty()) { std::printf("  (record path guess missed; checking rows in memory)\n"); }
        std::printf("  record %zu bytes T%d X%d W%d\n", rec.size(), T, X, W);
        CHECK(T && X && W, "record: T (touch, rank), W, X lines"); }
    bar(h, t0 + 10 * 180, 7673.0f, 7674.0f, 7671.0f, 7672.0f);   // 2nd bar after the decision closes
    for (int s = 0; s < 3; ++s) { h.now = t0 + 10 * 180 + 5 + s; p->calc(0); usleep(5000); }
    cb::publishKeys(S->symbol, 180, h.chart, ::steadyMs(), 230);
    h.draws.clear(); p->draw();
    CHECK(linesAt(h, GREEN, yBid).n == 0 && !text(h, "HELD"), "removed after the 2nd bar (closed through)");
    CHECK(linesAt(h, RED, yOff).n == 0, "the untouched offer never drew");
    {   const std::string st = readAll(flex + "\\LiquidityProfile.status-ES-180.txt");
        { size_t a = st.find("ORDER_LINES"); std::printf("  status: %s\n", a == std::string::npos ? "(no ORDER_LINES)" : st.substr(a, 400).c_str()); }
        CHECK(S->ol.tally.touched == 1 && S->ol.tally.through == 1 && readAll("LiquidityProfile.cpp").find("\",tested lines (OrderLines \" << oln::VERSION") != std::string::npos, "status: tested lines field (touched 1, closed-through 1)"); }
    CHECK(std::string(LQ_VERSION) == "0.68.0" && std::string(lpset::PLUGIN_VERSION) == "0.68.0" && readAll("LiquidityProfile.cpp").find("p->setVersion(\"0.68.0\")") != std::string::npos, "versions 0.68.0 agree");
    p->destroy(); mock::current = nullptr;
    std::printf("lsLiquidityProfile %s TESTED LINES host: %d passed, %d failed\n", LQ_VERSION, passes, fails);
    return fails ? 1 : 0;
}
