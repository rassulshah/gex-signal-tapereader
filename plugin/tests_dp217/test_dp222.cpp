// see the header comment below
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
    const std::string root = "/tmp/dp218-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    flex = h + "/InvestorRT/rtx/lsFlexLevels/";
    return flex + "DeltaProfile/";
}
static int countPrefix(const std::string& text, const std::string& pre) {
    int n = 0; std::istringstream in(text); std::string l; while (std::getline(in, l)) if (l.compare(0, pre.size(), pre) == 0) ++n; return n;
}
static std::vector<std::vector<std::string> > dRows(const std::string& text) {
    std::vector<std::vector<std::string> > v; std::istringstream in(text); std::string l;
    while (std::getline(in, l)) { std::vector<std::string> c = fields(l); if (c.size() >= 22 && c[12] == "D" && c[2] == "A") v.push_back(c); }
    return v;
}
static std::string taConf(const std::string& text, const char* pre) {   // the conf field of the first TA line with this prefix
    std::istringstream in(text); std::string l;
    while (std::getline(in, l)) if (l.compare(0, std::strlen(pre), pre) == 0) { std::vector<std::string> c = fields(l); return c.size() == 14 ? c[13] : "?"; }
    return "";
}
static int rings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring") ++n; return n; }
// (2.19.0) the gray circle (full or dimmed away from a zone) vs the coloured A circles
static COLOR dimGray() { const COLOR a = 0x009CA3AF; COLOR o = 0; for (int sh = 0; sh < 24; sh += 8) { const float x = (float)((a >> sh) & 0xFF) * 0.45f; o |= ((COLOR)(x + 0.5f) & 0xFF) << sh; } return o; }
static int grayRings(const mock::Host& h) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "ring" && (d.color == 0x009CA3AF || d.color == dimGray())) ++n; return n; }
static int colourRings(const mock::Host& h) { return rings(h) - grayRings(h); }
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static const int PK = 100;
// Prices are given in BULLISH coordinates; bear = true mirrors them (x -> 220 - x): a rising market into heavy BUYING at 120.
static bool bear = false;
static double M(double x) { return bear ? 220 - x : x; }
static void setBar(mock::Bar& b, double lo, double hi, double o, double c) {
    b.low = static_cast<float>(std::min(M(lo), M(hi))); b.high = static_cast<float>(std::max(M(lo), M(hi)));
    b.open = static_cast<float>(M(o)); b.close = static_cast<float>(M(c));
}
// the test_dp217 market: falling into PK, heavy selling at 100 on PK (bullish); PK OHLC given (lo 99, hi 104)
static mock::Host chart(unsigned long t0, double pkOpen, double pkClose) {
    mock::Host h; h.seconds = 60; h.root = "ES"; h.tick = 0.25f;
    for (int i = 0; i <= PK; ++i) { mock::Bar b; b.time = t0 + static_cast<unsigned long>(i * 60);
        for (int r = 120; r <= 124; ++r) b.rows.push_back(mock::Price(static_cast<float>(M(r)), 1, 0, 1));
        b.volume = 5; const double lo = 130 - 0.25 * i; setBar(b, lo, lo + 25, lo + 5, lo + 5); h.bars.push_back(b); }
    mock::Bar& p = h.bars[PK];
    p.rows.push_back(bear ? mock::Price(static_cast<float>(M(100)), 10000, 0, 10000) : mock::Price(100, 0, 10000, 10000)); p.volume += 10000;
    setBar(p, 99, 104, pkOpen, pkClose);
    return h;
}
static void add(mock::Host& h, double lo, double hi, double o, double c) { mock::Bar b = h.bars.back(); b.time += 60;
    b.rows = { mock::Price(static_cast<float>(M(120)), 1, 0, 1) }; b.volume = 1; setBar(b, lo, hi, o, c); h.bars.push_back(b); }
