// test_hook.cpp -- the smart-read hook (AwareHook.h) and the whole loop with lsContext's pump: facts(t) -> CTX(t) -> AW(t) -> TEAM(t).
// g++ -std=c++17 -Wall -Wextra -DHB_FAKE -I. -I<plugin> test_hook.cpp && ./a.out
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include "AwareHook.h"
#include "ContextPump.h"

static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static int lines(const std::string& s) { int n = 0; for (char c : s) n += c == '\n'; return n; }

int main() {
    const char* fd = std::getenv("LRA_FLEX_DIR"); const std::string D = fd ? fd : "/tmp/lra_flex_test"; mkdir(D.c_str(), 0755);
    const long long t0 = cp::barId(2026, 10, 26, 9, 0, 0);
    std::vector<ctx::Bar> B; double px = 5000; for (int i = 0; i < 40; ++i) { ctx::Bar b; b.t = t0 + i * 180; b.o = px; px += (i % 10 < 5 ? 2 : -2); b.c = px; b.h = std::max(b.o, b.c) + 1; b.l = std::min(b.o, b.c) - 1; b.minOfDay = 540 + 3 * i; B.push_back(b); }
    const long long t = B.back().t;
    CHECK(awh::dayOf(t) == "20261026", "day of a bar id");
    CHECK(awh::dayOf(cp::barId(2024, 2, 29, 23, 59, 59)) == "20240229", "leap day");

    // reads
    { auto r = awh::stageRead(2, true, 1); CHECK(r.side == 1, "stage 2 low pivot -> up"); r = awh::stageRead(1, true, 1); CHECK(r.side == 0, "stage 1 -> no side");
      r = awh::stageRead(3, false, 2); CHECK(r.side == 0, "failed attempt -> no side"); r = awh::stageRead(3, false, 1); CHECK(r.side == -1, "high pivot -> down"); }
    { std::map<std::string, std::string> kv{ {"a", "final"}, {"a_side", "bear"}, {"a_x", "3.1"}, {"a_px", "5010.25"} };
      CHECK(awh::dpRead(kv).side == -1, "final bear A -> down"); kv["a"] = "tentative"; CHECK(awh::dpRead(kv).side == 0, "tentative A -> no side");
      CHECK(awh::dpRead({}).side == 0 && awh::dpRead({}).word == "no A", "no facts -> no A"); }

    // the loop: the indicators post facts(t), lsContext builds CTX(t), the hooks decide AW(t), lsContext builds TEAM(t)
    long long now = 1000000;
    cp::Pump P; P.sym = "ESZ26"; P.spb = 180; P.chart = "c1"; P.market = "ES"; P.bars = B;
    awh::Hook hs; hs.src = "STAGE"; hs.dir = D; awh::Hook hd; hd.src = "DP"; hd.dir = D;
    mst::publish("DP", "ESZ26", 180, "c1", now, t, { {"a", "final"}, {"a_side", "bull"} });
    mst::publish("VWAP", "ESZ26", 180, "c1", now, t, { {"pos", "below2"} });
    CHECK(!hd.tick("ESZ26", 180, "c1", "ES", now, t, awh::dpRead({ {"a", "final"}, {"a_side", "bull"} })), "hook waits for CTX(t)");
    cp::Out o = P.step(now + 5); CHECK(!o.ctxLine.empty(), "CTX(t) built");
    CHECK(o.teamLine.empty(), "TEAM never decided before any read could arrive (no reads posting -> wait the timeout)");
    CHECK(hd.tick("ESZ26", 180, "c1", "ES", now + 10, t, awh::dpRead({ {"a", "final"}, {"a_side", "bull"} })), "DP hook decides once CTX(t) is OK");
    CHECK(hd.A.ctx == "ok" && hd.A.aware.side == 1 && hd.A.isolated.side == 1, "aware = isolated (links OFF), ctx ok");
    CHECK(!hd.tick("ESZ26", 180, "c1", "ES", now + 20, t, awh::dpRead({ {"a", "final"}, {"a_side", "bear"} })), "decided once");
    CHECK(hd.A.aware.side == 1, "a later different read for t never changes it");
    o = P.step(now + 30); CHECK(!o.teamLine.empty(), "TEAM decided: every read that is posting (DP) is at t");
    { mst::Facts tm = mst::read("TEAM", "ESZ26", 180, "c1", now + 30); CHECK(tm.ok && mst::getS(tm, "side") == "1" && mst::getS(tm, "n") == "1", "TEAM up from DP"); }
    // stage hook arrives late: missing CTX? no - CTX(t) is OK, it decides immediately
    CHECK(hs.tick("ESZ26", 180, "c1", "ES", now + 40, t, awh::stageRead(2, true, 1)), "STAGE hook decides");
    const std::string rec = slurp(D + "/Context-AWR-ES-180s-20261026.rec");
    CHECK(lines(rec) >= 2 && rec.find("AWR|2|DP|" + std::to_string(t) + "|ok|-|-|1|0|1|0|") != std::string::npos, "AWR record line for DP");
    CHECK(rec.find("AWR|2|STAGE|") != std::string::npos, "AWR record line for STAGE");

    // a hook whose CTX never comes: MISSING after the timeout, aware = isolated, recorded as missing
    { mst::clear("CTX", "ESZ26", 180, "c2"); awh::Hook h; h.src = "AGG"; h.dir = D;
      CHECK(!h.tick("ESZ26", 180, "c2", "ES", 2000000, t, awh::factsRead("spike")), "waits");
      CHECK(h.tick("ESZ26", 180, "c2", "ES", 2003000, t, awh::factsRead("spike")) && h.A.ctx == "missing", "missing after the timeout");
      mst::Facts a = mst::read("AWAGG", "ESZ26", 180, "c2", 2003000); CHECK(a.ok && a.bar == t && mst::getS(a, "ctx") == "missing", "AWAGG posted with ctx=missing"); }
    // a CTX for another bar is never used
    { mst::publish("CTX", "ESZ26", 180, "c3", 3000000, t - 180, { {"v", "0.1.0"} }); awh::Hook h; h.src = "HODLOD"; h.dir = D;
      CHECK(!h.tick("ESZ26", 180, "c3", "ES", 3000000, t, awh::factsRead("hod")), "a CTX of t-1 is not CTX(t)"); }
    // re-post every 15 s
    { awh::Hook h = hd; long long before = h.postMs; h.tick("ESZ26", 180, "c1", "ES", now + 10 + 14000, t, awh::dpRead({})); CHECK(h.postMs == before, "no re-post before 15 s");
      h.tick("ESZ26", 180, "c1", "ES", now + 10 + 15000, t, awh::dpRead({})); CHECK(h.postMs == now + 15010, "re-posted after 15 s"); }
    // new bar -> a new decision
    CHECK(!hd.tick("ESZ26", 180, "c1", "ES", now + 200000, t + 180, awh::dpRead({})) && !hd.decided, "new bar waits again");
    hd.clear("ESZ26", 180, "c1"); CHECK(!mst::read("AWDP", "ESZ26", 180, "c1", now + 200000).ok, "cleared");

    std::printf("hook tests: %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
