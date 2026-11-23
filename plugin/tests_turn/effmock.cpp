// effmock.cpp -- the mockup's picture: the WHOLE plugin (Turn.cpp, the version on the include path) on the mock SDK over Friday's real ES
// order events 8:20 - 10:33 AM CT (2026-11-20), candle by candle as a live morning; at the end the draw list of the pane at his zoom
// (22 px a candle) and the candles' prices. usage: effmock <ev file> <hostdata dir> <out prefix> [barW] [paneH] [mouseBar mouseAbove]
#define TURN_FILE_CHECK_MS 0
#define TURN_STATUS_MS 0
#define TURN_DECIDE_WAIT_MS 0
#define TURN_NOSUM_WAIT_MS 0
#define E_TIMER_MOCK 0
#define PC_TEST_CURSOR 1
#define TURN_TEST_WINDOW 1
#include "irtsdk.h"
namespace mock { Feed feed; Host* current = nullptr; }
#include "Turn.cpp"
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
void* turn_test_window = nullptr;
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
static std::string root;
static std::string P(const std::string& rel) { return root + "\\InvestorRT\\rtx\\lsFlexLevels\\" + rel; }
static void put(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
static std::string get(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
struct Ev { long long sec, act, bs, px, sz, oid, aid; };
struct CandleVap { std::map<long long, std::pair<long, long> > rows; long long hi = 0, lo = 0, open = 0, close = 0; };
struct Sim {
    std::vector<Ev> ev; size_t next = 0; std::map<long long, CandleVap> vap; double tick = 1; long long k0 = 0;
    mock::Host H; mock::Feed F;
    void load(const std::string& f, long long a, long long b) {
        std::ifstream in(f.c_str()); Ev e;
        while (in >> e.sec >> e.act >> e.bs >> e.px >> e.sz >> e.oid >> e.aid) { if (e.sec >= b) break; if (e.sec < a) continue; ev.push_back(e);
            if (e.act == 3 && e.bs != 2) { CandleVap& c = vap[(e.sec / 180 + 1) * 180]; (e.bs == 1 ? c.rows[e.px].first : c.rows[e.px].second) += (long)e.sz; if (!c.hi || e.px > c.hi) c.hi = e.px; if (!c.lo || e.px < c.lo) c.lo = e.px; if (!c.open) c.open = e.px; c.close = e.px; } }
    }
    void chartTo(long long forming) {
        H.bars.clear();
        for (long long key = k0; key <= forming; key += 180) {
            mock::Bar b; b.time = (unsigned long)key; const CandleVap& c = vap[key];
            const long long hi = c.hi ? c.hi : 28800, lo = c.lo ? c.lo : 28800;
            b.high = (float)(hi * tick); b.low = (float)(lo * tick); b.open = (float)((c.open ? c.open : lo) * tick); b.close = (float)((c.close ? c.close : hi) * tick); b.volume = 0;
            for (const auto& r : c.rows) { b.rows.push_back(mock::Price((float)(r.first * tick), r.second.first, r.second.second, r.second.first + r.second.second)); b.volume += (unsigned long)(r.second.first + r.second.second); }
            if (b.rows.empty()) { b.rows.push_back(mock::Price((float)(hi * tick), 1, 1, 2)); b.volume = 2; }
            H.bars.push_back(b);
        }
        H.vis0 = 0; H.vis1 = (int)H.bars.size() - 1;
    }
    bool feedTo(long long before) {
        if (F.at >= F.ev.size()) { F.ev.clear(); F.at = 0; }
        bool any = false;
        for (; next < ev.size() && ev[next].sec < before; ++next) { const Ev& e = ev[next]; DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.timestamp = (RTDATE)e.sec; o.price = (float)(e.px * tick); o.size = (unsigned long)e.sz;
            o.orderID = (int)(e.oid & 0x7FFFFFFF); o.aggressorOrderID = (int)(e.aid & 0x7FFFFFFF); o.action = (unsigned char)e.act; o.buySell = (unsigned char)e.bs; F.ev.push_back(o); any = true; }
        return any;
    }
};
static std::string dumpDraws(const mock::Host& H) {
    std::ostringstream o; for (const mock::Draw& d : H.draws) o << d.type << "|" << d.l << "|" << d.t << "|" << d.r << "|" << d.b << "|" << std::hex << d.color << "|" << d.border << std::dec << "|" << d.fsize << "|" << d.fstyle << "|" << d.centred << "|" << d.text << "\n"; return o.str();
}
static void cycle(cppExtension* x, mock::Host& H) {
    mock::current = &H; H.draws.clear();
    x->calc(0);
    for (const auto& t : std::map<int, int>(H.timers)) { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_TIMER_MOCK; e.v.timer.id = t.first; x->timer(&e); }
    x->draw();
}
static void drain(cppExtension* x, Sim& a) {
    for (int i = 0; i < 8000 && a.F.at < a.F.ev.size(); ++i) cycle(x, a.H);
    for (int i = 0; i < 3; ++i) cycle(x, a.H);
}
int main(int argc, char** argv) {
    if (argc < 4) { std::printf("usage\n"); return 2; }
    const std::string evf = argv[1], data = argv[2], out = argv[3];
    const int barW = argc > 4 ? std::atoi(argv[4]) : 22, paneH = argc > 5 ? std::atoi(argv[5]) : 170;
    const char* tmp = std::getenv("TMPDIR"); root = std::string(tmp ? tmp : "/tmp") + "/effmock_u_" + std::to_string((long long)getpid()); setenv("USERPROFILE", root.c_str(), 1);
    { const std::string mk = "mkdir -p '" + root + "'"; if (std::system(mk.c_str())) return 3; }
    const long long K824 = 1795163040LL, KEND = 1795170780LL;                // bar ends 8:24 AM and 10:33 AM CT (the mock's clock)
    Sim es; es.tick = 0.25; es.k0 = K824;
    es.load(evf, K824 - 240, KEND);
    es.H.feedp = &es.F; es.H.tick = 0.25f; es.H.seconds = 180; es.H.barW = barW; es.H.barX0 = 20 + barW / 2; es.H.paneTop = 0; es.H.paneBottom = paneH; es.H.paneRight = 20 + barW * 46 + 40; es.H.textPx = 0.6;
    es.H.symbol = "ESZ6"; es.H.root = "ES";
    cppExtension* x = CreateExtension();
    es.chartTo(K824); cycle(x, es.H);                                    // (the files are made by the first cycle's directory: write them, then go)
    put(P("LRA-TurnPane-ES.csv"), get(data + "/LRA-Turn-ES.csv")); put(P("RevRead\\wy-ES-180.txt"), get(data + "/wy-ES.txt"));
    for (long long forming = K824; forming <= KEND - 180; forming += 180) { es.chartTo(forming); es.feedTo(forming); drain(x, es); }
    es.chartTo(KEND); es.feedTo(KEND - 150); drain(x, es);                 // the 10:33 candle forms: 10:30 is closed
    for (int i = 0; i < 6; ++i) cycle(x, es.H);
    put(out + "_draws.txt", dumpDraws(es.H));
    if (argc > 7) {                                                       // the mouse on a candle
        const long bar = std::atol(argv[6]); const bool above = std::atoi(argv[7]) != 0;
        mock::current = &es.H; turn_test_window = (void*)&es.H; pc::test_ok = true;
        const int h = es.H.barX0 + (int)bar * barW, v = above ? paneH / 2 - 20 : paneH / 2 + 30; pc::test_cx = h + 100; pc::test_cy = v + 50;
        RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)h; e.v.mouse.v = (short)v; e.v.mouse.bar = bar; x->mouse(&e);
        cycle(x, es.H); cycle(x, es.H);
        put(out + "_mouse.txt", dumpDraws(es.H));
    }
    if (std::getenv("EFF_RESTART")) {                                    // the chart is removed and added again, no order events: the same picture (no repaint)
        turn_test_window = nullptr; for (int i = 0; i < 3; ++i) cycle(x, es.H); const std::string before = dumpDraws(es.H), rec0 = get(P("TurnPane\\turn-ES-180.txt"));
        mock::current = &es.H; x->destroy(); mock::Feed empty; es.H.feedp = &empty; for (int i = 0; i < 4; ++i) cycle(x, es.H);
        std::printf("restart: %s, record %s\n", dumpDraws(es.H) == before ? "the same picture" : "DIFFERENT PICTURE", get(P("TurnPane\\turn-ES-180.txt")) == rec0 ? "unchanged" : "CHANGED");
    }
    if (const char* pf = std::getenv("EFF_PERF")) {                       // draw cost: N candles in view (the morning's candles repeated), px a candle
        const int n = std::atoi(pf); mock::Host h = es.H; h.data = nullptr; h.timers.clear(); mock::Feed f; h.feedp = &f; h.symbol = "ESH7"; const std::vector<mock::Bar> src = es.H.bars; h.bars.clear();
        for (int i = 0; i < n; ++i) { mock::Bar b = src[(size_t)i % (src.size() - 1)]; b.time = (unsigned long)(K824 + 180LL * i); h.bars.push_back(b); }
        h.vis0 = 0; h.vis1 = n - 1; h.paneRight = 20 + barW * (n + 2) + 40;
        for (int i = 0; i < 12; ++i) cycle(x, h);                             // every candle's volume at price read (400 a pass)
        const std::clock_t t0 = std::clock(); const int reps = 200; size_t draws = 0;
        for (int i = 0; i < reps; ++i) { mock::current = &h; h.draws.clear(); x->draw(); draws = h.draws.size(); }
        std::printf("perf: %d candles at %d px: %.3f ms a draw, %zu draw calls\n", n, barW, 1000.0 * (double)(std::clock() - t0) / CLOCKS_PER_SEC / reps, draws);
        mock::current = &h; x->destroy();
    }
    std::ostringstream c;
    for (const mock::Bar& b : es.H.bars) c << b.time << "," << b.open << "," << b.high << "," << b.low << "," << b.close << "," << b.volume << "\n";
    put(out + "_candles.csv", c.str());
    put(out + "_status.txt", get(P("TurnPane\\status-ES-180.txt")));
    put(out + "_record.txt", get(P("TurnPane\\turn-ES-180.txt")));
    mock::current = &es.H; x->done(); delete x;
    std::printf("ok %zu candles, %zu draws\n", es.H.bars.size(), es.H.draws.size());
    return 0;
}
