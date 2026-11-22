// tests_erpane/test_erpane.cpp -- unit tests of ErPaneLogic.h (lsEffortRewardPane 0.2.0). g++ -std=c++17 -fsanitize=address,undefined -Werror
#include "../ErPaneLogic.h"
#include <cassert>
#include <cstdio>
#include <iostream>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static bool near(double a, double b, double e = 1e-9) { return std::fabs(a - b) <= e; }
using namespace erp;

static void bars() {
    CHECK(zoneOf(4) == 1); CHECK(zoneOf(5) == 1); CHECK(zoneOf(6) == 2); CHECK(zoneOf(10) == 3); CHECK(zoneOf(12) == 3); CHECK(zoneOf(14) == 4); CHECK(zoneOf(1) == 1);
    // 4 levels 100..103, zone 1: top = 103 / rest 100..102
    std::vector<Row> r = { {100, 10, 40}, {101, 10, 10}, {102, 10, 10}, {103, 30, 5} };
    Bars b = barRatios(r, 100, 103);
    CHECK(b.topOk && near(b.top, (30.0 / 1) / (30.0 / 3)));     // 3.0
    CHECK(b.botOk && near(b.bot, (40.0 / 1) / (25.0 / 3)));     // 4.8
    CHECK(!barRatios(r, 100, 102).topOk);                       // 3 levels: no ratio
    CHECK(!barRatios(r, 103, 100).topOk);                       // inverted
    std::vector<Row> onlyTop = { {100, 0, 5}, {103, 9, 0} };
    Bars c = barRatios(onlyTop, 100, 103); CHECK(!c.topOk); CHECK(!c.botOk);   // nothing bought in the rest / nothing sold in the rest
    // levels with no prints still count as levels (6 levels, zone 2)
    std::vector<Row> g = { {200, 4, 4}, {205, 4, 4} };
    Bars d = barRatios(g, 200, 205); CHECK(d.levels == 6 && d.zone == 2); CHECK(near(d.top, (4.0 / 2) / (4.0 / 4))); CHECK(near(d.bot, 2.0));
    std::vector<Row> neg = { {100, -5, 1}, {101, 1, 1}, {102, 1, 1}, {103, 1, 1} };
    CHECK(barRatios(neg, 100, 103).topOk);                      // a negative row is skipped, not summed
    std::vector<Row> big = { {100, 3000000000LL, 1}, {101, 3000000000LL, 1}, {102, 3000000000LL, 1}, {103, 9000000000LL, 1} };
    CHECK(near(barRatios(big, 100, 103).top, 3.0));             // volumes over 2^31
    CHECK(ratioText(1.74) == "1.7x"); CHECK(ratioText(120) == "99x+"); CHECK(ratioText(-1).empty()); CHECK(ratioText(NAN).empty());
}
static void words() {
    CHECK(std::string(wordOf(0)) == "NO"); CHECK(std::string(wordOf(0.49)) == "NO"); CHECK(std::string(wordOf(0.5)) == "LIGHT"); CHECK(std::string(wordOf(0.99)) == "LIGHT");
    CHECK(std::string(wordOf(1.0)) == "MODERATE"); CHECK(std::string(wordOf(1.49)) == "MODERATE"); CHECK(std::string(wordOf(1.5)) == "HEAVY"); CHECK(std::string(wordOf(NAN)) == "NO");
    CHECK(thousands(5934) == "5,934"); CHECK(thousands(0) == "0"); CHECK(thousands(1234567) == "1,234,567"); CHECK(thousands(-1234) == "-1,234");
    CHECK(thousands(3000000000LL) == "3,000,000,000");
}
static void wy() {
    std::vector<Sw> v; std::string why;
    const std::string t = "WY|1|ES|180|0.21.7\nL|28420|ONL/PHL\n3878010180|Sw|-1|ONH|28797|R99+\n3878009460|Sw|-1|LHI|28733|R99+\n3877953480|Bk|1|PDL|28768|R10\n"
                          "bad|Sw|-1|ONH|1|\n3878010000|Sw|2|ONH|28797|\n3878010001|Sw|1|O N H|28797|\n3878020000|Sw|1|PDL|28768";   // last line unterminated
    CHECK(parseWy(t, "ES", 180, v, why)); CHECK(v.size() == 2);
    CHECK(v.size() == 2 && v[0].stamp == 3878009460LL && v[0].lv == "LonHI" && v[0].dir == -1 && v[0].lvt == 28733);
    CHECK(v.size() == 2 && v[1].stamp == 3878010180LL && v[1].lv == "ONH");
    CHECK(!parseWy(t, "NQ", 180, v, why) && v.empty());          // foreign market
    CHECK(!parseWy(t, "ES", 60, v, why));                        // bar size
    CHECK(!parseWy("garbage\n", "ES", 180, v, why));
    CHECK(!parseWy(std::string(WY_MAX_BYTES + 1, 'x'), "ES", 180, v, why));
    std::string many = "WY|1|ES|180|x\n"; for (int i = 0; i < 1000; ++i) many += std::to_string(3878000000LL + i * 180) + "|Sw|1|PDL|28768|\n";
    CHECK(parseWy(many, "ES", 180, v, why) && v.size() == SW_KEEP && v.back().stamp == 3878000000LL + 999 * 180);
}
static std::string paramsText(const std::string& mkt) {
    std::string s = "VERSION,1\nMARKET," + mkt + "\nABS_MED,200\nHARD_CUT,3\nBIG_CUT,10\nTURN_PCT,20,30,50,70\nN15";
    for (int i = 0; i < 48; ++i) { s += ",50"; }
    s += "\nAVG_ORDER";
    for (int i = 0; i < 48; ++i) { s += ",2.5"; }
    s += "\nSW,3878010180,-1,28797,441,5934,25\nSW,3878009460,-1,28733,12,49,-1\nFUTURE_KEY,1\nEND\n"; return s;
}
static void params() {
    Params p = parseParams(paramsText("ES"), "ES");
    CHECK(p.fromFile && p.why == "ok" && near(p.absMed, 200) && near(p.hardCut, 3) && near(p.bigCut, 10) && near(p.pct[3], 70) && near(p.n15[47], 50) && near(p.avgOrd[0], 2.5));
    CHECK(p.hist.size() == 2 && p.hist[1].strict == 441 && p.hist[0].hid == -1);
    Params w = parseParams(paramsText("ES"), "NQ"); CHECK(!w.fromFile && near(w.absMed, 55.5) && w.why.find("market") != std::string::npos);
    std::string g = paramsText("ES"); g.replace(g.find("HARD_CUT,3"), 10, "HARD_CUT,x"); Params q = parseParams(g, "ES"); CHECK(!q.fromFile && near(q.hardCut, 3.010) && q.hist.empty());
    std::string g2 = paramsText("ES"); g2.replace(g2.find("TURN_PCT,20"), 11, "TURN_PCT,200"); CHECK(!parseParams(g2, "ES").fromFile);
    std::string g3 = paramsText("ES"); g3.replace(g3.find("SW,3878010180,-1,28797,441,5934"), 31, "SW,3878010180,-1,28797,441,4"); CHECK(!parseParams(g3, "ES").fromFile);   // tot < strict
    CHECK(!parseParams(std::string(PARAMS_MAX_BYTES + 5, 'a'), "ES").fromFile);
    CHECK(!parseParams("", "ES").fromFile); CHECK(parseParams("", "XX").absMed == 0);
    std::string nan = paramsText("ES"); nan.replace(nan.find("ABS_MED,200"), 11, "ABS_MED,nan"); CHECK(!parseParams(nan, "ES").fromFile);
    std::string trunc = paramsText("ES"); trunc = trunc.substr(0, trunc.find("AVG_ORDER") + 20); CHECK(!parseParams(trunc, "ES").fromFile || parseParams(trunc, "ES").avgOrd[0] == 0);
}
// a little candle: spb 60, key 1260 = [1200, 1260)
static void strictAndTurn() {
    Live L; L.reset(60);
    L.onEvent(1190, 3, 1, 105, 1, 1, 2);                             // warm-up event in the candle before: the feed was live before 1200
    // resting offers near 100..106; order 200 shows 2 (an iceberg that fills 6)
    L.onEvent(1200, 0, 0, 104, 2, 200, 0);
    // buyers lift 101, 102, 103 (progress), 104 (no progress after: the high), sellers then hit back
    L.onEvent(1201, 3, 1, 101, 10, 150, 300);
    L.onEvent(1202, 3, 1, 102, 10, 151, 301);
    L.onEvent(1203, 3, 1, 103, 10, 152, 302);
    L.onEvent(1204, 3, 1, 104, 6, 200, 303);                          // 6 lots on an order that showed 2: 4 hidden
    L.onEvent(1205, 3, 2, 104, 3, 201, 0);                            // an implied print at the high (no side)
    L.onEvent(1206, 3, 1, 103, 7, 153, 304);                          // buying at 103 again: no progress after it (nothing higher later)
    // a fresh seller (new id 400 > every id) through 3 prices within 15 s of the high (1204)
    L.onEvent(1210, 3, 0, 103, 5, 160, 400); L.onEvent(1210, 3, 0, 102, 5, 161, 400); L.onEvent(1210, 3, 0, 101, 5, 162, 400);
    L.onEvent(1230, 3, 0, 100, 30, 163, 401);                         // 26 s after the high: in B's 30 s, not in H's 15 s
    L.onEvent(1240, 3, 0, 99, 50, 164, 402);                          // 36 s after: outside both windows
    L.onEvent(1261, 3, 1, 99, 1, 165, 403);                           // next candle: closes 1260
    const Sum* s = L.find(1260); CHECK(s != nullptr); if (!s) return;
    CHECK(s->complete);
    const Strict x = strictOf(*s, -1, 101);                           // high swept at 101: buying above 101
    CHECK(x.tot == 10 + 10 + 6 + 7);                                  // 102, 103, 104, 103
    CHECK(x.strict == 6 + 7);                                         // only 104 (the high) and the later 103 got no further
    CHECK(x.hid == 4);                                                // the iceberg's 4 hidden lots (at the no-progress 104)
    CHECK(x.unk == 7);                                                // 153 was never seen resting
    CHECK(s->hi.ok && s->hi.extPx == 104 && s->hi.extSec == 1204 && !s->hi.late);
    CHECK(s->hi.fresh);
    CHECK(s->hi.hard == -7 + 15);                                     // (1206 buy 7) + (1210 sell 15) within 15 s, counter (sellers) positive
    CHECK(s->hi.big == 30);                                           // the biggest seller within 30 s
    // the low: the extreme low is 99 at 1240, buyers' answer after it: none in the candle
    CHECK(s->lo.ok && s->lo.extPx == 99 && s->lo.extSec == 1240 && s->lo.late && s->lo.hard == 0 && s->lo.big == 0);
    const Strict y = strictOf(*s, 1, 101);                            // a low swept at 101: selling below it
    CHECK(y.tot == 30 + 50 && y.strict == 50);                        // 100 was followed by 99 (lower): progress, not strict
}
static void strictLowPrecise() {
    Live L; L.reset(60); L.onEvent(1190, 3, 1, 105, 1, 1, 2);
    L.onEvent(1201, 3, 0, 100, 30, 10, 20); L.onEvent(1202, 3, 0, 99, 50, 11, 21); L.onEvent(1203, 3, 0, 100, 4, 12, 22);
    L.closeThrough(1260); const Sum* s = L.find(1260); CHECK(s && s->complete);
    if (s) { const Strict y = strictOf(*s, 1, 101); CHECK(y.tot == 84); CHECK(y.strict == 50 + 4); }   // 100 (first) was followed by 99: progress
}
static void incomplete() {
    Live L; L.reset(60);
    L.onEvent(1205, 3, 1, 100, 1, 1, 2); L.onEvent(1261, 3, 1, 100, 1, 3, 4);   // the feed started inside the candle
    const Sum* s = L.find(1260); CHECK(s && !s->complete);
    L.onEvent(1325, 3, 1, 100, 1, 5, 6); const Sum* t = L.find(1320); CHECK(t && t->complete);
    L.gap(); L.onEvent(1330, 3, 1, 100, 1, 7, 8); L.onEvent(1381, 3, 1, 100, 1, 9, 10);
    const Sum* u = L.find(1380); CHECK(u && !u->complete);           // a gap: the candle after it cannot be whole
    L.onEvent(1300, 3, 1, 100, 1, 11, 12); CHECK(L.n.late == 1);       // an event of a closed candle is dropped
    CHECK(L.find(1320) && !L.find(1320)->complete);                 // ... and that candle is no longer whole
    L.onEvent(0, 3, 1, 100, 1, 1, 1); L.onEvent(1500, 9, 1, 100, 1, 1, 1); L.onEvent(1500, 3, 1, -5, 1, 1, 1); CHECK(L.n.bad == 3);
    for (int i = 0; i < 30; ++i) L.onEvent(2000 + i * 60, 3, 1, 100, 1, 100 + i, 200 + i);
    CHECK(L.sums.size() == SUMS_KEEP);
}
static void reviewCases() {
    // a far-future event never opens a candle (review #4)
    Live L; L.reset(60); L.setLimit(1260); L.onEvent(1190, 3, 1, 100, 1, 1, 2);
    L.onEvent(1190 + 86400 * 30, 3, 1, 100, 1, 3, 4); CHECK(L.n.future == 1);
    L.onEvent(1201, 3, 1, 101, 5, 5, 6); L.closeThrough(1260); CHECK(L.find(1260) && L.find(1260)->complete && L.find(1260)->lots == 5);
    // an event of a closed candle after a quiet close: never reopened, the candle is no longer whole (review #2)
    L.onEvent(1259, 3, 1, 101, 2, 7, 8); CHECK(L.n.late == 1 && !L.find(1260)->complete); int k = 0; for (const Sum& x : L.sums) if (x.key == 1260) ++k; CHECK(k == 1);
    // the live clock starts at the first FILL, not at a book snapshot's old NEW (review #6)
    Live M; M.reset(60); M.onEvent(1100, 0, 0, 100, 1, 1, 0); M.onEvent(1230, 3, 1, 100, 1, 2, 3); M.closeThrough(1260); CHECK(M.find(1260) && !M.find(1260)->complete);
    // a garbage ID jump does not freeze the clock (review #9)
    IdClock c; c.note(100, 10); c.note(100 + (1LL << 30), 11); c.note(101, 12); CHECK(c.maxId == 101 && c.age(102, 13) == 0 && c.age(101, 13) == 1);
    // a wrong-contract chart (never traded through the level) decides nothing (review #5)
    Live W; W.reset(60); W.onEvent(1190, 3, 1, 100, 1, 1, 2); W.onEvent(1201, 3, 1, 90, 5, 3, 4); W.closeThrough(1260);
    Sw sw; sw.stamp = 1260; sw.dir = -1; sw.lv = "ONH"; sw.lvt = 95; CHECK(decideLive(*W.find(1260), sw, defaults("ES"), 18).src == '-');
    // a tiny divisor is refused (review #11)
    std::string t = "VERSION,1\nMARKET,ES\nN15"; for (int i = 0; i < 48; ++i) { t += ",1e-300"; } t += "\n"; CHECK(!parseParams(t, "ES").fromFile);
}
static void overflowCap() {
    Live L; L.reset(60); L.onEvent(1190, 3, 1, 100, 1, 1, 2);
    for (size_t i = 0; i < FILL_CAP + 10; ++i) L.onEvent(1201, 3, 1, 100, 1, 0, 0);
    L.closeThrough(1260); const Sum* s = L.find(1260); CHECK(s && s->overflow && !s->complete); CHECK(L.n.overflows == 1);
    CHECK(L.openFills() == 0);
    // shown cap: far orders never tracked, near ones capped
    Live M; M.reset(60); M.onEvent(1200, 3, 1, 1200, 1, 0, 0);
    for (long long i = 0; i < (long long)SHOWN_CAP + 100; ++i) M.onEvent(1201, 0, 0, 1200 + (i % 50), 1, 10 + i, 0);
    CHECK(M.orders() <= SHOWN_CAP);
    for (long long i = 0; i < 1200; ++i) M.onEvent(1202, 0, 0, 5000, 1, 900000 + i, 0);      // far from the last trade
    CHECK(M.orders() <= SHOWN_CAP);
}
static void idClock() {
    IdClock c; CHECK(c.age(5, 100) == -1);
    c.note(10, 100); c.note(20, 101); c.note(30, 105);
    CHECK(c.age(31, 106) == 0); CHECK(c.age(25, 106) == 1); CHECK(c.age(15, 106) == 5); CHECK(c.age(5, 106) == 6);   // older than the ring: its first point
    c.note(25, 106); CHECK(c.maxId == 30);                            // never backwards
    // unwrap across 2^32
    IdClock w; w.note(0xFFFFFFF0LL, 1); CHECK(w.unwrap((long long)(int32_t)0x00000010) == 0x100000010LL); CHECK(w.unwrap(0xFFFFFFF5LL) == 0xFFFFFFF5LL);
    IdClock r; for (int i = 0; i < 10000; ++i) r.note(i + 1, i); CHECK(r.pts.size() == CLOCK_CAP);
}
static void freshNeedsAge() {
    Live L; L.reset(60); L.onEvent(1190, 3, 1, 100, 1, 1, 2);
    L.onEvent(1195, 0, 0, 90, 1, 50, 0);                                 // id 50 issued at 1195
    L.onEvent(1200, 0, 0, 90, 1, 60, 0);
    L.onEvent(1201, 3, 1, 101, 1, 61, 62);                              // the high at 1201
    L.onEvent(1205, 3, 0, 100, 1, 63, 50); L.onEvent(1205, 3, 0, 99, 1, 64, 50); L.onEvent(1205, 3, 0, 98, 1, 65, 50);   // 3 prices but id 50 is 10 s old
    L.closeThrough(1260); const Sum* s = L.find(1260); CHECK(s && !s->hi.fresh);
}
static void decide() {
    Live L; L.reset(60); L.onEvent(1190, 3, 1, 100, 1, 1, 2);
    L.onEvent(1201, 3, 1, 102, 10, 2, 3); L.onEvent(1210, 3, 0, 101, 40, 4, 5); L.closeThrough(1260);
    Params P = parseParams(paramsText("ES"), "ES"); Sw w; w.stamp = 1260; w.dir = -1; w.lv = "ONH"; w.lvt = 101;
    const Sum* s = L.find(1260); CHECK(s != nullptr); if (!s) return;
    Read r = decideLive(*s, w, P, 18);
    CHECK(r.src == 'L' && r.strict == 10 && r.tot == 10 && r.F == 0);
    CHECK(r.H == 0 && r.hard100 == 80);                                 // 40 / 50 = 0.8 < 3
    CHECK(r.B == 1 && r.big100 == 1600);                                 // 40 / 2.5 = 16 > 10
    CHECK(r.score == 1);
    Read none = decideLive(*s, w, defaults("ES"), 18); CHECK(none.H == -1 && none.score == -1 && none.strict == 10);   // no normals: no pill
    Sw other = w; other.stamp = 1320; CHECK(decideLive(*s, other, P, 18).src == '-');
    CHECK(decideLive(*s, w, P, 48).score == -1 && decideLive(*s, w, P, -1).score == -1);
    // record round trip
    const std::string ln = recLine(r); Read b; CHECK(parseRec(ln, b) && recLine(b) == ln);
    CHECK(!parseRec(ln + "|x", b)); CHECK(!parseRec("R|1|0|ONH|1|1|1|1|1|0|0|0|0|0|L|1|1", b));   // dir 0
    std::map<long long, Read> m; for (int i = 0; i < 400; ++i) { Read q = r; q.stamp = 1200 + i; m[q.stamp] = q; }
    const std::string ft = recFileText(m, "ES", 180); std::map<long long, Read> back = parseRecFile(ft, "ES", 180);
    CHECK(back.size() == REC_KEEP && back.begin()->first == 1300); CHECK(parseRecFile(ft, "NQ", 180).empty()); CHECK(parseRecFile(ft.substr(0, ft.size() - 3), "ES", 180).size() == REC_KEEP - 1);
    // history row
    Read h = fromHist(P.hist[1], Sw{3878010180LL, -1, "ONH", 28797}); CHECK(h.src == 'N' && h.strict == 441 && h.score == -1);
    CHECK(fromHist(P.hist[1], Sw{3878010180LL, 1, "ONH", 28797}).src == '-');
}
static void strip() {
    Params P = parseParams(paramsText("ES"), "ES"); P.absMed = 244;
    Read r; r.stamp = 1; r.dir = -1; r.lv = "ONH"; r.lvt = 28797; r.strict = 441; r.tot = 5934; r.hid = 25; r.score = 3; r.src = 'L';
    Strip s = stripOf(r, P, true, 1.7);
    CHECK(s.text == "HEAVY ABSORPTION 1.8x \x96 441 of 5,934 buy lots above ONH got no further (25 hidden)"); CHECK(s.pill == "70% turn" && s.pillTone == 2 && s.word == 3);
    r.dir = 1; r.lv = "LonLO"; r.hid = 0; r.score = 1; r.strict = 12; r.tot = 49;
    s = stripOf(r, P, true, 0.7);
    CHECK(s.text == "NO ABSORPTION 0.0x \x96 12 of 49 sell lots below LonLO got no further \xB7 thin poke"); CHECK(s.pill == "30% turn" && s.pillTone == 1);
    r.late = true; CHECK(stripOf(r, P, false, 0).pill == "late");
    Read n; n.lv = "PDL"; n.dir = 1; Strip e = stripOf(n, P, true, 2); CHECK(e.pill == "\x96" && e.pillTone == -1 && e.word == -1);
    Read h; h.dir = -1; h.lv = "1hH"; h.strict = 10; h.tot = 50; h.src = 'N'; CHECK(stripOf(h, P, true, 2).pill == "\x96");
    P.absMed = 0; r.late = false; r.strict = 3; CHECK(stripOf(r, P, true, 2).text.find("HEAVY") == 0);   // a zero median never divides by zero
}
int main() {
    bars(); words(); wy(); params(); strictAndTurn(); strictLowPrecise(); incomplete(); overflowCap(); idClock(); reviewCases(); freshNeedsAge(); decide(); strip();
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
