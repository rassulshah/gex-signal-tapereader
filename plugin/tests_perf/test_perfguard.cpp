// test_perfguard.cpp -- PerfGuard.h 1.0.0 in fake mode (Linux). Build: g++ -std=c++17 -Wall -Wextra -DLSPERF_FAKE -I../code test_perfguard.cpp
#include "PerfGuard.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

static int fails = 0, passes = 0;
#define CHECK(c) do { if (c) passes++; else { fails++; std::cerr << "FAIL line " << __LINE__ << ": " #c "\n"; } } while (0)

struct Bare { int calc(int) { LSPERF_SCOPE("BarePlug", "calc"); return 0; } };   // no SDK getters at all

struct Mock {   // the three cppExtension getters PerfGuard uses
    std::string sym; int spb; long bars; int infoCalls = 0;
    char* getSymbol() { infoCalls++; return &sym[0]; }
    int getSecondsPerBar() { return spb; }
    long getBarCount() { return bars; }
    // the instrumented functions, as apply_perf.py writes them
    int calc(int, int busyUs = 0) { LSPERF_SCOPE("TestPlug", "calc"); spin(busyUs); return 0; }
    int draw(int busyUs = 0) { LSPERF_SCOPE("TestPlug", "draw"); spin(busyUs); return 0; }
    static void spin(int us) { if (us > 0) usleep((useconds_t)us); }
};

