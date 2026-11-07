// HodLodRange.h -- lsHodLod 2.19.0 (2026-11-06, Rassul 14:54-15:11 CT "how do I identify if the market will have a big move today",
// "at the open and then 30 min later ... hold cob longer ... buy pullbacks", "keep it compact and near the gamma regime"). SDK-free.
// The Range Day pill in the header, beside the gamma pill: P(big RTH day) / P(small RTH day) from the nightly logistic models in
// lsFlexLevels\LRA-RangeDay-<MKT>.csv (lra.range_day). Inputs, computed natively (View rd*): overnight range / ATR14, prior RTH range /
// ATR14, |gap| / ATR14 (roll > 4 ATR = unknown), news day (HodLodEvents.h), and from open + 30 min the first-30-minute range / ATR14.
// Before 30 min: the pre-open read with "?"; after: the 30-minute read, final for the day. Big = P(big) >= PILL big; Small = P(small) >=
// PILL small; else Norm. No file / no ATR14 (14 prior sessions) = no pill.
#pragma once
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
namespace hrd {
static const char* const VERSION = "1.1.0";   // 1.1.0: + REG = the expected RTH range / a normal day ("Exp Range", Rassul 16:30 CT)
static const size_t MAX_BYTES = 16384;
struct Md { bool ok = false, filled = false; double b0 = 0; double w[5] = { 0, 0, 0, 0, 0 }; double fill[5] = { 0, 0, 0, 0, 0 }; int n = 0; };
struct Reg { bool ok = false, show = false; double b0 = 0; double w[5] = { 0, 0, 0, 0, 0 }; int n = 0; };
struct File { bool ok = false; std::string why; Md m[2][2]; Reg reg[2]; double tBig = 0.45, tSmall = 0.50; };   // reg[pre / post]: optional   // m[pre=0 / post=1][big=0 / small=1]
inline std::vector<std::string> split(const std::string& s) { std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t q = s.find(',', a); f.push_back(s.substr(a, q == std::string::npos ? std::string::npos : q - a)); if (q == std::string::npos) break; a = q + 1; } return f; }
inline bool num(const std::string& s, double& v) { if (s.empty()) return false; char* e = nullptr; v = std::strtod(s.c_str(), &e); return e && *e == 0 && std::isfinite(v); }
inline File parse(const std::string& text, const std::string& mkt) {
    File F; bool mk = false; size_t a = 0;
    while (a < text.size()) {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        std::string l = text.substr(a, b - a); a = b + 1; if (!l.empty() && l.back() == '\r') l.pop_back();
        const std::vector<std::string> f = split(l); if (f.empty()) continue;
        if (f[0] == "MARKET" && f.size() >= 2) mk = f[1] == mkt;
        else if (f[0] == "PILL" && f.size() >= 3) { double x, y; if (num(f[1], x) && num(f[2], y) && x > 0 && x < 1 && y > 0 && y < 1) { F.tBig = x; F.tSmall = y; } }
        else if (f[0] == "REG" && f.size() >= 8) {   // REG,<pre|post>,<show 0/1>,b0,w... (1.1.0); a bad line just leaves no Exp Range
            const int t = f[1] == "pre" ? 0 : f[1] == "post" ? 1 : -1; if (t < 0) continue;
            const int need = t == 0 ? 4 : 5; if ((int)f.size() != 4 + need) continue;
            Reg R; double v; bool good = num(f[3], v); R.b0 = v; R.show = f[2] == "1";
            for (int i = 0; i < need && good; i++) { good = num(f[(size_t)(4 + i)], v); R.w[i] = v; }
            if (good) { R.n = need; R.ok = true; F.reg[t] = R; }
        }
        else if ((f[0] == "MODEL" || f[0] == "FILL") && f.size() >= 7) {
            const int t = f[1] == "pre" ? 0 : f[1] == "post" ? 1 : -1, k = f[2] == "big" ? 0 : f[2] == "small" ? 1 : -1;
            if (t < 0 || k < 0) continue;
            const int need = t == 0 ? 4 : 5; Md& M = F.m[t][k]; double v;
            if (f[0] == "MODEL") { if ((int)f.size() != 4 + need || !num(f[3], v)) { F.why = "bad MODEL line"; return F; } M.b0 = v;
                for (int i = 0; i < need; i++) { if (!num(f[(size_t)(4 + i)], v)) { F.why = "bad MODEL weight"; return F; } M.w[i] = v; } M.n = need; M.ok = true; }
            else { if ((int)f.size() != 3 + need) { F.why = "bad FILL line"; return F; }
                for (int i = 0; i < need; i++) { if (!num(f[(size_t)(3 + i)], v)) { F.why = "bad FILL value"; return F; } M.fill[i] = v; } M.filled = true; }
        }
    }
    if (!mk) { F.why = "MARKET line missing or not " + mkt; return F; }
    for (int t = 0; t < 2; t++) for (int k = 0; k < 2; k++) if (!F.m[t][k].ok || !F.m[t][k].filled) { F.why = "a model or its fill line is missing"; return F; }
    F.ok = true; return F;
}
inline double prob(const Md& M, const double* x) {
    double z = M.b0; for (int i = 0; i < M.n; i++) z += M.w[i] * (std::isfinite(x[i]) ? x[i] : M.fill[i]);
    return 1.0 / (1.0 + std::exp(-z));
}
struct In { bool ok = false; double on = NAN, pr = NAN, gap = NAN, news = 0, x30 = NAN; };
struct Out { std::string s; int kind = -1; bool post = false; double pBig = NAN, pSmall = NAN, xr = NAN; };   // xr = expected RTH range / a normal day (NaN = not shown)   // kind 1 big, 2 small, 0 norm, -1 none
inline Out read(const File& F, const In& I) {
    Out o; if (!F.ok || !I.ok) return o;
    o.post = std::isfinite(I.x30);
    const double x[5] = { I.on, I.pr, I.gap, I.news, I.x30 }; const int t = o.post ? 1 : 0;
    o.pBig = prob(F.m[t][0], x); o.pSmall = prob(F.m[t][1], x);
    {   const Reg& R = F.reg[t];   // (1.1.0) missing inputs take the big model's fill values
        if (R.ok && R.show) { double z = R.b0; for (int i = 0; i < R.n; i++) z += R.w[i] * (std::isfinite(x[i]) ? x[i] : F.m[t][0].fill[i]); o.xr = std::exp(z); } }
    const char* q = o.post ? "" : "?"; char b[40];
    if (o.pBig >= F.tBig) { o.kind = 1; std::snprintf(b, sizeof b, "Big%s %d%%", q, (int)std::lround(o.pBig * 100)); }
    else if (o.pSmall >= F.tSmall) { o.kind = 2; std::snprintf(b, sizeof b, "Small%s %d%%", q, (int)std::lround(o.pSmall * 100)); }
    else { o.kind = 0; std::snprintf(b, sizeof b, "Norm%s", q); }
    o.s = b; return o;
}
}   // namespace hrd
