// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2180.cpp -o t2180 && ./t2180   (from plugin\tests_hodlod)
// lsHodLod 2.18.0 (2026-11-04 16:59 CT, Rassul "in the hodlod box, swap the LOD Sweeps with HOD sweeps") - SDK-free:
//  [1] the box reads SESSION | LOD SWEEPS | HOD SWEEPS: the LOD strip title, its rows (names, prices, %) in the left sweep section, the HOD
//      ones in the right; strip colours unchanged (LOD green, HOD red)
//  [2] nothing else changes: in all 1890 scenarios of the 2.17.9 grid (6 markets x 7 pane widths x 5 font scales x 3 positions x rows /
//      no rows, + the stretched-to-axis pass) the box rectangle, the sweep form and every text (string, size, bold, colour) are identical
//      to 2.17.9 (grid_2179.txt, written by gen_2179.cpp from the 2.17.9 header); no texts overlap, every text inside the box
//  [3] versions 2.18.0 everywhere
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <map>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "lib_widths.h"
#include "scen.h"
#include "grid_2179.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 40) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
static std::vector<hl::Item> texts(const hl::Layout& L) { std::vector<hl::Item> t; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && !it.s.empty() && it.role == hl::R_TABLE) t.push_back(it); return t; }
static std::string compactPx(const std::string& px) { std::string o; for (size_t k = 0; k < px.size(); k++) { if (px.compare(k, 3, " - ") == 0) { o += '-'; k += 2; } else o += px[k]; } return o; }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (a.t < b.b - 1 && b.t < a.b - 1 && a.l < b.r - 1 && b.l < a.r - 1) ++n; } return n; }
static void order(const hl::Layout& L, const hl::View& v, const std::string& id) {
    if (!L.swOk) return;
    CHECK(L.swCols.size() == 2 && L.swCols[0].r == L.swCols[1].l && L.swCols[1].r == L.table.r, id + ": two sections butt up, the right one ends at the box edge");
    const std::vector<hl::Item> tx = texts(L); const hl::Item* tLod = nullptr; const hl::Item* tHod = nullptr;
    for (const hl::Item& it : tx) { if (it.s.find("LOD SWEEPS") == 0 || it.s.find("LOD sweeps") == 0) tLod = &it; if (it.s.find("HOD SWEEPS") == 0 || it.s.find("HOD sweeps") == 0) tHod = &it; }
    CHECK(tLod && tHod && tLod->l >= L.swCols[0].l && tLod->r <= L.swCols[0].r && tHod->l >= L.swCols[1].l && tHod->r <= L.swCols[1].r, id + ": LOD SWEEPS title left, HOD SWEEPS right");
    int miss = 0;
    for (int q = 0; q < 2; q++) { const hl::Box& c = L.swCols[(size_t)(1 - q)];   // q 0 = HOD -> right section, 1 = LOD -> left
        for (const hl::View::SwRow& r : (q ? v.swLo : v.swHi)) { const std::string px = L.swTier >= 3 ? compactPx(r.px) : r.px;
            for (const std::string& s : { r.name, px, r.pct }) { bool f = false; for (const hl::Item& it : tx) if (it.s == s && it.l >= c.l && it.r <= c.r) f = true; if (!f) ++miss; } } }
    CHECK(miss == 0, id + ": every LOD row in the left section, every HOD row in the right (" + std::to_string(miss) + " missing)");
    int gL = 0, rL = 0; for (const hl::Item& it : L.items) if (it.role == hl::R_TABLE && it.k == hl::K_FILL) { if (it.c1 == hl::C_GREEN && it.l >= L.swCols[0].l && it.r <= L.swCols[0].r) ++gL; if (it.c1 == hl::C_RED && it.l >= L.swCols[1].l && it.r <= L.swCols[1].r) ++rL; }
    CHECK(gL > 0 && rL > 0, id + ": LOD strip green on the left, HOD strip red on the right");
    int out = 0; for (const hl::Item& it : tx) if (it.l < L.table.l || it.r > L.table.r || it.t < L.table.t || it.b > L.table.b) ++out;
    CHECK(out == 0 && overlaps(tx) == 0, id + ": every text inside the box, none overlap");
}
// the box is the same size (the version text "v2.18.0" is as long as "v2.17.9"); every field (box, form, the text hash with the version
// the text hash with the version normalised and "PhrH"/"PhrL" read back as "PHH"/"PHL") must match 2.17.9
static bool sameBut(const std::string& g, const std::string& n) {
    std::istringstream a(g), b(n); std::vector<std::string> x, y; std::string t; while (a >> t) x.push_back(t); while (b >> t) y.push_back(t);
    if (x.size() != y.size() || x.size() < 8) { return false; }
    const size_t r = x.size() - 3;   // ... l t r b hash
    for (size_t i = 0; i < x.size(); i++) if (i != r && x[i] != y[i]) return false;
    const long d = std::stol(y[r]) - std::stol(x[r]); return d == 0; }
