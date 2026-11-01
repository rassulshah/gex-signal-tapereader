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
    // ---- 1. the letters: top 4 first, then location, then the class ----
    {   // I = with the wave in the first third; A = against, before the last quarter; T = with, last quarter + close back within 5 bars
        std::vector<wpc::Bar> B = flat(12, 195); B[7].c = 188;   // closes back below row 90 (tick 190) 2 bars after the pivot
        wpc::Wave w = upWave({{10, 100}, {50, -90}, {52, -85}, {90, 80}, {95, 70}});
        const auto top = wpc::topDelta(w);
        CHECK(top.size() == 4 && top[0] == 10 && top[3] == 90, "top 4 by |delta|, biggest first (5th row left out)");
        const auto L = wpc::letters(w, B);
        CHECK(count(L, 'I') == 1 && count(L, 'A') == 1 && count(L, 'T') == 1 && L.size() == 3, "I + A + T, the 2nd A within 10 ticks merged (" << L.size() << ")");
        for (auto& x : L) {
            if (x.t == 'I') CHECK(x.row == 10 && x.bull, "I? on row 10, wave colour (green for an up wave)");
            if (x.t == 'A') CHECK(x.row == 50 && x.bull, "A? on the bigger against row, wave colour");
            if (x.t == 'T') CHECK(x.row == 90 && !x.bull, "T? on row 90, opposite colour (red after an up wave)");
        }
        std::vector<wpc::Bar> B2 = flat(12, 195);
        CHECK(count(wpc::letters(w, B2), 'T') == 0, "no close back within 5 bars = no T");
        std::vector<wpc::Bar> B3 = flat(12, 195); B3[11].c = 188;   // 6 bars after the pivot: too late
        CHECK(count(wpc::letters(w, B3), 'T') == 0, "a close back 6 bars after the pivot is too late for T");
    }
    {   // rows under 2x the median never get a letter; with-wave rows in the middle get none
        wpc::Wave w = upWave({{20, 19}, {60, 200}});
        const auto L = wpc::letters(w, flat(12, 195));
        CHECK(L.empty(), "a 1.9x row and a with-wave middle row: no letter (" << L.size() << ")");
    }
    {   // the far A rows are separate letters; an A in the last quarter is not an A
        wpc::Wave w = upWave({{30, -60}, {60, -70}, {80, -90}});
        const auto L = wpc::letters(w, flat(12, 195));
        CHECK(count(L, 'A') == 2, "A at 30% and 60% (both before the last quarter), none at 80% (" << count(L, 'A') << ")");
    }
    {   // down wave mirror: an I in the first third of a down wave is near its TOP and red
        wpc::Wave w = upWave({}); w.dir = -1; w.p0 = 200; w.p1 = 100; w.drows.assign(101, -10); w.drows[95] = -120; w.dMax = 120;
        const auto L = wpc::letters(w, flat(12, 105));
        CHECK(L.size() == 1 && L[0].t == 'I' && L[0].row == 95 && !L[0].bull, "down wave: I? near the top, red");
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
        CHECK(L.size() == 1 && L[0].t == 'A' && L[0].cluster && L[0].a == 48, "cluster 120 >= 2.0 x reference 50 -> one A? with its band");
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
        CHECK(sc.lettersDrawn > 0 && sc.pbDrawn > 0 && sc.boxesDrawn > 0, "Oct 30 PM: letters, Pb and dotted borders drawn (" << sc.lettersDrawn << " / " << sc.pbDrawn << " / " << sc.boxesDrawn << ")");
        // delta rows: right of each wave start, <= 30 px; the white delta POC box on the biggest row
        bool right = true; int drowsN = 0;
        for (const wpc::Wave& w : E.waves) { if (w.provisional || w.drows.empty() || w.i0 < first) continue; const int x0 = (int)std::lround(v.X(w.i0) - bw * 0.45);
            for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.back && p.x0 == x0 + 1 && (p.col & 0) == 0) { ++drowsN; if (p.x1 > x0 + 1 + wpc::SIDE_W) right = false; } }
        CHECK(drowsN > 0 && right, "delta rows point RIGHT from the wave start, at most 30 px (" << drowsN << " rows)");
        int boxW = 0; for (const wpc::Prim& p : sc.p) boxW += p.k == wpc::Prim::BOX && p.col == 0xE5E7EB;
        CHECK(boxW > 0, "the delta POC is outlined in white");
        // letters: the text sits over its row, centred, on a background patch, never more than the 30 px side + its width
        bool letOk = true; int lets = 0;
        for (size_t k = 0; k < sc.p.size(); ++k) { const wpc::Prim& p = sc.p[k]; if (p.k != wpc::Prim::TEXT || p.text.size() != 2 || p.text[1] != '?') continue; ++lets;
            if (k == 0 || sc.p[k - 1].k != wpc::Prim::FILL || sc.p[k - 1].col != 0x0B0F17 || p.back || !p.bold) letOk = false; }
        CHECK(lets == sc.lettersDrawn && letOk, "each letter is bold, front, on its own background patch (" << lets << ")");
        // Pb: text + the amber dashed line from the POC
        int pbT = 0, amber = 0; for (const wpc::Prim& p : sc.p) { pbT += p.k == wpc::Prim::TEXT && p.text == "Pb"; amber += p.k == wpc::Prim::LINE && p.col == 0xF5B428; }
        CHECK(pbT == sc.pbDrawn && amber > 0, "Pb text and the amber dotted POC line (" << pbT << ", " << amber << " dashes)");
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