static std::string readAll(const std::string& p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int countLines(const std::string& s, const std::string& needle) { int n = 0; size_t a = 0; while ((a = s.find(needle, a)) != std::string::npos) { n++; a += needle.size(); } return n; }
static void minute(int k = 1) { lsperf::fakeWall() += 60 * k; lsperf::fakeMs() += 60000LL * k; }

int main() {
    char tmpl[] = "/tmp/lsperfXXXXXX"; const std::string dir = mkdtemp(tmpl);
    setenv("LRA_FLEX_DIR", dir.c_str(), 1);
    using namespace lsperf;

    // 1. accumulator: mean / p95 / max; p95 never above max; empty = unknown (-1 -> "?")
    { Acc a; CHECK(a.p95Ms() < 0); CHECK(fmt(a.p95Ms()) == "?");
      for (int i = 0; i < 95; ++i) a.add(1000);
      for (int i = 0; i < 5; ++i) a.add(40000);
      CHECK(a.n == 100); CHECK(a.p95Ms() == 1.0); CHECK(a.maxMs() == 40.0); CHECK(a.meanMs() > 2.9 && a.meanMs() < 3.0);
      Acc b; b.add(12345); CHECK(b.p95Ms() == 12.345);          // a single sample: capped at the max, not the bucket edge
      Acc c; c.add(-5); c.add(0.0 / 0.0); CHECK(c.n == 0); }

    // 2. budgets file: strict numbers, '*' default, bad lines skipped
    { auto B = parseBudgets("plugin,calc_ms,draw_ms,mem_mb\r\n*,25,40,6000\r\nDeltaProfile,50,abc,\r\n# note\r\nBad,-3,1e99\r\n");
      CHECK(B["*"].calcMs == 25 && B["*"].drawMs == 40 && B["*"].memMb == 6000);
      CHECK(B["DeltaProfile"].calcMs == 50 && B["DeltaProfile"].drawMs == 40);   // bad draw -> the '*' value
      CHECK(B["Bad"].calcMs == 25 && B["Bad"].drawMs == 40); }

    // 3. lag: UTC stamp, local stamp, implausible -> unknown
    { fakeTzMin() = -300; const long long now = 1793030880;
      CHECK(lagSec(now - 2, now) == 2); CHECK(lagSec(now - 300 * 60 - 3, now) == 3); CHECK(lagSec(now - 86400, now) < 0); CHECK(lagSec(0, now) < 0);
      CHECK(lagSec(now + 3, now) == 0); }

    // 4. leak slope: unknown under 30 samples; +60 MB over an hour = ~60 MB/h
    { std::vector<std::pair<long long, double> > h; for (int i = 0; i < 20; ++i) h.push_back({i * 60LL, 1000.0}); CHECK(slopeMbH(h) < -1e8);
      h.clear(); for (int i = 0; i < 60; ++i) h.push_back({i * 60LL, 1000.0 + i}); const double s = slopeMbH(h); CHECK(s > 59 && s < 61); }

    // 5. two charts on one DLL: separate rows, separate symbols, no cross-talk; the SDK getters are called once a minute, not per call
    resetForTest(); fakeWall() = 1793030880; fakeMs() = 1000;
    Mock es{"ESZ6", 180, 6803}, nq{"NQZ6", 60, 5000};
    for (int i = 0; i < 10; ++i) { es.calc(0); es.draw(); nq.calc(0); fakeMs() += 5000; }
    CHECK(es.infoCalls == 1 && nq.infoCalls == 1);
    minute(); es.calc(0);      // rolls the minute: rows written
    const std::string f1 = dir + "/Perf/perf-TestPlug-" + dayStr(fakeWall()) + ".csv";
    std::string t = readAll(f1);
    CHECK(t.find("ts_ct,kind,plugin,sym") == 0);
    CHECK(countLines(t, ",M,TestPlug,ESZ6,180,") == 1); CHECK(countLines(t, ",M,TestPlug,NQZ6,60,") == 1);
    CHECK(t.find("\r\n") != std::string::npos);
    CHECK(t.find(",ESZ6,180,") != std::string::npos && t.find(",6803,10,") != std::string::npos);   // bars 6803, 10 calcs
    // the board summary
    { const char* v = getenv("LRA_PERF_TestPlug"); CHECK(v != nullptr); std::string s = v ? v : "";
      CHECK(s.find("M.calc_p95_ms=") != std::string::npos); CHECK(s.find("M.ws_mb=1000") != std::string::npos); CHECK(s.find("M.lag_s=?") != std::string::npos);
      CHECK(s.find("M.mem_slope_mb_h=?") != std::string::npos); CHECK(s.find("M.gap_p95_s=5") != std::string::npos); }

    // 6. over budget: 3 slow minutes -> OVER event, flag on the board, smartAllowed false for ES only; 10 good minutes clear it
    resetForTest(); fakeWall() = 1793034000; fakeMs() = 900000;
    { std::ofstream b(dir + "/LRA-PerfBudget.csv"); b << "plugin,calc_ms,draw_ms,mem_mb\n*,2,30,0\n"; }
    Mock es2{"ESZ6", 180, 100}, nq2{"NQZ6", 180, 100};
    CHECK(smartAllowed("TestPlug", "ESZ6"));
    for (int m = 0; m < 3; ++m) { for (int i = 0; i < 4; ++i) { es2.calc(0, 6000); nq2.calc(0, 0); } minute(); }
    es2.calc(0); nq2.calc(0);   // closes minute 3
    CHECK(!smartAllowed("TestPlug", "ESZ6")); CHECK(smartAllowed("TestPlug", "NQZ6"));
    CHECK(statusOf(&es2).find("perf OVER calc p95") == 0); CHECK(statusOf(&nq2) == "perf ok");
    t = readAll(dir + "/Perf/perf-TestPlug-" + dayStr(fakeWall()) + ".csv");
    CHECK(countLines(t, ",EVENT,TestPlug,ESZ6,") == 1); CHECK(t.find("smart features off") != std::string::npos);
    { const char* v = getenv("LRA_PERF_TestPlug"); CHECK(v && std::string(v).find("over=ESZ6") != std::string::npos); }
    for (int m = 0; m < 10; ++m) { es2.calc(0); minute(); }
    es2.calc(0);
    CHECK(smartAllowed("TestPlug", "ESZ6")); CHECK(getenv(overKey("TestPlug", "ESZ6").c_str()) == nullptr);
    t = readAll(dir + "/Perf/perf-TestPlug-" + dayStr(fakeWall()) + ".csv");
    CHECK(t.find("CLEAR after 10 good minutes") != std::string::npos);

    // 7. a stale flag (its writer died) is ignored after 15 minutes
    setenv("LRA_PERF_OVER_Other_ESZ6", (std::to_string(fakeWall() - 2000) + "|x").c_str(), 1); CHECK(smartAllowed("Other", "ESZ6"));
    setenv("LRA_PERF_OVER_Other_ESZ6", (std::to_string(fakeWall() - 10) + "|x").c_str(), 1); CHECK(!smartAllowed("Other", "ESZ6"));
    unsetenv("LRA_PERF_OVER_Other_ESZ6");

    // 8. memory budget: process-wide flag turns every indicator's smart features off; leak slope reported after 30 minutes
    resetForTest(); fakeWall() = 1793040000;
    { std::ofstream b(dir + "/LRA-PerfBudget.csv"); b << "*,20,30,1500\n"; }
    fakeMs() += 120000;   // let the budget file be re-checked
    Mock gc{"GCZ6", 180, 100};
    for (int m = 0; m < 35; ++m) { fakePrivMb() = 1400 + 4 * m; gc.calc(0); minute(); }
    gc.calc(0);
    CHECK(!smartAllowed("AnyPlugin", "CLZ6"));     // private 1536+ > 1500 for 3+ minutes
    { const char* v = getenv("LRA_PERF_TestPlug"); std::string s = v ? v : ""; const size_t k = s.find("M.mem_slope_mb_h="); const double sl = k == std::string::npos ? -1 : atof(s.c_str() + k + 17); CHECK(sl > 230 && sl < 250); }
    t = readAll(dir + "/Perf/perf-TestPlug-" + dayStr(fakeWall()) + ".csv");
    CHECK(t.find("MEMOVER private") != std::string::npos);
    fakePrivMb() = 1000; for (int m = 0; m < 11; ++m) { gc.calc(0); minute(); } gc.calc(0);
    CHECK(smartAllowed("AnyPlugin", "CLZ6"));

    // 9. feed lag from the order feed hook
    resetForTest(); fakeWall() = 1793050000;
    Mock cl{"CLZ6", 180, 100}; cl.calc(0); noteEvent(fakeWall() + 55 - 4 * 0); minute(); noteEvent(fakeWall() - 3); cl.calc(0);
    { const char* v = getenv("LRA_PERF_TestPlug"); std::string s = v ? v : ""; CHECK(s.find("M.lag_s=3;") != std::string::npos); }

    // 10. board size counts only LRA_ entries
    { setenv("LRA_TEST_BIG", std::string(2000, 'x').c_str(), 1); Board b = boardNow(); CHECK(b.bytes >= 2000 && b.entries >= 1); unsetenv("LRA_TEST_BIG"); }

    // 11. chart cap and staleness: 64 charts max; a chart unseen for 10 minutes is dropped
    resetForTest(); fakeWall() = 1793060000;
    { std::vector<Mock> v; v.reserve(80); for (int i = 0; i < 80; ++i) v.push_back(Mock{"S" + std::to_string(i), 60, 10});
      for (auto& m : v) m.calc(0);
      { std::lock_guard<std::mutex> g(st().m); CHECK(st().charts.size() == MAX_CHARTS); }
      Mock keep{"KEEP", 60, 10};
      for (int m = 0; m < 12; ++m) { v[0].calc(0); minute(); }
      v[0].calc(0);
      { std::lock_guard<std::mutex> g(st().m); CHECK(st().charts.size() == 1); } }

    // 12. unwritable folder: no crash, rows kept for the next minute, failures counted
    resetForTest(); fakeWall() = 1793070000; setenv("LRA_FLEX_DIR", "/proc/forbidden-perf", 1);
    { Mock x{"ESZ6", 180, 1}; x.calc(0); minute(); x.calc(0); std::lock_guard<std::mutex> g(st().m); CHECK(st().writeFails >= 1); CHECK(!st().pending.empty()); }
    setenv("LRA_FLEX_DIR", dir.c_str(), 1);

    // 13. kill switch
    resetForTest(); setenv("LRA_PERF_DISABLE", "1", 1);
    { Mock x{"ESZ6", 180, 1}; x.calc(0); std::lock_guard<std::mutex> g(st().m); CHECK(st().off); CHECK(st().charts.empty()); }
    unsetenv("LRA_PERF_DISABLE");

    // 14. timing really measures: a 3 ms calc shows a p95 >= 2.5 ms
    resetForTest(); fakeWall() = 1793080000;
    { Mock x{"ESZ6", 180, 1}; for (int i = 0; i < 5; ++i) x.calc(0, 3000);
      std::lock_guard<std::mutex> g(st().m); const double p = st().charts.begin()->second.calc.p95Ms(); CHECK(p >= 2.5 && p < 50); }

    // 15. a class without the SDK getters compiles and is timed (symbol unknown, spb -1)
    resetForTest();
    { Bare b; b.calc(0); std::lock_guard<std::mutex> g(st().m); CHECK(st().charts.size() == 1); CHECK(st().charts.begin()->second.spb == -1); }

    std::cout << "perfguard: " << passes << " pass, " << fails << " fail\n";
    return fails ? 1 : 0;
}
