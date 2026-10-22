// test_lqp044.cpp -- lsLiquidityProfile 0.44.0 focused tests (SDK-free, no Investor/RT needed). From plugin\:
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests_lqp044/test_lqp044.cpp -o t044 && ./t044
// Covers: 1) the strict 2.0x draw gate with bar-close hysteresis (records / attempts keep the old shown state); 2) LMT R single-order
// check (bol::Engine::biggestSingle); 3) the zone normal's source (norms file / 5-session pool / session median) + the pool's
// save / load / roll; 4) MenthorQ contract + 18 h rule; 5) the Market State LIQ / LOTS kv (and that they pass through MarketState.h);
// 6) BigFlow per-bar market-order max.
#include "LiquidityProfileLogic.h"
#include "BigOrderLines.h"
#include "BigFlow.h"
#include "MarketState.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) ++passes; else { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)
using namespace lqp;
typedef std::vector<std::pair<double, double>> Lv;

// a CL-like book (as test_liquidityprofile_logic.cpp): bids from bestBid down, asks above, tick 0.01, base 10 lots per level
static void book(Engine& e, Ms t, double bestBid, int levels, std::map<long long, double> extraBid = {}) {
    Lv b, a;
    for (int i = 0; i < levels; ++i) {
        const double pb = bestBid - i * 0.01, pa = bestBid + 0.01 + i * 0.01;
        const long long tb = toTick(pb, 0.01);
        b.push_back(std::make_pair(pb, 10.0 + (extraBid.count(tb) ? extraBid[tb] : 0)));
        a.push_back(std::make_pair(pa, 10.0));
    }
    e.onBook(t, b, a);
}
static Engine fresh() { Engine e; e.P.tick = 0.01; e.P.hwMin = 2; e.P.hwMax = 2; e.P.persistMs = 30000; e.P.requireShown = true; return e; }
static const Zone* zoneAt(const Engine& e, Tick c) { for (size_t i = 0; i < e.zones.size(); ++i) if (e.zones[i].lo <= c && e.zones[i].hi >= c && e.zones[i].side == BID) return &e.zones[i]; return nullptr; }
static void close(Engine& e, int bar, Ms t) { std::vector<std::pair<Tick, Tick>> none; e.onBarClose(bar, 1000 + bar, t, 70.005, none, none); }

