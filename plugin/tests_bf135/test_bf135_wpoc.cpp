// test_bf135_wpoc.cpp -- lsBookFlow 1.35.0 / BookLevelBox 1.1.0 (2026-11-04, Rassul "add wave poc as one of the levels for bookflow"):
//   PARITY of the new zone source: the WPOC levels BookFlow builds from lsWavePOC's RECORD (blb::WpFeed + blb::wpocLines) == the wave POC
//   lines lsWavePOC itself draws (wpc::drawPocLines on its own engine), at EVERY closed bar of the real ES 10-29 / 10-30 fixture; plus
//   hand-made rules (replaced leg, other N, trading day, tested + newer, 2 nearest a side, torn lines), level names / zones.
// Build (tests_bf135/): g++ -std=c++17 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I.. test_bf135_wpoc.cpp -o tw
//        && ./tw ../tests_wavepoc/wavepoc_es1029_1030_delta.csv
#include "../WavePOCLogic.h"
#include "../BookLevelBox.h"
#include <fstream>
#include <iostream>
#include <set>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)
typedef std::multiset<std::pair<long long, int> > LSet;   // (tick, open)
static LSet engineLines(const wpc::Engine& E, int lastBar) {
    wpc::View v; v.l = 0; v.t = 0; v.r = 2000000000; v.b = 2000000000; v.first = 0; v.last = lastBar; v.lastBar = lastBar; v.bw = 100; v.tick = .25;
    v.X = [](int i) { return 1000.0 + i * 100.0; }; v.Y = [](double tk) { return tk - 25000.0; };   // (wpc::ci clamps px to +-30000)
    std::vector<const wpc::Wave*> ws; for (const wpc::Wave& w : E.waves) if (!w.pending && !w.provisional) ws.push_back(&w);
    wpc::Scene s; wpc::Colours C; wpc::drawPocLines(s, E, v, C, ws);
    LSet out; for (const wpc::Prim& p : s.p) if (p.k == wpc::Prim::LINE) out.insert(std::make_pair((long long)p.y0 + 25000, p.col == C.cyan ? 1 : 0));
    return out;
}
static LSet mine(const blb::WpFeed& F, const std::vector<blb::WpBar>& bars, int n) {
    LSet out; for (const blb::WpLine& l : blb::wpocLines(F.waves(), bars, n)) out.insert(std::make_pair(l.tick, l.open ? 1 : 0)); return out;
}
static blb::WpRec rec(int dir, long long t0, long long t1, long long tc, long long poc, int n = 3) { blb::WpRec r; r.n = n; r.dir = dir; r.t0 = t0; r.t1 = t1; r.tConf = tc; r.havePoc = true; r.poc = poc; static long long s = 0; r.seq = ++s; return r; }
int main(int argc, char** argv) {
    // ---- 1. the real fixture: record -> levels == the engine's drawn lines, every closed bar ----
    if (argc > 1) {
        std::ifstream f(argv[1]); std::string line; std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow> > V;
        while (std::getline(f, line)) { if (line.empty() || line[0] == '#') continue; auto x = wpc::splitCsv(line);
            if (x[0] == "B") { wpc::Bar b; b.t = atoll(x[2].c_str()); wpc::toTicks(atof(x[3].c_str()), .25, b.o); wpc::toTicks(atof(x[4].c_str()), .25, b.h); wpc::toTicks(atof(x[5].c_str()), .25, b.l); wpc::toTicks(atof(x[6].c_str()), .25, b.c); B.push_back(b); V.resize(B.size()); }
            else { wpc::VRow r; wpc::toTicks(atof(x[2].c_str()), .25, r.tick); r.vol = atoll(x[3].c_str()); r.dlt = atoll(x[4].c_str()); V[(size_t)atoi(x[1].c_str())].push_back(r); } }
        CHECK(B.size() == 247, "fixture 247 bars");
        wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = V[(size_t)i]; return 1; };
        wpc::Store S; wpc::Engine E; E.store = &S;
        blb::WpFeed F; std::vector<blb::WpBar> bars; std::string recText = std::string(wpc::REC_HEAD) + "\n";
        int same = 0, cmp = 0, nonEmpty = 0, open = 0, tested = 0, firstBad = -1, levelsMax = 0;
        for (size_t i = 0; i < B.size(); ++i) {
            E.addBar(B[i], vap);
            for (const std::string& ln : E.out) { recText += ln + "\n"; F.line(ln, .25); }
            E.out.clear();
            blb::WpBar b; b.t = B[i].t; b.h = B[i].h; b.l = B[i].l; b.c = B[i].c; bars.push_back(b);
            const LSet a = engineLines(E, (int)i), m = mine(F, bars, F.nNewest);
            ++cmp; if (a == m) ++same; else if (firstBad < 0) firstBad = (int)i;
            if (!a.empty()) ++nonEmpty;
            for (auto& x : a) (x.second ? open : tested) += 1;
            levelsMax = std::max(levelsMax, (int)blb::wpocLevels(blb::wpocLines(F.waves(), bars, F.nNewest), .25).size());
        }
        CHECK(same == cmp, "PARITY: record -> wpocLines == wpc::drawPocLines at every closed bar (" << same << " / " << cmp << ", first diff bar " << firstBad << ")");
        CHECK(nonEmpty > 100 && open > 100 && tested > 20, "the comparison is not empty: bars with lines " << nonEmpty << ", open line-bars " << open << ", tested line-bars " << tested);
        CHECK(F.bad == 0 && F.nNewest == 3, "the engine's own record parses clean (bad " << F.bad << "), N 3");
        // the same, reading the record TEXT back in arbitrary chunks (the host's incremental read, torn last line held back)
        blb::WpFeed G; std::string pend;
        for (size_t k = 0; k < recText.size(); k += 37) { pend += recText.substr(k, 37); size_t nl; while ((nl = pend.find('\n')) != std::string::npos) { G.line(pend.substr(0, nl), .25); pend.erase(0, nl + 1); } }
        CHECK(mine(G, bars, G.nNewest) == engineLines(E, (int)B.size() - 1) && G.w.size() == F.w.size(), "chunked read (37-byte pieces) = the same waves and lines");
        CHECK(levelsMax >= 1 && levelsMax <= 2 * blb::WP_EACH_SIDE + 8, "levels per bar bounded (max " << levelsMax << ")");
        std::cout << "fixture: " << same << "/" << cmp << " bars identical, " << nonEmpty << " bars with lines, max levels " << levelsMax << ", waves " << F.w.size() << "\n";
    }
    // ---- 2. hand-made rules ----
    const long long D = 1793700000;   // a Central wall-clock second; tday(D) is the day below
    auto bar = [](long long t, long long h, long long l, long long c) { blb::WpBar b; b.t = t; b.h = h; b.l = l; b.c = c; return b; };
    {   std::vector<blb::WpBar> bars; for (int k = 0; k < 10; ++k) bars.push_back(bar(D + 180 * k, 1100, 1090, 1095));
        std::vector<blb::WpRec> R = { rec(1, D, D + 360, D + 540, 1200), rec(1, D + 180, D + 540, D + 720, 1210), rec(1, D + 360, D + 720, D + 900, 1220),
                                      rec(-1, D, D + 360, D + 540, 1000), rec(-1, D + 180, D + 540, D + 720, 1010), rec(-1, D + 360, D + 720, D + 900, 990) };
        auto L = blb::wpocLines(R, bars, 3);
        std::set<long long> t; for (auto& l : L) t.insert(l.tick);
        CHECK(L.size() == 4 && t.count(1200) && t.count(1210) && t.count(1010) && t.count(1000) && !t.count(1220) && !t.count(990), "only the 2 nearest above and the 2 nearest below the last close");
        R.push_back(rec(1, D, D + 900, D + 1080, 1150));   // the leg from D extended: replaces the D..D+360 wave (1200)
        auto L2 = blb::wpocLines(R, bars, 3); std::set<long long> t2; for (auto& l : L2) t2.insert(l.tick);
        CHECK(t2.count(1150) && !t2.count(1200) && t2.count(1210), "an extended leg (same start / direction, later end) replaces the earlier wave");
        std::vector<blb::WpBar> early(bars.begin(), bars.begin() + 5);   // last bar D+720: the extension (tConf D+1080) and D+900 waves not confirmed yet
        auto L3 = blb::wpocLines(R, early, 3); std::set<long long> t3; for (auto& l : L3) t3.insert(l.tick);
        CHECK(t3.count(1200) && !t3.count(1150), "a wave counts only from its confirmation bar (no look-ahead)");
        CHECK(blb::wpocLines(R, bars, 4).empty(), "another reversal-bars N: none");
    }
    {   // tested: ends at the bar that traded through; shown until a newer wave of the same direction
        std::vector<blb::WpBar> bars; for (int k = 0; k < 6; ++k) bars.push_back(bar(D + 180 * k, 1100, 1090, 1095));
        bars[4] = bar(D + 720, 1121, 1095, 1110);   // trades through 1120
        std::vector<blb::WpRec> R = { rec(1, D - 900, D + 180, D + 360, 1120) };
        auto L = blb::wpocLines(R, bars, 3);
        CHECK(L.size() == 1 && !L[0].open && L[0].hit == 4, "tested line = drawn to the bar that traded through (open " << (L.empty() ? -1 : L[0].open) << ")");
        R.push_back(rec(-1, D + 180, D + 540, D + 720, 1050));
        CHECK(blb::wpocLines(R, bars, 3).size() == 2, "a newer wave of the OTHER direction keeps the tested line");
        R.push_back(rec(1, D + 360, D + 720, D + 900, 1060)); bars.push_back(bar(D + 900, 1100, 1090, 1095));
        auto L4 = blb::wpocLines(R, bars, 3); bool has = false; for (auto& l : L4) has = has || l.tick == 1120;
        CHECK(!has, "a newer wave of the same direction hides the tested line");
        std::vector<blb::WpBar> b2; for (int k = 0; k < 4; ++k) b2.push_back(bar(D + 180 * k, 1100, 1090, 1095));
        b2[1] = bar(D + 180, 1125, 1090, 1095);   // the wave's own extreme bar (t <= t1) touches: not a test
        CHECK(blb::wpocLines({ rec(1, D - 900, D + 180, D + 360, 1120) }, b2, 3).at(0).open, "the extreme bar itself never tests its POC");
    }
    {   // trading day: 17:00 CT starts the next day; a POC never carries into the next session
        const long long d0 = (blb::wpDay(D) + 1) * 86400 - 7 * 3600;   // the first second of the next CME day (bar END 17:00 CT = wpc::tday's next day)
        bool eq = true; for (long long t = d0 - 400; t <= d0 + 400; t += 20) eq = eq && blb::wpDay(t) == wpc::tday(t);
        CHECK(eq && blb::wpDay(d0) == blb::wpDay(d0 - 1) + 1 && blb::wpDay(D) == wpc::tday(D), "wpDay = wpc::tday around the 17:00 CT break");
        std::vector<blb::WpBar> bars = { bar(d0 - 540, 1100, 1090, 1095), bar(d0 - 360, 1100, 1090, 1095), bar(d0 - 180, 1100, 1090, 1095) };
        std::vector<blb::WpRec> R = { rec(1, d0 - 1200, d0 - 540, d0 - 360, 1200) };
        CHECK(blb::wpocLines(R, bars, 3).size() == 1, "same trading day: kept");
        bars.push_back(bar(d0, 1100, 1090, 1095));
        CHECK(blb::wpocLines(R, bars, 3).empty(), "the next trading day: gone");
    }
    {   // the record parser: torn / garbled / other lines; levels: names, zones, duplicates
        blb::WpFeed F;
        F.line("WAVE,1.8.3,3,up,100,200,300,a,b,c,1,2,3,4,5,1,9,1,0,WAVE,0,BOTH,4.0", .25);
        F.line("NODE,100,200,1,7001.00,5,2.0,WAVE", .25);
        F.line("NODE,100,200,0,7000.25,9,4.0,WAVE", .25);
        F.line("ROWS,100,200,6999.00,1,1;2;3", .25); F.line("STOP,100,200,7000.25,touch,400,x,WAVE", .25);
        F.line("WAVE,1.8.3,3,up,100", .25); F.line("NODE,1,2,0,abc,1,1", .25); F.line("NODE,9,9,0,7000.00,1,1", .25);
        F.line("WAVE,1.8.3,3,sideways,1,2,3,a,b,c,1,2,3,4,5,1,9,1,0", .25); F.line("WAVE,1.8.3,3,up,300,200,400,a,b,c,1,2,3,4,5,1,9,1,0", .25);
        CHECK(F.w.size() == 1 && F.w.begin()->second.havePoc && F.w.begin()->second.poc == 28001 && F.bad == 4, "parser: rank-0 POC only, torn / garbled lines counted (" << F.bad << "), unknown wave's NODE ignored");
        std::vector<blb::WpLine> L(3); L[0].tick = 28001; L[1].tick = 28001; L[2].tick = 28100;
        auto V = blb::wpocLevels(L, .25);
        CHECK(V.size() == 2 && V[0].name == "WPOC 7000.25" && V[0].type == blb::L_WPOC && V[0].lo == 28001 - blb::HALF && V[0].hi == 28001 + blb::HALF && !V[0].fixedZone && V[1].name == "WPOC 7025.00",
              "levels: 'WPOC <price>', zone +-4 ticks, one per price");
        CHECK(std::string(blb::ltypeName(blb::L_WPOC)) == "wave_poc" && blb::L_TYPES == 6 && std::string(blb::VERSION) == "1.1.0", "ltype 'wave_poc' (the nightly's), 6 types, BookLevelBox 1.1.0");
        CHECK(blb::wpPx(28001, .25) == "7000.25" && blb::wpPx(1234, 0.01) == "12.34" && blb::wpPx(7, 1.0) == "7" && blb::wpPx(2, 0.005) == "0.010", "price text by tick decimals");
        F.prune(250); CHECK(F.w.empty(), "prune drops waves ended before the cut");
    }
    std::cout << "BookFlow 1.35.0 WPOC levels: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
