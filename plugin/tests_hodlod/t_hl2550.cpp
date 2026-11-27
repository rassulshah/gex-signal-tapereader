// t_hl2550.cpp -- lsHodLod 2.55.0 / HodLodStops 1.0.0 / NowLevels 1.8.0 (Rassul 2026-11-26 15:53-16:18 CT, StopsBusy-mockup.html, "build"):
// Stops resting / Stops run from the real ES session of 11/26 (key levels + 3-min bars), the windows (Asia after 1:00, London after 7:30,
// PhrH / ON from the open; PDL from 5:00 PM - 16:18 "if during london session price goes below pdl it will be mentioned in the stops run"),
// 1 tick through (a touch is not a run), volume levels left out, 2 names a line, grows upward; the box re-arranged; "Next" on the Level line.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2550.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodSweeps.h"
#include "../HodLodStops.h"
#include "../NowLevels.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
static int M(const std::string& s, bool b) { return (int)s.size() * 6 + (b ? (int)s.size() : 0); }
static long long abs_(int y, int mo, int d, int h, int mi) { return hl::daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60; }
static std::vector<stp::Bar> bars(long long upTo) {
    std::vector<stp::Bar> v; std::ifstream in("es1126_stop_bars.csv"); std::string ln;
    while (std::getline(in, ln)) { int y, mo, d, h, mi; double hi, lo, c; if (std::sscanf(ln.c_str(), "%d-%d-%d %d:%d,%lf,%lf,%lf", &y, &mo, &d, &h, &mi, &hi, &lo, &c) != 8) continue;
        const long long t = abs_(y, mo, d, h, mi); if (t <= upTo) v.push_back(stp::Bar{ t, hi, lo, c }); }
    return v; }
