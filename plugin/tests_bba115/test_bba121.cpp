// test_bba121.cpp -- lsBigBidAsk 1.21.0 BREAKOUT / BREAKDOWN (Rassul approved breakout-mockup-v2 at 20:10 CT 2026-10-21:
// "no dashed pivot line. yes build").
//   [logic]  bba::breakFlagsPx (the pivot price traded through, the higher / lower of the last two), bba::breakoutAt: through the
//            pivot yes / no, the 10-candle high / low, range >= prior, volume or range up, the BD mirror, BO judged first, the print
//            bigger / not bigger than the prior candle's, biggest in N and its cap 50, waiting for unknown candles; the BBO| record
//            round trip and garbage refused
//   [nq]     the NQ 2026-10-21 3-min candles the mockup was made from (nq_2026-10-21_3m.txt, make_nq1021_fixture.py): the logic
//            reproduces bo.py's 20 events from 8:33 AM to 3:00 PM (8:33 BO 80, 8:45 BD 72, 10:48 BO 124, 11:42 BO 110 ...)
//   [host]   the same day on the mock chart: the BBO| record (fired or not + reason), the print ON the candle at its price, BO above /
//            BD below, a Big Bid/Ask print drawn once, no pivot line, the keep-out of the HOD/LOD box, the skip first / last minutes,
//            the setting, no repaint across recomputes and new bars, a restart redraws from the record, the status BREAKOUTS line
// Build (from tests_bba115/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_bba121.cpp -o /tmp/t_bba121 && /tmp/t_bba121
#include "irtsdk.h"
#include "../BigBidAsk.cpp"
#include <iostream>
#include <sys/stat.h>
#include <set>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)

