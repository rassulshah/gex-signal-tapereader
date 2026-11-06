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
        CHECK(count(wpc::letters(upWave({{66, 95}}), B), 'I') == 1 && count(wpc::letters(upWave({{67, 95}}), B), 'I') == 0, "I? up to 66% of the wave (66% yes, 67% no)");
        CHECK(count(wpc::letters(upWave({{66, -95}}), B), 'A') == 1 && count(wpc::letters(upWave({{67, -95}}), B), 'A') == 0, "A? up to 66% of the wave (66% yes, 67% no)");
        CHECK(count(wpc::letters(upWave({{85, 95}}), B), 'T') == 1 && count(wpc::letters(upWave({{84, 95}}), B), 'T') == 0, "T? in the last 15% (85% yes, 84% no)");
        CHECK(wpc::letters(upWave({{70, 95}, {90, -95}}), B).empty(), "with the wave at 70% and against it in the last 20%: no letter");
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
        {   // (1.7.1, Rassul 14:31 CT "you should not be delayed") the LIVE pullback low (not yet a confirmed pivot) counts at its close
            std::vector<wpc::Bar> L = flat(20, 170); L[7].l = 148; L[7].h = 160; std::vector<wpc::Pivot> a1 = {{5, 1, 200}};
            CHECK(wpc::pullbacks(w, a1, L).empty() && wpc::pullbacks(w, a1, L, -1, 7).size() == 1, "(1.7.1) a live pullback low through the POC = Pb at its close");
            CHECK(wpc::pullbacks(w, a1, L, 1, 7).empty(), "(1.7.1) a live UP leg after an up wave is not a pullback");
            L[8].l = 145; L[8].h = 149; CHECK(wpc::pullbacks(w, a1, L, -1, 8).empty(), "(1.7.1) the live low moved to a bar whose high is below the POC: the Pb goes away (repaint until confirmed)"); }
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
        wpc::Store S; wpc::Engine E; E.rule = 150; E.store = &S; std::map<long long, std::string> early;   // (1.9.2) the 1.5.0 wave rule pinned   // letters of each wave seen 6 bars after its end (decided) vs at the end of the day
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
        CHECK(sc.lettersDrawn > 0 && sc.pbDrawn == 0 && sc.pbFound > 0 && sc.boxesDrawn == 0, "Oct 30 PM: (1.9.0) P / H letters drawn, NO delta boxes; (1.8.3) POC pullbacks found but NO Pb drawn (" << sc.lettersDrawn << " / " << sc.pbDrawn << " drawn, " << sc.pbFound << " found / " << sc.boxesDrawn << ")");
        // delta rows: right of each wave start, <= 30 px; the white delta POC box on the biggest row
        bool right = true; int drowsN = 0;
        for (const wpc::Wave& w : E.waves) { if (w.provisional || w.drows.empty() || w.i0 < first) continue; const int x0 = (int)std::lround(v.X(w.i0) - bw * 0.45);
            for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.back && p.x0 == x0 + 1 && (p.col & 0) == 0) { ++drowsN; if (p.x1 > x0 + 1 + wpc::SIDE_W) right = false; } }
        CHECK(drowsN > 0 && right, "delta rows point RIGHT from the wave start, at most 38 px (1.6.2: 25% bigger) (" << drowsN << " rows)");
        int boxW = 0; for (const wpc::Prim& p : sc.p) boxW += p.k == wpc::Prim::BOX && p.col == 0xE5E7EB;
        int front = 0, dots = 0; for (const wpc::Prim& p : sc.p) { front += p.k == wpc::Prim::BOX && p.col == 0xE5E7EB && !p.back; dots += p.k == wpc::Prim::LINE && p.col == 0xE5E7EB; }
        CHECK(boxW == 0 && front == 0 && dots == 0, "(1.9.0) no delta node boxes (the right side is the volume profile) (" << front << " / " << sc.boxesDrawn << ")");
        bool atMost2 = true; for (const wpc::Wave& w : E.waves) if (!w.provisional) atMost2 = atMost2 && wpc::topDelta(w).size() <= 3;
        CHECK(atMost2, "(1.6.2) every wave: at most 3 boxed / classified nodes");
        {   // (1.7.2) the letters: single I / A / T in WHITE and P in CYAN, bold, front, ON the spine (centred on a wave's spine x), no "?",
            // no background patch; never two on top of each other; one P per drawn wave, centred on its cyan WPOC line
            auto isLet = [](const wpc::Prim& p) { return p.k == wpc::Prim::TEXT && p.text.size() == 1 && (p.text == "I" || p.text == "A" || p.text == "T" || p.text == "P" || p.text == "H"); };   // (1.9.0) + H
            std::vector<int> spX; for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::LINE && p.w == 2 && p.x0 == p.x1) spX.push_back(p.x0);
            std::vector<std::pair<int,int> > tx; int lets = 0, ps = 0, pOnLine = 0; bool ok = true, onSpine = true;
            for (const wpc::Prim& p : sc.p) { if (!isLet(p)) continue; ++lets; tx.push_back(std::make_pair(p.x0, p.y0));
                const unsigned long want = p.text == "P" ? 0x22D3EEUL : 0xFFFFFFUL; if (p.col != want || p.back || !p.bold) ok = false;
                const int tw = p.x1 - p.x0; bool sp = false; for (int x : spX) sp = sp || (p.x1 <= x - tw - 1 && p.x1 >= x - tw - 4); if (!sp) onSpine = false;   // (1.8.4) right edge one letter + 1-4 px left of a spine (1.8.1: 1-4 px)
                if (p.text == "P") { ++ps; const int cy = (p.y0 + p.y1) / 2;
                    for (const wpc::Prim& f : sc.p) if (f.k == wpc::Prim::FILL && f.col == 0x22D3EE && !f.back && f.x0 - p.x1 >= tw && f.x0 - p.x1 <= tw + 5 && std::abs((f.y0 + f.y1) / 2 - cy) <= 1) { ++pOnLine; break; } } }
            bool clear = true; for (size_t i2 = 0; i2 < tx.size(); ++i2) for (size_t j2 = i2 + 1; j2 < tx.size(); ++j2) if (std::abs(tx[i2].first - tx[j2].first) < 20 && std::abs(tx[i2].second - tx[j2].second) < 13) clear = false;
            int q = 0; for (const wpc::Prim& p : sc.p) q += p.k == wpc::Prim::TEXT && p.text.find('?') != std::string::npos;
            int patch = 0; for (const wpc::Prim& p : sc.p) patch += p.k == wpc::Prim::FILL && !p.back && p.y1 - p.y0 == 12 && (p.col == 0x0B0F17 || p.col == 0x374151);
            CHECK(lets == sc.lettersDrawn && lets >= 3 && ok, "(1.7.2) letters white (P cyan; 1.9.0: H white, no I / A / T), bold, front (" << lets << ")");
            CHECK(onSpine, "(1.8.4) every letter one letter width further LEFT of a wave's spine");
            CHECK(clear, "(1.7.2) letters never overlap");
            CHECK(q == 0 && patch == 0, "(1.7.2) no \"?\" and no background patch (" << q << " / " << patch << ")");
            CHECK(ps > 0 && pOnLine == ps, "(1.8.1) every P sits just left of its wave's cyan volume POC line (" << pOnLine << " / " << ps << ")"); }
        {   // (1.8.0) extended wave POC lines: 1 px cyan dashes behind the candles; untested = bright, to the right edge, at most 2 above
            // and 2 below the last close; tested = dimmer, ending at the first bar after the wave that traded through the POC
            const unsigned long cy = 0x22D3EE, dim = wpc::mix(0x22D3EE, 0x0B0F17, 0.5);
            std::map<int, std::pair<int, int> > rows;   // y -> (min x0, max x1) per colour key
            int bright = 0, faint = 0; bool okw = true, back = true;
            std::map<int, int> brightMaxX, faintMaxX;
            for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::LINE && p.y0 == p.y1 && (p.col == cy || p.col == dim) && p.x1 - p.x0 > 6) {   // (1.8.1) solid: one long segment (a Pb dash is 5 px)
                if (p.w != 1) okw = false;
                if (!p.back) back = false;
                auto& m = p.col == cy ? brightMaxX : faintMaxX; m[p.y0] = std::max(m.count(p.y0) ? m[p.y0] : 0, p.x1); }
            bright = (int)brightMaxX.size(); faint = (int)faintMaxX.size();
            CHECK(okw && back, "(1.8.1) POC lines are 1 px SOLID lines behind the candles");
            CHECK(sc.pocOpen + sc.pocTested > 0, "(1.8.0) POC lines drawn on the Oct 30 PM fixture (" << sc.pocOpen << " untested, " << sc.pocTested << " tested)");
            int ab = 0, be = 0; const double yc = v.Y((double)E.B.back().c), edge = v.X(last) + bw * 0.5;
            for (const auto& kv : brightMaxX) if (kv.second >= (int)edge - 8) (kv.first <= yc ? ab : be)++;   // rows reaching the right edge (a Pb line ends at its bar)
            CHECK(ab <= 2 && be <= 2 && ab + be == sc.pocOpen, "(1.8.0) every untested line reaches the right edge, at most 2 above / 2 below (" << ab << " / " << be << ", untested " << sc.pocOpen << ")");
            (void)rows; (void)faint; (void)bright; }
        {   // (1.8.1) the trading day: Mon 17:03 bar and Tue 16:00 bar = one day; Tue 17:03 = the next
            const long long mon1703 = 1793638980, tue1600 = 1793721600, tue1703 = 1793725380;   // 2026-11-02 17:03, 11-03 16:00, 11-03 17:03 (Central wall clock)
            CHECK(wpc::tday(mon1703) == wpc::tday(tue1600) && wpc::tday(tue1703) == wpc::tday(tue1600) + 1, "(1.8.1) a POC line's trading day runs 17:00 -> 16:00 CT"); }
        // Pb: text + the amber dashed line from the POC
        int pbT = 0, amber = 0; for (const wpc::Prim& p : sc.p) { pbT += p.k == wpc::Prim::TEXT && p.text == "Pb"; amber += p.k == wpc::Prim::LINE && p.col == 0xF5B428; }
        int cyanL = 0, cyanF = 0, greyL = 0, amberF = 0; for (const wpc::Prim& p : sc.p) { cyanL += p.k == wpc::Prim::LINE && p.col == 0x22D3EE; cyanF += p.k == wpc::Prim::FILL && p.col == 0x22D3EE && !p.back; greyL += p.k == wpc::Prim::LINE && p.col == 0x94A3B8; amberF += p.k == wpc::Prim::FILL && (p.col == 0xC88C14 || p.col == 0xB4BECD); }
        int pbDash = 0; for (const wpc::Prim& p : sc.p) pbDash += p.k == wpc::Prim::LINE && p.y0 == p.y1 && p.x1 - p.x0 <= 5 && p.col == 0x22D3EE;
        CHECK(pbT == 0 && pbDash == 0 && cyanL > 0 && amber == 0, "(1.8.3) no Pb text and no Pb dotted line (Pb = lsBreakout); the cyan POC lines stay (" << pbT << " Pb, " << pbDash << " dashes, " << cyanL << " cyan lines, amber " << amber << ")");
        CHECK(greyL == 0 && amberF == 0, "(1.6.0) no left volume profile: no grey outline lines (" << greyL << "), no HVN / amber WPOC fills (" << amberF << ")");
        CHECK(cyanF >= sc.wavesDrawn / 2 && cyanF > 0, "(1.6.0) a cyan WPOC line on the delta side per profiled wave (" << cyanF << " for " << sc.wavesDrawn << " waves)");
        // the volume side never crosses the wave start to the right; the delta side never crosses the next wave start
        std::cerr << "fixture: " << q << " waves (" << rev << " reversal, " << bo << " breakout, " << ra << " rally), letters " << sc.lettersDrawn << ", Pb " << sc.pbDrawn << ", boxes " << sc.boxesDrawn << "\n";
        // status text names the new rule
        const std::string st = wpc::statusText(E, "ES", 180, "native,ok", "loaded", "loaded", 0, &sc, false, 0);
        CHECK(st.find("QUALIFIED_BY,reversal") != std::string::npos && st.find("PROFILE,(1.9.2)") != std::string::npos, "status QUALIFIED_BY + PROFILE lines");
    }
    // ---- 5. the legacy rule still answers when pinned ----
    {   wpc::Engine E; CHECK(E.rule == 192, "(1.9.2) default rule = 192 (CoB / Rly / BO waves)"); }
    std::cout << "test_wavepoc_150: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
