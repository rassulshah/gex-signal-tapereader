// test_bo100_parity.cpp -- (1.2.0: the lsBigBidAsk record parity [2] / [3] runs the engine with zzRev = 1 = lsBigBidAsk's own 1-bar path;
//   lsBreakout itself now runs 3-bar - see test_bo120_week.cpp)
// test_bo100_parity.cpp -- lsBreakout 1.0.0 (2026-11-03) PARITY with lsBigBidAsk, SDK-free (BreakoutLogic.h).
//   [1] the decision function: bba::boFromCandles (the code lsBigBidAsk 1.32.0's boPass and lsBreakout both call) == the direct
//       boWaves inputs of tests_bo130 (the 1.31.x host recipe) on ES Oct 26 / 27 and Oct 28 - 30 - every decision equal.
//   [2] REAL parity, ES Oct 30 (ESZ6, 3-min): the 3-min candles are built from data\irt\2026-10-28..30\ES\fp_bars.csv (1-min) and
//       replayed bar by bar through bko::Engine as IRT feeds the plugin (live window, as lsBigBidAsk); the fired BO / BD (t0, side)
//       and Rly (t_e, side) are compared with lsBigBidAsk 1.31.3's records lsFlexLevels\BigBidAsk\ES-ESZ6-180-breakouts-20756.csv and
//       lsFlexLevels\Rly\ES-ESZ6-180-20756.csv (expected_bo.txt / expected_rly.txt: naive CT seconds = t - 2082844800).
//   [3] no repaint: a decision once made never changes or disappears over the replay; a restart at the end (one full pass) keeps
//       every fired letter of the replay.
// Run: g++ -std=c++17 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo100_parity.cpp -o t && ./t
#include "BreakoutLogic.h"
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("PASS %s\n", std::string(m).c_str()); } else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;   // IRT stamp (seconds since 1904) - naive CT unix seconds
static std::string clk(long long u) { char b[32]; const long long s = ((u % 86400) + 86400) % 86400; std::snprintf(b, sizeof b, "%02lld:%02lld", s / 3600, s / 60 % 60); return b; }
static std::string day(long long u) { const long d = (long)(u / 86400); int y; unsigned m, dd; { long z = d + 719468; const long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097); const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; y = (int)yoe + (int)era * 400; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153; dd = doy - (153 * mp + 2) / 5 + 1; m = mp < 10 ? mp + 3 : mp - 9; y += m <= 2; }
    char b[48]; std::snprintf(b, sizeof b, "%04d-%02u-%02u", y, m, dd); return b; }
