// test_bo161_onebar.cpp -- lsBreakout1 "BO 1-bar" (2026-11-04 21:25 CT Rassul "there needs to be a 1bar option for breakout indicator ..
// this will help when i put it on higher timeframes", 21:31 "go"). Built with BKO_ONE_BAR = 1 (= Breakout1.cpp). SDK-free.
// Run (from plugin/tests_bo100): g++ -std=c++17 -O1 -fsanitize=address,undefined -Werror -Wall -Wextra -I.. test_bo161_onebar.cpp -o t161 && ./t161
//   [1] the build: zzRev 1, the 3-bar refinements on (follow-through tie, carry across 17:00 CT), records in Breakout1, record version -zz1.
//   [2] 15-min bars (built from the ES / NQ / GC 3-min weeks): every Pb pivot and every cached pivot equals an independent 1-bar
//       zzd::Engine::step walk (rev 1) over the same closed bars; BO / BD decided and fired on every market.
//   [3] the rule: every Pb comes back to its BO / BD pivot or stops < ATR(14, Wilder) / 3 short of it.
//   [4] no repaint: a bar-by-bar replay of each market == one full pass (BO / BD, Pb).
//   [5] the 1-bar build is the 3-bar engine with zzRev 1 + refine1 (same decisions as setting them on a default engine).
//   [6] Breakout1.cpp wraps Breakout.cpp with BKO_ONE_BAR 1 and carries the gex-build setVersion line.
#define BKO_ONE_BAR 1
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
// 3-min -> 15-min: a bar ends on a 15-min boundary (end stamps); gaps start a new bar
static std::vector<bko::In> agg15(const std::vector<bko::In>& B) {
    std::vector<bko::In> o; long long key = -1;
    for (const bko::In& b : B) {
        const long long k = (b.t - 1) / 900;   // end stamp -> 15-min slot
        if (o.empty() || k != key) { bko::In n = b; n.t = (k + 1) * 900; const long long w = n.t - OFF; n.days = (long)(w / 86400); n.endSec = (int)(w % 86400); o.push_back(n); key = k; continue; }
        bko::In& n = o.back(); n.h = std::max(n.h, b.h); n.l = std::min(n.l, b.l); n.c = b.c; n.vol += b.vol;
    }
    return o;
}
template <class M> static std::set<std::string> keys(const M& m) { std::set<std::string> s; for (const auto& kv : m) s.insert(std::to_string(kv.first)); return s; }
static std::set<std::string> boSet(const bko::Engine& E) { std::set<std::string> s; char b[96];
    for (const auto& kv : E.bo) { std::snprintf(b, sizeof b, "%lld %lld %d %d", kv.second.t0, kv.second.t, kv.second.dir, kv.second.fired ? 1 : 0); s.insert(b); } return s; }
static std::set<std::string> pbSet(const bko::Engine& E) { std::set<std::string> s; char b[96];
    for (const auto& kv : E.pb) { std::snprintf(b, sizeof b, "%lld %lld %d", kv.second.tp, kv.second.t0, kv.second.dir); s.insert(b); } return s; }
