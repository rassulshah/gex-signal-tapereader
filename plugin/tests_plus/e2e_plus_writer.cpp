// e2e_plus_writer.cpp -- writes REAL lsContext 0.2.0 records (patched ContextPump.h 0.3.0 + AwarePlus.h) for the nightly tests:
// 3 sessions x 130 ES 3-min bars, teammates posting facts + aware reads, a release window (07:30 CPI on day 2), a shock bar, a source
// going stale mid-session and coming back. Usage: ./e2e_plus_writer <out dir>  -> Context-ES-180s-<day>.rec + LRA-MarketProfile-ES.csv
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include "ContextPump.h"

static const char* PROF = "VERSION,1\nMARKET,ES\nP,tick,0.25\nP,aggr_p99,27\nP,shock_rng_x,3\nP,shock_vol_x,2.5\nP,news_conf_x,0.5\n"
                          "SLOT,450,8,20,1000,3000\nSLOT,480,8,20,1000,3000\nSLOT,510,8,20,1000,3000\nSLOT,540,8,20,1000,3000\nSLOT,570,8,20,1000,3000\n"
                          "SLOT,600,8,20,1000,3000\nSLOT,630,8,20,1000,3000\nSLOT,660,8,20,1000,3000\nSLOT,690,8,20,1000,3000\nSLOT,720,8,20,1000,3000\n"
                          "SLOT,750,8,20,1000,3000\nSLOT,780,8,20,1000,3000\nSLOT,810,8,20,1000,3000\nSLOT,840,8,20,1000,3000\n"
                          "KN,absorb,all,3,1\nKN,collision,all,5,1\nKN,stage3,all,5,0\nKN,confirm,all,3,1\nNEWS,2026-10-21,0730,CPI,5,15\nSRC,VWAP,1,60\nSRC,TREND,1,60\n";

static unsigned long long rng = 88172645463325252ULL;
static double rnd() { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (double)(rng % 1000000) / 1000000.0; }

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : ".";
    { std::ofstream f(out + "/LRA-MarketProfile-ES.csv"); f << PROF; }
    const int days[3] = { 20, 21, 22 };
    for (int di = 0; di < 3; ++di) {
        const int d = days[di];
        const char* all[] = { "AGG", "BOOK", "LOTS", "DP", "LIQ", "GEX", "HODLOD", "COB", "VWAP", "TREND", "READ", "STAGE", "TREND1H", "CTX", "TEAM", "AWSTAGE", "AWDP", "AWAGG", "AWHODLOD", "EXP", "NEWS", "HEALTH", nullptr };
        for (const char** s = all; *s; ++s) mst::clear(*s, "ESZ26", 180, "c1");
        cp::Pump P; P.sym = "ESZ26"; P.spb = 180; P.chart = "c1"; P.market = "ES"; P.tick = 0.25; P.rthOpenMin = 510;
        P.plus.load("ES", PROF, 0.25); P.plus.reader = "ok"; P.plus.mbo = "on";
        char fn[256]; std::snprintf(fn, sizeof(fn), "%s/Context-ES-180s-202610%02d.rec", out.c_str(), d);
        std::ofstream rec(fn);
        const long long t0 = cp::barId(2026, 10, d, 6, 3, 0);
        double px = 5800 + di * 20; long long now = 1000000000LL * (di + 1);
        std::vector<ctx::Bar> bars;
        for (int i = 0; i < 130; ++i) {
            ctx::Bar b; b.t = t0 + i * 180; b.o = px;
            const double drift = std::sin(i / 9.0) * 1.5 + (rnd() - 0.5) * 3.0;
            double c = px + drift; c = std::round(c * 4) / 4;
            b.c = c; b.h = std::max(b.o, b.c) + std::round(rnd() * 6) / 4; b.l = std::min(b.o, b.c) - std::round(rnd() * 6) / 4; b.v = 800 + rnd() * 500;
            if (di == 1 && i == 60) { b.h += 6; b.l -= 6; b.v = 4000; }          // a shock bar (range + volume far above the slot normal)
            b.minOfDay = (int)((b.t % 86400) / 60); px = c; bars.push_back(b);
            if (i < 25) continue;                                              // history first (ATR, legs)
            P.bars = bars; const long long t = b.t; now += 180000;
            mst::publish("VWAP", "ESZ26", 180, "c1", now, t, { {"vwap", "5800"}, {"pos", "above1"} });
            mst::publish("TREND", "ESZ26", 180, "c1", now, t, { {"trend", rnd() < 0.5 ? "1" : "-1"} });
            const bool dpStale = di == 2 && i >= 60 && i < 70;                 // DP stops posting for 10 bars on day 3
            if (!dpStale) mst::publish("DP", "ESZ26", 180, "c1", now, t, { {"a", "final"}, {"a_side", rnd() < 0.5 ? "bull" : "bear"} });
            // live hooks re-post their last aware read every 15 s: refresh the previous bar's reads before the new bar's step
            for (const char* aw : { "AWDP", "AWSTAGE" }) { mst::Facts f = mst::read(aw, "ESZ26", 180, "c1", -1); if (f.ok && !(dpStale && std::string(aw) == "AWDP")) mst::publish(aw, "ESZ26", 180, "c1", now - 5000, f.bar, f.kv); }
            std::vector<std::string> lines;
            cp::Out o = P.step(now);
            auto take = [&](const cp::Out& x) { if (!x.ctxLine.empty()) lines.push_back(x.ctxLine); if (!x.teamLine.empty()) lines.push_back(x.teamLine); for (auto& l : x.plusLines) lines.push_back(l); };
            take(o);
            // aware reads for t
            // every teammate's hook posts an aware read for every bar (side 0 = no read), as AwareHook does live
            if (!dpStale) { const bool a = rnd() < 0.5; mst::publish("AWDP", "ESZ26", 180, "c1", now + 50, t, { {"side", a ? (rnd() < 0.5 ? "1" : "-1") : "0"}, {"conf", "0"}, {"word", a ? "final A" : "no A"} }); }
            { const bool a = rnd() < 0.4; const int st = a ? 2 + (int)(rnd() * 3) : 1;
              mst::publish("AWSTAGE", "ESZ26", 180, "c1", now + 50, t, { {"side", a ? (rnd() < 0.5 ? "1" : "-1") : "0"}, {"conf", "0"}, {"word", "stage " + std::to_string(st)} }); }
            take(P.step(now + 100)); take(P.step(now + 3200));
            for (auto& l : lines) rec << l << "\n";
        }
    }
    std::printf("e2e_plus_writer: wrote 3 sessions to %s\n", out.c_str());
    return 0;
}
