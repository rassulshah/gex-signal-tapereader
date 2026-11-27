/*
 *  RevRead.cpp  --  Investor/RT RTX extension  lsRevRead  "Reversal Read"  0.24.0  (2026-11-23 14:18 CT: LevelMarks 1.14.0 - a Sw only on a major level, the 3-hour S / R as mark levels, the AR's ATR way; was 0.23.0  2026-11-23 09:34 CT: LevelMarks 1.13.0 / WyMarks 2.5.0 - the AR bar after a Cx; 0.22.1 2026-11-21 05:51 CT: LevelMarks 1.12.0 - no Sw on a data-release candle; 0.22.0 05:40 CT: LevelMarks 1.11.0 - no Sw on the RTH opening candle; 0.21.8 2026-11-20, 20:3x CT: LevelMarks 1.10.1 - a close ON the level before the attempt is no side, no Sw; 0.21.7 Rassul 16:44 CT "yes, use strict rule": LevelMarks 1.10.0 - a Sw needs a level nobody traded through (wicks count), labelled with the key level its candle crossed; 0.21.6 Rassul 14:20-14:26 CT "the 9:09 was a lonHi", "the sw candle clearly swept the Phrh key level": LevelMarks 1.9.0 - session levels counted from the candle that printed them, a poke that closes back leaves the level, combined names, labels 1hH / 1hL / PhrH / PhrL; 0.21.5 Rassul 13:15 CT "lets go with this": Book Flow back on top - bids / offers net, solid adding, hollow pulling, 2x+ normal labels - and the delta CHANGE alone below; 0.21.4 Rassul 11:58 CT: level marks without the order feed; 0.21.3 10:35 / 10:37 CT: Sw and I judged by their own candle at 20+, the I on named key levels only; 0.21.2 09:20 / 09:24 CT: the I is back - price badge I / level / strength and the strip letter; a candle that is a Sw and an I shows only the Sw; see RVS_VERSION for every version; 0.11.1: 2026-11-16: ratio = bigger side / smaller; 0.11.0: 2026-11-16: the bars are Book Flow; 0.10.1: 2026-11-16: AccumLogic 2.0.1; 0.10.0: 2026-11-16: accumulation / distribution = the simple rule, AccumLogic 2.0.0; 0.9.2: 2026-11-16: bars in a low pane; 0.9.1: 2026-11-16: the accumulation read is always shown (the last zone), + the quiet end and the other side building, AccumLogic 1.2.0; 0.9.0: accumulation / distribution zones + their read, AccumLogic.h; 0.8.0: lsSR runs in the background, SRBack.h; 0.7.0 2026-11-13)
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
#include "BsRead.h"            // (0.29.0) the buy / sell read
#include "WyRead.h"            // (0.17.0) the Wyckoff strip: E / A / Sp / Ut / I + their plain read
#include "WyMarks.h"           // (0.21.2) the pane writes the I lines with wyi::iLine (SDK-free, tests_wymarks / tests_rr0212)
#include "WyFreeze.h"          // (0.20.3) each finished session's wy lines, frozen (SDK-free, tests_wyfreeze)
#include <mutex>
#include <iterator>
#include "LevelMarks.h"
#include "HtfSr.h"             // (0.24.0) the 3-hour S / R walk (SDK-free) for LevelMarks 1.14.0
#include "ThrustDot.h"         // (0.44.0) the cyan thrust dot (SDK-free, tests_levelmarks/t_td100)
#include "EventCalLogic.h"     // (0.22.1) the scheduled releases (lsEventCal's list) - no Sw on a release candle        // (0.19.0) Bk / Sw / Cx + one level each, for the price candles (SDK-free, tests_levelmarks)
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>

static const double BF_RATIO_MIN = 1.2;
static const double BF_LABEL_X = 2.0;   // (0.21.5) a Book Flow label = the side's net 2x its normal or more   // (0.11.1) a Book Flow ratio label needs the bigger side 1.2x the smaller or more
static const bool ACC_READ_DRAWN = false;   // (0.12.0, Rassul 2026-11-17 06:03 CT "remove what is there currently now") the Accum / Distrib read is not drawn in the pane (the boxes on the price chart stay; the read stays in the status file)
static const bool RV_DRAW_BOOKFLOW = false;   // (0.29.0, Rassul 19:33 CT "i would also hide the book flow") hidden again; its numbers stay in the background
// was static const bool RV_DRAW_BOOKFLOW = true;   // (0.21.5, Rassul 13:15 CT "lets go with this") drawn again, on top   // was false   // (0.17.3, Rassul 09:29 CT "can we hide bookflow for now and just keep delta + change") the Book Flow half is not drawn - delta + change take the whole pane; Book Flow is still read in the background (the read, the Wyckoff springs)
static const bool STORY_TOP_LINE = false;   // (0.18.3, Rassul 2026-11-18 20:34 CT "there is a read in the signal strip for reversal that needs to be removed": when the story / Rev checks do not fit right of the last candle they are NOT written across the top (over the letter strip) any more
static const bool RV_SHOW_I = true;    // (0.21.2, Rassul 2026-11-20 09:20 CT "im thinking of adding I back since it considers levels it should be ok.. add it", 09:24 "build": the I is shown again - strip and price badge - named key levels only (0.19.4); a candle that is also a Sw shows only the Sw)   // was false   // (0.19.5, Rassul 2026-11-19 15:34 CT "i think we should table away I for now. sweep is enough": the I is not shown - not in the letter strip, not on the candles (the wy file carries no I lines, so lsRevReadMarks and lsSDMO show none); the Sw is the mark. Set true to bring it back
static const bool RV_SHOW_BOOKFLOW = true;   // (0.11.0) the pane's bars: true = Book Flow (resting orders added - pulled, bids / offers), false = the candle's buy / sell volume (0.6.0 - 0.10.x)
#define RVS_VERSION "0.44.1"   // (0.44.1, Rassul 2026-11-27 13:47 CT "add these requirements") LevelMarks 1.32.0: the BI also needs Thrust >= 1.20 and PIC >= 1.20
// was #define RVS_VERSION "0.44.0"   // (0.44.0, Rassul 2026-11-27 12:38-12:44 CT "a blue cyan dot on a green candle that has its low below the high of the down pivot and vice versa, that has its thrust and pic greater than or equal to 1.2", "in the range bar and in the candle in the middle", "1. yes 2. every 3. all 4. no repaint") ThrustDot 1.0.0: "stamp|TD|dir" lines; lsRevReadMarks draws the cyan dot in the middle of the candle body, lsRangeRead in the middle of its bar
// was #define RVS_VERSION "0.43.2"   // (0.43.2, Rassul 2026-11-27 10:29 CT "it needs to be a level that wasnt run") LevelMarks 1.31.2: a BI level untraded since it formed (Asia / London included)
// was #define RVS_VERSION "0.43.1"   // (0.43.1, Rassul 2026-11-27 10:19 CT) LevelMarks 1.31.1: a BI's high above the prior candle's high (bearish: low below the prior low)
// was #define RVS_VERSION "0.43.0"   // (0.43.0, Rassul 2026-11-27 09:50-10:00 CT "consolidate AR and Vob (t) into a new BI (Break In) signal", "its the green one after the lod", "go", "ok" = BVMax or BRMax 20+; BI-mockup.html) LevelMarks 1.31.0 / WyMarks 2.9.0: + the BI badge (BI / level / V or R strength); VOB / VOT and AR not drawn
// was #define RVS_VERSION "0.42.0"   // (0.42.0, Rassul 2026-11-26 17:21 CT) LevelMarks 1.30.0: Asia 6:00 PM - 2:00 AM CT (was 1:00 AM)
// was #define RVS_VERSION "0.41.0"   // (0.41.0, Rassul 2026-11-26 13:36-13:40 CT) LevelMarks 1.29.0: a Cx only at the HOD / LOD stats levels + weekly levels
// was #define RVS_VERSION "0.40.0"   // (0.40.0, Rassul 2026-11-26 13:06-13:16 CT) LevelMarks 1.28.0: a Cx only at a level the climax (and the pivot candle after it) did NOT close beyond
// was #define RVS_VERSION "0.39.0"   // (0.39.0, Rassul 2026-11-26 12:40 / 12:45 CT "cx ... requires a level", "A") LevelMarks 1.27.0: a Cx / Cx? only when a level is inside its candle
// was #define RVS_VERSION "0.38.0"   // (0.38.0, Rassul 2026-11-26 12:23 CT "why is this not a cmx" - ES 10:15 AM) LevelMarks 1.26.0: a sweep candle too light for a VOB / VOT (BVMax < 99) no longer blocks its Cx / Bk
// was RVS_VERSION "0.37.0"   // (0.37.0, Rassul 2026-11-25 16:46-16:59 CT "You know I'm looking for high volume rejection from an attempted break of a level", "In this case we should use 99+ bvmax", both sides (answered 17:00)) LevelMarks 1.25.0: VOB / VOT = BVMax 99+, both sides
// was RVS_VERSION "0.36.2"   // (0.36.2, Rassul 2026-11-25 16:10 CT "thats a problem .. lets just use BVMax 20+") LevelMarks 1.24.2: VOB / VOT strength = BVMax 20+ again
// was RVS_VERSION "0.36.1"   // (0.36.1, Rassul 2026-11-25 16:04 CT "the vob / vot strength requires BV 20+") LevelMarks 1.24.1: the VOB / VOT strength = BV (all candles), was BVMax
// was RVS_VERSION "0.36.0"   // (0.36.0, Rassul 2026-11-25 14:28-14:29 CT "relabel sweep to VOB (volume off the bottom) and VOT (Volume off the top)") the Sw mark is DRAWN as VOB (bullish, under a low) / VOT (bearish, at a high); the files keep the code "Sw"
// was RVS_VERSION "0.35.0"   // (0.35.0, Rassul 2026-11-25 13:56-14:00 CT "tighten strength to reuire the bigest volume in at least 20 bars", "go") LevelMarks 1.24.0: a Sw needs its own candle's BVMax >= 20 (volume only)
// was RVS_VERSION "0.34.0"   // (0.34.0, Rassul 2026-11-25 13:11 CT "bearish sweeps use highs and bullish sweeps use lows") LevelMarks 1.23.0
// was RVS_VERSION "0.33.0"   // (0.33.0, Rassul 2026-11-25 11:39 CT "so lets update the rule to current session") LevelMarks 1.22.0: in the day session a Sw needs its level untouched only since the RTH open (was: since the level formed) - GC 11:24 AM AsiaLO now a bearish Sw; overnight sweeps unchanged
// was "0.32.0"   // (0.32.0, Rassul 2026-11-25 08:39-08:49 CT) LevelMarks 1.21.0: Bk on the last completed 3-hour bar's high / low (3hH / 3hL); the break candle = volume OR range bigger than the candle before, every level
// was "0.31.0"   // (0.31.0, Rassul 2026-11-24 21:23-22:39 CT "i dont want repetitive information in the read", "everything else is a go" on reads-conclude-mockup.html v30): the buy / sell read concludes (BsRead 2.0.0) - who is in control, delta against price (trapped), strong side + delta flip, drying up
// was "0.30.0"   // (0.30.0, Rassul 2026-11-24 20:46-20:54 CT "add the question each answers to the read", "make the update for magnitude also so everything is covered", "go"): the buy / sell bars' DARK part = lots from big orders (S20+, OrderFlowFeed 1.18.0 Rec.bigC); the read's big-order line (BsRead 1.1.0) and a grey column with each line's volume-model question
// was #define RVS_VERSION "0.29.0"   // (0.29.0, Rassul 2026-11-24 19:31-19:44 CT "should this be updated to buy and sell volume instead", "i would also hide the book flow", "i would read the volume, then the buy sell vol, then the absorbtion volume", "Go" on panes-read-mockup.html) the pane: Book Flow hidden, the delta-change bars replaced by BUY / SELL VOLUME bars (both up from zero, "biggest in N" from 10), the read = who hit whom (BsRead.h); the signal strip and every mark unchanged
// was #define RVS_VERSION "0.28.1"   // (0.28.1, Rassul 2026-11-24 16:01 CT "bullish breaks are of highs and bearish breaks are of lows") LevelMarks 1.20.0: a Bk is drawn only through a level on its own side (a high for a bullish Bk, a low for a bearish one; Open / POC either way)
// was #define RVS_VERSION "0.28.0"   // (0.28.0, Rassul 2026-11-24 12:21 CT "use the third line which currently has the percentage .. show mockup", 12:51 "2. do the test", "3. yes"; tst-line3-mockup.html) LevelMarks 1.19.0 / WyMarks 2.8.0 / WyRead 1.11.0 / RevReadLogic 0.17.0 / lsTurn 1.6.0: the Tst badge's 3rd line = A+E / E / A! / blank (absorption and exhaustion at the Tst pivot) instead of the retrace %, decided once and kept in RevRead\\tstl3-<MKT>-<spb>.txt; the Abs badge's P line always shown once known
// was #define RVS_VERSION "0.27.0"   // (0.27.0, Rassul 2026-11-24 11:12 CT "ok. build this", abs-badge-mockup.html v15) LevelMarks 1.18.0 / WyMarks 2.7.0 / RevReadLogic 0.16.0: the Abs badge - effort without result at the first test of a Bk level (Abs / Low 21% or Top 21% / P16 from lsTurn), "Abs/Ex?" when lsTurn saw the attackers' last minute fade, and the next candle's "Ex" (attackers 0.5x or less, closed further away); written "stamp|Abs|..." / "stamp|Ex|dir" in wy-<MKT>-<spb>.txt
// was #define RVS_VERSION "0.26.0"   // (0.26.0, Rassul 2026-11-24 08:33-09:01 CT "go for hourly pivots and 4h high and lows", "go for bk with hourly pivots and 4 hour highs and lows") LevelMarks 1.17.0: the pane also walks the HOURLY PIVOTS (1-hour 1-bar High / Low ZigZag, standing R1 R2 S1 S2) and the last completed 4-HOUR bar's high / low over the chart's closed candles and hands them to the Bk (HpH / HpL / 4hH / 4hL, Bk only, biggest volume in 5+); Sw / Cx / I unchanged
// was #define RVS_VERSION "0.25.0"   // (0.25.0, Rassul 2026-11-23 23:18 CT "the CX came late .. why ?", 23:20 "go") LevelMarks 1.16.0 liveCx: the climax candle shows "Cx?" from its own close while it is the extreme of the running leg (RevRead\\wylive-<MKT>-<spb>.txt, drawn by lsRevReadMarks); the Cx is unchanged
// was #define RVS_VERSION "0.24.1"   // (0.24.1, Rassul 2026-11-23 19:54 CT "why isnt this a cmx ?", 20:16 "go") LevelMarks 1.15.0: the Cx look-back reaches into the 20 candles before the session (run() preV)
// was #define RVS_VERSION "0.24.0"   // (0.24.0, Rassul 2026-11-23 14:00-14:18 CT 'for SW signal it needs to cross a major level like a session or pd pf  weekly or 3h S1 S2 , R1 R2 level etc', 'no the bk and tst can also include hourly', 'for the AR badge ,   if the Green bar is bigger than the ATR and its high is above the cmx bar with the hevy volume , it is also valid AR', 'build': LevelMarks 1.14.0. The pane walks the 3-hour S / R (HtfSr.h - the lines lsHtfSR draws, the Limit Profile's levels) over the chart's closed candles and hands every session the levels standing before each candle; no new file, nothing read from another indicator)
// was "0.23.0"   // (0.23.0, Rassul 2026-11-23 09:25 CT 'i want you to implement the AR which is an automatic rally bar that occurs after a cx . so if there is a climax, and there is a bar that has its low below the pivot high and is within 3 bars of the pivot and has a BRMax of at least 10. so if the cx was in a downwave , the ARbar would have to be green', 09:26 'the AR bar should also have a strength line', 'here is a decent example' (NQ 11-23 6:51 AM, the green candle after the 6:48 pivot low; Cx 6:45), 09:34 'go'; mockup ar-mockup.html) the AR: LevelMarks 1.13.0 arCands (per session, next to seq) + arPick (after the strength lines), written to the wy file as stamp|AR|dir||0|strength and drawn by the marks half as a badge (WyMarks 2.5.0); every other reader of the file skips the new code
// was "0.22.1"   // (0.22.1, Rassul 2026-11-21 05:51 CT "we can skip data release for sweeps, they shouldnt qualify") LevelMarks 1.12.0: a candle holding a scheduled release (Event Calendar list) is never a Sw   // was "0.22.0"   // (0.22.0, Rassul 2026-11-21 05:40 CT "we need to remove opening candle as a candidate for sweeps") LevelMarks 1.11.0: the RTH opening candle is never a Sw   // was "0.21.8"   // (0.21.8) LevelMarks 1.10.1 - no Sw when the close before the attempt sits exactly on the level   // was "0.21.7"   // (0.21.7) LevelMarks 1.10.0   // was "0.21.6"   // (0.21.6) LevelMarks 1.9.0   // was "0.21.5"   // (0.21.5) the split pane: Book Flow on top, delta change below   // was "0.21.4"   // (0.21.4, Rassul 2026-11-20 11:54 / 11:58 CT "go": NG 8:03 Sw ONH was not drawn - with no order-by-order feed the engine never started and the level marks had no key levels) the key levels for Bk / Sw / I / Cx come from the level files when the order engine is not running; nothing else changes
// was "0.21.3"   // (0.21.3, Rassul 2026-11-20 10:34-10:37 CT "the red sweep bar doesnt meet the threshold.. fix", "I should only be on key levels that are named like overnight etc.. and we should have a 20 threshold just like sweeps"): a Sw needs ITS OWN candle at BVMax / BRMax 20+ and shows that candle's strength (LevelMarks 1.8.0); an I needs a session / PD / PF / volume / weekly level (no hour levels) and its own candle at 20+
// was "0.21.2"   // (0.21.2, Rassul 2026-11-20 09:20 CT "im thinking of adding I back since it considers levels it should be ok.. add it and also add the mark to the signals line in reversal indicator, add the badge for it also and include the level and strength line, if both sweep and i are true, only show sweep", 09:24 "build"; mockup i-badge-mockup.html): the I is back (RV_SHOW_I true; named key levels only, as 0.19.4) - on the price chart a badge like Bk / Sw (I / its ONE level by the Bk / Sw priority / strength V or R from 10 up), green UNDER a bullish candle, red OVER a bearish one (WyMarks 2.4.0); on the strip after Sp / Ut (WyRead 1.9.0: the strip's I / B / C come from the published wy lines, so strip and chart agree); a candle that is a Sw AND an I of the same direction shows only the Sw (badge + strip) - the I is written to the wy file with a 7th field "Sw" for the nightly (LevelMarks 1.7.0 iUnderSw)   // was "0.21.1"   // (0.21.1, Rassul 2026-11-20 07:50 CT "update the sweep threshold to 20": a Sw needs BVMax or BRMax >= 20 (was 10) - LevelMarks 1.6.0; the strength line still shows from 10 up)   // was "0.21.0"   // (0.21.0, Rassul 2026-11-20 05:52 CT "ok.. you decide and add it. you can also add for example b for break and c for climax etc."): the letter strip gets B (a Bk candle) and C (a Cx candle) from the price chart's marks, and T (trapped - measured: held 57-63% vs 52-56% for the same reclaim without the trap, ES / NQ / CL, both halves of the days) from WyRead 1.8.0 more(); P (pulled while hit) and X (exhaustion without pulling) are computed and journaled to RevRead\\wyx-<MKT>-<spb>.txt but not drawn (they did not beat their base). At most 2 letters per candle by priority Sp / Ut > T > P > A > X > E > B > C   // was "0.20.3"   // (0.20.3, Rassul 2026-11-19 ~18:35 CT: after 5 PM the earlier sessions lost their key-level marks) each session's wy lines are frozen to RevRead\wymarks-<MKT>-<spb>-<YYYYMMDD>.txt and read back unchanged (WyFreeze.h)   // was "0.20.2"   // (0.20.2, Rassul 2026-11-19 18:29 CT: a Sw needs BVMax or BRMax >= 10 - LevelMarks 1.5.0)   // was "0.20.1"   // (0.20.1, Rassul 2026-11-19 18:19 CT "switch to 10": the strength line on Bk / Sw / Rev badges from 10 up - LevelMarks 1.4.0)   // was "0.20.0"   // (0.20.0, Rassul 2026-11-19 15:35-16:58 CT, mockup price-pane-cleanup-mockup.html, "1. ok 2. ok 3. ok"): lsRevReadMarks draws, after a Bk, its FT (plain letter), Tst (badge: Tst / level / %) and Rev (badge + strength line) - LevelMarks 1.3.0 seq: the BO indicator's FT / Pb / PB Rev rules on the Bk; Bk needs the BO candle rule; a Sw is green (bullish) / red (bearish); no POC / Open levels; one Rev per candle (a Cob Rev from lsCobSignal wins); the reads say Sw / Tst instead of I (WyRead 1.7.1)   // was "0.19.5"   // (0.19.5, Rassul 2026-11-19 15:34 CT "i think we should table away I for now. sweep is enough": the I is hidden everywhere (RV_SHOW_I = false); the Sw on the price candles is the mark)   // was "0.19.4"   // (0.19.4, Rassul 2026-11-19 14:12-14:14 CT "I didnt mention a level", "I should not print if there is no level": an I is kept only when it reclaimed a NAMED key level (the Bk / Sw list: session, PD, PF, volume, weekly, hour levels) - no gamma / MenthorQ levels, no VWAP bands (they move: an I matched to the band's value now was a false I)   // was "0.19.3"   // (0.19.3, Rassul 2026-11-19 13:29 CT "its hard for me to see the white lines so lets make them black instead so bk and other badges will have black writing instead": the candle tags (I / Bk / Sw / Cx - code, level, strength) are written in BLACK on the bright green / red fill)   // was "0.19.2"   // (0.19.2, Rassul 2026-11-19 11:22-11:35 CT "sweeps of key levels on big volume or range are very meaningfull", "consider sw and a second line like v50 or r50, same with breaks", "use the max numbers that we have ... biggest in green bars, red bars, consecutive", "ok.. good build it", "will you add this strength line to the I also": Bk / Sw / I tags get a 3rd line = his BVMax / BRMax (the Volume / Range panes' Max number) "V<N>" / "R<N>" from 20 up - LevelMarks 1.2.0; the letters file carries it as a 6th field)   // was "0.19.1"   // (0.19.1, Rassul 11:13 CT "ok": the marks also keep clear of lsZzoComboMarks (SR / CB35 / CB99) and lsCobSignal (Cob, Lvp, Rev) labels - SignalBoard 1.6.0 kinds Z / K)   // was "0.19.0"   // (0.19.0, Rassul 2026-11-19 10:20-10:40 CT, mockup level-marks-mockup.html v8, "go"): lsRevReadMarks draws Bk / Sw / Cx on the candles too (LevelMarks.h 1.0.0), each with ONE short level name on a 2nd line (session > PD > PF > volume > weekly > hours), and the I with its level (WyRead 1.7.0); tags never overlap (WyMarks 2.0.0 layout); a short dashed line at the level; repeats cut; the letters file carries the level (stamp|letter|dir|level|px)   // was "0.18.5"   // (0.18.5, Rassul 2026-11-19 08:41 CT "lets include this ... a sweep of some htf level, in this case it was an hourly level. build": WyRead 1.2.0 - the prior 3 hours' highs / lows are levels the I can reclaim (the failed HTF breakout))   // was "0.18.4"   // (0.18.4, Rassul 22:33-22:37 CT "modify the I so it is a reclaim of a key level, volume level or a local level", "go": WyRead 1.1.0 - the I closes back through a level its pivot swept; the key / volume level files feed it for the current session)   // was "0.18.3"   // (0.18.3, Rassul 20:34 CT: no story / Rev-check line written over the letter strip when the chart is scrolled to the right edge)   // was "0.18.2"   // (0.18.2, Rassul 19:58 CT "remove the show poc option", "it should only show the I", "there should be no options": lsRevReadMarks draws only the I on the candle - no settings window, POC dash, Cx or hold dots)   // was "0.18.1"   // (0.18.1, Rassul 19:40 CT "all of the symbols are gone like A E I from the strip and the I is not displayed on the candles": the letters of the current session and the 3 before it, each session read by itself - at 5 PM only the new session was kept)   // was "0.18.0"   // (0.18.0, Rassul 2026-11-18 17:06 CT "I also want the I indicator on the candle, put it on top of green candle and below red candles", 17:11 "go" on i-on-candles.html: lsRevReadMarks draws each I of the letters file on its price candle - a small tag, green = bullish I / red = bearish I, white letter - OVER a green candle, UNDER a red one, one row further out when a Cx is on that side; WyMarks.h 1.0.0; A / E / Sp / Ut stay in the pane)   // was "0.17.5"   // (0.17.5, Rassul 13:08 CT: the session letters published to RevRead\\wy-<MKT>-<secs>.txt for the SDMO pane's I letters)   // was "0.17.4"   // (0.17.4, Rassul 09:53 CT "make the signal strip have a dark gray background": a dark gray band behind the letter strip)   // was "0.17.3"   // (0.17.3, Rassul 09:29 CT "can we hide bookflow for now and just keep delta + change": the Book Flow half is hidden, delta + change fill the pane; the old story letters on the Book Flow bars go with it)   // was "0.17.2"   // (0.17.2, Rassul 09:21 CT "a bullish I should have its low below the high of the dnpivot and vice versa": WyRead 1.0.1)   // was "0.17.1"   // (0.17.1, Rassul 09:11 CT "get rid of the reversal read header and make the letters smaller and use 1 line instead of two, if there are multiple letters ... make them smaller"): no title row; the strip is one row at the top, 9 pt, 7 pt side by side when a candle has 2+ letters   // was "0.17.0"   // (0.17.0, 2026-11-18 Rassul 05:45-08:15 CT "filter based on a wyckoff perspective", "lets add the letters on a strip on the top of the pane", "use Sp and Ut", "get rid of the ? marks", "dont put anything ont he candle chart", "lets build this"): a letter strip at the top of the pane - E lack of demand / supply, A effort without result, Sp spring / Ut upthrust (absorbed / flush, stop run, no follow-through), I the other side took over (WyRead.h 1.0.0); the read on the right says the Wyckoff step of a lettered candle   // was "0.16.0"   // (0.16.0, 2026-11-17 Rassul 21:15-21:17 CT "i wnat a more human readable and simplified format for these reads ... i need something practical", "i like it. lets do this"): the read = 3 plain lines (who is winning / why / what to watch, CombRead.h 1.2.0 toPlain), no numbers table; the Rev checks stay   // was "0.15.3"   // (0.15.3, Rassul 18:06 CT "lets go with 1.33x": the delta-change ratio label at 1.33x+ normal, was 2x)   // was "0.15.2"   // (0.15.2, his 17:58 CT screenshot "the delta is not drawing on the new bar", "make the delta pane on the top instead": delta + change on top, Book Flow below; the forming candle's delta drawn live; each session scaled by itself - the evening bars were too small to see; no ratio over the title)   // was "0.15.1"   // (0.15.1, Rassul 16:03 CT "go" after the case review: the pivot reads lead the one read - absorption, trapped, rejection wick, breakout met - CombRead 1.1.0)   // was "0.15.0"   // (0.15.0, Rassul 2026-11-17 15:24-15:41 CT "bookflow at the top and delta with change in delta at the bottom", "1 effective read", "i also want cx (climax) to be on the candles", "after that build"): the pane split - Book Flow bars on top, delta + delta change bars below (all above 0, colour = side, a ratio over a change of 1.33x+ normal); ONE read (CombRead.h: the passive Book Flow + the aggressive delta); lsRevReadMarks draws Cx on the climax candle of a pivot (over a down-pivot, under an up-pivot)   // was "0.14.0"   // (0.14.0, Rassul 12:12 CT "build": posts fact M - the forming candle's market orders - once a second for the HOD / LOD box's Now line)   // was "0.13.3"   // (0.13.3, Rassul 11:20 CT: one split read - verdict + sentences left (the story / Rev checks as the last sentence), the Bids / Offers table right)   // was "0.13.2"   // (0.13.2, his 11:18 CT screenshot: candle time on the Book Flow verdict line; story prices with the tick's decimals)   // was "0.13.1"   // (0.13.1, 2026-11-17 09:32 CT screenshot: the Book Flow read was blank - a candle without a record falls back to the newest earlier one; the mouse right of the candles reads the last closed one)   // was "0.13.0"   // (0.13.0, 2026-11-17 Rassul 08:35-08:44 CT: the Book Flow read split - verdict + plain-English sentences left, the Bids / Offers table right; the title now shows the real version - 0.12.0 still said 0.11.1)   // was "0.11.1"   // (0.11.1, 2026-11-16 Rassul 20:27 CT "i want ratios on bars where 1 is bigger than the other": the label = the bigger Book Flow side / the smaller, on the bigger bar - was the bar against its own 20-candle average)   // was "0.11.0"   // (0.11.0, 2026-11-16 Rassul 20:20 CT "i want to put book flow back": the pane draws the Book Flow bars itself - no lsBookFlowBg needed; buy / sell volume, delta and delta change stay computed, not drawn)   // was "0.10.1"   // (0.10.1, 2026-11-16 19:02 CT: AccumLogic 2.0.1 - the pivot close rule when the climax is the candle before the pivot)   // was "0.10.0"   // (0.10.0, 2026-11-16 Rassul 14:32-14:43 CT "are you overcomplicating it", "high volume on the pivot or right before it should be considered as part of the accum or dsit process", "go": the accumulation / distribution zones are AccumLogic 2.0.0, the simple rule - climax volume, stall, delta turned, with the trend; the read shows Climax / Stall / Delta)   // was "0.9.2"   // (0.9.2, 2026-11-16 Rassul 13:50 CT "where is the reversal read .. its blank": the bars are drawn in a low pane - under about 52 px nothing was drawn)   // was "0.9.1"   // (0.9.1, 2026-11-16 Rassul 12:44-12:45 CT "build", "i dont see the accumulaiton or distibution": the read shows the LAST zone when none is open, in 3 lines when the pane is low; every order event feeds AccumLogic 1.2.0 (resting lots added / pulled / filled per price) for the quiet end and the other side building)   // was "0.9.0"   // (0.9.0, 2026-11-16 Rassul 10:54-10:58 CT "can you create an accumulation indicator like you did with the boxes", "build now create read on the right side": accumulation / distribution zones (AccumLogic.h) found in the background from each candle's buy / sell lots and the Absorbed reads, their read right of the last candle, the zones saved to RevRead\\accum-<MKT>-<spb>.txt for the price-chart boxes (lsCobSignal 1.8.3 draws them). UNPROVEN = "?")   // was "0.8.0"   // (0.8.0, 2026-11-16 Rassul 06:00 CT "take the information you get from ... SR and put it in the reversal read indicator in the background ... continue to only show what it shows now": lsSR's stop-run engine runs here unseen (SRBack.h) and keeps lsSR's journal; nothing new is drawn)   // was "0.7.0"   // = rvs::VERSION = the one setVersion literal (rvh::stamp)

namespace rvh {
inline void stamp(cppExtension* p) {   // the literal on its own line: gex-build reads the text between the first ( and )
    p->setVersion("0.44.1");
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
// (0.22.1) scheduled releases for the sweep rule: lsFlexLevels\LRA-Calendar-ALL.csv (the nightly lra.event_calendar, any age - past
// releases count too) + the Event Calendar's built-in dated list and weekly rules for the days the bars cover; times in the bars' own unit
// (IRT RTDATE seconds: CT minutes since 1970 x 60 + 2082844800). Bounded read (2 MB), re-read only when the file stamp changes.
static std::vector<long long> rvReleases(const std::string& flex, const std::string& mkt, long long t0, long long t1) {
    static std::mutex mu; static long long stamp = -2; static ecal::File F;
    std::lock_guard<std::mutex> lk(mu);
    if (!flex.empty()) {
        const std::string p = flex + "\\LRA-Calendar-ALL.csv"; struct stat a;
        const long long st = stat(p.c_str(), &a) == 0 ? (long long)a.st_mtime * 1000000LL + (long long)a.st_size : -1;
        if (st != stamp) { stamp = st; F = ecal::File();
            if (st >= 0 && a.st_size > 0 && a.st_size < 2 * 1024 * 1024) { std::ifstream f(p.c_str(), std::ios::binary); std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); F = ecal::parse(txt); } } }
    const long long OFF = 2082844800LL; std::vector<long long> out;
    auto add = [&](const ecal::Ev& e) { if (e.mkt == "ALL" || e.mkt == mkt) out.push_back(e.t * 60 + OFF); };
    if (F.ok) for (const ecal::Ev& e : F.ev) add(e);
    if (t1 > t0) { const long long d0 = (t0 - OFF) / 86400 - 1, d1 = (t1 - OFF) / 86400 + 1; for (const ecal::Ev& e : ecal::builtIn(mkt, d0, d1)) add(e); }
    std::sort(out.begin(), out.end()); out.erase(std::unique(out.begin(), out.end()), out.end()); return out; }
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
        long long bigBars = 0, qTags = 0;   // (0.30.0) bars drawn with a big-order part / question tags drawn (the status)
        size_t written = 0; long long writeFails = 0, events = 0, skipped = 0, sessions = 0, reconnects = -1, marksLoaded = 0;
        long long stamp = -1; int ctSec = 0; long long localSec = 0; int rthO = 510, rthC = 900;
        rvh::PocStore ps; long long fedCandle = 0; bool aligned = true;
        double vwPV = 0, vwV = 0; std::string vwSession; long long vwTick = 0;
        nl::Cache lvc; long long lvlMs = -100000; size_t nLevels = 0; int lvNoFeed = -1; std::string lmDiag;   // (0.21.7) LEVEL_MARKS status: candles / marks per session read, errors   // (0.21.4) levels read without the order feed (-1 = the engine supplied them)
        int hDir = 0, hRot = 0; long long hPx = 0, hTop = 0; bool hBroken = false; long hCount = -1;
        long long lastPx = 0, barLo = 0, barHi = 0;                       // the forming candle (ticks)
        long long parMs = -100000; long long parSig = -2; int parKeys = 0; int pbBits = 0; long long pbMs = -100000;
        long long drawn = 0; std::string lastRead;
        long long wySig = -1; wyr::Out wy; int wyBars = 0;
        std::map<long long, std::pair<int, int> > bsv; bool bsvDirty = false; long long bsvMs = -1; std::string bsvKey;   // (0.31.0) the buy / sell verdict + trapped side of each closed candle, for the Volume read's overall call (RevRead\\bsv-<MKT>-<spb>.txt)
        wyr::Out wyMore; std::map<long long, std::vector<wyr::SMk> > wyBC;   // (0.21.0) T / P / X of the session (WyRead 1.8.0 more(); drawn per showMore) and the B / C strip letters (the Bk / Cx candle marks)
        std::pair<long long, int> cxLive = std::make_pair(0LL, 0);   // (0.25.0) the live Cx? of the running leg (LevelMarks liveCx): stamp, side
        std::map<long long, int> tdDone, tdMk;   /* (0.44.0) the thrust dots: every decided candle (0 = none) and the dots */ std::map<long long, lvm::Mark> lvMarks; std::map<long long, std::vector<lvm::SMark> > lvSeq; std::map<long long, lvm::AbsMk> absMk;   /* (0.27.0) the Abs badges (LevelMarks absMarks) */ std::map<long long, std::pair<int, int> > teq; std::string teqSig; std::map<long long, std::string> tl3; bool tl3Loaded = false, tl3Dirty = false; std::string tl3Path;   /* (0.28.0) lsTurn's Ex? per candle + the Tst line-3 decisions */   /* (0.20.0) FT / Tst / Rev after a Bk */ std::map<long long, std::pair<std::string, long long> > iLv; std::map<long long, std::string> iSt; std::map<long long, int> iSw;   /* (0.21.2) the I's under a Sw (stamp -> dir): written flagged, not drawn */   /* (0.19.2) the I's strength */ std::vector<std::pair<long long, std::string> > lvKey;   // (0.19.0) the candle marks (Bk / Sw / Cx) with their level, and the level each I reclaimed (short name, px in ticks)   // (0.17.0) the Wyckoff strip of the session, rebuilt when a candle closes or a Book Flow record arrives
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
        o << "\nLEVEL_MARKS," << (e->lmDiag.empty() ? std::string("not computed yet") : e->lmDiag) << " (0.21.7: candles / wy letters / Bk-Sw-Cx marks per session read)";
        o << "\nLEVELS_NO_FEED," << e->lvNoFeed << " (0.21.4: key levels read from the level files when the order engine is not running; -1 = the engine supplied them)" << "\nREV_READ,version " << RVS_VERSION << ",market " << e->mkt << ",session " << e->session << ",events " << e->events << ",skipped " << e->skipped << (e->note.empty() ? std::string() : ",note " + e->note)
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
        e.bigBars = 0; e.qTags = 0;   // (0.30.0, the review) counts of THIS draw
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
        struct BFc { int x = 0; long long net[2] = { 0, 0 }; double x2[2] = { 0, 0 }; long long t = 0; }; std::vector<BFc> BF;   // (0.21.5) per candle: bid / offer net (added - pulled), signed, and x the side's normal
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
                PNT p; p.set(i, 0.0f, kBarCenter); c.push_back(std::make_pair((int)p.h, q));
                {   // (0.21.5, Rassul 2026-11-20 13:15 CT "lets go with this") the Book Flow bar = the side's net (added - pulled), signed: + adding (solid), - pulling (hollow);
                    // its label = |net| / the side's normal (the mean |net| of the up to 20 candles before with a record, 10 at least), shown at BF_LABEL_X or more
                    BFc f; f.x = (int)p.h; f.t = (long long)times[i]; f.net[0] = netOf(*r, off::BID); f.net[1] = netOf(*r, off::ASK);
                    long long sm[2] = { 0, 0 }; int n = 0;
                    for (int j = i - 1; j >= 0 && n < 20 && i - j <= 60; --j) { const off::Rec* q2 = recOf(j); if (!q2) continue; ++n; sm[0] += std::llabs(netOf(*q2, off::BID)); sm[1] += std::llabs(netOf(*q2, off::ASK)); }
                    for (int s2 = 0; s2 < 2; ++s2) f.x2[s2] = n >= 10 && sm[s2] > 0 ? std::min(99.9, (double)std::llabs(f.net[s2]) * (double)n / (double)sm[s2]) : 0.0;
                    BF.push_back(f); } }
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
        const int paneH = (int)(pane.bottom - pane.top); const bool split = RV_SHOW_BOOKFLOW && paneH >= 70; const int bfBottom = split ? (int)pane.top + paneH * 50 / 100 : (int)pane.bottom;   /* (0.21.5) the split line: BOOK FLOW above it, DELTA CHANGE below (Rassul 13:15 CT) */   // (0.15.2) bfBottom = the split line: DELTA above it, BOOK FLOW below (Rassul 17:58 CT "make the delta pane on the top instead")
        const std::vector<rvs::BSBar> B = (RV_DRAW_BOOKFLOW || true) ? std::vector<rvs::BSBar>() /* (0.21.5) the Book Flow bars are drawn in the split block below */ : split ? rvs::layoutBS(c, e.eng ? e.eng->marks : none, bfBottom + 2, pane.bottom, 12, bw, 14) : rvs::layoutBS(c, e.eng ? e.eng->marks : none, pane.top, bfBottom, 12, bw, LINE_H + 2);
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
            const int wyShift = 0; (void)wyShift;   // (0.21.5) the strip sits over the Book Flow half   // a very low pane: the row is drawn over the bars' top
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
                    // (0.24.0) the 3-hour S / R as they stood BEFORE each candle traded (= at the close of the candle before): one walk over the chart's closed
                    // candles, in ticks (the same numbers the level file uses). A chart whose bar size does not divide 3 hours, or unreadable prices: none.
                    std::vector<lvm::H3> h3All; int h3Now = 0;
                    try { RTARRAY xh(barHigh), xl(barLow), xc(barClose); const int nC = (int)count - 1;
                        if (hsr::periodOk((int)S.spb) && S.tick > 0 && nC > 1 && xh.count >= nC && xl.count >= nC && xc.count >= nC) {
                            h3All.assign((size_t)nC + 1, lvm::H3()); hsr::Walk hw; hsr::Snap prev; lvm::PivWalk pw; pw.secs = 3600; pw.off = 0; lvm::Blk4 b4; lvm::Blk4 b3; b3.len = 10800; b3.off = 7200; lvm::PivSnap pprev;   /* (0.32.0) + the 3-hour bar (LevelMarks 1.21.0) */   /* (0.26.0) + the hourly pivots and the 4-hour high / low (LevelMarks 1.17.0) */
                            for (int i = 0; i < nC; ++i) { hsr::Bar q; q.t = (long long)times[i]; q.h = std::round((double)xh[i] / S.tick); q.l = std::round((double)xl[i] / S.tick); q.c = std::round((double)xc[i] / S.tick);
                                if (q.h < q.l) std::swap(q.h, q.l);
                                const hsr::Snap sn = (q.t > 0 && std::isfinite(q.h) && std::isfinite(q.l) && std::isfinite(q.c)) ? hw.add(q, (int)S.spb) : prev; prev = sn;
                                lvm::H3 z; const auto tk = [](bool ok, double x) -> long long { return ok && std::isfinite(x) && x > 0 ? (long long)std::llround(x) : 0LL; };
                                z.r1 = tk(sn.r1ok, sn.r1); z.r2 = tk(sn.r2ok, sn.r2); z.s1 = tk(sn.s1ok, sn.s1); z.s2 = tk(sn.s2ok, sn.s2);
                                // (0.26.0) the hourly pivots after this candle (= before the next) and the 4-hour high / low before THIS candle
                                const bool okq = q.t > 0 && std::isfinite(q.h) && std::isfinite(q.l) && std::isfinite(q.c);
                                if (okq) { const std::pair<long long, long long> f4 = b4.add(q.t, (long long)S.spb, (long long)std::llround(q.h), (long long)std::llround(q.l)); h3All[(size_t)i].h4h = f4.first > 0 ? f4.first : 0; h3All[(size_t)i].h4l = f4.second > 0 ? f4.second : 0;
                                    const std::pair<long long, long long> f3 = b3.add(q.t, (long long)S.spb, (long long)std::llround(q.h), (long long)std::llround(q.l)); h3All[(size_t)i].h3h = f3.first > 0 ? f3.first : 0; h3All[(size_t)i].h3l = f3.second > 0 ? f3.second : 0; }
                                const lvm::PivSnap ps = okq ? pw.add(q.t, (long long)S.spb, (long long)std::llround(q.h), (long long)std::llround(q.l), (long long)std::llround(q.c)) : pprev; pprev = ps;
                                z.hr1 = ps.r1; z.hr2 = ps.r2; z.hs1 = ps.s1; z.hs2 = ps.s2;
                                h3All[(size_t)i + 1] = z; }
                            const lvm::H3& zl = h3All.back(); h3Now = (zl.r1 > 0) + (zl.r2 > 0 && zl.r2 != zl.r1) + (zl.s1 > 0) + (zl.s2 > 0 && zl.s2 != zl.s1); } } catch (...) { h3All.clear(); h3Now = 0; }
                    wyr::Out all; int nBars = 0; int segEnd = (int)count - 2;
                    wyr::Out moreAll;   // (0.21.0) T / P / X, each session read by itself (decided on closed candles)
                    std::map<long long, lvm::Mark> lvAll; std::map<long long, std::pair<std::string, long long> > iLvAll;   // (0.19.0)
                    std::map<long long, std::vector<lvm::SMark> > seqAll;   // (0.20.0) FT / Tst / Rev after a Bk
                    std::map<long long, lvm::AbsMk> absAll; std::map<long long, int> tdAll;   /* (0.44.0) */ tdot::Chart tdX;
                    try { RTARRAY th0(barHigh), tl0(barLow), to0(barOpen), tc0(barClose); const int nC = (int)count - 1;   /* (0.44.0) the chart's closed candles for Thrust / PIC */
                        if (nC > 0 && th0.count >= nC && tl0.count >= nC && to0.count >= nC && tc0.count >= nC) for (int i = 0; i < nC; ++i) { tdX.T.push_back((long long)times[i]); tdX.O.push_back((double)to0[i]); tdX.H.push_back((double)th0[i]); tdX.L.push_back((double)tl0[i]); tdX.C.push_back((double)tc0[i]); }
                        if (!std::is_sorted(tdX.T.begin(), tdX.T.end())) tdX = tdot::Chart(); } catch (...) { tdX = tdot::Chart(); }   // (0.27.0) the Abs badges + the next-candle Ex
                    std::vector<lvm::ArCand> arAll; std::vector<lvm::BiCand> biAll;   /* (0.43.0) the BI candidates (LevelMarks 1.31.0), picked with the strength lines */   // (0.23.0) the AR candidates of every Cx (each session by itself); picked after the strength lines
                    // (0.18.4) the key / volume levels the I can reclaim (WyRead 1.1.0): the level files as they stand now - for the CURRENT session only
                    // (older sessions had other levels; they use their own pivot lows / highs); the moving VWAP and the HBO line are not reclaim levels
                    // (0.21.4, Rassul 2026-11-20 11:54 CT "would this have been a sweep" - NG 8:03 Sw ONH not drawn: no order feed on NG, so no engine and no levels)
                    // the key levels come from the order engine when it runs, else straight from the same level files (nl::levels, the 5 s cache) - the level
                    // marks (Bk / Sw / I / Cx) no longer need the order-by-order feed
                    std::vector<std::pair<long long, std::string> > lvSrc; if (e.eng && !e.eng->levels.empty()) lvSrc = e.eng->levels;
                    else { const std::string flex = rvh::flexDir(), mk = !e.mkt.empty() ? e.mkt : rvh::marketOf(S.market);
                        if (!flex.empty() && !mk.empty() && S.tick > 0) { const std::vector<nl::Level> v = nl::levels(e.lvc, flex + "\\", mk, S.symbol, off::steadyMs(), (long long)std::time(nullptr));
                            for (const nl::Level& l : v) if (l.px > 0 && lvSrc.size() < 400) lvSrc.push_back(std::make_pair((long long)std::llround(l.px / S.tick), l.name.substr(0, 24))); }
                        e.lvNoFeed = (int)lvSrc.size(); }
                    std::vector<wyr::Lvl> lvNow; for (const auto& lp : lvSrc) { if (!lvm::keyForI(lp.second, lp.first)) continue;   /* (0.21.2) the same rule, now LevelMarks keyForI (tested) */   /* (0.19.4, Rassul 14:14 CT "I should not print if there is no level") only the named key levels (the Bk / Sw list) - gamma / MenthorQ levels give no I */   /* (0.19.4, Rassul 14:12 CT "I didnt mention a level": the VWAP bands (VWAP-1 / -2 / +1 ...) move - an I that "reclaimed" today's band at its value NOW is not a level it reclaimed then; only VWAP itself was skipped before) */ wyr::Lvl z; z.px = lp.first; z.name = lp.second; lvNow.push_back(z); }
                    // (0.20.3, Rassul 2026-11-19 ~18:35 CT: after 5 PM the earlier sessions lost their key-level marks - ES 11-19 9:42 AM Sw PDL V99+) each
                    // session's range, so its wy lines can be frozen to RevRead\wymarks-<MKT>-<spb>-<YYYYMMDD>.txt and read back unchanged (WyFreeze.h)
                    struct SegR { int seg; std::string nm; long long t0, t1; }; std::vector<SegR> segs;
                    auto sessName = [&](int i) -> std::string { struct tm t; std::memset(&t, 0, sizeof t); if (!getLocaltime((RTDATE)times[i], &t)) return std::string();
                        if (t.tm_hour >= 17) { t.tm_mday += 1; t.tm_hour = 12; t.tm_isdst = -1; if (std::mktime(&t) == (time_t)-1) return std::string(); }
                        char b[16]; if (std::strftime(b, sizeof b, "%Y%m%d", &t) != 8) return std::string(); return std::string(b); };
                    {   // (0.28.0) lsTurn 1.6.0's Ex? of every candle it read whole, and the Tst line-3 decisions made before (kept for good: no repaint)
                        const std::string fx = rvh::flexDir(), mx = !e.mkt.empty() ? e.mkt : rvh::marketOf(S.market);
                        auto rd = [](const std::string& path, size_t cap, std::string& out, std::string* sg) -> int { struct stat st; if (stat(path.c_str(), &st) != 0) return 0; if ((size_t)st.st_size > cap) return -1;
                            const std::string z = std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime); if (sg && *sg == z) return 2;
                            std::ifstream f(path.c_str(), std::ios::binary); if (!f) return -1; out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); if (sg) *sg = z; return 1; };
                        if (!fx.empty() && !mx.empty() && S.spb > 0) {
                            const std::string base = rvh::recDir(fx), sp = std::to_string((long long)S.spb);
                            std::string t; const int r1 = rd(base + "\\turnexq-" + mx + "-" + sp + ".txt", 262144, t, &e.teqSig);
                            if (r1 == 1) { std::map<long long, std::pair<int, int> > m; if (wyi::parseTeq(t, mx, (long long)S.spb, m)) e.teq.swap(m); } else if (r1 == 0) { e.teq.clear(); e.teqSig.clear(); }
                            const std::string p3 = base + "\\tstl3-" + mx + "-" + sp + ".txt"; if (p3 != e.tl3Path) { e.tl3Path = p3; e.tl3.clear(); e.tl3Loaded = false; e.tl3Dirty = false; }
                            if (!e.tl3Loaded) { std::string u; const int r3 = rd(p3, 262144, u, nullptr); std::map<long long, std::string> m;
                                if (r3 == 0) e.tl3Loaded = true; else if (r3 == 1) { if (wyi::parseTl3(u, mx, (long long)S.spb, m)) for (const auto& kv : m) e.tl3[kv.first] = kv.second; e.tl3Loaded = true; } } } }
                    std::set<long long> noSw;   // (0.38.0) LevelMarks 1.26.0: the closed candles whose own BVMax is under 99 can not be a Sw - known BEFORE run(), so a failed sweep candle keeps its Cx / Bk (ES 11-26 10:15 AM)
                    try { RTARRAYI vo(barVolume); RTARRAY so(barOpen), sc(barClose); const int nC = (int)count - 1;
                        if (nC > 0 && vo.count >= nC && so.count >= nC && sc.count >= nC) {
                            std::vector<long long> T((size_t)nC); std::vector<double> V((size_t)nC); std::vector<char> G((size_t)nC);
                            for (int i = 0; i < nC; ++i) { T[(size_t)i] = (long long)times[i]; const double vv = (double)vo[i]; V[(size_t)i] = std::isfinite(vv) && vv > 0 ? vv : 0.0; G[(size_t)i] = (double)sc[i] >= (double)so[i] ? 1 : 0; }
                            if (std::is_sorted(T.begin(), T.end())) noSw = lvm::swWeak(T, V, G); } } catch (...) { noSw.clear(); }
                    for (int seg = 0; seg < 4 && segEnd >= 0 && (int)count - 2 - segEnd < 2400; ++seg) {
                    std::vector<wyr::Bar> WB; std::vector<lvm::H3> WH;   /* (0.24.0) WH[j] = the 3h S / R before WB[j] */ const long long sk = sessKey(segEnd); int first = segEnd;
                    while (first > 0 && segEnd - first < 600 && sessKey(first - 1) == sk) --first;
                    for (int i = first; i <= segEnd; ++i) { const rvs::Poc* p = pocAt(i); if (!p) continue; wyr::Bar b; b.t = (long long)times[i];
                        b.h = p->hi; b.l = p->lo; b.c = p->close; b.o = p->open > 0 ? p->open : p->close; b.buy = p->buy; b.sell = p->sell; b.qTop = p->qTop; b.qBot = p->qBot; if (p->hi - p->lo + 1 >= 4) { b.qTopLots = p->qTopLots; b.qBotLots = p->qBotLots; }   /* (0.28.0) Tst line 3 */   /* (0.27.0) the Abs position line */
                        if (const off::Rec* r = recClosed(i)) { b.book = true; for (int s2 = 0; s2 < 2; ++s2) { b.addNet[s2] = r->addC[s2] - r->pullC[s2]; b.pull[s2] = r->pullC[s2]; /* (0.21.0) P / X */ b.aggrMax[s2] = r->aggrMax[s2]; b.sweepLv[s2] = r->sweepLv[s2]; b.ice[s2] = r->ice[s2]; } }
                        struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)times[i], &t)) b.when = off::clock12(t.tm_hour, t.tm_min);
                        WB.push_back(b); WH.push_back((size_t)i < h3All.size() ? h3All[(size_t)i] : lvm::H3()); }
                    nBars += (int)WB.size(); std::string dg = "s" + std::to_string(seg) + " bars " + std::to_string(WB.size());   // (0.21.7) LEVEL_MARKS
                    try { std::vector<wyr::Lvl> lvI; if (seg == 0) for (const wyr::Lvl& z : lvNow) if (lvm::levelForI(z.name, z.px)) lvI.push_back(z);   /* (0.21.2) no file PHH / PHL for the I (they move every hour - a repaint); the candle hour levels stay */
                        const wyr::Out o = wyr::run(WB, S.tick, dec, wyr::Params(), lvI); for (const auto& kv : o.marks) all.marks[kv.first] = kv.second; for (const auto& kv : o.reads) all.reads[kv.first] = kv.second;
                        // (0.19.0, Rassul 10:37 CT "put level under I too") the I's level as the candle tag's short name (MenthorQ / gamma: no name)
                        for (const auto& kv : o.ilv) { const auto hb = o.ilvHb.find(kv.first); int h = hb != o.ilvHb.end() ? hb->second : 0;
                            std::string nm = kv.second.second; long long lpx = kv.second.first;
                            {   // (0.21.2, "same priority as Bk / Sw") the ONE level shown = the best-priority level the candle reclaimed (LevelMarks iPick)
                                const auto ic = o.ilvC.find(kv.first); const auto mk2 = o.marks.find(kv.first); int sd = 0;
                                if (mk2 != o.marks.end()) for (const wyr::Mark& m : mk2->second) if (m.k == 'I') sd = m.dir;
                                wyr::Lvl z; if (ic != o.ilvC.end() && sd != 0 && lvm::iPick(ic->second, sd, z)) { nm = z.name; lpx = z.px; h = z.hb; } }
                            if (!lvm::iKey(lvm::pri(nm, h))) { nm.clear(); h = 0; }   // (0.21.3, Rassul 10:37 CT "I should only be on key levels that are named like overnight etc..") no hour level names an I
                            const std::string sn = lvm::iLevel(nm, h, nm.find("high") != std::string::npos); iLvAll[kv.first] = std::make_pair(sn, lpx); }
                        dg += " wy " + std::to_string(o.marks.size()); } catch (const std::exception& ex) { dg += std::string(" wy ERR ") + ex.what(); } catch (...) { dg += " wy ERR"; }
                    try { const wyr::Out o2 = wyr::more(WB, S.tick, dec); for (const auto& kv : o2.marks) moreAll.marks[kv.first] = kv.second; for (const auto& kv : o2.reads) moreAll.reads[kv.first] = kv.second; } catch (...) {}   // (0.21.0)
                    // (0.19.0, Rassul 10:20-10:40 CT, mockup v8, "go") Bk / Sw / Cx with one level each - the current session with the level files, earlier ones with their hour levels
                    const std::vector<long long> relNow = WB.empty() ? std::vector<long long>() : rvh::rvReleases(rvh::flexDir(), !e.mkt.empty() ? e.mkt : rvh::marketOf(S.market), WB.front().t, WB.back().t);   // (0.22.1)
                    std::vector<double> preV;   // (0.24.1) the volume of the 20 candles before the session's first: the Cx look-back (LevelMarks 1.15.0)
                    { for (int i = first - 1; i >= 0 && (int)preV.size() < 20; --i) { const rvs::Poc* p = pocAt(i); if (!p) continue; preV.push_back((double)(p->buy + p->sell)); } std::reverse(preV.begin(), preV.end()); }
                    try { const std::map<long long, lvm::Mark> lm = lvm::run(WB, seg == 0 ? lvNow : std::vector<wyr::Lvl>(), lvm::rthOpenMin(!e.mkt.empty() ? e.mkt : rvh::marketOf(S.market)), (long long)S.spb, &relNow, WH.size() == WB.size() && !h3All.empty() ? &WH : nullptr, &preV, noSw.empty() ? nullptr : &noSw)   /* (0.38.0) LevelMarks 1.26.0 noSw */   /* (0.24.0) LevelMarks 1.14.0: + the 3h S / R, a Sw only on a major level */   /* (0.22.0) LevelMarks 1.11.0: no Sw on the RTH opening candle; (0.22.1) 1.12.0: nor on a release candle */; for (const auto& kv : lm) lvAll[kv.first] = kv.second;
                        if (seg == 0) { try { e.cxLive = lvm::liveCx(WB, lm, &preV, &lvNow, WH.size() == WB.size() && !h3All.empty() ? &WH : nullptr);   /* (0.39.0) LevelMarks 1.27.0: the Cx? needs a level too */ } catch (...) { e.cxLive = std::make_pair(0LL, 0); } }   // (0.25.0) the live Cx? (this session, the last closed candle)
                        // (0.20.0, Rassul 15:35-16:58 CT, price-pane-cleanup-mockup.html) Bk -> FT -> Tst -> Rev (LevelMarks 1.3.0 seq), the session by itself
                        const std::map<long long, std::vector<lvm::SMark> > sq = lvm::seq(WB, lm); for (const auto& kv : sq) seqAll[kv.first] = kv.second;
                        if (e.tl3Loaded) {   // (0.28.0) the Tst's 3rd line (LevelMarks 1.19.0 tstL3), decided once the candle after the pivot closed; an older session decides its last pivot too
                            std::vector<double> preR; std::vector<lvm::PreQ> preQ;
                            for (int i = first - 1; i >= 0 && (int)preR.size() < 20; --i) { const rvs::Poc* p = pocAt(i); if (!p) continue; preR.push_back((double)(p->hi - p->lo)); lvm::PreQ z; if (p->hi - p->lo + 1 >= 4) { z.top = p->qTopLots; z.bot = p->qBotLots; } preQ.push_back(z); }
                            std::reverse(preR.begin(), preR.end()); std::reverse(preQ.begin(), preQ.end());
                            const std::map<long long, std::string> l3 = lvm::tstL3(WB, sq, &e.teq, &preV, &preR, &preQ, seg > 0);
                            for (const auto& kv : l3) if (!e.tl3.count(kv.first)) { e.tl3[kv.first] = kv.second; e.tl3Dirty = true; } }
                        { const std::map<long long, lvm::AbsMk> am = lvm::absMarks(WB, seg == 0 ? lvNow : std::vector<wyr::Lvl>(), !e.mkt.empty() ? e.mkt : rvh::marketOf(S.market), WH.size() == WB.size() && !h3All.empty() ? &WH : nullptr, &preV); for (const auto& kv : am) absAll[kv.first] = kv.second; }   // (0.27.0) the Abs badge (LevelMarks 1.18.0)
                        { const std::vector<lvm::ArCand> ac = lvm::arCands(WB, lm); arAll.insert(arAll.end(), ac.begin(), ac.end()); }
                        { std::vector<long long> dec; const std::map<long long, int> td = tdot::dots(WB, tdX, &e.tdDone, &dec); for (long long t : dec) { const auto q = td.find(t); e.tdDone[t] = q != td.end() ? q->second : 0; } for (const auto& kv : td) tdAll[kv.first] = kv.second; }   // (0.44.0) the thrust dots, decided once per closed candle (no repaint)
                        { const std::vector<lvm::BiCand> bc = lvm::biCands(WB, seg == 0 ? lvNow : std::vector<wyr::Lvl>(), lvm::rthOpenMin(!e.mkt.empty() ? e.mkt : rvh::marketOf(S.market)), (long long)S.spb, &relNow, WH.size() == WB.size() && !h3All.empty() ? &WH : nullptr); biAll.insert(biAll.end(), bc.begin(), bc.end()); }   // (0.43.0) the BI (Break In): pivot + first break of a major level + colour + overlap here, BVMax / BRMax 20+ below   // (0.23.0) the AR after a Cx: colour + overlap here, BRMax below
                        dg += " lv " + std::to_string(lm.size()) + " keys " + std::to_string(seg == 0 ? lvNow.size() : 0) + (seg == 0 ? " 3h " + std::to_string(h3Now) : std::string()); } catch (const std::exception& ex) { dg += std::string(" lv ERR ") + ex.what(); } catch (...) { dg += " lv ERR"; }
                    e.lmDiag = (seg == 0 ? std::string() : e.lmDiag + "; ") + dg;
                    { SegR r; r.seg = seg; r.nm = sessName(segEnd); r.t0 = (long long)times[first]; r.t1 = (long long)times[segEnd]; segs.push_back(r); }   // (0.20.3)
                    segEnd = first - 1; }
                    // (0.19.4, Rassul 14:14 CT "I should not print if there is no level") an I without a named level is not kept (letter strip, candle tag and file alike)
                    for (auto it = all.marks.begin(); it != all.marks.end(); ) { const auto il = iLvAll.find(it->first); const bool named = il != iLvAll.end() && !il->second.first.empty();
                        if (!named || !RV_SHOW_I) { std::vector<wyr::Mark>& v = it->second; v.erase(std::remove_if(v.begin(), v.end(), [](const wyr::Mark& m) { return m.k == 'I'; }), v.end()); }
                        if (it->second.empty()) { all.reads.erase(it->first); it = all.marks.erase(it); } else ++it; }
                    // (0.19.2) the strength line (his BVMax / BRMax, as the Volume / Range panes count it) for every Bk / Sw and I, from the chart's closed candles
                    std::map<long long, std::string> iStAll;
                    try { RTARRAYI vo(barVolume); RTARRAY sh(barHigh), sl(barLow), so(barOpen), sc(barClose); const int nC = (int)count - 1;
                        if (nC > 0 && vo.count >= nC && sh.count >= nC && sl.count >= nC && so.count >= nC && sc.count >= nC) {
                            std::vector<long long> T((size_t)nC); std::vector<double> V((size_t)nC), R((size_t)nC); std::vector<char> G((size_t)nC);
                            for (int i = 0; i < nC; ++i) { T[(size_t)i] = (long long)times[i]; const double vv = (double)vo[i]; V[(size_t)i] = std::isfinite(vv) && vv > 0 ? vv : 0.0;
                                R[(size_t)i] = lvm::rangeTicks((double)sh[i], (double)sl[i], S.tick); G[(size_t)i] = (double)sc[i] >= (double)so[i] ? 1 : 0; }
                            if (std::is_sorted(T.begin(), T.end())) { lvm::fillStrength(lvAll, T, V, R, G); e.lmDiag += " | after strength " + std::to_string(lvAll.size());
                                for (auto& kv : seqAll) for (lvm::SMark& m : kv.second) if (m.code == 'R') m.st = lvm::strengthAt(T, V, R, G, kv.first);   // (0.20.0) the Rev's strength line
                                { const std::map<long long, lvm::SMark> ar = lvm::arPick(arAll, lvAll, T, V, R, G); for (const auto& kv : ar) seqAll[kv.first].push_back(kv.second); }
                                { const std::map<long long, lvm::SMark> bi = lvm::biPick(biAll, T, V, R, G, tdX.T.empty() ? nullptr : &tdX);   /* (0.44.1) + Thrust / PIC 1.20 */ for (const auto& kv : bi) seqAll[kv.first].push_back(kv.second); }   // (0.43.0) the BI: the first candidate of each pivot with BVMax or BRMax 20+   // (0.23.0) the AR: the first candidate with BRMax 10+, with its strength line
                                for (const auto& kv : all.marks) for (const wyr::Mark& m : kv.second) if (m.k == 'I') { const std::string st = lvm::strengthAt(T, V, R, G, kv.first); if (!st.empty()) iStAll[kv.first] = st; }
                                // (0.21.3, Rassul 10:37 CT "we should have a 20 threshold just like sweeps") an I whose own candle's BVMax / BRMax is under 20 is not kept
                                for (auto it = all.marks.begin(); it != all.marks.end(); ) { const auto ti = std::lower_bound(T.begin(), T.end(), it->first);
                                    const bool weak = ti == T.end() || *ti != it->first || lvm::strengthN(V, R, G, (int)(ti - T.begin()), (int)(ti - T.begin())) < lvm::I_MIN;
                                    std::vector<wyr::Mark>& v = it->second; if (weak) v.erase(std::remove_if(v.begin(), v.end(), [](const wyr::Mark& m) { return m.k == 'I'; }), v.end());
                                    if (weak && v.empty()) { iStAll.erase(it->first); all.reads.erase(it->first); it = all.marks.erase(it); } else { if (weak) iStAll.erase(it->first); ++it; } } } } } catch (...) {}
                    e.iSt.swap(iStAll);
                    // (0.21.2, Rassul 09:20 CT "if both sweep and i are true, only show sweep") an I on a Sw candle of the same direction leaves the strip and the
                    // price chart (after the Sw strength rule, fillStrength, dropped the weak Sw's); it is kept in iSwAll and written to the wy file flagged (7th field "Sw") so the nightly still scores it
                    std::map<long long, int> iSwAll;
                    for (auto it = all.marks.begin(); it != all.marks.end(); ) { std::vector<wyr::Mark>& v = it->second;
                        for (auto q = v.begin(); q != v.end(); ) { if (q->k == 'I' && lvm::iUnderSw(lvAll, it->first, q->dir)) { iSwAll[it->first] = q->dir; q = v.erase(q); } else ++q; }
                        if (v.empty()) { all.reads.erase(it->first); it = all.marks.erase(it); } else ++it; }
                    e.lvKey.clear(); for (const wyr::Lvl& z : lvNow) if (lvm::pri(z.name, 0) >= 0 && z.name.find('|') == std::string::npos) e.lvKey.push_back(std::make_pair(z.px, z.name.substr(0, 24)));   // (0.19.0) the key / volume levels the BO indicator labels its Rly with
                    // (0.21.0) the read of a DRAWN T / P / X joins the candle's read (right of the last candle); hidden ones are journaled only
                    {   const std::string mkS = rvh::marketOf(S.market);
                        for (const auto& kv : moreAll.marks) for (const wyr::Mark& m : kv.second) { if (!wyr::showMore(m.k, mkS)) continue; const auto rd = moreAll.reads.find(kv.first); if (rd == moreAll.reads.end()) continue;
                            wyr::Read& rr = all.reads[kv.first]; if (rr.head.empty()) rr = rd->second; else if (rr.head != rd->second.head) { rr.lines.push_back(""); rr.lines.push_back(rd->second.head); for (const std::string& l : rd->second.lines) rr.lines.push_back(l); } } }
                    e.wyMore.marks.swap(moreAll.marks); e.wyMore.reads.swap(moreAll.reads);
                    {   e.wyBC.clear(); for (const auto& kv : lvAll) if (kv.second.code == 'B' || kv.second.code == 'C') { wyr::SMk x; x.lab = kv.second.code == 'B' ? "B" : "C"; x.dir = kv.second.dir; e.wyBC[kv.first].push_back(x); }
                        for (const auto& kv : all.marks) for (const wyr::Mark& m : kv.second) if (m.k == 'I') { wyr::SMk x; x.lab = "I"; x.dir = m.dir; e.wyBC[kv.first].push_back(x); } }   // (0.21.2) + the shown I's   // (0.21.0) B / C, replaced below by the published (frozen) file lines when they are written
                    e.wyBars = nBars; e.wy.marks.swap(all.marks); e.wy.reads.swap(all.reads); e.lvMarks.swap(lvAll); e.iLv.swap(iLvAll); e.lvSeq.swap(seqAll); e.absMk.swap(absAll); e.tdMk.swap(tdAll); if (e.tdDone.size() > 20000) e.tdDone.clear(); e.iSw.swap(iSwAll);
                    // (0.17.5, Rassul 13:08 CT "yes, just like the mockup" - the I letters on the SDMO pane too) the session's letters are published
                    // once per closed candle to lsFlexLevels\RevRead\wy-<MKT>-<secs>.txt (candle stamp|letter|dir); lsSDMO draws the I's from it, so the
                    // two panes always agree (and the nightly scores every letter from it)
                    try { const std::string flex = rvh::flexDir(), mk = rvh::marketOf(S.market);
                        if (!flex.empty() && !mk.empty() && S.spb > 0) {
                            const std::string head = "WY|1|" + mk + "|" + std::to_string((long long)S.spb) + "|" + RVS_VERSION + "\n"; std::string txt;   // (0.20.3) the body first, then the frozen sessions
                            for (const auto& kv : e.wy.marks) for (const wyr::Mark& m : kv.second) { txt += std::to_string(kv.first) + "|" + wyr::label(m) + "|" + std::to_string(m.dir);
                                if (m.k == 'I') { const auto il = e.iLv.find(kv.first); const auto is = e.iSt.find(kv.first); const std::string st = is != e.iSt.end() ? is->second : std::string();
                                    if (il != e.iLv.end()) txt += "|" + il->second.first + "|" + std::to_string(il->second.second) + "|" + st;   // (0.19.0) the I's level: 5 fields; (0.19.2) + its strength: 6
                                    else if (!st.empty()) txt += "||0|" + st; }
                                txt += "\n"; }
                            for (const auto& kv : e.iSw) { const auto il = e.iLv.find(kv.first); if (il == e.iLv.end()) continue; const auto is = e.iSt.find(kv.first);   // (0.21.2) the I under a Sw: recorded, flagged, never drawn
                                txt += wyi::iLine(kv.first, kv.second, il->second.first, il->second.second, is != e.iSt.end() ? is->second : std::string(), true) + "\n"; }
                            for (const auto& z : e.lvKey) txt += "L|" + std::to_string(z.first) + "|" + z.second + "\n";   // (0.19.0) the levels (WyMarks parseLevels)
                            for (const auto& kv : e.lvMarks) txt += std::to_string(kv.first) + "|" + lvm::codeText(kv.second.code) + "|" + std::to_string(kv.second.dir) + "|" + kv.second.lv + "|" + std::to_string(kv.second.px) + "|" + kv.second.st + "\n";   // (0.19.0) Bk / Sw / Cx; (0.19.2) + the strength (6th field, may be empty)
                            for (const auto& kv : e.lvSeq) for (const lvm::SMark& m : kv.second) {   // (0.20.0) FT: 3 fields; Tst: level + its % (no dashed line: px 0); Rev: + its strength
                                txt += std::to_string(kv.first) + "|" + lvm::seqText(m.code) + "|" + std::to_string(m.dir);
                                if (m.code == 'T') { const auto q3 = e.tl3.find(kv.first); txt += "|" + m.lv + "|0|" + (q3 != e.tl3.end() ? q3->second : std::string()); }   /* (0.28.0) line 3 = A+E / E / A! / blank (was the retrace %) */ else if (m.code == 'N') txt += "|" + m.lv + "|" + std::to_string(m.px) + "|" + m.st;   /* (0.43.0) BI: level, its price, strength (6 fields) */ else if (m.code == 'R' || m.code == 'A') txt += "||0|" + m.st;   // (0.23.0) AR: as a Rev - no level, its strength
                                txt += "\n"; }
                            for (const auto& kv : e.tdMk) txt += std::to_string(kv.first) + "|TD|" + std::to_string(kv.second) + "\n";   // (0.44.0) the thrust dot: 3 fields
                            // (0.27.0) the Abs badge: "stamp|Abs|dir|<Top 21% / Low 21% or empty>|<level px>|<level short name>"; its next-candle Ex: "stamp|Ex|dir"
                            for (const auto& kv : e.absMk) { txt += std::to_string(kv.first) + "|Abs|" + std::to_string(kv.second.dir) + "|" + lvm::absPos(kv.second) + "|" + std::to_string(kv.second.lvPx) + "|" + kv.second.lv + "\n";
                                if (kv.second.ex && kv.second.exT > 0) txt += std::to_string(kv.second.exT) + "|Ex|" + std::to_string(kv.second.dir) + "\n"; }
                            auto putFile = [](const std::string& path, const std::string& body) { const std::string tmp = path + ".tmp";
                                bool ok = false; { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (f) { f << body; f.flush(); ok = f.good(); } }
#if defined(_WIN32)
                                if (ok && !::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) std::remove(tmp.c_str());
#else
                                if (ok && std::rename(tmp.c_str(), path.c_str()) != 0) std::remove(tmp.c_str());
#endif
                                if (!ok) std::remove(tmp.c_str()); };
                            auto getFile = [](const std::string& path) -> std::string { struct stat st; if (stat(path.c_str(), &st) != 0 || (size_t)st.st_size > wyf::MAX_BYTES) return std::string();
                                std::ifstream f(path.c_str(), std::ios::binary); if (!f) return std::string(); return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); };
                            // (0.20.3) the frozen sessions: the live one is written as computed; once over (the forming candle is in a newer session) its file is
                            // read back unchanged (+ any candle that closed after its last write) and replaces the recompute, which had only hour levels
                            std::vector<wyf::Seg> sg; for (const SegR& g : segs) { wyf::Seg x; x.seg = g.seg; x.nm = g.nm; x.t0 = g.t0; x.t1 = g.t1; sg.push_back(x); }
                            const bool liveSame = (int)count >= 2 && sessKey((int)count - 1) == sessKey((int)count - 2);
                            const std::string dir = rvh::recDir(flex), pre = dir + "\\wymarks-" + mk + "-" + std::to_string((long long)S.spb) + "-";
                            const std::vector<std::string> fin = wyf::apply(wyf::splitLines(txt), sg, liveSame, mk, (long long)S.spb,
                                [&](const std::string& nm) { return pre + nm + ".txt"; }, getFile, putFile);
                            std::string out = head; for (const std::string& ln : fin) out += ln + "\n";
                            // (0.21.0) the strip's B / C = the Bk / Cx lines the price chart draws (the same frozen file lines - they never change)
                            {   std::map<long long, std::vector<wyr::SMk> > bc = wyr::stripFromLines(fin);   // (0.21.2) + the I (WyRead 1.9.0): the strip shows what the price chart shows, frozen sessions included
                                e.wyBC.swap(bc); }
                            // (0.21.0) the T / P / X journal (every one computed, shown or not): RevRead\wyx-<MKT>-<spb>.txt "stamp|letter|dir|shown"; a
                            // separate file so the readers of wy-<MKT>-<spb>.txt (lsRevReadMarks, lsSDMO, lsBreakout, lsHodLod, lsVolRead, lsWavePOC) see no new codes
                            {   std::string jx = "WYX|1|" + mk + "|" + std::to_string((long long)S.spb) + "|" + RVS_VERSION + "\n";
                                for (const auto& kv : e.wyMore.marks) for (const wyr::Mark& m : kv.second) jx += std::to_string(kv.first) + "|" + wyr::label(m) + "|" + std::to_string(m.dir) + "|" + (wyr::showMore(m.k, mk) ? "1" : "0") + "\n";
                                putFile(rvh::recDir(flex) + "\\wyx-" + mk + "-" + std::to_string((long long)S.spb) + ".txt", jx); }
                            putFile(rvh::recDir(flex) + "\\wy-" + mk + "-" + std::to_string((long long)S.spb) + ".txt", out);
                            if (e.tl3Dirty && e.tl3Loaded && !e.tl3Path.empty()) { putFile(e.tl3Path, wyi::tl3Text(e.tl3, mk, (long long)S.spb, RVS_VERSION)); e.tl3Dirty = false; }   // (0.28.0)
                            // (0.25.0) the live Cx? in a file of its own (lsRevReadMarks draws it; nothing else reads it; never in the wy / wymarks records):
                            // RevRead\wylive-<MKT>-<spb>.txt  "WYL|1|<MKT>|<spb>|<version>" then "stamp|Cx?|dir" (or no line)
                            putFile(rvh::recDir(flex) + "\\wylive-" + mk + "-" + std::to_string((long long)S.spb) + ".txt", "WYL|1|" + mk + "|" + std::to_string((long long)S.spb) + "|" + RVS_VERSION + "\n"
                                + (e.cxLive.first > 0 && !e.lvMarks.count(e.cxLive.first) ? std::to_string(e.cxLive.first) + "|Cx?|" + std::to_string(e.cxLive.second) + "\n" : std::string())); } } catch (...) {} } }
            {   FONT f7 = fb; f7.size = 7;
                // (0.17.4, Rassul 09:53 CT "make the signal strip have a dark gray background") a dark gray band behind the letters, from the pane's left
                // edge to just past the last candle (never under the read text on the right)
                {   const int lastI = std::min(v1, (int)count - 1); PNT pl; pl.set(lastI, 0.0f, kBarCenter); PNT pp; pp.set(std::max(v0, lastI - 1), 0.0f, kBarCenter);
                    const int half = std::max(2, ((int)pl.h - (int)pp.h) / 2 + 1);
                    off::fillFlat(this, (int)pane.left, wyTop - 1, (int)pl.h + half, wyTop + WY_ROW + 1, rvh::C_STRIP); }
                const std::string mkD = rvh::marketOf(S.market);
                for (int i = v0; i <= v1 && i < (int)count - 1; ++i) { const long long ti = (long long)times[i];
                    // (0.21.0, Rassul 2026-11-20 05:52 CT) the candle's letters = A / E / Sp / Ut (+ I when shown) + the drawn T / P / X + B / C, at most 2
                    // by priority (WyRead stripPick: Sp / Ut > T > P > A > X > E > B > C), the same colours and sizes as before
                    std::vector<wyr::SMk> cand;
                    { const auto it = e.wy.marks.find(ti); if (it != e.wy.marks.end()) for (const wyr::Mark& m : it->second) { if (m.k == 'I') continue;   /* (0.21.2) the I comes with B / C from the published lines */ wyr::SMk x; x.lab = wyr::label(m); x.dir = m.dir; cand.push_back(x); } }
                    { const auto it = e.wyMore.marks.find(ti); if (it != e.wyMore.marks.end()) for (const wyr::Mark& m : it->second) if (wyr::showMore(m.k, mkD)) { wyr::SMk x; x.lab = wyr::label(m); x.dir = m.dir; cand.push_back(x); } }
                    { const auto it = e.wyBC.find(ti); if (it != e.wyBC.end()) for (const wyr::SMk& x : it->second) cand.push_back(x); }
                    const std::vector<wyr::SMk> pick = wyr::stripPick(cand, 2); if (pick.empty()) continue;
                    PNT pt; pt.set(i, 0.0f, kBarCenter); const int cx = (int)pt.h; const bool many = pick.size() > 1; setFont(many ? f7 : fb);
                    int tw = 0; for (const wyr::SMk& m : pick) tw += (int)getTextWidth(m.lab.c_str(), -1);
                    int xx = cx - tw / 2; const int yy = wyTop + (many ? 1 : 0);
                    for (const wyr::SMk& m : pick) { const std::string t = m.lab; const int w = (int)getTextWidth(t.c_str(), -1);
                        setTextColor(m.dir > 0 ? rvh::C_GREEN : rvh::C_RED); RCT r; r.set(rvh::coord(xx), rvh::coord(yy), rvh::coord(xx + w + 1), rvh::coord(yy + WY_ROW)); r.drawText(t.c_str(), false, false); xx += w; } } }
            const int dTop = bfBottom + 3, dBot = (int)pane.bottom - 2, dH = dBot - dTop - 14;   // (0.21.5) the BOTTOM half: the title row, then the change bars   // (0.15.2) the top half under the title row; 14 px kept for the labels; (0.17.0) under the Wyckoff strip
            struct DB { int x; long long dl, chg; bool hasChg; double lab; long long sess; }; std::vector<DB> D;
            for (int i = v0; RV_DRAW_BOOKFLOW && i <= v1; ++i) { const rvs::Poc* p = pocAt(i); if (!p) continue; const rvs::Poc* q = pocAt(i - 1); DB d;   /* (0.30.0, the review) only while the delta-change bars are drawn */ PNT pt; pt.set(i, 0.0f, kBarCenter); d.x = (int)pt.h; d.dl = p->cdlt; d.hasChg = q != nullptr; d.chg = q ? p->cdlt - q->cdlt : 0; d.lab = 0;
                if (q) { std::vector<long long> h; for (int k = i - 1; k >= 1 && (int)h.size() < 20 && i - k <= 40; --k) { const rvs::Poc* a = pocAt(k); const rvs::Poc* b2 = pocAt(k - 1); if (a && b2) h.push_back(a->cdlt - b2->cdlt); }
                    if (h.size() >= 10) d.lab = cbr::changeLabel(d.chg, cbr::medAbs(h)); }
                d.sess = sessKey(i); D.push_back(d); }
            // (0.15.2, Rassul 17:58 CT "the delta is not drawing on the new bar") the FORMING candle is drawn live: its delta so far from IRT's volume at price
            if (RV_DRAW_BOOKFLOW && v1 >= (int)count - 1 && count >= 2) { RTARRAYP vap(barVolumeProfile); std::vector<rvs::VRow> rows; double pv = 0;   /* (0.30.0) as above */
                if (rvh::vapRows(vap, (int)count - 1, S.tick, rows, pv) > 0) { long long dl = 0; for (const rvs::VRow& r : rows) dl += r.dlt; const rvs::Poc* q = pocAt((int)count - 2);
                    DB d; PNT pt; pt.set((int)count - 1, 0.0f, kBarCenter); d.x = (int)pt.h; d.dl = dl; d.hasChg = q != nullptr; d.chg = q ? dl - q->cdlt : 0; d.lab = 0; d.sess = sessKey((int)count - 1); D.push_back(d); } }
            // (0.15.2) each SESSION is scaled by itself (the evening's bars were too small to see next to the day session's), on the session's 95th
            // percentile so one huge bar does not flatten the rest - a taller bar is clipped at the top
            std::map<long long, long long> mxS; { std::map<long long, std::vector<long long> > all; for (const DB& d : D) { if (d.hasChg) all[d.sess].push_back(std::llabs(d.chg)); }
                for (auto& kv : all) { std::vector<long long>& v = kv.second; std::sort(v.begin(), v.end()); const long long p95 = v.empty() ? 0 : v[(size_t)std::min(v.size() - 1, (size_t)(0.95 * (double)v.size()))]; mxS[kv.first] = std::max(1LL, p95 * 115 / 100); } }
            const long long mx = 1;
            const int w = std::max(1, (int)std::lround(bw * 0.5));   // (0.21.5) one bar per candle now
            if (!RV_DRAW_BOOKFLOW) {   // (0.29.0) BUY / SELL VOLUME over the whole pane under the strip: per candle buys (green, left) and sells (red, right), up from
                                       // zero, scaled per session on its 95th percentile; the "biggest in N" count on a bar from 10 (Poc.bbv / bsv); the forming candle not drawn
                bool any = false; for (int i = v0; i <= v1 && i <= (int)count - 2 && !any; ++i) if (pocAt(i)) any = true;
                const int bTop = wyTop + WY_ROW + 14, bBot = (int)pane.bottom - 2, bH = bBot - bTop;
                if (bH > 8 && any) {
                    std::vector<long long> skv((size_t)(v1 - v0 + 1), 0); for (int i = v0; i <= v1; ++i) skv[(size_t)(i - v0)] = sessKey(i);   // (0.30.0, the review) the session of each candle once per draw
                    std::map<long long, long long> mxB; { std::map<long long, std::vector<long long> > all;
                        for (int i = v0; i <= v1 && i <= (int)count - 2; ++i) { const rvs::Poc* p = pocAt(i); if (!p) continue; std::vector<long long>& v = all[skv[(size_t)(i - v0)]]; v.push_back(p->buy); v.push_back(p->sell); }
                        for (auto& kv : all) { std::vector<long long>& v = kv.second; std::sort(v.begin(), v.end()); const long long p95 = v.empty() ? 0 : v[(size_t)std::min(v.size() - 1, (size_t)(0.95 * (double)v.size()))]; mxB[kv.first] = std::max(1LL, p95 * 115 / 100); } }
                    const int wb = std::max(1, (int)std::lround(bw * 0.36)); setFont(fb);
                    for (int i = v0; i <= v1 && i <= (int)count - 2; ++i) { const rvs::Poc* p = pocAt(i); if (!p) continue; PNT pt; pt.set(i, 0.0f, kBarCenter); const int x = (int)pt.h; const double m2 = (double)mxB[skv[(size_t)(i - v0)]];
                        const int hB = p->buy > 0 ? std::min(bH, std::max(1, (int)std::llround((double)p->buy * bH / m2))) : 0, hS = p->sell > 0 ? std::min(bH, std::max(1, (int)std::llround((double)p->sell * bH / m2))) : 0;
                        if (hB > 0) off::fillFlat(this, x - wb, bBot - hB, x, bBot, rvh::C_GREEN);
                        if (hS > 0) off::fillFlat(this, x + 1, bBot - hS, x + 1 + wb, bBot, rvh::C_RED);
                        {   // (0.30.0) the DARK part from the bottom = the share of the side's market lots that came from big orders (S20+), from the order feed
                            const auto ri = S.ser.bars.find((long long)times[i]);
                            if (ri != S.ser.bars.end() && ri->second.ok && ri->second.r.bigOk) { const off::Rec& rr = ri->second.r;
                                const long long ab = rr.aggrC[off::ASK], as = rr.aggrC[off::BID];
                                const int dB = ab > 0 && hB > 0 ? (int)std::min((long long)hB, (long long)hB * rr.bigC[off::ASK] / ab) : 0, dS = as > 0 && hS > 0 ? (int)std::min((long long)hS, (long long)hS * rr.bigC[off::BID] / as) : 0;
                                if (dB > 0) off::fillFlat(this, x - wb, bBot - dB, x, bBot, rvh::C_BAR_GREEN);
                                if (dS > 0) off::fillFlat(this, x + 1, bBot - dS, x + 1 + wb, bBot, rvh::C_BAR_RED);
                                if (dB > 0 || dS > 0) ++e.bigBars; } }
                        const int nB = p->bbv >= 10 ? p->bbv : 0, nS = p->bsv >= 10 ? p->bsv : 0;
                        if (nB) { const std::string t = nB >= 99 ? std::string("99+") : std::to_string(nB); const int w2 = (int)getTextWidth(t.c_str(), -1); const int cx = x - wb / 2 - (nS ? w2 / 2 : 0); const int yy = std::max(bTop - 13, bBot - hB - 13);
                            setTextColor(rvh::C_GREEN); RCT r; r.set(rvh::coord(cx - w2 / 2), rvh::coord(yy), rvh::coord(cx - w2 / 2 + w2 + 1), rvh::coord(yy + 12)); r.drawText(t.c_str(), false, false); }
                        if (nS) { const std::string t = nS >= 99 ? std::string("99+") : std::to_string(nS); const int w2 = (int)getTextWidth(t.c_str(), -1); const int cx = x + 1 + wb / 2 + (nB ? w2 / 2 : 0); const int yy = std::max(bTop - 13, bBot - hS - 13);
                            setTextColor(rvh::C_RED); RCT r; r.set(rvh::coord(cx - w2 / 2), rvh::coord(yy), rvh::coord(cx - w2 / 2 + w2 + 1), rvh::coord(yy + 12)); r.drawText(t.c_str(), false, false); } }
                }
            }
            else if (dH > 8 && mx > 0 && !D.empty()) { setFont(fb); int lastR = -100000; double lastX = 0;
                const std::string ttl = "Delta change"; const int tlR = (int)pane.left + 6 + (int)getTextWidth(ttl.c_str(), -1) + 4, tlB = dTop + 14;   // the title's box: no ratio label over it
                for (const DB& d : D) { const double m2 = (double)mxS[d.sess]; const int h1 = std::min(dH, (int)((double)std::llabs(d.dl) * dH / m2)), h2 = std::min(dH, (int)((double)std::llabs(d.chg) * dH / m2));
                    (void)h1;   // (0.21.5, Rassul 13:15 CT) the delta bar itself is not drawn - only its change from the candle before, green / red by sign
                    if (d.hasChg && h2 > 0) off::fillFlat(this, d.x - w / 2, dBot - h2, d.x - w / 2 + w, dBot, d.chg >= 0 ? rvh::C_GREEN : rvh::C_RED);
                    if (d.lab > 0) { char t[16]; std::snprintf(t, sizeof t, "%.1fx", d.lab); const int tw2 = (int)getTextWidth(t, -1), cx = d.x, l = cx - tw2 / 2;
                        if (l < lastR + 2 && d.lab <= lastX) continue;   // two labels side by side: the bigger one stays (the later one is skipped when smaller)
                        if (l < tlR && dBot - h2 - 13 < tlB) continue;     // (0.15.2) never over the "Delta + change" title
                        setTextColor(d.chg >= 0 ? rvh::C_GREEN : rvh::C_RED); RCT r; r.set(rvh::coord(l), rvh::coord(dBot - h2 - 13), rvh::coord(l + tw2 + 1), rvh::coord(dBot - h2 - 1)); r.drawText(t, false, false); lastR = l + tw2; lastX = d.lab; } }
                setFont(fb); put(ttl, (int)pane.left + 6, dTop, rvh::C_GRAY, (int)pane.right - 6); }
            if (RV_DRAW_BOOKFLOW) {   // (0.29.0) hidden: not drawn   // (0.21.5, Rassul 2026-11-20 13:15 CT "lets go with this") BOOK FLOW, the top half - "are they defending?": per candle the bids' net (green, left)
                // and the offers' net (red, right), up from zero; solid = more added than pulled, hollow = more pulled; a label at BF_LABEL_X+ the side's normal.
                // No legend row (13:16 CT "dont keep the extra line at the top ... i dont want it taking up too much space")
                const int tY = wyTop + WY_ROW + 1, bBot = bfBottom - 1, bTop = tY + 14, bH = bBot - bTop;
                setFont(fb); const std::string bt = "Book Flow"; const int btR = (int)pane.left + 6 + (int)getTextWidth(bt.c_str(), -1) + 4;
                if (bH > 6 && !BF.empty()) {
                    std::vector<long long> mg; for (const BFc& f : BF) { mg.push_back(std::llabs(f.net[0])); mg.push_back(std::llabs(f.net[1])); } std::sort(mg.begin(), mg.end());
                    const long long mxB = std::max(1LL, mg[std::min(mg.size() - 1, (size_t)(0.95 * (double)mg.size()))] * 115 / 100);   // the 95th percentile: one huge bar does not flatten the rest (taller = clipped)
                    const int wb = std::max(1, (int)std::lround(bw * 0.36));
                    struct BL { int l, t, r, side; double x; char tx[16]; }; std::vector<BL> labs;
                    for (const BFc& f : BF) for (int s2 = 0; s2 < 2; ++s2) { const long long n = f.net[s2]; if (n == 0) continue;
                        const int h = std::min(bH, std::max(1, (int)std::llround((double)std::llabs(n) * (double)bH / (double)mxB)));
                        const int l = s2 == 0 ? f.x - wb : f.x + 1, r = l + wb, t = bBot - h; const COLOR col = s2 == 0 ? rvh::C_GREEN : rvh::C_RED;
                        if (n > 0) off::fillFlat(this, l, t, r, bBot, col);
                        else { off::fillFlat(this, l, t, l + 1, bBot, col); off::fillFlat(this, r - 1, t, r, bBot, col); off::fillFlat(this, l, t, r, t + 1, col); off::fillFlat(this, l, bBot - 1, r, bBot, col); }
                        if (f.x2[s2] >= BF_LABEL_X) { BL L; L.side = s2; L.x = f.x2[s2]; std::snprintf(L.tx, sizeof L.tx, "%.1fx", f.x2[s2]);
                            const int tw = (int)getTextWidth(L.tx, -1); L.l = (l + r) / 2 - tw / 2; L.r = L.l + tw + 1; L.t = std::max(tY, t - 13);
                            if (L.l < btR && L.t < tY + 13) continue;   // never over the title
                            if (!labs.empty()) { BL& p = labs.back(); if (L.l < p.r + 2 && L.t < p.t + 12 && p.t < L.t + 12) { if (L.x > p.x) p = L; continue; } }   // overlapping: the bigger stays
                            labs.push_back(L); } }
                    for (const BL& L : labs) { setTextColor(L.side == 0 ? rvh::C_GREEN : rvh::C_RED); RCT rr; rr.set(rvh::coord(L.l), rvh::coord(L.t), rvh::coord(L.r), rvh::coord(L.t + 12)); rr.drawText(L.tx, false, false); } }
                setFont(fb); put(bt, (int)pane.left + 6, tY, rvh::C_GRAY, (int)pane.right - 6); }
            if (split && RV_DRAW_BOOKFLOW) {   // (0.29.0) no halves while Book Flow is hidden   // a thin line between the halves, under the candles only (the read right of the last candle runs over both halves)
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
        if ((cb.bookOk || cb.deltaOk || !RV_DRAW_BOOKFLOW) && cbBar >= 0 && v1 >= (int)count - 1) {   // (0.29.0) with Book Flow hidden: the buy / sell read   // (0.15.0, Rassul 15:31 CT "1 effective read") THE ONE READ: Book Flow (passive) + delta (aggressive)
            const std::string mk = rvh::marketOf(getRootSymbol() ? getRootSymbol() : "", getSymbol() ? getSymbol() : ""); cb.mkt = mk;
            std::string when; { struct tm t; std::memset(&t, 0, sizeof t); if (getLocaltime((RTDATE)times[cbBar], &t)) when = off::clock12(t.tm_hour, t.tm_min); } cb.when = when;
            cbr::Out fo; std::string foHeadQ; std::vector<std::string> foExQ;   // (0.30.0) the volume-model question of the head / each sentence (buy / sell read)
            if (RV_DRAW_BOOKFLOW) fo = cbr::toPlain(cb, cbr::read(cb));
            else {   // (0.29.0) THE BUY / SELL READ (BsRead.h): who hit whom on the candle under the mouse (else the last closed one), from IRT's volume at price
                auto pk = [&](int i) -> const rvs::Poc* { if (i < 0 || i >= (int)count) return nullptr; const auto it = e.ps.poc.find((long long)times[i]); return it != e.ps.poc.end() && it->second.ok ? &it->second : nullptr; };
                const auto buildIn = [&](int cbBar) { bsr::In bi; const rvs::Poc* p0 = pk(cbBar);
                if (p0) { bi.ok = true; bi.buy = p0->buy; bi.sell = p0->sell; bi.nB = p0->bbv; bi.nS = p0->bsv; bi.open = p0->open; bi.close = p0->close; bi.hi = p0->hi; bi.lo = p0->lo;   /* (0.31.0) + the candle and the 40 before for the conclusions */
                    for (int k = cbBar - 1; k >= 0 && cbBar - k <= 40; --k) { const rvs::Poc* a = pk(k); bsr::Prev q; if (a && a->hi > 0 && a->lo > 0) { q.ok = true; q.hi = a->hi; q.lo = a->lo; q.close = a->close; q.buy = a->buy; q.sell = a->sell; } bi.prev.push_back(q); if (!q.ok) break; }
                    if (const rvs::Poc* q0 = pk(cbBar - 1)) { bi.prevOk = true; bi.prevDelta = q0->buy - q0->sell; }
                    for (int k = cbBar - 1; k >= 0 && (int)bi.pb.size() < bsr::NORM_N && cbBar - k <= 40; --k) { const rvs::Poc* a = pk(k); if (!a) continue; bi.pb.push_back(a->buy); bi.ps.push_back(a->sell);
                        if (const rvs::Poc* b2 = pk(k - 1)) bi.pswing.push_back(std::llabs((a->buy - a->sell) - (b2->buy - b2->sell))); } } return bi; };
                bsr::In bi = buildIn(cbBar);
                {   // (0.30.0) the big orders' share, from the order feed's record of this candle
                    const auto ri = S.ser.bars.find((long long)times[cbBar]);
                    if (ri != S.ser.bars.end() && ri->second.ok) { const off::Rec& rr = ri->second.r; bi.bigOk = rr.bigOk; bi.bigBuy = rr.bigC[off::ASK]; bi.bigSell = rr.bigC[off::BID];
                        bi.allBuy = rr.aggrC[off::ASK]; bi.allSell = rr.aggrC[off::BID]; bi.thr = rr.thrAggr; } }
                {   // (0.31.0) every closed candle in view gets its verdict once (the Volume read's overall call): +1 / -1 / 0 and the trapped side (+1 buyers, -1 sellers)
                    const std::string key = mk + "|" + std::to_string((long long)S.spb); if (e.bsvKey != key) { e.bsv.clear(); e.bsvKey = key; e.bsvMs = -1; }
                    int added = 0; for (int i = (int)count - 2; i >= 0 && i >= (int)count - 200 && added < 400; --i) { const long long t = (long long)times[i]; if (e.bsv.count(t) || !pk(i) || !pk(i - 20)) continue;
                        const bsr::Out q = bsr::read(buildIn(i)); if (!q.ok) continue; int trap = 0; for (const std::string& x : q.ex) { if (x.find("buyers trapped") != std::string::npos) trap = 1; if (x.find("sellers trapped") != std::string::npos) trap = -1; }
                        e.bsv[t] = std::make_pair(q.verdict, trap); e.bsvDirty = true; ++added; }
                    while (e.bsv.size() > 900) e.bsv.erase(e.bsv.begin());
                    const long long nowB = rvh::PocStore::nowMs();
                    if (e.bsvDirty && (e.bsvMs < 0 || nowB - e.bsvMs >= 10000 || nowB < e.bsvMs)) { e.bsvMs = nowB; const std::string flex = rvh::flexDir();
                        if (!flex.empty() && !mk.empty()) { std::string t = "BSV|1|" + mk + "|" + std::to_string((long long)S.spb) + "|" + RVS_VERSION + "\n"; for (const auto& kv : e.bsv) t += std::to_string(kv.first) + "|" + std::to_string(kv.second.first) + "|" + std::to_string(kv.second.second) + "\n";
                            const std::string path = rvh::recDir(flex) + "\\bsv-" + mk + "-" + std::to_string((long long)S.spb) + ".txt", tmp = path + ".tmp"; bool ok = false; { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (f) { f << t; f.flush(); ok = f.good(); } }
#if defined(_WIN32)
                            if (ok && !::MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) { std::remove(tmp.c_str()); ok = false; }
#else
                            if (ok && std::rename(tmp.c_str(), path.c_str()) != 0) { std::remove(tmp.c_str()); ok = false; }
#endif
                            if (ok) e.bsvDirty = false; } } }
                const bsr::Out bo = bsr::read(bi); foHeadQ = bo.headQ; foExQ = bo.exQ; fo.verdict = bo.verdict; fo.head = bo.ok ? bo.head : std::string("No volume at price for this candle"); fo.ex = bo.ex; storySent.clear(); }   // (0.16.0, Rassul 21:15-21:17 CT "more human readable and simplified", "lets do this") three plain lines, no table
            {   // (0.17.0) a lettered candle reads its Wyckoff step: the candle under the mouse, or (no mouse) the newest letter of the last 3 candles
                int wb = -1; const int hbW = S.hov.hover.bar;
                if (hbW >= 0 && hbW < (int)count - 1) { if (e.wy.reads.count((long long)times[hbW])) wb = hbW; }
                else for (int k = (int)count - 2; k >= 0 && k >= (int)count - 4; --k) if (e.wy.reads.count((long long)times[k])) { wb = k; break; }
                if (wb >= 0 && !RV_DRAW_BOOKFLOW) { if (wb == cbBar) { const wyr::Read& wr = e.wy.reads.at((long long)times[wb]); if (!wr.head.empty()) { fo.ex.push_back("Strip: " + wr.head); if (!foExQ.empty()) foExQ.push_back(std::string()); } } }   // (0.29.0) the strip letter's step as one more line of the same candle
                else if (wb >= 0) { const wyr::Read& wr = e.wy.reads.at((long long)times[wb]); fo = cbr::Out(); fo.head = wr.head; fo.verdict = wr.dir > 0 ? 1 : wr.dir < 0 ? -1 : 0;
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
                // (0.30.0) the volume-model question column (the buy / sell read): full questions when the read keeps 220 px, short ones when 150, else none
                const bool hasQ = !foHeadQ.empty() && foExQ.size() == fo.ex.size();
                int qW = 0; bool qFull = false;
                auto shortQ = [](const std::string& q) { return q == "2 Who hit whom?" ? std::string("2 Who hit?") : q; };
                if (hasQ) { int wf = tw(foHeadQ), ws = tw(shortQ(foHeadQ)); for (const std::string& q : foExQ) { if (q.empty()) continue; wf = std::max(wf, tw(q)); ws = std::max(ws, tw(shortQ(q))); }
                    if (cl.leftW - (wf + 8) >= 220) { qW = wf + 8; qFull = true; } else if (cl.leftW - (ws + 8) >= 150) qW = ws + 8; }
                const int xQ = xT, xR = xT + qW, lw = cl.leftW - qW; std::string lastQ;
                auto tagQ = [&](const std::string& q, int yy) { if (!qW || q.empty() || q == lastQ) return; lastQ = q; put(qFull ? q : shortQ(q), xQ, yy, rvh::C_DIM, xR - 2); ++e.qTags; };
                const std::string lead = when.empty() ? fo.head : when + " " + fo.head;
                const std::vector<std::string> HL = spt::headLines(lead, fo.why, lw, std::max(1, std::min(2, fit - 1)), tw); const int nh = (int)HL.size();
                for (int i = 0; i < nh; ++i) { const int yy = y0 + i * rowH; const std::string& h = HL[(size_t)i];
                    if (i == 0) tagQ(foHeadQ, yy);
                    if (i == 0 && h.compare(0, lead.size(), lead) == 0) { int xx = xR;
                        if (!when.empty()) { put(when, xx, yy, rvh::C_GRAY, lim + 1); xx += tw(when + " "); }
                        put(fo.head, xx, yy, vc, lim + 1); xx += tw(fo.head + " ");
                        if (h.size() > lead.size() + 1) put(h.substr(lead.size() + 1), xx, yy, rvh::C_WHITE, lim + 1); }
                    else put(h, xR, yy, rvh::C_WHITE, lim + 1); }
                std::vector<std::string> SS = fo.ex; if (!storySent.empty() && storySent.compare(0, 6, "Story ") != 0) SS.push_back(storySent);   // (0.16.0) the Rev checks stay; the long climax story sentence goes   // the story / Rev checks last
                std::vector<std::string> EX;
                if (!hasQ) { EX = spt::wrap(SS, lw, fit - nh, tw); for (size_t i = 0; i < EX.size(); ++i) put(EX[i], xR, y0 + (int)(i + (size_t)nh) * rowH, rvh::C_WHITE, lim + 1); }
                else for (size_t k = 0; k < SS.size(); ++k) {   // sentence by sentence: each question on the first line of its sentence
                    const int left = fit - nh - (int)EX.size(); if (left <= 0) break;
                    const std::vector<std::string> W = spt::wrap(std::vector<std::string>(1, SS[k]), lw, left, tw); if (W.empty()) break;
                    for (size_t i = 0; i < W.size(); ++i) { const int yy = y0 + (int)(EX.size() + (size_t)nh) * rowH; if (i == 0 && k < foExQ.size()) tagQ(foExQ[k], yy); put(W[i], xR, yy, rvh::C_WHITE, lim + 1); EX.push_back(W[i]); } }
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
            read += " || big-order bars " + std::to_string(e.bigBars) + " | question tags " + std::to_string(e.qTags);   // (0.30.0) counts of the last draw (the status)
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
    p->setDescription("LRA Reversal Read 0.21.2 - a letter strip at the top, at most 2 per candle: Sp spring / Ut upthrust, I the other side took over at a key level (not on a Sw candle), T trapped traders, A effort without result, E lack of demand / supply, B a break of a key level (the price chart's Bk), C a climax candle (Cx) (green = buyers / a low, red = sellers / a top); then Book Flow (resting orders added - pulled: bids green, offers red); bottom: delta (bright) and its change from the candle before (dim), green = buyers, red = sellers, a ratio over a change of 1.33x+ normal; one read right of the last candle combines both. Earlier: two bars per candle = its buy volume (green) and sell volume (red); a number on a bar = the biggest in that many candles (10 or more); letters C climax (red = selling, green = buying), A absorbed, E exhausted, I initiation; the read (right of the last candle) = the six checks of the newest pullback / Rev (Rev N: buying fading, big buyer trapped, quick rejection, bids pulled, big seller, with trend), else the reversal behaviours of the candle under the mouse. Market-by-order feed.");
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
    long long wyCk = -100000; int wyRead = -1; long long iDrawn = 0;
    std::string liveSig; long long liveT = 0; wyi::M liveM;
    std::string tabSig; std::map<long long, wyi::TurnAbs> tabs; };   // (0.27.0) lsTurn's P / Ex? per Abs candle (RevRead\\turnabs-<MKT>-<spb>.txt)   // (0.25.0) the live Cx? (RevRead\\wylive-<MKT>-<spb>.txt): its candle stamp and mark   // the I letters (candle stamp -> +1 / -1) from RevRead\\wy-<MKT>-<secs>.txt, looked at every 3 s, re-read only when its size / time changed
class RevReadMarks : public cppExtension {
public:
    HostSlot<RmState> slot_;
    // the I letters the pane published for this market and candle size (bounded 1 MB read; a torn / foreign file keeps the last good set)
    void pump() {
        RmState* sp = slot_.get(this, true); if (!sp) return; RmState& s = *sp;
        const long long now = rmSteadyMs(); if (now - s.wyCk < 3000 && now >= s.wyCk) return; s.wyCk = now;
        const char* rs = getRootSymbol(); const char* sy = getSymbol(); const std::string mk = rvh::marketOf(rs ? rs : "", sy ? sy : ""); const long spb = (long)getSecondsPerBar();
        const std::string flex = rvh::flexDir(); if (flex.empty() || mk.empty() || spb <= 0) return;
        try { pumpLive(s, flex, mk, spb); } catch (...) { s.liveT = 0; }   // (0.25.0)
        try { pumpTurnAbs(s, flex, mk, spb); } catch (...) { s.tabs.clear(); s.tabSig.clear(); }   // (0.27.0)
        const std::string path = rvh::recDir(flex) + "\\wy-" + mk + "-" + std::to_string((long long)spb) + ".txt";
        struct stat st; if (stat(path.c_str(), &st) != 0) { if (!s.wySig.empty()) { s.wySig.clear(); s.iMarks.clear(); } s.wyRead = -1; return; }
        const std::string sig = path + ":" + std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime); if (sig == s.wySig) return;
        if ((size_t)st.st_size > wyi::MAX_BYTES) { s.wyRead = -2; return; }
        std::ifstream f(path.c_str(), std::ios::binary); if (!f) return;
        const std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::map<long long, std::vector<wyi::M> > m; const int n = wyi::parseAll(txt, m); if (n < 0) return;
        s.iMarks.swap(m); s.wySig = sig; s.wyRead = n;
    }
    // (0.27.0) lsTurn's line per Abs candle: the biggest print (line 3) and the same-candle Ex? (line 1); a missing / foreign file = none
    void pumpTurnAbs(RmState& s, const std::string& flex, const std::string& mk, long spb) {
        const std::string path = rvh::recDir(flex) + "\\turnabs-" + mk + "-" + std::to_string((long long)spb) + ".txt";
        struct stat st; if (stat(path.c_str(), &st) != 0 || st.st_size > 262144) { s.tabSig.clear(); s.tabs.clear(); return; }
        const std::string sig = std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime); if (sig == s.tabSig) return;
        std::ifstream f(path.c_str(), std::ios::binary); if (!f) return;
        const std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::map<long long, wyi::TurnAbs> m; if (!wyi::parseTurnAbs(txt, mk, m)) return;
        s.tabs.swap(m); s.tabSig = sig;
    }
    // (0.25.0) the live Cx? the pane wrote for this market and candle size: one "stamp|Cx?|dir" line or none (a missing / foreign file = none)
    void pumpLive(RmState& s, const std::string& flex, const std::string& mk, long spb) {
        const std::string path = rvh::recDir(flex) + "\\wylive-" + mk + "-" + std::to_string((long long)spb) + ".txt";
        struct stat st; if (stat(path.c_str(), &st) != 0 || st.st_size > 4096) { s.liveSig.clear(); s.liveT = 0; return; }
        const std::string sig = std::to_string((long long)st.st_size) + ":" + std::to_string((long long)st.st_mtime); if (sig == s.liveSig) return;
        std::ifstream f(path.c_str(), std::ios::binary); if (!f) return;
        const std::string txt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        s.liveSig = sig; s.liveT = 0; long long t = 0; int d = 0;
        if (wyi::parseLive(txt, mk, t, d)) { s.liveT = t; s.liveM = wyi::M(); s.liveM.code = 'C'; s.liveM.dir = d; }
    }
    virtual int draw(void) {
        LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
        RmState* sp = slot_.get(this, false); if (!sp) return RTX_OK; RmState& s = *sp;
        try {
            if (s.iMarks.empty() && s.liveT <= 0) return RTX_OK;
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
                const auto it = s.iMarks.find((long long)times[i]);
                if (s.liveT > 0 && (long long)times[i] == s.liveT) {   // (0.25.0) the live Cx?: a badge like the Cx, "Cx?", no level / strength; not where a Cx already is
                    bool hasCx = false; if (it != s.iMarks.end()) for (const wyi::M& m : it->second) if (m.code == 'C') hasCx = true;
                    if (!hasCx) { setFont(f1); wyi::Want wt; wt.candle = (int)C.size() - 1; wt.side = wyi::sideOf(s.liveM); wt.w = (int)getTextWidth("Cx?", -1) + 2 * wyi::PAD_X; wt.h = wyi::LINE1_H;
                        W.push_back(wt); WM.push_back(&s.liveM); l1.push_back("Cx?"); l2.push_back(std::string()); l3.push_back(std::string()); } }
                if (it == s.iMarks.end()) continue;
                for (const wyi::M& m : it->second) {
                    if (m.code == 'S' || m.code == 'A' || m.code == 'D') continue;   /* (0.44.0) D = the thrust dot, drawn below as a dot */   // (0.43.0, Rassul 2026-11-27 09:50 CT "consolidate AR and Vob (t) into a new BI") VOB / VOT and AR are still written (the nightly scores them) but no longer drawn - the BI replaces them
                    const wyi::TurnAbs* tz = nullptr; if (m.code == 'Q') { const auto tq = s.tabs.find((long long)times[i]); if (tq != s.tabs.end() && tq->second.dir == m.dir) tz = &tq->second; }   // (0.27.0)
                    const std::string a1 = m.code == 'Q' ? wyi::absLine1(tz) : m.code == 'S' ? std::string(m.dir > 0 ? "VOB" : "VOT") /* (0.36.0) was "Sw" */ : std::string(wyi::textOf(m.code)), a2 = m.lv, a3 = m.code == 'C' ? std::string() : m.code == 'Q' ? wyi::absLine3(tz) : m.st;   /* (0.27.0) Abs: "Abs" / "Abs/Ex?", position, P */ setFont(f1); int w = (int)getTextWidth(a1.c_str(), -1); if (!a2.empty()) { setFont(f2); w = std::max(w, (int)getTextWidth(a2.c_str(), -1)); }
                    if (!a3.empty()) { setFont(f2); w = std::max(w, (int)getTextWidth(a3.c_str(), -1)); }   // (0.19.2) the strength: a 3rd line, same font as the level
                    wyi::Want wt; wt.candle = (int)C.size() - 1; wt.side = wyi::sideOf(m); wt.w = w + 2 * wyi::PAD_X; wt.h = wyi::LINE1_H + (a2.empty() ? 0 : wyi::LINE2_H) + (a3.empty() ? 0 : wyi::LINE2_H); W.push_back(wt); WM.push_back(&m); l1.push_back(a1); l2.push_back(a2); l3.push_back(a3); } }
            {   // (0.44.0, Rassul 2026-11-27 12:39 CT "put the cyan dot ... in the candle in the middle") the thrust dot: cyan, in the middle of the candle's body
                const COLOR TD_COL = (COLOR)0x0022D3EE;   // cyan #22D3EE (0x00RRGGBB, as C_GREEN)
                const int rr = std::max(2, std::min(4, dx * 32 / 100 + 1));
                for (size_t q = 0; q < C.size(); ++q) { const int i = barOf[q]; const auto it = s.iMarks.find((long long)times[i]); if (it == s.iMarks.end()) continue;
                    bool has = false; for (const wyi::M& m : it->second) if (m.code == 'D') has = true; if (!has) continue;
                    const int ym = (int)getVerticalDrawPosition((float)(((double)op[i] + (double)cl[i]) / 2.0)); const int xm = C[q].x;
                    setPen(TD_COL, 1, P_SOLID); CBRUSH bb(TD_COL, PAT_SOLID); bb.set(); RCT o; o.set((short)(xm - rr), (short)(ym - rr), (short)(xm + rr + 1), (short)(ym + rr + 1)); o.drawOval(DRAW_OPAQUE); } }
            if (W.empty()) return RTX_OK;
            // (0.19.0, Rassul 10:51 CT "remember placement of other signals like cmx so you have to coordinate everything") the BO indicator's letters
            // as drawn (SignalBoard 'L': candle, side, px from the extreme to the letter's outer edge) are taken boxes: our tags go around them
            std::vector<wyi::Box> taken; std::set<int> cobRev;
            try { const char* sy = getSymbol(); const char* chl = getChartLabel(); std::vector<sb::Ev> ev;
                // (0.19.1, Rassul 11:13 CT "ok": lsZzoComboMarks 'Z' (SR / CB35 / CB99) and lsCobSignal 'K' (Cob..., Lvp, Rev) publish their labels as drawn too)
                for (const char kind : { 'L', 'Z', 'K' }) { ev.clear();
                if (sb::readAll(kind, sy ? sy : "", getSecondsPerBar(), chl ? chl : "", rmSteadyMs(), ev) && !ev.empty()) {
                    std::map<long long, size_t> at; for (size_t q = 0; q < barOf.size(); ++q) { struct tm m; std::memset(&m, 0, sizeof m); if (!getLocaltime((RTDATE)times[barOf[q]], &m)) continue;
                        at[wyi::civilDays(m.tm_year + 1900, (unsigned)(m.tm_mon + 1), (unsigned)m.tm_mday) * 86400LL + m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec] = q; }
                    for (const sb::Ev& x : ev) { const auto it = at.find(x.bar); if (it == at.end() || !(x.val > 0) || x.val > 400) continue; const wyi::Candle& cd = C[it->second];
                        if (kind == 'K' && x.flag == 7) cobRev.insert((int)it->second);   // (0.20.0) a Cob Rev on this candle: one Rev per candle
                        wyi::Box b; sb::tagBox(x, cd.x, cd.yHigh, cd.yLow, b.l, b.t, b.r, b.b); taken.push_back(b); } } } } catch (...) {}
            if (!cobRev.empty()) {   // (0.20.0, Rassul 16:03 CT "thats fine") a Tst Rev on a candle where lsCobSignal drew its Rev is not drawn again
                std::vector<wyi::Want> W2; std::vector<const wyi::M*> WM2; std::vector<std::string> a1, a2, a3;
                for (size_t k = 0; k < W.size(); ++k) { if (WM[k]->code == 'R' && cobRev.count(W[k].candle)) continue; W2.push_back(W[k]); WM2.push_back(WM[k]); a1.push_back(l1[k]); a2.push_back(l2[k]); a3.push_back(l3[k]); }
                W.swap(W2); WM.swap(WM2); l1.swap(a1); l2.swap(a2); l3.swap(a3); }
            const std::vector<wyi::Box> P = wyi::layout(C, W, (int)pane.top, (int)pane.bottom, xLast, taken);
            double rmTick_ = (double)getProperty(SYM_TICKINCR); if (!std::isfinite(rmTick_) || rmTick_ <= 0 || rmTick_ >= 1000) rmTick_ = 0;   // the only tick source
            for (size_t k = 0; k < W.size(); ++k) { const wyi::Box& g = P[k]; if (!g.ok) continue; const wyi::M& m = *WM[k];
                const COLOR col = m.dir > 0 ? rvh::C_GREEN : rvh::C_RED;
                const bool plain = wyi::plainOf(m);   // (0.20.0) FT = a plain coloured letter; every other mark a badge (black text)
                if (!plain) { RCT r; r.set(rvh::coord(g.l), rvh::coord(g.t), rvh::coord(g.r), rvh::coord(g.b)); r.draw(0, col, col, DRAW_OPAQUE, PAT_SOLID); }
                setTextColor(plain ? col : rvh::C_TAG_TEXT); setFont(f1); RCT t1; t1.set(rvh::coord(g.l), rvh::coord(g.t), rvh::coord(g.r), rvh::coord(g.t + wyi::LINE1_H)); t1.drawText(l1[k].c_str(), true, false);
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
    p->setDescription("LRA Reversal Read marks 0.21.2 - Bk / Sw / I / Cx on the candles with their level and strength (I under a bullish candle, over a bearish one; a Sw candle shows only the Sw), and after a Bk its FT, Tst (level + A+E / E / A!) and Rev (+ strength); green = bullish, red = bearish. Needs the Reversal Read pane on the same chart. No settings.");
    rvh::stamp(p);   // RVS_VERSION
    return p;
}
#endif
