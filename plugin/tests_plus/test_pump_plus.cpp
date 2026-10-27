// test_pump_plus.cpp -- lsContext's pump AFTER apply_plus.py (ContextPump.h 0.3.0) on the fake board: graceful failure reaches CTX / TEAM,
// METRIC fields in CTX, expectations registered + resolved through the pump, PLUS off = the 0.2.0 behaviour.
// run_tests.sh patches a copy of smart_indicators' ContextPump.h first and puts it first on the include path.
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "ContextPump.h"

static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

static std::vector<ctx::Bar> mkBars(int n, long long t0) {
    std::vector<ctx::Bar> b; double px = 5000;
    for (int i = 0; i < n; ++i) { ctx::Bar x; x.t = t0 + i * 180; x.o = px; px += (i % 20 < 10 ? 2.0 : -1.5); x.c = px; x.h = std::max(x.o, x.c) + 0.75; x.l = std::min(x.o, x.c) - 0.75; x.v = 1000 + i; x.minOfDay = (int)((x.t % 86400) / 60); b.push_back(x); }
    return b;
}
static void post(const char* src, long long bar, long long now, std::map<std::string, std::string> kv) { mst::publish(src, "ESZ26", 180, "c1", now, bar, kv); }
static void wipe() { const char* all[] = { "AGG", "BOOK", "LOTS", "DP", "LIQ", "GEX", "HODLOD", "COB", "VWAP", "TREND", "READ", "STAGE", "TREND1H", "CTX", "TEAM", "AWSTAGE", "AWDP", "AWAGG", "AWHODLOD", "EXP", "NEWS", "HEALTH", nullptr };
                         for (const char** s = all; *s; ++s) mst::clear(*s, "ESZ26", 180, "c1"); }
static cp::Pump mkPump(const std::vector<ctx::Bar>& b) { cp::Pump p; p.sym = "ESZ26"; p.spb = 180; p.chart = "c1"; p.market = "ES"; p.bars = b; return p; }
static const char* PROF = "VERSION,1\nMARKET,ES\nP,aggr_p99,27\nKN,absorb,all,3,1\nNEWS,wd2,0830,TEST_DATA,5,15\nSRC,LIQ,1,60\n";

