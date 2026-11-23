// t_vr1100.cpp -- VolRangeLogic 1.10.0 (Rassul 2026-11-23 07:26 CT "are the rules for the cx badge and the cx label on the volume different",
// 07:32 "go with badge for both", 07:33 "why is cx on the range ?"): the Volume pane's Cx = the price chart's Cx (the marks file), the Range
// pane has no Cx, the pane's own pivot rule is gone. Real candles: NQ 2026-11-23 (his first screenshot) and ES 2026-11-20 (his second).
#include "VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int ok = 0, bad = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++bad; std::printf("FAIL %s\n", m); } }
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int idx(const vrr::Engine& E, long long localEnd) { for (size_t i = 0; i < E.LT.size(); ++i) if (E.LT[i] == localEnd) return (int)i; return -1; }
static vrr::Engine load(const char* bars, const std::string& wy, int mode, bool oneByOne = false) {
    vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180; E.hlOn = true;
    { std::map<long long, vrr::Mk> w; const int k = vrr::parseWy(wy, w); if (k >= 0) { E.wy.swap(w); E.wyLines = k; } }
    std::ifstream f(bars); std::string l;
    while (std::getline(f, l)) { vrr::Bar b; long long L = 0; if (std::sscanf(l.c_str(), "%lld,%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &L, &b.o, &b.h, &b.l, &b.c, &b.v) != 7) continue; E.addBar(b, L); if (oneByOne) { E.markLevels(); (void)E.markOf((int)E.V.size() - 1); } }
    E.markLevels(); return E;
}
static int count(const vrr::Engine& E, int code) { int k = 0; for (int i = 0; i < (int)E.T.size(); ++i) if (E.markOf(i).code == code) ++k; return k; }
static std::string dropLine(const std::string& s, const std::string& start) { std::string o; std::stringstream in(s); std::string l; while (std::getline(in, l)) if (l.compare(0, start.size(), start) != 0) o += l + "\n"; return o; }
int main() {
    // ---- the parser: Cx lines, with and without a level; a Bk / Sw is never replaced by a Cx; the count returned stays the Bk / Sw count
    { std::map<long long, vrr::Mk> w;
      const int k = vrr::parseWy("WY|1|NQ|180|0.22.1\n100|Cx|1||0|\n101|Cx|-1|AHI|103780|\n102|Bk|1|x|5|V10\n103|Cx|2||0|\n104|Cx|0||0|\nabc|Cx|1\n105|Sw|-1|PDH|5|\n102|Cx|-1||0|\n106|Cx|1\n107|Cx|-1||0|\n107|Sw|1|PDL|9|\n108|Tst|1|x|0|62%\n109|E|1\n", w);
      C(k == 3, "the count returned = Bk / Sw lines only (3)"); C(w.size() == 6, "6 candles: 100 101 102 105 106 107 (no 103 / 104 / 108 / 109)");
      C(w[100].code == vrr::MK_P && w[100].dir == 1 && w[101].code == vrr::MK_P && w[101].dir == -1 && w[106].code == vrr::MK_P, "Cx read with its direction, with or without a level, 3 fields enough");
      C(!w.count(103) && !w.count(104) && !w.count(108) && !w.count(109), "a Cx with a bad direction and every other code are dropped");
      C(w[102].code == vrr::MK_BK && w[102].dir == 1, "a Cx line after a Bk on the same candle does not replace the Bk");
      C(w[107].code == vrr::MK_SW && w[107].dir == 1, "a Sw line after a Cx on the same candle replaces the Cx");
      std::map<long long, vrr::Mk> g; C(vrr::parseWy("100|Cx|1||0|\n", g) == -1 && g.empty(), "no header = not parsed"); }
    const std::string wyNQ = slurp("wy_NQ_1123.txt"), wyES = slurp("wy_ES_1123.txt");
    const long long d23 = 1795392000LL, d20 = 1795132800LL;   // 2026-11-23 / 2026-11-20 00:00 local
    auto at = [](long long day, int h, int m) { return day + h * 3600LL + m * 60LL; };
    // ---- NQ this morning, the Volume pane: Cx exactly where the price chart has it
    { const vrr::Engine E = load("nq_1123.csv", wyNQ, vrr::VOL);
      const int a = idx(E, at(d23, 6, 21)), b = idx(E, at(d23, 6, 45)), c = idx(E, at(d23, 6, 48)); C(a > 0 && b > 0 && c > 0 && E.V.size() > 500, "NQ candles loaded");
      C(E.markOf(a).code == 0 && E.S[(size_t)a].mx() == 31 && E.S[(size_t)a].all == 1, "6:21 AM: no Cx any more (biggest green in 31, of all in 1 - the badge's test fails), the 31 stays");
      C(E.markOf(b).code == vrr::MK_P && E.markOf(b).dir == 1 && E.S[(size_t)b].mx() == 69, "6:45 AM: Cx, green side (a pivot low) - the candle with the badge; 69");
      C(E.markOf(c).code == 0 && E.S[(size_t)c].mx() == 41, "6:48 AM: no Cx any more (the pane had it here, one candle after the badge), the 41 stays");
      int inFile = 0, shown = 0, stray = 0;
      for (int i = 0; i < (int)E.T.size(); ++i) { const auto w = E.wy.find(E.T[(size_t)i]); const bool f = w != E.wy.end() && w->second.code == vrr::MK_P; const vrr::Mk m = E.markOf(i);
          if (f) { ++inFile; if (m.code == vrr::MK_P && m.dir == w->second.dir) ++shown; }
          else if (m.code == vrr::MK_P) ++stray; }
      C(inFile >= 5 && shown == inFile, "every Cx of the file on a loaded candle is a Cx in the pane, same direction"); C(stray == 0, "no Cx the price chart does not have");
      C(E.cxAt(b) == 1 && E.cxAt(c) == 0 && E.cxAt(-1) == 0 && E.cxAt((int)E.T.size()) == 0, "cxAt: the file's Cx, 0 elsewhere and out of range");
      // the label is drawn over the badge's candle, bold, in the candle's colour; the two candles that lost it are grey bars with their number
      vrr::View v; const int first = idx(E, at(d23, 5, 57)), last = idx(E, at(d23, 7, 12)); v.l = 0; v.t = 0; v.r = 100 + (last - first + 1) * 24; v.b = 64; v.first = first; v.last = last; v.bw = 24; v.textH = 11;
      v.X = [first](int j) { return 112.0 + (j - first) * 24; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
      const std::vector<vrr::Prim> P = vrr::layout(E, v, nullptr);
      int cx = 0; double cxX = -1; unsigned cxCol = 0; bool cxBold = false; unsigned colA = 0, colB = 0, colC = 0; bool n31 = false, n41 = false, n69 = false;
      for (const vrr::Prim& p : P) { const double mid = (p.x0 + p.x1) / 2.0;
          if (!p.fill && p.text == "Cx") { ++cx; cxX = mid; cxCol = p.col; cxBold = p.bold; }
          if (p.fill && std::fabs(mid - v.X(a)) < 1.5) colA = p.col;
          if (p.fill && std::fabs(mid - v.X(b)) < 1.5) colB = p.col;
          if (p.fill && std::fabs(mid - v.X(c)) < 1.5) colC = p.col;
          if (!p.fill && p.text == "31" && std::fabs(mid - v.X(a)) < 4) n31 = true;
          if (!p.fill && p.text == "41" && std::fabs(mid - v.X(c)) < 4) n41 = true;
          if (!p.fill && p.text == "69" && std::fabs(mid - v.X(b)) < 4) n69 = true; }
      C(cx == 1 && std::fabs(cxX - v.X(b)) < 4 && cxBold && cxCol == vrr::C_UP, "drawn: ONE Cx in the window, over the 6:45 candle, bold, green (the candle's colour)");
      C(colB == vrr::C_UP && colA == vrr::C_GREY && colC == vrr::C_GREY, "drawn: the 6:45 bar green, the 6:21 and 6:48 bars grey (unmarked, under 50)");
      C(n31 && n41 && n69, "drawn: the numbers 31, 69 and 41 stay over their bars");
      const std::string st = vrr::statusText(E, "NQ", "NQZ6", 180);
      C(st.find("VERSION,1.11.0") == 0 && st.find("WY,1.8.0 Bk / Sw from the price chart's marks file,") != std::string::npos && st.find("CX,1.10.0 Cx from the price chart's marks file," + std::to_string(E.wyCx()) + " Cx read") != std::string::npos, "status: version, the WY line as before, the new CX line");
      C(st.find(",P " + std::to_string(shown) + ",") != std::string::npos, "status: the P count = the Cx shown");
      // the label appears when the badge does: without the 6:45 line the candle is only 'biggest in 50+' (green, no letters)
      const vrr::Engine N = load("nq_1123.csv", dropLine(wyNQ, std::to_string(E.T[(size_t)b]) + "|Cx|"), vrr::VOL);
      C(N.markOf(b).code == vrr::MK_BIG && N.markOf(a).code == 0 && N.markOf(c).code == 0, "before the price chart writes the 6:45 Cx: no Cx anywhere near (the pane never decides it itself)");
      // bar by bar = one pass
      const vrr::Engine S1 = load("nq_1123.csv", wyNQ, vrr::VOL, true); bool same = S1.T.size() == E.T.size();
      for (int i = 0; same && i < (int)E.T.size(); ++i) { const vrr::Mk x = E.markOf(i), y = S1.markOf(i); if (x.code != y.code || x.dir != y.dir || E.S[(size_t)i].mx() != S1.S[(size_t)i].mx()) same = false; }
      C(same, "bar by bar = one straight pass (marks and numbers)");
      // priority as before: CB, SR and Ct stand over a Cx
      vrr::Engine Q = load("nq_1123.csv", wyNQ, vrr::VOL); Q.cbAt[Q.T[(size_t)b]] = 1; C(Q.markOf(b).code == vrr::MK_CB, "CB over Cx");
      vrr::Engine Q2 = load("nq_1123.csv", wyNQ, vrr::VOL); Q2.srAt[Q2.T[(size_t)b]] = -1; C(Q2.markOf(b).code == vrr::MK_SR, "SR over Cx");
      vrr::Engine Q3 = load("nq_1123.csv", wyNQ, vrr::VOL); Q3.ctAt[Q3.T[(size_t)b]] = -1; C(Q3.markOf(b).code == vrr::MK_CT, "Ct over Cx");
      // no file = no Cx
      vrr::Engine Z = load("nq_1123.csv", wyNQ, vrr::VOL); Z.wy.clear(); Z.wyLines = -1;
      C(count(Z, vrr::MK_P) == 0 && count(Z, vrr::MK_BK) == 0 && count(Z, vrr::MK_SW) == 0, "no marks file = no Cx, no Bk, no Sw");
      C(vrr::statusText(Z, "NQ", "NQZ6", 180).find("CX,1.10.0 Cx from the price chart's marks file,no file - no Cx") != std::string::npos, "status says no file - no Cx"); }
    // ---- the Range pane: no Cx at all, Bk / Sw as the Volume pane
    { const vrr::Engine V = load("nq_1123.csv", wyNQ, vrr::VOL), R = load("nq_1123.csv", wyNQ, vrr::RANGE);
      C(count(R, vrr::MK_P) == 0 && R.wyCx() >= 5, "Range pane: no Cx although the file has them");
      bool lv = true, zero = true;
      for (int i = 0; i < (int)R.T.size(); ++i) { const int a = V.markOf(i).code, b = R.markOf(i).code;
          if ((a == vrr::MK_BK || a == vrr::MK_SW || b == vrr::MK_BK || b == vrr::MK_SW) && a != b) lv = false;
          if (R.cxAt(i)) zero = false; }
      C(lv, "Range pane: Bk / Sw on the same candles as the Volume pane"); C(zero, "Range pane: cxAt is 0 on every candle (its read never says 'climax volume')");
      C(vrr::statusText(R, "NQ", "NQZ6", 180).find("CX,1.10.0 no Cx in the Range pane (Cx = climactic volume)") != std::string::npos, "Range status: no Cx in the Range pane"); }
    // ---- ES Friday (his second screenshot)
    { const vrr::Engine V = load("es_1120.csv", wyES, vrr::VOL), R = load("es_1120.csv", wyES, vrr::RANGE);
      const int a = idx(V, at(d20, 12, 21)), b = idx(V, at(d20, 12, 57)), c = idx(V, at(d20, 13, 33)), r1 = idx(R, at(d20, 12, 36)), r2 = idx(R, at(d20, 13, 0)); C(a > 0 && b > 0 && c > 0 && r1 > 0 && r2 > 0, "ES candles loaded");
      C(V.markOf(a).code == vrr::MK_P && V.markOf(a).dir == -1 && V.markOf(b).code == vrr::MK_P && V.markOf(b).dir == -1, "ES 12:21 and 12:57 PM: Cx at a pivot high, the two candles with the badge");
      C(V.markOf(c).code == vrr::MK_BIG && V.S[(size_t)c].mx() == 51 && V.S[(size_t)c].all == 11, "ES 1:33 PM: no Cx any more (biggest red in 51, of all in 11); still a 50+ bar");
      C(R.markOf(r1).code != vrr::MK_P && R.S[(size_t)r1].mx() == 20 && R.markOf(r2).code != vrr::MK_P && R.S[(size_t)r2].mx() == 38, "ES Range 12:36 PM and 1:00 PM: no Cx, the 20 and the 38 stay");
      vrr::View v; const int first = idx(R, at(d20, 12, 6)), last = idx(R, at(d20, 13, 45)); v.l = 0; v.t = 0; v.r = 100 + (last - first + 1) * 22; v.b = 56; v.first = first; v.last = last; v.bw = 22; v.textH = 11;
      v.X = [first](int j) { return 111.0 + (j - first) * 22; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
      int rc = 0, n20 = 0, n38 = 0;
      for (const vrr::Prim& p : vrr::layout(R, v, nullptr)) {
          if (!p.fill && p.text == "Cx") ++rc;
          if (!p.fill && p.text == "20") ++n20;
          if (!p.fill && p.text == "38") ++n38; }
      C(rc == 0 && n20 >= 1 && n38 >= 1, "drawn: no Cx in the Range pane, the numbers 20 and 38 stay");
      int vc = 0; for (const vrr::Prim& p : vrr::layout(V, v, nullptr)) if (!p.fill && p.text == "Cx") ++vc; C(vc == 2, "drawn: two Cx in the Volume pane window (12:21, 12:57)"); }
    C(std::string(vrr::VERSION) == "1.11.0", "version 1.11.0 (the Cx rule of 1.10.0 unchanged; t_vr1110 covers the AR)");
    std::printf("%s %d pass, %d fail\n", bad ? "FAILED" : "ALL PASS", ok, bad); return bad;
}
