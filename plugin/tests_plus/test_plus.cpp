// test_plus.cpp -- awareness_plus C++ layer: MarketProfile / Expectations / NewsGate / SourceHealth / AwarePlus on the fake board (HB_FAKE).
// g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE -I../code -I<smart_indicators/code> -I<plugin> test_plus.cpp && ./a.out
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "AwarePlus.h"

static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

static const char* PROFILE_ES =
    "VERSION,1\nMARKET,ES\nP,tick,0.25\nP,aggr_p90,8\nP,aggr_p99,27\nP,shown_med,38\nP,news_conf_x,0.4\nP,shock_rng_x,3\nP,shock_vol_x,2.5\n"
    "SLOT,510,8,20,4000,9000\nSLOT,540,6,15,3000,7000\nKN,absorb,all,5,1\nKN,stage3,all,8,0\nKN,turn,stretched,3,1.5\nKN,collision,all,4,1\n"
    "NEWS,2026-10-28,0730,CPI,5,15\nNEWS,wd3,0930,EIA_CRUDE,5,30\nNEWS,2026-10-29,1300,FOMC,5,60\nSRC,DP,1,60\nSRC,LIQ,1,60\nbogus,line\n";

static std::vector<ctx::Bar> mkBars(int n, long long t0, double px0) {
    std::vector<ctx::Bar> b; double px = px0;
    for (int i = 0; i < n; ++i) { ctx::Bar x; x.t = t0 + i * 180; x.o = px; px += (i % 20 < 10 ? 1.0 : -0.75); x.c = px; x.h = std::max(x.o, x.c) + 0.5; x.l = std::min(x.o, x.c) - 0.5; x.v = 4000; x.minOfDay = (int)((x.t % 86400) / 60); b.push_back(x); }
    return b;
}
static long long cp_days(int y, int m, int d) { y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; return era * 146097 + (long long)doe - 719468; }
static long long barId(int y, int mo, int d, int hh, int mi) { return cp_days(y, mo, d) * 86400LL + hh * 3600LL + mi * 60LL; }

