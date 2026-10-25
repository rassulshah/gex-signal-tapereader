// test_bo128.cpp -- lsBigBidAsk 1.28.0 BO wave confirmation (Rassul approved 2026-10-24 21:38 CT), SDK-free (BigBidAskLogic.h).
//   [real]  ES 2026-10-23 3-min: the red bar ending 11:54 (pivot low 7676.25 of the bar ending 11:27) is the BD bar and the red bar
//           ending 12:00 (lower low 7674.75, lower close 7676.00, below the pivot, same down-wave) confirms it.
//   [unit]  confirm; wave flips before confirm; counter-trend without a 30-bar extreme fails; with it passes; wick BO bar; a candle
//           that is higher but does not close beyond the pivot waits; wrong colour; restart = same decision; records (old 24-field
//           line still parses as rule 1, rule-2 round trip, a fired rule-2 line with t == t0 refused).
// Run: see README.txt
#include "BigBidAskLogic.h"
#include "VwapTrendLogic.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("PASS %s\n", m); } else { ++fails; std::printf("FAIL %s\n", m); } } while (0)

struct Bar { long days; int endSec; double o, h, l, c, v; std::string clk; };
static std::vector<Bar> load(const char* f) {
    std::ifstream in(f); std::string l; std::vector<Bar> v;
    while (std::getline(in, l)) { if (l.empty() || l[0] == '#') continue; int y, mo, d, h, mi; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &o, &hi, &lo, &c, &vo) != 10) continue;
        Bar b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60; b.o = o; b.h = hi; b.l = lo; b.c = c; b.v = vo; b.clk = l.substr(11, 5); v.push_back(b); }
    return v;
}

