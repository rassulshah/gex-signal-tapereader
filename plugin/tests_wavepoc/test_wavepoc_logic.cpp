// test_wavepoc_logic.cpp - lsWavePOC 1.3.3 logic suite (1.0.0 origin) (WavePOCLogic.h, no SDK).
//   (in plugin\tests_wavepoc) g++ -std=c++17 -Wall -Wextra -Werror -I.. test_wavepoc_logic.cpp -o t && ./t wavepoc_es1022.csv wavepoc_es1022_liq.csv wavepoc_es1028_bars.csv
// Fixture wavepoc_es1022.csv: ES 2026-10-22 11:00-15:00 CT, 3-min bars + volume at price (Databento trades) - the data of the approved
// mockup (wavepoc_v3, reference wp2.py / wp3.py). The reference used the DAY's average wave (down 23.625 pts, up 21.5 pts); here the
// rolling average is seeded with 20 earlier waves of exactly those sizes, so the no-look-ahead rule picks the same two waves.
#include "WavePOCLogic.h"
#include <fstream>
#include <iostream>
#include <map>

static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

struct Fix { std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow> > V; };
static Fix load(const char* path, double tick) {
    Fix F; std::ifstream f(path); std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::vector<std::string> x = wpc::splitCsv(line);
        if (x[0] == "B") { wpc::Bar b; b.t = std::atoll(x[2].c_str()); wpc::toTicks(std::atof(x[3].c_str()), tick, b.o); wpc::toTicks(std::atof(x[4].c_str()), tick, b.h);
            wpc::toTicks(std::atof(x[5].c_str()), tick, b.l); wpc::toTicks(std::atof(x[6].c_str()), tick, b.c); F.B.push_back(b); F.V.resize(F.B.size()); }
        else if (x[0] == "V") { const int i = std::atoi(x[1].c_str()); wpc::VRow r; wpc::toTicks(std::atof(x[2].c_str()), tick, r.tick); r.vol = std::atoll(x[3].c_str()); F.V[(size_t)i].push_back(r); }
    }
    return F;
}
static void seed(wpc::Store& S, long long before, long long dnTicks2x, long long upTicks2x) {
    // 20 earlier candidates per direction; sizes alternate so the mean is exactly dnTicks2x / 2 (23.625 pts = 94.5 ticks)
    for (int k = 0; k < 20; ++k) {
        S.add(-1, before - 100000 + k * 1000, k % 2 ? dnTicks2x / 2 : dnTicks2x - dnTicks2x / 2);
        S.add(+1, before - 100000 + k * 1000 + 500, k % 2 ? upTicks2x / 2 : upTicks2x - upTicks2x / 2);
    }
}
static void run(wpc::Engine& E, const Fix& F, int upto, wpc::VapFn& vap) { for (int i = 0; i <= upto && i < (int)F.B.size(); ++i) E.addBar(F.B[(size_t)i], vap); }
static std::string px(long long tk) { return wpc::pxText(tk, 0.25); }

static void testReference(const Fix& F) {
    wpc::Store S; seed(S, F.B[0].t, 189, 172);
    int nu = 0; CHECK(std::fabs(S.avgBefore(-1, F.B[0].t, nu) - 94.5) < 1e-9 && nu == 20, "seeded down average = 23.625 pts");
    wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25;
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = F.V[(size_t)i]; return 1; };
    run(E, F, (int)F.B.size() - 1, vap);
    // (1.3.3) 14-bar extreme + (AVG or 3 ATR): 5 waves (was 3) - + 12:12-12:36 up (19.00 pts < the 21.25 up average but > 3 ATR 17.30 = ATR)
    // and + 13:06-13:24 up (a 14-bar high, not a 20-bar one; 24.25 > 21.00 avg and > 3 ATR 16.98 = BOTH); the three 1.3.2 waves stay (BOTH)
    CHECK(E.waves.size() == 5, "(1.3.3) five qualifying waves on Oct 22 (got " << E.waves.size() << ")");
    if (E.waves.size() != 5) return;
    CHECK(E.waves[4].dir == 1 && E.waves[4].i0 == 60 && E.waves[4].i1 == 69 && E.waves[4].conf == 71, "(1.3.2) + the 14:00-14:27 up wave (IRT pivots: its start low is no longer equalled later)");
    CHECK(E.waves[1].dir == 1 && E.waves[1].i0 == 23 && E.waves[1].i1 == 31 && E.waves[1].conf == 35 && E.waves[1].size == 76 && E.waves[1].qWhy == wpc::Q_ATR
          && E.waves[1].size < E.waves[1].avg && E.waves[1].size > 3 * E.waves[1].atr, "(1.3.3) 12:12-12:36 up wave: smaller than the average, bigger than 3 ATR -> qualifies (ATR)");
    CHECK(E.waves[3].dir == 1 && E.waves[3].i0 == 41 && E.waves[3].i1 == 47 && E.waves[3].qWhy == 3 && E.is20(47, 1) && [&] { for (int k = 47 - 19; k < 47; ++k) if (E.B[(size_t)k].h > E.B[47].h) return true; return false; }(), "(1.3.3) 13:06-13:24 up wave: a 14-bar high but not a 20-bar high -> qualifies (BOTH)");
    CHECK(E.waves[0].qWhy == 3 && E.waves[2].qWhy == 3 && E.waves[4].qWhy == 3, "(1.3.3) the three 1.3.2 waves qualify by BOTH");
    CHECK(E.C.qBy[wpc::Q_AVG] == 0 && E.C.qBy[wpc::Q_ATR] == 1 && E.C.qBy[3] == 4, "(1.3.3) counted by AVG 0 / ATR 1 / BOTH 4");
    const wpc::Wave& a = E.waves[0]; const wpc::Wave& b = E.waves[2];
    CHECK(a.i0 == 6 && a.i1 == 16 && a.dir == -1 && a.conf == 18, "wave 1 = 11:18-11:48 sell wave, confirmed when a high beats the previous 3 highs (IRT)");
    CHECK(b.i0 == 31 && b.i1 == 41 && b.dir == -1 && b.conf == 43, "wave 2 = 12:33-13:03 sell wave (confirmed 13:09, IRT rule)");
    CHECK(a.size == 150 && b.size == 106, "sizes 37.50 / 26.50 pts");
    CHECK(a.rowTicks == 1 && b.rowTicks == 1 && a.rows.size() == 151 && b.rows.size() == 107, "(1.3.2) row size rule: 1-tick rows in every profile, 151 / 107 rows (" << a.rows.size() << "/" << b.rows.size() << ")");
    // (1.3.2) 1-tick rows, node = the max raw row of its distribution: the same places as the approved 2-tick nodes (inside those rows) + 7693.25
    const char* ea[5] = { "7719.00", "7697.75", "7690.50", "7707.00", "7693.25" }; const double ra[5] = { 5.5, 3.5, 3.0, 2.7, 2.2 };
    CHECK(a.nodes.size() == 5, "wave 1 has 5 nodes (" << a.nodes.size() << ")");
    for (size_t k = 0; k < a.nodes.size() && k < 5; ++k) {
        CHECK(px(a.nodes[k].tick) == ea[k], "wave 1 node " << k << " = " << ea[k] << " (got " << px(a.nodes[k].tick) << ")");
        CHECK(std::fabs(std::round(a.nodes[k].ratio * 10) / 10 - ra[k]) < 1e-9, "wave 1 node " << k << " ratio " << ra[k] << " (got " << a.nodes[k].ratio << ")");
    }
    const char* eb[3] = { "7700.00", "7693.00", "7681.00" }; const double rb[3] = { 7.5, 3.2, 2.4 };
    CHECK(b.nodes.size() == 3, "wave 2 has 3 nodes");
    for (size_t k = 0; k < b.nodes.size() && k < 3; ++k) {
        CHECK(px(b.nodes[k].tick) == eb[k], "wave 2 node " << k << " = " << eb[k] << " (got " << px(b.nodes[k].tick) << ")");
        CHECK(std::fabs(std::round(b.nodes[k].ratio * 10) / 10 - rb[k]) < 1e-9, "wave 2 node " << k << " ratio " << rb[k]);
    }
    CHECK(wpc::tagText(b.nodes[0], 0.25) == "WPOC 7700.00 7.5x" && wpc::tagText(b.nodes[1], 0.25) == "7693.00 3.2x", "tag text (approved format)");
    // the stop rules, as wp3.py on the same day
    // (1.3.3) wave 1's two open nodes now end at bar 35 (the new 12:12-12:36 ATR wave confirms; was the 13:09 wave at 43)
    const int sa[5][2] = { { wpc::NEWWAVE, 35 }, { wpc::TOUCH, 18 }, { wpc::TOUCH, 17 }, { wpc::NEWWAVE, 35 }, { wpc::TOUCH, 17 } };
    for (size_t k = 0; k < 5 && k < a.nodes.size(); ++k) CHECK(a.nodes[k].stop == sa[k][0] && a.nodes[k].stopBar == sa[k][1], "wave 1 node " << k << " stop " << wpc::stopName(sa[k][0]) << " at bar " << sa[k][1] << " (got " << wpc::stopName(a.nodes[k].stop) << " " << a.nodes[k].stopBar << ")");
    // (1.3.3) node 7681.00 ends at bar 49 (the 13:06-13:24 up wave now qualifies and confirms; was touched at 60)
    const int sb[3][2] = { { wpc::TOUCH, 46 }, { wpc::TOUCH, 43 }, { wpc::NEWWAVE, 49 } };
    for (size_t k = 0; k < 3 && k < b.nodes.size(); ++k) CHECK(b.nodes[k].stop == sb[k][0] && b.nodes[k].stopBar == sb[k][1], "wave 2 node " << k << " stop at bar " << sb[k][1]);
    CHECK(E.C.qual[0] == 3 && E.C.qual[1] == 2, "(1.3.3) three up waves, two down waves qualify");
    // the record lines (one WAVE + its NODEs per wave, one STOP per node)
    int nw = 0, nn = 0, ns = 0; for (const std::string& l : E.out) { nw += l.rfind("WAVE,", 0) == 0; nn += l.rfind("NODE,", 0) == 0; ns += l.rfind("STOP,", 0) == 0; }
    int nr = 0; for (const std::string& l : E.out) nr += l.rfind("ROWS,", 0) == 0;
    CHECK(nr == 5, "(1.3.2) one ROWS line per wave");
    CHECK(nw == 5 && nn == 16 && ns == 16, "record: 5 WAVE, 16 NODE, 16 STOP lines (" << nw << "/" << nn << "/" << ns << ")");
    {   bool q23 = true; std::string w2; for (const std::string& l : E.out) if (l.rfind("WAVE,", 0) == 0) { const auto f = wpc::splitCsv(l); q23 = q23 && f.size() == 23; if (f[4] == std::to_string(E.waves[1].t0)) w2 = f[21] + "/" + f[22]; }
        CHECK(q23 && w2 == "ATR/5.7679", "(1.3.3) WAVE lines: 23 fields, + qualifiedBy and atr14 (12:12 wave: " << w2 << ")"); }
}

