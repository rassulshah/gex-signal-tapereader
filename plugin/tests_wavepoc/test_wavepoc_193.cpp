// test_wavepoc_193.cpp - lsWavePOC 1.9.3 (2026-11-06 17:38 CT Rassul): HVNs at / below the break level; left out ONLY when the POC is
// below the BO bar's low (down: above the BD bar's high).
#include "../WavePOCLogic.h"
#include <iostream>
#include <map>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
static wpc::Wave mk(int dir, const std::map<int, long long>& rows) {
    wpc::Wave w; w.dir = dir; w.e0 = 1000; w.rowTicks = 1; w.rows.assign(100, 10);
    for (auto& kv : rows) w.rows[(size_t)kv.first] = kv.second;
    w.med2 = wpc::median2(w.rows); w.maxVol = 0; for (long long v : w.rows) w.maxVol = std::max(w.maxVol, v);
    const std::vector<int> pk = wpc::pickNodes(w.rows, w.med2);
    for (size_t r = 0; r < pk.size(); ++r) { wpc::Node nd; nd.row = pk[r]; nd.rank = (int)r; nd.vol = w.rows[(size_t)pk[r]]; nd.tick = w.e0 + pk[r]; w.nodes.push_back(nd); }
    return w;
}
int main() {
    // up wave: POC row 60 (biggest), HVNs 40 and 20; BO bar = high 1070, low 1055
    wpc::Wave u = mk(1, {{60, 200}, {40, 120}, {20, 90}});
    std::vector<int> h = wpc::hvnRows(u, 1070, true, 1055);
    CHECK(h.size() == 2 && h[0] == 40 && h[1] == 20, "up: POC inside the BO zone (above the BO low) -> still 2 HVNs below the BO, POC excluded");
    h = wpc::hvnRows(u, 1070, true, 1065);
    CHECK(h.empty(), "up: POC (1060) below the BO bar low (1065) -> no HVN");
    h = wpc::hvnRows(u, 1070, true, 1060);
    CHECK(h.size() == 2, "up: POC exactly at the BO low -> not below it -> 2 HVNs");
    h = wpc::hvnRows(u, 1070, false, 0);
    CHECK(h.size() == 2, "no break bar: HVNs at / below the extreme");
    // down wave mirror: POC row 40, HVNs 60 and 80; BD bar = low 1030, high 1045
    wpc::Wave d = mk(-1, {{40, 200}, {60, 120}, {80, 90}});
    h = wpc::hvnRows(d, 1030, true, 1045);
    CHECK(h.size() == 2 && h[0] == 60 && h[1] == 80, "down: POC below the BD high -> 2 HVNs above the BD");
    h = wpc::hvnRows(d, 1030, true, 1035);
    CHECK(h.empty(), "down: POC (1040) above the BD bar high (1035) -> no HVN");
    // breakLevel returns the far side
    { wpc::Wave w = u; w.i0 = 0; w.i1 = 3; std::vector<wpc::Bar> B(5); for (int i = 0; i < 5; ++i) { B[i].t = 100 + i; B[i].h = 1050 + 5 * i; B[i].l = 1040 + 5 * i; }
      std::vector<wpc::Pivot> alt; wpc::Pivot p; p.i = -1; std::vector<wpc::BoCand> bos(1); bos[0].dir = 1; bos[0].t0 = 102; bos[0].hi = 1060; bos[0].lo = 1050;
      int why = 0; long long far = 0; const long long cut = wpc::breakLevel(w, B, alt, bos, why, &far);
      CHECK(why == 1 && cut == 1060 && far == 1050, "breakLevel: BO candle high + its low as the far side"); }
    CHECK(std::string(wpc::VERSION) == "1.9.3", "version 1.9.3");
    std::cout << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
