// test_bba123.cpp -- lsBigBidAsk 1.23.0 BO / BD rules (Rassul approved 2026-10-22; built on his 11:00 CT "go restart"):
//   BO only on a GREEN candle, BD only on a RED candle; SAME-WAVE rule: a wick-only break of the pivot is confirmed when the breakout
//   candle OR a later candle of the same up-wave (BO) / down-wave (BD) CLOSES beyond the pivot - the mark goes on the confirming
//   candle, decided on closed candles, never repainted; no FB mark - failed wick breaks (the wave ended first) are logged only.
//   [logic] bba::boWaves on synthetic candles: green / red only (BO and the BD mirror), a wick break confirmed later in the same wave,
//           a wrong-colour close beyond waits, the wave-flip candle cannot confirm, the wave ends first = wick_wave_ended (logged, not
//           fired), one break per wave (same_wave_already), the time filter on the MARK candle, emitFrom, decisions never change as
//           candles arrive (prefix runs), the 24-field record line and the 1.22 lines refused
//   [nq]    the NQ 2026-10-21 day: every fired BO is a green candle closing above its pivot, every BD a red candle closing below, each
//           mark at or after its breakout candle in the same wave
//   [host]  the mock chart: letters only on fired marks (8:45 BD for the 8:42 wick break, no 8:33 BO, no FB), a restart after EVERY
//           new candle gives the same record and the same letters as one straight pass (no repaint), old 1.22 record lines ignored
// Build (from tests_bba115/): g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. -I.. test_bba123.cpp -o /tmp/t_bba123 && /tmp/t_bba123
#include "irtsdk.h"
#include "../BigBidAsk.cpp"
#include <iostream>
#include <sys/stat.h>
#include <set>
namespace mock { Host* current = nullptr; }
static int passed = 0, failed = 0;
#define CHECK(c, why) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL line " << __LINE__ << ": " << why << "\n"; } } while (0)

