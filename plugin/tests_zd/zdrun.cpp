// zdrun  --  runs the lsZoneDefense engine (ZoneDefenseLogic.h, the plugin's own code) over one session of raw MBO events.
// stdin: packed records <q ts_ns><b act><b side><q px_ticks><i size><Q oid> (zd_export.py); args: MARKET ctOffsetMin [barSec]
// stdout: one JSON object per zone (all decided fields + truth) then a "lines" object with the decision journal count.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <climits>
#include "ZoneDefenseLogic.h"
#pragma pack(push, 1)
struct R { long long ts; signed char act, side; long long px; int sz; unsigned long long oid; };
#pragma pack(pop)
int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: zdrun MKT ctOffsetMin [barSec] [linesFile|-] [minLotsFloor]\n"); return 2; }
    zd::Engine E(zdp::forMarket(argv[1])); E.ctOffsetMin = std::atoi(argv[2]); if (argc > 3) E.barSec = std::atoi(argv[3]);
    if (argc > 5) E.minLotsOverride = std::atoll(argv[5]);
    static R buf[65536]; size_t n; long long last = 0;
    while ((n = std::fread(buf, sizeof(R), 65536, stdin)) > 0) {
        for (size_t i = 0; i < n; ++i) { zd::In e; e.t = buf[i].ts / 1000000; e.act = buf[i].act; e.side = buf[i].side; e.px = buf[i].px; e.sz = buf[i].sz; e.oid = (long long)buf[i].oid; E.feed(e); last = e.t; }
    }
    E.finish(last + 1);
    for (size_t i = 0; i < E.zones.size(); ++i) {
        const zd::Zone& z = E.zones[i];
        std::printf("{\"id\":%d,\"side\":%d,\"per\":%d,\"T\":%lld,\"lo\":%lld,\"hi\":%lld,\"size\":%lld,\"dens\":%.3f,\"oldest\":%.2f,\"touch\":%lld,\"at_touch\":%lld,"
                    "\"cancel60\":%.4f,\"cnorm\":%.4f,\"n60\":%d,\"pulling\":%d,\"sold\":%lld,\"refills\":%lld,\"left\":%lld,\"reloadX\":%.4f,\"reload\":%d,\"broke_early\":%d,"
                    "\"dec\":%lld,\"sweep\":%d,\"run_lots\":%lld,\"lift\":%.4f,\"back_in\":%d,\"rebid\":%lld,\"end\":%d,\"endT\":%lld,\"held9\":%d,\"held30\":%d,\"s_held9\":%d,\"big\":%lld}\n",
                    z.id, z.side, z.per, z.T, z.lo, z.hi, z.size, z.dens, z.oldestMin, z.touchT, z.atTouch, z.cancel60, z.cnorm, z.n60, z.pulling ? 1 : 0, z.sold, z.refills, z.left, z.reloadX, z.reload,
                    z.brokeEarly ? 1 : 0, z.decT, z.sweep, z.runLots, z.lift, z.backIn ? 1 : 0, z.rebid, z.endCode, z.endT, z.held9, z.held30, z.sHeld9, z.bigSz);
    }
    if (argc > 4 && std::strcmp(argv[4], "-") != 0) { FILE* f = std::fopen(argv[4], "w"); if (f) { for (size_t i = 0; i < E.out.size(); ++i) std::fprintf(f, "%s\n", E.out[i].text.c_str()); std::fclose(f); } }
    std::fprintf(stderr, "%s events %lld fills %lld minutes %lld zones %zu lines %zu\n", argv[1], E.events, E.fills, E.minutes, E.zones.size(), E.out.size());
    return 0;
}
