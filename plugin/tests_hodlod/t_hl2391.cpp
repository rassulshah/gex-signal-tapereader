// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2391.cpp -o t2391 && ./t2391 [draw-dump.txt]   (from plugin\tests_hodlod)
// lsHodLod 2.39.1 (Rassul 2026-11-20 09:32 CT, ES screenshot of 2.39.0: "the gamma badge should be more aligned to the left so you will have
// more space for the options line and fix the arrows because they are overlapping")
//  [1] the real strings of his screenshot (gamma: "GAMMA WATCH: 7,150 v . 7,200 ^ . Break | G ^ V - C -", Now: "... Next v 2hH . ^ 3hH | Last Bk ^ ALO"),
//      Liberation Sans (Arial metrics) widths at 9 pt and 10 pt: every arrow's ink box (strokes + the 2-px pen) is >= 3 px clear of every
//      glyph box on both sides, sits vertically centred on the text row, stays inside its badge fill; black on badges, coloured on dark
//  [2] the regime pill + "0DTE Exp 3:00 PM" at the FAR LEFT (the box's left edge); the line centred and wider than 2.39.0 allowed
//  [3] narrow panes 100..1600 px and every centre: the pill box never intersects the centred line (>= 10 px), the line moves right only as
//      much as needed, drop order 0DTE, setups, Next, G V C, regime word, pill; the badge stays
//  [4] host: Now / gamma text drawn in its measured box (textIn, centred by IRT), versions 2.39.1, NowLevels 1.3.1, GammaLine 1.1.0
#include <cstdio>
#include <fstream>
#include <sstream>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../GammaLine.h"
#include "../HodLodTop.h"
#include "lib_widths.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static std::string rd(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }

struct Box { int l, t, r, b; };
// arrow ink boxes: each arrow = 3 consecutive arw strokes; the 2-px pen adds 1 px each side
template <class It> static std::vector<Box> arrows(const std::vector<It>& items, std::vector<unsigned>* cols = nullptr) {
    std::vector<Box> v;
    for (size_t i = 0; i + 2 < items.size(); i++) if (items[i].k == 2 && items[i].arw) {
        int l = 1 << 30, r = -(1 << 30), t = 1 << 30, b = -(1 << 30);
        for (size_t j = i; j < i + 3; j++) { l = std::min(l, std::min(items[j].l, items[j].r)); r = std::max(r, std::max(items[j].l, items[j].r)); t = std::min(t, std::min(items[j].t, items[j].b)); b = std::max(b, std::max(items[j].t, items[j].b)); }
        v.push_back(Box{ l - 1, t - 1, r + 1, b + 1 }); if (cols) cols->push_back(items[i].col); i += 2; }
    return v;
}
template <class It> static std::vector<Box> glyphs(const std::vector<It>& items) { std::vector<Box> v; for (const It& it : items) if (it.k == 1) v.push_back(Box{ it.l, it.t, it.r, it.b }); return v; }
static int hgap(const Box& a, const Box& g) { return a.l >= g.r ? a.l - g.r : g.l >= a.r ? g.l - a.r : -1; }   // horizontal clearance, -1 = overlap

static FILE* dump = nullptr;
template <class It> static void dumpItems(const char* tag, int fs, const std::vector<It>& items) {
    if (!dump) return;
    std::fprintf(dump, "ROW %s %d\n", tag, fs);
    for (const It& it : items) { std::fprintf(dump, "%d %d %d %d %d %06X %06X %d %d|%s\n", it.k, it.l, it.t, it.r, it.b, it.col & 0xFFFFFF, it.fill & 0xFFFFFF, it.bold ? 1 : 0, (int)it.arw, it.s.c_str()); } }

// the gamma line of the screenshot: WATCH 7,150 below / 7,200 above (no names), Break, G up, V / C dashes, Next 7,100 / 7,250
static gml::Out gammaOut() {
    gml::Out o; o.ok = true; o.state = gml::WATCH;
    o.dn.ok = true; o.dn.px = 7150; o.dn.label = "7,150"; o.dn.expect = -1;
    o.up.ok = true; o.up.px = 7200; o.up.label = "7,200"; o.up.expect = -1;
    o.nDn.ok = true; o.nDn.px = 7100; o.nDn.label = "PS 7,100"; o.nUp.ok = true; o.nUp.px = 7250; o.nUp.label = "7,250";
    o.g.dir = 1; o.g.ratio = NAN; return o; }
