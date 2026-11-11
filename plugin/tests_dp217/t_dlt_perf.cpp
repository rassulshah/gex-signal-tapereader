// test_dp2321.cpp -- lsDeltaProfile 2.32.1 (2026-11-03 night, Rassul "fix all" - his 7:48 PM CT ES screenshot: the profile labels "3.9x"
// and "3x" sat 11 px apart because they did not overlap side by side). The 3 ratio labels keep at least one text row apart vertically:
// three big rows on neighbouring prices -> every pair of drawn labels is >= one label height apart (or the weaker is stepped / dropped);
// the strongest keeps its own row; versions 2.32.1 agree; no circles still.
// Build (from tests_dp217/): g++ -std=c++17 -O1 -w -fsanitize=address,undefined -I. -I../tests_delta -I.. test_dp2321.cpp -o /tmp/t_dp2321 && /tmp/t_dp2321
// (helpers from test_dp232.cpp) lsDeltaProfile 2.32.0 (2026-11-03 16:12 CT Rassul circled a gray ring on his ES chart: "get rid of the circles that
// relate to the delta profile"). DRAWING ONLY: while A records exist (D lines, SignalBoard A final, shared A SAB line, status counts),
// NO ellipse / circle (mock "ring" = drawOval) is painted by the Delta Profile: coloured same-candle A, next-candle live A, gray big node,
// shared A ring (+ its letter); decisions, records and restart behaviour unchanged; versions match.
// Build (from tests_dp217/): g++ -std=c++17 -O1 -w -fsanitize=address,undefined -I. -I../tests_delta -I.. test_dp232.cpp -o /tmp/t_dp232 && /tmp/t_dp232
#include "../DeltaProfile.cpp"
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
namespace mock { Host* current = NULL; }
static int passed = 0, failed = 0;
static void check(bool ok, const std::string& why) { if (ok) ++passed; else { ++failed; std::cerr << "FAIL " << why << '\n'; } }
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static void rmrf(const std::string& d) { std::string c = "rm -rf '" + d + "'"; if (std::system(c.c_str()) != 0) {} }
static std::string flex;
static std::string home(const std::string& name) {
    const std::string root = "/tmp/dp232-run", h = root + "/" + name; rmrf(h);
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1);
    flex = h + "/InvestorRT/rtx/lsFlexLevels/";
    return flex + "DeltaProfile/";
}

// t_dlt_perf.cpp -- the Delta Profile's speed harness (2026-11-11, Rassul "go" on the Delta Profile speed audit): PERF MODE for
// tests_perf/check_draw_io.py. A day of 3-minute candles on a random walk with a volume-at-price row per tick of each candle's range; the scene
// is drawn N times the way IRT does it: 4 draws a second, the profile rebuilt once a second (builtAt), the status due by its own timer.
#include <chrono>
static ChartState& state(mock::Host& h) { return *static_cast<ChartState*>(h.data); }
static mock::Host day(int n) {
    mock::Host h; h.seconds = 180; h.root = "ES"; h.tick = 0.25f; h.period = "3 min"; unsigned s = 12345; auto rnd = [&]() { s = s * 1103515245u + 12345u; return (s >> 16) & 0x7FFF; };
    double px = 7500; const unsigned long T0 = 1791820800UL + 18 * 3600;
    for (int i = 0; i < n; ++i) { mock::Bar b; b.time = T0 + (unsigned long)i * 180; const double o = px; px += ((int)(rnd() % 17) - 8) * 0.25; const double c = px;
        const double hi = std::max(o, c) + (rnd() % 6) * 0.25, lo = std::min(o, c) - (rnd() % 6) * 0.25; b.open = (float)o; b.close = (float)c; b.high = (float)hi; b.low = (float)lo; b.rows.clear(); b.volume = 0;
        for (double p = lo; p <= hi + 1e-9; p += 0.25) { const long bu = 20 + (long)(rnd() % 200), se = 20 + (long)(rnd() % 200); b.rows.push_back(mock::Price((float)p, bu, se, bu + se)); b.volume += (unsigned long)(bu + se); }
        h.bars.push_back(b); }
    return h; }
