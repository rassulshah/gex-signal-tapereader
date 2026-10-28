// test_combiner.cpp -- CombinerLogic.h 0.1.0: the native read equals the research model on 300 real candles (7 markets, the MBO archive),
// the params file parses, a bad file falls back to the built-in fit, non-pivot candles give no read, odds stay in (0, 1).
// From plugin\:  g++ -std=c++17 -O1 -I. tests_chain/test_combiner.cpp -o tcm && ./tcm tests_chain
#include "CombinerLogic.h"
#include <cstdio>
#include <fstream>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "tests_chain";
    cmb::Params P = cmb::load(dir + "/LRA-Combiner.csv");
    CHECK(P.ok && P.why == "nightly params", "[params] the nightly file parses");
    CHECK(P.atrMed.count("ES") && std::fabs(P.showTurn - 0.72) < 1e-9, "[params] ATR medians and the show threshold");
    cmb::Params bad = cmb::parse("VERSION,1\nFEATURES,A,wick\nMODEL,turn,1,2\n");
    CHECK(!bad.ok, "[params] a short file is rejected");
    cmb::Params miss = cmb::load(dir + "/missing.csv");
    CHECK(miss.ok && miss.why.find("built-in") != std::string::npos, "[params] a missing file uses the built-in fit and says so");
    std::ifstream in((dir + "/parity_combiner.txt").c_str()); std::string line; int n = 0, okT = 0, okS = 0, piv = 0; double worst = 0;
    while (std::getline(in, line)) {
        const size_t bar = line.find('|'); if (bar == std::string::npos) continue;
        const std::vector<std::string> h = cmb::split(line.substr(0, bar), ',');
        const std::vector<std::string> cs = cmb::split(line.substr(bar + 1), ';');
        std::vector<cmb::Candle> c;
        for (const std::string& s : cs) { const std::vector<std::string> v = cmb::split(s, ':'); cmb::Candle k; cmb::num(v[0], k.o); cmb::num(v[1], k.h); cmb::num(v[2], k.l); cmb::num(v[3], k.c); cmb::num(v[4], k.v); double md; cmb::num(v[5], md); k.mod = (int)md; c.push_back(k); }
        cmb::Inputs I; I.A = h[2] == "1"; I.wick = h[3] == "1"; I.node = h[4] == "1"; I.key = h[5] == "1"; I.gamma = h[6] == "1"; I.bigfill = h[7] == "1";
        double tick, et, es; cmb::num(h[10], tick); cmb::num(h[11], et); cmb::num(h[12], es);
        const cmb::Read r = cmb::score(P, c, (int)c.size() - 1, h[1] == "1", I, h[0], std::atoi(h[8].c_str()), std::atoi(h[9].c_str()), tick);
        ++n; if (r.pivot) ++piv;
        const double dt = std::fabs(r.turn - et), ds = std::fabs(r.swing - es); worst = std::max(worst, std::max(dt, ds));
        if (dt < 2e-3) ++okT; if (ds < 2e-3) ++okS;
        if (dt >= 2e-3 && n - okT <= 3) std::printf("  diff %s turn %.5f vs %.5f swing %.5f vs %.5f\n", h[0].c_str(), r.turn, et, r.swing, es);
    }
    std::printf("  parity: %d candles, %d pivots, turn %d / swing %d within 0.002, worst %.5f\n", n, piv, okT, okS, worst);
    CHECK(n == 300 && piv == n, "[parity] every research candle is a pivot-reversal candle natively too");
    CHECK(okT == n && okS == n, "[parity] the native odds equal the research model on all 300 candles");
    std::vector<cmb::Candle> flat(30); for (int i = 0; i < 30; ++i) { flat[i].o = 100; flat[i].h = 101; flat[i].l = 99; flat[i].c = 100; flat[i].v = 10; flat[i].mod = 600; }
    CHECK(!cmb::score(P, flat, 29, true, cmb::Inputs(), "ES", 510, 900, 0.25).pivot, "[shape] a flat candle is no read");
    std::printf("CombinerLogic %s: %d passed, %d failed\n", cmb::VERSION, passes, fails);
    return fails ? 1 : 0;
}
