// test_liqpick.cpp -- LiqPick 1.0.0 on the approved mockup's book (Databento ES Oct 29 2:50 PM, lp_proposal.html): top 5 + the two level-linked
// nodes >= 50% of the king, shown with their real ranks 6 / 7 and the level names; % of the king.
// From plugin\:  g++ -std=c++17 -O1 -Wall -Wextra -fsanitize=address,undefined -I. tests_liqpick/test_liqpick.cpp -o tlpk && ./tlpk
#include "LiqPick.h"
#include <cstdio>
static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace lpk;
int main() {
    CHECK(kindOf("PDH") == HIGH && kindOf("PFH") == HIGH && kindOf("ONH") == HIGH && kindOf("LonHI") == HIGH && kindOf("VAH") == HIGH && kindOf("HOD") == HIGH, "high levels");
    CHECK(kindOf("PDL") == LOW && kindOf("PFL") == LOW && kindOf("ONL") == LOW && kindOf("LonLO") == LOW && kindOf("VAL") == LOW && kindOf("PWL") == LOW, "low levels");
    CHECK(kindOf("WPoc") == POC && kindOf("DPoc") == POC && kindOf("dPOC") == POC, "POC levels");
    CHECK(kindOf("VWAP") == NONE && kindOf("Open") == NONE && kindOf("RTH Open") == NONE && kindOf("CR") == NONE && kindOf("HVL") == NONE, "not linking levels");
    std::vector<Node> n = { {7755.00, 265, false, 1}, {7752.00, 230, false, 2}, {7760.25, 209, false, 3}, {7721.00, 199, true, 4}, {7707.00, 174, true, 5},
                            {7746.75, 171, false, 6}, {7750.25, 164, false, 7}, {7748.00, 103, false, 8}, {7790.00, 150, false, 9} };
    std::vector<Level> L; auto add = [&](const char* nm, double px) { Level x; x.name = nm; x.px = px; x.kind = kindOf(nm); L.push_back(x); };
    add("PDH", 7794.25); add("PFH", 7819.5); add("ONH", 7773.75); add("LonHI", 7771.5); add("VAH", 7776.5);
    add("PDL", 7751.5); add("PFL", 7750.25); add("ONL", 7740.0); add("LonLO", 7745.0); add("VAL", 7761.5); add("DPoc", 7772.5); add("WPoc", 7690.0);
    const std::vector<Pick> p = pick(n, L, 3.61);
    for (const Pick& x : p) std::printf("  #%d %.2f %lld %d%% %s\n", n[x.idx].rank, n[x.idx].px, n[x.idx].lots, x.pct, x.names.c_str());
    CHECK(p.size() == 7, "top 5 + 2 level-linked");
    CHECK(p.size() >= 7 && n[p[5].idx].rank == 6 && p[5].extra && p[5].names == "PFL" && p[5].pct == 65, "rank 6: 7746.75 offer 65% under PFL (1 ATR below a low level)");
    CHECK(p.size() >= 7 && n[p[6].idx].rank == 7 && p[6].names == "PFL/PDL" && p[6].pct == 62, "rank 7: 7750.25 offer 62% PFL/PDL (nearest level first)");
    CHECK(p.size() >= 5 && p[0].pct == 100 && p[1].pct == 87 && p[2].pct == 79 && p[3].pct == 75 && p[4].pct == 66 && !p[0].extra, "% of the king");
    bool has8 = false, has9 = false; for (const Pick& x : p) { if (n[x.idx].rank == 8) has8 = true; if (n[x.idx].rank == 9) has9 = true; }
    CHECK(!has8, "39% of the king: not shown even near a level");
    CHECK(!has9, "57% but not within 1 ATR above a high / below a low (7790 is 4.25 under PDH)");
    {   std::vector<Node> m = n; m[8].px = 7796.0; const std::vector<Pick> q = pick(m, L, 3.61); bool s9 = false; for (const Pick& x : q) if (m[x.idx].rank == 9) s9 = x.names == "PDH";
        CHECK(s9, "1.75 above PDH (a high level): shown"); }
    {   std::vector<Node> m = n; m[8].px = 7692.0; const std::vector<Pick> q = pick(m, L, 3.61); bool s9 = false; for (const Pick& x : q) if (m[x.idx].rank == 9) s9 = x.names == "WPoc";
        CHECK(s9, "a POC counts on either side (2 above WPoc)"); }
    CHECK(pick(n, L, 0).size() == 5, "no ATR: no extras");
    CHECK(pick(std::vector<Node>(), L, 3.61).empty(), "no nodes");
    std::printf("LiqPick %s: %d passed, %d failed\n", VERSION, passes, fails);
    return fails ? 1 : 0;
}
