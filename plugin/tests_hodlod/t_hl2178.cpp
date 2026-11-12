// build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I.. t_hl2178.cpp -o t2178 && ./t2178   (from plugin\tests_hodlod)
// lsHodLod 2.17.8 (2026-11-04 audit, Rassul "just build it") - ONE London rule everywhere: London = 02:00 CT to the EARLIER of 07:30 and the
// market's RTH open (svl::rthOf, the open KeyLevelsLogic and the LRA use). SDK-free:
//  [1] lonEndMin == min(07:30, svl::rthOf open) for all 7 markets (ES / NQ / CL / NG 07:30, GC / EU 07:20, HG 07:10)
//  [2] the Tracker's LonHI = KeyLevelsLogic's rule (bar START in [02:00, open), END by 07:30): HG 07:12-07:15 is not London, 07:06-07:09 is;
//      GC 07:21-07:24 is not, 07:18-07:21 (starts before the 07:20 open) is - as the Dealer Profile's LonHI badge; ES 07:30-07:33 is not
//  [3] versions 2.17.8 everywhere
#include "HodLodLogic.h"
#include "SessionVWAPLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int P = 0, F = 0;
static void ok(bool c, const std::string& m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m.c_str()); } }
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }

// London high of one market after feeding 3-min bars 00:00 .. 08:00 CT of day D, flat at 'base' except one spike bar starting at spikeMin
static double lonHigh(int mi, double tick, double base, int spikeMin, double spike)
{
    hl::Cfg c; c.mkt = mi; c.per = 180; c.tick = tick; hl::Tracker T(c);
    const long long D = 20400;
    for (int m = 0; m < 8 * 60; m += 3) {
        hl::Bar b; b.absEnd = D * 86400 + (long long)(m + 3) * 60;
        b.o = base; b.l = base - tick; b.c = base; b.h = (m == spikeMin) ? spike : base + tick; b.v = 10;
        T.push(b);
    }
    return T.curLon.ok ? T.curLon.hi * tick : -1;
}
int main()
{
    // [1]
    const char* M[7] = { "ES", "NQ", "CL", "GC", "HG", "NG", "EU" };
    for (int i = 0; i < 7; ++i) {
        int o = 0, cl = 0; const bool have = svl::rthOf(M[i], o, cl);
        ok(have && hl::lonEndMin(i) == std::min(7 * 60 + 30, o) && std::string(hl::MKTS[i].code) == M[i], std::string("[1] ") + M[i] + " London ends at min(07:30, RTH open " + std::to_string(o) + ") = " + std::to_string(hl::lonEndMin(i)));
    }
    ok(hl::lonEndMin(4) == 430 && hl::lonEndMin(3) == 440 && hl::lonEndMin(0) == 450 && hl::lonEndMin(2) == 450, "[1] HG 07:10, GC 07:20, ES / CL 07:30");
    // [2]
    {   const double hg = 0.0005;
        ok(std::fabs(lonHigh(4, hg, 4.5, 7 * 60 + 12, 4.6) - (4.5 + hg)) < 1e-9, "[2] HG: a spike 07:12-07:15 (after the 07:10 open) is not the LonHI");
        ok(std::fabs(lonHigh(4, hg, 4.5, 7 * 60 + 6, 4.6) - 4.6) < 1e-9, "[2] HG: a spike 07:06-07:09 is the LonHI");
        ok(std::fabs(lonHigh(3, 0.1, 2700.0, 7 * 60 + 21, 2710.0) - 2700.1) < 1e-6, "[2] GC: 07:21-07:24 (after the 07:20 open; was London until 2.17.8) is not London");
        ok(std::fabs(lonHigh(3, 0.1, 2700.0, 7 * 60 + 18, 2710.0) - 2710.0) < 1e-6, "[2] GC: 07:18-07:21 (starts before the open) is London, as KeyLevelsLogic");
        ok(std::fabs(lonHigh(4, hg, 4.5, 7 * 60 + 9, 4.6) - 4.6) < 1e-9, "[2] HG: 07:09-07:12 (starts before the 07:10 open) is London, as KeyLevelsLogic");
        ok(std::fabs(lonHigh(0, 0.25, 7800.0, 7 * 60 + 27, 7810.0) - 7810.0) < 1e-9, "[2] ES: 07:27-07:30 is London (07:30 end unchanged)");
        ok(std::fabs(lonHigh(0, 0.25, 7800.0, 7 * 60 + 30, 7810.0) - 7800.25) < 1e-9, "[2] ES: 07:30-07:33 is not London");
        ok(std::fabs(lonHigh(0, 0.25, 7800.0, 1 * 60 + 57, 7810.0) - 7800.25) < 1e-9, "[2] ES: 01:57-02:00 is not London (start unchanged)");
    }
    // [3]
    {   const std::string cpp = slurp("../HodLod.cpp"), set = slurp("../HodLodSettings.h");
        ok(std::string(hl::HL_VERSION) == "2.26.0" && cpp.find("p->setVersion(\"2.17.8\")") != std::string::npos && set.find("PLUGIN_VERSION = \"2.17.8\"") != std::string::npos, "[3] versions 2.17.8 (HL_VERSION, setVersion, settings)");
    }
    std::printf("lsHodLod %s: %d passed, %d failed\n", hl::HL_VERSION, P, F);
    return F ? 1 : 0;
}
