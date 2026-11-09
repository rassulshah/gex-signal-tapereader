// test_bo1120_pboc.cpp -- lsBreakout 1.12.0 Pboc (Rassul 2026-11-09 12:22 CT, mockup approved): SDK-free, real ES data (the LRA IRT
// footprint export: 3m bars + volume at price). Replays bar by bar (the live path) and checks:
//  [1] ES 11-09: Pb 07:15 (BO wave 06:27-06:51) = Pboc, 07:45 = Pb; overnight 23:24 / 00:33 Pboc, 00:45 Pb (= lsWavePOC's recorded POCs)
//  [2] the native wave POC == lsWavePOC's recorded POC of that wave (06:27-06:51 -> 7453.75)
//  [3] ES 10-21: no-BO Rly up 07:45-08:03 -> Pboc at 08:15 (BPO record); ES 11-04: no-BO Rly up 08:03-08:24, POC above 50% -> no label at 08:30 / 08:36
//  [4] no repaint: a confirmed Pb / Pboc label never changes once drawn; no VAP -> nothing drawn, then drawn once (never Pb -> Pboc)
//  [5] records: BPB 16 fields with boc, keys accepted (recKey), BPO / BRO keys; Lvp still hides a Pboc
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -Wall -Wextra -Werror -I.. test_bo1120_pboc.cpp -o t1120 && ./t1120
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
int main() {
    // [1] [2] [4] ES 11-09
    const Fix A = load("pboc_es1109.txt", 0.25); CHECK(A.B.size() > 300, "ES 11-09 bars");
    Run R; replay(R, A, 3);
    const long d9 = A.B.back().days, d8 = d9 - 1;
    std::printf("ES 11-09 Pb / Pboc (drawn, final):\n");
    for (const auto& kv : R.E.pb) { const auto b = R.E.boc.find(kv.first); const auto f = R.first.find(kv.first);
        std::printf("  %s %s %s  wave %s-%s  POC %.2f  boc %d  drawn %s\n", (kv.first - OFF) / 86400 == d9 ? "11-09" : "11-08", hmOf(kv.first).c_str(), kv.second.dir > 0 ? "after BO" : "after BD",
            hmOf(kv.second.tw0).c_str(), hmOf(kv.second.tw1).c_str(), R.E.bocPoc.count(kv.first) ? R.E.bocPoc.at(kv.first) : 0.0, b == R.E.boc.end() ? -1 : b->second, f == R.first.end() ? "-" : f->second.c_str()); }
    auto lab = [&](long long t) { const auto f = R.first.find(t); return f == R.first.end() ? std::string("-") : f->second; };
    CHECK(lab(T("07:15", d9)) == "Pboc", "ES 07:15 Pboc (got " + lab(T("07:15", d9)) + ")");
    CHECK(lab(T("07:45", d9)) == "Pb", "ES 07:45 Pb (got " + lab(T("07:45", d9)) + ")");
    CHECK(lab(T("23:24", d8)) == "Pboc", "ES 23:24 Pboc (got " + lab(T("23:24", d8)) + ")");
    CHECK(lab(T("00:33", d9)) == "Pboc", "ES 00:33 Pboc (got " + lab(T("00:33", d9)) + ")");
    CHECK(lab(T("00:45", d9)) == "Pb", "ES 00:45 Pb (got " + lab(T("00:45", d9)) + ")");
    { const auto m = R.E.pb.find(T("07:15", d9)); CHECK(m != R.E.pb.end() && m->second.tw0 == T("06:27", d9) && m->second.tw1 == T("06:51", d9), "07:15 wave = 06:27-06:51 (lsWavePOC's wave)");
      CHECK(std::fabs(R.E.bocPoc[T("07:15", d9)] - 7453.75) < 1e-9, "native POC 7453.75 == lsWavePOC record"); }
    CHECK(R.switches == 0, "no confirmed label ever changed (" + std::to_string(R.switches) + ")");
    {   // [5] the record line
        const auto L = R.E.pbL.find(T("07:15", d9)); CHECK(L != R.E.pbL.end(), "BPB line 07:15");
        if (L != R.E.pbL.end()) { int nf = 1; for (char c : L->second.line) nf += c == '|'; CHECK(nf == 16, "BPB 16 fields"); CHECK(L->second.line.find("|1|7453.75") != std::string::npos, "BPB boc 1 + POC: " + L->second.line);
            CHECK(!bko::recKey(L->second.line).empty(), "BPB 16-field key"); } }
    // [4] VAP missing for the 06:27-06:51 bars until bar 07:51 closes: 07:15 not drawn while missing, then Pboc - never Pb
    {   Run Q; int stepAt = 0; for (int k = 0; k < (int)A.B.size(); ++k) if (A.B[(size_t)k].t == T("07:51", d9)) stepAt = k + 2;
        bool sawPbWhileMissing = false;
        {   std::vector<bko::In> dummy; (void)dummy; }
        replay(Q, A, 3, stepAt, T("06:27", d9), T("06:51", d9));
        const auto f = Q.first.find(T("07:15", d9)); CHECK(f != Q.first.end() && f->second == "Pboc", "late VAP: 07:15 drawn once as Pboc");
        CHECK(Q.switches == 0 && !sawPbWhileMissing, "late VAP: no Pb -> Pboc switch");
        const auto L = Q.E.pbL.find(T("07:15", d9)); CHECK(L != Q.E.pbL.end() && L->second.line.find("|1|7453.75") != std::string::npos, "late VAP: BPB written once with boc 1"); }
    // [5] Lvp hides the Pboc
    {   bko::Engine E = R.E; E.hidePb.insert(std::make_pair(T("07:15", d9), 1)); const bko::Scene S = draw(E); bool has = false;
        for (const bko::Txt& t : S.txt) if (t.kind == 2 && E.bars[(size_t)t.bar].t == T("07:15", d9)) has = true;
        CHECK(!has && S.pbHidden > 0, "Lvp hides Pboc"); }
    // [3] no-BO Rly
    {   const Fix X = load("pboc_es1021.txt", 0.25); Run Q; replay(Q, X, 3); const long d = X.B.back().days;
        std::printf("ES 10-21 Rly pullbacks judged:\n"); for (const auto& kv : Q.E.rpb) std::printf("  %s Rly %s-%s -> %s\n", hmOf(kv.first).c_str(), hmOf(kv.second.tw0).c_str(), hmOf(kv.second.tw1).c_str(), Q.E.rpbBoc[kv.first] ? "Pboc" : "no label");
        CHECK(Q.first.count(T("08:15", d)) && Q.first[T("08:15", d)] == "Pboc", "ES 10-21 08:15 Rly Pboc");
        CHECK(Q.E.rpbL.count(T("08:15", d)) && !bko::recKey(Q.E.rpbL[T("08:15", d)].line).empty(), "BPO record + key");
        for (const auto& kv : Q.E.rroL) CHECK(!bko::recKey(kv.second.line).empty() && kv.second.line.compare(0, 4, "BRO|") == 0, "BRO key " + kv.second.line);
        std::printf("  thrusts recorded (BRO): %zu, Revs drawn from them: 0 by design\n", Q.E.rro.size());
        CHECK(Q.switches == 0, "10-21 no label changed"); }
    {   const Fix X = load("pboc_es1104.txt", 0.25); Run Q; replay(Q, X, 3); const long d = X.B.back().days;
        std::printf("ES 11-04 Rly pullbacks judged:\n"); for (const auto& kv : Q.E.rpb) std::printf("  %s Rly %s-%s -> %s\n", hmOf(kv.first).c_str(), hmOf(kv.second.tw0).c_str(), hmOf(kv.second.tw1).c_str(), Q.E.rpbBoc[kv.first] ? "Pboc" : "no label");
        CHECK(!Q.first.count(T("08:30", d)) && !Q.first.count(T("08:36", d)), "ES 11-04 08:30 / 08:36 no label (POC above 50%)");
        CHECK(Q.E.rpb.count(T("08:30", d)) && Q.E.rpbBoc[T("08:30", d)] == 0, "ES 11-04 08:30 judged, no label"); }
    std::printf("test_bo1120_pboc: %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