// ---- [logic] -------------------------------------------------------------------------------------------------------------------
static std::vector<bba::BoIn> mkIn(const std::vector<double>& h, const std::vector<double>& l, double tick = 0.25) {
    std::vector<bba::BoIn> v(h.size());
    for (size_t k = 0; k < h.size(); ++k) { v[k].known = true; v[k].hk = std::llround(h[k] / tick); v[k].lk = std::llround(l[k] / tick); v[k].vol = 100; v[k].big[0] = v[k].big[1] = 10; v[k].bigKey[0] = v[k].lk; v[k].bigKey[1] = v[k].hk; }
    return v;
}
static void logic() {
    // the ZigZag: top 105 (bar 2), bottom 99 (bar 4), top 104 (bar 6), bottom 99 (bar 8); bar 10 goes through 105 (the higher, 2 back)
    std::vector<double> h = { 100, 103, 105, 104, 102, 103, 104, 103, 101, 102, 107 }, l = { 98, 100, 102, 101, 99, 100, 101, 100, 99, 100, 103 };
    std::vector<signed char> up, dn, up0, dn0; std::vector<double> upPx, dnPx;
    bba::breakFlagsPx(h, l, (int)h.size(), up, dn, &upPx, &dnPx);
    bba::breakFlags(h, l, (int)h.size(), up0, dn0);
    CHECK(up[10] == 2 && upPx[10] == 105 && up == up0 && dn == dn0, "[logic] breakFlagsPx: bar 10 trades through the higher pivot 2 back (105); breakFlags unchanged");
    { std::vector<double> h2 = h, l2 = l; l2[10] = 105.25; h2[10] = 107; std::vector<signed char> u2, d2; std::vector<double> p2, q2;
      bba::breakFlagsPx(h2, l2, (int)h2.size(), u2, d2, &p2, &q2); CHECK(u2[10] == 0 && p2[10] == 0, "[logic] a candle entirely above the pivot (gap) did not trade THROUGH it: no candidate"); }
    std::vector<bba::BoIn> in = mkIn(h, l);
    in[9].vol = 100; in[10].vol = 150; in[10].big[1] = 60; in[10].bigKey[1] = std::llround(106 / 0.25); in[9].big[1] = 40;
    bool wait = false;
    bba::BoDec D = bba::breakoutAt(in, up, dn, upPx, dnPx, 10, wait);
    CHECK(!wait && D.fired && D.dir == 1 && D.reason == bba::BO_R_FIRED && D.piv == 105 && D.which == 2, "[logic] BO: through 105, 10-candle high, range 4 >= 2, volume up -> fired");
    CHECK(D.print && D.lots == 60 && D.prior == 40 && D.key == 424 && D.n == 10, "[logic] the print: 60 bought at 106 > the prior candle's 40; biggest in 10 (every earlier candle 10 / 40)");
    CHECK(D.range == 16 && D.prange == 8 && D.vol == 150 && D.pvol == 100, "[logic] range / volume in ticks / lots vs the prior candle");
    { std::vector<signed char> u = up; u[10] = 0; bba::BoDec E = bba::breakoutAt(in, u, dn, upPx, dnPx, 10, wait); CHECK(!wait && E.dir == 0 && !E.fired, "[logic] no pivot traded through: not a candidate"); }
    { std::vector<bba::BoIn> x = in; x[3].hk = std::llround(107.25 / 0.25); bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(!E.fired && E.dir == 1 && E.reason == bba::BO_R_NOT_EXTREME, "[logic] a higher high 7 candles back: not the 10-candle high"); }
    { std::vector<bba::BoIn> x = in; x[0].hk = std::llround(110 / 0.25); bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(E.fired, "[logic] a higher high 10 candles back is outside the 10 (itself included)"); }
    { std::vector<bba::BoIn> x = in; x[3].hk = x[10].hk; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait); CHECK(E.fired, "[logic] an EQUAL high in the 10 still counts as the highest"); }
    { std::vector<bba::BoIn> x = in; x[9].lk = x[10].lk - 24; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(!E.fired && E.reason == bba::BO_R_RANGE, "[logic] range below the prior candle's: not fired"); }
    { std::vector<bba::BoIn> x = in; x[9].hk = x[9].lk + 16; x[10].vol = 100; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(!E.fired && E.reason == bba::BO_R_VOLRANGE, "[logic] range EQUAL to the prior and volume not up: not fired"); }
    { std::vector<bba::BoIn> x = in; x[9].hk = x[9].lk + 16; x[10].vol = 101; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(E.fired, "[logic] range equal + volume up: fired"); }
    { std::vector<bba::BoIn> x = in; x[10].vol = 50; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(E.fired, "[logic] range up + volume DOWN: fired (volume OR range)"); }
    { std::vector<bba::BoIn> x = in; x[10].big[1] = 40; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(E.fired && !E.print && E.n == 0, "[logic] the print EQUAL to the prior candle's: no print (letter only), biggest in 0"); }
    { std::vector<bba::BoIn> x = in; x[10].big[1] = 0; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(E.fired && !E.print && E.lots == 0, "[logic] no buy cell at all: letter only"); }
    { std::vector<bba::BoIn> x = in; x[6].big[1] = 60; bba::BoDec E = bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(E.print && E.n == 3, "[logic] biggest in N stops at an equal cell 4 back -> 3"); }
    { std::vector<bba::BoIn> x = in; x[8].known = false; (void)bba::breakoutAt(x, up, dn, upPx, dnPx, 10, wait);
      CHECK(wait, "[logic] a candle in the 10 not final yet: wait (decided later, never half-judged)"); }
    {   // the cap: 70 candles, the breakout at the end; every earlier candle smaller
        std::vector<double> H(70), L(70);
        for (int k = 0; k < 70; ++k) { H[(size_t)k] = 100 + (k % 4 == 1 ? 2 : 0) + (k % 8 == 5 ? 1 : 0); L[(size_t)k] = H[(size_t)k] - 2; }
        H[69] = 110; L[69] = 101;
        std::vector<signed char> u, d; std::vector<double> pu, pd; bba::breakFlagsPx(H, L, 70, u, d, &pu, &pd);
        std::vector<bba::BoIn> x = mkIn(H, L); x[69].big[1] = 500; x[69].vol = 500;
        bba::BoDec E = bba::breakoutAt(x, u, d, pu, pd, 69, wait);
        CHECK(u[69] != 0 && E.fired && E.print && E.n == 50, "[logic] biggest in N capped at 50 (69 smaller candles before it)");
    }
    {   // BD mirror: bottoms 95 (bar 2) and 96 (bar 6): the LOWER is 95; bar 10 goes through it
        std::vector<double> H = { 100, 98, 97, 99, 101, 99, 98, 100, 102, 101, 99 }, L = { 98, 96, 95, 97, 99, 97, 96, 98, 100, 99, 93 };
        std::vector<signed char> u, d; std::vector<double> pu, pd; bba::breakFlagsPx(H, L, (int)H.size(), u, d, &pu, &pd);
        std::vector<bba::BoIn> x = mkIn(H, L); x[10].vol = 200; x[10].big[0] = 77; x[9].big[0] = 30;
        bba::BoDec E = bba::breakoutAt(x, u, d, pu, pd, 10, wait);
        CHECK(d[10] == 2 && pd[10] == 95 && E.fired && E.dir == -1 && E.piv == 95 && E.which == 2, "[logic] BD: through the lower pivot 2 back (95), 10-candle low -> fired");
        CHECK(E.print && E.lots == 77 && E.prior == 30 && E.n == 10, "[logic] BD print = the biggest SELL cell (77 > 30)");
        x[3].lk = std::llround(92.75 / 0.25); E = bba::breakoutAt(x, u, d, pu, pd, 10, wait);
        CHECK(!E.fired && E.dir == -1 && E.reason == bba::BO_R_NOT_EXTREME, "[logic] BD: a lower low in the 10 -> not fired");
    }
    {   // both pivots traded through by one outside candle that makes both extremes: BO first
        std::vector<signed char> u(11, 0), d(11, 0); std::vector<double> pu(11, 0), pd(11, 0); u[10] = 1; d[10] = 1; pu[10] = 105; pd[10] = 99;
        std::vector<bba::BoIn> x = mkIn(h, l); x[10].hk = std::llround(108 / 0.25); x[10].lk = std::llround(97 / 0.25); x[10].vol = 300;
        bba::BoDec E = bba::breakoutAt(x, u, d, pu, pd, 10, wait); CHECK(E.fired && E.dir == 1, "[logic] an outside candle through both: BO judged first");
    }
    {   // the record line
        bba::BoDec R = D; R.t = 1792575180; R.t0 = R.t; std::string ln = bba::boLine(R, 1, "1.23.0"); bba::BoDec Q;   // (1.23.0) 24 fields, t0 = breakout candle
        CHECK(bba::parseBoLine(ln, Q) && Q.t == R.t && Q.dir == 1 && Q.fired && Q.reason == bba::BO_R_FIRED && Q.piv == 105 && Q.which == 2 && Q.lots == 60 && Q.key == 424 && Q.n == 10 && Q.print && Q.prior == 40 && Q.vol == 150 && Q.pvol == 100,
              "[logic] BBO| line round trip");
        CHECK(ln.compare(0, 4, "BBO|") == 0 && std::count(ln.begin(), ln.end(), '|') == 23 && std::string(bba::BO_HEADER).find("BBO|t|t0|dir|fired|reason") == 0, "[logic] 24 fields (1.23.0), header names them");
        bba::BoDec T2 = R; T2.fired = false; T2.reason = bba::BO_R_TIME; CHECK(bba::parseBoLine(bba::boLine(T2, 1, "1.23.0"), Q) && !Q.fired && Q.reason == bba::BO_R_TIME, "[logic] a not-fired decision keeps its reason");
        const char* bad[] = { "", "BBO|", "garbage", "BBO|1|2|1|fired|1|1|1|1|1|1|1|1|1|1|1|1|1|1|x", "BBO|1|1|0|fired|1|1|1|1|1|1|1|1|1|1|1|1|1|1|v", "BBO|1|1|1|fired|1|1|1|1|1|1|1|1|1|1|1|51|1|1|v", "BBO|1|1|1|fired|nan|1|1|1|1|1|1|1|1|1|1|1|1|1|v" };
        for (const char* b : bad) CHECK(!bba::parseBoLine(b, Q), std::string("[logic] garbage refused: ") + b);
        const std::string det = bba::boDetail(R, 0.25, "8:33 AM");
        CHECK(det.find("Q: did buyers break out with real size?") != std::string::npos && det.find("pivot 105.00 (higher pivot 2 back)") != std::string::npos && det.find("range 2.0x prior,volume 1.5x prior") != std::string::npos
              && det.find("print 60 lots bought at 106.00") != std::string::npos && det.find("biggest in 10") != std::string::npos && det.find("never repainted") != std::string::npos && det.find("don't fade it") != std::string::npos && det.find("Not an entry") != std::string::npos, "[logic] the details line in his order");
    }
}

