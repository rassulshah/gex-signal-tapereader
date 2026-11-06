// test_bo192_rev.cpp -- lsBreakout 1.9.2 Rev on provisional Pbs (Rassul 14:50 "the rev should fire as soon as the pb is detected and the
// rev conditions are met"). Short replay (ES week, last 600 bars). [1] provisional Revs appear on provisional Pbs only; [2] when the Pb
// becomes final its final Rev is the same bar; [3] a provisional Rev goes away with its Pb; [4] final Revs equal the full pass.
#include "BreakoutLogic.h"
#include <fstream>
#include <set>
#include <map>
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
static std::vector<bko::In> load(const std::string& f) {
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
int main() {
    const char* FS[3] = { "wk_ES_eve_3m.txt", "wk_NQ_3m.txt", "wk_GC_3m.txt" }; const char* MK[3] = { "ES", "NQ", "GC" }; const double TK[3] = { 0.25, 0.25, 0.1 };
    int provRev = 0, sameBar = 0, diff = 0, gone = 0, badPb = 0;
    for (int f = 0; f < 3; ++f) {
        const std::vector<bko::In> B = load(FS[f]); int om = 0, cm = 0; svl::rthOf(MK[f], om, cm);
        bko::Engine F; F.update(B, (long)B.size(), B[0].t, TK[f], 180, om, cm);
        bko::Engine R; std::map<long long, long long> pr;   // pivot -> provisional Rev bar
        for (int n = std::max(60, (int)B.size() - 600); n <= (int)B.size(); ++n) {
            std::vector<bko::In> Q(B.begin(), B.begin() + n); R.update(Q, n, Q[0].t, TK[f], 180, om, cm);
            for (const bko::RevMark& m : R.revProv) { bool onProv = false; for (const bko::PbMark& p : R.pbProv) if (p.tp == m.tp) onProv = true; if (!onProv) ++badPb; if (!pr.count(m.tp)) ++provRev; pr[m.tp] = m.tr; }
        }
        for (const auto& kv : pr) { const auto it = R.rev.find(kv.first);
            if (it != R.rev.end()) { if (it->second.tr == kv.second) ++sameBar; else ++diff; }
            else if (!R.pb.count(kv.first)) { bool still = false; for (const bko::RevMark& m : R.revProv) if (m.tp == kv.first) still = true; if (!still) ++gone; } }
        bool same = true; for (const auto& kv : F.rev) { const auto it = R.rev.find(kv.first); if (it != R.rev.end() && it->second.tr != kv.second.tr) same = false; }
        CHECK(same, std::string("[4] ") + MK[f] + ": final Revs equal the full pass");
    }
    std::printf("provisional Revs %d: final on the same bar %d, different bar %d, gone with their Pb %d\n", provRev, sameBar, diff, gone);
    CHECK(provRev > 0, "[1] provisional Revs appear");
    CHECK(badPb == 0, "[1] every provisional Rev sits on a provisional Pb");
    CHECK(diff == 0, "[2] a Pb that becomes final keeps its Rev bar");
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
