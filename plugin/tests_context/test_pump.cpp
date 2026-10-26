// test_pump.cpp -- lsContext's pump (ContextPump.h) on the fake board (HB_FAKE): the host order / timeout / restart behaviour.
// g++ -std=c++17 -Wall -Wextra -DHB_FAKE -I. -I<plugin> test_pump.cpp && ./a.out
#include <cstdio>
#include <cstdlib>
#include "ContextPump.h"

static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

static std::vector<ctx::Bar> mkBars(int n, long long t0) {
    std::vector<ctx::Bar> b; double px = 5000;
    for (int i = 0; i < n; ++i) { ctx::Bar x; x.t = t0 + i * 180; x.o = px; px += (i % 20 < 10 ? 2.0 : -1.5); x.c = px; x.h = std::max(x.o, x.c) + 0.75; x.l = std::min(x.o, x.c) - 0.75; x.v = 1000 + i; x.minOfDay = 510 + i * 3; b.push_back(x); }
    return b;
}
static void post(const char* src, long long bar, long long now, std::map<std::string, std::string> kv) { mst::publish(src, "ESZ26", 180, "c1", now, bar, kv); }
static void wipe() { const char* all[] = { "AGG", "BOOK", "LOTS", "DP", "LIQ", "GEX", "HODLOD", "COB", "VWAP", "TREND", "READ", "STAGE", "TREND1H", "CTX", "TEAM", "AWSTAGE", "AWDP", "AWAGG", "AWHODLOD", nullptr };
                         for (const char** s = all; *s; ++s) mst::clear(*s, "ESZ26", 180, "c1"); }
static cp::Pump mkPump(const std::vector<ctx::Bar>& b) { cp::Pump p; p.sym = "ESZ26"; p.spb = 180; p.chart = "c1"; p.market = "ES"; p.bars = b; return p; }

