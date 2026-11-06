// test_bo192_prov.cpp -- lsBreakout 1.9.2 provisional Pb (2026-11-06 Rassul 13:23 "the PBb should be allowed to repaint when it turns
// out it doesn't become a pivot", 14:06 "go both"). SDK-free.
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo192_prov.cpp -o t192 && ./t192
//   [1] bar-by-bar replay (ES / NQ / GC weeks): provisional Pbs appear; each one either becomes a final Pb on the same pivot or vanishes.
//   [2] a provisional Pb only exists while its BO / BD wave has no confirmed 3-bar pivot end, and its pivot is after the wave's extreme so far.
//   [3] every vanished one is NOT a final Pb of the full pass (it really did not hold), and finals never change (replay == full pass).
//   [4] at most 2 Pb (final + provisional) per BO / BD at every bar.
//   [5] a final Pb that was provisional first was shown no later than its final decision bar (shown earlier = the point of 1.9.2).
//   [6] the layout draws provisional Pbs (same letter and colour as a final one).
#include "BreakoutLogic.h"
#include <fstream>
#include <set>
#include <map>
#include <cstdio>
#include <cstring>
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
int main() {
    struct Mk { const char* f; const char* m; double tk; } MS[3] = { { "wk_ES_eve_3m.txt", "ES", 0.25 }, { "wk_NQ_3m.txt", "NQ", 0.25 }, { "wk_GC_3m.txt", "GC", 0.1 } };
    int allProv = 0, allBecame = 0, allVanished = 0; double leadSum = 0; int leadN = 0;
    for (const Mk& m : MS) {
        const std::vector<bko::In> B = load(m.f); int om = 0, cm = 0; svl::rthOf(m.m, om, cm);
        std::map<long long, size_t> at; for (size_t k = 0; k < B.size(); ++k) at[B[k].t] = k;
        bko::Engine F; F.update(B, (long)B.size(), B[0].t, m.tk, 180, om, cm);
        bko::Engine R; std::map<long long, long long> firstProv;  // pivot t -> closed bar time when first shown provisionally
        std::set<long long> everProv; int over = 0, badState = 0;
        for (int n = 60; n <= (int)B.size(); ++n) {
            std::vector<bko::In> Q(B.begin(), B.begin() + n); R.update(Q, n, Q[0].t, m.tk, 180, om, cm);
            const long long lc = Q[(size_t)n - 2].t;
            std::map<long long, int> per; for (const auto& kv : R.pb) ++per[kv.second.t0];
            for (const bko::PbMark& p : R.pbProv) { ++per[p.t0]; everProv.insert(p.tp); firstProv.emplace(p.tp, lc);
                // [2] no confirmed wave end at / after the BO candle of the wanted side, and the pivot is after the extreme so far
                const int want = p.dir > 0 ? -1 : 1; bool conf = false; for (const bko::Pivot& w : R.wav) if (w.t >= p.t0 && w.type == -want) { conf = true; break; }
                double ext = p.dir > 0 ? -1e300 : 1e300; long long extT = 0;
                for (size_t k = at[p.t0]; k + 1 < Q.size(); ++k) { const bko::In& b = Q[k]; if (p.dir > 0 ? b.h >= ext : b.l <= ext) { ext = p.dir > 0 ? b.h : b.l; extT = b.t; } }
                bool legExt = false; for (const bko::Pivot& z : R.zzPb) if (z.prov && z.t == p.tp) legExt = true;   // (1.9.3) the forming ZigZag leg's extreme
                bool onZz = legExt; for (const bko::Pivot& z : R.wav) if (z.t == p.tp && z.type == want) onZz = true;    // (1.9.3) every Pb on a ZigZag pivot
                if (!onZz || ((conf || !(p.tp > extT)) && !legExt) || R.pb.count(p.tp)) ++badState; }
            for (const auto& kv : per) over += kv.second > bko::PB_MAX_PER_BO;
        }
        int became = 0, vanished = 0, vanishedButFinal = 0;
        for (long long t : everProv) { if (R.pb.count(t)) { ++became; const bko::PbMark& P = R.pb.at(t);
                const long long shown = firstProv[t]; if (shown <= P.td) { leadSum += (double)(at[P.td] - at[shown]); ++leadN; } else ++badState; }
            else { bool still = false; for (const bko::PbMark& p : R.pbProv) if (p.tp == t) still = true; if (!still) { ++vanished; if (F.pb.count(t)) ++vanishedButFinal; } } }
        bool same = R.pb.size() == F.pb.size(); for (const auto& kv : F.pb) { const auto it = R.pb.find(kv.first); same = same && it != R.pb.end() && it->second.t0 == kv.second.t0 && it->second.td == kv.second.td; }
        { int off = 0; for (const auto& kv : R.pb) { const int want = kv.second.dir > 0 ? -1 : 1; bool on = false; for (const bko::Pivot& z : R.wav) if (z.t == kv.first && z.type == want) on = true; off += !on; }
          CHECK(off == 0, std::string("[6] ") + m.m + ": every final Pb sits on a 3-bar High / Low ZigZag pivot (" + std::to_string(off) + " off)"); }
        CHECK(!everProv.empty(), std::string("[1] ") + m.m + ": provisional Pbs appear (" + std::to_string(everProv.size()) + "): " + std::to_string(became) + " became final, " + std::to_string(vanished) + " vanished");
        CHECK(badState == 0, std::string("[2][5] ") + m.m + ": provisional only while the wave end is unconfirmed, pivot after the extreme so far, shown no later than its final decision (" + std::to_string(badState) + " bad)");
        CHECK(vanishedButFinal == 0 && same, std::string("[3] ") + m.m + ": vanished ones are not final Pbs and the finals equal the full pass (" + std::to_string(R.pb.size()) + " vs " + std::to_string(F.pb.size()) + ")");
        CHECK(over == 0, std::string("[4] ") + m.m + ": at most 2 Pb per BO / BD at every bar, provisional included");
        allProv += (int)everProv.size(); allBecame += became; allVanished += vanished;
    }
    // [6] layout draws a provisional Pb
    {   const std::vector<bko::In> B = load("wk_ES_eve_3m.txt"); int om = 0, cm = 0; svl::rthOf("ES", om, cm);
        bko::Engine R; int drawn = 0, tried = 0;
        for (int n = 60; n <= (int)B.size() && tried < 30; ++n) { std::vector<bko::In> Q(B.begin(), B.begin() + n); R.update(Q, n, Q[0].t, 0.25, 180, om, cm);
            if (R.pbProv.empty()) continue;
            ++tried;
            bko::View v; v.first = std::max(0, n - 200); v.last = n - 1; v.area = { 0, 0, 100000, 100000 };
            double lo = 1e300, hi = -1e300; for (int k = v.first; k <= v.last; ++k) { lo = std::min(lo, Q[(size_t)k].l); hi = std::max(hi, Q[(size_t)k].h); }
            v.X = [&](int b) { return 50 + (b - v.first) * 40; }; v.Y = [&](double p) { return 20 + (int)((hi - p) / (hi - lo + 1e-9) * 90000); };
            v.textW = [](const char* s, int) { return 7 * (int)std::strlen(s); };
            const bko::Scene S = bko::layout(R, v);
            std::map<long long, int> idx; for (int k = 0; k < n; ++k) idx[Q[(size_t)k].t] = k;
            for (const bko::PbMark& p : R.pbProv) for (const bko::Txt& t : S.txt) if (t.kind == 2 && t.bar == idx[p.tp]) { ++drawn; break; } }
        CHECK(tried > 0 && drawn > 0, "[6] the layout draws provisional Pbs (" + std::to_string(drawn) + " drawn in " + std::to_string(tried) + " frames)");
    }
    std::printf("provisional %d: became final %d, vanished %d; shown on average %.1f bars before the final decision\n", allProv, allBecame, allVanished, leadN ? leadSum / leadN : 0.0);
    CHECK(std::string(bko::VERSION) == "1.9.3", "version 1.9.3");
    std::printf("%d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
