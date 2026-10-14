// test_cobimpulse_adr_parity.cpp -- lsCobImpulse 0.3.0: the chart-side 20-day ADR and prior 15:00 CT close (CobImpulseLogic.h dailyTable /
// adrAt) printed per session for analytics/test_cob_tag_gamma.py to compare with lra.trade_def.prepare's ADR and lra.cob_tag._close15.
// input lines: ctSec,open,high,low,close   output: DAY <sessDay> <adr %.10g> <has15> <c15 %.10g>
#include "CobImpulseLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
int main(int argc, char** argv)
{
    if (argc < 2) return 2;
    std::ifstream in(argv[1]); std::string line; std::vector<long long> ct; std::vector<cimp::Bar> B;
    while (std::getline(in, line)) {
        std::stringstream ss(line); std::string f[5]; for (int k = 0; k < 5; ++k) std::getline(ss, f[k], ',');
        cimp::Bar b; ct.push_back(std::strtoll(f[0].c_str(), nullptr, 10)); b.t = ct.back(); b.o = std::strtod(f[1].c_str(), nullptr);
        b.h = std::strtod(f[2].c_str(), nullptr); b.l = std::strtod(f[3].c_str(), nullptr); b.c = std::strtod(f[4].c_str(), nullptr); b.v = 1; B.push_back(b);
    }
    const std::vector<cimp::DayAgg> D = cimp::dailyTable(ct, B);
    for (size_t k = 0; k < D.size(); ++k) std::printf("DAY %lld %.10g %d %.10g\n", D[k].day, cimp::adrAt(D, k), D[k].has15 ? 1 : 0, D[k].c15);
    return 0;
}