// ---- 1) the strict 2.0x draw gate --------------------------------------------------------------------------------------------
static void test_draw_gate_pure() {
    CHECK(drawEnter(true, true, 2.0, true, 2.0), "2.0x, durable, rank 1, book behind: drawn");
    CHECK(!drawEnter(true, true, 1.99, true, 2.0), "1.99x: not drawn");
    CHECK(!drawEnter(true, true, 1.5, true, 2.0), "the old 1.5x near entry is gone");
    CHECK(!drawEnter(true, true, 3.0, false, 2.0), "not rested 30 s: not drawn");
    CHECK(!drawEnter(false, true, 3.0, true, 2.0), "too little book behind it to measure: not drawn");
    CHECK(!drawEnter(true, false, 3.0, true, 2.0), "far and not rank 1: not drawn");
    CHECK(drawKeep(8, 2.0, 2.0) && !drawKeep(8, 1.99, 2.0), "kept at 2.0x, removed under it (no 1.7x / 1.2x keep line)");
    CHECK(drawKeep(0, 0.5, 2.0), "nothing behind to measure (edge of the book): unchanged");
}
static void test_draw_gate_engine() {
    // a near support at 69.95 holding 60 extra lots: band 5 ticks = 110 vs 50 behind = 2.2x
    Engine e = fresh(); e.onLastPrice(70.01);
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 60}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneAt(e, 6995);
    CHECK(z && z->shown && z->drawn, "a rested 2.2x zone is shown and drawn");
    CHECK(z && z->bx > 2.0 && z->bx < 2.4, "bx is about 2.2");
    CHECK(e.drawOrder().size() == 1, "drawOrder lists the drawn zone");
    // it drops to 1.8x inside the bar: still drawn (decided only at the bar close)
    for (int s = 41; s <= 50; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 40}}); e.updateZones(s * 1000, 0.08, 0); }
    z = zoneAt(e, 6995);
    CHECK(z && z->drawn && z->bx < 2.0, "under 2.0x mid-bar: not removed yet (no flicker)");
    // back to 2.2x before the close: nothing changed on screen
    for (int s = 51; s <= 55; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 60}}); e.updateZones(s * 1000, 0.08, 0); }
    close(e, 1, 56000);
    z = zoneAt(e, 6995);
    CHECK(z && z->drawn, "back at 2.2x at the close: stays drawn");
    // under 2.0x at a close: removed from the drawing; still 'shown' (the records / attempts are unchanged)
    for (int s = 57; s <= 60; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 40}}); e.updateZones(s * 1000, 0.08, 0); }
    close(e, 2, 61000);
    z = zoneAt(e, 6995);
    CHECK(z && !z->drawn, "1.8x at a bar close: removed from the drawing");
    CHECK(z && z->shown, "the old near rule still keeps it 'shown' for the records (logging unchanged)");
    CHECK(e.drawOrder().empty(), "drawOrder empty: nothing under 2.0x is drawn");
    // it rebuilds to 2.2x: drawn again live (the entry is not delayed to a close)
    for (int s = 62; s <= 64; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 60}}); e.updateZones(s * 1000, 0.08, 0); }
    z = zoneAt(e, 6995);
    CHECK(z && z->drawn, "back at 2.0x+: drawn again");
}
static void test_near_15x_not_drawn() {
    // 1.6x near zone (30 extra: 80 vs 50): the 0.43 build drew it (near 1.5x); 0.44 shows it for the records but never draws it
    Engine e = fresh(); e.onLastPrice(70.01);
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 30}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneAt(e, 6995);
    CHECK(z && z->shown && !z->drawn, "a 1.6x near zone: shown (records) but not drawn");
    CHECK(e.drawOrder().empty(), "nothing drawn");
    const Zone* c = e.confirming(6995, 7001);
    CHECK(c == nullptr, "an undrawn zone never confirms a level (no LQ label)");
}
static void test_absorption_does_not_keep_drawing() {
    // a drawn zone in an attempt that is eaten under 2.0x: removed at the close (the 0.14.0 any-size rule kept it)
    Engine e = fresh(); e.onLastPrice(70.01);
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 60}}); e.updateZones(s * 1000, 0.08, 0); }
    for (size_t i = 0; i < e.zones.size(); ++i) e.zones[i].inAttempt = true;      // being defended
    for (int s = 41; s <= 45; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 5}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneAt(e, 6995);
    CHECK(z && z->shown, "in an attempt: still 'shown' for the attempt / records");
    for (size_t i = 0; i < e.zones.size(); ++i) e.zones[i].drawn = e.zones[i].drawn;   // (no-op: the close decides)
    std::vector<std::pair<Tick, Tick>> none;
    e.drawClose();
    z = zoneAt(e, 6995);
    CHECK(z && !z->drawn, "in an attempt but under 2.0x at the close: not drawn");
}

