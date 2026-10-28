// test_dp223.cpp -- lsDeltaProfile 2.23.0 / DeltaProfileCore 2.23.0 (2026-10-25, Iteration 6 parity fix: "make the drawn A the researched A")
//   [core] researchA: same-candle A, next-candle A, a peak 2 candles back is never an A, the 4th row is never an A, under 2.0x, doji /
//          wrong colour, close not beyond the row, the 6-candle pivot rule, the next candle's low vs the pivot bar's high, bearish mirror,
//          short window, rows cut from the window LOW (60 rows), same answer after the window slides on (pure function)
//   [host] the A is decided at its candle's (or the next candle's) close: D line 29 fields, code A, rule=research, delay_bars 0 / 1, late=0,
//          coloured ring (same) / gray ring (next: the 2.22.0 defenders'-close display rule), SignalBoard flag 1; an A the old path finds
//          later = code AL, rule=old, late=1, NOT drawn; ticking "Draw late A (not tested)" draws it; restart = identical rings, no line
//          written twice, no candle judged twice (RW watermark); settings: the new row, default off, an old (v16) settings file loads
//   [ver]  DLT_VERSION = setVersion literal = core header = settings version
// Build (from tests_dp217/): see run_tests.sh
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
    const std::string root = "/tmp/dp223-run", h = root + "/" + name; rmrf(h);
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

// ---------------- core ----------------
static dp::Bar mkb(int i, double o, double h, double l, double c, std::vector<dp::Row> rows) {
    dp::Bar b; b.index = i; b.time = 1000 + i * 180; b.closed = true; b.open = o; b.high = h; b.low = l; b.close = c; b.rows = rows; return b; }
// a quiet 30-candle window (prices in ticks, tick 1): rows 109 - 113 each +1 a candle; candle `pk` sells 500 at 105
static std::vector<dp::Bar> quiet(int n) {
    std::vector<dp::Bar> w; for (int i = 0; i < n; ++i) w.push_back(mkb(i, 111, 113, 109, 111, { dp::Row(109, 1, 3), dp::Row(110, 1, 3), dp::Row(111, 1, 3), dp::Row(112, 1, 3), dp::Row(113, 1, 3) })); return w; }
