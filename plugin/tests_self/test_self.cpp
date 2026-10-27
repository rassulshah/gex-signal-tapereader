// test_self.cpp -- SelfAware.h: own values since the open, own signals resolved with later bars only, track record,
// shrunk self-adjusted confidence for NEW signals only, OFF by default, params parsing, board facts, record lines.
// g++ -std=c++17 -Wall -Wextra -DHB_FAKE -I. -I<plugin> test_self.cpp && ./a.out
#include <cstdio>
#include <sys/stat.h>
#include "SelfAware.h"

static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static bool has(const std::vector<std::string>& v, const std::string& pre) { for (auto& s : v) if (s.rfind(pre, 0) == 0) return true; return false; }
static int count(const std::vector<std::string>& v, const std::string& sub) { int n = 0; for (auto& s : v) n += s.find(sub) != std::string::npos; return n; }

// Central wall-clock seconds of 2026-10-26 hh:mm
static long long T(int hh, int mm) { return 1792972800LL /*2026-10-26 00:00*/ + hh * 3600LL + mm * 60LL; }

int main() {
    // calendar helpers
    CHECK(sa::sessionKey(T(9, 0)) == "20261026", "09:00 CT -> today's session");
    CHECK(sa::sessionKey(T(17, 0)) == "20261027", "17:00 CT -> next session (Globex roll)");
    CHECK(sa::sessionKey(T(16, 59)) == "20261026", "16:59 CT -> still today");
    CHECK(sa::minuteOfDay(T(9, 45)) == 585, "minute of day");
    CHECK(sa::todBucket(500) == "on" && sa::todBucket(520) == "open" && sa::todBucket(600) == "am" && sa::todBucket(700) == "mid" &&
          sa::todBucket(800) == "pm" && sa::todBucket(1000) == "on" && sa::todBucket(-1) == "?", "time buckets");
    CHECK(sa::recName("DP", "ES", 60, "20261026") == "Self-DP-ES-60s-20261026.rec" && sa::paramsName("DP", "ES") == "LRA-Self-DP-ES.csv", "names");

    // params
    { sa::Params P = sa::parseParams("", "ES", "DP"); CHECK(!P.on && P.K == 10 && P.horizon == 20 && P.prior.empty(), "defaults OFF"); }
    { sa::Params P = sa::parseParams("VERSION,1\r\nMARKET,ES\r\nIND,DP\r\nSWITCH,1\r\nPARAM,K,20\r\nPARAM,HORIZON,30\r\nPRIOR,am,30,50\r\nPRIOR,all,110,200\r\nPRIOR,bad,9,5\r\nPARAM,K,9999\r\n", "ES", "DP");
      CHECK(P.on && P.K == 20 && P.horizon == 30, "switch + params"); CHECK(P.prior.size() == 2 && P.prior["am"].first == 30, "priors (bad rows dropped)"); }
    { sa::Params P = sa::parseParams("MARKET,NQ\nSWITCH,1\nPRIOR,am,30,50\n", "ES", "DP"); CHECK(!P.on && P.prior.empty() && P.note.find("NQ") != std::string::npos, "other market -> defaults"); }
    { sa::Params P = sa::parseParams("MARKET,ES\nIND,LIQ\nSWITCH,1\n", "ES", "DP"); CHECK(!P.on, "other indicator -> defaults"); }

    // a session: 30 quiet bars (ATR ~2), a long signal, then price climbs -> HIT
    sa::Tracker tr; tr.ind = "DP";
    tr.P.prior["am"] = { 55, 100 }; tr.P.prior["all"] = { 50, 100 };
    const std::string day = sa::sessionKey(T(9, 0));
    double px = 5000; long long t = 0;
    for (int i = 0; i < 30; ++i) { t = T(8, 30) + i * 60; sa::Bar b{ t, px, px + 1, px - 1, px }; tr.onBar(day, b); tr.onValue(1.0 + 0.1 * i); }
    CHECK(tr.atr() > 1.9 && tr.atr() < 2.1, "ATR from own bars");
    CHECK(tr.pctile(10) == 100 && tr.pctile(0) == 0 && tr.pctile(2.45) == 50, "own value percentile since the open");
    // OFF: returned confidence = base, adjusted is still computed and recorded
    int used = tr.onSignal(t, +1, "am", 60, 3.1, "a_x=3.1 node=5000", px);
    CHECK(used == 60, "OFF -> base used");
    CHECK(tr.sigs.back().adj == 60, "no record today -> shrunk = prior -> factor 1");
    std::vector<std::string> L = tr.takeLines(); CHECK(L.size() == 1 && has(L, "SELF|1|DP|") && L[0].find("|am|1|60|60|0|3.1|") != std::string::npos, "SELF record line");
    // the signal bar itself never resolves it; re-feeding an old bar changes nothing
    tr.onBar(day, sa::Bar{ t, px, px + 50, px - 50, px }); CHECK(tr.sigs.back().outcome == 9, "same bar re-fed -> still pending (no look-ahead)");
    for (int i = 1; i <= 3; ++i) { sa::Bar b{ t + i * 60, px, px + 0.8 * i, px - 0.5, px + 0.5 * i }; tr.onBar(day, b); }
    CHECK(tr.sigs.back().outcome == 1 && tr.hits == 1 && tr.resolved == 1, "+1 ATR first -> HIT");
    L = tr.takeLines(); CHECK(has(L, "SELFO|1|DP|") && count(L, "|HIT|") == 1, "SELFO HIT line");
    t += 3 * 60; px += 1.5;

    // a short that fails: price rises 1 ATR -> MISS
    tr.onSignal(t, -1, "am", 60, 2.0, "a_x=2.0", px);
    tr.onBar(day, sa::Bar{ t + 60, px, px + 2.5, px - 0.2, px + 2.4 });
    CHECK(tr.sigs.back().outcome == -1 && tr.misses == 1, "1 ATR against first -> MISS");
    t += 60; px += 2.4;
    // both sides in one bar: the close decides
    tr.onSignal(t, +1, "am", 60, 2.0, "", px);
    tr.onBar(day, sa::Bar{ t + 60, px, px + 5, px - 5, px - 1 });
    CHECK(tr.sigs.back().outcome == -1, "both touched, close against -> MISS (conservative)");
    t += 60;
    // expiry after HORIZON bars of nothing
    tr.onSignal(t, +1, "mid", 60, 2.0, "", px);
    for (int i = 1; i <= tr.P.horizon; ++i) tr.onBar(day, sa::Bar{ t + i * 60, px, px + 0.1, px - 0.1, px });
    CHECK(tr.sigs.back().outcome == 0 && tr.expired == 1 && tr.resolved == 3, "no move in HORIZON bars -> EXP (not a miss)");
    t += tr.P.horizon * 60;
    tr.takeLines();

    // today in "am": 1 hit / 3 resolved. Prior am 0.55, K 10: shrunk = (1 + 5.5) / 13 = 0.5 -> factor 0.909 -> 60 * 0.909 = 55
    int before = tr.sigs[0].adj;
    used = tr.onSignal(t, +1, "am", 60, 2.0, "", px);
    CHECK(tr.sigs.back().adj == 55, "shrunk self-adjusted confidence from the record BEFORE the signal");
    CHECK(used == 60, "still OFF -> base used");
    CHECK(tr.sigs[0].adj == before, "an earlier signal is never re-scored (no repaint)");
    // same bar + side twice -> decided once
    tr.onSignal(t, +1, "am", 90, 2.0, "", px); CHECK(tr.sigs.size() == 5, "decided once per bar and side");
    // ON: the adjusted one is used; clipped to [0.5, 1.5]
    tr.P.on = true; tr.todayCtx["pm"] = { 0, 40 }; tr.P.prior["pm"] = { 60, 100 };
    used = tr.onSignal(t + 60, -1, "pm", 80, 2.0, "", px);
    CHECK(used == 40, "ON -> adjusted used; 0/40 vs prior 0.6 -> clipped at 0.5x");
    tr.todayCtx["open"] = { 40, 40 }; tr.P.prior["open"] = { 20, 100 };
    used = tr.onSignal(t + 120, -1, "open", 50, 2.0, "", px); CHECK(used == 75, "clipped at 1.5x");
    used = tr.onSignal(t + 180, -1, "zzz", 50, 2.0, "", px); CHECK(used == 50, "unknown ctx -> falls back to 'all' prior (0.5, no record) -> x1");
    // unknown long run: no prior at all -> unchanged
    { sa::Tracker u; u.ind = "DP"; u.P.on = true; for (int i = 0; i < 20; ++i) u.onBar(day, sa::Bar{ T(9, 0) + i * 60, 1, 2, 0, 1 });
      CHECK(u.onSignal(T(9, 19), 1, "am", 70, 1, "", 1) == 70, "no long run -> base"); }
    // no ATR yet -> cannot be judged; recorded as EXP
    { sa::Tracker u; u.ind = "DP"; u.onBar(day, sa::Bar{ T(9, 0), 1, 2, 0, 1 }); u.onSignal(T(9, 0), 1, "am", 70, 1, "", 1);
      CHECK(u.expired == 1 && u.resolved == 0 && has(u.takeLines(), "SELF|") , "first bar signal -> EXP, never a miss"); }

    // board facts
    auto kv = tr.facts(3.1);
    CHECK(kv["M.today_hit"] == "1/3" && kv["M.today_rate"] == "0.333", "today hit / rate");
    CHECK(kv["M.longrun_rate"] == "0.5" && kv["M.self_on"] == "1" && kv["class"] == "METRIC", "long run + switch + class");
    CHECK(kv["D.signals_today"] == "8" && kv["D.pending"] == "4", "signal counts");
    CHECK(kv["D.last_ctx"] == "zzz" && kv["M.last_base"] == "50" && kv["M.ctx_longrun"] == "0.5" && kv["M.ctx_today"] == "0/0", "latest signal explained");
    CHECK(kv["M.value_pctile"] == "70" && kv["M.values_today"] == "30", "own value context");
    { sa::Tracker u; auto k = u.facts(std::nan("")); CHECK(k["M.today_rate"] == "?" && k["M.value_pctile"] == "?" && k["D.value_last"] == "?", "unknown = ?, never 0"); }
    mst::publish(sa::boardSrc("DP"), "ESZ6", 60, "c1", 1000, t, kv);
    { mst::Facts F = mst::read("SELFDP", "ESZ6", 60, "c1", 1500); CHECK(F.ok && F.kv["M.today_hit"] == "1/3", "posted to the board as SELFDP"); }

    // new session: pending ones close as EXP, today resets
    tr.takeLines();
    tr.onBar(sa::sessionKey(T(17, 0)), sa::Bar{ T(17, 0), 1, 2, 0, 1 });
    L = tr.takeLines(); CHECK(count(L, "|EXP|") == 4 && tr.sigs.empty() && tr.hits == 0 && tr.values.empty(), "session roll: pending -> EXP, today reset");

    // record file
    { const std::string D = "/tmp/claude-0/sa"; mkdir("/tmp/claude-0", 0755); mkdir(D.c_str(), 0755);
      const std::string p = D + "/" + sa::recName("DP", "ES", 60, "20261026"); std::remove(p.c_str());
      CHECK(sa::appendLines(p, { "SELF|1|DP|1|am|1|60|60|0|3|?|0/0|?|x", "SELFO|1|DP|1|2|HIT|1|am|1|60|60" }), "append");
      const std::string txt = sa::readText(p); CHECK(txt.find("SELFO|1|DP|1|2|HIT") != std::string::npos && txt.find("\r\n") != std::string::npos, "CRLF lines"); }

    // the hook: params file per indicator + market, record file, board post, only new bars do work
    { const std::string D = "/tmp/claude-0/sa/hook"; mkdir(D.c_str(), 0755);
      for (const char* f : { "/Self-DP-ES-60s-20261026.rec" }) std::remove((D + f).c_str());
      FILE* f = std::fopen((D + "/LRA-Self-DP-ES.csv").c_str(), "wb"); std::fputs("VERSION,1\r\nMARKET,ES\r\nIND,DP\r\nSWITCH,0\r\nPRIOR,am,60,100\r\n", f); std::fclose(f);
      sa::Hook h; h.ind = "DP"; h.dir = D;
      for (int i = 0; i < 20; ++i) h.bar("ESZ6", 60, "c9", "ES", 1000 + i, sa::Bar{ T(9, 30) + i * 60, 10, 11, 9, 10 }, 2.0 + i);
      CHECK(h.tr.P.prior.count("am") == 1 && !h.tr.P.on, "params loaded for ES/DP");
      CHECK(h.tr.values.size() == 20, "values fed");
      h.bar("ESZ6", 60, "c9", "ES", 5000, sa::Bar{ T(9, 49), 10, 99, 0, 10 }, 99); CHECK(h.tr.values.size() == 20 && h.tr.bars.size() == 20, "same bar again -> no work");
      const int c = h.signal("ES", 60, T(9, 49), -1, "", 70, 21, "a_x=21", 10);
      CHECK(c == 70 && h.tr.sigs.back().ctx == "am", "signal: OFF -> base; ctx = time bucket");
      h.bar("ESZ6", 60, "c9", "ES", 6000, sa::Bar{ T(9, 50), 10, 10.5, 7, 7.5 }, 1);
      CHECK(h.tr.hits == 1, "resolved by the next bar");
      const std::string txt = sa::readText(D + "/Self-DP-ES-60s-20261026.rec");
      CHECK(txt.find("SELF|1|DP|") != std::string::npos && txt.find("SELFO|1|DP|") != std::string::npos && h.writes == 2, "record file written");
      mst::Facts F = mst::read("SELFDP", "ESZ6", 60, "c9", 6100); CHECK(F.ok && F.kv["M.today_hit"] == "1/1", "board SELFDP after the outcome");
      sa::Hook n; n.ind = "DP"; n.dir = D; n.bar("NQZ6", 60, "c9", "NQ", 1, sa::Bar{ T(9, 30), 1, 2, 0, 1 }, 1);
      CHECK(n.tr.P.prior.empty() && !n.tr.P.on, "NQ chart reads its own NQ file (none) -> defaults");
      h.signal("ES", 60, T(9, 50), 1, "", 50, 1, "", 7.5);
      h.bar("NQZ6", 60, "c9", "NQ", 7000, sa::Bar{ T(9, 51), 20, 21, 19, 20 }, 1);
      CHECK(h.tr.sigs.empty() && h.tr.values.size() == 1 && h.tr.hits == 0, "symbol change -> a fresh memory");
      CHECK(sa::readText(D + "/Self-DP-ES-60s-20261026.rec").find("|EXP|") != std::string::npos, "the old symbol's pending signal closed as EXP");
      h.clear("ESZ6", 60, "c9"); CHECK(!mst::read("SELFDP", "ESZ6", 60, "c9", 6100).ok, "clear"); }

    // declarative glue: facts -> value + signal
    { CHECK(sa::sideOf("support") == 1 && sa::sideOf("resistance") == -1 && sa::sideOf("buy_arriving") == 1 && sa::sideOf("sell_leaving") == -1 &&
            sa::sideOf("none") == 0 && sa::sideOf("?") == 0 && sa::sideOf("Bull") == 1, "side words");
      sa::Hook h; h.ind = "LIQ"; h.dir = "/tmp/claude-0/sa/hook";
      sa::Spec sp; sp.valueKey = "zone_x"; sp.sideKey = "zone_side"; sp.eventKey = "zone_id"; sp.onChange = true; sp.why = { "zone_id", "zone_lots" };
      int fired = 0;
      for (int i = 0; i < 20; ++i) {
          std::map<std::string, std::string> kv{ { "zone_x", "2.5x" }, { "zone_side", "support" }, { "zone_id", i < 15 ? "?" : "VAL 5000.25" }, { "zone_lots", "900" } };
          if (sa::fromFacts(h, sp, "ESZ6", 60, "c7", "ES", 100 + i, sa::Bar{ T(10, 0) + i * 60, 10, 11, 9, 10 }, kv) >= 0) ++fired;
      }
      CHECK(fired == 1 && h.tr.sigs.size() == 1 && h.tr.sigs[0].side == 1, "ON_CHANGE: one signal per new zone");
      { sa::Hook gh; gh.ind = "LIQ"; gh.dir = h.dir; sa::Spec gs = sp; gs.gateKey = "zone_step"; int n = 0;
        for (int i = 0; i < 6; ++i) n += sa::fromFacts(gh, gs, "ESZ6", 60, "c8", "ES", 1, sa::Bar{ T(10, 0) + i * 60, 10, 11, 9, 10 },
            { { "zone_side", "support" }, { "zone_id", i < 3 ? "VAL 1" : "VAL 2" }, { "zone_step", i == 4 ? "absorb" : "none" } }) >= 0;
        CHECK(n == 1 && gh.tr.sigs[0].t == T(10, 4), "gate: only while the zone is being worked"); }
      CHECK(h.tr.values.size() == 20 && h.tr.values[0] == 2.5, "value with an x suffix parsed");
      CHECK(h.tr.sigs[0].inputs.find("zone_id=VAL 5000.25") != std::string::npos, "reasons from the facts");
      std::map<std::string, std::string> kv{ { "zone_x", "2.5x" }, { "zone_side", "resistance" }, { "zone_id", "VAH 5010" } };
      CHECK(sa::fromFacts(h, sp, "ESZ6", 60, "c7", "ES", 200, sa::Bar{ T(10, 19), 10, 11, 9, 10 }, kv) == -1, "same bar again -> nothing");
      sa::Spec ea; ea.valueKey = "buy_x"; ea.sideKey = "spike"; ea.confKey = "p";
      sa::Hook g; g.ind = "AGG"; g.dir = "/tmp/claude-0/sa/hook";
      for (int i = 0; i < 5; ++i) sa::fromFacts(g, ea, "ESZ6", 60, "c7", "ES", 1, sa::Bar{ T(10, 0) + i * 60, 10, 11, 9, 10 }, { { "spike", i % 2 ? "sell" : "none" }, { "buy_x", "1.2" }, { "p", "0.62" } });
      CHECK(g.tr.sigs.size() == 2 && g.tr.sigs[0].side == -1 && g.tr.sigs[0].base == 62, "EACH_BAR: a signal on each side bar, conf 0-1 -> 0-100");
      { sa::Spec bs; bs.sideKey = "spike"; bs.sideMap = { { "bid_leaving", -1 }, { "ask_leaving", 1 } }; sa::Hook bh; bh.ind = "DEF"; bh.dir = g.dir;
        sa::fromFacts(bh, bs, "ESZ6", 60, "c7", "ES", 1, sa::Bar{ T(10, 0), 1, 2, 0, 1 }, { { "spike", "bid_leaving" } });
        sa::fromFacts(bh, bs, "ESZ6", 60, "c7", "ES", 1, sa::Bar{ T(10, 1), 1, 2, 0, 1 }, { { "spike", "bid_arriving" } });
        CHECK(bh.tr.sigs.size() == 2 && bh.tr.sigs[0].side == -1 && bh.tr.sigs[1].side == 1, "side map before the side words"); }
      { sa::Spec rs; rs.valueKey = "stage"; rs.sideKey = "turn"; rs.onChange = true; rs.rearm = true; sa::Hook rh; rh.ind = "RS"; rh.dir = g.dir;
        const char* seq[] = { "none", "up", "up", "none", "up", "down" };
        for (int i = 0; i < 6; ++i) sa::fromFacts(rh, rs, "ESZ6", 60, "c7", "ES", 1, sa::Bar{ T(10, 0) + i * 60, 1, 2, 0, 1 }, { { "turn", seq[i] }, { "stage", "3" } });
        CHECK(rh.tr.sigs.size() == 3, "rearm: up, (up held), none, up again, down = 3 signals"); }
      { sa::Spec vs; vs.sideKey = "pos"; vs.onChange = true; vs.rearm = true; vs.sideMap = { { "above2", -1 }, { "below2", 1 }, { "above1", 0 }, { "above", 0 } };
        sa::Hook vh; vh.ind = "VWAP"; vh.dir = g.dir; const char* seq[] = { "above1", "above2", "above2", "above1", "above2", "above" };
        for (int i = 0; i < 6; ++i) sa::fromFacts(vh, vs, "ESZ6", 60, "c7", "ES", 1, sa::Bar{ T(10, 0) + i * 60, 1, 2, 0, 1 }, { { "pos", seq[i] } });
        CHECK(vh.tr.sigs.size() == 2 && vh.tr.sigs[0].side == -1, "rearm on a no-side bar: band touch, leave, touch again = 2"); }
      { sa::Spec cs; cs.sideKey = "cob"; cs.confKey = "cob_odds"; cs.allowLate = true; sa::Hook ch; ch.ind = "COB"; ch.dir = g.dir;
        sa::fromFacts(ch, cs, "ESZ6", 60, "c7", "ES", 1, sa::Bar{ T(10, 0), 1, 2, 0, 1 }, { { "cob", "none" } });
        sa::fromFacts(ch, cs, "ESZ6", 60, "c7", "ES", 2, sa::Bar{ T(10, 0), 1, 2, 0, 1 }, { { "cob", "bull" }, { "cob_odds", "0.58" } });
        sa::fromFacts(ch, cs, "ESZ6", 60, "c7", "ES", 3, sa::Bar{ T(10, 0), 1, 2, 0, 1 }, { { "cob", "bull" }, { "cob_odds", "0.58" } });
        sa::fromFacts(ch, cs, "ESZ6", 60, "c7", "ES", 4, sa::Bar{ T(9, 59), 1, 2, 0, 1 }, { { "cob", "bear" } });
        CHECK(ch.tr.sigs.size() == 1 && ch.tr.sigs[0].base == 58 && ch.tr.bars.size() == 1, "allowLate: the same bar's late record counts once; an older bar never"); }
      sa::Spec inv = ea; inv.invert = true; sa::Hook q; q.ind = "X"; q.dir = g.dir;
      sa::fromFacts(q, inv, "ESZ6", 60, "c7", "ES", 1, sa::Bar{ T(10, 0), 1, 2, 0, 1 }, { { "spike", "sell" } }); CHECK(q.tr.sigs[0].side == 1 && q.tr.sigs[0].base == 50, "invert; no conf -> 50"); }

    std::printf("test_self: %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
