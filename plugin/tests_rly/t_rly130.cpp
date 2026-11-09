// t_rly130.cpp - RallyLogic 1.3.0 (overnight Rly from the 17:00 CT open). build (plugin\tests_rly):
//   g++ -std=c++17 -I.. t_rly130.cpp -o t130 && ./t130 es_2026-11-08_on_3m.txt ES_3m_2026-10_11.txt
//  [1] ACCEPTANCE: ES Sun 2026-11-08 Globex - the three up waves Rassul called rallies print Rly (tops ~19:06 / ~19:54 / ~20:39 CT)
//  [2] RTH unchanged: every RTH wave (fired or not) is the same as RallyLogic 1.2.0 (RallyLogic_120.h) on Oct-Nov ES bars
//  [3] overnight waves the 1.2.0 rule judged (02:00 CT on) are unchanged; only waves ending 17:00-02:00 CT are new
//  [4] NO REPAINT: replaying the night one closed bar at a time, every decided wave keeps its fields; the last pass = the straight pass
#include "../RallyLogic.h"
namespace old120 {
#define rly rly120
#include "RallyLogic_120.h"
#undef rly
}
#include <fstream>
#include <map>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %d %s\n", __LINE__, std::string(m).c_str()); } } while (0)
template <class Bar> static std::vector<Bar> load(const char* p) { std::vector<Bar> v; std::ifstream f(p); Bar b; while (f >> b.days >> b.endSec >> b.o >> b.h >> b.l >> b.c >> b.v) v.push_back(b); return v; }
// (1.4.0) compared on rules 1-5: the later HH / LL rule only removes fires (t_rly140.cpp tests it)
static std::string key5(const rly::Wave& w) { char b[160]; std::snprintf(b, sizeof b, "%d|%d|%d|%d|%d|%.4f|%.4f|%d", w.gp, w.ge, w.gc, w.dir, (w.c[0] && w.c[1] && w.c[2] && w.c[3] && w.c[4]) ? 1 : 0, w.size, w.ratio, w.trend); return b; }
static std::string key(const rly::Wave& w) { char b[160]; std::snprintf(b, sizeof b, "%d|%d|%d|%d|%d|%.4f|%.4f|%d", w.gp, w.ge, w.gc, w.dir, w.fired ? 1 : 0, w.size, w.ratio, w.trend); return b; }
int main(int argc, char** argv) {
    const auto N = load<rly::Bar>(argc > 1 ? argv[1] : "es_2026-11-08_on_3m.txt");
    CHECK(N.size() > 200, "tonight's bars");
    const auto W = rly::run(N, 180, 510, 900, 0, 1, true);
    int hit = 0;
    for (const int want : { 19 * 60 + 6, 19 * 60 + 54, 20 * 60 + 39 }) {
        bool ok = false;
        for (const auto& w : W) { const auto& e = N[(size_t)w.ge]; if (w.on && w.fired && w.dir > 0 && e.days == 20765 && std::abs(e.endSec / 60 - want) <= 6) ok = true; }
        hit += ok; CHECK(ok, "Rly on the up wave topping ~" + std::to_string(want / 60) + ":" + std::to_string(want % 60));
    }
    // [2] + [3] vs 1.2.0 on two months of ES bars
    if (argc > 2) {
        const auto A = load<rly::Bar>(argv[2]); std::vector<old120::rly120::Bar> A0; for (const auto& b : A) { old120::rly120::Bar c; c.days = b.days; c.endSec = b.endSec; c.o = b.o; c.h = b.h; c.l = b.l; c.c = b.c; c.v = b.v; A0.push_back(c); }
        const auto n1 = rly::run(A, 180, 510, 900, 0, 1, true); const auto o1 = old120::rly120::run(A0, 180, 510, 900, 0, 1, true);
        std::map<std::string, int> nk, ok; int rthN = 0, rthO = 0, onLate = 0, onLateKept = 0;
        for (const auto& w : n1) if (!w.on) { nk[key5(w)]++; rthN++; }
        for (const auto& w : o1) if (!w.on) { char b[160]; std::snprintf(b, sizeof b, "%d|%d|%d|%d|%d|%.4f|%.4f|%d", w.gp, w.ge, w.gc, w.dir, w.fired ? 1 : 0, w.size, w.ratio, w.trend); ok[b]++; rthO++; }
        CHECK(rthN == rthO && nk == ok, "RTH waves identical to 1.2.0 (" + std::to_string(rthN) + " vs " + std::to_string(rthO) + ")");
        std::map<std::string, int> on1; for (const auto& w : n1) if (w.on) on1[key5(w)]++;
        for (const auto& w : o1) if (w.on) { onLate++; char b[160]; std::snprintf(b, sizeof b, "%d|%d|%d|%d|%d|%.4f|%.4f|%d", w.gp, w.ge, w.gc, w.dir, w.fired ? 1 : 0, w.size, w.ratio, w.trend); if (on1.count(b)) onLateKept++; }
        CHECK(onLate > 0 && onLateKept == onLate, "every overnight wave 1.2.0 judged (02:00 CT on) is judged the same (" + std::to_string(onLateKept) + " / " + std::to_string(onLate) + ")");
    }
    // [4] no repaint
    {   std::map<int, std::string> seen; bool stable = true; std::vector<rly::Bar> P;
        for (size_t i = 0; i < N.size(); ++i) { P.push_back(N[i]); for (const auto& w : rly::run(P, 180, 510, 900, 0, 1, true)) { auto it = seen.find(w.ge * 4 + (w.dir > 0)); if (it == seen.end()) seen[w.ge * 4 + (w.dir > 0)] = key(w); else if (it->second != key(w)) stable = false; } }
        std::map<int, std::string> fin; for (const auto& w : W) fin[w.ge * 4 + (w.dir > 0)] = key(w);
        bool same = true; for (auto& kv : fin) if (seen[kv.first] != kv.second) same = false;
        CHECK(stable && same, "no repaint: a decided wave never changes; the last pass = the straight pass"); }
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
