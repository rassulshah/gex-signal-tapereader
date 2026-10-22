// lsSweepRefill 1.1.0 host tests on the mock SDK: RTH candles -> the ZigZag (swing high 7680, swing low 7672.50); the MBO feed -> a
// sweep of 10 bid prices through the swing low -> refilled (50% within 5 s, still there at 10 s) -> a green "Sr" UNDER the candle
// (past the 2 reserved lanes), the swing line, the ZigZag line, recorded (SR line, 23 fields); restart redraws the same; never repainted;
// the hover box; settings (bullish off, colour, ZigZag off, swing line off, garbage = defaults); the params file (LOG = not drawn,
// missing = LOG); the bearish mirror drawn "Sr?" red OVER the high on a second (NQ) chart.
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
static const unsigned long T0 = 20735UL * 86400UL + 9UL * 3600UL;   // a bar END time: 9:00 (the mock clock is UTC = CT here)
static void setup(mock::Host& h) {
    h.symbol = "ESZ26"; h.root = "ES"; h.tick = 0.25f; h.seconds = 180; h.priceBase = 7660; h.paneTop = 0; h.paneBottom = 1000; h.paneRight = 2000; h.barW = 10;
    addBar(h, T0, 7676, 7680, 7675, 7679);              // 0 green: up
    addBar(h, T0 + 180, 7678, 7679, 7673, 7673.5f);     // 1 lower low -> swing high 7680 (0)
    addBar(h, T0 + 360, 7673.5f, 7676, 7672.5f, 7675);  // 2 the low extends
    addBar(h, T0 + 540, 7675, 7679, 7674, 7678);        // 3 higher high -> swing low 7672.50 (2)
    for (int i = 4; i < 9; ++i) addBar(h, T0 + 180UL * (unsigned long)i, 7678, 7680, 7675, 7679);
    addBar(h, T0 + 180UL * 9, 7678, 7679, 7676, 7677);  // 9 the forming candle
    for (size_t i = 0; i < h.bars.size(); ++i) { mock::Price a; a.price = 7677; a.buy = 60; a.sell = 40; a.total = 100; mock::Price b = a; b.price = 7676; b.total = 30; h.bars[i].rows.push_back(a); h.bars[i].rows.push_back(b); }
    h.now = T0 + 180UL * 9 - 100;            // inside the forming candle (bar 9)
}
static void params(const std::string& flex, const std::string& m, const std::string& bull, const std::string& bear) {
    std::ofstream f((flex + "/LRA-StopSweep-" + m + ".csv").c_str());
    f << "VERSION,1\nMARKET," << m << "\nPARAM,srMinPrices,8\nPARAM,srRefillPct,50\nPARAM,srWindowSec,5\nPARAM,srStaySec,5\nSR_BULL_SHOW," << bull << "\nSR_BEAR_SHOW," << bear << "\n";
}
static int penLines(unsigned long pen) { int n = 0; for (const mock::Draw& d : mock::current->draws) if (d.type == "line" && d.pen == pen) ++n; return n; }
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
    params(flex, "ES", "SHOW", "LOG");
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
    CHECK(S && S->D.Z.swings.size() == 2 && S->D.Z.swings[1].px == 7672.5 / 0.25, "ZigZag from the RTH candles: swing high, swing low 7672.50");
    CHECK(S && S->D.pending.size() == 1 && S->D.pending[0].e.refKind == swr::REF_PIVOT && S->D.pending[0].e.refT == (long long)(T0 + 360), "through the swing low (its candle)");
    run(p, 9000);
    CHECK(S->marks.empty(), "not decided before 10 s");
    run(p, 1600);
    CHECK(S->marks.size() == 1 && S->marks[0].refilled && S->marks[0].gapOk && S->marks[0].prices == 10 && S->marks[0].pre == 100 && S->marks[0].post == 80 && S->marks[0].show == swg::SHOW, "decided: refilled 80%, SHOW");
    std::vector<mock::Draw> t = texts("Sr");
    CHECK(t.size() == 1 && t[0].color == 0x22C55E, "a green Sr drawn");
    if (t.size() == 1) { cppExtension::PNT q; q.set(9, h.bars[9].low); CHECK(t[0].b - 14 >= q.v + 2 * 16 && std::abs((t[0].x + t[0].r) / 2 - q.h) <= 3, "UNDER the candle, past the BD / Rly lanes"); }
    CHECK(penLines(0x93C5FD) == 1, "the ZigZag: one segment between the two confirmed swings");
    CHECK(penLines(0x22C55E) == 1, "the swing line in the Sr colour");
    const std::string rec = S->recPath;
    std::vector<std::string> L = lines(rec);
    CHECK(L.size() == 2 && L[1].compare(0, 3, "SR|") == 0 && L[1].find("|bull|swing low|swing|7672.50|") != std::string::npos && L[1].find("|10|100|100|80|800|") != std::string::npos && L[1].find("|1|SHOW") != std::string::npos, "the candidate recorded (SR line, the swing, SHOW)");
    // hover
    if (!S->hits.empty()) { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((S->hits[0].l + S->hits[0].r) / 2); e.v.mouse.v = (short)((S->hits[0].t + S->hits[0].b) / 2);
        CHECK(p->mouse(&e) == RTX_FAIL && S->hover == 0, "hover found, event not swallowed"); drawOnce(p);
        int rows = 0; bool header = false, read = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.fsize == 10 && d.text != "Sr") { ++rows; if (d.text.find("Sweep refilled?") != std::string::npos) header = true; if (d.text.find("Swept 10 prices (7674.00 - 7671.75) through the swing low 7672.50 from 9:03 AM") == 0) read = true; }
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
    { std::ofstream f(sf.c_str()); f << "SPEC1|SweepRefill|1.1.0|ES\nshow_bull=1\ncol_bull=123456\nshow_bear=1\ncol_bear=EF4444\nshow_zz=1\ncol_zz=654321\nshow_ref=1\ngap_bars=1\nfont=12\n"; }
    g_ms += 5000; drawOnce(q);
    std::vector<mock::Draw> a = texts("Sr"); CHECK(a.size() == 1 && a[0].color == 0x123456 && a[0].fsize == 12, "colour + font from the settings file");
    CHECK(penLines(0x654321) >= 1 && penLines(0x123456) == 1, "ZigZag colour from the settings; swing line in the Sr colour");
    { std::ofstream f(sf.c_str()); f << "SPEC1|SweepRefill|1.1.0|ES\nshow_bull=1\nshow_zz=0\nshow_ref=0\n"; }
    g_ms += 5000; drawOnce(q);
    CHECK(texts("Sr").size() == 1 && penLines(0x93C5FD) == 0 && penLines(0x22C55E) == 0, "ZigZag off, swing line off: only the Sr");
    { std::ofstream f(sf.c_str()); f << "SPEC1|SweepRefill|1.1.0|ES\nshow_bull=0\ncol_bull=123456\nshow_bear=1\n"; }
    g_ms += 5000; drawOnce(q);
    CHECK(texts("Sr").empty(), "bullish off: nothing drawn");
    { std::ofstream f(sf.c_str()); f << "SPEC1|SweepRefill|1.1.0|ES\nshow_bull=1\ngap_bars=abc\nfont=99\n"; }
    g_ms += 5000; drawOnce(q);
    SwrState* S2 = q->testState(); CHECK(S2 && S2->D.P.gapBars == 1 && texts("Sr").size() == 1 && texts("Sr")[0].fsize == 10, "garbage values = the defaults");
    std::vector<std::string> st2 = lines(flex + "/SweepRefill/SweepRefill.status-ES-180.txt");
    CHECK(st2.size() >= 4 && st2[0] == "VERSION,1.1.1" && st2[2].find("STATUS,lsSweepRefill v1.1.1") == 0, "status line written with the version");
    bool pl = false; for (const std::string& x : st2) if (x.find("PARAMS,LRA-StopSweep-ES.csv loaded") == 0 && x.find("bullish SHOW,bearish LOG") != std::string::npos) pl = true;
    CHECK(pl, "status: the params file and the verdicts");
    bool vp = false; for (const std::string& x : st2) if (x.find("VOLUME_PROFILE,ok") == 0 && x.find("POC 7677.00") != std::string::npos) vp = true;
    CHECK(vp && (h.flags & VAP_REQUIRED), "volume profile from the chart's VAP: POC 7677.00 in the status");
    std::remove(sf.c_str());
    q->destroy(); delete q;
    // a second chart (NQ) on the same DLL, its own state; the bearish mirror: offers swept up through the swing high 7680 -> red "Sr?" OVER the candle
    {
        params(flex, "NQ", "SHOW", "?");
        mock::Host h2; mock::current = &h2; setup(h2); h2.symbol = "NQZ26"; h2.root = "NQ";
        SweepRefill* r = static_cast<SweepRefill*>(CreateExtension()); r->setup();
        const unsigned long s2 = (unsigned long)h2.now;
        for (int k = 0; k < 9; ++k) ev(0, 0, 7680.25f + 0.25f * (float)k, 10, 8000 + k, s2 - 10);
        drawOnce(r); g_ms += 10000;
        for (int k = 0; k < 9; ++k) { ev(3, 0, 7680.25f + 0.25f * (float)k, 10, 8000 + k, s2 + 10); ev(2, 0, 7680.25f + 0.25f * (float)k, 0, 8000 + k, s2 + 10); }
        g_ms += 5; drawOnce(r);
        g_ms += 400; for (int k = 0; k < 9; ++k) ev(0, 0, 7680.25f + 0.25f * (float)k, 9, 9000 + k, s2 + 10);
        drawOnce(r); run(r, 11000);
        std::vector<mock::Draw> red = texts("Sr?");
        CHECK(r->testState() && r->testState()->market == "NQ" && red.size() == 1 && red[0].color == 0xEF4444 && r->testState()->marks[0].refPx == 7680 * 4, "second chart: its own state; a red bearish Sr? through the swing high 7680");
        if (red.size() == 1) { cppExtension::PNT q2; q2.set(9, h2.bars[9].high); CHECK(red[0].b <= q2.v - 2 * 16, "OVER the candle, past the BO / Rly lanes"); }
        r->destroy(); delete r; mock::current = &h;
    }
    // the params file says LOG for bullish (ES verdict): a new chart's new sweep is recorded, not drawn; the old mark keeps its SHOW
    {
        params(flex, "ES", "LOG", "LOG");
        SweepRefill* y = static_cast<SweepRefill*>(CreateExtension()); y->setup(); g_ms += 6000; drawOnce(y);
        CHECK(texts("Sr").size() == 1, "params changed to LOG: the decided mark is never repainted");
        y->destroy(); delete y;
        std::remove((flex + "/LRA-StopSweep-ES.csv").c_str());
        SweepRefill* x = static_cast<SweepRefill*>(CreateExtension()); x->setup(); g_ms += 6000; drawOnce(x);
        CHECK(x->testState()->prm.srBull == swg::LOG && x->testState()->D.P.showBull == swg::LOG && x->testState()->prm.why.find("no file") == 0, "no params file: every mark LOG");
        x->destroy(); delete x;
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
