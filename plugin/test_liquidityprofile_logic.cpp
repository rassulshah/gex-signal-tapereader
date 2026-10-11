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

static void test_market_threshold_changes_Exh() {
    Engine e = zoneSetup(); e.P.exhaust = 0.2;              // (0.3.1) this market's data-chosen threshold
    e.onTrade(41000, 69.97, 0, 30); e.onTrade(60000, 69.96, 0, 30);
    e.onTrade(200000, 69.96, 0, 20);                        // 2nd half 20 / 1st 60 = 0.33: dried up at 0.5, not at 0.2
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(2, 2000, 240000, 70.00, none, none);
    CHECK(!has(e, "Exh"), "with exhaust 0.2 a 0.33 pace is not exhaustion");
}

static void test_hold_with_delta_absorption_is_logged() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 10);
    e.onTrade(150000, 69.96, 0, 10);       // steady selling: no exhaustion
    std::vector<std::pair<Tick, Tick>> dpBid = {{6994, 6996}}, none;
    e.onBarClose(2, 2000, 240000, 70.00, dpBid, none);
    CHECK(e.marks.empty(), "nothing drawn on the chart");
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
    for (const char* c : {"Pull", "LoF", "HiB", "RpB", "RpO"}) { Mark q = m; q.code = c; Mark z; CHECK(Engine::parse(Engine::line(q, 0.01), z, 0.01) && z.code == c, "every pane code round-trips"); }
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
    CHECK(!e.attempts.empty() && e.attempts[0].barO == 11 && has(e, "LoF"), "defense first (sellers led since arrival), then the buyers' total crosses above in bar 11 = LoF");
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
    CHECK(!e.attempts.empty() && e.attempts[0].barR == 11 && e.attempts[0].barO == 11 && has(e, "LoF") && has(e, "RpB"), "both crosses in bar 11 = LoF (aggression) and RpB (defense)");
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
int main() {
    test_other_side_band_is_tracked();
    test_double_crossover();
    test_story_defense_then_offense();
    test_pull_not_replaced_reads_PULL(); test_eaten_and_replaced_reads_holding(); test_eaten_not_replaced_reads_depleting();
    test_pull_vs_execution(); test_window_edge_is_not_a_pull(); test_refill_is_counted(); test_bad_books_refused();
    test_zone_found_and_durable(); test_eaten_break_is_logged_not_marked(); test_pulled_while_attacked_is_Pull_on_the_defense_pane();
    test_pulled_without_attack_is_not_Pull(); test_exhaustion_is_Exh_on_the_aggression_pane(); test_market_threshold_changes_Exh();
    test_hold_with_delta_absorption_is_logged(); test_signal_never_repeats_or_changes(); test_record_round_trip(); test_half_width_rule(); test_norms_fallback();
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
