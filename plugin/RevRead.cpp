/*
 *  RevRead.cpp  --  Investor/RT RTX extension  lsRevRead  "Reversal Read"  0.11.1  (2026-11-16: ratio = bigger side / smaller; 0.11.0: 2026-11-16: the bars are Book Flow; 0.10.1: 2026-11-16: AccumLogic 2.0.1; 0.10.0: 2026-11-16: accumulation / distribution = the simple rule, AccumLogic 2.0.0; 0.9.2: 2026-11-16: bars in a low pane; 0.9.1: 2026-11-16: the accumulation read is always shown (the last zone), + the quiet end and the other side building, AccumLogic 1.2.0; 0.9.0: accumulation / distribution zones + their read, AccumLogic.h; 0.8.0: lsSR runs in the background, SRBack.h; 0.7.0 2026-11-13)
 *
 *  Rassul 2026-11-11 21:13-21:33 CT: "I want to know how i can have indicators that tell this story so i can understand how and why it
 *  reveresed"; "i have the poc of the candle in a pane. i want you to create a similar pane that has each candles poc and also makes a dash
 *  ont he candle. in the indicator pane you can place the read as well as mark the candles with letters like c for climax, whether it is a
 *  buying or selling climax"; "it must be added to the nightly refinement"; "jumpstart this indicator for all markets and refine"; "pay
 *  attention to session, volatility times, location and context"; "key levels, volume levels, gamma levels, vwap, hbo and its tests (like the
 *  testing line), whether the market has a pb or its variant ... the hourly rotation and hbo and whether its pulling back, testing or going
 *  higher with the rotation"; "pay attention to the location of the poc in the candle"; 21:33 "go".
 *
 *  lsRevRead (this file)       - its own pane: one bar per candle = the lots at that candle's POC (IRT's volume at price), the number on top
 *      (green = more bought at the offer there, red = more sold into the bid), the story letters over the bars (C climax red / green, A
 *      absorbed, E exhausted, I initiation; no "?" - his choice 22:19 CT) and the read line of the newest story at the top.
 *      The story comes from the market-by-order feed in 10-second blocks (RevReadLogic.h); the context from the chart itself: session,
 *      volatility window, the hourly steps (lsStepTrend's rule: rotation, the HBO test line, pulling back / testing / going with it), the key /
 *      MenthorQ / volume level files and the session VWAP, and lsBreakout's decided Pb labels (SignalBoard fact E).
 *      Records: lsFlexLevels\RevRead\<MKT>-<spb>-<session>.csv (every step, every measurement; the nightly lra.rev_read scores them).
 *      Thresholds: lsFlexLevels\LRA-RevRead-<MKT>.csv (written by the nightly; defaults when absent).
 *  lsRevReadMarks (RevReadMarks.cpp: #define RVS_MARKS 1 + #include this file) - the price-chart overlay: a white dash on every candle
 *      at its POC price.
 *      (0.4.2, Rassul 2026-11-12 19:20 "can you check the CL because it is not showing") the POC bars / dashes fill from the newest 6000 candles (was: from the
 *      chart's oldest candle, so the candles on screen came last), and a pass runs on while it has used under 10 ms.
 *      (0.4.1, Rassul 2026-11-12 16:33 "tell me about reversal read for hourly how do we get that done"; 16:37 "Ok for hourly reversal read") on a
 *      60-MINUTE (or longer) chart the hold dots are drawn by themselves - the read that was measured on hourly candles (3,709 hourly
 *      extremes, 42 sessions x 7 markets: the extreme held 3 hours 65% at the strong tier against 30% for any; the story letters did not
 *      help on the hourly, so the hourly chart gets the dots only). The checkbox still decides the shorter charts.
 *      (0.4.0, Rassul 2026-11-12 07:07 / 07:12 CT) + the HOLD READ dots, OFF by default (settings: "Hold read dots"): a small red dot over a
 *      high / a green dot under a low that is likely done (rvs::holdOf, the strong tier only: the candle closed well back from its extreme,
 *      on heavy volume / after a long run / back inside the prior high or low). Closed candles only: a dot never changes.
 *      (0.5.0, Rassul 2026-11-13 07:05 "ok build" / 07:09 "the climax waits for the confirmation .. climax on one bar and then after that a green bar occurs
 *      confirming the climax"; 08:14 "buying climax (bearish) should be near the top of a green candle and selling climax (bullish) should be near the
 *      bottom of a red candle and both cases should be confirmed by the next bar") the letters of a story appear only when its climax is in the top
 *      third of a green candle / the bottom third of a red one (CX_LOC) and the NEXT candle closes beyond the climax candle's close the story's way
 *      (CX_CONFIRM 1); I (initiation) is the signal - bold and bright; C / A / E are drawn plain and dim; stories that begin as a test of a level with
 *      no climax are recorded only (LEVEL_SHOW); "Stop run of <level>" in the read is now "Flush / Push through <level>" (the stop run is SR's).
 *      (0.7.0, Rassul 2026-11-13 22:05-22:23 CT "this will be our reversal framework for local pivots", "make sure you can identify all of this in
 *      realtime", "go , but you must allow for variability and discovery as well") THE SIX CHECKS (rvs::Six): Buying fading, Big buyer -> Buyer trapped,
 *      Quick rejection, Bids pulled, Big seller, With trend - computed live from the order feed for the newest pullback lsBreakout decided (fact E:
 *      side + the pivot extreme) and, once lsBreakout posts its Rev candles (fact W: lo = the Rev candle, hi = the pivot candle, raw bar stamps),
 *      decided once at the Rev candle's close, recorded ("R6|" lines: raw values + candidate reads) and posted back as fact X (bar = the Rev candle's
 *      raw stamp, code = count x 1000 + check bits + 64 trapped, lo / hi = the big buyer's / big seller's price, val = the prior candle's extreme) for
 *      the "Rev N" label and the BT / P / S marks. The read on the right = "Rev N" + six lines, a green check / red x each; a white ratio (3.5x) over
 *      the volume bar of the big buyer / big seller. Cut-offs: lsFlexLevels\LRA-RevSix-<MKT>.csv (defaults = Nov 9-11).
 *  lsRevRead has no settings; lsRevReadMarks has two switches (POC dash, Hold read dots). No hovers, no text on the price chart. Order-feed letters exist from the moment the pane is on the chart (IRT has no
 *  order-by-order history); decided letters are reloaded from the session record after a restart. The POC bars / dashes cover all bars.
 */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifndef LSPERF_PLUGIN
#ifdef RVS_MARKS
#define LSPERF_PLUGIN "RevReadMarks"
#else
#define LSPERF_PLUGIN "RevRead"
#endif
#endif
#include "PerfGuard.h"
#ifdef RVS_MARKS
#include "SettingsGuard.h"
#endif
#include "HostSlot.h"
#ifdef RVS_MARKS
#include "WyMarks.h"           // (0.18.0) the I on the candle (SDK-free, tests_wymarks)
#include "SettingsSpec.h"
#include "SettingsHost.h"
#include "RevReadSettings.h"
#include <chrono>
#endif
#include "MarketKey.h"
#include "NearLevels.h"
#include "SignalBoard.h"
#include "StepTrendLogic.h"
#include "VwapTrendLogic.h"
#include "RevReadLogic.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include "CombRead.h"          // (0.15.0) the one read + the Cx rule - both plugins use it
#include "WyRead.h"            // (0.17.0) the Wyckoff strip: E / A / Sp / Ut / I + their plain read
#include "LevelMarks.h"        // (0.19.0) Bk / Sw / Cx + one level each, for the price candles (SDK-free, tests_levelmarks)
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>

static const double BF_RATIO_MIN = 1.2;   // (0.11.1) a Book Flow ratio label needs the bigger side 1.2x the smaller or more
static const bool ACC_READ_DRAWN = false;   // (0.12.0, Rassul 2026-11-17 06:03 CT "remove what is there currently now") the Accum / Distrib read is not drawn in the pane (the boxes on the price chart stay; the read stays in the status file)
static const bool RV_DRAW_BOOKFLOW = false;   // (0.17.3, Rassul 09:29 CT "can we hide bookflow for now and just keep delta + change") the Book Flow half is not drawn - delta + change take the whole pane; Book Flow is still read in the background (the read, the Wyckoff springs)
static const bool STORY_TOP_LINE = false;   // (0.18.3, Rassul 2026-11-18 20:34 CT "there is a read in the signal strip for reversal that needs to be removed": when the story / Rev checks do not fit right of the last candle they are NOT written across the top (over the letter strip) any more
static const bool RV_SHOW_BOOKFLOW = true;   // (0.11.0) the pane's bars: true = Book Flow (resting orders added - pulled, bids / offers), false = the candle's buy / sell volume (0.6.0 - 0.10.x)
#define RVS_VERSION "0.19.4"   // (0.19.4, Rassul 2026-11-19 14:12-14:14 CT "I didnt mention a level", "I should not print if there is no level": an I is kept only when it reclaimed a NAMED key level (the Bk / Sw list: session, PD, PF, volume, weekly, hour levels) - no gamma / MenthorQ levels, no VWAP bands (they move: an I matched to the band's value now was a false I)   // was "0.19.3"   // (0.19.3, Rassul 2026-11-19 13:29 CT "its hard for me to see the white lines so lets make them black instead so bk and other badges will have black writing instead": the candle tags (I / Bk / Sw / Cx - code, level, strength) are written in BLACK on the bright green / red fill)   // was "0.19.2"   // (0.19.2, Rassul 2026-11-19 11:22-11:35 CT "sweeps of key levels on big volume or range are very meaningfull", "consider sw and a second line like v50 or r50, same with breaks", "use the max numbers that we have ... biggest in green bars, red bars, consecutive", "ok.. good build it", "will you add this strength line to the I also": Bk / Sw / I tags get a 3rd line = his BVMax / BRMax (the Volume / Range panes' Max number) "V<N>" / "R<N>" from 20 up - LevelMarks 1.2.0; the letters file carries it as a 6th field)   // was "0.19.1"   // (0.19.1, Rassul 11:13 CT "ok": the marks also keep clear of lsZzoComboMarks (SR / CB35 / CB99) and lsCobSignal (Cob, Lvp, Rev) labels - SignalBoard 1.6.0 kinds Z / K)   // was "0.19.0"   // (0.19.0, Rassul 2026-11-19 10:20-10:40 CT, mockup level-marks-mockup.html v8, "go"): lsRevReadMarks draws Bk / Sw / Cx on the candles too (LevelMarks.h 1.0.0), each with ONE short level name on a 2nd line (session > PD > PF > volume > weekly > hours), and the I with its level (WyRead 1.7.0); tags never overlap (WyMarks 2.0.0 layout); a short dashed line at the level; repeats cut; the letters file carries the level (stamp|letter|dir|level|px)   // was "0.18.5"   // (0.18.5, Rassul 2026-11-19 08:41 CT "lets include this ... a sweep of some htf level, in this case it was an hourly level. build": WyRead 1.2.0 - the prior 3 hours' highs / lows are levels the I can reclaim (the failed HTF breakout))   // was "0.18.4"   // (0.18.4, Rassul 22:33-22:37 CT "modify the I so it is a reclaim of a key level, volume level or a local level", "go": WyRead 1.1.0 - the I closes back through a level its pivot swept; the key / volume level files feed it for the current session)   // was "0.18.3"   // (0.18.3, Rassul 20:34 CT: no story / Rev-check line written over the letter strip when the chart is scrolled to the right edge)   // was "0.18.2"   // (0.18.2, Rassul 19:58 CT "remove the show poc option", "it should only show the I", "there should be no options": lsRevReadMarks draws only the I on the candle - no settings window, POC dash, Cx or hold dots)   // was "0.18.1"   // (0.18.1, Rassul 19:40 CT "all of the symbols are gone like A E I from the strip and the I is not displayed on the candles": the letters of the current session and the 3 before it, each session read by itself - at 5 PM only the new session was kept)   // was "0.18.0"   // (0.18.0, Rassul 2026-11-18 17:06 CT "I also want the I indicator on the candle, put it on top of green candle and below red candles", 17:11 "go" on i-on-candles.html: lsRevReadMarks draws each I of the letters file on its price candle - a small tag, green = bullish I / red = bearish I, white letter - OVER a green candle, UNDER a red one, one row further out when a Cx is on that side; WyMarks.h 1.0.0; A / E / Sp / Ut stay in the pane)   // was "0.17.5"   // (0.17.5, Rassul 13:08 CT: the session letters published to RevRead\\wy-<MKT>-<secs>.txt for the SDMO pane's I letters)   // was "0.17.4"   // (0.17.4, Rassul 09:53 CT "make the signal strip have a dark gray background": a dark gray band behind the letter strip)   // was "0.17.3"   // (0.17.3, Rassul 09:29 CT "can we hide bookflow for now and just keep delta + change": the Book Flow half is hidden, delta + change fill the pane; the old story letters on the Book Flow bars go with it)   // was "0.17.2"   // (0.17.2, Rassul 09:21 CT "a bullish I should have its low below the high of the dnpivot and vice versa": WyRead 1.0.1)   // was "0.17.1"   // (0.17.1, Rassul 09:11 CT "get rid of the reversal read header and make the letters smaller and use 1 line instead of two, if there are multiple letters ... make them smaller"): no title row; the strip is one row at the top, 9 pt, 7 pt side by side when a candle has 2+ letters   // was "0.17.0"   // (0.17.0, 2026-11-18 Rassul 05:45-08:15 CT "filter based on a wyckoff perspective", "lets add the letters on a strip on the top of the pane", "use Sp and Ut", "get rid of the ? marks", "dont put anything ont he candle chart", "lets build this"): a letter strip at the top of the pane - E lack of demand / supply, A effort without result, Sp spring / Ut upthrust (absorbed / flush, stop run, no follow-through), I the other side took over (WyRead.h 1.0.0); the read on the right says the Wyckoff step of a lettered candle   // was "0.16.0"   // (0.16.0, 2026-11-17 Rassul 21:15-21:17 CT "i wnat a more human readable and simplified format for these reads ... i need something practical", "i like it. lets do this"): the read = 3 plain lines (who is winning / why / what to watch, CombRead.h 1.2.0 toPlain), no numbers table; the Rev checks stay   // was "0.15.3"   // (0.15.3, Rassul 18:06 CT "lets go with 1.33x": the delta-change ratio label at 1.33x+ normal, was 2x)   // was "0.15.2"   // (0.15.2, his 17:58 CT screenshot "the delta is not drawing on the new bar", "make the delta pane on the top instead": delta + change on top, Book Flow below; the forming candle's delta drawn live; each session scaled by itself - the evening bars were too small to see; no ratio over the title)   // was "0.15.1"   // (0.15.1, Rassul 16:03 CT "go" after the case review: the pivot reads lead the one read - absorption, trapped, rejection wick, breakout met - CombRead 1.1.0)   // was "0.15.0"   // (0.15.0, Rassul 2026-11-17 15:24-15:41 CT "bookflow at the top and delta with change in delta at the bottom", "1 effective read", "i also want cx (climax) to be on the candles", "after that build"): the pane split - Book Flow bars on top, delta + delta change bars below (all above 0, colour = side, a ratio over a change of 1.33x+ normal); ONE read (CombRead.h: the passive Book Flow + the aggressive delta); lsRevReadMarks draws Cx on the climax candle of a pivot (over a down-pivot, under an up-pivot)   // was "0.14.0"   // (0.14.0, Rassul 12:12 CT "build": posts fact M - the forming candle's market orders - once a second for the HOD / LOD box's Now line)   // was "0.13.3"   // (0.13.3, Rassul 11:20 CT: one split read - verdict + sentences left (the story / Rev checks as the last sentence), the Bids / Offers table right)   // was "0.13.2"   // (0.13.2, his 11:18 CT screenshot: candle time on the Book Flow verdict line; story prices with the tick's decimals)   // was "0.13.1"   // (0.13.1, 2026-11-17 09:32 CT screenshot: the Book Flow read was blank - a candle without a record falls back to the newest earlier one; the mouse right of the candles reads the last closed one)   // was "0.13.0"   // (0.13.0, 2026-11-17 Rassul 08:35-08:44 CT: the Book Flow read split - verdict + plain-English sentences left, the Bids / Offers table right; the title now shows the real version - 0.12.0 still said 0.11.1)   // was "0.11.1"   // (0.11.1, 2026-11-16 Rassul 20:27 CT "i want ratios on bars where 1 is bigger than the other": the label = the bigger Book Flow side / the smaller, on the bigger bar - was the bar against its own 20-candle average)   // was "0.11.0"   // (0.11.0, 2026-11-16 Rassul 20:20 CT "i want to put book flow back": the pane draws the Book Flow bars itself - no lsBookFlowBg needed; buy / sell volume, delta and delta change stay computed, not drawn)   // was "0.10.1"   // (0.10.1, 2026-11-16 19:02 CT: AccumLogic 2.0.1 - the pivot close rule when the climax is the candle before the pivot)   // was "0.10.0"   // (0.10.0, 2026-11-16 Rassul 14:32-14:43 CT "are you overcomplicating it", "high volume on the pivot or right before it should be considered as part of the accum or dsit process", "go": the accumulation / distribution zones are AccumLogic 2.0.0, the simple rule - climax volume, stall, delta turned, with the trend; the read shows Climax / Stall / Delta)   // was "0.9.2"   // (0.9.2, 2026-11-16 Rassul 13:50 CT "where is the reversal read .. its blank": the bars are drawn in a low pane - under about 52 px nothing was drawn)   // was "0.9.1"   // (0.9.1, 2026-11-16 Rassul 12:44-12:45 CT "build", "i dont see the accumulaiton or distibution": the read shows the LAST zone when none is open, in 3 lines when the pane is low; every order event feeds AccumLogic 1.2.0 (resting lots added / pulled / filled per price) for the quiet end and the other side building)   // was "0.9.0"   // (0.9.0, 2026-11-16 Rassul 10:54-10:58 CT "can you create an accumulation indicator like you did with the boxes", "build now create read on the right side": accumulation / distribution zones (AccumLogic.h) found in the background from each candle's buy / sell lots and the Absorbed reads, their read right of the last candle, the zones saved to RevRead\\accum-<MKT>-<spb>.txt for the price-chart boxes (lsCobSignal 1.8.3 draws them). UNPROVEN = "?")   // was "0.8.0"   // (0.8.0, 2026-11-16 Rassul 06:00 CT "take the information you get from ... SR and put it in the reversal read indicator in the background ... continue to only show what it shows now": lsSR's stop-run engine runs here unseen (SRBack.h) and keeps lsSR's journal; nothing new is drawn)   // was "0.7.0"   // = rvs::VERSION = the one setVersion literal (rvh::stamp)