int main() {
    {   bko::Engine E;
        CHECK(E.zzRev == 1 && E.refine1 && E.tieOn() && E.carryOn() && E.recVer() == std::string(bko::VERSION) + "-zz1", "[1] zzRev 1, tie + carry on, record version " + E.recVer());
        CHECK(std::string(BKO_REC_DIR) == "Breakout1", "[1] records go to lsFlexLevels\\Breakout1"); }
    struct Mk { const char* f; double tk; } MS[3] = { { "wk_ES_eve_3m.txt", 0.25 }, { "wk_NQ_3m.txt", 0.25 }, { "wk_GC_3m.txt", 0.1 } };
    for (const Mk& m : MS) {
        const std::vector<bko::In> B = agg15(load(m.f));
        CHECK(B.size() > 200, std::string("[2] ") + m.f + ": 15-min bars " + std::to_string(B.size()));
        bko::Engine E; E.update(B, (long)B.size(), B[0].t, m.tk, 900, 510, 900);
        // [2] independent 1-bar walk over the closed bars (the forming last bar excluded, as the engine)
        std::vector<zzd::Bar> Z; std::vector<int> at;
        for (int k = 0; k + 1 < (int)B.size(); ++k) { zzd::Bar z; z.t = B[(size_t)k].t; if (!zzd::toTicks(B[(size_t)k].h, m.tk, z.h) || !zzd::toTicks(B[(size_t)k].l, m.tk, z.l)) continue; Z.push_back(z); at.push_back(k); }
        std::set<long long> ref; zzd::Walk w;
        for (int j = 0; j < (int)Z.size(); ++j) { int s0 = 0, e = 0, d = 0; if (zzd::Engine::step(Z, w, j, s0, e, d, 1)) ref.insert(B[(size_t)at[(size_t)e]].t); }
        std::set<long long> got; for (const bko::Pivot& p : E.piv) got.insert(p.t);
        CHECK(got == ref && !got.empty(), std::string("[2] ") + m.f + ": engine pivots == independent 1-bar walk (" + std::to_string(got.size()) + " vs " + std::to_string(ref.size()) + ")");
        int pbBad = 0; for (const auto& kv : E.pb) if (!ref.count(kv.second.tp)) ++pbBad;
        CHECK(pbBad == 0, std::string("[2] ") + m.f + ": every Pb sits on a 1-bar pivot (" + std::to_string(E.pb.size()) + " Pb)");
        int fired = 0; for (const auto& kv : E.bo) if (kv.second.fired) ++fired;
        CHECK(fired > 0, std::string("[2] ") + m.f + ": BO / BD fired on 15-min (" + std::to_string(fired) + " of " + std::to_string(E.bo.size()) + ")");
        // [3] the Pb rule from the bars
        std::vector<double> atr(B.size(), NAN); { double y = 0; bool have = false;
            for (size_t k = 1; k < B.size(); ++k) { const double tr = std::max(B[k].h - B[k].l, std::max(std::fabs(B[k].h - B[k - 1].c), std::fabs(B[k].l - B[k - 1].c)));
                if (!have) { if (k >= 14) { double s = 0; for (size_t q = k - 13; q <= k; ++q) s += std::max(B[q].h - B[q].l, std::max(std::fabs(B[q].h - B[q - 1].c), std::fabs(B[q].l - B[q - 1].c))); y = s / 14; have = true; atr[k] = y; } }
                else { y = (y * 13 + tr) / 14; atr[k] = y; } } }
        int bad = 0, checked = 0;
        for (const auto& kv : E.pb) { const bko::PbMark& P = kv.second; const auto it = E.bo.find(P.t0); if (it == E.bo.end()) { ++bad; continue; }
            size_t i0 = 0, ip = 0; for (size_t k = 0; k < B.size(); ++k) { if (B[k].t == P.t0) i0 = k; if (B[k].t == P.tp) ip = k; }
            if (!std::isfinite(atr[i0])) continue;
            ++checked;
            const double piv = it->second.piv, ext = P.dir > 0 ? B[ip].l : B[ip].h, gap = P.dir > 0 ? ext - piv : piv - ext;
            if (!(gap <= atr[i0] / 3.0 + 1e-6)) ++bad; }
        CHECK(bad == 0, std::string("[3] ") + m.f + ": every Pb at / through the broken pivot or < ATR / 3 short (" + std::to_string(checked) + " checked, " + std::to_string(bad) + " bad)");
        // [4] replay
        bko::Engine R; for (int n = 40; n <= (int)B.size(); ++n) { std::vector<bko::In> P(B.begin(), B.begin() + n); R.update(P, n, P[0].t, m.tk, 900, 510, 900); }
        CHECK(boSet(R) == boSet(E) && pbSet(R) == pbSet(E), std::string("[4] ") + m.f + ": bar-by-bar replay == one full pass (" + std::to_string(E.bo.size()) + " BO / BD, " + std::to_string(E.pb.size()) + " Pb)");
        // [5] same as a default engine set to zzRev 1 + refine1
        bko::Engine D; D.zzRev = 1; D.refine1 = true; D.update(B, (long)B.size(), B[0].t, m.tk, 900, 510, 900);
        CHECK(boSet(D) == boSet(E) && pbSet(D) == pbSet(E) && keys(D.rly) == keys(E.rly), std::string("[5] ") + m.f + ": == the engine with zzRev 1 + refine1");
        std::printf("  %s 15-min: %zu bars, %d BO/BD fired, %zu Pb, %zu Rly\n", m.f, B.size(), fired, E.pb.size(), E.rly.size());
    }
    {   std::ifstream f("../Breakout1.cpp"); std::stringstream ss; ss << f.rdbuf(); const std::string s = ss.str();
        CHECK(s.find("#define BKO_ONE_BAR 1") != std::string::npos && s.find("#include \"Breakout.cpp\"") != std::string::npos && s.find("p->setVersion(\"1.6.0\")") != std::string::npos, "[6] Breakout1.cpp wrapper + version line"); }
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
