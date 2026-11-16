// VolRangeLogic.h (1.0.0, 2026-11-15) - SDK-free logic of lsVolRead / lsRangeRead (VolRead.cpp, RangeRead.cpp): Rassul's "little reader
// for the volume and range" (2026-11-14 21:58 CT "BVG , biggest green bar volume in x many green bars, BVR, biggest red bar volume in x many
// red bars , BV which is the biggest volume in x consecutive bars. and BVMax which is the max number of all three. i will use bvmax to
// display on top of the green or red bars when it is a high number"; 21:59 "put the max stat on top then Bv then BV green annd then BVR";
// 2026-11-15 16:07 "i also want to implement the new volume and range indicators that you are showing in the mockup", 17:06 "go build").
//   value   VOLUME pane: the bar's volume.  RANGE pane: high - low in ticks.
//   colour  green = close >= open, red = close < open.
//   BV / BR   = how many bars in a row before this one had a SMALLER value (the count stops at the first bar as big or bigger).
//   BVG / BRG = the same count over GREEN bars only (for a green bar);  BVR / BRR over RED bars only (for a red bar).
//   Max       = the larger of the two. Counts stop at 99 ("99+").
//   On the bars: the Max number over every CLOSED bar whose Max is 20 or more (decided at the bar's close, never repainted).
//   Read box (left of the pane): title, Max, BV, BVG, BVR for the last CLOSED bar; the other colour's row shows "-".
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace vrr {

static const char* const VERSION = "1.1.1";   // (1.1.1, 2026-11-15 independent review: lsBreakout's record files are found by lsBreakout's own file key (boMarket / boFileKey); an unreadable bar no longer inflates the ATR)   // was "1.1.0";   // (1.1.0, 2026-11-15 20:38-20:47 CT Rassul "for the range indicator , when there is a bo that has a range of 1.2*ATR or more as well
// as a pb rev bar ... i want it highlighted in green or red ddepending on whether it is bullish or bearish", "for PB rev do it whenever there is a rev signal also", mockup
// FNJNiY1fR9JBJsU18aZvg4 v2, "build") RANGE pane: bars grey; a BO / BD candle (lsBreakout's record) whose range is >= 1.2 x ATR(13) of the bar before, every PB Rev bar
// (lsBreakout's record) and every CoB pullback Rev bar (lsCobSignal's record) in green (bullish) / red (bearish). A Rev wins over a BO on the same bar. Volume pane unchanged.
static const double HL_ATR_X = 1.2; static const int HL_ATR_N = 13;
enum HlKind { HL_BO = 1, HL_PBREV = 2, HL_COBREV = 3 };
struct Hl { int dir = 0, kind = 0; };
inline std::vector<std::string> hlSplit(const std::string& l, char sep, size_t maxN) {
    std::vector<std::string> f; size_t p = 0;
    while (p <= l.size() && f.size() < maxN) { size_t q = l.find(sep, p); if (q == std::string::npos) q = l.size(); f.push_back(l.substr(p, q - p)); p = q + 1; }
    return f;
}
inline bool hlNum(const std::string& s, long long& v) { if (s.empty()) return false; char* e = nullptr; v = std::strtoll(s.c_str(), &e, 10); return e && *e == 0; }
template <class F> inline void hlLines(const std::string& text, F fn) {
    size_t a = 0;
    while (a < text.size()) { size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size(); std::string l = text.substr(a, b - a); a = b + 1; if (!l.empty() && l.back() == '\r') l.pop_back(); fn(l); }
}
// "BBO|t|t0|dir|fired|..." (lsBreakout breakouts file): a FIRED breakout -> the BO candle t0 (chart time as IRT stamps it)
inline void parseBo(const std::string& text, std::map<long long, Hl>& out) {
    hlLines(text, [&](const std::string& l) {
        if (l.rfind("BBO|", 0) != 0) return;
        const std::vector<std::string> f = hlSplit(l, '|', 6); long long t0 = 0, d = 0;
        if (f.size() < 5 || f[4] != "1" || !hlNum(f[2], t0) || t0 <= 0 || !hlNum(f[3], d) || (d != 1 && d != -1)) return;
        if (!out.count(t0)) { Hl h; h.dir = (int)d; h.kind = HL_BO; out[t0] = h; }
    });
}
// "BRV|t_rev|side|..." (lsBreakout pb file): every PB Rev bar
inline void parsePbRev(const std::string& text, std::map<long long, Hl>& out) {
    hlLines(text, [&](const std::string& l) {
        if (l.rfind("BRV|", 0) != 0) return;
        const std::vector<std::string> f = hlSplit(l, '|', 4); long long t = 0, d = 0;
        if (f.size() < 3 || !hlNum(f[1], t) || t <= 0 || !hlNum(f[2], d) || (d != 1 && d != -1)) return;
        Hl h; h.dir = (int)d; h.kind = HL_PBREV; out[t] = h;
    });
}
// "REV,<bar end, local secs>,BULL|BEAR,..." (lsCobSignal record): every CoB pullback Rev bar, keyed by LOCAL seconds
inline void parseCobRev(const std::string& text, std::map<long long, Hl>& out) {
    hlLines(text, [&](const std::string& l) {
        if (l.rfind("REV,", 0) != 0) return;
        const std::vector<std::string> f = hlSplit(l, ',', 4); long long t = 0;
        if (f.size() < 3 || !hlNum(f[1], t) || t <= 0 || (f[2] != "BULL" && f[2] != "BEAR")) return;
        Hl h; h.dir = f[2] == "BULL" ? 1 : -1; h.kind = HL_COBREV; out[t] = h;
    });
}
static const int CAP = 99;          // counts are shown as "99+" from here
static const int SHOW_MIN = 20;     // the Max number goes on a bar from this count
static const int LOOK = 1500;       // bars looked back at most (a count can only reach CAP)
enum Mode { VOL = 0, RANGE = 1 };

struct Bar { long long t = 0; double o = 0, h = 0, l = 0, c = 0, v = 0; };
struct Stat { int all = 0, same = 0; bool green = false; int mx() const { return std::max(all, same); } };
inline std::string fm(int n) { return n >= CAP ? std::string("99+") : std::to_string(n); }

// (1.1.1) lsBreakout's record file name parts, the same rules as BreakoutLogic.h bko::recFileKey / bko::recMarket (tests_volrange compares them)
inline std::string boFileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}
inline std::string boMarket(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return boFileKey(r.substr(0, 8));
}
struct Engine {
    int mode = VOL; double tick = 0.25;
    std::vector<double> V; std::vector<char> G; std::vector<long long> T; std::vector<Stat> S;   // closed bars only
    // (1.1.0) the highlight: per closed bar its price range, Wilder ATR(13) and its local time; the signal bars come from the host (files)
    std::vector<double> PR, ATR; std::vector<long long> LT; double prevC = NAN, trSum = 0; bool hlOn = false;
    std::map<long long, Hl> hlRaw, hlLocal;            // by chart time (as T) / by local seconds (as LT)
    void reset() { V.clear(); G.clear(); T.clear(); S.clear(); PR.clear(); ATR.clear(); LT.clear(); prevC = NAN; trSum = 0; }
    // +1 green, -1 red, 0 grey for closed bar i: a Rev (PB Rev or CoB Rev) always; a BO / BD only with range >= HL_ATR_X x ATR(13) of the bar before
    int highlight(int i) const {
        if (!hlOn || i < 0 || i >= (int)T.size()) return 0;
        int bo = 0, rev = 0;
        const auto a = hlRaw.find(T[(size_t)i]); if (a != hlRaw.end()) { if (a->second.kind == HL_BO) bo = a->second.dir; else rev = a->second.dir; }
        if (i < (int)LT.size()) { const auto b = hlLocal.find(LT[(size_t)i]); if (b != hlLocal.end()) { if (b->second.kind == HL_BO) { if (!bo) bo = b->second.dir; } else if (!rev) rev = b->second.dir; } }
        if (rev) return rev;
        if (bo && i >= 1 && ATR[(size_t)i - 1] > 0 && PR[(size_t)i] >= HL_ATR_X * ATR[(size_t)i - 1] - 1e-12) return bo;
        return 0;
    }
    double valueOf(const Bar& b) const {
        if (mode == VOL) return std::isfinite(b.v) && b.v > 0 ? b.v : 0.0;
        const double r = b.h - b.l; if (!std::isfinite(r) || r <= 0) return 0.0;
        return tick > 0 ? (double)std::llround(r / tick) : r;                    // whole ticks: equal ranges compare equal
    }
    // the counts of a bar with value x and colour g against the closed bars before index n
    Stat statFor(double x, bool g, int n) const {
        Stat s; s.green = g; if (!(x > 0)) return s;
        bool da = false, ds = false; const int lo = std::max(0, n - LOOK);
        for (int j = n - 1; j >= lo && !(da && ds); --j) {
            const bool big = V[(size_t)j] >= x;
            if (!da) { if (big || s.all >= CAP) da = true; else ++s.all; }
            if (!ds && (G[(size_t)j] != 0) == g) { if (big || s.same >= CAP) ds = true; else ++s.same; }
        }
        return s;
    }
    void addBar(const Bar& b, long long localT = 0) { const double x = valueOf(b); const bool g = b.c >= b.o; S.push_back(statFor(x, g, (int)V.size())); V.push_back(x); G.push_back(g ? 1 : 0); T.push_back(b.t);
        const bool dead = b.h == 0 && b.l == 0 && b.c == 0;            // (1.1.1) an unreadable bar (the host zeroes it): no true range, the last real close stays
        const double r = std::isfinite(b.h - b.l) && b.h > b.l ? b.h - b.l : 0.0; const double tr = dead ? 0.0 : std::isfinite(prevC) ? std::max(r, std::max(std::fabs(b.h - prevC), std::fabs(b.l - prevC))) : r;
        const int n = (int)PR.size(); double a = 0;                    // Wilder: the mean of the first HL_ATR_N true ranges, then (prev * (N - 1) + TR) / N
        if (n < HL_ATR_N) { trSum += tr; a = n == HL_ATR_N - 1 ? trSum / HL_ATR_N : 0; } else a = (ATR[(size_t)n - 1] * (HL_ATR_N - 1) + tr) / HL_ATR_N;
        PR.push_back(r); ATR.push_back(a); LT.push_back(localT); if (!dead && std::isfinite(b.c)) prevC = b.c; }
    Stat forming(const Bar& b) const { return statFor(valueOf(b), b.c >= b.o, (int)V.size()); }
};

