// test_zzocombo_210.cpp - lsZzoCombo 2.1.0 / lsZzoZigZag 1.1.0 (2026-11-03, Rassul 13:59 "make it 3bar using high and low by default",
// 14:02 "remove weak" + the count only vs prior waves of the same colour on the ramp's own measure, 14:03 "move the count 1 more to the left").
//   1. the 3-bar High/Low walk on synthetic bars (up and down): a 1- or 2-bar pullback that stays above (below) the extreme of the previous
//      3 bars does NOT turn the wave, the bar that breaks the previous 3 bars' low (high) does; equal does not; the 1-bar engine turns on
//      the first lower low (Delta / Volume unchanged); the walk reads highs / lows only
//   2. the 3-bar walk = lsWavePOC's IRT ZIGZAG rule (an independent copy of WavePOCLogic.h addBar) pivot for pivot on real ES 3-min bars
//   3. the count: a green wave stops at an earlier bigger green wave; red waves never count against green ones (and back); waves with no
//      normal are skipped; 50+ cap; on real ES every count = the ramp-ratio walk
//   4. drawing: no "weak" anywhere (while weak waves exist), the count's right edge one bar left of the wave's right edge, the ratio still
//      centred on the wave's last bar
//   5. the hosts: ZzoCombo.cpp / ZzoZigZag.cpp set revBars = COMBO_REV_BARS; version literals = the logic's
//   6. regression: ES Oct 26 - Nov 3 14:00 CT (fp_bars), the Nov 3 wave list printed
#include "../ZzoComboView.h"
#include <climits>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

static zzd::Bar hb(long long t, long long h, long long l) { zzd::Bar b; b.t = t; b.h = h; b.l = l; b.vol = 100; b.vap = true; return b; }
static zzd::Engine run(const std::vector<std::pair<long long, long long>>& hl, int N) {
    zzd::Engine E; E.revBars = N; E.setMarket("ES", 180);
    long long t = 1793700000; for (auto& p : hl) { E.addBar(hb(t, p.first, p.second)); t += 180; }
    return E;
}
static std::vector<zzd::Bar> load(const char* csv) {
    std::vector<zzd::Bar> v; std::ifstream f(csv); std::string ln;
    while (std::getline(f, ln)) {
        if (ln.empty() || ln[0] == '#') continue;
        std::stringstream s(ln); std::string a; std::vector<std::string> x; while (std::getline(s, a, ',')) x.push_back(a);
        zzd::Bar b; b.t = std::stoll(x[0]); zzd::toTicks(std::stod(x[1]), 0.25, b.h); zzd::toTicks(std::stod(x[2]), 0.25, b.l);
        b.vol = std::stoll(x[3]); b.dlt = std::stoll(x[4]); b.vap = true; b.xh = std::stoll(x[5]); b.xl = std::stoll(x[6]); b.xok = x[7] == "1"; v.push_back(b);
    }
    return v;
}
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
static std::string hhmm(long long t) { t -= 180; char b[16]; const int s = (int)(((t % 86400) + 86400) % 86400); int h = s / 3600, m = s / 60 % 60;
    std::snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return b; }   // bar START, 12-hour CT

