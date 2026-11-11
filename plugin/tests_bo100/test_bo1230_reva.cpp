// test_bo1230_reva.cpp -- lsBreakout 1.23.0 (Rassul 2026-11-11 14:41 CT "2. ok and only delta profile"): a Rev whose pullback pivot bar (or the bar
// either side, up to the Rev bar) has an ABSORBED DELTA NODE is drawn "RevA". Node = the bar's biggest attackers' 2-row delta band >= 2.0 x the bar's
// median 2-row band, with the bar closing its own colour beyond the band. Real ES 3m bars + VAP, replayed bar by bar.
//  [1] every Rev's absn re-derived from the fixture's VAP by an independent rule   [2] both kinds occur; "RevA" is drawn exactly on the absn Revs
//  [3] the bar-by-bar replay = one straight pass (decided once, no repaint)        [4] no VAP = plain "Rev"
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


static int barOf(const bko::Engine& E, long long t) { for (size_t i = 0; i < E.bars.size(); ++i) if (E.bars[i].t == t) return (int)i; return -1; }
static bool node(const Fix& X, const bko::In& b, int d) { if (!(d > 0 ? b.c > b.o : b.c < b.o)) return false; const auto it = X.V.find(b.t); if (it == X.V.end() || it->second.size() < 3) return false;
    std::map<long long, long long> m; for (const wpc::VRow& r : it->second) m[r.tick] += r.dlt; std::vector<std::pair<long long, long long>> v(m.begin(), m.end());
    std::vector<double> bd; double best = 0; long long lo = 0, hi = 0; bool have = false;
    for (size_t k = 0; k + 1 < v.size(); ++k) if (v[k + 1].first - v[k].first == 1) { const double s = (double)(v[k].second + v[k + 1].second) * d; bd.push_back(std::fabs(s)); if (!have || s < best) { have = true; best = s; lo = v[k].first; hi = v[k + 1].first; } }
    if (!have || best >= 0) return false;
    std::sort(bd.begin(), bd.end()); const size_t n = bd.size(); double med = n % 2 ? bd[n / 2] : (bd[n / 2 - 1] + bd[n / 2]) / 2; if (!(med > 0)) med = 1;
    if (std::fabs(best) < 2.0 * med) return false;
    return d > 0 ? b.c / 0.25 > (double)hi + 1e-6 : b.c / 0.25 < (double)lo - 1e-6; }
int main() {
    CHECK(std::string(bko::VERSION) == "1.23.2", "version 1.23.2");
    int nA = 0, nPlain = 0, bad = 0, txtBad = 0, drawnA = 0;
    const char* F[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
    for (int f = 0; f < 3; ++f) { const Fix X = load(F[f], 0.25); Run R; replay(R, X, 3); const bko::Engine& E = R.E;
        std::map<long long, std::string> txt; { const bko::Scene S = draw(E); for (const bko::Txt& t : S.txt) if (t.kind == 4) txt[E.bars[(size_t)t.bar].t] = t.s; }
        auto one = [&](const bko::RevMark& m) { const int j = barOf(E, m.tr), p = barOf(E, m.tp); if (j < 1 || p < 1) { ++bad; return; }
            bool want = false; for (int k = p - 1; k <= std::min(j, p + 1); ++k) if (node(X, E.bars[(size_t)k], m.dir > 0 ? 1 : -1)) want = true;
            if (!bko::REV_ABS_MARK) want = false;   // (1.23.1) the mark is switched off: no Rev carries it
            if (want != (m.absn == 1)) ++bad;
            if (m.absn) ++nA; else ++nPlain;
            const auto it = txt.find(m.tr); if (it != txt.end()) { if (it->second == "RevA") ++drawnA; } };
        for (const auto& kv : E.rev) one(kv.second);
        for (const auto& kv : E.rro) one(kv.second);
        // a bar can carry two Revs (after a Pb and after a Rly Pboc): its text is RevA when either is
        std::map<long long, int> anyA; for (const auto& kv : E.rev) anyA[kv.second.tr] |= kv.second.absn; for (const auto& kv : E.rro) anyA[kv.second.tr] |= kv.second.absn; for (const bko::RevMark& x : E.revProv) anyA[x.tr] |= x.absn;
        for (const auto& kv : txt) { const auto a = anyA.find(kv.first); if (a == anyA.end()) continue; if ((kv.second == "RevA") && !a->second) ++txtBad; }
        Run S1; replay(S1, X, (int)X.B.size()); int diff = 0; for (const auto& kv : E.rev) { const auto o = S1.E.rev.find(kv.first); if (o == S1.E.rev.end() || o->second.absn != kv.second.absn) ++diff; }
        CHECK(diff == 0 && S1.E.rev.size() == E.rev.size(), std::string("[3] bar-by-bar = one pass: ") + F[f]);
        if (f == 0) { bko::Engine N; std::vector<bko::In> B(X.B.begin(), X.B.end()); N.bocLive = false; N.vapAt = nullptr; N.update(B, (long)B.size(), X.B[0].t, 0.25, 180, 510, 900); int a = 0; for (const auto& kv : N.rev) a += kv.second.absn; CHECK(a == 0, "[4] no VAP: no RevA"); }
    }
    CHECK(bad == 0, "[1] absn = the independent node rule on every Rev (" + std::to_string(nA + nPlain) + " Revs)");
    CHECK(bko::REV_ABS_MARK ? (nA >= 3 && nPlain >= 3) : (nA == 0 && nPlain >= 6), "[2] both kinds occur: RevA " + std::to_string(nA) + ", Rev " + std::to_string(nPlain));
    CHECK(txtBad == 0 && (bko::REV_ABS_MARK ? drawnA >= 3 : drawnA == 0), "[2] RevA is drawn only on absorbed Revs (" + std::to_string(drawnA) + " drawn)");
    std::printf("test_bo1230_reva: %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
