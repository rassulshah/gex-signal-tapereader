// test_bo1130_pdoc.cpp -- lsBreakout 1.13.0 Pdoc (Rassul 2026-11-09 14:09-14:12 CT: "pdoc where d represent the developing poc"): a Pb / Rly
// pullback whose pivot bar straddles the pullback's DEVELOPING volume POC (lsWavePOC's wave PB POC: >= 38.2% retrace, its value at the decision
// bar) and NOT the wave POC is drawn "Pdoc"; both -> Pboc. Built on test_bo1120_pboc.cpp (real ES 3m bars + VAP, bar-by-bar replay):
//  [1] ES 11-09: 07:15 / 23:24 / 00:33 Pboc, 07:45 / 20:03 Pb, 00:45 Pdoc (PB POC 7429.75; the wave POC 7426.75 not tested) - checked by hand in python
//  [2] ES 10-21: BO Pdoc 01:06 / 02:12 / 14:30; Rly Pdoc 22:12 / 00:12 / 12:03; Rly Pboc 08:15 / 03:27 unchanged; ES 11-04: Rly 08:30 Pdoc (wave POC above
//      50%: no Pboc), 08:36 no label; BO Pdoc 01:21
//  [3] every drawn Pdoc: the wave POC NOT straddled, the PB POC straddled; every Pboc: the wave POC straddled (Pboc wins)
//  [4] no repaint: no confirmed label ever changes; the pullback's VAP missing -> nothing drawn, then Pdoc once (never Pb -> Pdoc)
//  [5] records: BPB 18 fields (pdoc, pb_poc), BPD key; Lvp still hides a Pdoc
//  [6] PARITY: every pullback pivot judged, lsBreakout's developing PB POC == lsWavePOC's own wpc::pullbackOf POC at the same decision bar
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bo1130_pdoc.cpp -o t1130 && ./t1130
#include "BreakoutLogic.h"
#include <fstream>
#include <cstdio>
#include <cstring>
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
struct Run { bko::Engine E; std::map<long long, std::string> first; int switches = 0, provSwitch = 0; };
// replay: vapFrom = the first bar index whose VAP is "loaded" (bars before it have none until step vapAtStep)
static void replay(Run& R, const Fix& X, int from, int vapAtStep = -1, long long noVapFrom = 0, long long noVapTo = 0) {
    std::map<long long, std::string> lastFinal;
    for (int k = from; k <= (int)X.B.size(); ++k) {
        std::vector<bko::In> B(X.B.begin(), X.B.begin() + k); if (B.size() < 3) continue;
        const bool hide = vapAtStep >= 0 && k < vapAtStep;
        R.E.bocLive = true;
        R.E.vapAt = [&X, &R, hide, noVapFrom, noVapTo](int j, std::vector<wpc::VRow>& out) { out.clear(); const long long t = R.E.bars[(size_t)j].t;
            if (hide && t >= noVapFrom && t <= noVapTo) return 0;
            const auto it = X.V.find(t); if (it == X.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; };
        R.E.update(B, (long)B.size(), X.B[0].t, 0.25, 180, 510, 900); R.E.vapAt = nullptr;
        const bko::Scene S = draw(R.E);
        for (const bko::Txt& t : S.txt) if (t.kind == 2) { const long long bt = R.E.bars[(size_t)t.bar].t; const bool fin = R.E.pb.count(bt) || R.E.rpb.count(bt);
            if (!fin) continue;
            auto f = R.first.find(bt); if (f == R.first.end()) R.first[bt] = t.s; else if (f->second != t.s) ++R.switches; }
    }
}
static int barOf(const bko::Engine& E, long long t) { for (int k = 0; k < (int)E.bars.size(); ++k) if (E.bars[(size_t)k].t == t) return k; return -1; }
static bool straddle(int dir, const bko::In& b, double px) { long long l = 0, h = 0, p = 0; wpc::toTicks(b.l, 0.25, l); wpc::toTicks(b.h, 0.25, h); wpc::toTicks(px, 0.25, p); return bko::pbocHit(dir, l, h, p); }
static void invariants(const Run& Q, const char* nm) {
    int nd = 0;
    for (const auto& kv : Q.E.boc) { const int bi = barOf(Q.E, kv.first); if (bi < 0) continue; const auto m = Q.E.pb.find(kv.first); if (m == Q.E.pb.end()) continue; const int dir = m->second.dir;
        const auto wp = Q.E.bocPoc.find(kv.first);
        if (kv.second == 2) { ++nd; CHECK(Q.E.dpoc.count(kv.first) && straddle(dir, Q.E.bars[(size_t)bi], Q.E.dpoc.at(kv.first)), std::string(nm) + " Pdoc " + hmOf(kv.first) + " straddles its PB POC");
            CHECK(wp == Q.E.bocPoc.end() || !straddle(dir, Q.E.bars[(size_t)bi], wp->second), std::string(nm) + " Pdoc " + hmOf(kv.first) + " not on the wave POC"); }
        if (kv.second == 1) CHECK(wp != Q.E.bocPoc.end() && straddle(dir, Q.E.bars[(size_t)bi], wp->second), std::string(nm) + " Pboc " + hmOf(kv.first) + " on the wave POC"); }
    for (const auto& kv : Q.E.rpbBoc) if (kv.second == 2) { ++nd; const int bi = barOf(Q.E, kv.first); const int dir = Q.E.rpb.at(kv.first).dir;
        CHECK(bi >= 0 && Q.E.dpoc.count(kv.first) && straddle(dir, Q.E.bars[(size_t)bi], Q.E.dpoc.at(kv.first)), std::string(nm) + " Rly Pdoc " + hmOf(kv.first) + " straddles its PB POC"); }
    CHECK(Q.switches == 0, std::string(nm) + " no confirmed label ever changed (" + std::to_string(Q.switches) + ")");
    std::printf("  %s: Pdoc %d (BO/BD + Rly)\n", nm, nd);
}
// [6] the developing POC as lsWavePOC computes it (wpc::pullbackOf, no Rev list: lsBreakout ends the profile at BO / BD, Rly, the wave end, 16:00)
static int parity(const bko::Engine& E, const char* nm) {
    std::vector<wpc::Bar> WB; for (const bko::In& b : E.bars) { wpc::Bar x; x.t = b.t; wpc::toTicks(b.o, 0.25, x.o); wpc::toTicks(b.h, 0.25, x.h); wpc::toTicks(b.l, 0.25, x.l); wpc::toTicks(b.c, 0.25, x.c); WB.push_back(x); }
    std::vector<wpc::BoCand> bos; for (const auto& kv : E.bo) if (E.shownBo(kv.second)) { wpc::BoCand c; c.t0 = kv.second.t0; c.tf = kv.second.t; c.dir = kv.second.dir; bos.push_back(c); }
    std::vector<wpc::RlyCand> rl; for (const auto& kv : E.rly) if (kv.second.fired) { wpc::RlyCand r; r.te = kv.second.te; r.tfire = kv.second.tc; r.dir = kv.second.dir; rl.push_back(r); }
    const std::vector<wpc::RevCand> none;
    int checked = 0, same = 0;
    auto one = [&](const bko::PbMark& m) {
        const int i0 = E.idxT(m.tw0), i1 = E.idxT(m.tw1), ip = E.idxT(m.tp), idec = E.idxT(m.td); if (i0 < 0 || i1 < 0 || ip < 0 || idec < 0) return;
        long long mine = 0; const int r = E.pbPocTk(i0, i1, ip, idec, m.dir > 0 ? 1 : -1, mine); if (r != 1) return;
        wpc::Wave w; w.dir = m.dir > 0 ? 1 : -1; w.i0 = i0; w.i1 = i1; w.t0 = m.tw0; w.t1 = m.tw1; w.p0 = m.dir > 0 ? WB[(size_t)i0].l : WB[(size_t)i0].h; w.p1 = m.dir > 0 ? WB[(size_t)i1].h : WB[(size_t)i1].l; w.qWhy = wpc::Q_RLY | wpc::Q_BOW;
        long long lo = LLONG_MAX, hi = LLONG_MIN; for (int k = i0; k <= i1; ++k) { lo = std::min(lo, WB[(size_t)k].l); hi = std::max(hi, WB[(size_t)k].h); } w.rowTicks = wpc::rowTicksFor(hi - lo + 1);
        const wpc::PbProf P = wpc::pullbackOf(w, WB, idec, bos, rl, none, E.vapAt);
        ++checked; const long long theirs = P.poc >= 0 ? P.e0 + (long long)P.poc * P.rowTicks + P.rowTicks / 2 : LLONG_MIN;
        if (theirs == mine) ++same; else std::printf("  PARITY DIFF %s %s: lsBreakout %.2f, lsWavePOC %.2f (state %d %s)\n", nm, hmOf(m.tp).c_str(), mine * 0.25, theirs * 0.25, P.state, P.why); };
    for (const auto& kv : E.pb) one(kv.second);
    for (const auto& kv : E.rpb) one(kv.second);
    std::printf("  %s parity: %d of %d pullback pivots with a PB POC agree with lsWavePOC\n", nm, same, checked);
    CHECK(checked > 0 && same == checked, std::string(nm) + " PB POC parity with lsWavePOC (" + std::to_string(same) + "/" + std::to_string(checked) + ")");
    return checked;
}
static Fix FX;   // the fixture the parity run reads VAP from
static void withVap(bko::Engine& E) { E.vapAt = [&E](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = FX.V.find(E.bars[(size_t)j].t); if (it == FX.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; }; }
int main() {
    const Fix A = load("pboc_es1109.txt", 0.25); CHECK(A.B.size() > 300, "ES 11-09 bars");
    Run R; replay(R, A, 3);
    const long d9 = A.B.back().days, d8 = d9 - 1;
    auto lab = [&](const Run& Q, long long t) { const auto f = Q.first.find(t); return f == Q.first.end() ? std::string("-") : f->second; };
    std::printf("ES 11-09:\n"); for (const auto& kv : R.first) std::printf("  %s %s\n", hmOf(kv.first).c_str(), kv.second.c_str());
    CHECK(lab(R, T("07:15", d9)) == "Pboc", "ES 07:15 Pboc (got " + lab(R, T("07:15", d9)) + ")");
    CHECK(lab(R, T("07:45", d9)) == "Pb", "ES 07:45 Pb (got " + lab(R, T("07:45", d9)) + ")");
    CHECK(lab(R, T("23:24", d8)) == "Pboc", "ES 23:24 Pboc");
    CHECK(lab(R, T("00:33", d9)) == "Pboc", "ES 00:33 Pboc");
    CHECK(lab(R, T("20:03", d8)) == "Pb", "ES 20:03 Pb");
    CHECK(lab(R, T("00:45", d9)) == "Pdoc", "ES 00:45 Pdoc (got " + lab(R, T("00:45", d9)) + ")");
    CHECK(R.E.dpoc.count(T("00:45", d9)) && std::fabs(R.E.dpoc.at(T("00:45", d9)) - 7429.75) < 1e-9, "00:45 PB POC 7429.75 (python check)");
    CHECK(std::fabs(R.E.bocPoc[T("07:15", d9)] - 7453.75) < 1e-9, "07:15 wave POC 7453.75 unchanged");
    invariants(R, "ES 11-09");
    {   const auto L = R.E.pbL.find(T("00:45", d9)); CHECK(L != R.E.pbL.end(), "BPB line 00:45");
        if (L != R.E.pbL.end()) { int nf = 1; for (char c : L->second.line) nf += c == '|'; CHECK(nf == 18, "BPB 18 fields"); CHECK(L->second.line.find("|0|7426.75|1|7429.75") != std::string::npos, "BPB pdoc 1 + PB POC: " + L->second.line);
            CHECK(!bko::recKey(L->second.line).empty(), "BPB 18-field key"); }
        const auto M = R.E.pbL.find(T("07:15", d9)); CHECK(M != R.E.pbL.end() && M->second.line.find("|1|7453.75|0|") != std::string::npos, "BPB 07:15 Pboc pdoc 0"); }
    // [4] the pullback's VAP (00:15-00:51) missing until 01:03 closes: 00:45 nothing, then Pdoc once
    {   Run Q; int stepAt = 0; for (int k = 0; k < (int)A.B.size(); ++k) if (A.B[(size_t)k].t == T("01:03", d9)) stepAt = k + 2;
        replay(Q, A, 3, stepAt, T("00:15", d9), T("00:51", d9));
        CHECK(lab(Q, T("00:45", d9)) == "Pdoc", "late pullback VAP: 00:45 drawn once as Pdoc (got " + lab(Q, T("00:45", d9)) + ")");
        CHECK(Q.switches == 0, "late pullback VAP: no Pb -> Pdoc switch");
        const auto L = Q.E.pbL.find(T("00:45", d9)); CHECK(L != Q.E.pbL.end() && L->second.line.find("|1|7429.75") != std::string::npos, "late VAP: BPB written once with pdoc 1"); }
    {   bko::Engine E = R.E; E.hidePb.insert(std::make_pair(T("00:45", d9), -1)); const bko::Scene S = draw(E); bool has = false;
        for (const bko::Txt& t : S.txt) if (t.kind == 2 && E.bars[(size_t)t.bar].t == T("00:45", d9)) has = true;
        CHECK(!has && S.pbHidden > 0, "Lvp hides Pdoc"); }
    {   const bko::Scene S = draw(R.E); const std::string st = bko::statusText(R.E, "ES", "ESZ6", 180, S); CHECK(st.find("PDOC,(1.13.0)") != std::string::npos, "status PDOC line"); }
    {   const Fix X = load("pboc_es1021.txt", 0.25); Run Q; replay(Q, X, 3); const long d = X.B.back().days, dp = d - 1;
        std::printf("ES 10-21:\n"); for (const auto& kv : Q.first) std::printf("  %s %s\n", hmOf(kv.first).c_str(), kv.second.c_str());
        CHECK(lab(Q, T("08:15", d)) == "Pboc", "10-21 08:15 Rly Pboc (unchanged)");
        CHECK(lab(Q, T("03:27", d)) == "Pboc", "10-21 03:27 Rly Pboc (unchanged)");
        CHECK(lab(Q, T("01:06", d)) == "Pdoc" && lab(Q, T("02:12", d)) == "Pdoc" && lab(Q, T("14:30", d)) == "Pdoc", "10-21 BD Pdoc 01:06 / 02:12 / 14:30");
        CHECK(lab(Q, T("22:12", dp)) == "Pdoc" && lab(Q, T("00:12", d)) == "Pdoc" && lab(Q, T("12:03", d)) == "Pdoc", "10-21 Rly Pdoc 22:12 / 00:12 / 12:03");
        int bpd = 0; for (const auto& kv : Q.E.rpbL) if (kv.second.line.compare(0, 4, "BPD|") == 0) { ++bpd; CHECK(!bko::recKey(kv.second.line).empty(), "BPD key " + kv.second.line); }
        CHECK(bpd == 3, "10-21 3 BPD lines");
        invariants(Q, "ES 10-21"); }
    {   const Fix X = load("pboc_es1104.txt", 0.25); Run Q; replay(Q, X, 3); const long d = X.B.back().days, dp = d - 1;
        std::printf("ES 11-04:\n"); for (const auto& kv : Q.first) std::printf("  %s %s\n", hmOf(kv.first).c_str(), kv.second.c_str());
        CHECK(lab(Q, T("08:30", d)) == "Pdoc", "11-04 08:30 Rly Pdoc (wave POC above 50%, PB POC tested)");
        CHECK(lab(Q, T("08:36", d)) == "-", "11-04 08:36 no label");
        CHECK(lab(Q, T("01:21", d)) == "Pdoc", "11-04 01:21 BO Pdoc");
        CHECK(lab(Q, T("19:27", dp)) == "Pboc" && lab(Q, T("13:15", d)) == "Pboc", "11-04 Pboc 19:27 / 13:15 unchanged");
        invariants(Q, "ES 11-04"); }
    {   const char* fs[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
        for (const char* f : fs) { FX = load(f, 0.25); Run Q; replay(Q, FX, 3); withVap(Q.E); parity(Q.E, f); Q.E.vapAt = nullptr; } }
    std::printf("test_bo1130_pdoc: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
