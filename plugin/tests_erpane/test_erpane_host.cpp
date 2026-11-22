// tests_erpane/test_erpane_host.cpp -- the WHOLE plugin (EffortRewardPane.cpp) on the mock SDK: two charts on one DLL, the real 11-20 order
// events (ES / GC, Databento -> IRT event form), restart without repaint, garbled / foreign files, symbol change, backlog cap, no leaks.
// g++ -std=c++17 -DLSPERF_FAKE -fsanitize=address,undefined -Werror -Imock -I.. ; run from tests_erpane (needs ev_ES.txt / ev_GC.txt)
#define ERP_FILE_CHECK_MS 0
#define ERP_STATUS_MS 0
#define ERP_DECIDE_WAIT_MS 0
#define E_TIMER_MOCK 0
#include "irtsdk.h"
namespace mock { Feed feed; Host* current = nullptr; }
#include "../EffortRewardPane.cpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static std::string root;
static std::string P(const std::string& rel) { return root + "\\InvestorRT\\rtx\\lsFlexLevels\\" + rel; }
static void put(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
static std::string get(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::string paramsText(const std::string& m, double med, double n15, double avg) {
    std::ostringstream s; s << "VERSION,1\nMARKET," << m << "\nABS_MED," << med << "\nHARD_CUT," << (m == "ES" ? 3.010 : 3.059) << "\nBIG_CUT," << (m == "ES" ? 12.094 : 3.309) << "\nTURN_PCT,28,38,56,67\nN15";
    for (int i = 0; i < 48; ++i) { s << "," << n15; }
    s << "\nAVG_ORDER";
    for (int i = 0; i < 48; ++i) { s << "," << avg; }
    s << "\nEND\n"; return s.str();
}
// events from the converted file in [a, b) (CT clock seconds), as DEPTH_ORDERs; VAP rows of each candle from the aggressor fills
struct Ev { long long sec, act, bs, px, sz, oid, aid; };
static std::vector<Ev> load(const char* f, long long a, long long b) {
    std::vector<Ev> v; std::ifstream in(f); Ev e;
    while (in >> e.sec >> e.act >> e.bs >> e.px >> e.sz >> e.oid >> e.aid) { if (e.sec >= b) break; if (e.sec >= a) v.push_back(e); }
    return v;
}
static void toFeed(const std::vector<Ev>& v, double tick, mock::Feed& F) {
    F.ev.clear(); F.at = 0;
    for (const Ev& e : v) { DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.timestamp = (RTDATE)e.sec; o.price = (float)(e.px * tick); o.size = (unsigned long)e.sz;
        o.orderID = (int)(e.oid & 0x7FFFFFFF); o.aggressorOrderID = (int)(e.aid & 0x7FFFFFFF); o.action = (unsigned char)e.act; o.buySell = (unsigned char)e.bs; F.ev.push_back(o); }
}
static void chart(mock::Host& H, const std::vector<Ev>& v, long long k0, int n, double tick) {
    H.bars.clear();
    for (int i = 0; i < n; ++i) {
        const long long key = k0 + 180LL * i; mock::Bar b; b.time = (unsigned long)key; std::map<long long, std::pair<long, long> > rows; long long hi = 0, lo = 0;
        for (const Ev& e : v) if (e.act == 3 && e.bs != 2 && e.sec >= key - 180 && e.sec < key) { (e.bs == 1 ? rows[e.px].first : rows[e.px].second) += (long)e.sz; if (!hi || e.px > hi) hi = e.px; if (!lo || e.px < lo) lo = e.px; }
        if (!hi) { hi = lo = 28800; rows[28800] = std::make_pair(1L, 1L); }
        b.high = (float)(hi * tick); b.low = (float)(lo * tick); b.open = b.low; b.close = b.high; b.volume = 0;
        for (const auto& r : rows) { b.rows.push_back(mock::Price((float)(r.first * tick), r.second.first, r.second.second, r.second.first + r.second.second)); b.volume += (unsigned long)(r.second.first + r.second.second); }
        H.bars.push_back(b);
    }
    H.vis0 = 0; H.vis1 = n - 1;
}
static std::string stripText(const mock::Host& H) {
    for (const mock::Draw& d : H.draws) if (d.type == "text" && (d.text.find("ABSORPTION") != std::string::npos || d.text.find("sweep") != std::string::npos)) return d.text;
    return std::string();
}
static std::string pillText(const mock::Host& H) {
    for (const mock::Draw& d : H.draws) if (d.type == "text" && (d.text.find("turn") != std::string::npos || d.text == "late" || d.text == "\x96")) return d.text;
    return std::string();
}
// one IRT cycle on host H: calc, the feed clock (if running), draw
static void cycle(cppExtension* x, mock::Host& H) {
    mock::current = &H; H.draws.clear();
    x->calc(0);
    for (const auto& t : std::map<int, int>(H.timers)) { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_TIMER_MOCK; e.v.timer.id = t.first; x->timer(&e); }
    x->draw();
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* tmp = std::getenv("TMPDIR"); root = std::string(tmp ? tmp : "/tmp") + "/erp_host_u"; setenv("USERPROFILE", root.c_str(), 1);
    std::remove(P("EffortReward\\er-ES-180.txt").c_str()); std::remove(P("EffortReward\\er-GC-180.txt").c_str());
    std::remove(P("EffortReward\\_opening-ES-180.txt").c_str()); std::remove(P("EffortReward\\_opening-GC-180.txt").c_str());
    const long long K0 = 1795164660LL;                                 // 8:51 CT (bar end)
    const long long K903 = 1795165380LL;
    put(P("LRA-EffortReward-ES.csv"), paramsText("ES", 244, 69, 2.8));
    put(P("LRA-EffortReward-GC.csv"), paramsText("GC", 24.5, 8, 2.0));
    put(P("RevRead\\wy-ES-180.txt"), "WY|1|ES|180|0.21.7\nL|28797|ONH\n1795164660|Sw|-1|LHI|28733|R99+\n1795165380|Sw|-1|ONH|28797|R99+\n");
    put(P("RevRead\\wy-GC-180.txt"), "WY|1|GC|180|0.21.7\n1795165380|Sw|-1|PhrH|35824|R99+\n");
    const std::vector<Ev> es = load("ev_ES.txt", K0 - 540, K903 + 200), gc = load("ev_GC.txt", K0 - 540, K903 + 200);
    CHECK(es.size() > 100000 && gc.size() > 10000);
    mock::Feed fES, fGC; toFeed(es, 0.25, fES); toFeed(gc, 0.1, fGC);
    mock::Host hES, hGC; hES.feedp = &fES; hGC.feedp = &fGC; hES.tick = 0.25f; hGC.tick = 0.1f; hES.seconds = hGC.seconds = 180;
    hES.symbol = "ESZ6"; hES.root = "ES"; hGC.symbol = "GCZ6"; hGC.root = "GC"; hES.barW = hGC.barW = 60; hES.barX0 = hGC.barX0 = 900; hES.paneTop = hGC.paneTop = 0; hES.paneBottom = hGC.paneBottom = 300;
    hES.paneRight = hGC.paneRight = 1400; hES.textPx = hGC.textPx = 0.6;
    chart(hES, es, K0, 6, 0.25); chart(hGC, gc, K0, 6, 0.1);           // bars 8:51 .. 9:06 (9:06 forming)
    cppExtension* x = CreateExtension();
    // the two charts interleaved, one DLL object
    for (int i = 0; i < 400 && (fES.at < fES.ev.size() || fGC.at < fGC.ev.size()); ++i) { cycle(x, hES); cycle(x, hGC); }
    CHECK(fES.at == fES.ev.size() && fGC.at == fGC.ev.size());
    CHECK(fES.opens == 1 && fGC.opens == 1 && fES.lastTicker == "ESZ6" && fGC.lastTicker == "GCZ6");
    CHECK(!hES.timers.empty() && !hGC.timers.empty() && hES.timers.begin()->first != hGC.timers.begin()->first);
    for (int i = 0; i < 3; ++i) { cycle(x, hES); cycle(x, hGC); }
    const std::string recES = get(P("EffortReward\\er-ES-180.txt")), recGC = get(P("EffortReward\\er-GC-180.txt"));
    std::cout << recES << recGC;
    CHECK(recES.find("R|1795165380|-1|ONH|28797|441|5934|25|") != std::string::npos);
    CHECK(recES.find("R|1795164660|-1|LonHI|28733|12|49|0|") != std::string::npos);
    CHECK(recES.find("|1|1|1|3|0|L|") != std::string::npos);           // ES 9:03: F H B = 3
    CHECK(recGC.find("R|1795165380|-1|PhrH|35824|42|602|0|") != std::string::npos);
    const std::string sES = stripText(hES), sGC = stripText(hGC);
    std::cout << "ES strip: " << sES << " | " << pillText(hES) << "\nGC strip: " << sGC << " | " << pillText(hGC) << "\n";
    CHECK(sES == "HEAVY ABSORPTION 1.8x \x96 441 of 5,934 buy lots above ONH got no further (25 hidden)");
    CHECK(pillText(hES) == "67% turn");
    CHECK(sGC.find("HEAVY ABSORPTION 1.7x \x96 42 of 602 buy lots above PhrH got no further") == 0);
    // the bars: a ratio label drawn on the 9:03 candle (green and red)
    int labels = 0; for (const mock::Draw& d : hES.draws) if (d.type == "text" && d.text.size() >= 4 && d.text.back() == 'x' && d.text.find('.') != std::string::npos) ++labels;
    CHECK(labels >= 8);
    if (const char* dump = std::getenv("ERP_DUMP")) {                  // the render check: every draw of the ES pane
        std::ofstream o(dump); for (const mock::Draw& d : hES.draws) o << d.type << "|" << d.l << "|" << d.t << "|" << d.r << "|" << d.b << "|" << std::hex << d.color << "|" << d.border << std::dec << "|" << d.fsize << "|" << d.fstyle << "|" << d.centred << "|" << d.text << "\n"; }
    // overlap check: no two text labels overlap, every drawing inside the pane
    { std::vector<const mock::Draw*> tx; for (const mock::Draw& d : hES.draws) { CHECK(d.t >= hES.paneTop && d.b <= hES.paneBottom); if (d.type == "text") tx.push_back(&d); }
      for (size_t i = 0; i < tx.size(); ++i) for (size_t j = i + 1; j < tx.size(); ++j) { const mock::Draw& a = *tx[i]; const mock::Draw& b = *tx[j];
          const bool ov = a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; if (ov) std::printf("overlap: '%s' / '%s'\n", a.text.c_str(), b.text.c_str()); CHECK(!ov); } }
    const std::string stES = get(P("EffortReward\\status-ES-180.txt")); std::cout << stES;
    CHECK(stES.find("COVER|last 100%") != std::string::npos);
    CHECK(stES.find("VERSION|0.2.0") == 0 && stES.find("PARAMS|ok") != std::string::npos && stES.find("SWEEPS|ok|2") != std::string::npos);
    // RESTART: the chart is removed and added again; no feed events this time -> the recorded read is shown unchanged (no repaint)
    mock::current = &hES; x->destroy(); CHECK(fES.live == 0); CHECK(hES.data == nullptr); CHECK(hES.timers.empty());
    mock::current = &hGC; x->destroy(); CHECK(fGC.live == 0);
    mock::Feed empty; hES.feedp = &empty; for (int i = 0; i < 3; ++i) cycle(x, hES);
    CHECK(stripText(hES) == sES && pillText(hES) == "67% turn");
    CHECK(get(P("EffortReward\\er-ES-180.txt")) == recES);             // nothing re-decided / rewritten
    // a newer Sw without live data and without a nightly row: "absorption -" and no pill
    put(P("RevRead\\wy-ES-180.txt"), "WY|1|ES|180|0.21.7\n1795165380|Sw|-1|ONH|28797|R99+\n1795165560|Sw|1|PDL|28768|\n");
    chart(hES, es, K0, 7, 0.25); hES.vis1 = 6; for (int i = 0; i < 3; ++i) { hES.now += 6; cycle(x, hES); }
    std::cout << "new Sw strip: " << stripText(hES) << "\n";
    CHECK(stripText(hES) == "PDL sweep \xB7 absorption \x96" && pillText(hES) == "\x96");
    // garbled params and a foreign sweep file: defaults + reasons, the last good sweep list kept
    put(P("LRA-EffortReward-ES.csv"), "VERSION,1\nMARKET,ES\nABS_MED,abc\n");
    put(P("RevRead\\wy-ES-180.txt"), "WY|1|NQ|180|0.21.7\n");
    for (int i = 0; i < 2; ++i) cycle(x, hES);
    { const std::string st = get(P("EffortReward\\status-ES-180.txt")); CHECK(st.find("PARAMS|params line 3 garbled (defaults)") != std::string::npos); CHECK(st.find("SWEEPS|sweep file is NQ|2") != std::string::npos); }
    CHECK(stripText(hES).find("PDL sweep") == 0);                     // the last good sweep list is kept
    // symbol change on the ES chart: the feed is closed (unsubscribed) and the state starts clean
    hES.feedp = &fES; fES.at = 0; hES.symbol = "NQZ6"; hES.root = "NQ"; cycle(x, hES);
    CHECK(fES.live == 0 || fES.live == 1);
    // backlog cap with no timer: one calc reads at most SLICE_MAX events
    mock::Host hB = hGC; hB.data = nullptr; hB.timers.clear(); hB.timerRefuse = true; mock::Feed fB; toFeed(gc, 0.1, fB); hB.feedp = &fB;
    mock::current = &hB; x->calc(0); x->calc(0); const size_t at1 = fB.at; x->calc(0);
    CHECK(fB.at - at1 <= (size_t)erh::SLICE_MAX && fB.at > at1);
    mock::current = &hB; x->destroy(); CHECK(fB.live == 0);
    // a faulting feed read stops the feed, never crashes
    mock::Host hF = hGC; hF.data = nullptr; hF.timers.clear(); mock::Feed fF; toFeed(gc, 0.1, fF); fF.throwOnNext = true; hF.feedp = &fF;
    for (int i = 0; i < 3; ++i) cycle(x, hF);
    mock::current = &hF; x->destroy(); CHECK(fF.live == 0);
    mock::current = &hES; x->destroy(); CHECK(fES.live == 0);
    { EffortRewardPane* px = static_cast<EffortRewardPane*>(x);   // IRT owns the extension object; the per-chart states were freed by destroy()
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
      delete px;
#pragma GCC diagnostic pop
    }
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
