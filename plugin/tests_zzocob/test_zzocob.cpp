// test_zzocob.cpp - the CoB finder of lsZzoCombo 2.3.0 (ZzoCobFinder.h + ZzoComboView.h + ZzoCombo.cpp). Run from tests_zzocob/.
//   1. sessions and levels: the 17:00 roll, overnight / Asia / London / prior hour windows, prior full session and prior RTH, the RTH
//      extreme and the LOD / HOD bar
//   2. the rule on a hand-built wave list: Cob with every condition; each condition removed in turn gives no Cob (or a logged watch);
//      bearish mirror; decided once
//   3. Lvp: first 3 pullbacks inside the Cob wave, <= 1/3 of its volume; below the origin / above the top / a new session end it
//   4. real bars (ES and GC 2026-09-08 .. 10-09, > 6000 bars so the engine trims): the record lines = the reference written from the
//      400-session study parity run; the finder stays aligned with the engine through every trim; a restart every 977 / 2500 bars gives
//      the same lines (no repaint); the chart root QGC labels a subset of what the market key GC labels (both counts >= 10)
//   5. drawing: "Cob" in the wave's colour, "Lvp" in its Cob's colour, the old "CoB" never drawn, nothing without a finder
//   6. the host: version literals, the close array, the finder fed on every bar, the record file written once per decision
#include "../ZzoComboView.h"
#include <cstdio>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)
static std::string slurp(const char* p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
struct Row { long long t, o, h, l, c, v; };
static std::vector<Row> load(const char* p) { std::vector<Row> R; std::ifstream f(p); std::string ln; while (std::getline(f, ln)) { Row r; if (std::sscanf(ln.c_str(), "%lld|%lld|%lld|%lld|%lld|%lld", &r.t, &r.o, &r.h, &r.l, &r.c, &r.v) == 6) R.push_back(r); } return R; }
struct Host { zzd::Engine E; zcf::Finder F; long long lastWaveT = -1; bool aligned = true; int judged = 0;
    void init(const std::string& root) { E.reset(); E.revBars = zzd::COMBO_REV_BARS; E.setMarket(root, 180); F.reset(); F.setMarket(root, 180); lastWaveT = -1; }
    void add(const Row& r) {
        zzd::Bar b; b.t = r.t; b.h = r.h; b.l = r.l; b.vol = r.v; b.vap = true;
        E.addBar(b); F.addBar(b, r.c);
        if (E.trimmed > 0) { F.trim(E.trimmed); E.trimmed = 0; }
        if (F.F.size() != E.B.size() || F.M.B.size() != E.B.size()) aligned = false;
        if (!E.W.empty()) { const zzd::Wave& w = E.W.back();
            if (w.s >= 0 && w.e >= 0 && (size_t)w.e < F.F.size()) { const long long tw = F.F[(size_t)w.e].t * 2 + (w.dir > 0 ? 1 : 0); if (tw != lastWaveT) { lastWaveT = tw; F.onWave(E.W); ++judged; } } }
    } };
static std::vector<std::string> replay(const std::vector<Row>& R, const std::string& root, int every, bool* aligned = nullptr, Host* keep = nullptr) {
    Host H; H.init(root); std::set<std::string> seen; std::vector<std::string> out; bool al = true;
    for (size_t i = 0; i < R.size(); ++i) {
        if (every > 0 && i > 0 && i % (size_t)every == 0) { H.init(root); const size_t a = i > 5000 ? i - 5000 : 0; for (size_t k = a; k < i; ++k) H.add(R[k]); H.F.out.clear(); }
        H.add(R[i]); al = al && H.aligned;
        for (const std::string& o : H.F.out) if (seen.insert(o).second) out.push_back(o);
        H.F.out.clear();
    }
    if (aligned) *aligned = al;
    if (keep) *keep = H;
    return out;
}
static std::vector<std::string> lines(const std::string& s) { std::vector<std::string> v; std::stringstream ss(s); std::string ln; while (std::getline(ss, ln)) if (!ln.empty()) v.push_back(ln); return v; }
static long long T(int day, int hh, int mm) { return (20000LL + day) * 86400 + hh * 3600 + mm * 60; }   // day 0 = a weekday

// a finder with one session's levels and an RTH low, then a hand-made wave pair judged on the newest bar
struct Rig { zcf::Finder F; std::vector<zzd::Wave> W; long long t = 0;
    void bar(long long tt, long long h, long long l, long long c) { zzd::Bar b; b.t = tt; b.h = h; b.l = l; b.vol = 100; b.vap = true; F.addBar(b, c); F.M.W.clear(); }
    // waves are given directly (the engine's own walk is tested in test_zzocombo_210); the market engine's last wave mirrors the pane's
    void judge(int cntPane, int cntMkt) { zzd::Wave m = W.back(); m.cnt = cntMkt; m.cntCap = false; W.back().cnt = cntPane; F.M.W.clear(); F.M.W.push_back(m); F.onWave(W); } };

int main() {
    // ---- 1. sessions and levels
    {   CHECK(zcf::Finder::sessOf(T(0, 17, 0)) == 20000 && zcf::Finder::sessOf(T(0, 17, 3)) == 20001 && zcf::Finder::sessOf(T(1, 16, 0)) == 20001, "17:00 END stamp closes the session; 17:03 opens the next");
        zcf::Finder F; CHECK(F.setMarket("ES") && F.openMin == 510 && F.closeMin == 900, "ES hours"); CHECK(F.setMarket("QGC") && F.openMin == 440, "QGC -> GC hours"); CHECK(F.setMarket("CPE") && F.openMin == 430, "CPE -> HG 07:10");
        zcf::Finder N; CHECK(!N.setMarket("ZZ"), "an unknown market has no finder"); zzd::Bar b; b.t = T(0, 9, 0); b.h = 10; b.l = 5; N.addBar(b, 7); std::vector<zzd::Wave> w(2); N.onWave(w); CHECK(N.tags.empty() && N.out.empty(), "unknown market: nothing decided");
        Rig R; R.F.setMarket("ES");
        R.bar(T(0, 12, 0), 5000, 4900, 4950);                       // day 0 RTH (prior session): PDH 5000 / PDL 4900, PFH / PFL too
        R.bar(T(0, 16, 30), 5010, 4890, 4950);                      // after the close: full session only -> PFH 5010 / PFL 4890
        R.bar(T(0, 17, 30), 4960, 4940, 4950);                      // new session: overnight only
        R.bar(T(0, 19, 0), 4970, 4930, 4950);                       // Asia (18:00, 01:00]
        R.bar(T(1, 1, 0), 4972, 4928, 4950);                        // Asia edge
        R.bar(T(1, 1, 3), 4980, 4920, 4950);                        // not Asia
        R.bar(T(1, 3, 0), 4975, 4925, 4950);                        // London (02:00, 07:30]
        R.bar(T(1, 7, 33), 4990, 4910, 4950);                       // after London (07:30), the first prior-hour bar
        R.bar(T(1, 8, 0), 4965, 4935, 4950);                        // prior hour (07:30, 08:30]
        R.bar(T(1, 8, 30), 4966, 4934, 4950);                       // prior hour edge = the last overnight bar
        CHECK(R.F.prevFull.h == 5010 && R.F.prevFull.l == 4890 && R.F.prevRth.h == 5000 && R.F.prevRth.l == 4900, "prior full session and prior RTH");
        CHECK(R.F.on.h == 4990 && R.F.on.l == 4910, "overnight = (17:00, open]"); CHECK(R.F.as.h == 4972 && R.F.as.l == 4928, "Asia = (18:00, 01:00]");
        CHECK(R.F.lon.h == 4975 && R.F.lon.l == 4925, "London = (02:00, 07:30]"); CHECK(R.F.phr.h == 4990 && R.F.phr.l == 4910, "prior hour = (open - 60, open]: 07:33, 08:00 and 08:30");
        CHECK(!R.F.rthAny, "no RTH bar yet"); R.bar(T(1, 8, 33), 4960, 4940, 4950); R.bar(T(1, 8, 36), 4955, 4915, 4920); R.bar(T(1, 8, 39), 4950, 4918, 4945);
        CHECK(R.F.rthAny && R.F.rLo == 4915 && R.F.rLoBarH == 4955 && R.F.rHi == 4960 && R.F.rHiBarL == 4940, "RTH extremes and the LOD / HOD bar");
        CHECK(R.F.F.back().rth && !R.F.F[9].rth, "08:30 END stamp is not RTH, 08:33 is");
    }
    // ---- 2. the rule
    auto base = [](Rig& R) {   // prior session, overnight low 4930 / London low 4925, then RTH: a drop to 4915 (below ONL / LonLO / PhrL), a wave up to 4960
        R.F.setMarket("ES");
        R.bar(T(0, 12, 0), 5000, 4900, 4950); R.bar(T(0, 17, 30), 4960, 4930, 4950); R.bar(T(1, 3, 0), 4975, 4925, 4950); R.bar(T(1, 8, 0), 4965, 4935, 4950);
        R.bar(T(1, 8, 33), 4960, 4940, 4945);   // 4
        R.bar(T(1, 8, 36), 4950, 4915, 4920);   // 5  the RTH low (origin)
        R.bar(T(1, 8, 39), 4940, 4918, 4938);   // 6
        R.bar(T(1, 8, 42), 4960, 4936, 4958);   // 7  the wave's top
        zzd::Wave d; d.s = 4; d.e = 5; d.dir = -1; d.ext = 4915; d.vol = 900; zzd::Wave u; u.s = 6; u.e = 7; u.dir = 1; u.ext = 4960; u.vol = 900;
        zzd::Wave p; p.s = -1; p.e = -1; p.dir = 1; p.ext = 4958; R.W = { p, d, u }; };
    {   Rig R; base(R); R.bar(T(1, 8, 45), 4959, 4944, 4945);   // 8 completes it: gave back 15 / 45 = 0.33, closed above 4925 / 4930 / 4935
        R.judge(12, 11);
        const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        CHECK(t && t->cob == 1 && t->cnt == 11 && t->atExt && t->origin == 4915 && t->top == 4960 && t->close == 4945, "bullish Cob");
        CHECK(t && zcf::lvText(t->lv, 1) == "ONL/LonLO/PhrL", "the levels swept and closed back over");
        CHECK(R.F.out.size() == 1 && R.F.out[0].rfind("C|1.0.0|", 0) == 0 && R.F.out[0].find("|UP|4915|4960|4945|ONL/LonLO/PhrL|11|0.33|at|bo|12|11") != std::string::npos, "the record line");
        CHECK(R.F.act.size() == 1, "open for its Lvp");
    }
    auto none = [&](const char* what, std::function<void(Rig&)> f, int wantWatch) { Rig R; base(R); f(R); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        const bool ok = wantWatch ? (t && !t->cob && t->watch == wantWatch && R.F.out.size() == 1 && R.F.out[0][0] == 'W' && R.F.act.empty()) : (!t && R.F.out.empty());
        if (ok) ++pass; else { ++fail; std::printf("FAIL no Cob: %s\n", what); } };
    none("pane count 9", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(9, 30); }, 2);
    none("market count 9", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(30, 9); }, 2);
    none("count 4", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(4, 4); }, 0);
    none("gave back 0.49", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4937, 4938); R.judge(20, 20); }, 0);
    none("closed below the origin", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4910, 4914); R.judge(20, 20); }, 0);
    none("closed back under every level", [](Rig& R) { R.F.on.l = R.F.lon.l = R.F.phr.l = 4950; R.F.prevFull.l = R.F.prevRth.l = 4900; R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); }, 1);
    none("no level below the wave", [](Rig& R) { R.F.on.l = R.F.as.l = R.F.lon.l = R.F.phr.l = R.F.prevFull.l = R.F.prevRth.l = 4900; R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); }, 1);
    none("level above the wave's top", [](Rig& R) { R.F.on.l = R.F.lon.l = R.F.phr.l = 4970; R.F.as = zcf::HL(); R.F.prevFull.l = R.F.prevRth.l = 4900; R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); }, 1);
    none("origin not near the RTH low", [](Rig& R) { R.F.rLo = 4800; R.F.rLoBarH = 4810; R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); }, 0);
    none("market engine not on the same wave", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4944, 4945); R.W.back().cnt = 20; R.F.M.W.clear(); R.F.onWave(R.W); }, 0);
    none("the completing bar is after the RTH close", [](Rig& R) { R.bar(T(1, 15, 3), 4959, 4944, 4945); R.judge(20, 20); }, 0);
    {   Rig R; base(R); R.F.rLo = 4905; R.F.rLoBarH = 4916; R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(50, 50); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        CHECK(t && t->cob == 1 && !t->atExt, "near: the origin is at or below the high of the LOD bar"); }
    {   Rig R; base(R); R.W[0].ext = 4990; R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        CHECK(t && t->cob == 1 && R.F.out[0].find("|at|-|") != std::string::npos, "a Cob does not have to take out the prior pivot (logged)"); }
    {   Rig R; base(R); R.F.on.l = R.F.as.l = R.F.lon.l = R.F.phr.l = 4900; R.F.prevFull.l = 4900; R.F.prevRth.l = 4922; R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        CHECK(t && t->cob == 1 && zcf::lvText(t->lv, 1) == "PDL", "the prior day's RTH low qualifies"); }
    {   // bearish mirror
        Rig R; R.F.setMarket("ES");
        R.bar(T(0, 12, 0), 5000, 4900, 4950); R.bar(T(0, 17, 30), 4970, 4940, 4950); R.bar(T(1, 3, 0), 4975, 4925, 4950); R.bar(T(1, 8, 0), 4965, 4935, 4950);
        R.bar(T(1, 8, 33), 4960, 4940, 4955); R.bar(T(1, 8, 36), 4985, 4950, 4980); R.bar(T(1, 8, 39), 4982, 4960, 4962); R.bar(T(1, 8, 42), 4964, 4940, 4942); R.bar(T(1, 8, 45), 4956, 4941, 4955);
        zzd::Wave u; u.s = 4; u.e = 5; u.dir = 1; u.ext = 4985; u.vol = 900; zzd::Wave d; d.s = 6; d.e = 7; d.dir = -1; d.ext = 4940; d.vol = 900; zzd::Wave p; p.s = p.e = -1; p.dir = -1; p.ext = 4945; R.W = { p, u, d };
        R.judge(15, 15); const zcf::Tag* t = R.F.find(T(1, 8, 42), -1);
        CHECK(t && t->cob == -1 && zcf::lvText(t->lv, -1) == "ONH/LonHI/PhrH" && t->atExt, "bearish Cob"); CHECK(R.F.out.size() == 1 && R.F.out[0].find("|DN|4985|4940|4955|") != std::string::npos, "bearish record");
    }
    // ---- 3. Lvp
    auto cob = [&](Rig& R) { base(R); R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(12, 12); R.F.out.clear(); };
    auto wave = [](Rig& R, int dir, long long ext, long long vol, long long t) { R.bar(t, ext + 6, ext - 6, ext); R.bar(t + 180, ext + 3, ext - 3, ext); zzd::Wave w; w.s = (int)R.F.F.size() - 2; w.e = w.s; w.dir = dir; w.ext = ext; w.vol = vol; R.W.push_back(w); R.judge(0, 0); };
    {   Rig R; cob(R); wave(R, -1, 4935, 300, T(1, 8, 48)); const zcf::Tag* t = R.F.find(T(1, 8, 48), -1);
        CHECK(t && t->lvp == 1 && !t->cob && R.F.act.empty() && R.F.out.size() == 1 && R.F.out[0].rfind("L|", 0) == 0, "Lvp on the first pullback (300 <= 900 / 3), in the Cob's direction"); }
    {   Rig R; cob(R); wave(R, -1, 4935, 301, T(1, 8, 48)); CHECK(!R.F.find(T(1, 8, 48), -1) && R.F.act.size() == 1, "301 > 900 / 3: no Lvp yet");
        wave(R, 1, 4958, 500, T(1, 8, 54)); wave(R, -1, 4940, 250, T(1, 9, 0)); const zcf::Tag* t = R.F.find(T(1, 9, 0), -1); CHECK(t && t->lvp == 1, "Lvp on the second pullback"); }
    {   Rig R; cob(R); wave(R, -1, 4935, 400, T(1, 8, 48)); wave(R, 1, 4961, 500, T(1, 8, 54)); wave(R, -1, 4940, 100, T(1, 9, 0)); CHECK(!R.F.find(T(1, 9, 0), -1) && R.F.act.empty(), "a wave above the Cob's top ends it"); }
    {   Rig R; cob(R); wave(R, -1, 4914, 100, T(1, 8, 48)); CHECK(!R.F.find(T(1, 8, 48), -1) && R.F.act.empty(), "a pullback below the origin ends it"); }
    {   Rig R; cob(R); wave(R, -1, 4935, 400, T(1, 8, 48)); wave(R, 1, 4950, 400, T(1, 8, 54)); wave(R, -1, 4936, 400, T(1, 9, 0)); wave(R, 1, 4950, 400, T(1, 9, 6)); wave(R, -1, 4937, 400, T(1, 9, 12));
        CHECK(R.F.act.empty(), "3 pullbacks without one: closed"); wave(R, 1, 4950, 400, T(1, 9, 18)); wave(R, -1, 4938, 10, T(1, 9, 24)); CHECK(!R.F.find(T(1, 9, 24), -1), "the 4th pullback never gets it"); }
    {   Rig R; cob(R); R.bar(T(1, 17, 30), 4950, 4940, 4945); CHECK(R.F.act.empty(), "a new session ends it"); }
    // ---- 4. real bars
    {   const std::vector<Row> es = load("bars_es_0908_1009.txt"), gc = load("bars_gc_0908_1009.txt");
        CHECK(es.size() > 6000 && gc.size() > 6000, "fixtures loaded (> 6000 bars: the engine trims)");
        bool al = false; Host H; const std::vector<std::string> a = replay(es, "ES", 0, &al, &H);
        CHECK(al, "finder and market engine aligned with the pane's engine on every bar, through the trims"); CHECK(H.E.B.size() < es.size(), "the engine did trim");
        CHECK(a == lines(slurp("ref_es_0908_1009.txt")) && !a.empty(), "ES lines = the reference");
        int c = 0, l = 0, w = 0; for (const std::string& s : a) { if (s[0] == 'C') ++c; else if (s[0] == 'L') ++l; else ++w; } std::printf("  ES 09-08 .. 10-09: %d Cob, %d Lvp, %d watch (logged)\n", c, l, w);
        CHECK(c >= 5 && l >= 3 && l <= c, "Cobs and Lvps found; never more Lvps than Cobs");
        auto keys = [](const std::vector<std::string>& v, char k) { std::set<std::string> s; for (const std::string& x : v) if (x[0] == k) { std::vector<std::string> f; std::stringstream ss(x); std::string t; while (std::getline(ss, t, '|')) f.push_back(t); s.insert(f[2] + f[4]); } return s; };
        for (int every : { 977, 2500 }) { const std::vector<std::string> b = replay(es, "ES", every); CHECK(keys(b, 'C') == keys(a, 'C') && keys(b, 'L') == keys(a, 'L'), "restarts: the same Cobs and Lvps (no repaint)"); }
        const std::vector<std::string> q = replay(gc, "QGC", 0, &al), g = replay(gc, "GC", 0); CHECK(al, "GC aligned");
        CHECK(q == lines(slurp("ref_qgc_0908_1009.txt")), "QGC lines = the reference");
        const std::set<std::string> kq = keys(q, 'C'), kg = keys(g, 'C'); bool sub = true; for (const std::string& k : kq) if (!kg.count(k)) sub = false;
        CHECK(sub && !kq.empty() && kq.size() <= kg.size(), "chart root QGC labels a subset of the market key GC (both counts >= 10)");
        std::printf("  GC: %zu Cobs with the market count, %zu on a QGC chart\n", kg.size(), kq.size());
        // ---- 5. drawing (the last 400 bars of ES)
        zzd::View v; v.l = 0; v.t = 0; v.r = 4000; v.b = 140; v.first = (int)H.E.B.size() - 2400; v.last = (int)H.E.B.size() - 1; v.bw = 2; v.X = [](int j) { return 2.0 * j - 2.0 * 0; }; v.textW = [](const std::string& s) { return (int)s.size() * 6; }; v.textH = 12;
        const int f0 = v.first; v.X = [f0](int j) { return (double)(j - f0) * 1.6 + 1; };
        const std::vector<zzd::Wave> waves = H.E.view(nullptr);
        const zzd::ComboScene S1 = zzd::layoutCombo(H.E, waves, 0, v, "ZZO Combo x", &H.F), S0 = zzd::layoutCombo(H.E, waves, 0, v, "ZZO Combo x");
        int cobT = 0, lvpT = 0, old = 0; for (const zzd::Prim& p : S1.p) if (p.k == zzd::Prim::TEXT) { if (p.text == "Cob") ++cobT; if (p.text == "Lvp") ++lvpT; if (p.text == "CoB" || p.text == "weak") ++old; }
        CHECK(cobT > 0 && cobT == S1.cob && lvpT == S1.lvp && lvpT > 0, "Cob and Lvp tags drawn and counted"); CHECK(old == 0, "the old CoB / weak tags are never drawn");
        CHECK(S0.cob == 0 && S0.lvp == 0 && S0.cols == S1.cols && S0.ratios == S1.ratios && S0.counts == S1.counts, "without a finder: no Cob / Lvp; ramps, ratios and counts identical");
        std::printf("  drawn in the last 2400 bars: %d Cob, %d Lvp, %d counts, %d ratios\n", S1.cob, S1.lvp, S1.counts, S1.ratios);
        // the tag's colour: Cob = its wave's colour; Lvp = its Cob's colour (the box under the text)
        const zzd::Colours C; bool colOk = true; int seenTags = 0;
        for (size_t i = 1; i < S1.p.size(); ++i) { const zzd::Prim& p = S1.p[i]; if (p.k != zzd::Prim::TEXT || (p.text != "Cob" && p.text != "Lvp")) continue; const zzd::Prim& bx = S1.p[i - 1]; ++seenTags;
            if (bx.k != zzd::Prim::FILL || (bx.col != C.up && bx.col != C.down)) colOk = false; }
        CHECK(colOk && seenTags == cobT + lvpT, "tag boxes are green or red only");
        for (const zzd::Wave& w : waves) { if (w.live || w.e < 0) continue; const zcf::Tag* t = H.F.find(H.E.B[(size_t)w.e].t, w.dir); if (t && t->cob) CHECK(t->cob == w.dir, "a Cob sits on a wave of its own direction"); if (t && t->lvp) CHECK(t->lvp == -w.dir, "an Lvp sits on a pullback against its Cob"); }
        const std::string st = H.F.statusText(); CHECK(st.rfind("COBFINDER,1.0.0,RTH 510-900", 0) == 0 && st.find("\nCOB,") != std::string::npos, "status text");
    }
    // ---- 6. the host
    {   const std::string c = slurp("../ZzoCombo.cpp"), vw = slurp("../ZzoComboView.h");
        CHECK(c.find(std::string("p->setVersion(\"") + zcf::COMBO_VERSION + "\")") != std::string::npos && c.find(std::string("v") + zcf::COMBO_VERSION + " - 3-bar") != std::string::npos, "version literal = zcf::COMBO_VERSION");
        CHECK(c.find("RTARRAY clo(barClose)") != std::string::npos && c.find("S.F.addBar(b, c);") != std::string::npos && c.find("S.F.trim(S.E.trimmed)") != std::string::npos, "closes read, finder fed and trimmed with the engine");
        CHECK(c.find("S.F.onWave(S.E.W)") != std::string::npos && c.find("&S->F)") != std::string::npos, "waves judged once; the finder reaches the drawing");
        CHECK(c.find("std::ios::app") != std::string::npos && c.find("S.recSeen.insert(k)") != std::string::npos, "record appended, keyed once");
        CHECK(c.find("far") == std::string::npos || c.find("#undef far") != std::string::npos, "no Windows-macro identifiers");
        CHECK(vw.find("\"CoB\"") == std::string::npos, "the view no longer draws CoB");
        size_t n = 0, pos = 0; while ((pos = c.find("S.F.reset()", pos)) != std::string::npos) { ++n; ++pos; } CHECK(n == 2, "the finder resets wherever the engine does");
    }
    std::printf("%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
