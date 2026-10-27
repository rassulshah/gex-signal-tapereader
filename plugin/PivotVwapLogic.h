#ifndef LS_PIVOTVWAP_H
#define LS_PIVOTVWAP_H
// PivotVwapLogic.h 1.0.0 (2026-10-27, Rassul 09:46 CT "lets go with your recommendation and modify the vwap to the 3bar pivot")
// The 3-BAR PIVOT VWAP: a VWAP anchored at the last confirmed 3-bar-reversal pivot of the RTH session, restarted at each new pivot,
// coloured by the close: green = the bar closed above it, red = below. SDK-free, C++11, no files, no state between calls.
// Pivot (same rule as the 2026-10-26 trend test, trend_test/code/anchors.py rev3): in an up swing a pivot high is confirmed when
// 3 closes in a row fall (c[k] < c[k-1] < c[k-2] < c[k-3]), at least 3 bars after the swing high, and the 3rd close is below the
// low of the swing-high bar; mirror for a pivot low. The line at bar k uses only the anchor known at bar k's close: no look-ahead,
// a closed bar's value and colour never change (no repaint).
// Tested (2026-10-27, 194 recorded sessions, 7 markets): direction 30 min ahead 0.500 (same as session VWAP), flips 11.4 / day
// (session VWAP 9.1), 30% of a 2-ATR swing left at the flip (session VWAP 27%): faster, NOT more accurate - a context line only.
#include <vector>
#include <cmath>
namespace pvw {
static const char* const VERSION = "1.0.0";
// h, l, c, v: one RTH session's bars in time order. out: line value (NaN = none yet), state (+1 above / -1 below / 0), anchor bar.
struct Out { std::vector<double> line; std::vector<int> state, anchor; };
inline void anchors(const std::vector<double>& h, const std::vector<double>& l, const std::vector<double>& c, std::vector<int>& piv) {
    const int n = (int)c.size(); piv.assign(n, -1); if (n < 2) return;
    int cur = c[1] >= c[0] ? 1 : -1, ext = 0, last = -1;
    for (int k = 1; k < n; ++k) {
        if (cur == 1) {
            if (h[k] >= h[ext]) ext = k;
            else if (k - ext >= 3 && c[k] < c[k-1] && c[k-1] < c[k-2] && c[k-2] < c[k-3] && c[k] < l[ext]) {
                last = ext; cur = -1; int m = ext + 1; for (int j = ext + 1; j <= k; ++j) if (l[j] < l[m]) m = j; ext = m; }
        } else {
            if (l[k] <= l[ext]) ext = k;
            else if (k - ext >= 3 && c[k] > c[k-1] && c[k-1] > c[k-2] && c[k-2] > c[k-3] && c[k] > h[ext]) {
                last = ext; cur = 1; int m = ext + 1; for (int j = ext + 1; j <= k; ++j) if (h[j] > h[m]) m = j; ext = m; }
        }
        piv[k] = last;
    }
}
inline Out compute(const std::vector<double>& h, const std::vector<double>& l, const std::vector<double>& c, const std::vector<double>& v) {
    Out o; const int n = (int)c.size(); o.line.assign(n, NAN); o.state.assign(n, 0); o.anchor.assign(n, -1);
    if (n == 0 || (int)h.size() != n || (int)l.size() != n || (int)v.size() != n) return o;
    std::vector<int> piv; anchors(h, l, c, piv);
    std::vector<double> cpv(n + 1, 0.0), cv(n + 1, 0.0);
    for (int i = 0; i < n; ++i) {
        const double tp = (h[i] + l[i] + c[i]) / 3.0, vv = (std::isfinite(v[i]) && v[i] > 0 && std::isfinite(tp)) ? v[i] : 0.0;
        cpv[i + 1] = cpv[i] + tp * vv; cv[i + 1] = cv[i] + vv;
    }
    for (int k = 0; k < n; ++k) {
        const int a = piv[k] >= 0 ? piv[k] : 0;          // before the first pivot: anchored at the session open
        const double den = cv[k + 1] - cv[a];
        if (den > 0) { o.line[k] = (cpv[k + 1] - cpv[a]) / den; o.anchor[k] = a;
            o.state[k] = c[k] > o.line[k] ? 1 : (c[k] < o.line[k] ? -1 : (k > 0 ? o.state[k - 1] : 0)); }
    }
    return o;
}
} // namespace pvw
#endif
