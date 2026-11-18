// SdmoLogic.h  --  lsSDMO 0.1.0 (2026-11-18): SD momentum. SDK-free (all rules, the read and the layout; SDMO.cpp only feeds bars and draws).
// Rassul 2026-11-18 11:20 CT "create an indicator called sdmomo, it is made of two indicators that are on my chart sd and sdma ... and create
// its read too"; 11:29 "use a green line and red for the avg"; 12:40 "just call it SDMO"; 12:44 "the ma for sd is 3 period. the ma for sdma is
// 20"; 12:46 "test against your setting and find ideal settings"; 12:51 "letsgo with yours.. build" (8 / 14: the settings test of 11-18 on
// 2-3 months x 7 markets - crosses alone are ~50/50 for direction in every setting; 8 / 14 halves the fake crosses of 3 / 20 for ~2 candles).
//
//   raw[i] = sum over the last 20 candles of (high - low) on up-closes minus (high - low) on down-closes   (his RTL: SUM(HLR*(CL>CL.1),20) - ...)
//   SD     = simple MA(raw, SD_LEN = 8)          the green line
//   AVG    = simple MA(SD,  AVG_LEN = 14)        the red line (his SDMA)
//   gap    = SD - AVG; normal gap = median |gap| of the 100 candles before (30+ needed); x = |gap| / normal
// THE READ (closed candles only, never repainted), first match wins:
//   1 took over  : SD crossed AVG in the last 3 candles and x >= 0.5          "Buyers took over momentum"
//   2 fading at  : a new 20-candle low (high) while SD is above (below) SD at the lowest (highest) local low (high) of the 6-40 candles before
//                                                                              "Sellers' momentum fading at the low"
//   3 even       : x < 0.5                                                     "Momentum even"
//   4 building   : |gap| wider than 3 candles ago                              "Buyers' momentum building"
//   5 fading     : otherwise                                                   "Buyers' momentum fading"
// No buy / sell signals (Rassul 11-18 09:3x "dont put buy or sell signals at this time").
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace sdmo {

static const char* const VERSION = "0.2.0";   // (0.2.0, Rassul 13:08 CT "yes, just like the mockup": the Reversal Read I letters on a dark gray strip at the top of the pane, read from RevRead\wy-<MKT>-<secs>.txt - the same I as the Reversal Read)
static const int SUM_LEN = 20, SD_LEN = 8, AVG_LEN = 14;
static const int NORM_LOOK = 100, NORM_MIN = 30, EXT_LOOK = 20, DIV_FROM = 6, DIV_TO = 40, DIV_WIN = 10, CROSS_RECENT = 3;
static const double EVEN_X = 0.5;
static const unsigned C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_WHITE = 0x00E5E7EB, C_GRAY = 0x009CA3AF, C_ZERO = 0x006B7280, C_STRIP = 0x00333333;
static const int STRIP_H = 11;
static const size_t MAX_BARS = 20000;

inline bool fin(double v) { return v == v && v < 1e300 && v > -1e300; }

struct Bar { long long t = 0; double h = 0, l = 0, c = 0; long long local = 0; };   // local = the chart's wall clock in seconds (for the times in the read)

struct Read { bool ok = false; int col = 0; std::string when, head, l1, l2; };

struct Engine {
    std::vector<Bar> B; std::vector<double> raw, sd, avg;   // NaN where not ready
    size_t dropped = 0;
    void reset() { B.clear(); raw.clear(); sd.clear(); avg.clear(); dropped = 0; }
    static double nan() { return std::nan(""); }
    void add(const Bar& b0) {
        Bar b = b0; if (!fin(b.h) || !fin(b.l) || !fin(b.c)) { b.h = b.l = b.c = B.empty() ? 0 : B.back().c; }
        if (b.h < b.l) std::swap(b.h, b.l);
        B.push_back(b); const size_t i = B.size() - 1;
        double r = nan();
        if (i >= (size_t)SUM_LEN) { double s = 0; for (size_t k = i + 1 - SUM_LEN; k <= i; ++k) { const double rg = B[k].h - B[k].l; if (B[k].c > B[k - 1].c) s += rg; else if (B[k].c < B[k - 1].c) s -= rg; } r = s; }
        raw.push_back(r);
        sd.push_back(ma(raw, i, SD_LEN));
        avg.push_back(ma(sd, i, AVG_LEN));
        if (B.size() > MAX_BARS) { const size_t cut = B.size() - MAX_BARS; B.erase(B.begin(), B.begin() + (long)cut); raw.erase(raw.begin(), raw.begin() + (long)cut); sd.erase(sd.begin(), sd.begin() + (long)cut); avg.erase(avg.begin(), avg.begin() + (long)cut); dropped += cut; }
    }
    static double ma(const std::vector<double>& x, size_t i, int n) {
        if (i + 1 < (size_t)n) return nan();
        double s = 0; for (size_t k = i + 1 - n; k <= i; ++k) { if (!fin(x[k])) return nan(); s += x[k]; } return s / n;
    }
    double gap(int i) const { return (i >= 0 && i < (int)B.size() && fin(sd[(size_t)i]) && fin(avg[(size_t)i])) ? sd[(size_t)i] - avg[(size_t)i] : nan(); }
};

inline std::string clock12(long long local) {
    if (local <= 0) return std::string();
    long long s = local % 86400; if (s < 0) s += 86400; const int h = (int)(s / 3600), m = (int)((s % 3600) / 60);
    char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", (h + 11) % 12 + 1, m, h < 12 ? "AM" : "PM"); return b;
}
inline std::string fx(double v) { char b[16]; std::snprintf(b, sizeof b, "%.1fx", v); return b; }
inline std::string sgn(double v) { char b[24]; std::snprintf(b, sizeof b, "%+.0f", v); return std::string(b) == "-0" ? std::string("+0") : std::string(b); }

// the read of closed candle i
inline Read readAt(const Engine& E, int i) {
    Read r; const int n = (int)E.B.size(); if (i < 1 || i >= n) return r;
    const double g = E.gap(i); if (!fin(g)) return r;
    std::vector<double> h; for (int k = std::max(0, i - NORM_LOOK); k < i; ++k) { const double v = E.gap(k); if (fin(v)) h.push_back(std::fabs(v)); }
    if ((int)h.size() < NORM_MIN) return r;
    std::nth_element(h.begin(), h.begin() + (long)(h.size() / 2), h.end());
    double norm = h[h.size() / 2]; if (h.size() % 2 == 0) { const double hi = norm; std::nth_element(h.begin(), h.begin() + (long)(h.size() / 2 - 1), h.end()); norm = (hi + h[h.size() / 2 - 1]) / 2; }
    if (!(norm > 0)) norm = 1e-9;
    const double x = std::fabs(g) / norm; const int side = g > 0 ? 1 : -1;
    const Bar& b = E.B[(size_t)i]; const double sd = E.sd[(size_t)i], av = E.avg[(size_t)i];
    const std::string who = side > 0 ? "Buyers" : "Sellers";
    r.ok = true; r.when = clock12(b.local);
    // 1 took over
    int cross = -1; for (int k = i; k > i - CROSS_RECENT && k >= 1; --k) { const double a = E.gap(k), p = E.gap(k - 1); if (fin(a) && fin(p) && a * p < 0) { cross = k; break; } }
    if (cross >= 0 && x >= EVEN_X) {
        r.col = side; r.head = who + " took over momentum";
        r.l1 = std::string("SD crossed ") + (side > 0 ? "above" : "below") + " its average at " + clock12(E.B[(size_t)cross].local) + " (" + fx(x) + " gap, SD " + (sd > 0 ? "above" : "below") + " 0).";
        r.l2 = std::string("Watch: a ") + (side > 0 ? "higher low holds -> long idea." : "lower high holds -> short idea.");
        return r;
    }
    // 2 fading at the low / high (a divergence)
    if (i >= EXT_LOOK) {
        double lo = b.l, hi = b.h; for (int k = i - EXT_LOOK + 1; k <= i; ++k) { lo = std::min(lo, E.B[(size_t)k].l); hi = std::max(hi, E.B[(size_t)k].h); }
        for (int s : { 1, -1 }) {
            const bool atExt = s > 0 ? b.l <= lo : b.h >= hi; if (!atExt) continue;
            int best = -1;
            for (int k = std::max(0, i - DIV_TO); k < i - DIV_FROM; ++k) {
                bool local = true; for (int q = std::max(0, k - DIV_WIN); q <= std::min(n - 1, k + DIV_WIN); ++q) if (s > 0 ? E.B[(size_t)q].l < E.B[(size_t)k].l : E.B[(size_t)q].h > E.B[(size_t)k].h) { local = false; break; }
                if (!local) continue;
                if (best < 0 || (s > 0 ? E.B[(size_t)k].l < E.B[(size_t)best].l : E.B[(size_t)k].h > E.B[(size_t)best].h)) best = k;
            }
            if (best < 0 || !fin(E.sd[(size_t)best])) continue;
            const bool newer = s > 0 ? E.B[(size_t)best].l > b.l : E.B[(size_t)best].h < b.h;
            const bool weaker = s > 0 ? sd > E.sd[(size_t)best] : sd < E.sd[(size_t)best];
            if (!newer || !weaker) continue;
            r.col = s; r.head = std::string(s > 0 ? "Sellers" : "Buyers") + "' momentum fading at the " + (s > 0 ? "low" : "high");
            r.l1 = std::string("New ") + (s > 0 ? "low" : "high") + ", but SD " + sgn(sd) + " vs " + sgn(E.sd[(size_t)best]) + " at the " + clock12(E.B[(size_t)best].local) + " " + (s > 0 ? "low." : "high.");
            r.l2 = std::string("Watch: ") + (s > 0 ? "buyers" : "sellers") + " take over (SD crosses its average) -> " + (s > 0 ? "long" : "short") + " idea.";
            return r;
        }
    }
    // 3 even
    if (x < EVEN_X) { r.col = 0; r.head = "Momentum even"; r.l1 = "SD hugging its average (" + fx(x) + " normal gap)."; r.l2 = "Watch: the next cross picks the side."; return r; }
    // 4 / 5 building or fading
    const double g3 = E.gap(i - 3);
    r.col = side;
    if (fin(g3) && std::fabs(g) > std::fabs(g3)) {
        r.head = who + "' momentum building";
        r.l1 = "SD " + sgn(sd) + (side > 0 ? " above" : " below") + " its average " + sgn(av) + ": " + fx(x) + " the normal gap, widening.";
        r.l2 = std::string("Watch: ") + (side > 0 ? "pullbacks get bought -> stay with buyers." : "bounces get sold -> stay with sellers.");
    } else {
        r.head = who + "' momentum fading";
        r.l1 = "SD " + sgn(sd) + " still" + (side > 0 ? " above" : " below") + " its average " + sgn(av) + " (" + fx(x) + " gap), but closing in.";
        r.l2 = std::string("Watch: a cross -> ") + (side > 0 ? "sellers" : "buyers") + " take over.";
    }
    return r;
}

// ---- layout (pixels; SDMO.cpp maps bars to x) ----
struct View { int l = 0, t = 0, r = 0, b = 0, first = 0, last = -1, textH = 12, readX = -1; std::function<double(int)> X; std::function<int(const std::string&, bool)> textW;
              const std::map<long long, int>* iMarks = nullptr; };   // (0.2.0) candle stamp -> +1 / -1 for the Reversal Read I letters (nullptr = no strip)
struct Prim { int kind = 0; int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned col = 0; int width = 1; bool bold = false; std::string text; };   // kind 0 line, 1 text, 2 filled box

inline std::vector<Prim> layout(const Engine& E, const View& v) {
    std::vector<Prim> P; const int n = (int)E.B.size(); if (n < 2 || !v.X) return P;
    const int a = std::max(0, v.first), z = std::min(n - 1, v.last); if (z < a) return P;
    const bool strip = v.iMarks != nullptr;
    const int top = v.t + (strip ? STRIP_H + 2 : 0) + v.textH + 4, bot = v.b - 4; if (bot - top < 20) return P;
    if (strip) {   // (0.2.0) the dark gray strip with the I letters, from the left edge to just past the last candle on screen
        Prim S; S.kind = 2; S.col = C_STRIP; S.x0 = v.l; S.y0 = v.t; S.y1 = v.t + STRIP_H; S.x1 = std::min(v.r, (int)std::lround(v.X(z)) + 8); P.push_back(S);
        for (int i = a; i <= z; ++i) { const auto it = v.iMarks->find(E.B[(size_t)i].t); if (it == v.iMarks->end() || it->second == 0) continue;
            Prim T; T.kind = 1; T.bold = true; T.col = it->second > 0 ? C_GREEN : C_RED; T.text = "I"; const int w = v.textW ? v.textW(T.text, true) : 4; const int cx = (int)std::lround(v.X(i));
            T.x0 = cx - w / 2; T.x1 = T.x0 + w; T.y0 = v.t; T.y1 = v.t + STRIP_H; P.push_back(T); } }
    double lo = 0, hi = 0; bool any = false;
    for (int i = a; i <= z; ++i) for (double y : { E.sd[(size_t)i], E.avg[(size_t)i] }) if (fin(y)) { lo = std::min(lo, y); hi = std::max(hi, y); any = true; }
    // title (version next to the name)
    { Prim T; T.kind = 1; T.bold = true; T.col = C_WHITE; T.text = std::string("SDMO v") + VERSION; T.x0 = v.l + 4; T.y0 = v.t + 1 + (strip ? STRIP_H + 2 : 0); T.x1 = T.x0 + (v.textW ? v.textW(T.text, true) : 60); T.y1 = T.y0 + v.textH; P.push_back(T); }
    if (!any) return P;
    const double pad = (hi - lo) * 0.06 + 1e-9; lo -= pad; hi += pad;
    auto Y = [&](double y) { return (int)std::lround(bot - (y - lo) / (hi - lo) * (bot - top)); };
    { Prim Z; Z.kind = 0; Z.col = C_ZERO; Z.x0 = v.l; Z.x1 = v.r; Z.y0 = Z.y1 = Y(0); P.push_back(Z); }
    const double* srcs[2] = { E.avg.data(), E.sd.data() }; const unsigned cols[2] = { C_RED, C_GREEN }; const int wid[2] = { 1, 2 };
    for (int s = 0; s < 2; ++s)
        for (int i = std::max(a, 1); i <= z; ++i) {
            const double y0 = srcs[s][i - 1], y1 = srcs[s][i]; if (!fin(y0) || !fin(y1)) continue;
            Prim L; L.kind = 0; L.col = cols[s]; L.width = wid[s]; L.x0 = (int)std::lround(v.X(i - 1)); L.x1 = (int)std::lround(v.X(i)); L.y0 = Y(y0); L.y1 = Y(y1); P.push_back(L);
        }
    // the read: right of the last candle when there is room, else at the top-left under the title
    const Read r = readAt(E, n - 1);
    if (r.ok) {
        const std::string head = r.when + " ", body[2] = { r.l1, r.l2 };
        const int lh = v.textH + 2, y0 = v.t + 1 + (strip ? STRIP_H + 2 : 0); int x = v.readX >= 0 ? v.readX : v.l + 4; int y = v.readX >= 0 ? y0 : y0 + lh;
        int wmax = (v.textW ? v.textW(head, true) + v.textW(r.head, true) : 300); for (const std::string& s : body) wmax = std::max(wmax, v.textW ? v.textW(s, true) : 300);
        if (x + wmax > v.r - 2) { x = v.l + 4; y = y0 + lh; }
        const unsigned hc = r.col > 0 ? C_GREEN : r.col < 0 ? C_RED : C_WHITE;
        Prim T1; T1.kind = 1; T1.bold = true; T1.col = C_WHITE; T1.text = head; T1.x0 = x; T1.y0 = y; T1.x1 = x + (v.textW ? v.textW(head, true) : 60); T1.y1 = y + v.textH; P.push_back(T1);
        Prim T2 = T1; T2.col = hc; T2.text = r.head; T2.x0 = T1.x1; T2.x1 = T2.x0 + (v.textW ? v.textW(r.head, true) : 200); P.push_back(T2);
        for (int k = 0; k < 2; ++k) { Prim T = T1; T.col = C_WHITE; T.text = body[k]; T.y0 = y + lh * (k + 1); T.y1 = T.y0 + v.textH; T.x1 = x + (v.textW ? v.textW(body[k], true) : 300); if (T.y1 <= v.b) P.push_back(T); }
    }
    return P;
}

// (0.2.0) lsRevRead's letters file: "WY|1|<MKT>|<secs>|<ver>" then "stamp|letter|dir" lines; only the I's are kept
inline int parseLetters(const std::string& txt, std::map<long long, int>& out) {
    out.clear(); size_t a = 0; int n = 0; bool head = false;
    while (a < txt.size()) { size_t e = txt.find('\n', a); if (e == std::string::npos) e = txt.size(); std::string l = txt.substr(a, e - a); a = e + 1;
        if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.empty()) continue;
        if (!head) { if (l.compare(0, 5, "WY|1|") != 0) return -1; head = true; continue; }
        const size_t p1 = l.find('|'), p2 = p1 == std::string::npos ? p1 : l.find('|', p1 + 1); if (p2 == std::string::npos) continue;
        char* end = nullptr; const long long t = std::strtoll(l.c_str(), &end, 10); if (!end || *end != '|' || t <= 0) continue;
        if (l.substr(p1 + 1, p2 - p1 - 1) != "I") continue;
        const int d = std::atoi(l.c_str() + p2 + 1); if (d != 1 && d != -1) continue;
        out[t] = d; ++n; }
    return head ? n : -1;
}

