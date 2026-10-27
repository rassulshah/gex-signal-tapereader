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
 *
 *  1.4.0 (VW100, 2026-10-10 - the VWAP odds study on 17 months, all 7 markets, RTH + overnight): the law's chance is passed through
 *  a CALIBRATION LAYER (VwapOddsLogic.h, coefficients compiled in from VwapOddsParams.h, written by lra.vwap_odds only when its
 *  nightly judge adopts a fit that beat the plain law on unseen months): per line, + vwapTrend of the same session (VwapTrendLogic.h,
 *  closed bars only: the line on the trend's side or against it, via the 2SD touch or the majority), + time of day. The "?" now
 *  follows ONE rule for RTH and overnight: it stays until the market / session is proven out of sample (VO_CAL proven flag) - the
 *  RTH badges used to show no "?" at all although no RTH law was validated.
 *
 *  1.5.0 (2026-10-13 21:23 CT, Rassul: "session vwap settings don't work. They should allow me to uncheck and remove bands as well
 *  as change colors"): REAL SETTINGS. Each line has a show box and a colour - VWAP, +1 SD, -1 SD, +2 SD, -2 SD, the prior RTH
 *  VWAP - plus the VWAP width, the band width and one box for the whole overnight set. Unticking a line removes it (RTH and
 *  overnight alike) and the odds badge that belongs to it. Defaults = exactly what 1.4.2 drew. The choices are kept per market in
 *  lsFlexLevels\SessionVWAP.settings-<MKT>.txt, written only by the settings window (OK / Apply / each change) and read back on
 *  every draw by its stamp, so a change shows without restarting IRT (SessionVWAPStyle.h). The stale badge text is gone from
 *  the description (the badges have not been drawn since 1.4.1).
 *
 *  1.6.0 (2026-10-18 night, Rassul approved the Market State): posts SRC "VWAP" (MarketState.h, MARKET-STATE-SPEC 1.0) - vwap,
 *  sd1_hi / sd1_lo, sd2_hi / sd2_lo, pos (above2 .. below2), slope (5 bars, 0.1 SD) - for the last CLOSED bar of its session (RTH,
 *  the frozen RTH lines after the close, or the overnight set), re-posted every 10 s so readers see it, cleared on removal. The VWAP
 *  trend (TREND) is posted by the HOD / LOD box, which shows it. Nothing drawn changes.
 ********************************************************************************/
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>     // BEFORE irtsdk.h (MSVC C2371 RTBOOL clash otherwise); NOMINMAX keeps std::max usable
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "SessionVWAP"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
// windows.h (included by some supported SDK build configurations) defines far.
// DealerLogic uses far as a data member, so remove only that obsolete macro here.
#ifdef far
#undef far
#endif
#include "DealerLogic.h"
#include "HostSlot.h"
#include "SessionVWAPLogic.h"    // (1.2.0) the session rules + VWAP arithmetic, tested without the SDK
#include "TouchParams.h"          // (1.1.0) the touch-odds law per market, written by lra.level_touch.plugin_params (nightly)
#include "VwapTrendLogic.h"       // (1.4.0) vwapTrend, natively from the chart's bars (closed bars only)
#include "VwapOddsLogic.h"        // (1.4.0) the calibration layer + the proven flag per market / session (VwapOddsParams.h, nightly)
#include "SessionVWAPStyle.h"     // (1.5.0) show / colour / width per line, kept per market (no SDK; tested)
#include "SessionVWAPSettings.h"  // (1.6.1) the settings window table (SettingsSpec.h)
#include "PivotVwapLogic.h"       // (1.8.0) the 3-bar pivot VWAP (SDK-free, tested: test_pivotvwap.cpp)
#include "SettingsHost.h"
static ss::Ids& svIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
#include "KeepOutDraw.h"          // (1.6.3) BoxDock.h 1.2.0 keep-out: nothing painted over the HOD/LOD table
#include "SelfAware.h"           // (1.7.0) self-awareness
#include "MarketState.h"          // (1.6.0) SRC VWAP, once per closed bar
#include <chrono>
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
#include <atomic>

static const bool SV_SHOW_BADGES = false;   // (1.4.1) Rassul 18:07: VWAP odds off the chart for now
static const char* SV_VERSION = "1.8.0";        // (1.8.0, 2026-10-27, Rassul 09:46 CT "modify the vwap to the 3bar pivot") the RTH VWAP LINE is drawn as the 3-BAR PIVOT VWAP (PivotVwapLogic.h 1.0.0): anchored at the last confirmed 3-bar-reversal pivot of the session, restarted at each new pivot, green while the bar closed above it, red below; never repainted; setting "3-bar pivot VWAP" (appended row, default on; off = the old magenta RTH VWAP). The 11 outputs, the bands, the odds, vwapTrend and the Market State are unchanged (still the session VWAP). //        // (1.7.0, 2026-10-26) self-awareness SELFVWAP (SelfAware.h, SELF-AWARE.md; ships OFF per market) // 1.6.3 (2026-10-21 16:05 CT Rassul "fix this, it should be solid"): the VWAP / band lines stop at the lsHodLod table's edges (KeepOutDraw.h; values, odds and Market State unchanged).        // 1.6.1 (2026-10-18 settings audit): the window from SessionVWAPSettings.h / SettingsHost.h (same rows, same file); each line's show box greys its colour.        // 1.6.0 (2026-10-18): Market State SRC VWAP (vwap, bands, pos, slope) for the last closed bar, cleared on removal.        // 1.5.1 (2026-10-17): FIX unchecking / re-checking a line in the settings did not stick - only the window saves, nothing while it loads, untouched controls keep the file.        // 1.5.0 (2026-10-14; built as 1.4.3 on 10-13, never shipped): real settings - every line has a show box and a colour, VWAP / band widths, the overnight set on / off; kept per market (SessionVWAPStyle.h). 1.4.2: vwapTrend cache also keys the detected bar period, so a chart periodicity change cannot retain trend state computed with different session boundaries. 1.4.1: the odds badges are NOT drawn (Rassul 2026-10-11 18:07 "the chart is very messy ... get rid of the probabilities for vwap and its bands"); the odds are still computed and logged (SessionVWAP-<MKT>.odds.txt) for the nightly check.        // 1.4.0: calibrated odds (VwapOddsParams.h: per line + vwapTrend + time of day, adopted only out of sample) and one "?" rule for RTH and overnight (proven per market / session).        // 1.3.4: no options at all in the settings window (outputs OUTPUT_NO_UI); -1 SD light red instead of yellow.        // 1.3.3: his colours (VWAP magenta, +1 SD green, -1 SD yellow, +2 SD lime, -2 SD red), VWAP 2 px, bands 1 px, no options in the settings window.        // 1.3.2: no badges in the first 30 min of the RTH (and of the 17:00 overnight) session - too clustered while the bands are tight.        // 1.3.1: the overnight odds use the nightly champion (window / tail / scale from OnTouchParams.h); the "?" goes when the study calls the market ready.        // 1.3.0: overnight touch odds on the O/N VWAP + bands ("42%?" - native, tested nightly by lra.on_vwap_study; the "?" stays until proven).        // 1.2.0: automatic overnight / RTH / closed switch. 1.1.1-audit:   // badges are computed locally from chart bars; the law is compiled in from TouchParams.h
// (1.5.0) the default colours (0x00RRGGBB) live in SessionVWAPStyle.h (svs::D_*) - the settings window starts from them
static const COLOR C_VWAP = (COLOR)svs::D_VWAP;   // (1.2.0) Rassul's own settings 22:37 - VWAP magenta (the outputs' nominal pen)
static const COLOR C_DARK = 0x000B0F19;
static const char* CODES[5] = { "VWAP", "VWAP+1", "VWAP-1", "VWAP+2", "VWAP-2" };

// Status and odds are consumed asynchronously.  Keep different contracts and
// periods separate, and publish a complete replacement rather than exposing a
// partially written truncation to a reader.
static std::atomic<unsigned long> SV_FILE_SEQ(0);

static std::string svFilePart(const char* s, const char* fallback)
{
    std::string r;
    if (s) for (const char* p = s; *p && r.size() < 48; ++p) {
        const char c = *p;
        const bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                          (c >= '0' && c <= '9') || c == '_' || c == '-';
        r += safe ? c : '_';
    }
    return r.empty() ? std::string(fallback) : r;
}

static std::string svRecordPath(cppExtension* x, const char* profile, const std::string& market, int per, const char* type)
{
    const char* symbol = x ? x->getSymbol() : nullptr;
    const int secs = per > 0 && per <= 3600 ? per : 180;
    return std::string(profile) + "\\InvestorRT\\rtx\\lsFlexLevels\\SessionVWAP-" + svFilePart(market.c_str(), "unknown") +
           "-" + svFilePart(symbol, "unknown") + "-" + std::to_string(secs) + "s." + type;
}

static bool svPublishText(const std::string& path, const std::string& text)
{
    const std::string tmp = path + ".tmp." + std::to_string((unsigned long long)time(nullptr)) + "." +
                            std::to_string(SV_FILE_SEQ.fetch_add(1, std::memory_order_relaxed));
    {
        std::ofstream f(tmp.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!f.is_open()) return false;
        f.write(text.data(), (std::streamsize)text.size());
        f.flush();
        if (!f.good()) { f.close(); std::remove(tmp.c_str()); return false; }
    }
#if defined(_WIN32)
    if (!::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::remove(tmp.c_str());
        return false;
    }
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::remove(tmp.c_str());
        return false;
    }
#endif
    return true;
}

// The SDK can route one extension object through more than one chart. Keep
// mutable cache and I/O throttling in the current host's UserData slot.
struct SVState {
    bdk::TableWatch keep;   // (1.6.3) the HOD/LOD table on this chart (keep-out)
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
    // (1.4.0) vwapTrend cache: recomputed when the last closed bar, stamp, market, session kind, or chart bar period changes
    int trIdx = -1;
    RTDATE trDate = 0;
    std::string trMkt;
    int trKind = 0;
    int trPer = 0;
    vtl::Trend tr;
    // (1.5.0) this chart's line style, from SessionVWAP.settings-<MKT>.txt (re-read when its stamp changes, checked once a second)
    svs::Style style;
    std::string styleMkt;
    long long styleStamp = -2, styleCheckMs = -1;
    int styleBad = 0;
    // (1.6.0) the Market State entry of this chart: identity, the facts decided for the last closed bar, when last posted
    bool mstPosted = false; std::string mstSym, mstChart; long mstSpb = 0; long long mstBar = -1, mstMs = -1; sa::Hook self;
    std::map<std::string, std::string> mstKv;
};

class SessionVWAP : public cppExtension {
public:
    SS_SAFE_DIALOG(SessionVWAP, "SessionVWAP")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    SessionVWAP() : cppExtension() { for (int i = 0; i < svs::R_COUNT; i++) P[i] = -1; }   // (1.5.0) rows unknown until setup()
    virtual int draw(void);
    virtual int parmsLoadBody(void);                // (1.5.0) fills the window from the market's saved choices; never saves (lesson 29)
    virtual int parmsApplyBody(void);               // (1.5.0) OK / Apply: saves the window's choices for this chart's market
    virtual int parmsUpdtBody(unsigned int);        // (1.5.0) each change in the window: saved at once, so the chart follows it live
    int P[svs::R_COUNT];                        // (1.5.0) the parameter index of each settings row (set in setup)
    std::string marketNow();
    void saveStyle();
    bool loading_ = false;                      // (1.5.1) inside parmsLoad: the window is being filled - nothing is saved
    ss::Snap snap_;                             // (1.5.1) the window as parmsLoad left it: only the controls he changed are saved (1.6.1: SettingsSpec.h)
    const svs::Style& styleFor(SVState& S, const std::string& market, bool force = false);
    void fill(int from);
    void writeStatus(const char* what, int nRth, const std::string& market, int nOn = 0);
    bool rthOf(const std::string& m, int& openMin, int& closeMin);
    SVState* state(bool create);
    HostSlot<SVState> slot_;
    void clearState();
    void drawLines(long n, const svs::Style& st, int per = 0, int openMin = -1, int closeMin = -1);
    void postMst(long n, SVState& S, const std::string& market, int openMin, int closeMin);   // (1.6.0) SRC VWAP
    // Volatility is cached per chart/bar; distance updates each draw.
    bool forecast(const TPLaw& L, long n, const std::string& market, SVState& S);
    // (1.4.0) the badge numbers of bar `last` (kind = its session), vwapTrend from bars up to `lastClosed`; returns 2 RTH odds,
    // 1 overnight odds, 0 none. Shared by draw() and the tests (the oracle replays history with lastClosed = last).
    int badgeOdds(int last, int lastClosed, const std::string& market, int kind, int endMin, const float* v, double px,
                  int pv[5], bool& proven, SVState& S);
    vtl::Trend trendFor(int lastClosed, const std::string& market, int kind, SVState& S);
    svl::Bar barAt(int i);
    const TPLaw* lawFor(const std::string& m) { for (int i = 0; i < TP_NLAWS; i++) if (m == TP_LAWS[i].m) return &TP_LAWS[i]; return nullptr; }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<SessionVWAP*>(this)->clearState(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<SessionVWAP*>(this)->clearState(); return RTX_OK; }
int cppExtension::calc(int iStartBar) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<SessionVWAP*>(this)->fill(iStartBar); return RTX_OK; }

int cppExtension::setup(void)
{
    // (1.5.0, Rassul 2026-10-13 21:23 "They should allow me to uncheck and remove bands as well as change colors") the settings
    // window: one row per line - show box + colour (+ width for the VWAP), the band width, the overnight set, the prior RTH VWAP.
    // Controls only (no how-to text). Defaults = 1.4.2's fixed look. Parameter version 6: the old (empty) saved set is replaced.
    // IRT keeps saved values BY POSITION: rows may only be appended below the last one (svs::Row) from here on.
    SessionVWAP* me = static_cast<SessionVWAP*>(this);
    // (1.6.1, settings audit 2026-10-18) the same 15 rows in the same order from svset::spec() (parameter version 6 kept); P[] keeps
    // each row's id for the host tests (svs::Row order = the table order)
    ss::registerAll(this, svset::spec(), svIds());
    (void)svs::WIDTH_CHOICES;   // the table uses the same "1 px;2 px;3 px;4 px" text (svset::WIDTHS)
    for (int r = 0; r < svs::R_COUNT && r < (int)svIds().id.size(); ++r) me->P[r] = svIds().id[(size_t)r];
    // (1.3.3 / 1.3.4) the 11 value arrays stay OUTPUT_NO_UI: they exist for anything that reads them but never appear in the
    // settings window; the plugin draws the lines itself, in the style above.
    static const char* NAMES[11] = { "VWAP", "VWAP +1 SD", "VWAP -1 SD", "VWAP +2 SD", "VWAP -2 SD",
                                     "O/N VWAP", "O/N +1 SD", "O/N -1 SD", "O/N +2 SD", "O/N -2 SD", "pRTH VWAP" };
    for (int a = 0; a < 11; a++) {
        CPEN p(C_VWAP, 1, P_SOLID);
        setOutputParameter(NAMES[a], DRAW_INVISIBLE, &p, C_VWAP, OUTPUT_NO_UI);
        setArrayDrawingFlags(a, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    }
    return RTX_OK;
}

// ---------------------------------------------------------------- (1.5.0) the settings window <-> the per-market file
std::string SessionVWAP::marketNow()
{
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    return dl::marketForRoot(rs ? rs : "");
}

// (1.6.1, settings audit 2026-10-18) the window from svset::spec() through SettingsHost.h; the per-market file keeps its format.
// Kept from 1.5.1: only the settings WINDOW's copy saves, nothing while it loads, an untouched control keeps the file's value
// (ss::Snap), a near-black swatch keeps the saved colour (black lines would vanish on his black chart).
static ss::Values svValues(const svs::Style& s)
{
    using namespace svset;
    ss::Values V = ss::defaults(spec(), -1);
    const int show[svs::K_N] = { K_VWAP, K_P1, K_M1, K_P2, K_M2 }, col[svs::K_N] = { K_VWAP_COLOR, K_P1_COLOR, K_M1_COLOR, K_P2_COLOR, K_M2_COLOR };
    for (int k = 0; k < svs::K_N; ++k) { V.v[(size_t)show[k]] = s.show[k] ? 1 : 0; V.v[(size_t)col[k]] = (double)(s.col[k] & 0xFFFFFFUL); }
    V.v[K_VWAP_WIDTH] = s.vwapW - svs::W_MIN; V.v[K_BAND_WIDTH] = s.bandW - svs::W_MIN;
    V.v[K_OVERNIGHT] = s.on ? 1 : 0; V.v[K_PRIOR] = s.prior ? 1 : 0; V.v[K_PRIOR_COLOR] = (double)(s.priorCol & 0xFFFFFFUL);
    V.v[K_PIVOT] = s.pivot ? 1 : 0;   // (1.8.0)
    return V;
}
static svs::Style svStyle(const ss::Values& V, const svs::Style& keep)
{
    using namespace svset;
    svs::Style s = keep;
    const int show[svs::K_N] = { K_VWAP, K_P1, K_M1, K_P2, K_M2 }, col[svs::K_N] = { K_VWAP_COLOR, K_P1_COLOR, K_M1_COLOR, K_P2_COLOR, K_M2_COLOR };
    for (int k = 0; k < svs::K_N; ++k) { s.show[k] = V.b(show[k]); const svs::Colour c = (svs::Colour)V.c(col[k]); if (!svs::nearBlack(c)) s.col[k] = c; }
    s.vwapW = svs::W_MIN + V.i(K_VWAP_WIDTH); s.bandW = svs::W_MIN + V.i(K_BAND_WIDTH);
    s.on = V.b(K_OVERNIGHT); s.prior = V.b(K_PRIOR); s.pivot = V.b(K_PIVOT);   // (1.8.0)
    { const svs::Colour c = (svs::Colour)V.c(K_PRIOR_COLOR); if (!svs::nearBlack(c)) s.priorCol = c; }
    return s;
}
static bool svInDialog(cppExtension* x) { return ss::inDialog(x); }
void SessionVWAP::saveStyle()
{
    if (loading_ || !svInDialog(this)) return;
    const std::string market = marketNow();
    const std::string path = svs::pathFor(getenv("USERPROFILE"), market);
    if (path.empty() || !svIds().ok) return;
    svs::Style keep; std::string text; long long st = -1;
    if (svs::readFile(path, text, st) == 0) svs::fromText(text, keep);
    const ss::Values V = ss::readDialog(this, svset::spec(), svIds(), svValues(keep), snap_);   // untouched -> the file's value; invalid never saved
    const svs::Style next = svStyle(V, keep);
    const std::string out = svs::toText(next);
    snap_.take(ss::dialogNow(this, svset::spec(), svIds(), V));          // this change is now the baseline
    if (st >= 0 && out == text) return;
    if (!svPublishText(path, out)) return;
    SVState* S = state(false);
    if (S) { S->styleCheckMs = -1; S->styleStamp = -2; }                 // the next draw re-reads it at once
}

int SessionVWAP::parmsApplyBody(void) { saveStyle(); return RTX_OK; }
int SessionVWAP::parmsUpdtBody(unsigned int i) { if (!loading_) ss::onUpdate(this, svset::spec(), svIds(), (int)i, snap_); saveStyle(); return RTX_OK; }

// the window opens on what this market's chart is drawing (another chart of the market may have changed it); never saves
int SessionVWAP::parmsLoadBody(void)
{
    struct Unload { bool& f; ~Unload() { f = false; } } unload = { loading_ };
    loading_ = true;
    if (!svInDialog(this) || !svIds().ok) return RTX_OK;
    const std::string path = svs::pathFor(getenv("USERPROFILE"), marketNow());
    std::string text; long long st = -1; svs::Style s;                    // no file / unreadable = the defaults (svs::Style())
    if (!path.empty() && svs::readFile(path, text, st) == 0) { svs::Style r; if (svs::fromText(text, r)) s = r; }
    ss::fillDialog(this, svset::spec(), svIds(), svValues(s), snap_);
    return RTX_OK;
}

// this chart's style: the market's file, re-read when its stamp changes (checked at most once a second); no file = defaults
const svs::Style& SessionVWAP::styleFor(SVState& S, const std::string& market, bool force)
{
    const long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (!force && market == S.styleMkt && S.styleCheckMs >= 0 && now >= S.styleCheckMs && now - S.styleCheckMs < 1000) return S.style;
    S.styleCheckMs = now;
    const std::string path = svs::pathFor(getenv("USERPROFILE"), market);
    const long long st = svs::fileStamp(path);
    if (market == S.styleMkt && st == S.styleStamp) return S.style;
    std::string text; long long st2 = -1;
    const int rd = path.empty() ? 1 : svs::readFile(path, text, st2);
    if (rd == 2) return S.style;                                          // torn / oversized: keep the last good style, retry
    svs::Style s; int bad = 0;
    if (rd == 0) svs::fromText(text, s, &bad);
    S.style = s; S.styleBad = bad; S.styleMkt = market; S.styleStamp = rd == 0 ? st2 : st;
    return S.style;
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
    SVState* S = state(false);   // (1.6.0) this chart's Market State entry leaves the board (removal, symbol / period change)
    if (S && S->mstPosted) { mst::clear("VWAP", S->mstSym, S->mstSpb, S->mstChart); S->self.clear(S->mstSym, S->mstSpb, S->mstChart); S->mstPosted = false; }
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

// (1.2.0) the lines: each series joins consecutive non-zero bars only, so the O/N, RTH and prior-RTH lines never join each other
// and nothing is drawn where a session has no value. (1.5.0) each series in its own style: an unticked line is not drawn at all.
void SessionVWAP::drawLines(long n, const svs::Style& st, int per, int openMin, int closeMin)
{
    int a = 0, b = (int)n - 1;
    if (getVisibleBars(&a, &b) != RTX_OK || a < 0 || b >= (int)n || a > b) { a = std::max(0, (int)n - 400); b = (int)n - 1; }
    if (a > 0) a--;                                                       // join into the first visible bar
    RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5), o6(fOut6), o7(fOut7), o8(fOut8), o9(fOut9), o10(fOut10), o11(fOut11);
    RTARRAY* O[11] = { &o1, &o2, &o3, &o4, &o5, &o6, &o7, &o8, &o9, &o10, &o11 };
    for (int k = 10; k >= 0; k--) {                                       // the dim prior-RTH line underneath
        const svs::Pen pen = svs::penFor(st, k);
        if (!pen.show) continue;                                          // (1.5.0) unticked: no line
        if (k == 0 && st.pivot && openMin >= 0) continue;                 // (1.8.0) the RTH VWAP line is the 3-bar pivot VWAP below
        setPen((COLOR)pen.col, (short)pen.width, P_SOLID);
        RTARRAY& v = *O[k];
        for (int i = a + 1; i <= b; i++) {
            float y0 = v[i - 1], y1 = v[i];
            if (!(y0 > 0) || !(y1 > 0) || !std::isfinite(y0) || !std::isfinite(y1)) continue;
            PNT p0; p0.set(i - 1, y0, kBarCenter);
            PNT p1; p1.set(i, y1, kBarCenter);
            ko::linePts(p0, p1);                                          // (1.6.3) cut at the HOD/LOD table
        }
    }
    // (1.8.0) THE 3-BAR PIVOT VWAP, RTH only, natively from the chart's bars: each RTH session that reaches the visible bars is computed
    // from its own first RTH bar (PivotVwapLogic.h); a segment joins two bars of the same session and the same anchor; green / red by
    // that bar's close vs the line. Shown with the "VWAP" box ticked and "3-bar pivot VWAP" on; width = the VWAP width.
    if (!st.pivot || openMin < 0 || !st.show[svs::K_VWAP]) return;
    RTARRAY hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume), dt(barDateTime);   // as fill() reads them
    const int pp = per > 0 ? per : 180;
    auto clsOf = [&](int i) { struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        return svl::classify(svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday), t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec, pp, openMin, closeMin); };
    int i = a;
    { const svl::Cls c0 = clsOf(i); if (c0.kind == svl::RTH) while (i > 0) { const svl::Cls q = clsOf(i - 1); if (q.kind != svl::RTH || q.sess != c0.sess) break; --i; } }
    const short w = (short)std::max(1, st.vwapW);
    while (i <= b) {
        const svl::Cls c0 = clsOf(i);
        if (c0.kind != svl::RTH) { ++i; continue; }
        int j = i; std::vector<double> H, L, C, V;
        while (j <= b) { const svl::Cls q = clsOf(j); if (q.kind != svl::RTH || q.sess != c0.sess) break;
            H.push_back(hi[j]); L.push_back(lo[j]); C.push_back(cl[j]); V.push_back(vo[j]); ++j; }
        const pvw::Out o = pvw::compute(H, L, C, V);
        for (int k = 1; k < (int)o.line.size(); ++k) {
            if (!std::isfinite(o.line[k - 1]) || !std::isfinite(o.line[k]) || o.anchor[k] != o.anchor[k - 1] || o.state[k] == 0) continue;
            setPen(o.state[k] > 0 ? (COLOR)0x0022C55EUL : (COLOR)0x00EF4444UL, w, P_SOLID);
            PNT p0; p0.set(i + k - 1, (float)o.line[k - 1], kBarCenter); PNT p1; p1.set(i + k, (float)o.line[k], kBarCenter);
            ko::linePts(p0, p1);
        }
        i = j;
    }
}

