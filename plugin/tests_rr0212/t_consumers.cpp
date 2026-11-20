// t_consumers.cpp -- lsRevRead 0.21.2: the I lines (shown 6-field + flagged 7-field "Sw") added to the wy file change NOTHING for the other readers:
// lsHodLod's Now line (NowLevels nowl::parseWy) and signal line (TopSignals tsig::parseWy), lsVolRead / lsRangeRead (VolRangeLogic vrr::parseWy),
// the Sw voice (SweepVoice svx::parseSw). They read Bk / Sw / FT / Tst / Rev by code; an I never enters their paths.
// build (in plugin\tests_rr0212): g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. t_consumers.cpp -o tc && ./tc
#include "NowLevels.h"
#include "TopSignals.h"
#include "VolRangeLogic.h"
#include "SweepVoice.h"
#include "../WyMarks.h"
#include <cstdio>
static int nf = 0, nok = 0; static void C(bool c, const char* m) { if (c) ++nok; else { ++nf; std::printf("FAIL %s\n", m); } }
int main() {
    const long long E = 2082844800LL, t0 = 1795107780LL + E;   // 11-19 17:03 local, 1904 epoch
    std::string base = "WY|1|ES|180|0.21.2\n";
    base += std::to_string(t0) + "|A|1\n" + std::to_string(t0 + 180) + "|Sw|1|PDL|29221|V40\n" + std::to_string(t0 + 360) + "|Bk|-1|ONL|29100|V12\n"
          + std::to_string(t0 + 540) + "|FT|-1\n" + std::to_string(t0 + 900) + "|Tst|-1|ONL|0|62%\n" + std::to_string(t0 + 1080) + "|Rev|-1||0|V15\nL|29221|PDL\n";
    const std::string withI = base + wyi::iLine(t0 + 180, 1, "PDL", 29221, "V40", true) + "\n" + wyi::iLine(t0 + 720, 1, "PHL", 29150, "R22", false) + "\n"
                            + wyi::iLine(t0 + 1260, -1, "ONH", 29400, "", false) + "\n" + std::to_string(t0 + 1440) + "|I|1\n";
    const long long from = t0 - E - 10, to = t0 - E + 100000;
    {   const auto a = nowl::parseWy(base, 0.25, from, to), b = nowl::parseWy(withI, 0.25, from, to); bool same = a.size() == b.size();
        for (size_t i = 0; same && i < a.size(); ++i) same = a[i].t == b[i].t && a[i].code == b[i].code && a[i].dir == b[i].dir && a[i].lv == b[i].lv && a[i].st == b[i].st;
        C(same && !a.empty(), "HodLod Now line (nowl::parseWy): same marks with the I lines"); for (const auto& m : b) C(m.code != "I", "Now line: no I"); }
    {   const auto a = tsig::parseWy(base, "ES", from, to), b = tsig::parseWy(withI, "ES", from, to); bool same = a.size() == b.size();
        for (size_t i = 0; same && i < a.size(); ++i) same = a[i].t == b[i].t && a[i].type == b[i].type && a[i].dir == b[i].dir && a[i].lv == b[i].lv;
        C(same && !a.empty(), "HodLod signal line (tsig::parseWy): same signals with the I lines"); for (const auto& s : b) C(s.type == "Sw" || s.type == "BkTst", "signal line: Sw / BkTst only"); }
    {   std::map<long long, vrr::Mk> a, b; const int na = vrr::parseWy(base, a), nb = vrr::parseWy(withI, b); bool same = na == nb && a.size() == b.size();
        for (const auto& kv : a) { const auto it = b.find(kv.first); same = same && it != b.end() && it->second.code == kv.second.code && it->second.dir == kv.second.dir; }
        C(same && na == 2, "VolRead / RangeRead (vrr::parseWy): Bk / Sw only, same with the I lines"); }
    {   const auto a = svx::parseSw(base, from, to), b = svx::parseSw(withI, from, to); bool same = a.size() == b.size();
        for (size_t i = 0; same && i < a.size(); ++i) same = a[i].t == b[i].t && a[i].dir == b[i].dir && a[i].lv == b[i].lv;
        C(same && a.size() == 1, "Sw voice (svx::parseSw): same with the I lines"); }
    {   std::map<long long, std::vector<wyi::M> > a, b; wyi::parseAll(base, a); wyi::parseAll(withI, b); int iN = 0; for (const auto& kv : b) for (const auto& m : kv.second) if (m.code == 'I') ++iN;
        C(iN == 3, "lsRevReadMarks / lsBreakout / lsWavePOC (wyi::parseAll): the 2 shown I's + the old 3-field I; the flagged one skipped");
        std::map<long long, int> bk; for (const auto& kv : b) for (const auto& m : kv.second) if (m.code == 'B') bk[kv.first] = m.dir; C(bk.size() == 1, "lsBreakout / lsWavePOC read only Bk: unchanged"); }
    std::printf("%s %d pass, %d fail\n", nf ? "FAILED" : "ALL PASS", nok, nf); return nf;
}