// ---- 2) LMT R: the largest SINGLE order, not the price level's total -----------------------------------------------------------
static void test_biggest_single() {
    bol::Engine b; b.tick = 0.25; b.setPrice(20000, false); b.setAtr(10.0);   // reach = 2 ATR = 80 ticks by default
    CHECK(b.reach > 0, "reach set");
    const bol::Tick farPx = 20000 - b.reach - 20, farPx2 = 20000 - b.reach - 40;
    long long id = 1;
    for (int k = 0; k < 40; ++k) b.onEvent(bol::NEW, bol::BID, farPx, 5, id++, 1000);   // a 200-lot level of 5-lot orders
    b.onEvent(bol::NEW, bol::BID, farPx2, 120, id++, 1000);                               // one 120-lot order
    b.onEvent(bol::NEW, bol::BID, 20000 - 10, 90, id++, 1000);                            // a 90-lot order within the reach
    bol::Tick px = 0; long long n = 0;
    CHECK(b.biggestSingle(bol::BID, true, 100, px, n) && n == 120 && px == farPx2, "beyond the reach: the single 120-lot order, not the 200-lot level of 5s");
    CHECK(!b.biggestSingle(bol::BID, true, 150, px, n), "no single order of 150+: nothing (the old check drew the 200-lot level)");
    CHECK(b.biggestSingle(bol::BID, true, 1, px, n) && n == 120, "below the floor (whole-book path): same answer");
    CHECK(b.biggestSingle(bol::BID, false, 1, px, n) && n == 90 && px == 20000 - 10, "within the reach: the 90-lot order");
    CHECK(!b.biggestSingle(bol::ASK, false, 1, px, n), "no offers: nothing");
    b.onEvent(bol::FILL, bol::ASK, farPx2, 120, id - 2, 2000);                            // the 120 traded away
    CHECK(!b.biggestSingle(bol::BID, true, 100, px, n), "filled away: gone");
}

// ---- 3) the zone normal: its source + the 5-session pool ------------------------------------------------------------------------
static void test_norm_source() {
    Norms n; int src = -1, ns = -1;
    n.median[16] = 50; n.loaded = true;
    CHECK(n.pick(16, src) == 50 && src == NS_FILE, "an RTH slot from the norms file");
    CHECK(n.pick(40, src, &ns) == 0 && src == NS_NONE, "overnight, nothing learned yet: none (no fake 'norms file')");
    for (int i = 0; i < 40; ++i) n.sample(3.0);
    CHECK(n.pick(40, src) == 3.0 && src == NS_SESSION, "only the session median: labelled as the fallback");
    for (int i = 0; i < 100; ++i) n.pool.sample(100, 40, 200.0);
    for (int i = 0; i < 100; ++i) n.pool.sample(101, 40, 220.0);
    CHECK(n.pick(40, src, &ns) == 3.0 && src == NS_SESSION && ns == 1, "one finished session in the pool: not enough (2 needed)");
    for (int i = 0; i < 100; ++i) n.pool.sample(102, 40, 240.0);
    const double v = n.pick(40, src, &ns);
    CHECK(src == NS_POOL && ns == 2 && v > 195 && v < 235, "two finished sessions: the pool's slot normal (~210)");
    CHECK(n.pick(16, src) == 50 && src == NS_FILE, "the file still wins where it has the slot");
    CHECK(std::string(normSrcName(NS_POOL)) == "5-session pool slot" && std::string(normSrcName(NS_FILE)) == "norms file slot", "status names");
}
static void test_band_pool() {
    BandPool p;
    for (long long s = 1; s <= 8; ++s) for (int i = 0; i < 80; ++i) p.sample(s, 2, 100.0 * (double)s);
    p.sample(9, 2, 1.0);                                   // the 8th session rolls in
    CHECK((int)p.past.size() == BandPool::KEEP, "only the 5 newest sessions are kept");
    CHECK(p.past.begin()->first == 4, "the oldest kept is session 4");
    // an empty session (weekend / no depth) is never counted
    BandPool q; for (int i = 0; i < 80; ++i) q.sample(10, 5, 50.0);
    q.sample(11, 5, 50.0);                                 // session 11: one sample only
    q.sample(12, 5, 50.0);                                 // rolls 11 (1 sample < MIN_N): not stored
    CHECK(q.past.size() == 1 && q.past.count(10) == 1 && q.past.count(11) == 0, "a session with too few samples is not a session");
    // save / load round trip; the current session's saved row is not restored as finished
    for (int i = 0; i < 80; ++i) q.sample(12, 5, 70.0);
    const std::string t = q.text();
    CHECK(t.compare(0, 4, "BP1\n") == 0 && t.find("S,10,5,") != std::string::npos && t.find("S,12,5,") != std::string::npos, "text has the finished and the current session");
    BandPool r; CHECK(r.parse(t, 12), "parsed");
    CHECK(r.past.size() == 1 && r.past.count(10) == 1, "the current session (12) is not restored as finished");
    BandPool r2; CHECK(r2.parse(t, 13) && r2.past.size() == 2, "a later session restores both");
    BandPool bad; CHECK(!bad.parse("garbage\nS,1,2,3,100\n", 5), "a file without the header is refused");
    BandPool odd; CHECK(odd.parse("BP1\nS,1,99,3,100\nS,x,2,3,100\nS,2,3,-1,100\nS,3,4,nan,100\nS,4,5,10,5\nS,5,6,12,100\n", 9) && odd.past.size() == 1, "bad rows skipped (slot range, number, sign, NaN, too few samples)");
    SlotHist h; for (int i = 1; i <= 99; ++i) h.add((double)i);
    CHECK(h.median() > 47 && h.median() < 53, "histogram median within one 5% bucket");
}

