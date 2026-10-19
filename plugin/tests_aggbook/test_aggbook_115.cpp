// test_aggbook_115.cpp -- the Read rows (lsBidAskStats 1.15.0 bas::aggRead, lsBookFlow 1.4.0 bf::bookRead, BoxRead.h wrap / fit),
// SDK-free (2026-10-18, Rassul approved 23:48 CT aggr-read-v2-mockup.png / bookflow-read-mockup.png): every branch, the clauses and
// their thresholds, the stronger-side rule, the wrap (word boundaries, never cut, dropping clauses from the last to fit 2 lines).
#include "BidAskStatsLogic.h"
#include "BookFlowLogic.h"
#include <cstdio>

static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
#define EQS(a, b) do { ++checks; const std::string _a = (a), _b = (b); if (_a != _b) { ++fails; std::printf("FAIL %s:%d  '%s' != '%s'\n", __FILE__, __LINE__, _a.c_str(), _b.c_str()); } } while (0)

static bas::Stat mkst(double rb, double ra, int bbi = 1, int bai = 1, bool okb = true, bool oka = true) {
    bas::Stat s; s.m.ok = true; s.m.bid = 100; s.m.ask = 100; s.bidRatioOk = okb; s.askRatioOk = oka; s.bidRatio = rb; s.askRatio = ra;
    s.bBid = bbi + 1; s.bAsk = bai + 1; return s;   // bBid = 1 + the run (countBiggest), BBI = the run
}
static bas::PaceIn pace(long long b, long long a) { bas::PaceIn p; p.p100[0] = b; p.p100[1] = a; return p; }
static void aggregation() {
    // mockup A: sellers 5.5x, buyers 0.9x, BBI 50+, pace 1.3x -> BEARISH
    bas::Stat a = mkst(5.5, 0.9, 80, 3); a.bBidCap = false;
    br::Read r = bas::aggRead(&a, pace(130, 60));
    CHECK(r.impl == br::BEAR); EQS(r.word(), "BEARISH"); EQS(r.full(), "Sellers hitting 5.5x - biggest bid in 50+, buyers quiet (0.9x)");
    // mockup B: buyers 3.4x fast 2.2x, BAI 24, sellers 0.7x -> BULLISH
    bas::Stat b = mkst(0.7, 3.4, 2, 24);
    r = bas::aggRead(&b, pace(80, 220));
    CHECK(r.impl == br::BULL); EQS(r.full(), "Buyers lifting 3.4x, fast (2.2x) - biggest ask in 24, sellers quiet (0.7x)");
    // mockup C: both >= 2.0x -> NEUTRAL fight
    bas::Stat c = mkst(3.1, 2.9, 12, 10);
    r = bas::aggRead(&c, pace(190, 180)); CHECK(r.impl == br::NEUTRAL); EQS(r.word(), "NEUTRAL"); EQS(r.full(), "Both sides hitting ~3x - fight; watch the close and Book Flow");
    // both < 2.0x -> Quiet
    bas::Stat q = mkst(1.9, 1.2); r = bas::aggRead(&q, pace(300, 300)); CHECK(r.impl == br::NEUTRAL); EQS(r.full(), "Quiet - no side attacking");
    // thresholds: exactly 2.0x attacks; 1.99 does not; the other side 1.5x is not "quiet"; 1.49 is
    bas::Stat t1 = mkst(2.0, 1.5); r = bas::aggRead(&t1, pace(0, 0)); CHECK(r.impl == br::BEAR); EQS(r.full(), "Sellers hitting 2.0x");
    bas::Stat t2 = mkst(2.0, 1.49); r = bas::aggRead(&t2, pace(0, 0)); EQS(r.full(), "Sellers hitting 2.0x, buyers quiet (1.5x)");
    bas::Stat t3 = mkst(1.99, 0.5); r = bas::aggRead(&t3, pace(0, 0)); CHECK(r.impl == br::NEUTRAL);
    // fast only at >= 2.0x pace; biggest only at >= 20; BBI 19 no
    bas::Stat f1 = mkst(2.5, 1.7, 19); r = bas::aggRead(&f1, pace(199, 0)); EQS(r.full(), "Sellers hitting 2.5x");
    bas::Stat f2 = mkst(2.5, 1.7, 20); r = bas::aggRead(&f2, pace(200, 0)); EQS(r.full(), "Sellers hitting 2.5x, fast (2.0x) - biggest bid in 20");
    // n/a ratio never attacks; no data
    bas::Stat na = mkst(5.0, 5.0, 1, 1, false, true); r = bas::aggRead(&na, pace(0, 0)); CHECK(r.impl == br::BULL);
    r = bas::aggRead(nullptr, pace(0, 0)); CHECK(r.impl == br::NEUTRAL); EQS(r.full(), "No volume data for this candle");
}
static bf::NetStat ns(long long nb, double rb, long long no, double ro) { bf::NetStat n; n.ok = true; n.net[0] = nb; n.net[1] = no; n.ratioOk[0] = n.ratioOk[1] = true; n.ratio[0] = rb; n.ratio[1] = ro; return n; }
static void book() {
    bf::NetStat n; br::Read r;
    n = ns(29, 0.1, 377, 1.6); r = bf::bookRead(&n); CHECK(r.impl == br::BEAR); EQS(r.full(), "Sellers stacking offers 1.6x, little support under price (0.1x)");   // mockup A
    n = ns(410, 2.3, -260, 1.8); r = bf::bookRead(&n); CHECK(r.impl == br::BULL); EQS(r.full(), "Bids stacking 2.3x, offers pulled 1.8x - path up opening");          // mockup B
    n = ns(300, 1.6, 320, 1.7); r = bf::bookRead(&n); CHECK(r.impl == br::NEUTRAL); EQS(r.full(), "Both sides stacking ~1.6x - book thickening, wait for the hit");  // mockup C (avg 1.65 -> 1.6)
    n = ns(200, 1.4, 5, 0.2); r = bf::bookRead(&n); CHECK(r.impl == br::BULL); EQS(r.full(), "Bids stacking 1.4x - support arriving");
    n = ns(-200, 1.4, 300, 1.2); r = bf::bookRead(&n); CHECK(r.impl == br::BEAR); EQS(r.full(), "Bids pulled 1.4x, offers stacking 1.2x - path down opening");
    n = ns(-200, 1.4, -2, 3.0); r = bf::bookRead(&n); CHECK(r.impl == br::BEAR); EQS(r.full(), "Bids pulled 1.4x - support leaving");   // offers |net| 2 lots: flat
    n = ns(10, 0.9, -150, 1.3); r = bf::bookRead(&n); CHECK(r.impl == br::BULL); EQS(r.full(), "Offers pulled 1.3x - resistance leaving");
    n = ns(-150, 1.2, -140, 1.1); r = bf::bookRead(&n); CHECK(r.impl == br::NEUTRAL); EQS(r.full(), "Both sides pulling ~1.1x - book thinning");   // (1.15 rounds to 1.1)
    n = ns(5, 0.5, -5, 0.4); r = bf::bookRead(&n); CHECK(r.impl == br::NEUTRAL); EQS(r.full(), "Book quiet - nothing building or leaving");
    // the stronger side decides when both move the same way (>= 1.5x the other)
    n = ns(300, 3.0, 100, 1.5); r = bf::bookRead(&n); CHECK(r.impl == br::BULL); EQS(r.full(), "Bids stacking 3.0x - support arriving");
    n = ns(100, 1.2, 300, 1.8); r = bf::bookRead(&n); CHECK(r.impl == br::BEAR); CHECK(r.full().find("Sellers stacking offers 1.8x") == 0);
    n = ns(-300, 3.0, -100, 1.9); r = bf::bookRead(&n); CHECK(r.impl == br::BEAR); EQS(r.full(), "Bids pulled 3.0x - support leaving");
    n = ns(-100, 1.0, -300, 2.0); r = bf::bookRead(&n); CHECK(r.impl == br::BULL); EQS(r.full(), "Offers pulled 2.0x - resistance leaving");
    // ratio exactly 1.0 moves, 0.99 is flat; no ratio yet = flat; no bid ratio: " - resistance building"
    n = ns(50, 1.0, 50, 0.99); r = bf::bookRead(&n); EQS(r.full(), "Bids stacking 1.0x - support arriving");
    n = ns(50, 2.0, 400, 2.0); n.ratioOk[0] = false; r = bf::bookRead(&n); EQS(r.full(), "Sellers stacking offers 2.0x - resistance building");
    r = bf::bookRead(nullptr); EQS(r.full(), "No order data for this candle");
}
static bf::PullStat mkps(long long ab, double rb, long long pb, double prb, bool pullb, long long ao, double ro, long long po, double pro, bool pullo) {
    bf::PullStat p; p.ok = true; p.add[0] = ab; p.add[1] = ao; p.pull[0] = pb; p.pull[1] = po; p.addR[0] = rb; p.addR[1] = ro; p.pullR[0] = prb; p.pullR[1] = pro;
    p.addOk[0] = p.addOk[1] = p.pullOk[0] = p.pullOk[1] = true; p.pulling[0] = pullb; p.pulling[1] = pullo; return p;
}
static void pulls() {
    // the marker rule: pulled >= 2.0x the plain average of the prior 20 AND >= 1.2x added; 5 prior needed; a partial candle never
    std::vector<long long> a[2], p[2];
    for (int k = 0; k < 20; ++k) for (int s = 0; s < 2; ++s) { a[s].push_back(100); p[s].push_back(80); }
    off::Rec r; r.addC[0] = 100; r.pullC[0] = 160; r.addC[1] = 100; r.pullC[1] = 159;
    bf::PullStat q = bf::pullStatOf(a, p, r);
    CHECK(q.pulling[0] && !q.pulling[1]);                                 // 160 = 2.0x of 80 exactly; 159 just under
    EQS(bf::pullLabel(q, 0), "P 2.0");
    r.addC[0] = 134; q = bf::pullStatOf(a, p, r); CHECK(!q.pulling[0]);  // 160 < 1.2 x 134 (160.8): re-quoting, not pulling
    r.addC[0] = 133; q = bf::pullStatOf(a, p, r); CHECK(q.pulling[0]);   // 160 >= 159.6
    r.partial = true; q = bf::pullStatOf(a, p, r); CHECK(!q.pulling[0]); r.partial = false;
    std::vector<long long> a4[2], p4[2]; for (int k = 0; k < 4; ++k) for (int s = 0; s < 2; ++s) { a4[s].push_back(1); p4[s].push_back(1); }
    q = bf::pullStatOf(a4, p4, r); CHECK(!q.pullOk[0] && !q.pulling[0]);  // fewer than 5 prior candles: no ratio, no marker
    std::vector<long long> a0[2], p0[2]; for (int k = 0; k < 20; ++k) for (int s = 0; s < 2; ++s) { a0[s].push_back(10); p0[s].push_back(0); }
    r.pullC[0] = 2; r.addC[0] = 0; q = bf::pullStatOf(a0, p0, r); CHECK(q.pulling[0] && q.pullR[0] == 2.0);   // a zero average is floored at 1 lot
    r.pullC[0] = 0; q = bf::pullStatOf(a0, p0, r); CHECK(!q.pulling[0]);
    // the Read with pulls
    bf::NetStat n; br::Read rd;
    n = ns(-1773, 2.6, 183, 0.5); bf::PullStat x = mkps(1180, 0.8, 2953, 2.6, true, 1265, 0.9, 1082, 0.9, false);
    rd = bf::bookRead(&n, &x); CHECK(rd.impl == br::BEAR); EQS(rd.full(), "Bids pulled 2.6x, more than added - support leaving under price");   // mockup
    n = ns(-500, 1.5, 600, 2.0); rd = bf::bookRead(&n, &x); CHECK(rd.impl == br::BEAR); EQS(rd.full(), "Sellers stacking offers 2.0x, bids pulled 2.6x - support leaving");
    x = mkps(500, 1.0, 100, 0.5, false, 200, 0.6, 900, 3.1, true);
    n = ns(10, 0.2, -700, 2.5); rd = bf::bookRead(&n, &x); CHECK(rd.impl == br::BULL); EQS(rd.full(), "Offers pulled 3.1x, more than added - resistance leaving, path up opening");
    n = ns(400, 2.2, -700, 2.5); rd = bf::bookRead(&n, &x); CHECK(rd.impl == br::BULL); EQS(rd.full(), "Bids stacking 2.2x, offers pulled 3.1x - path up opening");
    x = mkps(100, 1, 900, 3.0, true, 100, 1, 800, 2.5, true); rd = bf::bookRead(&n, &x); CHECK(rd.impl == br::NEUTRAL); EQS(rd.full(), "Both sides pulling - book thinning, a fast move is likely");
    // pulling >= 1.5x said even when it does not decide (here: below the 1.2x-of-added test)
    x = mkps(900, 2.0, 700, 1.7, false, 100, 1, 50, 0.4, false); n = ns(200, 1.4, 5, 0.2);
    rd = bf::bookRead(&n, &x); CHECK(rd.impl == br::BULL); EQS(rd.full(), "Bids stacking 1.4x - support arriving, bids pulling 1.7x");
    x.pullR[0] = 1.49; rd = bf::bookRead(&n, &x); EQS(rd.full(), "Bids stacking 1.4x - support arriving");
    // never said twice: the net text already names the bids pulled
    x = mkps(100, 1, 150, 1.6, false, 100, 1, 10, 0.2, false); n = ns(-200, 1.4, -2, 3.0);
    rd = bf::bookRead(&n, &x); EQS(rd.full(), "Bids pulled 1.4x - support leaving");
    // no pull data = the stacking rules exactly as before
    n = ns(410, 2.3, -260, 1.8); EQS(bf::bookRead(&n, nullptr).full(), bf::bookRead(&n).full());
}
static void wrapFit() {
    const br::Width W = [](const std::string& t) { return (int)t.size() * 6; };   // 6 px a character
    std::vector<std::string> L = br::wrap("Sellers hitting 5.5x - biggest bid in 50+, buyers quiet (0.9x)", 220, W);
    CHECK(L.size() == 2); for (const auto& l : L) CHECK(W(l) <= 220);
    std::string j; for (const auto& l : L) j += (j.empty() ? "" : " ") + l; EQS(j, "Sellers hitting 5.5x - biggest bid in 50+, buyers quiet (0.9x)");   // nothing cut
    L = br::wrap("Supercalifragilisticexpialidocious word", 60, W); CHECK(L.size() == 2 && L[0] == "Supercalifragilisticexpialidocious");   // a long word stays whole
    br::Read r; r.head = "Buyers lifting 3.4x"; r.opt = { ", fast (2.2x)", " - biggest ask in 24", ", sellers quiet (0.7x)" };
    L = br::fit(r, 150, 2, W);                                            // 25 characters a line: the full read needs 3 -> clauses dropped from the last
    CHECK(L.size() <= 2); std::string k; for (const auto& l : L) { CHECK(W(l) <= 150); k += (k.empty() ? "" : " ") + l; }
    CHECK(k.find("Buyers lifting 3.4x") == 0 && k.find("sellers quiet") == std::string::npos);
    L = br::fit(r, 1000, 2, W); CHECK(L.size() == 1);                    // wide box: everything on one line
    L = br::fit(r, 60, 2, W); CHECK(!L.empty()); for (const auto& l : L) CHECK(l.find("Buyers") == 0 || l.find("lifting") == 0 || l.find("3.4x") == 0);   // head only, its own lines
    EQS(br::approx(3.0), "3"); EQS(br::approx(1.65), "1.6"); EQS(br::x1(2.25), "2.2");
    CHECK(br::Read().color() == br::C_NEUTRAL && br::C_NEUTRAL == 0x00D1D5DB);
}
int main() {
    aggregation(); book(); pulls(); wrapFit();
    std::printf("Read rows: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
