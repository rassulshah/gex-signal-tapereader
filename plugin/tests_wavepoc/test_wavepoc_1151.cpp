// lsWavePOC 1.15.1: a CoB marked on the bar AFTER the up-leg's end pivot (as lsCobSignal marks it) gets its CoB leg profile
#include "../WavePOCLogic.h"
#include <iostream>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static long long run(int cobOffset, bool ok) {
    wpc::Engine E; E.N = 3; std::vector<int> flag(200, 0);
    // down 0..9, up 10..19 (end pivot = bar 19, the high), down 20..32
    std::vector<wpc::Bar> bars; long long px = 1000;
    for (int i = 0; i < 33; ++i) { const int d = i < 10 ? -1 : i < 20 ? 1 : -1; px += d * 4; wpc::Bar b; b.t = 1000000 + i * 180; b.o = px - d * 2; b.c = px; b.h = std::max(b.o, b.c) + 1; b.l = std::min(b.o, b.c) - 1; bars.push_back(b); }
    const int cb = 19 + cobOffset; flag[(size_t)cb] = 1;
    wpc::CobCand q; q.t = bars[(size_t)cb].t; q.dir = 1; q.ok = ok; q.tOk = bars[(size_t)cb].t; E.cobs.push_back(q);
    E.cobFn = [&flag](int k) { return k >= 0 && k < (int)flag.size() ? flag[(size_t)k] : 0; };
    wpc::VapFn vap = [&bars](int bar, std::vector<wpc::VRow>& out) { out.clear(); for (long long t = bars[(size_t)bar].l; t <= bars[(size_t)bar].h; ++t) { wpc::VRow r; r.tick = t; r.vol = 10 + (t % 7); out.push_back(r); } return 1; };
    for (const wpc::Bar& b : bars) E.addBar(b, vap);
    return E.C.cobOnly + E.C.cobBoth;
}
int main() {
    CHECK(run(0, true) == 1, "a CoB on the end-pivot bar: the leg is profiled (as before)");
    CHECK(run(1, true) == 1, "a CoB on the bar after the end pivot (lsCobSignal's mark): the leg is profiled - was 0");
    CHECK(run(3, true) == 1, "3 bars after the pivot: still this leg");
    CHECK(run(5, true) == 0, "5 bars after: not this leg");
    CHECK(run(1, false) == 0, "a CoB not drawn (ZZO under 10): no leg");
    CHECK(wpc::GREY_FADE > 0.3, "lighter rows (1.15.2)");
    std::cout << "test_wavepoc_1151: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
