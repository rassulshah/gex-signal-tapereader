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
static long long floorDivT(long long t) { return stp::floorDiv(t, 3600); }
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
    CHECK(std::string(stp::VERSION) == "1.5.1", "version 1.5.1");
    const std::vector<stp::Bar> H = load("es_60m.csv"); CHECK(H.size() > 300, "hourly bars loaded: " + std::to_string(H.size()));
    stp::Engine E; E.hourly = true; for (const stp::Bar& b : H) E.addBar(b);
    std::vector<stp::Seg> s; std::vector<stp::Dot> d; std::vector<stp::Mark> m; stp::layoutHourly(E, (int)H.size() - 1, s, d, m);
    int bad = 0, up = 0, dn = 0; for (const stp::Step& q : E.S) { if (q.bi < 1) { ++bad; continue; } const stp::Bar& c = H[(size_t)q.bi]; const stp::Bar& p = H[(size_t)q.bi - 1];
        if (q.dir > 0) { ++up; if (!(c.c > c.o && c.c > p.h && q.px == p.h)) ++bad; } else { ++dn; if (!(c.c < c.o && c.c < p.l && q.px == p.l)) ++bad; } }
    CHECK(bad == 0 && up >= 20 && dn >= 20, "[1] every step sits on its breakout candle: green closing above the prior high / red below the prior low (" + std::to_string(up) + " up, " + std::to_string(dn) + " down)");
    { int db = 0; for (size_t i = 0; i < d.size(); ++i) if (d[i].i != E.S[i].bi || d[i].px != (E.S[i].dir > 0 ? H[(size_t)E.S[i].bi].h : H[(size_t)E.S[i].bi].l)) ++db; CHECK(db == 0 && d.size() == E.S.size(), "[1] (1.3.1) the dot is on the step candle at its high (low for a step down)");
      int sb = 0, solid = 0; for (const stp::Seg& g : s) if (g.kind == 0) { ++solid; bool ok = false; for (size_t i = 1; i < E.S.size(); ++i) { const stp::Step& p = E.S[i - 1]; const stp::Step& q = E.S[i]; const long long pt = p.dir > 0 ? p.bhi : p.blo, qt = q.dir > 0 ? q.bhi : q.blo;
              const bool flip = p.dir != q.dir; const int at = flip ? q.bi - 1 : p.bi; const long long fr = flip ? q.px : pt;   // (1.4.5) a flip: at the candle before the step candle, from the broken level (was 1.4.3: the other end of the last step candle)
              if (flip && !(q.dir > 0 ? (fr == H[(size_t)at].h && qt > fr) : (fr == H[(size_t)at].l && qt < fr))) continue;   // the broken level IS that candle's high (low) and the line points the step's way
              if ((g.i0 == at && g.i1 == at && g.p0 == fr && g.p1 == qt) || (g.i0 == at && g.i1 == q.bi && g.p0 == qt && g.p1 == qt)) ok = true; } if (!ok) ++sb; }
      CHECK(sb == 0 && solid == 2 * ((int)E.S.size() - 1), "[1] (1.3.2) up at the former step candle to the next high, then across; was: across at the prior step candle's high to the next step candle, then up to its high"); }
    // [2] the test, re-derived: the first later candle that trades below the breakout candle's low ends it (no T); else the first that touches / exceeds the level = T; the next step ends it
    int tb = 0, nT = 0, nBroken = 0; for (size_t i = 0; i < E.S.size(); ++i) { const stp::Step& q = E.S[i]; int wantT = -1, wantEnd = -1; const int stop = i + 1 < E.S.size() ? E.S[i + 1].bi : (int)H.size();
        for (int j = q.bi + 1; j < (int)H.size(); ++j) { const stp::Bar& b = H[(size_t)j];
            if (q.dir > 0 ? b.c < q.blo : b.c > q.bhi) { wantEnd = j; break; }   // (1.3.3) by the close
            if (q.dir > 0 ? b.l <= q.px : b.h >= q.px) { wantT = j; wantEnd = j; break; }
            if (j >= stop) { wantEnd = stop; break; } }
        if (wantEnd > stop) { wantEnd = stop; wantT = -1; }
        if (wantEnd == stop && wantT == stop) { /* the candle that made the next step also tested this level first: kept as a test */ }
        if (q.tI != wantT || q.endI != wantEnd) ++tb;
        if (q.tI >= 0) ++nT; else if (q.endI >= 0 && q.endI != stop) ++nBroken; }
    CHECK(tb == 0 && nT >= 10 && nBroken >= 1, "[2] T = touched or exceeded without breaking the step candle's low (" + std::to_string(nT) + " tested, " + std::to_string(nBroken) + " broken first), mismatches " + std::to_string(tb));
    { int sb = 0, dashed = 0; for (const stp::Seg& g : s) if (g.kind == 2) { ++dashed; bool ok = false; for (const stp::Step& q : E.S) if (q.bi - 1 == g.i0 && q.px == g.p0 && g.i1 > g.i0 && (q.dir > 0 ? H[(size_t)g.i0].h : H[(size_t)g.i0].l) == g.p0) ok = true; if (!ok) ++sb; }
      CHECK(sb == 0 && dashed >= 20, "[2] (1.3.1) every dashed line starts at the prior high's candle, at that high, never before (" + std::to_string(dashed) + ")"); int mt = 0, mb = 0; for (const stp::Mark& x : m) (x.kind == 1 ? mb : mt)++; CHECK(mt == nT, "[2] one T mark per tested step");
      // [5] (1.4.0) B = the first candle after the step candle, while it is the newest step, that closes below its low (above its high for a step down)
      int bb = 0, nB = 0; for (size_t i = 0; i < E.S.size(); ++i) { const stp::Step& q = E.S[i]; int want = -1; const int stop = i + 1 < E.S.size() ? E.S[i + 1].bi : (int)H.size() - 1;
          for (int j = q.bi + 1; j <= stop && j < (int)H.size(); ++j) { if (q.dir > 0 ? H[(size_t)j].c < q.blo : H[(size_t)j].c > q.bhi) { want = j; break; } }
          if (q.brI != want) ++bb;
          if (q.brI >= 0) ++nB; }
      CHECK(bb == 0 && nB >= 5 && mb == nB, "[5] B = the first close beyond the newest step candle's low / high (" + std::to_string(nB) + " breaks), mismatches " + std::to_string(bb)); }
    // [4] nothing decided changes: a shorter history gives the same steps / tests for everything decided by then
    { int rp = 0; for (size_t cut = 60; cut < H.size(); cut += 37) { stp::Engine P; P.hourly = true; for (size_t k = 0; k < cut; ++k) P.addBar(H[k]);
          for (size_t i = 0; i < P.S.size(); ++i) { const stp::Step& a = P.S[i]; const stp::Step& b = E.S[i]; if (a.bi != b.bi || a.px != b.px || a.dir != b.dir) ++rp; if (a.endI >= 0 && (a.endI != b.endI || a.tI != b.tI)) ++rp; if (a.brI >= 0 && a.brI != b.brI) ++rp; } }
      CHECK(rp == 0, "[4] no repaint: steps and decided tests are the same from any earlier point"); }
    {   // ---- (1.5.0, Rassul 2026-11-11 19:32 CT "the 3m ltf is inconsistent with the 60m steps") the 3-minute chart = the 60-minute design ----
        stp::Engine E3; for (const stp::Bar& b : M) E3.addBar(b);
        std::vector<stp::Seg> s3; std::vector<stp::Dot> d3; std::vector<stp::Mark> m3; const int last3 = (int)M.size() - 1; stp::layoutLtf(E3, last3, s3, d3, m3, true);
        { std::vector<stp::Seg> sx; std::vector<stp::Dot> dx; std::vector<stp::Mark> mx; stp::layoutLtf(E3, last3, sx, dx, mx); bool eq = sx.size() == s3.size() && dx.size() == d3.size(); for (size_t i = 0; eq && i < sx.size(); ++i) eq = sx[i].i0 == s3[i].i0 && sx[i].i1 == s3[i].i1 && sx[i].p0 == s3[i].p0 && sx[i].p1 == s3[i].p1;
          CHECK(mx.empty() && eq && !m3.empty(), "[L0] (1.5.1) as drawn: NO T / B letters on the 3-minute chart, the lines exactly the same (" + std::to_string(m3.size()) + " letters left out)"); }
        const int nh = (int)E3.H.size() - (E3.curClosed ? 0 : 1);
        stp::Engine EH; EH.hourly = true; std::vector<int> st(E3.H.size() + 1, 0); for (size_t j = 0; j < E3.H.size(); ++j) st[j + 1] = st[j] + E3.H[j].n;
        for (int j = 0; j < nh; ++j) { const stp::Hour& h = E3.H[(size_t)j]; stp::Bar b; b.t = h.k * 3600; b.o = h.o; b.h = h.h; b.l = h.l; b.c = h.c; EH.addBar(b); }
        EH.sealTo((E3.H[(size_t)nh - 1].k + 1) * 3600);
        std::vector<stp::Seg> sh; std::vector<stp::Dot> dh; std::vector<stp::Mark> mh; stp::layoutHourly(EH, nh - 1, sh, dh, mh);
        bool same = EH.S.size() == E3.S.size(); for (size_t i = 0; same && i < EH.S.size(); ++i) same = EH.S[i].dir == E3.S[i].dir && EH.S[i].px == E3.S[i].px && EH.S[i].hour == E3.S[i].hour;
        CHECK(same && EH.S.size() >= 20, "[L1] the hourly walk of the 3-minute chart's own hours finds the same steps as the 3-minute engine (" + std::to_string(EH.S.size()) + ")");
        bool dOk = d3.size() == dh.size(); for (size_t i = 0; dOk && i < d3.size(); ++i) dOk = d3[i].px == dh[i].px && d3[i].dir == dh[i].dir;
        CHECK(dOk && !d3.empty(), "[L2] the same dots at the same prices as the 60-minute chart (" + std::to_string(d3.size()) + ")");
        auto pick = [](const std::vector<stp::Seg>& v, int kind) { std::vector<stp::Seg> o; for (const stp::Seg& g : v) if ((g.kind == 2) == (kind == 2)) o.push_back(g); return o; };
        { const std::vector<stp::Seg> a = pick(s3, 0), b = pick(sh, 0); bool ok = a.size() == b.size(); for (size_t i = 0; ok && i < a.size(); ++i) ok = a[i].p0 == b[i].p0 && a[i].p1 == b[i].p1 && a[i].dir == b[i].dir;
          CHECK(ok && !a.empty(), "[L3] the same solid lines (prices, colours, order) as the 60-minute chart (" + std::to_string(a.size()) + ")"); }
        { const std::vector<stp::Seg> a = pick(s3, 2), b = pick(sh, 2); bool ok = a.size() == b.size(); for (size_t i = 0; ok && i < a.size(); ++i) ok = a[i].p0 == b[i].p0 && a[i].dir == b[i].dir;
          CHECK(ok && !a.empty(), "[L4] the same dashed test lines at the same levels (" + std::to_string(a.size()) + " vs " + std::to_string(b.size()) + ")"); }
        { bool ok = m3.size() == mh.size(); int t = 0, br = 0; for (size_t i = 0; ok && i < m3.size(); ++i) { ok = m3[i].kind == mh[i].kind && m3[i].dir == mh[i].dir; (m3[i].kind ? br : t)++; }
          CHECK(ok && t >= 5 && br >= 3, "[L5] the same T and B marks as the 60-minute chart (" + std::to_string(t) + " T, " + std::to_string(br) + " B)"); }
        { int bad = 0; for (const stp::Seg& g : s3) if (g.i0 < 0 || g.i1 > last3 || g.i0 > g.i1) ++bad; for (const stp::Dot& q : d3) if (q.i < 0 || q.i > last3 || (q.dir > 0 ? M[(size_t)q.i].h != q.px : M[(size_t)q.i].l != q.px)) ++bad;
          CHECK(bad == 0, "[L6] every line runs left to right on real candles; every dot sits on the candle that MADE the step hour's high (low) (" + std::to_string(bad) + " bad)"); }
        { int bad = 0, seen = 0; for (const stp::Mark& k : m3) { const stp::Bar& b = M[(size_t)k.i]; if (k.kind == 0) { ++seen; bool hit = false; for (const stp::Seg& g : s3) if (g.kind == 2 && g.i1 == k.i && g.dir == k.dir && (k.dir > 0 ? b.l <= g.p0 : b.h >= g.p0)) hit = true; if (!hit) ++bad; }
              else if (floorDivT(b.t + 180) == floorDivT(b.t)) ++bad; }   // a B sits on the LAST candle of its hour
          CHECK(bad == 0 && seen > 0, "[L7] each T is on the candle that touched its test line (the line ends there); each B on the last candle of its hour (" + std::to_string(bad) + " bad)"); }
        {   // nothing decided changes: the layout of the first 3,000 candles is the start of the full layout (dots, marks); a forming hour adds nothing
            stp::Engine P; for (int i = 0; i < 3000; ++i) P.addBar(M[(size_t)i]); std::vector<stp::Seg> sp; std::vector<stp::Dot> dp; std::vector<stp::Mark> mp; stp::layoutLtf(P, 2999, sp, dp, mp, true);
            bool ok = dp.size() <= d3.size() && !dp.empty(); for (size_t i = 0; ok && i < dp.size(); ++i) ok = dp[i].i == d3[i].i && dp[i].px == d3[i].px;
            int miss = 0; for (const stp::Mark& k : mp) { bool f = false; for (const stp::Mark& q : m3) if (q.i == k.i && q.kind == k.kind && q.dir == k.dir) f = true; if (!f) ++miss; }
            CHECK(ok && miss == 0, "[L8] no repaint: what 3,000 candles drew is still drawn, unchanged, with all 6,000 (" + std::to_string(dp.size()) + " dots, " + std::to_string(mp.size()) + " marks, " + std::to_string(miss) + " changed)");
            int k = 3000; const long long hr = floorDivT(M[2999].t); while (k < (int)M.size() && floorDivT(M[(size_t)k].t) == hr) { P.addBar(M[(size_t)k]); ++k; }   // finish hour A
            const long long hb = floorDivT(M[(size_t)k].t); P.addBar(M[(size_t)k]); ++k;   // 1 candle of hour B: A is closed, B is forming
            std::vector<stp::Seg> s1; std::vector<stp::Dot> d1; std::vector<stp::Mark> m1; stp::layoutLtf(P, k - 1, s1, d1, m1, true);
            int cnt = 1; while (k + 1 < (int)M.size() && floorDivT(M[(size_t)k + 1].t) == hb) { P.addBar(M[(size_t)k]); ++k; ++cnt; }   // ... all but the last candle of hour B
            std::vector<stp::Seg> s2; std::vector<stp::Dot> d2; std::vector<stp::Mark> m2; stp::layoutLtf(P, k - 1, s2, d2, m2, true);
            bool eq = d1.size() == d2.size() && m1.size() == m2.size(); for (size_t i = 0; eq && i < d1.size(); ++i) eq = d1[i].i == d2[i].i && d1[i].px == d2[i].px; for (size_t i = 0; eq && i < m1.size(); ++i) eq = m1[i].i == m2[i].i && m1[i].kind == m2[i].kind;
            CHECK(cnt >= 3 && eq, "[L9] the candles of a FORMING hour change no dot and no mark (" + std::to_string(cnt) + " candles into the hour)"); }
    }
    std::printf("t_step130 (lsStepTrend %s): %d passed, %d failed\n", stp::VERSION, passes, fails); return fails ? 1 : 0;
#endif
}
