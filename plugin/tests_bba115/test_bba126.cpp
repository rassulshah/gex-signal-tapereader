// test_bba126.cpp -- lsBigBidAsk 1.26.0 (Rassul 18:01 CT 2026-10-22: "you only put the volume at the bid or ask like we currently do at
// the bo or the second high volume bid or ask if it exists near the pivot using the pivot rule, its as simple as that"):
//   [swing]  bba::swingBreaksPx: the BO pivot = one of the LAST 3 INTACT swings of the 1-bar ZigZag (a 4th older one never counts, a
//            swing a wick passed is gone, the highest of several broken is the pivot), the mirror for lows, prefix runs never change
//   [pick]   obk::pick: the highest bid / ask cell; the 2nd when a BO mark already shows the top; none when nothing is left
//   [rec]    OBX| round trip (rank field), 1.25.0 box lines / BO kind refused as legacy, garbled lines refused; obOf window = the pivot
//            candle + 1 - 3 candles, never past the wave's extreme
//   [host]   ES 2026-10-05 + NQ 2026-10-09 (3-min bars with every price's bid / ask lots from the MBO trades): NO rectangles in the
//            OB / BO colours, OB letters below an up-wave candle / above a down-wave candle, BO letters green above / red below (no
//            BD), every OB print is drawn on its candle, the OB is the top cell of its window or the 2nd when the top is the BO print,
//            settings: OB off removes the OB letters, a restart after EVERY candle 8:30 AM - 12:00 PM = the same OBX / BBO record and
//            the same letters as one straight pass (no repaint); lists every OB / BO label of the two days.
// Build (from tests_bba115/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_bba126.cpp -o /tmp/t_bba126 && /tmp/t_bba126
#include "irtsdk.h"
#include "../BigBidAsk.cpp"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <set>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)

// ---- [swing] ---------------------------------------------------------------------------------------------------------------------
static void swb(const std::vector<double>& H, const std::vector<double>& L, bool red, std::vector<signed char>& up, std::vector<signed char>& dn, std::vector<double>& pu, std::vector<double>& pd) {
    const int n = (int)H.size(); std::vector<double> O(n), C(n); for (int k = 0; k < n; ++k) { O[(size_t)k] = red ? H[(size_t)k] : L[(size_t)k]; C[(size_t)k] = red ? L[(size_t)k] : H[(size_t)k]; }
    std::vector<char> r((size_t)n, 1); std::vector<long long> d((size_t)n, 7);
    bba::swingBreaksPx(O, H, L, C, r, d, n, 0.25, up, dn, pu, pd);
}
static void swing() {
    // SwingRef.h's rule: a falling staircase of 4 confirmed swing highs 110 / 108 / 106 / 104, then a candle 103 -> 111
    std::vector<double> H = { 100, 110, 107, 108, 105, 106, 103, 104, 101, 102, 111 }, L = { 98, 107, 104, 105, 102, 103, 100, 101, 99, 100, 103 };
    std::vector<signed char> up, dn; std::vector<double> pu, pd;
    swb(H, L, false, up, dn, pu, pd);
    CHECK(up[10] == 3 && pu[10] == 108, "[swing] through 4 swing highs: only the last 3 count, the highest of them (108, 3 back) is the pivot");
    std::vector<double> H2 = H, L2 = L; H2[10] = 105; L2[10] = 101.5;
    swb(H2, L2, false, up, dn, pu, pd);
    CHECK(up[10] == 1 && pu[10] == 104, "[swing] through the latest swing high only: 1 back, pivot 104");
    H2.push_back(104.5); L2.push_back(103.5); H2.push_back(104.75); L2.push_back(104.25);
    swb(H2, L2, false, up, dn, pu, pd);
    CHECK(up[11] == 0 && up[12] == 0, "[swing] a swing a candle already went above is not intact: no second break of 104");
    std::vector<double> H3 = { 100, 103, 101, 102, 106 }, L3 = { 98, 101, 99, 100, 104 };
    swb(H3, L3, false, up, dn, pu, pd);
    CHECK(up[4] == 0, "[swing] a candle whose low is above the swing high did not trade through it");
    std::vector<double> Hm, Lm; for (size_t k = 0; k < H.size(); ++k) { Hm.push_back(400 - L[k]); Lm.push_back(400 - H[k]); }
    swb(Hm, Lm, true, up, dn, pu, pd);
    CHECK(dn[10] == 3 && pd[10] == 292, "[swing] mirror: the lowest of the last 3 swing lows (292)");
    {   std::vector<double> O(H.size()), C(H.size()); for (size_t k = 0; k < H.size(); ++k) { O[k] = L[k]; C[k] = H[k]; }
        std::vector<char> r(H.size(), 1); std::vector<long long> d(H.size(), 7); d[10] = 8;
        bba::swingBreaksPx(O, H, L, C, r, d, (int)H.size(), 0.25, up, dn, pu, pd);
        CHECK(up[10] == 0, "[swing] a new RTH session starts a new zigzag: yesterday's swings never count");
        r.assign(H.size(), 1); d.assign(H.size(), 7); r[10] = 0; bba::swingBreaksPx(O, H, L, C, r, d, (int)H.size(), 0.25, up, dn, pu, pd);
        CHECK(up[10] == 0, "[swing] an overnight candle: no BO pivot (RTH zigzag only)"); }
    std::vector<signed char> u0, d0; std::vector<double> p0, q0; swb(H2, L2, false, u0, d0, p0, q0);
    bool same = true;
    for (int n = 2; n <= (int)H2.size(); ++n) { std::vector<double> h(H2.begin(), H2.begin() + n), l(L2.begin(), L2.begin() + n);
        swb(h, l, false, up, dn, pu, pd); for (int k = 0; k < n; ++k) if (up[(size_t)k] != u0[(size_t)k] || dn[(size_t)k] != d0[(size_t)k] || pu[(size_t)k] != p0[(size_t)k]) same = false; }
    CHECK(same, "[swing] a pure function of the bars up to the candle (prefix runs agree)");
}

