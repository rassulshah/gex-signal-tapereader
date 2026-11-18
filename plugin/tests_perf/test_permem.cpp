// t_permem.cpp -- PerfGuard 1.1.0: the memory a call adds is pinned to its plugin / chart (permem file)
#include "PerfGuard.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
struct M { std::string sym; int spb; long bars; double leakDraw; double leakCalc;
  char* getSymbol() { return &sym[0]; } int getSecondsPerBar() { return spb; } long getBarCount() { return bars; }
  void calc() { LSPERF_SCOPE("RevRead", "calc"); lsperf::fakePrivMb() += leakCalc; }
  void draw() { LSPERF_SCOPE("RevRead", "draw"); lsperf::fakePrivMb() += leakDraw; } };
int fails = 0; void ok(bool c, const char* w) { if (!c) { ++fails; std::cout << "FAIL " << w << "\n"; } else std::cout << "ok " << w << "\n"; }
int main() {
    char tmpl[] = "/tmp/pmXXXXXX"; std::string dir = mkdtemp(tmpl); setenv("LRA_FLEX_DIR", dir.c_str(), 1);
    lsperf::resetForTest(); lsperf::fakeTzMin() = -360; lsperf::fakeWall() = 20775LL * 86400 + 16 * 3600; lsperf::fakePrivMb() = 1000;
    M a{"ESZ6", 180, 100, 0.5, 0.0}, b{"NQZ6", 180, 100, 0.0, 0.0};
    for (int k = 0; k < 10; ++k) { a.calc(); a.draw(); b.calc(); b.draw(); lsperf::fakePrivMb() += 3.0; /* growth outside any call */ lsperf::fakeMs() += 5000; }
    lsperf::fakeWall() += 60; lsperf::fakeMs() += 60000; b.calc();          // closes the minute
    std::ifstream f(dir + "/Perf/permem-RevRead-" + lsperf::dayStr(lsperf::fakeWall() - 60) + ".csv"); ok((bool)f, "permem file written");
    std::string hdr, l1, l2; std::getline(f, hdr); std::getline(f, l1); std::getline(f, l2);
    ok(hdr.find("draw_net_mb") != std::string::npos, "header");
    std::string es = l1.find("ESZ6") != std::string::npos ? l1 : l2, nq = l1.find("NQZ6") != std::string::npos ? l1 : l2;
    ok(es.find(",10,10,0.000,5.000,0.000,5.000,") != std::string::npos, "ES draw growth 5 MB pinned to ES draw");
    ok(nq.find(",10,10,0.000,0.000,0.000,0.000,") != std::string::npos, "NQ nothing (outside growth not blamed)");
    std::ifstream g(dir + "/Perf/perf-RevRead-" + lsperf::dayStr(lsperf::fakeWall() - 60) + ".csv"); std::string h2; std::getline(g, h2); if (!h2.empty() && h2.back() == 13) h2.pop_back(); ok(h2 == lsperf::header(), "perf file unchanged header");
    std::cout << (fails ? "FAILED " : "ALL PASS ") << fails << "\n"; return fails ? 1 : 0;
}
