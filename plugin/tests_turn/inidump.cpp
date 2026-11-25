// inidump.cpp -- replay an event file through turn::Live; print each complete candle's Initiative order details and iniCalc (both ends)
#include "../TurnLogic.h"
#include <cstdio>
#include <fstream>
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    turn::Live L; L.reset(180);
    std::ifstream f(argv[1]); long long sec, act, bs, px, sz, oid, aid;
    while (f >> sec >> act >> bs >> px >> sz >> oid >> aid) L.onEvent(sec, (int)act, (int)bs, px, sz, oid, aid);
    L.closeThrough(L.forming());
    for (const turn::Sum& s : L.sums) { if (!s.complete) continue;
        for (int e : { 1, -1 }) { const turn::IniCalc c = turn::iniCalc(L, s.key, e, 180); const turn::IniQ& q = s.iq[e > 0 ? 0 : 1];
            std::printf("%lld %d lo %lld hi %lld big %lld %lld-%lld sweep %lld burst %lld | calc feed %d burstx %lld\n", s.key, e, s.loPx, s.hiPx, q.big.lots, q.big.lo, q.big.hi, q.sweep, q.burst, c.feed ? 1 : 0, c.burst1000); } }
}
