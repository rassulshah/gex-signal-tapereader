// test_bo1240_rlytrend.cpp -- lsBreakout 1.24.0 (Rassul 2026-11-11 20:26-20:27 CT "you dont need the vwap, if we are using the hourly trend",
// "remove the vwap contidion. we need to align witht he hbo instead"): a Rly fires on its 4 wave conditions (3+ bars, 2.5 ATR, +-1 SD tag or
// 30-bar extreme, 20-bar extreme) WITHOUT the vwapTrend condition; it is drawn only inside an hourly breakout bar of its direction (1.21.0).
// Real ES 3m bars + VAP (the 1.21.0 fixtures), one straight pass.
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

[[maybe_unused]] static int barOf(const bko::Engine& E, long long t) { const auto it = std::lower_bound(E.bars.begin(), E.bars.end(), t, [](const bko::In& b, long long x) { return b.t < x; }); return it != E.bars.end() && it->t == t ? (int)(it - E.bars.begin()) : -1; }

static std::map<long long, int> stepHours(const bko::Engine& E) { stp::Engine S; for (size_t k = 0; k + 1 < E.bars.size(); ++k) { const bko::In& b = E.bars[k]; stp::Bar x; x.t = (long long)b.days * 86400LL + b.endSec - 180;
        wpc::toTicks(b.o, 0.25, x.o); wpc::toTicks(b.h, 0.25, x.h); wpc::toTicks(b.l, 0.25, x.l); wpc::toTicks(b.c, 0.25, x.c); S.addBar(x); }
    std::map<long long, int> m; for (const stp::Step& q : S.S) m[q.hour] = q.dir; return m; }
static long long hourOf(const bko::In& b) { return stp::floorDiv((long long)b.days * 86400LL + b.endSec - 180, 3600) * 3600; }
int main() {
    CHECK(std::string(bko::VERSION) == "1.24.0" && std::string(rly::VERSION) == "1.5.0" && !bko::RLY_NEEDS_VWAP_TREND && bko::RLY_NEEDS_HBO, "versions 1.24.0 / RallyLogic 1.5.0; vwapTrend not needed, hourly breakout bar needed");
    int oldAll = 0, newAll = 0, drawnAll = 0, early = 0; const char* F[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
    for (int f = 0; f < 3; ++f) { const Fix X = load(F[f], 0.25);
        bko::Engine G; G.bocLive = true; G.vapAt = [&X, &G](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = X.V.find(G.bars[(size_t)j].t); if (it == X.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; };
        G.update(X.B, (long)X.B.size(), X.B[0].t, 0.25, 180, 510, 900); G.vapAt = nullptr;
        std::vector<rly::Bar> B; std::vector<long long> T; for (size_t k = 0; k + 1 < X.B.size(); ++k) { const bko::In& x = X.B[k]; rly::Bar b; b.days = x.days; b.endSec = x.endSec; b.o = x.o; b.h = x.h; b.l = x.l; b.c = x.c; b.v = (double)x.vol; B.push_back(b); T.push_back(x.t); }
        const std::vector<rly::Wave> A = rly::run(B, 180, 510, 900, 0, G.zzRev, true, true), N = rly::run(B, 180, 510, 900, 0, G.zzRev, true, false);
        bool same = A.size() == N.size(); int o = 0, n = 0, lost = 0, badExtra = 0, noTrendYet = 0;
        for (size_t i = 0; same && i < A.size(); ++i) { const rly::Wave& a = A[i]; const rly::Wave& b = N[i];
            same = a.ge == b.ge && a.gp == b.gp && a.dir == b.dir && a.trend == b.trend && a.c[4] == b.c[4];
            o += a.fired; n += b.fired; if (a.fired && !b.fired) ++lost;
            if (b.fired && !a.fired) { if (!(b.c[0] && b.c[1] && b.c[2] && b.c[3]) || b.c[4]) ++badExtra; if (b.trend == 0) ++noTrendYet; } }
        CHECK(same && lost == 0 && badExtra == 0 && n > o, std::string("[1] ") + F[f] + ": the same waves; every old Rly still fires; each new one passes the 4 wave conditions and failed ONLY vwapTrend (" + std::to_string(o) + " -> " + std::to_string(n) + ")");
        int eng = 0, miss = 0; for (const rly::Wave& w : N) if (w.fired) { const long long key = T[(size_t)w.ge] * 2 + (w.dir > 0 ? 1 : 0); const auto it = G.rly.find(key); if (it == G.rly.end() || !it->second.fired) ++miss; }
        for (const auto& kv : G.rly) eng += kv.second.fired;
        CHECK(miss == 0 && eng == n, std::string("[2] ") + F[f] + ": the plugin engine fires exactly these (" + std::to_string(eng) + ")");
        const bko::Scene S = draw(G); const std::map<long long, int> H = stepHours(G); int drawn = 0, bad = 0;
        for (const bko::Txt& t : S.txt) if (t.kind == 1) { ++drawn; const bko::In& b = G.bars[(size_t)t.bar]; const auto it = H.find(hourOf(b)); if (it == H.end() || (it->second > 0) != (t.dir > 0)) ++bad; }
        CHECK(bad == 0 && drawn > 0 && drawn <= n, std::string("[3] ") + F[f] + ": every DRAWN Rly is inside an hourly breakout bar of its direction (" + std::to_string(drawn) + " drawn of " + std::to_string(n) + " fired, " + std::to_string(bad) + " bad)");
        oldAll += o; newAll += n; drawnAll += drawn; early += noTrendYet;
        std::printf("  %s: Rly fired with vwapTrend %d -> without %d (+%d, of them %d had no trend reading yet); drawn (in an hourly breakout bar) %d\n", F[f], o, n, n - o, noTrendYet, drawn); }
    CHECK(newAll > oldAll && early >= 1, "[4] waves in a session's first bars (no vwapTrend reading yet) can now be a Rly: " + std::to_string(early));
    {   // lsBigBidAsk's call (default needTrend = true) is unchanged
        const Fix X = load(F[0], 0.25); std::vector<rly::Bar> B; for (size_t k = 0; k + 1 < X.B.size(); ++k) { const bko::In& x = X.B[k]; rly::Bar b; b.days = x.days; b.endSec = x.endSec; b.o = x.o; b.h = x.h; b.l = x.l; b.c = x.c; b.v = (double)x.vol; B.push_back(b); }
        const std::vector<rly::Wave> D = rly::run(B, 180, 510, 900, 0), E = rly::run(B, 180, 510, 900, 0, 1, false, true); bool eq = D.size() == E.size(); int bad = 0; for (size_t i = 0; eq && i < D.size(); ++i) { eq = D[i].fired == E[i].fired; if (D[i].fired && !D[i].c[4]) ++bad; }
        CHECK(eq && bad == 0 && !D.empty(), "[5] the default call (lsBigBidAsk) still needs vwapTrend: unchanged"); }
    std::printf("test_bo1240_rlytrend: %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
