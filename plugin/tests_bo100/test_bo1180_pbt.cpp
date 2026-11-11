// test_bo1180_pbt.cpp -- lsBreakout 1.18.0 (Rassul 2026-11-11): (1) 07:50 CT "this pb should have gotting a pboc too", rule "allow if it doens't
// close below": a pullback is void only when a candle CLOSES beyond its wave's origin (1.17.0: a trade beyond it). (2) 07:33-07:40 CT "if there is
// a pb and it is below this level then it should be called a PbT", "it would be the dashed lne that would be tested", "update pboc to pbocT": a
// drawn Pb / Pboc whose pullback traded at / through a Step Trend level of its direction is drawn PbT / PbocT. Real ES 3m bars + VAP
// (11-09 / 10-21 / 11-04), bar-by-bar replay.
//  [1] no decided label has a candle CLOSING beyond its origin inside its pullback; some have a candle TRADING beyond it (the new allowance)
//  [2] every PbT / PbocT re-checked from the bars alone: its BPT level is an hour's high (low) that a later green (red) hour closed beyond
//      before the decision bar; a pullback bar after the wave end trades at / through it; the wave end is beyond it
//  [3] labels: the scene text is PbT / PbocT exactly for the T marks; a label never changes once final (no repaint); BPT lines dedupe
//  [4] a chart period that does not divide the hour: no T
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bo1180_pbt.cpp -o t1180 && ./t1180
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
[[maybe_unused]] static long long T(const char* hm, long day) { int h, m; std::sscanf(hm, "%d:%d", &h, &m); return (long long)day * 86400 + h * 3600 + m * 60 + OFF; }
[[maybe_unused]] static std::string hmOf(long long t) { const long long s = (t - OFF) % 86400; char b[8]; std::snprintf(b, sizeof b, "%02lld:%02lld", s / 3600, (s / 60) % 60); return b; }
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

static int barOf(const bko::Engine& E, long long t) { const auto it = std::lower_bound(E.bars.begin(), E.bars.end(), t, [](const bko::In& b, long long x) { return b.t < x; }); return it != E.bars.end() && it->t == t ? (int)(it - E.bars.begin()) : -1; }
struct Hr { long long k; double o, h, l, c; };
static std::vector<Hr> hoursTo(const bko::Engine& E, int id) {   // clock hours of the bar START, closed by the decision bar
    std::vector<Hr> H; for (int k = 0; k <= id; ++k) { const bko::In& b = E.bars[(size_t)k]; const long long st = (long long)b.days * 86400 + b.endSec - 180, hk = st >= 0 ? st / 3600 : -1;
        if (H.empty() || H.back().k != hk) H.push_back({ hk, b.o, b.h, b.l, b.c }); else { Hr& h = H.back(); h.h = std::max(h.h, b.h); h.l = std::min(h.l, b.l); h.c = b.c; } }
    const bko::In& d = E.bars[(size_t)id]; const long long endHk = ((long long)d.days * 86400 + d.endSec) / 3600; if (!H.empty() && H.back().k >= endHk) H.pop_back();   // the forming hour is not closed
    return H; }
