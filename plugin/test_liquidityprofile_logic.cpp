// test_liquidityprofile_logic.cpp -- logic tests for lsLiquidityProfile (no Investor/RT needed).
//   g++ -std=c++17 -Wall -Wextra -I. test_liquidityprofile_logic.cpp -o t && ./t
#include "LiquidityProfileLogic.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) ++passes; else { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)
using namespace lqp;
typedef std::vector<std::pair<double, double>> Lv;

// a CL-like book: bids 70.00 down, asks 70.01 up, 1 tick = 0.01, `levels` levels each side, base size 10 + extra at chosen ticks
static void book(Engine& e, Ms t, double bestBid, int levels, std::map<long long, double> extraBid = {}, std::map<long long, double> extraAsk = {}) {
    Lv b, a;
    for (int i = 0; i < levels; ++i) {
        const double pb = bestBid - i * 0.01, pa = bestBid + 0.01 + i * 0.01;
        const long long tb = toTick(pb, 0.01), ta = toTick(pa, 0.01);
        b.push_back(std::make_pair(pb, 10.0 + (extraBid.count(tb) ? extraBid[tb] : 0)));
        a.push_back(std::make_pair(pa, 10.0 + (extraAsk.count(ta) ? extraAsk[ta] : 0)));
    }
    e.onBook(t, b, a);
}
static Engine fresh() { Engine e; e.P.tick = 0.01; e.P.hwMin = 2; e.P.hwMax = 2; e.P.persistMs = 30000; return e; }

static void test_pull_vs_execution() {
    Engine e = fresh();
    book(e, 0, 70.00, 10);
    e.onTrade(500, 70.00, 0, 4);          // 4 sold into the 70.00 bid
    book(e, 1000, 70.00, 10, {{7000, -4}});   // 70.00 now 6: all of the drop was executed -> no pull
    double pulled = 0, ex = 0; for (auto& f : e.flow) { pulled += f.pulled; ex += f.executed; }
    CHECK(ex == 4 && pulled == 0, "a drop explained by trades is executed, not pulled");
    book(e, 2000, 70.00, 10, {{7000, -9}});   // 70.00 now 1 with no trade -> 5 pulled
    pulled = 0; for (auto& f : e.flow) pulled += f.pulled;
    CHECK(pulled == 5, "a drop with no trade is pulled");
}

static void test_window_edge_is_not_a_pull() {
    Engine e = fresh();
    book(e, 0, 70.00, 10);                 // bids 69.91..70.00
    book(e, 1000, 70.05, 10);              // price moved up 5 ticks: bids 69.96..70.05, 69.91..69.95 scrolled out of the window
    double pulled = 0; for (auto& f : e.flow) if (f.side == BID && f.px < 6996) pulled += f.pulled;
    CHECK(pulled == 0, "levels that leave a 10-level window are not counted as pulled");
}

static void test_refill_is_counted() {
    Engine e = fresh();
    book(e, 0, 70.00, 10);
    e.onTrade(500, 70.00, 0, 25);          // 25 traded against a 10-lot bid
    book(e, 1000, 70.00, 10);              // still 10 there -> refilled 15 + the 10 shown came back
    double refill = 0; for (auto& f : e.flow) refill += f.refill;
    CHECK(refill == 15, "traded beyond what was shown = refill");
}

static void test_bad_books_refused() {
    Engine e = fresh();
    Lv b = {{70.00, 5}}, a = {{69.99, 5}};
    CHECK(!e.onBook(0, b, a), "crossed book refused");
    CHECK(!e.onBook(0, b, Lv()), "one-sided book refused");
    CHECK(!e.haveBook, "nothing accepted");
}

// a support zone at 69.95 (+200 lots), price comes down into it
static Engine zoneSetup() {
    Engine e = fresh();
    for (Ms t = 0; t <= 40000; t += 1000) { book(e, t, 70.00, 12, {{6995, 200}}); e.onLastPrice(70.01); e.updateZones(t, 0.20, 16); }
    return e;
}

static void test_zone_found_and_durable() {
    Engine e = zoneSetup();
    const Zone* z = nullptr; for (auto& q : e.zones) if (q.side == BID && q.center == 6995) z = &q;
    CHECK(z != nullptr, "the 200-lot bid is a zone");
    CHECK(z && z->lo == 6993 && z->hi == 6997, "zone = centre +- hw (2 ticks)");
    CHECK(z && z->durable(40000, 30000), "rested 30 s+ = durable");
    CHECK(e.zones.size() <= 6, "at most top 3 per side");
}

static const Mark* get(const Engine& e, const char* code) { for (auto& m : e.marks) if (m.code == code) return &m; return nullptr; }
static bool has(const Engine& e, const char* code) { for (auto& m : e.marks) if (m.code == code) return true; return false; }

// (0.4.0) no chart marks: a break / hold ends the attempt and is LOGGED for the study; the six pane signals are the only marks
static void test_eaten_break_is_logged_not_marked() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);         // arrival from above into [69.93, 69.97]
    CHECK(e.attempts.size() == 1, "arrival starts an attempt");
    const double shown = e.attempts[0].shown;
    CHECK(shown >= 200, "shown = zone size at arrival");
    e.onTrade(60000, 69.95, 0, shown);     // sellers trade 1x+ the shown size
    e.onTrade(70000, 69.92, 0, 10);        // and through the far edge
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 180000, 69.91, none, none);   // closes below 69.93
    CHECK(!has(e, "LX") && !has(e, "L?"), "no LX / L? any more");
    CHECK(e.attempts.empty(), "the attempt is closed");
    CHECK(e.attemptLog.size() == 1 && e.attemptLog[0].find("|break eaten") != std::string::npos, "logged as an eaten break for the study");
}

static void test_pulled_while_attacked_is_Pull_on_the_defense_pane() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 2);         // sellers attack (2 vs 0)...
    book(e, 42000, 70.00, 12);             // ...while the 200 lots are pulled
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 180000, 69.90, none, none);
    CHECK(has(e, "Pull"), "defenders pulled while the attackers lead = Pull");
    bool red = false; for (auto& m : e.marks) if (m.code == "Pull") red = !Engine::bullish(m);
    CHECK(!Engine::topPane("Pull") && red, "Pull is a defense-pane signal, red at a support");
    CHECK(!e.attemptLog.empty() && e.attemptLog[0].find("|break pulled") != std::string::npos, "the break is logged with its reason");
    CHECK(!e.finished.empty() && e.finished.back().pulled >= 150, "the pull is measured");
}

static void test_pulled_without_attack_is_not_Pull() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 2);         // arrival
    e.onTrade(45000, 69.96, 10, 0);        // then buyers out-trade the sellers in the zone
    book(e, 42000, 70.00, 12);             // the size is pulled, but nobody is attacking it
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 180000, 69.96, none, none);
    CHECK(!has(e, "Pull"), "pulling needs the attackers ahead (Rassul's pulling + aggression)");
}

static void test_exhaustion_is_Exh_on_the_aggression_pane() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 30);        // heavy selling in the first half...
    e.onTrade(60000, 69.96, 0, 30);
    e.onTrade(200000, 69.96, 0, 1);        // ...almost none in the second half
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(2, 2000, 240000, 70.00, none, none);     // 3 min+ after arrival, close above the zone
    CHECK(has(e, "Exh") && Engine::topPane("Exh") && Engine::bullish(e.marks[0]), "sellers exhausted at a support = Exh, green, aggression pane");
    CHECK(e.attemptLog.size() == 1 && e.attemptLog[0].find("|hold Exh") != std::string::npos, "the hold is logged with Exh");
}

