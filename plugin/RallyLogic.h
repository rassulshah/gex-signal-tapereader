// RallyLogic.h 1.0.0 (2026-10-22) -- the "Rly" (rally) signal, SDK-free. Rassul approved mockup v0.2 (rly-mockup-v2.png) at
// 15:10-15:15 CT 2026-10-22 ("keep 4 a day as it is now"). Exact port of the research twin rly2.py session() (v0.2):
//   - RTH bars only (svl::rthOf: ES / NQ 08:30, CL / NG 08:00, GC / HG / 6E 07:20 CT), the chart's own bars (3-min).
//   - Session VWAP from the RTH open on the typical price (H + L + C) / 3 x volume (volume floored at 1), SD bands +-1 / +-2
//     (volume-weighted variance = sum(tp^2 v) / sum(v) - vwap^2).
//   - vwapTrend from the 10th RTH bar over the last up-to-20 bars (L): maj = L / 2 + 1, strong = max(round(0.55 L), maj).
//     Strong Uptrend = closes > +1 SD >= strong and no low <= VWAP; Uptrend = any high >= +2 SD OR (closes > VWAP >= maj and no
//     low <= -1 SD); mirror down. Both up and down: the MORE RECENT +-2 SD tag decides (no tag on one side = Balance).
//   - 1-bar ZigZag over the session's bars (green / red tie rules as rly2.py).
//   - Signal on the END of a CONFIRMED wave (decided on the bar where the ZigZag turns): wave pivot p -> extreme e with
//     e - p >= 3, size >= 2.5 x ATR(14 Wilder, chart bars incl. overnight) at e, a bar of the wave tagged +1 SD (high >= u1),
//     high e = the 20-bar high (session bars), trend at e Uptrend / Strong Uptrend. Mirror for the down side (-1 SD, 20-bar low,
//     Dntrend / Strong Dntrend). Drawn "Rly" on bar e: green above (up) / red below (down). Never changes afterwards: the host
//     records every judged wave and never judges a recorded wave again.
// Causal: every value at bar i uses bars <= i only, so a wave judged live equals the same wave judged after a restart.
#pragma once
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "SessionVWAPLogic.h"