int main() {
    // ---------------------------------------------------------------- MarketProfile
    {
        apm::Profile P = apm::parse(PROFILE_ES, "ES");
        CHECK(P.loaded, "ES file loads");
        CHECK(P.get("aggr_p99", -1) == 27 && P.get("missing", -7) == -7 && !P.has("missing"), "P rows + unknown stays the default");
        CHECK(P.slots.size() == 2 && P.slotFor(515) && P.slotFor(515)->rngMed == 8 && P.slotFor(545)->min == 540 && !P.slotFor(600), "slot lookup");
        CHECK(apm::knFor(P, "turn", "stretched").K == 3 && apm::knFor(P, "turn", "all").K == 3 /* default turn|all */ && apm::knFor(P, "absorb", "with_trend").K == 5, "K/N cell -> all -> default");
        CHECK(apm::knFor(P, "nosuch", "all").K == 5, "unknown read -> generic 5 bars");
        CHECK(P.note.find("1 bad") != std::string::npos, "bad lines counted, not fatal");
        apm::Profile Q = apm::parse(PROFILE_ES, "EU");
        CHECK(!Q.loaded && Q.note.find("another market") != std::string::npos && !Q.has("aggr_p99"), "never copy another market's numbers");
        apm::Profile R = apm::parse("MARKET,ES\nP,tick,1\n", "ES");
        CHECK(!R.loaded && R.note.find("VERSION") != std::string::npos, "no VERSION -> defaults");
        CHECK(apm::defaults("CL").news.size() == 1 && apm::defaults("NG").news[0].event == "EIA_GAS" && apm::defaults("ES").news.empty(), "built-in EIA weekday rules");
    }
    // ---------------------------------------------------------------- Expectations
    {
        apx::Exp E;
        CHECK(!apx::make(E, "DP", "absorb", "all", apx::Kind::LEAVE, 0, 100, 10, 9, 1, 5, 1, false), "LEAVE needs a side");
        CHECK(!apx::make(E, "DP", "absorb", "all", apx::Kind::LEAVE, 1, 100, 10, 9, 0, 5, 1, false), "LEAVE needs a known ATR");
        CHECK(apx::make(E, "DP", "absorb", "all", apx::Kind::LEAVE, 1, 100, 10, 9, 1, 3, 1, false) && E.target == 11, "target = ref + N x ATR");
        apx::Bar b{100, 12, 9.5, 11};
        CHECK(!apx::step(E, b) && E.bars == 0, "never judged on its own bar (no look-ahead)");
        apx::Bar b1{101, 10.6, 9.2, 10.4}, b2{102, 11.2, 10.0, 11.1};
        CHECK(!apx::step(E, b1) && apx::step(E, b2) && E.st == apx::State::MET && E.tDone == 102, "MET when it leaves N ATR");
        apx::make(E, "DP", "absorb", "all", apx::Kind::LEAVE, 1, 100, 10, 9, 1, 3, 1, false);
        apx::Bar both{101, 11.5, 8.9, 10};
        CHECK(apx::step(E, both) && E.st == apx::State::BROKEN && E.why.find("same bar") != std::string::npos, "target + extreme in one bar -> BROKEN");
        apx::make(E, "DP", "absorb", "all", apx::Kind::LEAVE, -1, 100, 10, 11, 1, 2, 1, false);
        apx::Bar s1{101, 10.5, 9.6, 9.8}, s2{102, 10.4, 9.5, 9.7};
        CHECK(!apx::step(E, s1) && apx::step(E, s2) && E.st == apx::State::BROKEN && E.why.rfind("slow", 0) == 0, "deadline without the move -> BROKEN slow");
        CHECK(E.mfe > 0.4 && E.mfe < 0.6, "mfe kept in ATR");
        apx::make(E, "STAGE", "stage3", "all", apx::Kind::NO_NEW_EXTREME, 1, 100, 10, 9, 1, 2, 0, false);
        apx::Bar n1{101, 10.2, 9.1, 10}, n2{102, 10.3, 9.05, 10.1};
        CHECK(!apx::step(E, n1) && apx::step(E, n2) && E.st == apx::State::MET, "stage 3: no new extreme in K -> MET");
        apx::make(E, "STAGE", "stage3", "all", apx::Kind::NO_NEW_EXTREME, 1, 100, 10, 9, 1, 5, 0, false);
        apx::Bar n3{101, 10, 8.9, 9.5};
        CHECK(apx::step(E, n3) && E.st == apx::State::BROKEN, "stage 3: new low -> BROKEN");
        apx::make(E, "LIQ", "held", "all", apx::Kind::HOLD_ABOVE, 1, 100, 10, 0, 1, 3, 0, false);
        apx::Bar h1{101, 10.1, 9.5, 9.8}, h2{102, 9.9, 9.5, 9.6};
        CHECK(!apx::step(E, h1) && apx::step(E, h2) && E.st == apx::State::BROKEN, "HELD bid: a bar wholly below -> BROKEN");
        apx::make(E, "LIQ", "held", "all", apx::Kind::HOLD_BELOW, -1, 100, 10, 0, 1, 1, 0, false);
        apx::Bar h3{101, 10.2, 9.9, 10};
        CHECK(apx::step(E, h3) && E.st == apx::State::MET, "HELD offer: deadline with every low <= ref -> MET");
        // the book: one per (src, t0), bars once, counts, record line
        apx::Book B; apx::Exp a, c;
        apx::make(a, "DP", "absorb", "all", apx::Kind::LEAVE, 1, 100, 10, 9, 1, 3, 1, true);
        apx::make(c, "STAGE", "stage3", "all", apx::Kind::NO_NEW_EXTREME, -1, 100, 10, 11, 1, 1, 0, false);
        CHECK(B.add(a) && !B.add(a) && B.add(c), "dedupe per source x bar");
        auto d1 = B.onBar(apx::Bar{101, 11.5, 9.5, 11.2});
        CHECK(d1.size() == 2 && B.met == 1 && B.broken == 1 && B.metNews == 1, "both resolved on one bar; news counted apart");
        CHECK(B.onBar(apx::Bar{101, 20, 0, 1}).empty(), "a bar is applied once");
        apx::Exp late; apx::make(late, "DP", "absorb", "all", apx::Kind::LEAVE, 1, 100, 10, 9, 1, 3, 1, false); late.src = "AGG";
        CHECK(!B.add(late), "a read for a bar already checked is refused");
        const std::string L = apx::recordLine(d1[0]);
        CHECK(L.rfind("EXP|1|", 0) == 0 && std::count(L.begin(), L.end(), '|') == 17, "record line shape");
        auto kv = B.kv(); CHECK(kv["class"] == "METRIC" && kv["exp_met"] == "1" && kv["last"] != "?", "board facts are METRIC");
    }
    // ---------------------------------------------------------------- NewsGate
    {
        apm::Profile P = apm::parse(PROFILE_ES, "ES");
        CHECK(apn::weekday(2026, 10, 28) == 3 && apn::weekday(2026, 10, 26) == 1, "weekday");
        apn::Clock c{2026, 10, 28, 3, 7 * 60 + 27};
        apn::Window W = apn::window(P, c); CHECK(W.state == "pre" && W.event == "CPI" && W.minsTo == 3, "3 min before CPI = pre");
        c.minOfDay = 7 * 60 + 40; W = apn::window(P, c); CHECK(W.state == "in" && W.minsSince == 10, "10 min after = in");
        c.minOfDay = 7 * 60 + 50; W = apn::window(P, c); CHECK(W.state == "off" && W.minsTo == 100, "after the window: off, next is EIA in 100");
        c.minOfDay = 9 * 60 + 45; W = apn::window(P, c); CHECK(W.state == "in" && W.event == "EIA_CRUDE", "weekday rule");
        apn::Clock thu{2026, 10, 29, 4, 13 * 60 + 30}; W = apn::window(P, thu); CHECK(W.state == "in" && W.event == "FOMC", "FOMC +60 window");
        apm::Profile S = P; S.news.push_back(apm::NewsRule{ "2026-10-28", 1000, "EIA_CRUDE", 5, 30 });
        apn::Clock w{2026, 10, 28, 3, 9 * 60 + 45}; CHECK(apn::window(S, w).state == "off", "a dated row shifts the weekday rule (holiday)");
        w.minOfDay = 10 * 60 + 5; CHECK(apn::window(S, w).state == "in", "the shifted release");
        apn::Shock k = apn::shock(P, 515, 30, 12000, 180); CHECK(k.state == "yes" && k.rngX > 3.7 && k.volX == 3, "shock = range AND volume vs slot");
        k = apn::shock(P, 515, 30, 5000, 180); CHECK(k.state == "no", "range alone is not a shock");
        k = apn::shock(P, 515, 10, 2000, 30); CHECK(k.rngX > 3.0 && k.rngX < 3.1 && k.volX > 2.99 && k.state == "yes", "30-s bars scale by sqrt / linear");
        k = apn::shock(P, 700, 99, 99999, 180); CHECK(k.state == "?", "no slot normal -> unknown, never no");
        apn::Gate G; apn::Clock q{2026, 10, 27, 2, 515};
        G.onBar(P, q, 1000, 30, 12000, 180); CHECK(G.news && G.kv["shock"] == "yes" && G.kv["conf_x"] == "0.40", "shock -> news + per-market conf_x");
        q.minOfDay = 530; G.onBar(P, q, 1000 + 900, 4, 3000, 180); CHECK(G.news && G.kv["shock_hot"] == "1", "hot for 15 min");
        q.minOfDay = 534; G.onBar(P, q, 1000 + 1140, 4, 3000, 180); CHECK(!G.news && G.kv["conf_x"] == "1.00" && G.kv["window"] == "off", "cools after");
        CHECK(G.kv["class"] == "METRIC" && G.kv.count("side") == 0, "news never gives a side");
    }
    // ---------------------------------------------------------------- SourceHealth
    {
        apm::Profile P = apm::parse(PROFILE_ES, "ES");
        aph::Monitor M; std::map<std::string, aph::Seen> s;
        const long long now = 1000000, t = 50000;
        s["DP"] = aph::Seen{ true, now - 2000, t };          // ok
        s["AGG"] = aph::Seen{ true, now - 90000, t };        // stale
        s["VWAP"] = aph::Seen{ true, now - 1000, t - 180 * 5 };   // lagging 5 bars
        // LIQ expected, absent -> missing; GEX never seen, not expected -> off
        M.update(P, s, now, t, 180, "ok", "on");
        CHECK(M.last["DP"].state == "ok" && M.last["AGG"].state == "stale" && M.last["VWAP"].state == "lagging" && M.last["LIQ"].state == "missing", "states");
        CHECK(M.last.count("GEX") == 0 || M.last["GEX"].state == "off", "unexpected + unseen is off");
        CHECK(!M.usable("AGG") && !M.usable("LIQ") && M.usable("DP"), "bad sources excluded");
        CHECK(M.kv["state"] == "degraded" && M.kv["bad"].find("AGG:stale") != std::string::npos && M.kv["class"] == "METRIC", "team fact");
        s["AGG"] = aph::Seen{ true, now + 5000 - 1000, t }; s["VWAP"].bar = t; s["LIQ"] = aph::Seen{ true, now, t };
        M.update(P, s, now + 5000, t, 180, "ok", "on");
        CHECK(M.kv["state"] == "ok" && M.changes >= 5 && !M.changeLines.empty(), "recovery is a change");
        s["DP"] = aph::Seen{}; M.update(P, s, now + 6000, t, 180, "ok", "on");
        CHECK(M.last["DP"].state == "gone", "cleared after being seen = gone");
        M.update(P, s, now + 7000, t, 180, "silent", "off");
        CHECK(M.kv["reader"] == "silent" && M.kv["mbo"] == "off", "recording reported");
        CHECK(aph::readerState(false, 0, "") == "?" && aph::readerState(true, 30, "ok") == "ok" && aph::readerState(true, 400, "") == "silent" && aph::readerState(true, 5, "ERROR x") == "error", "reader heartbeat");
        const std::string rec = "REC,ES,ok,on,1000000\nREC,NQ,ok,n/a,1000000\n";
        CHECK(aph::mboState(rec, "ES", 1000100) == "on" && aph::mboState(rec, "NQ", 1000100) == "n/a" && aph::mboState(rec, "ES", 1000000 + 8000) == "?" && aph::mboState(rec, "CL", 1000100) == "?", "MBO flag file, old = unknown");
    }
    // ---------------------------------------------------------------- AwarePlus on the fake board
    {
        const char* all[] = { "AGG", "BOOK", "LOTS", "DP", "LIQ", "GEX", "HODLOD", "COB", "VWAP", "TREND", "READ", "STAGE", "TREND1H", "AWSTAGE", "AWDP", "AWAGG", "AWHODLOD", "EXP", "NEWS", "HEALTH", nullptr };
        for (const char** s = all; *s; ++s) mst::clear(*s, "ESZ26", 180, "c1");
        const long long t0 = barId(2026, 10, 27, 8, 33);
        auto B = mkBars(40, t0, 5000);
        ap::Plus X; X.load("ES", PROFILE_ES, 0.25); X.reader = "ok"; X.mbo = "on";
        CHECK(X.prof.loaded, "profile loaded");
        apn::Clock ck = ap::clockOf(t0); CHECK(ck.y == 2026 && ck.mo == 10 && ck.d == 27 && ck.wd == 2 && ck.minOfDay == 513, "clock of a bar id");
        long long now = 5000000;
        std::vector<ctx::Bar> b1(B.begin(), B.begin() + 20); const long long t = b1.back().t;
        mst::publish("DP", "ESZ26", 180, "c1", now, t, { {"a", "final"} });
        mst::publish("AGG", "ESZ26", 180, "c1", now - 120000, t, { {"x", "1"} });   // stale
        auto exc = X.preCtx("ESZ26", 180, "c1", now, b1);
        CHECK(exc.count("AGG") && exc.count("LIQ") && !exc.count("DP"), "stale + missing excluded before CTX");
        std::map<std::string, std::string> ctxKv{ {"M.atr", "2"}, {"M.stretched", "0"}, {"R.trend_3m", "1~?~TREND"} };
        X.foldCtx(ctxKv);
        CHECK(ctxKv["M.src_state"] == "degraded" && ctxKv["M.news"] == "0" && ctxKv["D.mkt_aggr_p99"] == "27" && ctxKv["D.mkt_refill_share"] == "?", "CTX gets METRIC facts; unknown personality = ?");
        mst::publish("AWDP", "ESZ26", 180, "c1", now, t, { {"side", "1"}, {"conf", "0"}, {"word", "final A"}, {"class", "READ"} });
        mst::publish("AWSTAGE", "ESZ26", 180, "c1", now, t, { {"side", "-1"}, {"conf", "0"}, {"word", "stage 3"}, {"class", "READ"} });
        mst::publish("AWAGG", "ESZ26", 180, "c1", now, t, { {"side", "1"}, {"word", "aggression"} });   // AGG is stale -> its read registers nothing
        mst::publish("AWHODLOD", "ESZ26", 180, "c1", now, t, { {"side", "0"}, {"word", "hod / lod"}, {"exp_kind", "hold_above"}, {"exp_read", "held"}, {"exp_ref", "5005"} });
        X.postTeam("ESZ26", 180, "c1", now, b1, ctxKv);
        CHECK(X.book.open.size() == 3, "DP absorb + STAGE stage3 + declared HOLD registered; stale AGG skipped");
        bool dpWithTrend = false; for (const auto& e : X.book.open) if (e.src == "DP" && e.ctx == "with_trend" && e.K == 5 && e.N == 1) dpWithTrend = true;
        CHECK(dpWithTrend, "K/N from the market file, context cell with_trend -> absorb|all fallback");
        X.postTeam("ESZ26", 180, "c1", now, b1, ctxKv); CHECK(X.book.open.size() == 3, "TEAM step once per bar");
        auto L = X.take(); int regs = 0; for (auto& l : L) if (l.rfind("EXPREG|1|", 0) == 0) ++regs;
        CHECK(regs == 3 && std::any_of(L.begin(), L.end(), [](const std::string& l) { return l.rfind("HEALTH|1|", 0) == 0; }) && std::any_of(L.begin(), L.end(), [](const std::string& l) { return l.rfind("NEWS|1|", 0) == 0; }), "record lines");
        std::map<std::string, std::string> team{ {"side", "1"} }; X.foldTeam(team, ctxKv);
        CHECK(team["conf_x"] == "1.00" && team["news"] == "0" && team["excluded"].find("AGG") != std::string::npos, "TEAM carries the gate");
        X.publish("ESZ26", 180, "c1", now);
        CHECK(mst::read("EXP", "ESZ26", 180, "c1", now + 1).ok && mst::read("HEALTH", "ESZ26", 180, "c1", now + 1).ok && mst::read("NEWS", "ESZ26", 180, "c1", now + 1).ok, "posted to the board");
        // the next bars resolve them (only bars after t)
        int verdicts = 0;
        for (size_t i = 21; i <= 30; ++i) { std::vector<ctx::Bar> bi(B.begin(), B.begin() + i); X.preCtx("ESZ26", 180, "c1", now + (long long)i * 1000, bi); for (auto& l : X.take()) if (l.rfind("EXP|1|", 0) == 0) ++verdicts; }
        CHECK(verdicts == 3 && X.book.open.empty() && X.book.met + X.book.broken == 3, "every expectation resolved by its deadline");
        CHECK(ap::sourceExcluded(ctxKv, "AGG") && ap::sourceExcluded(ctxKv, "LIQ") && !ap::sourceExcluded(ctxKv, "DP") && !ap::sourceExcluded({}, "AGG"), "links fall back while a source is excluded");
        // PLUS off: nothing
        ap::Plus Off; Off.enabled = false; Off.load("ES", PROFILE_ES, 0.25);
        CHECK(Off.preCtx("ESZ26", 180, "c1", now, b1).empty() && Off.take().empty(), "disabled = no effect");
        std::map<std::string, std::string> k2; Off.foldCtx(k2); CHECK(k2.empty(), "disabled = CTX unchanged");
    }
    std::printf("test_plus: %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
