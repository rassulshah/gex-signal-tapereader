// t_vr1110.cpp -- VolRangeLogic 1.11.0 (Rassul 2026-11-23 09:25-09:34 CT "i want you to implement the AR which is an automatic rally bar that occurs after
// a cx ... has a BRMax of at least 10", "the AR bar should also have a strength line", "go"): the Range pane shows AR on the candle the price chart marked AR
// (the AR lines lsRevRead 0.23.0 writes into the marks file), the Volume pane does not. Real candles: NQ 2026-11-20 .. 11-23 (the mockup's windows).
#include "VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int ok = 0, bad = 0; static void C(bool c, const char* m) { if (c) ++ok; else { ++bad; std::printf("FAIL %s\n", m); } }
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int idx(const vrr::Engine& E, long long localEnd) { for (size_t i = 0; i < E.LT.size(); ++i) if (E.LT[i] == localEnd) return (int)i; return -1; }
static vrr::Engine load(const char* bars, const std::string& wy, int mode, bool oneByOne = false) {
    vrr::Engine E; E.mode = mode; E.tick = 0.25; E.spb = 180; E.hlOn = true;
    { std::map<long long, vrr::Mk> w; std::map<long long, int> ar; const int k = vrr::parseWy(wy, w, &ar); if (k >= 0) { E.wy.swap(w); E.wyAr.swap(ar); E.wyLines = k; } }
    std::ifstream f(bars); std::string l;
    while (std::getline(f, l)) { vrr::Bar b; long long L = 0; if (std::sscanf(l.c_str(), "%lld,%lld,%lf,%lf,%lf,%lf,%lf", &b.t, &L, &b.o, &b.h, &b.l, &b.c, &b.v) != 7) continue; E.addBar(b, L); if (oneByOne) { E.markLevels(); (void)E.markOf((int)E.V.size() - 1); } }
    E.markLevels(); return E;
}
static int count(const vrr::Engine& E, int code) { int k = 0; for (int i = 0; i < (int)E.T.size(); ++i) if (E.markOf(i).code == code) ++k; return k; }
int main() {
    const long long E1904 = 2082844800LL, d23 = 1795392000LL, d20 = 1795132800LL;
    auto at = [](long long day, int h, int m) { return day + h * 3600LL + m * 60LL; };
    auto line = [&](long long local, int dir, const char* st) { return std::to_string(local + E1904) + "|AR|" + std::to_string(dir) + "||0|" + st + "\n"; };
    // ---- the parser
    { std::map<long long, vrr::Mk> w; std::map<long long, int> ar;
      const std::string t = "WY|1|NQ|180|0.23.0\n100|AR|1||0|R51\n101|AR|-1||0|V99+\n102|Bk|1|x|5|V10\n102|AR|1||0|R20\n103|AR|2||0|R12\n104|AR|1\nabc|AR|1\n105|Cx|-1||0|\n105|AR|-1||0|R30\n";
      C(vrr::parseWy(t, w, &ar) == 1, "the count returned stays the Bk / Sw count (1)");
      C(ar.size() == 5 && ar[100] == 1 && ar[101] == -1 && ar[102] == 1 && ar[104] == 1 && ar[105] == -1 && !ar.count(103), "AR lines read with their direction (3 fields enough); a bad direction dropped");
      C(w.size() == 2 && w[102].code == vrr::MK_BK && w[105].code == vrr::MK_P, "an AR never replaces the Bk or the Cx of its candle: it is kept apart");
      std::map<long long, vrr::Mk> w2; C(vrr::parseWy(t, w2) == 1 && w2.size() == 2, "without the AR map (older callers): the same marks, no AR"); }
    const std::string wyNQ = slurp("wy_NQ_1123.txt");
    const std::string wyAR = wyNQ + line(at(d23, 6, 51), 1, "R51") + line(at(d20, 8, 33), -1, "V99+") + line(at(d20, 8, 48), 1, "V99+");
    // ---- the Range pane: AR on the price chart's AR candles
    { const vrr::Engine R = load("nq_1123.csv", wyAR, vrr::RANGE), V = load("nq_1123.csv", wyAR, vrr::VOL);
      const int a = idx(R, at(d23, 6, 51)), b = idx(R, at(d20, 8, 33)), c = idx(R, at(d20, 8, 48)); C(a > 0 && b > 0 && c > 0, "NQ candles loaded");
      C(R.markOf(a).code == vrr::MK_AR && R.markOf(a).dir == 1 && R.S[(size_t)a].mx() == 51, "Range: 6:51 AM = AR, green, BRMax 51");
      C(R.markOf(b).code == vrr::MK_AR && R.markOf(b).dir == -1 && R.markOf(c).code == vrr::MK_AR && R.markOf(c).dir == 1, "Range: Friday 8:33 AM = AR red, 8:48 AM = AR green");
      C(count(R, vrr::MK_AR) == 3 && R.arAt(a) == 1 && R.arAt(a - 1) == 0 && R.arAt(-1) == 0 && R.arAt((int)R.T.size()) == 0, "Range: three AR, none elsewhere");
      C(count(V, vrr::MK_AR) == 0 && V.arAt(a) == 0 && V.markOf(a).code == 0, "Volume: no AR (6:51 AM stays an unmarked bar)");
      C(V.markOf(idx(V, at(d23, 6, 45))).code == vrr::MK_P && R.markOf(idx(R, at(d23, 6, 45))).code != vrr::MK_P, "the Cx rule of 1.10.0 is untouched: Volume has the 6:45 Cx, Range has none");
      // drawn: AR bold in the candle's colour over the 6:51 bar, the 51 under it, the bar green
      vrr::View v; const int first = idx(R, at(d23, 5, 57)), last = idx(R, at(d23, 7, 12)); v.l = 0; v.t = 0; v.r = 100 + (last - first + 1) * 24; v.b = 60; v.first = first; v.last = last; v.bw = 24; v.textH = 11;
      v.X = [first](int j) { return 112.0 + (j - first) * 24; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
      int ar = 0, n51 = 0; unsigned arCol = 0, barCol = 0; bool arBold = false; double arX = -1;
      for (const vrr::Prim& p : vrr::layout(R, v, nullptr)) { const double mid = (p.x0 + p.x1) / 2.0;
          if (!p.fill && p.text == "AR") { ++ar; arCol = p.col; arBold = p.bold; arX = mid; }
          if (!p.fill && p.text == "51" && std::fabs(mid - v.X(a)) < 4) ++n51;
          if (p.fill && std::fabs(mid - v.X(a)) < 1.5) barCol = p.col; }
      C(ar == 1 && std::fabs(arX - v.X(a)) < 4 && arBold && arCol == vrr::C_UP && n51 == 1 && barCol == vrr::C_UP, "drawn (Range): ONE AR over the 6:51 bar, bold, green, its 51, the bar green");
      int va = 0; for (const vrr::Prim& p : vrr::layout(V, v, nullptr)) if (!p.fill && p.text == "AR") ++va; C(va == 0, "drawn (Volume): no AR");
      C(vrr::statusText(R, "NQ", "NQZ6", 180).find("AR,1.11.0 AR from the price chart's marks file,3 AR read") != std::string::npos, "Range status: 3 AR read");
      C(vrr::statusText(V, "NQ", "NQZ6", 180).find("AR,1.11.0 no AR in the Volume pane (AR = a range bar)") != std::string::npos, "Volume status: no AR in the Volume pane");
      C(vrr::statusText(R, "NQ", "NQZ6", 180).find("VERSION,1.14.0") == 0 && vrr::statusText(R, "NQ", "NQZ6", 180).find("CX,1.10.0 no Cx in the Range pane") != std::string::npos, "status: version 1.11.0, the CX line as before");
      // a Bk / Sw on the same candle keeps the label; CB / SR / Ct stand over an AR
      int bk = -1; for (int i = 0; i < (int)R.T.size(); ++i) if (R.markOf(i).code == vrr::MK_BK) bk = i;
      C(bk > 0, "a Bk candle found");
      if (bk > 0) { const vrr::Engine Q = load("nq_1123.csv", wyAR + line(R.LT[(size_t)bk], 1, "R20"), vrr::RANGE); C(Q.markOf(bk).code == vrr::MK_BK && Q.arAt(bk) == 1, "an AR on a Bk candle: the pane keeps Bk (one label per bar)"); }
      { vrr::Engine Q = load("nq_1123.csv", wyAR, vrr::RANGE); Q.cbAt[Q.T[(size_t)a]] = 1; C(Q.markOf(a).code == vrr::MK_CB, "CB over AR"); }
      { vrr::Engine Q = load("nq_1123.csv", wyAR, vrr::RANGE); Q.srAt[Q.T[(size_t)a]] = 1; C(Q.markOf(a).code == vrr::MK_SR, "SR over AR"); }
      { vrr::Engine Q = load("nq_1123.csv", wyAR, vrr::RANGE); Q.ctAt[Q.T[(size_t)a]] = -1; C(Q.markOf(a).code == vrr::MK_CT, "Ct over AR"); }
      // the number on an AR bar shows from 10 (as on a Bk / Sw bar): an unmarked bar with a count of 10..19 gets its number only once it is an AR
      int q = -1; for (int i = first; i <= last; ++i) if (R.markOf(i).code == 0 && R.S[(size_t)i].mx() >= 10 && R.S[(size_t)i].mx() < 20) { q = i; break; }
      C(q > 0, "a bar with a count of 10..19 in the window");
      if (q > 0) { const std::string num = std::to_string(R.S[(size_t)q].mx()); auto has = [&](const vrr::Engine& E) { for (const vrr::Prim& p : vrr::layout(E, v, nullptr)) if (!p.fill && p.text == num && std::fabs((p.x0 + p.x1) / 2.0 - v.X(q)) < 4) return true; return false; };
          const vrr::Engine Q = load("nq_1123.csv", wyAR + line(R.LT[(size_t)q], R.G[(size_t)q] ? 1 : -1, "R12"), vrr::RANGE);
          C(!has(R) && has(Q) && Q.markOf(q).code == vrr::MK_AR, "an AR bar shows its number from 10 up; the same bar unmarked does not"); }
      // no AR lines = the pane of 1.10.0; no file = nothing
      { const vrr::Engine P = load("nq_1123.csv", wyNQ, vrr::RANGE); C(count(P, vrr::MK_AR) == 0 && P.wyAr.empty(), "no AR lines in the file: no AR");
        bool same = true; for (int i = 0; i < (int)R.T.size(); ++i) { if (R.arAt(i)) continue; const vrr::Mk x = R.markOf(i), y = P.markOf(i); if (x.code != y.code || x.dir != y.dir) same = false; } C(same, "every other bar's mark is what it was without the AR lines"); }
      { vrr::Engine Z = load("nq_1123.csv", wyAR, vrr::RANGE); Z.wy.clear(); Z.wyAr.clear(); Z.wyLines = -1; C(count(Z, vrr::MK_AR) == 0 && vrr::statusText(Z, "NQ", "NQZ6", 180).find("AR,1.11.0 AR from the price chart's marks file,no file - no AR") != std::string::npos, "no marks file: no AR, the status says so"); }
      { const vrr::Engine S1 = load("nq_1123.csv", wyAR, vrr::RANGE, true); bool same = true; for (int i = 0; i < (int)R.T.size(); ++i) { const vrr::Mk x = R.markOf(i), y = S1.markOf(i); if (x.code != y.code || x.dir != y.dir) same = false; } C(same, "bar by bar = one straight pass"); } }
    C(std::string(vrr::mkText(vrr::MK_AR)) == "AR" && vrr::MK_CB > vrr::MK_AR && vrr::MK_AR > vrr::MK_P, "MK_AR: a labelled code; MK_CB stays the last code");
    C(std::string(vrr::VERSION) == "1.14.0", "version 1.11.0");
    std::printf("%s %d pass, %d fail\n", bad ? "FAILED" : "ALL PASS", ok, bad); return bad;
}