static void test_Exh_prints_under_half_with_its_number() {
    Engine e = zoneSetup(); e.P.exhaust = 0.2;              // the market's confirmed level only decides whether the host draws it
    e.onTrade(41000, 69.97, 0, 30); e.onTrade(60000, 69.96, 0, 30);
    e.onTrade(200000, 69.96, 0, 20);                        // 2nd half 20 / 1st 60 = 0.33: dried up at 0.5, not at 0.2
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(2, 2000, 240000, 70.00, none, none);
    const Mark* m = get(e, "ew");
    CHECK(!has(e, "Exh") && m && m->value > 0.3 && m->value < 0.35, "(0.7.0) 0.33 is not under this market's 0.2 line: no e yet, e? (the watch: under twice the line) with its number");
}

static void test_hold_with_delta_absorption_is_logged() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 10);
    e.onTrade(150000, 69.96, 0, 10);       // steady selling: no exhaustion
    std::vector<std::pair<Tick, Tick>> dpBid = {{6994, 6996}}, none;
    e.onBarClose(2, 2000, 240000, 70.00, dpBid, none);
    CHECK(e.marks.size() == 1 && e.marks[0].code == "Held" && e.marks[0].reasons.find("Absorbed: sellers hit the bid") == 0, "(0.7.0) only the plain-words reason is recorded for the hold");
    CHECK(e.attemptLog.size() == 1 && e.attemptLog[0].find("|hold A|") != std::string::npos, "the Delta Profile A is logged for the study");
}

static void test_signal_never_repeats_or_changes() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 30); e.onTrade(200000, 69.96, 0, 1);
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 230000, 69.96, none, none);
    const size_t n = e.marks.size();
    e.onBarClose(2, 2000, 400000, 69.96, none, none);     // later bars cannot add the same signal again
    CHECK(n == 1 && e.marks.size() == 1 && e.marks[0].code == "Exh" && e.marks[0].barTime == 1000, "a decided signal never changes or repeats");
}

static void test_record_round_trip() {
    Mark m; m.code = "Exh"; m.reasons = ""; m.side = BID; m.barTime = 1791700000; m.lo = 6993; m.hi = 6997; m.shown = 210; m.eaten = 0.43;
    m.stayed = 0.82; m.exhaust = 0.31; m.resil = 0.9; m.tA = 41000; m.tDecided = 240000;
    Mark r; CHECK(Engine::parse(Engine::line(m, 0.01), r, 0.01), "the record line parses");
    CHECK(r.code == "Exh" && r.lo == 6993 && r.hi == 6997 && r.barTime == 1791700000, "fields survive a restart");
    { Mark q = m; q.code = "Pull"; Mark z; CHECK(Engine::parse(Engine::line(q, 0.01), z, 0.01) && z.code == "Pull", "Pull round-trips"); }
    for (const char* c : {"LoF", "HiB", "RpB", "RpO"}) { Mark q = m; q.code = c; Mark z; CHECK(!Engine::parse(Engine::line(q, 0.01), z, 0.01), "(0.5.0) a removed code in an old record is not drawn again"); }
    Mark old = m; old.code = "L?"; Mark z; CHECK(!Engine::parse(Engine::line(old, 0.01), z, 0.01), "a retired code (L? / LX) is not drawn again");
    Mark bad; CHECK(!Engine::parse("x|y", bad, 0.01), "a torn line is rejected");
    std::string l = Engine::line(m, 0.01); l.replace(l.find("|0.43|"), 6, "|nan|");
    CHECK(!Engine::parse(l, bad, 0.01), "a non-finite number is rejected");
    std::string l2 = Engine::line(m, 0.01); l2.replace(0, 10, "17917abc00");
    CHECK(!Engine::parse(l2, bad, 0.01), "junk inside a number is rejected");
}

static void test_half_width_rule() {
    Params p; p.tick = 0.01; p.k = 0.25; p.hwMin = 4; p.hwMax = 8;
    CHECK(halfWidth(p, 0.21) == 5, "CL: 0.25 x 21 ticks = 5");
    CHECK(halfWidth(p, 0.05) == 4 && halfWidth(p, 1.0) == 8, "clamped to 4..8");
}

static void test_norms_fallback() {
    Norms n; n.median[16] = 80;
    CHECK(n.at(16) == 80, "the nightly slot median is used");
    CHECK(n.at(17) == 0, "no slot and no session samples = unknown (no x drawn)");
    for (int i = 0; i < 40; ++i) n.sample(50 + i);
    CHECK(n.at(17) > 0, "session median once 30+ samples");
}

