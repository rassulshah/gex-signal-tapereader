// e2e_writer.cpp -- writes a REAL PerfGuard session (fake clocks) for the lra.irt_perf tests:
//   plugin A ("DeltaProfile"): ES 180 s (slow from 10:00 -> OVER), NQ 180 s, ES 180 s again on a 2nd chart (duplicate), CL 60 s hidden (calc, never draws)
//   memory: private bytes +2 MB a minute (leak), feed lag 2 s, 08:20 -> 15:10 CT. Build: g++ -std=c++17 -DLSPERF_FAKE -I../code e2e_writer.cpp
#include "PerfGuard.h"
#include <cstdio>
#include <string>
#include <unistd.h>
struct M { std::string sym; int spb; long bars; char* getSymbol() { return &sym[0]; } int getSecondsPerBar() { return spb; } long getBarCount() { return bars; }
  void calc(int us) { LSPERF_SCOPE("DeltaProfile", "calc"); if (us) usleep(us); }
  void draw(int us) { LSPERF_SCOPE("DeltaProfile", "draw"); if (us) usleep(us); } };
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    setenv("LRA_FLEX_DIR", argv[1], 1);
    lsperf::fakeTzMin() = -300;
    // 2026-10-26 08:20 CT = 13:20 UTC
    lsperf::fakeWall() = 1793020800LL - 300 * 60 + 8 * 3600 + 20 * 60 + 5 * 3600;   // placeholder, fixed below
    // compute 2026-10-26 13:20:00 UTC exactly: days since epoch for 2026-10-26 = 20752
    lsperf::fakeWall() = 20752LL * 86400 + 13 * 3600 + 20 * 60;
    M es{"ESZ6", 180, 6803}, nq{"NQZ6", 180, 6803}, es2{"ESZ6", 180, 6803}, cl{"CLZ6", 60, 60000};
    for (int m = 0; m < 410; ++m) {                       // 08:20 .. 15:10
        lsperf::fakePrivMb() = 2000 + 2.0 * m; lsperf::fakeWsMb() = 1800 + 2.0 * m;
        lsperf::noteEvent(lsperf::fakeWall() - 2);
        const bool slow = m >= 100 && m < 110;            // 10:00 .. 10:10 ES slow
        for (int k = 0; k < 6; ++k) {
            es.calc(slow ? 25000 : 200); es.draw(300);
            nq.calc(200); nq.draw(300);
            es2.calc(150); es2.draw(250);
            cl.calc(100);
            lsperf::fakeMs() += 10000;
        }
        lsperf::fakeWall() += 60;
    }
    es.calc(0);
    std::printf("written\n");
    return 0;
}
