// ZzoDeltaLogic.h - lsZzoDelta 1.0.0 (2026-11-02, Rassul 19:40-20:05 CT: "creat an indicator based on delta like zzo delta using high
// low and 1 bar reversal default. it shoud use 1 pane and should be green for upwave and red for down wave" -> "use a ratio instead"
// -> mockup "ZZO Delta Mockup" option 1 (median of the last 5) -> "ok lets do option 2" = the history restarts at the RTH open).
// SDK-free (tested: tests_zzodelta).
//   ZigZag   High / Low, 1-bar reversal: an up wave runs while highs extend and turns down on a bar with a LOWER LOW than the bar
//            before (the mirror for a down wave) - the same walk as CoB Signal (CobSignalLogic.h). A wave = the bars after the
//            previous pivot through its own pivot (the first wave on the chart starts at its first bar).
//   delta    each bar's buy - sell volume from IRT's VAP (summed over its rows); a wave's delta = the sum over its bars.
//   ratio    the wave's delta / the MEDIAN |delta| of the last 5 COMPLETED waves in the same direction in the same part of the day
//            (overnight / RTH / after the close, by the bar that made the wave's pivot); needs 3 such waves, so the first 3 up and
//            3 down waves after the RTH open show no ratio (option 2). 1.0x = a normal wave; plotted bar by bar as the wave's running
//            total / that median (green up wave, red down wave; below zero = delta against the wave). The label at the wave's end
//            is the ratio WITH the wave ("2.6x"; "-0.4x" = an up wave sold into / a down wave bought).
//   closed bars only, plus the forming bar live; the live wave's last bars move to the next wave when the 1-bar reversal flips
//   (as the ZigZag does). Completed waves never change.
#ifndef ZZO_DELTA_LOGIC_H
#define ZZO_DELTA_LOGIC_H
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include "SessionVWAPLogic.h"

namespace zzd {
static const char* const VERSION = "1.0.0";
static const int HIST = 5, MIN_HIST = 3, MAX_BARS = 6000, TRIM = 1000;

inline bool toTicks(double px, double tick, long long& out) {
    if (!std::isfinite(px) || !(tick > 0)) return false;
    const double q = px / tick;
    if (!std::isfinite(q) || std::fabs(q) > 9e15) return false;
    out = (long long)std::llround(q); return true;
}
inline long long floorDiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }

struct Bar { long long t = 0, h = 0, l = 0, dlt = 0; bool vap = false; long long seg = 0; };   // t = bar END (local secs), prices in ticks
struct Wave { int s = 0, e = 0, dir = 0; long long dlt = 0, seg = 0; bool hasR = false; double med = 0; bool live = false; };
struct Walk { int dirn = 0, ext = 0, start = 0; bool any = false; };   // any = a wave completed already

inline bool medianOf(std::vector<long long> v, double& m) {
    if (v.empty()) return false;
    std::sort(v.begin(), v.end()); const size_t n = v.size();
    m = n % 2 ? (double)v[n / 2] : ((double)v[n / 2 - 1] + (double)v[n / 2]) / 2.0; return true;
}

