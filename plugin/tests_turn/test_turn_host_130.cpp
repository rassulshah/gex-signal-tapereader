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
    std::ostringstream o; for (const mock::Draw& d : H.draws) if (d.t > H.paneTop + 21) o << d.type << "|" << d.l << "|" << d.t << "|" << d.r << "|" << d.b << "|" << std::hex << d.color << std::dec << "|" << d.text << "\n"; return o.str();
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
        CHECK(ab.find("ABS|1 Abs lines in the wy file|1 decided") == 0 && ab.find("read shows an Abs 1") != std::string::npos);
        CHECK(rd.find("READ|bullish|") == 0 && rd.find("Bullish ? - absorbed at ONL 7105.00") != std::string::npos && rd.find("Attack: Sellers hit ") != std::string::npos && rd.find("no order feed") == std::string::npos);
        CHECK(rd.find("Absorbed: Bids showing up to ") != std::string::npos && rd.find("Result: Volume ") != std::string::npos && rd.find("last minute sellers ") != std::string::npos && rd.find("Ex: ") != std::string::npos);
        CHECK(tb.compare(0, 13, "TAB|1|ES|180|") == 0 && tb.find("\n" + std::to_string(KA) + "|1|") != std::string::npos && tb.find("\n" + std::to_string(KA) + "|1|-1|") == std::string::npos); }
    CHECK(st.find("VERSION|1.5.0") != std::string::npos && std::string(TURN_VERSION) == "1.5.0" && std::string(turn::VERSION) == "1.5.0");
    CHECK(st.find("SHOW|no display file (the defaults)|absorption stop exhaustion initiative read|absorption coloured from 2.0\n") != std::string::npos);
    CHECK(rec0.find("TRN|1|ES|180\n") == 0 && countLines(rec0, "T|") == 2);                                 // the record format is unchanged: the two sweeps, decided once
    // ---- nothing says Effort / Answer; the strip of the last sweep names the four pieces ----
    for (const mock::Draw& d : es.H.draws) if (d.type == "text") CHECK(d.text.find("Effort") == std::string::npos && d.text.find("Answer") == std::string::npos);
    const std::string s0 = stripText(es.H); std::cout << "strip: " << s0 << " | " << pillText(es.H) << "\n";
    CHECK(s0.find("ONH 9:03 AM   Heavy Stop Run / ") == 0 && s0.find(" Absorption / ") != std::string::npos && s0.find(" Exhaustion") != std::string::npos && s0.find(" Initiative   \xBB ") != std::string::npos);
    noOverlap(es.H); CHECK(barLabels(es.H, 60) >= 20);
    // ---- every closed candle against the independent calculation ----
    struct Exp { bool have = false; long long mx = 0; bool away = false; bool known = false, on = false; std::string label; double v = 0; int cls = 0; };
    const size_t NB = es.H.bars.size(); std::vector<Exp> up(NB), dn(NB); std::vector<long long> lv(NB, 0);
    for (size_t i = 0; i + 1 < NB; ++i) { const CandleVap& c = es.vap[(long long)es.H.bars[i].time]; if (!c.hi || !c.lo) continue;
        const long long L = c.hi - c.lo + 1; lv[i] = L;
        if (L < 4) continue;
        long long z = (L + 2) / 4; if (z < 1) z = 1; long long bm = 0, sm = 0;
        for (const auto& r : c.rows) { if (r.first > c.hi - z && r.second.first > bm) bm = r.second.first; if (r.first < c.lo + z && r.second.second > sm) sm = r.second.second; }
        up[i].have = dn[i].have = true; up[i].mx = bm; dn[i].mx = sm; up[i].away = c.close <= c.hi - z; dn[i].away = c.close >= c.lo + z; }
    for (int side = 0; side < 2; ++side) { std::vector<Exp>& E = side ? dn : up;
        for (size_t i = 0; i + 1 < NB; ++i) { double sum = 0; int n = 0;
            if (!E[i].have) continue;
            for (size_t j = i; j-- > 0 && n < 20;) { if (lv[j] < 4) continue; sum += (double)E[j].mx; ++n; }
            if (n < 10 || !(sum > 0)) continue;
            const double ratio = (double)E[i].mx / (sum / n); const long long x1000 = (long long)std::llround(ratio * 1000.0);
            E[i].known = true; E[i].v = (double)x1000 / 1000.0; E[i].label = turn::labelText(E[i].v); const long long t10 = turn::shown10(E[i].label); E[i].on = E[i].away && t10 >= 20;
            E[i].cls = !E[i].away || t10 < 10 ? 0 : t10 >= 20 ? 3 : t10 < 15 ? 1 : 2; } }
    int axis = -1; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.b - d.t == 1 && d.r - d.l > 200 && d.t > 21) axis = d.t;
    CHECK(axis > 100); const int half = axis - 1 - (es.H.paneTop + 22) > 0 ? 125 : 125;
    const auto boxAt = [&](int l, bool u) -> const mock::Draw* { for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.l == l && d.r == l + 20 && (u ? d.b == axis : d.t == axis + 1)) return &d; return nullptr; };
    const auto labelOf = [&](const mock::Draw& b, bool u) -> std::string { for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t > 21 && (d.l + d.r) / 2 >= b.l + 6 && (d.l + d.r) / 2 <= b.r - 6 && (u ? d.b == b.t - 1 : d.t == b.b + 1)) return d.text; return std::string(); };
    int known = 0, coloured = 0, grey = 0, none = 0, heavyNotAway = 0; int hpx = -1;
    for (size_t i = 0; i + 1 < NB; ++i) { const long long key = (long long)es.H.bars[i].time; const int cx = es.H.barX0 + (int)i * es.H.barW;
        const mock::Draw *ru = boxAt(cx + 1, true), *rd = boxAt(cx + 1, false); const bool sweep = key == K851 || key == K903;
        if (hpx < 0 && ru && up[i].known && up[i].v > 0.2 && up[i].v < 3) hpx = (int)std::lround((ru->b - ru->t) / up[i].v * 3.0);   // the pane's scale, from the first bar
        const auto same = [&](const mock::Draw* b, const Exp& e, bool u, COLOR win) { if (!e.known) { CHECK(b == nullptr); ++none; return; } ++known; CHECK(b != nullptr); if (!b) return;
            CHECK(b->color == (e.on ? win : trh::C_AXIS)); CHECK(labelOf(*b, u) == e.label); if (e.on) ++coloured; else ++grey; if (!e.away && turn::shown10(e.label) >= 20) ++heavyNotAway; };
        same(ru, up[i], true, trh::C_GREEN);                                // (1.4.2) the top bar: green (the buyers at the offer), also on a sweep of a high
        if (sweep) { CHECK(rd != nullptr); if (rd) CHECK(isMarks(labelOf(*rd, false))); }   // the turn half of a sweep candle: the Initiative as marks
        else same(rd, dn[i], false, trh::C_RED); }   // (1.4.2) the bottom bar: red (the sellers at the bid)
    std::printf("Absorption bars: %d with a number (%d coloured, %d grey; %d read 2.0+ but closed in that quarter), %d candle sides with none yet\n", known, coloured, grey, heavyNotAway, none);
    CHECK(known >= 20 && coloured >= 1 && grey >= 10 && none >= 18); (void)half; (void)hpx;
    // heights: equal numbers -> equal heights, a bigger number is never shorter (the scale itself is 1.2.1's barPx)
    { std::vector<std::pair<double, int> > hs; for (size_t i = 0; i + 1 < NB; ++i) { const int cx = es.H.barX0 + (int)i * es.H.barW; if (const mock::Draw* b = boxAt(cx + 1, true)) if (up[i].known) hs.push_back(std::make_pair(up[i].v, b->b - b->t)); }
      std::sort(hs.begin(), hs.end()); for (size_t q = 1; q < hs.size(); ++q) CHECK(hs[q].second >= hs[q - 1].second); }
    // ---- the mouse on a plain candle below the strip: "<time>   High: <word> Absorption / Low: <word> Absorption" ----
    const std::string bars0 = barsOnly(es.H); int tried = 0;
    for (size_t i = 12; i + 1 < NB && tried < 6; ++i) { const long long key = (long long)es.H.bars[i].time; if (key == K851 || key == K903 || !up[i].known || !dn[i].known) continue;
        const std::string rec = get(P("TurnPane\\turn-ES-180.txt")); if (rec.find("C|" + std::to_string(key) + "|") != std::string::npos) continue;   // a candle at a named level has its own strip
        const int hx = es.H.barX0 + (int)i * es.H.barW; mouseAt(x, es.H, (long)i, hx, 250); cycle(x, es.H); cycle(x, es.H); const std::string s = stripText(es.H);
        const std::string want = std::string("High: ") + turn::word4(up[i].cls) + " Absorption / Low: " + turn::word4(dn[i].cls) + " Absorption";
        if (s.find(want) == std::string::npos) std::printf("strip '%s' wants '%s'\n", s.c_str(), want.c_str());
        CHECK(s.find(want) != std::string::npos && s.find(" AM   High: ") != std::string::npos); CHECK(barsOnly(es.H) == bars0); noOverlap(es.H); ++tried;
        for (const mock::Draw& d : es.H.draws) if (d.type == "text" && d.t == es.H.paneTop + 4) { if (d.text.find("High: ") == 0) CHECK(d.color == (up[i].on ? trh::C_RED : trh::C_MUTED)); if (d.text.find("Low: ") == 0) CHECK(d.color == (dn[i].on ? trh::C_GREEN : trh::C_MUTED)); } }
    CHECK(tried >= 3); turn_test_window = nullptr; cycle(x, es.H); cycle(x, es.H); CHECK(stripText(es.H) == s0 && barsOnly(es.H) == bars0);
    // ---- the chart removed and added again, no order events: the same bars and strip, the record untouched (no repaint) ----
    { const std::string before = dumpDraws(es.H); mock::current = &es.H; x->destroy(); static mock::Feed empty; es.H.feedp = &empty; for (int i = 0; i < 6; ++i) cycle(x, es.H);
      CHECK(dumpDraws(es.H) == before); CHECK(get(P("TurnPane\\turn-ES-180.txt")) == rec0); }
    // ---- the display file: Absorption only (one centred bar a side, the top line of the last closed candle); garbled = the defaults ----
    put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|0\nexhaustion|0\nanswer|0\nread|0\nEND\n"); for (int i = 0; i < 8; ++i) cycle(x, es.H);
    { const std::string s = stripText(es.H); std::cout << "Absorption only: " << s << "\n"; CHECK(s.find("   High: ") != std::string::npos && s.find(" / Low: ") != std::string::npos && s.find("Stop Run") == std::string::npos && pillText(es.H).empty());
      CHECK(get(P("TurnPane\\status-ES-180.txt")).find("SHOW|ok|absorption|absorption coloured from 2.0\n") != std::string::npos); noOverlap(es.H);
      int reds = 0, greens = 0, wrong = 0; for (const mock::Draw& d : es.H.draws) if (d.type == "box" && d.t > 21 && d.b - d.t > 1 && d.r - d.l <= 60) { if (d.color == trh::C_GREEN) { ++greens; if (d.b != axis) ++wrong; } else if (d.color == trh::C_RED) { ++reds; if (d.t != axis + 1) ++wrong; } else if (d.color != trh::C_AXIS) ++wrong; }
      CHECK(wrong == 0 && reds + greens == coloured + (up[(size_t)((K851 - K0) / 180)].on ? 0 : 0)); }                 // red only above the axis (highs), green only below (lows); the same candles count
    put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|0\nexhaustion|x\nEND\n"); for (int i = 0; i < 8; ++i) cycle(x, es.H);
    CHECK(get(P("TurnPane\\status-ES-180.txt")).find("SHOW|display line 3 garbled (the defaults)|absorption stop exhaustion initiative read|") != std::string::npos); CHECK(barsOnly(es.H) == bars0);
    std::remove(P("TurnPane\\display.txt").c_str()); for (int i = 0; i < 8; ++i) cycle(x, es.H); CHECK(barsOnly(es.H) == bars0 && stripText(es.H) == s0);
    { const std::string r = lineOf(get(P("TurnPane\\status-ES-180.txt")), "READ|"); std::cout << "(1.4.0) " << r << "\n"; CHECK(r.compare(0, 5, "READ|") == 0); CHECK(r.find("lines drawn ") != std::string::npos);
      std::cout << "(1.4.0) read lines seen right of the bars: " << g_readLines << "\n"; }
    mock::current = &es.H; x->destroy(); drop(x); (void)&drain; (void)&lineOf;
    std::printf("%d checks, %d failed\n", checks, fails); return fails ? 1 : 0;
}