static void test_only_Exh_and_Pull_are_signals() {
    // (0.5.0, Rassul 2026-10-11 "remove whatever doesn't work and keep what works") only the confirmed ideas are pane signals
    CHECK(Engine::paneCode("Exh") && Engine::paneCode("Pull"), "Exh and Pull are signals");
    CHECK(!Engine::paneCode("LoF") && !Engine::paneCode("HiB") && !Engine::paneCode("RpB") && !Engine::paneCode("RpO") && !Engine::paneCode("Stop"), "LoF / HiB / RpB / RpO / Stop are not");
    CHECK(Engine::topPane("Exh") && !Engine::topPane("Pull"), "Exh on the aggression pane, Pull on the defense pane");
}
static void test_pull_not_replaced_reads_PULL() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);
    book(e, 42000, 70.00, 12, {{6995, 60}});      // 140 of the 200 left without trading, nothing new came
    Readout r = e.readout(43000);
    CHECK(r.active && r.pulling && !r.depleting, "half+ of the shown size pulled and not replaced = PULL?");
    CHECK(r.refill >= 0 && r.refill < 0.5, "refill is low");
}
static void test_eaten_and_replaced_reads_holding() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);
    e.onTrade(41500, 69.95, 0, 150);              // 150 eaten at 69.95 ...
    book(e, 42000, 70.00, 12, {{6995, 200}});     // ... and the 200 are back: replaced
    Readout r = e.readout(43000);
    CHECK(r.active && !r.pulling && !r.depleting, "eaten but replaced is not a warning");
    CHECK(r.refill >= 0.9, "refill near 1x");
}
static void test_eaten_not_replaced_reads_depleting() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);
    e.onTrade(41500, 69.95, 0, 260);              // more than the shown size (250) eaten ...
    book(e, 42000, 70.00, 12, {{6995, -10}});     // ... and nothing left there
    Readout r = e.readout(43000);
    CHECK(r.active && r.depleting && !r.pulling, "eaten 1x+ and not replaced = DEPLETING?");
}
static void test_story_defense_then_offense() {
    Engine e = zoneSetup();
    std::vector<std::pair<Tick, Tick>> none;
    e.formBar = 10; e.curBar = 9;
    e.onTrade(41000, 69.97, 0, 40);               // bar 10: sellers attack the zone
    e.onTrade(60000, 69.96, 5, 30);
    e.onBarClose(10, 1000, 180000, 69.96, none, none);
    e.formBar = 11;
    e.onTrade(200000, 69.96, 90, 3);              // bar 11: buyers' total since arrival passes the sellers' (0.3.0: the cumulative cross), sellers' pace collapsed
    e.onBarClose(11, 2000, 360000, 69.96, none, none);
    CHECK(!e.attempts.empty() && e.attempts[0].barO == 11, "defense first (sellers led since arrival), then the buyers' total crosses above in bar 11 = O (logged)");
    CHECK(!has(e, "LoF") && !has(e, "HiB"), "(0.5.0) the cross is logged for the study, never drawn");
    CHECK(!e.attempts.empty() && e.attempts[0].barE >= 10 && e.attempts[0].barE <= 11, "the sellers' pace halved by bar 11 = E");
    CHECK(!e.attempts.empty() && e.attempts[0].flowBars[10].attack == 70 && e.attempts[0].flowBars[11].defend == 90, "per-bar attack / defend kept for the panes");
    e.formBar = 12;
    e.onBarClose(12, 3000, 540000, 70.00, none, none);  // closes back out: decided
    CHECK(!e.attemptLog.empty() && e.attemptLog.back().find("|E>O") != std::string::npos, "the story is logged with its order (E, then O)");
}
static void test_double_crossover() {
    Engine e = zoneSetup();
    std::vector<std::pair<Tick, Tick>> none;
    e.formBar = 10; e.curBar = 9;
    e.onTrade(41000, 69.97, 0, 40);
    book(e, 42000, 70.00, 12, {{6995, 120}});     // bar 10: 80 pulled, nothing new - pulled leads
    e.onTrade(60000, 69.96, 2, 30);               // sellers lead the trading
    e.onBarClose(10, 1000, 180000, 69.96, none, none);
    CHECK(!e.attempts.empty() && e.attempts[0].pulledFirst && e.attempts[0].defendedFirst, "both lines start on the losing side");
    e.formBar = 11;
    book(e, 190000, 70.00, 12, {{6995, 300}});    // bar 11: 180 new size comes in - the bids' net passes the offers' (0.3.0)
    e.onTrade(200000, 69.96, 80, 4);              // and the buyers' total passes the sellers'
    e.onBarClose(11, 2000, 360000, 69.96, none, none);
    CHECK(!e.attempts.empty() && e.attempts[0].barR == 11 && e.attempts[0].barO == 11 && !has(e, "LoF") && !has(e, "RpB"), "both crosses in bar 11 are logged (O, R), nothing drawn (0.5.0)");
    CHECK(e.attempts.empty() || e.attempts[0].barX < 0, "no X any more");
    e.formBar = 12;
    e.onBarClose(12, 3000, 540000, 70.00, none, none);
    CHECK(!has(e, "L?") && !has(e, "X"), "no L? / X on the chart after the crosses");
}
static void test_other_side_band_is_tracked() {
    // (0.3.0) the sellers' offers in the band just above a bid zone are tracked as their own net line; offers further up are not
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 40);
    CHECK(!e.attempts.empty(), "attempt started");
    if (e.attempts.empty()) return;
    const Attempt& a0 = e.attempts[0];
    CHECK(a0.oppLo == a0.hi + 1 && a0.oppHi == a0.hi + (a0.hi - a0.lo + 1), "the offers' band = same width, just above the zone");
    const double before = e.attempts[0].oppNet;
    std::vector<std::pair<double, double>> bids, asks;
    for (int i = 0; i < 12; ++i) bids.push_back(std::make_pair(70.00 - i * 0.01, i == 5 ? 200.0 : 10.0));
    for (int i = 0; i < 12; ++i) {
        const double px = 70.01 + i * 0.01; const Tick t = toTick(px, 0.01);
        double sz = 10; if (t >= a0.oppLo && t <= a0.oppHi) sz = 2.0; if (t > a0.oppHi) sz = 50;   // offers in the band pulled, far offers added
        asks.push_back(std::make_pair(px, sz));
    }
    e.onBook(50000, bids, asks);
    CHECK(e.attempts[0].oppNet == before - 16, "8 offers pulled at each of the band's 2 prices = the sellers' line goes down 16; offers further up are not counted");
}
// (0.5.0) old orders: the size at a price that never dipped during the last 10 min while the price stayed in the depth window
static Engine aged(std::map<long long, double> at300 = {}, Ms upTo = 610000) {
    Engine e = fresh();
    for (Ms t = 0; t <= upTo; t += 5000) book(e, t, 70.00, 12, t == 300000 && !at300.empty() ? at300 : std::map<long long, double>{{6995, 200}});
    return e;
}
static void test_old_orders() {
    { Engine e = aged(); CHECK(std::fabs(e.zoneOld(BID, 6993, 6997, 610000) - 1.0) < 1e-9, "a zone untouched for 10 min+ is all old"); }
    { Engine e = aged({}, 300000); CHECK(e.zoneOld(BID, 6993, 6997, 300000) < 0, "seen for only 5 min: age not known yet (no mark)"); }
    { Engine e = aged(); book(e, 615000, 70.00, 12, {{6995, 400}});
      const double o = e.zoneOld(BID, 6993, 6997, 615000);
      CHECK(o > 0.55 && o < 0.56, "200 new lots join: old = 250 / 450 = 0.56"); }
    { Engine e = aged({{6995, 40}});
      const double o = e.zoneOld(BID, 6993, 6997, 610000);
      CHECK(o > 0.35 && o < 0.37, "the level dipped to 50 at 5 min: only 50 of its 210 can be old (90 / 250 = 0.36)"); }
    { Engine e = aged(); book(e, 612000, 69.90, 12); book(e, 614000, 70.00, 12, {{6995, 200}});
      CHECK(e.zoneOld(BID, 6993, 6997, 614000) < 0, "the zone left the depth window: its clock restarts"); }
    { Engine e = aged(); book(e, 700000, 70.00, 12, {{6995, 200}});
      CHECK(e.zoneOld(BID, 6993, 6997, 700000) < 0, "a feed gap of a minute+ restarts every clock"); }
    { Engine e = fresh();
      for (Ms t = 0; t <= 620000; t += 5000) { book(e, t, 70.00, 12, {{6995, 200}}); e.onLastPrice(70.01); e.updateZones(t, 0.20, 16); }
      e.onTrade(621000, 69.97, 0, 5);
      CHECK(!e.attempts.empty() && e.attempts[0].oldShare > 0.9, "the zone's old share at arrival is kept on the attempt");
      std::vector<std::pair<Tick, Tick>> none; e.onBarClose(1, 1000, 640000, 69.90, none, none);
      CHECK(!e.attemptLog.empty() && e.attemptLog[0].substr(e.attemptLog[0].rfind('|')) == "|1", "and logged (old_x) for the study");
      CHECK(Engine::attemptHeader().substr(Engine::attemptHeader().rfind('|')) == "|old_x", "the log header names it"); }
}
// (0.6.0) the pane statistics, measured as the nightly study measures them
static void test_stats_in_process_order() {
    Engine e = zoneSetup();                                   // a 200-lot bid zone [69.93, 69.97] (5 ticks)
    for (int b = 0; b < 20; ++b) e.setBar(b, 100, 100, 10);   // normal: sellers 100 lots per bar over 10 ticks = 10 per tick per bar
    e.formBar = 20; e.curBar = 19;
    e.onTrade(41000, 69.97, 0, 50);                           // arrival in bar 20: sellers 50 + 70 = 120 into the 5-tick zone = 24 per tick
    e.onTrade(42000, 69.95, 0, 70);
    Stats st; CHECK(e.latest(43000, st) && st.side == BID, "the live attempt's statistics");
    CHECK(st.attack > 2.39 && st.attack < 2.41, "Attack = 24 per tick / 10 normal = 2.4x");
    CHECK(st.eaten > 0 && st.eaten <= 1, "Eaten is a share of everything there (0..1)");
    Engine f = zoneSetup(); for (int b = 0; b < 10; ++b) f.setBar(b, 100, 100, 10);
    f.formBar = 20; f.onTrade(41000, 69.97, 0, 50);
    Stats s2; f.latest(43000, s2); CHECK(s2.attack < 0, "under 15 known bars: no Attack (never a guess)");
}
static void test_eaten_counts_new_arrivals() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);
    const double shown = e.attempts[0].shown;                 // ~250
    e.onTrade(41500, 69.95, 0, 100);                          // 100 eaten at 69.95 ...
    book(e, 42000, 70.00, 12, {{6995, 390}});                 // ... and 300 new lots arrived there (200 - 100 + 300 = 400 showing)
    Stats st; e.latest(43000, st);
    const double tot = shown + e.attempts[0].added + e.attempts[0].refill;
    CHECK(e.attempts[0].added >= 290 && std::fabs(st.eaten - 105.0 / tot) < 1e-9, "Eaten = attackers / (shown + new + refills): new arrivals keep it low");
    CHECK(st.eaten < 0.2, "eaten a lot but the zone stays: a small share of everything there");
}
static void test_pulled_and_rebuilt() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);
    book(e, 42000, 70.00, 12, {{6995, 60}});                  // 140 of the 200 pulled
    Stats a; e.latest(43000, a);
    CHECK(a.pulled > 0.5 && a.rebuilt == 0, "Pulled = pulled / shown; the zone lost size and nothing came back = Rebuilt 0");
    book(e, 44000, 70.00, 12, {{6995, 260}});                 // all of it back and more
    Stats b; e.latest(45000, b);
    CHECK(b.rebuilt > 1.0, "Rebuilt > 1x = more came back than was lost");
}
// (0.7.0) e? -> e, p? -> p, t, the reasons and the warnings
static void test_e_watch_then_e() {
    Engine e = zoneSetup(); e.P.exhaust = 0.3;
    e.onTrade(41000, 69.97, 0, 30); e.onTrade(60000, 69.96, 0, 30);
    e.onTrade(170000, 69.96, 0, 30);                         // bar 2 close at 240 s: 2nd half 30 / 1st 60 = 0.5 -> under 0.6 (twice 0.3): e?
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(2, 2000, 240000, 69.96, none, none);
    CHECK(has(e, "ew") && !has(e, "Exh"), "e? first: slowing toward the line");
    e.onBarClose(3, 3000, 600000, 69.96, none, none);         // by 600 s the 2nd half (300-600 s) has nothing: 0x -> under 0.3: e
    CHECK(has(e, "Exh") && get(e, "Exh")->barTime == 3000, "then e on the bar it crossed the line (the e? record stays: both are history)");
}
static void test_p_watch_then_p() {
    Engine e = zoneSetup(); e.P.pullShare = 0.6;
    e.onTrade(41000, 69.97, 0, 5);                            // sellers lead
    book(e, 42000, 70.00, 12, {{6995, 120}});                 // 80 of ~250 pulled = 0.32 -> over half of 0.6: p?
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 120000, 69.96, none, none);
    CHECK(has(e, "pw") && !has(e, "Pull"), "p? at half-way to the line");
    book(e, 130000, 70.00, 12, {{6995, 0}});                  // the rest pulled: past the line
    e.onBarClose(2, 2000, 180000, 69.96, none, none);
    CHECK(has(e, "Pull"), "then p");
}
static void test_trapped_t() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);
    e.onTrade(50000, 69.90, 0, 20);                           // sellers trade BELOW the zone's far edge (69.93)
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 120000, 69.95, none, none);         // back inside: not yet
    CHECK(!has(e, "t"), "no t while price is still in or below the zone");
    e.onBarClose(2, 2000, 240000, 70.00, none, none);         // a close back above the zone after the push below
    CHECK(has(e, "t") && get(e, "t")->barTime == 2000 && Engine::bullish(*get(e, "t")), "t on the close back above, green at a support");
}
static void test_reason_uses_only_confirmed_lines() {
    Engine e = zoneSetup();
    for (int b = 0; b < 20; ++b) e.setBar(b, 100, 100, 10);
    e.formBar = 20; e.curBar = 19;
    e.onTrade(41000, 69.97, 0, 60); e.onTrade(60000, 69.96, 0, 60);   // 120 into 5 ticks = 24 per tick vs 10 = 2.4x
    e.onTrade(200000, 69.96, 0, 3);
    std::vector<std::pair<Tick, Tick>> none;
    e.L.attack = 2.0; e.L.exh = 0.5;                          // only Attack and Exh confirmed for this market
    e.onBarClose(20, 2000, 240000, 70.00, none, none);
    const Mark* m = get(e, "Held");
    CHECK(m && m->reasons.find("Absorbed: sellers hit the bid 2.5x normal, then exhausted (") == 0, "the reason in plain words with the confirmed numbers");
    CHECK(m && m->reasons.find("refilled") == std::string::npos && m->reasons.find("; buyers held") != std::string::npos, "Rebuilt not confirmed -> not used");
    Engine f = zoneSetup(); f.onTrade(41000, 69.97, 0, 60); f.onTrade(60000, 69.96, 0, 60);
    f.onBarClose(2, 2000, 240000, 70.00, none, none);
    const Mark* n = get(f, "Held");
    CHECK(n && n->reasons.find("x normal") == std::string::npos, "no confirmed lines -> no numbers in the sentence");
    // (2026-10-22) never dereference a missing mark: the checks above already FAIL when there is no "Held" mark (the suite used to crash here)
    CHECK(m && Engine::line(*m, 0.01).find("Absorbed") != std::string::npos, "the reason is recorded (no repaint after a restart)");
    Mark r; CHECK(m && Engine::parse(Engine::line(*m, 0.01), r, 0.01) && r.reasons == m->reasons && r.code == "Held", "and read back");
}
static void test_warnings_priority() {
    Engine e = zoneSetup();
    for (int b = 0; b < 20; ++b) e.setBar(b, 100, 100, 10);
    e.formBar = 20; e.L.attack = 2.0; e.L.exh = 0.3; e.L.pulled = 0.6; e.L.eaten = 0.9;
    e.onTrade(41000, 69.97, 0, 60); e.onTrade(42000, 69.96, 0, 60);   // heavy (2.4x), not slowing yet
    Warn w;
    CHECK(e.warning(43000, w, 0.01) && w.name == "BUYERS HOLDING" && w.favour == 0, "heavy, not slowing, no pulling: BUYERS HOLDING (grey)");
    book(e, 44000, 70.00, 12, {{6995, 30}});                  // the bids pulled past the line while sellers lead
    CHECK(e.warning(45000, w, 0.01) && w.name == "BREAKDOWN RISK" && w.favour == -1 && w.detail.find("buyers pulling their bids") != std::string::npos,
          "heavy + pulling + no exhaustion: BREAKDOWN RISK (red)");
    Engine g = zoneSetup(); Warn v;
    g.onTrade(41000, 69.97, 0, 60);
    CHECK(!g.warning(43000, v, 0.01), "no confirmed lines: no warning (never a guess)");
}

