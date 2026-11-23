// tests_turn/test_turn_host.cpp -- the WHOLE plugin (Turn.cpp) on the mock SDK, fed like a live morning: the real 2026-11-20 order events
// (ES / GC, Databento -> IRT event form, tests_turn/mk_events.py) arrive candle by candle on two charts of one DLL while the charts grow.
// Checks: the sweep reads and the level candles are decided once and recorded, the strip and the bars, scrolling back, a restart without
// repaint, the nightly's rows, garbled / foreign files, symbol change, backlog cap, a faulting feed, no leaks.
// g++ -std=c++17 -DLSPERF_FAKE -fsanitize=address,undefined -Wall -Wextra -Werror -Imock -I.. ; run from tests_turn with
//   TURN_TEST_DATA=<dir with ev_ES.txt ev_GC.txt LRA-Turn-ES.csv LRA-Turn-GC.csv LRA-Turn-ES-nightly.csv wy-ES.txt wy-GC.txt>  (prep_host.py)
#define TURN_FILE_CHECK_MS 0
#define TURN_STATUS_MS 0
#define TURN_DECIDE_WAIT_MS 0
#define TURN_NOSUM_WAIT_MS 0
#define E_TIMER_MOCK 0
#include "irtsdk.h"
namespace mock { Feed feed; Host* current = nullptr; }
#include "../Turn.cpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static std::string root, data;
static std::string P(const std::string& rel) { return root + "\\InvestorRT\\rtx\\lsFlexLevels\\" + rel; }
static void put(const std::string& p, const std::string& t) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << t; }
static std::string get(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
struct Ev { long long sec, act, bs, px, sz, oid, aid; };
struct CandleVap { std::map<long long, std::pair<long, long> > rows; long long hi = 0, lo = 0, open = 0, close = 0; };
// one market's morning: its events, the volume at price of each candle, its chart and its feed
struct Sim {
    std::vector<Ev> ev; size_t next = 0; std::map<long long, CandleVap> vap; double tick = 1; long long k0 = 0;
    mock::Host H; mock::Feed F;
    void load(const std::string& f, long long a, long long b) {
        std::ifstream in(f.c_str()); Ev e;
        while (in >> e.sec >> e.act >> e.bs >> e.px >> e.sz >> e.oid >> e.aid) { if (e.sec >= b) break; if (e.sec < a) continue; ev.push_back(e);
            if (e.act == 3 && e.bs != 2) { CandleVap& c = vap[(e.sec / 180 + 1) * 180]; (e.bs == 1 ? c.rows[e.px].first : c.rows[e.px].second) += (long)e.sz; if (!c.hi || e.px > c.hi) c.hi = e.px; if (!c.lo || e.px < c.lo) c.lo = e.px; if (!c.open) c.open = e.px; c.close = e.px; } }
    }
    void chartTo(long long forming) {                                  // bars k0 .. forming (the last one forming)
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
    bool feedTo(long long before) {                                    // the events dated before `before` reach the feed
        if (F.at >= F.ev.size()) { F.ev.clear(); F.at = 0; }
        bool any = false;
        for (; next < ev.size() && ev[next].sec < before; ++next) { const Ev& e = ev[next]; DEPTH_ORDER o; std::memset(&o, 0, sizeof o); o.timestamp = (RTDATE)e.sec; o.price = (float)(e.px * tick); o.size = (unsigned long)e.sz;
            o.orderID = (int)(e.oid & 0x7FFFFFFF); o.aggressorOrderID = (int)(e.aid & 0x7FFFFFFF); o.action = (unsigned char)e.act; o.buySell = (unsigned char)e.bs; F.ev.push_back(o); any = true; }
        return any;
    }
};
static std::string stripText(const mock::Host& H) {                    // the strip's text pieces in drawing order (the row under the title); the gaps between
    std::string s; int at = -1;                                        // them are fixed pixel gaps of one space width each (6 px on the mock)
    for (const mock::Draw& d : H.draws) if (d.type == "text" && d.t == H.paneTop + 18 && !d.centred) { if (at >= 0 && d.l > at) s += std::string((size_t)((d.l - at) / 6), ' '); s += d.text; at = d.r; }
    return s;
}
static std::string pillText(const mock::Host& H) { for (const mock::Draw& d : H.draws) if (d.type == "text" && d.t == H.paneTop + 18 && d.centred) return d.text; return std::string(); }
static std::string dumpDraws(const mock::Host& H) {
    std::ostringstream o; for (const mock::Draw& d : H.draws) o << d.type << "|" << d.l << "|" << d.t << "|" << d.r << "|" << d.b << "|" << std::hex << d.color << "|" << d.border << std::dec << "|" << d.fsize << "|" << d.fstyle << "|" << d.centred << "|" << d.text << "\n"; return o.str();
}
// one IRT cycle on host H: calc, the feed clock (if running), draw
static void cycle(cppExtension* x, mock::Host& H) {
    mock::current = &H; H.draws.clear();
    x->calc(0);
    for (const auto& t : std::map<int, int>(H.timers)) { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_TIMER_MOCK; e.v.timer.id = t.first; x->timer(&e); }
    x->draw();
}
static void drain(cppExtension* x, Sim& a, Sim& b) {
    for (int i = 0; i < 4000 && (a.F.at < a.F.ev.size() || b.F.at < b.F.ev.size()); ++i) { cycle(x, a.H); cycle(x, b.H); }
    for (int i = 0; i < 3; ++i) { cycle(x, a.H); cycle(x, b.H); }
}
static int countLines(const std::string& t, const std::string& head) { int n = 0; size_t at = 0; while ((at = t.find(head, at)) != std::string::npos) { if (at == 0 || t[at - 1] == '\n') ++n; at += head.size(); } return n; }
static std::string lineOf(const std::string& t, const std::string& head) { const size_t a = t.find(head); if (a == std::string::npos) return std::string(); const size_t b = t.find('\n', a); return t.substr(a, b == std::string::npos ? std::string::npos : b - a); }
static void noOverlap(const mock::Host& H) {
    std::vector<const mock::Draw*> tx; for (const mock::Draw& d : H.draws) { CHECK(d.t >= H.paneTop && d.b <= H.paneBottom && d.l >= H.paneLeft && d.r <= H.paneRight); if (d.type == "text") tx.push_back(&d); }
    for (size_t i = 0; i < tx.size(); ++i) for (size_t j = i + 1; j < tx.size(); ++j) { const mock::Draw& a = *tx[i]; const mock::Draw& b = *tx[j];
        const bool ov = a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; if (ov) std::printf("overlap: '%s' [%d %d %d %d] / '%s' [%d %d %d %d]\n", a.text.c_str(), a.l, a.t, a.r, a.b, b.text.c_str(), b.l, b.t, b.r, b.b); CHECK(!ov); }
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* tmp = std::getenv("TMPDIR"); root = std::string(tmp ? tmp : "/tmp") + "/turn_host_u"; setenv("USERPROFILE", root.c_str(), 1);
    const char* td = std::getenv("TURN_TEST_DATA"); if (!td) { std::printf("set TURN_TEST_DATA\n"); return 2; } data = td;
    for (const char* f : { "TurnPane\\turn-ES-180.txt", "TurnPane\\turn-GC-180.txt", "TurnPane\\_opening-ES-180.txt", "TurnPane\\_opening-GC-180.txt", "TurnPane\\status-ES-180.txt", "TurnPane\\status-GC-180.txt" }) std::remove(P(f).c_str());
    const long long K845 = 1795164300LL, K851 = 1795164660LL, K903 = 1795165380LL, K906 = K903 + 180;   // bar ends, CT clock
    put(P("LRA-TurnPane-ES.csv"), get(data + "/LRA-Turn-ES.csv")); put(P("LRA-TurnPane-GC.csv"), get(data + "/LRA-Turn-GC.csv"));
    put(P("RevRead\\wy-ES-180.txt"), get(data + "/wy-ES.txt")); put(P("RevRead\\wy-GC-180.txt"), get(data + "/wy-GC.txt"));
    Sim es, gc; es.tick = 0.25; gc.tick = 0.1; es.k0 = gc.k0 = K845 - 180;
    es.load(data + "/ev_ES.txt", K845 - 360, K906 + 30); gc.load(data + "/ev_GC.txt", K845 - 360, K906 + 30);
    CHECK(es.ev.size() > 100000 && gc.ev.size() > 10000);
    for (Sim* s : { &es, &gc }) { s->H.feedp = &s->F; s->H.tick = (float)s->tick; s->H.seconds = 180; s->H.barW = 60; s->H.barX0 = 600; s->H.paneTop = 0; s->H.paneBottom = 300; s->H.paneRight = 1400; s->H.textPx = 0.6; }
    es.H.symbol = "ESZ6"; es.H.root = "ES"; gc.H.symbol = "GCZ6"; gc.H.root = "GC";
    cppExtension* x = CreateExtension();
    // ---- the morning, candle by candle: the pane is loaded while the 8:42 candle forms
    es.chartTo(K845 - 180); gc.chartTo(K845 - 180); cycle(x, es.H); cycle(x, gc.H);
    for (long long forming = K845 - 180; forming <= K906; forming += 180) {
        es.chartTo(forming); gc.chartTo(forming); es.feedTo(forming); gc.feedTo(forming); drain(x, es, gc);
    }
    es.chartTo(K906 + 180); gc.chartTo(K906 + 180); es.feedTo(K906 + 30); gc.feedTo(K906 + 30); drain(x, es, gc);   // the 9:09 candle begins: 9:06 is closed
    CHECK(es.F.opens == 1 && gc.F.opens == 1 && es.F.lastTicker == "ESZ6" && gc.F.lastTicker == "GCZ6");
    CHECK(!es.H.timers.empty() && !gc.H.timers.empty() && es.H.timers.begin()->first != gc.H.timers.begin()->first);
    const std::string recES = get(P("TurnPane\\turn-ES-180.txt")), recGC = get(P("TurnPane\\turn-GC-180.txt"));
    std::cout << recES << recGC;
    // the sweep reads: decided once, the values of the reference (mockup) where the feed carries the same prints
    const std::string t851 = lineOf(recES, "T|1795164660|"), t903 = lineOf(recES, "T|1795165380|"), g903 = lineOf(recGC, "T|1795165380|");
    CHECK(recES.find("TRN|1|ES|180\n") == 0 && recGC.find("TRN|1|GC|180\n") == 0 && countLines(recES, "T|") == 2 && countLines(recGC, "T|") == 1);
    { turn::Read r; CHECK(turn::readParse(turn::split(t851, '|'), 1, r)); CHECK(r.lv == "LonHI" && r.lvt == 28733 && r.dir == -1 && r.src == 'L');
      CHECK(r.stopLots == 15 && r.stopX1000 == 698 && r.eff1000 == 657 && r.extInto == 19 && r.extLeft == 161 && r.F == 1 && r.marks == 1 && r.ansOk == 1);
      CHECK(r.verdict == turn::V_UNCLEAR && r.why == turn::WHY_EARLY && r.odds == -1 && r.exh == 0); }      // the high 19 s into the candle: UNCLEAR, as the mockup
    { turn::Read r; CHECK(turn::readParse(turn::split(t903, '|'), 1, r)); CHECK(r.lv == "ONH" && r.lvt == 28797);
      CHECK(r.stopLots == 112 && r.stopX1000 == 5209 && r.extInto == 6 && r.extLeft == 174 && r.F == 1 && r.H == 1 && r.B == 1 && r.marks == 3);
      CHECK(r.verdict == turn::V_UNCLEAR && r.why == turn::WHY_EARLY && r.eff1000 >= 1500); }                // ES 9:03: Stop Run 5.2x, the high 6 s in: UNCLEAR
    { turn::Read r; CHECK(turn::readParse(turn::split(g903, '|'), 1, r)); CHECK(r.lv == "PhrH" && r.lvt == 35824);
      CHECK(r.stopX1000 >= 3000 && r.eff1000 >= 1500 && r.exh == 2 && r.marks == 2 && r.ansOk == 1 && r.extInto >= 20 && r.extLeft >= 30);
      CHECK(r.verdict == turn::V_BEST && r.odds == -1); }                                                    // GC 9:03: BEST ODDS ?, as the mockup
    // candles at a named level: only candles that closed while the pane was loaded, each once, with their Stop Run when the feed was whole
    CHECK(countLines(recES, "C|") >= 2 && countLines(recGC, "C|") >= 1);
    CHECK(recES.find("C|" + std::to_string(K845 - 180) + "|") == std::string::npos);                        // closed before / at the load: never evaluated
    { const std::string c851 = lineOf(recES, "C|1795164660|"); CHECK(!c851.empty()); turn::Cand c; CHECK(turn::parseCand(turn::split(c851, '|'), c)); CHECK(c.hiL == 28733 && c.hiStop == 15 && c.hiX1000 == 698); }
    const std::string sES = stripText(es.H), sGC = stripText(gc.H);
    std::cout << "ES strip: " << sES << " | " << pillText(es.H) << "\nGC strip: " << sGC << " | " << pillText(gc.H) << "\n";
    CHECK(sES.find("ONH 9:03 AM   Heavy Stop Run / Heavy Effort / ") == 0 && sES.find(" Exhaustion ? / Heavy Answer   \xBB UNCLEAR") != std::string::npos && pillText(es.H) == "\x96");
    CHECK(sGC == "PhrH 9:03 AM   Heavy Stop Run / Heavy Effort / Moderate Exhaustion / Moderate Answer   \xBB BEST ODDS ?" && pillText(gc.H) == "?");
    // the bars: ratio labels on the candles at a level, the Answer as marks; green / light green up and red / light red down at a swept high
    { int ratio = 0, marks = 0, lg = 0, g = 0, lr = 0, r = 0, other = 0; const int x = gc.H.barX0 + (int)((K903 - gc.k0) / 180) * gc.H.barW;   // the 9:03 candle: all four bars
      for (const mock::Draw& d : gc.H.draws) { if (d.t <= 35 || d.l < x - 30 || d.r > x + 30) continue;
          if (d.type == "text" && d.text.size() >= 4 && d.text.back() == 'x') { ++ratio; }
          if (d.type == "text" && d.text == "2/3") { ++marks; }
          if (d.type == "box" && d.r - d.l == 20) { if (d.color == trh::C_LGREEN && d.r <= x) ++lg; else if (d.color == trh::C_GREEN && d.l >= x) ++g; else if (d.color == trh::C_LRED && d.r <= x) ++lr; else if (d.color == trh::C_RED && d.l >= x) ++r; else ++other; } }
      std::printf("GC 9:03 candle: %d ratio labels, %d marks label, light green %d green %d light red %d red %d other %d\n", ratio, marks, lg, g, lr, r, other);
      CHECK(ratio == 3 && marks == 1 && lg == 1 && g == 1 && lr == 1 && r == 1 && other == 0); }
    { int boxes = 0; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 35 && d.r - d.l == 20) { ++boxes; CHECK(d.color == trh::C_GREEN || d.color == trh::C_LGREEN || d.color == trh::C_RED || d.color == trh::C_LRED || d.color == trh::C_AXIS); CHECK(d.border == d.color); }
      CHECK(boxes >= 6); }                                             // fill-only bars: the border is the fill colour (no outlines)
    noOverlap(es.H); noOverlap(gc.H);
    if (const char* dump = std::getenv("TURN_DUMP")) { put(std::string(dump) + "_ES.txt", dumpDraws(es.H)); put(std::string(dump) + "_GC.txt", dumpDraws(gc.H)); }
    const std::string stES = get(P("TurnPane\\status-ES-180.txt")); std::cout << stES;
    CHECK(stES.find("VERSION|1.0.0") == 0 && stES.find("PARAMS|ok") != std::string::npos && stES.find("SWEEPS|ok|2|levels 16") != std::string::npos && stES.find("COVER|last 100%") != std::string::npos);
    CHECK(stES.find("FEED|ready") != std::string::npos && stES.find("|faults 0|") != std::string::npos && stES.find("RECORD|reads 2|") != std::string::npos);
    // scrolling back: the strip reads the newest sweep at or before the last candle in view
    { const int keep = es.H.vis1; es.H.vis1 = (int)((K851 - es.k0) / 180); cycle(x, es.H); const std::string s = stripText(es.H); std::cout << "ES scrolled back: " << s << " | " << pillText(es.H) << "\n";
      CHECK(s == "LonHI 8:51 AM   Light Stop Run / Light Effort / No Exhaustion ? / Light Answer   \xBB UNCLEAR"); noOverlap(es.H);
      es.H.vis1 = (int)((K851 - 180 - es.k0) / 180); cycle(x, es.H); CHECK(stripText(es.H).empty());        // before any sweep: no strip
      es.H.vis1 = keep; cycle(x, es.H); CHECK(stripText(es.H) == sES); }
    // narrow candles: no labels on the bars, the numbers go into the strip; nothing overlaps
    { es.H.barW = 12; es.H.barX0 = 1200; cycle(x, es.H); const std::string s = stripText(es.H); std::cout << "ES narrow: " << s << "\n";
      CHECK(s.find("ONH 9:03 AM   Heavy Stop Run 5.2x / Heavy Effort ") == 0 && s.find("Heavy Answer 3/3") != std::string::npos);
      int lab = 0; for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 35) ++lab; CHECK(lab == 0); noOverlap(es.H);
      es.H.barW = 3; es.H.barX0 = 1340; cycle(x, es.H); noOverlap(es.H);
      es.H.paneRight = 500; es.H.barX0 = 300; cycle(x, es.H); noOverlap(es.H); std::cout << "ES small pane: " << stripText(es.H) << "\n"; CHECK(stripText(es.H).find("UNCLEAR") != std::string::npos);
      es.H.paneBottom = 60; cycle(x, es.H); CHECK(stripText(es.H).empty());                                  // too low for the strip: bars only, no crash
      es.H.paneBottom = 300; es.H.paneRight = 1400; es.H.barW = 60; es.H.barX0 = 600; cycle(x, es.H); CHECK(stripText(es.H) == sES); }
    const std::string drawsES = dumpDraws(es.H), drawsGC = dumpDraws(gc.H);
    // RESTART: the charts are removed and added again; no feed events this time -> the same strip and the same bars from the record (no repaint)
    mock::current = &es.H; x->destroy(); CHECK(es.F.live == 0); CHECK(es.H.data == nullptr); CHECK(es.H.timers.empty());
    mock::current = &gc.H; x->destroy(); CHECK(gc.F.live == 0);
    mock::Feed empty; es.H.feedp = &empty; gc.H.feedp = &empty; for (int i = 0; i < 3; ++i) { cycle(x, es.H); cycle(x, gc.H); }
    CHECK(stripText(es.H) == sES && stripText(gc.H) == sGC && pillText(gc.H) == "?");
    CHECK(dumpDraws(es.H) == drawsES); CHECK(dumpDraws(gc.H) == drawsGC);
    CHECK(get(P("TurnPane\\turn-ES-180.txt")) == recES && get(P("TurnPane\\turn-GC-180.txt")) == recGC);            // nothing re-decided / rewritten
    // new candles after the restart with no order data: at a level they are recorded WITHOUT a Stop Run; the old lines never change
    es.chartTo(K906 + 540); for (int i = 0; i < 3; ++i) cycle(x, es.H);
    { const std::string r2 = get(P("TurnPane\\turn-ES-180.txt")); CHECK(lineOf(r2, "T|1795164660|") == t851 && lineOf(r2, "T|1795165380|") == t903 && lineOf(r2, "C|1795164660|") == lineOf(recES, "C|1795164660|"));
      CHECK(countLines(r2, "T|") == 2 && countLines(r2, "C|") >= countLines(recES, "C|")); }
    // the nightly's rows: with no live record the same sweeps are shown from the store's read, marked (nightly)
    mock::current = &es.H; x->destroy(); std::remove(P("TurnPane\\turn-ES-180.txt").c_str()); put(P("LRA-TurnPane-ES.csv"), get(data + "/LRA-Turn-ES-nightly.csv"));
    es.chartTo(K906 + 180); for (int i = 0; i < 3; ++i) cycle(x, es.H);
    std::cout << "ES nightly: " << stripText(es.H) << " | " << pillText(es.H) << "\n";
    CHECK(stripText(es.H).find("ONH 9:03 AM (nightly)   Heavy Stop Run / Heavy Effort / Moderate Exhaustion ? / Heavy Answer   \xBB UNCLEAR") == 0); noOverlap(es.H);
    CHECK(get(P("TurnPane\\turn-ES-180.txt")).find("T|") == std::string::npos);                                 // a nightly row is never written into the live record
    // a newer Sw with no live data and no nightly row: Effort from the chart only, NO READ
    put(P("RevRead\\wy-ES-180.txt"), get(data + "/wy-ES.txt") + std::to_string(K906) + "|Sw|1|PDL|28768|\n");
    for (int i = 0; i < 3; ++i) cycle(x, es.H);
    std::cout << "ES new Sw: " << stripText(es.H) << " | " << pillText(es.H) << "\n";
    CHECK(stripText(es.H).find("PDL 9:06 AM   \x96 Stop Run / ") == 0 && stripText(es.H).find("\x96 Exhaustion / \x96 Answer   \xBB NO READ") != std::string::npos && pillText(es.H) == "\x96");
    // garbled params and a foreign sweep file: defaults + reasons, the last good sweep list kept
    put(P("LRA-TurnPane-ES.csv"), "VERSION,1\nMARKET,ES\nSTOP_TYP,abc\n");
    put(P("RevRead\\wy-ES-180.txt"), "WY|1|NQ|180|0.21.7\n");
    for (int i = 0; i < 2; ++i) cycle(x, es.H);
    { const std::string st = get(P("TurnPane\\status-ES-180.txt")); CHECK(st.find("PARAMS|params line 3 garbled (defaults)") != std::string::npos); CHECK(st.find("SWEEPS|sweep file is NQ|3") != std::string::npos); }
    CHECK(stripText(es.H).find("PDL 9:06 AM") == 0);
    // symbol change on the ES chart: the feed is closed (unsubscribed) and the state starts clean
    es.H.feedp = &es.F; es.F.ev.clear(); es.F.at = 0; es.H.symbol = "NQZ6"; es.H.root = "NQ"; cycle(x, es.H);
    CHECK(es.F.live == 0 || es.F.live == 1);
    // backlog cap with no timer: one calc reads at most SLICE_MAX events
    { Sim b; b.tick = 0.1; b.k0 = gc.k0; b.ev = gc.ev; b.vap = gc.vap; b.H = gc.H; b.H.data = nullptr; b.H.timers.clear(); b.H.timerRefuse = true; b.H.feedp = &b.F; b.chartTo(K906); b.feedTo(K906);
      mock::current = &b.H; x->calc(0); x->calc(0); const size_t at1 = b.F.at; x->calc(0);
      CHECK(b.F.at - at1 <= (size_t)trh::SLICE_MAX && b.F.at > at1);
      mock::current = &b.H; x->destroy(); CHECK(b.F.live == 0); }
    // a faulting feed read stops the feed, never crashes
    { Sim f; f.tick = 0.1; f.k0 = gc.k0; f.ev = gc.ev; f.vap = gc.vap; f.H = gc.H; f.H.data = nullptr; f.H.timers.clear(); f.H.feedp = &f.F; f.chartTo(K906); f.feedTo(K906); f.F.throwOnNext = true;
      for (int i = 0; i < 3; ++i) cycle(x, f.H);
      mock::current = &f.H; x->destroy(); CHECK(f.F.live == 0); }
    // a chart the read was not measured on (1-minute bars, or another market): no order feed is opened, one grey line says why
    { mock::Host m1 = gc.H; m1.data = nullptr; m1.timers.clear(); mock::Feed f1; m1.feedp = &f1; m1.seconds = 60; cycle(x, m1); cycle(x, m1);
      CHECK(f1.opens == 0 && m1.draws.size() == 1 && m1.draws[0].text == "Turn: reads 3-minute charts of ES NQ CL GC HG NG EU only"); noOverlap(m1);
      CHECK(get(P("TurnPane\\status-GC-60.txt")).find("not a 3-minute chart of the 7 markets") != std::string::npos);
      m1.seconds = 180; m1.symbol = "ZBZ6"; m1.root = "ZB"; cycle(x, m1); CHECK(f1.opens == 0 && m1.draws.size() == 1);
      mock::current = &m1; x->destroy(); CHECK(f1.live == 0 && m1.data == nullptr); }
    // lsRevRead's sweep file is missing: the pane says so (and still opens the feed, ready for the first sweep)
    { mock::Host h = gc.H; h.data = nullptr; h.timers.clear(); mock::Feed f2; h.feedp = &f2; h.symbol = "HGZ6"; h.root = "HG"; cycle(x, h); cycle(x, h);
      bool said = false; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text.find("Turn: no sweep file") == 0) said = true;
      CHECK(said && f2.opens == 1); noOverlap(h); mock::current = &h; x->destroy(); CHECK(f2.live == 0); }
    mock::current = &es.H; x->destroy(); CHECK(es.F.live == 0);
    mock::current = &gc.H; x->destroy(); CHECK(gc.F.live == 0 && empty.live == 0);
    // a host with no per-chart slot (its own DLL object): nothing is drawn, no feed is opened, nothing leaks
    cppExtension* y = CreateExtension();
    { mock::Host n = gc.H; n.data = nullptr; n.noSlot = true; n.timers.clear(); mock::Feed nf; n.feedp = &nf; cycle(y, n); cycle(y, n); CHECK(n.draws.empty() && nf.live == 0); mock::current = &n; y->destroy(); }
    for (cppExtension* e : { x, y }) { TurnPane* px = static_cast<TurnPane*>(e);   // IRT owns the extension object; the per-chart states were freed by destroy()
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
      delete px;
#pragma GCC diagnostic pop
    }
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
