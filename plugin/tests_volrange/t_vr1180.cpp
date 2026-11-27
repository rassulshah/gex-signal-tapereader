// VolRangeLogic 1.18.0 - Thrust / PIC (Range) and VolTH / Change (Volume) over 14 candles, on ES 2026-11-27 (IRT store bars, START stamps)
#include "../VolRangeLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int f = 0, p = 0; static void chk(bool c, const std::string& w) { std::printf("%s %s\n", c ? "PASS" : "FAIL", w.c_str()); c ? ++p : ++f; }
int main() {
    std::ifstream in("es1127b.csv"); std::string ln; std::vector<std::string> W; vrr::Engine R, V; R.mode = vrr::RANGE; V.mode = vrr::VOL;
    while (std::getline(in, ln)) { std::stringstream ss(ln); std::string c[6]; for (int k = 0; k < 6; ++k) std::getline(ss, c[k], ',');
        for (vrr::Engine* e : { &R, &V }) { e->Oo.push_back(std::stod(c[1])); e->Hh.push_back(std::stod(c[2])); e->Ll.push_back(std::stod(c[3])); e->Cc.push_back(std::stod(c[4])); e->Vol.push_back(std::stod(c[5])); }
        W.push_back(c[0].substr(11, 5)); }
    auto at = [&](const char* t) { for (size_t i = 0; i < W.size(); ++i) if (W[i] == t) return (int)i; return -1; };
    struct X { const char* t; const char* th; const char* pic; const char* vth; const char* chg; };
    const X xs[] = { { "10:36", "1.23x", "2.51x", "1.01x", "2.09x" }, { "09:06", "1.34x", "2.62x", "0.80x", "0.66x" }, { "08:30", "4.26x", "5.21x", "16.78x", "41.05x" } };
    for (const X& x : xs) { const int i = at(x.t); const vrr::ThStat a = vrr::thStat(R, i, nullptr), b = vrr::thStat(V, i, nullptr);
        chk(vrr::thText(a.a) == x.th && vrr::thText(a.b) == x.pic, std::string("Range ") + x.t + " start: Thrust " + vrr::thText(a.a) + " PIC " + vrr::thText(a.b) + " (expected " + x.th + " " + x.pic + ")");
        chk(vrr::thText(b.a) == x.vth && vrr::thText(b.b) == x.chg, std::string("Volume ") + x.t + " start: VolTH " + vrr::thText(b.a) + " Change " + vrr::thText(b.b) + " (expected " + x.vth + " " + x.chg + ")"); }
    { const vrr::ThStat a = vrr::thStat(R, 5, nullptr); chk(!std::isfinite(a.a) && vrr::thText(a.a) == "-", "fewer than 14 candles before: -"); }
    { const int n = (int)R.Cc.size(); vrr::Bar fb; fb.o = R.Cc.back(); fb.h = fb.o + 5; fb.l = fb.o - 5; fb.c = fb.o + 5; fb.v = 4000;
      const vrr::ThStat a = vrr::thStat(R, n, &fb); chk(std::isfinite(a.a) && std::isfinite(a.b), "the forming candle gets values (" + vrr::thText(a.a) + " / " + vrr::thText(a.b) + ")"); }
    { vrr::Engine D = R; const int i = at("09:06"); D.Hh[(size_t)i - 3] = 0; D.Ll[(size_t)i - 3] = 0; D.Cc[(size_t)i - 3] = 0; const vrr::ThStat a = vrr::thStat(D, i, nullptr); chk(std::isfinite(a.a), "an empty candle in the window is skipped, not a crash"); }
    chk(vrr::thText(1.0 / 3.0) == "0.33x" && vrr::thText(NAN) == "-", "2 decimals + x; none = -");
    chk(std::string(vrr::VERSION) >= "1.18.0", "VolRangeLogic 1.18.0+");
    std::printf("%s %d pass, %d fail\n", f ? "FAILED" : "ALL PASS", p, f); return f ? 1 : 0;
}
