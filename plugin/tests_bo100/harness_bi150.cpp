// harness_bi150: lsBreakout 1.5.0 BI decisions (one full pass) for the LRA port's oracle; argv: bars tick om cm ; env PROTO=1 = biCancel false
// prints "BI t dir t0 t_fire kind waiting pivot stop range sum10 prior_range vol prior_vol daykey session" (CT-naive epoch seconds) + "CX t0"
#include "BreakoutLogic.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
static const long long OFF = 2082844800LL;
int main(int argc, char** argv) {
    if (argc < 5) return 2;
    std::ifstream in(argv[1]); std::string l; std::vector<bko::In> B;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; B.push_back(b); }
    bko::Engine E; if (getenv("PROTO")) E.biCancel = false;
    E.update(B, (long)B.size(), B[0].t, atof(argv[2]), 180, atoi(argv[3]), atoi(argv[4]));
    for (const auto& kv : E.bi) { const auto& m = kv.second;
        std::printf("BI %lld %d %lld %lld %s %d %.4f %.4f %lld %lld %lld %lld %lld %lld %s\n", m.t - OFF, m.dir, m.t0 - OFF, m.tf ? m.tf - OFF : 0, bko::biKindName(m.kind), m.cancel ? 1 : 0,
            m.piv, m.stop, m.rng, m.sum10, m.prng, m.vol, m.pvol, m.sess, bko::sessName(m.skind)); }
    for (long long t : E.boCancel) std::printf("CX %lld\n", t - OFF);
}
