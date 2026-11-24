// t_read140.cpp -- lsTurn 1.4.0: the Absorption read (absRead / pxText / wrapRead / the heaviest price's tick)
#include "../TurnLogic.h"
#include <cstdio>
using namespace turn;
static int nCheck = 0, nFail = 0;
#define CHECK(c) do { ++nCheck; if (!(c)) { ++nFail; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
static Abs ab(long long x, bool away) { Abs a; a.x1000 = x; a.away = away; return a; }
static ACandle cnd(long long hi, long long lo, long long cl) { ACandle c; c.hi = hi; c.lo = lo; c.cl = cl; c.up = ab(800, false); c.dn = ab(800, false); c.upPx = hi; c.dnPx = lo; c.upLots = 5; c.dnLots = 5; return c; }
int main() {
    // pxText: decimals from the tick (float ticks too)
    CHECK(pxText(22551, 0.25) == "5637.75"); CHECK(pxText(11276, (float)0.0005) == "5.6380"); CHECK(pxText(70000, 0.01) == "700.00");
    CHECK(pxText(23361, 0.00005) == "1.16805"); CHECK(pxText(4500, 1.0) == "4500"); CHECK(pxText(12, 0.0) == "12"); CHECK(pxText(39998, 0.1) == "3999.8");
    // quiet
    { std::vector<ACandle> v; for (int i = 0; i < 5; ++i) v.push_back(cnd(110, 100, 105)); const ARead r = absRead(v, 20, 0.25); CHECK(r.verdict == 0 && r.head == "Quiet" && r.ev.empty()); }
    { const ARead r = absRead(std::vector<ACandle>(), 20, 0.25); CHECK(r.verdict == 0); }
    // an absorbed low that holds, tested twice; a candle trading to it and closing above = tested; candle 0 is the event
    { std::vector<ACandle> v; ACandle e = cnd(110, 100, 108); e.dn = ab(2400, true); e.dnPx = 101; e.dnLots = 77; e.lvDn = "VAL"; v.push_back(e);
      v.push_back(cnd(112, 101, 110)); v.push_back(cnd(113, 105, 111)); v.push_back(cnd(112, 100, 104));
      const ARead r = absRead(v, 20, 0.25); CHECK(r.verdict == 1); CHECK(r.ev.size() == 1 && r.ev[0].tested == 2 && r.ev[0].held == 3 && !r.ev[0].failed);
      CHECK(r.head == "Bullish ? - above the absorbed low at 25.25 (VAL)"); CHECK(r.why == "77 lots sold there, 2.4x normal. Held 3 candles, tested 2 times."); CHECK(r.watch == "Watch: a close below 25.25 = failed, bearish."); }
    // not away, or under the cutoff: no event
    { std::vector<ACandle> v; ACandle e = cnd(110, 100, 101); e.dn = ab(3000, false); v.push_back(e); ACandle f = cnd(110, 100, 108); f.dn = ab(1900, true); v.push_back(f);
      CHECK(absRead(v, 20, 1).verdict == 0); CHECK(absRead(v, 19, 1).verdict == 1); }
    // the low fails: a later close below it = bearish, newest
    { std::vector<ACandle> v; ACandle e = cnd(110, 100, 108); e.dn = ab(2000, true); e.dnPx = 101; e.dnLots = 9; v.push_back(e); v.push_back(cnd(109, 99, 100)); v.push_back(cnd(102, 97, 98));
      const ARead r = absRead(v, 20, 1); CHECK(r.verdict == -1 && r.ev[0].failed && r.ev[0].failAt == 1); CHECK(r.head == "Bearish ? - the absorbed low at 101 failed");
      CHECK(r.why == "9 lots sold there, 2.0x normal. Closed below it 1 candle ago."); CHECK(r.watch == "Watch: a close back above 101 = the failure is undone."); }
    // an absorbed high that holds: bearish; then a range when a low holds too; the newer one leads line 2
    { std::vector<ACandle> v; ACandle h = cnd(120, 110, 112); h.up = ab(3100, true); h.upPx = 119; h.upLots = 40; v.push_back(h);
      v.push_back(cnd(116, 108, 113));
      ARead r = absRead(v, 20, 1); CHECK(r.verdict == -1 && r.head == "Bearish ? - below the absorbed high at 119" && r.watch == "Watch: a close above 119 = failed, bullish.");
      ACandle l = cnd(115, 105, 113); l.dn = ab(2200, true); l.dnPx = 106; l.dnLots = 30; v.push_back(l);
      r = absRead(v, 20, 1); CHECK(r.verdict == 2); CHECK(r.head == "Range ? - absorbed low 106 / absorbed high 119");
      CHECK(r.why == "Newest, the low: 30 lots sold there, 2.2x normal. On the last candle. The high: 40 lots, 3.1x."); CHECK(r.watch == "Watch: a close above 119 = bullish; a close below 106 = bearish."); }
    // rising absorbed lows; a failure older than a holding event does not lead
    { std::vector<ACandle> v; ACandle a = cnd(110, 100, 108); a.dn = ab(2000, true); a.dnPx = 100; v.push_back(a);
      ACandle b = cnd(112, 102, 110); b.dn = ab(2500, true); b.dnPx = 103; v.push_back(b); v.push_back(cnd(114, 104, 112));
      ARead r = absRead(v, 20, 1); CHECK(r.verdict == 1 && r.head.find("103") != std::string::npos); CHECK(r.why.find("Absorbed lows are rising.") != std::string::npos);
      std::vector<ACandle> w; ACandle f = cnd(120, 110, 118); f.dn = ab(2000, true); f.dnPx = 111; w.push_back(f); w.push_back(cnd(115, 105, 106));   // fails at 1
      ACandle g = cnd(108, 100, 106); g.dn = ab(2000, true); g.dnPx = 101; w.push_back(g);   // a new low at 2, after the failure
      r = absRead(w, 20, 1); CHECK(r.verdict == 1 && r.head.find("above the absorbed low at 101") != std::string::npos);
      ACandle h = cnd(125, 110, 112); h.up = ab(2000, true); h.upPx = 124; w.push_back(h); w.push_back(cnd(127, 118, 126));   // a high at 3 fails at 4: newest
      r = absRead(w, 20, 1); CHECK(r.verdict == 1 && r.head == "Bullish ? - the absorbed high at 124 failed" && r.watch.find("The absorbed low at 101 still holds.") != std::string::npos); }
    // the window: only the last ARD_WIN candles
    { std::vector<ACandle> v; ACandle e = cnd(110, 100, 108); e.dn = ab(2500, true); v.push_back(e); for (int i = 0; i < ARD_WIN; ++i) v.push_back(cnd(112, 104, 108));
      CHECK(absRead(v, 20, 1).verdict == 0); v.erase(v.begin() + 1); CHECK(absRead(v, 20, 1).verdict == 1); }
    // a sweep name and a missing price
    { std::vector<ACandle> v; ACandle e = cnd(110, 100, 108); e.dn = ab(2500, true); e.swDn = "ONL"; v.push_back(e); CHECK(absRead(v, 20, 1).why.compare(0, 18, "At the ONL sweep. ") == 0);
      v[0].dnPx = 0; CHECK(absRead(v, 20, 1).verdict == 0); }
    // wrapRead: whole words, the width, the cut
    { const auto tw = [](const std::string& s) { return (int)s.size() * 6; };
      std::vector<std::string> L = wrapRead("one two three four five six", 60, 3, tw); CHECK(L.size() == 3 && L[0] == "one two" && L[1] == "three four" && L[2] == "five six");
      L = wrapRead("one two three four five six", 60, 2, tw); CHECK(L.size() == 2 && L[1].size() >= 3 && L[1].substr(L[1].size() - 3) == "..." && tw(L[1]) <= 60);
      CHECK(wrapRead("x", 60, 0, tw).empty() && wrapRead("", 60, 2, tw).empty()); L = wrapRead("abcdefghijklmnop", 30, 2, tw); CHECK(L.size() == 1);
      for (int w = 20; w < 400; w += 7) for (int m = 1; m < 5; ++m) { L = wrapRead("Bullish ? - the absorbed high at 5.6380 failed and more words here", w, m, tw); CHECK((int)L.size() <= m); } }
    // the heaviest price's tick: highest such price at the top, lowest at the bottom; none = the candle's end
    { std::vector<Row> rs; for (long long t = 100; t <= 107; ++t) { Row r; r.tick = t; r.buy = t == 106 || t == 107 ? 9 : 1; r.sell = t == 100 || t == 101 ? 7 : 1; rs.push_back(r); }
      const Bars b = barRatios(rs, 100, 107); CHECK(b.topMax == 9 && b.topMaxTick == 107 && b.botMax == 7 && b.botMaxTick == 100); }
    // (1.4.1) sessionOf: 17:00 CT starts a session (RT seconds since 1904, CT clock)
    { const long long d = 45000LL * 86400; CHECK(sessionOf(d + 16 * 3600 + 3599) == sessionOf(d + 9 * 3600)); CHECK(sessionOf(d + 17 * 3600) == sessionOf(d + 9 * 3600) + 1);
      CHECK(sessionOf(d + 17 * 3600) == sessionOf(d + 86400 + 15 * 3600)); CHECK(sessionOf(100) == -1); }
    std::printf("%d checks, %d failed\n", nCheck, nFail); return nFail ? 1 : 0;
}