struct Engine {
    int openMin = 510, closeMin = 900, per = 180;
    std::vector<Bar> B; Walk Z; std::vector<Wave> W; long long trimmed = 0; long long noVap = 0;
    void reset() { B.clear(); Z = Walk(); W.clear(); trimmed = 0; noVap = 0; }
    bool setMarket(const std::string& root, int spb) { per = spb > 0 ? spb : 180; const std::string m = root == "6E" ? std::string("EU") : root; return svl::rthOf(m.c_str(), openMin, closeMin); }
    long long segOf(long long tEnd) const {   // overnight / RTH / after the close of one session = one history
        const long long days = floorDiv(tEnd, 86400); const int sec = (int)(tEnd - days * 86400);
        const svl::Cls c = svl::classify((long)days, sec, per, openMin, closeMin);
        return (long long)c.sess * 4 + c.kind;
    }
    // the median |delta| of the last HIST completed waves with this direction and segment among W[0 .. nW)
    bool baseFor(int dir, long long seg, size_t nW, double& med) const {
        std::vector<long long> v;
        for (size_t k = nW; k-- > 0 && (int)v.size() < HIST;) { const Wave& w = W[k]; if (w.dir == dir && w.seg == seg) v.push_back(w.dlt < 0 ? -w.dlt : w.dlt); }
        if ((int)v.size() < MIN_HIST) return false;
        return medianOf(v, med) && med > 0;
    }
    Wave make(const std::vector<Bar>& bars, int s, int e, int dir, size_t nW, bool live) const {
        Wave w; w.s = s; w.e = e; w.dir = dir; w.live = live;
        for (int i = s; i <= e; ++i) w.dlt += bars[(size_t)i].dlt;
        w.seg = bars[(size_t)e].seg; w.hasR = baseFor(dir, w.seg, nW, w.med); return w;
    }
    // the 1-bar walk with bar j (CobSignalLogic.h): true = a wave completed (s .. e, dir)
    static bool step(const std::vector<Bar>& b, Walk& z, int j, int& s, int& e, int& dir) {
        if (j == 1) { z.dirn = b[1].h >= b[0].h ? 1 : -1; z.ext = 0; z.start = 0; }
        if (j < 1) return false;
        if (z.dirn == 1) {
            if (b[(size_t)j].h > b[(size_t)z.ext].h) { z.ext = j; return false; }
            if (b[(size_t)j].l < b[(size_t)j - 1].l && j != z.ext) { s = z.any ? z.start + 1 : z.start; e = z.ext; dir = 1; z.start = z.ext; z.ext = j; z.dirn = -1; z.any = true; return true; }
        } else {
            if (b[(size_t)j].l < b[(size_t)z.ext].l) { z.ext = j; return false; }
            if (b[(size_t)j].h > b[(size_t)j - 1].h && j != z.ext) { s = z.any ? z.start + 1 : z.start; e = z.ext; dir = -1; z.start = z.ext; z.ext = j; z.dirn = 1; z.any = true; return true; }
        }
        return false;
    }
    void addBar(Bar b) {
        b.seg = segOf(b.t); if (!b.vap) ++noVap;
        B.push_back(b); const int j = (int)B.size() - 1;
        int s = 0, e = 0, d = 0;
        if (step(B, Z, j, s, e, d)) W.push_back(make(B, s, e, d, W.size(), false));
        if ((int)B.size() > MAX_BARS) trim(TRIM);
    }
    void trim(int k) {
        if (k <= 0 || k >= (int)B.size() || Z.start < k) return;   // never cut the live wave
        B.erase(B.begin(), B.begin() + k);
        std::vector<Wave> hist, keep;
        for (Wave w : W) {
            if (w.e < k) { w.s = w.e = -1; hist.push_back(w); }            // before the cut: kept for the median only (delta stays)
            else { w.s = std::max(0, w.s - k); w.e -= k; keep.push_back(w); }
        }
        if (hist.size() > 40) hist.erase(hist.begin(), hist.end() - 40);
        hist.insert(hist.end(), keep.begin(), keep.end()); W.swap(hist);
        Z.ext -= k; Z.start -= k; trimmed += k;
    }
    // the waves to draw: the completed ones + the live tail (closed bars after the last pivot, and the forming bar when given;
    // a flip on the forming bar shows the provisional completed wave and the new live one)
    std::vector<Wave> view(const Bar* form) const {
        std::vector<Wave> out; for (const Wave& w : W) if (w.s >= 0) out.push_back(w);
        if (B.empty()) return out;
        if (form) {
            std::vector<Bar> b2 = B; Bar f = *form; f.seg = segOf(f.t); b2.push_back(f);
            Walk z = Z; int s = 0, e = 0, d = 0; const int j = (int)b2.size() - 1;
            Engine tmp; tmp.W = W;
            if (step(b2, z, j, s, e, d)) { Wave w = tmp.make(b2, s, e, d, tmp.W.size(), true); out.push_back(w); tmp.W.push_back(w); }
            const int ls = z.any ? z.start + 1 : 0;
            if (j >= 1 && ls <= j) out.push_back(tmp.make(b2, ls, j, z.dirn, tmp.W.size(), true));
            return out;
        }
        const int j = (int)B.size() - 1, ls = Z.any ? Z.start + 1 : 0;
        if (j >= 1 && ls <= j) out.push_back(make(B, ls, j, Z.dirn, W.size(), true));
        return out;
    }
};

inline std::string ratioText(const Wave& w) {
    if (!w.hasR) return "-";
    const double r = (double)w.dir * (double)w.dlt / w.med; char b[24];
    const bool z = std::fabs(r) < 0.05; std::snprintf(b, sizeof b, "%s%.1fx", r < 0 && !z ? "-" : "", z ? 0.0 : std::fabs(r)); return b;
}

// ---- drawing (own pane) ----
struct Prim { enum K { FILL, TEXT } k = FILL; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned long col = 0; std::string text; };
struct Colours { unsigned long up = 0x22C55E, down = 0xEF4444, zero = 0x334155, grey = 0x94A3B8; };
struct View {
    int l = 0, t = 0, r = 0, b = 0, first = 0, last = 0; double bw = 8;   // engine bar indexes of the visible range, pixels per bar
    std::function<double(int)> X; std::function<int(const std::string&)> textW; int textH = 12;
};
struct Scene { std::vector<Prim> p; int bars = 0, labels = 0; double lo = 0, hi = 0; };
inline int ci(double v) { return (int)std::lround(std::max(-30000.0, std::min(30000.0, v))); }

