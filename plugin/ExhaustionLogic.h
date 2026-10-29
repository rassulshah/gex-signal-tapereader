#ifndef LS_EXHAUSTION_LOGIC_H
#define LS_EXHAUSTION_LOGIC_H
// ExhaustionLogic.h (0.3.0, 2026-10-29: + the T mark, tMark) (0.2.2, 2026-10-13) -- the SDK-free half of lsExhaustion. 0.2.2 (audit port): incomplete/non-finite inputs are rejected before a crossing (never an E); 0.1.0 2026-10-11. Rassul 2026-10-11 21:17-21:28: "model the exhaustion
// indicator also with two lines", option A chosen ("lets go with A"), "stick with an exhaustion indicator ... customized for each market
// to catch tops and bottoms (pivot highs and lows)", "build exhaustion indicator".
// At a new N-bar HIGH: BUYERS FADING = the buyers' peak pace (aggressive volume, S-bar average) since the lowest low of the last 20 bars /
// their pace now. At a new N-bar LOW: SELLERS FADING, the mirror. Not at a new extreme: 1.0 (quiet). E = the first bar the fade reaches T
// (not the bar after another E of the same side). Exactly lra.exh_study's read, so the nightly numbers are the indicator's numbers.
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ex {

static const char* const VERSION = "0.3.1";   // (0.3.1, 2026-10-29) + the T mark (tMark)       // keep equal to EX_VERSION / setVersion in Exhaustion.cpp
static const int LOOK = 20;

struct Params { int N = 10; int S = 1; double T = 2.0; };
// per market (3-minute bars, the study of 2026-10-11 on his IRT store: ES Aug-Oct, the rest Sep-Oct) until LRA-ExhNorms-<MKT>.csv says else
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

struct Out { std::vector<double> fb, fs; std::vector<char> newH, newL, eTop, eBot; };

// h / l / buy / sell of the bars 0..n-1 (closed bars; the caller may include the forming bar last)
inline Out compute(const std::vector<double>& h, const std::vector<double>& l, const std::vector<double>& buy, const std::vector<double>& sell, const Params& P) {
    const int n = (int)h.size(); Out o;
    o.fb.assign(n, 1.0); o.fs.assign(n, 1.0); o.newH.assign(n, 0); o.newL.assign(n, 0); o.eTop.assign(n, 0); o.eBot.assign(n, 0);
    if (n == 0 || (int)l.size() != n || (int)buy.size() != n || (int)sell.size() != n || !std::isfinite(P.T)) return o;
    // (0.2.2) admission is all-or-nothing: an unavailable/corrupt value is never turned into a zero pace and a false E
    for (int i = 0; i < n; ++i) if (!std::isfinite(h[i]) || !std::isfinite(l[i]) || !std::isfinite(buy[i]) || !std::isfinite(sell[i]) || h[i] < l[i] || buy[i] < 0 || sell[i] < 0) return o;
    const int S = std::max(1, P.S), N = std::max(1, P.N);
    std::vector<double> bp(n), sp(n);
    for (int i = 0; i < n; ++i) { double a = 0, b = 0; int k = 0; for (int j = std::max(0, i - S + 1); j <= i; ++j) { a += buy[j]; b += sell[j]; ++k; } bp[i] = a / k; sp[i] = b / k; }
    for (int i = N; i < n; ++i) {
        const int a = std::max(0, i - LOOK);
        double mh = h[i - N], ml = l[i - N];
        for (int j = i - N; j < i; ++j) { mh = std::max(mh, h[j]); ml = std::min(ml, l[j]); }
        if (h[i] >= mh) {
            o.newH[i] = 1; int j0 = a; for (int j = a; j <= i; ++j) if (l[j] < l[j0]) j0 = j;
            double pk = 0; for (int j = j0; j <= i; ++j) pk = std::max(pk, bp[j]);
            o.fb[i] = pk / std::max(bp[i], 1.0);
        }
        if (l[i] <= ml) {
            o.newL[i] = 1; int j0 = a; for (int j = a; j <= i; ++j) if (h[j] > h[j0]) j0 = j;
            double pk = 0; for (int j = j0; j <= i; ++j) pk = std::max(pk, sp[j]);
            o.fs[i] = pk / std::max(sp[i], 1.0);
        }
    }
    for (int i = 1; i < n; ++i) {
        o.eTop[i] = o.newH[i] && o.fb[i] >= P.T && !(o.newH[i - 1] && o.fb[i - 1] >= P.T);
        o.eBot[i] = o.newL[i] && o.fs[i] >= P.T && !(o.newL[i - 1] && o.fs[i - 1] >= P.T);
    }
    return o;
}


// ---- (0.3.0, 2026-10-29, Rassul approved 10:23 CT) the T mark: an order-line HELD (lsLiquidityProfile 0.68.0, SignalBoard fact 'G')
// confirmed by exhaustion of the side that attacked it. At an OFFER that HELD (decision bar d): the BUYERS' fade >= T_EXH (2.0x) on some
// bar e in [d - 5, d], and a close back below the offer within the 3 bars after e (e + 1 .. e + 3) -> a red T (a top); a BID that
// HELD: the SELLERS' fade >= 2.0x and a close back above the bid -> a green T (a bottom). Decided on CLOSED bars only, at the bar where
// the last condition is met (never before the HELD decision bar); once decided it is recorded and never changes.
static const double T_EXH = 2.0; static const int T_BEFORE = 5, T_AFTER = 3;
struct Held { int bar = -1; int side = 0; double px = 0; };   // side +1 = a bid HELD (support), -1 = an offer HELD (resistance)
struct TMark { int bar = -1; int side = 0; int exhBar = -1; double exh = 0; };
// c = closes of the same closed bars as o (fb / fs); lastClosed = the last index that may be used
inline bool tMark(const std::vector<double>& c, const Out& o, const Held& H, int lastClosed, TMark& out)
{
    const int n = (int)c.size(); if (H.bar < 0 || H.bar >= n || H.side == 0 || (int)o.fb.size() != n || (int)o.fs.size() != n) return false;
    for (int e = std::max(0, H.bar - T_BEFORE); e <= H.bar; ++e) {
        const double x = H.side < 0 ? o.fb[e] : o.fs[e]; if (!(x >= T_EXH)) continue;
        for (int j = e + 1; j <= std::min(e + T_AFTER, lastClosed); ++j) {
            if (H.side < 0 ? c[j] < H.px : c[j] > H.px) { out.bar = std::max(j, H.bar); out.side = H.side; out.exhBar = e; out.exh = x; return true; }
        }
    }
    return false;
}
}  // namespace ex
#endif
