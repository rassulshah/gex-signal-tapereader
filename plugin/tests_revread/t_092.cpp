// t_092.cpp - RevReadLogic 0.9.2: layoutBS draws the buy / sell bars in a low pane.   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_092.cpp -o t92 && ./t92
#include "RevReadLogic.h"
#include <iostream>
using namespace rvs;
static int passed = 0, failed = 0;
#define OK(c) do { if (c) ++passed; else { ++failed; std::cout << "FAIL line " << __LINE__ << "\n"; } } while (0)
int main() {
    std::vector<std::pair<int, Poc> > c; std::map<long long, std::vector<Mark> > mm;
    for (int i = 0; i < 5; ++i) { Poc p; p.ok = true; p.t = 1000 + i * 180; p.buy = 100 * (i + 1); p.sell = 50 * (i + 1); p.bbv = i == 4 ? 30 : 0; c.push_back(std::make_pair(100 + i * 10, p)); }
    const std::vector<BSBar> T = layoutBS(c, mm, 0, 300, 12, 8, 16);                 // a tall pane: unchanged (title 16 + two text rows above the bars)
    OK(T.size() == 5 && T[4].bottom == 298 && T[4].topB == 44 && T[4].nB == 30 && T[4].numYB == 44 - 13);
    const std::vector<BSBar> L = layoutBS(c, mm, 500, 545, 12, 8, 16);               // his 45 px pane: 0.9.1 drew nothing
    OK(L.size() == 5);
    if (L.size() == 5) { OK(L[4].bottom == 543 && L[4].topB == 518 && L[0].topB < L[0].bottom && L[4].topS > L[4].topB);   // the tallest bar starts right under the title row
        OK(L[4].nB == 30 && L[4].numYB >= 516); for (const BSBar& b : L) OK(b.topB >= 518 && b.topS >= 518 && b.numYB >= 516 && b.numYS >= 516); }
    OK(layoutBS(c, mm, 500, 524, 12, 8, 16).empty());                                 // no room at all under the title: nothing
    OK(layoutBS(c, mm, 500, 592, 12, 8, 16).size() == 5 && layoutBS(c, mm, 500, 592, 12, 8, 16)[4].topB == 544);   // 20 px of bars or more: the full layout
    std::cout << "t_092: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
