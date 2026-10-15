/********************************************************************************
 *  BidAskStats.cpp  --  Investor/RT RTX extension  lsBidAskStats  1.7.0 (2026-10-14 night)
 *  1.7.0 (2026-10-14 night, Rassul approved 22:37 CT, mockup-one-bar.png v2): ONE BAR PER CANDLE (OrderFlowFeed.h 1.6.0 drawBars
 *        one-bar mode, buyer side = 1 offers lifted): the bigger side's bar (red = sellers hitting the bid, green = buyers lifting the
 *        offer), its lots as the height; a tie = the side of the candle's close direction, else red. Bright = the winner is a spike (its
 *        label above, persistence unchanged), dim otherwise. A white tick across the bar at the smaller side's lots ONLY when BOTH sides
 *        spiked. S / L / R? letters on the single bar for the winner (the loser's stack in the same column only when both spiked; always in
 *        the hover / record). The pane title (parmsTitle) "Bid/Ask Stats - Who is Pressing v1.7.0"; "attacking" -> "pressing" in the
 *        message. Hover (both sides), read line, settings, records, axis: unchanged.
 *  1.6.0 (2026-10-14 night, Rassul via the coordinator "every order-flow pane gets its own ONE read line in place of its box";
 *        approval list 2026-10-14 s.7): the READ box is gone. Box shows: Read (default) = ONE line at the box's Position / Offset, only
 *        while price is inside a key-level or gamma zone (ZoneRead.h 1.0.0, OrderFlowFeed.h 1.5.0): who is hitting the level and where
 *        price is after the first 3 minutes - "ES 7750 - sellers hitting 3.1x normal, price held ?" / "... sellers hitting 3.1x, price
 *        through ?" - ratio vs the market's same-30-min-slot normal (LRA-ZoneReadNorms-<MKT>.csv), always "?", green / red by
 *        implication, locked when the window closes (BidAskStats\<stem>-zoneread-<session>.csv, read back on load). Mouse on the line =
 *        its detail (numbers, normal, what / why / how / example, the study caveat). Numbers mode keeps the numbers box with the line
 *        beside it and the options line reworded "Unsigned put volume 2.1x pace ? - buyer / seller unknown" (approval: MARK ?).
 *        The candle message ("... price held: buyers absorbing") is no longer drawn; it stays in the session record unchanged
 *        (the Tape Reader reads that record). Spikes, labels, letters, R? marks, settings: unchanged.
 *  1.5.3 (2026-10-14, CR5300; Rassul 11:59 CT CLZ6 3-min "the label for pace and lots on the red spike goes away but it is there on
 *        the green spike. fix it"): spike labels never lose their pace / lots because of another spike (OrderFlowFeed.h 1.3.1: labels
 *        decided in time order from older labels only, no shrink pass; the hover grid drawn over them, not re-laying them out;
 *        candles left of the view in the layout so a scroll keeps them). Seller and buyer labels behave the same.
 *  1.5.2 (2026-10-14, CR4800; Rassul 07:02-07:22): the left box is a READ box (the read, max 3 lines, + the blue oHit line; the numbers
 *        Lots ... Burst on its hover, in column groups) - "Box shows: Read / Numbers"; Offset X / Y on the 9-spot Position (clamped
 *        inside the pane, left of the last bar); letters white text with no box; one spike label per candle never over the read; L only
 *        for a rare large order (the candle's biggest vs the prior candles' biggest, OrderFlowFeed.h 1.3.0). Settings from the
 *        per-market file on every draw (IRT hands the chart copy list 0), the dialog filled from it when it opens.
 *  1.5.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md, mockup orderflow-contracts-mockup.png):
 *        bars unchanged (lots: the biggest bid / ask at one price). PACE = aggressive lots / s per side (the candle's VAP volume at the
 *        bid / at the ask / seconds per bar) vs the market's same-time-of-day norm (LRA-OrderFlow-<MKT>.csv PACE_NORM,AGGR, else the
 *        rolling norm), decided once at the close and recorded. Spike labels in 3 rows: "3.4x" / "p 2.3x" / "240". The SPEC message:
 *        "10:24 Sellers hit the bid with a 240-lot (3.4x) fast (p 2.3x), swept 3 levels into PDL - price held: buyers absorbing."
 *        (pace named only when notable: >= 2.0x "fast", <= 0.7x "slowly" on a spike; "?" until PACE_SHOW,on). Hover box in two
 *        columns Sellers | Buyers: Lots (ratio), Pace x (thr), Orders N · avg X lots, Top order NN%, Sweep, Large, StopRun, Burst.
 *        The blue oHit line under the message (LRA-Options-<MKT>.csv, OptionsFlow.h; only when present and fresh; "?" until OHIT_SHOW,on).
 *        Letters unchanged (S, L, R? stop run - 1.4.0). No outlines; sellers red, buyers green.
 *  1.4.0 (2026-10-13, Rassul 1:30 / 1:36 PM): STOP RUN (R). A bar breaks one of his key levels (ONH/ONL, PDH/PDL, PFH/PFL, LonHI/
 *        LonLO from LRA-KeyLevels-<MKT>.csv; highs approached from below, lows from above) with an S sweep or a spike on the break
 *        side in that bar, then price CLOSES back inside within N bars (LRA-StopRun-<MKT>.csv N, default 2). When the reclaim bar
 *        closes, a mark goes at the bottom of the break bar's attacking-side bar: failed upside break = red R (buyers trapped),
 *        failed downside break = green R (sellers trapped); "R?" until the file says SHOW,on (SHOW,off = logged, not drawn).
 *        White box, no border (as the S / L letters). Hover: StopRun (side + level + ticks beyond) and Burst (ratio / kind + bars to
 *        get back inside); message "Stop run above PDH - buyers trapped (...)". Every event -> an SR| line in the session record.
 *        Decided once, never repainted; only bars that closed while the indicator ran are judged. Rules: StopRun.h.
 *  1.3.1 (2026-10-13, Rassul 1:43 PM CT): no white outline on the histogram bars or the letter boxes - every bar / letter box
 *        is filled with a same-colour pen (OrderFlowFeed.h 1.0.1 fillFlat); the hovered candle is no longer outlined.
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
#include "StopRun.h"
#include "OptionsFlow.h"
#include "HoverBoard.h"
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

#define BAS_VERSION "1.7.3"          // keep equal to bas::VERSION and the setVersion literal below

static const COLOR C_ASK = 0x0022C55E, C_BID = 0x00EF4444, C_LABEL = 0x00FFFFFF, C_BORDER = 0x009CA3AF, C_BLACK = 0x00000000;
static const int WINDOW_BARS = 3000;          // closed bars read back from the last one (BBid is capped at the bars available)
static const int READS_PER_PASS = 600;        // bounded work per pass; the rest continues on the next pass
static const int NA_TRIES = 10;               // retries of a closed n/a bar (IRT back-fill)
static const size_t KEEP_BARS = 5000;
static const int RATIO_PARM = 0, POS_PARM = 1, SPIKE_AVG_PARM = 2, SPIKE_JUMP_PARM = 3, SPIKE_LB_PARM = 4, OFFX_PARM = 5, OFFY_PARM = 6, MODE_PARM = 7;   // (1.5.2) + 5-7
static const COLOR C_ASK_BAR = 0x001F9D55, C_BID_BAR = 0x00962D2D;     // normal histogram bars: darker green / red
static const COLOR C_ASK_SPIKE = 0x0000FF00, C_BID_SPIKE = 0x00FF3B3B; // spikes: bright green (buyers) / bright red (sellers)
static const COLOR C_SR_GREEN = 0x00168A42, C_SR_RED = 0x00E0242A;     // (1.4.0) the R mark's letter on its white box (readable on white)
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
#ifndef BAS_DEFAULT_MODE
#define BAS_DEFAULT_MODE off::MODE_READ   // (1.5.2) a 2 / 3 / 5-field file (1.5.1 and before) = Read; the 1.4 / 1.5 host suites build with Numbers
#endif
struct BasPanel { int dx = 0, dy = 0, mode = BAS_DEFAULT_MODE; };       // (1.5.2) fields 6-8
static bool parseSettings(const std::string& line, int& rb, int& pos, bas::SpikeCfg* spike = nullptr, BasPanel* panel = nullptr) {
    if (line.empty() || line.size() > 48) return false;
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = line.find(',', a); f.push_back(line.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    BasPanel bpn;
    if (f.size() == 8) {                                                   // (1.5.2) Offset X, Offset Y, Box shows
        if (!bp::intTok(f[5], bpn.dx) || !bp::intTok(f[6], bpn.dy) || !bp::offOk(bpn.dx) || !bp::offOk(bpn.dy) || (f[7] != "0" && f[7] != "1")) return false;
        bpn.mode = f[7] == "1" ? off::MODE_NUMBERS : off::MODE_READ; f.resize(5);
    }
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
    rb = r; pos = p; if (spike) *spike = c; if (panel) *panel = bpn; return true;
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
    nl::Msg msg; bas::PaceIn pc;                 // (1.5.0) the pace decided at the close (also in r.pace100 / paceNote / paceGate)
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
    int dx = 0, dy = 0, mode = BAS_DEFAULT_MODE;     // (1.5.2) Offset X / Y, Box shows
    bp::Box lastBox; bool inBox = false; long long boxHovers = 0;   // (1.5.2) the read box last drawn; the mouse over it
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
    // (1.4.0) the stop run: the detector (closed bars in order), its params file, the events by break bar
    sr::Detector det; sr::ParamCache srp; long long srFrom = -1, srCursor = 0;
    std::map<long long, std::vector<sr::Event> > srEv;   // break bar time -> events (at most one per direction)
    std::map<long long, long long> srReclaim;            // reclaim bar time -> break bar time (for its message / box)
    long long srEvents = 0, srLoaded = 0;
    off::OptCache opt;                                   // (1.5.0) LRA-Options-<MKT>.csv: the blue oHit line
    zread::Host zr; long zrCount = -1; long long zrSession = -1;   // (1.6.0) the zone read line
};
// (1.6.0, approval list s.7 MARK ?) the options volume is unsigned: who bought / sold and opening / closing are unknown
static std::string unsignedHitLine(const ofo::Snap& s, int gate) {
    if (!s.ok || !s.hit.ok || gate == ofo::GATE_OFF) return std::string();
    char b[32]; std::snprintf(b, sizeof(b), "%.1fx", s.hit.ratio);
    return std::string("Unsigned ") + (s.hit.type == 'P' ? "put" : "call") + " volume " + b + " pace ? - buyer / seller unknown";
}

class BidAskStats : public cppExtension {
public:
    BidAskStats() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoad(void);                                         // (1.5.2) fills the dialog from the file; loading never saves (lesson 29)
    bp::DlgSnap snap_;                                                   // (1.5.2) the dialog as it opened
    virtual int parmsApply(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { saveSettings(); return RTX_OK; }
    virtual int mouse(RTX_EVENT* e);
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax);
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, bas::TITLE, BAS_VERSION) ? RTX_OK : RTX_FAIL; }   // (1.7.0) the pane title, version next to it
    bool mouseLeft(BASState& S);
    bool visRange(unsigned long long& t0, unsigned long long& t1);   // (1.5.1) HoverBoard.h: the chart-wide hover
    bool followBoard(BASState& S);
    void publishBoard(BASState& S, long bar);
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
    void stopRuns(BASState& S);
    bas::PaceIn paceAt(BASState& S, long long t, const bas::Stat& st);
    int minuteOf(long long t);
    long long clockSec(long long t);
    long long unixOf(long long t);
    void addEvent(BASState& S, const sr::Event& e, bool fromFile);
    const sr::Event* eventAt(BASState& S, long long t);
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
    // (1.5.2, CR4800) + Offset X / Y and "Box shows" (appended); version 4 = IRT resets the dialog rows once - harmless: the chart reads
    // the per-market file and the dialog is filled from it when it opens
    setParameterVersion(4);
    setParameterDialogHeight(3);
    setIntegerParameter("Ratio bars", bas::RATIO_DEFAULT, 50);
    setListParameter("Position", (short)bas::POS_DEFAULT, bas::POS_CHOICES, 110, kParmAppendSameLine);
    setFloatParameter("Spike vs average", (float)bas::SPIKE_AVG_DEFAULT_TENTHS / 10.0f, 50);
    setFloatParameter("Spike vs prior biggest", (float)bas::SPIKE_JUMP_DEFAULT_TENTHS / 10.0f, 50, kParmAppendSameLine);
    setIntegerParameter("Spike lookback", bas::SPIKE_LOOKBACK_DEFAULT, 50, kParmAppendSameLine);
    setIntegerParameter("Offset X", 0, 50);                                // (1.5.2) parameter 5
    setIntegerParameter("Offset Y", 0, 50, kParmAppendSameLine);           // 6
    setListParameter("Box shows", (short)off::MODE_READ, off::MODE_CHOICES, 90, kParmAppendSameLine);   // 7
    return RTX_OK;
}
int cppExtension::calc(int) {
    BidAskStats* x = static_cast<BidAskStats*>(this);
    x->pump();
    BASState* S = x->testState();
    if (S) {
        bool ch = x->mouseLeft(*S);                                        // the mouse left the chart: back to Live
        if (x->followBoard(*S)) ch = true;                                 // (1.5.1) the mouse moved over another pane of this chart
        if (ch) { ++S->redraws; x->invalidateChart(false); }               // only on a change
    }
    return RTX_OK;
}
int BidAskStats::draw(void) {
    pump();
    BASState* S = slot_.get(this, false);
    if (S) { mouseLeft(*S); followBoard(*S); }                            // (1.5.1) the candle under the mouse ANYWHERE on this chart
    if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
bool BidAskStats::visRange(unsigned long long& t0, unsigned long long& t1) {
    t0 = t1 = 0; int a = 0, b = 0; const long n = getBarCount();
    if (n < 1 || getVisibleBars(&a, &b) != RTX_OK) return false;
    RTARRAYI times(barDateTime); if ((long)times.count < n) return false;
    if (a < 0) a = 0;
    if (b > (int)n - 1) b = (int)n - 1;
    if (b < a) return false;
    t0 = (unsigned long long)times[a]; t1 = (unsigned long long)times[b]; return t0 > 0;
}
bool BidAskStats::followBoard(BASState& S) {
    unsigned long long t0 = 0, t1 = 0, t = 0; if (S.symbol.empty() || !visRange(t0, t1) || !hb::hovered(S.symbol, S.spb, t0, t1, t)) return false;
    RTARRAYI times(barDateTime); const long n = getBarCount(); if ((long)times.count < n) return false;
    return S.hover.set(hb::indexOf(times, n, t));
}
void BidAskStats::publishBoard(BASState& S, long bar) {
    unsigned long long t0 = 0, t1 = 0; if (S.symbol.empty() || !visRange(t0, t1)) return;
    RTARRAYI times(barDateTime); const long n = getBarCount(); if ((long)times.count < n) return;
    hb::publish(S.symbol, S.spb, bar >= 0 && bar < n - 1 ? (unsigned long long)times[(int)bar] : 0ULL, t0, t1);
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
    S.det = sr::Detector(); S.srFrom = -1; S.srCursor = 0; S.srEv.clear(); S.srReclaim.clear(); S.srp = sr::ParamCache();
    S.feed.reset(); S.e = off::Engine(); S.scratch = off::Acc(); S.zone = nl::zoneFor(S.market); S.atr = 0; S.atrCount = -1;
    S.accs.clear(); S.closedAt.clear(); S.barEnds.clear(); S.formingKey = 0; S.firstBarKey = -1; S.finFrom = 0; S.fin.clear();
    S.file = off::SessionFile(); S.curSession.clear(); S.loaded = false; S.params = off::ParamCache(); S.lastStatusText.clear();
    S.opt = off::OptCache();
    S.zr = zread::Host(); S.zrCount = -1; S.zrSession = -1;               // (1.6.0)
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
    {   // (1.5.2) over the read box: its numbers show (hover) and the candle it reads stays
        const bool in = S->mode == off::MODE_READ && off::inBox(S->lastBox, e->v.mouse.h, e->v.mouse.v);
        if (in != S->inBox) { S->inBox = in; if (in) ++S->boxHovers; invalidateChart(false); }
        if (in) return RTX_FAIL;
    }
    publishBoard(*S, e->v.mouse.bar);                                    // (1.5.1) every pane of this chart follows this candle
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
        off::axisFor(top, S->paneH, off::drawPx(S->paneH, off::textReserve(S->paneH, S->mode)), mn, mx);
    }
    if (dMin) *dMin = mn;
    if (dMax) *dMax = mx;
    return RTX_OK;
}
bool BidAskStats::mouseLeft(BASState& S) {
    if ((S.hover.bar < 0 && !S.inBox) || S.hoverWnds.empty()) return false;
    void* w = cursorWindow(); if (!w) return false;
    if (std::find(S.hoverWnds.begin(), S.hoverWnds.end(), w) != S.hoverWnds.end()) return false;
    const bool wasIn = S.inBox; S.inBox = false;                          // (1.5.2) off the chart: no box hover either
    return S.hover.set(-1) || wasIn;
}
// the per-market settings file, re-read only when its stamp changes (checked at most once a second)
void BidAskStats::settings(BASState& S, long long now) {
    if (S.settingsCheckMs >= 0 && now >= S.settingsCheckMs && now - S.settingsCheckMs < 1000) return;
    S.settingsCheckMs = now;
    const std::string p = placePath(S.market);
    const long long st = p.empty() ? -1 : fileStamp(p);
    if (st == S.settingsStamp) return;
    S.settingsStamp = st;
    int rb = bas::RATIO_DEFAULT, pos = bas::POS_DEFAULT; bas::SpikeCfg sc; BasPanel pn;
    if (st >= 0) {
        std::ifstream f(p.c_str(), std::ios::binary); std::string ln;
        if (f && std::getline(f, ln)) {
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            int r = 0, q = 0; bas::SpikeCfg c; BasPanel b2; if (parseSettings(ln, r, q, &c, &b2)) { rb = r; pos = q; sc = c; pn = b2; }
        }
    }
    if (pos != S.pos || pn.dx != S.dx || pn.dy != S.dy || pn.mode != S.mode) { S.lastBox = bp::Box(); S.inBox = false; }   // (1.5.2) moved: no stale hover rect
    S.pos = pos; S.dlgSpike = sc; S.dx = pn.dx; S.dy = pn.dy; S.mode = pn.mode;
    const bas::SpikeCfg eff = effSpike(sc, S.params.load.p);
    const bool reratioed = rb != S.ratioBars;
    if (reratioed) { S.ratioBars = rb; S.ser.reratio(rb); }
    if (reratioed || eff != S.ser.spike) { S.ser.respike(eff); reMessage(S); }   // only a settings change re-decides closed bars
}
// (1.5.2) the chart's market and its saved settings (defaults when there is no file / it is malformed)
static std::string basMarket(cppExtension* x) { char b[32]; std::memset(b, 0, sizeof(b)); const char* rs = x->getRootSymbol(b); const char* sy = x->getSymbol(); const std::string root = rs ? rs : ""; return marketName(root.empty() ? (sy ? sy : "") : root); }
static bool basInDialog(cppExtension* x) { const char* pl = x->getPeriodicityLabel(); return !(pl && pl[0]); }   // the settings window's copy has no bar size
struct BasFile { int rb = bas::RATIO_DEFAULT, pos = bas::POS_DEFAULT; bas::SpikeCfg sc; BasPanel pn; };
static BasFile basFile(const std::string& p) {
    BasFile F; if (p.empty()) return F;
    std::ifstream f(p.c_str(), std::ios::binary); std::string ln;
    if (f && std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); int r = 0, q = 0; bas::SpikeCfg c; BasPanel b2; if (parseSettings(ln, r, q, &c, &b2)) { F.rb = r; F.pos = q; F.sc = c; F.pn = b2; } }
    return F;
}
static std::vector<long long> basDialog(cppExtension* x) {
    std::vector<long long> v;
    v.push_back(x->getIntegerValue(RATIO_PARM)); v.push_back(x->getListIndex(POS_PARM)); v.push_back(bas::tenthsOf(x->getFloatValue(SPIKE_AVG_PARM)));
    v.push_back(bas::tenthsOf(x->getFloatValue(SPIKE_JUMP_PARM))); v.push_back(x->getIntegerValue(SPIKE_LB_PARM));
    v.push_back(x->getIntegerValue(OFFX_PARM)); v.push_back(x->getIntegerValue(OFFY_PARM)); v.push_back(x->getListIndex(MODE_PARM));
    return v;
}
int BidAskStats::parmsLoad(void) {
    if (!basInDialog(this)) return RTX_OK;
    const BasFile F = basFile(placePath(basMarket(this)));
    setIntegerValue(RATIO_PARM, F.rb); setListIndex(POS_PARM, F.pos); setFloatValue(SPIKE_AVG_PARM, (float)F.sc.avgT / 10.0f); setFloatValue(SPIKE_JUMP_PARM, (float)F.sc.jumpT / 10.0f);
    setIntegerValue(SPIKE_LB_PARM, F.sc.lookback); setIntegerValue(OFFX_PARM, F.pn.dx); setIntegerValue(OFFY_PARM, F.pn.dy); setListIndex(MODE_PARM, F.pn.mode);
    snap_.take(basDialog(this));
    return RTX_OK;
}
void BidAskStats::saveSettings() {
    if (!basInDialog(this)) return;                                       // (1.5.2) only the settings window's values are real
    const std::string mkt = basMarket(this);
    const std::string p = placePath(mkt); if (p.empty()) return;
    const BasFile F = basFile(p);
    const std::vector<long long> now = basDialog(this);
    const long long fv[8] = { F.rb, F.pos, F.sc.avgT, F.sc.jumpT, F.sc.lookback, F.pn.dx, F.pn.dy, F.pn.mode };
    long long v[8]; for (size_t k = 0; k < 8; ++k) v[k] = snap_.pick(k, now[k], fv[k]);   // a control he did not touch keeps the file's value
    const int rb = (int)v[0], pos = bp::posOk((int)v[1]) ? (int)v[1] : F.pos, sa = (int)v[2], sj = (int)v[3], sl = (int)v[4];
    const int dx = bp::offOk((int)v[5]) && v[5] == (int)v[5] ? (int)v[5] : F.pn.dx, dy = bp::offOk((int)v[6]) && v[6] == (int)v[6] ? (int)v[6] : F.pn.dy;
    const int mode = v[7] == off::MODE_READ || v[7] == off::MODE_NUMBERS ? (int)v[7] : F.pn.mode;
    if (bas::clampRatioBars(rb) < 0) return;
    if (bas::clampLineTenths(sa) < 0 || bas::clampLineTenths(sj) < 0 || bas::clampLookback(sl) < 0) return;   // never overwrite a saved choice
    makeDir(flexDir());
    publishFile(p, std::to_string(rb) + "," + std::to_string(pos) + "," + std::to_string(sa) + "," + std::to_string(sj) + "," + std::to_string(sl)
                   + "," + std::to_string(dx) + "," + std::to_string(dy) + "," + std::to_string(mode) + "\n");
    snap_.take(now);
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
// (1.5.0) the CT minute of the day of a bar stamp; its unix time from the wall clock and the chart's clock (no time-zone assumption)
int BidAskStats::minuteOf(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return -1; return x.tm_hour * 60 + x.tm_min; }
long long BidAskStats::clockSec(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return LLONG_MIN; return off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec; }
long long BidAskStats::unixOf(long long t) { const long long a = clockSec((long long)currentDate()), b = clockSec(t); if (a == LLONG_MIN || b == LLONG_MIN) return 0; return (long long)std::time(NULL) - (a - b); }
// (1.5.0) a closed candle's PACE per side: its aggressive lots / s (VAP volume at the bid / at the ask over the bar's seconds) vs the
// PACE_NORM,AGGR row of its 30-min bucket, else the rolling norm of the prior <= 60 candles of the last 4 h (needs 5)
bas::PaceIn BidAskStats::paceAt(BASState& S, long long t, const bas::Stat& st) {
    bas::PaceIn pc; const off::Params& P = S.params.load.p; pc.gate = P.gate[ofo::G_PACE];
    if (!st.m.ok || S.spb <= 0) return pc;
    std::vector<double> prior;
    std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.lower_bound(t);
    while (it != S.ser.bars.begin() && prior.size() < (size_t)off::PACE_ROLL_MAX) {
        --it; if (t - it->first > off::PACE_ROLL_SPAN_S) break;
        if (it->second.m.ok) prior.push_back(((double)it->second.m.totBid + (double)it->second.m.totAsk) / 2.0 / (double)S.spb);
    }
    std::reverse(prior.begin(), prior.end());
    const double rn = off::rollingNorm(prior), fn = off::fileNorm(P, off::PM_AGGR, minuteOf(t));
    const long long tot[2] = { st.m.totBid, st.m.totAsk }; const bool spk[2] = { st.bidSpike, st.askSpike };
    for (int k = 0; k < 2; ++k) { const off::Pace p = off::paceOf((double)tot[k] / (double)S.spb, fn, rn, P, spk[k]); pc.p100[k] = off::pace100(p); pc.note[k] = p.note; }
    return pc;
}
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
        if (!S.curSession.empty()) { S.file.save(); S.e.newSession(); S.det = sr::Detector(); S.srFrom = -1; }
        S.curSession = ss; S.loaded = false;
    }
}
void BidAskStats::route(BASState& S, const off::Ev& x) {
    S.zr.e.onEvent(x.arrMs, x.action, x.side, x.id, x.px, x.size);       // (1.6.0) every event, in order, to the zone read
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
    S.file.extraTag = "SR|"; S.file.extraHead = sr::SR_HEAD;     // (1.4.0) stop-run events kept after the candle rows
    S.file.load();
    S.file.loadSizes(S.e.aggrDist, S.e.restDist);
    for (size_t i = 0; i < S.file.extra.size(); ++i) { sr::Event e; if (sr::parseEvent(S.file.extra[i], e)) { addEvent(S, e, true); S.srLoaded++; } }
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
            if (F.dbo) S.e.barFrom(rr);                                   // (1.5.2) the rare-L history after a restart
            for (int k = 0; k < 2; ++k) { F.pc.p100[k] = rr.pace100[k]; F.pc.note[k] = rr.paceNote[k]; } F.pc.gate = rr.paceGate;   // (1.5.0) as decided then
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
            F.pc = paceAt(S, t, it->second);                                // (1.5.0) decided once, recorded
            for (int k = 0; k < 2; ++k) { F.r.pace100[k] = F.pc.p100[k]; F.r.paceNote[k] = F.pc.note[k]; } F.r.paceGate = F.pc.gate;
            F.msg = bas::message(it->second, F.dbo ? &F.r : nullptr, F.let, F.level, F.held, F.pc);
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
// (1.4.0) THE STOP RUN: every decided candle that closed while the indicator ran, strictly in time order, once (StopRun.h)
void BidAskStats::addEvent(BASState& S, const sr::Event& e, bool fromFile) {
    std::vector<sr::Event>& v = S.srEv[e.breakT];
    for (size_t i = 0; i < v.size(); ++i) if (v[i].dir == e.dir) return;                 // decided once: the first one stays
    v.push_back(e); S.srReclaim[e.reclaimT] = e.breakT;
    if (!fromFile) { S.srEvents++; S.file.putExtra(sr::eventLine(e, ctOf(e.breakT))); }
    while (S.srEv.size() > KEEP_BARS) S.srEv.erase(S.srEv.begin());
    while (S.srReclaim.size() > KEEP_BARS) S.srReclaim.erase(S.srReclaim.begin());
}
const sr::Event* BidAskStats::eventAt(BASState& S, long long t) {
    std::map<long long, std::vector<sr::Event> >::const_iterator it = S.srEv.find(t);
    if (it == S.srEv.end()) { std::map<long long, long long>::const_iterator r = S.srReclaim.find(t); if (r != S.srReclaim.end()) it = S.srEv.find(r->second); }
    if (it == S.srEv.end()) return nullptr;
    const sr::Event* best = nullptr;                                                    // a shown one first (both directions: the first)
    for (size_t i = 0; i < it->second.size(); ++i) if (it->second[i].show != sr::SHOW_OFF && (it->second[i].breakT == t || it->second[i].reclaimT == t)) { best = &it->second[i]; break; }
    return best;
}
void BidAskStats::stopRuns(BASState& S) {
    if (S.srFrom <= 0 || S.tick <= 0) return;
    RTARRAY high(barHigh), low(barLow), close(barClose);
    const std::vector<sr::KeyLevel> lv = sr::keyLevels(S.lv, (long long)std::time(NULL));
    bool any = false;
    for (std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.upper_bound(std::max(S.srCursor, S.srFrom - 1)); it != S.ser.bars.end(); ++it) {
        const long long t = it->first;
        if (S.formingKey > 0 && t >= S.formingKey) break;
        std::map<long long, BasFin>::const_iterator f = S.fin.find(t);
        if (f == S.fin.end()) break;                                                    // in order: wait for the first undecided candle
        S.srCursor = t;
        if (sessionStr(t) != S.curSession) continue;
        std::map<long long, int>::const_iterator bi = S.barEnds.find(t);
        if (bi == S.barEnds.end()) continue;
        const int k = bi->second;
        if (k < 1 || high.count <= k || low.count <= k || close.count <= k) continue;
        sr::Bar b; b.t = t; b.h = (double)high[k]; b.l = (double)low[k]; b.c = (double)close[k];
        const bas::Stat& st = it->second;
        b.spike[0] = st.m.ok && st.bidSpike; b.spike[1] = st.m.ok && st.askSpike;
        b.ratioOk[0] = st.bidRatioOk; b.ratio[0] = st.bidRatio; b.ratioOk[1] = st.askRatioOk; b.ratio[1] = st.askRatio;
        for (int s = 0; s < 2; ++s) b.sweep[s] = f->second.dbo && f->second.let[s].find('S') != std::string::npos;
        const std::vector<sr::Event> ev = S.det.onClose(b, true, (double)close[k - 1], lv, S.tick, S.srp.p);
        for (size_t i = 0; i < ev.size(); ++i) { addEvent(S, ev[i], false); any = true; }
    }
    if (any) { S.file.save(); invalidateChart(false); }
}
// a settings change re-decides the messages of every decided candle (letters and levels kept)
void BidAskStats::reMessage(BASState& S) {
    for (std::map<long long, BasFin>::iterator it = S.fin.begin(); it != S.fin.end(); ++it) {
        std::map<long long, bas::Stat>::const_iterator st = S.ser.bars.find(it->first);
        if (st == S.ser.bars.end()) continue;
        it->second.msg = bas::message(st->second, it->second.dbo ? &it->second.r : nullptr, it->second.let, it->second.level, it->second.held, it->second.pc);
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
      << "\nSTOPRUN,file " << S.srp.p.source << (S.srp.p.note.empty() ? std::string() : " (" + S.srp.p.note + ")") << ",N " << S.srp.p.n << ",SHOW " << sr::showText(S.srp.p.show)
      << ",key_levels " << sr::keyLevels(S.lv, (long long)std::time(NULL)).size() << ",judged_from " << S.srFrom << ",watching " << S.det.pending.size()
      << ",breaks " << S.det.breaks << ",no_burst " << S.det.noBurst << ",held " << S.det.held << ",events " << S.srEvents << ",reloaded " << S.srLoaded
      << "\nOPTIONS," << S.opt.why << ",asof " << S.opt.last.asof << ",reads " << S.opt.reads << ",arrived " << S.opt.hist.v.size()
      << "\nPARAMS_SOURCE," << S.params.load.source << "\nPARAMS," << off::paramsText(S.params.load.p);
    for (size_t i = 0; i < S.params.load.notes.size() && i < 20; ++i) o << "\nPARAMS_NOTE," << S.params.load.notes[i];
    {   // (1.5.0) the last closed candle's pace
        std::map<long long, BasFin>::const_iterator f = S.fin.find(S.lastClosedTime);
        if (f != S.fin.end()) o << "\nPACE,bid " << off::paceHover(f->second.pc.p100[0], S.params.load.p.paceFastT, f->second.pc.gate) << ",ask " << off::paceHover(f->second.pc.p100[1], S.params.load.p.paceFastT, f->second.pc.gate)
                                << ",norm " << (off::fileNorm(S.params.load.p, off::PM_AGGR, minuteOf(S.lastClosedTime)) > 0 ? "file" : "rolling");
    }
    o << "\nLARGE_AGGRESSOR_NOW," << off::largeThreshold(e.aggrDist, S.params.load.p.largeLotAggr, S.params.load.p.largeLotPct, S.params.load.p.largeLotMin) << " (" << e.aggrDist.total << " orders)"
      << "\nSESSION," << S.curSession << "\nSESSION_FILE,rows " << S.file.rows.size() << ",loaded " << S.file.loaded << ",rejected " << S.file.rejected << ",writes " << S.file.writes << ",fails " << S.file.fails << "\n"
      << S.zr.status() << "\n";                                           // (1.6.0)
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
        if (S.formingKey > 0) off::refreshOptions(S.opt, flexDir(), S.market, now, [&](long long mt) {   // (1.5.0) arrival = the bar the file was written in
            const long long a = clockSec((long long)currentDate()); if (a == LLONG_MIN || mt <= 0) return S.formingKey;
            const long long target = a - ((long long)std::time(NULL) - mt); long long best = S.formingKey;
            for (std::map<long long, int>::const_reverse_iterator it = S.barEnds.rbegin(); it != S.barEnds.rend(); ++it) { const long long c = clockSec(it->first); if (c == LLONG_MIN || c < target) break; best = it->first; }
            return best; });
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
            if (count >= 2 && !S.faulted && !S.backlog) {
                sr::refresh(S.srp, flexDir().empty() || S.market.empty() ? std::string() : flexDir() + "\\LRA-StopRun-" + S.market + ".csv", now);
                if (S.srFrom < 0 && S.formingKey > 0) S.srFrom = S.formingKey;   // (1.4.0) bars closing from now on are judged
                finalize(S, now);
                stopRuns(S);
            }
            if (count >= 2 && !S.faulted) {                                // (1.6.0) the zone read line
                const std::string d = dir();
                off::zoneTickFor(this, S.zr, S.zrCount, S.zrSession, zread::P_BAS, S.tick, S.market, S.zone, S.atr,
                                 d.empty() || S.curSession.empty() ? std::string() : d + "\\" + stem(S) + "-zoneread-" + S.curSession + ".csv", count, now);
            }
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
    const off::PaneGeo g = off::paneGeo(this, S.mode);
    // (12:05) the info box (the 1.2.0 box + Sweep + Large) and the message line, both for the hovered closed candle (as of that
    // candle), else the last closed candle (header "Live")
    const int mb = hov >= 0 ? hov : last - 1;
    const long long mt = (long long)times[mb];
    std::map<long long, BasFin>::const_iterator f = S.fin.find(mt);
    const bas::Stat* bst = NULL;
    if (hov >= 0) { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(mt); if (it != S.ser.bars.end()) bst = &it->second; }
    else { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(S.lastClosedTime); if (it != S.ser.bars.end() && S.lastClosedTime == mt) bst = &it->second; }
    // (1.5.0) two columns: Sellers (bids hit, red) | Buyers (offers lifted, green)
    off::Info info; info.twoCol = true; info.colHead[0] = "Sellers"; info.colHead[1] = "Buyers"; info.tmpl = off::CROWD_TMPL;   // (1.5.1) the Crowd row is the widest
    {
        struct tm x; std::memset(&x, 0, sizeof(x));
        info.head = hov < 0 ? std::string("Live") : (mt > 0 && getLocaltime((RTDATE)mt, &x) ? bas::clock12(x.tm_hour, x.tm_min) : std::string("n/a"));
        const std::string none[2]; const bas::PaceIn nopc;
        const std::vector<bas::Row2> L = bas::infoRows(bst, f != S.fin.end() ? &f->second.r : nullptr, f != S.fin.end() ? f->second.let : none, f != S.fin.end() && f->second.dbo,
                                                       f != S.fin.end() ? f->second.pc : nopc, S.params.load.p.paceFastT);
        for (size_t k = 0; k < L.size(); ++k) { off::InfoRow r; r.label = L[k].label; r.v[0] = L[k].v[0]; r.v[1] = L[k].v[1]; info.rows.push_back(r); }
        // (1.4.0) StopRun: "above PDH 6t" (red = buyers trapped, in the Buyers column) / "below LonLO 4t" (green, Sellers column); Burst
        const sr::Event* e = eventAt(S, mt);
        off::InfoRow a; a.label = "StopRun"; off::InfoRow b; b.label = "Burst";
        a.v[0] = a.v[1] = "-"; b.v[0] = b.v[1] = "-"; a.side[0] = a.side[1] = b.side[0] = b.side[1] = -1;
        if (e) {
            const int c = e->dir > 0 ? 1 : 0;                              // the trapped side's column (buyers trapped above a high)
            a.v[c] = std::string(e->dir > 0 ? "above " : "below ") + e->level + " " + std::to_string(e->ticks) + "t" + (e->show == sr::SHOW_ON ? "" : "?");
            b.v[c] = (e->ratioOk ? nl::ratioText(e->ratio) : std::string(e->sweep ? "sweep" : "spike")) + ", " + (e->bars <= 0 ? std::string("same bar") : "in " + sr::barsText(e->bars));
            a.side[c] = b.side[c] = e->dir > 0 ? 0 : 1;                    // colour by implication: trapped buyers = red, trapped sellers = green
        }
        // (1.5.1) order: Lots, Pace, Sweep, Large, StopRun, Crowd, Burst (bas::infoRows ends with Crowd)
        info.rows.insert(info.rows.end() - (info.rows.empty() ? 0 : 1), a); info.rows.push_back(b);
    }
    // (1.5.2) the read and the blue oHit line first (the panel is laid out before the bars: labels never go over it)
    nl::Msg m;
    if (f != S.fin.end()) m = f->second.msg;
    else if (S.ser.bars.count(mt)) m = provisional(S, mt).msg;
    else m.line1 = "No volume data for this candle";
    { const sr::Event* e = eventAt(S, mt); if (e) { m.line1 = sr::message(*e); m.impl = e->dir > 0 ? nl::BEAR : nl::BULL; } }   // (1.4.0)
    if (S.faulted || S.tick <= 0 || S.state == "loading bars") m.line1 = S.state;
    const std::string l1 = off::line1Of(S.faulted || S.tick <= 0 || S.state == "loading bars" ? std::string() : ctOf(mt), m.line1);
    // (1.5.0) the blue oHit line: the snapshot that had ARRIVED by the candle read (hovered: fresh at its end; live: now)
    const ofo::Snap* os = S.opt.hist.latest(hov >= 0 ? mt : (long long)times[last]);
    const long long ref = hov >= 0 ? unixOf(mt) : (long long)std::time(NULL);
    const std::string optL = os && ofo::fresh(*os, ref) && !S.faulted ? unsignedHitLine(*os, S.params.load.p.gate[ofo::G_OHIT]) : std::string();
    bp::Place pl; pl.pos = S.pos; pl.dx = S.dx; pl.dy = S.dy;
    const bp::Box area = bp::mk(g.left + 3, g.top + 3, g.edge - 3, g.bottom - 3);
    // (1.6.0) Read = the ONE zone read line (only inside a key / gamma zone); Numbers = the numbers box + that line beside it
    const zread::Line zl = S.zr.e.line(zread::P_BAS, steadyMs());
    const bool readMode = off::clampMode(S.mode) == off::MODE_READ;
    off::PanelPlan P; off::LinePlan LP;
    if (readMode) { LP = off::planLine(area, pl, zl, S.inBox, off::measureWith(this), off::paneQuestion(bas::TITLE)); S.lastBox = LP.show ? LP.box : bp::Box(); if (!LP.show) S.inBox = false; }
    else { P = off::planPanel(area, g.top, S.mode, pl, info, zl.show ? zl.text : std::string(), optL, g.reserve, off::measureWith(this)); S.lastBox = P.ok ? P.box : bp::Box(); }
    (void)l1;
    std::vector<off::LBox> avoid;
    if (LP.show) avoid.push_back(off::LBox{ LP.box.l, LP.box.t, LP.box.r, LP.box.b, 0, 0 });
    if (P.ok) avoid.push_back(off::LBox{ P.box.l, P.box.t, P.box.r, P.box.b, 0, 0 });
    // (1.5.3) the hover grid is not an obstacle: it is drawn over the bars while the mouse is in the box (labels never re-decide for it)
    for (size_t k = 0; k < P.textRects.size(); ++k) avoid.push_back(off::LBox{ P.textRects[k].l, P.textRects[k].t, P.textRects[k].r, P.textRects[k].b, 0, 0 });
    const COLOR normal[2] = { C_BID_BAR, C_ASK_BAR }, spike[2] = { C_BID_SPIKE, C_ASK_SPIKE };   // left = bids hit (red), right = offers lifted (green)
    RTARRAY op(barOpen), cl(barClose);                                     // (1.7.0) the close direction (one-bar ties)
    off::drawBars(this, g, v0, v1, slot, top, hov, normal, spike, xOf, [&](int i) {
        off::BarLook k; const bas::BarMax* bm = maxOfBar(i); if (!bm) return k;
        const bas::Stat* sp = statOfBar(i);
        k.ok = true; k.v[0] = bm->bid; k.v[1] = bm->ask; k.spike[0] = sp && sp->bidSpike; k.spike[1] = sp && sp->askSpike;
        if (i < op.count && i < cl.count) { const float o = op[i], c = cl[i]; if (std::isfinite(o) && std::isfinite(c)) k.dir = c > o ? 1 : c < o ? -1 : 0; }
        if (sp) { k.ratioOk[0] = sp->bidRatioOk; k.ratio[0] = sp->bidRatio; k.ratioOk[1] = sp->askRatioOk; k.ratio[1] = sp->askRatio; }   // (12:03) spike labels
        if (i != last) { std::map<long long, BasFin>::const_iterator ff = S.fin.find((long long)times[i]); if (ff != S.fin.end()) { k.let[0] = ff->second.let[0]; k.let[1] = ff->second.let[1]; k.p100[0] = ff->second.pc.p100[0]; k.p100[1] = ff->second.pc.p100[1]; } }
        if (i != last) {                                                   // (1.4.0) the stop-run mark on the break bar's attacking side
            std::map<long long, std::vector<sr::Event> >::const_iterator se = S.srEv.find((long long)times[i]);
            if (se != S.srEv.end()) for (size_t q = 0; q < se->second.size(); ++q) {
                const sr::Event& e = se->second[q]; if (e.show == sr::SHOW_OFF) continue;
                k.mark[e.side()] = sr::markText(e); k.markCol[e.side()] = e.dir > 0 ? C_SR_RED : C_SR_GREEN;
            }
        }
        return k;
    }, avoid, 1, false);   // (1.7.1, Rassul 09:55 "get rid of the labeling like S L") no letters / R marks drawn; hover + record keep them                                                          // (1.7.0) one bar per candle; buyers = side 1 (offers lifted)
    const COLOR vcol[2] = { C_BID, C_ASK };                                // values: sellers (bid side) red, buyers (ask side) green, labels white
    if (readMode) off::drawLine(this, LP); else off::drawPanel(this, P, info, vcol, off::implOf(zl), false);
    if (hov >= 0) {   // (1.7.3, Rassul 13:25) the hovered candle's answer beside it (OrderFlowFeed 1.7.0 bar hover); buyers (offers lifted) first
        static const char* const verbs[2] = { "Sellers hit", "Buyers lifted" };
        off::drawBarHover(this, off::planBarHover(area, xOf(hov), info, "Who pressed?", verbs, 1, off::measureWith(this)));
    }
}

extern "C" cppExtension* CreateExtension(void) {
    BidAskStats* p = new BidAskStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);       // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                                // (1.3.0) the order feed is drained while no trade prints
    p->setDescription("LRA BidAskStats: who is PRESSING. One bar per candle = the bigger side - red = the biggest bid (most contracts sold into the bid at one price), green = the biggest ask (most bought at the ask at one price); a white tick at the other side's lots when both sides spiked. Bright = a spike: >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bid or ask of the prior Spike lookback candles. Letters from the order-by-order feed: S = one order swept 3+ price levels, L = one large aggressive order. R = stop run: a key level (ONH/ONL, PDH/PDL, PFH/PFL, LonHI/LonLO) broken with a sweep or spike that closed back inside within N bars - red R = buyers trapped above a high, green R = sellers trapped below a low; R? = not proven yet in this market (nightly stop-run study). Spike labels: the ratio, the pace (p = aggressive lots per second vs the same time of day) and the lots. Top: a plain-English message (pace named only when fast / slow), a blue options line when fresh (oHit), and (green = bullish, red = bearish), and the info box (Sellers | Buyers: lots, pace, sweep, large, stop run, crowd = orders, avg size, top order share, burst) for the candle under the mouse anywhere on the chart (else the last closed candle). Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off).");
    p->setVersion("1.7.3");   // BAS_VERSION
    return p;
}
