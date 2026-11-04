// harness3: lsBreakout engine decisions in the test_bo120_week line format; argv: bars tick om cm [replay] ; env NOCARRY=1, STRICT=1
#include "BreakoutLogic.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
static const long long OFF = 2082844800LL;
int main(int argc, char** argv) {
    std::ifstream in(argv[1]); std::string l; std::vector<bko::In> B;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; B.push_back(b); }
    const double tk = atof(argv[2]); const int om = atoi(argv[3]), cm = atoi(argv[4]); const bool rep = argc > 5;
    bko::Engine E; if (getenv("NOCARRY")) E.zzCarry = false; if (getenv("STRICT")) E.ftTie = false;
    if (rep) { for (int N = 60; N <= (int)B.size(); ++N) { std::vector<bko::In> P(B.begin(), B.begin() + N); E.update(P, N, P[0].t, tk, 180, om, cm); } }
    else E.update(B, (long)B.size(), B[0].t, tk, 180, om, cm);
    char b[160];
    for (const auto& kv : E.bo) { const auto& D = kv.second; std::snprintf(b, sizeof b, "BO %lld %lld %d %d %s %.4f", D.t0 - OFF, D.t - OFF, D.dir, D.fired ? 1 : 0, bba::boReasonName(D.reason), D.piv); std::puts(b); }
    for (const auto& kv : E.rly) { const auto& R = kv.second; std::snprintf(b, sizeof b, "RLY %lld %d %d", R.te - OFF, R.dir, R.fired ? 1 : 0); std::puts(b); }
    for (const auto& kv : E.pb) { const auto& P = kv.second; std::snprintf(b, sizeof b, "PB %lld %lld %d", P.tp - OFF, P.t0 - OFF, P.dir); std::puts(b); }
}