int SessionVWAP::draw(void)
{ LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    long n = getBarCount(); if (n < 2) return RTX_OK;
    const int fontPt = 9;                                                 // fixed (no settings)
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    const std::string market = dl::marketForRoot(rs ? rs : "");
    int openMin = 0, closeMin = 0;
    if (!rthOf(market, openMin, closeMin)) return RTX_OK;
    SVState* S = state(true);
    if (!S) { drawLines(n, svs::Style()); return RTX_OK; }               // lines remain available (default style) if allocation fails
    const svs::Style& st = styleFor(*S, market);                         // (1.5.0) this market's show / colour / width choices
    S->keep.refresh(getSymbol(), getSecondsPerBar(), getChartLabel());   // (1.6.3) the HOD/LOD table: nothing painted over it this draw
    bdk::KeepScope keepScope(S->keep);
    drawLines(n, st, S->per > 0 ? S->per : 180, openMin, closeMin);       // the lines first, whatever the session; (1.8.0) + the pivot VWAP
    postMst(n, *S, market, openMin, closeMin);                            // (1.6.0) the Market State (behind the scenes)
    RTARRAYI dtk(barDateTime);
    int last = (int)n - 1;
    // (1.2.0) which session the newest bar is in decides which lines get badges
    int kind = svl::NONE;
    int minOfDay = -1;
    {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dtk[last], &t);
        minOfDay = t.tm_hour * 60 + t.tm_min;
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
    // (1.3.2, Rassul 2026-10-09 08:38 "dont show the probabilities on the vwap until after the first 30 min. it looks too clustered")
    // no badges in the first 30 minutes of the session that is drawing: the RTH open, or the 17:00 overnight open (bar END stamps)
    bool early = false;
    if (minOfDay >= 0) {
        int since = on ? (minOfDay >= 17 * 60 ? minOfDay - 17 * 60 : -1) : minOfDay - openMin;
        early = since >= 0 && since <= 30;
    }
    // (1.4.0) the odds: the law (RTH: TouchParams.h; overnight: OnTouchParams.h) through the calibration layer, with vwapTrend
    // from the CLOSED bars (the newest bar is still forming); "?" until the market / session is proven out of sample
    int pv[5] = { -1, -1, -1, -1, -1 };
    bool proven = false;
    int shown = early ? 0 : badgeOdds(last, last - 1, market, kind, minOfDay, v, (double)clN[last], pv, proven, *S);
    const bool odds = shown == 2;
    COLOR col[6]; bool vis[6];                                            // (1.5.0) a badge follows its line: colour, and gone when unticked
    for (int j = 0; j < 6; j++) { const svs::Pen pen = svs::penFor(st, j == 5 ? 10 : (on ? 5 + j : j)); col[j] = (COLOR)pen.col; vis[j] = pen.show; }
    // (1.2.0, Rassul 22:32) the badge is just the percentage, "42%" - with "?" while not proven (1.4.0: RTH and overnight alike)
    char txt[6][16];
    for (int j = 0; j < 6; j++) txt[j][0] = 0;
    if (shown) for (int j = 0; j < 5; j++) if (v[j] > 0 && pv[j] >= 0) snprintf(txt[j], sizeof(txt[j]), proven ? "%d%%" : "%d%%?", pv[j]);
    RCT pane; pane.getPaneRect(false);
    FONT f; f.id = HELVETICA; f.size = (short)fontPt; f.style = BOLD; setFont(f);
    std::vector<std::pair<short, int>> ys;                              // badge y, line index - placed top to bottom without overlap
    for (int j = 0; j < 6; j++) {
        if (!(v[j] > 0) || !txt[j][0] || !vis[j]) continue;
        PNT p; p.set(last, v[j], kBarCenter);
        if (p.v < pane.top + 4 || p.v > pane.bottom - 4) continue;
        ys.push_back(std::make_pair(p.v, j));
    }
    std::sort(ys.begin(), ys.end());
    PNT px; px.set(last, v[0] > 0 ? v[0] : v[5], kBarCenter);
    short x0 = (short)(px.h + 10);
    short h = (short)(fontPt + 6), lastY = -1000;
    for (size_t k = 0; k < ys.size() && SV_SHOW_BADGES; k++) {   // (1.4.1) not drawn; the odds below are still logged
        int j = ys[k].second;
        short y = ys[k].first; if (y - lastY < h + 1) y = (short)(lastY + h + 1); lastY = y;
        int w = (int)getTextWidth(txt[j], -1) + 8;
        RCT bg; bg.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        if (ko::off(bg)) continue;                                        // (1.6.3) not under the HOD/LOD table
        bg.draw(1, col[j], C_DARK, DRAW_OPAQUE, PAT_SOLID);
        setTextColor(col[j]);
        RCT rc; rc.set(x0, (short)(y - h / 2), (short)(x0 + w), (short)(y + h / 2));
        rc.drawText(txt[j], true, false);
    }
    if (odds) {   // the odds record: what the badges said and why (compared with lra.level_touch's numbers in the nightly check)
        time_t now_ = time(nullptr);
        if (now_ - S->lastOddsWrite >= 30) {
            const char* up = getenv("USERPROFILE");
            if (up) {
                std::ostringstream text;
                text << "VERSION|" << SV_VERSION << "\nLAW|" << TP_SOURCE << "\nCAL|" << VO_SOURCE << "|" << (proven ? "proven" : "?")
                     << "\nTREND|" << vtl::label(S->tr) << "|" << vtl::viaLabel(S->tr) << "\nPX|" << clN[last] << "\nSIGMA_MIN|" << S->sigMin << "\nV60|" << S->V60
                     << "\nK|" << S->kEff << "\nNU|" << S->nuEff << "\nLEFT|" << S->leftMin << "\nMODEL|" << S->featNote << "\n";
                for (int j = 0; j < 5; j++) text << "LINE|" << CODES[j] << "|" << v[j] << "|" << pv[j] << "\n";
                if (svPublishText(svRecordPath(this, up, market, S->per, "odds.txt"), text.str())) S->lastOddsWrite = now_;
            }
        }
    }
    return RTX_OK;
}

