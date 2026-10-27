// g++ -std=c++11 -I. test_pivotvwap.cpp -o tpv && ./tpv
#include "PivotVwapLogic.h"
#include <cstdio>
int F = 0; void ck(bool b, const char* w) { std::printf("%s %s\n", b ? "PASS" : "FAIL", w); if (!b) ++F; }
int main() {
    // up 6 bars, then 3 falling closes below the swing-high bar's low -> pivot high at bar 5, then down
    std::vector<double> c = {100,101,102,103,104,105,104.5,103.5,102,101,100,99}, h, l, v(c.size(), 10);
    for (double x : c) { h.push_back(x + 0.5); l.push_back(x - 0.5); }
    l[5] = 104.6; h[5] = 106;
    pvw::Out o = pvw::compute(h, l, c, v);
    ck(o.anchor[0] == 0 && o.anchor[7] == 0, "anchored at the session open before the first pivot");
    int first = -1; for (size_t k = 0; k < c.size(); ++k) if (o.anchor[k] == 5) { first = (int)k; break; }
    ck(first == 8, "pivot high at bar 5 confirmed at bar 8 (3rd falling close below the pivot bar's low)");
    ck(o.state[2] == 1, "green while closes are above the line");
    ck(o.state[11] == -1, "red after the turn");
    // no repaint: computing on a prefix gives the same values for those bars
    std::vector<double> c2(c.begin(), c.begin() + 9), h2(h.begin(), h.begin() + 9), l2(l.begin(), l.begin() + 9), v2(v.begin(), v.begin() + 9);
    pvw::Out p = pvw::compute(h2, l2, c2, v2); bool same = true;
    for (int k = 0; k < 9; ++k) same = same && p.anchor[k] == o.anchor[k] && std::fabs(p.line[k] - o.line[k]) < 1e-9 && p.state[k] == o.state[k];
    ck(same, "no repaint: a closed bar's value / colour / anchor never change when later bars arrive");
    pvw::Out e = pvw::compute({}, {}, {}, {}); ck(e.line.empty(), "empty input");
    std::vector<double> z(5, 0.0), cz = {1,2,3,4,5}; pvw::Out q = pvw::compute(cz, cz, cz, z); ck(std::isnan(q.line[4]), "no volume -> no line");
    std::printf(F ? "FAILED %d\n" : "ALL PASS pivot VWAP\n", F); return F;
}
