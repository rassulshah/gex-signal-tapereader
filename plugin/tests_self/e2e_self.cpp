// e2e_self.cpp -- writes REAL Self-DP records with SelfAware.h (sa::Hook) for 12 sessions of a seeded random walk, for lra.self_aware tests.
// g++ -std=c++17 -Wall -Wextra -DHB_FAKE -I. -I<plugin> e2e_self.cpp && LRA_FLEX_DIR=<dir> ./a.out
#include <cstdio>
#include "SelfAware.h"
int main() {
    unsigned s = 12345; auto rnd = [&]() { s = s * 1103515245u + 12345u; return (double)((s >> 8) & 0xFFFF) / 65536.0; };
    sa::Hook h; h.ind = "DP"; int sig = 0;
    for (int d = 0; d < 12; ++d) {
        const long long day0 = 1792972800LL + d * 86400LL;            // 2026-10-26 + d (weekends included: fine for a test)
        double px = 5000;
        for (int i = 0; i < 400; ++i) {
            const long long t = day0 + 8 * 3600 + 30 * 60 + i * 60;
            const double o = px; px += (rnd() - 0.5) * 4; const double c = px; const double hi = std::max(o, c) + rnd(), lo = std::min(o, c) - rnd();
            h.bar("ESZ6", 60, "e2e", "ES", t * 1000, sa::Bar{ t, o, hi, lo, c }, 1 + rnd() * 4);
            if (i > 20 && i % 17 == 0) { h.signal("ES", 60, t, rnd() < 0.5 ? 1 : -1, "", 50, 1 + rnd() * 4, "e2e", c); ++sig; }
        }
    }
    h.bar("ESZ6", 60, "e2e", "ES", 1, sa::Bar{ 1792972800LL + 13 * 86400LL, 1, 1, 1, 1 }, 1);   // roll: the last session's pending -> EXP
    std::printf("e2e signals %d writes %ld fails %ld\n", sig, h.writes, h.writeFails);
    return h.writeFails ? 1 : 0;
}
