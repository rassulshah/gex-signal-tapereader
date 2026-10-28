#include "../LabelPlace.h"
#include <cstdio>
#include <cmath>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } } while (0)
int main() {
    // ES 10-28 08:30-08:33 real prints (price -> y with 10.6 px / point, 7775 at y 190), row height 13 px
    struct P { double px; long long lots; } p[] = { {7775.0,330},{7773.0,741},{7773.0,1571},{7772.75,801},{7772.75,831},{7766.25,755},{7766.25,791},{7766.0,908},{7766.0,790},{7765.75,713} };
    std::vector<lpl::Cand> c; for (auto& q : p) c.push_back({ (int)std::lround(190 + (7775.0 - q.px) * 10.6), q.lots });
    std::vector<size_t> k = lpl::place(c, 13);
    bool has1571 = false; for (size_t i : k) if (c[i].weight == 1571) has1571 = true;
    CHECK(has1571, "the biggest print (1571 @ 7773.00) is drawn");
    for (size_t i : k) for (size_t j : k) if (i < j) CHECK(std::abs(c[i].y - c[j].y) >= 13 || false, "kept labels never overlap");
    // every kept label is drawn at its own y (place() never returns a moved row) - checked by construction: caller uses c[i].y
    bool drop801 = true; for (size_t i : k) if (c[i].weight == 801 || c[i].weight == 831) drop801 = false;
    CHECK(drop801, "7772.75 prints (2.7 px from 7773.00) are not drawn on top of / below the bigger 7773.00 print");
    bool has330 = false; for (size_t i : k) if (c[i].weight == 330) has330 = true; CHECK(has330, "7775.00 print is far enough to be drawn");
    // a busy band (another mark) blocks without moving the label
    std::vector<std::pair<int,int> > busy{{200, 220}}; std::vector<lpl::Cand> one{{210, 999}};
    CHECK(lpl::place(one, 13, busy).empty(), "a label whose own row is busy is not drawn (never shifted)");
    CHECK(lpl::place(one, 13).size() == 1, "a free row is drawn");
    std::printf("labelplace: %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