static void add(mock::Host& h, double lo, double hi) { add(h, lo, hi, (lo + hi) / 2, (lo + hi) / 2); }
static void redraw(mock::Host& h, DeltaProfile& p) { h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; state(h).statusAt = -1; } mock::current = &h; p.draw(); }
static std::vector<sb::Ev> boardAll() { std::vector<sb::Ev> v; sb::readAll('A', "ESZ26", 60, "test", -1, v); return v; }
static void finish(mock::Host& h, DeltaProfile& p) { mock::current = &h; p.destroy(); latchBooks().clear(); }
static const long long SESSION = 20735;
static std::string recPath(const std::string& dir) { return dir + "ES-ESZ26-60-signals-" + std::to_string(SESSION) + ".csv"; }
// test_dp222.cpp -- lsDeltaProfile 2.22.0 (2026-10-22, Rassul 06:43 "then do it", 06:46 "only one circle per candle", 06:49 "then require it"):
//   [shared] the SHARED A decided here (no attacker pane on the chart): 5-candle low + sellers >= ES 3.0x their 20-candle average + GREEN
//            close -> one SAB line in SharedAbsorb/SAB-ES-60.csv, the letter A in a green ring, ONE circle on that candle (the node A rides
//            on it, its "A" label kept), SignalBoard A carries it; restart = the same, nothing written twice
//   [red]    the same attack on a RED candle: no shared A, no coloured circle (gray only), even after the next candle confirms the node A
//   [one]    two node A's on one candle (no shared A): ONE coloured circle (the biggest |delta|)
//   [adopt]  an attacker pane on the chart (fresh SignalBoard 'O'): this plugin never decides, it draws the record's line
static int textA(const mock::Host& h, COLOR c) { int n = 0; for (const mock::Draw& d : h.draws) if (d.type == "text" && d.text == "A" && d.color == c) ++n; return n; }
// circles on the candle at chart bar `bar` (its x from the mock's bar geometry: any ring whose centre is within half a bar of that x)
static int ringsOn(const mock::Host& h, int bar, bool colourOnly) { const int qh = 50 + (bar - h.barOffset) * 10; int n = 0; if (std::getenv("DBG")) { std::cerr << "qh " << qh << ":"; for (const mock::Draw& d : h.draws) if (d.type == "ring") std::cerr << " " << d.x << "/" << std::hex << d.color << std::dec; std::cerr << "\n"; }
    for (const mock::Draw& d : h.draws) if (d.type == "ring" && std::abs(d.x - qh) <= 4 && (!colourOnly || (d.color != 0x009CA3AF && d.color != dimGray()))) ++n; return n; }
