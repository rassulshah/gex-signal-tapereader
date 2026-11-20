// MorrisRsiLogic.h 0.1.4 (2026-11-20)
// 0.1.4 Rassul 2026-11-20 17:04 CT "morris rsi updates to have solid green and red lines": the 70 (red) and 30 (green) lines are SOLID 1 px lines (were dashes)
// 0.1.3 Rassul 2026-11-20 10:58 CT ("why are you not showing divergences" / "same issue", ES 08:36 high RSI 78 -> 09:00 higher high RSI 70 not marked):
//       the Dv pivots come from the 1-bar High / Low walk (a lower low than the previous candle turns an up leg; the swings his chart's
//       ZigZag line shows) - was the 3-bar walk, which skipped the 08:27 / 08:36 swings (no low under the previous 3 lows). Same RSI
//       tests, same 60-candle prior-swing rule, decided at the 2nd pivot, never changed. ES November: 106 -> 140 Dv (+32%).
// was 0.1.2 (2026-11-20) -- lsMorrisRSI: the Morris modified RSI in its own pane, with regular divergences.
// 0.1.2 Rassul 2026-11-20 (ES overnight 11-19/20: a clear bullish divergence - low ~7,148 at ~11:15 PM RSI ~20, lower low ~7,145 at ~1:18 AM
//       RSI ~47 - was not marked because smaller ZigZag lows sat between them and they were ~40 candles apart; 08:00 CT "go"): a confirmed
//       pivot is compared with the prior swing it UNDERCUT - the lowest confirmed pivot low (highest pivot high) within the last 60 candles,
//       the new one excluded - instead of only the previous consecutive pivot <= 30 candles back. Same RSI tests (higher RSI low with the
//       1st RSI <= 40 / lower RSI high with the 1st RSI >= 60), decided at the 2nd pivot's confirmation, never changed; one Dv per pivot.
//       Measured on Sep - Nov 3-min bars, RTH sessions: count per session ~1.0 - 1.13x the old rule in all 7 markets -> no RSI-gap minimum.
// 0.1.1 Rassul 2026-11-20 05:46 CT "make the red and green lines dashed and slightly thinner and get rid of the gray dashed line": the 70 (red)
//       and 30 (green) lines are drawn as short explicit dashes (DASH_ON px on, DASH_OFF px off, 1 px, solid pen per dash - does not depend on
//       the platform's pen style); the grey 50 line is gone. The RSI curve and everything else unchanged.
// Rassul 2026-11-19 11:55 CT "make a morris modified rsi based on 20 bars, the color is blue but below 30 make it green and above 70 make it
// red and if there are diversions also"; 12:00 his IRT RSI settings (RSI Period 20, Smoothing Period 3 Simple, Price Close, Momentum Period 1,
// Morris Modified RSI checked, Ref lines 70 / 30); 12:07 "go" on morris-rsi-mockup.html.
//   RSI     change = close - the close 1 candle earlier; AG / AL = SIMPLE means of the gains / losses over ONLY the last 20 changes (Morris:
//           no running Wilder average); RSI = 100 - 100 / (1 + AG / AL) (AL = 0 -> 100, both 0 -> 50); plotted = the 3-candle simple average
//   colour  blue between 30 and 70, green under 30, red over 70 (a segment takes the colour of its middle value)
//   Dv      regular divergence between a confirmed pivot of the 3-bar High / Low ZigZag (the chart's ZigZag, wyr::legs rules) and the prior
//           swing it undercut / overshot = the lowest pivot low / highest pivot high within the last DV_LOOK (60) candles (0.1.2), decided when the 2nd pivot is confirmed (never changed after): bullish = the 2nd low lower in price, the RSI at
//           it higher, and the RSI at the 1st low <= 40; bearish = the 2nd high higher, the RSI lower, the RSI at the 1st high >= 60
// SDK-free: the host feeds CLOSED candles; the forming candle is drawn live from liveValue(). No settings, no hovers.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace mrsi {

