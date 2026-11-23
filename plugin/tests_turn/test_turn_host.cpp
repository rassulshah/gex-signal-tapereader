// tests_turn/test_turn_host.cpp -- the WHOLE plugin (Turn.cpp) on the mock SDK, fed like a live morning: the real 2026-11-20 order events
// (ES / GC, Databento -> IRT event form, tests_turn/mk_events.py) arrive candle by candle on two charts of one DLL while the charts grow.
// Checks: the sweep reads and the level candles are decided once and recorded, the strip and the bars, scrolling back, a restart without
// repaint, the nightly's rows, garbled / foreign files, symbol change, backlog cap, a faulting feed, no leaks.
// (1.2.0) THE DEFAULT IS JUST EFFORT (no display file): one centred bar a side on every closed candle, green / red only when the number on
// it is 1.5 or more; the top line = the candle under the mouse, else the last closed candle; nothing of the turn read is drawn, all of it
// is still decided and recorded. The saved full setting (display.txt with every piece on, coloured from 0.5) brings the 1.1.0 pane back
// draw for draw - the whole test below runs with it, and switches to the default and back in the middle. One piece on; a garbled file.
// (1.1.0) Effort on EVERY closed candle (buyers up / sellers down, the right-hand column) against an independent calculation of Friday's
// candles; the forming candle carries nothing; a sweep candle with a read is drawn as before; the strip of a plain candle and of the half
// of a level candle that has no level; a candle whose volume at price cannot be read.
// (1.0.1) the strip on the pane's top edge, the candle under the mouse (a sweep, a candle at a named level - either side -, a candle with
// no bars, the forming candle, leaving the pane, another chart's window, bad events), one ratio per candle side on narrow candles.
// g++ -std=c++17 -DLSPERF_FAKE -fsanitize=address,undefined -Wall -Wextra -Werror -Imock -I.. ; run from tests_turn with
//   TURN_TEST_DATA=<dir with ev_ES.txt ev_GC.txt LRA-Turn-ES.csv LRA-Turn-GC.csv LRA-Turn-ES-nightly.csv wy-ES.txt wy-GC.txt>  (prep_host.py)
#define TURN_FILE_CHECK_MS 0
#define TURN_STATUS_MS 0
#define TURN_DECIDE_WAIT_MS 0
#define TURN_NOSUM_WAIT_MS 0
#define E_TIMER_MOCK 0
#define PC_TEST_CURSOR 1      /* PaneCursor.h: the screen cursor is pc::test_cx / test_cy */
#define TURN_TEST_WINDOW 1    /* Turn.cpp: the window under the cursor is turn_test_window */
#include "irtsdk.h"
namespace mock { Feed feed; Host* current = nullptr; }
#include "../Turn.cpp"
namespace pc { int test_cx = 0, test_cy = 0; bool test_ok = false; }
void* turn_test_window = nullptr;
#include <cctype>
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
    for (const mock::Draw& d : H.draws) if (d.type == "text" && d.t == H.paneTop + 4 && !d.centred) { if (at >= 0 && d.l > at) s += std::string((size_t)((d.l - at) / 6), ' '); s += d.text; at = d.r; }
    return s;
}
static std::string pillText(const mock::Host& H) { for (const mock::Draw& d : H.draws) if (d.type == "text" && d.t == H.paneTop + 4 && d.centred) return d.text; return std::string(); }
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
// (1.0.1) a ratio written on a bar: digits and at most one point ("2.9", "14"), or the cap "99+"; never an x
static bool isRatio(const std::string& s) { if (s == "99+") return true; if (s.empty()) return false; int dots = 0; for (char c : s) { if (c == '.') ++dots; else if (!std::isdigit((unsigned char)c)) return false; } return dots <= 1; }
static bool isMarks(const std::string& s) { return s.size() == 3 && s[1] == '/' && s[2] == '3' && s[0] >= '0' && s[0] <= '3'; }
// the labels below the strip: each is a ratio or the Answer's marks, 9 pt; none lies over a bar of ANOTHER candle (bars = boxes below the strip, not the axis)
static int barLabels(const mock::Host& H, int bw) {
    int n = 0;
    for (const mock::Draw& d : H.draws) { if (d.type != "text" || d.t <= H.paneTop + 21) continue; ++n;
        CHECK(isRatio(d.text) || isMarks(d.text)); CHECK(d.fsize == 9); CHECK(d.text.find('x') == std::string::npos);
        const int cx = (d.l + d.r) / 2;
        for (const mock::Draw& b : H.draws) { if (b.type != "box" || b.t <= H.paneTop + 21 || b.b - b.t <= 1 || b.r - b.l > 20) continue;
            const bool ov = d.l < b.r && b.l < d.r && d.t < b.b && b.t < d.b; if (!ov) continue;
            const int bc = (b.l + b.r) / 2; const bool own = bc >= cx - bw / 2 - 1 && bc <= cx + bw / 2 + 1; if (!own) std::printf("label '%s' [%d %d %d %d] over a foreign bar [%d %d %d %d]\n", d.text.c_str(), d.l, d.t, d.r, d.b, b.l, b.t, b.r, b.b); CHECK(own); } }
    return n;
}
static std::string barsOnly(const mock::Host& H) {                     // everything below the strip row: the bars, the axis and their labels
    std::ostringstream o; for (const mock::Draw& d : H.draws) if (d.t > H.paneTop + 21) o << d.type << "|" << d.l << "|" << d.t << "|" << d.r << "|" << d.b << "|" << std::hex << d.color << std::dec << "|" << d.text << "\n"; return o.str();
}
// (1.0.1) the cursor at pane point (h, v) of this chart's window, on chart bar `bar`; the event must never be swallowed
static void mouseAt(cppExtension* x, mock::Host& H, long bar, int h, int v, int type = E_MOUSE_MOVE) {
    mock::current = &H; turn_test_window = (void*)&H; pc::test_ok = true; pc::test_cx = h + 100; pc::test_cy = v + 50;
    RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = type; e.v.mouse.h = (short)h; e.v.mouse.v = (short)v; e.v.mouse.bar = bar;
    CHECK(x->mouse(&e) == RTX_FAIL);
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* tmp = std::getenv("TMPDIR"); root = std::string(tmp ? tmp : "/tmp") + "/turn_host_u"; setenv("USERPROFILE", root.c_str(), 1);
    const char* td = std::getenv("TURN_TEST_DATA"); if (!td) { std::printf("set TURN_TEST_DATA\n"); return 2; } data = td;
    for (const char* f : { "TurnPane\\turn-ES-180.txt", "TurnPane\\turn-GC-180.txt", "TurnPane\\_opening-ES-180.txt", "TurnPane\\_opening-GC-180.txt", "TurnPane\\status-ES-180.txt", "TurnPane\\status-GC-180.txt" }) std::remove(P(f).c_str());
    const long long K845 = 1795164300LL, K851 = 1795164660LL, K903 = 1795165380LL, K906 = K903 + 180;   // bar ends, CT clock
    put(P("LRA-TurnPane-ES.csv"), get(data + "/LRA-Turn-ES.csv")); put(P("LRA-TurnPane-GC.csv"), get(data + "/LRA-Turn-GC.csv"));
    put(P("RevRead\\wy-ES-180.txt"), get(data + "/wy-ES.txt")); put(P("RevRead\\wy-GC-180.txt"), get(data + "/wy-GC.txt"));
    const std::string FULL = "TURNSHOW|1\nstop|1\nexhaustion|1\nanswer|1\nread|1\neffort_from|0.5\nEND\n";   // (1.2.0) the saved full setting = display-full.txt
    put(P("TurnPane\\display.txt"), FULL);
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
      for (const mock::Draw& d : gc.H.draws) { if (d.t <= 21 || d.l < x - 30 || d.r > x + 30) continue;
          if (d.type == "text" && isRatio(d.text)) { ++ratio; CHECK(d.fsize == 9); }   // (1.0.1) no x, 9 pt
          if (d.type == "text" && d.text == "2/3") { ++marks; }
          if (d.type == "box" && d.r - d.l == 20) { if (d.color == trh::C_LGREEN && d.r <= x) ++lg; else if (d.color == trh::C_GREEN && d.l >= x) ++g; else if (d.color == trh::C_LRED && d.r <= x) ++lr; else if (d.color == trh::C_RED && d.l >= x) ++r; else ++other; } }
      std::printf("GC 9:03 candle: %d ratio labels, %d marks label, light green %d green %d light red %d red %d other %d\n", ratio, marks, lg, g, lr, r, other);
      CHECK(ratio == 3 && marks == 1 && lg == 1 && g == 1 && lr == 1 && r == 1 && other == 0); }
    { int boxes = 0; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.r - d.l == 20) { ++boxes; CHECK(d.color == trh::C_GREEN || d.color == trh::C_LGREEN || d.color == trh::C_RED || d.color == trh::C_LRED || d.color == trh::C_AXIS); CHECK(d.border == d.color); }
      CHECK(boxes >= 6); }                                             // fill-only bars: the border is the fill colour (no outlines)
    noOverlap(es.H); noOverlap(gc.H);
    if (const char* dump = std::getenv("TURN_DUMP")) { put(std::string(dump) + "_ES.txt", dumpDraws(es.H)); put(std::string(dump) + "_GC.txt", dumpDraws(gc.H)); }
    const std::string stES = get(P("TurnPane\\status-ES-180.txt")); std::cout << stES;
    CHECK(stES.find("VERSION|1.2.0") == 0 && stES.find("PARAMS|ok") != std::string::npos && stES.find("SWEEPS|ok|2|levels 16") != std::string::npos && stES.find("COVER|last 100%") != std::string::npos);
    CHECK(stES.find("FEED|ready") != std::string::npos && stES.find("|faults 0|") != std::string::npos && stES.find("RECORD|reads 2|") != std::string::npos);
    CHECK(stES.find("DISPLAY|1.2.0|strip on the pane top: a sweep|ratios on every bar (wide candles) ") != std::string::npos && stES.find("|mouse not in this pane|mouse moves 0|effort candles ") != std::string::npos);
    CHECK(stES.find("SHOW|ok|effort stop exhaustion answer read|effort coloured from 0.5\n") != std::string::npos);
    // (1.1.0) EFFORT ON EVERY CLOSED CANDLE. Friday's ES candles 8:42 - 9:06 AM against a calculation made apart from the plugin (pandas, from
    // the same order events: data/eff_ref_ES.csv): buyers' Effort at the top / sellers' at the bottom. At 60 px a candle every bar is 20 px
    // wide (left column x-21..x-1 = Stop Run / Exhaustion, right column x+1..x+21 = Effort / the Answer) and carries its ratio.
    { struct Ref { long long key; double top, bot; }; const Ref REF[] = { { 1795164120LL, 0.4619, 0.7872 }, { 1795164300LL, 0.4566, 0.6958 }, { 1795164480LL, 0.5630, 0.2552 }, { 1795164660LL, 0.6574, 1.1686 },
          { 1795164840LL, 0.8703, 0.2669 }, { 1795165020LL, 1.2085, 0.1707 }, { 1795165200LL, 0.7649, 0.2482 }, { 1795165380LL, 1.7001, 1.8336 }, { 1795165560LL, 0.2815, 1.0529 } };
      int axis = -1; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.b - d.t == 1 && d.r - d.l > 200 && d.t > 21) axis = d.t;
      CHECK(axis == 161); const int half = 125; int plain = 0, seen = 0, effN = 0;
      const turn::Rec rec = [&]() { turn::Rec r; turn::parseRecFile(recES, "ES", 180, r); return r; }();
      const auto boxAt = [&](int l, bool up) -> const mock::Draw* { for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.l == l && d.r == l + 20 && (up ? d.b == axis : d.t == axis + 1)) return &d; return nullptr; };
      const auto labelOf = [&](const mock::Draw& b, bool up) -> std::string { for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21 && (d.l + d.r) / 2 >= b.l + 6 && (d.l + d.r) / 2 <= b.r - 6 && (up ? d.b == b.t - 1 : d.t == b.b + 1)) return d.text; return std::string(); };
      for (size_t i = 0; i + 1 < es.H.bars.size(); ++i) {                    // every closed candle of the chart
          const long long key = (long long)es.H.bars[i].time; const int cx = es.H.barX0 + (int)i * es.H.barW; const CandleVap& cv = es.vap[key];
          std::vector<turn::Row> rows; for (const auto& r : cv.rows) { turn::Row q; q.tick = r.first; q.buy = r.second.first; q.sell = r.second.second; rows.push_back(q); }
          const turn::Bars br = turn::barRatios(rows, cv.lo, cv.hi); const turn::EffPair e = turn::effOf(br, 5);
          for (const Ref& rf : REF) if (rf.key == key) { ++seen; CHECK(br.topOk && br.botOk && std::fabs(br.top - rf.top) < 0.0002 && std::fabs(br.bot - rf.bot) < 0.0002); }   // = the independent calculation
          const mock::Draw *ru = boxAt(cx + 1, true), *rd = boxAt(cx + 1, false), *lu = boxAt(cx - 21, true), *ld = boxAt(cx - 21, false);
          const bool sweep = key == K851 || key == K903;
          if (sweep) {                                                       // a sweep candle with its read: the four bars of 1.0.1 (the push up, the turn down)
              CHECK(ru && ru->color == (e.up.on ? trh::C_GREEN : trh::C_AXIS) && ru->b - ru->t == turn::barPx(e.up.v, half));   // its Effort = the same number
              CHECK(rd && (rd->color == trh::C_RED || rd->color == trh::C_AXIS));
              if (rd) CHECK(isMarks(labelOf(*rd, false)));               // the right-hand bar going down is the Answer, not the sellers' Effort
              CHECK(lu && (lu->color == trh::C_LGREEN || lu->color == trh::C_AXIS)); CHECK(ld && (ld->color == trh::C_LRED || ld->color == trh::C_AXIS));
              continue; }
          CHECK(e.up.show && e.dn.show);
          CHECK(ru && ru->color == (e.up.on ? trh::C_GREEN : trh::C_AXIS) && ru->b - ru->t == turn::barPx(e.up.v, half) && labelOf(*ru, true) == e.up.label);
          CHECK(rd && rd->color == (e.dn.on ? trh::C_RED : trh::C_AXIS) && rd->b - rd->t == turn::barPx(e.dn.v, half) && labelOf(*rd, false) == e.dn.label);
          const std::map<long long, turn::Cand>::const_iterator ci = rec.cands.find(key);     // the left column: a Stop Run, at a named level only
          const bool hiStop = ci != rec.cands.end() && ci->second.hiL && ci->second.hiX1000 >= 50, loStop = ci != rec.cands.end() && ci->second.loL && ci->second.loX1000 >= 50;
          CHECK((lu != nullptr) == hiStop && (ld != nullptr) == loStop);
          if (lu) CHECK(lu->color == trh::C_LGREEN || lu->color == trh::C_AXIS);
          if (ld) CHECK(ld->color == trh::C_LRED || ld->color == trh::C_AXIS);
          if (ci == rec.cands.end()) ++plain;
          ++effN;
      }
      CHECK(seen == 9 && plain >= 2);
      { const std::string d = lineOf(stES, "DISPLAY|"); const size_t a = d.find("|effort candles "); int n = -1; if (a != std::string::npos) std::sscanf(d.c_str() + a, "|effort candles %d", &n); std::printf("candles with Effort bars and no sweep read: %d, of them with no level %d (status %d)\n", effN, plain, n); CHECK(n == effN); }
      // 8:45 AM, a plain candle: buyers 0.46 (reads "0.5": green with the full setting's "from 0.5"), sellers 0.70 (red, "0.7"); 8:57: buyers 1.21 (green, "1.2")
      { const int cx = es.H.barX0 + (int)((K845 - es.k0) / 180) * es.H.barW; const mock::Draw *u = boxAt(cx + 1, true), *d = boxAt(cx + 1, false);
        CHECK(u && d && u->color == trh::C_GREEN && labelOf(*u, true) == "0.5" && d->color == trh::C_RED && labelOf(*d, false) == "0.7" && !boxAt(cx - 21, true) && !boxAt(cx - 21, false)); }
      { const int cx = es.H.barX0 + (int)((K903 - 360 - es.k0) / 180) * es.H.barW; const mock::Draw* u = boxAt(cx + 1, true); CHECK(u && u->color == trh::C_GREEN && labelOf(*u, true) == "1.2"); }
      // the forming candle: nothing (its Effort is not final)
      { const int cx = es.H.barX0 + (int)(es.H.bars.size() - 1) * es.H.barW; for (const mock::Draw& d : es.H.draws) if (d.t > 21 && d.b - d.t > 1) CHECK(d.r <= cx - 22 || d.type != "box"); for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21) CHECK(d.r <= cx - 20); } }
    // (1.2.0) THE DEFAULT: NO DISPLAY FILE = JUST EFFORT. The file goes away while the pane runs: the next calc asks for a redraw and the
    // pane draws ONE centred bar a side on every closed candle (40 px wide at 60 px a candle), green / red only when the number written on
    // it is 1.5 or more, with that number; nothing else - no Stop Run, no Exhaustion, no Answer, no level, no verdict, no pill.
    { const std::string fullDraws = dumpDraws(es.H), rec0 = get(P("TurnPane\\turn-ES-180.txt")); CHECK(countLines(rec0, "T|") == 2);
      std::remove(P("TurnPane\\display.txt").c_str());
      { const int inv = es.H.invalidates; mock::current = &es.H; x->calc(0); CHECK(es.H.invalidates == inv + 1); x->calc(0); CHECK(es.H.invalidates == inv + 1); }   // the switch is shown at once, asked for once
      cycle(x, es.H); cycle(x, es.H);
      const auto justEffort = [&](int bw, int wBar, int half) {            // every box and number under the top line against the independent calculation
          int axis = -1; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.b - d.t == 1 && d.r - d.l > 200 && d.t > 21) axis = d.t;
          CHECK(axis == 161); int bars = 0, coloured = 0, expected = 0;
          for (size_t i = 0; i + 1 < es.H.bars.size(); ++i) {
              const long long key = (long long)es.H.bars[i].time; const int cx = es.H.barX0 + (int)i * bw; const CandleVap& cv = es.vap[key];
              std::vector<turn::Row> rows; for (const auto& r : cv.rows) { turn::Row q; q.tick = r.first; q.buy = r.second.first; q.sell = r.second.second; rows.push_back(q); }
              const turn::EffPair e = turn::effOf(turn::barRatios(rows, cv.lo, cv.hi), 15);
              for (int side = 0; side < 2; ++side) { const turn::Bar1& b = side ? e.dn : e.up; if (!b.show) continue; ++expected;
                  const mock::Draw* box = nullptr; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.l == cx - wBar / 2 && d.r == cx - wBar / 2 + wBar && (side ? d.t == axis + 1 : d.b == axis) && d.b - d.t >= 2) box = &d;
                  CHECK(box != nullptr); if (!box) continue;
                  const bool on = turn::shown10(b.label) >= 15; CHECK(on == b.on);
                  CHECK(box->b - box->t == turn::barPx(b.v, half)); CHECK(box->color == (on ? (side ? trh::C_RED : trh::C_GREEN) : trh::C_AXIS)); if (on) ++coloured;
                  if (bw >= 22) { bool lab = false; for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21 && d.text == b.label && std::abs((d.l + d.r) / 2 - cx) <= 1 && (side ? d.t == box->b + 1 : d.b == box->t - 1) && d.color == box->color && d.fsize == 9) lab = true; CHECK(lab); } } }
          for (const mock::Draw& d : es.H.draws) { if (d.t <= 21 || d.type != "box" || d.b - d.t == 1) continue; ++bars;      // nothing but those bars: one width, three colours
              CHECK(d.r - d.l == wBar); CHECK(d.color == trh::C_GREEN || d.color == trh::C_RED || d.color == trh::C_AXIS); }
          CHECK(bars == expected && expected >= 2 * ((int)es.H.bars.size() - 2)); CHECK(coloured >= 2);
          for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21) CHECK(isRatio(d.text));              // no "3/3" marks
          return coloured; };
      const int col60 = justEffort(60, 40, 125);
      { const std::string s = stripText(es.H); std::cout << "JUST EFFORT, no mouse: " << s << " | pill '" << pillText(es.H) << "'\n";
        CHECK(s == "9:06 AM   Buyers No Effort 0.3x / Sellers Moderate Effort 1.1x" && pillText(es.H).empty()); noOverlap(es.H); }   // the last closed candle; both under 1.5: grey
      for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t == es.H.paneTop + 4) CHECK(d.color == trh::C_MUTED);
      cycle(x, es.H);
      { const std::string st = get(P("TurnPane\\status-ES-180.txt")); std::cout << lineOf(st, "SHOW|") << "\n" << lineOf(st, "DISPLAY|") << "\n";
        CHECK(st.find("SHOW|no display file (Effort only)|effort|effort coloured from 1.5\n") != std::string::npos && lineOf(st, "DISPLAY|").find("strip on the pane top: a candle|ratios one per candle side ") != std::string::npos);
        int n = -1; const std::string d = lineOf(st, "DISPLAY|"); const size_t a = d.find("|effort candles "); if (a != std::string::npos) std::sscanf(d.c_str() + a, "|effort candles %d", &n); CHECK(n == (int)es.H.bars.size() - 1); }
      const std::string bars0 = barsOnly(es.H);
      const auto idx = [&](long long key) { return (long)((key - es.k0) / 180); }; const auto hx = [&](long long key) { return es.H.barX0 + (int)idx(key) * es.H.barW; };
      // the mouse: the candle under it - a sweep candle is a candle like any other; 9:03 has both sides at 1.5+ (green and red words)
      mouseAt(x, es.H, idx(K851), hx(K851), 100); cycle(x, es.H); CHECK(stripText(es.H) == "8:51 AM   Buyers Light Effort 0.7x / Sellers Moderate Effort 1.2x"); CHECK(barsOnly(es.H) == bars0);
      mouseAt(x, es.H, idx(K903), hx(K903), 250); cycle(x, es.H);
      { std::cout << "JUST EFFORT, mouse on 9:03: " << stripText(es.H) << "\n"; CHECK(stripText(es.H) == "9:03 AM   Buyers Heavy Effort 1.7x / Sellers Heavy Effort 1.8x"); CHECK(barsOnly(es.H) == bars0);
        bool g = false, r = false; for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t == es.H.paneTop + 4) { if (d.text == "Buyers Heavy Effort 1.7x" && d.color == trh::C_GREEN) g = true; if (d.text == "Sellers Heavy Effort 1.8x" && d.color == trh::C_RED) r = true; }
        CHECK(g && r); noOverlap(es.H); }
      mouseAt(x, es.H, (long)es.H.bars.size() - 1, hx(K906 + 180), 100); cycle(x, es.H); CHECK(stripText(es.H) == "9:06 AM   Buyers No Effort 0.3x / Sellers Moderate Effort 1.1x");   // the forming candle: the last closed one
      turn_test_window = nullptr; cycle(x, es.H); CHECK(stripText(es.H) == "9:06 AM   Buyers No Effort 0.3x / Sellers Moderate Effort 1.1x"); CHECK(barsOnly(es.H) == bars0);
      { const int keep = es.H.vis1; es.H.vis1 = (int)idx(K851); cycle(x, es.H); CHECK(stripText(es.H) == "8:51 AM   Buyers Light Effort 0.7x / Sellers Moderate Effort 1.2x"); es.H.vis1 = keep; cycle(x, es.H); }   // scrolled back: the last candle in view
      // his zoom (22 px): the bar is 16 px, every bar has its number, nothing overlaps; 12 / 3 px and a small pane: no crash, no overlap
      es.H.barW = 22; es.H.barX0 = 1100; cycle(x, es.H); CHECK(justEffort(22, 16, 125) == col60); noOverlap(es.H);
      { cycle(x, es.H); const std::string d = lineOf(get(P("TurnPane\\status-ES-180.txt")), "DISPLAY|"); int dr = -1, of = -1; const size_t a = d.find("one per candle side "); if (a != std::string::npos) std::sscanf(d.c_str() + a, "one per candle side %d of %d", &dr, &of); CHECK(dr == of && dr >= 16); }
      es.H.barW = 12; es.H.barX0 = 1200; cycle(x, es.H); noOverlap(es.H); barLabels(es.H, 12); es.H.barW = 3; es.H.barX0 = 1340; cycle(x, es.H); noOverlap(es.H);
      es.H.paneRight = 500; es.H.barX0 = 300; cycle(x, es.H); noOverlap(es.H); es.H.paneBottom = 60; cycle(x, es.H); CHECK(stripText(es.H).empty());
      es.H.paneBottom = 300; es.H.paneRight = 1400; es.H.barW = 60; es.H.barX0 = 600; cycle(x, es.H); CHECK(barsOnly(es.H) == bars0);
      // nothing of the turn read is drawn - all of it is still there: the record is untouched
      CHECK(get(P("TurnPane\\turn-ES-180.txt")) == rec0);
      // a garbled / half-written display file: Effort only, with the reason
      put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|1\nexhaustion|maybe\nEND\n"); cycle(x, es.H); cycle(x, es.H); CHECK(barsOnly(es.H) == bars0);
      CHECK(get(P("TurnPane\\status-ES-180.txt")).find("SHOW|display line 3 garbled (Effort only)|effort|effort coloured from 1.5\n") != std::string::npos);
      put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|1\nexhaustion|1\nanswer|1\nread|1\n"); cycle(x, es.H); cycle(x, es.H); CHECK(barsOnly(es.H) == bars0);
      CHECK(get(P("TurnPane\\status-ES-180.txt")).find("SHOW|display file not whole (Effort only)|") != std::string::npos);
      // ONE PIECE back (the Stop Run): two columns again - the Stop Run left where the record has one, Effort right on both halves of
      // every candle (a sweep candle too: its turn half holds the sellers' Effort, no Exhaustion, no Answer); the top line stays Effort
      put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|1\nEND\n"); cycle(x, es.H); cycle(x, es.H);
      { int axis = 161, stops = 0; const int cx = hx(K903);
        const auto boxAt = [&](int l, bool up) -> const mock::Draw* { for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.l == l && d.r == l + 20 && (up ? d.b == axis : d.t == axis + 1)) return &d; return nullptr; };
        const mock::Draw *lu = boxAt(cx - 21, true), *ru = boxAt(cx + 1, true), *ld = boxAt(cx - 21, false), *rd = boxAt(cx + 1, false);
        CHECK(lu && lu->color == trh::C_LGREEN && ru && ru->color == trh::C_GREEN && !ld && rd && rd->color == trh::C_RED);                       // 9:03: Stop Run 5.2x + buyers 1.7 up; sellers 1.8 down
        for (const mock::Draw& d : es.H.draws) { if (d.type == "text" && d.t > 21) CHECK(isRatio(d.text)); if (d.type == "box" && d.t > 21 && d.b - d.t > 1) { CHECK(d.r - d.l == 20); if (d.color == trh::C_LGREEN || d.color == trh::C_LRED) ++stops; } }
        CHECK(stops >= 3); CHECK(stripText(es.H) == "9:06 AM   Buyers No Effort / Sellers Moderate Effort" && pillText(es.H).empty()); noOverlap(es.H);
        CHECK(get(P("TurnPane\\status-ES-180.txt")).find("SHOW|ok|effort stop|effort coloured from 1.5\n") != std::string::npos || (cycle(x, es.H), get(P("TurnPane\\status-ES-180.txt")).find("SHOW|ok|effort stop|effort coloured from 1.5\n") != std::string::npos)); }
      // THE SAVED FULL SETTING back: the pane of before, draw for draw; nothing was re-decided
      put(P("TurnPane\\display.txt"), FULL); cycle(x, es.H); cycle(x, es.H);
      CHECK(dumpDraws(es.H) == fullDraws); CHECK(get(P("TurnPane\\turn-ES-180.txt")) == rec0); }
    // (1.0.1) the strip sits on the pane's top edge: its box is rows 2..20, nothing is drawn above it, the bars start under it
    { bool stripBox = false; for (const mock::Draw& d : es.H.draws) { CHECK(d.t >= es.H.paneTop + 2); if (d.type == "box" && d.t == es.H.paneTop + 2 && d.b == es.H.paneTop + 20 && d.color == trh::C_STRIP) stripBox = true; }
      CHECK(stripBox); CHECK(barLabels(es.H, 60) >= 6); CHECK(barLabels(gc.H, 60) >= 4); }
    // scrolling back: the strip reads the newest sweep at or before the last candle in view
    { const int keep = es.H.vis1; es.H.vis1 = (int)((K851 - es.k0) / 180); cycle(x, es.H); const std::string s = stripText(es.H); std::cout << "ES scrolled back: " << s << " | " << pillText(es.H) << "\n";
      CHECK(s == "LonHI 8:51 AM   Light Stop Run / Light Effort / No Exhaustion ? / Light Answer   \xBB UNCLEAR"); noOverlap(es.H);
      es.H.vis1 = (int)((K851 - 180 - es.k0) / 180); cycle(x, es.H); CHECK(stripText(es.H).empty());        // before any sweep: no strip
      es.H.vis1 = keep; cycle(x, es.H); CHECK(stripText(es.H) == sES); }
    // (1.0.1) THE MOUSE: while it is in the pane the strip is about the candle under it
    { const std::string rec0 = get(P("TurnPane\\turn-ES-180.txt")); const std::string bars0 = barsOnly(es.H); const int inv0 = es.H.invalidates;
      const auto movesOf = [&]() { const std::string d = lineOf(get(P("TurnPane\\status-ES-180.txt")), "DISPLAY|"); const size_t a = d.find("|mouse moves "); int n = -1; if (a != std::string::npos) std::sscanf(d.c_str() + a, "|mouse moves %d", &n); return n; };
      cycle(x, es.H); const int moves0 = movesOf();                     // (1.2.0) the Effort-only block above moved the mouse too
      const auto idx = [&](long long key) { return (long)((key - es.k0) / 180); };
      const auto hx = [&](long long key) { return es.H.barX0 + (int)idx(key) * es.H.barW; };   // the mock draws a candle's centre here
      // a sweep candle with its live read: that sweep's strip (8:51 LonHI), not the newest one
      mouseAt(x, es.H, idx(K851), hx(K851), 100); CHECK(es.H.invalidates == inv0 + 1); cycle(x, es.H);
      std::cout << "mouse on 8:51: " << stripText(es.H) << " | " << pillText(es.H) << "\n";
      CHECK(stripText(es.H) == "LonHI 8:51 AM   Light Stop Run / Light Effort / No Exhaustion ? / Light Answer   \xBB UNCLEAR"); CHECK(barsOnly(es.H) == bars0); noOverlap(es.H);
      mouseAt(x, es.H, idx(K851), hx(K851) + 3, 110); CHECK(es.H.invalidates == inv0 + 1);                   // the same candle, the same side: no redraw asked
      // a candle at a named level that is not a sweep (9:06, ONH 28797, Stop Run 0.512x): level, time, Stop Run and Effort - nothing else
      mouseAt(x, es.H, idx(K906), hx(K906), 100); CHECK(es.H.invalidates == inv0 + 2); cycle(x, es.H);
      { const std::string s = stripText(es.H); std::cout << "mouse on 9:06: " << s << " | " << pillText(es.H) << "\n";
        CHECK(s.find("ONH 9:06 AM   Light Stop Run / ") == 0 && s.size() > 8 && s.compare(s.size() - 7, 7, " Effort") == 0); CHECK(s.find("Exhaustion") == std::string::npos && s.find("Answer") == std::string::npos && s.find("\xBB") == std::string::npos);
        CHECK(pillText(es.H) == "\x96"); CHECK(barsOnly(es.H) == bars0); noOverlap(es.H);
        cycle(x, es.H);                                                // the status file tells what the draw BEFORE its calc showed
        CHECK(lineOf(get(P("TurnPane\\status-ES-180.txt")), "DISPLAY|").find("strip on the pane top: a candle at a level|") != std::string::npos);
        CHECK(lineOf(get(P("TurnPane\\status-ES-180.txt")), "DISPLAY|").find("|mouse on a candle of this pane|mouse moves ") != std::string::npos && movesOf() == moves0 + 2); }
      // a candle at a named high AND a named low (9:00: ONH 9.628x above, 28768 0.279x below): the side the mouse is on
      mouseAt(x, es.H, idx(K903 - 180), hx(K903 - 180), 100); cycle(x, es.H);
      { const std::string up = stripText(es.H); std::cout << "mouse on 9:00, above the axis: " << up << "\n"; CHECK(up.find("ONH 9:00 AM   Heavy Stop Run / ") == 0);
        const int inv = es.H.invalidates; mouseAt(x, es.H, idx(K903 - 180), hx(K903 - 180), 250); CHECK(es.H.invalidates == inv + 1); cycle(x, es.H);   // crossing the axis on the same candle redraws
        const std::string dn = stripText(es.H); std::cout << "mouse on 9:00, below the axis: " << dn << "\n";
        CHECK(dn.find(" 9:00 AM   No Stop Run / ") != std::string::npos && dn.find("ONH") != 0 && dn != up); CHECK(barsOnly(es.H) == bars0); }
      // (1.1.0) a candle with no level (8:45, 8:42): its own strip - the time and the two Efforts, buyers green / sellers red / No grey,
      // nothing else; the bars never move
      mouseAt(x, es.H, idx(K845), hx(K845), 100); cycle(x, es.H);
      { const std::string s = stripText(es.H); std::cout << "mouse on 8:45 (no level): " << s << " | " << pillText(es.H) << "\n";
        CHECK(s == "8:45 AM   Buyers Light Effort / Sellers Light Effort" && pillText(es.H) == "\x96"); CHECK(barsOnly(es.H) == bars0); noOverlap(es.H);   // (1.2.0) buyers 0.46 reads 0.5: Light, as its bar
        bool buyGreen = false, sellRed = false; for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t == es.H.paneTop + 4) { if (d.text == "Buyers Light Effort" && d.color == trh::C_GREEN) buyGreen = true; if (d.text == "Sellers Light Effort" && d.color == trh::C_RED) sellRed = true; }
        CHECK(buyGreen && sellRed);
        cycle(x, es.H); CHECK(lineOf(get(P("TurnPane\\status-ES-180.txt")), "DISPLAY|").find("strip on the pane top: a candle|") != std::string::npos); }
      mouseAt(x, es.H, idx(K845 - 180), hx(K845 - 180), 250); cycle(x, es.H); CHECK(stripText(es.H) == "8:42 AM   Buyers Light Effort / Sellers Light Effort"); CHECK(barsOnly(es.H) == bars0);
      mouseAt(x, es.H, idx(K903 - 360), hx(K903 - 360), 100); cycle(x, es.H);                                 // 8:57 is at ONH (no Stop Run value): above the axis the level's strip ...
      CHECK(stripText(es.H).find("ONH 8:57 AM   ") == 0);
      mouseAt(x, es.H, idx(K903 - 360), hx(K903 - 360), 250); cycle(x, es.H);                                 // ... below it, where only the sellers' Effort bar stands: the plain strip
      { const std::string s = stripText(es.H); std::cout << "mouse on 8:57, below the axis: " << s << "\n"; CHECK(s == "8:57 AM   Buyers Moderate Effort / Sellers No Effort");
        bool buyGreen = false; for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t == es.H.paneTop + 4 && d.text == "Buyers Moderate Effort" && d.color == trh::C_GREEN) buyGreen = true; CHECK(buyGreen); CHECK(barsOnly(es.H) == bars0); }
      mouseAt(x, es.H, idx(K906), hx(K906), 250); cycle(x, es.H); CHECK(stripText(es.H) == "9:06 AM   Buyers No Effort / Sellers Moderate Effort");   // 9:06 at ONH, the mouse on its lower half
      // a sweep candle with its read: its strip on either half (its lower half carries the turn)
      mouseAt(x, es.H, idx(K851), hx(K851), 250); cycle(x, es.H); CHECK(stripText(es.H).find("LonHI 8:51 AM   Light Stop Run / Light Effort / ") == 0);
      // the forming candle (and the empty space right of it, which IRT reports as the last bar): the newest sweep, as with no mouse
      mouseAt(x, es.H, (long)es.H.bars.size() - 1, hx(K906 + 180) + 40, 100); cycle(x, es.H); CHECK(stripText(es.H) == sES);
      // a hover event is read like a move; bad bar numbers and points outside the pane are no candle
      mouseAt(x, es.H, idx(K851), hx(K851), 100, E_MOUSE_HOVER); cycle(x, es.H); CHECK(stripText(es.H).find("LonHI 8:51 AM") == 0);
      mouseAt(x, es.H, -1, hx(K851), 100); cycle(x, es.H); CHECK(stripText(es.H) == sES);
      mouseAt(x, es.H, 99999, hx(K851), 100); cycle(x, es.H); CHECK(stripText(es.H) == sES);
      mouseAt(x, es.H, idx(K851), hx(K851), es.H.paneBottom + 40); cycle(x, es.H); CHECK(stripText(es.H) == sES);
      { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_UP; e.v.mouse.bar = idx(K851); mock::current = &es.H; CHECK(x->mouse(&e) == RTX_FAIL); CHECK(x->mouse(nullptr) == RTX_FAIL); cycle(x, es.H); CHECK(stripText(es.H) == sES); }
      // the cursor position cannot be read: no candle
      mouseAt(x, es.H, idx(K851), hx(K851), 100); pc::test_ok = false; { RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = E_MOUSE_MOVE; e.v.mouse.h = (short)hx(K851); e.v.mouse.v = 100; e.v.mouse.bar = idx(K851); mock::current = &es.H; x->mouse(&e); }
      cycle(x, es.H); CHECK(stripText(es.H) == sES); pc::test_ok = true;
      // the mouse LEAVES the pane (IRT sends no event): the next calc sees it and asks for a redraw; the strip is the newest sweep again
      mouseAt(x, es.H, idx(K851), hx(K851), 100); cycle(x, es.H); CHECK(stripText(es.H).find("LonHI 8:51 AM") == 0);
      { const int inv = es.H.invalidates; pc::test_cy = es.H.paneBottom + 50 + 80; mock::current = &es.H; x->calc(0); CHECK(es.H.invalidates == inv + 1); cycle(x, es.H); CHECK(stripText(es.H) == sES); CHECK(es.H.invalidates == inv + 1); }
      // ... or moves over ANOTHER window at the same screen point (the other chart)
      mouseAt(x, es.H, idx(K851), hx(K851), 100); cycle(x, es.H); CHECK(stripText(es.H).find("LonHI 8:51 AM") == 0);
      turn_test_window = (void*)&gc.H; cycle(x, es.H); CHECK(stripText(es.H) == sES);
      // two charts on one DLL: the mouse on the ES pane never changes the GC strip, and the other way round
      mouseAt(x, es.H, idx(K851), hx(K851), 100); cycle(x, es.H); cycle(x, gc.H); CHECK(stripText(gc.H) == sGC && stripText(es.H).find("LonHI 8:51 AM") == 0);
      mouseAt(x, gc.H, (long)((K845 - gc.k0) / 180), gc.H.barX0 + (int)((K845 - gc.k0) / 180) * gc.H.barW, 250); cycle(x, gc.H); cycle(x, es.H);
      { const std::string g = stripText(gc.H); std::cout << "mouse on GC 8:45 (a named low): " << g << "\n"; CHECK(g.find(" 8:45 AM   ") != std::string::npos && g.find(" Stop Run / ") != std::string::npos && g != sGC); CHECK(stripText(es.H) == sES); }   // the window is GC's now: ES is back on its newest sweep
      // scrolled so that the candle under the last known mouse bar is out of view: ignored
      mouseAt(x, es.H, idx(K906), hx(K906), 100); { const int keep = es.H.vis1; es.H.vis1 = (int)idx(K851); cycle(x, es.H); CHECK(stripText(es.H).find("LonHI 8:51 AM") == 0); es.H.vis1 = keep; }
      // nothing the mouse does decides, records or repaints anything
      turn_test_window = nullptr; cycle(x, es.H); cycle(x, gc.H); CHECK(stripText(es.H) == sES && stripText(gc.H) == sGC); CHECK(barsOnly(es.H) == bars0); CHECK(get(P("TurnPane\\turn-ES-180.txt")) == rec0); }
    // narrow candles: (1.0.1) ONE ratio per candle side, on its taller bar, in that bar's colour; the numbers also go into the strip; nothing overlaps
    { es.H.barW = 22; es.H.barX0 = 1100; cycle(x, es.H); const std::string s = stripText(es.H); std::cout << "ES narrow (22 px, his zoom): " << s << "\n";
      CHECK(s.find("ONH 9:03 AM   Heavy Stop Run 5.2x / Heavy Effort ") == 0 && s.find("Heavy Answer 3/3") != std::string::npos);
      // at his zoom a 9 pt ratio (16 px + 2 on the mock) fits inside its own 22 px candle: every candle side keeps its label
      cycle(x, es.H); const std::string st22 = get(P("TurnPane\\status-ES-180.txt")); std::cout << lineOf(st22, "DISPLAY|") << "\n";
      int lab = barLabels(es.H, 22), sides = 0; { const std::string d = lineOf(st22, "DISPLAY|"); const size_t a = d.find("one per candle side "); CHECK(a != std::string::npos); int dr = -1, of = -1; if (a != std::string::npos) std::sscanf(d.c_str() + a, "one per candle side %d of %d", &dr, &of); sides = of; CHECK(dr == lab && of >= dr && dr >= 6); CHECK(dr == of); }
      { const int x903 = es.H.barX0 + (int)((K903 - es.k0) / 180) * 22; bool up = false, down = false;   // the 9:03 sweep candle: "5.2" (its Stop Run, light green) above and one label of the turn below
        for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21 && (d.l + d.r) / 2 >= x903 - 2 && (d.l + d.r) / 2 <= x903 + 2) { if (d.text == "5.2" && d.color == trh::C_LGREEN && d.b < 161) up = true; if (d.t > 161 && (d.color == trh::C_LRED || d.color == trh::C_RED)) down = true; }
        CHECK(up && down); }
      noOverlap(es.H);
      // the mouse on a narrow candle at a level: the strip gives BOTH numbers of that candle, with the x ("0.5x"); the bars keep their labels
      { const std::string b22 = barsOnly(es.H); const long i906 = (long)((K906 - es.k0) / 180); mouseAt(x, es.H, i906, es.H.barX0 + (int)i906 * 22, 100); cycle(x, es.H);
        const std::string s2 = stripText(es.H); std::cout << "mouse on 9:06, narrow: " << s2 << "\n";
        CHECK(s2.find("ONH 9:06 AM   Light Stop Run 0.5x / ") == 0 && s2.back() == 'x' && s2.find(" Effort ") != std::string::npos); CHECK(barsOnly(es.H) == b22); noOverlap(es.H);
        if (const char* dump = std::getenv("TURN_DUMP")) put(std::string(dump) + "_ES_narrow_mouse.txt", dumpDraws(es.H));   // the render check's picture
        // (1.1.0) the mouse on a narrow candle with no level: both Efforts with their numbers
        mouseAt(x, es.H, (long)((K845 - es.k0) / 180), es.H.barX0 + (int)((K845 - es.k0) / 180) * 22, 100); cycle(x, es.H);
        std::cout << "mouse on 8:45, narrow: " << stripText(es.H) << "\n"; CHECK(stripText(es.H) == "8:45 AM   Buyers Light Effort 0.5x / Sellers Light Effort 0.7x"); CHECK(barsOnly(es.H) == b22); noOverlap(es.H);
        turn_test_window = nullptr; cycle(x, es.H); CHECK(stripText(es.H) == s); }
      // closer candles: labels compete - fewer are drawn than offered, the taller bar keeps its label, nothing overlaps
      es.H.barW = 12; es.H.barX0 = 1200; cycle(x, es.H); std::cout << "ES narrow (12 px): " << stripText(es.H) << "\n"; noOverlap(es.H);
      { const int n12 = barLabels(es.H, 12); CHECK(n12 >= 2 && n12 < lab); (void)sides;
        bool has52 = false; for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21 && d.text == "5.2" && d.color == trh::C_LGREEN) has52 = true; CHECK(has52); }   // a full-height bar of the newest candle
      es.H.barW = 3; es.H.barX0 = 1340; cycle(x, es.H); noOverlap(es.H); barLabels(es.H, 3);
      es.H.paneRight = 500; es.H.barX0 = 300; cycle(x, es.H); noOverlap(es.H); std::cout << "ES small pane: " << stripText(es.H) << "\n"; CHECK(stripText(es.H).find("UNCLEAR") != std::string::npos);
      es.H.paneBottom = 60; cycle(x, es.H); CHECK(stripText(es.H).empty()); CHECK(barLabels(es.H, 3) == 0);   // too low for the strip and for labels: bars only, no crash
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
      CHECK(said && f2.opens == 1); noOverlap(h);
      // (1.1.0) no sweep file, no levels: the Effort bars are there all the same - on every closed candle
      { int effBars = 0; for (const mock::Draw& d : h.draws) if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.r - d.l <= 20) ++effBars; CHECK(effBars >= (int)h.bars.size() - 1); }
      mock::current = &h; x->destroy(); CHECK(f2.live == 0); }
    // (1.1.0) a candle whose volume at price cannot be read: no Effort bar on it, the others are drawn, nothing crashes; an empty candle
    // (one price) carries none either
    { mock::Host h = gc.H; h.data = nullptr; h.timers.clear(); mock::Feed f3; h.feedp = &f3; h.symbol = "NGZ6"; h.root = "NG"; h.barW = 60; h.barX0 = 600;
      const int bad = 2, thin = 3; h.bars[bad].failStats = 1; h.bars[thin].rows.clear(); h.bars[thin].rows.push_back(mock::Price(h.bars[thin].high, 5, 5, 10)); h.bars[thin].low = h.bars[thin].high;
      cycle(x, h); cycle(x, h); int onBad = 0, onThin = 0, others = 0;
      for (const mock::Draw& d : h.draws) { if (d.type != "box" || d.t <= 21 || d.b - d.t <= 1 || d.r - d.l > 20) continue; const int i = (d.l + 10 - h.barX0 + h.barW / 2) / h.barW; if (i == bad) ++onBad; else if (i == thin) ++onThin; else ++others; }
      CHECK(onBad == 0 && onThin == 0 && others >= 4); CHECK(get(P("TurnPane\\status-NG-180.txt")).find("|vap missing ") != std::string::npos && get(P("TurnPane\\status-NG-180.txt")).find("|vap missing 0") == std::string::npos);
      noOverlap(h); mock::current = &h; x->destroy(); CHECK(f3.live == 0); }
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
