// test_qm.cpp -- QueueMeter.h 1.0.0 logic suite (SDK-free).
// From plugin\:  g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror -I. tests_qm/test_qm.cpp -o tqm && ./tqm
#include "QueueMeter.h"
#include <cstdio>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, m); } } while (0)
using namespace qm;
static const long long T0 = 1791536100000LL;   // 2026-10-09 08:55:00 CT (naive epoch ms)
static const double TK = 0.25;
static Tick tk(double px) { return (Tick)(px / TK + 0.5); }

// a quiet market: ATR from 20 candles of 30 points (120 ticks): zone = 0.12 ATR = 14 ticks
static void warm(Engine& E, long long endMs = T0 - 60000) {
    E.market = "NQ"; E.tick = TK;
    for (int i = 20; i >= 1; --i) { const double b = 31050 + (i % 2) * 5; E.closeBar(endMs - i * 180000LL, b + 15, b - 15, b); }
}
static std::string firstQ(const std::vector<std::string>& r) { for (auto& x : r) if (x.rfind("Q|", 0) == 0) return x; return std::string(); }
static size_t nQ(const std::vector<std::string>& r) { size_t n = 0; for (auto& x : r) if (x.rfind("Q|", 0) == 0) ++n; return n; }
static void lvl(Engine& E, double px, const char* name) { Anchor a; a.px = tk(px); a.name = name; std::vector<Anchor> L = E.levels; L.push_back(a); E.setLevels(L); }
static long long oid = 1000;
static long long add(Engine& E, int side, double px, long long sz, long long t) { const long long o = ++oid; E.onEvent(NEW, side, tk(px), sz, o, t); return o; }
static void hitBid(Engine& E, long long o, double px, long long sz, long long t) { E.onEvent(FILL, ASK, tk(px), sz, o, t); }
static void hitAsk(Engine& E, long long o, double px, long long sz, long long t) { E.onEvent(FILL, BID, tk(px), sz, o, t); }