// ---- [pick] + [rec] ------------------------------------------------------------------------------------------------------------
static obk::CandleCells cells(long long t, std::vector<double> px, std::vector<long long> bid, std::vector<long long> ask) { obk::CandleCells c; c.t = t; c.px = px; c.bid = bid; c.ask = ask; return c; }
static void pickRec() {
    std::vector<obk::CandleCells> C = { cells(100, { 10.0, 10.25 }, { 50, 120 }, { 30, 10 }), cells(280, { 10.5, 10.75 }, { 5, 7 }, { 90, 120 }) };
    int rank = -1; obk::Print p = obk::pick(C, {}, 0.25, rank);
    CHECK(rank == 1 && p.lots == 120 && p.t == 100 && p.side == -1 && p.px == 10.25, "[pick] the highest cell (120 at the bid 10.25, the earlier candle wins the tie)");
    obk::Print bo; bo.t = 100; bo.px = 10.25; bo.side = -1; bo.lots = 120;
    p = obk::pick(C, { bo }, 0.25, rank);
    CHECK(rank == 2 && p.lots == 120 && p.t == 280 && p.side == 1, "[pick] the top is the BO print: the 2nd highest (120 at the ask 10.75)");
    obk::Print other = bo; other.side = 1;
    p = obk::pick(C, { other }, 0.25, rank); CHECK(rank == 1 && p.t == 100, "[pick] a BO print on another side / price does not take the top");
    std::vector<obk::CandleCells> one = { cells(100, { 10.25 }, { 120 }, { 0 }) };
    p = obk::pick(one, { bo }, 0.25, rank); CHECK(rank == 0, "[pick] the only cell is the BO's: no OB print (none drawn)");
    p = obk::pick({}, {}, 0.25, rank); CHECK(rank == 0, "[pick] no cells: no print");
    // the record
    obk::Box b; b.dir = 1; b.cls = obk::CLS_RLY; b.t0 = b.t1 = 280; b.tp = 100; b.te = 900; b.tc = 1080; b.lo = b.hi = 10.75; b.pivot = 9.75; b.nb = 6; b.ratio = 3.4; b.atr = 2.5; b.rly = true; b.rank = 2;
    b.prints.push_back(p.lots ? p : obk::Print()); b.prints[0].t = 280; b.prints[0].px = 10.75; b.prints[0].side = 1; b.prints[0].lots = 120;
    obk::Box q; const std::string ln = obk::line(b, "1.26.0"); bool leg = true;
    CHECK(obk::parseLine(ln, q, &leg) && !leg && q.rank == 2 && q.has() && q.prints[0].lots == 120 && q.prints[0].side == 1 && q.id() == b.id(), "[rec] OBX round trip with the rank");
    CHECK(!obk::parseLine("OBX|OB|1|1|1|2|1|3|4|10|11|10|3|3.0|2|1|300|2|1:10:B:300;2:11:S:200|1.25.0", q, &leg) && leg, "[rec] a 1.25.0 box line is legacy (not drawn)");
    CHECK(!obk::parseLine("OBX|BO|1|1|1|2|1|3|4|10|11|10|3|3.0|2|1|300|0|-|1.26.0", q, &leg) && leg, "[rec] a BO box line is legacy");
    std::string bad = ln; bad.replace(bad.find("|2|1|"), 5, "|0|1|");
    CHECK(!obk::parseLine(bad, q, &leg) && !leg, "[rec] rank 0 with a print refused");
    CHECK(!obk::parseLine("OBX|OB|1|1|x", q, &leg) && !obk::parseLine(obk::REC_HEADER, q, &leg), "[rec] garbled / header refused");
    // obOf: the window
    rly::Sess S; for (int k = 0; k < 12; ++k) S.g.push_back(k + 100);
    std::vector<rly::Wave> W(1); W[0].fired = true; W[0].dir = 1; W[0].gp = 102; W[0].ge = 104; W[0].gc = 105; W[0].nb = 2; W[0].from = 9.75;
    for (int og = 1; og <= 3; ++og) { obk::Params P; P.origin = og; std::vector<obk::Cand> o; obk::obOf(S, W, P, o);
        CHECK(o.size() == 1 && o[0].i0 == 102 && o[0].i1 == std::min(102 + og, 104) && o[0].pivot == 9.75, "[rec] OB window = pivot candle + " + std::to_string(og) + ", never past the extreme"); }
    { obk::Params P; P.origin = 9; std::vector<obk::Cand> o; W[0].ge = 110; obk::obOf(S, W, P, o); CHECK(o.size() == 1 && o[0].i1 == 105, "[rec] origin capped at 3"); }
    { obk::Params P; std::vector<obk::Cand> o; W[0].fired = false; W[0].atr = 2; W[0].ratio = 2.9; obk::obOf(S, W, P, o); CHECK(o.empty(), "[rec] not an Rly wave and under 3 ATR: no OB");
      W[0].ratio = 3.0; o.clear(); obk::obOf(S, W, P, o); CHECK(o.size() == 1 && o[0].cls == obk::CLS_ATR, "[rec] a 3.0 ATR wave: OB (class 3+ ATR)"); }
}