// ---- 4) MenthorQ contract + age ---------------------------------------------------------------------------------------------------
static void test_mq_rules() {
    CHECK(!sameContract("CLZ6", "CLEX26"), "CLEX26 (Nov) on a CLZ6 chart: skipped");
    CHECK(sameContract("CLZ6", "CLEZ26"), "CLEZ26 on CLZ6: same contract");
    CHECK(sameContract("ESZ6", "EPZ26"), "EPZ26 on ESZ6: same");
    CHECK(sameContract("ESZ26", "EPZ26"), "ESZ26 on EPZ26: same");
    CHECK(!sameContract("ESZ6", "EPH27"), "next quarter: skipped");
    CHECK(sameContract("@ES", "EPZ26") && sameContract("", "EPZ26"), "continuous / no symbol: accepted");
    CHECK(sameContract("6EZ6", "EU6Z26"), "6E: EU6Z26 on 6EZ6");
    const long long now = 1800000000;
    CHECK(mqFresh(now - 3600, now), "1 h old: used");
    CHECK(mqFresh(now - 18 * 3600, now) && !mqFresh(now - 18 * 3600 - 1, now), "18 h is the line");
    CHECK(!mqFresh(0, now) && !mqFresh(now + 3600, now), "no stamp / an hour in the future: not used");
}

