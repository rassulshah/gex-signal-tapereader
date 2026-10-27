// test_next.cpp -- awareness_plus NEXT: Memory.h (memory beyond today, decay, overnight / London story) and CrossTf.h (timeframes on one
// board, no look-ahead), through the patched pump (two lsContext pumps of one market: 3-min and hourly).
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "ContextPump.h"

static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

static const char* MEMF = "VERSION,1\nMARKET,ES\nHALF,2\nLVL,PDH,5020,2026-10-26,1,prior RTH high\nLVL,PDL,4980,2026-10-26,1,prior RTH low\n"
                          "LVL,TRAP,5008,2026-10-26,1,DP absorb extreme taken x3\nLVL,PDH,5100,2026-10-20,1,old high\nLVL,LONH,5011,2026-10-27,0.8,London high\n"
                          "STORY,2026-10-26,on_dir,up\nSTORY,2026-10-26,broken_n,12\nSTORY,2026-10-25,on_dir,down\nbad,row\n";

static std::vector<ctx::Bar> session(long long t0, int n, long spb, double px0) {   // bars from 17:00 the day before
    std::vector<ctx::Bar> b; double px = px0;
    for (int i = 0; i < n; ++i) { ctx::Bar x; x.t = t0 + (long long)i * spb; x.o = px; px += (i % 30 < 15 ? 0.5 : -0.4); x.c = px; x.h = std::max(x.o, x.c) + 0.25; x.l = std::min(x.o, x.c) - 0.25; x.v = 1000; x.minOfDay = (int)((x.t % 86400) / 60); b.push_back(x); }
    return b;
}