int main() {
    CHECK(zzd::COMBO_REV_BARS == 3, "Combo default = 3 reversal bars");
    { zzd::Engine E; CHECK(E.revBars == 1, "a plain engine (ZZO Delta / Volume) stays 1-bar"); E.revBars = 3; E.reset(); CHECK(E.revBars == 3, "reset keeps revBars"); }

    // ---- 1. synthetic up wave: bars 4 / 5 make lower lows (a 2-bar pullback) but stay at / above the lowest low of the previous 3 bars
    const std::vector<std::pair<long long, long long>> up = { {10, 5}, {12, 7}, {14, 9}, {16, 11}, {15, 10}, {15, 9}, {17, 12}, {16, 11}, {15, 10}, {14, 8}, {13, 7} };
    {   zzd::Engine E3 = run(up, 3), E1 = run(up, 1);
        CHECK(E1.W.size() >= 1 && E1.W[0].e == 3 && E1.W[0].dir == 1, "1-bar: the first lower low (bar 4) turns the up wave at bar 3");
        bool none = true; for (const zzd::Wave& w : E3.W) if (w.e <= 6 && w.dir == 1 && w.e != 6) none = false;
        CHECK(none, "3-bar: the 1-bar (bar 4) and 2-bar (bar 5, equal low = not below) pullbacks do not turn the wave");
        CHECK(E3.W.size() == 1 && E3.W[0].dir == 1 && E3.W[0].e == 6 && E3.W[0].s == 0, "3-bar: bar 9 (low 8 < the previous 3 lows 12 / 11 / 10) turns it - the up wave ends at bar 6's high");
        CHECK(E3.Z.dirn == -1 && E3.Z.ext == 10, "3-bar: the down wave runs from bar 9, extreme at bar 10");
        // highs / lows only: the same highs / lows with any volume / delta give the same waves
        zzd::Engine E4; E4.revBars = 3; E4.setMarket("ES", 180); long long t = 1793700000;
        for (size_t i = 0; i < up.size(); ++i) { zzd::Bar b = hb(t, up[i].first, up[i].second); b.vol = 1000 + 37 * (long long)i; b.dlt = (i % 2 ? -500 : 500); E4.addBar(b); t += 180; }
        CHECK(E4.W.size() == E3.W.size() && E4.W[0].e == E3.W[0].e, "the walk reads highs / lows only"); }
    // mirror: a down wave, 1- and 2-bar bounces with higher highs stay below the previous 3 bars' highest high
    {   std::vector<std::pair<long long, long long>> dn; for (auto& p : up) dn.push_back({ 100 - p.second, 100 - p.first });
        zzd::Engine E3 = run(dn, 3), E1 = run(dn, 1);
        CHECK(E1.W.size() >= 1 && E1.W[0].e == 3 && E1.W[0].dir == -1, "1-bar mirror: the first higher high turns the down wave");
        CHECK(E3.W.size() == 1 && E3.W[0].dir == -1 && E3.W[0].e == 6, "3-bar mirror: only the bar above the previous 3 highs turns the down wave"); }

    // ---- 2. = lsWavePOC's IRT ZIGZAG rule on real bars (independent copy of WavePOCLogic.h addBar, N = 3)
    const std::vector<zzd::Bar> es = load("zzc_es_1103.csv");
    CHECK(es.size() > 3000, "ES fixture loaded");
    zzd::Engine E; E.revBars = zzd::COMBO_REV_BARS; E.setMarket("ES", 180);
    for (size_t i = 0; i + 1 < es.size(); ++i) E.addBar(es[i]);   // the last (partial) bar = forming
    {   const int N = 3; int zDir = 0, zI = 0, zHi = 0, zLo = 0; std::vector<int> piv;   // WavePOC's pivots (bar index of each final extreme)
        for (int k = 0; k + 1 < (int)es.size(); ++k) {
            if (k >= N) { long long ph = LLONG_MIN, pl = LLONG_MAX; for (int j = k - N; j < k; ++j) { ph = std::max(ph, es[(size_t)j].h); pl = std::min(pl, es[(size_t)j].l); }
                const zzd::Bar& c = es[(size_t)k];
                if (zDir == 0) { if (c.h > ph) { piv.push_back(zLo); zDir = 1; zI = k; } else if (c.l < pl) { piv.push_back(zHi); zDir = -1; zI = k; } }
                else if (zDir > 0) { if (c.h > es[(size_t)zI].h) zI = k; else if (c.l < pl) { piv.push_back(zI); zDir = -1; zI = k; } }
                else { if (c.l < es[(size_t)zI].l) zI = k; else if (c.h > ph) { piv.push_back(zI); zDir = 1; zI = k; } } }
            if (zDir == 0) { if (es[(size_t)k].h > es[(size_t)zHi].h) zHi = k; if (es[(size_t)k].l < es[(size_t)zLo].l) zLo = k; }
        }
        // compare from the 3rd pivot on (the very first leg's start differs by design: WavePOC waits for N bars, ZZO starts at bar 1)
        std::vector<int> zz; for (const zzd::Wave& w : E.W) zz.push_back(w.e);
        int same = 0, cmp = 0;
        for (size_t a = 2; a < piv.size(); ++a) { ++cmp; for (int x : zz) if (x == piv[a]) { ++same; break; } }
        std::printf("  WavePOC rule pivots %zu, ZZO 3-bar waves %zu, pivots matched %d / %d\n", piv.size(), zz.size(), same, cmp);
        CHECK(cmp > 100 && same == cmp && zz.size() + 2 >= piv.size() && zz.size() <= piv.size() + 2, "3-bar walk = lsWavePOC's IRT ZIGZAG (3 bars, High/Low) pivot for pivot"); }

    // ---- 2b. = the nightly's Python replica (lra.zzo_model 1.2.0, rev_bars = 3) wave for wave: pivot, side, count, cap, Ab, extreme, E/R
    {   zzd::Engine P; P.revBars = zzd::COMBO_REV_BARS; P.setMarket("ES", 180); for (const zzd::Bar& b : es) P.addBar(b);
        std::ifstream r("ref_zzc3_es_1103.txt"); std::string ln; std::vector<std::string> want; while (std::getline(r, ln)) if (!ln.empty()) want.push_back(ln);
        int same = 0, shown = 0;
        for (size_t k = 0; k < P.W.size() && k < want.size(); ++k) { const zzd::Wave& w = P.W[k];
            char xs[32], er[32]; double e = 0;
            if (w.xsh < 0) std::snprintf(xs, sizeof xs, "na"); else std::snprintf(xs, sizeof xs, "%.4f", w.xsh);
            if (zzd::effortResult(w, e)) std::snprintf(er, sizeof er, "%.3f", e); else std::snprintf(er, sizeof er, "na");
            char g[160]; std::snprintf(g, sizeof g, "%d,%d,%d,%d,%d,%s,%s,%d", w.e, w.dir, w.cnt, (int)w.cntCap, w.ab, xs, er, (int)zzd::abHeavy(w));
            if (want[k] == g) ++same; else if (shown++ < 5) std::printf("  wave %zu got %s want %s\n", k, g, want[k].c_str()); }
        std::printf("  Python replica (3-bar): %zu waves, %d match\n", P.W.size(), same);
        CHECK(P.W.size() == want.size() && same == (int)want.size(), "3-bar engine = lra.zzo_model rev_bars=3 (the nightly scorer)"); }

    // ---- 3. the count
    {   auto mk = [](int dir, long long vol, double medV, bool hasV = true) { zzd::Wave w; w.dir = dir; w.vol = vol; w.medV = medV; w.hasV = hasV; return w; };
        std::vector<zzd::Wave> W = { mk(1, 300, 100), mk(-1, 900, 100), mk(1, 100, 100), mk(1, 999, 100, false), mk(-1, 5000, 100), mk(1, 150, 100), mk(-1, 800, 100) };
        zzd::Wave g = mk(1, 200, 100); zzd::countBack(W, W.size(), g);
        CHECK(g.cnt == 2 && !g.cntCap, "green 2.0x: beats the green 1.5x and 1.0x, stops at the earlier green 3.0x; reds (8x, 50x, 9x) and the no-normal wave never count");
        zzd::Wave r = mk(-1, 850, 100); zzd::countBack(W, W.size(), r);
        CHECK(r.cnt == 1, "red 8.5x: beats the red 8.0x, stops at the red 50x - greens never count against reds");
        zzd::Wave rr = mk(-1, 100, 100); zzd::countBack(W, W.size(), rr); CHECK(rr.cnt == 0, "a small red wave: 0 (hidden)");
        zzd::Wave eq = mk(1, 150, 100); zzd::countBack(W, W.size(), eq); CHECK(eq.cnt == 0, "equal ratio stops the walk");
        zzd::Wave rv = mk(1, 400, 200); zzd::countBack(W, W.size(), rv); CHECK(rv.cnt == 2, "the RATIO counts, not raw volume (400 lots at a 200 normal = 2.0x)");
        std::vector<zzd::Wave> M; for (int i = 0; i < 60; ++i) { M.push_back(mk(1, 100, 100)); M.push_back(mk(-1, 9000, 100)); }
        zzd::Wave big = mk(1, 300, 100); zzd::countBack(M, M.size(), big); CHECK(big.cntCap && big.cnt == zzd::CNT_CAP && zzd::countText(big) == "50+", "50+ cap");
        zzd::Wave nv = mk(1, 300, 0, false); zzd::countBack(W, W.size(), nv); CHECK(nv.cnt == 0, "no normal: no count"); }
    {   int bad = 0, n = 0;   // real ES: every wave's count = the ramp-ratio walk over prior same-colour waves
        for (size_t k = 0; k < E.W.size(); ++k) { const zzd::Wave& w = E.W[k]; if (!w.hasV) { bad += w.cnt != 0; continue; }
            const double r = (double)w.vol / w.medV; int c = 0;
            for (size_t j = k; j-- > 0;) { const zzd::Wave& x = E.W[j]; if (x.dir != w.dir || !x.hasV) continue; if ((double)x.vol / x.medV >= r) break; if (++c >= zzd::CNT_CAP) break; }
            bad += c != w.cnt; n += w.cnt > 0; }
        std::printf("  ES: %zu waves, %d with a count\n", E.W.size(), n);
        CHECK(bad == 0 && n > 50, "ES: every count = prior same-colour waves with a smaller ramp ratio"); }

    // ---- 4. drawing
    {   zzd::Bar form = es.back(); form.seg = E.segOf(form.t);
        const std::vector<zzd::Wave> waves = E.view(&form);
        const int from = std::max(0, (int)E.B.size() - 400);
        zzd::View v; v.l = 0; v.t = 0; v.r = 9 * ((int)E.B.size() + 1 - from) + 20; v.b = 260; v.first = from; v.last = (int)E.B.size(); v.bw = 9;
        v.X = [from](int j) { return (j - from) * 9.0 + 5; }; v.textW = [](const std::string& t) { return (int)t.size() * 6; }; v.textH = 11;
        const zzd::ComboScene S = zzd::layoutCombo(E, waves, form.vol, v, "ZZO Combo 2.1.0");
        int weakW = 0; for (const zzd::Wave& w : waves) if (w.cobTag == 2 && w.e >= from) ++weakW;
        bool weakTxt = false; for (const zzd::Prim& p : S.p) if (p.k == zzd::Prim::TEXT && p.text == "weak") weakTxt = true;
        std::printf("  drawn: ratios %d counts %d CoB %d Cmx %d Ab %d; weak waves in view %d\n", S.ratios, S.counts, S.cob, S.cmx, S.ab, weakW);
        CHECK(!weakTxt && S.weak == 0, "no weak tag drawn");
        CHECK(zzd::comboStatus(E, "ES", 180, "ok").find("ZIGZAG,3-bar reversal, High/Low") != std::string::npos, "status file names the 3-bar High/Low ZigZag");
        int counts = 0, atX = 0, ratiosC = 0, ratios = 0; const double half = std::max(0.5, v.bw * 0.5);
        for (const zzd::Prim& p : S.p) {   // every count / ratio label belongs to a 2.0x+ wave at the expected x
            if (p.k != zzd::Prim::TEXT || p.text.empty() || p.text.rfind("ZZO", 0) == 0) continue;
            const bool isCount = p.col == 0xFFFFFF && std::isdigit((unsigned char)p.text[0]) && p.text.back() != 'x';
            const bool isRatio = p.text.back() == 'x' && p.col != 0xFFFFFF;
            if (!isCount && !isRatio) continue;
            if (isCount) ++counts; else ++ratios;
            for (const zzd::Wave& w : waves) {
                if (!w.hasV || w.e < v.first || w.e > v.last || (double)w.vol / w.medV < zzd::CNT_MIN_VOL) continue;
                if (isCount && w.cnt > 0 && p.text == zzd::countText(w) && (p.x1 == zzd::ci(v.X(w.e) + half - v.bw) - 1 || p.x0 == v.l + 1)) { ++atX; break; }   // one bar left of the old anchor (the wave's right edge)
                char rb[24]; std::snprintf(rb, sizeof rb, "%.1fx", (double)w.vol / w.medV);
                if (isRatio && p.text == rb) { const int tw = v.textW(rb); int x0 = zzd::ci(v.X(w.e) - tw / 2.0); x0 = std::max(v.l + 1, std::min(x0, v.r - 1 - tw)); if (p.x0 == x0) { ++ratiosC; break; } }
            }
        }
        std::printf("  counts at the new x %d / %d, ratios centred %d / %d\n", atX, counts, ratiosC, ratios);
        CHECK(counts > 0 && atX == counts, "the count's right edge = one bar left of the wave's right edge");
        CHECK(ratios > 0 && ratiosC == ratios, "the ratio label stays centred on the wave's last bar");
        // a hand-made check of the shift: a wave at bar e draws its count ending at X(e) - bw/2 - 1
        for (const zzd::Prim& p : S.p) if (p.k == zzd::Prim::TEXT && p.col == 0xFFFFFF && !p.text.empty() && std::isdigit((unsigned char)p.text[0])) {
            bool inside = false; for (const zzd::Wave& w : waves) if (w.cnt > 0 && p.x1 <= zzd::ci(v.X(w.e) - half) && p.x1 >= zzd::ci(v.X(w.s) - half) - 1) inside = true;
            CHECK(inside || p.x0 == v.l + 1, "every count ends left of its wave's last bar"); }
    }

    // ---- 5. hosts
    {   const std::string c = slurp("../ZzoCombo.cpp"), z = slurp("../ZzoZigZag.cpp");
        CHECK(c.find("S.E.revBars = zzd::COMBO_REV_BARS") != std::string::npos, "ZzoCombo.cpp sets the 3-bar walk");
        CHECK(z.find("S.E.revBars = zzd::COMBO_REV_BARS") != std::string::npos, "ZzoZigZag.cpp follows Combo's walk");
        CHECK(c.find(std::string("p->setVersion(\"") + zzd::COMBO_VERSION + "\")") != std::string::npos && c.find(std::string("v") + zzd::COMBO_VERSION + " - 3-bar") != std::string::npos, "Combo version literal = COMBO_VERSION");
        CHECK(z.find(std::string("p->setVersion(\"") + zzd::ZIGZAG_VERSION + "\")") != std::string::npos, "ZigZag version literal = ZIGZAG_VERSION");
        const std::string d = slurp("../ZzoDelta.cpp"); CHECK(d.find("revBars") == std::string::npos, "ZZO Delta / Volume keep the 1-bar default"); }

    // ---- 6. regression: the Nov 3 waves (3-bar, High/Low)
    {   std::printf("  ES 2026-11-03 waves (3-bar High/Low ZigZag; pivot bar START CT; size pts; vol ratio; count):\n");
        const long long day = E.segOf(es.back().t) / 4; int n = 0, nr = 0; long long sz = 0;
        for (const zzd::Wave& w : E.W) { if (w.s < 0 || w.seg / 4 != day) continue; ++n; sz += w.size;
            const bool rth = zzd::Engine::kindOf(w.seg) == 2; nr += rth;
            if (rth) std::printf("    %-8s %s %6.2f  %-5s %s%s%s\n", hhmm(E.B[(size_t)w.e].t).c_str(), w.dir > 0 ? "UP" : "DN", w.size * 0.25, zzd::volText(w).c_str(),
                w.cnt > 0 ? zzd::countText(w).c_str() : "", w.cmx ? " Cmx" : "", w.cobTag == 1 ? " CoB" : w.cobTag == 2 ? " (weak, not drawn)" : ""); }
        std::printf("  Nov 3 session: %d waves (%d RTH), average %.2f pts\n", n, nr, n ? sz * 0.25 / n : 0.0);
        zzd::Engine E1; E1.setMarket("ES", 180); for (size_t i = 0; i + 1 < es.size(); ++i) E1.addBar(es[i]);
        int n1 = 0; for (const zzd::Wave& w : E1.W) if (w.s >= 0 && w.seg / 4 == day) ++n1;
        std::printf("  same session with the 1-bar walk: %d waves\n", n1);
        CHECK(n > 10 && n1 > n, "3-bar gives fewer, bigger waves than 1-bar"); }

    std::printf("%d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
