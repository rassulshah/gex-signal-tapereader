#ifndef LS_EXHAUSTION_PACE_H
#define LS_EXHAUSTION_PACE_H
// ExhaustionPace.h (0.4.0, 2026-10-29) -- the SDK-free PACE / SIGNAL half of lsExhaustion 0.4.0.
// Rassul 07:08 CT approved "implement and then show mockup" (his sketch: the attacker's pace line rising then rolling over, a slower
// yellow signal line under it - MACD-style).
//   PACE   (per side, every closed bar) = that side's aggressive volume (buyers = traded at the ask, sellers = at the bid) averaged over
//          the market's S bars (ES / NQ 2) - the same pace the ratio uses.
//   SIGNAL = EMA(PACE, 9 bars), alpha 2 / 10, seeded with the first pace.
//   PUSH   (0.2.4 window) = the last 20 bars incl. this one: lowest low came BEFORE the highest high -> UP push (buyers attack), else DOWN
//          push (sellers attack); size = highest high - lowest low; distance = bars since the push extreme (the high of an up push).
//   ATR    = mean bar range (high - low) of the last 14 bars.
//   MARKS  (attacking side only, only when push size >= 2.0 x ATR - smaller pushes are noise; decided on the close, never repainted):
//          SLOWED = pace still above signal, pace below the bar before, and the bar before had risen (first bar of flattening);
//          TURNED = pace crosses below signal (bar before >= its signal, this bar < its signal).
#include <algorithm>
#include <cmath>
#include <vector>

namespace pace {

static const int SIG = 9, PUSH = 20, ATRN = 14;
static const double MIN_PUSH_ATR = 2.0;
enum { NONE = 0, SLOWED = 1, TURNED = 2 };

struct Out {
    std::vector<double> pb, ps, sb, ss, push, atr;   // pace buyers / sellers, signal buyers / sellers, push size, ATR
    std::vector<int> dir, dist;                      // +1 up push (buyers attack) / -1 down push (sellers); bars since the push extreme
    std::vector<signed char> mark;                   // NONE / SLOWED / TURNED for the attacking side of that bar
};

inline Out compute(const std::vector<double>& h, const std::vector<double>& l, const std::vector<double>& buy, const std::vector<double>& sell, int S) {
    Out o; const int n = (int)h.size();
    if (n == 0 || (int)l.size() != n || (int)buy.size() != n || (int)sell.size() != n) return o;
    for (int i = 0; i < n; ++i) if (!std::isfinite(h[i]) || !std::isfinite(l[i]) || !std::isfinite(buy[i]) || !std::isfinite(sell[i]) || h[i] < l[i] || buy[i] < 0 || sell[i] < 0) return o;
    S = std::max(1, S);
    o.pb.assign(n, 0); o.ps.assign(n, 0); o.sb.assign(n, 0); o.ss.assign(n, 0); o.push.assign(n, 0); o.atr.assign(n, 0);
    o.dir.assign(n, 0); o.dist.assign(n, 0); o.mark.assign(n, NONE);
    const double a = 2.0 / (SIG + 1);
    for (int i = 0; i < n; ++i) {
        double x = 0, y = 0; int k = 0; for (int j = std::max(0, i - S + 1); j <= i; ++j) { x += buy[j]; y += sell[j]; ++k; }
        o.pb[i] = x / k; o.ps[i] = y / k;
        o.sb[i] = i == 0 ? o.pb[i] : o.sb[i - 1] + a * (o.pb[i] - o.sb[i - 1]);
        o.ss[i] = i == 0 ? o.ps[i] : o.ss[i - 1] + a * (o.ps[i] - o.ss[i - 1]);
        const int f = std::max(0, i - PUSH + 1); int jh = f, jl = f;
        for (int j = f; j <= i; ++j) { if (h[j] >= h[jh]) jh = j; if (l[j] <= l[jl]) jl = j; }   // the later of equal extremes
        o.dir[i] = jl < jh ? 1 : -1; o.push[i] = h[jh] - l[jl]; o.dist[i] = o.dir[i] > 0 ? i - jh : i - jl;
        double r = 0; int m = 0; for (int j = std::max(0, i - ATRN + 1); j <= i; ++j) { r += h[j] - l[j]; ++m; }
        o.atr[i] = r / m;
        if (i < 2 || o.atr[i] <= 0 || o.push[i] < MIN_PUSH_ATR * o.atr[i]) continue;
        const std::vector<double>& p = o.dir[i] > 0 ? o.pb : o.ps; const std::vector<double>& s = o.dir[i] > 0 ? o.sb : o.ss;
        if (p[i - 1] >= s[i - 1] && p[i] < s[i]) o.mark[i] = TURNED;
        else if (p[i] > s[i] && p[i] < p[i - 1] && p[i - 1] > p[i - 2]) o.mark[i] = SLOWED;
    }
    return o;
}

}  // namespace pace
#endif