static void coreTests() {
    using namespace dp;
    {   std::vector<Bar> w = quiet(30); w[29] = mkb(29, 105.5, 111, 104, 107, { Row(105, -500, 500), Row(110, 1, 3) });
        std::vector<RaHit> h = researchA(w, 1.0);
        check(h.size() == 1 && h[0].sell && h[0].conf == CONF_SAME && h[0].pk == 29 && h[0].k == 29 && h[0].lo == 105 && h[0].hi == 105 && h[0].rank == 0, "[core] bullish same-candle A at the candle's close");
        check(h.size() == 1 && h[0].ratio > 2.0 && h[0].net == -500, "[core] ratio / net carried");
        std::vector<Bar> w2 = w; w2.push_back(mkb(30, 107, 112, 106, 111, { Row(111, 1, 3) })); w2.erase(w2.begin());
        check(researchA(w2, 1.0).empty(), "[core] the next candle never re-decides a same-candle A (peak k-1, own close was beyond)"); }
    {   std::vector<Bar> w = quiet(30); w[28] = mkb(28, 108, 111, 104, 106, { Row(105, -500, 500) });   // red: own close fails
        w[29] = mkb(29, 106, 109, 105.5, 108, { Row(110, 1, 3) });                                        // green above 105, low 105.5 < pivot high 111
        std::vector<RaHit> h = researchA(w, 1.0);
        check(h.size() == 1 && h[0].conf == CONF_NEXT && h[0].pk == 28 && h[0].k == 29, "[core] bullish next-candle A at the next candle's close");
        std::vector<Bar> w3 = w; w3[29].low = 111.5; w3[29].open = 112; w3[29].close = 113; w3[29].high = 113;
        check(researchA(w3, 1.0).empty(), "[core] next candle's low not below the pivot bar's high = no A");
        std::vector<Bar> w4 = w; w4[29].close = 105; w4[29].open = 104.5;
        check(researchA(w4, 1.0).empty(), "[core] next candle closes ON the row (not beyond) = no A");
        std::vector<Bar> w5 = w; w5.erase(w5.begin()); w5.push_back(mkb(30, 108, 110, 107, 109, { Row(111, 1, 3) }));
        check(researchA(w5, 1.0).empty(), "[core] a peak 2 candles back is never an A (the old late path)"); }
    {   std::vector<Bar> w = quiet(30); w[29] = mkb(29, 107, 111, 104, 107, { Row(105, -500, 500) });
        check(researchA(w, 1.0).empty(), "[core] doji = no A");
        w[29] = mkb(29, 105.5, 111, 104, 105, { Row(105, -500, 500) });
        check(researchA(w, 1.0).empty(), "[core] close at the row's top tick is not beyond"); }
    {   std::vector<Bar> w = quiet(30); w[24].low = 100; w[24].high = 103;                             // a lower low 5 candles back
        w[29] = mkb(29, 105.5, 111, 104, 107, { Row(105, -500, 500) });
        std::vector<RaHit> h = researchA(w, 1.0);
        check(h.empty(), "[core] pivot rule: candle low 104 not below the pivot bar's high 103 (pivot = the 100 low, 5 back) = no A");
        check(researchA(w, 1.0, 60, 3, 2.0, false).size() == 1, "[core] the same case without the pivot rule = A (the rule is what blocks it)");
        w[24].high = 104.5; check(researchA(w, 1.0).size() == 1, "[core] pivot rule: low 104 below the pivot bar's high 104.5 = A");
        w[23].low = 99; w[23].high = 120; check(researchA(w, 1.0).size() == 1, "[core] a lower low 6 candles back is outside [peak-5, k]"); }
    {   std::vector<Bar> w = quiet(30); w[2].rows = { Row(140, 600, 600) }; w[2].high = 140; w[3].rows = { Row(141, 550, 550) }; w[3].high = 141;
        w[4].rows = { Row(142, 520, 520) }; w[4].high = 142;
        w[29] = mkb(29, 105.5, 111, 104, 107, { Row(105, -500, 500) });
        check(researchA(w, 1.0).empty(), "[core] the 4th biggest row is never an A");
        w[4].rows = { Row(142, 1, 1) };
        check(researchA(w, 1.0).size() == 1, "[core] ... and is the 3rd once one bigger row leaves"); }
    {   std::vector<Bar> w = quiet(30); for (Bar& b : w) b.rows = { Row(109, 30, 30), Row(110, 30, 30), Row(111, 30, 30), Row(112, 30, 30), Row(113, 30, 30) };
        w[29] = mkb(29, 105.5, 111, 104, 107, { Row(105, -100, 100) });
        check(researchA(w, 1.0).empty(), "[core] under 2.0x the average row = no A"); }
    {   std::vector<Bar> w = quiet(30); for (Bar& b : w) { b.open = 220 - b.open; b.close = 220 - b.close; const double hh = 220 - b.low; b.low = 220 - b.high; b.high = hh;
            for (Row& r : b.rows) r.key = 220 - r.key; std::reverse(b.rows.begin(), b.rows.end()); }
        w[29] = mkb(29, 114.2, 116, 109, 113, { Row(115, 500, 500) });   // (2.27.0) open in ticks 114: the row above the body
        std::vector<RaHit> h = researchA(w, 1.0);
        check(h.size() == 1 && !h[0].sell && h[0].conf == CONF_SAME && h[0].lo == 115, "[core] bearish mirror: buying row, red close below it"); }
    {   std::vector<Bar> w = quiet(6); w[5] = mkb(5, 105.5, 111, 104, 107, { Row(105, -500, 500) });
        check(researchA(w, 1.0).empty(), "[core] a window under 7 candles = no A"); }
    {   // rows cut from the window LOW: span 0..119 ticks -> 2 ticks a row starting at the low 1 (rows [1,2] [3,4] ...); 105 and 106 share a row
        std::vector<Bar> w = quiet(30); w[0].low = 1; w[0].rows.push_back(Row(1, 1, 1)); w[1].high = 120; w[1].rows.push_back(Row(120, 1, 1));
        w[29] = mkb(29, 107, 111, 104, 108, { Row(105, -300, 300), Row(106, -300, 300) });   // (2.27.0) open 107: the row below the body
        std::vector<RaHit> h = researchA(w, 1.0);
        check(h.size() == 1 && h[0].lo == 105 && h[0].hi == 106, "[core] 60 rows cut from the window's lowest price: 105-106 is one row"); }
    // (2.27.0, Rassul 15:04 CT) the open rule (node at / below the open, bullish) and the halfway rule (node middle at / below (H+L)/2)
    {   std::vector<Bar> w = quiet(30); w[29] = mkb(29, 104.2, 111, 103, 107, { Row(105, -500, 500) });   // green, open 104: node 105 above the open
        check(researchA(w, 1.0).empty(), "[open] bullish node above the open = no A");
        check(researchA(w, 1.0, 60, 3, 2.0, true, false).size() == 1, "[open] the same without the rules = A (the rule is what blocks it)");
        w[29] = mkb(29, 105.2, 111, 103, 107, { Row(105, -500, 500) });
        check(researchA(w, 1.0).size() == 1, "[open] node AT the open (105) = A (at or below)");
        w[29] = mkb(29, 106, 111, 103, 107, { Row(105, -500, 500) });
        check(researchA(w, 1.0).size() == 1, "[open] node below the open 106 = A (the close no longer matters for it)"); }
    {   std::vector<Bar> w = quiet(30); w[29] = mkb(29, 110.2, 112, 104, 111, { Row(109, -500, 500) });   // node 109 below open 110, but (112+104)/2 = 108 < 109
        check(researchA(w, 1.0).empty(), "[half] bullish node above the halfway mark = no A");
        w[29] = mkb(29, 110.2, 114, 104, 111, { Row(109, -500, 500) });                                    // halfway 109: at it
        check(researchA(w, 1.0).size() == 1, "[half] node at the halfway mark = A"); }
    {   std::vector<Bar> w = quiet(30); w[24].low = 100; w[24].high = 104;                                // pivot bar high 104 = the candle's low 104
        w[29] = mkb(29, 105.5, 111, 104, 107, { Row(105, -500, 500) });
        check(researchA(w, 1.0).empty(), "[pivot] low AT the down-pivot bar's high = no A (strictly below, 15:07)");
        w[24].high = 105; check(researchA(w, 1.0).size() == 1, "[pivot] low below the down-pivot bar's high = A"); }
}