// bar by bar: a confirmed wave never changes (nodes, ranks) and a stop once set never changes; a restart after every bar (fresh engine,
// the store + record as written so far) rebuilds exactly the same waves
static void testNoRepaint(const Fix& F) {
    wpc::Store S; seed(S, F.B[0].t, 189, 172);
    wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25;
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = F.V[(size_t)i]; return 1; };
    std::vector<std::string> recLines; std::vector<wpc::Wave> seen; bool stable = true, restartSame = true;
    for (int k = 0; k < (int)F.B.size(); ++k) {
        E.addBar(F.B[(size_t)k], vap);
        for (const std::string& l : E.out) recLines.push_back(l);
        E.out.clear();
        for (size_t w = 0; w < seen.size(); ++w) {
            const wpc::Wave& a = seen[w]; const wpc::Wave& b = E.waves[w];
            if (a.i0 != b.i0 || a.i1 != b.i1 || a.nodes.size() != b.nodes.size()) stable = false;
            for (size_t n = 0; n < a.nodes.size() && n < b.nodes.size(); ++n) {
                if (a.nodes[n].tick != b.nodes[n].tick || a.nodes[n].rank != b.nodes[n].rank) stable = false;
                if (a.nodes[n].stop != wpc::OPEN && (a.nodes[n].stop != b.nodes[n].stop || a.nodes[n].stopBar != b.nodes[n].stopBar)) stable = false;
            }
        }
        seen = E.waves;
        // restart now: a new engine, the store file as saved, the record as appended
        wpc::Store S2; std::string why;
        const bool ok = wpc::storeFromText(S.toText("ES", 180), "ES", 180, 3, 250000000LL, S2, why);
        if (!ok) { S2 = S; }
        wpc::Engine R; R.store = &S2; R.N = 3; R.tick = 0.25; int bad = 0; R.rec = wpc::parseRecord(recLines, 3, 0.25, bad);
        run(R, F, k, vap);
        if (R.waves.size() != E.waves.size()) restartSame = false;
        for (size_t w = 0; w < R.waves.size() && w < E.waves.size(); ++w) {
            const wpc::Wave& a = E.waves[w]; const wpc::Wave& b = R.waves[w];
            if (a.i0 != b.i0 || a.i1 != b.i1 || a.nodes.size() != b.nodes.size()) restartSame = false;
            for (size_t n = 0; n < a.nodes.size() && n < b.nodes.size(); ++n)
                if (a.nodes[n].tick != b.nodes[n].tick || a.nodes[n].stop != b.nodes[n].stop || a.nodes[n].stopBar != b.nodes[n].stopBar) restartSame = false;
        }
    }
    CHECK(stable, "no repaint: confirmed waves / nodes / stops never change bar by bar");
    CHECK(restartSame, "a restart after every bar rebuilds byte-identical waves, nodes and stops");
    // the record parses back to the same nodes
    int bad = 0; const auto rec = wpc::parseRecord(recLines, 3, 0.25, bad);
    CHECK(bad == 0 && rec.size() == 5, "record parses back (5 waves, 0 bad lines)");
    const auto it = rec.find(std::make_pair(E.waves[2].t0, E.waves[2].t1));
    CHECK(it != rec.end() && it->second.nodes.size() == 3 && it->second.nodes[0].tick == E.waves[2].nodes[0].tick, "recorded WPOC of the 12:36 sell wave = 7700.00");
    { const auto i1 = rec.find(std::make_pair(E.waves[1].t0, E.waves[1].t1)); CHECK(i1 != rec.end() && i1->second.qWhy == wpc::Q_ATR, "(1.3.3) record parses qualifiedBy ATR back"); }
    // a recorded wave is drawn as recorded even when the average (store) would now refuse it
    wpc::Store big; seed(big, F.B[0].t, 2000, 2000);
    wpc::Engine R; R.store = &big; R.N = 3; R.tick = 0.25; R.rec = rec; run(R, F, (int)F.B.size() - 1, vap);
    CHECK(R.waves.size() == 5 && R.C.recordUsed == 5 && R.waves[0].fromRecord, "recorded waves are redrawn from the record (never vanish)");
    // a different N ignores the record
    int bad2 = 0; CHECK(wpc::parseRecord(recLines, 5, 0.25, bad2).empty(), "the record of N=3 is not used at N=5");
}

static void testN5(const Fix& F) {
    wpc::Store S; seed(S, F.B[0].t, 189, 172);
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = F.V[(size_t)i]; return 1; };
    wpc::Engine A; A.store = &S; A.N = 3; run(A, F, (int)F.B.size() - 1, vap);
    wpc::Store S5; seed(S5, F.B[0].t, 189, 172);
    wpc::Engine Bn; Bn.store = &S5; Bn.N = 5; run(Bn, F, (int)F.B.size() - 1, vap);
    bool differ = A.alt.size() != Bn.alt.size();
    for (size_t k = 0; !differ && k < A.alt.size(); ++k) differ = A.alt[k].i != Bn.alt[k].i;
    (void)differ;
    // (1.3.2) IRT's rule for N: each leg is confirmed by the first bar beyond the previous N bars' opposite extreme
    for (const wpc::Engine* G : { &A, &Bn }) for (const wpc::Wave& w : G->waves) {
        const int c = w.conf, n = G->N; bool ok = c >= n && c > w.i1;
        long long ph = LLONG_MIN, pl = LLONG_MAX; for (int j = c - n; j < c && ok; ++j) { ph = std::max(ph, F.B[(size_t)j].h); pl = std::min(pl, F.B[(size_t)j].l); }
        if (ok) ok = w.dir < 0 ? F.B[(size_t)c].h > ph : F.B[(size_t)c].l < pl;
        CHECK(ok, "(1.3.2) N=" << n << ": wave " << w.i0 << "-" << w.i1 << " confirmed at " << c << " by a bar beyond the previous N bars (IRT ZIGZAG bars)");
    }
    CHECK(wpc::clampN(1) == 3 && wpc::clampN(11) == 3 && wpc::clampN(2) == 2 && wpc::clampN(10) == 10, "N outside 2-10 = default 3");
}

// synthetic: rise to a swing high, fall (the provisional wave follows every new low), turn up (confirmed), then gap above the origin
static void testProvisionalAndOrigin() {
    std::vector<wpc::Bar> B; long long t = 1000000;
    auto add = [&](long long h, long long l, long long c) { wpc::Bar b; b.t = (t += 180); b.h = h; b.l = l; b.o = (h + l) / 2; b.c = c; B.push_back(b); };
    for (int i = 0; i < 10; ++i) add(100 + 2 * i, 97 + 2 * i, 99 + 2 * i);
    add(125, 120, 121);                                  // bar 10 = the swing high (origin 125)
    for (int j = 0; j < 14; ++j) add(122 - 2 * j, 119 - 2 * j, 119 - 2 * j);   // bars 11..24 fall, low 119 .. 93
    for (int j = 0; j < 3; ++j) add(98 + j, 95 + j, 97 + j);                   // bars 25..27 turn up: bar 24 low 93 = swing low, confirmed at 27
    add(150, 126, 148);                                  // bar 28: a gap above every node, closes above the origin (125)
    std::vector<std::vector<wpc::VRow> > V(B.size());
    for (size_t i = 0; i < B.size(); ++i) { for (long long p = B[i].l; p <= B[i].h; ++p) V[i].push_back({ p, 10 }); }
    V[14].push_back({ 112, 400 }); V[19].push_back({ 101, 250 });             // two volume nodes inside the down wave
    wpc::Store S; for (int k = 0; k < 6; ++k) { S.add(-1, 1000 + k, 8); S.add(1, 2000 + k, 8); }
    wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25;
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = V[(size_t)i]; return 1; };
    std::vector<int> provEnd; wpc::Wave last;
    for (int k = 0; k <= 25; ++k) { E.addBar(B[(size_t)k], vap); wpc::Wave p; if (E.provisional(p, vap)) { provEnd.push_back(p.i1); last = p; } else provEnd.push_back(-1); }
    CHECK(provEnd[15] == -1, "no provisional wave before the extreme is a 20-bar low");
    CHECK(provEnd[21] == 21 && provEnd[22] == 22 && provEnd[24] == 24, "the provisional wave moves to every new extreme (21, 22 .. 24)");
    CHECK(provEnd[25] == 24 && last.provisional && !last.nodes.empty(), "after the turn the provisional wave stays at the extreme (bar 24), dashed");
    // (1.3.2) IRT's zigzag starts at bar 0's low, so the 0 -> 10 rise is an up wave (confirmed at 13); the down wave is what this test follows
    auto downs = [&]() { std::vector<const wpc::Wave*> d; for (const wpc::Wave& w : E.waves) if (w.dir < 0) d.push_back(&w); return d; };
    CHECK(downs().empty(), "(1.3.2) nothing confirmed before IRT's reversal bar");
    E.addBar(B[26], vap); wpc::Wave p;
    CHECK(!E.provisional(p, vap), "confirmed: the provisional copy is gone");
    CHECK(downs().size() == 1 && downs()[0]->i0 == 10 && downs()[0]->i1 == 24 && downs()[0]->conf == 26, "(1.3.2) the wave confirmed at bar 26 (its high 99 beats the previous 3 highs 98) (10 -> 24)");
    if (downs().size() != 1) return;
    wpc::Wave& DW = const_cast<wpc::Wave&>(*downs()[0]);
    bool same = DW.nodes.size() == last.nodes.size();
    for (size_t k = 0; same && k < last.nodes.size(); ++k) same = last.nodes[k].tick == DW.nodes[k].tick;
    CHECK(same, "the confirmed nodes = the provisional nodes (only the line style changes)");
    CHECK(px(DW.nodes[0].tick) == wpc::pxText(112, 0.25), "WPOC = the 400-lot row (" << px(DW.nodes[0].tick) << ")");
    E.addBar(B[27], vap); E.addBar(B[28], vap);
    bool allOrigin = true; for (const wpc::Node& n : DW.nodes) allOrigin = allOrigin && (n.stop == wpc::ORIGIN && n.stopBar == 28);
    CHECK(allOrigin, "a close above the origin (125) stops every line not touched (origin close)");
    // VAP missing: the wave waits (pending), nothing drawn, retried later; the stops are then replayed from its end
    wpc::Store S2; for (int k = 0; k < 6; ++k) S2.add(-1, 1000 + k, 8);
    wpc::Engine M; M.store = &S2; M.N = 3; M.tick = 0.25; bool have = false;
    wpc::VapFn vap2 = [&](int i, std::vector<wpc::VRow>& o) { if (!have && i == 17) return 0; o = V[(size_t)i]; return 1; };
    for (int k = 0; k <= 28; ++k) M.addBar(B[(size_t)k], vap2);
    CHECK(M.waves.size() == 1 && M.waves[0].pending && M.waves[0].nodes.empty() && M.pendingCount() == 1, "VAP missing for one bar: the wave is pending, not drawn");
    have = true; CHECK(M.retryPending(vap2) == 1 && !M.waves[0].pending && M.waves[0].nodes.size() == DW.nodes.size(), "retry fills it in");
    bool st = true; for (const wpc::Node& n : M.waves[0].nodes) st = st && n.stop == wpc::ORIGIN && n.stopBar == 28;
    CHECK(st, "the stops are replayed after a retry");
}

