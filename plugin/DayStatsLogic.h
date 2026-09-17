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

} // namespace dsl
