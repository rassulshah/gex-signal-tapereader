// test_dp2321.cpp -- lsDeltaProfile 2.32.1 (2026-11-03 night, Rassul "fix all" - his 7:48 PM CT ES screenshot: the profile labels "3.9x"
// and "3x" sat 11 px apart because they did not overlap side by side). The 3 ratio labels keep at least one text row apart vertically:
// three big rows on neighbouring prices -> every pair of drawn labels is >= one label height apart (or the weaker is stepped / dropped);
// the strongest keeps its own row; versions 2.32.1 agree; no circles still.
// Build (from tests_dp217/): g++ -std=c++17 -O1 -w -fsanitize=address,undefined -I. -I../tests_delta -I.. test_dp2321.cpp -o /tmp/t_dp2321 && /tmp/t_dp2321
// (helpers from test_dp232.cpp) lsDeltaProfile 2.32.0 (2026-11-03 16:12 CT Rassul circled a gray ring on his ES chart: "get rid of the circles that
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

static std::vector<mock::Draw> labels(const mock::Host& h) { std::vector<mock::Draw> v; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text.size() >= 2 && d.text.back() == 'x' && (std::isdigit((unsigned char)d.text[0]))) v.push_back(d); return v; }
int main() {
    setenv("TZ", "UTC", 1); tzset();
    g_sabGraceMs = 0;
    check(std::string(DLT_VERSION) == "2.32.1", "[ver] DLT_VERSION 2.32.1");
    check(std::string(dpset::PLUGIN_VERSION) == DLT_VERSION, "[ver] settings PLUGIN_VERSION = DLT_VERSION");
    check(slurp("../DeltaProfile.cpp").find(std::string("p->setVersion(\"") + DLT_VERSION + "\")") != std::string::npos, "[ver] setVersion literal = DLT_VERSION");
    {   // his 7:48 PM ES screenshot geometry: "3.9x" l 603 r 627 centred y 332, then "3x" l 629 r 644 at its row y 321 (font 10: h 13)
        const int h = 13;
        check(ratioLabelClash(629, 644, 321, h, 603, 332 - h / 2, 627, 332 + h / 2), "[7a] 3x 11 px above 3.9x (not side by side): a clash now -> stepped a row / dropped");
        check(!ratioLabelClash(629, 644, 321 - 13 - 2, h, 603, 332 - h / 2, 627, 332 + h / 2), "[7a] one text row + 2 px higher: no clash (where it is drawn, with its leader line)");
        check(!ratioLabelClash(629, 644, 360, h, 603, 332 - h / 2, 627, 332 + h / 2), "[7a] 2.6x 28 px below: no clash");
        check(ratioLabelClash(603, 627, 334, h, 603, 332 - h / 2, 627, 332 + h / 2), "[7a] overlapping side by side: still a clash");
    }
    int scenes = 0;
    for (int gap = 1; gap <= 3; ++gap) {   // big rows 1, 2, 3 ticks apart: their labels would sit 1-3 rows of the profile apart
        const std::string dir = home("gap" + std::to_string(gap));
        mock::Host h = chart(false, false);
        const float base = 115.0f; const long dl[3] = { 3000, 700, 650 };   // the 2nd / 3rd bars much shorter: their labels sit right of the 1st one's (no side-by-side overlap, as on his chart)
        for (int j = 0; j < 3; ++j) { mock::Bar& b = h.bars[6 + j]; b.high = 118; b.rows.push_back(mock::Price(base + 0.25f * gap * j, dl[j], 0, dl[j])); b.volume += dl[j]; }
        addq(h);
        DeltaProfile p; mock::current = &h; p.init(); redraw(h, p); redraw(h, p);
        const std::vector<mock::Draw> L = labels(h);
        std::cerr << "gap " << gap << ":"; for (const mock::Draw& d : L) std::cerr << " '" << d.text << "' x " << d.x << " y " << d.y; std::cerr << "\n";
        const int hgt = h.data ? state(h).layout.font + 3 : 13;
        int close = 0; for (size_t i = 0; i < L.size(); ++i) for (size_t j = i + 1; j < L.size(); ++j) if (std::abs(L[i].y - L[j].y) < hgt) ++close;
        check(!L.empty(), "gap " + std::to_string(gap) + ": the profile labels are drawn (" + std::to_string(L.size()) + ")");
        check(close == 0, "gap " + std::to_string(gap) + ": every two ratio labels at least one text row (" + std::to_string(hgt) + " px) apart (" + std::to_string(close) + " too close)");
        check(ovals(h) == 0, "gap " + std::to_string(gap) + ": still no circles");
        ++scenes; finish(h, p);
    }
    std::cout << "dp " << DLT_VERSION << " label row gap: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