static void testStoreAndHelpers() {
    wpc::Store S; S.n = 3; S.tickKey = 250000000LL; S.add(1, 100, 10); S.add(1, 200, 20); S.add(-1, 150, 30); S.add(1, 200, 20);
    CHECK(S.up.size() == 2, "dedupe by end time");
    int n = 0; CHECK(std::fabs(S.avgBefore(1, 200, n) - 10) < 1e-9 && n == 1, "average uses only earlier waves (no look-ahead)");
    wpc::Store R; std::string why;
    CHECK(wpc::storeFromText(S.toText("ES", 180), "ES", 180, 3, 250000000LL, R, why) && R.up.size() == 2 && R.dn.size() == 1, "store round trip");
    CHECK(!wpc::storeFromText(S.toText("ES", 180), "NQ", 180, 3, 250000000LL, R, why), "other market refused");
    CHECK(!wpc::storeFromText(S.toText("ES", 180), "ES", 60, 3, 250000000LL, R, why), "other bar size refused");
    CHECK(!wpc::storeFromText(S.toText("ES", 180), "ES", 180, 5, 250000000LL, R, why), "other N refused (rebuilt from the chart)");
    std::string torn = S.toText("ES", 180); torn = torn.substr(0, torn.size() - 8);
    CHECK(!wpc::storeFromText(torn, "ES", 180, 3, 250000000LL, R, why), "torn file refused (" << why << ")");
    std::string garb = S.toText("ES", 180); garb.replace(garb.find("U,100,10"), 8, "U,1x0,10");
    CHECK(!wpc::storeFromText(garb, "ES", 180, 3, 250000000LL, R, why), "garbled line refused");
    CHECK(!wpc::storeFromText("", "ES", 180, 3, 250000000LL, R, why), "empty refused");
    for (int k = 0; k < 1500; ++k) S.add(-1, 1000 + k, 5);
    CHECK((int)S.dn.size() == wpc::STORE_CAP, "store capped at 1000 per direction");
    CHECK(wpc::rowTicksFor(150) == 1 && wpc::rowTicksFor(400) == 1 && wpc::rowTicksFor(401) == 2 && wpc::rowTicksFor(800) == 2, "(1.3.2) row size rule: 1 tick up to 400 rows");
    CHECK(wpc::decimalsFor(0.25) == 2 && wpc::decimalsFor(0.1) == 1 && wpc::decimalsFor(0.0005) == 4 && wpc::decimalsFor(0.001) == 3 && wpc::decimalsFor(0.00005) == 5, "price decimals per market");
    CHECK(wpc::pxText(30801, 0.25) == "7700.25" && wpc::pxText(23456, 0.00005) == "1.17280", "price text");
    CHECK(wpc::hm(20383LL * 86400 + 13 * 3600 + 12 * 60) == "2025-10-22 13:12", "hm() civil date");
    std::vector<long long> rows = { 0, 1, 5, 1, 0, 0, 1, 1, 9, 1, 0 };
    CHECK(wpc::median2(rows) == 2, "median of non-zero rows (1,1,1,1,1,5,9 -> 1) x 2");
    const std::vector<int> pk = wpc::pickNodes(rows, wpc::median2(rows));
    CHECK(pk.size() == 2 && pk[0] == 8 && pk[1] == 2, "two separate nodes, biggest first");
    std::vector<long long> twin = { 1, 1, 9, 9, 9, 1, 1, 1 };   // one wide node: a second peak inside it is not separate
    CHECK(wpc::pickNodes(twin, wpc::median2(twin)).size() == 1, "nodes in one mini-distribution count once");
    std::vector<long long> many; for (int k = 0; k < 60; ++k) many.push_back(k % 6 == 3 ? 50 + k : 1);
    CHECK(wpc::pickNodes(many, wpc::median2(many)).size() == 5, "max 5 nodes");
}

static void testLayout(const Fix& F) {
    wpc::Store S; seed(S, F.B[0].t, 189, 172);
    wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25;
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = F.V[(size_t)i]; return 1; };
    run(E, F, 50, vap);    // 13:33: wave 2 lines partly open
    wpc::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 700; v.first = 0; v.last = 51; v.lastBar = 51; v.bw = 1200.0 / 52;
    long long lo = 1LL << 60, hi = -lo; for (int i = 0; i <= 51; ++i) { lo = std::min(lo, F.B[(size_t)i].l); hi = std::max(hi, F.B[(size_t)i].h); }
    v.X = [&](int i) { return 10 + (i + 0.5) * v.bw; };
    v.Y = [&](double tk) { return 690 - (tk - lo + 4) / (double)(hi - lo + 8) * 680; };
    v.textW = [](const std::string& s, bool) { return (int)s.size() * 7; }; v.tick = 0.25;
    const wpc::Scene sc = wpc::layout(E, nullptr, v);
    const double xLast = v.X(51) + v.bw / 2;
    bool tagsRight = true, amberLine3 = false, amberBox2 = false, whiteLine1 = false, noBoxes = true; int texts = 0;
    for (const wpc::Prim& p : sc.p) {
        if (p.k == wpc::Prim::TEXT && p.text.rfind("Wave POC", 0) != 0) { ++texts; if (p.x0 <= xLast) tagsRight = false; }
        if (p.k == wpc::Prim::LINE && p.col == 0xF5B428 && p.w == 3) amberLine3 = true;
        if (p.k == wpc::Prim::FILL && p.col == 0xC88C14) amberBox2 = true;
        if (p.k == wpc::Prim::BOX) noBoxes = false;
        if (p.k == wpc::Prim::LINE && p.col == 0xE5E7EB && p.w == 1) whiteLine1 = true;
    }
    CHECK(texts == 0 && tagsRight, "(1.4.0) no right-edge price tags (the open WPOC is a badge in lsDealerProfile's Levels column)");
    (void)whiteLine1;
    {   // (1.2.0, Rassul 16:26 CT "remove the drawing of levels across the chart and from the wave pocs") no line / circle at all (zigzag off)
        int lines = 0, ovals = 0; for (const wpc::Prim& p : sc.p) { if (p.k == wpc::Prim::LINE && std::min(p.x0, p.x1) < xLast - 1 && !(p.w == 2 && p.x0 == p.x1)) ++lines; /* (1.3.3) not the spines */ if (p.k == wpc::Prim::OVAL) ++ovals; }   // (the tag leader right of the last bar is the tag itself)
        CHECK(!amberLine3 && lines == 0 && ovals == 0 && amberBox2 && noBoxes, "(1.2.0) no node lines / touch circles / end ticks; WPOC = the bright amber row; no outline boxes (" << lines << " lines, " << ovals << " ovals)");
        // tags only for nodes still OPEN (untouched, wave not replaced, origin not broken)
        int open = 0, closed = 0; for (const wpc::Wave& w : E.waves) if (!w.pending) for (const wpc::Node& nd : w.nodes) { if (nd.stop == wpc::OPEN) ++open; else ++closed; }
        CHECK(closed > 0 && sc.tagsShown + sc.tagsHidden == open, "(1.2.0) tags only for OPEN nodes (" << sc.tagsShown + sc.tagsHidden << " tags, " << open << " open, " << closed << " closed)");
        // (1.2.0, Rassul 16:31 CT "the wave poc lines are too large ... 33% less") the profile max width x 0.67
        bool wOk = true; int waves = 0;
        for (const wpc::Wave& w : E.waves) { if (w.pending || w.rows.empty() || w.maxVol <= 0) continue;
            const double x0 = v.X(w.i0) - v.bw * 0.45, PW0 = std::max(4.0, std::min(170.0, (v.X(w.i1) - x0) * 0.95));
            int mx = 0; for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.x0 == (int)std::lround(x0) && (p.col == 0x334155 || p.col == 0xC88C14 || p.col == 0xB4BECD)) mx = std::max(mx, p.x1 - p.x0);
            ++waves; if (std::abs(mx - (int)std::lround(0.67 * PW0)) > 1) wOk = false; }
        CHECK(waves > 0 && wOk && wpc::PROF_SCALE == 0.67, "(1.2.0) wave profile max bar width = 0.67 x the 1.1 width (" << waves << " waves)");
    }
    CHECK(sc.nodesDrawn <= 5 * sc.wavesDrawn && sc.nodesDrawn == 13, "(1.3.3) nodes drawn = 5 + 2 + 3 + 3 (four waves confirmed by 13:33), at most 5 per wave (" << sc.nodesDrawn << ")");
    {   // (1.3.3, Rassul 04:52 CT) the spine: one 2 px vertical line per drawn wave at its profile start x, over start pivot .. end pivot; green up, red down
        int sp = 0; bool ok = true;
        for (const wpc::Wave& w : E.waves) { if (w.pending) continue; const int x = (int)std::lround(v.X(w.i0) - v.bw * 0.45);
            const int ya = (int)std::lround(v.Y((double)std::min(w.p0, w.p1) - 0.5)), yb = (int)std::lround(v.Y((double)std::max(w.p0, w.p1) + 0.5)); int n = 0;
            for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::LINE && p.w == 2 && p.x0 == x && p.x1 == x) { ++n; if (p.col != (w.dir > 0 ? 0x22C55EUL : 0xEF4444UL) || p.y0 != std::min(ya, yb) || p.y1 != std::max(ya, yb)) ok = false; }
            sp += n; if (n != 1) ok = false; }
        CHECK(ok && sp == 4, "(1.3.3) spines: 4 waves, one each, green up / red down, spanning the wave's range (" << sp << ")"); }
    bool header = false; for (const wpc::Prim& p : sc.p) header = header || (p.k == wpc::Prim::TEXT && p.text == std::string("Wave POC v") + wpc::VERSION);
    CHECK(header && std::string(wpc::VERSION) == "1.4.1", "header Wave POC v1.4.1");
    {   // (1.4.0) the open WPOC handed to lsDealerProfile: the latest confirmed wave's node [0] while OPEN, else none
        long long tk = 0; const bool has = wpc::latestOpenPoc(E, tk);
        const wpc::Wave* lw = nullptr; for (const wpc::Wave& w : E.waves) if (!w.pending) lw = &w;
        const bool expect = lw && !lw->nodes.empty() && lw->nodes[0].stop == wpc::OPEN;
        CHECK(has == expect && (!has || tk == lw->nodes[0].tick), "(1.4.0) latestOpenPoc = the latest wave's OPEN WPOC (or none)");
        wpc::Engine E2 = E; if (!E2.waves.empty()) { for (wpc::Wave& w : E2.waves) if (!w.nodes.empty()) w.nodes[0].stop = wpc::OPEN + 1; }
        long long t2 = 0; CHECK(!wpc::latestOpenPoc(E2, t2), "(1.4.0) a closed WPOC is never handed over");
    }
    // no room right of the last bar: no tags at all (counted)
    wpc::View v2 = v; v2.r = (int)xLast + 20; const wpc::Scene s2 = wpc::layout(E, nullptr, v2);
    int t2 = 0; for (const wpc::Prim& p : s2.p) t2 += p.k == wpc::Prim::TEXT && p.text.rfind("Wave POC", 0) != 0;
    CHECK(t2 == 0 && s2.tagsHidden > 0, "tags hidden (not drawn over candles) when there is no room");
    // tags never overlap each other
    std::vector<std::pair<int, int> > ys; for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::TEXT && p.text.rfind("Wave POC", 0) != 0) ys.push_back(std::make_pair(p.y0, p.y1));
    std::sort(ys.begin(), ys.end()); bool noOv = true; for (size_t k = 1; k < ys.size(); ++k) noOv = noOv && ys[k].first >= ys[k - 1].second;
    CHECK(noOv, "tags stacked without overlap");
    // zigzag off by default, on = legs + dots
    int zz = 0; for (const wpc::Prim& p : sc.p) zz += p.col == 0x78849A;
    v.zigzag = true; const wpc::Scene s3 = wpc::layout(E, nullptr, v); int zz3 = 0; for (const wpc::Prim& p : s3.p) zz3 += p.col == 0x78849A || p.col == 0xCBD5E1;
    CHECK(zz == 0 && zz3 > 5, "zigzag only when switched on");
    // the profile sits at the wave start, pointing right, behind the candles
    bool profAtStart = false; for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.col == 0x334155) { profAtStart = profAtStart || (std::abs(p.x0 - (int)std::lround(v.X(6) - v.bw * 0.45)) <= 1 && p.back); }
    CHECK(profAtStart, "profile drawn at the wave start (behind the candles)");
}

