// test_dp232.cpp -- lsDeltaProfile 2.32.0 (2026-11-03 16:12 CT Rassul circled a gray ring on his ES chart: "get rid of the circles that
// relate to the delta profile"). DRAWING ONLY: while A records exist (D lines, SignalBoard A final, shared A SAB line, status counts),
// NO ellipse / circle (mock "ring" = drawOval) is painted by the Delta Profile: coloured same-candle A, next-candle live A, gray big node,
// shared A ring (+ its letter); decisions, records and restart behaviour unchanged; versions match.
// Build (from tests_dp217/): g++ -std=c++17 -O1 -w -fsanitize=address,undefined -I. -I../tests_delta -I.. test_dp232.cpp -o /tmp/t_dp232 && /tmp/t_dp232
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string flex;
static std::string home(const std::string& name) {
    const std::string root = "/tmp/dp232-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    flex = h + "/InvestorRT/rtx/lsFlexLevels/";
    return flex + "DeltaProfile/";
}
static int countPrefix(const std::string& text, const std::string& pre) { int n = 0; std::istringstream in(text); std::string l; while (std::getline(in, l)) if (l.compare(0, pre.size(), pre) == 0) ++n; return n; }
static std::vector<std::vector<std::string> > dRows(const std::string& text, const char* code) {
    std::vector<std::vector<std::string> > v; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() >= 22 && c[12] == "D" && c[2] == code) v.push_back(c); }
    return v;
}
static int rings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
static COLOR dimGray() { const COLOR a = 0x009CA3AF; COLOR o = 0; for (int sh = 0; sh < 24; sh += 8) { const float x = (float)((a >> sh) & 0xFF) * 0.45f; o |= ((COLOR)(x + 0.5f) & 0xFF) << sh; } return o; }
static int grayRings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring" && (d.color == 0x009CA3AF || d.color == dimGray())) ++n; return n; }
static int colourRings(const mock::Host& h) { return rings(h) - grayRings(h); }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; } mock::current = &h; p.draw(); }
static std::vector<sb::Ev> boardAll() { std::vector<sb::Ev> v; sb::readAll('A', "ESZ26", 180, "test", -1, v); return v; }
static void finish(mock::Host& h, DeltaProfile& p) { mock::current = &h; p.destroy(); latchBooks().clear(); }

static const unsigned long T0 = 1791820800UL + 18 * 3600;   // 18:00 (mock local = UTC): one session
static mock::Bar qb(int i) { mock::Bar b; b.time = T0 + static_cast<unsigned long>(i) * 180; b.open = 111; b.close = 111; b.low = 109; b.high = 113;
    b.rows = { mock::Price(109, 2, 1, 3), mock::Price(110, 2, 1, 3), mock::Price(111, 2, 1, 3), mock::Price(112, 2, 1, 3), mock::Price(113, 2, 1, 3) }; b.volume = 15; return b; }
// PK = candle 35 sells 500 at 105; bigRows = 3 big BUYING rows at 115 / 116 / 117 on candles 6 / 7 / 8 (they push 105 to the 4th row at PK)
static mock::Host chart(bool sameGreen, bool bigRows) {
    mock::Host h; h.seconds = 180; h.root = "ES"; h.tick = 0.25f; h.period = "3 min";
    for (int i = 0; i < 35; ++i) h.bars.push_back(qb(i));
    if (bigRows) for (int j = 0; j < 3; ++j) { mock::Bar& b = h.bars[6 + j]; b.high = 118; b.rows.push_back(mock::Price(115.0f + j, 2000, 0, 2000)); b.volume += 2000; }
    mock::Bar p = qb(35); p.low = 104; p.high = 111; p.open = sameGreen ? 105.5f : 108; p.close = sameGreen ? 107 : 106;
    p.rows.push_back(mock::Price(105, 0, 500, 500)); p.volume += 500; h.bars.push_back(p);
    return h;
}
static void addq(mock::Host& h) { h.bars.push_back(qb(static_cast<int>(h.bars.size()))); }
static std::string recDir;
static std::string recFile() {   // the one session record in the folder (the name carries the session key)
    std::string out; FILE* ls = popen(("ls " + recDir + "*-signals-*.csv 2>/dev/null | head -1").c_str(), "r"); if (!ls) return out;
    char b[1024]; if (fgets(b, sizeof b, ls)) { out = b; while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back(); } pclose(ls); return out; }
