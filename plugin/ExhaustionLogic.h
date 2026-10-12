#ifndef LS_EXHAUSTION_LOGIC_H
#define LS_EXHAUSTION_LOGIC_H
// ExhaustionLogic.h (0.3.0, 2026-10-11) -- the SDK-free half of lsExhaustion. Rassul 2026-10-11 21:17-21:28: "model the exhaustion
// indicator also with two lines", option A chosen ("lets go with A"), "stick with an exhaustion indicator ... customized for each market
// to catch tops and bottoms (pivot highs and lows)", "build exhaustion indicator".
// At a new N-bar HIGH: BUYERS FADING = the buyers' peak pace (aggressive volume, S-bar average) in the push / their pace now. At a new
// N-bar LOW: SELLERS FADING, the mirror. Not at a new extreme: 1.0 (quiet). Exactly lra.exh_study's read (fades2), so the nightly numbers
// are the indicator's numbers.
// 0.3.0 (Rassul 22:24-22:26 "the exhaustion signal needs to be improved .. look at es", "look at this example in gold"): in a slow
// overnight drift every 1-tick new high was compared with one burst bar 20+ bars back, so the ratio crossed 3x with nobody pushing
// (ES overnight E at 3.0x: 0.0R on 500+ trades). Now the push must be RECENT (peak pace within the last K bars; K 0 = since the swing,
// the 0.1 read) and REAL (the peak >= P x the side's normal pace = median of the prior 40 bars, at least 10 known; P 0 = no gate).
// x* = the first bar over T (the value is written there once); e* = x* on the sides the study chose (SIDES 1 tops, 2 bottoms, 3 both).
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ex {

static const char* const VERSION = "0.3.0";       // keep equal to EX_VERSION / setVersion in Exhaustion.cpp
static const int LOOK = 20, NORMAL = 40, NORMAL_MIN = 10;

struct Params { int N = 10; int S = 1; double T = 2.0; int K = 0; double P = 0; int sides = 3; };
// per market (3-minute bars) until LRA-ExhNorms-<MKT>.csv says else: the 0.2 read in RTH (both sides), the study of 2026-10-11 22:40
// picks for the push gate where they held on unseen days
inline Params defaults(const std::string& m) {
    Params p;
    if (m == "ES" || m == "NQ") { p.N = 5; p.S = 2; p.T = 3.0; }
    else if (m == "CL") { p.N = 10; p.S = 1; p.T = 3.0; }
    else if (m == "GC") { p.N = 20; p.S = 1; p.T = 3.0; }
    else if (m == "HG") { p.N = 5; p.S = 2; p.T = 2.0; }
    else if (m == "NG") { p.N = 20; p.S = 1; p.T = 1.5; }
    else if (m == "EU") { p.N = 10; p.S = 1; p.T = 1.5; }
    return p;
}

struct Out { std::vector<double> fb, fs; std::vector<char> newH, newL, xTop, xBot, eTop, eBot; };

// the median of v[a..b) (pandas rolling median: the mean of the two middle values for an even count)
inline double median(const std::vector<double>& v, int a, int b) {
    std::vector<double> w(v.begin() + a, v.begin() + b); const int n = (int)w.size();
    std::sort(w.begin(), w.end());
    return n % 2 ? w[n / 2] : 0.5 * (w[n / 2 - 1] + w[n / 2]);
}

// h / l / buy / sell of CLOSED bars 0..n-1 (0.3.0: the bar in progress is never passed - its half-built volume read as a fade)
inline Out compute(const std::vector<double>& h, const std::vector<double>& l, const std::vector<double>& buy, const std::vector<double>& sell, const Params& P) {
    const int n = (int)h.size(); Out o;
    o.fb.assign(n, 1.0); o.fs.assign(n, 1.0); o.newH.assign(n, 0); o.newL.assign(n, 0); o.xTop.assign(n, 0); o.xBot.assign(n, 0); o.eTop.assign(n, 0); o.eBot.assign(n, 0);
    if (n == 0 || (int)l.size() != n || (int)buy.size() != n || (int)sell.size() != n) return o;
    const int S = std::max(1, P.S), N = std::max(1, P.N), K = std::max(0, P.K);
    std::vector<double> bp(n), sp(n);
    for (int i = 0; i < n; ++i) { double a = 0, b = 0; int k = 0; for (int j = std::max(0, i - S + 1); j <= i; ++j) { a += buy[j]; b += sell[j]; ++k; } bp[i] = a / k; sp[i] = b / k; }
    for (int i = N; i < n; ++i) {
        const int a = std::max(0, i - LOOK);
        double mh = h[i - N], ml = l[i - N];
        for (int j = i - N; j < i; ++j) { mh = std::max(mh, h[j]); ml = std::min(ml, l[j]); }
        const int na = std::max(0, i - NORMAL); const bool known = i - na >= NORMAL_MIN;
        for (int side = 0; side < 2; ++side) {
            const bool top = side == 0;
            if (top ? !(h[i] >= mh) : !(l[i] <= ml)) continue;
            (top ? o.newH : o.newL)[i] = 1;
            const std::vector<double>& pc = top ? bp : sp;
            int j0;
            if (K > 0) j0 = std::max(0, i - K);
            else { j0 = a; for (int j = a; j <= i; ++j) if (top ? l[j] < l[j0] : h[j] > h[j0]) j0 = j; }
            double pk = 0; for (int j = j0; j <= i; ++j) pk = std::max(pk, pc[j]);
            if (P.P > 0) { if (!known || pk < P.P * median(pc, na, i)) continue; }
            (top ? o.fb : o.fs)[i] = pk / std::max(pc[i], 1.0);
        }
    }
    for (int i = 1; i < n; ++i) {
        o.xTop[i] = o.newH[i] && o.fb[i] >= P.T && !(o.newH[i - 1] && o.fb[i - 1] >= P.T);
        o.xBot[i] = o.newL[i] && o.fs[i] >= P.T && !(o.newL[i - 1] && o.fs[i - 1] >= P.T);
        o.eTop[i] = o.xTop[i] && (P.sides & 1); o.eBot[i] = o.xBot[i] && (P.sides & 2);
    }
    return o;
}

}  // namespace ex
#endif
