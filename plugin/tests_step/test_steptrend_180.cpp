// test_steptrend_180.cpp (g++ -std=c++17 -I.. test_steptrend_180.cpp && ./a.out ../tests_cobsignal/cobsignal_es.csv) - lsStepTrend 1.5.2: a 180-minute chart = each candle one step unit; the same steps as the same candles fed as hours.
#include "StepTrendLogic.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <map>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
int main(int, char** argv) {
    CHECK(stp::periodOk(10800) && stp::periodOk(3600) && stp::periodOk(180) && !stp::periodOk(7200) && !stp::periodOk(14400) && !stp::periodOk(0), "periods: 180 minutes joins 1-60 minutes; 120 / 240 stay off");
    CHECK(stp::unitFor(10800) == 10800 && stp::unitFor(180) == 3600 && stp::unitFor(3600) == 3600, "unit: 3 hours on the 180-minute chart, the hour elsewhere");
    // 3-hour candles from the ES 3-min fixture (bar END stamps), sessions from 17:00
    std::map<long long, stp::Bar> M; { std::ifstream f(argv[1]); std::string l; std::getline(f, l); long long t; double o, h, lo, c, v;
        while (std::getline(f, l)) if (std::sscanf(l.c_str(), "%lld,%lf,%lf,%lf,%lf,%lf", &t, &o, &h, &lo, &c, &v) == 6) { const long long st = t - 180, k = stp::floorDiv(st - 7200, 10800); auto it = M.find(k);
            const long long O = std::llround(o * 4), Hh = std::llround(h * 4), L = std::llround(lo * 4), C = std::llround(c * 4);
            if (it == M.end()) { stp::Bar b; b.t = k * 10800 + 7200; b.o = O; b.h = Hh; b.l = L; b.c = C; M[k] = b; } else { stp::Bar& b = it->second; b.h = std::max(b.h, Hh); b.l = std::min(b.l, L); b.c = C; } } }
    CHECK(M.size() > 200, "3-hour candles built (" << M.size() << ")");
    for (int rule = 0; rule < 2; ++rule) {
        stp::Engine A; A.rule = rule; A.unit = 10800; A.hourly = true; stp::Engine B; B.rule = rule; B.hourly = true; long long n = 0;
        for (const auto& kv : M) { stp::Bar b = kv.second; A.addBar(b); b.t = n * 3600; B.addBar(b); ++n; }
        bool same = A.S.size() == B.S.size() && A.skipped.size() == B.skipped.size() && A.H.size() == M.size();
        for (size_t i = 0; same && i < A.S.size(); ++i) { const stp::Step& x = A.S[i]; const stp::Step& y = B.S[i]; same = x.dir == y.dir && x.px == y.px && x.oi == y.oi && x.bi == y.bi && x.tI == y.tI && x.endI == y.endI && x.brI == y.brI && x.sup == y.sup; }
        CHECK(same && A.S.size() > 20, "rule " << rule << ": the 180-minute chart makes the same steps as the same candles fed as hours (" << A.S.size() << " steps, " << A.skipped.size() << " skipped)");
        bool one = true; for (const stp::Hour& h : A.H) one = one && h.n == 1; CHECK(one, "each 3-hour candle is its own unit (incl. the short 2:00-4:00 PM candle)");
        std::vector<stp::Seg> sg; std::vector<stp::Dot> dt; stp::layout(A, (int)A.B.size() - 1, sg, dt); CHECK(!sg.empty(), "the hourly layout draws on the 180-minute engine (" << sg.size() << " segments)");
    }
    std::cout << "test_steptrend_180: " << passed << " passed, " << failed << " failed\n"; return failed ? 1 : 0;
}
