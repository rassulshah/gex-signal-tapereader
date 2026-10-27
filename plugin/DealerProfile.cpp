/********************************************************************************
 *  DealerProfile.cpp  --  Investor/RT RTX extension  lsDealerProfile  (v1.0, 2026-09-29; v2.12.0, 2026-10-14; v2.17.0, 2026-10-18)
 *
 *  2.17.0 (2026-10-18 night, Rassul approved the audit-C fixes - findings 7 / 10 + the Market State): the grid's STALE test is the
 *     shared Fresh.h rule (the same thresholds as the HOD / LOD box): over 20 min old while MenthorQ updates; while it pauses (16:00-
 *     21:40 CT, the weekend) the closing set is fine but an older one is STALE (before: never stale outside MenthorQ hours, and the
 *     old-file badge waited 25 min). One status file PER CHART, DealerProfile.status-<MKT>-<spb>.txt (before: one file for every chart,
 *     the last chart to draw won - the Tape Reader could lay out against another chart's REACH); DealerProfile.status.txt is still
 *     written for one more version (ChartView / Tape Reader read it), each chart with its own write gate (was a function static shared
 *     by all charts). MARKET STATE (MarketState.h): SRC "GEX" (regime / hvl / cr / ps / the level nearest price, its distance and the
 *     dealers' hedge lots there / data age / stale) for the last closed bar, re-posted every 10 s, cleared on removal. DEX stays
 *     context inside the gamma read (no DEX signal). Nothing drawn moved.
 *  2.12.0 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, APPROVAL-LIST-2026-10-14.md section 5): hovers
 *     say what the vendor model shows, not a hold / turn forecast; "How to use" = his locked trigger; HL badge "HL?"; touch odds
 *     "a chance to REACH only"; the pick marked ?; HVL regime text behind the hvl_cushion gate. Nothing on the chart moved; the node
 *     B / S text and "futures dealers must BUY N" stay (KEEP, his call 10-14).
 *  2.11.0 (2026-10-14, CR5200, Rassul approved 11:00 CT - mockup-columns.png 10:53 "Liquidity 60 stays LEFT (solid) | right:
 *     Levels 40 | Delta 80 | Dealer 80"): the right side of the price chart is three columns, left to right after a 20 px gap from
 *     the last bar: the LEVELS column (compact badges ONH / CR0 / HVL / PFH / PDH / LonLO, close ones stacked with a tick to their
 *     line; the level line still runs across the chart), the Delta Profile, then the DEALER column (this Profile). This plugin sets
 *     the chart's right margin (gap + levels + delta + dealer, ColumnBoard.h) so price never enters the columns, recomputed when any
 *     width changes. The Profile is one-sided: every bar anchored on the column's right edge growing left toward price, + teal /
 *     - orange; inside each node "B 1.2K" / "S 980" = futures dealers must buy / sell to stay hedged at that price (hidden when the
 *     node is too short or the number is missing); the hover adds the $. The wide level labels ("7870.00 CR0 STALE 24m 1h 68%?")
 *     and the HVL regime tags are gone from the chart - the price, odds "33%  /  50%", liquidity and STALE / age are in each badge's
 *     hover. Settings (controls only, per market): Dealer column px 40-250 (80), Levels column px 30-150 (78 since 11:39), Badge font (9),
 *     Show buy / sell text in node. The dialog is filled from the settings file and an untouched control never overwrites it.
 *     (Rassul 11:39 CT, design B - mockup-hl.png) each badge = [name on its level colour] + [the contracts resting at the level, the
 *     Liquidity Profile's own band, "310" / "1.2K" grey on dark grey]; at the market's LQ confirmation line (2.5x the book behind,
 *     off under 2.1x after 30 s) the block reads "HL 900" bold white on GREEN (bids below price) / RED (offers above) with a 2 px
 *     white outline, and the level line is drawn 3 px across the chart. Native: lsLiquidityProfile 0.16.0 writes the per-price
 *     numbers to lsFlexLevels\LRA-LiqLevels-<MKT>.csv; no file / older than 20 s = name-only badges (never 0). Levels column default
 *     78 px (30-150); "Liquidity in badge" checkbox.
 *     (14:37 CT review, Rassul "F everywhere, makes no sense") no W / F letters anywhere (the colour says it: teal = wall / long gamma,
 *     orange = fuel / short gamma; the hover explains W / F); strikes under 3% of the biggest wall on screen are not drawn; node and
 *     badge text sized to the real text height (pt x 4/3) with padding - hidden / shortened rather than clipped; HrHI / HrLO never shown
 *     (an older shared key-level file can still carry them).
 *  2.9.0 (2026-10-13, Rassul approved 18:05 - dealer-profile-mockup.png): each node's LENGTH = MenthorQ's ABSOLUTE GEX at the
 *     strike (the wall's size), its colour + a "W" / "F" letter by the NET sign (cyan W = wall, net long gamma: holds / pins; lime
 *     F = fuel, net short gamma: accelerant); big absolute with near-zero net = a thin muted node. The 3 biggest strikes carry
 *     their % change since the session's first grid ("+7%"). The dealer hedge on the way to a strike is a coloured number only
 *     (red "-599" = dealers SELL, green "599" = BUY) on the strikes >= 35% of the biggest wall. No GEX value on the face - it is
 *     in the hover. HVL regime tags (+GAMMA fades above / -GAMMA runs below). Header "Dealer Profile v2.9.0" + the grid's time;
 *     "STALE 10h - 4:55 AM" and faded nodes when the grid is over 20 min old while MenthorQ updates. MenthorQ levels: "CR 0D /
 *     GW 0D" = one thin "CR0" line; G2-G10 hidden unless on a key level. Labels stay inside the Profile's published REACH (never
 *     over the Delta / Liquidity Profile columns); every text 10 pt+. Hovers on nodes, HVL tags and the header: the question,
 *     how the number answers it, how to use it, an example. Needs file 2.4 (NODEX / BOOKASOF rows); older files draw as 2.8.7.
 *  2.7.4 (2026-10-12, review): every level badge reads "<price> <NAME>  <odds>" (key levels now carry their price, like the
 *     MenthorQ badges), 10 pt with the name in bold; MenthorQ levels green = PS (support), red = CR (resistance), yellow = HVL,
 *     cyan = other gamma levels; badges never cover a price bar, another badge or a level line, nor the Liquidity / Delta /
 *     Dealer Profile columns. Fixes: key levels from a partly loaded session / week are not drawn; a rolled or Rithmic-named
 *     chart (ESH7, ESZ6) gets the right MenthorQ contract and offset; the status file and the settings file are no longer
 *     written / re-read on every repaint; the MinGW / GNU syntax check compiles (snprintf).
 *  2.0.0 (2026-10-02): THE DEALER PROFILE = one bar per strike of MenthorQ's net GEX (dealer delta change in $ per 1-pt
 *     move): red = dealers SHORT gamma (fuel: they chase price), green = LONG (wall: they lean against it), the darker part
 *     = today's 0DTE (gone at expiry), the value ("-3.9M") on the bar. Same numbers as MenthorQ's Net GEX panel. The running
 *     BUY / SELL total moved to the Dealer Read. Files without net GEX (older Reader) still draw the 1.x nodes below.
 *  1.x: two nodes per strike on the right edge of the price pane, facing price.
 *     (1.3.6: delta on TOP, gamma below)
 *     lower node  synthetic GAMMA on arrival (futures per 0.1 EM): yellow = dealers long gamma (a WALL), purple = short (FUEL)
 *                 "4220  10  (9 to 17)": 10 at the usual pace, 9 if price gets there twice as fast, 17 twice as slow
 *                 (1.3.5 - the whisker line is gone, Rassul 2026-09-30); outline = MenthorQ's snapshot now
 *     upper node  synthetic DELTA: futures dealers must trade because of this strike on the way there (green BUY / red SELL)
 *  Values sit INSIDE the nodes when they fit (operator 2026-09-29: no text outside the nodes); the rest is on the
 *  whole-book banner (top-left) and the legend line. Strikes beyond 1.2 EM fade.
 *
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Dealer-<MKT>.csv, written every 5 min by the LRA Reader
 *  (level-reversal-analytics analytics/lra/dealer_irt.py). Grammar and decisions: DealerLogic.h (tested,
 *  test_dealer_logic.cpp). The market comes from the chart's root symbol (EP -> ES, GCE -> GC ...) unless set.
 *
 *  A separate indicator from lsDealerRead (the checklist strip), so each can be on or off, on any chart.
 *  Parameters are read ONLY in the parms callbacks (GAMMA-PROFILE-PLUGIN.md gotcha 3); positions numbered explicitly.
 *  Never black: the chart background is black.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <map>
#ifdef _WIN32
#include <windows.h>     // BEFORE irtsdk.h: the SDK's RTBOOL clashes with minwindef.h's BOOL the other way round (MSVC C2371)
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "DealerProfile"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#if defined(DP_TEST_CURSOR)
extern void* dp_test_cursor_wnd;
static void* dpCursorWindow() { return dp_test_cursor_wnd; }
#elif defined(_WIN32)
#pragma comment(lib, "user32.lib")   // (2.9.0) GetCursorPos / WindowFromPoint: the hover clears when the mouse leaves the chart
static void* dpCursorWindow() { POINT p; if (!GetCursorPos(&p)) return NULL; return (void*)WindowFromPoint(p); }
#else
static void* dpCursorWindow() { return NULL; }
#endif
#ifdef far
#undef far
#endif
#include "DealerLogic.h"
#include "DealerProfileSettings.h"   // (2.17.1) the settings window table (SettingsSpec.h)
#include "SettingsHost.h"
#include "HostSlot.h"
#include "DealerProfileAuditLogic.h"
#include "OnTouchLogic.h"   // (2.6.0) the overnight touch odds, natively
#include "KeyLevelsLogic.h" // (2.7.0) the key levels, natively from the chart's bars
#include "QueueMeter.h"     // (2.20.0) the queue meter: the live file from lsLiquidityProfile, the badge text and the hover (SDK-free, tested)
#include "ColumnBoard.h"    // (2.11.0) the right-side columns: widths, layout, margin, board (shared with Delta / Liquidity)
#include "DealerColumnsLogic.h" // (2.11.0) node text, compact badge names, the badge / node hovers
#include "ClaimGates.h"         // (2.12.0) the hvl_cushion gate on the HVL badge hover
#include "Fresh.h"              // (2.17.0) the one freshness / STALE rule (shared with the HOD / LOD box)
#include "SelfAware.h"           // (2.26.0) self-awareness
#include "MarketState.h"        // (2.17.0) SRC GEX, once per closed bar
#include "LevelsColumn.h"       // (2.28.0) the combined LEVELS + LIQUIDITY column (rows, order, liquidity, rank; SDK-free, tested)
#include "LevelPctLogic.h"      // (2.19.0) the level % badges (LRA-LevelBadges-<MKT>.csv)
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>
#include <algorithm>
#include <chrono>
#include <memory>

static const char* DP_VERSION = "2.29.0";   // (2.29.0, 2026-10-27 Rassul 14:35-14:58 CT, mockups level_lines / liqrank / tag_compare) LEVEL LINES: each key / volume level is a line across the price area (right of the left Delta column) with ONE badge centred on it - chips, price, odds, the liquidity rank (top 10) at its end; DPoc cyan, WPoc bright cyan; gamma levels off for now (DP_GAMMA); the Levels column is now the narrow LIQUIDITY column (rank + the big limit orders within 0.5 ATR of the level, LevelsColumn 1.1.0) // was 2.28.0:   // (2.28.0, 2026-10-27, Rassul approved 13:30-13:39 CT "i think this is a cleaner approach. build", mockup levels_final.png) ONE combined LEVELS + LIQUIDITY column: each row = [rank] [liquidity: the big limit orders at the level from lsLiquidityProfile 0.60.0, "800 + 50", red offers / green bids, fill = lots vs the biggest] [level chips: key (solid, SessionPrices colours) then volume (amber outline) then gamma (violet outline) - his 13:37 order] [price] [the % block]; levels within max(2 ticks, 0.3 ATR) share a row; rank 1 = most resting lots on screen (tie: nearer price); every MenthorQ gamma level shown (G2-G10 no longer hidden) + the 3 biggest unnamed gamma strikes as a violet "GEX" row; the nearest level off screen above / below; the Dealer Profile BARS are off (13:36-13:38 "do we need the dealer profile since it is already mentioned in the level" / "lets just get rid of it") - the header (version / STALE) moves to the top of the Levels column; no hovers, no dealers column, no ratio (LevelsColumn.h 1.0.0)   // was 2.27.0:   // (2.27.0, 2026-10-27) the QUEUE METER is no longer drawn in the Levels column (Rassul 2026-10-27 00:43 CT "i dont see the queue helping at all, remove it but you can continue logging it"); its record / file unchanged (lsLiquidityProfile writes it) //   // (2.26.0, 2026-10-26) self-awareness SELFGEX (SelfAware.h, SELF-AWARE.md; ships OFF per market) // 2.25.0 (2026-10-24) level badges = name | price | % block only: no liquidity ratio, no level line after the badge
// prior: "2.24.0";   // 2.23.0 (2026-10-22 evening fix build) QueueMeter.h 1.3.0: the meter badge reads "G1  Q 143 (2.4x)" (lots + the ratio to the normal queue there), the fill / percent only once the queue reached the normal, hover percent capped "200%+" //   // 2.22.0 (2026-10-22 evening, Rassul 17:55 CT "more readable"; QueueMeter.h 1.2.0): the meter badge reads "G1  Q 76%" (wider gap, a space after Q; unlabelled "Q 41%"), no "?" on the badge in any market (NQ / GC still red call only); news / settle window and feed-check lines in the meter hover, a failed feed check = no call outline // 2.21.0 (2026-10-22 evening, the queue meter validation verdict VALIDATE-DEFENDER-SIZE-2026-10-22.md): the meter badge OUTLINE follows the 10-s CALL from lsLiquidityProfile 0.50.0 - 2 px red = under 40% left 10 s after contact (expect the break), 2 px green = 100%+ and holding (ES / CL only); "?" after the text in NQ / GC (red only); no call / LOG markets = 1 px grey (yellow 2 px when the originals are gone); the hover gives the market's out-of-sample odds; the candle-close Q is a record only (its hover line says so); the live file is now LRA-QueueLive-<MKT>.csv //   // 2.20.0 (2026-10-22, Rassul approved the QUEUE METER 13:51-13:52 CT, build ordered 15:20 CT, mockup queue-placement.png panel A): the Levels column doubles as the meter column - the level badge at a tested price fills left to right with the queue left vs the start (green bids defending a low, red offers a high), "G1 Q76%"; originals gone = yellow outline / text "Q33% . orig 0%"; a tested price with no key level (pivot / suddenly defended) = a small unlabeled badge "Q33%"; only within the reach (default 1 ATR) of price; text shrinks to fit, never outside the Levels column; hover with the numbers (? not proven yet); from lsLiquidityProfile 0.49.0's lsFlexLevels\\LRA-QueueMeter-<MKT>.csv (QueueMeter.h 1.0.0; missing / stale > 5 s / garbled = normal badges, QUEUE_METER status line); settings Queue meter / Reach (ATR) / colours (parameter version 9) //   // 2.19.0 (2026-10-21 22:30 CT, Rassul approved Option B + 22:52 "show both hod and lod badges tomorrow on all markets"): the level % badges - next to every level badge a block "33%  /  66%" = how often the 1st / the final LOD (level below the RTH open) or HOD (above) came within 0.25 ADR of it, green outline = beats a random price the same distance from the open, grey = no better; from lsFlexLevels\LRA-LevelBadges-<MKT>.csv (nightly lra.hodlod_levels 1.2.0, per market; missing / garbled = no block, LEVEL_PCT status line); the block alone has a hover (DP_PCT_HOVERS); setting "Level % badges" (parameter version 8); Levels column + 92 px (ColumnBoard 1.3.0) // 2.18.2 (2026-10-19 18:51 CT, Rassul) VAH / VAL red, DPoc cyan, WPoc beige // 2.18.1 (2026-10-19 17:45 CT, live check: DPoc 7703 = his ProfPrevDay to the tick; status showed 3,679 bars with no volume-at-price re-read on every recompute) a bar IRT has no rows for is retried every 10 min, not every new bar // 2.18.0 (2026-10-19 13:14-13:35 CT, Rassul "i want an open live also, make its badge and line white .. prior day poc and prior week poc (Wpoc) .. VAH and VAL"; "RTH open"; "prior day poc is full session"; "both use volume"): new key levels from the chart - Open (today's RTH open, white), DPoc / VAH / VAL (the prior full session's volume profile, 70% value area, teal), WPoc (the prior week's volume POC, blue), from IRT's own volume-at-price (barVolumeProfile) summed per session; status PROFILE line // 2.17.3 (2026-10-18 23:37 CT, Rassul "get rid of the hover over on the gex levels and other places"): every hover OFF (level badges CR0 / HVL / G1 ..., nodes, HVL tags, header) - DP_HOVERS = false, the code kept; no TRACK_MOUSE // 2.17.2 (2026-10-18 his decision list): Snapshot outline removed (dashed); Market / Clock offset / Indicators text hidden (kept in the file) // 2.17.1 (2026-10-18 settings audit): the window from DealerProfileSettings.h / SettingsHost.h; settings per MARKET in DealerProfile.settings-<MKT>.txt (the root-keyed DealerProfile.settings.txt reset at every roll; migrated once, left in place); the indicators row no longer cut off; drawing unchanged // 2.17.0 (2026-10-18): STALE by Fresh.h (one rule with HOD / LOD), a status file per chart (DealerProfile.status-<MKT>-<spb>.txt, the old one kept one more version), Market State SRC GEX // 2.16.0 (2026-10-16, Rassul): node text "480 lots" (colour = buy / sell); level badge NAME | price | GAMMA | ratio (yellow) // 2.15.1: node / hedge-text cut-offs vs the whole grid's biggest strike (not the screen's) // 2.15.0: compact hovers (level / node / HVL / header), dynamic hover width, hover clear of the BIG column // 2.14.1: Levels column >= 150 px (NQ prices) //   // 2.14.0 (2026-10-15 10:34, Rassul): level badges NAME | price | liquidity, Levels column >= 124 px //   // 2.13.0 (2026-10-15 09:53, Rassul): level lines only in the Levels column, not across the chart //   // 2.12.1 (2026-10-15): the chart margin also leaves room for the BIG column (ColumnBoard 1.1.0) //   // 2.12.0 (2026-10-14, Rassul approved the external-audit display list at 18:25 CT, section 5): the bar hover "Teal = MenthorQ model shows + net gamma here (vendor model, data N min old). Not a prediction it holds."; MenthorQ badge hover "MenthorQ model level, about 6 to 8 min delayed. Watch price and tape here; it isn't a turn forecast."; touch odds "A chance to REACH only - not hold or reverse."; "How to use: wait for your trigger - a sweep through the exact level, then RB or CoB, before 11:00 AM CT."; HL badge "HL?" + "whether they hold is unknown"; today's pick marked ? in its hover; HVL regime text behind the hvl_cushion gate. The node B / S text and "dealers must BUY N" stay (his call).   // 2.11.1 (MQF100, 2026-10-14 after-close - "STALE 2h - 9:35 AM" with no reason): when the grid is STALE the header adds WHY and what is being done ("no internet - retrying", "signed out - sign in", "tab asleep - retrying") from the bridge's lsFlexLevels\LRA-MQFeed.txt (bridge 1.30.0, read at most every 5 s, used only while written in the last 5 min); the header hover says why / what is being done / what you do   // 2.11.0 (CR5200, Rassul 2026-10-14 11:00 CT): the Levels / Delta / Dealer columns right of price, the chart margin, one-sided teal / orange nodes with B / S hedge text, compact level badges (detail in the hovers)   // 2.10.0 (Rassul 2026-10-14): HrHI / HrLO no longer drawn; AsiaHI / AsiaLO (18:00-02:00 CT, muted purple) and CWH / CWL (this week's high / low set before yesterday, soft blue) added; a new level within the market's merge ticks joins the line ("ONH . AsiaHI")   // 2.9.0: absolute-GEX nodes, W / F, top-3 %, hedge numbers, HVL tags, STALE header, CR0 merge, G2-G10 hidden, hovers   // 2.8.7: audit defect fixes - external files read bounded (a torn / oversized / malformed file keeps the last good one), level matching on the tick grid (no adjacent-tick merge), settings / status written atomically   // 2.8.6: badges always right of the Liquidity Profile column (its published width); a narrow chart gives up right-side room instead   // 2.8.5: a level's Liq tag = the zone holding its price (else the nearest), as the Liquidity column names it   // 2.8.4: the badge column half-way between the left (past the Liquidity column) and the centre   // 2.8.3: Liq tags for every zone the Liquidity Profile draws (1.7x+; it publishes only its picks)   // 2.8.2: badges in one centred column (price bars no longer push them aside), a badge with no room on its row steps up / down with a tick to its line   // 2.8.1: badges keep their place (no left / centre / left), Liq tags held 30 s (no blinking)   // 2.8.0: on an hourly (15-min+) chart the levels only, from the 3-min chart (LRA-KeyLevels-<MKT>.csv); every level badge (key + gamma) shows the liquidity resting on it, "Liq 977 (2.6x)" (green bids / red offers, from the Liquidity Profile); DEALER DATA STALE only past 25 min   // 2.7.4: badges "<price> <NAME>  <odds>", 10 pt bold names, never over price / labels / profiles; partial-history levels dropped; roll / Rithmic contract match; no file I/O per repaint   // 2.7.3: key-level labels centred like the MenthorQ labels, stepping beside a label on the same row, never over the Delta / Dealer Profiles (Rassul 2026-10-11 19:41)   // 2.7.2: the native key-level cache is per chart (host state), not shared by every chart on the DLL - RA1009 regression audit   // 2.7.1: key levels in his SessionPrices colours (PF teal, PD orange, PW blue, ON grey), a bigger font, odds as "33%  /  50%"   // 2.7.0: every key level (PFH/PFL, PDH/PDL, PWH/PWL, ONH/ONL, LonHI/LonLO, HrHI/HrLO) computed natively from the chart and drawn with its odds   // 2.6.3: key-level odds labels only, no extra amber / purple lines   // 2.6.2: no VWAP lines from the key levels (Session VWAP draws them)   // 2.6.1: overnight odds from the nightly champion (OnTouchParams.h), "?" gone when proven   // 2.6.0: outside RTH (no fresh LRA odds) each MenthorQ level shows its 60-min touch odds computed here from the chart's last 20 closes, "1h 45%?"   // 2.5.2: MenthorQ nodes and levels drawn exactly at their strikes on the same contract (no snapshot-noise shift)   // 2.5.1: per-host state, robust external-input handling, atomic bar export. 2.5.0 (2026-10-08): every level's touch odds "55%/70%" (within 90 min / by the RTH close, one law - lra.level_touch): key levels + hourly swings drawn with their odds, MenthorQ labels from the same file, a white box = a pick   // 2.4.1: the odds text from the model ("1h 50%  Exp 50%  CL 50%", nearest first)   // 2.4.0 (2026-10-07): touch odds on each MenthorQ level (1h / by expiry / by close)   // 2.3.0 (2026-10-07): the MenthorQ key levels (FlexLevels replaced)   // 2.2.3 (20:29): width default 50 (all profiles fit on the right). 2.2.1 (2026-10-05 17:55): Profile width default 80 px. 2.2.0 (2026-10-05): per-chart settings + per-market bar export (one DLL object serves every chart)
static const bool DP_PCT_HOVERS = true;   // (2.19.0) only the level % block has a hover (approved Option B); every other hover stays off
static const int  DP_PCT_W = 92;           // (2.19.0) px the % block adds to the Levels column ("100%  /  100%" bold 9 pt + pads)
static const bool DP_LINES = true;    // (2.29.0, Rassul 14:35-14:58 CT) LEVEL LINES across the chart, one badge centred on each line
                                       // (chips, price, odds, liquidity rank at the end), the Levels column becomes the LIQUIDITY column
static const bool DP_GAMMA = false;   // (2.29.0, Rassul 14:40 "remove the gamma levels for now") MenthorQ levels + GEX rows not drawn
static const int  DP_LQ_W = 170;      // (2.29.0) the liquidity column width
static const bool DP_LVC = true;      // (2.28.0) the combined Levels + Liquidity column (false = the 2.27.0 badges)
static const bool DP_BARS = false;     // (2.28.0, Rassul 13:38 "lets just get rid of it") the Dealer Profile bars are not drawn (true brings the column back)
static const int  DP_LVC_W = 420;      // (2.28.0) the column: rank 18 + liquidity 96 + chips + price + % block (ColumnBoard 1.4.0 Levels up to 460)
static const bool DP_HOVERS = false;   // (2.17.3, Rassul 23:37 CT) the hovers are off; true brings them back (the code is unchanged)
//   // 2.1.0 (2026-10-03): SPX / QQQ book tag (file 2.0 SRC row)
//   // 2.0.3 (2026-10-03): number boxes get an explicit width (NUMW) - 0 is the SDK default and still showed "T"
//   // 2.0.2 (2026-10-03, Rassul: "ng looks wierd", "euro also looks strange", font / clock show T): bars capped in height (NG / EU strikes are 100-300 px apart when zoomed in - each bar was a block), values on every visible bar, CL / NG dimmed (options data context only), stale age by date, number fields at the default width
//   // 2.0.1 (2026-10-02 19:12): the how-to text in the settings is gone   // 2.0.0 (2026-10-02, Rassul: "there is no way it is supposed to look like this"): one bar per strike = MenthorQ's net GEX ($ per 1-pt move), red short / green long, today's 0DTE part darker, the value on the bar - as MenthorQ draws it; the old synthetic gamma + running BUY/SELL nodes only for files without net GEX   // 1.5.0: option D pills - the % pill takes its node's colour, filled by size (HEAVY solid / BIG half / MODERATE faint / LIGHT outline), as the Reader's size chips   // 1.4.0: KEY / TGT / MAG tags, REACH in the status (mockup v26)   // 1.3.11: settings guide = (open) lists, no checkbox rows   // 1.3.10: whole-book banner off by default (Rassul 2026-09-30: "i dont think i need the top part")
//   // 1.3.4: cyan wall / lime fuel. 1.3.5: "4220  10  (9 to 17)", no whisker; delta keeps BUY / SELL
static const COLOR C_WALL2 = 0x0022D3EE;   // (2.9.0, approved mockup) WALL = cyan
static const COLOR C_FUEL2 = 0x00A3E635;   // (2.9.0, approved mockup) FUEL = lime
static const COLOR C_WALL  = 0x00E3C341;   // (1.3.9, Rassul 2026-09-30: "yellow and purple like Skylit") long gamma = YELLOW //   // long gamma: CYAN (1.3.4, Rassul 2026-09-30: "Cyan (wall) and Lime (fuel)")
static const COLOR C_FUEL  = 0x00AB47BC;   // short gamma = PURPLE (Skylit) //   // short gamma: LIME (delta keeps green buy / red sell)
static const COLOR C_BUY   = 0x0022C55E;   // dealers buy
static const COLOR C_SELL  = 0x00EF4444;   // dealers sell
static const COLOR C_WHISK = 0x00F8FAFC;   // arrival range
static const COLOR C_SNAP  = 0x00E5E7EB;   // snapshot outline
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_DARK  = 0x000B0F19;   // fills and text ON a coloured node only (never a line)
static const COLOR C_BANNERBG = 0x001C1305;

static COLOR fade(COLOR a, float t)   // toward the dark ground
{
    int ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    int br = (C_DARK >> 16) & 0xFF, bg = (C_DARK >> 8) & 0xFF, bb = C_DARK & 0xFF;
    ar = (int)(ar + (br - ar) * t); ag = (int)(ag + (bg - ag) * t); ab = (int)(ab + (bb - ab) * t);
    return (COLOR)(((COLOR)ar << 16) | ((COLOR)ag << 8) | (COLOR)ab);
}

struct PIdx { int market, width, labels, banner, legend, snap, whisk, fadefar, font, clock, keyPill, keyNode, keyHow, inds, levelsW, badgeFont, liqBadge, levelPct; };
static PIdx PX;
// (2.11.0) width = the DEALER column (40-250, default 80), levelsW = the LEVELS column (30-150, 40), badgeFont = the level badges (9 pt),
// labels = "Show buy / sell text in node" (the B / S hedge text inside each node); (11:39 CT) levelsW default 78 and liqBadge =
// "Liquidity in badge" (the resting contracts at each level beside its name, "HL 900" on a green / red block at the market's 2.5x)
struct Settings { int market = 0, width = 80, font = 10, clock = 0, levelsW = 78, badgeFont = 9; bool liqBadge = true, levelPct = true, labels = true, banner = false, legend = true, snap = true, whisk = true, fadefar = true;
                  bool qm = true; double qmReach = 1.0; COLOR qmBid = 0x0022C55E, qmAsk = 0x00EF4444, qmWarn = 0x00FACC15;   // (2.20.0) the queue meter
                  std::string inds = "cob_bull,cob_bear"; };   // (1.5.0) his custom indicators copied into the bar file

static bool dpFinite(float v) { return dpaudit::finite(v); }
static bool dpFinite(double v) { return dpaudit::finite(v); }

// DealerLogic is also consumed by DealerRead, so keep this Profile-specific boundary check here
// until the shared-header owner can make parsing validation consistent for every consumer.
static void dpSanitizeDealerData(dl::Data& d)
{
    if (!dpFinite(d.asofSo)) d.asofSo = -1;
    if (!dpFinite(d.srcRatio)) d.srcRatio = 1.0f;
    if (!dpFinite(d.book)) { d.book = 0; d.hasBook = false; }
    if (!dpFinite(d.px) || !dpFinite(d.em) || d.px <= 0 || d.em <= 0) d.hasPrice = false;
    std::vector<dl::Node> clean;
    clean.reserve(d.nodes.size());
    for (size_t i = 0; i < d.nodes.size(); ++i) {
        const dl::Node& n = d.nodes[i];
        if (dpFinite(n.k) && dpFinite(n.g) && dpFinite(n.lo) && dpFinite(n.hi) && dpFinite(n.snap) && dpFinite(n.d) &&
            dpFinite(n.gp) && dpFinite(n.dp) && dpFinite(n.usd) && dpFinite(n.usd0)) clean.push_back(n);
    }
    d.nodes.swap(clean);
    std::vector<dl::Data::Tag> cleanTags;
    cleanTags.reserve(d.tags.size());
    for (size_t i = 0; i < d.tags.size(); ++i) if (dpFinite(d.tags[i].k)) cleanTags.push_back(d.tags[i]);
    d.tags.swap(cleanTags);
}

// std::rename does not replace an existing target on MSVC. MoveFileEx preserves the last
// complete bar export if a reader has the target open or replacement otherwise fails.
static bool dpReplaceFile(const std::string& tmp, const std::string& path)
{
#ifdef _WIN32
    return MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(tmp.c_str(), path.c_str()) == 0;
#endif
}

static void dpNormalizeSettings(Settings& s)
{
    if (s.market < 0 || s.market > 7) s.market = 0;
    s.width = cb::cleanWidth(cb::DEALER, s.width);              // (2.11.0) the Dealer column: 40-250, else the default 80
    s.levelsW = cb::cleanWidth(cb::LEVELS, s.levelsW);          // (2.11.0) the Levels column: 30-150, else 40
    s.badgeFont = dcol::cleanBadgeFont(s.badgeFont);
    if (s.font < 10) s.font = 10;               // (2.9.0) every text 10 pt or larger
    if (s.font > 24) s.font = 24;
    if (s.clock < -720 || s.clock > 720) s.clock = 0;
    if (s.inds.size() > 160) s.inds.resize(160);
}

// Unknown future rows remain forward-compatible, but a malformed row in the
// declared Dealer grammar rejects the whole changed snapshot rather than
// replacing the prior complete display with a torn prefix.
static bool dpKnownDealerRow(const std::string& line)
{
    size_t p = line.find('|'); std::string k = line.substr(0, p);
    return k == "VERSION" || k == "MARKET" || k == "SYMBOL" || k == "SRC" || k == "ASOF" || k == "LIVE" || k == "PRICE" || k == "BOOK" ||
           k == "NODE" || k == "LEVEL" || k == "WALLHDR" || k == "FUELHDR" || k == "NAMES" || k == "NOWALL" || k == "XROW" || k == "ANOW" ||
           k == "ABIAS" || k == "ASCN" || k == "WALLROW" || k == "FUELROW" || k == "TRADE" || k == "FLAG" || k == "DO" || k == "TRIG" ||
           k == "BOOKLINE" || k == "NEXT" || k == "ADEAL" || k == "AMAG" || k == "KEYLVL" || k == "CHK" || k == "VERDICT" || k == "STOPTGT" ||
           k == "TAG" || k == "LEARN" || k == "VERDICT2" || k == "VOTE" || k == "DLINE" || k == "PLAN" || k == "BOOKCHK" || k == "SUMMARY" ||
           k == "FSUM" || k == "FWIN" || k == "GSTR" || k == "SIG" || k == "LSIG" || k == "FERR" || k == "TURN" || k == "RD" || k == "RMODE" ||
           k == "TR" || k == "TERR" || k == "STAGE" || k == "STG" || k == "SSUM" || k == "SR";
}

using dpaudit::LblBox;                 // (2.7.3) a drawn label box (2.7.4: DealerProfileAuditLogic.h, tested)
class DealerProfile : public cppExtension {
public:
    SS_SAFE_DIALOG(DealerProfile, "DealerProfile")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    DealerProfile();
    virtual int parmsLoadBody(void);
    virtual int parmsApplyBody(void);
    virtual int parmsUpdtBody(unsigned int iParmNumber);
    virtual int done(void);
    virtual int destroy(void);
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int calc(int iStartBar);   // (2.11.0) the chart margin is set here / in preCalc (IRT accepts it in pre-calc and Apply calls)
    virtual int preCalc(void);

    Settings cfg;
    // (2.11.0) the right-side columns of THIS chart (kept in the host state like every other cache)
    struct ColsState {
        cb::Peers peers; cb::UnitProbe probe; cb::MarginCtl mctl; cb::Layout lay;
        std::string sym, chart; long spb = 0; bool published = false;
        int needPx = 0, ppb = 0, marginRead = -1, marginAsked = -1, setOk = 0, setFail = 0, gapSeen = -1;
        int badgesShown = 0, badgesDropped = 0, nodeTexts = 0, nodeTextsHidden = 0, nodesTiny = 0;
        long long firstMs = -1;
        dcol::LiqLevels liqL; long long liqLStamp = -2, liqLChecked = -1; std::string liqLWhy;   // (11:39 CT) the per-level liquidity file
        lpct::Table lpT; long long lpStamp = -2, lpChecked = -1; std::string lpWhy = "not read yet", lpMkt; int lpShown = 0, lpGreen = 0, lpNoRow = 0, lpNoRoom = 0;   // (2.19.0) the level % file
        qm::File qmF; long long qmStamp = -2, qmChecked = -1, qmMtime = 0; std::string qmMkt; int qmOnBadge = 0, qmAlone = 0, qmFar = 0, qmInactive = 0, qmNoRoom = 0;   // (2.20.0) the queue meter file
        std::vector<lvc::Order> lmtO; long long lmtStamp = -2, lmtChecked = -1; std::string lmtWhy = "not read yet"; int lvcRows = 0, lvcLiq = 0, lvcMerged = 0, lvcGex = 0, lvcChipsCut = 0;   // (2.28.0)
        std::map<std::string, dcol::HLState> hl; int hlOn = 0, liqBlocks = 0;                   // HL hysteresis per level
        dpaudit::WriteGate gateOld, gateMkt;                                                     // (2.17.0) this chart's status writes (was one static for every chart)
        bool mstPosted = false; std::string mstSym, mstChart; long mstSpb = 0;                   // (2.17.0) the Market State entry of this chart
        long long mstBar = -1, mstMs = -1; std::string mstSig; std::map<std::string, std::string> mstKv; std::shared_ptr<sa::Hook> self;
    };
    ColsState cs;
    ss::Snap dsnap_;                                                    // (2.11.0) the dialog's values when it opened (2.17.1: SettingsSpec.h)
    std::vector<std::pair<std::string, ss::Store> > stores_;            // (2.17.1) per market (files keyed by market: safe across charts)
    ss::Store& settingsStore(const std::string& mkt);
    void columns(const RCT& pane, short axisR, bool hasDealer);         // this draw's layout + the board
    void applyMargin();                                                 // calc / preCalc / Apply: the right margin
    void unpublishCols();
    long long nowMsC();
    // (2.9.0) the file 2.4 rows the Profile alone reads, the hover state and this draw's hover targets
    std::vector<dpaudit::NodeX> nx; dpaudit::BookAsOf bookAt;
    struct HoverT { short l = 0, t = 0, r = 0, b = 0; std::string key; dpaudit::Hover h; bool pct = false, qm = false; };   // (2.20.0) qm = a queue meter badge (its hover stays on)   // (2.19.0) pct = the level % block (its hover stays on)
    std::vector<HoverT> hovT;                       // this draw's targets (kept in the host state for mouse())
    bool msHas = false; long msBar = -1; float msPrice = 0; short msH = 0, msV = 0; std::string hovKey;
    std::vector<void*> hovWnds;
    bool hvlFound = false; float hvlChartPx = 0;    // this draw's HVL (from the MenthorQ levels) for the regime tags
    std::vector<double> keyPx;                      // this draw's key-level prices (chart scale): G2-G10 show only on one
    bool hitTarget(const HoverT& T) ;
    const HoverT* hovered();
    void drawHover(const HoverT& T, const Settings& S);
    void header(const Settings& S, bool stale, double age, short anchor, int colW, const char* note);
    void renderNodeX(const Settings& S, const RCT& pane, short anchor, int hs, bool stale, double ageMin = -1);
    // (2.7.4) this draw's level badges: every level LINE is drawn first, then the badges (MenthorQ first, then key levels), so no
    // line crosses a badge; each badge keeps clear of the price bars, the other badges and the profile columns
    struct LSeg { std::string s; bool bold; COLOR c; };
    struct PendLbl { short y; COLOR border; std::vector<LSeg> seg; std::string key;   // (2.8.1) key = the level, for its sticky x
                     std::string name0, priceTxt, odds, liq; COLOR col = 0; bool pick = false, above = false, stale = false, hvl = false;   // (2.11.0) the badge + its hover
                     bool haveLiq = false, bid = false, hl = false; long long lots = 0; double bx = -1; double px = NAN; int src = 0; };   // (2.28.0) src 0 = key / volume level, 1 = MenthorQ, 2 = big unnamed gamma strike   // (2.19.0) px = the level price (chart scale) for its % side   // (11:39 CT) the liquidity block
    void loadLiqLevels();
    void loadLevelPct();                         // (2.19.0) lsFlexLevels\LRA-LevelBadges-<MKT>.csv, stamp-checked at most once a second
    void loadQueue();                            // (2.20.0) lsFlexLevels\LRA-QueueMeter-<MKT>.csv, stamp-checked at most every 500 ms
    const qm::FileT* queueAt(double chartPx, const Settings& S, bool& used);   // the live test whose zone holds this price (within the reach)
    bool drawMeter(const qm::FileT& T, const std::string& name, short l, short r, int t, int b, int yc, const Settings& S, const std::string& key, const std::string& pxTxt);
    bool levelLiq(PendLbl& P, double chartPx);   // fills the block; true = HL (the level line is drawn 3 px)
    std::vector<PendLbl> pend;
    void flushLabels(const Settings& S);
    void flushColumn(const Settings& S);
    void flushLines(const Settings& S);         // (2.29.0) level lines + centred badges + the liquidity column        // (2.28.0) the combined Levels + Liquidity column
    void loadLmt();                              // (2.28.0) lsFlexLevels\LRA-LmtLevels-<MKT>-<spb>.txt (lsLiquidityProfile 0.60.0)
    void addGexRows();                           // (2.28.0) the 3 biggest unnamed gamma strikes
    double atr14();                              // (2.28.0) the chart's 14-candle ATR (the row merge tolerance)
    struct OffLvl { bool have = false; std::string name; double px = 0, dist = 0; };
    OffLvl offUp, offDn; double lastPxLv = 0;  // (2.28.0) the nearest level above / below the screen, this draw
    short rightEdge(const RCT& pane);
    dl::Data D;
    std::string mkt, root;
    float off;
    int lastBar;
    long long loadedStamp = -2; std::string loadedPath;   // (1.3.7) re-read the file only when it changed
    struct Exp { long n = 0; float c = 0; time_t t = 0; };
    std::map<std::string, Exp> expBy;                        // (2.2.0) the chart-bar export throttle, per market
    void exportBars();

    bool dialogReady();
    void syncSettings(bool fromDialog = false);
    // (2.2.7, Rassul 23:33-23:34 "KEY and MAGNET cross over into the delta profile column ... keep them in the dealer profile
    // column") the KEY / TGT / MAG chips stay, drawn INSIDE the 70 px the Profile reserves left of its bars (REACH), never past it.
    // KEY carries the strike's 1-hour GEX change (KEY +18%).
    void drawKeyPct(float k, float step, short anchor, short y, const Settings& S)
    {
        if (true) return;   // (2.11.0) the chips sat left of the Dealer column (in the Delta column); the strike's tags are in its hover
        for (size_t j = 0; j < D.tags.size(); j++) {
            const dl::Data::Tag& T = D.tags[j];
            if (std::fabs(T.k - k) > step * 0.5f) continue;
            int fsz = S.font - 1; if (fsz < 7) fsz = 7;
            std::string txt = T.text;
            int tw = textW(txt.c_str(), fsz, true) + 8;
            while (tw > 62 && txt.size() > 3) { txt = txt.substr(0, txt.size() - 1); tw = textW(txt.c_str(), fsz, true) + 8; }   // never wider than the reserve
            COLOR bg = T.col == 'A' ? 0x00C084FC : T.col == 'B' ? C_WALL : T.col == 'G' ? C_BUY : T.col == 'C' ? 0x0022D3EE : C_INK;
            short ch = (short)(fsz + 5);
            short x2 = (short)(anchor - S.width - 36);                  // (2.2.8) clear of the bar values (e.g. 5.4M) left of a full bar
            box((short)(x2 - tw), (short)(y - ch / 2), x2, (short)(y + ch / 2), bg);
            textLJ((short)(x2 - tw + 4), y, txt.c_str(), C_DARK, fsz, true);
        }
    }
    std::string chartKey(); std::string savedKey; long long savedStamp = -2;
    void fillDialog();
    void load();
    void alignContract();
    void render(const Settings& S);
    void staleBadge();
    fr::Verdict gridVerdict(int clockOff, double& ageMin);           // (2.17.0) the grid's age + its verdict (Fresh.h)
    void postMst();                                                  // (2.17.0) the Market State, SRC GEX
    void clearMst();
    void srcTag();
    void writeStatus(const char* what);
    short yOf(float price);
    void box(short l, short t, short r, short b, COLOR c);
    void dashRect(short l, short t, short r, short b, COLOR c);
    void line(short x1, short y1, short x2, short y2, COLOR c, int w);
    void textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold);
    void textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold);
    int  textW(const char* s, int sz, bool bold);
    void strengthMark(short rightX, short y, float pct, bool live, int font, COLOR base);
    void outline(short l, short t, short r, short b);
    // (2.3.0, Rassul 2026-10-06 21:02 "irt ... pauses every few minutes ... i think its because of flexlevels ... replace with
    // dealerprofile giving the levels"; 2026-10-07 the bridge log: FlexLevels' Remote File check hits the bridge every 61-86 s,
    // HEAD then GET, up to 11 s apart - his "slightly over a minute" pause) the MenthorQ key levels drawn HERE from the local
    // MenthorQLevels.csv the bridge already writes: no HTTP, re-read only when the file changes. Remove lsFlexLevels from the charts.
    struct MQL { float px = 0; std::string label; COLOR col = 0; int w = 1; };
    std::map<std::string, std::vector<MQL> > mql; long long mqlStamp = -2; time_t mqlChecked = 0;
    // (2.4.0) LRA-Touch-<MKT>.csv: TOUCH|code|price|P 1h|P by expiry|P by close|proven (lra.touch_prob, every 5 min)
    struct TP { float px = 0; std::string p1, pe, pc, txt; bool proven = false; };
    std::map<std::string, std::vector<TP> > tps; std::map<std::string, long long> tpStamp; std::map<std::string, time_t> tpChecked, tpAt;
    void loadTouch(const std::string& m);
    // (2.8.0) the Liquidity Profile's zones near price (LRA-LiqZones-<MKT>.csv) -> "Liq 977 (2.6x)" on every level badge
    std::vector<dpaudit::LiqZ> liqZ; long long liqStamp = -2; time_t liqChecked = 0; std::string liqWhy;
    std::map<long long, bool> liqShown;            // level (in 1/10000s of a point) -> it showed a tag last draw (hysteresis)
    struct LiqMem { dpaudit::LiqZ z; time_t seen = 0; };
    std::map<long long, LiqMem> liqMem;            // (2.8.1) the last tag per level, held 30 s through a dip / a refresh gap
    time_t liqGoodAt = 0;                          // (2.8.1) the last good read of the zones file
    int liqColRight = 0;                           // (2.8.6) the Liquidity Profile column's width on this market's chart (px)
    std::map<std::string, short> lblX;             // (2.8.1) each badge's x last draw (sticky placement)
    void loadLiq(const std::string& m);
    void addLiq(PendLbl& P, double chartPx);
    void loadMQLevels();
    void drawMQLevels(const Settings& S);
    // (2.5.0, Rassul 2026-10-08 07:16 "my expectation now is for the levels to [have] probabilities in their labels") LRA-Levels-<MKT>.csv
    // (lra.level_touch, every 5 min): LEVEL|code|name|price|family|P15|P30|P45|P60|P90|Pclose|validated|label|Pexp, PICK|code|price|window|P,
    // STATE|STALE|why. Prices are MenthorQ's (the chart draws them at price + off). Stale (> 20 min or STATE|STALE) = nothing shown.
    struct LV { float px = 0; std::string code, show, fam, label; bool pick = false; };
    std::map<std::string, std::vector<LV> > lvs; std::map<std::string, long long> lvStamp; std::map<std::string, time_t> lvChecked, lvAt;
    std::map<std::string, bool> lvStale;
    void loadLevels(const std::string& m);
    dpaudit::FeedStat feed; long long feedStamp = -2; time_t feedChecked = 0;   // (2.11.1) the bridge's feed diagnosis (one file for every chart)
    void loadFeed();
    const LV* levelAt(const std::string& m, float chartPx);
    double chartTick(const std::string& m);
    void drawKeyLevels(const Settings& S);
    void levelSpan(const RCT& pane, short& x0, short& x1);   // (2.13.0)
    // (2.7.0) the native key levels, cached per market: recomputed when the chart gets a new bar (at most every 10 s)
    // (2.7.2) the cache is per chart: copied in/out of the host State like every other cache (loadHostState / saveHostState)
    // (2.18.0) + the volume-at-price of every closed bar (the DPoc / VAH / VAL / WPoc profiles), held by a shared pointer so the
    // per-draw host-state copy stays cheap; diag = what the profiles were built from (status file PROFILE line)
    struct VapCache { std::map<long long, std::vector<klv::Vap>> bars; std::map<long long, time_t> bad; };   // (2.18.1) bad = a bar IRT had no rows for: retried every 10 min, not every recompute
    struct KLC { long n = -1; long long lastT = 0; time_t at = 0; std::vector<klv::Merged> v; std::shared_ptr<VapCache> vap; klv::ProfDiag diag; int vapRead = 0, vapMissing = 0; };
    std::map<std::string, KLC> klc;
    const std::vector<klv::Merged>& nativeLevels(const std::string& m);
    std::string profileLine();   // (2.18.0) the PROFILE status line

    // cppExtension can be shared by host charts. These fields are the complete mutable
    // chart state, kept in the SDK's current-host storage rather than on the shared object.
    struct State {
        Settings cfg;
        dl::Data D;
        std::string mkt, root;
        float off = 0.0f;
        int lastBar = 0;
        long long loadedStamp = -2;
        std::string loadedPath;
        std::map<std::string, Exp> expBy;
        std::string savedKey;
        long long savedStamp = -2;
        std::map<std::string, std::vector<MQL> > mql;
        long long mqlStamp = -2;
        time_t mqlChecked = 0;
        std::map<std::string, std::vector<TP> > tps;
        std::map<std::string, long long> tpStamp;
        std::map<std::string, time_t> tpChecked, tpAt;
        std::map<std::string, std::vector<LV> > lvs;
        std::map<std::string, long long> lvStamp;
        std::map<std::string, time_t> lvChecked, lvAt;
        std::map<std::string, bool> lvStale;
        std::map<std::string, KLC> klc;   // (2.7.2, RA1009 regression audit) the native key-level cache is THIS chart's (was on the shared DLL object)
        std::vector<dpaudit::LiqZ> liqZ; long long liqStamp = -2; time_t liqChecked = 0; std::map<long long, bool> liqShown;   // (2.8.0)
        std::map<long long, LiqMem> liqMem; time_t liqGoodAt = 0; std::map<std::string, short> lblX; int liqColRight = 0;     // (2.8.1)
        std::vector<dpaudit::NodeX> nx; dpaudit::BookAsOf bookAt;                                                          // (2.9.0)
        std::vector<HoverT> hovT; bool msHas = false; long msBar = -1; float msPrice = 0; short msH = 0, msV = 0; std::string hovKey;
        std::vector<void*> hovWnds;
        ColsState cs;                                                                                                      // (2.11.0)
    };
    bool loadHostState();
    HostSlot<State> slot_;
    void saveHostState();
    void clearWorkingState();
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

DealerProfile::DealerProfile() : cppExtension() { off = 0.0f; lastBar = 0; }

void DealerProfile::clearWorkingState()
{
    cfg = Settings(); D = dl::Data(); mkt.clear(); root.clear(); off = 0.0f; lastBar = 0;
    loadedStamp = -2; loadedPath.clear(); expBy.clear(); savedKey.clear(); savedStamp = -2;
    mql.clear(); mqlStamp = -2; mqlChecked = 0; tps.clear(); tpStamp.clear(); tpChecked.clear(); tpAt.clear();
    lvs.clear(); lvStamp.clear(); lvChecked.clear(); lvAt.clear(); lvStale.clear(); klc.clear();
    liqZ.clear(); liqStamp = -2; liqChecked = 0; liqShown.clear(); liqMem.clear(); liqGoodAt = 0; lblX.clear(); liqColRight = 0;
    nx.clear(); bookAt = dpaudit::BookAsOf(); hovT.clear(); msHas = false; msBar = -1; msPrice = 0; msH = msV = 0; hovKey.clear(); hovWnds.clear();
    hvlFound = false; hvlChartPx = 0; keyPx.clear(); cs = ColsState();
}

bool DealerProfile::loadHostState()
{
    State* s = slot_.get(this, true);
    if (!s) { clearWorkingState(); return false; }
    cfg = s->cfg; D = s->D; mkt = s->mkt; root = s->root; off = s->off; lastBar = s->lastBar;
    loadedStamp = s->loadedStamp; loadedPath = s->loadedPath; expBy = s->expBy; savedKey = s->savedKey; savedStamp = s->savedStamp;
    mql = s->mql; mqlStamp = s->mqlStamp; mqlChecked = s->mqlChecked; tps = s->tps; tpStamp = s->tpStamp;
    tpChecked = s->tpChecked; tpAt = s->tpAt; lvs = s->lvs; lvStamp = s->lvStamp; lvChecked = s->lvChecked;
    lvAt = s->lvAt; lvStale = s->lvStale; klc = s->klc;
    liqZ = s->liqZ; liqStamp = s->liqStamp; liqChecked = s->liqChecked; liqShown = s->liqShown;
    liqMem = s->liqMem; liqGoodAt = s->liqGoodAt; lblX = s->lblX; liqColRight = s->liqColRight;
    nx = s->nx; bookAt = s->bookAt; hovT = s->hovT; msHas = s->msHas; msBar = s->msBar; msPrice = s->msPrice; msH = s->msH; msV = s->msV;
    hovKey = s->hovKey; hovWnds = s->hovWnds; cs = s->cs;
    return true;
}

void DealerProfile::saveHostState()
{
    State* s = slot_.get(this, false);
    if (!s) return;
    s->cfg = cfg; s->D = D; s->mkt = mkt; s->root = root; s->off = off; s->lastBar = lastBar;
    s->loadedStamp = loadedStamp; s->loadedPath = loadedPath; s->expBy = expBy; s->savedKey = savedKey; s->savedStamp = savedStamp;
    s->mql = mql; s->mqlStamp = mqlStamp; s->mqlChecked = mqlChecked; s->tps = tps; s->tpStamp = tpStamp;
    s->tpChecked = tpChecked; s->tpAt = tpAt; s->lvs = lvs; s->lvStamp = lvStamp; s->lvChecked = lvChecked;
    s->lvAt = lvAt; s->lvStale = lvStale; s->klc = klc;
    s->liqZ = liqZ; s->liqStamp = liqStamp; s->liqChecked = liqChecked; s->liqShown = liqShown;
    s->liqMem = liqMem; s->liqGoodAt = liqGoodAt; s->lblX = lblX; s->liqColRight = liqColRight;
    s->nx = nx; s->bookAt = bookAt; s->hovT = hovT; s->msHas = msHas; s->msBar = msBar; s->msPrice = msPrice; s->msH = msH; s->msV = msV;
    s->hovKey = hovKey; s->hovWnds = hovWnds; s->cs = cs;
}

int DealerProfile::done(void)
{
    if (loadHostState()) { unpublishCols(); clearMst(); }   // (2.17.0) + the Market State entry   // (2.11.0) this chart's columns leave the board (symbol / period change, removal)
    slot_.release(this);
    clearWorkingState();
    return RTX_OK;
}

int DealerProfile::destroy(void)
{
    if (loadHostState()) { unpublishCols(); clearMst(); }
    slot_.release(this);
    clearWorkingState();
    return RTX_OK;
}

// (2.17.1, settings audit 2026-10-18) the settings come from DealerProfileSettings.h through SettingsHost.h: the window is filled / read
// by the layer, the chart reads ONLY lsFlexLevels\DealerProfile.settings-<MKT>.txt (keyed, per MARKET; the old DealerProfile.settings.txt
// was keyed by the contract root, so every roll reset the settings - it is migrated once and left in place for older readers).
static ss::Ids& dpIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
static std::string dpFlexDir() { const char* up = getenv("USERPROFILE"); return std::string(up ? up : "C:") + "\\InvestorRT\\rtx\\lsFlexLevels\\"; }
static std::string dpSettingsPath() { return dpFlexDir() + "DealerProfile.settings.txt"; }
static long long dpNowMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static Settings dpFromValues(const ss::Values& V)
{
    using namespace dprset;
    Settings c; c.market = V.i(K_MARKET); c.width = V.i(K_WIDTH); c.levelsW = V.i(K_LEVELS_W); c.badgeFont = V.i(K_BADGE_FONT);
    c.liqBadge = V.b(K_LIQ_BADGE); c.levelPct = V.b(K_LEVEL_PCT); c.font = V.i(K_FONT); c.clock = V.i(K_CLOCK); c.labels = V.b(K_LABELS); c.snap = false;   // (2.17.2) Snapshot outline removed (dashed)
    c.fadefar = V.b(K_FADEFAR); c.banner = V.b(K_BANNER); c.inds = V.s(K_INDS); c.whisk = false; c.legend = false;
    if (c.inds.empty()) c.inds = "cob_bull,cob_bear";   // blank = the default (as before); "none" = off
    c.qm = false;   // (2.27.0, Rassul 2026-10-27 00:43 CT "i dont see the queue helping at all, remove it but you can continue logging it") the meter is never drawn (setting rows hidden); the record is written by lsLiquidityProfile as before
    (void)V.b(K_QUEUE_METER); c.qmReach = V.f(K_QM_REACH); c.qmBid = (COLOR)V.c(K_QM_BID); c.qmAsk = (COLOR)V.c(K_QM_ASK); c.qmWarn = (COLOR)V.c(K_QM_WARN);   // (2.20.0)
    if (!(c.qmReach >= 0.25 && c.qmReach <= 2.0)) c.qmReach = 1.0;
    dpNormalizeSettings(c); return c;
}
bool DealerProfile::dialogReady() { return ss::inDialog(this); }
std::string DealerProfile::chartKey()
{
    // the file follows the chart's own market (the Market list overrides which book is drawn, not where the settings live)
    char rb[32] = {0}; const char* rs = getRootSymbol(rb);
    const int m = ss::marketOfRoot(rs ? rs : "");
    return m >= 0 ? ss::MARKETS[m] : std::string();
}
ss::Store& DealerProfile::settingsStore(const std::string& mkt)
{
    for (size_t i = 0; i < stores_.size(); i++) if (stores_[i].first == mkt) return stores_[i].second;
    if (stores_.size() >= 8) stores_.erase(stores_.begin());
    ss::Store st; st.S = &dprset::spec();
    if (!mkt.empty()) { st.path = dpFlexDir() + "DealerProfile.settings-" + mkt + ".txt"; st.legacyPaths.push_back(dpSettingsPath()); st.legacy = dprset::legacy;
        char rb[32] = {0}; const char* rs = getRootSymbol(rb); st.hint = rs ? rs : ""; }   // the old file was keyed by the root: this chart's row first
    stores_.push_back(std::make_pair(mkt, st)); return stores_.back().second;
}
void DealerProfile::syncSettings(bool fromDialog)
{
    const std::string mkt = chartKey();
    if (fromDialog) {                                   // saved ONLY from the settings window's Apply / OK / changes
        if (!ss::inDialog(this) || mkt.empty()) return;
        ss::Store& st = settingsStore(mkt);
        const ss::Values V = ss::readDialog(this, dprset::spec(), dpIds(), st.load(ss::marketIndex(mkt), dpNowMs(), true), dsnap_);   // untouched -> the file's value; invalid never saved
        if (st.save(V)) dsnap_.take(ss::dialogNow(this, dprset::spec(), dpIds(), V));
        cfg = dpFromValues(V); savedKey = mkt;
        return;
    }
    if (mkt.empty()) { cfg = Settings(); savedKey.clear(); return; }
    cfg = dpFromValues(settingsStore(mkt).load(ss::marketIndex(mkt), dpNowMs()));   // re-read at most once a second, by content
    savedKey = mkt;
}
// (2.11.0) the dialog opens: fill every control from this market's saved values (or the defaults)
void DealerProfile::fillDialog()
{
    if (!ss::inDialog(this)) return;                    // the chart's copy: nothing to fill
    const std::string mkt = chartKey(); if (mkt.empty()) return;
    ss::fillDialog(this, dprset::spec(), dpIds(), settingsStore(mkt).load(ss::marketIndex(mkt), dpNowMs(), true), dsnap_);
}
int DealerProfile::parmsLoadBody(void)  { if (loadHostState()) { fillDialog(); saveHostState(); } return RTX_OK; }   // (2.11.0) opening the window never saves
int DealerProfile::parmsApplyBody(void) { if (loadHostState()) { syncSettings(true); applyMargin(); saveHostState(); } return RTX_OK; }
int DealerProfile::parmsUpdtBody(unsigned int i) { if (loadHostState()) { ss::onUpdate(this, dprset::spec(), dpIds(), (int)i, dsnap_); syncSettings(true); saveHostState(); } return RTX_OK; }

int cppExtension::setup(void)
{
    // (1.3.12, Rassul 2026-09-30) only real settings have a control; controls only, no how-to text.
    // (2.11.0) BUMPED to 6: the column rows (Dealer column / Levels column / Badge font / Show buy / sell text in node). IRT resets
    // the saved rows once; the per-market settings file keeps his values and fills the window when it opens.
    // (2.17.1, settings audit 2026-10-18) the same 12 rows in the same order from dprset::spec() (parameter version 6 kept); the
    // indicators row's label shortened (the row was ~444 px, cut off)
    ss::registerAll(this, dprset::spec(), dpIds());
    {   // PX keeps the row ids for the host tests that address the mocked controls by name (the plugin itself reads through the layer)
        using namespace dprset; const ss::Ids& I = dpIds(); if ((int)I.id.size() == K_N) {
        PX.market = I.id[K_MARKET]; PX.width = I.id[K_WIDTH]; PX.levelsW = I.id[K_LEVELS_W]; PX.badgeFont = I.id[K_BADGE_FONT]; PX.liqBadge = I.id[K_LIQ_BADGE]; PX.levelPct = I.id[K_LEVEL_PCT];
        PX.font = I.id[K_FONT]; PX.clock = I.id[K_CLOCK]; PX.labels = I.id[K_LABELS]; PX.snap = -1; PX.fadefar = I.id[K_FADEFAR];
        PX.banner = I.id[K_BANNER]; PX.inds = I.id[K_INDS]; }
        PX.keyHow = PX.keyNode = PX.keyPill = PX.whisk = PX.legend = -1; }
    return RTX_OK;
}


void DealerProfile::load()
{
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) { D = dl::Data(); nx.clear(); bookAt = dpaudit::BookAsOf(); loadedPath.clear(); return; }
    const char* up = getenv("USERPROFILE"); if (!up) { D = dl::Data(); nx.clear(); bookAt = dpaudit::BookAsOf(); return; }
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Dealer-" + mkt + ".csv";
    long long st = dl::fileStamp(path);
    if (st == loadedStamp && path == loadedPath) return;                   // unchanged, including a known-missing file: no repeat disk open
    if (st < 0) { D = dl::Data(); nx.clear(); bookAt = dpaudit::BookAsOf(); loadedStamp = st; loadedPath = path; return; } // cache a missing file rather than stat/opening each repaint
    std::vector<std::string> lines;
    if (dpaudit::readBoundedLines(path, lines) != dpaudit::READ_OK || dl::fileStamp(path) != st) return;
    dl::Data parsed;
    std::vector<dpaudit::NodeX> px_; dpaudit::BookAsOf ba_;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i];
        if (ln.compare(0, 6, "NODEX|") == 0) { dpaudit::NodeX x; if (!dpaudit::parseNodeX(ln, x)) return; px_.push_back(x); continue; }    // (2.9.0) file 2.4
        if (ln.compare(0, 9, "BOOKASOF|") == 0) { if (!dpaudit::parseBookAsOf(ln, ba_)) return; continue; }                                // (2.9.0) the grid's time
        if (!dl::parseLine(parsed, ln) && dpKnownDealerRow(ln)) return;   // (2.8.7) every row parsed; a malformed known row rejects the snapshot
    }
    if (parsed.ver.empty()) return;                              // declared file grammar starts with VERSION; do not install a torn prefix
    dpSanitizeDealerData(parsed);
    D = parsed; nx.swap(px_); bookAt = ba_;
    loadedStamp = st; loadedPath = path;
}

// the chart may be another contract than MenthorQ's front future: shift by (close at the file's minute - PRICE)
void DealerProfile::alignContract()
{
    off = 0.0f;
    long n = getBarCount(); if (n < 1 || !D.hasPrice) return;
    RTARRAY close(barClose);
    RTARRAYI dt(barDateTime);
    float c = close[(int)n - 1];
    if (D.asofSo >= 0) {
        double want = D.asofSo + cfg.clock * 60.0;
        int from = (int)n - 3000; if (from < 0) from = 0;
        for (int i = (int)n - 1; i >= from; i--) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
            double so = t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec;
            bool sameDay = (D.y == 0) || (t.tm_year + 1900 == D.y && t.tm_mon + 1 == D.mo && t.tm_mday == D.d);
            if (sameDay && so <= want) { c = close[i]; break; }
        }
    }
    // (2.5.2, Rassul 22:15 "irt gamma levels are not placed at the right price level") MenthorQ's strikes are prices on the
    // contract named in the file (SYMBOL, the bridge's irt_symbols.json): on THAT contract they are drawn exactly at the strike.
    // Only a chart on another contract (e.g. the next month during a roll) is shifted by the measured spread. A file without
    // SYMBOL uses the size rule in dl::offsetFor (a gap under a quarter of the expected move is snapshot noise, not a contract).
    if (!D.chartSym.empty() && dl::sameContract(root, D.chartSym)) return;                   // same contract (or continuous): off = 0
    float o = 0;
    if (dl::offsetFor(c, D.px, o, D.chartSym.empty() ? D.em : 0.0f)) off = o;
}

short DealerProfile::yOf(float price) { PNT p; p.set(lastBar, price); return p.v; }
void DealerProfile::box(short l, short t, short r, short b, COLOR c) { RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
void DealerProfile::line(short x1, short y1, short x2, short y2, COLOR c, int w)
{
    setPen(c, (short)w, P_SOLID);
    PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
}
void DealerProfile::dashRect(short l, short t, short r, short b, COLOR c)
{
    setPen(c, 1, P_SOLID);   // (1.3.2) operator: "the border of the node is dashed, make it solid"
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
int DealerProfile::textW(const char* s, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (int)getTextWidth(s, -1);
}
// the rect text draw baselines low (~0.45 x font below y, GammaProfile v0.73): shift up so it sits ON y
void DealerProfile::textRJ(short rightX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
    RCT rc; rc.set((short)(rightX - 400), (short)(yy - sz), rightX, (short)(yy + sz));
    rc.drawText(s, false, true);
}
void DealerProfile::textLJ(short leftX, short y, const char* s, COLOR col, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short yy = (short)(y - (short)(sz * 0.45f + 0.5f));
    RCT rc; rc.set(leftX, (short)(yy - sz), (short)(leftX + 900), (short)(yy + sz));
    rc.drawText(s, false, false);
}

void DealerProfile::render(const Settings& S)
{
    long n = getBarCount(); if (n < 2) return;
    lastBar = (int)n - 1;
    RCT pane; pane.getPaneRect(false);
    // (2.11.0) the Dealer column (ColumnBoard.h layout): every bar anchored on its right edge, growing left, never past its left edge
    const bp::Box& col = cs.lay.dealer;
    if (col.w() <= 0 || !cs.lay.ok) return;                         // no room for price on this chart: nothing drawn
    short anchor = (short)(col.r - 2);
    int W = std::max(8, col.w() - 6);

    if (!D.hasPrice || D.nodes.empty()) {   // (2.11.0) no data: said in the column's own header, nothing on the price area
        header(S, false, -1, anchor, col.w(), "no data");
        return;
    }
    float gmax = 0, dmax = 0;
    {   // (1.3.1) scale to the nodes ON SCREEN: a big strike off the visible price range (9/29 night: 4,150 at -47) shrank every
        // node in view to a few pixels
        dl::Data V = D; V.nodes.clear();
        for (size_t i = 0; i < D.nodes.size(); i++) {
            if (D.nodes[i].far) continue;
            short yy = yOf(D.nodes[i].k + off);
            if (yy >= pane.top && yy <= pane.bottom) V.nodes.push_back(D.nodes[i]);
        }
        dl::scales(V.nodes.empty() ? D : V, gmax, dmax);
    }
    // strike spacing in pixels
    float step = 0;
    for (size_t i = 1; i < D.nodes.size(); i++) { float d = D.nodes[i].k - D.nodes[i - 1].k; if (d > 0 && (step == 0 || d < step)) step = d; }
    if (step <= 0) step = D.em * 0.1f;
    int hs = std::abs((int)yOf(D.nodes[0].k + off) - (int)yOf(D.nodes[0].k + step + off));
    if (hs < 6) hs = 6;
    int nodeH = (int)(hs * 0.42f); if (nodeH < 3) nodeH = 3;
    int gap = hs >= 12 ? 1 : 0;

    if (!nx.empty() && dl::anyUsd(D)) {   // ---- 2.9.0: absolute GEX per strike, W / F by the net sign (file 2.4)
        double age = -1;
        const bool stale = gridVerdict(S.clock, age).stale();   // (2.17.0) Fresh.h: the same rule as the HOD / LOD box (was: never stale outside MenthorQ hours)
        renderNodeX(S, pane, anchor, hs, stale, age);   // (2.12.0) the data age for the bar hover
        header(S, stale, age, anchor, col.w(), nullptr);
        return;
    }
    if (dl::anyUsd(D)) {   // ---- 2.0.0: net GEX per strike (MenthorQ's numbers)
        std::vector<dl::Node> vis;
        for (size_t i = 0; i < D.nodes.size(); i++) { short yy = yOf(D.nodes[i].k + off); if (yy >= pane.top && yy <= pane.bottom) vis.push_back(D.nodes[i]); }
        float umax = dl::usdMax(vis.empty() ? D.nodes : vis);
        int barH = hs - 2 * gap - 1; if (barH > 2 * nodeH + 2) barH = 2 * nodeH + 2; if (barH < 3) barH = 3;
        if (barH > S.font + 6) barH = S.font + 6;              // (2.0.2) a bar, never a block (NG / EU zoomed in: strikes 100-300 px apart)
        bool ctxOnly = (mkt == "CL" || mkt == "NG");             // (2.0.2) options data context only in these markets: dimmed
        short lastLabY = -10000;
        for (size_t i = 0; i < D.nodes.size(); i++) {
            const dl::Node& N = D.nodes[i];
            if (!N.hasUsd) continue;
            short y = yOf(N.k + off);
            if (y < pane.top - hs || y > pane.bottom + hs) continue;
            float ft = (S.fadefar && N.far) ? 0.55f : 0.0f;
            if (ctxOnly && ft < 0.35f) ft = 0.35f;
            int L = dl::barLen(N.usd, umax, W), L0 = dl::barLen(dl::odtePart(N), umax, W);
            short t = (short)(y - barH / 2), b = (short)(t + barH);
            COLOR c = fade(N.usd < 0 ? C_SELL : C_BUY, ft), c0 = fade(N.usd < 0 ? C_SELL : C_BUY, ft + (1.0f - ft) * 0.45f);
            if (L > 0) box((short)(anchor - L), t, anchor, b, c);
            if (L0 > 0) box((short)(anchor - L0), t, anchor, b, c0);          // today's 0DTE: the darker part next to the scale
            // (2.0.2) a value on every visible bar (NQ's 5-pt strikes made the bars too thin and every value was skipped); when bars
            // are closer than the text is tall, the biggest bar there keeps its value
            if (S.labels && L >= 4 && y >= pane.top && y <= pane.bottom) {
                std::string lab = dl::usdLabel(N.usd);
                int tw = textW(lab.c_str(), S.font, true);
                bool inside = barH >= S.font - 3 && L - L0 > 0 && dl::fits(tw, L - L0);
                bool bigger = false;                                     // a bigger bar within one text height keeps the value
                for (size_t q = 0; q < D.nodes.size() && !bigger; q++) {
                    if (q == i || !D.nodes[q].hasUsd) continue;
                    short yq = yOf(D.nodes[q].k + off);
                    if (std::abs((int)yq - (int)y) < S.font + 3 && std::fabs(D.nodes[q].usd) > std::fabs(N.usd)) bigger = true;
                }
                if (!bigger && std::abs((int)y - (int)lastLabY) >= S.font + 3) {
                    if (inside) textLJ((short)(anchor - L + 3), y, lab.c_str(), N.usd < 0 ? 0x00FFFFFF : C_DARK, S.font, true);
                    else if (anchor - L - 3 - tw >= col.l + 1) textRJ((short)(anchor - L - 3), y, lab.c_str(), c, S.font, true);   // (2.11.0) never past the column
                    lastLabY = y;
                }
            }
            drawKeyPct(N.k, step, anchor, y, S);   // (2.2.7) no KEY / TGT / MAG chips - only the level strike's 1-hour % change
        }
        return;
    }
    for (size_t i = 0; i < D.nodes.size(); i++) {
        const dl::Node& N = D.nodes[i];
        short y = yOf(N.k + off);
        if (y < pane.top - hs || y > pane.bottom + hs) continue;
        float ft = (S.fadefar && N.far) ? 0.65f : 0.0f;
        int Lg = dl::barLen(N.g, gmax, W), Ls = dl::barLen(N.snap, gmax, W), Ld = dl::barLen(N.d, dmax, W);
        // (1.3.6, Rassul 2026-09-30: "delta on top because that is how we read it") DELTA on top (what dealers trade on the
        // way there), GAMMA below (what happens at the level) - read each strike top to bottom
        short dt_ = (short)(y - gap - nodeH), db = (short)(y - gap), gt = (short)(y + gap), gb = (short)(y + gap + nodeH);
        COLOR gc = fade(N.g >= 0 ? C_WALL : C_FUEL, ft), dc = fade(N.d >= 0 ? C_BUY : C_SELL, ft);
        if (Lg > 0) box((short)(anchor - Lg), gt, anchor, gb, gc);
        if (Lg > 2 && dl::strengthBars(N.gp) == 3) outline((short)(anchor - Lg), gt, anchor, gb);   // (1.3) market-moving: 3%+
        if (S.snap && Ls > 1) dashRect((short)(anchor - Ls), gt, anchor, gb, fade(C_SNAP, ft));
        if (Ld > 0) box((short)(anchor - Ld), dt_, anchor, db, dc);
        if (Ld > 2 && dl::strengthBars(N.dp) == 3) outline((short)(anchor - Ld), dt_, anchor, db);
        if (false && S.whisk && !N.far && std::fabs(N.hi - N.lo) >= 1.0f) {   // (1.3.5) no whisker: "(9 to 17)" is in the node's text
            int a = dl::barLen(N.lo, gmax, W), b = dl::barLen(N.hi, gmax, W);
            short ym = (short)((gt + gb) / 2), cap = (short)(nodeH / 2 > 2 ? nodeH / 2 : 2);
            short x1 = (short)(anchor - (a > b ? a : b)), x2 = (short)(anchor - (a < b ? a : b));
            line(x1, ym, x2, ym, C_WHISK, 1);
            line(x1, (short)(ym - cap), x1, (short)(ym + cap), C_WHISK, 1);
            line(x2, (short)(ym - cap), x2, (short)(ym + cap), C_WHISK, 1);
        }
        if (S.labels && !N.far && nodeH >= S.font - 1) {
            std::string gl = dl::gammaLabel(N), dlab = dl::deltaLabel(N);
            if (!dl::fits(textW(gl.c_str(), S.font, true), Lg)) gl = dl::gammaShort(N);   // (1.3.5) short node: "4220  10"
            if (dl::fits(textW(gl.c_str(), S.font, true), Lg)) textRJ((short)(anchor - 4), (short)((gt + gb) / 2), gl.c_str(), N.g < 0 ? 0x00FFFFFF : C_DARK, S.font, true);   // (1.3.9) white on purple
            if (dl::fits(textW(dlab.c_str(), S.font, true), Ld)) textRJ((short)(anchor - 4), (short)((dt_ + db) / 2), dlab.c_str(), N.d < 0 ? 0x00FFFFFF : C_DARK, S.font, true);
        }
        if (false && S.labels && !N.far && nodeH >= S.font - 3) {   // (2.11.0) off: it sat outside the Dealer column (1.x files only)   // (1.2) the strength share right OUTSIDE each node: % of a normal 15 min of volume
            int reachG = Lg; if (S.snap && Ls > reachG) reachG = Ls;
            if (false && S.whisk) { int a2 = dl::barLen(N.lo, gmax, W), b2 = dl::barLen(N.hi, gmax, W); if (a2 > reachG) reachG = a2; if (b2 > reachG) reachG = b2; }
            // (1.2) [lightning if LIVE] [strength bars] [%]  right outside the node, coloured by band (Rassul 2026-09-29)
            if (Lg > 0 && N.gp >= 0) strengthMark((short)(anchor - reachG - 4), (short)((gt + gb) / 2), N.gp, N.live, S.font, gc);
            if (Ld > 0 && N.dp >= 0) strengthMark((short)(anchor - Ld - 4), (short)((dt_ + db) / 2), N.dp, false, S.font, dc);
        }
        // (2.2.7, Rassul 23:32 "why even have KEY" / 23:33 "KEY and MAGNET cross over into the delta profile column") the chips are
        // gone; the level strike shows only its 1-hour GEX change, inside the Profile's own reach
        drawKeyPct(N.k, step, anchor, (short)((gt + gb) / 2), S);
    }
    // whole-book banner + legend (top-left of the pane)
    short bx = (short)(pane.left + 8), by = (short)(pane.top + 6);
    if (S.banner && !D.bookLine.empty()) {
        int tw = textW(D.bookLine.c_str(), S.font + 1, true);
        RCT bg; bg.set(bx, by, (short)(bx + tw + 20), (short)(by + S.font + 12));
        bg.draw(1, C_FUEL, C_BANNERBG, DRAW_OPAQUE, PAT_SOLID);
        textLJ((short)(bx + 10), (short)(by + (S.font + 12) / 2), D.bookLine.c_str(), D.book < 0 ? C_FUEL : C_WALL, S.font + 1, true);
        by = (short)(by + S.font + 16);
    }
    if (false && S.legend) {   // (1.1.1) operator 2026-09-29: "no more text on the chart - add it to the indicator" (the description)       // (1.1) operator 2026-09-29: say what each node is FOR, not only what it is
        short ly = (short)(by + S.font / 2 + 2), lh = (short)(S.font + 5);
        textLJ((short)(bx + 2), ly, "GAMMA node (lower): how hard this strike pushes back WHEN PRICE GETS THERE.  Cyan = WALL, dealers lean against every tick - stops a sweep and turns it.  Lime = FUEL, dealers chase the move there.", C_INK, S.font - 1, false);
        textLJ((short)(bx + 2), (short)(ly + lh), "DELTA node (upper): futures dealers MUST trade on the way to this strike (green buy / red sell) - the push into your level, and the stock they unwind after the turn (fuel for the reversal).", C_INK, S.font - 1, false);
        textLJ((short)(bx + 2), (short)(ly + 2 * lh), "Use both: the delta node says if there is flow to push price into the level; the gamma node says if the level stops it.  Whisker = earlier/later arrival, dashed = MenthorQ's number now.", C_MUTED, S.font - 1, false);
    }
}

// ------------------------------------------------------------------------------------------------------------------------
// (2.9.0) the Profile from file 2.4: one bar per strike, LENGTH = absolute GEX, colour / letter by the net sign (NODE usd)
// ------------------------------------------------------------------------------------------------------------------------
void DealerProfile::renderNodeX(const Settings& S, const RCT& pane, short anchor, int hs, bool stale, double ageMin)
{
    // (2.11.0) one-sided: every node anchored on the Dealer column's right edge, growing LEFT toward price; + teal / - orange by the
    // net sign; length = absolute GEX (the wall's size). Inside the node "B 1.2K" / "S 980" = futures dealers must buy / sell to stay
    // hedged at that price (Show buy / sell text in node) - only when it fits and the hedge number is real. Nothing is drawn outside
    // the column: the W / F letters, the % change and the KEY / TGT tags moved into the node's hover.
    const bp::Box& col = cs.lay.dealer;
    const int W = std::max(8, col.w() - 6);
    const int fz = S.font < 10 ? 10 : S.font;
    std::map<long long, float> netBy;                               // strike (1/10000 pt) -> net GEX from the NODE row
    for (size_t i = 0; i < D.nodes.size(); i++) if (D.nodes[i].hasUsd) netBy[std::llround((double)D.nodes[i].k * 10000.0)] = D.nodes[i].usd;
    std::map<long long, bool> farBy;
    for (size_t i = 0; i < D.nodes.size(); i++) farBy[std::llround((double)D.nodes[i].k * 10000.0)] = D.nodes[i].far;
    std::vector<size_t> vis;
    for (size_t i = 0; i < nx.size(); i++) { short yy = yOf(nx[i].k + off); if (yy >= pane.top && yy <= pane.bottom) vis.push_back(i); }
    // (2.15.1, Rassul 16:40 yardstick audit #4) the yardstick is the biggest strike in the WHOLE grid (this session's book), not the
    // biggest on screen: scrolling or zooming no longer changes which nodes / hedge numbers show or how long the bars are
    double amax = 0;
    for (size_t i = 0; i < nx.size(); i++) if (nx[i].absG > amax) amax = nx[i].absG;
    (void)vis;
    if (amax <= 0) amax = 1;
    const std::vector<size_t> top = dpaudit::top3(nx, vis);
    int barH = hs - 1; if (barH > fz + 6) barH = fz + 6; if (barH < 3) barH = 3;
    const bool ctxOnly = (mkt == "CL" || mkt == "NG");               // (2.0.2) options data context only in these markets: dimmed
    const float stF = stale ? 0.40f : 0.0f;                          // (2.9.0) a stale grid: every node fades
    float step = 0; for (size_t q = 1; q < nx.size(); q++) { float d = nx[q].k - nx[q - 1].k; if (d > 0 && (step == 0 || d < step)) step = d; }
    if (step <= 0) step = D.em * 0.1f;
    cs.nodeTexts = cs.nodeTextsHidden = cs.nodesTiny = 0;
    for (size_t i = 0; i < nx.size(); i++) {
        const dpaudit::NodeX& X = nx[i];
        const long long key = std::llround((double)X.k * 10000.0);
        const float net = netBy.count(key) ? netBy[key] : 0.0f;
        const short y = yOf(X.k + off);
        if (y < pane.top - hs || y > pane.bottom + hs) continue;
        const bool muted = dpaudit::mutedNode(X.absG, net);
        float ft = (S.fadefar && farBy.count(key) && farBy[key]) ? 0.55f : 0.0f;
        if (ctxOnly && ft < 0.35f) ft = 0.35f;
        if (muted) ft = ft + (1.0f - ft) * 0.45f;
        ft = stF + (1.0f - stF) * ft;
        const COLOR base = net >= 0 ? (COLOR)dcol::C_POS : (COLOR)dcol::C_NEG;
        if (!dcol::nodeShown(X.absG, amax)) { ++cs.nodesTiny; continue; }   // (14:37 CT) under 3% of the biggest: nothing (no slivers, no text)
        int L = dl::barLen((float)X.absG, (float)amax, W);
        if (L > W) L = W;
        const int bh = muted ? std::max(3, barH / 2) : barH;
        const short t = (short)std::max<int>(y - bh / 2, pane.top), b = (short)std::min<int>(y - bh / 2 + bh, pane.bottom);
        if (L > 0 && b > t) box((short)(anchor - L), t, anchor, b, fade(base, ft));
        if (y < pane.top || y > pane.bottom) continue;
        // the node's text: B / S + the contracts, inside, dark ink on the bright node; hidden when it does not fit or is not real
        const std::string nt = dcol::nodeText(X.hasD, X.d);
        bool shown = false;
        if (S.labels && !nt.empty()) {
            const int w = textW(nt.c_str(), fz, true);
            if (dcol::nodeTextFits(w, L, bh, fz)) { textLJ((short)(anchor - L + 3), y, nt.c_str(), C_DARK, fz, true); shown = true; ++cs.nodeTexts; }
            else ++cs.nodeTextsHidden;
        }
        // the hover: the node's row across the whole column
        HoverT H; H.l = col.l; H.r = col.r; H.t = (short)(y - std::max(bh, fz + 4) / 2); H.b = (short)(H.t + std::max(bh, fz + 4));
        H.key = "N|" + std::to_string(key); H.h = dcol::nodeHoverC(mkt, X, net, D.px + off, ageMin); (void)amax; (void)shown;   // (2.15.0) compact
        bool inTop = false; for (size_t j = 0; j < top.size(); j++) if (top[j] == i) inTop = true;
        if (inTop && X.hasChg) H.h.lines.push_back("Top 3 wall today: " + dpaudit::pctChg(X.chg) + " since the session's first grid.");
        for (size_t j = 0; j < D.tags.size(); j++) if (std::fabs(D.tags[j].k - X.k) <= step * 0.5f) { H.h.lines.push_back("Tag: " + D.tags[j].text + " (the Reader's mark on this strike)."); break; }
        hovT.push_back(H);
    }
}

// (2.9.0) the header over the Profile column: "Dealer Profile v2.11.0", then the grid's time / STALE, right-aligned inside the column
// (2.11.0) never wider than the column: "DP v2.11.0" when the full name does not fit, the second line shortened ("STALE 10h")
void DealerProfile::header(const Settings& S, bool stale, double age, short anchor, int colW, const char* note)
{
    RCT pane; pane.getPaneRect(false);
    const int fz = S.font < 10 ? 10 : S.font;
    const int room = colW - 6;
    std::string h1 = std::string("Dealer Profile v") + DP_VERSION;
    if (textW(h1.c_str(), fz, true) > room) h1 = std::string("DP v") + DP_VERSION;
    if (textW(h1.c_str(), fz, true) > room) h1 = "DP";
    std::string h2 = note ? std::string(note) : dpaudit::bookLine(bookAt, age, stale, D.srcBook, mkt);
    if (!h2.empty() && textW(h2.c_str(), fz, true) > room) {
        const size_t dash = h2.find(" - ");
        if (stale && dash != std::string::npos) h2 = h2.substr(0, dash);                      // "STALE 10h"
        else if (h2.find("book ") != std::string::npos) h2 = h2.substr(h2.find("book ") + 5);  // "2:55 PM"
        if (textW(h2.c_str(), fz, true) > room) h2.clear();                                   // never cut mid-word
    }
    // (2.11.1) WHY it is stale and what is being done - one or two more lines, only when stale and the bridge's diagnosis is fresh
    loadFeed();
    const long long nowU = (long long)time(nullptr);
    const std::vector<std::string> hx = (stale && !note && !h2.empty()) ? dpaudit::feedLines(feed, nowU, [&](const std::string& t) { return textW(t.c_str(), fz, true) <= room; }) : std::vector<std::string>();
    const short y1 = (short)(pane.top + 4 + (fz + 6) / 2), y2 = (short)(y1 + fz + 8);
    int w1 = textW(h1.c_str(), fz, true), w2 = h2.empty() ? 0 : textW(h2.c_str(), fz, true), wx = 0;
    for (size_t i = 0; i < hx.size(); ++i) wx = std::max(wx, textW(hx[i].c_str(), fz, true));
    const int w = std::min(colW, std::max(std::max(w1, w2), wx) + 10);
    const int extra = (int)hx.size() * (fz + 8);
    box((short)(anchor - w + 2), (short)(pane.top + 2), anchor, (short)(pane.top + 4 + (fz + 6) + (h2.empty() ? 0 : fz + 8) + extra), C_DARK);
    textRJ((short)(anchor - 4), y1, h1.c_str(), C_INK, fz, true);
    if (!h2.empty()) {
        if (stale) {
            RCT bg; bg.set((short)(anchor - std::min(colW - 2, w2 + 10)), (short)(y2 - (fz + 6) / 2), anchor, (short)(y2 + (fz + 6) / 2));
            bg.draw(1, 0x00C0392B, 0x003A1416, DRAW_OPAQUE, PAT_SOLID);
            textRJ((short)(anchor - 5), y2, h2.c_str(), 0x00FF9A8F, fz, true);
        } else textRJ((short)(anchor - 4), y2, h2.c_str(), C_MUTED, fz, true);
    }
    short yl = y2;
    for (size_t i = 0; i < hx.size(); ++i) { yl = (short)(yl + fz + 8); textRJ((short)(anchor - 5), yl, hx[i].c_str(), 0x00FF9A8F, fz, true); }
    HoverT H; H.l = (short)(anchor - w); H.r = anchor; H.t = pane.top; H.b = (short)(yl + fz); H.key = "HDR";
    H.h = dcol::headerHoverC(DP_VERSION, bookAt, age, stale);   // (2.15.0) compact
    dpaudit::feedHover(H.h, feed, nowU);
    if (note) H.h.lines.insert(H.h.lines.begin(), std::string("No dealer data for ") + (mkt.empty() ? "this chart" : mkt) + " yet: the LRA Reader writes LRA-Dealer-" + (mkt.empty() ? "<MKT>" : mkt) + ".csv every 5 min.");
    hovT.push_back(H);
}

bool DealerProfile::hitTarget(const HoverT& T)
{
    if (!msHas) return false;
    if (msH >= T.l && msH <= T.r && msV >= T.t && msV <= T.b) return true;           // the event's own pane position
    if (msBar >= 0 && dpFinite(msPrice)) {                                             // the bar / price the mouse points to
        PNT p; p.set((int)msBar, msPrice);
        if (p.h >= T.l && p.h <= T.r && p.v >= T.t && p.v <= T.b) return true;
    }
    return false;
}
const DealerProfile::HoverT* DealerProfile::hovered()
{
    for (size_t i = hovT.size(); i-- > 0;) if ((DP_HOVERS || hovT[i].pct || hovT[i].qm) && hitTarget(hovT[i])) return &hovT[i];   // (2.20.0) + the queue meter   // the last drawn (on top) first; (2.19.0) with the hovers off only the % block
    return nullptr;
}
// the hover box: title + lines, 10 pt+, left of the column it explains, inside the pane, never left of the Liquidity column
void DealerProfile::drawHover(const HoverT& T, const Settings& S)
{
    RCT pane; pane.getPaneRect(false);
    const int fz = S.font < 10 ? 10 : S.font, lh = fz + 5, pad = 7;
    const int lqW = (liqColRight > 0 && time(nullptr) - liqGoodAt <= dpaudit::LIQ_STALE_S) ? liqColRight + 8 : 110;
    const int minL = pane.left + (lqW > 110 ? lqW : 110);
    // (2.15.0, Rassul 13:28) the box never sits under the Liquidity Profile's BIG column (drawn after this indicator, it covered the
    // hover): its right edge stops left of the BIG column when the target is right of it
    int rMax = ((T.pct || T.qm) && cs.lay.levels.w() > 0 ? std::min<int>(T.l, cs.lay.levels.l) : T.l) - 8;   // (2.19.0) the % hover sits left of the whole badge row (its name stays visible)
    if (cs.lay.big.r > cs.lay.big.l && T.l >= cs.lay.big.l) rMax = std::min(rMax, cs.lay.big.l - 4);
    // (2.15.0, Rassul 13:25) the width that wraps evenly: fewest rows, then no row left with only a word or two, then the narrowest
    const int wHi = std::max(240, std::min(440, rMax - minL)), wLo = std::min(240, wHi);
    auto wrapAll = [&](int boxW, std::vector<std::string>& L1, std::vector<std::string>& L2) {
        L1 = dpaudit::wrapPx(T.h.title, boxW - 2 * pad, [&](const std::string& t) { return textW(t.c_str(), fz, true); });
        L2.clear();
        for (size_t i = 0; i < T.h.lines.size(); i++) {
            std::vector<std::string> w = dpaudit::wrapPx(T.h.lines[i], boxW - 2 * pad, [&](const std::string& t) { return textW(t.c_str(), fz, false); });
            L2.insert(L2.end(), w.begin(), w.end());
        }
    };
    const int boxW = dcol::hoverWidth(wLo, wHi, [&](int bw, int& rows, int& widows) {
        rows = 0; widows = 0;
        std::vector<std::string> t = dpaudit::wrapPx(T.h.title, bw - 2 * pad, [&](const std::string& u) { return textW(u.c_str(), fz, true); });
        rows += (int)t.size(); if (t.size() > 1 && textW(t.back().c_str(), fz, true) < (bw - 2 * pad) * 2 / 5) ++widows;
        for (size_t i = 0; i < T.h.lines.size(); i++) {
            std::vector<std::string> w = dpaudit::wrapPx(T.h.lines[i], bw - 2 * pad, [&](const std::string& u) { return textW(u.c_str(), fz, false); });
            rows += (int)w.size(); if (w.size() > 1 && textW(w.back().c_str(), fz, false) < (bw - 2 * pad) * 2 / 5) ++widows;
        }
    });
    std::vector<std::string> L1, L2; wrapAll(boxW, L1, L2);
    int tw = 0; for (const std::string& x : L1) tw = std::max(tw, textW(x.c_str(), fz, true)); for (const std::string& x : L2) tw = std::max(tw, textW(x.c_str(), fz, false));
    const int bw = std::min(boxW, tw + 2 * pad + 2);                    // hug the longest row
    const int h = 2 * pad + lh * (int)(L1.size() + L2.size()) + 4;
    int r = rMax, l = r - bw;
    if (l < minL) { l = minL; r = l + bw; }
    if (r > pane.right - 2) { r = pane.right - 2; l = r - bw; }
    int t = (T.t + T.b) / 2 - h / 2;
    if (t < pane.top + 2) t = pane.top + 2;
    if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
    RCT bg; bg.set((short)l, (short)t, (short)r, (short)(t + h)); bg.draw(1, C_MUTED, 0x00111827, DRAW_OPAQUE, PAT_SOLID);
    int y = t + pad + lh / 2;
    for (size_t i = 0; i < L1.size(); i++, y += lh) textLJ((short)(l + pad), (short)y, L1[i].c_str(), C_INK, fz, true);
    y += 4;
    for (size_t i = 0; i < L2.size(); i++, y += lh) textLJ((short)(l + pad), (short)y, L2[i].c_str(), C_MUTED, fz, false);
}
// (2.9.0) the mouse: remember where it is; redraw only when the hovered target changes. Never swallows the event.
int DealerProfile::mouse(RTX_EVENT* e)
{
    if (!DP_HOVERS && !DP_PCT_HOVERS) return RTX_FAIL;                   // (2.17.3) hovers off: the mouse is never used; (2.19.0) the % block hover
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    if (!loadHostState()) return RTX_FAIL;
    msHas = true; msBar = e->v.mouse.bar; msPrice = e->v.mouse.price; msH = e->v.mouse.h; msV = e->v.mouse.v;
    void* w = dpCursorWindow();
    if (w) {
        std::vector<void*>::iterator it = std::find(hovWnds.begin(), hovWnds.end(), w);
        if (it != hovWnds.end()) hovWnds.erase(it);
        hovWnds.push_back(w);
        if (hovWnds.size() > 8) hovWnds.erase(hovWnds.begin());
    }
    const HoverT* T = hovered();
    const std::string k = T ? T->key : std::string();
    const bool changed = k != hovKey;
    hovKey = k;
    saveHostState();
    if (changed) invalidateChart(false);
    return RTX_FAIL;
}

// (1.2) the strength mark, drawn right-to-left ending at rightX: "2.8%" in its band colour, then 0-3 rising bars, then a
// lightning bolt when COVER has confirmed dealers are actually trading at this level. Shapes, not glyphs (IRT text is ASCII).
void DealerProfile::outline(short l, short t, short r, short b)
{
    setPen(0x00FFFFFF, 2, P_SOLID);
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
static COLOR mix(COLOR c, float a)   // the colour at alpha a over the black chart (no line is ever black)
{
    int r = (int)(((c >> 16) & 0xFF) * a), g = (int)(((c >> 8) & 0xFF) * a), b = (int)((c & 0xFF) * a);
    return (COLOR)((r << 16) | (g << 8) | b);
}
static bool darkInk(COLOR c)        // dark text on a bright fill, white on a dark one (purple)
{
    float l = 0.299f * ((c >> 16) & 0xFF) + 0.587f * ((c >> 8) & 0xFF) + 0.114f * (c & 0xFF);
    return l >= 150.0f;
}
void DealerProfile::strengthMark(short rightX, short y, float pct, bool live, int font, COLOR base)
{
    // (1.5.0, option D - Rassul 2026-09-30, as the Reader's size chips) the % pill takes the colour of the node it sizes:
    // HEAVY 3%+ solid, BIG 1-3% half, MODERATE 0.25-1% faint fill + coloured text, LIGHT under 0.25% outline only
    int b = dl::strengthBars(pct);
    std::string t = dl::pctTxt(pct);
    int fsz = font - 1; if (fsz < 6) fsz = 6;
    int tw = textW(t.c_str(), fsz, true);
    short x = rightX;
    {
        COLOR fill = b == 3 ? base : b == 2 ? mix(base, 0.55f) : b == 1 ? mix(base, 0.22f) : 0x00000000;
        COLOR ink  = b == 3 ? (darkInk(base) ? 0x000B0F19 : 0x00FFFFFF) : b == 2 ? (darkInk(fill) ? 0x000B0F19 : 0x00FFFFFF) : base;
        COLOR edge = b == 0 ? mix(base, 0.6f) : fill;
        short padx = 6, h = (short)(fsz + 6);
        short w = (short)(tw + 2 * padx + 2);
        short l = (short)(rightX - w), t0 = (short)(y - h / 2), bt = (short)(t0 + h);
        setPen(edge, 1, P_SOLID);
        CBRUSH br(fill, PAT_SOLID); br.set();
        RCT rc; rc.set(l, t0, rightX, bt); rc.drawRounded(6, 6);
        {
            FONT f; f.id = HELVETICA; f.size = (short)fsz; f.style = BOLD; setFont(f);
            int lead = 0, asc = 0, desc = 0; getFontMetrics(&lead, &asc, &desc);
            short twc = (short)getTextWidth(t.c_str(), -1);
            setTextColor(ink);
            PNT tp; tp.set(0, 0.0f); tp.h = (short)(l + (w - twc) / 2); tp.v = (short)(y + (asc - desc) / 2); tp.drawText(t.c_str());
        }
        x = (short)(l - 4);
    }
    if (live) {                                              // the yellow lightning bolt: COVER confirmed dealers trading here
        short h = (short)(fsz + 2), w = (short)(h / 2 + 1);
        short top = (short)(y - h / 2), bot = (short)(y + h / 2), l = (short)(x - w), r = x;
        COLOR c = 0x00FACC15;
        line(r, top, (short)(l + 1), (short)(y + 1), c, 2);
        line((short)(l + 1), (short)(y + 1), (short)(r - 1), (short)(y - 1), c, 2);
        line((short)(r - 1), (short)(y - 1), l, bot, c, 2);
    }
}

void DealerProfile::staleBadge()
{
    if (D.asofSo < 0) return;
    RTDATE now = currentDate(); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(now, &t);
    double age = dl::staleMin(D.asofSo + cfg.clock * 60.0, t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec);
    if (D.y > 2000) {                                            // (2.0.2) by date: the time-of-day difference wrapped (Friday 16:03 read as 21h on Saturday)
        struct tm a; memset(&a, 0, sizeof(a)); a.tm_year = D.y - 1900; a.tm_mon = D.mo - 1; a.tm_mday = D.d;
        a.tm_hour = (int)(D.asofSo / 3600); a.tm_min = (int)((long)(D.asofSo / 60) % 60); a.tm_isdst = -1;
        struct tm b = t; b.tm_isdst = -1;
        double d = difftime(mktime(&b), mktime(&a)) / 60.0 - cfg.clock;
        if (d > -600) age = d < 0 ? 0 : d;
    }
    // (2.8.0, Rassul 10:24 "i see this alot. dealer data stale. why?") the bridge's options snapshot refreshes every ~10-15 min,
    // so a 10-min line fired on every normal cycle; warn only when a refresh was really missed
    {   // (2.17.0) Fresh.h (was: only past 25 min, at any hour): 20 min while MenthorQ updates, older than the last close while it pauses
        const long long nowCt = fr::ctAbs(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec);
        if (!fr::judge(fr::K_DEALER, nowCt - (long long)std::llround(age * 60.0), nowCt).stale()) return;
    }
    char b[40]; if (age >= 2880) snprintf(b, sizeof(b), "STALE %dd", (int)(age / 1440 + 0.5)); else if (age >= 90) snprintf(b, sizeof(b), "STALE %dh", (int)(age / 60 + 0.5)); else snprintf(b, sizeof(b), "STALE %dm", (int)(age + 0.5));
    RCT pane; pane.getPaneRect(false);
    int tw = textW(b, 10, true);
    const bp::Box& col = cs.lay.dealer;                           // (2.11.0) inside the Dealer column (older files only)
    if (col.w() < tw + 16) return;
    short x = (short)(col.r - tw - 16), y = (short)(pane.top + 6);
    RCT bg; bg.set(x, y, (short)(x + tw + 14), (short)(y + 18));
    bg.draw(1, 0x00C0392B, 0x003A1416, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x + 7), (short)(y + 9), b, 0x00FF9A8F, 10, true);
}

// (2.1.0, Rassul 2026-10-03: "for ES it should use SPX data ... SPX instead of ES") when the bars come from the index book
// (SPX for ES, QQQ for NQ, 08:30-15:00 CT) a small tag over the profile says so - the bars jump in size when the book changes
void DealerProfile::srcTag()
{
    if (D.srcBook.empty() || D.srcBook == mkt || D.nodes.empty()) return;
    RCT pane; pane.getPaneRect(false);
    std::string t = D.srcBook + " book";
    int tw = textW(t.c_str(), 10, true);
    const bp::Box& col = cs.lay.dealer;                           // (2.11.0) inside the Dealer column (older files only)
    if (col.w() < tw + 14) return;
    short x = (short)(col.r - tw - 14), y = (short)(pane.top + 30);
    RCT bg; bg.set(x, y, (short)(x + tw + 10), (short)(y + 16));
    bg.draw(1, 0x0022D3EE, 0x00062A33, DRAW_OPAQUE, PAT_SOLID);
    textLJ((short)(x + 5), (short)(y + 8), t.c_str(), 0x0022D3EE, 10, true);
}

// (2.18.0) PROFILE,day cover 99.8%,used 1,DPoc r1 7703 r2 7702.5 r4 7702,week cover 100%,used 1,WPoc r1 .. r4 ..,rows 1,vap read N missing M
std::string DealerProfile::profileLine()
{
    std::map<std::string, KLC>::const_iterator it = klc.find(mkt);
    if (it == klc.end()) return "PROFILE,none yet";
    const KLC& K = it->second; const klv::ProfDiag& g = K.diag;
    auto px = [](double v) { if (!std::isfinite(v)) return std::string("-"); char b[32]; std::snprintf(b, sizeof(b), "%.6g", v); return std::string(b); };
    auto pc = [](double v) { char b[32]; std::snprintf(b, sizeof(b), "%.1f%%", v * 100.0); return std::string(b); };
    return "PROFILE,day cover " + pc(g.dayCover) + ",used " + (g.dayUsed ? "1" : "0") + ",DPoc r1 " + px(g.dpoc1) + " r2 " + px(g.dpoc2) + " r4 " + px(g.dpoc4)
         + ",week cover " + pc(g.weekCover) + ",used " + (g.weekUsed ? "1" : "0") + ",WPoc r1 " + px(g.wpoc1) + " r4 " + px(g.wpoc4)
         + ",rows " + std::to_string(klv::profileRowTicks(mkt)) + ",vap read " + std::to_string(K.vapRead) + " missing " + std::to_string(K.vapMissing)
         + ",cached " + std::to_string(K.vap ? K.vap->bars.size() : 0);
}
void DealerProfile::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerProfile.status.txt";
    std::ostringstream f;
    f << "VERSION," << DP_VERSION << "\nROOT," << root << "\nMARKET," << mkt << "\nNODES," << D.nodes.size() << "\nOFFSET," << off << "\nBOOK," << D.srcBook << "\nRATIO," << D.srcRatio << "\nRATIO_MODE," << D.srcMode << "\nSTATE," << what
      << "\nREACH," << cs.lay.w[cb::DEALER] << "\n"   // (2.11.0) the Dealer column's px from the scale's edge (the Read / Analyst add the Delta column)
      << "COLUMNS," << cs.lay.levels.l << "-" << cs.lay.levels.r << "," << cs.lay.delta.l << "-" << cs.lay.delta.r << "," << cs.lay.dealer.l << "-" << cs.lay.dealer.r
      << ",price right " << cs.lay.priceRight << (cs.lay.squeezed ? ",squeezed" : "") << (cb::columnsClean(cs.lay) ? ",clean" : ",OVERLAP") << ",delta " << (cs.peers.on[cb::DELTA] ? "on" : "off") << "\n"
      << "MARGIN,need " << cs.needPx << " px,units " << (cs.probe.units == cb::PIXELS ? "pixels" : "bars") << ",ppb " << cs.ppb << ",read " << cs.marginRead
      << ",asked " << cs.marginAsked << ",set ok " << cs.setOk << ",set fail " << cs.setFail << ",gap " << cs.gapSeen << "\n"
      << "BADGES,shown " << cs.badgesShown << ",dropped " << cs.badgesDropped << ",liquidity blocks " << cs.liqBlocks << ",HL " << cs.hlOn
      << ",liquidity file " << (cs.liqLWhy.empty() ? std::string("ok") : cs.liqLWhy) << ",HL line " << cs.liqL.onX << "x / " << cs.liqL.offX << "x" << "\nNODE_TEXT,shown " << cs.nodeTexts << ",hidden " << cs.nodeTextsHidden << ",tiny nodes not drawn " << cs.nodesTiny << "\n"
      << "LEVELS_COLUMN," << (DP_LINES ? "lines (LevelsColumn " : DP_LVC ? "combined (LevelsColumn " : "off (") << lvc::VERSION << ")" << ",rows " << cs.lvcRows << ",merged rows " << cs.lvcMerged << ",with liquidity " << cs.lvcLiq << ",GEX rows " << cs.lvcGex << ",chips cut " << cs.lvcChipsCut << ",orders " << cs.lmtO.size() << ",order file " << (cs.lmtWhy.empty() ? std::string("ok") : cs.lmtWhy) << ",dealer bars " << (DP_BARS ? "on" : "off") << ",off-screen " << (offUp.have ? offUp.name : std::string("-")) << " / " << (offDn.have ? offDn.name : std::string("-")) << "\n"
      << "LEVEL_PCT," << (cfg.levelPct ? "on" : "off (setting)") << ",file " << (cs.lpWhy.empty() ? std::string("ok") : cs.lpWhy) << ",rows " << cs.lpT.rows.size()
      << ",built " << (cs.lpT.built.empty() ? std::string("-") : cs.lpT.built) << ",shown " << cs.lpShown << ",green " << cs.lpGreen << ",no row " << cs.lpNoRow << ",no room " << cs.lpNoRoom << "\n"
      << "QUEUE_METER," << (cfg.qm ? "on" : "off (setting)") << ",reach " << cfg.qmReach << " ATR,file " << cs.qmF.why << (cs.qmF.ok ? ",records " + std::to_string(cs.qmF.t.size()) + ",age " + std::to_string((long long)time(nullptr) - cs.qmMtime) + " s" : std::string())
      << ",on a level badge " << cs.qmOnBadge << ",unlabeled " << cs.qmAlone << ",beyond reach " << cs.qmFar << ",inactive " << cs.qmInactive << ",no room " << cs.qmNoRoom << ",outline = the 10-s call (red / green; grey none),Q letter: Holding strip of lsBookFlow 1.23.0 (a record, Strips\\QUEUE-<MKT>-<spb>.csv by lsLiquidityProfile)\n"
      << profileLine() << "\n";
    // (2.7.4) written when its content changes (or once a minute), not on every repaint. (2.17.0, audit C finding 10) one file PER CHART
    // (DealerProfile.status-<MKT>-<spb>.txt) + the old shared DealerProfile.status.txt for one more version (ChartView / the Tape Reader
    // still read it); each with this chart's own gate (the 2.7.4 function static was shared by every chart on the DLL, lesson 1)
    const std::string body = f.str();
    State* state = slot_.get(this, false);
    const std::uintptr_t id = state ? reinterpret_cast<std::uintptr_t>(state) : reinterpret_cast<std::uintptr_t>(this);
    auto publish = [&](const std::string& target, dpaudit::WriteGate& gate) {
        if (!gate.due(body, time(nullptr))) return;
        // (2.8.7, audit DP-004) written to a unique temp file, checked, then replaced: a reader never sees a half-written status
        const std::string tmp = dpaudit::exportTempPath(target, id);
        bool wrote = false;
        {
            std::ofstream out(tmp.c_str(), std::ios::trunc);
            if (out.is_open()) { out << body; out.flush(); wrote = out.good(); }
        }
        if (!(wrote && dpReplaceFile(tmp, target))) { std::remove(tmp.c_str()); gate.last.clear(); }
    };
    const std::string per = dcol::statusName(mkt, cs.spb);
    if (!per.empty()) publish(std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\" + per, cs.gateMkt);
    publish(path, cs.gateOld);
}

// (2.17.0) the grid's age (minutes, -1 = unknown) and its verdict: Fresh.h, the same thresholds as the HOD / LOD box (unknown = STALE)
fr::Verdict DealerProfile::gridVerdict(int clockOff, double& ageMin)
{
    RTDATE now = currentDate(); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(now, &t);
    ageMin = dpaudit::ageMinutes(bookAt, t, clockOff);
    if (ageMin < 0) return fr::staleV(-1, false);
    const long long nowCt = fr::ctAbs(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec);
    return fr::judge(fr::K_DEALER, nowCt - (long long)std::llround(ageMin * 60.0), nowCt);
}
static int dpDecOfTick(double tick)
{
    if (!(tick > 0)) return 2;
    for (int d = 0; d <= 6; ++d) { const double m = tick * std::pow(10.0, d); if (std::fabs(m - std::floor(m + 0.5)) < 1e-6) return d; }
    return 6;
}
// (2.17.0) MARKET STATE, SRC "GEX": decided for the last CLOSED bar (slow state may update it within the bar), re-posted every 10 s so a
// reader sees it (MarketState.h ignores entries older than 60 s), cleared on removal. The MenthorQ levels are the ones drawn on this
// chart (the same contract pick as drawMQLevels); nodes = the grid's strikes with the dealers' hedge lots (file 2.4).
void DealerProfile::postMst()
{
    if (mkt.empty() || cs.sym.empty()) return;
    const long nb = getBarCount(); if (nb < 2) return;
    RTARRAY cl(barClose); RTARRAYI dt(barDateTime);
    if ((long)cl.count < nb || (long)dt.count < nb) return;
    const double close = (double)cl[(int)nb - 2];
    struct tm bt; memset(&bt, 0, sizeof(bt)); if (!getLocaltime((RTDATE)dt[(int)nb - 2], &bt)) return;
    const long long bar = fr::ctAbs(bt.tm_year + 1900, bt.tm_mon + 1, bt.tm_mday, bt.tm_hour * 3600 + bt.tm_min * 60 + bt.tm_sec);
    std::vector<dcol::MqLvl> lv;
    {   std::vector<std::string> keys; for (auto& kv : mql) keys.push_back(kv.first);
        bool shift = false; const std::string sym = keys.empty() ? std::string() : dpaudit::pickMQSymbol(keys, root, mkt, D.chartSym, shift);
        if (!sym.empty()) { const float sh = shift ? off : 0.0f; for (const MQL& q : mql[sym]) { dcol::MqLvl L; L.label = q.label; L.px = (double)(q.px + sh); lv.push_back(L); } } }
    std::vector<dcol::HedgeNode> nodes;
    for (const dpaudit::NodeX& x : nx) { dcol::HedgeNode h; h.px = (double)(x.k + off); h.hasD = x.hasD; h.d = x.d; nodes.push_back(h); }
    double age = -1; const fr::Verdict v = gridVerdict(cfg.clock, age);
    const double tick = chartTick(mkt);
    std::map<std::string, std::string> kv = dcol::mstGex(lv, nodes, close, tick, dpDecOfTick(tick), age, v.stale());
    std::string sig; for (const auto& p : kv) if (p.first != "data_age_min") sig += p.first + "=" + p.second + ";";
    const long long now = nowMsC();
    if (cs.mstPosted && (cs.sym != cs.mstSym || cs.spb != cs.mstSpb || cs.chart != cs.mstChart)) clearMst();
    const bool newBar = bar != cs.mstBar, changed = sig != cs.mstSig;
    if (newBar || changed) { cs.mstKv = kv; cs.mstBar = bar; cs.mstSig = sig; }
    if (!(newBar || changed || !cs.mstPosted || cs.mstMs < 0 || now - cs.mstMs >= 10000 || now < cs.mstMs)) return;
    mst::publish("GEX", cs.sym, cs.spb, cs.chart, now, cs.mstBar, cs.mstKv);
    if (lsperf::smartAllowed(LSPERF_PLUGIN, cs.sym)) {   // (2.26.0) PerfGuard: self-awareness only while this indicator is within budget
    {   // (2.26.0) SELF-AWARENESS (SelfAware.h, SELF-AWARE.md): price closing within 0.25 ATR of the nearest MenthorQ level is a level call -
        // close above it = support (up), below = resistance (down), once per touch; HIT = 1 ATR away before 1 ATR through (later bars only).
        // Value = the dealers' hedge lots at that level. Display unchanged (SWITCH OFF per market).
        if (!cs.self) cs.self = std::make_shared<sa::Hook>();
        RTARRAY op(barOpen), hi(barHigh), lo(barLow); const int i = (int)nb - 2;
        if (op.count > i && hi.count > i && lo.count > i) {
            std::map<std::string, std::string> kv = cs.mstKv; const std::string nl = sa::factStr(kv, "near_level"); const size_t at = nl.rfind('@');
            const double lpx = at == std::string::npos ? std::nan("") : std::strtod(nl.c_str() + at + 1, nullptr), atr = cs.self->tr.atr();
            const bool touch = lpx == lpx && atr > 0 && std::fabs(close - lpx) <= 0.25 * atr && kv["stale"] != "1";
            kv["call"] = !touch ? "none" : close >= lpx ? "support" : "resistance";
            static const sa::Spec SP = [] { sa::Spec x; x.valueKey = "near_lots"; x.sideKey = "call"; x.eventKey = "near_level"; x.onChange = true; x.rearm = true;
                x.why = { "near_level", "near_dist_ticks", "near_lots", "regime", "hvl", "cr", "ps" }; return x; }();
            cs.self->ind = "GEX";
            sa::fromFacts(*cs.self, SP, cs.sym, cs.spb, cs.chart, mkt, now, sa::Bar{ cs.mstBar, op[i], hi[i], lo[i], close }, kv);
        }
    }
    }
    cs.mstPosted = true; cs.mstSym = cs.sym; cs.mstSpb = cs.spb; cs.mstChart = cs.chart; cs.mstMs = now;
}
void DealerProfile::clearMst()
{
    if (!cs.mstPosted) return;
    mst::clear("GEX", cs.mstSym, cs.mstSpb, cs.mstChart);     if (cs.self) cs.self->clear(cs.mstSym, cs.mstSpb, cs.mstChart);
    cs.mstPosted = false;
}

// (1.3.7, Rassul 2026-09-30: the Read went blank overnight - MenthorQ's price freezes outside RTH) the chart's own last 200
// bars -> LRA-IRT-Bars-<MKT>.csv, so the Reader always has the live price, highs / lows and volume (sweeps, triggers) even at
// night. Written when a bar changes, at most every 15 s; raw chart prices plus the contract offset the Reader takes off.
void DealerProfile::exportBars()
{
    long n = getBarCount(); if (n < 2 || mkt.empty()) return;
    RTARRAY o(barOpen), h(barHigh), l(barLow), c(barClose);
    RTARRAYI v(barVolume);                                   // volume is an integer array in the SDK
    RTARRAYI dtm(barDateTime);
    float lc = c[(int)n - 1];
    time_t now = time(0);
    Exp& E = expBy[mkt];
    if (now - E.t < 15) return;
    if (n == E.n && lc == E.c && now - E.t < 60) return;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-IRT-Bars-" + mkt + ".csv";
    State* state = slot_.get(this, false);
    const std::uintptr_t id = state ? reinterpret_cast<std::uintptr_t>(state) : reinterpret_cast<std::uintptr_t>(this);
    std::string tmp = dpaudit::exportTempPath(path, id);
    // (1.5.0, Rassul 2026-09-30) his own indicators (cob_bull, cob_bear ... by name, set in the settings) ride along, one
    // column each, so the Reader can check its rebuilt triggers against the chart's - the chart is the source of truth
    std::vector<std::string> names; std::vector<RTARRAY*> arrs;
    {
        std::string cur; std::string all = cfg.inds + ",";
        for (size_t i = 0; i < all.size(); i++) {
            char ch = all[i];
            if (ch == ',' || ch == ';') {
                while (!cur.empty() && cur.back() == ' ') cur.pop_back();
                if (!cur.empty() && names.size() < 8) {
                    RTARRAY* a = new RTARRAY(fEmptyArray);
                    char nm[80]; memset(nm, 0, sizeof(nm)); strncpy_s(nm, sizeof(nm), cur.c_str(), _TRUNCATE);
                    if ((a->getChartIndicator(nm) == RTX_OK && a->count > 0) || (a->getCustomIndicator(nm, 1, true) == RTX_OK && a->count > 0)) { names.push_back(cur); arrs.push_back(a); } else delete a;
                }
                cur.clear();
            } else if (!(cur.empty() && ch == ' ') && ch != '|') cur += ch;
        }
    }
    std::string nl; for (size_t j = 0; j < names.size(); j++) nl += (j ? "," : "") + names[j];
    bool wrote = false;
    {
        std::ofstream f(tmp.c_str(), std::ios::trunc);
        if (!f.is_open()) { for (size_t j = 0; j < arrs.size(); j++) delete arrs[j]; return; }
        char b[200];
        snprintf(b, sizeof(b), "IRTBARS|2|%s|%s|%.6f|%ld|%s\n", mkt.c_str(), root.c_str(), (double)off, (long)now, nl.c_str()); f << b;
        int from = (int)n - 200; if (from < 0) from = 0;
        for (int i = from; i < (int)n; i++) {
            struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dtm[i], &t);
            snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d|%.6f|%.6f|%.6f|%.6f|%.0f\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                      t.tm_hour, t.tm_min, t.tm_sec, (double)o[i], (double)h[i], (double)l[i], (double)c[i], (double)v[i]);
            std::string row(b); if (!row.empty() && row.back() == '\n') row.pop_back();
            for (size_t j = 0; j < arrs.size(); j++) {
                long k = (long)i - ((long)n - arrs[j]->count);           // the indicator's array ends on the chart's last bar
                char vb[32]; if (k >= 0 && k < arrs[j]->count) snprintf(vb, sizeof(vb), "|%.4f", (double)(*arrs[j])[(int)k]); else snprintf(vb, sizeof(vb), "|");
                row += vb;
            }
            f << row << "\n";
        }
        f.flush(); wrote = f.good();
    }
    for (size_t j = 0; j < arrs.size(); j++) delete arrs[j];
    if (wrote && dpReplaceFile(tmp, path)) { E.n = n; E.c = lc; E.t = now; }
    else std::remove(tmp.c_str());
}

void DealerProfile::loadMQLevels()
{
    time_t now = time(nullptr);
    if (now - mqlChecked < 5 && mqlStamp != -2) return;      // a stat at most every 5 s
    mqlChecked = now;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\MenthorQLevels.csv";
    long long st = dl::fileStamp(p);
    if (st == mqlStamp) return;
    if (st < 0) { mql.clear(); mqlStamp = st; return; }
    std::vector<std::string> lines;
    if (dpaudit::readBoundedLines(p, lines) != dpaudit::READ_OK || dl::fileStamp(p) != st) return;
    std::map<std::string, std::vector<MQL> > m;
    bool head = true;
    for (size_t row = 0; row < lines.size(); ++row) {
        const std::string& ln = lines[row];
        if (head) { head = false; if (ln.compare(0, 6, "SYMBOL") == 0) continue; }
        std::vector<std::string> c; std::stringstream ss(ln); std::string x;
        while (std::getline(ss, x, ',')) c.push_back(x);
        if (c.size() < 5 || c[0].empty()) continue;
        MQL q; if (!dpaudit::parsePositiveFinite(c[1], q.px)) continue; q.label = c[2];
        int v = 0, w = 0; if (!dpaudit::parseInt(c[3], v) || !dpaudit::parseInt(c[4], w)) continue;
        q.col = (COLOR)(v & 0x00FFFFFF); if (q.col == 0) q.col = 0x0022D3EE;   // never black
        q.w = w; if (q.w < 1) q.w = 1; if (q.w > 3) q.w = 3;
        m[c[0]].push_back(q);
    }
    mql.swap(m); mqlStamp = st;
}

void DealerProfile::loadTouch(const std::string& m)
{
    time_t now = time(nullptr);
    if (now - tpChecked[m] < 5 && tpStamp.count(m)) return;
    tpChecked[m] = now;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Touch-" + m + ".csv";
    long long st = dl::fileStamp(p);
    if (tpStamp.count(m) && st == tpStamp[m]) return;
    if (st < 0) { tps[m].clear(); tpStamp[m] = st; tpAt[m] = 0; return; }
    std::vector<std::string> lines;
    if (dpaudit::readBoundedLines(p, lines) != dpaudit::READ_OK || dl::fileStamp(p) != st) return;
    std::vector<TP> v;
    for (size_t row = 0; row < lines.size(); ++row) {
        const std::string& ln = lines[row];
        std::vector<std::string> c; std::stringstream ss(ln); std::string x;
        while (std::getline(ss, x, '|')) c.push_back(x);
        if (c.size() >= 7 && c[0] == "TOUCH") { TP t; if (!dpaudit::parsePositiveFinite(c[2], t.px)) continue; t.p1 = c[3]; t.pe = c[4]; t.pc = c[5]; t.proven = c[6] == "1"; if (c.size() >= 8) t.txt = c[7]; v.push_back(t); }
    }
    tps[m].swap(v); tpStamp[m] = st; tpAt[m] = st > 0 ? (time_t)(st / 1000003LL) : 0;   // the file's modified time
}

// (2.8.0) re-read at most every second, only when the file changed; parsed (and judged fresh: <= 20 s) every time it is read.
// The freshness is re-checked at each draw from the file's own stamp, so a Liquidity Profile that stops drops the tags in 20 s.
void DealerProfile::loadLiq(const std::string& m)
{
    time_t now = time(nullptr);
    if (now == liqChecked && liqStamp != -2) return;
    liqChecked = now;
    const char* up = getenv("USERPROFILE"); if (!up) { liqZ.clear(); return; }
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-LiqZones-" + m + ".csv";
    long long st = dl::fileStamp(p);
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (f.is_open()) { text.resize(65536); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); } }
    liqStamp = st;
    std::vector<dpaudit::LiqZ> z;
    int cr = 0;
    if (dpaudit::parseLiqZones(text, (long long)now, z, liqWhy, &cr)) { liqZ.swap(z); liqGoodAt = now; liqColRight = cr; }
    else if (liqWhy == "stale" || now - liqGoodAt > dpaudit::LIQ_STALE_S) liqZ.clear();   // (2.8.1) a read caught mid-replace keeps the last zones
}
void DealerProfile::addLiq(PendLbl& P, double chartPx)
{
    const long long key = (long long)std::llround(chartPx * 10000.0);
    const time_t now = time(nullptr);
    const dpaudit::LiqZ* z = dpaudit::liqAt(liqZ, chartPx, liqShown[key]);
    liqShown[key] = z != nullptr;
    // (2.8.1, Rassul 11:18 "the liq goes away, then comes back") a tag stays 30 s after its zone dips under the line or the
    // zones drop out between refreshes - while the Liquidity Profile is live; then it goes
    if (z) { LiqMem& m = liqMem[key]; m.z = *z; m.seen = now; }
    else {
        std::map<long long, LiqMem>::iterator it = liqMem.find(key);
        if (it == liqMem.end()) return;
        if (now - it->second.seen > 30 || now - liqGoodAt > dpaudit::LIQ_STALE_S) { liqMem.erase(it); return; }
        z = &it->second.z;
    }
    P.seg.push_back(LSeg{ dpaudit::liqTag(*z), true, z->bid ? C_BUY : C_SELL });   // green = resting bids, red = resting offers
}

// (11:39 CT) LRA-LiqLevels-<MKT>.csv from the Liquidity Profile on this market's chart (native, the per-market file pattern): read at
// most once a second, only when it changed; over 20 s old / garbled = no liquidity (name-only badges, never 0 or garbage)
void DealerProfile::loadLiqLevels()
{
    const long long now = nowMsC();
    if (cs.liqLChecked >= 0 && now >= cs.liqLChecked && now - cs.liqLChecked < 1000) return;
    cs.liqLChecked = now;
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty()) { cs.liqL = dcol::LiqLevels(); cs.liqLWhy = "no market"; return; }
    const std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-LiqLevels-" + mkt + ".csv";
    const long long st = dl::fileStamp(p);
    const long long unixNow = (long long)time(nullptr);
    if (st == cs.liqLStamp) {                                       // unchanged: only the age can make it stale
        if (cs.liqL.at > 0 && unixNow - cs.liqL.at > dcol::LIQL_STALE_S) { cs.liqL = dcol::LiqLevels(); cs.liqLWhy = "stale"; }
        return;
    }
    if (st < 0) { cs.liqL = dcol::LiqLevels(); cs.liqLStamp = st; cs.liqLWhy = "no file"; return; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (f.is_open()) { text.resize(262144); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); } }
    if (dl::fileStamp(p) != st) return;                              // torn read: try again next second
    dcol::LiqLevels L; std::string why;
    if (dcol::parseLiqLevels(text, unixNow, L, why)) { cs.liqL = L; cs.liqLWhy.clear(); }
    else { cs.liqL = dcol::LiqLevels(); cs.liqLWhy = why; }
    cs.liqLStamp = st;
}
// (2.19.0) the level % file of THIS chart's market: re-read only when its stamp changes (checked at most once a second), bounded, torn-safe;
// missing / garbled / another market = no blocks and the reason on the LEVEL_PCT status line (the last good table is NOT kept for a
// garbled file: a wrong number must never show)
void DealerProfile::loadLevelPct()
{
    const long long now = nowMsC();
    if (cs.lpMkt != mkt) { cs.lpT = lpct::Table(); cs.lpStamp = -2; cs.lpChecked = -1; cs.lpMkt = mkt; cs.lpWhy = "not read yet"; }
    if (cs.lpChecked >= 0 && now >= cs.lpChecked && now - cs.lpChecked < 1000) return;
    cs.lpChecked = now;
    if (mkt.empty()) { cs.lpT = lpct::Table(); cs.lpWhy = "no market"; return; }
    const std::string p = dpFlexDir() + "LRA-LevelBadges-" + mkt + ".csv";
    const long long st = dl::fileStamp(p);
    if (st == cs.lpStamp) return;
    if (st < 0) { cs.lpT = lpct::Table(); cs.lpStamp = st; cs.lpWhy = "no file LRA-LevelBadges-" + mkt + ".csv (the nightly writes it)"; return; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (f.is_open()) { text.resize(lpct::MAX_BYTES + 1); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); } }
    if (dl::fileStamp(p) != st) return;                              // torn read: try again next second
    lpct::Table T; std::string why;
    if (lpct::parse(text, mkt, T, why)) { cs.lpT = T; cs.lpWhy.clear(); } else { cs.lpT = lpct::Table(); cs.lpWhy = why; }
    cs.lpStamp = st;
}
// The reader owns this boundary check rather than QueueMeter.h: it has no MBO book and must not turn a freshly touched but incomplete
// producer file into a queue claim.  All four fields are emitted together by qm::fileText(); q0/q and the test rows are meaningless
// without their producer time, tick grid, current price and ATR reach context.
static const char* dpQueueHeaderIssue(const qm::File& f)
{
    if (f.asof <= 0) return "incomplete live header: ASOF";
    if (!(f.tick > 0) || !dpFinite(f.tick)) return "incomplete live header: TICK";
    if (!(f.last > 0) || !dpFinite(f.last)) return "incomplete live header: LAST";
    if (!(f.atr > 0) || !dpFinite(f.atr)) return "incomplete live header: ATR";
    return nullptr;
}
// (2.20.0) THE QUEUE METER file (lsLiquidityProfile 0.49.0 writes it about once a second while a test is live): bounded, torn-safe,
// stamp-checked at most every 500 ms; older than 5 s (the Liquidity Profile stopped / was removed), incomplete, or garbled = normal badges
void DealerProfile::loadQueue()
{
    const long long now = nowMsC();
    if (cs.qmMkt != mkt) { cs.qmF = qm::File(); cs.qmStamp = -2; cs.qmChecked = -1; cs.qmMkt = mkt; cs.qmMtime = 0; }
    if (cs.qmChecked >= 0 && now >= cs.qmChecked && now - cs.qmChecked < 500) return;
    cs.qmChecked = now;
    if (mkt.empty()) { cs.qmF = qm::File(); cs.qmF.why = "no market"; return; }
    const std::string p = dpFlexDir() + "LRA-QueueLive-" + mkt + ".csv";   // (2.21.0) LRA-QueueMeter-<MKT>.csv is now the nightly params file
    const long long st = dl::fileStampPrecise(p);
    if (st == cs.qmStamp) return;
    if (st < 0) { cs.qmF = qm::File(); cs.qmF.why = "no file LRA-QueueLive-" + mkt + ".csv (lsLiquidityProfile 0.50.0+ on this market writes it)"; cs.qmStamp = st; return; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (f.is_open()) { text.resize(65536 + 1); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); } }
    if (dl::fileStampPrecise(p) != st) return;                       // torn read: again next time
    if (text.size() > 65536) { cs.qmF = qm::File(); cs.qmF.why = "file too big - not used"; cs.qmStamp = st; return; }
    std::vector<std::string> L; { std::string cur; for (size_t i = 0; i < text.size(); ++i) { if (text[i] == '\n') { L.push_back(cur); cur.clear(); } else cur += text[i]; } if (!cur.empty()) L.push_back(cur); }
    qm::File parsed = qm::parseFile(L, mkt);
    const char* issue = parsed.ok ? dpQueueHeaderIssue(parsed) : nullptr;
    if (issue) { cs.qmF = qm::File(); cs.qmF.why = issue; }
    else cs.qmF = parsed;
    cs.qmStamp = st; cs.qmMtime = dl::fileStampSeconds(st);
}
// the live test whose zone holds this level price, if the meter is on, the file is fresh and the test is within the reach of price
const qm::FileT* DealerProfile::queueAt(double chartPx, const Settings& S, bool& used)
{
    used = false;
    if (!S.qm || !cs.qmF.ok || !(chartPx == chartPx)) return nullptr;
    if (std::llabs((long long)time(nullptr) - cs.qmMtime) > 5) return nullptr;      // stale: the writer stopped
    const double atr = cs.qmF.atr, last = cs.qmF.last, tk = cs.qmF.tick > 0 ? cs.qmF.tick : 0.25;
    const qm::FileT* best = nullptr; double bestD = 1e18;
    for (size_t i = 0; i < cs.qmF.t.size(); ++i) {
        const qm::FileT& T = cs.qmF.t[i];
        if (!T.contact || T.decided >= 0) continue;                       // retained rows support the producer's Q record, not a live badge
        if (chartPx < T.lo - tk * 0.5 || chartPx > T.hi + tk * 0.5) continue;
        if (!(atr > 0) || std::fabs(last - T.anchor) > S.qmReach * atr) continue;   // only where a test is happening, within the reach
        const double d = std::fabs(chartPx - T.anchor); if (d < bestD) { bestD = d; best = &T; }
    }
    used = best != nullptr; return best;
}
// one meter badge between l and r: dark ground, the fill = queue left (green bids / red offers), 1 px outline in the side colour, or 2 px
// yellow with yellow text when the originals are gone; the text shrinks (badge font .. 7 pt, then shorter variants) and never leaves l..r
bool DealerProfile::drawMeter(const qm::FileT& T, const std::string& name, short l, short r, int t, int b, int yc, const Settings& S, const std::string& key, const std::string& pxTxt)
{
    if (r - l < 24) return false;
    const COLOR side = T.side == qm::BID ? S.qmBid : S.qmAsk, ink = T.warn ? S.qmWarn : (COLOR)0x00FFFFFF;
    std::string tx; int fz = 0;
    for (int v = 0; v < 3 && tx.empty(); ++v) for (int f = S.badgeFont; f >= 7; --f) {
        const std::string c = qm::badgeText(v == 2 ? std::string() : name, T, v);
        if (textW(c.c_str(), f, true) + 8 <= r - l) { tx = c; fz = f; break; }
    }
    if (tx.empty()) return false;
    box(l, (short)t, r, (short)b, (COLOR)0x00191919);
    const int fw = (int)std::lround((double)(r - l) * qm::fillFrac(T));
    if (fw > 0) box(l, (short)t, (short)(l + fw), (short)b, mix(side, 0.45f));
    const int call = qm::outlineCall(T);   // (2.21.0) the outline follows the 10-s call: red / green 2 px; none = yellow 2 px (originals gone) or grey 1 px
    const COLOR oc = call == 0 ? C_SELL : call == 1 ? C_BUY : T.warn ? S.qmWarn : C_MUTED;
    if (call >= 0 || T.warn) { setPen(oc, 2, P_SOLID); PNT q; q.set(0, 0.0f); q.h = (short)(l + 1); q.v = (short)(t + 1); q.setDrawPosition(); q.h = (short)(r - 1); q.drawLineTo(); q.v = (short)(b - 1); q.drawLineTo(); q.h = (short)(l + 1); q.drawLineTo(); q.v = (short)(t + 1); q.drawLineTo(); }
    else { line(l, (short)t, r, (short)t, oc, 1); line(r, (short)t, r, (short)b, oc, 1); line(r, (short)b, l, (short)b, oc, 1); line(l, (short)b, l, (short)t, oc, 1); }
    textLJ((short)(l + 4), (short)yc, tx.c_str(), ink, fz, true);
    HoverT H; H.l = l; H.r = r; H.t = (short)t; H.b = (short)b; H.key = "Q|" + key; H.qm = true;
    const qm::Hover qh = qm::hover(T, name, pxTxt, mkt); H.h.title = qh.title; H.h.lines = qh.lines;
    hovT.push_back(H);
    return true;
}
bool DealerProfile::levelLiq(PendLbl& P, double chartPx)
{
    P.haveLiq = false; P.hl = false; P.lots = 0; P.bx = -1;
    if (!cfg.liqBadge) return false;
    loadLiqLevels();
    const dcol::LiqRow* r = dcol::liqRowAt(cs.liqL, chartPx);
    dcol::HLState& h = cs.hl[P.key];
    const bool have = r && r->n >= 1;
    P.hl = dcol::hlStep(h, have, have && r->bid, have ? r->bx : -1, cs.liqL.onX, cs.liqL.offX, nowMsC());
    if (!have) return false;
    P.haveLiq = true; P.bid = r->bid; P.lots = r->n; P.bx = r->bx;
    char b[96];
    if (r->bx > 0) snprintf(b, sizeof(b), "%lld contracts resting %s (%.1fx the book behind)", r->n, r->bid ? "bids" : "offers", r->bx);
    else snprintf(b, sizeof(b), "%lld contracts resting %s (no book behind to compare)", r->n, r->bid ? "bids" : "offers");
    P.liq = b;
    return P.hl;
}

// (2.11.1) lsFlexLevels\LRA-MQFeed.txt (bridge 1.30.0): re-read only when its stamp changes, at most every 5 s; a torn / garbled /
// oversized file keeps the last good copy; its freshness (written in the last 5 min) is checked at use, so a stopped bridge shows nothing
void DealerProfile::loadFeed()
{
    const time_t now = time(nullptr);
    if (now - feedChecked < 5 && feedStamp != -2) return;
    feedChecked = now;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    const std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-MQFeed.txt";
    const long long st = dl::fileStamp(p);
    if (st == feedStamp) return;
    if (st < 0) { feed = dpaudit::FeedStat(); feedStamp = st; return; }
    std::vector<std::string> lines;
    if (dpaudit::readBoundedLines(p, lines, 4096, 16, 512) != dpaudit::READ_OK || dl::fileStamp(p) != st) return;
    const dpaudit::FeedStat f = dpaudit::parseFeed(lines);
    if (f.ok) feed = f;
    feedStamp = st;
}

void DealerProfile::loadLevels(const std::string& m)
{
    time_t now = time(nullptr);
    if (now - lvChecked[m] < 5 && lvStamp.count(m)) return;
    lvChecked[m] = now;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Levels-" + m + ".csv";
    long long st = dl::fileStamp(p);
    if (lvStamp.count(m) && st == lvStamp[m]) return;
    if (st < 0) { lvs[m].clear(); lvStamp[m] = st; lvStale[m] = false; lvAt[m] = 0; return; }
    std::vector<std::string> lines;
    if (dpaudit::readBoundedLines(p, lines) != dpaudit::READ_OK || dl::fileStamp(p) != st) return;
    std::vector<LV> v; std::vector<float> picks; bool stale = false;
    for (size_t row = 0; row < lines.size(); ++row) {
        const std::string& ln = lines[row];
        std::vector<std::string> c = dl::split(ln, '|');
        if (c.size() >= 2 && c[0] == "STATE" && c[1] == "STALE") stale = true;
        if (c.size() >= 13 && c[0] == "LEVEL") { LV q; if (!dpaudit::parsePositiveFinite(c[3], q.px)) continue; q.code = c[1]; q.show = c[2]; q.fam = c[4]; q.label = c[12]; v.push_back(q); }
        if (c.size() >= 3 && c[0] == "PICK") { float pp = 0; if (dpaudit::parsePositiveFinite(c[2], pp)) picks.push_back(pp); }
    }
    const double tick = chartTick(m);
    for (size_t i = 0; i < picks.size(); ++i) for (size_t j = 0; j < v.size(); ++j)
        if (dpaudit::sameTick(v[j].px, picks[i], tick)) v[j].pick = true;
    lvs[m].swap(v); lvStamp[m] = st; lvStale[m] = stale; lvAt[m] = st > 0 ? (time_t)(st / 1000003LL) : 0;
}

const DealerProfile::LV* DealerProfile::levelAt(const std::string& m, float chartPx)
{
    loadLevels(m);
    if (lvStale[m] || lvAt[m] <= 0 || time(nullptr) - lvAt[m] > 20 * 60) return nullptr;     // stale odds are never shown
    const double tick = chartTick(m);
    for (const LV& q : lvs[m]) if (dpaudit::sameTick(q.px + off, chartPx, tick) || dpaudit::sameTick(q.px, chartPx, tick)) return &q;
    return nullptr;
}

static double tickOfMkt(const std::string& m)
{
    return m == "CL" ? 0.01 : m == "NG" ? 0.001 : m == "GC" ? 0.1 : m == "HG" ? 0.0005 : m == "EU" ? 0.00005 : 0.25;
}

double DealerProfile::chartTick(const std::string& m)
{
    const double tick = getTickIncrement();                    // verified in supplied IRT SDK; do not infer from decimal formatting
    return dpaudit::finite(tick) && tick > 0.0 ? tick : tickOfMkt(m);
}

// (2.18.0) one closed bar's volume-at-price, read the way the Delta Profile reads it (any failure = no rows for that bar)
static int dpVapStats(cppExtension::RTARRAYP* vap, int bar, cppExtension::BARSTATISTICS* st)
{
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || !st || bar < 0) return -2;
        std::memset(st, 0, sizeof(*st));
        if (vap->getBarStatistics(bar, *st) != RTX_OK) return -2;
        return st->prices < 0 ? -2 : st->prices;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
static int dpVapRows(cppExtension::RTARRAYP* vap, int bar, cppExtension::VOLPROFILE* rows, int count)
{
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
static bool dpReadVap(cppExtension::RTARRAYP* vap, int bar, std::vector<klv::Vap>& out)
{
    out.clear();
    cppExtension::BARSTATISTICS st; const int n = dpVapStats(vap, bar, &st);
    if (n < 0 || n > 4000) return false;
    std::vector<cppExtension::VOLPROFILE> rows((size_t)n);
    if (n > 0 && dpVapRows(vap, bar, &rows[0], n) != n) return false;
    for (int i = 0; i < n; ++i) if (std::isfinite(rows[(size_t)i].price) && rows[(size_t)i].totalVolume > 0) out.push_back(klv::Vap{ (double)rows[(size_t)i].price, (double)rows[(size_t)i].totalVolume });
    return true;
}

const std::vector<klv::Merged>& DealerProfile::nativeLevels(const std::string& m)
{
    KLC& K = klc[m];
    long n = getBarCount();
    if (n < 2) { K.v.clear(); return K.v; }
    RTARRAY h(barHigh), l(barLow), c(barClose), op(barOpen); RTARRAYI dt(barDateTime), bv(barVolume);
    long long lastT = (long long)dt[(int)n - 1];
    time_t now = time(nullptr);
    if (n == K.n && lastT == K.lastT && now - K.at < 60) return K.v;
    if (now - K.at < 10 && K.n >= 0) return K.v;
    int per = 0;
    for (long i = n - 1; i > 0 && i > n - 40; i--) { long long d = (long long)dt[(int)i] - (long long)dt[(int)i - 1]; if (d > 0 && (per == 0 || d < per)) per = (int)d; }
    if (per <= 0 || per > 86400) per = 180;
    if (per > 900) {   // (2.8.0) an hourly (15-min+) chart: the levels a 3-min chart of this market computed - never its own coarse bars
        K.v.clear(); K.n = n; K.lastT = lastT; K.at = now;
        const char* up = getenv("USERPROFILE"); if (!up) return K.v;
        std::ifstream f((std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-KeyLevels-" + m + ".csv").c_str(), std::ios::binary);
        std::string text; if (f.is_open()) { text.resize(65536); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); }
        std::vector<dpaudit::KeyLv> kv;
        if (dpaudit::parseKeyLevels(text, (long long)now, kv)) for (size_t i = 0; i < kv.size(); i++) {
            const std::string nm = dcol::dropHourly(kv[i].name);         // (2.11.0) an older file may still carry HrHI / HrLO
            if (!nm.empty()) K.v.push_back(klv::Merged{ nm, kv[i].price, kv[i].pivot });
        }
        return K.v;
    }
    long from = n - (long)(15LL * 86400 / per); if (from < 0) from = 0;     // two weeks and a bit: enough for the prior week
    std::vector<klv::Bar> B; B.reserve((size_t)(n - from));
    // (2.18.0) the volume-at-price of each closed bar, read once and kept (the forming bar is never used: the profiles are
    // the PRIOR session / week, so their bars are all closed); a bar IRT has no rows for yet is retried at the next recompute
    if (!K.vap) K.vap = std::make_shared<VapCache>();
    std::vector<std::vector<klv::Vap>> V; V.reserve((size_t)(n - from));
    RTARRAYP vapArr(barVolumeProfile);
    K.vapRead = 0; K.vapMissing = 0;
    const long long firstT = (long long)dt[(int)from];
    for (auto it = K.vap->bars.begin(); it != K.vap->bars.end(); ) { if (it->first < firstT) it = K.vap->bars.erase(it); else ++it; }   // trim to the window
    for (auto it = K.vap->bad.begin(); it != K.vap->bad.end(); ) { if (it->first < firstT) it = K.vap->bad.erase(it); else ++it; }
    for (long i = from; i < n; i++) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[(int)i], &t);
        klv::Bar b{ svl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday), t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec, h[(int)i], l[(int)i], c[(int)i] };
        b.o = op.count > (int)i ? op[(int)i] : NAN;
        b.v = bv.count > (int)i ? (double)bv[(int)i] : -1;
        B.push_back(b);
        const long long key = (long long)dt[(int)i];
        auto f = K.vap->bars.find(key);
        if (f != K.vap->bars.end()) { V.push_back(f->second); continue; }
        std::vector<klv::Vap> rows;
        if (i < n - 1) {
            std::map<long long, time_t>::iterator bd = K.vap->bad.find(key);
            if (bd != K.vap->bad.end() && now - bd->second < 600) { K.vapMissing++; V.push_back(rows); continue; }   // (2.18.1) known gap: not re-read yet
            if (dpReadVap(&vapArr, (int)i, rows) && !(rows.empty() && b.v > 0)) { K.vap->bars[key] = rows; K.vapRead++; if (bd != K.vap->bad.end()) K.vap->bad.erase(bd); }
            else { K.vapMissing++; K.vap->bad[key] = now; rows.clear(); }
        }
        V.push_back(rows);
    }
    int o = 0, cl = 0;
    if (!svl::rthOf(m.c_str(), o, cl)) { K.v.clear(); return K.v; }
    double tk = chartTick(m);
    int a0 = 18 * 60, a1 = 2 * 60; klv::asiaWindow(m.c_str(), a0, a1);              // (2.10.0)
    K.diag = klv::ProfDiag();
    K.v = klv::merge(klv::compute(B, per, o, cl, tk, a0, a1, &V, klv::profileRowTicks(m), &K.diag), tk, klv::mergeTicksFor(m));   // (2.18.0) + Open, DPoc / VAH / VAL, WPoc
    {   std::vector<klv::Merged> keep; for (size_t i = 0; i < K.v.size(); ++i) { klv::Merged q = K.v[i]; q.name = dcol::dropHourly(q.name); if (!q.name.empty()) keep.push_back(q); } K.v.swap(keep); }
    K.n = n; K.lastT = lastT; K.at = now;
    {   // (2.8.0) share them with this market's hourly charts (written at each recompute: every new bar, at most every 10 s)
        std::vector<dpaudit::KeyLv> kv; for (size_t i = 0; i < K.v.size(); i++) kv.push_back(dpaudit::KeyLv{ K.v[i].name, K.v[i].price, K.v[i].pivotOnly });
        const char* up = getenv("USERPROFILE");
        if (up && !kv.empty()) {
            const std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-KeyLevels-" + m + ".csv";
            const std::string tmp = dpaudit::exportTempPath(path, reinterpret_cast<std::uintptr_t>(getUserData()));
            bool ok = false; { std::ofstream f(tmp.c_str(), std::ios::trunc | std::ios::binary); if (f.is_open()) { f << dpaudit::keyLevelsText(kv, (long long)now); ok = (bool)f; } }
            if (!ok || !dpReplaceFile(tmp, path)) std::remove(tmp.c_str());
        }
    }
    return K.v;
}

// (2.7.1, Rassul 2026-10-09 09:12) key-level colours from his SessionPrices settings: Prev Full = teal, Prev Day = orange,
// Prev Week = blue, Overnight = grey; London keeps amber; (2.10.0) Asia muted purple, current week soft blue (hourly swings gone). A merged label takes its first name's colour.
static COLOR klColor(const std::string& name, bool pivotOnly)
{
    if (pivotOnly) return (COLOR)0x00C084FC;
    std::string f = name.substr(0, name.find('/'));
    if (f == "PFH" || f == "PFL") return (COLOR)0x00008A8A;
    if (f == "PDH" || f == "PDL") return (COLOR)0x00FFA500;
    if (f == "PWH" || f == "PWL") return (COLOR)0x000000FF;
    if (f == "ONH" || f == "ONL") return (COLOR)0x00808080;
    if (f == "AsiaHI" || f == "AsiaLO") return (COLOR)0x008E6CC0;   // (2.10.0) Asia = muted purple
    if (f == "CWH" || f == "CWL") return (COLOR)0x005470D6;         // (2.10.0) current week = a softer Prev Week blue
    if (f == "Open") return (COLOR)0x00FFFFFF;                      // (2.18.0, Rassul 2026-10-19 13:14 "make its badge and line white")
    if (f == "VAH" || f == "VAL") return (COLOR)0x00EF4444;          // (2.18.2, Rassul 2026-10-19 18:51 "make VAH and VAL red and DPOC badge cyan and WPoc beige")
    if (f == "DPoc") return (COLOR)0x0022D3EE;                      // (2.18.2) cyan
    if (f == "WPoc") return (COLOR)0x00E8D5B0;                      // (2.18.2) beige
    return (COLOR)0x00F59E0B;
}
// "33%/50%?" -> "33%  /  50%?" (two spaces each side of the slash between the odds; names are not touched)
static std::string spacedOdds(const std::string& o)
{
    std::string r; r.reserve(o.size() + 8);
    for (size_t i = 0; i < o.size(); i++) {
        if (o[i] == '/' && r.find_last_not_of(' ') != std::string::npos && r[r.find_last_not_of(' ')] == '%') { while (!r.empty() && r.back() == ' ') r.pop_back(); r += "  /  "; while (i + 1 < o.size() && o[i + 1] == ' ') i++; }
        else r += o[i];
    }
    return r;
}

// (2.7.3, Rassul 2026-10-11 19:41 "the levels labels are over the delta profile.. fix this .. for all markets . maybe you can put
// the levels in the center") labels centred; 2.7.4: dpaudit::placeLabel also keeps them off the price bars (DealerProfileAuditLogic.h).

// (2.13.0, Rassul 2026-10-15 09:49 "to cut the clutter of lines, we cap the levels to their column so they dont go across the chart")
// a level line runs only across the Levels column (its badge sits on it); no column laid out = the old full-width line
void DealerProfile::levelSpan(const RCT& pane, short& x0, short& x1)
{
    const bp::Box& c = cs.lay.levels;
    if (cs.lay.ok && c.r > c.l) { x0 = (short)c.l; x1 = (short)c.r; } else { x0 = pane.left; x1 = pane.right; }
}
void DealerProfile::drawKeyLevels(const Settings& S)
{
    if (!mkt.empty()) loadLiq(mkt);                                   // (2.8.0) the liquidity on each level
    // (2.7.0, Rassul 2026-10-09 08:40 "it would probably be better if the dealer profile did all the levels and their probabilities")
    // the key levels (PFH / PFL, PDH / PDL, PWH / PWL, ONH / ONL, LonHI / LonLO) and the latest standing hourly swings (HrHI / HrLO)
    // are computed HERE from the chart's own bars (KeyLevelsLogic.h - lra.key_levels' definitions): a thin line + the label at the
    // right, amber = key level, purple = hourly swing, white box = a pick. The odds: the LRA's champion law while its file is fresh
    // ("42%/60%" = within 90 min / by the RTH close), otherwise the native 60-min odds ("1h 45%?"). The VWAP is never drawn here.
    (void)S;
    if (mkt.empty()) return;
    const std::vector<klv::Merged>& L = nativeLevels(mkt);
    if (L.empty()) return;
    RCT pane; pane.getPaneRect(false);
    long nb = getBarCount(); if (nb < 1) return;
    const otl::OT op = otl::paramsFor(mkt.c_str());
    double onSig = 0, lastC = 0;
    {
        RTARRAY cl(barClose); RTARRAYI dt(barDateTime);
        if (nb > op.win + 1) {
            double cw[64]; int k = 0;
            for (long i = nb - op.win - 1; i < nb; i++) cw[k++] = (double)cl[(int)i];
            long long per = (long long)dt[(int)nb - 1] - (long long)dt[(int)nb - 2];
            if (per <= 0 || per > 3600) per = 180;
            onSig = otl::onSigmaMin(cw, k, (int)per, op.win); lastC = (double)cl[(int)nb - 1];
        }
    }
    if (lastC <= 0) { RTARRAY cl(barClose); const double c0 = (double)cl[(int)nb - 1]; if (dpFinite(c0) && c0 > 0) lastC = c0; }   // (2.11.0) above / below price for the hover
    for (const klv::Merged& q0 : L) {
        klv::Merged q = q0; q.name = dcol::dropHourly(q0.name);       // (2.11.0) never an hourly swing (removed in 2.10.0)
        if (q.name.empty()) continue;
        PNT pp; pp.set((int)(nb - 1), (float)q.price); short y = pp.v;
        if (y <= pane.top || y >= pane.bottom) {   // (2.28.0) the nearest level off screen above / below
            if (DP_LVC && lastC > 0) { OffLvl& o = q.price > lastC ? offUp : offDn; const double d = q.price - lastC;
                if (!o.have || std::fabs(d) < std::fabs(o.dist)) { o.have = true; o.name = dcol::compactName(klv::displayName(q.name)); o.px = q.price; o.dist = d; } }
            continue;
        }
        if (lastC > 0) lastPxLv = lastC;
        COLOR col = klColor(q.name, q.pivotOnly);                      // (2.7.1) his SessionPrices colours
        PendLbl P; P.key = "K|" + q.name;
        const bool hl = levelLiq(P, q.price);                         // (11:39 CT) high liquidity: the line 3 px across the chart
        (void)hl; (void)col;   // (2.25.0, Rassul 2026-10-24 21:05 "get rid of ... the extra line after the probability") no level line - the badge alone
        std::string odds; bool pick = false;
        const LV* lv = levelAt(mkt, (float)q.price);                // the LRA's champion odds (fresh file only)
        if (lv && !lv->label.empty()) { odds = lv->label; pick = lv->pick; }
        else if (onSig > 0 && lastC > 0) {
            double pr = otl::onTouch(q.price - lastC, onSig, 60, op.nu, op.k);
            if (pr >= 0) { char a[24]; snprintf(a, sizeof(a), op.ready ? "1h %d%%" : "1h %d%%?", (int)std::floor(100.0 * pr + 0.5)); odds = a; }
        }
        odds = spacedOdds(odds);                                      // (2.7.1) "33%  /  50%"
        // (2.7.4, Rassul 2026-10-12: the key-level badges had no price) "<price> <NAME>  <odds>", as the MenthorQ badges
        P.y = y; P.border = pick ? (COLOR)0x00FFFFFF : col;
        const dpaudit::Badge B = dpaudit::badge(mkt, q.price, klv::displayName(q.name), odds);   // (2.10.0) "ONH \xB7 AsiaHI"
        P.seg.push_back(LSeg{ B.price, false, C_INK });
        P.seg.push_back(LSeg{ B.name, true, (COLOR)dpaudit::liftForText(col) });   // his SessionPrices colour (lifted when too dark to read)
        if (!B.odds.empty()) P.seg.push_back(LSeg{ B.odds, false, C_INK });
        // (2.11.0) the compact badge + what its hover says (price, odds, liquidity) - the segments above are no longer drawn
        P.name0 = q.name; P.priceTxt = dpaudit::priceText(mkt, q.price); P.odds = odds; P.col = col; P.pick = pick; P.px = q.price;
        P.above = lastC > 0 && q.price > lastC;
        pend.push_back(P);
    }
}

void DealerProfile::drawMQLevels(const Settings& S)
{
    loadMQLevels();
    if (mql.empty() || mkt.empty()) return;
    loadLiq(mkt);                                                     // (2.8.0)
    (void)S;
    // (2.7.4) the chart's MenthorQ contract: its own symbol, the same contract under another feed's name (ESZ6 = EPZ26), or -
    // on a roll / continuous chart - the dealer file's contract, else the nearest expiry (never the back month by alphabet).
    // Levels of ANOTHER contract than the chart's are drawn at strike + the contract offset (before: at the raw strike).
    std::vector<std::string> keys; for (auto& kv : mql) keys.push_back(kv.first);
    bool shift = false;
    const std::string sym = dpaudit::pickMQSymbol(keys, root, mkt, D.chartSym, shift);
    if (sym.empty()) return;
    const std::vector<MQL>* L = &mql[sym];
    const float sh = shift ? off : 0.0f;
    RCT pane; pane.getPaneRect(false);
    long nb = getBarCount(); if (nb < 1) return;
    // (2.6.0, Rassul 2026-10-09 07:22 "i also dont see the percentages on the levels like CR") outside RTH the LRA odds file is not
    // written (its law is fitted on RTH), so the chart computes the next-60-min touch odds itself: sigma from the last 20 closes
    double onSig = 0, lastC = 0;
    const otl::OT op = otl::paramsFor(mkt.c_str());                   // the nightly champion (OnTouchParams.h)
    {
        RTARRAY cl(barClose); RTARRAYI dt(barDateTime);
        if (nb > op.win + 1) {
            double cw[64]; int k = 0;
            for (long i = nb - op.win - 1; i < nb; i++) cw[k++] = (double)cl[(int)i];
            long long per = (long long)dt[(int)nb - 1] - (long long)dt[(int)nb - 2];
            if (per <= 0 || per > 3600) per = 180;
            onSig = otl::onSigmaMin(cw, k, (int)per, op.win); lastC = (double)cl[(int)nb - 1];
        }
    }
    if (lastC <= 0) { RTARRAY cl(barClose); const double c0 = (double)cl[(int)nb - 1]; if (dpFinite(c0) && c0 > 0) lastC = c0; }   // (2.11.0) above / below price for the hover
    // (2.9.0 clean-up) the key levels' prices on this chart: a G2-G10 line shows only when it sits on one
    keyPx.clear();
    { const std::vector<klv::Merged>& K = nativeLevels(mkt); for (size_t k = 0; k < K.size(); k++) keyPx.push_back(K[k].price); }
    const double tickK = chartTick(mkt);
    for (size_t i = 0; i < L->size(); i++) {
        const MQL& q = (*L)[i];
        const float cp = q.px + sh;                                           // where it is on THIS chart
        const std::string nm0 = dpaudit::stripMQ(q.label);
        // (2.9.0) the HVL = the regime line (its tags are drawn with the Profile), found even when off screen
        if (!hvlFound && dpaudit::mqKind(q.label) == 'Y') { hvlFound = true; hvlChartPx = cp; }
        if (!DP_LVC && dpaudit::mqHiddenGex(nm0) && !dpaudit::onKeyLevel((double)cp, keyPx, tickK)) continue;   // (2.9.0) G2-G10 hidden off key levels; (2.28.0) every gamma level in the combined column
        const std::string nm = dpaudit::mqShowName(nm0);                       // (2.9.0) "CR 0D / GW 0D" -> "CR0"
        PNT pp; pp.set((int)(nb - 1), cp); short y = pp.v;
        if (y <= pane.top || y >= pane.bottom) {   // (2.28.0) the nearest level off screen above / below
            if (DP_LVC && DP_GAMMA && lastC > 0) { OffLvl& o = cp > lastC ? offUp : offDn; const double d = (double)cp - lastC;
                if (!o.have || std::fabs(d) < std::fabs(o.dist)) { o.have = true; o.name = dcol::compactName(nm); o.px = (double)cp; o.dist = d; } }
            continue;
        }
        if (lastC > 0) lastPxLv = lastC;
        // (2.7.4) green = support (PS), red = resistance (CR), yellow = HVL, cyan = the other gamma levels
        const char kind = dpaudit::mqKind(q.label);
        const COLOR col = kind == 'G' ? C_BUY : kind == 'R' ? C_SELL : kind == 'Y' ? (COLOR)0x00FACC15 : (COLOR)0x0022D3EE;
        PendLbl P; P.key = "M|" + q.label;
        const bool hl = levelLiq(P, (double)cp);                              // (11:39 CT) high liquidity: the line 3 px across the chart
        (void)hl;   // (2.25.0) no level line (gamma levels too) - the badge alone
        std::string odds;
        // (2.4.0, Rassul 2026-10-07 12:57 "start showing it now on the levels") the chance price touches this level: in the next
        // hour, by today's 0DTE expiry (when it comes before the close) and by the close; "?" until the market's numbers are proven
        const LV* lv = levelAt(mkt, cp);                                      // (2.5.0) the one-law odds first
        if (lv && !lv->label.empty()) odds = spacedOdds(lv->label);
        else {
            loadTouch(mkt);
            bool fresh = tpAt[mkt] > 0 && time(nullptr) - tpAt[mkt] < 20 * 60;    // stale odds (the bridge stopped) are not shown
            const std::vector<TP>& T = tps[mkt];
            const double tick = chartTick(mkt);   // (2.8.7, audit DP-002) the tick grid, not a relative tolerance
            for (size_t k = 0; fresh && k < T.size(); k++) {
                if (!dpaudit::sameTick(T[k].px + off, cp, tick) && !dpaudit::sameTick(T[k].px, cp, tick)) continue;
                const char* qm = T[k].proven ? "" : "?";
                if (!T[k].txt.empty()) { odds = T[k].txt; break; }                                  // (2.4.1)
                odds = "1h " + T[k].p1 + "%" + qm;
                if (!T[k].pe.empty() && T[k].pe != T[k].pc) odds += "  exp " + T[k].pe + "%" + qm;
                if (!T[k].pc.empty()) odds += "  close " + T[k].pc + "%" + qm;
                break;
            }
            // nothing fresh from the LRA (overnight): the native 60-min odds, "?" until lra.on_vwap_study proves them
            if (odds.find('%') == std::string::npos && onSig > 0 && lastC > 0) {
                double pr = otl::onTouch((double)cp - lastC, onSig, 60, op.nu, op.k);
                if (pr >= 0) { char a[24]; snprintf(a, sizeof(a), op.ready ? "1h %d%%" : "1h %d%%?", (int)std::floor(100.0 * pr + 0.5)); odds = a; }
            }
        }
        P.y = y; P.border = (lv && lv->pick) ? (COLOR)0x00FFFFFF : col;   // (2.5.0) a pick: white box
        const dpaudit::Badge B = dpaudit::badge(mkt, q.px, nm, odds);   // the strike, as MenthorQ writes it (2.9.0: CR0 merged name)
        P.seg.push_back(LSeg{ B.price, false, C_INK });
        P.seg.push_back(LSeg{ B.name, true, col });
        if (!B.odds.empty()) P.seg.push_back(LSeg{ B.odds, false, C_INK });
        // (2.11.0) the compact badge ("CR0", "HVL"); price, odds, liquidity and STALE / age go to its hover
        P.name0 = nm; P.priceTxt = dpaudit::priceText(mkt, q.px); P.odds = odds; P.col = col; P.pick = lv && lv->pick; P.px = (double)cp;
        P.above = lastC > 0 && cp > lastC;
        P.stale = nm.find(" STALE") != std::string::npos; P.hvl = kind == 'Y'; P.src = 1;   // (2.28.0) a gamma level
        pend.push_back(P);
    }
}

// (2.28.0) the order lines lsLiquidityProfile 0.60.0 tagged to a level (this chart's market and bar size): re-read when the stamp
// changes (checked at most every 500 ms), bounded, torn-safe; stale (> 20 s) / missing = no liquidity (the reason on LEVELS_COLUMN)
void DealerProfile::loadLmt()
{
    const long long now = nowMsC();
    if (cs.lmtChecked >= 0 && now >= cs.lmtChecked && now - cs.lmtChecked < 500) return;
    cs.lmtChecked = now;
    const char* up = getenv("USERPROFILE"); if (!up || mkt.empty() || cs.spb <= 0) { cs.lmtO.clear(); cs.lmtWhy = "no market"; return; }
    const std::string p = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-LmtLevels-" + mkt + "-" + std::to_string(cs.spb) + ".txt";
    const long long st = dl::fileStamp(p);
    if (st < 0) { cs.lmtO.clear(); cs.lmtStamp = st; cs.lmtWhy = "no file (lsLiquidityProfile 0.60.0 not on this chart)"; return; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (f.is_open()) { text.resize(65536); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); } }
    if (dl::fileStamp(p) != st) return;                              // torn read: next time
    std::vector<lvc::Order> o; std::string why;
    if (lvc::parseOrders(text, (long long)time(nullptr), o, why)) { cs.lmtO.swap(o); cs.lmtWhy.clear(); } else { cs.lmtO.clear(); cs.lmtWhy = why; }
    cs.lmtStamp = st;
}
double DealerProfile::atr14()
{   // the chart's 14-candle average true range (the merge tolerance; 0 when the chart is too short)
    const long nb = getBarCount();
    if (nb < 16) return 0;
    RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    double s = 0; int n = 0;
    for (long i = nb - 14; i < nb; ++i) {
        const double h = hi[(int)i], l = lo[(int)i], pc = cl[(int)i - 1];
        if (!(h >= l) || !std::isfinite(pc)) continue;
        s += std::max(h - l, std::max(std::fabs(h - pc), std::fabs(l - pc))); ++n;
    }
    return n ? s / n : 0;
}
// (2.28.0) the 3 biggest gamma strikes of the grid (absolute GEX) that are not already a named level within the merge tolerance:
// a violet "GEX" row (gamma family) - the Dealer bars are off, so a big wall with no MenthorQ name stays visible
void DealerProfile::addGexRows()
{
    cs.lvcGex = 0;
    if (nx.empty() || mkt.empty()) return;
    std::vector<size_t> ord; for (size_t i = 0; i < nx.size(); ++i) if (nx[i].absG > 0) ord.push_back(i);
    std::sort(ord.begin(), ord.end(), [&](size_t a, size_t b) { return nx[a].absG > nx[b].absG; });
    if (ord.size() > 3) ord.resize(3);
    const double tol = lvc::mergeTol(chartTick(mkt), atr14());
    RCT pane; pane.getPaneRect(false);
    const long nb = getBarCount(); if (nb < 1) return;
    for (size_t k : ord) {
        const double cp = (double)nx[k].k + off;
        bool named = false; for (const PendLbl& P : pend) if (std::isfinite(P.px) && std::fabs(P.px - cp) <= tol) named = true;
        if (named) continue;
        PNT pp; pp.set((int)(nb - 1), (float)cp); const short y = pp.v;
        if (y <= pane.top || y >= pane.bottom) continue;
        PendLbl P; P.key = "X|" + std::to_string(std::llround(cp * 10000.0)); P.y = y; P.border = 0x00A78BFA; P.col = 0x00A78BFA;
        P.name0 = "GEX"; P.priceTxt = dpaudit::priceText(mkt, cp); P.px = cp; P.src = 2;
        pend.push_back(P); ++cs.lvcGex;
    }
}
// (2.28.0, Rassul approved 13:30-13:39 CT, mockup levels_final.png) THE COMBINED LEVELS + LIQUIDITY COLUMN. Top: "DP v2.28.0" (+ STALE),
// the nearest level ABOVE the screen, the column titles; then one row per price - [rank] [liquidity bar] [chips: key, volume, gamma]
// [price] [% block] - stacked in price order without overlap (cb::stack), a short tick + leader from the true price when a row had to
// move; bottom: the nearest level BELOW the screen. Nothing outside the column; no hovers.
void DealerProfile::flushColumn(const Settings& S)
{
    cs.badgesShown = cs.badgesDropped = 0; cs.lvcRows = cs.lvcLiq = cs.lvcMerged = cs.lvcChipsCut = 0;
    const bp::Box& col = cs.lay.levels;
    if (col.w() <= 0 || !cs.lay.ok) { cs.badgesDropped = (int)pend.size(); pend.clear(); return; }
    RCT pane; pane.getPaneRect(false);
    const long nb = getBarCount();
    const int kf = S.badgeFont, sf = std::max(8, kf - 1);
    const int bh = dcol::textPx(kf) + 6;
    const COLOR C_COL = 0x000B0F19, C_ROW = 0x00111827, C_ROWLQ = 0x00273449, C_HEAD = 0x0064748B, C_VIOLET = 0x00A78BFA, C_AMBER = 0x00FBBF24, C_GREYC = 0x0094A3B8;
    box(col.l, pane.top, col.r, pane.bottom, C_COL);                       // the column's own dark ground
    // header: version + STALE (the Dealer column is gone, so its header lives here)
    int y0 = pane.top + 4;
    {   double age = -1; const bool stale = !nx.empty() && gridVerdict(S.clock, age).stale();
        const std::string h1 = std::string("DP v") + DP_VERSION;
        textLJ((short)(col.l + 6), (short)(y0 + bh / 2), h1.c_str(), C_MUTED, sf, true);
        if (stale) {
            char b[40]; if (age >= 2880) snprintf(b, sizeof(b), "STALE %dd", (int)(age / 1440 + 0.5)); else if (age >= 90) snprintf(b, sizeof(b), "STALE %dh", (int)(age / 60 + 0.5)); else snprintf(b, sizeof(b), "STALE %dm", (int)(age + 0.5));
            const int tw = textW(b, sf, true); RCT bg; bg.set((short)(col.r - tw - 14), (short)y0, (short)(col.r - 4), (short)(y0 + bh));
            bg.draw(1, 0x00C0392B, 0x003A1416, DRAW_OPAQUE, PAT_SOLID);
            textRJ((short)(col.r - 9), (short)(y0 + bh / 2), b, 0x00FF9A8F, sf, true);
        }
        y0 += bh + 2;
    }
    const int dec = std::max(0, std::min(6, (int)std::lround(-std::log10(std::max(1e-6, chartTick(mkt))) + 0.49)));
    auto offLine = [&](const OffLvl& o, int yc, bool up) {
        if (!o.have) return;
        const std::string t = lvc::offText(o.name, dpaudit::priceText(mkt, o.px), o.dist, dec);
        const int tw = textW(t.c_str(), sf, false), x = std::max<int>(col.l + 18, (col.l + col.r) / 2 - (tw + 12) / 2);
        for (int k = 0; k < 4; ++k) { const short yy = (short)(up ? yc - 2 + k : yc + 2 - k); line((short)(x + 3 - k), yy, (short)(x + 3 + k), yy, C_GREYC, 1); }   // a small triangle
        textLJ((short)(x + 10), (short)yc, t.c_str(), C_GREYC, sf, false);
    };
    offLine(offUp, y0 + bh / 2, true); y0 += bh;
    // column titles
    const int RK = col.l + 28, LQX = col.l + 40, LQW = 96, CHX = LQX + LQW + 8;
    const int ODR = col.r - 6, ODW = textW("100%  /  100%", kf, true) + 8, PRR = ODR - ODW - 8;
    textLJ((short)(RK - 3), (short)(y0 + bh / 2), "#", C_HEAD, sf, true);
    textLJ((short)LQX, (short)(y0 + bh / 2), "LIQUIDITY", C_HEAD, sf, true);
    textLJ((short)CHX, (short)(y0 + bh / 2), "LEVEL", C_HEAD, sf, true);
    textRJ((short)PRR, (short)(y0 + bh / 2), "PRICE", C_HEAD, sf, true);
    textRJ((short)ODR, (short)(y0 + bh / 2), "ODDS", C_HEAD, sf, true);
    y0 += bh + 2;
    line((short)(col.l + 8), (short)(y0 - 1), (short)(col.r - 6), (short)(y0 - 1), 0x001E293B, 1);
    const int bottomRows = pane.bottom - 2 - (offDn.have ? bh + 2 : 0);
    if (pend.empty()) { if (offDn.have) offLine(offDn, pane.bottom - 2 - bh / 2, false); return; }
    // rows: group, liquidity, rank
    const double tick = chartTick(mkt), atr = atr14();
    std::vector<lvc::In> in;
    for (size_t i = 0; i < pend.size(); ++i) {
        const PendLbl& P = pend[i]; if (!std::isfinite(P.px)) continue;
        lvc::In x; x.px = P.px; x.name = P.src == 0 ? dcol::compactName(klv::displayName(P.name0)) : dcol::compactName(P.name0); x.src = P.src;
        x.col = (unsigned long)P.col; x.pick = P.pick; x.stale = P.stale; x.idx = i; in.push_back(x);
    }
    std::vector<lvc::Row> rows = lvc::group(in, tick, atr);
    loadLmt();
    double lastPx = lastPxLv;
    if (!(lastPx > 0) && nb > 0) { RTARRAY cl(barClose); lastPx = (double)cl[(int)nb - 1]; }
    lvc::attach(rows, in, cs.lmtO, tick, atr, lastPx);
    std::vector<cb::Item> items;
    for (size_t r = 0; r < rows.size(); ++r) { PNT p; p.set((int)nb - 1, (float)rows[r].px); cb::Item it; it.want = p.v; it.h = bh + 2; items.push_back(it); if (rows[r].members.size() > 1) ++cs.lvcMerged; }
    int focusY = (pane.top + pane.bottom) / 2; if (lastPx > 0) { PNT p; p.set((int)nb - 1, (float)lastPx); focusY = p.v; }
    cs.badgesDropped += cb::stack(items, y0 + 1, bottomRows, focusY);
    std::vector<size_t> shown; for (size_t r = 0; r < rows.size(); ++r) if (items[r].shown) shown.push_back(r);
    lvc::rank(rows, shown, lastPx);
    long long maxLots = 0; for (size_t r : shown) maxLots = std::max(maxLots, rows[r].lots);
    // the % block's side (the RTH open; before it the last price)
    double rthOpen = NAN;
    if (S.levelPct) { loadLevelPct(); const std::vector<klv::Merged>& K = nativeLevels(mkt); for (size_t k = 0; k < K.size(); ++k) if (("/" + K[k].name + "/").find("/Open/") != std::string::npos) rthOpen = K[k].price; }
    cs.lpShown = cs.lpGreen = cs.lpNoRow = cs.lpNoRoom = 0;
    auto blend = [](COLOR a, COLOR b, double f) -> COLOR {   // f of a over b (0x00RRGGBB)
        const int r = (int)(((a >> 16) & 0xFF) * f + ((b >> 16) & 0xFF) * (1 - f)), g = (int)(((a >> 8) & 0xFF) * f + ((b >> 8) & 0xFF) * (1 - f)), bl = (int)((a & 0xFF) * f + (b & 0xFF) * (1 - f));
        return (COLOR)((r << 16) | (g << 8) | bl); };
    auto rect = [&](short l, short t, short r, short b, COLOR c) { line(l, t, r, t, c, 1); line(r, t, r, b, c, 1); line(r, b, l, b, c, 1); line(l, b, l, t, c, 1); };
    for (size_t r : shown) {
        const lvc::Row& R = rows[r];
        const int yc = items[r].y, t = yc - bh / 2, b = t + bh;
        PNT tp; tp.set((int)nb - 1, (float)R.px); const int yTrue = tp.v;
        // the row card, the tick at the true price, a leader when the row moved
        box((short)(col.l + 16), (short)t, (short)(col.r - 3), (short)b, C_ROW);
        if (R.lots > 0) rect((short)(col.l + 16), (short)t, (short)(col.r - 3), (short)b, C_ROWLQ);
        line(col.l, (short)yTrue, (short)(col.l + 7), (short)yTrue, C_GREYC, 2);
        if (std::abs(yc - yTrue) > 1) { line((short)(col.l + 7), (short)yTrue, (short)(col.l + 12), (short)yc, 0x00475569, 1); line((short)(col.l + 12), (short)yc, (short)(col.l + 16), (short)yc, 0x00475569, 1); }
        // rank + liquidity
        if (R.lots > 0) {
            ++cs.lvcLiq;
            const COLOR base = R.bid ? C_BUY : C_SELL, txt = R.bid ? (COLOR)0x004ADE80 : (COLOR)0x00F87171;
            const std::string rk = std::to_string(R.rank);
            const int rw = std::max(bh - 2, textW(rk.c_str(), kf, true) + 6);
            if (R.rank == 1) box((short)(RK - rw / 2), (short)(t + 1), (short)(RK + rw / 2), (short)(b - 1), 0x00F8FAFC);
            else box((short)(RK - rw / 2), (short)(t + 1), (short)(RK + rw / 2), (short)(b - 1), 0x001E293B);
            const int rkw = textW(rk.c_str(), kf, true);
            textLJ((short)(RK - rkw / 2), (short)yc, rk.c_str(), R.rank == 1 ? (COLOR)0x000B0F19 : (COLOR)0x00F8FAFC, kf, true);
            box((short)LQX, (short)(t + 2), (short)(LQX + LQW), (short)(b - 2), blend(base, C_ROW, 0.12));
            const int fl = std::max(4, (int)std::lround((double)LQW * std::min(1.0, maxLots > 0 ? (double)R.lots / (double)maxLots : 1.0)));
            box((short)LQX, (short)(t + 2), (short)(LQX + fl), (short)(b - 2), blend(base, C_ROW, 0.38));
            std::string lt = R.liq; while (lt.size() > 3 && textW(lt.c_str(), kf, true) > LQW - 6) lt = lt.substr(0, lt.size() - 1);
            textLJ((short)(LQX + 4), (short)yc, lt.c_str(), txt, kf, true);
        }
        // chips: key (solid, his SessionPrices colours), volume (amber outline), gamma (violet outline); the same order always
        int x = CHX; const int chipR = PRR - textW("00,000.00", kf, true) - 10;
        for (size_t c = 0; c < R.chips.size(); ++c) {
            const lvc::Chip& C = R.chips[c];
            const int w = textW(C.text.c_str(), kf, true) + 8;
            if (x + w > chipR) { ++cs.lvcChipsCut; break; }
            const PendLbl& P = pend[in[C.member].idx];
            if (C.fam == lvc::KEY) {
                const COLOR fill = fade(P.col, P.stale ? 0.45f : 0.0f);
                box((short)x, (short)(t + 1), (short)(x + w), (short)(b - 1), fill);
                if (P.pick) rect((short)x, (short)(t + 1), (short)(x + w), (short)(b - 1), 0x00FFFFFF);
                const bool dark = (0.299 * ((fill >> 16) & 0xFF) + 0.587 * ((fill >> 8) & 0xFF) + 0.114 * (fill & 0xFF)) >= 150.0;
                textLJ((short)(x + 4), (short)yc, C.text.c_str(), dark ? C_DARK : (COLOR)0x00FFFFFF, kf, true);
            } else {
                const COLOR oc = C.fam == lvc::VOLUME ? C_AMBER : (P.src == 0 && C.fam == lvc::GAMMA ? C_GREYC : C_VIOLET);
                rect((short)x, (short)(t + 1), (short)(x + w), (short)(b - 1), P.stale ? fade(oc, 0.45f) : oc);
                textLJ((short)(x + 4), (short)yc, C.text.c_str(), P.stale ? fade(oc, 0.45f) : oc, kf, true);
            }
            x += w + 4;
        }
        // price (the row's own level) and the % block
        const PendLbl& P0 = pend[in[R.chips.front().member].idx];
        textRJ((short)PRR, (short)yc, P0.priceTxt.c_str(), (COLOR)0x00F1F5F9, kf, true);
        if (S.levelPct && cs.lpT.ok) {
            const lpct::Row* pr = nullptr; std::string used;
            const std::string side = lpct::sideOf(R.px, rthOpen, lastPx);
            for (size_t c = 0; c < R.chips.size() && !pr && !side.empty(); ++c) pr = lpct::lookup(cs.lpT, dcol::compactName(pend[in[R.chips[c].member].idx].name0), side, used);
            if (pr) {
                const std::string pt = lpct::text(*pr); const COLOR oc = (COLOR)lpct::outline(*pr);
                const short xl = (short)(ODR - ODW), xr = (short)ODR;
                box(xl, (short)(t + 1), xr, (short)(b - 1), (COLOR)0x00191919); rect(xl, (short)(t + 1), xr, (short)(b - 1), oc);
                textRJ((short)(xr - 4), (short)yc, pt.c_str(), C_INK, kf, true);
                ++cs.lpShown; if (lpct::green(*pr)) ++cs.lpGreen;
            } else ++cs.lpNoRow;
        }
        ++cs.badgesShown; ++cs.lvcRows;
    }
    if (offDn.have) offLine(offDn, pane.bottom - 2 - bh / 2, false);
    pend.clear();
}


// (2.29.0, Rassul 2026-10-27 14:35-14:58 CT, mockups level_lines / liqrank / tag_compare) LEVEL LINES. Each level (key / volume; gamma off)
// is a thin line across the price area (right of the left Delta column to the liquidity column) with ONE badge centred on it:
// [chips: key solid, volume outlined (DPoc cyan, WPoc bright cyan solid)] [price] [odds] [liquidity rank, ringed red = offers / green =
// bids, white = #1]. Levels within the merge tolerance share one line and badge. A badge that would overlap another slides sideways
// on its own line (the line never moves). The LIQUIDITY column on the right: rank + the big limit orders within 0.5 ATR of the level,
// top 10 ranked, on the level's own line. Off-screen nearest levels as text at the top / bottom of the price area. No hovers.
void DealerProfile::flushLines(const Settings& S)
{
    cs.badgesShown = cs.badgesDropped = 0; cs.lvcRows = cs.lvcLiq = cs.lvcMerged = cs.lvcChipsCut = 0;
    const bp::Box& col = cs.lay.levels;
    if (col.w() <= 0 || !cs.lay.ok) { cs.badgesDropped = (int)pend.size(); pend.clear(); return; }
    RCT pane; pane.getPaneRect(false);
    const long nb = getBarCount();
    const int kf = S.badgeFont, sf = std::max(8, kf - 1);
    const int bh = dcol::textPx(kf) + 4;
    const COLOR C_COL = 0x000B0F19, C_HEAD = 0x0064748B, C_AMBER = 0x00FBBF24, C_GREYC = 0x0094A3B8, C_CYAN = 0x0022D3EE, C_CYANB = 0x00A5F3FC;
    const int xL = (cb::DELTA_LEFT && cs.lay.delta.w() > 0) ? cs.lay.delta.r + 3 : pane.left + 2, xR = col.l - 1;
    if (xR - xL < 120) { cs.badgesDropped = (int)pend.size(); pend.clear(); return; }
    box(col.l, pane.top, col.r, pane.bottom, C_COL);                       // the liquidity column's ground
    {   double age = -1; const bool stale = !nx.empty() && gridVerdict(S.clock, age).stale();
        const std::string h1 = std::string("DP v") + DP_VERSION + "  LIQUIDITY";
        textLJ((short)(col.l + 6), (short)(pane.top + 4 + bh / 2), h1.c_str(), C_HEAD, sf, true);
        if (stale) { char b[40]; if (age >= 2880) snprintf(b, sizeof(b), "STALE %dd", (int)(age / 1440 + 0.5)); else if (age >= 90) snprintf(b, sizeof(b), "STALE %dh", (int)(age / 60 + 0.5)); else snprintf(b, sizeof(b), "STALE %dm", (int)(age + 0.5));
            textLJ((short)(col.l + 6), (short)(pane.top + 6 + bh + bh / 2), b, 0x00FF9A8F, sf, true); }
    }
    const int dec = std::max(0, std::min(6, (int)std::lround(-std::log10(std::max(1e-6, chartTick(mkt))) + 0.49)));
    const int midX = (xL + xR) / 2;
    auto offLine = [&](const OffLvl& o, int yc, bool up) {
        if (!o.have) return;
        const std::string t = std::string(up ? "^ " : "v ") + lvc::offText(o.name, dpaudit::priceText(mkt, o.px), o.dist, dec);
        const int tw = textW(t.c_str(), sf, false);
        textLJ((short)(midX - tw / 2), (short)yc, t.c_str(), C_GREYC, sf, false);
    };
    offLine(offUp, pane.top + 4 + bh / 2, true);
    if (offDn.have) offLine(offDn, pane.bottom - 4 - bh / 2, false);
    // the levels: key / volume only (gamma off)
    const double tick = chartTick(mkt), atr = atr14();
    std::vector<lvc::In> in;
    for (size_t i = 0; i < pend.size(); ++i) {
        const PendLbl& P = pend[i]; if (!std::isfinite(P.px)) continue;
        if (!DP_GAMMA && P.src != 0) continue;
        lvc::In x; x.px = P.px; x.name = dcol::compactName(klv::displayName(P.name0)); x.src = P.src;
        x.col = (unsigned long)P.col; x.pick = P.pick; x.stale = P.stale; x.idx = i; in.push_back(x);
    }
    std::vector<lvc::Row> rows = lvc::group(in, tick, atr);
    if (rows.empty()) { pend.clear(); return; }
    loadLmt();
    double lastPx = lastPxLv;
    if (!(lastPx > 0) && nb > 0) { RTARRAY cl(barClose); lastPx = (double)cl[(int)nb - 1]; }
    lvc::attach(rows, in, cs.lmtO, tick, atr, lastPx, lvc::TAG_ATR);
    std::vector<size_t> shown; std::vector<int> yOf(rows.size(), 0);
    for (size_t r = 0; r < rows.size(); ++r) { PNT p; p.set((int)nb - 1, (float)rows[r].px); yOf[r] = p.v;
        if (yOf[r] - bh / 2 > pane.top + bh + 4 && yOf[r] + bh / 2 < pane.bottom - bh - 4) shown.push_back(r); else ++cs.badgesDropped; }
    lvc::rank(rows, shown, lastPx);
    long long maxLots = 0; for (size_t r : shown) maxLots = std::max(maxLots, rows[r].lots);
    double rthOpen = NAN;
    if (S.levelPct) { loadLevelPct(); const std::vector<klv::Merged>& K = nativeLevels(mkt); for (size_t k = 0; k < K.size(); ++k) if (("/" + K[k].name + "/").find("/Open/") != std::string::npos) rthOpen = K[k].price; }
    cs.lpShown = cs.lpGreen = cs.lpNoRow = cs.lpNoRoom = 0;
    auto rect = [&](short l, short t, short r, short b, COLOR c) { line(l, t, r, t, c, 1); line(r, t, r, b, c, 1); line(r, b, l, b, c, 1); line(l, b, l, t, c, 1); };
    auto lower = [](std::string a) { for (char& c : a) c = (char)std::tolower((unsigned char)c); return a; };
    auto chipCol = [&](const lvc::Chip& C, const PendLbl& P, bool& solid) -> COLOR {
        const std::string lt = lower(C.text);
        if (lt.find("wpoc") != std::string::npos) { solid = true; return C_CYANB; }   // (14:41 "wpoc should be bright")
        if (lt.find("dpoc") != std::string::npos) { solid = false; return C_CYAN; }   // (14:41 "dpoc should be cyan")
        if (C.fam == lvc::KEY) { solid = true; return fade(P.col, P.stale ? 0.45f : 0.0f); }
        solid = false; return C_AMBER;
    };
    // pass 1: each row's badge width, line colour / style
    struct BG { size_t r; int y, w, l = 0; COLOR lc; bool dash; std::string odds; };
    std::vector<BG> bgs;
    for (size_t r : shown) {
        const lvc::Row& R = rows[r]; if (R.chips.empty()) continue;
        BG g; g.r = r; g.y = yOf[r];
        const PendLbl& P0 = pend[in[R.chips.front().member].idx];
        bool solid = true; g.lc = chipCol(R.chips.front(), P0, solid); g.dash = R.chips.front().fam != lvc::KEY && lower(R.chips.front().text).find("wpoc") == std::string::npos;
        if (R.chips.front().fam == lvc::KEY) g.lc = P0.col;
        int w = 4;
        for (const lvc::Chip& C : R.chips) w += textW(C.text.c_str(), kf, true) + 8 + 3;
        w += textW(P0.priceTxt.c_str(), kf, true) + 6;
        if (S.levelPct && cs.lpT.ok) {
            const lpct::Row* pr = nullptr; std::string used; const std::string side = lpct::sideOf(R.px, rthOpen, lastPx);
            for (size_t c = 0; c < R.chips.size() && !pr && !side.empty(); ++c) pr = lpct::lookup(cs.lpT, dcol::compactName(pend[in[R.chips[c].member].idx].name0), side, used);
            if (pr) { g.odds = lpct::text(*pr); ++cs.lpShown; if (lpct::green(*pr)) ++cs.lpGreen; } else ++cs.lpNoRow;
        }
        if (!g.odds.empty()) w += textW(g.odds.c_str(), sf, false) + 6;
        if (R.rank > 0) w += bh + 2;
        g.w = w + 2; bgs.push_back(g);
    }
    // pass 2: place - centred, a badge that would overlap a placed one slides sideways on its own line
    std::vector<bp::Box> placed;
    std::sort(bgs.begin(), bgs.end(), [](const BG& a, const BG& b) { return a.y < b.y; });
    for (BG& g : bgs) {
        const int t = g.y - bh / 2, b = t + bh;
        int best = midX - g.w / 2; bool ok = false;
        for (int k = 0; k < 11 && !ok; ++k) {
            const int s = k == 0 ? 0 : ((k % 2) ? -1 : 1) * ((k + 1) / 2) * (g.w / 2 + 50);
            const int l = midX - g.w / 2 + s;
            if (l < xL + 2 || l + g.w > xR - 2) continue;
            const bp::Box bx = bp::mk(l - 4, t - 1, l + g.w + 4, b + 1);
            bool hit = false; for (const bp::Box& q : placed) if (bp::overlap(q, bx)) { hit = true; break; }
            if (!hit) { best = l; ok = true; }
        }
        g.l = best; placed.push_back(bp::mk(best - 4, t - 1, best + g.w + 4, b + 1));
    }
    // the lines (behind the badges), into the liquidity column
    for (const BG& g : bgs) {
        if (!g.dash) line((short)xL, (short)g.y, (short)col.l, (short)g.y, g.lc, 1);
        else for (int x = xL; x < col.l; x += 9) line((short)x, (short)g.y, (short)std::min(x + 5, (int)col.l), (short)g.y, g.lc, 1);
    }
    // the badges
    for (const BG& g : bgs) {
        const lvc::Row& R = rows[g.r];
        const int t = g.y - bh / 2, b = t + bh, l = g.l, rgt = l + g.w;
        if (R.members.size() > 1) ++cs.lvcMerged;
        box((short)l, (short)t, (short)rgt, (short)b, C_COL);
        rect((short)l, (short)t, (short)rgt, (short)b, g.lc);
        int x = l + 3;
        for (const lvc::Chip& C : R.chips) {
            const PendLbl& P = pend[in[C.member].idx];
            const int w = textW(C.text.c_str(), kf, true) + 8;
            bool solid = true; const COLOR cc = chipCol(C, P, solid);
            if (solid) {
                box((short)x, (short)(t + 2), (short)(x + w), (short)(b - 2), cc);
                if (P.pick) rect((short)x, (short)(t + 2), (short)(x + w), (short)(b - 2), 0x00FFFFFF);
                const bool dark = (0.299 * ((cc >> 16) & 0xFF) + 0.587 * ((cc >> 8) & 0xFF) + 0.114 * (cc & 0xFF)) >= 150.0;
                textLJ((short)(x + 4), (short)g.y, C.text.c_str(), dark ? C_DARK : (COLOR)0x00FFFFFF, kf, true);
            } else {
                rect((short)x, (short)(t + 2), (short)(x + w), (short)(b - 2), P.stale ? fade(cc, 0.45f) : cc);
                textLJ((short)(x + 4), (short)g.y, C.text.c_str(), P.stale ? fade(cc, 0.45f) : cc, kf, true);
            }
            x += w + 3;
        }
        const PendLbl& P0 = pend[in[R.chips.front().member].idx];
        textLJ((short)(x + 1), (short)g.y, P0.priceTxt.c_str(), (COLOR)0x00F1F5F9, kf, true);
        x += textW(P0.priceTxt.c_str(), kf, true) + 6;
        if (!g.odds.empty()) { textLJ((short)(x + 1), (short)g.y, g.odds.c_str(), C_GREYC, sf, false); x += textW(g.odds.c_str(), sf, false) + 6; }
        if (R.rank > 0) {   // (14:54 "put the rank at the end of the badge")
            const std::string rk = std::to_string(R.rank); const int rr = rgt - 3, rl = rr - (bh - 2);
            const COLOR ring = R.bid ? C_BUY : C_SELL;
            RCT rb; rb.set((short)rl, (short)(t + 1), (short)rr, (short)(b - 1)); rb.draw(1, ring, R.rank == 1 ? (COLOR)0x00F8FAFC : (COLOR)0x001E293B, DRAW_OPAQUE, PAT_SOLID);
            const int rkw = textW(rk.c_str(), sf, true);
            textLJ((short)((rl + rr) / 2 - rkw / 2), (short)g.y, rk.c_str(), R.rank == 1 ? C_DARK : (COLOR)0x00F8FAFC, sf, true);
        }
        ++cs.badgesShown; ++cs.lvcRows;
    }
    // the LIQUIDITY column: rank + the orders at the level, on the level's line (rows that would touch step apart, a leader to the line)
    std::vector<size_t> lq; for (size_t r : shown) if (rows[r].lots > 0) lq.push_back(r);
    std::vector<cb::Item> items;
    for (size_t r : lq) { cb::Item it; it.want = yOf[r]; it.h = bh + 1; items.push_back(it); }
    int focusY = (pane.top + pane.bottom) / 2; if (lastPx > 0) { PNT p; p.set((int)nb - 1, (float)lastPx); focusY = p.v; }
    cs.badgesDropped += cb::stack(items, pane.top + 2 * bh + 10, pane.bottom - 4, focusY);
    const int RKX = col.l + 6, LQX = RKX + bh + 4, LQW = col.r - LQX - 6;
    for (size_t k = 0; k < lq.size(); ++k) {
        if (!items[k].shown) continue;
        const lvc::Row& R = rows[lq[k]]; ++cs.lvcLiq;
        const int yc = items[k].y, t = yc - bh / 2, b = t + bh;
        if (std::abs(yc - yOf[lq[k]]) > 1) line((short)col.l, (short)yOf[lq[k]], (short)(RKX - 1), (short)yc, 0x00475569, 1);
        const COLOR base = R.bid ? C_BUY : C_SELL, txt = R.bid ? (COLOR)0x004ADE80 : (COLOR)0x00F87171;
        if (R.rank > 0) {
            const std::string rk = std::to_string(R.rank);
            RCT rb; rb.set((short)RKX, (short)(t + 1), (short)(RKX + bh - 2), (short)(b - 1)); rb.draw(1, base, R.rank == 1 ? (COLOR)0x00F8FAFC : (COLOR)0x001E293B, DRAW_OPAQUE, PAT_SOLID);
            const int rkw = textW(rk.c_str(), sf, true);
            textLJ((short)(RKX + (bh - 2) / 2 - rkw / 2), (short)yc, rk.c_str(), R.rank == 1 ? C_DARK : (COLOR)0x00F8FAFC, sf, true);
        }
        const int fl = std::max(18, (int)std::lround((double)LQW * std::min(1.0, maxLots > 0 ? (double)R.lots / (double)maxLots : 1.0)));
        box((short)LQX, (short)(t + 2), (short)(LQX + fl), (short)(b - 2), fade(base, 0.55f));
        std::string lt = R.liq; while (lt.size() > 3 && textW(lt.c_str(), kf, true) > LQW - 6) lt = lt.substr(0, lt.size() - 1);
        textLJ((short)(LQX + 4), (short)yc, lt.c_str(), txt, kf, true);
    }
    pend.clear();
}

short DealerProfile::rightEdge(const RCT& pane)
{
    RCT scale; scale.getScaleRect();
    short r = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) r = (short)(scale.left - 2);
    return r;
}

// (2.11.0, Rassul approved 2026-10-14 11:00 CT) the LEVELS column: one compact badge per level ("ONH", "CR0", "HVL", "LonLO") in
// the column between the gap after the last bar and the Delta Profile, in the level's colour (white border = a pick, dimmed =
// STALE). Close levels stack in price order, never overlapping (cb::stack), each with a tick from its line to the badge; a badge
// that cannot fit is dropped (counted on the status). The level LINE still runs across the chart and through the columns. Every
// detail that used to sit in the wide label (price, odds "33%  /  50%", liquidity, STALE / age) is in the badge's hover.
// (2.14.0) the Levels column is at least 124 px so "LonHI+ 7755.75 476" fits (the setting can make it wider, never narrower than that)
static int lvlW(const Settings& c) { if (DP_LINES) return std::min(cb::range(cb::LEVELS).hi, DP_LQ_W); if (DP_LVC) return std::min(cb::range(cb::LEVELS).hi, DP_LVC_W); return std::min(cb::range(cb::LEVELS).hi, std::max(c.levelsW, 150) + (c.levelPct ? DP_PCT_W : 0)); }   // (2.19.0) + the % block   // (2.14.1) 150: an NQ 5-digit price + the HL block
void DealerProfile::flushLabels(const Settings& S)
{
    if (DP_LINES) { flushLines(S); return; }   // (2.29.0)
    if (DP_LVC) { flushColumn(S); return; }   // (2.28.0) the combined Levels + Liquidity column
    cs.badgesShown = cs.badgesDropped = 0;
    if (pend.empty()) return;
    const bp::Box& col = cs.lay.levels;
    if (col.w() <= 0 || !cs.lay.ok) { cs.badgesDropped = (int)pend.size(); pend.clear(); return; }
    RCT pane; pane.getPaneRect(false);
    const int kf = S.badgeFont;
    // (14:37 CT review) the badge is the text's real height (pt x 4/3) + 3 px above and below (the HL block's 2 px outline never touches
    // its text); the name block: 2 px from the column's left edge, 3 px text pad each side, 1 px right
    const int bh = dcol::textPx(kf) + 6, room = col.w() - 3 - 6;
    long nb = getBarCount();
    int focusY = (pane.top + pane.bottom) / 2;
    if (nb > 0) { RTARRAY cl(barClose); const float c0 = cl[(int)nb - 1]; if (dpFinite(c0)) { PNT p; p.set((int)nb - 1, c0); focusY = p.v; } }
    // one badge per name at a price: the same compact name on the same row (e.g. CR0 from two feeds) is drawn once
    std::vector<cb::Item> items; std::vector<size_t> src; std::vector<std::string> names;
    for (size_t i = 0; i < pend.size(); ++i) {
        const PendLbl& P = pend[i];
        const std::string nm = cb::fitName(dcol::compactName(P.name0), room, [&](const std::string& t) { return textW(t.c_str(), kf, true); });
        if (nm.empty()) { ++cs.badgesDropped; continue; }
        bool dup = false;
        for (size_t k = 0; k < src.size(); ++k) if (names[k] == nm && std::abs(pend[src[k]].y - P.y) <= 1) dup = true;
        if (dup) continue;
        cb::Item it; it.want = P.y; it.h = bh; items.push_back(it); src.push_back(i); names.push_back(nm);
    }
    // (2.20.0) the queue meter: which badge holds a live test; a test with no badge in its zone gets its own small unlabeled badge row
    cs.qmOnBadge = cs.qmAlone = cs.qmFar = cs.qmInactive = cs.qmNoRoom = 0;
    std::vector<const qm::FileT*> qmOf(items.size(), nullptr);
    const size_t QM_ALONE = (size_t)-1;   // src[] marker base: QM_ALONE - k = the k-th unlabeled meter
    std::vector<const qm::FileT*> qmAloneT;
    if (S.qm) {
        loadQueue();
        std::vector<char> hit(cs.qmF.t.size(), 0);
        std::vector<double> bestD(cs.qmF.t.size(), 1e18); std::vector<size_t> bestJ(cs.qmF.t.size(), (size_t)-1);
        for (size_t j = 0; j < items.size(); ++j) {   // ONE badge per test: the level nearest its anchor (PDH and PFH in one zone = PDH only)
            bool used = false; const qm::FileT* T = queueAt(pend[src[j]].px, S, used);
            if (!T) continue;
            const size_t k = (size_t)(T - &cs.qmF.t[0]); const double d = std::fabs(pend[src[j]].px - T->anchor);
            if (d < bestD[k]) { bestD[k] = d; bestJ[k] = j; }
        }
        for (size_t k = 0; k < bestJ.size(); ++k) if (bestJ[k] != (size_t)-1) { qmOf[bestJ[k]] = &cs.qmF.t[k]; hit[k] = 1; }
        for (size_t k = 0; k < cs.qmF.t.size(); ++k) {
            if (hit[k]) continue;
            bool used = false; const qm::FileT* T = queueAt(cs.qmF.t[k].anchor, S, used);
            if (T != &cs.qmF.t[k]) {
                if (!cs.qmF.t[k].contact || cs.qmF.t[k].decided >= 0) ++cs.qmInactive;
                else ++cs.qmFar;
                continue;
            }
            PNT p; p.set(lastBar, (float)cs.qmF.t[k].anchor);   // the file is in chart prices (the Liquidity Profile on this chart)
            cb::Item it; it.want = p.v; it.h = bh; items.push_back(it); src.push_back(QM_ALONE - qmAloneT.size()); names.push_back(std::string()); qmOf.push_back(&cs.qmF.t[k]);
            qmAloneT.push_back(&cs.qmF.t[k]);
        }
    }
    const int top = pane.top + 2, bottom = pane.bottom - 2;
    cs.badgesDropped += cb::stack(items, top, bottom, focusY);
    // (2.19.0) the level % blocks: this market's file, the side of each level from today's RTH open (before the open: the last price)
    cs.lpShown = cs.lpGreen = cs.lpNoRow = cs.lpNoRoom = 0;
    double rthOpen = NAN, lastPx = NAN;
    if (S.levelPct) {
        loadLevelPct();
        const std::vector<klv::Merged>& K = nativeLevels(mkt);
        for (size_t k = 0; k < K.size(); ++k) if (("/" + K[k].name + "/").find("/Open/") != std::string::npos) rthOpen = K[k].price;
        if (nb > 0) { RTARRAY cl(barClose); const float c0 = cl[(int)nb - 1]; if (dpFinite(c0)) lastPx = (double)c0; }
    }
    cs.hlOn = cs.liqBlocks = 0;
    for (size_t j = 0; j < items.size(); ++j) {
        if (!items[j].shown) continue;
        if (src[j] > pend.size()) {   // (2.20.0) an unlabeled meter (a pivot / a suddenly defended price): small, at its price, inside the Levels column
            const qm::FileT& T = *qmOf[j]; const int yc = items[j].y, t = yc - bh / 2, b = t + bh;
            const std::string tx = qm::badgeText(std::string(), T, 0);
            const short l = (short)(col.l + 2), r = (short)std::min<int>(col.r - 1, l + std::max(textW(tx.c_str(), kf, true) + 10, 56));
            char kb[64]; std::snprintf(kb, sizeof(kb), "%c%.6f", T.side == qm::BID ? 'B' : 'A', T.anchor);
            if (drawMeter(T, std::string(), l, r, t, b, yc, S, kb, qm::fmtPx(T.anchor, cs.qmF.tick))) ++cs.qmAlone; else ++cs.qmNoRoom;
            continue;
        }
        const PendLbl& P = pend[src[j]];
        const int yc = items[j].y, t = yc - bh / 2, b = t + bh;
        const float dim = P.stale ? 0.45f : 0.0f;
        if (std::abs(yc - P.y) > 1) {   // a stacked badge: a short tick from its line to the badge (solid, the level's colour)
            const short xt = (short)(col.l + 1);
            line(col.l, P.y, xt, P.y, P.col, 1);
            line(xt, P.y, xt, (short)(P.y < t ? t : b), P.col, 1);
        }
        if (qmOf[j]) {   // (2.20.0) THE METER: this level's badge becomes the meter "G1 Q76%" across the Levels column (its % block / liquidity give way)
            if (drawMeter(*qmOf[j], names[j], (short)(col.l + 2), (short)(col.r - 1), t, b, yc, S, P.key, P.priceTxt)) { ++cs.qmOnBadge; ++cs.badgesShown; continue; }
            ++cs.qmNoRoom;
        }
        // design B (Rassul 11:39 CT): [name on its level colour] + [the liquidity block: "310" grey on dark grey, or "HL 900" bold white
        // on green (bids below price) / red (offers above) with a 2 px white outline]. Fit, in order: both at the badge font; both 1 pt
        // smaller (8 pt floor); an HL block without the "HL " word (its colour + outline still say HL); else the name alone (never cut)
        const int avail = col.w() - 3;
        int bf = kf;
        std::string nm = names[j];
        std::string lt;   // (2.16.0, Rassul 22:18) the liquidity ratio ("3.1x", yellow); contracts only when no book behind to compare
        (void)lt;   // (2.25.0, Rassul 2026-10-24 21:05 "only name, value and prob ... get rid of the 0.06x") no liquidity ratio on the badge (still in the hover)
        std::string gm;   // (2.16.0) the gamma name merged on this level, right before the ratio
        { std::string kk, gg; if (names[j].find('+') == std::string::npos && dcol::splitGamma(names[j], kk, gg)) { nm = kk; gm = gg; } }
        const int lp = P.hl ? 10 : 6;                                  // pads: 3 px each side; the HL block + its 2 px outline
        // (2.19.0) the % block "33%  /  66%" goes last; its room is taken first (the rest gives up price / gamma / ratio as before)
        const lpct::Row* pr = nullptr; std::string pUsed, pTxt; int xw = 0;
        if (S.levelPct && cs.lpT.ok) {
            const std::string side = lpct::sideOf(P.px, rthOpen, lastPx);
            if (!side.empty()) pr = lpct::lookup(cs.lpT, dcol::compactName(P.name0), side, pUsed);
            if (pr) { pTxt = lpct::text(*pr); xw = textW(pTxt.c_str(), kf, true) + 8; }
            else ++cs.lpNoRow;
        }
        const int availAll = avail;
        if (xw > 0 && textW(names[j].c_str(), kf, true) + 6 + xw > availAll) { xw = 0; pr = nullptr; ++cs.lpNoRoom; }
        const int availL = availAll - xw;
        // (2.14.0, Rassul 10:31 "include the actual level in the middle of the level name and the liquidity") NAME | price | liquidity;
        // the price is the first thing given up when the column is too narrow (it stays in the hover)
        std::string pt = P.priceTxt; int pw = 0;
        int gw = 0;
        auto widths = [&](int& nw, int& lw) { nw = textW(nm.c_str(), bf, true) + 6; lw = lt.empty() ? 0 : textW(lt.c_str(), bf, true) + lp; pw = pt.empty() ? 0 : textW(pt.c_str(), bf, false) + 6; gw = gm.empty() ? 0 : textW(gm.c_str(), bf, true) + 6; };
        int nw = 0, lw = 0; widths(nw, lw);
        // (2.16.0) give up in order: 1 pt smaller, the price, the gamma name (both stay in the hover), the ratio
        if (nw + pw + gw + lw > availL && kf > 8) { bf = kf - 1; widths(nw, lw); }
        if (nw + pw + gw + lw > availL) { pt.clear(); widths(nw, lw); }
        if (nw + pw + gw + lw > availL) { gm.clear(); widths(nw, lw); }
        if (nw + pw + gw + lw > availL && lw > 0) { lt.clear(); widths(nw, lw); }
        if (nw + pw + gw + lw > availL) { bf = kf; nm = names[j]; widths(nw, lw); lw = 0; gw = 0; pw = 0; }
        const short l = (short)(col.l + 2), pmid = (short)(l + nw), gmid = (short)(pmid + pw), mid = (short)(gmid + gw), r = (short)(mid + lw);
        const COLOR fill = fade(P.col, dim);
        box(l, (short)t, pmid, (short)b, fill);
        if (P.pick) { line(l, (short)t, pmid, (short)t, 0x00FFFFFF, 1); line(pmid, (short)t, pmid, (short)b, 0x00FFFFFF, 1); line(pmid, (short)b, l, (short)b, 0x00FFFFFF, 1); line(l, (short)b, l, (short)t, 0x00FFFFFF, 1); }
        const bool dark = (0.299 * ((fill >> 16) & 0xFF) + 0.587 * ((fill >> 8) & 0xFF) + 0.114 * (fill & 0xFF)) >= 150.0;
        textLJ((short)(l + 3), (short)yc, nm.c_str(), dark ? C_DARK : (COLOR)0x00FFFFFF, bf, true);
        if (pw > 0) { box(pmid, (short)t, mid, (short)b, (COLOR)0x00191919); textLJ((short)(pmid + 3), (short)yc, pt.c_str(), C_INK, bf, false); }   // (2.14.0) the level's price
        if (gw > 0) { box(gmid - 0, (short)t, mid, (short)b, (COLOR)0x00191919); textLJ((short)(gmid + 3), (short)yc, gm.c_str(), dcol::C_POS, bf, true); }   // (2.16.0) the gamma name
        if (lw > 0) {
            ++cs.liqBlocks;
            if (P.hl) {
                ++cs.hlOn;
                box(mid, (short)t, r, (short)b, P.bid ? C_BUY : C_SELL);
                setPen(0x00FFFFFF, 2, P_SOLID);   // the 2 px white outline, inside the block
                PNT q; q.set(0, 0.0f); q.h = (short)(mid + 1); q.v = (short)(t + 1); q.setDrawPosition(); q.h = (short)(r - 1); q.drawLineTo(); q.v = (short)(b - 1); q.drawLineTo(); q.h = (short)(mid + 1); q.drawLineTo(); q.v = (short)(t + 1); q.drawLineTo();
                textLJ((short)(mid + 5), (short)yc, lt.c_str(), 0x00FFE14D, bf, true);
            } else {
                box(mid, (short)t, r, (short)b, (COLOR)0x00282828);
                textLJ((short)(mid + 3), (short)yc, lt.c_str(), 0x00FFE14D, bf, true);
            }
        }
        if (pr && xw > 0) {   // (2.19.0) the % block: dark fill, 1 px outline green (beats random) / grey (no better), the text in ink
            const short xl = (short)(std::max(r, std::max(mid, gmid)) + 1), xr = (short)(xl + xw - 1);
            const COLOR oc = (COLOR)lpct::outline(*pr);
            box(xl, (short)t, xr, (short)b, (COLOR)0x00191919);
            line(xl, (short)t, xr, (short)t, oc, 1); line(xr, (short)t, xr, (short)b, oc, 1); line(xr, (short)b, xl, (short)b, oc, 1); line(xl, (short)b, xl, (short)t, oc, 1);
            textLJ((short)(xl + 4), (short)yc, pTxt.c_str(), C_INK, kf, true);
            ++cs.lpShown; if (lpct::green(*pr)) ++cs.lpGreen;
            HoverT X; X.l = xl; X.r = xr; X.t = (short)t; X.b = (short)b; X.key = "P|" + P.key; X.pct = true;
            const lpct::Hover lh = lpct::hover(mkt, klv::displayName(dcol::compactName(P.name0)), pUsed, P.priceTxt, *pr, cs.lpT);
            X.h.title = lh.title; X.h.lines = lh.lines;
            hovT.push_back(X);
        }
        ++cs.badgesShown;
        HoverT H; H.l = l; H.r = std::max(r, std::max(mid, gmid)); H.t = (short)t; H.b = (short)b; H.key = "L|" + P.key;
        H.h = dcol::levelHoverC(mkt, P.name0, P.priceTxt, P.odds, P.liq, P.pick, P.above);   // (2.15.0) compact
        if (P.hvl) {
            if (cgate::kShowHvlCushion) { const dpaudit::Hover V = dcol::hvlHoverC(mkt, (double)hvlChartPx - off); H.h.lines.insert(H.h.lines.begin() + 1, V.lines.begin(), V.lines.end() - 2); }
            else H.h.lines.insert(H.h.lines.begin() + 1, "HVL " + dpaudit::fmtStrike(mkt, (double)hvlChartPx - off) + " = the MenthorQ model's gamma flip level (vendor model).");   // (2.12.0) no "moves fade / run" regime cause (hvl_cushion gate)
        }
        hovT.push_back(H);
    }
    pend.clear();
}

long long DealerProfile::nowMsC()
{
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
// (2.11.0) this draw's columns: the other plugins' widths from the board (Delta / Liquidity, with their heartbeat), this plugin's
// own Levels + Dealer widths (its settings), the layout (ColumnBoard.h), and what the margin must be; the unit check
void DealerProfile::columns(const RCT& pane, short axisR, bool hasDealer)
{
    const long long now = nowMsC();
    if (cs.firstMs < 0) cs.firstMs = now;
    const char* sy = getSymbol(); cs.sym = sy ? sy : ""; cs.spb = getSecondsPerBar();
    const char* cl = getChartLabel(); cs.chart = cl ? cl : "";
    cs.peers.step(cs.sym, cs.spb, cs.chart, now, (1u << cb::DEALER) | (1u << cb::LEVELS));
    cb::In in; in.paneL = pane.left; in.paneT = pane.top; in.paneB = pane.bottom; in.axisR = axisR;
    in.levels = true; in.levelsW = lvlW(cfg);
    in.dealer = hasDealer; in.dealerW = cfg.width;
    in.delta = cs.peers.on[cb::DELTA]; in.deltaW = cs.peers.w[cb::DELTA];
    in.big = cs.peers.on[cb::BIG]; in.bigW = cs.peers.w[cb::BIG];   // (2.12.1) the BIG column first after the last bar
    cs.lay = cb::layout(in);
    cb::publish(cb::LEVELS, cs.sym, cs.spb, cs.chart, now, lvlW(cfg));
    if (hasDealer) cb::publish(cb::DEALER, cs.sym, cs.spb, cs.chart, now, cfg.width);
    else cb::clear(cb::DEALER, cs.sym, cs.spb, cs.chart);          // an hourly (levels-only) chart has no Dealer column
    cs.published = !cs.sym.empty();
    cs.needPx = cs.lay.ok ? cs.lay.margin : 0;
    // the unit IRT uses for the right margin: the measured gap (last bar on screen) against the value it reports
    int ppb = 0, cur = -1;
    if (getChartOption(OPT_PIXELS_PER_BAR, &ppb) == RTX_OK && ppb > 0) cs.ppb = ppb;
    if (getChartOption(OPT_RIGHT_MARGIN, &cur) == RTX_OK) cs.marginRead = cur;
    const long nb = getBarCount(); int vs = 0, ve = -1;
    if (nb > 1 && getVisibleBars(&vs, &ve) == RTX_OK && ve == (int)nb - 1) {
        PNT pe; pe.set((int)nb - 1, 0.0f, kBarRight);
        cs.gapSeen = axisR - pe.h;
        cs.probe.observe(cs.marginRead, cs.ppb, cs.gapSeen);
    }
}
// (2.11.0) the chart's right margin = gap + Levels + Delta + Dealer, in IRT's unit (cb::marginValue), set only here: calc / preCalc /
// Apply (the SDK: "available in parmsApply and pre calc calls"). Grows at once, shrinks after 60 s; nothing before the first draw.
void DealerProfile::applyMargin()
{
    if (cs.firstMs < 0 || cs.needPx <= 0) return;                  // no layout yet (the settings window's copy, before the first draw)
    int ppb = 0;
    if (getChartOption(OPT_PIXELS_PER_BAR, &ppb) != RTX_OK || ppb <= 0) ppb = cs.ppb;
    if (cs.probe.units == cb::BARS && ppb <= 0) return;             // the bar width is unknown: never guess a margin
    int cur = -1; if (getChartOption(OPT_RIGHT_MARGIN, &cur) != RTX_OK) cur = cs.marginRead;
    const int v = cs.mctl.want(cs.needPx, ppb, cs.probe.units, cur, nowMsC());
    if (v < 0) return;
    cs.marginAsked = v;
    if (setChartOption(OPT_RIGHT_MARGIN, v) == RTX_OK) ++cs.setOk; else ++cs.setFail;
}
void DealerProfile::unpublishCols()
{
    if (!cs.published) return;
    cb::clear(cb::DEALER, cs.sym, cs.spb, cs.chart); cb::clear(cb::LEVELS, cs.sym, cs.spb, cs.chart);
    cs.published = false;
}
int DealerProfile::calc(int)  { LSPERF_SCOPE(LSPERF_PLUGIN, "calc"); if (loadHostState()) { applyMargin(); saveHostState(); } return RTX_OK; }
int DealerProfile::preCalc(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "preCalc"); if (loadHostState()) { applyMargin(); saveHostState(); } return RTX_OK; }

int DealerProfile::draw(void)
{ LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    // (2.2.0, 2026-10-05) IRT runs ONE object of this DLL for every chart, so the settings read when another chart's dialog
    // was last applied leaked here: the HG chart drew / exported as GC (LRA-IRT-Bars-GC.csv held copper bars, root CPEZ26) and
    // the profile flipped between markets ("disappears and reappears"). Read THIS chart's settings on every draw.
    if (!loadHostState()) return RTX_OK;
    syncSettings();
    load();
    alignContract();
    pend.clear();                             // (2.7.4) this draw's badges
    offUp = OffLvl(); offDn = OffLvl(); lastPxLv = 0;   // (2.28.0)
    hovT.clear(); hvlFound = false;           // (2.9.0) this draw's hover targets / HVL
    if (msHas && !hovWnds.empty()) {          // (2.9.0) the mouse left the chart: no hover
        void* w = dpCursorWindow();
        if (w && std::find(hovWnds.begin(), hovWnds.end(), w) == hovWnds.end()) { msHas = false; hovKey.clear(); }
    }
    // (2.8.0, Rassul 10:29 "add just the levels to the hourly chart") a chart of 15-min+ bars: the levels only - no profile, no
    // stale banner, and never the bar export (the Reader's LRA-IRT-Bars file is the 3-min chart's)
    bool levelsOnly = false;
    {   long nb = getBarCount(); RTARRAYI dt(barDateTime); std::vector<long long> st;
        for (long i = nb - 41 < 0 ? 0 : nb - 41; i < nb && i < (long)dt.count; i++) st.push_back((long long)dt[(int)i]);
        levelsOnly = dpaudit::barSeconds(st) > 900; }
    {   // (2.11.0) the columns of this chart, before anything is drawn
        RCT pane; pane.getPaneRect(false);
        columns(pane, rightEdge(pane), !levelsOnly && DP_BARS);   // (2.28.0) no Dealer column while its bars are off
    }
    drawMQLevels(cfg);                        // (2.3.0) under the profile: the key-level lines first
    drawKeyLevels(cfg);                       // (2.5.0) key levels + hourly swings with their touch odds
    if (DP_LVC && DP_GAMMA) addGexRows();     // (2.29.0) no GEX rows while gamma is off                 // (2.28.0) the biggest unnamed gamma strikes
    flushLabels(cfg);                         // (2.11.0) the compact badges in the Levels column, over every line
    if (!levelsOnly) {
        if (DP_BARS) render(cfg);
        if (nx.empty()) { staleBadge(); srcTag(); }   // (2.9.0) a 2.4 file: the header says both (book + STALE)
        exportBars();
    }
    if (const HoverT* T = (DP_HOVERS || DP_PCT_HOVERS) ? hovered() : nullptr) { hovKey = T->key; drawHover(*T, cfg); }   // (2.17.3) DP_HOVERS   // (2.9.0) the hover, over everything (2.11.0: badges too)
    else hovKey.clear();
    writeStatus(D.nodes.empty() ? "no data" : "drawn");
    postMst();                                // (2.17.0) the Market State (behind the scenes; nothing drawn changes)
    saveHostState();
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerProfile *p = new DealerProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | ((DP_HOVERS || DP_PCT_HOVERS) ? TRACK_MOUSE : 0));   // (2.17.3) no TRACK_MOUSE while the hovers are off   // (2.9.0) hovers: mouse events even when not selected
    p->setDescription("LRA Dealer Profile: what dealers must trade at each strike. The guide is below the settings.");
    p->setVersion("2.29.0");   // = DP_VERSION (gex-build reads this literal; test_dealerprofile_2110 checks they match)
    return p;
}
