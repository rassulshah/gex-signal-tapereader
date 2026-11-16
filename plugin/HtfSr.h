// HtfSr.h - the 3-hour S / R (R1 R2 S1 S2) from a chart's own bars. SDK-free, shared by lsLimitProfile 2.66.0 and lsHtfSR 1.0.0.
// (2026-11-15 Rassul 20:33-20:35 CT "do you look for liquidity around those levels on the 3 min on the limit profile ?" ... "we look for
//  liquidity around key levels right ?" ... "yes"; 20:55 "just build them . regarding limit profile, i will go with your recommendation")
// The SAME walk as lsCobSignal 1.7.x (CobSignalLogic.h struct Htf, his setHrlyLvls): tests_htfsr proves the two give identical values
// on every bar of the recorded charts.
//   3-hour bars from 5:00 PM CT (17-20, 20-23, 23-2, 2-5, 5-8, 8-11, 11-14, 14-17), built from the chart's bars; the forming bar counts.
//   ZigZag  his 1-bar High / Low ZigZag as IRT walks it (a bar that makes both a new high and a lower low keeps the extreme).
//   R1 = the last confirmed pivot high, R2 = the higher of the last two;  S1 = the last confirmed pivot low, S2 = the lower of the last two.
//   standing = no COMPLETED 3-hour bar since that pivot has closed beyond the level. A broken level is not a level any more.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace hsr {
static const char* const VERSION = "1.0.1";   // = the lsHtfSR setVersion literal
static const int SECS = 10800;
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
struct Bar { long long t = 0; double h = 0, l = 0, c = 0; };   // t = the bar END, local wall-clock seconds (any epoch that starts on a local midnight)
struct HBar { long long k = 0; double h = 0, l = 0, c = 0; };
struct Snap { double r1 = NAN, r2 = NAN, s1 = NAN, s2 = NAN, ext = 0; int dir = 0; bool r1ok = false, r2ok = false, s1ok = false, s2ok = false; };
inline bool periodOk(int perSec) { return perSec > 0 && perSec <= SECS && SECS % perSec == 0; }   // a 3-hour chart walks its own bars
struct Walk {
    std::vector<HBar> HB; int d = 0, ext = 0; std::vector<std::pair<int, int>> piv;   // piv = (bar, +1 high / -1 low), confirmed by completed bars
    void reset() { HB.clear(); d = 0; ext = 0; piv.clear(); }
    static void step(const std::vector<HBar>& H, int i, int& d, int& ext, std::vector<std::pair<int, int>>& out) {
        if (i < 1 || i >= (int)H.size()) return;
        if (d == 0) { d = H[1].h >= H[0].h ? 1 : -1; ext = 0; }
        const HBar& b = H[(size_t)i]; const HBar& p = H[(size_t)i - 1];
        if (d == 1) { if (b.h > H[(size_t)ext].h) ext = i; else if (b.l < p.l && i != ext) { out.push_back(std::make_pair(ext, 1)); ext = i; d = -1; } }
        else        { if (b.l < H[(size_t)ext].l) ext = i; else if (b.h > p.h && i != ext) { out.push_back(std::make_pair(ext, -1)); ext = i; d = 1; } }
    }
    Snap add(const Bar& b, int perSec) {
        Snap o;
        if (!periodOk(perSec) || !std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(b.c)) return o;
        // 17:00 = 7200 s past a 3-hour boundary of the day. (review) On a 3-hour chart every chart bar IS a 3-hour bar - also the short
        // last one of the session (14:00 - 16:00), which a clock bucket would merge into the bar before it.
        const long long k = perSec == SECS ? (HB.empty() ? 0 : HB.back().k + 1) : floorDiv(b.t - perSec - 7200, SECS);
        if (HB.empty() || k != HB.back().k) {
            if (!HB.empty()) step(HB, (int)HB.size() - 1, d, ext, piv);       // the bar that just completed joins the walk
            HBar n; n.k = k; n.h = b.h; n.l = b.l; n.c = b.c; HB.push_back(n);
        } else { HBar& n = HB.back(); n.h = std::max(n.h, b.h); n.l = std::min(n.l, b.l); n.c = b.c; }
        const int last = (int)HB.size() - 1; int d2 = d, e2 = ext; std::vector<std::pair<int, int>> extra;
        step(HB, last, d2, e2, extra);                                         // the forming bar, as the live chart walks it
        if (d2 == 0) return o;
        o.dir = d2; o.ext = d2 > 0 ? HB[(size_t)e2].h : HB[(size_t)e2].l;
        int hi[2] = { -1, -1 }, lo[2] = { -1, -1 }, nh = 0, nl = 0;
        auto take = [&](const std::pair<int, int>& q) { if (q.second > 0) { if (nh < 2) hi[nh++] = q.first; } else { if (nl < 2) lo[nl++] = q.first; } };
        for (size_t q = extra.size(); q-- > 0;) take(extra[q]);
        for (size_t q = piv.size(); q-- > 0 && (nh < 2 || nl < 2);) take(piv[q]);
        auto stand = [&](int pb, double L, int sd) { for (int q = pb + 1; q < last; ++q) if (sd > 0 ? HB[(size_t)q].c > L : HB[(size_t)q].c < L) return false; return true; };
        if (nh >= 1) { o.r1 = HB[(size_t)hi[0]].h; o.r1ok = stand(hi[0], o.r1, 1); int b2 = hi[0]; if (nh >= 2 && HB[(size_t)hi[1]].h > o.r1) b2 = hi[1]; o.r2 = HB[(size_t)b2].h; o.r2ok = stand(b2, o.r2, 1); }
        if (nl >= 1) { o.s1 = HB[(size_t)lo[0]].l; o.s1ok = stand(lo[0], o.s1, -1); int b2 = lo[0]; if (nl >= 2 && HB[(size_t)lo[1]].l < o.s1) b2 = lo[1]; o.s2 = HB[(size_t)b2].l; o.s2ok = stand(b2, o.s2, -1); }
        return o;
    }
};