static int parity() {   // PARITY=1: a growing chart (the forming candle changes, candles close, 3 full re-reads forced), every draw's output hashed
    home("parity"); mock::Host full = day(900); mock::Host h = full; h.bars.resize(600); DeltaProfile p; unsigned long long hash = 1469598103934665603ULL; size_t nd = 0;
    auto mix = [&](const std::string& x) { for (unsigned char c : x) { hash ^= c; hash *= 1099511628211ULL; } };
    unsigned sd = 777; auto rnd = [&]() { sd = sd * 1103515245u + 12345u; return (sd >> 16) & 0x7FFF; };
    for (int step = 0; step < 600; ++step) {
        if (step % 2 == 1 && h.bars.size() < full.bars.size()) h.bars.push_back(full.bars[h.bars.size()]);
        else { mock::Bar& b = h.bars.back(); if (rnd() % 2) { b.high += 0.25f; b.rows.push_back(mock::Price(b.high, 30, 10, 40)); b.volume += 40; } else { b.low -= 0.25f; b.rows.push_back(mock::Price(b.low, 10, 30, 40)); b.volume += 40; } b.close = (rnd() % 2) ? b.high : b.low; }
        h.draws.clear(); if (h.data) { state(h).builtAt -= 1001; if (step % 200 == 199) state(h).zFullAt = -1; }
        mock::current = &h; p.draw(); nd += h.draws.size();
        for (const mock::Draw& d : h.draws) mix(d.type + "|" + d.text + "|" + std::to_string(d.x) + "|" + std::to_string(d.y) + "|" + std::to_string((unsigned long)d.color) + ";"); }
    std::string st = slurp(flex + "DeltaProfile.status-ES-180.txt"), keep; { std::istringstream in(st); std::string l; while (std::getline(in, l)) if (l.compare(0, 8, "VERSION,") != 0 && l.compare(0, 6, "SPEED,") != 0) keep += l + "\n"; }
    FILE* ls = popen(("cat " + flex + "DeltaProfile/*-signals-*.csv 2>/dev/null | sed 's/2\\.3[56]\\.0/V/g'").c_str(), "r"); std::string rec; char buf[4096]; size_t n; while (ls && (n = fread(buf, 1, sizeof buf, ls)) > 0) rec.append(buf, n); if (ls) pclose(ls);
    mix(rec); std::printf("PARITY draws %zu status %zu bytes record %zu bytes hash %016llx (full re-reads %lld, partial %lld)\n", nd, keep.size(), rec.size(), hash, h.data ? state(h).zFull : -1LL, h.data ? state(h).zPart : -1LL);
    mock::current = &h; p.destroy(); latchBooks().clear(); return 0; }
int main() {
    if (getenv("PARITY")) return parity();
    (void)passed; (void)failed; (void)check; (void)slurp;
    const int N = getenv("PERF_DRAWS") ? atoi(getenv("PERF_DRAWS")) : 200;
    for (int big = 0; big < 2; ++big) { home(big ? "perfB" : "perfA"); mock::Host h = day(big ? 6580 : 480); DeltaProfile p; mock::current = &h; p.draw(); p.draw();
        size_t ops = 0; double msAll = 0, msRebuild = 0, msPlain = 0; int nR = 0, nP = 0; long rcR = 0, rcP = 0, scR = 0, scP = 0;
        for (int k = 0; k < N; ++k) { h.draws.clear(); const bool rebuild = k % 4 == 0; if (rebuild && h.data) state(h).builtAt -= 1001;
            mock::current = &h; const int rc0 = h.rowCalls, sc0 = h.statsCalls; const auto t0 = std::chrono::steady_clock::now(); p.draw(); (rebuild ? rcR : rcP) += h.rowCalls - rc0; (rebuild ? scR : scP) += h.statsCalls - sc0; const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            msAll += ms; if (rebuild) { msRebuild += ms; ++nR; } else { msPlain += ms; ++nP; } ops += h.draws.size(); }
        std::printf("PERF %-36s candles %5zu  %.3f ms/draw  %zu draw calls/draw  (rebuild %.3f ms x %d, plain %.3f ms x %d; SDK row calls %d, stats calls %d)\n", big ? "6,580 candles, 4 draws / s" : "480 candles, 4 draws / s",
            h.bars.size(), msAll / N, ops / (size_t)(N ? N : 1), nR ? msRebuild / nR : 0, nR, nP ? msPlain / nP : 0, nP, h.rowCalls, h.statsCalls);
        std::printf("  SDK calls per draw: rebuild rows %.1f stats %.1f | plain rows %.1f stats %.1f\n", nR ? (double)rcR / nR : 0, nR ? (double)scR / nR : 0, nP ? (double)rcP / nP : 0, nP ? (double)scP / nP : 0);
        mock::current = &h; p.destroy(); latchBooks().clear(); }
    return 0; }
