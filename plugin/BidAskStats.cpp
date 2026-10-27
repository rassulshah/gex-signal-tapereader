/********************************************************************************
 *  BidAskStats.cpp  --  Investor/RT RTX extension  lsBidAskStats (the "Aggression" pane)  1.15.0 (2026-10-18)
 *  1.28.0 (2026-10-22, Rassul approved layout-stages-2strips.png 11:41-11:46 CT, build ordered 15:20 CT): the badge row becomes TWO LETTER
 *        STRIPS (LetterStrips.h 1.0.0), each its own row with a small left label: "Attack" (stage 1: S > K > L) and "Running out" (stages
 *        3-4: R > Tr > G). ONE letter per strip per candle (the strongest by that fixed priority; the others in the hover), coloured by the
 *        side that ACTS (buyers green, sellers red: buyers attacking a high = green, sellers at a low = red), no boxes / borders, no grey
 *        sub-text; a letter that would touch its neighbour drops to the strip's lower half-row. The shared A is no longer drawn here (Defense
 *        pane only). Hovers per strip (the existing per-letter blocks). Every letter is also appended to lsFlexLevels\Strips\ATK-<MKT>-<spb>.csv
 *        (first line per candle + letter wins) for the Reversal Stage pane (lsReversalStage 1.0.0). Histogram, box, records: unchanged.
 *  1.26.0 (2026-10-21 20:11-20:23 CT, Rassul approved attacker-mockup-v2.png - "the attacker pane"): question "Who is aggressive?" (was
 *        "Is the effort rewarded?"); a BADGE ROW under it, above the bars (AttackLogic.h 1.0.0), one badge per closed candle, at most 4 letters
 *        (the font shrinks to fit, the rest in the hover), colour by meaning (bearish red / bullish green: buyers attacking or giving up at a
 *        high = red, sellers at a low = green), no "?": S = the biggest sweep of the leg (one aggressor order >= 3 prices, bigger than every
 *        sweep on that side in the 10 candles before) and K = one aggressor order >= Big Lot S10 (no initiative candle), both at a 5-candle
 *        high / low; G (attackers giving up) and Tr (trapped) MOVED from lsBookFlow - read from its turns file G lines and the Delta Profile's
 *        TR record (their rules and records unchanged); R = the stop run; L = one order >= S20 but < S10 (K replaces it). E (fading) and
 *        speeding up: hover only. The badge HOVER: one block per letter (question, what happened with the record numbers, Use, Example, Tested,
 *        the combination with the Defense letter on the same candle - e.g. "with Defense A: K + A held 61% vs 47% (n 49, learning)" - and
 *        "Status: still learning, n = .."), inside the pane, never over the info box. Decided once at the close: AS / K / S lines in the session
 *        record (reloaded after a restart, never repainted). The message / read line carries no "?". OrderFlowFeed 1.13.0.
 *  1.15.0 (2026-10-18 23:48 CT, Rassul approved aggr-read-v2-mockup.png): box rows Lots, Pace, BBI / BAI, READ - one word (BULLISH green /
 *        BEARISH red / NEUTRAL light grey) + the read in up to 2 lines that always fit the box (bas::aggRead, BoxRead.h / BoxReadDraw.h): who
 *        is attacking, fast, biggest in N, the other side quiet. The BL5 row is removed (the Big Lot cyan cap and its setting stay). The box's
 *        value columns are sized on the Lots / Pace values (no longer the wide Crowd template the BBI / BAI row picked up in 1.14.2).
 *  1.14.2 (2026-10-18 23:34 CT, Rassul approved aggr-bbi-mockup.png): box row "BBI / BAI" under Pace, above BL5 - Sellers = BBI (Biggest
 *        Bid In) = how many prior candles in a row this candle's biggest bid beats (strictly; an equal or bigger one stops it), Buyers = BAI
 *        the same for the ask, up to 50 ("50+"). Same hover rule as 1.14.1: the candle under the mouse inside the pane, else the live
 *        forming candle (updating with each trade, final at its close).
 *  1.14.1 (2026-10-18 23:19 CT, Rassul: "when i move the mouse outside the aggression pane the updated values should still be displayed
 *        in the info box"): ROOT CAUSE - IRT sends mouse events only while the mouse is over this pane and nothing when it leaves; the
 *        hovered candle stayed latched until the cursor left the whole chart window (mouseLeft) and also followed the candle other panes
 *        published (HoverBoard), and "Live" showed the last CLOSED candle. FIX (PaneCursor.h): the box follows the mouse ONLY while the
 *        cursor is inside this pane (screen cursor vs the pane rectangle located at the last mouse event, checked on every draw / calc);
 *        anywhere else it shows LIVE = the forming candle (Lots and ratio vs the prior average, Pace from the seconds elapsed so far, the
 *        Big Lot size), refreshed whenever its numbers change. The forming bar on the chart is still dim and unlabelled (no repaint).
 *  1.14.0 (2026-10-18, Rassul 17:26 CT "implement Big lot 5 style on the aggression indicator. I want to see if it will help identify
 *        spikes near the hod and lod"; study AGGR-BL5.md: bars >= BL5 sit within 2 bars of the final HOD / LOD 1.8x (RTH) / 2.9x
 *        (overnight) as often as other bars - an EXTRA mark, the 2.0x spike rule unchanged): BIG LOT MARK. A closed candle whose bar (the
 *        most lots at one price, one side - the same quantity as a lsBigBidAsk print) is >= the Big Lot size of its session type (RTH /
 *        overnight) gets a 3 px CYAN CAP on top of its bar; the bar keeps its green / red, bright / dim and its ratio label. Setting
 *        "Big Lot mark" Off / BL5 (default) / BL10 / BL20. The sizes (BigLotTiers.h = the lsBigBidAsk 1.16 box rule): the average of each
 *        complete earlier session's 5th / 10th / 20th biggest print of that type, latest 5 sessions, one outlier dropped; under 3 complete
 *        sessions = learning (no cap). Decided ONCE per session and type and frozen with each candle at its close (never repainted; after
 *        a restart the session's tiers and marks are read back). The numbers box adds one row "BL5  311" (the tier of the candle shown).
 *        Recorded: lsFlexLevels\BidAskStats\BigLot-<MKT>-<spb>.txt (complete sessions S| + the tiers decided T|) and
 *        BidAskStats\<stem>-<session>-biglot.txt (every closed candle >= BL20: lots, side, ratio, spike, reached tier) for the nightly;
 *        status file BIG_LOT_* lines.
 *  1.13.0 (2026-10-18, Rassul approved the audit-A fixes): (1) NO REPAINT - only CLOSED candles are judged: the forming candle is drawn
 *        dim with no label until it closes (was: its bright colour / ratio label came and went intrabar). (2) ONE SPIKE RULE for both
 *        order-flow panes (Aggression, Book Flow): a side is a spike (bright + ratio / pace above the bar) when it is >= "Spike vs average"
 *        (default 2.0x) the plain average of that side over the prior "Ratio bars" (20) closed candles, decided once at the close. The
 *        "1.5x the biggest of the prior 10" jump is no longer part of it (kept as the Jump record / status field); a 1.12 record row keeps
 *        its colour as drawn then. (3) Settings: "Spike vs prior biggest", "Spike lookback" and "Box shows" (did nothing) removed
 *        (parameter version 5); the description says what is drawn. (4) MARKET STATE (MarketState.h, MARKET-STATE-SPEC.md 1.0): the facts
 *        of the last closed candle posted as SRC "AGG" (buy_lots, sell_lots, buy_x, sell_x, spike, pace_x, sweep, big_aggr, big_tier) once
 *        per closed candle, re-posted unchanged every 15 s so it stays fresh, cleared on removal. Behind the scenes only.
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
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "BidAskStats"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "HostSlot.h"
#define OFF_SDK 1
#include "BidAskStatsLogic.h"
#include "OrderFlowPaneSettings.h"   // (1.13.2) the settings window table (SettingsSpec.h)
#include "SettingsHost.h"
static ss::Ids& basIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
#include "StopRun.h"
#include "OptionsFlow.h"
#include "HoverBoard.h"
#include "AwareHook.h"                // (1.34.0) the smart-read hook (layer L3)
#include "SelfAware.h"           // (1.35.0) self-awareness
#include "MarketState.h"              // (1.13.0) SRC "AGG" facts for the read
#include "SessionVWAPLogic.h"         // (1.14.0) svl::rthOf / classify (the session and its RTH)
#include "BigLotTiers.h"              // (1.14.0) the Big Lot sizes per session type
#include "PaneCursor.h"               // (1.14.1) the box follows the mouse only while the cursor is inside this pane
#include "BoxReadDraw.h"
#include "SignalBoard.h"              // (1.25.0) posts the order-book absorption prices (kind 'O')
#include "EffortReward.h"            // (1.16.0) Attack Effort + Reward rows and their read
#include "LetterStrips.h"            // (1.28.0) the two letter strips + the letter record for lsReversalStage
#include "ReversalEvidence.h"        // additive typed sidecar; legacy LT strips stay unchanged
#include "AttackLogic.h"             // (1.26.0) the attacker letters S / K / G / Tr / R / L: badge row, hover, K / S / AS lines
#include "SharedAbsorb.h"            // (1.27.0) the SHARED absorption A: decided here at the close, one record for every plugin
#include "DefenseLinesSettings.h"    // (1.26.0) dlset::turnsPath - lsBookFlow's turns file (G lines + the Defense letters, read only)
#include <array>              // (1.15.0) the Read row
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

#define BAS_VERSION "1.36.0"          // 1.36.0 (2026-10-26, Rassul 18:33 CT) no letters drawn: the S strip, the bar letters and the stop-run R / R? mark are recorded only; the strip row goes to the bars; "S letter" row hidden
// prev:          // (1.35.0, 2026-10-26) self-awareness SELFAGG (SelfAware.h, SELF-AWARE.md; ships OFF per market) // 1.34.0 (2026-10-26) smart-read hook (AwareHook.h 0.1.0, TEAM-DESIGN.md): posts AWAGG for each closed bar once lsContext has posted CTX for it (or after 3 s), records the isolated + aware read in lsFlexLevels\\Context-AWR-<MKT>-<spb>s-<day>.rec; links OFF per market (LRA-Context-<MKT>.csv) so aware = isolated and nothing drawn changes (facts only: no side of its own yet); // 1.33.0 (2026-10-25, BUILD-SPEC "Attacker pane: bars + S only", Rassul approved 22:29 CT) the pane draws the buy / sell bars and ONE letter strip with the S letter only (one order swept 3+ prices); K / L / R / Tr / D / E / G / PB are still decided and written to the letter + EV records (the Reversal Stage and the nightly read them) but not drawn; the E pace stays in the S hover; new settings row "S letter" (file field 10); // 1.32.0 (2026-10-23 4-step stage revamp) new D letter (MBO CVD divergence, Trapped strip) + the attack average fill " vw" in S / K / L nums for the Reversal Stage underwater measure; // was 1.31.0:          // 1.30.0 (2026-10-22 evening fix build) compact strips (LetterStrips 1.1.0: 11 px per strip), bars >= 50% of the pane (OrderFlowFeed 1.15.0 pairPx floor), the Implied row one line "Implied: 0% of volume";          // keep equal to bas::VERSION and the setVersion literal below

static const COLOR C_ASK = 0x0022C55E, C_BID = 0x00EF4444, C_LABEL = 0x00FFFFFF, C_BORDER = 0x009CA3AF, C_BLACK = 0x00000000;
static const int WINDOW_BARS = 3000;          // closed bars read back from the last one (BBid is capped at the bars available)
static const int READS_PER_PASS = 600;        // bounded work per pass; the rest continues on the next pass
static const int NA_TRIES = 10;               // retries of a closed n/a bar (IRT back-fill)
static const size_t KEEP_BARS = 5000;
// (1.13.2) the window rows come from OrderFlowPaneSettings.h (no index constants)
//   // (1.13.0) parameter version 5: the jump, lookback and Box shows rows removed
static const COLOR C_ASK_BAR = 0x001F9D55, C_BID_BAR = 0x00962D2D;     // normal histogram bars: darker green / red
static const COLOR C_ASK_SPIKE = 0x0000FF00, C_BID_SPIKE = 0x00FF3B3B; // spikes: bright green (buyers) / bright red (sellers)
static const COLOR C_BIG_LOT = 0x0000FFFF;                            // (1.14.0) the Big Lot cap: cyan = Big Lot in lsBigBidAsk too
static const int BIG_LOT_CAP_PX = 3;
static const int BAS_BADGE_H = 0;   // (1.35.0, Rassul 18:33 CT "remove all the signal letters") no strip: the row goes to the bars; was 1 * lst::ROW_H + 2   // (1.33.0) ONE strip (S only); was 2 * lst::ROW_H + 2 -   // (1.28.0) the two letter strips under the question line (bars start below them); 1.26.0: one 16 px badge row
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
struct BasPanel { int dx = 0, dy = 0, mode = BAS_DEFAULT_MODE, bigLot = blt::CHOICE_DEFAULT, showS = 1; };   // (1.33.0) field 10: S letter 0 / 1   // (1.14.0) field 9: Big Lot mark 0-3       // (1.5.2) fields 6-8
static bool parseSettings(const std::string& line, int& rb, int& pos, bas::SpikeCfg* spike = nullptr, BasPanel* panel = nullptr) {
    if (line.empty() || line.size() > 54) return false;   // (1.33.0) 52 -> 54: field 10
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = line.find(',', a); f.push_back(line.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    BasPanel bpn;
    int showS = 1;
    if (f.size() == 10) {                                                  // (1.33.0) + S letter (a 9-field file = shown)
        if (f[9] != "0" && f[9] != "1") return false;
        showS = f[9] == "1" ? 1 : 0; f.resize(9);
    }
    bpn.showS = showS;
    if (f.size() == 9) {                                                   // (1.14.0) + Big Lot mark (an 8-field file = the default BL5)
        if (f[8].size() != 1 || f[8][0] < '0' || f[8][0] > '3') return false;
        bpn.bigLot = f[8][0] - '0'; f.resize(8);
    }
    if (f.size() == 8) {                                                   // (1.5.2) Offset X, Offset Y, Box shows
        if (!bp::intTok(f[5], bpn.dx) || !bp::intTok(f[6], bpn.dy) || !bp::offOk(bpn.dx) || !bp::offOk(bpn.dy) || (f[7] != "0" && f[7] != "1")) return false;
        const int bl = bpn.bigLot; bpn.mode = f[7] == "1" ? off::MODE_NUMBERS : off::MODE_READ; bpn.bigLot = bl; bpn.showS = showS; f.resize(5);
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
    // (1.13.0) the jump / lookback no longer decide a spike (record field only): never let an old 0 there switch spikes off
    off::SpikeCfg o; o.avgT = d.avgT; o.jumpT = d.jumpT > 0 ? d.jumpT : bas::SPIKE_JUMP_DEFAULT_TENTHS; o.lookback = d.lookback > 0 ? d.lookback : bas::SPIKE_LOOKBACK_DEFAULT;
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
    std::vector<sb::Ev> obEv; long long obMs = -1, obPosts = 0, obMarks = 0;   // (1.25.0) order-book absorption per decided candle (board kind 'O')
    off::IconRect infoIcon; bool infoTipOn = false;                        // (1.22.0) the info icon (pane pixels) and its hover tip
    std::string identity, market, symbol, state = "starting", lastStatusText;
    int spb = 0; double tick = 0;
    bas::Series ser;                                 // closed bars: time -> stats (computed once)
    std::map<long long, int> naTries;                // closed n/a bars still being retried: time -> attempts
    long count = 0; long long lastStamp = 0; bool backlog = true;
    long long lastClosedTime = 0;                    // the last CLOSED bar's stamp
    long long liveTime = 0; bas::BarMax live;        // the forming bar (drawn only)
    long long liveReadMs = -1, retryMs = -1, statusMs = -1, settingsCheckMs = -1, settingsStamp = -2; std::string settingsText = "\x01";   // (1.14.0) the line last applied
    br::Read lastRead; std::vector<std::string> lastReadLines;            // (1.15.0) the Read last drawn (status / tests)
    pc::Tracker cur; long long liveDrawnT = -1, liveDrawnBid = -1, liveDrawnAsk = -1;   // (1.14.1) the pane on the screen; the live numbers last drawn
    int ratioBars = bas::RATIO_DEFAULT, pos = bas::POS_DEFAULT;
    int dx = 0, dy = 0, mode = BAS_DEFAULT_MODE;     // (1.5.2) Offset X / Y, Box shows
    bp::Box lastBox; bool inBox = false; long long boxHovers = 0;   // (1.5.2) the read box last drawn; the mouse over it
    bas::SpikeCfg dlgSpike;                          // (1.3.0) the dialog's spike settings (the params file may override)
    bas::Hover hover;                                // the closed bar under the mouse, -1 = Live
    std::vector<void*> hoverWnds;                    // windows the chart's mouse events came from (most recent last)
    long long redraws = 0;
    long long axisTop = 0, axisUp = -1, axisDown = -1; int paneH = 0;   // (1.16.0) axisUp / axisDown: the biggest buy / sell on screen (mirror bars)
    long long vapReads = 0, vapFailures = 0, vapMismatch = 0;
    // (1.3.0) the order feed, the attack reads, the session record, params, levels
    off::Conn feed; off::Engine e; off::Acc scratch; nl::Zone zone; double atr = 0; long atrCount = -1;
    std::map<long long, off::Acc> accs; std::map<long long, long long> closedAt;
    std::map<long long, int> barEnds;                // bar end -> chart index (the last 3,001 bars)
    long long formingKey = 0, firstBarKey = -1, finFrom = 0;
    long long lateDropped = 0, noBar = 0, decidedLate = 0, finalized = 0;
    std::map<long long, BasFin> fin;
    off::SessionFile file; std::string curSession; bool loaded = false;
    long long llpSaveMs = -1000000; std::string llpText[llp::NCH];   // (2026-10-17) aggressor-order large-lot pool files: last save, last text written
    nl::Cache lv; std::vector<nl::Level> levels;
    off::ParamCache params;
    off::ImpCache impC;                              // (1.29.0) LRA-Implied-<MKT>.csv -> e.ip (implied fills excluded by default)
    // (1.4.0) the stop run: the detector (closed bars in order), its params file, the events by break bar
    sr::Detector det; sr::ParamCache srp; long long srFrom = -1, srCursor = 0;
    std::map<long long, std::vector<sr::Event> > srEv;   // break bar time -> events (at most one per direction)
    std::map<long long, long long> srReclaim;            // reclaim bar time -> break bar time (for its message / box)
    long long srEvents = 0, srLoaded = 0;
    off::OptCache opt;                                   // (1.5.0) LRA-Options-<MKT>.csv: the blue oHit line
    zread::Host zr; long zrCount = -1; long long zrSession = -1;   // (1.6.0) the zone read line
    // (1.13.0) Market State SRC AGG: the chart label (from the draw), the candle posted, its facts, the last post
    std::string chartKey; long long mstBar = 0, mstCt = 0, mstMs = -1, mstPosts = 0; std::map<std::string, std::string> mstKv; sa::Hook self; awh::Hook aw;   // (1.34.0) aw: the smart-read hook
    // (1.14.0) the Big Lot mark: the setting, the market's RTH, the history book (+ its file), each closed candle's mark decided once,
    // this session's record of candles >= BL20 (read back after a restart: the marks stay exactly as decided)
    int bigLot = blt::CHOICE_DEFAULT, openMin = 510, closeMin = 900; int showS = 1;   // (1.33.0) S letter
    blt::Book blBook; bool blLoaded = false; std::string blWritten; size_t blBarsN = (size_t)-1; long long blLast = -1;
    std::map<long long, blt::Mark> blMarks;
    std::string blSession; std::map<long long, std::string> blRows; std::map<long long, blt::Mark> blFile; std::string blRowsWritten; long long blRejected = 0;
    // (1.26.0) THE ATTACKER LETTERS: K / S decided once at each 5-candle high / low close (K| / S| lines), every order-feed candle's AS line
    // (sweeps, biggest orders, the frozen S20 / S10 / S5), the other panes' records read here (lsBookFlow G + Defense letters, the Delta
    // Profile's TR), the badge row's rectangles + hover texts, the badge under the mouse
    std::map<std::pair<int, long long>, atk::KRead> atkK; std::map<std::pair<int, long long>, atk::SRead> atkS; std::map<long long, atk::AS> atkAS;
    atk::Others atkO; std::map<std::string, long long> atkOff; long long atkReadMs = -1; std::string atkTurnsPath, atkTrPath;
    long long atkKn = 0, atkSn = 0, atkUndecided = 0, atkLoaded = 0, atkRejected = 0, badgesDrawn = 0, badgeDropped = 0, badgeTipNoRoom = 0, badgeTipCompact = 0;
    std::vector<std::pair<off::IconRect, std::string> > badgeHit; int badgeTip = -1; dt::TipRect lastTip; int lastTipLevel = -1;
    // (1.27.0) THE SHARED A (SharedAbsorb.h): the record host, candles already judged, the store size last copied to atkO.SA
    sab::Host sab; std::set<long long> sabDone; size_t sabSeen = (size_t)-1; bool sabDir = false;
    // (1.28.0) the letter record for lsReversalStage (lsFlexLevels\Strips\ATK-<MKT>-<spb>.csv): ids already in the file, counters
    std::set<std::string> stripIds; std::string stripPath; long long stripReadMs = -1, stripWritten = 0, stripFail = 0, stripsDrawn = 0, stripSkipped = 0;
    rse::Publisher evidence; std::string evidencePath; long long evidenceWritten = 0, evidenceFail = 0; // one publisher per host binding
    // (1.32.0) per finalized candle: the aggressive fills' average price x100 per RESTING side (BID = sellers hitting bids), from the
    // aggressor orders' fill ranges (each order at the middle of its lowest / highest fill, size-weighted); and the D divergences
    std::map<long long, std::pair<long long, long long> > fillVw; std::map<std::pair<int, long long>, std::pair<long long, long long> > atkD; long long dCount = 0;
};
// (1.23.0, Rassul 2026-10-20 07:43 "do it") ORDER-BY-ORDER TOTALS: each side's aggressive lots from the exchange's order feed (the
// aggressor order's own fills: bid hit = sellers, offer lifted = buyers) - what every tested turn rate was measured on (Databento MBO).
// A decided candle: its record (live then, or read back after a restart); a candle still collecting: its open accumulator. No order
// data for that candle (before the feed was live, a partial first candle, a restart gap) = false: the caller keeps IRT's bid / ask volume.
static const bool BAS_MBO_TOTALS = true;    // (1.24.0) ON after the 2026-10-20 close (Rassul 07:43 "do it"; 08:01: held until after the close)
static bool mboTotOf(const BASState& S, long long t, long long out[2]) {
    if (!BAS_MBO_TOTALS) return false;
    std::map<long long, BasFin>::const_iterator f = S.fin.find(t);
    if (f != S.fin.end()) {
        if (!f->second.dbo || f->second.r.partial) return false;
        const long long b = f->second.r.aggrC[off::BID], a = f->second.r.aggrC[off::ASK];
        if (b < 0 || a < 0 || (b == 0 && a == 0)) return false;
        out[0] = b; out[1] = a; return true;
    }
    if (!(S.firstBarKey > 0 && t > S.firstBarKey)) return false;             // the first candle after the feed opened is partial
    std::map<long long, off::Acc>::const_iterator ac = S.accs.find(t);
    if (ac == S.accs.end()) return false;
    if (ac->second.aggrCap || ac->second.aggrUnknown > 0) return false;      // truncated / unidentified fills are not complete MBO totals
    long long b = 0, a = 0;
    for (std::unordered_map<long long, off::Aggr>::const_iterator g = ac->second.aggr.begin(); g != ac->second.aggr.end(); ++g) {
        if (g->second.size < 0 || (g->second.side != off::BID && g->second.side != off::ASK)) return false;
        long long& total = g->second.side == off::BID ? b : a;
        if (g->second.size > LLONG_MAX - total) return false;
        total += g->second.size;
    }
    if (b == 0 && a == 0) return false;
    out[0] = b; out[1] = a; return true;
}
// a copy of the bar's IRT numbers with the totals replaced by the order-by-order ones when this candle has them; src: true = order feed
static bas::BarMax withMbo(const BASState& S, long long t, const bas::BarMax& m, bool* src = nullptr) {
    bas::BarMax c = m; long long o[2] = {0, 0};
    const bool ok = mboTotOf(S, t, o);
    if (ok) { c.totBid = o[0]; c.totAsk = o[1]; }
    if (src) *src = ok;
    return c;
}
// (1.6.0, approval list s.7 MARK ?) the options volume is unsigned: who bought / sold and opening / closing are unknown
static std::string unsignedHitLine(const ofo::Snap& s, int gate) {
    if (!s.ok || !s.hit.ok || gate == ofo::GATE_OFF) return std::string();
    char b[32]; std::snprintf(b, sizeof(b), "%.1fx", s.hit.ratio);
    return std::string("Unsigned ") + (s.hit.type == 'P' ? "put" : "call") + " volume " + b + " pace ? - buyer / seller unknown";
}

class BidAskStats : public cppExtension {
public:
    SS_SAFE_DIALOG(BidAskStats, "BidAskStats")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    BidAskStats() : cppExtension() {}
    virtual int draw(void);
    virtual int parmsLoadBody(void);                                         // (1.5.2) fills the dialog from the file; loading never saves (lesson 29)
    ss::Snap snap_;                                                      // (1.5.2) the dialog as it opened (1.13.2: SettingsSpec.h)
    virtual int parmsApplyBody(void) { saveSettings(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, ofset::bidAskSpec(), basIds(), (int)i, snap_); saveSettings(); return RTX_OK; }
    virtual int mouse(RTX_EVENT* e);
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax);
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, bas::TITLE, BAS_VERSION) ? RTX_OK : RTX_FAIL; }   // (1.7.0) the pane title, version next to it
    bool mouseLeft(BASState& S);
    bool visRange(unsigned long long& t0, unsigned long long& t1);   // (1.5.1) HoverBoard.h: the chart-wide hover
    bool followBoard(BASState& S);
    void publishBoard(BASState& S, long bar);
    void pump();
    void release() {                                                     // (1.13.0) the Market State entry goes with the indicator
        BASState* S = slot_.get(this, false);
        if (S && !S->chartKey.empty()) { mst::clear("AGG", S->symbol, S->spb, S->chartKey); if (S) S->self.clear(S->symbol, S->spb, S->chartKey); S->aw.clear(S->symbol, S->spb, S->chartKey); }
        slot_.release(this);
    }
    BASState* testState() { return slot_.get(this, false); }
    long long testKey(long long t) { return obBarKey(t); }              // (1.27.0) tests: the CT bar key of a chart stamp
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
    void marketState(BASState& S, long long now);
    void selfAware(BASState& S, long long now);   // (1.35.0) self-awareness SELFAGG
    void obPublish(BASState& S, long long now);                          // (1.25.0)
    long long obBarKey(long long t);                                       // (1.25.0)
    void bigLots(BASState& S);                                            // (1.14.0)
    void attack(BASState& S, long long t, const off::Rec& r, const atk::Pace pace[2], const llp::Tiers& tiers, int ch);   // (1.26.0) K / S / AS at the close
    void sharedAbs(BASState& S, long long now);                           // (1.27.0) the shared A: decide + record + read
    void readOthers(BASState& S, long long now);                          // (1.26.0) lsBookFlow's G + Defense letters, the Delta Profile's TR
    void candleLetters(BASState& S, long long key, bool isHigh, bool isLow, std::vector<lst::Lt>& v);   // (1.28.0) the strip letters of one closed candle
    void attackEvidence(BASState& S, long long key, const lst::Lt& l, long long published, std::vector<rse::Event>& out);
    void exportStrips(BASState& S, long long now);                        // (1.28.0) lsFlexLevels\Strips\ATK-<MKT>-<spb>.csv
    std::string stripTipText(BASState& S, long long key, int strip, const std::vector<lst::Lt>& v, const lst::Pick& pk);
    std::string secClock(long long sec);                                  // (1.26.0) "11:39:18 AM"
    std::string badgeTipText(BASState& S, long long key, const atk::BadgeIn& in, bool isHigh, bool isLow, const std::string& badge, const std::vector<std::string>& dropped);
    template <class XOf> void badgeRow(BASState& S, const off::PaneGeo& g, int v0, int v1, int last, int slot, RTARRAYI& times, XOf xOf, const bp::Box* box);
    struct TipLay { std::vector<std::string> lines; std::vector<int> w; std::vector<off::TipCol> cols; int boxW = 0, boxH = 0, pt = 9; };
    TipLay tipLayout(const std::string& text, int top, int bottom, int pt);
    void drawBadgeTip(BASState& S, const off::PaneGeo& g, const off::IconRect& badge, const std::string& text, const bp::Box* box);
    bool partsOf(long long t, long& days, int& sec);
    const blt::Mark* markAt(BASState& S, long long t) { std::map<long long, blt::Mark>::const_iterator it = S.blMarks.find(t); return it != S.blMarks.end() && it->second.decided ? &it->second : nullptr; }
    std::string blHistPath(BASState& S) { const std::string d = dir(); return d.empty() || S.market.empty() ? std::string() : d + "\\BigLot-" + S.market + "-" + std::to_string(S.spb) + ".txt"; }
    bas::PaceIn paceAt(BASState& S, long long t, const bas::Stat& st);
    bas::PaceIn livePace(BASState& S, const bas::Stat& st);              // (1.14.1) the forming candle's pace so far
public:
    bool paneLeft(BASState& S);                                           // (1.14.1) the cursor is not inside this pane: Live (true = changed)
    bool liveChanged(BASState& S) { return S.hover.bar < 0 && S.live.ok && (S.liveTime != S.liveDrawnT || S.live.bid != S.liveDrawnBid || S.live.ask != S.liveDrawnAsk); }
private:
    int minuteOf(long long t);
    long long clockSec(long long t);
    long long unixOf(long long t);
    void addEvent(BASState& S, const sr::Event& e, bool fromFile);
    const sr::Event* eventAt(BASState& S, long long t);
    void status(BASState& S, long long now);
    void render(BASState& S);
    std::array<er::Side, 2> erSides(BASState& S, int ci, RTARRAYI& times, int last);   // (1.16.0)
    bool newExtreme(int ci, bool& newLow, bool& newHigh);                                 // (1.16.0) a new 10-bar low / high
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
    // (1.13.0) version 5: "Spike vs prior biggest", "Spike lookback" (no longer part of the spike rule) and "Box shows" (did nothing)
    // removed - every row left is read by the chart: Ratio bars (the average), Position + Offset X / Y (the box), Spike vs average (0 = off)
    // (1.13.1) Spike vs average on its own line (the first line was ~458 px, cut off at ~400); (1.13.2) the same rows from
    // ofset::bidAskSpec() through SettingsHost.h, parameter version 5 kept (same rows, same order: saved values keep their rows)
    ss::registerAll(this, ofset::bidAskSpec(), basIds());
    return RTX_OK;
}
int cppExtension::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc");
    BidAskStats* x = static_cast<BidAskStats*>(this);
    x->pump();
    BASState* S = x->testState();
    if (S) {
        bool ch = x->paneLeft(*S);                                         // (1.14.1) the cursor is not inside this pane: Live
        if (x->liveChanged(*S)) ch = true;                                 // (1.14.1) Live: the forming candle's numbers changed
        if (ch) { ++S->redraws; x->invalidateChart(false); }               // only on a change
    }
    return RTX_OK;
}
int BidAskStats::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    {   BASState* S0 = slot_.get(this, false);                            // (1.13.0) the chart label (Market State key), read in the draw as BigBidAsk
        if (S0) { const char* cl = getChartLabel(); const std::string c = cl ? cl : ""; if (!c.empty() && c != S0->chartKey) { if (!S0->chartKey.empty() && !S0->symbol.empty()) mst::clear("AGG", S0->symbol, S0->spb, S0->chartKey); S0->self.clear(S0->symbol, S0->spb, S0->chartKey); S0->chartKey = c; S0->mstMs = -1; } } }
    pump();
    BASState* S = slot_.get(this, false);
    if (S) paneLeft(*S);                                                  // (1.14.1) the candle under the mouse only while the cursor is inside this pane
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
    if (!S.chartKey.empty() && !S.symbol.empty()) { mst::clear("AGG", S.symbol, S.spb, S.chartKey); S.self.clear(S.symbol, S.spb, S.chartKey); S.aw.clear(S.symbol, S.spb, S.chartKey); }   // (1.13.0) (1.34.0 + AWAGG) the old chart identity's facts
    S.mstBar = 0; S.mstCt = 0; S.mstMs = -1; S.mstKv.clear();
    S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
    S.ser.bars.clear(); S.naTries.clear(); S.count = 0; S.lastStamp = 0; S.backlog = true; S.lastClosedTime = 0;
    S.liveTime = 0; S.live = bas::BarMax(); S.liveReadMs = -1; S.faulted = false; S.settingsStamp = -2; S.settingsText = "\x01"; S.settingsCheckMs = -1;
    S.hover.set(-1); S.hoverWnds.clear();
    S.det = sr::Detector(); S.srFrom = -1; S.srCursor = 0; S.srEv.clear(); S.srReclaim.clear(); S.srp = sr::ParamCache();
    S.feed.reset(); S.e = off::Engine(); S.scratch = off::Acc(); S.zone = nl::zoneFor(S.market); S.atr = 0; S.atrCount = -1;
    S.accs.clear(); S.closedAt.clear(); S.barEnds.clear(); S.formingKey = 0; S.firstBarKey = -1; S.finFrom = 0; S.fin.clear();
    S.file = off::SessionFile(); S.curSession.clear(); S.loaded = false; S.params = off::ParamCache(); S.lastStatusText.clear();
    S.opt = off::OptCache();
    S.zr = zread::Host(); S.zrCount = -1; S.zrSession = -1;               // (1.6.0)
    S.blBook = blt::Book(); S.blLoaded = false; S.blWritten.clear(); S.blBarsN = (size_t)-1; S.blLast = -1; S.blMarks.clear();   // (1.14.0)
    S.blSession.clear(); S.blRows.clear(); S.blFile.clear(); S.blRowsWritten.clear(); S.blRejected = 0;
    S.atkK.clear(); S.atkS.clear(); S.atkAS.clear(); S.fillVw.clear(); S.atkD.clear(); S.atkO = atk::Others(); S.atkOff.clear(); S.atkReadMs = -1; S.atkTurnsPath.clear(); S.atkTrPath.clear();   // (1.26.0)
    S.sab = sab::Host(); S.sabDone.clear(); S.sabSeen = (size_t)-1; S.sabDir = false;   // (1.27.0)
    S.stripIds.clear(); S.stripPath.clear(); S.stripReadMs = -1; S.stripWritten = S.stripFail = S.stripsDrawn = S.stripSkipped = 0;   // (1.28.0)
    S.atkKn = S.atkSn = S.atkUndecided = S.atkLoaded = S.atkRejected = 0; S.badgeHit.clear(); S.badgeTip = -1; S.lastTip = dt::TipRect(); S.lastTipLevel = -1;
    { int o = 0, c = 0; if (svl::rthOf(S.market.c_str(), o, c)) { S.openMin = o; S.closeMin = c; } else { S.openMin = 510; S.closeMin = 900; } }
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
    {   int cx = 0, cy = 0; const bool ok = pc::cursorPos(cx, cy); S->cur.onEvent(w, ok, cx, cy, e->v.mouse.h, e->v.mouse.v); }   // (1.14.1) where the pane is
    if (off::infoTip(S->infoIcon, S->infoTipOn, e->v.mouse.h, e->v.mouse.v, bas::INFO_TIP)) invalidateChart(false);   // (1.22.0) the info icon's description; (1.23.0) drawn as a box
    {   int hit = -1;                                                       // (1.26.0) the attacker badge under the mouse (its hover drawn as a tip box)
        if (S->cur.eventInside(e->v.mouse.h, e->v.mouse.v)) for (size_t k = 0; k < S->badgeHit.size() && hit < 0; ++k) if (S->badgeHit[k].first.has(e->v.mouse.h, e->v.mouse.v)) hit = (int)k;
        if (hit != S->badgeTip) { S->badgeTip = hit; invalidateChart(false); } }
    publishBoard(*S, e->v.mouse.bar);                                    // (1.5.1) other panes may follow this candle (this box no longer follows theirs)
    const int hi = S->cur.eventInside(e->v.mouse.h, e->v.mouse.v) ? bas::hoverIndex(e->v.mouse.bar, getBarCount()) : -1;
    if (S->hover.set(hi)) { ++S->redraws; invalidateChart(false); }      // only on a change
    return RTX_FAIL;
}
// this pane's own value axis in contracts (not the instrument's prices): the tallest bid / ask in the bars IRT scales
int BidAskStats::scale(int iStartBar, int iEndBar, double* dMin, double* dMax) {
    double mn = 0, mx = 10;
    BASState* S = slot_.get(this, false);
    if (S) {
        const long count = getBarCount();
        RTARRAYI times(barDateTime);
        long long top = 0, up = 0, down = 0; std::vector<long long> mags;
        if (count > 0 && times.count >= count) {
            const int last = (int)count - 1;
            int a = std::max(0, iStartBar), b = std::min(last, iEndBar);
            if (b < a) { a = std::max(0, last - 200); b = last; }
            if (b - a > 20000) a = b - 20000;
            for (int i = a; i <= b; ++i) {
                const long long bt = (long long)times[i];
                const bas::BarMax* bm = nullptr;
                if (i == last) { if (S->liveTime == bt && S->live.ok) bm = &S->live; }
                else { std::map<long long, bas::Stat>::const_iterator it = S->ser.bars.find(bt); if (it != S->ser.bars.end() && it->second.m.ok) bm = &it->second.m; }
                if (!bm) continue;
                const bas::BarMax bmo = withMbo(*S, bt, *bm); bm = &bmo;     // (1.23.0) the axis on the same totals as the bars
                top = std::max(top, std::max(bm->totBid, bm->totAsk)); up = std::max(up, bm->totAsk); down = std::max(down, bm->totBid);   // (1.19.0) total lots
                mags.push_back(bm->totBid); mags.push_back(bm->totAsk);
            }
        }
        { const long long cp = off::capTop(mags); up = off::capped(up, cp); down = off::capped(down, cp); top = off::capped(top, cp); }   // (1.17.0) the scale cap
        S->axisTop = top; S->axisUp = up; S->axisDown = down;
        off::axisFor(top, S->paneH, off::pairPx(S->paneH, off::textReserve(S->paneH, off::MODE_NONE) + off::QUESTION_H + BAS_BADGE_H), mn, mx);   // (1.18.0) pair bars above zero; the question line on top; (1.26.0) + the badge row
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
    // (1.14.0, lesson 84) compared by CONTENT, not the whole-second stamp: two Applies within one second that keep the size
    // ("...,0,0,0,2" -> "...,0,0,0,0", BL10 -> Off) were missed; the file is one short line, read at most once a second
    const long long st = p.empty() ? -1 : fileStamp(p);
    std::string ln;
    if (st >= 0) { std::ifstream f(p.c_str(), std::ios::binary); if (!(f && std::getline(f, ln))) ln.clear(); if (!ln.empty() && ln.back() == '\r') ln.pop_back(); }
    if (st == S.settingsStamp && ln == S.settingsText) return;
    S.settingsStamp = st; S.settingsText = ln;
    int rb = bas::RATIO_DEFAULT, pos = bas::POS_DEFAULT; bas::SpikeCfg sc; BasPanel pn;
    if (st >= 0 && !ln.empty()) { int r = 0, q = 0; bas::SpikeCfg c; BasPanel b2; if (parseSettings(ln, r, q, &c, &b2)) { rb = r; pos = q; sc = c; pn = b2; } }
    if (pos != S.pos) { S.lastBox = bp::Box(); S.inBox = false; }   // (1.5.2) moved: no stale rect
    S.pos = pos; S.dlgSpike = sc; S.dx = 0; S.dy = 0; S.bigLot = pn.bigLot; S.showS = pn.showS;   // (1.14.0) Big Lot mark: frozen numbers, a change only changes what is shown   // (1.13.3) a saved Offset X / Y is ignored (the 9-spot Position only)           // (1.13.0) the file's 8th field (old Box shows) is read and ignored
    const bas::SpikeCfg eff = effSpike(sc, S.params.load.p);
    const bool reratioed = rb != S.ratioBars;
    if (reratioed) { S.ratioBars = rb; S.ser.reratio(rb); }
    if (reratioed || eff != S.ser.spike) { S.ser.respike(eff); reMessage(S); }   // only a settings change re-decides closed bars
}
// (1.5.2) the chart's market and its saved settings (defaults when there is no file / it is malformed)
static std::string basMarket(cppExtension* x) { char b[32]; std::memset(b, 0, sizeof(b)); const char* rs = x->getRootSymbol(b); const char* sy = x->getSymbol(); const std::string root = rs ? rs : ""; return marketName(root.empty() ? (sy ? sy : "") : root); }
//   // the settings window's copy has no bar size
struct BasFile { int rb = bas::RATIO_DEFAULT, pos = bas::POS_DEFAULT; bas::SpikeCfg sc; BasPanel pn; };
static BasFile basFile(const std::string& p) {
    BasFile F; if (p.empty()) return F;
    std::ifstream f(p.c_str(), std::ios::binary); std::string ln;
    if (f && std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); int r = 0, q = 0; bas::SpikeCfg c; BasPanel b2; if (parseSettings(ln, r, q, &c, &b2)) { F.rb = r; F.pos = q; F.sc = c; F.pn = b2; } }
    return F;
}
// (1.13.2, settings audit 2026-10-18) the window from ofset::bidAskSpec() through SettingsHost.h (rows, order and file unchanged)
static ss::Values basValues(const BasFile& F) {
    ss::Values V = ss::defaults(ofset::bidAskSpec(), -1);
    V.v[ofset::bask::K_RATIO_BARS] = F.rb; V.v[ofset::bask::K_POSITION] = F.pos; V.v[ofset::bask::K_SPIKE_AVG] = F.sc.avgT / 10.0;
    V.v[ofset::bask::K_BIG_LOT] = F.pn.bigLot;                         // (1.14.0)
    V.v[ofset::bask::K_SHOW_S] = F.pn.showS;                           // (1.33.0)
    return V;
}
int BidAskStats::parmsLoadBody(void) {
    if (!ss::inDialog(this)) return RTX_OK;
    ss::fillDialog(this, ofset::bidAskSpec(), basIds(), basValues(basFile(placePath(basMarket(this)))), snap_);
    return RTX_OK;
}
void BidAskStats::saveSettings() {
    if (!ss::inDialog(this)) return;                                      // (1.5.2) only the settings window's values are real
    const std::string mkt = basMarket(this);
    const std::string p = placePath(mkt); if (p.empty()) return;
    const BasFile F = basFile(p);
    const ss::Values V = ss::readDialog(this, ofset::bidAskSpec(), basIds(), basValues(F), snap_);   // untouched -> the file's value; invalid never saved
    // (1.13.0) the file keeps its 8-field layout (older builds read it): the jump / lookback / Box shows fields carry the file's values unchanged
    const int rb = V.i(ofset::bask::K_RATIO_BARS), pos = V.i(ofset::bask::K_POSITION), sa = ofset::tenths(V.f(ofset::bask::K_SPIKE_AVG)), sj = F.sc.jumpT, sl = F.sc.lookback;
    const int dx = 0, dy = 0, bl = V.i(ofset::bask::K_BIG_LOT), sS = V.b(ofset::bask::K_SHOW_S) ? 1 : 0;   // (1.13.3) no offsets: the 9-spot Position only; (1.14.0) Big Lot mark
    const int mode = F.pn.mode == off::MODE_NUMBERS ? off::MODE_NUMBERS : off::MODE_READ;
    if (bas::clampRatioBars(rb) < 0 || bl < 0 || bl > 3) return;
    if (bas::clampLineTenths(sa) < 0 || bas::clampLineTenths(sj) < 0 || bas::clampLookback(sl) < 0) return;   // never overwrite a saved choice
    makeDir(flexDir());
    publishFile(p, std::to_string(rb) + "," + std::to_string(pos) + "," + std::to_string(sa) + "," + std::to_string(sj) + "," + std::to_string(sl)
                   + "," + std::to_string(dx) + "," + std::to_string(dy) + "," + std::to_string(mode) + "," + std::to_string(bl) + "," + std::to_string(sS) + "\n");
    snap_.take(ss::dialogNow(this, ofset::bidAskSpec(), basIds(), V));
    BASState* S = slot_.get(this, false);
    if (S) { S->settingsCheckMs = -1; S->settingsStamp = -2; S->settingsText = "\x01"; }
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
    S.e.useLP = true;                                   // (2026-10-17) Large Lot Methodology for the aggressor orders (L)
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
        if (!S.curSession.empty()) { S.file.save(); S.e.newSession(); S.det = sr::Detector(); S.srFrom = -1; S.llpSaveMs = -1000000; }
        S.curSession = ss; S.loaded = false; S.e.lp.setSession(sessionKey(S.formingKey));
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
// (2026-10-17) the aggressor-order large-lot pool files: lsFlexLevels\BidAskStats.aggrlots-<MKT>[-RTH|-ON].txt (+ .seed.txt = history, read only)
static std::string llpPath(const BASState& S, int k, bool seed) {
    static const char* const suf[llp::NCH] = { "", "-RTH", "-ON" };
    // (1.12.1) the nightly names the 6E seeds "6E" while the chart's market code is "EU": the seeds were never read on 6E
    const std::string mk = (seed && S.market == "EU") ? std::string("6E") : S.market;
    return flexDir() + "\\BidAskStats.aggrlots-" + mk + suf[k] + (seed ? ".seed.txt" : ".txt");
}
static void llpLoad(BASState& S) {
    if (S.market.empty()) return;
    for (int k = 0; k < llp::NCH; ++k) {
        for (int seed = 0; seed < 2; ++seed) {
            std::vector<std::string> lines;
            if (!nl::readBoundedLines(llpPath(S, k, seed != 0), lines, 256 * 1024, 64, 2048)) continue;
            S.e.lp.feed(k, lines, seed != 0);
        }
    }
}
static void llpSave(BASState& S, long long now, bool force) {
    llp::Pool& lp = S.e.lp;
    if (S.market.empty() || lp.session <= 0 || (!lp.dirty && !force)) return;
    if (!force && now - S.llpSaveMs < 60000) return;
    S.llpSaveMs = now; bool ok = true;
    for (int k = 0; k < llp::NCH; ++k) {
        const std::string t = lp.text(k); if (t.empty() || t == S.llpText[k]) continue;
        if (publishFile(llpPath(S, k, false), t)) S.llpText[k] = t; else ok = false;
    }
    if (ok) lp.dirty = false;
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
    S.file.moreTags.clear(); S.file.moreTags.push_back("K|"); S.file.moreTags.push_back("S|"); S.file.moreTags.push_back("AS|"); S.file.extraCap = 5000;   // (1.26.0) the attacker letters
    S.file.load();
    S.file.loadSizes(S.e.aggrDist, S.e.restDist);
    S.e.lp.setSession(sessionKey(S.formingKey)); llpLoad(S);
    for (size_t i = 0; i < S.file.extra.size(); ++i) { sr::Event e; if (sr::parseEvent(S.file.extra[i], e)) { addEvent(S, e, true); S.srLoaded++; continue; }
        const std::string& ln = S.file.extra[i];                              // (1.26.0) the attacker letters exactly as decided then
        atk::KRead k; atk::SRead w; atk::AS a;
        if (atk::parseK(ln, k)) { if (!S.atkK.count(std::make_pair(k.side, k.t))) { S.atkK[std::make_pair(k.side, k.t)] = k; if (k.K) ++S.atkKn; } ++S.atkLoaded; }
        else if (atk::parseS(ln, w)) { if (!S.atkS.count(std::make_pair(w.side, w.t))) { S.atkS[std::make_pair(w.side, w.t)] = w; if (w.S) ++S.atkSn; } ++S.atkLoaded; }
        else if (atk::parseAS(ln, a)) { if (!S.atkAS.count(a.t)) S.atkAS[a.t] = a; ++S.atkLoaded; }
        else if (ln.compare(0, 2, "K|") == 0 || ln.compare(0, 2, "S|") == 0 || ln.compare(0, 3, "AS|") == 0) ++S.atkRejected; }
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
            if (it->second.m.ok) {                                         // the colour as drawn then (1.13.0: a 1.12 row = its spike OR the one rule)
                it->second.bidSpike = bas::fileSpike(fr->second.spike[0] != 0, it->second.m.bid, it->second.bidRatioOk, it->second.bidMean, S.ser.spike);
                it->second.askSpike = bas::fileSpike(fr->second.spike[1] != 0, it->second.m.ask, it->second.askRatioOk, it->second.askMean, S.ser.spike);
            }
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
                { struct tm cx; std::memset(&cx, 0, sizeof(cx)); S.e.chan = (t > 0 && getLocaltime((RTDATE)t, &cx)) ? llp::channelOf(cx.tm_hour, cx.tm_min) : llp::CH_ALL; }   // the bar's own hours
                const bool incomplete = t == S.firstBarKey || a == S.accs.end()
                    || a->second.aggrCap || a->second.aggrUnknown > 0;
                const llp::Tiers tiers = S.e.lp.tiersFor(S.e.chan);            // (1.26.0) the Big Lot tiers in force BEFORE this candle joins the pool (frozen)
                F.r = S.e.close(a != S.accs.end() ? a->second : off::Acc(), incomplete, P); F.dbo = true;
                if (cur && !incomplete && a != S.accs.end()) {                  // (1.26.0) the attacker letters of this candle, decided once
                    const atk::Pace pace[2] = { atk::paceOf(a->second, atk::LOW), atk::paceOf(a->second, atk::HIGH) };
                    attack(S, t, F.r, pace, tiers, S.e.chan);
                }
                {   // (1.25.0, MBO Roadmap Top Defender #1) the order-book truth of absorption for this candle: every resting price where the
                    // defenders traded >= 2x the most ever shown there AND reloaded >= 2 times; plus one marker (px 0) = "decided, order feed
                    // present" so lsDeltaProfile / lsBigBidAsk can tell "no absorption" from "no order feed". Incomplete candles post nothing.
                    const long long bk = obBarKey(t);
                    if (bk > 0 && !incomplete && a != S.accs.end()) {
                        sb::Ev mk; mk.bar = bk; S.obEv.push_back(mk);
                        for (int rs = 0; rs < 2; ++rs) {
                            std::vector<off::ObPx> ob; off::absorbedPx(a->second, rs, ob);
                            for (size_t q = 0; q < ob.size(); ++q) {
                                sb::Ev e; e.bar = bk; e.px = (double)ob[q].px * S.tick; e.side = rs == off::ASK ? 1 : -1; e.val = (double)ob[q].traded;
                                e.flag = sb::obFlag(ob[q].shown, ob[q].refills); S.obEv.push_back(e); S.obMarks++;
                            }
                        }
                        if (S.obEv.size() > sb::CAP_O) S.obEv.erase(S.obEv.begin(), S.obEv.begin() + (long)(S.obEv.size() - sb::CAP_O));
                        S.obMs = -1;
                    }
                }
                if (!incomplete && a != S.accs.end()) {   // (1.32.0) the aggressive fills' average price per resting side (x100 ticks)
                    long double num[2] = {0, 0}; long long den[2] = {0, 0};
                    for (std::unordered_map<long long, off::Aggr>::const_iterator g = a->second.aggr.begin(); g != a->second.aggr.end(); ++g) {
                        const int rs = g->second.side; if ((rs != off::BID && rs != off::ASK) || g->second.size <= 0 || g->second.lo <= 0 || g->second.hi < g->second.lo) continue;
                        num[rs] += (long double)g->second.size * (long double)(g->second.lo + g->second.hi) * 50.0L; den[rs] += g->second.size;
                    }
                    S.fillVw[t] = std::make_pair(den[0] > 0 ? (long long)std::llround((double)(num[0] / den[0])) : 0LL, den[1] > 0 ? (long long)std::llround((double)(num[1] / den[1])) : 0LL);
                    while (S.fillVw.size() > 800) S.fillVw.erase(S.fillVw.begin());
                }
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
    if (any) { S.file.sizes(S.e.aggrDist, S.e.restDist); S.file.save(); llpSave(S, now, false); invalidateChart(false); }
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
// (1.13.0) MARKET STATE, SRC "AGG" (MARKET-STATE-SPEC.md 1.0): the facts of the newest DECIDED closed candle, built once for it (never
// changed - no repaint), posted when it is new and re-posted unchanged every 15 s (an entry older than mst::FRESH_MS = 60 s is ignored by
// the readers; a 3-min candle would otherwise go stale between closes). Keyed like BannerBoard / SignalBoard: symbol, bar size, chart label.
// (1.35.0) SELF-AWARENESS (SelfAware.h, SELF-AWARE.md): each decided candle with an attack spike (buyers / sellers) is a signal in the
// attackers' direction; HIT = price went 1 ATR that way before 1 ATR against (later bars only). A spike that keeps missing in a context
// is the exhaustion / stop-run case - the nightly reverse engineering finds it. Value = the pace x normal. Display unchanged (OFF).
void BidAskStats::selfAware(BASState& S, long long now) {
    if (S.symbol.empty() || S.market.empty() || S.chartKey.empty() || S.mstBar == 0 || S.mstCt <= 0) return;
    if (!lsperf::smartAllowed(LSPERF_PLUGIN, S.symbol)) return;                    // PerfGuard: smart features OFF while slow
    std::map<long long, int>::const_iterator bi = S.barEnds.find(S.mstBar); if (bi == S.barEnds.end()) return;
    const int k = bi->second; RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if (k < 0 || op.count <= k || hi.count <= k || lo.count <= k || cl.count <= k) return;
    const sa::Bar b{ S.mstCt, op[k], hi[k], lo[k], cl[k] };
    if (!std::isfinite(b.o) || !std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(b.c)) return;
    static const sa::Spec SP = [] { sa::Spec s; s.valueKey = "pace_x"; s.sideKey = "spike";
        s.why = { "spike", "sweep", "buy_x", "sell_x", "big_aggr", "big_tier" }; return s; }();
    S.self.ind = "AGG";
    sa::fromFacts(S.self, SP, S.symbol, S.spb, S.chartKey, S.market, now, b, S.mstKv);
}
void BidAskStats::marketState(BASState& S, long long now) {
    if (S.chartKey.empty() || S.symbol.empty() || S.fin.empty()) return;
    const long long t = S.fin.rbegin()->first;
    if (t != S.mstBar) {
        const long long ct = clockSec(t); if (ct == LLONG_MIN) return;
        const BasFin& F = S.fin.rbegin()->second;
        std::map<long long, bas::Stat>::const_iterator st = S.ser.bars.find(t);
        bas::AggIn in; in.s = st != S.ser.bars.end() ? &st->second : nullptr; in.r = F.dbo ? &F.r : nullptr;
        for (int k = 0; k < 2; ++k) { in.let[k] = F.let[k]; in.p100[k] = F.pc.p100[k]; }
        std::map<long long, int>::const_iterator bi = S.barEnds.find(t);
        if (bi != S.barEnds.end()) { RTARRAY op(barOpen), cl(barClose); const int k = bi->second; if (k >= 0 && op.count > k && cl.count > k) { const float o = op[k], c = cl[k]; if (std::isfinite(o) && std::isfinite(c)) in.dir = c > o ? 1 : c < o ? -1 : 0; } }
        {   struct tm cx; std::memset(&cx, 0, sizeof(cx));
            const int ch = getLocaltime((RTDATE)t, &cx) ? llp::channelOf(cx.tm_hour, cx.tm_min) : llp::CH_ALL;   // the candle's own hours
            const llp::Tiers T = S.e.lp.tiersFor(ch); in.tiersOk = T.ok; for (int k = 0; k < 3; ++k) in.tiers[k] = T.t[k]; }
        S.mstKv = bas::aggFacts(in); S.mstBar = t; S.mstCt = ct; S.mstMs = -1;
    }
    if (S.mstMs >= 0 && now >= S.mstMs && now - S.mstMs < 15000) return;
    S.mstMs = now; S.mstPosts++;
    mst::publish("AGG", S.symbol, S.spb, S.chartKey, now, S.mstCt, S.mstKv);
    selfAware(S, now);   // (1.35.0) self-awareness
}
// (1.25.0) the order-book absorption events on the chart's SignalBoard (kind 'O'), refreshed at least every 15 s while drawing
void BidAskStats::obPublish(BASState& S, long long now) {
    if (S.symbol.empty() || S.obEv.empty()) return;
    if (S.obMs >= 0 && now >= S.obMs && now - S.obMs < 15000) return;
    S.obMs = now; S.obPosts++;
    sb::publish('O', S.symbol, S.spb, S.chartKey, now, S.obEv);
}
// (1.25.0) a bar stamp -> Central wall-clock seconds (the SignalBoard bar key; lsBigBidAsk ctSec / lsDeltaProfile localSec)
long long BidAskStats::obBarKey(long long t) {
    long days = 0; int sec = 0; if (!partsOf(t, days, sec)) return 0;
    return (long long)days * 86400LL + sec;
}
// (1.14.0) a bar stamp -> the civil day and second of its CLOSE on the chart's (Central) clock, as lsBigBidAsk parts()
bool BidAskStats::partsOf(long long t, long& days, int& sec) {
    struct tm x; std::memset(&x, 0, sizeof(x));
    if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return false;
    days = svl::daysFromCivil(x.tm_year + 1900, x.tm_mon + 1, x.tm_mday); sec = x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec; return true;
}
// (1.14.0) THE BIG LOT MARK. Runs only once every closed bar in the window is read (no backlog), so a session's tier is decided from its
// full history. (1) the history file once; (2) each complete earlier session of each type from the chart -> the book (file rewritten
// when it changed); (3) every closed candle not decided yet: its session / type, the drawn bar's lots and the tier of its session
// (Book::tierAt, decided once) frozen in its Mark - this session's candles come back from the record exactly as decided before a
// restart; (4) this session's candles >= BL20 -> the record (the nightly scores BL5 / BL10 / BL20 near the HOD / LOD from it).
void BidAskStats::bigLots(BASState& S) {
    if (S.backlog || S.spb <= 0 || S.market.empty()) return;
    const int per = S.spb;
    if (!S.blLoaded) {
        S.blLoaded = true; const std::string p = blHistPath(S); std::string t;
        if (!p.empty()) { std::ifstream f(p.c_str(), std::ios::binary); if (f) { std::stringstream b; b << f.rdbuf(); t = b.str(); } }
        if (t.size() <= 256 * 1024) { S.blBook.load(t); S.blWritten = t; }
    }
    // this session's record (read once per session): the marks decided before a restart
    if (S.blSession != S.curSession) {
        S.blSession = S.curSession; S.blRows.clear(); S.blFile.clear(); S.blRowsWritten.clear();
        const std::string d = dir();
        if (!d.empty() && !S.curSession.empty()) {
            std::ifstream f((d + "\\" + stem(S) + "-" + S.curSession + "-biglot.txt").c_str(), std::ios::binary); std::string ln; int n = 0;
            while (f && std::getline(f, ln) && ++n < 2000) {
                if (!ln.empty() && ln.back() == '\r') ln.pop_back();
                if (ln.empty() || ln[0] == '#' || ln.compare(0, 9, "bar_time|") == 0) continue;
                const std::vector<std::string> v = blt::fields(ln); long long bt = 0, ty = 0, sd = 0, lots = 0, b[3] = {0, 0, 0}, ns = 0, ss = 0; bool ok = v.size() == 13;
                ok = ok && blt::num(v[0], 1, LLONG_MAX / 2, bt) && blt::num(v[2], 1, 10000000, ss) && blt::num(v[3], 0, 1, ty) && blt::num(v[4], 0, 1, sd) && blt::num(v[5], 1, 100000000, lots) && blt::num(v[12], blt::MIN_SESS, 1000, ns);
                for (int q = 0; ok && q < 3; ++q) ok = blt::num(v[(size_t)9 + q], 1, 100000000, b[q]);
                if (!ok) { ++S.blRejected; continue; }
                blt::Mark m; m.decided = true; m.sess = (long)ss; m.type = (int)ty; m.side = (int)sd; m.lots = lots; m.tier.ok = true; m.tier.nSess = (int)ns;
                for (int q = 0; q < 3; ++q) { m.tier.bl[q] = b[q]; }
                m.tier.why = "recorded";
                S.blFile[bt] = m; S.blRows[bt] = ln;
            }
            S.blRowsWritten.clear();
            for (const auto& kv : S.blRows) S.blRowsWritten += kv.second + "\n";
        }
    }
    if (S.ser.bars.size() == S.blBarsN && S.lastClosedTime == S.blLast) return;   // nothing new
    S.blBarsN = S.ser.bars.size(); S.blLast = S.lastClosedTime;
    long d0 = 0; int e0 = 0;
    const long cur = partsOf(S.formingKey, d0, e0) ? svl::classify(d0, e0, per, S.openMin, S.closeMin).sess : 0;
    // (2) complete earlier sessions from the chart
    std::map<long, std::vector<long long> > pr[2]; std::map<long, int> nc[2];
    for (const auto& kv : S.ser.bars) {
        long d = 0; int e = 0; if (!partsOf(kv.first, d, e)) continue;
        const svl::Cls c = svl::classify(d, e, per, S.openMin, S.closeMin); if (c.kind == svl::NONE || c.sess >= cur) continue;
        const int ty = blt::typeOf(e / 60, S.openMin, S.closeMin);
        ++nc[ty][c.sess];
        if (kv.second.m.ok) { pr[ty][c.sess].push_back(kv.second.m.bid); pr[ty][c.sess].push_back(kv.second.m.ask); }
    }
    for (int ty = 0; ty < 2; ++ty) for (const auto& kv : nc[ty]) S.blBook.put(kv.first, ty, blt::sessOf(pr[ty][kv.first], kv.second));
    // (3) the marks, decided once
    bool rows = false;
    for (const auto& kv : S.ser.bars) {
        const long long t = kv.first; if (S.formingKey > 0 && t >= S.formingKey) break;
        if (S.blMarks.count(t) || !kv.second.m.ok || S.naTries.count(t)) continue;
        blt::Mark m; long d = 0; int e = 0; if (!partsOf(t, d, e)) continue;
        const svl::Cls c = svl::classify(d, e, per, S.openMin, S.closeMin);
        std::map<long long, blt::Mark>::const_iterator fm = S.blFile.find(t);
        if (fm != S.blFile.end()) m = fm->second;                          // decided before a restart: exactly as then
        else {
            m.decided = true; m.sess = c.sess; m.type = blt::typeOf(e / 60, S.openMin, S.closeMin);
            m.side = bas::domSide(kv.second, 0); m.lots = std::max(kv.second.m.bid, kv.second.m.ask);
            if (c.kind != svl::NONE) { const blt::Tier* T = S.blBook.tierAt(c.sess, m.type, blt::typeNeed(m.type, S.openMin, S.closeMin, per), true); if (T) m.tier = *T; }
            // (4) this session's candles >= BL20 -> the record
            if (sessionStr(t) == S.curSession && m.tier.ok && m.tier.bl[2] > 0 && m.lots >= m.tier.bl[2]) {
                const bas::Stat& st = kv.second; const bool b = m.side == 0;
                char line[512]; const std::string rch = blt::reached(m.lots, m.tier);
                std::snprintf(line, sizeof(line), "%lld|%s|%ld|%d|%d|%lld|%s|%d|%s|%lld|%lld|%lld|%d", t, ctOf(t).c_str(), m.sess, m.type, m.side, m.lots,
                              bas::fmtRatio(b ? st.bidRatioOk : st.askRatioOk, b ? st.bidRatio : st.askRatio).c_str(), (b ? st.bidSpike : st.askSpike) ? 1 : 0, rch.c_str(),
                              m.tier.bl[0], m.tier.bl[1], m.tier.bl[2], m.tier.nSess);
                S.blRows[t] = line; rows = true;
            }
        }
        S.blMarks[t] = m;
    }
    if (cur > 0) { const int ty = blt::typeOf(e0 / 60, S.openMin, S.closeMin); S.blBook.tierAt(cur, ty, blt::typeNeed(ty, S.openMin, S.closeMin, per), true); }   // the forming candle's type: the box shows its size from the first candle
    while (S.blMarks.size() > KEEP_BARS) S.blMarks.erase(S.blMarks.begin());
    const std::string d = dir();
    if (!d.empty()) {
        makeDir(flexDir()); makeDir(d);
        const std::string h = S.blBook.text(), p = blHistPath(S);
        if (h != S.blWritten && !p.empty() && publishFile(p, h)) S.blWritten = h;
        if (rows && !S.curSession.empty()) {
            std::string o = std::string("# BidAskStats ") + BAS_VERSION + " Big Lot marks " + S.market + " " + S.symbol + " " + std::to_string(S.spb)
                          + "\nbar_time|ct_time|session|type|side|lots|ratio|spike|reached|bl5|bl10|bl20|n_sess\n";
            std::string body; for (const auto& kv : S.blRows) body += kv.second + "\n";
            if (body != S.blRowsWritten && publishFile(d + "\\" + stem(S) + "-" + S.curSession + "-biglot.txt", o + body)) S.blRowsWritten = body;
        }
    }
    invalidateChart(false);
}
// (1.14.1) the cursor is inside this pane only when it is over the window the last mouse event came from and within the pane rectangle
// located then (PaneCursor.h); anywhere else the box shows Live. IRT sends no event when the mouse leaves a pane - this is checked on
// every draw / calc instead.
bool BidAskStats::paneLeft(BASState& S) {
    int cx = 0, cy = 0; const bool ok = pc::cursorPos(cx, cy);
    if (S.cur.inside(cursorWindow(), ok, cx, cy)) return false;
    bool ch = S.hover.set(-1);
    if (S.inBox) { S.inBox = false; ch = true; }
    if (S.badgeTip >= 0) { S.badgeTip = -1; ch = true; }                 // (1.26.0) the badge hover only while the cursor is inside this pane
    return ch;
}
// (1.14.1) the forming candle's pace so far: its lots per second over the seconds elapsed (the bar stamp is its close when it lies ahead of
// the chart clock, else its start); under 5 s elapsed = n/a
bas::PaceIn BidAskStats::livePace(BASState& S, const bas::Stat& st) {
    const long long now = clockSec((long long)currentDate()), t = clockSec(S.liveTime);
    if (now == LLONG_MIN || t == LLONG_MIN || S.spb <= 0 || !st.m.ok) return bas::PaceIn();
    const long long el = t >= now ? (long long)S.spb - (t - now) : now - t;
    if (el < 5 || el > (long long)S.spb) return bas::PaceIn();
    bas::Stat sc = st; sc.m.totBid = st.m.totBid * S.spb / el; sc.m.totAsk = st.m.totAsk * S.spb / el;
    return paceAt(S, S.liveTime, sc);
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
      << "\nSPIKE_VS_AVERAGE," << bas::fmtTenths(S.ser.spike.avgT) << (S.ser.spike.on() ? "" : " (off)")
      << "\nSPIKE_RULE,>= " << bas::fmtTenths(S.ser.spike.avgT) << " the plain " << S.ratioBars << "-candle average of that side, decided at the close; forming candle dim, no label"
      << "\nJUMP_RECORD_FIELD,vs the biggest of the prior " << S.ser.spike.lookback << " candles (not part of the spike rule)"
      << "\nAWARENESS," << awh::VERSION << " AWAGG decided " << S.aw.writes << " records" << (lsperf::smartAllowed(LSPERF_PLUGIN, S.symbol) ? "" : " (smart off)") << "\nPERF," << lsperf::statusOf(this)   // (1.34.0)
      << "\nMARKET_STATE,AGG " << (S.chartKey.empty() ? std::string("waiting for the chart label") : "bar " + std::to_string(S.mstCt) + ",posts " + std::to_string(S.mstPosts) + ",keys " + std::to_string(S.mstKv.size()));
    o << "\nIMPLIED," << S.e.ip.why << ",fills " << S.e.impliedFills << ",lots " << S.e.impliedLots << " (side '?' FILLs: not hits / aggressors / big lots / letters; kept in volume)";
    o << "\nORDERBOOK_ABS,board O events " << S.obEv.size() << ",absorbed prices posted " << S.obMarks << ",posts " << S.obPosts << " (traded >= 2x shown + >= 2 refills; read by lsDeltaProfile / lsBigBidAsk)";
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
    {   // (1.14.0) the Big Lot mark: the setting, the tiers in force for the forming candle's session (both types), today's marks
        static const char* const ch[4] = { "Off", "BL5", "BL10", "BL20" };
        o << "\nBIG_LOT_MARK," << ch[S.bigLot < 0 || S.bigLot > 3 ? 1 : S.bigLot] << ",cyan cap on a closed bar >= the size of its session type, decided at the close (frozen)";
        long d = 0; int e = 0;
        const long cur = S.spb > 0 && partsOf(S.formingKey, d, e) ? svl::classify(d, e, S.spb, S.openMin, S.closeMin).sess : 0;
        for (int ty = 1; ty >= 0; --ty) {
            std::map<long, blt::Tier>::const_iterator it = S.blBook.tiers[ty].find(cur);
            o << (ty ? "\nBIG_LOT_TIER_RTH," : "\nBIG_LOT_TIER_ON,") << "session " << cur << ",";
            if (it == S.blBook.tiers[ty].end()) o << "not decided yet";
            else o << "BL5 " << it->second.bl[0] << ",BL10 " << it->second.bl[1] << ",BL20 " << it->second.bl[2] << "," << it->second.why;
            int done = 0; const int need = blt::typeNeed(ty, S.openMin, S.closeMin, S.spb);
            for (const auto& kv : S.blBook.hist[ty]) if (kv.first < cur && kv.second.candles >= need) ++done;
            o << ",complete sessions known " << done;
        }
        int h[3] = { 0, 0, 0 };
        for (const auto& kv : S.blMarks) if (sessionStr(kv.first) == S.curSession) for (int q = 0; q < 3; ++q) h[q] += blt::shows(kv.second, q + 1) ? 1 : 0;
        o << "\nBIG_LOT_HITS_TODAY,BL5 " << h[0] << ",BL10 " << h[1] << ",BL20 " << h[2] << ",record rows " << S.blRows.size() << ",rejected " << S.blRejected + S.blBook.rejected;
    }
    {   // (2026-10-17) Large Lot Methodology for aggressor orders: the tiers L is using now (this clock's channel, else all hours)
        struct tm cx; std::memset(&cx, 0, sizeof(cx)); const long long nowT = S.formingKey;
        const int ch = (nowT > 0 && getLocaltime((RTDATE)nowT, &cx)) ? llp::channelOf(cx.tm_hour, cx.tm_min) : llp::CH_ALL;
        const llp::Tiers T = S.e.lp.tiersFor(ch);
        static const char* const nm[llp::NCH] = { "all hours", "RTH", "overnight" };
        if (T.ok) o << "\nAGGR_LARGE_TIERS," << nm[T.channel] << ",orders in pool " << T.n << ",start " << (long long)(T.start + 0.5) << ",Big Lot S20 " << T.t[0] << ",S10 " << T.t[1] << ",S5 " << T.t[2] << ",L = S20";
        else o << "\nAGGR_LARGE_TIERS,not enough orders yet (the old percentile rule is used)";
        o << "\nAGGR_LARGE_POOL,sessions " << (S.e.lp.past[llp::CH_ALL].size() + (S.e.lp.cur[llp::CH_ALL].total() > 0 ? 1 : 0)) << ",orders>=10 lots " << S.e.lp.sum(llp::CH_ALL).total();
    }
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
      << "\nFEED_WATCH,dropouts " << c.dropouts << ",reconnects " << c.reconnects << ",silent now " << (c.silentFromMs >= 0 ? "yes" : "no")
      << "\nMBO,subscribed " << (c.flag < 0 ? "unknown" : c.flag ? "1" : "0") << ",opened_with " << (c.via.empty() ? "not opened" : c.via) << ",state " << c.stateText(now)
      << "\nMBO_EVENTS,per_min " << c.roll.perMin(now) << ",total " << e.events << ",new " << e.perAction[0] << ",mod " << e.perAction[1] << ",del " << e.perAction[2] << ",fill " << e.perAction[3] << ",garbage " << e.garbage
      << "\nMBO_READS,slices " << c.slices << ",cap_hits " << c.capHits << ",longest_backlog " << c.longestBacklog << ",faults " << c.faults
      << "\nMBO_DROPPED,late " << S.lateDropped << ",no_bar " << S.noBar << ",decided_with_backlog " << S.decidedLate << ",book " << e.book.size() << ",book_resets " << e.bookResets
      << "\nNEAR_TICKS," << e.near << "\nLEVELS," << S.levels.size() << "," << S.lv.note
      << "\nSTOPRUN,file " << S.srp.p.source << (S.srp.p.note.empty() ? std::string() : " (" + S.srp.p.note + ")") << ",N " << S.srp.p.n << ",SHOW " << sr::showText(S.srp.p.show)
      << ",key_levels " << sr::keyLevels(S.lv, (long long)std::time(NULL)).size() << ",judged_from " << S.srFrom << ",watching " << S.det.pending.size()
      << ",breaks " << S.det.breaks << ",no_burst " << S.det.noBurst << ",held " << S.det.held << ",events " << S.srEvents << ",reloaded " << S.srLoaded
      << "\nATTACK,K " << S.atkKn << ",S " << S.atkSn << ",K_lines " << S.atkK.size() << ",S_lines " << S.atkS.size() << ",AS " << S.atkAS.size() << ",undecided " << S.atkUndecided
      << ",reloaded " << S.atkLoaded << ",rejected " << S.atkRejected << ",badges " << S.badgesDrawn << ",cut " << S.badgeDropped << ",hover_no_room " << S.badgeTipNoRoom << ",hover_compact " << S.badgeTipCompact
      << "\n" << S.sab.statusLine() << ",record SAB lines (nightly read SHARED_A)"
      << "\nSTRIPS," << lst::VERSION << ",drawn " << S.stripsDrawn << ",skipped " << S.stripSkipped << ",exported " << S.stripWritten << ",export_fail " << S.stripFail << ",file " << S.stripPath
      << "\nATTACK_READS,G " << S.atkO.G.size() << ",TR " << S.atkO.Tr.size() << ",TR_bad " << S.atkO.bad << ",turns " << S.atkTurnsPath << ",tr " << S.atkTrPath      << "\nOPTIONS," << S.opt.why << ",asof " << S.opt.last.asof << ",reads " << S.opt.reads << ",arrived " << S.opt.hist.v.size()
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
        off::refreshImplied(S.impC, flexDir(), S.market, now); S.e.ip = S.impC.p;   // (1.29.0) implied fills: exclude (default) / include
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
                {   RTARRAYI vt(barDateTime), vv(barVolume);                    // (1.17.0) the watchdog: is the chart trading?
                    if (vt.count >= count && vv.count >= count) S.feed.watch(now, (long long)vt[(int)count - 1] * 1000003LL + (long long)vv[(int)count - 1]); }
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
                readOthers(S, now);                                        // (1.26.0) G / Tr / Defense letters from the other panes' records
                finalize(S, now);
                stopRuns(S);
                marketState(S, now); obPublish(S, now);
                if (S.mstCt > 0 && !S.chartKey.empty() && lsperf::smartAllowed(LSPERF_PLUGIN, S.symbol)) { S.aw.src = "AGG"; S.aw.tick(S.symbol, S.spb, S.chartKey, S.market, now, S.mstCt, awh::factsRead("aggression")); }   // (1.34.0) L3, links OFF
                sharedAbs(S, now);                                         // (1.27.0) after the order book of each candle is posted
                exportStrips(S, now);                                      // (1.28.0) the letter record for lsReversalStage                                       // (1.13.0) SRC AGG, behind the scenes
                bigLots(S);                                                // (1.14.0) the Big Lot sizes + each closed candle's mark, once
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
// (1.16.0) Attack Effort (the side's whole aggressive lots / its 20 prior candles' average) and Reward (ticks the attacker's way per 1,000
// lots / the market's normal) for candle ci (EffortReward.h); side 0 = sellers (bids hit), 1 = buyers (offers lifted)
std::array<er::Side, 2> BidAskStats::erSides(BASState& S, int ci, RTARRAYI& times, int last) {
    std::array<er::Side, 2> out;
    if (ci < 1 || ci > last) return out;
    const long long t = (long long)times[ci];
    const bas::BarMax* bm = nullptr;
    if (ci == last) { if (S.liveTime == t && S.live.ok) bm = &S.live; }
    else { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(t); if (it != S.ser.bars.end() && it->second.m.ok) bm = &it->second.m; }
    if (!bm) return out;
    const bas::BarMax bmo = withMbo(S, t, *bm); bm = &bmo;               // (1.23.0) order-by-order totals when the candle has them
    std::vector<long long> pb, pa;
    std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.lower_bound(t);
    while (it != S.ser.bars.begin() && (int)pb.size() < er::EFFORT_BARS) { --it; if (it->second.m.ok) { const bas::BarMax pm = withMbo(S, it->first, it->second.m); pb.push_back(pm.totBid); pa.push_back(pm.totAsk); } }
    RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if (op.count <= ci || hi.count <= ci || lo.count <= ci || cl.count <= ci) return out;
    if (!std::isfinite((double)cl[ci])) return out;                         // no current close: cannot decide who lost the candle
    const long long tot[2] = { bm->totBid, bm->totAsk };
    for (int q = 0; q < 2; ++q) {
        out[q].effOk = er::effortX(tot[q], q == 0 ? pb : pa, out[q].eff);
        out[q].rewOk = er::rewardX(op[ci], hi[ci], lo[ci], cl[ci - 1], S.tick, tot[q], q, S.market, out[q].rew);
        out[q].lost = q == 0 ? cl[ci] > op[ci] : cl[ci] < op[ci];   // (1.21.0) the close went against this side
        er::guard(out[q]);   // (1.17.0) too few lots: "-"
    }
    return out;
}
bool BidAskStats::newExtreme(int ci, bool& newLow, bool& newHigh) {
    newLow = newHigh = false; if (ci < 9) return false;
    RTARRAY hi(barHigh), lo(barLow); if (hi.count <= ci || lo.count <= ci) return false;
    double mn = lo[ci], mx = hi[ci];
    for (int k = ci - 9; k < ci; ++k) { mn = std::min(mn, (double)lo[k]); mx = std::max(mx, (double)hi[k]); }
    newLow = lo[ci] <= mn; newHigh = hi[ci] >= mx; return true;
}
// =====================================================================================================================================
// (1.26.0, 2026-10-21 Rassul 20:11-20:23 CT, attacker-mockup-v2.png) THE ATTACKER LETTERS (AttackLogic.h): S / K decided once at the close of
// a 5-candle high / low (K| / S| lines), every order-feed candle's AS line; G / Tr read from lsBookFlow's turns file / the Delta Profile's TR
// record (their rules and records unchanged); R from the stop run; L = one order >= S20 but < S10. One badge per candle in the badge row,
// at most 4 letters, colour by meaning, the rest in the badge's hover (one block per letter, the combination with a Defense letter).
// =====================================================================================================================================
void BidAskStats::attack(BASState& S, long long t, const off::Rec& r, const atk::Pace pace[2], const llp::Tiers& T, int ch) {
    std::map<long long, int>::const_iterator bi = S.barEnds.find(t); if (bi == S.barEnds.end() || !(S.tick > 0)) return;
    const int i = bi->second;
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if (i < 0 || i >= times.count || i >= hi.count || i >= lo.count || i >= op.count || i >= cl.count || (long long)times[i] != t) return;
    const long long tier[3] = { T.ok ? T.t[0] : 0, T.ok ? T.t[1] : 0, T.ok ? T.t[2] : 0 };
    if (!S.atkAS.count(t)) { const atk::AS as = atk::asOf(r, t, tier); S.atkAS[t] = as; S.file.putExtra(atk::asLine(as)); }
    if (i < atk::EXT_BARS) return;
    std::vector<long long> H, L;
    for (int j = i - atk::EXT_BARS; j <= i; ++j) { off::Tick h = 0, l = 0; if (!off::toTick((double)hi[j], S.tick, h) || !off::toTick((double)lo[j], S.tick, l)) return; H.push_back(h); L.push_back(l); }
    bool isHigh = false, isLow = false; if (!atk::extremeOf(H, L, isHigh, isLow)) return;
    dt::Candle x; off::Tick o = 0, c = 0;
    if (!off::toTick((double)op[i], S.tick, o) || !off::toTick((double)cl[i], S.tick, c)) return;
    x.t = t; x.o = o; x.h = H.back(); x.l = L.back(); x.c = c; x.aggr[0] = r.aggrC[0]; x.aggr[1] = r.aggrC[1]; x.ok = true;
    std::vector<long long> prior;                                           // the initiative veto: the deltas of the 20 candles before
    for (int j = std::max(0, i - atk::INIT_BARS); j < i; ++j) { std::map<long long, BasFin>::const_iterator f = S.fin.find((long long)times[j]); if (f != S.fin.end() && f->second.dbo && !f->second.r.partial) prior.push_back(f->second.r.aggrC[1] - f->second.r.aggrC[0]); }
    std::vector<const atk::AS*> legs;                                       // the 10 candles before (no AS line = unknown: S not decided)
    for (int j = i - atk::LEG_BARS; j < i; ++j) { if (j < 0) { legs.push_back(nullptr); continue; } std::map<long long, atk::AS>::const_iterator f = S.atkAS.find((long long)times[j]); legs.push_back(f == S.atkAS.end() ? nullptr : &f->second); }
    for (int sd = 0; sd < 2; ++sd) {
        if (!(sd == atk::HIGH ? isHigh : isLow)) continue;
        const std::pair<int, long long> key(sd, t);
        const bool init = dt::initiativeOf(x, sd, prior);
        if (!S.atkK.count(key)) { atk::KRead k; if (atk::decideK(r, t, sd, T.ok, tier, ch, init, pace[sd], k)) { S.atkK[key] = k; S.file.putExtra(atk::kLine(k)); if (k.K) ++S.atkKn; } else ++S.atkUndecided; }
        if (!S.atkS.count(key)) { long long mx = 0; const bool known = atk::legMax(legs, sd == atk::HIGH ? off::ASK : off::BID, mx);
            atk::SRead w; if (atk::decideS(r, t, sd, known, mx, S.tick, pace[sd], w)) { S.atkS[key] = w; S.file.putExtra(atk::sLine(w)); if (w.S) ++S.atkSn; } else ++S.atkUndecided; }
    }
    while (S.atkAS.size() > KEEP_BARS) S.atkAS.erase(S.atkAS.begin());
    while (S.atkK.size() > KEEP_BARS) S.atkK.erase(S.atkK.begin());
    while (S.atkS.size() > KEEP_BARS) S.atkS.erase(S.atkS.begin());
}
static std::string basContractTok(const std::string& sym) {                // as lsBookFlow / the Delta Profile name their files
    std::string out;
    for (size_t i = 0; i < sym.size() && out.size() < 64; ++i) { char c = sym[i]; if (c == '|' || c == ',' || c == '\n' || c == '\r') c = ' ';
        const unsigned char u = (unsigned char)c; const bool keep = (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z') || (u >= '0' && u <= '9') || u == '_' || u == '-' || u == '.';
        out += keep ? c : '_'; }
    return out.empty() ? std::string("unknown-contract") : out;
}
// every 3 s: the new lines of lsBookFlow's turns file (G + the Defense letters) and of the Delta Profile's signals file (TR), this session
// and the one before - read only, never re-decided here
// =====================================================================================================================================
// (1.27.0, 2026-10-22 Rassul 06:43 CT "then do it" + 06:49 "then require it") THE SHARED ABSORPTION A (SharedAbsorb.h): every candle this
// pane has finalized (its order book posted) is judged ONCE: at a 5-candle high / low, the attackers' lots (the pane's Attack Effort
// totals) >= E x their 20-candle average AND the close in the defenders' colour. A fired A is appended to the shared record
// lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv with the book (lsBidAskStats' own order-book absorption in the wick), the pace at the
// extreme and the nearest level; the Delta Profile, the Defense pane, Big Bid / Ask and this pane all DRAW from that record. A candle that
// already has a line (another chart / plugin decided it first) is adopted, never re-decided. Never repainted.
// =====================================================================================================================================
void BidAskStats::sharedAbs(BASState& S, long long now) {
    const std::string f = flexDir(); if (f.empty() || S.market.empty() || S.spb <= 0 || !(S.tick > 0)) return;
    const std::string d = f + "\\SharedAbsorb";
    S.sab.bind(S.market, (int)S.spb, d + "\\" + sab::fileName(S.market, (int)S.spb), f + "\\LRA-Badges-" + sab::canon(S.market) + ".csv");
    if (!S.sabDir) { makeDir(d); S.sabDir = true; }
    S.sab.poll(now);
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    const int n = (int)std::min(std::min((long)times.count, (long)op.count), std::min(std::min((long)hi.count, (long)lo.count), (long)cl.count));
    if (n >= 2) {
        int first = -1;                                                       // the oldest closed candle not judged yet (last 2,000)
        for (int i = std::max(0, n - 2001); i <= n - 2; ++i) { const long long t = (long long)times[i]; if (t > 0 && !S.sabDone.count(t) && S.fin.count(t)) { first = i; break; } }
        if (first >= 0) {
            const int from = std::max(0, first - sab::EFFORT_BARS - sab::EXT_BARS - 15);
            std::vector<sab::Bar> B; B.reserve((size_t)(n - from));
            for (int j = from; j <= n - 2; ++j) {
                sab::Bar b; off::Tick o = 0, h = 0, l = 0, c = 0; const long long t = (long long)times[j];
                b.ok = t > 0 && off::toTick((double)op[j], S.tick, o) && off::toTick((double)hi[j], S.tick, h) && off::toTick((double)lo[j], S.tick, l) && off::toTick((double)cl[j], S.tick, c);
                b.o = o; b.h = h; b.l = l; b.c = c;
                std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(t);
                if (b.ok && it != S.ser.bars.end() && it->second.m.ok) { const bas::BarMax m = withMbo(S, t, it->second.m); b.buy = m.totAsk; b.sell = m.totBid; }
                B.push_back(b);
            }
            std::vector<sab::Lv> lv; for (size_t k = 0; k < S.levels.size(); ++k) { sab::Lv x; x.name = S.levels[k].name; x.px = S.levels[k].px; lv.push_back(x); }
            for (int i = first; i <= n - 2; ++i) {
                const long long t = (long long)times[i]; if (t <= 0 || S.sabDone.count(t)) continue;
                if (!S.fin.count(t)) continue;                                // not finalized yet (VAP retry / order feed grace): later
                const size_t bi = (size_t)(i - from);
                const long long key = obBarKey(t); if (key <= 0) { S.sabDone.insert(t); continue; }
                bool present = false; std::vector<sab::ObEv> ev;
                for (const sb::Ev& e : S.obEv) { if (e.bar != key) continue; if (e.side == 0) { present = true; continue; } sab::ObEv o; o.px = e.px; o.side = e.side; o.traded = e.val; o.refills = sb::obRefills(e.flag); ev.push_back(o); }
                for (int sd = 0; sd < 2; ++sd) {
                    if (S.sab.st.has(sd, key)) { ++S.sab.adopted; continue; }
                    const sab::Decision D = sab::decide(B, bi, sd, S.sab.P.E);
                    if (!D.decided) { ++S.sab.learning; continue; }
                    S.sab.count(D);
                    if (!D.fired) continue;
                    sab::Rec r; r.mkt = S.market; r.spb = (int)S.spb; r.key = key; r.raw = t; r.side = sd; r.effort = D.effort; r.E = D.E; r.att = D.att; r.avg = D.avg;
                    r.close = sd == sab::LOW ? 'G' : 'R'; r.extPx = (double)(sd == sab::HIGH ? B[bi].h : B[bi].l) * S.tick; r.closePx = (double)B[bi].c * S.tick;
                    sab::bookOf(B[bi], sd, present, ev, S.tick, r.book, r.bookLots, r.refills);
                    r.decided = obBarKey((long long)currentDate()); if (r.decided <= 0) r.decided = key;
                    r.by = "BAS"; r.loc = sab::locName(sab::LOCATION);
                    const std::pair<int, long long> kk(sd, t);
                    std::map<std::pair<int, long long>, atk::KRead>::const_iterator kr = S.atkK.find(kk); std::map<std::pair<int, long long>, atk::SRead>::const_iterator sr2 = S.atkS.find(kk);
                    const atk::Pace pc = kr != S.atkK.end() ? kr->second.p : sr2 != S.atkS.end() ? sr2->second.p : atk::Pace();
                    r.e100 = pc.e100; r.into100 = pc.into100;
                    const double atr = sab::atrTicks(B, bi) * S.tick; sab::levelOf(lv, r.extPx, r.closePx, atr, r.level, r.lvd, r.gamma);
                    S.sab.put(r);
                    sab::Extra x; x.mkt = S.market; x.spb = (int)S.spb; x.key = key; x.side = sd; x.by = "BAS";   // pace / level ride in an SABX line (the SAB line keeps the decision)
                    if (r.e100 >= 0) x.kv["e100"] = std::to_string(r.e100);
                    if (r.into100 >= 0) x.kv["into100"] = std::to_string(r.into100);
                    if (!r.level.empty()) { x.kv["level"] = r.level; x.kv["lvd"] = sab::fmt(r.lvd, 2); }
                    if (r.gamma) x.kv["gamma"] = std::to_string(r.gamma);
                    S.sab.putX(x);
                }
                S.sabDone.insert(t);
            }
            while (S.sabDone.size() > 6000) S.sabDone.erase(S.sabDone.begin());
        }
    }
    if (S.sab.st.byKey.size() != S.sabSeen) {                                 // the badges read the record by the chart's own bar key
        S.sabSeen = S.sab.st.byKey.size(); S.atkO.SA.clear();
        for (std::map<std::pair<int, long long>, sab::Rec>::const_iterator it = S.sab.st.byKey.begin(); it != S.sab.st.byKey.end(); ++it) if (it->second.raw > 0) S.atkO.SA.insert(std::make_pair(it->first.first, it->second.raw));
        invalidateChart(false);
    }
}
void BidAskStats::readOthers(BASState& S, long long now) {
    if (S.atkReadMs >= 0 && now >= S.atkReadMs && now - S.atkReadMs < 3000) return;
    S.atkReadMs = now;
    const std::string f = flexDir(); if (f.empty() || S.market.empty() || S.spb <= 0) return;
    const std::string tp = dlset::turnsPath(f, S.market, (int)S.spb); S.atkTurnsPath = tp;
    atk::tailRead(tp, S.atkOff[tp], [&](const std::string& ln) { S.atkO.turnsLine(ln); });
    if (!S.symbol.empty() && S.formingKey > 0) {
        struct tm x; std::memset(&x, 0, sizeof(x));
        if (getLocaltime((RTDATE)S.formingKey, &x)) {
            const long long sess = off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) + (x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec >= 17 * 3600 ? 1 : 0);
            for (long long so = 1; so >= 0; --so) {
                const std::string p = f + "\\DeltaProfile\\" + S.market + "-" + basContractTok(S.symbol) + "-" + std::to_string(S.spb) + "-signals-" + std::to_string(sess - so) + ".csv";
                if (so == 0) S.atkTrPath = p;
                atk::tailRead(p, S.atkOff[p], [&](const std::string& ln) { S.atkO.trLine(ln); });
            }
        }
    }
    while (S.atkOff.size() > 16) { std::map<std::string, long long>::iterator it = S.atkOff.begin(); if (it->first == tp) ++it; if (it == S.atkOff.end()) break; S.atkOff.erase(it); }
    S.atkO.trim();
}
std::string BidAskStats::secClock(long long sec) {
    struct tm x; std::memset(&x, 0, sizeof(x)); if (sec <= 0 || !getLocaltime((RTDATE)sec, &x)) return std::string();
    char b[24]; std::snprintf(b, sizeof b, "%d:%02d:%02d %s", x.tm_hour % 12 == 0 ? 12 : x.tm_hour % 12, x.tm_min, x.tm_sec, x.tm_hour < 12 ? "AM" : "PM"); return b;
}
// the hover of one badge: one block per letter (also the ones with no room), the record's numbers, the combination lines, the pace line
std::string BidAskStats::badgeTipText(BASState& S, long long key, const atk::BadgeIn& in, bool isHigh, bool isLow, const std::string& badge, const std::vector<std::string>& dropped) {
    (void)isHigh; (void)isLow;
    const std::vector<atk::Letter> L = atk::letters(in);
    std::vector<std::string> names; for (size_t k = 0; k < L.size(); ++k) names.push_back(L[k].s);
    atk::Pace pace; int paceSide = -1;
    for (int sd = 1; sd >= 0 && paceSide < 0; --sd) {                     // the pace of the attack at this candle's extreme (K / S lines carry it)
        std::map<std::pair<int, long long>, atk::KRead>::const_iterator k = S.atkK.find(std::make_pair(sd, key)); if (k != S.atkK.end()) { pace = k->second.p; paceSide = sd; break; }
        std::map<std::pair<int, long long>, atk::SRead>::const_iterator w = S.atkS.find(std::make_pair(sd, key)); if (w != S.atkS.end()) { pace = w->second.p; paceSide = sd; }
    }
    auto othersOf = [&](const std::string& me, int sd) {
        std::vector<std::string> o; for (size_t k = 0; k < names.size(); ++k) if (names[k] != me && (names[k] == "K" || names[k] == "S" || names[k] == "G" || names[k] == "Tr")) o.push_back(names[k]);
        if (paceSide == sd && pace.e100 >= 0 && pace.e100 <= atk::E_SHOW100) o.push_back("E");
        return o; };
    long long nG = 0, nL = 0, nR = 0;
    for (std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator g = S.atkO.G.begin(); g != S.atkO.G.end(); ++g) nG += g->second.G ? 1 : 0;
    for (std::map<long long, atk::AS>::const_iterator a = S.atkAS.begin(); a != S.atkAS.end(); ++a) nL += (atk::lOf(a->second, off::BID) || atk::lOf(a->second, off::ASK)) ? 1 : 0;
    for (std::map<long long, std::vector<sr::Event> >::const_iterator e = S.srEv.begin(); e != S.srEv.end(); ++e) for (size_t q = 0; q < e->second.size(); ++q) nR += e->second[q].show != sr::SHOW_OFF ? 1 : 0;
    std::vector<dt::HoverBlock> blocks;
    for (size_t k = 0; k < L.size(); ++k) {
        const std::string& n = L[k].s;
        for (int sd = 1; sd >= 0; --sd) {
            const std::pair<int, long long> kk(sd, key);
            if (n == "S") { std::map<std::pair<int, long long>, atk::SRead>::const_iterator w = S.atkS.find(kk); if (w != S.atkS.end() && w->second.S) blocks.push_back(atk::hoverS(w->second, secClock(w->second.sec), S.market, othersOf("S", sd), S.atkO.defenseAt(key, sd), S.atkSn)); }
            else if (n == "K") { std::map<std::pair<int, long long>, atk::KRead>::const_iterator q = S.atkK.find(kk); if (q != S.atkK.end() && q->second.K) blocks.push_back(atk::hoverK(q->second, secClock(q->second.sec), S.market, othersOf("K", sd), S.atkO.defenseAt(key, sd), S.atkKn)); }
            else if (n == "G") { std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator g = S.atkO.G.find(kk); if (g != S.atkO.G.end() && g->second.G) blocks.push_back(atk::hoverG(g->second, othersOf("G", sd), S.atkO.defenseAt(key, sd), nG)); }
            else if (n == "Tr") { std::map<std::pair<int, long long>, trapl::Rec>::const_iterator r = S.atkO.Tr.find(kk); if (r != S.atkO.Tr.end()) blocks.push_back(atk::hoverTr(r->second.top, r->second.lots, r->second.x, othersOf("Tr", sd), S.atkO.defenseAt(key, sd), (long long)S.atkO.Tr.size())); }
        }
        if (n == "A") { for (int sd = 1; sd >= 0; --sd) { const sab::Rec* rr = S.sab.st.atRaw(sd, key); if (rr) { dt::HoverBlock b; b.letter = "A"; b.rows = sab::hover(*rr, S.sab.P); blocks.push_back(b); } } }   // (1.27.0)
        else if (n == "R") { std::map<long long, std::vector<sr::Event> >::const_iterator se = S.srEv.find(key);
            if (se != S.srEv.end()) for (size_t q = 0; q < se->second.size(); ++q) { const sr::Event& e = se->second[q]; if (e.show == sr::SHOW_OFF) continue; blocks.push_back(atk::hoverR(e.dir, e.level, e.ticks, e.bars, sr::burstText(e), nR)); } }
        else if (n == "L") { std::map<long long, atk::AS>::const_iterator a = S.atkAS.find(key);
            if (a != S.atkAS.end()) for (int rs = 1; rs >= 0; --rs) if (atk::lOf(a->second, rs)) blocks.push_back(atk::hoverL(rs, a->second.big[rs], a->second.tier, secClock(a->second.sec[rs]), nL)); }
    }
    struct tm x; std::memset(&x, 0, sizeof(x));
    const std::string when = key > 0 && getLocaltime((RTDATE)key, &x) ? bas::clock12(x.tm_hour, x.tm_min) : std::string();
    return atk::hoverText(badge, when, L.empty() ? -1 : L.front().colourKey, dropped, blocks, paceSide >= 0 ? atk::paceRow(pace, paceSide) : std::string());
}
// (1.28.0) the strip letters of one closed candle: S / K / L (Attack) and R / Tr / G (Running out), each with its pivot side, the book side
// for L and the key numbers for the Reversal Stage hover. The shared A is not an attacker-pane letter any more (Defense pane only).
static int basEvidenceQuality(const BASState& S, long long key);
// (1.32.0) " vw<avg fill x100 ticks>" of one resting side on a finalized candle, or "" (restarted / incomplete = no token)
static std::string basVwTok(const BASState& S, long long key, int restSide) {
    std::map<long long, std::pair<long long, long long> >::const_iterator f = S.fillVw.find(key); if (f == S.fillVw.end()) return std::string();
    const long long v = restSide == off::ASK ? f->second.second : f->second.first; return v > 0 ? " vw" + std::to_string(v) : std::string();
}
void BidAskStats::candleLetters(BASState& S, long long key, bool isHigh, bool isLow, std::vector<lst::Lt>& v) {
    v.clear(); char b[96];
    for (int sd = 1; sd >= 0; --sd) {
        const std::pair<int, long long> kk(sd, key);
        std::map<std::pair<int, long long>, atk::SRead>::const_iterator w = S.atkS.find(kk);
        const std::string vw = basVwTok(S, key, sd == lst::HIGH ? off::ASK : off::BID);   // (1.32.0) the attackers' average fill (hidden token)
        if (w != S.atkS.end() && w->second.S) { std::snprintf(b, sizeof b, "%lld lots %lld prices", w->second.lots, w->second.nPx); v.push_back(lst::make(lst::ATK, "S", sd, -1, b + vw)); }
        std::map<std::pair<int, long long>, atk::KRead>::const_iterator k = S.atkK.find(kk);
        if (k != S.atkK.end() && k->second.K) { std::snprintf(b, sizeof b, "%lld lots", k->second.lots); v.push_back(lst::make(lst::ATK, "K", sd, -1, b + vw)); }
        const long long fade = w != S.atkS.end() && w->second.S ? w->second.p.e100 : k != S.atkK.end() && k->second.K ? k->second.p.e100 : -1;
        if (fade >= 0 && fade <= atk::E_SHOW100 && basEvidenceQuality(S, key) == rse::COMPLETE) {
            std::snprintf(b, sizeof b, "executed pace %.2fx after extreme", (double)fade / 100.0);
            v.push_back(lst::make(lst::ATK, "E", sd, -1, b));
        }
        std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator g = S.atkO.G.find(kk);
        if (g != S.atkO.G.end() && g->second.G) { std::snprintf(b, sizeof b, "pulled %.1fx", g->second.x); v.push_back(lst::make(lst::ATK, "G", sd, -1, b)); }
        std::map<std::pair<int, long long>, trapl::Rec>::const_iterator t = S.atkO.Tr.find(kk);
        if (t != S.atkO.Tr.end()) { std::snprintf(b, sizeof b, "%.0f lots %.1fx", t->second.lots, t->second.x); v.push_back(lst::make(lst::ATK, "Tr", sd, -1, b)); }
        std::map<std::pair<int, long long>, std::pair<long long, long long> >::const_iterator dv = S.atkD.find(kk);   // (1.32.0) D
        if (dv != S.atkD.end()) { std::snprintf(b, sizeof b, "delta %lld new %s, candle %s", dv->second.first, sd == lst::LOW ? "low" : "high", sd == lst::LOW ? "higher low" : "lower high"); v.push_back(lst::make(lst::ATK, "D", sd, -1, b)); }
    }
    { std::map<long long, std::vector<sr::Event> >::const_iterator se = S.srEv.find(key);
      if (se != S.srEv.end()) for (size_t q = 0; q < se->second.size(); ++q) { const sr::Event& e = se->second[q]; if (e.show == sr::SHOW_OFF) continue;
          std::snprintf(b, sizeof b, "%s %dt", e.level.c_str(), e.ticks); v.push_back(lst::make(lst::ATK, "R", e.dir > 0 ? lst::HIGH : lst::LOW, -1, b)); break; } }
    { std::map<long long, atk::AS>::const_iterator a = S.atkAS.find(key);
      if (a != S.atkAS.end()) for (int rs = 1; rs >= 0; --rs) if (atk::lOf(a->second, rs)) {
          // the pivot an L order points at: buyers lifting = a high attempt, sellers hitting = a low attempt (at an extreme: that extreme)
          const int piv = isHigh && !isLow ? lst::HIGH : isLow && !isHigh ? lst::LOW : (rs == off::ASK ? lst::HIGH : lst::LOW);
          std::snprintf(b, sizeof b, "%lld lots", a->second.big[rs]); v.push_back(lst::make(lst::ATK, "L", piv, rs == off::ASK ? 1 : 0, b + basVwTok(S, key, rs))); break; } }
}
// Typed evidence is deliberately built from the same already-decided source records as LT.
// No source calculation, ordering, or legacy line is altered here.  A persisted letter without
// its in-memory finalized record is retained, but honestly marked UNVERIFIED rather than promoted.
static long long basEvidenceLots(long long v) { return v >= 0 && v <= 1000000000000LL ? v : -1; }
static long long basEvidencePace(long long v) { return v >= 0 && v <= 100000000 ? v : -1; }
static double basEvidenceRatio(double v) { return std::isfinite(v) && v >= 0 && v <= 1000000000.0 ? v : -1; }
static int basEvidenceQuality(const BASState& S, long long key) {
    std::map<long long, BasFin>::const_iterator f = S.fin.find(key);
    if (f != S.fin.end()) {
        if (!f->second.dbo) return rse::UNAVAILABLE;
        if (f->second.r.partial) return rse::PARTIAL;
        long long totals[2] = {0, 0};
        // COMPLETE means the same full native aggressor coverage accepted by mboTotOf,
        // not merely a non-partial persisted aggregate record.
        return mboTotOf(S, key, totals) ? rse::COMPLETE : rse::UNVERIFIED;
    }
    return S.accs.count(key) || S.closedAt.count(key) ? rse::PENDING : rse::UNVERIFIED;
}
static long long basEvidenceExtreme(BASState& S, long long key, int pivot) {
    std::map<long long, int>::const_iterator i = S.barEnds.find(key);
    if (i == S.barEnds.end() || !(S.tick > 0)) return LLONG_MIN;
    cppExtension::RTARRAY hi(barHigh), lo(barLow); off::Tick p = 0;
    if (pivot == lst::HIGH && i->second < hi.count && off::toTick((double)hi[i->second], S.tick, p)) return p;
    if (pivot == lst::LOW && i->second < lo.count && off::toTick((double)lo[i->second], S.tick, p)) return p;
    return LLONG_MIN;
}
static long long basEvidenceTick(double px, double tick) {
    off::Tick p = 0; return tick > 0 && std::isfinite(px) && px > 0 && off::toTick(px, tick, p) ? (long long)p : LLONG_MIN;
}
static long long basEvidenceDoubleLots(double v) {
    return std::isfinite(v) && v >= 0 && v <= 1000000000000.0 ? (long long)std::llround(v) : -1;
}
void BidAskStats::attackEvidence(BASState& S, long long key, const lst::Lt& l, long long published, std::vector<rse::Event>& out) {
    if (key <= 0 || published <= 0 || S.symbol.empty()) return;
    auto put = [&](long long decision, long long cause, int basis, int quality, long long price, long long lots, long long aux, long long pace, double ratio, const std::string& suffix) {
        if (decision <= 0) return;
        rse::Event e; e.key = decision; e.pane = lst::ATK; e.l = l; e.m.typed = true; e.m.basis = basis; e.m.quality = quality;
        e.m.cause = cause > 0 ? cause : 0; e.m.published = published; e.m.priceTicks = price; e.m.lots = lots; e.m.auxLots = aux; e.m.pace100 = pace; e.m.ratio = ratio;
        e.m.contract = S.symbol; e.m.id = rse::eventId(e.pane, e.key, e.l, e.m.cause) + suffix; out.push_back(e);
    };
    const std::pair<int, long long> kk(l.pivot, key);
    if (l.code == "S") {
        std::map<std::pair<int, long long>, atk::SRead>::const_iterator w = S.atkS.find(kk);
        if (w != S.atkS.end() && w->second.S) put(key, w->second.t, rse::MBO, basEvidenceQuality(S, w->second.t), basEvidenceExtreme(S, w->second.t, l.pivot), basEvidenceLots(w->second.lots), -1, basEvidencePace(w->second.p.e100), -1, "-S");
    } else if (l.code == "K") {
        std::map<std::pair<int, long long>, atk::KRead>::const_iterator k = S.atkK.find(kk);
        if (k != S.atkK.end() && k->second.K) put(key, k->second.t, rse::MBO, basEvidenceQuality(S, k->second.t), basEvidenceExtreme(S, k->second.t, l.pivot), basEvidenceLots(k->second.lots), -1, basEvidencePace(k->second.p.e100), -1, "-K");
    } else if (l.code == "E") {
        const auto w = S.atkS.find(kk); const auto k = S.atkK.find(kk);
        const bool sweep = w != S.atkS.end() && w->second.S;
        const bool big = k != S.atkK.end() && k->second.K;
        const long long pace = sweep ? w->second.p.e100 : big ? k->second.p.e100 : -1;
        const long long lots = sweep ? w->second.lots : big ? k->second.lots : -1;
        if (pace >= 0 && pace <= atk::E_SHOW100 && basEvidenceQuality(S, key) == rse::COMPLETE)
            put(key, key, rse::MBO, rse::COMPLETE, basEvidenceExtreme(S, key, l.pivot), basEvidenceLots(lots), -1, pace, (double)pace / 100.0, "-E");
    } else if (l.code == "L") {
        std::map<long long, atk::AS>::const_iterator a = S.atkAS.find(key);
        const int side = l.book == 1 ? off::ASK : l.book == 0 ? off::BID : -1;
        if (a != S.atkAS.end() && side >= 0 && atk::lOf(a->second, side)) put(key, a->second.t, rse::MBO, basEvidenceQuality(S, a->second.t), basEvidenceExtreme(S, a->second.t, l.pivot), basEvidenceLots(a->second.big[side]), -1, -1, -1, "-L");
    } else if (l.code == "G") {
        std::map<std::pair<int, long long>, dt::GiveUp>::const_iterator g = S.atkO.G.find(kk);
        if (g != S.atkO.G.end() && g->second.G) put(key, g->second.t, rse::MBO, rse::UNVERIFIED, basEvidenceExtreme(S, g->second.t, l.pivot), basEvidenceLots(g->second.pull), basEvidenceLots(g->second.add), -1, basEvidenceRatio(g->second.pace), "-G");
    } else if (l.code == "Tr") {
        std::map<std::pair<int, long long>, trapl::Rec>::const_iterator t = S.atkO.Tr.find(kk);
        if (t != S.atkO.Tr.end()) put(key, (long long)t->second.barRaw, rse::IMPORTED, rse::UNVERIFIED, t->second.extreme > 0 ? t->second.extreme : LLONG_MIN, basEvidenceDoubleLots(t->second.lots), -1, -1, basEvidenceRatio(t->second.x), "-Tr-" + std::to_string(t->second.id));
    } else if (l.code == "D") {   // (1.32.0) MBO CVD divergence: decided from complete order-feed totals only
        std::map<std::pair<int, long long>, std::pair<long long, long long> >::const_iterator d = S.atkD.find(kk);
        if (d != S.atkD.end() && basEvidenceQuality(S, key) == rse::COMPLETE) put(key, key, rse::MBO, rse::COMPLETE, d->second.second, basEvidenceLots(d->second.first < 0 ? -d->second.first : d->second.first), -1, -1, -1, "-D");
    } else if (l.code == "R") {
        std::map<long long, std::vector<sr::Event> >::const_iterator s = S.srEv.find(key);
        if (s != S.srEv.end()) for (size_t n = 0; n < s->second.size(); ++n) { const sr::Event& r = s->second[n];
            if (r.show == sr::SHOW_OFF || r.breakT != key || r.reclaimT <= 0) continue;
            put(r.reclaimT, r.breakT, rse::HYBRID, basEvidenceQuality(S, r.breakT), basEvidenceTick(r.px, S.tick), -1, -1, -1, r.ratioOk ? basEvidenceRatio(r.ratio) : -1, "-R-" + std::to_string(n)); break;
        }
    }
}
// (1.28.0) every 3 s: append the strip letters of the closed candles (last 600) not yet in lsFlexLevels\Strips\ATK-<MKT>-<spb>.csv. First
// line per candle + letter + pivot wins (another chart on the same market may have written it); an id counts as written only after a good
// flushed append. The file is read once per binding (bounded 4 MB) to know what is already there.
void BidAskStats::exportStrips(BASState& S, long long now) {
    if (S.stripReadMs >= 0 && now >= S.stripReadMs && now - S.stripReadMs < 3000) return;
    S.stripReadMs = now;
    const std::string f = flexDir(); if (f.empty() || S.market.empty() || S.spb <= 0 || !(S.tick > 0)) return;
    const std::string d = f + "\\Strips", p = d + "\\" + lst::fileName(lst::ATK, S.market, (int)S.spb);
    if (p != S.stripPath) {
        S.stripPath = p; S.stripIds.clear(); makeDir(d);
        long long off0 = 0; atk::tailRead(p, off0, [&](const std::string& ln) { lst::Rec r; if (lst::parse(ln, r) && r.pane == lst::ATK) S.stripIds.insert(lst::idOf(lst::ATK, r.key, r.l.code, r.l.pivot)); }, 4LL * 1024 * 1024);
    }
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow);
    const int n = (int)std::min((long)times.count, std::min((long)hi.count, (long)lo.count));
    if (n < 2) return;
    const long long published = (long long)currentDate();
    std::string out; std::vector<std::string> ids; std::vector<rse::Event> evidence; std::vector<rse::Frame> frames;
    for (int i = std::max(0, n - 601); i <= n - 2; ++i) {
        const long long key = (long long)times[i]; if (key <= 0) continue;
        if (published > 0) { rse::Frame f0; f0.key = key; f0.pane = lst::ATK; f0.quality = basEvidenceQuality(S, key); f0.contract = S.symbol; f0.published = published; frames.push_back(f0); }
        bool isHigh = false, isLow = false;
        if (i >= atk::EXT_BARS) { std::vector<long long> H, Lw; bool ok = true;
            for (int j = i - atk::EXT_BARS; j <= i && ok; ++j) { off::Tick h = 0, l = 0; ok = off::toTick((double)hi[j], S.tick, h) && off::toTick((double)lo[j], S.tick, l); H.push_back(h); Lw.push_back(l); }
            if (ok) atk::extremeOf(H, Lw, isHigh, isLow); }
        if (i >= 5) {   // (1.32.0) D: the order feed's cumulative delta over this + the 5 candles before vs the candle's low / high
            long long dl[6], lw[6], hg[6]; bool ok[6];
            for (int j = 0; j < 6; ++j) { const int ix = i - 5 + j; long long tt[2] = {0, 0}; off::Tick h = 0, l = 0;
                ok[j] = mboTotOf(S, (long long)times[ix], tt) && off::toTick((double)hi[ix], S.tick, h) && off::toTick((double)lo[ix], S.tick, l);
                dl[j] = tt[1] - tt[0]; lw[j] = l; hg[j] = h; }
            const int piv = lst::cvdDiv(dl, lw, hg, ok, 6);
            if (piv >= 0) {
                const std::pair<int, long long> dk(piv, key);
                if (!S.atkD.count(dk)) { S.atkD[dk] = std::make_pair(dl[5], piv == lst::LOW ? lw[5] : hg[5]); ++S.dCount; while (S.atkD.size() > 800) S.atkD.erase(S.atkD.begin()); }
            }
        }
        std::vector<lst::Lt> v; candleLetters(S, key, isHigh, isLow, v);
        if (published > 0) for (size_t k = 0; k < v.size(); ++k) attackEvidence(S, key, v[k], published, evidence);
        for (size_t k = 0; k < v.size(); ++k) { const std::string id = lst::idOf(lst::ATK, key, v[k].code, v[k].pivot); if (S.stripIds.count(id) || std::find(ids.begin(), ids.end(), id) != ids.end()) continue;
            out += lst::line(lst::ATK, key, v[k]) + "\n"; ids.push_back(id); }
    }
    if (!out.empty()) {
        std::ofstream o(p.c_str(), std::ios::binary | std::ios::app); if (o) { o.write(out.data(), (std::streamsize)out.size()); o.flush(); }
        if (o) { for (size_t k = 0; k < ids.size(); ++k) S.stripIds.insert(ids[k]); S.stripWritten += (long long)ids.size(); } else ++S.stripFail;
    }
    if (published > 0 && (!evidence.empty() || !frames.empty())) {
        const std::string ep = d + "\\" + rse::fileName(lst::ATK, S.market, S.symbol, (int)S.spb); S.evidencePath = ep;
        if (S.evidence.append(ep, evidence, frames)) S.evidenceWritten += (long long)evidence.size(); else ++S.evidenceFail;
    }
    while (S.stripIds.size() > 20000) S.stripIds.erase(S.stripIds.begin());
}
// (1.28.0) the hover of one strip on one candle: the strip header (letter shown, the time, the others), the strip's question rows, then the
// existing per-letter blocks (strongest first) and the pace line - the badge hover's blocks, now per strip
std::string BidAskStats::stripTipText(BASState& S, long long key, int strip, const std::vector<lst::Lt>& v, const lst::Pick& pk) {
    atk::BadgeIn in; std::vector<std::string> others;
    for (size_t q = 0; q < pk.order[strip].size(); ++q) {
        const lst::Lt& l = v[(size_t)pk.order[strip][q]];
        if (q > 0) others.push_back(l.code);
        const int key6 = l.pivot;
        if (l.code == "S") in.S = key6; else if (l.code == "K") in.K = key6; else if (l.code == "G") in.G = key6; else if (l.code == "Tr") in.Tr = key6;
        else if (l.code == "R") in.R = key6; else if (l.code == "L") in.L = key6;
    }
    const std::string full = badgeTipText(S, key, in, false, false, std::string(), std::vector<std::string>());
    // split into header / blocks / pace (blocks are separated by an empty line; the pace line starts "Hover only:")
    std::vector<std::string> parts; { size_t a = 0; for (;;) { const size_t b = full.find("\n\n", a); parts.push_back(full.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 2; } }
    std::vector<std::string> blocks; std::string pace;
    for (size_t k = 1; k < parts.size(); ++k) { if (parts[k].compare(0, 11, "Hover only:") == 0) pace = parts[k]; else if (!parts[k].empty()) blocks.push_back(parts[k]); }
    auto codeOf = [](const std::string& blk) { const size_t sp = blk.find(' '); return blk.substr(0, sp); };
    std::stable_sort(blocks.begin(), blocks.end(), [&](const std::string& a, const std::string& b) { return lst::rankOf(lst::ATK, codeOf(a)) < lst::rankOf(lst::ATK, codeOf(b)); });
    struct tm x; std::memset(&x, 0, sizeof(x));
    const std::string when = key > 0 && getLocaltime((RTDATE)key, &x) ? bas::clock12(x.tm_hour, x.tm_min) : std::string();
    std::string t = lst::header(lst::ATK, strip, v[(size_t)pk.shown[strip]].code, when, others);
    const std::vector<std::string> qr = lst::stripQuestion(lst::ATK, strip);
    t += "\n"; for (size_t k = 0; k < qr.size(); ++k) t += "\n" + dt::wrapRow(qr[k]);
    for (size_t q = 0; q < pk.order[strip].size(); ++q) {
        const lst::Lt& l = v[(size_t)pk.order[strip][q]];
        if (l.code == "E") t += "\n\nE " + l.nums + "\nMeasured classified MBO executions; >=20s after the extreme. Fading is not a reversal guarantee.";
        if (l.code == "D") t += "\n\nD " + l.nums + "\nWhat: the order feed's cumulative delta (buyers - sellers, implied fills excluded) made a new " + std::string(l.pivot == lst::LOW ? "low" : "high") + " over this and the 5 candles before, but the candle did not." + "\nWhy it matters: the attackers hit harder and price would not follow - they are trapped (step 3 of the Reversal Stage, counted within 5 candles of a collision at the level)." + "\nExample: red D at a low = sellers -1,240 delta, yet the low held 3 ticks above the prior low.";
    }
    for (size_t k = 0; k < blocks.size(); ++k) t += "\n\n" + blocks[k];
    if (!pace.empty()) t += "\n\n" + pace;
    return t;
}
// (1.28.0, Rassul approved layout-stages-2strips.png) the TWO LETTER STRIPS under the question line: "Attack" (S > K > L) and "Running out"
// (R > Tr > G), a small grey label at the left of each; ONE letter per strip per closed candle in the colour of the side that acts, no
// boxes / borders; neighbours never touch (lst::placeStrip: the lower half-row, else a small shift, else not drawn); never over the box.
// Every drawn letter keeps its rectangle + its strip's hover for the mouse.
template <class XOf>
void BidAskStats::badgeRow(BASState& S, const off::PaneGeo& g, int v0, int v1, int last, int slot, RTARRAYI& times, XOf xOf, const bp::Box* box) {
    (void)slot;
    S.badgeHit.clear();
    const int y0 = g.top + g.reserve + 1;
    const int NS = 1;                                                      // (1.33.0) one strip: the S letter only
    if (y0 + NS * lst::ROW_H + 10 > g.bottom) { S.badgeTip = -1; return; }   // pane too short
    RTARRAY hi(barHigh), lo(barLow);
    struct C { int i; long long key; int xc; std::vector<lst::Lt> v; lst::Pick pk; };
    std::vector<C> cs;
    for (int i = std::max(0, v0); i <= v1 && i < last; ++i) {
        const long long key = (long long)times[i];
        const int xc = xOf(i); if (xc < g.left || xc > g.edge) continue;
        bool isHigh = false, isLow = false;
        if (i >= atk::EXT_BARS && S.tick > 0 && i < hi.count && i < lo.count) {
            std::vector<long long> H, Lw; bool ok = true;
            for (int j = i - atk::EXT_BARS; j <= i && ok; ++j) { off::Tick h = 0, l = 0; ok = off::toTick((double)hi[j], S.tick, h) && off::toTick((double)lo[j], S.tick, l); H.push_back(h); Lw.push_back(l); }
            if (ok) atk::extremeOf(H, Lw, isHigh, isLow);
        }
        C c; c.i = i; c.key = key; c.xc = xc; candleLetters(S, key, isHigh, isLow, c.v); if (c.v.empty()) continue;
        c.pk = lst::pick(lst::ATK, c.v); cs.push_back(c);
    }
    off::fillFlat(this, g.left, y0, g.edge, y0 + NS * lst::ROW_H, (COLOR)0x000B0F15);
    off::fillFlat(this, g.left, y0 + NS * lst::ROW_H, g.edge, y0 + NS * lst::ROW_H, (COLOR)0x00374151);
    FONT f; f.id = HELVETICA; f.size = 8; f.style = BOLD;
    for (int s = 0; s < NS; ++s) {
        const int yr = y0 + s * lst::ROW_H;
        FONT fl = f; fl.style = PLAIN; setFont(fl); setTextColor(C_BORDER);
        { RCT rc; rc.set(off::coord(g.left + 3), off::coord(yr), off::coord(g.left + lst::LABEL_W), off::coord(yr + lst::ROW_H)); rc.drawText(lst::stripName(lst::ATK, s), false, false); }
        setFont(f);
        std::vector<lst::Item> items; std::vector<size_t> idx;
        for (size_t k = 0; k < cs.size(); ++k) { if (cs[k].pk.shown[s] < 0) continue; if (!S.showS || cs[k].v[(size_t)cs[k].pk.shown[s]].code != "S") continue;   // (1.33.0) S only
            lst::Item it; it.xc = cs[k].xc; it.w = (int)getTextWidth(cs[k].v[(size_t)cs[k].pk.shown[s]].code.c_str(), -1) + 1; items.push_back(it); idx.push_back(k); }
        const std::vector<lst::Placed> pl = lst::placeStrip(items, g.left + lst::LABEL_W, g.edge);
        for (size_t q = 0; q < pl.size(); ++q) {
            if (!pl[q].drawn) { ++S.stripSkipped; continue; }
            const C& c = cs[idx[q]]; const lst::Lt& l = c.v[(size_t)c.pk.shown[s]];
            const int t = yr + pl[q].sub * lst::SUB_H, b = t + lst::SUB_H;
            if (box && pl[q].l < box->r && pl[q].r > box->l && t < box->b && b > box->t) { ++S.stripSkipped; continue; }   // never over the box
            setTextColor(l.colour == lst::GREEN ? C_ASK : C_BID);
            RCT rc; rc.set(off::coord(pl[q].l), off::coord(t), off::coord(pl[q].r + 1), off::coord(b)); rc.drawText(l.code.c_str(), false, false);
            off::IconRect ir; ir.l = pl[q].l; ir.t = t; ir.r = pl[q].r; ir.b = b; std::string tip = stripTipText(S, c.key, s, c.v, c.pk);
            for (const lst::Lt& e : c.v) if (e.code == "E") tip += "\n\nE  Attack pace (hover only, not drawn): " + e.nums;   // (1.33.0) E pace lives in the S hover
            S.badgeHit.push_back(std::make_pair(ir, tip));
            ++S.stripsDrawn;
        }
    }
    if (S.badgeTip >= (int)S.badgeHit.size()) S.badgeTip = -1;
    ++S.badgesDrawn;
}
// the badge's hover box: measured like lsBookFlow's (first line bold, columns when the pane is short), placed by dt::tipSpot below the badge,
// inside the pane and left of the price axis, never over the info box; the fit cascade drops Use / Example, then Tested / Status, then 8 pt
BidAskStats::TipLay BidAskStats::tipLayout(const std::string& text, int top, int bottom, int pt) {
    TipLay T; { std::string cur; for (size_t k = 0; k < text.size(); ++k) { if (text[k] == '\n') { T.lines.push_back(cur); cur.clear(); } else cur += text[k]; } T.lines.push_back(cur); }
    FONT fb; fb.id = HELVETICA; fb.size = pt; fb.style = BOLD; FONT fp = fb; fp.style = PLAIN;
    T.w.assign(T.lines.size(), 0);
    for (size_t k = 0; k < T.lines.size(); ++k) { setFont(k == 0 ? fb : fp); T.w[k] = T.lines[k].empty() ? 0 : (int)getTextWidth(T.lines[k].c_str(), -1); }
    const int lh = pt + 4, pad = 6, gap = 18, rows = std::max(1, (bottom - 2 - top - 2 * pad) / lh);
    T.cols = off::tipColumns(T.lines, T.w, rows);
    int used = 0; T.boxW = 2 * pad; for (size_t c = 0; c < T.cols.size(); ++c) { T.boxW += T.cols[c].w + (c ? gap : 0); used = std::max(used, (int)(T.cols[c].to - T.cols[c].from)); }
    T.boxH = 2 * pad + used * lh; T.pt = pt; return T;
}
void BidAskStats::drawBadgeTip(BASState& S, const off::PaneGeo& g, const off::IconRect& badge, const std::string& text, const bp::Box* box) {
    const int R = std::min(g.right, g.edge);
    dt::TipRect br; br.l = badge.l; br.t = badge.t; br.r = badge.r; br.b = badge.b;
    dt::TipRect bx; if (box) { bx.l = box->l; bx.t = box->t; bx.r = box->r; bx.b = box->b; }
    dt::TipRect at; std::string shown; int level = -1;
    for (int lv = 0; lv < 5 && level < 0; ++lv) {                          // all rows -> - Use / Example -> - Tested / Status -> - attacker combinations -> 8 pt
        const std::string t = atk::compact(text, std::min(lv, 3)); const int pt = lv < 4 ? 9 : 8;
        if (dt::tipSpot(g.left, g.top, R, g.bottom, br, box ? &bx : nullptr, [&](int top, int& w, int& h) { const TipLay T = tipLayout(t, top, g.bottom, pt); w = T.boxW; h = T.boxH; }, at)) { level = lv; shown = t; }
    }
    if (level < 0) { ++S.badgeTipNoRoom; return; }
    S.lastTipLevel = level; if (level > 0) ++S.badgeTipCompact;
    const TipLay T = tipLayout(shown, at.t, g.bottom, level < 4 ? 9 : 8);
    setPen(off::C_TIP_BORDER, 1, P_SOLID);
    off::fillRect(at.l, at.t, at.r, at.b, off::C_TIP_BORDER, off::C_TIP_BG, 1);
    FONT fb; fb.id = HELVETICA; fb.size = T.pt; fb.style = BOLD; FONT fp = fb; fp.style = PLAIN;
    int cx = at.l + 6; const int lh = T.pt + 4;
    for (size_t c = 0; c < T.cols.size(); ++c) {
        int y = at.t + 6;
        for (size_t k = T.cols[c].from; k < T.cols[c].to; ++k, y += lh) {
            if (T.lines[k].empty()) continue;
            setFont(k == 0 ? fb : fp); setTextColor(off::C_TIP_TEXT);
            RCT t; t.set(off::coord(cx), off::coord(y), off::coord(std::min(at.r - 2, cx + T.cols[c].w + 4)), off::coord(y + lh)); t.drawText(T.lines[k].c_str(), false, false);
        }
        cx += T.cols[c].w + 18;
    }
    S.lastTip = at;
}

void BidAskStats::render(BASState& S) {
    RCT pane; pane.getPaneRect(false);
    S.cur.pane.l = pane.left; S.cur.pane.t = pane.top; S.cur.pane.r = pane.right; S.cur.pane.b = pane.bottom;   // (1.14.1)
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
    std::map<int, bas::BarMax> mboCache;                                  // (1.23.0) the bars' numbers with order-by-order totals, this draw
    auto maxOfBar = [&](int i) -> const bas::BarMax* {
        const long long bt = (long long)times[i];
        const bas::BarMax* m = nullptr;
        if (i == last) m = S.liveTime == bt && S.live.ok ? &S.live : nullptr;
        else { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(bt); m = it != S.ser.bars.end() && it->second.m.ok ? &it->second.m : nullptr; }
        if (!m) return nullptr;
        std::map<int, bas::BarMax>::iterator c = mboCache.find(i);
        if (c == mboCache.end()) c = mboCache.insert(std::make_pair(i, withMbo(S, bt, *m))).first;
        return &c->second;
    };
    long long top = 0, upMax = 0, downMax = 0;
    std::vector<long long> mags;
    for (int i = v0; i <= v1; ++i) { const bas::BarMax* m = maxOfBar(i); if (m) { top = std::max(top, std::max(m->totBid, m->totAsk)); upMax = std::max(upMax, m->totAsk); downMax = std::max(downMax, m->totBid); mags.push_back(m->totBid); mags.push_back(m->totAsk); } }   // (1.19.0) total lots
    { const long long cp = off::capTop(mags); upMax = off::capped(upMax, cp); downMax = off::capped(downMax, cp); top = off::capped(top, cp); }   // (1.17.0, Rassul 18:26) the scale cap: 1.5x the 90th percentile on screen
    if (S.axisTop > top) top = S.axisTop;                                  // draw on the top the value axis was given
    upMax = std::max(upMax, S.axisUp); downMax = std::max(downMax, S.axisDown);   // (1.16.0) the mirror's two sides, as the axis was given
    S.paneH = pane.bottom - pane.top;
    // (1.13.0) the forming candle is never judged: no stats, so it is drawn dim with no label (closed candles only - no repaint)
    auto statOfBar = [&](int i) -> const bas::Stat* {
        if (i == last) return nullptr;
        std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find((long long)times[i]);
        return it != S.ser.bars.end() && it->second.m.ok ? &it->second : nullptr;
    };
    const int hov = S.hover.bar >= 0 && S.hover.bar < last ? S.hover.bar : -1;   // a bar index past the data (bars trimmed) = Live
    off::PaneGeo g = off::paneGeo(this, off::MODE_NONE);   // (1.9.0, Rassul 2026-10-16 18:10) no text lines on the pane
    g.reserve += off::QUESTION_H;                                          // (1.18.0) the question line on top
    const int badgeY0 = g.top + g.reserve + 1;                             // (1.26.0) the attacker badge row: under the question line, above the bars
    g.reserve += BAS_BADGE_H;
    // (12:05) the info box (the 1.2.0 box + Sweep + Large) and the message line, both for the hovered closed candle (as of that
    // candle), else the last closed candle (header "Live")
    const int mb = hov >= 0 ? hov : last - 1;
    const long long mt = (long long)times[mb];
    std::map<long long, BasFin>::const_iterator f = S.fin.find(mt);
    const bas::Stat* bst = NULL;
    if (hov >= 0) { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(mt); if (it != S.ser.bars.end()) bst = &it->second; }
    else { std::map<long long, bas::Stat>::const_iterator it = S.ser.bars.find(S.lastClosedTime); if (it != S.ser.bars.end() && S.lastClosedTime == mt) bst = &it->second; }
    // (1.14.1) Live = the FORMING candle: its biggest bid / ask so far against the prior closed candles' average, its pace so far (box only:
    // the forming bar on the chart stays dim and unlabelled until it closes)
    bas::Stat liveSt; bas::PaceIn livePc; bool liveBox = false;
    if (hov < 0 && S.live.ok && S.liveTime == (long long)times[last]) {
        std::vector<long long> pb, pa; S.ser.prior(S.liveTime, (size_t)bas::RATIO_HI, pb, pa);
        liveSt = bas::statOf(S.live, pb, pa, S.ratioBars); livePc = livePace(S, liveSt); bst = &liveSt; liveBox = true;
        S.liveDrawnT = S.liveTime; S.liveDrawnBid = S.live.bid; S.liveDrawnAsk = S.live.ask;
    }
    // (1.5.0) two columns: Sellers (bids hit, red) | Buyers (offers lifted, green)
    off::Info info; info.twoCol = true; info.colHead[0] = "Sellers"; info.colHead[1] = "Buyers"; info.tmpl = "99.9x through";   // (1.17.0) wide enough for the Reward cells ("8.9x through" was cut off);   // (1.15.0) Lots / Pace / BBI / Read only: the Lots template (Crowd is not shown)
    bas::PaceIn readPc;                                                    // (1.15.0) the pace the Read row uses (the candle shown)
    std::array<er::Side, 2> erNow;                                         // (1.16.0) Attack Effort / Reward of the candle shown
    {
        struct tm x; std::memset(&x, 0, sizeof(x));
        info.head = hov < 0 ? std::string("Live") : (mt > 0 && getLocaltime((RTDATE)mt, &x) ? bas::clock12(x.tm_hour, x.tm_min) : std::string("n/a"));
        const std::string none[2]; const bas::PaceIn nopc;
        const bool rec = !liveBox && f != S.fin.end();                    // (1.14.1) the live candle has no record yet
        readPc = liveBox ? livePc : rec ? f->second.pc : nopc;
        const std::vector<bas::Row2> L = bas::infoRows(bst, rec ? &f->second.r : nullptr, rec ? f->second.let : none, rec && f->second.dbo,
                                                       liveBox ? livePc : rec ? f->second.pc : nopc, S.params.load.p.paceFastT);
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
        {   // (1.10.1, Rassul 2026-10-16 22:54) the box shows Lots and Pace only (no Sweep, Large, StopRun, Crowd, Burst)
            std::vector<off::InfoRow> kept;
            for (size_t k = 0; k < info.rows.size(); ++k) if (info.rows[k].label == "Lots") {   // (1.19.0, Rassul 19:25) Pace removed (unproven)
                off::InfoRow r = info.rows[k];                               // (1.19.0) Lots = the side's TOTAL aggressive lots (the bar), its ratio is Attack Effort
                {   const int lc = liveBox ? last : mb; const bas::BarMax* lm = lc >= 0 && lc <= last ? maxOfBar(lc) : nullptr;   // (1.23.0) the same candle and totals as Effort / Reward
                    bool src = false; if (lm) { long long o[2]; src = mboTotOf(S, (long long)times[lc], o); }
                    if (lm) { r.v[0] = er::lotsText(lm->totBid); r.v[1] = er::lotsText(lm->totAsk); }
                    if (lm && !src && BAS_MBO_TOTALS) r.label = "Lots (IRT)";                 // no order data for this candle: IRT's bid / ask volume
                }
                kept.push_back(r);
            }
            info.rows.swap(kept);
        }
        if (false) {   // (1.19.0, Rassul 19:25) BBI / BAI removed (never tested); the code kept
            const bool ok = bst && bst->m.ok;
            off::InfoRow r; r.label = "BBI / BAI";
            r.v[0] = ok ? bas::beatText(bst->bBid, bst->bBidCap, true) : std::string("n/a");
            r.v[1] = ok ? bas::beatText(bst->bAsk, bst->bAskCap, true) : std::string("n/a");
            info.rows.push_back(r);
        }
        // (1.15.0) the BL5 row is removed (Rassul 23:48 CT); the Big Lot size stays in the status file and the cyan cap on the chart
        {   // (1.16.0, EffortReward.h) ATTACK EFFORT and REWARD under Lots, for the candle shown (hovered closed, else the live forming one)
            const int ci = liveBox ? last : mb;
            erNow = erSides(S, ci, times, last);
            off::InfoRow ef; ef.label = "Effort";   // (1.20.0) was "Attack Effort"
            off::InfoRow rw; rw.label = "Reward";
            for (int q = 0; q < 2; ++q) { ef.v[q] = er::effortCell(erNow[q].effOk, erNow[q].eff); rw.v[q] = er::rewardCellOf(erNow[q]); }
            std::vector<off::InfoRow>::iterator at = info.rows.begin(); if (at != info.rows.end()) ++at;   // after Lots
            at = info.rows.insert(at, ef); info.rows.insert(at + 1, rw);
        }
        {   // (1.29.0, IMPLIED FILLS option B) "Implied: 23% of this candle's volume" - the order feed's side "?" fills of the candle shown
            std::map<long long, off::Acc>::const_iterator ac = S.accs.find(liveBox ? (long long)times[last] : mt);
            off::InfoRow ir; if (ac != S.accs.end() && off::impliedRow(S.e.ip, ac->second.fillAll, ac->second.fillImpl, ir)) info.rows.push_back(ir);
        }
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
    // (1.10.0) the pane shows the numbers box only (1.13.0: the "Box shows" setting that did nothing is gone); records + tape reader unchanged
    off::PanelPlan P;
    (void)zl; (void)optL; (void)l1;
    // (1.15.0) + the Read row: the box grows to hold its lines, wrapped to the box width (BoxReadDraw.h)
    br::Read rd = bas::aggRead(bst, readPc); (void)rd;
    {   // (1.16.0) at a new 10-bar low / high the Effort vs Reward read leads (absorbed / going through), as Book Flow's refill read does
        const int ci = liveBox ? last : mb; bool nlw = false, nhi = false;
        { const er::Side sd0[2] = { erNow[0], erNow[1] }; rd = er::effortRead(sd0); }   // (1.19.0) the read from Attack Effort (was the one-price Lots ratio)
        if (newExtreme(ci, nlw, nhi)) { br::Read er0; const er::Side sd[2] = { erNow[0], erNow[1] }; if (er::read(nlw, nhi, sd, er0)) rd = er0; else if (er::counterRead(nlw, nhi, sd, er0)) rd = er0; else rd = er::effortRead(sd); }   // (1.17.0) + support / resistance coming in at 1.5x; (1.19.0) else the Attack Effort read
    }
    atk::noQ(rd);                                                          // (1.26.0, Rassul 20:11-20:23) no "?" on the message line
    if (S.feed.silentNow(steadyMs())) { rd = br::Read(); rd.head = off::silentRead(S.feed.silentFromWall, S.feed.reconnects); }   // (1.17.0) the watchdog says so
    const off::TextW tw = off::measureWith(this);
    {   // (1.20.0, Rassul 19:31) Buyers | Sellers - buyers on the LEFT, as the bars and the Defense pane (the rows are built sellers-first)
        std::swap(info.colHead[0], info.colHead[1]);
        for (size_t k = 0; k < info.rows.size(); ++k) std::swap(info.rows[k].v[0], info.rows[k].v[1]);   // (1.21.0) colours follow the column (vcol below), so the headings are right too
    }
    const br::Plan rp = br::addAndPlan(info, rd, tw, [&](const off::Info& I) { return off::planPanel(area, g.top, off::MODE_NUMBERS, pl, I, std::string(), std::string(), g.reserve, tw); }, P);
    S.lastBox = P.ok ? P.box : bp::Box(); S.lastRead = rd; S.lastReadLines = rp.lines;
    std::vector<off::LBox> avoid;
    if (P.ok) avoid.push_back(off::LBox{ P.box.l, P.box.t, P.box.r, P.box.b, 0, 0 });
    // (1.5.3) the hover grid is not an obstacle: it is drawn over the bars while the mouse is in the box (labels never re-decide for it)
    for (size_t k = 0; k < P.textRects.size(); ++k) avoid.push_back(off::LBox{ P.textRects[k].l, P.textRects[k].t, P.textRects[k].r, P.textRects[k].b, 0, 0 });
    const COLOR normal[2] = { C_BID_BAR, C_ASK_BAR }, spike[2] = { C_BID_SPIKE, C_ASK_SPIKE };   // left = bids hit (red), right = offers lifted (green)
    RTARRAY op(barOpen), cl(barClose);                                     // (1.7.0) the close direction (one-bar ties)
    (void)downMax;
    const off::ValY vy = off::paneValY(v0, top, g);                         // (1.23.0, Rassul 08:19 / 14:22) the y-axis drives the bars
    off::drawPairBars(this, g, v0, v1, slot, top, normal, spike, xOf, [&](int i) {   // (1.18.0, Rassul 19:02 / 19:09) BOTH sides side by side, above zero: buyers green left, sellers red right
        off::BarLook k; const bas::BarMax* bm = maxOfBar(i); if (!bm) return k;
        const bas::Stat* sp = statOfBar(i);
        k.ok = true; k.v[0] = bm->totBid; k.v[1] = bm->totAsk;          // (1.19.0, Rassul 19:25) the bars = each side's TOTAL aggressive lots (the validated Attack Effort)
        (void)sp;
        if (i != last) { const std::array<er::Side, 2> ef = erSides(S, i, times, last);   // bright + its ratio at 2.0x+ Attack Effort (closed candles only, no repaint)
            for (int q = 0; q < 2; ++q) { k.ratioOk[q] = ef[q].effOk; k.ratio[q] = ef[q].eff; } }
        if (i < op.count && i < cl.count) { const float o = op[i], c = cl[i]; if (std::isfinite(o) && std::isfinite(c)) k.dir = c > o ? 1 : c < o ? -1 : 0; }
        if (i != last) { std::map<long long, BasFin>::const_iterator ff = S.fin.find((long long)times[i]); if (ff != S.fin.end()) { (void)ff; /* (1.35.0) the bar letters are recorded only, never drawn */ /* (1.22.0) no pace on the bar labels (unproven) */ } }
        if (i != last) {                                                   // (1.4.0) the stop-run mark on the break bar's attacking side
            std::map<long long, std::vector<sr::Event> >::const_iterator se = S.srEv.find((long long)times[i]);
            if (se != S.srEv.end()) for (size_t q = 0; q < se->second.size(); ++q) {
                const sr::Event& e = se->second[q]; if (e.show == sr::SHOW_OFF) continue;
                (void)e;   // (1.35.0) the stop-run R / R? mark is recorded only, never drawn
            }
        }
        return k;
    }, avoid, 1, &vy);   // (1.18.0) pair bars, buyers (side 1) left (was the 1.16.0 mirror) (was drawBars one bar per candle, 1.7.0); bright + ratio / pace only at 2.0x+, no letters (1.7.1)   // (1.9.1) only bars at 2.0x+ their average are bright and labelled; the rest are dim   // (1.7.1, Rassul 09:55 "get rid of the labeling like S L") no letters / R marks drawn; hover + record keep them                                                          // (1.7.0) one bar per candle; buyers = side 1 (offers lifted)
    if (S.bigLot >= 1 && S.bigLot <= 3 && top > 0) {                       // (1.14.0) the Big Lot cap: 3 px cyan on the top of a marked closed bar
        for (int i = v0; i <= v1 && i < last; ++i) {
            const blt::Mark* mk = markAt(S, (long long)times[i]); if (!mk || !blt::shows(*mk, S.bigLot)) continue;
            const int xc = xOf(i); if (xc < g.left || xc > g.right) continue;
            const bas::BarMax* bmk = maxOfBar(i); if (!bmk) continue;
            int yb = g.bottom - 1; const int hb = off::pairBarH(&vy, g, top, mk->side == 1 ? bmk->totAsk : bmk->totBid, yb); if (hb <= 0) continue;   // (1.23.0) on the axis like the bar   // (1.19.0) the cap marks the candle on top of its side's total bar
            const off::Pair pr = off::pairIn(xc, slot); const int pos = mk->side == 1 ? 0 : 1;   // buyers left, sellers right
            const int l = pr.l[pos], r = pr.r[pos];
            const int cap = std::min(BIG_LOT_CAP_PX, hb);
            const int y0 = yb - hb + 1, y1 = y0 + cap - 1;
            off::fillFlat(this, l, y0, r, y1, C_BIG_LOT);
        }
    }
    {   off::PaneGeo gb = g; gb.reserve = badgeY0 - g.top - 1;             // (1.26.0) the attacker badge row (one badge per candle, never over the box)
        (void)gb; S.badgeHit.clear(); S.badgeTip = -1; }   // (1.35.0) the S strip is not drawn (still decided + exported)
    const COLOR vcol[2] = { C_ASK, C_BID };                                // (1.21.0) column 1 Buyers green, column 2 Sellers red - headings and values (1.20.0 had the headings swapped)
    off::drawPanel(this, P, info, vcol, off::implOf(zl), false);
    br::draw(this, P, info, rp, rd);                                      // (1.15.0) the Read word + lines over its rows
    off::drawQuestion(this, g, bas::QUESTION, P.ok ? &P.box : nullptr, &S.infoIcon);   // (1.22.0) + the info icon
    if (S.infoTipOn) off::drawInfoTip(this, g, S.infoIcon, bas::INFO_TIP);  // (1.23.0) the description box while the mouse is on the icon (last: over everything)   // (1.18.0, Rassul 19:09) the question the pane answers
    S.lastTip = dt::TipRect(); S.lastTipLevel = -1;
    if (!S.infoTipOn && S.badgeTip >= 0 && S.badgeTip < (int)S.badgeHit.size())   // (1.26.0) the badge's hover, per letter, inside the pane, never over the box
        drawBadgeTip(S, g, S.badgeHit[(size_t)S.badgeTip].first, S.badgeHit[(size_t)S.badgeTip].second, P.ok ? &P.box : nullptr);
    (void)hov;   // (1.8.1, Rassul 18:57 "get rid of the hoverover") no bar hover, no read-line hover
}