// (0.8.0) s = COUNTER: after e, the defenders out-trade the attackers near the zone at the Counter line x their normal pace per tick
static Engine counterSetup() {
    Engine e = zoneSetup(); e.P.exhaust = 0.3; e.P.counterX = 1.25; e.L.counter = 1.25;
    for (int b = 0; b < 20; ++b) e.setBar(b, 100, 100, 10, 7005, 6996);   // buyers' normal: 100 a bar over 10 ticks = 10 per tick
    e.formBar = 20; e.curBar = 19;
    e.onTrade(41000, 69.97, 0, 30); e.onTrade(60000, 69.96, 0, 30);         // sellers attack the 69.93-69.97 bids, then stop
    return e;
}
static void test_counter_s_after_e() {
    Engine e = counterSetup(); std::vector<std::pair<Tick, Tick>> none;
    e.setBar(20, 50, 100, 5, 6997, 6993); e.onBarClose(20, 2000, 240000, 69.96, none, none);   // e (nothing in the 2nd half)
    CHECK(has(e, "Exh") && !has(e, "s"), "e first, no s while the sellers still out-trade the buyers");
    e.setBar(21, 400, 100, 10, 7005, 6996); e.onBarClose(21, 3000, 420000, 70.02, none, none);  // buyers 400 vs 100 over 10 ticks = 4.0x
    const Mark* m = get(e, "s");
    CHECK(m && m->barTime == 3000 && std::fabs(m->value - 4.0) < 1e-9, "s on the bar the buyers stepped in, with its pace (4.0x)");
    CHECK(m && m->reasons == "Buyers stepped in: 4.0x normal, out-traded sellers" && Engine::bullish(*m), "its reason in plain words, green at a support");
    Mark r; CHECK(m && Engine::parse(Engine::line(*m, 0.01), r, 0.01) && r.code == "s" && r.reasons == m->reasons, "s is recorded and read back (no repaint)");
    Warn w;
    CHECK(e.warning(421000, w, 0.01) && w.name == "BUYERS STEPPING IN" && w.favour == 1 && w.detail == "sellers exhausted (0.0x); buyers now lifting offers 4.0x normal",
          "BUYERS STEPPING IN while the touch was just decided as held");
    CHECK(!e.warning(420000 + 3 * 180000, w, 0.01), "the STEPPING IN warning ends 2 bars later");
    Stats st; CHECK(e.latest(421000, st) && std::fabs(st.counter - 4.0) < 1e-9, "the Counter readout is the buyers' pace on the latest bar");
}
static void test_counter_s_on_the_bars_after_the_hold() {
    Engine e = counterSetup(); std::vector<std::pair<Tick, Tick>> none;
    e.setBar(20, 50, 100, 5, 6997, 6993); e.onBarClose(20, 2000, 240000, 70.02, none, none);   // e, and the close back above: held
    CHECK(has(e, "Exh") && e.finished.size() == 1 && e.finished[0].end.compare(0, 4, "hold") == 0, "e and the touch decided as held on the same bar");
    e.setBar(21, 90, 60, 6, 7003, 6998); e.onBarClose(21, 3000, 420000, 70.02, none, none);    // buyers 90 vs 60 over 6 ticks = 1.5x, near the zone
    const Mark* m = get(e, "s");
    CHECK(m && m->barTime == 3000 && std::fabs(m->value - 1.5) < 1e-9, "s on a bar after the hold (the buyers stepping in after the close back out)");
}
static void test_counter_needs_e_near_and_the_line() {
    std::vector<std::pair<Tick, Tick>> none;
    { Engine e = counterSetup();
      e.onTrade(200000, 69.96, 0, 40);                                      // sellers keep going: no e
      e.setBar(20, 400, 100, 10, 7005, 6996); e.onBarClose(20, 2000, 240000, 69.96, none, none);
      CHECK(!has(e, "Exh") && !has(e, "s"), "no s before e (stepping in only counts after the attackers ran out)"); }
    { Engine e = counterSetup();
      e.setBar(20, 50, 100, 5, 6997, 6993); e.onBarClose(20, 2000, 240000, 69.96, none, none);
      e.setBar(21, 400, 100, 10, 7030, 7020); e.onBarClose(21, 3000, 420000, 69.96, none, none);   // the bar traded 23+ ticks above the zone
      CHECK(!has(e, "s"), "no s on a bar away from the zone"); }
    { Engine e = counterSetup();
      e.setBar(20, 50, 100, 5, 6997, 6993); e.onBarClose(20, 2000, 240000, 69.96, none, none);
      e.setBar(21, 110, 100, 10, 7005, 6996); e.onBarClose(21, 3000, 420000, 69.96, none, none);   // out-traded, but 1.1x < 1.25x
      Stats st; e.latest(421000, st);
      CHECK(!has(e, "s") && std::fabs(st.counter - 1.1) < 1e-9, "no s under the line, but the Counter readout still shows 1.1x"); }
}

