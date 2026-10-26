// test_bo129.cpp -- lsBigBidAsk 1.29.0 BO / BD rule 3 (Rassul 2026-10-26 18:06-18:20 CT), SDK-free (BigBidAskLogic.h).
//   decided at the BO candle's own close (no close beyond the pivot, no confirming candle), every session (no RTH-only, no skip
//   minutes), counter-trend needs a 20-bar extreme, one per wave, a poke that did not CLOSE beyond keeps the swing intact.
//   [real] ES 2026-10-26 3-min (ChartView export, Central bar-end stamps): 09:12 BO (closed 1.75 below the 7704 swing: rule 3 fires it
//          at its own close); 09:45 poke of 7719.5 (range / volume below prior: not a BO, does not use the swing up) then 09:48 BO;
//          10:48 BD (Rassul counts it): reported with the rule that decides it.
// Run:  g++ -std=c++17 -I.. test_bo129.cpp -o t && ./t es_2026-10-26_3m.txt
#include "BigBidAskLogic.h"
#include "VwapTrendLogic.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("PASS %s\n", std::string(m).c_str()); } else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
struct Bar { long days; int endSec; double o, h, l, c, v; std::string clk; };
static std::vector<Bar> load(const char* f) {
    std::ifstream in(f); std::string l; std::vector<Bar> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        Bar b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60; b.o = o; b.h = hi; b.l = lo; b.c = c; b.v = vo; b.clk = l.substr(11, 5); v.push_back(b); }
    return v;
}
struct Run { bba::BoRun R; std::vector<Bar> B; std::vector<signed char> up, dn; std::vector<double> upPx, dnPx; std::vector<signed char> trend; };
static Run run(const std::vector<Bar>& B, int nUse, bool rule3Inputs) {
    const double tick = 0.25; const int per = 180, openMin = 510, closeMin = 900; Run X; X.B = B; const int n = nUse;
    std::vector<double> op(n), hi(n), lo(n), cl(n); std::vector<char> rth(n, 1); std::vector<long long> day(n, 0);
    std::vector<svl::Bar> sb(n); std::vector<char> skip(n, 0);
    for (int i = 0; i < n; ++i) { op[i] = B[i].o; hi[i] = B[i].h; lo[i] = B[i].l; cl[i] = B[i].c;
        day[i] = B[i].days + (B[i].endSec > 17 * 3600 ? 1 : 0);                 // the full Globex session (as the 1.29.0 host)
        if (!rule3Inputs) { const svl::Cls c = svl::classify(B[i].days, B[i].endSec, per, openMin, closeMin); rth[i] = c.kind == svl::RTH; }
        sb[i] = svl::Bar{ B[i].days, B[i].endSec, (float)B[i].h, (float)B[i].l, (float)B[i].c, B[i].v };
        skip[i] = 1; }                                                           // rule 3 ignores the time filter: every candle "skipped"
    bba::swingBreaksPx(op, hi, lo, cl, rth, day, n, tick, X.up, X.dn, X.upPx, X.dnPx);
    std::vector<signed char> zd; bba::zigDir(hi, lo, n, zd);
    std::vector<bba::BoIn> in(n); std::vector<bba::BoBar> bars(n); X.trend.assign(n, 0);
    for (int i = 0; i < n; ++i) { bba::BoIn& x = in[i]; x.known = true; x.hk = std::llround(hi[i] / tick); x.lk = std::llround(lo[i] / tick); x.vol = (long long)B[i].v;
        bba::BoBar& b = bars[i]; b.ok = true; b.o = std::llround(op[i] / tick); b.c = std::llround(cl[i] / tick); b.h = x.hk; b.l = x.lk;
        X.trend[i] = (signed char)vtl::level(vtl::fromChart(sb, i, per, openMin, closeMin)); }
    X.R = bba::boWaves(in, bars, zd, X.up, X.dn, X.upPx, X.dnPx, skip, X.trend, day, tick, 1, n, 0);
    return X;
}
static const bba::BoDec* at(const Run& X, const std::string& clk, int dir) {
    for (const bba::BoDec& D : X.R.ev) { if (D.i0 >= 0 && X.B[(size_t)D.i0].clk == clk && D.dir == dir) return &D; }
    return nullptr; }
