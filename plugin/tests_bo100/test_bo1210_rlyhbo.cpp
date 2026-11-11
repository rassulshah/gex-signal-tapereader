// test_bo1210_rlyhbo.cpp -- lsBreakout 1.21.0 (Rassul 2026-11-11 08:19 / 10:21 / 12:51 CT: a Rly is displayed only when it is part of the hourly
// breakout bar). Real ES 3m bars + VAP, replayed bar by bar.
//  [1] every drawn Rly sits in a clock hour that is a Step Trend breakout hour of its direction (re-computed here from the bars)
//  [2] some fired Rly are not drawn (their hour was not a breakout hour); a Rly whose hour has not closed is not drawn yet
//  [3] no Rly Pboc on a wave whose Rly is not shown; the replay draws the Rly of one straight pass (decided once)
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
    CHECK(std::string(bko::VERSION) == "1.21.0", "version 1.21.0");
    int drawnAll = 0, firedAll = 0; const char* F[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
    for (int f = 0; f < 3; ++f) { const Fix X = load(F[f], 0.25); Run R; replay(R, X, 3); const bko::Engine& E = R.E; const bko::Scene S = draw(E); const std::map<long long, int> H = stepHours(E);
        int drawn = 0, bad = 0, fired = 0, pbBad = 0;
        for (const bko::Txt& t : S.txt) if (t.kind == 1) { ++drawn; const bko::In& b = E.bars[(size_t)t.bar]; const auto it = H.find(hourOf(b)); if (it == H.end() || (it->second > 0) != (t.dir > 0)) ++bad; }
        for (const auto& kv : E.rly) if (kv.second.fired) ++fired;
        for (const auto& kv : E.rpb) { bool ok = false; for (const auto& r : E.rly) if (r.second.fired && r.second.te == kv.second.tw1 && E.rlyShown(r.second)) ok = true; if (!ok) ++pbBad; }
        CHECK(bad == 0 && drawn > 0, std::string("[1] ") + F[f] + ": every drawn Rly is inside an hourly breakout bar of its direction (" + std::to_string(drawn) + " drawn, " + std::to_string(bad) + " bad)");
        CHECK(pbBad == 0, std::string("[3] ") + F[f] + ": every Rly pullback belongs to a shown Rly (" + std::to_string(pbBad) + " bad)");
        bko::Engine G; G.bocLive = true; G.vapAt = [&X, &G](int j, std::vector<wpc::VRow>& out) { out.clear(); const auto it = X.V.find(G.bars[(size_t)j].t); if (it == X.V.end()) return 0; out = it->second; return out.empty() ? 0 : 1; };
        G.update(X.B, (long)X.B.size(), X.B[0].t, 0.25, 180, 510, 900); G.vapAt = nullptr;
        int same = 0, diff = 0; for (const auto& kv : G.rlyHbo) { const auto o = E.rlyHbo.find(kv.first); if (o != E.rlyHbo.end()) { if (o->second == kv.second) ++same; else ++diff; } }
        CHECK(diff == 0 && same > 0, std::string("[3] ") + F[f] + ": the replay decides what one straight pass decides (" + std::to_string(same) + " same, " + std::to_string(diff) + " different)");
        drawnAll += drawn; firedAll += fired; std::printf("  %s: %d fired Rly, %d drawn, not in an hourly breakout bar / hour open %d\n", F[f], fired, drawn, S.rlyNoHbo); }
    CHECK(drawnAll < firedAll && drawnAll >= 10, "[2] some Rly are not drawn: " + std::to_string(drawnAll) + " of " + std::to_string(firedAll));
    {   bko::Engine Z; Z.spb = 420; Z.tick = 0.25; bko::RlyMark r; r.fired = true; r.te = 1; r.dir = 1; CHECK(Z.rlyShown(r), "a 7-minute chart: the rule is off, every Rly counts"); }
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
