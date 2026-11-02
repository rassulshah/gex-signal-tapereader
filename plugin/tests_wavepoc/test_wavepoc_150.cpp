// test_wavepoc_150.cpp (1.5.0) - the wave delta side, letters I? / A? / T?, cluster A, dotted node borders, POC pullbacks, the new wave rule
//   g++ -std=c++17 -Wall -Wextra -Werror -I.. test_wavepoc_150.cpp -o t150 && ./t150 wavepoc_es1029_1030_delta.csv
#include "../WavePOCLogic.h"
#include <fstream>
#include <iostream>
#include <map>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

// an up wave 100 -> 200 ticks, 1-tick rows, delta 10 everywhere (median 10, 2x = 20) plus the given rows
static wpc::Wave upWave(const std::map<int, long long>& d) {
    wpc::Wave w; w.dir = 1; w.i0 = 0; w.i1 = 5; w.p0 = 100; w.p1 = 200; w.size = 100; w.e0 = 100; w.rowTicks = 1;
    w.rows.assign(101, 50); w.maxVol = 50; w.drows.assign(101, 10);
    for (auto& kv : d) w.drows[(size_t)kv.first] = kv.second;
    w.dMed2 = 20; w.dMax = 0; for (long long x : w.drows) w.dMax = std::max(w.dMax, std::llabs(x));
    wpc::Node n; n.tick = 150; n.row = 50; n.rank = 0; n.vol = 50; w.nodes.push_back(n);
    return w;
}
static std::vector<wpc::Bar> flat(int n, long long c) { std::vector<wpc::Bar> B((size_t)n); for (auto& b : B) { b.o = b.c = c; b.h = c + 2; b.l = c - 2; } return B; }
static int count(const std::vector<wpc::Letter>& L, char t) { int k = 0; for (auto& x : L) k += x.t == t; return k; }