static void check(const Run& Q, const char* nm, int& nT, int& nPlain, int& nTraded) {
    const bko::Engine& E = Q.E; int closeBad = 0, tBad = 0, lblBad = 0; const bko::Scene S = draw(E);
    std::map<long long, std::string> txt; for (const bko::Txt& t : S.txt) if (t.kind == 2) txt[E.bars[(size_t)t.bar].t] = t.s;
    auto one = [&](const bko::PbMark& m, bool rlyK, int tflag, const std::string& line) {
        const int i1 = barOf(E, m.tw1), pi = barOf(E, m.tp), id = barOf(E, m.td), i0 = barOf(E, m.tw0); if (i1 < 0 || pi < 0 || id < 0) return;
        if (i0 >= 0) { const double org = m.dir > 0 ? E.bars[(size_t)i0].l : E.bars[(size_t)i0].h; bool tr = false;
            for (int k = i1 + 1; k <= pi; ++k) { const bko::In& b = E.bars[(size_t)k]; if (m.dir > 0 ? b.c < org : b.c > org) ++closeBad; if (m.dir > 0 ? b.l < org : b.h > org) tr = true; }
            if (tr) ++nTraded; }
        const auto tx = txt.find(m.tp); const bool drawnT = tx != txt.end() && (tx->second == "PbT" || tx->second == "PbocT");
        if (tx != txt.end() && drawnT != (tflag == 1)) ++lblBad;
        if (tx != txt.end() && rlyK && tflag == 1 && tx->second != "PbocT") ++lblBad;
        if (tflag != 1) { ++nPlain; if (!line.empty()) ++tBad; return; }
        ++nT; double lvl = 0; long long tt = 0, td = 0; int side = 0; char kind[8] = { 0 }; char ver[40] = { 0 };
        if (std::sscanf(line.c_str(), "BPT|%*d|%d|%7[^|]|%lf|%lld|%lld|%39s", &side, kind, &lvl, &tt, &td, ver) != 6 || side != m.dir || std::string(kind) != (rlyK ? "rly" : "pb") || td != m.td) { ++tBad; return; }
        const int it = barOf(E, tt); if (it <= i1 || it > pi) { ++tBad; return; }
        const bko::In& tb = E.bars[(size_t)it]; if (!(m.dir > 0 ? tb.l <= lvl : tb.h >= lvl)) { ++tBad; return; }
        const bko::In& w = E.bars[(size_t)i1]; if (!(m.dir > 0 ? w.h > lvl : w.l < lvl)) { ++tBad; return; }
        const std::vector<Hr> H = hoursTo(E, id); bool isStep = false;
        for (size_t k = 1; k < H.size() && !isStep; ++k) isStep = m.dir > 0 ? (H[k - 1].h == lvl && H[k].c > lvl && H[k].c > H[k].o) : (H[k - 1].l == lvl && H[k].c < lvl && H[k].c < H[k].o);
        if (!isStep) ++tBad; };
    for (const auto& kv : E.pb) { const auto t = E.pbT.find(kv.first); const auto l = E.pbtL.find(kv.first); one(kv.second, false, t == E.pbT.end() ? -1 : t->second, l == E.pbtL.end() ? std::string() : l->second.line); }
    for (const auto& kv : E.rpb) { const auto bc = E.rpbBoc.find(kv.first); if (bc == E.rpbBoc.end() || bc->second != 1) continue;
        const auto t = E.rpbT.find(kv.first); const auto l = E.rpbtL.find(kv.first); one(kv.second, true, t == E.rpbT.end() ? -1 : t->second, l == E.rpbtL.end() ? std::string() : l->second.line); }
    CHECK(closeBad == 0, std::string("[1] ") + nm + ": no decided pullback has a candle closing beyond its origin (" + std::to_string(closeBad) + ")");
    CHECK(tBad == 0, std::string("[2] ") + nm + ": every PbT / PbocT re-checked from the bars (level = a broken hour high / low, tested after the wave end) (" + std::to_string(tBad) + " bad)");
    CHECK(lblBad == 0, std::string("[3] ") + nm + ": the text is PbT / PbocT exactly for the T marks (" + std::to_string(lblBad) + " bad)");
    CHECK(Q.switches == 0, std::string("[3] ") + nm + ": a final label never changes (" + std::to_string(Q.switches) + " switches)");
    std::set<std::string> keys; int dup = 0, badKey = 0; for (const auto* M : { &E.pbtL, &E.rpbtL }) for (const auto& kv : *M) { const std::string k = bko::recKey(kv.second.line); if (k.empty()) ++badKey; else if (!keys.insert(k).second) ++dup; }
    CHECK(dup == 0 && badKey == 0, std::string("[3] ") + nm + ": BPT lines have a record key, one per mark");
    std::printf("  %s: Pb %zu, Rly marks %zu, T %zu + %zu\n", nm, E.pb.size(), E.rpb.size(), E.pbtL.size(), E.rpbtL.size());
}
int main() {
    CHECK(std::string(bko::VERSION) == "1.18.0", "version 1.18.0");
    int nT = 0, nPlain = 0, nTraded = 0;
    const char* F[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
    for (int f = 0; f < 3; ++f) { const Fix X = load(F[f], 0.25); CHECK(X.B.size() > 200, std::string(F[f]) + " loaded"); Run R; replay(R, X, 3); check(R, F[f], nT, nPlain, nTraded);
        if (f == 0) {   // one straight pass = the bar-by-bar replay (same T marks)
            bko::Engine E; E.bocLive = true; E.vapAt = [&X, &E](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = X.V.find(E.bars[(size_t)j].t); if (it == X.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; };
            E.update(X.B, (long)X.B.size(), X.B[0].t, 0.25, 180, 510, 900); E.vapAt = nullptr;
            int same = 0, diff = 0; for (const auto& kv : E.pbT) { const auto o = R.E.pbT.find(kv.first); if (o != R.E.pbT.end()) { if (o->second == kv.second) ++same; else ++diff; } }
            CHECK(diff == 0 && same > 0, "[3] one straight pass gives the replay's T marks (" + std::to_string(same) + " same, " + std::to_string(diff) + " different)");
            bko::Engine G = E; G.spb = 420; int judged = 0; auto idx = [&G](long long t) { return barOf(G, t); };
            for (const auto& kv : G.pb) { double l = 0; long long tt = 0; if (G.stepTest(kv.second, idx, l, tt) != -1) ++judged; }
            CHECK(judged == 0 && !G.pb.empty(), "[4] a 7-minute chart: no pullback is judged against the steps"); } }
    CHECK(nT > 0, "[2] at least one PbT / PbocT in the three days (" + std::to_string(nT) + ")");
    CHECK(nPlain > 0, "[2] at least one plain Pb / Pboc (" + std::to_string(nPlain) + ")");
    CHECK(nTraded > 0, "[1] at least one decided pullback traded beyond its origin without closing there (" + std::to_string(nTraded) + ")");
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
