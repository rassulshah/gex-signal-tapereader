// tests_turn/soak.cpp -- leak / soak test of lsTurn 1.3.0 (1.3.0: every other cycle runs the DEFAULT = the four pieces, the others Absorption only by the display file; was) lsTurn 1.2.1 (1.2.0: every other cycle runs JUST EFFORT - no display file -, the others the saved full setting; the record must be the same in both; 1.1.0: every candle carries Effort bars and has a strip under the mouse; 1.0.1: the mouse moves over the pane in every cycle). 40 times: add the chart, feed Friday's real ES order events candle by candle
// (about 2.5 million events a cycle, the reads and the level candles decided and recorded), draw, remove the chart. Memory must not grow.
// build as test_turn_host.cpp (plain for the RSS numbers, -fsanitize=address,undefined for the leak check); needs TURN_TEST_DATA.
#define main turn_host_test_main
#include "test_turn_host.cpp"
#undef main
#include <unistd.h>
static long rssKb() { long sz = 0, rss = 0; FILE* f = std::fopen("/proc/self/statm", "r"); if (!f) return -1; if (std::fscanf(f, "%ld %ld", &sz, &rss) != 2) rss = -1; std::fclose(f); return rss * (sysconf(_SC_PAGESIZE) / 1024); }
int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const int reps = argc > 1 ? std::atoi(argv[1]) : 40;
    const char* tmp = std::getenv("TMPDIR"); root = std::string(tmp ? tmp : "/tmp") + "/turn_soak_u"; setenv("USERPROFILE", root.c_str(), 1);
    const char* td = std::getenv("TURN_TEST_DATA"); if (!td) { std::printf("set TURN_TEST_DATA\n"); return 2; } data = td;
    const long long K845 = 1795164300LL, K903 = 1795165380LL, K906 = K903 + 180;
    put(P("LRA-TurnPane-ES.csv"), get(data + "/LRA-Turn-ES.csv")); put(P("RevRead\\wy-ES-180.txt"), get(data + "/wy-ES.txt"));
    Sim base; base.tick = 0.25; base.k0 = K845 - 180; base.load(data + "/ev_ES.txt", K845 - 360, K906 + 30);
    std::printf("events a cycle %zu\n", base.ev.size());
    cppExtension* x = CreateExtension();
    long first = -1, last = -1, peak = 0; long long drawsTotal = 0; std::string rec0;
    for (int rep = 0; rep < reps; ++rep) {
        std::remove(P("TurnPane\\turn-ES-180.txt").c_str());
        const bool full = rep % 2 == 1;                                // (1.2.0) the display file: every piece on / none (Effort only)
        if (!full) put(P("TurnPane\\display.txt"), "TURNSHOW|1\nstop|0\nexhaustion|0\nanswer|0\nread|0\nEND\n"); else std::remove(P("TurnPane\\display.txt").c_str());   // (1.3.0) no file = the four pieces
        Sim s; s.tick = base.tick; s.k0 = base.k0; s.ev = base.ev; s.vap = base.vap; Sim idle; idle.tick = 0.1; idle.k0 = base.k0;   // a second, quiet chart of another market on the same DLL
        for (Sim* q : { &s, &idle }) { q->H.feedp = &q->F; q->H.tick = (float)q->tick; q->H.seconds = 180; q->H.barW = 60; q->H.barX0 = 600; q->H.paneTop = 0; q->H.paneBottom = 300; q->H.paneRight = 1400; q->H.textPx = 0.6; }
        s.H.symbol = "ESZ6"; s.H.root = "ES"; idle.H.symbol = "GCZ6"; idle.H.root = "GC";
        s.chartTo(K845 - 180); idle.chartTo(K845 - 180); cycle(x, s.H); cycle(x, idle.H);
        for (long long forming = K845 - 180; forming <= K906 + 180; forming += 180) { s.chartTo(forming); idle.chartTo(forming); s.feedTo(forming <= K906 ? forming : K906 + 30); drain(x, s, idle); drawsTotal += (long long)s.H.draws.size(); }
        CHECK(s.F.at == s.F.ev.size());
        for (long b = 0; b < (long)s.H.bars.size(); ++b) { mouseAt(x, s.H, b, s.H.barX0 + (int)b * s.H.barW, b % 2 ? 100 : 250); cycle(x, s.H); drawsTotal += (long long)s.H.draws.size(); }   // (1.0.1) the mouse over every candle, both sides of the axis
        turn_test_window = nullptr; cycle(x, s.H);                     // ... and out of the pane again
        const std::string rec = get(P("TurnPane\\turn-ES-180.txt")); if (rep == 0) rec0 = rec;
        CHECK(countLines(rec, "T|") == 2 && rec == rec0);              // every cycle decides the same reads, byte for byte
        if (full) CHECK(stripText(s.H).find("ONH 9:03 AM   Heavy Stop Run / ") == 0 && stripText(s.H).find(" Absorption / ") != std::string::npos && stripText(s.H).find(" Initiative   ") != std::string::npos);
        else CHECK(stripText(s.H).find("9:06 AM   High: ") == 0 && stripText(s.H).find(" Absorption / Low: ") != std::string::npos && stripText(s.H).find("Effort") == std::string::npos);
        mock::current = &s.H; x->destroy(); CHECK(s.F.live == 0); CHECK(s.H.data == nullptr); CHECK(s.H.timers.empty());
        mock::current = &idle.H; x->destroy(); CHECK(idle.F.live == 0 && idle.H.data == nullptr);
        const long r = rssKb(); if (rep == 4) first = r; last = r; if (r > peak) peak = r;
        if (rep < 3 || rep % 5 == 4) std::printf("cycle %2d  rss %ld KB\n", rep + 1, r);
    }
    { TurnPane* px = static_cast<TurnPane*>(x);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
      delete px;
#pragma GCC diagnostic pop
    }
    std::printf("rss after cycle 5: %ld KB, after the last: %ld KB, growth %ld KB, peak %ld KB, draws %lld\n", first, last, last - first, peak, drawsTotal);
    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
