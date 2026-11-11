// test_bo1140_entry.cpp (fixtures + helpers from test_bo1130_pdoc.cpp) -- lsBreakout 1.13.0 Pdoc (Rassul 2026-11-09 14:09-14:12 CT: "pdoc where d represent the developing poc"): a Pb / Rly
// pullback whose pivot bar straddles the pullback's DEVELOPING volume POC (lsWavePOC's wave PB POC: >= 38.2% retrace, its value at the decision
// bar) and NOT the wave POC is drawn "Pdoc"; both -> Pboc. Built on test_bo1120_pboc.cpp (real ES 3m bars + VAP, bar-by-bar replay):
//  [1] ES 11-09: 07:15 / 23:24 / 00:33 Pboc, 07:45 / 20:03 Pb, 00:45 Pdoc (PB POC 7429.75; the wave POC 7426.75 not tested) - checked by hand in python
//  [2] (1.15.0: Pdoc only with a PB POC >= 38.2% deep) ES 10-21: BD Pdoc 02:12 / 14:30 (01:06 now Pb; Rly 22:12 / 00:12 / 12:03 no label); was: BO Pdoc 01:06 / 02:12 / 14:30; Rly Pdoc 22:12 / 00:12 / 12:03; Rly Pboc 08:15 / 03:27 unchanged; ES 11-04: Rly 08:30 Pdoc (wave POC above
//      50%: no Pboc), 08:36 no label; BO Pdoc 01:21
//  [3] every drawn Pdoc: the wave POC NOT straddled, the PB POC straddled; every Pboc: the wave POC straddled (Pboc wins)
//  [4] no repaint: no confirmed label ever changes; the pullback's VAP missing -> nothing drawn, then Pdoc once (never Pb -> Pdoc)
//  [5] records: BPB 18 fields (pdoc, pb_poc), BPD key; Lvp still hides a Pdoc
//  [6] PARITY: every pullback pivot judged, lsBreakout's developing PB POC == lsWavePOC's own wpc::pullbackOf POC at the same decision bar
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bo1130_pdoc.cpp -o t1130 && ./t1130
#include "BreakoutLogic.h"
#include "EntryFacts.h"
#include <fstream>
#include <cstdio>
#include <cstring>
static std::string baseLbl(const std::string& s) { return s == "PbT" ? "Pb" : s == "PbocT" ? "Pboc" : s; }   // (1.18.0) PbT / PbocT = the Pb / Pboc that tested a Step Trend level (test_bo1180_pbt checks the T)
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
struct Fix { std::vector<bko::In> B; std::map<long long, std::vector<wpc::VRow> > V; };
static long long tOf(const char* s, long& days, int& sec) { int y, mo, d, h, mi, se; if (std::sscanf(s, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &se) != 6) return 0;
    days = svl::daysFromCivil(y, mo, d); sec = h * 3600 + mi * 60 + se; return (long long)days * 86400 + sec + OFF; }