// (1.6.0) MARKET STATE, SRC "VWAP": the outputs IRT already holds, at the last CLOSED bar (n - 2) of its session; decided once for that bar
// (the outputs of a closed bar never change), re-posted every 10 s (MarketState.h drops entries older than 60 s), cleared in clearState()
void SessionVWAP::postMst(long n, SVState& S, const std::string& market, int openMin, int closeMin)
{
    if (n < 2) return;
    const char* sy = getSymbol(); const char* cl = getChartLabel();
    const std::string sym = sy ? sy : "", chart = cl ? cl : ""; const long spb = getSecondsPerBar();
    if (sym.empty()) return;
    if (S.mstPosted && (sym != S.mstSym || spb != S.mstSpb || chart != S.mstChart)) { mst::clear("VWAP", S.mstSym, S.mstSpb, S.mstChart); S.self.clear(S.mstSym, S.mstSpb, S.mstChart); S.mstPosted = false; }
    const int lc = (int)n - 2;
    const svl::Bar b = barAt(lc);
    const long long bar = (long long)b.days * 86400LL + b.endSec;           // CT wall-clock seconds of the bar's END stamp (the SignalBoard convention)
    const long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    const bool newBar = bar != S.mstBar;
    if (newBar) {
        const int per = S.per > 0 ? S.per : 180;
        const svl::Cls c = svl::classify(b.days, b.endSec, per, openMin, closeMin);
        RTARRAY o1(fOut1), o2(fOut2), o3(fOut3), o4(fOut4), o5(fOut5), o6(fOut6), o7(fOut7), o8(fOut8), o9(fOut9), o10(fOut10);
        float v[5] = { 0, 0, 0, 0, 0 }; double back = 0;
        const bool on = c.kind == svl::ON;
        if (c.kind != svl::NONE) {
            RTARRAY& a0 = on ? o6 : o1;
            if (on) { v[0] = o6[lc]; v[1] = o7[lc]; v[2] = o8[lc]; v[3] = o9[lc]; v[4] = o10[lc]; }
            else    { v[0] = o1[lc]; v[1] = o2[lc]; v[2] = o3[lc]; v[3] = o4[lc]; v[4] = o5[lc]; }
            const int kb = lc - svl::SLOPE_BARS;
            if (kb >= 0) { const svl::Bar bb2 = barAt(kb); const svl::Cls c2 = svl::classify(bb2.days, bb2.endSec, per, openMin, closeMin);
                           if (c2.sess == c.sess && (c2.kind == c.kind || (!on && c2.kind == svl::RTH))) back = (double)a0[kb]; }
        }
        S.mstKv = svl::vwapFacts(v, (double)b.c, back, svl::decOf(market)); S.mstBar = bar;
    }
    if (!newBar && S.mstPosted && S.mstMs >= 0 && now >= S.mstMs && now - S.mstMs < 10000) return;
    mst::publish("VWAP", sym, spb, chart, now, S.mstBar, S.mstKv);
    if (lsperf::smartAllowed(LSPERF_PLUGIN, sym)) {   // (1.7.0) PerfGuard: self-awareness only while this indicator is within budget
    if (S.mstBar == bar) {   // (1.7.0) SELF-AWARENESS (SelfAware.h, SELF-AWARE.md): a close beyond the 2nd band is a reversion call (above +2 = down,
        // below -2 = up), once per touch; HIT = 1 ATR back toward VWAP before 1 ATR further (later bars only). Value = the VWAP slope. OFF.
        RTARRAY op(barOpen);
        if (op.count > lc) {
            static const sa::Spec SP = [] { sa::Spec x; x.valueKey = "slope"; x.sideKey = "pos"; x.onChange = true; x.rearm = true;
                x.sideMap = { { "above2", -1 }, { "below2", 1 }, { "above1", 0 }, { "below1", 0 }, { "above", 0 }, { "below", 0 } };
                x.why = { "pos", "vwap", "sd2_hi", "sd2_lo", "slope" }; return x; }();
            S.self.ind = "VWAP";
            sa::fromFacts(S.self, SP, sym, spb, chart, market, now, sa::Bar{ bar, op[lc], b.h, b.l, b.c }, S.mstKv);
        }
    }
    }
    S.mstPosted = true; S.mstSym = sym; S.mstSpb = spb; S.mstChart = chart; S.mstMs = now;
}