// ---- the drawing (SDK-free)
struct View { int l = 0, t = 0, r = 800, b = 100, first = 0, last = 0, bw = 6, textH = 11; std::function<double(int)> X; std::function<int(const std::string&)> textW; };
struct Prim { int x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned col = 0; std::string text; bool fill = false, bold = false; };
static const unsigned C_GREY = 0x006B6B6B;
static const unsigned C_UP = 0x0022C55E, C_DN = 0x00EF4444, C_TXT = 0x00FFFFFF, C_BOXBG = 0x000D0D0D, C_BOXLN = 0x00888888;   // 0x00RRGGBB
static const int BOX_W = 92, BOX_PAD = 3;

// E = the closed bars; form / haveForm = the forming bar (drawn as a bar, never numbered); v.first / v.last = visible bar indexes
// (the forming bar's index = E.V.size())
inline std::vector<Prim> layout(const Engine& E, const View& v, const Bar* form)
{
    std::vector<Prim> P; const int n = (int)E.V.size(), W = v.r - v.l, H = v.b - v.t; if (W < 40 || H < 12 || !v.X) return P;
    const bool box = W >= BOX_W + 60; const int bx1 = v.l + 2 + BOX_W;                    // the box needs room; bars start right of it
    const int th = std::max(8, v.textH), top = v.t + th + 2;                             // room for the numbers over the tallest bar
    const double fv = form ? E.valueOf(*form) : 0.0;
    const int a = std::max(0, v.first), z = std::min(n - 1 + (form ? 1 : 0), v.last);
    auto valAt = [&](int i) { return i < n ? E.V[(size_t)i] : fv; };
    double mx = 0; for (int i = a; i <= z; ++i) { if (box && v.X(i) - v.bw / 2.0 < bx1 + 1) continue; mx = std::max(mx, valAt(i)); }
    const int hw = std::max(1, (int)(v.bw * 0.8) / 2);
    if (mx > 0 && v.b - top >= 4) for (int i = a; i <= z; ++i) {
        const double x = v.X(i); const int x0 = (int)std::lround(x) - hw, x1 = (int)std::lround(x) + hw; if (x0 < v.l || x1 > v.r || (box && x0 < bx1 + 1)) continue;
        const double val = valAt(i); if (!(val > 0)) continue;
        const bool g = i < n ? E.G[(size_t)i] != 0 : (form->c >= form->o);
        const int y = v.b - std::max(1, (int)std::lround(val / mx * (v.b - top)));
        unsigned col = g ? C_UP : C_DN;
        if (E.hlOn) { const int hd = i < n ? E.highlight(i) : 0; col = hd > 0 ? C_UP : hd < 0 ? C_DN : C_GREY; }   // (1.1.0) grey unless a BO / Rev bar
        Prim f; f.fill = true; f.x0 = x0; f.x1 = x1; f.y0 = y; f.y1 = v.b; f.col = col; P.push_back(f);
        if (i < n && E.S[(size_t)i].mx() >= SHOW_MIN) {                                  // closed bars only: decided once
            const std::string s = fm(E.S[(size_t)i].mx()); const int w = v.textW ? v.textW(s) : (int)s.size() * 6;
            Prim t; t.text = s; t.col = C_TXT; t.x0 = (int)std::lround(x - w / 2.0); t.x1 = t.x0 + w; t.y1 = y - 1; t.y0 = t.y1 - th;
            if (t.y0 < v.t) { t.y0 = v.t; t.y1 = v.t + th; }
            bool ok = t.x0 >= (box ? bx1 + 1 : v.l) && t.x1 <= v.r;
            for (int up = 0; ok && up < 3; ++up) {                                       // never over another number: one row up (twice at most), else not drawn
                bool clash = false; for (const Prim& q : P) if (!q.fill && q.x0 < t.x1 + 2 && t.x0 < q.x1 + 2 && q.y0 < t.y1 && t.y0 < q.y1) { clash = true; break; }
                if (!clash) break;
                if (up == 2 || t.y0 - th < v.t) { ok = false; break; }
                t.y0 -= th; t.y1 -= th;
            }
            if (ok) P.push_back(t);
        }
    }
    if (box) {                                                                          // the read box: the last CLOSED bar
        const int rows = 5, rh = std::max(8, std::min(th + 1, (H - 2 * BOX_PAD) / rows)); const int bh = rows * rh + 2 * BOX_PAD;
        const int x0 = v.l + 2, y0 = v.t + std::max(0, (H - bh) / 2), x1 = bx1, y1 = std::min(v.b, y0 + bh);
        Prim bg; bg.fill = true; bg.x0 = x0; bg.y0 = y0; bg.x1 = x1; bg.y1 = y1; bg.col = C_BOXBG; P.push_back(bg);
        const int E4[4][4] = { {x0, y0, x1, y0}, {x0, y1, x1, y1}, {x0, y0, x0, y1}, {x1, y0, x1, y1} };
        for (const auto& e : E4) { Prim ln; ln.fill = true; ln.x0 = e[0]; ln.y0 = e[1]; ln.x1 = e[2]; ln.y1 = e[3]; ln.col = C_BOXLN; P.push_back(ln); }
        const bool vol = E.mode == VOL; const std::string pre = vol ? "BV" : "BR";
        const bool have = n > 0; const Stat s = have ? E.S[(size_t)n - 1] : Stat();
        const std::string title = std::string(vol ? "Volume v" : "Range v") + VERSION;
        struct Row { std::string k, val; unsigned col; };
        const Row R[4] = { { pre + "Max", have ? fm(s.mx()) : "-", s.green ? C_UP : C_DN }, { pre, have ? fm(s.all) : "-", C_TXT },
                           { pre + "G", have && s.green ? fm(s.same) : "-", C_UP }, { pre + "R", have && !s.green ? fm(s.same) : "-", C_DN } };
        auto tw = [&](const std::string& q) { return v.textW ? v.textW(q) : (int)q.size() * 6; };
        { Prim t; t.bold = true; t.text = title; t.col = C_TXT; const int w = tw(title); t.x0 = std::max(x0 + 2, (x0 + x1) / 2 - w / 2); t.x1 = std::min(x1 - 1, t.x0 + w); t.y0 = y0 + BOX_PAD; t.y1 = t.y0 + rh; P.push_back(t); }
        for (int k = 0; k < 4; ++k) {
            const int y = y0 + BOX_PAD + (k + 1) * rh; if (y + rh > y1) break;
            Prim a2; a2.text = R[k].k; a2.col = C_TXT; a2.x0 = x0 + 5; a2.x1 = a2.x0 + tw(R[k].k); a2.y0 = y; a2.y1 = y + rh; P.push_back(a2);
            Prim b2; b2.bold = true; b2.text = R[k].val; b2.col = R[k].col; b2.x1 = x1 - 5; b2.x0 = b2.x1 - tw(R[k].val); b2.y0 = y; b2.y1 = y + rh; P.push_back(b2);
        }
    }
    return P;
}