// (0.10.0) a zone is drawn only when it stands out from the book BEHIND it; hysteresis; a level is confirmed at 2.5x+
static void test_prominence_show_and_confirm() {
    Engine e = fresh(); e.P.requireShown = true;
    // 60 levels each side, base 10 per level; one 200-lot bid at 69.95 (tick 6995): band 5 ticks = 250 vs 50 behind -> 5x
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 200}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = nullptr; for (auto& q : e.zones) if (q.side == BID && q.center == 6995) z = &q;
    CHECK(z && z->behindOk && z->rank == 1 && z->bx > 4.9 && z->bx < 5.1, "bx = band / median of the 8 bands behind (250 / 50 = 5x)");
    CHECK(z && z->shown, "a 5x rank-1 durable zone is shown");
    CHECK(z && z->edgeLo == z->lo && z->edgeHi == z->hi, "real edges: size at every price of the band -> the band edges");
    CHECK(e.confirming(6995, 7000) == z, "a level inside a shown 2.5x+ support below price is confirmed");
    CHECK(e.confirming(6995, 6990) == nullptr, "a support above price never confirms");
    CHECK(e.confirming(6980, 7000) == nullptr, "a level outside the band is not confirmed");
    // the plain book (no stand-out) is never shown: every other bid zone is 1x
    int shownPlain = 0; for (auto& q : e.zones) if (q.shown && q.center != 6995) shownPlain++;
    CHECK(shownPlain == 0, "uniform book: no zone shown (big bands everywhere are not walls)");
    // hysteresis: drop to 1.8x (band 90 vs 50) -> stays; drop to 1.4x for > 10 s -> removed
    const long long zid = z ? z->id : -1;
    for (int s = 41; s <= 60; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 40}}); e.updateZones(s * 1000, 0.08, 0); }
    z = nullptr; for (auto& q : e.zones) if (q.id == zid) z = &q;   // the same zone (its centre may drift a tick)
    CHECK(z && z->shown && z->bx > 1.7 && z->bx < 1.9, "1.8x after showing: kept (hysteresis at 1.7x)");
    for (int s = 61; s <= 80; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 20}}); e.updateZones(s * 1000, 0.08, 0); }
    z = nullptr; for (auto& q : e.zones) if (q.id == zid) z = &q;
    // (0.14.0) this zone sits 3 ticks under price (near band 8 ticks): it is a NEAR zone, kept down to 1.2x - the far 1.7x keep is
    // tested on a far zone in test_near_rule_far_unchanged
    CHECK(z && z->near && z->shown, "(0.14.0) 1.4x near price for over 10 s: kept (near keep 1.2x)");
    for (int s = 81; s <= 100; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 5}}); e.updateZones(s * 1000, 0.08, 0); }
    z = nullptr; for (auto& q : e.zones) if (q.id == zid) z = &q;
    CHECK(!z || !z->shown, "(0.14.0) below the near keep (1.1x) for over 10 s: removed");
}

static void test_new_wall_waits_and_edge_of_book() {
    Engine e = fresh(); e.P.requireShown = true;
    for (int s = 0; s <= 20; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 200}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = nullptr; for (auto& q : e.zones) if (q.side == BID && q.center == 6995) z = &q;
    CHECK(z && !z->shown, "a wall younger than 30 s is not drawn yet");
    Engine f = fresh(); f.P.requireShown = true;          // a wall near the far edge of a 12-level book: too few bands behind
    for (int s = 0; s <= 40; ++s) { book(f, s * 1000, 70.00, 12, {{6991, 200}}); f.updateZones(s * 1000, 0.08, 0); }
    const Zone* g = nullptr; for (auto& q : f.zones) if (q.side == BID && q.center == 6991) g = &q;
    CHECK(!g || (!g->behindOk && !g->shown), "fewer than 6 bands behind inside the book: never drawn");
}

// (0.11.0) an absorption read also starts at a Delta Profile A on a zone too small to be drawn; the always-on pace and near book
static void test_delta_A_starts_a_read_and_always_on_numbers() {
    Engine e = fresh(); e.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 8}}); e.updateZones(s * 1000, 0.08, 0); }   // a plain 18-lot level
    const Zone* z = nullptr; for (auto& q : e.zones) if (q.side == BID && q.lo <= 6995 && q.hi >= 6995) z = &q;
    CHECK(z && !z->shown, "fixture: the band at 69.95 is not 2x the book behind (not drawn)");
    e.onTrade(41000, 70.03, 3, 0); e.onTrade(41500, 69.95, 0, 4);
    CHECK(e.attempts.empty(), "no Delta Profile A: no read at an undrawn band");
    Engine f = fresh(); f.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(f, s * 1000, 70.00, 60, {{6995, 8}}); f.updateZones(s * 1000, 0.08, 0); }
    f.dpZone[BID].push_back(std::make_pair((Tick)6995, (Tick)6995));         // the Delta Profile's A (support) at 69.95
    f.onTrade(41000, 70.03, 3, 0); f.onTrade(41500, 69.95, 0, 4);
    CHECK(f.attempts.size() == 1 && f.attempts[0].fromDelta && f.attempts[0].dpLo == 6995, "a Delta Profile A starts the read, marked as from the Delta Profile");
    for (int b = 0; b < 20; ++b) f.setBar(b, 100, 50, 5);
    f.setBar(20, 250, 25, 5);
    CHECK(std::fabs(f.paceX(20, true) - 2.5) < 1e-9 && std::fabs(f.paceX(20, false) - 0.5) < 1e-9, "buyers 2.5x / sellers 0.5x their 20-bar average");
    CHECK(f.paceX(10, true) < 0, "under 15 bars before it: no number");
    const std::pair<double, double> nb = f.nearNow();
    CHECK(nb.first > 0 && nb.second > 0, "resting bids below / offers above price are summed");
    std::vector<std::pair<Tick, Tick>> none;
    f.onBarClose(21, 1000, 180000, 69.96, none, none);
    CHECK(f.nearBook.count(21) == 1, "each closed bar keeps the near-price book");
}

