// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2371.cpp -o t2371 && ./t2371   (from plugin\tests_hodlod)
// lsHodLod 2.37.1 (2026-11-19 22:35 CT, Rassul approved; his wording, mockup gamma-tag-mockup.html rows "YOUR IDEA, once the forward test
// proves it", shown WITHOUT "?"): the gamma pill at the far left of the Now row carries a label inside it.
//  [1] the exact strings: "-GAMMA \xB7 Trending \xB7 Bk / BkTst" (red) / "+GAMMA \xB7 Range \xB7 Sw / Cob" (green), cp1252 0xB7, no "?"
//  [2] room: the labelled pill at the box's left edge, then "0DTE Exp 3:00 PM"; text inside the pill
//  [3] narrow pane drop order: expiry -> " \xB7 Bk / BkTst" / " \xB7 Sw / Cob" -> " \xB7 Trending" / " \xB7 Range"; the +/-GAMMA word stays; never
//      touching the Now line; never returns to a longer form as room shrinks
//  [4] with the real centred Now line over box widths 260..1600 px: never overlaps, the Now line stays centred
//  [5] no regime / GAMMA STALE unchanged; the host uses nowLeftTag + gammaTag; versions 2.37.1 everywhere + the build list
#include <cstdio>
#include <climits>
#include <fstream>
#include <sstream>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../NowLevels.h"
#include "lib_widths.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 60) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static int MS(const std::string& s, int fs, bool b) { return measureLib(s, fs, b); }
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static int stateOf(const htop::Left& a) { return a.keep == 2 ? 0 : a.keep == 1 ? 1 + a.lvl : 4; }   // 0 full + exp, 1 full, 2 short, 3 word, 4 none

