// fuzz_tapeflow_logic.cpp - random tapes: no crash / UB, mirror symmetry, prefix + redraw invariance, sane outputs.
// (1.1.x) a missing quote now poisons its second and restarts the warm-up, as on a real broken feed; real IRT data has bid / ask
// on ~100% of trades, so the fuzz tapes drop a quote 1 in 2000 trades (was 1 in 50, which meant almost no signals).
// build: g++ -std=c++17 -O1 -g -fsanitize=address,undefined -I. -o f fuzz_tapeflow_logic.cpp && ./f
#include "TapeFlowLogic.h"
#include <random>
#include <cstdlib>
using namespace tfl;
static const long long T0 = 1790000000LL - (1790000000LL % 86400) + 9 * 3600;
static std::vector<Tick> tape(unsigned seed, int secs)
{
    std::mt19937 r(seed); std::vector<Tick> v; int p = 1000; long long t = T0; int regime = 0;
    for (int i = 0; i < secs; i++) {
        if (r() % 60 == 0) regime = (int)(r() % 5);
        int step = (int)(r() % 7) - 3; if (regime == 1) step += 1; if (regime == 2) step -= 1; if (r() % 3) step = 0;
        p += step; if (p < 100) p = 100;
        int spread = (r() % 20 == 0) ? 2 : 1;
        int n = regime == 3 ? 0 : (int)(r() % 6);                      // quiet regime: no trades
        if (r() % 400 == 0) t += 20 + (long long)(r() % 600);           // holes, sometimes > 5 min
        for (int k = 0; k < n; k++) {
            int b = p, a = p + spread; int kind = (int)(r() % 10);
            int px = kind < 4 ? a : kind < 8 ? b : (spread == 2 ? p + 1 : a);
            if (regime == 4 && kind < 7) px = b;                            // a selling attack
            long q = 1 + (long)(r() % (regime == 4 ? 40 : 15));
            v.push_back(Tick{t, px, (r() % 2000 == 0) ? 0 : b, a, q});
        }
        t++;
    }
    return v;
}
static BinBase base() { BinBase b; b.n = 100; for (int h = 0; h < 8; h++) { b.qb[h] = 120; b.qs[h] = 120; } b.med20 = 12; b.med5 = 12; b.medSpread = 1; return b; }
static Engine run(const std::vector<Tick>& v, bool inter)
{
    Engine e; e.setFixedBase(base());
    for (auto& k : v) { if (inter) e.advanceTo(k.t - 1); e.add(k); }
    if (!v.empty()) e.advanceTo(v.back().t);
    return e;
}
static std::string key(const Ev& x, bool mir) { char b[160]; snprintf(b, sizeof(b), "%lld %s %d %d %d", x.t, x.kind.c_str(), mir ? -x.dir : x.dir, mir ? 4000 - x.hi : x.lo, mir ? 4000 - x.lo : x.hi); return b; }
int main(int argc, char** argv)
{
    int maxSeed = 300;
    if (argc > 1) { long n = std::strtol(argv[1], nullptr, 10); if (n > 0 && n <= 300) maxSeed = (int)n; }
    int bad = 0, evTotal = 0; std::map<std::string, int> kinds;
    for (unsigned seed = 1; seed <= (unsigned)maxSeed; seed++) {
        std::vector<Tick> v = tape(seed, 2400);
        Engine a = run(v, false), b = run(v, true);
        Engine lv; lv.setFixedBase(base()); { size_t i = 0; for (long long t = v.front().t; t <= v.back().t; t++) { while (i < v.size() && v[i].t == t) lv.add(v[i++]); lv.advanceTo(t - 3); } lv.advanceTo(v.back().t); }
        { std::vector<std::string> K1, K2; for (auto& x : a.evs) K1.push_back(key(x, false)); for (auto& x : lv.evs) K2.push_back(key(x, false));
          if (K1 != K2 || a.hist.size() != lv.hist.size()) { bad++; printf("seed %u: live clock walk differs from back-fill (%zu vs %zu events)\n", seed, K1.size(), K2.size()); } }
        std::vector<Tick> m = v; for (auto& k : m) { int bb = k.bid, aa = k.ask; k.px = 4000 - k.px; k.bid = bb > 0 ? 4000 - aa : 0; k.ask = 4000 - bb; if (bb <= 0) { k.ask = 4000 - aa; } }
        // a missing bid mirrors to a missing ask: rebuild exactly
        for (size_t i = 0; i < v.size(); i++) { if (v[i].bid <= 0) { m[i].bid = 4000 - v[i].ask; m[i].ask = 0; } }
        Engine c = run(m, false);
        // the two sides are evaluated bullish-first in a second, so a mirrored tape lists same-second events in the other order:
        // compare the sorted event sets (the order across seconds is still checked by the time in the key)
        std::vector<std::string> KA, KB, KC;
        for (auto& x : a.evs) KA.push_back(key(x, false));
        for (auto& x : b.evs) KB.push_back(key(x, false));
        for (auto& x : c.evs) KC.push_back(key(x, true));
        bool sameAB = KA == KB;
        std::sort(KA.begin(), KA.end()); std::sort(KC.begin(), KC.end());
        bool sameAC = KA == KC;
        if (!sameAB) { bad++; printf("seed %u: redraw changed events\n", seed); }
        if (!sameAC) { bad++; printf("seed %u: mirror differs (%zu vs %zu)\n", seed, a.evs.size(), c.evs.size());
            for (size_t i = 0; i < std::max(KA.size(), KC.size()); i++) { std::string x = i < KA.size() ? KA[i] : "-", y = i < KC.size() ? KC[i] : "-"; if (x != y) { printf("  %s | %s\n", x.c_str(), y.c_str()); } } break; }
        // prefix invariance at 3 cuts
        for (int cut : {700, 1500, 2100}) {
            if ((size_t)cut >= v.size()) continue;
            std::vector<Tick> pre(v.begin(), v.begin() + cut); Engine p = run(pre, false); long long lt = pre.back().t;
            std::vector<std::string> A, P;
            for (auto& x : a.evs) if (x.t < lt) A.push_back(key(x, false));
            for (auto& x : p.evs) if (x.t < lt) P.push_back(key(x, false));
            if (A != P) { bad++; printf("seed %u cut %d: prefix changed (%zu vs %zu)\n", seed, cut, A.size(), P.size()); }
        }
        // sanity: values in range, one live watch per side, AR / outcomes follow their AW
        std::map<int, int> state;
        for (auto& x : a.evs) {
            kinds[x.kind]++; evTotal++;
            if (x.kind == "AW") { if (state[x.ep] != 1) { bad++; printf("seed %u: AW without candidate ep %d\n", seed, x.ep); } state[x.ep] = 2; }
            else if (x.kind == "CAND") state[x.ep] = 1;
            else if (x.kind == "AR" || x.kind == "FAIL" || x.kind == "INVP" || x.kind == "EXP") { if (state[x.ep] != 2) { bad++; printf("seed %u: %s without AW ep %d\n", seed, x.kind.c_str(), x.ep); } state[x.ep] = 3; }
        }
        for (auto& h : a.hist) if (!std::isnan(h.f30) && (h.f30 < -100.001f || h.f30 > 100.001f)) { bad++; printf("seed %u: F30 out of range\n", seed); break; }
        for (size_t i = 1; i < a.hist.size(); i++) if (a.hist[i].t <= a.hist[i - 1].t) { bad++; printf("seed %u: history not increasing\n", seed); break; }
    }
    printf("events %d:", evTotal); for (auto& k : kinds) printf(" %s=%d", k.first.c_str(), k.second); printf("\n%s (%d problems)\n", bad ? "FAIL" : "OK", bad);
    return bad ? 1 : 0;
}