// ---- (0.11.4) review fixes
// the engine keeps the last 50 finished attempts; finTotal keeps counting so the Flow pane can record every one (before: none after the 50th)
static void test_finished_total_counts_past_50() {
    Engine e = fresh(); std::vector<std::pair<Tick, Tick>> none;
    for (int i = 0; i < 60; ++i) { Attempt a; a.side = BID; a.lo = 6990; a.hi = 6994; a.barA = i; a.tA = i * 1000; e.attempts.push_back(a); e.onBarClose(i + 25, 1000 + i, 10000000 + i, 70.00, none, none); }
    CHECK(e.finished.size() == 50 && e.finTotal == 60, "(0.11.4) 60 finished: the list keeps 50, the total says 60");
    CHECK(e.finished.back().barA == 59 && e.finished.front().barA == 10, "(0.11.4) finished[0] is number finTotal - size");
}
// a shown zone near the edge of the visible book keeps being judged on the bands that are visible (no 10 s hide / come-back flicker)
static void test_shown_zone_at_book_edge_does_not_flicker() {
    Engine e = fresh(); e.P.graceMs = 10000;
    Zone z; z.shown = true; z.bx = 3.1; z.behindN = 5; z.behindOk = false; z.rank = 1; z.born = 0;
    for (Ms t = 0; t <= 30000; t += 1000) e.showState(z, t);
    CHECK(z.shown, "(0.11.4) 5 bands behind (book window moved) but still 3.1x them: stays shown");
    Zone n = z; n.behindN = 0; n.bx = -1;
    for (Ms t = 0; t <= 30000; t += 1000) e.showState(n, t);
    CHECK(n.shown, "(0.11.4) no band behind it visible: no evidence, it stays as it is");
    Zone w = z; w.bx = 1.2;
    for (Ms t = 0; t <= 30000; t += 1000) e.showState(w, t);
    CHECK(!w.shown, "a shown zone that really weakened (1.2x the bands behind) is still removed after the grace");
    Zone f; f.shown = false; f.bx = 3.0; f.behindN = 5; f.behindOk = false; f.rank = 1; f.born = 0;
    e.showState(f, 60000);
    CHECK(!f.shown, "a zone is still only SHOWN with the full 6 bands behind it");
}
// LQ keeps its zone down to 2.1x (hysteresis) - it does not blink while the zone hovers around 2.5x
static void test_lq_confirm_hysteresis() {
    Engine e = fresh();
    Zone z; z.id = 7; z.side = BID; z.lo = 6990; z.hi = 7000; z.shown = true; z.bx = 2.3; e.zones.push_back(z);
    CHECK(e.confirming(6995, 7010) == nullptr, "a new LQ needs 2.5x");
    CHECK(e.confirming(6995, 7010, 7) == &e.zones[0], "(0.11.4) the zone that already confirmed it keeps it at 2.3x");
    e.zones[0].bx = 2.0;
    CHECK(e.confirming(6995, 7010, 7) == nullptr, "(0.11.4) ... down to 2.1x only");
    e.zones[0].bx = 2.6;
    CHECK(e.confirming(6995, 7010) == &e.zones[0], "2.5x+ confirms as before");
}
// two shown zones whose real edges overlap: only the stronger is drawn; labels never share a row span
static void test_draw_order_and_label_lanes() {
    Engine e = fresh();
    Zone a; a.id = 1; a.side = BID; a.shown = true; a.bx = 2.2; a.edgeLo = 6990; a.edgeHi = 6998;
    Zone b = a; b.id = 2; b.bx = 3.0; b.edgeLo = 6995; b.edgeHi = 7003;
    Zone c = a; c.id = 3; c.bx = 2.5; c.edgeLo = 6970; c.edgeHi = 6978;
    Zone d = a; d.id = 4; d.shown = false; d.bx = 9; d.edgeLo = 6960; d.edgeHi = 6965;
    e.zones = {a, b, c, d};
    const std::vector<size_t> o = e.drawOrder();
    CHECK(o.size() == 2 && e.zones[o[0]].id == 2 && e.zones[o[1]].id == 3, "(0.11.4) strongest first; the overlapping weaker zone and hidden zones are not drawn");
    CHECK(decimals((double)0.01f) == 2 && decimals((double)0.25f) == 2 && decimals((double)0.0005f) == 4 && decimals((double)0.00005f) == 5 && decimals(1.0) == 0 && decimals((double)0.001f) == 3,
          "(0.11.4) price decimals from IRT's float tick: CL 2, ES 2, HG 4, 6E 5, NG 3");
    Lanes L(2);
    CHECK(L.place(100, 160) == 0 && L.place(150, 200) == 1 && L.place(155, 165) == -1 && L.place(170, 220) == 0, "(0.11.4) a label goes to the first free row, none when all rows are taken");
}

