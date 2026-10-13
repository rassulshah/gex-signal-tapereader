// FirstExtremeView.h - what the First Extreme indicator shows, from the native model (FirstExtremeLogic.h). Pure C++ (no SDK).
//   the panel: "1st EXTREME · shadow   In 61%" / "LOD 4,178.1 7:39 38%   HOD 4,199.3 8:30 23%" / the state line
//   the marker: on the called extreme's candle, "LOD? 74%" + "called 8:51" (the call is made later than the candle - the
//   declaration time is always shown so the mark never looks earlier than it was); failed = grey with the time it broke
// States (the review's chart design): UNAVAILABLE (reason) / WAITING (first read 9 min after the open) / WATCH (no call yet) /
// CALLED (the locked 70% call, immutable) / FAILED (its level broke) / NO CALL (deadline passed) / VERDICT after the close.
#pragma once
#include "FirstExtremeLogic.h"
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace fx2 {

static const unsigned C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_GREY = 0x00B8BEC8, C_DIM = 0x00707884, C_WHITE = 0x00F1F5F9,
                      C_AMBER = 0x00F59E0B, C_BG = 0x000B1220, C_EDGE = 0x00334155;
static inline unsigned rgb(unsigned c) { return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF); }   // RGB constant -> COLORREF (BGR)

enum State { ST_UNAVAILABLE = 0, ST_WAITING, ST_WATCH, ST_CALLED, ST_FAILED, ST_NOCALL };

struct View {
    int m = -1; State state = ST_UNAVAILABLE; std::string why;
    long long day = -1; int proxy = 0, deadline = 0, nBars = 0; bool complete = false;
    bool haveProb = false; int elapsed = 0; double pLow = 0, pHigh = 0, pIn = 0, lo = 0, hi = 0; int loForm = 0, hiForm = 0;
    // the immutable call
    bool called = false; int side = 0; double price = 0, prob = 0; int formed = 0, declared = 0;
    bool failed = false; int failedAt = 0;          // elapsed minutes of the bar that broke it
    long long formedEnd = 0;                        // the called candle's END stamp (local CT seconds)
    // after the close
    bool verdictKnown = false; int verdict = 0;     // 1 right, -1 wrong, 0 unresolved (same-bar order)
    std::vector<Decision> decisions;                // every assessed bar today (for the log)
    int callDecision = -1;
};

inline std::string clk(int mod)                     // 13:54 -> "1:54" (12-hour CT)
{ mod = ((mod % 1440) + 1440) % 1440; int h = mod / 60; char b[16]; snprintf(b, sizeof b, "%d:%02d", h % 12 == 0 ? 12 : h % 12, mod % 60); return b; }
inline int decimalsOf(int m) { static const int D[7] = { 2, 2, 2, 3, 1, 4, 5 }; return m >= 0 && m < 7 ? D[m] : 2; }
inline std::string px(double v, int m)
{
    char b[48]; snprintf(b, sizeof b, "%.*f", decimalsOf(m), v); std::string s = b;
    size_t dot = s.find('.'); if (dot == std::string::npos) dot = s.size();
    size_t start = (s[0] == '-') ? 1 : 0;
    for (long long k = (long long)dot - 3; k > (long long)start; k -= 3) s.insert((size_t)k, ",");
    return s;
}
inline std::string pct(double p) { char b[16]; snprintf(b, sizeof b, "%d%%", (int)std::lround(p * 100)); return b; }

