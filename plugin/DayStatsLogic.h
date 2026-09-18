// DayStatsLogic.h — lsDayStats' decisions on plain data, no SDK. Pinned by plugin/test_daystats_logic.cpp.
//   · the row grammar (DAYSA / DAYSE: 17 fields; CONDE: the E clocks' stage + the 2ND ladder)
//   · the clock / duration / price formatting (his clocks: "9:12am"; durations "1h 04m"; prices to the chart's contract)
//   · the twelve A and E cells exactly as the strip prints them (the '~' on expectations, "39% last hr" on the 2ND)
//   · the ACTUAL row's colour rules (LOD red / HOD green on 1ST and 2ND; MUD by the day's phase)
#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

namespace dsl {

struct StatRow {
    bool valid;
    std::string first, second, firstPx, secondPx;
    double firstClk, secondClk, wendSo;   // sec-of-day (<0 = missing)
    double took, bop, wick, mud, gap;     // minutes (<0 = missing)
    double wickPct;                       // percent (<0 = missing)
    std::string rngPts, rngUsd, rngP25, rngP75;
    StatRow() : valid(false), firstClk(-1), secondClk(-1), wendSo(-1), took(-1), bop(-1), wick(-1), mud(-1), gap(-1), wickPct(-1) {}
};
struct Cond { std::string basis; int lastHr, last30; int p20, p50, p80; Cond() : lastHr(-1), last30(-1), p20(-1), p50(-1), p80(-1) {} };   // (0.11) + the 2ND clock's percentiles, minutes after the open

inline double num(const std::string& s) { return s.empty() ? -1.0 : atof(s.c_str()); }

// DAYSA / DAYSE,<first>,<firstPx>,<firstClkSo>,<took>,<bop>,<wick>,<wendSo>,<wickPct>,<mud>,<second>,<secondPx>,<secondClkSo>,<gap>,<rngPts>,<rngUsd>[,<p25>,<p75>]
inline bool parseStatRow(const std::vector<std::string>& t, StatRow& r)
{
    if (t.size() < 16) return false;
    r.first = t[1]; r.firstPx = t[2]; r.firstClk = num(t[3]); r.took = num(t[4]); r.bop = num(t[5]); r.wick = num(t[6]);
    r.wendSo = num(t[7]); r.wickPct = num(t[8]); r.mud = num(t[9]); r.second = t[10]; r.secondPx = t[11]; r.secondClk = num(t[12]);
    r.gap = num(t[13]); r.rngPts = t[14]; r.rngUsd = t[15]; r.rngP25 = (t.size() > 16) ? t[16] : ""; r.rngP75 = (t.size() > 17) ? t[17] : "";
    r.valid = true; return true;
}
// CONDE,<basis>,<t1>,<t2>,<lod%>,<n>[,<lastHr%>,<last30%>]
inline bool parseCond(const std::vector<std::string>& t, Cond& c)
{
    if (t.size() < 6) return false;
    c.basis = t[1]; c.lastHr = (t.size() >= 7) ? atoi(t[6].c_str()) : -1; c.last30 = (t.size() >= 8) ? atoi(t[7].c_str()) : -1;
    c.p20 = (t.size() >= 9 && !t[8].empty()) ? atoi(t[8].c_str()) : -1; c.p50 = (t.size() >= 10 && !t[9].empty()) ? atoi(t[9].c_str()) : -1; c.p80 = (t.size() >= 11 && !t[10].empty()) ? atoi(t[10].c_str()) : -1;   // (0.11, panel 16.45)
    return true;
}

inline std::string clk(double so)
{
    if (so < 0) return "--";
    int s = (int)(so + 0.5), h = s / 3600, m = (s % 3600) / 60;
    const char* ap = (h < 12) ? "am" : "pm"; int h12 = h % 12; if (h12 == 0) h12 = 12;
    char b[16]; snprintf(b, sizeof(b), "%d:%02d%s", h12, m, ap); return std::string(b);
}
inline std::string dur(double m)
{
    if (m < 0) return "--";
    int mins = (int)(m + 0.5); char b[16];
    if (mins >= 60) snprintf(b, sizeof(b), "%dh %02dm", mins / 60, mins % 60); else snprintf(b, sizeof(b), "%dm", mins);
    return std::string(b);
}
inline std::string px1(const std::string& raw, double priceOff)
{
    if (raw.empty()) return "--";
    double v = atof(raw.c_str()) + priceOff; char b[16]; snprintf(b, sizeof(b), "%d", (int)(v + 0.5)); return std::string(b);
}
inline std::string pct(double v, bool tilde) { if (v < 0) return "--"; char b[16]; snprintf(b, sizeof(b), "%s%d%%", tilde ? "~" : "", (int)(v + 0.5)); return std::string(b); }

const int NCOL = 12;
// the ACTUAL row's twelve cells, exactly as printed
inline void actualCells(const StatRow& A, double priceOff, std::string* c)
{
    double mudt = (A.gap >= 0 && A.bop >= 0) ? (A.gap - A.bop) : -1;
    c[0] = "A";
    c[1] = A.first + " " + clk(A.firstClk) + " " + px1(A.firstPx, priceOff);
    c[2] = dur(A.took); c[3] = dur(A.bop); c[4] = dur(A.wick); c[5] = clk(A.wendSo); c[6] = pct(A.wickPct, false);
    c[7] = dur(A.mud); c[8] = dur(mudt);
    c[9] = A.second + " " + clk(A.secondClk) + " " + px1(A.secondPx, priceOff);
    c[10] = dur(A.gap);
    c[11] = (A.rngUsd.empty() ? std::string("--") : ("$" + A.rngUsd)) + (A.rngPts.empty() ? std::string("") : ("  " + A.rngPts + "p"));
}
// the EXPECTED row's twelve cells: '~' marks an expectation; a READ-IN 1ST prints '='; the 2ND carries its ladder
inline void expectedCells(const StatRow& E, const Cond& C, std::string* c)
{
    double mudt = (E.gap >= 0 && E.bop >= 0) ? (E.gap - E.bop) : -1;
    c[0] = "E";
    c[1] = E.first + (C.basis.rfind("read-in", 0) == 0 ? " =" : " ~") + clk(E.firstClk);
    c[2] = "~" + dur(E.took); c[3] = "~" + dur(E.bop); c[4] = "~" + dur(E.wick); c[5] = "~" + clk(E.wendSo); c[6] = pct(E.wickPct, true);
    c[7] = "~" + dur(E.mud); c[8] = "~" + dur(mudt);
    c[9] = E.second + " ~" + clk(E.secondClk) + (C.lastHr >= 0 ? ("  " + std::to_string(C.lastHr) + "% last hr") : std::string());
    c[10] = "~" + dur(E.gap);
    c[11] = (E.rngUsd.empty() ? std::string("--") : ("~$" + E.rngUsd)) + (E.rngPts.empty() ? std::string("") : ("  ~" + E.rngPts + "p"));
}
// colour rule for the ACTUAL row: 0 = plain, 1 = up (green), -1 = down (red), per column
inline void actualTone(const StatRow& A, int* tone)
{
    for (int i = 0; i < NCOL; i++) tone[i] = 0;
    bool fL = A.first == "LOD", fH = A.first == "HOD", sL = A.second == "LOD", sH = A.second == "HOD";
    tone[1] = fL ? -1 : (fH ? 1 : 0);
    tone[9] = sL ? -1 : (sH ? 1 : 0);
    tone[7] = fL ? 1 : (fH ? -1 : 0);   // MUD: markup after a first LOD (green) / markdown after a first HOD (red)
}

// (v0.10) THE ACTUAL ROW'S PRICES ARE CHART FACTS. Operator, 2026-09-17 evening: "why haven't you fixed the header values"
// — the A row's HOD price read 7722, 7718, 7716, 7711, 7710, 7708 across the evening. The panel's DAYSA prices are in
// Skylit's ES1 space (the September contract until ES1 rolled to December at 15:16 CT) and the plugin biased them by
// (live chart close − SPOT), which moves with every after-hours tick and jumped at the roll. The charted contract's own
// session high / low ARE the actual HOD / LOD, so they replace the panel's prices, with no offset at all; the range
// follows. The clocks, durations and ordering stay the panel's (they are time facts, not price facts).
inline std::string fmtPx(double v) { char b[32]; snprintf(b, sizeof(b), "%.2f", v); return std::string(b); }
inline void applyChartExtremes(StatRow& A, double chartHi, double chartLo, bool have)
{
    if (!have || !(chartHi > chartLo) || chartHi <= 0) return;
    A.firstPx  = fmtPx(A.first  == "LOD" ? chartLo : chartHi);
    A.secondPx = fmtPx(A.second == "HOD" ? chartHi : chartLo);
    double pts = chartHi - chartLo;
    char b[32]; snprintf(b, sizeof(b), "%.1f", pts); A.rngPts = b;
    snprintf(b, sizeof(b), "%d", (int)(pts * 50.0 + 0.5)); A.rngUsd = b;   // ES: $50 a point
}
// the session's high / low on the chart's own bars: the bars of `y-m-d` (the day the rows describe) inside RTH
// (08:30–15:00 CT); returns false when that day has no RTH bar on the chart
struct Ext { double hi, lo; int hiSod, loSod; };
inline bool chartExtremes(const int* y, const int* m, const int* d, const double* sod, const float* hi, const float* lo, int n,
                          int wy, int wm, int wd, Ext& out)
{
    bool any = false; out.hi = -1; out.lo = 1e12; out.hiSod = out.loSod = -1;
    for (int i = 0; i < n; i++) {
        if (y[i] != wy || m[i] != wm || d[i] != wd) continue;
        if (sod[i] < 30600 || sod[i] > 54000) continue;
        if (hi[i] > out.hi) { out.hi = hi[i]; out.hiSod = (int)sod[i]; }
        if (lo[i] < out.lo) { out.lo = lo[i]; out.loSod = (int)sod[i]; }
        any = true;
    }
    return any;
}

// (0.11) THE SECOND HALF OF THE READ LINE — operator, 2026-09-17: "HOD in X%, LOD after <time> 80% ... so it predicts if the
// first extremity is in and when the 2nd extremity will be in with high probability". One rung at a time, descending as
// the session passes each clock, so the line never states a probability the clock has already falsified:
//   before p20  -> "LOD after 10:42  80%"   (on 80% of the stage's days the 2ND printed after p20)
//   before p50  -> "LOD after 12:57  50%"
//   before 14:00 -> "LOD 39% last hr"       (the ladder the E row already prints)
//   before 14:30 -> "LOD 29% last 30"
//   after       -> "LOD any minute"
//   after the close, with the actual 2ND known -> "LOD IN 9:09am" (the A row's second clock)
// second = the extreme the READ did not call ("LOD" when the READ says HOD IN). nowSo = chart clock, CT sec-of-day.
inline std::string secondRung(const std::string& second, const Cond& C, double nowSo, bool closed, double actualSecondSo)
{
    const double OPEN = 30600.0;   // 08:30 CT
    if (second.empty()) return "";
    if (closed && actualSecondSo >= 0) return second + " IN " + clk(actualSecondSo);
    char b[64];
    if (C.p20 >= 0 && nowSo < OPEN + C.p20 * 60.0) { snprintf(b, sizeof(b), "%s after %s  80%%", second.c_str(), clk(OPEN + C.p20 * 60.0).c_str()); return b; }
    if (C.p50 >= 0 && nowSo < OPEN + C.p50 * 60.0) { snprintf(b, sizeof(b), "%s after %s  50%%", second.c_str(), clk(OPEN + C.p50 * 60.0).c_str()); return b; }
    if (C.lastHr >= 0 && nowSo < 50400.0) { snprintf(b, sizeof(b), "%s %d%% last hr", second.c_str(), C.lastHr); return b; }
    if (C.last30 >= 0 && nowSo < 52200.0) { snprintf(b, sizeof(b), "%s %d%% last 30", second.c_str(), C.last30); return b; }
    return second + " any minute";
}

// (0.12) THE SECOND EXTREME'S READ — READ2,<side>,<p in 0-100>,<d>,<minsLeft>,<age>,<share>,<arrival min or blank>,<n>,<src>
// (panel 16.46: the logistic "is the running extreme on the other side the day's final one", refit nightly). The line:
//   "LOD IN 80%"                                  p >= 50: the probability alone
//   "LOD IN 25%  · if not, ~10:09am (50%)"        p < 50: plus the median first-passage arrival from the bar the row was written at
//   after the close with the actual 2ND known:    "LOD IN 9:09am" (the A row's clock)
// Operator, 2026-09-17: "I want to know if the extremity is in or not and the likelihood" — one number per extreme, rising with evidence.
struct Read2 { std::string side, src; int p; double d, minsLeft, age, share, arrival; long n; bool valid; Read2() : p(-1), d(0), minsLeft(-1), age(-1), share(0), arrival(-1), n(0), valid(false) {} };
inline bool parseRead2(const std::vector<std::string>& t, Read2& r)
{
    if (t.size() < 7 || (t[0] != "READ2" && t[0] != "READ1")) return false;   // (v0.14) READ1 = the first extreme's read, same shape
    r.side = t[1]; r.p = atoi(t[2].c_str()); r.d = atof(t[3].c_str()); r.minsLeft = num(t[4]); r.age = num(t[5]); r.share = atof(t[6].c_str());
    r.arrival = (t.size() >= 8 && !t[7].empty()) ? atof(t[7].c_str()) : -1; r.n = (t.size() >= 9) ? atol(t[8].c_str()) : 0; r.src = (t.size() >= 10) ? t[9] : "";
    r.valid = (r.side == "HOD" || r.side == "LOD") && r.p >= 0 && r.p <= 100;
    return r.valid;
}
inline std::string secondLine(const Read2& r, double asofSo, bool closed, double actualSecondSo)
{
    if (closed && actualSecondSo >= 0 && !r.side.empty()) return r.side + " IN " + clk(actualSecondSo);
    if (!r.valid) return "";
    char b[80]; snprintf(b, sizeof(b), "%s IN %d%%", r.side.c_str(), r.p);
    std::string s = b;
    if (r.p < 50 && r.arrival >= 0 && asofSo >= 0) s += "  · if not, ~" + clk(asofSo + r.arrival * 60.0) + " (50%)";
    return s;
}
// (v0.13) THE WHOLE READ LINE. The FIRST extreme always leads — operator, 2026-09-17: "if the lod occurs, it should be
// before the HOD and vice versa" — so the left half is whichever extreme printed first (the READ row's side while the
// session runs; the A row's 1ST after the close). After the close, with the A row known, the left half prints the
// clock exactly as the right half does ("HOD IN 8:33am  ·  LOD IN 9:09am") — 0.12 left it at "HOD IN 96%" beside
// "LOD IN 9:09am", one half live and one half settled, which the operator saw on the first evening.
//   first: HOD|LOD · call: IN|NOTIN|"" · pct: the cell % (<0 = none) · closed + actualFirstSo: the A row's 1ST clock
//   second: the right half, already composed by secondLine ("" = none)
// (v0.14) THE FIRST HALF FROM THE SAME MODEL. Operator: "both .. do it" — the READ1 row (panel 16.48, the secondin logistic
// on the first extreme: AUC 0.859, calibrated) drives the left half exactly as READ2 drives the right: "HOD IN 74%", and
// "· if not, ~11:42am (50%)" while p < 50. The HLTAB cell (call / pct) is the fallback only while READ1 is absent — the
// first 36 minutes, before the model's gate — so the line reads from 8:45 and switches once, at 9:06.
inline std::string firstHalf(const Read2& r1, const std::string& first, const std::string& call, double pct, bool closed, double actualFirstSo, double asofSo)
{
    if (first.empty()) return "";
    if (closed && actualFirstSo >= 0) return first + " IN " + clk(actualFirstSo);
    if (r1.valid && r1.side == first) return secondLine(r1, asofSo, false, -1);
    std::string rl = first;
    if      (call == "IN")    rl += " IN";
    else if (call == "NOTIN") rl += " NOT IN";
    if (pct >= 0) { char pb[16]; snprintf(pb, sizeof(pb), "  %d%%", (int)(pct + 0.5)); rl += pb; }
    return rl;
}
// the tone of the line: 1 green (>= 70, or IN), -1 amber (<= 30, or NOT IN), 0 plain — from the model's p when READ1 drives it
inline int firstTone(const Read2& r1, const std::string& first, const std::string& call, bool closed, double actualFirstSo)
{
    if (closed && actualFirstSo >= 0) return 1;
    if (r1.valid && r1.side == first) return r1.p >= 70 ? 1 : (r1.p <= 30 ? -1 : 0);
    return call == "IN" ? 1 : (call == "NOTIN" ? -1 : 0);
}
inline std::string readLine(const std::string& first, const std::string& call, double pct, bool closed, double actualFirstSo, const std::string& second)
{
    if (first.empty()) return "";
    std::string rl;
    if (closed && actualFirstSo >= 0) rl = first + " IN " + clk(actualFirstSo);
    else {
        rl = first;
        if      (call == "IN")    rl += " IN";
        else if (call == "NOTIN") rl += " NOT IN";
        if (pct >= 0) { char pb[16]; snprintf(pb, sizeof(pb), "  %d%%", (int)(pct + 0.5)); rl += pb; }
    }
    if (!second.empty()) rl += "   \xB7   " + second;
    return rl;
}
// (v0.13) THE PANEL'S LEFT EDGE. Operator, 2026-09-17: "its being cut off, move to center". The block is anchored to a
// corner; on a pane narrower than the block a right-anchored panel starts left of the pane and its first columns are
// gone. Centre anchors (list entries 4 / 5) put the block on the pane's midline; every anchor is then clamped so the
// block never starts left of pane.left + inset — the RIGHT edge is what overflows, and the READ line stays visible.
//   corner: 0 TL · 1 TR · 2 BL · 3 BR · 4 top-centre · 5 bottom-centre
inline int anchorX(int corner, int paneL, int paneR, int blockW, int xoff)
{
    int x;
    if      (corner == 1 || corner == 3) x = paneR - xoff - blockW;
    else if (corner == 4 || corner == 5) x = (paneL + paneR) / 2 - blockW / 2;
    else                                 x = paneL + xoff;
    if (x < paneL + xoff) x = paneL + xoff;
    return x;
}
inline bool anchorBottom(int corner) { return corner == 2 || corner == 3 || corner == 5; }

} // namespace dsl
