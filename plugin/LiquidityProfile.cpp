/********************************************************************************
 *  0.18.0 (2026-10-14 night; lsLiquidityProfile - lsLiquidityFlow shares the source and only takes the number):
 *    1. THE BIGGEST RESTING ORDER LINES (Rassul approved 22:11 CT, mockup v2 mockup-big-resting-orders.png; BigOrderLines.h 1.0.0).
 *       On the price chart, over the candles: at most 2 live lines - the single biggest DISPLAYED resting bid (one MBO order id, never
 *       a price-level total) within 2 ATR below price in green, the biggest offer within 2 ATR above in red (ATR = 14 bars of the
 *       chart's own closed bars). Minimum size = max(the market's floor, the session's own top 0.1% of displayed order sizes so far,
 *       past-only) - floors from his recorded order files: ES 30, NQ 10, GC 15, HG 12, CL 15, NG 10, 6E 50 (a setting overrides).
 *       A line starts where the order became the biggest within reach (it must stay the biggest 10 s, or trade first - flashed orders
 *       never draw), labelled "Bid 120" / "Ask 95" at its left end (11 pt, its colour), extends bar by bar, and ends ONCE: dot =
 *       FILLED, X = PULLED / moved, arrow = still resting, a plain end = a bigger one took over / out of reach for 60 s / the depth
 *       book no longer shows it (the feed missed its exit) / IRT closed / the session ended. Never repaints: S / E / H rows in
 *       LiquidityProfile\<MKT>-<SYM>-<spb>-bigorders-<session>.csv, read back after a restart (a line resting when IRT closed ends at
 *       the last heartbeat, "after <time> not known"); an order first seen after a restart has no known start ("since restart").
 *       Hover: size, price, appeared (12-hour CT), rested, traded vs cancelled, how it ended, the rule and numbers, what it answers /
 *       how / use / example, and the limit (displayed size only). Never inside the Liquidity column nor past the last bar + arrow nor
 *       into the right-side columns (ColumnBoard). Settings (rows 5-9, per market in lsFlexLevels\LiquidityProfile.bigorders-<MKT>.txt):
 *       Big resting orders on / off, reach (ATR), min lots, ended lines kept per side, font. setParameterVersion 4.
 *    2. THE WARNING DETAIL ON TWO LINES (Rassul 22:48 CT, mockup-lp-warning-v5.png): the grey detail next to the badge is split one line
 *       per side (line 2 directly under line 1, the connector "but" / ";" / "while" dropped), never truncated (was cut with "..."); the
 *       second line's height is always reserved so the column header below never overlaps. Badge, font, colour, position unchanged.
 ********************************************************************************
 *  0.17.0 (2026-10-14 night, Rassul approved 18:25 CT; lsLiquidityProfile + lsLiquidityFlow):
 *    1. HELD / BROKE BY HIS ZONE RULE (ZoneRule.h 1.0.0, ported natively from lra.level_confluence LCF v1.1 zone_outcome /
 *       docs\study-methods\level-confluence.md; 400 / 400 random cases identical to the study's own function). Levels are ZONES price
 *       trades into, then reverses or continues: zone = the touched zone's centre +/- 0.06 ADR (ADR = the daily true range of the full
 *       17:00 CT session, mean of the prior 20 sessions from the chart's own bars, min 10; 5-9 labelled short); HELD = a 3-min close
 *       0.10 ADR back out beyond the near edge; BROKE = a close 0.10 ADR beyond the far edge with NO close back inside within 15 min
 *       (5 bars) - written on the bar where that became known; otherwise in zone (no label), pinned after 2 h (logged). A first touch
 *       or a tick through is never a break (was: a single 3-min close past the liquidity band = "Broke", a close back out after 3 min =
 *       "Absorbed"). Labels only once resolved, never changed (append-only record, redrawn from it); open encounters are rebuilt after a
 *       restart from <stem>-zonerule-<chart|panes>-<session>.csv and the closed bars replayed. The attempts log keeps every column
 *       and gains zr_adr|zr_lo|zr_hi|zr_x|zr_rule (an older session file gets the longer header once); the outcome is in the
 *       -zonerule- file (OPEN / RES rows: REV / CONT / PIN / CENSORED, failed breaks). Status line ZONE_RULE.
 *    2. AUDIT WORDING (APPROVAL-LIST-2026-10-14 s.6): hover "Hit 140 lots \xB7 rebuilt 90% - absorbing" -> "Hit 140 lots \xB7 90% re-added ?";
 *       "LQ PDL" -> "LQ? PDL" and the hover header says Bids / Offers (no "support" / "resistance"); reasons "Absorbed: ..." -> "Held:
 *       ...", "then exhausted (0.3x)" -> "then slowed to 0.3x", "held and refilled their bids" -> "re-added bids", "instead of
 *       defending" dropped; warnings ABSORPTION OCCURRING / WEAKENING / GIVING UP -> "<SIDE> SLOWED ?" / "<SIDE> SLOWED, <BIDS> PULLED ?" /
 *       "<SIDE> LIGHT, SLOWING ?"; the Flow pane's "Absorbed" statistic is named "Eaten" (it is the eaten share). KEEP items unchanged.
 *  0.16.1 (2026-10-14 evening, LIQ-DAY-2026-10-14 review "rebuilt_pct is pegged at 200 on every attempt"): ROOT CAUSE - the hit /
 *    rebuilt tally counted every NEW order in the 3-point zone after its first fill as "rebuilt" (in ES the queue churn alone is
 *    4-5x the lots hit: 10-14 15:25 status hit 3,854 / rebuilt 16,911), then capped the ratio at 200%. Now an add counts only at a
 *    price that was hit, up to the lots hit there and not yet restored (the OrderFlowFeed 1.2.1 rule), so rebuilt % is 0-100%.
 *    It feeds the hover line "Hit N lots - rebuilt NN% - absorbing" and the makeup log's rebuilt_pct only: no signal, mark or
 *    decision reads it (Stats.rebuilt / the 'Rebuilt' pane line is the separate resilience measure, unchanged).
 *  0.16.0 (2026-10-14, CR5200, Rassul approved 11:00 CT - mockup-columns.png; 11:02 the node text; 11:39 the level badges):
 *    - the column stays on the LEFT edge in its own SOLID dark column (Width 20-200 px, default 60; "Solid background" checkbox)
 *      from under the top banner to the bottom; its bars grow right toward price; candles pass behind it
 *    - each node's size INSIDE the node in white ("103"), hidden when the node is too short or narrow for it; the ratio stays
 *      OUTSIDE on the price side as before ("2.5x", only where it was shown); no other labels added
 *    - the column's width is on the ColumnBoard (ColumnBoard.h) and the zone hover stops at the right-side columns (Levels / Delta /
 *      Dealer) instead of the old Dealer reach / Delta settings guess
 *    - LRA-LiqLevels-<MKT>.csv for the Dealer Profile's level badges: every price within reach that holds size, its contracts in this
 *      plugin's own band and its prominence (lqp::Engine::levelLiq), plus the market's LQ lines (2.5x / 2.1x) - written about once a
 *      second while the book is live (the badge shows "HL 900" at 2.5x+)
 *    - a size file holding the OLD default width 70 (no 5th field) is read as the new default 60 once; OK writes "w,f,dx,dy,bg"
 ********************************************************************************
 *  LiquidityProfile.cpp  --  Investor/RT RTX extension  lsLiquidityProfile  (0.15.1a, 2026-10-14, CR5000 - the live 0.15.1 WITHOUT the 0.15.2 hover-wording change (not approved yet) + the CR4800 one-banner hand-off (Rassul 07:13 variant B): while a Tape Reader 1.1.3+ banner is on the same chart (BannerBoard.h) the warning (e.g. ABSORPTION WEAKENING ...) is NOT drawn here - it is handed to the Tape Reader as line 2 of its banner, and the LIQ label goes under that banner; with no Tape Reader the warning is drawn exactly as before. Also: the Tape Reader hand-off decided by a heartbeat (BannerBoard 1.0.1 bb::Beat: present while its stamp moves / under 5 s old / for 3 draws - no flicker on an idle chart's first repaint), the warning entry cleared when this indicator is removed, and the LIQ label gets Offset X / Y (px, -600..600, per market in LiquidityProfile.size-<MKT>.txt "w,f,dx,dy"; the dialog is filled from the file and a control he did not touch keeps the file's value; setParameterVersion 2: IRT resets the saved rows once, the file keeps Width / Font); 0.15.0, 2026-10-13: the HOVER of each NEAR zone, built around support / resistance (Rassul approved the v2 mockup 6:44 PM CT) - mouse over a near zone's bar / label in the column: "Resistance [at <LEVEL>] 7,864.00 - 7,866.00 � 366 lots (1.5x)" (green support / red resistance), "Orders 96 � avg 3.8 lots � Top order 12%" (white), "Mostly one order - may not reload ?" at a 70%+ top order (white, replaces 'may be bait'), "As price came: up 20% - building" / "down 35% - leaving" / "unchanged" (lots now vs when price first came within reach, +/-10%, baseline once per approach; coloured by what it means for price), "Hit 140 lots � rebuilt 90% - absorbing" only while price is in the zone (green support / red resistance); from the chart's order-by-order feed (OrderFlowFeed.h off::Conn / off::Engine reused, one DEPTH_BY_ORDER per chart behind a crash guard); nothing new on the chart face; the box never covers the column or the Delta Profile; each decision's makeup + 'as price came' recorded (<MKT>-<sym>-<spb>-makeup-<session>.csv); 0.14.1, 2026-10-13: the warning (e.g. ABSORPTION WEAKENING ...) starts BELOW the chart's title row (LQ_TITLE_ROW_PX) - it was drawn at the pane top, over the IRT title text (Rassul screenshot); the column header moves down with it; 0.14.0, 2026-10-13: NEAR-price rule - zones within 1x the 3-min range of price shown at 1.5x the book behind them without the rank-1 rule, kept to 1.2x; far zones unchanged at 2.0x / rank 1 / 1.7x; never under 1.0x (Rassul "i want to see liquidity when it is close so i can see if there is any absorbtion going on"); 0.13.3, 2026-10-13: audit defect fixes - a bar re-drawn at the same index is a new baseline (no replayed volume), records keyed by the full contract, input files size-bounded, prices checked before the tick grid; 0.13.2, 2026-10-12: the column's width published so the Dealer Profile badges never sit on its labels; 0.13.1, 2026-10-12: each level named on ONE zone (the one holding it, else the nearest) - as the Dealer Profile badge; 0.13.0, 2026-10-12: only the strongest zone each side (solid) + zones on a key / gamma level (outlined, named) are drawn and published - the rest logged; 0.12.0, 2026-10-12: writes LRA-LiqZones-<MKT>.csv - the resting-liquidity zones near price - for the Dealer Profile's level badges ("Liq 2.6x"); 0.11.4, 2026-10-12: review fixes - no repaint when a market's line changes, the closing bar's last trades counted before it is decided, absorption windows recorded past the 50th touch, per-market state reset on a symbol change, stale Delta A zones dropped after an hour, no zone / LQ flicker, labels never overlap, fonts 10 pt+; 0.11.3, 2026-10-12 08:58: NO letters on the price chart at all - p / e / s / t are decided and recorded for the study, never drawn (Rassul "there are all these signals like P on the es chart .. i had asked you to remove them"); 0.11.2, 2026-10-12: the e? / p? watch letters are no longer drawn on the price chart - recorded only (Rassul 06:07); 0.11.1, 2026-10-11; 0.11.1: the Flow pane backfills - buyers / sellers from every loaded bar (up to 1,000), and its own record of each bar's near-price book and each absorption window (<MKT>-<spb>-flow-<session>.csv) re-read on load for this session and the two before (Rassul 20:36 "liquidity flow"); 0.11.0: the Flow pane is ALWAYS ON - buyers / sellers of every bar as x their 20-bar average and the resting bids / offers near price at each bar close; an absorption read also starts at a Delta Profile A / A? (not only a shown zone), drawn as a yellow window with its price, the steps and a readout on the right, idle = what it watches (Rassul 19:48-19:49, mockup LiquidityFlow_always_on); 0.10.0: only zones that stand out are drawn - 2x+ the 8 same-width bands behind them and the biggest of them, kept down to 1.7x, after resting 30 s at the same band; reach = 1/2 the expected RTH range (lsHodLod), else 1/2 the 14-session average RTH range, else 6x the 3-min range; the label = that prominence; no boxes across the chart (Rassul 18:53 "remove the boxes too"); LQ = a key level or a CR / PS / HVL level with a 2.5x+ zone on it (ES, GC, HG, NG; logged elsewhere) - research "Liquidity display for absorption trading" + the rules review 18:21-18:51; 0.9.0: the column on the LEFT edge of the price pane, bars facing price, the x-normal label on the price side of each bar, each zone a thin box across the chart to the last bar (mockup v4, Rassul 18:07-18:14); 0.8.1: never over the Delta Profile or its labels (the column moves 64 px further left, past the Delta Profile's "A 4.4x +62" labels) - the Dealer Profile reach is kept PER MARKET (its status file is shared by every chart: the last chart to write it wins) and starts at the Dealer Profile default (150 px) until this market's own reach is seen (Rassul 17:56 "the liquidity profile is overlapping the delta profile"); 0.8.0: the absorption PROCESS in every market (Rassul 2026-10-11 "i want to see the process of absorption, which has the same scenarios in all markets - absorb, exhaust, counter or absorb, pull, break"): every step drawn everywhere at the market's data line where the nightly study set one, the standard line until then; s = Counter (the defenders out-traded the attackers after e, at the Counter line x their normal pace) on the price chart with its reason on the panes and the BUYERS / SELLERS STEPPING IN warning; panes AGGRESSIVE (Attack, Exh, Counter) / PASSIVE (Absorbed = the % of everything resting that took fills, Pulled, Rebuilt); 0.7.0: e? -> e and p? -> p (the watches) and t (trapped) on the price chart at the zone side of the bar, recorded and re-read for the last 10 sessions; the warning top left of the price chart (EXHAUSTION WATCH / PULLING WATCH / BUYERS HOLDING / BIDS THINNING / BUYERS PULLING / ABSORPTION OCCURRING / ABSORPTION WEAKENING / BREAKDOWN RISK / TRAPPED / GIVING UP, mirrored at resistance); the reason in plain words at each decided touch on the panes; mouse over a past touch shows its statistics as of that bar; headers AGGRESSION / DEFENSE; e decided at this market's line from arrival + 3 min; 0.6.0: the pane statistics on the left of each half in process order, Metric: now (line) - Attack (x normal per tick vs the prior 20 bars), Eaten (% of everything there: shown + new + refills), Exh | Pulled, Rebuilt - each measured as lra.mbo_study measures it and drawn only where its per-market line came from the data; depth read every 1 s (the study's snapshot step); 0.5.0: only what the data confirmed - Exh (aggression) and Pull (defense), each drawn only in markets where the nightly study confirmed it, no '?' (Rassul 2026-10-11 "remove whatever doesn't work and keep what works"); LoF / HiB / RpB / RpO and the bar-pace code removed; 0.4.1: each pane signal drawn with its number - Exh 0.3x (2nd / 1st half pace, under 0.5), Pull 72% (share pulled), LoF / HiB / RpB / RpO 2.4x (x the prior 20 bars' pace, 2.0x or more; PARAM pace_min per market); 0.4.0: no marks on the price chart (L? / LX / X removed, Rassul 2026-10-10 23:35); six pane signals Exh / LoF / HiB (aggression) and Pull / RpB / RpO (defense), recorded per session and redrawn after a restart, '?' until the study confirms each for the market; the zone label text removed; 0.3.1: the PULL? / E / LX thresholds per market from the data (PARAM lines in LRA-LiqNorms-<MKT>.csv, lra.liq_params), shown on the status file; 0.3.0: Flow pane green = buyers / red = sellers in both halves, DEFENSE = each side's net resting size (bids in / offers beyond a support zone, mirrored at resistance), O and R on the cumulative crosses; 0.2.0: the absorption story per bar (E / O / P / R / X) + the lsLiquidityFlow pane build (LiquidityFlow.cpp defines LQ_FLOW); 0.1.3: pulling blocks L? and labels a break LX P; 0.1.2: PULL? / refill - a zone being pulled instead of replenished; 0.1.1: status built at most once a second)
 *
 *  THE LIQUIDITY PROFILE (Rassul 2026-10-10: "get the liquidity profile done and the signals that support absorption ... so i can
 *  trade monday"; the agreed model: Liquidity_profile_model_v2 / Liquidity_profile_v01_mockup).
 *  One column next to the Delta Profile, bars POINTING TOWARD PRICE (anchored on the right, growing left): green = buy orders
 *  below price, red = sell orders above. The top 3 zones per side are bright, with the LOTS inside the bar and "x normal" at
 *  the tip (normal = that market's usual band size for that 30-min slot, from the nightly study; LRA-LiqNorms-<MKT>.csv).
 *  SOLID = rested 30 s+ (durable), HOLLOW = new / flickering. Each zone is also outlined on the price area up to the last bar.
 *  While price is in a zone the live read sits next to it: "eaten 0.4x  stayed 82%  E".
 *  Marks (decided on CLOSED bars, recorded, never repainted): "L? E" / "L? A" / "L? E A" = the zone held (close back out 3 min+
 *  after arrival) with exhaustion (E) and / or a Delta Profile absorption in the zone (A); "LX" = eaten 1x+ then a close through.
 *  The header says TESTING until the all-market study passes (checklist #21).
 *  Data: the chart's own depth (MARKET_DEPTH, one object per chart kept alive - MboCheck's lesson) and the chart's own VAP
 *  (trades by side per price). Native, no outside dependency at run time. Order-by-order events are not used here (their
 *  Rithmic / CQG meaning is unverified - fail closed); lsMboCheck records them for the check.
 *  Files (lsFlexLevels\LiquidityProfile\): <MKT>-<spb>-signals-<session>.csv (marks), <MKT>-<spb>-attempts-<session>.csv (every
 *  zone touch with its numbers, for the nightly scoring), and lsFlexLevels\LiquidityProfile.status-<MKT>-<spb>.txt.
 *  Logic without the SDK: LiquidityProfileLogic.h (tested: test_liquidityprofile_logic.cpp). Per-chart state: HostSlot.h.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(DELTA_WINDOWS_SDK_CHECK) && defined(__GNUC__)
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include "irtsdk.h"
#pragma pop_macro("__GNUC__")
#else
#include "irtsdk.h"
#endif
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "LiquidityProfileLogic.h"
#include "KeyLevelsLogic.h"      // (0.10.0) the key levels, computed natively from the chart (as lsDealerProfile)
#include "HostSlot.h"
#include "BannerBoard.h"           // (0.15.3) the one top banner: the warning handed to the Tape Reader
#include "BoxPlace.h"              // (CR5000) the LIQ label Offset X / Y: validation + clamping (shared, as is)
#include "SignalBoard.h"           // (0.21.0) reads / posts the chart's signals (Delta Profile A, Big Bid/Ask, big fills)
#include "ZoneNear.h"              // (0.20.0) the zone + level names for the banner verdict
#include "ColumnBoard.h"           // (0.16.0) the right-side columns (Levels / Delta / Dealer) and this column's width on the board
#ifndef LQ_FLOW
// (0.15.0) the order-by-order feed and its order book: the shared order-flow header, reused as is (lsBidAskStats / PullStats /
// StackStats). After irtsdk.h and after the near / far #undefs above (off::Engine has a member named 'near').
#define OFF_SDK 1
#include "OrderFlowFeed.h"
#include "BigOrderLines.h"     // (0.18.0) the biggest resting order lines on the price chart (SDK-free, tested)
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")   // (0.15.0) GetCursorPos / WindowFromPoint (off::cursorWindow, the hover leaving the chart)
#endif
#endif
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <locale>
#include <map>
#include <deque>
#include <set>
#include <new>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#define LQ_VERSION "0.22.8"          // keep equal to lqp::VERSION and the setVersion literal below
// (0.15.1 settings audit, 2026-10-13) Width / Font reach every chart and survive a restart: OK / Apply writes them per market to
// lsFlexLevels\<LiquidityProfile|LiquidityFlow>.size-<MKT>.txt and every draw reads that back (by its stamp, at most once a second).
// Before, Apply set them only on the copy the settings window ran in, and a restart went back to 70 px / 11 pt. The Flow pane
// draws no profile bars, so its window has only Font pt (its Width row did nothing).

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;          // green bullish / support, red bearish / resistance
static const COLOR C_BUY_DIM = 0x00166534, C_SELL_DIM = 0x007F1D1D;  // the same two colours, dimmed, for the plain book rows
static const COLOR C_GREY = 0x009CA3AF, C_AXIS = 0x00334155, C_INK = 0x000B0F17;
static const COLOR C_WHITE = 0x00FFFFFF, C_COLBG = 0x0020242C;     // (0.16.0) the lots inside a node; the column's solid background
#ifdef LQ_FLOW
static const int WIDTH_INDEX = -1, FONT_INDEX = 0, OFFX_INDEX = -1, OFFY_INDEX = -1, BG_INDEX = -1;     // (0.15.1) the Flow pane has no Width row
static const char* const LQ_SIZE_NAME = "LiquidityFlow";
#else
static const int WIDTH_INDEX = 0, FONT_INDEX = 1, OFFX_INDEX = 2, OFFY_INDEX = 3, BG_INDEX = 4;   // (CR5000) the LIQ label Offset X / Y; (0.16.0) Solid background
// (0.18.0) the big resting order lines: on / off, reach (ATR), minimum lots (the market's floor shown; 0 = the market default), ended
// lines kept per side, font - saved per market in lsFlexLevels\LiquidityProfile.bigorders-<MKT>.txt, the chart reads only that file
static const int BO_ON_INDEX = 5, BO_REACH_INDEX = 6, BO_MIN_INDEX = 7, BO_KEEP_INDEX = 8, BO_FONT_INDEX = 9;
static const char* const LQ_SIZE_NAME = "LiquidityProfile";
#endif
static const long long DEPTH_EVERY_MS = 1000, ZONES_EVERY_MS = 500, STATUS_EVERY_MS = 30000;
static const int DEPTH_READ_MAX = 200, VAP_ROWS_MAX = 5000;

// (0.11.4) the pixel height of one text row at a font size in points (96 dpi: 4/3 px per pt, plus a little leading)
static int rowPx(int pt) { return pt * 4 / 3 + 4; }
static int warnH(int font) { return rowPx(font + 2) + 2; }       // the warning box (top left of the price chart)
// (0.18.0) the warning's whole block: the badge row + the detail's further rows (at least the 2nd line is ALWAYS reserved, so the
// column header / labels below never move under a two-line warning)
static int warnBlockH(int font, int rows) { return warnH(font) + std::max(1, rows - 1) * rowPx(font + 1); }
// (0.14.1, 2026-10-13) IRT draws the chart's title ("ES LTF: EPZ26 (3m*) ...") along the top of the price pane; nothing of ours
// may start above this row. The warning sits under it and the column header under the warning.
static const int LQ_TITLE_ROW_PX = 20;
static int warnTop(int paneTop) { return paneTop + LQ_TITLE_ROW_PX; }
static const int LQ_BIG_W = 56;   // (0.22.5, Rassul 10:35 "the column for Big needs to be slightly wider") in ColumnBoard's 30-120 range
static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
static bool statFile(const std::string& p, long long* stamp = nullptr, long long* bytes = nullptr) {
#if defined(_WIN32)
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return false;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
#endif
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    if (stamp) *stamp = (long long)st.st_mtime * 1000000LL + (long long)st.st_size;
    if (bytes) *bytes = (long long)st.st_size;
    return true;
}
static const size_t INPUT_LINE_MAX = 4096, INPUT_FILE_MAX = 4U * 1024U * 1024U;
// External status/record files are advisory inputs.  Refuse non-regular or oversized
// files and discard oversized records while preserving later complete records.
static bool openBoundedInput(const std::string& path, std::ifstream& in) {
    long long bytes = 0;
    if (!statFile(path, nullptr, &bytes) || bytes < 0 || (unsigned long long)bytes > INPUT_FILE_MAX) return false;
    in.open(path.c_str(), std::ios::binary);
    return (bool)in;
}
static bool readBoundedLine(std::istream& in, std::string& line) {
    line.clear(); line.reserve(INPUT_LINE_MAX);
    bool any = false, oversized = false; char ch = 0;
    while (in.get(ch)) {
        any = true;
        if (ch == '\n') break;
        if (line.size() < INPUT_LINE_MAX) line.push_back(ch); else oversized = true;
    }
    if (oversized) line.clear();
    return any;
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// checklist #5: tmp, old to .bak, rename into place, .bak back on failure
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    const bool hadOld = statFile(path);
    if (hadOld && !moveOver(path, bak)) { std::remove(tmp.c_str()); return false; }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    return true;
}
// checklist #6: append, flush, and report success only when the stream is still good
static bool appendLines(const std::string& path, const std::string& header, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    const bool fresh = !statFile(path);
    std::ofstream f(path.c_str(), std::ios::binary | std::ios::app); if (!f) return false;
    if (fresh) f << header << "\n";
    for (size_t i = 0; i < lines.size(); ++i) f << lines[i] << "\n";
    f.flush(); return (bool)f;
}
static bool parseLL(const std::string& t, long long lo, long long hi, long long& out) {   // checklist #4
    if (t.empty() || t.size() > 24) return false;
    errno = 0; char* e = nullptr; const long long v = std::strtoll(t.c_str(), &e, 10);
    if (errno || e != t.c_str() + t.size() || v < lo || v > hi) return false;
    out = v; return true;
}
static bool parseD(const std::string& t, double& out) {
    if (t.empty() || t.size() > 40) return false;
    errno = 0; char* e = nullptr; const double v = std::strtod(t.c_str(), &e);
    if (errno || e != t.c_str() + t.size() || !lqp::finite(v)) return false;
    out = v; return true;
}
static std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> f; size_t a = 0;
    for (size_t i = 0; i <= s.size(); ++i) if (i == s.size() || s[i] == c) { f.push_back(s.substr(a, i - a)); a = i + 1; }
    return f;
}
static std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return r.substr(0, 8);
}
// Record filenames must retain the contract identity: market-only records mix a rolled
// contract with the expiring contract and can replay marks on the wrong chart.
static std::string fileToken(const std::string& value) {
    std::string out; out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = (unsigned char)value[i];
        out += (std::isalnum(c) || c == '_' || c == '-') ? (char)std::toupper(c) : '_';
    }
    if (out.empty()) out = "UNKNOWN";
    if (out.size() > 40) out.resize(40);
    return out;
}
// DeltaProfile 2.5.7 keeps its own exact token convention: preserve case and
// dots, and use its documented empty-symbol fallback.  Do not apply this
// package's uppercasing/capping convention to Delta records.
static inline std::string deltaContractToken(const std::string& value) {
    std::string out; out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = (unsigned char)value[i];
        out += (std::isalnum(c) || c == '_' || c == '-' || c == '.') ? (char)c : '_';
    }
    return out.empty() ? "unknown-contract" : out;
}
// the study's MEDIUM zone rule per market (lra.mbo_fetch.ZONES) - the norms file overrides it
static void zoneRule(const std::string& m, lqp::Params& p) {
    if (m == "ES") { p.k = 0.30; p.hwMin = 4; p.hwMax = 6; }
    else if (m == "NQ") { p.k = 0.11; p.hwMin = 8; p.hwMax = 12; }
    else if (m == "GC") { p.k = 0.25; p.hwMin = 8; p.hwMax = 15; }
    else if (m == "HG") { p.k = 0.30; p.hwMin = 2; p.hwMax = 5; }
    else if (m == "NG") { p.k = 0.30; p.hwMin = 2; p.hwMax = 4; }
    else if (m == "EU") { p.k = 0.40; p.hwMin = 2; p.hwMax = 3; }
    else { p.k = 0.25; p.hwMin = 4; p.hwMax = 8; }                 // CL and anything else
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}

struct LQState {
    lqp::Engine e;
    cppExtension::MARKET_DEPTH* md = nullptr;
    bool mdDead = false, mdBlocked = false, busy = false, faulted = false, primed = false;
    std::string identity, root, market, symbol; int spb = 0; double tick = 0;
    std::string params = "built-in defaults";
    std::set<std::string> confirmed;               // (0.4.0) pane signals the nightly study confirmed for this market (only these are drawn, 0.5.0)   // (0.3.1) the per-market thresholds in use and where each came from (data / default)
    // VAP of the bar being read: tick -> (buy, sell) already counted
    int vapBar = -1; RTDATE vapStamp = 0; std::map<lqp::Tick, std::pair<long long, long long>> vapRows;
    bool trBanner = false; bb::Rect trRect; long long handed = 0;
    bb::Beat trBeat; std::string chartKey; bool published = false;   // (CR5000) the Tape Reader heartbeat; this chart's key; my warning entry is out
    int labDx = 0, labDy = 0; std::string sizeWhy; bp::Box labBox;                    // (CR5000) the LIQ label Offset X / Y (size file); why the file was refused   // (0.15.3) a Tape Reader banner on this chart: its reserved rectangle; warnings handed over
    long barCount = 0;
    long long ctOffset = 0; bool haveOffset = false;
    long long lastBuild = -1, lastDepth = -1, lastZones = -1, lastStatus = -1, lastNorms = -1, lastLayout = -1;
    bool depthAvail = false; int depthMax = 0, depthLevels = 0; long long depthReads = 0;
    long long session = -1; size_t marksWritten = 0; long long recWrites = 0, recFails = 0;
    long long normsStamp = -2, layoutStamp = -2, dealerStamp = -2;
    int dpWidth = 50, dpGap = 0, dealerReach = 150, width = 60, font = 11; bool bg = true;   // (0.16.0) default 60 px, solid background
    cb::Peers peers; bool colPublished = false; int colPriceRight = -1;                    // (0.16.0) the right-side columns on this chart
    std::string lvlBody; long long lvlAt = -1, lvlWrites = 0; int lvlRows = 0;             // (0.16.0) LRA-LiqLevels-<MKT>.csv
    long long sizeStamp = -2, sizeCheckMs = -1; std::string sizeMkt;     // (0.15.1) <name>.size-<MKT>.txt   // (0.8.1) 150 = the Dealer Profile default reach until this market's is seen
    std::string reachMkt;                                                       // (0.8.1) the market whose saved reach was read
    std::vector<std::pair<lqp::Tick, lqp::Tick>> dpBid, dpAsk; long long dpStamp = -2; std::string dpPath;
    double atr = 0; int slot = -1;
    // (0.10.0) levels for the LQ confirmation and the reach
    struct Lvl { std::string code; lqp::Tick px = 0; };
    std::vector<Lvl> levels; long levelsAt = -1; long long mqStamp = -2; std::vector<Lvl> mqLevels;
    double avgRth = 0; int avgRthN = 0, rthOpen = -1, rthClose = -1;
    double expRange = 0; long long hlStamp = -2; std::string reachSrc = "6x 3-min range";
    // (0.17.0) the zone rule: the lagged ADR (chart bars, prior 20 sessions), the last closed bar (session end), the restore of open encounters
    int zrAdrDays = 0; std::string zrNote = "no ADR yet"; long long zrLastBarTime = 0, zrLastEndMs = 0; bool zrRestored = false; long long zrRestoredN = 0, zrWrites = 0, zrFails = 0;
    std::vector<std::string> confirms;
    // (0.11.1, Rassul 20:35-20:36 "how do i backfill ... liquidity flow") the Flow pane's own record: each closed bar's resting bids /
    // offers near price and each absorption window, per session (<MKT>-<spb>-flow-<session>.csv), re-read on load so a restart or a
    // scroll back shows what was there (IRT keeps no order-book history: the book side exists from the first recording on)
    struct Win { long long tA = 0, tE = 0; int side = 0; lqp::Tick px = 0; bool dp = false; };
    std::map<long long, std::pair<double, double>> bookHist; std::vector<Win> winHist; std::map<long long, size_t> winKeys;
    void addWin(const Win& w) {   // one window per start bar and side; a later line (the live one growing) extends its end
        std::map<long long, size_t>::iterator i = winKeys.find(w.tA * 4 + w.side);
        if (i == winKeys.end()) { winKeys[w.tA * 4 + w.side] = winHist.size(); winHist.push_back(w); } else if (w.tE > winHist[i->second].tE) winHist[i->second].tE = w.tE;
    }
    std::vector<std::string> flowLines; long long finSeen = 0;   // (0.11.4) counted against lqp::Engine::finTotal (the list keeps 50)
    std::map<long long, std::string> lqMark;       // zone id -> "LQ PDL" (drawn in ES / GC / HG / NG only)
    std::map<std::string, long long> lqPrev;       // (0.11.4) level -> the zone that confirmed it last time (hysteresis)
    long long dpAt = 0;                            // (0.11.4) Central second the Delta Profile record was last read
    std::string liqBody; long long liqAt = -1;
    int colRight = 0;                              // (0.13.2) px from the pane's left edge to the right end of the column's widest label     // (0.12.0) the level-liquidity file for the Dealer Profile badges: last body, steady ms
    bool lqShow = false;             // what LQ marks are showing (status file)
    // (0.6.0) the per-market lines of the pane statistics (PARAM lines); a statistic is drawn only when its line came from the data
    struct Line { double v = -1; bool data = false; };
    Line lAttack, lEaten, lExh, lPulled, lRebuilt, lCounter;
    int hoverBar = -1;                             // (0.7.0) the bar the mouse is over (Flow), -1 = live
    std::string verdict = "none", vlogKey = "none"; int verdictScore = 0;
    int vIncDir = 0; double vIncPx = 0; bool vIncPick = false;                                // (0.22.7) the verdict on screen (sticky)                  // (0.20.0) the banner verdict last drawn (status)
    int warnRows = 2;                              // (0.18.0) the warning detail's rows last drawn (>= 2 reserved)
    std::string state = "starting", lastStatusText;
#ifndef LQ_FLOW
    // (0.15.0) the order-by-order feed (its own DEPTH_BY_ORDER, kept alive) and its order book (off::Engine: only the book is used -
    // near = 0, so nothing is counted as near; its per-candle scratch is cleared every minute); the hover
    off::Conn feed; off::Engine ob; off::Acc obAcc; std::string feedState = "MBO: not opened"; long long obClearMs = -1;
    struct HovZone { long long id = 0; lqp::Tick lo = 0, hi = 0; std::string names; };   // names = the level(s) the zone holds
    std::vector<HovZone> hovZones;                 // the near zones drawn last time (the mouse picks among them by price)
    int hovColX0 = 0, hovColX1 = -1, hovLastBar = -1, hovLastX = 0; double hovBarW = 0;   // the column's x span and the bar -> x map
    long long hovId = 0; void* hovWnd = nullptr;   // the hovered zone (0 = none) and the window its mouse event came from
    std::string hovState = "none";                 // the last draw's hover: none / shown N rows / no room (status file)
    // (0.18.0) the biggest resting order lines: the engine (fed by the same order feed), the per-market settings file, the record
    bol::Engine bo; bol::Settings boSet; long long boStampT = -2, boStampN = -2, boCheckMs = -1; std::string boMkt, boWhy;
    long long boSession = -1, boFlushMs = -1, boLastH = 0, boWrites = 0, boFails = 0, boRestored = 0, boBad = 0, boDropped = 0;
    long long boLastStamp = -1, boLastStampMs = 0;                  // an event's stamp -> Central ms (cached per second)
    std::vector<long long> barEndMs; long boAtrCount = -1;          // every bar's END in Central ms (bars stamp their end)
    struct BoHit { long long key = 0; int b0 = 0, b1 = 0; long long px = 0, tol = 1; };
    std::vector<BoHit> boHits; long long boHovKey = 0; int boHovBar = -1; double boHovPrice = 0; void* boHovWnd = nullptr;
    int boXMin = 0, boXMax = 0, boTop = 0, boBot = 0; std::string boState = "starting", boHovState = "none"; int boDrawn = 0, boLabels = 0;
    // (0.19.0, Rassul 10-15 mockup v3) the BIG column + stack / pull: net change of the displayed size at each live order's price over 30 s
    int colBigL = -1, colBigR = -1; std::deque<std::pair<long long, double>> boHist[2]; long long boHistPx[2] = { -1, -1 };
    int boSign[2] = { 0, 0 }; double boSize[2] = { 0, 0 }; long long boNormsMs = -1, boNorms[24] = { 0 }; bool boNormsOk = false;
    int boExec = 0; long long boDepthMin = 0;
#endif
    ~LQState() { delete md; }
};

class LiquidityProfile : public cppExtension {
public:
    LiquidityProfile() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    virtual int parmsApply(void) { saveSize(); saveBig(); snap_.take(dlgValues()); return RTX_OK; }   // (0.18.0) both files judged against the SAME opening snapshot
    virtual int parmsLoad(void);                                        // (CR5000) the dialog filled from the size file (never saves)
    bp::DlgSnap snap_;                                                   // (CR5000) the dialog as it opened         // (0.15.1) OK / Apply: kept per market (loading never saves, #29)
    void saveSize();
    void saveBig();                                                      // (0.18.0) the big-order rows -> their per-market file
    std::vector<long long> dlgValues();                                  // (0.18.0) every dialog row as numbers (the snapshot)
    void loadSize(LQState& S);
    void pump();
    void release() { slot_.release(this); }
    void unpublish() {                                                  // (CR5000) removed: the Tape Reader drops line 2 at once
#ifndef LQ_FLOW
        LQState* S = slot_.get(this, false); if (S && S->published) { bb::clearWarn(S->symbol, S->spb, S->chartKey); S->published = false; }
        if (S && S->colPublished) { cb::clear(cb::LIQ, S->symbol, S->spb, S->chartKey); S->colPublished = false; }
#endif
    }
    static int clampWidth(int w) { return w < cb::range(cb::LIQ).lo ? cb::range(cb::LIQ).def : std::min(w, cb::range(cb::LIQ).hi); }   // (0.16.0) 20-200, default 60
    static int clampFont(int f) { return f < 6 ? 11 : std::min(std::max(f, 10), 16); }   // (0.11.4) 10 pt at least (Rassul: fonts too small)
private:
    HostSlot<LQState> slot_;
    long long ctSec(RTDATE d);
    long long nowMs(LQState& S);
    void identify(LQState& S);
    void openDepth(LQState& S);
    bool readDepth(LQState& S, long long t);
    void readTrades(LQState& S, long long t, long count);
    void closedBars(LQState& S, long count);
    void loadNorms(LQState& S);
    void loadLayout(LQState& S);
    void loadLevels(LQState& S, long count);
    void zoneRestore(LQState& S, long count);   // (0.17.0) open zone-rule encounters of this session after a restart     // (0.10.0) key levels + CR / PS / HVL + the 14-session RTH range
    void loadReach(LQState& S);
    void updateConfirms(LQState& S);              // (0.10.0) LQ: a key / CR / PS / HVL level with a 2.5x+ shown zone on it                  // (0.10.0) the reach: 1/2 expected range (lsHodLod) / 1/2 average RTH range / 6x
    void loadDelta(LQState& S);
    void session(LQState& S, long long t);
    void persist(LQState& S);
    void status(LQState& S, long long now);
    void writeLiqZones(LQState& S);    // (0.12.0) the zones for the Dealer Profile's level badges
    void writeLiqLevels(LQState& S);   // (0.16.0) every price's liquidity for the level badges
#ifndef LQ_FLOW
    void orders(LQState& S, long long st);          // (0.15.0) the order-by-order feed -> order book, hits / rebuilds, makeup
    void renderHover(LQState& S, int left, int top, int bottom, int paneRight);   // (0.15.0) the hovered near zone's box
    void loadBig(LQState& S);                       // (0.18.0) the big-order settings of this market (file, by its sub-second stamp)
    void boSessionCheck(LQState& S, long long t);   // (0.18.0) a new session: end / flush / restore the lines
    void boAfter(LQState& S, long count, long long st);   // (0.18.0) ATR, bar ends, price, the time-driven steps, the book check, flush
    void boFlush(LQState& S, bool force);
    void renderBig(LQState& S, int paneLeft, int paneRight, int top, int bottom, int lastBar, int lastX);
    void renderBigHover(LQState& S, int paneRight);
    std::string boPath(LQState& S) { const std::string d = dir(); return d.empty() || S.boSession < 0 ? std::string() : d + "\\" + recordStem(S) + "-bigorders-" + std::to_string(S.boSession) + ".csv"; }
    lqp::HoverIn hoverIn(const LQState& S, const lqp::Zone& z, const std::string& names) const;   // (0.15.0) its inputs
#endif
    void render(LQState& S);
    void renderFlow(LQState& S);
    void renderWarning(LQState& S);                // (0.7.0) the warning, top left of the price chart
    int barOfTime(long long barTime, long count);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\LiquidityProfile"; }
    std::string stem(LQState& S) { return S.market + "-" + std::to_string(S.spb); }
    std::string recordStem(LQState& S) { return S.market + "-" + fileToken(S.symbol) + "-" + std::to_string(S.spb); }
};

// the SDK leaves these five undefined: every RTX plugin supplies them (else LNK2019)
int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<LiquidityProfile*>(this)->unpublish(); static_cast<LiquidityProfile*>(this)->release(); return RTX_OK; }   // symbol / period change: start clean
int cppExtension::destroy(void) { static_cast<LiquidityProfile*>(this)->unpublish(); static_cast<LiquidityProfile*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
#ifdef LQ_FLOW
    setParameterVersion(2);                       // (0.15.1) Flow: Font pt only (its Width row did nothing; IRT resets the saved values once)
    setParameterDialogHeight(1);
    setIntegerParameter("Font pt", 11, 50);
#else
    setParameterVersion(4);                       // (0.16.0) + Solid background, Width 20-200; (0.18.0) + the big-order rows (IRT resets the saved rows once; the files keep them)
    setParameterDialogHeight(5);
    setIntegerParameter("Width px (20-200)", 60, 50);
    setIntegerParameter("Font pt", 11, 50, kParmAppendSameLine);
    setIntegerParameter("Label offset X", 0, 50);
    setIntegerParameter("Label offset Y", 0, 50, kParmAppendSameLine);
    setBoolParameter("Solid background", true);
    setBoolParameter("Big resting orders", true);                                   // 5  (0.18.0)
    setFloatParameter("Big order reach (ATR, 0.5-5)", 2.0f, 50, kParmAppendSameLine);   // 6
    setIntegerParameter("Big order min lots", 0, 60);                              // 7  (the market's floor is shown; 0 = market default)
    setIntegerParameter("Ended lines kept per side (0-50)", 10, 50, kParmAppendSameLine);   // 8
    setIntegerParameter("Big order font pt (10-16)", 11, 50, kParmAppendSameLine);  // 9
#endif
    return RTX_OK;
}
// (0.15.1) the window's Width / Font -> lsFlexLevels\<name>.size-<MKT>.txt ("70,11"), for every chart of the market and after a restart
// (CR5000) the size file: "w,f" (0.15.1) or "w,f,dx,dy" (the profile's LIQ label offset). false = missing / garbled (why says which)
// (0.16.0) "w,f,dx,dy,bg" (bg = Solid background 1 / 0); a file without the 5th field holding the OLD default width 70 is read as
// the new default 60 (migrated once - the next OK writes the 5-field row); widths are clamped into 20-200
struct LqSize { int w = 60, f = 11, dx = 0, dy = 0; bool bg = true, ok = false, migrated = false; std::string why; };
static LqSize readSize(const std::string& p) {
    LqSize z; std::ifstream in; std::string ln;
    long long st = -1; if (!statFile(p, &st) || st < 0) { z.why = "no size file"; return z; }
    if (!openBoundedInput(p, in) || !readBoundedLine(in, ln)) { z.why = "size file unreadable"; return z; }
    if (!ln.empty() && ln.back() == '\r') ln.pop_back();
    const std::vector<std::string> f = bp::fields(ln);
    long long w = 0, ft = 0; int dx = 0, dy = 0;
    if ((f.size() != 2 && f.size() != 4 && f.size() != 5) || !parseLL(f[0], 20, 300, w) || !parseLL(f[1], 6, 16, ft)) { z.why = "size file garbled: '" + ln.substr(0, 40) + "' - last good kept"; return z; }
    if (f.size() >= 4 && (!bp::intTok(f[2], dx) || !bp::intTok(f[3], dy) || !bp::offOk(dx) || !bp::offOk(dy))) { z.why = "bad label offset '" + f[2] + "," + f[3] + "' - 0,0"; dx = dy = 0; }
    if (f.size() == 5) { if (f[4] == "0") z.bg = false; else if (f[4] != "1") { z.why = "size file garbled: bad background flag '" + f[4].substr(0, 8) + "' - last good kept"; return z; } }
    else if (w == 70) { w = cb::range(cb::LIQ).def; z.migrated = true; }      // (0.16.0) the old default 70 -> the new default 60
    z.w = (int)std::min<long long>(w, cb::range(cb::LIQ).hi); z.f = (int)ft; z.dx = dx; z.dy = dy; z.ok = true; return z;
}
static bool lqInDialog(cppExtension* x) { const char* pl = x->getPeriodicityLabel(); return !(pl && pl[0]); }   // the settings window's copy has no bar size
static std::string lqSizePath(cppExtension* x) {
    char rb[32]; std::memset(rb, 0, sizeof(rb)); const char* rs = x->getRootSymbol(rb); const char* sy = x->getSymbol();
    const std::string root = rs ? rs : "", mkt = marketName(root.empty() ? (sy ? sy : "") : root), dirp = flexDir();
    if (mkt.empty() || dirp.empty()) return std::string();
    return dirp + "\\" + LQ_SIZE_NAME + ".size-" + mkt + ".txt";
}
// ---- (0.18.0) the big resting order lines: settings file, sub-second stamp (lesson 84), dialog rows ----------------------------------
#ifndef LQ_FLOW
static bool statSub(const std::string& p, long long& t, long long& n) {
#if defined(_WIN32)
    WIN32_FILE_ATTRIBUTE_DATA a; if (!GetFileAttributesExA(p.c_str(), GetFileExInfoStandard, &a) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return false;
    t = (long long)((((unsigned long long)a.ftLastWriteTime.dwHighDateTime) << 32 | a.ftLastWriteTime.dwLowDateTime) & 0x7FFFFFFFFFFFFFFFULL);
    n = ((long long)a.nFileSizeHigh << 32) | (long long)a.nFileSizeLow; return true;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0 || (st.st_mode & S_IFMT) != S_IFREG) return false;
    t = (long long)st.st_mtim.tv_sec * 1000000000LL + (long long)st.st_mtim.tv_nsec; n = (long long)st.st_size; return true;
#endif
}
struct BigFile { bol::Settings s; bool ok = false; std::string why; };
static BigFile readBig(const std::string& p) {
    BigFile z; std::ifstream in; std::string ln;
    if (!statFile(p)) { z.why = "no big-orders file (defaults)"; return z; }
    if (!openBoundedInput(p, in) || !readBoundedLine(in, ln)) { z.why = "big-orders file unreadable - last good kept"; return z; }
    bol::Settings s; if (!bol::parseSettings(ln, s)) { z.why = s.why; return z; }
    z.s = s; z.ok = true; return z;
}
static std::string bigMarket(cppExtension* x) {
    char rb[32]; std::memset(rb, 0, sizeof(rb)); const char* rs = x->getRootSymbol(rb); const char* sy = x->getSymbol();
    const std::string root = rs ? rs : ""; return marketName(root.empty() ? (sy ? sy : "") : root);
}
static std::string bigPath(cppExtension* x) {
    const std::string mkt = bigMarket(x), dirp = flexDir();
    if (mkt.empty() || dirp.empty()) return std::string();
    return dirp + "\\LiquidityProfile.bigorders-" + mkt + ".txt";
}
#endif
int LiquidityProfile::parmsLoad(void) {
#ifndef LQ_FLOW
    if (!lqInDialog(this)) return RTX_OK;
    const std::string p = lqSizePath(this); if (p.empty()) return RTX_OK;
    const LqSize z = readSize(p);
    if (z.ok) { setIntegerValue(WIDTH_INDEX, z.w); setIntegerValue(FONT_INDEX, z.f); setIntegerValue(OFFX_INDEX, z.dx); setIntegerValue(OFFY_INDEX, z.dy); checkBox(BG_INDEX, z.bg); }
    else { setIntegerValue(WIDTH_INDEX, cb::range(cb::LIQ).def); setIntegerValue(OFFX_INDEX, 0); setIntegerValue(OFFY_INDEX, 0); checkBox(BG_INDEX, true); }
    {   // (0.18.0) the big-order rows from their per-market file (defaults when there is none); min lots shows the market's real floor
        const std::string bp2 = bigPath(this); const BigFile bf = bp2.empty() ? BigFile() : readBig(bp2);
        const bol::Settings b = bf.ok ? bf.s : bol::Settings();
        checkBox(BO_ON_INDEX, b.on); setFloatValue(BO_REACH_INDEX, (float)(b.reach10 / 10.0));
        setIntegerValue(BO_MIN_INDEX, (int)(b.minLots > 0 ? b.minLots : bol::floorFor(bigMarket(this))));
        setIntegerValue(BO_KEEP_INDEX, b.keep); setIntegerValue(BO_FONT_INDEX, b.font);
    }
    snap_.take(dlgValues());
#endif
    return RTX_OK;
}
// (0.15.1) the window's Width / Font -> lsFlexLevels\<name>.size-<MKT>.txt ("70,11"), for every chart of the market and after a restart;
// (CR5000) + the LIQ label Offset X / Y ("70,11,dx,dy"); a control he did not touch since the dialog opened keeps the file's value
void LiquidityProfile::saveSize() {
    int w = WIDTH_INDEX >= 0 ? clampWidth(getIntegerValue(WIDTH_INDEX)) : 70, f = clampFont(getIntegerValue(FONT_INDEX));
    bool bg = true;
    const std::string p = lqSizePath(this), dirp = flexDir();
    LQState* S = slot_.get(this, false);
    int dx = 0, dy = 0;
#ifndef LQ_FLOW
    const LqSize z = p.empty() ? LqSize() : readSize(p);
    if (snap_.have) {
        const long long now[5] = { getIntegerValue(WIDTH_INDEX), getIntegerValue(FONT_INDEX), getIntegerValue(OFFX_INDEX), getIntegerValue(OFFY_INDEX), isBoxChecked(BG_INDEX) ? 1 : 0 };
        const long long fv[5] = { z.w, z.f, z.dx, z.dy, z.bg ? 1 : 0 };
        long long v[5]; for (size_t k = 0; k < 5; ++k) v[k] = snap_.pick(k, now[k], fv[k]);
        bg = v[4] != 0;
        w = clampWidth((int)std::max(-100000LL, std::min(100000LL, v[0]))); f = clampFont((int)std::max(-100000LL, std::min(100000LL, v[1])));
        dx = v[2] >= bp::OFF_MIN && v[2] <= bp::OFF_MAX ? (int)v[2] : z.dx; dy = v[3] >= bp::OFF_MIN && v[3] <= bp::OFF_MAX ? (int)v[3] : z.dy;   // garbage never reaches the file
    } else {
        const long long ox = getIntegerValue(OFFX_INDEX), oy = getIntegerValue(OFFY_INDEX);
        dx = ox >= bp::OFF_MIN && ox <= bp::OFF_MAX ? (int)ox : z.dx; dy = oy >= bp::OFF_MIN && oy <= bp::OFF_MAX ? (int)oy : z.dy;
        bg = isBoxChecked(BG_INDEX) != 0;
    }
#endif
    if (S) { S->width = w; S->font = f; S->labDx = dx; S->labDy = dy; S->bg = bg; }
    if (p.empty()) return;
    makeDir(dirp);
#ifdef LQ_FLOW
    const std::string body = std::to_string(w) + "," + std::to_string(f) + "\n";
#else
    const std::string body = std::to_string(w) + "," + std::to_string(f) + "," + std::to_string(dx) + "," + std::to_string(dy) + "," + (bg ? "1" : "0") + "\n";
#endif
    if (publishFile(p, body)) {
        if (S) { S->sizeStamp = -2; S->sizeCheckMs = -1; }
    }                                                                  // (0.18.0) the snapshot is retaken by parmsApply after BOTH files
}
std::vector<long long> LiquidityProfile::dlgValues() {
    std::vector<long long> v;
#ifndef LQ_FLOW
    v.push_back(getIntegerValue(WIDTH_INDEX)); v.push_back(getIntegerValue(FONT_INDEX)); v.push_back(getIntegerValue(OFFX_INDEX)); v.push_back(getIntegerValue(OFFY_INDEX)); v.push_back(isBoxChecked(BG_INDEX) ? 1 : 0);
    const double r = (double)getFloatValue(BO_REACH_INDEX);
    v.push_back(isBoxChecked(BO_ON_INDEX) ? 1 : 0); v.push_back(bol::finiteD(r) ? (long long)std::llround(r * 10.0) : -1);
    v.push_back(getIntegerValue(BO_MIN_INDEX)); v.push_back(getIntegerValue(BO_KEEP_INDEX)); v.push_back(getIntegerValue(BO_FONT_INDEX));
#endif
    return v;
}
// (0.18.0) OK / Apply: the big-order rows -> lsFlexLevels\LiquidityProfile.bigorders-<MKT>.txt (a control he did not touch since the
// dialog opened keeps the file's value - IRT may hand a copy with boxes unchecked; garbage never reaches the file)
void LiquidityProfile::saveBig() {
#ifndef LQ_FLOW
    const std::string p = bigPath(this); if (p.empty()) return;
    const std::string mkt = bigMarket(this);
    const BigFile bf = readBig(p); const bol::Settings fv = bf.ok ? bf.s : bol::Settings();
    const long long floorM = bol::floorFor(mkt), fileMin = fv.minLots > 0 ? fv.minLots : floorM;
    const std::vector<long long> now = dlgValues();
    const long long fileV[5] = { fv.on ? 1 : 0, fv.reach10, fileMin, fv.keep, fv.font };
    long long v[5]; for (size_t k = 0; k < 5; ++k) v[k] = snap_.pick(5 + k, now[5 + k], fileV[k]);
    long long minLots = v[2]; if (minLots == floorM) minLots = 0;           // the market's own floor: keep following its default
    if (minLots < 0 || minLots > bol::FLOOR_MAX) minLots = fv.minLots;
    const bol::Settings z = bol::cleanSettings(v[0] != 0, v[1] >= 0 ? (double)v[1] / 10.0 : -1.0, minLots, v[3], v[4], fv);
    makeDir(flexDir());
    if (publishFile(p, bol::settingsLine(z) + "\n")) { LQState* S = slot_.get(this, false); if (S) { S->boStampT = S->boStampN = -2; S->boCheckMs = -1; } }
#endif
}
void LiquidityProfile::loadSize(LQState& S) {
    const long long now = steadyMs();
    if (S.sizeMkt == S.market && S.sizeCheckMs >= 0 && now >= S.sizeCheckMs && now - S.sizeCheckMs < 1000) return;
    S.sizeCheckMs = now;
    const std::string dirp = flexDir();
    if (dirp.empty() || S.market.empty()) return;
    const std::string p = dirp + "\\" + LQ_SIZE_NAME + ".size-" + S.market + ".txt";
    long long st = -1; if (!statFile(p, &st)) st = -1;
    if (S.sizeMkt == S.market && st == S.sizeStamp) return;
    S.sizeMkt = S.market; S.sizeStamp = st;
    if (st < 0) { S.labDx = S.labDy = 0; S.sizeWhy.clear(); return; }    // no file: the window's / default size stays, no offset
    const LqSize z = readSize(p);
    S.sizeWhy = z.why;
    if (!z.ok) return;                                                    // garbled: keep (reason on the status file)
    if (z.migrated && S.sizeWhy.empty()) S.sizeWhy = "old default width 70 read as 60 (0.16.0)";
    S.width = clampWidth(z.w); S.font = clampFont(z.f); S.labDx = z.dx; S.labDy = z.dy; S.bg = z.bg;
}
int cppExtension::calc(int) { static_cast<LiquidityProfile*>(this)->pump(); return RTX_OK; }
// (0.8.0, Rassul 2026-10-11 "i want to see the process of absorption, which has the same scenarios in all markets") every step of the
// process is drawn in every market: e (Exh) at or under the market's line, p (Pull) at or over it, s (Counter) at or over it, t (trapped),
// a watch (e? p?) until that same touch printed the full step (then the e / p stands for it, like A? -> A), the reasons whenever they have
// words. The line is the market's own from the nightly data, the standard line until the study sets one (the status file says which).
static bool shown(const LQState& S, const lqp::Mark& m) {
    if (m.code == "Held" || m.code == "Broke") return !m.reasons.empty();
    if (m.code == "t") return true;
    // (0.11.2, Rassul 2026-10-12 06:07 "where are all these e and p signals coming from on crude" -> "Drop e? / p? watches") the watches
    // are still decided and recorded for the study, never drawn; only the full e / p (and t, s) reach the price chart
    if (m.code == "ew" || m.code == "pw") return false;
    // (0.11.4) a step is recorded only when it crossed the line in force when its bar closed; it is NOT checked again against today's
    // line - before, a new nightly line (norms file reloaded every minute) hid or re-showed past Exh / Pull / s marks (a repaint)
    (void)S;
    if (m.value < 0) return false;
    return m.code == "Exh" || m.code == "Pull" || m.code == "s";
}
// (0.8.0) the standard lines, used until the nightly study sets this market's own (lra.liq_params defaults)
static void standardLines(LQState& S) {
    S.lAttack = {2.0, false}; S.lEaten = {0.15, false}; S.lExh = {0.5, false}; S.lPulled = {0.7, false}; S.lRebuilt = {1.0, false}; S.lCounter = {1.25, false};
    S.e.P.exhaust = 0.5; S.e.P.pullShare = 0.7; S.e.P.counterX = 1.25;
    S.e.L.attack = 2.0; S.e.L.eaten = 0.15; S.e.L.exh = 0.5; S.e.L.pulled = 0.7; S.e.L.rebuilt = 1.0; S.e.L.counter = 1.25; S.e.L.trapped = true;
}
int LiquidityProfile::draw(void) {
    pump();
    LQState* S = slot_.get(this, false);
    if (S && !S->faulted) { try { loadSize(*S); } catch (...) {} }          // (0.15.1) this market's Width / Font
#ifdef LQ_FLOW
    if (S && !S->faulted) { try { renderFlow(*S); } catch (...) { S->state = "draw error"; } }
#else
    if (S && !S->faulted) { try { renderWarning(*S); render(*S); } catch (...) { S->state = "draw error"; } }   // (0.11.3) no price-chart letters
#endif
    return RTX_OK;
}

// a bar / clock stamp as the Central wall clock in epoch seconds (TapeFlow / Delta Profile convention); 0 = unreadable
long long LiquidityProfile::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
// engine time: the Central clock in ms, smoothed with the steady clock (the SDK clock has whole seconds only)
long long LiquidityProfile::nowMs(LQState& S) {
    const long long st = steadyMs(), ct = ctSec(currentDate()) * 1000LL;
    if (ct <= 0) return S.haveOffset ? st + S.ctOffset : st;
    if (!S.haveOffset || std::llabs((st + S.ctOffset) - ct) > 2000) { S.ctOffset = ct - st; S.haveOffset = true; }
    return st + S.ctOffset;
}

void LiquidityProfile::identify(LQState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    double tick = (double)getProperty(SYM_TICKINCR);                       // checklist #24: the chart's own tick
    if (!lqp::finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
    else tick = std::round(tick * 1e8) / 1e8;                             // (0.11.4) IRT's tick is a float (0.01f = 0.0099999998): snap it
    const std::string id = sym + "|" + std::to_string(spb);
    if (id != S.identity || tick != S.tick) {
        const int w = S.width, f = S.font; const bool bg = S.bg;
#ifndef LQ_FLOW
        if (S.colPublished) { cb::clear(cb::LIQ, S.symbol, S.spb, S.chartKey); S.colPublished = false; }   // (0.16.0) the old chart's entry
#endif
        S.peers = cb::Peers(); S.colPriceRight = -1; S.lvlBody.clear(); S.lvlAt = -1;
        LQState fresh; S.e = fresh.e; delete S.md; S.md = nullptr; S.mdDead = S.mdBlocked = false; S.barCount = 0; S.vapRows.clear(); S.vapBar = -1; S.vapStamp = 0; S.primed = false; S.session = -1; S.marksWritten = 0;
        S.normsStamp = S.layoutStamp = S.dealerStamp = S.dpStamp = -2; S.width = w; S.font = f; S.bg = bg; S.params = "built-in defaults"; S.confirmed.clear();
        // (0.11.4) everything that belongs to the old market / chart: its RTH hours, key and MenthorQ levels (the MenthorQ file is only
        // re-read when it changes, so the old market's CR / PS stayed), reach, LQ, Delta A zones and the Flow record
        S.levels.clear(); S.levelsAt = -1; S.mqLevels.clear(); S.mqStamp = -2; S.avgRth = 0; S.avgRthN = 0; S.rthOpen = S.rthClose = -1;
        S.expRange = 0; S.reachSrc = "6x 3-min range"; S.confirms.clear(); S.lqMark.clear(); S.lqPrev.clear(); S.dpBid.clear(); S.dpAsk.clear(); S.dpAt = 0;
#ifndef LQ_FLOW
        S.feed.reset(); S.ob = off::Engine(); S.obAcc = off::Acc(); S.feedState = "MBO: not opened"; S.obClearMs = -1; S.hovZones.clear(); S.hovId = 0; S.hovWnd = nullptr;   // (0.15.0)
        if (S.boSession >= 0) boFlush(S, true);                         // (0.18.0) the old chart's pending rows go to ITS file first
        S.bo = bol::Engine(); S.boMkt.clear(); S.boStampT = S.boStampN = -2; S.boCheckMs = -1; S.boSession = -1; S.boLastH = 0; S.boLastStamp = -1;
        S.barEndMs.clear(); S.boAtrCount = -1; S.boHits.clear(); S.boHovKey = 0; S.boHovWnd = nullptr;
#endif
        S.atr = 0; S.finSeen = 0; S.hoverBar = -1; S.bookHist.clear(); S.winHist.clear(); S.winKeys.clear(); S.flowLines.clear(); S.lastStatusText.clear();
        S.identity = id; S.root = root; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
        S.e.P.tick = tick > 0 ? tick : 0.01; zoneRule(S.market, S.e.P); standardLines(S);
#ifndef LQ_FLOW
        S.bo.tick = S.e.P.tick;                                         // (0.18.0)
#endif
        S.e.P.requireShown = true; S.e.P.topN = 8;                         // (0.10.0) candidates; only the shown ones are drawn / read
        S.lqShow = S.market == "ES" || S.market == "GC" || S.market == "HG" || S.market == "NG";   // CL / NQ / 6E: logged only
        S.e.P.barMs = spb > 0 ? (lqp::Ms)spb * 1000 : 180000;
    }
}

// one depth object per chart, kept alive (its destructor unsubscribes); opened behind a crash guard like lsMboCheck
void LiquidityProfile::openDepth(LQState& S) {
    if (S.md || S.mdDead || S.mdBlocked) return;
    const std::string d = dir();
    const std::string guard = d.empty() ? std::string() : d + "\\_opening-" + stem(S) + ".txt";
    if (!guard.empty() && statFile(guard)) { S.mdBlocked = true; S.state = "depth open crashed IRT before: delete " + guard + " to retry"; return; }
    if (!guard.empty()) { makeDir(flexDir()); makeDir(d); std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening MARKET_DEPTH\n"; }
    try { S.md = new (std::nothrow) MARKET_DEPTH(NULL); } catch (...) { S.mdDead = true; }
    if (!guard.empty()) std::remove(guard.c_str());
}

bool LiquidityProfile::readDepth(LQState& S, long long t) {
    if (!S.md || S.mdDead) return false;
    std::vector<std::pair<double, double>> bids, asks;
    try {
        S.depthAvail = S.md->available() != 0;
        S.depthMax = S.md->maxLevels;
        if (!S.depthAvail) return false;
        int n = S.depthMax; if (n > DEPTH_READ_MAX) n = DEPTH_READ_MAX; if (n < 0) n = 0;
        S.depthLevels = 0;
        for (int k = 0; k < n; ++k) {
            const DEPTH_LEVEL L = (*S.md)[(unsigned)k];                   // a copy: the SDK hands out a pointer into IRT's book
            const bool b = lqp::finite(L.bid) && L.bid > 0 && L.bidsize > 0, a = lqp::finite(L.ask) && L.ask > 0 && L.asksize > 0;
            if (b) bids.push_back(std::make_pair((double)L.bid, (double)L.bidsize));
            if (a) asks.push_back(std::make_pair((double)L.ask, (double)L.asksize));
            if (b || a) S.depthLevels++;
        }
    } catch (...) { S.mdDead = true; S.state = "depth read fault"; return false; }
    if (bids.empty() || asks.empty()) return false;
    S.depthReads++;
    return S.e.onBook(t, bids, asks);
}

// trades = the increase of the forming bar's VAP since the last read (buy = lifted the offer, sell = hit the bid)
static int vapStats(cppExtension::RTARRAYP* vap, int bar, cppExtension::BARSTATISTICS* st) {
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
static int vapRowsRead(cppExtension::RTARRAYP* vap, int bar, cppExtension::VOLPROFILE* rows, int count) {
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) { std::memset(&rows[i], 0, sizeof(rows[i])); if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2; }
        return count;                                                    // checklist #22: any row failure rejects the whole read
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
void LiquidityProfile::readTrades(LQState& S, long long t, long count) {
    if (count < 1 || S.tick <= 0) return;
    RTARRAYP vap(barVolumeProfile);
    RTARRAYI times(barDateTime);
    if (vap.count < count || times.count < count) return;
    const int bars[2] = { S.vapBar >= 0 && S.vapBar < (int)count - 1 ? S.vapBar : -1, (int)count - 1 };   // the bar just closed gets its last read
    for (int k = 0; k < 2; ++k) {
        const int bar = bars[k]; if (bar < 0) continue;
        BARSTATISTICS st; const int n = vapStats(&vap, bar, &st);
        if (n == -1) { S.faulted = true; S.state = "VAP fault: reload the indicator to retry"; return; }
        if (n < 0 || n > VAP_ROWS_MAX) continue;
        std::vector<VOLPROFILE> rows((size_t)n);
        const int r = vapRowsRead(&vap, bar, rows.empty() ? nullptr : &rows[0], n);
        if (r == -1) { S.faulted = true; S.state = "VAP fault: reload the indicator to retry"; return; }
        if (r < 0) continue;
        const RTDATE stamp = times[bar];                                 // (0.13.3, audit LQP-002) a bar's identity = index + time
        const bool newIdentity = bar != S.vapBar || stamp != S.vapStamp;
        if (newIdentity) { S.vapRows.clear(); S.vapBar = bar; S.vapStamp = stamp; }
        // A first view of a VAP identity lacks a causal prior snapshot: baseline it rather than replaying history shifted into this index
        const bool prime = !S.primed || newIdentity;                     // what traded before we loaded is history, not new trades
        long long tt = t;                                                // (0.11.4) the closed bar's last trades belong to it: stamped at its end
        if (k == 0) { RTARRAYI tms(barDateTime); if (tms.count > bar) { const long long e = ctSec((RTDATE)tms[bar]) * 1000LL; if (e > 0 && e < t) tt = e; } }
        for (size_t i = 0; i < rows.size(); ++i) {
            const VOLPROFILE& v = rows[i];
            if (!lqp::finite(v.price) || v.price <= 0 || v.buyVolume < 0 || v.sellVolume < 0) continue;
            lqp::Tick px = 0; if (!lqp::tryToTick(v.price, S.tick, px)) continue;
            std::pair<long long, long long>& seen = S.vapRows[px];
            const long long db = (long long)v.buyVolume - seen.first, ds = (long long)v.sellVolume - seen.second;
            seen.first = (long long)v.buyVolume; seen.second = (long long)v.sellVolume;
            S.e.formBar = bar;                                           // (0.2.0) the story's per-bar flow goes to the bar it traded in
            if (!prime && (db > 0 || ds > 0)) S.e.onTrade(tt, (double)px * S.tick, (double)std::max(0LL, db), (double)std::max(0LL, ds));
        }
        S.primed = true;
    }
    RTARRAY close(barClose);
    if (close.count >= count) S.e.onLastPrice(close[(int)count - 1]);
}

// (0.6.0) the Attack readout's normal: each closed bar's buyers' / sellers' volume (IRT's own bar statistics) and the ticks it traded
static void seedBars(LQState& S, cppExtension::RTARRAYP& vap, cppExtension::RTARRAY& high, cppExtension::RTARRAY& low, int from, int to) {
    if (S.tick <= 0) return;
    for (int b = std::max(0, from); b <= to && b < (int)vap.count && b < (int)high.count && b < (int)low.count; ++b) {
        cppExtension::BARSTATISTICS st; const int n = vapStats(&vap, b, &st);
        if (n < 0 || st.buyVolume < 0 || st.sellVolume < 0) continue;
        const double hi = (double)high[b], lo = (double)low[b];
        if (!lqp::finite(hi) || !lqp::finite(lo) || hi < lo) continue;
        lqp::Tick hiTick = 0, loTick = 0;
        if (!lqp::tryToTick(hi, S.tick, hiTick) || !lqp::tryToTick(lo, S.tick, loTick) || hiTick < loTick) continue;
        const double span = (double)hiTick - (double)loTick + 1.0;
        if (!lqp::finite(span) || span > INT_MAX) continue;
        S.e.setBar(b, (double)st.buyVolume, (double)st.sellVolume, (int)span, hiTick, loTick);
    }
}
// a bar is closed once a later bar exists (checklist #27); each closed bar is decided once, in order
void LiquidityProfile::closedBars(LQState& S, long count) {
    if (S.barCount == 0) { S.barCount = count; S.e.curBar = (int)count - 2; S.e.formBar = (int)count - 1; RTARRAYP vap(barVolumeProfile); RTARRAY hi(barHigh), lo(barLow); seedBars(S, vap, hi, lo, (int)count - 1000, (int)count - 2); zoneRestore(S, count); return; }   // (0.11.1) every loaded bar (up to 1,000) for the always-on pane   // history before load: never decided
    if (count <= S.barCount) { if (count < S.barCount) S.barCount = count; return; }
    RTARRAYI times(barDateTime); RTARRAY close(barClose), high(barHigh), low(barLow);
    if (times.count < count || close.count < count || high.count < count || low.count < count) return;   // (0.11.4) high / low too
    loadDelta(S);
    RTARRAYP vap(barVolumeProfile);
    // ATR of the chart's closed bars (20), scaled to a 3-min bar when the chart is not 3-min
    double sum = 0; int m = 0;
    for (int i = (int)count - 2; i >= 0 && m < 20; --i, ++m) sum += (double)high[i] - (double)low[i];
    if (m > 0) { S.atr = sum / m; if (S.spb > 0 && S.spb != 180) S.atr *= std::sqrt(180.0 / (double)S.spb); }
    for (long i = S.barCount - 1; i < count - 1; ++i) {
        const long long bt = (long long)times[(int)i];
        const long long endMs = ctSec((RTDATE)times[(int)i]) * 1000LL;   // IRT stamps the bar END (checklist #12)
        if (endMs > 0) { seedBars(S, vap, high, low, (int)i, (int)i); S.e.onBarClose((int)i, bt, endMs, (double)close[(int)i], S.dpBid, S.dpAsk); S.zrLastBarTime = bt; S.zrLastEndMs = endMs; }
#ifdef LQ_FLOW
        std::map<int, std::pair<double, double>>::const_iterator nb = S.e.nearBook.find((int)i);
        if (nb != S.e.nearBook.end() && !S.bookHist.count(bt)) {
            S.bookHist[bt] = nb->second; char b[96]; std::snprintf(b, sizeof(b), "B|%lld|%.0f|%.0f", bt, nb->second.first, nb->second.second); S.flowLines.push_back(b);
        }
#endif
    }
#ifdef LQ_FLOW
    {   // (0.11.1) each absorption window: the finished ones once, the live ones at every bar close (their end grows; a restart keeps it)
        auto rec = [&](const lqp::Attempt& a, int e) {
            if (a.barA < 0 || a.barA >= (int)count || e < a.barA || e >= (int)count) return;
            LQState::Win w; w.tA = (long long)times[a.barA]; w.tE = (long long)times[e]; w.side = a.side; w.dp = a.dpHi > 0;
            w.px = a.dpHi > 0 ? (a.side == lqp::BID ? a.dpHi : a.dpLo) : (a.side == lqp::BID ? a.hi : a.lo);
            S.addWin(w); char b[128]; std::snprintf(b, sizeof(b), "W|%lld|%lld|%d|%lld|%d", w.tA, w.tE, w.side, (long long)w.px, w.dp ? 1 : 0); S.flowLines.push_back(b);
        };
        const long long base = S.e.finTotal - (long long)S.e.finished.size();   // (0.11.4) the list keeps the last 50: count by total
        if (S.finSeen < base) S.finSeen = base;
        for (; S.finSeen < S.e.finTotal; ++S.finSeen) {
            const lqp::Attempt& a = S.e.finished[(size_t)(S.finSeen - base)];
            int e = a.flowBars.empty() ? a.barA : a.flowBars.rbegin()->first; if (a.barE >= 0) e = std::max(e, a.barE);
            rec(a, e);
        }
        for (size_t k = 0; k < S.e.attempts.size(); ++k) if (!S.e.attempts[k].done) rec(S.e.attempts[k], (int)count - 2);
    }
#endif
    S.barCount = count;
    persist(S);
}

void LiquidityProfile::loadNorms(LQState& S) {
    const std::string f = flexDir(); if (f.empty() || S.market.empty()) return;
    const std::string path = f + "\\LRA-LiqNorms-" + S.market + ".csv";
    long long stamp = -1; if (!statFile(path, &stamp)) { S.normsStamp = -1; return; }
    if (stamp == S.normsStamp) return;
    std::ifstream in;
    if (!openBoundedInput(path, in)) { S.normsStamp = stamp; return; }
    S.normsStamp = stamp; std::string line; int lines = 0;
    std::map<int, double> med; lqp::Params p = S.e.P; std::string info; std::set<std::string> conf;
    LQState::Line la{2.0, false}, le{0.15, false}, lx{0.5, false}, lp{0.7, false}, lr{1.0, false}, lc{1.25, false};   // (0.8.0) the standard lines
    p.exhaust = 0.5; p.pullShare = 0.7; p.counterX = 1.25;
    while (readBoundedLine(in, line) && ++lines < 2000) {               // checklist #33: bounded input
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, ',');
        long long a = 0, b = 0; double x = 0;
        if (c.size() >= 4 && c[0] == "ZONE" && parseD(c[1], x) && parseLL(c[2], 1, 200, a) && parseLL(c[3], 1, 200, b) && x > 0 && a <= b) { p.k = x; p.hwMin = (int)a; p.hwMax = (int)b; }
        else if (c.size() >= 3 && c[0] == "SLOT" && parseLL(c[1], 0, 47, a) && parseD(c[2], x) && x > 0) med[(int)a] = x;
        // (0.3.1, Rassul 2026-10-10 "data driven ... optimized for each market") PARAM,<name>,<value>,<data|default>,... - each value is
        // range-checked; a bad or missing line keeps the built-in default
        else if (c.size() >= 3 && c[0] == "SIGNAL" && (lqp::Engine::paneCode(c[1]) || c[1] == "Old" || c[1] == "Trapped" || c[1] == "Counter")) { if (c[2] == "confirmed") conf.insert(c[1]); }
        else if (c.size() >= 4 && c[0] == "PARAM" && parseD(c[2], x)) {
            const std::string src = c[3] == "data" ? "data" : "default";
            const bool d = src == "data";
            if (c[1] == "pull_share" && x >= 0.2 && x <= 5) { p.pullShare = x; lp.v = x; lp.data = d; info += " pull_share " + c[2] + " " + src; }
            else if (c[1] == "exhaust" && x >= 0.1 && x <= 0.9) { p.exhaust = x; lx.v = x; lx.data = d; info += " exhaust " + c[2] + " " + src; }
            else if (c[1] == "attack_x" && x >= 0.5 && x <= 10) { la.v = x; la.data = d; info += " attack_x " + c[2] + " " + src; }
            else if (c[1] == "eaten_total" && x >= 0.01 && x <= 1) { le.v = x; le.data = d; info += " eaten_total " + c[2] + " " + src; }
            else if (c[1] == "rebuilt" && x >= 0.1 && x <= 3) { lr.v = x; lr.data = d; info += " rebuilt " + c[2] + " " + src; }
            else if (c[1] == "counter_x" && x >= 0.5 && x <= 10) { p.counterX = x; lc.v = x; lc.data = d; info += " counter_x " + c[2] + " " + src; }
            else if (c[1] == "eaten_break" && x >= 0.25 && x <= 5) { p.eatenBreak = x; info += " eaten_break " + c[2] + " " + src; }
        }
    }
    S.e.norms.median.swap(med); S.e.norms.loaded = true; S.e.P.k = p.k; S.e.P.hwMin = p.hwMin; S.e.P.hwMax = p.hwMax;
    S.e.P.pullShare = p.pullShare; S.e.P.exhaust = p.exhaust; S.e.P.eatenBreak = p.eatenBreak; S.e.P.counterX = p.counterX;
    S.params = info.empty() ? "built-in defaults" : info.substr(1);
    S.confirmed.swap(conf);
    S.lAttack = la; S.lEaten = le; S.lExh = lx; S.lPulled = lp; S.lRebuilt = lr; S.lCounter = lc;
    // (0.8.0) the process in every market: the reasons and warnings use the market's data line, the standard line until it exists
    S.e.L.attack = la.v; S.e.L.eaten = le.v; S.e.L.exh = lx.v; S.e.L.pulled = lp.v; S.e.L.rebuilt = lr.v; S.e.L.counter = lc.v; S.e.L.trapped = true;
}

// where the column goes: just left of the Delta Profile (its width / gap for this root) and the Dealer Profile (its reach)
void LiquidityProfile::loadLayout(LQState& S) {
    const std::string f = flexDir(); if (f.empty()) return;
    if (S.reachMkt != S.market) {                                               // (0.8.1) this market's last known Dealer Profile reach
        S.reachMkt = S.market; S.dealerReach = 150;
        std::ifstream in; long long r = 0; std::string ln;
        if (openBoundedInput(f + "\\LiquidityProfile.reach-" + S.market + ".txt", in) && readBoundedLine(in, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (parseLL(ln, 0, 1199, r) && r > 0) S.dealerReach = (int)r; }
    }
    long long st = -1;
    if (statFile(f + "\\DeltaProfile.settings.txt", &st) && st != S.layoutStamp) {
        S.layoutStamp = st;
        std::ifstream in; std::string line; int n = 0;
        if (openBoundedInput(f + "\\DeltaProfile.settings.txt", in)) while (readBoundedLine(in, line) && ++n < 200) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> c = split(line, '|'); long long w = 0, g = 0;
            if (c.size() >= 4 && c[0] == S.root && parseLL(c[2], 30, 600, w) && parseLL(c[3], -300, 400, g)) { S.dpWidth = (int)w; S.dpGap = (int)g; }
        }
    }
    if (statFile(f + "\\DealerProfile.status.txt", &st) && st != S.dealerStamp) {
        S.dealerStamp = st;
        std::ifstream in; std::string line, mkt; long long reach = 0; int n = 0;
        if (openBoundedInput(f + "\\DealerProfile.status.txt", in)) while (readBoundedLine(in, line) && ++n < 400) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> c = split(line, ',');
            if (c.size() >= 2 && c[0] == "MARKET") mkt = c[1];
            else if (c.size() >= 2 && c[0] == "REACH") parseLL(c[1], 0, 1199, reach);
        }
        if (mkt == S.market && reach > 0 && (int)reach != S.dealerReach) {      // (0.8.1) this market's own reach: use it and keep it
            S.dealerReach = (int)reach;
            publishFile(f + "\\LiquidityProfile.reach-" + S.market + ".txt", std::to_string(reach) + "\n");   // checklist #5
        }
    }
}

// (0.10.0) the market of a MenthorQLevels.csv symbol: CQG (EPZ26, ENQZ26, CLEX26, GCEZ26, CPEZ26, NGEX26, EU6Z26) or Rithmic
// (ESZ6, NQZ6, CLX6, GCZ6, HGZ6, NGX6, 6EZ6) - the root before the month code
static std::string mqMarket(const std::string& sym) {
    static const char* const map[][2] = { {"ENQ", "NQ"}, {"EP", "ES"}, {"CLE", "CL"}, {"GCE", "GC"}, {"CPE", "HG"}, {"NGE", "NG"}, {"EU6", "EU"},
                                          {"NQ", "NQ"}, {"ES", "ES"}, {"CL", "CL"}, {"QCL", "CL"}, {"GC", "GC"}, {"QGC", "GC"}, {"HG", "HG"}, {"QHG", "HG"},
                                          {"NG", "NG"}, {"6E", "EU"} };
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
        const std::string r = map[i][0];
        if (sym.size() >= r.size() + 2 && sym.compare(0, r.size(), r) == 0 && std::strchr("FGHJKMNQUVXZ", sym[r.size()])) return map[i][1];
    }
    return "";
}
// a MenthorQ label part that is a gamma-1 level (call resistance, put support, high-vol level); gamma walls 1-10 never confirm
static bool gamma1(const std::string& part) {
    std::string p = part; while (!p.empty() && p[0] == ' ') p.erase(0, 1);
    return p.compare(0, 2, "CR") == 0 || p.compare(0, 2, "PS") == 0 || p.compare(0, 3, "HVL") == 0;
}
void LiquidityProfile::loadLevels(LQState& S, long count) {
    if (S.market.empty() || S.tick <= 0) return;
    // MenthorQ CR / PS / HVL levels (the one external input of the confirmation; read only when the file changes)
    const std::string f = flexDir();
    if (!f.empty()) {
        long long st = -1; const std::string mq = f + "\\MenthorQLevels.csv";
        if (!statFile(mq, &st)) { S.mqLevels.clear(); S.mqStamp = -1; }
        else if (st != S.mqStamp) {
            S.mqStamp = st; S.mqLevels.clear();
            std::ifstream in; std::string line; int n = 0;
            if (openBoundedInput(mq, in)) while (readBoundedLine(in, line) && ++n < 2000) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                const std::vector<std::string> c = split(line, ',');
                if (c.size() < 3 || mqMarket(c[0]) != S.market) continue;
                double px = 0; if (!parseD(c[1], px) || px <= 0) continue;
                const std::vector<std::string> parts = split(c[2], '/');
                for (size_t k = 0; k < parts.size(); ++k) if (gamma1(parts[k])) {
                    LQState::Lvl L; if (!lqp::tryToTick(px, S.tick, L.px)) continue; std::string code = parts[k];
                    while (!code.empty() && code[0] == ' ') code.erase(0, 1);
                    while (!code.empty() && code.back() == ' ') code.pop_back();
                    L.code = code; S.mqLevels.push_back(L); break;
                }
            }
        }
    }
    // the key levels from the chart's own bars (lsDealerProfile's KeyLevelsLogic) and the 14-session RTH range - on a new bar only
    if (count == S.levelsAt || count < 30) return;
    S.levelsAt = count;
    if (S.rthOpen < 0 && !svl::rthOf(S.market.c_str(), S.rthOpen, S.rthClose)) { S.rthOpen = -2; return; }
    if (S.rthOpen < 0) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if (times.count < count || hi.count < count || lo.count < count || cl.count < count) return;   // (0.11.4) close too
    const long first = std::max(0L, count - 12000);                    // ~25 sessions of 3-min bars; bounded work (checklist #40)
    std::vector<klv::Bar> B; B.reserve((size_t)(count - first));
    for (long i = first; i < count; ++i) {
        const long long cs = ctSec((RTDATE)times[(int)i]); if (cs <= 0) continue;
        klv::Bar b; b.days = (long)(cs / 86400); b.endSec = (int)(cs % 86400); b.h = hi[(int)i]; b.l = lo[(int)i]; b.c = cl[(int)i];
        B.push_back(b);
    }
    if (B.size() < 30) return;
    const int per = S.spb > 0 ? S.spb : 180;
    {   // (0.17.0) the zone rule's ADR: the daily TRUE range of the full 17:00-CT session, mean of the prior 20 sessions (min 10; 5-9 used
        // and labelled when the chart holds fewer), from these same bars - the session in progress never counts
        zrule::Adr adr;
        for (size_t i = 0; i < B.size(); ++i) adr.add(zrule::sessionKey((long long)B[i].days * 86400 + B[i].endSec), B[i].h, B[i].l, B[i].c);
        const long long cur = zrule::sessionKey((long long)B.back().days * 86400 + B.back().endSec);
        int n = 0; double v = adr.adrFor(cur, n, zrule::ADR_MIN); bool shortAdr = false;
        if (!(v > 0)) { v = adr.adrFor(cur, n, 5); shortAdr = v > 0; }
        S.e.zrAdr = v > 0 ? v : -1; S.e.zrSpb = per; S.zrAdrDays = n;
        S.zrNote = v > 0 ? std::to_string(n) + " sessions" + (shortAdr ? " (short: the study uses 10+)" : "") : "under 5 complete sessions on the chart: no zone rule (no labels)";
    }
    const std::vector<klv::Level> L = klv::compute(B, per, S.rthOpen, S.rthClose, S.tick);
    S.levels.clear();
    for (size_t i = 0; i < L.size(); ++i) {
        const std::string& c = L[i].code;                                   // his trade levels only (no weekly, no hourly swings)
        if (c == "PDH" || c == "PDL" || c == "PFH" || c == "PFL" || c == "ONH" || c == "ONL" || c == "LonHI" || c == "LonLO") {
            LQState::Lvl v; if (!lqp::tryToTick(L[i].price, S.tick, v.px)) continue; v.code = c; S.levels.push_back(v);
        }
    }
    // (0.13.0, Rassul 12:02 "include the pocs, like the prior day and prior week pocs, i want to see how much liquidity there is
    // around those levels") his own IRT custom indicators, read by name (the values his DPoc / WPoc lines show): a zone on one is
    // picked and named like any key level. Missing on this chart -> not used (nothing guessed).
    {   static const char* const POC[2][2] = { { "ProfPrevDay", "DPoc" }, { "ProfPrevWeekly", "WPoc" } };
        for (int k = 0; k < 2; ++k) {
            RTARRAY a(fEmptyArray); char nm[40]; std::snprintf(nm, sizeof(nm), "%s", POC[k][0]);
            if (a.getCustomIndicator(nm, 1, true) != RTX_OK || a.count < 1) continue;
            const float v = a[(int)a.count - 1];
            if (!std::isfinite(v) || v <= 0) continue;
            LQState::Lvl p; p.code = POC[k][1]; p.px = lqp::toTick((double)v, S.tick); S.levels.push_back(p);
        }
    }
    // the average RTH range of the last 14 COMPLETE sessions (the current one excluded)
    std::map<long, std::pair<double, double>> rth;
    for (size_t i = 0; i < B.size(); ++i) {
        const svl::Cls c = svl::classify(B[i].days, B[i].endSec, per, S.rthOpen, S.rthClose);
        if (c.kind != svl::RTH) continue;
        std::map<long, std::pair<double, double>>::iterator it = rth.find(c.sess);
        if (it == rth.end()) rth[c.sess] = std::make_pair((double)B[i].h, (double)B[i].l);
        else { it->second.first = std::max(it->second.first, (double)B[i].h); it->second.second = std::min(it->second.second, (double)B[i].l); }
    }
    const svl::Cls cur = svl::classify(B.back().days, B.back().endSec, per, S.rthOpen, S.rthClose);
    double sum = 0; int n = 0;
    for (std::map<long, std::pair<double, double>>::reverse_iterator it = rth.rbegin(); it != rth.rend() && n < 14; ++it) {
        if (it->first >= cur.sess && cur.kind != svl::NONE) continue;
        sum += it->second.first - it->second.second; ++n;
    }
    S.avgRthN = n; S.avgRth = n > 0 ? sum / n : 0;
}
// the reach: 1/2 of today's expected RTH range from lsHodLod's status file (current session, hle valid, written in the last 10
// minutes), else 1/2 of the 14-session average RTH range (10+ sessions on the chart), else the engine's 6x 3-min range
void LiquidityProfile::loadReach(LQState& S) {
    S.expRange = 0;
    const std::string f = flexDir();
    if (!f.empty() && S.spb > 0) {
        const std::string p = f + "\\HodLod.status-" + S.market + "-" + std::to_string(S.spb) + ".txt";
        long long st = -1;
        if (statFile(p, &st)) {
            const bool fresh = (long long)std::time(nullptr) - st / 1000000LL < 600;   // statFile stamp = mtime x 1e6 + size
            std::ifstream in; std::string line; bool prior = true, valid = false; double rng = 0; int n = 0;
            if (openBoundedInput(p, in)) while (readBoundedLine(in, line) && ++n < 200) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line == "PRIOR,0") prior = false;
                if (line.compare(0, 4, "HLE,") == 0 && line.find(",valid") != std::string::npos) valid = true;
                const size_t k = line.find("range=");
                if (line.compare(0, 11, "HLE_VALUES,") == 0 && k != std::string::npos) { double v = 0; if (parseD(line.substr(k + 6, line.find(' ', k) - k - 6), v)) rng = v; }
            }
            if (fresh && !prior && valid && rng > 0) S.expRange = rng;
        }
    }
    lqp::Tick reach = 0;
    if (S.expRange > 0 && lqp::tryToTick(0.5 * S.expRange, S.tick, reach) && reach > 0) { S.e.P.reachTicks = reach; S.reachSrc = "1/2 expected range"; }
    else if (S.avgRthN >= 10 && S.avgRth > 0 && lqp::tryToTick(0.5 * S.avgRth, S.tick, reach) && reach > 0) { S.e.P.reachTicks = reach; S.reachSrc = "1/2 avg RTH range " + std::to_string(S.avgRthN) + "d"; }
    else { S.e.P.reachTicks = 0; S.reachSrc = "6x 3-min range"; }
}

void LiquidityProfile::updateConfirms(LQState& S) {
    S.confirms.clear(); S.lqMark.clear();
    std::map<std::string, long long> prev; prev.swap(S.lqPrev);
    if (!S.e.havePx || S.e.zones.empty()) return;
    std::vector<LQState::Lvl> all = S.levels; all.insert(all.end(), S.mqLevels.begin(), S.mqLevels.end());
    for (size_t i = 0; i < all.size(); ++i) {
        const std::string key = all[i].code + "@" + std::to_string(all[i].px);
        std::map<std::string, long long>::const_iterator pv = prev.find(key);
        const lqp::Zone* z = S.e.confirming(all[i].px, S.e.lastPx, pv == prev.end() ? 0 : pv->second);   // (0.11.4) kept to 2.1x: no flicker
        if (!z) continue;
        S.lqPrev[key] = z->id;
        std::ostringstream q; q.imbue(std::locale::classic());
        q << all[i].code << "," << (double)all[i].px * S.e.P.tick << "," << (z->side == lqp::BID ? "support" : "resistance") << ",bx " << z->bx
          << "," << (S.lqShow ? "shown" : "logged");
        S.confirms.push_back(q.str());
        if (S.lqShow) { std::string& m = S.lqMark[z->id]; if (m.empty()) m = "LQ? " + all[i].code; else if (m.find(all[i].code) == std::string::npos) m += "/" + all[i].code; }
    }
}

// the Delta Profile's absorption candidates (A? / A) of this session from its record file -> zones for the "A" reason
void LiquidityProfile::loadDelta(LQState& S) {
    const std::string f = flexDir(); if (f.empty() || S.session < 0) return;
    const std::string path = f + "\\DeltaProfile\\" + S.market + "-" + std::to_string(S.spb) + "-signals-" + std::to_string(S.session) + ".csv";
    long long st = -1; if (!statFile(path, &st)) { S.dpBid.clear(); S.dpAsk.clear(); S.e.dpZone[lqp::BID].clear(); S.e.dpZone[lqp::ASK].clear(); S.dpStamp = -1; return; }
    const long long nowCt = ctSec(currentDate());
    // (0.11.4) re-read every minute even when the file did not change: "the last hour only" was applied only when the Delta Profile
    // wrote a new line, so on a quiet stretch an A from hours ago could still start an absorption read
    if (st == S.dpStamp && !(nowCt > 0 && (nowCt - S.dpAt >= 60 || nowCt < S.dpAt))) return;
    S.dpStamp = st; S.dpAt = nowCt; S.dpBid.clear(); S.dpAsk.clear();
    std::ifstream in;                                                    // (0.13.3, audit LQP-006) bounded file / record size
    if (!openBoundedInput(path, in)) return;
    std::string line; int n = 0;
    while (readBoundedLine(in, line) && ++n < 20000) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::vector<std::string> c = split(line, '|');
        if (c.size() < 8 || (c[2] != "A" && c[2] != "A?")) continue;
        long long tf = 0; double lo = 0, hi = 0;
        if (!parseLL(c[0], 0, LLONG_MAX / 2, tf) || !parseD(c[5], lo) || !parseD(c[6], hi) || hi < lo) continue;
        if (nowCt > 0 && tf > 0 && nowCt - tf > 3600) continue;           // the last hour only
        lqp::Tick loTick = 0, hiTick = 0;
        if (!lqp::tryToTick(lo, S.e.P.tick, loTick) || !lqp::tryToTick(hi, S.e.P.tick, hiTick) || hiTick < loTick) continue;
        const std::pair<lqp::Tick, lqp::Tick> z(loTick, hiTick);
        (c[3] == "support" ? S.dpBid : S.dpAsk).push_back(z);
    }
    S.e.dpZone[lqp::BID] = S.dpBid; S.e.dpZone[lqp::ASK] = S.dpAsk;      // (0.11.0) an absorption read also starts at a Delta Profile A
}

// the 17:00 Central session: a new session starts a new record (marks of the old one are kept in their file)
void LiquidityProfile::session(LQState& S, long long t) {
    const long long sec = t / 1000; if (sec <= 0) return;
    const long long key = (sec - 17LL * 3600) / 86400LL + 1;               // 17:00 CT belongs to the next day's session
    if (key == S.session) return;
    if (S.session >= 0 && S.zrLastEndMs > 0) { S.e.zrEndSession(S.zrLastBarTime, S.zrLastEndMs); persist(S); }   // (0.17.0) i_end: open breaks count, the rest pin
    S.session = key; S.e.marks.clear(); S.marksWritten = 0; S.zrRestored = false; S.e.resolvedSeen.clear();
    const std::string d = dir(); if (d.empty()) return;
    // (0.7.0, Rassul 2026-10-11 "do you store the reason so i can see the reason when i go back on the chart") every recorded mark and
    // reason of THIS session and the 9 before it is read back, so scrolling back through earlier days shows what was drawn then (#11).
    // The price chart (e? e p? p t) keeps its own record (-chart-), the panes theirs (-panes-: Exh / Pull / the reasons).
#ifdef LQ_FLOW
    const char* kind = "-panes-";
#else
    const char* kind = "-chart-";
#endif
    for (long long k = key - 9; k <= key; ++k) {
        std::ifstream in;
        if (!openBoundedInput(d + "\\" + recordStem(S) + kind + std::to_string(k) + ".csv", in)) continue;
        std::string line; int n = 0;
        while (readBoundedLine(in, line) && ++n < 5000) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lqp::Mark m; if (lqp::Engine::parse(line, m, S.e.P.tick)) S.e.marks.push_back(m);
        }
    }
    S.marksWritten = S.e.marks.size();
#ifdef LQ_FLOW
    S.bookHist.clear(); S.winHist.clear(); S.winKeys.clear(); S.flowLines.clear(); S.finSeen = S.e.finTotal;
    for (long long k = key - 2; k <= key; ++k) {                         // (0.11.1) this session and the two before it
        std::ifstream in;
        if (!openBoundedInput(d + "\\" + recordStem(S) + "-flow-" + std::to_string(k) + ".csv", in)) continue;
        std::string line; int n = 0;
        while (readBoundedLine(in, line) && ++n < 20000) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::vector<std::string> c = split(line, '|');
            long long t = 0, t2 = 0, side = 0, pxv = 0, dp = 0; double b = 0, a = 0;
            if (c.size() == 4 && c[0] == "B" && parseLL(c[1], 1, LLONG_MAX / 2, t) && parseD(c[2], b) && parseD(c[3], a) && b >= 0 && a >= 0) S.bookHist[t] = std::make_pair(b, a);
            else if (c.size() == 6 && c[0] == "W" && parseLL(c[1], 1, LLONG_MAX / 2, t) && parseLL(c[2], 1, LLONG_MAX / 2, t2) && parseLL(c[3], 0, 1, side)
                     && parseLL(c[4], -(LLONG_MAX / 4), LLONG_MAX / 4, pxv) && parseLL(c[5], 0, 1, dp) && t2 >= t) {
                LQState::Win w; w.tA = t; w.tE = t2; w.side = (int)side; w.px = (lqp::Tick)pxv; w.dp = dp != 0;
                S.addWin(w);
            }
        }
    }
#endif
}

// (0.17.0) the zone-rule record of this instance: OPEN at an arrival, RES with the outcome (REV / CONT / PIN / CENSORED) once final
#ifdef LQ_FLOW
static const char* const ZR_KIND = "-zonerule-panes-";
#else
static const char* const ZR_KIND = "-zonerule-chart-";
#endif
// after a restart: every OPEN encounter of this session with no outcome yet is rebuilt and the closed bars since its arrival replayed
// (closes only: the same answer as if the indicator had run), so a label is never lost or written twice
void LiquidityProfile::zoneRestore(LQState& S, long count) {
    if (S.zrRestored) return;
    S.zrRestored = true;
    const std::string d = dir(); if (d.empty() || S.session < 0 || count < 3) return;
    std::ifstream in; if (!openBoundedInput(d + "\\" + recordStem(S) + ZR_KIND + std::to_string(S.session) + ".csv", in)) return;
    std::vector<std::string> open; std::string line; int n = 0;
    while (readBoundedLine(in, line) && ++n < 20000) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.compare(0, 4, "RES|") == 0) S.e.zrResolved(line); else if (line.compare(0, 5, "OPEN|") == 0) open.push_back(line);
    }
    RTARRAYI times(barDateTime); RTARRAY close(barClose);
    if (times.count < count || close.count < count) return;
    for (size_t k = 0; k < open.size(); ++k) {
        const std::vector<std::string> f = split(open[k], '|'); long long tA = 0;
        if (f.size() < 2 || !parseLL(f[1], 1, LLONG_MAX / 2, tA) || S.e.resolvedSeen.count(tA)) continue;
        int barA = -1;                                                    // the bar the arrival traded in: the first bar ending after it
        for (long i = std::max(0L, count - 2000); i < count; ++i) { const long long e = ctSec((RTDATE)times[(int)i]) * 1000LL; if (e > tA) { barA = (int)i; break; } }
        if (barA < 0 || !S.e.zrRestore(open[k], barA)) continue;
        S.zrRestoredN++;
    }
    for (long i = 0; i < count - 1; ++i) {                               // replay: zrClose only acts on the restored encounters here
        bool any = false; for (size_t k = 0; k < S.e.ztracks.size(); ++k) if (!S.e.ztracks[k].o.resolved() && S.e.ztracks[k].barA <= (int)i) any = true;
        if (!any) continue;
        const long long endMs = ctSec((RTDATE)times[(int)i]) * 1000LL; if (endMs <= 0) continue;
        S.e.zrClose((int)i, (long long)times[(int)i], endMs, (double)close[(int)i]);
    }
}

// (0.17.0) append with a header that may have GROWN: a session file started by an older version (fewer columns) gets the new header
// line once (atomic rewrite; old rows keep their fields - readers fill the new columns as empty), so no row has more fields than its header
static bool appendLinesHeader(const std::string& path, const std::string& header, const std::vector<std::string>& lines) {
    if (lines.empty()) return true;
    long long bytes = 0;
    if (statFile(path, nullptr, &bytes) && bytes > 0 && bytes < 8LL * 1024 * 1024) {
        std::ifstream in(path.c_str(), std::ios::binary); std::string first;
        if (in && std::getline(in, first)) {
            if (!first.empty() && first.back() == '\r') first.pop_back();
            if (first != header && header.compare(0, first.size() + 1, first + "|") == 0) {
                std::string rest((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()); in.close();
                if (!publishFile(path, header + "\n" + rest)) return false;
            }
        }
    }
    return appendLines(path, header, lines);
}

void LiquidityProfile::persist(LQState& S) {
    // (0.4.0) lsLiquidityFlow records its pane signals (<MKT>-<spb>-panes-<session>.csv, re-read after a restart: no repaint);
    // lsLiquidityProfile records every attempt for the nightly study (<MKT>-<spb>-attempts-<session>.csv)
    const std::string d = dir(); if (d.empty() || S.session < 0) { S.e.attemptLog.clear(); S.e.makeupLog.clear(); return; }
    makeDir(flexDir()); makeDir(d);
    if (!S.e.zoneLog.empty()) {                                          // (0.17.0) the zone-rule record (each instance its own file)
        if (appendLines(d + "\\" + recordStem(S) + ZR_KIND + std::to_string(S.session) + ".csv", lqp::Engine::zoneHeader(), S.e.zoneLog)) { S.e.zoneLog.clear(); S.zrWrites++; }
        else { S.zrFails++; if (S.e.zoneLog.size() > 2000) S.e.zoneLog.erase(S.e.zoneLog.begin(), S.e.zoneLog.begin() + 1000); }
    }
#ifdef LQ_FLOW
    S.e.attemptLog.clear(); S.e.makeupLog.clear();
    if (S.marksWritten < S.e.marks.size()) {
        std::vector<std::string> lines;
        for (size_t i = S.marksWritten; i < S.e.marks.size(); ++i) lines.push_back(lqp::Engine::line(S.e.marks[i], S.e.P.tick));
        if (appendLines(d + "\\" + recordStem(S) + "-panes-" + std::to_string(S.session) + ".csv", lqp::Engine::header(), lines)) { S.marksWritten = S.e.marks.size(); S.recWrites++; }
        else S.recFails++;                                               // kept and retried at the next bar close (#6)
    }
    if (!S.flowLines.empty()) {                                          // (0.11.1) the pane's book and windows record
        if (appendLines(d + "\\" + recordStem(S) + "-flow-" + std::to_string(S.session) + ".csv", "kind|bar_time|...", S.flowLines)) S.flowLines.clear();
        else S.recFails++;
    }
#else
    // (0.7.0) the price chart records its letters (e? e p? p t) - re-read after a restart, never repainted
    if (S.marksWritten < S.e.marks.size()) {
        std::vector<std::string> lines;
        for (size_t i = S.marksWritten; i < S.e.marks.size(); ++i) lines.push_back(lqp::Engine::line(S.e.marks[i], S.e.P.tick));
        if (appendLines(d + "\\" + recordStem(S) + "-chart-" + std::to_string(S.session) + ".csv", lqp::Engine::header(), lines)) { S.marksWritten = S.e.marks.size(); S.recWrites++; }
        else S.recFails++;
    }
    if (!S.e.attemptLog.empty()) {
        if (appendLinesHeader(d + "\\" + recordStem(S) + "-attempts-" + std::to_string(S.session) + ".csv", lqp::Engine::attemptHeader(), S.e.attemptLog)) S.e.attemptLog.clear();
        else S.recFails++;
    }
    if (!S.e.makeupLog.empty()) {                                        // (0.15.0) each decision's zone makeup, hits and rebuilds
        if (appendLines(d + "\\" + recordStem(S) + "-makeup-" + std::to_string(S.session) + ".csv", lqp::Engine::makeupHeader(), S.e.makeupLog)) S.e.makeupLog.clear();
        else S.recFails++;
    }
#endif
}

void LiquidityProfile::status(LQState& S, long long now) {
    if (S.lastBuild >= 0 && now - S.lastBuild < 1000 && now >= S.lastBuild) return;   // #32: built at most once a second
    S.lastBuild = now;
    auto xfmt = [](double x) { char b[32]; std::snprintf(b, sizeof(b), "%.1fx", x); return std::string(b); };   // (0.14.0) "1.5x"
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << LQ_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nTICK," << S.tick
      << "\nDEPTH," << (S.mdBlocked ? "blocked" : S.mdDead ? "fault" : !S.md ? "not opened" : S.depthAvail ? "available" : "not available")
      << ",max " << S.depthMax << ",levels " << S.depthLevels << ",reads " << S.depthReads << ",books " << S.e.books << ",bad " << S.e.badBooks << "," << S.e.why
      << "\nCOLUMN,width " << S.width << ",background " << (S.bg ? "on" : "off") << ",right columns end price at " << S.colPriceRight
      << "\nLIQ_LEVELS,rows " << S.lvlRows << ",writes " << S.lvlWrites
      << "\nBANNER," << (S.trBanner ? "Tape Reader banner on this chart: the warning is shown there" : "no Tape Reader banner: the warning is drawn here") << ",handed " << S.handed << ",verdict " << S.verdict << " (agree " << S.verdictScore << "/3)" << ",label offset " << S.labDx << "," << S.labDy << (S.sizeWhy.empty() ? std::string() : "," + S.sizeWhy)
      << "\nTRADES," << S.e.trades << "\nZONE_HW," << S.e.hw << ",atr " << S.atr << ",slot " << S.slot << ",normal " << S.e.normal << ",norms " << (S.e.norms.loaded ? "file" : "session") << "\nPARAMS," << S.params
      << "\nZONES," << S.e.zones.size() << "\nATTEMPTS," << S.e.attempts.size() << "\nMARKS," << S.e.marks.size() << ",written " << S.marksWritten
      << ",writes " << S.recWrites << ",fails " << S.recFails << "\nDELTA_A," << S.dpBid.size() << "," << S.dpAsk.size()
      << "\nSESSION," << S.session << "\nSTATE," << S.state
      << "\nZONE_RULE," << zrule::VERSION << ",adr " << S.e.zrAdr << " (" << S.zrNote << "),zone +/- " << zrule::ZONE_W << " ADR,held/broke " << zrule::RES_X << " ADR,reclaim 15 min"
      << ",open " << S.e.ztracks.size() << ",opened " << S.e.zrOpened << ",held " << S.e.zrRev << ",broke " << S.e.zrCont << ",pinned " << S.e.zrPin << ",censored " << S.e.zrCens
      << ",skipped (debounce) " << S.e.zrSkipped << ",no_adr " << S.e.zrNoAdr << ",restored " << S.zrRestoredN << ",writes " << S.zrWrites << ",fails " << S.zrFails << "\n";
    {   // (0.6.0) the pane statistics now and the lines in use (value, data / default)
        lqp::Stats st; const bool have = S.e.latest(S.e.bookT, st);
        auto ln = [](const LQState::Line& l) { std::ostringstream q; q.imbue(std::locale::classic()); q << l.v << (l.data ? " data" : " default"); return q.str(); };
        o << "STATS," << (have ? (st.side == lqp::BID ? "support" : "resistance") : "none") << ",attack " << (have ? st.attack : -1) << " (" << ln(S.lAttack)
          << "),eaten " << (have ? st.eaten : -1) << " (" << ln(S.lEaten) << "),exh " << (have ? st.exh : -1) << " (" << ln(S.lExh) << "),pulled "
          << (have ? st.pulled : -1) << " (" << ln(S.lPulled) << "),rebuilt " << (have ? st.rebuilt : -1) << " (" << ln(S.lRebuilt) << "),counter "
          << (have ? st.counter : -1) << " (" << ln(S.lCounter) << "),bars_known "
          << S.e.barNorm.size() << "\n";
    }
    o << "REACH," << S.reachSrc << ",ticks " << S.e.P.reachTicks << ",expected " << S.expRange << ",avg_rth " << S.avgRth << ",sessions " << S.avgRthN
      << "\nNEAR,ticks " << S.e.nearTicks << ",show " << xfmt(S.e.P.nearShowX) << ",keep " << xfmt(S.e.P.nearKeepX)   // (0.14.0)
      << "\nLEVELS," << S.levels.size() << " key," << S.mqLevels.size() << " gamma1\nLQ," << (S.lqShow ? "shown" : "logged") << "," << S.confirms.size() << "\n";
    for (size_t i = 0; i < S.confirms.size(); ++i) o << "CONFIRM," << S.confirms[i] << "\n";
#ifndef LQ_FLOW
    {   // (0.15.0) the order feed and each near zone's makeup (what its hover shows)
        const long long st = steadyMs(); const std::string fs = S.feed.stateText(st);
        o << "MBO," << (fs.empty() ? S.feedState : fs) << ",via " << S.feed.via << ",orders " << S.ob.book.size() << ",events " << S.feed.events << ",faults " << S.feed.faults
          << ",garbage " << S.ob.garbage << ",backlog " << S.feed.longestBacklog << ",hover " << S.hovState << "\n";
        // (0.18.0) the big resting order lines
        o << "BIG_ORDERS," << bol::VERSION << "," << (S.boSet.on ? "on" : "off") << "," << S.boState << ",settings " << S.boWhy << ",floor " << S.bo.P.floor
          << ",min_lots_now " << S.bo.threshold() << ",top_0.1pct " << (S.bo.hist.n >= S.bo.P.pctMinN ? S.bo.hist.pct(S.bo.P.pct) : 0) << " (orders " << S.bo.hist.n << ")"
          << ",reach " << S.bo.P.reachAtr << " ATR = " << S.bo.reach << " ticks (ATR14 " << S.bo.atrPrice << "),keep " << S.bo.P.keep << ",font " << S.boSet.font
          << ",live " << (S.bo.live[0] >= 0 ? "bid" : "-") << "/" << (S.bo.live[1] >= 0 ? "offer" : "-") << ",lines " << S.bo.lineN[0] << "/" << S.bo.lineN[1]
          << ",drawn " << S.boDrawn << ",labels " << S.boLabels << ",events " << S.bo.events << ",not_known " << S.bo.unknown << ",lost_ends " << S.bo.lostEnds
          << ",orders " << S.bo.book.size() << ",restored " << S.boRestored << " (bad rows " << S.boBad << "),writes " << S.boWrites << ",fails " << S.boFails
          << ",dropped " << S.boDropped << ",hover " << S.boHovState << "\n";
        for (size_t i = 0; i < S.e.zones.size(); ++i) {
            const lqp::Zone& z = S.e.zones[i]; if (!z.shown || !z.near) continue;
            const lqp::HoverIn in = hoverIn(S, z, std::string()); const lqp::Makeup& m = in.mk;
            const lqp::Came c = lqp::cameOf(in.nearBase, in.lots, in.band);
            o << "MAKEUP," << (z.side == lqp::BID ? "support" : "resistance") << "," << (double)z.lo * S.e.P.tick << "," << (double)z.hi * S.e.P.tick << ",orders " << m.orders
              << ",lots " << m.lots << ",top " << m.topPct() << "%,coverage " << m.coverage << (m.ok ? ",shown" : ",not shown") << ",one_order " << (lqp::oneOrder(m, in.oneShare) ? 1 : 0)
              << ",came_base " << in.nearBase << ",came " << (c.state == lqp::CAME_NONE ? "none" : lqp::cameWord(c.state)) << " " << c.pct << "%"
              << ",in_zone " << (in.inZone ? 1 : 0) << ",hit " << in.hit << ",rebuilt " << in.rebuilt << "\n";
        }
    }
#endif
    for (size_t i = 0; i < S.e.zones.size(); ++i) {
        const lqp::Zone& z = S.e.zones[i];
        o << "ZONE,bx " << z.bx << ",rank " << z.rank << ",behind " << z.behindN << "," << (z.shown ? "shown" : "hidden") << "," << (z.side == lqp::BID ? "support" : "resistance") << "," << (double)z.lo * S.e.P.tick << "," << (double)z.hi * S.e.P.tick
          << "," << z.size << ",x " << S.e.xNormal(z.size) << ",age_s " << (S.e.bookT - z.born) / 1000 << ",old " << S.e.zoneOld(z.side, z.lo, z.hi, S.e.bookT) << (z.near ? ",near" : ",far") << "\n";   // (0.14.0)
    }
    const std::string text = o.str();
    if (text == S.lastStatusText && S.lastStatus >= 0 && now - S.lastStatus < STATUS_EVERY_MS) return;   // #31: on change or every 30 s
    const std::string f = flexDir(); if (f.empty()) return;
#ifdef LQ_FLOW
    const char* kind = "LiquidityFlow";
#else
    const char* kind = "LiquidityProfile";
#endif
    if (publishFile(f + "\\" + kind + ".status-" + stem(S) + ".txt", text)) { S.lastStatusText = text; S.lastStatus = now; }
}

// (0.13.0, Rassul 2026-10-12 11:59 "it shows so much liquidity, its difficult to know which one is important" -> mockup
// liquidity-clarity, "ok") of the shown zones only these are drawn / published: the biggest resting size on each side (bids below,
// offers above) and every zone sitting ON a key / gamma level (the level +/- half the zone's band) - named with the level.
// (0.14.0, Rassul 2026-10-13 "i want to see liquidity when it is close so i can see if there is any absorbtion going on") + every
// shown NEAR zone (within 1x the 3-min range of price, shown at 1.5x / kept to 1.2x), drawn solid like the strongest; far zones keep
// the 0.13.0 rule (2.0x, rank 1, kept to 1.7x). Research basis: zones of 1.5-2.0x held 34% vs 35% at 2-3x, under 1x 23%.
struct LqPick { size_t idx; bool top; std::string names; };
static std::vector<LqPick> lqPicks(const LQState& S)
{
    std::vector<LqPick> out;
    const std::vector<size_t> order = S.e.drawOrder();
    long bestB = -1, bestA = -1;
    for (size_t k = 0; k < order.size(); ++k) {
        const lqp::Zone& z = S.e.zones[order[k]];
        long& b = z.side == lqp::BID ? bestB : bestA;
        if (b < 0 || z.size > S.e.zones[(size_t)b].size) b = (long)order[k];
    }
    std::vector<LQState::Lvl> all = S.levels; all.insert(all.end(), S.mqLevels.begin(), S.mqLevels.end());
    // (0.13.1, Rassul 13:27 "they dont match") each level belongs to ONE zone - the one whose band holds its price, else the nearest
    // within half a band - the same rule the Dealer Profile's badge uses, so the column and the badge name the same contracts
    std::map<size_t, std::string> owner;
    for (size_t j = 0; j < all.size(); ++j) {
        long best = -1; long long bestD = 0;
        for (size_t k = 0; k < order.size(); ++k) {
            const lqp::Zone& z = S.e.zones[order[k]]; const lqp::Tick pad = (z.hi - z.lo) / 2;
            if (all[j].px < z.lo - pad || all[j].px > z.hi + pad) continue;
            const long long d = all[j].px < z.lo ? (long long)(z.lo - all[j].px) : all[j].px > z.hi ? (long long)(all[j].px - z.hi) : 0;
            const long long dc = d * 1000 + std::llabs((long long)(2 * all[j].px - z.lo - z.hi));   // inside first, then the nearer centre
            if (best < 0 || dc < bestD) { best = (long)order[k]; bestD = dc; }
        }
        if (best >= 0 && owner[(size_t)best].find(all[j].code) == std::string::npos) owner[(size_t)best] += (owner[(size_t)best].empty() ? "" : "/") + all[j].code;
    }
    for (size_t k = 0; k < order.size(); ++k) {
        const size_t i = order[k];
        const std::string names = owner.count(i) ? owner[i] : std::string();
        const bool top = (long)i == bestB || (long)i == bestA || S.e.zones[i].near;   // (0.14.0) a shown near zone is drawn solid
        if (top || !names.empty()) out.push_back(LqPick{ i, top, names });
    }
    return out;
}

// (0.12.0, Rassul 2026-10-12 "modify the level badges to indicate if there is liquidity at those levels ... even if you have CR or
// other gamma levels indicate Liq") the resting-liquidity zones near price, for the Dealer Profile's level badges:
// LRA-LiqZones-<MKT>.csv = "LIQZ,1,<unix seconds>,<tick>" then "Z,<S|R>,<lo>,<hi>,<bx>,<contracts>" (S = bids / support,
// R = offers / resistance, bx = the zone's size x the 8 same-width bands behind it - the same prominence the zones are drawn by,
// contracts = what rests in the zone, Rassul 10:23 "also add the number of contracts"). Zones of 1.0x or more (the reader decides). Written when the zones change, else every 5 s so the reader can tell it is live (it ignores a file 20 s old).
void LiquidityProfile::writeLiqZones(LQState& S) {
#ifndef LQ_FLOW
    if (S.market.empty() || !(S.e.P.tick > 0)) return;
    std::ostringstream b; b.imbue(std::locale::classic()); b.precision(10);
    const std::vector<LqPick> picks = lqPicks(S);                      // (0.13.0) the same zones the column draws
    for (size_t pi = 0; pi < picks.size(); ++pi) {
        const lqp::Zone& z = S.e.zones[picks[pi].idx];
        if (!(z.bx > 0)) continue;
        b << "Z," << (z.side == lqp::BID ? "S" : "R") << "," << (double)z.lo * S.e.P.tick << "," << (double)z.hi * S.e.P.tick << "," << z.bx << "," << (long long)std::llround((double)z.size) << "\n";
    }
    const std::string body = b.str() + "#" + std::to_string(S.colRight);
    const long long st = steadyMs();
    if (body == S.liqBody && S.liqAt >= 0 && st - S.liqAt < 5000) return;
    const std::string f = flexDir(); if (f.empty()) return;
    std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
    o << "LIQZ,1," << (long long)time(nullptr) << "," << S.e.P.tick << "," << S.colRight << "\n" << body.substr(0, body.rfind('#'));   // (0.13.2) + the column's width
    if (publishFile(f + "\\LRA-LiqZones-" + S.market + ".csv", o.str())) { S.liqBody = body; S.liqAt = st; }
#else
    (void)S;
#endif
}

// (0.16.0, Rassul 11:39 CT - level badges with liquidity, design B) LRA-LiqLevels-<MKT>.csv for the Dealer Profile's Levels column:
// "LIQL,1,<unix>,<tick>,<confirmX>,<confirmKeepX>,<hw>" then "L,<tick index>,<S|R>,<contracts>,<bx>" for every price within the
// reach (both sides, capped at 600 ticks a side) that holds size - S = bids below price, R = offers above; bx = the band x the book
// behind it (-1 = nothing behind to compare). Computed at most once a second; rewritten when it changed, else every 5 s (the reader
// treats a file over 20 s old as no data).
void LiquidityProfile::writeLiqLevels(LQState& S) {
#ifndef LQ_FLOW
    if (S.market.empty() || !(S.e.P.tick > 0) || !S.e.havePx) return;
    const long long st = steadyMs();
    if (S.lvlAt >= 0 && st >= S.lvlAt && st - S.lvlAt < 1000) return;
    const lqp::Tick hw = (lqp::Tick)std::max(1, S.e.hw);
    lqp::Tick reach = S.e.P.reachTicks > 0 ? std::max((lqp::Tick)(10 * hw), S.e.P.reachTicks) : (lqp::Tick)(10 * hw);
    if (reach > 600) reach = 600;
    std::ostringstream b; b.imbue(std::locale::classic());
    int rows = 0;
    for (lqp::Tick px = S.e.lastPx - reach; px <= S.e.lastPx + reach; ++px) {
        double lots = 0, bx = -1; int side = lqp::BID;
        if (!S.e.levelLiq(px, lots, bx, side)) continue;
        // (0.20.1, Rassul 08:51 "the liquidity numbers on the key levels are not in sync with the liquidity profile") a price inside a
        // zone the column DRAWS takes that zone's own lots and x - the badge and the column show the same number for the same level
        for (size_t zi = 0; zi < S.e.zones.size(); ++zi) { const lqp::Zone& z = S.e.zones[zi];
            if (z.shown && z.side == side && px >= z.lo && px <= z.hi && z.size > 0) { lots = z.size; bx = z.bx > 0 ? z.bx : bx; break; } }
        const long long n = (long long)std::llround(lots); if (n < 1) continue;
        char row[96]; std::snprintf(row, sizeof(row), "L,%lld,%s,%lld,%.3f\n", (long long)px, side == lqp::BID ? "S" : "R", n, bx > 0 ? bx : -1.0);
        b << row; ++rows;
    }
    const std::string body = b.str();
    if (body == S.lvlBody && S.lvlAt >= 0 && st - S.lvlAt < 5000) return;
    S.lvlAt = st;
    const std::string f = flexDir(); if (f.empty()) return;
    std::ostringstream o; o.imbue(std::locale::classic()); o.precision(10);
    o << "LIQL,1," << (long long)time(nullptr) << "," << S.e.P.tick << "," << S.e.P.confirmX << "," << S.e.P.confirmKeepX << "," << (long long)hw << "\n" << body;
    if (publishFile(f + "\\LRA-LiqLevels-" + S.market + ".csv", o.str())) { S.lvlBody = body; S.lvlRows = rows; ++S.lvlWrites; }
#else
    (void)S;
#endif
}

#ifndef LQ_FLOW
// (0.15.0) the chart's order-by-order feed, opened once the depth works (the lsMboCheck / OrderFlowFeed pattern: one DEPTH_BY_ORDER per
// chart kept alive, opened from calc / draw only behind a crash-guard file, drained <= 5,000 events / ~8 ms per call). Each event
// first goes to the attempts (hits / rebuilds, open attempts only), then to the order book. The open attempts' makeup is refreshed
// every call, so the one recorded at the decision is the last one before it.
void LiquidityProfile::orders(LQState& S, long long st) {
    if (!S.e.haveBook || !S.depthAvail || S.symbol.empty()) return;
    const std::string d = dir();
    if (!d.empty()) { makeDir(flexDir()); makeDir(d); }
    S.feed.open(S.symbol, d.empty() ? std::string() : d + "\\_opening-dbo-" + stem(S) + ".txt", st, S.feedState);
    const double tick = S.e.P.tick;
    S.feed.drain(st, off::steadyMs, [&](const auto& o) {
        const off::Ev ev = off::evOf(o, tick, st);
        if (ev.px > 0 && ev.action >= off::NEW && ev.action <= off::FILL) {
            std::unordered_map<long long, off::Engine::Ord>::const_iterator it = S.ob.book.find(ev.id);
            const bool had = it != S.ob.book.end() && it->second.known;
            const int side = it != S.ob.book.end() && ev.action != off::NEW ? it->second.side : ev.side;
            S.e.mboEvent(ev.action, side, ev.px, ev.size, had, had ? it->second.px : 0, had ? it->second.size : 0);
        }
        S.ob.apply(S.obAcc, ev);
        if (S.boSet.on) {                                               // (0.18.0) the same event to the big-order lines, on the Central clock
            long long ms = 0;
            if (ev.stamp > 0 && ev.stamp == S.boLastStamp) ms = S.boLastStampMs;
            else {
                ms = ev.stamp > 0 ? ctSec((RTDATE)ev.stamp) * 1000LL : 0;
                if (ms > 0) { S.boLastStamp = ev.stamp; S.boLastStampMs = ms; }
                else ms = S.haveOffset ? st + S.ctOffset : 0;
            }
            S.bo.onEvent(ev.action, ev.side, ev.px, ev.size, ev.id, ms);
        }
    });
    if (S.obClearMs < 0 || st - S.obClearMs >= 60000 || st < S.obClearMs) { S.obAcc = off::Acc(); S.obClearMs = st; }   // bounded scratch
    for (size_t i = 0; i < S.e.attempts.size(); ++i) {
        lqp::Attempt& a = S.e.attempts[i]; if (a.done) continue;
        a.mk = lqp::makeupOf(S.ob.book, a.side, a.lo, a.hi, S.e.band(a.side, a.lo, a.hi), S.e.P.coverMin, S.e.P.coverMax); a.mkSet = true;
    }
    // the mouse left the chart window: the hover goes (as the order-flow panes' HoverTrack)
    if (S.hovId != 0 && S.hovWnd) { void* w = off::cursorWindow(); if (w && w != S.hovWnd) { S.hovId = 0; S.hovWnd = nullptr; invalidateChart(false); } }
    if (S.boHovKey != 0 && S.boHovWnd) { void* w = off::cursorWindow(); if (w && w != S.boHovWnd) { S.boHovKey = 0; S.boHovWnd = nullptr; invalidateChart(false); } }   // (0.18.0)
}
#endif

#ifndef LQ_FLOW
// ---- (0.18.0, Rassul approved 22:11 CT mockup v2) THE BIGGEST RESTING ORDER LINES ----------------------------------------------------
// The engine (BigOrderLines.h, tested on its own and replayed on his recorded order files) is fed by the SAME order feed as the hover
// (orders()); here: its per-market settings file, the session record (append-only S / E / H rows, read back after a restart so a line
// never changes), the 14-bar ATR of the chart's closed bars, every bar's end time, the depth-book check and the drawing / hover.
void LiquidityProfile::loadBig(LQState& S) {
    const long long now = steadyMs();
    if (S.boMkt == S.market && S.boCheckMs >= 0 && now >= S.boCheckMs && now - S.boCheckMs < 1000) return;   // at most once a second
    S.boCheckMs = now;
    const std::string dirp = flexDir(); if (dirp.empty() || S.market.empty()) return;
    const std::string p = dirp + "\\LiquidityProfile.bigorders-" + S.market + ".txt";
    long long t = -1, n = -1; if (!statSub(p, t, n)) { t = -1; n = -1; }
    if (S.boMkt == S.market && t == S.boStampT && n == S.boStampN) return;
    const bool newMkt = S.boMkt != S.market;
    S.boMkt = S.market; S.boStampT = t; S.boStampN = n;
    if (t < 0) { S.boSet = bol::Settings(); S.boSet.ok = true; S.boWhy = "no file: defaults"; }
    else { const BigFile z = readBig(p); if (z.ok) { S.boSet = z.s; S.boWhy = "file"; } else { S.boWhy = z.why; if (newMkt) { S.boSet = bol::Settings(); S.boSet.ok = true; } } }
    S.bo.setFloor(S.boSet.minLots > 0 ? S.boSet.minLots : 5);   // (0.19.2) safety minimum only; the size tunes itself in boAfter
    S.bo.P.reachAtr = std::max(bol::REACH_MIN, std::min(bol::REACH_MAX, S.boSet.reach10 / 10.0)); S.bo.setAtr(S.bo.atrPrice);
    S.bo.P.keep = std::max(bol::KEEP_MIN, std::min(bol::KEEP_MAX, S.boSet.keep));
}
void LiquidityProfile::boFlush(LQState& S, bool force) {
    const long long st = steadyMs();
    if (!force && S.boFlushMs >= 0 && st >= S.boFlushMs && st - S.boFlushMs < 1000) return;   // buffered: once a second
    S.boFlushMs = st;
    if ((S.bo.live[0] >= 0 || S.bo.live[1] >= 0) && S.bo.lastT > 0 && S.bo.lastT - S.boLastH >= 30000) {   // a heartbeat while a line rests
        S.bo.rows.push_back(bol::Engine::hRow(S.bo.lastT)); S.boLastH = S.bo.lastT; }
    if (S.bo.rows.empty()) return;
    const std::string path = boPath(S); if (path.empty()) return;
    makeDir(flexDir()); makeDir(dir());
    if (appendLines(path, bol::Engine::header(), S.bo.rows)) { S.bo.rows.clear(); S.boWrites++; }     // #6: cleared only after a good write
    else { S.boFails++; if (S.bo.rows.size() > 5000) { S.bo.rows.erase(S.bo.rows.begin(), S.bo.rows.begin() + 1000); S.boDropped += 1000; } }
}
void LiquidityProfile::boSessionCheck(LQState& S, long long t) {
    if (S.session < 0 || S.session == S.boSession || S.symbol.empty()) return;
    if (S.boSession >= 0) { S.bo.endSession(t > 0 ? t : S.bo.lastT); boFlush(S, true); S.bo.clearSession(); S.boLastH = 0; }
    S.boSession = S.session;
    std::vector<std::string> rows; std::ifstream in;
    if (openBoundedInput(boPath(S), in)) { std::string line; int n = 0; while (readBoundedLine(in, line) && ++n < 200000) rows.push_back(line); }
    long long bad = 0; S.boRestored = (long long)S.bo.restore(rows, &bad); S.boBad = bad;
    S.bo.openedMs = t;
    boFlush(S, true);                                               // the RESTART ends of lines that rested while IRT was closed
}
void LiquidityProfile::boAfter(LQState& S, long count, long long st) {
    if (count < 2) return;
    RTARRAYI times(barDateTime); RTARRAY close(barClose), high(barHigh), low(barLow);
    if (times.count < count || close.count < count || high.count < count || low.count < count) return;
    // every bar's end (Central ms): appended as bars come; a reload / shorter chart starts over; the forming bar re-read each time
    if ((long)S.barEndMs.size() > count) S.barEndMs.clear();
    if (!S.barEndMs.empty()) S.barEndMs.pop_back();                 // the forming bar's entry is re-read
    for (long i = (long)S.barEndMs.size(); i < count; ++i) S.barEndMs.push_back(ctSec((RTDATE)times[(int)i]) * 1000LL);
    if (S.boAtrCount != count) {                                    // the 14-bar ATR of the chart's own CLOSED bars (the forming one never)
        S.boAtrCount = count; std::vector<double> H, L, C;
        for (long i = std::max(0L, count - 16); i <= count - 2; ++i) { H.push_back((double)high[(int)i]); L.push_back((double)low[(int)i]); C.push_back((double)close[(int)i]); }
        S.bo.setAtr(bol::atr14(H, L, C));
    }
    if (!S.boSet.on) { S.boState = "off (setting)"; boFlush(S, false); return; }
    { const double c = (double)close[(int)count - 1]; lqp::Tick tk = 0; if (lqp::finite(c) && c > 0 && lqp::tryToTick(c, S.e.P.tick, tk)) S.bo.setPrice(tk, false); }
    const long long t = std::max(S.bo.lastT, nowMs(S));            // the feed's time, carried on by the clock while it is quiet
    S.bo.evaluate(t);                                               // the dwell / leave steps also move on a quiet feed
    if (S.e.haveBook && S.depthAvail) {                             // the depth book shows less than the line's order at its price for 5 s: gone
        for (int s = 0; s < 2; ++s) {
            if (S.bo.live[s] < 0) continue;
            const bol::Line& L = S.bo.lines[(size_t)S.bo.live[s]];
            const std::map<lqp::Tick, double>& bk = S.e.book[s == bol::BID ? lqp::BID : lqp::ASK];
            const bool known = !bk.empty() && L.px >= bk.begin()->first && L.px <= bk.rbegin()->first;
            const std::map<lqp::Tick, double>::const_iterator it = bk.find(L.px);
            S.bo.checkBook(s, L.px, it == bk.end() ? 0.0 : it->second, known, st, t);
        }
    }
    // (0.19.0) stacking / pulling at each live order's price: the NET change of the displayed size over the last 30 s;
    // inside a dead band (10% of the size or 5 lots) = flat (no sign) - a 100 pulled and 99 re-added is flat, not pulling
    for (int s = 0; s < 2; ++s) {
        if (S.bo.live[s] < 0 || !S.e.haveBook) { S.boHist[s].clear(); S.boHistPx[s] = -1; S.boSign[s] = 0; S.boSize[s] = 0; continue; }
        const bol::Line& L = S.bo.lines[(size_t)S.bo.live[s]];
        const std::map<lqp::Tick, double>& bk = S.e.book[s == bol::BID ? lqp::BID : lqp::ASK];
        const std::map<lqp::Tick, double>::const_iterator it = bk.find(L.px);
        const double size = it == bk.end() ? 0.0 : it->second;
        if (S.boHistPx[s] != L.px) { S.boHist[s].clear(); S.boHistPx[s] = L.px; }
        if (S.boHist[s].empty() || S.boHist[s].back().first < t) S.boHist[s].push_back(std::make_pair(t, size)); else S.boHist[s].back().second = size;
        while (S.boHist[s].size() > 1 && S.boHist[s][1].first <= t - 30000) S.boHist[s].pop_front();
        const double net = size - S.boHist[s].front().second, band = std::max(5.0, 0.10 * size);
        S.boSign[s] = net > band ? 1 : net < -band ? -1 : 0; S.boSize[s] = size;
    }
    // (0.19.0) "large" = both tests: the market floor (nightly hourly norms) AND >= 3x the median displayed size within reach
    if (S.e.haveBook && S.bo.havePx && S.bo.reach > 0) {
        std::vector<double> v;
        for (int sd = 0; sd < 2; ++sd) for (std::map<lqp::Tick, double>::const_iterator it = S.e.book[sd].begin(); it != S.e.book[sd].end(); ++it)
            if (it->second > 0 && std::llabs((long long)(it->first - S.bo.px)) <= (long long)S.bo.reach) v.push_back(it->second);
        if (v.size() >= 5) { std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end()); S.bo.depthMin = (long long)std::llround(3.0 * v[v.size() / 2]); }
    } else S.bo.depthMin = 0;
    S.boDepthMin = S.bo.depthMin;
    {   const long long sm = steadyMs();
        if (S.boNormsMs < 0 || sm - S.boNormsMs > 60000) {           // LRA-BigOrderNorms-<MKT>.csv: "hour,floor" per Central hour (nightly)
            S.boNormsMs = sm; S.boNormsOk = false; for (int h = 0; h < 24; ++h) S.boNorms[h] = 0;
            const std::string dirp = flexDir();
            if (!dirp.empty() && !S.market.empty()) {
                std::ifstream in(dirp + "\\LRA-BigOrderNorms-" + S.market + ".csv"); std::string ln; int n = 0;
                while (n < 200 && std::getline(in, ln)) { ++n; int h = -1; long long fl = 0;
                    if (std::sscanf(ln.c_str(), "%d,%lld", &h, &fl) == 2 && h >= 0 && h < 24 && fl > 0 && fl <= bol::FLOOR_MAX) { S.boNorms[h] = fl; S.boNormsOk = true; } }
            }
        }
        if (S.boSet.minLots <= 0) {                                    // his own "min lots" setting always wins
            const int hr = (int)((t / 3600000LL) % 24);
            S.bo.P.roll = true; S.bo.setFloor(5);                      // (0.19.2) only a safety minimum; the size tunes itself (BigOrderLines 1.2.0)
            S.bo.prior = std::max(bol::floorFor(S.market), S.boNormsOk && hr >= 0 ? S.boNorms[hr] : 0LL);   // cold start: nightly norm / market default
        } else { S.bo.P.roll = false; S.bo.setFloor(S.boSet.minLots); }
        S.bo.roll(t);
    }
    S.boState = S.bo.reach <= 0 ? "waiting for 15 closed bars (ATR)" : !S.bo.havePx ? "waiting for a price" : "ready";
    boFlush(S, false);
}
void LiquidityProfile::renderBig(LQState& S, int paneLeft, int paneRight, int top, int bottom, int lastBar, int lastX) {
    // (0.19.0, Rassul approved 2026-10-15 mockup v3 + "like the 96") on the chart: at most 2 thin lines = the LIVE biggest bid / ask,
    // thicker while size is stacked, thin + dim while it is pulled, no labels / dots / X / arrows. In the BIG column (first after the
    // last bar): the live order as a solid box "+200" / "-28" / "200" (flat) centred-left, and the orders FILLED in the last 60 min as
    // small right-aligned "\x95 3\xD7 410" (merged by price, at most 5 per side, dimmer after 30 min). Colour = meaning: bid stacked or
    // ask pulled = green, bid pulled or ask stacked = red; flat = the side's own colour (bid green, ask red).
    S.boHits.clear(); S.boDrawn = 0; S.boLabels = 0; S.boExec = 0;
    if (!S.boSet.on) return;
    const double tick = S.e.P.tick; if (!(tick > 0)) return;
    const int xMin = paneLeft + 1 + std::max(S.width, S.colRight) + 6;
    const int xMax = std::min(lastX, S.colPriceRight > 0 ? S.colPriceRight - 4 : paneRight - 2);
    S.boXMin = xMin; S.boXMax = xMax; S.boTop = top; S.boBot = bottom;
    auto xOfBar = [&](int b) { PNT p; p.set(b, 0.0f, kBarCenter); return (int)p.h; };
    auto yOfPx = [&](long long t) { PNT p; p.set(lastBar, (float)((double)t * tick)); return (int)p.v; };
    auto stroke = [&](int x0, int y0, int x1, int y1) { PNT a; a.set(0, 0.0f); a.h = coord(x0); a.v = coord(y0); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(x1); b.v = coord(y1); b.drawLineTo(); };
    static const COLOR C_BUY_DIM = 0x00157A3D, C_SELL_DIM = 0x00933030, C_PINK = 0x00FECACA, C_MINT = 0x00BBF7D0;
    auto meaning = [&](int side, int sign) -> bool { return sign == 0 ? side == bol::BID : (side == bol::BID) == (sign > 0); };   // true = bullish (green)
    const long long midT = S.bo.havePx ? S.bo.px : 0;
    const double pxPerTick = std::fabs((double)(yOfPx(midT) - yOfPx(midT + 20))) / 20.0;
    const long long tol = std::max(1LL, (long long)std::ceil(5.0 / std::max(0.01, pxPerTick)));
    // 1) the live lines
    for (int s = 0; s < 2; ++s) {
        if (S.bo.live[s] < 0) continue;
        const size_t k = (size_t)S.bo.live[s]; const bol::Line L = S.bo.now(k);
        const int b0 = std::max(0, std::min(lastBar, bol::barOf(S.barEndMs, L.tStart)));
        const int y = yOfPx(L.px); if (y < top || y > bottom) continue;
        const int x1 = xMax, x0 = std::max(xMin, std::min(xOfBar(b0), x1 - 16)); if (x1 - x0 < 4) continue;   // (0.22.3) an order placed in the current bar shows at once (a 16 px stub), not a bar later
        const bool bull = meaning(s, S.boSign[s]);
        const COLOR c = S.boSign[s] < 0 ? (bull ? C_BUY_DIM : C_SELL_DIM) : (bull ? C_BUY : C_SELL);
        setPen(c, S.boSign[s] > 0 ? 3 : S.boSign[s] < 0 ? 1 : 2, P_SOLID); stroke(x0, y, x1, y); ++S.boDrawn;
        LQState::BoHit h; h.key = L.key; h.b0 = b0; h.b1 = lastBar; h.px = L.px; h.tol = tol; S.boHits.push_back(h);
    }
    // 2) the BIG column
    if (S.colBigL < 0 || S.colBigR - S.colBigL < 20) { S.boState = S.boState + " (no BIG column room)"; return; }
    const int cl = S.colBigL, cr = S.colBigR;
    { RCT bg; bg.set(coord(cl), coord(top), coord(cr), coord(bottom)); bg.draw(0, C_COLBG, C_COLBG, DRAW_OPAQUE, PAT_SOLID); }
    FONT fh; fh.id = HELVETICA; fh.size = coord(9); fh.style = BOLD; setFont(fh); setTextColor(C_GREY);
    { RCT r; r.set(coord(cl), coord(top + 2), coord(cr), coord(top + 14)); r.drawText("BIG", true, false); }
    std::vector<std::pair<int, int>> used;                          // y bands taken (top, bottom)
    auto freeAt = [&](int t0, int b0) { for (size_t i = 0; i < used.size(); ++i) if (!(b0 < used[i].first || t0 > used[i].second)) return false; return true; };
    FONT fb; fb.id = HELVETICA; fb.size = coord(std::max(10, S.boSet.font)); fb.style = BOLD; setFont(fb);
    const int boxW = std::max(24, (cr - cl) * 64 / 100), boxH = rowPx(std::max(10, S.boSet.font)) + 2;
    for (int s = 0; s < 2; ++s) {
        if (S.bo.live[s] < 0) continue;
        const bol::Line L = S.bo.now((size_t)S.bo.live[s]);
        const int y = yOfPx(L.px); const int t0 = y - boxH / 2, b0 = t0 + boxH;
        if (t0 < top + 14 || b0 > bottom) continue;
        const bool bull = meaning(s, S.boSign[s]); const long long n = (long long)std::llround(S.boSize[s] > 0 ? S.boSize[s] : (double)L.left);
        const std::string tx = (S.boSign[s] > 0 ? "+" : S.boSign[s] < 0 ? "-" : "") + std::to_string(n);
        const COLOR fill = S.boSign[s] < 0 ? (bull ? C_BUY_DIM : C_SELL_DIM) : (bull ? C_BUY : C_SELL);
        RCT r; r.set(coord(cl + 2), coord(t0), coord(cl + 2 + boxW), coord(b0)); r.draw(0, fill, fill, DRAW_OPAQUE, PAT_SOLID);
        setTextColor(S.boSign[s] < 0 ? (bull ? C_MINT : C_PINK) : C_WHITE); r.drawText(tx.c_str(), true, false);
        used.push_back(std::make_pair(t0, b0)); ++S.boLabels;
    }
    // executed in the last 60 min, merged by side + price; the 5 biggest per side
    struct Ex { int side; long long px, lots, last; int n; };
    std::vector<Ex> ex; const long long nowT = S.bo.lastT > 0 ? S.bo.lastT : nowMs(S);
    for (size_t k = 0; k < S.bo.lines.size(); ++k) {
        const bol::Line& L = S.bo.lines[k];
        if (L.end != bol::FILLED || L.tEnd <= 0 || L.tEnd < nowT - 3600000LL || L.traded <= 0) continue;
        bool merged = false;
        for (size_t i = 0; i < ex.size(); ++i) if (ex[i].side == L.side && ex[i].px == L.px) { ex[i].lots += L.traded; ex[i].n++; ex[i].last = std::max(ex[i].last, L.tEnd); merged = true; break; }
        if (!merged) { Ex e; e.side = L.side; e.px = L.px; e.lots = L.traded; e.last = L.tEnd; e.n = 1; ex.push_back(e); }
    }
    std::vector<Ex> keep;
    for (int s = 0; s < 2; ++s) {
        std::vector<Ex> v; for (size_t i = 0; i < ex.size(); ++i) if (ex[i].side == s) v.push_back(ex[i]);
        std::sort(v.begin(), v.end(), [](const Ex& a, const Ex& b) { return a.lots > b.lots; });
        for (size_t i = 0; i < v.size() && i < 5; ++i) keep.push_back(v[i]);
    }
    std::sort(keep.begin(), keep.end(), [](const Ex& a, const Ex& b) { return a.px > b.px; });   // top of the chart first
    FONT fs; fs.id = HELVETICA; fs.size = coord(8); fs.style = PLAIN; setFont(fs);
    const int sh = rowPx(8);
    for (size_t i = 0; i < keep.size(); ++i) {
        const Ex& e = keep[i]; const int y = yOfPx(e.px);
        const std::string tx = std::string("\x95 ") + (e.n > 1 ? std::to_string(e.n) + "\xD7 " : std::string()) + std::to_string(e.lots);
        int t0 = y - sh / 2, tries = 0;
        while (tries < 6 && !freeAt(t0, t0 + sh)) { t0 += sh + 1; ++tries; }   // never over a live box or another mark
        if (tries >= 6 || t0 < top + 14 || t0 + sh > bottom) continue;
        const bool old = nowT - e.last > 1800000LL;                  // older than 30 min: dimmer
        setTextColor(e.side == bol::BID ? (old ? C_SELL_DIM : C_SELL) : (old ? C_BUY_DIM : C_BUY));   // (0.19.1, Rassul 10-15) filled bid RED, filled ask GREEN - the same colours as Big Bid / Ask on the bar (who hit it)
        RCT r; r.set(coord(cl + 2), coord(t0), coord(cr - 2), coord(t0 + sh)); r.drawText(tx.c_str(), false, true);   // right-aligned
        used.push_back(std::make_pair(t0, t0 + sh)); ++S.boExec;
    }
}
// the hovered line's box: the rows from bol::hoverRows (tested), inside the drawable band, never over the column or the right columns
void LiquidityProfile::renderBigHover(LQState& S, int paneRight) {
    S.boHovState = "none";
    if (S.boHovKey == 0 || !S.boSet.on) return;
    size_t k = S.bo.lines.size(); for (size_t i = S.bo.lines.size(); i-- > 0;) if (S.bo.lines[i].key == S.boHovKey) { k = i; break; }
    if (k == S.bo.lines.size()) { S.boHovKey = 0; return; }
    bol::HoverCtx c; c.tick = S.e.P.tick; c.reachAtr = S.bo.P.reachAtr; c.atrPrice = S.bo.atrPrice; c.nowMs = S.bo.lastT > 0 ? S.bo.lastT : nowMs(S);
    c.minLots = S.bo.threshold(); c.floor = S.bo.P.floor; c.pctLots = S.bo.rollThr;   // (0.19.2) the self-tuned size
    const std::vector<bol::Row> rows = bol::hoverRows(S.bo.now(k), c);
    const COLOR cWhite = 0x00E2E8F0, cFill = 0x0011151C;
    int font = S.boSet.font, W = 0, rowH = 0; const int pad = 6;
    const int x1Lim = (S.colPriceRight > 0 ? std::min(S.colPriceRight, paneRight) : paneRight) - 4, x0Lim = S.boXMin;
    for (;;) {                                                     // the font steps down to 10 pt before any row is shortened
        FONT f; f.id = HELVETICA; f.size = coord(font); f.style = BOLD; setFont(f);
        W = 0; for (size_t i = 0; i < rows.size(); ++i) W = std::max(W, (int)getTextWidth(rows[i].text.c_str(), -1));
        W += 2 * pad + 4; rowH = rowPx(font);
        if (W <= x1Lim - x0Lim || font <= bol::FONT_MIN) break;
        --font;
    }
    const int H = rowH * (int)rows.size() + 2 * pad - 2;
    if (x1Lim - x0Lim < 120 || S.boBot - S.boTop < H) { S.boHovState = "no room"; return; }
    if (W > x1Lim - x0Lim) W = x1Lim - x0Lim;
    PNT pm; pm.set(S.boHovBar >= 0 ? S.boHovBar : 0, (float)S.boHovPrice);
    int x0 = (int)pm.h + 14 + W > x1Lim ? (int)pm.h - 14 - W : (int)pm.h + 14;
    x0 = std::max(x0Lim, std::min(x0, x1Lim - W));
    int y0 = (int)pm.v + 12 + H > S.boBot ? (int)pm.v - 12 - H : (int)pm.v + 12;
    y0 = std::max(S.boTop, std::min(y0, S.boBot - H));
    setPen(C_GREY, 1, P_SOLID);
    RCT bx; bx.set(coord(x0), coord(y0), coord(x0 + W), coord(y0 + H)); bx.draw(1, C_GREY, cFill, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < rows.size(); ++i) {
        std::string tx = rows[i].text;                             // only when even 10 pt cannot fit (a very narrow chart)
        while (tx.size() > 4 && (int)getTextWidth(tx.c_str(), -1) > W - 2 * pad - 2) tx.resize(tx.size() - 4);
        if (tx != rows[i].text) tx += "...";
        const int ink = rows[i].ink;
        setTextColor(ink == bol::INK_GREEN ? C_BUY : ink == bol::INK_RED ? C_SELL : ink == bol::INK_GREY ? C_GREY : cWhite);
        const int y = y0 + pad - 1 + (int)i * rowH; RCT r; r.set(coord(x0 + pad), coord(y), coord(x0 + W - 2), coord(y + rowH)); r.drawText(tx.c_str(), false, false);
    }
    S.boHovState = "shown " + std::to_string((long long)rows.size()) + " rows";
}
#endif

void LiquidityProfile::pump() {
    LQState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    LQState& S = *P;
    S.busy = true;
    struct Unbusy { LQState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        if (S.faulted || S.tick <= 0) { if (S.tick <= 0) S.state = "no tick size from IRT"; return; }
        const long long t = nowMs(S), st = steadyMs();
        session(S, t);
#ifndef LQ_FLOW
        loadBig(S); boSessionCheck(S, t);                              // (0.18.0) the big-order settings + this session's lines
#endif
        if (S.lastNorms < 0 || st - S.lastNorms >= 60000) { S.lastNorms = st; loadNorms(S); }
        if (S.lastLayout < 0 || st - S.lastLayout >= 1000) { S.lastLayout = st; loadLayout(S); loadLevels(S, getBarCount()); loadReach(S); updateConfirms(S); loadDelta(S); }
        openDepth(S);
        const long count = getBarCount();
        if (count < S.barCount) {                                       // (0.13.3, audit LQP-002) chart reload / history rewind: a new VAP baseline
            S.barCount = 0; S.vapBar = -1; S.vapStamp = 0; S.vapRows.clear(); S.primed = false;
        }
        // (0.11.4) a new bar: the closed bar's last trades are read BEFORE it is decided (before, up to 1 s of them - the depth read's
        // step - reached the attempt after its Exh / hold / break was decided)
        const bool newBar = S.barCount > 0 && count > S.barCount;
        if (newBar || S.lastDepth < 0 || t - S.lastDepth >= DEPTH_EVERY_MS || t < S.lastDepth) {
            S.lastDepth = t;
            readTrades(S, t, count);                                    // trades first: they explain the book change
            S.e.formBar = (int)count - 1;                               // book flow belongs to the bar in progress
            readDepth(S, t);
        }
#ifndef LQ_FLOW
        orders(S, st);                                                  // (0.15.0) before the bars are decided: hits / rebuilds in time
#endif
        closedBars(S, count);
#ifndef LQ_FLOW
        boAfter(S, count, st);                                          // (0.18.0) ATR, bar ends, price, time steps, book check, record
#endif
        if (S.e.haveBook && (S.lastZones < 0 || t - S.lastZones >= ZONES_EVERY_MS || t < S.lastZones)) {
            S.lastZones = t;
            struct tm tt; std::memset(&tt, 0, sizeof(tt));
            if (getLocaltime(currentDate(), &tt)) S.slot = (tt.tm_hour * 60 + tt.tm_min) / 30;
            S.e.updateZones(t, S.atr, S.slot);
        }
        S.state = S.mdBlocked ? S.state : !S.md ? "depth not opened" : !S.depthAvail ? "no depth from this feed" : !S.e.haveBook ? "waiting for depth" : "ready";
        status(S, st);
        if (S.e.haveBook && S.depthAvail) { writeLiqZones(S); writeLiqLevels(S); }   // (0.12.0 / 0.16.0) only from a live book
    } catch (...) {
        S.state = "calculation error";                                  // never let an exception reach IRT
    }
}

int LiquidityProfile::barOfTime(long long barTime, long count) {
    RTARRAYI times(barDateTime);
    if (times.count < count || count < 1) return -1;
    int lo = 0, hi = (int)count - 1;
    while (lo <= hi) { const int mid = (lo + hi) / 2; const long long v = (long long)times[mid]; if (v == barTime) return mid; if (v < barTime) lo = mid + 1; else hi = mid - 1; }
    return -1;
}

static std::string lots(double v) { char b[32]; std::snprintf(b, sizeof(b), v >= 10000 ? "%.0fk" : "%.0f", v >= 10000 ? v / 1000 : v); return b; }
static std::string mult(double x) { char b[32]; if (x >= 10) std::snprintf(b, sizeof(b), "10x+"); else std::snprintf(b, sizeof(b), "%.1fx", x); return b; }

void LiquidityProfile::render(LQState& S) {
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int paneRight = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneRight = scale.left - 2;
    // (0.9.0, Rassul 2026-10-11 18:07-18:14 mockup v4 "move liquidity profile to the left", "facing price", "the label ... on the right",
    // "draw the box across the chart", "justify left the profile"): the column sits FLUSH with the pane's left edge, its bars grow RIGHT
    // toward price, the x-normal label is on the price side of each bar's tip, and each zone is a thin box from there across the chart
    // to the last bar (never past it). Nothing is drawn on the right any more, so it can never cover the Delta / Dealer Profiles.
    const int left = pane.left + 1;
    const int full = S.width - 4;                                                   // the longest bar (the Width setting)
    const int rowH = rowPx(S.font);                                                 // (0.11.4) rows sized from the font
    int hdrTop = warnTop(pane.top) + warnBlockH(S.font, S.warnRows) + 4;           // (0.14.1) under the title row + the warning box; (0.18.0) + its 2nd detail line
    if (S.trBanner && S.trRect.l < left + S.width + 60 && S.trRect.t < hdrTop + rowH && S.trRect.b + 4 > hdrTop)
        hdrTop = S.trRect.b + 4;                                                    // (0.15.3) under the Tape Reader's banner (its reserved 2-line height)
    const int top = hdrTop + rowH + 2, bottom = pane.bottom;
    const long count = getBarCount();
    if (paneRight - left < 160 || bottom - top < 40 || count < 2) return;          // #35: no room, draw nothing
    const int lastBar = (int)count - 1;
    int lastX = paneRight;                                                          // the last bar's right edge: boxes stop there
    { PNT pe; if (pe.set(lastBar, 0.0f, kBarRight) == RTX_OK && pe.h > left) lastX = std::min((int)pe.h, paneRight); }
    FONT f; f.id = HELVETICA; f.size = coord(S.font); f.style = BOLD; setFont(f);
#ifndef LQ_FLOW
    {   // (0.16.0) the column's width on the board for this chart; the right-side columns (Levels / Delta / Dealer) this chart has
        const char* cl = getChartLabel(); const std::string chart = cl ? std::string(cl) : std::string(); const long long now = steadyMs();
        S.chartKey = chart;
        cb::publish(cb::LIQ, S.symbol, S.spb, chart, now, S.width); S.colPublished = !S.symbol.empty();
        S.peers.step(S.symbol, S.spb, chart, now, 1u << cb::LIQ);
        cb::In in; in.paneL = pane.left; in.paneT = pane.top; in.paneB = pane.bottom; in.axisR = paneRight;
        in.levels = S.peers.on[cb::LEVELS]; in.levelsW = S.peers.w[cb::LEVELS]; in.delta = S.peers.on[cb::DELTA]; in.deltaW = S.peers.w[cb::DELTA];
        in.dealer = S.peers.on[cb::DEALER]; in.dealerW = S.peers.w[cb::DEALER];
        in.big = S.boSet.on; in.bigW = LQ_BIG_W;                          // (0.19.0) our BIG column, first after the last bar; (0.22.5) 44 -> 56 px
        if (S.boSet.on) cb::publish(cb::BIG, S.symbol, S.spb, chart, now, LQ_BIG_W); else cb::clear(cb::BIG, S.symbol, S.spb, chart);
        const cb::Layout L = cb::layout(in);
        S.colPriceRight = L.margin > 0 ? L.priceRight : -1;
        S.colBigL = L.w[cb::BIG] > 0 && L.margin > 0 ? L.big.l : -1; S.colBigR = S.colBigL >= 0 ? L.big.r : -1;
    }
    // (0.16.0, Rassul 11:00 CT) the column's own SOLID dark background (Solid background): from under the banner / header to the
    // bottom, the full Width - drawn first, so the bars and the header sit on it and the candles pass behind it
    if (S.bg) { RCT bgr; bgr.set(coord(pane.left), coord(std::max((int)pane.top, hdrTop - 2)), coord(pane.left + S.width), coord(bottom));
                bgr.draw(0, C_COLBG, C_COLBG, DRAW_OPAQUE, PAT_SOLID); }
#endif
    // header: what the column is and whether it has data (no other text on the chart)
    {
        const bool ok = S.e.haveBook && S.depthAvail;
        const std::string h = ok ? "LIQ TESTING" : S.mdBlocked ? "LIQ blocked" : "LIQ no depth";
        setTextColor(ok ? C_GREY : C_SELL);
        // (CR5000) Offset X / Y moves the label only (the column stays): clamped inside the pane, left of the last bar, below the title row,
        // and never over the Tape Reader banner (then just under it)
        const int tw = getTextWidth(h.c_str(), -1) + 4;
        const bp::Box area = bp::mk(left, warnTop(pane.top), std::max(left + tw, lastX), bottom);
        bp::Box lb = bp::offset(bp::mk(left + 2, hdrTop, left + 2 + tw, hdrTop + rowH), S.labDx, S.labDy, area);
        if (S.trBanner && bp::overlap(lb, bp::mk(S.trRect.l, S.trRect.t, S.trRect.r, S.trRect.b + 4))) lb = bp::clampIn(bp::mk(lb.l, S.trRect.b + 4, lb.r, S.trRect.b + 4 + rowH), area);
        S.labBox = lb;
        RCT r; r.set(coord(lb.l), coord(lb.t), coord(lb.l + std::max(full + 58, tw - 2)), coord(lb.b)); r.drawText(h.c_str(), false, false);
    }
    if (!S.e.haveBook) return;
    const double tick = S.e.P.tick;
    auto yOf = [&](double price) { PNT p; p.set(lastBar, (float)price); return (int)p.v; };
    // plain book rows (dim), scaled to the biggest single level in view, growing right from the edge
    double maxRow = 0;
    for (int s = 0; s < 2; ++s) for (std::map<lqp::Tick, double>::const_iterator i = S.e.book[s].begin(); i != S.e.book[s].end(); ++i) {
        const int y = yOf((double)i->first * tick); if (y > top && y < bottom) maxRow = std::max(maxRow, i->second);
    }
    double maxZone = 0; for (size_t i = 0; i < S.e.zones.size(); ++i) if (S.e.zones[i].shown) maxZone = std::max(maxZone, S.e.zones[i].size);
    if (maxRow > 0) {
        for (int s = 0; s < 2; ++s) for (std::map<lqp::Tick, double>::const_iterator i = S.e.book[s].begin(); i != S.e.book[s].end(); ++i) {
            int yt = yOf(((double)i->first + 0.5) * tick), yb = yOf(((double)i->first - 0.5) * tick);
            if (yb <= yt) yb = yt + 1;
            if (yb <= top || yt >= bottom) continue;
            const int len = std::max(1, (int)(i->second / maxRow * full * 0.5));
            RCT r; r.set(coord(left), coord(std::max(top, yt)), coord(left + len), coord(std::min(bottom, yb - (yb - yt > 3 ? 1 : 0))));
            r.draw(0, s == lqp::BID ? C_BUY_DIM : C_SELL_DIM, s == lqp::BID ? C_BUY_DIM : C_SELL_DIM, DRAW_OPAQUE, PAT_SOLID);
        }
    }
    // the zones: bright bar (solid = durable, hollow = new) facing price with the lots inside, "5.1x" just past the tip, then the box
    const long long nowT = S.haveOffset ? steadyMs() + S.ctOffset : 0;
    const lqp::Readout rd = S.e.readout(nowT);
    // (0.11.4) strongest first; a zone overlapping a stronger one is not drawn; a label goes only where no other label is (y spans)
    const std::vector<LqPick> picks = lqPicks(S);                      // (0.13.0) the strongest each side + the zones on a level
    lqp::Lanes labels(1);
    int colR = 0;                                                       // (0.13.2) the column's right end this draw
#ifndef LQ_FLOW
    S.hovZones.clear();                                                 // (0.15.0) the near zones the mouse can hover this draw
#endif
    for (size_t oi = 0; oi < picks.size(); ++oi) {
        const lqp::Zone& z = S.e.zones[picks[oi].idx];                   // (0.10.0) only zones that stand out from the book behind them
        const bool topPick = picks[oi].top;
        int yt = yOf(((double)z.edgeHi + 0.5) * tick), yb = yOf(((double)z.edgeLo - 0.5) * tick);   // the real edges of the size
        if (yb <= yt) yb = yt + 2;
        if (yb <= top || yt >= bottom) continue;
        yt = std::max(top, yt); yb = std::min(bottom, yb);
        const COLOR c = z.side == lqp::BID ? C_BUY : C_SELL;
        const bool solid = topPick;                                    // (0.13.0) solid = the strongest on its side; outlined = on a level
#ifndef LQ_FLOW
        if (z.near && solid) {                                        // (0.15.0) the level it holds names the hover's header
            LQState::HovZone hz; hz.id = z.id; hz.lo = z.edgeLo; hz.hi = z.edgeHi; hz.names = picks[oi].names;
            if (hz.names.empty()) { std::map<long long, std::string>::const_iterator q = S.lqMark.find(z.id); if (q != S.lqMark.end()) hz.names = q->second.compare(0, 4, "LQ? ") == 0 ? q->second.substr(4) : q->second; }
            S.hovZones.push_back(hz); }
#endif
        const int len = std::max(8, (int)(maxZone > 0 ? z.size / maxZone * full : full));
        RCT r; r.set(coord(left), coord(yt), coord(left + len), coord(yb));
        if (solid) r.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID);
        else { setPen(c, 1, P_SOLID); CBRUSH br(c, PAT_HOLLOW); br.set(); r.draw(1, c, c, DRAW_OPAQUE, PAT_HOLLOW); }   // (0.11.4) no dashed borders
        const int ym = (yt + yb) / 2;
        FONT g; g.id = HELVETICA; g.size = coord(S.font); g.style = BOLD; setFont(g);
        const std::string lt = lots(z.size); const int lw = getTextWidth(lt.c_str(), -1);
        // (0.16.0, Rassul 11:02 CT) the size INSIDE every node in white ("103"), only when the node is long and tall enough for it
        if (lw + 4 < len && yb - yt >= rowH - 4) { setTextColor(C_WHITE); RCT tr; tr.set(coord(left), coord(ym - S.font), coord(left + len), coord(ym + S.font)); tr.drawText(lt.c_str(), true, false); }
        if (S.confirmed.count("Old")) {                                 // (0.5.0) old orders: a 2-px outline and "old 64%" under the bar
            const double os = S.e.zoneOld(z.side, z.lo, z.hi, S.e.bookT);
            if (os >= S.e.P.oldShow) {
                setPen(c, 2, P_SOLID); CBRUSH hb(c, PAT_HOLLOW); hb.set();
                RCT orr; orr.set(coord(left), coord(yt - 1), coord(left + len + 1), coord(yb + 1)); orr.draw(2, c, c, DRAW_OPAQUE, PAT_HOLLOW);
                char ot[24]; std::snprintf(ot, sizeof(ot), "old %.0f%%", os * 100.0);
                if (labels.place(yb + 1, yb + rowH, 1) >= 0) {
                    setTextColor(c); RCT otr; otr.set(coord(left), coord(yb + 1), coord(left + len + 60), coord(yb + rowH)); otr.drawText(ot, false, false); }
            }
        }
        // (0.10.0) the label on the PRICE side of the tip = the prominence (x the book behind), then LQ where it confirms a level.
        // No box across the chart (Rassul 18:53 "remove the boxes too because they clutter the chart").
        int xl = left + len + 4;
        const int ly0 = ym - rowH / 2, ly1 = ly0 + rowH;
        if (labels.place(ly0, ly1, 1) < 0) continue;                   // (0.11.4) another zone's label is there: the bar only
        // (0.16.0) an outlined node's lots are no longer printed beside it: inside it (above), or not at all when it does not fit
        if (z.bx > 0) { const std::string xs = mult(z.bx); const int xw = getTextWidth(xs.c_str(), -1); setTextColor(solid ? c : C_GREY);
            RCT xr; xr.set(coord(xl), coord(ly0), coord(xl + xw + 2), coord(ly1)); xr.drawText(xs.c_str(), false, false);
            colR = std::max(colR, xl + xw + 2 - left);
            xl += xw + 6; }
        std::map<long long, std::string>::const_iterator lq = S.lqMark.find(z.id);
        const std::string nm = lq != S.lqMark.end() ? lq->second : picks[oi].names;   // "LQ PDL" (confirmed) or the level's name
        if (!nm.empty() && xl < lastX) { const int qw = getTextWidth(nm.c_str(), -1); setTextColor(solid ? c : C_GREY);
            RCT qr; qr.set(coord(xl), coord(ly0), coord(xl + qw + 2), coord(ly1)); qr.drawText(nm.c_str(), false, false);
            colR = std::max(colR, xl + qw + 2 - left); }
    }
    S.colRight = std::max(colR, full + 4);   // (0.13.2) published so the Dealer Profile's badges start right of it
#ifndef LQ_FLOW
    renderBig(S, pane.left, paneRight, warnTop(pane.top), bottom, lastBar, lastX);   // (0.22.5, Rassul 10:35) the BIG column from the top of the chart (under the title row)   // (0.18.0) the biggest resting order lines, over the candles
    {   // (0.15.0) the column's x span and the bar -> x map, for the mouse; then the hovered near zone's makeup box
        S.hovColX0 = pane.left; S.hovColX1 = left + S.colRight;
        PNT a, b; a.set(lastBar, 0.0f, kBarCenter); b.set(lastBar - 1, 0.0f, kBarCenter);
        S.hovLastBar = lastBar; S.hovLastX = a.h; S.hovBarW = (double)(a.h - b.h);
        renderHover(S, left, top, bottom, paneRight);
        renderBigHover(S, paneRight);                                  // (0.18.0) the hovered order line's box, drawn last (on top)
    }
#endif
    (void)rd;   // (0.4.0) no read text and no marks on the price chart (Rassul 2026-10-10: signals only on the panes)
}


#ifndef LQ_FLOW
// (0.15.0, Rassul 6:44 PM CT approved mockup v2) the hovered NEAR zone's box: right of the column, centred on the zone, inside the
// pane, never over the column or the Delta Profile (its labels included); grey solid border (as the order-flow panes' info box); the
// rows and their colours come from lqp::hoverRows (tested) - green = good for buyers, red = good for sellers, white = fact only
void LiquidityProfile::renderHover(LQState& S, int left, int top, int bottom, int paneRight) {
    S.hovState = "none";
    if (S.hovId == 0) return;
    const LQState::HovZone* hz = nullptr;
    for (size_t i = 0; i < S.hovZones.size(); ++i) if (S.hovZones[i].id == S.hovId) hz = &S.hovZones[i];
    const lqp::Zone* zp = nullptr;
    for (size_t i = 0; i < S.e.zones.size(); ++i) if (S.e.zones[i].id == S.hovId) zp = &S.e.zones[i];
    if (!hz || !zp) { S.hovId = 0; return; }                          // no longer a drawn near zone: no box
    const lqp::HoverIn in = hoverIn(S, *zp, hz->names);
    const std::vector<lqp::HoverRow> rows = lqp::hoverRows(in);
    const COLOR cWhite = 0x00E2E8F0, cFill = 0x0011151C;
    FONT f; f.id = HELVETICA; f.size = coord(S.font); f.style = BOLD; setFont(f);   // S.font >= 10 pt (settings floor)
    const int rowH = rowPx(S.font), pad = 6;
    int w = 0; for (size_t i = 0; i < rows.size(); ++i) w = std::max(w, (int)getTextWidth(rows[i].text.c_str(), -1));
    const int W = w + 2 * pad + 4, H = rowH * (int)rows.size() + 2 * pad - 2;
    // (0.16.0) never into the right-side columns: up to the gap before the Levels column (ColumnBoard layout), else the axis
    const int dpLimit = S.colPriceRight > 0 ? S.colPriceRight - 4 : paneRight - 4;
    const int x0 = lqp::hoverX(left + S.colRight, W, dpLimit);
    if (x0 < 0) { S.hovState = "no room"; return; }                   // never over the column or the Delta Profile
    PNT pm; pm.set(S.hovLastBar >= 0 ? S.hovLastBar : 0, (float)(((double)zp->edgeLo + (double)zp->edgeHi) / 2.0 * S.e.P.tick));
    const int y0 = lqp::hoverY((int)pm.v, H, top, bottom);
    setPen(C_GREY, 1, P_SOLID);
    RCT bx; bx.set(coord(x0), coord(y0), coord(x0 + W), coord(y0 + H)); bx.draw(1, C_GREY, cFill, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < rows.size(); ++i) {
        const int k = rows[i].ink;
        setTextColor(k == lqp::INK_GREEN ? C_BUY : k == lqp::INK_RED ? C_SELL : k == lqp::INK_GREY ? C_GREY : cWhite);
        const int y = y0 + pad - 1 + (int)i * rowH; RCT r; r.set(coord(x0 + pad), coord(y), coord(x0 + W - 2), coord(y + rowH)); r.drawText(rows[i].text.c_str(), false, false);
    }
    S.hovState = "shown " + std::to_string((long long)rows.size()) + " rows";
}
// (0.15.0) the hover's inputs for one zone (also the status file's MAKEUP line): the makeup from the order book, the open attempt's
// hit / rebuilt while price is in the zone, the 'as price came' baseline of this approach
lqp::HoverIn LiquidityProfile::hoverIn(const LQState& S, const lqp::Zone& z, const std::string& names) const {
    lqp::HoverIn in; in.side = z.side; in.level = names; in.lo = z.lo; in.hi = z.hi; in.tick = S.e.P.tick; in.lots = z.size; in.bx = z.bx;
    in.mk = lqp::makeupOf(S.ob.book, z.side, z.lo, z.hi, S.e.band(z.side, z.lo, z.hi), S.e.P.coverMin, S.e.P.coverMax);
    in.feedReady = S.feed.stateText(steadyMs()) == "ready";
    in.nearBase = z.nearBase;
    in.inZone = S.e.havePx && S.e.lastPx >= z.lo && S.e.lastPx <= z.hi;
    if (const lqp::Attempt* a = S.e.openOn(z.id)) { in.hit = a->hitLots; in.rebuilt = a->rebuiltLots; }
    in.oneShare = S.e.P.oneOrderShare; in.band = S.e.P.cameBand;
    return in;
}
#endif

// ---- (0.2.0) lsLiquidityFlow: the absorption story in its own pane, for the zone touches since load (Rassul 2026-10-10:
// "the concept of aggression and defense ... the best way to show how and why absorption is occurring or failing"; the replenished-
// over-pulled cross "would mirror aggression giving a double crossover pattern that would be very easy to spot").
// TOP half  AGGRESSION: contracts per bar traded INTO the zone - sellers (red) / buyers (green); attackers are sellers at a support,
//           buyers at a resistance. The defenders' line crossing above the attackers' = O (defense first, then offense).
// BOTTOM    DEFENSE: since price reached the zone, size REPLENISHED vs size PULLED (lots, cumulative); green / red by what it means
//           (new bids / pulled offers are bullish). Replenished crossing above pulled = R. Both crosses = X (the double crossover).
// Signals (0.5.0): Exh (top half) and Pull (bottom half), drawn only where confirmed for the market. O / R crosses are logged only.
// (0.11.3, Rassul 2026-10-12 08:58) the order-book letters (t / p / e / s) are no longer drawn on the price chart; the marks are still
// decided, recorded (<MKT>-<spb>-marks-<session>.csv) and re-read for the study and the Flow pane.
// (0.7.0) the warning for the zone price is in now: name in its colour (green = good for buyers, red = good for sellers, grey = not
// decided) with a border, the plain words after it; top left of the price area, never past the last bar
void LiquidityProfile::renderWarning(LQState& S) {
    lqp::Warn w; const bool have = S.e.warning(S.e.bookT, w, S.e.P.tick);
    {   // (0.15.3) the one banner: publish the warning (or "none") for this chart; a Tape Reader banner here = it shows the warning
        const char* cl = getChartLabel(); const std::string chart = cl ? std::string(cl) : std::string(); const long long now = steadyMs();
        bb::Warn bw; bw.on = have; bw.favour = w.favour; bw.name = w.name; bw.detail = w.detail;
        bb::publishWarn(S.symbol, S.spb, chart, now, bw); S.chartKey = chart; S.published = !S.symbol.empty();
        bb::Rect rr; long long ms = -1; const bool seen = bb::peekBanner(S.symbol, S.spb, chart, rr, ms);   // (CR5000) the heartbeat
        S.trBanner = S.trBeat.step(seen, ms, now); if (S.trBanner) S.trRect = rr;
        if (S.trBanner) { if (have) ++S.handed; return; }
    }
    const long count = getBarCount(); if (count < 3) return;
    // ---- (0.20.0, Rassul approved mockup-banner-verdict-v1 08:42 CT) the badge stays as it is; its two grey lines sit INSIDE the
    // badge's height at one fixed column; line 1 names the level first; a VERDICT badge appears BELOW only during a turn:
    // REV UP ? / REV DOWN ? = a big order FILLED at a zone (high / low / pivot / level, ZoneNear.h) in the last 5 bars and price
    // closed back on its side (absorbed); FAILING ? = the stacking / pulling at the live big orders turns against it. Nothing otherwise.
    RTARRAY hiA(barHigh), loA(barLow), clA(barClose);
    if (hiA.count < count || loA.count < count || clA.count < count) return;
    const int lastBar = (int)count - 1;
    const double tick = S.e.P.tick;
    zn::Ctx Z; const int z0 = std::max(0, lastBar - 400);
#ifndef LQ_FLOW
    for (int i = 0; i <= lastBar; ++i) {
        Z.hi.push_back(i < z0 ? 0.0 : (double)hiA[i]); Z.lo.push_back(i < z0 ? 0.0 : (double)loA[i]);
        const long long em = i < (int)S.barEndMs.size() ? S.barEndMs[(size_t)i] : 0;
        Z.sess.push_back(i < z0 || em <= 0 ? -1 : (em / 1000 - 17LL * 3600) / 86400LL);
    }
#endif
    Z.band = zn::bandFrom(Z.hi, Z.lo, lastBar - 1);
    const std::vector<zn::NL>& named = zn::levelsNamed(flexDir(), S.market, S.symbol, steadyMs());
    for (size_t i = 0; i < named.size(); ++i) Z.levels.push_back(named[i].px);
    auto pxTxt = [&](double v) { char b2[32]; std::snprintf(b2, sizeof(b2), "%.*f", lqp::decimals(tick), v); return std::string(b2); };
    // the verdict
    auto clock12 = [](long long ctMs) { const long long sd = (ctMs / 1000) % 86400; int h = (int)(sd / 3600), m = (int)(sd % 3600 / 60); const int h12 = h % 12 == 0 ? 12 : h % 12; char b3[16]; std::snprintf(b3, sizeof(b3), "%d:%02d", h12, m); return std::string(b3); };
    std::string vlog;                                                  // (0.22.1) one verdict line for the day log
    int vdir = 0; bool vfail = false; std::string v1, v2, absorbTxt; double vpx = 0;
#ifndef LQ_FLOW
    if (tick > 0) {
        // (0.21.0, SignalBoard) the verdict now uses EVERY indicator on this chart: the big orders FILLED here, the Delta Profile's A
        // events and Big Bid/Ask's bold sizes / circles (posted on the chart's SignalBoard). A turn = at a zone, price closed back on
        // the absorbing side, and at least one of: a big order filled there, a Delta Profile A there, a Big Bid/Ask circle there.
        const char* cl0 = getChartLabel(); const std::string chart0 = cl0 ? std::string(cl0) : std::string(); const long long nowS = steadyMs();
        std::vector<sb::Ev> evA, evB, evF;
        sb::read('A', S.symbol, S.spb, chart0, nowS, evA); sb::read('B', S.symbol, S.spb, chart0, nowS, evB);
        auto barKey = [&](int i) { return i >= 0 && i < (int)S.barEndMs.size() ? S.barEndMs[(size_t)i] / 1000 : 0LL; };
        auto barIdx = [&](long long key) { for (int i = lastBar; i >= std::max(0, lastBar - 60); --i) if (barKey(i) == key) return i; return -1; };
        struct Cand { int dir = 0; double px = 0; int bar = -1; long long lots = 0; int n = 0; long long tEnd = 0; double aX = 0; bool circle = false; int src = 0; long long pulledOpp = 0; double pulledPx = 0; };
        std::vector<Cand> cands;
        if (S.boSet.on) {                                                      // 1 big orders filled in the last 5 bars (merged by price)
            for (size_t k = 0; k < S.bo.lines.size(); ++k) {
                const bol::Line& L = S.bo.lines[k];
                if (L.end != bol::FILLED || L.traded <= 0 || L.tEnd <= 0) continue;
                const int b = bol::barOf(S.barEndMs, L.tEnd);
                { sb::Ev e; e.bar = barKey(b); e.px = (double)L.px * tick; e.side = L.side == bol::BID ? -1 : 1; e.val = (double)L.traded; if (e.bar > 0) evF.push_back(e); }
                if (b < lastBar - 4) continue;
                const int dir = L.side == bol::BID ? 1 : -1; const double px = (double)L.px * tick; bool m = false;
                for (size_t i = 0; i < cands.size(); ++i) if (cands[i].src == 1 && cands[i].dir == dir && std::fabs(cands[i].px - px) < tick * 0.5) { cands[i].lots += L.traded; cands[i].n++; cands[i].tEnd = std::max(cands[i].tEnd, L.tEnd); cands[i].bar = std::max(cands[i].bar, b); m = true; }
                if (!m) { Cand c; c.dir = dir; c.px = px; c.bar = b; c.lots = L.traded; c.n = 1; c.tEnd = L.tEnd; c.src = 1; cands.push_back(c); }
            }
            sb::publish('F', S.symbol, S.spb, chart0, nowS, evF);
        }
        for (size_t i = 0; i < evA.size(); ++i) {                              // 2 Delta Profile A in the last 5 bars
            const int b = barIdx(evA[i].bar); if (b < 0 || b < lastBar - 4) continue;
            Cand c; c.dir = evA[i].side > 0 ? 1 : -1; c.px = evA[i].px; c.bar = b; c.aX = evA[i].val; c.tEnd = barKey(b) * 1000; c.src = 2; cands.push_back(c);
        }
        for (size_t i = 0; i < evB.size(); ++i) {                              // 3 Big Bid/Ask absorption circle in the last 5 bars
            if (!evB[i].flag) continue; const int b = barIdx(evB[i].bar); if (b < 0 || b < lastBar - 4) continue;
            Cand c; c.dir = evB[i].side < 0 ? 1 : -1; c.px = evB[i].px; c.bar = b; c.lots = (long long)evB[i].val; c.circle = true; c.tEnd = barKey(b) * 1000; c.src = 3; cands.push_back(c);
        }
        // 4 (0.22.0, Rassul 09:05 - the E-S 9:00 low: a 500 offer pulled + buyers lifted 1486) INITIATIVE at the session extreme: a bold
        //   Big Bid/Ask lift at / near the session low (or a bold hit near the session high) in the last 3 bars
        double sHi = -1e300, sLo = 1e300;
        for (int i = lastBar; i >= 0 && Z.sess[(size_t)i] == Z.sess[(size_t)lastBar]; --i) { sHi = std::max(sHi, Z.hi[(size_t)i]); sLo = std::min(sLo, Z.lo[(size_t)i]); }
        for (size_t i = 0; i < evB.size(); ++i) {
            const int b = barIdx(evB[i].bar); if (b < 0 || b < lastBar - 2) continue;
            const bool up = evB[i].side > 0 && std::fabs(evB[i].px - sLo) <= Z.band, dn = evB[i].side < 0 && std::fabs(evB[i].px - sHi) <= Z.band;
            if (!up && !dn) continue;
            Cand c; c.dir = up ? 1 : -1; c.px = evB[i].px; c.bar = b; c.lots = (long long)evB[i].val; c.tEnd = barKey(b) * 1000; c.src = 4; cands.push_back(c);
        }
        // the opposite side's big order PULLED near there in the last 3 bars (the seller / buyer left before price came)
        for (size_t i = 0; i < cands.size(); ++i) {
            for (size_t k = 0; k < S.bo.lines.size(); ++k) {
                const bol::Line& L = S.bo.lines[k];
                if (L.end != bol::PULLED || L.tEnd <= 0) continue;
                const int b = bol::barOf(S.barEndMs, L.tEnd); if (b < lastBar - 2) continue;
                const bool opp = cands[i].dir > 0 ? L.side == bol::ASK : L.side == bol::BID;
                const double lp = (double)L.px * tick;
                if (opp && std::fabs(lp - cands[i].px) <= Z.band && L.size0 > cands[i].pulledOpp) { cands[i].pulledOpp = L.size0; cands[i].pulledPx = lp; }
            }
        }
        const double tol = std::max(2.0 * tick, Z.band * 0.5);
        const Cand* best = nullptr; int bestScore = -1;
        for (size_t i = 0; i < cands.size(); ++i) {
            Cand& c = cands[i];
            // (0.22.7, 10:40 NQ log: REV DOWN / NONE / REV DOWN several times a minute) the verdict on screen is sticky: it stays until price
            // closes a quarter band (2 ticks min) through its price, and wins a tie against a new candidate - no flicker
            const bool inc = S.vIncDir == c.dir && std::fabs(c.px - S.vIncPx) < tick * 0.5;
            const double hyst = inc ? std::max(2.0 * tick, 0.25 * Z.band) : 0.0;
            const bool held = c.dir > 0 ? (double)clA[lastBar] > c.px - hyst : (double)clA[lastBar] < c.px + hyst;
            if (!held || !zn::atZone(Z, std::max(0, std::min(lastBar, c.bar)), c.px)) continue;
            {   // (0.22.2, Rassul 09:53 - the 9:42 E-S REV UP at 7753 came 30 pts after the 9:00 low: chasing) a turn is at the EXTREME:
                // within one band of the lowest low (up) / highest high (down) of the last 10 bars, else it is a fill inside a move
                double ext = c.dir > 0 ? 1e300 : -1e300;
                for (int i = std::max(0, lastBar - 9); i <= lastBar; ++i) ext = c.dir > 0 ? std::min(ext, Z.lo[(size_t)i]) : std::max(ext, Z.hi[(size_t)i]);
                if (Z.band > 0 && std::fabs(c.px - ext) > Z.band) continue;
            }
            if (c.src == 4) {                                                  // initiative needs the book on its side too
                const int gB = S.boSign[bol::BID], gA = S.boSign[bol::ASK];
                const bool lB = S.bo.live[bol::BID] >= 0, lA = S.bo.live[bol::ASK] >= 0;
                const int sp = c.dir > 0 ? (lB && gB > 0) + (lA && gA < 0) : (lA && gA > 0) + (lB && gB < 0);
                if (sp == 0 && c.pulledOpp <= 0) continue;
            }
            // cross-confirm with the other indicators at the same price / bars
            const long long k0 = barKey(std::max(0, c.bar - 2)), k1 = barKey(lastBar);
            if (c.aX <= 0) { const sb::Ev* a = sb::nearestEv(evA, c.px, tol, k0, k1, c.dir); if (a) c.aX = a->val; }
            if (!c.circle) { const sb::Ev* q = sb::nearestEv(evB, c.px, tol, k0, k1, c.dir > 0 ? -1 : 1); if (q && q->flag) c.circle = true; }
            if (c.lots <= 0) { const sb::Ev* f2 = sb::nearestEv(evF, c.px, tol, k0, k1, c.dir > 0 ? -1 : 1); if (f2) c.lots = (long long)f2->val; }
            const int score = (c.src == 1 || c.lots > 0 ? 1 : 0) + (c.aX > 0 ? 1 : 0) + (c.circle ? 1 : 0);
            const bool bInc = best && S.vIncDir == best->dir && std::fabs(best->px - S.vIncPx) < tick * 0.5;
            if (score > bestScore || (score == bestScore && best && ((inc && !bInc) || (inc == bInc && c.lots > best->lots)))) { best = &c; bestScore = score; }
        }
        if (!best) S.vIncDir = 0;                                           // (0.22.7) nothing on screen any more
        if (best) {
            vdir = best->dir; vpx = best->px; S.verdictScore = bestScore; S.vIncDir = vdir; S.vIncPx = vpx;
            const int sgB = S.boSign[bol::BID], sgA = S.boSign[bol::ASK];
            const bool liveB = S.bo.live[bol::BID] >= 0, liveA = S.bo.live[bol::ASK] >= 0;
            const int sup = vdir > 0 ? (liveB && sgB > 0) + (liveA && sgA < 0) : (liveA && sgA > 0) + (liveB && sgB < 0);
            const int ag = vdir > 0 ? (liveB && sgB < 0) + (liveA && sgA > 0) : (liveA && sgA < 0) + (liveB && sgB > 0);
            vfail = ag > sup && ag > 0;
            // (0.22.6, Rassul 10:37 "buyers pulling and Rev up ... you need to consider both together, they need to be coordinated"):
            // one story - a warning on the OTHER side (BUYERS PULLING / BIDS THINNING against a REV UP, or the mirror) makes it FAILING ?
            const bool against = have && w.favour != 0 && ((w.favour > 0) != (vdir > 0));
            if (against) vfail = true;
            std::string what;
            if (best->src == 4) what = std::string(vdir > 0 ? "buyers lifted " : "sellers hit ") + std::to_string(best->lots) + " at " + pxTxt(best->px) + " (initiative)";
            else if (best->lots > 0) what = std::string(vdir > 0 ? "big bid filled " : "big offer filled ") + (best->n > 1 ? std::to_string(best->n) + "\xD7 " : std::string()) + std::to_string(best->lots)
                + (vdir > 0 ? " (sellers hit it)" : " (buyers lifted it)");   // (0.22.2) who was passive and who hit it
            else what = std::string(vdir > 0 ? "sellers absorbed" : "buyers absorbed");
            v1 = what;
            if (best->pulledOpp > 0) v1 += std::string(" \xB7 big ") + (vdir > 0 ? "offer " : "bid ") + std::to_string(best->pulledOpp) + " at " + pxTxt(best->pulledPx) + " pulled";
            auto side = [&](int sd, const char* nm) { if (S.bo.live[sd] < 0) return std::string();
                const long long n = (long long)std::llround(S.boSize[sd]);
                return std::string(" \xB7 ") + nm + (S.boSign[sd] > 0 ? " stacking +" : S.boSign[sd] < 0 ? " pulled -" : " flat ") + std::to_string(n); };
            v1 += side(bol::BID, "bids") + side(bol::ASK, "offers");
            if (against) { std::string wn = w.name; for (size_t i = 0; i < wn.size(); ++i) if (wn[i] >= 'A' && wn[i] <= 'Z') wn[i] = (char)(wn[i] - 'A' + 'a'); v1 += " \xB7 against it: " + wn; }   // (0.22.6)
            const zn::NL lv = zn::nearest(named, vpx, Z.band);
            v2 = clock12(best->tEnd > 0 ? best->tEnd : barKey(lastBar) * 1000) + (lv.name.empty() ? std::string() : " \xB7 at " + lv.name + " " + pxTxt(lv.px));
            std::string conf;
            if (best->aX > 0) { char bx[24]; std::snprintf(bx, sizeof(bx), "A %.1fx", best->aX); conf = bx; }
            if (best->circle) conf += conf.empty() ? "circle" : ", circle";
            {   char vb[400]; std::snprintf(vb, sizeof(vb), "%s,%d,%s,%d,%lld,%.2f,%d,%d,%lld,%d,%d,%lld,%lld,%s,%s,%d,%s,%.2f",
                    vfail ? "FAILING" : vdir > 0 ? "REV_UP" : "REV_DOWN", vdir, pxTxt(best->px).c_str(), best->src, best->lots, best->aX, best->circle ? 1 : 0, bestScore,
                    best->pulledOpp, S.boSign[bol::BID], S.boSign[bol::ASK], (long long)std::llround(S.boSize[bol::BID]), (long long)std::llround(S.boSize[bol::ASK]),
                    pxTxt((double)clA[lastBar]).c_str(), zn::nearest(named, best->px, Z.band).name.c_str(),
                    against ? 1 : 0, have ? w.name.c_str() : "", Z.band);   // (0.22.8) + the warning against it, the warning on screen, the band
                vlog = vb; }
            if (best->src == 4) absorbTxt = std::string("\x85then turned: ") + (vdir > 0 ? "buyers took the low" : "sellers took the high");
            else absorbTxt = std::string("\x85then held: ") + (best->lots > 0 ? std::to_string(best->lots) + (vdir > 0 ? " sold into a big bid, absorbed" : " bought into a big offer, absorbed") : std::string("absorbed")) + (conf.empty() ? std::string() : " (" + conf + ")");
        }
    }
#endif
    S.verdict = vdir == 0 ? "none" : vfail ? "FAILING ?" : vdir > 0 ? "REV UP ?" : "REV DOWN ?";
#ifndef LQ_FLOW
    {   // (0.22.1, Rassul 09:16 "i want the tracking of indicators today") every verdict change to <MKT>-<sym>-<spb>-verdicts-<session>.csv
        const std::string key = vdir == 0 ? std::string("none") : vlog.substr(0, vlog.find(',', vlog.find(',', vlog.find(',') + 1) + 1));   // verdict,dir,px
        if (key != S.vlogKey && S.boSession >= 0) {
            const std::string d = dir();
            if (!d.empty()) {
                const long long em = lastBar < (int)S.barEndMs.size() ? S.barEndMs[(size_t)lastBar] : 0;
                // (0.22.8, Rassul 10:46 "the verdict accurate while the bar is forming ... for entry") the second it changed and how far into
                // the bar, so the nightly check can score the verdict as it was seen live (intrabar), not only at the bar's close
                const long long nm = nowMs(S), ts = nm / 1000;
                char tb[16]; std::snprintf(tb, sizeof(tb), "%02lld:%02lld:%02lld", (ts % 86400) / 3600, (ts % 3600) / 60, ts % 60);
                const long long into = em > 0 ? std::max(0LL, (nm - (em - (long long)S.spb * 1000)) / 1000) : -1;
                const std::string line = std::string(tb) + "," + std::to_string(em / 1000) + "," + std::to_string(into) + "," + (vdir == 0 ? std::string("NONE,0,,,,,,,,,,,,,,,,") : vlog);
                appendLines(d + "\\" + recordStem(S) + "-verdicts2-" + std::to_string(S.boSession) + ".csv",
                    "time_ct,bar_key,sec_into_bar,verdict,dir,px,src(1 fill 2 A 3 circle 4 initiative),lots,aX,circle,agree,pulled_opp,sign_bid,sign_ask,size_bid,size_ask,close,level,against,warning,band", std::vector<std::string>{ line });
            }
            S.vlogKey = key;
        }
    }
#endif
    if (!have && vdir == 0) { S.warnRows = 2; return; }
    RCT pane; pane.getPaneRect(false);
    PNT pl; pl.set((int)count - 1, 0.0f, kBarCenter); const int right = std::min((int)pl.h, (int)pane.right) - 4;
    const int x0 = pane.left + 6, y0 = warnTop(pane.top);                     // (0.14.1) below the chart title, never over it
    if (right - x0 < 120) return;
    FONT f; f.id = HELVETICA; f.size = coord(S.font + 2); f.style = BOLD;
    FONT g; g.id = HELVETICA; g.size = coord(S.font + 1); g.style = PLAIN;
    const int lineH = rowPx(S.font + 1);
    const std::string vname = vdir == 0 ? std::string() : vfail ? "FAILING ?" : vdir > 0 ? "REV UP ?" : "REV DOWN ?";
    setFont(f);
    const int nwW = have ? getTextWidth(w.name.c_str(), -1) : 0, nwV = vname.empty() ? 0 : getTextWidth(vname.c_str(), -1);
    const int bw = std::max(nwW, nwV) + 16;                                // one badge width for both rows: the grey text lines up
    const int bh = std::max(warnH(S.font), 2 * lineH + 2);                // the badge spans the two grey lines
    const int gx = x0 + bw + 10;                                          // the grey column
    const int room = std::max(40, right - gx);
    int y = y0;
    auto rowOf = [&](const std::string& name, COLOR c, bool filled, std::vector<std::string> lines) {
        setFont(f);
        RCT b; b.set(coord(x0), coord(y), coord(x0 + bw), coord(y + bh));
        if (filled) b.draw(1, c, c, DRAW_OPAQUE, PAT_SOLID); else { setPen(c, 1, P_SOLID); b.draw(1, c, c, DRAW_OPAQUE, PAT_HOLLOW); }
        setTextColor(filled ? (c == C_BUY ? C_INK : C_WHITE) : c);
        {   // (0.22.4, Rassul 10:31 "the badges like Buyers Pulling need to be better aligned inside the box, same with the Rev badge")
            // the name in a one-row rect at the badge's vertical middle (IRT does not centre text vertically in a taller rect)
            const int nh = rowPx(S.font + 2), ty = y + std::max(0, (bh - nh) / 2);
            RCT t; t.set(coord(x0 + 2), coord(ty), coord(x0 + bw - 2), coord(ty + nh)); t.drawText(name.c_str(), true, false); }
        setFont(g); setTextColor(C_GREY);
        std::vector<std::string> rows;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].empty()) continue;
            const std::vector<std::string> wr = lqp::wrapWords(lines[i], [&](const std::string& tx) { return getTextWidth(tx.c_str(), -1) <= room; });
            rows.insert(rows.end(), wr.begin(), wr.end());
        }
        const int top = y + std::max(0, (bh - 2 * lineH) / 2);
        for (size_t i = 0; i < rows.size(); ++i) {
            const int ty = top + (int)i * lineH; const int tw = getTextWidth(rows[i].c_str(), -1) + 4;
            RCT r; r.set(coord(gx), coord(ty), coord(std::max(right, gx + tw)), coord(ty + lineH)); r.drawText(rows[i].c_str(), false, false);
        }
        y = std::max(y + bh, top + (int)rows.size() * lineH) + 6;
    };
    if (have) {
        std::vector<std::string> sides = lqp::warnLines(w.detail);
        const zn::NL lv = zn::nearest(named, (double)clA[lastBar], Z.band);
        if (!sides.empty() && !lv.name.empty()) sides[0] = lv.name + " " + pxTxt(lv.px) + " \xB7 " + sides[0];   // line 1 names the level first
        if (!absorbTxt.empty()) { if (sides.size() < 2) sides.push_back(absorbTxt); else sides[1] += " " + absorbTxt; }
        const COLOR c = w.favour > 0 ? C_BUY : w.favour < 0 ? C_SELL : C_GREY;
        rowOf(w.name, c, false, sides);                                   // the badge as it is (outlined), its two grey lines
    }
    if (vdir != 0) {
        const bool bull = vfail ? vdir < 0 : vdir > 0;                    // FAILING of an up-turn is bearish (red); of a down-turn bullish
        rowOf(vname, bull ? C_BUY : C_SELL, true, { v1, v2 });            // the verdict (filled), only during a turn
    }
    const int used = y - y0 - 6;
    S.warnRows = std::max(2, 1 + (int)std::ceil(std::max(0, used - warnH(S.font)) / (double)rowPx(S.font + 1)));   // what render() reserves below
}
void LiquidityProfile::renderFlow(LQState& S) {
    auto flowLine = [this](const std::vector<std::pair<int, int>>& pts, COLOR c) {
        if (pts.empty()) return;
        setPen(c, 2, P_SOLID);
        PNT a; a.set(0, 0.0f); a.h = coord(pts[0].first); a.v = coord(pts[0].second); a.setDrawPosition();
        if (pts.size() == 1) { PNT b; b.set(0, 0.0f); b.h = coord(pts[0].first + 2); b.v = coord(pts[0].second); b.drawLineTo(); return; }
        for (size_t i = 1; i < pts.size(); ++i) { PNT b; b.set(0, 0.0f); b.h = coord(pts[i].first); b.v = coord(pts[i].second); b.drawLineTo(); }
    };
    RCT pane; pane.getPaneRect(false);
    const long count = getBarCount();
    if (count < 2 || pane.bottom - pane.top < 60 || pane.right - pane.left < 80) return;   // #35
    const int lastBar = (int)count - 1;
    const int mid = (pane.top + pane.bottom) / 2;
    // (0.11.4) every row is sized from the font (10 pt+): header row, then the window-label row (t0), the step row, the reason rows
    const int rowH = rowPx(S.font);
    const int t0 = pane.top + rowH + 2, t1 = mid - 3, b0 = mid + rowH + 2, b1 = pane.bottom - 3, zeroY = (b0 + b1) / 2;
    FONT f; f.id = HELVETICA; f.size = coord(S.font); f.style = BOLD; setFont(f); setTextColor(C_GREY);
    // (0.3.0, Rassul 2026-10-10) green = buyers and red = sellers in BOTH halves, at support and resistance alike
    { RCT r; r.set(coord(pane.left + 4), coord(pane.top + 1), coord(pane.left + 400), coord(pane.top + 1 + rowH)); r.drawText("AGGRESSIVE", false, false); }
    { RCT r; r.set(coord(pane.left + 4), coord(mid + 1), coord(pane.left + 400), coord(mid + 1 + rowH)); r.drawText("PASSIVE", false, false); }
    setPen(C_AXIS, 1, P_SOLID);
    { PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(mid); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(pane.right); b.v = coord(mid); b.drawLineTo(); }
    // ---- (0.11.0, Rassul 2026-10-11 19:48 "it should always show something and when absorption is occurring then it should show related
    // signals and metrics", mockup LiquidityFlow_always_on 19:49 "build"): ALWAYS ON - top half the buyers (green, up) and sellers (red,
    // down) of every bar as x their own 20-bar average, with the Attack line; bottom half the resting bids below / offers above price
    // within the reach at each bar's close. ABSORPTION (a shown zone or a Delta Profile A / A? touched) = a yellow window over its bars
    // with its price, the steps on its bars and the readout on the right (Metric: now (line)); idle, the readout names what it watches.
    const COLOR C_WIN = 0x003CC8FA;                                      // yellow (BGR): the absorption window
    auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
    auto txt = [&](int x, int y, const std::string& t, COLOR c) { setTextColor(c); const int w = getTextWidth(t.c_str(), -1); RCT r; r.set(coord(x), coord(y), coord(x + w + 2), coord(y + rowH)); r.drawText(t.c_str(), false, false); return x + w; };
    auto px = [&](lqp::Tick t) {
        const double v = (double)t * S.e.P.tick; const int d = lqp::decimals(S.e.P.tick);   // (0.11.4) "79.98", not "79.979998"
        char b[32]; std::snprintf(b, sizeof(b), "%.*f", d, v); std::string r = b; const size_t dot = r.find('.'); int i = (int)(dot == std::string::npos ? r.size() : dot);
        while ((i -= 3) > (r[0] == '-' ? 1 : 0)) r.insert((size_t)i, ",");
        return r;
    };
    auto xs = [](double v) { char b[16]; if (v < 0) std::snprintf(b, sizeof(b), "-"); else std::snprintf(b, sizeof(b), "%.1fx", v); return std::string(b); };
    const int rdW = std::min(330, (pane.right - pane.left) / 3);           // the readout box on the right
    const int plotR = pane.right - rdW - 8;
    const double line = S.lAttack.v > 0 ? S.lAttack.v : 2.0;
    const int aMid = (t0 + t1) / 2, aH = std::max(6, (t1 - t0) / 2 - 2);
    const double aFull = std::max(3.0, line * 1.5);                      // a bar at aFull x fills the half
    FONT hf; hf.id = HELVETICA; hf.size = coord(S.font); hf.style = BOLD; setFont(hf);
    // the Aggressive bars and their 2.0x guides
    setPen(C_AXIS, 1, P_SOLID);
    for (int k = -1; k <= 1; k += 2) { const int y = aMid - k * (int)(aH * std::min(1.0, line / aFull)); PNT a; a.set(0, 0.0f); a.h = coord(pane.left); a.v = coord(y); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(plotR); b.v = coord(y); b.drawLineTo(); }
    double bw = 6; { const int x1 = xOf(lastBar), x0 = xOf(std::max(0, lastBar - 1)); if (x1 - x0 > 2) bw = std::max(2.0, (x1 - x0) * 0.35); }
    int lastAggBar = -1;
    for (std::map<int, lqp::Engine::BarNorm>::const_iterator i = S.e.barNorm.begin(); i != S.e.barNorm.end(); ++i) {
        const int x = xOf(i->first); if (x < pane.left + 2 || x > plotR) continue;
        const double bx = S.e.paceX(i->first, true), sx = S.e.paceX(i->first, false); if (bx < 0 || sx < 0) continue;
        lastAggBar = i->first;
        const int hb = (int)(aH * std::min(1.0, bx / aFull)), hs = (int)(aH * std::min(1.0, sx / aFull));
        RCT g; g.set(coord(x - (int)bw), coord(aMid - std::max(1, hb)), coord(x + (int)bw), coord(aMid)); g.draw(0, C_BUY, C_BUY, DRAW_OPAQUE, PAT_SOLID);
        RCT r; r.set(coord(x - (int)bw), coord(aMid), coord(x + (int)bw), coord(aMid + std::max(1, hs))); r.draw(0, C_SELL, C_SELL, DRAW_OPAQUE, PAT_SOLID);
    }
    {   // header: the last closed bar, Metric: now (line)
        int x = pane.left + 4 + getTextWidth("AGGRESSIVE", -1) + 14;
        if (lastAggBar >= 0) {
            const double bx = S.e.paceX(lastAggBar, true), sx = S.e.paceX(lastAggBar, false);
            x = txt(x, pane.top + 1, "Buyers: ", C_GREY); x = txt(x, pane.top + 1, xs(bx), C_BUY); x = txt(x, pane.top + 1, " (" + xs(line) + ")   ", C_GREY);
            x = txt(x, pane.top + 1, "Sellers: ", C_GREY); x = txt(x, pane.top + 1, xs(sx), C_SELL); txt(x, pane.top + 1, " (" + xs(line) + ")", C_GREY);
        } else txt(x, pane.top + 1, "building the 20-bar normal", C_GREY);
    }
    // the Passive lines: resting bids (green) / offers (red) near price at each bar's close, the live book on the forming bar
    {
        std::vector<std::pair<int, int>> gb, rs; double mx = 1;
        // (0.11.1) the recorded bars (this session and the two before, re-read on load) + the engine's + the live book on the forming bar
        std::map<int, std::pair<double, double>> nb; const std::pair<double, double> now = S.e.nearNow();
        { RTARRAYI tm(barDateTime);
          if (tm.count >= count) for (int i = lastBar; i >= 0 && xOf(i) >= pane.left; --i) {
              std::map<long long, std::pair<double, double>>::const_iterator h = S.bookHist.find((long long)tm[i]);
              if (h != S.bookHist.end()) nb[i] = h->second;
              else { std::map<int, std::pair<double, double>>::const_iterator e = S.e.nearBook.find(i); if (e != S.e.nearBook.end()) nb[i] = e->second; } } }
        if (S.e.haveBook && S.e.havePx) nb[lastBar] = now;
        for (std::map<int, std::pair<double, double>>::const_iterator i = nb.begin(); i != nb.end(); ++i) { const int x = xOf(i->first); if (x >= pane.left && x <= plotR) mx = std::max(mx, std::max(i->second.first, i->second.second)); }
        const int pTop = b0 + 2, pBot = b1 - 2;
        for (std::map<int, std::pair<double, double>>::const_iterator i = nb.begin(); i != nb.end(); ++i) {
            const int x = xOf(i->first); if (x < pane.left || x > plotR) continue;
            gb.push_back(std::make_pair(x, pBot - (int)((pBot - pTop) * i->second.first / mx)));
            rs.push_back(std::make_pair(x, pBot - (int)((pBot - pTop) * i->second.second / mx)));
        }
        flowLine(rs, C_SELL); flowLine(gb, C_BUY);
        int x = pane.left + 4 + getTextWidth("PASSIVE", -1) + 14;
        if (S.e.haveBook && S.e.havePx) {
            const std::map<int, std::pair<double, double>>::const_iterator pv = S.e.nearBook.empty() ? S.e.nearBook.end() : std::prev(S.e.nearBook.end());
            auto chg = [](double a, double b) { if (b <= 0) return std::string(); char t[24]; std::snprintf(t, sizeof(t), " (%+.0f%%)", (a / b - 1) * 100.0); return std::string(t); };
            x = txt(x, mid + 1, "Bids near price: ", C_GREY); x = txt(x, mid + 1, lots(now.first), C_BUY); x = txt(x, mid + 1, (pv != S.e.nearBook.end() ? chg(now.first, pv->second.first) : "") + "   ", C_GREY);
            x = txt(x, mid + 1, "Offers: ", C_GREY); x = txt(x, mid + 1, lots(now.second), C_SELL); txt(x, mid + 1, pv != S.e.nearBook.end() ? chg(now.second, pv->second.second) : "", C_GREY);
        } else if (!S.e.haveBook) txt(x, mid + 1, S.mdBlocked ? "blocked" : "no depth", C_SELL);   // (0.11.4) in its own place, not over the header
    }
    // the absorption windows, yellow, with the price they defended: the recorded ones (0.11.1, re-read on load) and the live ones
    auto aPx = [&](const lqp::Attempt& a) { return a.dpHi > 0 ? (a.side == lqp::BID ? a.dpHi : a.dpLo) : (a.side == lqp::BID ? a.hi : a.lo); };
    lqp::Lanes winLabels(1);                                             // (0.11.4) window labels never overlap (the live one is placed first)
    auto window = [&](int ba, int e, const std::string& lb) {
        int xa = xOf(ba) - (int)bw - 3, xe = xOf(e) + (int)bw + 3;
        if (xe < pane.left || xa > plotR) return;
        xa = std::max(xa, pane.left + 1); xe = std::min(xe, plotR);
        setPen(C_WIN, 1, P_SOLID); CBRUSH hb(C_WIN, PAT_HOLLOW); hb.set();
        RCT w; w.set(coord(xa), coord(t0 - 1), coord(xe), coord(b1)); w.draw(1, C_WIN, C_WIN, DRAW_OPAQUE, PAT_HOLLOW);
        const int lw = getTextWidth(lb.c_str(), -1);
        if (xa + 2 + lw <= plotR && winLabels.place(xa + 2, xa + 2 + lw) >= 0) txt(xa + 2, t0, lb, C_WIN);
    };
    for (size_t k = 0; k < S.e.attempts.size(); ++k) {
        const lqp::Attempt& a = S.e.attempts[k]; if (a.done) continue;
        window(a.barA, lastBar, (a.dpHi > 0 ? "A " : "") + px(aPx(a)));
    }
    const long long finBase = S.e.finTotal - (long long)S.e.finished.size();
    for (size_t k = S.e.finished.size(); k-- > 0;) {                     // finished but not recorded yet (the bar close records it)
        const lqp::Attempt& a = S.e.finished[k]; if (finBase + (long long)k < S.finSeen) continue;
        int e = a.flowBars.empty() ? a.barA : a.flowBars.rbegin()->first; if (a.barE >= 0) e = std::max(e, a.barE);
        window(a.barA, e, (a.dpHi > 0 ? "A " : "") + px(aPx(a)));
    }
    for (size_t k = S.winHist.size(); k-- > 0;) {
        const LQState::Win& w = S.winHist[k];
        const int ba = barOfTime(w.tA, count), e = barOfTime(w.tE, count); if (ba < 0 || e < ba) continue;
        bool live = false; for (size_t j = 0; j < S.e.attempts.size(); ++j) if (!S.e.attempts[j].done && S.e.attempts[j].barA == ba && S.e.attempts[j].side == w.side) live = true;
        if (live) continue;                                              // the live one is drawn below, to the last bar
        window(ba, e, (w.dp ? "A " : "") + px(w.px));
    }
    // the readout on the right: the live (or last) absorption's steps as Metric: now (line); idle = what it is watching
    {
        const int rx = plotR + 8;
        setPen(C_AXIS, 1, P_SOLID); CBRUSH hb(C_AXIS, PAT_HOLLOW); hb.set();
        { RCT r; r.set(coord(rx - 4), coord(t0 - 1), coord(pane.right - 2), coord(t1)); r.draw(1, C_AXIS, C_AXIS, DRAW_OPAQUE, PAT_HOLLOW); }
        { RCT r; r.set(coord(rx - 4), coord(b0 - 1), coord(pane.right - 2), coord(b1)); r.draw(1, C_AXIS, C_AXIS, DRAW_OPAQUE, PAT_HOLLOW); }
        lqp::Stats st; bool have = false; const lqp::Attempt* src = nullptr;
        for (size_t i = 0; i < S.e.attempts.size(); ++i) if (!S.e.attempts[i].done && (!src || S.e.attempts[i].tA > src->tA)) src = &S.e.attempts[i];
        const bool live = src != nullptr;
        if (!src && !S.e.finished.empty()) src = &S.e.finished.back();
        if (src) { st = S.e.stats(*src, S.e.bookT); have = true; }
        std::string asOf;
        if (S.hoverBar >= 0) {     // (0.7.0) the mouse over a past bar of a touch shows that touch's statistics as of that bar
            const lqp::Attempt* hit = nullptr;
            for (size_t i = 0; i < S.e.finished.size() && !hit; ++i) if (S.e.finished[i].barStats.count(S.hoverBar)) hit = &S.e.finished[i];
            for (size_t i = 0; i < S.e.attempts.size() && !hit; ++i) if (S.e.attempts[i].barStats.count(S.hoverBar)) hit = &S.e.attempts[i];
            if (hit) {
                st = hit->barStats.find(S.hoverBar)->second; have = true; src = hit;
                RTARRAYI tm(barDateTime);
                if (tm.count > S.hoverBar) { struct tm t; std::memset(&t, 0, sizeof(t));
                    if (getLocaltime((RTDATE)tm[S.hoverBar], &t)) { char b[32]; const int h12 = t.tm_hour % 12 == 0 ? 12 : t.tm_hour % 12;
                        std::snprintf(b, sizeof(b), "  at %d:%02d %s", h12, t.tm_min, t.tm_hour < 12 ? "AM" : "PM"); asOf = b; } }
            }
        }
        FONT sf; sf.id = HELVETICA; sf.size = coord(S.font); sf.style = BOLD; setFont(sf);
        if (have && src) {
            const std::string head = std::string(live || !asOf.empty() ? "ABSORPTION  " : "LAST  ") + (src->dpHi > 0 ? "A " : "") + px(aPx(*src)) + (src->side == lqp::BID ? " support" : " resistance") + asOf;
            txt(rx, t0, head, C_WIN);
            const COLOR def = st.side == lqp::BID ? C_BUY : C_SELL, att = st.side == lqp::BID ? C_SELL : C_BUY;
            struct It { const char* name; double now; LQState::Line line; bool pct, higherSafe; };
            const It agg[3] = { {"Attack", st.attack, S.lAttack, false, false}, {"Exh", st.exh, S.lExh, false, false}, {"Counter", st.counter, S.lCounter, false, true} };
            // (0.11.4) Pulled reads as a % like the Pull signal (Rassul: "Pull signal keeps its % form"); ratios as 2.0x (one decimal)
            const It dfn[3] = { {"Eaten", st.eaten, S.lEaten, true, false}, {"Pulled", st.pulled, S.lPulled, true, false}, {"Rebuilt", st.rebuilt, S.lRebuilt, false, true} };
            auto fmt = [](double v, bool pct) { char b[24]; if (v < 0) std::snprintf(b, sizeof(b), "-"); else if (pct) std::snprintf(b, sizeof(b), "%.0f%%", v * 100.0); else std::snprintf(b, sizeof(b), "%.1fx", v); return std::string(b); };
            auto column = [&](const It* it, int y, int yMax) {
                for (int k = 0; k < 3; ++k) {
                    if (it[k].line.v <= 0 || y + rowH > yMax) continue;
                    const bool safe = it[k].now < 0 ? true : (it[k].higherSafe ? it[k].now >= it[k].line.v : it[k].now < it[k].line.v);
                    int x = txt(rx, y, std::string(it[k].name) + ": ", C_GREY);
                    x = txt(x, y, fmt(it[k].now, it[k].pct), it[k].now < 0 ? C_GREY : (safe ? def : att));
                    txt(x, y, " (" + fmt(it[k].line.v, it[k].pct) + ")", C_GREY);
                    y += rowH;
                }
            };
            column(agg, t0 + rowH, t1);
            column(dfn, b0, b1);
            if ((src->fromDelta || src->dpHi > 0) && b0 + 4 * rowH <= b1) txt(rx, b0 + 3 * rowH, "from: Delta Profile A + book " + lots(src->shown) + " lots", C_GREY);
        } else {
            // idle: the nearest Delta Profile A / shown zone it is watching
            std::string w = "Watching: no absorption yet"; double best = 1e18;
            if (S.e.havePx) {
                for (int sd = 0; sd < 2; ++sd) for (size_t i = 0; i < S.e.dpZone[sd].size(); ++i) {
                    const lqp::Tick t = sd == lqp::BID ? S.e.dpZone[sd][i].second : S.e.dpZone[sd][i].first; const double d = std::fabs((double)(t - S.e.lastPx));
                    if (d < best) { best = d; w = "Watching: A " + px(t) + (sd == lqp::BID ? " support" : " resistance"); }
                }
                for (size_t i = 0; i < S.e.zones.size(); ++i) { const lqp::Zone& z = S.e.zones[i]; if (!z.shown) continue;
                    const lqp::Tick t = z.side == lqp::BID ? z.hi : z.lo; const double d = std::fabs((double)(t - S.e.lastPx));
                    if (d < best) { best = d; w = "Watching: zone " + px(t) + " " + xs(z.bx) + (z.side == lqp::BID ? " support" : " resistance"); } }
                if (best < 1e17) { char b[32]; std::snprintf(b, sizeof(b), ", %.2f pts", best * S.e.P.tick); w += b; }
            }
            txt(rx, t0, w, C_GREY);
        }
    }
    // (0.5.0) the two pane signals, from the session record (so a restart redraws exactly what was shown): Exh in the top half, Pull in
    // the bottom half, at the bar they were decided on; green = buyers win, red = sellers win. Drawn ONLY where the nightly study confirmed
    // that signal for this market (SIGNAL lines in LRA-LiqNorms-<MKT>.csv) - Rassul 2026-10-11 "remove whatever doesn't work and keep
    // what works": an unconfirmed signal is not drawn at all (no "?")
    RTARRAYI times(barDateTime);
    if (times.count < count) return;
    std::map<int, std::vector<const lqp::Mark*>> at;                     // key = bar * 2 + half
    for (size_t i = 0; i < S.e.marks.size(); ++i) {
        const lqp::Mark& m = S.e.marks[i];
        if (!shown(S, m) || (m.code != "Exh" && m.code != "Pull")) continue;     // (0.7.0) e? p? t are price-chart letters; reasons below
        const int bar = barOfTime(m.barTime, count); if (bar < 0 || bar > lastBar) continue;
        std::vector<const lqp::Mark*>& v = at[bar * 2 + (lqp::Engine::topPane(m.code) ? 0 : 1)];
        bool dup = false; for (size_t k = 0; k < v.size(); ++k) if (v[k]->code == m.code) dup = true;
        if (!dup && v.size() < 2) v.push_back(&m);
    }
    FONT g; g.id = HELVETICA; g.size = coord(S.font); g.style = BOLD; setFont(g);
    // (0.11.4) the steps sit one row under the window labels (top) / at the top of the bottom half, in two rows; a step that would cover
    // another step or the readout on the right is not drawn there (before, they were drawn over the window price and the readout)
    lqp::Lanes stepRows[2] = { lqp::Lanes(2), lqp::Lanes(2) };
    for (std::map<int, std::vector<const lqp::Mark*>>::const_iterator it = at.begin(); it != at.end(); ++it) {
        const int bar = it->first / 2; const bool top = it->first % 2 == 0;
        const int x = xOf(bar); if (x < pane.left + 10 || x > plotR) continue;
        const int y0 = top ? t0 + rowH : b0;
        for (size_t k = 0; k < it->second.size(); ++k) {
            const lqp::Mark& m = *it->second[k];
            // the number: Exh 0.3x (weakness), Pull 72%
            char num[24] = "";
            if (m.value >= 0) { if (m.code == "Pull") std::snprintf(num, sizeof(num), " %.0f%%", m.value * 100.0); else std::snprintf(num, sizeof(num), " %.1fx", m.value); }
            const std::string t = m.code + num;
            const int w = getTextWidth(t.c_str(), -1);
            if (x - w / 2 - 1 < pane.left || x + w / 2 + 1 > plotR) continue;
            const int row = stepRows[top ? 0 : 1].place(x - w / 2 - 1, x + w / 2 + 1);
            if (row < 0) continue;
            const int y = y0 + row * rowH; if (y + rowH > (top ? t1 : b1)) continue;
            setTextColor(lqp::Engine::bullish(m) ? C_BUY : C_SELL);
            RCT r; r.set(coord(x - w / 2 - 1), coord(y), coord(x + w / 2 + 1), coord(y + rowH)); r.drawText(t.c_str(), true, false);
        }
    }
    // (0.7.0, Rassul 2026-10-11) the reason in plain words at the bar each touch was decided (Absorbed / Broke), from the record: scroll
    // back and every past one is still there; a thin line marks the bar; split into two lines at "; " when it does not fit
    // (0.11.4) the reasons go in rows under the steps, left to right in time order, each in the first rows where it fits, ending before
    // the readout (before, every reason was written on the header row from its bar to the pane's right edge: over the AGGRESSIVE
    // header, over each other and over the readout)
    std::vector<std::pair<int, const lqp::Mark*>> rs;
    for (size_t i = 0; i < S.e.marks.size(); ++i) {
        const lqp::Mark& m = S.e.marks[i];
        if ((m.code != "Held" && m.code != "Broke" && m.code != "s") || !shown(S, m) || m.reasons.empty()) continue;
        const int bar = barOfTime(m.barTime, count); if (bar < 0 || bar > lastBar) continue;
        rs.push_back(std::make_pair(bar, &m));
    }
    std::stable_sort(rs.begin(), rs.end(), [](const std::pair<int, const lqp::Mark*>& a, const std::pair<int, const lqp::Mark*>& b) { return a.first < b.first; });
    const int rTop = t0 + 3 * rowH, nRows = std::max(0, std::min(4, (t1 - rTop) / rowH));
    lqp::Lanes reasonRows(std::max(1, nRows));
    for (size_t i = 0; i < rs.size(); ++i) {
        const lqp::Mark& m = *rs[i].second;
        const int x = xOf(rs[i].first); if (x < pane.left + 10 || x > plotR - 40) continue;
        const COLOR c = lqp::Engine::bullish(m) ? C_BUY : C_SELL;
        setPen(c, 1, P_SOLID);
        { PNT a; a.set(0, 0.0f); a.h = coord(x); a.v = coord(t0); a.setDrawPosition(); PNT b; b.set(0, 0.0f); b.h = coord(x); b.v = coord(b1); b.drawLineTo(); }
        if (nRows < 1) continue;
        std::vector<std::string> lines; const int room = plotR - 4 - (x + 4);
        if (getTextWidth(m.reasons.c_str(), -1) <= room || nRows < 2) lines.push_back(m.reasons);
        else { const size_t k = m.reasons.find("; "); if (k == std::string::npos) lines.push_back(m.reasons); else { lines.push_back(m.reasons.substr(0, k + 1)); lines.push_back("  " + m.reasons.substr(k + 2)); } }
        int wmax = 0;
        for (size_t k = 0; k < lines.size(); ++k) {
            std::string& d = lines[k]; const std::string full = d;
            while (!d.empty() && getTextWidth(d.c_str(), -1) > room) d.resize(d.size() > 4 ? d.size() - 4 : 0);
            if (d.empty()) continue;
            if (d != full) { d.resize(d.size() > 3 ? d.size() - 3 : 0); d += "..."; }
            wmax = std::max(wmax, getTextWidth(d.c_str(), -1));
        }
        lines.erase(std::remove_if(lines.begin(), lines.end(), [](const std::string& l) { return l.empty(); }), lines.end());
        if (wmax <= 0 || lines.empty()) continue;
        const int need = (int)lines.size(); int row = -1;
        for (int r = 0; r + need <= nRows && row < 0; ++r) { bool ok = true; for (int q = 0; q < need; ++q) ok = ok && reasonRows.fits(r + q, x + 4, x + 4 + wmax); if (ok) row = r; }
        if (row < 0) continue;                                           // no free rows: the line at the bar only
        setTextColor(c);
        for (int q = 0; q < need; ++q) {
            reasonRows.rows[(size_t)(row + q)].push_back(std::make_pair(x + 4, x + 4 + wmax));
            const int y = rTop + (row + q) * rowH; RCT r; r.set(coord(x + 4), coord(y), coord(plotR - 4), coord(y + rowH)); r.drawText(lines[(size_t)q].c_str(), false, false);
        }
    }
    (void)zeroY;
}

// (0.7.0) the mouse over a bar (the Flow pane follows it to show that bar's statistics of a past touch)
int LiquidityProfile::mouse(RTX_EVENT* e) {
#ifndef LQ_FLOW
    // (0.15.0) the price chart: the mouse over a NEAR zone's bar / label in the column -> that zone's makeup box (by price and bar: the
    // bar's x from the last draw's bar -> x map, the price from the event; window pixels are not used)
    if (!e) return RTX_FAIL;
    LQState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (e->type == E_MOUSE_MOVE || e->type == E_MOUSE_HOVER) {
        long long id = 0;
        const double tick = S->e.P.tick, price = (double)e->v.mouse.price;
        if (S->hovColX1 > S->hovColX0 && S->hovLastBar >= 0 && tick > 0 && lqp::finite(price) && price > 0) {
            const double bw = std::fabs(S->hovBarW) > 0 ? std::fabs(S->hovBarW) : 1.0;
            const double x = (double)S->hovLastX + (double)(e->v.mouse.bar - S->hovLastBar) * S->hovBarW;
            if (x >= (double)S->hovColX0 - bw && x <= (double)S->hovColX1 + bw) {
                const double t = price / tick;
                for (size_t i = 0; i < S->hovZones.size() && !id; ++i) if (t >= (double)S->hovZones[i].lo - 0.5 && t <= (double)S->hovZones[i].hi + 0.5) id = S->hovZones[i].id;
            }
        }
        if (id) S->hovWnd = off::cursorWindow();
        if (id != S->hovId) { S->hovId = id; invalidateChart(false); }
        // (0.18.0) the mouse on a big-order line (its price within ~5 px, its bars or the end mark): that line's box
        long long key = 0; long long bestD = LLONG_MAX; const long bar = e->v.mouse.bar;
        if (!id && tick > 0 && lqp::finite(price) && price > 0) {
            const double t = price / tick;
            for (size_t i = 0; i < S->boHits.size(); ++i) {
                const LQState::BoHit& h = S->boHits[i];
                const double d = std::fabs(t - (double)h.px);
                if (d > (double)h.tol + 0.5 || bar < h.b0 - 1 || bar > h.b1 + 1) continue;
                const long long di = (long long)(d * 1000.0); if (di < bestD) { bestD = di; key = h.key; }
            }
        }
        if (key) { S->boHovWnd = off::cursorWindow(); S->boHovBar = (int)bar; S->boHovPrice = price; }
        if (key != S->boHovKey || (key && S->boHovBar != (int)bar)) { S->boHovKey = key; invalidateChart(false); }
    }
    return RTX_FAIL;                                                     // never swallow the event
#else
    if (!e) return RTX_FAIL;
    LQState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (e->type == E_MOUSE_MOVE || e->type == E_MOUSE_HOVER) { const long b = e->v.mouse.bar; S->hoverBar = (b >= 0 && b < getBarCount() - 1) ? (int)b : -1; }
    return RTX_FAIL;                                                     // never swallow the event: the chart keeps its own mouse behaviour
#endif
}

extern "C" cppExtension* CreateExtension(void) {
    LiquidityProfile* p = new LiquidityProfile();
    p->setArrayCount(1);
#ifdef LQ_FLOW
    p->setFlags(POST_DRAWING | NO_UI | VAP_REQUIRED | TRACK_MOUSE);                    // its own pane (no OVERLAY), draws itself
    p->setExtendedFlags(CALL_CONTINUOUSLY);
    p->setDescription("LRA Liquidity Flow: the process at the zone price is in. AGGRESSIVE (market orders): buyers (green) and sellers (red) trading into the zone - Attack, Exh (the attackers slowed), Counter (the other side turned aggressive). PASSIVE (resting orders): buyers' bids (green) and sellers' offers (red), net added since arrival - Eaten, Pulled, Rebuilt. Each reads Metric: now (line), the line set per market by the nightly study. Held / Broke at a touch = the zone rule (zone = centre +/- 0.06 ADR; held = a 3-min close 0.10 ADR back out; broke = a close 0.10 ADR beyond with no close back inside within 15 min), written only once decided.");
#else
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | VAP_REQUIRED | TRACK_MOUSE);   // (0.15.0) TRACK_MOUSE: the zone hover
    p->setExtendedFlags(CALL_CONTINUOUSLY);   // the book keeps moving while no trade prints (#36)
    p->setDescription("LRA Liquidity Profile (LIQ): resting orders by price pointing toward price, top 3 zones per side with lots and x normal (solid = rested, hollow = new); old 64% + outline = half+ of the zone's size has rested 10 min+ (only in markets where the study confirmed it). Hover a near zone: bids / offers and the level they sit on, the orders (count, average lots, top order's share; mostly one order at 70%+ - may not reload ?), whether it was building or leaving as price came (+/-10% vs when price first came within reach), and while price is in it the lots hit and the share re-added ?. LQ? = a 2.5x+ zone on a key / gamma level (unproven). The signals are on the Liquidity Flow panes. TESTING.");
#endif
    p->setVersion("0.22.8");   // LQ_VERSION
    return p;
}
