// fxlv FIXTURE.bin MKT LEVELS.lv SHIFT_S [floorOverride] : the engine at the CHART floor (or an override) over a fixture slice, with
// levels (t_ms UTC shifted by SHIFT_S to the fixture's CT-as-UTC clock); one row per zone: side per touch state held9 low_floor near
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include "ZoneDefenseLogic.h"
#pragma pack(push, 1)
struct FR { int t; signed char a, s; int px; unsigned sz; int oid; };
#pragma pack(pop)
int main(int argc, char** argv) {
    std::vector<FR> v; { FILE* f = std::fopen(argv[1], "rb"); FR r; while (f && std::fread(&r, sizeof r, 1, f) == 1) v.push_back(r); if (f) std::fclose(f); }
    zd::Engine E(zdp::forMarket(argv[2])); E.barSec = 180; if (argc > 5) E.minLotsOverride = std::atoll(argv[5]);
    if (argc > 6) E.M.levelFloor = std::atoll(argv[6]); if (argc > 7) E.M.levelFloorX = std::atof(argv[7]);
    struct LV { long long t; std::string n; long long px; }; std::vector<LV> L; const long long sh = std::atoll(argv[4]) * 1000;
    { FILE* f = std::fopen(argv[3], "r"); char ln[256]; while (f && std::fgets(ln, sizeof ln, f)) { char nm[64]; long long t, px; if (std::sscanf(ln, "%lld,%63[^,],%lld", &t, nm, &px) == 3) L.push_back({ t + sh, nm, px }); } if (f) std::fclose(f); }
    size_t li = 0; std::vector<zd::Lvl> cur;
    for (auto& r : v) {
        const long long t = (long long)r.t * 1000; bool ch = false;
        while (li < L.size() && L[li].t <= t) { for (auto& x : zd::levelsOf(L[li].n, L[li].px)) { bool f = false; for (auto& c : cur) if (c.name == x.name) { c.px = x.px; f = true; } if (!f) cur.push_back(x); } ++li; ch = true; }
        if (ch) E.setLevels(cur);
        zd::In e; e.t = t; e.act = r.a; e.side = r.s; e.px = r.px; e.sz = r.sz; e.oid = r.oid; E.feed(e);
    }
    E.finish(E.lastT + 1, 4);
    for (auto& z : E.zones) std::printf("%d %d %lld %d %d %d %d %d %.3f %lld\n", z.side, z.per, z.touchT, z.state, z.sHeld9b, z.lowFloor ? 1 : 0, z.nearKind, z.levelSearch ? 1 : 0, z.nearGapAtr, z.T);
}
