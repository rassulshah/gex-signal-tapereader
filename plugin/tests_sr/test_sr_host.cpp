// lsSweepRefill 1.0.0 host tests on the mock SDK: the MBO feed -> a sweep of 10 bid prices through the 5-candle low -> refilled ->
// a green "Sr" ABOVE the candle (past the 2 reserved lanes), decided at 4 s, recorded (SR line); restart redraws the same; never
// repainted; the hover box; settings (bullish off, colour, garbage = defaults); the bearish mirror drawn red BELOW; two charts.
#include "irtsdk.h"
namespace mock { Feed feed; Host* current = nullptr; }
static long long g_ms = 1000000;
long long swrTestClockMs() { return g_ms; }
#include "../SweepRefill.cpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
static int fails = 0, checks = 0;
#define CHECK(c, m) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static std::string HOME;
static std::vector<mock::Draw> texts(const std::string& t) { std::vector<mock::Draw> o; for (const mock::Draw& d : mock::current->draws) if (d.type == "text" && d.text == t) o.push_back(d); return o; }
static void addBar(mock::Host& h, unsigned long t, float o, float hi, float lo, float c) { mock::Bar b; b.time = t; b.open = o; b.high = hi; b.low = lo; b.close = c; h.bars.push_back(b); }
static void ev(unsigned char a, unsigned char bs, float px, unsigned long sz, int id, unsigned long stamp) { DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.timestamp = stamp; o.price = px; o.size = sz; o.orderID = id; o.action = a; o.buySell = bs; mock::feed.ev.push_back(o); }
static std::vector<std::string> lines(const std::string& p) { std::vector<std::string> v; std::ifstream f(p.c_str()); std::string l; while (std::getline(f, l)) v.push_back(l); return v; }
static int drawOnce(SweepRefill* p) { mock::current->draws.clear(); return p->draw(); }
static const unsigned long T0 = 1791500400UL;   // a bar END time
static void setup(mock::Host& h) {
    h.symbol = "ESZ26"; h.root = "ES"; h.tick = 0.25f; h.seconds = 180; h.priceBase = 7660; h.paneTop = 0; h.paneBottom = 1000; h.paneRight = 2000; h.barW = 10;
    for (int i = 0; i < 10; ++i) addBar(h, T0 + 180UL * (unsigned long)i, 7676, 7680 - (float)(i % 2), 7674 + 0.25f * (float)(i % 3), 7677);
    h.now = T0 + 180UL * 9 - 100;            // inside the forming candle (bar 9)
}
// 10 bids 7674.00 .. 7671.75 (10 lots each) resting 10 s, swept at `st`; `back` lots per price re-added 0.5 s later
static void sweepBids(unsigned long st, int idBase, unsigned long back) {
    for (int k = 0; k < 10; ++k) ev(0, 1, 7674.0f - 0.25f * (float)k, 10, idBase + k, st - 10);
    (void)back;
}
static void fillBids(unsigned long st, int idBase) { for (int k = 0; k < 10; ++k) { ev(3, 1, 7674.0f - 0.25f * (float)k, 10, idBase + k, st); ev(2, 1, 7674.0f - 0.25f * (float)k, 0, idBase + k, st); } }
static void refillBids(unsigned long st, int idBase, unsigned long lots) { for (int k = 0; k < 10; ++k) ev(0, 1, 7674.0f - 0.25f * (float)k, lots, idBase + k, st); }
static void run(SweepRefill* p, int ms, int step = 100) { for (int t = 0; t < ms; t += step) { g_ms += step; drawOnce(p); } }

