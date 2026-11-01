// lsStopRun 1.2.0 host tests on the mock SDK: RTH candles -> the ZigZag swing low 7672.50; the feed -> a 3x burst through it -> grey
// "St?" (GC-style verdict) at the stop price -> 10 s later back above + a 25-lot bid filled -> green "Sta?" (NQ-style verdict); record
// ST + SA (1.2.0 fields, the verdicts stamped); restart redraws the same; never repainted (params changed to LOG later); settings show /
// colour / font; hover box; the same-candle rule (a drawn Sr hides the St); no params file = recorded, not drawn; VAP flag; two charts.
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
static const unsigned long T0 = 20735UL * 86400UL + 9UL * 3600UL;   // 9:00 bar END (the mock clock is UTC = CT here)
static void setup(mock::Host& h, const char* sym, const char* root) {
    h.symbol = sym; h.root = root; h.tick = 0.25f; h.seconds = 180; h.priceBase = 7660; h.paneTop = 0; h.paneBottom = 1000; h.paneRight = 2000; h.barW = 10;
    addBar(h, T0, 7676, 7680, 7675, 7679);              // 0 green: up
    addBar(h, T0 + 180, 7678, 7679, 7673, 7673.5f);     // 1 lower low -> swing high 7680
    addBar(h, T0 + 360, 7673.5f, 7676, 7672.5f, 7675);  // 2 the low extends
    addBar(h, T0 + 540, 7675, 7679, 7674, 7678);        // 3 higher high -> swing low 7672.50
    for (int i = 4; i < 9; ++i) addBar(h, T0 + 180UL * (unsigned long)i, 7678, 7680, 7675, 7679);
    addBar(h, T0 + 180UL * 9, 7676, 7677, 7673, 7674);  // 9 forming
    for (size_t i = 0; i < h.bars.size(); ++i) { mock::Price a; a.price = 7677; a.buy = 60; a.sell = 40; a.total = 100; h.bars[i].rows.push_back(a); }
    h.now = T0 + 180UL * 9 - 100;
}
static void params(const std::string& flex, const std::string& m, const char* grey, const char* bull) {
    std::ofstream f((flex + "/LRA-StopSweep-" + m + ".csv").c_str());
    f << "VERSION,1\nMARKET," << m << "\nPARAM,stRefPivots,1\nPARAM,stRatio,3.0\nPARAM,stNearTicks,4\nPARAM,staBigLots,20\nPARAM,staDecideLive,1\nPARAM,staReclaimSec,10\n"
      << "STA_BULL_SHOW," << bull << "\nSTA_BEAR_SHOW,LOG\nST_GREY_SHOW," << grey << "\n";
}
// the burst: 10 sellers' fills 7673.00 -> 7672.00 hitting bids (5 lots each, distinct aggressor ids) incl. a 25-lot bid id at 7672.25
static void burst(unsigned long st) {
    for (int i = 0; i < 9; ++i) fill(st, 7673.0f - 0.25f * (float)(i / 2), 5, 9000 + i, 500 + i, 1);
    fill(st, 7672.25f, 25, 4242, 777, 1);
}
int main() {
    char tmpl[] = "/tmp/srm_host_XXXXXX"; HOME = mkdtemp(tmpl); setenv("USERPROFILE", HOME.c_str(), 1);
    const std::string flex = HOME + "/InvestorRT/rtx/lsFlexLevels"; std::system(("mkdir -p " + flex).c_str());
    params(flex, "ES", "?", "?");
    mock::Host h; mock::current = &h; setup(h, "ESZ26", "ES");
    StopRunMark* p = static_cast<StopRunMark*>(CreateExtension());
    p->setup();
    CHECK((h.flags & TRACK_MOUSE) && (h.flags & OVERLAY) && (h.flags & VAP_REQUIRED), "price-pane overlay, tracks the mouse, volume at price");
    drawOnce(p);
    SrmState* S = p->testState();
    CHECK(mock::feed.opens == 1 && mock::feed.lastTicker == "ESZ26", "one MBO object, opened with the chart symbol");
    CHECK(S && S->D.Z.swings.size() == 2 && S->D.Z.swings[1].px == 7672.5 * 4 && S->D.V.now.ok && S->D.V.now.poc == 7677 * 4, "ZigZag + volume profile from the closed candles");
    CHECK(S && S->prm.loaded && S->D.P.ratio == 3.0 && S->D.P.refPivots == 3 && S->D.P.showGrey == swg::QUERY, "params loaded (stRefPivots 1 ignored: the last 3 swings, as Sr)");
    const unsigned long st = (unsigned long)h.now;
    burst(st);
    g_ms += 10; drawOnce(p);
    g_ms += 200; drawOnce(p);
    CHECK(S->marks.size() == 1 && S->marks[0].refKind == stl::REF_SWING && S->marks[0].refPx == 7672.5 * 4 && S->marks[0].refBack == 1, "one stop run through the swing low");
    std::vector<mock::Draw> t = texts("St?");
    CHECK(t.size() == 1 && t[0].color == 0x9CA3AF, "a grey St? drawn (verdict ?)");
    if (t.size() == 1) { cppExtension::PNT q; q.set(9, 7672.25f); CHECK(std::abs(t[0].y - q.v) <= 1, "at the stop price"); }
    const std::string rec = S->recPath;
    std::vector<std::string> L = lines(rec);
    CHECK(L.size() == 2 && L[1].compare(0, 3, "ST|") == 0 && L[1].find("|down|swing low|7672.50|7672.25|") != std::string::npos && L[1].find("|swing|" + std::to_string(T0 + 360) + "|1|?") != std::string::npos, "the fire recorded (ST, 19 fields, verdict ?)");
    if (!S->hits.empty()) { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)((S->hits[0].l + S->hits[0].r) / 2); e.v.mouse.v = (short)((S->hits[0].t + S->hits[0].b) / 2);
        CHECK(p->mouse(&e) == RTX_FAIL && S->hover == 0, "hover found, event not swallowed"); drawOnce(p);
        int rows = 0; bool header = false, head = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.fsize == 10 && d.text != "St?") { ++rows; if (d.text.find("St?  Stop run?") == 0) header = true; if (d.text.find("Stop run at 7672.25 below the swing low 7672.50 from 9:03 AM: ") == 0) head = true; }
        CHECK(rows == 9 && header && head, "hover box: 9 rows, header, names the swing");
        e.v.mouse.h = 5; e.v.mouse.v = 5; p->mouse(&e); CHECK(S->hover == -1, "mouse away: no hover"); }
    // a buyer lifts 7672.75 (back above 7672.50) 2 s later; at 10 s: back above + the 25-lot bid filled -> absorbed
    fill(st + 2, 7672.75f, 1, 7001, 802, 0);
    g_ms += 2000; drawOnce(p);
    CHECK(S->marks[0].outcome == stl::PENDING, "not decided before 10 s");
    g_ms += 8500; drawOnce(p);
    std::vector<mock::Draw> a = texts("Sta?");
    CHECK(a.size() == 1 && a[0].color == 0x22C55E && texts("St?").empty(), "upgraded once to a green Sta? at 10 s");
    L = lines(rec);
    CHECK(L.size() == 3 && L[2].compare(0, 3, "SA|") == 0 && L[2].find("|absorbed|") != std::string::npos && L[2].find("|25|?") != std::string::npos, "the decision recorded (SA: absorbed, biggest 25 lots, verdict ?)");
    { std::vector<std::string> hr = stl::hoverRows(S->marks[0], 0.25, 10, 20); CHECK(hr[3].find("Absorbed at ") == 0 && hr[3].find("back above the level, 1 big resting bid filled (biggest 25 lots)") != std::string::npos, "hover outcome row"); }
    // later candles never change it
    for (int k = 0; k < 3; ++k) { addBar(h, h.bars.back().time + 180, 7660, 7661, 7650, 7651); h.now += 180; g_ms += 1000; drawOnce(p); }
    CHECK(texts("Sta?").size() == 1 && lines(rec).size() == 3, "never decided again, never moves");
    std::vector<mock::Draw> before = texts("Sta?");
    // the params change to LOG: the decided mark keeps its stamped verdict (never repainted); RESTART redraws the same
    params(flex, "ES", "LOG", "LOG");
    p->destroy(); delete p; mock::feed.ev.clear(); mock::feed.at = 0;
    StopRunMark* q = static_cast<StopRunMark*>(CreateExtension()); q->setup(); g_ms += 6000; drawOnce(q);
    std::vector<mock::Draw> after = texts("Sta?");
    CHECK(after.size() == 1 && before.size() == 1 && after[0].x == before[0].x && after[0].y == before[0].y && after[0].color == before[0].color, "restart (params now LOG): identical Sta? at the same place");
    CHECK(lines(rec).size() == 3 && q->testState()->D.P.showBull == swg::LOG, "restart writes nothing; new marks would be LOG");
    // settings: colour + font, show off
    const std::string sf = flex + "/StopRunMark.settings-ES.txt";
    { std::ofstream f(sf.c_str()); f << "SPEC1|StopRunMark|1.2.0|ES\nshow=1\ncol_st=9CA3AF\ncol_bull=123456\ncol_bear=EF4444\nmin_ids=2\nfont=12\n"; }
    g_ms += 5000; drawOnce(q);
    a = texts("Sta?"); CHECK(a.size() == 1 && a[0].color == 0x123456 && a[0].fsize == 12, "colour + font from the settings file");
    { std::ofstream f(sf.c_str()); f << "SPEC1|StopRunMark|1.2.0|ES\nshow=0\n"; }
    g_ms += 5000; drawOnce(q);
    CHECK(texts("Sta?").empty(), "show off: nothing drawn");
    std::remove(sf.c_str()); g_ms += 5000; drawOnce(q);
    // the same-candle rule: a drawn Sr on this candle -> only the Sr (the St / Sta is hidden, still recorded)
    {
        const std::string srDir = flex + "/SweepRefill"; std::system(("mkdir -p " + srDir).c_str());
        SrmState* Q = q->testState();
        swr::Event e; e.key = "1-b-1"; e.barT = Q->marks[0].barT; e.sec = st; e.ct = "9:27:00 AM"; e.side = swr::BID; e.ref = "swing low"; e.refKind = swr::REF_PIVOT; e.refPx = 7672 * 4;
        e.hiPx = 7673 * 4; e.loPx = 7672 * 4; e.prices = 8; e.lots = 40; e.pre = 40; e.post = 30; e.refill = 75; e.ttrMs = 2000; e.refilled = true; e.gapOk = true; e.show = swg::SHOW;
        std::ofstream f((srDir + "/ES-180-" + Q->session + ".csv").c_str()); f << swr::REC_HEAD << "\n" << swr::recLine(e, 0.25) << "\n"; f.close();
        g_ms += 4000; drawOnce(q);
        CHECK(texts("Sta?").empty() && Q->srBars.count(Q->marks[0].barT) == 1 && Q->marks.size() == 1, "a drawn Sr on the candle: only the Sr (the Sta is hidden, still recorded)");
        std::remove((srDir + "/ES-180-" + Q->session + ".csv").c_str());
    }
    std::vector<std::string> stt = lines(flex + "/StopRun/StopRun.status-ES-180.txt");
    bool ver = stt.size() > 2 && stt[0] == "VERSION,1.2.1" && stt[2].find("STATUS,lsStopRun v1.2.1") == 0, prm = false, ref = false;
    for (const std::string& x : stt) { if (x.find("PARAMS,LRA-StopSweep-ES.csv loaded") == 0 && x.find("Sta bullish LOG") != std::string::npos) prm = true; if (x.find("REFERENCE,ZigZag") == 0 && x.find("POC 7677.00") != std::string::npos) ref = true; }
    CHECK(ver && prm && ref, "status: version, params, the reference (ZigZag, POC)");
    q->destroy(); delete q;
    // a second chart (NQ) with NO params file: a run is recorded, nothing drawn (every mark LOG)
    {
        mock::Host h2; mock::current = &h2; setup(h2, "NQZ26", "NQ");
        StopRunMark* r = static_cast<StopRunMark*>(CreateExtension()); r->setup(); drawOnce(r);
        CHECK(r->testState()->prm.why.find("no file") == 0 && r->testState()->D.P.showGrey == swg::LOG, "no params file: every mark LOG");
        for (int i = 0; i < 12; ++i) fill((unsigned long)h2.now, 7673.0f - 0.25f * (float)(i / 3), 5, 19000 + i, 1500 + i, 1);
        g_ms += 10; drawOnce(r); g_ms += 200; drawOnce(r);
        CHECK(r->testState()->market == "NQ" && r->testState()->marks.size() == 1 && texts("St").empty() && texts("St?").empty(), "NQ chart: its own state; the run recorded, not drawn");
        r->destroy(); delete r; mock::current = &h;
    }
    std::printf("%d checks, %d failed\n", checks, fails);
    std::system(("rm -rf " + HOME).c_str());
    return fails ? 1 : 0;
}
