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
        E.addBar(b); F.addBar(b, r.c, r.o);
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

static int QB(int q) { return q >= 4 ? 2 : q == 3 ? 1 : 0; }
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
        CHECK(R.F.out.size() == 1 && R.F.out[0].rfind("C|1.5.0|", 0) == 0 && R.F.out[0].find("|UP|4915|4960|4945|ONL/LonLO/PhrL|11|0.33|at|bo|12|11|-") != std::string::npos, "the record line");
        CHECK(R.F.act.size() == 1, "open for its Lvp");
    }
    auto none = [&](const char* what, std::function<void(Rig&)> f, int wantWatch) { Rig R; base(R); f(R); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        const bool ok = wantWatch ? (t && !t->cob && t->watch == wantWatch && R.F.out.size() == 1 && R.F.out[0][0] == 'W' && R.F.act.empty()) : (!t && R.F.out.empty());
        if (ok) ++pass; else { ++fail; std::printf("FAIL no Cob: %s\n", what); } };
    none("pane count 9", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(9, 30); }, 2);
    none("market count 9", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(30, 9); }, 2);
    none("count 4", [](Rig& R) { R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(4, 4); }, 0);
    {   Rig R; base(R); R.bar(T(1, 8, 45), 4959, 4937, 4938); R.judge(20, 20); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);   // (1.2.0) no retracement limit
        CHECK(t && t->cob == 1 && t->retr > zcf::MAX_RETR, "a Cob that gave back 0.49 is still a Cob (1.2.0)"); const zcf::Read rd = R.F.read();
        CHECK(rd.any && rd.kind == 1 && rd.hold == zcf::HOLD_DEEP[QB(rd.q) > 0 ? 1 : 0][0], "a deep Cob shows the deep Hold rate"); }
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
    // ---- 2b. (1.1.0) the prior full session's POC / VAH / VAL
    {   std::map<long long, double> r; for (long long p = 100; p <= 110; ++p) r[p] = 10; r[104] = 50; r[105] = 40; r[103] = 30;
        const zcf::VA a = zcf::profileOf(r); CHECK(a.ok && a.poc == 104, "POC = the biggest row");
        CHECK(a.val <= 103 && a.vah >= 105 && a.vah <= 110 && a.val >= 100, "the value area surrounds the POC");
        double in = 0, tot = 0; for (auto& kv : r) { tot += kv.second; if (kv.first >= a.val && kv.first <= a.vah) in += kv.second; } CHECK(in >= 0.70 * tot && in < 0.95 * tot, "value area >= 70% and not everything");
        std::map<long long, double> f; for (long long p = 0; p <= 10; ++p) f[p] = 5; const zcf::VA b = zcf::profileOf(f); CHECK(b.ok && b.poc == 5, "a flat profile: the tie goes to the middle row");
        std::map<long long, double> e; CHECK(!zcf::profileOf(e).ok, "an empty profile has no levels");
        std::map<long long, double> two; two[1] = 5; two[9] = 5; CHECK(zcf::profileOf(two).poc == 1, "an exact tie at the same distance: the lower row");
        Rig R; R.F.setMarket("ES"); zzd::Bar x; x.t = T(0, 12, 0); x.h = 5000; x.l = 4990; x.vol = 1100; x.vap = true; R.F.addBar(x, 4995); x.t = T(0, 12, 3); x.h = 4995; x.l = 4995; x.vol = 900; R.F.addBar(x, 4995);
        CHECK(!R.F.prevVA.ok, "no prior profile inside the first session"); x.t = T(0, 17, 30); x.h = 4996; x.l = 4994; x.vol = 10; R.F.addBar(x, 4995);
        CHECK(R.F.prevVA.ok && R.F.prevVA.poc == 4995 && R.F.prevVA.vah >= 4995 && R.F.prevVA.val <= 4995 && R.F.prof.size() == 3, "the bar-spread profile of the prior session; a new one starts"); }
    {   Rig R; base(R); R.F.on.l = R.F.as.l = R.F.lon.l = R.F.phr.l = R.F.prevFull.l = R.F.prevRth.l = 4900; R.F.prevVA.ok = true; R.F.prevVA.poc = 4922; R.F.prevVA.vah = 4990; R.F.prevVA.val = 4800;
        R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        CHECK(t && t->cob == 1 && zcf::lvText(t->lv, 1) == "DPoc", "a low that dips under the prior day's POC and closes back above = a Cob on DPoc alone"); }
    {   Rig R; base(R); R.F.on.l = R.F.as.l = R.F.lon.l = R.F.phr.l = R.F.prevFull.l = R.F.prevRth.l = 4900; R.F.prevVA.ok = true; R.F.prevVA.poc = 4800; R.F.prevVA.vah = 4930; R.F.prevVA.val = 4920;
        R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1);
        CHECK(t && t->cob == 1 && zcf::lvText(t->lv, 1) == "VAH/VAL", "VAH and VAL count from below too"); }
    {   Rig R; base(R); R.F.on.l = R.F.as.l = R.F.lon.l = R.F.phr.l = R.F.prevFull.l = R.F.prevRth.l = 4900; R.F.prevVA.ok = false; R.F.prevVA.poc = 4922;
        R.bar(T(1, 8, 45), 4959, 4944, 4945); R.judge(20, 20); const zcf::Tag* t = R.F.find(T(1, 8, 42), 1); CHECK(t && !t->cob && t->watch == 1, "no profile: its numbers are never used"); }
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
    // ---- 3b. (1.2.0) Cob2, the hourly turn, the read box
    {   // 25 flat RTH bars (range 4), the RTH low 4980 at 09:48, a close back above that bar's high on the next bar, a wave to 5012
        auto q = [](Rig& R, long long c1, long long cEnd, int minShift) {
            R.F.setMarket("ES"); R.bar(T(0, 12, 0), 5300, 5200, 5250); R.bar(T(1, 8, 0), 5004, 4970, 5000);   // no level between the low and the wave
            for (int i = 0; i < 25; ++i) R.bar(T(1, 8, 33 + minShift) + i * 180, 5002, 4998, 5000);      // idx 2..26
            const long long t0 = T(1, 8, 33 + minShift) + 25 * 180;
            R.bar(t0, 4990, 4980, 4984);            // 27 the RTH low
            R.bar(t0 + 180, 4996, 4984, c1);        // 28
            R.bar(t0 + 360, 5012, 4994, 5010);      // 29 the top
            R.bar(t0 + 540, 5011, 5004, cEnd);      // 30 completes
            zzd::Wave d; d.s = 20; d.e = 27; d.dir = -1; d.ext = 4980; d.vol = 900; zzd::Wave u; u.s = 28; u.e = 29; u.dir = 1; u.ext = 5012; u.vol = 900;
            zzd::Wave p; p.s = -1; p.e = -1; p.dir = 1; p.ext = 5002; R.W = { p, d, u }; return t0 + 360; };
        {   Rig R; const long long tw = q(R, 4992, 5010, 0); R.judge(2, 2); const zcf::Tag* t = R.F.find(tw, 1);
            CHECK(t && t->cob2 == 1 && !t->cob && t->lv == 0 && R.F.out.size() == 1 && R.F.out[0].rfind("Q|1.5.0|", 0) == 0, "Cob2: a quiet turn with count 2 and no level");
            CHECK(R.F.act.size() == 1, "a Cob2 is open for its Lvp"); const zcf::Read rd = R.F.read();
            CHECK(rd.any && rd.kind == 2 && rd.dir == 1 && rd.lvpSt == 0 && rd.hold == zcf::HOLD_COB2[QB(rd.q)][0], "the read: Cob2, Lvp still possible");
            const auto rows = zzd::readRows(rd); CHECK(rows.size() == 5 && rows[0].first == "Cob2" && rows[0].second.rfind("Up ", 0) == 0 && rows[1].second == "-" && rows[3].second.rfind("wait / ", 0) == 0, "Cob2 rows: no SR+ without a swept level"); }
        {   Rig R; const long long tw = q(R, 4990, 5010, 0); R.F.F[29].c = 4990; R.judge(2, 2); CHECK(!R.F.find(tw, 1), "no Cob2: no close above the extreme bar's high within 2 bars"); }
        {   Rig R; const long long tw = q(R, 4992, 5002, 0); R.judge(2, 2); CHECK(!R.F.find(tw, 1), "no Cob2: the completing close is not above the 10 bars before the extreme"); }
        {   Rig R; const long long tw = q(R, 4992, 5010, -60); R.judge(2, 2); CHECK(!R.F.find(tw, 1), "no Cob2: under 60 minutes after the open"); }
        {   Rig R; const long long tw = q(R, 4992, 5010, 0); R.W[1].ext = 4981; R.judge(2, 2); CHECK(!R.F.find(tw, 1), "no Cob2: the RTH low is not that down-wave's pivot"); }
        {   Rig R; const long long tw = q(R, 4992, 5010, 0); for (int i = 7; i < 27; ++i) { R.F.F[(size_t)i].h = 5010; R.F.F[(size_t)i].l = 4990; } R.judge(2, 2); CHECK(!R.F.find(tw, 1), "no Cob2: under 5 bar ranges from the low"); }
        {   Rig R; const long long tw = q(R, 4992, 5010, 0); R.F.prevRth.any = true; R.F.prevRth.l = 4985; R.F.prevRth.h = 5300; R.judge(12, 12); const zcf::Tag* t = R.F.find(tw, 1);
            CHECK(t && t->cob == 1 && t->cob2 == 0 && R.F.out.size() == 1 && R.F.out[0][0] == 'C', "a Cob is never also a Cob2"); }
    }
    {   Rig R; base(R); CHECK(R.F.hourTurn(T(1, 8, 36), 1, 4966) && !R.F.hourTurn(T(1, 8, 36), 1, 4965), "Hr: a close above the high of the clock hour before the origin's hour");
        CHECK(R.F.hourTurn(T(1, 8, 36), -1, 4934) && !R.F.hourTurn(T(1, 8, 36), -1, 4935) && !R.F.hourTurn(T(5, 8, 36), 1, 9999), "Hr: bearish mirror; an unknown hour is no turn"); }
    {   Rig R; cob(R); zcf::Read rd = R.F.read(); CHECK(rd.any && rd.kind == 1 && rd.dir == 1 && rd.cnt == 12 && rd.lvpSt == 0 && !rd.hr && rd.q >= 0 && rd.hold == zcf::HOLD_COB[QB(rd.q)][0][0], "the read after a Cob: Lvp still possible");
        auto rows = zzd::readRows(rd); CHECK(rows[0].first == "Cob" && rows[0].second == "Up 8:45 AM" && rows[1].second == "SR+ ONL LonLO PhrL" && rows[2].first == "Q" && rows[2].second == std::to_string(rd.q) + "/6" && rows[3].second == "wait / no" && rows[4].second == std::to_string(rd.hold) + "%", "the box rows");
        wave(R, -1, 4935, 100, T(1, 8, 48)); rd = R.F.read(); CHECK(rd.lvpSt == 1 && rd.hold == zcf::HOLD_COB[QB(rd.q)][0][1] && zzd::readRows(rd)[3].second == "yes / no", "the read after the Lvp");
        R.bar(T(1, 17, 30), 4950, 4940, 4945); CHECK(!R.F.read().any && zzd::readRows(R.F.read())[0].second == "-", "a new session: the box is empty"); }
    {   Rig R; cob(R); wave(R, -1, 4914, 100, T(1, 8, 48)); const zcf::Read rd = R.F.read(); CHECK(rd.any && rd.lvpSt == 2 && rd.hold == zcf::HOLD_COB[QB(rd.q)][0][2] && zzd::readRows(rd)[3].second == "no / no", "no Lvp: the lower Hold rate"); }
    CHECK(zzd::clock12(T(0, 12, 0)) == "12:00 PM" && zzd::clock12(T(0, 0, 3)) == "12:03 AM" && zzd::clock12(T(0, 14, 6)) == "2:06 PM", "12-hour times");
    CHECK(zzd::lvShort(0x1FF, -1) == "SR+ ONH AsiaHI LonHI" && zzd::lvShort(0, 1) == "-", "Lvl: SR+ and at most 3 levels");
    // ---- 3c. (1.3.0) the quality score
    {   zcf::Finder F; F.setMarket("ES"); long long t = T(1, 9, 0); long long px = 5000;
        auto add = [&](long long o, long long h, long long l, long long c) { zzd::Bar b; b.t = t; b.h = h; b.l = l; b.vol = 100; b.vap = true; F.addBar(b, c, o); t += 180; };
        for (int i = 0; i < 40; ++i) add(px, px + 2, px - 2, px);                          // 0..39: flat, range 4 -> ATR about 4
        add(px, px + 1, px - 10, px - 8);                                                  // 40: the origin (low 4990)
        CHECK(F.F[39].atr > 3.9 && F.F[39].atr < 4.1, "Wilder ATR(14) of flat 4-tick bars is 4");
        long long p = px - 8; for (int i = 0; i < 15; ++i) { add(p, p + 6, p - 1, p + 5); p += 5; }   // 41..55: 15 up bars, range 7, closing near the high
        CHECK(F.quality(1, 40, 55) == 6, "15 up bars closing high, each above the last, no dip, efficient: Q 6");
        CHECK(F.quality(1, 40, 44) == 4, "4 bars: high closes, first 3, efficiency and the run score; 3 closes above the prior high and a 7 / 23 dip do not (size and length are not scored)");
        CHECK(F.quality(1, 40, 40) == -1 && F.quality(1, 40, 99999) == -1 && F.quality(1, 55, 40) == -1, "nothing to score");
        zcf::Finder G; G.setMarket("ES"); t = T(1, 9, 0);
        auto addG = [&](long long h, long long l, long long c) { zzd::Bar b; b.t = t; b.h = h; b.l = l; b.vol = 100; b.vap = true; G.addBar(b, c); t += 180; };
        addG(5002, 4998, 5000); addG(5004, 4999, 5003); CHECK(G.F[0].o == 5000 && G.F[1].o == 5000, "no open given: the previous close");
        // bearish mirror: the same bars upside down score the same
        zcf::Finder M; M.setMarket("ES"); t = T(1, 9, 0); auto addM = [&](long long o, long long h, long long l, long long c) { zzd::Bar b; b.t = t; b.h = -l; b.l = -h; b.vol = 100; b.vap = true; M.addBar(b, -c, -o); t += 180; };
        for (int i = 0; i < 40; ++i) addM(px, px + 2, px - 2, px);
        addM(px, px + 1, px - 10, px - 8); p = px - 8; for (int i = 0; i < 15; ++i) { addM(p, p + 6, p - 1, p + 5); p += 5; }
        CHECK(M.quality(-1, 40, 55) == 6 && M.quality(-1, 40, 44) == F.quality(1, 40, 44), "bearish mirror");
        // a deep dip inside the wave costs the point
        zcf::Finder D; D.setMarket("ES"); t = T(1, 9, 0); auto addD = [&](long long o, long long h, long long l, long long c) { zzd::Bar b; b.t = t; b.h = h; b.l = l; b.vol = 100; b.vap = true; D.addBar(b, c, o); t += 180; };
        for (int i = 0; i < 40; ++i) addD(px, px + 2, px - 2, px);
        addD(px, px + 1, px - 10, px - 8); p = px - 8; for (int i = 0; i < 7; ++i) { addD(p, p + 6, p - 1, p + 5); p += 5; }
        addD(p, p + 1, p - 30, p - 28); p -= 28; for (int i = 0; i < 8; ++i) { addD(p, p + 6, p - 1, p + 5); p += 5; }
        CHECK(D.quality(1, 40, 56) == 4, "a dip of 31 inside a 50-tick wave costs the shallow-dip point and the efficiency point");
    }
    // ---- 3d. (1.4.0) the Cob bar
    {   zcf::Finder F; F.setMarket("ES"); long long t = T(1, 9, 0); const long long px = 5000;
        auto add = [&](long long o, long long h, long long l, long long c, long long vol) { zzd::Bar b; b.t = t; b.h = h; b.l = l; b.vol = vol; b.vap = true; F.addBar(b, c, o); t += 180; };
        for (int i = 0; i < 40; ++i) add(px, px + 2, px - 2, px, 100);                     // ATR 4, median volume 100
        add(px, px + 1, px - 10, px - 8, 100);                                             // 40: the origin (high 5001)
        add(px - 8, px - 2, px - 9, px - 3, 100);                                          // 41: up, starts inside the pivot bar, range 7 >= 6: the Cob bar
        add(px - 3, px + 3, px - 4, px + 2, 100); add(px + 2, px + 8, px + 1, px + 7, 100);
        CHECK(F.cobBar(1, 40, 43) == 41, "the first up bar, inside the pivot bar, range 1.5 ATR");
        F.F[41].h = px - 4; F.F[41].c = px - 5; CHECK(F.cobBar(1, 40, 43) == 42, "a small first up bar: the second up bar (range 7, low under the pivot high) is the Cob bar");
        F.F[42].l = px + 1; F.F[42].h = px + 8; CHECK(F.cobBar(1, 40, 43) == -1, "the second up bar starts above the pivot bar's high: no Cob bar (only the first two up bars count)");
        F.F[41].v = 150; CHECK(F.cobBar(1, 40, 43) == 41, "heavy volume (1.5x the median) makes the small first up bar the Cob bar");
        F.F[41].v = 149; CHECK(F.cobBar(1, 40, 43) == -1, "1.49x is not heavy");
        F.F[41].c = F.F[41].o - 1; F.F[41].v = 500; F.F[42].l = px - 4; CHECK(F.cobBar(1, 40, 43) == 42, "a down bar is skipped whatever its volume");
        CHECK(F.cobBar(1, 5, 9) == -1 && F.cobBar(1, 40, 40) == -1, "too little history / no wave: none");
    }
    // ---- 3e. (1.5.0) the Cob bar's rank and the stop-run bar
    {   zcf::Finder F; F.setMarket("ES"); long long t = T(1, 9, 0);
        auto add = [&](long long o, long long h, long long l, long long c, long long vol) { zzd::Bar b; b.t = t; b.h = h; b.l = l; b.vol = vol; b.vap = true; F.addBar(b, c, o); t += 180; };
        for (int i = 0; i < 60; ++i) { add(5000, 5003, 4999, 5002, 100); add(5002, 5003, 4999, 5000, 300); }   // 60 up bars (range 4, volume 100) between down bars (volume 300)
        add(5000, 5010, 4999, 5009, 90);                                                                          // 120: an up bar of range 11
        CHECK(F.cbRankOf(1, 120) == 60, "beats all 60 up bars before it on range (down bars are not counted)");
        add(5009, 5012, 5008, 5011, 500); CHECK(F.cbRankOf(1, 121) == 61, "beats 61 on volume though only 60 on range: the larger");
        add(5011, 5014, 5010, 5013, 100); CHECK(F.cbRankOf(1, 122) == 0, "the bar before it is as big: rank 0");
        F.F[10].h = 5020; CHECK(F.cbRankOf(1, 120) == 54, "stops at the first up bar at least as big (bar 10 is the 55th up bar back)");
        // the stop run: levels ONL 4990 and LonLO 4985, a wave down from bar 123
        F.on.any = true; F.on.l = 4990; F.on.h = 5100; F.lon.any = true; F.lon.l = 4985; F.lon.h = 5100;
        add(5013, 5013, 4992, 4993, 100); add(4993, 4994, 4988, 4989, 100); add(4989, 4990, 4983, 4984, 100); add(4984, 4986, 4980, 4985, 100);   // 123..126
        int lv = -1; CHECK(F.stopRunBar(1, 123, 126, (1 << 0) | (1 << 2), &lv) == 125 && lv == 2, "the first bar through the deepest swept level (LonLO 4985), not the first through ONL");
        CHECK(F.stopRunBar(1, 123, 126, 1 << 0, &lv) == 124 && lv == 0, "ONL alone: the bar that first traded under 4990");
        CHECK(F.stopRunBar(1, 126, 126, 1 << 2, &lv) == 126 && F.stopRunBar(1, -1, 126, 1 << 2, &lv) >= 0 && F.stopRunBar(1, 123, 126, 0, &lv) == -1 && lv == -1, "the origin bar at the latest; a trimmed start is fine; no level, no stop run");
    }
    // ---- 4. real bars
    {   const std::vector<Row> es = load("bars_es_0908_1009.txt"), gc = load("bars_gc_0908_1009.txt");
        CHECK(es.size() > 6000 && gc.size() > 6000, "fixtures loaded (> 6000 bars: the engine trims)");
        bool al = false; Host H; const std::vector<std::string> a = replay(es, "ES", 0, &al, &H);
        CHECK(al, "finder and market engine aligned with the pane's engine on every bar, through the trims"); CHECK(H.E.B.size() < es.size(), "the engine did trim");
        CHECK(a == lines(slurp("ref_es_0908_1009.txt")) && !a.empty(), "ES lines = the reference");
        int c = 0, l = 0, w = 0; for (const std::string& s : a) { if (s[0] == 'C') ++c; else if (s[0] == 'L') ++l; else if (s[0] == 'W') ++w; } std::printf("  ES 09-08 .. 10-09: %d Cob, %d Lvp, %d watch (logged)\n", c, l, w);
        CHECK(c >= 5 && l >= 3 && l <= c, "Cobs and Lvps found; never more Lvps than Cobs");
        auto keys = [](const std::vector<std::string>& v, char k) { std::set<std::string> s; for (const std::string& x : v) if (x[0] == k) { std::vector<std::string> f; std::stringstream ss(x); std::string t; while (std::getline(ss, t, '|')) f.push_back(t); s.insert(f[2] + f[4]); } return s; };
        for (int every : { 977, 2500 }) { const std::vector<std::string> b = replay(es, "ES", every); CHECK(keys(b, 'C') == keys(a, 'C') && keys(b, 'L') == keys(a, 'L') && keys(b, 'Q') == keys(a, 'Q'), "restarts: the same Cobs, Cob2s and Lvps (no repaint)"); }
        { int q2 = 0; for (const std::string& x : a) if (x[0] == 'Q') ++q2; CHECK(q2 >= 2, "Cob2s found on real bars");
          bool qok = true; int ncb = 0;
          for (const std::string& x : a) if (x[0] == 'C' || x[0] == 'Q') { std::vector<std::string> f; std::stringstream ss(x); std::string t; while (std::getline(ss, t, '|')) f.push_back(t);
              if (f.size() != 21 || (f[19] != "0" && (f[20] == "-" || std::atoll(f[19].c_str()) > std::atoll(f[2].c_str()))) || (f[19] == "0" && f[20] != "-") || f[16].size() != 1 || f[16][0] < '0' || f[16][0] > '6') qok = false; else if (f[17] != "0") { ++ncb; if (std::atoll(f[17].c_str()) <= std::atoll(f[2].c_str()) - 86400 || std::atoll(f[17].c_str()) > std::atoll(f[2].c_str())) qok = false; } }
          CHECK(qok && ncb >= 3, "every C / Q line ends with the quality score, the Cob bar's time (inside the wave) or 0, its rank, and the stop-run bar's time and level (or 0 and -)"); std::printf("  ES: %d Cob2\n", q2);
          const std::set<std::string> kc = keys(a, 'C'), kq2 = keys(a, 'Q'); bool both = false; for (const std::string& k : kq2) if (kc.count(k)) both = true; CHECK(!both, "a wave is never Cob and Cob2"); }
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
        int cobT = 0, lvpT = 0, old = 0; auto inBox = [&](const zzd::Prim& p) { return S1.boxRows > 0 && p.x0 >= S1.boxL && p.x1 <= S1.boxR && p.y0 >= S1.boxT && p.y1 <= S1.boxB; };
        int cob2T = 0;
        for (const zzd::Prim& p : S1.p) if (p.k == zzd::Prim::TEXT && !inBox(p)) { if (p.text == "Cob2") ++cob2T; if (p.text == "Cob") ++cobT; if (p.text == "Lvp") ++lvpT; if (p.text == "CoB" || p.text == "weak") ++old; }
        CHECK(cobT > 0 && cobT == S1.cob && lvpT == S1.lvp && lvpT > 0, "Cob and Lvp tags drawn and counted"); CHECK(cob2T == S1.cob2, "Cob2 tags drawn and counted");
        { int cbT = 0; for (const zzd::Prim& p : S1.p) if (p.k == zzd::Prim::TEXT && !inBox(p) && (p.text == "CB" || p.text == "CB35" || p.text == "CB99")) ++cbT;
          CHECK(cbT == S1.cb && cbT >= 1 && cbT <= S1.cob + S1.cob2 && S0.cb == 0, "CB / CB35 / CB99 drawn once per Cob bar, never without a finder");
          int srT = 0; for (const zzd::Prim& p : S1.p) if (p.k == zzd::Prim::TEXT && !inBox(p) && p.text == "SR") ++srT; CHECK(srT == S1.sr && srT >= 1 && srT <= S1.cob + S1.cob2 && S0.sr == 0, "SR drawn once per stop-run bar, never without a finder"); }
        CHECK(S1.boxRows == 5 && S1.boxL >= v.l && S1.boxR < v.r && S1.boxT >= v.t && S1.boxB <= v.b && S0.boxRows == 0, "the read box: 5 rows inside the pane, none without a finder");
        { int head1 = 0, head0 = 0; for (const zzd::Prim& p : S1.p) if (p.k == zzd::Prim::TEXT && p.text == "ZZO Combo x") { ++head1; CHECK(inBox(p), "the title sits inside the box"); }
          for (const zzd::Prim& p : S0.p) if (p.k == zzd::Prim::TEXT && p.text == "ZZO Combo x") ++head0;
          CHECK(head1 == 1 && head0 == 1, "the title is drawn once"); }
        { zzd::View s2 = v; s2.b = 60; const zzd::ComboScene A = zzd::layoutCombo(H.E, waves, 0, s2, "ZZO Combo x", &H.F); CHECK(A.boxRows == 2 && A.boxB <= s2.b && A.box[0].first.rfind("Cob", 0) == 0 && A.box[1].first == "Lvl", "a short pane keeps the top rows");
          s2.b = 40; const zzd::ComboScene B = zzd::layoutCombo(H.E, waves, 0, s2, "ZZO Combo x", &H.F); CHECK(B.boxRows == 0, "under title + 2 rows: no box"); } CHECK(old == 0, "the old CoB / weak tags are never drawn");
        CHECK(S0.cob == 0 && S0.lvp == 0 && S0.cols == S1.cols && S0.ratios == S1.ratios && S0.counts == S1.counts, "without a finder: no Cob / Lvp; ramps, ratios and counts identical");
        std::printf("  drawn in the last 2400 bars: %d Cob, %d Lvp, %d counts, %d ratios\n", S1.cob, S1.lvp, S1.counts, S1.ratios);
        // the tag's colour: Cob = its wave's colour; Lvp = its Cob's colour (the box under the text)
        const zzd::Colours C; bool colOk = true; int seenTags = 0;
        for (size_t i = 1; i < S1.p.size(); ++i) { const zzd::Prim& p = S1.p[i]; if (p.k != zzd::Prim::TEXT || inBox(p) || (p.text != "Cob" && p.text != "Lvp" && p.text != "Cob2")) continue; const zzd::Prim& bx = S1.p[i - 1]; ++seenTags;
            if (bx.k != zzd::Prim::FILL || (bx.col != C.up && bx.col != C.down)) colOk = false; }
        CHECK(colOk && seenTags == cobT + lvpT + cob2T, "tag boxes are green or red only");
        for (const zzd::Wave& w : waves) { if (w.live || w.e < 0) continue; const zcf::Tag* t = H.F.find(H.E.B[(size_t)w.e].t, w.dir); if (t && t->cob) CHECK(t->cob == w.dir, "a Cob sits on a wave of its own direction"); if (t && t->lvp) CHECK(t->lvp == -w.dir, "an Lvp sits on a pullback against its Cob"); }
        const std::string st = H.F.statusText(); CHECK(st.rfind("COBFINDER,1.5.0,RTH 510-900", 0) == 0 && st.find("\nCOB,") != std::string::npos, "status text");
    }
    // ---- 6. the host
    {   const std::string c = slurp("../ZzoCombo.cpp"), vw = slurp("../ZzoComboView.h");
        CHECK(c.find(std::string("p->setVersion(\"") + zcf::COMBO_VERSION + "\")") != std::string::npos && c.find(std::string("v") + zcf::COMBO_VERSION + " - 3-bar") != std::string::npos, "version literal = zcf::COMBO_VERSION");
        CHECK(c.find("RTARRAY clo(barClose)") != std::string::npos && c.find("S.F.addBar(b, c, o);") != std::string::npos && c.find("RTARRAY opn(barOpen)") != std::string::npos && c.find("S.F.trim(S.E.trimmed)") != std::string::npos, "closes read, finder fed and trimmed with the engine");
        CHECK(c.find("S.F.onWave(S.E.W)") != std::string::npos && c.find("&S->F)") != std::string::npos, "waves judged once; the finder reaches the drawing");
        CHECK(c.find("std::ios::app") != std::string::npos && c.find("S.recSeen.insert(k)") != std::string::npos, "record appended, keyed once");
        CHECK(c.find("far") == std::string::npos || c.find("#undef far") != std::string::npos, "no Windows-macro identifiers");
        CHECK(vw.find("\"CoB\"") == std::string::npos, "the view no longer draws CoB");
        {   const std::string mk = slurp("../ZzoComboMarks.cpp");   // (2.7.0) the price-pane overlay
            CHECK(mk.find("#define ZC_MARKS 1") != std::string::npos && mk.find("#include \"ZzoCombo.cpp\"") != std::string::npos && mk.find(std::string("p->setVersion(\"") + zcf::COMBO_VERSION + "\")") != std::string::npos, "lsZzoComboMarks = the same source + ZC_MARKS, same version");
            CHECK(c.find("POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | NO_UI") != std::string::npos && c.find("S.recPath.clear();") != std::string::npos && c.find("#define ZD_NAME \"ZzoComboMarks\"") != std::string::npos, "the overlay: price pane flags, its own status file, never writes the record");
            CHECK(c.find("getVerticalDrawPosition") != std::string::npos && c.find("badge(\"SR\"") != std::string::npos, "SR and CB are placed on the candle by price"); }
        size_t n = 0, pos = 0; while ((pos = c.find("S.F.reset()", pos)) != std::string::npos) { ++n; ++pos; } CHECK(n == 2, "the finder resets wherever the engine does");
    }
    std::printf("%d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