int main() {
    const long long t0 = cp::barId(2026, 10, 26, 8, 33, 0);
    auto B = mkBars(60, t0); const long long t = B.back().t, tPrev = B[B.size() - 2].t;
    CHECK(cp::barId(2026, 10, 26, 0, 0, 0) % 86400 == 0, "bar id is CT seconds");

    // 1. waits for a present source still at t-1, then builds at t when it arrives
    wipe(); long long now = 1000000;
    post("VWAP", t, now, { {"vwap", "5010"}, {"pos", "above1"} }); post("TREND", tPrev, now, { {"trend", "1"} });
    cp::Pump P = mkPump(B);
    cp::Out o = P.step(now);
    CHECK(o.ctxLine.empty() && o.state == "waiting", "waits for TREND at t-1");
    post("TREND", t, now + 200, { {"trend", "1"}, {"trend_word", "up"} });
    o = P.step(now + 250);
    CHECK(!o.ctxLine.empty() && o.ctxPosted, "CTX(t) built once TREND reaches t");
    { mst::Facts c = mst::read("CTX", "ESZ26", 180, "c1", now + 250); CHECK(c.ok && c.bar == t, "CTX posted for t");
      CHECK(mst::getS(c, "state") == "ok", "state ok (nobody late)"); CHECK(mst::getS(c, "R.trend_3m") == "1~?~TREND", "trend copied as a labelled READ (value~conf~source)"); CHECK(mst::getS(c, "M.vwap_pos") == "above1", "vwap position copied as a METRIC");
      CHECK(mst::getD(c, "D.bar_c", 0) == B.back().c && mst::getD(c, "D.bar_h", 0) == B.back().h, "bar close / high carried for the nightly"); }
    // decided once: a later change of a source for t does not rebuild
    post("TREND", t, now + 500, { {"trend", "-1"} });
    o = P.step(now + 600);
    CHECK(o.ctxLine.empty(), "CTX(t) decided once (no rebuild)");
    { mst::Facts c = mst::read("CTX", "ESZ26", 180, "c1", now + 600); CHECK(mst::getS(c, "R.trend_3m") == "1~?~TREND", "still the first decision"); }

    // 2. timeout: a source that never arrives -> built after the timeout, marked partial with the late source
    wipe(); now = 2000000; post("VWAP", t, now, { {"vwap", "5010"} }); post("DP", tPrev, now, { {"a", "1"} });
    P = mkPump(B); P.links.timeoutMs = 3000;
    o = P.step(now); CHECK(o.ctxLine.empty(), "waits for DP");
    o = P.step(now + 2999); CHECK(o.ctxLine.empty(), "still waiting at 2999 ms");
    o = P.step(now + 3000); CHECK(!o.ctxLine.empty(), "built at the timeout");
    { mst::Facts c = mst::read("CTX", "ESZ26", 180, "c1", now + 3000); CHECK(mst::getS(c, "state") == "partial", "partial"); CHECK(mst::getS(c, "late").find("DP") != std::string::npos, "DP listed late"); }

    // 3. a source that posted t+1 (from a later bar) is not used for CTX(t)
    wipe(); now = 3000000; post("VWAP", t + 180, now, { {"vwap", "5099"}, {"pos", "below2"} });
    P = mkPump(B); o = P.step(now);
    { mst::Facts c = mst::read("CTX", "ESZ26", 180, "c1", now); CHECK(c.ok && mst::getS(c, "M.vwap_pos") == "?", "fact for t+1 not used (UNKNOWN)"); }

    // 4. host order: indicators posting before or after lsContext's calc give the same CTX(t)
    std::string a, b;
    { wipe(); now = 4000000; post("VWAP", t, now, { {"pos", "above1"} }); post("TREND", t, now, { {"trend", "1"} });
      P = mkPump(B); a = P.step(now + 10).ctxLine; }
    { wipe(); now = 4000000; P = mkPump(B); P.step(now); post("TREND", t, now + 5, { {"trend", "1"} }); post("VWAP", t, now + 6, { {"pos", "above1"} });
      b = P.step(now + 10).ctxLine; }
    CHECK(!a.empty() && a == b, "same CTX(t) whatever the host order");

    // 5. restart: a fresh pump on the same board builds the same CTX(t)
    { wipe(); now = 5000000; post("VWAP", t, now, { {"pos", "above1"} }); P = mkPump(B); a = P.step(now).ctxLine;
      cp::Pump Q = mkPump(B); b = Q.step(now + 50000).ctxLine; CHECK(!a.empty() && a == b, "restart -> same CTX(t)"); }

    // 6. TEAM: waits for the aware reads for t, conflict without trust -> no side
    wipe(); now = 6000000; post("VWAP", t, now, { {"pos", "above1"} });
    post("AWSTAGE", tPrev, now, { {"side", "1"}, {"conf", "60"} }); post("AWDP", tPrev, now, { {"side", "-1"}, {"conf", "55"} });
    P = mkPump(B); o = P.step(now);
    CHECK(!o.ctxLine.empty() && o.teamLine.empty(), "TEAM waits for the reads for t");
    post("AWSTAGE", t, now + 100, { {"side", "1"}, {"conf", "60"} }); post("AWDP", t, now + 100, { {"side", "-1"}, {"conf", "55"} });
    o = P.step(now + 200);
    CHECK(!o.teamLine.empty(), "TEAM decided when both are at t");
    { mst::Facts tm = mst::read("TEAM", "ESZ26", 180, "c1", now + 200); CHECK(mst::getS(tm, "conflict") == "1" && mst::getS(tm, "side") == "0", "conflict, no trust -> no side"); }
    // with trust, the trusted one decides
    wipe(); now = 7000000; post("VWAP", t, now, { {"pos", "above1"} });
    post("AWSTAGE", t, now, { {"side", "1"}, {"conf", "60"} }); post("AWDP", t, now, { {"side", "-1"}, {"conf", "55"} });
    P = mkPump(B); P.trust["STAGE"] = 0.7; P.trust["DP"] = 0.3; P.step(now);
    { mst::Facts tm = mst::read("TEAM", "ESZ26", 180, "c1", now); CHECK(mst::getS(tm, "side") == "1", "trust decides the conflict"); }
    // a read for t+1 never counts for TEAM(t); timeout closes TEAM without it
    wipe(); now = 8000000; post("VWAP", t, now, { {"pos", "above1"} }); post("AWAGG", t + 180, now, { {"side", "-1"}, {"conf", "70"} });
    P = mkPump(B); o = P.step(now); CHECK(o.teamLine.empty(), "TEAM waits (AWAGG not at t)");
    o = P.step(now + 3000); CHECK(!o.teamLine.empty(), "TEAM closed at the timeout");
    { mst::Facts tm = mst::read("TEAM", "ESZ26", 180, "c1", now + 3000); CHECK(mst::getS(tm, "n") == "0", "the t+1 read not counted"); }

    // 7. re-post every 10 s (readers see it; MarketState drops > 60 s), unchanged
    o = P.step(now + 9000); CHECK(!o.ctxPosted, "no re-post before 10 s");
    o = P.step(now + 13001); CHECK(o.ctxPosted && o.teamPosted && o.ctxLine.empty(), "re-posted unchanged after 10 s");

    // 8. a new bar starts a new collection
    B.push_back(B.back()); B.back().t += 180; P.bars = B;
    o = P.step(now + 13100); CHECK(o.ctxLine.empty() && o.state == "waiting", "new bar -> collect again (VWAP at t-1 now)");

    // 8b. vocabulary v2 on the posted CTX: meta keys plain, every other key class-tagged; TEAM is a READ
    { wipe(); cp::Pump V = mkPump(B); post("VWAP", B.back().t, 9500000, { {"pos", "above1"} }); V.step(9500000);
      mst::Facts c = mst::read("CTX", "ESZ26", 180, "c1", 9500000); bool tagged = c.ok;
      for (const auto& p : c.kv) { const std::string& k = p.first; if (ctx::isMeta(k)) continue; tagged = tagged && k.size() > 2 && k[1] == '.' && (k[0] == 'D' || k[0] == 'M' || k[0] == 'R'); }
      CHECK(tagged && mst::getS(c, "cls_v") == "2", "every CTX fact class-tagged");
      CHECK(mst::getS(c, "M.atr") != "" && mst::getS(c, "D.hod") == "?" && !c.kv.count("atr"), "METRIC / DATA keys, no untagged copy");
      CHECK(ctx::classOf("dp_a") == 'R' && ctx::classOf("absorb_lots") == 'D' && ctx::classOf("spike") == 'M' && ctx::classOf("unknown_new") == 'R', "class table; unknown = READ");
      V.step(9503000); mst::Facts tm = mst::read("TEAM", "ESZ26", 180, "c1", 9503000); CHECK(mst::getS(tm, "class") == "READ", "TEAM posted as a READ"); }

    // 9. levels from facts (bar <= t)
    { std::map<std::string, mst::Facts> F; mst::Facts v; v.ok = true; v.bar = t; v.kv["vwap"] = "5010"; F["VWAP"] = v;
      mst::Facts d; d.ok = true; d.bar = t + 180; d.kv["node_px"] = "5000"; F["DP"] = d;
      auto L = cp::levelsFromFacts(F, t); CHECK(L.size() == 1 && L[0].type == "key", "levels: only facts decided for <= t"); }

    // 9b. an empty board (IRT start-up, sources not posted yet) waits the timeout before building
    { wipe(); cp::Pump E = mkPump(B); CHECK(E.step(9000000).ctxLine.empty(), "empty board waits");
      CHECK(!E.step(9003000).ctxLine.empty(), "empty board builds at the timeout"); }

    // 9c. market / open / trust file
    CHECK(cp::marketOf("CLEX26") == "CL" && cp::marketOf("EPZ26") == "ES" && cp::marketOf("CPEZ26") == "HG", "market names");
    CHECK(cp::rthOpenOf("HG") == 440 && cp::rthOpenOf("ES") == 510 && cp::rthOpenOf("NG") == 480, "RTH opens");
    { auto T = cp::parseTrust("VERSION,1\r\nMARKET,ES\r\nTRUST,STAGE,0.6\r\nTRUST,DP,7\r\n", "ES"); CHECK(T.size() == 1 && T["STAGE"] == 0.6, "trust parsed, bad weight dropped");
      CHECK(cp::parseTrust("MARKET,NQ\nTRUST,STAGE,0.6\n", "ES").empty(), "wrong-market trust file ignored"); }

    // 10. clear removes CTX / TEAM
    P.clear(); CHECK(!mst::read("CTX", "ESZ26", 180, "c1", now + 13100).ok, "cleared on removal");

    std::printf("%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
