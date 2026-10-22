// lsStopRun 1.0.0 host tests on the mock SDK: feed -> grey St drawn at the stop price -> decided -> Sta green; record ST + SA;
// restart redraws the same; never repainted; settings show / colour; hover box; two charts on one DLL.
#include "irtsdk.h"
namespace mock { Feed feed; Host* current = nullptr; }
static long long g_ms = 1000000;
long long srmTestClockMs() { return g_ms; }
#include "../StopRunMark.cpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static std::string HOME;
static std::vector<mock::Draw> texts(const std::string& t) { std::vector<mock::Draw> o; for (const mock::Draw& d : mock::current->draws) if (d.type == "text" && d.text == t) o.push_back(d); return o; }
static void addBar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; h.bars.push_back(b); }
static void fill(unsigned long stamp, float px, unsigned long sz, int restId, int aggId, unsigned char restBuySell) {
    DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.timestamp = stamp; o.price = px; o.size = sz; o.orderID = restId; o.aggressorOrderID = aggId; o.action = 3; o.buySell = restBuySell; mock::feed.ev.push_back(o);
}
static std::vector<std::string> lines(const std::string& p) { std::vector<std::string> v; std::ifstream f(p.c_str()); std::string l; while (std::getline(f, l)) v.push_back(l); return v; }
static int drawOnce(StopRunMark* p) { mock::current->draws.clear(); return p->draw(); }
static void setup(mock::Host& h) {
    h.symbol = "ESZ26"; h.root = "ES"; h.tick = 0.25f; h.seconds = 180; h.priceBase = 7660; h.paneTop = 0; h.paneBottom = 1000; h.paneRight = 2000; h.barW = 10;
    const unsigned long T0 = 1791500400UL;   // a bar END time
    for (int i = 0; i < 10; ++i) addBar(h, T0 + 180UL * (unsigned long)i, 7674, 7676, 7673, 7674.5f);
    h.bars[8].low = 7672.5f;                 // the prior candle low = the level
    h.now = T0 + 180UL * 9 - 100;            // inside the forming candle (bar 9)
}
int main() {
    char tmpl[] = "/tmp/srm_host_XXXXXX"; HOME = mkdtemp(tmpl); setenv("USERPROFILE", HOME.c_str(), 1);
    const std::string flex = HOME + "/InvestorRT/rtx/lsFlexLevels"; std::system(("mkdir -p " + flex).c_str());
    mock::Host h; mock::current = &h; setup(h);
    StopRunMark* p = static_cast<StopRunMark*>(CreateExtension());
    p->setup();
    CHECK((h.flags & TRACK_MOUSE) && (h.flags & OVERLAY), "price-pane overlay that tracks the mouse");
    drawOnce(p);                                                   // primes: opens the feed, the closed candles are context only
    CHECK(mock::feed.opens == 1 && mock::feed.lastTicker == "ESZ26", "one MBO object, opened with the chart symbol");
    const unsigned long st = (unsigned long)h.now;
    // the burst: 10 sellers' fills 7672.75 -> 7672.00 hitting bids, distinct aggressor ids, 3 lots each + a 25-lot bid id at 7672.25
    for (int i = 0; i < 10; ++i) fill(st, 7672.75f - 0.25f * (float)(i / 3), 3, 9000 + i, 500 + i, 1);
    fill(st, 7672.25f, 25, 4242, 777, 1);
    g_ms += 10; drawOnce(p);
    g_ms += 200; drawOnce(p);                                      // the window passed: the burst is over -> St
    SrmState* S = p->testState();
    CHECK(S && S->marks.size() == 1, "one stop run");
    std::vector<mock::Draw> t = texts("St");
    CHECK(t.size() == 1 && t[0].color == 0x9CA3AF, "a grey St drawn");
    if (t.size() == 1) { cppExtension::PNT q; q.set(9, 7672.25f); CHECK(std::abs(t[0].y - q.v) <= 1 && std::abs((t[0].x + t[0].r) / 2 - q.h) <= 3, "at the stop price, on the forming candle"); }
    const std::string rec = S ? S->recPath : std::string();
    std::vector<std::string> L = lines(rec);
    CHECK(L.size() == 2 && L[1].compare(0, 3, "ST|") == 0 && L[1].find("|down|5-candle low|7672.50|7672.25|") != std::string::npos, "the fire recorded (ST line)");
    // hover: the mouse over the mark -> the box with the header and the 9 rows
    if (!S->hits.empty()) { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((S->hits[0].l + S->hits[0].r) / 2); e.v.mouse.v = (short)((S->hits[0].t + S->hits[0].b) / 2);
        CHECK(p->mouse(&e) == RTX_FAIL && S->hover == 0, "hover found, event not swallowed"); drawOnce(p);
        int rows = 0; bool header = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.fsize == 10 && d.text != "St") { ++rows; if (d.text.find("Stop run?") != std::string::npos) header = true; }
        CHECK(rows == 9 && header, "hover box: 9 rows, header 'Stop run?'");
        bool box = false; for (const mock::Draw& d : h.draws) if (d.type == "box" && d.width == 1) box = true; CHECK(box, "hover box drawn with a solid 1 px border");
        e.v.mouse.h = 5; e.v.mouse.v = 5; p->mouse(&e); CHECK(S->hover == -1, "mouse away: no hover"); }
    // candle 9 closes BELOW the level (a later bar appears): still St
    addBar(h, h.bars.back().time + 180, 7672, 7672.5f, 7671, 7671.25f); h.bars[9].low = 7671.5f; h.bars[9].close = 7671.75f; h.now += 180;
    g_ms += 1000; drawOnce(p);
    CHECK(texts("St").size() == 1 && texts("Sta").empty(), "candle 1 below: still grey St");
    // candle 10 closes back ABOVE 7672.50 -> absorbed (the 25-lot bid id) -> Sta green
    h.bars[10].close = 7673.25f; h.bars[10].high = 7673.5f; addBar(h, h.bars.back().time + 180, 7673, 7674, 7673, 7673.5f); h.now += 180;
    g_ms += 1000; drawOnce(p);
    std::vector<mock::Draw> a = texts("Sta");
    CHECK(a.size() == 1 && a[0].color == 0x22C55E && texts("St").empty(), "upgraded once to a green Sta");
    L = lines(rec);
    CHECK(L.size() == 3 && L[2].compare(0, 3, "SA|") == 0 && L[2].find("|absorbed|2|1|") != std::string::npos, "the decision recorded (SA: absorbed, 2 candles, 1 big id)");
    // later candles never change it (no repaint)
    for (int k = 0; k < 4; ++k) { addBar(h, h.bars.back().time + 180, 7660, 7661, 7650, 7651); h.now += 180; g_ms += 1000; drawOnce(p); }
    CHECK(texts("Sta").size() == 1 && lines(rec).size() == 3, "never decided again, the mark never moves");
    std::vector<mock::Draw> before = texts("Sta");
    // RESTART: a new object + state, same chart -> the record redraws the same mark
    p->destroy(); delete p; mock::feed.ev.clear(); mock::feed.at = 0;
    StopRunMark* q = static_cast<StopRunMark*>(CreateExtension()); q->setup(); g_ms += 5000; drawOnce(q);
    std::vector<mock::Draw> after = texts("Sta");
    CHECK(after.size() == 1 && before.size() == 1 && after[0].x == before[0].x && after[0].y == before[0].y && after[0].color == before[0].color, "restart: identical Sta at the same place");
    CHECK(lines(rec).size() == 3, "restart writes nothing new");
    // settings: colour + show off, from the per-market keyed file
    { std::ofstream f((flex + "/StopRunMark.settings-ES.txt").c_str()); f << "SPEC1|StopRunMark|1.0.0|ES\nshow=1\ncol_st=9CA3AF\ncol_bull=123456\ncol_bear=EF4444\nwindow_ms=50\nmin_fills=5\nratio=2\nmin_ids=2\nnear_ticks=4\nbig_lots=20\nabsorb_bars=3\nfont=12\n"; }
    g_ms += 5000; drawOnce(q);
    a = texts("Sta"); CHECK(a.size() == 1 && a[0].color == 0x123456 && a[0].fsize == 12, "colour + font from the settings file");
    { std::ofstream f((flex + "/StopRunMark.settings-ES.txt").c_str()); f << "SPEC1|StopRunMark|1.0.0|ES\nshow=0\ncol_st=9CA3AF\ncol_bull=123456\ncol_bear=EF4444\nwindow_ms=50\nmin_fills=5\nratio=2\nmin_ids=2\nnear_ticks=4\nbig_lots=20\nabsorb_bars=3\nfont=12\n"; }
    g_ms += 5000; drawOnce(q);
    CHECK(texts("Sta").empty() && texts("St").empty(), "show off: nothing drawn");
    { std::ofstream f((flex + "/StopRunMark.settings-ES.txt").c_str()); f << "SPEC1|StopRunMark|1.0.0|ES\nshow=1\nwindow_ms=99999\nfont=abc\n"; }
    g_ms += 5000; drawOnce(q);
    SrmState* S2 = q->testState(); CHECK(S2 && S2->D.P.windowMs == 50 && texts("Sta").size() == 1 && texts("Sta")[0].fsize == 10, "garbage values = the defaults");
    // status file
    std::vector<std::string> st2 = lines(flex + "/StopRun/StopRun.status-ES-180.txt");
    CHECK(st2.size() >= 3 && st2[0] == "VERSION,1.0.0" && st2[2].find("STATUS,lsStopRun v1.0.0") == 0, "status line written with the version");
    // a second chart (NQ) on the same DLL: its own state, its own record, nothing of ES drawn
    mock::Host h2; mock::current = &h2; setup(h2); h2.symbol = "NQZ26"; h2.root = "NQ";
    StopRunMark* r = static_cast<StopRunMark*>(CreateExtension()); g_ms += 1000; drawOnce(r);
    CHECK(texts("Sta").empty() && texts("St").empty() && r->testState() && r->testState()->market == "NQ", "second chart: separate state");
    r->destroy(); delete r; mock::current = &h; q->destroy(); delete q;
    // garbled record: torn / foreign lines are skipped, good lines kept
    { std::ofstream f(rec.c_str(), std::ios::app); f << "SA|zzz|1|absorbed|1|1|0|1\nST|half\n"; }
    StopRunMark* z = static_cast<StopRunMark*>(CreateExtension()); g_ms += 1000; drawOnce(z);
    CHECK(texts("Sta").size() == 1 && z->testState()->recBad == 2, "bad lines counted, the good mark kept");
    z->destroy(); delete z;
    std::printf("%d checks, %d failed\n", checks, fails);
    std::system(("rm -rf " + HOME).c_str());
    return fails ? 1 : 0;
}
