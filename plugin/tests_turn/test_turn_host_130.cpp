// test_turn_host_130.cpp -- lsTurn 1.3.0 on the mock SDK: the four pieces Stop Run / Absorption / Exhaustion / Initiative (the default),
// Friday's real ES order events 2026-11-20, 8:24 - 10:21 AM CT, candle by candle as a live morning.
//   g++ -std=c++17 -g -O1 -fsanitize=address,undefined -Wall -Wextra -Werror -DLSPERF_FAKE -Imock -I.. test_turn_host_130.cpp -o t_host130
//   TURN_TEST_DATA=<hostdata> TURN_EV=<ev_ES.txt to 9:50 AM or later> ./t_host130
// Covers: no display file = all four pieces, Absorption coloured from 2.0; the word Effort / Answer is drawn nowhere; EVERY closed candle's
// Absorption bars against an independent calculation from the candle's volume at price (the heaviest price in the end quarter / its
// average over the 20 candles before, 10 at least; the close test) - number, height, colour (red at the high, green at the low, grey when
// under 2.0 or closed in that quarter); a sweep candle: its Absorption at the swept end, its Initiative as marks; the top line of a plain
// candle under the mouse; nothing overlaps; removing and adding the chart again draws the same bars (no repaint) and leaves the record
// alone; the display file (Absorption only; garbled = the defaults); the status lines; the version.
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
static int g_readLines = 0;   // (1.4.0) read lines seen below the strip
static int barLabels(const mock::Host& H, int bw) {
    int n = 0;
    int maxBar = -1; for (const mock::Draw& b : H.draws) if (b.type == "box" && b.t > H.paneTop + 21 && b.b - b.t > 1 && b.r - b.l <= 20) maxBar = std::max(maxBar, b.r);
    for (const mock::Draw& d : H.draws) { if (d.type != "text" || d.t <= H.paneTop + 21) continue;
        if (maxBar >= 0 && d.l > maxBar + 8) { ++g_readLines; CHECK(d.fsize == 9); CHECK(d.r <= H.paneRight); continue; }   // (1.4.0) a line of the read: right of every bar
        CHECK(d.text.find(' ') == std::string::npos);                    // a read line never lands among the bars
        ++n;
        CHECK(isRatio(d.text) || isMarks(d.text)); CHECK(d.fsize == 9); CHECK(d.text.find('x') == std::string::npos);
        const int cx = (d.l + d.r) / 2;
        for (const mock::Draw& b : H.draws) { if (b.type != "box" || b.t <= H.paneTop + 21 || b.b - b.t <= 1 || b.r - b.l > 20) continue;
            const bool ov = d.l < b.r && b.l < d.r && d.t < b.b && b.t < d.b; if (!ov) continue;
            const int bc = (b.l + b.r) / 2; const bool own = bc >= cx - bw / 2 - 1 && bc <= cx + bw / 2 + 1; if (!own) std::printf("label '%s' [%d %d %d %d] over a foreign bar [%d %d %d %d]\n", d.text.c_str(), d.l, d.t, d.r, d.b, b.l, b.t, b.r, b.b); CHECK(own); } }
    return n;
}
static std::string barsOnly(const mock::Host& H) {                     // everything below the strip row: the bars, the axis and their labels
    int readX = 0; for (const mock::Draw& d : H.draws) if (d.type == "box" && d.t > H.paneTop + 21) readX = std::max(readX, d.r + 1);   // (1.7.0) the read right of the bars follows the mouse: not part of the bars
    std::ostringstream o; for (const mock::Draw& d : H.draws) if (d.t > H.paneTop + 21 && !(d.type == "text" && d.l >= readX)) o << d.type << "|" << d.l << "|" << d.t << "|" << d.r << "|" << d.b << "|" << std::hex << d.color << std::dec << "|" << d.text << "\n"; return o.str();
}
// (1.0.1) the cursor at pane point (h, v) of this chart's window, on chart bar `bar`; the event must never be swallowed
static void mouseAt(cppExtension* x, mock::Host& H, long bar, int h, int v, int type = E_MOUSE_MOVE) {
    mock::current = &H; turn_test_window = (void*)&H; pc::test_ok = true; pc::test_cx = h + 100; pc::test_cy = v + 50;
    RTX_EVENT e; std::memset(&e, 0, sizeof e); e.type = type; e.v.mouse.h = (short)h; e.v.mouse.v = (short)v; e.v.mouse.bar = bar;
    CHECK(x->mouse(&e) == RTX_FAIL);
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
static void drop(cppExtension* e) { delete static_cast<TurnPane*>(e); }   // IRT owns the extension object; the per-chart state was freed by destroy()
#pragma GCC diagnostic pop
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* tmp = std::getenv("TMPDIR"); root = std::string(tmp ? tmp : "/tmp") + "/turn_host130_u"; setenv("USERPROFILE", root.c_str(), 1);
    const char* td = std::getenv("TURN_TEST_DATA"); const char* evp = std::getenv("TURN_EV"); if (!td || !evp) { std::printf("set TURN_TEST_DATA and TURN_EV\n"); return 2; } data = td;
    { const std::string mk = "mkdir -p '" + root + "'"; if (std::system(mk.c_str())) return 3; }
    const long long K0 = 1795163040LL, KEND = 1795170060LL, K851 = 1795164660LL, K903 = 1795165380LL;   // bar ends 8:24 .. 10:21 AM, CT clock
    Sim es; es.tick = 0.25; es.k0 = K0; es.load(evp, K0 - 240, KEND); CHECK(es.ev.size() > 100000);
    es.H.feedp = &es.F; es.H.tick = 0.25f; es.H.seconds = 180; es.H.barW = 60; es.H.barX0 = 100; es.H.paneTop = 0; es.H.paneBottom = 300; es.H.paneRight = 100 + 60 * 44; es.H.textPx = 0.6; es.H.symbol = "ESZ6"; es.H.root = "ES";
    cppExtension* x = CreateExtension();
    es.chartTo(K0); cycle(x, es.H);                                       // the first cycle makes the folders
    for (const char* f : { "TurnPane\\turn-ES-180.txt", "TurnPane\\_opening-ES-180.txt", "TurnPane\\status-ES-180.txt", "TurnPane\\display.txt" }) std::remove(P(f).c_str());
    put(P("LRA-TurnPane-ES.csv"), get(data + "/LRA-Turn-ES.csv")); put(P("RevRead\\wy-ES-180.txt"), get(data + "/wy-ES.txt") + std::to_string(KEND - 180 * 5) + "|Abs|1|Low 20%|28420|ONL\n" + std::to_string(KEND - 180 * 4) + "|Ex|1\n");   // (1.5.0) an Abs of lsRevRead on a candle the feed sees whole
    mock::current = &es.H; x->destroy(); drop(x); x = CreateExtension();           // start clean: no record, no display file
    const auto drain1 = [&]() { for (int i = 0; i < 8000 && es.F.at < es.F.ev.size(); ++i) cycle(x, es.H); for (int i = 0; i < 3; ++i) cycle(x, es.H); };
    es.chartTo(K0); cycle(x, es.H);
    for (long long forming = K0; forming <= KEND - 180; forming += 180) { es.chartTo(forming); es.feedTo(forming); drain1(); }
    es.chartTo(KEND); es.feedTo(KEND - 150); drain1(); for (int i = 0; i < 6; ++i) cycle(x, es.H);
    const std::string st = get(P("TurnPane\\status-ES-180.txt")), rec0 = get(P("TurnPane\\turn-ES-180.txt"));
    {   // (1.5.0) the Abs read and the badge file, straight after the replay (the feed saw the Abs candle whole)
        const std::string ab = lineOf(st, "ABS|"), rd = lineOf(st, "READ|"), tb = get(P("RevRead\\turnabs-ES-180.txt")); const long long KA = KEND - 180 * 5;
        std::cout << "(1.5.0) " << ab << "\n(1.5.0) " << rd << "\n(1.5.0) turnabs: " << tb;
        CHECK(ab.find("ABS|1 Abs lines in the wy file|1 decided") == 0);
        // (1.7.0) the compact read: the last closed candle (no mouse) - its time, then quiet or who won + Attack + Absorbed / Not absorbed; never Result / Next / Ex
        CHECK(rd.find("READ|") == 0 && rd.find("Result:") == std::string::npos && rd.find("Next:") == std::string::npos && rd.find("Ex:") == std::string::npos);
        CHECK(rd.find("No absorption or initiative at the ends") != std::string::npos || ((rd.find("Passive defense won") != std::string::npos || rd.find("Aggressor won") != std::string::npos || rd.find("Initiative") != std::string::npos) && rd.find("Attack: ") == std::string::npos && rd.find(" ?") == std::string::npos));   // (1.11.0) the read concludes
        CHECK(tb.compare(0, 13, "TAB|1|ES|180|") == 0 && tb.find("\n" + std::to_string(KA) + "|1|") != std::string::npos && tb.find("\n" + std::to_string(KA) + "|1|-1|") == std::string::npos);
        // (1.6.0) every candle's Ex?: the turnexq file has the Abs candle, and its sellers' flag = the badge's Ex? (the same rule, the same candle)
        const std::string te = get(P("RevRead\\turnexq-ES-180.txt")); const std::string te1 = lineOf(st, "TEQ|"); std::cout << "(1.6.0) " << te1 << "\n";
        CHECK(te.compare(0, 13, "TEQ|1|ES|180|") == 0 && std::count(te.begin(), te.end(), '\n') >= 10 && te1.find("fails 0") != std::string::npos);
        { const size_t a = tb.find("\n" + std::to_string(KA) + "|1|"); const size_t b = te.find("\n" + std::to_string(KA) + "|"); CHECK(a != std::string::npos && b != std::string::npos);
          if (a != std::string::npos && b != std::string::npos) { const std::string la = tb.substr(a + 1, tb.find('\n', a + 1) - a - 1), lb = te.substr(b + 1, te.find('\n', b + 1) - b - 1);
            std::cout << "(1.6.0) abs " << la << " / exq " << lb << "\n"; CHECK(la.substr(la.size() - 1) == lb.substr(lb.find('|') + 1, 1)); } }
        std::map<long long, std::pair<int, int> > tq; CHECK(turn::parseTeqFile(te, "ES", 180, tq) && !tq.empty() && !turn::parseTeqFile(te, "NQ", 180, tq)); }
    {   // (1.11.0) the Volume read's file: kinds of the closed candles drawn + the order numbers of the candles the feed saw whole
        const std::string vf = get(P("TurnPane\\vread-ES-180.txt")); std::map<long long, tvr::Row> vm; const bool ok = tvr::parse(vf, "ES", 180, vm);
        int kinds = 0, nums = 0, lm = 0, ll = 0, sl = 0, pl = 0; for (const auto& kv : vm) { if (kv.second.kind != -9) ++kinds; if (kv.second.lm[0] >= 0) { ++nums; lm += (int)(kv.second.lm[0] + kv.second.lm[1]); } if (kv.second.ll[0] >= 0) ll += (int)(kv.second.ll[0] + kv.second.ll[1]); if (kv.second.slN[0] + kv.second.slN[1] > 0) ++sl; if (kv.second.pullN[0] > 0) ++pl; }
        std::cout << "(1.11.0) vread: " << vm.size() << " candles, kinds " << kinds << ", with order numbers " << nums << " (large market " << lm << ", large limit " << ll << ", sliced candles " << sl << ", pull normal known " << pl << ")\n";
        CHECK(ok && kinds >= 10 && lineOf(st, "TOVOL|").find("fails 0") != std::string::npos); CHECK(!tvr::parse(vf, "NQ", 180, vm)); }
    CHECK(st.find("VERSION|1.11.0") != std::string::npos && std::string(TURN_VERSION) == "1.11.0" && std::string(turn::VERSION) == "1.11.0");
    CHECK(st.find("SHOW|no display file (the defaults)|absorption stop exhaustion initiative read|absorption coloured from 2.0\n") != std::string::npos);
    CHECK(rec0.find("TRN|1|ES|180\n") == 0 && countLines(rec0, "T|") == 2);                                 // the record format is unchanged: the two sweeps, decided once
    // ---- nothing says Effort / Answer; the strip of the last sweep names the four pieces ----
    for (const mock::Draw& d : es.H.draws) if (d.type == "text") CHECK(d.text.find("Effort") == std::string::npos && d.text.find("Answer") == std::string::npos);
    const std::string s0 = stripText(es.H); std::cout << "strip: " << s0 << " | " << pillText(es.H) << "\n";
    CHECK(s0.find("ONH 9:03 AM   Heavy Stop Run / ") == 0 && s0.find(" Absorption / ") != std::string::npos && s0.find(" Exhaustion") != std::string::npos && s0.find(" Initiative   \xBB ") != std::string::npos);
    noOverlap(es.H); CHECK(barLabels(es.H, 60) >= 20);
    // ---- every closed candle against the independent calculation ----
    struct Exp { bool have = false; long long mx = 0, att = 0, x1000 = -1; bool away = false; bool known = false, on = false; std::string label; double v = 0; int cls = 0; };
    const size_t NB = es.H.bars.size(); std::vector<Exp> up(NB), dn(NB), iu(NB), idn(NB); std::vector<long long> lv(NB, 0);   // (1.8.0) iu / idn = Initiative at the top (sellers) / bottom (buyers)
    for (size_t i = 0; i + 1 < NB; ++i) { const CandleVap& c = es.vap[(long long)es.H.bars[i].time]; if (!c.hi || !c.lo) continue;
        const long long L = c.hi - c.lo + 1; lv[i] = L;
        if (L < 4) continue;
        long long z = (L + 2) / 4; if (z < 1) z = 1; long long bm = 0, sm = 0, ts = 0, bb = 0;
        for (const auto& r : c.rows) { if (r.first > c.hi - z && r.second.first > bm) bm = r.second.first; if (r.first < c.lo + z && r.second.second > sm) sm = r.second.second;
            if (r.first > c.hi - z && r.second.second > ts) { ts = r.second.second; }
            if (r.first < c.lo + z && r.second.first > bb) { bb = r.second.first; } }
        up[i].have = dn[i].have = iu[i].have = idn[i].have = true; up[i].mx = bm; dn[i].mx = sm; up[i].away = iu[i].away = c.close <= c.hi - z; dn[i].away = idn[i].away = c.close >= c.lo + z;
        iu[i].mx = ts; iu[i].att = bm; idn[i].mx = bb; idn[i].att = sm; }
    for (int side = 0; side < 4; ++side) { std::vector<Exp>& E = side == 0 ? up : side == 1 ? dn : side == 2 ? iu : idn;
        for (size_t i = 0; i + 1 < NB; ++i) { double sum = 0; int n = 0;
            if (!E[i].have) continue;
            for (size_t j = i; j-- > 0 && n < 20;) { if (lv[j] < 4) continue; sum += (double)E[j].mx; ++n; }
            if (n < 10 || !(sum > 0)) continue;
            const double ratio = (double)E[i].mx / (sum / n); const long long x1000 = (long long)std::llround(ratio * 1000.0);
            E[i].known = true; E[i].x1000 = x1000; E[i].v = (double)x1000 / 1000.0; E[i].label = turn::labelText(E[i].v); const long long t10 = turn::shown10(E[i].label); E[i].on = side < 2 ? E[i].away && t10 >= 20 : E[i].away && x1000 >= 1500 && E[i].mx > E[i].att && E[i].mx > 0;
            E[i].cls = !E[i].away || t10 < 10 ? 0 : t10 >= 20 ? 3 : t10 < 15 ? 1 : 2; } }
    int axis = -1; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.b - d.t == 1 && d.r - d.l > 200 && d.t > 21) axis = d.t;
    CHECK(axis > 100); 
    const auto boxAt = [&](int l, bool u) -> const mock::Draw* { for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.l == l && d.r == l + 20 && (u ? d.b == axis : d.t == axis + 1)) return &d; return nullptr; };
    (void)boxAt;
    const auto labelOf = [&](const mock::Draw& b, bool u) -> std::string { for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21 && (d.l + d.r) / 2 >= b.l + 6 && (d.l + d.r) / 2 <= b.r - 6 && (u ? d.b == b.t - 1 : d.t == b.b + 1)) return d.text; return std::string(); };
    // (1.8.0) TWO BARS A CANDLE: Absorption at its stronger end in the LEFT column (xl = centre - 1 - 20), Initiative at its stronger end in the RIGHT
    // one (xr = centre + 1); Absorption buyers-at-the-high green / sellers-at-the-low red, Initiative sellers-at-the-high red / buyers-at-the-low green
    const auto boxAtW = [&](int l, bool u) -> const mock::Draw* { for (const mock::Draw& d : es.H.draws) { if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.l == l && d.r == l + 20 && (u ? d.b == axis : d.t == axis + 1)
                                                                    && d.color != trh::C_LGREEN && d.color != trh::C_LRED) return &d; } return nullptr; };
    int known = 0, coloured = 0, grey = 0, none = 0, heavyNotAway = 0, iniKnown = 0, iniCol = 0;
    std::vector<std::pair<double, int> > hs; std::vector<long> iniIdx;
    for (size_t i = 0; i + 1 < NB; ++i) { const long long key = (long long)es.H.bars[i].time; const int cx = es.H.barX0 + (int)i * es.H.barW;
        if (key == K851 || key == K903) continue;                          // sweep candles keep their own layout
        // (1.10.0) the end that counts (coloured) wins, then the bigger number
        const bool abUp = (up[i].known && up[i].on) != (dn[i].known && dn[i].on) ? (up[i].known && up[i].on) : (up[i].known && up[i].x1000 >= (dn[i].known ? dn[i].x1000 : -1)); const Exp& A = abUp ? up[i] : dn[i];
        const bool inUp = (iu[i].known && iu[i].on) != (idn[i].known && idn[i].on) ? (iu[i].known && iu[i].on) : (iu[i].known && iu[i].x1000 > (idn[i].known ? idn[i].x1000 : -1)); const Exp& I = inUp ? iu[i] : idn[i];
        const mock::Draw* ba = boxAtW(cx - 21, abUp); const mock::Draw* bi = boxAtW(cx + 1, inUp);
        if (!A.known) { CHECK(boxAtW(cx - 21, true) == nullptr || boxAtW(cx - 21, false) == nullptr); ++none; }
        else { ++known; CHECK(ba != nullptr); if (ba) { CHECK(ba->color == (A.on ? (abUp ? trh::C_GREEN : trh::C_RED) : trh::C_AXIS)); CHECK(labelOf(*ba, abUp) == A.label || labelOf(*ba, abUp).empty());
                if (A.on) ++coloured; else ++grey; if (!A.away && turn::shown10(A.label) >= 20) ++heavyNotAway; if (abUp) hs.push_back(std::make_pair(A.v, ba->b - ba->t)); } }
        if (I.known) { ++iniKnown; CHECK(bi != nullptr); if (bi) { CHECK(bi->color == (I.on ? (inUp ? trh::C_RED : trh::C_GREEN) : trh::C_AXIS)); if (I.on) { ++iniCol; iniIdx.push_back((long)i); } } } }
    std::printf("Absorption bars: %d with a number (%d coloured, %d grey; %d read 2.0+ but closed in that quarter), %d candles with none yet; Initiative bars: %d (%d coloured)\n", known, coloured, grey, heavyNotAway, none, iniKnown, iniCol);
    CHECK(known >= 20 && grey >= 10 && none >= 9 && iniKnown >= 20);
    // heights: equal numbers -> equal heights, a bigger number is never shorter (the scale itself is 1.2.1's barPx)
    { std::sort(hs.begin(), hs.end()); for (size_t q = 1; q < hs.size(); ++q) CHECK(hs[q].second >= hs[q - 1].second); }
    // ---- the mouse on a plain candle below the strip: "<time>   High: <word> Absorption / Low: <word> Absorption" ----
    const std::string bars0 = barsOnly(es.H); int tried = 0;
    for (size_t i = 12; i + 1 < NB && tried < 6; ++i) { const long long key = (long long)es.H.bars[i].time; if (key == K851 || key == K903 || !up[i].known || !dn[i].known) continue;
        const std::string rec = get(P("TurnPane\\turn-ES-180.txt")); if (rec.find("C|" + std::to_string(key) + "|") != std::string::npos) continue;   // a candle at a named level has its own strip
        const int hx = es.H.barX0 + (int)i * es.H.barW; mouseAt(x, es.H, (long)i, hx, 250); cycle(x, es.H); cycle(x, es.H); const std::string s = stripText(es.H);
        const std::string want = std::string("High: ") + turn::word4(up[i].cls) + " Absorption / Low: " + turn::word4(dn[i].cls) + " Absorption";
        if (s.find(want) == std::string::npos) std::printf("strip '%s' wants '%s'\n", s.c_str(), want.c_str());
        CHECK(s.find(want) != std::string::npos && s.find(" AM   High: ") != std::string::npos); { const std::string b1 = barsOnly(es.H); if (b1 != bars0) { std::istringstream A(bars0), Bq(b1); std::string la, lb; int shown = 0; while (shown < 4) { const bool ga = (bool)std::getline(A, la), gb = (bool)std::getline(Bq, lb); if (!ga && !gb) break; if (la != lb) { std::printf("DIFF0 %s\nDIFF1 %s\n", la.c_str(), lb.c_str()); ++shown; } } } CHECK(b1 == bars0); } noOverlap(es.H); ++tried;
        for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t == es.H.paneTop + 4) { if (d.text.find("High: ") == 0) CHECK(d.color == (up[i].on ? trh::C_RED : trh::C_MUTED)); if (d.text.find("Low: ") == 0) CHECK(d.color == (dn[i].on ? trh::C_GREEN : trh::C_MUTED)); } }
    CHECK(tried >= 3);
    {   // (1.7.0) the read follows the mouse: on the Abs candle it reads that candle; numbers of a candle the feed saw whole; never Result / Next
        const long long KA = KEND - 180 * 5; long ia = -1; for (size_t i = 0; i < NB; ++i) if ((long long)es.H.bars[i].time == KA) ia = (long)i;
        CHECK(ia >= 0);
        if (ia >= 0) { mouseAt(x, es.H, ia, es.H.barX0 + (int)ia * es.H.barW, 250); cycle(x, es.H); cycle(x, es.H);
            const std::string r1 = lineOf(get(P("TurnPane\\status-ES-180.txt")), "READ|"); std::cout << "(1.7.0) hover Abs candle: " << r1 << "\n";
            CHECK(r1.find("Result:") == std::string::npos && r1.find("Next:") == std::string::npos);
            const bool q = r1.find("No absorption or initiative at the ends") != std::string::npos;
            CHECK(q || ((r1.find("Initiative") != std::string::npos || r1.find(" won") != std::string::npos) && r1.find("no order feed") == std::string::npos && r1.find("Price: ") == std::string::npos));
            if (r1.find("sellers absorbed at the bottom") != std::string::npos || r1.find("sellers got through the bottom") != std::string::npos) CHECK(r1.find(" at ONL - ") != std::string::npos); }
    }
    {   // (1.8.0) the mouse on a candle whose Initiative is on: the read is the Initiative read (Initiative / How / Against), its colour the winner's
        int seen = 0;
        for (long q : iniIdx) { mouseAt(x, es.H, q, es.H.barX0 + (int)q * es.H.barW, 250); cycle(x, es.H); cycle(x, es.H);
            const std::string r1 = lineOf(get(P("TurnPane\\status-ES-180.txt")), "READ|"); std::cout << "(1.8.0) hover Initiative candle: " << r1 << "\n";
            CHECK(r1.find("Initiative") != std::string::npos && r1.find("Initiative: ") == std::string::npos && r1.find("How: ") == std::string::npos && r1.find("Against: ") == std::string::npos && r1.find(" ?") == std::string::npos);
            CHECK(r1.find(" won the ") != std::string::npos); CHECK(r1.find("Price: ") == std::string::npos && r1.find("Did price agree") == std::string::npos);
            { int q3 = 0, q5 = 0, q6 = 0; for (const mock::Draw& d : es.H.draws) if (d.type == "text") { if (d.text == "3 Who won at the end?" || d.text == "3 Who won?") ++q3; if (d.text == "5 Tactics?") ++q5; if (d.text == "6 Did price agree?" || d.text == "6 Agreed?") ++q6; }
              std::cout << "(1.10.0) question tags drawn: 3 x" << q3 << " 5 x" << q5 << " 6 x" << q6 << "\n"; CHECK(q3 >= 1 && q6 == 0); }   // (1.11.0) no Price line; 5 Tactics? only when a tactic shows   // (1.10.0) 3 Who won / 5 How / 6 Did price agree
            ++seen; noOverlap(es.H); }   // (1.9.0) the Price line
        CHECK(seen >= 1); }
    turn_test_window = nullptr; cycle(x, es.H); cycle(x, es.H); CHECK(stripText(es.H) == s0 && barsOnly(es.H) == bars0);
    // ---- the chart removed and added again, no order events: the same bars and strip, the record untouched (no repaint) ----
    { const std::string before = dumpDraws(es.H); mock::current = &es.H; x->destroy(); static mock::Feed empty; es.H.feedp = &empty; for (int i = 0; i < 6; ++i) cycle(x, es.H);
      CHECK(dumpDraws(es.H) == before); CHECK(get(P("TurnPane\\turn-ES-180.txt")) == rec0); }
    // ---- the display file: Absorption only (one centred bar a side, the top line of the last closed candle); garbled = the defaults ----
    put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|0\nexhaustion|0\nanswer|0\nread|0\nEND\n"); for (int i = 0; i < 8; ++i) cycle(x, es.H);
    { const std::string s = stripText(es.H); std::cout << "Absorption only: " << s << "\n"; CHECK(s.find("   High: ") != std::string::npos && s.find(" / Low: ") != std::string::npos && s.find("Stop Run") == std::string::npos && pillText(es.H).empty());
      CHECK(get(P("TurnPane\\status-ES-180.txt")).find("SHOW|ok|absorption|absorption coloured from 2.0\n") != std::string::npos); noOverlap(es.H);
      int reds = 0, greens = 0, wrong = 0; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.r - d.l <= 60) { if (d.color == trh::C_GREEN) ++greens; else if (d.color == trh::C_RED) ++reds; else if (d.color != trh::C_AXIS && d.color != trh::C_LGREEN && d.color != trh::C_LRED) ++wrong; }
      CHECK(wrong == 0 && reds + greens >= coloured); }                 // (1.8.0) Absorption and Initiative both coloured green / red                 // red only above the axis (highs), green only below (lows); the same candles count
    put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|0\nexhaustion|x\nEND\n"); for (int i = 0; i < 8; ++i) cycle(x, es.H);
    CHECK(get(P("TurnPane\\status-ES-180.txt")).find("SHOW|display line 3 garbled (the defaults)|absorption stop exhaustion initiative read|") != std::string::npos); CHECK(barsOnly(es.H) == bars0);
    std::remove(P("TurnPane\\display.txt").c_str()); for (int i = 0; i < 8; ++i) cycle(x, es.H); CHECK(barsOnly(es.H) == bars0 && stripText(es.H) == s0);
    { const std::string r = lineOf(get(P("TurnPane\\status-ES-180.txt")), "READ|"); std::cout << "(1.4.0) " << r << "\n"; CHECK(r.compare(0, 5, "READ|") == 0); CHECK(r.find("lines drawn ") != std::string::npos);
      std::cout << "(1.4.0) read lines seen right of the bars: " << g_readLines << "\n"; }
    mock::current = &es.H; x->destroy(); drop(x); (void)&drain; (void)&lineOf;
    std::printf("%d checks, %d failed\n", checks, fails); return fails ? 1 : 0;
}
