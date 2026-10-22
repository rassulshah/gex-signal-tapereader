// tests_sab/test_sab.cpp - SharedAbsorb.h 1.0.0 logic (rule, location switch, params, record, store, book, level, hover)
#include <unistd.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include "../SharedAbsorb.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
using namespace sab;
// 30 candles falling; candle i made a low with sellers `sell` lots (others 100), closing green / red
static std::vector<Bar> fall(long long sell, bool green, int n = 30, int at = 25) {
    std::vector<Bar> v;
    for (int i = 0; i < n; ++i) { Bar b; b.o = 1000 - i * 4; b.c = b.o - 3; b.h = b.o + 1; b.l = b.c - 1; b.buy = 100; b.sell = 100; b.ok = true; v.push_back(b); }
    Bar& x = v[at]; x.l = v[at - 1].l - 6; x.sell = sell;
    if (green) { x.o = x.l + 2; x.c = x.l + 5; } else { x.o = x.l + 5; x.c = x.l + 2; }
    x.h = std::max(x.o, x.c) + 1;
    return v;
}
int main() {
    // [rule] threshold 3.0 (a file value): 3.0x green at a 5-candle low = A; 2.9x no; red no; doji no
    { std::vector<Bar> b = fall(300, true); Decision d = decide(b, 25, LOW, 3.0);
      CHECK(d.decided && d.fired && d.loc && d.colour && std::fabs(d.effort - 3.0) < 1e-9 && d.att == 300 && std::fabs(d.avg - 100) < 1e-9); }
    { std::vector<Bar> b = fall(290, true); Decision d = decide(b, 25, LOW, 3.0); CHECK(d.decided && !d.fired && d.why == "effort under the threshold"); }
    { std::vector<Bar> b = fall(900, false); Decision d = decide(b, 25, LOW, 3.0); CHECK(d.decided && !d.fired && !d.colour && d.why == "closed in the attackers' colour"); }
    { std::vector<Bar> b = fall(900, true); b[25].c = b[25].o; Decision d = decide(b, 25, LOW, 3.0); CHECK(!d.fired); }
    // NQ 1.5: 1.5x fires
    { std::vector<Bar> b = fall(200, true); CHECK(decide(b, 25, LOW, defaultE("NQ")).fired && decide(b, 25, LOW, defaultE("ES")).fired); b = fall(190, true); CHECK(!decide(b, 25, LOW, defaultE("CL")).fired);
      for (const char* m : { "ES", "NQ", "CL", "GC", "HG", "NG", "EU", "6E" }) CHECK(defaultE(m) == 2.0); }   // review 2026-10-22: one E 2.0x
    // the high side: buyers lifting 4x at a 5-candle high, red close; green close no
    { std::vector<Bar> b; for (int i = 0; i < 30; ++i) { Bar x; x.o = 1000 + i * 4; x.c = x.o + 3; x.l = x.o - 1; x.h = x.c + 1; x.buy = 100; x.sell = 100; x.ok = true; b.push_back(x); }
      Bar& x = b[25]; x.h = b[24].h + 6; x.buy = 400; x.o = x.h - 2; x.c = x.h - 5; x.l = x.c - 1;
      CHECK(decide(b, 25, HIGH, 3.0).fired); CHECK(!decide(b, 25, LOW, 3.0).fired);
      x.c = x.o + 1; CHECK(!decide(b, 25, HIGH, 3.0).fired); }
    // not at a 5-candle low: a lower low 3 candles before
    { std::vector<Bar> b = fall(900, true); b[22].l = b[25].l - 1; Decision d = decide(b, 25, LOW, 3.0); CHECK(d.decided && !d.fired && !d.loc); }
    // equal low counts as the extreme (>=)
    { std::vector<Bar> b = fall(900, true); b[22].l = b[25].l; CHECK(decide(b, 25, LOW, 3.0).fired); }
    // too few earlier candles with lots: not decided (never written)
    { std::vector<Bar> b = fall(900, true); for (int k = 0; k < 25; ++k) if (k < 21) b[k].sell = -1; b[21].sell = -1; Decision d = decide(b, 25, LOW, 3.0); CHECK(!d.decided && !d.fired); }
    // the average uses at most the 20 candles before (a huge one 21 back is ignored)
    { std::vector<Bar> b = fall(300, true); b[4].sell = 1000000; CHECK(decide(b, 25, LOW, 3.0).fired); b[5].sell = 1000000; CHECK(!decide(b, 25, LOW, 3.0).fired); }
    // unknown lots on the candle itself: not decided
    { std::vector<Bar> b = fall(300, true); b[25].sell = -1; CHECK(!decide(b, 25, LOW, 3.0).decided); }
    // [location switch] LOC_PIVOT / LOC_BOTH callable, the default is the 5-candle extreme
    { CHECK(LOCATION == LOC_EXT5); std::vector<Bar> b = fall(300, true); CHECK(location(b, 25, LOW, LOC_EXT5)); CHECK(location(b, 25, LOW, LOC_PIVOT)); CHECK(location(b, 25, LOW, LOC_BOTH));
      CHECK(std::string(locName(LOC_EXT5)) == "ext5"); }
    // [params]
    { Params p = parseParams("", "ES", false); CHECK(p.E == 2.0 && p.n == 141 && p.learning && !p.fromFile && p.why.find("missing") != std::string::npos); }
    { Params p = parseParams("VERSION,1\nMARKET,ES\nASOF,2026-10-22\nPARAM,ABS_MIN_EFFORT_X,2.5\nPARAM,ABS_REQUIRE_DEFENDER_CLOSE,0\nREAD,SHARED_A,SHOW,40,70.0,48.0,22.0,30\n", "ES");
      CHECK(p.E == 2.5 && p.fromFile && p.oddsFromFile && p.n == 40 && p.held == 70.0 && p.base == 48.0 && !p.learning && p.why.find("ignored") != std::string::npos); }
    { Params p = parseParams("VERSION,1\nMARKET,ES\nREAD,SHARED_A,?,12,70.0,48.0,22.0,30\n", "ES"); CHECK(p.E == 2.0 && !p.oddsFromFile && p.n == 141); }
    { Params p = parseParams("VERSION,1\nMARKET,NQ\nPARAM,ABS_MIN_EFFORT_X,2.5\n", "ES"); CHECK(p.E == 2.0 && !p.fromFile && p.why.find("another market") != std::string::npos); }
    { Params p = parseParams("garbage\nPARAM,ABS_MIN_EFFORT_X,1.0\n", "CL"); CHECK(p.E == 2.0 && !p.fromFile); }
    { Params p = parseParams("VERSION,1\nMARKET,GC\nPARAM,ABS_MIN_EFFORT_X,nan\n", "GC"); CHECK(p.E == 2.0 && !p.fromFile); }
    { Params p = parseParams("VERSION,1\nMARKET,GC\nPARAM,ABS_MIN_EFFORT_X,50\n", "GC"); CHECK(p.E == 2.0 && !p.fromFile); }
    { Params p = parseParams("VERSION,1\nMARKET,6E\nPARAM,ABS_MIN_EFFORT_X,2.0\n", "EU"); CHECK(p.fromFile && p.E == 2.0); CHECK(defaultE("6E") == 2.0 && defaultE("XX") == 2.0); }
    { Params p = parseParams("# LRA order-flow badges - orderflow_badges 1.0.0 (2026-10-21) - x\nVERSION,1\nMARKET,ES\nPARAM,ABS_MIN_EFFORT_X,2.4\n", "ES");
      CHECK(p.E == 2.0 && !p.fromFile && p.why.find("pooled") != std::string::npos); }        // his 10-21 file: the pooled 2.0 is not ES's 3.0
    { Params p = parseParams("# LRA order-flow badges - orderflow_badges 1.2.0 (2026-10-22) - x\nVERSION,1\nMARKET,ES\nPARAM,ABS_MIN_EFFORT_X,3.5\n", "ES"); CHECK(p.E == 3.5 && p.fromFile); }
    { Params p = parseParams("VERSION,1\nMARKET,HG\n", "HG"); CHECK(p.E == 2.0 && !p.fromFile && p.why.find("study default") != std::string::npos); }
    // [record] round trip, colour enforced on parse, garbled lines rejected
    Rec r; r.mkt = "ES"; r.spb = 180; r.key = 1792600000; r.raw = 3875446980LL; r.side = LOW; r.effort = 3.8; r.E = 3.0; r.att = 7306; r.avg = 1922.6; r.close = 'G';
    r.extPx = 7758.75; r.closePx = 7761.5; r.book = 1; r.bookLots = 1240; r.refills = 5; r.decided = 1792600005; r.by = "BAS"; r.loc = "ext5";
    { Rec q; CHECK(parse(line(r), q)); CHECK(q.key == r.key && q.raw == r.raw && q.side == LOW && std::fabs(q.effort - 3.8) < 1e-9 && q.att == 7306 && q.book == 1 && q.bookLots == 1240 && q.refills == 5 && q.by == "BAS" && q.extPx == 7758.75); }
    { Rec bad = r; bad.close = 'R'; Rec q; CHECK(!parse(line(bad), q)); }                 // attacker colour never a valid A
    { Rec h = r; h.side = HIGH; h.close = 'R'; Rec q; CHECK(parse(line(h), q) && q.side == HIGH); }
    { Rec q; std::string s = line(r); CHECK(!parse(s.substr(0, s.size() - 5), q)); CHECK(!parse("SAB|2|x", q)); CHECK(!parse(s + "|extra", q)); }
    { Rec nb = r; nb.book = -1; Rec q; CHECK(parse(line(nb), q) && q.book == -1); }
    // [store] first line wins, extras merge (first value wins), orphans, other market ignored, raw lookup, torn line waits
    { Store S; S.mkt = "ES"; S.spb = 180;
      Rec r2 = r; r2.effort = 9.9; r2.by = "DP";
      Extra x; x.mkt = "ES"; x.spb = 180; x.key = r.key; x.side = LOW; x.kv["pullx"] = "2.70"; x.kv["e100"] = "40"; x.kv["level"] = "ONL"; x.kv["lvd"] = "0.4"; x.kv["gamma"] = "1"; x.by = "BF";
      S.add(xline(x));                                                    // before its SAB line: kept as an orphan
      S.add(line(r)); S.add(line(r2));
      Extra x2 = x; x2.kv["pullx"] = "9"; S.add(xline(x2));
      Rec o = r; o.mkt = "NQ"; o.key += 180; S.add(line(o));
      const Rec* g = S.at(LOW, r.key); CHECK(g && std::fabs(g->effort - 3.8) < 1e-9 && g->by == "BAS" && std::fabs(g->pullx - 2.7) < 1e-9 && g->e100 == 40 && g->level == "ONL" && g->gamma == 1);
      CHECK(S.atRaw(LOW, r.raw) == g && !S.at(HIGH, r.key) && S.wrongMkt == 1 && S.byKey.size() == 1); }
    { char tmpl[] = "/tmp/sabXXXXXX"; const int fd = mkstemp(tmpl); CHECK(fd >= 0); if (fd >= 0) close(fd);
      const std::string path = tmpl; { std::ofstream o(path.c_str(), std::ios::trunc); o << line(r) << "\n" << "SAB|1|1.0.0|ES|180|17926"; }
      Store S; S.mkt = "ES"; S.spb = 180; CHECK(refresh(S, path)); CHECK(S.byKey.size() == 1);
      { std::ofstream o(path.c_str(), std::ios::app); o << "00000|torn\n"; } refresh(S, path); CHECK(S.byKey.size() == 1 && S.bad == 1);
      Rec r3 = r; r3.key += 180; r3.raw += 180; CHECK(append(path, std::vector<std::string>{ line(r3) })); CHECK(refresh(S, path) && S.byKey.size() == 2);
      CHECK(!refresh(S, path));                                           // nothing new
      { std::ofstream o(path.c_str(), std::ios::trunc); o << line(r3) << "\n"; } CHECK(refresh(S, path) && S.byKey.size() == 1);   // rewritten shorter: re-read
      std::remove(path.c_str()); CHECK(!refresh(S, path)); CHECK(fileName("6E", 180) == "SAB-EU-180.csv"); }
    // [book] defenders' resting bids inside the low wick
    { Bar x; x.o = 110; x.c = 115; x.l = 100; x.h = 116; x.ok = true; int bk = 0, rf = 0; long long lt = 0;
      std::vector<ObEv> ev; ObEv e; e.px = 102 * 0.25; e.side = -1; e.traded = 800; e.refills = 3; ev.push_back(e); e.px = 112 * 0.25; ev.push_back(e); e.px = 101 * 0.25; e.side = 1; ev.push_back(e);
      bookOf(x, LOW, true, ev, 0.25, bk, lt, rf); CHECK(bk == 1 && lt == 800 && rf == 3);
      bookOf(x, LOW, false, ev, 0.25, bk, lt, rf); CHECK(bk == -1);
      bookOf(x, HIGH, true, ev, 0.25, bk, lt, rf); CHECK(bk == 0 && lt == 0); }
    // [level] nearest within 0.5 ATR + gamma sign from HVL
    { std::vector<Lv> v = { { "ONL", 7757.0 }, { "MQ HVL", 7755.0 }, { "PDL", 7752.75 } }; std::string n; double d = 0; int g = 0;
      levelOf(v, 7758.75, 7761.5, 4.0, n, d, g); CHECK(n == "ONL" && std::fabs(d - 0.4375) < 1e-9 && g == 1);
      levelOf(v, 7740.0, 7741.0, 4.0, n, d, g); CHECK(n.empty() && g == -1); }
    // [hover] the agreed order: question, what, effort, close, book, pulling, attack, level, held, use, example
    { Rec h = r; h.pullx = 2.7; h.e100 = 40; h.level = "ONL"; h.lvd = 0.4; h.gamma = 1;
      const std::vector<std::string> v = hover(h, defaults("ES"));
      CHECK(v.size() == 11);
      CHECK(v[0] == "A  Absorbed - will this low hold?");
      CHECK(v[1].find("3.8x their 20-candle average (ES needs 3.0x)") != std::string::npos && v[1].find("closed green") != std::string::npos);
      CHECK(v[2].find("   Effort: 3.8x (3.0x)") == 0 && v[3].find("   Close: green") == 0 && v[4].find("   Book: bids absorbed 1,240 lots") == 0);
      CHECK(v[5].find("   Pulling: sellers pulled") == 0 && v[6].find("   Attack: faded") == 0 && v[7].find("   Level: ONL 0.4 ATR") == 0);
      CHECK(v[8].find("this low held 66% vs 51% for comparable 5-candle lows (+15 points, n 141) - LEARNING in this market") != std::string::npos);
      CHECK(hover(h, defaults("NQ"))[8].find("LEARNING") == std::string::npos && hover(h, defaults("GC"))[8].find("LEARNING in this market") != std::string::npos);
      CHECK(v[9].find("   Use: arms your reversal bar / CoB entry - not a direction call") == 0 && v[10].find("   Example:") == 0);
      for (const std::string& s : v) { CHECK(s.find('?') == std::string::npos || &s == &v[0]); CHECK(s.size() < 170); }
      Rec n = r; n.book = -1; const std::vector<std::string> w = hover(n, defaults("ES"));
      CHECK(w[4].find("n/a") != std::string::npos && w[5].find("n/a") != std::string::npos && w[6].find("n/a") != std::string::npos);
      Rec a = r; a.into100 = 210; CHECK(hover(a, defaults("ES"))[6].find("accelerating 2.1x - a warning") != std::string::npos);
      Rec hi = r; hi.side = HIGH; hi.close = 'R'; CHECK(hover(hi, defaults("ES"))[0] == "A  Absorbed - will this high hold?"); }
    std::printf("test_sab: %d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
