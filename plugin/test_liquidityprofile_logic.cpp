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

static void test_attempt_eaten_then_break_gives_LX() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 5);         // arrival from above into [69.93, 69.97]
    CHECK(e.attempts.size() == 1, "arrival starts an attempt");
    const double shown = e.attempts[0].shown;
    CHECK(shown >= 200, "shown = zone size at arrival");
    e.onTrade(60000, 69.95, 0, shown);     // sellers trade 1x+ the shown size
    e.onTrade(70000, 69.92, 0, 10);        // and through the far edge
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 180000, 69.91, none, none);   // closes below 69.93
    CHECK(e.marks.size() == 1 && e.marks[0].code == "LX", "eaten 1x+ and a close through = LX");
    CHECK(e.marks.size() == 1 && e.marks[0].eaten >= 1.0, "LX carries the eaten multiple");
    CHECK(e.attempts.empty(), "the attempt is closed");
    CHECK(e.attemptLog.size() == 1, "every attempt is logged");
}

static void test_break_without_eating_is_logged_not_drawn() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 2);
    book(e, 42000, 70.00, 12);             // the 200 lots pulled
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 180000, 69.90, none, none);
    CHECK(e.marks.empty(), "a break on pulled (not eaten) size draws nothing");
    CHECK(!e.attemptLog.empty() && e.attemptLog[0].find("not eaten") != std::string::npos, "it is logged");
    CHECK(!e.finished.empty() && e.finished.back().pulled >= 150, "the pull is measured");
}

static void test_hold_with_exhaustion_gives_L_E() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 30);        // heavy selling in the first half...
    e.onTrade(60000, 69.96, 0, 30);
    e.onTrade(200000, 69.96, 0, 5);        // ...little in the second half
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(1, 1000, 100000, 69.98, none, none);     // closes back out but before 3 min: not decided
    CHECK(e.marks.empty(), "no decision before arrival + 3 min");
    e.onBarClose(2, 2000, 240000, 70.00, none, none);     // 3 min+ after arrival, close above the zone
    CHECK(e.marks.size() == 1 && e.marks[0].code == "L?" && e.marks[0].reasons == "E", "hold + exhaustion = L? E");
}

static void test_hold_with_delta_absorption_gives_L_A() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 10);
    e.onTrade(150000, 69.96, 0, 10);       // steady selling: no exhaustion
    std::vector<std::pair<Tick, Tick>> dpBid = {{6994, 6996}}, none;
    e.onBarClose(2, 2000, 240000, 70.00, dpBid, none);
    CHECK(e.marks.size() == 1 && e.marks[0].reasons == "A", "hold + Delta Profile A in the zone = L? A");
}

static void test_hold_without_reason_is_logged_only() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 10);
    e.onTrade(150000, 69.96, 0, 10);
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(2, 2000, 240000, 70.00, none, none);
    CHECK(e.marks.empty(), "a hold with no supporting read draws nothing");
    CHECK(e.attemptLog.size() == 1 && e.attemptLog[0].find("no supporting read") != std::string::npos, "but it is logged");
}

static void test_one_mark_per_bar_and_no_repaint() {
    Engine e = zoneSetup();
    e.onTrade(41000, 69.97, 0, 30); e.onTrade(200000, 69.96, 0, 1);
    std::vector<std::pair<Tick, Tick>> none;
    e.onBarClose(2, 2000, 240000, 70.00, none, none);
    const size_t n = e.marks.size();
    e.onBarClose(3, 3000, 420000, 69.80, none, none);     // later bars cannot change the decided mark
    CHECK(n == 1 && e.marks.size() == n && e.marks[0].code == "L?", "a decided mark never changes");
}

static void test_record_round_trip() {
    Mark m; m.code = "L?"; m.reasons = "E A"; m.side = BID; m.barTime = 1791700000; m.lo = 6993; m.hi = 6997; m.shown = 210; m.eaten = 0.43;
    m.stayed = 0.82; m.exhaust = 0.31; m.resil = 0.9; m.tA = 41000; m.tDecided = 240000;
    Mark r; CHECK(Engine::parse(Engine::line(m, 0.01), r, 0.01), "the record line parses");
    CHECK(r.code == "L?" && r.reasons == "E A" && r.lo == 6993 && r.hi == 6997 && r.barTime == 1791700000, "fields survive a restart");
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
int main() {
    test_pull_not_replaced_reads_PULL(); test_eaten_and_replaced_reads_holding(); test_eaten_not_replaced_reads_depleting();
    test_pull_vs_execution(); test_window_edge_is_not_a_pull(); test_refill_is_counted(); test_bad_books_refused();
    test_zone_found_and_durable(); test_attempt_eaten_then_break_gives_LX(); test_break_without_eating_is_logged_not_drawn();
    test_hold_with_exhaustion_gives_L_E(); test_hold_with_delta_absorption_gives_L_A(); test_hold_without_reason_is_logged_only();
    test_one_mark_per_bar_and_no_repaint(); test_record_round_trip(); test_half_width_rule(); test_norms_fallback();
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