struct RecPath { operator std::string() const { return recFile(); } };
static std::string slurp(const RecPath& r) { return slurp(std::string(r)); }
static RecPath recPath(const std::string& dir) { recDir = dir; return RecPath(); }
static const int PK = 100;
// Prices are given in BULLISH coordinates; bear = true mirrors them (x -> 220 - x): a rising market into heavy BUYING at 120.
static bool bear = false;
static double M(double x) { return bear ? 220 - x : x; }
static void setBar(mock::Bar& b, double lo, double hi, double o, double c) {
    b.low = static_cast<float>(std::min(M(lo), M(hi))); b.high = static_cast<float>(std::max(M(lo), M(hi)));
    b.open = static_cast<float>(M(o)); b.close = static_cast<float>(M(c));
}
// the test_dp217 market: falling into PK, heavy selling at 100 on PK (bullish); PK OHLC given (lo 99, hi 104)
static mock::Host chart2(unsigned long t0, double pkOpen, double pkClose) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.tick = 0.25f;
    for (int i = 0; i <= PK; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
        for (int r = 120; r <= 124; ++r) b.rows.push_back(mock::Price(static_cast<float>(M(r)), 1, 0, 1));
        b.volume = 5; const double lo = 130 - 0.25 * i; setBar(b, lo, lo + 25, lo + 5, lo + 5); h.bars.push_back(b); }
    mock::Bar& p = h.bars[PK];
    p.rows.push_back(bear ? mock::Price(static_cast<float>(M(100)), 10000, 0, 10000) : mock::Price(100, 0, 10000, 10000)); p.volume += 10000;
    setBar(p, 99, 104, pkOpen, pkClose);
    return h;
}
static void add2(mock::Host& h, double lo, double hi, double o, double c) { mock::Bar b = h.bars.back(); b.time += 60;
    b.rows = { mock::Price(static_cast<float>(M(120)), 1, 0, 1) }; b.volume = 1; setBar(b, lo, hi, o, c); h.bars.push_back(b); }
static void add2(mock::Host& h, double lo, double hi) { add2(h, lo, hi, (lo + hi) / 2, (lo + hi) / 2); }
static void redraw2_unused(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; } mock::current = &h; p.draw(); }
static std::vector<sb::Ev> boardAll2() { std::vector<sb::Ev> v; sb::readAll('A', "ESZ26", 60, "test", -1, v); return v; }
static void finish2_unused(mock::Host& h, DeltaProfile& p) { mock::current = &h; p.destroy(); latchBooks().clear(); }
static const long long SESSION = 20735;
static std::string recPath2(const std::string& dir) { return dir + "ES-ESZ26-60-signals-" + std::to_string(SESSION) + ".csv"; }
// test_dp222.cpp -- lsDeltaProfile 2.22.0 (2026-10-22, Rassul 06:43 "then do it", 06:46 "only one circle per candle", 06:49 "then require it"):
//   [shared] the SHARED A decided here (no attacker pane on the chart): 5-candle low + sellers >= ES 3.0x their 20-candle average + GREEN
//            close -> one SAB line in SharedAbsorb/SAB-ES-60.csv, the letter A in a green ring, ONE circle on that candle (the node A rides
//            on it, its "A" label kept), SignalBoard A carries it; restart = the same, nothing written twice
//   [red]    the same attack on a RED candle: no shared A, no coloured circle (gray only), even after the next candle confirms the node A
//   [one]    two node A's on one candle (no shared A): ONE coloured circle (the biggest |delta|)
//   [adopt]  an attacker pane on the chart (fresh SignalBoard 'O'): this plugin never decides, it draws the record's line
// (2.28.0) the READ strip's "A" (inside the Delta column) is not a candle mark: only the letters left of the column count here
static int textA(const mock::Host& h, COLOR c) { const int colL = h.data ? static_cast<const ChartState*>(h.data)->lay.delta.l : 1 << 30; int n = 0;
    for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text == "A" && d.color == c && d.x < colL) ++n; return n; }
// circles on the candle at chart bar `bar` (its x from the mock's bar geometry: any ring whose centre is within half a bar of that x)
static int ringsOn(const mock::Host& h, int bar, bool colourOnly) { const int qh = 50 + (bar - h.barOffset) * 10; int n = 0; if (std::getenv("DBG")) { std::cerr << "qh " << qh << ":"; for (const mock::Draw& d : h.draws) if (d.type == "ring") std::cerr << " " << d.x << "/" << std::hex << d.color << std::dec; std::cerr << "\n"; }
    for (const mock::Draw& d : h.draws) if (d.type == "ring" && std::abs(d.x - qh) <= 4 && (!colourOnly || (d.color != 0x009CA3AF && d.color != dimGray()))) ++n; return n; }
static std::string sabPath() { return flex + "SharedAbsorb/SAB-ES-60.csv"; }
// prior candles with sellers ~100 lots each (so a 20-candle average exists); PK sells `pkSell`
static mock::Host chartS2(unsigned long t0, double pkOpen, double pkClose, long pkSell) {
    mock::Host h = chart2(t0, pkOpen, pkClose);
    for (int i = 0; i < PK; ++i) { h.bars[i].rows.push_back(mock::Price(static_cast<float>(M(125)), 0, 100, 100)); h.bars[i].volume += 100; }
    h.bars[PK].rows.push_back(mock::Price(static_cast<float>(M(99.5)), 0, pkSell, pkSell)); h.bars[PK].volume += pkSell;
    return h;
}