// ---- (1.1.0) liquidity nodes ----
// the wave's average resting lots per price and side from the ES Oct 22 MBO snapshots (market makers removed), as wp8.py
static std::map<std::pair<int, int>, std::vector<std::pair<long long, std::pair<double, double> > > > LQ;   // (i0, i1) -> tick, (bid, offer)
static void loadLiq(const char* path) {
    std::ifstream f(path); std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::vector<std::string> x = wpc::splitCsv(line); if (x.size() != 6 || x[0] != "L") continue;
        long long tk = 0; wpc::toTicks(std::atof(x[4].c_str()), 0.25, tk); const double v = std::atof(x[5].c_str());
        auto& vec = LQ[std::make_pair(std::atoi(x[1].c_str()), std::atoi(x[2].c_str()))];
        vec.push_back(std::make_pair(tk, std::make_pair(x[3] == "0" ? v : 0.0, x[3] == "1" ? v : 0.0)));
    }
}
static bool fixLiq(const wpc::Wave& w, std::vector<double>& rows, std::vector<double>& bids) {
    rows.assign(w.rows.size(), 0); bids.assign(w.rows.size(), 0);
    auto it = LQ.find(std::make_pair(w.i0, w.i1)); if (it == LQ.end()) return false;
    for (const auto& e : it->second) { const long long d = e.first - w.e0; if (d < 0) continue; const long long k = d / w.rowTicks; if (k >= (long long)rows.size()) continue;
        rows[(size_t)k] += e.second.first + e.second.second; bids[(size_t)k] += e.second.first; }
    return true;
}
static void testLiquidityReference(const Fix& F) {
    wpc::Store S; seed(S, F.B[0].t, 189, 172);
    wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25; E.liqFn = fixLiq;
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = F.V[(size_t)i]; return 1; };
    run(E, F, (int)F.B.size() - 1, vap);
    CHECK(E.waves.size() == 5, "(1.3.3) five waves with the liquidity function on (the fixture has book data for the 1.3.2 waves only)");
    if (E.waves.size() != 5) return;
    const wpc::Wave& a = E.waves[0]; const wpc::Wave& b = E.waves[2];
    CHECK(a.liqOk && a.lnodes.size() == 4 && b.lnodes.size() == 5, "(1.3.2, 1-tick rows) liquidity nodes (1.9x, raw-row valley): 4 (wave 1) and 5 (wave 2) (" << a.lnodes.size() << "/" << b.lnodes.size() << ")");
    const char* ea[4] = { "7707.25", "7688.00", "7710.00", "7695.00" }; const double ra[4] = { 2.98, 2.72, 2.65, 2.44 };
    for (size_t k = 0; k < 4 && k < a.lnodes.size(); ++k) CHECK(px(a.lnodes[k].tick) == ea[k] && std::fabs(a.lnodes[k].ratio - ra[k]) < 0.006 && a.lnodes[k].side == 1,
        "wave 1 liquidity node " << k << " = " << ea[k] << " " << ra[k] << "x bids (got " << px(a.lnodes[k].tick) << " " << a.lnodes[k].ratio << ")");
    const char* eb[5] = { "7685.00", "7680.00", "7682.25", "7689.00", "7690.00" }; const double rb[5] = { 8.66, 7.20, 4.48, 4.04, 2.52 };
    for (size_t k = 0; k < 5 && k < b.lnodes.size(); ++k) CHECK(px(b.lnodes[k].tick) == eb[k] && std::fabs(b.lnodes[k].ratio - rb[k]) < 0.006 && b.lnodes[k].side == 1,
        "wave 2 liquidity node " << k << " = " << eb[k] << " " << rb[k] << "x bids (got " << px(b.lnodes[k].tick) << " " << b.lnodes[k].ratio << ")");
    CHECK(a.lnodes.size() == 4 && a.lnodes[0].nearVol && !a.lnodes[1].nearVol, "7707.25 sits on the volume node 7707.00 (within 2 ticks), 7688.00 alone");
    CHECK(b.lnodes.size() == 5 && b.lnodes[0].stop == wpc::TOUCH && b.lnodes[0].stopBar > b.i1, "liquidity lines stop by the same rules (LPOC 7685.00 touched later)");
    int nl = 0, ls = 0; for (const std::string& l : E.out) { nl += l.rfind("LNODE,", 0) == 0; ls += l.rfind("LSTOP,", 0) == 0; }
    CHECK(nl == 9 && ls == 9, "record: 9 LNODE lines (+ near / alone) and 9 LSTOP lines (" << nl << "/" << ls << ")");
    bool nearTxt = false; for (const std::string& l : E.out) nearTxt = nearTxt || (l.rfind("LNODE,", 0) == 0 && l.find(",7707.25,") != std::string::npos && l.find("near_volume_node") != std::string::npos);
    CHECK(nearTxt, "LNODE line says near_volume_node");
    // restart without a book: the recorded liquidity nodes come back from the record
    int bad = 0; wpc::Store S2; seed(S2, F.B[0].t, 189, 172);
    wpc::Engine R; R.store = &S2; R.N = 3; R.tick = 0.25; R.rec = wpc::parseRecord(E.out, 3, 0.25, bad); run(R, F, (int)F.B.size() - 1, vap);
    CHECK(bad == 0 && R.waves.size() == 5 && R.waves[2].lnodes.size() == 5 && R.waves[2].lnodes[0].tick == b.lnodes[0].tick && R.waves[2].lnodes[0].stopBar == b.lnodes[0].stopBar, "restart: liquidity nodes redrawn from the record");
    // no book (waves formed before the indicator loaded): no liquidity nodes, no error
    wpc::Store S3; seed(S3, F.B[0].t, 189, 172);
    wpc::Engine Nb; Nb.store = &S3; Nb.N = 3; Nb.liqFn = [](const wpc::Wave&, std::vector<double>&, std::vector<double>&) { return false; }; run(Nb, F, (int)F.B.size() - 1, vap);
    CHECK(Nb.waves.size() == 5 && Nb.waves[0].lnodes.empty() && Nb.C.liqNoBook == 5 && Nb.waves[0].nodes.size() == 5, "no book: no liquidity nodes, the volume nodes unchanged");
    // layout: left of the wave start, <= 60 px, LPOC bright green, others 60%, dotted lines, no tags for liquidity
    wpc::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 700; v.first = 0; v.last = 79; v.lastBar = 79; v.bw = 1200.0 / 80;
    v.X = [&](int i) { return 100 + (i + 0.5) * v.bw; }; v.Y = [&](double tk) { return 690 - (tk * 0.25 - 7672) / 60.0 * 680; };
    v.textW = [](const std::string& s2, bool) { return (int)s2.size() * 7; }; v.tick = 0.25;
    const wpc::Scene sc = wpc::layout(E, nullptr, v);
    bool left = true; int bright = 0, dim = 0, dots = 0, texts = 0;
    for (const wpc::Prim& p : sc.p) {
        if (p.k == wpc::Prim::FILL && (p.col == 0x22C55E || p.col == wpc::mix(0x22C55E, 0x0B0F17, 0.6))) {
            if (p.col == 0x22C55E) ++bright; else ++dim;
            const int e1 = (int)std::lround(v.X(6) - v.bw * 0.45 - 2), e2 = (int)std::lround(v.X(31) - v.bw * 0.45 - 2);
            if (p.x0 >= p.x1 || (std::abs(p.x1 - e1) > 1 && std::abs(p.x1 - e2) > 1)) left = false;
            if (p.x1 - p.x0 > 41) left = false;   // (1.2.0) 60 -> 40 px
        }
        if (p.k == wpc::Prim::LINE && p.w == 1 && (p.col == wpc::mix(0x22C55E, 0x0B0F17, 0.8) || p.col == wpc::mix(0x22C55E, 0x0B0F17, 0.5)) && p.x1 - p.x0 <= 2) ++dots;
        if (p.k == wpc::Prim::TEXT) ++texts;
        if (p.k == wpc::Prim::BOX) left = false;
    }
    CHECK(sc.lnodesDrawn == 9 && bright == 2 && dim == 7 && left, "(1.2.0) liquidity bars: left of the wave start, <= 40 px, LPOC bright, others 60% (" << bright << "/" << dim << ")");
    { int mxw = 0; for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.col == 0x22C55E) mxw = std::max(mxw, p.x1 - p.x0);
      CHECK(dots == 0 && mxw == 40 && wpc::LNODE_W == 40, "(1.2.0) no dotted liquidity lines; LPOC bar = 40 px max (" << dots << " dots, " << mxw << " px)"); }
    const wpc::Scene s0 = wpc::layout(Nb, nullptr, v); int t0 = 0; for (const wpc::Prim& p : s0.p) t0 += p.k == wpc::Prim::TEXT;
    CHECK(texts == t0, "no price tags for liquidity nodes");
}
static void testBook() {
    wpl::Book b; long long now = 1000000;
    auto ev = [&](int a, int side, long long tk, long long sz, long long id) { wpl::Ev e; e.action = a; e.side = side; e.tick = tk; e.size = sz; e.id = id; b.onEvent(e, now); };
    ev(wpl::NEW, 1, 100, 50, 1); ev(wpl::NEW, 0, 104, 30, 2); ev(wpl::NEW, 1, 99, 20, 3); ev(wpl::NEW, 1, 98, 10, 4);
    std::map<long long, std::pair<long long, long long> > snap; long long mm = 0;
    b.snapshot(now, snap, mm); CHECK(snap[100].first == 50 && snap[104].second == 30 && mm == 0, "book: resting lots per price and side");
    ev(wpl::FILL, 1, 100, 20, 1); ev(wpl::DEL, 1, 99, 0, 3); b.snapshot(now, snap, mm);
    CHECK(snap[100].first == 30 && snap.count(99) == 0, "FILL takes lots off, DEL removes");
    ev(wpl::MOD, 1, 97, 10, 4); now += 60000; ev(wpl::MOD, 1, 96, 10, 4); b.snapshot(now, snap, mm);
    CHECK(snap.count(96) == 0 && mm == 1, "re-priced 2x within 5 min = market maker, not counted");
    now += 6 * 60000; b.snapshot(now, snap, mm);
    CHECK(snap[96].first == 10 && mm == 0, "after 5 quiet minutes it counts again");
    ev(wpl::MOD, 1, 96, 25, 4); b.snapshot(now, snap, mm); CHECK(snap[96].first == 25, "a size change is not a re-price");
    wpl::Sampler sp;
    CHECK(sp.sample(b, 500, now) && !sp.sample(b, 500, now + 500) && sp.sample(b, 500, now + 1000), "sampled at most once a second");
    sp.sample(b, 680, now + 2000);
    std::vector<double> rows, bids;
    CHECK(sp.average({ 500, 680 }, 96, 2, 6, rows, bids) && std::fabs(rows[0] - 25) < 1e-9 && std::fabs(rows[2] - 30) < 1e-9 && std::fabs(bids[4]) < 1e-9 && std::fabs(rows[4] - 30) < 1e-9,
          "average lots per row over the wave's bars (" << rows[0] << "," << rows[2] << "," << rows[4] << ")");
    // (1.3.4) the 2026-10-29 live bug: a wave whose first bars are before the book was read takes the bars that have samples (>= 2 bars, >= 3 samples)
    CHECK(sp.average({ 100, 300, 500, 680 }, 96, 2, 6, rows, bids) && std::fabs(rows[0] - 25) < 1e-9 && std::fabs(rows[2] - 30) < 1e-9, "(1.3.4) bars before the book was read are left out: the average of the sampled bars");
    CHECK(!sp.average({ 100, 300, 500 }, 96, 2, 6, rows, bids), "(1.3.4) only 1 of 3 bars sampled: no liquidity yet");
    { wpl::Sampler one; one.sample(b, 500, now); one.sample(b, 500, now + 1000); one.sample(b, 500, now + 2000);
      CHECK(one.average({ 500 }, 96, 2, 6, rows, bids), "(1.3.4) a 1-bar wave with 3 samples has a book");
      wpl::Sampler two; two.sample(b, 500, now); two.sample(b, 680, now + 1000);
      CHECK(!two.average({ 500, 680 }, 96, 2, 6, rows, bids), "(1.3.4) fewer than 3 samples: no liquidity"); }
    { wpl::Sampler rk; rk.sample(b, 500, now); rk.sample(b, 610, now + 1000); rk.sample(b, 680, now + 2000); rk.rekey({ 500, 610 }, 680, 500);
      CHECK(rk.bars.count(500) == 1 && rk.bars.count(610) == 0 && rk.bars[680].samples == 2, "(1.3.4) re-key never moves the previous closed bar's own stamp"); }
    std::vector<double> r = { 1, 1, 9, 1, 1, 1, 8, 1, 1, 1, 1 }; double med = 0; const std::vector<int> pk = wpl::pickRaw(r, med);
    CHECK(pk.size() == 2 && pk[0] == 2 && pk[1] == 6 && std::fabs(med - 1) < 1e-9, "raw peaks, separated, biggest first");
    std::vector<double> edge = { 9, 1, 1, 1, 1, 1 }; CHECK(wpl::pickRaw(edge, med).size() == 1, "a peak on the edge row counts (raw, as wp8)");
}

