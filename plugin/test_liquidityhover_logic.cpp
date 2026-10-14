// test_liquidityhover_logic.cpp -- Liquidity Profile 0.15.0 (v2 hover, Rassul approved 6:44 PM CT 2026-10-13): every hover line and
// colour state, the 'as price came' +/-10% band, the baseline recorded once per approach, the makeup CSV columns, the box placement.
//   g++ -std=c++17 -Wall -Wextra -I. test_liquidityhover_logic.cpp -o thl && ./thl
#include "LiquidityProfileLogic.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(cond, msg) do { if (cond) ++passes; else { ++fails; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)
using namespace lqp;
typedef std::vector<std::pair<double, double>> Lv;
static const std::string DOT = "\xB7";     // the chart's ANSI middle dot

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
static const Zone* zoneHolding(const Engine& e, int side, Tick t) { for (auto& z : e.zones) if (z.side == side && t >= z.lo && t <= z.hi) return &z; return nullptr; }
static Makeup mk(long long orders, long long lots, long long top, bool ok = true) { Makeup m; m.orders = orders; m.lots = lots; m.top = top; m.ok = ok; m.coverage = ok ? 1.0 : 0.3; return m; }
static std::vector<std::string> split(const std::string& s, char c) { std::vector<std::string> r; std::string x; for (char ch : s) { if (ch == c) { r.push_back(x); x.clear(); } else x += ch; } r.push_back(x); return r; }