namespace rvh {
inline void stamp(cppExtension* p) {   // the literal on its own line: gex-build reads the text between the first ( and )
    p->setVersion("0.19.4");
}
inline short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
inline std::string flexDir() { const char* u = std::getenv("USERPROFILE"); return u && *u ? std::string(u) + "\\InvestorRT\\rtx\\lsFlexLevels" : std::string(); }
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0700);
#endif
}
inline bool known(const std::string& m) { static const char* const K[] = { "ES", "NQ", "CL", "NG", "GC", "HG", "EU" }; for (const char* k : K) if (m == k) return true; return false; }
inline std::string upper(std::string r) { for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]); return r; }
inline std::string marketOf(const std::string& root, const std::string& symbol = std::string()) {
    std::string m = mkey::norm(upper(root)); if (known(m)) return m;
    m = mkey::norm(upper(symbol)); if (known(m)) return m;
    const std::string s = upper(symbol); if (s.size() > 3) { m = mkey::norm(s.substr(0, s.size() - 2)); if (known(m)) return m; }
    return std::string();
}
// the session a local time belongs to: 17:00 CT starts the next trade date
inline std::string sessionOf(const struct tm& t) {
    struct tm x = t; x.tm_isdst = -1; if (x.tm_hour >= 17) x.tm_mday += 1; x.tm_hour = 12; x.tm_min = 0; x.tm_sec = 0;
    std::time_t tt = std::mktime(&x); struct tm y; std::memset(&y, 0, sizeof y);
#if defined(_WIN32)
    localtime_s(&y, &tt);
#else
    localtime_r(&tt, &y);
#endif
    char b[40]; std::snprintf(b, sizeof b, "%04d-%02d-%02d", y.tm_year + 1900, y.tm_mon + 1, y.tm_mday); return b;
}
// RTH open / close (Central minutes) per market, for the session name and the volatility windows
inline void rthOf(const std::string& m, int& o, int& c) {
    if (m == "ES" || m == "NQ") { o = 510; c = 900; } else if (m == "CL" || m == "NG") { o = 480; c = 870; } else if (m == "GC") { o = 440; c = 810; } else if (m == "HG") { o = 440; c = 780; } else { o = 440; c = 840; }
}
inline std::string recDir(const std::string& flex) { return flex.empty() ? std::string() : flex + "\\RevRead"; }
inline std::string recPath(const std::string& flex, const std::string& mkt, int spb, const std::string& session) {
    return flex.empty() || mkt.empty() || spb <= 0 || session.empty() ? std::string() : recDir(flex) + "\\" + mkt + "-" + std::to_string(spb) + "-" + session + ".csv";
}
// a small text file, bounded (params / the session record's letter lines)
inline bool readLines(const std::string& path, std::vector<std::string>& out, size_t maxBytes) {
    out.clear(); std::ifstream in(path.c_str(), std::ios::binary); if (!in.is_open()) return false;
    std::string all; all.reserve(4096); char buf[4096];
    while (in.good() && all.size() < maxBytes) { in.read(buf, sizeof buf); const std::streamsize n = in.gcount(); if (n <= 0) break; all.append(buf, (size_t)n); }
    size_t a = 0, nl; while ((nl = all.find('\n', a)) != std::string::npos) { std::string l = all.substr(a, nl - a); if (!l.empty() && l.back() == '\r') l.pop_back(); out.push_back(l); a = nl + 1; }
    return true;
}
static const COLOR C_GREEN = 0x0022C55E, C_RED = 0x00EF4444, C_WHITE = 0x00E5E7EB, C_GRAY = 0x009CA3AF, C_DIM = 0x006B7280, C_BAR_GREEN = 0x00178A42, C_BAR_RED = 0x00B03A3A, C_BAR_FLAT = 0x004B5563, C_DASH = 0x00FFFFFF, C_STRIP = 0x00333333, C_LEVEL = 0x00F0C040, C_TAG_TEXT = 0x00000000;   // (0.19.3) C_TAG_TEXT: black writing on the candle tags   // (0.19.0) C_LEVEL: the level line of a candle mark
// SEH-guarded VAP reads (as lsCandleAbsorb / lsWavePOC): -1 = a fault (never read again on this chart), -2 = no data
typedef cppExtension::RTARRAYP RTARRAYP; typedef cppExtension::BARSTATISTICS BARSTATISTICS; typedef cppExtension::VOLPROFILE VOLPROFILE;
inline int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !stats || bar < 0) return -2;
        std::memset(stats, 0, sizeof(*stats));
        if (vap->getBarStatistics(bar, *stats) != RTX_OK) return -2;
        if (stats->prices < 0) return -2;
        return stats->prices;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
