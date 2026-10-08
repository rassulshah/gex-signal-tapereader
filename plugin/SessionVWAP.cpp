/********************************************************************************
 *  SessionVWAP.cpp  --  Investor/RT RTX extension  lsSessionVWAP  (1.0.0, 2026-10-08)
 *
 *  THE DAY-SESSION VWAP (Rassul 2026-10-08 11:15-11:17: "is it better to use irt indicator for vwap or your own ... especially for
 *  testing and studies?" -> "make me a day session vwap indicator, it should have the same 1sd and 2 sd bands and the probabilities
 *  badge label should just be the percentage like 42%").
 *
 *  ONE definition, the same in this plugin, in the LRA research (lra.level_touch.vwap_levels, the 17-month replay) and in the live
 *  odds, so the line you trade, the badge and the tested numbers all describe the same thing:
 *    session   each day's RTH only: ES / NQ 08:30-15:00, CL / NG 08:00-13:30, GC / HG 07:21-12:30, 6E 07:21-14:00 (bar START at or
 *              after the open, bar END at or before the close - lra.markets); nothing is drawn outside RTH
 *    price     the typical price of each chart bar, (high + low + close) / 3, weighted by the bar's volume, from the RTH open
 *    bands     +-1 / +-2 SD, SD = the volume-weighted spread of the typical price around VWAP (sqrt(sum v tp^2 / sum v - VWAP^2))
 *  Five output lines (VWAP, +1 SD, -1 SD, +2 SD, -2 SD) drawn by IRT - colour / width editable in the settings like any indicator,
 *  and readable by other plugins by name (the Dealer Profile's bar file can carry them).
 *
 *  BADGES: at the right end of each line, the chance price touches it within the next 60 minutes - just "42%" - from
 *  %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Levels-<MKT>.csv (lra.level_touch, every 5 min: LEVEL rows VWAP / VWAP+1 / VWAP-1 /
 *  VWAP+2 / VWAP-2, the 60-min column). No badge when the file is older than 20 min, says STALE, or has no row for that line.
 *  IRT runs ONE object per DLL for every chart: the market is read from the chart's own symbol on every calc / draw.
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

static const char* SV_VERSION = "1.0.0";
static const COLOR C_VWAP = 0x00E5E7EB;   // 0x00RRGGBB (as every LRA plugin): VWAP light grey
static const COLOR C_SD1  = 0x0022D3EE;   // +-1 SD cyan
static const COLOR C_SD2  = 0x00C084FC;   // +-2 SD violet
static const COLOR C_DARK = 0x000B0F19;
static const char* CODES[5] = { "VWAP", "VWAP+1", "VWAP-1", "VWAP+2", "VWAP-2" };

struct SVIdx { int font; };
static SVIdx SV;

class SessionVWAP : public cppExtension {
public:
    SessionVWAP() : cppExtension() {}
    virtual int draw(void);
    int fontPt = 9;
    std::string mkt;
    void fill(int from);
    void writeStatus(const char* what, int nRth);
    bool rthOf(const std::string& m, int& openMin, int& closeMin);
    // the odds file
    long long oddsStamp = -2; std::string oddsMkt; std::map<std::string, int> p60; bool oddsStale = true; time_t oddsAt = 0;
    void loadOdds();
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }
int cppExtension::calc(int iStartBar) { static_cast<SessionVWAP*>(this)->fill(iStartBar); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(6);
    CPEN p0(C_VWAP, 2, P_SOLID), p1(C_SD1, 1, P_SOLID), p2(C_SD2, 1, P_SOLID);
    setOutputParameter("VWAP", DRAW_CONNECTEDLINE, &p0, C_VWAP, OUTPUT_ENABLED);
    setOutputParameter("VWAP +1 SD", DRAW_CONNECTEDLINE, &p1, C_SD1, OUTPUT_ENABLED);
    setOutputParameter("VWAP -1 SD", DRAW_CONNECTEDLINE, &p1, C_SD1, OUTPUT_ENABLED);
    setOutputParameter("VWAP +2 SD", DRAW_CONNECTEDLINE, &p2, C_SD2, OUTPUT_ENABLED);
    setOutputParameter("VWAP -2 SD", DRAW_CONNECTEDLINE, &p2, C_SD2, OUTPUT_ENABLED);
    for (int a = 0; a < 5; a++) setArrayDrawingFlags(a, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));   // no line to 0 outside RTH
    SV.font = getParameterCount(); setIntegerParameter("Badge font pt", 9, 60);
    return RTX_OK;
}

bool SessionVWAP::rthOf(const std::string& m, int& openMin, int& closeMin)
{
    // the RTH of lra.markets (rth_first - 3 = the first bar's START, rth_last = the last bar's END), minutes of the day, Central
    if (m == "ES" || m == "NQ") { openMin = 8 * 60 + 30; closeMin = 15 * 60; return true; }
    if (m == "CL" || m == "NG") { openMin = 8 * 60;      closeMin = 13 * 60 + 30; return true; }
    if (m == "GC" || m == "HG") { openMin = 7 * 60 + 21; closeMin = 12 * 60 + 30; return true; }
    if (m == "EU")              { openMin = 7 * 60 + 21; closeMin = 14 * 60; return true; }
    return false;
}

void SessionVWAP::fill(int from)
{
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    mkt = dl::marketForRoot(rs ? rs : "");
    long n = getBarCount(); if (n < 2) return;
    RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5);
    RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    RTARRAYI vo(barVolume), dt(barDateTime);
    int openMin = 0, closeMin = 0;
    if (!rthOf(mkt, openMin, closeMin)) { for (int i = 0; i < (int)n; i++) { o1[i] = o2[i] = o3[i] = o4[i] = o5[i] = 0; } writeStatus("unknown market", 0); return; }
    // the bar size from the chart's own stamps (IRT stamps a bar at its CLOSE): the smallest positive gap of the last bars
    int per = 0;
    for (int i = (int)n - 1; i > 0 && i > (int)n - 40; i--) {
        long long d = (long long)dt[i] - (long long)dt[i - 1];
        if (d > 0 && (per == 0 || d < per)) per = (int)d;
    }
    if (per <= 0 || per > 3600) per = 180;
    auto tmOf = [&](int i, struct tm& t) { memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t); };
    auto dayKey = [](const struct tm& t) { return (t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday; };
    // recompute from the first bar of that day so the running sums start at the day's RTH open
    int s = from < 0 ? 0 : (from >= (int)n ? (int)n - 1 : from);
    { struct tm t0; tmOf(s, t0); int k0 = dayKey(t0); while (s > 0) { struct tm t; tmOf(s - 1, t); if (dayKey(t) != k0) break; s--; } }
    double sv = 0, spv = 0, sp2v = 0; int curDay = -1, nRth = 0;
    for (int i = s; i < (int)n; i++) {
        struct tm t; tmOf(i, t);
        int dk = dayKey(t);
        if (dk != curDay) { sv = spv = sp2v = 0; curDay = dk; }
        int endMin = t.tm_hour * 60 + t.tm_min;                         // the bar's close stamp
        int startMin = endMin - per / 60;
        if (startMin < openMin || endMin > closeMin || t.tm_wday == 0 || t.tm_wday == 6) { o1[i] = o2[i] = o3[i] = o4[i] = o5[i] = 0; continue; }
        double tp = ((double)hi[i] + (double)lo[i] + (double)cl[i]) / 3.0;
        double v = (double)vo[i]; if (v < 0) v = 0;
        sv += v; spv += tp * v; sp2v += tp * tp * v;
        double vw = sv > 0 ? spv / sv : tp;
        double var = sv > 0 ? sp2v / sv - vw * vw : 0.0; if (var < 0) var = 0;
        double sd = std::sqrt(var);
        o1[i] = (float)vw; o2[i] = (float)(vw + sd); o3[i] = (float)(vw - sd); o4[i] = (float)(vw + 2 * sd); o5[i] = (float)(vw - 2 * sd);
        nRth++;
    }
    writeStatus("calc", nRth);
}

void SessionVWAP::loadOdds()
{
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) { oddsStale = true; return; }
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Levels-" + mkt + ".csv";
    long long st = dl::fileStamp(p);
    if (st == oddsStamp && mkt == oddsMkt) return;
    oddsStamp = st; oddsMkt = mkt; p60.clear(); oddsStale = false; oddsAt = st > 0 ? (time_t)(st / 1000003LL) : 0;
    std::ifstream f(p.c_str()); std::string ln;
    if (!f.is_open()) { oddsStale = true; return; }
    while (std::getline(f, ln)) {
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        std::vector<std::string> c = dl::split(ln, '|');
        if (c.size() >= 2 && c[0] == "STATE" && c[1] == "STALE") oddsStale = true;
        // LEVEL|code|name|price|family|P15|P30|P45|P60|P90|Pclose|validated|label|Pexp - code may hold aliases: "VWAP+1/PDH"
        if (c.size() >= 9 && c[0] == "LEVEL" && !c[8].empty()) {
            std::vector<std::string> names = dl::split(c[1], '/');
            for (size_t k = 0; k < names.size(); k++) for (int j = 0; j < 5; j++) if (names[k] == CODES[j]) p60[CODES[j]] = atoi(c[8].c_str());
        }
    }
}

int SessionVWAP::draw(void)
{
    long n = getBarCount(); if (n < 2) return RTX_OK;
    { int f_ = getIntegerValue(SV.font); if (f_ >= 6 && f_ <= 18) fontPt = f_; }
    loadOdds();
    if (oddsStale || oddsAt <= 0 || time(nullptr) - oddsAt > 20 * 60 || p60.empty()) return RTX_OK;   // no fresh odds: lines only
    RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5);
    int last = (int)n - 1;
    float v[5] = { o1[last], o2[last], o3[last], o4[last], o5[last] };
    if (!(v[0] > 0)) return RTX_OK;                                     // outside RTH: no lines, no badges
    const COLOR col[5] = { C_VWAP, C_SD1, C_SD1, C_SD2, C_SD2 };
    RCT pane; pane.getPaneRect(false);
    FONT f; f.id = HELVETICA; f.size = (short)fontPt; f.style = BOLD; setFont(f);
    std::vector<std::pair<short, int>> ys;                              // badge y, line index - placed top to bottom without overlap
    for (int j = 0; j < 5; j++) {
        if (!p60.count(CODES[j]) || !(v[j] > 0)) continue;
        PNT p; p.set(last, v[j], kBarCenter);
        if (p.v < pane.top + 4 || p.v > pane.bottom - 4) continue;
        ys.push_back(std::make_pair(p.v, j));
    }
    std::sort(ys.begin(), ys.end());
    PNT px; px.set(last, v[0], kBarCenter);
    short x0 = (short)(px.h + 10);
    short h = (short)(fontPt + 6), lastY = -1000;
    for (size_t k = 0; k < ys.size(); k++) {
        int j = ys[k].second;
        short y = ys[k].first; if (y - lastY < h + 1) y = (short)(lastY + h + 1); lastY = y;
        char b[16]; snprintf(b, sizeof(b), "%d%%", p60[CODES[j]]);
        int w = (int)getTextWidth(b, -1) + 8;
        RCT bg; bg.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        bg.draw(1, col[j], C_DARK, DRAW_OPAQUE, PAT_SOLID);
        setTextColor(col[j]);
        RCT rc; rc.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        rc.drawText(b, true, false);
    }
    return RTX_OK;
}

void SessionVWAP::writeStatus(const char* what, int nRth)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionVWAP.status.txt").c_str(), std::ios::trunc);
    if (!f.is_open()) return;
    f << "VERSION," << SV_VERSION << "\nMARKET," << mkt << "\nRTH_BARS," << nRth << "\nSTATE," << what << "\n";
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionVWAP *p = new SessionVWAP();
    p->setArrayCount(5);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("LRA Session VWAP: the RTH VWAP with 1 and 2 SD bands, and the chance each is touched in the next 60 min.");
    p->setVersion(SV_VERSION);
    return p;
}
