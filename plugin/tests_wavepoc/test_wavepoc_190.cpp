// test_wavepoc_190.cpp - lsWavePOC 1.9.0 (2026-11-05 08:13-08:46 CT, Rassul: "draw a volume profile instead of the delta profile ... upto 2
// hvns and the poc ... the other two hvn should be below the breakout high and if there is no breakout, it should be below the bar that broke
// the prior high pivot", "display the poc wherever it may be and then look for 2 hvn's at the break level or lower", "just show the poc line",
// "now"). SDK-free: WavePOCLogic.h + BreakoutLogic.h (the host runs bko::Engine for the BO candles, as here).
//   g++ -std=c++17 -Wall -Wextra -Werror -I.. test_wavepoc_190.cpp -o t190 && ./t190 wavepoc_es1029_1030_delta.csv
//   [1] the rule on a made-up wave: HVNs at / below the break level only, max 2, biggest first, never the POC; the POC anywhere; down mirror
//   [2] the break level: BO / BD candle of the wave's direction inside the wave > the bar that broke the prior pivot > the wave extreme
//   [3] the real fixture (ES Oct 29-30 RTH, 3-min + VAP) with lsBreakout's own BO / BD candles: every HVN on the right side of its wave's
//       break level, each >= 2x the median row, <= 2 per wave, never the POC row; HVNs of a finished wave unchanged at the end of the day
//   [4] the drawing: volume rows right of the spine (<= 38 px), POC row cyan, HVN rows in the wave colour, letters only P / H, no boxes
//   [1b] (1.9.1) HVNs = the top 2 nodes in their own distributions inside the zone, no 2x floor (NQ Nov 6 case)
//   [5] versions 1.9.1
#include "../WavePOCLogic.h"
#include "../BreakoutLogic.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
static int passed = 0, failed = 0;
#define CHECK(c, t) do { if (c) ++passed; else { ++failed; std::cerr << "FAIL: " << t << " (line " << __LINE__ << ")\n"; } } while (0)

static wpc::Wave mk(int dir, const std::map<int, long long>& rows) {   // 1-tick rows 1000.., 10 lots each + the given rows
    wpc::Wave w; w.dir = dir; w.e0 = 1000; w.rowTicks = 1; w.rows.assign(100, 10);
    for (auto& kv : rows) w.rows[(size_t)kv.first] = kv.second;
    w.med2 = wpc::median2(w.rows); w.maxVol = 0; for (long long v : w.rows) w.maxVol = std::max(w.maxVol, v);
    const std::vector<int> pk = wpc::pickNodes(w.rows, w.med2);
    for (size_t r = 0; r < pk.size(); ++r) { wpc::Node nd; nd.row = pk[r]; nd.rank = (int)r; nd.vol = w.rows[(size_t)pk[r]]; nd.tick = w.e0 + pk[r]; w.nodes.push_back(nd); }
    return w;
}
static std::string slurp(const std::string& f) { std::ifstream i(f); std::stringstream s; s << i.rdbuf(); return s.str(); }