inline int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rws, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || !rws || bar < 0 || count <= 0) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rws[i], 0, sizeof(VOLPROFILE)); if (vap->getVolumeProfile(bar, i, rws[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's volume-at-price rows; 1 = read, 0 = no VAP for the bar, -1 = fault. pv = sum(price in ticks x volume) for the VWAP
inline int vapRows(RTARRAYP& vap, int chartBar, double tick, std::vector<rvs::VRow>& out, double& pv, long long* dmm = nullptr) {   // dmm (0.6.0): [0] the bar's max delta, [1] its min delta
    out.clear(); pv = 0; if (dmm) { dmm[0] = 0; dmm[1] = 0; }
    if (vap.count <= chartBar) return 0;
    BARSTATISTICS st; const int prices = readStatistics(&vap, chartBar, &st);
    if (prices == -1) return -1;
    if (prices <= 0 || prices > 100000) return 0;
    if (dmm) { dmm[0] = (long long)st.maxDelta; dmm[1] = (long long)st.minDelta; }
    std::vector<VOLPROFILE> rv((size_t)prices);
    const int got = readPrices(&vap, chartBar, &rv[0], prices);
    if (got == -1) return -1;
    if (got < 0) return 0;
    for (const VOLPROFILE& v : rv) {
        if (!std::isfinite((double)v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.buyVolume + v.sellVolume <= 0) continue;
        const double q = (double)v.price / tick; if (!std::isfinite(q) || std::fabs(q) > 4e15) continue;
        rvs::VRow r; r.tick = (long long)std::llround(q); r.vol = (long long)v.buyVolume + (long long)v.sellVolume; r.dlt = (long long)v.buyVolume - (long long)v.sellVolume;
        out.push_back(r); pv += (double)r.tick * (double)r.vol;
    }
    return out.empty() ? 0 : 1;
}
// the candle POC store of one chart (both plugins): candle end -> Poc; read in calc only, a capped number of bars per pass
struct PocStore {
    std::map<long long, rvs::Poc> poc; std::map<long long, double> pv; int next = 0; long seenCount = -1; bool fault = false; long long missing = 0; long long lastKey = 0;
    void reset() { poc.clear(); pv.clear(); next = 0; seenCount = -1; missing = 0; lastKey = 0; }
    // (0.4.2, Rassul 2026-11-12 19:20 "can you check the CL because it is not showing": the pane filled from the chart's OLDEST candle,
    // 300 a pass, so the candles on screen came last - minutes on a quiet evening) only the newest KEEP candles are ever kept, so the
    // fill starts there; and a pass may run on past 300 candles while it has used under 10 ms (at most 3000).
    static const int KEEP = 6000;
    void start(long count) { if (next == 0 && lastKey == 0 && count - 1 > KEEP) next = (int)(count - 1 - KEEP); }
    static long long nowMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
    static bool more(int done, long long t0) { return done < 300 || (done < 3000 && nowMs() - t0 < 10); }
};
} // namespace rvh

#ifndef RVS_MARKS
// =====================================================================================================================
// lsRevRead - the pane
// =====================================================================================================================
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#define OFF_SDK 1
#include "OrderFlowFeed.h"
#include "FlowRead.h"          // (0.12.0) the order flow + pulling / stacking read (SDK-free, tests_revread/t_flowread)
#include "SplitText.h"         // (0.13.0) the split read: verdict + sentences left, numbers right (SDK-free)
#include "AccumLogic.h"        // (0.9.0) accumulation / distribution zones (SDK-free, tests_accum)
#include "SRBack.h"            // (0.8.0) lsSR in the background: same rules, same journal, nothing drawn
namespace rvs {
struct PaneLogic {   // the MboPane contract (the base's own bars / records are not drawn by this pane)
    static const char* name() { return "RevRead"; }
    static const char* title() { return "Reversal Read"; }
    static const char* version() { return RVS_VERSION; }
    static int metrics() { return 3; }
    static int paceMetric() { return off::PM_ADD; }
    static int optGate() { return ofo::G_OSTACK; }
    static std::string optLine(const ofo::Snap& s, int gate, const std::string&) { return ofo::stackLine(s, gate); }
    static bool resets() { return false; }
    static std::string hoverQuestion() { return "Why did it reverse?"; }
    static const char* const* hoverVerbs() { static const char* const v[2] = { "Buyers", "Sellers" }; return v; }
    static off::Info info(const off::PaneBar*, const off::Params&) { return off::Info(); }
    static long long value(const off::Rec& r, int metric, int s) { return metric == 0 ? r.addC[s] : metric == 1 ? r.pullC[s] : r.refN[s]; }
    static void decide(off::PaneBar& b, const off::Params&, bool) { for (int s = 0; s < 2; ++s) { b.spike[s] = false; b.dash[s] = -1; b.let[s].clear(); } b.msg = decltype(b.msg)(); }
};
} // namespace rvs

class RevRead : public off::MboPane<rvs::PaneLogic> {
public:
    RevRead() : off::MboPane<rvs::PaneLogic>() {}
    virtual bool barMarks() const { return false; }
    virtual bool textOff() const { return true; }
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, rvs::PaneLogic::title(), RVS_VERSION) ? RTX_OK : RTX_FAIL; }
    struct Ext : off::PaneExt {
        std::unique_ptr<rvs::Engine> eng; std::string mkt, session, path, note, symbol; int spb = 0; double tick = 0;
        size_t written = 0; long long writeFails = 0, events = 0, skipped = 0, sessions = 0, reconnects = -1, marksLoaded = 0;
        long long stamp = -1; int ctSec = 0; long long localSec = 0; int rthO = 510, rthC = 900;
        rvh::PocStore ps; long long fedCandle = 0; bool aligned = true;
        double vwPV = 0, vwV = 0; std::string vwSession; long long vwTick = 0;
        nl::Cache lvc; long long lvlMs = -100000; size_t nLevels = 0;
        int hDir = 0, hRot = 0; long long hPx = 0, hTop = 0; bool hBroken = false; long hCount = -1;
        long long lastPx = 0, barLo = 0, barHi = 0;                       // the forming candle (ticks)
        long long parMs = -100000; long long parSig = -2; int parKeys = 0; int pbBits = 0; long long pbMs = -100000;
        long long drawn = 0; std::string lastRead;
        long long wySig = -1; wyr::Out wy; int wyBars = 0;
        std::map<long long, lvm::Mark> lvMarks; std::map<long long, std::pair<std::string, long long> > iLv; std::map<long long, std::string> iSt;   /* (0.19.2) the I's strength */ std::vector<std::pair<long long, std::string> > lvKey;   // (0.19.0) the candle marks (Bk / Sw / Cx) with their level, and the level each I reclaimed (short name, px in ticks)   // (0.17.0) the Wyckoff strip of the session, rebuilt when a candle closes or a Book Flow record arrives
        // (0.7.0) the six checks
        rvs::Six six; int sixKeys = 0; long long sixParMs = -100000, sixParSig = -2; int sixLoaded = 0, sixDecided = 0;
        int pvDir = 0; long long pvTp = 0, pvExt = 0, pvW = 0; std::map<long long, int> trendAt; int trendNow = 0; std::vector<sb::Fact> revF; long long sixPostMs = -100000; long long mPostMs = -100000; int mPosts = 0;   // (0.14.0) fact M: the forming candle's market orders
        rvs::SixRead live; bool haveLive = false;
        acc::Engine ac; bool acLoaded = false; std::string acMkt, acPath, acBody; int acSpb = 0; long long acMs = -100000, acWrites = 0, acFails = 0; std::set<long long> acSeen; std::vector<acc::Abs> acPend;   // (0.9.0)
        srb::Back sr;                                                     // (0.8.0) the stop-run engine in the background
    };
    Ext* ext(S_& S) { if (!S.ext) S.ext.reset(new (std::nothrow) Ext()); return static_cast<Ext*>(S.ext.get()); }

    // ---- the feed: every order event, once, in order ------------------------------------------------------------------
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        Ext* e = ext(S); if (!e) return;
        try {   // (0.8.0) the stop-run engine first and on its own: nothing it does can change the read below
            struct tm st; const struct tm* ns = nullptr;
            if (o.timestamp && (long long)o.timestamp != e->sr.stamp) { std::memset(&st, 0, sizeof st); if (getLocaltime(o.timestamp, &st)) ns = &st; }
            e->sr.onRaw(o, ns, S.market, S.symbol, (int)S.spb, S.tick);
        } catch (...) { ++e->sr.faults; }
        const std::string mkt = rvh::marketOf(S.market);
        if (mkt.empty()) { e->note = "market not supported (" + S.market + ")"; ++e->skipped; return; }
        if (S.spb <= 0 || 3600 % (int)S.spb != 0) { e->note = "needs bars that divide the hour"; ++e->skipped; return; }
        if (!(S.tick > 0) || !o.timestamp) { ++e->skipped; return; }
        e->note.clear();
        if ((long long)o.timestamp != e->stamp) {                         // one clock conversion per second
            struct tm t; std::memset(&t, 0, sizeof t);
            if (!getLocaltime(o.timestamp, &t)) { ++e->skipped; return; }
            e->stamp = (long long)o.timestamp; e->ctSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
            const std::string sess = rvh::sessionOf(t);
            if (!e->eng || sess != e->session || mkt != e->mkt || (int)S.spb != e->spb) openSession(S, *e, sess, mkt);
            if (e->eng) { char cb[40]; std::snprintf(cb, sizeof cb, "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec); e->eng->clock = cb; context(S, *e); }
        }
        if (!e->eng) return;
        if (e->reconnects < 0) { e->reconnects = S.feed.reconnects; e->symbol = S.symbol; }
        if (S.feed.reconnects != e->reconnects || S.symbol != e->symbol) { e->six.reset(); e->eng->reset(); e->reconnects = S.feed.reconnects; e->symbol = S.symbol; }   // the book is re-sent
        const off::Ev x = off::evOf(o, S.tick, 0);
        if (x.action > 3 || x.px <= 0) { ++e->skipped; return; }
        e->six.onEvent(e->stamp, x.action, x.implied ? -1 : x.side, (long long)x.px, x.size, x.id, x.aggr, e->eng->book);   // (0.7.0) before the engine applies it (a MOD needs the order's old price)
        e->eng->onEvent(e->stamp * 1000, x.action, x.implied ? -1 : x.side, (long long)x.px, x.size, x.id, x.aggr); ++e->events;
        if (x.action == off::FILL) e->lastPx = (long long)x.px;
        if (!x.implied && x.side >= 0 && x.action != off::MOD) e->ac.onBook(e->stamp, x.action == off::NEW ? 0 : x.action == off::DEL ? 1 : 2, x.side == off::BID, (long long)x.px, x.size);   /* (0.9.1) the book cells of the accumulation zones */
    }
    void openSession(S_& S, Ext& e, const std::string& sess, const std::string& mkt) {
        if (e.eng) flush(e);
        std::unique_ptr<rvs::Engine> old; old.swap(e.eng);
        const bool same = old && e.mkt == mkt && e.spb == (int)S.spb;
        e.mkt = mkt; e.spb = (int)S.spb; e.tick = S.tick; e.session = sess; ++e.sessions; rvh::rthOf(mkt, e.rthO, e.rthC);
        e.eng.reset(new (std::nothrow) rvs::Engine()); if (!e.eng) return;
        if (!same) { e.six = rvs::Six(); e.sixParSig = -2; e.trendAt.clear(); e.haveLive = false; } e.six.spb = e.spb; if (e.sixParSig == -2) e.six.P = rvs::sixFor(mkt);
        e.eng->P = rvs::forMarket(mkt); e.eng->tick = e.tick; e.eng->spb = e.spb; e.written = 0; e.parMs = -100000; e.parSig = -2;
        if (same) { e.eng->book.swap(old->book); e.eng->lvl[0].swap(old->lvl[0]); e.eng->lvl[1].swap(old->lvl[1]); e.eng->hist.swap(old->hist); e.eng->base.swap(old->base); e.eng->p95 = old->p95; e.eng->nextId = old->nextId; }   // the 17:00 roll keeps the book and the normal
        const std::string flex = rvh::flexDir(); rvh::makeDir(flex); rvh::makeDir(rvh::recDir(flex));
        e.path = rvh::recPath(flex, mkt, e.spb, sess);
        std::vector<std::string> L; if (!e.path.empty() && rvh::readLines(e.path, L, 4 * 1024 * 1024)) { e.marksLoaded = e.eng->loadMarks(L); e.eng->lastV = rvs::lastV(L); e.sixLoaded += e.six.load(L);
            try { accLoad(S, e); for (const std::string& l : L) { acc::Abs a; if (acc::parseAbsLine(l, e.tick, a) && a.t > e.ac.resume && e.acSeen.insert(a.t * 2 + (a.side > 0 ? 1 : 0)).second) e.acPend.push_back(a); } } catch (...) {}   /* (0.9.0) the Absorbed reads decided before this restart */ }   // a restart: the letters decided before stay
        if (!e.path.empty()) { struct stat st; if (stat(e.path.c_str(), &st) != 0) { std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app); f << "H|" << RVS_VERSION << "|" << mkt << "|" << e.spb << "|" << sess << "|tick " << rvs::Engine::num(e.tick) << "\n"; } }
    }
    // the context a story is stamped with when it starts: session, volatility window, the hourly read, levels, Pb (once a second)
    void context(S_& S, Ext& e) {
        rvs::Ctx c; c.session = rvs::sessionOf(e.ctSec, e.rthO, e.rthC); c.volWin = rvs::volWindow(e.ctSec, e.rthO, e.rthC);
        const long long last = e.lastPx > 0 ? e.lastPx : e.barLo; const long long lo = e.barLo > 0 ? std::min(e.barLo, last) : last, hi = e.barHi > 0 ? std::max(e.barHi, last) : last;
        if (last > 0) c.hourly = rvs::hourlyText(e.hDir, e.hRot, e.hPx, e.hTop, e.hBroken, last, lo, hi, e.eng->P.zoneTicks + 2, e.tick);
        const long long now = off::steadyMs();
        if (now - e.pbMs >= 1000 || now < e.pbMs) {                      // lsBreakout's decided Pb / Pboc / Pdoc (fact E), the last 30 minutes
            e.pbMs = now; e.pbBits = 0; const char* sy = getSymbol(); const char* cl = getChartLabel(); std::vector<sb::Fact> F;
            if (sy && cl && sb::readFacts('E', sy, getSecondsPerBar(), cl, now, F, true)) { long long newest = 0; for (const sb::Fact& f : F) newest = std::max(newest, f.bar);
                for (const sb::Fact& f : F) if (newest - f.bar <= 1800) e.pbBits |= f.side > 0 ? 1 : 2;
                e.pvDir = 0; for (const sb::Fact& f : F) if (f.bar == newest && f.side != 0 && std::isfinite(f.hi) && f.hi > 0) { e.pvDir = f.side > 0 ? 1 : -1; e.pvExt = (long long)std::llround(f.hi / e.tick) + (f.side > 0 ? 1 : -1); } }   // (0.7.0) the newest decided pullback: its stop is 1 tick beyond the pivot
            e.revF.clear(); e.pvW = 0; if (sy && cl) { std::vector<sb::Fact> W; sb::readFacts('W', sy, getSecondsPerBar(), cl, now, W, false); for (const sb::Fact& f : W) { if (f.state == 1) e.revF.push_back(f); else if (f.side != 0 && f.hi > 0) { e.pvW = (long long)std::llround(f.hi); e.pvDir = f.side > 0 ? 1 : -1; } } }   // state 0 = the newest pullback without a Rev (also a provisional one): the live read's pivot                                  // (0.7.0) lsBreakout's Rev candles (when it posts them)
            sixTick(e, sy ? sy : "", cl ? cl : "", now);
        }
        if (now - e.sixParMs >= 30000 || now < e.sixParMs) {             // (0.7.0) the nightly's cut-offs of the six checks
            e.sixParMs = now; const std::string p = rvh::flexDir() + "\\LRA-RevSix-" + e.mkt + ".csv"; struct stat st; const long long sig = stat(p.c_str(), &st) == 0 ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
            if (sig != e.sixParSig) { e.sixParSig = sig; rvs::SixParams P = rvs::sixFor(e.mkt); e.sixKeys = 0; std::vector<std::string> L; if (sig >= 0 && rvh::readLines(p, L, 64 * 1024)) e.sixKeys = rvs::sixParse(L, e.mkt, P); e.six.P = P; }
        }
        c.pb = e.pbBits;
        if (now - e.lvlMs >= 5000 || now < e.lvlMs) {                    // the level files (key / MenthorQ), re-read at most every 5 s, + the session VWAP
            e.lvlMs = now; const std::string flex = rvh::flexDir(); std::vector<std::pair<long long, std::string> > out;
            if (!flex.empty()) { const std::vector<nl::Level> v = nl::levels(e.lvc, flex + "\\", e.mkt, S.symbol, now, (long long)std::time(nullptr));
                for (const nl::Level& l : v) if (l.px > 0 && out.size() < 400) out.push_back(std::make_pair((long long)std::llround(l.px / e.tick), l.name.substr(0, 24))); }
            if (e.vwV > 0) out.push_back(std::make_pair((long long)std::llround(e.vwPV / e.vwV), std::string("VWAP")));
            if (e.hDir != 0 && e.hPx > 0) out.push_back(std::make_pair(e.hPx, std::string("the HBO test line")));
            e.nLevels = out.size(); e.eng->levels.swap(out);
        }
        if (now - e.parMs >= 30000 || now < e.parMs) {                   // the nightly's thresholds (LRA-RevRead-<MKT>.csv), re-read on change
            e.parMs = now; const std::string p = rvh::flexDir() + "\\LRA-RevRead-" + e.mkt + ".csv"; struct stat st; const long long sig = stat(p.c_str(), &st) == 0 ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
            if (sig != e.parSig) { e.parSig = sig; rvs::Params P = rvs::forMarket(e.mkt); e.parKeys = 0; std::vector<std::string> L; if (sig >= 0 && rvh::readLines(p, L, 64 * 1024)) e.parKeys = rvs::parseParams(L, e.mkt, P); e.eng->P = P; }
        }
        e.eng->ctx = c;
    }
    // (0.7.0) once a second: decide the Revs lsBreakout posted, the live read of the newest pullback, and post the reads (fact X)
    int trendOf(const Ext& e, long long candle) const { const auto it = e.trendAt.find(candle); return it != e.trendAt.end() ? it->second : e.trendNow; }
    void sixTick(Ext& e, const std::string& sym, const std::string& chart, long long now) {
        const long long cur = e.six.candleOf(e.stamp);
        for (const sb::Fact& f : e.revF) { const long long tr = (long long)std::llround(f.lo), tp = (long long)std::llround(f.hi); if (f.side == 0 || tr <= 0 || tp <= 0 || tr >= cur || tr < tp || e.six.done.count(tr)) continue;
            if (e.six.decide(f.side > 0 ? 1 : -1, tp, tr, trendOf(e, tp), e.eng ? e.eng->clock : std::string())) ++e.sixDecided; }
        e.haveLive = false;
        if (e.pvDir != 0) { long long tp = e.pvW; int n = 0; if (tp > 0 && !e.six.cn.count(tp)) tp = -1;   /* lsBreakout named the pivot candle (fact W); else the newest decided pullback's stop (fact E) finds it */
            if (tp == 0)                                          // the pivot candle: the newest of the last 10 whose extreme is the pivot (1 tick of tolerance: an unsigned trade may have made it)
            for (auto it = e.six.cn.rbegin(); it != e.six.cn.rend() && n < 10; ++it, ++n) { if (!it->second.any) continue; const long long x = e.pvDir < 0 ? it->second.hi : it->second.lo; if (std::llabs(x - e.pvExt) <= 1) { tp = it->first; break; } }
            if (tp < 0) tp = 0; e.pvTp = tp; bool printed = false; for (auto it = e.six.done.rbegin(); it != e.six.done.rend() && !printed; ++it) printed = it->second.tp == tp;   /* its Rev is decided: the read is final, no live one */
            if (tp > 0 && !printed) { e.live = e.six.eval(e.pvDir, tp, 0, e.stamp, trendOf(e, tp)); e.haveLive = e.live.ok; } }
        if (now - e.sixPostMs >= 1000 || now < e.sixPostMs) { e.sixPostMs = now; std::vector<sb::Fact> v;
            auto mk = [&](const rvs::SixRead& r, int state) { sb::Fact f; f.bar = r.tr; f.side = r.dir; int bits = 0; for (int i = 0; i < 6; ++i) if (r.c[i]) bits |= 1 << i; if (r.trapped) bits |= 64;
                f.code = r.n * 1000 + bits; f.lo = (double)r.bbPx * e.tick; f.hi = (double)r.bsPx * e.tick; f.val = (double)r.pl * e.tick; f.state = state; f.proven = 0; v.push_back(f); };
            int n = 0; for (auto it = e.six.done.rbegin(); it != e.six.done.rend() && n < 30; ++it, ++n) mk(it->second, 1);
            if (e.haveLive) mk(e.live, 0);
            if (!sym.empty()) sb::postFacts('X', sym, getSecondsPerBar(), chart, now, v); }
        if (e.eng) { for (const std::string& l : e.six.out) e.eng->out.push_back(l); e.six.out.clear(); }
    }
    void flush(Ext& e) {
        if (!e.eng || e.path.empty() || e.written >= e.eng->out.size()) return;
        std::ofstream f(e.path.c_str(), std::ios::binary | std::ios::app);
        if (!f.is_open()) { ++e.writeFails; return; }
        for (; e.written < e.eng->out.size(); ++e.written) f << e.eng->out[e.written] << "\n";
        f.flush(); if (!f.good()) ++e.writeFails;
        if (e.eng->out.size() > 20000) { e.eng->out.clear(); e.written = 0; }
    }
    // ---- (0.9.0) accumulation / distribution zones (AccumLogic.h) ----------------------------------------------------------------
    void accLoad(S_& S, Ext& e) {
        const std::string mkt = rvh::marketOf(S.market); const int spb = (int)S.spb; if (mkt.empty() || spb <= 0) return;
        if (e.acLoaded && mkt == e.acMkt && spb == e.acSpb) return;
        e.ac.reset(); e.ac.spb = spb; e.acMkt = mkt; e.acSpb = spb; e.acLoaded = true; e.acSeen.clear(); e.acPend.clear(); e.acBody.clear();
        const std::string flex = rvh::flexDir(); e.acPath = flex.empty() ? std::string() : rvh::recDir(flex) + "\\accum-" + mkt + "-" + std::to_string(spb) + ".txt";
        if (e.acPath.empty()) return;
        std::ifstream f(e.acPath.c_str(), std::ios::binary); if (!f) return;
        std::string txt(262144, '\0'); f.read(&txt[0], (std::streamsize)txt.size()); txt.resize((size_t)f.gcount()); if (txt.size() >= 262144) return;
        e.ac.load(acc::parseFile(txt), acc::parseLast(txt)); e.acBody = txt;
    }
    void accFeed(S_& S, Ext& e, long long key, const rvs::Poc& p, const std::string& cs) {
        accLoad(S, e); if (!e.acLoaded || !p.ok) return;
        acc::Bar b; b.t = key; b.lo = p.lo; b.hi = p.hi; b.close = p.close; b.buy = p.buy; b.sell = p.sell;
        const bool live = e.ac.resume > 0 ? key > e.ac.resume : (!cs.empty() && cs == e.session);   // a first run starts with the running session; a restart goes on after the file
        if (!live) { e.ac.prime(b); return; }
        for (size_t i = 0; i < e.acPend.size();) { const acc::Abs& a = e.acPend[i]; if (a.t <= key) { e.ac.onAbsorb(a.t, a.side > 0, a.px, a.shown, a.lots, a.x); e.acPend.erase(e.acPend.begin() + (long)i); } else ++i; }
        e.ac.onCandle(b);
    }
    void accPump(S_& S, Ext& e) {
        if (!e.acLoaded) return;
        if (e.eng) {                                                      // a story that just reached Absorbed (decided once)
            auto see = [&](const rvs::Story& s) { if (s.tA <= 0 || s.anchor <= 0) return; const long long t = s.tA / 1000; if (!e.acSeen.insert(t * 2 + (s.bottom ? 1 : 0)).second) return;
                acc::Abs a; a.t = t; a.side = s.bottom ? 1 : -1; a.px = s.anchor; a.shown = s.shownSz; a.lots = s.absLots; a.x = s.absR; e.acPend.push_back(a); };
            for (int b = 0; b < 2; ++b) if (e.eng->live[b]) see(e.eng->st[b]);
            for (size_t i = e.eng->ended.size() > 8 ? e.eng->ended.size() - 8 : 0; i < e.eng->ended.size(); ++i) see(e.eng->ended[i]);
            if (e.acSeen.size() > 4000) e.acSeen.clear();
            if (e.acPend.size() > 400) e.acPend.erase(e.acPend.begin(), e.acPend.begin() + 200);
        }
        const long long now = off::steadyMs(); if (now - e.acMs < 1000 && now >= e.acMs) return; e.acMs = now;
        if (e.acPath.empty() || e.ac.lastT <= 0) return;
        const std::string body = acc::fileText(e.ac, e.acMkt, S.tick); if (body == e.acBody) return;
        const std::string tmp = e.acPath + ".tmp"; bool ok = false;
        { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (f) { f.write(body.data(), (std::streamsize)body.size()); f.flush(); ok = f.good(); } }
#if defined(_WIN32)
        if (ok) ok = ::MoveFileExA(tmp.c_str(), e.acPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        if (ok) ok = std::rename(tmp.c_str(), e.acPath.c_str()) == 0;
#endif
        if (ok) { e.acBody = body; ++e.acWrites; } else { std::remove(tmp.c_str()); ++e.acFails; }
    }
    void stopAll() { S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (e) { flush(*e); try { e->sr.stop(); } catch (...) {} } }
    virtual void afterPump(S_& S, long long, bool) { Ext* e = ext(S); if (e) { if (e->eng && e->stamp > 0) e->eng->onClock(e->stamp * 1000); flush(*e); try { e->sr.afterPump(); } catch (...) { ++e->sr.faults; } try { accPump(S, *e); } catch (...) {} } }
    virtual void afterTimer(S_& S, long long) { Ext* e = ext(S); if (e) { flush(*e); try { e->sr.flush(); } catch (...) { ++e->sr.faults; } } }
    virtual void statusExtra(S_& S, std::ostream& o) {
        Ext* e = ext(S); if (!e) { o << "\nREV_READ,unavailable"; return; }
        const rvs::Engine* E = e->eng.get();
        try { o << "\n" << e->sr.status(); } catch (...) {}
        try { int op = 0, cf = 0; for (const acc::Zone& z : e->ac.Z) { if (z.state == acc::OPEN) ++op; else if (z.state == acc::CONFIRMED) ++cf; }
              o << "\nACCUM,AccumLogic " << acc::VERSION << ",UNPROVEN (every zone is a ?),zones " << e->ac.Z.size() << ",open " << op << ",confirmed " << cf << ",candles " << e->ac.B.size() << ",absorbed reads " << e->ac.A.size() << ",resumed after " << e->ac.resume << ",file " << e->acPath << ",writes " << e->acWrites << ",fails " << e->acFails; } catch (...) {}
        o << "\nREV_READ,version " << RVS_VERSION << ",market " << e->mkt << ",session " << e->session << ",events " << e->events << ",skipped " << e->skipped << (e->note.empty() ? std::string() : ",note " + e->note)
          << ",fills " << (E ? E->fills : 0) << ",side mismatch " << (E ? E->sideMismatch : 0) << ",book " << (E ? E->book.size() : 0) << ",blocks " << (E ? E->hist.size() : 0) << ",normal p95 " << (E ? E->p95 : 0)
          << ",stories " << (E ? E->nextId - 1 : 0) << ",letters " << (E ? E->marks.size() : 0) << " (reloaded " << e->marksLoaded << ")" << ",levels " << e->nLevels << ",params keys " << e->parKeys << ",proven " << (E ? E->P.proven : 0)
          << ",record " << e->path << ",write fails " << e->writeFails
          << "\nREV_READ_CONTEXT," << (E ? E->ctx.session : std::string("?")) << "," << (E && E->ctx.volWin ? rvs::volName(E->ctx.volWin) : "no volatility window") << "," << (E ? E->ctx.hourly : std::string()) << ",pb bits " << e->pbBits
          << ",bar ends aligned " << (e->aligned ? "yes" : "NO - letters off")
          << "\nREV_READ_POC,candles " << e->ps.poc.size() << ",without VAP " << e->ps.missing << (e->ps.fault ? ",VAP FAULT - remove and re-add the indicator" : "")
          << "\nREV_SIX,fills " << e->six.fills << ",candles " << e->six.cn.size() << ",decided " << e->six.done.size() << " (now " << e->sixDecided << ", reloaded " << e->sixLoaded << ")" << ",params keys " << e->sixKeys
          << ",big buyer lots " << e->six.P.bbLots << ",big seller lots " << e->six.P.bsLots << ",rejection x " << e->six.P.rejX << ",pulled x " << e->six.P.pullX << ",fading x " << e->six.P.fadeX
          << ",pullback " << (e->pvDir > 0 ? "long" : e->pvDir < 0 ? "short" : "none") << ",pivot candle " << e->pvTp << ",trend " << e->trendNow << ",Rev facts " << e->revF.size() << ",live " << (e->haveLive ? e->live.n : -1)
          << "\nREV_READ_READ," << e->lastRead << ",draws " << e->drawn;
    }
    // ---- calc: the candles (bars + VAP are read here only) ----------------------------------------------------------------
    void rvCalc() {
        pump();
        S_* S = testState(); if (!S) return; Ext* e = ext(*S); if (!e) return;
        if (S->hov.left()) invalidateChart(false);                         // (0.6.0) the mouse left the chart: back to the newest story
        try { candles(*S, *e); } catch (...) { e->note = "calc error"; }
    }
    void candles(S_& S, Ext& e) {
        const long count = getBarCount(); if (count < 2 || !(S.tick > 0) || S.spb <= 0) return;
        RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose), opn(barOpen);   // (0.5.0) the open: the climax candle's colour
        if (times.count < count || hi.count < count || lo.count < count || cl.count < count) return;
        const double tick = S.tick; rvh::PocStore& P = e.ps;
        if (P.seenCount > count || (P.lastKey != 0 && P.next > 0 && P.next <= count - 1 && (long long)times[P.next - 1] != P.lastKey)) P.reset();   // the chart was reloaded
        P.seenCount = count;
        { const double fl = (double)lo[(int)count - 1], fh = (double)hi[(int)count - 1]; if (std::isfinite(fl) && std::isfinite(fh) && fl > 0) { e.barLo = (long long)std::llround(fl / tick); e.barHi = (long long)std::llround(fh / tick); } }
        e.aligned = ((long long)times[(int)count - 1] % (long long)S.spb) == 0;
        if (!P.fault) {
            RTARRAYP vap(barVolumeProfile); int done = 0; P.start(count); const long long t0 = rvh::PocStore::nowMs();
            for (; P.next < count - 1 && rvh::PocStore::more(done, t0); ++P.next, ++done) {       // closed bars only; 300 a pass, more while under 10 ms (0.4.2)
                const int i = P.next; const long long key = (long long)times[i]; std::vector<rvs::VRow> rows; double pv = 0; long long dmm[2] = { 0, 0 };
                const int r = rvh::vapRows(vap, i, tick, rows, pv, dmm);
                if (r < 0) { P.fault = true; break; }
                const double h = (double)hi[i], l = (double)lo[i], c = (double)cl[i];
                if (r == 0 || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c)) { ++P.missing; P.lastKey = key; continue; }
                const long long ht = (long long)std::llround(h / tick), lt = (long long)std::llround(l / tick), ct = (long long)std::llround(c / tick);
                rvs::Poc p = rvs::pocOf(key, rows, lt, ht, ct); { const double o = opn.count >= count ? (double)opn[i] : 0.0; if (std::isfinite(o) && o > 0) p.open = (long long)std::llround(o / tick); } p.maxD = dmm[0]; p.minD = dmm[1];
                struct tm t; std::memset(&t, 0, sizeof t); const bool tOk = getLocaltime((RTDATE)key - 1, &t) != 0; const std::string cs = tOk ? rvh::sessionOf(t) : std::string(); p.sess = rvs::sessId(cs); rvs::biggestIn(P.poc, p);   // (0.6.0)
                P.poc[key] = p; P.pv[key] = pv; P.lastKey = key; e.six.setTotal(key, p.total);
                try { accFeed(S, e, key, p, cs); } catch (...) {}   // (0.9.0) the accumulation zones: this closed candle, once
                while (P.poc.size() > (size_t)rvh::PocStore::KEEP) { P.pv.erase(P.poc.begin()->first); P.poc.erase(P.poc.begin()); }
                // the session VWAP from IRT's own volume at price (17:00 CT starts a new one)
                if (tOk) { const std::string& s = cs; if (s != e.vwSession) { e.vwSession = s; e.vwPV = 0; e.vwV = 0; } e.vwPV += pv; e.vwV += (double)p.total; }
                if (e.eng && e.aligned && key > e.fedCandle) { e.eng->onCandle(key, p, ct); e.fedCandle = key;
                    if (cs == e.session && key > e.eng->lastV) { e.eng->out.push_back(rvs::vLine(p, tick)); e.eng->lastV = key; } }   // (0.6.0) the background record, once per candle of this session   // the letters of that candle are decided now, once
            }
        }
        if (count != e.hCount && P.next >= count - 1) { e.hCount = count; hourly(S, e, times, hi, lo, cl, (int)count); vwTrend(S, e, times, hi, lo, cl, (int)count); }
        if (e.eng) flush(e);
    }
    // lsStepTrend's hourly steps on this chart's closed candles (the same walk as its 60-minute design): rotation, the newest step, its test line
    template <class TA, class FA> void hourly(S_& S, Ext& e, TA& times, FA& hi, FA& lo, FA& cl, int count) {
        e.hDir = 0; e.hRot = 0; e.hBroken = false; if (!stp::periodOk((int)S.spb) || (int)S.spb >= 3600) return;
        RTARRAY op(barOpen); if (op.count < count) return;
        stp::Engine E; const int first = std::max(0, count - 1 - stp::MAX_BARS);
        for (int i = first; i < count - 1; ++i) { stp::Bar b; b.t = (long long)times[i] - (long long)S.spb;
            const double o = (double)op[i], h = (double)hi[i], l = (double)lo[i], c = (double)cl[i]; if (!std::isfinite(o) || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c) || h < l) continue;
            b.o = (long long)std::llround(o / S.tick); b.h = (long long)std::llround(h / S.tick); b.l = (long long)std::llround(l / S.tick); b.c = (long long)std::llround(c / S.tick); E.addBar(b); }
        const int nh = (int)E.H.size() - (E.curClosed ? 0 : 1); if (nh < 2) return;
        stp::Engine X; X.hourly = true;
        for (int j = 0; j < nh; ++j) { const stp::Hour& h = E.H[(size_t)j]; stp::Bar b; b.t = h.k * 3600; b.o = h.o; b.h = h.h; b.l = h.l; b.c = h.c; X.addBar(b); }
        X.sealTo((E.H[(size_t)nh - 1].k + 1) * 3600);
        e.hRot = X.zdir; if (X.S.empty()) return;
        const stp::Step& q = X.S.back(); e.hDir = q.dir; e.hPx = q.px; e.hTop = stp::stepTop(q); e.hBroken = q.brI >= 0;
    }
    // (0.7.0) his vwapTrend at the newest closed candle (VwapTrendLogic.h on this chart's closed candles, the session of that candle), kept per candle
    template <class TA, class FA> void vwTrend(S_& S, Ext& e, TA& times, FA& hi, FA& lo, FA& cl, int count) {
        RTARRAYI vo(barVolume); if (vo.count < count || count < 3) return;
        std::vector<svl::Bar> B; const int first = std::max(0, count - 1 - 700); B.reserve((size_t)(count - first));
        for (int i = first; i < count - 1; ++i) { struct tm t; std::memset(&t, 0, sizeof t); if (!getLocaltime((RTDATE)times[i], &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return;
            svl::Bar b; b.days = svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday); b.endSec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec; b.h = (float)hi[i]; b.l = (float)lo[i]; b.c = (float)cl[i]; b.v = (double)vo[i]; B.push_back(b); }
        if (B.empty()) return;
        const vtl::Trend tr = vtl::fromChart(B, (int)B.size(), (int)S.spb, e.rthO, e.rthC);
        e.trendNow = vtl::level(tr); e.trendAt[(long long)times[count - 2]] = e.trendNow; while (e.trendAt.size() > 400) e.trendAt.erase(e.trendAt.begin());
    }
    virtual int scale(int, int, double* dMin, double* dMax) { if (dMin) *dMin = 0; if (dMax) *dMax = 1; return RTX_OK; }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        pump(true);
        S_* S = testState(); if (!S) return RTX_OK;
        Ext* e = ext(*S); if (!e) return RTX_OK;
        S->hov.left();
        try { render(*S, *e); } catch (...) { S->state = "draw error"; }
        return RTX_OK;
    }
    static const int LINE_H = 14;
    int put(const std::string& t, int x, int y, COLOR c, int lim) {
        const int w = (int)getTextWidth(t.c_str(), -1); if (x + w > lim) return -1;
        setTextColor(c); RCT r; r.set(rvh::coord(x), rvh::coord(y), rvh::coord(x + w), rvh::coord(y + LINE_H - 1)); r.drawText(t.c_str(), false, false);
        return w;
    }
    void render(S_& S, Ext& e) {
        RCT pane; pane.getPaneRect(false);
        const long count = getBarCount(); if (count < 2) return;
        RTARRAYI times(barDateTime); if (times.count < count) return;
        int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1);
        FONT fp; fp.id = HELVETICA; fp.size = 9; fp.style = PLAIN; FONT fb = fp; fb.style = BOLD; FONT fl = fp; fl.size = 11; fl.style = BOLD; FONT fq = fl; fq.style = PLAIN;   // fq (0.5.0): the quiet letters C / A / E
        const double bw = (double)std::max(1, (int)getPixelsPerBar());
        std::vector<std::pair<int, rvs::Poc> > c; c.reserve((size_t)(v1 - v0 + 1));
        for (int i = v0; i <= v1; ++i) { const auto it = e.ps.poc.find((long long)times[i]); if (it == e.ps.poc.end()) continue; PNT p; p.set(i, 0.0f, kBarCenter); c.push_back(std::make_pair((int)p.h, it->second)); }
        // (0.11.0, Rassul 2026-11-16 19:31-20:20 CT "instead of having the buy and sell volume, would it be an issue if the book flow showed instead", "i want to put
        // book flow back ... delta change, delta and buy and sell volume can be invisible in the background") the pane's bars = BOOK FLOW: per candle the resting
        // orders ADDED minus PULLED near the price (bf::netOf, the very numbers lsBookFlow draws), bids left in green, offers right in red, a bar only when
        // the net is positive; (0.11.1) "1.6x" on the bigger bar = the bigger side / the smaller side. Buy / sell volume, delta and the letters' own
        // numbers are still computed (the reads use them) - not drawn.
        std::vector<double> bfx[2];
        flr::In flowIn; bool flowOk = false; int flowBar = -1;   // (0.12.0)
        cbr::In cb; int cbBar = -1;   // (0.15.0) the one read: the candle under the mouse, else the last closed one
        if (RV_SHOW_BOOKFLOW) {
            c.clear(); off::PaneBar lb; bool haveLive = false;                 // the forming candle: its numbers as of now (as lsBookFlow builds them)
            {   const auto it = S.accs.find(S.formingKey);
                if (it != S.accs.end() && it->second.events > 0 && (long long)times[(int)count - 1] == S.formingKey) { lb = S.ser.make(S.formingKey, S.e.numbers(it->second, S.formingKey == S.firstBarKey, S.params.load.p)); haveLive = lb.ok; } }
            auto netOf = [](const off::Rec& r, int s) { return r.addC[s] - r.pullC[s]; };   // = bf::netOf (BookFlowLogic.h): resting lots added - pulled on side s
            auto recOf = [&](int i) -> const off::Rec* { if (i == (int)count - 1) return haveLive ? &lb.r : nullptr;
                const auto it = S.ser.bars.find((long long)times[i]); return it != S.ser.bars.end() && it->second.ok ? &it->second.r : nullptr; };
            for (int i = v0; i <= v1; ++i) { const off::Rec* r = recOf(i); if (!r) continue;
                rvs::Poc q; q.ok = true; q.t = (long long)times[i]; q.buy = std::max(0LL, netOf(*r, off::BID)); q.sell = std::max(0LL, netOf(*r, off::ASK));
                {   // (0.11.1, Rassul 20:27 CT "i want ratios on bars where 1 is bigger than the other") the bigger side / the smaller side, on the bigger bar
                    const long long big = std::max(q.buy, q.sell), sml = std::min(q.buy, q.sell); const int sd = q.buy >= q.sell ? 0 : 1;
                    bfx[0].push_back(big > 0 && big != sml ? (sml > 0 ? std::min(99.9, (double)big / (double)sml) : 99.9) : 0.0); bfx[1].push_back((double)sd); }
                PNT p; p.set(i, 0.0f, kBarCenter); c.push_back(std::make_pair((int)p.h, q)); }
            {   // (0.12.0) the order flow read's candle: the one under the mouse, else the last closed one; its numbers + x the 20 candles before
                const int hbF = S.hov.hover.bar; flowBar = hbF >= 0 && hbF < (int)count - 1 ? hbF : (int)count - 2;   // (0.13.1) the forming candle (= the mouse right of the candles) reads the last closed one
                cbBar = flowBar;
                if (const off::Rec* r0 = cbBar >= 0 ? recOf(cbBar) : nullptr) {   // (0.15.0) the passive side of the one read: this very candle's Book Flow (no fallback - without it the read says so)
                    cb.bookOk = true;
                    for (int s2 = 0; s2 < 2; ++s2) { cb.add[s2] = r0->addC[s2]; cb.pull[s2] = r0->pullC[s2]; cb.pullNear[s2] = r0->pullClose[s2]; cb.ref[s2] = r0->refC[s2]; cb.ice[s2] = r0->ice[s2]; }
                    cb.hit[0] = r0->hitC[off::BID]; cb.hit[1] = r0->hitC[off::ASK]; cb.bigBuy = r0->aggrMax[off::ASK]; cb.bigSell = r0->aggrMax[off::BID]; }
                const off::Rec* r = flowBar >= 0 ? recOf(flowBar) : nullptr;
                // (0.13.1, his 09:32 CT screenshot: the read was blank) a candle without a Book Flow record (the feed restarted with IRT, a gap) falls back to
                // the newest earlier candle that has one, up to 10 back; the table's time shows which candle it is
                for (int j = flowBar - 1; !r && j >= 0 && j >= flowBar - 10; --j) { const off::Rec* q = recOf(j); if (q) { r = q; flowBar = j; } }
                if (r) { flowOk = true; std::vector<long long> pb[2], rb[2];
                    for (int j = flowBar - 1, n = 0; j >= 0 && n < flr::NORM_N && flowBar - j <= 3 * flr::NORM_N; --j) { const off::Rec* q = recOf(j); if (!q) continue; ++n; for (int s2 = 0; s2 < 2; ++s2) { pb[s2].push_back(q->pullC[s2]); rb[s2].push_back(q->refC[s2]); } }
                    for (int s2 = 0; s2 < 2; ++s2) { flowIn.add[s2] = r->addC[s2]; flowIn.pull[s2] = r->pullC[s2]; flowIn.ref[s2] = r->refC[s2]; flowIn.hit[s2] = r->hitC[s2]; flowIn.ice[s2] = r->ice[s2];
                        flowIn.pullX[s2] = flr::timesNormal(r->pullC[s2], pb[s2]); flowIn.refX[s2] = flr::timesNormal(r->refC[s2], rb[s2]); } } }
            {   // (0.14.0, Rassul 2026-11-17 11:16-12:12 CT "look at the green candle and the flow .. it doesnt match", "build") FACT M, once a second: the
                // forming candle's market orders for the HOD / LOD box's "Now" line - lo = lots sold (hitting the bids), hi = lots bought (lifting the
                // offers), val = |delta| x the median |delta| of the 20 candles before (0 = not known), side = delta's sign, state 0 = live
                const long long msM = off::steadyMs();
                if (msM - e.mPostMs >= 1000 || msM < e.mPostMs) { e.mPostMs = msM; std::vector<sb::Fact> mv;
                    if (haveLive) { std::vector<long long> dd; for (int j = (int)count - 2, nn = 0; j >= 0 && nn < flr::NORM_N && (int)count - 2 - j <= 3 * flr::NORM_N; --j) { const off::Rec* q = recOf(j); if (!q) continue; ++nn; dd.push_back(std::llabs(q->hitC[1] - q->hitC[0])); }
                        const long long dl = lb.r.hitC[1] - lb.r.hitC[0]; sb::Fact f; f.bar = (long long)times[(int)count - 1]; f.lo = (double)lb.r.hitC[0]; f.hi = (double)lb.r.hitC[1];
                        f.side = dl > 0 ? 1 : dl < 0 ? -1 : 0; f.val = flr::timesNormal(std::llabs(dl), dd); f.state = 0; f.proven = 0; mv.push_back(f); }
                    const char* msy = getSymbol(); const char* mcl = getChartLabel(); if (msy && mcl) { sb::postFacts('M', msy, getSecondsPerBar(), mcl, msM, mv); ++e.mPosts; } } }
        }
        static const std::map<long long, std::vector<rvs::Mark> > none;
        // (0.6.0) buy and sell volume side by side; no values; the "biggest in" count on a bar that is the biggest in 10+ candles
        // (0.15.0, Rassul 15:24 CT "bookflow at the top and delta with change in delta at the bottom") a pane 70 px or taller is split: Book Flow on top, delta below
        const int paneH = (int)(pane.bottom - pane.top); const bool split = RV_SHOW_BOOKFLOW && paneH >= 70; const int bfBottom = split && RV_DRAW_BOOKFLOW ? (int)pane.top + paneH * 50 / 100 : (int)pane.bottom;   // (0.15.2) bfBottom = the split line: DELTA above it, BOOK FLOW below (Rassul 17:58 CT "make the delta pane on the top instead")
        const std::vector<rvs::BSBar> B = !RV_DRAW_BOOKFLOW ? std::vector<rvs::BSBar>() : split ? rvs::layoutBS(c, e.eng ? e.eng->marks : none, bfBottom + 2, pane.bottom, 12, bw, 14) : rvs::layoutBS(c, e.eng ? e.eng->marks : none, pane.top, bfBottom, 12, bw, LINE_H + 2);
        struct BfLab { int l, t, r, b, side; double x; char tx[16]; }; std::vector<BfLab> bfLab;   // (0.11.1) the ratio labels: bigger / smaller on the bigger bar, 1.2x or more; where two would overlap the bigger ratio stays
        if (RV_SHOW_BOOKFLOW) { setFont(fb);
            for (size_t k = 0; k < B.size() && k < bfx[0].size(); ++k) { const double x = bfx[0][k]; if (x < BF_RATIO_MIN) continue; const rvs::BSBar& b = B[k]; const int s = bfx[1][k] > 0.5 ? 1 : 0;
                BfLab L; L.x = x; L.side = s; if (x >= 99.0) std::snprintf(L.tx, sizeof L.tx, "99x+"); else std::snprintf(L.tx, sizeof L.tx, "%.1fx", x);
                const int w = (int)getTextWidth(L.tx, -1), cx = s == 0 ? b.x - b.w / 2 : b.x + 1 + b.w / 2, y = s == 0 ? b.numYB : b.numYS;
                L.l = cx - w / 2; L.r = L.l + w + 1; L.t = y; L.b = y + 12;
                if (!bfLab.empty()) { const BfLab& p = bfLab.back(); if (L.l < p.r + 2 && L.t < p.b && p.t < L.b) { if (L.x > p.x) bfLab.back() = L; continue; } }
                bfLab.push_back(L); } }
        for (const rvs::BSBar& b : B) {
            if (b.topB < b.bottom) off::fillFlat(this, b.x - b.w, b.topB, b.x, b.bottom, rvh::C_GREEN);
            if (b.topS < b.bottom) off::fillFlat(this, b.x + 1, b.topS, b.x + 1 + b.w, b.bottom, rvh::C_RED);
            setFont(fb);
            if (b.nB) { const std::string t = std::to_string(b.nB); const int w = (int)getTextWidth(t.c_str(), -1); const int cx = b.x - b.w / 2 - (b.nS ? w / 2 : 0); setTextColor(rvh::C_GREEN); RCT r; r.set(rvh::coord(cx - w / 2), rvh::coord(b.numYB), rvh::coord(cx - w / 2 + w + 1), rvh::coord(b.numYB + 12)); r.drawText(t.c_str(), false, false); }
            if (b.nS) { const std::string t = std::to_string(b.nS); const int w = (int)getTextWidth(t.c_str(), -1); const int cx = b.x + 1 + b.w / 2 + (b.nB ? w / 2 : 0); setTextColor(rvh::C_RED); RCT r; r.set(rvh::coord(cx - w / 2), rvh::coord(b.numYS), rvh::coord(cx - w / 2 + w + 1), rvh::coord(b.numYS + 12)); r.drawText(t.c_str(), false, false); }
            if (!b.marks.empty()) { setFont(fl); int tw = 0; std::vector<std::string> T;
                for (const rvs::Mark& m : b.marks) { std::string t(1, m.letter); T.push_back(t);   /* (Rassul 2026-11-11 22:19 CT "get rid of the ? mark after the signals so just have C instead of C?") no "?" */ tw += (int)getTextWidth(t.c_str(), -1) + 2; }
                int x = b.x - tw / 2; const int y = std::max((int)pane.top + LINE_H + 2, b.letY - 2);
                for (size_t k = 0; k < T.size(); ++k) { const int w = (int)getTextWidth(T[k].c_str(), -1); const bool sig = b.marks[k].letter == 'I';   /* (0.5.0, "ok build") the initiation is the signal: bold and bright; C / A / E are the story so far: plain and dim */
                    setFont(sig ? fl : fq); setTextColor(sig ? (b.marks[k].col > 0 ? rvh::C_GREEN : rvh::C_RED) : (b.marks[k].col > 0 ? rvh::C_BAR_GREEN : rvh::C_BAR_RED)); RCT r; r.set(rvh::coord(x), rvh::coord(y), rvh::coord(x + w + 1), rvh::coord(y + 13)); r.drawText(T[k].c_str(), false, false); x += w + 2; } }
        }
        if (split) {   // (0.15.0, 15:28 CT "the change in delta that is negative on the same side as the positive delta") per candle: delta (left, bright) and its
                       // change from the candle before (right, dim), both drawn UP from the bottom, green = buyers / red = sellers; a ratio over a change of 1.33x+ normal
            auto sessKey = [&](int i) -> long long { struct tm t; std::memset(&t, 0, sizeof t); if (!getLocaltime((RTDATE)times[i], &t)) return 0; long long k = (long long)(t.tm_year) * 400 + t.tm_yday; return t.tm_hour >= 17 ? k + 1 : k; };
            auto pocAt = [&](int i) -> const rvs::Poc* { if (i < 0 || i >= (int)count) return nullptr; const auto it = e.ps.poc.find((long long)times[i]); return it != e.ps.poc.end() && it->second.ok ? &it->second : nullptr; };
            // (0.17.0) the Wyckoff strip under the title row: up to 3 letters per candle, one under the other (Sp / Ut, A, E, I)
            // (0.17.1, Rassul 09:11 CT "the strip is taking up too much space ... get rid of the reversal read header ... make the letters smaller and use
            // 1 line ... if there are multiple letters, can you just make them smaller") ONE row at the very top of the pane (no title row), 9 pt; a candle
            // with 2+ letters writes them side by side in 7 pt
            const int WY_ROW = 11, wyTop = (int)pane.top + 2;
            const int wyShift = bfBottom - 2 - (wyTop + WY_ROW + 1) - 14 >= 24 ? WY_ROW : 0;   // a very low pane: the row is drawn over the bars' top
            {   // rebuilt only when a candle closed or a Book Flow record arrived (the same closed candles give the same letters - no repaint)
                auto hasRec = [&](int i) { if (i < 0) return 0; const auto it = S.ser.bars.find((long long)times[i]); return it != S.ser.bars.end() && it->second.ok ? 1 : 0; };
                const long long sig = (long long)times[(int)count - 2] * 16 + (pocAt((int)count - 2) ? 8 : 0) + hasRec((int)count - 2) * 4 + hasRec((int)count - 3) * 2 + hasRec((int)count - 4);
                if (sig != e.wySig) { e.wySig = sig;
                    auto recClosed = [&](int i) -> const off::Rec* { const auto it = S.ser.bars.find((long long)times[i]); return it != S.ser.bars.end() && it->second.ok ? &it->second.r : nullptr; };
                    // (0.18.1, Rassul 19:40 CT "all of the symbols are gone like A E I from the strip and the I is not displayed on the candles": at 5 PM
                    // the new session started and only ITS letters were kept, so the day's letters vanished) the letters of the current session AND the 3
                    // sessions before it (each session read by itself, exactly as before; at most 2,400 candles back) - the buy / sell volume comes from
                    // IRT's own volume at price, so earlier sessions keep their letters after a restart too
                    int dec = 0; { double t = S.tick; while (dec < 6 && std::fabs(t - std::round(t)) > 1e-9) { t *= 10.0; ++dec; } }
                    wyr::Out all; int nBars = 0; int segEnd = (int)count - 2;
                    std::map<long long, lvm::Mark> lvAll; std::map<long long, std::pair<std::string, long long> > iLvAll;   // (0.19.0)
                    // (0.18.4) the key / volume levels the I can reclaim (WyRead 1.1.0): the level files as they stand now - for the CURRENT session only
                    // (older sessions had other levels; they use their own pivot lows / highs); the moving VWAP and the HBO line are not reclaim levels
                    std::vector<wyr::Lvl> lvNow; if (e.eng) for (const auto& lp : e.eng->levels) { if (lp.second.compare(0, 4, "VWAP") == 0 || lp.second == "the HBO test line" || lp.first <= 0) continue; if (lvm::pri(lp.second, 0) < 0 || lp.second.find('|') != std::string::npos) continue;   /* (0.19.4, Rassul 14:14 CT "I should not print if there is no level") only the named key levels (the Bk / Sw list) - gamma / MenthorQ levels give no I */   /* (0.19.4, Rassul 14:12 CT "I didnt mention a level": the VWAP bands (VWAP-1 / -2 / +1 ...) move - an I that "reclaimed" today's band at its value NOW is not a level it reclaimed then; only VWAP itself was skipped before) */ wyr::Lvl z; z.px = lp.first; z.name = lp.second; lvNow.push_back(z); }
                    for (int seg = 0; seg < 4 && segEnd >= 0 && (int)count - 2 - segEnd < 2400; ++seg) {
                    std::vector<wyr::Bar> WB; const long long sk = sessKey(segEnd); int first = segEnd;
                    while (first > 0 && segEnd - first < 600 && sessKey(first - 1) == sk) --first;
                    for (int i = first; i <= segEnd; ++i) { const rvs::Poc* p = pocAt(i); if (!p) continue; wyr::Bar b; b.t = (long long)times[i];
                        b.h = p->hi; b.l = p->lo; b.c = p->close; b.o = p->open > 0 ? p->open : p->close; b.buy = p->buy; b.sell = p->sell;
                        if (const off::Rec* r = recClosed(i)) { b.book = true; for (int s2 = 0; s2 < 2; ++s2) { b.addNet[s2] = r->addC[s2] - r->pullC[s2]; b.aggrMax[s2] = r->aggrMax[s2]; b.sweepLv[s2] = r->sweepLv[s2]; b.ice[s2] = r->ice[s2]; } }
                        struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)times[i], &t)) b.when = off::clock12(t.tm_hour, t.tm_min);
                        WB.push_back(b); }
                    nBars += (int)WB.size();
                    try { const wyr::Out o = wyr::run(WB, S.tick, dec, wyr::Params(), seg == 0 ? lvNow : std::vector<wyr::Lvl>()); for (const auto& kv : o.marks) all.marks[kv.first] = kv.second; for (const auto& kv : o.reads) all.reads[kv.first] = kv.second;
                        // (0.19.0, Rassul 10:37 CT "put level under I too") the I's level as the candle tag's short name (MenthorQ / gamma: no name)
                        for (const auto& kv : o.ilv) { const auto hb = o.ilvHb.find(kv.first); const int h = hb != o.ilvHb.end() ? hb->second : 0;
                            const std::string sn = lvm::iLevel(kv.second.second, h, kv.second.second.find("high") != std::string::npos); iLvAll[kv.first] = std::make_pair(sn, kv.second.first); } } catch (...) {}
                    // (0.19.0, Rassul 10:20-10:40 CT, mockup v8, "go") Bk / Sw / Cx with one level each - the current session with the level files, earlier ones with their hour levels
                    try { const std::map<long long, lvm::Mark> lm = lvm::run(WB, seg == 0 ? lvNow : std::vector<wyr::Lvl>()); for (const auto& kv : lm) lvAll[kv.first] = kv.second; } catch (...) {}
                    segEnd = first - 1; }
                    // (0.19.4, Rassul 14:14 CT "I should not print if there is no level") an I without a named level is not kept (letter strip, candle tag and file alike)
                    for (auto it = all.marks.begin(); it != all.marks.end(); ) { const auto il = iLvAll.find(it->first); const bool named = il != iLvAll.end() && !il->second.first.empty();
                        if (!named) { std::vector<wyr::Mark>& v = it->second; v.erase(std::remove_if(v.begin(), v.end(), [](const wyr::Mark& m) { return m.k == 'I'; }), v.end()); }
                        if (it->second.empty()) { all.reads.erase(it->first); it = all.marks.erase(it); } else ++it; }
                    // (0.19.2) the strength line (his BVMax / BRMax, as the Volume / Range panes count it) for every Bk / Sw and I, from the chart's closed candles
                    std::map<long long, std::string> iStAll;
                    try { RTARRAYI vo(barVolume); RTARRAY sh(barHigh), sl(barLow), so(barOpen), sc(barClose); const int nC = (int)count - 1;
                        if (nC > 0 && vo.count >= nC && sh.count >= nC && sl.count >= nC && so.count >= nC && sc.count >= nC) {
                            std::vector<long long> T((size_t)nC); std::vector<double> V((size_t)nC), R((size_t)nC); std::vector<char> G((size_t)nC);
                            for (int i = 0; i < nC; ++i) { T[(size_t)i] = (long long)times[i]; const double vv = (double)vo[i]; V[(size_t)i] = std::isfinite(vv) && vv > 0 ? vv : 0.0;
                                R[(size_t)i] = lvm::rangeTicks((double)sh[i], (double)sl[i], S.tick); G[(size_t)i] = (double)sc[i] >= (double)so[i] ? 1 : 0; }
                            if (std::is_sorted(T.begin(), T.end())) { lvm::fillStrength(lvAll, T, V, R, G);
                                for (const auto& kv : all.marks) for (const wyr::Mark& m : kv.second) if (m.k == 'I') { const std::string st = lvm::strengthAt(T, V, R, G, kv.first); if (!st.empty()) iStAll[kv.first] = st; } } } } catch (...) {}
                    e.iSt.swap(iStAll);
                    e.lvKey.clear(); for (const wyr::Lvl& z : lvNow) if (lvm::pri(z.name, 0) >= 0 && z.name.find('|') == std::string::npos) e.lvKey.push_back(std::make_pair(z.px, z.name.substr(0, 24)));   // (0.19.0) the key / volume levels the BO indicator labels its Rly with
                    e.wyBars = nBars; e.wy.marks.swap(all.marks); e.wy.reads.swap(all.reads); e.lvMarks.swap(lvAll); e.iLv.swap(iLvAll);
                    // (0.17.5, Rassul 13:08 CT "yes, just like the mockup" - the I letters on the SDMO pane too) the session's letters are published
                    // once per closed candle to lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt (candle stamp|letter|dir); lsSDMO draws the I's from it, so the
                    // two panes always agree (and the nightly scores every letter from it)
                    try { const std::string flex = rvh::flexDir(), mk = rvh::marketOf(S.market);
                        if (!flex.empty() && !mk.empty() && S.spb > 0) {
                            std::string txt = "WY|1|" + mk + "|" + std::to_string((long long)S.spb) + "|" + RVS_VERSION + "\n";
                            for (const auto& kv : e.wy.marks) for (const wyr::Mark& m : kv.second) { txt += std::to_string(kv.first) + "|" + wyr::label(m) + "|" + std::to_string(m.dir);
                                if (m.k == 'I') { const auto il = e.iLv.find(kv.first); const auto is = e.iSt.find(kv.first); const std::string st = is != e.iSt.end() ? is->second : std::string();
                                    if (il != e.iLv.end()) txt += "|" + il->second.first + "|" + std::to_string(il->second.second) + "|" + st;   // (0.19.0) the I's level: 5 fields; (0.19.2) + its strength: 6
                                    else if (!st.empty()) txt += "||0|" + st; }
                                txt += "\n"; }
                            for (const auto& z : e.lvKey) txt += "L|" + std::to_string(z.first) + "|" + z.second + "\n";   // (0.19.0) the levels (WyMarks parseLevels)
                            for (const auto& kv : e.lvMarks) txt += std::to_string(kv.first) + "|" + lvm::codeText(kv.second.code) + "|" + std::to_string(kv.second.dir) + "|" + kv.second.lv + "|" + std::to_string(kv.second.px) + "|" + kv.second.st + "\n";   // (0.19.0) Bk / Sw / Cx; (0.19.2) + the strength (6th field, may be empty)
                            const std::string path = rvh::recDir(flex) + "\\wy-" + mk + "-" + std::to_string((long long)S.spb) + ".txt", tmp = path + ".tmp";
                            bool ok = false; { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (f) { f << txt; f.flush(); ok = f.good(); } }
#if defined(_WIN32)
                            if (ok && !::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) std::remove(tmp.c_str());
#else
                            if (ok && std::rename(tmp.c_str(), path.c_str()) != 0) std::remove(tmp.c_str());
#endif
                            if (!ok) std::remove(tmp.c_str()); } } catch (...) {} } }
            {   FONT f7 = fb; f7.size = 7;
                // (0.17.4, Rassul 09:53 CT "make the signal strip have a dark gray background") a dark gray band behind the letters, from the pane's left
                // edge to just past the last candle (never under the read text on the right)
                {   const int lastI = std::min(v1, (int)count - 1); PNT pl; pl.set(lastI, 0.0f, kBarCenter); PNT pp; pp.set(std::max(v0, lastI - 1), 0.0f, kBarCenter);
                    const int half = std::max(2, ((int)pl.h - (int)pp.h) / 2 + 1);
                    off::fillFlat(this, (int)pane.left, wyTop - 1, (int)pl.h + half, wyTop + WY_ROW + 1, rvh::C_STRIP); }
                for (int i = v0; i <= v1 && i < (int)count - 1; ++i) { const auto it = e.wy.marks.find((long long)times[i]); if (it == e.wy.marks.end() || it->second.empty()) continue;
                    PNT pt; pt.set(i, 0.0f, kBarCenter); const int cx = (int)pt.h; const bool many = it->second.size() > 1; setFont(many ? f7 : fb);
                    int tw = 0; for (const wyr::Mark& m : it->second) tw += (int)getTextWidth(wyr::label(m).c_str(), -1);
                    int xx = cx - tw / 2; const int yy = wyTop + (many ? 1 : 0);
                    for (const wyr::Mark& m : it->second) { const std::string t = wyr::label(m); const int w = (int)getTextWidth(t.c_str(), -1);
                        setTextColor(m.dir > 0 ? rvh::C_GREEN : rvh::C_RED); RCT r; r.set(rvh::coord(xx), rvh::coord(yy), rvh::coord(xx + w + 1), rvh::coord(yy + WY_ROW)); r.drawText(t.c_str(), false, false); xx += w; } } }
            const int dTop = wyTop + wyShift + 1, dBot = bfBottom - 2, dH = dBot - dTop - 14;   // (0.15.2) the top half under the title row; 14 px kept for the labels; (0.17.0) under the Wyckoff strip
            struct DB { int x; long long dl, chg; bool hasChg; double lab; long long sess; }; std::vector<DB> D;
            for (int i = v0; i <= v1; ++i) { const rvs::Poc* p = pocAt(i); if (!p) continue; const rvs::Poc* q = pocAt(i - 1); DB d; PNT pt; pt.set(i, 0.0f, kBarCenter); d.x = (int)pt.h; d.dl = p->cdlt; d.hasChg = q != nullptr; d.chg = q ? p->cdlt - q->cdlt : 0; d.lab = 0;
                if (q) { std::vector<long long> h; for (int k = i - 1; k >= 1 && (int)h.size() < 20 && i - k <= 40; --k) { const rvs::Poc* a = pocAt(k); const rvs::Poc* b2 = pocAt(k - 1); if (a && b2) h.push_back(a->cdlt - b2->cdlt); }
                    if (h.size() >= 10) d.lab = cbr::changeLabel(d.chg, cbr::medAbs(h)); }
                d.sess = sessKey(i); D.push_back(d); }
            // (0.15.2, Rassul 17:58 CT "the delta is not drawing on the new bar") the FORMING candle is drawn live: its delta so far from IRT's volume at price
            if (v1 >= (int)count - 1 && count >= 2) { RTARRAYP vap(barVolumeProfile); std::vector<rvs::VRow> rows; double pv = 0;
                if (rvh::vapRows(vap, (int)count - 1, S.tick, rows, pv) > 0) { long long dl = 0; for (const rvs::VRow& r : rows) dl += r.dlt; const rvs::Poc* q = pocAt((int)count - 2);
                    DB d; PNT pt; pt.set((int)count - 1, 0.0f, kBarCenter); d.x = (int)pt.h; d.dl = dl; d.hasChg = q != nullptr; d.chg = q ? dl - q->cdlt : 0; d.lab = 0; d.sess = sessKey((int)count - 1); D.push_back(d); } }
            // (0.15.2) each SESSION is scaled by itself (the evening's bars were too small to see next to the day session's), on the session's 95th
            // percentile so one huge bar does not flatten the rest - a taller bar is clipped at the top
            std::map<long long, long long> mxS; { std::map<long long, std::vector<long long> > all; for (const DB& d : D) { all[d.sess].push_back(std::llabs(d.dl)); if (d.hasChg) all[d.sess].push_back(std::llabs(d.chg)); }
                for (auto& kv : all) { std::vector<long long>& v = kv.second; std::sort(v.begin(), v.end()); const long long p95 = v.empty() ? 0 : v[(size_t)std::min(v.size() - 1, (size_t)(0.95 * (double)v.size()))]; mxS[kv.first] = std::max(1LL, p95 * 115 / 100); } }
            const long long mx = 1;
            const int w = B.empty() ? std::max(1, (int)(bw * 0.4)) : B.front().w;
            if (dH > 8 && mx > 0 && !D.empty()) { setFont(fb); int lastR = -100000; double lastX = 0;
                const std::string ttl = "Delta + change"; const int tlR = (int)pane.left + 6 + (int)getTextWidth(ttl.c_str(), -1) + 4, tlB = dTop + 14;   // the title's box: no ratio label over it
                for (const DB& d : D) { const double m2 = (double)mxS[d.sess]; const int h1 = std::min(dH, (int)((double)std::llabs(d.dl) * dH / m2)), h2 = std::min(dH, (int)((double)std::llabs(d.chg) * dH / m2));
                    if (h1 > 0) off::fillFlat(this, d.x - w, dBot - h1, d.x, dBot, d.dl >= 0 ? rvh::C_GREEN : rvh::C_RED);
                    if (d.hasChg && h2 > 0) off::fillFlat(this, d.x + 1, dBot - h2, d.x + 1 + w, dBot, d.chg >= 0 ? rvh::C_BAR_GREEN : rvh::C_BAR_RED);
                    if (d.lab > 0) { char t[16]; std::snprintf(t, sizeof t, "%.1fx", d.lab); const int tw2 = (int)getTextWidth(t, -1), cx = d.x + 1 + w / 2, l = cx - tw2 / 2;
                        if (l < lastR + 2 && d.lab <= lastX) continue;   // two labels side by side: the bigger one stays (the later one is skipped when smaller)
                        if (l < tlR && dBot - h2 - 13 < tlB) continue;     // (0.15.2) never over the "Delta + change" title
                        setTextColor(d.chg >= 0 ? rvh::C_GREEN : rvh::C_RED); RCT r; r.set(rvh::coord(l), rvh::coord(dBot - h2 - 13), rvh::coord(l + tw2 + 1), rvh::coord(dBot - h2 - 1)); r.drawText(t, false, false); lastR = l + tw2; lastX = d.lab; } }
                setFont(fb); put(ttl, (int)pane.left + 6, dTop, rvh::C_GRAY, (int)pane.right - 6); }
            if (RV_DRAW_BOOKFLOW) { setFont(fb); put("Book Flow", (int)pane.left + 6, bfBottom + 3, rvh::C_GRAY, (int)pane.right - 6); }
            if (RV_DRAW_BOOKFLOW) {   // a thin line between the halves, under the candles only (the read right of the last candle runs over both halves)
                int xEnd = (int)pane.right; if (v1 >= (int)count - 1) { PNT q; q.set((int)count - 1, 0.0f, kBarCenter); xEnd = (int)q.h + (int)bw; }
                setPen(rvh::C_DIM, 1, P_SOLID); PNT a; a.set(0, 0.0f); a.h = rvh::coord((int)pane.left); a.v = rvh::coord(bfBottom + 2); a.setDrawPosition(); PNT b2; b2.set(0, 0.0f); b2.h = rvh::coord(xEnd); b2.v = rvh::coord(bfBottom + 2); b2.drawLineTo(); }
            // the aggressive side of the one read
            if (cbBar >= 1) { const rvs::Poc* p = pocAt(cbBar); const rvs::Poc* q = pocAt(cbBar - 1);
                if (p && q) { cb.deltaOk = true; cb.dl = p->cdlt; cb.dlPrev = q->cdlt; std::vector<long long> hd, hc;
                    for (int k = cbBar - 1; k >= 1 && (int)hd.size() < 20 && cbBar - k <= 40; --k) { const rvs::Poc* a = pocAt(k); const rvs::Poc* b2 = pocAt(k - 1); if (!a) continue; hd.push_back(a->cdlt); if (b2) hc.push_back(a->cdlt - b2->cdlt); }
                    const double md = cbr::medAbs(hd), mc = cbr::medAbs(hc); cb.dlX = hd.size() >= 10 && md > 0 ? (double)std::llabs(cb.dl) / md : 0; cb.chgX = hc.size() >= 10 && mc > 0 ? (double)std::llabs(cb.dl - cb.dlPrev) / mc : 0; } }
            // (0.15.0 / CombRead 1.1.0, the case review 15:50-16:03 CT) the pivot reads: the read candle and the 40 before it - prices in ticks, each candle's
            // heaviest buying / selling row from its volume at price; the time each candle is called by in the sentences
            if (cbBar >= 41) { cb.tick = S.tick; { int d = 0; double t = S.tick; while (d < 6 && std::fabs(t - std::round(t)) > 1e-9) { t *= 10.0; ++d; } cb.dec = d; }
                long long prevC = 0;
                for (int k2 = cbBar - 40; k2 <= cbBar; ++k2) { const rvs::Poc* p = pocAt(k2); cbr::In::PC q;
                    if (p) { q.ok = true; q.h = p->hi; q.l = p->lo; q.c = p->close; q.o = p->open > 0 ? p->open : (prevC > 0 ? prevC : p->close); q.dl = p->cdlt; q.bN = p->bNode; q.bD = p->bNodeD; q.sN = p->sNode; q.sD = p->sNodeD; prevC = p->close;
                        struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)times[k2], &t)) q.when = off::clock12(t.tm_hour, t.tm_min); }
                    cb.pc.push_back(q); } }
        }
        // (0.7.0) the six checks: the read shown = the decided Rev under the mouse, else the live pullback, else the newest decided Rev of the last 10 candles
        const rvs::SixRead* six = nullptr;
        { const int hb6 = S.hov.hover.bar; if (hb6 >= 0 && hb6 < (int)count) { const auto it = e.six.done.find((long long)times[hb6]); if (it != e.six.done.end()) six = &it->second; }
          if (!six && e.haveLive) six = &e.live;
          if (!six && !e.six.done.empty() && (long long)times[(int)count - 1] - e.six.done.rbegin()->first <= 10LL * (long long)S.spb) six = &e.six.done.rbegin()->second; }
        // a white ratio over the volume bar of a big order that passed its check (the big buyer at the pivot, the big seller on the Rev candle)
        { setFont(fb); auto ratio = [&](long long candle, bool buyBar, double x) { for (const rvs::BSBar& b : B) { if (b.t != candle) continue; char t[24]; std::snprintf(t, sizeof t, "%.1fx", x); const int w = (int)getTextWidth(t, -1);
                    const int cx = buyBar ? b.x - b.w / 2 : b.x + 1 + b.w / 2; const int y = std::max((int)pane.top + LINE_H + 2, (buyBar ? (b.nB ? b.numYB : b.topB - 1) : (b.nS ? b.numYS : b.topS - 1)) - 13);
                    setTextColor(rvh::C_WHITE); RCT r; r.set(rvh::coord(cx - w / 2), rvh::coord(y), rvh::coord(cx - w / 2 + w + 1), rvh::coord(y + 12)); r.drawText(t, false, false); break; } };
          auto both = [&](const rvs::SixRead& r) { if (r.c[1] && r.bbC > 0) ratio(r.bbC, r.dir < 0, r.bbX); if (r.c[4]) ratio(r.tr, r.dir > 0, r.bsX); };
          if (!B.empty()) { for (auto it = e.six.done.lower_bound(B.front().t); it != e.six.done.end(); ++it) both(it->second); if (e.haveLive) both(e.live); } }
        // the title (top-left)
        int x = pane.left + 6; const int y = pane.top + 2, lim = pane.right - 6;
        // (0.17.1, Rassul 09:11 CT "lets get rid of the reversal read header") no title row: the version is in the status file and the settings description
        if (RV_SHOW_BOOKFLOW) { setFont(fb); for (const BfLab& L : bfLab) { setTextColor(L.side == 0 ? rvh::C_GREEN : rvh::C_RED); RCT r; r.set(rvh::coord(L.l), rvh::coord(L.t), rvh::coord(L.r), rvh::coord(L.b)); r.drawText(L.tx, false, false); } }
        // (0.9.0) the accumulation / distribution read: a column right of the last candle; the story / Rev read moves right of it
        int accShift = 0; std::string accRead;
        const acc::Zone* z = e.ac.current(); if (!z) z = e.ac.last();   /* (0.9.1) nothing open: the last zone, with its time and result */
        if (z) {
            std::string dec; if (z->tDec > 0) { struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)z->tDec, &t)) dec = off::clock12(t.tm_hour, t.tm_min); }
            std::vector<acc::Line> L = acc::readLines(*z, S.tick, (int)S.spb, dec);
            if (!L.empty() && (pane.bottom - pane.top - 2) / (int)L.size() < 12) L = acc::readCompact(*z, S.tick, (int)S.spb, dec);   /* (0.9.1) a low pane: 3 lines */
            for (size_t i = 0; i < L.size(); ++i) accRead += (i ? " > " : "") + L[i].text;
            const int y0 = pane.top + 2, room = pane.bottom - y0; const int rowH = L.empty() ? 17 : std::min(17, room / (int)L.size());
            if (rowH >= 10) { setFont(rowH >= 16 ? fl : fb); int wmax = 0; for (const acc::Line& l : L) wmax = std::max(wmax, (int)getTextWidth(l.text.c_str(), -1));
                PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter); const int xr = (int)pl.h + (int)bw / 2 + 18;
                if (ACC_READ_DRAWN && v1 >= (int)count - 1 && xr + wmax <= pane.right - 6) {
                    for (size_t i = 0; i < L.size(); ++i) { const COLOR c = L[i].col == 3 ? (COLOR)acc::C_AMBER : L[i].col == 2 ? rvh::C_WHITE : L[i].col > 0 ? rvh::C_GREEN : rvh::C_RED; put(L[i].text, xr, y0 + (int)i * rowH, c, pane.right - 5); }
                    accShift = wmax + 26; } }
        }
        std::string read; int blockR = 0;   // (0.12.0) blockR = the right end of the story drawn right of the last candle (0 = not there)
        // (0.13.3, Rassul 2026-11-17 11:20 CT "the left was supposed to have the read and the right the data points") with the Book Flow read on screen the
        // pane is ONE split read: left = verdict + sentences (the story / Rev checks become its last sentence), right = the Bids / Offers table. The story
        // block is not drawn on its own any more; it was taking the room and the table was dropped.
        const bool splitOn = flowOk && v1 >= (int)count - 1; std::string storySent;
        auto noNum = [](const std::string& t) { const size_t d = t.find(". "); return d != std::string::npos && d <= 3 ? t.substr(d + 2) : t; };
        if (!e.note.empty()) { setFont(fp); put(e.note, x, y, rvh::C_GRAY, lim); read = e.note; }
        else if (!e.eng) { const std::string fs = S.feed.stateText(off::steadyMs()); const std::string t = fs != "ready" && !fs.empty() ? "no order feed - " + fs : "waiting for orders"; setFont(fp); put(t, x, y, rvh::C_GRAY, lim); read = t; }
        else {
            // (0.6.0) the read: numbered behaviour steps for the candle under the mouse (default: the newest story), in the empty space right of
            // the last candle; when the candles fill the pane, one line next to the title
            struct Row { std::string t; COLOR c; std::string t2; COLOR c2; }; std::vector<Row> R;
            auto timeOf = [&](long long sec) { struct tm t; std::memset(&t, 0, sizeof t); return getLocaltime((RTDATE)sec, &t) ? off::clock12(t.tm_hour, t.tm_min) : std::string(); };
            const int hb = S.hov.hover.bar; const rvs::Story* s = nullptr;
            if (hb >= 0 && hb < (int)count - 1) { const long long key = (long long)times[hb]; s = e.eng->storyAt(key, e.spb);
                { Row r; r.t = timeOf(key); r.c = rvh::C_WHITE; R.push_back(r); }
                const auto it = e.ps.poc.find(key); if (it != e.ps.poc.end() && it->second.ok) { Row r; r.t = "Buy " + std::to_string(it->second.buy); r.c = rvh::C_GREEN; r.t2 = "Sell " + std::to_string(it->second.sell); r.c2 = rvh::C_RED; R.push_back(r); } }
            else { s = e.eng->newest(e.stamp * 1000); if (s) { Row r; r.t = timeOf(s->t0 / 1000); r.c = rvh::C_WHITE; R.push_back(r); } }
            if (s) { const std::vector<rvs::Seg> L = e.eng->readSteps(*s);
                for (const rvs::Seg& g : L) { Row r; r.t = g.text; r.c = g.col > 1 ? rvh::C_WHITE : g.col > 0 ? rvh::C_GREEN : g.col < 0 ? rvh::C_RED : rvh::C_GRAY; R.push_back(r); } }
            for (size_t i = 0; i < R.size(); ++i) read += (i ? " > " : "") + R[i].t + (R[i].t2.empty() ? std::string() : " " + R[i].t2);
            if (six && splitOn) {                                        // (0.13.3) the Rev checks as one sentence: what passed, how many of six
                const std::vector<rvs::SixLine> L = rvs::sixLines(*six); read.clear(); int okN = 0, nN = 0; std::string pass;
                for (size_t i = 0; i < L.size(); ++i) { read += (i ? " > " : "") + L[i].text + (L[i].ok == 1 ? " +" : L[i].ok == 0 ? " -" : "");
                    if (L[i].ok < 0) continue;
                    ++nN;
                    if (L[i].ok == 1) { ++okN; pass += (pass.empty() ? "" : ", ") + noNum(L[i].text); } }
                storySent = (L.empty() ? std::string("Rev") : L[0].text) + ": " + std::to_string(okN) + " of " + std::to_string(nN) + " checks" + (pass.empty() ? std::string(".") : " - " + pass + ".");
            }
            else if (six) {                                              // (0.7.0) "Rev N" + six lines, a green check / red x in front of each
                const std::vector<rvs::SixLine> L = rvs::sixLines(*six); read.clear(); for (size_t i = 0; i < L.size(); ++i) read += (i ? " > " : "") + L[i].text + (L[i].ok == 1 ? " +" : L[i].ok == 0 ? " -" : "");
                setFont(fl); int wmax = 0; for (const rvs::SixLine& l : L) wmax = std::max(wmax, (int)getTextWidth(l.text.c_str(), -1) + 18);
                PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter); const int xr = (int)pl.h + (int)bw / 2 + 18 + accShift; const int rowH = 17, y0 = pane.top + LINE_H + 6;
                if (v1 >= (int)count - 1 && xr + wmax <= lim && y0 + (int)L.size() * rowH <= pane.bottom) {
                    for (size_t i = 0; i < L.size(); ++i) { const int yy = y0 + (int)i * rowH; const rvs::SixLine& l = L[i];
                        if (l.ok < 0) { put(l.text, xr, yy, rvh::C_WHITE, lim + 1); continue; }
                        const COLOR mc = l.ok ? rvh::C_GREEN : rvh::C_RED; setPen(mc, 2, P_SOLID);
                        auto seg = [&](int x0, int y0_, int x1, int y1) { PNT a; a.set(0, 0.0f); a.h = rvh::coord(x0); a.v = rvh::coord(y0_); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = rvh::coord(x1); b.v = rvh::coord(y1); b.drawLineTo(); };
                        if (l.ok) { seg(xr, yy + 7, xr + 3, yy + 11); seg(xr + 3, yy + 11, xr + 10, yy + 3); } else { seg(xr + 1, yy + 3, xr + 9, yy + 11); seg(xr + 9, yy + 3, xr + 1, yy + 11); }
                        put(l.text, xr + 16, yy, l.ok ? rvh::C_WHITE : rvh::C_GRAY, lim + 1); } }
                else if (STORY_TOP_LINE) { setFont(fb); for (size_t i = 0; i < L.size(); ++i) { const COLOR c = L[i].ok < 0 ? rvh::C_WHITE : L[i].ok ? rvh::C_GREEN : rvh::C_GRAY; const int w = put(L[i].text, x, y, c, lim); if (w < 0) break; x += w + 8; } }
            }
            else if (R.empty()) read = "no story";
            else if (splitOn) {                                          // (0.13.3) the story as the read's last sentence: "Story 11:07 AM: buyers climax 2.5x, absorbed ..."
                std::string t; for (size_t i = 1; i < R.size(); ++i) { std::string w = noNum(R[i].t) + (R[i].t2.empty() ? std::string() : " " + R[i].t2);
                    if (!t.empty() && w.size() > 1 && std::isupper((unsigned char)w[0]) && std::islower((unsigned char)w[1])) w[0] = (char)std::tolower((unsigned char)w[0]);
                    t += (t.empty() ? "" : ", ") + w; }
                if (!t.empty()) storySent = "Story " + R[0].t + ": " + t + ".";
            }
            else {
                setFont(fl); int wmax = 0; for (const Row& r : R) wmax = std::max(wmax, (int)getTextWidth(r.t.c_str(), -1) + (r.t2.empty() ? 0 : 10 + (int)getTextWidth(r.t2.c_str(), -1)));
                PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter); const int xr = (int)pl.h + (int)bw / 2 + 18 + accShift; const int y0 = pane.top + 2;
                // (0.12.0, Rassul 06:03 CT "move the climax") the story sits in the empty space right of the last candle, from the pane's top; a low pane
                // gets the small font and 14 px rows so the steps still fit one under the other
                int rowH = 17; if (y0 + (int)R.size() * rowH > pane.bottom) { rowH = 14; setFont(fb); wmax = 0; for (const Row& r : R) wmax = std::max(wmax, (int)getTextWidth(r.t.c_str(), -1) + (r.t2.empty() ? 0 : 10 + (int)getTextWidth(r.t2.c_str(), -1))); }
                if (v1 >= (int)count - 1 && xr + wmax <= lim && y0 + (int)R.size() * rowH <= pane.bottom) {
                    for (size_t i = 0; i < R.size(); ++i) { const int yy = y0 + (int)i * rowH; const int w = put(R[i].t, xr, yy, R[i].c, lim + 1); if (w > 0 && !R[i].t2.empty()) put(R[i].t2, xr + w + 10, yy, R[i].c2, lim + 1); }
                    blockR = xr + wmax; }
                else if (STORY_TOP_LINE) { setFont(fb);   // (0.18.3) off: no story line over the letter strip
                    for (size_t i = 0; i < R.size(); ++i) { if (i) { const int w = put(">", x, y, rvh::C_GRAY, lim); if (w < 0) break; x += w + 5; }
                        int w = put(R[i].t, x, y, R[i].c, lim); if (w < 0) break; x += w + 5; if (!R[i].t2.empty()) { w = put(R[i].t2, x, y, R[i].c2, lim); if (w < 0) break; x += w + 5; } } }
            }
        }
        (void)flowOk;
        if ((cb.bookOk || cb.deltaOk) && cbBar >= 0 && v1 >= (int)count - 1) {   // (0.15.0, Rassul 15:31 CT "1 effective read") THE ONE READ: Book Flow (passive) + delta (aggressive)
            const std::string mk = rvh::marketOf(getRootSymbol() ? getRootSymbol() : "", getSymbol() ? getSymbol() : ""); cb.mkt = mk;
            std::string when; { struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)times[cbBar], &t)) when = off::clock12(t.tm_hour, t.tm_min); } cb.when = when;
            cbr::Out fo = cbr::toPlain(cb, cbr::read(cb));   // (0.16.0, Rassul 21:15-21:17 CT "more human readable and simplified", "lets do this") three plain lines, no table
            {   // (0.17.0) a lettered candle reads its Wyckoff step: the candle under the mouse, or (no mouse) the newest letter of the last 3 candles
                int wb = -1; const int hbW = S.hov.hover.bar;
                if (hbW >= 0 && hbW < (int)count - 1) { if (e.wy.reads.count((long long)times[hbW])) wb = hbW; }
                else for (int k = (int)count - 2; k >= 0 && k >= (int)count - 4; --k) if (e.wy.reads.count((long long)times[k])) { wb = k; break; }
                if (wb >= 0) { const wyr::Read& wr = e.wy.reads.at((long long)times[wb]); fo = cbr::Out(); fo.head = wr.head; fo.verdict = wr.dir > 0 ? 1 : wr.dir < 0 ? -1 : 0;
                    for (const std::string& l : wr.lines) if (!l.empty()) fo.ex.push_back(l);
                    struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)times[wb], &t)) when = off::clock12(t.tm_hour, t.tm_min); storySent.clear(); } }
            setFont(fb); const int rowH = 14, y0 = pane.top + 2; PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter);
            const int xT = blockR > 0 ? blockR + 26 : (int)pl.h + (int)bw / 2 + 18;
            auto tw = [&](const std::string& q) { return (int)getTextWidth(q.c_str(), -1); };
            int wl = 0, wa = 0, wb = 0; for (const cbr::Row& r : fo.rows) { wl = std::max(wl, tw(r.lab)); wa = std::max(wa, tw(r.a)); wb = std::max(wb, tw(r.b)); }
            const int headW = (when.empty() ? 0 : tw(when) + 8) + tw(fo.head) + (fo.why.empty() ? 0 : 6 + tw(fo.why));
            const int dataW = fo.rows.empty() ? 0 : wl + 12 + wa + 14 + wb;
            const int fit = (pane.bottom - y0) / rowH;   // the read runs down the whole pane, over both halves
            const spt::Cols cl = spt::cols(lim - xT, dataW, headW);
            if (cl.leftW > 0 && fit >= 1) {
                if (blockR > 0) { setPen(rvh::C_GRAY, 1, P_SOLID); PNT a; a.set(0, 0.0f); a.h = rvh::coord(xT - 13); a.v = rvh::coord(y0 + 2); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = rvh::coord(xT - 13); b.v = rvh::coord(std::min((int)pane.bottom - 3, y0 + fit * rowH)); b.drawLineTo(); }
                const COLOR vc = fo.verdict == 1 ? rvh::C_GREEN : fo.verdict == -1 ? rvh::C_RED : fo.verdict == 2 ? rvh::C_WHITE : rvh::C_GRAY;
                const std::string lead = when.empty() ? fo.head : when + " " + fo.head;
                const std::vector<std::string> HL = spt::headLines(lead, fo.why, cl.leftW, std::max(1, std::min(2, fit - 1)), tw); const int nh = (int)HL.size();
                for (int i = 0; i < nh; ++i) { const int yy = y0 + i * rowH; const std::string& h = HL[(size_t)i];
                    if (i == 0 && h.compare(0, lead.size(), lead) == 0) { int xx = xT;
                        if (!when.empty()) { put(when, xx, yy, rvh::C_GRAY, lim + 1); xx += tw(when + " "); }
                        put(fo.head, xx, yy, vc, lim + 1); xx += tw(fo.head + " ");
                        if (h.size() > lead.size() + 1) put(h.substr(lead.size() + 1), xx, yy, rvh::C_WHITE, lim + 1); }
                    else put(h, xT, yy, rvh::C_WHITE, lim + 1); }
                std::vector<std::string> SS = fo.ex; if (!storySent.empty() && storySent.compare(0, 6, "Story ") != 0) SS.push_back(storySent);   // (0.16.0) the Rev checks stay; the long climax story sentence goes   // the story / Rev checks last
                const std::vector<std::string> EX = spt::wrap(SS, cl.leftW, fit - nh, tw);
                for (size_t i = 0; i < EX.size(); ++i) put(EX[i], xT, y0 + (int)(i + (size_t)nh) * rowH, rvh::C_WHITE, lim + 1);
                if (cl.dataOn && fit >= 2) {
                    const int xb = xT + cl.leftW + spt::GAP, c1 = xb + wl + 12, c2 = c1 + wa + 14;
                    const int nRows = std::min(fit, (int)fo.rows.size());
                    setPen(rvh::C_GRAY, 1, P_SOLID); { PNT a; a.set(0, 0.0f); a.h = rvh::coord(xb - 13); a.v = rvh::coord(y0 + 2); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = rvh::coord(xb - 13); b.v = rvh::coord(std::min((int)pane.bottom - 3, y0 + nRows * rowH)); b.drawLineTo(); }
                    auto colOf = [](int c, bool head) -> COLOR { return c > 0 ? rvh::C_GREEN : c < 0 ? rvh::C_RED : head ? rvh::C_WHITE : rvh::C_WHITE; };
                    for (int i = 0; i < nRows; ++i) { const cbr::Row& r = fo.rows[(size_t)i]; const int yy = y0 + i * rowH;
                        if (!r.lab.empty()) put(r.lab, xb, yy, rvh::C_GRAY, lim + 1);
                        put(r.a, c1, yy, colOf(r.ca, r.head), lim + 1); put(r.b, c2, yy, colOf(r.cb, r.head), lim + 1); }
                }
            }
            read += " || read " + when + ": " + fo.head + " " + fo.why;
        }
        if (!accRead.empty()) read = accRead + " || " + read;
        e.lastRead = read; ++e.drawn;
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<RevRead*>(this)->stopAll(); static_cast<RevRead*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<RevRead*>(this)->stopAll(); static_cast<RevRead*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void)   { return RTX_OK; }
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); static_cast<RevRead*>(this)->rvCalc(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    RevRead* p = new RevRead();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);      // its own pane, draws itself; (0.6.0) the mouse picks the candle the read is about; IRT's volume at price
    p->setExtendedFlags(CALL_CONTINUOUSLY);                              // the order feed keeps coming while no trade prints
    p->setDescription("LRA Reversal Read 0.18.4 - a letter strip at the top: E lack of demand / supply, A effort without result, Sp spring, Ut upthrust, I the other side took over (green = buyers / a low, red = sellers / a top); then Book Flow (resting orders added - pulled: bids green, offers red); bottom: delta (bright) and its change from the candle before (dim), green = buyers, red = sellers, a ratio over a change of 1.33x+ normal; one read right of the last candle combines both. Earlier: two bars per candle = its buy volume (green) and sell volume (red); a number on a bar = the biggest in that many candles (10 or more); letters C climax (red = selling, green = buying), A absorbed, E exhausted, I initiation; the read (right of the last candle) = the six checks of the newest pullback / Rev (Rev N: buying fading, big buyer trapped, quick rejection, bids pulled, big seller, with trend), else the reversal behaviours of the candle under the mouse. Market-by-order feed.");
    rvh::stamp(p);   // RVS_VERSION
    return p;
}

