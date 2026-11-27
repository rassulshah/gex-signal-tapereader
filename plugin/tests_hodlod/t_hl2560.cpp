// t_hl2560.cpp -- lsHodLod 2.56.0 / HodLodStops 1.1.0 (Rassul 2026-11-26 17:11-17:21 CT, LevelSwept-mockup.html, "make the updates whatever
// you can and build"): gold "Stops" / grey "Swept" chips on the stop levels' lines (centre, never on each other or a box), stops / swept
// status by name (the Level line's short names too), Asia 6:00 PM - 2:00 AM CT.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2560.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodSweeps.h"
#include "../HodLodStops.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static int M(const std::string& s, bool b) { return (int)s.size() * 6 + (b ? (int)s.size() : 0); }
static long long A(int y, int mo, int d, int h, int mi) { return hl::daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60; }
int main() {
    C(std::string(hl::HL_VERSION) >= "2.56.0" && std::string(hl::HL_VERSION) == std::string(hlset::PLUGIN_VERSION) && std::string(stp::VERSION) >= "1.1.0", "versions");
    std::ifstream in("es1126_keys.csv"); std::stringstream ss; ss << in.rdbuf(); std::vector<hsw::Lv> K0; hsw::parseKeys(ss.str(), 1795729660LL, 3600, K0);
    std::vector<stp::Lv> K; for (const hsw::Lv& k : K0) K.push_back(stp::Lv{ k.name, k.px });
    std::vector<stp::Bar> B; { std::ifstream bi("es1126_stop_bars.csv"); std::string ln; while (std::getline(bi, ln)) { int y, mo, d, h, mi; double hi, lo, c;
        if (std::sscanf(ln.c_str(), "%d-%d-%d %d:%d,%lf,%lf,%lf", &y, &mo, &d, &h, &mi, &hi, &lo, &c) == 8 && A(y, mo, d, h, mi) <= A(2026, 11, 26, 8, 0)) B.push_back(stp::Bar{ A(y, mo, d, h, mi), hi, lo, c }); } }
    const stp::Out O = stp::compute(K, B, 510, 0.25, B.back().c);
    C(stp::status(O, "PDH") == 1 && stp::status(O, "PFH") == 1, "PDH / PFH swept at 8:00");
    C(stp::status(O, "LonLO") == 0 && stp::status(O, "LLO") == 0 && stp::status(O, "ONL") == 0 && stp::status(O, "PDL") == 0, "LonLO / ONL / PDL: stops (short name LLO too)");
    C(stp::status(O, "AsiaLO") == 0 && stp::status(O, "ALO") == 0, "AsiaLO: stops - Asia to 2:00 AM, the 1:12 AM bar is inside the window, London stayed above");
    C(stp::status(O, "ONH/LonHI/AsiaHI") == 0 && stp::status(O, "AHI") == 0, "combined name / AHI: stops");
    C(stp::status(O, "DPoc") == -1 && stp::status(O, "VAH") == -1 && stp::status(O, "R2") == 0, "volume levels: no tag; (1.2.0) R2 = stops");
    C(stp::status(stp::Out(), "PDH") == -1, "no data: no tag");
    // Asia window: a bar at 1:30 AM is still Asia (no run); at 2:03 AM it counts
    { std::vector<stp::Lv> k = { { "AsiaLO", 100.0 } };
      std::vector<stp::Bar> b = { { A(2026, 11, 26, 1, 30), 101, 99, 100 } };
      C(stp::status(stp::compute(k, b, 510, 0.25, 100), "AsiaLO") == 0, "1:30 AM is inside Asia");
      b.push_back(stp::Bar{ A(2026, 11, 26, 2, 3), 101, 99.5, 100 });
      C(stp::status(stp::compute(k, b, 510, 0.25, 100), "AsiaLO") == 1, "2:03 AM through the Asia low = swept"); }
    // chips: a pane 0..1000 x 0..500, price 7260 at y 17, 5.45 px a point
    auto yOf = [](double px) { return (int)(17 + (7260.0 - px) * 5.45); };
    std::string t; std::vector<stp::Box> av = { { 0, 430, 700, 500 } };   // the HOD / LOD box at the bottom
    const std::vector<stp::Item> ch = stp::chips(O, yOf, M, 0, 900, 30, 500, av, &t);
    int sw = 0, stp_ = 0; bool over = false, inBox = false;
    std::vector<stp::Box> bx;
    for (const stp::Item& it : ch) { if (it.k == 1) { if (it.s.rfind("Swept", 0) == 0) { sw++; C(it.col == stp::C_SWEPT, "Swept grey"); } if (it.s == "Stops") { stp_++; C(it.col == stp::C_STOPS, "Stops gold"); } }
        if (it.k == 0) { stp::Box b{ it.l, it.t, it.r, it.b }; for (const stp::Box& p : bx) if (stp::hit(b, p)) over = true; for (const stp::Box& p : av) if (stp::hit(b, p)) inBox = true; bx.push_back(b); } }
    C(sw == 2 && stp_ >= 4, "2 Swept (PDH, PFH) + the Stops chips on screen: " + t);
    C(!over && !inBox, "chips never on each other or on a box");
    C(t.find("PDH=Swept 5:45") != std::string::npos && t.find("PFH=Swept 6:21") != std::string::npos && t.find("LonLO=Stops") != std::string::npos && t.find("DPoc") == std::string::npos, "names: " + t);
    { bool centred = false; for (const stp::Item& it : ch) if (it.k == 0 && it.s.empty() && std::abs((it.l + it.r) / 2 - 450) <= 1) centred = true; C(centred, "the first chip in the centre of the price area"); }
    { const std::vector<stp::Item> none = stp::chips(O, yOf, M, 0, 900, 30, 60, av); bool far = false; for (const stp::Item& it : none) if (it.t < 30 || it.b > 60) far = true; C(!far, "off-screen levels get no chip"); }
    // (1.1.1, 18:57 "if multiple levels are the same, show only one badge") ONH / LonHI / AsiaHI 7254.75 = one chip; PFH + PDH at one price = one chip
    C(t.find("ONH/LonHI/AsiaHI=Stops") != std::string::npos && t.find("ONH=Stops") == std::string::npos, "same price: one chip: " + t);
    { std::vector<stp::Lv> k = { { "PFH", 7256.25 }, { "PDH", 7256.25 }, { "PDL", 7232.5 } };
      std::vector<stp::Bar> b = { { A(2026, 11, 27, 9, 0), 7250, 7240, 7245 } };
      const stp::Out o = stp::compute(k, b, 510, 0.25, 7245); std::string tt;
      const std::vector<stp::Item> c = stp::chips(o, yOf, M, 0, 900, 30, 500, {}, &tt); int n = 0; for (const stp::Item& it : c) if (it.k == 1) n++;
      C(n == 2 && tt.find("PDH/PFH=Stops") != std::string::npos, "PFH / PDH 7256.25 (his 11-27 chart): one Stops chip: " + tt);
      b.push_back(stp::Bar{ A(2026, 11, 27, 9, 3), 7257, 7244, 7246 });
      const stp::Out o2 = stp::compute(k, b, 510, 0.25, 7246); std::string t2; stp::chips(o2, yOf, M, 0, 900, 30, 500, {}, &t2);
      C(t2.find("PDH/PFH=Swept 9:03") != std::string::npos, "both run: one Swept chip with the time: " + t2); }
    std::printf("t_hl2560: %d pass, %d fail\n", ok, f);
    return f ? 1 : 0;
}