int main(int argc, char** argv) {
    // ---- 1. the letters (1.6.2, Rassul 2026-11-02 09:54 CT "first consider top 3 nodes, then placement: if its in the first 1/2 it can be
    //      absorbtion or initiation, if it is in the last 20% with the wave, consider it to be trapped traders")
    {   const std::vector<wpc::Bar> B = flat(12, 195);
        wpc::Wave w = upWave({{10, 100}, {90, 95}, {40, -90}, {60, 85}});   // 4 big rows: the 4th (85) is left out
        const auto top = wpc::topDelta(w);
        CHECK(top.size() == 3 && top[0] == 10 && top[1] == 90 && top[2] == 40 && wpc::LET_TOP == 3, "top 3 by |delta|, biggest first (the 4th left out)");
        const auto L = wpc::letters(w, B);
        CHECK(count(L, 'I') == 1 && count(L, 'A') == 1 && count(L, 'T') == 1 && L.size() == 3, "I (10%, green) + A (40%, red) + T (90%, green) (" << L.size() << ")");
        for (auto& x : L) {
            if (x.t == 'I') CHECK(x.row == 10 && x.bull, "I? on row 10 in the wave's colour (green, up wave)");
            if (x.t == 'A') CHECK(x.row == 40 && x.bull, "A? on row 40 (against, first half) in the wave's colour (green: sellers absorbed)");
            if (x.t == 'T') CHECK(x.row == 90 && !x.bull, "T? on row 90 (with, last 20%) in the opposite colour (red: buyers trapped)");
        }
        CHECK(count(wpc::letters(upWave({{50, 95}}), B), 'I') == 1 && count(wpc::letters(upWave({{51, 95}}), B), 'I') == 0, "I? up to the halfway mark (50% yes, 51% no)");
        CHECK(count(wpc::letters(upWave({{50, -95}}), B), 'A') == 1 && count(wpc::letters(upWave({{51, -95}}), B), 'A') == 0, "A? up to the halfway mark (50% yes, 51% no)");
        CHECK(count(wpc::letters(upWave({{80, 95}}), B), 'T') == 1 && count(wpc::letters(upWave({{79, 95}}), B), 'T') == 0, "T? from 80% (80% yes, 79% no)");
        CHECK(wpc::letters(upWave({{65, 95}, {90, -95}}), B).empty(), "with the wave at 65% and against it in the last 20%: no letter");
        CHECK(wpc::letters(upWave({{10, 19}}), B).empty(), "a 1.9x row: no letter");
        wpc::Wave w4 = upWave({{10, 300}, {20, 290}, {30, 280}, {90, 95}});   // the green row near the top is only 4th biggest: no T
        CHECK(count(wpc::letters(w4, B), 'T') == 0, "a with-wave row near the top that is not in the top 3: no T");
        std::vector<wpc::Bar> B3 = flat(12, 195); B3[7].c = 188;
        CHECK(count(wpc::letters(upWave({{90, 95}}), B3), 'T') == 1, "what the bars do after the pivot never changes a letter");
    }
    {   // the same letter within 10% of the wave (min 8 ticks): the bigger row keeps it
        wpc::Wave w = upWave({{30, -90}, {34, -85}, {5, 40}});
        const auto L = wpc::letters(w, flat(12, 195));
        CHECK(count(L, 'A') == 1 && L[0].row == 30, "two A rows 4 ticks apart = one A on the bigger (" << count(L, 'A') << ")");
    }
    {   // down wave mirror: I near the TOP (red), A green near the top, T near the BOTTOM (green)
        wpc::Wave w = upWave({}); w.dir = -1; w.p0 = 200; w.p1 = 100; w.drows.assign(101, -10); w.drows[95] = -120; w.drows[70] = 110; w.drows[10] = -100; w.dMax = 120;
        const auto L = wpc::letters(w, flat(12, 105)); int i = 0, a2 = 0, t = 0;
        for (auto& x : L) { if (x.t == 'I') i += x.row == 95 && !x.bull; if (x.t == 'A') a2 += x.row == 70 && !x.bull; if (x.t == 'T') t += x.row == 10 && x.bull; }
        CHECK(i == 1 && a2 == 1 && t == 1 && L.size() == 3, "down wave: I? red near the top, A? red (wave colour) at 30%, T? green near the bottom (" << L.size() << ")");
    }
    {   // (1.6.2) no cluster A?: letters come from the top-3 nodes only
        CHECK(!wpc::CLUSTER_A, "cluster A? off");
    }
    {   // distribution around a node: same sign, >= median, contiguous
        wpc::Wave w = upWave({{40, 30}, {41, 60}, {42, 12}, {43, -50}, {39, 9}});
        int a, b; wpc::distOf(w, 41, a, b);
        CHECK(a == 40 && b == 42, "distribution = rows 40-42 (row 39 under the median, 43 the other sign) (" << a << "-" << b << ")");
    }
    // ---- 2. cluster A ----
    {   wpc::Wave w = upWave({}); for (int j = 48; j < 56; ++j) w.drows[(size_t)j] = -15;   // a band of small against rows, none 2x
        int a = -1, b = -1; const long long best = wpc::Engine::clusterBest(w, &a, &b);
        CHECK(best == 8 * 15 - 0 && a == 48 && b == 55, "cluster band = the 8-tick against band in the middle (" << best << " " << a << "-" << b << ")");
        w.clRef = 50; auto L = wpc::letters(w, flat(12, 195));
        CHECK(L.empty(), "(1.6.2) cluster A? off: a 120 band >= 2.0 x reference 50 no longer makes an A?");
        w.clRef = 70; L = wpc::letters(w, flat(12, 195));
        CHECK(L.empty(), "cluster 120 < 2.0 x 70 -> no cluster A");
        w.clRef = 0; CHECK(wpc::letters(w, flat(12, 195)).empty(), "no reference yet (fewer than 10 waves) -> no cluster A");
    }
    // ---- 3. POC pullbacks ----
    {   wpc::Wave w = upWave({});
        std::vector<wpc::Bar> B = flat(20, 170);
        B[7].l = 148; B[7].h = 160;    // low 1: through the POC (150), high above -> Pb
        B[10].l = 155; B[10].h = 165;  // low 2: stays above -> no
        B[12].l = 150; B[12].h = 151;  // low 3: touches exactly -> Pb
        B[15].l = 140; B[15].h = 160;  // low 4: beyond the 3-pullback limit
        std::vector<wpc::Pivot> alt = {{5, 1, 200}, {7, -1, 148}, {9, 1, 170}, {10, -1, 155}, {11, 1, 170}, {12, -1, 150}, {14, 1, 170}, {15, -1, 140}};
        const auto pb = wpc::pullbacks(w, alt, B);
        CHECK(pb.size() == 2 && pb[0] == 7 && pb[1] == 12, "Pb at lows 1 and 3 only (touch counts, 4th low is past the limit) (" << pb.size() << ")");
        B[7].h = 150; CHECK(wpc::pullbacks(w, alt, B).size() == 1, "a low whose HIGH is also at / below the POC is not a pullback hold");
        w.provisional = true; CHECK(wpc::pullbacks(w, alt, B).empty(), "a provisional wave gets no Pb");
    }
    // ---- 4. the real fixture (ES 10-29 / 10-30 RTH with VAP delta) ----
    if (argc > 1) {
        std::ifstream f(argv[1]); std::string line; std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow> > V;
        while (std::getline(f, line)) { if (line.empty() || line[0] == '#') continue; auto x = wpc::splitCsv(line);
            if (x[0] == "B") { wpc::Bar b; b.t = atoll(x[2].c_str()); wpc::toTicks(atof(x[3].c_str()), .25, b.o); wpc::toTicks(atof(x[4].c_str()), .25, b.h); wpc::toTicks(atof(x[5].c_str()), .25, b.l); wpc::toTicks(atof(x[6].c_str()), .25, b.c); B.push_back(b); V.resize(B.size()); }
            else { wpc::VRow r; wpc::toTicks(atof(x[2].c_str()), .25, r.tick); r.vol = atoll(x[3].c_str()); r.dlt = atoll(x[4].c_str()); V[(size_t)atoi(x[1].c_str())].push_back(r); } }
        CHECK(B.size() == 247, "fixture 247 bars");
        wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = V[(size_t)i]; return 1; };
        wpc::Store S; wpc::Engine E; E.store = &S; std::map<long long, std::string> early;   // letters of each wave seen 6 bars after its end (decided) vs at the end of the day
        auto sig = [&](const wpc::Wave& w) { std::string s; for (auto& L : wpc::letters(w, E.B)) { s += L.t; s += std::to_string(L.row); s += L.bull ? "g" : "r"; } return s; };
        for (size_t i = 0; i < B.size(); ++i) { E.addBar(B[i], vap); for (const wpc::Wave& w : E.waves) if (!w.provisional && !w.pending && (int)i == w.i1 + 6 && !early.count(w.t1)) early[w.t1] = sig(w); }
        int q = 0, bo = 0, ra = 0, rev = 0, haveD = 0, same = 0, cmp = 0; long long sumD = 0;
        for (const wpc::Wave& w : E.waves) { if (w.provisional) continue; ++q; bo += w.qWhy == wpc::Q_BO; ra += w.qWhy == wpc::Q_RALLY; rev += w.qWhy == 3;
            if (!w.drows.empty()) { ++haveD; for (size_t j = 0; j < w.drows.size(); ++j) sumD += w.drows[j]; }
            if (early.count(w.t1)) { ++cmp; same += early[w.t1] == sig(w); } }
        CHECK(q > 0 && rev > 0 && bo > 0, "fixture: reversal and breakout waves both qualify (" << q << " waves, " << rev << " rev, " << bo << " bo, " << ra << " rally)");
        CHECK(haveD == q, "every qualifying wave carries a delta profile (" << haveD << "/" << q << ")");
        CHECK(cmp > 0 && same == cmp, "no repaint: each wave's letters 6 bars after its end = at the end of the day (" << same << "/" << cmp << ")");
        // every wave's delta rows sum to the wave bars' total delta (rows aligned, nothing lost)
        bool sums = true;
        for (const wpc::Wave& w : E.waves) { if (w.provisional || w.drows.empty()) continue; long long a = 0, b = 0; for (long long x : w.drows) a += x;
            for (int k = w.i0; k <= w.i1; ++k) { for (auto& r : V[(size_t)k]) b += r.dlt; }
            if (a != b) sums = false; }
        CHECK(sums, "each wave's delta rows add up to its bars' VAP delta");
        // the drawing (Oct 30 afternoon)
        const int first = 165, last = 246; long long lo = LLONG_MAX, hi = LLONG_MIN; for (int i = first; i <= last; ++i) { lo = std::min(lo, B[(size_t)i].l); hi = std::max(hi, B[(size_t)i].h); }
        const double W = 1700, H = 900, bw = (W - 80) / (last - first + 3);
        wpc::View v; v.l = 0; v.t = 0; v.r = (int)W; v.b = (int)H; v.first = first; v.last = last; v.lastBar = last; v.bw = bw; v.tick = .25;
        v.X = [&](int i) { return 40 + (i - first + 0.5) * bw; }; v.Y = [&](double tk) { return H - 20 - (tk - (lo - 8)) / (double)(hi - lo + 16) * (H - 40); };
        v.textW = [](const std::string& s, bool) { return (int)s.size() * 7; };
        const wpc::Scene sc = wpc::layout(E, nullptr, v);
        CHECK(sc.lettersDrawn > 0 && sc.pbDrawn > 0 && sc.boxesDrawn > 0, "Oct 30 PM: letters, Pb and solid node boxes drawn (" << sc.lettersDrawn << " / " << sc.pbDrawn << " / " << sc.boxesDrawn << ")");
        // delta rows: right of each wave start, <= 30 px; the white delta POC box on the biggest row
        bool right = true; int drowsN = 0;
        for (const wpc::Wave& w : E.waves) { if (w.provisional || w.drows.empty() || w.i0 < first) continue; const int x0 = (int)std::lround(v.X(w.i0) - bw * 0.45);
            for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.back && p.x0 == x0 + 1 && (p.col & 0) == 0) { ++drowsN; if (p.x1 > x0 + 1 + wpc::SIDE_W) right = false; } }
        CHECK(drowsN > 0 && right, "delta rows point RIGHT from the wave start, at most 30 px (" << drowsN << " rows)");
        int boxW = 0; for (const wpc::Prim& p : sc.p) boxW += p.k == wpc::Prim::BOX && p.col == 0xE5E7EB;
        int front = 0, dots = 0; for (const wpc::Prim& p : sc.p) { front += p.k == wpc::Prim::BOX && p.col == 0xE5E7EB && !p.back; dots += p.k == wpc::Prim::LINE && p.col == 0xE5E7EB; }
        CHECK(boxW > 0 && front == sc.boxesDrawn && dots == 0, "the 2 biggest nodes' distributions in SOLID white boxes (no dotted lines) (" << front << " / " << sc.boxesDrawn << ")");
        bool atMost2 = true; for (const wpc::Wave& w : E.waves) if (!w.provisional) atMost2 = atMost2 && wpc::topDelta(w).size() <= 3;
        CHECK(atMost2, "(1.6.2) every wave: at most 3 boxed / classified nodes");
        // letters: the text sits over its row, centred, on a background patch, never more than the 30 px side + its width
        bool letOk = true; int lets = 0;
        for (size_t k = 0; k < sc.p.size(); ++k) { const wpc::Prim& p = sc.p[k]; if (p.k != wpc::Prim::TEXT || p.text.size() != 2 || p.text[1] != '?') continue; ++lets;
            if (k == 0 || sc.p[k - 1].k != wpc::Prim::FILL || sc.p[k - 1].col != 0x0B0F17 || p.back || !p.bold) letOk = false; }
        CHECK(lets == sc.lettersDrawn && letOk, "each letter is bold, front, on its own background patch (" << lets << ")");
        // Pb: text + the amber dashed line from the POC
        int pbT = 0, amber = 0; for (const wpc::Prim& p : sc.p) { pbT += p.k == wpc::Prim::TEXT && p.text == "Pb"; amber += p.k == wpc::Prim::LINE && p.col == 0xF5B428; }
        int cyanL = 0, cyanF = 0, greyL = 0, amberF = 0; for (const wpc::Prim& p : sc.p) { cyanL += p.k == wpc::Prim::LINE && p.col == 0x22D3EE; cyanF += p.k == wpc::Prim::FILL && p.col == 0x22D3EE && !p.back; greyL += p.k == wpc::Prim::LINE && p.col == 0x94A3B8; amberF += p.k == wpc::Prim::FILL && (p.col == 0xC88C14 || p.col == 0xB4BECD); }
        CHECK(pbT == sc.pbDrawn && cyanL > 0 && amber == 0, "(1.6.0) Pb text and the CYAN dotted POC line (" << pbT << ", " << cyanL << " dashes, amber " << amber << ")");
        CHECK(greyL == 0 && amberF == 0, "(1.6.0) no left volume profile: no grey outline lines (" << greyL << "), no HVN / amber WPOC fills (" << amberF << ")");
        CHECK(cyanF >= sc.wavesDrawn / 2 && cyanF > 0, "(1.6.0) a cyan WPOC line on the delta side per profiled wave (" << cyanF << " for " << sc.wavesDrawn << " waves)");
        // the volume side never crosses the wave start to the right; the delta side never crosses the next wave start
        std::cerr << "fixture: " << q << " waves (" << rev << " reversal, " << bo << " breakout, " << ra << " rally), letters " << sc.lettersDrawn << ", Pb " << sc.pbDrawn << ", boxes " << sc.boxesDrawn << "\n";
        // status text names the new rule
        const std::string st = wpc::statusText(E, "ES", 180, "native,ok", "loaded", "loaded", 0, &sc, false, 0);
        CHECK(st.find("QUALIFIED_BY,reversal") != std::string::npos && st.find("PROFILE,(1.5.0)") != std::string::npos, "status QUALIFIED_BY + PROFILE lines");
    }
    // ---- 5. the legacy rule still answers when pinned ----
    {   wpc::Engine E; CHECK(E.rule == 150, "default rule = 1.5.0"); }
    std::cout << "test_wavepoc_150: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