svl::Bar SessionVWAP::barAt(int i)
{
    RTARRAY hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume), dt(barDateTime);
    struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
    svl::Bar b; b.days = svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    b.endSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    b.h = hi[i]; b.l = lo[i]; b.c = cl[i]; b.v = (double)vo[i];
    return b;
}

// (1.4.0) vwapTrend of the session holding bar `lastClosed`, for the session kind `kind`, from its own bars (VwapTrendLogic.h);
// at most one session of bars is read (capped at 2,000) and the result is cached until the last closed bar changes
vtl::Trend SessionVWAP::trendFor(int lastClosed, const std::string& market, int kind, SVState& S)
{
    vtl::Trend none;
    long n = getBarCount();
    if (lastClosed < 0 || lastClosed >= (int)n) return none;
    RTARRAYI dt(barDateTime);
    RTDATE d0 = (RTDATE)dt[lastClosed];
    if (S.trIdx == lastClosed && S.trDate == d0 && S.trMkt == market && S.trKind == kind && S.trPer == S.per) return S.tr;
    int openMin = 0, closeMin = 0;
    if (!rthOf(market, openMin, closeMin)) return none;
    const int per = S.per > 0 ? S.per : 180;
    svl::Bar lb = barAt(lastClosed);
    long sess = svl::classify(lb.days, lb.endSec, per, openMin, closeMin).sess;
    int s = lastClosed;
    while (s > 0 && lastClosed - s < 2000) {
        svl::Bar b = barAt(s - 1);
        svl::Cls c = svl::classify(b.days, b.endSec, per, openMin, closeMin);
        if (c.sess != sess || c.kind == svl::NONE) break;
        s--;
    }
    std::vector<svl::Bar> B; B.reserve((size_t)(lastClosed - s + 1));
    for (int i = s; i <= lastClosed; i++) B.push_back(barAt(i));
    S.tr = vtl::fromChart(B, (int)B.size(), per, openMin, closeMin, kind);
    S.trIdx = lastClosed; S.trDate = d0; S.trMkt = market; S.trKind = kind; S.trPer = S.per;
    return S.tr;
}