// the levels a chart shows: only the STANDING ones; R2 / S2 only where they are a different price from R1 / S1
struct Lv { std::string name; double px = 0; int side = 0; };   // side +1 resistance, -1 support
inline std::vector<Lv> standing(const Snap& s, double tick)
{
    std::vector<Lv> v; const double eps = (tick > 0 ? tick : 0.01) * 0.25;
    auto put = [&](const char* nm, double x, bool ok, double base, bool baseOk, int side) {
        if (!ok || !std::isfinite(x) || x <= 0) return;
        if (baseOk && std::isfinite(base) && std::fabs(x - base) < eps) return;
        Lv q; q.name = nm; q.px = tick > 0 ? std::round(x / tick) * tick : x; q.side = side; v.push_back(q);
    };
    put("R1", s.r1, s.r1ok, NAN, false, 1); put("R2", s.r2, s.r2ok, s.r1, s.r1ok, 1);
    put("S1", s.s1, s.s1ok, NAN, false, -1); put("S2", s.s2, s.s2ok, s.s1, s.s1ok, -1);
    return v;
}
inline bool isName(const std::string& t) { return t == "R1" || t == "R2" || t == "S1" || t == "S2"; }
// "R1" / "R1/S2" = true; "ONH/R1" = false (a named key level is on that price too); "" = false
inline bool onlyHtf(const std::string& name)
{
    if (name.empty()) return false;
    size_t a = 0;
    while (a <= name.size()) { size_t b = name.find('/', a); if (b == std::string::npos) b = name.size(); if (!isName(name.substr(a, b - a))) return false; a = b + 1; }
    return true;
}
inline bool hasHtf(const std::string& name)
{
    size_t a = 0;
    while (a <= name.size()) { size_t b = name.find('/', a); if (b == std::string::npos) b = name.size(); if (isName(name.substr(a, b - a))) return true; a = b + 1; }
    return false;
}
// into a level list M{name, price, pivotOnly}: a level on the exact tick of a named level joins its name last ("ONH/R1"), never the VWAP's;
// otherwise it is its own level. Returns how many were added or joined.
template <class M> inline int addTo(std::vector<M>& V, const std::vector<Lv>& L, double tick)
{
    int n = 0;
    if (!(tick > 0)) return 0;
    for (const Lv& q : L) {
        const long long k = (long long)std::llround(q.px / tick); bool done = false;
        for (M& m : V) {
            if (m.name == "VWAP" || (long long)std::llround(m.price / tick) != k) continue;
            if (("/" + m.name + "/").find("/" + q.name + "/") == std::string::npos) m.name += "/" + q.name;
            done = true; break;
        }
        if (!done) V.push_back(M{ q.name, q.px, false });
        ++n;
    }
    return n;
}
inline std::string statusText(const Snap& s, double tick, int bars, int perSec)
{
    char b[400]; auto rt = [&](double x) { return std::isfinite(x) ? (tick > 0 ? std::round(x / tick) * tick : x) : 0.0; };
    std::snprintf(b, sizeof b, "HTFSR,%s,3-hour bars from 17:00 CT (forming bar counts) 1-bar High/Low ZigZag,period %d,3-hour bars %d,R1 %.10g %s,R2 %.10g %s,S1 %.10g %s,S2 %.10g %s,swing %s %.10g",
                  VERSION, perSec, bars, rt(s.r1), s.r1ok ? "standing" : "broken", rt(s.r2), s.r2ok ? "standing" : "broken", rt(s.s1), s.s1ok ? "standing" : "broken", rt(s.s2), s.s2ok ? "standing" : "broken",
                  s.dir > 0 ? "up" : s.dir < 0 ? "down" : "-", rt(s.ext));
    return b;
}