// the view for one market from its chart's closed bars (chronological). `day` = the session to show (the last closed bar's day).
inline View buildView(int m, const std::vector<Bar>& bars, long long day)
{
    View V; V.m = m; V.day = day; V.proxy = proxyOpen(m); V.deadline = MKTS[m].deadline;
    Built B = buildSessions(m, bars, day);
    if (!B.haveToday) { V.why = B.todayWhy; return V; }
    const Session& s = B.today;
    V.nBars = (int)s.c.size(); V.complete = V.nBars == (MKTS[m].close - V.proxy) / 3;
    if (!std::isfinite(s.scale)) {
        int have = 0; for (size_t k = 0; k < B.complete.size(); k++) if (B.complete[k].day < day) have++;
        V.why = "warming up: needs 14 complete sessions in a row on the chart"; return V;
    }
    if (s.scale <= MKTS[m].tick) { V.why = "scale too small"; return V; }
    V.decisions = runSession(m, s);
    if (V.decisions.empty()) { V.state = ST_WAITING; V.why = "first read at " + clk(V.proxy + 9); return V; }
    const Decision& L = V.decisions.back();
    V.haveProb = L.probOk; V.elapsed = L.elapsed; V.pLow = L.p[1]; V.pHigh = L.p[2]; V.pIn = L.p[1] + L.p[2];
    V.lo = L.lo; V.hi = L.hi; V.loForm = L.loForm; V.hiForm = L.hiForm;
    // the locked policy, replayed over today's assessed bars: the first call is immutable
    FirstExtremeState st; st.reset_session(); FirstExtremePolicy P = lockedPolicy(m);
    for (size_t k = 0; k < V.decisions.size(); k++) {
        const Decision& D = V.decisions[k];
        if (!D.probOk) continue;
        if (st.assess(P, D.elapsed, D.lo, D.hi, D.loForm, D.hiForm, D.gateL, D.gateH, D.p, true)) { V.callDecision = (int)k; break; }
    }
    if (st.marker.exists) {
        V.called = true; V.side = st.marker.side; V.price = st.marker.candidate_price; V.prob = st.marker.probability;
        V.formed = st.marker.candidate_elapsed_minutes; V.declared = st.marker.declared_elapsed_minutes;
        V.formedEnd = s.day * 86400 + (long long)(V.proxy + V.formed) * 60;
        // failure: a strict break of the called level on ANY later bar (not only the assessed ones)
        int i0 = V.declared / 3;                    // bars after the declaration bar
        const double t2 = MKTS[m].tick / 2;
        for (int i = i0; i < V.nBars; i++) {
            if ((V.side == 1 && s.l[i] < V.price - t2) || (V.side == 2 && s.h[i] > V.price + t2)) { V.failed = true; V.failedAt = (i + 1) * 3; break; }
        }
        V.state = V.failed ? ST_FAILED : ST_CALLED;
    } else V.state = V.elapsed > V.deadline ? ST_NOCALL : ST_WATCH;
    if (V.complete) {                               // the exact-first verdict (the package's label)
        int hiidx = 0, loidx = 0;
        for (int i = 1; i < V.nBars; i++) { if (s.h[i] > s.h[hiidx]) hiidx = i; if (s.l[i] < s.l[loidx]) loidx = i; }
        int first = loidx < hiidx ? 1 : (hiidx < loidx ? 2 : 0);
        if (V.called) {
            V.verdictKnown = true;
            if (first == 0) V.verdict = 0;
            else {
                int idx = V.side == 1 ? loidx : hiidx;
                V.verdict = (first == V.side && (idx + 1) * 3 == V.formed) ? 1 : -1;
            }
        }
    }
    return V;
}

// ------------------------------------------------------------------------------------------------ layout (draw list)
enum { K_FILL = 0, K_RECT = 1, K_LINE = 2, K_TEXT = 3 };
struct Item { int k = 0, l = 0, t = 0, r = 0, b = 0; unsigned c1 = 0, c2 = 0; std::string s; int fs = 10; bool bold = false; int w = 1; };
struct Box { int l, t, r, b; };
struct Seg { std::string s; unsigned col; bool bold; };
using Measure = std::function<int(const std::string&, int, bool)>;
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
static const int POS_DEFAULT = 1;   // Top centre

struct Layout { std::vector<Item> items; Box panel{ 0, 0, 0, 0 }; bool markerOk = false; std::vector<std::vector<Seg>> lines; };

