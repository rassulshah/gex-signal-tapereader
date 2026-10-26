// test_context.cpp -- unit + mock-host tests for ContextLogic.h 0.1.0 and ContextRead.h 0.1.0 (Linux, HB_FAKE env board)
// g++ -std=c++17 -DHB_FAKE -I. -I<plugin> test_context.cpp -o t && ./t
#include <cassert>
#include <cstdio>
#include <random>
#include "ContextLogic.h"
#include "ContextRead.h"

static int fails = 0, passes = 0;
#define CHECK(c, msg) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s (line %d)\n", msg, __LINE__); } } while (0)

static std::vector<ctx::Bar> mkBars(int n) {
    std::vector<ctx::Bar> b; double p = 7000;
    for (int i = 0; i < n; ++i) {
        const double d = (i % 20 < 12) ? 2.0 : -2.5;           // staircase legs
        ctx::Bar x; x.t = 1000 + i * 180; x.o = p; x.c = p + d; x.h = std::max(x.o, x.c) + 0.5; x.l = std::min(x.o, x.c) - 0.5; x.v = 100 + i;
        x.minOfDay = 510 + i * 3; b.push_back(x); p = x.c;
    }
    return b;
}
static mst::Facts mkFacts(const std::string& src, long long bar, std::map<std::string, std::string> kv) {
    mst::Facts F; F.ok = true; F.src = src; F.bar = bar; F.ms = 1; F.kv = kv; return F;
}

