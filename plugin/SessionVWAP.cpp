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
 *  BADGES: at the right end of each line, the chance price touches it within the next 60 minutes (to the close when less is left) -
 *  just "42%". (1.1.0) Computed HERE from the chart's own bars, every new bar, with the touch-odds law compiled in (TouchParams.h:
 *  the volatility model, fat tails, time-of-day curve and the adopted adjustments per market - written by lra.level_touch, which
 *  the nightly refinement rewrites when a market's law improves; the gex auto-build then rebuilds this plugin). No file at run time.
 *  SessionVWAP-<MKT>.odds.txt records what the badges said (the nightly check compares it with the LRA's numbers).
 *  IRT runs ONE object per DLL for every chart: the market is read from the chart's own symbol on every calc / draw.
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include "TouchParams.h"          // (1.1.0) the touch-odds law per market, written by lra.level_touch.plugin_params (nightly)
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

static const char* SV_VERSION = "1.1.0";   // (1.1.0, Rassul 11:30 "why cant the badge come from within irt?") the badges are computed HERE, every tick, from the chart's bars (no file); the law comes compiled in (TouchParams.h, refreshed by the nightly refinement)
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
    // (1.1.0) native odds: the volatility forecast per market, cached per bar count (only the distance changes tick to tick)
    long featKey = -1; std::string featMkt; bool featOk = false; double sigMin = 0, V60 = 0, kEff = 1, nuEff = 0.35; int leftMin = 0;
    std::string featNote;
    bool forecast(const TPLaw& L, long n);
    const TPLaw* lawFor(const std::string& m) { for (int i = 0; i < TP_NLAWS; i++) if (mkt == TP_LAWS[i].m) return &TP_LAWS[i]; return nullptr; }
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

// ---------------------------------------------------------------- (1.1.0) the touch odds, natively
// The same arithmetic as lra.vol_study.live_inputs / forecast and lra.level_touch (p_touch, var_clock, the k adjustments):
//   inputs (per minute of price, from 3-min bars): rv15 / rv60 (closes, last 5 / 20 bars), rv_day (today's RTH closes), pk60 (Parkinson,
//   last 20 bars), atr3 (14 bars), atr30 / atr60 (clock 30 / 60-min bars, 8 / 6 complete ones), atr_d (10 prior RTH ranges / sqrt(RTH min))
//   sigma = exp(coef . [1, log inputs, weekday, log s_tod]) / s_tod;  V(60) = sigma^2 * sum (tod(m0+i) / tod(m0))^2
//   P(touch) = sum_k w_k erfc(d / (sqrt(2 V) k c_k)),  c_k = exp(nu z_k - nu^2 / 2)  (15-node Gauss-Hermite, the fat tails)
static const double GH_Z[15] = { -6.363947888829839, -5.190093591304781, -4.1962077112690155, -3.2890824243987664, -2.432436827009758,
    -1.6067100690287297, -0.799129068324548, 0.0, 0.799129068324548, 1.6067100690287297, 2.432436827009758, 3.2890824243987664,
    4.1962077112690155, 5.190093591304781, 6.363947888829839 };
static const double GH_W[15] = { 8.589649899633252e-10, 5.975419597920599e-07, 5.642146405189029e-05, 0.001567357503549956,
    0.017365774492137616, 0.08941779539984437, 0.23246229360973225, 0.31825951825951815, 0.23246229360973225, 0.08941779539984437,
    0.017365774492137616, 0.001567357503549956, 5.642146405189029e-05, 5.975419597920599e-07, 8.589649899633252e-10 };

static double pTouch(double d, double V, double nu, double k)
{
    if (!(V > 0)) return d <= 0 ? 1.0 : 0.0;
    double s = 0;
    for (int i = 0; i < 15; i++) {
        double sd = std::sqrt(V) * k * std::exp(nu * GH_Z[i] - 0.5 * nu * nu);
        s += GH_W[i] * std::erfc(std::fabs(d) / (std::sqrt(2.0) * sd));
    }
    return s;
}

// lra.session_info.RTH (the window the volatility study was fitted on) - for rv_day / atr_d only
static void featRth(const std::string& m, int& a, int& b)
{
    if (m == "ES" || m == "NQ") { a = 510; b = 900; } else if (m == "CL" || m == "NG") { a = 480; b = 810; }
    else if (m == "GC") { a = 440; b = 750; } else if (m == "HG") { a = 430; b = 720; } else { a = 440; b = 840; }
}

bool SessionVWAP::forecast(const TPLaw& L, long n)
{
    RTARRAY hi(barHigh), lo(barLow), cl(barClose); RTARRAYI dt(barDateTime);
    int per = 180;
    for (int i = (int)n - 1; i > 0 && i > (int)n - 40; i--) { long long d = (long long)dt[i] - (long long)dt[i - 1]; if (d > 0 && d < per) per = (int)d; }
    // the chart's bars: start minute of day, day key, weekday
    int from = (int)n - 4000; if (from < 0) from = 0;
    int N = (int)n - from;
    std::vector<int> smin(N), dkey(N), wday(N); std::vector<long long> tstart(N);
    std::vector<double> H(N), Lo(N), C(N);
    for (int j = 0; j < N; j++) {
        int i = from + j; struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        t.tm_isdst = -1; long long te = (long long)mktime(&t); long long ts = te - per;
        time_t tsv = (time_t)ts; struct tm s0; localtime_s(&s0, &tsv);
        smin[j] = s0.tm_hour * 60 + s0.tm_min; dkey[j] = (s0.tm_year + 1900) * 10000 + (s0.tm_mon + 1) * 100 + s0.tm_mday; wday[j] = s0.tm_wday;
        tstart[j] = ts; H[j] = hi[i]; Lo[j] = lo[i]; C[j] = cl[i];
    }
    if (N < 30) { featNote = "too few bars"; return false; }
    // now = the newest bar's close
    long long nowT = tstart[N - 1] + per;
    time_t nv = (time_t)nowT; struct tm nt; localtime_s(&nt, &nv);
    int m0 = nt.tm_hour * 60 + nt.tm_min, today = (nt.tm_year + 1900) * 10000 + (nt.tm_mon + 1) * 100 + nt.tm_mday;
    int openMin = 0, closeMin = 0; rthOf(mkt, openMin, closeMin);
    leftMin = closeMin - m0;
    if (m0 < openMin || leftMin <= 0) { featNote = "outside RTH"; return false; }
    double f[8]; bool have[8] = { false, false, false, false, false, false, false, false };
    std::vector<double> tr(N);
    for (int j = 0; j < N; j++) { double pc = j ? C[j - 1] : C[0]; tr[j] = std::max(H[j], pc) - std::min(Lo[j], pc); }
    auto rvLast = [&](int k) { double s = 0; int c = 0; for (int j = N - k; j < N; j++) if (j >= 1) { double d = C[j] - C[j - 1]; s += d * d; c++; } return c ? std::sqrt(s / c / 3.0) : 0.0; };
    f[0] = rvLast(5); have[0] = f[0] > 0;
    f[1] = rvLast(20); have[1] = f[1] > 0;
    { double s = 0; for (int j = N - 14; j < N; j++) s += tr[j]; f[4] = s / 14 / std::sqrt(3.0); have[4] = f[4] > 0; }
    { double s = 0; for (int j = N - 20; j < N; j++) { double r = std::log(H[j] / Lo[j]); s += r * r / (4 * std::log(2.0)); } f[3] = C[N - 1] * std::sqrt(s / 20 / 3.0); have[3] = f[3] > 0; }
    for (int pass = 0; pass < 2; pass++) {                               // atr30 / atr60 from complete clock bars
        int mins = pass ? 60 : 30, nb = pass ? 6 : 8;
        std::vector<double> gh, gl, gc; long long cur = -1;
        for (int j = 0; j < N; j++) {
            long long g = (tstart[j] / 60 / mins) * mins;                     // the clock bucket (local minutes since the epoch)
            if (g != cur) { if (cur >= 0 && (cur + mins) * 60 > nowT) break; gh.push_back(H[j]); gl.push_back(Lo[j]); gc.push_back(C[j]); cur = g; }
            else { gh.back() = std::max(gh.back(), H[j]); gl.back() = std::min(gl.back(), Lo[j]); gc.back() = C[j]; }
        }
        if (cur >= 0 && (cur + mins) * 60 > nowT && !gh.empty()) { gh.pop_back(); gl.pop_back(); gc.pop_back(); }   // the forming one
        int G = (int)gh.size();
        if (G >= 3) {
            double s = 0; int c = 0;
            for (int g = std::max(0, G - nb); g < G; g++) { double pc = g ? gc[g - 1] : gc[g]; s += std::max(gh[g], pc) - std::min(gl[g], pc); c++; }
            f[pass ? 6 : 5] = s / c / std::sqrt((double)mins); have[pass ? 6 : 5] = true;
        }
    }
    int fa = 0, fb = 0; featRth(mkt, fa, fb);
    {   // rv_day: today's RTH closes
        std::vector<double> cc; for (int j = 0; j < N; j++) if (dkey[j] == today && smin[j] >= fa && smin[j] < fb) cc.push_back(C[j]);
        if (cc.size() >= 5) { double s = 0; for (size_t k = 1; k < cc.size(); k++) { double d = cc[k] - cc[k - 1]; s += d * d; } f[2] = std::sqrt(s / (cc.size() - 1) / 3.0); have[2] = f[2] > 0; }
    }
    {   // atr_d: the last 10 prior RTH ranges / sqrt(RTH minutes)
        std::map<int, std::pair<double, double>> R;
        for (int j = 0; j < N; j++) if (dkey[j] < today && smin[j] >= fa && smin[j] < fb) {
            auto it = R.find(dkey[j]); if (it == R.end()) R[dkey[j]] = std::make_pair(H[j], Lo[j]);
            else { it->second.first = std::max(it->second.first, H[j]); it->second.second = std::min(it->second.second, Lo[j]); }
        }
        std::vector<double> r; for (auto& kv : R) r.push_back((kv.second.first - kv.second.second) / std::sqrt((double)std::max(1, fb - fa)));
        if (r.size() >= 5) { double s = 0; int c = 0; for (size_t k = r.size() > 10 ? r.size() - 10 : 0; k < r.size(); k++) { s += r[k]; c++; } f[7] = s / c; have[7] = f[7] > 0; }
    }
    // the time-of-day curve: 15-min buckets; a minute with no bucket counts as the current one (ratio 1)
    auto todAt = [&](int mm) -> double { if (mm < 0 || mm >= 1440) return -1; float x = L.tod15[mm / 15]; return x > 0 ? std::min(3.0, std::max(0.3, (double)x)) : -1; };
    double x0 = todAt(m0); if (x0 <= 0) x0 = 1.0;
    double sTod = 0; for (int q = 1; q <= 60; q++) { double x = todAt(m0 + q); double r = (x > 0 ? x : x0) / x0; sTod += r * r; } sTod = std::sqrt(sTod / 60.0);
    // the forecast
    bool ok = true; std::vector<double> v; v.push_back(1.0);
    for (int k = 0; k < L.nIn; k++) { int id = L.in[k]; if (!have[id] || !(f[id] > 0)) { ok = false; break; } v.push_back(std::log(f[id])); }
    int wdPy = (wday[N - 1] + 6) % 7;                                    // Python's weekday (Mon = 0)
    if (ok && L.dow) for (int j = 1; j <= 4; j++) v.push_back(wdPy == j ? 1.0 : 0.0);
    v.push_back(std::log(std::max(1e-6, sTod)));
    double fc = 0;
    if (ok && (int)v.size() == L.nCoef) { double z = 0; for (int k = 0; k < L.nCoef; k++) z += L.coef[k] * v[k]; fc = std::exp(z); featNote = "model"; }
    else { fc = f[1] * sTod; featNote = "fallback rv60"; }
    if (!(fc > 0)) { featNote = "no volatility"; return false; }
    sigMin = fc / std::max(1e-6, sTod);
    int W = std::min(60, leftMin);
    double vs = 0; for (int i = 1; i <= W; i++) { double x = todAt(m0 + i); double r = (x > 0 ? x : x0) / x0; vs += r * r; }
    V60 = sigMin * sigMin * vs;
    kEff = L.k;
    if (L.lateK != 1.0 && leftMin <= 75) kEff *= L.lateK;
    if (L.openK != 1.0 && m0 - openMin >= 0 && m0 - openMin < 75) kEff *= L.openK;
    if (L.usedB != 0.0 && have[7]) {
        double dh = -1e30, dl = 1e30; for (int j = 0; j < N; j++) if (dkey[j] == today && smin[j] >= openMin && smin[j] < closeMin) { dh = std::max(dh, H[j]); dl = std::min(dl, Lo[j]); }
        if (dh > dl) { double used = (dh - dl) / (f[7] * std::sqrt((double)std::max(1, closeMin - openMin))); kEff *= std::exp(L.usedB * std::log(std::max(0.05, used))); }
    }
    nuEff = L.nu;
    return true;
}

int SessionVWAP::draw(void)
{
    long n = getBarCount(); if (n < 2) return RTX_OK;
    { int f_ = getIntegerValue(SV.font); if (f_ >= 6 && f_ <= 18) fontPt = f_; }
    const TPLaw* L = lawFor(mkt);
    if (!L) return RTX_OK;
    RTARRAYI dtk(barDateTime);
    long key = n * 1000003L + (long)(dtk[(int)n - 1] % 1000003L);           // a new bar (or a new chart) = a new forecast
    if (key != featKey || featMkt != mkt) { featOk = forecast(*L, n); featKey = key; featMkt = mkt; }
    if (!featOk) return RTX_OK;                                         // outside RTH / not enough bars: lines only
    RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5);
    RTARRAY clN(barClose);
    int last = (int)n - 1;
    float v[5] = { o1[last], o2[last], o3[last], o4[last], o5[last] };
    if (!(v[0] > 0)) return RTX_OK;                                     // outside RTH: no lines, no badges
    const COLOR col[5] = { C_VWAP, C_SD1, C_SD1, C_SD2, C_SD2 };
    RCT pane; pane.getPaneRect(false);
    FONT f; f.id = HELVETICA; f.size = (short)fontPt; f.style = BOLD; setFont(f);
    int pv[5] = { -1, -1, -1, -1, -1 };
    std::vector<std::pair<short, int>> ys;                              // badge y, line index - placed top to bottom without overlap
    for (int j = 0; j < 5; j++) {
        if (!(v[j] > 0)) continue;
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
        double pr = pTouch((double)v[j] - (double)clN[last], V60, nuEff, kEff);
        pv[j] = (int)std::floor(100.0 * pr + 0.5);
        char b[16]; snprintf(b, sizeof(b), "%d%%", pv[j]);
        int w = (int)getTextWidth(b, -1) + 8;
        RCT bg; bg.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        bg.draw(1, col[j], C_DARK, DRAW_OPAQUE, PAT_SOLID);
        setTextColor(col[j]);
        RCT rc; rc.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        rc.drawText(b, true, false);
    }
    {   // the status file: what the badges said and why (compared with lra.level_touch's numbers in the nightly check)
        static time_t lastW = 0; time_t now_ = time(nullptr);
        if (now_ - lastW >= 30) {
            lastW = now_;
            const char* up = getenv("USERPROFILE");
            if (up) {
                std::ofstream f2((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionVWAP-" + mkt + ".odds.txt").c_str(), std::ios::trunc);
                f2 << "VERSION|" << SV_VERSION << "\nLAW|" << TP_SOURCE << "\nPX|" << clN[last] << "\nSIGMA_MIN|" << sigMin << "\nV60|" << V60
                   << "\nK|" << kEff << "\nNU|" << nuEff << "\nLEFT|" << leftMin << "\nMODEL|" << featNote << "\n";
                for (int j = 0; j < 5; j++) f2 << "LINE|" << CODES[j] << "|" << v[j] << "|" << pv[j] << "\n";
            }
        }
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
