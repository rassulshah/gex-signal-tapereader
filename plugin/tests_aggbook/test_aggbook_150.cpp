// test_aggbook_150.cpp (2026-10-19 / 2026-10-20) - lsBookFlow Defense 1.14.0: Refill Effort / Refilled box rows, the Refilled cut-offs
// per market, the new-extreme spot (spotOf), the REFILL read (refillRead: C+R / strong / weak / none), and the early WATCH / CAUTION read.
#include "BidAskStatsLogic.h"
#include "BookFlowLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
#define EQS(a, b) do { ++checks; const std::string _a = (a), _b = (b); if (_a != _b) { ++fails; std::printf("FAIL %s:%d  '%s' != '%s'\n", __FILE__, __LINE__, _a.c_str(), _b.c_str()); } } while (0)
static off::Rec rec(long long hitB, long long refB, long long hitA, long long refA) {
    off::Rec r; r.hitC[0] = hitB; r.refC[0] = refB; r.hitC[1] = hitA; r.refC[1] = refA; r.addC[0] = 1000; r.pullC[0] = 600; r.addC[1] = 1000; r.pullC[1] = 600; return r;
}
static bf::NetStat ns(double rb, long long netB, double ra, long long netA) {
    bf::NetStat n; n.ok = true; n.net[0] = netB; n.net[1] = netA; n.ratioOk[0] = n.ratioOk[1] = true; n.ratio[0] = rb; n.ratio[1] = ra; return n;
}
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "..";
    // ---- Refilled: ratio, classes per market, no cap ----
    { bf::Refill f = bf::refillOf(rec(100, 430, 0, 0), 0, "ES"); CHECK(f.ok && f.cls == bf::RF_STRONG && f.x > 4.29 && f.x < 4.31); }
    CHECK(bf::refillOf(rec(100, 270, 0, 0), 0, "ES").cls == bf::RF_STRONG);      // exactly at the strong cut
    CHECK(bf::refillOf(rec(100, 269, 0, 0), 0, "ES").cls == bf::RF_NORMAL);
    CHECK(bf::refillOf(rec(100, 170, 0, 0), 0, "ES").cls == bf::RF_WEAK);        // exactly at the weak cut
    CHECK(bf::refillOf(rec(100, 171, 0, 0), 0, "ES").cls == bf::RF_NORMAL);
    CHECK(bf::refillOf(rec(100, 120, 0, 0), 0, "HG").cls == bf::RF_STRONG);      // HG's own cut (1.0x)
    CHECK(bf::refillOf(rec(100, 300, 0, 0), 0, "NQ").cls == bf::RF_NORMAL);      // NQ's strong is 4.5x
    CHECK(bf::refillOf(rec(100, 340, 0, 0), 0, "EU").cls == bf::RF_STRONG && bf::refillOf(rec(100, 340, 0, 0), 0, "6E").cls == bf::RF_STRONG);
    CHECK(bf::refillOf(rec(100, 430, 0, 0), 0, "ZZ").cls == bf::RF_NA);          // unknown market: the ratio, no class
    CHECK(!bf::refillOf(rec(0, 430, 0, 0), 0, "ES").ok);                          // nothing eaten: no Refilled
    CHECK(bf::refillOf(rec(100, -5, 0, 0), 0, "ES").x == 0.0);                    // a negative count reads as 0
    EQS(bf::refillCell(rec(100, 430, 0, 0), 0, "ES"), "2.0x STRONG");   // 4.3 / ES normal 2.2
    EQS(bf::refillCell(rec(100, 120, 0, 0), 0, "ES"), "0.5x weak");
    EQS(bf::refillCell(rec(100, 220, 0, 0), 0, "ES"), "1.0x");             // exactly normal
    EQS(bf::refillCell(rec(100, 430, 0, 0), 0, "ZZ"), "?");                // no normal for an unknown market
    EQS(bf::refillCell(rec(0, 0, 0, 0), 0, "ES"), "-");
    EQS(bf::refillCell(rec(1, 100000, 0, 0), 0, "ES"), "99.9x STRONG");          // capped display
    EQS(bf::refillCell(rec(100, 120, 0, 0), 0, "HG"), "2.0x STRONG");          // HG normal 0.6: 1.2 = twice normal
    { bf::Refill f = bf::refillOf(rec(100, 430, 0, 0), 0, "ES"); CHECK(f.relOk && f.rel > 1.95 && f.rel < 1.96); }
    // ---- the info rows: order and labels; the host keeps Refill Effort + Refilled ----
    { off::PaneBar b; b.ok = true; b.r = rec(2010, 8624, 2131, 9707); off::Params P; bf::NetStat n = ns(2.2, 1377, 4.8, 2587);
      off::Info I = bf::info(&b, P, &n, "ES");
      CHECK(I.rows.size() == 8);
      EQS(I.rows[0].label, "Refill Effort"); EQS(I.rows[1].label, "Eaten"); EQS(I.rows[2].label, "Refilled"); EQS(I.rows[3].label, "Added");
      EQS(I.rows[4].label, "Pulled"); EQS(I.rows[5].label, "Rebuilt"); EQS(I.rows[6].label, "Pace"); EQS(I.rows[7].label, "Crowd");
      EQS(I.rows[0].v[1], "+400 (4.8x)"); EQS(I.rows[1].v[0], "2010"); EQS(I.rows[2].v[0], "2.0x STRONG"); EQS(I.rows[2].v[1], "2.1x STRONG");
      off::Info I0 = bf::info(nullptr, P, nullptr, "ES"); EQS(I0.rows[2].v[0], "n/a"); }
    { const std::string h = slurp(dir + "/BookFlow.cpp");
      CHECK(h.find("return \"Refill Effort;Refilled;Counter\";") != std::string::npos);
      CHECK(h.find("bf::info(mbar, S.params.load.p, shownNs, S.market)") != std::string::npos);
      CHECK(h.find("bf::refillRead(mbar->r, S.market, spotAt(") != std::string::npos);
      CHECK(h.find("1) REFILL EFFORT - Question: how much defense is there compared with normal?") != std::string::npos && h.find("2) REFILLED - Question: did they put back what was taken, compared with normal for this market?") != std::string::npos && h.find("Refilled = 2.0x STRONG") != std::string::npos && h.find("How to read: 1.0x = the usual refill") != std::string::npos);
      CHECK(h.find("bf::netLookOf(pb, ns, s, closed, S.market)") != std::string::npos); }
    // ---- spotOf: new 10-bar low / high, busy ----
    { std::vector<double> lo(21, 100), hi(21, 110); std::vector<long long> v(21, 100);
      lo[20] = 99; hi[20] = 105; v[20] = 250;
      bf::Spot p = bf::spotOf(lo, hi, v); CHECK(p.ok && p.newLow && !p.newHigh && p.busy && p.volX > 2.49 && p.volX < 2.51);
      v[20] = 150; CHECK(!bf::spotOf(lo, hi, v).busy);
      lo[20] = 100; CHECK(bf::spotOf(lo, hi, v).newLow);                     // equal to the 10-bar low counts (<=)
      lo[20] = 101; CHECK(!bf::spotOf(lo, hi, v).newLow);
      lo[12] = 98; lo[20] = 99; CHECK(!bf::spotOf(lo, hi, v).newLow);        // a lower low 8 bars back: not new
      std::vector<double> l9(9, 1), h9(9, 2); std::vector<long long> v9(9, 1); CHECK(!bf::spotOf(l9, h9, v9).ok);   // too few bars
      std::vector<double> l10(10, 100), h10(10, 110); std::vector<long long> v10(10, 1); l10[9] = 90;
      bf::Spot q = bf::spotOf(l10, h10, v10); CHECK(q.ok && q.newLow && !q.busy);   // 10 bars: extreme yes, busy needs 21
      std::vector<long long> vbad(20, 1); CHECK(!bf::spotOf(l10, h10, vbad).ok); }
    // ---- the REFILL read ----
    { bf::Spot sp; sp.ok = true; sp.newHigh = true; sp.busy = true; bf::NetStat n = ns(2.2, 1377, 4.8, 2587); br::Read r;
      CHECK(bf::refillRead(rec(2131, 9707, 2010, 8624), "ES", sp, &n, r));
      CHECK(r.impl == br::BEAR); EQS(r.head, "C+R ? New high: buyers took 2,010, sellers refilled 2.0x normal - defense holding");
      sp.busy = false; CHECK(bf::refillRead(rec(2131, 9707, 2010, 8624), "ES", sp, &n, r));
      EQS(r.head, "New high: sellers refilled 2.0x normal (strong) - defense holding ?");
      sp.busy = true; bf::NetStat n2 = ns(2.2, 1377, 1.5, 2587);               // defending side not 2x: strong, not C+R
      CHECK(bf::refillRead(rec(2131, 9707, 2010, 8624), "ES", sp, &n2, r) && r.head.find("C+R") == std::string::npos);
      bf::NetStat n3 = ns(2.2, 1377, 3.0, -500);                               // defending side pulled more than it added: not C+R
      CHECK(bf::refillRead(rec(2131, 9707, 2010, 8624), "ES", sp, &n3, r) && r.head.find("C+R") == std::string::npos);
      CHECK(bf::refillRead(rec(2131, 9707, 2010, 2412), "ES", sp, &n, r));     // offers 1.2x: weak
      CHECK(r.impl == br::BULL); EQS(r.head, "New high: sellers refilled only 0.5x normal (weak) - defense failing, don't fade ?");
      CHECK(!bf::refillRead(rec(2131, 9707, 2010, 4422), "ES", sp, &n, r));    // 2.2x normal: no refill read
      CHECK(!bf::refillRead(rec(2131, 9707, 2010, 8624), "ZZ", sp, &n, r));    // unknown market: no class, no read
      CHECK(!bf::refillRead(rec(2131, 9707, 0, 0), "ES", sp, &n, r));          // nothing eaten on the defending side
      bf::Spot mid; mid.ok = true; CHECK(!bf::refillRead(rec(2131, 9707, 2010, 8624), "ES", mid, &n, r));   // not at an extreme
      bf::Spot no; CHECK(!bf::refillRead(rec(2131, 9707, 2010, 8624), "ES", no, &n, r));
      bf::Spot lowS; lowS.ok = true; lowS.newLow = true; lowS.busy = true; bf::NetStat nb = ns(2.5, 900, 1.0, 100);
      CHECK(bf::refillRead(rec(1000, 3000, 50, 50), "ES", lowS, &nb, r) && r.impl == br::BULL);
      EQS(r.head, "C+R ? New low: sellers took 1,000, buyers refilled 1.4x normal - defense holding");
      CHECK(bf::refillRead(rec(1000, 1000, 50, 50), "ES", lowS, &nb, r) && r.impl == br::BEAR);
      EQS(r.head, "New low: buyers refilled only 0.5x normal (weak) - defense failing, don't fade ?");
      bf::Spot both; both.ok = true; both.newLow = both.newHigh = true;        // outside bar: the side hit more defends
      CHECK(bf::refillRead(rec(100, 1000, 900, 900), "ES", both, nullptr, r) && r.head.find("New high") == 0);
      CHECK(bf::refillRead(rec(900, 3000, 100, 100), "ES", both, nullptr, r) && r.head.find("New low") == 0); }
    {   // (1.14.0) the early-warning read while the candle forms
        off::Rec r = rec(500, 2000, 50, 50); r.addC[0] = 900; r.pullC[0] = 100; r.addC[1] = 300; r.pullC[1] = 0;   // bids +800, offers +300; refilled 4.0x eaten (strong)
        bf::EwTouch t; t.ok = true; t.s = 0; t.px = 7746.5; t.elapsed = 45; t.h1 = 400; t.h2 = 10;                  // 20 lots/s, then 0.25/s across the complete 20--60 s window: slowing
        br::Read rd;
        CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, rd));                 // incomplete second phase: do not call the 5 s sample a slowdown
        t.elapsed = 60;
        CHECK(bf::earlyRead(t, r, "ES", 0.095, 0.25, rd) && rd.tag == "WATCH" && rd.impl == br::BULL);
        EQS(rd.head, "Bids defending at 7746.50: sellers slowing, buyers 2.7x more than sellers, refilled 1.8x - watch the close ?");
        EQS(rd.word(), "WATCH");
        t.h2 = 0; CHECK(bf::earlyRead(t, r, "ES", 0.095, 0.25, rd) && rd.head.find("sellers stopped hitting") != std::string::npos);
        t.h2 = 300; CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, rd));      // still hitting at 12/s: no read
        t.h2 = 10; t.elapsed = 15; CHECK(!bf::earlyRead(t, r, "ES", 0.095, 0.25, rd));   // too early for either read
        t.elapsed = 60; off::Rec w = r; w.refC[0] = 600; CHECK(!bf::earlyRead(t, w, "ES", 0.095, 0.25, rd));   // refill only 1.2x eaten: not strong
        off::Rec e = r; e.addC[1] = 600; CHECK(bf::earlyRead(t, e, "ES", 0.095, 0.25, rd) && rd.tag == "WATCH");   // bids 800 vs offers 600 = 1.3x: strict net-bigger is enough
        e.addC[1] = 800; CHECK(!bf::earlyRead(t, e, "ES", 0.095, 0.25, rd));   // equal: neither WATCH nor CAUTION
        e.addC[1] = 900; CHECK(bf::earlyRead(t, e, "ES", 0.095, 0.25, rd) && rd.tag == "CAUTION" && rd.head.find("1.1x more above") != std::string::npos);   // offers 900 > bids 800: CAUTION
        off::Rec c = r; c.addC[0] = 300; c.pullC[0] = 100; c.addC[1] = 900;                    // bids +200, offers +900
        CHECK(bf::earlyRead(t, c, "ES", 0.095, 0.25, rd) && rd.tag == "CAUTION" && rd.impl == br::BEAR);
        EQS(rd.head, "Bids hit at 7746.50, but offers stacking 4.5x more above - don't lean on it ?");
        bf::EwTouch h = t; h.s = 1; h.px = 7753; off::Rec rh = rec(50, 50, 500, 2000); rh.addC[1] = 900; rh.addC[0] = 200; rh.pullC[0] = rh.pullC[1] = 0;
        CHECK(bf::earlyRead(h, rh, "ES", 0.095, 0.25, rd) && rd.tag == "WATCH" && rd.impl == br::BEAR && rd.head.find("Offers defending at 7753.00: buyers slowing, sellers 4.5x more than buyers") == 0);
        bf::EwTouch none; CHECK(!bf::earlyRead(none, r, "ES", 0.095, 0.25, rd));
        br::Read plain; plain.impl = br::BULL; EQS(plain.word(), "BULLISH"); }
    EQS(bf::commas(1234567), "1,234,567"); EQS(bf::commas(999), "999"); EQS(bf::commas(-2010), "-2,010");
    std::printf("BookFlow Defense 1.14 logic: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
