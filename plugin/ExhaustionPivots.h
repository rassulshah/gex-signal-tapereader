#ifndef LS_EXHAUSTION_PIVOTS_H
#define LS_EXHAUSTION_PIVOTS_H
// ExhaustionPivots.h (0.3.0, 2026-10-29) -- the SDK-free pivot half of lsExhaustion 0.3.0.
// Rassul 05:11 CT 2026-10-29: "update the exhaustion indicator so it follows the pivot rule and only bars above 2x are shown and draw
// it like a histogram instead of connected lines".
// PIVOTS = IRT's own ZIGZAG, Min Price Change = N BARS (N = 3), Use High/Low - the SAME rule as lsWavePOC 1.3.2 (copied, not shared,
// so the two plugins build independently): the live leg's extreme moves to every STRICTLY more extreme bar (an equal one does not); a
// bar whose high beats the previous N bars' highs reverses a down leg, a bar whose low is under the previous N bars' lows reverses an
// up leg - the old extreme is then a FINAL pivot, confirmed on that reversal bar (never earlier, never changed afterwards).
// MEASURED at each confirmed pivot, over the zigzag leg that ENDS at the pivot (start = the previous pivot; the first pivot of the
// window uses the lsExhaustion 0.2 start: the opposite extreme of the last 20 bars):
//   pivot HIGH -> BUYERS FADING  = the buyers' peak pace in the up leg / the buyers' pace on the pivot bar
//   pivot LOW  -> SELLERS FADING = the sellers' peak pace in the down leg / the sellers' pace on the pivot bar
// pace = aggressive volume (buyers = traded at the ask, sellers = at the bid) averaged over S bars (the market's S, ES / NQ 2), the
// same pace lsExhaustion 0.2 used; the denominator is floored at 1 contract. 3.0x = the side traded at a third of its peak pace when
// price made the turn. Only ratios >= 2.0x are drawn (all are logged).
#include <algorithm>
#include <cmath>
#include <vector>

namespace exp3 {

static const int ZZ_N = 3;          // IRT ZigZag "Min Price Change" in bars (lsWavePOC Reversal bars default)
static const int LOOK = 20;         // first-pivot fallback start (lsExhaustion 0.2 LOOK)
static const double SHOW = 2.0;     // drawn at / over this ratio

struct PivotEx { int i = 0; int type = 0; int conf = 0; int start = 0; double ratio = 1.0; };   // type +1 high (buyers fading) / -1 low

// h / l / buy / sell of CLOSED bars 0..n-1. Returns the confirmed pivots in order. Invalid input -> none (all-or-nothing, as ex::compute).
inline std::vector<PivotEx> pivots(const std::vector<double>& h, const std::vector<double>& l, const std::vector<double>& buy,
                                   const std::vector<double>& sell, int S, int N = ZZ_N) {
    std::vector<PivotEx> out; const int n = (int)h.size();
    if (n == 0 || (int)l.size() != n || (int)buy.size() != n || (int)sell.size() != n) return out;
    for (int i = 0; i < n; ++i) if (!std::isfinite(h[i]) || !std::isfinite(l[i]) || !std::isfinite(buy[i]) || !std::isfinite(sell[i]) || h[i] < l[i] || buy[i] < 0 || sell[i] < 0) return out;
    S = std::max(1, S); N = std::max(1, N);
    std::vector<double> bp(n), sp(n);
    for (int i = 0; i < n; ++i) { double a = 0, b = 0; int k = 0; for (int j = std::max(0, i - S + 1); j <= i; ++j) { a += buy[j]; b += sell[j]; ++k; } bp[i] = a / k; sp[i] = b / k; }
    int zDir = 0, zI = 0, zHi = 0, zLo = 0;
    auto turn = [&](int type, int at, int k) {
        PivotEx p; p.i = at; p.type = type; p.conf = k;
        if (!out.empty()) p.start = out.back().i;
        else { const int a = std::max(0, at - LOOK); int j0 = a; for (int j = a; j <= at; ++j) if (type > 0 ? l[j] < l[j0] : h[j] > h[j0]) j0 = j; p.start = j0; }
        double pk = 0; for (int j = std::min(p.start, at); j <= at; ++j) pk = std::max(pk, type > 0 ? bp[j] : sp[j]);
        p.ratio = pk / std::max(type > 0 ? bp[at] : sp[at], 1.0);
        out.push_back(p);
    };
    for (int k = 0; k < n; ++k) {
        if (k >= N) {
            double ph = h[k - N], pl = l[k - N]; for (int j = k - N; j < k; ++j) { ph = std::max(ph, h[j]); pl = std::min(pl, l[j]); }
            if (zDir == 0) {
                if (h[k] > ph) { turn(-1, zLo, k); zDir = 1; zI = k; }
                else if (l[k] < pl) { turn(+1, zHi, k); zDir = -1; zI = k; }
            } else if (zDir > 0) {
                if (h[k] > h[zI]) zI = k;
                else if (l[k] < pl) { turn(+1, zI, k); zDir = -1; zI = k; }
            } else {
                if (l[k] < l[zI]) zI = k;
                else if (h[k] > ph) { turn(-1, zI, k); zDir = 1; zI = k; }
            }
        }
        if (zDir == 0) { if (h[k] > h[zHi]) zHi = k; if (l[k] < l[zLo]) zLo = k; }
    }
    return out;
}

}  // namespace exp3
#endif