inline std::string statusText(const Engine& E, const std::string& mkt, const std::string& sym, int spb) {
    std::string o = std::string("VERSION,") + VERSION + "\nMARKET," + mkt + "\nSYMBOL," + sym + "\nPER," + std::to_string(spb)
        + "\nRULES,sum " + std::to_string(SUM_LEN) + " (up-close range - down-close range); SD = MA " + std::to_string(SD_LEN) + "; average = MA " + std::to_string(AVG_LEN) + " of SD; even under " + fx(EVEN_X) + " normal gap\nBARS," + std::to_string(E.B.size()) + ",dropped " + std::to_string(E.dropped) + "\n";
    const int n = (int)E.B.size();
    if (n > 0) { char b[160]; std::snprintf(b, sizeof b, "LAST,%s,sd %.2f,avg %.2f\n", clock12(E.B.back().local).c_str(), fin(E.sd.back()) ? E.sd.back() : 0.0, fin(E.avg.back()) ? E.avg.back() : 0.0); o += b;
        const Read r = readAt(E, n - 1); if (r.ok) o += "READ," + r.when + "," + r.head + "|" + r.l1 + "|" + r.l2 + "\n"; else o += "READ,not ready (needs " + std::to_string(SUM_LEN + SD_LEN + AVG_LEN + NORM_MIN) + " candles)\n"; }
    return o;
}

}   // namespace sdmo