static Fix load(const std::string& f, double tick) {
    Fix X; std::ifstream in(f); std::string l;
    while (std::getline(in, l)) {
        if (l.size() < 3 || l[1] != '|') continue;
        char ts[32] = { 0 };
        if (l[0] == 'B') { double o, h, lo, c, v; if (std::sscanf(l.c_str() + 2, "%19[^|]|%lf|%lf|%lf|%lf|%lf", ts, &o, &h, &lo, &c, &v) != 6) continue;
            bko::In b; b.t = tOf(ts, b.days, b.endSec); b.ok = true; b.o = o; b.h = h; b.l = lo; b.c = c; b.vol = (long long)v; X.B.push_back(b); }
        else if (l[0] == 'V') { double p, v, bu, se; if (std::sscanf(l.c_str() + 2, "%19[^|]|%lf|%lf|%lf|%lf", ts, &p, &v, &bu, &se) != 5) continue;
            long d; int s; const long long t = tOf(ts, d, s); wpc::VRow r; if (!wpc::toTicks(p, tick, r.tick)) continue; r.vol = (long long)v; r.dlt = (long long)(bu - se); X.V[t].push_back(r); }
    }
    return X;
}
static long long T(const char* hm, long day) { int h, m; std::sscanf(hm, "%d:%d", &h, &m); return (long long)day * 86400 + h * 3600 + m * 60 + OFF; }
static std::string hmOf(long long t) { const long long s = (t - OFF) % 86400; char b[8]; std::snprintf(b, sizeof b, "%02lld:%02lld", s / 3600, (s / 60) % 60); return b; }
static bko::Scene draw(const bko::Engine& E) {
    bko::View v; v.first = 0; v.last = (int)E.bars.size() - 1; v.area.l = -1000000; v.area.t = -1000000; v.area.r = 1000000; v.area.b = 1000000;
    v.X = [](int i) { return 10 + i * 40; }; v.Y = [](double p) { return (int)std::lround(400000 - p * 40); }; v.textW = [](const char* s, int) { return (int)std::strlen(s) * 6; };
    return bko::layout(E, v);
}
static int barOf(const bko::Engine& E, long long t) { for (int k = 0; k < (int)E.bars.size(); ++k) if (E.bars[(size_t)k].t == t) return k; return -1; }
// test_bo1140_entry: lsBreakout 1.14.0 fact kind E (EntryFacts.h) on real ES 3-min bars + VAP (the 1130 fixtures), bar-by-bar replay.
// Rassul 16:39 CT constraints: badges only on labels the indicator DRAWS; a hidden (Lvp) label gets none; no Pdoc / ENT without a valid Pb.
//  [1] every E fact = a drawn, decided label: a PbMark in E.pb (boc known or VAP off) / E.rpb (rpbBoc 1 / 2) with the same decision bar + side,
//      code = the drawn text (Pb 1 / Pboc 2 / Pdoc 3, +10 Rly), and that label is in the Scene; every drawn decided label has its fact
//  [2] code 3 (Pdoc) -> the PbMark's boc / rpbBoc is 2 and lo == E.dpoc[tp] (the PB POC); no other code carries a PB POC
//  [3] stop = pivot bar low - 1 tick (long) / high + 1 tick (short); target = the wave extreme beyond the decision close side
//  [4] an Lvp-hidden label (E.hidePb) -> no fact; provisional Pbs never; the pullback VAP missing -> no fact until drawn, then once
//  [5] no repaint: a fact once posted keeps its code, PB POC, stop and target on every later step
static std::string txtOf(int code) { const int c = code % 10; return c == 3 ? "Pdoc" : c == 2 ? "Pboc" : "Pb"; }
struct FRun { bko::Engine E; std::map<long long, sb::Fact> first; int changed = 0, orphan = 0, noFact = 0, steps = 0; };
static void freplay(FRun& R, const Fix& X, int from, int vapAtStep = -1, long long nvF = 0, long long nvT = 0) {
    for (int k = from; k <= (int)X.B.size(); ++k) {
        std::vector<bko::In> B(X.B.begin(), X.B.begin() + k); if (B.size() < 3) continue;
        const bool hide = vapAtStep >= 0 && k < vapAtStep;
        R.E.bocLive = true;
        R.E.vapAt = [&X, &R, hide, nvF, nvT](int j, std::vector<wpc::VRow>& out) { out.clear(); const long long t = R.E.bars[(size_t)j].t;
            if (hide && t >= nvF && t <= nvT) return 0;
            const auto it = X.V.find(t); if (it == X.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; };
        R.E.update(B, (long)B.size(), X.B[0].t, 0.25, 180, 510, 900); R.E.vapAt = nullptr; ++R.steps;
        const bko::Scene S = draw(R.E);
        const std::vector<sb::Fact> F = efx::facts(R.E, nullptr);
        std::set<std::pair<long long, int> > drawn;   // (decision bar, side) of every drawn decided pullback label + its text
        std::map<std::pair<long long, int>, std::string> dtxt;
        for (const bko::Txt& t : S.txt) if (t.kind == 2) { const long long bt = R.E.bars[(size_t)t.bar].t;
            const auto a = R.E.pb.find(bt); const auto b = R.E.rpb.find(bt);
            const bko::PbMark* M = a != R.E.pb.end() ? &a->second : b != R.E.rpb.end() ? &b->second : nullptr; if (!M) continue;
            drawn.insert(std::make_pair(M->td, M->dir)); dtxt[std::make_pair(M->td, M->dir)] = baseLbl(t.s); }
        for (const sb::Fact& f : F) {
            const auto key = std::make_pair(f.bar, f.side);
            if (!drawn.count(key)) { ++R.orphan; std::printf("orphan fact td %s code %d\n", hmOf(f.bar).c_str(), f.code); }
            else if (dtxt[key] != txtOf(f.code)) { ++R.orphan; std::printf("text mismatch %s %s vs %s\n", hmOf(f.bar).c_str(), dtxt[key].c_str(), txtOf(f.code).c_str()); }
            auto fi = R.first.find(f.bar * 2 + (f.side > 0));
            if (fi == R.first.end()) R.first[f.bar * 2 + (f.side > 0)] = f;
            else if (fi->second.code != f.code || fi->second.lo != f.lo || fi->second.hi != f.hi || fi->second.val != f.val) ++R.changed; }
        std::set<std::pair<long long, int> > fk; for (const sb::Fact& f : F) fk.insert(std::make_pair(f.bar, f.side));
        for (const auto& d : drawn) if (!fk.count(d)) ++R.noFact;
    }
}
static void checks(const FRun& R, const char* tag, bool wantPdoc = true) {
    const std::string t = tag;
    CHECK(R.orphan == 0, t + ": every fact is a drawn decided label with the drawn text");
    CHECK(R.noFact == 0, t + ": every drawn decided label has its fact");
    CHECK(R.changed == 0, t + ": no repaint of a fact");
    const std::vector<sb::Fact> F = efx::facts(R.E, nullptr); int pd = 0;
    for (const sb::Fact& f : F) {
        const bko::PbMark* M = nullptr; long long tp = 0; int bc = -1;
        for (const auto& kv : R.E.pb) if (kv.second.td == f.bar && kv.second.dir == f.side) { M = &kv.second; tp = kv.first; const auto b = R.E.boc.find(tp); bc = b == R.E.boc.end() ? -1 : b->second; }
        if (f.code > 10) for (const auto& kv : R.E.rpb) if (kv.second.td == f.bar && kv.second.dir == f.side) { M = &kv.second; tp = kv.first; bc = R.E.rpbBoc.count(tp) ? R.E.rpbBoc.at(tp) : -1; }
        CHECK(M != nullptr, t + ": fact has its PbMark");
        if (!M) continue;
        if (f.code % 10 == 3) { ++pd; CHECK(bc == 2, t + ": Pdoc fact only for a valid Pb drawn Pdoc"); CHECK(R.E.dpoc.count(tp) && R.E.dpoc.at(tp) == f.lo, t + ": Pdoc fact carries the PB POC"); }
        else CHECK(f.lo == 0, t + ": no PB POC on a non-Pdoc fact");
        if (f.code > 10) CHECK(bc == 1 || bc == 2, t + ": Rly fact only for a drawn Rly Pboc / Pdoc");
        const int pk = barOf(R.E, tp); if (pk >= 0) { const bko::In& pv = R.E.bars[(size_t)pk]; CHECK(std::fabs(f.hi - (f.side > 0 ? pv.l - 0.25 : pv.h + 0.25)) < 1e-9, t + ": stop beyond the pivot bar"); }
        CHECK(f.state == 1 && f.proven == 0, t + ": decided fact");
    }
    CHECK(wantPdoc ? pd > 0 : pd == 0, t + (wantPdoc ? ": at least one Pdoc fact" : ": no Pdoc fact (1.15.0: every PB POC shallower than 38.2%)"));
}
int main() {
    const Fix A = load("pboc_es1109.txt", 0.25); CHECK(A.B.size() > 300, "ES 11-09 bars");
    FRun R; freplay(R, A, 3); checks(R, "ES 11-09", false);
    const long d9 = A.B.back().days;
    {   const std::vector<sb::Fact> F = efx::facts(R.E, nullptr); bool p0045 = false;
        for (const sb::Fact& f : F) { const auto a = R.E.pb.find(T("00:45", d9)); if (a != R.E.pb.end() && f.bar == a->second.td) { p0045 = true; CHECK(f.code % 10 != 3 && f.lo == 0, "00:45 (1.15.0) Pb fact, no PB POC: its PB POC 7429.75 is shallower than 38.2%"); } }
        CHECK(p0045, "00:45 has its (Pb) fact");
        bko::Engine E = R.E; E.hidePb.insert(std::make_pair(T("00:45", d9), -1)); const std::vector<sb::Fact> G = efx::facts(E, nullptr); bool still = false;
        for (const sb::Fact& f : G) { const auto a = E.pb.find(T("00:45", d9)); if (a != E.pb.end() && f.bar == a->second.td) still = true; }
        CHECK(!still && G.size() + 1 == F.size(), "Lvp-hidden Pdoc: no fact (the others stay)");
        bko::Engine P = R.E; P.pbProv.clear(); bko::PbMark pm; pm.tp = A.B[A.B.size() - 3].t; pm.td = A.B[A.B.size() - 2].t; pm.dir = 1; pm.tw0 = A.B[A.B.size() - 20].t; pm.tw1 = A.B[A.B.size() - 5].t; P.pbProv.push_back(pm);
        CHECK(efx::facts(P, nullptr).size() == F.size(), "provisional Pb: no fact");
        bko::Engine Q = R.E; const auto a = Q.pb.find(T("00:45", d9)); if (a != Q.pb.end()) Q.boc.erase(a->first);
        CHECK(efx::facts(Q, nullptr).size() + 1 == F.size(), "boc unknown with VAP live (not drawn): no fact"); }
    {   FRun Q; int stepAt = 0; for (int k = 0; k < (int)A.B.size(); ++k) if (A.B[(size_t)k].t == T("01:03", d9)) stepAt = k + 2;
        freplay(Q, A, 3, stepAt, T("00:15", d9), T("00:51", d9)); checks(Q, "ES 11-09 late VAP", false); }
    {   const Fix X = load("pboc_es1021.txt", 0.25); FRun Q; freplay(Q, X, 3); checks(Q, "ES 10-21");
        int rly = 0; for (const sb::Fact& f : efx::facts(Q.E, nullptr)) if (f.code > 10) ++rly; CHECK(rly > 0, "ES 10-21: Rly Pboc / Pdoc facts"); }
    {   const Fix X = load("pboc_es1104.txt", 0.25); FRun Q; freplay(Q, X, 3); checks(Q, "ES 11-04"); }
    {   std::vector<sb::Fact> v = efx::facts(R.E, [](long long t) { return t - OFF; }); const std::vector<sb::Fact> w = efx::facts(R.E, nullptr);
        bool ok = v.size() == w.size(); for (size_t i = 0; ok && i < v.size(); ++i) ok = v[i].bar == w[i].bar - OFF; CHECK(ok, "decision bar through the host clock"); }
    std::printf("test_bo1140_entry: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
