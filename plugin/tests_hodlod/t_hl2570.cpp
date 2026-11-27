// t_hl2570.cpp -- lsHodLod 2.57.0 / HodLodStops 1.2.0 / NowLevels 1.9.0 (Rassul 2026-11-26 19:01-19:10 CT, 11-27 08:15-08:16 "go";
// StopsInBox-mockup.html / LevelSwept-mockup.html): the Stops section is a column inside the HOD / LOD box after a grey divider; the Swept
// chip's 2nd line (time run · how long ago, 8 pt, dim); "stops" / "swept" inside the Next chip (8 pt, dim); R1 / R2 / S1 / S2 = stops.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2570.cpp -o t && ./t
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodSweeps.h"
#include "../HodLodStops.h"
#include "../NowLevels.h"
#include <cstdio>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static int M(const std::string& s, bool b) { return (int)s.size() * 6 + (b ? (int)s.size() : 0); }
static long long A(int y, int mo, int d, int h, int mi) { return hl::daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60; }
int main() {
    C(std::string(hl::HL_VERSION) == "2.57.0" && std::string(hlset::PLUGIN_VERSION) == "2.57.0" && std::string(stp::VERSION) == "1.2.0" && std::string(nowl::VERSION) == "1.9.0", "versions");
    std::vector<stp::Lv> k = { { "PDH", 7214 }, { "PFH", 7224 }, { "AsiaHI", 7254.75 }, { "LonLO", 7213.75 }, { "PDL", 6982.5 } };
    std::vector<stp::Bar> b = { { A(2026, 11, 25, 17, 45), 7215, 7200, 7210 }, { A(2026, 11, 25, 18, 21), 7225, 7210, 7220 }, { A(2026, 11, 26, 8, 0), 7246, 7240, 7244.75 } };
    const stp::Out O = stp::compute(k, b, 510, 0.25, 7244.75);
    // the column inside the box
    const stp::Draw D0 = stp::layout(O, M, 0, 100000, 0, 0, 1000000, true);
    C(D0.ok, "dry run");
    bool rect = false; for (const stp::Item& it : D0.items) if (it.k == 0) rect = true;
    C(!rect, "in the box: no border of its own");
    const int stW = D0.r - (D0.l + stp::PX), stH = D0.b - D0.t + 2;
    hl::View v; v.sxHi = { { "PHH", 42, 35 } }; v.sxLo = { { "PHL", 35, 33 } };
    const hsb::Draw H0 = hsb::layout(v, M, 0, 600, 0, 2000), H = hsb::layout(v, M, 0, 600, 0, 2000, stW + 2, stH);
    C(H.ok && H.r - H.l == H0.r - H0.l + stW + 2, "the box is wider by the column");
    C(H.b - H.t == (std::max)(H0.b - H0.t, stH), "as tall as its tallest column");
    int divs = 0; for (const hsb::Item& it : H.items) if (it.k == 2 && it.l == it.r && it.l == H.r - (stW + 2)) divs++;
    C(divs == 1, "a grey divider before the column");
    const stp::Draw D = stp::layout(O, M, H.r - stW, H.b, H.b - H.t, H.t, H.r + 1, true);
    C(D.ok && D.r == H.r && D.t == H.t, "the column fills the box's right part, top to bottom");
    bool inside = true; for (const stp::Item& it : D.items) if (it.k == 1 && (it.l < H.r - stW || it.r > H.r)) inside = false;
    C(inside, "its text inside the box");
    // the Swept chip's 2nd line
    auto yOf = [](double px) { return (int)(17 + (7260.0 - px) * 5.45); };
    std::string t; const std::vector<stp::Item> ch = stp::chips(O, yOf, M, 0, 900, 30, 500, {}, &t, A(2026, 11, 26, 8, 0));
    bool two = false, small = false;
    for (const stp::Item& it : ch) { if (it.k == 1 && it.s == "5:45 \xB7 14h 15m") { two = true; small = it.fs == 8 && it.col == stp::C_SWEPT2; } }
    C(two && small, "PDH: 2nd line '5:45 · 14h 15m' at 8 pt, dim: " + t);
    C(t.find("PFH=Swept 6:21 \xB7 13h 39m") != std::string::npos, "PFH: 6:21 · 13h 39m: " + t);
    { std::vector<stp::Bar> b2 = b; b2.push_back(stp::Bar{ A(2026, 11, 26, 9, 0), 7260, 7244, 7250 });
      const stp::Out o2 = stp::compute(k, b2, 510, 0.25, 7250); std::string t2; stp::chips(o2, yOf, M, 0, 900, 30, 500, {}, &t2, A(2026, 11, 26, 9, 20));
      C(t2.find("AsiaHI=Swept 9:00 \xB7 20m") != std::string::npos, "under an hour: '9:00 · 20m': " + t2); }
    { bool tall = false; for (const stp::Item& it : ch) if (it.k == 0 && it.b - it.t > 13) tall = true; C(tall, "a Swept chip is two rows tall"); }
    // R1 / R2 / S1 / S2: stops
    C(stp::status(O, "R2") == 0 && stp::status(O, "S1") == 0 && stp::status(O, "DPoc") == -1 && stp::status(O, "R") == -1, "R1 / R2 / S1 / S2 = stops");
    // the Next chip tag
    std::vector<nowl::Seg> segs; { nowl::Seg a; a.s = "Next"; a.col = nowl::C_GOLD; segs.push_back(a); nowl::Seg c; c.arrow = -1; c.s2 = "PDL"; c.bold = true; c.chip = true; c.tag = "stops"; c.tagCol = stp::C_SWEPT2; segs.push_back(c); }
    std::vector<nowl::Seg> plain = segs; plain[1].tag.clear();
    const nowl::Draw L = nowl::layout(segs, M, 400, 500, 0, 800, 10), L0 = nowl::layout(plain, M, 400, 500, 0, 800, 10);
    bool tag = false; int chipR = -1, tagR = -1;
    for (const nowl::Item& it : L.items) { if (it.k == 1 && it.s == "stops") { tag = it.fs == 8 && it.col == stp::C_SWEPT2; tagR = it.r; } }
    for (const nowl::Item& it : L.items) if (it.k == 0 && it.fill != nowl::C_BG) chipR = it.r;
    C(tag && L.r - L.l > L0.r - L0.l, "the tag: 8 pt, dim, the line widens for it");
    C(tagR > 0 && chipR >= tagR, "the tag sits INSIDE the chip");
    std::printf("t_hl2570: %d pass, %d fail\n", ok, f);
    return f ? 1 : 0;
}
