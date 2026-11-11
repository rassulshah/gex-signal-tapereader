// test_bo1190_rev.cpp -- lsBreakout 1.19.0 (Rassul 2026-11-11 11:07 CT "Its range is at least 1.2x the 13-bar ATR (measured on the bar before) or it
// closes above the prior bar high. also note that it can be displayed after any type of pb including pboc etc."). Real ES 3m bars + VAP, replayed.
//  [1] every Rev (after a BO / BD pullback and after a Rly Pboc) re-checked from the bars: the right colour, low below the pivot bar's high
//      (bear: high above its low), and range >= 1.2 x ATR(13) OR a close beyond the prior bar; it is the FIRST such bar from the pivot bar
//  [2] both kinds occur: a Rev by range only and a Rev by the close only (new in 1.19.0)
//  [3] a Rev after a Rly Pboc is drawn ("Rev" on its bar); the bar-by-bar replay decides the same Revs as one straight pass (no repaint)
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

static std::vector<double> atr13(const bko::Engine& E) { const int n = (int)E.bars.size(); std::vector<double> A((size_t)n, NAN); double y = 0; bool have = false;
    for (int k = 1; k < n; ++k) { const bko::In& b = E.bars[(size_t)k]; const bko::In& a = E.bars[(size_t)k - 1]; const double tr = std::max(b.h - b.l, std::max(std::fabs(b.h - a.c), std::fabs(b.l - a.c))); y = have ? y + (tr - y) / 13.0 : tr; have = true; A[(size_t)k] = y; } return A; }
int main() {
    CHECK(std::string(bko::VERSION) == "1.23.0", "version 1.23.0");
    int nBig = 0, nPast = 0, nRro = 0, nRroDrawn = 0, nRev = 0;
    const char* F[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
    for (int f = 0; f < 3; ++f) { const Fix X = load(F[f], 0.25); Run R; replay(R, X, 3); const bko::Engine& E = R.E; const std::vector<double> A = atr13(E); int bad = 0, notFirst = 0;
        const bko::Scene S = draw(E); std::set<long long> revTxt; for (const bko::Txt& t : S.txt) if (t.kind == 4 && (t.s == "Rev" || t.s == "RevA")) revTxt.insert(E.bars[(size_t)t.bar].t);
        auto qual = [&](int j, int p, int d) { const bko::In& b = E.bars[(size_t)j]; const bko::In& pv = E.bars[(size_t)p]; const bko::In& pr = E.bars[(size_t)j - 1];
            if (!(d > 0 ? b.c > b.o : b.c < b.o)) return 0;
            if (!(d > 0 ? b.l < pv.h : b.h > pv.l)) return 0;
            const bool big = (b.h - b.l) >= 1.2 * A[(size_t)j - 1] - 1e-9, past = d > 0 ? b.c > pr.h : b.c < pr.l; return (big ? 1 : 0) + (past ? 2 : 0); };
        auto one = [&](const bko::RevMark& m, bool rro) { const int j = barOf(E, m.tr), p = barOf(E, m.tp); if (j < 1 || p < 1 || j < p) { ++bad; return; }
            const int q = qual(j, p, m.dir > 0 ? 1 : -1); if (!q) { ++bad; return; }
            if (q == 1) { ++nBig; }
            if (q == 2) { ++nPast; }
            ++nRev;
            for (int k = p; k < j; ++k) if (qual(k, p, m.dir > 0 ? 1 : -1)) ++notFirst;
            if (rro) { ++nRro; if (revTxt.count(m.tr)) ++nRroDrawn; } };
        for (const auto& kv : E.rev) one(kv.second, false);
        for (const auto& kv : E.rro) one(kv.second, true);
        CHECK(bad == 0, std::string("[1] ") + F[f] + ": every Rev has the colour, the low below the pivot bar's high and (range >= 1.2 x ATR13 or a close beyond the prior bar) (" + std::to_string(bad) + " bad of " + std::to_string(E.rev.size() + E.rro.size()) + ")");
        CHECK(notFirst == 0, std::string("[1] ") + F[f] + ": each Rev is the first qualifying bar from its pivot bar (" + std::to_string(notFirst) + ")");
        bko::Engine G; G.bocLive = true; G.vapAt = [&X, &G](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = X.V.find(G.bars[(size_t)j].t); if (it == X.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; };
        G.update(X.B, (long)X.B.size(), X.B[0].t, 0.25, 180, 510, 900); G.vapAt = nullptr;
        int same = 0, diff = 0; for (const auto& kv : G.rev) { const auto o = E.rev.find(kv.first); if (o != E.rev.end() && o->second.tr == kv.second.tr) ++same; else if (o != E.rev.end()) ++diff; }
        CHECK(diff == 0 && same > 0, std::string("[3] ") + F[f] + ": one straight pass decides the replay's Revs (" + std::to_string(same) + " same, " + std::to_string(diff) + " different)");
        std::printf("  %s: Rev %zu, Rly Pboc Rev %zu\n", F[f], E.rev.size(), E.rro.size()); }
    CHECK(nBig > 0 && nPast > 0, "[2] a Rev by range only (" + std::to_string(nBig) + ") and by the close beyond the prior bar only (" + std::to_string(nPast) + ") of " + std::to_string(nRev));
    CHECK(nRro > 0 && nRroDrawn > 0, "[3] a Rev after a Rly Pboc is drawn (" + std::to_string(nRroDrawn) + " drawn of " + std::to_string(nRro) + ")");
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