// ---- [nq] the mockup's day ---------------------------------------------------------------------------------------------------
struct Fx { std::string end; double o, h, l, c; long long v, bl; double bp; long long sl; double sp; };
static std::vector<Fx> fixture() {
    std::vector<Fx> v; std::ifstream f("nq_2026-10-21_3m.txt"); std::string ln; std::getline(f, ln);
    while (std::getline(f, ln)) { Fx x; char e[32];
        if (std::sscanf(ln.c_str(), "%19[^|]|%lf|%lf|%lf|%lf|%lld|%lld|%lf|%lld|%lf", e, &x.o, &x.h, &x.l, &x.c, &x.v, &x.bl, &x.bp, &x.sl, &x.sp) == 10) { x.end = e; v.push_back(x); } }
    return v;
}
struct Want { const char* t; int dir; long long lots; int n; double px; };
// bo.py (the approved mockup's rule) on the same candles, 8:33 AM - 3:00 PM CT; n 0 = the letter only
static const Want WANT[] = {
    { "08:33", 1, 80, 50, 29891.75 }, { "08:42", -1, 47, 0, 29854.00 }, { "08:45", -1, 72, 1, 29781.00 }, { "09:03", 1, 56, 2, 29890.00 },
    { "09:51", 1, 47, 0, 29919.25 }, { "10:03", 1, 55, 4, 29934.00 }, { "10:12", 1, 49, 2, 29960.00 }, { "10:18", 1, 45, 0, 29972.00 },
    { "10:48", 1, 124, 2, 30050.00 }, { "11:09", -1, 50, 1, 30016.25 }, { "11:12", -1, 49, 0, 30011.00 }, { "11:21", -1, 22, 0, 30015.25 },
    { "11:27", -1, 50, 2, 30001.50 }, { "11:42", 1, 110, 3, 30069.00 }, { "12:24", 1, 31, 0, 30113.00 }, { "13:18", 1, 84, 6, 30080.00 },
    { "13:33", 1, 52, 0, 30086.75 }, { "13:48", -1, 52, 1, 30054.50 }, { "13:51", -1, 68, 9, 30017.25 }, { "14:51", 1, 54, 2, 30049.00 } };