// ---- (0.14.0) NEAR-price rule (Rassul 2026-10-13 "i want to see liquidity when it is close so i can see if there is any absorbtion going on")
static const Zone* zoneHolding(const Engine& e, int side, Tick px) { for (auto& q : e.zones) if (q.side == side && q.lo <= px && q.hi >= px) return &q; return nullptr; }
// near band: tick 0.01, hw 2, ATR 0.08 -> max(8, 4*2) = 8 ticks; zone 69.93-69.97 under 70.00 = 3 ticks away -> near
static void test_near_rule_rank2_shows() {
    Engine e = fresh(); e.P.requireShown = true;
    // a 1.6x band at 69.95 (80 vs 50) with a bigger band (150) at 69.85 behind it: rank 2
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }
    CHECK(e.nearTicks == 8, "(0.14.0) near band = max(round(1.0 x ATR3 / tick), 4 x hw) = 8 ticks");
    const Zone* z = zoneHolding(e, BID, 6995);
    std::printf("  near zone: bx %.2f rank %d near %d shown %d behind %d\n", z ? z->bx : -1, z ? z->rank : -1, z ? (int)z->near : -1, z ? (int)z->shown : -1, z ? z->behindN : -1);
    CHECK(z && z->near && z->rank == 2 && z->bx >= 1.55 && z->bx < 1.7, "(0.14.0) fixture: near zone at 1.6x, rank 2");
    CHECK(z && z->shown, "(0.14.0) a NEAR zone at 1.6x rank 2 is shown (no rank-1 rule near price)");
    Engine y = fresh(); y.P.requireShown = true;          // the same zone, 20 s old: not durable yet
    for (int s = 0; s <= 20; ++s) { book(y, s * 1000, 70.00, 60, {{6995, 30}, {6985, 100}}); y.updateZones(s * 1000, 0.08, 0); }
    const Zone* yz = zoneHolding(y, BID, 6995);
    CHECK(yz && yz->near && !yz->shown, "(0.14.0) a near zone still needs to rest 30 s");
    Engine w = fresh(); w.P.requireShown = true;          // 1.4x near: under the 1.5x near line
    for (int s = 0; s <= 40; ++s) { book(w, s * 1000, 70.00, 60, {{6995, 20}, {6985, 100}}); w.updateZones(s * 1000, 0.08, 0); }
    const Zone* wz = zoneHolding(w, BID, 6995);
    CHECK(wz && wz->near && !wz->shown, "(0.14.0) a near zone at 1.4x is not shown (line 1.5x)");
    // mirrored: a 1.6x offer band 3 ticks above price, rank 2
    Engine o = fresh(); o.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(o, s * 1000, 70.00, 60, {}, {{7006, 30}, {7016, 100}}); o.updateZones(s * 1000, 0.08, 0); }
    const Zone* oz = zoneHolding(o, ASK, 7006);
    CHECK(oz && oz->near && oz->rank == 2 && oz->shown, "(0.14.0) a near OFFER zone at 1.6x rank 2 is shown");
}
static void test_near_rule_far_unchanged() {
    // the same 1.6x rank-2 zone 38+ ticks under price: far -> not shown
    Engine e = fresh(); e.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 120, {{6960, 30}, {6950, 100}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneHolding(e, BID, 6960);
    std::printf("  far zone: bx %.2f rank %d near %d shown %d\n", z ? z->bx : -1, z ? z->rank : -1, z ? (int)z->near : -1, z ? (int)z->shown : -1);
    CHECK(z && !z->near && z->rank == 2 && z->bx >= 1.55 && !z->shown, "(0.14.0) the same 1.6x rank-2 zone FAR from price is not shown");
    // far 2.0x rank 1: shown (unchanged); far 1.9x rank 1: not shown
    Engine f = fresh(); f.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(f, s * 1000, 70.00, 120, {{6960, 50}}); f.updateZones(s * 1000, 0.08, 0); }
    const Zone* fz = zoneHolding(f, BID, 6960);
    CHECK(fz && !fz->near && fz->rank == 1 && fz->bx >= 2.0 && fz->shown, "(0.14.0) far 2.0x rank 1: shown as before");
    const long long fid = fz ? fz->id : -1;
    for (int s = 41; s <= 60; ++s) { book(f, s * 1000, 70.00, 120, {{6960, 40}}); f.updateZones(s * 1000, 0.08, 0); }   // 1.8x
    fz = nullptr; for (auto& q : f.zones) if (q.id == fid) fz = &q;
    CHECK(fz && fz->shown, "(0.14.0) far 1.8x: kept (far keep 1.7x unchanged)");
    for (int s = 61; s <= 80; ++s) { book(f, s * 1000, 70.00, 120, {{6960, 20}}); f.updateZones(s * 1000, 0.08, 0); }   // 1.4x
    fz = nullptr; for (auto& q : f.zones) if (q.id == fid) fz = &q;
    CHECK(!fz || !fz->shown, "(0.14.0) far 1.4x for over 10 s: removed (a far zone does not get the near keep)");
    Engine g = fresh(); g.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(g, s * 1000, 70.00, 120, {{6960, 45}}); g.updateZones(s * 1000, 0.08, 0); }
    const Zone* gz = zoneHolding(g, BID, 6960);
    CHECK(gz && !gz->near && gz->bx < 2.0 && gz->bx >= 1.8 && !gz->shown, "(0.14.0) far 1.9x rank 1: not shown (2.0x line unchanged)");
}
static void test_near_keep_and_grace() {
    Engine e = fresh(); e.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneHolding(e, BID, 6995); const long long id = z ? z->id : -1;
    CHECK(z && z->shown, "(0.14.0) fixture: near zone shown at 1.6x");
    auto get1 = [&]() -> const Zone* { for (auto& q : e.zones) if (q.id == id) return &q; return nullptr; };
    for (int s = 41; s <= 70; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 15}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }   // 1.3x for 30 s
    z = get1();
    CHECK(z && z->shown && z->bx > 1.25 && z->bx < 1.35, "(0.14.0) near zone at 1.3x for 30 s: stays (near keep 1.2x)");
    for (int s = 71; s <= 78; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 5}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }    // 1.1x for 7 s
    z = get1();
    CHECK(z && z->shown, "(0.14.0) near zone at 1.1x: still drawn inside the 10 s grace");
    for (int s = 79; s <= 90; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 5}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }    // past 10 s
    z = get1();
    CHECK(!z || !z->shown, "(0.14.0) near zone at 1.1x for over 10 s: dropped");
}
static void test_near_never_under_1x() {
    // the floor holds even if the near lines were mis-set under 1x
    Engine e = fresh(); e.P.nearShowX = 0.8; e.P.nearKeepX = 0.7;
    Zone z; z.id = 1; z.side = BID; z.near = true; z.behindOk = true; z.behindN = 8; z.rank = 1; z.bx = 0.9; z.born = 0;
    e.showState(z, 60000);
    CHECK(!z.shown, "(0.14.0) 0.9x never shows (floor 1.0x), even near and rank 1");
    z.bx = 1.0; e.showState(z, 60000);
    CHECK(z.shown, "(0.14.0) ... 1.0x passes the floor when the near line is under it (fixture check)");
    z.bx = 0.9; e.showState(z, 61000);
    CHECK(!z.shown, "(0.14.0) a shown zone measured under 1.0x is hidden at once (no grace)");
    Engine d = fresh();      // default lines: a near zone at 0.9x through a full engine run
    for (int s = 0; s <= 40; ++s) { book(d, s * 1000, 70.00, 60, {{6995, -5}}); d.updateZones(s * 1000, 0.08, 0); }
    int under = 0; for (auto& q : d.zones) if (q.shown && q.bx < 1.0) under++;
    CHECK(under == 0, "(0.14.0) no zone under 1.0x is shown in an engine run");
}
static void test_near_hysteresis_no_flicker() {
    Engine e = fresh(); e.P.requireShown = true;
    e.onLastPrice(70.00);
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneHolding(e, BID, 6995); const long long id = z ? z->id : -1;
    CHECK(z && z->near && z->shown, "(0.14.0) fixture: near zone shown");
    const Tick hi = z ? z->hi : 0;
    auto get1 = [&]() -> const Zone* { for (auto& q : e.zones) if (q.id == id) return &q; return nullptr; };
    // price oscillates so the distance runs between nearTicks + 1 and 1.5 x nearTicks (9..12 ticks): stays near, stays shown
    int nearToggles = 0, shownToggles = 0; bool lastNear = true, lastShown = true;
    for (int s = 41; s <= 120; ++s) {
        const Tick d = 9 + (s % 4);                                           // 9, 10, 11, 12
        e.onLastPrice((double)(hi + d) * 0.01);
        book(e, s * 1000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0);
        const Zone* q = get1(); if (!q) { nearToggles = 99; break; }
        if (q->near != lastNear) { ++nearToggles; lastNear = q->near; }
        if (q->shown != lastShown) { ++shownToggles; lastShown = q->shown; }
    }
    CHECK(nearToggles == 0 && shownToggles == 0, "(0.14.0) price oscillating 9-12 ticks away (between 1x and 1.5x the near band): no near / shown toggling");
    e.onLastPrice((double)(hi + 13) * 0.01); book(e, 121000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(121000, 0.08, 0);
    z = get1();
    CHECK(z && !z->near, "(0.14.0) past 1.5x the near band (13 ticks): far");
    int back = 0;
    for (int s = 122; s <= 160; ++s) {
        e.onLastPrice((double)(hi + 9 + (s % 4)) * 0.01);
        book(e, s * 1000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0);
        const Zone* q = get1(); if (q && q->near) back++;
    }
    CHECK(back == 0, "(0.14.0) once far, 9-12 ticks away does not make it near again (needs <= 8)");
    z = get1();
    CHECK(!z || !z->shown, "(0.14.0) far at rank 2 / 1.6x: hidden after the grace (far rule)");
    e.onLastPrice((double)(hi + 8) * 0.01); book(e, 161000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(161000, 0.08, 0);
    z = get1();
    CHECK(z && z->near && z->shown, "(0.14.0) back within the near band: near and shown again at once (durable already)");
}
static void test_near_candidate_always_tracked() {
    // three bigger far bands fill the top 3; the 1.6x band near price is still tracked and shown
    auto run = [](int nearPerSide) {
        Engine e = fresh(); e.P.requireShown = true; if (nearPerSide >= 0) e.P.nearPerSide = nearPerSide;   // -1 = the default
        for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 120, {{6995, 30}, {6975, 100}, {6965, 100}, {6955, 100}}); e.updateZones(s * 1000, 0.08, 0); }
        return e;
    };
    Engine e = run(-1), c = run(0);
    const Zone* z = zoneHolding(e, BID, 6995);
    std::printf("  near candidate: tracked %d bx %.2f rank %d shown %d | control tracked %d\n", z ? 1 : 0, z ? z->bx : -1, z ? z->rank : -1, z ? (int)z->shown : -1, zoneHolding(c, BID, 6995) ? 1 : 0);
    CHECK(!zoneHolding(c, BID, 6995), "(0.14.0) control: without the near rule the near band is not in the top 3 by size");
    CHECK(z && z->near && z->shown, "(0.14.0) the strongest near candidate is always tracked (and shown at 1.6x)");
    int bids = 0; for (auto& q : e.zones) if (q.side == BID) bids++;
    CHECK(bids <= e.P.topN + e.P.nearPerSide, "(0.14.0) at most topN + 2 zones tracked per side");
}
static void test_draw_order_dense_near_zones() {
    // three near bands 5 ticks apart + far bands: every shown zone drawn, none overlapping
    Engine e = fresh(); e.P.requireShown = true;
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 120, {{6996, 30}, {6991, 30}, {7006, 30}, {7011, 30}, {6975, 100}, {6965, 100}, {7030, 100}}, {}); e.updateZones(s * 1000, 0.08, 0); }
    for (int s = 41; s <= 80; ++s) { book(e, s * 1000, 70.00, 120, {{6996, 30}, {6991, 30}, {6975, 100}, {6965, 100}}, {{7006, 30}, {7011, 30}, {7030, 100}}); e.updateZones(s * 1000, 0.08, 0); }
    const std::vector<size_t> o = e.drawOrder();
    int clash = 0, shownN = 0, nearShown = 0;
    for (auto& q : e.zones) if (q.shown) { shownN++; if (q.near) nearShown++; }
    for (size_t i = 0; i < o.size(); ++i) for (size_t j = i + 1; j < o.size(); ++j) {
        const Zone& a = e.zones[o[i]]; const Zone& b = e.zones[o[j]];
        if (a.edgeLo <= b.edgeHi && b.edgeLo <= a.edgeHi) clash++;
    }
    for (size_t i = 1; i < o.size(); ++i) if (e.zones[o[i]].bx > e.zones[o[i - 1]].bx) clash += 100;
    std::printf("  dense: %d shown (%d near), %d drawn\n", shownN, nearShown, (int)o.size());
    CHECK(nearShown >= 2 && clash == 0, "(0.14.0) dense near zones: drawn strongest first, no two drawn zones overlap");
    // forced overlap of shown near zones: the weaker is left out
    Engine f = fresh();
    Zone a; a.id = 1; a.side = BID; a.shown = true; a.near = true; a.bx = 1.6; a.edgeLo = 6990; a.edgeHi = 6994;
    Zone b = a; b.id = 2; b.bx = 1.5; b.edgeLo = 6993; b.edgeHi = 6997;
    Zone c = a; c.id = 3; c.bx = 1.8; c.edgeLo = 6980; c.edgeHi = 6984;
    f.zones = {a, b, c};
    const std::vector<size_t> fo = f.drawOrder();
    CHECK(fo.size() == 2 && f.zones[fo[0]].id == 3 && f.zones[fo[1]].id == 1, "(0.14.0) overlapping near zones: the weaker one is not drawn");
}