inline Scene layout(const Engine& E, const std::vector<Wave>& waves, long long formDlt, const View& v, const std::string& head) {
    Scene S; const Colours C;
    const int TH = v.textH, top = v.t + TH + 4 + TH + 2, bot = v.b - TH - 2;   // header row, room for a label above / below
    // running value per bar of the visible waves
    struct Pt { int i; double val; int dir; };
    std::vector<Pt> pts; double lo = 0, hi = 0;
    const int nB = (int)E.B.size();
    for (const Wave& w : waves) {
        if (!w.hasR || w.e < v.first || w.s > v.last) continue;
        double run = 0;
        for (int i = w.s; i <= w.e; ++i) {
            const long long d = i < nB ? E.B[(size_t)i].dlt : formDlt;   // the forming bar = index nB
            run += (double)d / w.med;
            if (i < v.first || i > v.last) continue;
            pts.push_back({ i, run, w.dir }); lo = std::min(lo, run); hi = std::max(hi, run);
        }
    }
    if (hi - lo < 1.0) { hi = std::max(hi, 0.5); lo = std::min(lo, -0.5); }
    S.lo = lo; S.hi = hi;
    auto Y = [&](double x) { return (double)bot - (x - lo) / (hi - lo) * (double)(bot - top); };
    const int z = ci(Y(0));
    { Prim p; p.k = Prim::FILL; p.x0 = v.l; p.x1 = v.r; p.y0 = p.y1 = z; p.col = C.zero; S.p.push_back(p); }
    const double half = std::max(1.0, v.bw * 0.35);
    for (const Pt& q : pts) {
        const double xc = v.X(q.i); const int y = ci(Y(q.val));
        Prim p; p.k = Prim::FILL; p.x0 = ci(xc - half); p.x1 = std::max(p.x0, ci(xc + half) - 1); p.y0 = std::min(y, z); p.y1 = std::max(y, z);
        if (p.y1 == p.y0) p.y1 = p.y0 + 1;
        p.col = q.dir > 0 ? C.up : C.down; if (p.x1 >= v.l && p.x0 <= v.r) { S.p.push_back(p); ++S.bars; }
    }
    // labels at each wave's end (skipped when they would overlap the previous one)
    int lastR = -100000;
    for (const Wave& w : waves) {
        if (w.e < v.first || w.e > v.last) continue;
        const std::string t = ratioText(w); const int tw = v.textW(t); const double xc = v.X(w.e);
        int x0 = ci(xc - tw / 2.0); if (x0 < v.l + 1) x0 = v.l + 1; if (x0 + tw > v.r - 1) x0 = v.r - 1 - tw;
        if (x0 <= lastR + 2) continue;
        double end = w.hasR ? (double)w.dlt / w.med : 0.0;
        // clear of every bar under the label's width: above the highest (end >= 0) / below the lowest (end < 0)
        int yTop = z, yBot = z;
        for (const Pt& q : pts) { const double xc2 = v.X(q.i); if (xc2 + half < x0 - 1 || xc2 - half > x0 + tw + 1) continue; const int y = ci(Y(q.val)); yTop = std::min(yTop, y); yBot = std::max(yBot, y); }
        Prim p; p.k = Prim::TEXT; p.x0 = x0; p.x1 = x0 + tw; p.text = t;
        if (end >= 0) { p.y1 = std::max(yTop - 2, v.t + TH + 4 + TH); p.y0 = p.y1 - TH; } else { p.y0 = std::min(yBot + 2, v.b - TH); p.y1 = p.y0 + TH; }
        p.col = !w.hasR ? C.grey : w.dir > 0 ? C.up : C.down; S.p.push_back(p); ++S.labels; lastR = x0 + tw;
    }
    { Prim p; p.k = Prim::TEXT; p.x0 = v.l + 4; p.y0 = v.t + 2; p.x1 = p.x0 + v.textW(head); p.y1 = p.y0 + TH; p.col = C.grey; p.text = head; S.p.push_back(p); }
    return S;
}

inline std::string statusText(const Engine& E, const std::string& mkt, int spb, const std::string& vap) {
    std::string s = std::string("lsZzoDelta ") + VERSION + "\nMARKET," + mkt + "," + std::to_string(spb) + "\nRTH," + std::to_string(E.openMin) + "-" + std::to_string(E.closeMin) +
        "\nRULES,1-bar High/Low ZigZag; wave delta / median |delta| of the last 5 same-way waves in the same session part (>= 3; RTH restarts)\nBARS," +
        std::to_string(E.B.size()) + ",no VAP " + std::to_string(E.noVap) + ",VAP " + vap + "\nWAVES," + std::to_string(E.W.size()) + "\n";
    int n = 0;
    for (size_t k = E.W.size(); k-- > 0 && n < 12; ++n) {
        const Wave& w = E.W[k]; if (w.s < 0) break;
        char b[160]; std::snprintf(b, sizeof b, "WAVE,%lld,%s,delta %lld,%s\n", E.B[(size_t)w.e].t, w.dir > 0 ? "UP" : "DN", w.dlt, ratioText(w).c_str()); s += b;
    }
    return s;
}
}  // namespace zzd
#endif
