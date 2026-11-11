// t_step130.cpp -- lsStepTrend 1.3.0 (Rassul 2026-11-11 15:08 CT "only draw the dashed line after the stp up. when a candle then pullbs back and touches
// it, without breaking the step up candle's low, it can be called a T ... you put the step on the wrong red candle next to it even though only green
// candles that close higher are suppose to have the step"; 15:12 "it should go under the low of the candle. only when it is touched or exceeded").
// Real ES bars (Oct - Nov 2026): es_60m.csv (hourly chart) and es_3m.csv (3-minute chart, must be unchanged).
//  -DOLD: built against the 1.2.0 header, prints the 3-minute layout hash (the parity reference old130.txt).
#include "StepTrendLogic.h"
#include <cstdio>
#include <cmath>
#include <ctime>
#include <string>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static std::vector<stp::Bar> load(const char* f) { std::vector<stp::Bar> B; FILE* fp = std::fopen(f, "r"); if (!fp) return B; char ts[32]; double o, h, l, c;
    while (std::fscanf(fp, "%31[^,],%lf,%lf,%lf,%lf\n", ts, &o, &h, &l, &c) == 5) { struct tm x = {}; std::sscanf(ts, "%d-%d-%d %d:%d", &x.tm_year, &x.tm_mon, &x.tm_mday, &x.tm_hour, &x.tm_min); x.tm_year -= 1900; x.tm_mon -= 1;
        stp::Bar b; b.t = (long long)timegm(&x); b.o = std::llround(o * 4); b.h = std::llround(h * 4); b.l = std::llround(l * 4); b.c = std::llround(c * 4); B.push_back(b); }
    std::fclose(fp); return B; }
static unsigned long long hashSeg(const std::vector<stp::Seg>& s, const std::vector<stp::Dot>& d) { unsigned long long h = 1469598103934665603ULL; auto mix = [&](long long v) { h ^= (unsigned long long)v; h *= 1099511628211ULL; };
    for (const stp::Seg& g : s) { mix(g.i0); mix(g.p0); mix(g.i1); mix(g.p1); mix(g.dir); mix(g.kind); } for (const stp::Dot& q : d) { mix(q.i); mix(q.px); mix(q.dir); } return h; }