int main() {
    char tmpl[] = "/tmp/swr_host_XXXXXX"; HOME = mkdtemp(tmpl); setenv("USERPROFILE", HOME.c_str(), 1);
    const std::string flex = HOME + "/InvestorRT/rtx/lsFlexLevels"; std::system(("mkdir -p " + flex).c_str());
    mock::Host h; mock::current = &h; setup(h);
    SweepRefill* p = static_cast<SweepRefill*>(CreateExtension());
    p->setup();
    CHECK((h.flags & TRACK_MOUSE) && (h.flags & OVERLAY), "price-pane overlay that tracks the mouse");
    const unsigned long st = (unsigned long)h.now;
    sweepBids(st, 5000, 0);
    drawOnce(p);                                                   // primes: history candles, opens the feed, drains the resting bids
    CHECK(mock::feed.opens == 1 && mock::feed.lastTicker == "ESZ26", "one MBO object, opened with the chart symbol");
    g_ms += 10000;
    fillBids(st + 10, 5000); g_ms += 5; drawOnce(p);             // the sweep (one exchange second, one drain)
    g_ms += 500; refillBids(st + 10, 6000, 8); drawOnce(p);      // 80 lots back
    SwrState* S = p->testState();
    CHECK(S && S->D.pending.size() == 1, "one candidate pending");
    run(p, 3000);
    CHECK(S->marks.empty(), "not decided before 4 s");
    run(p, 1200);
    CHECK(S->marks.size() == 1 && S->marks[0].refilled && S->marks[0].gapOk && S->marks[0].prices == 10 && S->marks[0].pre == 100 && S->marks[0].post == 80, "decided: refilled 80%");
    std::vector<mock::Draw> t = texts("Sr");
    CHECK(t.size() == 1 && t[0].color == 0x22C55E, "a green Sr drawn");
    if (t.size() == 1) { cppExtension::PNT q; q.set(9, h.bars[9].high); CHECK(t[0].y < q.v - 2 * 16 && std::abs((t[0].x + t[0].r) / 2 - q.h) <= 3, "above the candle, past the BO / Rly lanes"); }
    const std::string rec = S->recPath;
    std::vector<std::string> L = lines(rec);
    CHECK(L.size() == 2 && L[1].compare(0, 3, "SR|") == 0 && L[1].find("|bull|") != std::string::npos && L[1].find("|10|100|100|80|800|") != std::string::npos, "the candidate recorded (SR line)");
    // hover
    if (!S->hits.empty()) { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((S->hits[0].l + S->hits[0].r) / 2); e.v.mouse.v = (short)((S->hits[0].t + S->hits[0].b) / 2);
        CHECK(p->mouse(&e) == RTX_FAIL && S->hover == 0, "hover found, event not swallowed"); drawOnce(p);
        int rows = 0; bool header = false, read = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.fsize == 10 && d.text != "Sr") { ++rows; if (d.text.find("Sweep refilled?") != std::string::npos) header = true; if (d.text.find("Swept 10 prices (7674.00 - 7671.75) through") == 0) read = true; }
        CHECK(rows == 8 && header && read, "hover box: 8 rows, header, read line");
        bool box = false; for (const mock::Draw& d : h.draws) if (d.type == "box" && d.width == 1) box = true; CHECK(box, "hover box with a solid 1 px border");
        e.v.mouse.h = 5; e.v.mouse.v = 5; p->mouse(&e); CHECK(S->hover == -1, "mouse away: no hover"); }
    // later candles / events never change it
    for (int k = 0; k < 3; ++k) { addBar(h, h.bars.back().time + 180, 7660, 7661, 7650, 7651); h.now += 180; run(p, 1000, 500); }
    CHECK(texts("Sr").size() == 1 && lines(rec).size() == 2, "never decided again, never moves");
    std::vector<mock::Draw> before = texts("Sr");
    // RESTART: same chart -> the record redraws the same mark
    p->destroy(); delete p; mock::feed.ev.clear(); mock::feed.at = 0;
    SweepRefill* q = static_cast<SweepRefill*>(CreateExtension()); q->setup(); g_ms += 5000; drawOnce(q);
    std::vector<mock::Draw> after = texts("Sr");
    CHECK(after.size() == 1 && before.size() == 1 && after[0].x == before[0].x && after[0].y == before[0].y && after[0].color == before[0].color, "restart: identical Sr at the same place");
    CHECK(lines(rec).size() == 2 && q->testState()->D.lastDrawnBar[swr::BID] == q->testState()->marks[0].barT, "restart writes nothing, the gap rule continues from the record");
    // settings: colour + bullish off, from the per-market keyed file
    const std::string sf = flex + "/SweepRefill.settings-ES.txt";
    { std::ofstream f(sf.c_str()); f << "SPEC1|SweepRefill|1.0.0|ES\nshow_bull=1\ncol_bull=123456\nshow_bear=1\ncol_bear=EF4444\nmin_prices=8\nmin_lots=30\nrefill_pct=70\nwindow_s=3\ngap_bars=1\nfont=12\n"; }
    g_ms += 5000; drawOnce(q);
    std::vector<mock::Draw> a = texts("Sr"); CHECK(a.size() == 1 && a[0].color == 0x123456 && a[0].fsize == 12, "colour + font from the settings file");
    { std::ofstream f(sf.c_str()); f << "SPEC1|SweepRefill|1.0.0|ES\nshow_bull=0\ncol_bull=123456\nshow_bear=1\n"; }
    g_ms += 5000; drawOnce(q);
    CHECK(texts("Sr").empty(), "bullish off: nothing drawn");
    { std::ofstream f(sf.c_str()); f << "SPEC1|SweepRefill|1.0.0|ES\nshow_bull=1\nmin_prices=9999\nwindow_s=abc\nfont=99\n"; }
    g_ms += 5000; drawOnce(q);
    SwrState* S2 = q->testState(); CHECK(S2 && S2->D.P.minPrices == 8 && S2->D.P.refillMs == 3000 && texts("Sr").size() == 1 && texts("Sr")[0].fsize == 10, "garbage values = the defaults");
    std::vector<std::string> st2 = lines(flex + "/SweepRefill/SweepRefill.status-ES-180.txt");
    CHECK(st2.size() >= 3 && st2[0] == "VERSION,1.0.0" && st2[2].find("STATUS,lsSweepRefill v1.0.0") == 0, "status line written with the version");
    std::remove(sf.c_str());
    q->destroy(); delete q;
    // a second chart (NQ) on the same DLL, its own state; the bearish mirror: offers swept up through the 5-candle high -> red Sr BELOW
    {
        mock::Host h2; mock::current = &h2; setup(h2); h2.symbol = "NQZ26"; h2.root = "NQ";
        SweepRefill* r = static_cast<SweepRefill*>(CreateExtension()); r->setup();
        const unsigned long s2 = (unsigned long)h2.now;
        for (int k = 0; k < 9; ++k) ev(0, 0, 7680.25f + 0.25f * (float)k, 10, 8000 + k, s2 - 10);
        drawOnce(r); g_ms += 10000;
        for (int k = 0; k < 9; ++k) { ev(3, 0, 7680.25f + 0.25f * (float)k, 10, 8000 + k, s2 + 10); ev(2, 0, 7680.25f + 0.25f * (float)k, 0, 8000 + k, s2 + 10); }
        g_ms += 5; drawOnce(r);
        g_ms += 400; for (int k = 0; k < 9; ++k) ev(0, 0, 7680.25f + 0.25f * (float)k, 9, 9000 + k, s2 + 10);
        drawOnce(r); run(p == nullptr ? r : r, 4500);
        std::vector<mock::Draw> red = texts("Sr");
        CHECK(r->testState() && r->testState()->market == "NQ" && red.size() == 1 && red[0].color == 0xEF4444, "second chart: its own state; a red bearish Sr");
        if (red.size() == 1) { cppExtension::PNT q2; q2.set(9, h2.bars[9].low); CHECK(red[0].y > q2.v + 2 * 16, "below the candle, past the BD / Rly lanes"); }
        r->destroy(); delete r; mock::current = &h;
    }
    // garbled record: torn / foreign lines are skipped, good lines kept
    { std::ofstream f(rec.c_str(), std::ios::app); f << "SR|half\nST|x|y\n"; }
    SweepRefill* z = static_cast<SweepRefill*>(CreateExtension()); g_ms += 1000; drawOnce(z);
    CHECK(texts("Sr").size() == 1 && z->testState()->recBad == 2, "bad lines counted, the good mark kept");
    z->destroy(); delete z;
    std::printf("%d checks, %d failed\n", checks, fails);
    std::system(("rm -rf " + HOME).c_str());
    return fails ? 1 : 0;
}
