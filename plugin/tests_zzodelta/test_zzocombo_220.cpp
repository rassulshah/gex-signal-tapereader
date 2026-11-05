// test_zzocombo_220.cpp - lsZzoCombo 2.2.0 / lsZzoZigZag 1.2.0 (2026-11-04 21:01-21:14 CT, Rassul "zzo combo is still red while the zig zag turned
// up already .. they should be in sync", "do it"). Build (tests_zzodelta/): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_zzocombo_220.cpp -o t && ./t
//   1. synthetic: after a down wave's low, the first bar that lifts off it shows at once as a provisional UP wave (live), with no 3-bar
//      reversal yet; a new low folds it back into the down wave; the 3-bar reversal confirms it (completed waves unchanged)
//   2. real ES (zzc_es_1103.csv, bar by bar with the forming bar): at EVERY bar the Combo's last wave colour = the ZigZag's last leg
//      direction (combo and line in sync); every completed wave equals the 2.1.1 walk (no completed wave changed)
//   3. the 1-bar engine (ZZO Delta / Volume) is unchanged: one live wave
//   4. versions
#include "../ZzoComboView.h"
#include <climits>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

static zzd::Bar hb(long long t, long long h, long long l) { zzd::Bar b; b.t = t; b.h = h; b.l = l; b.vol = 100; b.vap = true; return b; }
static zzd::Engine run(const std::vector<std::pair<long long, long long>>& hl, int N) {
    zzd::Engine E; E.revBars = N; E.setMarket("ES", 180);
    long long t = 1793700000; for (auto& p : hl) { E.addBar(hb(t, p.first, p.second)); t += 180; }
    return E;
}
static std::vector<zzd::Bar> load(const char* csv) {
    std::vector<zzd::Bar> v; std::ifstream f(csv); std::string ln;
    while (std::getline(f, ln)) {
        if (ln.empty() || ln[0] == '#') continue;
        std::stringstream s(ln); std::string a; std::vector<std::string> x; while (std::getline(s, a, ',')) x.push_back(a);
        zzd::Bar b; b.t = std::stoll(x[0]); zzd::toTicks(std::stod(x[1]), 0.25, b.h); zzd::toTicks(std::stod(x[2]), 0.25, b.l);
        b.vol = std::stoll(x[3]); b.dlt = std::stoll(x[4]); b.vap = true; b.xh = std::stoll(x[5]); b.xl = std::stoll(x[6]); b.xok = x[7] == "1"; v.push_back(b);
    }
    return v;
}
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::string hhmm(long long t) { t -= 180; char b[16]; const int s = (int)(((t % 86400) + 86400) % 86400); int h = s / 3600, m = s / 60 % 60;
    std::snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return b; }   // bar START, 12-hour CT


