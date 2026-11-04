// test_liquidityprofile_0731.cpp - lsLiquidityProfile 0.73.1 (2026-11-04 audit, Rassul "just build it"): K on WIDE days.
// Kings were allowed only between today's high - ADR and today's low + ADR; on a day whose range is over 2 ADR that window is empty
// (ES 11-04 status "today's range is over 2 ADR: no kings"). Now: last price +- 1 ADR on such a day. SDK-free (lqp::kingWindow) + versions.
// g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -I. test_liquidityprofile_0731.cpp -o t && ./t
#include "LiquidityProfileLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int P = 0, F = 0;
static void ok(bool c, const std::string& m) { if (c) ++P; else { ++F; std::printf("FAIL %s\n", m.c_str()); } }
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main()
{
    const double t = 0.25; lqp::Tick a = 0, b = 0; std::string why;
    // a normal day: range 60 < 2 x ADR 80 -> [high - ADR, low + ADR] (unchanged from 0.69.0)
    ok(lqp::kingWindow(7900, 7840, 7870, 80, t, a, b, why) && a == (lqp::Tick)(7820 / t) && b == (lqp::Tick)(7920 / t) && why == "today's high - ADR .. low + ADR", "normal day: high - ADR .. low + ADR (" + why + ")");
    // exactly 2 ADR: the window is one price (still valid)
    ok(lqp::kingWindow(8000, 7840, 7900, 80, t, a, b, why) && a == b && a == (lqp::Tick)(7920 / t), "range exactly 2 ADR: one-price window");
    // ES 11-04: a wide day (range 210 > 2 x 80) -> last price +- 1 ADR, never empty
    ok(lqp::kingWindow(7950, 7740, 7760.25, 80, t, a, b, why) && a == (lqp::Tick)std::llround(7680.25 / t) && b == (lqp::Tick)std::llround(7840.25 / t) && why.find("wide day") == 0, "wide day: last price +- 1 ADR (" + why + ")");
    ok(b > a && a > 0, "wide day: a real window (kings possible)");
    // the window follows the last price on a wide day
    lqp::Tick a2 = 0, b2 = 0; lqp::kingWindow(7950, 7740, 7900, 80, t, a2, b2, why);
    ok(a2 - a == (lqp::Tick)std::llround((7900 - 7760.25) / t) && b2 - b == a2 - a, "wide day: the window moves with the last price");
    // no ADR / no bars / bad last: no window, the status says why
    ok(!lqp::kingWindow(7900, 7840, 7870, -1, t, a, b, why) && a == 1 && b == 1 && why == "no ADR: no kings", "no ADR: no kings");
    ok(!lqp::kingWindow(0, 1, 7870, 80, t, a, b, why) && why == "no bars today: no kings", "no bars today: no kings");
    ok(!lqp::kingWindow(7950, 7740, NAN, 80, t, a, b, why) && a == 1 && b == 1, "wide day, no last price: no window");
    ok(!lqp::kingWindow(7950, 7740, 7800, NAN, t, a, b, why), "NaN ADR: no window");
    // HG ticks
    ok(lqp::kingWindow(5.30, 5.10, 5.25, 0.08, 0.0005, a, b, why) && a == (lqp::Tick)std::llround(5.17 / 0.0005) && b == (lqp::Tick)std::llround(5.33 / 0.0005), "HG wide day: last +- ADR in HG ticks");
    // versions
    const std::string cpp = slurp("LiquidityProfile.cpp"), set = slurp("LiquidityProfileSettings.h");
    ok(std::string(lqp::VERSION) == "0.73.1" && cpp.find("#define LQ_VERSION \"0.73.1\"") != std::string::npos && cpp.find("p->setVersion(\"0.73.1\")") != std::string::npos && set.find("PLUGIN_VERSION = \"0.73.1\"") != std::string::npos, "versions 0.73.1 agree");
    ok(cpp.find("lqp::kingWindow(sh, sl, (double)B.back().c, v, S.tick, a, b, why)") != std::string::npos && cpp.find("today's range is over 2 ADR: no kings") == std::string::npos, "the host uses kingWindow; the old 'no kings' rule is gone");
    std::printf("lsLiquidityProfile %s: %d passed, %d failed\n", lqp::VERSION, P, F);
    return F ? 1 : 0;
}