static int ovals(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
int main() {
    setenv("TZ", "UTC", 1); tzset();
    g_sabGraceMs = 0;
    // [ver]
    check(std::string(DLT_VERSION) == "2.32.0", "[ver] DLT_VERSION 2.32.0");
    check(std::string(dpset::PLUGIN_VERSION) == DLT_VERSION, "[ver] settings PLUGIN_VERSION = DLT_VERSION");
    check(slurp("../DeltaProfile.cpp").find(std::string("p->setVersion(\"") + DLT_VERSION + "\")") != std::string::npos, "[ver] setVersion literal = DLT_VERSION");
    check(!PAINT_CIRCLES, "[ver] PAINT_CIRCLES off");
    // 1) same-candle A: decided, recorded, posted - no circle
    {   const std::string dir = home("same"); const RecPath path = recPath(dir);
        mock::Host h = chart(true, false); addq(h);
        DeltaProfile p; redraw(h, p); redraw(h, p);
        const std::vector<std::vector<std::string> > d = dRows(slurp(path), "A");
        check(d.size() == 1 && d[0].size() == 29 && d[0][24] == "conf=same" && d[0][25] == "rule=research", "[same] the A is still decided and recorded (D line, 29 fields)");
        check(boardAll().size() == 1 && boardAll()[0].flag == 1, "[same] SignalBoard A final still posted");
        check(ovals(h) == 0, "[same] NO circle painted while the A record exists");
        check(state(h).bubblesDrawn + state(h).sabDrawn == 1 && state(h).circlesHidden == 1, "[same] status counts unchanged (one circle decided: the shared / node A), 1 not painted");
        const std::string st = slurp(flex + "DeltaProfile.status-ES-180.txt");
        check(st.find("CIRCLES,off") != std::string::npos && st.find("not painted 1") != std::string::npos && st.find("ONE_CIRCLE,shared A drawn 1") != std::string::npos && st.find("RESEARCHED_A,judged candles ") != std::string::npos, "[same] status: CIRCLES off line (not painted 1), ONE_CIRCLE count kept, RESEARCHED_A kept");
        for (int i = 0; i < 40; ++i) { addq(h); redraw(h, p); check(ovals(h) == 0, "[same] later candle " + std::to_string(i) + ": no circle"); }
        check(dRows(slurp(path), "A").size() == 1, "[same] 40 candles later: the record unchanged");
        const std::string before = slurp(path);
        finish(h, p); DeltaProfile p2; redraw(h, p2);
        check(ovals(h) == 0 && slurp(path) == before, "[same] restart: no circle, nothing rewritten");
        finish(h, p2); }
    // 2) next-candle A: live while forming (was a live ring), then recorded - never a circle
    {   const std::string dir = home("next"); const RecPath path = recPath(dir);
        mock::Host h = chart(false, false);
        mock::Bar n = qb(36); n.open = 106; n.close = 108; n.low = 105.5f; n.high = 109; h.bars.push_back(n);
        DeltaProfile p; redraw(h, p);
        check(ovals(h) == 0 && state(h).liveDrawn == 1, "[next] the live A is decided (liveDrawn 1) but not painted");
        addq(h); redraw(h, p); redraw(h, p);
        const std::vector<std::vector<std::string> > d = dRows(slurp(path), "A");
        check(d.size() == 1 && d[0][24] == "conf=next", "[next] D line conf=next still written");
        check(ovals(h) == 0, "[next] no circle after the confirmation");
        finish(h, p); }
    // 3) late (gray big node): gray decided, not painted
    {   const std::string dir = home("late"); const RecPath path = recPath(dir);
        mock::Host h = chart(true, true); addq(h);
        DeltaProfile p; redraw(h, p);
        for (int i = 0; i < 6; ++i) { addq(h); redraw(h, p); }
        check(!dRows(slurp(path), "AL").empty(), "[gray] the late A still logged as AL");
        check(ovals(h) == 0, "[gray] no gray circle painted (grayDrawn " + std::to_string(state(h).grayDrawn) + ")");
        { std::ofstream f((flex + "DeltaProfile.settings-ES.txt").c_str()); f << "SPEC1|DeltaProfile|2.23.0|ES\nwidth=110\nfont=10\ntrap_on=1\ndraw_late=1\n"; }
        state(h).settingsAt = -1; redraw(h, p);
        check(ovals(h) == 0 && state(h).circlesHidden >= 1, "[gray] Draw late A ticked: decided for the chart (not painted " + std::to_string(state(h).circlesHidden) + "), still no circle");
        finish(h, p); }
    // 4) the shared A (SAB record decided here): record + board unchanged, no ring and no ring letter
    {   home("shared"); const unsigned long t0 = 1791504000UL + 9 * 3600;
        mock::Host h = chartS2(t0, 100.5, 101, 600); add2(h, 100, 103);
        DeltaProfile p; redraw(h, p); redraw(h, p);
        const std::string rec = slurp(sabPath());
        check(countPrefix(rec, "SAB|") == 1, "[shared] one SAB line still written");
        check(state(h).sabDrawn == 1, "[shared] status shared A count unchanged (1)");
        check(ovals(h) == 0 && textA(h, C_SUPPORT) == 0, "[shared] no ring and no ring letter A on the candle");
        bool board = false; for (const sb::Ev& e : boardAll2()) if (e.side == 1 && e.flag == sb::A_FINAL) board = true;
        check(board, "[shared] SignalBoard A final still posted");
        finish(h, p); }
    std::cout << "dp 2.32.0 no circles: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
