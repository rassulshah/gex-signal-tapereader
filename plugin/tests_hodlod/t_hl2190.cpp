// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2190.cpp -o t2190 && ./t2190   (from plugin\tests_hodlod)
// lsHodLod 2.19.0 (2026-11-06 Rassul 15:08-15:11 CT "do it", "keep it compact and near the gamma regime") - SDK-free:
//  [1] HodLodRange.h reads the nightly LRA-RangeDay-<MKT>.csv (all 7 markets) and its P(big) / P(small) equal lra.range_day.prob to
//      1e-9 on 105 real sessions (rd_parity.txt: pre-open and 30-min reads; a missing input takes the file's fill value)
//  [2] the pill text: "Big 61%" / "Small 56%" / "Norm", "?" before open + 30 min; no file / no inputs = no pill; garbled file not ok
//  [3] layout: in all 1260 scenarios of the 2.17.9 grid with a pill set, the pill is drawn inside the header band, right before the
//      gamma pill, no texts overlap, everything inside the box; with NO pill the box is identical to 2.18.0 (grid_2179.txt)
//  [4] versions 2.19.0
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <map>
#include "HodLodLogic.h"
#include "../HodLodSettings.h"
#include "../HodLodRange.h"
#include "lib_widths.h"
#include "scen.h"
#include "grid_2179.h"
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; if (fails < 40) std::printf("FAIL %s (line %d)\n", std::string(m).c_str(), __LINE__); } } while (0)
static double SC = 1.0;
static int MS(const std::string& s, int fs, bool b) { return (int)(measureLib(s, fs, b) * SC + 0.5); }
static std::string slurp(const std::string& p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int overlaps(const std::vector<hl::Item>& tx) { int n = 0; for (size_t i = 0; i < tx.size(); i++) for (size_t j = i + 1; j < tx.size(); j++) { const hl::Item& a = tx[i]; const hl::Item& b = tx[j];
    if (a.t < b.b - 1 && b.t < a.b - 1 && a.l < b.r - 1 && b.l < a.r - 1) ++n; } return n; }
int main() { (void)&itemsText;
    std::printf("[1] parity with lra.range_day\n");
    const char* MK7[7] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" }; std::map<std::string, hrd::File> F;
    for (const char* m : MK7) { F[m] = hrd::parse(slurp(std::string("LRA-RangeDay-") + m + ".csv"), m); CHECK(F[m].ok, std::string(m) + ": file parses (" + F[m].why + ")"); }
    { std::ifstream f("rd_parity.txt"); std::string ln; int n = 0, bad = 0; double worst = 0;
      while (std::getline(f, ln)) { std::istringstream is(ln); std::string m, t; double x[5], p[4];
        is >> m; for (int i = 0; i < 5; i++) { is >> t; x[i] = t == "nan" ? NAN : std::atof(t.c_str()); } for (int i = 0; i < 4; i++) is >> p[i];
        const hrd::File& G = F[m]; const double xp[5] = { x[0], x[1], x[2], x[3], NAN };
        const double q[4] = { hrd::prob(G.m[1][0], x), hrd::prob(G.m[1][1], x), hrd::prob(G.m[0][0], xp), hrd::prob(G.m[0][1], xp) };
        for (int i = 0; i < 4; i++) { const double d = std::fabs(q[i] - p[i]); worst = std::max(worst, d); if (d > 1e-6) { ++bad; std::printf("  off %s read %d: %s\n", m.c_str(), i, ln.c_str()); } } ++n; }
      CHECK(n == 105 && bad == 0, "105 real sessions x 4 reads equal lra.range_day (worst " + std::to_string(worst) + ", " + std::to_string(bad) + " off)"); }
    std::printf("[2] pill text\n");
    {   const hrd::File& G = F["GC"]; hrd::In I; I.ok = true; I.on = 2.0; I.pr = 1.5; I.gap = 0.5; I.news = 1;
        hrd::Out pre = hrd::read(G, I); CHECK(!pre.post && pre.s.find('?') != std::string::npos, "before open + 30 min: '?' read (" + pre.s + ")");
        I.x30 = 1.2; hrd::Out post = hrd::read(G, I); CHECK(post.post && post.kind == 1 && post.s.find("Big ") == 0 && post.s.find('?') == std::string::npos, "big 30-min range: '" + post.s + "'");
        I.on = 0.2; I.pr = 0.5; I.gap = 0; I.news = 0; I.x30 = 0.12; hrd::Out sm = hrd::read(G, I); CHECK(sm.kind == 2 && sm.s.find("Small ") == 0, "quiet day: '" + sm.s + "'");
        I.ok = false; CHECK(hrd::read(G, I).kind == -1, "no inputs (fewer than 14 prior sessions): no pill");
        CHECK(hrd::read(hrd::File(), I).kind == -1, "no file: no pill");
        CHECK(!hrd::parse("MARKET,GC\nMODEL,pre,big,x\n", "GC").ok && !hrd::parse(slurp("LRA-RangeDay-GC.csv"), "ES").ok, "garbled / other market's file not ok"); }
    std::printf("[3] layout with the pill\n");
    auto yOf = [](long long) { return 300; }; const double SCS[5] = { 1.0, 1.25, 1.3, 1.35, 1.4 };
    std::ifstream g("grid_2179.txt"); std::map<std::string, std::string> gm; { std::string ln; while (std::getline(g, ln)) if (ln.compare(0, 2, "S ") != 0) { std::istringstream is(ln); std::string k, t; for (int i = 0; i < 5 && is >> t; i++) k += t + " "; gm[k] = ln; } }
    int drawn = 0, bad = 0, same = 0, sameSw = 0, n = 0, noPillSame = 0, noPillN = 0;
    const char* PT[3] = { "Big 61%", "Small? 56%", "Norm" };
    for (int m = 0; m < 6; m++) for (int W : { 930, 950, 980, 1050, 1200, 1530, 1900 }) for (int sci = 0; sci < 5; sci++) for (int pos : { 6, 7, 1 }) for (int var = 0; var < 2; var++) {
        SC = SCS[sci]; const hl::Box pane = { 0, 0, W, 640 }; hl::Opts O; const std::string id = std::string(MKS[m].code) + " W" + std::to_string(W) + " s" + std::to_string(sci) + " p" + std::to_string(pos) + " v" + std::to_string(var);
        hl::View v0 = mkView(m, var == 0); hl::Layout L0 = hl::layoutAll(v0, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O);
        if (var == 1) { ++noPillN; const std::string key = std::to_string(m) + " " + std::to_string(W) + " " + std::to_string(sci) + " " + std::to_string(pos) + " 1 ";
            std::istringstream a(gm[key]), b(key + gridLine(L0)); std::vector<std::string> x, y; std::string t; while (a >> t) x.push_back(t); while (b >> t) y.push_back(t);
            if (x == y) ++noPillSame; }
        hl::View v = v0; const int k = (m + W + sci) % 3; v.rangePill = hl::Seg{ PT[k], k == 0 ? hl::C_GREEN : k == 1 ? hl::C_RED : hl::C_GREY };
        hl::Layout L = hl::layoutAll(v, pane, yOf, MS, (int)(W * 0.75), pos, -1, nullptr, &O); ++n;
        if (!L.tableOk) continue;
        std::vector<hl::Item> tx; const hl::Item* rp = nullptr; const hl::Item* gp = nullptr;
        for (const hl::Item& it : L.items) if (it.k == hl::K_TEXT && it.role == hl::R_TABLE && !it.s.empty()) { tx.push_back(it); if (it.s == PT[k]) rp = &tx.back(); if (it.s == "+GAMMA") gp = &tx.back(); }
        rp = nullptr; gp = nullptr; for (const hl::Item& it : tx) { if (it.s == PT[k]) rp = &it; if (it.s == "+GAMMA") gp = &it; }
        bool ok = rp && gp && rp->r < gp->l && gp->l - rp->r <= 30 && rp->t == gp->t && rp->t < L.table.t + 40;
        int out = 0; for (const hl::Item& it : tx) if (it.l < L.table.l || it.r > L.table.r || it.t < L.table.t || it.b > L.table.b) ++out;
        ok = ok && out == 0 && overlaps(tx) == 0; if (rp) ++drawn;
        if (!ok && ++bad < 5) std::printf("  bad %s (pill %d gamma %d out %d overlaps %d)\n", id.c_str(), rp != nullptr, gp != nullptr, out, overlaps(tx));
        if (L.table.l == L0.table.l && L.table.r == L0.table.r && L.table.t == L0.table.t && L.table.b == L0.table.b) ++same;
        if (L.swOk == L0.swOk) ++sameSw;
    }
    CHECK(noPillSame == noPillN, "no pill: every box identical to 2.17.9 / 2.18.0 (" + std::to_string(noPillSame) + " of " + std::to_string(noPillN) + ")");
    CHECK(bad == 0 && drawn > 1000, "pill drawn right before the gamma pill, inside, no overlaps (" + std::to_string(drawn) + " drawn, " + std::to_string(bad) + " bad)");
    CHECK(sameSw == n, "the pill never drops the sweep sections (" + std::to_string(sameSw) + " of " + std::to_string(n) + ")");
    std::printf("  %d scenarios, box unchanged in size in %d\n", n, same);
    std::printf("[4] versions\n");
    CHECK(std::string(hl::HL_VERSION) == "2.19.0" && std::string(hlset::PLUGIN_VERSION) == "2.19.0", "HL_VERSION = PLUGIN_VERSION = 2.19.0");
    { const std::string c = slurp("../HodLod.cpp"); CHECK(c.find("p->setVersion(\"2.19.0\")") != std::string::npos && c.find("loadRange(*S)") != std::string::npos, "HodLod.cpp setVersion 2.19.0 + loadRange"); }
    std::printf("t_hl2190 (lsHodLod 2.19.0): %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