int SessionVWAP::badgeOdds(int last, int lastClosed, const std::string& market, int kind, int endMin, const float* v, double px,
                           int pv[5], bool& proven, SVState& S)
{
    for (int j = 0; j < 5; j++) pv[j] = -1;
    proven = false;
    long n = getBarCount();
    if (last < 0 || last >= (int)n || !(v[0] > 0) || !std::isfinite(px)) return 0;
    int openMin = 0, closeMin = 0;
    if (!rthOf(market, openMin, closeMin)) return 0;
    double pr[5] = { -1, -1, -1, -1, -1 };
    RTARRAY clN(barClose); RTARRAYI dtk(barDateTime);
    if (kind == svl::RTH) {
        const TPLaw* L = lawFor(market);
        if (!L) return 0;
        RTDATE lastDate = (RTDATE)dtk[last];
        if (S.featBars != last + 1 || S.featDate != lastDate || S.featMkt != market) {
            S.featOk = forecast(*L, last + 1, market, S); S.featBars = last + 1; S.featDate = lastDate; S.featMkt = market;
        }
        if (!S.featOk) return 0;
        for (int j = 0; j < 5; j++) if (v[j] > 0) pr[j] = pTouch((double)v[j] - px, S.V60, S.nuEff, S.kEff);
    } else if (kind == svl::ON) {
        const otl::OT op = otl::paramsFor(market.c_str());                // the overnight champion (OnTouchParams.h)
        if (last < op.win) return 0;
        double clw[64]; int k = 0;
        for (int i = last - op.win; i <= last && k < 64; i++) clw[k++] = (double)clN[i];
        double sg = otl::onSigmaMin(clw, k, S.per > 0 ? S.per : 180, op.win);
        if (!(sg > 0)) return 0;
        for (int j = 0; j < 5; j++) if (v[j] > 0) pr[j] = otl::onTouch((double)v[j] - px, sg, 60, op.nu, op.k);
    } else return 0;
    const VOCal* cal = vol::find(market.c_str(), kind);
    vtl::Trend tr = trendFor(lastClosed, market, kind, S);
    const int tod = vol::todIndex(kind, endMin, openMin, closeMin);
    const double sd = (double)v[1] - (double)v[0];
    for (int j = 0; j < 5; j++) {
        if (!(v[j] > 0) || !std::isfinite(pr[j]) || pr[j] < 0) continue;
        double dist = (double)v[j] - px;
        double q = vol::calibrate(cal, j, pr[j], dist, tr.state, tr.via, tod, sd > 0 ? std::fabs(dist) / sd : std::numeric_limits<double>::quiet_NaN());
        if (std::isfinite(q)) { double qc = q < 0 ? 0.0 : (q > 1 ? 1.0 : q); pv[j] = (int)std::floor(100.0 * qc + 0.5); }
    }
    proven = vol::proven(cal);
    return kind == svl::RTH ? 2 : 1;
}