static const char* const VERSION = "0.1.4";
static const int ZZ_REV = 1;   // 0.1.3: pivots on the 1-bar High / Low walk (was 3)
static const int N = 20, SM = 3;                      // RSI period, smoothing (simple)
static const double OB = 70.0, OS = 30.0, MID = 50.0;
static const int DASH_ON = 4, DASH_OFF = 4;            // 70 / 30 line dashes (px), 1 px wide
static const int DV_LOOK = 60;                         // 0.1.2: the prior swing = the extreme pivot within the last 60 candles (was: the consecutive pivot <= 30)
static const double BULL_ZONE = 40.0, BEAR_ZONE = 60.0;
static const unsigned C_BLUE = 0x003D8BFF, C_GREEN = 0x0026C35A, C_RED = 0x00E04848, C_GREY = 0x00555555, C_WHITE = 0x00FFFFFF;   // IRT COLOR = 0x00RRGGBB (as SdmoLogic.h)

struct Bar { long long t = 0; double h = 0, l = 0, c = 0; };
struct Dv { long long t1 = 0, t2 = 0; int dir = 0; };   // dir +1 bullish, -1 bearish

inline double rsiOf(const double* ch, int n) {           // n changes, simple means
    double g = 0, s = 0; for (int k = 0; k < n; ++k) { if (ch[k] > 0) g += ch[k]; else s -= ch[k]; }
    if (s <= 0) return g <= 0 ? 50.0 : 100.0;
    const double rs = (g / n) / (s / n); return 100.0 - 100.0 / (1.0 + rs);
}
inline unsigned colourOf(double v) { return v < OS ? C_GREEN : v > OB ? C_RED : C_BLUE; }

struct Engine {
    std::vector<Bar> B; std::vector<double> raw, val;   // raw RSI / plotted (smoothed); NaN = not enough candles
    std::vector<Dv> dv;                                  // decided divergences (in order)
    // ZigZag state (wyr::legs): zd +1 up / -1 down, zext = the running extreme; pivots per side (bar index)
    int zd = 0, zext = 0; std::vector<int> lows, highs;
    void reset() { *this = Engine(); }
    static double nan() { return std::nan(""); }
    void add(const Bar& b) {
        B.push_back(b); const int i = (int)B.size() - 1;
        double r = nan();
        if (i >= N) { double ch[N]; for (int k = 0; k < N; ++k) ch[k] = B[(size_t)(i - N + 1 + k)].c - B[(size_t)(i - N + k)].c; r = rsiOf(ch, N); }
        raw.push_back(r);
        double v = nan();
        if (i >= SM - 1) { double s = 0; bool ok = true; for (int k = 0; k < SM; ++k) { const double x = raw[(size_t)(i - k)]; if (std::isnan(x)) { ok = false; break; } s += x; } if (ok) v = s / SM; }
        val.push_back(v);
        zig(i);
    }
    void zig(int i) {
        if (i == 0) return;
        if (i == 1) { zd = B[1].h >= B[0].h ? 1 : -1; zext = 0; }
        double pl = B[(size_t)i - 1].l, ph = B[(size_t)i - 1].h;
        for (int j = std::max(0, i - ZZ_REV); j < i; ++j) { pl = std::min(pl, B[(size_t)j].l); ph = std::max(ph, B[(size_t)j].h); }
        if (zd == 1) { if (B[(size_t)i].h > B[(size_t)zext].h) zext = i; else if (B[(size_t)i].l < pl && i != zext) { pivot(zext, -1); zext = i; zd = -1; } }
        else         { if (B[(size_t)i].l < B[(size_t)zext].l) zext = i; else if (B[(size_t)i].h > ph && i != zext) { pivot(zext, 1); zext = i; zd = 1; } }
    }
    void pivot(int k, int side) {                         // side +1 = a confirmed pivot LOW, -1 = a pivot HIGH
        std::vector<int>& P = side > 0 ? lows : highs;
        int a = -1;                                       // 0.1.2: the prior swing = the lowest low / highest high pivot within DV_LOOK candles
        for (size_t q = P.size(); q-- > 0;) {             // newest first; ties keep the later pivot
            const int j = P[q]; if (k - j > DV_LOOK) break; if (j >= k) continue;
            if (a < 0 || (side > 0 ? B[(size_t)j].l < B[(size_t)a].l : B[(size_t)j].h > B[(size_t)a].h)) a = j;
        }
        if (a >= 0) {
            const double ra = val[(size_t)a], rb = val[(size_t)k];
            if (!std::isnan(ra) && !std::isnan(rb)) {
                const bool bull = side > 0 && B[(size_t)k].l < B[(size_t)a].l && rb > ra && ra <= BULL_ZONE;
                const bool bear = side < 0 && B[(size_t)k].h > B[(size_t)a].h && rb < ra && ra >= BEAR_ZONE;
                if (bull || bear) { Dv d; d.t1 = B[(size_t)a].t; d.t2 = B[(size_t)k].t; d.dir = side; dv.push_back(d); }
            }
        }
        P.push_back(k);
    }
    // the forming candle's value, live (changes until it closes)
    double liveValue(double closeNow) const {
        const int n = (int)B.size(); if (n < N) return nan();
        double ch[N]; for (int k = 0; k < N - 1; ++k) ch[k] = B[(size_t)(n - N + 1 + k)].c - B[(size_t)(n - N + k)].c; ch[N - 1] = closeNow - B[(size_t)n - 1].c;
        const double r = rsiOf(ch, N); double s = r;
        for (int k = 1; k < SM; ++k) { const double x = raw[(size_t)(n - k)]; if (std::isnan(x)) return nan(); s += x; }
        return s / SM;
    }
};

