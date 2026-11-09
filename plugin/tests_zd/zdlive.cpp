// zdlive  --  (lsZoneDefense 1.0.5, live floors) replays the LIVE recorded market-by-order (the IRT recorder's text lines, CT clock,
// "time|action|price|prev_price|size|order_id|aggressor_id|side|bid|ask") through the plugin's own engine (ZoneDefenseLogic.h) the way
// the chart sees it: one continuous stream, the book only from what was recorded (a since-connect Rithmic book); a gap > 30 min or a
// clock step back = IRT was closed -> a new engine (cold book, the 10-min warm-up). lra.zone_defense feeds stdin in session order.
// args: MARKET tick f0,f1,...,f8 (the period floors, asia..post). stdout: one line per zone "YYYY-MM-DD per size state_held9 state low_floor side".
#include "ZoneDefenseLogic.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
static long long daysFrom(int y, int m, int d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
static long long parseT(const char* s) { int Y = 0, M = 0, D = 0, h = 0, mi = 0, se = 0; if (std::sscanf(s, "%d-%d-%d %d:%d:%d", &Y, &M, &D, &h, &mi, &se) != 6) return -1;
    return daysFrom(Y, M, D) * 86400 + h * 3600 + mi * 60 + se; }   // CT wall clock as epoch (the engine runs with ctOffsetMin 0)
static void civil(long long t, int& Y, int& M, int& D) { long long z = t / 86400 + 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153;
    D = (int)(doy - (153 * mp + 2) / 5 + 1); M = (int)(mp < 10 ? mp + 3 : mp - 9); Y = (int)(yoe + era * 400 + (M <= 2)); }
int main(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "usage: zdlive MKT tick f0,...,f8 < recorder lines\n"); return 2; }
    const double tick = std::atof(argv[2]); if (!(tick > 0)) return 2;
    zdp::Mkt M = zdp::forMarket(argv[1]); { const char* s = argv[3]; for (int p = 0; p < zdp::P_N && s; ++p) { M.per[p].minLots = std::atoll(s); s = std::strchr(s, ','); if (s) ++s; } }
    std::unique_ptr<zd::Engine> E; long long lastTs = -1, ev = 0; int engines = 0;
    auto flush = [&]() { if (!E) return; E->finish(E->lastT + 1, 4);
        for (size_t i = 0; i < E->zones.size(); ++i) { const zd::Zone& z = E->zones[i]; int Y, Mo, D; civil(z.T / 1000, Y, Mo, D);
            std::printf("%04d-%02d-%02d %d %lld %d %d %d %d\n", Y, Mo, D, z.per, z.size, z.sHeld9b, z.state, z.lowFloor ? 1 : 0, z.side); }
        E.reset(); };
    static char ln[1024];
    while (std::fgets(ln, sizeof ln, stdin)) {
        if (ln[0] != '2') continue;
        char* p[10]; int k = 0; p[k++] = ln; for (char* c = ln; *c && k < 10; c++) if (*c == '|') { *c = 0; p[k++] = c + 1; }
        if (k < 8) continue;
        const long long ts = parseT(p[0]); if (ts < 0) continue; const char* a = p[1];
        zd::In e; e.t = ts * 1000; e.act = !std::strcmp(a, "NEW") ? 0 : !std::strcmp(a, "MOD") ? 1 : !std::strcmp(a, "DEL") ? 2 : !std::strcmp(a, "FILL") ? 3 : 9; if (e.act == 9) continue;
        e.side = p[7][0] == 'B' ? 0 : (p[7][0] == 'S' ? 1 : -1); e.px = (long long)std::llround(std::atof(p[2]) / tick); e.sz = std::atoll(p[4]); e.oid = std::atoll(p[5]);
        if (lastTs >= 0 && (ts - lastTs > 1800 || ts < lastTs - 5)) flush();
        if (!E) { E.reset(new zd::Engine(M)); E->barSec = 180; E->ctOffsetMin = 0; ++engines; }
        E->feed(e); lastTs = ts; ++ev;
    }
    flush();
    std::fprintf(stderr, "%s events %lld engines %d\n", argv[1], ev, engines);
    return 0;
}