int main() {
    const long long t0 = cp::barId(2026, 10, 27, 7, 33, 0);            // a Tuesday; the test release is 08:30 CT
    auto B = mkBars(60, t0);
    std::vector<ctx::Bar> b1(B.begin(), B.begin() + 20); const long long t = b1.back().t;   // 08:30 bar end
    CHECK(cp::VERSION == std::string("0.3.0"), "pump version bumped");

    // 1. a stale source is a team fact, left out of CTX(t); missing expected LIQ listed; CTX carries the METRIC fields
    wipe(); long long now = 9000000;
    post("VWAP", t, now, { {"vwap", "5010"}, {"pos", "above1"} }); post("TREND", t, now, { {"trend", "1"} });
    post("DP", t, now, { {"a", "final"} });
    post("BOOK", t, now - 70000, { {"x", "1"} });                         // written 70 s ago: stale (mst::read drops it too)
    post("AGG", t - 180 * 6, now, { {"x", "1"} });                         // fresh writes but 6 bars behind: lagging
    cp::Pump P = mkPump(b1); P.plus.load("ES", PROF, 0.25); P.plus.reader = "ok"; P.plus.mbo = "on";
    cp::Out o = P.step(now);
    CHECK(!o.ctxLine.empty(), "CTX built (the lagging AGG does not block the wait)");
    CHECK(P.ctxKv["M.src_state"] == "degraded" && P.ctxKv["M.src_bad"].find("AGG:lagging") != std::string::npos && P.ctxKv["M.src_bad"].find("LIQ:missing") != std::string::npos && P.ctxKv["M.src_bad"].find("BOOK:stale") != std::string::npos, "CTX names the bad sources");
    CHECK(P.ctxKv["M.news"] == "1" && P.ctxKv["M.news_window"] == "in" && P.ctxKv["M.news_event"] == "TEST_DATA" && P.ctxKv["M.conf_x"] == "0.50", "news window in CTX");
    CHECK(P.ctxKv["D.mkt_aggr_p99"] == "27" && P.ctxKv["D.mkt_profile"] == "file", "personality in CTX");
    CHECK(o.ctxLine.find("M.news=1") != std::string::npos, "recorded");
    bool aggFact = false; for (const auto& kv : P.ctxKv) if (kv.first.find("big_aggr") != std::string::npos && kv.second != "?") aggFact = true;
    CHECK(!aggFact, "the lagging source's facts are not used");
    CHECK(std::any_of(o.plusLines.begin(), o.plusLines.end(), [](const std::string& l) { return l.rfind("HEALTH|1|", 0) == 0; }), "HEALTH record line");

    // 2. TEAM: the excluded teammate's read is dropped; conf_x / news carried; expectations registered from the reads at t
    post("AWDP", t, now + 100, { {"side", "1"}, {"conf", "0"}, {"word", "final A"} });
    post("AWAGG", t, now + 100, { {"side", "-1"}, {"conf", "0"}, {"word", "aggression"} });   // AGG is lagging -> excluded
    post("AWSTAGE", t, now + 100, { {"side", "1"}, {"conf", "0"}, {"word", "stage 3"} });
    post("AWHODLOD", t, now + 100, { {"side", "0"}, {"word", "hod / lod"} });
    o = P.step(now + 200);
    CHECK(!o.teamLine.empty() && P.teamKv["side"] == "1" && P.teamKv["conflict"] == "0", "AGG's opposite read dropped -> no conflict");
    CHECK(P.teamKv["news"] == "1" && P.teamKv["conf_x"] == "0.50" && P.teamKv["excluded"].find("AGG") != std::string::npos, "TEAM carries the gate");
    int regs = 0; for (auto& l : o.plusLines) if (l.rfind("EXPREG|1|", 0) == 0) ++regs;
    CHECK(regs == 2 && P.plus.book.open.size() == 2, "DP absorb + STAGE stage3 registered");
    CHECK(mst::read("EXP", "ESZ26", 180, "c1", now + 300).ok && mst::read("HEALTH", "ESZ26", 180, "c1", now + 300).ok, "EXP / HEALTH on the board");

    // 3. the next bars resolve them through the pump (record lines EXP|1|)
    int verdicts = 0; long long nw = now + 1000;
    for (size_t i = 21; i <= 32; ++i) {
        P.bars.assign(B.begin(), B.begin() + i); const long long ti = P.bars.back().t; nw += 4000;
        post("VWAP", ti, nw, { {"vwap", "5010"}, {"pos", "above1"} }); post("TREND", ti, nw, { {"trend", "1"} }); post("DP", ti, nw, { {"a", "none"} });
        for (long long dt : { 0LL, 3500LL }) { cp::Out oi = P.step(nw + dt); for (auto& l : oi.plusLines) if (l.rfind("EXP|1|", 0) == 0) ++verdicts; }
    }
    CHECK(verdicts == 2 && P.plus.book.open.empty(), "both resolved within their deadlines");

    // 4. PLUS off = the 0.2.0 pump: no plus fields, no plus lines, nothing excluded
    wipe(); now = 20000000;
    post("VWAP", t, now, { {"vwap", "5010"}, {"pos", "above1"} }); post("TREND", t, now, { {"trend", "1"} }); post("AGG", t - 180 * 6, now, { {"x", "1"} });
    cp::Pump Q = mkPump(b1); Q.plus.enabled = false;
    o = Q.step(now); CHECK(o.ctxLine.empty(), "PLUS off: the lagging AGG is waited for, as in 0.2.0");
    o = Q.step(now + 3100);
    bool anyPlus = false; for (const auto& kv : Q.ctxKv) if (kv.first.rfind("M.news", 0) == 0 || kv.first.rfind("M.src", 0) == 0 || kv.first.rfind("D.mkt", 0) == 0) anyPlus = true;
    CHECK(!o.ctxLine.empty() && !anyPlus && o.plusLines.empty(), "PLUS off: CTX as before");
    CHECK(!mst::read("HEALTH", "ESZ26", 180, "c1", now + 1).ok, "PLUS off: nothing posted");
    // 5. clear() wipes the plus sources too
    P.clear(); CHECK(!mst::read("EXP", "ESZ26", 180, "c1", nw + 1).ok && !mst::read("NEWS", "ESZ26", 180, "c1", nw + 1).ok, "clear wipes EXP / NEWS / HEALTH");
    CHECK(ap::plusOn("VERSION,1\nPLUS,0\n") == false && ap::plusOn("LINK,L1,0\n") == true, "PLUS switch");
    std::printf("test_pump_plus: %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
