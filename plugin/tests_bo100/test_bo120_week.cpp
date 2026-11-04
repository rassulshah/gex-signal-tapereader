// test_bo120_week.cpp -- lsBreakout 1.3.0 (2026-11-03 evening: + the follow-through tie, [2] expectations updated, [4] / [5] new);
//   1.2.0 (2026-11-03): BO / BD + Rly on the 3-bar High / Low ZigZag (BreakoutLogic.h ZZ_REV_BARS).
//   [4] (1.3.0) ftTie = false reproduces the 1.2.0 expectations exactly (expected_wk3_*_strict.txt = the 1.2.0 files), so the ONLY
//       change is the tie rule; every changed line is the same breakout candle, either no_confirm -> fired or fired with an earlier
//       (tie) follow-through candle; nothing lost.
//   [5] (1.3.0) ES Oct 20 - Nov 3 19:45 (wk_ES_eve_3m.txt): one full pass == the independent tie harness (bo_eve harness2 + hdrt,
//       expected_eve_ES_full.txt); fired BO / BD strict 142 -> tie 144, the two added = Oct 23 01:48 BO and Nov 3 19:06 BD (fired at
//       the 19:21 close).
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
// (1.3.0) the tie vs the strict rule on the same bars: every difference is the same t0, strict no_confirm -> tie fired, or both fired with
// the tie's follow-through candle no later; nothing else changes (Rly / Pb lines may only gain Pb after an added BO / BD)
static std::map<long long, bba::BoDec> fired(const bko::Engine& E) { std::map<long long, bba::BoDec> m; for (const auto& kv : E.bo) if (kv.second.fired) m[kv.first] = kv.second; return m; }
static void tieVsStrict(const char* m, const std::string& file, double tk, int om, int cm, const std::string& strictExp, std::vector<long long>* added) {
    const std::vector<bko::In> B = load(file);
    bko::Engine T, S; S.ftTie = false;
    T.update(B, (long)B.size(), B[0].t, tk, 180, om, cm); S.update(B, (long)B.size(), B[0].t, tk, 180, om, cm);
    CHECK(T.tieOn() && !S.tieOn(), std::string("[4] ") + m + " ftTie on by default (3-bar), off when set false");
    if (!strictExp.empty()) { const int d = diffCount(lines(S), readExp(strictExp)); CHECK(d == 0, std::string("[4] ") + m + " ftTie = false == the 1.2.0 expectations (" + std::to_string(d) + " differ)"); }
    int odd = 0, earlier = 0; std::vector<long long> add;
    for (const auto& kv : S.bo) { const auto it = T.bo.find(kv.first); if (it == T.bo.end()) { ++odd; continue; }
        const bba::BoDec& a = kv.second; const bba::BoDec& b = it->second;
        if (a.fired == b.fired && a.t == b.t && a.reason == b.reason && a.dir == b.dir) continue;
        if (a.dir != b.dir || a.piv != b.piv || !b.fired) { ++odd; continue; }
        if (!a.fired && a.reason == bba::BO_R_NO_CONFIRM && b.t < a.t) add.push_back(kv.first);
        else if (a.fired && b.t < a.t) ++earlier; else ++odd; }
    for (const auto& kv : T.bo) if (!S.bo.count(kv.first)) ++odd;
    for (const auto& kv : S.rly) { const auto it = T.rly.find(kv.first); if (it == T.rly.end() || it->second.fired != kv.second.fired) ++odd; }
    for (const auto& kv : S.pb) if (!T.pb.count(kv.first)) ++odd;
    CHECK(odd == 0, std::string("[4] ") + m + " tie vs strict: only no_confirm -> fired (" + std::to_string(add.size()) + ") or an earlier follow-through (" + std::to_string(earlier) + "); fired " + std::to_string(fired(S).size()) + " -> " + std::to_string(fired(T).size()) + ", " + std::to_string(odd) + " other changes");
    if (added) *added = add;
}
int main() {
    {   std::vector<long long> add; tieVsStrict("ES", "wk_ES_3m.txt", 0.25, 510, 900, "expected_wk3_ES_full_strict.txt", &add);
        CHECK(add.size() == 1 && add[0] - OFF == 20749LL * 86400 + 1 * 3600 + 48 * 60, "[4] ES week: the one added BO is Oct 23 01:48");
        tieVsStrict("NQ", "wk_NQ_3m.txt", 0.25, 510, 900, "expected_wk3_NQ_full_strict.txt", &add); CHECK(add.empty(), "[4] NQ week: none added (one earlier follow-through)");
        tieVsStrict("GC", "wk_GC_3m.txt", 0.1, 440, 750, "expected_wk3_GC_full_strict.txt", &add); CHECK(add.empty(), "[4] GC week: none added (two earlier follow-throughs)"); }
    {   const std::vector<bko::In> B = load("wk_ES_eve_3m.txt"); bko::Engine T, S; S.ftTie = false;
        T.update(B, (long)B.size(), B[0].t, 0.25, 180, 510, 900); S.update(B, (long)B.size(), B[0].t, 0.25, 180, 510, 900);
        const int d = diffCount(lines(T), readExp("expected_eve_ES_full.txt"));
        CHECK(d == 0, "[5] ES Oct 20 - Nov 3 19:45 full pass == the independent tie harness (" + std::to_string(lines(T).size()) + " lines, " + std::to_string(d) + " differ)");
        const auto fs = fired(S), ft = fired(T); std::vector<long long> add; for (const auto& kv : ft) if (!fs.count(kv.first)) add.push_back(kv.first);
        const long long bd = 20760LL * 86400 + 19 * 3600 + 6 * 60, bdFire = 20760LL * 86400 + 19 * 3600 + 21 * 60;
        CHECK(fs.size() == 142 && ft.size() == 144 && add.size() == 2 && add[0] - OFF == 20749LL * 86400 + 6480 && add[1] - OFF == bd
              && ft.at(add[1]).dir == -1 && ft.at(add[1]).t - OFF == bdFire && ft.at(add[1]).piv == 7553.25,
              "[5] ES fired 142 -> 144: + Oct 23 01:48 BO, + Nov 3 19:06 BD (pivot 7553.25, shown at the 19:21 close)"); }
    market("ES", 0.25, 510, 900);
    market("NQ", 0.25, 510, 900);
    market("GC", 0.1, 440, 750);
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
