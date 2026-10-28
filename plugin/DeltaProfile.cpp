/********************************************************************************
 * 2.8.5 (2026-10-14 night, Rassul approved 23:05 CT; his E-S chart showed a cluster of circles on one candle: the 2.8.0 records held
 *       the same peak bar 2-3 times at slightly different prices after the row size changed - 20:18 x3, 21:06 x3, 21:00 x2):
 *       (1) ONE BUBBLE PER CANDLE PER SIDE - among the confirmed A events whose peak bar is the same candle, a bullish (support)
 *           A draws ONE bubble at the LOWEST price, a bearish (resistance) A ONE bubble at the HIGHEST price; the profile A label
 *           follows the same choice. Only the events decided at the group's FIRST decision moment (its smallest t_decided = the
 *           candle's close when IRT was running) compete: a duplicate decided later never moves a drawn bubble. Pure function of the
 *           record set (dp::oneBubblePerCandle): load-order independent, identical after a restart, never repainted by later bars.
 *       (2) a ONE-TIME RE-CHECK of A records written before 2.8.4 (version column < 2.8.4) against the 2.8.4 rules on the chart's
 *           bars 0 .. peak (dp::recheckA = same-bar close beyond + halfwayOk + pivotOk): a failure is not drawn, stays in the record,
 *           and the status says "hidden: pre-2.8.4 rule (<rule>)"; bars not on the chart = undrawn, said so. 2.8.4+ records are
 *           never re-checked (decided once). The records are never changed - display only.
 * 2.8.0 (2026-10-14, CR5200, Rassul approved 11:00 CT - mockup-columns.png): the Delta Profile is the MIDDLE right-side column,
 *       between the Levels column and the Dealer Profile (ColumnBoard.h): its box is fixed from the axis (the Dealer column's width
 *       from the board, 0 when no Dealer Profile is on the chart), Width 40-250 px (default 80), every bar anchored on the column's
 *       right edge growing left toward price, and NOTHING drawn past its left edge (the node labels shorten "A 2.1x -861" ->
 *       "A 2.1x" -> "A", else are not drawn). Each row's delta value next to its bar on the price side ("+137" / "-162", green /
 *       red, "Show delta text"), only where it fits and does not overlap another (biggest first). The A circles / Ex marks stay
 *       left of the gap before the Levels column. With no Dealer Profile on the chart this plugin sets the chart's right margin
 *       (gap + Delta); with one, the Dealer Profile does. Settings (controls only, per market): Width px, Font pt, Show delta text
 *       (the old Gap row is gone - the columns abut; parameter version 12). Title "DLT v2.8.0".
 * DeltaProfile.cpp -- lsDeltaProfile 2.7.1 (2026-10-13: no "[offscreen]" A labels) + 2.7.0 thin-extreme exhaustion mark "Ex?" (on the 2.6.3 contract-qualified record and the
 *                     2.6.2 peak-bar absorption rule)
 * Native chart VAP; rolling (latest bar close - 90 min, latest bar close].
 * Right placement, left-facing one-sided bars, automatic price-anchored rows.
 * Only Width / Gap / Font are configurable. Delta = ask/buy minus bid/sell.
 * A = confirmed absorption heuristic; I = initiative; ? = pending. Rings ONLY for confirmed A.
 * Every absorption candidate is recorded (2.5.4; 2.6.3 file name) to lsFlexLevels\DeltaProfile\<MKT>-<contract>-<sec/bar>-signals-<session>.csv when
 * it first appears and when it is decided; only final A bubbles survive through the session and restart.
 * (2.7.0) Ex? = a thin extreme (new high / low at a key or gamma level, thin delta at the extreme, fading aggression), printed
 *         once at the first bar closing back; logged as E lines in the same record; shown only as the nightly
 *         LRA-DeltaEx-<MKT>.csv allows (missing file = log only).
 * This revision deliberately requires Investor/RT replay acceptance before use.
 ********************************************************************************/
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "DeltaProfile"
#endif
#include <string>
#include <ctime>
#include <cstring>
#include <climits>
#include <cfloat>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <locale>
#include <map>
#include <set>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
// The SDK selects its old Mac branch solely by __GNUC__. Clang Windows checking
// can use this override; the production MSVC build uses the unmodified header.
#if defined(DELTA_WINDOWS_SDK_CHECK) && defined(__GNUC__)
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include "irtsdk.h"
#pragma pop_macro("__GNUC__")
#else
#include "irtsdk.h"
#endif
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#include "DeltaProfileCore.h"
#include "DeltaProfileSettings.h"   // (2.18.1) the ONE settings table (SettingsSpec.h)
#include "SettingsHost.h"           // (2.18.1) the window from the table
// (2.11.0) a two-sided row: buying and selling both heavy - the net under 15% of the volume (a top-3 volume row is checked)
namespace delta_profile { inline bool twoSided(double delta, double volume) { return volume > 0 && (delta < 0 ? -delta : delta) < 0.15 * volume; } }
#include "AbsorbLogic.h"     // (2.20.0) the wick absorption A (footprint candidate + order-book truth) - retired 2.22.0 (records still read)
#include "SharedAbsorb.h"    // (2.22.0) the SHARED absorption A: one rule, one record for every plugin
#include "TrapLogic.h"       // (2.21.0) the trapped traders read (Tr / A|Tr on the profile)
#include "SignalBoard.h"
#include "CombinerLogic.h"   // (2.28.0) layer 3: the combined read (ChainSpec.h)     // (2.9.1) posts its confirmed A events for the other indicators on this chart
#include "ZoneNear.h"        // (2.9.0) location first: A bubbles / labels away from a zone are dimmed
#include "ColumnBoard.h"     // (2.8.0) the right-side columns, the board and the right margin (shared with Dealer / Liquidity)
#include "KeepOutDraw.h"     // (2.21.2) BoxDock.h 1.2.0 keep-out: nothing painted over the HOD/LOD table
#include "AwareHook.h"       // (2.24.0) the smart-read hook (layer L3)
#include "SelfAware.h"       // (2.25.0) self-awareness
#include "MarketState.h"     // (2.17.0) posts its SRC "DP" facts once per closed bar for the read (MARKET-STATE-SPEC.md)
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace dp = delta_profile;
static ss::Ids& dpIds() { static ss::Ids I; return I; }   // the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
static const char* const DLT_VERSION = "2.28.0"; // (2.28.0, 2026-10-28 overnight build, Rassul 19:08-19:29 CT "on the right we have LMTF then delta profile ... a seperate column that interprets delta profile right before the delta profile column ... the delta profile can have just the delta profile and the ratio" + 20:11-20:18 "all the indicators communicating", "the combiner" approved) the column is back on the RIGHT (ColumnBoard 1.6.0: gap, LMT F, Delta, Dealer, axis; the liquidity column moved to the left edge), bars growing toward price (no mirror); its first 26 px are the READ strip: every decided A in the 90-min profile shows "A" at its row in the winner's colour (the research 2026-10-28: A adds +9 turn / +9 swing over the same candle without it, 7 markets, held-back days; every other candidate code failed and stays record-only); the bars carry the ratio only ("7.2x", no "A " prefix); posts SignalBoard 1.4.0 facts R (READ), N (nodes), C (candle trap lots for lsTrappedTraders) and runs the layer-3 COMBINER (CombinerLogic.h): the turn / swing odds of every closed pivot-reversal candle from the nightly weights (LRA-Combiner.csv), drawn as "72%" beyond the candle when >= the SHOW threshold, logged for next-morning scoring // (2.27.1, 2026-10-27 18:52 CT Rassul circled the cramped left column "are you trying to interpret the delta profile") the left column shows the PROFILE only: the trapped-traders badges ("Tr?" / "A|Tr?", amber outline) are no longer drawn on it (decided and recorded as before - TR lines, status TRAP - never fired at a key level in the 10-27 test: unproven, so record only); their reserved room is given back to the bars, and the label room is sized from the label actually drawn ("A 7.2x", was sized as "AE? 7.2x"), so the bars get ~2/3 of the column instead of ~1/3; the 3 biggest bars keep their ratio label (his 2.18.0 rule) at their own row; nothing on the candles changes // (15:16 CT Rassul "allow it to repaint if the rules are not met", option "live, then confirm") the LIVE A: the A rules run on the forming candle every build; while they hold the coloured A shows, if a rule fails it goes away (back to gray); at the candle close the normal decision rules and never changes; display only (no record / SignalBoard line). (2.27.0, 2026-10-27 15:04 CT Rassul "at or below the open for bullish and at or above the open for bearish", "needs to be less than halfway of the bar", 15:07 "update pivot rule to below for bullish and above the low of the uppivot bar") THE OPEN + HALFWAY RULES: a node A needs its whole row at / below the open of its peak candle and its middle at / below the candle's halfway mark (bullish; bearish mirror); pivot rule: the candle is the pivot, or its low is BELOW the down-pivot bar's high (bearish: high ABOVE the up-pivot bar's low) - DeltaProfileCore.h researchA rule 6; a big node failing it stays the gray circle; old records are not re-decided. (14:00 CT Rassul "to save space move the delta profile to the left") the Delta column sits at the LEFT edge of the price pane (ColumnBoard 1.5.0) on a solid background, MIRRORED so the bars grow from the left edge toward price (14:42 CT "delta profile should be pointing towards price"); it no longer widens the right margin; circles / marks / rules / records unchanged // (2.26.0, 2026-10-27, Rassul 00:36-00:38 CT option 2 "placement only") the A circle and the gray circle are drawn wholly BELOW the candle body (below the open and the close) for bullish and wholly ABOVE it for bearish; a node already outside the body keeps its place; which candles get an A is unchanged (records, rules, SignalBoard, parameter version 17 unchanged); the shared A already sat beyond the candle extreme // (2.25.0, 2026-10-26) SELF-AWARENESS (SelfAware.h 0.1.0, SELF-AWARE.md): the Delta Profile remembers its own values since the open (biggest node ratio per closed bar, today's percentile), its own A signals (final on its candle or confirmed on the next) and their outcomes resolved by later bars only (HIT = 1 ATR its way before 1 ATR against within 20 bars), its record today and long run per context (LRA-Self-DP-<MKT>.csv from the nightly lra.self_aware), a self-adjusted confidence for future signals only (SWITCH OFF per market: nothing drawn changes), the reasons of each signal; posts SELFDP (class METRIC) and records lsFlexLevels\\Self-DP-<MKT>-<spb>s-<day>.rec // (2.24.0, 2026-10-26) smart-read hook (AwareHook.h 0.1.0, TEAM-DESIGN.md): posts AWDP for each closed bar once lsContext has posted CTX for it (or after 3 s), records the isolated + aware read in lsFlexLevels\\Context-AWR-<MKT>-<spb>s-<day>.rec; links OFF per market (LRA-Context-<MKT>.csv) so aware = isolated and nothing drawn changes // (2.23.1, 2026-10-25, Rassul approved 2026-10-25 22:29 CT "colour the next-candle A") a next-candle A (conf=next: its own candle closed in the attackers' colour, the NEXT candle closed beyond the node) is drawn in the winner's colour like every A (was the gray circle); the colour check uses its confirmation candle; records / rules / parameter version 17 unchanged // (2.23.0, 2026-10-25, the go for the Iteration 6 parity fix: "make the drawn A the researched A") THE RESEARCHED A: the iteration-6 parity found the live A decided a median 36 min after its candle (4% within 1 bar; Jaccard 0.01-0.04 vs the research detector) - nodes reached the top 3 late (median 31 min after their candle) and the 90-min row grid re-cut one price area into new nodes (53% of A records were a 2nd+ A on an already-A candle). Now an A is decided ONLY at the close of its own candle (same) or the next candle (next) by dp::researchA (DeltaProfileCore.h 2.23.0 = iter6 a6_indep.detect: 30-bar window ending at the candle, 60 rows cut from the window low, top 3 at 2.0x+, peak candle = this or the previous candle, the close rule, the 6-bar pivot rule) - evaluated once per closed candle in order (watermark RW lines), never re-decided; the old late path still runs but its A is recorded as code AL with rule=old, delay_bars, delay_min, late=0|1 and is NOT drawn unless "Draw late A (not tested)" is ticked (default off); no tentative A is drawn any more (no TA lines); D lines of 2.23.0 have 29 fields (rule / delay_bars / delay_min / late); parameter version 17 // (2.22.0, 2026-10-22 Rassul 06:43 "then do it", 06:46 "only one circle per candle", 06:49 "then require it") THE SHARED ABSORPTION A (SharedAbsorb.h 1.0.0): the 2.20.0 wick A is retired (not decided; old conf=wick records not drawn; its two settings rows hidden); the price chart shows the shared A - the letter A in a ring in the winner's colour just beyond the candle's extreme - read from lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv (lsBidAskStats decides; this plugin decides from the chart VAP only when no attacker pane is on the chart); ONE circle per candle: the shared A wins, else one node A (the biggest |delta| of the top-3 node A's) and only on a candle closed in the defenders' colour, else one gray; records unchanged; SignalBoard A also carries the shared A; status SHARED_A / ONE_CIRCLE // (2.21.2, 2026-10-21 16:05 CT Rassul "fix this, it should be solid") screen only: an A / gray circle, its tick, a label or a fill that would touch the lsHodLod table is not painted there (the record, SignalBoard and no-repaint rules unchanged; KeepOutDraw.h, status UNDER_HODLOD) // (2.21.1, 2026-10-21 render check) the Tr badge in its OWN clear slot: >= 4 px from every A label / bubble (above or below the outline, else beside the outer trapped row, else stepping out), text inside its box with 4 px padding, a step smaller font when needed; the tick still to the trapped rows // (2.21.0, 2026-10-21 Rassul approved delta-profile-trapped-mockup-v2.png 11:43 CT "ok.. approve.. build", Delta-Profile-research-2026-10-21.md) TRAPPED TRADERS (TrapLogic.h): a candle that makes a new 20-candle high / low, with the attackers' lots traded beyond the old extreme >= 1.0x the median attack of the 20 candles before it, and that closes back inside = trapped buyers (red) / sellers (green); decided once at the close, never repainted, a TR line in the session record; drawn ON the profile: the trapped rows outlined in amber and a badge beside them "Tr?" or "A|Tr?" when his A is on the same candle and side; the size only in the status / record; the old 3-bar T context of Ex stays logged only; settings row "Trapped traders" (trap_on, parameter version 15) // (2.20.0, 2026-10-21 Rassul approved mockup-absorption-v2-ES.png "approved", MBO Roadmap Top Defender #1) WICK ABSORPTION A: at a 5-candle high / low, the attackers' lots in the wick >= X (default 3.0, per-market setting) x this market's normal for that half hour (LRA-AbsNorm-<MKT>.csv seed + own LRA-AbsObs-<MKT>.csv, last 10 sessions), AND the order book (lsBidAskStats, SignalBoard kind O) shows the defenders filled >= 2x what they ever showed and reloaded >= 2 times at wick prices taking >= 25% of that attack; decided at the candle close after the order feed decided it, never repainted, A bubble in the winner's colour, D line conf=wick; no order feed = logged only (status WICK_A). Tested 7 markets x 12 sessions vs the order book (research_testing.md) // (2.19.1, Rassul 2026-10-19 17:39 CT "lets do it the other way around using bullish or bearish instead") the A circle in the WINNER's colour: bullish A green, bearish A red (was the trapped side's colour, 2.10.3); gray unchanged // (2.19.0, Rassul 2026-10-19 17:09-17:13 CT "the circle is just a gray circle and you dont know bullish or bearish until candle close" / "stay gray, keep the pivot rule and the next candle rule too" / "the idea is to see if volume is trapped and which way the close occurred at a pivot", mockup delta-gray-circle-mockup.png approved "ok build") every big node (3 biggest rows, 2.0x+) that is not a coloured A is drawn as ONE GRAY circle on its candle - while it forms, while it waits for the next candle or its pivot, and for good when no side won; it turns red (bullish A) / green (bearish A) the moment the A rules hold (rules unchanged); colour wins its candle; display only (no record or SignalBoard change); status GRAY_DRAWN // (2.18.3, 2026-10-18 his decision list) Font default 10 pt (was 9; a 9 from the old default reads as 10) // (2.18.2, 2026-10-18 settings audit) the one-time migration reads THIS chart's contract-root row first (as 2.18.0 did), per-chart (~) rows never; // (2.18.1, 2026-10-18 settings audit, Rassul 15:13 CT "make sure every indicator always has its settings verified and tested") the window from DeltaProfileSettings.h / SettingsSpec.h: settings per MARKET in DeltaProfile.settings-<MKT>.txt (the per-root file fell back to defaults at every contract roll), migrated once from DeltaProfile.settings.txt; Width 110-250 (the column never drew under 110); "Show delta text" left the window (no row values are drawn since 2.14.0); drawing unchanged // // (2.18.0, Rassul 2026-10-18 11:49 CT mockup approved: "the close can be from the same candle or the close of the next candle as long as the low of both bars is below the high of the pivot bar which may or may not be either bar for bullish absorption and vice versa for the bearish absorption") A = the node's own candle closes beyond the node (unchanged) OR, if it did not, the NEXT candle does (bullish green close above the node's high, bearish red close below its low) with the lows (bullish) / highs (bearish) of BOTH bars beyond the pivot bar's high / low; the bubble stays on the node's candle; a failed node waits exactly one candle, never a third; the pivot-confirmation repaint of 2.16.0 applies the same way; D and TA lines end in conf=same|next (older lines = same), status A_CONF counts them. (2.18.0, Rassul 2026-10-18 12:49 CT mockup approved) the profile's ratio label ONLY on the 3 biggest bars by abs delta (dp::labelKeys), even one under 2.0x; no label on any other bar, even at 2.0x+; "A " kept on an absorbed node's label; circles and A rules unchanged. Evidence (his recorded A candidates Oct 14-16, 7 markets; held = 1 ATR before 1 tick through the node within 20 bars): same-candle A held 42% (n 197), next-candle A meeting his rule 52% (n 31), next-candle failing it 8% (n 13) // (2.17.0, 2026-10-18 audit B) SignalBoard A events carry flag 1 = final / 2 = tentative (SignalBoard 1.1.0: read() gives readers such as the Liquidity Profile only final ones); a tentative A writes a TA|S line when first shown and a TA|X line when its confirmed pivot fails it (the nightly measures how often a shown A? vanished); per-chart status file DeltaProfile.status-<MKT>-<spb>.txt (the shared DeltaProfile.status.txt is still written for one more version); Market State SRC "DP" facts once per closed bar (MarketState.h, cleared on done / destroy) // (2.16.0, Rassul 2026-10-17 "it is ok to repaint when the pivot requirement is there") the pivot rule against the peak bar's OWN swing (the down / up pivot of the leg it belongs to, which a later bar may make): the A bubble shows while the close rule and that pivot hold, may go and come back until the pivot is confirmed; the record (D line) is written only then, so the nightly sees the final A // (2.15.0, Rassul 2026-10-17 15:35 option 2 (b)) the 2.6.0 absorption rules in this indicator: only the 3 biggest rows (2.0x or more), no halfway rule, more than one bubble per candle; kept: the pivot rule, the same-candle close, no repaint, the layout and the shared A events // (2.13.0, Rassul 2026-10-16 17:40 "just keep it simple to absorption") the absorption rule only: every row >= 2.0x the average |delta| (90-min profile) is tracked, no top-3 cap, no initiative (I never decided or labelled), no zone A, ONE bubble per candle (the higher ratio), never repainted // (2.12.0, Rassul 08:25) the 3 largest bars always show a value; A and I labels from the record, frozen, kept for 90 min // (2.11.2) the column is drawn at least 110 px wide (a saved Width of 80 no longer squeezes the A label) // (2.11.1) a decided A keeps its profile label while its row is in the 90-min profile (no longer only while the node is in the live top 3) // (2.11.0) two-sided rows: a top-3 volume row with net under 15% of its volume gets "\xB1" // (2.10.3) the A circle on the candle in the trapped side's colour (bullish red, bearish green) // (2.10.2) strictly the 3 biggest: one with no room is not replaced by the 4th // (2.10.1) the 3 biggest values in all (not 3 per side) // (2.10.0, Rassul 2026-10-15 "(b)") the delta value also on the biggest rows // (2.9.1) posts its A events on the SignalBoard // (2.9.0, Rassul 2026-10-15) location first: A bubbles + labels at a high / low / pivot / level full strength, mid-range dimmed (ZoneNear.h) // (2.8.6, Rassul 2026-10-15) A label delta without the +/- sign ("A 3.7x 92"); colour gives the side // (2.8.5, Rassul approved 2026-10-14 23:05 CT) ONE bubble per candle per side: of the confirmed A events on one peak bar, bullish draws one bubble at the LOWEST price, bearish one at the HIGHEST (only among those decided at the group's first decision moment - a later re-track never moves a drawn bubble); the profile A label follows the same choice; records unchanged (display only). A records written before 2.8.4 are re-checked ONCE against the 2.8.4 rules (same-bar close beyond, halfway, pivot) on the chart's bars up to their peak bar: a failure is not drawn ("hidden: pre-2.8.4 rule"), missing bars = undrawn and said so; 2.8.4+ records are never re-checked // (2.8.4, 2026-10-14 night verification) the pivot rule also on a node first seen after its peak bar closed (new events start undecided; one decision path); delta text only on drawn A labels; setVersion literal = DLT_VERSION; the Ex record label always "Ex?" // (2.8.3) A also needs the pivot rule: bullish = the peak bar makes the down pivot or its low is below the down-pivot bar's high; bearish mirror // (2.8.2) A only when the node is in the lower half of its peak bar (bullish) / upper half (bearish) // (2.8.1) delta text only on the labelled A nodes (not every row); I / I? / A? labels removed and no AE / AE? and no Ex? marks on the chart (still recorded); only A + its bubble - absorption only, consistent with the A bubbles // (2.8.0, CR5200) the middle right-side column, delta text per row, margin owner without a Dealer Profile // (2.7.1, 2026-10-13, Rassul screenshot: "[offscreen]" A labels sat on circles and swing labels when the chart was scrolled back) an A whose peak bar is off screen gets NO profile label at all - nothing is drawn for it; it stays in the record and the status (suppressed:offscreen)   // (2.7.0, Rassul 2026-10-13 "the thin extreme seems promising for exhaustion signals especially after absorbtion or trapped detla" / "this looks good, lets implement this") the native, non-repainting thin-extreme exhaustion mark Ex? (E record lines, LRA-DeltaEx-<MKT>.csv gate)   // (2.6.3, audit F3 persistence) the signal record file is named by the sanitized FULL contract (<MKT>-<contract>-<spb>-signals-<session>.csv): a new expiry never loads the prior contract's records; root-only files are no longer read or imported   // (2.6.2) absorption = the node's own peak bar closes green above a selling node / red below a buying node (Rassul 2026-10-13 05:16); a later bar can no longer make A; records written before 2.6.2 keep their A circles   // (2.6.1) 2.5.7 external-audit port: a latch whose tick re-key fails is dropped (never compared on a stale grid); old-session lines owed at 17:00 are merged per file, never dropped, retried after the record folder is re-made, and counted   // (2.6.0) shared event IDs, confirmed-A-only bubbles, formation-aware decisions, historical A retention
static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;
static const COLOR C_VOL = 0x00243040, C_AXIS = 0x00334155;
static const COLOR C_INK = 0x00E5E7EB, C_MUTED = 0x009CA3AF;
static const COLOR C_SUPPORT = 0x0086EFAC, C_RESISTANCE = 0x00FCA5A5;
static const COLOR C_TRAP = 0x00F59E0B;   // (2.21.0) the trapped rows' outline (amber, his one warning colour)
static const COLOR C_GRAY_RING = 0x009CA3AF;
static const COLOR C_COLBG = 0x00170F0B;   // (2.27.0) the left column's solid background (dark navy, BGR)   // (2.19.0) the gray circle: a big node, winner unknown / none
struct Layout {
    int width, gap, font; bool text;
    Layout() : width(80), gap(0), font(10), text(true) {}
};
static void validateLayout(Layout& s) {
    s.width = cb::cleanWidth(cb::DELTA, s.width);        // (2.8.0) 40-250, else the default 80
    s.gap = 0;                                           // (2.8.0) the columns abut (ColumnBoard.h); an old file's gap is ignored
    s.font = s.font < 6 ? 9 : std::min(s.font, 18);
}
static bool g_drawLate = false;   // (2.23.0) THIS chart's "Draw late A (not tested)" - set from the chart's settings at the top of every draw (one DLL, charts drawn one at a time)
static long long g_sabGraceMs = 90000;   // (2.22.0) no attacker-pane post for this long before this plugin decides a shared A itself (tests set 0)
static std::int64_t steadyMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
static std::string safeField(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i)
        if (s[i] == '|' || s[i] == ',' || s[i] == '\n' || s[i] == '\r') s[i] = ' ';
    return s;
}
static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string token;
    std::istringstream in(s);
    while (std::getline(in, token, sep)) {
        if (!token.empty() && token.back() == '\r') token.pop_back();
        out.push_back(token);
    }
    return out;
}
static bool integer(const std::string& s, int& out) {
    std::istringstream in(s); long long value;
    if (!(in >> value)) return false;
    in >> std::ws;
    if (!in.eof() || value < INT_MIN || value > INT_MAX) return false;
    out = static_cast<int>(value); return true;
}
static long long fileStamp(const std::string& path) {
#if defined(_WIN32)
    struct _stat64 st;
    if (_stat64(path.c_str(), &st) != 0) return -1;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return -1;
#endif
    return static_cast<long long>(st.st_mtime) * 1000003LL + static_cast<long long>(st.st_size);
}
static bool atomicWrite(const std::string& path, const std::string& contents) {
    const std::string temporary = path + ".tmp";
    {
        std::ofstream f(temporary.c_str(), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << contents; f.flush(); if (!f) return false;
    }
#if defined(_WIN32)
    if (MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#else
    if (std::rename(temporary.c_str(), path.c_str()) == 0) return true;
#endif
    std::remove(temporary.c_str()); return false;
}
// Market alias is only for existing Dealer Profile layout interoperability.
// It NEVER chooses the tick size or drives delta classification.
static std::string marketName(std::string r) {
    for (std::size_t i = 0; i < r.size(); ++i) r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return r;
}
static double marketTick(const std::string& m) {
    return m == "ES" || m == "NQ" ? 0.25 : m == "CL" ? 0.01 : m == "NG" ? 0.001 : m == "GC" ? 0.1 :
           m == "HG" ? 0.0005 : m == "EU" ? 0.00005 : 0.0;
}
// (2.5.0, Rassul 2026-10-09 13:44 "why does the code change signals and take away the circle once the rules have been
// satisfied") a decided signal is written to this session's record and never undone by the sliding 90-min profile:
// later builds that see the same price zone undecided show the recorded result, and an absorption's circle stays
// drawn even when the node drops out of the profile. Cleared at the 17:00 session change.
struct Latch {
    dp::Tick low, high; std::string code; bool support; RTDATE peakTime; double price; bool sell;
    bool candSupport;   // legacy record metadata only; bubble colour always uses final confirmed-A support
    RTDATE formedTime; long long formedT; // frozen last formation bar, distinct from biggest-contribution peak
    // (2.5.4) the record only - never read by the drawing: id (creation order), CT-clock bar ends (epoch-as-UTC, TapeFlow's
    // convention), the node's net delta and multiple, where it came from, and whether its decided line is already queued
    long long id, tFirst, tDecided, decidedBy, peakT; double net, mult; bool zone, logged;
    std::string version;   // (2.8.5) the version that wrote the decision (D line), DLT_VERSION for one decided here
    bool tent = false;     // (2.16.0) shown before its pivot is confirmed: the close rule holds and the pivot holds on the bars so far
    bool shownTent = false, failLogged = false; long long failT = 0;   // (2.17.0) ever shown as a tentative A (TA|S line) / failed (TA|X line, at failT)
    char conf = 'S';       // (2.18.0) which candle closed beyond the node: 'S' = its own (same), 'N' = the next one; older records = same
    char rule = 0;         // (2.23.0) 'R' = the researched A (decided on its candle / the next one, drawn), 'L' = the old late path (code AL, logged only), 0 = an older record
    long long delayBars = 0, delayMin = 0;   // (2.23.0) decision candle minus the peak candle, in candles / minutes
    Latch() : low(0), high(0), support(false), peakTime(0), price(0), sell(false), candSupport(false), id(-1), tFirst(0), tDecided(0), decidedBy(0),
        peakT(0), net(0), mult(0), zone(false), logged(false), version(DLT_VERSION) { formedTime = 0; formedT = 0; }   // (2.8.5) in memory = decided here; a parsed line sets its own
};
// (2.5.1) the record lives OUTSIDE the per-chart state, keyed by symbol and bar size: a chart-state reset (IRT's host
// switching contexts, a periodicity label read empty, two charts sharing one object) no longer wipes it, and the circle is
// stored by bar TIME (re-found on every draw) so a chart reload that shifts bar numbers cannot move it.
// (2.7.0) one thin-extreme exhaustion candidate. Frozen when first logged (C); decided once (P printed / X cancelled or expired).
// Keyed by side + the extreme bar's raw stamp; never re-decided, never moved (the mark sits on the extreme bar's stamp).
struct ExMark {
    long long id = -1; bool top = true; RTDATE extRaw = 0; long long extT = 0;
    char status = 'C'; RTDATE decRaw = 0; long long decT = 0; std::string reason;
    double extreme = 0, levelPx = 0, thin = 0, fade = -1, ctxPx = 0; long long levelTicks = -1, ctxT = 0;
    std::string level, mark = "none", params; char ctx = 'N'; bool thinOk = false, fadeOk = false, live = false; int W = 3;
    std::string drawn = "logged";   // render disposition (status only)
};
// (2.7.0) AE: one decision per confirmed A id - Y found / N not found / U not measurable; label frozen at decision
struct AeRec { char result = 'U'; long long decT = 0; RTDATE decRaw = 0, peakRaw = 0; double lo = 0, hi = 0, early = 0, late = 0, fade = -1;
               int bars = 0; double fadeX = 0.5; bool sellNode = false; std::string label = "A", params; };
struct LatchBook {
    std::vector<Latch> v; long long session = -1;
    std::map<long long, AeRec> ae;   // (2.7.0) A id -> its attacker-exhaustion decision (once)
    std::map<long long, RTDATE> aeThrough;   // (2.7.0) A id -> last bar checked (memory only)
    std::vector<ExMark> ex; long long exNext = 0, exLoaded = 0, exRejected = 0; RTDATE exThrough = 0; std::string exNote;   // (2.7.0)
    // (2.5.4) the session's record file: lines waiting to be appended (kept until a flushed, good write), next id, diagnostics
    std::vector<std::string> journal; std::string path, note; long long nextId = 0, written = 0, loaded = 0, rejected = 0;
    // (2.5.7, audit F6) an old session's lines that could not be written at the 17:00 change: kept and retried, never dropped
    std::vector<std::pair<std::string, std::vector<std::string> > > carry;
    double tick = 0;                        // (2.5.7, audit F5) the tick the in-memory keys were made with
    // (2.8.5) display only (memory; the record is never changed): the pre-2.8.4 re-check result per A id (cached once the bars were
    // on the chart), and every confirmed A's display disposition: "" = holds its candle's bubble, "merged:<id>", "hidden: pre-2.8.4
    // rule (<rule>)", "undrawn: pre-2.8.4 re-check - bars not on chart"
    std::map<long long, dp::Recheck> recheck; std::map<long long, std::string> show;
    // (2.20.0) the wick absorption A: candles decided (bar key), candles waiting for the order feed (first seen, steady ms), the
    // half-hour norm observations (seed + own) and their unwritten lines, counters for the status
    std::set<long long> wickDone; std::map<long long, long long> wickWait; std::vector<absl::Obs> obs; bool obsLoaded = false;
    std::vector<std::string> obsJournal; long long wickEval = 0, wickCand = 0, wickMarks = 0, wickNoFeed = 0, wickLearning = 0, wickNotBook = 0;
    std::string wickLast, obsNote;
    // (2.22.0) the SHARED A: the record host (one per market / bar size), candles judged here (only with no attacker pane on the chart)
    sab::Host sab; std::set<long long> sabDone; bool sabDir = false, sabBasHere = false; long long sabStartMs = -1;
    // (2.21.0) the trapped traders read: decided candles (bar key * 2 + side), the decided traps (record TR lines), counters
    // (2.23.0) the researched A: every closed candle of the session is judged ONCE, in order; raThrough = the last judged candle's end (CT clock,
    // from the RW lines after a restart, so a candle judged before a restart is never judged again); raCache = closed candles' VAP rows
    long long raThrough = 0, raJudged = 0, raFound = 0, raLate = 0, raOld = 0; std::map<long long, dp::Bar> raCache; std::string raNote;
    // (2.27.0, Rassul 15:16 CT "allow it to repaint if the rules are not met" -> "live, then confirm") the LIVE A: the researched-A rules
    // run on the FORMING candle every build (its own A, or the previous candle's next-candle A); display only - no record line, no
    // SignalBoard event; replaced every build, so it can appear and vanish until its candle closes, then the closed-candle decision rules
    struct LiveA { dp::Tick lo = 0, hi = 0; bool sell = true; RTDATE peak = 0; double price = 0; };
    std::vector<LiveA> live; long long liveRuns = 0;
    std::set<long long> trapDone; std::vector<trapl::Rec> traps; long long trapNext = 0, trapEval = 0, trapFired = 0, trapLearning = 0, trapAccepted = 0,
        trapLight = 0, trapLoaded = 0, trapRejected = 0; std::string trapLast;
};
// ---- (2.5.4) the session signal record. One pipe-separated line per event, append-only, never rewritten:
//   F = the candidate first appeared (code "A?"), D = it was decided (A / I / Acc / Dst). A line is complete only when it ends
//   in a newline; a torn last line is ended before the next append and ignored when read. Several roots of one market (ES and
//   MES) may share a file: every line names its root and its id within that root's book.
static const int REC_FIELDS = 24; // legacy 22-column records accepted; new fields formed_t and formed_raw (F lines and pre-2.18.0 D lines)
static const int REC_FIELDS_CONF = 25; // (2.18.0) a D line ends in conf=same|next (the candle whose close made the A)
static const int REC_FIELDS_RULE = 29; // (2.23.0) + rule=research|old, delay_bars=N, delay_min=N, late=0|1 (an old-path A has code AL)
static const std::size_t REC_MAX_BYTES = 16u * 1024u * 1024u;
static const std::size_t REC_MAX_LATCHES = 5000; // exact event identity can admit repeated distinct episodes at the same price
static const char* recordHeader() {
    return "t_first|t_decided|code|side|bias|zone_lo|zone_hi|tick|net_delta|multiple|decided_by_t|version|event|root|id|peak_t|peak_raw|price|lo_ticks|hi_ticks|node_side|source|formed_t|formed_raw|conf|rule|delay_bars|delay_min|late";
}
// (2.23.0) the 2.18.0-2.22.0 header (25 columns): upgraded once before the first 29-field line (lesson 66)
static const char* recordHeader25() {
    return "t_first|t_decided|code|side|bias|zone_lo|zone_hi|tick|net_delta|multiple|decided_by_t|version|event|root|id|peak_t|peak_raw|price|lo_ticks|hi_ticks|node_side|source|formed_t|formed_raw|conf";
}
// (2.18.0) the 2.17.0-and-older header (24 columns): a file that still has it is upgraded once before the first 25-field line (lesson 66)
static const char* recordHeaderOld() {
    return "t_first|t_decided|code|side|bias|zone_lo|zone_hi|tick|net_delta|multiple|decided_by_t|version|event|root|id|peak_t|peak_raw|price|lo_ticks|hi_ticks|node_side|source|formed_t|formed_raw";
}
static const char* confToken(char c) { return c == 'N' ? "conf=next" : c == 'W' ? "conf=wick" : "conf=same"; }   // (2.20.0) wick
// (2.6.3, audit F3 persistence part) the full contract as a file-name token: ASCII letters, digits, '_', '-' and '.' are kept,
// anything else (spaces, path separators, Windows-reserved characters, non-ASCII bytes) becomes '_'. Locale-independent; capped
// at 64 characters; an empty symbol is "unknown-contract".
static std::string contractToken(const std::string& symbol) {
    std::string out;
    for (std::size_t i = 0; i < symbol.size() && out.size() < 64; ++i) {
        const unsigned char c = static_cast<unsigned char>(symbol[i]);
        const bool keep = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        out += keep ? static_cast<char>(c) : '_';
    }
    return out.empty() ? "unknown-contract" : out;
}
// (2.6.3) <MKT>-<contract>-<spb>-signals-<session>.csv, e.g. ES-ESZ26-60-signals-20735.csv. The 2.5.x <MKT>-<spb>-signals-* and
// 2.6.0-2.6.2 <MKT>-<spb>-sync260-signals-* files carry no contract in their name; they stay on disk untouched and are not read.
static std::string contractRecordName(const std::string& market, const std::string& contract, int secondsPerBar, long long session) {
    return market + "-" + contractToken(contract) + "-" + std::to_string(secondsPerBar) + "-signals-" + std::to_string(session) + ".csv";
}
static bool confirmedAbsorption(const Latch& L) { return L.code == "A" && L.decidedBy > 0 && L.rule == 'R'; }   // (2.23.0) only the researched A
// (2.23.0) an A of the old (late) path: code AL, or an A record written before 2.23.0 (no rule field)
static bool oldPathA(const Latch& L) { return L.decidedBy > 0 && (L.code == "AL" || (L.code == "A" && L.rule != 'R')); }
// (2.23.0) drawn: the researched A; the old-path A only with "Draw late A (not tested)" ticked on this chart; never a tentative A (2.16.0 repaint removed)
static bool drawableAbsorption(const Latch& L) { return confirmedAbsorption(L) || (g_drawLate && oldPathA(L)); }
static const Latch* eventById(const LatchBook& bk, long long id) {
    for (const Latch& L : bk.v) if (L.id == id) return &L;
    return NULL;
}
static bool sameEvent(const Latch& L, dp::Tick low, dp::Tick high, bool sell, bool zone, RTDATE peak) {
    return L.low == low && L.high == high && L.sell == sell && L.zone == zone && L.peakTime == peak;
}
static std::string num(double v, const char* format) { char b[64]; std::snprintf(b, sizeof(b), format, v); return b; }
static std::string recordLine(const Latch& L, char event, double tick, const std::string& root) {
    const bool decidedLine = event == 'D';
    std::string code = L.code;
    if (!decidedLine && (code.empty() || code.back() != '?')) code += '?';   // the F line is always the candidate ("A?")
    std::ostringstream o; o.imbue(std::locale::classic());
    o << (L.tFirst > 0 ? L.tFirst : 0) << '|' << (decidedLine ? L.tDecided : 0) << '|' << code << '|'
      << (L.support ? "support" : "resistance") << '|' << (L.support ? "bullish" : "bearish") << '|'
      << num(static_cast<double>(L.low) * tick, "%.17g") << '|' << num(static_cast<double>(L.high) * tick, "%.17g") << '|'
      << num(tick, "%.17g") << '|' << num(L.net, "%.17g") << '|' << num(L.mult, "%.17g") << '|' << (decidedLine ? L.decidedBy : 0) << '|'
      << DLT_VERSION << '|' << event << '|' << root << '|' << L.id << '|' << L.peakT << '|'
      << static_cast<unsigned long long>(L.peakTime) << '|' << num(L.price, "%.17g") << '|' << L.low << '|' << L.high << '|'
      << (L.sell ? "sell" : "buy") << '|' << (L.zone ? "zone" : "node") << '|' << L.formedT << '|' << static_cast<unsigned long long>(L.formedTime);
    if (decidedLine) o << '|' << confToken(L.conf);   // (2.18.0) at the END, so 24-column readers keep their positions
    if (decidedLine && L.rule) o << '|' << (L.rule == 'R' ? "rule=research" : "rule=old") << "|delay_bars=" << L.delayBars << "|delay_min=" << L.delayMin << "|late=" << (L.delayBars > 1 ? 1 : 0);   // (2.23.0)
    return o.str();
}
// (2.17.0, audit B finding 1) the tentative-A record, in the same session file (append-only, never rewritten):
//   TA|status|version|root|id|peak_t|peak_raw|price|bias|multiple|t|lo_ticks|hi_ticks      (13 fields)
//   status S = first SHOWN as a tentative A (the bubble appeared before its pivot was confirmed); X = FAILED (its swing pivot was
//   confirmed against it - the bubble went for good). A tentative A that becomes final gets its normal D line. t = the chart's last
//   bar end (CT clock) when written. 2.16.0 and older count these lines as rejected and skip them.
static const int TA_FIELDS = 14;       // (2.18.0) + conf=same|next at the end
static const int TA_FIELDS_OLD = 13;   // 2.17.0 TA lines (= same)
static std::string taLine(const Latch& L, char status, long long t, const std::string& root) {
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "TA|" << status << '|' << DLT_VERSION << '|' << root << '|' << L.id << '|' << L.peakT << '|' << static_cast<unsigned long long>(L.peakTime) << '|'
      << num(L.price, "%.17g") << '|' << (L.sell ? "bullish" : "bearish") << '|' << num(L.mult, "%.17g") << '|' << t << '|' << L.low << '|' << L.high
      << '|' << confToken(L.conf);   // (2.18.0)
    return o.str();
}
// strict full-token parsing (checklist #4): no leading blanks, no trailing junk, in range, finite
static bool parseWhole(const std::string& t, long long lo, long long hi, long long& out) {
    if (t.empty() || t.size() > 24 || std::isspace(static_cast<unsigned char>(t[0]))) return false;
    errno = 0; char* e = NULL; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno != 0 || e != t.c_str() + t.size() || v < lo || v > hi) return false;
    out = v; return true;
}
static bool parseReal(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40 || std::isspace(static_cast<unsigned char>(t[0]))) return false;
    errno = 0; char* e = NULL; const double v = std::strtod(t.c_str(), &e);
    if (errno != 0 || e != t.c_str() + t.size() || !dp::finite(v)) return false;
    out = v; return true;
}
static std::vector<std::string> fields(const std::string& line) {
    std::vector<std::string> c; std::size_t a = 0;
    for (;;) {
        const std::size_t p = line.find('|', a);
        c.push_back(line.substr(a, p == std::string::npos ? std::string::npos : p - a));
        if (p == std::string::npos) break;
        a = p + 1;
    }
    return c;
}
// One record line -> its parts. false = a malformed line (counted, skipped).
struct RecordEvent { char event; Latch latch; std::string root; };
static bool parseRecordLine(const std::string& line, double chartTick, RecordEvent& r) {
    const std::vector<std::string> c = fields(line);
    const int nfld = static_cast<int>(c.size());
    if ((nfld != REC_FIELDS && nfld != REC_FIELDS_CONF && nfld != REC_FIELDS_RULE && c.size() != 22) || c[12].size() != 1 || (c[12][0] != 'F' && c[12][0] != 'D')) return false;
    long long rDelayB = 0, rDelayM = 0; char rRule = 0;
    if (nfld == REC_FIELDS_RULE) {   // (2.23.0) rule / delay_bars / delay_min / late
        if (c[12][0] != 'D' || (c[24] != "conf=same" && c[24] != "conf=next")) return false;
        if (c[25] == "rule=research") rRule = 'R'; else if (c[25] == "rule=old") rRule = 'L'; else return false;
        if (c[26].compare(0, 11, "delay_bars=") != 0 || c[27].compare(0, 10, "delay_min=") != 0 || (c[28] != "late=0" && c[28] != "late=1")) return false;
        if (!parseWhole(c[26].substr(11), 0, 100000, rDelayB) || !parseWhole(c[27].substr(10), 0, 10000000, rDelayM)) return false;
        if ((rRule == 'R') != (c[2] == "A") || (rRule == 'L') != (c[2] == "AL")) return false;
    }
    if (static_cast<int>(c.size()) == REC_FIELDS_CONF && (c[12][0] != 'D' || (c[24] != "conf=same" && c[24] != "conf=next" && c[24] != "conf=wick"))) return false;   // (2.18.0) (2.20.0) + wick
    Latch& L = r.latch; r.event = c[12][0]; r.root = c[13];
    if (r.root.empty()) return false;
    long long tf, td, by, id, pt, praw, lo, hi;
    const long long T_MAX = 32503680000LL;                                  // year 3000: CT-clock epoch seconds
    const unsigned long long rawMax = static_cast<unsigned long long>(std::numeric_limits<RTDATE>::max());
    const long long rawLimit = rawMax > static_cast<unsigned long long>(LLONG_MAX) ? LLONG_MAX : static_cast<long long>(rawMax);
    if (!parseWhole(c[0], 0, T_MAX, tf) || !parseWhole(c[1], 0, T_MAX, td) || !parseWhole(c[10], 0, T_MAX, by) ||
        !parseWhole(c[14], 0, 1000000, id) || !parseWhole(c[15], 0, T_MAX, pt) ||
        !parseWhole(c[16], 1, rawLimit, praw) ||
        !parseWhole(c[18], -9000000000000000LL, 9000000000000000LL, lo) || !parseWhole(c[19], -9000000000000000LL, 9000000000000000LL, hi) || lo > hi)
        return false;
    double zl, zh, tk, net, mult, price;
    if (!parseReal(c[5], zl) || !parseReal(c[6], zh) || !parseReal(c[7], tk) || !parseReal(c[8], net) || !parseReal(c[9], mult) ||
        !parseReal(c[17], price) || tk <= 0 || tk >= 1000 || mult < 0 || zl > zh) return false;
    const std::string& code = c[2];
    if (r.event == 'F' ? (code != "A?" && code != "I?" && code != "Acc?" && code != "Dst?") : (code != "A" && code != "I" && code != "Acc" && code != "Dst" && !(code == "AL" && nfld == REC_FIELDS_RULE))) return false;
    if (r.event == 'D' ? (td == 0 || by == 0) : (td != 0 || by != 0)) return false;
    const bool support = c[3] == "support";
    if ((!support && c[3] != "resistance") || c[4] != (support ? "bullish" : "bearish")) return false;
    if ((c[20] != "buy" && c[20] != "sell") || (c[21] != "node" && c[21] != "zone")) return false;
    L.code = code; L.support = support; L.tFirst = tf; L.tDecided = td; L.decidedBy = by; L.id = id; L.peakT = pt;
    L.peakTime = static_cast<RTDATE>(praw); L.price = price; L.net = net; L.mult = mult; L.sell = c[20] == "sell"; L.zone = c[21] == "zone";
    L.low = lo; L.high = hi; L.version = c[11].substr(0, 16);   // (2.8.5) an unreadable version is treated as old (re-checked)
    L.conf = nfld >= REC_FIELDS_CONF && c[24] == "conf=next" ? 'N' : nfld >= REC_FIELDS_CONF && c[24] == "conf=wick" ? 'W' : 'S';   // (2.18.0) older lines = same
    L.rule = rRule; L.delayBars = rDelayB; L.delayMin = rDelayM;   // (2.23.0)
    if (static_cast<int>(c.size()) >= REC_FIELDS) {
        long long ft, fr;
        if (!parseWhole(c[22], 0, T_MAX, ft) || !parseWhole(c[23], 0, rawLimit, fr)) return false;
        L.formedT = ft; L.formedTime = static_cast<RTDATE>(fr);
    }
    if (std::fabs(tk - chartTick) > 1e-9 * std::max(tk, chartTick)) {      // written on another tick size: the prices decide the zone
        try { L.low = dp::priceKey(zl, chartTick); L.high = dp::priceKey(zh, chartTick); } catch (...) { return false; }
        if (L.low > L.high) return false;
    }
    return true;
}
// ---- (2.7.0) the exhaustion record: E lines in the SAME contract-named session file as the F / D lines (never rewritten).
//   E|status|version|root|id|side|ext_t|ext_raw|dec_t|dec_raw|extreme|tick|level|level_px|level_ticks|thin|fade|thin_ok|fade_ok|
//     ctx|ctx_px|ctx_t|mark|mode|reason|params                                              (26 fields; NOTES.md is the contract)
//   status C = a candidate first logged at its extreme bar's close; P = printed (the first bar closing back); X = cancelled /
//   expired. A C line is written once per candidate, then exactly one P or X line. 2.6.3 and earlier skip these lines.
static const int EX_FIELDS = 26;
static const std::size_t EX_MAX_MARKS = 5000;
static std::string exNum(double v) { char b[64]; std::snprintf(b, sizeof(b), "%.10g", v); return b; }
static std::string exRatio(double x) {   // 0.42 -> "0.42x", 0.003 -> "<0.01x"
    if (!(x >= 0)) return "n/a";
    if (x < 0.005) return "<0.01x";
    char b[32]; std::snprintf(b, sizeof(b), "%.2fx", x); return b;
}
static std::string exLine(const ExMark& m, char status, double tick, const std::string& root) {
    std::ostringstream o; o.imbue(std::locale::classic());
    const bool decision = status != 'C';
    o << "E|" << status << '|' << DLT_VERSION << '|' << root << '|' << m.id << '|' << (m.top ? "top" : "bottom") << '|'
      << m.extT << '|' << static_cast<unsigned long long>(m.extRaw) << '|'
      << (decision ? m.decT : m.extT) << '|' << static_cast<unsigned long long>(decision ? m.decRaw : m.extRaw) << '|'
      << exNum(m.extreme) << '|' << exNum(tick) << '|' << safeField(m.level) << '|' << exNum(m.levelPx) << '|' << m.levelTicks << '|'
      << exNum(m.thin) << '|' << exNum(m.fade) << '|' << (m.thinOk ? 1 : 0) << '|' << (m.fadeOk ? 1 : 0) << '|'
      << (m.ctx == 'A' ? "A" : m.ctx == 'T' ? "T" : "none") << '|' << exNum(m.ctxPx) << '|' << m.ctxT << '|'
      << m.mark << '|' << (m.live ? "live" : "backfill") << '|' << (decision ? safeField(m.reason) : std::string()) << '|' << safeField(m.params);
    return o.str();
}
static int paramW(const std::string& params) {
    const std::size_t at = params.find("W=");
    int w = 3; if (at == std::string::npos) return w;
    std::size_t end = params.find(';', at); if (end == std::string::npos) end = params.size();
    if (!integer(params.substr(at + 2, end - at - 2), w) || w < 1 || w > 20) w = 3;
    return w;
}
struct ExEvent { char status; ExMark m; std::string root; };
static bool parseExLine(const std::string& line, ExEvent& r) {
    const std::vector<std::string> c = fields(line);
    if (static_cast<int>(c.size()) != EX_FIELDS || c[0] != "E" || c[1].size() != 1) return false;
    r.status = c[1][0]; r.root = c[3];
    if ((r.status != 'C' && r.status != 'P' && r.status != 'X') || r.root.empty() || (c[5] != "top" && c[5] != "bottom")) return false;
    const long long T_MAX = 32503680000LL;
    const unsigned long long rawMax = static_cast<unsigned long long>(std::numeric_limits<RTDATE>::max());
    const long long rawLimit = rawMax > static_cast<unsigned long long>(LLONG_MAX) ? LLONG_MAX : static_cast<long long>(rawMax);
    long long id, et, er, dt, dr, lt, ct, tOk, fOk;
    double ext, tk, lp, thin, fade, cp;
    if (!parseWhole(c[4], 0, 1000000, id) || !parseWhole(c[6], 0, T_MAX, et) || !parseWhole(c[7], 1, rawLimit, er) ||
        !parseWhole(c[8], 0, T_MAX, dt) || !parseWhole(c[9], 1, rawLimit, dr) || !parseWhole(c[14], -1, 1000000000LL, lt) ||
        !parseWhole(c[17], 0, 1, tOk) || !parseWhole(c[18], 0, 1, fOk) || !parseWhole(c[21], 0, T_MAX, ct)) return false;
    if (!parseReal(c[10], ext) || !parseReal(c[11], tk) || !parseReal(c[13], lp) || !parseReal(c[15], thin) || !parseReal(c[16], fade) ||
        !parseReal(c[20], cp) || tk <= 0 || ext <= 0 || thin < 0) return false;
    if (c[19] != "A" && c[19] != "T" && c[19] != "none") return false;
    if (c[22] != "none" && c[22] != "Ex?" && c[22] != "Ex") return false;
    if (c[23] != "live" && c[23] != "backfill") return false;
    if (r.status == 'C' ? (dt != et || dr != er) : dr < er) return false;
    ExMark& m = r.m;
    m.id = id; m.top = c[5] == "top"; m.extT = et; m.extRaw = static_cast<RTDATE>(er); m.decT = dt; m.decRaw = static_cast<RTDATE>(dr);
    m.extreme = ext; m.level = c[12]; m.levelPx = lp; m.levelTicks = lt; m.thin = thin; m.fade = fade; m.thinOk = tOk == 1; m.fadeOk = fOk == 1;
    m.ctx = c[19] == "A" ? 'A' : c[19] == "T" ? 'T' : 'N'; m.ctxPx = cp; m.ctxT = ct; m.mark = c[22]; m.live = c[23] == "live";
    m.reason = c[24]; m.params = c[25]; m.W = paramW(m.params); m.status = r.status;
    return true;
}
// ---- (2.7.0) the AE record: one line per confirmed A, in the same session file (17 fields; NOTES.md is the contract)
//   AE|result|version|root|a_id|dec_t|dec_raw|peak_raw|node_lo|node_hi|attackers|bars|early|late|fade|fadeX|label|params
//   result Y = attackers faded inside the node (decided at dec_t, the re-test bar's close), N = no fade within aeMaxBars bars of the
//   A's peak bar, U = not measurable (the A's bars not on hand); label = what the A's profile label shows from then on (A / AE? /
//   AE) - frozen at the decision.
static const int AE_FIELDS = 18;
static std::string aeLine(long long id, const AeRec& a, const std::string& root) {
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "AE|" << a.result << '|' << DLT_VERSION << '|' << root << '|' << id << '|' << a.decT << '|' << static_cast<unsigned long long>(a.decRaw) << '|'
      << static_cast<unsigned long long>(a.peakRaw) << '|'
      << exNum(a.lo) << '|' << exNum(a.hi) << '|' << (a.sellNode ? "sellers" : "buyers") << '|' << a.bars << '|' << exNum(a.early) << '|'
      << exNum(a.late) << '|' << exNum(a.fade) << '|' << exNum(a.fadeX) << '|' << a.label << '|' << safeField(a.params);
    return o.str();
}
static void seedAeLine(const std::string& line, const std::string& root, LatchBook& bk) {
    const std::vector<std::string> c = fields(line);
    if (static_cast<int>(c.size()) != AE_FIELDS || c[0] != "AE" || c[1].size() != 1 || (c[1] != "Y" && c[1] != "N" && c[1] != "U")) { ++bk.exRejected; return; }
    if (c[3] != root) return;
    const long long T_MAX = 32503680000LL;
    const unsigned long long rawMax = static_cast<unsigned long long>(std::numeric_limits<RTDATE>::max());
    const long long rawLimit = rawMax > static_cast<unsigned long long>(LLONG_MAX) ? LLONG_MAX : static_cast<long long>(rawMax);
    long long id, dt, dr, pr, bars; double lo, hi, early, late, fade, fx;
    if (!parseWhole(c[4], 0, 1000000, id) || !parseWhole(c[5], 0, T_MAX, dt) || !parseWhole(c[6], 0, rawLimit, dr) || !parseWhole(c[7], 0, rawLimit, pr) ||
        !parseWhole(c[11], 0, 100000, bars) || !parseReal(c[8], lo) || !parseReal(c[9], hi) || !parseReal(c[12], early) || !parseReal(c[13], late) ||
        !parseReal(c[14], fade) || !parseReal(c[15], fx) || (c[10] != "buyers" && c[10] != "sellers") ||
        (c[16] != "A" && c[16] != "AE?" && c[16] != "AE") || (c[16] != "A" && c[1] != "Y")) { ++bk.exRejected; return; }
    if (bk.ae.count(id)) return;                                                        // the first decision wins
    AeRec a; a.result = c[1][0]; a.decT = dt; a.decRaw = static_cast<RTDATE>(dr); a.peakRaw = static_cast<RTDATE>(pr); a.lo = lo; a.hi = hi;
    a.sellNode = c[10] == "sellers"; a.bars = static_cast<int>(bars); a.fadeX = fx; a.early = early; a.late = late; a.fade = fade; a.label = c[16]; a.params = c[17];
    bk.ae[id] = a;
}
// one E line into the book: C creates (first wins), P / X decides a known pending candidate once (identity must match)
static void seedExLine(const std::string& line, const std::string& root, LatchBook& bk) {
    ExEvent r;
    if (!parseExLine(line, r)) { ++bk.exRejected; return; }
    if (r.root != root) return;
    ExMark* at = NULL;
    for (ExMark& m : bk.ex) if (m.id == r.m.id) { at = &m; break; }
    if (r.status == 'C') {
        if (at || bk.ex.size() >= EX_MAX_MARKS) return;
        for (const ExMark& m : bk.ex) if (m.top == r.m.top && m.extRaw == r.m.extRaw) { ++bk.exRejected; return; }
        r.m.status = 'C'; r.m.reason.clear(); bk.ex.push_back(r.m); bk.exNext = std::max(bk.exNext, r.m.id + 1); ++bk.exLoaded;
        return;
    }
    if (!at || at->top != r.m.top || at->extRaw != r.m.extRaw) { ++bk.exRejected; return; }
    if (at->status != 'C') return;                                                  // the first decision wins
    at->status = r.status; at->decRaw = r.m.decRaw; at->decT = r.m.decT; at->reason = r.m.reason;
}
// The session record text -> this root's latches in creation order. Only lines ending in a newline count; a duplicated line
// (a retried append) is ignored; the FIRST decided line of an id wins (a decided signal never changes).
// (2.5.7, audit F3) lines name the full contract; a line naming only the root (written by 2.5.6 and earlier) is taken as this contract's
static void seedFromText(const std::string& text, const std::string& root, const std::string& legacyRoot, double chartTick, LatchBook& bk) {
    const std::size_t end = text.rfind('\n');
    if (end == std::string::npos) return;
    std::map<long long, Latch> byId;
    bool hasExactRoot=false;
    if(root!=legacyRoot){
        std::istringstream scan(text.substr(0,end+1));std::string row;
        while(std::getline(scan,row)){RecordEvent r;if(parseRecordLine(row,chartTick,r)&&r.root==root){hasExactRoot=true;break;}}
    }
    std::istringstream in(text.substr(0, end + 1)); std::string line; std::vector<std::string> taRows;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || line.compare(0, 8, "t_first|") == 0) continue;
        if (line.compare(0, 2, "E|") == 0) { seedExLine(line, root, bk); continue; }
        if (line.compare(0, 3, "AE|") == 0) { seedAeLine(line, root, bk); continue; }
        if (line.compare(0, 3, "TR|") == 0) {   // (2.21.0) a decided trap: drawn again after a restart exactly as before (first line per candle + side wins)
            trapl::Rec t;
            if (!trapl::parse(line, t)) { ++bk.trapRejected; continue; }
            if (t.root != root && t.root != legacyRoot) continue;
            const long long dk = t.barT * 2 + (t.top ? 1 : 0);
            if (bk.trapDone.count(dk) || bk.traps.size() >= 2000) continue;
            bk.trapDone.insert(dk); bk.traps.push_back(t); bk.trapNext = std::max(bk.trapNext, t.id + 1); ++bk.trapLoaded;
            continue;
        }
        if (line.compare(0, 3, "RW|") == 0) {   // (2.23.0) RW|version|root|candle_end_t: every candle up to there was judged for the researched A
            const std::vector<std::string> c = fields(line); long long t = 0;
            if (c.size() != 4 || (c[2] != root && c[2] != legacyRoot) || !parseWhole(c[3], 1, 32503680000LL, t)) { ++bk.rejected; continue; }
            bk.raThrough = std::max(bk.raThrough, t); continue;
        }
        if (line.compare(0, 3, "TA|") == 0) { taRows.push_back(line); continue; }   // (2.17.0) applied once the F lines are known   // (2.7.0) attacker exhaustion inside an A   // (2.7.0) exhaustion lines: their own book
        RecordEvent r;
        if (!parseRecordLine(line, chartTick, r)) { ++bk.rejected; continue; }
        if (r.root != root && r.root != legacyRoot) continue;
        if(hasExactRoot && r.root!=root)continue; // root-only fallback must not collide with this contract's event IDs
        std::map<long long, Latch>::iterator at = byId.find(r.latch.id);
        if (r.event == 'F') {
            if (at != byId.end() || byId.size() >= REC_MAX_LATCHES) continue;
            r.latch.tDecided = 0; r.latch.decidedBy = 0; r.latch.logged = false; r.latch.candSupport = r.latch.support; byId[r.latch.id] = r.latch; ++bk.loaded;
        } else {
            if (at == byId.end()) { ++bk.rejected; continue; }
            Latch& L = at->second;
            if (L.logged) continue;
            if (L.low != r.latch.low || L.high != r.latch.high || L.sell != r.latch.sell || L.zone != r.latch.zone || L.peakTime != r.latch.peakTime ||
                L.price != r.latch.price || L.formedTime != r.latch.formedTime || L.formedT != r.latch.formedT || L.tFirst != r.latch.tFirst) { ++bk.rejected; continue; }
            L.code = r.latch.code; L.support = r.latch.support; L.tDecided = r.latch.tDecided; L.decidedBy = r.latch.decidedBy; L.logged = true;
            L.version = r.latch.version;   // (2.8.5) the DECISION's version decides whether the pre-2.8.4 re-check applies
            L.conf = r.latch.conf;         // (2.18.0) same / next candle
            L.rule = r.latch.rule; L.delayBars = r.latch.delayBars; L.delayMin = r.latch.delayMin;   // (2.23.0)
            if (L.rule == 'R') bk.raThrough = std::max(bk.raThrough, L.decidedBy);   // a researched A is decided at its candle's close: judged
        }
    }
    for (const std::string& row : taRows) {   // (2.17.0) TA|S / TA|X: the tentative history of a known id (identity must match)
        const std::vector<std::string> c = fields(row);
        long long id = 0, praw = 0, t = 0;
        const int nf = static_cast<int>(c.size());   // (2.18.0) 14 fields (conf at the end) or 2.17.0's 13
        if ((nf != TA_FIELDS && nf != TA_FIELDS_OLD) || (nf == TA_FIELDS && c[13] != "conf=same" && c[13] != "conf=next") || (c[1] != "S" && c[1] != "X") || (c[3] != root && c[3] != legacyRoot) ||
            !parseWhole(c[4], 0, 1000000, id) || !parseWhole(c[6], 1, LLONG_MAX, praw) || !parseWhole(c[10], 0, 32503680000LL, t)) { ++bk.rejected; continue; }
        std::map<long long, Latch>::iterator at = byId.find(id);
        if (at == byId.end() || static_cast<unsigned long long>(at->second.peakTime) != static_cast<unsigned long long>(praw)) { ++bk.rejected; continue; }
        at->second.shownTent = true;
        if (c[1] == "X" && !at->second.failLogged) { at->second.failLogged = true; at->second.failT = t; }
    }
    for (std::map<long long, Latch>::const_iterator i = byId.begin(); i != byId.end(); ++i) {
        bk.v.push_back(i->second); bk.nextId = std::max(bk.nextId, i->first + 1);
    }
}
static bool readWholeFile(const std::string& path, std::string& text, bool& tooLarge) {
    tooLarge = false; text.clear();
    std::ifstream f(path.c_str(), std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    const std::streamoff size = static_cast<std::streamoff>(f.tellg());
    if (size < 0) return false;
    if (static_cast<unsigned long long>(size) > REC_MAX_BYTES) { tooLarge = true; return false; }
    text.resize(static_cast<std::size_t>(size)); f.seekg(0);
    if (size > 0 && !f.read(&text[0], size)) { text.clear(); return false; }
    return true;
}
// Append whole lines in ONE write: a header on a new file, a newline first if a crash left a torn last line.
// (2.20.0) plain append (the wick-norm observations file: "session,slot,top,lots" lines, no header); false = not written, kept for retry
static bool appendPlain(const std::string& path, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    std::ofstream o(path.c_str(), std::ios::binary | std::ios::app);
    if (!o.is_open()) return false;
    for (std::size_t i = 0; i < lines.size(); ++i) { o << lines[i] << '\n'; }
    o.flush(); return static_cast<bool>(o);
}
static bool appendRecord(const std::string& path, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    bool fresh = true, torn = false;
    {
        std::ifstream t(path.c_str(), std::ios::binary | std::ios::ate);
        if (t.is_open()) {
            const std::streamoff size = static_cast<std::streamoff>(t.tellg());
            if (size > 0) { fresh = false; t.seekg(-1, std::ios::end); char ch = 0; if (t.get(ch) && ch != '\n') torn = true; }
        }
    }
    if (!fresh) {   // (2.18.0, lesson 66) a file with the 24-column header gets the 25-column one (atomic rewrite) before the first longer line
        std::string head; { std::ifstream t(path.c_str(), std::ios::binary); char b[1024]; t.read(b, sizeof(b)); head.assign(b, static_cast<std::size_t>(t.gcount())); }
        std::string oldH = std::string("\n") + recordHeaderOld() + "\n";
        if (head.find(oldH) == std::string::npos) oldH = std::string("\n") + recordHeader25() + "\n";   // (2.23.0) the 25-column header too
        if (head.find(oldH) != std::string::npos) {
            std::string all; bool big = false;
            if (!readWholeFile(path, all, big)) return false;
            const std::size_t at = all.find(oldH);
            if (at == std::string::npos) return false;
            all.replace(at + 1, oldH.size() - 2, recordHeader());
            if (!atomicWrite(path, all)) return false;
        }
    }
    std::string text;
    if (fresh) { text += "# lsDeltaProfile signals - one line per event (F first seen, D decided); times = Central clock as epoch seconds\n"; text += recordHeader(); text += '\n'; }
    else if (torn) text += '\n';
    for (std::size_t i = 0; i < lines.size(); ++i) { text += lines[i]; text += '\n'; }
    std::ofstream o(path.c_str(), std::ios::binary | std::ios::app);
    if (!o.is_open()) return false;
    o.write(text.data(), static_cast<std::streamsize>(text.size())); o.flush();
    return o.good();
}
static void makeDirectory(const std::string& path) {
#if defined(_WIN32)
    CreateDirectoryA(path.c_str(), NULL);
#else
    mkdir(path.c_str(), 0777);
#endif
}
// (2.5.3, RA1009 audit) the Globex session a bar END stamp belongs to, from the chart's local wall clock: a bar ending at or after
// 17:00 belongs to the next day's session (the same >= edge 2.5.x used). Falls back to the old epoch rule if the stamp cannot be read.
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}
static void seedFromText(const std::string& text, const std::string& root, double chartTick, LatchBook& bk) { seedFromText(text, root, root, chartTick, bk); }
// ---- (2.7.0) bounded, torn-safe reads of the nightly / LRA files (the DealerProfileAuditLogic readBoundedLines pattern): whole
// lines only, capped bytes / lines / line length; the file's stamp must be the same before and after the read, else the last good
// snapshot is kept and the read is retried later.
static bool readBoundedLines(const std::string& path, std::vector<std::string>& lines, std::size_t maxBytes, std::size_t maxLines, std::size_t maxLine) {
    lines.clear();
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open()) return false;
    std::string line; char ch = 0; std::size_t bytes = 0;
    while (in.get(ch)) {
        if (++bytes > maxBytes) { lines.clear(); return false; }
        if (ch == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (lines.size() >= maxLines) { lines.clear(); return false; }
            lines.push_back(line); line.clear();
        } else { if (line.size() >= maxLine) { lines.clear(); return false; } line += ch; }
    }
    if (in.bad()) { lines.clear(); return false; }
    if (!line.empty()) { if (lines.size() >= maxLines) { lines.clear(); return false; } lines.push_back(line); }
    return true;
}
static long long fileMtime(const std::string& path) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(path.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(path.c_str(), &st) != 0) return -1;
#endif
    return static_cast<long long>(st.st_mtime);
}
// ---- (2.7.0) LRA-DeltaEx-<MKT>.csv, written nightly by the LRA study (NOTES.md is the contract). Comma-separated:
//   VERSION,1   MARKET,<MKT>   PARAM,<name>,<value> (N L T thinX fadeX M trapX K B W requireLevel)   SHOW,<none|ctx|all>,<?|proven>
// A missing file or ANY malformed / out-of-range line = the defaults and SHOW none (computed and logged, nothing drawn).
struct ExConfig { dp::exh::Params p; std::string src = "missing"; long long stamp = -2; std::int64_t at = -1; };
static bool parseExParams(const std::vector<std::string>& lines, const std::string& market, dp::exh::Params& out, std::string& why) {
    dp::exh::Params p; bool version = false, mkt = false;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i];
        if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = split(ln, ',');
        if (!version) { if (c.size() == 2 && c[0] == "VERSION" && c[1] == "1") { version = true; continue; } why = "first line is not VERSION,1"; return false; }
        if (c.size() < 2) { why = "line " + std::to_string(i + 1) + " malformed"; return false; }
        if (c[0] == "MARKET") { if (c.size() != 2 || c[1] != market) { why = "MARKET is not " + market; return false; } mkt = true; continue; }
        if (c[0] == "SHOW") {
            if (c.size() != 3 || (c[1] != "none" && c[1] != "ctx" && c[1] != "all") || (c[2] != "?" && c[2] != "proven")) { why = "SHOW malformed"; return false; }
            p.show = c[1] == "none" ? 0 : c[1] == "ctx" ? 1 : 2; p.proven = c[2] == "proven"; continue;
        }
        if (c[0] == "SHOW_AE") {
            if (c.size() != 2 || (c[1] != "none" && c[1] != "?" && c[1] != "proven")) { why = "SHOW_AE malformed"; return false; }
            p.showAe = c[1] == "none" ? 0 : c[1] == "?" ? 1 : 2; continue;
        }
        if (c[0] != "PARAM") continue;                                              // unknown keys: forward compatible, ignored
        if (c.size() != 3) { why = "PARAM line " + std::to_string(i + 1) + " malformed"; return false; }
        const std::string& n = c[1]; const std::string& v = c[2];
        auto whole = [&](int lo, int hi, int& dst) { long long x; if (!parseWhole(v, lo, hi, x)) return false; dst = static_cast<int>(x); return true; };
        auto real = [&](double lo, double hi, double& dst) { double x; if (!parseReal(v, x) || x < lo || x > hi) return false; dst = x; return true; };
        bool ok = true;
        if (n == "N") ok = whole(2, 100, p.N);
        else if (n == "L") ok = whole(0, 200, p.L);
        else if (n == "T") ok = whole(1, 10, p.T);
        else if (n == "M") ok = whole(3, 200, p.M);
        else if (n == "B") ok = whole(1, 20, p.B);
        else if (n == "W") ok = whole(1, 20, p.W);
        else if (n == "K") { if (v == "auto") p.K = -1; else ok = whole(0, 400, p.K); }
        else if (n == "thinX") ok = real(0.0, 10.0, p.thinX);
        else if (n == "fadeX") ok = real(0.0, 10.0, p.fadeX);
        else if (n == "trapX") ok = real(0.1, 50.0, p.trapX);
        else if (n == "aeFadeX") ok = real(0.0, 10.0, p.aeFadeX);
        else if (n == "aeMinBars") ok = whole(2, 50, p.aeMinBars);
        else if (n == "aeMaxBars") ok = whole(1, 200, p.aeMaxBars);
        else if (n == "requireLevel") { int b = 0; ok = whole(0, 1, b); p.requireLevel = b == 1; }
        if (!ok) { why = "PARAM " + n + " invalid"; return false; }
    }
    if (!version) { why = "empty"; return false; }
    if (!mkt) { why = "no MARKET line"; return false; }
    out = p; return true;
}
static std::map<std::string, ExConfig>& exConfigs() { static std::map<std::string, ExConfig> m; return m; }
static const ExConfig& exConfig(const std::string& directory, const std::string& market) {
    ExConfig& C = exConfigs()[market];
    const std::int64_t now = steadyMs();
    if (C.at >= 0 && now - C.at < 5000) return C;
    C.at = now;
    if (directory.empty() || market.empty()) { C.p = dp::exh::Params(); C.src = "missing"; C.stamp = -1; return C; }
    const std::string path = directory + "LRA-DeltaEx-" + market + ".csv";
    const long long st = fileStamp(path);
    if (st == C.stamp) return C;
    if (st < 0) { C.p = dp::exh::Params(); C.src = "missing"; C.stamp = st; return C; }
    std::vector<std::string> lines;
    const bool read = readBoundedLines(path, lines, 64 * 1024, 512, 512);
    if (fileStamp(path) != st) return C;                                            // being replaced: keep the last good, retry
    C.stamp = st;
    dp::exh::Params p; std::string why = read ? "" : "too large or unreadable";
    if (read && parseExParams(lines, market, p, why)) { C.p = p; C.src = "file"; }
    else { C.p = dp::exh::Params(); C.src = "garbled: " + why; }
    return C;
}
static std::string exParamText(const ExConfig& C) {
    const dp::exh::Params& p = C.p; std::ostringstream o; o.imbue(std::locale::classic());
    o << "N=" << p.N << ";L=" << p.L << ";T=" << p.T << ";thinX=" << exNum(p.thinX) << ";fadeX=" << exNum(p.fadeX) << ";M=" << p.M
      << ";trapX=" << exNum(p.trapX) << ";K=" << (p.K < 0 ? std::string("auto") : std::to_string(p.K)) << ";B=" << p.B << ";W=" << p.W
      << ";req=" << (p.requireLevel ? 1 : 0) << ";show=" << (p.show == 0 ? "none" : p.show == 1 ? "ctx" : "all") << ";tag=" << (p.proven ? "proven" : "?")
      << ";src=" << (C.src == "file" ? "file" : C.src == "missing" ? "default" : "garbled");
    return o.str();
}
static std::string aeParamText(const ExConfig& C) {
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "aeFadeX=" << exNum(C.p.aeFadeX) << ";aeMinBars=" << C.p.aeMinBars << ";aeMaxBars=" << C.p.aeMaxBars << ";show_ae=" << (C.p.showAe == 0 ? "none" : C.p.showAe == 1 ? "?" : "proven")
      << ";src=" << (C.src == "file" ? "file" : C.src == "missing" ? "default" : "garbled");
    return o.str();
}
// ---- (2.7.0) the levels the extreme is measured against, read as the other LRA plugins read them:
//   LRA-KeyLevels-<MKT>.csv (lsDealerProfile 2.8.0+, the chart's own key levels): "KEYLV,1,<unix>" then L|<name>|<price>|<pivot>;
//     older than 18 h = not used.
//   LRA-Dealer-<MKT>.csv (LRA Reader, every 5 min): LEVEL|<label>|<px>|... and the 3 largest-|gamma| NODE|<strike>|<gamma>|...
//   LRA-Levels-<MKT>.csv (lra.level_touch): LEVEL|<code>|<name>|<price>|... (STATE|STALE = not used).
//   The two MenthorQ files are used only when their mtime is within 18 h and the Dealer file's SYMBOL is the chart's contract
//   (same dated contract, or a continuous chart / no SYMBOL); in a roll they are skipped (no offset is guessed here).
struct ExLevelSource { std::vector<dp::exh::Level> v; long long stamp = -2, at = 0; std::string sym; bool stale = false; };
struct ExLevelCache { ExLevelSource key, dealer, mq; std::int64_t checked = -1; };
static std::map<std::string, ExLevelCache>& exLevelCaches() { static std::map<std::string, ExLevelCache> m; return m; }
static bool monthYearOf(const std::string& s, char& mon, int& year) {
    const std::size_t n = s.size();
    auto isM = [](char c) { return c != 0 && std::strchr("FGHJKMNQUVXZ", c) != NULL; };
    auto dig = [](char c) { return c >= '0' && c <= '9'; };
    if (n >= 4 && isM(s[n - 3]) && dig(s[n - 2]) && dig(s[n - 1])) { mon = s[n - 3]; year = (s[n - 1] - '0'); return true; }
    if (n >= 4 && isM(s[n - 2]) && dig(s[n - 1]) && !dig(s[n - 3])) { mon = s[n - 2]; year = s[n - 1] - '0'; return true; }
    return false;
}
static bool sameContractLite(const std::string& chart, const std::string& file) {
    if (chart.empty() || file.empty()) return true;
    std::string a, b;
    for (char ch : chart) a += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    for (char ch : file) b += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    if (a == b) return true;
    char ma = 0, mb = 0; int ya = 0, yb = 0;
    if (!monthYearOf(a, ma, ya) || !monthYearOf(b, mb, yb)) return true;          // a continuous chart is the front contract
    return ma == mb && ya == yb;                                                   // ESZ6 (Rithmic) == EPZ26 (CQG)
}
static void exLoadSource(const std::string& path, ExLevelSource& S, int kind) {
    const long long st = fileStamp(path);
    if (st == S.stamp) return;
    if (st < 0) { S = ExLevelSource(); S.stamp = st; return; }
    std::vector<std::string> lines;
    if (!readBoundedLines(path, lines, 256 * 1024, 4096, 4096) || fileStamp(path) != st) return;   // keep the last good snapshot
    ExLevelSource n; n.stamp = st; n.at = fileMtime(path);
    std::vector<std::pair<double, dp::exh::Level> > nodes;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i];
        if (kind == 0) {
            if (i == 0) { long long t = 0; if (ln.compare(0, 8, "KEYLV,1,") != 0 || !parseWhole(ln.substr(8), 1, 32503680000LL, t)) return; n.at = t; continue; }
            const std::vector<std::string> c = fields(ln); double px;
            if (c.size() >= 4 && c[0] == "L" && !c[1].empty() && c[1].size() <= 40 && parseReal(c[2], px) && px > 0) n.v.push_back(dp::exh::Level{ c[1], px });
            continue;
        }
        const std::vector<std::string> c = fields(ln); double px, g;
        if (c.size() >= 2 && c[0] == "SYMBOL") n.sym = c[1];
        if (c.size() >= 2 && c[0] == "STATE" && c[1] == "STALE") n.stale = true;
        if (kind == 1 && c.size() >= 3 && c[0] == "LEVEL" && c[1] != "NONE" && parseReal(c[2], px) && px > 0) n.v.push_back(dp::exh::Level{ c[1].substr(0, 40), px });
        if (kind == 1 && c.size() >= 8 && c[0] == "NODE" && parseReal(c[1], px) && px > 0 && parseReal(c[2], g)) nodes.push_back(std::make_pair(std::fabs(g), dp::exh::Level{ "G " + c[1].substr(0, 20), px }));
        if (kind == 2 && c.size() >= 4 && c[0] == "LEVEL" && !c[1].empty() && parseReal(c[3], px) && px > 0) n.v.push_back(dp::exh::Level{ c[1].substr(0, 40), px });
    }
    std::stable_sort(nodes.begin(), nodes.end(), [](const std::pair<double, dp::exh::Level>& a, const std::pair<double, dp::exh::Level>& b) { return a.first > b.first; });
    for (std::size_t i = 0; i < nodes.size() && i < 3; ++i) if (nodes[i].first > 0) n.v.push_back(nodes[i].second);
    S = n;
}
static std::vector<dp::exh::Level> exLevels(const std::string& directory, const std::string& market, const std::string& contract, std::string& note) {
    std::vector<dp::exh::Level> out; note.clear();
    if (directory.empty() || market.empty()) { note = "no level folder"; return out; }
    ExLevelCache& C = exLevelCaches()[market];
    const std::int64_t now = steadyMs();
    if (C.checked < 0 || now - C.checked >= 5000) {
        C.checked = now;
        exLoadSource(directory + "LRA-KeyLevels-" + market + ".csv", C.key, 0);
        exLoadSource(directory + "LRA-Dealer-" + market + ".csv", C.dealer, 1);
        exLoadSource(directory + "LRA-Levels-" + market + ".csv", C.mq, 2);
    }
    const long long wall = static_cast<long long>(std::time(NULL)), FRESH = 18 * 3600;
    auto fresh = [&](const ExLevelSource& S) { return S.stamp >= 0 && S.at > 0 && wall - S.at <= FRESH && S.at - wall <= 600; };
    int used = 0;
    if (fresh(C.key)) { out.insert(out.end(), C.key.v.begin(), C.key.v.end()); ++used; }
    const bool contractOk = sameContractLite(contract, C.dealer.sym);
    if (!contractOk) note = "MenthorQ levels skipped (file is " + C.dealer.sym + ")";
    if (contractOk && fresh(C.dealer)) { out.insert(out.end(), C.dealer.v.begin(), C.dealer.v.end()); ++used; }
    if (contractOk && fresh(C.mq) && !C.mq.stale) { out.insert(out.end(), C.mq.v.begin(), C.mq.v.end()); ++used; }
    if (note.empty()) note = std::to_string(out.size()) + " levels from " + std::to_string(used) + " files";
    return out;
}
static std::map<std::string, LatchBook>& latchBooks() { static std::map<std::string, LatchBook> m; return m; }
struct ChartState {
    std::string identity, root, market, asof, state;
    bdk::TableWatch keep; std::string keepNote;   // (2.21.2) the HOD/LOD table on this chart (keep-out) + its status line
    std::string book;                       // (2.5.7, audit F3) the latch book / record identity: the FULL contract (ESZ6), not the root
    Layout layout;
    bool wickOn = true; double wickX = 3.0;   // (2.20.0) the wick absorption A settings
    bool trapOn = true;                        // (2.21.0) the trapped traders read
    bool drawLate = false;                     // (2.23.0) "Draw late A (not tested)": also draw the old-path (late) A's
    int trapsDrawn = 0, trapsWithA = 0; std::string trapNote;   // (2.21.0) the last draw (status)
    struct LBox2 { int l, t, r, b; }; std::vector<LBox2> trapBoxes;   // (2.21.1) the drawn badge boxes (tests)
    dp::Snapshot snapshot;
    int barCount;
    RTDATE latestStamp, firstStamp;
    double tick;
    std::int64_t builtAt, statusAt, dealerAt, settingsAt, touchedAt;
    long long settingsStamp;
    ss::Store setStore; ss::Snap setSnap;   // (2.18.1) DeltaProfile.settings-<MKT>.txt (keyed) and the window as it opened
    int dealerReach, drawnRows, missingVap, vapMismatch;
    bool dirty, faulted, ready, shortHistory, coarseBoundary;
    int absorptionEvents, bubblesDrawn, bubblesOffscreen, bubblesSuppressed, activeAbsorptions, pairedAbsorptions;
    int grayDrawn = 0, grayOffscreen = 0;   // (2.19.0) the gray circles
    int liveDrawn = 0;                      // (2.27.0) live A circles (forming / waiting candle)
    int sabDrawn = 0, sabOffscreen = 0, sabColourVeto = 0, sabGrayFromNode = 0;   // (2.22.0) the shared A circles, node A's shown gray (attackers' close)
    int zoneFull = 0, zoneDim = 0;   // (2.9.0) A bubbles at a zone / mid-range (dimmed)
    int twoSidedDrawn = 0;           // (2.11.0) the two-sided rows marked
    int bubblesMerged = 0, aHiddenPre284 = 0, aUncheckedPre284 = 0;   // (2.8.5)
    std::map<long long, std::string> bubbleState; // event ID -> drawn/shared/offscreen/suppressed
    std::map<long long, std::string> profileState; // actual emitted profile disposition, not just calculation
    std::string exParams, exSource = "missing", exLevelNote; std::size_t exLevelCount = 0;   // (2.7.0) exhaustion gate / levels
    int exDrawn = 0, exSuppressed = 0, exOffscreen = 0; std::map<long long, std::string> exState;
    // (2.8.0) the right-side columns of this chart
    cb::Peers peers; cb::Layout lay; cb::UnitProbe probe; cb::MarginCtl mctl; std::string chartKey, colSym; long colSpb = 0; bool colPublished = false;
    long long firstDrawMs = -1, dealerSeenMs = -1; int needPx = 0, ppb = 0, marginRead = -1, marginAsked = -1, setOk = 0, setFail = 0, gapSeen = -1;
    bool marginOwner = false; int textsDrawn = 0, labelsShort = 0;
    int readDrawn = 0, readClash = 0; std::vector<sb::Fact> readFacts, nodeFacts, candleFacts;   // (2.28.0) the READ strip + the facts this chart posts (SignalBoard 1.4.0)
    long long factsPostedMs = -100000; long long combinerT = 0;   // (2.28.0)
    cmb::Params cmbParams = cmb::defaults(); long long cmbLoadMs = -100000; std::map<long long, cmb::Read> cmbReads; std::vector<std::string> cmbLines;
    int cmbShown = 0, cmbScored = 0, cmbNoLevels = 0, cmbDeferred = 0; std::string cmbPath; long long cmbFirstMs = -1; bool cmbRestored = false;   // (2.28.0) the combiner: reads decided once per closed candle, never re-scored
    // (2.17.0) the Market State "DP" entry of this chart: the closed bar posted, its facts (decided once), when last written
    long long mstBar = 0, mstDecT = -1, mstFailT = -1; std::int64_t mstAt = -1; std::map<std::string, std::string> mstKv; std::string mstNote = "not posted yet"; sa::Hook self; awh::Hook aw;   // (2.24.0) aw: the smart-read hook
    bool mstPosted = false;
    ChartState() : barCount(0), latestStamp(0), firstStamp(0), tick(0), builtAt(-1),
        statusAt(-1), dealerAt(-1), settingsAt(-1), touchedAt(-1), settingsStamp(-2),
        dealerReach(0), drawnRows(0), missingVap(0), vapMismatch(0), dirty(true), faulted(false), ready(false), shortHistory(false), coarseBoundary(false),
        absorptionEvents(0), bubblesDrawn(0), bubblesOffscreen(0), bubblesSuppressed(0), activeAbsorptions(0), pairedAbsorptions(0) {}
};
class DeltaProfile : public cppExtension {
public:
    SS_SAFE_DIALOG(DeltaProfile, "DeltaProfile")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    ~DeltaProfile() { delete fallback_; }
    int draw() override;
    int calc(int) override;
    int preCalc() override;
    int done() override;
    int destroy() override;
    int parmsLoadBody();
    int parmsApplyBody();
    int parmsUpdtBody(unsigned int);
    ss::Store& settingsStore(ChartState& s);
private:
    ChartState* fallback_ = NULL;
    ChartState& chartState();
    std::string storageDirectory();
    std::string rootSymbol();
    void syncLayout(ChartState& s, bool apply);
    int dealerWidth(ChartState& s);
    void applyMargin(ChartState& s);   // (2.8.0)
    void columns(ChartState& s, const RCT& pane, int axisR);   // (2.8.0) this draw's layout + the board
    bool buildNative(ChartState& s);
    bool render(ChartState& s);
    void status(ChartState& s);
    void exhaust(ChartState& s, LatchBook& bk, const std::vector<dp::Bar>& window, double tick);   // (2.7.0)
    void absorbExhaust(ChartState& s, LatchBook& bk, const std::vector<dp::Bar>& window, double tick);   // (2.7.0) AE
    void cmbFacts(ChartState& s, long long bar);   // (2.28.0) the combiner read in SRC DP
    void postMarketState(ChartState& s);   // (2.17.0) Market State SRC "DP", once per closed bar
    void selfAware(ChartState& s);   // (2.25.0) own values, own A signals + outcomes, SELFDP
    static int readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    static int readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    bool epoch(RTDATE value, std::int64_t& out);
    long long localSessionKey(RTDATE value, std::int64_t fallbackEpoch);
    long long localSec(RTDATE value);
    std::string recordDirectory();
    int yOf(int lastBar, double price);
    void combine(ChartState& s, const std::vector<const Latch*>& confirmed, const LatchBook& book, RTARRAYI& times, int candleLeft, int priceLimit, int top, int bottom);   // (2.28.0)
    int textWidth(const std::string& text, int font, bool bold);
    void textRight(int right, int y, const std::string& text, COLOR color, int font, bool bold);
    void textLeft(int left, int y, const std::string& text, COLOR color, int font, bool bold);
    void stroke(int x1, int y1, int x2, int y2, COLOR color, int width);
    void ringAt(int bar, double price, COLOR color, int left, int right, int top, int bottom);
    void box(int left, int top, int right, int bottom, COLOR color);
    // (2.27.0, Rassul 14:42 CT "delta profile should be pointing towards price") the LEFT column is drawn as before and then MIRRORED inside
    // [mirL, mirR]: every x inside the column maps to mirL + mirR - x, text anchors swap sides - the bars grow from the left edge toward price.
    // Nothing outside the column (circles / marks on the candles) moves. Active only inside render() (MirrorScope).
    int mirL = 0, mirR = -1;
    int mx(int x) const { return (mirR > mirL && x >= mirL && x <= mirR) ? mirL + mirR - x : x; }
    bool inMir(int x) const { return mirR > mirL && x >= mirL && x <= mirR; }
    void textRightRaw(int right, int y, const std::string& text, COLOR color, int font, bool bold);
    void textLeftRaw(int left, int y, const std::string& text, COLOR color, int font, bool bold);
};