// the frozen facts of lsBigBidAsk for that session (lsFlexLevels\BigBidAsk\ES-ESZ6-180-20749.csv: IRT time = seconds since 1904 at the
// bar END; bid_lots / ask_lots = the candle's biggest cell at the bid / ask, keys in ticks)
struct Fact { long days; int endSec; long long bidK, bid, askK, ask; };
static std::vector<Fact> loadFacts(const char* f) {
    std::ifstream in(f); std::string l; std::vector<Fact> v; std::getline(in, l);
    while (std::getline(in, l)) { long long t, va, o, h, lo, c, bk, bl, bq, ak, al, aq;
        if (std::sscanf(l.c_str(), "%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld", &t, &va, &o, &h, &lo, &c, &bk, &bl, &bq, &ak, &al, &aq) != 12 || va != 1) continue;
        const long long u = t - 2082844800LL; Fact F; F.days = (long)(u / 86400); F.endSec = (int)(u % 86400); F.bidK = bk; F.bid = bl; F.askK = ak; F.ask = al; v.push_back(F); }
    return v;
}
static void realSession(const char* file, const char* facts) {
    const double tick = 0.25; const int per = 180, openMin = 510, closeMin = 900;
    const std::vector<Bar> B = load(file); const int n = (int)B.size();
    CHECK(n > 250, "[real] ES 2026-10-23 bars loaded");
    if (n < 250) return;
    std::vector<double> op(n), hi(n), lo(n), cl(n); std::vector<char> rth(n, 0); std::vector<long long> day(n, 0), sess(n, 0);
    std::vector<svl::Bar> sb(n); std::vector<char> skip(n, 0);
    for (int i = 0; i < n; ++i) { op[i] = B[i].o; hi[i] = B[i].h; lo[i] = B[i].l; cl[i] = B[i].c;
        const svl::Cls c = svl::classify(B[i].days, B[i].endSec, per, openMin, closeMin);
        rth[i] = c.kind == svl::RTH; day[i] = c.sess; sess[i] = rth[i] ? c.sess : 0;
        sb[i] = svl::Bar{ B[i].days, B[i].endSec, (float)B[i].h, (float)B[i].l, (float)B[i].c, B[i].v };
        const int st = c.startSec; skip[i] = c.kind == svl::NONE || (st >= openMin * 60 && st < openMin * 60 + 900) || (B[i].endSec > closeMin * 60 - 900 && B[i].endSec <= closeMin * 60); }
    std::vector<signed char> up, dn, zd; std::vector<double> upPx, dnPx;
    bba::swingBreaksPx(op, hi, lo, cl, rth, day, n, tick, up, dn, upPx, dnPx);
    bba::zigDir(hi, lo, n, zd);
    std::vector<bba::BoIn> in(n); std::vector<bba::BoBar> bars(n); std::vector<signed char> trend(n, 0);
    const std::vector<Fact> FX = loadFacts(facts); int nf = 0;
    for (int i = 0; i < n; ++i) {
        bba::BoIn& x = in[i]; x.known = true; x.hk = std::llround(hi[i] / tick); x.lk = std::llround(lo[i] / tick); x.vol = (long long)B[i].v;
        for (const Fact& F : FX) if (F.days == B[i].days && F.endSec == B[i].endSec) { x.big[0] = F.bid; x.bigKey[0] = F.bidK; x.big[1] = F.ask; x.bigKey[1] = F.askK; ++nf; break; }
        bba::BoBar& b = bars[i]; b.ok = true; b.o = std::llround(op[i] / tick); b.c = std::llround(cl[i] / tick); b.h = x.hk; b.l = x.lk;
        trend[i] = (signed char)vtl::level(vtl::fromChart(sb, i, per, openMin, closeMin));   // = the host's trendAt(S, i)
    }
    const bba::BoRun R = bba::boWaves(in, bars, zd, up, dn, upPx, dnPx, skip, trend, sess, tick, 1, n, 0);
    int iBD = -1, iC = -1, i27 = -1;
    for (int i = 0; i < n; ++i) { if (B[i].clk == "11:54") iBD = i; if (B[i].clk == "12:00") iC = i; if (B[i].clk == "11:27") i27 = i; }
    std::printf("  ES 2026-10-23 decisions (BO bar -> decision candle):\n");
    const bba::BoDec* hit = nullptr;
    for (const bba::BoDec& D : R.ev) {
        if (D.i0 < 0) continue;
        std::printf("    %s %s pivot %.2f -> %s %s trend %+d ext30 %d wick %d\n", B[D.i0].clk.c_str(), D.dir > 0 ? "BO" : "BD", D.piv, B[D.im].clk.c_str(), bba::boReasonName(D.reason), D.trend, D.ext30, D.wick);
        if (D.i0 == iBD && D.dir < 0) hit = &D;
    }
    CHECK(nf > 250, "[real] the frozen facts (biggest cell per side) matched to the bars");
    CHECK(i27 >= 0 && std::llround(lo[i27] / tick) == std::llround(7676.25 / tick), "[real] the bar ending 11:27 has the pivot low 7676.25");
    CHECK(hit != nullptr, "[real] the red bar ending 11:54 is a BD bar");
    if (hit) {
        CHECK(std::llround(hit->piv / tick) == std::llround(7676.25 / tick), "[real] BD pivot = 7676.25");
        CHECK(hit->fired && hit->reason == bba::BO_R_FIRED, "[real] BD fired (confirmed)");
        CHECK(hit->im == iC, "[real] decided on the red bar ending 12:00 (lower low 7674.75, lower close 7676.00, below the pivot)");
        CHECK(bba::boMarkT(*hit) == hit->t0 || hit->rule == 2, "[real] the letter goes on the BO bar (rule 2)");
        CHECK(zd[iBD] == -1 && zd[iC] == -1, "[real] same down-wave");
        CHECK(hit->lots == 479 && hit->key == 30719 && hit->prior == 704 && !hit->print && hit->n == 0, "[real] the BD bar's print = its own biggest sell cell (479 at 7679.75), not bigger than the prior candle's 704: letter only");
        std::printf("  BD trend at 11:54 = %+d (%s), 30-bar low by 12:00 = %d\n", hit->trend, bba::boWithTrend(-1, hit->trend) ? "with the trend" : "counter-trend", hit->ext30);
    }
    {   const bba::BoDec* b942 = nullptr; for (const bba::BoDec& D : R.ev) if (D.i0 >= 0 && B[D.i0].clk == "09:42" && D.dir > 0) b942 = &D;
        CHECK(b942 && b942->fired && B[b942->im].clk == "09:45" && b942->print && b942->lots == 1195 && b942->n == 50, "[real] 09:42 BO confirmed at 09:45, its box = biggest in 50 (1,195 lots in the status)"); }
    // the green bar ending 11:57 (lower low, but green and closing above the pivot) does not confirm: nothing is decided at 11:57 for it
    bool early = false; for (const bba::BoDec& D : R.ev) if (D.i0 == iBD && D.im != iC) early = true;
    CHECK(!early, "[real] the green 11:57 bar does not decide the BD");
    // prefix through 11:57 (12:00 not closed yet): the BD is still waiting, nothing fired -> it appears only at the confirming close
    const bba::BoRun P = bba::boWaves(in, bars, zd, up, dn, upPx, dnPx, skip, trend, sess, tick, 1, iC, 0);
    bool firedEarly = false; for (const bba::BoDec& D : P.ev) if (D.i0 == iBD) firedEarly = true;
    CHECK(!firedEarly && P.pendOn[0] && P.pend[0].i0 == iBD, "[real] before 12:00 closes the BD bar is waiting (not drawn yet)");
}

