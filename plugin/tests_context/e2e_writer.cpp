// e2e_writer.cpp -- writes REAL v2 records with the plugin code (ContextPump.h + AwareHook.h on the HB_FAKE board), exactly as
// lsContext (CTX / TEAM lines -> Context-<MKT>-<spb>s-<day>.rec) and the hooks (AWR v2 -> Context-AWR-...) write them in IRT.
// The nightly readers (lra.context_links, lra.team_health) are tested on this output end to end (test_e2e_v2.py).
// g++ -std=c++17 -DHB_FAKE -I. -I<plugin> e2e_writer.cpp -o e2e && LRA_FLEX_DIR=<dir> ./e2e
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "AwareHook.h"
#include "ContextPump.h"

int main() {
    const char* fd = std::getenv("LRA_FLEX_DIR"); if (!fd) { std::printf("set LRA_FLEX_DIR\n"); return 2; }
    const std::string D = fd, sym = "ESZ26", chart = "c1"; const long spb = 180;
    const long long t0 = cp::barId(2026, 10, 26, 8, 30, 0);
    cp::Pump P; P.sym = sym; P.spb = spb; P.chart = chart; P.market = "ES"; P.rthOpenMin = 510;
    awh::Hook hd; hd.src = "DP"; hd.dir = D; awh::Hook hs; hs.src = "STAGE"; hs.dir = D; awh::Hook ha; ha.src = "AGG"; ha.dir = D;
    std::vector<ctx::Bar> all; double px = 6800;
    for (int i = 0; i < 130; ++i) {                     // up 60 bars, down 40, up 30: turns to score
        const double step = i < 60 ? 0.75 : i < 100 ? -1.0 : 0.75;
        ctx::Bar b; b.t = t0 + 180LL * (i + 1); b.o = px; px += step + 0.25 * std::sin(i * 1.7); b.c = px;
        b.h = std::max(b.o, b.c) + 0.5; b.l = std::min(b.o, b.c) - 0.5; b.v = 1000; b.minOfDay = 510 + 3 * (i + 1); all.push_back(b);
    }
    std::ofstream rec((D + "/Context-ES-180s-" + awh::dayOf(all[0].t) + ".rec").c_str(), std::ios::app | std::ios::binary);
    long long now = 1000000;
    for (size_t i = 0; i < all.size(); ++i) {
        const ctx::Bar& b = all[i]; const long long t = b.t; now += 180000;
        P.bars.assign(all.begin(), all.begin() + (long)i + 1);
        // L1: the publishers post their facts for t
        mst::publish("VWAP", sym, spb, chart, now, t, { {"pos", b.c > 6800 ? "above1" : "below1"}, {"slope", "0.1"} });
        mst::publish("TREND", sym, spb, chart, now, t, { {"trend", i < 60 ? "1" : i < 100 ? "-1" : "1"}, {"trend_word", "?"} });
        const bool turnHi = i == 59, turnLo = i == 99;
        mst::publish("DP", sym, spb, chart, now, t, { {"a", (turnHi || turnLo || i % 7 == 0) ? "final" : "none"}, {"a_side", turnHi ? "bear" : turnLo ? "bull" : (i % 2 ? "bull" : "bear")},
                                                       {"a_x", "2.4"}, {"a_px", mst::num(b.c, 10)} });
        mst::publish("AGG", sym, spb, chart, now, t, { {"spike", "1.3"}, {"sweep", "2"}, {"pace_x", "1.1"} });
        // the hooks' first pass (CTX not built yet -> they wait), then lsContext (CALC_LAST), then the hooks again, then lsContext again
        const auto dpR = awh::dpRead({ {"a", (turnHi || turnLo || i % 7 == 0) ? "final" : "none"}, {"a_side", turnHi ? "bear" : turnLo ? "bull" : (i % 2 ? "bull" : "bear")}, {"a_x", "2.4"} });
        const auto stR = awh::stageRead(i % 5 == 0 ? 2 : 1, i % 10 < 5, 1);
        hd.tick(sym, spb, chart, "ES", now, t, dpR); hs.tick(sym, spb, chart, "ES", now, t, stR); ha.tick(sym, spb, chart, "ES", now, t, awh::factsRead("aggression"));
        cp::Out o = P.step(now + 50);
        if (!o.ctxLine.empty()) rec << o.ctxLine << "\n";
        hd.tick(sym, spb, chart, "ES", now + 300, t, dpR); hs.tick(sym, spb, chart, "ES", now + 300, t, stR); ha.tick(sym, spb, chart, "ES", now + 300, t, awh::factsRead("aggression"));
        o = P.step(now + 400);
        if (!o.ctxLine.empty()) rec << o.ctxLine << "\n";
        if (!o.teamLine.empty()) rec << o.teamLine << "\n";
        o = P.step(now + 4000);                          // the timeout pass (TEAM if a read never came)
        if (!o.teamLine.empty()) rec << o.teamLine << "\n";
    }
    std::printf("wrote %s\n", D.c_str());
    return 0;
}