// ---- 5) Market State LIQ / LOTS ----------------------------------------------------------------------------------------------------
static void test_liq_kv() {
    LiqFacts f; std::map<std::string, std::string> kv = liqKv(f);
    CHECK(kv.size() == 10 && kv["zone_id"] == "?" && kv["zone_step"] == "none" && kv["zone_x"] == "?", "no zone: every key present, '?' / none");
    f.have = true; f.level = "PDL"; f.tick = 0.25; f.lo = 5011.75; f.hi = 5012.75; f.lots = 812.4; f.x = 2.46; f.side = BID; f.step = "absorbing"; f.absorbLots = 233; f.pulledBeforePct = 12.6;
    kv = liqKv(f);
    CHECK(kv["zone_id"] == "PDL 5012.25" && kv["zone_lo"] == "5011.75" && kv["zone_hi"] == "5012.75", "zone id = level + price, market decimals");
    CHECK(kv["zone_level"] == "PDL" && kv["zone_lots"] == "812" && kv["zone_x"] == "2.5" && kv["zone_side"] == "support", "level, lots integer, ratio 1 decimal, side");
    CHECK(kv["zone_step"] == "absorbing" && kv["absorb_lots"] == "233" && kv["pulled_before_pct"] == "13", "step and the absorption numbers");
    f.x = 1.8; f.side = ASK; f.pulledBeforePct = -1; kv = liqKv(f);
    CHECK(kv["zone_x"] == "?" && kv["zone_side"] == "resistance" && kv["pulled_before_pct"] == "?", "under 2.0x the ratio is not stated; unknown = ?");
    CHECK(zoneStepOf(false, true, true, 3, 3, 3, 10) == "break" && zoneStepOf(true, false, false, -1, -1, -1, 0) == "held", "the zone rule's verdict on this bar first");
    CHECK(zoneStepOf(false, false, false, -1, -1, -1, 0) == "none", "no attempt: none");
    CHECK(zoneStepOf(false, false, true, -1, -1, -1, 0) == "arrived" && zoneStepOf(false, false, true, -1, -1, -1, 5) == "absorbing", "arrived / absorbing");
    CHECK(zoneStepOf(false, false, true, -1, -1, 4, 5) == "attack_fading" && zoneStepOf(false, false, true, -1, 6, 4, 5) == "counter", "attack fading, then counter");
    CHECK(zoneStepOf(false, false, true, 7, 6, 4, 5) == "defenders_pulling", "pulled wins");
}
static void test_lots_kv() {
    CHECK(tierOf(100, false, 80, 108, 195) == "?", "tiers learning: ?");
    CHECK(tierOf(79, true, 80, 108, 195) == "none" && tierOf(80, true, 80, 108, 195) == "S20" && tierOf(108, true, 80, 108, 195) == "S10" && tierOf(195, true, 80, 108, 195) == "S5", "S20 / S10 / S5 lines");
    LotsFacts f; std::map<std::string, std::string> kv = lotsKv(f);
    CHECK(kv.size() == 16, "16 LOTS keys (spec)");
    CHECK(kv["s20"] == "?" && kv["mkt_buy_max"] == "?" && kv["mkt_tier"] == "?" && kv["iceberg"] == "none" && kv["resting_bid_max"] == "?", "nothing known: ?");
    f.tiersOk = true; f.s20 = 80.4; f.s10 = 108; f.s5 = 195; f.mu = 153; f.mktTiersOk = true; f.ms20 = 111; f.ms10 = 156; f.ms5 = 233; f.tick = 0.25;
    f.mktBuy = 160; f.mktSell = 20; f.lmtFilled = 0; f.iceberg = ASK; f.restBid = 120; f.restBidPx = 5000.5; f.restAsk = 60; f.restAskPx = 5003.25;
    kv = lotsKv(f);
    CHECK(kv["s20"] == "80" && kv["s10"] == "108" && kv["s5"] == "195" && kv["mu"] == "153", "LMT tiers in force");
    CHECK(kv["mkt_buy_max"] == "160" && kv["mkt_sell_max"] == "20" && kv["mkt_tier"] == "S10", "largest market order + its MKT tier");
    CHECK(kv["lmt_filled_max"] == "0" && kv["lmt_filled_side"] == "none" && kv["iceberg"] == "ask", "no big fill this bar; an offer iceberg");
    CHECK(kv["resting_bid_max"] == "120" && kv["resting_bid_px"] == "5000.50" && kv["resting_tier_bid"] == "S10", "biggest resting bid (120 >= S10 108)");
    CHECK(kv["resting_ask_px"] == "5003.25" && kv["resting_tier_ask"] == "none", "biggest resting offer under S20");
    f.lmtFilled = 210; f.lmtFilledSide = BID; kv = lotsKv(f);
    CHECK(kv["lmt_filled_max"] == "210" && kv["lmt_filled_side"] == "bid", "a big bid filled");
}
static void test_market_state_roundtrip() {
    LiqFacts f; f.have = true; f.level = "PDL/CR"; f.tick = 0.01; f.lo = 69.93; f.hi = 69.97; f.lots = 110; f.x = 2.2; f.step = "held";
    const std::map<std::string, std::string> liq = liqKv(f);
    LotsFacts g; g.tiersOk = true; g.s20 = 10; g.s10 = 20; g.s5 = 40; g.mu = 8;
    const std::map<std::string, std::string> lots = lotsKv(g);
    for (std::map<std::string, std::string>::const_iterator it = liq.begin(); it != liq.end(); ++it)
        CHECK(it->second.find_first_of(";=|\n") == std::string::npos && !it->second.empty(), "LIQ values are clean and never empty");
    for (std::map<std::string, std::string>::const_iterator it = lots.begin(); it != lots.end(); ++it)
        CHECK(it->second.find_first_of(";=|\n") == std::string::npos && !it->second.empty(), "LOTS values are clean and never empty");
    mst::publish("LIQ", "CLZ6", 180, "t044", 5000, 1789000000, liq);
    mst::publish("LOTS", "CLZ6", 180, "t044", 5000, 1789000000, lots);
    const mst::Facts a = mst::read("LIQ", "CLZ6", 180, "t044", 6000), b = mst::read("LOTS", "CLZ6", 180, "t044", 6000);
    CHECK(a.ok && a.bar == 1789000000 && a.kv == liq, "LIQ round trip through MarketState.h");
    CHECK(b.ok && b.kv == lots, "LOTS round trip");
    CHECK(mst::getS(a, "zone_id") == "PDL/CR 69.95" && mst::getD(a, "zone_x") == 2.2, "readable by the Tape Reader");
    mst::clear("LIQ", "CLZ6", 180, "t044"); mst::clear("LOTS", "CLZ6", 180, "t044");
    CHECK(!mst::read("LIQ", "CLZ6", 180, "t044", 6000).ok && !mst::read("LOTS", "CLZ6", 180, "t044", 6000).ok, "cleared on removal");
}

