// test_bo120_week.cpp -- lsBreakout 1.2.0 (2026-11-03): BO / BD + Rly on the 3-bar High / Low ZigZag (BreakoutLogic.h ZZ_REV_BARS).
//   [1] the default engine runs 3-bar (zzRev == zzd::COMBO_REV_BARS == 3) and rev = 1 is still lsBigBidAsk's code (SwingRef / zigDir /
//       rly::zigzag rev 1 == the 1-bar rule: checked bit for bit against the 1-bar walks on the week).
//   [2] PARITY with the independent week harness (bo3/wk: header copies with the turn test replaced by the zzd 3-bar rule, REVN=3) on
//       ES Oct 21 - Nov 3, NQ / GC Oct 21 - 30 (3-min from fp_bars): every decision (BO / BD fired or not with reason, pivot, mark
//       candle; Rly judged waves; Pb) of a bar-by-bar replay and of one full pass identical to the harness output.
//   [3] no repaint over the replay; a restart (one full pass) keeps every fired BO / BD / Rly / Pb of the replay.
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo120_week.cpp
#include "BreakoutLogic.h"
#include <cstdio>
#include <fstream>
#include <set>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("PASS %s\n", std::string(m).c_str()); } else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
static std::vector<bko::In> load(const std::string& f) {
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
static std::vector<std::string> lines(const bko::Engine& E) {   // the harness print format
    std::vector<std::string> v; char b[160];
    for (const auto& kv : E.bo) { const auto& D = kv.second; std::snprintf(b, sizeof b, "BO %lld %lld %d %d %s %.4f", D.t0 - OFF, D.t - OFF, D.dir, D.fired ? 1 : 0, bba::boReasonName(D.reason), D.piv); v.push_back(b); }
    for (const auto& kv : E.rly) { const auto& R = kv.second; std::snprintf(b, sizeof b, "RLY %lld %d %d", R.te - OFF, R.dir, R.fired ? 1 : 0); v.push_back(b); }
    for (const auto& kv : E.pb) { const auto& P = kv.second; std::snprintf(b, sizeof b, "PB %lld %lld %d", P.tp - OFF, P.t0 - OFF, P.dir); v.push_back(b); }
    return v;
}
static std::vector<std::string> readExp(const std::string& f) { std::ifstream in(f); std::string l; std::vector<std::string> v; while (std::getline(in, l)) if (!l.empty()) v.push_back(l); return v; }
static int diffCount(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    int d = 0; const size_t n = std::max(a.size(), b.size());
    for (size_t k = 0; k < n; ++k) { const std::string x = k < a.size() ? a[k] : "-", y = k < b.size() ? b[k] : "-"; if (x != y) { if (d < 5) std::printf("  DIFF got [%s] want [%s]\n", x.c_str(), y.c_str()); ++d; } }
    return d;
}
static void market(const char* m, double tk, int om, int cm) {
    const std::vector<bko::In> B = load(std::string("wk_") + m + "_3m.txt");
    CHECK(B.size() > 3000, std::string("[2] ") + m + " week 3-min candles (" + std::to_string(B.size()) + ")");
    bko::Engine E; std::map<long long, std::string> seen; int changed = 0; int nFiredBo = 0;
    for (int N = 60; N <= (int)B.size(); ++N) {
        std::vector<bko::In> P(B.begin(), B.begin() + N); E.update(P, N, P[0].t, tk, 180, om, cm);
        if (N % 25 == 0 || N == (int)B.size()) {
            for (const auto& kv : E.bo) { char b[96]; std::snprintf(b, sizeof b, "%lld|%d|%d|%d|%.4f", kv.second.t, kv.second.dir, (int)kv.second.fired, kv.second.reason, kv.second.piv);
                auto it = seen.find(kv.first); if (it == seen.end()) seen[kv.first] = b; else if (it->second != b) ++changed; }
            for (const auto& kv : seen) if (!E.bo.count(kv.first)) ++changed;
        }
    }
    for (const auto& kv : E.bo) nFiredBo += kv.second.fired;
    CHECK(E.zzRev == 3, std::string("[1] ") + m + " engine runs the 3-bar ZigZag");
    const int dR = diffCount(lines(E), readExp(std::string("expected_wk3_") + m + "_replay.txt"));
    CHECK(dR == 0, std::string("[2] ") + m + " replay == week harness (REVN=3): " + std::to_string(lines(E).size()) + " decisions, " + std::to_string(nFiredBo) + " fired BO / BD, " + std::to_string(dR) + " differ");
    CHECK(changed == 0, std::string("[3] ") + m + " no repaint (" + std::to_string(seen.size()) + " BO decisions, " + std::to_string(changed) + " changed / vanished)");
    bko::Engine F; F.update(B, (long)B.size(), B[0].t, tk, 180, om, cm);
    const int dF = diffCount(lines(F), readExp(std::string("expected_wk3_") + m + "_full.txt"));
    CHECK(dF == 0, std::string("[2] ") + m + " one full pass == week harness: " + std::to_string(dF) + " differ");
    int lost = 0, newF = 0;
    for (const auto& kv : E.bo) if (kv.second.fired) { const auto it = F.bo.find(kv.first); if (it == F.bo.end() || !it->second.fired || it->second.dir != kv.second.dir || it->second.t != kv.second.t) ++lost; }
    for (const auto& kv : F.bo) if (kv.second.fired) { const auto it = E.bo.find(kv.first); if (it == E.bo.end() || !it->second.fired) ++newF; }
    for (const auto& kv : E.rly) if (kv.second.fired) { const auto it = F.rly.find(kv.first); if (it == F.rly.end() || !it->second.fired) ++lost; }
    for (const auto& kv : E.pb) { const auto it = F.pb.find(kv.first); if (it == F.pb.end() || it->second.t0 != kv.second.t0) ++lost; }
    CHECK(lost == 0 && newF == 0, std::string("[3] ") + m + " restart == replay (fired BO / BD, Rly, Pb): " + std::to_string(lost) + " lost, " + std::to_string(newF) + " new");
    // [1] rev 1 of the shared walks == the 1-bar rule (bba::zigDir default vs rev 1; swg::ZigZag rev 1 vs default)
    std::vector<double> hi, lo; for (const auto& b : B) { hi.push_back(b.h); lo.push_back(b.l); }
    std::vector<signed char> a, c, d3; bba::zigDir(hi, lo, (int)B.size(), a); bba::zigDir(hi, lo, (int)B.size(), c, 1); bba::zigDir(hi, lo, (int)B.size(), d3, 3);
    CHECK(a == c && a != d3, std::string("[1] ") + m + " zigDir rev 1 == default 1-bar walk, rev 3 differs");
}
int main() {
    market("ES", 0.25, 510, 900);
    market("NQ", 0.25, 510, 900);
    market("GC", 0.1, 440, 750);
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
