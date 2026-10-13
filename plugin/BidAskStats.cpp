/********************************************************************************
 *  BidAskStats.cpp  --  Investor/RT RTX extension  lsBidAskStats  1.3.0 (2026-10-13)
 *
 *  1.3.0 (2026-10-13): WHO IS ATTACKING. Approved mockups final-signals-mockup.png / three-panes-messages-mockup.png.
 *   - ONE plain-English MESSAGE LINE centred at the top of the pane for the candle under the mouse (TRACK_MOUSE, as 1.1.0) else the
 *     last closed candle, bold, coloured by IMPLICATION (green bullish, red bearish, #D8DDE3 "Normal activity - no spike"), prefixed
 *     with the candle's 12-hour Central time: "10:24  Sellers swept 4 levels with one 120-lot into LonLO - price held: buyers absorbing".
 *   - THE INFO BOX STAYS (Rassul 12:05 "all values go in the info box, which I should be able to see as I move my mouse over different
 *     bars"): the 1.2.0 box (BBid / BAsk / BidRatio / AskRatio / Jump, Position default Top left, fixed size, "Live" or the candle's
 *     time) + Sweep (levels, side) + Large (lots, side); sellers' values red, buyers' green. The message line centres in the width
 *     beside the box and moves with it on hover. The record keeps the numbers line ("BBid 240  BidRatio 4.1x  Jump 2.3x ...").
 *   - SPIKE LABELS (Rassul 12:03 "i want the quantity and a ratio on top of the spikes"): above every spike bar the ratio over the
 *     quantity ("6.4x" over "240") in the spike colour; adjacent labels stagger, never over the box / letters / message line. The
 *     pane now has headroom (the tallest bar <= ~69% of the pane; the axis from scale() matches the bars exactly).
 *     Rules (BidAskStatsLogic.h): a side is HIGH when a spike or >= 2.0x its average; bids hit + price held (the close did not break the
 *     prior low by > 1 tick) = "buyers absorbing" (green), broke = "price giving way" (red); offers lifted mirrored. " into <LEVEL>"
 *     only when a key / gamma level (LRA-KeyLevels / LRA-Dealer / LRA-Levels, read as lsDeltaProfile 2.7.0) is within the Liquidity
 *     Profile 0.14.0 near band of the candle's close.
 *   - LETTERS from the order-by-order feed (OrderFlowFeed.h, shared with lsPullStats / lsStackStats; DEPTH_BY_ORDER as lsMboCheck):
 *     Rassul "include L for Bid Ask Stacks and Pull". S = one aggressor order filling at >= sweepLevels (3) prices; L = one aggressor
 *     order >= largeLotAggr (99th percentile of this session's aggressor sizes before the candle, at least 20 lots). White box, black
 *     letter, at the BOTTOM of the side's bar (sellers -> the red bid bar, buyers -> the green ask bar), priority S then L, at most 2,
 *     never overlapping; only on closed candles where the feed was live (a candle is decided 0.5 s after it closed with the drain
 *     caught up, or 5 s after). CALL_CONTINUOUSLY added (the feed is drained while no trade prints).
 *   - Every closed candle of the session (numbers, letters, message) is written once to
 *     lsFlexLevels\BidAskStats\<MKT>-<CONTRACT>-<spb>-<session>.csv (atomic), re-read on load (a restart shows the same letters and
 *     messages); per-market thresholds from lsFlexLevels\LRA-OrderFlow-<MKT>.csv (NOTES.md; missing / bad -> defaults, reasons in the
 *     status file; its spike values win over the dialog's, the dialog's 0 = off always wins).
 *   - Bars, spikes, ratios, hover, axis and settings exactly as 1.2.0 (1.0.0 / 1.1.0 / 1.2.0 settings files still load).
 *  1.2.0 (2026-10-13):
 *   - SPIKES. Rassul 10:47 "i want to identify spikes on the bid ask indicator and color them differently like lime and some
 *     other color. for example it is not only over 2x but it is bigger than the prior biggest bid and ask values by at least 1.5x".
 *     Then 10:51 "forget the gray lines.. lets just color the spikes Green and a brighter version of red."
 *     A biggest bid (ask) is a spike when, as of its bar (prior bars only): >= "Spike vs average" (2.0) x that side's average
 *     AND >= "Spike vs prior biggest" (1.5) x the biggest bid OR ask of the prior "Spike lookback" (10) candles. Ask spike (buyers)
 *     = bright green #00FF00, bid spike (sellers) = bright red #FF3B3B; normal bars green #1F9D55, red #962D2D. A closed bar's colour
 *     is decided once at its close (only a settings change re-decides); the forming bar's colour may change until it closes.
 *   - The 1.1.0 gray Ratio lines and their "Ratio line" setting are REMOVED (10:51). A 1.1.0 settings file's third field is ignored.
 *  1.1.0 (2026-10-13): HOVER (Rassul 10:33 "the bid ask indicator info box should depend on mouse over.. i should be able to move it
 *   around and see the numbers change."), the pane's OWN VALUE AXIS (contracts, not prices) via scale().
 *  1.0.0 (2026-10-13): BIDASKSTATS - the biggest bid and the biggest ask in each candle (Rassul 06:34). Its own pane, two bars per
 *   candle on ONE shared scale: red (left) = the most contracts sold into the bid at one price, green (right) = the most bought at
 *   the ask at one price. Source: IRT's native volume at price (VOLPROFILE.sellVolume = at the bid, buyVolume = at the ask).
 *   Closed bars only for every statistic (computed once, never changed); a failed VAP read = n/a (never 0), retried for IRT back-fill;
 *   a native fault quarantines this chart's copy. Status: lsFlexLevels\BidAskStats.status-<MKT>-<CONTRACT>-<secs>.txt.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "HostSlot.h"
#define OFF_SDK 1
#include "BidAskStatsLogic.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define BAS_VERSION "1.3.0"          // keep equal to bas::VERSION and the setVersion literal below

static const COLOR C_ASK = 0x0022C55E, C_BID = 0x00EF4444, C_LABEL = 0x00FFFFFF, C_BORDER = 0x009CA3AF, C_BLACK = 0x00000000;
static const int WINDOW_BARS = 3000;          // closed bars read back from the last one (BBid is capped at the bars available)
static const int READS_PER_PASS = 600;        // bounded work per pass; the rest continues on the next pass
static const int NA_TRIES = 10;               // retries of a closed n/a bar (IRT back-fill)
static const size_t KEEP_BARS = 5000;
static const int RATIO_PARM = 0, POS_PARM = 1, SPIKE_AVG_PARM = 2, SPIKE_JUMP_PARM = 3, SPIKE_LB_PARM = 4;
static const COLOR C_ASK_BAR = 0x001F9D55, C_BID_BAR = 0x00962D2D;     // normal histogram bars: darker green / red
static const COLOR C_ASK_SPIKE = 0x0000FF00, C_BID_SPIKE = 0x00FF3B3B; // spikes: bright green (buyers) / bright red (sellers)
static const size_t MAX_HOVER_WNDS = 8;
static const int BAS_NUMS = 4 + off::REC_NUMS;                         // session record: bid|ask|bbid|bask + the order-feed numbers

#if defined(BAS_TEST_CLOCK)
extern void* bas_test_cursor_wnd;
static void* cursorWindow() { return bas_test_cursor_wnd; }
#elif defined(_WIN32)
#pragma comment(lib, "user32.lib")   // (2026-10-13 1.3.0a) GetCursorPos / WindowFromPoint - link fix
static void* cursorWindow() { POINT p; if (!GetCursorPos(&p)) return NULL; return (void*)WindowFromPoint(p); }
#else
static void* cursorWindow() { return NULL; }
#endif

#ifdef BAS_TEST_CLOCK
extern long long bas_test_now_ms;
static long long steadyMs() { return bas_test_now_ms; }
#else
static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
#endif
using off::coord; using off::flexDir; using off::makeDir; using off::fileStamp; using off::publishFile; using off::fileKey; using off::marketName;
// strict "ratioBars,position[,spikeAvgTenths,spikeJumpTenths,spikeLookback]" (whole numbers, in range);
// a 1.0.0 file (two fields) and a 1.1.0 file (three: its Ratio line field, read and ignored) = the default spike settings
static bool parseSettings(const std::string& line, int& rb, int& pos, bas::SpikeCfg* spike = nullptr) {
    if (line.empty() || line.size() > 32) return false;
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = line.find(',', a); f.push_back(line.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    if (f.size() != 2 && f.size() != 3 && f.size() != 5) return false;
    const size_t maxLen5[5] = { 3, 1, 3, 3, 2 }, maxLen3[3] = { 3, 1, 3 };
    const size_t* maxLen = f.size() == 3 ? maxLen3 : maxLen5;
    for (size_t k = 0; k < f.size(); ++k) {
        if (f[k].empty() || f[k].size() > maxLen[k]) return false;
        for (size_t i = 0; i < f[k].size(); ++i) if (!std::isdigit((unsigned char)f[k][i])) return false;
    }
    const int r = std::atoi(f[0].c_str()), p = f[1][0] - '0';
    if (bas::clampRatioBars(r) < 0 || p < 0 || p > 8) return false;
    if (f.size() == 3 && bas::clampLineTenths(std::atoi(f[2].c_str())) < 0) return false;
    bas::SpikeCfg c;
    if (f.size() == 5) {
        c.avgT = std::atoi(f[2].c_str()); c.jumpT = std::atoi(f[3].c_str()); c.lookback = std::atoi(f[4].c_str());
        if (bas::clampLineTenths(c.avgT) < 0 || bas::clampLineTenths(c.jumpT) < 0 || bas::clampLookback(c.lookback) < 0) return false;
    }
    rb = r; pos = p; if (spike) *spike = c; return true;
}
// (1.3.0) the spike settings in force: the params file's values where it sets them, else the dialog's; the dialog's 0 = off wins
static bas::SpikeCfg effSpike(const bas::SpikeCfg& d, const off::Params& P) {
    off::SpikeCfg o; o.avgT = d.avgT; o.jumpT = d.jumpT; o.lookback = d.lookback;
    const off::SpikeCfg e = off::effectiveSpike(o, P);
    bas::SpikeCfg c; c.avgT = (int)e.avgT; c.jumpT = (int)e.jumpT; c.lookback = (int)e.lookback; return c;
}

// (1.3.0) a closed candle's attack read, decided once
struct BasFin {
    bool dbo = false, fromFile = false;      // dbo: the order feed was live for it (letters possible)
    off::Rec r; std::string let[2]; std::string level; int held[2] = {1, 1};
    nl::Msg msg;
};

struct BASState {
    bool busy = false, faulted = false;
    std::string identity, market, symbol, state = "starting", lastStatusText;
    int spb = 0; double tick = 0;
    bas::Series ser;                                 // closed bars: time -> stats (computed once)
    std::map<long long, int> naTries;                // closed n/a bars still being retried: time -> attempts
    long count = 0; long long lastStamp = 0; bool backlog = true;
    long long lastClosedTime = 0;                    // the last CLOSED bar's stamp
    long long liveTime = 0; bas::BarMax live;        // the forming bar (drawn only)
    long long liveReadMs = -1, retryMs = -1, statusMs = -1, settingsCheckMs = -1, settingsStamp = -2;
    int ratioBars = bas::RATIO_DEFAULT, pos = bas::POS_DEFAULT;
    bas::SpikeCfg dlgSpike;                          // (1.3.0) the dialog's spike settings (the params file may override)
    bas::Hover hover;                                // the closed bar under the mouse, -1 = Live
    std::vector<void*> hoverWnds;                    // windows the chart's mouse events came from (most recent last)
    long long redraws = 0;
    long long axisTop = 0; int paneH = 0;
    long long vapReads = 0, vapFailures = 0, vapMismatch = 0;
    // (1.3.0) the order feed, the attack reads, the session record, params, levels
    off::Conn feed; off::Engine e; off::Acc scratch; nl::Zone zone; double atr = 0; long atrCount = -1;
    std::map<long long, off::Acc> accs; std::map<long long, long long> closedAt;
    std::map<long long, int> barEnds;                // bar end -> chart index (the last 3,001 bars)
    long long formingKey = 0, firstBarKey = -1, finFrom = 0;
    long long lateDropped = 0, noBar = 0, decidedLate = 0, finalized = 0;
    std::map<long long, BasFin> fin;
    off::SessionFile file; std::string curSession; bool loaded = false;
    nl::Cache lv; std::vector<nl::Level> levels;
    off::ParamCache params;
};

class BidAskStats : public cppExtension {
public:
    BidAskStats() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoad(void) { return RTX_OK; }                       // loading never saves (lesson 29)
    virtual int parmsApply(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { saveSettings(); return RTX_OK; }
    virtual int mouse(RTX_EVENT* e);
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax);
    bool mouseLeft(BASState& S);
    void pump();
    void release() { slot_.release(this); }
    BASState* testState() { return slot_.get(this, false); }
private:
    HostSlot<BASState> slot_;
    void identify(BASState& S);
    void settings(BASState& S, long long now);
    void saveSettings();
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    int readBar(BASState& S, RTARRAYP& vap, int bar, long long volume, bas::BarMax& out);
    void closedBars(BASState& S, long count, long long now);
    void formingBar(BASState& S, long count, long long now);
    void orderBars(BASState& S, long count, long long now);
    void route(BASState& S, const off::Ev& x);
    void loadSession(BASState& S);
    void finalize(BASState& S, long long now);
    void reMessage(BASState& S);
    BasFin provisional(BASState& S, long long t);
    std::string levelAt(BASState& S, long long t);
    void heldOf(BASState& S, long long t, int out[2]);
    void status(BASState& S, long long now);
    void render(BASState& S);
    std::string stamp(long long t);
    std::string ctOf(long long t);
    std::string sessionStr(long long t);
    long long sessionKey(long long t);
    std::string placePath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\BidAskStats.place-" + mkt + ".txt"; }
    std::string stem(BASState& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\BidAskStats"; }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<BidAskStats*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BidAskStats*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // controls only: Ratio bars, Position (9 spots), the three spike settings (0 = off). Unchanged list since 1.2.0.
    setParameterVersion(3);
    setParameterDialogHeight(2);
    setIntegerParameter("Ratio bars", bas::RATIO_DEFAULT, 50);
    setListParameter("Position", (short)bas::POS_DEFAULT, bas::POS_CHOICES, 110, kParmAppendSameLine);
    setFloatParameter("Spike vs average", (float)bas::SPIKE_AVG_DEFAULT_TENTHS / 10.0f, 50);
    setFloatParameter("Spike vs prior biggest", (float)bas::SPIKE_JUMP_DEFAULT_TENTHS / 10.0f, 50, kParmAppendSameLine);
    setIntegerParameter("Spike lookback", bas::SPIKE_LOOKBACK_DEFAULT, 50, kParmAppendSameLine);
    return RTX_OK;
}
int cppExtension::calc(int) {
    BidAskStats* x = static_cast<BidAskStats*>(this);
    x->pump();
    BASState* S = x->testState();
    if (S && x->mouseLeft(*S)) { ++S->redraws; x->invalidateChart(false); }   // the mouse left the chart: back to Live
    return RTX_OK;
}
int BidAskStats::draw(void) {
    pump();
    BASState* S = slot_.get(this, false);
    if (S) mouseLeft(*S);
    if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
void BidAskStats::identify(BASState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR); if (!bas::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity && tick == S.tick) return;
    S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
    S.ser.bars.clear(); S.naTries.clear(); S.count = 0; S.lastStamp = 0; S.backlog = true; S.lastClosedTime = 0;
    S.liveTime = 0; S.live = bas::BarMax(); S.liveReadMs = -1; S.faulted = false; S.settingsStamp = -2; S.settingsCheckMs = -1;
    S.hover.set(-1); S.hoverWnds.clear();
    S.feed.reset(); S.e = off::Engine(); S.scratch = off::Acc(); S.zone = nl::zoneFor(S.market); S.atr = 0; S.atrCount = -1;
    S.accs.clear(); S.closedAt.clear(); S.barEnds.clear(); S.formingKey = 0; S.firstBarKey = -1; S.finFrom = 0; S.fin.clear();
    S.file = off::SessionFile(); S.curSession.clear(); S.loaded = false; S.params = off::ParamCache(); S.lastStatusText.clear();
}
// the mouse over a bar: the info box and the message line follow it. Never swallows the event (the chart keeps its own mouse behaviour).
int BidAskStats::mouse(RTX_EVENT* e) {
    if (!e) return RTX_FAIL;
    BASState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER) return RTX_FAIL;
    void* w = cursorWindow();
    if (w) {
        std::vector<void*>::iterator it = std::find(S->hoverWnds.begin(), S->hoverWnds.end(), w);
        if (it != S->hoverWnds.end()) S->hoverWnds.erase(it);
        S->hoverWnds.push_back(w);
        if (S->hoverWnds.size() > MAX_HOVER_WNDS) S->hoverWnds.erase(S->hoverWnds.begin());
    }
    if (S->hover.set(bas::hoverIndex(e->v.mouse.bar, getBarCount()))) { ++S->redraws; invalidateChart(false); }   // only on a change
    return RTX_FAIL;
}
// this pane's own value axis in contracts (not the instrument's prices): the tallest bid / ask in the bars IRT scales
int BidAskStats::scale(int iStartBar, int iEndBar, double* dMin, double* dMax) {
    double mn = 0, mx = 10;
    BASState* S = slot_.get(this, false);
    if (S) {
        const long count = getBarCount();
        RTARRAYI times(barDateTime);
        long long top = 0;
        if (count > 0 && times.count >= count) {
            const int last = (int)count - 1;
            int a = std::max(0, iStartBar), b = std::min(last, iEndBar);
            if (b < a) { a = std::max(0, last - 200); b = last; }
            if (b - a > 20000) a = b - 20000;
            for (int i = a; i <= b; ++i) {
                const long long bt = (long long)times[i];
                if (i == last) { if (S->liveTime == bt && S->live.ok) top = std::max(top, std::max(S->live.bid, S->live.ask)); continue; }
                std::map<long long, bas::Stat>::const_iterator it = S->ser.bars.find(bt);
                if (it != S->ser.bars.end() && it->second.m.ok) top = std::max(top, std::max(it->second.m.bid, it->second.m.ask));
            }
        }
        S->axisTop = top;
        off::axisFor(top, S->paneH, off::drawPx(S->paneH, off::textReserve(S->paneH)), mn, mx);
    }
    if (dMin) *dMin = mn;
    if (dMax) *dMax = mx;
    return RTX_OK;
}
bool BidAskStats::mouseLeft(BASState& S) {
    if (S.hover.bar < 0 || S.hoverWnds.empty()) return false;
    void* w = cursorWindow(); if (!w) return false;
    if (std::find(S.hoverWnds.begin(), S.hoverWnds.end(), w) != S.hoverWnds.end()) return false;
    return S.hover.set(-1);
}
// the per-market settings file, re-read only when its stamp changes (checked at most once a second)
void BidAskStats::settings(BASState& S, long long now) {
    if (S.settingsCheckMs >= 0 && now >= S.settingsCheckMs && now - S.settingsCheckMs < 1000) return;
    S.settingsCheckMs = now;
    const std::string p = placePath(S.market);
    const long long st = p.empty() ? -1 : fileStamp(p);
    if (st == S.settingsStamp) return;
    S.settingsStamp = st;
    int rb = bas::RATIO_DEFAULT, pos = bas::POS_DEFAULT; bas::SpikeCfg sc;
    if (st >= 0) {
        std::ifstream f(p.c_str(), std::ios::binary); std::string ln;
        if (f && std::getline(f, ln)) {
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            int r = 0, q = 0; bas::SpikeCfg c; if (parseSettings(ln, r, q, &c)) { rb = r; pos = q; sc = c; }
        }
    }
    S.pos = pos; S.dlgSpike = sc;
    const bas::SpikeCfg eff = effSpike(sc, S.params.load.p);
    const bool reratioed = rb != S.ratioBars;
    if (reratioed) { S.ratioBars = rb; S.ser.reratio(rb); }
    if (reratioed || eff != S.ser.spike) { S.ser.respike(eff); reMessage(S); }   // only a settings change re-decides closed bars
}
void BidAskStats::saveSettings() {
    const int rb = getIntegerValue(RATIO_PARM), pos = getListIndex(POS_PARM);
    const int sa = bas::tenthsOf(getFloatValue(SPIKE_AVG_PARM)), sj = bas::tenthsOf(getFloatValue(SPIKE_JUMP_PARM)), sl = getIntegerValue(SPIKE_LB_PARM);
    if (bas::clampRatioBars(rb) < 0 || pos < 0 || pos > 8) return;
    if (bas::clampLineTenths(sa) < 0 || bas::clampLineTenths(sj) < 0 || bas::clampLookback(sl) < 0) return;   // never overwrite a saved choice
    char b[32]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const char* sy = getSymbol();
    const std::string root = rs ? rs : "";
    const std::string mkt = marketName(root.empty() ? (sy ? sy : "") : root);
    const std::string p = placePath(mkt); if (p.empty()) return;
    makeDir(flexDir());
    publishFile(p, std::to_string(rb) + "," + std::to_string(pos) + "," + std::to_string(sa) + "," + std::to_string(sj) + "," + std::to_string(sl) + "\n");
    BASState* S = slot_.get(this, false);
    if (S) { S->settingsCheckMs = -1; S->settingsStamp = -2; }
}
// MSVC SEH helpers hold only trivial locals (/EHsc compatible). -1 = native fault, -2 = not available.
int BidAskStats::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (vap->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
int BidAskStats::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) {
            std::memset(&rows[i], 0, sizeof(rows[i]));
            if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2;
        }
        return count;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// 1 = read, 0 = n/a, -1 = native fault
int BidAskStats::readBar(BASState& S, RTARRAYP& vap, int bar, long long volume, bas::BarMax& out) {
    out = bas::BarMax(); ++S.vapReads;
    BARSTATISTICS st;
    const int prices = readStatistics(&vap, bar, &st);
    if (prices == -1) return -1;
    if (prices < 0 || (prices == 0 && volume > 0)) return 0;
    std::vector<VOLPROFILE> rows((size_t)prices);
    const int got = readPrices(&vap, bar, rows.empty() ? NULL : &rows[0], prices);
    if (got == -1) return -1;
    if (got != prices) return 0;
    std::vector<bas::Row> r; r.reserve(rows.size());
    long long sb = 0, sa = 0;
    for (size_t k = 0; k < rows.size(); ++k) {
        bas::Row x; x.price = (double)rows[k].price; x.bid = (long long)rows[k].sellVolume; x.ask = (long long)rows[k].buyVolume;
        sb += x.bid; sa += x.ask; r.push_back(x);
    }
    if (sb != (long long)st.sellVolume || sa != (long long)st.buyVolume) ++S.vapMismatch;
    out = bas::maxOf(r, S.tick);
    return out.ok ? 1 : 0;
}
void BidAskStats::closedBars(BASState& S, long count, long long now) {
    if (count < 2 || S.tick <= 0) return;
    RTARRAYI times(barDateTime), volume(barVolume); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || volume.count < count || vap.count < count) { S.state = "chart data arrays unavailable"; return; }
    const int lastClosed = (int)count - 2;
    const long long lastStamp = (long long)times[lastClosed];
    const bool retryDue = !S.naTries.empty() && (S.retryMs < 0 || now < S.retryMs || now - S.retryMs >= 1000);
    if (count == S.count && lastStamp == S.lastStamp && !S.backlog && !retryDue) return;   // nothing new: no VAP touched
    if (retryDue) S.retryMs = now;
    const int first = std::max(0, lastClosed - WINDOW_BARS + 1);
    int reads = 0; bool more = false;
    for (int i = first; i <= lastClosed; ++i) {                            // oldest first: a bar's prior bars are in before it
        const long long bt = (long long)times[i];
        if (bt <= 0) continue;
        std::map<long long, bas::Stat>::const_iterator have = S.ser.bars.find(bt);
        std::map<long long, int>::iterator na = S.naTries.find(bt);
        if (have != S.ser.bars.end() && (have->second.m.ok || na == S.naTries.end())) continue;   // decided: never recomputed
        if (have != S.ser.bars.end() && !retryDue) continue;
        if (reads >= READS_PER_PASS) { more = true; break; }
        ++reads;
        bas::BarMax m;
        const int r = readBar(S, vap, i, (long long)volume[i], m);
        if (r < 0) { S.faulted = true; S.state = "VAP fault: reload indicator to retry"; return; }
        if (r == 0) {
            ++S.vapFailures;
            if (have == S.ser.bars.end()) { S.ser.add(bt, m, S.ratioBars); S.naTries[bt] = 1; }
            else if (++na->second >= NA_TRIES) S.naTries.erase(na);
            continue;
        }
        if (na != S.naTries.end()) S.naTries.erase(na);
        S.ser.add(bt, m, S.ratioBars);
    }
    S.backlog = more;
    S.count = count; S.lastStamp = lastStamp;
    if (!more) S.lastClosedTime = lastStamp;
    else S.state = "loading bars";
    if (S.ser.bars.size() > KEEP_BARS) {
        S.ser.trim(KEEP_BARS);
        for (std::map<long long, int>::iterator it = S.naTries.begin(); it != S.naTries.end();)
            if (!S.ser.bars.count(it->first)) S.naTries.erase(it++); else ++it;
    }
}
// the forming bar: its live pair only (drawn, never recorded), read at most 4 times a second
void BidAskStats::formingBar(BASState& S, long count, long long now) {
    if (count < 1 || S.tick <= 0 || S.faulted) return;
    RTARRAYI times(barDateTime), volume(barVolume); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || volume.count < count || vap.count < count) return;
    const int i = (int)count - 1;
    const bool newBar = (long long)times[i] != S.liveTime;
    if (!newBar && S.liveReadMs >= 0 && now >= S.liveReadMs && now - S.liveReadMs < 250) return;
    S.liveReadMs = now;
    bas::BarMax m; const int r = readBar(S, vap, i, (long long)volume[i], m);
    if (r < 0) { S.faulted = true; S.state = "VAP fault: reload indicator to retry"; return; }
    S.liveTime = (long long)times[i]; S.live = m;
}
std::string BidAskStats::stamp(long long t) {
    struct tm x; std::memset(&x, 0, sizeof(x));
    if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return "n/a";
    char b[64]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d", x.tm_year + 1900, x.tm_mon + 1, x.tm_mday, x.tm_hour, x.tm_min, x.tm_sec);
    return b;
}
std::string BidAskStats::ctOf(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return std::string(); return off::clockShort(x.tm_hour, x.tm_min); }
std::string BidAskStats::sessionStr(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return std::string(); return off::sessionOf(x.tm_year + 1900, x.tm_mon + 1, x.tm_mday, x.tm_hour); }
// the trading session of a bar stamp: the local (Central) civil day, a bar at / after 17:00 counted in the next day; -1 = unreadable
long long BidAskStats::sessionKey(long long t) {
    struct tm x; std::memset(&x, 0, sizeof(x));
    if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return -1;
    return off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) + (x.tm_hour >= 17 ? 1 : 0);
}
// (1.3.0) chart bars for the order feed: bar ends -> index, the forming bar, ATR + near band, the fallback last price, the session
void BidAskStats::orderBars(BASState& S, long count, long long now) {
    RTARRAYI times(barDateTime); RTARRAY high(barHigh), low(barLow), close(barClose);
    if (times.count < count || high.count < count || low.count < count || close.count < count) return;
    const long first = std::max(0L, count - (long)WINDOW_BARS - 1);
    if (S.barEnds.empty() || S.barEnds.rbegin()->first != (long long)times[(int)count - 1] || (long)S.barEnds.size() != count - first) {
        S.barEnds.clear();
        for (long i = first; i < count; ++i) { const long long bt = (long long)times[(int)i]; if (bt > 0) S.barEnds[bt] = (int)i; }
    }
    S.formingKey = (long long)times[(int)count - 1];
    if (count != S.atrCount) {
        S.atrCount = count;
        std::vector<double> rg;
        for (int i = (int)count - 2; i >= 0 && rg.size() < 20; --i) rg.push_back((double)high[i] - (double)low[i]);
        S.atr = nl::atrOf(rg, S.spb);
        S.e.near = nl::nearBand(S.atr, S.tick, S.zone, 0);
    }
    off::Tick px = 0;
    if (off::toTick((double)close[(int)count - 1], S.tick, px)) S.e.fallbackRef(px, now);
    const std::string ss = sessionStr(S.formingKey);
    if (!ss.empty() && ss != S.curSession) {
        if (!S.curSession.empty()) { S.file.save(); S.e.newSession(); }
        S.curSession = ss; S.loaded = false;
    }
}
void BidAskStats::route(BASState& S, const off::Ev& x) {
    const long long key = off::eventKey(S.barEnds, S.formingKey, S.spb, x.stamp);
    const bool noBar = key < 0, decided = !noBar && S.fin.count(key) > 0, before = !noBar && S.firstBarKey > 0 && key < S.firstBarKey;
    const bool early = !noBar && S.firstBarKey < 0 && key < S.formingKey;
    if (noBar || decided || before || early) {
        if (noBar) S.noBar++; else S.lateDropped++;
        S.e.apply(S.scratch, x);                                           // still learns the order; the counts are thrown away
        if (S.scratch.events > 20000) S.scratch = off::Acc();
        return;
    }
    S.e.apply(S.accs[key], x);
}
// (1.3.0) this session's record, once: rows come back at finalize (letters, levels, messages exactly as decided)
void BidAskStats::loadSession(BASState& S) {
    if (S.loaded || S.curSession.empty()) return;
    S.loaded = true;
    const std::string d = dir(); if (d.empty()) return;
    makeDir(flexDir()); makeDir(d);
    const std::string base = d + "\\" + stem(S) + "-" + S.curSession;
    S.file.open(S.curSession, base + ".csv", base + "-sizes.txt",
                std::string("# BidAskStats ") + BAS_VERSION + " " + S.market + " " + S.symbol + " " + std::to_string(S.spb)
                    + "\nbar_time|ct_time|dbo|bid|ask|bbid|bask|" + off::REC_COLS + "|letters_left|letters_right|spike_left|spike_right|dash_left|dash_right|implication|level|message|numbers\n",
                (size_t)BAS_NUMS);
    S.file.load();
    S.file.loadSizes(S.e.aggrDist, S.e.restDist);
    S.finFrom = 0;
}
std::string BidAskStats::levelAt(BASState& S, long long t) {
    std::map<long long, int>::const_iterator i = S.barEnds.find(t);
    if (i == S.barEnds.end() || S.e.near <= 0) return std::string();
    RTARRAY close(barClose); if (close.count <= i->second) return std::string();
    return nl::nearestLevel(S.levels, (double)close[i->second], (double)S.e.near * S.tick);
}
void BidAskStats::heldOf(BASState& S, long long t, int out[2]) {
    out[0] = out[1] = 1;
    std::map<long long, int>::const_iterator i = S.barEnds.find(t);
    if (i == S.barEnds.end()) return;
    RTARRAY close(barClose), high(barHigh), low(barLow);
    const int k = i->second;
    if (k < 1 || close.count <= k || high.count <= k || low.count <= k) return;
    for (int s = 0; s < 2; ++s) out[s] = bas::held(s, (double)close[k], (double)low[k - 1], (double)high[k - 1], S.tick, true);
}
// a closed candle not decided yet (order feed still catching up): its read without letters
BasFin BidAskStats::provisional(BASState& S, long long t) {
    BasFin F; std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(t);
    heldOf(S, t, F.held);
    if (sessionStr(t) == S.curSession) F.level = levelAt(S, t);
    if (it != S.ser.bars.end()) F.msg = bas::message(it->second, nullptr, F.let, F.level, F.held);
    else F.msg.line1 = "No volume data for this candle";
    return F;
}
// (1.3.0) closed candles are decided once: their letters (order feed), level and message; this session's go to the record
void BidAskStats::finalize(BASState& S, long long now) {
    if (S.ser.bars.empty()) return;
    const off::Params& P = S.params.load.p;
    bool any = false; long long firstPending = LLONG_MAX, lastSeen = S.finFrom;
    for (std::map<long long, bas::Stat>::iterator it = S.ser.bars.lower_bound(S.finFrom); it != S.ser.bars.end(); ++it) {
        const long long t = it->first; lastSeen = t;
        if (t >= S.formingKey && S.formingKey > 0) break;
        if (S.fin.count(t)) continue;
        if (S.naTries.count(t)) { firstPending = std::min(firstPending, t); continue; }   // VAP still being retried
        BasFin F; const std::string ss = sessionStr(t); const bool cur = !ss.empty() && ss == S.curSession;
        std::map<long long, off::Row>::const_iterator fr = cur ? S.file.rows.find(t) : S.file.rows.end();
        off::Rec rr;
        if (fr != S.file.rows.end() && off::recFrom(fr->second.nums, 4, rr)) {          // decided before a restart: exactly as then
            F.fromFile = true; F.dbo = fr->second.dbo != 0; F.r = rr; F.level = fr->second.level;
            for (int s = 0; s < 2; ++s) F.let[s] = fr->second.let[s];
            F.msg.impl = fr->second.impl; F.msg.line1 = fr->second.line1; F.msg.line2 = fr->second.line2;
            if (it->second.m.ok) { it->second.bidSpike = fr->second.spike[0] != 0; it->second.askSpike = fr->second.spike[1] != 0; }   // the colour as drawn then
            heldOf(S, t, F.held);
        } else {
            const bool dboBar = S.firstBarKey > 0 && t >= S.firstBarKey;
            if (dboBar) {
                std::map<long long, long long>::iterator c = S.closedAt.find(t);
                if (c == S.closedAt.end()) c = S.closedAt.insert(std::make_pair(t, now)).first;
                const long long age = now >= c->second ? now - c->second : 0;
                if (age < off::FINAL_GRACE_MS && (S.feed.lastCapped || age < off::DECIDE_DELAY_MS)) { firstPending = std::min(firstPending, t); continue; }
                if (S.feed.lastCapped) S.decidedLate++;
                std::map<long long, off::Acc>::iterator a = S.accs.find(t);
                F.r = S.e.close(a != S.accs.end() ? a->second : off::Acc(), t == S.firstBarKey, P); F.dbo = true;
                if (a != S.accs.end()) S.accs.erase(a);
                S.closedAt.erase(c);
                for (int s = 0; s < 2; ++s) F.let[s] = bas::letters(F.r, s, P);
            }
            if (cur) F.level = levelAt(S, t);
            heldOf(S, t, F.held);
            F.msg = bas::message(it->second, F.dbo ? &F.r : nullptr, F.let, F.level, F.held);
            if (cur && it->second.m.ok) {
                off::Row row; row.t = t; row.ct = ctOf(t); row.dbo = F.dbo ? 1 : 0;
                row.nums.push_back(it->second.m.bid); row.nums.push_back(it->second.m.ask); row.nums.push_back(it->second.bBid); row.nums.push_back(it->second.bAsk);
                off::recNums(F.r, row.nums);
                for (int s = 0; s < 2; ++s) row.let[s] = F.let[s];
                row.spike[0] = it->second.bidSpike ? 1 : 0; row.spike[1] = it->second.askSpike ? 1 : 0;
                row.impl = F.msg.impl; row.level = F.level; row.line1 = F.msg.line1; row.line2 = F.msg.line2;
                S.file.put(row);
            }
        }
        S.fin[t] = F; S.finalized++; any = true;
    }
    S.finFrom = firstPending != LLONG_MAX ? firstPending : lastSeen;
    if (any) { S.file.sizes(S.e.aggrDist, S.e.restDist); S.file.save(); invalidateChart(false); }
    while (S.fin.size() > KEEP_BARS) S.fin.erase(S.fin.begin());
    for (std::map<long long, off::Acc>::iterator a = S.accs.begin(); a != S.accs.end() && S.accs.size() > 16;) S.accs.erase(a++);
}
// a settings change re-decides the messages of every decided candle (letters and levels kept)
void BidAskStats::reMessage(BASState& S) {
    for (std::map<long long, BasFin>::iterator it = S.fin.begin(); it != S.fin.end(); ++it) {
        std::map<long long, bas::Stat>::const_iterator st = S.ser.bars.find(it->first);
        if (st == S.ser.bars.end()) continue;
        it->second.msg = bas::message(st->second, it->second.dbo ? &it->second.r : nullptr, it->second.let, it->second.level, it->second.held);
    }
}
void BidAskStats::status(BASState& S, long long now) {
    if (S.statusMs >= 0 && now >= S.statusMs && now - S.statusMs < 60000) return;   // at most once a minute
    const bas::Stat* st = NULL;
    std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(S.lastClosedTime);
    if (it != S.ser.bars.end()) st = &it->second;
    const bool ok = st && st->m.ok;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << BAS_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nRATIO_BARS," << S.ratioBars << "\nPOSITION," << S.pos
      << "\nSPIKE_VS_AVERAGE," << bas::fmtTenths(S.ser.spike.avgT) << "\nSPIKE_VS_PRIOR_BIGGEST," << bas::fmtTenths(S.ser.spike.jumpT)
      << "\nSPIKE_LOOKBACK," << S.ser.spike.lookback;
    {   // today's spikes and letters: closed bars of the last closed bar's 17:00 CT session
        int nb = 0, na = 0, ls = 0, ll = 0; const long long day = sessionKey(S.lastClosedTime);
        if (day >= 0) for (std::map<long long, bas::Stat>::const_reverse_iterator r = S.ser.bars.rbegin(); r != S.ser.bars.rend(); ++r) {
            if (r->first > S.lastClosedTime) continue;
            if (sessionKey(r->first) != day) break;
            nb += r->second.bidSpike ? 1 : 0; na += r->second.askSpike ? 1 : 0;
            std::map<long long, BasFin>::const_iterator f = S.fin.find(r->first);
            if (f != S.fin.end()) for (int s = 0; s < 2; ++s) { ls += f->second.let[s].find('S') != std::string::npos; ll += f->second.let[s].find('L') != std::string::npos; }
        }
        o << "\nSPIKES_TODAY_BID," << nb << "\nSPIKES_TODAY_ASK," << na << "\nLETTERS_TODAY,S " << ls << ",L " << ll;
    }
    o << "\nLAST_CLOSED_BAR," << S.lastClosedTime << "," << stamp(S.lastClosedTime);
    o.precision(10);
    if (ok) o << "\nBIGGEST_BID," << st->m.bid << "," << st->m.bidPx << "\nBIGGEST_ASK," << st->m.ask << "," << st->m.askPx;
    else o << "\nBIGGEST_BID,n/a\nBIGGEST_ASK,n/a";
    const std::vector<bas::Line> L = bas::boxLines(st);
    o << "\nBBID," << L[0].value << "\nBASK," << L[1].value << "\nBID_RATIO," << L[2].value << "\nASK_RATIO," << L[3].value << "\nJUMP," << L[4].value;
    {
        std::map<long long, BasFin>::const_iterator f = S.fin.find(S.lastClosedTime);
        if (f != S.fin.end()) o << "\nMESSAGE," << off::implText(f->second.msg.impl) << "," << f->second.msg.line1 << "\nNUMBERS," << f->second.msg.line2 << "\nLETTERS," << f->second.let[0] << "," << f->second.let[1];
    }
    const off::Engine& e = S.e; const off::Conn& c = S.feed;
    o << "\nBARS," << S.ser.bars.size() << "\nVAP_READS," << S.vapReads << "\nVAP_FAILURES," << S.vapFailures
      << "\nVAP_NA_RETRYING," << S.naTries.size() << "\nVAP_MISMATCH," << S.vapMismatch
      << "\nMBO,subscribed " << (c.flag < 0 ? "unknown" : c.flag ? "1" : "0") << ",opened_with " << (c.via.empty() ? "not opened" : c.via) << ",state " << c.stateText(now)
      << "\nMBO_EVENTS,per_min " << c.roll.perMin(now) << ",total " << e.events << ",new " << e.perAction[0] << ",mod " << e.perAction[1] << ",del " << e.perAction[2] << ",fill " << e.perAction[3] << ",garbage " << e.garbage
      << "\nMBO_READS,slices " << c.slices << ",cap_hits " << c.capHits << ",longest_backlog " << c.longestBacklog << ",faults " << c.faults
      << "\nMBO_DROPPED,late " << S.lateDropped << ",no_bar " << S.noBar << ",decided_with_backlog " << S.decidedLate << ",book " << e.book.size() << ",book_resets " << e.bookResets
      << "\nNEAR_TICKS," << e.near << "\nLEVELS," << S.levels.size() << "," << S.lv.note
      << "\nPARAMS_SOURCE," << S.params.load.source << "\nPARAMS," << off::paramsText(S.params.load.p);
    for (size_t i = 0; i < S.params.load.notes.size() && i < 20; ++i) o << "\nPARAMS_NOTE," << S.params.load.notes[i];
    o << "\nLARGE_AGGRESSOR_NOW," << off::largeThreshold(e.aggrDist, S.params.load.p.largeLotAggr, S.params.load.p.largeLotPct, S.params.load.p.largeLotMin) << " (" << e.aggrDist.total << " orders)"
      << "\nSESSION," << S.curSession << "\nSESSION_FILE,rows " << S.file.rows.size() << ",loaded " << S.file.loaded << ",rejected " << S.file.rejected << ",writes " << S.file.writes << ",fails " << S.file.fails << "\n";
    const std::string text = o.str(); if (text == S.lastStatusText) return;
    const std::string f = flexDir(); if (f.empty()) return;
    makeDir(f);
    S.statusMs = now;
    if (publishFile(f + "\\BidAskStats.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void BidAskStats::pump() {
    BASState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    BASState& S = *P; S.busy = true;
    struct Unbusy { BASState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        const long long now = steadyMs();
        settings(S, now);
        if (off::refreshParams(S.params, flexDir(), S.market, now)) {    // new candles only: a closed candle's colour is final
            S.e.refillMs = S.params.load.p.refillWindowMs; S.e.closeTicks = S.params.load.p.closeTicks;
            S.ser.spike = effSpike(S.dlgSpike, S.params.load.p);
        }
        const long count = getBarCount();
        if (S.tick <= 0) S.state = "native tick size unavailable";
        else if (!S.faulted) {
            S.state = count < 2 ? "waiting for bars" : "ready";
            if (count >= 2) {
                orderBars(S, count, now);
                S.levels = nl::levels(S.lv, flexDir().empty() ? std::string() : flexDir() + "\\", S.market, S.symbol, now, (long long)std::time(NULL));
                loadSession(S);
                std::string st;
                S.feed.open(S.symbol, dir().empty() ? std::string() : dir() + "\\_opening-" + stem(S) + ".txt", now, st);
                const double tick = S.tick;
                S.feed.drain(now, steadyMs, [&](const DEPTH_ORDER& o) { route(S, off::evOf(o, tick, now)); });
                if (S.feed.lastEventMs >= 0 && S.firstBarKey < 0) S.firstBarKey = S.formingKey;
                if (!st.empty()) S.state = st;
            }
            closedBars(S, count, now);
            formingBar(S, count, now);
            if (count >= 2 && !S.faulted && !S.backlog) finalize(S, now);
        }
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
void BidAskStats::render(BASState& S) {
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 2 || pane.bottom - pane.top < 24 || pane.right - pane.left < 60) return;
    RTARRAYI times(barDateTime);
    if (times.count < count) return;
    const int last = (int)count - 1;
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    int v0 = 0, v1 = last;
    if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0 || v1 > last) {
        v1 = last; v0 = last;
        while (v0 > 0 && xOf(v0 - 1) >= pane.left) --v0;
    }
    int slot = (int)getPixelsPerBar();
    if (slot <= 0 || slot > 400) slot = v1 > v0 ? std::max(2, std::abs(xOf(v1) - xOf(v1 - 1))) : 8;
    auto maxOfBar = [&](int i) -> const bas::BarMax* {
        const long long bt = (long long)times[i];
        if (i == last) return S.liveTime == bt && S.live.ok ? &S.live : nullptr;
        std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(bt);
        return it != S.ser.bars.end() && it->second.m.ok ? &it->second.m : nullptr;
    };
    long long top = 0;
    for (int i = v0; i <= v1; ++i) { const bas::BarMax* m = maxOfBar(i); if (m) top = std::max(top, std::max(m->bid, m->ask)); }
    if (S.axisTop > top) top = S.axisTop;                                  // draw on the top the value axis was given
    S.paneH = pane.bottom - pane.top;
    bas::Stat liveSt; if (S.live.ok && S.liveTime == (long long)times[last]) liveSt = S.ser.peek(S.liveTime, S.live, S.ratioBars);
    auto statOfBar = [&](int i) -> const bas::Stat* {
        if (i == last) return liveSt.m.ok ? &liveSt : nullptr;
        std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find((long long)times[i]);
        return it != S.ser.bars.end() && it->second.m.ok ? &it->second : nullptr;
    };
    const int hov = S.hover.bar >= 0 && S.hover.bar < last ? S.hover.bar : -1;   // a bar index past the data (bars trimmed) = Live
    const off::PaneGeo g = off::paneGeo(this, S.pos);
    // (12:05) the info box (the 1.2.0 box + Sweep + Large) and the message line, both for the hovered closed candle (as of that
    // candle), else the last closed candle (header "Live")
    const int mb = hov >= 0 ? hov : last - 1;
    const long long mt = (long long)times[mb];
    std::map<long long, BasFin>::const_iterator f = S.fin.find(mt);
    const bas::Stat* bst = NULL;
    if (hov >= 0) { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(mt); if (it != S.ser.bars.end()) bst = &it->second; }
    else { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(S.lastClosedTime); if (it != S.ser.bars.end() && S.lastClosedTime == mt) bst = &it->second; }
    off::Info info; info.twoCol = false; info.tmpl = "99 lvls sellers";
    {
        struct tm x; std::memset(&x, 0, sizeof(x));
        info.head = hov < 0 ? std::string("Live") : (mt > 0 && getLocaltime((RTDATE)mt, &x) ? bas::clock12(x.tm_hour, x.tm_min) : std::string("n/a"));
        const std::string none[2];
        const std::vector<bas::Line> L = bas::infoLines(bst, f != S.fin.end() ? &f->second.r : nullptr, f != S.fin.end() ? f->second.let : none, f != S.fin.end() && f->second.dbo);
        for (size_t k = 0; k < L.size(); ++k) { off::InfoRow r; r.label = L[k].label; r.v[0] = L[k].value; r.side[0] = L[k].side < 0 ? 0 : L[k].side > 0 ? 1 : -1; info.rows.push_back(r); }
    }
    const off::Box ib = off::infoRect(this, g, S.pos, info);
    const std::vector<off::LBox> avoid(1, off::LBox{ ib.l, ib.t, ib.r, ib.b, 0, 0 });
    const COLOR normal[2] = { C_BID_BAR, C_ASK_BAR }, spike[2] = { C_BID_SPIKE, C_ASK_SPIKE };   // left = bids hit (red), right = offers lifted (green)
    off::drawBars(this, g, v0, v1, slot, top, hov, normal, spike, xOf, [&](int i) {
        off::BarLook k; const bas::BarMax* m = maxOfBar(i); if (!m) return k;
        const bas::Stat* sp = statOfBar(i);
        k.ok = true; k.v[0] = m->bid; k.v[1] = m->ask; k.spike[0] = sp && sp->bidSpike; k.spike[1] = sp && sp->askSpike;
        if (sp) { k.ratioOk[0] = sp->bidRatioOk; k.ratio[0] = sp->bidRatio; k.ratioOk[1] = sp->askRatioOk; k.ratio[1] = sp->askRatio; }   // (12:03) spike labels
        if (i != last) { std::map<long long, BasFin>::const_iterator ff = S.fin.find((long long)times[i]); if (ff != S.fin.end()) { k.let[0] = ff->second.let[0]; k.let[1] = ff->second.let[1]; } }
        return k;
    }, avoid);
    const COLOR vcol[2] = { C_BID, C_ASK };                                // values: sellers (bid side) red, buyers (ask side) green, labels white
    off::drawInfo(this, ib, info, vcol);
    nl::Msg m;
    if (f != S.fin.end()) m = f->second.msg;
    else if (S.ser.bars.count(mt)) m = provisional(S, mt).msg;
    else m.line1 = "No volume data for this candle";
    if (S.faulted || S.tick <= 0 || S.state == "loading bars") m.line1 = S.state;
    off::drawMessage(this, g, &ib, off::line1Of(S.faulted || S.tick <= 0 || S.state == "loading bars" ? std::string() : ctOf(mt), m.line1), m.impl);
}

extern "C" cppExtension* CreateExtension(void) {
    BidAskStats* p = new BidAskStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);       // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                                // (1.3.0) the order feed is drained while no trade prints
    p->setDescription("LRA BidAskStats: who is ATTACKING. Two bars per candle - red = the biggest bid (most contracts sold into the bid at one price), green = the biggest ask (most bought at the ask at one price). Bright = a spike: >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bid or ask of the prior Spike lookback candles. Letters from the order-by-order feed: S = one order swept 3+ price levels, L = one large aggressive order. Spike labels: the ratio over the quantity. Top: a plain-English message (green = bullish, red = bearish) and the info box with every value, for the candle under the mouse (else the last closed candle). Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off).");
    p->setVersion("1.3.0");   // BAS_VERSION
    return p;
}
