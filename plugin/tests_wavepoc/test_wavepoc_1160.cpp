// lsWavePOC 1.16.0: the spike rule on the real ES wave of 2026-11-17 10:53-11:26 (lsIRTReader 1-min footprint): both pullbacks turned at the
// 7347.25 spike (1,271 lots) - it must be an H; at most SPIKE_SIDE each side of the POC, nearest first; below the POC green, above red
#include "../WavePOCLogic.h"
#include <cstdio>
#include <fstream>
#include <iostream>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
int main(int argc, char** argv) {
    std::ifstream f(argc > 1 ? argv[1] : "es_wave_20261117.txt"); long long lo = 0; int n = 0; f >> lo >> n;
    wpc::Wave w; w.dir = 1; w.e0 = lo; w.rowTicks = 1; w.rows.assign((size_t)n, 0); for (int i = 0; i < n; ++i) f >> w.rows[(size_t)i];
    CHECK(n == 257, "fixture read (" << n << " rows)");
    w.med2 = wpc::median2(w.rows); int poc = 0; for (int i = 0; i < n; ++i) if (w.rows[(size_t)i] > w.rows[(size_t)poc]) poc = i;
    wpc::Node P; P.row = poc; P.tick = lo + poc; P.vol = w.rows[(size_t)poc]; w.nodes.push_back(P);
    auto px = [&](int r) { return (double)(lo + r) * 0.25; };
    CHECK(px(poc) == 7361.0 && w.rows[(size_t)poc] == 1637, "POC 7361, 1,637 lots");
    const std::vector<int> H = wpc::hvnRows(w, 0, true, 0);
    int below = 0, above = 0; bool has7347 = false, has7330 = false, nearOk = true; int prevDn = -1, prevUp = -1;
    for (int r : H) { std::printf("  H %.2f  %lld lots\n", px(r), w.rows[(size_t)r]);
        if (r < poc) { ++below; if (prevDn >= 0 && poc - r < poc - prevDn) nearOk = false; prevDn = r; } else { ++above; if (prevUp >= 0 && r - poc < prevUp - poc) nearOk = false; prevUp = r; }
        if (std::fabs(px(r) - 7347.25) <= 0.5) has7347 = true;
        if (std::fabs(px(r) - 7330.75) <= 0.5) has7330 = true;
        CHECK(w.rows[(size_t)r] * 2 >= (long long)std::llround(wpc::SPIKE_X * (double)w.med2), "every H is 2x the median row or more"); }
    CHECK(has7347, "the 7347.25 spike where both pullbacks turned is an H");
    CHECK(has7330, "the 7330.75 node is still an H");
    CHECK(below <= wpc::SPIKE_SIDE && above <= wpc::SPIKE_SIDE && below >= 3, "at most 4 each side (" << below << " below, " << above << " above)");
    CHECK(nearOk, "nearest the POC first");
    { wpc::Wave e; CHECK(wpc::hvnRows(e, 0, true, 0).empty(), "an empty wave: no H"); }
    CHECK(std::string(wpc::VERSION) == "1.16.0", "version");
    std::cout << "test_wavepoc_1160: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