// ---- (1.3.0, Rassul approved 17:27 CT) CoB legs: every CoB leg gets a profile even when it is not a qualifying wave
// the synthetic down leg of testProvisionalAndOrigin (bar 10 high 125 -> bar 24 low 93, confirmed at bar 27); store average 1000 ticks =
// it can never qualify as a wave, so anything drawn comes from the CoB rule
// (1.3.3) the synthetic CoB tests isolate the CoB rule: atrMult = 1e9 turns the 3 ATR rule off (the synthetic leg is > 3 ATR)
struct Syn { std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow> > V; };
static Syn synth() {
    Syn s; long long t = 1000000;
    auto add = [&](long long h, long long l, long long c) { wpc::Bar b; b.t = (t += 180); b.h = h; b.l = l; b.o = (h + l) / 2; b.c = c; s.B.push_back(b); };
    for (int i = 0; i < 10; ++i) add(100 + 2 * i, 97 + 2 * i, 99 + 2 * i);
    add(125, 120, 121);
    for (int j = 0; j < 14; ++j) add(122 - 2 * j, 119 - 2 * j, 119 - 2 * j);
    for (int j = 0; j < 3; ++j) add(98 + j, 95 + j, 97 + j);
    for (int j = 0; j < 6; ++j) add(100, 96, 98);         // bars 28..33: inside, touch no node above 100, close below the origin
    s.V.resize(s.B.size());
    for (size_t i = 0; i < s.B.size(); ++i) for (long long p = s.B[i].l; p <= s.B[i].h; ++p) s.V[i].push_back({ p, 10 });
    s.V[14].push_back({ 112, 400 }); s.V[19].push_back({ 101, 250 });
    return s;
}
static void testCobSynthetic() {
    const Syn Y = synth();
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = Y.V[(size_t)i]; return 1; };
    auto store = [](wpc::Store& S, long long sz) { for (int k = 0; k < 6; ++k) { S.add(-1, 1000 + k, sz); S.add(1, 2000 + k, sz); } };
    std::map<int, int> flags; flags[13] = 2;                                         // a bear CoB on bar 13, inside the down leg 10 -> 24
    // 1) the CoB leg is NOT a qualifying wave: it still gets the profile, at the confirmation (bar 27) - never before, never provisional
    wpc::Store S; store(S, 1000);
    wpc::Engine E; E.store = &S; E.N = 3; E.atrMult = 1e9; E.tick = 0.25; E.cobFn = [&](int k) { auto it = flags.find(k); return it == flags.end() ? 0 : it->second; };
    bool early = false, prov = false;
    for (int k = 0; k <= 25; ++k) { E.addBar(Y.B[(size_t)k], vap); wpc::Wave p; early = early || !E.waves.empty(); prov = prov || E.provisional(p, vap); }
    CHECK(!early && !prov, "CoB leg: nothing drawn before its end pivot is confirmed, no provisional CoB leg");
    E.addBar(Y.B[26], vap);
    CHECK(E.waves.size() == 1 && E.waves[0].kind == wpc::K_COB && E.waves[0].i0 == 10 && E.waves[0].i1 == 24 && E.waves[0].conf == 26, "CoB leg 10 -> 24 (not a wave) drawn at its confirmation, kind COB");
    if (E.waves.size() != 1) return;
    const wpc::Wave& w = E.waves[0];
    CHECK(E.C.qual[0] == 0 && E.C.qual[1] == 0 && E.C.cobOnly == 1 && E.C.cobBars[1] == 1, "counted as a CoB leg, not as a qualified wave");
    CHECK(!w.nodes.empty() && px(w.nodes[0].tick) == px(112) && w.nodes[0].rank == 0 && w.nodes.size() <= 5, "same profile rules: WPOC = the 400-lot row, max 5 nodes");
    CHECK(w.cobT == Y.B[13].t && E.cobUsed.count(Y.B[13].t) == 1, "the CoB bar time is kept with the leg");
    bool kindLines = false; int nw = 0;
    for (const std::string& l : E.out) { if (l.rfind("WAVE,", 0) == 0) { ++nw; kindLines = wpc::splitCsv(l).size() == 23 && wpc::splitCsv(l)[21] == "-" && l.find(",COB," + std::to_string(Y.B[13].t) + ",-,") != std::string::npos;   /* (1.3.3) + qualifiedBy "-" (CoB only), atr14 */ } if (l.rfind("NODE,", 0) == 0 && l.substr(l.size() - 4) != ",COB") kindLines = false; }
    CHECK(nw == 1 && kindLines, "record: WAVE line with kind COB + the CoB time, NODE lines with kind COB");
    // the stop rules: an opposite-direction CoB leg does NOT stop it, a same-direction CoB leg does; touch / origin as for waves
    E.confsCob[0].push_back(29); E.confsCob[1].push_back(31);
    for (int k = 27; k <= 30; ++k) E.addBar(Y.B[(size_t)k], vap);
    bool openAt30 = true; for (const wpc::Node& n : E.waves[0].nodes) openAt30 = openAt30 && (n.stop == wpc::OPEN || n.tick <= 100);
    CHECK(openAt30, "a CoB leg of the OTHER direction (bar 29) does not stop its nodes");
    E.addBar(Y.B[31], vap);
    bool nwAt31 = true; for (const wpc::Node& n : E.waves[0].nodes) if (n.tick > 100) nwAt31 = nwAt31 && n.stop == wpc::NEWWAVE && n.stopBar == 31;
    CHECK(nwAt31, "a later CoB leg of the SAME direction (bar 31) stops them (new wave)");
    bool stopKind = false; for (const std::string& l : E.out) stopKind = stopKind || (l.rfind("STOP,", 0) == 0 && l.find(",new wave,") != std::string::npos && l.substr(l.size() - 4) == ",COB");
    CHECK(stopKind, "STOP lines carry the kind");
    // 2) the CoB leg is ALSO a qualifying wave: drawn once, kind BOTH
    wpc::Store S2; store(S2, 8);
    wpc::Engine D; D.store = &S2; D.N = 3; D.atrMult = 1e9; D.tick = 0.25; D.cobFn = E.cobFn;
    for (int k = 0; k <= 27; ++k) D.addBar(Y.B[(size_t)k], vap);
    CHECK(D.waves.size() == 2 && D.waves[1].kind == wpc::K_BOTH && D.waves[1].dir < 0 && D.C.qual[1] == 1 && D.C.cobBoth == 1, "CoB leg that is also a qualifying wave: ONE profile, kind BOTH");
    bool both = false; for (const std::string& l : D.out) both = both || (l.rfind("WAVE,", 0) == 0 && l.find(",BOTH,") != std::string::npos);
    CHECK(both, "record: kind BOTH");
    // 3) a bull CoB on a down leg / CoB legs off: nothing
    std::map<int, int> bull; bull[13] = 1;
    wpc::Store S3; store(S3, 1000); wpc::Engine U; U.store = &S3; U.N = 3; U.atrMult = 1e9; U.tick = 0.25; U.cobFn = [&](int k) { auto it = bull.find(k); return it == bull.end() ? 0 : it->second; };
    for (int k = 0; k <= 27; ++k) U.addBar(Y.B[(size_t)k], vap);
    CHECK(U.waves.empty(), "a bull CoB inside a DOWN leg does not make it a CoB leg");
    wpc::Store S4; store(S4, 1000); wpc::Engine O; O.store = &S4; O.N = 3; O.atrMult = 1e9; O.tick = 0.25; O.cobFn = E.cobFn; O.cobLegs = false;
    for (int k = 0; k <= 27; ++k) O.addBar(Y.B[(size_t)k], vap);
    CHECK(O.waves.empty() && O.C.cobBars[1] == 1, "setting CoB legs OFF: no CoB leg (the CoB bar is still seen)");
    // 4) restart: the record redraws the COB leg as recorded even with no CoB flags; with CoB legs off it is not drawn
    std::vector<std::string> lines(E.out.begin(), E.out.end()); int bad = 0; const auto rec = wpc::parseRecord(lines, 3, 0.25, bad);
    CHECK(bad == 0 && rec.size() == 1 && rec.begin()->second.kind == wpc::K_COB && rec.begin()->second.cobT == Y.B[13].t, "record parses back: kind COB, CoB time");
    wpc::Store S5; store(S5, 1000); wpc::Engine R; R.store = &S5; R.N = 3; R.atrMult = 1e9; R.tick = 0.25; R.rec = rec;
    for (int k = 0; k <= 27; ++k) R.addBar(Y.B[(size_t)k], vap);
    CHECK(R.waves.size() == 1 && R.waves[0].fromRecord && R.waves[0].kind == wpc::K_COB && R.waves[0].nodes[0].tick == w.nodes[0].tick, "restart: the recorded CoB leg is redrawn as recorded (never vanishes)");
    wpc::Store S6; store(S6, 1000); wpc::Engine R2; R2.store = &S6; R2.N = 3; R2.atrMult = 1e9; R2.tick = 0.25; R2.rec = rec; R2.cobLegs = false;
    for (int k = 0; k <= 27; ++k) R2.addBar(Y.B[(size_t)k], vap);
    CHECK(R2.waves.empty(), "CoB legs OFF: a recorded COB-only leg is not drawn");
    // 5) bar by bar with a restart after every bar = one straight pass (no repaint)
    std::vector<std::string> recL; bool same = true;
    wpc::Store S7; store(S7, 1000); wpc::Engine A; A.store = &S7; A.N = 3; A.atrMult = 1e9; A.tick = 0.25; A.cobFn = E.cobFn;
    for (int k = 0; k < (int)Y.B.size(); ++k) {
        A.addBar(Y.B[(size_t)k], vap); recL.insert(recL.end(), A.out.begin(), A.out.end()); A.out.clear();
        wpc::Store Sx; store(Sx, 1000); wpc::Engine X; X.store = &Sx; X.N = 3; X.atrMult = 1e9; X.tick = 0.25; X.cobFn = E.cobFn; int b2 = 0; X.rec = wpc::parseRecord(recL, 3, 0.25, b2);
        for (int j = 0; j <= k; ++j) X.addBar(Y.B[(size_t)j], vap);
        if (X.waves.size() != A.waves.size()) same = false;
        for (size_t q = 0; q < X.waves.size() && q < A.waves.size(); ++q) {
            if (X.waves[q].kind != A.waves[q].kind || X.waves[q].nodes.size() != A.waves[q].nodes.size()) same = false;
            for (size_t n = 0; n < X.waves[q].nodes.size() && n < A.waves[q].nodes.size(); ++n) same = same && X.waves[q].nodes[n].tick == A.waves[q].nodes[n].tick && X.waves[q].nodes[n].stop == A.waves[q].nodes[n].stop && X.waves[q].nodes[n].stopBar == A.waves[q].nodes[n].stopBar;
        }
    }
    CHECK(same, "CoB legs: a restart after every bar = one straight pass (no repaint)");
    // 6) old (1.2.0) record lines still parse: 19-field WAVE, 7-field NODE / STOP = kind WAVE; a bad kind is a bad line
    std::vector<std::string> old = { "WAVE,1.2.0,3,down,100,200,300,a,b,c,1,2,3,4,5,1,10,1,0", "NODE,100,200,0,7700.25,500,3.000", "STOP,100,200,7700.25,touch,400,x",
                                     "WAVE,1.3.0,3,up,500,600,700,a,b,c,1,2,3,4,5,1,10,1,0,MAYBE,0" };
    int bo = 0; const auto ro = wpc::parseRecord(old, 3, 0.25, bo);
    CHECK(bo == 1 && ro.size() == 1 && ro.begin()->second.kind == wpc::K_WAVE && ro.begin()->second.nodes.size() == 1, "1.2.0 record lines parse (kind WAVE); an unknown kind is a bad line");
}
// the Oct 22 ES data: the 13:06-13:24 up leg (bars 41 -> 47) with a bull CoB on bar 43. (1.3.3) it is now a qualifying wave itself (a 14-bar
// high, > the average and > 3 ATR), so the CoB makes it kind BOTH - one profile; every wave and stop is the same as without the CoB
static void testCobReal(const Fix& F) {
    wpc::Store S; seed(S, F.B[0].t, 189, 172);
    wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25; E.cobFn = [](int k) { return k == 43 ? 1 : 0; };
    wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = F.V[(size_t)i]; return 1; };
    run(E, F, (int)F.B.size() - 1, vap);
    wpc::Store S0; seed(S0, F.B[0].t, 189, 172); wpc::Engine P; P.store = &S0; P.N = 3; P.tick = 0.25; run(P, F, (int)F.B.size() - 1, vap);
    CHECK(E.waves.size() == 5 && E.waves[3].kind == wpc::K_BOTH && E.waves[3].i0 == 41 && E.waves[3].i1 == 47 && E.waves[3].dir == 1 && E.waves[3].conf == 49 && E.waves[3].qWhy == 3, "(1.3.3) Oct 22: the 13:06-13:24 up leg with a bull CoB = a wave too: ONE profile, kind BOTH (confirmed at IRT's reversal bar 49)");
    bool same = P.waves.size() == 5 && E.waves.size() == 5;
    for (size_t w = 0; same && w < 5; ++w) { same = same && P.waves[w].i0 == E.waves[w].i0 && P.waves[w].i1 == E.waves[w].i1; for (size_t n = 0; same && n < P.waves[w].nodes.size(); ++n) same = same && P.waves[w].nodes[n].stop == E.waves[w].nodes[n].stop && P.waves[w].nodes[n].stopBar == E.waves[w].nodes[n].stopBar; }
    CHECK(same && E.waves[0].kind == wpc::K_WAVE && E.waves[2].kind == wpc::K_WAVE, "the two sell waves: kind WAVE; all waves and stops = the run without the CoB");
    if (E.waves.size() == 5) CHECK(!E.waves[3].nodes.empty() && E.waves[3].nodes.size() <= 5 && E.waves[3].rows.size() > 0, "the BOTH leg has its own profile + nodes (" << E.waves[3].nodes.size() << " nodes)");
    const std::string st = wpc::statusText(E, "ES", 180, "native,ok", "loaded", "loaded", 0, nullptr, false, 0);
    CHECK(st.find("COB_LEGS,on,CoB bars bull 1 bear 0,legs drawn COB only 0 BOTH (also a wave) 1") != std::string::npos && st.find("QUALIFIED_BY,AVG 0,ATR 1,BOTH 4,rule 14-bar extreme") != std::string::npos, "status COB_LEGS + (1.3.3) QUALIFIED_BY lines");
}