inline std::vector<std::vector<Seg>> panelLines(const View& V)
{
    std::vector<std::vector<Seg>> L;
    std::string mk = V.m >= 0 ? MKTS[V.m].code : "";
    std::vector<Seg> t; t.push_back(Seg{ "1st EXTREME", C_GREY, true }); t.push_back(Seg{ "  shadow", C_DIM, false });
    if (V.haveProb) { t.push_back(Seg{ "   In ", C_GREY, false }); t.push_back(Seg{ pct(V.pIn), C_WHITE, true }); t.push_back(Seg{ "  " + clk(V.proxy + V.elapsed), C_DIM, false }); }
    L.push_back(t);
    if (V.state == ST_UNAVAILABLE || V.state == ST_WAITING) {
        std::vector<Seg> r; r.push_back(Seg{ V.why.empty() ? "no reading" : V.why, C_GREY, false }); L.push_back(r); return L;
    }
    std::vector<Seg> p;
    p.push_back(Seg{ "LOD ", C_GREEN, true }); p.push_back(Seg{ px(V.lo, V.m) + " " + clk(V.proxy + V.loForm) + "  ", C_GREY, false }); p.push_back(Seg{ pct(V.pLow), C_GREEN, true });
    p.push_back(Seg{ "     HOD ", C_RED, true }); p.push_back(Seg{ px(V.hi, V.m) + " " + clk(V.proxy + V.hiForm) + "  ", C_GREY, false }); p.push_back(Seg{ pct(V.pHigh), C_RED, true });
    L.push_back(p);
    std::vector<Seg> s;
    const unsigned sc = V.side == 1 ? C_GREEN : C_RED; const std::string sn = V.side == 1 ? "LOD?" : "HOD?";
    switch (V.state) {
        case ST_WATCH:   s.push_back(Seg{ "WATCH", C_AMBER, true }); s.push_back(Seg{ " \xB7 a call needs 70% by " + clk(V.proxy + V.deadline), C_GREY, false }); break;
        case ST_NOCALL:  s.push_back(Seg{ "NO CALL", C_GREY, true }); s.push_back(Seg{ " \xB7 nothing reached 70% by " + clk(V.proxy + V.deadline), C_GREY, false }); break;
        case ST_CALLED:  s.push_back(Seg{ sn + " " + pct(V.prob), sc, true }); s.push_back(Seg{ " " + px(V.price, V.m) + " \xB7 formed " + clk(V.proxy + V.formed) + " \xB7 called " + clk(V.proxy + V.declared), C_GREY, false }); break;
        case ST_FAILED:  s.push_back(Seg{ sn + " FAILED", C_GREY, true }); s.push_back(Seg{ " " + px(V.price, V.m) + " broke " + clk(V.proxy + V.failedAt) + " \xB7 called " + clk(V.proxy + V.declared), C_GREY, false }); break;
        default: break;
    }
    if (V.verdictKnown) {
        s.push_back(Seg{ "   ", C_GREY, false });
        s.push_back(V.verdict == 1 ? Seg{ "RIGHT", C_GREEN, true } : V.verdict == -1 ? Seg{ "WRONG", C_RED, true } : Seg{ "UNRESOLVED", C_GREY, true });
    }
    if (!s.empty()) L.push_back(s);
    return L;
}

inline int segW(const std::vector<Seg>& L, const Measure& M, int fs) { int w = 0; for (const auto& g : L) w += M(g.s, fs, g.bold); return w; }