// ---- 6) BigFlow: the largest market order in a bar --------------------------------------------------------------------------------
static void test_mkt_max_in_bar() {
    bfl::Mkt m;
    m.onFill(11, bfl::BID, 100, 30, 10000); m.onFill(11, bfl::BID, 101, 20, 10500);   // one 50-lot buy order, last fill 10.5 s
    m.onFill(12, bfl::ASK, 99, 70, 20000);                                          // a 70-lot sell at 20 s
    m.onFill(13, bfl::BID, 100, 5, 30000);                                          // a 5-lot buy at 30 s
    m.step(40000);                                                                  // all complete
    CHECK(m.maxIn(bfl::BID, 0, 30000) == 50, "bar [0, 30 s): the 50-lot buy (all fills of one aggressor id)");
    CHECK(m.maxIn(bfl::ASK, 0, 30000) == 70, "the 70-lot sell");
    CHECK(m.maxIn(bfl::BID, 30000, 60000) == 5, "next bar: the 5-lot buy (small orders count too)");
    CHECK(m.maxIn(bfl::ASK, 30000, 60000) == 0, "no sell: 0");
    m.onFill(14, bfl::ASK, 98, 90, 59000);                                          // still filling at the close
    CHECK(m.maxIn(bfl::ASK, 30000, 60000) == 90, "an order still filling at the close is included");
    m.step(59000 + bfl::Mkt::RECENT_MS + 10000);
    CHECK(m.maxIn(bfl::BID, 0, 30000) == 0, "older than 30 min: dropped");
}

int main() {
    test_draw_gate_pure(); test_draw_gate_engine(); test_near_15x_not_drawn(); test_absorption_does_not_keep_drawing();
    test_biggest_single(); test_norm_source(); test_band_pool(); test_mq_rules(); test_liq_kv(); test_lots_kv(); test_market_state_roundtrip();
    test_mkt_max_in_bar();
    CHECK(std::string(VERSION) >= "0.45.3", /* (2026-10-22) the version moves on with each approved build (0.47.1 now): at least this one */   /* (0.45.3) hovers off + lighter column inks */ "lqp::VERSION 0.45.2 (0.45.2 MKT no flicker + LMT F resting colour, 0.44.1 banner colour API, 0.44.2 Flow link fix, 0.45.0 settings layer)");
    CHECK(std::string(bol::VERSION) == "1.8.0" && std::string(bfl::VERSION) == "1.4.0", "bol 1.8.0 / bfl 1.4.0");
    std::printf("%d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
