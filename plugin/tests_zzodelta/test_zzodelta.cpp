// tests for ZzoDeltaLogic.h (lsZzoDelta 1.0.0): recorded ES 3m bars Oct 29-30 RTH vs the Python reference (ref_waves.txt)
#include "../ZzoDeltaLogic.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
struct Raw { long long t; double h, l; long long d; };
static std::vector<Raw> load(const char* p) {
    std::vector<Raw> v; std::map<int, long long> D; std::ifstream f(p); std::string ln;
    while (std::getline(f, ln)) {
        if (ln.empty()) continue;
        std::stringstream ss(ln); std::string k; std::getline(ss, k, ',');
        std::vector<std::string> x; std::string c; while (std::getline(ss, c, ',')) x.push_back(c);
        if (k == "B") v.push_back({ std::stoll(x[1]), std::stod(x[3]), std::stod(x[4]), 0 });
        else if (k == "V") D[std::stoi(x[0])] += std::stoll(x[3]);
    }
    for (size_t i = 0; i < v.size(); ++i) v[i].d = D[(int)i];
    return v;
}
static zzd::Bar mk(const Raw& r) { zzd::Bar b; b.t = r.t; zzd::toTicks(r.h, 0.25, b.h); zzd::toTicks(r.l, 0.25, b.l); b.dlt = r.d; b.vap = true; return b; }
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
        if (k < E.W.size()) { const zzd::Wave& w = E.W[k]; std::string t = zzd::ratioText(w); if (t.size() && t.back() == 'x') t.pop_back();
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
    { zzd::Wave w; w.dir = -1; w.dlt = -2600; w.med = 1000; w.hasR = true; CHECK(zzd::ratioText(w) == "2.6x", "down wave sold 2.6x"); w.dlt = 400; CHECK(zzd::ratioText(w) == "-0.4x", "down wave bought -0.4x");
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
    std::printf("%d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