static std::vector<bko::In> load(const char* f) {   // "YYYY-MM-DD HH:MM:SS|o|h|l|c|v" (bar END, naive CT)
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
// the 1.31.x host recipe (tests_bo130 run()): every input built inline, then boWaves
static bba::BoRun direct(const std::vector<bko::In>& B, int from, int end, int emitFrom, int fill0) {
    const double tick = 0.25; const int n = (int)B.size();
    std::vector<double> op(n), hi(n), lo(n), cl(n); std::vector<char> rth(n, 1); std::vector<long long> dayK(n, 0); std::vector<svl::Bar> sb(n);
    for (int i = 0; i < n; ++i) { op[i] = B[i].o; hi[i] = B[i].h; lo[i] = B[i].l; cl[i] = B[i].c; dayK[i] = B[i].days + (B[i].endSec > 17 * 3600 ? 1 : 0);
        sb[i] = svl::Bar{ B[i].days, B[i].endSec, (float)B[i].h, (float)B[i].l, (float)B[i].c, (double)B[i].vol }; }
    std::vector<signed char> up, dn, zd; std::vector<double> upPx, dnPx;
    bba::swingBreaksPx(op, hi, lo, cl, rth, dayK, n, tick, up, dn, upPx, dnPx); bba::zigDir(hi, lo, n, zd);
    std::vector<bba::BoIn> in(n); std::vector<bba::BoBar> bars(n); std::vector<char> skip(n, 0); std::vector<signed char> tr(n, 0); std::vector<long long> sess(n, 0);
    for (int k = fill0; k < end; ++k) { sess[k] = dayK[k]; if (k >= from && (up[k] || dn[k])) tr[k] = (signed char)vtl::level(vtl::fromChart(sb, k, 180, 510, 900));
        bba::BoBar& b = bars[k]; b.ok = true; b.o = std::llround(op[k] / tick); b.c = std::llround(cl[k] / tick); b.h = std::llround(hi[k] / tick); b.l = std::llround(lo[k] / tick);
        bba::BoIn& x = in[k]; x.known = true; x.hk = b.h; x.lk = b.l; x.vol = B[k].vol; }
    return bba::boWaves(in, bars, zd, up, dn, upPx, dnPx, skip, tr, sess, tick, from, end, emitFrom);
}
static bool same(const bba::BoRun& a, const bba::BoRun& b) {
    if (a.ev.size() != b.ev.size()) return false;
    for (size_t k = 0; k < a.ev.size(); ++k) { const bba::BoDec& x = a.ev[k]; const bba::BoDec& y = b.ev[k];
        if (x.i0 != y.i0 || x.im != y.im || x.dir != y.dir || x.fired != y.fired || x.reason != y.reason || x.piv != y.piv || x.trend != y.trend || x.ext30 != y.ext30) return false; }
    return true;
}
static void parityDecision(const char* f) {
    const std::vector<bko::In> B = load(f); const int n = (int)B.size();
    if (n < 50) { CHECK(false, std::string("bars loaded ") + f); return; }
    std::vector<bba::BoCandle> C(n); for (int k = 0; k < n; ++k) { C[k].ok = true; C[k].days = B[k].days; C[k].endSec = B[k].endSec; C[k].o = B[k].o; C[k].h = B[k].h; C[k].l = B[k].l; C[k].c = B[k].c; C[k].vol = B[k].vol; }
    std::vector<svl::Bar> sb(n); for (int i = 0; i < n; ++i) sb[i] = svl::Bar{ B[i].days, B[i].endSec, (float)B[i].h, (float)B[i].l, (float)B[i].c, (double)B[i].vol };
    auto tr = [&](int k) { return vtl::level(vtl::fromChart(sb, k, 180, 510, 900)); };
    int ok = 0, tot = 0;
    for (int cut = 40; cut <= n; cut += 9) {   // every 9th prefix: the full run and lsBigBidAsk's live window
        for (int mode = 0; mode < 2; ++mode) {
            int from = 1, emitFrom = 1, fill0 = 0; if (mode) bba::boWindow(cut, from, emitFrom, fill0);
            std::vector<bba::BoCandle> P(C.begin(), C.begin() + cut);
            const bba::BoRun a = bba::boFromCandles(P, 0.25, from, cut - 1, emitFrom, fill0, tr, [](int) { return true; }, [](int, bba::BoIn&) {});
            const bba::BoRun b = direct(std::vector<bko::In>(B.begin(), B.begin() + cut), from, cut - 1, emitFrom, fill0);
            ++tot; if (same(a, b)) ++ok;
        }
    }
    CHECK(ok == tot, std::string("[1] boFromCandles == the 1.31.x inline inputs + boWaves, ") + f + " (" + std::to_string(ok) + " / " + std::to_string(tot) + " runs identical)");
}
struct Exp { long long t0, t; int dir; double piv; };
int main() {
    parityDecision("../tests_bo130/es_2026-10-26_3m.txt");
    parityDecision("../tests_bo130/es_2026-10-27_3m.txt");
    parityDecision("es_2026-10-28_30_3m.txt");
    const std::vector<bko::In> B = load("es_2026-10-28_30_3m.txt");
    CHECK(B.size() > 1000, "[2] ES Oct 28 - 30 3-min candles from fp_bars (" + std::to_string(B.size()) + ")");
    // the expected decisions (unique t0 / side; the record repeats lines - see NOTES-BO100)
    std::map<std::pair<long long, int>, Exp> exBo; { std::ifstream in("expected_bo.txt"); std::string l; while (std::getline(in, l)) { Exp e; char v[32];
        if (std::sscanf(l.c_str(), "%lld|%lld|%d|%lf|%31s", &e.t0, &e.t, &e.dir, &e.piv, v) == 5 && !exBo.count({ e.t0, e.dir })) exBo[{ e.t0, e.dir }] = e; } }
    std::set<std::pair<long long, int>> exRly; { std::ifstream in("expected_rly.txt"); std::string l; while (std::getline(in, l)) { long long t; int d; if (std::sscanf(l.c_str(), "%lld|%d", &t, &d) == 2) exRly.insert({ t, d }); } }
    CHECK(exBo.size() == 20 && exRly.size() == 7, "[2] lsBigBidAsk records: " + std::to_string(exBo.size()) + " unique fired BO / BD, " + std::to_string(exRly.size()) + " fired Rly");
    // the Oct 30 session = bars ending after 17:00 CT Oct 29 through 16:00 CT Oct 30
    const long long s0 = (long long)svl::daysFromCivil(2026, 10, 29) * 86400 + 17 * 3600, s1 = (long long)svl::daysFromCivil(2026, 10, 30) * 86400 + 16 * 3600;
    auto inSess = [&](long long naive) { return naive > s0 && naive <= s1; };
    // live replay: the chart starts with history through the start of Oct 28's data; one new bar at a time (the last bar forming)
    bko::Engine E; E.zzRev = 1; std::map<long long, std::string> seen; int changed = 0;   // (1.2.0) lsBigBidAsk's 1-bar path
    for (int N = 60; N <= (int)B.size(); ++N) {
        std::vector<bko::In> P(B.begin(), B.begin() + N);
        E.update(P, N, P[0].t, 0.25, 180, 510, 900);
        for (const auto& kv : E.bo) { char b[96]; std::snprintf(b, sizeof b, "%lld|%d|%d|%d|%.4f", kv.second.t, kv.second.dir, (int)kv.second.fired, kv.second.reason, kv.second.piv);
            auto it = seen.find(kv.first); if (it == seen.end()) seen[kv.first] = b; else if (it->second != b) ++changed; }
        for (const auto& kv : seen) if (!E.bo.count(kv.first)) ++changed;
    }
    CHECK(changed == 0, "[3] no repaint over the bar-by-bar replay (" + std::to_string(seen.size()) + " decisions kept, " + std::to_string(changed) + " changed / vanished)");
    // BO / BD parity
    std::set<std::pair<long long, int>> gotBo; for (const auto& kv : E.bo) if (kv.second.fired && inSess(kv.first - OFF)) gotBo.insert({ kv.first - OFF, kv.second.dir });
    int m = 0; for (const auto& kv : exBo) { const bool hit = gotBo.count(kv.first) > 0; m += hit;
        if (!hit) { const auto it = E.bo.find(kv.first.first + OFF); std::printf("  MISSING %s %s %s (record: decided %s)%s%s\n", day(kv.first.first).c_str(), clk(kv.first.first).c_str(), kv.first.second > 0 ? "BO" : "BD", clk(kv.second.t).c_str(),
            it == E.bo.end() ? " - no decision on this candle here" : " - here: ", it == E.bo.end() ? "" : bba::boReasonName(it->second.reason)); } }
    int extra = 0; for (const auto& g : gotBo) if (!exBo.count(g)) { ++extra; std::printf("  EXTRA   %s %s %s\n", day(g.first).c_str(), clk(g.first).c_str(), g.second > 0 ? "BO" : "BD"); }
    int sameT = 0; for (const auto& kv : exBo) { const auto it = E.bo.find(kv.first.first + OFF); if (it != E.bo.end() && it->second.fired && it->second.t - OFF == kv.second.t) ++sameT; }
    std::printf("BO/BD PARITY: %d / %zu recorded fired matched, %d extra, follow-through candle equal on %d\n", m, exBo.size(), extra, sameT);
    CHECK(m == (int)exBo.size() && extra == 0, "[2] BO / BD parity ES Oct 30 (" + std::to_string(m) + " / " + std::to_string(exBo.size()) + " matched, " + std::to_string(extra) + " extra)");
    // Rly parity (RTH Oct 30)
    std::set<std::pair<long long, int>> gotR; for (const auto& kv : E.rly) if (kv.second.fired && inSess(kv.second.te - OFF)) gotR.insert({ kv.second.te - OFF, kv.second.dir });
    int mr = 0; for (const auto& e : exRly) { const bool h = gotR.count(e) > 0; mr += h; if (!h) std::printf("  MISSING Rly %s %s\n", clk(e.first).c_str(), e.second > 0 ? "up" : "down"); }
    int xr = 0; for (const auto& g : gotR) if (!exRly.count(g)) { ++xr; std::printf("  EXTRA   Rly %s %s\n", clk(g.first).c_str(), g.second > 0 ? "up" : "down"); }
    std::printf("RLY PARITY: %d / %zu recorded fired matched, %d extra\n", mr, exRly.size(), xr);
    CHECK(mr == (int)exRly.size() && xr == 0, "[2] Rly parity ES Oct 30 (" + std::to_string(mr) + " / " + std::to_string(exRly.size()) + " matched, " + std::to_string(xr) + " extra)");
    // a restart at the end: one full pass keeps every fired letter of the replay
    bko::Engine F; F.zzRev = 1; F.update(B, (long)B.size(), B[0].t, 0.25, 180, 510, 900);
    int keep = 0, lost = 0; for (const auto& kv : E.bo) if (kv.second.fired) { const auto it = F.bo.find(kv.first); (it != F.bo.end() && it->second.fired && it->second.dir == kv.second.dir) ? ++keep : ++lost; }
    int keepR = 0, lostR = 0; for (const auto& kv : E.rly) if (kv.second.fired) { const auto it = F.rly.find(kv.first); (it != F.rly.end() && it->second.fired) ? ++keepR : ++lostR; }
    int newF = 0; for (const auto& kv : F.bo) if (kv.second.fired) { const auto it = E.bo.find(kv.first); if (it == E.bo.end() || !it->second.fired) ++newF; }
    CHECK(lost == 0 && lostR == 0 && newF == 0, "[3] restart (one full pass) == the live replay: BO / BD " + std::to_string(keep) + " kept / " + std::to_string(lost) + " lost / " + std::to_string(newF) + " new, Rly " + std::to_string(keepR) + " kept / " + std::to_string(lostR) + " lost");
    std::printf("RESULT %d passed, %d failed\n", passes, fails);
    return fails ? 1 : 0;
}