void SessionVWAP::writeStatus(const char* what, int nRth, const std::string& market, int nOn)
{
    SVState* S = state(true);
    const char* up = getenv("USERPROFILE"); if (!up) return;
    time_t now_ = time(nullptr);
    bool changed = !S || S->statusWhat != what || S->statusNRth != nRth || S->statusMkt != market;
    if (S && !changed && now_ - S->lastStatusWrite < 30) return;
    std::ostringstream text;
    text << "VERSION," << SV_VERSION << "\nMARKET," << market << "\nRTH_BARS," << nRth << "\nON_BARS," << nOn << "\nSTATE," << what << "\n";
    if (!svPublishText(svRecordPath(this, up, market, S ? S->per : 180, "status.txt"), text.str())) return;
    if (S) { S->lastStatusWrite = now_; S->statusWhat = what; S->statusNRth = nRth; S->statusMkt = market; }
}

extern "C" cppExtension *CreateExtension(void)
{
    SessionVWAP *p = new SessionVWAP();
    p->setArrayCount(11);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE);
    p->setDescription("LRA Session VWAP: the session VWAP with its +-1 and +-2 SD bands - RTH in the day session, the overnight VWAP and the prior RTH VWAP overnight.");
    p->setVersion("1.8.0");   // = SV_VERSION (gex-build reads this literal; test_sessionvwap_settings checks they match)
    return p;
}