static std::string nm(const std::vector<stp::Lvl>& v) { return stp::names(v); }
int main() {
    C(std::string(hl::HL_VERSION) >= "2.55.0" && std::string(hl::HL_VERSION) == std::string(hlset::PLUGIN_VERSION) && std::string(stp::VERSION) >= "1.0.0" && std::string(nowl::VERSION) == "1.8.0", "versions");
    std::ifstream in("es1126_keys.csv"); std::stringstream ss; ss << in.rdbuf(); std::vector<hsw::Lv> K0;
    C(hsw::parseKeys(ss.str(), 1795729660LL, 3600, K0) && K0.size() >= 15, "real key levels parsed");
    std::vector<stp::Lv> K; for (const hsw::Lv& k : K0) K.push_back(stp::Lv{ k.name, k.px });
    // 11:54 AM, the end of the Thanksgiving session (price 7232.75)
    std::vector<stp::Bar> B = bars(abs_(2026, 11, 26, 11, 57));
    const stp::Out O = stp::compute(K, B, 510, 0.25, B.back().c);
    C(O.ok && nm(O.runHi) == "PhrH ONH LonHI AsiaHI PDH PFH", "11:54 highs run: " + nm(O.runHi));
    C(nm(O.runLo) == "PhrL", "11:54 lows run (1.1.0 Asia to 2:00 AM: the 1:12 AM bar is inside Asia): " + nm(O.runLo));
    C(nm(O.restUp) == "CWH PWH" && nm(O.restDn) == "LonLO AsiaLO", "11:54 resting: " + nm(O.restUp) + " / " + nm(O.restDn));
    for (const stp::Lvl& x : O.runHi) C(x.name != "DPoc" && x.name != "VAH" && x.name != "WPoc", "no volume levels");
    // 8:00 AM (price 7244.75): PDH 5:45 PM, PFH 6:21 PM run overnight; AsiaHI = 7254.75 touched, not run
    B = bars(abs_(2026, 11, 26, 8, 0));
    const stp::Out O8 = stp::compute(K, B, 510, 0.25, B.back().c);
    C(nm(O8.runHi) == "PDH PFH" && nm(O8.runLo) == "-", "8:00 run (no low run: London held above the Asia low): " + nm(O8.runHi) + " / " + nm(O8.runLo));
    C(has(nm(O8.restUp), "PhrH") || has(nm(O8.restUp), "ONH"), "8:00 resting above: " + nm(O8.restUp));
    for (const stp::Lvl& x : O8.runHi) if (x.name == "PDH") C(x.runT == abs_(2026, 11, 25, 17, 45), "PDH run at 5:45 PM");
    // windows: a level inside its own window is never 'run'; ONH / PhrH only from the open
    { std::vector<stp::Lv> k = { { "AsiaHI", 100.0 }, { "LonHI", 100.0 }, { "ONH", 100.0 }, { "PDL", 90.0 }, { "PDH", 100.0 } };
      std::vector<stp::Bar> b = { { abs_(2026, 11, 26, 0, 30), 101, 99, 100 } };          // 0:30 AM, inside Asia: the high is still forming
      stp::Out o = stp::compute(k, b, 510, 0.25, 100); C(nm(o.runHi) == "PDH", "inside Asia only PDH runs: " + nm(o.runHi));
      b.push_back(stp::Bar{ abs_(2026, 11, 26, 3, 0), 101, 89, 95 });                       // 3:00 AM London: above AsiaHI, below PDL
      o = stp::compute(k, b, 510, 0.25, 95);
      C(nm(o.runHi) == "AsiaHI PDH" && nm(o.runLo) == "PDL", "London through AsiaHI and PDL (16:18): " + nm(o.runHi) + " / " + nm(o.runLo));
      b.push_back(stp::Bar{ abs_(2026, 11, 26, 7, 30), 101.5, 95, 96 });                    // 7:30 still London: LonHI not formed yet
      o = stp::compute(k, b, 510, 0.25, 96); C(!has(nm(o.runHi), "LonHI") && !has(nm(o.runHi), "ONH"), "LonHI / ONH not before their windows");
      b.push_back(stp::Bar{ abs_(2026, 11, 26, 8, 36), 101.5, 96, 101 });                   // 8:36 RTH
      o = stp::compute(k, b, 510, 0.25, 101); C(nm(o.runHi) == "ONH LonHI AsiaHI PDH", "after the open: " + nm(o.runHi));
      std::vector<stp::Bar> t = { { abs_(2026, 11, 26, 9, 0), 100.0, 95, 99 } };          // a touch of PDH (no tick through)
      o = stp::compute(k, t, 510, 0.25, 99); C(!has(nm(o.runHi), "PDH"), "a touch is not a run"); }
    C(!stp::compute({}, B, 510, 0.25, 1).ok && !stp::compute(K, {}, 510, 0.25, 1).ok && !stp::compute(K, B, 510, 0, 1).ok, "no keys / bars / tick: not drawn");
    // layout: right of the box, bottom on the pane, at least the box's height, 2 names a line, grows upward
    const stp::Draw D = stp::layout(O, M, 700, 900, 120, 100, 1200);
    C(D.ok && D.l == 699 && D.b == 900 && D.b - D.t >= 120, "docked right of the box, bottom on the pane");
    int lines = 0; bool hiLab = false, loLab = false, upA = false; int yRest = -1, yRun = -1;
    for (const stp::Item& it : D.items) {
        if (it.k != 1) continue;
        if (it.s == "PhrH ONH" || it.s == "LonHI AsiaHI" || it.s == "PDH PFH" || it.s == "PhrL") lines++;
        if (it.s == "Highs") hiLab = it.col == stp::C_RED;
        if (it.s == "Lows") loLab = it.col == stp::C_GREEN;
        if (it.s == "Stops resting") yRest = it.t;
        if (it.s == "Stops run") yRun = it.t;
    }
    for (const stp::Item& it : D.items) if (it.k == 2 && it.col == stp::C_RED) upA = true;   // (1.1.1) the up arrow is drawn (red lines)
    C(lines == 4 && hiLab && loLab && upA, "2 names a line, red / green labels, drawn arrows");
    C(yRest >= 0 && yRun > yRest, "resting on top, run below");
    { int yLine = -1; for (const stp::Item& it : D.items) if (it.k == 2 && it.t < yRun && it.t > yRest + stp::ROW) yLine = it.t; C(yLine > 0 && yLine < yRun, "the divider above 'Stops run'"); }
    stp::Out big = O; for (int i = 0; i < 6; i++) big.runHi.push_back(big.runHi[0]);
    const stp::Draw D2 = stp::layout(big, M, 700, 900, 120, 100, 1200);
    C(D2.ok && D2.t < D.t && D2.b == 900, "more levels: the section grows upward, every one shown");
    C(!stp::layout(big, M, 700, 900, 120, 860, 1200).ok, "no room upward: not drawn (never cut)");
    C(!stp::layout(O, M, 1150, 900, 120, 100, 1200).ok, "no room before the last bar: not drawn");
    // the box: 1st / 2nd stacked left, swept columns PRE-HR ON PD PF DPoc VA
    hl::View v; v.sxHi = { { "PHH", 42, 35 }, { "ON3", 36, 36 }, { "DPoc", 23, 19 }, { "VAH", 20, 18 }, { "PDH", 13, 15 }, { "PFH", 12, 13 } };
    v.sxLo = { { "PHL", 35, 33 }, { "ON3", 29, 30 }, { "DPoc", 25, 18 }, { "VAL", 9, 11 }, { "PDL", 8, 9 }, { "PFL", 5, 7 } };
    const hsb::Draw H = hsb::layout(v, M, 0, 600, 0, 1600);
    int xP = -1, xD = -1, xV = -1, xF = -1, y1 = -1, y2 = -1, xE = -1, xR = -1;
    for (const hsb::Item& it : H.items) {
        if (it.k != 1) continue;
        if (it.s == "PD") xP = it.l;
        if (it.s == "PF") xF = it.l;
        if (it.s == "DPoc") xD = it.l;
        if (it.s == "VA") xV = it.l;
        if (it.s == "1st" && y1 < 0) y1 = it.t;
        if (it.s == "2nd") y2 = it.t;
        if (it.s == "Exp" && xE < 0) xE = it.l;
        if (it.s == "HOD swept") xR = it.l;
    }
    C(H.ok && xP > 0 && xP < xF && xF < xD && xD < xV, "swept columns PRE-HR ON PD PF then DPoc VA");
    C(y1 >= 0 && y2 > y1 + 3 * 18 && xR > xE, "1st over 2nd on the left, the stats + swept on the right");
    std::printf("t_hl2550: %d pass, %d fail\n", ok, f);
    return f ? 1 : 0;
}