// (FAV) with sweep rows: exactly one FAV per section (scen.h: the top row of each side has the highest % and a price), drawn after its
// row's name, inside its section, in the FAV colour; if the sections are drawn
static bool favOk(const hl::Layout& L, const hl::View& v) {
    if (!L.swOk) return L.favN == 0;
    if (L.favN != 2) { std::printf("  favN %d\n", L.favN); return false; }
    if (L.swTier >= 3) { int y = 0, t = 0; for (const hl::Item& it : L.items) { if (it.k != hl::K_TEXT) continue; if (it.s == "FAV") t++; if ((it.s == v.swHi[0].name || it.s == v.swLo[0].name) && it.c1 == hl::C_FAV) y++; }
        return t == 0 && y == 2; }   // tightest form: the FAV names in yellow, no tag
    int n = 0; for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && it.s == "FAV") { n++;
        if (it.c1 != hl::C_FAV) return false;
        bool after = false; for (const hl::Item& nm : L.items) if (nm.k == hl::K_TEXT && (nm.s == v.swHi[0].name || nm.s == v.swLo[0].name) && nm.t == it.t && it.l > nm.r) after = true;
        if (!after) { std::printf("  FAV not after its name\n"); return false; } }
    return n == 2; }
int main() { (void)&itemsText;
    auto yOf = [](long long) { return 300; }; const double SCS[5] = { 1.0, 1.25, 1.3, 1.35, 1.4 };
    std::ifstream g("grid_2179.txt"); std::vector<std::string> gold; { std::string ln; while (std::getline(g, ln)) gold.push_back(ln); }
    CHECK(gold.size() == 1890, "grid_2179.txt: 1890 lines (got " + std::to_string(gold.size()) + ")");
    std::printf("[1] order + [2] unchanged vs 2.17.9\n");
    std::map<std::string, std::string> gm, gs; { std::string last; for (const std::string& ln : gold) { if (ln.compare(0, 2, "S ") == 0) gs[last] = ln; else { std::istringstream is(ln); std::string k, t; for (int i = 0; i < 5 && is >> t; i++) k += t + " "; gm[k] = ln; last = k; } } }
    int same = 0, diff = 0, swN = 0, favBoxes = 0;
    for (int m = 0; m < 6; m++) for (int W : { 930, 950, 980, 1050, 1200, 1530, 1900 }) for (int sci = 0; sci < 5; sci++) for (int pos : { 6, 7, 1 }) for (int var = 0; var < 2; var++) {
        SC = SCS[sci]; const hl::Box pane = { 0, 0, W, 640 }; hl::View v = mkView(m, var == 0); hl::Opts O;
        const std::string key = std::to_string(m) + " " + std::to_string(W) + " " + std::to_string(sci) + " " + std::to_string(pos) + " " + std::to_string(var) + " ";
        const std::string id = std::string(MKS[m].code) + " W" + std::to_string(W) + " s" + std::to_string(sci) + " p" + std::to_string(pos) + " v" + std::to_string(var);
        hl::Layout L = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O);
        bool ok = var == 1 ? (gm.count(key) && sameBut(gm[key], key + gridLine(L))) : favOk(L, v);
        if (var == 0 && L.swOk) favBoxes++;
        order(L, v, id); if (L.swOk) ++swN;
        if (hl::stretchToAxis(L, v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, O, W - 2)) {
            ok = ok && (var == 1 ? (gs.count(key) && sameBut(gs[key], "S " + gridLine(L))) : favOk(L, v)); order(L, v, id + " stretched"); }
        else if (var == 1 && gs.count(key)) ok = false;
        if (ok) ++same; else if (++diff < 4) std::printf("  diff %s\n", id.c_str());
    }
    CHECK(diff == 0, "no-row boxes identical to 2.17.9, every sweep box carries its 2 FAV tags (" + std::to_string(diff) + " differ)");
    CHECK(swN == 630, "the sweep sections are drawn in every scenario 2.17.9 drew them (630, got " + std::to_string(swN) + ")");
    CHECK(favBoxes > 500, "FAV checked on " + std::to_string(favBoxes) + " sweep boxes");
    std::printf("  scenarios passing: %d of 1260; layouts with sweep sections checked: %d; FAV boxes %d\n", same, swN, favBoxes);
    std::printf("[3] versions\n");
    CHECK(std::string(hl::HL_VERSION) == std::string(hlset::PLUGIN_VERSION), "HL_VERSION = PLUGIN_VERSION");  // (watchdog 2026-11-07) version-agnostic: was pinned to one release and failed on every bump
    { std::ifstream f("../HodLod.cpp"); std::stringstream ss; ss << f.rdbuf(); CHECK(ss.str().find(std::string("p->setVersion(\"") + hl::HL_VERSION + "\")") != std::string::npos, "HodLod.cpp setVersion == HL_VERSION"); }
    std::printf("t_hl2180 (lsHodLod 2.18.0): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