int main() {
    (void)&slurp; (void)&hhmm;
    // ---- 1. synthetic down wave then a lift-off
    {   const std::vector<std::pair<long long, long long>> hl = { {20, 15}, {22, 17}, {24, 19}, {23, 16}, {21, 14}, {19, 12}, {17, 10}, {18, 11}, {19, 12} };
        zzd::Engine E = run(hl, 3);
        CHECK(E.Z.dirn == -1 && E.Z.ext == 6, "down wave live, extreme (low 10) at bar 6");
        const std::vector<zzd::Wave> v = E.view(nullptr);
        CHECK(v.size() >= 2 && v.back().live && v.back().dir == 1 && v.back().s == 7 && v.back().e == 8, "bars 7 - 8 (above the low, no 3-bar reversal yet) = a provisional UP wave");
        CHECK(v[v.size() - 2].live && v[v.size() - 2].dir == -1 && v[v.size() - 2].e == 6, "the down wave ends at its low (bar 6)");
        std::vector<std::pair<long long, long long>> h2 = hl; h2.push_back({ 16, 9 });   // a new low
        zzd::Engine F = run(h2, 3); const std::vector<zzd::Wave> w = F.view(nullptr);
        CHECK(!w.empty() && w.back().dir == -1 && w.back().e == 9 && F.Z.ext == 9, "a new low folds the provisional up wave back into the down wave");
        std::vector<std::pair<long long, long long>> h3 = hl; h3.push_back({ 20, 13 }); h3.push_back({ 22, 15 });
        zzd::Engine G = run(h3, 3);
        bool conf = false; for (const zzd::Wave& x : G.W) if (x.dir == -1 && x.e == 6) conf = true;
        CHECK(conf, "the 3-bar reversal confirms it: a completed down wave ending at bar 6"); }
    // ---- 2. real ES, bar by bar
    {   const std::vector<zzd::Bar> B = load("zzc_es_1103.csv"); CHECK(B.size() > 400, "ES bars loaded");
        zzd::Engine E; E.revBars = zzd::COMBO_REV_BARS; E.setMarket("ES", 180);
        int steps = 0, inSync = 0, prov = 0;
        for (size_t i = 0; i + 1 < B.size(); ++i) {
            E.addBar(B[i]); const zzd::Bar form = B[i + 1];
            const std::vector<zzd::Wave> v = E.view(&form); const std::vector<zzd::ZPt> z = zzd::zigzag(E, &form);
            if (v.empty() || z.size() < 2) { continue; }
            ++steps;
            const int legDir = z.back().px > z[z.size() - 2].px ? 1 : (z.back().px < z[z.size() - 2].px ? -1 : v.back().dir);
            if (legDir == v.back().dir) ++inSync;
            if (v.back().live && v.size() >= 2 && v[v.size() - 2].live) ++prov;
        }

        std::printf("  in sync %d / %d bars, provisional wave shown on %d\n", inSync, steps, prov);
        CHECK(inSync == steps && steps > 400, "combo and zigzag in sync on every ES bar");
        CHECK(prov > 50, "the provisional opposite wave shows up often (before the 3-bar reversal)");
        // completed waves = the 2.1.1 walk (same step): compare with an engine that never draws (W only)
        zzd::Engine R; R.revBars = zzd::COMBO_REV_BARS; R.setMarket("ES", 180); for (const zzd::Bar& b : B) R.addBar(b);
        zzd::Engine S; S.revBars = zzd::COMBO_REV_BARS; S.setMarket("ES", 180); for (const zzd::Bar& b : B) { S.addBar(b); (void)S.view(nullptr); }
        bool same = R.W.size() == S.W.size(); for (size_t k = 0; same && k < R.W.size(); ++k) same = R.W[k].s == S.W[k].s && R.W[k].e == S.W[k].e && R.W[k].dir == S.W[k].dir && R.W[k].vol == S.W[k].vol;
        CHECK(same && R.W.size() > 50, "completed waves are untouched by the live split");
        const std::string ref = slurp("ref_zzc3_es_1103.txt"); CHECK(!ref.empty(), "2.1.1 reference present (test_zzocombo_210 checks the completed waves against it)"); }
    // ---- 3. 1-bar engine unchanged
    {   const std::vector<std::pair<long long, long long>> hl = { {20, 15}, {22, 17}, {24, 19}, {23, 16}, {21, 14}, {19, 12}, {17, 10}, {18, 11} };
        zzd::Engine E = run(hl, 1); const std::vector<zzd::Wave> v = E.view(nullptr);
        int live = 0; for (const zzd::Wave& w : v) live += w.live; CHECK(live == 1, "1-bar engine (ZZO Delta / Volume): one live wave, no split"); }
    // ---- 4. versions
    {   const std::string c = slurp("../ZzoCombo.cpp"), z = slurp("../ZzoZigZag.cpp");
        CHECK(std::string(zzd::COMBO_VERSION) == "2.2.0" && c.find("p->setVersion(\"2.2.0\")") != std::string::npos, "Combo 2.2.0");
        CHECK(std::string(zzd::ZIGZAG_VERSION) == "1.2.0" && z.find("p->setVersion(\"1.2.0\")") != std::string::npos, "ZigZag 1.2.0"); }
    std::printf("%d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
