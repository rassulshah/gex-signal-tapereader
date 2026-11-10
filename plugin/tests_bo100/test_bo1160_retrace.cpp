// test_bo1160_retrace.cpp -- lsBreakout 1.16.0 (Rassul 2026-11-09 21:06-21:08 CT "see this pb, it is too low", "all pullbacks including pb pboc pdoc
// are required to be at least 38.2%"): every pullback label - Pb, Pboc, Pdoc after a BO / BD, the no-BO Rly Pboc / Pdoc, and the no-VAP fallback Pb -
// needs its pivot to retrace >= 38.2% of its wave (origin bar -> wave end bar). Real ES 3m bars + VAP (11-09 / 10-21 / 11-04), bar-by-bar replay.
//  [1] deepPb() on hand cases (up / down, exactly 38.2%, shallow, bad input)
//  [2] every DECIDED label of every type (BO Pb / Pboc / Pdoc, Rly Pboc / Pdoc) is >= 38.2% of its wave; each type occurs at least once
//  [3] the shallow pivots were seen and skipped (shallowPb > 0) and ES 11-09 20:03 (a 1.15.0 Pb) has no label
//  [4] no-VAP fallback: VAP withheld for the whole run -> the plain Pbs decided after 3 bars are all >= 38.2% too
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bo1160_retrace.cpp -o t1160 && ./t1160
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
    CHECK(same == checked, std::string(nm) + " PB POC parity with lsWavePOC (" + std::to_string(same) + "/" + std::to_string(checked) + ")");
    return checked;
}
static Fix FX;   // the fixture the parity run reads VAP from
static void withVap(bko::Engine& E) { E.vapAt = [&E](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = FX.V.find(E.bars[(size_t)j].t); if (it == FX.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; }; }
static bool deepOf(const bko::Engine& E, const bko::PbMark& m) {
    const int a0 = E.idxT(m.tw0), a1 = E.idxT(m.tw1), ip = E.idxT(m.tp); if (a0 < 0 || a1 < 0 || ip < 0) return false;
    const int dir = m.dir > 0 ? 1 : -1; const bko::In& o = E.bars[(size_t)a0]; const bko::In& z = E.bars[(size_t)a1]; const bko::In& p = E.bars[(size_t)ip];
    return bko::Engine::deepPb(dir, dir > 0 ? o.l : o.h, dir > 0 ? z.h : z.l, dir > 0 ? p.l : p.h);
}
int main() { (void)&withVap; (void)&parity; (void)&invariants;
    // [1]
    CHECK(bko::Engine::deepPb(1, 100, 110, 106.18), "up: exactly 38.2% qualifies");
    CHECK(!bko::Engine::deepPb(1, 100, 110, 106.5), "up: 35% is too shallow");
    CHECK(bko::Engine::deepPb(-1, 110, 100, 104), "down: 40% qualifies");
    CHECK(!bko::Engine::deepPb(-1, 7599.75, 7568, 7576.75), "down: the 11-09 BD pullback (27.6%) is too shallow");
    CHECK(!bko::Engine::deepPb(1, 110, 100, 105) && !bko::Engine::deepPb(1, NAN, 110, 105), "bad wave / NaN -> no label");
    // [2] + [3]
    const char* fs[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
    int nPb = 0, nPboc = 0, nPdoc = 0, nRPboc = 0, nRPdoc = 0; long long shallow = 0; bool gone2003 = false;
    for (const char* f : fs) {
        const Fix X = load(f, 0.25); Run Q; replay(Q, X, 3);
        shallow += Q.E.shallowPb + Q.E.shallowRly;
        for (const auto& kv : Q.E.pb) { const auto b = Q.E.boc.find(kv.first); const int t = b == Q.E.boc.end() ? 0 : b->second;
            CHECK(deepOf(Q.E, kv.second), std::string(f) + " " + hmOf(kv.first) + (t == 2 ? " Pdoc" : t == 1 ? " Pboc" : " Pb") + " >= 38.2%");
            if (t == 2) ++nPdoc; else if (t == 1) ++nPboc; else ++nPb; }
        for (const auto& kv : Q.E.rpb) { const auto b = Q.E.rpbBoc.find(kv.first); const int t = b == Q.E.rpbBoc.end() ? 0 : b->second; if (t == 0) continue;
            CHECK(deepOf(Q.E, kv.second), std::string(f) + " Rly " + hmOf(kv.first) + (t == 2 ? " Pdoc" : " Pboc") + " >= 38.2%");
            if (t == 2) ++nRPdoc; else ++nRPboc; }
        if (std::string(f) == "pboc_es1109.txt") { const long d8 = X.B.back().days - 1; gone2003 = !Q.E.pb.count(T("20:03", d8)) && !Q.E.rpb.count(T("20:03", d8)); }
    }
    std::printf("  decided: Pb %d, Pboc %d, Pdoc %d, Rly Pboc %d, Rly Pdoc %d; shallow pivots skipped %lld\n", nPb, nPboc, nPdoc, nRPboc, nRPdoc, shallow);
    CHECK(nPb > 0 && nPboc > 0 && nPdoc > 0 && nRPboc > 0 && nRPdoc > 0, "every label type occurs in the fixtures");
    CHECK(shallow > 0, "shallow pivots were seen and skipped");
    CHECK(gone2003, "ES 11-09 20:03 (a 1.15.0 Pb, < 38.2%) has no label");
    // [4] no-VAP fallback
    {   const Fix X = load("pboc_es1109.txt", 0.25); Run Q; replay(Q, X, 3, (int)X.B.size() + 5, 0, LLONG_MAX);
        int n = 0; for (const auto& kv : Q.E.pb) { ++n; CHECK(deepOf(Q.E, kv.second), "no-VAP " + hmOf(kv.first) + " >= 38.2%"); }
        CHECK(n > 0, "no-VAP run decided Pbs"); std::printf("  no-VAP: %d Pb decided, all >= 38.2%%\n", n); }
    std::printf("test_bo1160_retrace: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
