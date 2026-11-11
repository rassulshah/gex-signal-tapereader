// test_bo1200_pct.cpp -- lsBreakout 1.20.0 (Rassul 2026-11-11 12:27 CT "I want to add the retracement amount right below any type of pb like 45%",
// "Keep it two digits"). Real ES 3m bars + VAP, replayed bar by bar.
//  [1] every retracement text is two digits + "%", sits right under a pullback label on the same bar in its colour, and equals the retracement
//      re-computed from the bars (wave end -> pivot over origin -> wave end); none under 38%
//  [2] every drawn decided pullback label has one (or it had no room); no other text overlaps it; labels unchanged
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

int main() {
    CHECK(std::string(bko::VERSION) == "1.22.0", "version 1.22.0");
    int total = 0; const char* F[3] = { "pboc_es1109.txt", "pboc_es1021.txt", "pboc_es1104.txt" };
    for (int f = 0; f < 3; ++f) { const Fix X = load(F[f], 0.25); Run R; replay(R, X, 3); const bko::Engine& E = R.E; const bko::Scene S = draw(E);
        int bad = 0, n = 0, lab = 0, over = 0;
        for (size_t i = 0; i < S.txt.size(); ++i) { const bko::Txt& t = S.txt[i];
            if (t.kind == 2) { const long long bt = E.bars[(size_t)t.bar].t; if (E.pb.count(bt) || E.rpb.count(bt)) ++lab; }
            if (t.kind != 5) continue;
            ++n;
            if (t.s.size() != 3 || t.s[2] != '%' || t.s[0] < '0' || t.s[0] > '9' || t.s[1] < '0' || t.s[1] > '9') { ++bad; continue; }
            const bko::Txt* L = nullptr; for (const bko::Txt& o : S.txt) if (o.kind == 2 && o.bar == t.bar && o.f.b + 1 == t.f.t && o.col == t.col) L = &o;
            if (!L) { ++bad; continue; }
            const long long bt = E.bars[(size_t)t.bar].t; const bko::PbMark* P = nullptr; { const auto a = E.pb.find(bt); if (a != E.pb.end()) P = &a->second; else { const auto b2 = E.rpb.find(bt); if (b2 != E.rpb.end()) P = &b2->second; } }
            if (!P) { ++bad; continue; }
            const int i0 = barOf(E, P->tw0), i1 = barOf(E, P->tw1), ip = barOf(E, P->tp); if (i0 < 0 || i1 < 0 || ip < 0) { ++bad; continue; }
            const double mv = P->dir > 0 ? E.bars[(size_t)i1].h - E.bars[(size_t)i0].l : E.bars[(size_t)i0].h - E.bars[(size_t)i1].l, bk = P->dir > 0 ? E.bars[(size_t)i1].h - E.bars[(size_t)ip].l : E.bars[(size_t)ip].h - E.bars[(size_t)i1].l;
            const int want = std::min(99, (int)std::lround(100.0 * bk / mv)); if (std::atoi(t.s.c_str()) != want || want < 38) ++bad;
            for (size_t j = 0; j < S.txt.size(); ++j) if (j != i && bko::hits(t.f, S.txt[j].f)) ++over; }
        CHECK(bad == 0 && n > 0, std::string("[1] ") + F[f] + ": every retracement text is NN%, under its label, the right value, 38% or more (" + std::to_string(n) + " texts, " + std::to_string(bad) + " bad)");
        CHECK(n + S.pctNoRoom >= lab && over == 0, std::string("[2] ") + F[f] + ": every decided label has its retracement or had no room; nothing overlaps it (" + std::to_string(lab) + " labels, " + std::to_string(n) + " texts, no room " + std::to_string(S.pctNoRoom) + ", overlaps " + std::to_string(over) + ")");
        CHECK(R.switches == 0, std::string("[2] ") + F[f] + ": the labels never change");
        total += n; std::printf("  %s: %d labels, %d retracement texts\n", F[f], lab, n); }
    CHECK(total >= 10, "retracement texts in the three days: " + std::to_string(total));
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