// ---- (1.3.2) Rassul 18:15-18:24 CT 2026-10-28: the wrong (lower) node, the outlined node, one row height, overlap, Levels column ----
static wpc::Bar mk(long long t, long long o, long long h, long long l, long long c) { wpc::Bar b; b.t = t; b.o = o; b.h = h; b.l = l; b.c = c; return b; }
static void test132() {
    // (a) the 10:03 ES wave: 7767.25 = 1793 (the recorded node) but the row above has more (1900) in the same distribution -> 7767.50 wins
    {   std::vector<long long> v(57, 877);                      // rows 7761.00 .. 7775.00 (1 tick), the wave's normal row 877 (med2 1754)
        auto at = [&](double px) -> long long& { return v[(size_t)std::llround((px - 7761.0) * 4)]; };
        at(7763.00) = 1500; at(7763.25) = 2613; at(7763.50) = 1400;   // the WPOC distribution
        at(7766.75) = 100; at(7767.00) = 1500; at(7767.25) = 1793; at(7767.50) = 1900; at(7767.75) = 100;
        // 1.3.1 picked the smoothed peak 7767.25 (1500 + 1793 + 1900 = 5193 > 7767.50's 1793 + 1900 + 100) although 7767.50 has more
        const long long med2 = wpc::median2(v);
        const std::vector<int> pk = wpc::pickNodes(v, med2);
        bool has = false, wrong = false; for (int r : pk) { has = has || r == (int)std::llround((7767.50 - 7761.0) * 4); wrong = wrong || r == (int)std::llround((7767.25 - 7761.0) * 4) || r == (int)std::llround((7767.75 - 7761.0) * 4); }
        CHECK(med2 == 1754 && !pk.empty() && pk[0] == (int)std::llround((7763.25 - 7761.0) * 4), "(1.3.2) WPOC = 7763.25 (2613)");
        CHECK(has && !wrong, "(1.3.2) the adjacent HIGHER row of the same distribution wins: 7767.50 (1900), not 7767.25 (1793), the 1.3.1 pick");
        // every node = the max raw row of its distribution, >= 2x, >= 3 rows apart, max 5 (random profiles)
        unsigned long long x = 88172645463325252ULL; bool ok = true;
        for (int it = 0; it < 300; ++it) {
            std::vector<long long> r(20 + it % 90); for (long long& q : r) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; q = (long long)(x % 1000) + ((x >> 20) % 9 == 0 ? 2500 : 0); }
            const long long m2 = wpc::median2(r); const std::vector<int> p2 = wpc::pickNodes(r, m2);
            if (p2.size() > 5) ok = false;
            for (size_t a = 0; a < p2.size(); ++a) {
                if (r[(size_t)p2[a]] < m2) ok = false;
                if (a && r[(size_t)p2[a]] > r[(size_t)p2[a - 1]]) ok = false;
                for (size_t b = 0; b < a; ++b) if (std::abs(p2[a] - p2[b]) < wpc::SEP_ROWS) ok = false;
                for (int d = -1; d <= 1; d += 2) { const int nb = p2[a] + d; if (nb >= 0 && nb < (int)r.size() && r[(size_t)nb] > r[(size_t)p2[a]]) ok = false; }   // never a lower row next to a higher one
            }
        }
        CHECK(ok, "(1.3.2) 300 random profiles: nodes >= 2x, biggest first, >= 3 rows apart, max 5, never next to a higher row");
    }
    // (b) a leg EXTENDED by an equal high later replaces its wave: ONE profile at that start (10:03 = 09:57-10:24 and 09:57-10:27)
    {   wpc::Store S; for (int k = 0; k < 20; ++k) { S.add(1, 1000 + k, 8); S.add(-1, 1000 + k, 8); }
        wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25;
        std::vector<wpc::Bar> B; long long t = 100000;
        for (int i = 0; i < 25; ++i) B.push_back(mk(t += 180, 31100 - i * 2, 31102 - i * 2, 31096 - i * 2, 31098 - i * 2));   // a down drift to 31050 lows
        const long long lo = B.back().l - 6;
        B.push_back(mk(t += 180, lo + 4, lo + 6, lo, lo + 2));                                     // 25: the low (wave start)
        for (int i = 1; i <= 5; ++i) B.push_back(mk(t += 180, lo + i * 8, lo + i * 8 + 6, lo + i * 8 - 4, lo + i * 8 + 4));   // 26-30 up
        const long long H = B.back().h + 6;
        B.push_back(mk(t += 180, H - 6, H, H - 8, H - 4));                                          // 31: the high
        B.push_back(mk(t += 180, H - 4, H - 2, H - 10, H - 8)); B.push_back(mk(t += 180, H - 8, H - 4, H - 12, H - 6));
        B.push_back(mk(t += 180, H - 6, H - 1, H - 9, H - 3));                                      // 34 -> confirms 31
        B.push_back(mk(t += 180, H - 3, H, H - 6, H - 2));                                          // 35: an EQUAL high later
        for (int i = 1; i <= 4; ++i) B.push_back(mk(t += 180, H - 2 - i * 4, H - 2 - i * 4 + 1, H - 8 - i * 4, H - 6 - i * 4));   // 36-39 down (confirms 35)
        wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o.clear(); for (long long k = B[(size_t)i].l; k <= B[(size_t)i].h; ++k) { wpc::VRow r; r.tick = k; r.vol = 10 + (k % 7 == 0 ? 90 : 0) + (i == 31 ? 40 : 0); o.push_back(r); } return 1; };
        int upWaves = 0, maxUpAt38 = 0;
        for (size_t i = 0; i < B.size(); ++i) { E.addBar(B[i], vap); if (i == 34) { for (const wpc::Wave& w : E.waves) upWaves += w.dir > 0; } if (i == 38) for (const wpc::Wave& w : E.waves) if (w.dir > 0) ++maxUpAt38; }
        int up = 0, i1 = -1; for (const wpc::Wave& w : E.waves) if (w.dir > 0) { ++up; i1 = w.i1; }
        CHECK(up == 1 && maxUpAt38 == 1 && i1 == 31 && E.C.replaced == 0, "(1.3.2, IRT zigzag) an EQUAL high later does not move the pivot: ONE up wave, ending at the first high (bar 31) (" << upWaves << "/" << up << "/" << i1 << "/" << E.C.replaced << ")");
        wpc::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 700; v.first = 0; v.last = 39; v.lastBar = 39; v.bw = 20; v.tick = 0.25;
        v.X = [&](int i) { return 50 + (i + 0.5) * 20; }; v.Y = [&](double tk) { return 690 - (tk - (lo - 10)) * 6.0; };
        v.textW = [](const std::string& q, bool) { return (int)q.size() * 7; };
        const wpc::Scene sc = wpc::layout(E, nullptr, v);
        const int xs = (int)std::lround(v.X(25) - 20 * 0.45);
        std::map<int, int> rowsAtY; int boxes = 0, hl = 0, hlOk = 0;
        for (const wpc::Prim& p : sc.p) { if (p.k == wpc::Prim::BOX) ++boxes; if (p.k == wpc::Prim::FILL && p.x0 == xs && p.col == 0x334155) ++rowsAtY[p.y0]; }
        bool one = true; for (const auto& kv : rowsAtY) one = one && kv.second == 1;
        for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.x0 == xs && (p.col == 0xC88C14 || p.col == 0xB4BECD)) { ++hl; for (const wpc::Prim& q : sc.p) if (q.k == wpc::Prim::FILL && q.col == 0x334155 && q.x0 == xs && q.y0 == p.y0 && q.y1 == p.y1 && q.x1 == p.x1) { ++hlOk; break; } }
        CHECK(!rowsAtY.empty() && one, "(1.3.2) one profile row per price at that start (no stacked profiles)");
        CHECK(boxes == 0 && hl > 0 && hl == hlOk, "(1.3.2) no outline (BOX) primitive; each highlight = exactly its own profile row (same y and length), filled (" << hl << "/" << hlOk << ")");
        // rows first, highlights after: no profile row is painted after a highlight
        bool seenHl = false, after = false; for (const wpc::Prim& p : sc.p) { if (p.k == wpc::Prim::FILL && (p.col == 0xC88C14 || p.col == 0xB4BECD)) seenHl = true; else if (seenHl && p.k == wpc::Prim::FILL && p.col == 0x334155) after = true; }
        CHECK(seenHl && !after, "(1.3.2) every profile row is drawn before any highlight (nothing paints over a highlight)");
        // (c) the Levels column: nothing left of clipL; a profile starting inside it is CLIPPED (its rows keep their right ends)
        wpc::View c = v; c.clipL = xs + 15; c.clipR = 1200;
        const wpc::Scene s2 = wpc::layout(E, nullptr, c);
        bool inside = true; int cut = 0; for (const wpc::Prim& p : s2.p) { if (std::min(p.x0, p.x1) < c.clipL || std::max(p.x0, p.x1) > c.clipR) inside = false; if (p.k == wpc::Prim::FILL && p.col == 0x334155 && p.x0 == c.clipL) ++cut; }
        int sameEnds = 0; for (const wpc::Prim& p : s2.p) if (p.k == wpc::Prim::FILL && p.col == 0x334155 && p.x0 == c.clipL) for (const wpc::Prim& q : sc.p) if (q.k == wpc::Prim::FILL && q.col == 0x334155 && q.y0 == p.y0 && q.x0 == xs && q.x1 == p.x1) { ++sameEnds; break; }
        CHECK(inside && cut > 0 && sameEnds == cut, "(1.3.2) Levels column: nothing drawn left of it, the profile is clipped not shifted (" << cut << " rows cut)");
        bool hdr = false; for (const wpc::Prim& p : s2.p) hdr = hdr || (p.k == wpc::Prim::TEXT && p.x0 == c.clipL + 4);
        CHECK(hdr, "(1.3.2) the version header starts right of the Levels column");
    }
    // (d) profiles never overlap: a profile is cut at the next profile's start
    {   wpc::Engine E; wpc::Wave a; a.dir = -1; a.i0 = 2; a.i1 = 12; a.e0 = 100; a.rowTicks = 1; a.rows.assign(10, 50); a.rows[4] = 400; a.maxVol = 400; a.scanned = 12;
        wpc::Wave b = a; b.dir = 1; b.i0 = 5; b.i1 = 15; E.waves.push_back(a); E.waves.push_back(b);
        wpc::View v; v.l = 0; v.t = 0; v.r = 1400; v.b = 700; v.first = 0; v.last = 20; v.lastBar = 20; v.bw = 40; v.tick = 0.25;
        v.X = [&](int i) { return 20 + (i + 0.5) * 40; }; v.Y = [&](double tk) { return 600 - (tk - 90) * 10.0; };
        const wpc::Scene sc = wpc::layout(E, nullptr, v);
        const int xa = (int)std::lround(v.X(2) - 18), xb = (int)std::lround(v.X(5) - 18); int mx = 0;
        for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.x0 == xa) mx = std::max(mx, p.x1);
        CHECK(mx > xa && mx <= xb - 2, "(1.3.2) a profile ends before the next profile's start (" << mx << " <= " << xb - 2 << ")");
    }
    // (e) a recorded node of 2-tick rows highlights the highest RAW row inside its recorded row
    {   wpc::Engine E; wpc::Wave w; w.e0 = 1000; w.rowTicks = 1; w.rows = { 5, 5, 300, 700, 5, 5 };
        CHECK(E.recRow(w, 1003, 2) == 3 && E.recRow(w, 1003, 1) == 3 && E.recRow(w, 1002, 1) == 2, "(1.3.2) recorded 2-tick node -> its highest raw 1-tick row");
    }
    // (f) liquidity: the forming bar's samples under moving stamps still count for the closed bar (span / re-key)
    {   wpl::Book b; long long now = 5000000; wpl::Ev e; e.action = wpl::NEW; e.side = 1; e.tick = 100; e.size = 40; e.id = 1; b.onEvent(e, now);
        wpl::Sampler sp; for (int k = 0; k < 179; k += 3) sp.sample(b, 1001 + k, now += 1000);   // stamps 1001 .. 1178 while the bar formed
        std::vector<double> rows, bids;
        CHECK(!sp.average({ 1180 }, 100, 1, 3, rows, bids), "1.1.1 exact stamp: a moving forming stamp never matched (the 10-28 record has no LNODE)");
        CHECK(sp.average({ 1180 }, 100, 1, 3, rows, bids, 180) && std::fabs(rows[0] - 40) < 1e-9, "(1.1.2) span: every sample inside the bar counts");
        std::vector<long long> keys; for (int k = 0; k < 179; k += 3) keys.push_back(1001 + k);
        sp.rekey(keys, 1180);
        CHECK(sp.bars.size() == 1 && sp.average({ 1180 }, 100, 1, 3, rows, bids) && std::fabs(bids[0] - 40) < 1e-9, "(1.3.2) re-keyed to the closed bar's stamp");
    }
    // (g) recorded liquidity nodes come back after a restart even when the wave had no volume node; a recorded wave is not written again
    {   std::vector<std::string> lines = { "WAVE,1.3.1,3,up,1000,2000,2540,a,b,c,1.00,2.00,1.00,0.50,20,1,10,0,0,WAVE,0", "LNODE,1000,2000,0,1.25,50.0,3.000,bid,alone,WAVE" };
        int bad = 0; const auto m = wpc::parseRecord(lines, 3, 0.25, bad);
        CHECK(bad == 0 && m.size() == 1 && m.begin()->second.lnodes.size() == 1 && m.begin()->second.nodes.empty() && m.begin()->second.rowTicks == 1, "record: a wave with only a liquidity node parses");
        std::vector<std::string> l2 = { "ROWS,1000,2000,1.00,1,5;6;7" }; int bad2 = 0; wpc::parseRecord(l2, 3, 0.25, bad2);
        CHECK(bad2 == 0, "(1.3.2) ROWS lines are valid record lines");
    }
}