int main() {
    // ---- 1) the book: NEW / MOD / DEL / FILL keep the per-price sums
    {   Engine E; warm(E);
        const long long a = add(E, BID, 31000, 10, T0), b = add(E, BID, 31000, 5, T0);
        CHECK(E.zoneSum(BID, tk(31000), tk(31000)) == 15, "book: 2 bids = 15 lots");
        E.onEvent(MOD, BID, tk(31000), 7, a, T0 + 10);  CHECK(E.zoneSum(BID, tk(31000), tk(31000)) == 12, "book: MOD size down");
        E.onEvent(MOD, BID, tk(30999), 7, a, T0 + 20);  CHECK(E.zoneSum(BID, tk(31000), tk(31000)) == 5 && E.zoneSum(BID, tk(30999), tk(30999)) == 7, "book: MOD re-price moves the size");
        E.onEvent(DEL, BID, tk(31000), 5, b, T0 + 30);  CHECK(E.zoneSum(BID, tk(31000), tk(31000)) == 0 && E.lv[BID].count(tk(31000)) == 0, "book: DEL removes, empty price erased");
        E.onEvent(FILL, ASK, tk(30999), 3, a, T0 + 40); CHECK(E.zoneSum(BID, tk(30999), tk(30999)) == 4, "book: FILL reduces (the FILL row's side is ignored)");
        E.onEvent(MOD, BID, tk(31010), 9, 77777, T0 + 50); CHECK(E.zoneSum(BID, tk(31010), tk(31010)) == 9, "book: a MOD of an unknown order (rested before the feed) is added");
        E.onEvent(FILL, ASK, tk(31010), 2, 99999, T0 + 60); CHECK(E.nUnknownFill == 1, "book: a FILL of an unknown order is counted, not applied");
    }
    // ---- 2) a bid test at a level: start on entry from above, q0, originals (>= 1 s old only), the meter
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        const long long o1 = add(E, BID, 31000, 30, T0 - 5000), o2 = add(E, BID, 30998, 15, T0 - 5000), o3 = add(E, BID, 30999.50, 6, T0 - 300);   // o3 = flicker age at start
        (void)o2; (void)o3;
        const long long m = add(E, BID, 31005, 1, T0 - 4000); hitBid(E, m, 31005, 1, T0 - 100);   // price at 31005 (above the zone top 31000.50)
        CHECK(E.tests.empty(), "no test while price is above the zone");
        hitBid(E, o1, 31000, 4, T0);   // trades into the zone
        CHECK(E.tests.size() == 1, "a bid test starts when price trades into the G1 zone from above");
        if (!E.tests.empty()) {
            const Engine::Test& T = E.tests[0];
            CHECK(T.side == BID && T.kind == K_LEVEL && T.name == "G1" && T.anchor == tk(31000), "the test is the G1 level, bids");
            CHECK(T.q0 == 51, "q0 = everything resting in the zone when the test began (30 + 15 + 6)");
            CHECK(T.origSum == 30, "originals = orders at the defended price (+/- 2 ticks) that rested 1 s+ (the 0.3-s-old 6 lots are flicker, 30998 is outside)");
            CHECK(T.lo == tk(31000) - E.zoneW() && T.hi == tk(31000) + 2, "zone = anchor - 0.12 ATR .. anchor + 2 ticks");
            const Live v = E.snapshot(T, T0 + 1000);
            CHECK(v.q == 47 && std::fabs(v.pct - 47.0 / 51) < 1e-9, "queue left after 4 eaten = 47 / 51");
            CHECK(std::fabs(v.origPct - 26.0 / 30) < 1e-9, "originals left 26 / 30");
            CHECK(std::fabs(v.eat - 4.0) < 1e-9, "eaten 4 lots in 1 s = 4.0 / s");
        }
    }
    // ---- 3) TIME TO EMPTY: pulled + eaten, refills only if they STAY (the 21 s vs 1 s case)
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        std::vector<long long> ids; for (int i = 0; i < 10; ++i) ids.push_back(add(E, BID, 31000 - 0.25 * (i % 3), 4, T0 - 10000));
        const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, ids[0], 31000, 2, T0);   // start
        // over 10 s: 10 lots eaten, 20 lots pulled (rested), and 40 lots added that are pulled again within 0.5 s (flicker)
        for (int s = 0; s < 10; ++s) {
            const long long t = T0 + 100 + s * 1000;
            if (s < 5) hitBid(E, ids[0 + (s < 1 ? 0 : 1)], 31000, 1, t);
            E.onEvent(FILL, ASK, tk(31000), 1, ids[1], t + 1);
            const long long f = add(E, BID, 30999.75, 4, t + 10); E.onEvent(DEL, BID, tk(30999.75), 4, f, t + 400);
            if (s < 5) E.onEvent(DEL, BID, tk(31000 - 0.25 * ((s + 2) % 3)), 4, ids[(size_t)(s + 2)], t + 500);
        }
        if (!E.tests.empty()) {
            const Live v = E.snapshot(E.tests[0], T0 + 10100);
            CHECK(v.refill == 0, "refills pulled again within 1 s never count (neither refill nor pull)");
            CHECK(v.pull > 1.9 && v.pull < 2.1, "pulled = the 20 rested lots / 10 s");
            CHECK(v.tte >= 0 && v.tte < 10, "time to empty from pulled + eaten (the flicker does not stretch it)");
            CHECK(v.high, "break risk HIGH");
            CHECK(v.why == "pulled", "reason: defenders pulled (pulled >= eaten)");
        } else CHECK(false, "the TTE test started");
        // a refill that stays counts
        Engine F; warm(F); lvl(F, 31000, "G1");
        const long long q1 = add(F, BID, 31000, 20, T0 - 10000); const long long mm = add(F, BID, 31006, 1, T0 - 9000); hitBid(F, mm, 31006, 1, T0 - 50);
        hitBid(F, q1, 31000, 5, T0);
        add(F, BID, 30999.75, 10, T0 + 500);   // stays
        if (!F.tests.empty()) {
            const Live a = F.snapshot(F.tests[0], T0 + 1200);   // the refill is 0.7 s old: not yet
            const Live b = F.snapshot(F.tests[0], T0 + 2000);   // 1.5 s old: counts
            CHECK(a.refill == 0 && b.refill > 0, "a refill counts only once it has rested 1 s");
            CHECK(b.tte < 0 && b.why == "refilling", "refills that stay >= drain: holding / refilling");
        } else CHECK(false, "the refill test started");
    }
    // ---- 4) decisions at the candle close: broke = red, held = green, undecided waits, 3 closes = by the queue; recorded once
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        const long long q1 = add(E, BID, 31000, 20, T0 - 10000); const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, q1, 31000, 5, T0);
        CHECK(E.closeBar(T0 + 60000, 31006, 30999, 31000) == 0, "in contact at the close: waits");
        const long long lo = add(E, BID, 30990, 3, T0 + 61000); hitBid(E, lo, 30990, 1, T0 + 62000);   // 30990 < zone low (30996.50) - 2 ticks
        CHECK(!E.tests.empty() && E.tests[0].broke, "a trade 2+ ticks below the zone = broke");
        CHECK(E.closeBar(T0 + 240000, 31001, 30989, 31000) == 1 && nQ(E.rows) == 1, "decided at the next close: one Q row");
        Engine::QRec r; CHECK(nQ(E.rows) > 0 && Engine::parseRec(firstQ(E.rows), r) && r.g == 0 && r.why == "broke", "red Q: broke");
        CHECK(E.closeBar(T0 + 420000, 31001, 30989, 31000) == 0, "never decided twice");
    }
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        const long long q1 = add(E, BID, 31000, 20, T0 - 10000); const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, q1, 31000, 5, T0);
        const long long up = add(E, ASK, 31012, 3, T0 + 1000); hitAsk(E, up, 31012, 1, T0 + 2000);   // 31012 > zone top 31000.50 + 0.25 ATR (7.5)
        CHECK(!E.tests.empty() && E.tests[0].left, "price left the zone on the defenders' side = held");
        CHECK(E.closeBar(T0 + 60000, 31012, 30999, 31010) == 1, "decided at the close");
        Engine::QRec r; CHECK(nQ(E.rows) > 0 && Engine::parseRec(firstQ(E.rows), r) && r.g == 1 && r.why == "held" && r.name == "G1" && r.kind == K_LEVEL, "green Q: held, named G1");
    }
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        const long long q1 = add(E, BID, 31000, 20, T0 - 10000); const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, q1, 31000, 2, T0);
        E.closeBar(T0 + 60000, 31006, 30999, 31000); E.closeBar(T0 + 240000, 31003, 30999, 31000);
        CHECK(nQ(E.rows) == 0, "two closes in contact: still waiting");
        CHECK(E.closeBar(T0 + 420000, 31003, 30999, 31000) == 1, "the 3rd close decides by the queue");
        Engine::QRec r; CHECK(nQ(E.rows) > 0 && Engine::parseRec(firstQ(E.rows), r) && r.g == 1 && r.why == "refilled", "90% left, originals there = green 'refilled'");
    }
    // ---- 5) no look-ahead: a break AFTER the close is not used for that close
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        const long long q1 = add(E, BID, 31000, 20, T0 - 10000); const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, q1, 31000, 2, T0);
        const long long lo = add(E, BID, 30990, 3, T0 + 61000); hitBid(E, lo, 30990, 1, T0 + 61000);   // breaks 1 s AFTER the 60-s close
        CHECK(E.closeBar(T0 + 60000, 31006, 30999, 31000) == 0, "a break after the candle's end does not decide that candle");
    }
    // ---- 6) the record: round trip, torn / garbled lines, restart adopts (never twice, never changed)
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        const long long q1 = add(E, BID, 31000, 20, T0 - 10000); const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, q1, 31000, 5, T0);
        const long long lo = add(E, BID, 30990, 3, T0 + 1000); hitBid(E, lo, 30990, 1, T0 + 2000);
        E.closeBar(T0 + 60000, 31006, 30989, 31000);
        CHECK(nQ(E.rows) == 1, "one Q");
        const std::string line = firstQ(E.rows);
        Engine::QRec r; CHECK(Engine::parseRec(line, r) && r.anchor == tk(31000) && r.side == BID && r.startBar == T0 + 60000, "round trip");
        CHECK(!Engine::parseRec(line.substr(0, line.size() / 2), r), "a torn line is refused");
        CHECK(!Engine::parseRec("Q|x|1|B|1|G|held|1|1|1|L|G1|1", r), "a garbled number is refused");
        CHECK(!Engine::parseRec("Q|1|1|B|1|Y|held|1|1|1|L|G1|1", r), "a bad G/R is refused");
        // restart: a new engine adopts the record; the same test again (same anchor, side, start candle) is never written twice
        Engine F; warm(F); lvl(F, 31000, "G1");
        CHECK(F.adopt(std::vector<std::string>{ Engine::header(), line, "garbage", line.substr(0, 10) }) == 1, "adopt: 1 good line (header / garbage / torn skipped)");
        const long long q2 = add(F, BID, 31000, 20, T0 - 10000); const long long m2 = add(F, BID, 31006, 1, T0 - 9000); hitBid(F, m2, 31006, 1, T0 - 50);
        hitBid(F, q2, 31000, 5, T0 + 3000);
        const long long lo2 = add(F, BID, 30990, 3, T0 + 4000); hitBid(F, lo2, 30990, 1, T0 + 5000);
        CHECK(F.closeBar(T0 + 60000, 31006, 30989, 31000) == 0 && nQ(F.rows) == 0, "restart: the recorded Q is not written again");
    }
    // ---- 7) the nightly norms: strict, per market, used per slot; else this session's
    {   Norms n = parseNorms({ "VERSION,1", "MARKET,NQ", "SLOT,0830,15.9,14.2", "SLOT,0900,12.0,11.0" }, "NQ");
        CHECK(n.ok && n.slot[17][BID] == 15.9 && n.slot[18][ASK] == 11.0, "norms parsed into the 08:30 / 09:00 slots");
        CHECK(!parseNorms({ "VERSION,1", "MARKET,ES", "SLOT,0830,15.9,14.2" }, "NQ").ok, "wrong market refused");
        CHECK(!parseNorms({ "MARKET,NQ", "SLOT,0830,15.9,14.2" }, "NQ").ok, "no VERSION first refused");
        CHECK(!parseNorms({ "VERSION,1", "MARKET,NQ", "SLOT,0845,15.9,14.2" }, "NQ").ok, "off-grid slot refused");
        CHECK(!parseNorms({ "VERSION,1", "MARKET,NQ", "SLOT,0830,nan,14.2" }, "NQ").ok, "NaN refused");
        CHECK(!parseNorms({ "VERSION,1", "MARKET,NQ", "SLOT,0830,-1,14.2" }, "NQ").ok, "negative refused");
        Engine E; warm(E); E.norms = n;
        CHECK(E.normal(BID, T0) == 15.9 && E.normSrc(BID, T0) == "nightly", "the nightly normal for 08:55 (slot 08:30)");
        Engine F; warm(F);
        CHECK(F.normal(BID, T0) == 0 && F.normSrc(BID, T0) == "learning", "no file, no fills yet: learning (0)");
        for (int s = 0; s < 40; ++s) { const long long o = add(F, BID, 31080, 5, T0 + s * 1000); hitBid(F, o, 31080, 3, T0 + s * 1000 + 10); }
        CHECK(std::fabs(F.normal(BID, T0 + 40000) - 3.0) < 1e-9 && F.normSrc(BID, T0 + 40000) == "this session, this half hour", "the session's own: 3 lots per active second");
    }
    // ---- 8) the live file: text -> parse round trip; garbled / wrong market refused
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        const long long q1 = add(E, BID, 31000, 20, T0 - 10000); const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, q1, 31000, 5, T0);
        const std::string tx = fileText(E, T0 + 1000, 31000.0);
        std::vector<std::string> L = split(tx, '\n');
        File f = parseFile(L, "NQ");
        CHECK(f.ok && f.t.size() == 1, "live file round trip: one test");
        if (f.ok && !f.t.empty()) {
            const FileT& t = f.t[0];
            CHECK(t.side == BID && t.anchor == 31000.0 && t.name == "G1" && t.q0 == 20 && t.q == 15 && std::fabs(t.pct - 0.75) < 1e-9, "the row: G1 bids 15 / 20 = 75%");
            CHECK(f.last == 31000.0 && f.atr > 29 && f.atr < 31 && f.tick == 0.25, "LAST / ATR / TICK");
            CHECK(badgeText("G1", t, 0) == "G1 Q75%", "badge text 'G1 Q75%'");
            CHECK(std::fabs(fillFrac(t) - 0.75) < 1e-9, "fill = 75% of the badge");
        }
        CHECK(!parseFile(L, "ES").ok, "wrong market refused");
        std::vector<std::string> G = L; G[0] = "MARKET,NQ"; CHECK(!parseFile(G, "NQ").ok, "no VERSION first refused");
        std::vector<std::string> H = L; for (auto& x : H) if (x.rfind("T,", 0) == 0) x = x.substr(0, x.rfind(',')); CHECK(!parseFile(H, "NQ").ok, "a torn T row refused");
        FileT w; w.pct = 0.33; w.warn = true;
        CHECK(badgeText("", w, 0) == "Q33% . orig 0%" && badgeText("", w, 1) == "Q33%" && badgeText("9:24 low", w, 2) == "Q33%", "warning text and the shorter fits");
        FileT big; big.pct = 1.4; CHECK(fillFrac(big) == 1.0 && badgeText("G1", big, 0) == "G1 Q140%", "more than the start: full fill, true number");
        const Hover h = hover(f.t.empty() ? w : f.t[0], "G1", "31000.00");
        CHECK(h.title.find("?") != std::string::npos && h.title.find("not proven yet") != std::string::npos, "hover header carries ? (not proven yet)");
        bool order = false; size_t iq = 0, iw = 0, iy = 0, ih = 0, ie = 0;
        for (size_t i = 0; i < h.lines.size(); ++i) { const std::string& x = h.lines[i];
            if (x.rfind("Question:", 0) == 0) iq = i;
            if (x.rfind("What it is:", 0) == 0) iw = i;
            if (x.rfind("Why it matters:", 0) == 0) iy = i;
            if (x.rfind("How to use:", 0) == 0) ih = i;
            if (x.rfind("Example:", 0) == 0) ie = i; }
        order = iq > 0 && iq < iw && iw < iy && iy < ih && ih < ie;
        CHECK(order, "hover: question / what it is / why it matters / how to use / example");
        CHECK(h.lines.size() > 6 && h.lines[0].rfind("Queue left:", 0) == 0 && h.lines[6].rfind("Break risk:", 0) == 0, "hover numbers first: queue left .. break risk");
    }
    // ---- 9) pivots: a 1-left / 2-right low becomes an anchor; a close through it removes it; a test at it is a K_PIVOT (unlabeled)
    {   Engine E; E.market = "NQ"; E.tick = TK;
        long long t = T0 - 40 * 180000LL;
        for (int i = 0; i < 20; ++i) E.closeBar(t += 180000, 31060, 31030, 31045);
        E.closeBar(t += 180000, 31050, 31029, 31040);   // left
        E.closeBar(t += 180000, 31045, 31026.5, 31040); // the pivot low
        E.closeBar(t += 180000, 31050, 31038.75, 31045);
        E.closeBar(t += 180000, 31055, 31041.25, 31050); // confirmed
        bool have = false; for (auto& a : E.pivots) if (a.side == BID && a.px == tk(31026.5)) have = true;
        CHECK(have, "pivot low 31026.50 found after 2 candles to the right");
        const long long q1 = add(E, BID, 31026.5, 10, t + 1000); const long long m = add(E, BID, 31035, 1, t + 1000); hitBid(E, m, 31035, 1, t + 5000);
        hitBid(E, q1, 31026.5, 2, t + 6000);
        CHECK(E.tests.size() == 1 && E.tests[0].kind == K_PIVOT && E.tests[0].name.empty(), "a test at the pivot low: unlabeled");
        E.closeBar(t += 180000, 31030, 31000, 31010);   // closes through
        have = false; for (auto& a : E.pivots) if (a.side == BID && a.px == tk(31026.5)) have = true;
        CHECK(!have, "a close through the pivot removes it");
    }
    // ---- 10) a suddenly defended price (no level, no pivot): a new 2-min low into a queue 2.5x+ the typical size
    {   Engine E; warm(E);
        for (int i = 0; i < 40; ++i) add(E, BID, 31030 - 0.25 * i, 2, T0 - 20000);   // a book of 2 lots a tick 31030 .. 31020.25 (bids below price)
        const long long big = add(E, BID, 31030, 80, T0 - 15000);
        const long long m = add(E, BID, 31045, 1, T0 - 15000); hitBid(E, m, 31045, 1, T0 - 10000);
        hitBid(E, big, 31030, 1, T0);
        CHECK(E.tests.size() == 1 && E.tests[0].kind == K_DEFENDED && E.tests[0].anchor == tk(31030), "support that suddenly appears = a defended-price test (unlabeled)");
        Engine F; warm(F);
        for (int i = 0; i < 40; ++i) add(F, BID, 31030 - 0.25 * i, 2, T0 - 20000);
        const long long m2 = add(F, BID, 31045, 1, T0 - 15000); hitBid(F, m2, 31045, 1, T0 - 10000);
        const long long o = add(F, BID, 31030, 2, T0 - 1000); hitBid(F, o, 31030, 1, T0);
        CHECK(F.tests.empty(), "a normal book at a new low: no test");
    }
    // ---- 11) offers mirror: a test of offers defending a high
    {   Engine E; warm(E); lvl(E, 31100, "HVL0");
        const long long a1 = add(E, ASK, 31100, 25, T0 - 10000); const long long m = add(E, ASK, 31094, 1, T0 - 9000); hitAsk(E, m, 31094, 1, T0 - 50);
        hitAsk(E, a1, 31100, 5, T0);
        CHECK(E.tests.size() == 1 && E.tests[0].side == ASK && E.tests[0].hi == tk(31100) + E.zoneW() && E.tests[0].lo == tk(31100) - 2, "offers: zone anchor - 2 ticks .. anchor + 0.12 ATR");
        const long long hi = add(E, ASK, 31110, 3, T0 + 1000); hitAsk(E, hi, 31110, 1, T0 + 2000);
        CHECK(!E.tests.empty() && E.tests[0].broke, "offers broke: traded 2+ ticks above the zone");
    }
    // ---- 12) the clean-up after price left is not a pull; one test per anchor; a new session starts clean
    {   Engine E; warm(E); lvl(E, 31000, "G1");
        std::vector<long long> ids; for (int i = 0; i < 5; ++i) ids.push_back(add(E, BID, 30999, 4, T0 - 10000));
        const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, ids[0], 30999, 1, T0);
        const long long up = add(E, ASK, 31020, 3, T0 + 1000); hitAsk(E, up, 31020, 1, T0 + 1500);   // far away: no contact
        for (int i = 1; i < 5; ++i) E.onEvent(DEL, BID, tk(30999), 4, ids[(size_t)i], T0 + 2000);
        CHECK(!E.tests.empty() && E.snapshot(E.tests[0], T0 + 3000).pull == 0, "orders pulled after price left are not counted as pulled");
        bool both = false; for (auto& T : E.tests) if (T.side == ASK && T.anchor == tk(31000)) both = true;
        CHECK(!both, "never a bid AND an offer test on the same level at once");
        const long long nextSession = (E.sessionOf(T0) + 1) * 86400000LL - 7 * 3600000LL + 1000;   // just after 17:00 CT
        add(E, BID, 30000, 1, nextSession);
        CHECK(E.tests.empty() && E.normal(BID, nextSession) == 0, "a new session: tests and the session's normal reset");
        CHECK(E.atr > 0, "the candles (ATR) are kept across the session change");
    }
    // ---- 13) (1.1.0) the 10-s CALL, per market rule; params strict; file fields; badge "?"; outline
    {   CallRule es = callRuleFor("ES"), nq = callRuleFor("NQ"), hg = callRuleFor("HG");
        CHECK(es.mark == M_SHOW && es.greenShow && nq.mark == M_SHOWQ && !nq.greenShow && hg.mark == M_LOG, "defaults: ES SHOW red+green, NQ SHOW? red only, HG LOG");
        bool ok = false;
        CallRule p = parseCallRule({ "VERSION,1", "MARKET,GC", "# c", "PARAM,MARK,SHOW?", "PARAM,CALL_AT_S,10", "PARAM,RED_PCT,0.40", "PARAM,GREEN_PCT,1.00", "PARAM,GREEN_NEEDS_HOLDING,1", "PARAM,GREEN_SHOW,0", "PARAM,Q_CLOSE_RECORD,LOG" }, "GC", ok);
        CHECK(ok && p.mark == M_SHOWQ && p.callAtS == 10 && p.redPct == 0.40 && !p.greenShow, "the GC params file (as written by the validation)");
        CallRule w = parseCallRule({ "VERSION,1", "MARKET,ES", "PARAM,MARK,SHOW" }, "CL", ok);
        CHECK(!ok && w.mark == M_SHOW && w.src.find("wrong market") != std::string::npos, "wrong market: CL defaults");
        w = parseCallRule({ "VERSION,1", "MARKET,ES", "PARAM,RED_PCT,1.7" }, "ES", ok); CHECK(!ok && w.redPct == 0.40, "garbled RED_PCT: defaults");
        w = parseCallRule({ "VERSION,1", "MARKET,ES", "PARAM,MARK,MAYBE" }, "ES", ok); CHECK(!ok, "garbled MARK refused");
        // red: the queue under 40% at 10 s, still in the zone
        Engine E; warm(E); E.market = "ES"; lvl(E, 31000, "G1"); E.rule = callRuleFor("ES");
        const long long q1 = add(E, BID, 31000, 20, T0 - 10000), q2 = add(E, BID, 30999, 20, T0 - 10000); (void)q2;
        const long long m = add(E, BID, 31006, 1, T0 - 9000); hitBid(E, m, 31006, 1, T0 - 50);
        hitBid(E, q1, 31000, 5, T0);
        E.onEvent(DEL, BID, tk(30999), 20, q2, T0 + 3000);   // pulled: 15 of 40 left = 38%
        E.stepCalls(T0 + 9000); CHECK(!E.tests.empty() && E.tests[0].call == -1, "no call before 10 s");
        E.stepCalls(T0 + 10000);
        CHECK(!E.tests.empty() && E.tests[0].call == 0, "RED at 10 s: 38% left");
        bool cl = false; for (auto& r : E.rows) if (r.rfind("C|", 0) == 0 && r.find("|B|124000|R|38|") != std::string::npos) cl = true;
        CHECK(cl, "C line written: C|t0|t|B|anchor|R|38|...");
        const size_t nr = E.rows.size(); E.stepCalls(T0 + 20000); CHECK(E.rows.size() == nr && E.tests[0].call == 0, "decided once (never repainted)");
        File f = parseFile(split(fileText(E, T0 + 20000, 31000.0), '\n'), "NQ");
        CHECK(!f.ok, "file of ES read as NQ: refused");
        f = parseFile(split(fileText(E, T0 + 20000, 31000.0), '\n'), "ES");
        CHECK(f.ok && f.t.size() == 1 && f.t[0].call == 0 && f.t[0].mark == M_SHOW && outlineCall(f.t[0]) == 0, "file round trip: call RED, mark SHOW, red outline");
        FileT q = f.t.empty() ? FileT() : f.t[0]; q.mark = M_SHOWQ; CHECK(badgeText("G1", q, 0).back() == '?', "SHOW?: the badge text ends with '?'");
        q.mark = M_LOG; CHECK(outlineCall(q) == -1 && badgeText("G1", q, 0).back() != '?', "LOG market: no call outline, no '?'");
        const Hover h = hover(f.t.empty() ? q : f.t[0], "G1", "31000.00", "ES"); bool odds = false; for (auto& x : h.lines) if (x.find("broke 76% of the time (base 30%)") != std::string::npos) odds = true;
        CHECK(odds && h.title.find('?') == std::string::npos, "hover: ES odds line; no '?' in a SHOW market's header");
        // green: 100%+ and holding, only where GREEN_SHOW
        for (int pass = 0; pass < 2; ++pass) {
            Engine G; warm(G); lvl(G, 31000, "G1"); G.rule = callRuleFor(pass == 0 ? "ES" : "NQ");
            const long long g1 = add(G, BID, 31000, 20, T0 - 10000); const long long gm = add(G, BID, 31006, 1, T0 - 9000); hitBid(G, gm, 31006, 1, T0 - 50);
            hitBid(G, g1, 31000, 2, T0);
            add(G, BID, 30999.75, 10, T0 + 5000);   // refill that stays: 28 / 20 = 140%
            G.stepCalls(T0 + 10000);
            CHECK(!G.tests.empty() && G.tests[0].call == (pass == 0 ? 1 : 2), pass == 0 ? "ES GREEN: 140% left and holding" : "NQ: no green (red only) -> none");
        }
        // broke before the call -> none
        Engine B; warm(B); lvl(B, 31000, "G1"); B.rule = callRuleFor("CL");
        const long long b1 = add(B, BID, 31000, 20, T0 - 10000); const long long bm = add(B, BID, 31006, 1, T0 - 9000); hitBid(B, bm, 31006, 1, T0 - 50);
        hitBid(B, b1, 31000, 2, T0); const long long lo = add(B, BID, 30990, 3, T0 + 1000); hitBid(B, lo, 30990, 1, T0 + 2000);
        B.stepCalls(T0 + 10000); CHECK(!B.tests.empty() && B.tests[0].call == 2, "broke before 10 s: no call (none)");
    }
    std::printf("test_qm: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
