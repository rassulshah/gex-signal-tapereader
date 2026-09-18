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
struct Cond { std::string basis; int lastHr, last30; Cond() : lastHr(-1), last30(-1) {} };

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

} // namespace dsl