// (0.14.0, coordinator review) a shown zone in an absorption attempt is never hidden mid-touch; after the attempt the normal rules apply
static void test_near_zone_kept_during_attempt() {
    Engine e = fresh(); e.P.requireShown = true;
    e.onLastPrice(70.00);
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 30}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneHolding(e, BID, 6995); const long long id = z ? z->id : -1;
    CHECK(z && z->near && z->shown, "(0.14.0) fixture: near zone shown at 1.6x");
    auto get1 = [&]() -> const Zone* { for (auto& q : e.zones) if (q.id == id) return &q; return nullptr; };
    e.onTrade(41000, 70.00, 2, 0); e.onTrade(41500, 69.95, 0, 6);          // sellers hit the zone from above: an attempt starts
    z = get1();
    CHECK(z && z->inAttempt && e.attempts.size() == 1, "(0.14.0) fixture: the zone is in an absorption attempt");
    for (int s = 42; s <= 70; ++s) { book(e, s * 1000, 69.95, 60, {{6995, -20}, {6985, 100}}); e.updateZones(s * 1000, 0.08, 0); }   // eaten to 0.6x for 28 s
    z = get1();
    std::printf("  in attempt: bx %.2f shown %d\n", z ? z->bx : -1, z ? (int)z->shown : -1);
    CHECK(z && z->bx < 0.65 && z->shown, "(0.14.0) a shown near zone eaten to 0.6x during its attempt stays shown (no floor, no grace mid-touch)");
    { std::vector<std::pair<Tick, Tick>> none; e.onBarClose(1, 1000, 71000, 69.80, none, none); }   // a bar closes through the zone: the attempt ends (break)
    book(e, 71000, 69.95, 60, {{6995, -20}, {6985, 100}}); e.updateZones(71000, 0.08, 0);
    z = get1();
    bool open = false; for (auto& a : e.attempts) if (!a.done && a.zoneId == id) open = true;
    CHECK(!open, "(0.14.0) fixture: the attempt has ended");
    CHECK(z && !z->inAttempt && !z->shown, "(0.14.0) after the attempt ends below 1.0x it is hidden at once");
    // the floor still stops a zone APPEARING under 1.0x, even in an attempt
    Engine f = fresh(); f.P.nearShowX = 0.8;
    Zone q; q.id = 1; q.side = BID; q.near = true; q.behindOk = true; q.behindN = 8; q.rank = 1; q.bx = 0.9; q.inAttempt = true;
    f.showState(q, 60000);
    CHECK(!q.shown, "(0.14.0) a zone in an attempt still cannot APPEAR under 1.0x");
}

int main() {
    test_near_rule_rank2_shows(); test_near_rule_far_unchanged(); test_near_keep_and_grace(); test_near_never_under_1x();
    test_near_hysteresis_no_flicker(); test_near_candidate_always_tracked(); test_draw_order_dense_near_zones(); test_near_zone_kept_during_attempt();
    test_finished_total_counts_past_50(); test_shown_zone_at_book_edge_does_not_flicker(); test_lq_confirm_hysteresis(); test_draw_order_and_label_lanes();
    test_delta_A_starts_a_read_and_always_on_numbers();
    test_prominence_show_and_confirm(); test_new_wall_waits_and_edge_of_book();
    test_counter_s_after_e(); test_counter_s_on_the_bars_after_the_hold(); test_counter_needs_e_near_and_the_line();
    test_e_watch_then_e(); test_p_watch_then_p(); test_trapped_t(); test_reason_uses_only_confirmed_lines(); test_warnings_priority();
    test_old_orders(); test_stats_in_process_order(); test_eaten_counts_new_arrivals(); test_pulled_and_rebuilt();
    test_other_side_band_is_tracked();
    test_double_crossover();
    test_story_defense_then_offense();
    test_pull_not_replaced_reads_PULL(); test_eaten_and_replaced_reads_holding(); test_eaten_not_replaced_reads_depleting();
    test_pull_vs_execution(); test_window_edge_is_not_a_pull(); test_refill_is_counted(); test_bad_books_refused();
    test_zone_found_and_durable(); test_eaten_break_is_logged_not_marked(); test_pulled_while_attacked_is_Pull_on_the_defense_pane();
    test_pulled_without_attack_is_not_Pull(); test_exhaustion_is_Exh_on_the_aggression_pane(); test_Exh_prints_under_half_with_its_number(); test_only_Exh_and_Pull_are_signals();
    test_hold_with_delta_absorption_is_logged(); test_signal_never_repeats_or_changes(); test_record_round_trip(); test_half_width_rule(); test_norms_fallback();
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
