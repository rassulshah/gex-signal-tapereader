// test_wavepoc_logic.cpp - lsWavePOC 1.0.0 logic suite (WavePOCLogic.h, no SDK).
//   (in plugin\tests_wavepoc) g++ -std=c++17 -Wall -Wextra -Werror -I.. test_wavepoc_logic.cpp -o t && ./t wavepoc_es1022.csv wavepoc_es1022_liq.csv
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
    CHECK(E.waves.size() == 2, "two qualifying waves on Oct 22 (got " << E.waves.size() << ")");
    if (E.waves.size() != 2) return;
    const wpc::Wave& a = E.waves[0]; const wpc::Wave& b = E.waves[1];
    CHECK(a.i0 == 6 && a.i1 == 16 && a.dir == -1 && a.conf == 19, "wave 1 = 11:18-11:48 sell wave, confirmed 3 bars later");
    CHECK(b.i0 == 31 && b.i1 == 41 && b.dir == -1 && b.conf == 44, "wave 2 = 12:33-13:03 sell wave (confirmed 13:12)");
    CHECK(a.size == 150 && b.size == 106, "sizes 37.50 / 26.50 pts");
    CHECK(a.rowTicks == 2 && a.rows.size() == 76 && b.rows.size() == 54, "row size rule: 2-tick rows, 76 / 54 rows (" << a.rows.size() << "/" << b.rows.size() << ")");
    const char* ea[4] = { "7719.25", "7690.25", "7698.25", "7707.25" }; const double ra[4] = { 5.2, 2.7, 2.7, 2.6 };
    CHECK(a.nodes.size() == 4, "wave 1 has 4 nodes (" << a.nodes.size() << ")");
    for (size_t k = 0; k < a.nodes.size() && k < 4; ++k) {
        CHECK(px(a.nodes[k].tick) == ea[k], "wave 1 node " << k << " = " << ea[k] << " (got " << px(a.nodes[k].tick) << ")");
        CHECK(std::fabs(std::round(a.nodes[k].ratio * 10) / 10 - ra[k]) < 1e-9, "wave 1 node " << k << " ratio " << ra[k] << " (got " << a.nodes[k].ratio << ")");
    }
    const char* eb[3] = { "7700.25", "7693.75", "7681.25" }; const double rb[3] = { 6.3, 2.5, 2.3 };
    CHECK(b.nodes.size() == 3, "wave 2 has 3 nodes");
    for (size_t k = 0; k < b.nodes.size() && k < 3; ++k) {
        CHECK(px(b.nodes[k].tick) == eb[k], "wave 2 node " << k << " = " << eb[k] << " (got " << px(b.nodes[k].tick) << ")");
        CHECK(std::fabs(std::round(b.nodes[k].ratio * 10) / 10 - rb[k]) < 1e-9, "wave 2 node " << k << " ratio " << rb[k]);
    }
    CHECK(wpc::tagText(b.nodes[0], 0.25) == "WPOC 7700.25 6.3x" && wpc::tagText(b.nodes[1], 0.25) == "7693.75 2.5x", "tag text as approved");
    // the stop rules, as wp3.py on the same day
    const int sa[4][2] = { { wpc::NEWWAVE, 44 }, { wpc::TOUCH, 17 }, { wpc::TOUCH, 18 }, { wpc::NEWWAVE, 44 } };
    for (size_t k = 0; k < 4 && k < a.nodes.size(); ++k) CHECK(a.nodes[k].stop == sa[k][0] && a.nodes[k].stopBar == sa[k][1], "wave 1 node " << k << " stop " << wpc::stopName(sa[k][0]) << " at bar " << sa[k][1] << " (got " << wpc::stopName(a.nodes[k].stop) << " " << a.nodes[k].stopBar << ")");
    const int sb[3][2] = { { wpc::TOUCH, 46 }, { wpc::TOUCH, 44 }, { wpc::TOUCH, 60 } };
    for (size_t k = 0; k < 3 && k < b.nodes.size(); ++k) CHECK(b.nodes[k].stop == sb[k][0] && b.nodes[k].stopBar == sb[k][1], "wave 2 node " << k << " touched at bar " << sb[k][1]);
    CHECK(E.C.qual[0] == 0 && E.C.qual[1] == 2, "no up wave qualifies (14:00-14:27 = 21.5 pts is not bigger than 21.5)");
    // the record lines (one WAVE + its NODEs per wave, one STOP per node)
    int nw = 0, nn = 0, ns = 0; for (const std::string& l : E.out) { nw += l.rfind("WAVE,", 0) == 0; nn += l.rfind("NODE,", 0) == 0; ns += l.rfind("STOP,", 0) == 0; }
    CHECK(nw == 2 && nn == 7 && ns == 7, "record: 2 WAVE, 7 NODE, 7 STOP lines (" << nw << "/" << nn << "/" << ns << ")");
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
    CHECK(bad == 0 && rec.size() == 2, "record parses back (2 waves, 0 bad lines)");
    const auto it = rec.find(std::make_pair(E.waves[1].t0, E.waves[1].t1));
    CHECK(it != rec.end() && it->second.nodes.size() == 3 && it->second.nodes[0].tick == E.waves[1].nodes[0].tick, "recorded WPOC of wave 2 = 7700.25");
    // a recorded wave is drawn as recorded even when the average (store) would now refuse it
    wpc::Store big; seed(big, F.B[0].t, 2000, 2000);
    wpc::Engine R; R.store = &big; R.N = 3; R.tick = 0.25; R.rec = rec; run(R, F, (int)F.B.size() - 1, vap);
    CHECK(R.waves.size() == 2 && R.C.recordUsed == 2 && R.waves[0].fromRecord, "recorded waves are redrawn from the record (never vanish)");
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
    CHECK(differ && Bn.alt.size() < A.alt.size(), "N=5 gives fewer / other pivots (" << A.alt.size() << " vs " << Bn.alt.size() << ")");
    for (size_t k = 0; k < Bn.waves.size(); ++k) CHECK(Bn.waves[k].conf == Bn.waves[k].i1 + 5, "N=5: confirmation 5 bars after the extreme");
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
    for (int k = 0; k <= 26; ++k) { E.addBar(B[(size_t)k], vap); wpc::Wave p; if (E.provisional(p, vap)) { provEnd.push_back(p.i1); last = p; } else provEnd.push_back(-1); }
    CHECK(provEnd[15] == -1, "no provisional wave before the extreme is a 20-bar low");
    CHECK(provEnd[21] == 21 && provEnd[22] == 22 && provEnd[24] == 24, "the provisional wave moves to every new extreme (21, 22 .. 24)");
    CHECK(provEnd[26] == 24 && last.provisional && !last.nodes.empty(), "after the turn the provisional wave stays at the extreme (bar 24), dashed");
    CHECK(E.waves.empty(), "nothing confirmed before N bars after the extreme");
    E.addBar(B[27], vap); wpc::Wave p;
    CHECK(!E.provisional(p, vap), "confirmed: the provisional copy is gone");
    CHECK(E.waves.size() == 1 && E.waves[0].i0 == 10 && E.waves[0].i1 == 24 && E.waves[0].conf == 27, "the wave confirmed at bar 27 (10 -> 24)");
    if (E.waves.size() != 1) return;
    bool same = E.waves[0].nodes.size() == last.nodes.size();
    for (size_t k = 0; same && k < last.nodes.size(); ++k) same = last.nodes[k].tick == E.waves[0].nodes[k].tick;
    CHECK(same, "the confirmed nodes = the provisional nodes (only the line style changes)");
    CHECK(px(E.waves[0].nodes[0].tick) == wpc::pxText(112, 0.25), "WPOC = the 400-lot row (" << px(E.waves[0].nodes[0].tick) << ")");
    E.addBar(B[28], vap);
    bool allOrigin = true; for (const wpc::Node& n : E.waves[0].nodes) allOrigin = allOrigin && (n.stop == wpc::ORIGIN && n.stopBar == 28);
    CHECK(allOrigin, "a close above the origin (125) stops every line not touched (origin close)");
    // VAP missing: the wave waits (pending), nothing drawn, retried later; the stops are then replayed from its end
    wpc::Store S2; for (int k = 0; k < 6; ++k) S2.add(-1, 1000 + k, 8);
    wpc::Engine M; M.store = &S2; M.N = 3; M.tick = 0.25; bool have = false;
    wpc::VapFn vap2 = [&](int i, std::vector<wpc::VRow>& o) { if (!have && i == 17) return 0; o = V[(size_t)i]; return 1; };
    for (int k = 0; k <= 28; ++k) M.addBar(B[(size_t)k], vap2);
    CHECK(M.waves.size() == 1 && M.waves[0].pending && M.waves[0].nodes.empty() && M.pendingCount() == 1, "VAP missing for one bar: the wave is pending, not drawn");
    have = true; CHECK(M.retryPending(vap2) == 1 && !M.waves[0].pending && M.waves[0].nodes.size() == E.waves[0].nodes.size(), "retry fills it in");
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
    CHECK(wpc::rowTicksFor(150) == 2 && wpc::rowTicksFor(80) == 1 && wpc::rowTicksFor(81) == 2 && wpc::rowTicksFor(800) == 10, "row size rule");
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
    CHECK(texts > 0 && tagsRight, "price tags only right of the last bar (never over candles)");
    (void)whiteLine1;
    {   // (1.2.0, Rassul 16:26 CT "remove the drawing of levels across the chart and from the wave pocs") no line / circle at all (zigzag off)
        int lines = 0, ovals = 0; for (const wpc::Prim& p : sc.p) { if (p.k == wpc::Prim::LINE && std::min(p.x0, p.x1) < xLast - 1) ++lines; if (p.k == wpc::Prim::OVAL) ++ovals; }   // (the tag leader right of the last bar is the tag itself)
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
    CHECK(sc.nodesDrawn <= 5 * sc.wavesDrawn && sc.nodesDrawn == 7, "nodes drawn = 4 + 3, at most 5 per wave");
    bool header = false; for (const wpc::Prim& p : sc.p) header = header || (p.k == wpc::Prim::TEXT && p.text == std::string("Wave POC v") + wpc::VERSION);
    CHECK(header && std::string(wpc::VERSION) == "1.2.0", "header 'Wave POC v1.2.0'");
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
    CHECK(E.waves.size() == 2, "two waves with the liquidity function on");
    if (E.waves.size() != 2) return;
    const wpc::Wave& a = E.waves[0]; const wpc::Wave& b = E.waves[1];
    CHECK(a.liqOk && a.lnodes.size() == 3 && b.lnodes.size() == 4, "liquidity nodes (1.1.1: 1.9x, raw-row valley): 3 (wave 1) and 4 (wave 2) as wp9.py (" << a.lnodes.size() << "/" << b.lnodes.size() << ")");
    const char* ea[3] = { "7688.25", "7707.25", "7710.25" }; const double ra[3] = { 2.31, 2.01, 1.92 };
    for (size_t k = 0; k < 3 && k < a.lnodes.size(); ++k) CHECK(px(a.lnodes[k].tick) == ea[k] && std::fabs(a.lnodes[k].ratio - ra[k]) < 0.006 && a.lnodes[k].side == 1,
        "wave 1 liquidity node " << k << " = " << ea[k] << " " << ra[k] << "x bids (got " << px(a.lnodes[k].tick) << " " << a.lnodes[k].ratio << ")");
    const char* eb[4] = { "7685.25", "7680.25", "7682.25", "7689.25" }; const double rb[4] = { 4.53, 4.01, 2.85, 2.39 };
    for (size_t k = 0; k < 4 && k < b.lnodes.size(); ++k) CHECK(px(b.lnodes[k].tick) == eb[k] && std::fabs(b.lnodes[k].ratio - rb[k]) < 0.006 && b.lnodes[k].side == 1,
        "wave 2 liquidity node " << k << " = " << eb[k] << " " << rb[k] << "x bids (got " << px(b.lnodes[k].tick) << " " << b.lnodes[k].ratio << ")");
    CHECK(a.lnodes.size() == 3 && !a.lnodes[0].nearVol && a.lnodes[1].nearVol, "7707.25 sits on the volume node 7707.25 (within 2 ticks), 7688.25 alone");
    CHECK(b.lnodes.size() == 4 && b.lnodes[0].stop == wpc::TOUCH && b.lnodes[0].stopBar > b.i1, "liquidity lines stop by the same rules (LPOC 7685.25 touched later)");
    int nl = 0, ls = 0; for (const std::string& l : E.out) { nl += l.rfind("LNODE,", 0) == 0; ls += l.rfind("LSTOP,", 0) == 0; }
    CHECK(nl == 7 && ls == 7, "record: 7 LNODE lines (+ near / alone) and 7 LSTOP lines (" << nl << "/" << ls << ")");
    bool nearTxt = false; for (const std::string& l : E.out) nearTxt = nearTxt || (l.rfind("LNODE,", 0) == 0 && l.find(",7707.25,") != std::string::npos && l.find("near_volume_node") != std::string::npos);
    CHECK(nearTxt, "LNODE line says near_volume_node");
    // restart without a book: the recorded liquidity nodes come back from the record
    int bad = 0; wpc::Store S2; seed(S2, F.B[0].t, 189, 172);
    wpc::Engine R; R.store = &S2; R.N = 3; R.tick = 0.25; R.rec = wpc::parseRecord(E.out, 3, 0.25, bad); run(R, F, (int)F.B.size() - 1, vap);
    CHECK(bad == 0 && R.waves.size() == 2 && R.waves[1].lnodes.size() == 4 && R.waves[1].lnodes[0].tick == b.lnodes[0].tick && R.waves[1].lnodes[0].stopBar == b.lnodes[0].stopBar, "restart: liquidity nodes redrawn from the record");
    // no book (waves formed before the indicator loaded): no liquidity nodes, no error
    wpc::Store S3; seed(S3, F.B[0].t, 189, 172);
    wpc::Engine Nb; Nb.store = &S3; Nb.N = 3; Nb.liqFn = [](const wpc::Wave&, std::vector<double>&, std::vector<double>&) { return false; }; run(Nb, F, (int)F.B.size() - 1, vap);
    CHECK(Nb.waves.size() == 2 && Nb.waves[0].lnodes.empty() && Nb.C.liqNoBook == 2 && Nb.waves[0].nodes.size() == 4, "no book: no liquidity nodes, the volume nodes unchanged");
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
    CHECK(sc.lnodesDrawn == 7 && bright == 2 && dim == 5 && left, "(1.2.0) liquidity bars: left of the wave start, <= 40 px, LPOC bright, others 60% (" << bright << "/" << dim << ")");
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
    CHECK(!sp.average({ 500, 680, 860 }, 96, 2, 6, rows, bids), "a wave with bars before the book was read: no liquidity (< 90% covered)");
    std::vector<double> r = { 1, 1, 9, 1, 1, 1, 8, 1, 1, 1, 1 }; double med = 0; const std::vector<int> pk = wpl::pickRaw(r, med);
    CHECK(pk.size() == 2 && pk[0] == 2 && pk[1] == 6 && std::fabs(med - 1) < 1e-9, "raw peaks, separated, biggest first");
    std::vector<double> edge = { 9, 1, 1, 1, 1, 1 }; CHECK(wpl::pickRaw(edge, med).size() == 1, "a peak on the edge row counts (raw, as wp8)");
}

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "wavepoc_es1022.csv";
    const Fix F = load(path, 0.25);
    CHECK(F.B.size() == 80, "fixture: 80 bars (" << F.B.size() << ")");
    if (F.B.size() != 80) { std::cerr << "no fixture\n"; return 1; }
    testReference(F); testNoRepaint(F); testN5(F); testProvisionalAndOrigin(); testStoreAndHelpers(); testLayout(F);
    loadLiq(argc > 2 ? argv[2] : "wavepoc_es1022_liq.csv"); testLiquidityReference(F); testBook();
    std::cout << "test_wavepoc_logic: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
