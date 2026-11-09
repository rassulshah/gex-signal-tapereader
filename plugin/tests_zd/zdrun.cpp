// zdrun  --  runs the lsZoneDefense engine (ZoneDefenseLogic.h, the plugin's own code) over one session of raw MBO events.
// stdin: packed records <q ts_ns><b act><b side><q px_ticks><i size><Q oid> (zd_export.py); args: MARKET ctOffsetMin [barSec]
// (1.0.2 levels) argv[6] = a levels file, lines "t_ms,NAME,px_ticks" sorted by t (a level known from t on; px 0 = gone);
//   argv[7] = the level reach in ATR (default the market's Mkt.levelReach).
// stdout: one JSON object per zone (all decided fields + truth) then a "lines" object with the decision journal count.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <climits>
#include <map>
#include <vector>
#include "ZoneDefenseLogic.h"
#pragma pack(push, 1)
struct R { long long ts; signed char act, side; long long px; int sz; unsigned long long oid; };
#pragma pack(pop)
int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: zdrun MKT ctOffsetMin [barSec] [linesFile|-] [minLotsFloor] [levelsFile|-] [reachAtr]\n"); return 2; }
    zd::Engine E(zdp::forMarket(argv[1])); E.ctOffsetMin = std::atoi(argv[2]); if (argc > 3) E.barSec = std::atoi(argv[3]);
    if (argc > 5) E.minLotsOverride = std::atoll(argv[5]);
    zdp::Mkt M2 = zdp::forMarket(argv[1]); if (argc > 7) { M2.levelReach = std::atof(argv[7]); E.M = M2; }
    struct LE { long long t; std::string name; long long px; }; std::vector<LE> lev; size_t li = 0; std::map<std::string, long long> now;
    if (argc > 6 && std::strcmp(argv[6], "-") != 0) { FILE* f = std::fopen(argv[6], "r"); char ln[256];
        while (f && std::fgets(ln, sizeof ln, f)) { char nm[128]; long long t, px; if (std::sscanf(ln, "%lld,%127[^,],%lld", &t, nm, &px) == 3) lev.push_back(LE{ t, nm, px }); }
        if (f) std::fclose(f); }
    auto levelsTo = [&](long long t) {
        bool ch = false; while (li < lev.size() && lev[li].t <= t) { if (lev[li].px > 0) now[lev[li].name] = lev[li].px; else now.erase(lev[li].name); ++li; ch = true; }
        if (!ch) return; std::vector<zd::Lvl> v; for (std::map<std::string, long long>::const_iterator it = now.begin(); it != now.end(); ++it) { std::vector<zd::Lvl> a = zd::levelsOf(it->first, it->second); v.insert(v.end(), a.begin(), a.end()); }
        E.setLevels(v); };
    static R buf[65536]; size_t n; long long last = 0;
    while ((n = std::fread(buf, sizeof(R), 65536, stdin)) > 0) {
        for (size_t i = 0; i < n; ++i) { zd::In e; e.t = buf[i].ts / 1000000000 * 1000; /* (1.0.2) the live clock: whole seconds */ e.act = buf[i].act; e.side = buf[i].side; e.px = buf[i].px; e.sz = buf[i].sz; e.oid = (long long)buf[i].oid; levelsTo(e.t); E.feed(e); last = e.t; }
    }
    E.finish(last + 1);
    for (size_t i = 0; i < E.zones.size(); ++i) {
        const zd::Zone& z = E.zones[i];
        std::printf("{\"id\":%d,\"side\":%d,\"per\":%d,\"T\":%lld,\"lo\":%lld,\"hi\":%lld,\"size\":%lld,\"dens\":%.3f,\"oldest\":%.2f,\"touch\":%lld,\"at_touch\":%lld,"
                    "\"cancel60\":%.4f,\"cnorm\":%.4f,\"n60\":%d,\"pulling\":%d,\"sold\":%lld,\"refills\":%lld,\"left\":%lld,\"reloadX\":%.4f,\"reload\":%d,\"broke_early\":%d,"
                    "\"dec\":%lld,\"sweep\":%d,\"run_lots\":%lld,\"lift\":%.4f,\"back_in\":%d,\"rebid\":%lld,\"end\":%d,\"endT\":%lld,\"held9\":%d,\"held30\":%d,\"s_held9\":%d,\"big\":%lld,\"state\":%d,\"state_t\":%lld,\"state_held9\":%d,\"reclaim_bars\":%d,\"lvl_kind\":%d,\"lvl_name\":\"%s\",\"lvl_gap_atr\":%.3f,\"lvl_search\":%d,\"tags\":\"%s\"}\n",
                    z.id, z.side, z.per, z.T, z.lo, z.hi, z.size, z.dens, z.oldestMin, z.touchT, z.atTouch, z.cancel60, z.cnorm, z.n60, z.pulling ? 1 : 0, z.sold, z.refills, z.left, z.reloadX, z.reload,
                    z.brokeEarly ? 1 : 0, z.decT, z.sweep, z.runLots, z.lift, z.backIn ? 1 : 0, z.rebid, z.endCode, z.endT, z.held9, z.held30, z.sHeld9, z.bigSz, z.state, z.stateT, z.sHeld9b, z.reclaimBars, z.nearKind, z.nearName.c_str(), z.nearGapAtr, z.levelSearch ? 1 : 0, [&]() { static std::string s; s.clear(); for (size_t k = 0; k < z.tags.size(); ++k) { if (k) s += ";"; s += z.tags[k].name; } return s.c_str(); }());
    }
    if (argc > 4 && std::strcmp(argv[4], "-") != 0) { FILE* f = std::fopen(argv[4], "w"); if (f) { for (size_t i = 0; i < E.out.size(); ++i) std::fprintf(f, "%s\n", E.out[i].text.c_str()); std::fclose(f); } }
    std::fprintf(stderr, "%s events %lld fills %lld minutes %lld zones %zu lines %zu ghosts %lld max book %zu\n", argv[1], E.events, E.fills, E.minutes, E.zones.size(), E.out.size(), E.ghosts, E.maxBook);
    return 0;
}