namespace rly {

static const char* const VERSION = "1.0.0";
static const int MIN_BARS = 3;          // e - p
static const double ATR_MULT = 2.5;
static const int ATR_LEN = 14;
static const int EXT_BARS = 20;         // 20-bar high / low (incl. bar e)
static const int TREND_FROM = 9;        // trend from the 10th RTH bar
static const int TREND_WIN = 20;

// one chart bar, stamped at its END like svl::Bar (days = civil day of the close stamp, endSec = second of that day)
struct Bar { long days = 0; int endSec = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };

// trend codes: +2 Strong Uptrend, +1 Uptrend, 0 Balance / none, -1 Dntrend, -2 Strong Dntrend
inline const char* trendName(int t) { return t == 2 ? "Strong Uptrend" : t == 1 ? "Uptrend" : t == -1 ? "Dntrend" : t == -2 ? "Strong Dntrend" : "Balance"; }

// Wilder ATR as pandas tr.ewm(alpha=1/14, adjust=False).mean() with tr[0] = NaN (no prior close): atr[0] = NaN, atr[1] = tr[1]
inline void atrWilder(const std::vector<Bar>& B, std::vector<double>& atr) {
    const size_t n = B.size(); atr.assign(n, NAN);
    const double a = 1.0 / ATR_LEN; bool have = false; double y = 0;
    for (size_t i = 1; i < n; ++i) {
        const double pc = B[i - 1].c;
        double tr = B[i].h - B[i].l; const double x1 = std::fabs(B[i].h - pc), x2 = std::fabs(B[i].l - pc);
        if (x1 > tr) { tr = x1; }
        if (x2 > tr) { tr = x2; }
        if (!std::isfinite(tr)) { atr[i] = have ? y : NAN; continue; }
        y = have ? (1.0 - a) * y + a * tr : tr; have = true; atr[i] = y;
    }
}

// one session's RTH bars with VWAP / bands / trend / ZigZag (indices into the session; g = the chart index)
struct Sess {
    long sess = 0; std::vector<int> g;
    std::vector<double> vw, sd, u1, d1, u2, d2; std::vector<int> trend, dir;
};

inline int roundHalfEven(double x) { const double f = std::floor(x), r = x - f; if (r > 0.5) return (int)f + 1; if (r < 0.5) return (int)f; return ((long long)f % 2 == 0) ? (int)f : (int)f + 1; }

inline void bands(const std::vector<Bar>& B, Sess& S) {
    const size_t n = S.g.size(); S.vw.assign(n, 0); S.sd.assign(n, 0); S.u1 = S.d1 = S.u2 = S.d2 = S.vw;
    double cv = 0, ctv = 0, ct2v = 0;
    for (size_t k = 0; k < n; ++k) {
        const Bar& b = B[(size_t)S.g[k]]; const double tp = (b.h + b.l + b.c) / 3.0; const double v = b.v < 1 ? 1.0 : b.v;
        cv += v; ctv += tp * v; ct2v += (tp * tp) * v;
        const double vw = ctv / cv; double var = ct2v / cv - vw * vw; if (var < 0) var = 0; const double sd = std::sqrt(var);
        S.vw[k] = vw; S.sd[k] = sd; S.u1[k] = vw + sd; S.d1[k] = vw - sd; S.u2[k] = vw + 2 * sd; S.d2[k] = vw - 2 * sd;
    }
}

inline void trends(const std::vector<Bar>& B, Sess& S) {
    const int n = (int)S.g.size(); S.trend.assign((size_t)n, 0);
    for (int i = TREND_FROM; i < n; ++i) {
        const int a = i - (TREND_WIN - 1) > 0 ? i - (TREND_WIN - 1) : 0, L = i - a + 1, maj = L / 2 + 1;
        int strong = roundHalfEven(L * 0.55); if (strong < maj) strong = maj;
        int cU1 = 0, cD1 = 0, cUv = 0, cDv = 0, lu = -1, ld = -1; bool lowVw = false, hiVw = false, lowD1 = false, hiU1 = false;
        for (int k = a; k <= i; ++k) {
            const Bar& b = B[(size_t)S.g[(size_t)k]];
            const size_t K = (size_t)k;
            cU1 += b.c > S.u1[K] ? 1 : 0; cD1 += b.c < S.d1[K] ? 1 : 0;
            cUv += b.c > S.vw[K] ? 1 : 0; cDv += b.c < S.vw[K] ? 1 : 0;
            lowVw = lowVw || b.l <= S.vw[K]; hiVw = hiVw || b.h >= S.vw[K];
            lowD1 = lowD1 || b.l <= S.d1[K]; hiU1 = hiU1 || b.h >= S.u1[K];
            lu = b.h >= S.u2[K] ? k : lu; ld = b.l <= S.d2[K] ? k : ld;
        }
        const bool upS = cU1 >= strong && !lowVw, dnS = cD1 >= strong && !hiVw;
        bool up = lu >= 0 || (cUv >= maj && !lowD1), dn = ld >= 0 || (cDv >= maj && !hiU1);
        if (up && dn && lu >= 0 && ld >= 0) { const bool u = lu > ld, d = ld > lu; up = u; dn = d; }
        S.trend[(size_t)i] = upS ? 2 : dnS ? -2 : (up && !dn) ? 1 : (dn && !up) ? -1 : 0;
    }
}

struct Pivot { int i; int type; };   // session index, +1 = high pivot (end of an up-wave), -1 = low pivot

// rly2.py's 1-bar ZigZag: dir[0] = 0, pivots incl. the starting pseudo-pivot at 0 and the open extreme at the end
inline void zigzag(const std::vector<Bar>& B, Sess& S, std::vector<Pivot>& piv) {
    const int n = (int)S.g.size(); S.dir.assign((size_t)n, 0); piv.clear(); if (n == 0) return;
    auto X = [&](int k) -> const Bar& { return B[(size_t)S.g[(size_t)k]]; };
    int st = X(0).c >= X(0).o ? 1 : -1, ext = 0; piv.push_back({ 0, -st });
    for (int i = 1; i < n; ++i) {
        const bool hh = X(i).h > X(i - 1).h, ll = X(i).l < X(i - 1).l;
        if (st == 1) {
            if (ll && !(hh && X(i).c >= X(i).o)) { st = -1; piv.push_back({ ext, 1 }); ext = i; }
            else if (X(i).h >= X(ext).h) ext = i;
        } else {
            if (hh && !(ll && X(i).c < X(i).o)) { st = 1; piv.push_back({ ext, -1 }); ext = i; }
            else if (X(i).l <= X(ext).l) ext = i;
        }
        S.dir[(size_t)i] = st;
    }
    piv.push_back({ ext, st });
}

// one judged (confirmed) wave
struct Wave {
    long sess = 0; int dir = 0;            // +1 up-wave (bullish Rly), -1 down-wave (bearish)
    int gp = -1, ge = -1, gc = -1;         // chart indices of the pivot p, the extreme e (the Rly bar) and the confirming bar
    int nb = 0; double size = 0, atr = 0, ratio = 0; int trend = 0;
    double from = 0, to = 0;               // low p -> high e (up) / high p -> low e (down)
    double vw = 0, band1 = 0;              // VWAP and the +1 SD (up) / -1 SD (down) band at e
    bool c[5] = { false, false, false, false, false };   // 3+ bars, 2.5 ATR, tag 1 SD, 20-bar extreme, trend
    bool fired = false;
};
static const char* const COND_NAMES[5] = { "3+ bars", "2.5 ATR", "tag 1SD", "20-bar ext", "trend" };

// judge every confirmed wave of one session (closed bars only)
inline void judge(const std::vector<Bar>& B, const std::vector<double>& atr, Sess& S, std::vector<Wave>& out) {
    bands(B, S); trends(B, S); std::vector<Pivot> P; zigzag(B, S, P);
    const int n = (int)S.g.size();
    auto X = [&](int k) -> const Bar& { return B[(size_t)S.g[(size_t)k]]; };
    for (size_t q = 0; q + 1 < P.size(); ++q) {
        const int p = P[q].i, e = P[q + 1].i;
        if (e == n - 1 && q + 1 == P.size() - 1) continue;          // last wave: not confirmed
        int conf = -1; for (int k = e + 1; k < n; ++k) if (S.dir[(size_t)k] != S.dir[(size_t)e]) { conf = k; break; }
        if (conf < 0) continue;
        Wave W; W.sess = S.sess; W.nb = e - p; W.gp = S.g[(size_t)p]; W.ge = S.g[(size_t)e]; W.gc = S.g[(size_t)conf];
        W.atr = atr[(size_t)W.ge]; W.trend = S.trend[(size_t)e];
        const int a20 = e - (EXT_BARS - 1) > 0 ? e - (EXT_BARS - 1) : 0;
        if (P[q + 1].type == 1) {
            W.dir = 1; W.from = X(p).l; W.to = X(e).h; W.size = W.to - W.from;
            bool tag = false; for (int k = p; k <= e; ++k) if (X(k).h >= S.u1[(size_t)k]) { tag = true; break; }
            double mx = X(a20).h; for (int k = a20; k <= e; ++k) if (X(k).h > mx) mx = X(k).h;
            W.c[2] = tag; W.c[3] = X(e).h >= mx; W.c[4] = W.trend >= 1; W.band1 = S.u1[(size_t)e];
        } else {
            W.dir = -1; W.from = X(p).h; W.to = X(e).l; W.size = W.from - W.to;
            bool tag = false; for (int k = p; k <= e; ++k) if (X(k).l <= S.d1[(size_t)k]) { tag = true; break; }
            double mn = X(a20).l; for (int k = a20; k <= e; ++k) if (X(k).l < mn) mn = X(k).l;
            W.c[2] = tag; W.c[3] = X(e).l <= mn; W.c[4] = W.trend <= -1; W.band1 = S.d1[(size_t)e];
        }
        W.c[0] = W.nb >= MIN_BARS; W.c[1] = std::isfinite(W.atr) && W.size >= ATR_MULT * W.atr;
        W.ratio = std::isfinite(W.atr) && W.atr > 0 ? W.size / W.atr : 0; W.vw = S.vw[(size_t)e];
        W.fired = W.c[0] && W.c[1] && W.c[2] && W.c[3] && W.c[4];
        out.push_back(W);
    }
}

// every RTH session of the chart from chart index `from` on (B = CLOSED bars only), per = bar seconds
inline std::vector<Wave> run(const std::vector<Bar>& B, int per, int openMin, int closeMin, int from = 0) {
    std::vector<Wave> out; std::vector<double> atr; atrWilder(B, atr);
    Sess cur; bool open = false;
    auto flush = [&]() { if (open && !cur.g.empty()) judge(B, atr, cur, out); cur = Sess(); open = false; };
    for (int i = 0; i < (int)B.size(); ++i) {
        const svl::Cls c = svl::classify(B[(size_t)i].days, B[(size_t)i].endSec, per, openMin, closeMin);
        if (c.kind != svl::RTH) { if (open && c.sess != cur.sess) flush(); continue; }
        if (open && c.sess != cur.sess) flush();
        if (!open) { cur.sess = c.sess; open = true; }
        cur.g.push_back(i);
    }
    flush();
    if (from > 0) { std::vector<Wave> k; for (const Wave& w : out) if (w.gc >= from) k.push_back(w); out.swap(k); }
    return out;
}

// ---- record (lsFlexLevels\Rly\<MKT>-<CONTRACT>-<spb>-<session>.csv): every judged wave, fired or not, one line, never rewritten
static const char* const REC_HEADER = "RLY|t_e|t_p|t_conf|dir|fired|nb|size|atr|ratio|trend|from|to|vwap|band1|failed|version";
inline std::string fmtD(double x, int d) { char b[48]; std::snprintf(b, sizeof b, "%.*f", d, std::isfinite(x) ? x : 0.0); return b; }
inline std::string failed(const Wave& W) { std::string s; for (int k = 0; k < 5; ++k) if (!W.c[k]) { if (!s.empty()) s += ";"; s += COND_NAMES[k]; } return s.empty() ? "-" : s; }
inline std::string line(const Wave& W, long long te, long long tp, long long tc, const char* ver) {
    return "RLY|" + std::to_string(te) + "|" + std::to_string(tp) + "|" + std::to_string(tc) + "|" + std::to_string(W.dir) + "|" + (W.fired ? "1" : "0")
        + "|" + std::to_string(W.nb) + "|" + fmtD(W.size, 4) + "|" + fmtD(W.atr, 4) + "|" + fmtD(W.ratio, 2) + "|" + std::to_string(W.trend)
        + "|" + fmtD(W.from, 4) + "|" + fmtD(W.to, 4) + "|" + fmtD(W.vw, 4) + "|" + fmtD(W.band1, 4) + "|" + failed(W) + "|" + ver;
}
// a recorded wave (what the chart redraws)
struct Rec { long long te = 0, tp = 0, tc = 0; int dir = 0; bool fired = false; int nb = 0; double size = 0, atr = 0, ratio = 0; int trend = 0; double from = 0, to = 0, vw = 0, band1 = 0; };
inline bool parseLine(const std::string& s0, Rec& R) {
    std::string s = s0; if (!s.empty() && s.back() == '\r') s.pop_back();
    if (s.compare(0, 4, "RLY|") != 0 || s.compare(0, 6, "RLY|t_") == 0) return false;
    std::vector<std::string> f; size_t a = 0; for (;;) { const size_t b = s.find('|', a); f.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    if (f.size() != 17) return false;
    auto ll = [](const std::string& x, long long& v) { if (x.empty()) return false; char* e = nullptr; v = std::strtoll(x.c_str(), &e, 10); return e && *e == 0; };
    auto dd = [](const std::string& x, double& v) { if (x.empty()) return false; char* e = nullptr; v = std::strtod(x.c_str(), &e); return e && *e == 0 && std::isfinite(v); };
    long long d = 0, fi = 0, nb = 0, tr = 0;
    if (!ll(f[1], R.te) || !ll(f[2], R.tp) || !ll(f[3], R.tc) || !ll(f[4], d) || !ll(f[5], fi) || !ll(f[6], nb) || !ll(f[10], tr)) return false;
    if (!dd(f[7], R.size) || !dd(f[8], R.atr) || !dd(f[9], R.ratio) || !dd(f[11], R.from) || !dd(f[12], R.to) || !dd(f[13], R.vw) || !dd(f[14], R.band1)) return false;
    if ((d != 1 && d != -1) || (fi != 0 && fi != 1) || nb < 0 || nb > 100000 || tr < -2 || tr > 2 || R.te <= 0) return false;
    R.dir = (int)d; R.fired = fi == 1; R.nb = (int)nb; R.trend = (int)tr;
    return true;
}
inline Rec toRec(const Wave& W, long long te, long long tp, long long tc) {
    Rec R; R.te = te; R.tp = tp; R.tc = tc; R.dir = W.dir; R.fired = W.fired; R.nb = W.nb; R.size = W.size; R.atr = W.atr; R.ratio = W.ratio;
    R.trend = W.trend; R.from = W.from; R.to = W.to; R.vw = W.vw; R.band1 = W.band1; return R;
}

// the hover (approved mockup v0.2 wording); clock = "9:24 AM", px() formats a price
inline std::vector<std::string> hover(const Rec& R, const std::string& clock, const std::string& fromPx, const std::string& toPx, const std::string& sizePx) {
    std::vector<std::string> L; const bool up = R.dir > 0;
    L.push_back(std::string("Rly - confirmed ") + (up ? "up" : "down") + "-wave " + std::to_string(R.nb) + " bars, " + fmtD(R.ratio, 1) + " ATR, tagged "
                + (up ? "+1 SD, 20-bar high, " : "-1 SD, 20-bar low, ") + trendName(R.trend) + "   (" + clock + ")");
    L.push_back(up ? "Question: is this up-move a real trend rally I can join?" : "Question: is this down-move a real trend sell-off I can join?");
    L.push_back(std::string("What it is: a 1-bar ZigZag ") + (up ? "up" : "down") + "-wave of 3+ bars and 2.5+ ATR that reaches VWAP " + (up ? "+1 SD and makes a 20-bar high" : "-1 SD and makes a 20-bar low")
                + " while vwapTrend is " + (up ? "Uptrend." : "Dntrend."));
    L.push_back(up ? "Why it matters: in a trend day the rallies that tag +1 SD at a new 20-bar high show buyers in control; the pullback after it is the with-trend entry zone."
                   : "Why it matters: in a trend day the drops that tag -1 SD at a new 20-bar low show sellers in control; the bounce after it is the with-trend entry zone.");
    L.push_back(up ? "How to use: don't chase the Rly bar; wait for the pullback to VWAP / +1 SD and a bullish trigger (e.g. CoB). Trend only - never fade it."
                   : "How to use: don't chase the Rly bar; wait for the bounce to VWAP / -1 SD and a bearish trigger (e.g. CoB). Trend only - never fade it.");
    L.push_back("Example: this wave from " + fromPx + " to " + toPx + " = " + sizePx + " pts = " + fmtD(R.ratio, 1) + " x ATR (ATR " + fmtD(R.atr, 2) + "), " + std::to_string(R.nb) + " bars.");
    return L;
}

} // namespace rly
