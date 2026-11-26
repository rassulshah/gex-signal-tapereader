// t_hl2470.cpp -- lsHodLod 2.47.0 / NowLevels 1.7.0 (Rassul 2026-11-26 02:30-02:32 CT "make the level line look better . make it look nice", "add it ...
// i'll go with your recommendation" = option A of Level-Line-mockup.html): the Level line as level chips - "VOB(T) WATCH" gold text, a low below price
// in a green chip, a high above in a red chip, no "Bk / VOB(T)"; the state words stay filled badges with the level in a chip after them; NEXT / LAST BK
// grey labels with a green / red arrow and a white level. The layout: every chip a tinted box with a 1-px border of its colour, inside the line's panel.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2470.cpp -o t && ./t [dump.txt]
#include "../NowLevels.h"
#include "../HodLodLogic.h"
#include "../HodLodSettings.h"
#include <cstdio>
#include <string>
static int ok = 0, f = 0;
static void C(bool c, const std::string& m) { if (c) ++ok; else { ++f; std::printf("FAIL %s\n", m.c_str()); } }
static bool has(const std::string& s, const std::string& x) { return s.find(x) != std::string::npos; }
int main(int argc, char** argv) {
    C(std::string(nowl::VERSION) == "1.7.0" && std::string(hl::HL_VERSION) >= std::string("2.47.0") && std::string(hlset::PLUGIN_VERSION) == hl::HL_VERSION, "versions 1.7.0 / 2.47.0");
    auto meas = [](const std::string& s, bool bold) { return (int)s.size() * (bold ? 8 : 7); };
    nowl::Out o; o.ok = true; o.dn.ok = true; o.dn.name = "PDL"; o.up.ok = true; o.up.name = "AHI"; o.haveBk = true; o.bk.lv = "R1"; o.bk.dir = 1;
    const std::vector<nowl::Seg> v = nowl::segments(o); const std::string p = nowl::plain(v);
    C(p == "VOB(T) WATCH {\xE2\x86\x93 PDL} {\xE2\x86\x91 AHI} \xE2\x94\x82 LAST BK \xE2\x86\x91 R1", "WATCH line: " + p);
    C(!has(p, "Bk / VOB(T)") && !has(p, "["), "no 'Bk / VOB(T)', no filled badge on a plain WATCH");
    int chips = 0; for (const nowl::Seg& g : v) if (g.chip) { ++chips; C(g.s2 == "PDL" ? (g.col == nowl::CH_DN_T && g.bcol == nowl::CH_DN_B && g.arrow == -1) : (g.col == nowl::CH_UP_T && g.bcol == nowl::CH_UP_B && g.arrow == 1), "chip colours / arrow for " + g.s2); }
    C(chips == 2, "two level chips");
    C(v[0].s == "VOB(T) WATCH" && v[0].col == nowl::C_GOLD && !v[0].badge, "the label: gold text, no block");
    { bool gl = false, gr = false, wh = false; for (const nowl::Seg& g : v) { if (g.s == "LAST BK" && g.col == nowl::C_GREY) gl = true; if (g.arrow == 1 && !g.chip && g.col == nowl::C_GREEN) gr = true; if (g.s == "R1" && g.col == nowl::C_WHITE) wh = true; }
      C(gl && gr && wh, "LAST BK grey, arrow green, R1 white"); }
    const nowl::Draw D = nowl::layout(v, meas, 500, 300, 0, 1000, 10);
    C(D.ok && D.level == 0 && D.shown == (int)v.size(), "laid out whole");
    int boxes = 0; std::vector<nowl::Item> cb;
    for (const nowl::Item& it : D.items) { C(it.l >= D.l && it.r <= D.r && it.t >= D.t && it.b <= D.b, "every item inside the panel");
        if (it.k == 0 && (it.col == nowl::CH_DN_B || it.col == nowl::CH_UP_B)) { ++boxes; cb.push_back(it); } }
    C(boxes == 2 && cb[0].r < cb[1].l, "two chip boxes, side by side, not overlapping");
    for (const nowl::Item& it : D.items) if (it.k == 1 && (it.s == "PDL" || it.s == "AHI")) { const nowl::Item& b = it.s == "PDL" ? cb[0] : cb[1]; C(it.l > b.l && it.r < b.r, "the name sits inside its chip: " + it.s); }
    // the states
    { nowl::Out s1 = o; s1.bs.state = 2; s1.bs.dir = 1; s1.bs.name = "LonLO"; const std::string q = nowl::plain(nowl::segments(s1));
      C(has(q, "[VOB] {\xE2\x86\x91 LonLO}") && has(q, "NEXT"), "VOB state: " + q); }
    { nowl::Out s2 = o; s2.bs.state = 1; s2.bs.dir = -1; s2.bs.name = "ONL"; const std::vector<nowl::Seg> w = nowl::segments(s2); const std::string q = nowl::plain(w);
      C(has(q, "[BREAK] {\xE2\x86\x93 ONL}"), "BREAK state: " + q); bool red = false; for (const nowl::Seg& g : w) if (g.chip && g.s2 == "ONL" && g.bcol == nowl::CH_UP_B) red = true; C(red, "a bearish break: red chip"); }
    { nowl::Out s3 = o; s3.bs.state = 3; s3.bs.name = "PDH"; const std::vector<nowl::Seg> w = nowl::segments(s3); const std::string q = nowl::plain(w);
      C(has(q, "[TESTING] {PDH}"), "TESTING (live): " + q); bool gold = false; for (const nowl::Seg& g : w) if (g.chip && g.bcol == nowl::CH_GD_B) gold = true; C(gold, "the live test: gold chip (no side yet)"); }
    { nowl::Out s4 = o; s4.tst.ok = true; s4.tst.state = 2; s4.tst.dir = 1; s4.tst.lv = "PFH"; const std::string q = nowl::plain(nowl::segments(s4));
      C(has(q, "[TESTING BkTst] {\xE2\x86\x91 PFH}"), "BkTst: " + q); }
    // narrow: the LAST BK part goes first, the chips stay
    { const nowl::Draw N = nowl::layout(v, meas, 150, 300, 0, 200, 10); C(N.ok && N.level == 1, "narrow: LAST BK dropped first (level " + std::to_string(N.level) + ")"); }
    if (argc > 1) { FILE* fp = std::fopen(argv[1], "w"); if (fp) { for (const nowl::Item& it : D.items) std::fprintf(fp, "%d|%d|%d|%d|%d|%u|%u|%d|%d|%s\n", it.k, it.l, it.t, it.r, it.b, it.col, it.fill, it.bold ? 1 : 0, it.arw ? 1 : 0, it.s.c_str()); std::fclose(fp); } }
    std::printf("t_hl2470 (lsHodLod 2.47.0): %d passed, %d failed\n", ok, f); return f ? 1 : 0;
}
