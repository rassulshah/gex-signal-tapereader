// test_pivotvwap_110.cpp -- PivotVwapLogic.h 1.1.0 (lsSessionVWAP 1.8.7, 2026-10-29): anchors = IRT's ZigZag (3 bars, high/low), the same
// pivots as lsWavePOC's zigzag; no repaint. g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_pivotvwap_110.cpp
#include "PivotVwapLogic.h"
#include <cstdio>
#include <string>
#include <algorithm>
int F = 0; void ck(bool b, const char* w) { std::printf("%s %s\n", b ? "PASS" : "FAIL", w); if (!b) ++F; }
int main() {
    // up legs and down legs with equal highs (an equal high does not move the pivot)
    std::vector<double> h = {10,11,12,13,13,12.5,12,11,10.5,11,12,13,14,15,14,13,12,11,12,13}, l, c, v(h.size(), 10);
    for (double x : h) { l.push_back(x - 1.5); c.push_back(x - 0.7); }
    std::vector<int> piv; pvw::anchors(h, l, c, piv);
    // by hand: bar 3's high beats bars 0-2 -> the first up leg (pivot low = bar 0); bar 4 is EQUAL (no move); bar 6's low 10.5 is below
    // the lows of bars 3-5 (11) -> reversal, pivot HIGH = bar 3 (the strict extreme, not the equal bar 4)
    ck(piv[2] == -1 && piv[3] == 0 && piv[5] == 0, "first leg: pivot low at the session's first bar once bar 3 beats the previous 3 highs");
    ck(piv[6] == 3, "bar 6 breaks the previous 3 bars' lows: the up leg's STRICT extreme (bar 3, not the equal bar 4) is the pivot");
    ck(piv[10] == 8 && piv[15] == 13 && piv[19] == 17, "later pivots: low 8 (at bar 10), high 13 (at bar 15), low 17 (at bar 19)");
    int lowPiv = -1; for (size_t k = 8; k < h.size(); ++k) if (piv[k] != 3 && piv[k] >= 0) { lowPiv = piv[k]; break; }
    ck(lowPiv == 8, "the down leg's low (bar 8) becomes the next pivot when a bar beats the previous 3 highs");
    // same pivots as lsWavePOC's zigzag on these bars
    {   std::vector<int> ref; int zDir = 0, zI = 0, zHi = 0, zLo = 0, last = -1; const int N = 3;
        for (int k = 0; k < (int)h.size(); ++k) { if (k >= N) { double ph = -1e9, pl = 1e9; for (int j = k - N; j < k; ++j) { ph = std::max(ph, h[j]); pl = std::min(pl, l[j]); }
            if (zDir == 0) { if (h[k] > ph) { last = zLo; zDir = 1; zI = k; } else if (l[k] < pl) { last = zHi; zDir = -1; zI = k; } }
            else if (zDir > 0) { if (h[k] > h[zI]) zI = k; else if (l[k] < pl) { last = zI; zDir = -1; zI = k; } }
            else { if (l[k] < l[zI]) zI = k; else if (h[k] > ph) { last = zI; zDir = 1; zI = k; } } }
            if (zDir == 0) { if (h[k] > h[zHi]) zHi = k; if (l[k] < l[zLo]) zLo = k; } ref.push_back(last); }
        ck(ref == piv, "identical to the WavePOCLogic 1.3.2 zigzag rule (transcribed)"); }
    pvw::Out o = pvw::compute(h, l, c, v); bool same = true;
    for (size_t n = 1; n <= h.size(); ++n) { std::vector<double> a(h.begin(), h.begin() + n), b(l.begin(), l.begin() + n), cc(c.begin(), c.begin() + n), vv(v.begin(), v.begin() + n);
        pvw::Out p = pvw::compute(a, b, cc, vv); for (size_t k = 0; k < n; ++k) same = same && p.anchor[k] == o.anchor[k] && p.state[k] == o.state[k]; }
    ck(same, "no repaint: every prefix gives the same anchors / colours");
    ck(std::string(pvw::VERSION) == "1.1.0", "version 1.1.0");
    std::printf(F ? "FAILED %d\n" : "ALL PASS pivot VWAP 1.1.0\n", F); return F;
}
