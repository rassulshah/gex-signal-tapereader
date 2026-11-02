// HodLodHtf.h - lsHodLod 2.17.0 (2026-11-02, Rassul 14:19-14:31 CT, mockup "HTF Trend Mockup" v2 + "start") - SDK-free:
// the HIGHER-TIMEFRAME TREND and RELATIVE STRENGTH line under the HOD / LOD box header, from the chart's own CLOSED bars (no repaint
// inside a bar; updates at each bar close):
//   60m / Day TREND = Kaufman efficiency x 100, signed: (last close - close N bars back) / the sum of |close-to-close| moves over those
//                    N bars; N = 20 hourly bars / 10 sessions. Up / Dn by the sign; Strong >= 40, Moderate 20-39, Weak < 20.
//   RS (vs itself) = RSI 14 (Wilder) of the same hourly / session closes; Strong >= 60, Weak <= 40, Neutral between;
//                    >= 70 Overbought, <= 30 Oversold.
//   60m bars = clock hours by bar END (a bar ending 10:03 .. 11:00 is the 11:00 hour); sessions start 17:00 CT (Sun-Thu evening).
// Too few bars for a value = "-" for that part (the chart holds ~21 calendar days = ~15 sessions).
#ifndef HODLOD_HTF_H
#define HODLOD_HTF_H
#include <climits>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace htf {
static const char* const VERSION = "1.0.0";
static const int N_HOUR = 20, N_DAY = 10, RSI_N = 14;
static const double TR_STRONG = 40, TR_MOD = 20, RS_STRONG = 60, RS_WEAK = 40, RS_OB = 70, RS_OS = 30;
struct In { long long absEnd; double c; };   // bar END (local CT seconds since 1970) and its close
struct Out { bool hOk = false, dOk = false, hRsOk = false, dRsOk = false; double hEr = 0, dEr = 0, hRsi = 0, dRsi = 0; int nH = 0, nD = 0; };

// the closes of each hour / session (the last bar's close in it), oldest first
inline std::vector<double> hourCloses(const std::vector<In>& b) {
    std::vector<double> o; long long cur = LLONG_MIN;
    for (const In& x : b) { const long long k = (x.absEnd - 1) / 3600; if (k != cur) { o.push_back(x.c); cur = k; } else o.back() = x.c; }
    return o;
}
inline std::vector<double> sessionCloses(const std::vector<In>& b) {
    std::vector<double> o; long long cur = LLONG_MIN;
    for (const In& x : b) { const long long k = (x.absEnd - 1 + 7 * 3600) / 86400; if (k != cur) { o.push_back(x.c); cur = k; } else o.back() = x.c; }   // 17:00 CT = the next day's session
    return o;
}
inline bool efficiency(const std::vector<double>& c, int n, double& er) {
    if ((int)c.size() < n + 1) return false;
    const size_t a = c.size() - 1 - (size_t)n; double path = 0;
    for (size_t i = a + 1; i < c.size(); ++i) path += std::fabs(c[i] - c[i - 1]);
    er = path > 0 ? (c.back() - c[a]) / path : 0; return true;
}
inline bool rsi(const std::vector<double>& c, int n, double& r) {
    if ((int)c.size() < n + 1) return false;
    double au = 0, ad = 0;
    for (int i = 1; i <= n; ++i) { const double d = c[(size_t)i] - c[(size_t)i - 1]; if (d > 0) au += d; else ad -= d; }
    au /= n; ad /= n;
    for (size_t i = (size_t)n + 1; i < c.size(); ++i) { const double d = c[i] - c[i - 1]; au = (au * (n - 1) + (d > 0 ? d : 0)) / n; ad = (ad * (n - 1) + (d < 0 ? -d : 0)) / n; }
    r = ad > 0 ? 100.0 - 100.0 / (1.0 + au / ad) : (au > 0 ? 100.0 : 50.0); return true;
}
inline Out compute(const std::vector<In>& b) {
    Out o; const std::vector<double> h = hourCloses(b), d = sessionCloses(b); o.nH = (int)h.size(); o.nD = (int)d.size();
    o.hOk = efficiency(h, N_HOUR, o.hEr); o.dOk = efficiency(d, N_DAY, o.dEr);
    o.hRsOk = rsi(h, RSI_N, o.hRsi); o.dRsOk = rsi(d, RSI_N, o.dRsi);
    return o;
}
// display pieces: "Dn 3 Weak", "RS 37 Weak"; dir +1 up (green) / -1 down (red) / 0 neutral (grey)
inline std::string trendText(double er, int& dir) {
    const int s = (int)std::lround(std::fabs(er) * 100.0); dir = er > 0 && s > 0 ? 1 : er < 0 && s > 0 ? -1 : 0;
    const char* q = s >= TR_STRONG ? "Strong" : s >= TR_MOD ? "Moderate" : "Weak";
    char b[40]; std::snprintf(b, sizeof b, "%s %d %s", dir > 0 ? "Up" : dir < 0 ? "Dn" : "Flat", s, q); return b;
}
inline std::string rsText(double r, int& dir) {
    const int s = (int)std::lround(r); dir = s >= RS_STRONG ? 1 : s <= RS_WEAK ? -1 : 0;
    const char* q = s >= RS_OB ? "Overbought" : s <= RS_OS ? "Oversold" : s >= RS_STRONG ? "Strong" : s <= RS_WEAK ? "Weak" : "Neutral";
    char b[40]; std::snprintf(b, sizeof b, "RS %d %s", s, q); return b;
}
}  // namespace htf
#endif
