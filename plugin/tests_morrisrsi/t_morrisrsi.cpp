// t_morrisrsi.cpp -- MorrisRsiLogic.h 0.1.3 tests (0.1.3: 1-bar pivots, ES 11-20 08:36 -> 09:00) / 0.1.2 tests (0.1.2: undercut-swing divergences, ES 11-19/20 fixture, mirror, 60-candle limit, restart replay)
#include "../MorrisRsiLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <map>
#ifndef ES_FIX
#define ES_FIX "tests_morrisrsi/mrsi_es1120_bars.txt"
#endif
static int ok = 0, bad = 0;
#define C(c, m) do { if (c) ++ok; else { ++bad; std::printf("FAIL %d: %s\n", __LINE__, m); } } while (0)
static mrsi::Bar bar(long long t, double c, double h = -1, double l = -1) { mrsi::Bar b; b.t = t; b.c = c; b.h = h < 0 ? c + 0.5 : h; b.l = l < 0 ? c - 0.5 : l; return b; }
static bool near(double a, double b, double e = 1e-9) { return std::fabs(a - b) <= e; }
int main(int argc, char** argv) {
    using namespace mrsi;
    { // hand calc: 20 changes, 12 up by 1, 8 down by 2 -> AG = 12/20, AL = 16/20, RS = 0.75, RSI = 100 - 100/1.75
      Engine E; double c = 100; E.add(bar(0, c)); for (int k = 0; k < 20; ++k) { c += k < 12 ? 1 : -2; E.add(bar(k + 1, c)); }
      C(near(E.raw[20], 100.0 - 100.0 / 1.75), "RSI hand calc (simple means over 20 changes)"); C(std::isnan(E.raw[19]), "no RSI before 20 changes"); }
    { // Morris window: a change 21 candles back does not count (Wilder's would)
      Engine A, B2; double c = 100; A.add(bar(0, c)); B2.add(bar(0, c)); c += 50; A.add(bar(1, c)); B2.add(bar(1, 100 + 1));
      double ca = c, cb = 101; for (int k = 0; k < 20; ++k) { ca += k % 2 ? 1 : -1; cb += k % 2 ? 1 : -1; A.add(bar(k + 2, ca)); B2.add(bar(k + 2, cb)); }
      C(near(A.raw[21], B2.raw[21]), "Morris: only the last 20 changes count"); }
    { // smoothing = simple 3-candle mean of the raw RSI
      Engine E; double c = 100; E.add(bar(0, c)); for (int k = 0; k < 30; ++k) { c += (k * 7 % 5) - 2; E.add(bar(k + 1, c)); }
      C(near(E.val[25], (E.raw[23] + E.raw[24] + E.raw[25]) / 3), "3-candle simple smoothing"); C(std::isnan(E.val[21]), "smoothing needs 3 RSI values"); C(!std::isnan(E.val[22]), "first smoothed value at 22"); }
    { // AL = 0 -> 100, no change -> 50
      Engine E; double c = 100; E.add(bar(0, c)); for (int k = 0; k < 20; ++k) { c += 1; E.add(bar(k + 1, c)); } C(near(E.raw[20], 100), "all gains = 100");
      Engine F; for (int k = 0; k < 21; ++k) F.add(bar(k, 100)); C(near(F.raw[20], 50), "flat = 50"); }
    C(colourOf(29.9) == C_GREEN && colourOf(30) == C_BLUE && colourOf(70) == C_BLUE && colourOf(70.1) == C_RED && colourOf(50) == C_BLUE, "colours: green < 30, blue 30-70, red > 70");
    // divergence fixtures: build price paths with explicit pivots
    auto run = [](const std::vector<double>& closes, const std::vector<double>& lows) { Engine E; for (size_t i = 0; i < closes.size(); ++i) E.add(bar((long long)i * 180, closes[i], closes[i] + 0.25, lows[i])); return E; };
    { // bullish: a fall, pivot low A, a bounce, a slower fall to a LOWER low B with less momentum -> RSI higher at B; then a bounce confirms B
      std::vector<double> c, l; double p = 200; for (int k = 0; k < 25; ++k) { p -= 1.0; c.push_back(p); l.push_back(p - 0.25); }          // steady fall -> RSI low
      for (int k = 0; k < 4; ++k) { p += 1.5; c.push_back(p); l.push_back(p - 0.25); }                                                     // bounce confirms A
      for (int k = 0; k < 8; ++k) { p -= 0.95; c.push_back(p); l.push_back(p - 0.25); }                                                    // slower fall, lower low
      for (int k = 0; k < 6; ++k) { p += 1.5; c.push_back(p); l.push_back(p - 0.25); }                                                     // bounce confirms B
      Engine E = run(c, l);
      bool bull = false; for (const Dv& d : E.dv) if (d.dir == 1) bull = true;
      C(bull, "bullish divergence: lower low, higher RSI, 1st RSI <= 40");
      C(E.dv.size() == 1 && E.dv[0].t1 == 24 * 180, "consecutive case: the Dv starts at the 1st low (A)");
      // 0.1.2 60-candle limit: the same path with the 2nd low n candles later (slow drift down) - 40 apart = marked, 61+ apart = none
      auto drift = [&](int nChop) { std::vector<double> c2, l2; double q = 200; for (int k = 0; k < 25; ++k) { q -= 1.0; c2.push_back(q); l2.push_back(q - 0.25); }
        for (int k = 0; k < 4; ++k) { q += 1.5; c2.push_back(q); l2.push_back(q - 0.25); }
        for (int k = 0; k < nChop; ++k) { q += (k % 2 ? 0.6 : -0.6); c2.push_back(q); l2.push_back(q - 0.25); }      // sideways above A
        for (int k = 0; k < 4; ++k) { q -= 2.0; c2.push_back(q); l2.push_back(q - 0.25); }                             // drop under A
        for (int k = 0; k < 6; ++k) { q += 1.5; c2.push_back(q); l2.push_back(q - 0.25); }
        Engine F = run(c2, l2); int gap = -1; for (const Dv& d : F.dv) if (d.dir == 1) gap = (int)((d.t2 - d.t1) / 180); return gap; };
      const int g38 = drift(30), g60 = drift(52), g61 = drift(53), g70 = drift(62);
      C(g38 == 38, "38 candles apart = marked (0.1.1 missed these)"); C(g60 == 60, "exactly 60 candles apart = marked");
      C(g61 == -1, "61 candles apart = no divergence"); C(g70 == -1, "more than 60 candles apart = no divergence"); }
    { // 0.1.2 THE CASE: a deep low A (RSI ~20), then a chop of SMALLER ZigZag lows above A, then a lower low B ~44 candles later with a higher RSI
      std::vector<double> c, l; double p = 200; for (int k = 0; k < 25; ++k) { p -= 1.0; c.push_back(p); l.push_back(p - 0.25); }   // A at index 24
      for (int k = 0; k < 4; ++k) { p += 1.0; c.push_back(p); l.push_back(p - 0.25); }
      for (int r = 0; r < 4; ++r) { for (int k = 0; k < 4; ++k) { p -= 0.8; c.push_back(p); l.push_back(p - 0.25); } for (int k = 0; k < 4; ++k) { p += 0.9; c.push_back(p); l.push_back(p - 0.25); } }  // small lows above A
      for (int k = 0; k < 8; ++k) { p -= 0.9; c.push_back(p); l.push_back(p - 0.25); }    // B under A
      for (int k = 0; k < 6; ++k) { p += 1.5; c.push_back(p); l.push_back(p - 0.25); }
      Engine E = run(c, l); int lowsBetween = 0; for (const int j : E.lows) if (j > 24 && E.B[(size_t)j].l > E.B[24].l) ++lowsBetween;
      bool hit = false; for (const Dv& d : E.dv) if (d.dir == 1 && d.t1 == 24 * 180 && E.B[(size_t)(d.t2 / 180)].l < E.B[24].l) hit = true;
      C(lowsBetween >= 2, "fixture: smaller ZigZag lows sit between A and B");
      C(hit, "undercut swing: B is compared with A (not the consecutive small low) -> bullish Dv");
      int perPivot = 0; std::map<long long, int> cnt; for (const Dv& d : E.dv) if (++cnt[d.t2] > 1) ++perPivot; C(perPivot == 0, "at most one Dv per pivot");
      // mirror: the same path upside down = a bearish Dv from the high A
      std::vector<double> mc, mh; for (size_t i = 0; i < c.size(); ++i) { mc.push_back(400 - c[i]); mh.push_back(400 - l[i]); }
      Engine M; for (size_t i = 0; i < mc.size(); ++i) M.add(bar((long long)i * 180, mc[i], mh[i], mc[i] - 0.25));
      bool mhit = false, mbull = false; for (const Dv& d : M.dv) { if (d.dir == -1 && d.t1 == 24 * 180) mhit = true; if (d.dir == 1) mbull = true; }
      C(mhit && !mbull, "mirror: highest pivot high overshot with a lower RSI -> bearish Dv"); }
    { // 0.1.2 his ES overnight 11-19/20 (research 3-min bars, labels = bar START): low 7,149.75 at 11:15 PM (RSI 20.3), lower low 7,144.50
      // at 1:27 AM (RSI 42.9), 44 candles apart with smaller lows between - 0.1.1 did not mark it
      std::ifstream f(ES_FIX); Engine E; long long t; double c, h, l; int n = 0; while (f >> t >> c >> h >> l) { mrsi::Bar b; b.t = t; b.c = c; b.h = h; b.l = l; E.add(b); ++n; }
      bool hit = false; for (const Dv& d : E.dv) if (d.dir == 1 && d.t1 == 27904500LL && d.t2 == 27912420LL) hit = true;
      C(n > 300, "ES fixture loaded"); C(hit, "ES 11-19/20: bullish Dv 11:15 PM 7,149.75 -> 1:27 AM 7,144.50 is marked"); }
    { // the 40 zone: a higher RSI low but the 1st RSI above 40 = no divergence
      Engine E; std::vector<double> c, l; double p = 100;
      for (int k = 0; k < 25; ++k) { p += (k % 2 ? 1.0 : -0.9); c.push_back(p); l.push_back(p - 0.25); }   // choppy, RSI ~ 50
      for (int k = 0; k < 3; ++k) { p -= 1; c.push_back(p); l.push_back(p - 0.25); } for (int k = 0; k < 4; ++k) { p += 1.2; c.push_back(p); l.push_back(p - 0.25); }
      for (int k = 0; k < 3; ++k) { p -= 1.5; c.push_back(p); l.push_back(p - 0.25); } for (int k = 0; k < 4; ++k) { p += 1.2; c.push_back(p); l.push_back(p - 0.25); }
      for (size_t i = 0; i < c.size(); ++i) E.add(bar((long long)i * 180, c[i], c[i] + 0.25, l[i]));
      bool bad40 = false; for (const Dv& d : E.dv) { const int i1 = (int)(d.t1 / 180); if (d.dir == 1 && E.val[(size_t)i1] > BULL_ZONE) bad40 = true; if (d.dir == -1 && E.val[(size_t)i1] < BEAR_ZONE) bad40 = true; }
      C(!bad40, "every divergence starts in its zone (<= 40 / >= 60)"); }
    { // no repaint: feeding candle by candle gives the same divergences as one pass, and a decided one never changes
      std::vector<mrsi::Bar> X; double p = 100; unsigned s = 7;
      for (int i = 0; i < 1500; ++i) { s = s * 1103515245u + 12345u; const double d = ((int)(s >> 16) % 21 - 10) * 0.25; p += d; X.push_back(bar(i * 180, p, p + ((s >> 8) % 5) * 0.25, p - ((s >> 4) % 5) * 0.25)); }
      Engine A; for (const auto& b : X) A.add(b);
      Engine B2; std::vector<Dv> seen; bool changed = false;
      for (const auto& b : X) { B2.add(b); for (size_t k = 0; k < seen.size(); ++k) if (B2.dv[k].t1 != seen[k].t1 || B2.dv[k].t2 != seen[k].t2 || B2.dv[k].dir != seen[k].dir) changed = true; seen = B2.dv; }
      C(!changed, "a decided divergence never changes (replay)");
      { // 0.1.2 restart after every candle (fresh engine over the candles so far, record merged by t2 as the host does) = the one-pass record
        std::map<long long, Dv> rec; bool rchg = false;
        for (size_t m = 50; m <= X.size(); m += 1) { if (m < X.size() && m % 7 && m < 1400) continue;   // every 7th candle + the last 100 each one
          Engine R; for (size_t i = 0; i < m; ++i) R.add(X[i]);
          for (const Dv& d : R.dv) { auto it = rec.find(d.t2); if (it == rec.end()) rec[d.t2] = d; else if (it->second.t1 != d.t1 || it->second.dir != d.dir) rchg = true; } }
        std::map<long long, Dv> one; for (const Dv& d : A.dv) one[d.t2] = d; bool eq = rec.size() == one.size();
        for (const auto& kv : one) { auto it = rec.find(kv.first); if (it == rec.end() || it->second.t1 != kv.second.t1 || it->second.dir != kv.second.dir) eq = false; }
        C(!rchg && eq, "restart replay: record identical to one pass, no decision changed"); } C(A.dv.size() == B2.dv.size() && !A.dv.empty(), "replay = one pass, divergences found");
      bool same = true; for (size_t i = 0; i < A.val.size(); ++i) if (!(std::isnan(A.val[i]) && std::isnan(B2.val[i])) && A.val[i] != B2.val[i]) same = false; C(same, "values identical");
      // the live value = what the candle gets once closed
      Engine L; for (size_t i = 0; i + 1 < X.size(); ++i) L.add(X[i]); const double lv = L.liveValue(X.back().c); L.add(X.back()); C(near(lv, L.val.back()), "live value = the closed value");
      // record file round trip + strict parse
      std::map<long long, Dv> D; for (const Dv& d : A.dv) D[d.t2] = d; const std::string tx = dvText("ES", 180, D);
      std::map<long long, Dv> R; C(dvParse(tx, "ES", 180, R) == (int)D.size() && R.size() == D.size(), "record round trip");
      std::map<long long, Dv> W; C(dvParse(tx, "NQ", 180, W) == -1 && W.empty(), "wrong market = rejected");
      std::map<long long, Dv> G; C(dvParse("MRSI|1|ES|180|0.1.0\n12|x|1\n5|3|1\n7|9|2\n10|20|-1\n", "ES", 180, G) == 1, "garbled lines skipped");
      // two charts on one DLL = two engines, no shared state
      Engine E1, E2; for (int i = 0; i < 100; ++i) { E1.add(X[(size_t)i]); E2.add(bar(i * 60, 5000 + i)); } C(E1.B.size() == 100 && E2.raw[50] == 100.0, "independent engines"); }
    { // layout: lines inside the pane, the title, colours, Dv tag below a bullish point
      std::vector<mrsi::Bar> X; double p = 100; unsigned s = 11;
      for (int i = 0; i < 400; ++i) { s = s * 1103515245u + 12345u; p += ((int)(s >> 16) % 21 - 10) * 0.25; X.push_back(bar(i * 180, p, p + 0.5, p - 0.5)); }
      Engine E; for (const auto& b : X) E.add(b);
      View v; v.l = 0; v.t = 0; v.r = 1000; v.b = 200; v.first = 0; v.last = 399; v.X = [](int j) { return (double)j * 2.5; }; v.textW = [](const std::string& t) { return (int)t.size() * 7; };
      const auto P = layout(E, v); bool inside = true, title = false, seg = false; int dvTags = 0;
      for (const auto& q : P) { if (q.kind == 0 && (q.y0 < 0 || q.y0 > 200 || q.y1 < 0 || q.y1 > 200)) inside = false; if (q.kind == 1 && q.text.find(std::string("Morris RSI v") + VERSION) == 0) title = true; if (q.kind == 0 && q.width == 2) seg = true; if (q.kind == 1 && q.text == "Dv") ++dvTags; }
      int vis = 0; for (const Dv& d : E.dv) if (d.t2 / 180 <= 399) ++vis;
      C(inside && title && seg, "layout: inside the pane, title with version, RSI segments"); C(dvTags == vis, "one Dv tag per divergence");
      C(yOf(v, 70) < yOf(v, 30) && yOf(v, 100) == 0 && yOf(v, 0) == 200, "0 - 100 axis");
      // 0.1.1: 70 / 30 = short 1 px dashes in red / green, no grey 50 line
      const int y70 = yOf(v, 70), y30 = yOf(v, 30), y50 = yOf(v, 50); int d70 = 0, d30 = 0, grey = 0, at50 = 0; bool thin = true, shortD = true, gaps = true; int prev70 = -100;
      for (const auto& q : P) { if (q.kind != 0) continue; if (q.col == C_GREY) ++grey;
        if (q.y0 == q.y1 && q.y0 == y50 && q.x0 == v.l && q.x1 == v.r) ++at50;
        const bool horiz = q.y0 == q.y1 && q.width == 1 && q.dash;
        if (horiz && q.y0 == y70 && q.col == C_RED) { ++d70; if (q.x0 - prev70 < DASH_ON + DASH_OFF) gaps = false; prev70 = q.x0; if (q.x1 - q.x0 + 1 > DASH_ON) shortD = false; }
        if (horiz && q.y0 == y30 && q.col == C_GREEN) { ++d30; if (q.x1 - q.x0 + 1 > DASH_ON) shortD = false; }
        if (q.dash && q.width != 1) thin = false; }
      C(grey == 0 && at50 == 0, "no grey 50 line");
      C(d70 >= 100 && d30 == d70, "70 / 30 lines drawn as many short dashes across the pane");
      C(thin && shortD && gaps, "dashes 1 px wide, <= DASH_ON px long, with gaps");
      bool curveSame = true; for (const auto& q : P) if (q.kind == 0 && !q.dash && q.width == 2 && q.col != C_BLUE && q.col != C_RED && q.col != C_GREEN) curveSame = false; C(curveSame, "RSI curve: width 2, blue / red / green");
      View tiny = v; tiny.r = 5; tiny.b = 5; C(layout(E, tiny).empty(), "too small a pane draws nothing"); }
    // parity with the Python reference on ES 11-19 (es_close.txt: t c h l per line; ref_rsi.txt: index value)
    if (argc > 2) { std::ifstream f(argv[1]); Engine E; long long t; double c, h, l; while (f >> t >> c >> h >> l) { mrsi::Bar b; b.t = t; b.c = c; b.h = h; b.l = l; E.add(b); }
      std::ifstream r(argv[2]); int i; double x; int cmp = 0, diff = 0; double mx = 0; while (r >> i >> x) { ++cmp; const double d = std::fabs(E.val[(size_t)i] - x); mx = std::max(mx, d); if (d > 1e-6) ++diff; }
      std::printf("PARITY: %d values compared, %d differ, max |diff| %.2e; divergences %zu\n", cmp, diff, mx, E.dv.size()); C(cmp > 100 && diff == 0, "parity with the Python reference"); }
    { // 0.1.3 his ES 11-20 (10:58 CT "same issue"): high 7,174.50 at 08:36 (RSI 78.3), higher high 7,216.75 at 09:00 (RSI 69.7) -> bearish Dv
      // research 3-min bars (labels = bar START); the 3-bar walk had no pivot at 08:27 / 08:36 (no low under the previous 3 lows)
      std::ifstream f("tests_morrisrsi/mrsi_es1120_am.txt"); Engine E; long long t; double h, l, c; std::string d, tm; std::map<long long, std::string> T; long long i = 0;
      while (f >> t >> h >> l >> c >> d >> tm) { Bar b; b.t = i; b.h = h; b.l = l; b.c = c; E.add(b); T[i++] = tm; }
      bool hit = false; for (const Dv& x : E.dv) if (T[x.t2] == "09:00:00" && T[x.t1] == "08:36:00" && x.dir == -1) hit = true;
      { const std::string m = "0.1.3: ES 11-20 08:36 -> 09:00 bearish Dv marked (" + std::to_string(i) + " candles)"; C(i > 300 && hit, m.c_str()); }
      C(ZZ_REV == 1 && std::string(VERSION) == "0.1.3", "0.1.3: 1-bar walk, version"); }
    std::printf("%s %d pass, %d fail\n", bad ? "FAILED" : "ALL PASS", ok, bad); return bad;
}