int main() {
    auto m = [](const std::string& s, int fs, bool b) { return MS(s, fs, b); };
    const std::string NEG = "-GAMMA \xB7 Trending \xB7 Bk / BkTst", POS = "+GAMMA \xB7 Range \xB7 Sw / Cob", EXP = "0DTE Exp 3:00 PM";

    std::printf("[1] exact strings\n");
    {   const std::vector<std::string> n = htop::gammaTag("-GAMMA"), p = htop::gammaTag("+GAMMA");
        CHECK(n.size() == 3 && n[0] == NEG && n[1] == "-GAMMA \xB7 Trending" && n[2] == "-GAMMA", "negative forms: full, regime word, word only");
        CHECK(p.size() == 3 && p[0] == POS && p[1] == "+GAMMA \xB7 Range" && p[2] == "+GAMMA", "positive forms: full, regime word, word only");
        CHECK(NEG.size() == 30 && (unsigned char)NEG[7] == 0xB7 && (unsigned char)NEG[18] == 0xB7 && NEG.find('\xC2') == std::string::npos, "the middle dot is one cp1252 0xB7 byte (not UTF-8)");
        CHECK(NEG.find('?') == std::string::npos && POS.find('?') == std::string::npos, "shown WITHOUT ?");
        CHECK(htop::gammaTag("GAMMA STALE").size() == 1 && htop::gammaTag("GAMMA STALE")[0] == "GAMMA STALE" && htop::gammaTag("").empty(), "GAMMA STALE as is; no regime = no pill"); }

    std::printf("[2] room\n");
    for (int sgn : { -1, 1 }) {
        const std::string pill = sgn < 0 ? "-GAMMA" : "+GAMMA", full = sgn < 0 ? NEG : POS;
        const unsigned col = sgn < 0 ? hl::C_RED : hl::C_GREEN, fill = sgn < 0 ? hl::C_PILL_R : hl::C_PILL_G;
        const htop::Left a = htop::nowLeftTag(htop::gammaTag(pill), col, fill, EXP, hl::C_SLATE, m, 100, 50, 68, 1000000, 3000);
        CHECK(a.keep == 2 && a.lvl == 0 && a.tag == full && a.items.size() == 3, pill + ": full label + expiry");
        CHECK(a.items[0].k == 3 && a.items[0].l == 100 && a.items[0].col == col && a.items[0].fill == fill, pill + ": pill outline in the regime colour, at the box's left edge");
        CHECK(a.items[1].k == 1 && a.items[1].s == full && a.items[1].col == col && a.items[1].bold && a.items[1].fs == 10, pill + ": the whole label inside the pill, regime colour, bold 10 pt");
        CHECK(a.items[1].l >= a.items[0].l + 6 && a.items[1].l + m(full, 10, true) <= a.items[0].r - 6, pill + ": label text fits inside the pill");
        CHECK(a.items[2].s == EXP && a.items[2].l > a.items[0].r && a.items[2].col == hl::C_SLATE && !a.items[2].bold, pill + ": the 0DTE expiry follows as today"); }

    std::printf("[3] narrow drop order\n");
    for (int sgn : { -1, 1 }) {
        const std::string pill = sgn < 0 ? "-GAMMA" : "+GAMMA"; const std::vector<std::string> F = htop::gammaTag(pill);
        int last = -1, seen[5] = { 0, 0, 0, 0, 0 }; bool mono = true, clear = true, word = true;
        for (int nowL = 900; nowL >= 100; --nowL) {
            const htop::Left a = htop::nowLeftTag(F, hl::C_RED, hl::C_PILL_R, EXP, hl::C_SLATE, m, 100, 50, 68, nowL, 3000);
            const int s = stateOf(a); seen[s]++; if (s < last) mono = false; last = s;
            if (a.keep > 0 && a.r + 10 > nowL) clear = false;
            if (a.keep > 0 && a.items[1].s.compare(0, 6, pill) != 0) word = false;
            const int wordW = m(pill, 10, true) + 12;
            if (a.keep == 0 && 100 + wordW + 10 <= nowL) word = false;   // the word only goes when even it would touch the Now line
            if (s == 0 && !(a.items.size() == 3 && a.items[1].s == F[0])) clear = false;
            if (s >= 1 && s <= 3 && !(a.items.size() == 2 && a.items[1].s == F[(size_t)(s - 1)])) clear = false; }
        CHECK(seen[0] && seen[1] && seen[2] && seen[3] && seen[4], pill + ": all five steps seen (full+exp, full, short, word, none)");
        CHECK(mono, pill + ": as the room shrinks: expiry first, then the setups, then the regime word part - never back to a longer form");
        CHECK(clear, pill + ": never within 10 px of the Now line; the right form drawn at each step");
        CHECK(word, pill + ": the +/-GAMMA word always stays while it fits"); }
    {   // the xMax (price axis) limit works the same way
        const htop::Left a = htop::nowLeftTag(htop::gammaTag("-GAMMA"), hl::C_RED, hl::C_PILL_R, EXP, hl::C_SLATE, m, 0, 50, 68, 1000000, m(NEG, 10, true) + 12 + 4);
        CHECK(a.keep == 1 && a.lvl == 0 && a.r <= m(NEG, 10, true) + 16, "axis limit: the expiry drops first"); }

    std::printf("[4] with the real Now line\n");
    {   std::vector<nowl::Seg> sg;
        nowl::Seg b1; b1.s = "TESTING: PDL \xB7 BkTst Rev"; b1.arrow = -1; b1.badge = true; b1.bold = true; b1.fill = nowl::C_RED; b1.col = nowl::C_BLACK; sg.push_back(b1);
        nowl::Seg sp; sp.sep = true; sg.push_back(sp);
        nowl::Seg n1; n1.s = "Next"; n1.col = nowl::C_GREY; sg.push_back(n1); nowl::Seg n2; n2.arrow = -1; n2.s2 = "2hL"; n2.col = nowl::C_GOLD; sg.push_back(n2);
        nowl::Seg n3; n3.arrow = 1; n3.s2 = "VAL"; n3.col = nowl::C_GOLD; sg.push_back(n3);
        auto mn = [](const std::string& s, bool b) { return MS(s, 10, b); };
        {   const nowl::Draw D = nowl::layout(sg, mn, 565, 200, 0, 1400, 10);
            for (int sgn : { -1, 1 }) { const std::string pill = sgn < 0 ? "-GAMMA" : "+GAMMA";
                const htop::Left a = htop::nowLeftTag(htop::gammaTag(pill), hl::C_RED, hl::C_PILL_R, EXP, hl::C_SLATE, m, 0, D.t, D.b, D.l, 1400);
                CHECK(D.ok && a.keep == 2 && a.lvl == 0 && a.r < D.l && std::abs((D.l + D.r) / 2 - 565) <= 1, pill + ": mockup case (1,130 px box): full label + expiry, the Now line centred and untouched"); } }
        int bad = 0, n = 0, full = 0, word = 0;
        for (int W = 260; W <= 1600; W += 2) {
            const nowl::Draw D = nowl::layout(sg, mn, W / 2, 200, 0, W, 10); if (!D.ok) continue;
            for (int sgn : { -1, 1 }) { const std::string pill = sgn < 0 ? "-GAMMA" : "+GAMMA";
                const htop::Left a = htop::nowLeftTag(htop::gammaTag(pill), hl::C_RED, hl::C_PILL_R, EXP, hl::C_SLATE, m, 0, D.t, D.b, D.l, W); n++;
                for (const htop::It& it : a.items) if (it.r > D.l - 10 || it.l < 0) bad++;
                full += a.keep == 2 ? 1 : 0; word += (a.keep == 1 && a.lvl == 2) ? 1 : 0; } }
        CHECK(n > 600 && bad == 0, "box widths 260..1600: the left part never reaches the Now line (" + std::to_string(n) + " layouts)");
        CHECK(full > 0 && word > 0, "both ends seen: full label + expiry on wide panes, the word only on narrow ones"); }

    std::printf("[5] fallbacks, host, versions\n");
    {   const htop::Left d = htop::nowLeftTag(htop::gammaTag(""), hl::C_GREY, 0, EXP, hl::C_SLATE, m, 100, 50, 68, 1000000, 2000);
        CHECK(d.keep == 2 && d.items.size() == 1 && d.items[0].s == EXP && d.lvl == -1, "no regime known: the expiry alone (as 2.37.0)");
        const htop::Left g = htop::nowLeftTag(htop::gammaTag("GAMMA STALE"), hl::C_GREY, hl::C_PILL_N, EXP, hl::C_SLATE, m, 100, 50, 68, 1000000, 2000);
        CHECK(g.keep == 2 && g.items[1].s == "GAMMA STALE", "GAMMA STALE unchanged");
        const htop::Left e = htop::nowLeftTag(htop::gammaTag("+GAMMA"), hl::C_GREEN, hl::C_PILL_G, "", hl::C_SLATE, m, 100, 50, 68, 1000000, 2000);
        CHECK(e.keep == 1 && e.lvl == 0 && e.items.size() == 2 && e.items[1].s == POS, "no expiry: the full label alone"); }
    {   const std::string s = rd("../HodLod.cpp");
        CHECK(s.find("htop::nowLeftTag(htop::gammaTag(pill.s), pill.col, pf, htop::zeroDte(ex)") != std::string::npos, "host draws the labelled pill");
        CHECK(s.find("htop::nowLeft(pill.s") == std::string::npos, "host no longer draws the bare pill");
        CHECK(s.find(",tag \"") != std::string::npos && s.find("gamma word only") != std::string::npos, "NOW_ROW status names the form and the tag");
        CHECK(s.find("p->setVersion(\"2.37.1\")") != std::string::npos && s.find(" *  2.37.1 (2026-11-19 22:35 CT") != std::string::npos, "setVersion + header comment 2.37.1"); }
    CHECK(std::string(hl::HL_VERSION) == "2.37.1" && std::string(hlset::PLUGIN_VERSION) == "2.37.1" && std::string(htop::VERSION) == "1.1.0", "HL_VERSION = PLUGIN_VERSION = 2.37.1; HodLodTop 1.1.0");
    {   const std::string b = rd("../../tools/gex-build.bat"); const size_t p = b.find("call :one HodLod ");
        const std::string ln = p == std::string::npos ? std::string() : b.substr(p, b.find('\n', p) - p);
        CHECK(ln.find("HodLodTop.h") != std::string::npos && ln.find("HodLodLogic.h") != std::string::npos, "gex-build HodLod line lists HodLodTop.h (edit triggers a rebuild)"); }
    std::printf("t_hl2371 (lsHodLod 2.37.1): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