// ---- synthetic: tick 1, 40 flat bars (h 105 / l 100), bar 25 a 120 spike, bar 26 a lower low (the walk turns down), bar 27 turns up;
// BO bar = 40 through pivot 106 (up[40] = 1). The 30-bar window of bars 27 - 54 holds the 120 spike.
struct Syn { std::vector<bba::BoIn> in; std::vector<bba::BoBar> bar; std::vector<signed char> zd, up, dn, trend; std::vector<double> upPx, dnPx; std::vector<char> skip; std::vector<long long> sess; };
static void put(Syn& S, int i, long long o, long long h, long long l, long long c, long long vol, long long big) {
    S.bar[i].ok = true; S.bar[i].o = o; S.bar[i].h = h; S.bar[i].l = l; S.bar[i].c = c;
    S.in[i].known = true; S.in[i].hk = h; S.in[i].lk = l; S.in[i].vol = vol; S.in[i].big[1] = big; S.in[i].bigKey[1] = (h + l) / 2; S.in[i].big[0] = 5; S.in[i].bigKey[0] = l;
}
static Syn base(int n, long long boO, long long boC, int trend) {
    Syn S; S.in.resize(n); S.bar.resize(n); S.up.assign(n, 0); S.dn.assign(n, 0); S.upPx.assign(n, 0); S.dnPx.assign(n, 0); S.skip.assign(n, 0); S.trend.assign(n, 0); S.sess.assign(n, 1);
    for (int i = 0; i < 40; ++i) put(S, i, 101, 105, 100, 102, 100, 10);
    put(S, 25, 101, 120, 100, 102, 100, 10); put(S, 26, 101, 104, 98, 100, 100, 10); put(S, 27, 100, 105, 100, 102, 100, 10);
    put(S, 40, boO, 108, 102, boC, 200, 50);                                   // range 6 > 5, volume up, 10-candle high, big buy 50
    S.up[40] = 1; S.upPx[40] = 106; S.trend[40] = (signed char)trend;
    return S;
}
static bba::BoRun run(Syn& S, int end) {
    std::vector<double> h(S.bar.size()), l(S.bar.size()); for (size_t i = 0; i < S.bar.size(); ++i) { h[i] = (double)S.bar[i].h; l[i] = (double)S.bar[i].l; }
    bba::zigDir(h, l, (int)S.bar.size(), S.zd);
    return bba::boWaves(S.in, S.bar, S.zd, S.up, S.dn, S.upPx, S.dnPx, S.skip, S.trend, S.sess, 1.0, 1, end, 0);
}
static const bba::BoDec* find40(const bba::BoRun& R) { for (const bba::BoDec& D : R.ev) if (D.i0 == 40) return &D; return nullptr; }