// ---- the divergence record (lsFlexLevels\MorrisRSI\dv-<MKT>-<secs>.txt): header + t1|t2|dir, one per line; strict parse
inline std::string dvText(const std::string& mkt, int spb, const std::map<long long, Dv>& D) {
    std::string s = "MRSI|1|" + mkt + "|" + std::to_string(spb) + "|" + VERSION + "\n";
    for (const auto& kv : D) s += std::to_string(kv.second.t1) + "|" + std::to_string(kv.second.t2) + "|" + std::to_string(kv.second.dir) + "\n";
    return s;
}
inline bool parseLL(const std::string& s, long long& v) { if (s.empty()) return false; char* e = nullptr; const long long x = std::strtoll(s.c_str(), &e, 10); if (!e || *e) return false; v = x; return true; }
inline int dvParse(const std::string& txt, const std::string& mkt, int spb, std::map<long long, Dv>& out) {
    size_t p = 0; int n = 0; bool head = false;
    while (p < txt.size()) {
        size_t e = txt.find('\n', p); if (e == std::string::npos) e = txt.size();
        std::string ln = txt.substr(p, e - p); p = e + 1; if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (ln.empty()) continue;
        std::vector<std::string> f; size_t a = 0; for (;;) { const size_t b = ln.find('|', a); f.push_back(ln.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
        if (!head) { if (f.size() < 4 || f[0] != "MRSI" || f[2] != mkt || f[3] != std::to_string(spb)) return -1; head = true; continue; }
        long long t1, t2, d; if (f.size() != 3 || !parseLL(f[0], t1) || !parseLL(f[1], t2) || !parseLL(f[2], d) || (d != 1 && d != -1) || t2 <= t1) continue;
        Dv x; x.t1 = t1; x.t2 = t2; x.dir = (int)d; out[t2] = x; ++n;
    }
    return head ? n : -1;
}

// ---- layout: primitives in pixels (kind 0 line, 1 text, 2 filled box)
struct Prim { int kind = 0; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned col = 0; int width = 1; bool dash = false; bool bold = false; std::string text; };
struct View {
    int l = 0, t = 0, r = 0, b = 0, first = 0, last = -1, lastX = -1;   // engine indices visible; lastX = the pixel right of the last candle
    std::function<double(int)> X; std::function<int(const std::string&)> textW; int textH = 12;
    bool hasLive = false; double live = 0; int liveIdx = -1;             // the forming candle (engine index = B.size())
};
inline int yOf(const View& v, double val) { return v.t + (int)std::lround((100.0 - val) / 100.0 * (double)(v.b - v.t)); }
inline std::string fmt1(double x) { char b[16]; std::snprintf(b, sizeof b, "%.1f", x); return b; }

inline std::vector<Prim> layout(const Engine& E, const View& v) {
    std::vector<Prim> P; if (v.r - v.l < 20 || v.b - v.t < 20) return P;
    auto hline = [&](double lv, unsigned col) {              // short dashes, 1 px (0.1.1); no 50 line
        const int y = yOf(v, lv);
        Prim p; p.kind = 0; p.x0 = v.l; p.x1 = v.r; p.y0 = p.y1 = y; p.col = col; p.width = 1; p.dash = false; P.push_back(p);   // (0.1.4) one solid line
    };
    hline(OB, C_RED); hline(OS, C_GREEN);
    const int n = (int)E.B.size();
    const int a = std::max(0, v.first), z = std::min(n - 1, v.last);
    for (int i = std::max(1, a); i <= z; ++i) {
        const double p0 = E.val[(size_t)i - 1], p1 = E.val[(size_t)i]; if (std::isnan(p0) || std::isnan(p1)) continue;
        Prim p; p.kind = 0; p.x0 = (int)std::lround(v.X(i - 1)); p.x1 = (int)std::lround(v.X(i)); p.y0 = yOf(v, p0); p.y1 = yOf(v, p1); p.col = colourOf((p0 + p1) / 2); p.width = 2; P.push_back(p);
    }
    double shown = n > 0 ? E.val[(size_t)n - 1] : std::nan("");
    if (v.hasLive && v.liveIdx == n && n > 0 && !std::isnan(v.live) && v.liveIdx <= v.last && !std::isnan(E.val[(size_t)n - 1])) {
        Prim p; p.kind = 0; p.x0 = (int)std::lround(v.X(n - 1)); p.x1 = (int)std::lround(v.X(n)); p.y0 = yOf(v, E.val[(size_t)n - 1]); p.y1 = yOf(v, v.live); p.col = colourOf((E.val[(size_t)n - 1] + v.live) / 2); p.width = 2; P.push_back(p);
        shown = v.live;
    }
    // divergences: a line between the two RSI points + "Dv" at the 2nd (under it for bullish, over it for bearish)
    std::map<long long, int> idx; for (int i = 0; i < n; ++i) idx[E.B[(size_t)i].t] = i;
    for (const Dv& d : E.dv) {
        const auto i1 = idx.find(d.t1), i2 = idx.find(d.t2); if (i1 == idx.end() || i2 == idx.end()) continue;
        if (i2->second < a || i1->second > z) continue;
        const double r1 = E.val[(size_t)i1->second], r2 = E.val[(size_t)i2->second]; if (std::isnan(r1) || std::isnan(r2)) continue;
        const unsigned col = d.dir > 0 ? C_GREEN : C_RED;
        Prim ln; ln.kind = 0; ln.x0 = (int)std::lround(v.X(i1->second)); ln.x1 = (int)std::lround(v.X(i2->second)); ln.y0 = yOf(v, r1); ln.y1 = yOf(v, r2); ln.col = col; ln.width = 1; P.push_back(ln);
        const std::string tx = "Dv"; const int tw = v.textW ? v.textW(tx) : 14, th = v.textH;
        const int cx = ln.x1, y = d.dir > 0 ? ln.y1 + 4 : ln.y1 - 4 - th;
        Prim bx; bx.kind = 2; bx.x0 = cx - tw / 2 - 2; bx.x1 = cx + tw / 2 + 2; bx.y0 = std::max(v.t, std::min(v.b - th, y)); bx.y1 = bx.y0 + th; bx.col = col; P.push_back(bx);
        Prim t; t.kind = 1; t.x0 = bx.x0 + 2; t.x1 = bx.x1; t.y0 = bx.y0; t.y1 = bx.y1; t.col = C_WHITE; t.bold = true; t.text = tx; P.push_back(t);
    }
    // title + the value (top left)
    Prim tl; tl.kind = 1; tl.x0 = v.l + 4; tl.y0 = v.t + 2; tl.y1 = v.t + 2 + v.textH;
    tl.text = std::string("Morris RSI v") + VERSION + "  (20, 3): " + (std::isnan(shown) ? std::string("-") : fmt1(shown));
    tl.x1 = tl.x0 + (v.textW ? v.textW(tl.text) : (int)tl.text.size() * 7); tl.col = std::isnan(shown) ? C_BLUE : colourOf(shown); P.push_back(tl);
    return P;
}

inline std::string statusText(const Engine& E, const std::string& mkt, const std::string& sym, int spb) {
    const int n = (int)E.B.size();
    std::string s = std::string("VERSION,") + VERSION + "\nMARKET," + mkt + "\nSYMBOL," + sym + "\nSPB," + std::to_string(spb) + "\nRULES,Morris RSI 20 (simple means over the last 20 changes) smoothed 3 simple; 70 / 30; Dv vs the undercut / overshot swing = lowest / highest ZigZag pivot within 60 candles, 1st RSI <= 40 / >= 60\nBARS," + std::to_string(n) + "\n";
    if (n > 0) s += "LAST," + std::to_string(E.B[(size_t)n - 1].t) + "," + (std::isnan(E.val[(size_t)n - 1]) ? std::string("-") : fmt1(E.val[(size_t)n - 1])) + "\n";
    s += "DIVERGENCES," + std::to_string(E.dv.size()) + "\n";
    return s;
}

} // namespace mrsi