// ---- [host] -----------------------------------------------------------------------------------------------------------------------
struct Fx { std::string end; double o, h, l, c; long long v; std::vector<mock::VRow> rows; };
static std::vector<Fx> fixture(const std::string& path) {
    std::vector<Fx> v; std::ifstream f(path.c_str()); std::string ln; std::getline(f, ln);
    while (std::getline(f, ln)) { Fx x; char e[32]; char cs[200000];
        if (std::sscanf(ln.c_str(), "%19[^|]|%lf|%lf|%lf|%lf|%lld|%199999s", e, &x.o, &x.h, &x.l, &x.c, &x.v, cs) != 7) continue;
        x.end = e; std::istringstream in(cs); std::string cell;
        while (std::getline(in, cell, ';')) { double p = 0; long a = 0, b = 0; if (std::sscanf(cell.c_str(), "%lf:%ld:%ld", &p, &a, &b) == 3) x.rows.push_back(mock::VRow{ (float)p, a, b }); }
        v.push_back(x); }
    return v;
}
static unsigned long stampOf(const std::string& e) {
    struct tm t; std::memset(&t, 0, sizeof(t)); std::sscanf(e.c_str(), "%d-%d-%d %d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min);
    t.tm_year -= 1900; t.tm_mon -= 1; return (unsigned long)timegm(&t);
}
static std::vector<mock::Bar> bars(const std::vector<Fx>& F, size_t upto) {
    std::vector<mock::Bar> v;
    for (size_t k = 0; k < F.size() && k < upto; ++k) { const Fx& x = F[k]; mock::Bar b; b.time = stampOf(x.end); b.open = (float)x.o; b.high = (float)x.h; b.low = (float)x.l; b.close = (float)x.c; b.volume = (unsigned long)x.v;
        b.rows = x.rows; if (b.rows.empty()) b.rows.push_back(mock::VRow{ (float)x.c, 1, 1 }); v.push_back(b); }
    return v;
}
static int idxOf(const std::vector<Fx>& F, const std::string& end) { for (size_t k = 0; k < F.size(); ++k) if (F[k].end == end) return (int)k; return -1; }
static std::string MKT = "ES", SYM = "ESZ6";
struct Chart {
    mock::Host H; BigBidAsk* x = nullptr;
    explicit Chart(const std::vector<mock::Bar>& b) { H.bars = b; H.now = b.back().time; H.symbol = SYM; H.root = MKT; H.chart = MKT + " 3m"; H.tick = 0.25f;
        H.paneBottom = 1400; H.pxPerTick = 1; H.barW = 40;
        mock::current = &H; x = static_cast<BigBidAsk*>(CreateExtension()); x->setup(); }
    ~Chart() { mock::current = &H; x->destroy(); delete x; mock::current = nullptr; }
    void pump(int n, bool draw) {
        for (int k = 0; k < n; ++k) { mock::current = &H; BBState* s = x->testState();
            if (s) { s->builtMs = -1; s->boardMs = -1; s->sdCheckMs = -1; s->statsMs = -1; s->lastStatusMs = -1; s->logMs = -1; s->boMs = -1; s->boFlushMs = -1; s->obFlushMs = -1; s->obRetryMs = -1; }
            if (draw) { H.draws.clear(); x->draw(); } else x->calc(0); }
    }
    void view(int v0, int v1) { H.visStart = v0; H.visEnd = v1; H.barOffset = v0;
        float lo = 1e9f, hi = -1e9f; for (int k = v0; k <= v1 && k < (int)H.bars.size(); ++k) { lo = std::min(lo, H.bars[(size_t)k].low); hi = std::max(hi, H.bars[(size_t)k].high); }
        H.priceBase = lo - 30; H.pxPerTick = std::max(0.25f, std::min(4.0f, 1000.0f / std::max(1.0f, (hi - lo + 60) / H.tick)));
        pump(2, true); }
    BBState& st() { return *x->testState(); }
};
static std::string HOME;
static void home(const std::string& name, const ss::Values* set) {
    const std::string root = "/tmp/bba126-run", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str()) != 0) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1); HOME = h;
    std::ofstream f((h + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings2-" + MKT + ".txt").c_str()); f << ss::format(bbaset::spec(), *set);
    // the 1.25.1 nightly verdict on the BOXES (LOG ONLY): the 1.26.0 letters are drawn anyway (his 18:01 request), the status reports it
    std::ofstream g((h + "/InvestorRT/rtx/lsFlexLevels/LRA-OrderBlock-" + MKT + ".csv").c_str()); g << "VERSION,1\nMARKET," << MKT << "\nPARAM,SHOW,0\n";
}
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static long long SINCE = 0;
static std::set<std::string> obLines(Chart& c) { std::set<std::string> o; for (const auto& kv : c.st().obRec) if (kv.second.te >= SINCE) o.insert(obk::line(kv.second, "x")); return o; }
static std::set<std::string> boFired(Chart& c) { std::set<std::string> o; for (const auto& kv : c.st().boDec) if (kv.second.fired && kv.second.t >= SINCE) o.insert(bba::boLine(kv.second, 0, "x")); return o; }
static std::string clk(long long t) { const int st = (int)(t % 86400) - 180; char b[16]; const int h = st / 3600, m = st / 60 % 60; std::snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return b; }
static std::string skipTag(long long t) { const long long e = t % 86400, s = e - 180; return (s < 8 * 3600 + 45 * 60 || e > 14 * 3600 + 45 * 60) ? "  [skip first / last 15 min: recorded, not drawn]" : ""; }
static bool hits(const mock::Draw& a, const mock::Draw& b) { return !(a.r <= b.l || b.r <= a.l || a.b <= b.t || b.b <= a.t); }

static void day(const std::string& mkt, const std::string& sym, const std::string& file, const std::string& date) {
    MKT = mkt; SYM = sym; const std::string tag = "[" + mkt + " " + date + "] ";
    const std::vector<Fx> F = fixture(file);
    CHECK(F.size() > 400, tag + "fixture read (" + std::to_string(F.size()) + " candles)");
    const int i830 = idxOf(F, date + " 08:33"), i1200 = idxOf(F, date + " 12:00"), iEnd = (int)F.size() - 1;
    CHECK(i830 > 0 && i1200 > i830, tag + "8:30 / 12:00 found");
    ss::Values V0 = ss::defaults(bbaset::spec(), ss::marketIndex(mkt));
    SINCE = (long long)stampOf(date + " 08:30");
    std::set<std::string> sOB, sBO, sOB12, sBO12;
    {   home(mkt + "-straight", &V0);
        Chart c(bars(F, (size_t)iEnd + 1)); c.pump(16, false);
        sOB = obLines(c); sBO = boFired(c);
        CHECK(!sOB.empty() && !sBO.empty(), tag + "OB and BO decided (" + std::to_string(sOB.size()) + " OB, " + std::to_string(sBO.size()) + " BO)");
        // the OB rule on the record: its print is the top cell of the window, or the 2nd when the top is a BO print on those candles
        int okRule = 0, n = 0, second = 0;
        for (const auto& kv : c.st().obRec) { const obk::Box& b = kv.second; if (b.te < SINCE) continue; ++n;
            const int ip = idxOf(F, ""), _u = ip; (void)_u;
            int i0 = -1; for (size_t k = 0; k < F.size(); ++k) if ((long long)stampOf(F[k].end) == b.tp) i0 = (int)k;
            const int ie = [&] { for (size_t k = 0; k < F.size(); ++k) if ((long long)stampOf(F[k].end) == b.te) return (int)k; return -1; }();
            const int i1 = std::min(i0 + c.st().obOrigin, ie);
            std::vector<obk::CandleCells> C;
            for (int j = i0; j <= i1; ++j) { obk::CandleCells cc; cc.t = (long long)stampOf(F[(size_t)j].end);
                for (const auto& r : F[(size_t)j].rows) { cc.px.push_back(r.price); cc.bid.push_back(r.bid); cc.ask.push_back(r.ask); } C.push_back(cc); }
            std::vector<obk::Print> taken;
            for (const auto& d : c.st().boDec) { const bba::BoDec& D = d.second; if (!D.fired || !D.print || D.t < C.front().t || D.t > C.back().t) continue;
                obk::Print q; q.t = D.t; q.px = D.key * 0.25; q.side = D.dir > 0 ? 1 : -1; q.lots = D.lots; taken.push_back(q); }
            int rk = 0; const obk::Print p = obk::pick(C, taken, 0.25, rk);
            if (b.rank == 2) ++second;
            if (rk == b.rank && (rk == 0 || (b.has() && obk::samePrint(p, b.prints[0], 0.25) && p.lots == b.prints[0].lots))) ++okRule;
            else CHECK(false, tag + "OB at " + clk(b.te) + " is not the top / 2nd cell of its window"); }
        CHECK(n > 0 && okRule == n, tag + "every OB print = the highest bid / ask cell of the pivot candle + next candles, 2nd when the BO showed the top (" + std::to_string(okRule) + " / " + std::to_string(n) + ", 2nd " + std::to_string(second) + ")");
        // the chart: views over the RTH day
        int obL = 0, obBad = 0, boL = 0, boBad = 0, bd = 0, rects = 0, overlap = 0, obPrintMissing = 0;
        std::map<long long, const obk::Box*> obByPrint; for (const auto& kv : c.st().obRec) if (kv.second.has() && kv.second.te >= SINCE) obByPrint[kv.second.prints[0].t] = &kv.second;
        for (int v0 = i830; v0 <= iEnd - 1; v0 += 45) {
            const int v1 = std::min(iEnd, v0 + 49); c.view(v0, v1);
            std::vector<const mock::Draw*> marks;
            for (const auto& d : c.H.draws) {
                if (d.type == "rect" && (d.color == 0x0022C55E || d.color == 0x00EF4444)) ++rects;
                if (d.type == "ring" || (d.type == "ctext" && (d.text == "OB" || d.text == "BO" || d.text == "BD" || d.text == "Rly"))) if (d.color != 0) marks.push_back(&d);
                if (d.type != "ctext" || d.color == 0) continue;                 // the black 1 px edge (4 draws) under every letter
                if (d.text == "BD") ++bd;
                const int bar = v0 + ((d.l + d.r) / 2 - 20) / c.H.barW; if (bar < v0 || bar > v1) continue;
                cppExtension::PNT ph, pl; ph.set(bar, c.H.bars[(size_t)bar].high); pl.set(bar, c.H.bars[(size_t)bar].low);
                const bool above = d.b <= ph.v + 2, below = d.t >= pl.v - 2;
                if (d.text == "BO") { ++boL; if (!((d.color == 0x0022C55E && above) || (d.color == 0x00EF4444 && below))) ++boBad; }
                if (d.text == "OB") { ++obL; const auto it = obByPrint.find((long long)c.H.bars[(size_t)bar].time);
                    if (it == obByPrint.end() || !((it->second->dir > 0 && below && d.color == 0x0022C55E) || (it->second->dir < 0 && above && d.color == 0x00EF4444))) ++obBad;
                    else { bool pr = false; cppExtension::PNT pp; pp.set(bar, (float)it->second->prints[0].px, cppExtension::kBarCenter);
                        for (const auto& r : c.H.draws) if (r.type == "ring" && r.t <= pp.v + 1 && r.b >= pp.v - 1 && std::abs((r.l + r.r) / 2 - pp.h) <= c.H.barW / 2) pr = true;
                        if (!pr) ++obPrintMissing; } }
            }
            // letters never over each other (rings of one candle sit side by side and may touch the candle's own letter edge: 1 px)
            for (size_t a = 0; a < marks.size(); ++a) for (size_t b = a + 1; b < marks.size(); ++b) {
                if (marks[a]->type == "ring" && marks[b]->type == "ring") continue;
                mock::Draw A = *marks[a], B = *marks[b]; A.l += 1; A.r -= 1; A.t += 1; A.b -= 1;
                if (hits(A, B)) ++overlap; }
        }
        CHECK(rects == 0, tag + "no rectangles in the OB / BO colours (boxes / zones / bands gone)");
        CHECK(bd == 0, tag + "no BD letter: both sides are BO");
        CHECK(boL > 0 && boBad == 0, tag + "BO green above a breakout / red below a breakdown (" + std::to_string(boL) + " letters, " + std::to_string(boBad) + " wrong)");
        (void)obBad; CHECK(obL == 0, tag + "1.29.0 (Rassul 2026-10-26 17:55 CT): no OB letter is drawn (" + std::to_string(obL) + ")");
        CHECK(obPrintMissing == 0, tag + "every OB letter has its print box on the candle at the print's price");
        CHECK(overlap == 0, tag + "no letter over another letter or a print box (" + std::to_string(overlap) + ")");
        const std::string stt = slurp(HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.status-" + mkt + "-" + sym + "-180.txt");
        CHECK(stt.find("VERSION,1.29.0\n") == 0 && stt.find("\nORDER_BLOCK,OB not drawn (retired 1.29.0") != std::string::npos && stt.find("logic 2.0.0 (prints + letters only; no boxes)") != std::string::npos,
              tag + "status: version 1.29.0 + ORDER_BLOCK 'not drawn' line; OB still decided + recorded");
        // the list for Rassul
        std::cout << "  " << mkt << " " << date << " RTH labels:\n";
        std::map<long long, std::string> L;
        for (const auto& kv : c.st().boDec) { const bba::BoDec& D = kv.second; if (!D.fired || D.t < SINCE) continue;
            L[D.t * 10 + 1] = "    " + clk(D.t) + "  BO " + (D.dir > 0 ? "up  (green, above)" : "down (red, below) ") + "  swing " + bba::price((long long)std::llround(D.piv / 0.25), 0.25) + (D.which > 1 ? " (" + std::to_string(D.which) + " back)" : "") + (D.print ? "  print " + std::to_string(D.lots) + (D.dir > 0 ? " bought at " : " sold at ") + bba::price(D.key, 0.25) : "  (no print)") + skipTag(D.t); }
        for (const auto& kv : c.st().obRec) { const obk::Box& b = kv.second; if (b.te < SINCE) continue;
            const obk::Print q = b.has() ? b.prints[0] : obk::Print();
            L[(b.has() ? q.t : b.tp) * 10 + 2] = "    " + clk(b.has() ? q.t : b.tp) + "  OB " + (b.dir > 0 ? "up-wave  (green, below)" : "down-wave (red, above) ") + (b.cls == obk::CLS_RLY ? " Rly " : " 3ATR")
                + " " + rly::fmtD(b.ratio, 1) + " ATR to " + clk(b.te) + (b.has() ? "  print " + std::to_string(q.lots) + (q.side > 0 ? " at the ask " : " at the bid ") + bba::price((long long)std::llround(q.px / 0.25), 0.25) + (b.rank == 2 ? " (2nd: top = BO print)" : "") : "  no print (not drawn)") + skipTag(b.has() ? q.t : b.tp); }
        for (const auto& kv : L) std::cout << kv.second << "\n";
        // (1.29.0) the OB settings rows are gone: no "ob*" key in the spec; the OB print box at an OB's own price (not a Big Bid/Ask print) is never drawn
        { int obKeys = 0; const ss::Spec& SP = bbaset::spec(); for (int k = 0; k < SP.n; ++k) if (std::string(SP.rows[k].key).compare(0, 2, "ob") == 0 || std::string(SP.rows[k].key).find("_ob_") != std::string::npos) ++obKeys;
          CHECK(obKeys == 0 && SP.paramVersion == 14, tag + "1.29.0: no OB settings rows (" + std::to_string(obKeys) + "), parameter version 14"); }
        CHECK(!c.st().obOn, tag + "1.29.0: the OB draw switch is off for good (decision + record kept: " + std::to_string(sOB.size()) + " OB lines)");
    }
    {   // no repaint: a restart after EVERY candle 8:30 AM - 12:00 PM = one straight pass to 12:00
        home(mkt + "-straight12", &V0);
        { Chart c(bars(F, (size_t)i1200 + 1)); c.pump(16, false); sOB12 = obLines(c); sBO12 = boFired(c); }
        home(mkt + "-restarts", &V0);
        for (int k = i830 - 1; k <= i1200; ++k) { Chart c(bars(F, (size_t)k + 1)); c.pump(4, false); }
        Chart c(bars(F, (size_t)i1200 + 1)); c.pump(4, false);
        const std::set<std::string> rOB = obLines(c), rBO = boFired(c);
        CHECK(!sOB12.empty() && rOB == sOB12, tag + "restart after every candle: the same OB records as one pass (" + std::to_string(rOB.size()) + " / " + std::to_string(sOB12.size()) + ")");
        CHECK(!sBO12.empty() && rBO == sBO12, tag + "restart after every candle: the same BO marks as one pass (" + std::to_string(rBO.size()) + " / " + std::to_string(sBO12.size()) + ")");
        // the 12:00 pass is a prefix of the full day (decisions never change later)
        bool pre = true; for (const std::string& s : sOB12) if (!sOB.count(s)) pre = false; for (const std::string& s : sBO12) if (!sBO.count(s)) pre = false;
        CHECK(pre, tag + "every 12:00 decision is still the same at the close (never repainted)");
        // a 1.25.0 box line and an old-rule BBO line in the record are ignored (counted)
        const std::string obp = HOME + "/InvestorRT/rtx/lsFlexLevels/OrderBlock/" + mkt + "-" + sym + "-180-" + std::to_string(c.st().session) + ".csv";
        { std::ofstream o(obp.c_str(), std::ios::app); o << "OBX|BO|1|1|" << SINCE << "|" << SINCE << "|" << SINCE << "|" << SINCE + 900 << "|" << SINCE + 1080 << "|10|11|10|3|3.0|2|1|300|0|-|1.25.0\n"; }
        Chart c2(bars(F, (size_t)i1200 + 1)); c2.pump(3, false);
        CHECK(c2.st().obLegacy == 1 && obLines(c2) == sOB12, tag + "a 1.25.0 box line in the record is ignored (counted), the OBs are unchanged");
    }
}

int main() {
    setenv("TZ", "UTC", 1); tzset();
    swing(); pickRec();
    day("ES", "ESZ6", "../tests_ob126/es_2026-10-05_3m.txt", "2026-10-05");
    day("NQ", "NQZ6", "../tests_ob126/nq_2026-10-09_3m.txt", "2026-10-09");
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
