// lsDealerProfile 2.7.0 native key levels (KeyLevelsLogic.h) - checked against the lra.key_levels definitions
#include "KeyLevelsLogic.h"
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) passes++; else { fails++; printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
using namespace klv;
static std::vector<Bar> day(long sessDays, double base, double rthHi, double rthLo, double onHi, double onLo, int upToEndMin = 16 * 60)
{
    // 3-min bars for one ES session: 17:00 (previous day) .. upToEndMin of sessDays; ON range onLo..onHi, RTH range rthLo..rthHi
    std::vector<Bar> v;
    for (int e = 17 * 60 + 3; e <= 24 * 60; e += 3) v.push_back(Bar{sessDays - 1, e == 24 * 60 ? 0 : e * 60, (float)base, (float)base, (float)base});
    if (v.back().endSec == 0) v.back().days = sessDays;
    for (int e = 3; e <= upToEndMin; e += 3) {
        double h = base, l = base;
        if (e <= 510) { if (e == 300) h = onHi; if (e == 360) l = onLo; }
        else if (e <= 900) { if (e == 600) h = rthHi; if (e == 702) l = rthLo; }
        v.push_back(Bar{sessDays, e * 60, (float)h, (float)l, (float)base});
    }
    return v;
}
int main()
{
    long thu = svl::daysFromCivil(2026, 10, 8), wed = thu - 1;
    std::vector<Bar> B = day(wed, 100, 110, 90, 105, 95);
    std::vector<Bar> T = day(thu, 100, 120, 80, 104, 97, 10 * 60);
    B.insert(B.end(), T.begin(), T.end());
    auto L = compute(B, 180, 510, 900, 0.25);
    auto find = [&](const char* c) { for (auto& x : L) if (x.code == c) return x.price; return -1.0; };
    CHECK(find("PFH") == 110 && find("PFL") == 90, "prior full session high / low");
    CHECK(find("PDH") == 110 && find("PDL") == 90, "prior RTH high / low");
    CHECK(find("ONH") == 104 && find("ONL") == 97, "overnight high / low once RTH started");
    CHECK(find("LonHI") == 104 && find("LonLO") == 97, "London window 02:00-07:30 (the 06:00 low 97 is in it: LonLO = 97)" ) ;
    std::vector<Bar> N(B.begin(), B.end() - (10 * 60 - 7 * 60) / 3);   // cut at 07:00: before London ends, before RTH
    auto L2 = compute(N, 180, 510, 900, 0.25);
    bool on = false, lon = false; for (auto& x : L2) { if (x.code == "ONH") on = true; if (x.code == "LonHI") lon = true; }
    CHECK(!on && !lon, "no ONH before the RTH open, no LonHI before 07:30");
    CHECK(compute({}, 180, 510, 900, 0.25).empty(), "no bars: no levels");
    auto M = merge({{"PDH", 110, false}, {"PFH", 110, false}, {"HrHI", 110, true}, {"PDL", 90, false}}, 0.25);
    CHECK(M.size() == 2 && M[1].name == "PDH/PFH/HrHI" && !M[1].pivotOnly, "same tick merged, key names first, pivots last");
    // (2.7.4) a chart whose history starts part-way through a session: no level from the partial session / week
    {
        std::vector<Bar> P(B.begin() + 200, B.end());            // starts Wed ~03:00: Wed's session is only partly loaded
        auto L3 = compute(P, 180, 510, 900, 0.25);
        bool pf = false, pd = false; for (auto& x : L3) { if (x.code == "PFH" || x.code == "PFL") pf = true; if (x.code == "PDH") pd = true; }
        CHECK(!pf && !pd, "(2.7.4) prior session only partly loaded: no PFH / PFL / PDH (they would be wrong)");
        CHECK(find("PFH") == 110, "(2.7.4) loaded from the 17:00 open: PFH still drawn");
        // a full previous week (Mon 5 .. Fri 9 Oct sessions) and then Mon 12 Oct: PWH / PWL; cut its Monday: none
        std::vector<Bar> W;
        long mon = svl::daysFromCivil(2026, 10, 5);
        for (int d = 0; d < 5; d++) { auto v = day(mon + d, 100, 110 + d, 90 - d, 105, 95); W.insert(W.end(), v.begin(), v.end()); }
        auto v = day(mon + 7, 100, 101, 99, 105, 95, 10 * 60); W.insert(W.end(), v.begin(), v.end());
        auto LW = compute(W, 180, 510, 900, 0.25);
        double pwh = -1; for (auto& x : LW) if (x.code == "PWH") pwh = x.price;
        CHECK(pwh == 114, "(2.7.4) whole previous week loaded: PWH = the week's high");
        std::vector<Bar> W2(W.begin() + 100, W.end());           // the week's Monday session only partly loaded
        bool pw = false; for (auto& x : compute(W2, 180, 510, 900, 0.25)) if (x.code == "PWH") pw = true;
        CHECK(!pw, "(2.7.4) previous week only partly loaded: no PWH");
        std::vector<Bar> O(T.begin() + 150, T.end());            // today's session from ~00:30: ON partial, London (02:00-07:30) whole
        bool onh = false, lon = false; for (auto& x : compute(O, 180, 510, 900, 0.25)) { if (x.code == "ONH") onh = true; if (x.code == "LonHI") lon = true; }
        CHECK(!onh && lon, "(2.7.4) loaded from 00:30: no ONH (partial overnight), LonHI kept (London fully loaded)");
    }
    printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