int main(int argc, char** argv) {
    // ---- [1] the HVN rule ----
    {   // up wave: POC at row 80 (above the cut 60), nodes at 70 (above, big), 50, 30, 10 (below)
        wpc::Wave w = mk(1, {{80, 200}, {70, 150}, {50, 120}, {30, 90}, {10, 60}});
        std::vector<int> h = wpc::hvnRows(w, 1060);
        CHECK(h.size() == 2 && h[0] == 50 && h[1] == 30, "up: the 2 biggest HVNs AT / BELOW the break level (row 70 above it skipped; POC row 80 never one) (" << h.size() << ")");
        CHECK(w.nodes[0].row == 80, "up: the POC stays where the most volume is (above the break level)");
        h = wpc::hvnRows(w, 1050); CHECK(h.size() == 2 && h[0] == 50 && h[1] == 30, "up: a node exactly AT the break level counts");
        h = wpc::hvnRows(w, 1049); CHECK(h.size() == 2 && h[0] == 30 && h[1] == 10, "up: one tick lower and row 50 is out");
        h = wpc::hvnRows(w, 1005); CHECK(h.empty(), "up: nothing at / below a low break level -> no HVN (the POC still shows)");
        wpc::Wave d = mk(-1, {{20, 200}, {30, 150}, {50, 120}, {70, 90}, {90, 60}});
        h = wpc::hvnRows(d, 1040); CHECK(h.size() == 2 && h[0] == 50 && h[1] == 70, "down: the 2 biggest HVNs AT / ABOVE the break level (mirror)");
        wpc::Wave f = mk(1, {{40, 15}}); { const std::vector<int> hf = wpc::hvnRows(f, 2000); CHECK(hf.size() == 1 && hf[0] == 40, "(1.9.1) a bump under 2x the median row, in its own distribution, IS an HVN now (no 2x floor)"); }
        wpc::Wave g = mk(1, {}); CHECK(wpc::hvnRows(g, 2000).empty(), "(1.9.1) flat rows (nothing above the median row) give no HVN");
        {   // (1.9.1) Rassul 13:00 "the top 2 nodes above the breakdown low": NQ Nov 6 11:24-11:54 down wave (rows from the record), BD low 27,154.50
            wpc::Wave q; q.dir = -1; q.rowTicks = 3; q.e0 = 107952; std::istringstream in("4 3 12 13 16 41 29 36 76 86 46 24 25 14 9 11 185 60 50 39 50 31 62 110 95 74 101 72 106 130 187 152 155 178 153 284 169 158 193 143 153 194 166 139 102 100 88 112 170 145 122 111 120 114 133 134 218 215 227 228 228 274 355 252 335 324 270 202 200 187 134 114 84 74 110 118 110 116 151 185 223 158 600 278 246 267 244 259 220 231 200 195 151 133 162 154 367 155 194 210 195 248 222 214 191 161 447 277 312 348 307 259 303 307 252 274 313 220 200 166 213 182 236 198 193 201 244 240 237 260 280 233 203 318 276 233 433 302 283 269 290 197 197 149 159 139 182 170 134 216 176 203 216 172 170 143 126 84 143 90 100 95 83 63 47 57 50 39 71 77 52 45 29 32 39 53 58 28 25 26 27 28 33 24 36 21 33 19 23 197 19 30 23 19 46 30 42 26 17 22 20 19 31 52 64 40 38 15 36 14 67 11 10 12 9 9 25 11 19 25 33 35 49 47 67 95 121 145 153 168 184 174 197 177 149 136 123 162 137 115 93 69 65 46 40 88 112 106 109 85 88 104 88 72 80 72 92 86 67 39 52 45 62 84 88 99 114 66 62 53 82 102 71 89 74 57 45 31 19 21 36 35 53 28 32 44 29 37 22 23 23 32 23 40 51 102 83 78 102 93 108 148 155 150 207 195 259 267 270 261 209 222 232 180 244 243 222 190 200 217 204 138 144 113 75 66 65 53 58 45 46 99 53 78 67 64 51 62 64 30 38 26 22 29 37 20 30 22 15 15 22 27 38 28 34 16 11 4 1"); long long x; while (in >> x) q.rows.push_back(x);
            q.med2 = wpc::median2(q.rows); int poc = 0; for (int r = 0; r < (int)q.rows.size(); ++r) if (q.rows[(size_t)r] > q.rows[(size_t)poc]) poc = r; wpc::Node nd; nd.row = poc; q.nodes.push_back(nd);
            const std::vector<int> hq = wpc::hvnRows(q, 108618);
            CHECK(hq.size() == 2 && hq[0] == 308 && hq[1] == 232, "(1.9.1) NQ Nov 6: both H above the BD low - 27,219 (270) and 27,162 (197; 1.9.0 dropped it, 197 < the 2x floor 198)"); }
    }
    // ---- [2] the break level ----
    {   std::vector<wpc::Bar> B(12); for (int i = 0; i < 12; ++i) { B[(size_t)i].t = 1000 + 180 * i; B[(size_t)i].h = 110 + i; B[(size_t)i].l = 100 + i; B[(size_t)i].o = B[(size_t)i].c = 105 + i; }
        wpc::Wave w; w.dir = 1; w.i0 = 3; w.i1 = 10; w.p1 = 120;
        std::vector<wpc::Pivot> alt = {{1, 1, 114}, {3, -1, 103}};
        int why = 0; std::vector<wpc::BoCand> bos;
        wpc::BoCand c; c.t0 = B[7].t; c.dir = 1; c.hi = 117; c.lo = 107; bos.push_back(c);
        CHECK(wpc::breakLevel(w, B, alt, bos, why) == 117 && why == 1, "BO candle inside the wave: its HIGH");
        bos[0].dir = -1; CHECK(wpc::breakLevel(w, B, alt, bos, why) == B[5].h && why == 2, "no BO of the wave's side: the high of the first bar above the prior pivot high 114 (bar 5, high 115)");
        bos[0].dir = 1; bos[0].t0 = B[3].t; CHECK(wpc::breakLevel(w, B, alt, bos, why) == B[5].h && why == 2, "a BO on the start pivot bar is not inside the wave");
        bos[0].t0 = B[11].t; CHECK(why == 2 && wpc::breakLevel(w, B, alt, bos, why) == B[5].h, "a BO after the wave end is not its BO");
        std::vector<wpc::Pivot> none = {{3, -1, 103}}; CHECK(wpc::breakLevel(w, B, none, {}, why) == 120 && why == 3, "no prior pivot high: the wave extreme (no limit)");
    }
    // ---- [3] the real fixture ----
    if (argc > 1) {
        std::ifstream fi(argv[1]); std::string line; std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow> > V; std::vector<long long> vol;
        while (std::getline(fi, line)) { if (line.empty() || line[0] == '#') continue; auto x = wpc::splitCsv(line);
            if (x[0] == "B") { wpc::Bar b; b.t = atoll(x[2].c_str()); wpc::toTicks(atof(x[3].c_str()), .25, b.o); wpc::toTicks(atof(x[4].c_str()), .25, b.h); wpc::toTicks(atof(x[5].c_str()), .25, b.l); wpc::toTicks(atof(x[6].c_str()), .25, b.c); B.push_back(b); V.emplace_back(); vol.push_back(0); }
            else { wpc::VRow r; wpc::toTicks(atof(x[2].c_str()), .25, r.tick); r.vol = atoll(x[3].c_str()); r.dlt = atoll(x[4].c_str()); V[(size_t)atoi(x[1].c_str())].push_back(r); vol[(size_t)atoi(x[1].c_str())] += r.vol; } }
        CHECK(B.size() == 247, "fixture 247 bars");
        // lsBreakout's engine on the same bars, as WavePOC::feedBreakouts does
        std::vector<bko::In> I; for (size_t i = 0; i < B.size(); ++i) { bko::In b; b.t = B[i].t; b.ok = true; b.days = (long)wpc::floorDiv(b.t, 86400); b.endSec = (int)(b.t - (long long)b.days * 86400);
            b.o = B[i].o * .25; b.h = B[i].h * .25; b.l = B[i].l * .25; b.c = B[i].c * .25; b.vol = vol[i]; I.push_back(b); }
        bko::Engine K; K.update(I, (long)I.size(), I[0].t, .25, 180, 510, 900);
        wpc::Store S; wpc::Engine E; E.store = &S;
        for (const auto& kv : K.bo) { if (!kv.second.fired) continue; for (size_t j = 0; j < I.size(); ++j) if (I[j].t == kv.second.t0) { wpc::BoCand c; c.t0 = kv.second.t0; c.dir = kv.second.dir; c.hi = B[j].h; c.lo = B[j].l; E.bos.push_back(c); } }
        CHECK(E.bos.size() >= 2, "lsBreakout fired BO / BD candles on the fixture (" << E.bos.size() << ")");
        wpc::VapFn vap = [&](int i, std::vector<wpc::VRow>& o) { o = V[(size_t)i]; return 1; };
        std::map<long long, std::vector<int> > early;
        auto hv = [&](const wpc::Wave& w) { int why = 0; return wpc::hvnRows(w, wpc::breakLevel(w, E.B, E.alt, E.bos, why)); };
        for (size_t i = 0; i < B.size(); ++i) { E.addBar(B[i], vap); for (const wpc::Wave& w : E.waves) if (!w.provisional && !w.pending && (int)i == w.i1 + 6 && !early.count(w.t1)) early[w.t1] = hv(w); }
        int waves = 0, hvns = 0, side = 0, big = 0, notPoc = 0, why1 = 0, why2 = 0, why3 = 0, same = 0, cmp = 0;
        for (const wpc::Wave& w : E.waves) { if (w.provisional || w.pending || w.rows.empty()) continue; ++waves;
            int why = 0; const long long cut = wpc::breakLevel(w, E.B, E.alt, E.bos, why); why1 += why == 1; why2 += why == 2; why3 += why == 3;
            const std::vector<int> h = wpc::hvnRows(w, cut);
            for (int r : h) { ++hvns; const long long lo = w.e0 + (long long)r * w.rowTicks, hi = lo + w.rowTicks - 1;
                side += w.dir > 0 ? lo <= cut : hi >= cut; big += 2 * w.rows[(size_t)r] > w.med2; notPoc += w.nodes.empty() || r != w.nodes[0].row; }
            CHECK(h.size() <= 2, "at most 2 HVNs a wave");
            if (early.count(w.t1)) { ++cmp; same += early[w.t1] == h; }
            std::printf("  wave %s %s->%s ticks %lld..%lld  break %.2f (%s)  POC %.2f  HVN", w.dir > 0 ? "up  " : "down", std::to_string(w.t0 % 86400 / 3600).c_str(), std::to_string(w.t1 % 86400 / 3600).c_str(),
                (long long)w.p0, (long long)w.p1, cut * .25, why == 1 ? (w.dir > 0 ? "BO high" : "BD low") : why == 2 ? "broke prior pivot" : "none", w.nodes.empty() ? 0.0 : w.nodes[0].tick * .25);
            for (int r : h) std::printf(" %.2f", (w.e0 + (long long)r * w.rowTicks) * .25);
            std::printf("\n"); }
        CHECK(waves >= 5 && hvns > 0, "fixture: waves profiled and HVNs found (" << waves << " waves, " << hvns << " HVNs)");
        CHECK(why1 > 0, "fixture: at least one wave's break level came from a BO / BD candle (" << why1 << " BO, " << why2 << " prior pivot, " << why3 << " none)");
        CHECK(side == hvns && big == hvns && notPoc == hvns, "every HVN at / beyond its wave's break level, above the median row (1.9.1: no 2x floor), never the POC (" << side << "/" << big << "/" << notPoc << " of " << hvns << ")");
        CHECK(cmp > 0 && same == cmp, "no repaint: a finished wave's HVNs 6 bars after its end = at the end of the day (" << same << "/" << cmp << ")");
        // ---- [4] the drawing (Oct 30 afternoon) ----
        const int first = 165, last = 246; long long lo = LLONG_MAX, hi = LLONG_MIN; for (int i = first; i <= last; ++i) { lo = std::min(lo, B[(size_t)i].l); hi = std::max(hi, B[(size_t)i].h); }
        const double W = 1700, H = 900, bw = (W - 80) / (last - first + 3);
        wpc::View v; v.l = 0; v.t = 0; v.r = (int)W; v.b = (int)H; v.first = first; v.last = last; v.lastBar = last; v.bw = bw; v.tick = .25;
        v.X = [&](int i) { return 40 + (i - first + 0.5) * bw; }; v.Y = [&](double tk) { return H - 20 - (tk - (lo - 8)) / (double)(hi - lo + 16) * (H - 40); };
        v.textW = [](const std::string& s, bool) { return (int)s.size() * 7; };
        const wpc::Scene sc = wpc::layout(E, nullptr, v);
        int rowsN = 0, tooWide = 0, cyanRows = 0, hvnFill = 0, boxes = 0, badText = 0, Hs = 0, Ps = 0;
        for (const wpc::Wave& w : E.waves) { if (w.provisional || w.pending || w.i0 < first) continue; const int x0 = (int)std::lround(v.X(w.i0) - bw * 0.45);
            for (const wpc::Prim& p : sc.p) if (p.k == wpc::Prim::FILL && p.back && p.x0 == x0 + 1) { ++rowsN; tooWide += p.x1 > x0 + 1 + (int)wpc::SIDE_W + 1;
                cyanRows += p.col == 0x22D3EE; hvnFill += p.col == (w.dir > 0 ? 0x22C55EUL : 0xEF4444UL); } }
        for (const wpc::Prim& p : sc.p) { boxes += p.k == wpc::Prim::BOX; if (p.k == wpc::Prim::TEXT && p.text.size() == 1) { Hs += p.text == "H"; Ps += p.text == "P"; badText += p.text != "H" && p.text != "P"; } }
        CHECK(rowsN > 20 && tooWide == 0, "volume rows right of each spine, at most 38 px (" << rowsN << " rows)");
        CHECK(cyanRows > 0, "the POC row filled cyan (" << cyanRows << ")");
        CHECK(hvnFill > 0 && hvnFill <= sc.hvnDrawn, "HVN rows filled in the wave colour (" << hvnFill << " fills, " << sc.hvnDrawn << " HVNs in the scene)");
        CHECK(boxes == 0 && badText == 0 && Hs > 0 && Ps > 0, "letters only P and H (no I / A / T), no boxes (" << Ps << " P, " << Hs << " H, " << badText << " other, " << boxes << " boxes)");
        const std::string st = wpc::statusText(E, "ES", 180, "native,ok", "loaded", "loaded", 0, &sc, false, 0);
        CHECK(st.find("PROFILE,(1.9.1)") != std::string::npos && st.find("HVNs drawn now") != std::string::npos, "status PROFILE line (1.9.1)");
    }
    // ---- [5] versions ----
    {   const std::string cpp = slurp("../WavePOC.cpp"), set = slurp("../WavePOCSettings.h");
        CHECK(std::string(wpc::VERSION) == "1.9.1" && cpp.find("WP_VERSION = \"1.9.1\"") != std::string::npos && cpp.find("p->setVersion(\"1.9.1\")") != std::string::npos
              && set.find("PLUGIN_VERSION = \"1.9.1\"") != std::string::npos, "versions 1.9.1"); }
    std::cerr << "test_wavepoc_190: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