// (1.3.2) parity with IRT's own ZIGZAG (3 bars, High/Low) on his ES chart today: the pivots he sees (screenshots 18:15-18:32 CT)
static void testIrtZigzag1028(const char* path) {
    const Fix F = load(path, 0.25);
    CHECK(F.B.size() == 300, "10-28 bars fixture (" << F.B.size() << ")");
    if (F.B.size() != 300) return;
    wpc::Store S; wpc::Engine E; E.store = &S; E.N = 3; E.tick = 0.25; wpc::VapFn vap = [](int, std::vector<wpc::VRow>&) { return 0; };   // (1.3.3) + an (empty) store: the AVG rule warms up
    for (const wpc::Bar& b : F.B) E.addBar(b, vap);
    std::string got; for (const wpc::Pivot& p : E.alt) { const std::string hm2 = wpc::hm(F.B[(size_t)p.i].t).substr(11); if (hm2 >= "09:00" && hm2 <= "14:30") got += hm2 + (p.type > 0 ? "H" : "L") + px(p.px) + " "; }
    // what his IRT ZIGZAG shows: 09:57 L, 10:24 H (not the equal 10:27), 11:39 L (-0.6), 11:57 H (2.0), 12:30 L (not 12:33), 13:18 L (-1.1),
    // 13:39 H (4.0) - the old N-bar fractal had 13:36 L 7756.25 instead (his "zig zag different than irts") -, 13:57 L (-2.4), 14:06 H (2.1), 14:15 L (-2.5)
    const char* want[] = { "09:57L7759.25", "10:24H7775.25", "11:39L7770.50", "11:57H7787.25", "12:30L7766.75", "13:03H7788.50", "13:18L7758.00", "13:39H7794.25", "13:57L7762.25", "14:06H7786.25", "14:15L7757.00" };
    bool all = true; for (const char* w : want) all = all && got.find(w) != std::string::npos;
    CHECK(all && got.find("13:36L") == std::string::npos && got.find("10:27H") == std::string::npos, "(1.3.2) IRT ZIGZAG pivots on ES 10-28 (" << got << ")");
    // (1.3.3) the new rule on the same day (no earlier store: the average warms up from 02:24 CT; VAP absent = pending, still counted):
    // 1.3.2 (20-bar, AVG) qualified 11 (7 up / 4 down); 1.3.3 (14-bar, AVG or 3 ATR) 21 (10 up / 11 down) = AVG 1, ATR 7, BOTH 13;
    // 29 candidates (16 up / 13 down) - the 8 that beat neither the average nor 3 ATR are not drawn
    int q0 = 0; for (const wpc::Wave& w : E.waves) q0 += w.qWhy == 0;
    CHECK(E.C.cand[0] == 16 && E.C.cand[1] == 13 && E.C.qual[0] == 10 && E.C.qual[1] == 11 && E.waves.size() == 21 && q0 == 0
          && E.C.qBy[wpc::Q_AVG] == 1 && E.C.qBy[wpc::Q_ATR] == 7 && E.C.qBy[3] == 13, "(1.3.3) ES 10-28: 21 waves (AVG 1 / ATR 7 / BOTH 13), 8 candidates not drawn (" << E.C.qual[0] << "/" << E.C.qual[1] << " waves " << E.waves.size() << " q0 " << q0 << " by " << E.C.qBy[1] << "/" << E.C.qBy[2] << "/" << E.C.qBy[3] << ")");
}