static std::string sabPath() { return flex + "SharedAbsorb/SAB-ES-60.csv"; }
// prior candles with sellers ~100 lots each (so a 20-candle average exists); PK sells `pkSell`
static mock::Host chartS(unsigned long t0, double pkOpen, double pkClose, long pkSell) {
    mock::Host h = chart(t0, pkOpen, pkClose);
    for (int i = 0; i < PK; ++i) { h.bars[i].rows.push_back(mock::Price(static_cast<float>(M(125)), 0, 100, 100)); h.bars[i].volume += 100; }
    h.bars[PK].rows.push_back(mock::Price(static_cast<float>(M(99.5)), 0, pkSell, pkSell)); h.bars[PK].volume += pkSell;
    return h;
}
int main() {
    setenv("TZ", "UTC", 1); tzset();
    g_sabGraceMs = 0;                                                   // the 90 s attacker-pane grace off for the test clock
    const unsigned long t0 = 1791504000UL + 9 * 3600;
    // [shared]
    {   home("shared"); mock::Host h = chartS(t0, 100.5, 101, 600); add(h, 100, 103);
        DeltaProfile p; redraw(h, p); redraw(h, p);
        const std::string rec = slurp(sabPath());
        sab::Rec r; bool ok = false; { std::istringstream in(rec); std::string l; while (std::getline(in, l)) if (sab::parse(l, r)) { ok = true; break; } }
        check(ok && r.side == sab::LOW && r.close == 'G' && r.by == "DP" && r.raw == static_cast<long long>(h.bars[PK].time) && r.E == 2.0, "[shared] one SAB line: bullish, green close, decided here (DP), the PK candle, E 2.0");
        check(countPrefix(rec, "SAB|") == 1, "[shared] exactly one A in the record");
        check(ringsOn(h, PK, false) == 1 && ringsOn(h, PK, true) == 1 && textA(h, C_SUPPORT) == 1, "[shared] ONE circle on the candle: the letter A in a green ring (the node A rides on it)");
        check(state(h).sabDrawn == 1, "[shared] status: shared A drawn 1");
        bool board = false; for (const sb::Ev& e : boardAll()) if (e.bar == r.key && e.side == 1 && e.flag == sb::A_FINAL) board = true;
        check(board, "[shared] SignalBoard A carries the shared A (final)");
        add(h, 100.5, 103.5); add(h, 101, 104); redraw(h, p);
        check(slurp(sabPath()) == rec && ringsOn(h, PK, false) == 1 && textA(h, C_SUPPORT) == 1, "[shared] later candles: never repainted, nothing written again");
        finish(h, p);
        DeltaProfile q; mock::Host h2 = h; h2.data = NULL; redraw(h2, q); redraw(h2, q);
        check(slurp(sabPath()) == rec && ringsOn(h2, PK, false) == 1 && textA(h2, C_SUPPORT) == 1, "[shared] restart: the same A from the record, nothing written twice");
        finish(h2, q); }
    // [red] attackers' colour: no shared A; (2.23.1) the node A confirmed by the next candle's green close is ONE green circle (was gray)
    {   home("red"); mock::Host h = chartS(t0, 101.5, 100.75, 600);
        add(h, 99.5, 103.5, 100, 103);
        DeltaProfile p; redraw(h, p);
        add(h, 99.75, 103.25); redraw(h, p);
        h.bars.back().high = 104.5f; add(h, 100, 105); redraw(h, p); redraw(h, p);
        check(countPrefix(slurp(sabPath()), "SAB|") == 0, "[red] sellers 6x but a RED close at the low: no shared A in the record");
        check(ringsOn(h, PK, true) == 1 && ringsOn(h, PK, false) == 1 && textA(h, C_RESISTANCE) == 0, "[red] (2.23.1, Rassul 2026-10-25) the node A confirmed by the NEXT candle's green close is coloured green like every A (was: gray)");
        finish(h, p); }
    // [one] two node A's on one green candle, no shared A (no prior sellers): one coloured circle
    {   home("one"); mock::Host h = chart(t0, 100.5, 101);
        h.bars[PK].rows.push_back(mock::Price(99.5f, 0, 9000, 9000)); h.bars[PK].volume += 9000;
        add(h, 100, 103);
        DeltaProfile p; redraw(h, p); redraw(h, p);
        int nodeA = 0, merged = 0; for (const auto& kv : state(h).bubbleState) { if (kv.second == "drawn") ++nodeA; if (kv.second == "merged: one circle per candle") ++merged; }
        check(countPrefix(slurp(sabPath()), "SAB|") == 0, "[one] fixture: no shared A (no earlier sellers to average)");
        check(ringsOn(h, PK, true) == 1 && nodeA == 1 && merged == 1, std::string("[one] two node A's on the candle -> ONE coloured circle, the other merged (drawn ") + std::to_string(nodeA) + ", merged " + std::to_string(merged) + ")");
        check(ringsOn(h, PK, false) == 1, "[one] and no gray beside it on that candle");
        finish(h, p); }
    // [adopt] an attacker pane posts on this chart: the record's line is drawn, this plugin decides nothing
    {   home("adopt"); mock::Host h = chartS(t0, 100.5, 101, 600); add(h, 100, 103);
        std::vector<sb::Ev> mk(1); mk[0].bar = 1; sb::publish('O', "ESZ26", 60, "test", (long long)steadyMs(), mk);
        DeltaProfile p; redraw(h, p); sb::publish('O', "ESZ26", 60, "test", (long long)steadyMs(), mk); redraw(h, p);
        if (std::getenv("DBG")) std::cerr << "adopt: SAB " << countPrefix(slurp(sabPath()), "SAB|") << " ringsOnC " << ringsOn(h, PK, true) << " A " << textA(h, C_SUPPORT) << " basHere " << latchBooks().begin()->second.sabBasHere << "\n";
        check(countPrefix(slurp(sabPath()), "SAB|") == 0 && ringsOn(h, PK, true) == 1 && textA(h, C_SUPPORT) == 0, "[adopt] attacker pane present: nothing decided here (the node A circle only)");
        sab::Rec r; r.mkt = "ES"; r.spb = 60; r.key = static_cast<long long>(h.bars[PK].time); r.raw = static_cast<long long>(h.bars[PK].time); r.side = sab::LOW;
        r.effort = 4.2; r.E = 3.0; r.att = 600; r.avg = 140; r.close = 'G'; r.extPx = 99; r.closePx = 101; r.by = "BAS"; r.loc = "ext5";
        { std::ofstream f(sabPath().c_str(), std::ios::binary | std::ios::app); f << sab::line(r) << "\n"; }
        sb::publish('O', "ESZ26", 60, "test", (long long)steadyMs(), mk);
        state(h).builtAt -= 5000; latchBooks().begin()->second.sab.sReadMs = -1; redraw(h, p);
        check(ringsOn(h, PK, false) == 1 && textA(h, C_SUPPORT) == 1 && countPrefix(slurp(sabPath()), "SAB|") == 1, "[adopt] the attacker pane's A drawn from the record: still ONE circle on the candle");
        finish(h, p); }
    std::cout << "dp 2.22.0 shared A / one circle: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
