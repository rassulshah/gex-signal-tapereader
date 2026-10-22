// test_tor_host.cpp -- lsTurnOrder 1.0.0 on the mock host: the order feed (DEPTH_BY_ORDER) -> the engine -> ONE solid green bar at the
// order's price on the pivot candle (no outline, no text), the record file (M line), the status file (version / market / bar size),
// the hover (title in green, question ... rows), restart = the same bar from the record and no new line, settings: show off removes it,
// a colour applies, garbled settings = defaults; a red bar at a high.
#include "../TurnOrder.cpp"
#include <cstdio>
#include <unistd.h>
namespace mock { Host* current = nullptr; Feed feed; }
namespace off { long long off_test_now_ms = 1000000; void* off_test_cursor_wnd = nullptr; }
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
static const unsigned long T0 = 1791500000UL / 180 * 180;   // seconds (the mock's localtime is the CT wall clock)
static std::string readAll(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream ss_; ss_ << f.rdbuf(); return ss_.str(); }
static void bar(mock::Host& h, int k, float o, float hi, float lo, float c) { mock::Bar b; b.time = T0 + 180UL * (k + 1); b.open = o; b.high = hi; b.low = lo; b.close = c; b.volume = 100 + k; h.bars.push_back(b); }
static std::vector<const mock::Draw*> boxes(mock::Host& h, COLOR c) { std::vector<const mock::Draw*> v; for (auto& d : h.draws) if (d.type == "box" && d.color == c && d.border == c) v.push_back(&d); return v; }
static void lowChart(mock::Host& h) {   // the logic test's scenario in prices: low 995.00 on candle 20, a 80-lot bid at 994.75 (1 tick under)
    h.bars.clear();
    for (int k = 0; k < 18; ++k) bar(h, k, 1000, 1001, 999, 1000);
    bar(h, 18, 1000, 1000.5f, 998, 998.5f); bar(h, 19, 998.5f, 998.75f, 996.5f, 996.75f); bar(h, 20, 996.75f, 997, 995, 995.75f);
    bar(h, 21, 995.75f, 997.75f, 995.5f, 997.5f); bar(h, 22, 997.5f, 998.25f, 995.5f, 998); bar(h, 23, 998, 998.5f, 997.5f, 998.25f);   // 23 = forming
}
static void lowFeed() {
    mock::feed = mock::Feed();
    const unsigned long tx = T0 + 20 * 180 + 100;
    mock::feed.push(0, 0, 1002.5f, 5, T0 + 1, 999);
    mock::feed.push(0, 1, 994.75f, 80, T0 + 18 * 180 + 5, 2);       // buySell 1 = a bid
    mock::feed.push(0, 1, 995.0f, 30, T0 + 18 * 180, 1);
    mock::feed.push(3, 0, 995.0f, 10, tx, 1);                       // a FILL at the low (the print that times the extreme)
    mock::feed.push(1, 1, 995.0f, 20, tx, 1);
}
int main() {
    char tmpl[] = "/tmp/tor100XXXXXX"; const char* tmp = mkdtemp(tmpl); setenv("USERPROFILE", tmp, 1);
    const std::string flex = std::string(tmp) + "\\InvestorRT\\rtx\\lsFlexLevels";
    mkdir((std::string(tmp) + "\\InvestorRT").c_str(), 0700);
    mock::Host h; mock::current = &h; h.tick = 0.25f; h.seconds = 180; h.symbol = "ESZ6"; h.root = "ES"; h.period = "3 Minutes"; h.paneBottom = 600; h.priceBase = 980; h.ppb = 10; h.barW = 12;
    h.visFrom = 0; h.visTo = 23; lowChart(h); h.now = (RTDATE)(T0 + 23 * 180 + 30); lowFeed();
    const COLOR GRN = 0x22C55E, RED = 0xEF4444;
    TurnOrder* p = static_cast<TurnOrder*>(CreateExtension()); p->setup();
    h.draws.clear(); p->draw();
    off::off_test_now_ms += 4000; h.draws.clear(); p->draw();          // the newest closed candle after its 3 s grace
    TorState* S = p->stateFor();
    CHECK(S && S->mkt == "ES" && S->spb == 180 && S->routed >= 5, "chart identified, the feed's events routed to the engine");
    CHECK(S && S->E.marks.size() == 1, "one turn order decided");
    if (S && !S->E.marks.empty()) { const tor::Mark& m = S->E.marks[0];
        CHECK(m.side == tor::BID && m.px == 3979 && m.shown == 80 && !m.absorbed && m.oid == 2, "the 80-lot bid 1 tick under the low; untouched = TOO THICK"); }
    std::vector<const mock::Draw*> g = boxes(h, GRN);
    cppExtension::PNT pt; pt.set(20, 994.75f);
    CHECK(g.size() == 1 && g[0]->b - g[0]->x >= 0 && g[0]->r - g[0]->x >= 8 && std::abs((g[0]->y + g[0]->b) / 2 - pt.v) <= 1 && g[0]->x <= pt.h && g[0]->r >= pt.h, "ONE green solid bar at 994.75 on the pivot candle (>= 8 px wide)");
    CHECK(g.size() == 1 && g[0]->b - g[0]->y == 4 && g[0]->pen == GRN, "4 px thick, pen = fill (no white outline)");
    int texts = 0; for (auto& d : h.draws) texts += d.type == "text"; CHECK(texts == 0, "no text on the chart");
    const std::string rec = flex + "\\TurnOrder\\ES-180-" + "20261008" + ".csv";
    std::string body = readAll(rec);
    if (body.empty()) { for (int dd = 1; dd <= 31 && body.empty(); ++dd) { char b[32]; std::snprintf(b, sizeof b, "202610%02d", dd); body = readAll(flex + "\\TurnOrder\\ES-180-" + b + ".csv"); } }
    CHECK(body.find("\nM|1.0.0|B|") != std::string::npos && body.find("|994.75|80|0|") != std::string::npos && body[0] == '#', "the record: header + the M line (price 994.75, 80 shown, 0 filled)");
    const std::string st = readAll(flex + "\\TurnOrder.status-ES-180.txt");
    CHECK(st.find("VERSION,1.0.0") == 0 && st.find("MARKET,ES") != std::string::npos && st.find("BAR_SECONDS,180") != std::string::npos && st.find("MARKS,1 on record") != std::string::npos && st.find("FEED,") != std::string::npos, "status file: version, market, bar size, marks, feed");
    // the hover: the cursor on the bar
    hb::fakeCursor() = 7; RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)pt.h; e.v.mouse.v = (short)pt.v; p->mouse(&e);
    h.draws.clear(); p->draw();
    const mock::Draw* title = nullptr; int rows = 0; for (auto& d : h.draws) if (d.type == "text") { ++rows; if (!title) title = &d; }
    CHECK(title && title->text == "Bid 80 lots at 994.75 stopped the selloff - 0 filled, price turned " + tor::x100(S->E.marks[0].maxRun100) + " ATR ?" && title->color == GRN, "hover title in green");
    CHECK(rows == 9, "9 hover rows (question / what / result / why / how / later / example / ?)");
    bool hasQ = false, thick = false; for (auto& d : h.draws) if (d.type == "text") { hasQ = hasQ || d.text.compare(0, 9, "Question:") == 0; thick = thick || d.text.compare(0, 10, "TOO THICK:") == 0; }
    CHECK(hasQ && thick, "hover has the question and TOO THICK");
    e.v.mouse.h = 5; e.v.mouse.v = 5; p->mouse(&e); h.draws.clear(); p->draw();
    rows = 0; for (auto& d : h.draws) rows += d.type == "text"; CHECK(rows == 0, "mouse away: no hover");
    // restart: a new chart object (state deleted) with the same feed replayed and the same bars -> the same bar from the record, no new line
    const size_t len = body.size();
    p->release(); lowFeed();
    TurnOrder* q = static_cast<TurnOrder*>(CreateExtension()); q->setup();
    h.draws.clear(); q->draw(); off::off_test_now_ms += 4000; h.draws.clear(); q->draw();
    TorState* S2 = q->stateFor();
    CHECK(S2 && S2->E.marks.size() == 1 && S2->restored == 1 && boxes(h, GRN).size() == 1, "restart: the mark comes back from the record, drawn once");
    std::string body2; for (int dd = 1; dd <= 31 && body2.empty(); ++dd) { char b[32]; std::snprintf(b, sizeof b, "202610%02d", dd); body2 = readAll(flex + "\\TurnOrder\\ES-180-" + b + ".csv"); }
    CHECK(body2.size() == len, "restart wrote no new line (never re-decided)");
    // settings: show off -> nothing drawn; colour applies; garbled = defaults
    { std::ofstream f((flex + "\\TurnOrder.settings-ES.txt").c_str()); ss::Values V = ss::defaults(torset::spec(), 0); V.v[torset::K_SHOW] = 0; f << ss::format(torset::spec(), V); }
    S2->setCheckMs = -100000; S2->setStore = ss::Store(); off::off_test_now_ms += 3000; h.draws.clear(); q->draw();
    CHECK(boxes(h, GRN).empty(), "Turn orders unchecked: nothing drawn");
    { std::ofstream f((flex + "\\TurnOrder.settings-ES.txt").c_str()); ss::Values V = ss::defaults(torset::spec(), 0); V.v[torset::K_COL_BID] = 0x3366FF; f << ss::format(torset::spec(), V); }
    S2->setCheckMs = -100000; S2->setStore = ss::Store(); off::off_test_now_ms += 3000; h.draws.clear(); q->draw();
    CHECK(boxes(h, 0x3366FF).size() == 1 && boxes(h, GRN).empty(), "the Bid colour applies");
    { std::ofstream f((flex + "\\TurnOrder.settings-ES.txt").c_str()); f << "garbage\n"; }
    S2->setCheckMs = -100000; S2->setStore = ss::Store(); off::off_test_now_ms += 3000; h.draws.clear(); q->draw();
    CHECK(boxes(h, GRN).size() == 1 && S2->E.P.gateAtr == 0.5, "garbled settings: defaults (green, 0.5 ATR)");
    q->release();
    // a high: a red bar for the offer that stopped the rally (a fresh market folder: NQ)
    {
        mock::Host h2; mock::current = &h2; h2.tick = 0.25f; h2.seconds = 180; h2.symbol = "NQZ6"; h2.root = "NQ"; h2.period = "3 Minutes"; h2.paneBottom = 600; h2.priceBase = 980; h2.barW = 12; h2.visFrom = 0; h2.visTo = 23;
        for (int k = 0; k < 18; ++k) bar(h2, k, 1000, 1001, 999, 1000);
        bar(h2, 18, 1000, 1002, 999.75f, 1001.5f); bar(h2, 19, 1001.5f, 1003.5f, 1001.25f, 1003.25f); bar(h2, 20, 1003.25f, 1005, 1003, 1004.25f);
        bar(h2, 21, 1004.25f, 1004.5f, 1002.25f, 1002.5f); bar(h2, 22, 1002.5f, 1002.75f, 1001.5f, 1002); bar(h2, 23, 1002, 1002.25f, 1001.5f, 1001.75f);
        h2.now = (RTDATE)(T0 + 23 * 180 + 30);
        mock::feed = mock::Feed(); mock::feed.push(0, 1, 990.0f, 5, T0 + 1, 999); mock::feed.push(0, 0, 1005.25f, 60, T0 + 17 * 180, 11);
        mock::feed.push(3, 1, 1005.0f, 1, T0 + 20 * 180 + 100, 999999);
        TurnOrder* r = static_cast<TurnOrder*>(CreateExtension()); r->setup(); r->draw(); off::off_test_now_ms += 4000; h2.draws.clear(); r->draw();
        TorState* S3 = r->stateFor();
        CHECK(S3 && S3->E.marks.size() == 1 && S3->E.marks[0].side == tor::ASK && boxes(h2, RED).size() == 1 && boxes(h2, GRN).empty(), "a high: ONE red bar (the 60-lot offer 1 tick above)");
        r->release();
    }
    std::printf("test_tor_host: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