// (1.3.3, Rassul 04:51 CT 2026-10-29) the rule itself: 14-bar extreme; AVG or > 3 x the 14-bar ATR; neither = not a wave
static void testRule133() {
    wpc::Engine E; E.N = 3; E.tick = 0.25;
    for (int i = 0; i < 30; ++i) { wpc::Bar b; b.t = 1000 + 180 * i; b.l = 100; b.h = 104; b.o = 101; b.c = 102; E.B.push_back(b); }
    double atr = 0;
    CHECK(!E.atrAt(13, atr) && E.atrAt(14, atr) && std::fabs(atr - 4) < 1e-12, "(1.3.3) ATR14 = mean true range of the 14 closed bars to the end bar (needs 15 bars)");
    E.B[20].h = 110; E.B[25].c = 110; E.B[25].h = 110;   // a gap: true range uses the previous close (bar 26: 110 -> 100)
    CHECK(E.atrAt(26, atr) && std::fabs(atr - (4.0 * 11 + 10 + 10 + 10) / 14) < 1e-12, "(1.3.3) true range includes the gap from the previous close (bars 20, 25 = 10; bar 26 = 110 close -> 100 low = 10)");
    E.B[20].h = 104; E.B[25].c = 102; E.B[25].h = 104;
    auto q = [&](long long size, double avg, int nHist, int end) { wpc::Wave w; w.size = size; w.avg = avg; w.nHist = nHist; return E.qualify(w, end); };
    CHECK(q(13, 20, 20, 29) == wpc::Q_ATR, "(1.3.3) smaller than the average (13 < 20) but > 3 ATR (12) -> qualifies, ATR");
    CHECK(q(11, 20, 20, 29) == 0, "(1.3.3) neither (11 < 20 avg, 11 < 12 = 3 ATR) -> not a wave");
    CHECK(q(12, 20, 20, 29) == 0, "(1.3.3) exactly 3 ATR is not bigger");
    CHECK(q(11, 10, 20, 29) == wpc::Q_AVG && q(25, 20, 20, 29) == 3, "(1.3.3) AVG only / BOTH");
    CHECK(q(11, 10, 4, 29) == 0 && q(13, 10, 4, 29) == wpc::Q_ATR, "(1.3.3) the AVG warm-up (5 earlier waves) does not hold back the ATR rule");
    CHECK(q(13, 20, 20, 10) == 0, "(1.3.3) too few bars for the ATR: the AVG rule only");
    E.B[5].h = 120; E.B[20].h = 110;   // bar 5 is 15 bars before bar 20, outside the 14-bar window
    CHECK(wpc::LOOKBACK == 14 && E.is20(20, 1), "(1.3.3) a high above the previous 13 bars = a 14-bar high (a higher bar 15 bars back does not count)");
    E.B[7].h = 120; CHECK(!E.is20(20, 1), "(1.3.3) a higher bar 13 bars back: not a 14-bar high");
    E.B[20].l = 90; CHECK(E.is20(20, -1), "(1.3.3) 14-bar low");
    // the record line: qualifiedBy + atr14 (23 fields) and its parse; an unknown qualifiedBy = a bad line
    wpc::Wave w; w.dir = 1; w.t0 = 100; w.t1 = 200; w.qWhy = wpc::Q_ATR; w.atr = 16; const std::string l = wpc::waveLine(w, 3, 0.25);
    const auto f = wpc::splitCsv(l); int bad = 0;
    const auto r = wpc::parseRecord({ l, "WAVE,1.3.3,3,up,300,400,500,a,b,c,1,2,3,4,5,1,10,1,0,WAVE,0,MAYBE,1.0" }, 3, 0.25, bad);
    CHECK(f.size() == 23 && f[21] == "ATR" && f[22] == "4.0000" && bad == 1 && r.size() == 1 && r.begin()->second.qWhy == wpc::Q_ATR, "(1.3.3) WAVE line ...,ATR,4.0000 parses back; unknown qualifiedBy = bad (" << l << ")");
    // the store: version 2 (14-bar candidates); a version 1 (20-bar) store is rebuilt
    wpc::Store S, S2; S.add(1, 100, 10); std::string why;
    std::string t = S.toText("ES", 180); CHECK(t.rfind("WAVEPOC_STORE,2,", 0) == 0 && wpc::storeFromText(t, "ES", 180, 3, 0, S2, why), "(1.3.3) store version 2 round trip");
    t.replace(0, 16, "WAVEPOC_STORE,1,"); CHECK(!wpc::storeFromText(t, "ES", 180, 3, 0, S2, why) && why.find("older store version") == 0, "(1.3.3) a version 1 (20-bar) store is rebuilt (" << why << ")");
}

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "wavepoc_es1022.csv";
    const Fix F = load(path, 0.25);
    CHECK(F.B.size() == 80, "fixture: 80 bars (" << F.B.size() << ")");
    if (F.B.size() != 80) { std::cerr << "no fixture\n"; return 1; }
    testReference(F); testNoRepaint(F); testN5(F); testProvisionalAndOrigin(); testStoreAndHelpers(); testLayout(F); testCobSynthetic(); testCobReal(F);
    loadLiq(argc > 2 ? argv[2] : "wavepoc_es1022_liq.csv"); testLiquidityReference(F); testBook(); test132(); testIrtZigzag1028(argc > 3 ? argv[3] : "wavepoc_es1028_bars.csv"); testRule133();
    std::cout << "test_wavepoc_logic: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