// ---------------- host ----------------
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
static void hostTests() {
    // 1) same-candle A, decided at its close, coloured ring, D line rule=research delay 0
    {   const std::string dir = home("same"); const RecPath path = recPath(dir);
        mock::Host h = chart(true, false); addq(h);                                    // candle 36 forming: 35 closed
        DeltaProfile p; redraw(h, p); redraw(h, p);                                     // (the journal is appended on the next pass)
        const std::vector<std::vector<std::string> > d = dRows(slurp(path), "A");
        check(d.size() == 1 && d[0].size() == 29, "[host] same: one D line, 29 fields");
        check(d.size() == 1 && d[0][24] == "conf=same" && d[0][25] == "rule=research" && d[0][26] == "delay_bars=0" && d[0][27] == "delay_min=0" && d[0][28] == "late=0", "[host] same: conf=same rule=research delay 0 late=0");
        check(d.size() == 1 && d[0][15] == d[0][10], "[host] same: decided_by_t = peak_t (its own close)");
        check(colourRings(h) == 1, "[host] same: one coloured ring at the candle's close");
        check(boardAll().size() == 1 && boardAll()[0].flag == 1, "[host] same: SignalBoard final");
        check(countPrefix(slurp(path), "TA|") == 0, "[host] no tentative lines");
        const std::string st = slurp(flex + "DeltaProfile.status-ES-180.txt");
        check(st.find("RESEARCHED_A,judged candles ") != std::string::npos && st.find(",A 1,") != std::string::npos, "[host] status RESEARCHED_A");
        for (int i = 0; i < 40; ++i) { addq(h); redraw(h, p); }
        check(colourRings(h) == 1 && dRows(slurp(path), "A").size() == 1 && dRows(slurp(path), "AL").empty(), "[host] 40 candles later: same ring, no new A / AL line (never repaints)");
        const std::string before = slurp(path); const int ringsBefore = rings(h);
        finish(h, p);
        DeltaProfile p2; redraw(h, p2);
        check(rings(h) == ringsBefore && colourRings(h) == 1, "[host] restart: identical rings");
        check(slurp(path) == before, "[host] restart: nothing written (no candle judged twice, no line twice)");
        addq(h); redraw(h, p2);
        check(countPrefix(slurp(path), "RW|") == countPrefix(before, "RW|") + 1, "[host] after the restart only the NEW candle is judged (one RW line)");
        finish(h, p2); }
    // 2) next-candle A: decided at the next candle's close, delay 1; (2.23.1) coloured by its confirmation candle's close
    {   const std::string dir = home("next"); const RecPath path = recPath(dir);
        mock::Host h = chart(false, false);
        mock::Bar n = qb(36); n.open = 106; n.close = 108; n.low = 105.5f; n.high = 109; h.bars.push_back(n);   // 36 forming
        DeltaProfile p; redraw(h, p);
        check(dRows(slurp(path), "A").empty() && colourRings(h) == 1, "[host] next: no record line while the next candle forms, but the LIVE A shows (2.27.0, 15:16 live then confirm)");
        { mock::Bar& f = h.bars.back(); const float o0 = f.open, c0 = f.close; f.close = 104.5f; f.open = 106; redraw(h, p);
          check(dRows(slurp(path), "A").empty() && colourRings(h) == 0, "[host] next: the live A vanishes when the forming candle breaks the rule"); f.open = o0; f.close = c0; redraw(h, p); }
        addq(h); redraw(h, p); redraw(h, p);                                            // 36 closed
        const std::vector<std::vector<std::string> > d = dRows(slurp(path), "A");
        check(d.size() == 1 && d[0].size() == 29 && d[0][24] == "conf=next" && d[0][26] == "delay_bars=1" && d[0][27] == "delay_min=3" && d[0][28] == "late=0", "[host] next: D line conf=next delay 1 candle / 3 min");
        check(colourRings(h) == 1 && grayRings(h) == 0, "[host] next: (2.23.1) the circle is GREEN - its confirmation candle (36) closed green, like every A");
        finish(h, p); }
    // 3) late: the row is only the 4th at its candle's close, reaches the top 3 later -> old path: AL, rule=old, late=1, not drawn
    {   const std::string dir = home("late"); const RecPath path = recPath(dir);
        mock::Host h = chart(true, true); addq(h);
        DeltaProfile p; redraw(h, p);
        check(dRows(slurp(path), "A").empty() && colourRings(h) == 0, "[host] late: the 4th row at the close = no A");
        for (int i = 0; i < 6; ++i) { addq(h); redraw(h, p); }
        const std::vector<std::vector<std::string> > al = dRows(slurp(path), "AL");
        check(!al.empty() && al[0].size() == 29 && al[0][25] == "rule=old" && al[0][28] == "late=1", "[host] late: logged as AL rule=old late=1");
        long long db = -1; if (!al.empty()) db = std::atoll(al[0][26].substr(11).c_str());
        check(db > 1, "[host] late: delay_bars > 1 (" + std::to_string(db) + ")");
        check(dRows(slurp(path), "A").empty() && colourRings(h) == 0 && boardAll().empty(), "[host] late: not drawn, not on the SignalBoard");
        const int g0 = grayRings(h);
        // the setting: Draw late A (not tested)
        { std::ofstream f((flex + "DeltaProfile.settings-ES.txt").c_str()); f << "SPEC1|DeltaProfile|2.23.0|ES\nwidth=110\nfont=10\ntrap_on=1\ndraw_late=1\n"; }
        state(h).settingsAt = -1; redraw(h, p);
        check(colourRings(h) == 1, "[host] Draw late A ticked: the late A is drawn");
        check(boardAll().empty(), "[host] Draw late A ticked: the late A is still never posted on the SignalBoard");
        { std::ofstream f((flex + "DeltaProfile.settings-ES.txt").c_str()); f << "SPEC1|DeltaProfile|2.22.0|ES\nwidth=110\nfont=10\ntrap_on=1\n"; }   // an old (v16) file
        state(h).settingsAt = -1; redraw(h, p);
        check(colourRings(h) == 0 && grayRings(h) == g0, "[host] an old settings file (no draw_late key) loads: default off, the late A back to gray");
        const std::string before = slurp(path);
        finish(h, p); DeltaProfile p2; redraw(h, p2);
        check(colourRings(h) == 0 && slurp(path) == before, "[host] late: restart draws nothing new, writes nothing");
        finish(h, p2); }
}
static void settingsTests() {
    const ss::Spec& S = dpset::spec();
    check(S.paramVersion == 17 && std::string(S.version) >= "2.28.0", "[set] parameter version 17, spec version 2.27.0");
    const int k = ss::find(S, "draw_late");
    check(k == dpset::K_DRAW_LATE && S.rows[k].type == ss::T_BOOL && S.rows[k].def == 0 && !S.rows[k].hidden && std::string(S.rows[k].label) == "Draw late A (not tested)", "[set] Draw late A (not tested): visible checkbox, default off");
}
int main() {
    g_sabGraceMs = 1LL << 60;   // the shared A (SharedAbsorb.h) never decides here: every ring below is a node A of this plugin
    coreTests(); hostTests(); settingsTests();
    check(std::string(DLT_VERSION) >= "2.28.0" && std::string(dpset::PLUGIN_VERSION) == DLT_VERSION, "[ver] DLT_VERSION = settings version");
    check(slurp("../DeltaProfile.cpp").find(std::string("p->setVersion(\"") + DLT_VERSION + "\")") != std::string::npos, "[ver] setVersion literal");
    check(slurp("../DeltaProfileCore.h").find("DeltaProfileCore.h 2.28.0") != std::string::npos, "[ver] core header");
    std::cout << "dp 2.23.1 researched A: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
