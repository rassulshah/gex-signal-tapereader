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
static Box hoverBox(mock::Host& h) {
    Box x;
    for (auto& d : h.draws) if (d.type == "box" && d.color == 0x0011151C) { x.found = true; x.l = d.x; x.t = d.y; x.r = d.r; x.b = d.b; }
    if (!x.found) return x;
    for (auto& d : h.draws) if (d.type == "text" && d.x > x.l && d.x < x.r && d.y >= x.t && d.y < x.b) { x.rows.push_back(std::make_pair(d.text, d.color)); x.minFont = std::min(x.minFont, d.fsize); }
    return x;
}
static void mouseAt(LiquidityProfile* p, float price, long bar) { RTX_EVENT e; std::memset(&e, 0, sizeof(e)); e.type = E_MOUSE_MOVE; e.v.mouse.price = price; e.v.mouse.bar = bar; p->mouse(&e); }
static int nextId = 100;
static void order(unsigned char action, float px, unsigned long size, RTDATE t, int id, int side = 1) { mock::feed.push(action, (unsigned char)side, px, size, t, id); }

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    char tmpl[] = "/tmp/lqhXXXXXX"; const char* tmp = mkdtemp(tmpl);
    setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    {   std::ofstream nf((flex + "\\LRA-LiqNorms-CL.csv").c_str()); nf << "VERSION,test\r\nMARKET,CL\r\nZONE,0.25,4,8\r\n"; }
    {   std::ofstream mq((flex + "\\MenthorQLevels.csv").c_str()); mq << "SYMBOL,PRICE,LABEL\r\nCLEX26,79.95,PS 0D\r\n"; }   // the zone holds PS
    mock::Host h; mock::current = &h;
    h.tick = 0.01f; h.seconds = 180; h.symbol = "CLEX26"; h.root = "CLEX26";
    h.priceBase = 79.5f; h.pxPer = 1000.0f; h.barW = 10; h.barX0 = 50; h.paneBottom = 1000; h.paneRight = 2000; h.scaleLeft = 1980;
    const unsigned long day = (unsigned long)(civilDays(2026, 10, 12) * 86400LL);
    const unsigned long t0 = day + 9 * 3600;
    for (int i = 0; i < 25; ++i) {
        mock::Bar b; b.time = t0 - (24 - i) * 180; b.open = 80.05f; b.high = 80.12f; b.low = 79.98f; b.close = 80.03f;
        b.rows.push_back(mock::Price(80.03f, 50, 40, 90)); h.bars.push_back(b);
    }
    { mock::Bar f; f.time = t0 + 180; f.open = f.high = f.low = f.close = 80.03f; f.rows.push_back(mock::Price(80.03f, 0, 0, 0)); h.bars.push_back(f); }
    // the 79.95 support: 320 lots over the book's 10, near price (80.00, 5 ticks)
    setDepth(h, 80.00, 80, 79.95, 320);
    // its orders on the order-by-order feed: the zone is 79.90 - 79.98 (9 ticks): 8 ticks x 10 lots + at 79.95 3 x 10 + one of 300
    // = 410 lots = the depth's 410 (coverage 100%); top order 300 / 410 = 73%
    const int big = nextId++;
    for (int k = 0; k <= 8; ++k) if (k != 5) order(0, 79.90f + 0.01f * (float)k, 10, (RTDATE)t0, nextId++);
    for (int k = 0; k < 3; ++k) order(0, 79.95f, 10, (RTDATE)t0, nextId++);
    order(0, 79.95f, 300, (RTDATE)t0, big);
    LiquidityProfile* p = static_cast<LiquidityProfile*>(CreateExtension());
    for (int s = 0; s < 45; ++s) { h.now = t0 + s; p->calc(0); usleep(10000); }
    LQState* S = static_cast<LQState*>(h.data);
    CHECK(S && S->e.haveBook, "fixture: depth accepted");
    if (!S) { std::printf("no state\n"); return 1; }
    std::printf("  feed: %s / %s, orders %zu, events %lld\n", S->feed.stateText(steadyMs()).c_str(), S->feedState.c_str(), S->ob.book.size(), S->feed.events);
    CHECK(S->ob.book.size() == 12, "the order feed's 12 orders are in the order book");
    const lqp::Zone* z = nullptr; for (auto& q : S->e.zones) if (q.side == lqp::BID && q.lo <= 7995 && q.hi >= 7995) z = &q;
    CHECK(z && z->near && z->shown, "fixture: the 79.95 support is a shown NEAR zone");
    const long long zid = z ? z->id : -1;
    CHECK(z && z->nearBase > 0, "its baseline was recorded when it came within reach");
    const double base = z ? z->nearBase : -1;
    // ---- no mouse: nothing new on the chart face
    h.draws.clear(); p->draw();
    CHECK(!hoverBox(h).found, "no mouse over a zone: no box (chart face unchanged)");
    const size_t faceDraws = h.draws.size();
    // ---- the mouse over the zone's bar in the column (bar 0 sits at x 50, inside the column)
    mouseAt(p, 79.95f, 0);
    CHECK(S->hovId == zid, "the mouse over the near zone's bar picks it");
    h.draws.clear(); p->draw();
    Box b = hoverBox(h);
    CHECK(b.found, "the hover box is drawn");
    CHECK(h.draws.size() > faceDraws, "the hover only ADDS the box (the face draws are the same)");
    for (auto& r : b.rows) std::printf("  row [%s] %06lx\n", r.first.c_str(), (unsigned long)r.second);
    CHECK(b.rows.size() == 4, "4 rows: header, orders, one order, as price came (not hit yet)");
    if (b.rows.size() == 4) {
        CHECK(b.rows[0].first.compare(0, 23, "Support at PS 0D 79.93 ") == 0 || b.rows[0].first.compare(0, 20, "Support at PS 0D 79.") == 0, "1 'Support at <LEVEL>' - the level the zone holds");
        CHECK(b.rows[0].first.find(" lots (") != std::string::npos && b.rows[0].second == C_BUY, "1 ... lots (x), green");
        CHECK(b.rows[1].first == "Orders 12 " + DOT + " avg 34 lots " + DOT + " Top order 73%" && b.rows[1].second == WHITE, "2 'Orders 12 \xB7 avg 34 lots \xB7 Top order 73%', white");
        CHECK(b.rows[2].first == "Mostly one order - may not reload ?" && b.rows[2].second == WHITE, "3 one order 73% (>= 70%): 'Mostly one order - may not reload ?', white");
        CHECK(b.rows[3].first == "As price came: unchanged" && b.rows[3].second == WHITE, "4 unchanged, white");
    }
    CHECK(b.minFont >= 10, "hover fonts 10 pt or larger");
    {   // never over the column, never over the Delta Profile (or its labels)
        int colEnd = 0; for (auto& d : h.draws) if (!(d.color == 0x0011151C) && (d.type == "box") && d.x <= 2) colEnd = std::max(colEnd, (int)d.r);
        const int colRight = 1 + S->colRight;
        const int dpLimit = (h.scaleLeft - 2) - 2 - S->dealerReach - S->dpGap - S->dpWidth - 64;
        std::printf("  box x %d..%d, column ends %d (bars %d), Delta Profile labels from %d\n", b.l, b.r, colRight, colEnd, dpLimit);
        CHECK(b.l > colRight && b.l > colEnd, "the box starts right of the column (bars and labels)");
        CHECK(b.r <= dpLimit, "the box ends left of the Delta Profile's labels");
        bool dashed = false; for (auto& d : h.draws) if (d.type == "box" && d.color == 0x0011151C && d.pat != PAT_SOLID) dashed = true;
        CHECK(!dashed, "solid box");
    }
    // ---- the zone builds as price comes: 90 more lots at 79.95 (depth + orders): 410 -> 500 = up 22%, top order 60%
    setDepth(h, 80.00, 80, 79.95, 410);
    order(0, 79.95f, 90, (RTDATE)(t0 + 46), nextId++);
    for (int s = 46; s < 50; ++s) { h.now = t0 + s; usleep(1100000 / 4); p->calc(0); }
    usleep(800000); h.now = t0 + 50; p->calc(0);
    z = nullptr; for (auto& q : S->e.zones) if (q.id == zid) z = &q;
    CHECK(z && std::fabs(z->nearBase - base) < 1e-9, "the baseline does not move while price stays near (once per approach)");
    h.draws.clear(); p->draw(); b = hoverBox(h);
    for (auto& r : b.rows) std::printf("  row [%s] %06lx\n", r.first.c_str(), (unsigned long)r.second);
    bool building = false; for (auto& r : b.rows) if (r.first.compare(0, 18, "As price came: up ") == 0 && r.first.find("- building") != std::string::npos && r.second == C_BUY) building = true;
    CHECK(building, "support building as price came: 'As price came: up NN% - building', green");
    bool one = false; for (auto& r : b.rows) if (r.first == "Mostly one order - may not reload ?") one = true;
    CHECK(!one, "the top order is now under 70% of the zone: no one-order line");
    bool hit = false; for (auto& r : b.rows) if (r.first.compare(0, 4, "Hit ") == 0) hit = true;
    CHECK(!hit, "price not in the zone: no Hit line");
    // ---- price comes into the zone: sellers hit 79.95 (fills on the feed), the bids are rebuilt
    mock::Bar& fb = h.bars.back();
    fb.rows.push_back(mock::Price(79.97f, 0, 5, 5)); fb.low = 79.97f; fb.close = 79.97f;
    setDepth(h, 79.97, 80, 79.95, 410);
    h.now = t0 + 52; usleep(1100000); p->calc(0);
    CHECK(S->e.attempts.size() == 1 && !S->e.attempts[0].done, "arrival into the zone starts an attempt");
    order(3, 79.95f, 100, (RTDATE)(t0 + 53), big);                                   // 100 of the 150 filled
    order(0, 79.95f, 60, (RTDATE)(t0 + 53), nextId++);                                // 60 re-added after the hit
    fb.rows.push_back(mock::Price(79.95f, 0, 100, 100)); fb.low = 79.95f; fb.close = 79.95f;
    setDepth(h, 79.95, 80, 79.95, 370);
    h.now = t0 + 54; usleep(1100000); p->calc(0);
    std::printf("  attempt hit %lld rebuilt %lld, last px %lld\n", S->e.attempts.empty() ? -1 : S->e.attempts[0].hitLots, S->e.attempts.empty() ? -1 : S->e.attempts[0].rebuiltLots, S->e.lastPx);
    CHECK(!S->e.attempts.empty() && S->e.attempts[0].hitLots == 100 && S->e.attempts[0].rebuiltLots == 60, "the feed's fill = 100 lots hit, the new bid after it = 60 rebuilt");
    mouseAt(p, 79.95f, 0);
    h.draws.clear(); p->draw(); b = hoverBox(h);
    for (auto& r : b.rows) std::printf("  row [%s] %06lx\n", r.first.c_str(), (unsigned long)r.second);
    CHECK(!b.rows.empty() && b.rows.back().first == "Hit 100 lots " + DOT + " rebuilt 60% - absorbing" && b.rows.back().second == C_BUY, "price in the zone: 'Hit 100 lots \xB7 rebuilt 60% - absorbing', green at support, last line");
    // ---- price leaves the zone upward (still open attempt): the Hit line goes
    fb.rows.push_back(mock::Price(80.01f, 5, 0, 5)); fb.high = 80.12f; fb.close = 80.01f;
    setDepth(h, 80.00, 80, 79.95, 370);
    h.now = t0 + 56; usleep(1100000); p->calc(0);
    h.draws.clear(); p->draw(); b = hoverBox(h);
    hit = false; for (auto& r : b.rows) if (r.first.compare(0, 4, "Hit ") == 0) hit = true;
    CHECK(b.found && !hit, "price back out of the zone: no Hit line");
    // ---- the status file: the MAKEUP line with 'as price came', the hover state
    usleep(1100000); h.now = t0 + 58; p->calc(0);
    const std::string st = readAll(flex + "\\LiquidityProfile.status-CL-180.txt");
    CHECK(st.find("MAKEUP,support,") != std::string::npos && st.find(",came building ") != std::string::npos && st.find(",came_base ") != std::string::npos, "status: MAKEUP line with the baseline and 'came building'");
    CHECK(st.find(",hover shown ") != std::string::npos, "status: hover shown N rows");
    // ---- the mouse moves away: no box
    mouseAt(p, 80.40f, 0);
    h.draws.clear(); p->draw();
    CHECK(!hoverBox(h).found && S->hovId == 0, "mouse off the zones: no box");
    mouseAt(p, 79.95f, 20);                                                            // the right price, but over the candles
    h.draws.clear(); p->draw();
    CHECK(!hoverBox(h).found, "the zone's price over the candles (not the column): no box");
    // ---- no room between the column and the Delta Profile: no box rather than an overlap
    mouseAt(p, 79.95f, 0);
    const int sr = h.scaleLeft; h.scaleLeft = 520; h.paneRight = 540;
    h.draws.clear(); p->draw();
    CHECK(!hoverBox(h).found && S->hovState == "no room", "a narrow chart: no box (never over the column or the Delta Profile)");
    h.scaleLeft = sr; h.paneRight = 2000;
    // ---- the bar closes under the zone -> the attempt is decided: the makeup record with the new columns
    fb.rows.push_back(mock::Price(79.90f, 0, 400, 400)); fb.low = 79.88f; fb.close = 79.89f;
    order(3, 79.95f, 50, (RTDATE)(t0 + 170), big);
    setDepth(h, 79.88, 80);
    h.now = t0 + 170; usleep(1100000); p->calc(0);
    { mock::Bar n; n.time = t0 + 360; n.open = n.high = n.low = n.close = 79.88f; n.rows.push_back(mock::Price(79.88f, 0, 0, 0)); h.bars.push_back(n); }
    h.now = t0 + 181; usleep(300000); p->calc(0);
    for (int k = 0; k < 25 && S->e.attempts.size() && !S->e.finished.size(); ++k) {
        mock::Bar n; n.time = h.bars.back().time + 180; n.open = n.high = n.low = n.close = 79.80f; n.rows.push_back(mock::Price(79.80f, 0, 1, 1)); h.bars.push_back(n);
        h.now += 180; usleep(50000); p->calc(0);
    }
    CHECK(!S->e.finished.empty(), "the attempt is decided");
    usleep(1100000); h.now += 2; p->calc(0);
    std::string rec; {
        const std::string dir = flex + "\\LiquidityProfile\\";
        DIR* d = opendir(tmp); (void)d; if (d) closedir(d);
        rec = readAll(dir + S->market + "-" + fileToken(S->symbol) + "-" + std::to_string(S->spb) + "-makeup-" + std::to_string(S->session) + ".csv");
    }
    std::printf("  makeup record:\n%s", rec.c_str());
    CHECK(rec.find(lqp::Engine::makeupHeader()) == 0, "the makeup record starts with the v2 header");
    CHECK(rec.find("|support|") != std::string::npos && rec.find("|building|0.15.1") != std::string::npos, "the decided attempt's line: 'as price came' = building at arrival");
    CHECK(rec.find("|150|60|40|") != std::string::npos, "hit / rebuilt / rebuilt % as of the decision (100 + the 50 filled later)");
    CHECK(rec.find("|410|500|22|building|") != std::string::npos, "came_base 410, came_arrival 500, +22%, building");
    // ---- no repaint: the record line does not change after the decision
    const std::string rec1 = rec; usleep(1100000); h.now += 5; p->calc(0);
    const std::string rec2 = readAll(flex + "\\LiquidityProfile\\" + S->market + "-" + fileToken(S->symbol) + "-" + std::to_string(S->spb) + "-makeup-" + std::to_string(S->session) + ".csv");
    CHECK(rec2 == rec1, "the decided line is never rewritten");
    // every text the plugin drew in this run is 10 pt+
    int small = 0; for (auto& d : h.draws) if (d.type == "text" && d.fsize > 0 && d.fsize < 10) small++;
    CHECK(small == 0, "all text 10 pt or larger");
    CHECK(std::string(LQ_VERSION) == "0.15.1" && std::string(lqp::VERSION) == "0.15.1", "version 0.15.0");
    p->destroy(); p->~LiquidityProfile(); ::operator delete(p);       // as the host frees it (no leak under ASan)
    std::printf("hover host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