int main(int argc, char** argv) {
    const std::vector<Bar> B = load(argc > 1 ? argv[1] : "es_2026-10-26_3m.txt");
    CHECK(B.size() > 250, "[real] ES 2026-10-26 bars loaded (" + std::to_string(B.size()) + ")");
    const Run X = run(B, (int)B.size(), true);
    std::map<std::string, int> reasons; int fired = 0; for (const bba::BoDec& D : X.R.ev) { ++reasons[bba::boReasonName(D.reason)]; if (D.fired) ++fired; }
    CHECK(bba::BO_RULE == 3 && bba::BO_EXT20 == 20, "rule 3, 20-bar counter-trend extreme");
    CHECK(reasons["no_confirm"] == 0 && reasons["time_filter"] == 0, "no no_confirm and no time_filter decisions any more");
    for (const bba::BoDec& D : X.R.ev) if (D.fired) CHECK(D.im == D.i0 && D.later == 0, "fired " + X.B[(size_t)D.i0].clk + (D.dir > 0 ? " BO" : " BD") + " decided at its own close");
    const bba::BoDec* d912 = at(X, "09:12", +1);
    CHECK(d912 && d912->fired && d912->wick == 1, std::string("09:12 BO fires at its own close although it closed below the 7704 swing (") + (d912 ? bba::boReasonName(d912->reason) : "no candidate") + ")");
    const bba::BoDec* d945 = at(X, "09:45", +1);
    CHECK(d945 && !d945->fired && d945->reason == bba::BO_R_RANGE, std::string("09:45 poke of 7719.5: not a BO (range below prior) (") + (d945 ? bba::boReasonName(d945->reason) : "no candidate") + ")");
    const bba::BoDec* d948 = at(X, "09:48", +1);
    CHECK(d948 && d948->fired && std::fabs(d948->piv - 7719.5) < 1e-9, std::string("09:48 BO through 7719.5 (the 09:45 poke did not use the swing up) (") + (d948 ? bba::boReasonName(d948->reason) + std::string(" pivot ") + std::to_string(d948->piv) : "no candidate") + ")");
    const bba::BoDec* d1048 = at(X, "10:48", -1);
    std::printf("INFO 10:48 BD: %s\n", d1048 ? (std::string(d1048->fired ? "FIRED" : "not fired: ") + (d1048->fired ? "" : bba::boReasonName(d1048->reason)) + " pivot " + std::to_string(d1048->piv) + " trend " + std::to_string(d1048->trend) + " ext20 " + std::to_string(d1048->ext30)).c_str() : "no candidate (no intact swing low traded through)");
    // no repaint: the run over a prefix decides every candle of the prefix the same way as the full run
    { int same = 0, diff = 0; for (int cut = 140; cut < (int)B.size(); cut += 7) { const Run P = run(B, cut, true);
        for (const bba::BoDec& D : P.R.ev) { bool ok = false; for (const bba::BoDec& E : X.R.ev) if (E.i0 == D.i0 && E.dir == D.dir && E.fired == D.fired && E.reason == D.reason) ok = true; ok ? ++same : ++diff; } }
      CHECK(diff == 0 && same > 0, "a prefix run decides every candle the same as the full run (no repaint) (" + std::to_string(same) + " / " + std::to_string(diff) + ")"); }
    // record round trip: a fired rule-3 line (t == t0) parses; t != t0 is refused
    { bba::BoDec D = d948 ? *d948 : bba::BoDec(); D.t = D.t0 = 3875852880; const std::string L = bba::boLine(D, 1, "1.29.0"); bba::BoDec P;
      CHECK(bba::parseBoLine(L, P) && P.rule == 3 && P.fired == D.fired, "rule-3 record line round trip");
      bba::BoDec D2 = D; D2.t = D.t0 + 180; bba::BoDec P2; CHECK(!bba::parseBoLine(bba::boLine(D2, 1, "1.29.0"), P2), "a rule-3 line with t != t0 is refused"); }
    std::printf("ES 2026-10-26 rule 3: %d fired;", fired); for (const auto& kv : reasons) std::printf(" %s %d;", kv.first.c_str(), kv.second); std::printf("\n");
    for (const bba::BoDec& D : X.R.ev) if (D.fired) std::printf("  %s %s pivot %.2f (%d back) trend %d\n", X.B[(size_t)D.i0].clk.c_str(), D.dir > 0 ? "BO" : "BD", D.piv, D.which, D.trend);
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