static const int NWANT = (int)(sizeof(WANT) / sizeof(WANT[0]));
static void nqLogic(const std::vector<Fx>& F) {
    const int n = (int)F.size(); std::vector<double> H, L; for (const Fx& x : F) { H.push_back(x.h); L.push_back(x.l); }
    std::vector<signed char> u, d; std::vector<double> pu, pd; bba::breakFlagsPx(H, L, n, u, d, &pu, &pd);
    std::vector<bba::BoIn> in((size_t)n);
    for (int k = 0; k < n; ++k) { bba::BoIn& b = in[(size_t)k]; b.known = true; b.hk = std::llround(F[(size_t)k].h / 0.25); b.lk = std::llround(F[(size_t)k].l / 0.25); b.vol = F[(size_t)k].v;
        b.big[1] = F[(size_t)k].bl; b.bigKey[1] = std::llround(F[(size_t)k].bp / 0.25); b.big[0] = F[(size_t)k].sl; b.bigKey[0] = std::llround(F[(size_t)k].sp / 0.25); }
    std::vector<std::string> got;
    int match = 0;
    for (int i = 1; i < n; ++i) {
        const std::string& e = F[(size_t)i].end; if (e.compare(0, 10, "2026-10-21") != 0) continue;
        const std::string hm = e.substr(11, 5); if (hm < "08:33" || hm > "15:00") continue;
        bool w = false; const bba::BoDec D = bba::breakoutAt(in, u, d, pu, pd, i, w);
        if (w) { CHECK(false, "[nq] no wait with every candle known"); continue; }
        if (!D.fired) continue;
        got.push_back(hm);
        const Want* W = nullptr; for (int k = 0; k < NWANT; ++k) if (hm == WANT[k].t) W = &WANT[k];
        CHECK(W != nullptr, "[nq] fired at " + hm + " but the mockup has no event there");
        if (!W) continue;
        const bool ok = D.dir == W->dir && D.lots == W->lots && D.n == W->n && D.print == (W->n >= 1) && std::fabs(D.key * 0.25 - W->px) < 1e-9;
        CHECK(ok, "[nq] " + hm + ": " + (D.dir > 0 ? "BO " : "BD ") + std::to_string(D.lots) + " at " + std::to_string(D.key * 0.25) + " biggest in " + std::to_string(D.n));
        if (ok) ++match;
    }
    CHECK((int)got.size() == NWANT && match == NWANT, "[nq] the logic reproduces all " + std::to_string(NWANT) + " mockup events (got " + std::to_string(got.size()) + ", matched " + std::to_string(match) + ")");
    std::cout << "  NQ 2026-10-21 8:33 AM - 3:00 PM: " << got.size() << " breakouts / breakdowns fired, " << match << " match bo.py (side, lots, biggest in N, price)\n";
}