// pane = the price pane; priceEdge = x of the last bar's right edge (the box never uses the empty space right of price);
// markerX = the called candle's x (-1 = off the chart); yOf = price -> y
inline Layout layoutAll(const View& V, const Box& pane, const Measure& M, int priceEdge, int pos, int markerX, const std::function<int(double)>& yOf)
{
    Layout Lo; const int FS = 10, LH = 17, PAD = 7;
    Lo.lines = panelLines(V);
    int w = 0; for (const auto& l : Lo.lines) w = std::max(w, segW(l, M, FS));
    w += 2 * PAD; int h = (int)Lo.lines.size() * LH + 2 * PAD - 4;
    int right = (priceEdge > pane.l + 100 && priceEdge < pane.r) ? priceEdge : pane.r;
    int col = pos % 3, row = pos / 3;
    int x = col == 0 ? pane.l + 6 : col == 1 ? (pane.l + right) / 2 - w / 2 : right - w - 6;
    int y = row == 0 ? pane.t + 6 : row == 1 ? (pane.t + pane.b) / 2 - h / 2 : pane.b - h - 6;
    x = std::max(pane.l + 2, std::min(x, pane.r - w - 2)); y = std::max(pane.t + 2, std::min(y, pane.b - h - 2));
    Lo.panel = { x, y, x + w, y + h };
    Item bg; bg.k = K_RECT; bg.l = x; bg.t = y; bg.r = x + w; bg.b = y + h; bg.c1 = C_EDGE; bg.c2 = C_BG; bg.w = 1; Lo.items.push_back(bg);
    for (size_t i = 0; i < Lo.lines.size(); i++) {
        int cx = x + PAD, cy = y + PAD + (int)i * LH;
        for (const auto& g : Lo.lines[i]) {
            Item t; t.k = K_TEXT; t.l = cx; t.t = cy; t.r = cx + M(g.s, FS, g.bold); t.b = cy + LH - 4; t.c1 = g.col; t.s = g.s; t.fs = FS; t.bold = g.bold;
            Lo.items.push_back(t); cx = t.r;
        }
    }
    // the marker on the called candle
    if (V.called && markerX >= pane.l && markerX <= pane.r) {
        const bool low = V.side == 1; const unsigned c = V.failed ? C_DIM : (low ? C_GREEN : C_RED);
        std::string l1 = (low ? "LOD? " : "HOD? ") + (V.failed ? std::string("\xD7") : pct(V.prob));
        std::string l2 = V.failed ? "broke " + clk(V.proxy + V.failedAt) : "called " + clk(V.proxy + V.declared);
        int yp = yOf(V.price), mw = std::max(M(l1, 9, true), M(l2, 9, false)) + 10, mh = 30;
        // the label: below the low / above the high; if that hits the panel or the pane edge, beside the candle (right, then left)
        auto clash = [&](int l, int t) { const Box& P = Lo.panel; return (l < P.r + 4 && l + mw > P.l - 4 && t < P.b + 4 && t + mh > P.t - 4) ||
                                          t < pane.t + 2 || t + mh > pane.b - 2 || l < pane.l + 2 || l + mw > pane.r - 2; };
        int mx = markerX - mw / 2, my = low ? yp + 10 : yp - 10 - mh; bool beside = false;
        if (clash(mx, my)) { int bx_ = markerX + 14, by_ = yp - mh / 2; if (clash(bx_, by_)) bx_ = markerX - 14 - mw; mx = bx_; my = by_; beside = true; }
        mx = std::max(pane.l + 2, std::min(mx, pane.r - mw - 2)); my = std::max(pane.t + 2, std::min(my, pane.b - mh - 2));
        Item tk; tk.k = K_LINE; tk.l = markerX - 9; tk.r = markerX + 9; tk.t = tk.b = yp; tk.c1 = c; tk.w = 2; Lo.items.push_back(tk);
        Item st; st.k = K_LINE; st.c1 = c; st.w = 1;
        if (beside) { st.t = st.b = yp; st.l = mx > markerX ? markerX + 9 : mx + mw; st.r = mx > markerX ? mx : markerX - 9; }
        else { st.l = st.r = markerX; st.t = low ? yp + 1 : my + mh; st.b = low ? my : yp - 1; }
        Lo.items.push_back(st);
        Item bx; bx.k = K_RECT; bx.l = mx; bx.t = my; bx.r = mx + mw; bx.b = my + mh; bx.c1 = c; bx.c2 = C_BG; bx.w = 1; Lo.items.push_back(bx);
        Item a; a.k = K_TEXT; a.l = mx + 5; a.t = my + 2; a.r = mx + mw - 5; a.b = my + 15; a.c1 = c; a.s = l1; a.fs = 9; a.bold = true; Lo.items.push_back(a);
        Item b; b.k = K_TEXT; b.l = mx + 5; b.t = my + 15; b.r = mx + mw - 5; b.b = my + 28; b.c1 = C_GREY; b.s = l2; b.fs = 9; Lo.items.push_back(b);
        Lo.markerOk = true;
    }
    return Lo;
}

// one line per assessed bar (the shadow forecast log) and the call line - the nightly scorer reads these
inline std::string decisionLine(const View& V, const Decision& D, const std::string& dayTxt)
{
    char b[400];
    snprintf(b, sizeof b, "%s,%d,%.10g,%.10g,%d,%d,%.6f,%.6f,%.6f,%d,%d,%s\n", dayTxt.c_str(), D.elapsed, D.lo, D.hi, D.loForm, D.hiForm,
             D.p[0], D.p[1], D.p[2], (int)D.gateL, (int)D.gateH, FX_VERSION);
    (void)V; return b;
}
static const char* const DECISION_HEADER = "day,elapsed,low,high,low_formed,high_formed,p_neither,p_low,p_high,gate_low,gate_high,version\n";
static const char* const CALL_HEADER = "day,declared,side,price,probability,formed,deadline,threshold,version,model\n";
inline std::string callLine(const View& V, const std::string& dayTxt)
{
    char b[400];
    snprintf(b, sizeof b, "%s,%d,%s,%.10g,%.6f,%d,%d,0.70,%s,%s\n", dayTxt.c_str(), V.declared, V.side == 1 ? "LOD" : "HOD", V.price, V.prob, V.formed,
             V.deadline, FX_VERSION, "v2-locked-primary");
    return b;
}

}  // namespace fx2