extern "C" cppExtension* CreateExtension(void) {
    BidAskStats* p = new BidAskStats();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);       // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                                // (1.3.0) the order feed is drained while no trade prints
    p->setDescription("LRA Aggression: who is PRESSING - aggressive sellers hitting the bid or aggressive buyers lifting the offer. One bar per candle = the bigger side: green = buyers (the most contracts bought at the ask at one price in the candle), red = sellers (the most contracts sold into the bid at one price). A CLOSED candle is drawn bright, with its ratio (yellow, e.g. 2.4 = 2.4x) and its pace (grey, aggressive lots per second vs the same time of day) above the bar, only when that side is at least Spike vs average (default 2.0x) its plain average over the prior Ratio bars (default 20) closed candles - decided once at the close and never changed. Every other candle is drawn dim with no label; the forming candle stays dim until it closes. The numbers box in the pane shows Lots (ratio), Pace, BBI / BAI (how many prior candles in a row the biggest bid / ask beats, up to 50+) and the Read (BULLISH / BEARISH / NEUTRAL: who is attacking, in words) for sellers and buyers of the candle under the mouse while it is inside this pane, else the live forming candle. Big Lot mark (Off / BL5 / BL10 / BL20): a cyan cap on a closed candle whose bar is at least the Big Lot size of its session type (RTH / overnight: the average of the 5th / 10th / 20th biggest print of the latest 5 complete earlier sessions, as in Big Bid/Ask; no cap while fewer than 3 are known), decided at the close and never changed; the box shows that size. Settings: Position (the box), Big Lot mark.");
    p->setVersion("1.36.0");   // BAS_VERSION
    return p;
}