#else
// =====================================================================================================================
// lsRevReadMarks - the price-chart overlay: a white dash on each candle at its POC price
// =====================================================================================================================
// (0.18.2, Rassul 2026-11-18 19:58 CT "remove the show poc option from the reversal read marks", "it should only show the I", "there should
// be no options") lsRevReadMarks draws ONLY the Reversal Read I on the candle; no settings window, no POC dash, no Cx, no hold dots
inline long long rmSteadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct RmState { std::map<long long, std::vector<wyi::M> > iMarks; std::string wySig;   // (0.19.0) every candle mark (I / Bk / Sw / Cx with its level)
    long long wyCk = -100000; int wyRead = -1; long long iDrawn = 0; };   // the I letters (candle stamp -> +1 / -1) from RevRead\\wy-<MKT>-<secs>.txt, looked at every 3 s, re-read only when its size / time changed
class RevReadMarks : public cppExtension {
public:
    HostSlot<RmState> slot_;
    // the I letters the pane published for this market and candle size (bounded 1 MB read; a torn / foreign file keeps the last good set)
    void pump() {
        RmState* sp = slot_.get(this, true); if (!sp) return; RmState& s = *sp;
        const long long now = rmSteadyMs(); if (now - s.wyCk < 3000 && now >= s.wyCk) return; s.wyCk = now;
        const char* rs = getRootSymbol(); const char* sy = getSymbol(); const std::string mk = rvh::marketOf(rs ? rs : "", sy ? sy : ""); const long spb = (long)getSecondsPerBar();
        const std::string flex = rvh::flexDir(); if (flex.empty() || mk.empty() || spb <= 0) return;
        const std::string path = rvh::recDir(flex) + "\\wy-" + mk + "-" + std::to_string((long long)spb) + ".txt";
        struct stat st; if (stat(path.c_str(), &st) != 0) { if (!s.wySig.empty()) { s.wySig.clear(); s.iMarks.clear(); } s.wyRead = -1; return; }
        const std::string sig = path + ":" + std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime); if (sig == s.wySig) return;
        if ((size_t)st.st_size > wyi::MAX_BYTES) { s.wyRead = -2; return; }
        std::ifstream f(path.c_str(), std::ios::binary); if (!f) return;
        const std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::map<long long, std::vector<wyi::M> > m; const int n = wyi::parseAll(txt, m); if (n < 0) return;
        s.iMarks.swap(m); s.wySig = sig; s.wyRead = n;
    }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        RmState* sp = slot_.get(this, false); if (!sp) return RTX_OK; RmState& s = *sp;
        try {
            if (s.iMarks.empty()) return RTX_OK;
            const long count = getBarCount(); if (count < 2) return RTX_OK;
            RTARRAYI times(barDateTime); if (times.count < count) return RTX_OK;
            int v0 = 0, v1 = 0; if (getVisibleBars(&v0, &v1) != RTX_OK) return RTX_OK; v0 = std::max(0, v0); v1 = std::min((int)count - 1, v1);
            RCT pane; pane.getPaneRect(false);
            RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
            if (op.count < count || hi.count < count || lo.count < count || cl.count < count) return RTX_OK;
            // (0.18.0, Rassul 17:06 CT) the I on the candle: OVER a green candle, UNDER a red one; green tag = bullish, red = bearish; decided marks only (never repaints)
            // (0.19.0, Rassul 2026-11-19 10:20-10:40 CT, mockup level-marks-mockup.html v8, "go") + Bk / Sw / Cx; each tag = the code (bold) and ONE short
            // level name on a 2nd line; the I first, the level mark further out; no tag covers a candle or another tag (WyMarks layout); a short dashed
            // line at the level price; nothing right of the last bar
            FONT f1; f1.id = HELVETICA; f1.size = 10; f1.style = BOLD; FONT f2; f2.id = HELVETICA; f2.size = 8; f2.style = PLAIN;
            std::vector<wyi::Candle> C; std::vector<int> barOf; std::vector<wyi::Want> W; std::vector<const wyi::M*> WM; std::vector<std::string> l1, l2, l3;
            int xLast = 0; { PNT pl; pl.set(std::min(v1, (int)count - 1), 0.0f, kBarCenter); xLast = (int)pl.h + 6; }
            int dx = 8; if (v1 > v0) { PNT a; a.set(v0, 0.0f, kBarCenter); PNT b; b.set(v0 + 1, 0.0f, kBarCenter); dx = std::max(2, (int)b.h - (int)a.h); }
            for (int i = v0; i <= v1; ++i) {
                const double o = (double)op[i], h = (double)hi[i], l = (double)lo[i], c = (double)cl[i]; if (!std::isfinite(o) || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c)) continue;
                PNT p; p.set(i, 0.0f, kBarCenter); wyi::Candle cd; cd.x = (int)p.h; cd.yHigh = (int)getVerticalDrawPosition((float)h); cd.yLow = (int)getVerticalDrawPosition((float)l); cd.halfW = std::max(1, dx * 32 / 100); cd.green = c >= o;
                C.push_back(cd); barOf.push_back(i);
                const auto it = s.iMarks.find((long long)times[i]); if (it == s.iMarks.end()) continue;
                for (const wyi::M& m : it->second) {
                    const std::string a1 = wyi::textOf(m.code), a2 = m.lv, a3 = m.code == 'C' ? std::string() : m.st; setFont(f1); int w = (int)getTextWidth(a1.c_str(), -1); if (!a2.empty()) { setFont(f2); w = std::max(w, (int)getTextWidth(a2.c_str(), -1)); }
                    if (!a3.empty()) { setFont(f2); w = std::max(w, (int)getTextWidth(a3.c_str(), -1)); }   // (0.19.2) the strength: a 3rd line, same font as the level
                    wyi::Want wt; wt.candle = (int)C.size() - 1; wt.w = w + 2 * wyi::PAD_X; wt.h = wyi::LINE1_H + (a2.empty() ? 0 : wyi::LINE2_H) + (a3.empty() ? 0 : wyi::LINE2_H); W.push_back(wt); WM.push_back(&m); l1.push_back(a1); l2.push_back(a2); l3.push_back(a3); } }
            if (W.empty()) return RTX_OK;
            // (0.19.0, Rassul 10:51 CT "remember placement of other signals like cmx so you have to coordinate everything") the BO indicator's letters
            // as drawn (SignalBoard 'L': candle, side, px from the extreme to the letter's outer edge) are taken boxes: our tags go around them
            std::vector<wyi::Box> taken;
            try { const char* sy = getSymbol(); const char* chl = getChartLabel(); std::vector<sb::Ev> ev;
                // (0.19.1, Rassul 11:13 CT "ok": lsZzoComboMarks 'Z' (SR / CB35 / CB99) and lsCobSignal 'K' (Cob..., Lvp, Rev) publish their labels as drawn too)
                for (const char kind : { 'L', 'Z', 'K' }) { ev.clear();
                if (sb::readAll(kind, sy ? sy : "", getSecondsPerBar(), chl ? chl : "", rmSteadyMs(), ev) && !ev.empty()) {
                    std::map<long long, size_t> at; for (size_t q = 0; q < barOf.size(); ++q) { struct tm m; std::memset(&m, 0, sizeof m); if (!getLocaltime((RTDATE)times[barOf[q]], &m)) continue;
                        at[wyi::civilDays(m.tm_year + 1900, (unsigned)(m.tm_mon + 1), (unsigned)m.tm_mday) * 86400LL + m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec] = q; }
                    for (const sb::Ev& x : ev) { const auto it = at.find(x.bar); if (it == at.end() || !(x.val > 0) || x.val > 400) continue; const wyi::Candle& cd = C[it->second];
                        wyi::Box b; sb::tagBox(x, cd.x, cd.yHigh, cd.yLow, b.l, b.t, b.r, b.b); taken.push_back(b); } } } } catch (...) {}
            const std::vector<wyi::Box> P = wyi::layout(C, W, (int)pane.top, (int)pane.bottom, xLast, taken);
            double rmTick_ = (double)getProperty(SYM_TICKINCR); if (!std::isfinite(rmTick_) || rmTick_ <= 0 || rmTick_ >= 1000) rmTick_ = 0;   // the only tick source
            for (size_t k = 0; k < W.size(); ++k) { const wyi::Box& g = P[k]; if (!g.ok) continue; const wyi::M& m = *WM[k];
                const COLOR col = m.dir > 0 ? rvh::C_GREEN : rvh::C_RED;
                RCT r; r.set(rvh::coord(g.l), rvh::coord(g.t), rvh::coord(g.r), rvh::coord(g.b)); r.draw(0, col, col, DRAW_OPAQUE, PAT_SOLID);
                setTextColor(rvh::C_TAG_TEXT); setFont(f1); RCT t1; t1.set(rvh::coord(g.l), rvh::coord(g.t), rvh::coord(g.r), rvh::coord(g.t + wyi::LINE1_H)); t1.drawText(l1[k].c_str(), true, false);
                int yNext = g.t + wyi::LINE1_H - 1;
                if (!l2[k].empty()) { setFont(f2); RCT t2; t2.set(rvh::coord(g.l), rvh::coord(yNext), rvh::coord(g.r), rvh::coord(yNext + wyi::LINE2_H)); t2.drawText(l2[k].c_str(), true, false); yNext += wyi::LINE2_H; }
                if (!l3[k].empty()) { setFont(f2); RCT t3; t3.set(rvh::coord(g.l), rvh::coord(yNext), rvh::coord(g.r), rvh::coord(std::min((int)g.b, yNext + wyi::LINE2_H))); t3.drawText(l3[k].c_str(), true, false); }   // (0.19.2) the strength
                if (m.code == 'I') ++s.iDrawn;
                if (m.px > 0 && rmTick_ > 0) {   // the level: a short dashed line from 1.5 candles left to 2.5 candles right of the candle
                    const int y = (int)getVerticalDrawPosition((float)((double)m.px * rmTick_)); if (y <= (int)pane.top || y >= (int)pane.bottom) continue;
                    const int x = C[(size_t)W[k].candle].x; const int xa = std::max((int)pane.left, x - dx * 3 / 2), xb = std::min(xLast, x + dx * 5 / 2);
                    setPen(rvh::C_LEVEL, 1, P_DASH); PNT a; a.set(0, 0.0f); a.h = rvh::coord(xa); a.v = rvh::coord(y); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = rvh::coord(xb); b.v = rvh::coord(y); b.drawLineTo(); } }
        } catch (...) {}
        return RTX_OK;
    }
};
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { static_cast<RevReadMarks*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void)   { return RTX_OK; }   // (0.18.2) no settings window
int cppExtension::calc(int)     { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); try { static_cast<RevReadMarks*>(this)->pump(); } catch (...) {} return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    RevReadMarks* p = new RevReadMarks();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | INSTRUMENT_SCALE | NO_UI);
    p->setDescription("LRA Reversal Read marks 0.19.1 - the Reversal Read I and Bk / Sw / Cx on the candles, each with its level: over a green candle, under a red one; green = bullish, red = bearish. Needs the Reversal Read pane on the same chart. No settings.");
    rvh::stamp(p);   // RVS_VERSION
    return p;
}
#endif