// ---- line 3: 'as price came' - the +/-10% band (inclusive), rounding, guards
static void test_came_band() {
    CHECK(cameOf(100, 110, 0.10).state == CAME_BUILDING && cameOf(100, 110, 0.10).pct == 10, "exactly +10% = building, up 10%");
    CHECK(cameOf(100, 109.9, 0.10).state == CAME_UNCHANGED, "+9.9% = unchanged (inside the band)");
    CHECK(cameOf(100, 90, 0.10).state == CAME_LEAVING && cameOf(100, 90, 0.10).pct == 10, "exactly -10% = leaving, down 10%");
    CHECK(cameOf(100, 90.1, 0.10).state == CAME_UNCHANGED, "-9.9% = unchanged");
    CHECK(cameOf(100, 100, 0.10).state == CAME_UNCHANGED, "no change = unchanged");
    CHECK(cameOf(305, 366, 0.10).state == CAME_BUILDING && cameOf(305, 366, 0.10).pct == 20, "305 -> 366 lots = up 20% (mockup resistance)");
    CHECK(cameOf(511, 332, 0.10).state == CAME_LEAVING && cameOf(511, 332, 0.10).pct == 35, "511 -> 332 lots = down 35% (mockup support at PDL)");
    CHECK(cameOf(366, 402.6, 0.10).state == CAME_BUILDING, "floating point: 366 x 1.1 = 402.6 is building (no rounding miss)");
    CHECK(cameOf(70, 77, 0.10).state == CAME_BUILDING, "floating point: 70 x 1.1 = 77 is building");
    CHECK(cameOf(70, 63, 0.10).state == CAME_LEAVING, "floating point: 70 x 0.9 = 63 is leaving");
    CHECK(cameOf(100, 0, 0.10).state == CAME_LEAVING && cameOf(100, 0, 0.10).pct == 100, "all gone = down 100% - leaving");
    CHECK(cameOf(100, 250, 0.10).pct == 150, "up 150%");
    CHECK(cameOf(-1, 100, 0.10).state == CAME_NONE, "no baseline (not near) = no line");
    CHECK(cameOf(0, 100, 0.10).state == CAME_NONE, "zero baseline = no line (no divide by zero)");
    CHECK(cameOf(100, std::nan(""), 0.10).state == CAME_NONE && cameOf(std::nan(""), 100, 0.10).state == CAME_NONE, "NaN = no line");
    CHECK(cameOf(100, -5, 0.10).state == CAME_NONE, "negative lots = no line");
    CHECK(cameOf(100, 115, 0.20).state == CAME_UNCHANGED && cameOf(100, 120, 0.20).state == CAME_BUILDING, "the band is a parameter");
}
static void test_came_text_and_colour() {
    CHECK(cameText(cameOf(305, 366, 0.10)) == "As price came: up 20% - building", "text: up NN% - building");
    CHECK(cameText(cameOf(511, 332, 0.10)) == "As price came: down 35% - leaving", "text: down NN% - leaving");
    CHECK(cameText(cameOf(312, 315, 0.10)) == "As price came: unchanged", "text: unchanged (no number)");
    CHECK(cameText(cameOf(-1, 315, 0.10)).empty(), "text: none = empty");
    CHECK(cameInk(BID, CAME_BUILDING) == INK_GREEN, "support building = green");
    CHECK(cameInk(BID, CAME_LEAVING) == INK_RED, "support leaving = red");
    CHECK(cameInk(ASK, CAME_BUILDING) == INK_RED, "resistance building = red");
    CHECK(cameInk(ASK, CAME_LEAVING) == INK_GREEN, "resistance leaving = green");
    CHECK(cameInk(BID, CAME_UNCHANGED) == INK_WHITE && cameInk(ASK, CAME_UNCHANGED) == INK_WHITE, "unchanged = white, both sides");
}
// ---- line 5: hit / rebuilt / absorbing; line 3b: one order
static void test_hit_and_one_order() {
    CHECK(hitText(140, 126) == "Hit 140 lots " + DOT + " rebuilt 90% - absorbing", "Hit 140 lots \xB7 rebuilt 90% - absorbing");
    CHECK(hitText(140, 0) == "Hit 140 lots " + DOT + " rebuilt 0% - absorbing", "rebuilt 0%");
    CHECK(hitText(10, 50) == "Hit 10 lots " + DOT + " rebuilt 200% - absorbing", "rebuilt capped at 200%");
    CHECK(hitText(10, -4) == "Hit 10 lots " + DOT + " rebuilt 0% - absorbing", "negative rebuilt reads 0%");
    CHECK(sideInk(BID) == INK_GREEN && sideInk(ASK) == INK_RED, "absorbing / header: green at support, red at resistance");
    CHECK(oneOrder(mk(4, 100, 70), 0.70), "top order exactly 70% = mostly one order");
    CHECK(!oneOrder(mk(4, 100, 69), 0.70), "69% = not");
    CHECK(!oneOrder(mk(4, 100, 90, false), 0.70), "makeup not shown (coverage) = no one-order line");
    CHECK(!oneOrder(mk(0, 0, 0), 0.70), "empty = not");
    CHECK(oneOrderText() == "Mostly one order - may not reload ?", "the v2 wording (replaces 'may be bait')");
}
static void test_px_text() {
    CHECK(pxText(toTick(7864.0, 0.25), 0.25) == "7,864.00", "ES 7,864.00");
    CHECK(pxText(toTick(79.98, 0.01), 0.01) == "79.98", "CL 79.98");
    CHECK(pxText(toTick(24510.5, 0.25), 0.25) == "24,510.50", "NQ 24,510.50");
    CHECK(pxText(toTick(1.1725, 0.00005), 0.00005) == "1.17250", "6E 1.17250");
    CHECK(pxText(toTick(3.456, 0.001), 0.001) == "3.456", "NG 3.456");
    CHECK(pxText(toTick(4012.3, 0.1), 0.1) == "4,012.3", "GC 4,012.3");
}
// ---- the whole hover, in the approved order, with its colours (the v2 mockup's three states)
static void test_hover_rows_mockup_states() {
    HoverIn r; r.side = ASK; r.lo = toTick(7864.0, 0.25); r.hi = toTick(7866.0, 0.25); r.tick = 0.25; r.lots = 366; r.bx = 1.5;
    r.mk = mk(96, 366, 44); r.feedReady = true; r.nearBase = 305; r.inZone = true; r.hit = 140; r.rebuilt = 126;
    std::vector<HoverRow> v = hoverRows(r);
    CHECK(v.size() == 4, "resistance being hit: 4 lines");
    CHECK(v.size() == 4 && v[0].text == "Resistance 7,864.00 - 7,866.00 " + DOT + " 366 lots (1.5x)" && v[0].ink == INK_RED, "1 Resistance header, red");
    CHECK(v.size() == 4 && v[1].text == "Orders 96 " + DOT + " avg 3.8 lots " + DOT + " Top order 12%" && v[1].ink == INK_WHITE, "2 Orders line, white");
    CHECK(v.size() == 4 && v[2].text == "As price came: up 20% - building" && v[2].ink == INK_RED, "3 resistance building = red");
    CHECK(v.size() == 4 && v[3].text == "Hit 140 lots " + DOT + " rebuilt 90% - absorbing" && v[3].ink == INK_RED, "4 hit at resistance = red");
    HoverIn s; s.side = BID; s.level = "PDL"; s.lo = toTick(7856.0, 0.25); s.hi = toTick(7858.0, 0.25); s.tick = 0.25; s.lots = 332; s.bx = 1.3;
    s.mk = mk(87, 332, 30); s.feedReady = true; s.nearBase = 511; s.inZone = false; s.hit = 0;
    v = hoverRows(s);
    CHECK(v.size() == 3, "support at PDL, not hit: 3 lines");
    CHECK(v.size() == 3 && v[0].text == "Support at PDL 7,856.00 - 7,858.00 " + DOT + " 332 lots (1.3x)" && v[0].ink == INK_GREEN, "1 'Support at PDL', green");
    CHECK(v.size() == 3 && v[1].text == "Orders 87 " + DOT + " avg 3.8 lots " + DOT + " Top order 9%" && v[1].ink == INK_WHITE, "2 white");
    CHECK(v.size() == 3 && v[2].text == "As price came: down 35% - leaving" && v[2].ink == INK_RED, "3 support leaving = red");
    HoverIn o; o.side = BID; o.lo = toTick(7853.0, 0.25); o.hi = toTick(7855.0, 0.25); o.tick = 0.25; o.lots = 312; o.bx = 1.2;
    o.mk = mk(4, 312, 265); o.feedReady = true; o.nearBase = 310;
    v = hoverRows(o);
    CHECK(v.size() == 4, "support, mostly one order: 4 lines");
    CHECK(v.size() == 4 && v[1].text == "Orders 4 " + DOT + " avg 78 lots " + DOT + " Top order 85%", "avg 10+ lots has no decimal");
    CHECK(v.size() == 4 && v[2].text == "Mostly one order - may not reload ?" && v[2].ink == INK_WHITE, "one order line, white, under the orders line");
    CHECK(v.size() == 4 && v[3].text == "As price came: unchanged" && v[3].ink == INK_WHITE, "unchanged, white");
}
static void test_hover_rows_states() {
    HoverIn h; h.side = BID; h.lo = 100; h.hi = 104; h.tick = 0.25; h.lots = 200; h.bx = 1.6; h.mk = mk(20, 200, 30); h.feedReady = true; h.nearBase = 150;
    std::vector<HoverRow> v = hoverRows(h);
    CHECK(v.size() == 3 && v[2].text == "As price came: up 33% - building" && v[2].ink == INK_GREEN, "support building = green");
    h.side = ASK; v = hoverRows(h);
    CHECK(v.size() == 3 && v[0].ink == INK_RED && v[0].text.compare(0, 11, "Resistance ") == 0, "resistance header red");
    CHECK(v.size() == 3 && v[2].ink == INK_RED, "resistance building = red");
    h.nearBase = 300; v = hoverRows(h);
    CHECK(v.size() == 3 && v[2].text == "As price came: down 33% - leaving" && v[2].ink == INK_GREEN, "resistance leaving = green");
    h.side = BID; v = hoverRows(h);
    CHECK(v.size() == 3 && v[2].ink == INK_RED, "support leaving = red");
    h.nearBase = -1; v = hoverRows(h);
    CHECK(v.size() == 2, "no baseline: no 'as price came' line");
    h.nearBase = 200; h.inZone = false; h.hit = 50; h.rebuilt = 25; v = hoverRows(h);
    CHECK(v.size() == 3 && v.back().text.compare(0, 4, "Hit ") != 0, "hit lots but price NOT in the zone: no Hit line");
    h.inZone = true; h.hit = 0; v = hoverRows(h);
    CHECK(v.size() == 3, "price in the zone, nothing hit yet: no Hit line");
    h.hit = 50; v = hoverRows(h);
    CHECK(v.size() == 4 && v[3].text == "Hit 50 lots " + DOT + " rebuilt 50% - absorbing" && v[3].ink == INK_GREEN, "price in the zone + hit: the Hit line, green at support");
    h.side = ASK; v = hoverRows(h);
    CHECK(v.size() == 4 && v[3].ink == INK_RED, "... red at resistance");
    h.feedReady = false; h.mk = mk(0, 0, 0, false); v = hoverRows(h);
    CHECK(v.size() >= 2 && v[1].text == "Orders: waiting for the order feed" && v[1].ink == INK_GREY, "no feed: says so in grey");
    h.mk = mk(30, 300, 290, false); v = hoverRows(h);
    bool one = false; for (auto& x : v) if (x.text == oneOrderText()) one = true;
    CHECK(!one, "no one-order line while the makeup is not shown");
    h.feedReady = true; h.mk = mk(3, 60, 50, false); h.mk.coverage = 0.3; v = hoverRows(h);
    CHECK(v.size() >= 2 && v[1].text == "Orders: building (30% of the lots seen)" && v[1].ink == INK_GREY, "feed live, orders not all seen yet: building, grey");
    h.level = "LonLO/CR"; h.side = BID; v = hoverRows(h);
    CHECK(v[0].text.compare(0, 20, "Support at LonLO/CR ") == 0, "two levels on one zone: 'Support at LonLO/CR'");
    h.bx = -1; v = hoverRows(h);
    CHECK(v[0].text.find("lots (-)") != std::string::npos, "no prominence yet: (-)");
    h.lots = 12500; h.bx = 12; v = hoverRows(h);
    CHECK(v[0].text.find("12k lots (10x+)") != std::string::npos, "big numbers as the column shows them: 12k, 10x+");
    // nothing in the hover may have a grey sub-text line or a dashed look: every row is one of the 4 inks
    for (auto& x : v) CHECK(x.ink >= INK_WHITE && x.ink <= INK_GREY, "inks: white / green / red / grey only");
}
// ---- the baseline: once per approach, from the near rule, through the engine
static void test_baseline_per_approach() {
    Engine e = fresh();
    // price far above the 69.95 bid zone (best bid 70.30: 33 ticks away, near band 8, exit 12)
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.30, 60, {{6995, 30}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneHolding(e, BID, 6995);
    CHECK(z && !z->near && z->nearBase < 0, "far zone: no baseline");
    const long long id = z ? z->id : -1;
    auto get = [&]() -> const Zone* { for (auto& q : e.zones) if (q.id == id) return &q; return nullptr; };
    // price comes within reach: best bid 70.00 (zone top 69.97, 3 ticks)
    for (int s = 41; s <= 45; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 30}}); e.updateZones(s * 1000, 0.08, 0); }
    z = get();
    const double base1 = z ? z->size : -1;
    CHECK(z && z->near && z->nearBase > 0 && std::fabs(z->nearBase - base1) < 1e-9, "came within reach: baseline = the zone's lots then");
    std::printf("  approach 1: base %.0f\n", z ? z->nearBase : -1);
    // the zone builds while price stays near: the baseline does NOT move
    for (int s = 46; s <= 60; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 60}}); e.updateZones(s * 1000, 0.08, 0); }
    z = get();
    CHECK(z && std::fabs(z->nearBase - base1) < 1e-9 && z->size > base1, "zone grew: the baseline is kept (recorded once per approach)");
    const Came c1 = z ? cameOf(z->nearBase, z->size, e.P.cameBand) : Came();
    CHECK(c1.state == CAME_BUILDING, "grew 30 lots on 80+ = building");
    std::printf("  approach 1: base %.0f now %.0f -> %s\n", z ? z->nearBase : -1, z ? z->size : -1, cameText(c1).c_str());
    // price drifts to 10 ticks away (inside the exit band 12): still the same approach, same baseline
    for (int s = 61; s <= 62; ++s) { book(e, s * 1000, 70.07, 60, {{6995, 60}}); e.updateZones(s * 1000, 0.08, 0); }
    z = get();
    CHECK(z && z->near && std::fabs(z->nearBase - base1) < 1e-9, "inside the exit hysteresis: same approach, same baseline");
    // price leaves (past 1.5 x the band): baseline cleared
    for (int s = 63; s <= 65; ++s) { book(e, s * 1000, 70.30, 60, {{6995, 60}}); e.updateZones(s * 1000, 0.08, 0); }
    z = get();
    CHECK(z && !z->near && z->nearBase < 0, "price left: no longer near, baseline cleared");
    // the zone shrinks while price is away; price comes back: a NEW baseline = the lots on the new approach
    for (int s = 66; s <= 70; ++s) { book(e, s * 1000, 70.30, 60, {{6995, 20}}); e.updateZones(s * 1000, 0.08, 0); }
    for (int s = 71; s <= 72; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 20}}); e.updateZones(s * 1000, 0.08, 0); }
    z = get();
    CHECK(z && z->near && z->nearBase > 0 && std::fabs(z->nearBase - z->size) < 1e-9 && z->nearBase < base1 + 1e-9, "second approach: a new baseline (the lots now), not the first approach's");
    CHECK(z && cameOf(z->nearBase, z->size, e.P.cameBand).state == CAME_UNCHANGED, "second approach just started: unchanged");
    std::printf("  approach 2: base %.0f\n", z ? z->nearBase : -1);
    // mirrored: an offer zone approached from below
    Engine o = fresh();
    for (int s = 0; s <= 40; ++s) { book(o, s * 1000, 69.70, 60, {}, {{7006, 30}}); o.updateZones(s * 1000, 0.08, 0); }
    const Zone* oz = zoneHolding(o, ASK, 7006);
    CHECK(oz && !oz->near && oz->nearBase < 0, "far offer zone: no baseline");
    for (int s = 41; s <= 42; ++s) { book(o, s * 1000, 70.00, 60, {}, {{7006, 30}}); o.updateZones(s * 1000, 0.08, 0); }
    oz = zoneHolding(o, ASK, 7006);
    CHECK(oz && oz->near && oz->nearBase > 0, "offer zone within reach: baseline set");
    for (int s = 43; s <= 50; ++s) { book(o, s * 1000, 70.00, 60, {}, {{7006, 5}}); o.updateZones(s * 1000, 0.08, 0); }
    oz = zoneHolding(o, ASK, 7006);
    const Came co = oz ? cameOf(oz->nearBase, oz->size, o.P.cameBand) : Came();
    CHECK(co.state == CAME_LEAVING && cameInk(ASK, co.state) == INK_GREEN, "offers pulled as price came: resistance leaving = green");
}
// ---- the makeup CSV: the new columns, recorded at the decision
static void test_makeup_csv() {
    Engine e = fresh();
    const std::vector<std::string> head = split(Engine::makeupHeader(), '|');
    CHECK(head.size() == 21, "makeup header: 21 columns");
    CHECK(Engine::makeupHeader().find("one_order|came_base|came_arrival|came_pct|came|version") != std::string::npos, "new columns: one_order, came_base, came_arrival, came_pct, came");
    CHECK(Engine::makeupHeader().find("bait") == std::string::npos, "no 'bait' column");
    Attempt a; a.side = BID; a.lo = 6993; a.hi = 6997; a.tA = 1000; a.end = "held"; a.tEnd = 9000; a.hitLots = 140; a.rebuiltLots = 126;
    a.mk = mk(4, 312, 265); a.mkSet = true; a.cameBase = 511; a.cameArr = 332;
    std::vector<std::string> f = split(e.makeupLine(a, 9000), '|');
    CHECK(f.size() == head.size(), "makeup line: as many fields as the header");
    if (f.size() == 21) {
        CHECK(f[1] == "support" && f[6] == "140" && f[7] == "126" && f[8] == "90", "side / hit / rebuilt / rebuilt %");
        CHECK(f[15] == "1", "one_order = 1 (85%)");
        CHECK(f[16] == "511" && f[17] == "332" && f[18] == "-35" && f[19] == "leaving", "came: base 511, arrival 332, -35 %, leaving");
        CHECK(f[20] == VERSION, "version");
    }
    a.cameBase = 305; a.cameArr = 366; a.mk = mk(96, 366, 44);
    f = split(e.makeupLine(a, 9000), '|');
    CHECK(f.size() == 21 && f[15] == "0" && f[18] == "20" && f[19] == "building", "came +20 building, not one order");
    a.cameBase = -1; f = split(e.makeupLine(a, 9000), '|');
    CHECK(f.size() == 21 && f[16] == "-1" && f[18] == "0" && f[19] == "none", "no baseline recorded: -1 / 0 / none");
    a.cameBase = 300; a.cameArr = 310; f = split(e.makeupLine(a, 9000), '|');
    CHECK(f.size() == 21 && f[18] == "3" && f[19] == "unchanged", "unchanged keeps its signed %");
}
// ---- the attempt takes the zone's baseline at arrival (engine run: zone near, price trades into it)
static void test_attempt_records_came() {
    Engine e = fresh();
    for (int s = 0; s <= 40; ++s) { book(e, s * 1000, 70.30, 60, {{6995, 30}}); e.updateZones(s * 1000, 0.08, 0); }
    for (int s = 41; s <= 45; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 30}}); e.updateZones(s * 1000, 0.08, 0); }
    const Zone* z = zoneHolding(e, BID, 6995); const double base = z ? z->nearBase : -1;
    for (int s = 46; s <= 50; ++s) { book(e, s * 1000, 70.00, 60, {{6995, 50}}); e.updateZones(s * 1000, 0.08, 0); }
    e.onLastPrice(70.00); e.onTrade(50500, 69.95, 0, 2); e.onTrade(50600, 69.95, 0, 2);
    const Attempt* a = nullptr; for (auto& x : e.attempts) if (x.side == BID) a = &x;
    CHECK(a != nullptr, "fixture: an attempt on the near support zone");
    if (a) {
        CHECK(std::fabs(a->cameBase - base) < 1e-9, "the attempt keeps the approach's baseline");
        CHECK(a->cameArr > base, "and the zone's lots at arrival (it had built)");
        std::printf("  attempt: came base %.0f arrival %.0f -> %s\n", a->cameBase, a->cameArr, cameText(cameOf(a->cameBase, a->cameArr, 0.10)).c_str());
    }
}
// ---- the box: right of the column, never over it or the Delta Profile
static void test_box_placement() {
    CHECK(hoverX(100, 300, 1000) == 112, "box starts 12 px right of the column");
    CHECK(hoverX(100, 888, 1000) == 112, "box that just fits (right edge = the Delta Profile's labels)");
    CHECK(hoverX(100, 889, 1000) == -1, "1 px too wide: no box (never over the Delta Profile)");
    CHECK(hoverX(100, 300, 50) == -1, "Delta Profile left of the column: no box");
    for (int w = 1; w < 900; w += 37) { const int x = hoverX(100, w, 1000); CHECK(x < 0 || (x > 100 && x + w <= 1000), "never over the column, never past the limit"); }
    CHECK(hoverY(500, 80, 0, 1000) == 460, "centred on the zone");
    CHECK(hoverY(990, 80, 0, 1000) == 918, "kept inside the pane bottom");
    CHECK(hoverY(5, 80, 30, 1000) == 30, "kept under the header");
}

int main() {
    test_came_band(); test_came_text_and_colour(); test_hit_and_one_order(); test_px_text();
    test_hover_rows_mockup_states(); test_hover_rows_states(); test_baseline_per_approach(); test_makeup_csv();
    test_attempt_records_came(); test_box_placement();
    std::printf("hover logic: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
