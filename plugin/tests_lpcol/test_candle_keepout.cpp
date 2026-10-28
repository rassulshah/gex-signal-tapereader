// test_candle_keepout.cpp - lsHodLod 2.9.3: the visible candles as keep-outs (grouped ~40 px) + hl::placeAvoid on real ES bars (10-27 / 10-28 to 06:39)
// = the table + strip dock where they cover no candle.   From plugin\: g++ -std=c++17 -I. tests_lpcol/test_candle_keepout.cpp && ./a.out
#include "HodLodLogic.h"
#include <cstdio>
#include <fstream>
#include <vector>
int main() {
    std::ifstream f("tests_lpcol/es_hl.csv"); std::vector<double> H, L; double h, l; char c;
    while (f >> h >> c >> l) { H.push_back(h); L.push_back(l); }
    const int W = 1200, top = 40, bot = 600, ppb = 9; const double pmax = 7781, pmin = 7740;   // a 1200 px price area, 9 px per bar, the last 120 bars
    auto y = [&](double p) { return (int)(bot - (p - pmin) / (pmax - pmin) * (bot - top)); };
    std::vector<hl::Box> av; int sl = 0, sr = -1, st = 0, sb = 0;
    for (int i = (int)H.size() - 1; i >= 0; --i) {   // newest first, as in HodLod::draw
        const int xl = 20 + i * ppb, xr = xl + ppb - 2, yt = y(H[i]) - 4, yb = y(L[i]) + 4;
        if (sr < 0) { sl = xl; sr = xr; st = yt; sb = yb; continue; }
        if (sr - xl > 40) { av.push_back(hl::Box{ sl, st, sr, sb }); sl = xl; sr = xr; st = yt; sb = yb; }
        else { sl = std::min(sl, xl); st = std::min(st, yt); sb = std::max(sb, yb); }
    }
    if (sr >= 0) av.push_back(hl::Box{ sl, st, sr, sb });
    const hl::Box area = { 20, top, W, bot }; bool moved = false, clear = true;
    const hl::Box b = hl::placeAvoid(area, 480, 190, 7 /* bottom-left */, av, &moved, &clear);
    int covered = 0; for (int i = 0; i < (int)H.size(); ++i) { const int xl = 20 + i * ppb, xr = xl + ppb - 2; if (xr > b.l && xl < b.r && y(L[i]) > b.t && y(H[i]) < b.b) ++covered; }
    std::printf("keep-out boxes %zu, table at %d,%d-%d,%d, clear %d, candles covered %d\n", av.size(), b.l, b.t, b.r, b.b, clear ? 1 : 0, covered);
    const bool pass = clear && covered == 0 && av.size() >= 10 && av.size() <= 40;
    std::printf("test_candle_keepout: %s\n", pass ? "1 passed, 0 failed" : "0 passed, 1 failed"); return pass ? 0 : 1;
}