int main() {
    // 1. collector: ready when all present sources decided t; timeout -> late list
    {
        ctx::Collector C; C.timeoutMs = 3000; std::vector<std::string> late;
        std::set<std::string> present = {"AGG", "DP", "LIQ"};
        std::map<std::string, long long> posted = {{"AGG", 100}, {"DP", 99}};
        CHECK(!C.ready(100, 0, present, posted, late), "waits for late sources");
        CHECK(late.size() == 2, "late lists DP and LIQ");
        CHECK(!C.ready(100, 2999, present, posted, late), "still waiting before timeout");
        CHECK(C.ready(100, 3000, present, posted, late) && late.size() == 2, "timeout -> build with late list");
        posted["DP"] = 100; posted["LIQ"] = 100;
        CHECK(C.ready(101 - 1, 3500, present, posted, late) && late.empty(), "all present decided -> ready, no late");
        // absent source (not present by heartbeat) never blocks
        ctx::Collector C2; std::map<std::string, long long> p2 = {{"AGG", 200}};
        CHECK(C2.ready(200, 0, {"AGG"}, p2, late), "absent sources never block");
    }
    // 2. no look-ahead: facts for bar > t and bars after t are ignored
    {
        ctx::Inputs in; in.bars = mkBars(60); const long long t = in.bars[40].t;
        in.facts["HODLOD"] = mkFacts("HODLOD", t, {{"hod", "7050"}, {"range_used", "0.6"}});
        auto a = ctx::buildCtx(in, t);
        ctx::Inputs in2 = in; in2.facts["HODLOD"] = mkFacts("HODLOD", t + 180, {{"hod", "9999"}, {"range_used", "0.9"}});
        auto b = ctx::buildCtx(in2, t);
        CHECK(a["hod"] == "7050" && b["hod"] == "?", "a fact decided after t is not used for CTX(t)");
        ctx::Inputs in3 = in; in3.bars.resize(41);
        auto c = ctx::buildCtx(in3, t);
        bool same = true; for (const auto& kv : a) if (kv.first != "present" && kv.first != "late" && c[kv.first] != kv.second) same = false;
        CHECK(same, "CTX(t) identical when later bars are removed");
    }
    // 3. UNKNOWN is never 0
    {
        ctx::Inputs in; in.bars = mkBars(30); auto k = ctx::buildCtx(in, in.bars.back().t);
        CHECK(k["trend_1h"] == "?" && k["hod"] == "?" && k["sweep"] == "?", "missing sources -> ?");
        CHECK(k["state"] == "ok", "state ok with no late sources");
    }
    // 4. location: at a level / confluence
    {
        ctx::Inputs in; in.bars = mkBars(40); const auto& B = in.bars.back();
        in.levels = {{B.c, "LonLO", "key"}, {B.c + 0.25, "VAL", "volume"}, {B.c + 50, "CR", "gamma"}};
        auto k = ctx::buildCtx(in, B.t);
        CHECK(k["at_level"] == "confluence", "two levels within tolerance -> confluence");
        in.levels = {{B.c, "LonLO", "key"}};
        k = ctx::buildCtx(in, B.t);
        CHECK(k["at_level"] == "key" && k["level_name"] == "LonLO", "single key level");
    }
    // 5. wave fields exist on a staircase
    {
        ctx::Inputs in; in.bars = mkBars(120); auto k = ctx::buildCtx(in, in.bars.back().t);
        CHECK(k["wave_dir"] != "?" && k["leg_no"] != "?", "wave found");
        CHECK(k["wave_pos"] == "start" || k["wave_pos"] == "mid" || k["wave_pos"] == "top", "wave_pos word");
        CHECK(k["pivot_forming"] == "high" || k["pivot_forming"] == "low" || k["pivot_forming"] == "none", "pivot word");
    }
    // 6. mock host: random indicator order per pass -> CTX(t) identical; AWARE(t) only uses CTX(t)
    {
        const std::string sym = "ESZ6", chart = "ES 3m"; const long spb = 180;
        std::vector<ctx::Bar> bars = mkBars(50); const long long t = bars[45].t;
        std::vector<std::string> order = {"AGG", "DP", "LIQ", "HODLOD", "TREND", "VWAP"};
        std::string first;
        std::mt19937 rng(7);
        for (int run = 0; run < 5; ++run) {
            for (const auto& s : order) mst::clear(s, sym, spb, chart);
            std::shuffle(order.begin(), order.end(), rng);
            long long now = 10000;
            for (const auto& s : order) { mst::publish(s, sym, spb, chart, now, t, {{"x", s}, {"trend", "1"}, {"hod", "7100"}}); now += 50; }
            ctx::Inputs in; in.bars = bars; std::map<std::string, mst::Facts> all = mst::readAll(sym, spb, chart, now);
            in.facts = all; for (auto& p : all) in.present.insert(p.first);
            auto k = ctx::buildCtx(in, t);
            std::string enc; for (auto& kv : k) enc += kv.first + "=" + kv.second + ";";
            if (run == 0) first = enc; else CHECK(enc == first, "CTX(t) independent of indicator order");
            mst::publish("CTX", sym, spb, chart, now, t, k);
        }
        // an indicator waiting for CTX(t+1) never takes CTX(t)
        cr::Ctx C = cr::readCtx(sym, spb, chart, 11000, t + 180, 100, 3000);
        CHECK(C.st == cr::CtxState::LATE, "CTX for another bar is not used (late)");
        C = cr::readCtx(sym, spb, chart, 11000, t + 180, 3000, 3000);
        CHECK(C.st == cr::CtxState::MISSING, "after timeout -> missing");
        C = cr::readCtx(sym, spb, chart, 11000, t, 0, 3000);
        CHECK(C.st == cr::CtxState::OK, "CTX(t) read");
        // restart: collector reset -> same decision
        ctx::Collector A, B; std::vector<std::string> l1, l2; std::map<std::string, long long> posted = {{"AGG", t}};
        A.ready(t, 0, {"AGG", "DP"}, posted, l1); B.reset(); B.ready(t, 0, {"AGG", "DP"}, posted, l2);
        CHECK(l1 == l2, "restart -> same collector decision");
    }
    // 7. fallback + record
    {
        cr::Read iso; iso.side = 1; iso.conf = 55; iso.word = "A";
        cr::Read adj = iso; adj.conf = 72; adj.reasons = "big seller HELD 7712 wave top stage 2";
        cr::Aware a1 = cr::finish(iso, &adj, cr::CtxState::MISSING, "L2");
        CHECK(a1.aware.conf == 55 && a1.ctx == "missing" && a1.links.empty(), "missing CTX -> aware = isolated");
        cr::Aware a2 = cr::finish(iso, &adj, cr::CtxState::OK, "L2");
        CHECK(a2.aware.conf == 72 && a2.isolated.conf == 55, "both answers kept");
        CHECK(cr::recordLine("DP", 123, a2).rfind("AWR|2|DP|123|ok|L2|-|1|55|1|72|", 0) == 0, "record line format v2 (basis -)");
    }
    // 7b. vocabulary v2: facts vs opinions, the basis of an aware read
    {
        cr::Ctx C; C.st = cr::CtxState::OK; C.f.ok = true;
        C.f.kv = { {"D.absorb_lots", "420"}, {"M.spike", "2.4"}, {"R.dp_a", "final~?~DP"}, {"R.trend_3m", "?"}, {"M.vwap_pos", "?"} };
        std::string v; double x = 0;
        CHECK(cr::field(C, "absorb_lots", v) && v == "420", "DATA is a fact");
        CHECK(cr::fieldD(C, "spike", x) && x == 2.4, "METRIC is a fact");
        CHECK(!cr::field(C, "dp_a", v), "a READ is never returned as a fact");
        CHECK(!cr::field(C, "vwap_pos", v), "unknown metric -> not known");
        cr::Opinion o = cr::opinion(C, "dp_a"); CHECK(o.ok && o.value == "final" && o.conf == "?" && o.src == "DP", "READ = labelled opinion (value, conf, source)");
        CHECK(!cr::opinion(C, "trend_3m").ok && !cr::opinion(C, "absorb_lots").ok, "unknown READ / a fact is not an opinion");
        CHECK(cr::factClass(C, "spike") == 'M' && cr::factClass(C, "dp_a") == 'R' && cr::factClass(C, "nope") == 0, "class of a field");
        cr::Read iso; iso.side = 1; cr::Read adj = iso; adj.conf = 60; adj.basis = "DM";
        cr::Aware a = cr::finish(iso, &adj, cr::CtxState::OK, "L2");
        CHECK(cr::recordLine("DP", 9, a).rfind("AWR|2|DP|9|ok|L2|DM|", 0) == 0, "the basis (classes used) is recorded");
        a = cr::finish(iso, &adj, cr::CtxState::LATE, "L2"); CHECK(a.aware.basis.empty(), "fallback -> no basis");
        CHECK(ctx::tagCtx({ {"dp_a", "final"}, {"state", "ok"} }, { {"DP", "70"} }).at("R.dp_a") == "final~70~DP", "READ carries its source's confidence");
    }
    // 8. link switches default OFF; market mismatch -> defaults
    {
        cr::Links L = cr::parseLinks("", "ES"); CHECK(!cr::linkOn(L, "L1"), "no file -> all OFF");
        L = cr::parseLinks("VERSION,1\r\nMARKET,ES\r\nLINK,L1,1\r\nLINK,L2,0\r\nPARAM,TIMEOUT_MS,2500\r\n", "ES");
        CHECK(cr::linkOn(L, "L1") && !cr::linkOn(L, "L2") && L.timeoutMs == 2500, "file switches");
        L = cr::parseLinks("MARKET,NQ\nLINK,L1,1\n", "ES"); CHECK(!cr::linkOn(L, "L1"), "wrong market -> defaults");
    }
    // 9. team read: agreement, conflict without trust -> no side, trust decides
    {
        std::vector<ctx::AwareRead> r = {{"DP", 1, 60, 5}, {"LIQ", 1, 70, 5}, {"AGG", -1, 50, 5}, {"STAGE", 1, 80, 4}};
        auto k = ctx::teamRead(5, r, {});
        CHECK(k["conflict"] == "1" && k["side"] == "0", "conflict with no trust -> no side");
        CHECK(k["n"] == "3", "a read for another bar is ignored");
        k = ctx::teamRead(5, r, {{"DP", 1.0}, {"LIQ", 1.0}, {"AGG", 0.5}});
        CHECK(k["side"] == "1", "trust decides the conflict");
        std::vector<ctx::AwareRead> r2 = {{"DP", -1, 60, 5}, {"LIQ", -1, 60, 5}};
        CHECK(ctx::teamRead(5, r2, {})["side"] == "-1", "agreement -> side");
    }
    std::printf("context tests: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