int cppExtension::init() { return RTX_OK; }
int cppExtension::calc(int) { return RTX_OK; }
int cppExtension::done() { return RTX_OK; }
int cppExtension::destroy() { return RTX_OK; }
int cppExtension::setup() {
    // (2.18.1, settings audit 2026-10-18) the window from dpset::spec(): Width 110-250 (the column never drew under 110 - the old
    // 40-109 did nothing), Font; "Show delta text" left the window (the row values are not drawn since 2.14.0; the value is kept)
    ss::registerAll(this, dpset::spec(), dpIds());
    return RTX_OK;
}
std::string DeltaProfile::rootSymbol() {
    char buffer[64] = {0}; const char* p = getRootSymbol(buffer); return p ? safeField(p) : "";
}
ChartState& DeltaProfile::chartState() {
    // SDK persistent user-data belongs to the current host context, unlike this
    // DLL's shared cppExtension object. Destruction uses matching delete.
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (!p) {
        std::unique_ptr<ChartState> fresh(new ChartState());
        setUserData(fresh.get());
        if (getUserData() == fresh.get()) p = fresh.release();
        else {                                    // (2.4.2) host keeps no per-chart slot: one shared state, as before 2.4.1
            if (!fallback_) fallback_ = new ChartState();
            p = fallback_;
        }
    }
    const std::string root = rootSymbol();
    const char* symbol = getSymbol();
    const std::string symbolText = symbol ? symbol : "";
    const std::string periodText = std::to_string(getSecondsPerBar());   // (2.5.1) bar size, not the label (it can read empty)
    const std::string id = safeField(symbolText + "|" + periodText);
    if (p->identity != id || p->root != root) {
        *p = ChartState(); p->identity = id; p->root = root; p->market = marketName(root);
        p->book = symbolText.empty() ? root : safeField(symbolText);
    }
    return *p;
}
int DeltaProfile::calc(int) { LSPERF_SCOPE(LSPERF_PLUGIN, "calc");
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (p) { p->dirty = true; applyMargin(*p); } // historical corrections/backfills invalidate the snapshot; (2.8.0) the margin when it owns it
    return RTX_OK;
}
int DeltaProfile::preCalc() { LSPERF_SCOPE(LSPERF_PLUGIN, "preCalc");
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (p) applyMargin(*p);
    return RTX_OK;
}
// (2.8.0) the chart's right margin, ONLY while no Dealer Profile is on this chart (it owns the margin when it is): gap + this column,
// in IRT's unit; not before 5 s of drawing without a Dealer Profile beat (the two never fight at load), grows at once / shrinks after 60 s
void DeltaProfile::applyMargin(ChartState& s) {
    s.marginOwner = false;
    if (s.firstDrawMs < 0 || s.needPx <= 0 || s.peers.on[cb::DEALER]) return;
    const std::int64_t now = steadyMs();
    const std::int64_t since = s.dealerSeenMs >= 0 ? std::max(s.dealerSeenMs, s.firstDrawMs) : s.firstDrawMs;
    if (now - since < 5000) return;
    s.marginOwner = true;
    int ppb = 0;
    if (getChartOption(OPT_PIXELS_PER_BAR, &ppb) != RTX_OK || ppb <= 0) ppb = s.ppb;
    if (s.probe.units == cb::BARS && ppb <= 0) return;
    int cur = -1; if (getChartOption(OPT_RIGHT_MARGIN, &cur) != RTX_OK) cur = s.marginRead;
    const int v = s.mctl.want(s.needPx, ppb, s.probe.units, cur, (long long)now);
    if (v < 0) return;
    s.marginAsked = v;
    if (setChartOption(OPT_RIGHT_MARGIN, v) == RTX_OK) ++s.setOk; else ++s.setFail;
}
int DeltaProfile::done() {
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (p) { if (p->colPublished) cb::clear(cb::DELTA, p->colSym, p->colSpb, p->chartKey); if (p->mstPosted) { mst::clear("DP", p->colSym, p->colSpb, p->chartKey); p->self.clear(p->colSym, p->colSpb, p->chartKey); p->aw.clear(p->colSym, p->colSpb, p->chartKey); } *p = ChartState(); } // symbol/periodicity changes safely release fault state
    return RTX_OK;
}
int DeltaProfile::destroy() {
    ChartState* p = static_cast<ChartState*>(getUserData());
    if (p && p->colPublished) cb::clear(cb::DELTA, p->colSym, p->colSpb, p->chartKey);
    if (p && p->mstPosted) { mst::clear("DP", p->colSym, p->colSpb, p->chartKey); p->self.clear(p->colSym, p->colSpb, p->chartKey); p->aw.clear(p->colSym, p->colSpb, p->chartKey); }   // (2.17.0) (2.24.0 + AWDP) the Market State entry goes with the indicator
    if (p && p != fallback_) { delete p; setUserData(NULL); }
    return RTX_OK;
}
std::string DeltaProfile::storageDirectory() {
    // (2.4.2) the same folder every other LRA plugin reads (Dealer Profile / Dealer Read find this status there)
    const char* p = std::getenv("USERPROFILE");
    if (!p || !p[0]) return "";
    std::string base(p);
    if (base.back() != '\\' && base.back() != '/') base += DIR_SEPARATOR_CHR;
    base += "InvestorRT"; base += DIR_SEPARATOR_CHR; base += "rtx"; base += DIR_SEPARATOR_CHR; base += "lsFlexLevels";
#if defined(_WIN32)
    // Existing RTX folder normally exists. This is layout/settings storage only.
    CreateDirectoryA(base.c_str(), NULL);
#endif
    base += DIR_SEPARATOR_CHR; return base;
}
// (2.18.1) the settings come from DeltaProfile.settings-<MKT>.txt only (keyed, per MARKET - the old per-root file lost them at every roll);
// migrated once from DeltaProfile.settings.txt, which is left in place. The window is read / filled through SettingsHost.h.
ss::Store& DeltaProfile::settingsStore(ChartState& s) {
    const std::string directory = storageDirectory();
    const std::string want = directory.empty() || s.market.empty() ? std::string() : directory + "DeltaProfile.settings-" + s.market + ".txt";
    if (s.setStore.path != want) {
        s.setStore = ss::Store(); s.setStore.S = &dpset::spec(); s.setStore.path = want;
        if (!directory.empty()) s.setStore.legacyPaths.push_back(directory + "DeltaProfile.settings.txt");
        s.setStore.legacy = dpset::legacy; s.setStore.upgrade = dpset::upgrade; s.setStore.hint = s.root;   // the old file was keyed by the contract root: this chart's row first
    }
    return s.setStore;
}
void DeltaProfile::syncLayout(ChartState& s, bool apply) {
    ss::Store& st = settingsStore(s);
    const int mkt = ss::marketIndex(s.market);
    if (apply) {
        if (!ss::inDialog(this) || st.path.empty()) return;              // never from the chart copy
        const ss::Values F = st.load(mkt, steadyMs(), true);
        const ss::Values V = ss::readDialog(this, dpset::spec(), dpIds(), F, s.setSnap);
        if (!st.save(V)) { s.state = "settings write failed"; return; }
        s.setSnap.take(ss::dialogNow(this, dpset::spec(), dpIds(), V));
        s.settingsAt = -1;
    }
    const std::int64_t now = steadyMs();
    if (!apply && s.settingsAt >= 0 && now - s.settingsAt < 1000) return;
    s.settingsAt = now;
    const ss::Values V = st.load(mkt, now);
    Layout next; next.width = V.i(dpset::K_WIDTH); next.font = V.i(dpset::K_FONT); next.text = V.b(dpset::K_DELTA_TEXT);
    validateLayout(next); s.layout = next;
    s.wickOn = V.b(dpset::K_WICK_ON); s.wickX = V.f(dpset::K_WICK_X); if (!(s.wickX >= 1.0 && s.wickX <= 20.0)) s.wickX = 3.0;   // (2.20.0)
    s.trapOn = V.b(dpset::K_TRAP_ON);   // (2.21.0)
    s.drawLate = V.b(dpset::K_DRAW_LATE);   // (2.23.0)
}
int DeltaProfile::parmsLoadBody() {
    try {
        ChartState& s = chartState(); if (!ss::inDialog(this)) return RTX_OK;
        // Populate the dialog. Merely opening preferences NEVER saves a row.
        ss::fillDialog(this, dpset::spec(), dpIds(), settingsStore(s).load(ss::marketIndex(s.market), steadyMs(), true), s.setSnap);
        return RTX_OK;
    } catch (...) { return RTX_FAIL; }
}
int DeltaProfile::parmsApplyBody() {
    try { ChartState& s = chartState(); syncLayout(s, true); return RTX_OK; }
    catch (...) { return RTX_FAIL; }
}
int DeltaProfile::parmsUpdtBody(unsigned int i) { try { ss::onUpdate(this, dpset::spec(), dpIds(), (int)i, chartState().setSnap); } catch (...) {} return RTX_OK; } // groups only; saved on Apply/OK
int DeltaProfile::dealerWidth(ChartState& s) { return s.lay.w[cb::DEALER]; }   // (2.8.0) the Dealer column from the board (not its status file)
// (2.8.0) this draw's columns: the Dealer Profile's Levels + Dealer widths from the board (with its heartbeat), this column's own
// Width, the layout (ColumnBoard.h, the same the Dealer Profile computes), what the margin must be, the unit check
void DeltaProfile::columns(ChartState& s, const RCT& pane, int axisR) {
    const std::int64_t now = steadyMs();
    if (s.firstDrawMs < 0) s.firstDrawMs = now;
    const char* sy = getSymbol(); s.colSym = sy ? sy : ""; s.colSpb = getSecondsPerBar();
    const char* cl = getChartLabel(); s.chartKey = cl ? cl : "";
    s.peers.step(s.colSym, s.colSpb, s.chartKey, (long long)now, 1u << cb::DELTA);
    if (s.peers.on[cb::DEALER]) s.dealerSeenMs = now;
    cb::In in; in.paneL = pane.left; in.paneT = pane.top; in.paneB = pane.bottom; in.axisR = axisR;
    in.delta = true; in.deltaW = std::max(s.layout.width, 110);   // (2.11.2, Rassul 08:19) never narrower than 110 px, so the A label always has room
    in.dealer = s.peers.on[cb::DEALER]; in.dealerW = s.peers.w[cb::DEALER];
    in.levels = s.peers.on[cb::LEVELS]; in.levelsW = s.peers.w[cb::LEVELS];
    in.big = s.peers.on[cb::BIG]; in.bigW = s.peers.w[cb::BIG];   // (2.8.6) the BIG column
    s.lay = cb::layout(in);
    cb::publish(cb::DELTA, s.colSym, s.colSpb, s.chartKey, (long long)now, std::max(s.layout.width, 110)); s.colPublished = !s.colSym.empty();
    s.needPx = s.lay.ok ? s.lay.margin : 0;
    int ppb = 0, cur = -1;
    if (getChartOption(OPT_PIXELS_PER_BAR, &ppb) == RTX_OK && ppb > 0) s.ppb = ppb;
    if (getChartOption(OPT_RIGHT_MARGIN, &cur) == RTX_OK) s.marginRead = cur;
    int vs = 0, ve = -1;
    if (s.barCount > 1 && getVisibleBars(&vs, &ve) == RTX_OK && ve == s.barCount - 1) {
        PNT pe; pe.set(s.barCount - 1, 0.0f, kBarRight); s.gapSeen = axisR - pe.h;
        s.probe.observe(s.marginRead, s.ppb, s.gapSeen);
    }
}
int DeltaProfile::readStatistics(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
    // Only trivial values live in these MSVC SEH helpers (/EHsc compatible).
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
int DeltaProfile::readPrices(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) {
            std::memset(&rows[i], 0, sizeof(rows[i]));
            if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2;
        }
        return count; // ANY row failure rejects the whole snapshot, never a prefix
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
long long DeltaProfile::localSessionKey(RTDATE value, std::int64_t fallbackEpoch) {
    struct tm t = {};
    if (!getLocaltime(value, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31)
        return (fallbackEpoch - 17LL * 3600 + 86400LL * 4) / 86400LL;
    const long long days = civilDays(t.tm_year + 1900, static_cast<unsigned>(t.tm_mon + 1), static_cast<unsigned>(t.tm_mday));
    const int sec = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    return days + (sec >= 17 * 3600 ? 1 : 0);
}
// (2.5.4) a bar stamp as the chart's Central wall clock encoded as epoch seconds (TapeFlow's convention); 0 = unreadable
long long DeltaProfile::localSec(RTDATE value) {
    struct tm t = {};
    if (!getLocaltime(value, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, static_cast<unsigned>(t.tm_mon + 1), static_cast<unsigned>(t.tm_mday)) * 86400LL +
        t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
std::string DeltaProfile::recordDirectory() {
    const std::string base = storageDirectory();
    return base.empty() ? base : base + "DeltaProfile" + DIR_SEPARATOR_CHR;
}
bool DeltaProfile::epoch(RTDATE value, std::int64_t& out) {
    struct tm t = {};
    if (!getLocaltime(value, &t)) return false;
    t.tm_isdst = -1;
    const time_t result = std::mktime(&t);
    if (result == static_cast<time_t>(-1)) return false;
    out = static_cast<std::int64_t>(result); return true;
}
bool DeltaProfile::buildNative(ChartState& s) {
    if (s.faulted) { s.ready = false; s.state = "VAP fault: reload indicator to retry"; return false; }
    const long count = getBarCount();
    if (count < 2 || count > INT_MAX) { s.ready = false; s.state = "insufficient chart bars"; return false; }
    RTARRAYI times(barDateTime);
    if (times.count < count) { s.ready = false; s.state = "timestamp array unavailable"; return false; }
    const RTDATE latest = static_cast<RTDATE>(times[static_cast<int>(count) - 1]);
    const RTDATE first = static_cast<RTDATE>(times[0]);
    double tick = static_cast<double>(getProperty(SYM_TICKINCR));
    if (!dp::finite(tick) || tick <= 0 || tick >= 1000) tick = marketTick(s.market);   // (2.4.2) IRT's own first, the exchange's as fallback
    if (!dp::finite(tick) || tick <= 0) {
        s.ready = false; s.state = "native tick size unavailable"; return false;
    }
    const std::int64_t now = steadyMs();
    const bool changed = count != s.barCount || latest != s.latestStamp || first != s.firstStamp || tick != s.tick;
    // At most once/second per host context; new bars and tick changes rebuild now.
    // Even unchanged total volume is re-read periodically: bid/ask corrections
    // and OHLC changes do not necessarily change barVolume.
    if (!changed && s.builtAt >= 0 && now - s.builtAt < 1000) return s.ready;
    s.builtAt = now; s.barCount = static_cast<int>(count);
    s.latestStamp = latest; s.firstStamp = first; s.tick = tick; s.dirty = false;
    std::int64_t end;
    if (!epoch(latest, end)) { s.ready = false; s.state = "invalid chart timestamp"; return false; }
    const std::int64_t cutoff = end - 90 * 60;
    RTARRAYP vap(barVolumeProfile);
    RTARRAY open(barOpen), high(barHigh), low(barLow), close(barClose);
    RTARRAYI volume(barVolume);
    if (vap.count < count || open.count < count || high.count < count || low.count < count ||
        close.count < count || volume.count < count) {
        s.ready = false; s.state = "chart data arrays unavailable"; return false;
    }
    std::vector<dp::Bar> bars;
    std::vector<VOLPROFILE> rows;
    std::size_t examinedPrices = 0;
    const std::size_t MAX_WINDOW_PRICES = 2000000; // fail explicitly, not silently truncate
    bool reachedBoundary = false, coarseBar = false;
    int missing = 0, mismatched = 0;
    std::int64_t nextTime = end;
    for (int i = static_cast<int>(count) - 1; i >= 0; --i) {
        std::int64_t stamp;
        if (!epoch(static_cast<RTDATE>(times[i]), stamp)) {
            s.ready = false; s.state = "invalid chart timestamp"; return false;
        }
        if (stamp > nextTime) stamp = nextTime;   // (2.4.2) DST fall-back repeats an hour of local stamps: keep order, never fail
        if (stamp <= cutoff) { reachedBoundary = true; break; }
        nextTime = stamp;
        BARSTATISTICS stats;
        const int prices = readStatistics(&vap, i, &stats);
        if (prices == -1) { s.faulted = true; s.ready = false; s.state = "VAP fault: reload indicator to retry"; return false; }
        if (prices < 0) { if (i != static_cast<int>(count) - 1) ++missing; continue; }   // (2.4.2) a bar IRT has no VAP for yet (forming / back-filling): skip it, flag PARTIAL
        examinedPrices += static_cast<std::size_t>(prices);
        if (examinedPrices > MAX_WINDOW_PRICES) { s.ready = false; s.state = "VAP window exceeds safety budget"; return false; }
        rows.resize(static_cast<std::size_t>(prices));
        const int read = readPrices(&vap, i, rows.empty() ? NULL : &rows[0], prices);
        if (read == -1) { s.faulted = true; s.ready = false; s.state = "VAP fault: reload indicator to retry"; return false; }
        if (read < 0 || (prices == 0 && volume[i] > 0)) { if (i != static_cast<int>(count) - 1) ++missing; continue; }   // (2.4.2) skip + flag, never blank the profile
        dp::Bar bar;
        bar.index = i; bar.time = stamp; bar.closed = i < static_cast<int>(count) - 1;
        bar.open = open[i]; bar.high = high[i]; bar.low = low[i]; bar.close = close[i];
        bar.rows.reserve(rows.size());
        double total = 0, buys = 0, sells = 0;
        for (std::size_t k = 0; k < rows.size(); ++k) {
            const VOLPROFILE& v = rows[k];
            if (!dp::finite(v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.totalVolume < 0) { ++mismatched; continue; }
            const double buy = static_cast<double>(v.buyVolume), sell = static_cast<double>(v.sellVolume);
            double vol = static_cast<double>(v.totalVolume);
            if (buy + sell > vol + 0.5) { ++mismatched; vol = buy + sell; }   // (2.4.2) count it, keep the row
            total += vol; buys += buy; sells += sell;
            if (vol == 0 && buy == 0 && sell == 0) continue;
            bar.rows.push_back(dp::Row(dp::priceKey(v.price, tick), buy - sell, vol));
        }
        // Reject inconsistent statistics or snapshots read while VAP is mutating.
        // A subsequent periodic draw retries; no incomplete signal is published.
        // (2.4.2) the bar's own totals vs its rows: a mismatch is COUNTED (status VAP_MISMATCH) - IRT's statistics may
        // aggregate differently, and a forming bar moves between the two reads; neither may blank the whole profile.
        if (std::fabs(total - static_cast<double>(stats.totalVolume)) > 0.5 ||
            std::fabs(buys - static_cast<double>(stats.buyVolume)) > 0.5 ||
            std::fabs(sells - static_cast<double>(stats.sellVolume)) > 0.5) ++mismatched;
        dp::normalize(bar); bars.push_back(std::move(bar));
    }
    std::reverse(bars.begin(), bars.end());
    // (2.28.0, 2026-10-28) CANDLE TRAP LOTS for lsTrappedTraders (it gets no volume at price in its own pane): per CLOSED candle of the window,
    // the sellers' lots traded BELOW its close and the buyers' lots traded ABOVE it - SignalBoard 1.4.0 kind 'C', decided (state 1)
    s.candleFacts.clear();
    for (const dp::Bar& bb2 : bars) {
        if (!bb2.closed) continue;
        dp::Tick ck; try { ck = dp::priceKey(bb2.close, tick); } catch (...) { continue; }
        double below = 0, above = 0;
        for (const dp::Row& r : bb2.rows) { const double buy = (r.volume + r.delta) / 2, sell = (r.volume - r.delta) / 2;
            if (r.key < ck) below += sell; else if (r.key > ck) above += buy; }
        sb::Fact f; f.bar = localSec(static_cast<RTDATE>(times[bb2.index])); f.lo = below; f.hi = above; f.side = below > above ? 1 : below < above ? -1 : 0;
        f.val = below + above; f.code = 0; f.state = 1; f.proven = 0; if (f.bar > 0) s.candleFacts.push_back(f);
    }
    // Chart-bar VAP has no within-bar timestamps. A long bar crossing the window
    // edge is included whole (close-time policy), not falsely prorated to 90 min.
    const int seconds = getSecondsPerBar();
    if (seconds > 0 && !bars.empty()) coarseBar = bars.front().time - seconds < cutoff;
    s.shortHistory = !reachedBoundary || missing > 0;
    s.missingVap = missing; s.vapMismatch = mismatched;
    s.coarseBoundary = coarseBar;
    dp::Snapshot candidate = dp::build(bars, tick, !s.shortHistory && !coarseBar);
    if (candidate.buckets.empty()) { s.ready = false; s.state = "no native volume at price"; return false; }
    {   // (2.5.2, Rassul 14:37 "if the top 3 change, the signal changes") every node is TRACKED from the moment it first
        // shows as an absorption candidate: its price zone and bar are frozen, and it is decided by the chart's own bars
        // (a RED close below a buying node / a GREEN close above a selling node = A; the opposite = I), whether or not it
        // is still one of the profile's top 3. A decided result never changes. Cleared at the 17:00 session change.
        // (2.5.3, RA1009 audit) the session key from the chart's LOCAL (Central) clock. bars[].time is a true UTC epoch (mktime),
        // so "epoch - 17 h" changed day at 17:00 UTC = 12:00 CDT / 11:00 CST and wiped the record (A's and their circles) at noon.
        const long long sess = localSessionKey(static_cast<RTDATE>(times[bars.back().index]), bars.back().time);
        const int spb = getSecondsPerBar();
        LatchBook& bk = latchBooks()[s.book + "|" + std::to_string(spb)];
        if (sess != bk.session) {
            if (!bk.journal.empty() && !bk.path.empty()) {                                          // (2.5.4) last try for the old session's lines
                makeDirectory(recordDirectory());
                if (!appendRecord(bk.path, bk.journal)) {                                           // (2.5.7, F6) kept, retried below
                    // (2.6.1, audit F6) never dropped: lines owed to the same file are merged into one entry (no count cap)
                    std::size_t c = 0;
                    while (c < bk.carry.size() && bk.carry[c].first != bk.path) ++c;
                    if (c < bk.carry.size()) bk.carry[c].second.insert(bk.carry[c].second.end(), bk.journal.begin(), bk.journal.end());
                    else bk.carry.push_back(std::make_pair(bk.path, bk.journal));
                }
            }
            bk.v.clear(); bk.session = sess;
            bk.journal.clear(); bk.path.clear(); bk.note.clear(); bk.nextId = 0; bk.written = bk.loaded = bk.rejected = 0;
            bk.raThrough = bk.raJudged = bk.raFound = bk.raLate = bk.raOld = 0; bk.raCache.clear(); bk.raNote.clear();   // (2.23.0)
            bk.recheck.clear(); bk.show.clear(); bk.ae.clear(); bk.aeThrough.clear(); bk.ex.clear(); bk.exNext = bk.exLoaded = bk.exRejected = 0; bk.exThrough = 0; bk.exNote.clear();   // (2.7.0)
            // (2.5.4) after an IRT restart (or the first chart of the day) the session's record seeds the book: what was shown stays
            const std::string folder = recordDirectory();
            if (!folder.empty() && !s.root.empty()) {
                bk.path = folder + contractRecordName(s.market, s.book, spb, sess);                  // (2.6.3) this contract's own file
                std::string text; bool tooLarge = false;
                if (readWholeFile(bk.path, text, tooLarge)) seedFromText(text, s.book, "", tick, bk);
                else if (tooLarge) { bk.note = "record too large - not read, not appended"; bk.path.clear(); }
                // (2.6.3) no import from root-only files: they cannot tell one expiry from the next, so a new contract-qualified
                // file always starts empty (the audit's compatibility boundary)
            }
        }
        if (!bk.carry.empty()) makeDirectory(recordDirectory());                                // (2.6.1) the folder may have been removed
        for (std::size_t c = 0; c < bk.carry.size();) {                                       // (2.5.7, F6) old-session lines still owed
            if (bk.carry[c].first.empty() || bk.carry[c].second.empty()) { bk.carry.erase(bk.carry.begin() + static_cast<long>(c)); continue; }
            if (appendRecord(bk.carry[c].first, bk.carry[c].second)) bk.carry.erase(bk.carry.begin() + static_cast<long>(c)); else ++c;
        }
        if (!bk.carry.empty()) {                                                                // (2.6.1) visible in the status note only
            std::size_t owed = 0; for (std::size_t c = 0; c < bk.carry.size(); ++c) owed += bk.carry[c].second.size();
            bk.note = "record write failed - " + std::to_string(owed) + " old-session lines retained for retry";
        }
        if (bk.tick > 0 && std::fabs(bk.tick - tick) > 1e-9 * std::max(bk.tick, tick)) {           // (2.5.7, F5) the tick changed: re-key by price
            std::size_t dropped = 0;
            for (std::size_t k = 0; k < bk.v.size();) {
                bool ok = true;
                try { const dp::Tick lo = dp::priceKey(static_cast<double>(bk.v[k].low) * bk.tick, tick), hi = dp::priceKey(static_cast<double>(bk.v[k].high) * bk.tick, tick);
                      bk.v[k].low = std::min(lo, hi); bk.v[k].high = std::max(lo, hi); } catch (...) { ok = false; }
                // (2.6.1, audit F5) a latch that cannot be re-keyed keeps old-grid keys - never compare it on the new grid
                if (ok) ++k; else { bk.v.erase(bk.v.begin() + static_cast<long>(k)); ++dropped; }
            }
            if (dropped) bk.note = "tick changed: " + std::to_string(dropped) + " latches could not be re-keyed and were dropped";
        }
        if (bk.tick > 0 && std::fabs(bk.tick - tick) > 1e-9 * std::max(bk.tick, tick)) bk.recheck.clear();   // (2.8.5) keys re-made
        bk.tick = tick;
        std::vector<Latch>& latches = bk.v;
        const long long nowT = localSec(static_cast<RTDATE>(times[static_cast<int>(count) - 1]));   // the chart's last bar end
        auto barEnd = [&](int index) -> long long { return index >= 0 && index < static_cast<int>(count) ? localSec(static_cast<RTDATE>(times[index])) : 0; };
        auto decided = [](const std::string& c) { return !c.empty() && c.back() != '?'; };
        auto sellNode = [](const std::string& c, bool sup) { return c[0] == 'A' && c.compare(0, 3, "Acc") != 0 ? sup : c[0] == 'I' ? !sup : c.compare(0, 3, "Dst") == 0; };
        const bool allowedNow = !s.shortHistory && !coarseBar;
        auto track = [&](dp::Tick lo, dp::Tick hi, std::string& code, bool& support, int& peakBar, double& price,
                         int& decidedBar, int formedBar, double net, double mult, bool zone, long long& eventId) {
            if (!allowedNow || code.empty() || peakBar < 0 || peakBar >= static_cast<int>(count)) return;
            const bool mySell = sellNode(code, support);
            const RTDATE peak = static_cast<RTDATE>(times[peakBar]);
            for (const Latch& L : latches) {
                if (!sameEvent(L, lo, hi, mySell, zone, peak)) continue;
                code = L.code; support = L.support; price = L.price; eventId = L.id;
                return;
            }
            if (latches.size() >= REC_MAX_LATCHES || formedBar < 0 || formedBar >= static_cast<int>(count)) {
                if (code == "A") code = "A?"; // never publish unrecorded confirmed A
                return;
            }
            Latch L; L.low = lo; L.high = hi; L.code = allowedNow ? code : (decided(code) ? code + "?" : code);
            // (2.8.4) classify() sees only the 90-min window and cannot apply the 2.8.3 pivot rule (chart bars 0 .. peak) - a node
            // first seen AFTER its peak bar closed (chart opened, IRT restarted, the node reached 2.0x / the top 3 later) took its A
            // without that rule. A new event now starts undecided; the decision loop below applies EVERY A rule from the peak bar's
            // own closed data (bars 0 .. peak only), so the late path and the live path give the same answer.
            if (L.code == "A") L.code = "A?";
            L.support = support; L.candSupport = support; L.peakTime = peak; L.price = price; L.sell = mySell;
            L.formedTime = static_cast<RTDATE>(times[formedBar]); L.formedT = barEnd(formedBar);
            L.id = bk.nextId++; L.tFirst = nowT; L.peakT = barEnd(peakBar); L.net = net; L.mult = mult; L.zone = zone;
            if (decided(L.code)) L.decidedBy = barEnd(decidedBar);
            L.version = DLT_VERSION;
            eventId = L.id; latches.push_back(L); bk.journal.push_back(recordLine(L, 'F', tick, s.book));
        };
        candidate.zones.clear();   // (2.13.0) absorption is the row rule only: no zone-merged A
        const dp::Tick tpr = candidate.ticksPerRow;
        for (dp::Node& n : candidate.cands) {   // (2.15.0) the 3 biggest rows at 2.0x or more (DeltaProfileCore build: back to the 2.6.0 top-3 rule)
            double net = 0;
            for (const dp::Bucket& b : candidate.buckets) if (b.key == n.bucket) { net = b.delta; break; }
            track(n.bucket*tpr, n.bucket*tpr+tpr-1, n.state.code, n.state.support, n.state.peakBar, n.price,
                  n.state.decidedBar, n.state.formedBar, net, n.ratio, false, n.state.eventId);
        }
        for (dp::Node& n : candidate.nodes) {
            double net = 0;
            for (const dp::Bucket& b : candidate.buckets) if (b.key == n.bucket) { net = b.delta; break; }
            track(n.bucket*tpr, n.bucket*tpr+tpr-1, n.state.code, n.state.support, n.state.peakBar, n.price,
                  n.state.decidedBar, n.state.formedBar, net, n.ratio, false, n.state.eventId);
        }
        for (dp::Zone& z : candidate.zones) {
            double price = (static_cast<double>(z.low)+static_cast<double>(z.high))*.5*tick;
            const double mult = candidate.meanAbsDelta > 0 ? std::fabs(z.net)/candidate.meanAbsDelta : 0;
            track(z.low,z.high,z.code,z.support,z.peakBar,price,z.decidedBar,z.formedBar,z.net,mult,true,z.eventId);
        }

        // (2.23.0) THE RESEARCHED A (dp::researchA): every CLOSED candle of this session is judged ONCE, in order, at its close: the
        // window = the W candles ending with it (W = 90 min / bar size, 30 on a 3-min chart; all in this session - no A in the session's
        // first 90 min, as in the research), an A only when the row's peak candle is this candle (same) or the one before (next). The
        // decision is final at once (no pivot wait, no later node): a candle judged before a restart is never judged again (RW lines).
        bool raCaught = true;
        {
            const int raW = spb > 0 ? std::max(7, static_cast<int>((5400 + spb - 1) / spb)) : 0;
            const int lastClosed = static_cast<int>(count) - 2;
            auto readBar = [&](int i, dp::Bar& bar) -> int {   // 1 = read, 0 = not available yet, -1 = VAP fault
                BARSTATISTICS st; std::memset(&st, 0, sizeof(st));
                const int prices = readStatistics(&vap, i, &st);
                if (prices == -1) return -1;
                if (prices < 0) return 0;
                std::vector<VOLPROFILE> pr(static_cast<std::size_t>(prices));
                const int got = readPrices(&vap, i, pr.empty() ? NULL : &pr[0], prices);
                if (got == -1) return -1;
                if (got < 0 || (prices == 0 && volume[i] > 0)) return 0;
                bar.index = i; bar.closed = true; bar.time = localSec(static_cast<RTDATE>(times[i]));
                bar.open = open[i]; bar.high = high[i]; bar.low = low[i]; bar.close = close[i];
                if (!dp::finite(bar.open) || !dp::finite(bar.high) || !dp::finite(bar.low) || !dp::finite(bar.close)) return 0;
                for (const VOLPROFILE& v : pr) {
                    if (!dp::finite(v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.totalVolume < 0) continue;
                    const double b = static_cast<double>(v.buyVolume), sl = static_cast<double>(v.sellVolume);
                    const double vol = std::max(static_cast<double>(v.totalVolume), b + sl);
                    if (vol == 0) continue;
                    bar.rows.push_back(dp::Row(dp::priceKey(v.price, tick), b - sl, vol));
                }
                dp::normalize(bar); return 1;
            };
            if (allowedNow && raW > 0 && lastClosed >= raW - 1) {
                int s0 = lastClosed;                                              // the session's first candle on the chart
                while (s0 > 0 && localSessionKey(static_cast<RTDATE>(times[s0 - 1]), 0) == sess) --s0;
                int k = std::max(s0 + raW - 1, 0);
                {   int lo2 = k, hi2 = lastClosed, first = lastClosed + 1;            // the first candle after the watermark
                    while (lo2 <= hi2) { const int mid = (lo2 + hi2) / 2; if (barEnd(mid) > bk.raThrough) { first = mid; hi2 = mid - 1; } else lo2 = mid + 1; }
                    k = first; }
                int budget = 60; bool stop = false;
                for (; k <= lastClosed && budget > 0 && !stop; ++k, --budget) {
                    std::vector<dp::Bar> w; w.reserve(static_cast<std::size_t>(raW));
                    for (int j = k - raW + 1; j <= k; ++j) {
                        const long long key = static_cast<long long>(static_cast<RTDATE>(times[j]));
                        std::map<long long, dp::Bar>::iterator c = bk.raCache.find(key);
                        if (c == bk.raCache.end()) {
                            dp::Bar b; const int r = readBar(j, b);
                            if (r < 0) { s.faulted = true; bk.raNote = "VAP fault"; stop = true; break; }
                            if (r == 0) { bk.raNote = "waiting for VAP of candle " + std::to_string(barEnd(j)); stop = true; break; }
                            c = bk.raCache.insert(std::make_pair(key, b)).first;
                        }
                        w.push_back(c->second); w.back().index = j;
                    }
                    if (stop) break;
                    std::vector<dp::RaHit> hits;
                    try { hits = dp::researchA(w, tick); } catch (...) { hits.clear(); bk.raNote = "researchA error"; }
                    for (const dp::RaHit& h : hits) {
                        const int pk = k - raW + 1 + h.pk, fm = k - raW + 1 + std::max(h.formed, h.pk);
                        const RTDATE peak = static_cast<RTDATE>(times[pk]);
                        Latch* hit = NULL;
                        for (Latch& L : latches) if (sameEvent(L, h.lo, h.hi, h.sell, false, peak)) { hit = &L; break; }
                        if (hit && decided(hit->code)) continue;                    // never re-decided
                        if (!hit) {
                            if (latches.size() >= REC_MAX_LATCHES) { bk.raNote = "latch cap"; continue; }
                            Latch L; L.low = h.lo; L.high = h.hi; L.code = "A?"; L.support = h.sell; L.candSupport = h.sell; L.peakTime = peak;
                            L.price = (static_cast<double>(h.lo) + static_cast<double>(h.hi)) * .5 * tick; L.sell = h.sell;
                            L.formedTime = static_cast<RTDATE>(times[fm]); L.formedT = barEnd(fm);
                            L.id = bk.nextId++; L.tFirst = nowT; L.peakT = barEnd(pk); L.net = h.net; L.mult = h.ratio; L.zone = false; L.version = DLT_VERSION;
                            latches.push_back(L); bk.journal.push_back(recordLine(latches.back(), 'F', tick, s.book));
                            hit = &latches.back();
                        }
                        hit->code = "A"; hit->support = h.sell; hit->rule = 'R'; hit->conf = h.conf == dp::CONF_NEXT ? 'N' : 'S';
                        hit->decidedBy = barEnd(k); hit->delayBars = k - pk; hit->delayMin = (barEnd(k) - barEnd(pk)) / 60; hit->tent = false;
                        ++bk.raFound;
                    }
                    bk.raThrough = std::max(bk.raThrough, barEnd(k)); ++bk.raJudged;
                    bk.journal.push_back(std::string("RW|") + DLT_VERSION + "|" + s.book + "|" + std::to_string(bk.raThrough));
                    for (std::map<long long, dp::Bar>::iterator c = bk.raCache.begin(); c != bk.raCache.end();) {   // keep only what the next windows need
                        if (c->first < static_cast<long long>(static_cast<RTDATE>(times[std::max(0, k - raW + 1)]))) c = bk.raCache.erase(c); else ++c;
                    }
                    if (!bk.raNote.empty() && bk.raNote.compare(0, 7, "waiting") == 0) bk.raNote.clear();
                }
                raCaught = k > lastClosed;
            }
            // (2.27.0) the LIVE A on the forming candle (count - 1): the same rules, the forming candle as "k"
            bk.live.clear();
            if (allowedNow && raW > 0 && static_cast<int>(count) >= raW) {
                const int kf = static_cast<int>(count) - 1;
                std::vector<dp::Bar> w; w.reserve(static_cast<std::size_t>(raW)); bool okW = true;
                for (int j = kf - raW + 1; j <= kf && okW; ++j) {
                    const long long key = static_cast<long long>(static_cast<RTDATE>(times[j]));
                    std::map<long long, dp::Bar>::iterator c = bk.raCache.find(key);
                    dp::Bar b;
                    if (j < kf && c != bk.raCache.end()) b = c->second;
                    else { const int r = readBar(j, b); if (r <= 0) { okW = false; break; } }
                    b.index = j; w.push_back(b);
                }
                if (okW) {
                    std::vector<dp::RaHit> hits;
                    try { hits = dp::researchA(w, tick); } catch (...) { hits.clear(); }
                    for (const dp::RaHit& h : hits) {
                        const int pk = kf - raW + 1 + h.pk; const RTDATE peak = static_cast<RTDATE>(times[pk]);
                        bool done = false; for (const Latch& L : latches) if (sameEvent(L, h.lo, h.hi, h.sell, false, peak) && decided(L.code)) done = true;
                        if (done) continue;
                        LatchBook::LiveA a; a.lo = h.lo; a.hi = h.hi; a.sell = h.sell; a.peak = peak; a.price = (static_cast<double>(h.lo) + static_cast<double>(h.hi)) * .5 * tick;
                        bk.live.push_back(a);
                    }
                    ++bk.liveRuns;
                }
            }
        }
        // (2.6.2, Rassul 2026-10-13 05:16 "the same bar has to have a green close above the absorption") a pending event is decided
        // by its OWN PEAK BAR (frozen peakTime, the bar its circle sits on) once that bar has CLOSED - never by the forming last bar:
        //   selling node: peak bar GREEN (close > open) closing ABOVE the node = A (sellers absorbed, support);
        //   buying node:  peak bar RED   (close < open) closing BELOW the node = A (buyers absorbed, resistance);
        //   anything else (inside, wrong side, wrong colour, doji) = never A. Such a node can still become I on the FIRST closed bar
        //   after its frozen formation that closes through it the other way in its own colour (selling: red below; buying: green
        //   above); until then it shows I?. A later bar can never make a node A. A decision found when the event first appears (its
        //   peak bar closed before the node formed) is stamped t_decided = that build's time - never back-dated.
        // (2.5.7, audit F2) only a complete build (no missing VAP / short history) decides - the same gate as a fresh node
        auto findBar = [&](RTDATE when) -> int {
            int lo = 0, hi = static_cast<int>(count) - 1;
            while (lo <= hi) {
                const int mid = (lo + hi) / 2; const RTDATE t = static_cast<RTDATE>(times[mid]);
                if (t == when) return mid;
                if (t < when) lo = mid + 1; else hi = mid - 1;
            }
            return -1;
        };
        // (2.8.3, Rassul 19:47) the pivot rule, from bars 0 .. pk only (decided once at the peak bar's close, never repainted): his
        // 1-bar ZigZag (CobImpulseLogic legPivot / trade_def zigzag). Bullish A: the peak bar CREATES the down pivot (it is the current
        // down leg's low) or its LOW is below the HIGH of the latest down-pivot bar. Bearish A: mirror (creates the up pivot, or its
        // HIGH is above the LOW of the latest up-pivot bar). A later lower low / higher high changes nothing.
        // (2.8.4) the pivot rule is dp::pivotOk (DeltaProfileCore.h), unit-tested; the halfway rule dp::halfwayOk
        // (2.16.0) the pivot of each bar's own swing over ALL the bars (may change until conf = 1)
        std::vector<char> swBull, swBear, swConf; std::vector<int> swD, swU;
        dp::swingPivot(high, low, static_cast<int>(count), swBull, swBear, swConf, &swD, &swU);   // (2.18.0) + each bar's pivot bar
        for (std::size_t k = 0; allowedNow && raCaught && k < latches.size(); ++k) {   // (2.23.0) after the researched A has judged every closed candle
            Latch& L = latches[k];
            L.tent = false;
            if (decided(L.code)) continue;
            const int pk = findBar(L.peakTime);
            if (pk < 0 || pk >= static_cast<int>(count) - 1) continue;                            // peak bar still forming / not on chart
            const double po = open[pk], pc = close[pk];
            if (!dp::finite(po) || !dp::finite(pc)) continue;
            // (2.5.7, audit F1) beyond the node is decided in WHOLE TICKS: the close's own tick key against the node's keys
            dp::Tick pck; try { pck = dp::priceKey(pc, tick); } catch (...) { continue; }
            // (2.8.2, Rassul 19:44 "for bullish circle the absorption needs to be at or below the half way mark of the candle and vice
            // versa") the node's middle must sit at / below the peak bar's midpoint for a bullish A, at / above it for a bearish A
            const double ph = high[pk], pl = low[pk];
            if (!dp::finite(ph) || !dp::finite(pl)) continue;
            // (2.15.0, Rassul 2026-10-17 13:25 / 15:35 option 2 (b)) the 2.6.0 rules plus the pivot rule and the same-candle close: NO halfway rule
            // (2.16.0) the pivot against the peak bar's own swing: A is recorded once that pivot is confirmed; before that the A shows
            // (tentative, code A?) while the close rule and the pivot on the bars so far hold - it may go and come back
            const bool closeOk = L.sell ? (pc > po && pck > L.high) : (pc < po && pck < L.low);
            const bool pivNow = pk < static_cast<int>(swBull.size()) && (L.sell ? swBull[pk] : swBear[pk]) != 0;
            const bool pivFinal = pk < static_cast<int>(swConf.size()) && swConf[pk] != 0;
            // (2.18.0, Rassul 2026-10-18) if the node's own candle did not close beyond it, the NEXT candle may (one candle only, decided
            // at its close): bullish green close above the node, bearish red close below, and BOTH bars' lows (bullish) / highs (bearish)
            // beyond the node candle's pivot bar's high / low. The bubble stays on the node's candle (peakTime).
            const int nb = pk + 1;
            const bool nextClosed = nb < static_cast<int>(count) - 1;
            bool closeNext = false, pivBoth = false;
            if (!closeOk && nextClosed) {
                const double no = open[nb], nc = close[nb];
                dp::Tick nck = 0; bool keyOk = dp::finite(no) && dp::finite(nc);
                if (keyOk) { try { nck = dp::priceKey(nc, tick); } catch (...) { keyOk = false; } }
                closeNext = keyOk && (L.sell ? (nc > no && nck > L.high) : (nc < no && nck < L.low));
                const int piv = pk < static_cast<int>(swD.size()) ? (L.sell ? swD[pk] : swU[pk]) : -1;
                pivBoth = pivNow && dp::nextPivotOk(high, low, static_cast<int>(count), nb, piv, L.sell);
            }
            const dp::AStep as = dp::aStep(closeOk, pivNow, nextClosed, closeNext, pivBoth, pivFinal);   // (2.18.0) one tested function
            const dp::TentStep step = as.step;
            if (as.by != dp::CONF_NONE) L.conf = as.by == dp::CONF_NEXT ? 'N' : 'S';
            if (step == dp::TS_FINAL) {   // (2.23.0) the OLD path: an A the researched rule did not give at its candle's close - logged (code AL), never drawn by default
                const int dec = static_cast<int>(count) - 2;   // decided now: at the last closed candle's close
                L.code = "AL"; L.support = L.sell; L.rule = 'L'; L.decidedBy = barEnd(as.by == dp::CONF_NEXT ? nb : pk);
                L.delayBars = std::max(0, dec - pk); L.delayMin = std::max(0LL, (barEnd(dec) - barEnd(pk)) / 60);
                ++bk.raOld; if (L.delayBars > 1) ++bk.raLate;
                continue;
            }
            if (step == dp::TS_PENDING) { L.code = "A?"; L.support = L.sell; continue; }            // (2.18.0) waits for the next candle's close (not shown)
            if (step == dp::TS_SHOWN || step == dp::TS_HIDDEN) {                                   // undecided until confirmed
                L.code = "A?"; L.support = L.sell; L.tent = false;   // (2.23.0) a tentative A is never drawn (no TA lines)
                continue;
            }
            // (2.17.0) a tentative A that was shown and whose confirmed pivot went against it: one TA|X line (the nightly measures it)
            // (2.23.0) no TA|X line: nothing tentative is shown any more
            L.code = "I?"; L.support = !L.sell;                                                   // absorption ruled out
            // (2.13.0) no initiative: a node that is not absorption stays "I?" (undecided, never drawn, never labelled)
        }
        // (2.22.0, 2026-10-22 Rassul 06:43 "then do it" / 06:49 "then require it") THE SHARED ABSORPTION A (SharedAbsorb.h) replaces the
        // 2.20.0 wick A (no longer decided; old conf=wick records are kept but not drawn). Every A drawn here comes from the ONE shared
        // record lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv. lsBidAskStats (the attacker pane) decides it; only when no attacker pane
        // posts on this chart (no fresh SignalBoard 'O') does this plugin decide a closed candle itself, from the chart's VAP statistics
        // (the same buy / sell lots), and append it. Decided once at the close, never repainted.
        {
            const long long nowMs = static_cast<long long>(steadyMs());
            const std::string fdir = storageDirectory();
            const int spbNow = getSecondsPerBar();
            if (!fdir.empty() && !s.market.empty() && spbNow > 0) {
                const std::string sdir = fdir + "SharedAbsorb";
                bk.sab.bind(s.market, spbNow, sdir + DIR_SEPARATOR_CHR + sab::fileName(s.market, spbNow), fdir + "LRA-Badges-" + sab::canon(s.market) + ".csv");
                if (!bk.sabDir) { makeDirectory(sdir); bk.sabDir = true; }
                bk.sab.poll(nowMs);
                std::vector<sb::Ev> obAll; const bool basHere = sb::readAll('O', s.colSym, s.colSpb, s.chartKey, nowMs, obAll);
                bk.sabBasHere = basHere;
                const int nBars = static_cast<int>(count);
                if (bk.sabStartMs < 0 || basHere) bk.sabStartMs = nowMs;           // an attacker pane seen: the grace starts over
                // decide only with the chart identity known and after 90 s without any attacker-pane post (it posts at least every 15 s),
                // so a chart that has lsBidAskStats never gets a line from here while that pane is still loading
                if (allowedNow && !basHere && !s.colSym.empty() && nowMs - bk.sabStartMs >= g_sabGraceMs) {
                    for (int idx = std::max(sab::EXT_BARS, nBars - 7); idx <= nBars - 2; ++idx) {
                        const RTDATE raw = static_cast<RTDATE>(times[idx]);
                        const long long key = localSec(raw);
                        if (key <= 0 || bk.sabDone.count(key)) continue;
                        const int from = std::max(0, idx - sab::EFFORT_BARS - 15);
                        std::vector<sab::Bar> SB; bool okAll = true;
                        for (int j = from; j <= idx; ++j) {
                            sab::Bar x;
                            try { x.o = dp::priceKey(open[j], tick); x.h = dp::priceKey(high[j], tick); x.l = dp::priceKey(low[j], tick); x.c = dp::priceKey(close[j], tick); x.ok = true; } catch (...) { x.ok = false; }
                            BARSTATISTICS st; std::memset(&st, 0, sizeof(st));
                            const int pr = x.ok ? readStatistics(&vap, j, &st) : -2;
                            if (pr == -1) { okAll = false; break; }                       // a VAP fault: never decided on it
                            if (pr >= 0 && st.buyVolume >= 0 && st.sellVolume >= 0) { x.buy = static_cast<long long>(st.buyVolume); x.sell = static_cast<long long>(st.sellVolume); }
                            else if (j == idx) okAll = false;                             // the candle's own lots not read yet: later
                            SB.push_back(x);
                        }
                        if (!okAll) continue;
                        const size_t bi = SB.size() - 1;
                        std::vector<sab::Lv> lv;
                        for (const zn::NL& z : zn::levelsNamed(fdir, s.market, s.book, nowMs)) { sab::Lv q; q.name = z.name; q.px = z.px; lv.push_back(q); }
                        for (int sd = 0; sd < 2; ++sd) {
                            if (bk.sab.st.has(sd, key)) { ++bk.sab.adopted; continue; }
                            const sab::Decision D = sab::decide(SB, bi, sd, bk.sab.P.E);
                            if (!D.decided) { ++bk.sab.learning; continue; }
                            bk.sab.count(D);
                            if (!D.fired) continue;
                            sab::Rec r; r.mkt = s.market; r.spb = spbNow; r.key = key; r.raw = static_cast<long long>(raw); r.side = sd; r.effort = D.effort; r.E = D.E;
                            r.att = D.att; r.avg = D.avg; r.close = sd == sab::LOW ? 'G' : 'R';
                            r.extPx = static_cast<double>(sd == sab::HIGH ? SB[bi].h : SB[bi].l) * tick; r.closePx = static_cast<double>(SB[bi].c) * tick;
                            r.book = -1; r.decided = localSec(static_cast<RTDATE>(times[nBars - 1])); if (r.decided <= 0) r.decided = key;   // decided at the forming candle's stamp (~ now)
                            r.by = "DP"; r.loc = sab::locName(sab::LOCATION);
                            sab::levelOf(lv, r.extPx, r.closePx, sab::atrTicks(SB, bi) * tick, r.level, r.lvd, r.gamma);
                            bk.sab.put(r);
                            sab::Extra x; x.mkt = s.market; x.spb = spbNow; x.key = key; x.side = sd; x.by = "DP";
                            if (!r.level.empty()) { x.kv["level"] = r.level; x.kv["lvd"] = sab::fmt(r.lvd, 2); }
                            if (r.gamma) x.kv["gamma"] = std::to_string(r.gamma);
                            bk.sab.putX(x);
                        }
                        bk.sabDone.insert(key);
                    }
                    while (bk.sabDone.size() > 4000) bk.sabDone.erase(bk.sabDone.begin());
                }
            }
        }
        // (2.21.0) TRAPPED TRADERS (TrapLogic.h): every recently closed candle, decided once at its close (never repainted): a new 20-candle
        // high / low, the attackers' lots beyond the old extreme >= 1.0x the median attack of the 20 candles before it, the close back inside.
        if (s.trapOn && allowedNow) {
            const int nBars = static_cast<int>(count);
            for (std::size_t bi = bars.size() > 6 ? bars.size() - 6 : 0; bi < bars.size(); ++bi) {
                const dp::Bar& B = bars[bi];
                const int idx = B.index;
                if (!B.closed || idx < trapl::LOOKBACK || idx >= nBars - 1) continue;
                const RTDATE raw = static_cast<RTDATE>(times[idx]);
                const long long key = localSec(raw);
                if (key <= 0) continue;
                trapl::Candle c; bool ok = true;
                try { c.high = dp::priceKey(high[idx], tick); c.low = dp::priceKey(low[idx], tick); c.close = dp::priceKey(close[idx], tick); } catch (...) { ok = false; }
                if (!ok) continue;
                for (const dp::Row& r : B.rows) { trapl::Row x; x.key = r.key; x.bought = (r.volume + r.delta) * 0.5; x.sold = (r.volume - r.delta) * 0.5; c.rows.push_back(x); }
                long long hx = LLONG_MIN, lx = LLONG_MAX;
                for (int k = idx - trapl::LOOKBACK; k < idx && ok; ++k) { try { hx = std::max<long long>(hx, static_cast<long long>(dp::priceKey(high[k], tick))); lx = std::min<long long>(lx, static_cast<long long>(dp::priceKey(low[k], tick))); } catch (...) { ok = false; } }
                if (!ok) continue;
                for (int sd = 0; sd < 2; ++sd) {
                    const bool top = sd == 0;
                    const long long dk = key * 2 + (top ? 1 : 0);
                    if (bk.trapDone.count(dk)) continue;
                    bk.trapDone.insert(dk);
                    std::vector<double> prior;                                    // the attack of the earlier candles in the window (same side)
                    for (std::size_t pj = 0; pj < bars.size(); ++pj) {
                        const dp::Bar& P = bars[pj];
                        if (P.index < idx - trapl::LOOKBACK || P.index >= idx) continue;
                        double a = 0; for (const dp::Row& r : P.rows) a += top ? (r.volume + r.delta) * 0.5 : (r.volume - r.delta) * 0.5;
                        prior.push_back(a);
                    }
                    const trapl::Result R = trapl::decide(c, top, top ? hx : lx, prior);
                    if (!R.broke) continue;
                    ++bk.trapEval;
                    if (R.prior < trapl::MIN_PRIOR) { ++bk.trapLearning; continue; }
                    if (!R.backIn) { ++bk.trapAccepted; continue; }
                    if (!R.fired) { ++bk.trapLight; continue; }
                    if (bk.traps.size() >= 2000) continue;
                    trapl::Rec t; t.id = bk.trapNext++; t.top = top; t.barT = key; t.barRaw = static_cast<unsigned long long>(raw); t.lo = R.lo; t.hi = R.hi;
                    t.tick = tick; t.lots = R.lots; t.med = R.med; t.x = R.x; t.prevX = top ? hx : lx; t.extreme = top ? c.high : c.low; t.closeK = c.close;
                    t.version = DLT_VERSION; t.root = s.book;
                    bk.traps.push_back(t); bk.journal.push_back(trapl::line(t)); ++bk.trapFired;
                    char b[160]; std::snprintf(b, sizeof(b), "%s %lld:%02lld: %.0f lots beyond the old %s (%.1fx the median attack %.0f)", top ? "trapped buyers" : "trapped sellers",
                                               (key % 86400) / 3600, (key % 3600) / 60, R.lots, top ? "high" : "low", R.x, R.med);
                    bk.trapLast = b;
                }
            }
            if (bk.trapDone.size() > 4000) bk.trapDone.erase(bk.trapDone.begin());
        }
        // (2.5.4) a decision is recorded once; lines are marked written only after a flushed, good append (checklist #6)
        for (std::size_t k = 0; k < latches.size(); ++k) {
            Latch& L = latches[k];
            if (!decided(L.code) || L.logged) continue;
            L.tDecided = nowT; L.logged = true; L.version = DLT_VERSION;   // (2.8.5) decided by this version
            bk.journal.push_back(recordLine(L, 'D', tick, s.book));
        }
        // (2.8.5) display dispositions for the confirmed A events (records untouched):
        //   rule 2 - an A decided by a version before 2.8.4 is re-checked ONCE against the 2.8.4 rules on the chart's bars 0 .. peak
        //            (cached once the bars were there; never repeated, so a later bar or a scroll cannot flip it); 2.8.4+ never;
        //   rule 1 - one bubble per candle per side (dp::oneBubblePerCandle), among the events that are drawable after rule 2.
        {
            std::vector<dp::BubbleCand> cands;
            bk.show.clear();
            for (const Latch& L : latches) {
                if (!drawableAbsorption(L)) continue;
                if (!L.tent && dp::versionBefore(L.version, 2, 8, 4)) {
                    std::map<long long, dp::Recheck>::const_iterator rc = bk.recheck.find(L.id);
                    if (rc == bk.recheck.end()) {
                        const int pk = findBar(L.peakTime);
                        const dp::Recheck r = pk < 0 ? dp::Recheck() :
                            dp::recheckA(open, high, low, close, static_cast<int>(count), pk, L.low, L.high, tick, L.sell);
                        if (r.bars) rc = bk.recheck.insert(std::make_pair(L.id, r)).first;
                    }
                    if (rc == bk.recheck.end()) { bk.show[L.id] = "undrawn: pre-2.8.4 re-check - bars not on chart"; continue; }
                    const dp::Recheck& r = rc->second;
                    if (!r.pass()) {
                        std::string why;
                        if (!r.close) why += "close";
                        if (!r.halfway) why += std::string(why.empty() ? "" : "+") + "halfway";
                        if (!r.pivot) why += std::string(why.empty() ? "" : "+") + "pivot";
                        bk.show[L.id] = "hidden: pre-2.8.4 rule (" + why + ")"; continue;
                    }
                }
                dp::BubbleCand c; c.id = L.id; c.candle = static_cast<long long>(L.peakTime); c.tDecided = L.tDecided;
                c.support = L.support; c.price = L.price; c.mult = L.mult; cands.push_back(c);
            }
            // (2.15.0, Rassul 2026-10-17 13:24 "a candle can have more than 1 bubble") every confirmed A draws its own bubble (2.6.0)
            for (const dp::BubbleCand& c : cands) bk.show[c.id] = std::string();
        }
        // (2.7.0) the thin-extreme exhaustion candidates: decided on closed bars after this build's A decisions, logged as E lines
        // in the same record (flushed just below), the same complete-build gate as A
        if (allowedNow) { try { absorbExhaust(s, bk, bars, tick); } catch (...) { bk.exNote = "AE step failed - retried on the next build"; } }
        if (allowedNow) { try { exhaust(s, bk, bars, tick); } catch (...) { bk.exNote = "exhaustion step failed - retried on the next bar"; } }
        if (!bk.journal.empty()) {
            if (bk.path.empty()) bk.journal.clear();                                                 // no storage: nothing to keep
            else {
                makeDirectory(recordDirectory());
                if (appendRecord(bk.path, bk.journal)) { bk.written += static_cast<long long>(bk.journal.size()); bk.journal.clear(); if (bk.carry.empty()) bk.note.clear(); }
                else bk.note = "record write failed - kept, retried on the next change";
            }
        }
        // Join by EVENT ID, never by spatial overlap with an unrelated historic episode.
        for (dp::Node& n : candidate.nodes) {
            const Latch* L = eventById(bk, n.state.eventId);
            if (L) { n.state.code = L->code; n.state.support = L->support; n.price = L->price; }
        }
        for (dp::Zone& z : candidate.zones) {
            const Latch* L = eventById(bk, z.eventId);
            if (L) { z.code = L->code; z.support = L->support; }
        }
    }
    if (s.shortHistory || coarseBar) { candidate.nodes.clear(); candidate.zones.clear(); }
    s.snapshot = std::move(candidate); s.ready = true;
    s.state = s.shortHistory ? "short chart history: provisional signals" : coarseBar ?
        "boundary bar crosses window: provisional signals" : "ready";
    struct tm t = {};
    if (getLocaltime(latest, &t)) {
        char b[32]; std::strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &t); s.asof = b;
    }
    return true;
}
// (2.7.0, scope update 10:33) AE: each CONFIRMED A is watched bar by bar (each closed bar once) for up to aeMaxBars bars after its
// peak bar: at the close of a bar that re-trades the node, the attackers' delta into it is compared later half vs earlier half on
// the closed bars up to that bar (decision-time data only). The first fade decides Y; none within aeMaxBars decides N. The result
// and the label it gives the A (A / AE? / AE) are frozen in one AE line and never change.
void DeltaProfile::absorbExhaust(ChartState& s, LatchBook& bk, const std::vector<dp::Bar>& window, double tick) {
    const long count = getBarCount();
    RTARRAYI times(barDateTime);
    if (count < 2 || times.count < count || window.empty()) return;
    const int lastClosed = static_cast<int>(count) - 2;
    const std::string directory = storageDirectory();
    const ExConfig& cfg = exConfig(directory, s.market);
    auto findBar = [&](RTDATE when) -> int {
        int lo = 0, hi = static_cast<int>(count) - 1;
        while (lo <= hi) {
            const int mid = (lo + hi) / 2; const RTDATE t = static_cast<RTDATE>(times[mid]);
            if (t == when) return mid;
            if (t < when) lo = mid + 1; else hi = mid - 1;
        }
        return -1;
    };
    std::int64_t end;
    if (!epoch(static_cast<RTDATE>(times[static_cast<int>(count) - 1]), end)) return;
    const std::int64_t windowCut = end - 90 * 60;                                       // the window holds every bar after this
    for (const Latch& L : bk.v) {
        if (!confirmedAbsorption(L) || bk.ae.count(L.id)) continue;
        const int pk = findBar(L.peakTime);
        if (pk >= lastClosed + 1) continue;                                             // its peak bar still forming
        AeRec a; a.peakRaw = L.peakTime; a.lo = static_cast<double>(L.low) * tick; a.hi = static_cast<double>(L.high) * tick;
        a.sellNode = L.sell; a.fadeX = cfg.p.aeFadeX; a.params = aeParamText(cfg);
        bool decided = false;
        if (pk < 0) { a.result = 'U'; a.decRaw = static_cast<RTDATE>(times[lastClosed]); decided = true; }   // its peak bar left the chart
        else {
            RTDATE& through = bk.aeThrough[L.id];
            for (int d = pk + 1; d <= lastClosed && !decided; ++d) {
                const RTDATE raw = static_cast<RTDATE>(times[d]);
                if (raw <= through) continue;
                through = raw;
                if (d - pk > cfg.p.aeMaxBars) { a.result = 'N'; a.decRaw = raw; decided = true; break; }   // always bar pk + aeMaxBars + 1
                const dp::Bar* bd = NULL;
                for (const dp::Bar& b : window) if (b.index == d) { bd = &b; break; }
                // the hour before d must be wholly inside the window; a bar evaluated live had it, so skipping here after a
                // restart cannot hide a fade the live pass would have found
                if (!bd || (window.front().index != 0 && bd->time - 3600 < windowCut)) continue;
                dp::exh::AeResult r;
                if (!dp::exh::attackerFade(window, d, bd->time - 3600, L.low, L.high, L.sell, cfg.p.aeFadeX, cfg.p.aeMinBars, r) || !r.found) continue;
                a.bars = r.bars; a.early = r.early; a.late = r.late; a.fade = r.fade; a.result = 'Y'; a.decRaw = raw; decided = true;
            }
        }
        if (!decided) continue;
        a.decT = localSec(a.decRaw);
        a.label = a.result == 'Y' && cfg.p.showAe > 0 ? (cfg.p.showAe == 2 ? "AE" : "AE?") : "A";
        bk.ae[L.id] = a; bk.aeThrough.erase(L.id); bk.journal.push_back(aeLine(L.id, a, s.book));
    }
}
static std::string aeLabel(const LatchBook& bk, long long id) {
    // (2.8.1, Rassul 18:32 "no AE") every drawn label says A; the AE result is still measured, recorded (AE| lines) and in the status
    (void)bk; (void)id; return std::string("A");
}
static std::string aeWords(const AeRec& a) {
    std::ostringstream o; o.imbue(std::locale::classic());
    const char* who = a.sellNode ? "sellers" : "buyers";
    if (a.result == 'U') o << "not measured (the A's bars are no longer on hand)";
    else if (a.result == 'N' && a.fade < 0) o << who << " never re-tested the node enough to measure a fade; stays A";
    else o << who << " pressing into the node: later bars " << exRatio(a.fade) << " the earlier bars (" << a.bars << " bars)"
           << (a.result == 'Y' ? "; they ran out inside the absorption" : "; not fading") << "; label " << a.label;
    return o.str();
}
// (2.7.0) the thin-extreme exhaustion step. Every closed bar is evaluated ONCE per session book (exThrough), on both sides; a
// candidate is frozen when first logged (C line) and decided once by later CLOSED bars (P / X line). Nothing here is ever undone.
void DeltaProfile::exhaust(ChartState& s, LatchBook& bk, const std::vector<dp::Bar>& window, double tick) {
    const long count = getBarCount();
    if (count < 3 || window.empty()) return;
    const int lastClosed = static_cast<int>(count) - 2;
    RTARRAYI times(barDateTime);
    RTARRAY open(barOpen), high(barHigh), low(barLow), close(barClose);
    if (times.count < count || open.count < count || high.count < count || low.count < count || close.count < count) return;
    const std::string directory = storageDirectory();
    const ExConfig& cfg = exConfig(directory, s.market);
    s.exParams = exParamText(cfg); s.exSource = cfg.src;
    auto findBar = [&](RTDATE when) -> int {
        int lo = 0, hi = static_cast<int>(count) - 1;
        while (lo <= hi) {
            const int mid = (lo + hi) / 2; const RTDATE t = static_cast<RTDATE>(times[mid]);
            if (t == when) return mid;
            if (t < when) lo = mid + 1; else hi = mid - 1;
        }
        return -1;
    };
    const RTDATE lastT = static_cast<RTDATE>(times[lastClosed]);
    if (lastT > bk.exThrough) {
        std::string note;
        const std::vector<dp::exh::Level> levels = exLevels(directory, s.market, s.book, note);
        s.exLevelNote = note; s.exLevelCount = levels.size();
        if (!(cfg.p.requireLevel && levels.empty())) {                                 // no level at all: nothing can qualify
            const int spb = std::max(1, getSecondsPerBar());
            const int H = std::max(std::max(cfg.p.N, cfg.p.M), 3) + 3;
            std::int64_t end;
            if (!epoch(static_cast<RTDATE>(times[static_cast<int>(count) - 1]), end)) return;
            // older bars than the 90-min window: enough for the move (M bars) and a full 90-min profile behind the extreme
            const std::int64_t exCut = end - 5400 - std::max<std::int64_t>(5400, static_cast<std::int64_t>(H) * spb);
            RTARRAYP vap(barVolumeProfile);
            std::vector<dp::Bar> bars; std::vector<VOLPROFILE> rows; std::size_t examined = 0;
            std::int64_t nextTime = window.front().time;
            for (int i = window.front().index - 1; i >= 0 && bars.size() < 600 && vap.count >= count; --i) {
                std::int64_t stamp;
                if (!epoch(static_cast<RTDATE>(times[i]), stamp)) break;
                if (stamp > nextTime) stamp = nextTime;
                if (stamp <= exCut) break;
                nextTime = stamp;
                BARSTATISTICS stats;
                const int prices = readStatistics(&vap, i, &stats);
                if (prices == -1) return;                                               // VAP fault: retried on the next build
                if (prices < 0) continue;                                               // a gap: candidates spanning it are not evaluated
                examined += static_cast<std::size_t>(prices);
                if (examined > 2000000) break;
                rows.resize(static_cast<std::size_t>(prices));
                const int read = readPrices(&vap, i, rows.empty() ? NULL : &rows[0], prices);
                if (read == -1) return;
                if (read < 0) continue;
                dp::Bar b; b.index = i; b.time = stamp; b.closed = true;
                b.open = open[i]; b.high = high[i]; b.low = low[i]; b.close = close[i];
                bool bad = false;
                for (std::size_t k = 0; k < rows.size(); ++k) {
                    const VOLPROFILE& v = rows[k];
                    if (!dp::finite(v.price) || v.buyVolume < 0 || v.sellVolume < 0 || v.totalVolume < 0) continue;
                    const double buy = static_cast<double>(v.buyVolume), sell = static_cast<double>(v.sellVolume);
                    const double vol = std::max(static_cast<double>(v.totalVolume), buy + sell);
                    if (vol == 0) continue;
                    try { b.rows.push_back(dp::Row(dp::priceKey(v.price, tick), buy - sell, vol)); } catch (...) { bad = true; break; }
                }
                if (bad) continue;
                try { dp::normalize(b); } catch (...) { continue; }
                bars.push_back(std::move(b));
            }
            std::reverse(bars.begin(), bars.end());
            bars.insert(bars.end(), window.begin(), window.end());
            std::vector<std::pair<long long, dp::exh::AEvent> > as;                     // confirmed A's: (decided-by bar end, event)
            for (const Latch& L : bk.v) {
                if (!confirmedAbsorption(L)) continue;
                const int pk = findBar(L.peakTime);
                if (pk >= 0) as.push_back(std::make_pair(L.decidedBy, dp::exh::AEvent{ pk, L.price, !L.support }));
            }
            for (std::size_t p = 0; p < bars.size(); ++p) {
                const dp::Bar& b = bars[p];
                if (!b.closed || b.index > lastClosed) continue;
                const RTDATE raw = static_cast<RTDATE>(times[b.index]);
                if (raw <= bk.exThrough) continue;
                const long long bEnd = localSec(raw);
                std::vector<dp::exh::AEvent> known;                                     // decided at or before the extreme's close
                for (std::size_t k = 0; k < as.size(); ++k) if (as[k].first > 0 && as[k].first <= bEnd) known.push_back(as[k].second);
                for (int side = 0; side < 2; ++side) {
                    const bool top = side == 0;
                    bool seen = false;
                    for (const ExMark& m : bk.ex) if (m.top == top && m.extRaw == raw) { seen = true; break; }
                    if (seen || bk.ex.size() >= EX_MAX_MARKS) continue;
                    dp::exh::Candidate c; bool ok = false;
                    try { ok = dp::exh::evaluate(bars, p, top, cfg.p, levels, known, tick, c); } catch (...) { ok = false; }
                    if (!ok) continue;
                    ExMark m; m.id = bk.exNext++; m.top = top; m.extRaw = raw; m.extT = bEnd; m.decRaw = raw; m.decT = bEnd;
                    m.extreme = c.extreme > 0 ? c.extreme : (top ? b.high : b.low);
                    m.level = c.level; m.levelPx = c.levelPx; m.levelTicks = c.levelTicks; m.thin = c.thin; m.fade = c.fade;
                    m.thinOk = c.thinOk; m.fadeOk = c.fadeOk; m.ctx = c.ctx; m.ctxPx = c.ctxPx;
                    m.ctxT = c.ctxIndex >= 0 && c.ctxIndex < static_cast<int>(count) ? localSec(static_cast<RTDATE>(times[c.ctxIndex])) : 0;
                    m.live = b.index == lastClosed; m.W = cfg.p.W; m.params = exParamText(cfg);
                    const bool gate = cfg.p.show == 2 || (cfg.p.show == 1 && c.ctx != 'N');
                    m.mark = c.qualifies() && gate ? "Ex?" : "none";   // (2.8.4) always "Ex?" in the record (never drawn; not proven)
                    bk.ex.push_back(m); bk.journal.push_back(exLine(m, 'C', tick, s.book));
                }
            }
        }
        bk.exThrough = lastT;
    }
    for (ExMark& m : bk.ex) {                                                           // decide pending candidates, once
        if (m.status != 'C') continue;
        const int e = findBar(m.extRaw);
        if (e < 0) continue;
        int at = -1;
        const int r = dp::exh::resolve(open, high, low, close, lastClosed, e, m.top, m.W, at);
        if (r == 0 || at < 0 || at >= static_cast<int>(count)) continue;
        m.status = r == 1 ? 'P' : 'X'; m.decRaw = static_cast<RTDATE>(times[at]); m.decT = localSec(m.decRaw);
        m.reason = r == 1 ? (m.top ? "red close" : "green close") : r == 2 ? (m.top ? "cancelled: higher high first" : "cancelled: lower low first") :
            "expired: no close back within W bars";
        bk.journal.push_back(exLine(m, m.status, tick, s.book));
    }
}
// (2.7.0) the mark's detail in plain words (status file EXH lines - the plugin has no hover)
static std::string exWords(const ExMark& m) {
    std::ostringstream o; o.imbue(std::locale::classic());
    o << (m.mark == "none" ? std::string("Ex (not shown)") : m.mark) << " thin " << (m.top ? "top " : "bottom ") << exRatio(m.thin)
      << " delta; aggression " << exRatio(m.fade);
    if (m.ctx == 'A') o << "; after A at " << exNum(m.ctxPx);
    else if (m.ctx == 'T') o << "; after trapped " << (m.top ? "buyers" : "sellers") << " at " << exNum(m.ctxPx);
    else o << "; no earlier absorption or trapped delta";
    if (m.levelTicks >= 0) o << "; at " << m.level << ' ' << exNum(m.levelPx) << " (" << m.levelTicks << " ticks)";
    o << (m.status == 'C' ? "; waiting for a close back" : m.status == 'P' ? "; printed on the " + m.reason : "; " + m.reason);
    if (!(m.thinOk && m.fadeOk)) o << "; did not qualify (" << (m.thinOk ? "" : "not thin") << (!m.thinOk && !m.fadeOk ? " and " : "") << (m.fadeOk ? "" : "aggression not fading") << ')';
    return o.str();
}
static short coordinate(int v) { return static_cast<short>(std::max<int>(SHRT_MIN, std::min<int>(SHRT_MAX, v))); }
int DeltaProfile::yOf(int lastBar, double price) {
    PNT point; point.set(lastBar, static_cast<float>(price)); return point.v;
}
int DeltaProfile::textWidth(const std::string& text, int font, bool bold) {
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f);
    return getTextWidth(text.c_str(), -1);
}
void DeltaProfile::textRight(int right, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (inMir(right)) { textLeftRaw(mx(right), y, text, color, font, bold); return; }   // (2.27.0) mirrored column
    textRightRaw(right, y, text, color, font, bold);
}
void DeltaProfile::textRightRaw(int right, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (text.empty()) return;
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(color);
    const int yy = y - static_cast<int>(font * 0.45 + 0.5);
    if (bdk::hide(right - getTextWidth(text.c_str(), -1), yy - font, right, yy + font)) return;   // (2.21.2) under the HOD/LOD table: not painted
    RCT rect; rect.set(coordinate(right - 400), coordinate(yy - font), coordinate(right), coordinate(yy + font));
    rect.drawText(text.c_str(), false, true);
}
void DeltaProfile::textLeft(int left, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (inMir(left)) { textRightRaw(mx(left), y, text, color, font, bold); return; }   // (2.27.0) mirrored column
    textLeftRaw(left, y, text, color, font, bold);
}
void DeltaProfile::textLeftRaw(int left, int y, const std::string& text, COLOR color, int font, bool bold) {
    if (text.empty()) return;
    FONT f; f.id = HELVETICA; f.size = coordinate(font); f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(color);
    const int yy = y - static_cast<int>(font * 0.45 + 0.5);
    if (bdk::hide(left, yy - font, left + getTextWidth(text.c_str(), -1), yy + font)) return;   // (2.21.2)
    RCT rect; rect.set(coordinate(left), coordinate(yy - font), coordinate(left + 400), coordinate(yy + font));
    rect.drawText(text.c_str(), false, false);
}
void DeltaProfile::stroke(int x1, int y1, int x2, int y2, COLOR color, int width) {
    x1 = mx(x1); x2 = mx(x2);   // (2.27.0) mirrored column
    setPen(color, coordinate(width), P_SOLID);
    ko::line(coordinate(x1), coordinate(y1), coordinate(x2), coordinate(y2));   // (2.21.2) cut at the HOD/LOD table
}
// (2.4.3) the absorption circle: on the bar where the node's delta peaked, at the node's price
void DeltaProfile::ringAt(int bar, double price, COLOR color, int left, int right, int top, int bottom) {
    PNT point; point.set(bar, static_cast<float>(price), kBarCenter);
    if (point.h - 6 < left || point.h + 6 > right || point.v - 6 < top || point.v + 6 > bottom) return;
    setPen(color, 2, P_SOLID); CBRUSH brush(color, PAT_HOLLOW); brush.set();
    RCT ring; ring.set(coordinate(point.h - 6), coordinate(point.v - 6), coordinate(point.h + 6), coordinate(point.v + 6));
    ko::oval(ring, DRAW_OPAQUE);   // (2.21.2) a circle under the HOD/LOD table: not painted (still recorded)
}
void DeltaProfile::box(int left, int top, int right, int bottom, COLOR color) {
    if (right <= left || bottom <= top) return;
    if (inMir(left) && inMir(right)) { const int l2 = mx(right), r2 = mx(left); left = l2; right = r2; }   // (2.27.0) mirrored column
    RCT rect; rect.set(coordinate(left), coordinate(top), coordinate(right), coordinate(bottom));
    ko::fill(rect, (short)0, color, color, DRAW_OPAQUE, PAT_SOLID);   // (2.21.2) the part under the HOD/LOD table is not painted
}
static std::string amount(double d) {
    std::ostringstream out; out << (d > 0 ? '+' : '-');
    if (std::fabs(d) >= 1000) out << std::fixed << std::setprecision(1) << std::fabs(d) / 1000 << 'k';
    else out << std::fixed << std::setprecision(0) << std::fabs(d);
    return out.str();
}
static std::string amountAbs(double d) { return amount(d).substr(1); }   // (2.8.6, Rassul 10-15) the A label shows the size only - the colour already gives the side
static std::string ratio(double x) {
    std::ostringstream out; out << std::fixed << std::setprecision(x < 10 ? 1 : 0) << x;
    std::string s = out.str();
    if (s.size() > 2 && s.substr(s.size() - 2) == ".0") s.resize(s.size() - 2);
    return s + 'x';
}
bool DeltaProfile::render(ChartState& s) {
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int paneRight = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneRight = scale.left - 2;
    // (2.8.0) the middle right-side column (ColumnBoard.h): [delta.l, delta.r], left of the Dealer column, right of the Levels column
    columns(s, pane, paneRight);
    const int colW = s.lay.w[cb::DELTA];
    const int base = s.lay.delta.r - 1;                 // every bar anchored here, growing left (toward price)
    // (2.28.0, 2026-10-28, Rassul 19:25 CT "a seperate column that interprets delta profile right before the delta profile column ... the delta
    // profile can have just the delta profile and the ratio") the READ strip: the first READ_W px of this column on the price side carries the
    // tested interpretation codes (A only, research 2026-10-27/28: every other candidate failed vs the same candle without it); the bars and
    // their ratio labels stay right of it
    static const int READ_W = 26;
    const int readL = s.lay.delta.l + 1, readR = readL + READ_W;
    const int left = readR + 2;                         // nothing of the profile is drawn left of this
    const int priceLimit = s.lay.priceRight;            // A circles / Ex marks stay left of the gap before the Levels column
    const int candleLeft = cb::DELTA_LEFT ? std::max((int)pane.left, s.lay.delta.r + 2) : (cb::LEVELS_LEFT && s.lay.levels.w() > 0 ? std::max((int)pane.left, s.lay.levels.r + 2) : (int)pane.left);   // (2.27.0 / 2.28.0) circles / marks stay right of the left column
    const int top = pane.top + 16, bottom = pane.bottom;
    if (!s.lay.ok || colW <= 0 || priceLimit <= pane.left + 40 || base >= paneRight || bottom <= top) {
        s.state = "not drawn: insufficient pane room"; return false;
    }
    struct MirrorScope { DeltaProfile* p; MirrorScope(DeltaProfile* q, int l, int r) : p(q) { p->mirL = l; p->mirR = r; } ~MirrorScope() { p->mirL = 0; p->mirR = -1; } };
    MirrorScope mirror(this, cb::DELTA_LEFT ? s.lay.delta.l : 0, cb::DELTA_LEFT ? s.lay.delta.r : -1);   // (2.27.0) bars point toward price
    if (cb::DELTA_LEFT) box(s.lay.delta.l, pane.top, s.lay.delta.r, bottom, C_COLBG);   // (2.27.0) the left column on its own solid background (candles pass behind)
    {   const std::string title = std::string("DLT v") + DLT_VERSION;   // (2.8.0) the version next to the title
        textLeft(left + 1, pane.top + 8, textWidth(title, 8, true) <= colW - READ_W - 6 ? title : std::string("DLT"), C_MUTED, 8, true);
        textLeft(readL + 1, pane.top + 8, "READ", C_MUTED, 7, true);   // (2.28.0) the READ strip's title
        stroke(readR, pane.top + 14, readR, bottom, C_AXIS, 1); }
    if (!s.ready) textRight(base - 2, top + 12, "NO VAP", C_SELL, 8, true);
    if (s.shortHistory || s.state.find("provisional") != std::string::npos)
        textRight(base - 2, top + 12, "PARTIAL", C_MUTED, 8, true);
    stroke(base, top, base, bottom, C_AXIS, 1);
    const dp::Snapshot unavailable;
    const dp::Snapshot& profile = s.ready ? s.snapshot : unavailable;
    const int lastBar = s.barCount - 1;
    // (2.8.0) the room for the text on the price side of the bars (delta values, node labels): the widest visible row value, at most
    // half the column; the bars use the rest
    int reserve = 0;
    if (s.layout.text && s.ready) {
        for (std::size_t i = 0; i < s.snapshot.buckets.size(); ++i) if (s.snapshot.buckets[i].delta != 0)
            reserve = std::max(reserve, textWidth(amount(s.snapshot.buckets[i].delta), s.layout.font, true) + 4);
        reserve = std::min(reserve, (colW - 2) / 2);
    }
    if (s.ready) {   // the node labels' short form ("AE? 2.1x") keeps its room too, up to 3/5 of the column
        int lab = 0;
        for (std::size_t i = 0; i < s.snapshot.nodes.size(); ++i) {
            const dp::Node& n = s.snapshot.nodes[i];
            lab = std::max(lab, textWidth(ratio(n.ratio), s.layout.font, true) + 6);   // (2.28.0) the label as drawn: the ratio only ("7.2x"); the A is in READ
        }
        reserve = std::max(reserve, std::min(lab, colW * 2 / 5));   // (2.27.1) at most 2/5 of the column (was 3/5): the bars keep the rest
    }
    // (2.21.0) a trap badge ("A|Tr?" + its box + tick) keeps its room left of the bars while a trap is in the record (the trapped rows are
    // often the longest bars, so without this the badge would never fit beside them)
    // (2.27.1) no trap badge on the column any more: no room kept for it
    const int full = std::max(4, colW - 3 - reserve - READ_W - 2);   // (2.28.0) the READ strip keeps its room
    const LatchBook& book = latchBooks()[s.book + "|" + std::to_string(getSecondsPerBar())];
    struct LBox { int l, t, r, b; };
    auto overlap = [](const LBox& a, const LBox& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; };
    struct Bubble { LBox box; int bar; double price; bool support; long long id; int offset; bool gray = false; bool shared = false; };   // (2.22.0) shared = the SHARED A (letter A in its ring)   // (2.19.0) gray = big node, no winner (yet)
    std::vector<Bubble> bubbles;
    // (2.9.0, Rassul approved mockup-location-first-v1 08:22 CT) LOCATION FIRST: an A at a high / low / pivot / key or MenthorQ level
    // is drawn full strength; mid-range it is dimmed (never hidden, never moved) - ZoneNear.h, using only bars up to the A's own bar
    zn::Ctx Z;
    {   const long cnt = getBarCount(); RTARRAYI zt(barDateTime); RTARRAY zh(barHigh), zl(barLow);
        if (cnt > 2 && zt.count >= cnt && zh.count >= cnt && zl.count >= cnt) {
            for (long i = 0; i < cnt; ++i) { Z.hi.push_back(zh[(int)i]); Z.lo.push_back(zl[(int)i]); Z.sess.push_back(localSessionKey(static_cast<RTDATE>(zt[(int)i]), 0)); }
            Z.band = zn::bandFrom(Z.hi, Z.lo, (int)cnt - 2); Z.levels = zn::levels(storageDirectory(), s.market, s.book, steadyMs());
        } }
    auto zoneFor = [&](long long id) { for (const Bubble& q : bubbles) if (q.id == id) return zn::atZone(Z, q.bar, q.price); return true; };
    s.zoneFull = s.zoneDim = 0;
    s.bubbleState.clear(); s.profileState.clear();
    s.absorptionEvents = s.bubblesDrawn = s.bubblesOffscreen = s.bubblesSuppressed = s.activeAbsorptions = s.pairedAbsorptions = 0;
    RTARRAYI bubbleTimes(barDateTime);
    // Immutable event order: a newly qualifying mark never evicts a previously retained bubble.
    std::vector<const Latch*> confirmed;
    // (2.8.5) only the event holding its candle's bubble (rule 1) that is not hidden by the pre-2.8.4 re-check (rule 2) is drawn; the
    // others keep their record and report their disposition ("merged:<id>", "hidden: pre-2.8.4 rule (...)", "undrawn: ...")
    s.bubblesMerged = s.aHiddenPre284 = s.aUncheckedPre284 = 0;
    for (const Latch& L : book.v) {
        if (!drawableAbsorption(L)) continue;
        const std::map<long long, std::string>::const_iterator d = book.show.find(L.id);
        const std::string q = d == book.show.end() ? std::string("undrawn: not decided for display yet") : d->second;
        if (q.empty()) { confirmed.push_back(&L); continue; }
        ++s.absorptionEvents; s.bubbleState[L.id] = q;
        if (q.compare(0, 7, "merged:") == 0) ++s.bubblesMerged; else if (q.compare(0, 7, "hidden:") == 0) ++s.aHiddenPre284; else ++s.aUncheckedPre284;
    }
    // (2.22.0, Rassul 06:46 "only one circle per candle" + 06:49 "then require it") ONE circle per candle on the price chart, display only
    // (records unchanged): the SHARED A wins its candle; else ONE node A - the biggest |delta| of the candle's top-3 node A's - and only when
    // the candle closed in the DEFENDERS' colour (green for a bullish A, red for a bearish one); a node A on an attacker-colour candle shows
    // as the one gray circle (no winner); the retired 2.20.0 wick A records are not drawn.
    RTARRAY dOpen(barOpen), dClose(barClose);
    auto barOf = [&](RTDATE t) -> int { if (t == 0 || bubbleTimes.count < s.barCount) return -1; int lo2 = 0, hi2 = s.barCount - 1;
        while (lo2 <= hi2) { const int mid = (lo2 + hi2) / 2; const RTDATE q = static_cast<RTDATE>(bubbleTimes[mid]); if (q == t) return mid; if (q < t) lo2 = mid + 1; else hi2 = mid - 1; } return -1; };
    auto defClose = [&](int at, bool bullish) -> bool { if (at < 0 || at >= dOpen.count || at >= dClose.count) return false;
        const double o = dOpen[at], c = dClose[at]; return dp::finite(o) && dp::finite(c) && (bullish ? c > o : c < o); };
    // (2.26.0, Rassul 2026-10-27 00:38 CT, placement only) the circle's centre: bullish wholly below the candle body, bearish wholly above
    auto bodyY = [&](int at, int yNode, bool bullish) -> int { if (at < 0 || at >= dOpen.count || at >= dClose.count) return yNode;
        const double o = dOpen[at], c = dClose[at]; if (!dp::finite(o) || !dp::finite(c)) return yNode;
        PNT t, b; t.set(at, static_cast<float>(std::max(o, c)), kBarCenter); b.set(at, static_cast<float>(std::min(o, c)), kBarCenter);
        return dp::outsideBodyY(yNode, t.v, b.v, 6, bullish); };
    std::map<int, const sab::Rec*> sharedAt;                                // chart bar -> its shared A (the first side read wins the candle)
    for (std::map<std::pair<int, long long>, sab::Rec>::const_iterator it = book.sab.st.byKey.begin(); it != book.sab.st.byKey.end(); ++it) {
        const int at = barOf(static_cast<RTDATE>(it->second.raw));
        if (at < 0 || at >= s.barCount - 1) continue;                     // off the chart / the forming candle never
        if (!defClose(at, it->second.side == sab::LOW)) { ++s.sabColourVeto; continue; }   // belt and braces: the chart's own candle must agree
        if (!sharedAt.count(at)) sharedAt[at] = &it->second;
    }
    s.sabGrayFromNode = 0; std::set<long long> demoted;
    {   std::map<int, const Latch*> best; std::vector<const Latch*> keep;
        for (const Latch* L : confirmed) {
            if (L->conf == 'W') { s.bubbleState[L->id] = "replaced: wick A retired (2.22.0 shared A)"; continue; }
            const int at = barOf(L->peakTime);
            if (at < 0) { keep.push_back(L); continue; }                    // offscreen: the loop below says so
            if (sharedAt.count(at)) { s.bubbleState[L->id] = (sharedAt[at]->side == sab::LOW) == L->support ? "shared" : "merged: shared A (other side) on this candle"; continue; }   // same side: its label / A|Tr ride on the shared A's circle
            if (!defClose(L->conf == 'N' ? at + 1 : at, L->support)) { s.bubbleState[L->id] = "gray: attackers' close"; demoted.insert(L->id); ++s.sabGrayFromNode; continue; }   // (2.23.1, Rassul 2026-10-25) a NEXT-candle A is coloured when the next candle (its confirmation) closed in the defenders' colour
            std::map<int, const Latch*>::iterator b = best.find(at);
            if (b == best.end()) { best[at] = L; continue; }
            const double ab = std::fabs(b->second->net), al = std::fabs(L->net);
            if (al > ab + 1e-9 || (std::fabs(al - ab) <= 1e-9 && L->id < b->second->id)) { s.bubbleState[b->second->id] = "merged: one circle per candle"; b->second = L; }
            else s.bubbleState[L->id] = "merged: one circle per candle";
        }
        for (std::map<int, const Latch*>::const_iterator b = best.begin(); b != best.end(); ++b) keep.push_back(b->second);
        confirmed.swap(keep);
    }
    std::sort(confirmed.begin(),confirmed.end(),[](const Latch* a,const Latch* b){
        return a->tDecided!=b->tDecided?a->tDecided<b->tDecided:a->id<b->id;
    });
    {   // (2.9.1, SignalBoard) every displayable A for the other indicators on this chart: bar, price, side, strength
        std::vector<sb::Ev> ev;
        // (2.17.0) flag 1 = final (pivot confirmed, D line written), 2 = tentative (may still vanish) - SignalBoard.h 1.1.0
        for (const Latch* item : confirmed) { if (!confirmedAbsorption(*item)) continue;   // (2.23.0) a late (old-path) A drawn by the checkbox is never posted
            sb::Ev e; e.bar = localSec(item->peakTime); e.px = item->price; e.side = item->support ? 1 : -1; e.val = item->mult; e.flag = dp::boardFlag(item->tent); if (e.bar > 0) ev.push_back(e); }
        for (std::map<int, const sab::Rec*>::const_iterator it = sharedAt.begin(); it != sharedAt.end(); ++it) {   // (2.22.0) the shared A's (final)
            sb::Ev e; e.bar = it->second->key; e.px = it->second->extPx; e.side = it->second->side == sab::LOW ? 1 : -1; e.val = it->second->effort; e.flag = sb::A_FINAL; ev.push_back(e); }
        sb::publish('A', s.colSym, s.colSpb, s.chartKey, (long long)steadyMs(), ev);
    }
    {   // (2.28.0) the delta NODES (top 3 bars of the 90-min profile) as SignalBoard 1.4.0 facts, kind 'N' (live: the profile moves with price)
        s.nodeFacts.clear();
        const std::vector<dp::Tick> t3 = dp::labelKeys(profile.buckets);
        const long long lastT = bubbleTimes.count >= s.barCount && s.barCount > 0 ? localSec(static_cast<RTDATE>(bubbleTimes[s.barCount - 1])) : 0;
        int rk = 0;
        for (dp::Tick k3 : t3) for (const dp::Bucket& b3 : profile.buckets) if (b3.key == k3) {
            sb::Fact f; f.bar = lastT; f.lo = (double)b3.low * profile.tick; f.hi = (double)b3.high * profile.tick; f.side = b3.delta > 0 ? 1 : -1;
            f.val = b3.ratio; f.code = ++rk; f.state = 0; f.proven = 0; s.nodeFacts.push_back(f); }
    }
    s.sabDrawn = s.sabOffscreen = 0;
    for (std::map<int, const sab::Rec*>::const_iterator it = sharedAt.begin(); it != sharedAt.end(); ++it) {   // (2.22.0) the shared A wins its candle
        const int at = it->first; const sab::Rec& R = *it->second; const bool bull = R.side == sab::LOW;
        PNT point; point.set(at, static_cast<float>(R.extPx), kBarCenter);
        const int dy = bull ? 9 : -9;                                       // just beyond the extreme: below a low, above a high
        const LBox anchorBox{point.h-7,point.v+dy-7,point.h+8,point.v+dy+8};
        if (anchorBox.l<candleLeft || anchorBox.r>priceLimit || anchorBox.t<top || anchorBox.b>bottom) { ++s.sabOffscreen; continue; }
        Bubble g{anchorBox,at,R.extPx,bull,-1 - static_cast<long long>(at),0}; g.shared = true; bubbles.push_back(g);
    }
    for (const Latch* item : confirmed) {
        const Latch& L=*item;
        ++s.absorptionEvents;
        int lo=0, hi=s.barCount-1, at=-1;
        if (bubbleTimes.count >= s.barCount && L.peakTime != 0) {
            while (lo<=hi) { const int mid=(lo+hi)/2; const RTDATE t=static_cast<RTDATE>(bubbleTimes[mid]);
                if(t==L.peakTime){at=mid;break;} if(t<L.peakTime)lo=mid+1;else hi=mid-1; }
        }
        if(at<0){s.bubbleState[L.id]="offscreen";++s.bubblesOffscreen;continue;}
        PNT point; point.set(at,static_cast<float>(L.price),kBarCenter);
        const int cyA = bodyY(at, point.v, L.support);   // (2.26.0) wholly below the body (bullish) / above it (bearish)
        const LBox anchorBox{point.h-6,cyA-6,point.h+7,cyA+7};
        if(anchorBox.l<candleLeft || anchorBox.r>priceLimit || anchorBox.t<top || anchorBox.b>bottom){
            s.bubbleState[L.id]="offscreen";++s.bubblesOffscreen;continue;
        }
        const Bubble* shared=NULL;
        for(const Bubble& b:bubbles)if(b.support==L.support&&b.bar==at&&std::fabs(b.price-L.price)<=s.tick*.000001){shared=&b;break;}
        if(shared){s.bubbleState[L.id]="shared";continue;}
        bool found=false;LBox selected=anchorBox;int offset=0;
        for(int attempt=0;attempt<13&&!found;++attempt){
            const int dx=((attempt+1)/2)*14*(attempt%2?-1:1);
            LBox box{anchorBox.l+dx,anchorBox.t,anchorBox.r+dx,anchorBox.b};
            if(box.l<candleLeft||box.r>priceLimit)continue;
            bool clash=false;for(const Bubble& b:bubbles)if(overlap(box,b.box)){clash=true;break;}
            if(!clash){selected=box;offset=dx;found=true;}
        }
        if(!found){s.bubbleState[L.id]="suppressed";++s.bubblesSuppressed;continue;}
        bubbles.push_back(Bubble{selected,at,L.price,L.support,L.id,offset});s.bubbleState[L.id]="drawn";
    }
    // (2.19.0) the GRAY circles (dp::grayPick): every big node that is not a coloured A - forming, waiting for the next candle, pivot
    // not holding, or no winner - one per candle, after the coloured ones (colour wins its candle and its place). Display only: no
    // record, no SignalBoard event.
    s.grayDrawn = s.grayOffscreen = 0;
    s.liveDrawn = 0;
    for (const LatchBook::LiveA& a : book.live) {   // (2.27.0) the LIVE A: coloured while the rules hold on the forming / waiting candle
        if (a.peak == 0 || bubbleTimes.count < s.barCount) continue;
        int lo=0, hi=s.barCount-1, at=-1;
        while (lo<=hi) { const int mid=(lo+hi)/2; const RTDATE t=static_cast<RTDATE>(bubbleTimes[mid]); if(t==a.peak){at=mid;break;} if(t<a.peak)lo=mid+1;else hi=mid-1; }
        if (at < 0) continue;
        bool taken = false; for (const Bubble& b : bubbles) if (b.bar == at) taken = true;
        if (taken) continue;
        PNT point; point.set(at,static_cast<float>(a.price),kBarCenter);
        const int cy = bodyY(at, point.v, a.sell);
        const LBox bx{point.h-6,cy-6,point.h+7,cy+7};
        if (bx.l<candleLeft || bx.r>priceLimit || bx.t<top || bx.b>bottom) continue;
        Bubble g{bx,at,a.price,a.sell,-1000000 - static_cast<long long>(at),0}; bubbles.push_back(g); ++s.liveDrawn;
    }
    {
        std::vector<dp::GrayCand> gc;
        for (const Latch& L : book.v) {
            dp::GrayCand g; g.id = L.id; g.candle = static_cast<long long>(L.peakTime); g.mult = L.mult;
            g.coloured = drawableAbsorption(L) && s.bubbleState.count(L.id) && s.bubbleState[L.id] == "drawn";
            g.open = L.code == "A?" || L.code == "I?" || (oldPathA(L) && !g.coloured) || demoted.count(L.id) != 0;   // (2.23.0) an old-path (late) A stays the gray circle it was   // (2.22.0) a node A on an attacker-colour candle = gray
            gc.push_back(g);
        }
        const std::vector<long long> pick = dp::grayPick(gc);
        for (long long gid : pick) {
            const Latch* lp = NULL; for (const Latch& L : book.v) if (L.id == gid) { lp = &L; break; }
            if (!lp || lp->peakTime == 0 || bubbleTimes.count < s.barCount) continue;
            int lo=0, hi=s.barCount-1, at=-1;
            while (lo<=hi) { const int mid=(lo+hi)/2; const RTDATE t=static_cast<RTDATE>(bubbleTimes[mid]); if(t==lp->peakTime){at=mid;break;} if(t<lp->peakTime)lo=mid+1;else hi=mid-1; }
            if (at < 0) { ++s.grayOffscreen; continue; }
            bool hasColour = false; for (const Bubble& b : bubbles) if (!b.gray && b.bar == at) hasColour = true;
            if (hasColour) continue;
            PNT point; point.set(at,static_cast<float>(lp->price),kBarCenter);
            const int cyG = bodyY(at, point.v, lp->support);   // (2.26.0) the gray circle too
            const LBox anchorBox{point.h-6,cyG-6,point.h+7,cyG+7};
            if (anchorBox.l<candleLeft || anchorBox.r>priceLimit || anchorBox.t<top || anchorBox.b>bottom) { ++s.grayOffscreen; continue; }
            bool found=false; LBox selected=anchorBox; int offset=0;
            for (int attempt=0; attempt<13 && !found; ++attempt) {
                const int dx=((attempt+1)/2)*14*(attempt%2?-1:1);
                LBox box{anchorBox.l+dx,anchorBox.t,anchorBox.r+dx,anchorBox.b};
                if (box.l<candleLeft||box.r>priceLimit) continue;
                bool clash=false; for (const Bubble& b:bubbles) if (overlap(box,b.box)) { clash=true; break; }
                if (!clash) { selected=box; offset=dx; found=true; }
            }
            if (!found) continue;
            Bubble g{selected,at,lp->price,lp->support,lp->id,offset}; g.gray = true; bubbles.push_back(g);
            if (!s.bubbleState.count(lp->id)) s.bubbleState[lp->id] = "gray";
        }
    }
    auto bubbleDisposition = [&](long long id) -> std::string {
        const auto it=s.bubbleState.find(id);return it==s.bubbleState.end()?"missing":it->second;
    };
    // A label only for an A whose circle is on screen. (2.7.1, 2026-10-13) an offscreen peak (scrolled back / off the pane) is NOT
    // labelled any more - its "[offscreen]" label landed on circles and swing labels; the event stays in the record and the status.
    auto permitAbsorptionLabel = [&](long long id) -> bool {
        const std::string q=bubbleDisposition(id);
        if(q=="drawn"||q=="shared")return true;
        s.profileState[id]=q=="offscreen"?"suppressed:offscreen":q=="suppressed"||q=="missing"?"suppressed:no bubble":"suppressed:"+q;return false;   // (2.8.5) merged / hidden: no label either
    };
    std::vector<std::size_t> ranked;
    for (std::size_t i = 0; i < profile.buckets.size(); ++i) ranked.push_back(i);
    std::sort(ranked.begin(), ranked.end(), [&profile](std::size_t a, std::size_t b) {
        const double x = std::fabs(profile.buckets[a].delta), y = std::fabs(profile.buckets[b].delta);
        return x != y ? x > y : profile.buckets[a].key < profile.buckets[b].key;
    });
    std::map<dp::Tick, int> edge, midY;
    struct RowText { double delta; int y, edge, h; const dp::Node* node; dp::Tick key; };   // (2.8.1) + the node   // (2.8.0) each drawn row's value, placed after the node labels
    std::vector<RowText> rowTexts;
    struct VisibleNode { const dp::Node* node; int y, edge; };
    std::vector<VisibleNode> nodes;
    // (2.11.0, Rassul 19:55 "build ... 3") TWO-SIDED rows: one of the 3 biggest VOLUME rows whose net delta is under 15% of its volume -
    // heavy buying AND selling at that price (a battle) that the delta bar alone shows as almost nothing; marked "\xB1" at the row
    std::set<dp::Tick> twoSided;
    {   std::vector<std::size_t> byVol; for (std::size_t i = 0; i < profile.buckets.size(); ++i) if (profile.buckets[i].volume > 0) byVol.push_back(i);
        std::sort(byVol.begin(), byVol.end(), [&profile](std::size_t a, std::size_t b) {
            return profile.buckets[a].volume != profile.buckets[b].volume ? profile.buckets[a].volume > profile.buckets[b].volume : profile.buckets[a].key < profile.buckets[b].key; });
        for (std::size_t k = 0; k < byVol.size() && k < 3; ++k) { const dp::Bucket& q = profile.buckets[byVol[k]];
            if (dp::twoSided(q.delta, q.volume)) twoSided.insert(q.key); } }
    struct TwoMark { int y, edge; };
    std::vector<TwoMark> twoMarks;
    s.drawnRows = 0;
    for (std::size_t i = 0; i < profile.buckets.size(); ++i) {
        const dp::Bucket& b = profile.buckets[i];
        const double lower = (static_cast<double>(b.key) * static_cast<double>(profile.ticksPerRow) - 0.5) * profile.tick;
        const double upper = (static_cast<double>(b.key + 1) * static_cast<double>(profile.ticksPerRow) - 0.5) * profile.tick;
        int yt = yOf(lastBar, upper), yb = yOf(lastBar, lower);
        if (yb <= yt) yb = yt + 1;
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb);
        ++s.drawnRows;
        const int dl = profile.maxAbsDelta > 0 ? std::min(full, std::max(b.delta != 0 ? 1 : 0,
            static_cast<int>(std::fabs(b.delta) / profile.maxAbsDelta * full))) : 0;
        const int vl = profile.maxVolume > 0 ? std::min(full, static_cast<int>(b.volume / profile.maxVolume * full)) : 0;
        box(base - vl, yt, base, yb, C_VOL);
        if (b.delta != 0) box(base - dl, yt, base, yb, b.delta > 0 ? C_BUY : C_SELL);
        int e = base - dl;
        const dp::Node* node = NULL;
        for (std::size_t j = 0; j < profile.nodes.size(); ++j) if (profile.nodes[j].bucket == b.key) node = &profile.nodes[j];
        // (2.5.8, Rassul 12:50 "I like the [labels] on the hourly profile ... fix for the delta profile on the 3 min charts") the
        // amount is no longer a separate text per row (on 3-min rows 1-2 px tall they piled up): it rides in the node's one label below
        const bool showAmount = false;
        (void)ranked;
        if (showAmount && b.delta != 0) {
            const std::string label = amount(b.delta);
            const int width = textWidth(label, s.layout.font - 1, false);
            if (e - 2 - width >= pane.left + 2) {
                textRight(e - 2, (yt + yb) / 2, label, C_INK, s.layout.font - 1, false);
                e -= width + 2;
            }
        }
        edge[b.key] = e; midY[b.key] = (yt + yb) / 2;
        // (2.14.0, Rassul 2026-10-16 18:41 "why is there a +-") the two-sided "\xB1" mark is no longer drawn (the rows stay in the status counts as 0)
        // (2.8.1, Rassul 18:30 "it has values for every node instead of the nodes that matter") the delta text only on node rows
        if (s.layout.text && b.delta != 0) rowTexts.push_back(RowText{ b.delta, (yt + yb) / 2, e, yb - yt, node, b.key });   // (2.10.0) every row is a candidate
        if (node) { VisibleNode v; v.node = node; v.y = (yt + yb) / 2; v.edge = e; nodes.push_back(v); }
    }
    // Native key/bucket lookup only. Pixel Y NEVER assigns a node to a bucket.
    // (2.5.8) the hourly profile's labels: the 3 strongest nodes only, each ONE label "I 4.1x  -861" (letter, strength, net delta)
    // at its row; a label that would touch another steps up / down with a tick to its row, else it is not drawn (never overlapping)
    std::sort(nodes.begin(), nodes.end(), [](const VisibleNode& a, const VisibleNode& b) {
        return a.node->ratio != b.node->ratio ? a.node->ratio > b.node->ratio : a.y < b.y; });
    // (2.8.1, Rassul 18:31 "remove the I ... only focused on identifying absorption and consistent with the bubbles") only A nodes
    // are labelled (and get their delta text); I / I? / A? nodes keep their bar only
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const VisibleNode& v) { return v.node->state.code != "A"; }), nodes.end());
    // (2.12.0, Rassul 2026-10-16 08:11-08:25 "once an A or I paints it never repaints", "A and I both", "stays until it leaves the 90 min")
    // every DECIDED A (its circle drawn) and every decided I of the record is labelled "A 2.3x  -88" / "I 3.9x  -274" at its price row, from the
    // record's own frozen ratio / net / price - never from the live top-3 node, so a later candle can neither move nor remove it. It leaves
    // only when its candle is older than the 90-min window. One label per row and letter (the earliest event). Zone A's keep their own pass.
    std::vector<dp::Node> keepNodes; keepNodes.reserve(book.v.size() + 1);
    nodes.clear();
    {   std::set<long long> have;
        for (std::size_t i2 = 0; i2 < profile.zones.size(); ++i2) if (profile.zones[i2].code == "A") have.insert(profile.zones[i2].eventId);
        const long long endSec = bubbleTimes.count >= s.barCount ? localSec(static_cast<RTDATE>(bubbleTimes[s.barCount - 1])) : 0;
        std::set<std::pair<dp::Tick, char> > seen;
        for (std::size_t c = 0; c < book.v.size(); ++c) {                    // the record is in event-id order
            const Latch& L = book.v[c];
            const bool isAev = drawableAbsorption(L), isIev = false;   // (2.13.0) absorption only: an old I record is never labelled
            if (!isAev && !isIev) continue;
            if (have.count(L.id)) continue;
            if (endSec > 0 && L.peakT > 0 && L.peakT <= endSec - 90 * 60) continue;   // older than the window
            if (isAev) { const std::string q = bubbleDisposition(L.id); if (q != "drawn" && q != "shared") continue; }
            dp::Tick pk; try { pk = dp::priceKey(L.price, profile.tick); } catch (...) { continue; }
            const dp::Bucket* hit = NULL;
            for (std::size_t j = 0; j < profile.buckets.size(); ++j) if (pk >= profile.buckets[j].low && pk <= profile.buckets[j].high) { hit = &profile.buckets[j]; break; }
            if (!hit) continue;
            if (!seen.insert(std::make_pair(hit->key, isAev ? 'A' : 'I')).second) continue;
            const std::map<dp::Tick, int>::const_iterator ee = edge.find(hit->key), my = midY.find(hit->key);
            if (ee == edge.end() || my == midY.end()) continue;
            dp::Node nn; nn.bucket = hit->key; nn.price = L.price; nn.ratio = L.mult; nn.state.code = isAev ? "A" : "I"; nn.state.support = L.support; nn.state.eventId = L.id;
            keepNodes.push_back(nn);
            VisibleNode v; v.node = &keepNodes.back(); v.y = my->second; v.edge = ee->second; nodes.push_back(v);
        }
    }
    std::set<dp::Tick> labFull;   // rows whose label already carries the value
    std::vector<int> used;
    std::vector<const dp::Node*> labelled;   // (2.8.4) the A nodes whose label was actually drawn
    std::vector<LBox> placed;
    s.twoSidedDrawn = 0;
    for (const TwoMark& m : twoMarks) {                                  // (2.11.0) drawn first: labels and values keep off it
        const int w = textWidth("\xB1", s.layout.font + 1, true), h = s.layout.font + 3;
        if (m.edge - 2 - w < pane.left + 2) continue;
        textRight(m.edge - 2, m.y, "\xB1", C_INK, s.layout.font + 1, true);
        placed.push_back(LBox{ m.edge - 3 - w, m.y - h / 2, m.edge - 1, m.y + h / 2 }); ++s.twoSidedDrawn;
    }
    // (2.14.0, Rassul 2026-10-16 18:38-18:41 "just labeling the nodes that have 2x or more ... just the ratio ... if it has absorption then put an A ...
    // if the ratio drops below 2, it loses the A and loses the ratio but its absorption circle stays without being repainted") every row at 2.0x or
    // more of the average |delta| (90-min profile, the LIVE ratio) is labelled with its ratio only - "2.2x", or "A 2.2x" when that row holds a decided
    // absorption (the circle's record). Below 2.0x the label (and the A) is gone; the circle on the candle is drawn from the record and never changes.
    // No dollar / lot values, no "\xB1" marks. (2.18.0, Rassul 2026-10-18 12:49 CT) replaced the 2.0x gate: ONLY the 3 biggest bars by abs delta are labelled (dp::labelKeys), at any ratio.
    {   struct LabRow { dp::Tick key; int y, edge; double ratio; const Latch* ev; };
        std::map<dp::Tick, const dp::Node*> aNode;
        for (std::size_t i = 0; i < nodes.size(); ++i) if (nodes[i].node->state.code == "A") { aNode[nodes[i].node->bucket] = nodes[i].node; ++s.activeAbsorptions; }
        std::vector<LabRow> rows;
        const std::vector<dp::Tick> top3 = dp::labelKeys(profile.buckets);   // (2.18.0, Rassul 12:49 CT) only the 3 biggest bars get a label
        for (std::size_t j = 0; j < profile.buckets.size(); ++j) {
            const dp::Bucket& b = profile.buckets[j];
            const std::map<dp::Tick, int>::const_iterator ee = edge.find(b.key), my = midY.find(b.key);
            if (ee == edge.end() || my == midY.end()) continue;
            const std::map<dp::Tick, const dp::Node*>::const_iterator an = aNode.find(b.key);
            const Latch* ev = NULL;
            if (an != aNode.end()) { ev = eventById(book, an->second->state.eventId); if (ev && !(drawableAbsorption(*ev) && permitAbsorptionLabel(ev->id))) ev = NULL; }
            if (std::find(top3.begin(), top3.end(), b.key) == top3.end()) { if (an != aNode.end()) s.profileState[an->second->state.eventId] = "label gone: not one of the 3 biggest bars"; continue; }   // (2.18.0) was: ratio under 2.0x
            rows.push_back(LabRow{ b.key, my->second, ee->second, b.ratio, ev });
        }
        std::stable_sort(rows.begin(), rows.end(), [](const LabRow& p, const LabRow& q) { return p.ratio != q.ratio ? p.ratio > q.ratio : p.y < q.y; });
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const LabRow& v = rows[i];
            std::string label = ratio(v.ratio);   // (2.28.0) the ratio only - the A is drawn in the READ strip
            int width = textWidth(label, s.layout.font, true);
            if (v.edge - 4 - width < left && v.ev) { label = ratio(v.ratio); width = textWidth(label, s.layout.font, true); ++s.labelsShort; }
            if (v.edge - 4 - width < left) continue;
            const int h = s.layout.font + 3, r = v.edge - 4, l = r - width;
            int y = v.y; bool ok = false;
            for (int k = 0; k <= 6 && !ok; k++) {
                const int yy = v.y + ((k + 1) / 2) * h * (k % 2 ? -1 : 1);
                if (yy - h / 2 <= top || yy + h / 2 >= bottom) continue;
                bool clash = false;
                for (const LBox& q : placed) if (l < q.r + 2 && q.l < r + 2 && yy - h / 2 < q.b && q.t < yy + h / 2) clash = true;
                for (const Bubble& b : bubbles) if (overlap(LBox{l,yy-h/2,r,yy+h/2},b.box)) clash=true;
                if (!clash) { y = yy; ok = true; }
            }
            if (!ok) continue;
            const COLOR color = v.ev ? (v.ev->support ? C_SUPPORT : C_RESISTANCE) : C_INK;
            textRight(r, y, label, color, s.layout.font, true); used.push_back(y);
            placed.push_back(LBox{ l, y - h / 2, r, y + h / 2 });
            if (y != v.y) stroke(v.edge - 2, v.y, v.edge - 2, y, color, 1);
            if (v.ev) { s.profileState[v.ev->id] = "shown:A"; if (bubbleDisposition(v.ev->id) == "drawn" || bubbleDisposition(v.ev->id) == "shared") ++s.pairedAbsorptions; }
        }
        // (2.28.0) THE READ STRIP: every decided A whose row is in the 90-min profile gets its code "A" at its row, in the winner's colour
        // (bullish green / bearish red) - not only the 3 labelled bars; at most 2 codes per row, never overlapping (steps up / down, else
        // not drawn and counted); the facts go to the SignalBoard as kind 'R' (decided) for the combiner and the other indicators
        s.readDrawn = 0; s.readClash = 0; s.readFacts.clear();
        std::vector<int> readUsed;
        for (std::map<dp::Tick, const dp::Node*>::const_iterator an = aNode.begin(); an != aNode.end(); ++an) {
            const Latch* ev = eventById(book, an->second->state.eventId);
            if (!ev || !drawableAbsorption(*ev) || !permitAbsorptionLabel(ev->id)) continue;
            const std::map<dp::Tick, int>::const_iterator my = midY.find(an->first);
            if (my == midY.end()) continue;
            const int h = s.layout.font + 2; int y = my->second; bool ok = false;
            for (int k = 0; k <= 4 && !ok; ++k) {
                const int yy = my->second + ((k + 1) / 2) * h * (k % 2 ? -1 : 1);
                if (yy - h / 2 <= top || yy + h / 2 >= bottom) continue;
                bool clash = false; for (int u : readUsed) if (std::abs(u - yy) < h) clash = true;
                if (!clash) { y = yy; ok = true; }
            }
            if (!ok) { ++s.readClash; continue; }
            readUsed.push_back(y);
            const COLOR rc = ev->support ? C_SUPPORT : C_RESISTANCE;
            textRight(readR - 3, y, "A", rc, s.layout.font + 1, true);
            if (y != my->second) stroke(readR - 1, my->second, readR + 1, my->second, rc, 1);
            ++s.readDrawn;
            sb::Fact f; f.bar = localSec(ev->peakTime); f.lo = an->second->price; f.hi = an->second->price; f.side = ev->support ? 1 : -1; f.val = ev->mult; f.code = 1; f.state = 1; f.proven = 1;
            s.readFacts.push_back(f);
        }
    }
    // Zones supplement, rather than suppress, node letters. Avoid both overlap
    // and invisible bucket defaults by considering ONLY rendered bucket edges.
    for (std::size_t i = 0; i < profile.zones.size(); ++i) {
        const dp::Zone& z = profile.zones[i];
        // (2.5.6) zone circles also come only from the session record (frozen)
        if (z.code != "A") continue;
        ++s.activeAbsorptions;
        const Latch* event=eventById(book,z.eventId);
        if(!event||!drawableAbsorption(*event)||!permitAbsorptionLabel(event->id))continue;
        int yt = yOf(lastBar, (static_cast<double>(z.high) + 0.5) * profile.tick);
        int yb = yOf(lastBar, (static_cast<double>(z.low) - 0.5) * profile.tick);
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb); if (yb <= yt) continue;
        int e = base; bool any = false;
        for (std::size_t j = 0; j < profile.buckets.size(); ++j) {
            const dp::Bucket& b = profile.buckets[j];
            if (b.high < z.low || b.low > z.high) continue;
            const std::map<dp::Tick, int>::const_iterator at = edge.find(b.key);
            if (at != edge.end()) { e = any ? std::min(e, at->second) : at->second; any = true; }
        }
        if (!any) continue;
        const std::string badge;   // (2.7.1) no "[offscreen]" labels
        std::string label=aeLabel(book,event->id)+" "+ratio(event->mult)+"  "+amountAbs(event->net)+badge;   // (2.7.0) A / AE? / AE
        if(e-7-textWidth(label,s.layout.font,true)<left)label=aeLabel(book,event->id)+" "+ratio(event->mult);   // (2.8.0) inside the column
        if(e-7-textWidth(label,s.layout.font,true)<left)label=aeLabel(book,event->id);
        const int width=textWidth(label,s.layout.font,true), height=s.layout.font+3;
        if(e-7-width<left)continue;
        const int anchorY=yOf(lastBar,event->price);
        int y=anchorY;bool ok=false;
        for(int k=0;k<=6&&!ok;++k){
            const int yy=anchorY+((k+1)/2)*height*(k%2?-1:1);
            if(yy-height/2<=top||yy+height/2>=bottom)continue;
            LBox box{e-7-width,yy-height/2,e-7,yy+height/2};bool clash=false;
            for(const LBox& q:placed)if(overlap(box,q))clash=true;
            for(const Bubble& q:bubbles)if(overlap(box,q.box))clash=true;
            if(!clash){y=yy;ok=true;}
        }
        if(!ok){s.profileState[event->id]="suppressed:label collision";continue;}
        const COLOR base=event->support?C_SUPPORT:C_RESISTANCE;
        const COLOR color=zoneFor(event->id)?base:getBlendedColor(base,0x00000000,55.0f);   // (2.9.0) mid-range = dimmed
        stroke(e-4,yt,e-4,yb,color,1);stroke(e-4,yt,e-1,yt,color,1);stroke(e-4,yb,e-1,yb,color,1);
        textRight(e-7,y,label,color,s.layout.font,true);used.push_back(y);
        placed.push_back(LBox{e-7-width,y-height/2,e-7,y+height/2});
        s.profileState[event->id]="shown:"+aeLabel(book,event->id);
        if(bubbleDisposition(event->id)=="drawn"||bubbleDisposition(event->id)=="shared")++s.pairedAbsorptions;
        if(y!=anchorY)stroke(e-5,anchorY,e-5,y,color,1);
    }
    // (2.21.0, Rassul approved delta-profile-trapped-mockup-v2.png) TRAPPED TRADERS on the profile: the trapped rows (beyond the old extreme)
    // outlined in amber, and a badge beside them - "Tr?" or "A|Tr?" when his drawn A is on the same candle and side - green = trapped sellers
    // (bullish), red = trapped buyers (bearish); from the record (decided once at the candle's close), kept while the candle is in the 90-min
    // window; never over another label or bubble (steps up / down with a tick to its rows, else not drawn and said so in the status)
    s.trapsDrawn = 0; s.trapsWithA = 0; s.trapNote.clear(); s.trapBoxes.clear();
    static const bool TRAP_ON_PROFILE = false;   // (2.27.1, Rassul 18:52 CT) record only: nothing drawn on the column
    if (!TRAP_ON_PROFILE && s.trapOn) s.trapNote = "2.27.1: record only (not drawn on the profile)";
    if (TRAP_ON_PROFILE && s.trapOn && !book.traps.empty()) {
        const long long endSec = bubbleTimes.count >= s.barCount ? localSec(static_cast<RTDATE>(bubbleTimes[s.barCount - 1])) : 0;
        int noRoom = 0, offProfile = 0;
        for (std::size_t ti = 0; ti < book.traps.size(); ++ti) {
            const trapl::Rec& t = book.traps[ti];
            if (endSec > 0 && t.barT <= endSec - 90 * 60) continue;                 // older than the window
            dp::Tick lo = t.lo, hi = t.hi;
            if (std::fabs(t.tick - profile.tick) > 1e-9 * std::max(t.tick, profile.tick)) {   // written on another tick size: re-key by price
                try { lo = dp::priceKey(static_cast<double>(t.lo) * t.tick, profile.tick); hi = dp::priceKey(static_cast<double>(t.hi) * t.tick, profile.tick); } catch (...) { continue; }
            }
            int minEdge = base; bool any = false;
            for (std::size_t j = 0; j < profile.buckets.size(); ++j) {
                const dp::Bucket& b = profile.buckets[j];
                if (b.high < lo || b.low > hi) continue;
                const std::map<dp::Tick, int>::const_iterator at = edge.find(b.key);
                if (at != edge.end()) { minEdge = any ? std::min(minEdge, at->second) : at->second; any = true; }
            }
            if (!any) { ++offProfile; continue; }
            int yt = yOf(lastBar, (static_cast<double>(hi) + 0.5) * profile.tick), yb = yOf(lastBar, (static_cast<double>(lo) - 0.5) * profile.tick);
            if (yb <= top || yt >= bottom) { ++offProfile; continue; }
            yt = std::max(top + 1, yt); yb = std::min(bottom - 1, yb); if (yb - yt < 3) { yt -= 2; yb += 2; }
            const int ol = std::max(left, minEdge - 2), orr = base + 1;
            bool withA = false;
            for (const Latch& L : book.v) if (drawableAbsorption(L) && L.support == !t.top && L.peakT == t.barT) { const std::string q = bubbleDisposition(L.id); if (q == "drawn" || q == "shared") withA = true; }
            const std::string label = trapl::badge(withA);
            const COLOR col = t.top ? C_SELL : C_BUY;
            // (2.21.1) the badge gets its OWN clear slot: >= 4 px from every other profile label / bubble; the text sits inside its box with
            // 4 px padding; tried in order: just ABOVE the outline (below it for trapped buyers at a high, i.e. away from price's extreme),
            // beside the outer trapped row, the other side of the outline, then stepping outward; a step smaller font when it does not fit
            const int GAP = 4, PAD = 4;
            int fnt = s.layout.font, w = 0, h = 0, br = ol - 5, bl = 0; bool fits = false;
            for (; fnt >= std::max(7, s.layout.font - 2) && !fits; --fnt) {
                w = textWidth(label, fnt, true); h = fnt + 2 * PAD; bl = br - w - 2 * PAD;
                if (bl >= left) { fits = true; break; }
            }
            if (!fits) { ++noRoom; continue; }
            const int rowY = t.top ? std::max(yt, std::min(yb, yOf(lastBar, (static_cast<double>(hi)) * profile.tick)))
                                   : std::max(yt, std::min(yb, yOf(lastBar, (static_cast<double>(lo)) * profile.tick)));   // the outer trapped row
            std::vector<int> tries;
            const int above = yt - GAP - h / 2, below = yb + GAP + h / 2;
            if (t.top) { tries.push_back(above); tries.push_back(rowY); tries.push_back(below); } else { tries.push_back(below); tries.push_back(rowY); tries.push_back(above); }
            for (int k = 1; k <= 8; ++k) { tries.push_back(above - k * (h + GAP)); tries.push_back(below + k * (h + GAP)); }
            int y = 0; bool ok = false;
            for (std::size_t k = 0; k < tries.size() && !ok; ++k) {
                const int yy = tries[k];
                if (yy - h / 2 <= top + 1 || yy + h / 2 >= bottom - 1) continue;
                const LBox bx{ bl - GAP, yy - h / 2 - GAP, br + GAP, yy + h / 2 + GAP }; bool clash = false;   // 4 px clear of every other label / bubble
                // textRight draws a label ~0.45 x its font ABOVE its logical y: compare with where the other labels really show (their box
                // raised by that much) as well as their logical box
                const int sh = static_cast<int>(s.layout.font * 0.45 + 0.5);
                for (const LBox& q : placed) if (overlap(bx, q) || overlap(bx, LBox{ q.l, q.t - sh, q.r, q.b - sh })) clash = true;
                for (const Bubble& q : bubbles) if (overlap(bx, q.box)) clash = true;
                if (!clash) { y = yy; ok = true; }
            }
            if (!ok) { ++noRoom; continue; }
            stroke(ol, yt, orr, yt, C_TRAP, 1); stroke(ol, yb, orr, yb, C_TRAP, 1); stroke(ol, yt, ol, yb, C_TRAP, 1); stroke(orr, yt, orr, yb, C_TRAP, 1);
            stroke(bl, y - h / 2, br, y - h / 2, col, 1); stroke(bl, y + h / 2, br, y + h / 2, col, 1); stroke(bl, y - h / 2, bl, y + h / 2, col, 1); stroke(br, y - h / 2, br, y + h / 2, col, 1);
            textRight(br - PAD, y + static_cast<int>(fnt * 0.45 + 0.5), label, col, fnt, true);   // (2.21.1) the text centred IN its box (textRight raises by 0.45 x font)
            stroke(br, y, ol, std::max(yt, std::min(yb, y)), col, 1);             // the short tick to the outlined rows
            placed.push_back(LBox{ bl, y - h / 2, br, y + h / 2 });
            s.trapBoxes.push_back(ChartState::LBox2{ bl, y - h / 2, br, y + h / 2 });
            ++s.trapsDrawn; if (withA) ++s.trapsWithA;
        }
        if (noRoom || offProfile) s.trapNote = std::to_string(noRoom) + " no room / " + std::to_string(offProfile) + " off the profile";
    }
    // (2.8.0, Rassul approved 11:00 CT) each row's delta next to its bar on the price side, "+137" green / "-162" red, inside the column;
    // biggest first, never over a node label or another value (a row with no room shows its bar only)
    s.textsDrawn = 0;
    if (false && s.layout.text) {   // (2.14.0, Rassul 18:40 "you dont need to show 954 or -934 ... just the ratio") no row values
        std::stable_sort(rowTexts.begin(), rowTexts.end(), [](const RowText& a, const RowText& b) { return std::fabs(a.delta) > std::fabs(b.delta); });
        const int h = s.layout.font + 3;
        int nBig = 0;                                                       // (2.10.1) the 3 biggest values in all, either side
        for (const RowText& t : rowTexts) {
            // (2.8.4) a node whose A label was DRAWN; (2.10.0, Rassul 2026-10-15 12:06 "(b)") plus the 3 biggest buying and the 3 biggest
            // selling rows on screen, so the profile always shows its largest values (biggest first, never over a label or another value)
            // (2.12.0, Rassul 08:25) the THREE LARGEST bars always show their value (biggest first); a row whose A / I label already
            // carries it is not written twice; rows beyond the third show a value only inside an A / I label
            if (nBig >= 3) continue;
            ++nBig;
            if (labFull.count(t.key)) continue;
            const std::string label = amount(t.delta);
            const int w = textWidth(label, s.layout.font, true), r = t.edge - 2, l = r - w;
            if (l < left || t.y - h / 2 <= top || t.y + h / 2 >= bottom) continue;
            const LBox bx{ l, t.y - h / 2, r, t.y + h / 2 };
            bool clash = false; for (const LBox& q : placed) if (overlap(bx, q)) { clash = true; break; }
            if (clash) continue;
            textRight(r, t.y, label, t.delta > 0 ? C_BUY : C_SELL, s.layout.font, true);
            placed.push_back(bx); ++s.textsDrawn;
        }
    }
    // (2.7.0, Rassul 2026-10-13 "this looks good, lets implement this") the printed Ex? / Ex marks: text only (red above a top,
    // green below a bottom), on the extreme bar's frozen stamp at its frozen price; never over an A circle, a profile label or
    // another mark - stepped away from the bar (up for a top, down for a bottom), else not drawn this repaint (counted).
    s.exDrawn = s.exSuppressed = s.exOffscreen = 0; s.exState.clear();
    {
        std::vector<LBox> exPlaced;
        std::vector<const ExMark*> marks;
        const bool drawEx = false;   // (2.8.1, Rassul 18:33 "keep it simple, implement A and its bubble") Ex? marks are recorded, not drawn
        for (const ExMark& m : book.ex) if (drawEx && m.status == 'P' && m.mark != "none") marks.push_back(&m);
        std::sort(marks.begin(), marks.end(), [](const ExMark* a, const ExMark* b) { return a->id < b->id; });
        for (const ExMark* item : marks) {
            const ExMark& m = *item;
            int lo = 0, hi = s.barCount - 1, at = -1;
            if (bubbleTimes.count >= s.barCount && m.extRaw != 0) {
                while (lo <= hi) {
                    const int mid = (lo + hi) / 2; const RTDATE t = static_cast<RTDATE>(bubbleTimes[mid]);
                    if (t == m.extRaw) { at = mid; break; }
                    if (t < m.extRaw) lo = mid + 1; else hi = mid - 1;
                }
            }
            if (at < 0) { s.exState[m.id] = "offscreen"; ++s.exOffscreen; continue; }
            PNT point; point.set(at, static_cast<float>(m.extreme), kBarCenter);
            const int font = s.layout.font, w = textWidth(m.mark, font, true), hh = font + 3;
            const int l = point.h - w / 2, r = l + w;
            if (l < candleLeft || r > priceLimit) { s.exState[m.id] = "offscreen"; ++s.exOffscreen; continue; }
            int y = 0; bool ok = false;
            for (int k = 0; k < 6 && !ok; ++k) {
                const int yy = m.top ? point.v - hh / 2 - 3 - k * hh : point.v + hh / 2 + 3 + k * hh;
                if (yy - hh / 2 <= top || yy + hh / 2 >= bottom) continue;
                const LBox bx{ l - 1, yy - hh / 2, r + 1, yy + hh / 2 };
                bool clash = false;
                for (const LBox& q : placed) if (overlap(bx, q)) clash = true;
                for (const Bubble& q : bubbles) if (overlap(bx, q.box)) clash = true;
                for (const LBox& q : exPlaced) if (overlap(bx, q)) clash = true;
                if (!clash) { y = yy; ok = true; exPlaced.push_back(bx); }
            }
            if (!ok) { s.exState[m.id] = "suppressed"; ++s.exSuppressed; continue; }
            textRight(r, y, m.mark, m.top ? C_SELL : C_BUY, font, true);
            s.exState[m.id] = "drawn"; ++s.exDrawn;
        }
    }
    // Historical confirmed absorption uses the SAME immutable events as the current profile.
    // I, A?, I?, Acc and Dst are never absorption bubbles. No one-bubble-per-bar filter.
    for(const Bubble& b:bubbles){
        const bool atZ=zn::atZone(Z,b.bar,b.price); if(atZ)++s.zoneFull;else ++s.zoneDim;
        // (2.10.3, Rassul 18:46 option 1) the circle on the candle is in the TRAPPED side's colour, like the absorption print circle
        // (Big Bid/Ask 1.6.0): bullish A (sellers absorbed at a low) = red, bearish A (buyers absorbed at a high) = green
        const COLOR trapped=b.gray?C_GRAY_RING:(b.support?C_SUPPORT:C_RESISTANCE);   // (2.19.1, Rassul 2026-10-19 17:39 "using bullish or bearish instead") the WINNER's colour: bullish A green, bearish A red; gray = big node, winner unknown / none (2.19.0). Was the trapped side's colour (2.10.3).
        const COLOR color=atZ?trapped:getBlendedColor(trapped,0x00000000,55.0f);   // (2.9.0) dimmed away from a zone
        if (b.shared) {                                                     // (2.22.0) the shared A: the letter A in its ring, winner's colour, nothing else
            setPen(color,2,P_SOLID);CBRUSH brush(color,PAT_HOLLOW);brush.set();
            RCT r;r.set(coordinate(b.box.l),coordinate(b.box.t),coordinate(b.box.r-1),coordinate(b.box.b-1));ko::oval(r,DRAW_OPAQUE);
            const int fpt = 8, cx = (b.box.l + b.box.r) / 2, cy = (b.box.t + b.box.b) / 2;
            textRight(cx + (textWidth("A", fpt, true) + 1) / 2, cy + (fpt * 45 + 50) / 100, "A", color, fpt, true);
            ++s.sabDrawn; continue;
        }
        if(b.offset==0){   // (2.27.0 fix) the ring at its box - the 2.26.0 place wholly beyond the body (ringAt drew it at the node's own price)
            setPen(color,2,P_SOLID);CBRUSH brush(color,PAT_HOLLOW);brush.set();
            RCT r;r.set(coordinate(b.box.l),coordinate(b.box.t),coordinate(b.box.r-1),coordinate(b.box.b-1));ko::oval(r,DRAW_OPAQUE);
        }
        else {
            PNT anchor;anchor.set(b.bar,static_cast<float>(b.price),kBarCenter);
            stroke(anchor.h,anchor.v,anchor.h+b.offset,anchor.v,color,1);
            setPen(color,2,P_SOLID);CBRUSH brush(color,PAT_HOLLOW);brush.set();
            RCT r;r.set(coordinate(b.box.l),coordinate(b.box.t),coordinate(b.box.r-1),coordinate(b.box.b-1));ko::oval(r,DRAW_OPAQUE);   // (2.21.2)
        }
        if (b.gray) ++s.grayDrawn; else ++s.bubblesDrawn;
    }
    combine(s, confirmed, book, bubbleTimes, candleLeft, priceLimit, top, bottom);   // (2.28.0) layer 3, after every layer-1 / layer-2 decision of this build
    {   // (2.28.0) post this chart's facts once a second (SignalBoard 1.4.0): READ codes, delta nodes, candle trap lots
        const long long nowMs = steadyMs();
        if (nowMs - s.factsPostedMs >= 1000) {
            sb::postFacts('R', s.colSym, s.colSpb, s.chartKey, nowMs, s.readFacts);
            sb::postFacts('N', s.colSym, s.colSpb, s.chartKey, nowMs, s.nodeFacts);
            sb::postFacts('C', s.colSym, s.colSpb, s.chartKey, nowMs, s.candleFacts);
            s.factsPostedMs = nowMs;
        }
    }
    return true;
}
// (2.28.0, 2026-10-28, Rassul 20:14-20:18 CT approved) THE COMBINER (CombinerLogic.h, layer 3 of ChainSpec.h): once per CLOSED candle that is a
// pivot-reversal candle, the odds of a turn and of a swing from the nightly weights (lsFlexLevels\LRA-Combiner.csv). Inputs are decided facts
// only: this chart's own A / nodes (READ, layer 2 / Delta, layer 1), key + gamma levels from lsDealerProfile ('V'), LMT F prints from
// lsLiquidityProfile ('P'). A read is decided once and never re-scored; a restart re-scores the same closed candles to the same numbers.
// Drawn: the turn odds ("72%") beyond the candle's extreme when the odds reach the SHOW threshold (0.72 = 73% turn on held-back days);
// every read is logged (lsFlexLevels\Combiner\<MKT>-<spb>-<session>.csv) and scored the next morning by the nightly part.
void DeltaProfile::combine(ChartState& s, const std::vector<const Latch*>& confirmed, const LatchBook& book, RTARRAYI& times, int candleLeft, int priceLimit, int top, int bottom) {
    const long long nowMs = steadyMs();
    if (nowMs - s.cmbLoadMs > 60000) { s.cmbParams = cmb::load(storageDirectory() + "LRA-Combiner.csv"); s.cmbLoadMs = nowMs; }
    const long cnt = getBarCount();
    RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume);
    if (cnt < 30 || times.count < cnt || op.count < cnt || hi.count < cnt || lo.count < cnt || cl.count < cnt || vo.count < cnt) return;
    int f0 = 450, f1 = 900;   // the market's RTH in Central minutes (the research windows)
    if (s.market == "ES" || s.market == "NQ") { f0 = 510; f1 = 900; } else if (s.market == "CL" || s.market == "NG") { f0 = 480; f1 = 810; }
    else if (s.market == "GC") { f0 = 440; f1 = 810; } else if (s.market == "HG") { f0 = 430; f1 = 780; } else if (s.market == "EU") { f0 = 440; f1 = 840; }
    const double tick = s.tick > 0 ? s.tick : 0.25;
    std::vector<sb::Fact> lv, pr;
    const bool haveV = sb::readFacts('V', s.colSym, s.colSpb, s.chartKey, nowMs, lv, false);
    sb::readFacts('P', s.colSym, s.colSpb, s.chartKey, nowMs, pr, true);
    const int k = (int)cnt - 2;   // the last CLOSED candle
    const std::string cdir = storageDirectory() + "Combiner";
    const long long sessK = localSessionKey(static_cast<RTDATE>(times[k]), 0);
    s.cmbPath = cdir + DIR_SEPARATOR_CHR + s.market + "-" + std::to_string(getSecondsPerBar()) + "-" + std::to_string(sessK) + ".csv";
    if (!s.cmbRestored) {   // a restart reads its own decided reads back: a closed candle is never scored twice, nothing is written twice
        s.cmbRestored = true; std::ifstream in(s.cmbPath.c_str()); std::string line;
        while (std::getline(in, line)) { const std::vector<std::string> f = cmb::split(line, ',');
            if (f.size() < 7 || f[0] != "K") continue; double bt = 0, sd = 0, t = 0, w = 0, sh = 0;
            if (!cmb::num(f[1], bt) || !cmb::num(f[2], sd) || !cmb::num(f[3], t) || !cmb::num(f[4], w) || !cmb::num(f[5], sh)) continue;
            cmb::Read r; r.pivot = true; r.bull = sd > 0; r.turn = t; r.swing = w; r.show = sh > 0; s.cmbReads[(long long)bt] = r; }
    }
    if (s.cmbFirstMs < 0) s.cmbFirstMs = nowMs;
    // load order: give the level producer (lsDealerProfile) 15 s after this chart starts before deciding without it, so which plugin loads
    // first does not change a decided read; after that a read without levels is decided and counted (status NO_LEVELS)
    if (!haveV && nowMs - s.cmbFirstMs < 15000) { ++s.cmbDeferred; return; }
    for (int kk = std::max(30, k - 2); kk <= k; ++kk) {   // a short catch-up after a restart / a missed build
        const long long bt = localSec(static_cast<RTDATE>(times[kk]));
        if (bt <= 0 || s.cmbReads.count(bt)) continue;
        std::vector<cmb::Candle> c;
        for (int i = kk - 25; i <= kk; ++i) { cmb::Candle q; q.o = op[i]; q.h = hi[i]; q.l = lo[i]; q.c = cl[i]; q.v = (double)vo[i];
            const long long ls = localSec(static_cast<RTDATE>(times[i])) - getSecondsPerBar(); q.mod = (int)(((ls % 86400) + 86400) % 86400 / 60); c.push_back(q); }   // the candle's START (IRT stamps the bar end)
        cmb::Read best; bool any = false;
        for (int side = 0; side < 2 && !any; ++side) {
            const bool bull = side == 0;
            if (!cmb::pivotShape(c, (int)c.size() - 1, bull)) continue;
            cmb::Inputs in;
            for (const Latch* L : confirmed) if (L->peakTime == static_cast<RTDATE>(times[kk]) && L->support == bull) {
                in.A = true; const double nodeHi = (double)L->high * tick, nodeLo = (double)L->low * tick;
                if (bull ? nodeHi < std::min(op[kk], cl[kk]) - tick / 2 : nodeLo > std::max(op[kk], cl[kk]) + tick / 2) in.wick = true; }
            for (const Latch& L : book.v) if (L.peakTime == static_cast<RTDATE>(times[kk]) && L.support == bull) in.node = true;
            const double ext = bull ? lo[kk] : hi[kk], at = cmb::atrAt(c, (int)c.size() - 1);
            for (const sb::Fact& f : lv) if (std::fabs(f.lo - ext) <= 0.5 * at) { if (f.code >= 1000) in.gamma = true; else in.key = true; }
            for (const sb::Fact& f : pr) if (f.bar == bt && f.side == (bull ? 1 : -1) && (f.code & 7) >= 1) in.bigfill = true;
            best = cmb::score(s.cmbParams, c, (int)c.size() - 1, bull, in, s.market, f0, f1, tick); any = best.pivot;
        }
        if (!any) { cmb::Read none; s.cmbReads[bt] = none; continue; }
        s.cmbReads[bt] = best; ++s.cmbScored; if (!haveV) ++s.cmbNoLevels;
        std::ostringstream o; o << "K," << bt << "," << (best.bull ? 1 : -1) << "," << best.turn << "," << best.swing << "," << (best.show ? 1 : 0) << "," << (haveV ? 1 : 0);
        for (int i = 0; i < cmb::NF; ++i) o << "," << best.f[i];
        s.cmbLines.push_back(o.str());
    }
    while (s.cmbReads.size() > 2000) s.cmbReads.erase(s.cmbReads.begin());
    if (!s.cmbLines.empty()) { makeDirectory(cdir); if (appendRecord(s.cmbPath, s.cmbLines)) s.cmbLines.clear(); }
    // the drawing + the board: the shown reads on screen; 'K' facts for the last 40 reads
    std::vector<sb::Fact> kf; s.cmbShown = 0;
    for (std::map<long long, cmb::Read>::const_reverse_iterator it = s.cmbReads.rbegin(); it != s.cmbReads.rend() && kf.size() < 40; ++it) {
        if (!it->second.pivot) continue;
        sb::Fact f; f.bar = it->first; f.side = it->second.bull ? 1 : -1; f.val = it->second.turn * 100; f.lo = it->second.swing * 100; f.hi = f.lo;
        f.code = it->second.show ? 1 : 0; f.state = 1; f.proven = s.cmbParams.showOn ? 1 : 2; kf.push_back(f);
    }
    sb::postFacts('K', s.colSym, s.colSpb, s.chartKey, nowMs, kf);
    for (int i = std::max(0, k - 400); i <= k; ++i) {
        const long long bt = localSec(static_cast<RTDATE>(times[i]));
        const std::map<long long, cmb::Read>::const_iterator it = s.cmbReads.find(bt);
        if (it == s.cmbReads.end() || !it->second.pivot || !it->second.show) continue;
        PNT a; a.set(i, static_cast<float>(it->second.bull ? lo[i] : hi[i]), kBarCenter);
        if (a.h < candleLeft || a.h > priceLimit) continue;
        const int y = it->second.bull ? a.v + 30 : a.v - 30;   // beyond the circle's row (the ring sits within ~22 px of the extreme)
        if (y < top + 6 || y > bottom - 6) continue;
        const std::string t = std::to_string((int)std::lround(it->second.turn * 100)) + "%";
        const int w = textWidth(t, 7, true);
        textRight(a.h + w / 2, y, t, it->second.bull ? C_SUPPORT : C_RESISTANCE, 7, true); ++s.cmbShown;
    }
}
// (2.17.0) DeltaProfile.status-<MKT>-<sec/bar>.txt (market code as the other plugins use it: ES NQ CL GC HG NG EU)
static std::string statusFileName(const std::string& market, int secondsPerBar) {
    std::string m; for (char c : market) { const unsigned char u = static_cast<unsigned char>(c); m += (std::isalnum(u) || c == '-' || c == '_') ? c : '_'; }
    if (m.empty()) m = "UNKNOWN";
    return "DeltaProfile.status-" + m.substr(0, 16) + "-" + std::to_string(secondsPerBar > 0 ? secondsPerBar : 0) + ".txt";
}
// (2.17.0, MARKET-STATE-SPEC.md SRC "DP") once per CLOSED bar: the top nodes as they stand at its close, the A on that bar (final /
// tentative, as it stood then), and the A events that turned final / failed since the previous posted bar. The facts of a bar are
// decided once and never changed; the same entry is re-posted (new stamp only) every 20 s so readers keep it fresh. Drawing unchanged.
// (2.25.0) SELF-AWARENESS: fed from the same closed bar the DP facts were posted for (s.mstBar), so it never looks ahead; display unchanged
static double dpNum(const std::map<std::string, std::string>& kv, const char* k) {
    auto it = kv.find(k); if (it == kv.end() || it->second.empty() || it->second == "?") return std::nan("");
    char* e = nullptr; const double v = std::strtod(it->second.c_str(), &e); return (e && *e == 0) ? v : std::nan("");
}
void DeltaProfile::selfAware(ChartState& s) {
    if (!s.mstPosted || s.mstBar <= 0 || s.colSym.empty() || s.market.empty()) return;
    if (!lsperf::smartAllowed(LSPERF_PLUGIN, s.colSym)) return;                     // PerfGuard: smart features OFF while slow (the basic indicator runs)
    const long count = getBarCount(); if (count < 2) return;
    RTARRAYI times(barDateTime); RTARRAY open(barOpen), high(barHigh), low(barLow), close(barClose);
    if (times.count < count || open.count < count || high.count < count || low.count < count || close.count < count) return;
    const int i = static_cast<int>(count) - 2;
    if (localSec(static_cast<RTDATE>(times[i])) != s.mstBar) return;                 // the facts are of another bar: wait
    s.self.ind = "DP";
    sa::Bar b; b.t = s.mstBar; b.o = open[i]; b.h = high[i]; b.l = low[i]; b.c = close[i];
    const bool fresh = b.t > s.self.lastFed;
    s.self.bar(s.colSym, s.colSpb, s.chartKey, s.market, static_cast<long long>(steadyMs()), b, dpNum(s.mstKv, "node_x"));
    if (!fresh) return;
    auto get = [&](const char* k) { auto it = s.mstKv.find(k); return it == s.mstKv.end() ? std::string("?") : it->second; };
    // an A decided on THIS closed bar: final on its own candle, or confirmed now on the next candle
    std::string side, how, px, x;
    if (get("a") == "final") { side = get("a_side"); how = "same"; px = get("a_px"); x = get("a_x"); }
    else if (get("a_conf_n") != "0" && get("a_conf_n") != "?") { side = get("a_conf_side"); how = "next"; px = get("a_conf_px"); x = "?"; }
    if (side != "bull" && side != "bear") return;
    const std::string why = "A " + how + " " + side + " x" + x + " @" + px + " node_x " + get("node_x") + " " + get("node_side");
    s.self.signal(s.market, s.colSpb, b.t, side == "bull" ? 1 : -1, std::string(), 50, dpNum(s.mstKv, "a_x"), why, b.c);
}
// (2.28.0, 2026-10-28 Rassul 19:37 CT "part of the market awareness system") the combiner's read of the closed candle in SRC DP for the
// Tape Reader snapshot / lra-tools market_state: cmb = none | pending | bull | bear, cmb_turn / cmb_swing = the odds, cmb_status = SHOW
// (proven, drawn) or "?" (LOG only - LRA-Combiner.csv STATUS), cmb_a = the READ A on that candle (1 / 0)
void DeltaProfile::cmbFacts(ChartState& s, long long bar) {
    std::map<long long, cmb::Read>::const_iterator it = s.cmbReads.find(bar);
    if (it == s.cmbReads.end()) { s.mstKv["cmb"] = "pending"; return; }
    if (!it->second.pivot) { s.mstKv["cmb"] = "none"; return; }
    s.mstKv["cmb"] = it->second.bull ? "bull" : "bear";
    s.mstKv["cmb_turn"] = mst::num(it->second.turn, 3); s.mstKv["cmb_swing"] = mst::num(it->second.swing, 3);
    s.mstKv["cmb_status"] = s.cmbParams.showOn ? "SHOW" : "?"; s.mstKv["cmb_a"] = it->second.f[0] > 0.5 ? "1" : "0";
}
void DeltaProfile::postMarketState(ChartState& s) {
    if (!s.ready || s.colSym.empty() || s.shortHistory || s.coarseBoundary) return;
    const long count = getBarCount();
    if (count < 2) return;
    RTARRAYI times(barDateTime);
    if (times.count < count) return;
    const RTDATE closedRaw = static_cast<RTDATE>(times[static_cast<int>(count) - 2]);
    if (closedRaw != s.latestStamp && static_cast<RTDATE>(times[static_cast<int>(count) - 1]) != s.latestStamp) return;   // snapshot not of this chart state
    const long long bar = localSec(closedRaw);
    const std::int64_t now = steadyMs();
    if (bar <= 0) return;
    if (bar == s.mstBar) {                                                          // same bar: re-post the same facts to stay fresh
        if (s.mstKv.count("cmb") == 0 || s.mstKv["cmb"] == "pending") { cmbFacts(s, bar); if (s.mstKv["cmb"] != "pending") s.mstAt = -1; }   // (2.28.0) the combiner decided after the first post
        if (s.mstAt >= 0 && now - s.mstAt < 20000) return;
        mst::publish("DP", s.colSym, s.colSpb, s.chartKey, now, s.mstBar, s.mstKv); s.mstAt = now; s.mstPosted = true; return;
    }
    const LatchBook& bk = latchBooks()[s.book + "|" + std::to_string(getSecondsPerBar())];
    std::vector<dp::FactNode> top;
    for (const dp::Node& n : s.snapshot.nodes) {
        dp::FactNode f; f.px = n.price; f.x = n.ratio;
        for (const dp::Bucket& b : s.snapshot.buckets) if (b.key == n.bucket) { f.buy = b.delta > 0; break; }
        top.push_back(f);
    }
    std::vector<dp::FactA> onBar, conf, fail;
    long long maxDec = s.mstDecT, maxFail = s.mstFailT;
    for (const Latch& L : bk.v) {
        dp::FactA a; a.px = L.price; a.x = L.mult; a.bull = L.support; a.bar = localSec(L.peakTime);
        const bool fin = confirmedAbsorption(L);
        if (L.peakTime == closedRaw && (fin || L.tent)) { a.fin = fin; onBar.push_back(a); }
        if (fin) { if (s.mstDecT >= 0 && L.tDecided > s.mstDecT) { a.fin = true; conf.push_back(a); } maxDec = std::max(maxDec, L.tDecided); }
        if (L.failLogged) { if (s.mstFailT >= 0 && L.failT > s.mstFailT) { a.bull = L.sell; fail.push_back(a); } maxFail = std::max(maxFail, L.failT); }
    }
    s.mstDecT = std::max<long long>(maxDec, 0); s.mstFailT = std::max<long long>(maxFail, 0);   // the first post reports no history
    s.mstKv = dp::dpFacts(top, onBar, conf, fail, s.tick);
    s.mstKv["version"] = DLT_VERSION;
    cmbFacts(s, bar);   // (2.28.0)
    mst::publish("DP", s.colSym, s.colSpb, s.chartKey, now, bar, s.mstKv);
    s.mstBar = bar; s.mstAt = now; s.mstPosted = true;
    s.mstNote = "bar " + std::to_string(bar) + " a " + s.mstKv["a"] + " node " + s.mstKv["node_px"] + " " + s.mstKv["node_x"] + "x";
}
void DeltaProfile::status(ChartState& s) {
    const std::int64_t now = steadyMs();
    if (s.statusAt >= 0 && now - s.statusAt < 1000) return;      // at most once a second; written only when it changed or 30 s passed
    const std::string directory = storageDirectory(); if (directory.empty()) return;
    std::ostringstream out;
    out << "VERSION," << DLT_VERSION << "\n" << s.keepNote << "ROOT," << s.root << "\nMARKET," << s.market
        << "\nCHART," << s.identity << "\nROWS," << (s.ready ? s.snapshot.buckets.size() : 0)
        << "\nASOF," << s.asof << "\nRANGE,Last 90 min LIVE (chart-bar close window)\nDRAWN_ROWS," << s.drawnRows
        << "\nBADGE,\nWIDTH," << s.layout.width << "\nSIDES,One\nFACE,Right\nPLACE,Right (READ strip on the price side)\nSTATE," << safeField(s.state)
        << "\nCOLUMN," << s.lay.delta.l << "-" << s.lay.delta.r << ",price right " << s.lay.priceRight << ",dealer " << (s.peers.on[cb::DEALER] ? "on" : "off")
        << ",levels " << (s.peers.on[cb::LEVELS] ? "on" : "off") << (s.lay.squeezed ? ",squeezed" : "") << (cb::columnsClean(s.lay) ? ",clean" : ",OVERLAP")
        << ",delta text " << (s.layout.text ? "on" : "off") << " drawn " << s.textsDrawn << ",labels shortened " << s.labelsShort
        << "\nMARGIN," << (s.marginOwner ? "owner (no Dealer Profile)" : "Dealer Profile owns it") << ",need " << s.needPx << " px,units "
        << (s.probe.units == cb::PIXELS ? "pixels" : "bars") << ",ppb " << s.ppb << ",read " << s.marginRead << ",asked " << s.marginAsked
        << ",set ok " << s.setOk << ",set fail " << s.setFail << ",gap " << s.gapSeen
        << "\nTICK," << std::setprecision(12) << s.tick << "\nMISSING_VAP_BARS," << s.missingVap
        << "\nVAP_MISMATCH," << s.vapMismatch << '\n'
        << "READ,strip 26 px,A drawn " << s.readDrawn << ",clash " << s.readClash << "\n"   // (2.28.0)
        << "FACTS,SignalBoard " << sb::VERSION << ",R " << s.readFacts.size() << ",N " << s.nodeFacts.size() << ",C " << s.candleFacts.size() << "\n"
        << "COMBINER," << cmb::VERSION << "," << safeField(s.cmbParams.why) << ",show turn >= " << s.cmbParams.showTurn << (s.cmbParams.showOn ? " on" : " off (log only)")
        << ",scored " << s.cmbScored << ",shown now " << s.cmbShown << ",no levels " << s.cmbNoLevels << ",deferred " << s.cmbDeferred << ",record " << safeField(s.cmbPath) << "\n";
    {   // (2.5.0) the session record: what was decided, where (for ChartView and the nightly scoring)
        const LatchBook& bk = latchBooks()[s.book + "|" + std::to_string(getSecondsPerBar())];
        const std::vector<Latch>& latches = bk.v;
        out << "RECORD," << safeField(bk.path) << ",session " << bk.session << ",loaded " << bk.loaded << ",written " << bk.written
            << ",pending " << bk.journal.size() << ",rejected " << bk.rejected << ',' << safeField(bk.note) << '\n';   // (2.5.4)
        {   std::size_t tentNow = 0, tentShown = 0, tentFailed = 0;   // (2.17.0) the tentative A's (SignalBoard flag 2) and their outcome
            for (const Latch& L : latches) { if (L.tent) ++tentNow; if (L.shownTent) ++tentShown; if (L.failLogged) ++tentFailed; }
            out << "TENTATIVE_A,showing now " << tentNow << ",ever shown " << tentShown << ",failed (TA|X) " << tentFailed << ",board flag 1 final / 2 tentative\n";
            std::size_t confSame = 0, confNext = 0, confWick = 0;   // (2.18.0) final A's by the candle whose close made them; (2.20.0) + wick
            for (const Latch& L : latches) if (confirmedAbsorption(L)) { if (L.conf == 'N') ++confNext; else if (L.conf == 'W') ++confWick; else ++confSame; }
            out << "A_CONF,same " << confSame << ",next " << confNext << ",wick " << confWick << ",final A by the candle that closed beyond the node (next = his 2.18.0 both-lows / both-highs pivot rule; wick = 2.20.0 order-book wick absorption)\n";
            out << "RESEARCHED_A,judged candles " << bk.raJudged << ",through " << bk.raThrough << ",A " << bk.raFound << ",old-path A logged (AL) " << bk.raOld << " of which late (>1 candle) " << bk.raLate
                << ",draw late " << (s.drawLate ? "on" : "off") << (bk.raNote.empty() ? std::string() : ",note " + safeField(bk.raNote)) << " (2.23.0: drawn A = decided at its candle or the next candle's close)\n";
            out << "WICK_A,retired 2.22.0 (replaced by the shared A; old conf=wick records kept, not drawn; settings rows hidden: " << (s.wickOn ? "on" : "off") << " x " << s.wickX << ")\n";
            out << bk.sab.statusLine() << ",decider " << (bk.sabBasHere ? "lsBidAskStats on this chart (this plugin reads)" : "this plugin (no attacker pane on this chart)") << '\n';
            out << "ONE_CIRCLE,shared A drawn " << s.sabDrawn << ",offscreen " << s.sabOffscreen << ",shared A vetoed by the chart candle colour " << s.sabColourVeto
                << ",node A shown gray (attackers' close) " << s.sabGrayFromNode << " (one circle per candle: shared A > biggest |delta| node A > one gray)\n";
            out << "TRAP," << (s.trapOn ? "on" : "off") << ",new 20-candle extremes " << bk.trapEval << ",trapped " << bk.trapFired << ",accepted breakouts " << bk.trapAccepted
                << ",under 1.0x " << bk.trapLight << ",learning " << bk.trapLearning << ",loaded " << bk.trapLoaded << ",bad lines " << bk.trapRejected
                << ",drawn " << s.trapsDrawn << " (A|Tr " << s.trapsWithA << "),not drawn " << safeField(s.trapNote) << ",last " << safeField(bk.trapLast) << '\n'; }
        out << "BUBBLE_POLICY,confirmed A only; historical peak annotations\nABSORPTION_EVENTS," << s.absorptionEvents
            << "\nTWO_SIDED_ROWS," << s.twoSidedDrawn << ",top-3 volume rows with net under 15% of volume (\xB1)"
            << "\nLIVE_A," << s.liveDrawn << ",shown while the rules hold on the forming candle - can vanish until it closes" << "\nGRAY_DRAWN," << s.grayDrawn << ",offscreen " << s.grayOffscreen << "\nBUBBLES_DRAWN," << s.bubblesDrawn << "\nBUBBLES_OFFSCREEN," << s.bubblesOffscreen
            << "\nBUBBLES_SUPPRESSED," << s.bubblesSuppressed << "\nACTIVE_ABSORPTIONS," << s.activeAbsorptions
            << "\nPAIRED_ABSORPTIONS," << s.pairedAbsorptions
            << "\nA_RULES_285,one bubble per candle per side (bullish lowest / bearish highest; first decision moment); pre-2.8.4 A re-checked once"
            << "\nBUBBLES_MERGED," << s.bubblesMerged << "\nA_HIDDEN_PRE284," << s.aHiddenPre284 << "\nA_UNDRAWN_PRE284_NO_BARS," << s.aUncheckedPre284 << '\n';
        for (std::map<long long, dp::Recheck>::const_iterator r = bk.recheck.begin(); r != bk.recheck.end(); ++r)   // (2.8.5) rule by rule
            out << "RECHECK_PRE284," << r->first << ",close " << (r->second.close ? "ok" : "fail") << ",halfway " << (r->second.halfway ? "ok" : "fail")
                << ",pivot " << (r->second.pivot ? "ok" : "fail") << " (pivot bar " << r->second.pivotBar << ")," << (r->second.pass() ? "shown" : "hidden: pre-2.8.4 rule") << '\n';
        for(const Latch& L:latches){
            const auto b=s.bubbleState.find(L.id), q=s.profileState.find(L.id);
            out << "EVENT_SYNC," << L.id << ',' << L.code << ',' << (L.support?"support":"resistance")
                << ',' << L.price << ',' << L.mult << ',' << L.net << ',' << L.peakT << ',' << L.tFirst << ',' << L.tDecided
                << ',' << (b==s.bubbleState.end()?"none":b->second) << ',' << (q==s.profileState.end()?"not shown":q->second) << '\n';
        }
        for(const dp::Node& n:s.snapshot.nodes){
            double d=0;for(const dp::Bucket& b:s.snapshot.buckets)if(b.key==n.bucket){d=b.delta;break;}
            out << "LIVE_NODE," << n.state.eventId << ',' << n.state.code << ',' << n.price << ',' << n.ratio << ',' << d << '\n';
        }
        {   // (2.7.0) the exhaustion gate, levels and candidates; the detail in plain words (no hover in this plugin)
            out << "EXH_PARAMS," << safeField(s.exSource) << ',' << safeField(s.exParams) << '\n';
            out << "EXH_LEVELS," << s.exLevelCount << ',' << safeField(s.exLevelNote) << '\n';
            std::size_t printed = 0, pending = 0;
            for (const ExMark& m : bk.ex) { if (m.status == 'P') ++printed; if (m.status == 'C') ++pending; }
            out << "EXH_RECORD,loaded " << bk.exLoaded << ",rejected " << bk.exRejected << ",candidates " << bk.ex.size() << ",printed " << printed
                << ",pending " << pending << ",drawn " << s.exDrawn << ",suppressed " << s.exSuppressed << ",offscreen " << s.exOffscreen << ',' << safeField(bk.exNote) << '\n';
            for (std::map<long long, AeRec>::const_iterator a = bk.ae.begin(); a != bk.ae.end(); ++a)
                out << "AE," << a->first << ',' << a->second.result << ',' << a->second.label << ',' << safeField(aeWords(a->second)) << '\n';
            const std::size_t from = bk.ex.size() > 20 ? bk.ex.size() - 20 : 0;
            for (std::size_t k = from; k < bk.ex.size(); ++k) {
                const ExMark& m = bk.ex[k]; const auto d = s.exState.find(m.id);
                out << "EXH," << m.id << ',' << m.status << ',' << (m.top ? "top" : "bottom") << ',' << exNum(m.extreme) << ',' << m.mark << ','
                    << (d == s.exState.end() ? (m.status == 'P' && m.mark != "none" ? "pending draw" : "logged") : d->second) << ',' << safeField(exWords(m)) << '\n';
            }
        }
        for (std::size_t k = 0; k < latches.size(); ++k)
            out << "SIGNAL," << latches[k].code << "," << (latches[k].support ? "support" : "resistance") << "," << latches[k].low * s.tick << "," << latches[k].high * s.tick << ",t " << latches[k].peakTime << '\n';
    }
    // (2.17.0, audit B) one status file PER CHART (market + seconds per bar) - the shared DeltaProfile.status.txt (last chart wins) is
    // still written for one more version (ChartView / older readers), then retired. No identical rewrites (per file, 30 s).
    out << "MARKET_STATE,DP " << safeField(s.mstNote) << '\n';
    out << "AWARENESS," << awh::VERSION << " AWDP decided " << s.aw.writes << " records" << (lsperf::smartAllowed(LSPERF_PLUGIN, s.colSym) ? "" : " (smart off)") << '\n';   // (2.24.0)
    out << "PERF," << lsperf::statusOf(this) << '\n';   // (2.24.0)
    struct Last { std::string text; std::int64_t at = -1; };
    static std::map<std::string, Last> lastByPath;
    s.statusAt = now;
    const std::string text = out.str();
    const std::string paths[2] = { directory + statusFileName(s.market, getSecondsPerBar()), directory + "DeltaProfile.status.txt" };
    for (const std::string& path : paths) {
        Last& w = lastByPath[path];
        if (text == w.text && w.at >= 0 && now - w.at < 30000) continue;
        if (atomicWrite(path, text)) { w.text = text; w.at = now; }
    }
}
int DeltaProfile::draw() { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    ChartState* current = NULL;
    try {
        ChartState& s = chartState(); current = &s; s.drawnRows = 0;
        syncLayout(s, false);
        g_drawLate = s.drawLate;   // (2.23.0) this chart's setting for every drawable test in this pass
        buildNative(s);
        if (s.ready) s.state = s.shortHistory ? "short chart history: provisional signals" :
            s.coarseBoundary ? "boundary bar crosses window: provisional signals" : "ready";
        bool drawn = false;
        {   // (2.21.2) the HOD/LOD table on this chart: nothing of the render painted over it
            const char* sy = getSymbol(); const char* cl = getChartLabel();
            s.keep.refresh(sy, getSecondsPerBar(), cl);
            bdk::KeepScope keepScope(s.keep);
            drawn = render(s);
            s.keepNote = bdk::keepLine();
        }
        if (drawn && s.state == "ready") s.state = "drawn";
        postMarketState(s);   // (2.17.0)
        selfAware(s);         // (2.25.0)
        if (s.mstPosted && s.mstBar > 0 && lsperf::smartAllowed(LSPERF_PLUGIN, s.colSym)) { s.aw.src = "DP"; s.aw.tick(s.colSym, s.colSpb, s.chartKey, s.market, (long long)steadyMs(), s.mstBar, awh::dpRead(s.mstKv)); }   // (2.24.0) L3, links OFF; off while over its perf budget
        status(s); return RTX_OK;
    } catch (const std::exception& e) {
        if (current) {
            current->ready = false; current->drawnRows = 0;
            try { current->state = std::string("calculation error: ") + e.what(); status(*current); } catch (...) {}
        }
        return RTX_FAIL;
    } catch (...) {
        if (current) { current->ready = false; current->drawnRows = 0; }
        return RTX_FAIL;
    }
}
extern "C" cppExtension* CreateExtension() {
    DeltaProfile* p = new DeltaProfile();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED);
    p->setDescription("LRA Delta Profile (DLT)"); p->setVersion("2.28.0");   // = DLT_VERSION (gex-build reads this literal; tests check they match)
    return p;
}