// ---- [host] ----------------------------------------------------------------------------------------------------------------------
static unsigned long stampOf(const std::string& e) {   // "2026-10-21 08:33" as Central wall time (the mock's getLocaltime = gmtime)
    struct tm t; std::memset(&t, 0, sizeof(t)); std::sscanf(e.c_str(), "%d-%d-%d %d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min);
    t.tm_year -= 1900; t.tm_mon -= 1; return (unsigned long)timegm(&t);
}
static std::vector<mock::Bar> nqBars(const std::vector<Fx>& F, size_t upto) {
    std::vector<mock::Bar> v;
    for (size_t k = 0; k < F.size() && k < upto; ++k) { const Fx& x = F[k]; mock::Bar b; b.time = stampOf(x.end); b.open = (float)x.o; b.high = (float)x.h; b.low = (float)x.l; b.close = (float)x.c; b.volume = (unsigned long)x.v;
        if (x.bl > 0) b.rows.push_back(mock::VRow{ (float)x.bp, (long)x.bl, 0 });
        if (x.sl > 0) { bool same = false; for (auto& r : b.rows) if (r.price == (float)x.sp) { r.bid = (long)x.sl; same = true; } if (!same) b.rows.push_back(mock::VRow{ (float)x.sp, 0, (long)x.sl }); }
        if (b.rows.empty()) b.rows.push_back(mock::VRow{ (float)x.c, 1, 1 });
        v.push_back(b); }
    return v;
}
static int idxOf(const std::vector<Fx>& F, const std::string& end) { for (size_t k = 0; k < F.size(); ++k) if (F[k].end == end) return (int)k; return -1; }
struct Chart {
    mock::Host H; BigBidAsk* x = nullptr;
    explicit Chart(const std::vector<mock::Bar>& b) { H.bars = b; H.now = b.back().time; H.symbol = "NQZ6"; H.root = "NQ"; H.chart = "NQ 3m"; H.tick = 0.25f;
        H.paneBottom = 1000; H.priceBase = 29740; H.pxPerTick = 1; H.barW = 40;
        mock::current = &H; x = static_cast<BigBidAsk*>(CreateExtension()); x->setup(); }
    ~Chart() { mock::current = &H; x->destroy(); delete x; mock::current = nullptr; }
    void pump(int n, bool draw) {
        for (int k = 0; k < n; ++k) { mock::current = &H; BBState* s = x->testState();
            if (s) { s->builtMs = -1; s->boardMs = -1; s->sdCheckMs = -1; s->statsMs = -1; s->lastStatusMs = -1; s->logMs = -1; s->boMs = -1; s->boFlushMs = -1; }
            if (draw) { H.draws.clear(); x->draw(); } else x->calc(0); }
    }
    void view(int v0, int v1) { H.visStart = v0; H.visEnd = v1; H.barOffset = v0;
        float lo = 1e9f; for (int k = v0; k <= v1 && k < (int)H.bars.size(); ++k) lo = std::min(lo, H.bars[(size_t)k].low); H.priceBase = lo - 10;   // the view's prices fit the pane
        pump(2, true); }
    BBState& st() { return *x->testState(); }
};
static std::string HOME;
static void home(const std::string& name, const ss::Values* set = nullptr) {
    const std::string root = "/tmp/bba121-run", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str()) != 0) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1); HOME = h;
    if (set) { std::ofstream f((h + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings2-NQ.txt").c_str()); f << ss::format(bbaset::spec(), *set); }
}
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static std::string recPath(Chart& c) { return HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk/NQ-NQZ6-180-breakouts-" + std::to_string(c.st().session) + ".csv"; }
static int barX(const mock::Host& H, int i);
// the text t in colour col, on bar i when i >= 0 (its centre within 2 px of the bar's centre)
static const mock::Draw* textAt(Chart& c, const std::string& t, COLOR col, int i = -1) {
    for (const auto& d : c.H.draws) if (d.type == "ctext" && d.text == t && d.color == col && (i < 0 || std::abs((d.l + d.r) / 2 - barX(c.H, i)) <= 2)) return &d;
    return nullptr; }
static int countText(Chart& c, const std::string& t) { int n = 0; for (const auto& d : c.H.draws) if (d.type == "ctext" && d.text == t && d.color != 0) ++n; return n; }
static int pxY(const mock::Host& H, double p) { return (int)(H.paneBottom - 100 - (p - H.priceBase) / H.tick * H.pxPerTick); }
static int barX(const mock::Host& H, int i) { return 20 + (i - H.barOffset) * H.barW + H.barW / 4; }

static void host(const std::vector<Fx>& F) {
    const int i833 = idxOf(F, "2026-10-21 08:33"), i845 = idxOf(F, "2026-10-21 08:45"), i842 = idxOf(F, "2026-10-21 08:42"), i1048 = idxOf(F, "2026-10-21 10:48");
    CHECK(i833 > 0 && i845 > 0 && i1048 > 0, "[host] fixture candles found");
    ss::Values V0 = ss::defaults(bbaset::spec(), ss::M_NQ); V0.v[bbaset::K_SKIP_FIRST] = 0; V0.v[bbaset::K_SKIP_LAST] = 0;
    {   home("skip0", &V0);
        Chart c(nqBars(F, F.size())); c.pump(14, false);
        BBState& S = c.st();
        int fired = 0, match = 0;   // (1.23.0) the 20 1.21 breakout candles are all decided (keyed by the breakout candle t0); 10 fire under green / red + same wave
        for (int k = 0; k < NWANT; ++k) {
            const auto it = S.boDec.find((long long)stampOf(std::string("2026-10-21 ") + WANT[k].t));
            if (it == S.boDec.end()) continue;
            ++match; if (it->second.fired) ++fired;
        }
        CHECK(match == NWANT && fired == 10, "[host] the 20 1.21 candidates are all decided (skip 0); 10 fire under the 1.23 rules: decided " + std::to_string(match) + ", fired " + std::to_string(fired));
        int extra = 0; for (const auto& kv : S.boDec) { const std::string e = [&] { time_t v = (time_t)kv.first; struct tm m; gmtime_r(&v, &m); char b[32]; std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M", &m); return std::string(b); }();
            if (kv.second.fired && e >= "2026-10-21 08:33" && e <= "2026-10-21 15:00") { bool w = false; for (int k = 0; k < NWANT; ++k) if (e.substr(11) == WANT[k].t) w = true; if (!w) ++extra; } }
        CHECK(extra == 0, "[host] no breakout the mockup does not have (8:33 AM - 3:00 PM)");
        const std::string rec = slurp(recPath(c));
        CHECK(rec.compare(0, std::strlen(bba::BO_HEADER), bba::BO_HEADER) == 0, "[host] the record starts with the BBO header");
        CHECK(rec.find("BBO|" + std::to_string(stampOf("2026-10-21 08:33")) + "|" + std::to_string(stampOf("2026-10-21 08:33")) + "|1|0|wick_wave_ended|29903|") != std::string::npos, "[host] record: 8:33 BO through 29,903 on a RED candle, the up-wave ended there: logged wick_wave_ended (1.23.0)");
        CHECK(rec.find("BBO|" + std::to_string(stampOf("2026-10-21 08:45")) + "|" + std::to_string(stampOf("2026-10-21 08:42")) + "|-1|1|fired|") != std::string::npos, "[host] record: the 8:42 wick break down confirmed by the 8:45 red close (mark on 8:45, 1.23.0)");
        int notFired = 0; { std::istringstream in(rec); std::string ln; while (std::getline(in, ln)) { bba::BoDec D; if (bba::parseBoLine(ln, D) && !D.fired) ++notFired; } }
        CHECK(notFired > 0, "[host] decisions that did not fire are recorded too, with the reason (" + std::to_string(notFired) + ")");
        // the draw: 8:30 - 9:06 in view
        c.view(i833 - 2, i833 + 12);
        if (std::getenv("BBA_DBG")) for (const auto& d : c.H.draws) if (d.color != 0) std::cout << d.type << " '" << d.text << "' " << d.l << "," << d.t << "," << d.r << "," << d.b << " col " << std::hex << d.color << std::dec << "\n";
        if (std::getenv("BBA_DBG")) std::cout << "x833 " << barX(c.H, i833) << " hi " << pxY(c.H, F[(size_t)i833].h) << " lo " << pxY(c.H, F[(size_t)i833].l) << "\n";
        // (1.23.0) 8:33 was a BO on a RED candle whose wave ended: no BO letter and no green breakout box there (the 1.21 mark is gone)
        CHECK(!textAt(c, "BO", 0x0022C55E, i833), "[host] 8:33: no BO (red candle, failed wick break - logged only)");
        CHECK(!textAt(c, "FB", 0x0022C55E) && !textAt(c, "FB", 0x00EF4444), "[host] no FB mark is ever drawn");
        const mock::Draw* p72 = textAt(c, "72", 0x00FFFFFF, i845);
        CHECK(p72 && std::abs((p72->l + p72->r) / 2 - barX(c.H, i845)) <= 2 && std::abs((p72->t + p72->b) / 2 - pxY(c.H, 29781.0)) <= 8, "[host] 8:45: the 72 ON its candle at 29,781");
        bool ringR = false; for (const auto& d : c.H.draws) if (d.type == "ring" && d.color == 0x00EF4444 && p72 && d.l <= p72->l && d.r >= p72->r) ringR = true;
        CHECK(ringR, "[host] 8:45: the 72's box outlined red");
        int bdBelow = 0; for (const auto& d : c.H.draws) if (d.type == "ctext" && d.text == "BD" && d.color == 0x00EF4444) {
            const int x = (d.l + d.r) / 2; if (x == barX(c.H, i845) && (d.t + d.b) / 2 > pxY(c.H, F[(size_t)i845].l)) ++bdBelow; if (x == barX(c.H, i842) && (d.t + d.b) / 2 > pxY(c.H, F[(size_t)i842].l)) ++bdBelow; }
        CHECK(bdBelow == 1 && !textAt(c, "BD", 0x00EF4444, i842), "[host] 8:45 (the confirming red close): BD red BELOW the candle; nothing on the 8:42 wick candle");
        CHECK(countText(c, "47") == 0, "[host] 8:42's 47 is not bigger than the prior candle's biggest sell (59): no number");
        int lines = 0; for (const auto& d : c.H.draws) if (d.type != "ctext" && d.type != "ring" && d.type != "rect" && d.type != "text") ++lines;
        CHECK(lines == 0, "[host] nothing but numbers, boxes and letters: no pivot line");
        // nothing drawn overlaps: the letters vs every box / number
        std::vector<const mock::Draw*> L, B; for (const auto& d : c.H.draws) if (d.color != 0) { if (d.type == "ctext" && (d.text == "BO" || d.text == "BD")) L.push_back(&d); else if (d.type == "ring") B.push_back(&d); }
        int ov = 0; for (auto* a : L) for (auto* b : B) if (!(a->r < b->l || a->l > b->r || a->b + 2 < b->t || a->t + 2 > b->b)) ++ov;
        CHECK(ov == 0 && L.size() >= 2, "[host] the letters never cover a print box (" + std::to_string(L.size()) + " letters)");
        c.view(i1048 - 6, i1048 + 6);
        CHECK(textAt(c, "124", 0x00FFFFFF, i1048) && textAt(c, "BO", 0x0022C55E, i1048), "[host] 10:48 BO 124 drawn on its candle");
        const std::string stt = slurp(HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.status-NQ-NQZ6-180.txt");
        CHECK(stt.find("VERSION,1.23.0\n") == 0 && stt.find("\nBREAKOUTS,on,today BO ") != std::string::npos && stt.find("BO_EVENT,9:03 AM,BO,fired,Q: did buyers break out with real size?") != std::string::npos
              && stt.find("print 56 lots bought at 29,890.00") != std::string::npos && stt.find("BO_EVENT,8:33 AM,BO,not fired: wick_wave_ended") != std::string::npos && stt.find("\nBREAKOUT_RULES,1.23.0") != std::string::npos, "[host] status: version, BREAKOUTS counts, the 9:03 details, the 8:33 failed wick break, the rules line");
        // no repaint: recomputes + a later bar changing nothing already decided
        std::map<long long, std::string> before; for (const auto& kv : S.boDec) if (kv.second.dir) before[kv.first] = bba::boLine(kv.second, 0, "x");
        c.H.bars[(size_t)i833].high += 50; c.H.bars[(size_t)i833 - 1].low -= 50;   // the chart's bars changing under a decided candle
        c.pump(6, false);
        int changed = 0; for (const auto& kv : before) { const auto it = c.st().boDec.find(kv.first); if (it == c.st().boDec.end() || bba::boLine(it->second, 0, "x") != kv.second) ++changed; }
        CHECK(changed == 0, "[host] decided candles never change across recomputes (no repaint)");
    }
    {   // the forming candle is never decided; new bars never re-decide
        home("grow", &V0);
        Chart c(nqBars(F, (size_t)i833 + 1)); c.pump(10, false);
        const long long t833 = (long long)stampOf("2026-10-21 08:33");
        CHECK(!c.st().boDec.count(t833), "[host] 8:33 still forming: not decided");
        for (size_t k = (size_t)i833 + 1; k < (size_t)i1048 + 3; ++k) { c.H.bars.push_back(nqBars(F, k + 1).back()); c.H.now = c.H.bars.back().time; c.pump(2, false); }
        const auto it = c.st().boDec.find(t833);
        const auto i2 = c.st().boDec.find((long long)stampOf("2026-10-21 10:48"));
        CHECK(it != c.st().boDec.end() && !it->second.fired && it->second.reason == bba::BO_R_WAVE_ENDED && i2 != c.st().boDec.end() && i2->second.fired && i2->second.lots == 124 && i2->second.n == 2, "[host] decided once closed: 8:33 failed wick break, 10:48 BO 124 (biggest in 2), bar by bar");
    }
    {   // a restart redraws from the record (the record wins: tampered 80 -> 81 shows 81)
        home("restart", &V0);
        { Chart c(nqBars(F, F.size())); c.pump(14, false); }
        std::string path; { Chart c(nqBars(F, F.size())); c.pump(1, false); path = recPath(c); }
        std::string rec = slurp(path); const std::string key = "BBO|" + std::to_string(stampOf("2026-10-21 10:48")) + "|" + std::to_string(stampOf("2026-10-21 10:48")) + "|1|1|fired|";
        const size_t a = rec.find(key); CHECK(a != std::string::npos, "[host] restart: the 10:48 line is in the record");
        if (a != std::string::npos) { const size_t e = rec.find('\n', a); std::string ln = rec.substr(a, e - a); bba::BoDec D; bba::parseBoLine(ln, D); D.lots = 125; rec.replace(a, e - a, bba::boLine(D, 1, "1.23.0")); std::ofstream(path.c_str(), std::ios::binary | std::ios::trunc) << rec; }
        Chart c(nqBars(F, F.size())); c.pump(8, false); c.view(i1048 - 6, i1048 + 6);
        CHECK(textAt(c, "125", 0x00FFFFFF, i1048) && !textAt(c, "124", 0x00FFFFFF, i1048), "[host] restart: the decision is reloaded from the record, not re-judged");
        const std::string after = slurp(path); size_t n833 = 0, p = 0; while ((p = after.find(key, p)) != std::string::npos) { ++n833; ++p; }
        CHECK(n833 == 1, "[host] restart: a candle is recorded once");
    }
    {   // the default skip first / last 15 minutes: 8:33 and 8:45 are inside the first 15 minutes of RTH
        home("defaults");
        Chart c(nqBars(F, F.size())); c.pump(14, false);
        const auto a = c.st().boDec.find((long long)stampOf("2026-10-21 08:33")), b = c.st().boDec.find((long long)stampOf("2026-10-21 08:45")), d = c.st().boDec.find((long long)stampOf("2026-10-21 10:48"));
        const auto b2 = c.st().boDec.find((long long)stampOf("2026-10-21 08:42"));
        CHECK(a != c.st().boDec.end() && !a->second.fired && b2 != c.st().boDec.end() && b2->second.reason == bba::BO_R_TIME && b2->second.t == (long long)stampOf("2026-10-21 08:45") && b != c.st().boDec.end() && !b->second.fired, "[host] skip first 15 (default): the 8:45 confirmation is inside the skipped minutes: time_filter, not fired");
        CHECK(d != c.st().boDec.end() && d->second.fired, "[host] 10:48 fires with the default skip");
        c.view(i833 - 2, i833 + 12);
        CHECK(!textAt(c, "BO", 0x0022C55E, i833) && !textAt(c, "BD", 0x00EF4444, i845) && !textAt(c, "72", 0x00FFFFFF, i845) && !textAt(c, "80", 0x00FFFFFF, i833), "[host] nothing drawn inside the skipped minutes (8:33 / 8:45)");
    }
    {   // (1.22.0, Rassul 06:43 / 06:49) the circle only on a SHARED A candle of its side that closed in the defenders' colour; skip first 15 stays the default
        for (int m = 0; m < 7; ++m) CHECK(ss::defaults(bbaset::spec(), m).v[bbaset::K_SKIP_FIRST] == 15 && ss::defaults(bbaset::spec(), m).v[bbaset::K_SKIP_LAST] == 15, "[1.22.0] skip first / last 15 min is the default in every market (07:08 \"first 15 min needs to be skipped\")");
        home("sab", &V0); mkdir((HOME + "/InvestorRT/rtx/lsFlexLevels/SharedAbsorb").c_str(), 0700);
        Chart c(nqBars(F, F.size())); c.pump(14, false); c.view(i833 - 2, i833 + 12);
        CHECK(c.st().circles == 0, "[1.22.0] no shared A record: no circle (the 1.20.0 order-book circle is retired)");
        const mock::Bar& b = c.H.bars[(size_t)i833]; const bool green = b.close > b.open;
        sab::Rec r; r.mkt = "NQ"; r.spb = 180; r.key = 1; r.raw = (long long)b.time; r.side = green ? sab::LOW : sab::HIGH; r.effort = 2.5; r.E = 2.0; r.att = 500; r.avg = 200;
        r.close = green ? 'G' : 'R'; r.extPx = green ? b.low : b.high; r.closePx = b.close; r.by = "TEST"; r.loc = "ext5";
        const mock::Bar& b2 = c.H.bars[(size_t)i845]; sab::Rec f2 = r; f2.raw = (long long)b2.time; f2.key = 2; f2.side = b2.close > b2.open ? sab::HIGH : sab::LOW;   // the attackers' colour on 8:45
        f2.close = f2.side == sab::LOW ? 'G' : 'R'; std::string forged = sab::line(f2); { const size_t g = forged.find(f2.close == 'G' ? "|G|" : "|R|"); forged.replace(g, 3, f2.close == 'G' ? "|R|" : "|G|"); }   // a line claiming the attackers' close
        { std::ofstream o((HOME + "/InvestorRT/rtx/lsFlexLevels/SharedAbsorb/SAB-NQ-180.csv").c_str(), std::ios::binary); o << sab::line(r) << "\n" << forged << "\n"; }
        c.st().sab.sReadMs = -1; c.view(i833 - 2, i833 + 12);
        CHECK(c.st().circles == 1, "[1.22.0] the shared A on 8:33: ONE circle, on the print of the attackers' side (" + std::to_string(c.st().circles) + ")");
        CHECK(c.st().sab.st.byKey.size() == 1 && c.st().sab.st.bad == 1, "[1.22.0] a record line on an attacker-colour candle is rejected: no circle at 8:45");
        V0.v[bbaset::K_ABS_CIRCLE] = 0; home("sab_off", &V0); mkdir((HOME + "/InvestorRT/rtx/lsFlexLevels/SharedAbsorb").c_str(), 0700);
        { std::ofstream o((HOME + "/InvestorRT/rtx/lsFlexLevels/SharedAbsorb/SAB-NQ-180.csv").c_str(), std::ios::binary); o << sab::line(r) << "\n"; }
        Chart c2(nqBars(F, F.size())); c2.pump(14, false); c2.view(i833 - 2, i833 + 12);
        CHECK(c2.st().circles == 0, "[1.22.0] 'Circle prints at a shared A' unchecked: no circle");
        V0.v[bbaset::K_ABS_CIRCLE] = 1;
    }
    {   // the setting off: no letters / breakout prints; still decided and recorded
        ss::Values V = V0; V.v[bbaset::K_BREAKOUT] = 0; home("off", &V);
        Chart c(nqBars(F, F.size())); c.pump(14, false); c.view(i833 - 2, i833 + 12);
        CHECK(!textAt(c, "BO", 0x0022C55E) && !textAt(c, "BD", 0x00EF4444) && !textAt(c, "72", 0x00FFFFFF) && !textAt(c, "80", 0x00FFFFFF), "[host] Breakout / breakdown unchecked: no BO / BD marks");
        int f833 = 0; for (const auto& d : c.H.draws) if (d.type == "ctext" && d.text == "50" && d.color != 0 && std::abs((d.l + d.r) / 2 - barX(c.H, i833)) <= 2) ++f833;
        CHECK(f833 == 2, "[host] ... the 8:33 Big Bid/Ask prints show as before (bid and ask, each biggest in 50)");
        CHECK(slurp(recPath(c)).find("|fired|") != std::string::npos, "[host] ... decisions still recorded for the nightly");
    }
    {   // his colours
        ss::Values V = V0; V.v[bbaset::K_COL_BO] = 0x00FFFF; V.v[bbaset::K_COL_BD] = 0xFF00FF; home("colours", &V);
        Chart c(nqBars(F, F.size())); c.pump(14, false); c.view(i833 - 2, i833 + 12);
        CHECK(textAt(c, "BO", 0x0000FFFF, idxOf(F, "2026-10-21 09:03")) && textAt(c, "BD", 0x00FF00FF, i845), "[host] BO / BD colours from the settings");
    }
    {   // the HOD/LOD table over the 8:33 BO letter: not drawn there (kept out); nothing of this plugin inside the table
        home("keepout", &V0);
        Chart c(nqBars(F, F.size())); c.pump(14, false);
        c.view(i833 - 2, i833 + 12);
        const mock::Draw* bo = textAt(c, "BO", 0x0022C55E, idxOf(F, "2026-10-21 09:03")); CHECK(bo != nullptr, "[host] keep-out: BO (9:03) there before the table");
        if (bo) {
            const bdk::Box tb = bdk::mk(bo->l - 30, bo->t - 10, bo->r + 30, bo->b + 6);
            for (int k = 0; k < 3; ++k) { bdk::publishHodLod(c.H.symbol, 180, c.H.chart, steadyMs() + k, tb); c.pump(1, true); }
            int inside = 0; for (const auto& d : c.H.draws) if (d.color != 0 && !(d.r < tb.l || d.l > tb.r || d.b < tb.t || d.t > tb.b)) ++inside;
            CHECK(inside == 0 && c.st().boKept >= 1, "[host] keep-out: the letter is not drawn under the HOD/LOD table (kept " + std::to_string(c.st().boKept) + ")");
            bdk::clearHodLod(c.H.symbol, 180, c.H.chart);
        }
    }
    { const std::string src = slurp("../BigBidAsk.cpp");
      const size_t a = src.find("(1.21.0) BREAKOUT / BREAKDOWN (decided in boPass"), b = src.find("the hover: only while the cursor");
      CHECK(a != std::string::npos && b > a && src.substr(a, b - a).find("P_DASH") == std::string::npos && src.substr(a, b - a).find("drawLineTo") == std::string::npos, "[host] no dashed / pivot line in the breakout drawing");
      CHECK(std::string(BBA_VERSION) == "1.23.0" && std::string(bba::VERSION) == "1.23.0" && std::string(bbaset::PLUGIN_VERSION) == "1.23.0" && src.find("p->setVersion(\"1.23.0\")") != std::string::npos, "[ver] 1.21.0 everywhere");
      CHECK(bbaset::spec().rows[bbaset::K_BREAKOUT].def == 1 && bbaset::spec().paramVersion == 10, "[set] Breakout / breakdown on by default; parameter version 10"); }
}

int main() {
    setenv("TZ", "UTC", 1); tzset();
    logic();
    const std::vector<Fx> F = fixture();
    CHECK(F.size() == 460, "[nq] fixture: 460 candles");
    nqLogic(F);
    host(F);
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
