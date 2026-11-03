// tests for ZzoDeltaLogic.h (lsZzoDelta 1.0.0): recorded ES 3m bars Oct 29-30 RTH vs the Python reference (ref_waves.txt)
#include "../ZzoDeltaLogic.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
struct Raw { long long t; double h, l; long long d; long long v = 0; };
static std::vector<Raw> load(const char* p) {
    std::vector<Raw> v; std::map<int, long long> D, VV; std::ifstream f(p); std::string ln;
    while (std::getline(f, ln)) {
        if (ln.empty()) continue;
        std::stringstream ss(ln); std::string k; std::getline(ss, k, ',');
        std::vector<std::string> x; std::string c; while (std::getline(ss, c, ',')) x.push_back(c);
        if (k == "B") v.push_back({ std::stoll(x[1]), std::stod(x[3]), std::stod(x[4]), 0 });
        else if (k == "V") { D[std::stoi(x[0])] += std::stoll(x[3]); VV[std::stoi(x[0])] += std::stoll(x[2]); }
    }
    for (size_t i = 0; i < v.size(); ++i) { v[i].d = D[(int)i]; v[i].v = VV[(int)i]; }
    return v;
}
static zzd::Bar mk(const Raw& r) { zzd::Bar b; b.t = r.t; zzd::toTicks(r.h, 0.25, b.h); zzd::toTicks(r.l, 0.25, b.l); b.dlt = r.d; b.vol = r.v; b.vap = true; return b; }
int main() {
    const std::vector<Raw> R = load("zzodelta_es1029_1030.csv");
    CHECK(R.size() == 247, "fixture bars");
    zzd::Engine E; CHECK(E.setMarket("ES", 180), "ES RTH known");
    for (const Raw& r : R) E.addBar(mk(r));
    // 1) every completed wave = the reference (end bar, direction, delta, ratio text)
    std::ifstream f("ref_waves.txt"); std::string ln; size_t k = 0; int same = 0;
    while (std::getline(f, ln)) {
        int e, d; long long dl; char rr[16] = { 0 };
        if (std::sscanf(ln.c_str(), "%d,%d,%lld,%15s", &e, &d, &dl, rr) != 4) continue;
        if (k < E.W.size()) { const zzd::Wave& w = E.W[k]; std::string t = "-"; if (w.hasR) { char tb[24]; const double r = w.dir * (double)w.dlt / w.med; std::snprintf(tb, sizeof tb, "%.1f", std::fabs(r) < 0.05 ? 0.0 : r); t = tb; }
            if (w.e == e && w.dir == d && w.dlt == dl && t == rr) ++same; else std::printf("  wave %zu: got e%d d%d %lld %s / want e%d d%d %lld %s\n", k, w.e, w.dir, w.dlt, t.c_str(), e, d, dl, rr); }
        ++k;
    }
    CHECK(k == E.W.size(), "wave count = reference"); CHECK(same == (int)k, "every wave = reference");
    // 2) option 2: the RTH restart - the first waves of Oct 30 have no ratio until 3 of that direction completed that day
    int firstUp = 0, firstDn = 0; bool okRestart = true; long long day30 = 0;
    for (const zzd::Wave& w : E.W) { const long long d = E.B[(size_t)w.e].t / 86400; if (!day30 && d > E.B[0].t / 86400) day30 = d; }
    for (const zzd::Wave& w : E.W) { if (E.B[(size_t)w.e].t / 86400 != day30) continue; int& c = w.dir > 0 ? firstUp : firstDn; if (c < 3 && w.hasR) okRestart = false; if (c >= 3 && !w.hasR) okRestart = false; ++c; }
    CHECK(okRestart && firstUp >= 3 && firstDn >= 3, "RTH restart: 3 waves each way before a ratio");
    // 3) waves tile the bars: no gap, no overlap
    bool tile = E.W[0].s == 0; for (size_t i = 1; i < E.W.size(); ++i) tile = tile && E.W[i].s == E.W[i - 1].e + 1; CHECK(tile, "waves tile the bars");
    // 4) incremental = replay (no repaint of completed waves): engine fed bar by bar keeps its earlier waves unchanged
    { zzd::Engine F; F.setMarket("ES", 180); std::vector<zzd::Wave> seen; bool stable = true;
      for (const Raw& r : R) { F.addBar(mk(r)); for (size_t i = 0; i < seen.size(); ++i) { const zzd::Wave& a = seen[i], &b = F.W[i]; if (a.s != b.s || a.e != b.e || a.dlt != b.dlt || a.hasR != b.hasR || a.med != b.med) stable = false; } seen = F.W; }
      CHECK(stable, "completed waves never change"); }
    // 5) the live view: the tail after the last pivot is one live wave in the walk's direction, delta = its bars
    { std::vector<zzd::Wave> v = E.view(nullptr); CHECK(v.size() == E.W.size() + 1, "view = waves + live"); const zzd::Wave& L = v.back();
      long long s = 0; for (int i = L.s; i <= L.e; ++i) s += E.B[(size_t)i].dlt; CHECK(L.live && L.dir == E.Z.dirn && L.dlt == s && L.e == (int)E.B.size() - 1 && L.s == E.W.back().e + 1, "live wave"); }
    // 6) the forming bar: view(form) = the same as adding it as closed (minus the live flags)
    { zzd::Engine P; P.setMarket("ES", 180); for (size_t i = 0; i + 1 < R.size(); ++i) P.addBar(mk(R[i]));
      zzd::Bar fb = mk(R.back()); std::vector<zzd::Wave> v = P.view(&fb); zzd::Engine Q = P; Q.addBar(fb); std::vector<zzd::Wave> q = Q.view(nullptr);
      bool same2 = v.size() == q.size(); for (size_t i = 0; same2 && i < v.size(); ++i) same2 = v[i].s == q[i].s && v[i].e == q[i].e && v[i].dlt == q[i].dlt && v[i].hasR == q[i].hasR;
      CHECK(same2, "forming bar view = closed result"); }
    // forming-bar flips at every point of the day
    { int bad = 0; for (size_t n = 3; n < R.size(); ++n) { zzd::Engine P; P.setMarket("ES", 180); for (size_t i = 0; i < n; ++i) P.addBar(mk(R[i]));
        zzd::Bar fb = mk(R[n]); std::vector<zzd::Wave> v = P.view(&fb); zzd::Engine Q = P; Q.addBar(fb); std::vector<zzd::Wave> q = Q.view(nullptr);
        bool s2 = v.size() == q.size(); for (size_t i = 0; s2 && i < v.size(); ++i) s2 = v[i].s == q[i].s && v[i].e == q[i].e && v[i].dlt == q[i].dlt && v[i].hasR == q[i].hasR && v[i].med == q[i].med; if (!s2) ++bad; }
      CHECK(bad == 0, "forming view = closed, every bar"); }
    // 7) ratio text
    { zzd::Wave w; w.dir = -1; w.dlt = -2600; w.med = 1000; w.hasR = true; CHECK(zzd::ratioText(w) == "-2.6x", "down wave sold = -2.6x (standard sign)"); w.dlt = 400; CHECK(zzd::ratioText(w) == "+0.4x", "down wave bought = +0.4x");
      w.dlt = 20; CHECK(zzd::ratioText(w) == "0.0x", "near zero has no sign");
      w.hasR = false; CHECK(zzd::ratioText(w) == "-", "no ratio"); }
    // 8) segments: overnight / RTH / after the close differ, the evening belongs to the next session
    { zzd::Engine S; S.setMarket("ES", 180); const long long d = 20391LL * 86400;   // a Wednesday
      CHECK(S.segOf(d + 9 * 3600) != S.segOf(d + 8 * 3600), "RTH vs overnight"); CHECK(S.segOf(d + 9 * 3600) == S.segOf(d + 14 * 3600), "RTH one segment");
      CHECK(S.segOf(d + 15 * 3600 + 900) != S.segOf(d + 14 * 3600), "after the close separate"); CHECK(S.segOf(d + 18 * 3600) == S.segOf(d + 86400 + 3 * 3600), "evening = next overnight"); }
    // 9) trim keeps the history and the live wave
    { zzd::Engine T; T.setMarket("ES", 180); std::vector<zzd::Wave> ref; zzd::Engine U; U.setMarket("ES", 180);
      for (int rep = 0; rep < 30; ++rep) for (const Raw& r0 : R) { Raw r = r0; r.t += (long long)rep * 2 * 86400; T.addBar(mk(r)); U.addBar(mk(r)); }
      CHECK(T.trimmed > 0 && (int)T.B.size() <= zzd::MAX_BARS, "trimmed bars reported and capped");
      const std::vector<zzd::Wave> a = T.view(nullptr); bool ok = !a.empty() && a.back().live; for (const zzd::Wave& w : a) ok = ok && w.s >= 0 && w.e < (int)T.B.size() && w.s <= w.e; CHECK(ok, "after trim the view is in range"); }
    // 10) layout: bars only for waves with a ratio, green / red, labels inside the pane
    { zzd::View v; v.l = 0; v.t = 0; v.r = 1200; v.b = 200; v.first = 120; v.last = 246; v.bw = 9; v.X = [](int j) { return (j - 120) * 9.0 + 5; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
      const std::vector<zzd::Wave> w = E.view(nullptr); const zzd::Scene s = zzd::layout(E, w, 0, v, "ZZO Delta");
      int nb = 0; for (const zzd::Wave& x : w) if (x.hasR) for (int i = std::max(x.s, v.first); i <= std::min(x.e, v.last); ++i) ++nb;
      CHECK(s.bars == nb, "one bar per visible bar of a wave with a ratio"); bool inside = true; for (const zzd::Prim& p : s.p) if (p.k == zzd::Prim::TEXT && (p.y0 < v.t || p.y1 > v.b)) inside = false; CHECK(inside, "labels inside the pane");
      bool cols = true; for (const zzd::Prim& p : s.p) if (p.k == zzd::Prim::FILL && p.y0 != p.y1 && !(p.col == 0x22C55E || p.col == 0xEF4444)) cols = false; CHECK(cols, "bars green / red"); }
    // 11) (1.1.0) Ab? = the 36-day study's marks on these days (bar START times): Oct 29 10:00 B; Oct 30 10:00 A, 10:57 B, 12:24 A, 14:18 B
    { std::vector<std::string> got; for (const zzd::Wave& w : E.W) if (w.ab) { const long long t0 = E.B[(size_t)w.e].t - 180; char b[40]; std::snprintf(b, sizeof b, "%lld %02lld:%02lld %c", (t0 / 86400) % 100, (t0 % 86400) / 3600, (t0 % 3600) / 60, w.ab == 1 ? 'A' : 'B'); got.push_back(b); }
      std::string all; for (const std::string& g : got) all += g + "; "; std::printf("  Ab?: %s\n", all.c_str());
      int d29 = 0, d30 = 0; bool a1 = false, a2 = false, a3 = false, a4 = false, b1 = false;
      for (const zzd::Wave& w : E.W) if (w.ab) { const long long t0 = E.B[(size_t)w.e].t - 180, m = (t0 % 86400) / 60; const bool fri = t0 / 86400 == E.B.back().t / 86400; (fri ? d30 : d29)++;
        b1 = b1 || (!fri && m == 600 && w.ab == 2); a1 = a1 || (fri && m == 600 && w.ab == 1); a2 = a2 || (fri && m == 657 && w.ab == 2); a3 = a3 || (fri && m == 744 && w.ab == 1); a4 = a4 || (fri && m == 858 && w.ab == 2 && w.dir < 0); }
      CHECK(d29 == 1 && d30 == 4 && b1 && a1 && a2 && a3 && a4, "Ab? marks = the study (incl. the circled 14:18 down wave)"); }
    // 12) (1.1.0) Ab? never on a live wave; the ZigZag points: pivots in order, alternating, live last
    { const std::vector<zzd::Wave> v = E.view(nullptr); CHECK(!v.back().live || v.back().ab == 0, "no Ab? on the live wave");
      const std::vector<zzd::ZPt> z = zzd::zigzag(E, nullptr); bool ord = z.size() == E.W.size() + 2; for (size_t i = 1; i < z.size(); ++i) ord = ord && z[i].i >= z[i - 1].i;
      bool piv = true; for (size_t k = 0; k < E.W.size(); ++k) piv = piv && z[k + 1].i == E.W[k].e && z[k + 1].px == E.W[k].ext; CHECK(ord && piv && z.back().live, "ZigZag points = the waves' pivots + the live leg"); }
    // 13) (1.1.0) layout: bars green when the running total is >= 0, an Ab? box for each visible completed Ab? wave
    { zzd::View v; v.l = 0; v.t = 0; v.r = 1300; v.b = 220; v.first = 124; v.last = 246; v.bw = 9; v.X = [](int j) { return (j - 124) * 9.0 + 5; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
      const std::vector<zzd::Wave> w = E.view(nullptr); const zzd::Scene s = zzd::layout(E, w, 0, v, "ZZO Delta");
      int want = 0; for (const zzd::Wave& x : w) if (x.ab && !x.live && x.e >= v.first && x.e <= v.last) ++want; CHECK(s.marks == want && want == 4, "4 Ab? boxes drawn on Oct 30");
      bool in2 = true; for (const zzd::Prim& p : s.p) if (p.y0 < v.t || p.y1 > v.b) in2 = false; CHECK(in2, "delta view inside the pane"); }
    // 14) (1.2.0) Cmx / CoB = the 36-day study's definitions on these days (bar START times):
    //     Oct 29 Cmx 12:12 DN, 12:54 DN, 14:30 UP; CoB 14:45 DN; 14:57 UP Cmx (its pivot confirmed by the next day's first bar in this fixture)   Oct 30 Cmx 10:45 DN, 13:33 DN; CoB 13:39 UP
    { std::string got; for (const zzd::Wave& w : E.W) if (w.cmx || w.cob) { const long long t0 = E.B[(size_t)w.e].t - 180; char b[48];
          std::snprintf(b, sizeof b, "%lld %02lld:%02lld %s %s; ", (t0 / 86400) % 100, (t0 % 86400) / 3600, (t0 % 3600) / 60, w.dir > 0 ? "UP" : "DN", w.cmx ? "Cmx" : "AR"); got += b; }
      std::printf("  Cmx/CoB: %s\n", got.c_str());
      CHECK(got == "56 13:33 DN Cmx; 56 13:39 UP AR; ", "Cmx (clear trend + new low + biggest volume) / AR = the 36-day reference: only Oct 30 13:33; not 10:45 (smaller than the 09:24 wave)"); }
    // 14b) (1.1.1 volume) the BUYING climax: the same day mirrored (prices and delta flipped) gives an UP Cmx at 13:33 and a DOWN AR at 13:39
    { zzd::Engine M; M.setMarket("ES", 180);
      for (const Raw& r : R) { zzd::Bar b = mk(r); const long long h = b.h; b.h = -b.l; b.l = -h; b.dlt = -b.dlt; M.addBar(b); }
      std::string got; for (const zzd::Wave& w : M.W) if (w.cmx || w.cob) { const long long t0 = M.B[(size_t)w.e].t - 180; char b[48];
          std::snprintf(b, sizeof b, "%lld %02lld:%02lld %s %s; ", (t0 / 86400) % 100, (t0 % 86400) / 3600, (t0 % 3600) / 60, w.dir > 0 ? "UP" : "DN", w.cmx ? "Cmx" : "AR"); got += b; }
      std::printf("  mirrored: %s\n", got.c_str()); CHECK(got == "56 13:33 UP Cmx; 56 13:39 DN AR; ", "buying climax (mirror) detected the same way"); }
    // 16) (1.1.0 volume) ST = the wave right after an AR, the climax's way, <= 0.7x the climax volume, giving back >= 75% of the AR (the Python reference: Oct 30 13:51 - the Cmx 13:33 -> AR 13:39 -> ST sequence)
    { int st = 0; for (size_t k = 2; k < E.W.size(); ++k) { const zzd::Wave& w = E.W[k]; const zzd::Wave& a = E.W[k - 1]; const zzd::Wave& c = E.W[k - 2];
          const bool want = a.cob && c.cmx && c.dir == w.dir && a.seg == w.seg && c.seg == w.seg && w.vol <= 0.7 * c.vol && w.size >= 0.75 * a.size; CHECK(want == w.st, "ST rule"); st += w.st; }
      std::printf("  ST: %d\n", st); CHECK(st == 1, "one ST on these days (Oct 30 13:51)"); }
    // 15) (1.2.0) volume view: bars green on up waves / red on down waves, labels unsigned, Cmx + CoB boxes on Oct 30, Ab? still drawn
    { zzd::View v; v.volume = true; v.l = 0; v.t = 0; v.r = 1300; v.b = 240; v.first = 124; v.last = 246; v.bw = 9; v.X = [](int j) { return (j - 124) * 9.0 + 5; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; };
      const std::vector<zzd::Wave> w = E.view(nullptr); const zzd::Scene s = zzd::layout(E, w, 0, v, "ZZO Volume");
      bool unsignedLab = true; for (const zzd::Prim& p : s.p) if (p.k == zzd::Prim::TEXT && !p.text.empty() && (p.text[0] == '+' || (p.text[0] == '-' && p.text.size() > 1))) unsignedLab = false;
      CHECK(unsignedLab, "volume labels unsigned"); CHECK(s.cmx == 1 && s.cob == 1 && s.st == 1 && s.marks == 4, "Oct 30 volume view: 1 Cmx, 1 AR, 1 ST, 4 Ab?");
      bool inside = true; for (const zzd::Prim& p : s.p) if (p.y0 < v.t || p.y1 > v.b) inside = false; if (!inside) for (const zzd::Prim& p : s.p) if (p.y0 < v.t || p.y1 > v.b) std::printf("  out: %d %d %d %d %s\n", p.x0, p.y0, p.x1, p.y1, p.text.c_str()); CHECK(inside, "volume view inside the pane"); }
    std::printf("%d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