static void units() {
    {   Syn S = base(43, 103, 107, 1); put(S, 41, 107, 110, 106, 109, 150, 10);   // green, higher high 110 > 108, higher close 109 > 107, above 106
        const bba::BoRun R = run(S, 42); const bba::BoDec* D = find40(R);
        CHECK(D && D->fired && D->im == 41 && D->later == 1 && D->trend == 1, "[unit] confirm: next green candle, higher high + higher close beyond the pivot (with the trend)");
        CHECK(D && D->print && D->lots == 50 && D->n == 40, "[unit] the print is the BO bar's (biggest in 40)");
        const bba::BoRun P = run(S, 41); CHECK(!find40(P) && P.pendOn[1], "[unit] not decided before the confirming candle closes (appears one candle later)"); }
    {   Syn S = base(43, 103, 107, 1); put(S, 41, 107, 107, 101, 104, 150, 10);   // lower low 101 < 102: the walk turns down
        const bba::BoRun RR = run(S, 42); const bba::BoDec* D = find40(RR);
        CHECK(D && !D->fired && D->reason == bba::BO_R_NO_CONFIRM && D->im == 41, "[unit] wave flips before confirm -> no_confirm (logged, not drawn)"); }
    {   Syn S = base(43, 103, 107, 0); put(S, 41, 107, 110, 106, 109, 150, 10);   // Balance + the 120 spike in the 30-bar window
        const bba::BoRun RR = run(S, 42); const bba::BoDec* D = find40(RR);
        CHECK(D && !D->fired && D->reason == bba::BO_R_COUNTER && D->ext30 == 0, "[unit] counter-trend (Balance) without a 30-bar high -> counter_trend_no_30"); }
    {   Syn S = base(43, 103, 107, -1); put(S, 41, 107, 121, 106, 109, 150, 10);  // Dntrend, but the confirming candle makes a 30-bar high (121 > 120)
        const bba::BoRun RR = run(S, 42); const bba::BoDec* D = find40(RR);
        CHECK(D && D->fired && D->ext30 == 1, "[unit] counter-trend with a 30-bar high by the confirming close -> fired"); }
    {   Syn S = base(43, 103, 107, 0); S.sess.assign(43, 1); for (int i = 0; i < 30; ++i) S.sess[i] = 0; put(S, 41, 107, 110, 106, 109, 150, 10);   // session starts at 30: the spike is clipped out
        const bba::BoRun RR = run(S, 42); const bba::BoDec* D = find40(RR);
        CHECK(D && D->fired && D->ext30 == 1, "[unit] the 30-bar window is clipped at the RTH session start (like Rly EXT_BARS)"); }
    {   Syn S = base(44, 103, 105, 1); put(S, 41, 105, 109, 104, 105, 150, 10); put(S, 42, 105, 110, 104, 107, 150, 10);   // BO bar closes 105 < 106 (wick); 41 higher but closes 105 (not beyond); 42 confirms
        const bba::BoRun RR = run(S, 43); const bba::BoDec* D = find40(RR);
        CHECK(D && D->fired && D->wick == 1 && D->im == 42 && D->later == 2, "[unit] wick BO bar; a higher candle not closing beyond the pivot waits; the next one confirms"); }
    {   Syn S = base(43, 107, 104, 1); put(S, 41, 104, 110, 104, 109, 150, 10);   // BO bar red
        const bba::BoRun RR = run(S, 42); const bba::BoDec* D = find40(RR);
        CHECK(D && !D->fired && D->reason == bba::BO_R_COLOUR, "[unit] a red BO bar -> wrong_colour (logged)"); }
    {   Syn S = base(43, 103, 107, 1); put(S, 41, 107, 110, 106, 106, 150, 10);   // red-ish: close 106 < open 107 -> not the same colour
        const bba::BoRun R = run(S, 42);
        CHECK(!find40(R) && R.pendOn[1], "[unit] a higher candle of the wrong colour does not confirm (still waiting)"); }
    {   Syn S = base(43, 103, 107, 1); put(S, 41, 107, 110, 106, 109, 150, 10); S.skip[40] = 1;
        const bba::BoRun RR = run(S, 42); const bba::BoDec* D = find40(RR);
        CHECK(D && !D->fired && D->reason == bba::BO_R_TIME, "[unit] BO bar in the time filter -> time_filter (not drawn)"); }
    {   Syn S = base(43, 103, 107, 1); put(S, 41, 107, 110, 106, 109, 150, 10);   // restart: a later start gives the same decision
        std::vector<double> h(43), l(43); for (int i = 0; i < 43; ++i) { h[i] = (double)S.bar[i].h; l[i] = (double)S.bar[i].l; } bba::zigDir(h, l, 43, S.zd);
        const bba::BoRun R = bba::boWaves(S.in, S.bar, S.zd, S.up, S.dn, S.upPx, S.dnPx, S.skip, S.trend, S.sess, 1.0, 30, 42, 35);
        const bba::BoDec* D = find40(R); CHECK(D && D->fired && D->im == 41, "[unit] recompute from a later start = the same decision (no repaint)"); }
    // records
    {   bba::BoDec Q;
        const char* old = "BBO|1792763640|1792763460|-1|1|fired|7676.25|1|30746|30704|42|18|9859|6882|120|30705|80|12|1|1|1|30704|1|1.27.1";
        CHECK(bba::parseBoLine(old, Q) && Q.rule == 1 && Q.fired && Q.t == 1792763640LL && bba::boMarkT(Q) == Q.t, "[record] an old 24-field (1.27.1) line still parses: rule 1, drawn on t as before");
        bba::BoDec D; D.t = 1792763640LL; D.t0 = 1792763460LL; D.dir = -1; D.fired = true; D.reason = bba::BO_R_FIRED; D.piv = 7676.25; D.which = 1; D.hk = 30746; D.lk = 30704;
        D.range = 42; D.prange = 18; D.vol = 9859; D.pvol = 6882; D.lots = 120; D.key = 30705; D.prior = 80; D.n = 12; D.print = true; D.wick = 0; D.later = 2; D.ck = 30704; D.rule = 2; D.trend = -1; D.ext30 = 1;
        const std::string ln = bba::boLine(D, 1, bba::VERSION);
        CHECK(bba::parseBoLine(ln, Q) && Q.rule == 2 && Q.trend == -1 && Q.ext30 == 1 && Q.n == 12 && Q.later == 2 && bba::boMarkT(Q) == D.t0, "[record] rule-2 27-field round trip (letter on t0)");
        { std::vector<std::string> f; std::string cur; for (char c : ln) { if (c == '|') { f.push_back(cur); cur.clear(); } else cur.push_back(c); } f.push_back(cur);
          CHECK(f.size() == 27 && f[23] == bba::VERSION && f[24] == "2", "[record] the first 24 fields keep their places (version at 23), rule after it"); }
        bba::BoDec W = D; W.t = W.t0; CHECK(!bba::parseBoLine(bba::boLine(W, 1, "v"), Q), "[record] a fired rule-2 line with t == t0 is refused");
        W = D; W.fired = false; W.reason = bba::BO_R_COUNTER; CHECK(bba::parseBoLine(bba::boLine(W, 1, "v"), Q) && Q.reason == bba::BO_R_COUNTER, "[record] counter_trend_no_30 round trip");
        W = D; W.fired = false; W.reason = bba::BO_R_NO_CONFIRM; CHECK(bba::parseBoLine(bba::boLine(W, 1, "v"), Q) && Q.reason == bba::BO_R_NO_CONFIRM, "[record] no_confirm round trip");
        {   std::ifstream rf("es_2026-10-23_breakouts_1.27.0.csv"); std::string ln; int lines = 0, ok = 0; std::getline(rf, ln);
            while (std::getline(rf, ln)) { if (ln.empty()) continue; ++lines; bba::BoDec O; if (bba::parseBoLine(ln, O) && O.rule == 1) ++ok; }
            CHECK(lines > 20 && ok == lines, "[record] every line of the real 1.27.0 record (ES 2026-10-23) still parses (rule 1)"); }
        CHECK(std::string(bba::BO_HEADER).find("|version|rule|trend|ext30") != std::string::npos, "[record] header names the 3 new fields");
        const std::string det = bba::boDetail(D, 0.25, "11:54 AM");
        CHECK(det.find("biggest in 12") != std::string::npos && det.find("120 lots") != std::string::npos && det.find("Dntrend") != std::string::npos, "[status] detail: biggest in N first, lots after, trend named");
        std::printf("  detail: %s\n", det.c_str()); }
}

int main(int argc, char** argv) {
    realSession(argc > 1 ? argv[1] : "es_2026-10-23_3m.txt", argc > 2 ? argv[2] : "es_2026-10-23_facts.csv");
    units();
    std::printf(fails ? "FAILED %d (passed %d)\n" : "ALL PASS (%d checks)\n", fails ? fails : passes, passes);
    return fails != 0;
}