int main() {
    const std::vector<stp::Bar> M = load("es_3m.csv");
    { stp::Engine E; for (const stp::Bar& b : M) E.addBar(b); std::vector<stp::Seg> s; std::vector<stp::Dot> d; stp::layout(E, (int)M.size() - 1, s, d);
#ifdef OLD
      std::printf("%llu %zu %zu\n", hashSeg(s, d), s.size(), E.S.size()); return 0;
#else
      unsigned long long oh = 0; size_t os = 0, on = 0; FILE* f = std::fopen("old130.txt", "r"); if (f) { if (std::fscanf(f, "%llu %zu %zu", &oh, &os, &on) != 3) oh = 0; std::fclose(f); }
      CHECK(oh != 0 && on >= 20 && hashSeg(s, d) == oh && s.size() == os && E.S.size() == on, "[3] the 3-minute chart draws exactly as 1.2.0 (" + std::to_string(on) + " steps)");
      int touched = 0; for (const stp::Step& q : E.S) if (q.bi >= 0 || q.tI >= 0 || q.endI >= 0) ++touched; CHECK(touched == 0, "[3] no hourly fields off the 60-minute chart");
#endif
    }
#ifndef OLD
    CHECK(std::string(stp::VERSION) == "1.3.0", "version 1.3.0");
    const std::vector<stp::Bar> H = load("es_60m.csv"); CHECK(H.size() > 300, "hourly bars loaded: " + std::to_string(H.size()));
    stp::Engine E; E.hourly = true; for (const stp::Bar& b : H) E.addBar(b);
    std::vector<stp::Seg> s; std::vector<stp::Dot> d; std::vector<stp::Mark> m; stp::layoutHourly(E, (int)H.size() - 1, s, d, m);
    int bad = 0, up = 0, dn = 0; for (const stp::Step& q : E.S) { if (q.bi < 1) { ++bad; continue; } const stp::Bar& c = H[(size_t)q.bi]; const stp::Bar& p = H[(size_t)q.bi - 1];
        if (q.dir > 0) { ++up; if (!(c.c > c.o && c.c > p.h && q.px == p.h)) ++bad; } else { ++dn; if (!(c.c < c.o && c.c < p.l && q.px == p.l)) ++bad; } }
    CHECK(bad == 0 && up >= 20 && dn >= 20, "[1] every step sits on its breakout candle: green closing above the prior high / red below the prior low (" + std::to_string(up) + " up, " + std::to_string(dn) + " down)");
    { int db = 0; for (size_t i = 0; i < d.size(); ++i) if (d[i].i != E.S[i].bi || d[i].px != E.S[i].px) ++db; CHECK(db == 0 && d.size() == E.S.size(), "[1] the dot is on the breakout candle at the broken level"); }
    // [2] the test, re-derived: the first later candle that trades below the breakout candle's low ends it (no T); else the first that touches / exceeds the level = T; the next step ends it
    int tb = 0, nT = 0, nBroken = 0; for (size_t i = 0; i < E.S.size(); ++i) { const stp::Step& q = E.S[i]; int wantT = -1, wantEnd = -1; const int stop = i + 1 < E.S.size() ? E.S[i + 1].bi : (int)H.size();
        for (int j = q.bi + 1; j < (int)H.size(); ++j) { const stp::Bar& b = H[(size_t)j];
            if (q.dir > 0 ? b.l < q.blo : b.h > q.bhi) { wantEnd = j; break; }
            if (q.dir > 0 ? b.l <= q.px : b.h >= q.px) { wantT = j; wantEnd = j; break; }
            if (j >= stop) { wantEnd = stop; break; } }
        if (wantEnd > stop) { wantEnd = stop; wantT = -1; }
        if (wantEnd == stop && wantT == stop) { /* the candle that made the next step also tested this level first: kept as a test */ }
        if (q.tI != wantT || q.endI != wantEnd) ++tb;
        if (q.tI >= 0) ++nT; else if (q.endI >= 0 && q.endI != stop) ++nBroken; }
    CHECK(tb == 0 && nT >= 10 && nBroken >= 5, "[2] T = touched or exceeded without breaking the step candle's low (" + std::to_string(nT) + " tested, " + std::to_string(nBroken) + " broken first), mismatches " + std::to_string(tb));
    { int sb = 0, dashed = 0; for (const stp::Seg& g : s) if (g.kind == 2) { ++dashed; bool ok = false; for (const stp::Step& q : E.S) if (q.bi == g.i0 && q.px == g.p0 && g.i1 > g.i0) ok = true; if (!ok) ++sb; }
      CHECK(sb == 0 && dashed >= 20, "[2] every dashed line starts at its step's breakout candle, never before (" + std::to_string(dashed) + ")"); CHECK(m.size() == (size_t)nT, "[2] one T mark per tested step"); }
    // [4] nothing decided changes: a shorter history gives the same steps / tests for everything decided by then
    { int rp = 0; for (size_t cut = 60; cut < H.size(); cut += 37) { stp::Engine P; P.hourly = true; for (size_t k = 0; k < cut; ++k) P.addBar(H[k]);
          for (size_t i = 0; i < P.S.size(); ++i) { const stp::Step& a = P.S[i]; const stp::Step& b = E.S[i]; if (a.bi != b.bi || a.px != b.px || a.dir != b.dir) ++rp; if (a.endI >= 0 && (a.endI != b.endI || a.tI != b.tI)) ++rp; } }
      CHECK(rp == 0, "[4] no repaint: steps and decided tests are the same from any earlier point"); }
    std::printf("t_step130 (lsStepTrend %s): %d passed, %d failed\n", stp::VERSION, passes, fails); return fails ? 1 : 0;
#endif
}