// the Now line of the screenshot: WATCH badge with an arrow, Next v 2hH . ^ 3hH, Last Bk ^ ALO
static nowl::Out nowOut() {
    nowl::Out o; o.ok = true; o.tst.ok = true; o.tst.state = 1; o.tst.dir = 1; o.tst.lv = "PDH";
    o.dn.ok = true; o.dn.name = "2hH"; o.up.ok = true; o.up.name = "3hH";
    o.haveBk = true; o.bk.lv = "ALO"; o.bk.dir = 1; return o; }

template <class It> static void checkArrows(const char* tag, int fs, const std::vector<It>& items, int ym) {
    std::vector<unsigned> cols; const std::vector<Box> A = arrows(items, &cols), G = glyphs(items);
    int minGap = 1 << 30; bool centred = true, inFill = true;
    for (size_t i = 0; i < A.size(); i++) {
        for (const Box& g : G) minGap = std::min(minGap, hgap(A[i], g));
        if (std::abs((A[i].t + A[i].b) / 2 - ym) > 1) centred = false;
        // an arrow inside a filled badge stays inside the fill (black on gold); one outside is coloured on the dark line
        for (const It& it : items) if (it.k == 0 && it.fill == it.col && it.fill != 0 && A[i].l >= it.l && A[i].r <= it.r + 0) { if (A[i].l - it.l < 0 || it.r - A[i].r < 0) inFill = false; }
    }
    char m[160]; std::snprintf(m, sizeof m, "%s %d pt: %d arrows, every arrow >= 3 px clear of every glyph (min %d px)", tag, fs, (int)A.size(), minGap);
    CHECK(!A.empty() && minGap >= 3, m);
    std::snprintf(m, sizeof m, "%s %d pt: arrows vertically centred on the text row, inside their badge", tag, fs); CHECK(centred && inFill, m);
}

