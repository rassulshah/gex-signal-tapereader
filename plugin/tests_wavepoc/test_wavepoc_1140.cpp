// test_wavepoc_1140.cpp -- Wave POC 1.14.0 (Rassul 2026-11-11 19:09-19:12 CT "remove the wave poc spine and move the wave poc in the candels that
// it belowngs in" / "also the hvn" / "you can move the lot count at the end of the poc") on today's REAL ES 3-min bars + volume at price
// (wavepoc_es1111.csv) and the three waves the plugin recorded today (ES-180-nodes.csv: 10:36-11:06, 11:12-11:45, 12:09-12:33).
// Build (in plugin\tests_wavepoc):  g++ -std=c++17 -Wall -Wextra -Werror -I.. test_wavepoc_1140.cpp -o t1140 && ./t1140 wavepoc_es1111.csv [dump.json]
#include "../WavePOCLogic.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
static int passed = 0, failed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::cout << "FAIL: " << m << " (line " << __LINE__ << ")\n"; } } while (0)
static long long T(const std::string& s) { struct tm t = {}; std::sscanf(s.c_str(), "%d-%d-%d %d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min); t.tm_year -= 1900; t.tm_mon -= 1; return (long long)timegm(&t); }
struct Fix { std::vector<wpc::Bar> B; std::map<long long, std::vector<wpc::VRow> > V; int at(long long t) const { for (size_t i = 0; i < B.size(); ++i) if (B[i].t == t) return (int)i; return -1; } };
static Fix load(const char* f) {
    Fix X; std::ifstream in(f); std::string ln;
    while (std::getline(in, ln)) {
        if (ln.size() < 3 || ln[0] == '#') { continue; }
        std::vector<std::string> c; size_t a = 0, b; while ((b = ln.find('|', a)) != std::string::npos) { c.push_back(ln.substr(a, b - a)); a = b + 1; } c.push_back(ln.substr(a));
        if (c[0] == "B" && c.size() == 6) { wpc::Bar r; r.t = T(c[1]); r.o = std::atoll(c[2].c_str()); r.h = std::atoll(c[3].c_str()); r.l = std::atoll(c[4].c_str()); r.c = std::atoll(c[5].c_str()); X.B.push_back(r); }
        else if (c[0] == "V" && c.size() == 4) { wpc::VRow v; v.tick = std::atoll(c[2].c_str()); v.vol = std::atoll(c[3].c_str()); X.V[T(c[1])].push_back(v); }
    }
    return X;
}
static long long tk(double px) { return (long long)std::llround(px * 4); }
struct Spec { const char* t0; const char* t1; double p0, p1, poc; long long lots; double hvn; };
// brute force: the bar of [i0, i1] with the most volume at one tick (earliest on a tie)
static int bestBar(const Fix& X, int i0, int i1, long long tick) {
    int best = -1; long long bv = 0;
    for (int k = i0; k <= i1; ++k) { long long q = 0; const auto it = X.V.find(X.B[(size_t)k].t); if (it != X.V.end()) for (const wpc::VRow& v : it->second) if (v.tick == tick) q += v.vol; if (q > bv) { bv = q; best = k; } }
    return best;
}
int main(int argc, char** argv) {
    const Fix X = load(argc > 1 ? argv[1] : "wavepoc_es1111.csv");
    CHECK(X.B.size() == 80, "fixture loaded (" << X.B.size() << " bars)");
    CHECK(std::string(wpc::VERSION) == "1.14.0", "version 1.14.0 (" << wpc::VERSION << ")");
    wpc::Engine E; E.B = X.B;
    wpc::VapFn vap = [&X](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = X.V.find(X.B[(size_t)j].t); if (it == X.V.end()) return 0; out = it->second; return 1; };
    const Spec SP[3] = { { "2026-11-11 10:36", "2026-11-11 11:06", 7631.50, 7652.00, 7650.00, 2544, 0 },
                         { "2026-11-11 11:12", "2026-11-11 11:45", 7644.50, 7660.25, 7656.50, 2421, 7652.50 },
                         { "2026-11-11 12:09", "2026-11-11 12:33", 7652.00, 7669.00, 7661.75, 1938, 7660.50 } };
    std::vector<wpc::Wave> W; std::vector<std::vector<int> > HV;
    for (const Spec& s : SP) {
        wpc::Wave w; w.dir = 1; w.i0 = X.at(T(s.t0)); w.i1 = X.at(T(s.t1)); w.t0 = T(s.t0); w.t1 = T(s.t1); w.p0 = tk(s.p0); w.p1 = tk(s.p1); w.qWhy = wpc::Q_RLY;
        CHECK(w.i0 >= 0 && w.i1 > w.i0, "wave bars found " << s.t0);
        CHECK(E.profile(w, vap), "profile built " << s.t0);
        wpc::Node n; n.tick = tk(s.poc); n.rank = 0; n.row = (int)((n.tick - w.e0) / w.rowTicks); n.vol = w.rows[(size_t)n.row]; w.nodes.push_back(n);
        CHECK(w.rowTicks == 1 && n.vol == s.lots && w.maxVol == s.lots, "POC " << s.poc << " = the plugin's recorded " << s.lots << " lots (got " << n.vol << ")");
        std::vector<int> hv; if (s.hvn > 0) hv.push_back((int)((tk(s.hvn) - w.e0) / w.rowTicks));
        W.push_back(w); HV.push_back(hv);
    }
    // ---- rowBar = the brute-force "most volume at that price" bar, for EVERY traded row of every wave ----
    {   int rows = 0, wrong = 0, outside = 0;
        for (const wpc::Wave& w : W) for (size_t j = 0; j < w.rows.size(); ++j) { if (w.rows[j] <= 0) continue; ++rows; const int b = bestBar(X, w.i0, w.i1, w.e0 + (long long)j); if (w.rowBar[j] != b) ++wrong; if (w.rowBar[j] < w.i0 || w.rowBar[j] > w.i1) ++outside; }
        CHECK(rows > 150 && wrong == 0 && outside == 0, "every row's candle = the bar with the most volume there (" << rows << " rows, " << wrong << " wrong, " << outside << " outside the wave)"); }
    // ---- the drawing ----
    wpc::View v; v.l = 0; v.t = 0; v.r = 1100; v.b = 520; v.bw = 15; v.first = X.at(T("2026-11-11 10:03")); v.last = (int)X.B.size() - 1; v.lastBar = v.last;
    const int f0 = v.first; long long lo = X.B[(size_t)f0].l, hi = X.B[(size_t)f0].h; for (int k = f0; k <= v.last; ++k) { lo = std::min(lo, X.B[(size_t)k].l); hi = std::max(hi, X.B[(size_t)k].h); }
    v.X = [f0](int i) { return 20.0 + (i - f0) * 15.0; };
    v.Y = [lo, hi](double p) { return 30.0 + ((double)hi + 4 - p) / ((double)(hi - lo) + 8) * 460.0; };
    v.textW = [](const std::string& s, bool bold) { return (int)s.size() * (bold ? 8 : 6); };
    wpc::Colours C; wpc::Scene s; std::vector<std::pair<const wpc::Wave*, const wpc::Node*> > tags;
    for (size_t k = 0; k < W.size(); ++k) wpc::drawWave(s, W[k], v, C, tags, false, -1, 1e9, &E.B, nullptr, 0, -1, &HV[k]);
    int vert = 0, fills = 0, pTxt = 0, hTxt = 0, lots = 0;
    for (const wpc::Prim& p : s.p) { if (p.k == wpc::Prim::LINE && p.x0 == p.x1 && p.y0 != p.y1) ++vert; if (p.k == wpc::Prim::FILL) ++fills; if (p.k == wpc::Prim::TEXT && p.text == "P") ++pTxt; if (p.k == wpc::Prim::TEXT && p.text == "H") ++hTxt; if (p.k == wpc::Prim::TEXT && p.role == 1) ++lots; }
    CHECK(vert == 0, "no spine (vertical lines: " << vert << ")");
    CHECK(fills == 5 && pTxt == 3 && hTxt == 2 && lots == 3, "3 POC dashes + 2 HVN dashes, 3 P, 2 H, 3 lot counts (" << fills << " / " << pTxt << " / " << hTxt << " / " << lots << ")");
    const double half = wpc::dashHalf(v);
    for (size_t k = 0; k < W.size(); ++k) {
        const wpc::Wave& w = W[k]; const long long pt = w.nodes[0].tick; const int bar = bestBar(X, w.i0, w.i1, pt); const double xc = v.X(bar), yc = v.Y((double)pt);
        const wpc::Prim* dash = nullptr; const wpc::Prim* P = nullptr; const wpc::Prim* lot = nullptr;
        for (const wpc::Prim& p : s.p) {
            if (p.k == wpc::Prim::FILL && p.col == C.cyan && std::abs(p.x0 - wpc::ci(xc - half)) <= 0 && std::abs((p.y0 + p.y1) / 2.0 - yc) <= 1.0) dash = &p;
            if (p.k == wpc::Prim::TEXT && p.text == "P" && p.x1 <= wpc::ci(xc - half) && p.x1 >= wpc::ci(xc - half) - 4 && std::abs((p.y0 + p.y1) / 2.0 - yc) <= 1.0) P = &p;
            if (p.k == wpc::Prim::TEXT && p.role == 1 && p.text == wpc::lotsText(SP[k].lots) && p.x0 >= wpc::ci(xc + half) && p.x0 <= wpc::ci(xc + half) + 4) lot = &p;
        }
        CHECK(dash && dash->x1 == wpc::ci(xc + half) && !dash->back && dash->y1 - dash->y0 >= 3, "wave " << k << ": cyan POC dash ON its candle (bar " << bar << "), candle-wide, in front");
        CHECK(bar >= w.i0 && bar <= w.i1 && X.B[(size_t)bar].l <= pt && pt <= X.B[(size_t)bar].h, "wave " << k << ": that candle is inside the wave and traded the POC price");
        CHECK(P && P->col == C.cyan, "wave " << k << ": P just left of the dash");
        CHECK(lot && std::abs((lot->y0 + lot->y1) / 2.0 - yc) <= 1.5 && lot->col == C.cyan, "wave " << k << ": lots (" << wpc::lotsText(SP[k].lots) << ") at the END of the dash, on its line");
        if (!HV[k].empty()) { const long long ht = w.e0 + HV[k][0]; const int hb = bestBar(X, w.i0, w.i1, ht); const double hx = v.X(hb), hy = v.Y((double)ht); bool ok = false, let = false;
            for (const wpc::Prim& p : s.p) { if (p.k == wpc::Prim::FILL && p.col == C.up && p.x0 == wpc::ci(hx - half) && p.x1 == wpc::ci(hx + half) && std::abs((p.y0 + p.y1) / 2.0 - hy) <= 1.0) ok = true;
                                             if (p.k == wpc::Prim::TEXT && p.text == "H" && p.x1 <= wpc::ci(hx - half) && p.x1 >= wpc::ci(hx - half) - 4) let = true; }
            CHECK(ok && let, "wave " << k << ": HVN dash (wave colour) on ITS candle (bar " << hb << ") with the H left of it"); }
    }
    {   // the 10:36 wave: 7650.00 traded 1,191 in the 11:03 bar and 1,353 in the 11:06 bar -> the 11:06 candle
        CHECK(bestBar(X, W[0].i0, W[0].i1, tk(7650.00)) == X.at(T("2026-11-11 11:06")), "7650.00 belongs in the 11:06 candle (1,353 of the 2,544 lots)"); }
    // ---- no text overlaps another text ----
    {   int hit = 0; std::vector<const wpc::Prim*> t; for (const wpc::Prim& p : s.p) if (p.k == wpc::Prim::TEXT) t.push_back(&p);
        for (size_t i = 0; i < t.size(); ++i) for (size_t j = i + 1; j < t.size(); ++j) if (t[i]->x0 < t[j]->x1 && t[j]->x0 < t[i]->x1 && t[i]->y0 < t[j]->y1 && t[j]->y0 < t[i]->y1) ++hit;
        CHECK(hit == 0, "no letter / lot count overlaps another (" << hit << ")"); }
    // ---- the extended POC line starts after the lot count ----
    {   wpc::Scene s2; std::vector<const wpc::Wave*> ws; for (const wpc::Wave& w : W) ws.push_back(&w); wpc::drawPocLines(s2, E, v, C, ws);
        int lines = 0, bad = 0;
        for (const wpc::Prim& p : s2.p) { if (p.k != wpc::Prim::LINE) continue; ++lines;
            for (const wpc::Prim& q : s.p) if (q.k == wpc::Prim::TEXT && q.role == 1 && std::abs((q.y0 + q.y1) / 2 - p.y0) <= 2 && p.x0 < q.x1) ++bad; }
        CHECK(lines >= 1 && bad == 0, "POC line(s) to the right start after the lot count (" << lines << " lines, " << bad << " through a label)"); }
    // ---- fallback (no per-row record): the first candle of the wave that traded the price; never outside the wave ----
    {   wpc::Wave w = W[0]; w.rowBar.clear(); const int b = wpc::nodeBar(w, w.nodes[0].row, w.nodes[0].tick, &E.B);
        CHECK(b >= w.i0 && b <= w.i1 && X.B[(size_t)b].l <= w.nodes[0].tick && w.nodes[0].tick <= X.B[(size_t)b].h, "fallback candle traded the price (bar " << b << ")");
        CHECK(wpc::nodeBar(w, -1, 1, nullptr) == w.i0, "no data at all -> the wave's first candle"); }
    // ---- stable: profiling again gives the same candles (no repaint); a candle left of the view draws nothing ----
    {   wpc::Wave w = W[1]; const std::vector<int> a = w.rowBar; E.profile(w, vap); CHECK(a == w.rowBar, "same candles on a second pass");
        wpc::View v2 = v; v2.first = W[0].i1 + 1; wpc::Scene s3; std::vector<std::pair<const wpc::Wave*, const wpc::Node*> > tg; wpc::drawWave(s3, W[0], v2, C, tg, false, -1, 1e9, &E.B, nullptr, 0, -1, &HV[0]);
        int f = 0; for (const wpc::Prim& p : s3.p) if (p.k == wpc::Prim::FILL || p.k == wpc::Prim::TEXT) ++f; CHECK(f == 0, "a POC candle scrolled out of view draws nothing (" << f << ")"); }
    if (argc > 2) {   // the mockup dump: bars + prims in pixels
        wpc::Scene all = s; std::vector<const wpc::Wave*> ws; for (const wpc::Wave& w : W) ws.push_back(&w); wpc::drawPocLines(all, E, v, C, ws);
        std::ofstream o(argv[2]); o << "{\"lo\":" << lo << ",\"hi\":" << hi << ",\"bars\":[";
        for (int k = f0; k <= v.last; ++k) { const wpc::Bar& b = X.B[(size_t)k]; o << (k > f0 ? "," : "") << "[" << b.t % 86400 / 60 << "," << b.o << "," << b.h << "," << b.l << "," << b.c << "]"; }
        o << "],\"prims\":[";
        for (size_t i = 0; i < all.p.size(); ++i) { const wpc::Prim& p = all.p[i]; char c[16]; std::snprintf(c, sizeof c, "#%06lx", p.col & 0xFFFFFF);
            o << (i ? "," : "") << "[" << (int)p.k << "," << p.x0 << "," << p.y0 << "," << p.x1 << "," << p.y1 << ",\"" << c << "\",\"" << p.text << "\"," << p.bold << "," << p.w << "]"; }
        o << "]}";
    }
    std::cout << "test_wavepoc_1140 (Wave POC " << wpc::VERSION << "): " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
