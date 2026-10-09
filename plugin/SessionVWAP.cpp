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
 *
 *  1.2.0 (2026-10-08 22:16, Rassul: "can you automatically switch the vwap during different sessions?" -> mockup -> "build
 *  it, make sure it uses the right day and overnight for all the markets i trade and the calculations are correct"):
 *    overnight  17:00 -> the RTH open: the O/N VWAP + its +-1 / +-2 SD bands (own outputs) and a dim line at the prior RTH
 *               session's final VWAP. No badges (the touch odds are fitted on RTH only).
 *    RTH        the RTH VWAP + bands with the touch-odds badges exactly as before: just "42%" (Rassul 22:32)
 *    closed     after the RTH close the RTH lines stay frozen, no badges, until 17:00
 *  The sessions and the arithmetic live in SessionVWAPLogic.h (no SDK), tested against an independent pandas reference.
 ********************************************************************************/
#include "irtsdk.h"
// windows.h (included by some supported SDK build configurations) defines far.
// DealerLogic uses far as a data member, so remove only that obsolete macro here.
#ifdef far
#undef far
#endif
#include "DealerLogic.h"
#include "HostSlot.h"
#include "SessionVWAPLogic.h"    // (1.2.0) the session rules + VWAP arithmetic, tested without the SDK
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
#include <limits>
#include <new>

static const char* SV_VERSION = "1.3.1";        // 1.3.1: the overnight odds use the nightly champion (window / tail / scale from OnTouchParams.h); the "?" goes when the study calls the market ready.        // 1.3.0: overnight touch odds on the O/N VWAP + bands ("42%?" - native, tested nightly by lra.on_vwap_study; the "?" stays until proven).        // 1.2.0: automatic overnight / RTH / closed switch. 1.1.1-audit:   // badges are computed locally from chart bars; the law is compiled in from TouchParams.h
static const COLOR C_VWAP = 0x00FF80FF;   // 0x00RRGGBB: (1.2.0) Rassul's own settings 22:37 - VWAP magenta
static const COLOR C_SD1  = 0x00C0DCC0;   // +-1 SD pale green
static const COLOR C_SD2  = 0x00F0CAA6;   // +-2 SD peach
static const COLOR C_DARK = 0x000B0F19;
static const COLOR C_PRTH = 0x006B7280;   // the prior RTH VWAP overnight: dim grey
static const char* CODES[5] = { "VWAP", "VWAP+1", "VWAP-1", "VWAP+2", "VWAP-2" };

struct SVIdx { int font; };
static SVIdx SV;

// The SDK can route one extension object through more than one chart. Keep
// mutable cache and I/O throttling in the current host's UserData slot.
struct SVState {
    long featBars = -1;
    RTDATE featDate = 0;
    std::string featMkt;
    bool featOk = false;
    double sigMin = 0, V60 = 0, kEff = 1, nuEff = 0.35;
    int leftMin = 0;
    std::string featNote;
    time_t lastStatusWrite = 0;
    std::string statusWhat, statusMkt;
    int statusNRth = -1;
    time_t lastOddsWrite = 0;
    int per = 180;                      // (1.2.0) the bar size the last calc used
};

class SessionVWAP : public cppExtension {
public:
    SessionVWAP() : cppExtension() {}
    virtual int draw(void);
    void fill(int from);
    void writeStatus(const char* what, int nRth, const std::string& market, int nOn = 0);
    bool rthOf(const std::string& m, int& openMin, int& closeMin);
    SVState* state(bool create);
    HostSlot<SVState> slot_;
    void clearState();
    void drawLines(long n);
    // Volatility is cached per chart/bar; distance updates each draw.
    bool forecast(const TPLaw& L, long n, const std::string& market, SVState& S);
    const TPLaw* lawFor(const std::string& m) { for (int i = 0; i < TP_NLAWS; i++) if (m == TP_LAWS[i].m) return &TP_LAWS[i]; return nullptr; }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<SessionVWAP*>(this)->clearState(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<SessionVWAP*>(this)->clearState(); return RTX_OK; }
int cppExtension::calc(int iStartBar) { static_cast<SessionVWAP*>(this)->fill(iStartBar); return RTX_OK; }

int cppExtension::setup(void)
{
    // (1.2.0, Rassul 22:36 "see how i want the vwap setting to be by default and remove the user modifying them. so they are
    // set" + his settings 22:37) NO settings: the plugin draws its own lines in HIS colours, VWAP 2 px and bands 1 px (VWAP magenta,
    // +-1 SD pale green, +-2 SD peach; the prior RTH VWAP dim grey) and the badges at a fixed 9 pt. The 11 outputs stay as invisible data arrays (the values,
    // per bar, for anything that reads them). Parameter version 3 drops whatever older settings a chart had saved.
    setParameterVersion(3);
    setParameterDialogHeight(1);
    static const char* SV_OUTNAMES[11] = { "VWAP", "VWAP +1 SD", "VWAP -1 SD", "VWAP +2 SD", "VWAP -2 SD",
                                   "O/N VWAP", "O/N +1 SD", "O/N -1 SD", "O/N +2 SD", "O/N -2 SD", "pRTH VWAP" };
    CPEN p(C_VWAP, 1, P_SOLID);
    for (int a = 0; a < 11; a++) {
        setOutputParameter(SV_OUTNAMES[a], DRAW_INVISIBLE, &p, C_VWAP, OUTPUT_ENABLED);
        setArrayDrawingFlags(a, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    }
    SV.font = -1;
    return RTX_OK;
}

bool SessionVWAP::rthOf(const std::string& m, int& openMin, int& closeMin)
{
    return svl::rthOf(m.c_str(), openMin, closeMin);                    // (1.2.0) one table, in SessionVWAPLogic.h
}

SVState* SessionVWAP::state(bool create)
{
    return slot_.get(this, create);
}

void SessionVWAP::clearState()
{
    slot_.release(this);
}

void SessionVWAP::fill(int from)
{
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    const std::string market = dl::marketForRoot(rs ? rs : "");
    SVState* S = state(true);
    long n = getBarCount(); if (n < 2) { writeStatus("too few bars", 0, market); return; }
    RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5), o6(fOut6), o7(fOut7), o8(fOut8), o9(fOut9), o10(fOut10), o11(fOut11);
    RTARRAY* O[11] = { &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10, &o11 };
    RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    RTARRAYI vo(barVolume), dt(barDateTime);
    int openMin = 0, closeMin = 0;
    if (!rthOf(market, openMin, closeMin)) {
        for (int i = 0; i < (int)n; i++) for (int k = 0; k < 11; k++) (*O[k])[i] = 0;
        writeStatus("unknown market", 0, market); return;
    }
    // the bar size from the chart's own stamps (IRT stamps a bar at its CLOSE): the smallest positive gap of the last bars
    int per = 0;
    for (int i = (int)n - 1; i > 0 && i > (int)n - 40; i--) {
        long long d = (long long)dt[i] - (long long)dt[i - 1];
        if (d > 0 && (per == 0 || d < per)) per = (int)d;
    }
    if (per <= 0 || per > 3600) per = 180;
    if (S) S->per = per;
    auto barOf = [&](int i) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        svl::Bar b; b.days = svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        b.endSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
        b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; b.v = (double)vo[i];
        return b;
    };
    auto clsOf = [&](int i) { svl::Bar b = barOf(i); return svl::classify(b.days, b.endSec, per, openMin, closeMin); };
    // recompute from the START of the session holding `from` (every running sum restarts at its own session's first bar),
    // warmed up from the latest earlier session that had RTH bars (the overnight's prior RTH VWAP, pRTH) - up to 5 back
    int w = from < 0 ? 0 : (from >= (int)n ? (int)n - 1 : from);
    { long k = clsOf(w).sess; while (w > 0 && clsOf(w - 1).sess == k) w--; }
    int s = w;
    for (int back = 0; back < 5 && s > 0; back++) {
        long k = clsOf(s - 1).sess; bool rth = false;
        while (s > 0) { svl::Cls c = clsOf(s - 1); if (c.sess != k) break; if (c.kind == svl::RTH) rth = true; s--; }
        if (rth) break;
    }
    std::vector<svl::Bar> B; B.reserve((size_t)((int)n - s));
    for (int i = s; i < (int)n; i++) B.push_back(barOf(i));
    std::vector<float> out;
    svl::Result R = svl::run(B, per, openMin, closeMin, out);
    for (int i = w; i < (int)n; i++) for (int k = 0; k < 11; k++) (*O[k])[i] = out[(size_t)(i - s) * 11 + k];
    svl::Bar lb = B.back();
    int kind = svl::classify(lb.days, lb.endSec, per, openMin, closeMin).kind;
    const char* what = R.invalid ? "calc invalid OHLC" : kind == svl::RTH ? "calc RTH" : kind == svl::ON ? "calc O/N" : kind == svl::POST ? "calc closed (RTH frozen)" : "calc no session";
    writeStatus(what, R.nRth, market, R.nOn);
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
    if (!std::isfinite(d) || !std::isfinite(V) || !std::isfinite(nu) || !std::isfinite(k)) return std::numeric_limits<double>::quiet_NaN();
    // With no future variation, only a level already at price can be touched.
    // The previous signed comparison incorrectly returned 100% for every level
    // below price while the regular path correctly uses absolute distance.
    if (!(V > 0) || !(k > 0)) return std::fabs(d) <= std::numeric_limits<double>::epsilon() ? 1.0 : 0.0;
    double rootV = std::sqrt(V);
    if (!std::isfinite(rootV)) return std::numeric_limits<double>::quiet_NaN();
    double s = 0;
    for (int i = 0; i < 15; i++) {
        double sd = rootV * k * std::exp(nu * GH_Z[i] - 0.5 * nu * nu);
        if (!(sd > 0) || !std::isfinite(sd)) return std::numeric_limits<double>::quiet_NaN();
        s += GH_W[i] * std::erfc(std::fabs(d) / (std::sqrt(2.0) * sd));
    }
    return std::max(0.0, std::min(1.0, s));
}

// lra.session_info.RTH (the window the volatility study was fitted on) - for rv_day / atr_d only
static void featRth(const std::string& m, int& a, int& b)
{
    if (m == "ES" || m == "NQ") { a = 510; b = 900; } else if (m == "CL" || m == "NG") { a = 480; b = 810; }
    else if (m == "GC") { a = 440; b = 750; } else if (m == "HG") { a = 430; b = 720; } else { a = 440; b = 840; }
}

bool SessionVWAP::forecast(const TPLaw& L, long n, const std::string& market, SVState& S)
{
    RTARRAY hi(barHigh), lo(barLow), cl(barClose); RTARRAYI dt(barDateTime);
    S.sigMin = S.V60 = 0; S.kEff = 1; S.nuEff = 0.35; S.leftMin = 0; S.featNote.clear();
    int per = 0;
    for (int i = (int)n - 1; i > 0 && i > (int)n - 40; i--) { long long d = (long long)dt[i] - (long long)dt[i - 1]; if (d > 0 && (per == 0 || d < per)) per = (int)d; }
    if (per <= 0 || per > 3600) per = 180;
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
        if (!std::isfinite(H[j]) || !std::isfinite(Lo[j]) || !std::isfinite(C[j]) || !(H[j] >= Lo[j]) || !(H[j] > 0) || !(Lo[j] > 0) || !(C[j] > 0)) {
            S.featNote = "invalid OHLC"; return false;
        }
    }
    if (N < 30) { S.featNote = "too few bars"; return false; }
    // now = the newest bar's close
    long long nowT = tstart[N - 1] + per;
    time_t nv = (time_t)nowT; struct tm nt; localtime_s(&nt, &nv);
    int m0 = nt.tm_hour * 60 + nt.tm_min, today = (nt.tm_year + 1900) * 10000 + (nt.tm_mon + 1) * 100 + nt.tm_mday;
    int openMin = 0, closeMin = 0; rthOf(market, openMin, closeMin);
    S.leftMin = closeMin - m0;
    if (m0 < openMin || S.leftMin <= 0) { S.featNote = "outside RTH"; return false; }
    double f[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; bool have[8] = { false, false, false, false, false, false, false, false };
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
    int fa = 0, fb = 0; featRth(market, fa, fb);
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
    for (int k = 0; k < L.nIn; k++) { int id = L.in[k]; if (id < 0 || id >= 8 || !have[id] || !(f[id] > 0) || !std::isfinite(f[id])) { ok = false; break; } v.push_back(std::log(f[id])); }
    int wdPy = (wday[N - 1] + 6) % 7;                                    // Python's weekday (Mon = 0)
    if (ok && L.dow) for (int j = 1; j <= 4; j++) v.push_back(wdPy == j ? 1.0 : 0.0);
    v.push_back(std::log(std::max(1e-6, sTod)));
    double fc = 0;
    if (ok && (int)v.size() == L.nCoef) { double z = 0; for (int k = 0; k < L.nCoef; k++) z += L.coef[k] * v[k]; fc = std::exp(z); S.featNote = "model"; }
    else if (have[1] && std::isfinite(f[1])) { fc = f[1] * sTod; S.featNote = "fallback rv60"; }
    else { S.featNote = "insufficient volatility"; return false; }
    if (!(fc > 0) || !std::isfinite(fc)) { S.featNote = "no volatility"; return false; }
    S.sigMin = fc / std::max(1e-6, sTod);
    int W = std::min(60, S.leftMin);
    double vs = 0; for (int i = 1; i <= W; i++) { double x = todAt(m0 + i); double r = (x > 0 ? x : x0) / x0; vs += r * r; }
    S.V60 = S.sigMin * S.sigMin * vs;
    S.kEff = L.k;
    if (L.lateK != 1.0 && S.leftMin <= 75) S.kEff *= L.lateK;
    if (L.openK != 1.0 && m0 - openMin >= 0 && m0 - openMin < 75) S.kEff *= L.openK;
    if (L.usedB != 0.0 && have[7]) {
        double dh = -1e30, dl = 1e30; for (int j = 0; j < N; j++) if (dkey[j] == today && smin[j] >= openMin && smin[j] < closeMin) { dh = std::max(dh, H[j]); dl = std::min(dl, Lo[j]); }
        if (dh > dl) { double used = (dh - dl) / (f[7] * std::sqrt((double)std::max(1, closeMin - openMin))); S.kEff *= std::exp(L.usedB * std::log(std::max(0.05, used))); }
    }
    S.nuEff = L.nu;
    if (!(S.V60 > 0) || !(S.kEff > 0) || !std::isfinite(S.sigMin) || !std::isfinite(S.V60) || !std::isfinite(S.kEff) || !std::isfinite(S.nuEff)) {
        S.featNote = "invalid forecast"; return false;
    }
    return true;
}

// (1.2.0) the lines, drawn here in fixed colours: each series joins consecutive non-zero bars only, so the O/N, RTH and
// prior-RTH lines never join each other and nothing is drawn where a session has no value
void SessionVWAP::drawLines(long n)
{
    int a = 0, b = (int)n - 1;
    if (getVisibleBars(&a, &b) != RTX_OK || a < 0 || b >= (int)n || a > b) { a = std::max(0, (int)n - 400); b = (int)n - 1; }
    if (a > 0) a--;                                                       // join into the first visible bar
    RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5), o6(fOut6), o7(fOut7), o8(fOut8), o9(fOut9), o10(fOut10), o11(fOut11);
    RTARRAY* O[11] = { &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10, &o11 };
    static const COLOR COL[11] = { C_VWAP, C_SD1, C_SD1, C_SD2, C_SD2, C_VWAP, C_SD1, C_SD1, C_SD2, C_SD2, C_PRTH };
    static const short W[11] = { 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1 };   // Rassul 22:37: VWAP 2 px, bands 1 px
    for (int k = 10; k >= 0; k--) {                                       // the dim prior-RTH line underneath
        setPen(COL[k], W[k], P_SOLID);
        RTARRAY& v = *O[k];
        for (int i = a + 1; i <= b; i++) {
            float y0 = v[i - 1], y1 = v[i];
            if (!(y0 > 0) || !(y1 > 0) || !std::isfinite(y0) || !std::isfinite(y1)) continue;
            PNT p0; p0.set(i - 1, y0, kBarCenter); p0.setDrawPosition();
            PNT p1; p1.set(i, y1, kBarCenter); p1.drawLineTo();
        }
    }
}

