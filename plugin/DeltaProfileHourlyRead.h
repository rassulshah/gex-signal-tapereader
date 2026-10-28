#ifndef LS_DELTA_PROFILE_HOURLY_READ_H
#define LS_DELTA_PROFILE_HOURLY_READ_H
// DeltaProfileHourlyRead.h 1.0.0 (2026-10-28, Rassul 22:41 CT "see how it can be optimized on the hourly chart and what signals to put on
// it and how to incorporate it to the context") -- the SDK-free hourly READ, from the hourly research (outputs/hdp1028, report
// "Hourly Delta Profile"):
//   1. SAME-CANDLE hourly A (the only hourly mark that earned a place: +0.33R build / +0.39R untuned vs the hourly reversal candle
//      without it, n 48 -> shown as "A?"): the closed hourly candle k is a local extreme (its low <= the 3 lows before it, bull), closes
//      in its own colour in its own half (green, close >= mid), and one of the 24-hour profile's top-3 nodes (>= 2.0x the mean |row
//      delta|) is the attackers' (a SELLING node for a bull A) with k's own delta there >= 10% of the node, inside k's range, at or below
//      k's open, in k's lower half, and k closes above it. Bearish mirrors. The NEXT-candle confirmation (the old rule) is NOT used: it
//      tested clearly negative (-0.64R) and is record only in the engine.
//   2. The hourly FLOW: net delta (bought at the ask - sold at the bid) of the last 4 closed hours and of the last closed hour. A 3-min A
//      whose side won the 4 hours while the last hour went against it ("pullback in the hourly flow") turned 82% vs 71%, swung 57% vs 46%,
//      +0.18R vs +0.04R (same-candle A, both halves, 7 / 7 markets better on turn and swing; no effect on plain reversal candles).
// Rows / ratios exactly as the research (hdp/hcore.py): rows of ceil((max-min+1)/60) ticks from the window's lowest traded tick.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
namespace dph_read {
static const char* const VERSION = "1.0.0";
typedef std::int64_t Tick;
struct Row { Tick key; double delta, volume; };
struct HBar { long long end = 0; Tick o = 0, h = 0, l = 0, c = 0; std::vector<Row> rows; };   // prices in ticks, rows ascending
struct Decision { bool ok = false, bull = false; Tick lo = 0, hi = 0; double ratio = 0; long long end = 0; };
struct Flow { bool ok = false; double net4 = 0, net1 = 0; int hours = 0; };
struct Params { int window = 24, top = 3, local = 3; double nodeMin = 2.0, formShare = 0.10; int rows = 60; };
inline double barNet(const HBar& b) { double d = 0; for (const Row& r : b.rows) d += r.delta; return d; }
// the window = the newest P.window closed bars ending at k (bars must be chronological); k = bars.size() - 1
inline Decision sameCandleA(const std::vector<HBar>& bars, const Params& P = Params()) {
    Decision out;
    const int n = static_cast<int>(bars.size());
    if (n < std::max(P.window, P.local + 1)) return out;
    const int k = n - 1, a = n - P.window;
    const HBar& K = bars[static_cast<std::size_t>(k)];
    Tick lo = 0, hi = 0; bool any = false;
    for (int i = a; i <= k; ++i) for (const Row& r : bars[static_cast<std::size_t>(i)].rows) {
        if (!any) { lo = hi = r.key; any = true; } lo = std::min(lo, r.key); hi = std::max(hi, r.key); }
    if (!any) return out;
    const Tick rs = std::max<Tick>(1, (hi - lo + 1 + P.rows - 1) / P.rows);
    const std::size_t nb = static_cast<std::size_t>((hi - lo) / rs) + 1;
    std::vector<double> D(nb, 0.0), V(nb, 0.0), Dk(nb, 0.0);
    for (int i = a; i <= k; ++i) for (const Row& r : bars[static_cast<std::size_t>(i)].rows) {
        const std::size_t j = static_cast<std::size_t>((r.key - lo) / rs); D[j] += r.delta; V[j] += r.volume; if (i == k) Dk[j] += r.delta; }
    double sum = 0; int cnt = 0; for (std::size_t j = 0; j < nb; ++j) if (V[j] > 0) { sum += std::fabs(D[j]); ++cnt; }
    if (cnt == 0 || sum <= 0) return out;
    const double mean = sum / cnt;
    std::vector<std::size_t> idx(nb); for (std::size_t j = 0; j < nb; ++j) idx[j] = j;
    std::stable_sort(idx.begin(), idx.end(), [&D](std::size_t x, std::size_t y) { return std::fabs(D[x]) > std::fabs(D[y]); });
    // the candle shape: a local extreme closing in its own colour in its own half
    for (int pass = 0; pass < 2; ++pass) {
        const bool bull = pass == 0;
        bool loc = true;
        for (int i = k - P.local; i < k; ++i) { const HBar& b = bars[static_cast<std::size_t>(i)]; if (bull ? !(K.l <= b.l) : !(K.h >= b.h)) loc = false; }
        if (!loc) continue;
        const double mid2 = static_cast<double>(K.h + K.l);   // 2 x mid
        const bool shape = bull ? (K.c > K.o && 2.0 * K.c >= mid2) : (K.c < K.o && 2.0 * K.c <= mid2);
        if (!shape) continue;
        for (std::size_t t = 0; t < idx.size() && static_cast<int>(t) < P.top; ++t) {
            const std::size_t j = idx[t]; const double ratio = std::fabs(D[j]) / mean;
            if (ratio < P.nodeMin) break;
            if ((D[j] < 0) != bull) continue;                                  // the attackers' node: selling under a bull A
            if (std::fabs(Dk[j]) < P.formShare * std::fabs(D[j]) || (Dk[j] < 0) != (D[j] < 0) || Dk[j] == 0) continue;
            const Tick nlo = lo + static_cast<Tick>(j) * rs, nhi = nlo + rs - 1;
            if (nlo > K.h || nhi < K.l) continue;
            const bool ok = bull ? (nhi <= K.o && static_cast<double>(nlo + nhi) <= mid2 && K.c > nhi)
                                 : (nlo >= K.o && static_cast<double>(nlo + nhi) >= mid2 && K.c < nlo);
            if (!ok) continue;
            out.ok = true; out.bull = bull; out.lo = nlo; out.hi = nhi; out.ratio = ratio; out.end = K.end;
            return out;
        }
    }
    return out;
}
inline Flow flow(const std::vector<HBar>& closedBars) {
    Flow f; const int n = static_cast<int>(closedBars.size());
    if (n < 1) return f;
    f.net1 = barNet(closedBars.back());
    for (int i = std::max(0, n - 4); i < n; ++i) f.net4 += barNet(closedBars[static_cast<std::size_t>(i)]);
    f.hours = std::min(4, n); f.ok = f.hours >= 4;
    return f;
}
// +1 = pullback in a BUYING 4-hour flow (supports a bull A), -1 = pullback in a SELLING flow, 0 = no pullback
inline int pullback(const Flow& f) { if (!f.ok) return 0; if (f.net4 > 0 && f.net1 <= 0) return 1; if (f.net4 < 0 && f.net1 >= 0) return -1; return 0; }
}  // namespace dph_read
#endif
