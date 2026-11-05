// test_bo160_pb.cpp -- lsBreakout 1.6.0 (2026-11-04 19:06-19:22 CT, Rassul "lets remove the BI signal and require the PB which tests the
// BO to pullback to the pivot with less than 1/2 atr remaining", "make it 1/3", "go"). SDK-free (BreakoutLogic.h).
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo160_pb.cpp -o t160 && ./t160
//   [1] PARITY: one full pass == the independent Python model analytics/lra/bo_pb.py 1.3.0 (pb_marks rule "atr", bo_decisions with the
//       ES session hours) on ES Oct 20 - Nov 3 (181 Pb), NQ and GC weeks (expected_pb160_*.txt) - every (pivot bar, BO candle, side) equal.
//   [2] THE RULE: every Pb's pivot came back to the broken pivot or stopped < ATR / 3 short of it (ATR = 14-bar Wilder at the BO candle);
//       and on ES at least one Pb stopped short (within the band) and at least one traded through the level.
//   [3] BI REMOVED: no BI decided, no BO / BD cancelled, no BI letter in the layout; the fired BO / BD equal the biCancel = false engine.
//   [4] NO REPAINT: a bar-by-bar replay of ES gives the same Pb set as one full pass (no Pb lost, moved or added).
//   [5] versions 1.6.0 (bko::VERSION, Breakout.cpp BKO_VERSION and setVersion).
#include "BreakoutLogic.h"
#include <fstream>
#include <sstream>
#include <set>
#include <cstdio>
static int fails = 0, passes = 0;
#define CHECK(c, m) do { if (c) ++passes; else { ++fails; std::printf("FAIL %s\n", std::string(m).c_str()); } } while (0)
static const long long OFF = 2082844800LL;
static std::vector<bko::In> load(const std::string& f) {
    std::ifstream in(f); std::string l; std::vector<bko::In> v;
    while (std::getline(in, l)) { int y, mo, d, h, mi, s; double o, hi, lo, c, vo;
        if (std::sscanf(l.c_str(), "%d-%d-%d %d:%d:%d|%lf|%lf|%lf|%lf|%lf", &y, &mo, &d, &h, &mi, &s, &o, &hi, &lo, &c, &vo) != 11) continue;
        bko::In b; b.days = svl::daysFromCivil(y, mo, d); b.endSec = h * 3600 + mi * 60 + s; b.t = (long long)b.days * 86400 + b.endSec + OFF;
        b.ok = true; b.o = o; b.h = hi; b.l = lo; b.c = c; b.vol = (long long)vo; v.push_back(b); }
    return v;
}
static std::set<std::string> pbSet(const bko::Engine& E) { std::set<std::string> s; char b[96];
    for (const auto& kv : E.pb) { std::snprintf(b, sizeof b, "PB %lld %lld %d", kv.second.tp - OFF, kv.second.t0 - OFF, kv.second.dir); s.insert(b); } return s; }
static std::set<std::string> readSet(const std::string& f) { std::ifstream in(f); std::string l; std::set<std::string> s; while (std::getline(in, l)) if (!l.empty()) s.insert(l); return s; }
int main() {
    struct M { const char* f; const char* exp; double tk; } MS[3] = { { "wk_ES_eve_3m.txt", "expected_pb160_ES_eve.txt", 0.25 }, { "wk_NQ_3m.txt", "expected_pb160_NQ.txt", 0.25 }, { "wk_GC_3m.txt", "expected_pb160_GC.txt", 0.1 } };
    for (const M& m : MS) {
        const std::vector<bko::In> B = load(m.f);
        bko::Engine E; E.update(B, (long)B.size(), B[0].t, m.tk, 180, 510, 900);
        const std::set<std::string> got = pbSet(E), want = readSet(m.exp);
        int d = 0; for (const auto& x : got) if (!want.count(x)) { if (d < 3) std::printf("  extra %s\n", x.c_str()); ++d; } for (const auto& x : want) if (!got.count(x)) { if (d < 6) std::printf("  missing %s\n", x.c_str()); ++d; }
        CHECK(d == 0 && !got.empty(), std::string("[1] ") + m.f + ": engine Pb == bo_pb 1.3.0 (" + std::to_string(got.size()) + " vs " + std::to_string(want.size()) + ", " + std::to_string(d) + " differ)");
        // [2] the rule, recomputed here from the bars
        std::vector<double> atr(B.size(), NAN); { double y = 0; bool have = false; for (size_t k = 1; k < B.size(); ++k) { const double tr = std::max(B[k].h - B[k].l, std::max(std::fabs(B[k].h - B[k - 1].c), std::fabs(B[k].l - B[k - 1].c))); y = have ? y + (tr - y) / 14.0 : tr; have = true; atr[k] = y; } }
        int bad = 0, shortOf = 0, through = 0;
        for (const auto& kv : E.pb) { const bko::PbMark& P = kv.second; const auto it = E.bo.find(P.t0); if (it == E.bo.end()) { ++bad; continue; }
            size_t i0 = 0, ip = 0; for (size_t k = 0; k < B.size(); ++k) { if (B[k].t == P.t0) i0 = k; if (B[k].t == P.tp) ip = k; }
            const double piv = it->second.piv, A = atr[i0], ext = P.dir > 0 ? B[ip].l : B[ip].h, gap = P.dir > 0 ? ext - piv : piv - ext;
            if (!(gap <= A / 3.0 + 1e-9)) { ++bad; }
            if (gap > 0) { ++shortOf; } else { ++through; } }
        CHECK(bad == 0, std::string("[2] ") + m.f + ": every Pb pivot is at / through the broken pivot or < ATR / 3 short (" + std::to_string(bad) + " bad)");
        if (std::string(m.f).find("ES") != std::string::npos) CHECK(shortOf > 0 && through > 0, "[2] ES has both kinds: stopped short within the band (" + std::to_string(shortOf) + ") and traded through the level (" + std::to_string(through) + ")");
        // [3] BI removed
        bko::Engine N; N.biCancel = false; N.update(B, (long)B.size(), B[0].t, m.tk, 180, 510, 900);
        int boDiff = 0; for (const auto& kv : E.bo) { const auto it = N.bo.find(kv.first); if (it == N.bo.end() || it->second.fired != kv.second.fired || it->second.t != kv.second.t) ++boDiff; }
        CHECK(E.bi.empty() && E.biL.empty() && E.boCancel.empty() && boDiff == 0 && E.bo.size() == N.bo.size(), std::string("[3] ") + m.f + ": no BI, no BBI line, no BO / BD cancelled, decisions == the no-BI engine (" + std::to_string(boDiff) + " differ)");
    }
    {   // [4] replay
        const std::vector<bko::In> B = load("wk_ES_eve_3m.txt"); bko::Engine R;
        for (int n = 60; n <= (int)B.size(); ++n) { std::vector<bko::In> P(B.begin(), B.begin() + n); R.update(P, n, P[0].t, 0.25, 180, 510, 900); }
        bko::Engine F; F.update(B, (long)B.size(), B[0].t, 0.25, 180, 510, 900);
        CHECK(pbSet(R) == pbSet(F), "[4] ES bar-by-bar replay == one full pass (" + std::to_string(pbSet(R).size()) + " Pb)");
    }
    {   // [5] versions
        std::ifstream f("../Breakout.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string s = ss.str();
        CHECK(std::string(bko::VERSION) == "1.7.0" && s.find("#define BKO_VERSION \"1.7.0\"") != std::string::npos && s.find("p->setVersion(\"1.7.0\")") != std::string::npos, "[5] versions 1.7.0");
    }
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