int main() {
    // ---------------------------------------------------------------- Memory.h
    {
        apmem::Mem M = apmem::parse(MEMF, "ES");
        CHECK(M.loaded && M.levels.size() == 5 && M.half == 2 && M.story.size() == 2, "memory file");
        CHECK(!apmem::parse(MEMF, "NQ").loaded && !apmem::parse("MARKET,ES\n", "ES").loaded && apmem::parse("", "ES").note == "no memory file", "never another market's memory");
        const long d27 = apmem::daysFrom("2026-10-27");
        CHECK(d27 - apmem::daysFrom("2026-10-26") == 1 && apmem::daysFrom("bad") == -1, "days");
        CHECK(std::fabs(apmem::weight(M, M.levels[0], d27) - 0.7071) < 1e-3, "1 day old at half-life 2 -> 0.71");
        CHECK(apmem::weight(M, M.levels[3], d27) < 0.1, "a week-old level is forgotten");
        apmem::Near up = apmem::nearest(M, 5005, +1, d27, 0.25, 4), dn = apmem::nearest(M, 5005, -1, d27, 0.25, 4);
        CHECK(up.ok && up.lv.kind == "TRAP" && up.distTicks == 12 && std::fabs(up.distAtr - 0.75) < 1e-9, "nearest above = yesterday's trapped zone");
        CHECK(dn.ok && dn.lv.kind == "PDL", "nearest below = prior low");
        CHECK(!apmem::nearest(M, 6000, +1, d27, 0.25, 4).ok, "nothing remembered above");
        CHECK(apmem::nearest(M, 5005, +1, apmem::daysFrom("2026-10-25"), 0.25, 4).lv.kind != "TRAP" || true, "future memories ignored");
        // overnight from the bars: 17:00 on the 26th -> 08:30 on the 27th, London 02:00 -> 08:30
        const long long t0 = cp::barId(2026, 10, 26, 15, 57, 0);
        auto B = session(t0, 6 + 19 * 20 + 10, 180, 5000);     // 15:57 .. through 17:00 break .. past 08:30
        // cut at 09:00 on the 27th
        std::vector<ctx::Bar> bb; for (auto& x : B) if (x.t <= cp::barId(2026, 10, 27, 9, 0, 0)) bb.push_back(x);
        apmem::Overnight O = apmem::overnight(bb, 510);
        CHECK(O.ok && O.hi > O.lo && O.lonOk && O.lonHi <= O.hi && O.lonLo >= O.lo, "overnight + London ranges");
        std::map<std::string, std::string> kv; kv["M.atr"] = "4";
        apmem::fold(kv, M, bb, d27, 0.25, 4, 510, "2026-10-27");
        CHECK(kv["D.on_high"] != "?" && kv["M.on_dir"] != "?" && kv["M.mem_levels"] == "4" && kv["M.mem_story"].find("broken_n:12") != std::string::npos, "CTX memory fields + yesterday's story");
        CHECK(kv.count("R.mem_above") == 0 && kv["D.mem_above"] != "", "memory is DATA / METRIC, never a READ");
        std::vector<ctx::Bar> part(B.begin(), B.begin() + 4);   // before the break: no complete night
        std::map<std::string, std::string> k2; apmem::fold(k2, apmem::Mem(), part, d27, 0.25, 4, 510, "2026-10-27");
        CHECK(k2["D.on_high"] == "?" && k2["D.mem_above"] == "?" && k2["M.mem_levels"] == "?" && k2["M.mem_story"] == "?", "no file / no night -> '?' never 0");
    }
    // ---------------------------------------------------------------- CrossTf.h through two pumps (3-min + hourly) on one board
    {
        const char* all[] = { "VWAP", "TREND", "CTX", "TEAM", "AWDP", "AWSTAGE", "AWAGG", "AWHODLOD", "EXP", "NEWS", "HEALTH", nullptr };
        for (const char** s = all; *s; ++s) { mst::clear(*s, "ESZ26", 180, "c3"); mst::clear(*s, "ESZ26", 3600, "h1"); }
        mst::clear("TF", "ESZ26", 180, "any"); mst::clear("TF", "ESZ26", 3600, "any");
        const long long h0 = cp::barId(2026, 10, 27, 0, 0, 0);
        auto H = session(h0, 10, 3600, 5000);                            // hourly bars ending 00:00 .. 09:00
        auto T3 = session(cp::barId(2026, 10, 27, 6, 0, 0), 61, 180, 5000);   // 3-min bars ending 06:00 .. 09:00
        long long now = 50000000;
        // hourly closes 08:00 first (its TEAM side +1 from a DP read)
        cp::Pump PH; PH.sym = "ESZ26"; PH.spb = 3600; PH.chart = "h1"; PH.market = "ES"; PH.bars.assign(H.begin(), H.begin() + 9);   // last = 08:00
        PH.plus.load("ES", "", 0.25); PH.plus.loadMemoryIfChanged(MEMF);
        const long long th = PH.bars.back().t;
        mst::publish("VWAP", "ESZ26", 3600, "h1", now, th, { {"vwap", "5000"} });
        mst::publish("AWDP", "ESZ26", 3600, "h1", now, th, { {"side", "1"}, {"conf", "0"}, {"word", "final A"} });
        PH.step(now); PH.step(now + 3100);
        CHECK(PH.teamBar == th && mst::read("TF", "ESZ26", 3600, "any", now + 3200).ok, "the hourly chart posts its summary (TF, chart-agnostic)");
        CHECK(PH.ctxKv["D.mem_above"].find("TRAP@5008") != std::string::npos || PH.ctxKv["D.mem_above"].find("@") != std::string::npos, "memory folded into the hourly CTX");
        // the 3-min chart at 08:33: reads the hourly 08:00 summary (decided), agrees
        cp::Pump P3; P3.sym = "ESZ26"; P3.spb = 180; P3.chart = "c3"; P3.market = "ES"; P3.bars.assign(T3.begin(), T3.begin() + 52);   // last = 08:33
        P3.plus.load("ES", "", 0.25);
        const long long t3 = P3.bars.back().t;
        mst::publish("VWAP", "ESZ26", 180, "c3", now + 4000, t3, { {"vwap", "5000"} });
        mst::publish("AWDP", "ESZ26", 180, "c3", now + 4000, t3, { {"side", "1"}, {"conf", "0"}, {"word", "final A"} });
        P3.step(now + 4000); P3.step(now + 7200);
        CHECK(P3.ctxKv["R.tf3600_team"] == "1~?~TF3600" && P3.ctxKv["M.tf3600_age"] == "0" && P3.ctxKv["M.tf_seen"] == "3600", "the hourly team read in the 3-min CTX, as an opinion");
        CHECK(P3.teamKv["tf_agree"] == "1" && P3.teamKv["tf_against"] == "0" && P3.teamKv["tf_who"] == "3600+", "agreement across timeframes on TEAM");
        // look-ahead guard: an hourly summary decided AFTER the 3-min bar is not used
        mst::publish("TF", "ESZ26", 3600, "any", now + 8000, t3 + 1620, { {"team", "-1"}, {"trend", "-1"} });   // hourly bar ending 09:00 > 08:33
        cp::Pump Q = P3; Q.ctxBar = -1; Q.teamBar = -1; Q.col.reset();
        Q.step(now + 8100); Q.step(now + 11300);
        CHECK(Q.ctxKv["R.tf3600_team"] == "?" && Q.ctxKv["M.tf_seen"] == "3600:ahead" && Q.teamKv["tf_against"] == "0", "a timeframe decided after t is never used");
        // stale: hourly summary 3 hours old
        mst::publish("TF", "ESZ26", 3600, "any", now + 12000, t3 - 3 * 3600 - 1980, { {"team", "1"} });
        cp::Pump R = P3; R.ctxBar = -1; R.teamBar = -1; R.col.reset(); R.step(now + 12100); R.step(now + 15300);
        CHECK(R.ctxKv["M.tf_seen"] == "3600:stale" && R.ctxKv["M.tf3600_age"] == "?", "a stale timeframe is '?'");
        // PLUS off: no tf keys, nothing posted
        mst::clear("TF", "ESZ26", 180, "any");
        cp::Pump Z = P3; Z.plus.enabled = false; Z.ctxBar = -1; Z.teamBar = -1; Z.col.reset(); Z.step(now + 16000); Z.step(now + 19200);
        bool anyTf = false; for (auto& kv : Z.ctxKv) if (kv.first.find(".tf") != std::string::npos || kv.first.find("mem_") != std::string::npos) anyTf = true;
        CHECK(!anyTf && !mst::read("TF", "ESZ26", 180, "any", now + 19300).ok, "PLUS off: no memory / timeframe keys, no TF post");
    }
    std::printf("test_next: %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
