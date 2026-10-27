// g++ -std=c++17 -Wall -Wextra -Werror -I. test_lineleveltag.cpp -o tllt && ./tllt
#include "LineLevelTag.h"
#include <cstdio>
int F = 0; void ck(bool b, const char* w) { std::printf("%s %s\n", b ? "PASS" : "FAIL", w); if (!b) ++F; }
int main() {
    using namespace llt;
    ck(kindOf("PDH") == KEY && kindOf("LonLO") == KEY && kindOf("AsiaH") == KEY && kindOf("Open") == KEY, "key levels");
    ck(kindOf("DPoc") == VOLUME && kindOf("WPoc") == VOLUME && kindOf("VAH") == VOLUME && kindOf("VAL") == VOLUME, "volume levels");
    ck(kindOf("HVL") == GAMMA && kindOf("CR") == GAMMA && kindOf("PS0") == GAMMA && kindOf("G1") == GAMMA && kindOf("GW") == GAMMA, "gamma levels");
    std::vector<Level> L = { {"PDH", 7739.50}, {"VAH", 7736.50}, {"HVL", 7720.00}, {"DPoc", 7714.50} };
    ck(tagFor(7739.50, L, 0.25, 4.0).text() == "PDH?", "an order on PDH -> PDH?");
    ck(tagFor(7740.50, L, 0.25, 4.0).text() == "PDH?", "close enough: 4 ticks away is inside the zone (0.25 x ATR 4 = 1.00)");
    ck(tagFor(7740.75, L, 0.25, 4.0).text().empty(), "5 ticks away: outside the zone, no tag");
    ck(tagFor(7720.25, L, 0.25, 4.0).text() == "HVL?" && tagFor(7720.25, L, 0.25, 4.0).kind == GAMMA, "gamma tag");
    std::vector<Level> T = { {"PDL", 7714.50}, {"DPoc", 7714.50} };
    ck(tagFor(7714.50, T, 0.25, 4.0).text() == "DPoc?", "same price: volume beats key");
    ck(tagFor(7715.00, T, 0.25, 0.0).text() == "DPoc?" && tagFor(7715.25, T, 0.25, 0.0).text().empty(), "no ATR: the 2-tick floor still works");
    ck(tagFor(7700.0, {}, 0.25, 4.0).text().empty() && tagFor(7700.0, L, 0.0, 4.0).text().empty(), "no levels / bad tick: no tag");
    std::printf(F ? "FAILED %d\n" : "ALL PASS line level tag\n", F); return F;
}