// ---- [logic] -------------------------------------------------------------------------------------------------------------------
struct Syn { std::vector<double> o, h, l, c; std::vector<long long> v; };
// the 1.21 test's ZigZag: tops 105 (bar 2) / 104 (bar 6), bottoms 99; bar 10 trades through 105 (low 103, high 107) = the BO candidate
static Syn base() {
    Syn s; s.h = { 100, 103, 105, 104, 102, 103, 104, 103, 101, 102, 107 }; s.l = { 98, 100, 102, 101, 99, 100, 101, 100, 99, 100, 103 };
    for (size_t k = 0; k < s.h.size(); ++k) { s.o.push_back(s.l[k] + 0.5); s.c.push_back(s.h[k] - 0.5); s.v.push_back(100); }
    return s;
}
static void add(Syn& s, double o, double h, double l, double c, long long v = 100) { s.o.push_back(o); s.h.push_back(h); s.l.push_back(l); s.c.push_back(c); s.v.push_back(v); }
static Syn mirror(const Syn& s) { Syn m; for (size_t k = 0; k < s.h.size(); ++k) { m.o.push_back(400 - s.o[k]); m.c.push_back(400 - s.c[k]); m.h.push_back(400 - s.l[k]); m.l.push_back(400 - s.h[k]); m.v.push_back(s.v[k]); } return m; }
static bba::BoRun run(const Syn& s, std::vector<char> skip = {}, int emitFrom = 0, int nUse = -1) {
    const int n = nUse < 0 ? (int)s.h.size() : nUse; const double tk = 0.25;
    std::vector<double> H(s.h.begin(), s.h.begin() + n), L(s.l.begin(), s.l.begin() + n);
    std::vector<signed char> up, dn, zd; std::vector<double> pu, pd; bba::breakFlagsPx(H, L, n, up, dn, &pu, &pd); bba::zigDir(H, L, n, zd);
    std::vector<bba::BoIn> in((size_t)n); std::vector<bba::BoBar> bb((size_t)n);
    for (int k = 0; k < n; ++k) { bba::BoIn& b = in[(size_t)k]; b.known = true; b.hk = std::llround(s.h[(size_t)k] / tk); b.lk = std::llround(s.l[(size_t)k] / tk); b.vol = s.v[(size_t)k];
        b.big[0] = b.big[1] = 10 + k; b.bigKey[0] = b.lk; b.bigKey[1] = b.hk; bb[(size_t)k].ok = true; bb[(size_t)k].o = std::llround(s.o[(size_t)k] / tk); bb[(size_t)k].c = std::llround(s.c[(size_t)k] / tk); }
    skip.resize((size_t)n, 0);
    return bba::boWaves(in, bb, zd, up, dn, pu, pd, skip, tk, 1, n, emitFrom);
}
static const bba::BoDec* firedOf(const bba::BoRun& R, int dir) { for (const auto& E : R.ev) if (E.fired && E.dir == dir) return &E; return nullptr; }
static int countReason(const bba::BoRun& R, int reason) { int n = 0; for (const auto& E : R.ev) if (E.reason == reason) ++n; return n; }
static void logicSide(bool bd) {
    const std::string tag = bd ? "[logic BD mirror] " : "[logic BO] "; const int dir = bd ? -1 : 1;
    auto M = [&](const Syn& s) { return bd ? mirror(s) : s; };
    {   // green candle closing above the pivot: fired on the breakout candle itself
        Syn s = base(); s.o[10] = 103.5; s.c[10] = 106; const bba::BoRun R = run(M(s));
        const bba::BoDec* F = firedOf(R, dir);
        CHECK(F && F->i0 == 10 && F->im == 10 && !F->wick && F->later == 0, tag + "the breakout candle closes beyond the pivot in its colour: marked on itself");
        CHECK(!R.pendOn[0] && !R.pendOn[1], tag + "nothing left waiting");
    }
    {   // closes beyond the pivot but in the WRONG colour (red for BO): not marked; a later right-colour close in the same wave confirms
        Syn s = base(); s.o[10] = 106.75; s.c[10] = 105.5;                                       // red, closed above 105
        bba::BoRun R0 = run(M(s)); CHECK(!firedOf(R0, dir) && R0.pendOn[bd ? 0 : 1], tag + "wrong colour: no mark, the break waits for a close in its wave");
        add(s, 105.5, 108, 104, 107);                                                             // bar 11: green, higher low (still up-wave)
        const bba::BoRun R = run(M(s)); const bba::BoDec* F = firedOf(R, dir);
        CHECK(F && F->i0 == 10 && F->im == 11 && F->wick == 1 && F->later == 1, tag + "a later right-colour close in the same wave confirms; the mark goes on THAT candle (11)");
        CHECK(F && F->lots == 21 && F->prior == 20 && F->print, tag + "the print is the confirming candle's (21 > 20)");
    }
    {   // a wick-only break, confirmed 2 candles later in the same wave
        Syn s = base(); s.o[10] = 103.5; s.c[10] = 104.5;                                         // green, closed BELOW 105: wick only
        add(s, 104.5, 106, 104, 104.75);                                                          // 11: green, not beyond
        add(s, 104.75, 107.5, 104.25, 106.5);                                                     // 12: green, closes 106.5 > 105, higher low
        const bba::BoRun R = run(M(s)); const bba::BoDec* F = firedOf(R, dir);
        CHECK(F && F->i0 == 10 && F->im == 12 && F->wick == 1 && F->later == 2 && F->piv == (bd ? 295 : 105), tag + "wick break at 10 confirmed by the close at 12 (same wave), marked on 12");
        CHECK(countReason(R, bba::BO_R_WAVE_ENDED) == 0, tag + "nothing failed");
        int f = 0; for (const auto& E : R.ev) if (E.fired) ++f; CHECK(f == 1, tag + "ONE mark for the break");
    }
    {   // the wave ends first: the wick break FAILED - logged (wick_wave_ended), nothing fired; the flip candle cannot confirm
        Syn s = base(); s.o[10] = 103.5; s.c[10] = 104.5;
        add(s, 104.5, 106, 104, 104.75);                                                          // 11: still up, not beyond
        add(s, 103.25, 106, 103.0, 105.5);                                                        // 12: LOWER LOW (flips the walk down) yet green closing above 105
        const bba::BoRun R = run(M(s));
        CHECK(!firedOf(R, dir), tag + "the candle that ends the wave cannot confirm (it belongs to the new wave)");
        const bba::BoDec* E = nullptr; for (const auto& x : R.ev) if (x.reason == bba::BO_R_WAVE_ENDED) E = &x;
        CHECK(E && E->i0 == 10 && E->im == 12 && !E->fired && E->dir == dir, tag + "logged as wick_wave_ended on the wave-end candle (12), not fired (no FB mark)");
        CHECK(!R.pendOn[bd ? 0 : 1], tag + "no longer waiting");
        add(s, 105.5, 108, 104.0, 107.5);                                                         // 13: a new up-wave closing above 105 - too late for this break
        const bba::BoRun R2 = run(M(s)); bool late = false; for (const auto& x : R2.ev) if (x.fired && x.i0 == 10) late = true;
        CHECK(!late, tag + "a close in a LATER wave never revives the failed break");
    }
    {   // one break per wave: a second qualifying candle in the same wave is logged same_wave_already
        Syn s = base(); s.o[10] = 103.5; s.c[10] = 106;
        add(s, 105.5, 108.5, 104.5, 108, 300);                                                    // 11: trades through 105 again, 10-high, range + volume up
        const bba::BoRun R = run(M(s));
        int f = 0; for (const auto& E : R.ev) if (E.fired) ++f;
        CHECK(f == 1 && countReason(R, bba::BO_R_SAME_WAVE) == 1, tag + "one mark per wave; the second candidate logged same_wave_already");
    }
    {   // the time filter applies to the MARK candle
        Syn s = base(); s.o[10] = 103.5; s.c[10] = 104.5; add(s, 104.5, 108, 104, 107);
        std::vector<char> sk(12, 0); sk[11] = 1; const bba::BoRun R = run(M(s), sk);
        CHECK(!firedOf(R, dir) && countReason(R, bba::BO_R_TIME) == 1, tag + "the confirming candle inside the skipped minutes: time_filter, not drawn");
        std::vector<char> sk2(12, 0); sk2[10] = 1; const bba::BoRun R2 = run(M(s), sk2);
        CHECK(firedOf(R2, dir) && firedOf(R2, dir)->im == 11, tag + "a breakout candle inside the skipped minutes confirmed after them: drawn on the confirming candle");
    }
    {   // emitFrom: earlier candles build the state only
        Syn s = base(); s.o[10] = 103.5; s.c[10] = 104.5; add(s, 104.5, 108, 104, 107);
        const bba::BoRun R = run(M(s), {}, 11); CHECK(R.ev.size() >= 1 && firedOf(R, dir) && firedOf(R, dir)->im == 11, tag + "emitFrom 11: the break that started at 10 is still confirmed on 11");
        const bba::BoRun R2 = run(M(s), {}, 12); CHECK(R2.ev.empty(), tag + "emitFrom past the end: nothing emitted");
    }
    {   // decisions never change as candles arrive: every prefix's events are a prefix of the longer run's
        Syn s = base(); s.o[10] = 103.5; s.c[10] = 104.5; add(s, 104.5, 106, 104, 104.75); add(s, 104.75, 107.5, 104.25, 106.5); add(s, 106, 107, 104, 104.5); add(s, 104.5, 109, 104.25, 108.5, 400);
        const bba::BoRun full = run(M(s)); bool same = true;
        for (int n = 11; n <= (int)s.h.size(); ++n) { const bba::BoRun p = run(M(s), {}, 0, n);
            for (size_t k = 0; k < p.ev.size(); ++k) if (k >= full.ev.size() || bba::boLine(p.ev[k], 0, "x") != bba::boLine(full.ev[k], 0, "x") || p.ev[k].im != full.ev[k].im) same = false; }
        CHECK(same, tag + "no repaint: each decision is the same in every later run");
    }
}
static void logic() {
    logicSide(false); logicSide(true);
    // the record line: 24 fields; the 1.21 / 1.22 20-field line is refused (ignored on load)
    bba::BoDec D; D.t = 1792575360; D.t0 = 1792575180; D.dir = 1; D.fired = true; D.reason = bba::BO_R_FIRED; D.piv = 105; D.which = 2; D.hk = 428; D.lk = 412; D.lots = 21; D.key = 432; D.prior = 20; D.n = 3; D.print = true; D.wick = 1; D.later = 1; D.ck = 428;
    bba::BoDec Q; const std::string ln = bba::boLine(D, 1, bba::VERSION);
    CHECK(bba::parseBoLine(ln, Q) && Q.t == D.t && Q.t0 == D.t0 && Q.wick == 1 && Q.later == 1 && Q.ck == 428 && Q.fired && Q.lots == 21, "[logic] BBO| 24-field round trip (t = mark, t0 = breakout candle)");
    { bba::BoDec W = D; W.t0 = W.t; CHECK(!bba::parseBoLine(bba::boLine(W, 1, "v"), Q), "[logic] 'confirmed later' on the breakout candle itself refused"); }
    { bba::BoDec W = D; W.wick = 0; CHECK(!bba::parseBoLine(bba::boLine(W, 1, "v"), Q), "[logic] fired on its own candle but t != t0 refused"); }
    { bba::BoDec W = D; W.fired = false; W.reason = bba::BO_R_WAVE_ENDED; W.wick = 0; CHECK(bba::parseBoLine(bba::boLine(W, 1, "v"), Q) && Q.reason == bba::BO_R_WAVE_ENDED && !Q.fired, "[logic] wick_wave_ended round trip"); }
    CHECK(!bba::parseBoLine("BBO|1792575180|1|1|fired|105|2|428|412|16|8|150|100|60|424|40|10|1|1|1.22.0", Q), "[logic] a 1.22 (20-field) line is refused: the old rule's letters are not redrawn");
    const std::string det = bba::boDetail(D, 0.25, "8:36 AM");
    CHECK(det.find("confirmed 1 candle after the wick break by a green close above the pivot in the same up-wave") != std::string::npos && det.find("context - a confirmed break up; don't fade it") != std::string::npos
          && det.find("Not an entry: it continued about 49%") != std::string::npos, "[logic] the details say it honestly: context, don't fade it, ~49% like any candle of the wave");
}