// ---- lsHtfSR: the standing levels as stepped lines over the chart's bars (each closed bar's value as it stood at its close - decided once;
// the forming bar's value is live), one colour, the name at the right end of the newest piece; R2 / S2 only where they differ from R1 / S1
struct Prim { bool line = false; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned long col = 0; std::string text; };
struct View { int l = 0, t = 0, r = 0, b = 0, first = 0, last = 0; std::function<double(int)> X; std::function<double(double)> Y; std::function<int(const std::string&)> textW; int textH = 12; };
static const unsigned long C_LINE = 0x00FF9F1A;     // the orange lsCobSignal draws its 3-hour lines in
inline double shown(const Snap& h, int q, double tick)
{
    const double x = q == 0 ? h.r1 : q == 1 ? h.r2 : q == 2 ? h.s1 : h.s2; const bool ok = q == 0 ? h.r1ok : q == 1 ? h.r2ok : q == 2 ? h.s1ok : h.s2ok;
    if (!ok || !std::isfinite(x)) return NAN;
    if (q == 1 && h.r1ok && std::isfinite(h.r1) && std::fabs(x - h.r1) < tick * 0.25) return NAN;
    if (q == 3 && h.s1ok && std::isfinite(h.s1) && std::fabs(x - h.s1) < tick * 0.25) return NAN;
    return x;
}
inline std::vector<Prim> layout(const std::vector<Snap>& hs, double tick, const View& v)
{
    std::vector<Prim> P; const int n = (int)hs.size(); const int a = std::max(0, v.first), b = std::min(n - 1, v.last);
    if (b < a || !v.X || !v.Y || !(tick > 0)) return P;
    const double bw = b > a ? (v.X(b) - v.X(a)) / (double)(b - a) : 6.0;
    static const char* const NM[4] = { "R1", "R2", "S1", "S2" };
    for (int q = 0; q < 4; ++q) {
        // (1.0.1, Rassul 2026-11-16 06:15 / 06:30 CT) ONE line per level, at where it stands at the last bar on screen, across the chart - no past steps
        const double x = shown(hs[(size_t)b], q, tick); if (!std::isfinite(x)) continue;
        const int y = (int)std::lround(v.Y(x)); if (y < v.t || y > v.b) continue;
        const int x0 = v.l, x1 = std::min(v.r, (int)std::lround(v.X(b) + bw / 2)); if (x1 <= x0) continue;
        Prim L; L.line = true; L.x0 = x0; L.x1 = x1; L.y0 = L.y1 = y; L.col = C_LINE; P.push_back(L);
        const std::string s = NM[q]; const int w = v.textW ? v.textW(s) : (int)s.size() * 6, h = v.textH;
        Prim T; T.col = C_LINE; T.text = s; T.x0 = x1 + 3; T.x1 = T.x0 + w; T.y0 = y - h / 2; T.y1 = T.y0 + h;
        if (T.x1 > v.r) { T.x1 = x1 - 2; T.x0 = T.x1 - w; T.y1 = y - 1; T.y0 = T.y1 - h; }
        if (T.x0 >= v.l && T.x1 <= v.r && T.y0 >= v.t && T.y1 <= v.b) P.push_back(T);
    }
    return P;
}
} // namespace hsr