int main(int argc, char** argv) {
    if (argc > 1) dump = std::fopen(argv[1], "w");
    const gml::Out go = gammaOut(); std::vector<gml::Seg> bd = gml::badgeSegs(go); gml::addWatchWord(bd, go, 7176);
    const std::vector<gml::Seg> gv = gml::gvcSegs(go), nx = gml::nextSegs(go);
    const std::vector<nowl::Seg> ns = nowl::segments(nowOut());
    const std::vector<std::string> forms = htop::gammaTag("-GAMMA");
    std::printf("[1] real strings, arrows clear of glyphs (9 / 10 pt)\n");
    CHECK(gml::plain(bd) == "[GAMMA WATCH: 7,150 \xE2\x86\x93 \xC2\xB7 7,200 \xE2\x86\x91 \xC2\xB7 Break]" && gml::plain(gv) == "G\xE2\x86\x91 V\xE2\x80\x93 C\xE2\x80\x93", "gamma strings as the screenshot: " + gml::plain(bd) + " | " + gml::plain(gv));
    CHECK(nowl::plain(ns).find("Next \xE2\x86\x93 2hH \xC2\xB7 \xE2\x86\x91 3hH") != std::string::npos && nowl::plain(ns).find("Last Bk \xE2\x86\x91 ALO") != std::string::npos, "Now strings as the screenshot: " + nowl::plain(ns));
    for (int fs : { 9, 10 }) {
        auto meas = [fs](const std::string& s, bool bold) { return measureLib(s, fs, bold); };
        const gml::Draw G = gml::layout(forms, gml::C_RED, 0x450A0A, bd, gv, nx, "0DTE Exp 3:00 PM", 0xE5E7EB, meas, 700, 500, 20, 1380, 40, fs);
        CHECK(G.ok && G.level == 0, "gamma line drawn in full");
        checkArrows("gamma", fs, G.items, (G.t + G.b) / 2);
        const nowl::Draw N = nowl::layout(ns, meas, 700, gml::nowBottom(G, 496), 20, 1380, fs);
        CHECK(N.ok && N.shown == N.pieces, "Now line drawn in full");
        checkArrows("now", fs, N.items, (N.t + N.b) / 2);
        // colours: black arrows on the gold badges, coloured on the dark line (G green, Next gold, Last Bk green)
        std::vector<unsigned> gc, nc; arrows(G.items, &gc); arrows(N.items, &nc);
        CHECK(gc.size() == 5 && gc[0] == gml::C_BLACK && gc[1] == gml::C_BLACK && gc[2] == gml::C_GREEN && gc[3] == gml::C_GOLD && gc[4] == gml::C_GOLD, "gamma arrows: 2 black (badge), G green, Next gold");
        CHECK(nc.size() == 4 && nc[0] == nowl::C_BLACK && nc[1] == nowl::C_GOLD && nc[2] == nowl::C_GOLD && nc[3] == nowl::C_GREEN, "Now arrows: black (badge), Next gold x2, Last Bk green");
        // every text item carries its measured box (the host centres the text in it)
        bool boxes = true; for (const gml::Item& it : G.items) if (it.k == 1 && it.r - it.l != measureLib(it.s, fs, it.bold)) boxes = false;
        for (const nowl::Item& it : N.items) if (it.k == 1 && it.r - it.l != measureLib(it.s, fs, it.bold)) boxes = false;
        CHECK(boxes, "text items = their measured boxes");
        // no two glyph boxes overlap either
        bool clear = true; for (const auto* V : { &G.items }) { const std::vector<Box> g = glyphs(*V); for (size_t i = 0; i < g.size(); i++) for (size_t j = i + 1; j < g.size(); j++) if (hgap(g[i], g[j]) < 0) clear = false; }
        { const std::vector<Box> g = glyphs(N.items); for (size_t i = 0; i < g.size(); i++) for (size_t j = i + 1; j < g.size(); j++) if (hgap(g[i], g[j]) < 0) clear = false; }
        CHECK(clear, "no glyph boxes overlap");
        CHECK(N.b <= G.t - 3, "Now line above the gamma line (3 px)");
        dumpItems("gamma", fs, G.items); dumpItems("now", fs, N.items);
        std::printf("    %d pt  gamma: %s\n", fs, G.text.c_str()); }
    std::printf("[2] pill + 0DTE at the far left\n");
    {   auto meas = [](const std::string& s, bool bold) { return measureLib(s, 10, bold); };
        const int xL = 40, cx = 700;
        const gml::Draw G = gml::layout(forms, gml::C_RED, 0x450A0A, bd, gv, nx, "0DTE Exp 3:00 PM", 0xE5E7EB, meas, cx, 500, 20, 1380, xL, 10);
        CHECK(G.pill && G.pillL == xL && G.exp && G.expL == G.pillR + gml::PILL_EXP && G.leftR == G.expR, "pill at the box's left edge, then 0DTE Exp");
        bool pillBox = false; for (const gml::Item& it : G.items) if (it.k == 0 && it.l == xL && it.r == G.pillR && it.col == gml::C_RED && it.fill == 0x450A0A) pillBox = true;
        CHECK(pillBox, "pill: red border on the dark red fill");
        CHECK(std::abs((G.l + G.r) / 2 - cx) <= 1, "line centred on the box");
        CHECK(G.text.find("(-GAMMA") == 0 && G.text.find("\xE2\x94\x82 (-GAMMA") == std::string::npos && G.text.find("| [GAMMA WATCH") != std::string::npos, "the line starts with the badge (no pill in it): " + G.text);
        // more room: on the 2.39.0 box the pill was inside the line; now a line of the same pane width keeps Next + G V C where it used to drop them
        int minFull = 0; for (int w = 400; w <= 1600; w += 2) { const gml::Draw D = gml::layout(forms, gml::C_RED, 0x450A0A, bd, gv, nx, "", 0, meas, 20 + w / 2, 500, 20, 20 + w, 20, 10); if (D.ok && D.level <= 1) { minFull = w; break; } }
        std::printf("    narrowest pane with the full line + full pill: %d px\n", minFull); CHECK(minFull > 0, "full line fits somewhere"); }
    std::printf("[3] narrow panes: the pill never touches the line\n");
    {   auto meas = [](const std::string& s, bool bold) { return measureLib(s, 10, bold); };
        bool never = true, inside = true, mono = true, badge = true, minimal = true; std::string seq; int prev = -1;
        for (int w = 1600; w >= 100; w -= 4) {
            for (int off = -w / 2; off <= w / 2; off += std::max(4, w / 16)) {   // the box centre anywhere in the pane
                const int xMin = 10, xMax = xMin + w, cx = (xMin + xMax) / 2 + off;
                const gml::Draw D = gml::layout(forms, gml::C_RED, 0x450A0A, bd, gv, nx, "0DTE Exp 3:00 PM", 0xE5E7EB, meas, cx, 500, xMin, xMax, xMin, 10);
                if (!D.ok) continue;
                if ((D.pill || D.exp) && D.leftR + gml::EXP_GAP > D.l) never = false;
                if (D.l < xMin || D.r > xMax || ((D.pill || D.exp) && D.leftR > xMax)) inside = false;
                if (D.text.find("GAMMA WATCH") == std::string::npos) badge = false;
                // shifted only as much as needed: either centred (or clamped to the pane) or exactly EXP_GAP right of the left group
                const int tw = D.r - D.l; int c0 = cx - tw / 2; if (c0 + tw > xMax) c0 = xMax - tw; if (c0 < xMin) c0 = xMin;
                if (D.l != c0 && D.l != D.leftR + gml::EXP_GAP) minimal = false;
                if (off == 0) { if (D.level < prev) mono = false; prev = D.level; if (seq.empty() || seq.back() != (char)('0' + D.level)) seq += (char)('0' + D.level); }
            }
        }
        CHECK(never, "pill / 0DTE always >= 10 px left of the centred line (every width, every centre)");
        CHECK(inside && badge && minimal && mono, "inside the pane, badge always there, the line shifted only as far as needed");
        std::string sq; for (char c : seq) if (sq.find(c) == std::string::npos) sq += c;
        CHECK(sq == "0123456", "drop order 0DTE, setups, Next, G V C, regime word, pill: " + sq);
        // a box centred near the left edge: the line moves right just clear of the pill
        const gml::Draw D = gml::layout(forms, gml::C_RED, 0x450A0A, bd, gv, nx, "", 0, meas, 300, 500, 10, 1400, 10, 10);
        CHECK(D.ok && D.pill && D.l == D.leftR + gml::EXP_GAP && D.level == 1, "centre too far left: line shifted right to pill + 10 px, nothing dropped"); }
    std::printf("[4] host + versions\n");
    {   const std::string h = rd("../HodLod.cpp"), lg = rd("../HodLodLogic.h"), st = rd("../HodLodSettings.h");
        CHECK(std::string(hl::HL_VERSION) == "2.39.1" && std::string(hlset::PLUGIN_VERSION) == "2.39.1" && h.find("p->setVersion(\"2.39.1\")") != std::string::npos && lg.find("was HL_VERSION = \"2.39.0\"") != std::string::npos, "versions 2.39.1 everywhere (2.39.0 kept in the history)");
        CHECK(h.find(" *  2.39.1 (2026-11-20 09:32 CT") != std::string::npos, "header comment 2.39.1");
        CHECK(std::string(nowl::VERSION) == "1.3.1" && std::string(gml::VERSION) == "1.1.0", "NowLevels 1.3.1, GammaLine 1.1.0");
        CHECK(h.find("void textIn(int l, int r, int yc") != std::string::npos && h.find("rc.drawText(s.c_str(), true, false)") != std::string::npos, "textIn: IRT centres the text in its measured box");
        size_t n = 0, p = 0; while ((p = h.find("textIn(it.l, it.r, (it.t + it.b) / 2, it.s", p)) != std::string::npos) { n++; p++; }
        CHECK(n == 2, "gamma + Now line text both drawn with textIn");
        CHECK(h.find("G.exp ? G.expL : G.l") == std::string::npos && h.find("G.pill ? G.pillL : G.expL") != std::string::npos, "the confirm strip keep-out includes the pill at the far left"); }
    if (dump) std::fclose(dump);
    std::printf("t_hl2391: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