// ---- [nq] + [host] helpers (as test_bba121) ----------------------------------------------------------------------------------
struct Fx { std::string end; double o, h, l, c; long long v, bl; double bp; long long sl; double sp; };
static std::vector<Fx> fixture() {
    std::vector<Fx> v; std::ifstream f("nq_2026-10-21_3m.txt"); std::string ln; std::getline(f, ln);
    while (std::getline(f, ln)) { Fx x; char e[32];
        if (std::sscanf(ln.c_str(), "%19[^|]|%lf|%lf|%lf|%lf|%lld|%lld|%lf|%lld|%lf", e, &x.o, &x.h, &x.l, &x.c, &x.v, &x.bl, &x.bp, &x.sl, &x.sp) == 10) { x.end = e; v.push_back(x); } }
    return v;
}
static unsigned long stampOf(const std::string& e) {
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
        float lo = 1e9f; for (int k = v0; k <= v1 && k < (int)H.bars.size(); ++k) lo = std::min(lo, H.bars[(size_t)k].low); H.priceBase = lo - 10;
        pump(2, true); }
    BBState& st() { return *x->testState(); }
};
static std::string HOME;
static void home(const std::string& name, const ss::Values* set = nullptr) {
    const std::string root = "/tmp/bba123-run", h = root + "/" + name; std::string c = "rm -rf '" + h + "'"; if (std::system(c.c_str()) != 0) {}
    const std::string parts[] = { root, h, h + "/InvestorRT", h + "/InvestorRT/rtx", h + "/InvestorRT/rtx/lsFlexLevels" };
    for (const std::string& p : parts) mkdir(p.c_str(), 0700);
    setenv("USERPROFILE", h.c_str(), 1); HOME = h;
    if (set) { std::ofstream f((h + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.settings2-NQ.txt").c_str()); f << ss::format(bbaset::spec(), *set); }
}
static std::string slurp(const std::string& p) { std::ifstream f(p.c_str(), std::ios::binary); std::ostringstream o; o << f.rdbuf(); return o.str(); }
static std::string recPath(Chart& c) { return HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk/NQ-NQZ6-180-breakouts-" + std::to_string(c.st().session) + ".csv"; }
static int barX(const mock::Host& H, int i) { return 20 + (i - H.barOffset) * H.barW + H.barW / 4; }
static const mock::Draw* textAt(Chart& c, const std::string& t, COLOR col, int i = -1) {
    for (const auto& d : c.H.draws) if (d.type == "ctext" && d.text == t && d.color == col && (i < 0 || std::abs((d.l + d.r) / 2 - barX(c.H, i)) <= 2)) return &d;
    return nullptr; }
// the fired decisions as "t0>t dir" strings, and the drawn letters in a view
static long long SINCE = 0;   // compare from 8:30 AM: one pass decides the last 300 candles, the restarts started earlier (overnight)
static std::set<std::string> firedSet(Chart& c) { std::set<std::string> o; for (const auto& kv : c.st().boDec) if (kv.second.fired && kv.second.t >= SINCE) o.insert(std::to_string(kv.second.t0) + ">" + std::to_string(kv.second.t) + (kv.second.dir > 0 ? " BO" : " BD")); return o; }
static std::set<std::string> lines(const std::string& rec) { std::set<std::string> o; std::istringstream in(rec); std::string ln; while (std::getline(in, ln)) { bba::BoDec D; if (bba::parseBoLine(ln, D) && D.t >= SINCE) o.insert(ln); } return o; }

static void nq(const std::vector<Fx>& F) {
    const int n = (int)F.size(); Syn s; for (const Fx& x : F) add(s, x.o, x.h, x.l, x.c, x.v);
    std::vector<signed char> zd; bba::zigDir(s.h, s.l, n, zd);
    const bba::BoRun R = run(s);
    int fired = 0, ok = 0, fb = 0;
    for (const auto& E : R.ev) {
        if (E.reason == bba::BO_R_WAVE_ENDED) ++fb;
        if (!E.fired) continue;
        ++fired;
        const Fx& m = F[(size_t)E.im]; const bool green = m.c > m.o, red = m.c < m.o;
        bool sameWave = true; for (int k = E.i0 + 1; k <= E.im; ++k) if (zd[(size_t)k] != E.dir) sameWave = false;
        if ((E.dir > 0 ? green && m.c > E.piv : red && m.c < E.piv) && E.im >= E.i0 && sameWave) ++ok;
        else CHECK(false, "[nq] fired mark at " + m.end + " breaks a rule");
    }
    CHECK(fired > 0 && ok == fired, "[nq] every fired BO is a green candle closing above its pivot, every BD a red one closing below, in the breakout's wave (" + std::to_string(ok) + " / " + std::to_string(fired) + ")");
    CHECK(fb > 0, "[nq] failed wick breaks are logged (" + std::to_string(fb) + ")");
    std::cout << "  NQ 2026-10-21 full fixture: " << fired << " marks, " << fb << " failed wick breaks logged\n";
}

static void host(const std::vector<Fx>& F) {
    const int i833 = idxOf(F, "2026-10-21 08:33"), i842 = idxOf(F, "2026-10-21 08:42"), i845 = idxOf(F, "2026-10-21 08:45"), i1121 = idxOf(F, "2026-10-21 11:21"), i1127 = idxOf(F, "2026-10-21 11:27"), i1200 = idxOf(F, "2026-10-21 12:00");
    ss::Values V0 = ss::defaults(bbaset::spec(), ss::M_NQ); V0.v[bbaset::K_SKIP_FIRST] = 0; V0.v[bbaset::K_SKIP_LAST] = 0;
    std::set<std::string> straightF, straightL; SINCE = (long long)stampOf("2026-10-21 08:30");
    {   home("straight", &V0);
        Chart c(nqBars(F, (size_t)i1200 + 1)); c.pump(14, false);
        straightF = firedSet(c); straightL = lines(slurp(recPath(c)));
        c.view(i833 - 2, i845 + 4);
        CHECK(!textAt(c, "BO", 0x0022C55E, i833), "[host] 8:33 red candle: no BO (failed wick break, logged only)");
        CHECK(!textAt(c, "BD", 0x00EF4444, i842) && textAt(c, "BD", 0x00EF4444, i845), "[host] 8:42 wick break: no letter on the wick candle, BD on the 8:45 red close below the pivot");
        int fb = 0; for (const auto& d : c.H.draws) if (d.type == "ctext" && d.text == "FB") ++fb;
        CHECK(fb == 0, "[host] no FB mark drawn");
        c.view(i1121 - 2, i1127 + 3);
        CHECK(!textAt(c, "BD", 0x00EF4444, i1121) && textAt(c, "BD", 0x00EF4444, i1127), "[host] 11:21 wick break down confirmed at 11:27: the letter on 11:27");
        const std::string rec = slurp(recPath(c));
        CHECK(rec.find("|" + std::to_string(stampOf("2026-10-21 08:33")) + "|1|0|wick_wave_ended|") != std::string::npos, "[host] the failed wick break is in the record (wick_wave_ended)");
        const std::string stt = slurp(HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk.status-NQ-NQZ6-180.txt");
        CHECK(stt.find("VERSION,1.23.0\n") == 0 && stt.find("\nBREAKOUT_RULES,1.23.0 BO only on a green candle") != std::string::npos && stt.find("failed wick breaks (wave ended first - logged only) ") != std::string::npos, "[host] status: version + the rules line + the counts");
    }
    {   // no repaint across a restart: a restart after EVERY new candle 8:30 - 12:00 = the same record and the same marks as one pass
        home("restarts", &V0);
        for (int k = i833 - 1; k <= i1200; ++k) { Chart c(nqBars(F, (size_t)k + 1)); c.pump(3, false); }
        Chart c(nqBars(F, (size_t)i1200 + 1)); c.pump(3, false);
        const std::set<std::string> rf = firedSet(c), rl = lines(slurp(recPath(c)));
        CHECK(rf == straightF && !rf.empty(), "[host] restart after every candle: the same marks as one straight pass (" + std::to_string(rf.size()) + " / " + std::to_string(straightF.size()) + ")");
        CHECK(rl == straightL, "[host] restart after every candle: the same record lines (" + std::to_string(rl.size()) + " / " + std::to_string(straightL.size()) + ")");
        std::map<long long, int> per; for (const std::string& l : rl) { bba::BoDec D; bba::parseBoLine(l, D); ++per[D.t0]; }
        int dup = 0; for (const auto& kv : per) if (kv.second > 1) ++dup; CHECK(dup == 0, "[host] each breakout candle recorded once");
    }
    {   // a restart reloads the record: a decision is never re-judged, even when the chart's bars change under it
        home("reload", &V0);
        { Chart c(nqBars(F, (size_t)i1200 + 1)); c.pump(14, false); }
        std::vector<mock::Bar> b = nqBars(F, (size_t)i1200 + 1); b[(size_t)i845].close = b[(size_t)i845].open + 5;   // 8:45 now green: a recompute would not mark it
        Chart c(b); c.pump(8, false); c.view(i833 - 2, i845 + 4);
        CHECK(textAt(c, "BD", 0x00EF4444, i845) && firedSet(c) == straightF, "[host] restart with changed bars: the recorded 8:45 BD stays (never repainted)");
    }
    {   // old 1.22 record lines (the old rule) are ignored: not drawn, counted on the status file
        home("legacy", &V0);
        std::string path; { Chart c(nqBars(F, (size_t)i833)); c.pump(1, false); path = recPath(c); }
        mkdir((HOME + "/InvestorRT/rtx/lsFlexLevels/BigBidAsk").c_str(), 0700);
        { std::ofstream o(path.c_str(), std::ios::binary); o << "BBO|t|dir|fired|reason|pivot|which|high_key|low_key|range|prior_range|vol|prior_vol|lots|key|prior_big|n|print|rth|version\n"
            << "BBO|" << stampOf("2026-10-21 08:33") << "|1|1|fired|29903|1|119620|119400|220|100|5000|3000|80|119567|50|50|1|1|1.22.0\n"; }
        Chart c(nqBars(F, (size_t)i1200 + 1)); c.pump(14, false); c.view(i833 - 2, i845 + 4);
        CHECK(!textAt(c, "BO", 0x0022C55E, i833) && c.st().boLegacy == 1, "[host] a 1.22 'BO fired' line at 8:33 is ignored (not drawn), counted once");
        CHECK(firedSet(c) == straightF, "[host] ... and the 1.23 decisions are made as usual");
    }
}

int main() {
    setenv("TZ", "UTC", 1); tzset();
    logic();
    const std::vector<Fx> F = fixture();
    CHECK(F.size() == 460, "[nq] fixture: 460 candles");
    nq(F);
    host(F);
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