inline std::string statusText(const Engine& E, const std::string& mkt, const std::string& sym, int spb)
{
    const bool vol = E.mode == VOL; const std::string pre = vol ? "BV" : "BR"; char b[200];
    std::string o = std::string("VERSION,") + VERSION + "\nPANE," + (vol ? "Volume" : "Range") + "\nMARKET," + mkt + "\nSYMBOL," + sym + "\nBAR_SECONDS," + std::to_string(spb) + "\nBARS," + std::to_string(E.V.size()) +
        "\nRULES,count of bars in a row before the bar with a smaller value (all bars / same colour); Max = the larger; cap 99+; number on closed bars with Max >= " + std::to_string(SHOW_MIN) + "; box = the last closed bar\n";
    const int n = (int)E.V.size(); int big = 0; for (const Stat& s : E.S) if (s.mx() >= SHOW_MIN) ++big;
    o += "NUMBERED," + std::to_string(big) + "\n";
    if (E.hlOn) { int g = 0, r = 0; for (int i = 0; i < n; ++i) { const int h = E.highlight(i); if (h > 0) ++g; else if (h < 0) ++r; }
        o += "HIGHLIGHT,1.1.0 grey bars; BO / BD with range >= 1.2 x ATR(13) of the bar before, every PB Rev and CoB Rev bar green / red,signals read " + std::to_string(E.hlRaw.size()) + " + " + std::to_string(E.hlLocal.size()) + ",green " + std::to_string(g) + ",red " + std::to_string(r) + "\n"; }
    for (int i = std::max(0, n - 5); i < n; ++i) { const Stat& s = E.S[(size_t)i]; std::snprintf(b, sizeof b, "LAST,%lld,%s,%.10g,%sMax=%s,%s=%s,%s%s=%s\n", E.T[(size_t)i], s.green ? "G" : "R", E.V[(size_t)i], pre.c_str(), fm(s.mx()).c_str(), pre.c_str(), fm(s.all).c_str(), pre.c_str(), s.green ? "G" : "R", fm(s.same).c_str()); o += b; }
    return o;
}

} // namespace vrr