int SessionVWAP::draw(void)
{
    long n = getBarCount(); if (n < 2) return RTX_OK;
    const int fontPt = 9;                                                 // fixed (no settings)
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    const std::string market = dl::marketForRoot(rs ? rs : "");
    int openMin = 0, closeMin = 0;
    if (!rthOf(market, openMin, closeMin)) return RTX_OK;
    drawLines(n);                                                         // the lines first, whatever the session
    SVState* S = state(true);
    if (!S) return RTX_OK;                                                // lines remain available if cache allocation fails
    RTARRAYI dtk(barDateTime);
    int last = (int)n - 1;
    // (1.2.0) which session the newest bar is in decides which lines get badges
    int kind = svl::NONE;
    {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dtk[last], &t);
        kind = svl::classify(svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday), t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec,
                             S->per > 0 ? S->per : 180, openMin, closeMin).kind;
    }
    if (kind == svl::NONE) return RTX_OK;
    RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5), o6(fOut6), o7(fOut7), o8(fOut8), o9(fOut9), o10(fOut10), o11(fOut11);
    RTARRAY clN(barClose);
    const bool on = kind == svl::ON;
    float v[6] = { 0, 0, 0, 0, 0, 0 };
    if (on) { v[0] = o6[last]; v[1] = o7[last]; v[2] = o8[last]; v[3] = o9[last]; v[4] = o10[last]; v[5] = o11[last]; }
    else    { v[0] = o1[last]; v[1] = o2[last]; v[2] = o3[last]; v[3] = o4[last]; v[4] = o5[last]; }
    if (!(v[0] > 0) && !(v[5] > 0)) return RTX_OK;                     // nothing drawn yet this session
    // the touch odds: RTH only (the law is fitted on RTH bars)
    int pv[5] = { -1, -1, -1, -1, -1 };
    bool odds = false;
    if (kind == svl::RTH && v[0] > 0) {
        const TPLaw* L = lawFor(market);
        RTDATE lastDate = (RTDATE)dtk[last];
        if (L && (S->featBars != n || S->featDate != lastDate || S->featMkt != market)) {
            S->featOk = forecast(*L, n, market, *S); S->featBars = n; S->featDate = lastDate; S->featMkt = market;
        }
        if (L && S->featOk) {
            for (int j = 0; j < 5; j++) {
                if (!(v[j] > 0)) continue;
                double pr = pTouch((double)v[j] - (double)clN[last], S->V60, S->nuEff, S->kEff);
                if (std::isfinite(pr)) pv[j] = (int)std::floor(100.0 * std::max(0.0, std::min(1.0, pr)) + 0.5);
            }
            odds = true;
        }
    }
    // (1.3.0) overnight: the odds from the chart's own last 20 closes (svl::onSigmaMin / onTouch), shown "42%?"
    bool onOdds = false;
    const otl::OT op = otl::paramsFor(market.c_str());                  // the nightly champion (OnTouchParams.h)
    if (on && v[0] > 0 && last >= op.win) {
        double clw[64]; int k = 0;
        for (int i = last - op.win; i <= last; i++) clw[k++] = (double)clN[i];
        double sg = otl::onSigmaMin(clw, k, S->per > 0 ? S->per : 180, op.win);
        if (sg > 0) {
            for (int j = 0; j < 5; j++) {
                if (!(v[j] > 0)) continue;
                double pr = otl::onTouch((double)v[j] - (double)clN[last], sg, 60, op.nu, op.k);
                if (pr >= 0) pv[j] = (int)std::floor(100.0 * pr + 0.5);
            }
            onOdds = true;
        }
    }
    const COLOR col[6] = { C_VWAP, C_SD1, C_SD1, C_SD2, C_SD2, C_PRTH };
    // (1.2.0, Rassul 22:32 "the vwap should not show labels like that.. i told you how i want the labels") the badge is
    // just the percentage, "42%" - RTH only, where the odds exist. Overnight and after the close: the lines, no badges.
    char txt[6][16];
    for (int j = 0; j < 6; j++) txt[j][0] = 0;
    if (odds) for (int j = 0; j < 5; j++) if (v[j] > 0 && pv[j] >= 0) snprintf(txt[j], sizeof(txt[j]), "%d%%", pv[j]);
    if (onOdds) for (int j = 0; j < 5; j++) if (v[j] > 0 && pv[j] >= 0) snprintf(txt[j], sizeof(txt[j]), op.ready ? "%d%%" : "%d%%?", pv[j]);
    RCT pane; pane.getPaneRect(false);
    FONT f; f.id = HELVETICA; f.size = (short)fontPt; f.style = BOLD; setFont(f);
    std::vector<std::pair<short, int>> ys;                              // badge y, line index - placed top to bottom without overlap
    for (int j = 0; j < 6; j++) {
        if (!(v[j] > 0) || !txt[j][0]) continue;
        PNT p; p.set(last, v[j], kBarCenter);
        if (p.v < pane.top + 4 || p.v > pane.bottom - 4) continue;
        ys.push_back(std::make_pair(p.v, j));
    }
    std::sort(ys.begin(), ys.end());
    PNT px; px.set(last, v[0] > 0 ? v[0] : v[5], kBarCenter);
    short x0 = (short)(px.h + 10);
    short h = (short)(fontPt + 6), lastY = -1000;
    for (size_t k = 0; k < ys.size(); k++) {
        int j = ys[k].second;
        short y = ys[k].first; if (y - lastY < h + 1) y = (short)(lastY + h + 1); lastY = y;
        int w = (int)getTextWidth(txt[j], -1) + 8;
        RCT bg; bg.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        bg.draw(1, col[j], C_DARK, DRAW_OPAQUE, PAT_SOLID);
        setTextColor(col[j]);
        RCT rc; rc.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        rc.drawText(txt[j], true, false);
    }
    if (odds) {   // the status file: what the badges said and why (compared with lra.level_touch's numbers in the nightly check)
        time_t now_ = time(nullptr);
        if (now_ - S->lastOddsWrite >= 30) {
            S->lastOddsWrite = now_;
            const char* up = getenv("USERPROFILE");
            if (up) {
                std::ofstream f2((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionVWAP-" + market + ".odds.txt").c_str(), std::ios::trunc);
                if (f2.is_open()) {
                    f2 << "VERSION|" << SV_VERSION << "\nLAW|" << TP_SOURCE << "\nPX|" << clN[last] << "\nSIGMA_MIN|" << S->sigMin << "\nV60|" << S->V60
                       << "\nK|" << S->kEff << "\nNU|" << S->nuEff << "\nLEFT|" << S->leftMin << "\nMODEL|" << S->featNote << "\n";
                    for (int j = 0; j < 5; j++) f2 << "LINE|" << CODES[j] << "|" << v[j] << "|" << pv[j] << "\n";
                }
            }
        }
    }
    return RTX_OK;
}

void SessionVWAP::writeStatus(const char* what, int nRth, const std::string& market, int nOn)
{
    SVState* S = state(true);
    const char* up = getenv("USERPROFILE"); if (!up) return;
    time_t now_ = time(nullptr);
    bool changed = !S || S->statusWhat != what || S->statusNRth != nRth || S->statusMkt != market;
    if (S && !changed && now_ - S->lastStatusWrite < 30) return;
    std::ofstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionVWAP.status.txt").c_str(), std::ios::trunc);
    if (!f.is_open()) return;
    f << "VERSION," << SV_VERSION << "\nMARKET," << market << "\nRTH_BARS," << nRth << "\nON_BARS," << nOn << "\nSTATE," << what << "\n";
    if (S) { S->lastStatusWrite = now_; S->statusWhat = what; S->statusNRth = nRth; S->statusMkt = market; }
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionVWAP *p = new SessionVWAP();
    p->setArrayCount(11);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("LRA Session VWAP: switches by itself - overnight the O/N VWAP + bands and the prior RTH VWAP; in RTH the RTH VWAP + bands with the chance each is touched in the next 60 min; frozen after the close.");
    p->setVersion(SV_VERSION);
    return p;
}
