// LevelMarks 1.31.0 - the BI (Break In) on his example: ES 2026-11-27 (IRT bar store, 3-min, stamps = START -> +180 = END); levels as on his chart
#include "../LevelMarks.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <ctime>
static long long toT(const std::string& s) { std::tm t{}; std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec); t.tm_year -= 1900; t.tm_mon -= 1; return (long long)timegm(&t); }
static int fails = 0;
static void chk(bool c, const char* w) { std::printf("%s: %s\n", c ? "PASS" : "FAIL", w); if (!c) ++fails; }
int main() {
    std::ifstream f("es1127.csv"); std::string ln; std::vector<wyr::Bar> B; std::vector<std::string> W;
    while (std::getline(f, ln)) { std::stringstream ss(ln); std::string c[6]; for (int k = 0; k < 6; ++k) std::getline(ss, c[k], ',');
        wyr::Bar b; b.t = toT(c[0]) + 180; b.o = std::llround(std::stod(c[1]) * 4); b.h = std::llround(std::stod(c[2]) * 4); b.l = std::llround(std::stod(c[3]) * 4); b.c = std::llround(std::stod(c[4]) * 4);
        const long long v = std::llround(std::stod(c[5])); b.buy = v / 2; b.sell = v - v / 2; B.push_back(b); W.push_back(c[0].substr(11, 5)); }
    std::vector<wyr::Lvl> K; auto L = [&](const char* n, double p) { wyr::Lvl z; z.px = std::llround(p * 4); z.name = n; K.push_back(z); };
    L("PFH/PDH", 7256.25); L("VAH", 7256.25); L("DPoc", 7250.0); L("AsiaLO/ONL", 7238.5); L("PDL", 7232.5); L("VAL", 7228.0); L("PFL", 7187.0);
    std::vector<long long> T; std::vector<double> V, R; std::vector<char> G;
    for (const auto& b : B) { T.push_back(b.t); V.push_back((double)(b.buy + b.sell)); R.push_back((double)(b.h - b.l)); G.push_back(b.c >= b.o ? 1 : 0); }
    const auto C = lvm::biCands(B, K, lvm::rthOpenMin("ES"), 180, nullptr, nullptr);
    const auto P = lvm::biPick(C, T, V, R, G);
    auto at = [&](long long t) { size_t i = 0; while (i < B.size() && B[i].t != t) ++i; return i; };
    for (const auto& c : C) { std::printf("pivot %s dir %d level %s:", W[at(c.pivT)].c_str(), c.dir, c.lv.c_str()); for (long long t : c.c) std::printf(" %s", W[at(t)].c_str()); std::printf("\n"); }
    for (const auto& kv : P) std::printf("BI start %s dir %d %s %s pivot %s\n", W[at(kv.first)].c_str(), kv.second.dir, kv.second.lv.c_str(), kv.second.st.c_str(), W[at(kv.second.bkT)].c_str());
    bool ex = false; for (const auto& kv : P) if (W[at(kv.first)] == "09:06" && kv.second.dir == 1 && kv.second.st == "R99+" && W[at(kv.second.bkT)] == "09:03") ex = true;
    chk(ex, "his example: the green candle after the 9:06 AM LOD (store start 09:06) is BI, R99+, pivot 09:03");
    for (const auto& kv : P) if (W[at(kv.first)] == "09:06") chk(kv.second.lv == "ONL" || kv.second.lv == "AsiaLO" || kv.second.lv.find("LO") != std::string::npos || kv.second.lv.find("ONL") != std::string::npos, "labelled with the session low it took out");
    // the pivot candle itself never: 09:03 is red anyway; a lighter rule check - no BI on a red candle
    bool red = false; for (const auto& kv : P) { const auto& b = B[at(kv.first)]; if (kv.second.dir > 0 ? !(b.c > b.o) : !(b.c < b.o)) red = true; } chk(!red, "every BI has its own colour (green bullish / red bearish)");
    // strength gate: with BI_MIN, every pick has BVMax or BRMax 20+
    bool weak = false; for (const auto& kv : P) { const int i = (int)at(kv.first); if (lvm::strengthN(V, R, G, i, i) < lvm::BI_MIN) weak = true; } chk(!weak, "every BI has BVMax or BRMax 20+");
    // no repaint: the same marks on every prefix that contains the BI candle and the confirmed pivot
    bool stable = true; for (size_t n = 20; n <= B.size(); n += 7) { std::vector<wyr::Bar> b2(B.begin(), B.begin() + n); std::vector<long long> T2(T.begin(), T.begin() + n); std::vector<double> V2(V.begin(), V.begin() + n), R2(R.begin(), R.begin() + n); std::vector<char> G2(G.begin(), G.begin() + n);
        const auto P2 = lvm::biPick(lvm::biCands(b2, K, lvm::rthOpenMin("ES"), 180), T2, V2, R2, G2);
        for (const auto& kv : P2) { const auto it = P.find(kv.first); if (it == P.end() || it->second.lv != kv.second.lv || it->second.st != kv.second.st) { stable = false; std::printf("  prefix %zu: %s differs\n", n, W[at(kv.first)].c_str()); } } }
    chk(stable, "a BI seen on an earlier prefix is the same at the end (no repaint)");
    {   // (10:08 "the downpivot just swept a level") a level the leg broke but nothing closed back above by the BI candle: no BI from it
        std::vector<wyr::Lvl> K2; wyr::Lvl z; z.px = std::llround(7250.0 * 4); z.name = "PDL"; K2.push_back(z);
        const auto P2 = lvm::biPick(lvm::biCands(B, K2, lvm::rthOpenMin("ES"), 180), T, V, R, G);
        bool at906 = false; for (const auto& kv : P2) if (W[at(kv.first)] == "09:06") at906 = true;
        chk(!at906, "PDL 7250 (broken at 8:33, no close back above it by 9:09) gives no BI - the BI candle did not close back above it");
        std::vector<wyr::Lvl> K3; z.px = std::llround(7238.5 * 4); z.name = "ONL"; K3.push_back(z);
        const auto P3 = lvm::biPick(lvm::biCands(B, K3, lvm::rthOpenMin("ES"), 180), T, V, R, G);
        bool ok3 = false; for (const auto& kv : P3) if (W[at(kv.first)] == "09:06" && kv.second.lv == "ONL") ok3 = true;
        chk(ok3, "ONL 7238.50 (the BI closed back above it at 7245.50) gives the BI, labelled ONL"); }
    {   // the mirror: prices flipped (a high swept, a red BI closing back below it)
        std::vector<wyr::Bar> M = B; std::vector<char> G2 = G; for (size_t i = 0; i < M.size(); ++i) { M[i].o = 40000 - B[i].o; M[i].c = 40000 - B[i].c; M[i].h = 40000 - B[i].l; M[i].l = 40000 - B[i].h; G2[i] = M[i].c >= M[i].o ? 1 : 0; }
        std::vector<wyr::Lvl> K4; wyr::Lvl z; z.px = 40000 - std::llround(7238.5 * 4); z.name = "ONH"; K4.push_back(z);
        const auto P4 = lvm::biPick(lvm::biCands(M, K4, lvm::rthOpenMin("ES"), 180), T, V, R, G2);
        bool ok4 = false; for (const auto& kv : P4) if (W[at(kv.first)] == "09:06" && kv.second.lv == "ONH" && kv.second.dir == -1) ok4 = true;
        chk(ok4, "mirror: the red candle after the HOD that swept ONH and closed back under it is a bearish BI"); }
    {   // (1.31.1, 10:19 "the high has to be above the prior bar high") every BI's high is above the prior candle's high (bearish: low below)
        bool hh = true; for (const auto& kv : P) { const size_t i = at(kv.first); if (i == 0 || (kv.second.dir > 0 ? !(B[i].h > B[i - 1].h) : !(B[i].l < B[i - 1].l))) hh = false; }
        chk(hh, "every BI made a higher high than the prior candle (his example 7246.00 > 7235.50)");
        std::vector<wyr::Bar> B2 = B; const size_t i9 = at(P.begin()->first); B2[i9].h = B2[i9 - 1].h;   // same candle, high only equal to the prior high
        const auto P5 = lvm::biPick(lvm::biCands(B2, K, lvm::rthOpenMin("ES"), 180), T, V, R, G);
        chk(!P5.count(B[i9].t), "the same candle with its high only EQUAL to the prior high is no BI"); }
    {   // (1.31.2, 10:29 "it needs to be a level that wasnt run") a level the overnight already traded through (low 7238.50 < 7240) gives no BI,
        // even though RTH first broke it in the 8:39 leg and the BI closed back above it
        std::vector<wyr::Lvl> K6; wyr::Lvl z; z.px = std::llround(7240.0 * 4); z.name = "PDL"; K6.push_back(z);
        const auto P6 = lvm::biPick(lvm::biCands(B, K6, lvm::rthOpenMin("ES"), 180), T, V, R, G);
        bool at906 = false; for (const auto& kv : P6) if (W[at(kv.first)] == "09:06") at906 = true;
        chk(!at906, "PDL 7240 already run overnight (low 7238.50): no BI - its stops were gone"); }
    std::printf("%s (%d failed)\n", fails ? "FAILED" : "ALL PASS", fails); return fails ? 1 : 0;
}
