// test_bo180_pb.cpp -- lsBreakout 1.8.0 Pb (2026-11-05 Rassul 14:08 "a pb needs to overlap the bo", "limit the pb to be fired upto two times",
// 16:47 "it should become a pivot low based on a bar close", 16:46 "go"). SDK-free.
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo180_pb.cpp -o t180 && ./t180
//   [1] the Pb pivots = an independent walk (16:48 "a green bar occurs, thats good enough"): a lower low moves the low, the first later
//       GREEN close confirms the pivot LOW (mirror: RED close -> pivot HIGH) on ES / NQ / GC weeks.
//   [2] every Pb: its pivot is of the wanted type (BO -> LOW, BD -> HIGH), after the BO candle, and its bar overlaps the BO / BD candle.
//   [3] at most 2 Pb per BO / BD, and they are its FIRST qualifying pivots (no earlier overlapping pivot of the same BO skipped).
//   [4] every Pb's confirm bar is a later GREEN close (BD: RED), from the bars.
//   [5] no repaint: a bar-by-bar replay keeps every Pb of the full pass, never more than 2 per BO.
//   [7] (1.9.1, Rassul 12:35 "the pullback is supposed to be on the dnwave after the rally upwave, it cannot be in the same wave as the bo
//       wave"): every Pb pivot is AFTER the BO / BD wave's end = the first 3-bar High / Low ZigZag pivot HIGH (BD: LOW) at / after the candle
//       (independent zzd walk), and its decision is not before that pivot's confirmation.
//   [6] the 1-bar build's rule is the same with N = 1 (checked in test_bo161); versions 1.8.0.
#include "BreakoutLogic.h"
#include <fstream>
#include <set>
#include <map>
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
struct P { long long t, tc; int type; };
// independent walk: a lower low moves the low, the first later GREEN close confirms the pivot LOW (mirror: higher high / RED close -> pivot HIGH)
static std::vector<P> walk(const std::vector<bko::In>& B) {
    std::vector<P> out; const int n = (int)B.size() - 1; if (n < 2) return out;
    int dirn = B[1].h >= B[0].h ? 1 : -1, ext = 0;
    for (int j = 1; j < n; ++j) {
        const bko::In& x = B[(size_t)j];
        if (dirn == 1) { if (x.h > B[(size_t)ext].h + 1e-9) { ext = j; continue; } if (x.c < x.o) { out.push_back({ B[(size_t)ext].t, x.t, 1 }); ext = j; dirn = -1; } }
        else { if (x.l < B[(size_t)ext].l - 1e-9) { ext = j; continue; } if (x.c > x.o) { out.push_back({ B[(size_t)ext].t, x.t, -1 }); ext = j; dirn = 1; } }
    }
    return out;
}
struct WP { long long t, tc; int type; };
static std::vector<WP> waves(const std::vector<bko::In>& B, double tk) {
    std::vector<zzd::Bar> Z; std::vector<int> at; for (int k = 0; k + 1 < (int)B.size(); ++k) { zzd::Bar z; z.t = B[(size_t)k].t; if (!zzd::toTicks(B[(size_t)k].h, tk, z.h) || !zzd::toTicks(B[(size_t)k].l, tk, z.l)) continue; Z.push_back(z); at.push_back(k); }
    std::vector<WP> out; zzd::Walk w; for (int j = 0; j < (int)Z.size(); ++j) { int s0 = 0, e = 0, d = 0; if (!zzd::Engine::step(Z, w, j, s0, e, d, 3)) continue; out.push_back({ B[(size_t)at[(size_t)e]].t, B[(size_t)at[(size_t)j]].t, d > 0 ? 1 : -1 }); }
    return out;
}
int main() {
    struct Mk { const char* f; const char* m; double tk; } MS[3] = { { "wk_ES_eve_3m.txt", "ES", 0.25 }, { "wk_NQ_3m.txt", "NQ", 0.25 }, { "wk_GC_3m.txt", "GC", 0.1 } };
    for (const Mk& m : MS) {
        const std::vector<bko::In> B = load(m.f); int om = 0, cm = 0; svl::rthOf(m.m, om, cm);
        std::map<long long, size_t> at; for (size_t k = 0; k < B.size(); ++k) at[B[k].t] = k;
        bko::Engine E; E.update(B, (long)B.size(), B[0].t, m.tk, 180, om, cm);
        const std::vector<WP> WV = waves(B, m.tk);
        // [1]
        const std::vector<P> W = walk(B);
        bool same = W.size() == E.piv.size(); for (size_t k = 0; same && k < W.size(); ++k) same = W[k].t == E.piv[k].t && W[k].tc == E.piv[k].tc && W[k].type == E.piv[k].type;
        CHECK(same && !W.empty(), std::string("[1] ") + m.m + ": Pb pivots == the independent green / red close walk (" + std::to_string(W.size()) + " vs " + std::to_string(E.piv.size()) + ")");
        // [2] [4]
        int bad = 0, badClose = 0; std::map<long long, int> perBo;
        for (const auto& kv : E.pb) { const bko::PbMark& p = kv.second; const auto d = E.bo.find(p.t0); if (d == E.bo.end()) { ++bad; continue; }
            ++perBo[p.t0]; const bko::In& c = B[at[p.t0]]; const bko::In& pb = B[at[p.tp]];
            if (!(p.tp > p.t0 && pb.l < c.h && pb.h > c.l && p.dir == (d->second.dir > 0 ? 1 : -1))) ++bad;
            bool isPiv = false; for (const bko::Pivot& q : E.piv) if (q.t == p.tp && q.type == (p.dir > 0 ? -1 : 1)) isPiv = true; if (!isPiv) ++bad;
            const size_t ic = at[p.tc]; if (!(p.dir > 0 ? B[ic].c > B[ic].o : B[ic].c < B[ic].o) || !(ic > at[p.tp])) ++badClose; }
        CHECK(bad == 0 && !E.pb.empty(), std::string("[2] ") + m.m + ": every Pb is a pivot of the wanted type after its BO / BD and its bar overlaps the BO / BD candle (" + std::to_string(E.pb.size()) + " Pb, " + std::to_string(bad) + " bad)");
        CHECK(badClose == 0, std::string("[4] ") + m.m + ": every Pb confirmed by a later GREEN close (BD: RED close)");
        // [3]
        int over = 0, skipped = 0;
        for (const auto& kv : perBo) { if (kv.second > bko::PB_MAX_PER_BO) ++over;
            const bko::In& c = B[at[kv.first]]; const int want = E.bo[kv.first].dir > 0 ? -1 : 1; long long lastPb = 0; for (const auto& q : E.pb) if (q.second.t0 == kv.first) lastPb = std::max(lastPb, q.second.tp);
            long long top = 0; for (const WP& w : WV) if (w.t >= kv.first && w.type == -want) { top = w.t; break; }
            for (const bko::Pivot& q : E.piv) if (q.type == want && q.t > kv.first && q.t > top && q.t < lastPb && q.l < c.h && q.h > c.l && !E.pb.count(q.t)) ++skipped; }
        CHECK(over == 0 && skipped == 0, std::string("[3] ") + m.m + ": at most 2 Pb per BO / BD, the first qualifying pivots (" + std::to_string(over) + " over, " + std::to_string(skipped) + " skipped)");
        int two = 0; for (const auto& kv : perBo) two += kv.second == 2;
        // [5]
        bko::Engine R; std::set<long long> seen;
        for (int n = 60; n <= (int)B.size(); ++n) { std::vector<bko::In> Q(B.begin(), B.begin() + n); R.update(Q, n, Q[0].t, m.tk, 180, om, cm); for (const auto& kv : R.pb) seen.insert(kv.first); }
        std::set<long long> full; for (const auto& kv : E.pb) full.insert(kv.first);
        std::map<long long, int> rper; for (const auto& kv : R.pb) ++rper[kv.second.t0]; int rover = 0; for (const auto& kv : rper) rover += kv.second > 2;
        CHECK(seen == full && rover == 0, std::string("[5] ") + m.m + ": bar-by-bar replay == one full pass (" + std::to_string(seen.size()) + " vs " + std::to_string(full.size()) + " Pb)");
        {   int inWave = 0; for (const auto& kv : E.pb) { const bko::PbMark& p = kv.second; const int want = p.dir > 0 ? -1 : 1; long long top = 0, topC = 0;
                for (const WP& w : WV) if (w.t >= p.t0 && w.type == -want) { top = w.t; topC = w.tc; break; }
                if (!top || p.tp <= top || p.td < topC) ++inWave; }
            CHECK(inWave == 0, std::string("[7] ") + m.m + ": every Pb is on the wave AFTER the BO / BD wave (" + std::to_string(inWave) + " inside it)"); }
        std::printf("  %s: %zu Pb on %zu BO / BD (%d with 2), %zu green / red close pivots\n", m.m, E.pb.size(), perBo.size(), two, W.size());
    }
    CHECK(std::string(bko::VERSION) == "1.9.1" && bko::PB_MAX_PER_BO == 2, "[6] version 1.9.1, PB_MAX_PER_BO 2");
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
