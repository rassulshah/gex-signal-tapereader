#ifndef LS_ORDERFLOWFEED_H
#define LS_ORDERFLOWFEED_H
// OrderFlowFeed.h (1.7.0, 2026-10-21, MBO Roadmap Top Defender #1 "is the level absorbing the attack?"): Acc counts refills PER PRICE
//   (refAt) and absorbedPx() lists, for a decided candle, the resting prices where the defenders were filled AND reloaded - the
//   order-book truth used by the absorption test (research_testing.md: traded >= 2x the most ever shown there, >= 2 refills).
//   lsBidAskStats posts them on the SignalBoard (kind 'O') for lsDeltaProfile / lsBigBidAsk. Nothing else changes.
// OrderFlowFeed.h (1.6.0, 2026-10-14 night) -- the ONE shared order-flow header of lsBidAskStats 1.7.0, lsPullStats 1.6.0 and
// lsStackStats 1.4.0 (and the order feed of lsLiquidityProfile / lsLiquidityFlow 0.17.0).
// 1.6.0 (2026-10-14 night, Rassul approved 22:37 CT, mockup-one-bar.png v2): ONE BAR PER CANDLE, as an opt-in mode of drawBars (its new
//   last argument oneBull = the pane's buyer side; -1 = the old two bars, the default for every other user of this header). The bar =
//   the bigger side (oneBarOf): its colour, its lots as the height; a tie goes to the side of the candle's close direction (BarLook.dir),
//   else red. Bright only when the WINNING side is a spike (its spike label above, CR5300 persistence unchanged); dim otherwise. A
//   WHITE TICK (2 px, across the bar) at the smaller side's lots ONLY when BOTH sides spiked (a two-sided fight). S / L / R? letters and
//   the refill dash on the single bar for the winner; the loser's letters stack in the same column only on a both-spiked candle (else
//   they stay in the hover / record). marks = false (StackStats only, Rassul 23:00 CT correction): NO letters and NO refill dash drawn -
//   the tick is its only white mark; the hover and the records keep them. Hover, records, settings, read lines and the axis (already
//   max of the two sides) unchanged.
//   MboPane: virtual oneBarBull() (-1 here; StackStats / PullStats return their bid side 0) and barMarks() (true; StackStats false).
// 1.5.0 (2026-10-14 night, Rassul via the coordinator: "every order-flow pane gets its own ONE read line in place of its box"):
//   - "Box shows: Read" (default) no longer draws the READ box (read + blue options line + the numbers hover grid). Each pane draws ONE
//     read line at the box's Position / Offset (planLine), ONLY while price is inside a key-level or gamma zone (ZoneRead.h: level
//     +/- 0.06 ADR, the Liquidity Profile zone rule), else nothing: BidAskStats "ES 7750 - sellers hitting 3.1x normal, price held ?",
//     StackStats "ES 7750 - bids stacking 2.3x normal ?", PullStats "ES 7750 - bids pulled before touch (45%) ?", always "?", green =
//     buyers / bullish, red = sellers / bearish, grey otherwise; locked when its window closes (record <Pane>\<stem>-zoneread-<session>.csv,
//     read back on load: no repaint). The mouse over the line shows its detail box (numbers, the normal, what / why / how / example, the
//     study and its caveat). "Box shows: Numbers" keeps the compact numbers box, the read line beside it.
//   - StackStats' refill / iceberg letter I is drawn "I?" (audit: inferred) - the record still stores I.
//   - Spike bars, spike labels (CR5300 label persistence), letters S / L, the feed clock, the session record: unchanged.
// 1.4.0 (2026-10-14 evening, Rassul's standing order "always record IRT data and keep the MBO files updated"; LIQ-DAY-2026-10-14 review:
//   the ES mbo_HH.csv held only 30-45% of the traded volume in busy periods, 96-100% pre-market): ROOT CAUSE - the order feed was read
//   only from the pane's calc / draw, which IRT ran every ~6-10 s on the busy 3-min chart (none while its layout tab is hidden), at most
//   5,000 events / ~8 ms per call, and IRT keeps only the last few thousand events per subscriber, so the rest were overwritten unread
//   (mbo_09.csv: events in 1-3 s bursts, then 5-9 s with none, 2,028 of 3,600 seconds empty). Now (MboPane = PullStats / StackStats):
//   a 250 ms clock per pane reads the stream (opened with the chart symbol spelled out) behind a structured-exception guard into a
//   queue (<= 400,000 events), each event goes to the recorder as it is read, and calc / draw route the queue in order before reading
//   more. Status: FEED_CLOCK (ticks, events by clock / chart, queue peak / dropped, faults) and MBO_COVERAGE = FILL lots read / bar
//   volume per closed candle (last bar, last 10, lowest with its time, since load). BidAskStats and the Liquidity Profile keep their
//   calc / draw drain (unchanged behaviour); they only gain the coverage counter. Tests: tests_of/test_feedclock_host.cpp.
// 1.3.1 (2026-10-14 CR5300; Rassul 11:59 CT, CLZ6 3-min BidAskStats: "the label for pace and lots on the red spike goes away but it
//   is there on the green spike. fix it .. also check other indicators"): ROOT CAUSE in spikeLabels (shared by all three panes):
//   pass 2 shrank the NEAREST already-placed full labels to their ratio alone whenever ANY other spike had no free spot (a spike under
//   the read box / hover grid, the forming bar, a crowded spot) - repeatedly, until no full label was left - so a label lost its pace
//   and lots because of a different, often far-away candle; which side lost depended only on which label was nearest. Fixed: labels
//   are decided in time order from older labels + fixed obstacles only (full above its bar, else the ratio alone for itself), never
//   shrinking another; the hover grid no longer re-lays out labels (it is drawn over them); candles just left of the view take part
//   in the layout so a scroll does not re-decide a label in view. Tests: tests_of/test_label_persist.cpp.
// 1.3.0 (2026-10-14, CR4800; Rassul 07:02 "make them more compact so you will have room for the read", 07:16 "remove the white boxes,
//   just keep the signals and use white font instead", 07:18 "test them thoroughly ... dont shake or act erratic, and dont have garbage
//   values", 07:22 "replace the info box with a read box because there isn't enough room for both"):
//   - THE PANEL: "Box shows" Read (default) = a fixed READ box (the plain-language read, max 3 lines, + the blue options line); the
//     numbers table moves to the hover over that box (all rows, same order / colours, flowing into column groups). Numbers = the
//     compact numbers box (columns sized to the rows that fit) + the read beside it, wrapped to 2 lines instead of cut.
//   - Offset X / Y (BoxPlace.h): the box placed by Position, then nudged, always fully inside the pane and left of the last bar.
//   - Letters: white text, no box, no border (R stop-run marks keep their red / green text). Spike labels: ONE per candle (the bigger
//     side), never over the box or the read lines (a pane under 80 px used to get no text reserve - labels drew over the read).
//   - L (one large order): the candle's biggest order must ALSO be in the top 10% (largeBarPct) of the prior candles' biggest
//     orders (20+ candles this session; none before) - the per-ORDER 99th percentile alone (20 lots in ES) fired on ~every candle.
//   - Settings are read from the per-market file on every draw (IRT hands the chart copy list 0 / boxes unchecked); the dialog is
//     filled from the file when it opens, and a field the user did not touch is never overwritten (BoxPlace.h DlgSnap).
// 1.2.2 (2026-10-14, settings audit + Rassul's approved "Record order feed" for PullStats): MboPane gets four virtual hooks with
//   empty defaults (recorderPane / onRaw / afterPump / statusExtra) and PaneSettings an optional 7th field `record` (written only by
//   a recorder pane; a 6-field file reads record = 1). lsBidAskStats / lsStackStats / lsTapeReader behave exactly as before.
// 1.2.1 (2026-10-13 9:12 PM CT, live ES "Rebuilt 200% ?" on both sides): REBUILT credits an add at a hit price only up to the lots
//   hit there and not yet restored (Ice.rebuilt), so it can never exceed the hit size; rebuiltPctOf caps at 100% (was 200%). Nothing
//   else changes (the record format, pull / add / refill counts and every other number are identical).
// 1.2.0 (2026-10-13, approved by Rassul 4:49 PM CT "build"; SPEC /home/claude/aud/of140/SPEC.md, mockup orderflow-contracts-mockup.png):
//   - Pull / Stack bars in LOTS (contracts pulled / added near price; were order counts). Spike labels: 3 rows - size ratio "3.4x" /
//     pace "p 2.3x" / lots "240".
//   - PACE = lots per second in the candle vs that market's same-time-of-day norm (PACE_NORM rows of LRA-OrderFlow-<MKT>.csv, 30-min
//     CT buckets, written nightly by lra.orderflow_params; no row = the rolling norm of the last <= 60 closed candles of the prior
//     4 h, needs 5). "fast (p 2.6x)" >= paceFastX (2.0), "slowly (p 0.7x)" <= paceSlowX (0.7) on a spike; '?' until PACE_SHOW,on.
//   - Engine: per side the pulled order's "traded first" (the biggest pulled order had a fill before it was pulled), the hit size
//     (fills) and the size REBUILT at a hit price after the hit (lra passive_aggr's restore, "rebuilt %"), aggressor orders / lots,
//     and per-second add samples for PullStats' later "reset" message (bids back within N s of a pull: posted when known).
//   - The session record gains 15 numbers at the END (older 38-number records still load, the new ones read 0).
//   - The blue options stat line (OptionsFlow.h, LRA-Options-<MKT>.csv) under each message line; gates PACE_SHOW / OHIT_SHOW /
//     OPULL_SHOW / OSTACK_SHOW / REBUILT_SHOW (on / ? / off).
// 1.1.0 (2026-10-13, Rassul 1:36 PM: stop run R in BidAskStats): BarLook.mark[side] draws a coloured mark ("R" / "R?") in a white
//   box with no border at the BOTTOM of that side's bar, the side's top letter above it (still at most 2 per side); the session
//   record keeps extra lines (SessionFile.extra, e.g. "SR|...") after the candle rows. PullStats / StackStats draw exactly as 1.0.1.
// 1.0.1 (2026-10-13, Rassul 1:43 PM CT): no WHITE OUTLINE on the histogram bars or the letter boxes in any pane. Every bar,
//   letter box and refill dash is drawn by fillFlat(): the pen is set to the fill colour first (IRT draws a rectangle's border
//   with the pen in effect), border = fill. The hovered candle is no longer outlined white (the info box shows which candle).
//   The info box keeps its grey solid border (it is a box, not a bar).
// 1.0.1 (2026-10-13, Rassul 1:30 PM CT): StackStats' refill / iceberg letter is I (was R; R is reserved for a future stop-run
//   signal). The session record reader accepts S, L, I and the legacy R, and reads R back as I. Column names unchanged. Rassul 2026-10-13: "i want to identify spikes" / "number of bids and ask pulled like a histogram" /
// "add a stacking indicator" / "keep seperate" / "include L for Bid Ask Stacks and Pull" / "why dont you just use a dash in the bar".
//
// PART 1 (SDK-free, unit-tested on Linux): the order-by-order (DBO / MBO) engine - every NEW / MOD / DEL / FILL classified into
//   per-candle numbers near price: orders PULLED, orders ADDED, REFILLS (iceberg reloads), ICEBERG confirmation, aggressor SWEEPS
//   and LARGE lots; the session size distributions behind the "large" thresholds; the per-market params file; the stats as of each
//   bar (ratio / spike, as lsBidAskStats 1.2.0); letters / bars / two-line layout; the per-session record file.
// PART 2 (#ifdef OFF_SDK, after irtsdk.h): the DEPTH_BY_ORDER connection and drain exactly as lsMboCheck 1.1.0 (one object per
//   chart kept alive, opened from calc / draw only behind a crash-guard file, <= 5,000 events or ~8 ms per call with the backlog
//   carried, bDboSubscribed through a pointer-to-member), the shared drawing, and MboPane<L> - the whole host of the two MBO panes.
//
// THE FEED (IRT DEPTH_BY_ORDER, checked on his own recordings for lsPulling 0.2.x): NEW = an order rests (buySell 1 = a bid, 0 = an
// offer); FILL = part or all of a RESTING order traded (orderID = the resting order, aggressorOrderID = the order that hit it, size =
// lots filled); then the resting order is DELeted (all filled) or MODified (the rest); DEL = the order left the book; MOD = a new
// price or a new size.
//   PULLED (who is LEAVING):  DEL of an order whose last event was not a FILL           -> +1 order, + its resting size
//                             DEL of an order never seen (rested before load)           -> +1 order, + the DEL's size
//                             MOD to a new price, the OLD price near                    -> +1 order, + its old size
//                             MOD to a smaller size at the same price (not after a fill)-> contracts only (a partial pull)
//                             DEL right after a FILL of that order                      -> NOT a pull (the trade took it)
//   ADDED (who is ARRIVING):  NEW near; MOD to a new price, the NEW price near          -> +1 order, + its size
//                             MOD to a bigger size at the same price                    -> contracts only
//   REFILL:                   after a FILL at a price, a NEW (or a size increase) on the SAME side at the SAME price within
//                             refillWindowMs (default 1000) -> +1 refill. Checked on the arrival clock AND on the event stamps
//                             (whole seconds, at most ceil(window / 1 s) apart) so a drained backlog cannot squeeze old events in.
//   ICEBERG confirmed:        at a refill price, the bar's traded volume there > the most ever DISPLAYED there in the bar
//                             (a price where a fill hit an order never seen is not judged).
//   SWEEP:                    one aggressorOrderID filling at >= sweepLevels (3) distinct prices in the candle.
//   LARGE aggressive order:   one aggressorOrderID's filled lots in the candle >= largeLotAggr.
//   LARGE resting order:      one order pulled (PullStats) / added (StackStats) with size >= largeLotResting.
//   Large thresholds: 0 in the params file = auto = the largeLotPct (99th) percentile of the sizes seen THIS SESSION BEFORE the
//   candle (aggressor orders / resting orders near price), never under largeLotMin (20) lots. No look-ahead.
//   NEAR = |price - the last trade price| <= the Liquidity Profile 0.14.0 near band (NearLevels.h).
#include "LargeLotPool.h"
#include "NearLevels.h"
#include "OptionsFlow.h"
#include "HoverBoard.h"
#include "BoxPlace.h"
#include "ZoneRead.h"          // (1.5.0) the one-line zone reads
#include <algorithm>
#include <cctype>
#include <climits>   // (1.17.0) capTop
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

namespace off {

static const char* const VERSION = "1.10.1";   // 1.10.1: box rows cut (Aggression: Lots, Pace; Book Flow: Net, Pace);   // 1.10.0 (2026-10-16 22:00, Rassul): Book Flow net-bar labels = ratio (yellow) over pace (grey), no lots; Aggression + Book Flow show the left-justified numbers box in their own pane (boxOnly);   // 1.9.0 (2026-10-16 18:04-18:16, Rassul): Aggression + Book Flow draw NO text lines (textOff); ratios against the plain N-bar average (the 30-min slot yardstick is off); spike labels = ratio above the bar, lots + pace inside it, no "p"; Aggression labels only bars of 2.0x or more (onlyHigh), the rest drawn dim;   // 1.8.1: no hovers on the panes (Rassul 18:57);   // 1.8.0 (2026-10-15 audit #1): ratios / spikes against the same 30-min slot (log mean / 2 SD) over this + up to 4 earlier sessions;   // 1.7.0 (2026-10-15): the bar hover - the hovered candle's answer beside it in every pane;   // 1.6.1 (2026-10-15): the pane name + question in front of the read line;   // 1.6.0 (2026-10-14 night): one bar per candle (opt-in drawBars mode);   // 1.5.0 (2026-10-14 night): the READ box -> one zone read line (ZoneRead.h), I -> I?;   // 1.4.0 (2026-10-14 evening): 250 ms order-feed clock + queue, MBO_COVERAGE; 1.3.1 (2026-10-14 CR5300): spike labels never lose rows to another spike (time-order layout, no shrink pass), scroll-stable; 1.3.0 (2026-10-14 CR4800): read box + numbers hover, offsets, white letters, one spike label per candle, rare-L;   // 1.2.1 (2026-10-13 9:30 PM): rebuilt % <= 100% (per-price credit); 1.2.0 (2026-10-13): lots, pace, rebuilt, reset, options line; 1.1.0: stop-run mark slot + extra record lines; 1.0.1: letter I
typedef long long Tick;
enum Side { BID = 0, ASK = 1 };                    // BID = resting buy orders, ASK = resting sell orders
enum Act { NEW = 0, MOD = 1, DEL = 2, FILL = 3 };
using nl::finite;

static const size_t BOOK_CAP = 200000;             // tracked orders at most (far ones dropped first)
static const size_t PER_ORDER_CAP = 50000;         // per bar per side: orders tracked for the concentration line
static const size_t ICE_CAP = 2000;                // per bar per side: prices tracked for the iceberg check
static const size_t FILLMEM_CAP = 5000;            // per side: recent fill prices remembered for refills
static const size_t AGGR_CAP = 50000;              // per bar: aggressor orders tracked
static const size_t REST_SIZES_CAP = 20000;        // per bar: resting sizes kept for the session distribution
static const int AGGR_PX_MAX = 24;                 // distinct prices remembered per aggressor order (more = "24+")
static const long long SIZE_MAX_SANE = 10000000;   // a size above this is garbage
static const long long REF_STALE_MS = 5000;        // no fill for 5 s: the chart's close is the last trade price

inline bool toTick(double price, double tick, Tick& out) {
    if (!finite(price) || !finite(tick) || tick <= 0) return false;
    const double q = price / tick;
    if (!finite(q) || q > 9.0e15 || q < -9.0e15) return false;
    out = (Tick)(q >= 0 ? std::floor(q + .5) : std::ceil(q - .5));
    return true;
}

// =====================================================================================================================
// PARAMS: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-OrderFlow-<MKT>.csv, written by the nightly (format: NOTES.md)
//   VERSION,1 / MARKET,<MKT> / PARAM,<name>,<value> ... ('#' lines and blank lines ignored). Missing / bad -> defaults, each
//   reason listed in the status file. A bad VERSION / MARKET = the whole file ignored; a bad value = that one default.
// =====================================================================================================================
struct Params {
    long long sweepLevels = 3, largeLotAggr = 0, largeLotResting = 0, largeLotPct = 99, largeLotMin = 20;
#ifndef OFF_LARGE_BAR_MIN_DEFAULT
#define OFF_LARGE_BAR_MIN_DEFAULT 20   // the 1.2.x host suites build with 0 (the per-order rule alone) to keep their scenarios; 1.3.0 tests use 20
#endif
    long long largeBarPct = 90, largeBarMin = OFF_LARGE_BAR_MIN_DEFAULT;  // (1.3.0) L: the biggest order vs the prior candles' biggest (top 10%, 20+ candles)
    long long refillWindowMs = 1000, refillNotableT = 20;                 // tenths: 20 = 2.0x
    long long spikeAvgT = 20, spikeJumpT = 15, spikeLookback = 10;        // tenths / tenths / bars
    long long closeTicks = 2, baitPct = 70;
    long long paceFastT = 20, paceSlowT = 7;                              // (1.2.0) tenths: pace >= 2.0x = "fast", <= 0.7x = "slowly"
    long long rebuiltPct = 90, resetWindowSec = 60, resetPct = 80;        // (1.2.0) "rebuilt N% after hits" from 90%; reset: 80% back within 60 s
    std::set<std::string> fromFile;                                       // the names the file set (valid)
    bool file(const char* n) const { return fromFile.count(n) > 0; }
    int gate[ofo::NGATE] = { ofo::GATE_Q, ofo::GATE_Q, ofo::GATE_Q, ofo::GATE_Q, ofo::GATE_Q, ofo::GATE_Q, ofo::GATE_Q };   // (1.2.0) PACE_SHOW ... OHVL_SHOW (all ? by default)
    std::map<int, double> norm[3];                                        // (1.2.0) PACE_NORM: metric (0 AGGR, 1 PULL, 2 ADD) -> 30-min bucket -> lots/s
};
struct ParamDef { const char* name; bool tenths; long long lo, hi; long long Params::* slot; };
static const ParamDef PARAM_DEFS[] = {
    { "sweepLevels", false, 2, 20, &Params::sweepLevels },
    { "largeLotAggr", false, 0, 100000, &Params::largeLotAggr },
    { "largeLotResting", false, 0, 100000, &Params::largeLotResting },
    { "largeLotPct", false, 50, 100, &Params::largeLotPct },
    { "largeLotMin", false, 1, 100000, &Params::largeLotMin },
    { "largeBarPct", false, 50, 99, &Params::largeBarPct },
    { "largeBarMin", false, 5, 500, &Params::largeBarMin },
    { "refillWindowMs", false, 50, 60000, &Params::refillWindowMs },
    { "refillNotableX", true, 10, 100, &Params::refillNotableT },
    { "spikeAvgX", true, 0, 100, &Params::spikeAvgT },
    { "spikeJumpX", true, 0, 100, &Params::spikeJumpT },
    { "spikeLookback", false, 0, 50, &Params::spikeLookback },
    { "closeTicks", false, 0, 20, &Params::closeTicks },
    { "baitPct", false, 50, 100, &Params::baitPct },
    { "paceFastX", true, 10, 100, &Params::paceFastT },
    { "paceSlowX", true, 1, 10, &Params::paceSlowT },
    { "rebuiltPct", false, 10, 200, &Params::rebuiltPct },
    { "resetWindowSec", false, 5, 600, &Params::resetWindowSec },
    { "resetPct", false, 10, 200, &Params::resetPct },
};
// (1.2.0) PACE_NORM,<AGGR|PULL|ADD>,<HHMM bucket start, a multiple of 30 min>,<lots per second per side>
static const char* const PACE_METRIC[3] = { "AGGR", "PULL", "ADD" };
enum { PM_AGGR = 0, PM_PULL = 1, PM_ADD = 2 };
inline int bucketOf(int minuteOfDay) { if (minuteOfDay < 0 || minuteOfDay >= 1440) return -1; return (minuteOfDay / 30) * 30; }
static const size_t PARAM_COUNT = sizeof(PARAM_DEFS) / sizeof(PARAM_DEFS[0]);
inline std::string tenthsText(long long t) { char b[32]; std::snprintf(b, sizeof(b), "%lld.%lldx", t / 10, t % 10); return b; }
struct ParamLoad { Params p; std::vector<std::string> notes; std::string source = "defaults"; };
inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
inline std::vector<std::string> csv(const std::string& line) {
    std::vector<std::string> c; size_t a = 0;
    for (;;) { const size_t p = line.find(',', a); c.push_back(trim(line.substr(a, p == std::string::npos ? std::string::npos : p - a))); if (p == std::string::npos) break; a = p + 1; }
    return c;
}
// fileFound = false: no file (defaults). lines = the file's lines (bounded read done by the caller).
inline ParamLoad parseParams(bool fileFound, bool readOk, const std::vector<std::string>& lines, const std::string& market) {
    ParamLoad L;
    if (!fileFound) { L.source = "defaults (no params file)"; return L; }
    if (!readOk) { L.source = "defaults (params file unreadable or too big)"; L.notes.push_back("file: unreadable / over 64 KB / over 400 lines"); return L; }
    bool haveVer = false, haveMkt = false; Params p; std::vector<std::string> notes;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string ln = trim(lines[i]);
        if (ln.empty() || ln[0] == '#') continue;
        const std::vector<std::string> c = csv(ln);
        if (c[0] == "VERSION") { if (c.size() != 2 || c[1] != "1") { L.source = "defaults (unsupported VERSION)"; L.notes.push_back("VERSION must be 1"); return L; } haveVer = true; continue; }
        if (!haveVer) { L.source = "defaults (VERSION line missing)"; L.notes.push_back("first line must be VERSION,1"); return L; }
        if (c[0] == "MARKET") {
            if (c.size() != 2 || c[1] != market) { L.source = "defaults (MARKET mismatch)"; L.notes.push_back("MARKET is " + (c.size() > 1 ? c[1] : std::string("?")) + ", chart is " + market); return L; }
            haveMkt = true; continue;
        }
        if (c[0] == "ASOF" || c[0] == "SOURCE") continue;                  // informational
        {   // (1.2.0) the show gates: <KEY>,<on|?|off>
            int g = 0; while (g < ofo::NGATE && c[0] != ofo::GATE_KEY[g]) ++g;
            if (g < ofo::NGATE) {
                if (c.size() == 2 && (c[1] == "on" || c[1] == "?" || c[1] == "off")) p.gate[g] = c[1] == "on" ? ofo::GATE_ON : c[1] == "off" ? ofo::GATE_OFF : ofo::GATE_Q;
                else notes.push_back(std::string(ofo::GATE_KEY[g]) + ": bad value (on, ?, off) - ?");
                continue;
            }
        }
        if (c[0] == "PACE_NORM") {                                         // (1.2.0) per-market same-time-of-day pace norms
            int m = 0; while (m < 3 && (c.size() < 2 || c[1] != PACE_METRIC[m])) ++m;
            long long hhmm = -1; double v = 0;
            if (c.size() != 4 || m >= 3 || c[2].size() != 4 || !nl::parseWhole(c[2], 0, 2359, hhmm) || hhmm % 100 >= 60 || ((hhmm / 100) * 60 + hhmm % 100) % 30 != 0
                || !nl::parseReal(c[3], v) || !(v > 0) || v > 1e6) { notes.push_back("line " + std::to_string(i + 1) + ": bad PACE_NORM (ignored)"); continue; }
            if (p.norm[m].size() < 48) p.norm[m][(int)((hhmm / 100) * 60 + hhmm % 100)] = v;
            continue;
        }
        if (c[0] != "PARAM") { notes.push_back("line " + std::to_string(i + 1) + ": unknown key " + c[0].substr(0, 20) + " (ignored)"); continue; }
        if (c.size() != 3) { notes.push_back("line " + std::to_string(i + 1) + ": PARAM needs name,value (ignored)"); continue; }
        size_t k = 0; while (k < PARAM_COUNT && c[1] != PARAM_DEFS[k].name) ++k;
        if (k == PARAM_COUNT) { notes.push_back("unknown PARAM " + c[1].substr(0, 30) + " (ignored)"); continue; }
        const ParamDef& d = PARAM_DEFS[k]; long long v = 0; bool ok = false;
        if (d.tenths) { double x = 0; if (nl::parseReal(c[2], x) && x >= 0 && x < 1e6) { v = (long long)std::llround(x * 10.0); ok = v >= d.lo && v <= d.hi; } }
        else ok = nl::parseWhole(c[2], d.lo, d.hi, v);
        if (!ok) {
            const long long def = Params().*(d.slot);
            notes.push_back(std::string(d.name) + ": bad value '" + c[2].substr(0, 20) + "' (" + (d.tenths ? tenthsText(d.lo) + ".." + tenthsText(d.hi) : std::to_string(d.lo) + ".." + std::to_string(d.hi))
                + ") - default " + (d.tenths ? tenthsText(def) : std::to_string(def)));
            continue;
        }
        if (p.fromFile.count(d.name)) notes.push_back(std::string(d.name) + ": duplicate (the last one used)");
        p.*(d.slot) = v; p.fromFile.insert(d.name);
    }
    if (!haveVer) { L.source = "defaults (empty params file)"; L.notes.push_back("no VERSION line"); return L; }
    if (!haveMkt) { L.source = "defaults (MARKET line missing)"; L.notes.push_back("no MARKET line"); return L; }
    L.p = p; L.notes = notes; L.source = "file (" + std::to_string(p.fromFile.size()) + " params)";
    return L;
}
inline std::string paramsText(const Params& p) {
    std::string o;
    for (size_t k = 0; k < PARAM_COUNT; ++k) {
        const ParamDef& d = PARAM_DEFS[k]; const long long v = p.*(d.slot);
        if (k) o += ";";
        o += std::string(d.name) + "=" + (d.tenths ? tenthsText(v) : std::to_string(v)) + (p.file(d.name) ? "(file)" : "(default)");
    }
    for (int g = 0; g < ofo::NGATE; ++g) o += std::string(";") + ofo::GATE_KEY[g] + "=" + ofo::gateText(p.gate[g]);   // (1.2.0)
    for (int m = 0; m < 3; ++m) {
        o += std::string(";PACE_NORM_") + PACE_METRIC[m] + "=" + std::to_string(p.norm[m].size());
        for (std::map<int, double>::const_iterator it = p.norm[m].begin(); it != p.norm[m].end(); ++it) { char b[48]; std::snprintf(b, sizeof(b), ",%d:%.4g", it->first, it->second); o += b; }
    }
    return o;
}

// =====================================================================================================================
// SIZE DISTRIBUTION (this session): exact counts per size; percentile = nearest rank
// =====================================================================================================================
struct SizeDist {
    std::map<long long, long long> n; long long total = 0;
    static const long long CAP = 1000000; static const size_t KEYS = 4096;
    void clear() { n.clear(); total = 0; }
    void add(long long s, long long k = 1) {
        if (s <= 0 || k <= 0) return;
        if (s > CAP) s = CAP;
        std::map<long long, long long>::iterator it = n.find(s);
        if (it == n.end() && n.size() >= KEYS) {                          // bounded: fold into the nearest smaller size
            it = n.upper_bound(s); if (it != n.begin()) --it;
            it->second += k; total += k; return;
        }
        n[s] += k; total += k;
    }
    long long pct(long long p) const {
        if (total <= 0 || n.empty()) return 0;
        if (p >= 100) return n.rbegin()->first;
        long long need = (p * total + 99) / 100; if (need < 1) need = 1;
        long long c = 0;
        for (std::map<long long, long long>::const_iterator it = n.begin(); it != n.end(); ++it) { c += it->second; if (c >= need) return it->first; }
        return n.rbegin()->first;
    }
    std::string save() const {
        std::string o; for (std::map<long long, long long>::const_iterator it = n.begin(); it != n.end(); ++it) { if (!o.empty()) o += ","; o += std::to_string(it->first) + ":" + std::to_string(it->second); }
        return o;
    }
    bool load(const std::string& s) {                                     // strict; false = left unchanged
        SizeDist d; if (s.empty()) { *this = d; return true; }
        if (s.size() > 200000) return false;
        size_t a = 0;
        for (;;) {
            const size_t c = s.find(',', a); const std::string f = s.substr(a, c == std::string::npos ? std::string::npos : c - a);
            const size_t k = f.find(':'); long long sz = 0, ct = 0;
            if (k == std::string::npos || !nl::parseWhole(f.substr(0, k), 1, CAP, sz) || !nl::parseWhole(f.substr(k + 1), 1, 1000000000000LL, ct)) return false;
            d.add(sz, ct);
            if (c == std::string::npos) break;
            a = c + 1;
        }
        *this = d; return true;
    }
};
inline long long largeThreshold(const SizeDist& d, long long fixed, long long pct, long long minLots) {
    if (fixed > 0) return fixed;
    const long long v = d.pct(pct);
    return v > minLots ? v : minLots;
}
// (1.3.0, Rassul 07:16 "why are there so many") a candle has thousands of orders, so its biggest one almost always beats the per-ORDER
// 99th percentile: L fired on ~every candle (ES: 46 of 46). L now also needs the candle's biggest order to be in the top
// (100 - largeBarPct)% of the PRIOR candles' biggest orders (per side, this session, at least largeBarMin candles; before that no L).
// A fixed lot size in the params file still wins. 0 = no threshold (no L).
inline long long rareThreshold(const SizeDist& orders, const SizeDist& bars, long long fixed, const Params& P) {
    if (fixed > 0) return fixed;
    const long long o = largeThreshold(orders, 0, P.largeLotPct, P.largeLotMin);
    if (P.largeBarMin <= 0) return o;                                     // the per-order rule alone (tests; the params file allows 5..500)
    if (bars.total < P.largeBarMin) return 0;
    const long long b = bars.pct(P.largeBarPct) + 1;                      // strictly above the bar percentile
    return std::max(o, b);
}

// =====================================================================================================================
// THE ENGINE
// =====================================================================================================================
struct Ev {
    int action = NEW;          // 0..3, anything else = garbage
    int side = -1;             // BID / ASK, -1 = unknown (buySell 2)
    Tick px = 0;               // price in ticks (> 0)
    long long size = 0, id = 0, aggr = 0;   // size, the exchange order id, FILL: the aggressor's order id (0 = none)
    long long arrMs = 0;       // arrival (steady clock, ms)
    long long stamp = 0;       // the event's own time (RTDATE, whole seconds), 0 = none
};
inline int sideOfRaw(unsigned buySell) { return buySell == 1 ? BID : buySell == 0 ? ASK : -1; }
// a DEPTH_ORDER (any struct with its fields) -> Ev; a bad price -> px 0 (the engine counts it as garbage)
template <class O> Ev evOf(const O& o, double tick, long long arrMs) {
    Ev x; x.action = o.action <= 3 ? (int)o.action : 99; x.side = sideOfRaw((unsigned)o.buySell);
    x.size = (long long)o.size; x.id = (long long)o.orderID; x.aggr = (long long)o.aggressorOrderID; x.arrMs = arrMs; x.stamp = (long long)o.timestamp;
    if (!finite((double)o.price) || (double)o.price <= 0 || !toTick((double)o.price, tick, x.px)) x.px = 0;
    return x;
}

struct Ice { long long traded = 0, shown = 0, rebuilt = 0; bool unknown = false; };   // rebuilt: (1.2.1) lots credited as re-added at this price (<= traded)
struct Aggr { int side = -1; long long size = 0; Tick px[AGGR_PX_MAX]; int nPx = 0; bool more = false; };
// what one candle collected (open while the candle is forming / closing)
struct Acc {
    long long events = 0;
    long long pullN[2] = {0, 0}, pullC[2] = {0, 0}, pullMax[2] = {0, 0}, pullClose[2] = {0, 0};
    long long addN[2] = {0, 0}, addC[2] = {0, 0}, addMax[2] = {0, 0};
    long long refN[2] = {0, 0}, refC[2] = {0, 0};
    int pullMaxTraded[2] = {0, 0};                       // (1.2.0) the biggest pulled order had traded (a fill) before it was pulled
    long long hitC[2] = {0, 0}, rebuildC[2] = {0, 0};    // (1.2.0) lots filled (hit) / lots re-added at a price after a hit there this candle
    long long tFirst = -1, tLast = -1, fillLo = 0, fillHi = 0, tLo = -1, tHi = -1, cumLo[2] = {0, 0}, cumHi[2] = {0, 0};   // (DefenseTurn 2026-10-20) fill times (s), the candle's low / high fill and the hits up to it
    std::vector<std::pair<long long, long long> > addT[2];   // (1.2.0) (event stamp s, cumulative lots added near price) - the reset clock
    std::unordered_map<long long, long long> addBy[2];  // order id -> contracts added in this candle (concentration)
    bool orderCap = false, aggrCap = false;
    std::map<Tick, Ice> ice[2];
    std::set<Tick> refPx[2];                             // prices with a refill this candle
    std::map<Tick, int> refAt[2];                        // (1.7.0) refills per price this candle (capped like refPx)
    std::unordered_map<long long, Aggr> aggr;            // aggressor order id -> what it filled this candle
    long long aggrUnknown = 0;                           // fills without an aggressor id
    std::vector<long long> restSizes;                    // resting sizes added near price (the session distribution)
};
// (1.7.0) ORDER-BOOK ABSORPTION at one resting price of a decided candle: the defenders' resting orders there traded at least
// minX x the most ever shown there (hidden / refilled size) AND were reloaded at least minRef times. s = the RESTING side (BID / ASK).
struct ObPx { Tick px = 0; long long traded = 0, shown = 0; int refills = 0; };
inline bool obAbsorbed(long long traded, long long shown, int refills, long long minX = 2, int minRef = 2) {
    return shown > 0 && traded >= minX * shown && refills >= minRef;
}
inline void absorbedPx(const Acc& a, int s, std::vector<ObPx>& out, long long minX = 2, int minRef = 2) {
    out.clear(); if (s != BID && s != ASK) return;
    for (std::map<Tick, Ice>::const_iterator i = a.ice[s].begin(); i != a.ice[s].end(); ++i) {
        if (i->second.unknown) continue;
        std::map<Tick, int>::const_iterator r = a.refAt[s].find(i->first);
        const int ref = r == a.refAt[s].end() ? 0 : r->second;
        if (!obAbsorbed(i->second.traded, i->second.shown, ref, minX, minRef)) continue;
        ObPx o; o.px = i->first; o.traded = i->second.traded; o.shown = i->second.shown; o.refills = ref; out.push_back(o);
    }
}
// a decided candle's numbers (all three indicators keep all of them; the nightly reads the record)
struct Rec {
    bool partial = false; long long events = 0;
    long long pullN[2] = {0, 0}, pullC[2] = {0, 0}, pullMax[2] = {0, 0}, pullClose[2] = {0, 0};
    long long addN[2] = {0, 0}, addC[2] = {0, 0}, addTop[2] = {0, 0}, addMax[2] = {0, 0};
    long long refN[2] = {0, 0}, refC[2] = {0, 0}, ice[2] = {0, 0}, iceShown[2] = {0, 0}, iceTraded[2] = {0, 0};   // the biggest iceberg price: shown / traded
    long long sweepLv[2] = {0, 0}, sweepSize[2] = {0, 0}, aggrMax[2] = {0, 0}, sweepIsMax[2] = {0, 0};   // [side of the RESTING orders hit]
    long long thrAggr = 0, thrRest = 0;                  // the large thresholds in force for this candle
    // (1.2.0) appended at the END of the record (a 38-number record from 1.0 / 1.1 reloads with these 0)
    long long pullTraded[2] = {0, 0}, hitC[2] = {0, 0}, rebuildC[2] = {0, 0}, aggrN[2] = {0, 0}, aggrC[2] = {0, 0};   // aggr*: [side of the RESTING orders hit]
    long long pace100[2] = {0, 0};                       // pace x 100 vs the norm (0 = n/a), decided once at the close
    long long paceNote[2] = {0, 0};                      // 0 = not notable, 1 = fast, 2 = slowly
    long long paceGate = 1;                              // the PACE_SHOW gate in force (0 off, 1 ?, 2 on)
};
static const int REC_NUMS_V1 = 38, REC_NEW = 15, REC_NUMS = REC_NUMS_V1 + REC_NEW;
static const char* const REC_COLS = "partial|events|pull_n_b|pull_n_a|pull_cts_b|pull_cts_a|pull_max_b|pull_max_a|pull_close_b|pull_close_a|"
    "add_n_b|add_n_a|add_cts_b|add_cts_a|add_top_b|add_top_a|add_max_b|add_max_a|refill_n_b|refill_n_a|refill_cts_b|refill_cts_a|iceberg_b|iceberg_a|"
    "sweep_lv_b|sweep_lv_a|sweep_size_b|sweep_size_a|aggr_max_b|aggr_max_a|sweep_is_max_b|sweep_is_max_a|ice_shown_b|ice_shown_a|ice_traded_b|ice_traded_a|thr_aggr|thr_rest|"
    "pull_traded_b|pull_traded_a|hit_cts_b|hit_cts_a|rebuilt_cts_b|rebuilt_cts_a|aggr_n_b|aggr_n_a|aggr_cts_b|aggr_cts_a|pace_x100_b|pace_x100_a|pace_note_b|pace_note_a|pace_gate";
inline void recNums(const Rec& r, std::vector<long long>& v) {
    v.push_back(r.partial ? 1 : 0); v.push_back(r.events);
    const long long* pairs[17] = { r.pullN, r.pullC, r.pullMax, r.pullClose, r.addN, r.addC, r.addTop, r.addMax, r.refN, r.refC, r.ice, r.sweepLv, r.sweepSize, r.aggrMax, r.sweepIsMax, r.iceShown, r.iceTraded };
    for (int k = 0; k < 17; ++k) { v.push_back(pairs[k][0]); v.push_back(pairs[k][1]); }
    v.push_back(r.thrAggr); v.push_back(r.thrRest);
    const long long* more[7] = { r.pullTraded, r.hitC, r.rebuildC, r.aggrN, r.aggrC, r.pace100, r.paceNote };
    for (int k = 0; k < 7; ++k) { v.push_back(more[k][0]); v.push_back(more[k][1]); }
    v.push_back(r.paceGate);
}
inline bool recFrom(const std::vector<long long>& v, size_t at, Rec& out) {
    if (v.size() < at + REC_NUMS || v[at] > 1) return false;
    Rec r; r.partial = v[at] == 1; r.events = v[at + 1];
    long long* pairs[17] = { r.pullN, r.pullC, r.pullMax, r.pullClose, r.addN, r.addC, r.addTop, r.addMax, r.refN, r.refC, r.ice, r.sweepLv, r.sweepSize, r.aggrMax, r.sweepIsMax, r.iceShown, r.iceTraded };
    for (int k = 0; k < 17; ++k) { pairs[k][0] = v[at + 2 + 2 * k]; pairs[k][1] = v[at + 3 + 2 * k]; }
    r.thrAggr = v[at + 36]; r.thrRest = v[at + 37];
    long long* more[7] = { r.pullTraded, r.hitC, r.rebuildC, r.aggrN, r.aggrC, r.pace100, r.paceNote };
    for (int k = 0; k < 7; ++k) { more[k][0] = v[at + 38 + 2 * k]; more[k][1] = v[at + 39 + 2 * k]; }
    r.paceGate = v[at + 52];
    for (int s = 0; s < 2; ++s) if (r.addTop[s] > r.addC[s] || r.sweepIsMax[s] > 1 || (r.ice[s] > 0 && r.iceTraded[s] <= r.iceShown[s]) || r.pullTraded[s] > 1 || r.paceNote[s] > 2) return false;
    if (r.paceGate > 2) return false;
    out = r; return true;
}

class Engine {
public:
    struct Ord { int side = BID; Tick px = 0; long long size = 0; bool filledLast = false; bool known = true; bool traded = false; };   // traded: (1.2.0) any fill
    Tick near = 0;                                     // the near band in ticks (0 = nothing is near)
    Tick ref = 0; bool haveRef = false; long long lastFillMs = -1;
    long long refillMs = 1000; Tick closeTicks = 2;
    std::unordered_map<long long, Ord> book;
    std::unordered_map<Tick, long long> rest[2];       // known resting size per price
    std::map<Tick, std::pair<long long, long long> > lastFill[2];   // price -> (arrival ms, stamp) of the latest fill there
    SizeDist aggrDist, restDist;                       // this session's aggressor order sizes / resting sizes near price
    // (2026-10-17, Large Lot Methodology) BidAskStats only: aggressor orders >= 10 lots go to per-channel pools (RTH / overnight / all, 5 sessions);
    // when a channel has enough orders, L = the 1SD tier of that pool (rank 84.13% of the orders >= mean + 2 SD); chan is set by the caller before close()
    llp::Pool lp; int chan = 0; bool useLP = false;
    SizeDist aggrBar, restBar;                         // (1.3.0) this session's per-candle, per-side BIGGEST aggressor / resting (add or pull) order
    long long events = 0, garbage = 0, unknownDel = 0, unknownMod = 0, unknownFill = 0, dupNew = 0, bookDrops = 0, bookResets = 0;
    long long perAction[4] = {0, 0, 0, 0};
    // (DefenseTurn 1.1.0, 2026-10-20 13:54 CT Rassul "okay build" - the gray E line) EXHAUSTION AT THE PRICE, per fresh-price touch: a fill at a
    // price untraded for 5 min (that side) starts a touch; the lots hit there in seconds 0-20 and 20-60 and the resting size at the touch /
    // at 60 s decide it once: ATTACK DIED = (lots/s 20-60) / (lots/s 0-20 + 0.05) <= the market's median slowdown AND >= half the size stayed.
    struct Touch { int s = BID; Tick px = 0; long long key = 0, t0 = 0, rest0 = 0, h1 = 0, h2 = 0; bool done = false, ad = false; };
    std::vector<Touch> touches;                       // newest last; bounded
    std::unordered_map<Tick, long long> lastTradeS[2]; // price -> last fill second (that resting side)
    long long curKey = 0, lastS = 0;                   // the candle the event belongs to (set by the caller); the newest event second
    double adMedian = 0.1;                             // the market's median slowdown (set by the caller; DefenseTurn.h table)
    void touchFill(int s, Tick px, long long size, long long ts, long long restBefore) {
        if ((s != BID && s != ASK) || px <= 0 || size <= 0 || ts <= 0 || ts < lastS) return;
        lastS = ts;
        std::unordered_map<Tick, long long>::iterator lt = lastTradeS[s].find(px);
        const long long prior = lt == lastTradeS[s].end() ? -1 : lt->second;
        const bool fresh = prior < 0 || ts - prior >= 300;                 // exactly five minutes untraded is fresh
        if (ts >= prior) lastTradeS[s][px] = ts;
        if (lastTradeS[s].size() > 20000) { for (std::unordered_map<Tick, long long>::iterator q = lastTradeS[s].begin(); q != lastTradeS[s].end();) if (lastS - q->second > 600) q = lastTradeS[s].erase(q); else ++q; }
        if (fresh) { Touch t; t.s = s; t.px = px; t.key = curKey; t.t0 = ts; t.rest0 = restBefore; touches.push_back(t); }
        for (size_t k = touches.size(); k-- > 0;) {
            Touch& t = touches[k];
            if (ts - t.t0 > 600) break;                                        // older touches are all finished
            if (t.done || t.s != s || t.px != px) continue;
            const long long dt = ts - t.t0;
            if (dt < 0) continue;                                              // stale MBO event: never count it in a later touch
            if (dt < 20) t.h1 += size; else if (dt < 60) t.h2 += size;
        }
        finishTouches(ts);
        if (touches.size() > 3000) touches.erase(touches.begin(), touches.begin() + 1000);
    }
    void finishTouches(long long nowS) {
        if (nowS > lastS) lastS = nowS;
        for (size_t k = touches.size(); k-- > 0;) {
            Touch& t = touches[k];
            if (lastS - t.t0 > 900) break;
            if (t.done || lastS - t.t0 < 60) continue;
            const double decel = ((double)t.h2 / 40.0) / ((double)t.h1 / 20.0 + 0.05);
            const long long left = restAt(t.s, t.px);
            t.ad = t.rest0 > 0 && finite(adMedian) && adMedian >= 0 && decel <= adMedian + 1e-12 && left * 2 >= t.rest0;
            t.done = true;
        }
    }

    bool isNear(Tick px) const { return haveRef && near > 0 && std::llabs(px - ref) <= near; }
    void fallbackRef(Tick px, long long nowMs) {
        if (px <= 0) return;
        if (lastFillMs < 0 || nowMs - lastFillMs > REF_STALE_MS || nowMs < lastFillMs) { ref = px; haveRef = true; }
    }
    void restAdd(int s, Tick px, long long d) {
        if (d == 0) return;
        std::unordered_map<Tick, long long>::iterator it = rest[s].find(px);
        if (it == rest[s].end()) { if (d > 0) rest[s][px] = d; return; }
        it->second += d; if (it->second <= 0) rest[s].erase(it);
    }
    long long restAt(int s, Tick px) const { std::unordered_map<Tick, long long>::const_iterator it = rest[s].find(px); return it == rest[s].end() ? 0 : it->second; }
    static void bump(std::unordered_map<long long, long long>& m, long long id, long long c, Acc& a) {
        if (c <= 0) return;
        std::unordered_map<long long, long long>::iterator it = m.find(id);
        if (it != m.end()) { it->second += c; return; }
        if (m.size() >= PER_ORDER_CAP) { a.orderCap = true; return; }
        m[id] = c;
    }
    void shown(Acc& a, int s, Tick px) {                // a price with fills this candle: the most displayed there
        std::map<Tick, Ice>::iterator it = a.ice[s].find(px);
        if (it != a.ice[s].end()) { const long long r = restAt(s, px); if (r > it->second.shown) it->second.shown = r; }
    }
    bool refillHit(int s, Tick px, const Ev& e) const {
        if (refillMs <= 0) return false;
        std::map<Tick, std::pair<long long, long long> >::const_iterator it = lastFill[s].find(px);
        if (it == lastFill[s].end()) return false;
        const long long dt = e.arrMs - it->second.first;
        if (dt < 0 || dt > refillMs) return false;
        if (e.stamp > 0 && it->second.second > 0) {
            const long long ds = e.stamp - it->second.second, maxS = (refillMs + 999) / 1000;
            if (ds < 0 || ds > maxS) return false;
        }
        return true;
    }
    void pull(Acc& a, int s, Tick px, long long c, bool wholeOrder, bool traded = false) {
        if (wholeOrder) {
            a.pullN[s]++;
            if (c > a.pullMax[s]) { a.pullMax[s] = c; a.pullMaxTraded[s] = traded ? 1 : 0; }
            if (haveRef && std::llabs(px - ref) <= closeTicks) a.pullClose[s]++;   // pulled just before price got there
        }
        if (c > 0) a.pullC[s] += c;
    }
    void add(Acc& a, int s, long long id, long long c, bool wholeOrder, Tick px = 0, long long stamp = 0) {
        if (wholeOrder) {
            a.addN[s]++;
            if (c > a.addMax[s]) a.addMax[s] = c;
            if (c > 0 && a.restSizes.size() < REST_SIZES_CAP) a.restSizes.push_back(c);
        }
        if (c > 0) {
            a.addC[s] += c; bump(a.addBy[s], id, c, a);
            if (px > 0) {                                                  // (1.2.0) rebuilt: added back at a price hit earlier this candle
                // (1.2.1, live 13 Oct 9:12 PM "Rebuilt 200%") credit at most the lots still un-restored at that price: every new order
                // joining the queue at a once-hit price used to count, so normal queue joins read as 200% (the cap) with a 1-lot hit
                std::map<Tick, Ice>::iterator h = a.ice[s].find(px);
                if (h != a.ice[s].end() && h->second.traded > h->second.rebuilt) {
                    const long long room = h->second.traded - h->second.rebuilt, k = c < room ? c : room;
                    h->second.rebuilt += k; a.rebuildC[s] += k;
                }
            }
            std::vector<std::pair<long long, long long> >& T = a.addT[s];   // (1.2.0) the reset clock: one sample per stamp second
            if (!T.empty() && T.back().first == stamp) T.back().second = a.addC[s];
            else if (T.size() < 4000) T.push_back(std::make_pair(stamp, a.addC[s]));
            else T.back().second = a.addC[s];
        }
    }
    void refill(Acc& a, int s, Tick px, long long c) { a.refN[s]++; a.refC[s] += c; if (a.refPx[s].size() < ICE_CAP) a.refPx[s].insert(px);
        if (a.refAt[s].size() < ICE_CAP || a.refAt[s].count(px)) a.refAt[s][px]++; }
    void aggressor(Acc& a, int restingSide, const Ev& e) {
        if (e.aggr == 0) { a.aggrUnknown++; return; }
        std::unordered_map<long long, Aggr>::iterator it = a.aggr.find(e.aggr);
        if (it == a.aggr.end()) {
            if (a.aggr.size() >= AGGR_CAP) { a.aggrCap = true; return; }
            it = a.aggr.insert(std::make_pair(e.aggr, Aggr())).first;
            it->second.side = restingSide;
        }
        Aggr& g = it->second;
        if (g.side != restingSide) return;                                 // an id seen on both sides: keep the first (garbage guard)
        g.size += e.size;
        for (int i = 0; i < g.nPx; ++i) if (g.px[i] == e.px) return;
        if (g.nPx < AGGR_PX_MAX) g.px[g.nPx++] = e.px; else g.more = true;
    }
    void apply(Acc& a, const Ev& e) {
        if (e.action < NEW || e.action > FILL || e.size < 0 || e.size > SIZE_MAX_SANE || e.px <= 0) { garbage++; return; }
        std::unordered_map<long long, Ord>::iterator it = book.find(e.id);
        int s = e.side;
        if (it != book.end() && e.action != NEW) s = it->second.side;     // the order's own side wins
        if (s != BID && s != ASK) { garbage++; return; }                  // an unknown side cannot be classified
        events++; perAction[e.action]++; a.events++;
        switch (e.action) {
        case NEW: {
            if (it != book.end()) {                                        // a duplicate / replayed NEW: refresh, never counted twice
                Ord& o = it->second; dupNew++;
                if (o.known) restAdd(o.side, o.px, -o.size);
                o.side = s; o.px = e.px; o.size = e.size; o.filledLast = false; o.known = true; o.traded = false;
                restAdd(s, e.px, e.size); shown(a, s, e.px);
                break;
            }
            Ord o; o.side = s; o.px = e.px; o.size = e.size; book[e.id] = o;
            restAdd(s, e.px, e.size); shown(a, s, e.px);
            if (isNear(e.px)) {
                add(a, s, e.id, e.size, true, e.px, e.stamp);
                if (refillHit(s, e.px, e)) refill(a, s, e.px, e.size);
            }
            break; }
        case FILL: {
            Ice* ic = nullptr;
            {
                std::map<Tick, Ice>::iterator ii = a.ice[s].find(e.px);
                if (ii != a.ice[s].end()) ic = &ii->second;
                else if (a.ice[s].size() < ICE_CAP) ic = &a.ice[s][e.px];
            }
            if (ic) { const long long r = restAt(s, e.px); if (r > ic->shown) ic->shown = r; ic->traded += e.size; }
            if (e.size > 0) a.hitC[s] += e.size;                           // (1.2.0) the hit size (rebuilt % = re-added / hit)
            {   const long long ts = e.stamp > 0 ? e.stamp : e.arrMs / 1000;     // (DefenseTurn 2026-10-20) when the candle's low / high printed and the hits up to it
                if (a.tFirst < 0) { a.tFirst = ts; }
                a.tLast = ts;
                if (a.fillLo == 0 || e.px < a.fillLo) { a.fillLo = e.px; a.tLo = ts; a.cumLo[0] = a.hitC[0]; a.cumLo[1] = a.hitC[1]; }
                if (a.fillHi == 0 || e.px > a.fillHi) { a.fillHi = e.px; a.tHi = ts; a.cumHi[0] = a.hitC[0]; a.cumHi[1] = a.hitC[1]; }
                if (e.size > 0) touchFill(s, e.px, e.size, ts, restAt(s, e.px)); }   // (DefenseTurn 1.1.0) before the book takes this fill
            if (it != book.end() && it->second.known) {
                Ord& o = it->second;
                const long long take = e.size < o.size ? e.size : o.size;
                restAdd(o.side, o.px, -take); o.size -= take; o.filledLast = true; o.traded = true;
            } else {
                unknownFill++;
                if (ic) ic->unknown = true;
                if (it == book.end()) { Ord o; o.side = s; o.px = e.px; o.size = 0; o.filledLast = true; o.known = false; o.traded = true; book[e.id] = o; }
                else { it->second.filledLast = true; it->second.traded = true; }
            }
            aggressor(a, s, e);
            ref = e.px; haveRef = true; lastFillMs = e.arrMs;              // a fill's price = the last trade
            lastFill[s][e.px] = std::make_pair(e.arrMs, e.stamp);
            if (lastFill[s].size() > FILLMEM_CAP) {                        // forget fills older than the window, then everything
                for (std::map<Tick, std::pair<long long, long long> >::iterator f = lastFill[s].begin(); f != lastFill[s].end();)
                    if (e.arrMs - f->second.first > refillMs) lastFill[s].erase(f++); else ++f;
                if (lastFill[s].size() > FILLMEM_CAP) lastFill[s].clear();
            }
            break; }
        case DEL: {
            if (it == book.end()) {                                        // rested before load: a real cancel
                unknownDel++;
                if (isNear(e.px)) pull(a, s, e.px, e.size, true);
                break;
            }
            const Ord o = it->second; book.erase(it);
            if (o.known) restAdd(o.side, o.px, -o.size);
            if (o.filledLast || !o.known) break;                           // just filled: the trade took it, not a pull
            if (isNear(o.px)) pull(a, o.side, o.px, o.size > 0 ? o.size : e.size, true, o.traded);
            break; }
        case MOD: {
            if (it == book.end() || !it->second.known) {                   // first sight: its state from now on, nothing counted
                unknownMod++;
                Ord o; o.side = s; o.px = e.px; o.size = e.size; book[e.id] = o;
                restAdd(s, e.px, e.size);
                break;
            }
            Ord& o = it->second;
            if (e.px != o.px) {                                            // moved: a pull at the old price + an add at the new
                if (isNear(o.px)) pull(a, o.side, o.px, o.size, true, o.traded);
                if (isNear(e.px)) add(a, o.side, e.id, e.size, true, e.px, e.stamp);
                restAdd(o.side, o.px, -o.size); restAdd(o.side, e.px, e.size); shown(a, o.side, e.px);
            } else {
                const long long d = e.size - o.size;
                if (d < 0 && !o.filledLast && isNear(o.px)) pull(a, o.side, o.px, -d, false);
                if (d > 0 && isNear(o.px)) {
                    add(a, o.side, e.id, d, false, o.px, e.stamp);
                    if (refillHit(o.side, o.px, e)) refill(a, o.side, o.px, d);
                }
                restAdd(o.side, o.px, d); if (d > 0) shown(a, o.side, o.px);
            }
            o.px = e.px; o.size = e.size; o.filledLast = false;
            break; }
        default: break;
        }
        if (book.size() > BOOK_CAP) trimBook();
    }
    // bounded memory: drop tombstones and orders far from price (> 4 near bands); if still too many, forget the book
    void trimBook() {
        const Tick far = near > 0 ? 4 * near : 400;
        for (std::unordered_map<long long, Ord>::iterator it = book.begin(); it != book.end();) {
            const Ord& o = it->second;
            if (!o.known || !haveRef || std::llabs(o.px - ref) > far) { if (o.known) restAdd(o.side, o.px, -o.size); it = book.erase(it); bookDrops++; }
            else ++it;
        }
        if (book.size() > BOOK_CAP * 9 / 10) { book.clear(); rest[0].clear(); rest[1].clear(); bookResets++; }
    }
    // a candle is decided: its numbers, the large thresholds AS OF the candles before it, then its sizes join the distributions
    // the numbers only (nothing recorded): the forming candle's live look uses this
    Rec numbers(const Acc& a, bool partial, const Params& P) const {
        Rec r; r.partial = partial; r.events = a.events;
        r.thrAggr = rareThreshold(aggrDist, aggrBar, P.largeLotAggr, P);
        if (useLP && P.largeLotAggr <= 0) { const llp::Tiers T = lp.tiersFor(chan); if (T.ok) r.thrAggr = std::max(1LL, T.t[0]); }   // a fixed lot size in the params file still wins
        r.thrRest = rareThreshold(restDist, restBar, P.largeLotResting, P);
        for (int s = 0; s < 2; ++s) {
            r.pullN[s] = a.pullN[s]; r.pullC[s] = a.pullC[s]; r.pullMax[s] = a.pullMax[s]; r.pullClose[s] = a.pullClose[s];
            r.addN[s] = a.addN[s]; r.addC[s] = a.addC[s]; r.addMax[s] = a.addMax[s];
            long long top = 0; for (std::unordered_map<long long, long long>::const_iterator i = a.addBy[s].begin(); i != a.addBy[s].end(); ++i) if (i->second > top) top = i->second;
            r.addTop[s] = top > r.addC[s] ? r.addC[s] : top;
            r.refN[s] = a.refN[s]; r.refC[s] = a.refC[s];
            r.pullTraded[s] = a.pullMaxTraded[s]; r.hitC[s] = a.hitC[s]; r.rebuildC[s] = a.rebuildC[s];   // (1.2.0)
            for (std::set<Tick>::const_iterator p = a.refPx[s].begin(); p != a.refPx[s].end(); ++p) {
                std::map<Tick, Ice>::const_iterator i = a.ice[s].find(*p);
                if (i != a.ice[s].end() && !i->second.unknown && i->second.shown > 0 && i->second.traded > i->second.shown) {
                    r.ice[s]++;
                    if (i->second.traded - i->second.shown > r.iceTraded[s] - r.iceShown[s]) { r.iceShown[s] = i->second.shown; r.iceTraded[s] = i->second.traded; }
                }
            }
        }
        long long bestLv[2] = {0, 0}, bestSz[2] = {0, 0};
        for (std::unordered_map<long long, Aggr>::const_iterator i = a.aggr.begin(); i != a.aggr.end(); ++i) {
            const Aggr& g = i->second; if (g.side != BID && g.side != ASK) continue;
            const int s = g.side; const long long lv = g.nPx + (g.more ? 1 : 0);
            if (g.size > r.aggrMax[s]) r.aggrMax[s] = g.size;
            r.aggrN[s]++; r.aggrC[s] += g.size;                            // (1.2.0) aggressor orders / their lots on this side
            if (lv > bestLv[s] || (lv == bestLv[s] && g.size > bestSz[s])) { bestLv[s] = lv; bestSz[s] = g.size; }
        }
        for (int s = 0; s < 2; ++s) { r.sweepLv[s] = bestLv[s]; r.sweepSize[s] = bestSz[s]; r.sweepIsMax[s] = bestSz[s] > 0 && bestSz[s] >= r.aggrMax[s] ? 1 : 0; }
        return r;
    }
    Rec close(const Acc& a, bool partial, const Params& P) {
        const Rec r = numbers(a, partial, P);
        for (std::unordered_map<long long, Aggr>::const_iterator i = a.aggr.begin(); i != a.aggr.end(); ++i) if (i->second.side == BID || i->second.side == ASK) { aggrDist.add(i->second.size); if (useLP) lp.add(chan, i->second.size); }
        for (size_t i = 0; i < a.restSizes.size(); ++i) restDist.add(a.restSizes[i]);
        if (!partial) for (int s = 0; s < 2; ++s) {                        // (1.3.0) the candle's biggest orders join the per-candle distributions
            if (r.aggrMax[s] > 0) aggrBar.add(r.aggrMax[s]);
            const long long rm = std::max(r.addMax[s], r.pullMax[s]); if (rm > 0) restBar.add(rm);
        }
        return r;
    }
    void newSession() { if (useLP) lp.roll(); aggrDist.clear(); restDist.clear(); aggrBar.clear(); restBar.clear(); }
    // (1.3.0) after a restart: the per-candle maxima come back from the session record's closed candles (no new file lines)
    void barFrom(const Rec& r) { if (r.partial) return; for (int s = 0; s < 2; ++s) { if (r.aggrMax[s] > 0) aggrBar.add(r.aggrMax[s]); const long long rm = std::max(r.addMax[s], r.pullMax[s]); if (rm > 0) restBar.add(rm); } }
};

// =====================================================================================================================
// STATS AS OF EACH BAR (the lsBidAskStats 1.2.0 rules)
//   ratio = the value / the mean of that side's prior `ratioBars` valid closed bars (needs 5); for the MBO counts the mean is
//   floored at 1 (a quiet history of 0.3 pulls a bar does not make 2 pulls a 6x spike).
//   spike = >= spikeAvg x that mean AND >= spikeJump x the biggest bar of EITHER side in the prior `lookback` valid bars.
// =====================================================================================================================
static const int RATIO_MIN_PRIOR = 5, RATIO_DEFAULT = 20, RATIO_LO = 5, RATIO_HI = 200, SPIKE_LOOKBACK_MAX = 50;
static const double HIGH_X = 2.0;                   // a side is HIGH (it gets the message) when a spike or >= 2.0x its average
struct SpikeCfg {
    long long avgT = 20, jumpT = 15, lookback = 10;
    bool on() const { return avgT > 0 && jumpT > 0 && lookback > 0; }
    bool operator==(const SpikeCfg& o) const { return avgT == o.avgT && jumpT == o.jumpT && lookback == o.lookback; }
    bool operator!=(const SpikeCfg& o) const { return !(*this == o); }
};
// the spike settings in force: the nightly params win where the file sets them; the dialog fills the rest; a dialog 0 = off
inline SpikeCfg effectiveSpike(const SpikeCfg& dialog, const Params& P) {
    SpikeCfg c;
    c.avgT = P.file("spikeAvgX") ? P.spikeAvgT : dialog.avgT;
    c.jumpT = P.file("spikeJumpX") ? P.spikeJumpT : dialog.jumpT;
    c.lookback = P.file("spikeLookback") ? P.spikeLookback : dialog.lookback;
    if (!dialog.on()) c.avgT = 0;                                          // the dialog's 0 = off always wins
    return c;
}
inline long long priorMaxEither(const std::vector<long long>& pb, const std::vector<long long>& pa, long long n) {
    long long mx = 0; if (n <= 0) return 0;
    const size_t k = pb.size() < (size_t)n ? pb.size() : (size_t)n;
    for (size_t i = pb.size() - k; i < pb.size(); ++i) { if (pb[i] > mx) mx = pb[i]; if (i < pa.size() && pa[i] > mx) mx = pa[i]; }
    return mx;
}
inline bool isSpike(long long v, double mean, long long priorMax, const SpikeCfg& c) {
    if (!c.on() || v <= 0 || !(mean > 0) || priorMax <= 0) return false;
    return (double)v * 10.0 >= (double)c.avgT * mean && v * 10 >= c.jumpT * priorMax;
}
inline double ratioOf(const std::vector<long long>& prior, long long v, int bars, bool& ok, double* meanOut = nullptr, double floor = 0) {
    ok = false; if (meanOut) *meanOut = 0;
    if (bars < RATIO_LO) bars = RATIO_LO;
    if (bars > RATIO_HI) bars = RATIO_HI;
    const size_t n = prior.size() < (size_t)bars ? prior.size() : (size_t)bars;
    if (n < (size_t)RATIO_MIN_PRIOR) return 0;
    double sum = 0;
    for (size_t i = prior.size() - n; i < prior.size(); ++i) sum += (double)prior[i];
    double mean = sum / (double)n;
    if (floor > 0 && mean < floor) mean = floor;
    if (!(mean > 0)) return 0;
    if (meanOut) *meanOut = mean;
    ok = true; return (double)v / mean;
}
// ---- (1.8.0, Rassul 2026-10-15 16:46 yardstick audit #1) THE SAME 30-MIN SLOT: a bar is measured against the bars of ITS half hour
// (bar END minute - 1) / 30 - from the earlier sessions on file and the earlier bars of this one (newest 80, values > 0), on the LOG
// scale: typical = e^mean (the ratio divides by it), spike = log(v) >= mean + 2 SD. Under 10 such bars: the 20-bar rule as before.
static const int SLOT_MIN_N = 10, SLOT_MAX_N = 80;
inline int slotOfKey(long long t) { const long long m = ((t % 86400) + 86400) % 86400 / 60; return (int)(((m == 0 ? 1440 : m) - 1) / 30); }
struct SlotLog { bool ok = false; double lm = 0, ls = 0; int n = 0; };
inline SlotLog slotLog(const std::vector<long long>& v) {
    SlotLog z; double s1 = 0, s2 = 0; int n = 0;
    for (size_t i = 0; i < v.size(); ++i) if (v[i] > 0) { const double x = std::log((double)v[i]); s1 += x; s2 += x * x; ++n; }
    z.n = n; if (n < SLOT_MIN_N) return z;
    z.lm = s1 / n; z.ls = std::sqrt(std::max(0.0, s2 / n - z.lm * z.lm)); z.ok = true; return z;
}
static const bool SLOT_YARDSTICK = false;   // (1.9.0, Rassul 2026-10-16 18:12 "2.8 should be using an average"): false = the plain average of the last N bars
inline void applySlot(long long v, const SlotLog& z, bool& ratioOk, double& ratio, double& mean, bool& spike) {
    if (!SLOT_YARDSTICK || !z.ok) return;
    mean = std::exp(z.lm); ratio = mean > 0 ? (double)v / mean : 0; ratioOk = mean > 0;
    spike = v > 0 && std::log((double)v) >= z.lm + 2.0 * z.ls;
}
// civil date from days since 1970-01-01 (H. Hinnant), for the earlier sessions' record names (YYYYMMDD)
inline void civilFrom(long long z, int& y, unsigned& m, unsigned& d) {
    z += 719468; const long long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const long long yy = (long long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1; m = mp < 10 ? mp + 3 : mp - 9; y = (int)(yy + (m <= 2));
}
inline long long civilDays(long long y, unsigned m, unsigned d);   // defined with the session keys below
// "20261015" -> the n session strings before it (calendar days back, the caller keeps those whose file exists)
inline std::vector<std::string> daysBefore(const std::string& s, int n) {
    std::vector<std::string> out;
    if (s.size() != 8) return out;
    for (size_t i = 0; i < 8; ++i) if (s[i] < '0' || s[i] > '9') return out;
    const long long y = std::atoll(s.substr(0, 4).c_str()); const unsigned m = (unsigned)std::atoi(s.substr(4, 2).c_str()), d = (unsigned)std::atoi(s.substr(6, 2).c_str());
    const long long z0 = civilDays(y, m, d);
    for (int k = 1; k <= n; ++k) { int yy; unsigned mm, dd; civilFrom(z0 - k, yy, mm, dd); char b[16]; std::snprintf(b, sizeof(b), "%04d%02u%02u", yy, mm, dd); out.push_back(b); }
    return out;
}
struct MStat { bool ratioOk[2] = {false, false}; double ratio[2] = {0, 0}, mean[2] = {0, 0}; long long priorMax = 0; bool spike[2] = {false, false}; };
inline MStat metricStat(const std::vector<long long>& pb, const std::vector<long long>& pa, const long long v[2], int ratioBars, const SpikeCfg& c, double floor) {
    MStat m;
    for (int s = 0; s < 2; ++s) m.ratio[s] = ratioOf(s == BID ? pb : pa, v[s], ratioBars, m.ratioOk[s], &m.mean[s], floor);
    m.priorMax = priorMaxEither(pb, pa, c.lookback > 0 ? c.lookback : 10);
    for (int s = 0; s < 2; ++s) m.spike[s] = m.ratioOk[s] && isSpike(v[s], m.mean[s], m.priorMax, c);
    return m;
}
inline bool high(const MStat& m, int s) { return m.spike[s] || (m.ratioOk[s] && m.ratio[s] >= HIGH_X); }
inline std::string ratioText(double r) { return nl::ratioText(r); }
inline std::string ratioOr(bool ok, double r) { return ok ? nl::ratioText(r) : std::string("n/a"); }

// =====================================================================================================================
// (1.2.0) PACE (SPEC "PACE = lots per second in the candle vs that market's same-time-of-day norm"): BidAskStats = aggressive lots / s,
// PullStats = pulled lots / s, StackStats = added lots / s, per side. The norm (lots / s per side) is the nightly PACE_NORM row of the
// candle's 30-min CT bucket; without one, the rolling norm = the mean per-side rate of up to 60 earlier closed candles of the last
// 4 h (needs 5). Decided once at the close (stored in the record); notable: >= paceFastX = "fast", <= paceSlowX on a spike = "slowly".
// =====================================================================================================================
static const int PACE_ROLL_MIN = 5, PACE_ROLL_MAX = 60; static const long long PACE_ROLL_SPAN_S = 4 * 3600;
struct Pace { bool ok = false; double x = 0, norm = 0; bool fromFile = false; int note = 0; };
inline double fileNorm(const Params& P, int metric, int minuteOfDay) {
    const int b = bucketOf(minuteOfDay); if (b < 0 || metric < 0 || metric > 2) return -1;
    std::map<int, double>::const_iterator it = P.norm[metric].find(b);
    return it == P.norm[metric].end() ? -1 : it->second;
}
// prior = earlier candles' per-side rates (lots / s, the two sides averaged), newest last
inline double rollingNorm(const std::vector<double>& prior) {
    if (prior.size() < (size_t)PACE_ROLL_MIN) return -1;
    const size_t n = prior.size() < (size_t)PACE_ROLL_MAX ? prior.size() : (size_t)PACE_ROLL_MAX;
    double s = 0; for (size_t i = prior.size() - n; i < prior.size(); ++i) s += prior[i];
    const double m = s / (double)n; return m > 0 ? m : -1;
}
inline Pace paceOf(double rate, double fileN, double rollN, const Params& P, bool spike) {
    Pace p; const double n = fileN > 0 ? fileN : rollN;
    if (!(n > 0) || !finite(rate) || rate < 0) return p;
    p.ok = true; p.norm = n; p.fromFile = fileN > 0; p.x = rate / n;
    if (p.x * 10.0 >= (double)P.paceFastT - 1e-9) p.note = 1;
    else if (spike && p.x * 10.0 <= (double)P.paceSlowT + 1e-9) p.note = 2;
    return p;
}
inline long long pace100(const Pace& p) { if (!p.ok) return 0; const long long v = (long long)std::llround(p.x * 100.0); return v < 1 ? 1 : v > 999900 ? 999900 : v; }
inline std::string paceX(long long p100) { return nl::ratioText((double)p100 / 100.0); }
// the message words: " fast (p 2.6x)" / " slowly (p 0.7x)"; gate ? -> " fast (p 2.6x ?)"; gate off / not notable -> ""
inline std::string paceWords(long long note, long long p100, long long gate) {
    if (p100 <= 0 || gate == ofo::GATE_OFF || (note != 1 && note != 2)) return std::string();
    return std::string(note == 1 ? " fast (p " : " slowly (p ") + paceX(p100) + (gate == ofo::GATE_ON ? ")" : " ?)");
}
// the hover: "2.3x (2.0x)" (+ " ?" until PACE_SHOW,on; " off" when the gate is off)
inline std::string paceHover(long long p100, long long fastT, long long gate) {
    if (p100 <= 0) return "n/a";
    return paceX(p100) + " (" + tenthsText(fastT) + ")" + (gate == ofo::GATE_ON ? "" : gate == ofo::GATE_OFF ? " off" : " ?");
}
inline std::string paceLabel(long long p100) { return p100 > 0 ? "p " + paceX(p100) : std::string(); }
// "42 \xB7 avg 3.6" (orders, the middle dot of the chart's ANSI code page) / "n/a"
inline std::string ordersText(long long n, long long lots) {
    if (n <= 0) return lots > 0 ? std::string("0") : std::string("-");
    const long long t = (lots * 20 + n) / (2 * n);                       // tenths, rounded half up (137 / 20 = 6.85 -> 6.9)
    char b[96]; std::snprintf(b, sizeof(b), "%lld \xB7 avg %lld.%lld", n, t / 10, t % 10); return b;
}
// (1.2.1, Rassul 13 Oct "just do it") the ONE "Crowd" row: orders, average size and the top order's share - "96 \xB7 avg 3.8 \xB7 top 12%"
inline std::string crowdText(long long n, long long lots, long long top) {
    std::string o = ordersText(n, lots);
    if (n <= 0) return o;
    if (lots > 0 && top > 0) { long long p = (top * 100 + lots / 2) / lots; if (p > 100) p = 100; o += " \xB7 top " + std::to_string(p) + "%"; }
    return o;
}
static const char* const CROWD_TMPL = "999 \xB7 avg 99.9 \xB7 top 99%";   // the info box value width (the longest row value)
inline std::string pctOf(long long part, long long whole) {
    if (whole <= 0 || part <= 0) return "-";
    long long p = (part * 100 + whole / 2) / whole; if (p > 999) p = 999; return std::to_string(p) + "%";
}
// rebuilt %: the share of the hit size restored after the hits (lots re-added at a hit price, at most that price's hit lots / the lots
// hit), shown only when there were hits (-1 = none), at most 100% (1.2.1; was capped at 200% and counted every queue join)
inline long long rebuiltPctOf(long long rebuilt, long long hit) {
    if (hit <= 0) return -1;
    if (rebuilt < 0) rebuilt = 0;
    long long p = (rebuilt * 100 + hit / 2) / hit; return p > 100 ? 100 : p;
}

// =====================================================================================================================
// THE MBO PANES' SERIES (PullStats / StackStats): one bar = a Rec + its display, decided once at the close
// =====================================================================================================================
struct PaneBar {
    bool ok = false, fromFile = false; int slotN = 0;    // (1.8.0) slotN = same-slot bars the ratio / spike used (0 = the 20-bar rule)
    Rec r; MStat m[3];                                   // m: stats as of the bar (metric 0 = the bars, 1 = refills; BookFlow 1.0.0: 0 added, 1 pulled, 2 refills)
    std::string let[2];                                  // letters on [left = BID side, right = ASK side], max 2 each
    bool spike[2] = {false, false}; long long dash[2] = {-1, -1};
    nl::Msg msg; std::string level;
    int msgSide = -1, kind = 0;                          // (1.2.0) the side the message is about (-1 none); PullStats kind 1 = a retreat, 2 = bait gone
};
// (1.2.0) RESET (PullStats, SPEC "a LATER message, posted only when known - never back-dated"): after a notable pull of candle K on one
// side (P lots), that side adding back resetPct (80%) of P near price within resetWindowSec (60 s) of K's end ->
// "11:06 Bids back within 40 s of the 11:05 pull - reset, not a retreat at PDL." on the candle in which it completed, at its close.
struct ResetWatch { bool on = false; int side = 0; long long pullEnd = 0, target = 0, cum = 0, resets = 0; std::string pullCt, level; };
inline void resetStart(ResetWatch& w, int side, long long pullEnd, long long pulledLots, const Params& P, const std::string& ct, const std::string& level) {
    w.on = pulledLots > 0; w.side = side; w.pullEnd = pullEnd; w.cum = 0; w.pullCt = ct; w.level = level;
    w.target = (pulledLots * P.resetPct + 99) / 100; if (w.target < 1) w.target = 1;
}
// one later candle, in order: its add samples on the watched side ((stamp s, cumulative lots in this candle), oldest first) and its end.
// Returns the seconds from the pull candle's end to the moment the target was reached (>= 1) when it completed in this candle, else -1.
// The watch closes on a reset or once the window has passed. A sample without a stamp counts at the candle's end.
inline long long resetStep(ResetWatch& w, const std::vector<std::pair<long long, long long> >& samples, long long candleEnd, const Params& P) {
    if (!w.on || candleEnd <= w.pullEnd) return -1;
    const long long deadline = w.pullEnd + P.resetWindowSec;
    long long prev = 0;
    for (size_t i = 0; i < samples.size(); ++i) {
        long long st = samples[i].first; const long long inc = samples[i].second - prev; prev = samples[i].second;
        if (st <= 0 || st > candleEnd) st = candleEnd;
        if (st < w.pullEnd) st = w.pullEnd;
        if (st > deadline) break;
        w.cum += inc > 0 ? inc : 0;
        if (w.cum >= w.target) { w.on = false; w.resets++; const long long sec = st - w.pullEnd; return sec < 1 ? 1 : sec; }
    }
    if (candleEnd >= deadline) w.on = false;
    return -1;
}
inline std::string resetMessage(int side, long long sec, const std::string& pullCt, const std::string& level) {
    return std::string(side == BID ? "Bids" : "Offers") + " back within " + std::to_string(sec) + " s of the " + pullCt + " pull - reset, not a retreat" + (level.empty() ? std::string() : " at " + level) + ".";
}
template <class L> struct PaneSeries {
    int ratioBars = RATIO_DEFAULT; SpikeCfg spike;
    std::map<long long, PaneBar> bars;
    std::map<long long, std::vector<long long>> hist;    // (1.8.0) earlier sessions' bars: metric k side s at [k * 2 + s] (slot yardstick only)
    // (1.8.0) the same-slot values of metric k before t: earlier sessions + this one, newest first, at most SLOT_MAX_N per side
    void slotVals(long long t, int k, std::vector<long long>& vb, std::vector<long long>& va) const {
        vb.clear(); va.clear(); const int sl = slotOfKey(t);
        typename std::map<long long, PaneBar>::const_iterator it = bars.lower_bound(t);
        while (it != bars.begin() && (int)vb.size() < SLOT_MAX_N) {
            --it; if (!it->second.ok || it->second.r.partial || slotOfKey(it->first) != sl) continue;
            vb.push_back(L::value(it->second.r, k, BID)); va.push_back(L::value(it->second.r, k, ASK));
        }
        std::map<long long, std::vector<long long>>::const_iterator h = hist.lower_bound(t);
        while (h != hist.begin() && (int)vb.size() < SLOT_MAX_N) {
            --h; if (slotOfKey(h->first) != sl || h->second.size() < (size_t)(k * 2 + 2)) continue;
            vb.push_back(h->second[(size_t)(k * 2)]); va.push_back(h->second[(size_t)(k * 2 + 1)]);
        }
    }
    long long computed = 0;
    // the prior values of one metric before t: valid, not partial, newest last (at most `need`)
    void prior(long long t, size_t need, int metric, std::vector<long long>& pb, std::vector<long long>& pa) const {
        pb.clear(); pa.clear();
        typename std::map<long long, PaneBar>::const_iterator it = bars.lower_bound(t);
        while (it != bars.begin() && pb.size() < need) {
            --it;
            if (it->second.ok && !it->second.r.partial) { pb.push_back(L::value(it->second.r, metric, BID)); pa.push_back(L::value(it->second.r, metric, ASK)); }
        }
        for (size_t i = 0, j = pb.size(); i < j / 2; ++i) { std::swap(pb[i], pb[j - 1 - i]); std::swap(pa[i], pa[j - 1 - i]); }
    }
    void stats(long long t, PaneBar& b) const {
        const size_t need = (size_t)(ratioBars > SPIKE_LOOKBACK_MAX ? ratioBars : SPIKE_LOOKBACK_MAX);
        std::vector<long long> pb, pa;
        for (int k = 0; k < L::metrics(); ++k) {
            prior(t, need, k, pb, pa);
            const long long v[2] = { L::value(b.r, k, BID), L::value(b.r, k, ASK) };
            b.m[k] = metricStat(pb, pa, v, ratioBars, spike, 1.0);
            std::vector<long long> vb, va; slotVals(t, k, vb, va);          // (1.8.0) the same 30-min slot when it has 10+ bars
            const SlotLog zb = slotLog(vb), za = slotLog(va);
            applySlot(v[0], zb, b.m[k].ratioOk[0], b.m[k].ratio[0], b.m[k].mean[0], b.m[k].spike[0]);
            applySlot(v[1], za, b.m[k].ratioOk[1], b.m[k].ratio[1], b.m[k].mean[1], b.m[k].spike[1]);
            if (!spike.on()) { b.m[k].spike[0] = b.m[k].spike[1] = false; }   // the dialog's spikes-off still wins
            if (k == 0) b.slotN = (zb.ok || za.ok) ? std::max(zb.n, za.n) : 0;
        }
    }
    PaneBar make(long long t, const Rec& r) const { PaneBar b; b.r = r; b.ok = r.events > 0; if (b.ok) stats(t, b); return b; }
    // a settings change (dialog) only: every recorded bar re-decided, oldest first, from its stored numbers (letters / level kept)
    template <class Decide> void recompute(Decide decide) {
        for (typename std::map<long long, PaneBar>::iterator it = bars.begin(); it != bars.end(); ++it) {
            if (!it->second.ok) continue;
            stats(it->first, it->second); decide(it->second);
        }
    }
    void trim(size_t keep) { while (bars.size() > keep) bars.erase(bars.begin()); }
    // (1.2.0) the rolling pace norm's inputs: earlier closed candles' per-side rates (lots / s, both sides averaged) within span s, newest last
    std::vector<double> priorRates(long long t, long long span, int spb) const {
        std::vector<double> out; if (spb <= 0) return out;
        typename std::map<long long, PaneBar>::const_iterator it = bars.lower_bound(t);
        while (it != bars.begin() && out.size() < (size_t)PACE_ROLL_MAX) {
            --it;
            if (t - it->first > span) break;
            if (it->second.ok && !it->second.r.partial) out.push_back(((double)L::value(it->second.r, 0, BID) + (double)L::value(it->second.r, 0, ASK)) / 2.0 / (double)spb);
        }
        std::reverse(out.begin(), out.end());
        return out;
    }
};

// (1.2.0) a closed pane candle's pace, decided once at its close (stored in its record numbers): lots / s per side vs the norm
template <class L> void panePace(PaneBar& b, const PaneSeries<L>& ser, long long t, int spb, int minuteOfDay, const Params& P) {
    b.r.paceGate = P.gate[ofo::G_PACE];
    if (!b.ok || spb <= 0) return;
    const double rn = rollingNorm(ser.priorRates(t, PACE_ROLL_SPAN_S, spb)), fn = fileNorm(P, L::paceMetric(), minuteOfDay);
    for (int s = 0; s < 2; ++s) {
        const Pace p = paceOf((double)L::value(b.r, 0, s) / (double)spb, fn, rn, P, b.m[0].spike[s]);
        b.r.pace100[s] = pace100(p); b.r.paceNote[s] = p.note;
    }
}

// =====================================================================================================================
// LAYOUT (pixels): 9-spot placement, the two bars of a candle, bar height, the value axis, letters, hover, clocks
// =====================================================================================================================
static const char* const POS_CHOICES = "Top left;Top centre;Top right;Middle left;Middle centre;Middle right;Bottom left;Bottom centre;Bottom right";
static const int POS_DEFAULT = 0;                  // the INFO BOX: Top left (12:05, as lsBidAskStats 1.2.0); the message line is always top centre
static const int TEXT_RESERVE = 32;                // px kept free at the top for the message line + (1.2.0) the blue options line
struct Box { int l, t, r, b; };
inline Box placeIn(const Box& area, int W, int H, int pos) {
    if (pos < 0 || pos > 8) pos = POS_DEFAULT;
    const int c = pos % 3, r = pos / 3;
    int x = c == 0 ? area.l : c == 1 ? (area.l + area.r) / 2 - W / 2 : area.r - W;
    int y = r == 0 ? area.t : r == 1 ? (area.t + area.b) / 2 - H / 2 : area.b - H;
    if (x > area.r - W) x = area.r - W;
    if (x < area.l) x = area.l;
    if (y > area.b - H) y = area.b - H;
    if (y < area.t) y = area.t;
    return Box{ x, y, x + W, y + H };
}
// the two bars inside one candle slot: [0] left, [1] right (never wider than the slot)
struct Pair { int l[2], r[2]; };
inline Pair pairIn(int xc, int slot) {
    if (slot < 2) slot = 2;
    int inner = (slot * 8) / 10; if (inner < 2) inner = 2;
    const int gap = inner >= 6 ? 1 : 0;
    const int half = (inner - gap) / 2 > 0 ? (inner - gap) / 2 : 1;
    Pair p; p.r[0] = xc; p.l[0] = xc - half; p.l[1] = xc + gap; p.r[1] = p.l[1] + half;
    return p;
}
inline int barHeight(long long v, long long top, int px) {
    if (v <= 0 || top <= 0 || px <= 0) return 0;
    const double h = (double)px * (double)v / (double)top;
    const int r = (int)(h + .5);
    return r < 1 ? 1 : (r > px ? px : r);
}
// (Effort Reward 1.23 / Defense 1.14, Rassul 2026-10-20 08:19 + 14:22 "if i move it dont the bars move also ?") THE Y-AXIS DRIVES THE BARS:
// the bars are placed with IRT's own value -> pixel mapping of the pane (the same mapping the axis labels use), so dragging / stretching
// the axis zooms the bars with it. ValY = that mapping read once per draw: y0 = the pixel row of value 0, ppu = pixels per unit (> 0).
// Not usable (no mapping, or value 0 far outside the pane) -> the old fixed-pixel scale. SDK-free so it is unit-tested.
struct ValY { bool ok = false; double y0 = 0, ppu = 0; int yOf(double v) const { return (int)std::lround(y0 - v * ppu); } };
inline ValY valYOf(double yZero, double yTop, long long top, int paneTop, int paneBottom) {
    ValY m; if (top <= 0) return m;
    const double ppu = (yZero - yTop) / (double)top, H = (double)(paneBottom - paneTop);
    if (!(ppu > 0) || !(ppu < 1e7) || !(yZero == yZero)) return m;
    if (yZero < paneTop - H || yZero > paneBottom + H) return m;               // value 0 far off the pane: not this pane's mapping
    m.ok = true; m.y0 = yZero; m.ppu = ppu; return m;
}
// the pixel rows of a bar of value v from zero: [yTopOut, yBotOut] inside the pane's drawing rows; returns its height (0 = not visible);
// capped = it was clipped at the top of the drawing area (the white tip)
// lo / hi = the first / last drawing row (pane top + reserve + 1 / pane bottom - 1)
inline int axisBar(const ValY& m, int lo, int hi, long long v, int& yTopOut, int& yBotOut, bool& capped) {
    int yb = m.yOf(0), yt = m.yOf((double)v); capped = false;
    if (yb > hi) yb = hi;
    if (yt < lo) { yt = lo; capped = true; }
    if (yt > yb || yb < lo) return 0;
    if (yt == yb && v > 0) yt = yb;                                       // at least 1 px
    yTopOut = yt; yBotOut = yb; return yb - yt + 1;
}
// (12:03 / 12:10) HEADROOM for the spike labels and the message line: the tallest bar tops out at <= 1/1.45 (~69%) of the drawing
// height AND leaves LABEL_ROOM px (two label lines) plus the text reserve above it. The axis (scale()) uses the same px, so every
// bar top still sits at its value on the axis.
static const int LABEL_PT = 10, LABEL_LH = 14, LABEL_ROWS = 3, LABEL_ROOM = LABEL_ROWS * LABEL_LH + 2;   // (1.2.0) 3 rows: ratio / pace / lots; (1.2.1) 10 pt (was 7)
static const int INFO_PT = 10, INFO_LH = 15;                                                            // (1.2.1) the info box at 10 pt (was 9 / 13)
inline int drawPx(int paneH, int reserve) {
    const int avail = paneH - 6; if (avail <= 1) return 1;
    int p = (int)((double)avail / 1.45);
    const int q = avail - reserve - LABEL_ROOM; if (q < p) p = q;
    if (p < 8) p = avail > 8 ? 8 : avail;
    return p;
}
// the axis range IRT draws over the whole pane so each value sits at its bar's top pixel (value 0 at bottom - 1)
inline void axisFor(long long top, int paneH, int px, double& mn, double& mx) {
    if (top <= 0) { mn = 0; mx = 10; return; }
    if (paneH < 24 || px <= 0) { mn = 0; mx = (double)top * 1.02; return; }
    const double unit = (double)top / (double)px;
    mn = -unit; mx = (double)(paneH - 1) * unit;
}
// ---- (Aggression 1.18.0 / Book Flow 1.8.0, Rassul 2026-10-19 19:02 "can book flow and bid ask stats show both buyers and sellers side by
// side and on the top side instead of negative values", 19:04 "make sure there is a read .. have a question on top of the pane", 19:09
// "who is aggressive - and is it moving price" / "who is defending - and are they refilling", "build") PAIR BARS: two bars per candle,
// both from the bottom up, green (buyers) LEFT, red (sellers) RIGHT in both panes; the question line on top of each pane.
static const int QUESTION_H = 16;                                          // the question line's rows at the top of the pane
inline int pairPx(int paneH, int reserve) { const int p = paneH - reserve - (2 * LABEL_LH + 4) - 3; return p < 4 ? 4 : p; }   // the tallest bar: room for 2 label rows above it
// (1.7.0, BookFlow) net bars geometry: SDK-free so it is unit-tested
struct NetGeo { int yTop = 0, yBot = 0, y0 = 0, maxPx = 1, downPx = 1; long long upTop = 0, downTop = 0; };   // rows from the pane's top: usable rows, the zero line, the tallest up / down bar; upTop / downTop = the value drawn maxPx / downPx tall
// (BookFlow 1.5.1, Rassul 2026-10-19 13:37 "there is a lot of black space .. get rid of the extra black space") up / down = the
// biggest + and - net on screen (-1 = not given: the old even split). The zero line sits where the data needs it: no negative net
// on screen -> the zero line drops to the bottom (only the pull-marker strip below it); some -> the height is split by size, one
// scale for both directions (a 1,000 bar is as tall up as down).
inline NetGeo netGeoFor(int paneH, int reserve, long long up = -1, long long down = -1) {
    NetGeo n; n.yTop = reserve + 3; n.yBot = paneH - 3;
    if (n.yBot - n.yTop < 12) { n.yTop = 2; n.yBot = paneH - 2; if (n.yBot <= n.yTop) n.yBot = n.yTop + 1; }
    if (up < 0 || down < 0) {
        n.y0 = (n.yTop + n.yBot) / 2;
        const int half = (n.yBot - n.yTop) / 2;
        int mp = half - LABEL_ROOM; if (mp < 10) mp = half - 4; if (mp < 4) mp = half > 1 ? half - 1 : 1;
        n.maxPx = mp; n.downPx = mp; n.upTop = n.downTop = 0; return n;
    }
    const int H = n.yBot - n.yTop;
    const int strip = down > 0 ? LABEL_ROOM : LABEL_LH + 12;               // under the bars: room for a down bar's labels, else only the pull-marker strip
    int room = LABEL_ROOM; if (H - room - strip < 12) room = std::max(4, H - strip - 12);
    const int bars = std::max(4, H - room - strip - 2);                    // pixels shared by the up and down bars
    int dn = 0;
    if (down > 0) {
        const double f = (double)down / (double)(std::max<long long>(up, 0) + down);
        dn = (int)(bars * f + .5); if (dn < 6) dn = std::min(6, bars / 2);
        if (bars - dn < 6) dn = bars - 6;                                  // the up side always keeps a little room
    }
    const int upPx = std::max(1, bars - dn);
    n.y0 = n.yTop + room + upPx; n.maxPx = upPx; n.downPx = std::max(1, dn);
    // one scale: the unit that fits both sides
    const double u = std::max(up > 0 ? (double)up / upPx : 0.0, down > 0 && dn > 0 ? (double)down / dn : 0.0);
    n.upTop = u > 0 ? (long long)std::llround(u * upPx) : std::max<long long>(up, 1);
    n.downTop = u > 0 ? (long long)std::llround(u * n.downPx) : std::max<long long>(down, 1);
    n.upTop = std::max<long long>(n.upTop, 1); n.downTop = std::max<long long>(n.downTop, 1);
    return n;
}
// (Aggression 1.17.0 / Book Flow 1.7.0, Rassul 2026-10-19 18:26 "there needs to be a different scale .. it is difficult to see and gets
// distorted", mockup aggr-cap-mockup.png approved 18:28) the SCALE CAP: the bars on screen scale to 1.5x their 90th-percentile size (fewer
// than 10 bars: no cap); a taller bar stops at the edge with a white tip (its label and the box keep the true size). Recomputed from what
// is on screen (zoom / scroll), the same in scale() (the axis) and the drawing.
static const double CAP_X = 1.5, CAP_PCT = 0.90; static const int CAP_MIN_BARS = 10;
inline long long capTop(std::vector<long long> v) {
    v.erase(std::remove_if(v.begin(), v.end(), [](long long a) { return a <= 0; }), v.end());
    if ((int)v.size() < CAP_MIN_BARS) return LLONG_MAX;
    const size_t k = (size_t)((double)(v.size() - 1) * CAP_PCT);
    std::nth_element(v.begin(), v.begin() + (long)k, v.end());
    const long long c = (long long)std::llround(CAP_X * (double)v[k]);
    return c < 1 ? 1 : c;
}
inline long long capped(long long v, long long cap) { return v > cap ? cap : v; }
static const int CAP_TIP_PX = 2;
// ---- (1.17.0 / BookFlow 1.7.0, Rassul 2026-10-19 18:47 "yes" - the order feed went silent 3:29 PM -> 6:24 PM with no reconnect) FEED WATCHDOG:
// silent = the feed is open and subscribed, no order event for SILENT_MS while the chart kept trading (its volume changed after the last
// event + 5 s). Then the feed is closed and reopened (at most once a minute) and the pane says so; every dropout / reconnect is counted.
static const long long FEED_SILENT_MS = 30000, FEED_RECONNECT_GAP_MS = 60000, FEED_TRADE_LAG_MS = 5000;
inline bool feedSilent(bool open, int flag, long long lastEventMs, long long lastTradeMs, long long now) {
    return open && flag == 1 && lastEventMs >= 0 && now - lastEventMs >= FEED_SILENT_MS && lastTradeMs >= 0 && lastTradeMs > lastEventMs + FEED_TRADE_LAG_MS;
}
inline bool reconnectDue(long long lastReconnectMs, long long now) { return lastReconnectMs < 0 || now - lastReconnectMs >= FEED_RECONNECT_GAP_MS; }
// the pane's read while the feed is silent: "Order feed silent since 3:29 PM - reconnecting (2)"
inline std::string silentRead(long long fromWall, long long reconnects) {
    std::string at; const std::time_t t = (std::time_t)fromWall; struct tm x; std::memset(&x, 0, sizeof(x));
#if defined(_WIN32)
    if (localtime_s(&x, &t) == 0) at = " since " + zread::clock12(x.tm_hour * 60 + x.tm_min);
#else
    if (localtime_r(&t, &x)) at = " since " + zread::clock12(x.tm_hour * 60 + x.tm_min);
#endif
    return "Order feed silent" + at + " - reconnecting" + (reconnects > 0 ? " (" + std::to_string(reconnects) + ")" : std::string()) + "; book numbers are stale";
}
// the axis range IRT draws over the pane so a value sits at its bar's pixel row (0 at the zero line, + above, - below)
inline void netAxisFor(long long top, int paneH, int reserve, double& mn, double& mx, long long up = -1, long long down = -1) {
    if (top <= 0 || paneH < 24) { mn = -10; mx = 10; return; }
    const NetGeo n = netGeoFor(paneH, reserve, up, down);
    const double unit = up < 0 ? (double)top / (double)n.maxPx : (double)n.upTop / (double)n.maxPx;
    mx = (double)n.y0 * unit; mn = (double)(n.y0 - (paneH - 1)) * unit;
}
inline int textReserve(int paneH) { return paneH >= 80 ? TEXT_RESERVE : 0; }
// LETTERS: white boxes (black bold letter) at the BOTTOM of a side's bar, priority letter on top (mockup), at most 2 per side.
// Never overlapping and never outside the candle's slot: if the two sides' columns would touch, one column in the slot's centre
// (left side's letters at the bottom, then the right side's); slot narrower than a box = no letters (zoomed out).
struct LBox { int l, t, r, b; char c; int side; };
inline std::vector<LBox> letterBoxes(int slotL, int slotR, int cx0, int cx1, int yBottom, int yTop, const std::string& let0, const std::string& let1, int bw, int bh) {
    std::vector<LBox> out;
    const std::string L[2] = { let0.substr(0, 2), let1.substr(0, 2) };
    if (bw <= 0 || bh <= 0 || slotR - slotL + 1 < bw || (L[0].empty() && L[1].empty())) return out;
    auto colX = [&](int cx) { int x = cx - bw / 2; if (x + bw - 1 > slotR) x = slotR - bw + 1; if (x < slotL) x = slotL; return x; };
    int x[2] = { colX(cx0), colX(cx1) };
    const bool single = !L[0].empty() && !L[1].empty() && x[1] <= x[0] + bw;   // the columns would touch
    if (single) x[0] = x[1] = colX((slotL + slotR + 1) / 2);
    int y = yBottom;
    for (int s = 0; s < 2; ++s) {
        if (!single) y = yBottom;
        for (int k = (int)L[s].size() - 1; k >= 0; --k) {               // bottom-up: the last (lower priority) letter lowest
            const int t = y - bh + 1;
            if (t < yTop) break;
            out.push_back(LBox{ x[s], t, x[s] + bw - 1, y, L[s][(size_t)k], s });
            y = t - 2;
        }
    }
    return out;
}
inline bool overlaps(const LBox& a, const LBox& b) { return !(a.r < b.l || b.r < a.l || a.b < b.t || b.b < a.t); }
// SPIKE LABELS (Rassul 12:03 "i want the quantity and a ratio on top of the spikes"): above every spike bar, centred on it, in the
// spike colour - the ratio on top, the pace, the quantity just above the bar top ("4.1x" / "p 2.3x" / "240"). Always ABOVE the bar
// (12:10: no inside-the-bar fallback); never over another label, a letter, the read box or the message line (yTop).
// 1.3.1 (2026-10-14 CR5300, Rassul 11:59 CT CLZ6 3-min: "the label for pace and lots on the red spike goes away but it is there on the
// green spike. fix it"): a label once given NEVER loses rows because of another spike. 1.2.1-1.3.0 had a pass 2 that, for a spike
// with no free spot (e.g. under the read box, or the forming bar), SHRANK the nearest full labels to their ratio one by one - every
// label on the pane if the spot never freed up - so a seller (or buyer) spike lost its pace and lots whenever some other spike could
// not be placed (new bars, the forming bar flickering, scrolling, the hover grid). Now each label is decided in TIME ORDER from the
// older labels and the fixed obstacles only: 1) the full label right above its bar, staggered up; 2) else the ratio alone (the lots
// when no ratio) above its bar, staggered up, then shifted left / right. A later spike can never change an earlier label.
struct SpikeIn { int cx = 0, barTop = 0, side = 0; std::string q, x, p; bool view = true; };   // (1.2.0) p = pace "p 2.3x" ("" = none); in time order; (1.3.1) view = its candle is in view
struct SLabel { int l, t, r, b, side; std::string q, x, p; bool view = true; };
template <class W> std::vector<SLabel> spikeLabels(const std::vector<SpikeIn>& in, int yTop, const std::vector<LBox>& letters, W width, int xMin = -1000000, int xMax = 1000000) {
    std::vector<SLabel> out;                                              // the labels placed so far (older first)
    // one free spot for a label of w x h near its bar: right above it, then staggered up, then (shift) moved left / right
    auto spot = [&](const SpikeIn& sp, int w, int h, int maxStep, LBox& got, bool shift) {
        const int shifts[3] = { 0, -(w / 2 + 3), w / 2 + 3 };
        for (int sh = 0; sh < (shift ? 3 : 1); ++sh)
            for (int step = 0; step < maxStep; ++step) {
                const int b = sp.barTop - 2 - step * (h + 2), t = b - h + 1;
                if (t < yTop) break;
                int l = sp.cx - w / 2 + shifts[sh];
                if (l + w - 1 > xMax) l = xMax - w + 1;
                if (l < xMin) l = xMin;
                const LBox me = { l, t, l + w - 1, b, 0, 0 };
                bool hit = false;
                for (size_t k = 0; k < out.size() && !hit; ++k) { const LBox o = { out[k].l, out[k].t, out[k].r, out[k].b, 0, 0 }; hit = overlaps(me, o); }
                for (size_t k = 0; k < letters.size() && !hit; ++k) hit = overlaps(me, letters[k]);
                if (!hit) { got = me; return true; }
            }
        return false;
    };
    for (size_t i = 0; i < in.size(); ++i) {
        const SpikeIn& sp = in[i];
        const int rows = sp.p.empty() ? 2 : 3;
        const int w = std::max(std::max(width(sp.q), width(sp.x)), width(sp.p)) + 2; LBox got;
        if (spot(sp, w, rows * LABEL_LH, 4, got, false)) {               // (a full label is never shifted: it could sit over another candle)
            out.push_back(SLabel{ got.l, got.t, got.r, got.b, sp.side, sp.q, sp.x, sp.p, sp.view }); continue;
        }
        const std::string x = sp.x.empty() ? sp.q : sp.x;                  // no room for the full label: the ratio alone (crowded)
        if (spot(sp, width(x) + 2, LABEL_LH, 8, got, true)) out.push_back(SLabel{ got.l, got.t, got.r, got.b, sp.side, std::string(), x, std::string(), sp.view });
    }
    return out;
}
// (1.3.1) how many candles LEFT of the view take part in the label layout (not drawn), so scrolling does not change a label that
// stays in view: labels only interact within a few label widths
inline int labelLead(int slot) { const int s = slot > 0 ? slot : 1; const int n = 240 / s + 2; return n < 4 ? 4 : (n > 60 ? 60 : n); }
inline int hoverIndex(long bar, long count) { return bar >= 0 && bar < count - 1 ? (int)bar : -1; }
struct Hover { int bar = -1; bool set(int b) { if (b < -1) b = -1; if (b == bar) return false; bar = b; return true; } };
inline std::string clock12(int hour, int minute) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return "n/a";
    char b[16]; std::snprintf(b, sizeof(b), "%d:%02d %s", hour % 12 == 0 ? 12 : hour % 12, minute, hour < 12 ? "AM" : "PM"); return b;
}
using nl::clockShort; using nl::fitText; using nl::Msg; using nl::NEUTRAL; using nl::BULL; using nl::BEAR;
// (of140 6:18 PM render check) a message too wide for the pane loses its least important words FIRST, never its verdict (the tail
// "- price held: buyers absorbing" / "- support leaving at PDL"): 1) the large-order clause (", plus one 120-lot order", " with one
// 120-lot order", ", one 120-lot order" - the hover's Large row has it), 2) the pace words (" fast (p 2.3x)" - the hover's Pace row),
// 3) "levels" -> "lvls", " totaling " -> ", ", only then the end is cut (fitText). Returns the variants in that order (the first = as is).
inline void eraseFrom(std::string& s, const std::string& start, const std::string& end) {   // erase start..end (end kept out) once
    const size_t a = s.find(start); if (a == std::string::npos) return;
    const size_t b = s.find(end, a + start.size()); if (b == std::string::npos) return;
    s.erase(a, b - a);
}
inline std::vector<std::string> compactVariants(const std::string& l1) {
    std::vector<std::string> v(1, l1); std::string t = l1;
    for (const char* k : { ", plus one ", " with one ", ", one " }) {
        const size_t a = t.find(k); if (a == std::string::npos) continue;
        const size_t b = t.find("-lot order", a); if (b == std::string::npos || t.find_first_of(",-", a + std::strlen(k)) < b) continue;
        t.erase(a, b + 10 - a); break;
    }
    if (t != v.back()) v.push_back(t);
    for (const char* k : { " fast (p ", " slowly (p " }) { const size_t a = t.find(k); if (a == std::string::npos) continue; const size_t b = t.find(')', a); if (b != std::string::npos) t.erase(a, b + 1 - a); }
    if (t != v.back()) v.push_back(t);
    for (;;) { const size_t a = t.find(" levels"); if (a == std::string::npos) break; t.replace(a, 7, " lvls"); }
    { const size_t a = t.find(" totaling "); if (a != std::string::npos) t.replace(a, 10, ", "); }
    if (t != v.back()) v.push_back(t);
    return v;
}
inline std::string fitMessage(const std::string& l1, int width, const std::function<int(const std::string&)>& w) {
    const std::vector<std::string> v = compactVariants(l1);
    for (const std::string& t : v) if (w(t) <= width) return t;
    return fitText(v.back(), width, w);
}
// the first top line: "10:24 <message>"
inline std::string line1Of(const std::string& ct, const std::string& msg) { return ct.empty() ? msg : ct + " " + msg; }   // (1.2.0) SPEC: one space
// (12:05) the info box's content (drawn by drawInfo): label | left value | right value; side -1 = white
struct InfoRow { std::string label, v[2]; int side[2] = {0, 1}; };          // side -1 = white
struct Info { std::string head; bool twoCol = true; std::string colHead[2]; std::vector<InfoRow> rows; std::string tmpl = "9999 (99.9x)";
              int topLines = 0, topW = 0; };   // (2026-10-21, lsBookFlow 1.15.0) lines kept free at the box's top for the question + info icon, and their width

// =====================================================================================================================
// (1.3.0, CR4800) THE PANEL - pure geometry, measured with the host's own font (TextW), so the plugin draws exactly what the tests check.
//   Read mode (default; Rassul 07:22 "replace the info box with a read box because there isn't enough room for both"): a READ box of
//   FIXED size (READ_W x the pane height, at most 3 read lines + 2 options lines) placed by Position + Offset X / Y; the read wraps
//   inside it (a 4th line would end in "..."). Hover over it = the numbers grid (every row, same order / colours) at the same top-left.
//   Numbers mode: the compact numbers box (each column as wide as its widest row template among the rows that FIT) + the read beside
//   it, wrapped to 2 lines (pane >= 92 px) instead of cut, the blue options line under it.
// Every size here depends on the pane, the settings and the row TEMPLATES only - never on the live values or the mouse (no shaking).
// =====================================================================================================================
static const int MODE_READ = 0, MODE_NUMBERS = 1, MODE_NONE = 2;   // (1.9.0) MODE_NONE: no text lines at all (a pane's textOff())
static const char* const MODE_CHOICES = "Read;Numbers";
static const int READ_W = 360, BOX_PAD = 5, READ_PT = 10, OPT_PT = 9, READ_LH = 15, OPT_LH = 13, READ_MAX = 3, OPT_MAX = 2, NUM_PT = 10, NUM_LH = 15;
typedef std::function<int(const std::string&, int, bool)> TextW;   // px width of a string at pt / bold
inline int clampMode(long long m) { return m == MODE_NUMBERS ? MODE_NUMBERS : MODE_READ; }
// word wrap into at most maxLines lines of <= w px; more text = the last line cut with "..." (fitText); a single over-long word is cut
inline std::vector<std::string> wrapText(const std::string& s, int w, int maxLines, int pt, bool bold, const TextW& M) {
    std::vector<std::string> out; if (s.empty() || maxLines <= 0 || w <= 0) return out;
    auto W = [&](const std::string& t) { return M(t, pt, bold); };
    std::vector<std::string> words; { size_t a = 0; while (a < s.size()) { size_t b = s.find(' ', a); if (b == std::string::npos) b = s.size(); if (b > a) words.push_back(s.substr(a, b - a)); a = b + 1; } }
    std::string cur; size_t i = 0;
    for (; i < words.size(); ++i) {
        const std::string t = cur.empty() ? words[i] : cur + " " + words[i];
        if (W(t) <= w) { cur = t; continue; }
        if (cur.empty()) { cur = nl::fitText(words[i], w, W); ++i; }      // one word wider than the box
        out.push_back(cur); cur.clear();
        if ((int)out.size() == maxLines) break;
        --i;                                                                // this word starts the next line
    }
    if ((int)out.size() < maxLines) { if (!cur.empty()) out.push_back(cur); return out; }
    if (i < words.size()) {                                                 // text left over: the last line ends in "..."
        std::string rest = out.back(); for (size_t k = i; k < words.size(); ++k) rest += " " + words[k];
        out.back() = nl::fitText(rest, w, W);
    }
    return out;
}
// (1.3.0) the read wrapped into maxLines: as is if it fits, else fitMessage's shorter forms (the large-order clause, then the pace words
// dropped - the hover has them), and only then the end cut - the verdict ("- price held: buyers absorbing") is kept as long as possible
inline std::vector<std::string> wrapRead(const std::string& s, int w, int maxLines, const TextW& M) {
    const std::vector<std::string> v = compactVariants(s);
    for (size_t k = 0; k < v.size(); ++k) {
        const std::vector<std::string> L = wrapText(v[k], w, maxLines, READ_PT, true, M);
        std::string j; for (size_t i = 0; i < L.size(); ++i) j += (i ? " " : "") + L[i];
        if (j == v[k]) return L;
    }
    return wrapText(v.back(), w, maxLines, READ_PT, true, M);
}
// the widest value a row can show (the column width comes from these, never from the live value - no shaking)
inline std::string rowTmpl(const std::string& label, const std::string& def) {
    static const char* const T[][2] = {
        { "Lots", "9999 (99.9x)" }, { "Added", "9999 (99.9x)" }, { "Pulled", "9999 (99.9x)" }, { "Pace", "9.9x (2.0x) ?" },
        { "Sweep", "99 lvls" }, { "Large", "9999" }, { "StopRun", "above LonLO 12t?" }, { "Burst", "99.9x, in 9 bars" },
        { "Rebuilt", "100% ?" }, { "Refills (I)", "999 (99.9x)" }, { "Before price", "yes" }, { "Traded first", "yes" }, { "Counter", "Absorbed only" } };   // (2026-10-21) Counter = BookFlow 1.15.0
    for (size_t k = 0; k < sizeof(T) / sizeof(T[0]); ++k) if (label == T[k][0]) return T[k][1];
    return def;                                                             // Crowd and anything new: the box's template (the widest)
}
// the compact numbers box for the rows that fit in maxH: label column + two value columns (Rassul 07:02 "make them more compact")
struct NumGeom { int lw = 0, vw = 0, W = 0, H = 0, rows = 0; };
inline NumGeom numGeom(const Info& I, int maxH, const TextW& M) {
    NumGeom g; const int head = 1 + (I.twoCol ? 1 : 0) + I.topLines;
    int fit = (maxH - BOX_PAD + 2) / NUM_LH - head; if (fit < 0) fit = 0;   // a row fits when its text ends inside the box (as 1.2.1 drew them)
    g.rows = std::min((int)I.rows.size(), fit);
    for (int k = 0; k < g.rows; ++k) { g.lw = std::max(g.lw, M(I.rows[(size_t)k].label, NUM_PT, true)); g.vw = std::max(g.vw, M(rowTmpl(I.rows[(size_t)k].label, I.tmpl), NUM_PT, true)); }
    if (I.twoCol) for (int s = 0; s < 2; ++s) g.vw = std::max(g.vw, M(I.colHead[s], NUM_PT, true));
    const int hw = std::max(M("12:59 PM", NUM_PT, true), M(I.head, NUM_PT, true));
    g.W = std::max(I.twoCol ? BOX_PAD + g.lw + 8 + g.vw + 8 + g.vw + BOX_PAD : BOX_PAD + g.lw + 10 + g.vw + BOX_PAD, hw + 2 * BOX_PAD);
    if (I.topLines > 0) g.W = std::max(g.W, I.topW + 2 * BOX_PAD);
    g.H = std::min(maxH, BOX_PAD + NUM_LH * (head + g.rows) + BOX_PAD - 2);
    return g;
}
// the hover grid: EVERY row, flowing into column groups of rowsPer rows (label | left | right), as wide as the rows' templates
struct GridGeom { struct G { int x = 0, lw = 0, vw = 0; size_t from = 0, n = 0; }; std::vector<G> g; int W = 0, H = 0, rowsPer = 0; };
inline GridGeom gridGeom(const Info& I, int H, const TextW& M) {
    GridGeom o; o.H = H;
    o.rowsPer = (H - BOX_PAD + 2) / NUM_LH - 1; if (o.rowsPer < 1) o.rowsPer = 1;
    int x = BOX_PAD + M("12:59 PM", NUM_PT, true) + 12;                     // the candle's time on the left of the header row
    for (size_t from = 0; from < I.rows.size(); from += (size_t)o.rowsPer) {
        GridGeom::G g; g.from = from; g.n = std::min((size_t)o.rowsPer, I.rows.size() - from); g.x = x;
        for (size_t k = from; k < from + g.n; ++k) { g.lw = std::max(g.lw, M(I.rows[k].label, NUM_PT, true)); g.vw = std::max(g.vw, M(rowTmpl(I.rows[k].label, I.tmpl), NUM_PT, true)); }
        if (I.twoCol) for (int s = 0; s < 2; ++s) g.vw = std::max(g.vw, M(I.colHead[s], NUM_PT, true));
        o.g.push_back(g); x += g.lw + 8 + g.vw + 8 + g.vw + 18;
    }
    o.W = x - 18 + BOX_PAD; return o;
}
// text kept free at the top of the pane (the bars scale to the rest; scale() uses the same number, so the axis stays true)
inline int textReserve(int paneH, int mode) {
    if (mode == MODE_READ || mode == MODE_NONE) return paneH >= 40 ? 4 : 0;                      // the read is in the box: the right side is bars + labels only
    if (paneH >= 92) return 2 * READ_LH + OPT_LH + 4;                      // two read lines + the options line
    return paneH >= 50 ? READ_LH + OPT_LH + 2 : 0;                         // (1.3.0) a short pane still keeps one line (was 0 under 80 px)
}
inline int msgLinesFor(int reserve) { return reserve >= 2 * READ_LH + OPT_LH ? 2 : (reserve >= READ_LH ? 1 : 0); }
// the panel plan for one draw
struct PanelPlan {
    int mode = MODE_READ; bp::Box area, box; bool ok = false;
    std::vector<std::string> read, opt;                                    // read mode: the lines in the box; numbers mode: beside it
    NumGeom num; GridGeom grid; bp::Box gridBox;                           // numbers box / the hover grid
    std::vector<bp::Box> textRects;                                        // numbers mode: the read / options line rectangles
    int yTop = 0;                                                          // spike labels never above this
};
inline PanelPlan planPanel(const bp::Box& area, int paneTop, int mode, const bp::Place& place, const Info& I, const std::string& line1,
                           const std::string& optLine, int reserve, const TextW& M) {
    PanelPlan P; P.mode = clampMode(mode); P.area = area; P.yTop = paneTop + reserve;
    if (area.w() < 40 || area.h() < 12) return P;
    P.ok = true;
    if (P.mode == MODE_READ) {
        const int W = std::min(READ_W, area.w()), H = std::min(area.h(), 2 * BOX_PAD + READ_MAX * READ_LH + OPT_MAX * OPT_LH);
        P.box = bp::placeBox(area, W, H, place);
        const int tw = W - 2 * 6;
        int roomRead = (H - 2 * BOX_PAD) / READ_LH; if (roomRead > READ_MAX) roomRead = READ_MAX;
        P.read = wrapRead(line1, tw, roomRead, M);
        const int used = (int)P.read.size() * READ_LH;
        int roomOpt = (H - 2 * BOX_PAD - used) / OPT_LH; if (roomOpt > OPT_MAX) roomOpt = OPT_MAX;
        P.opt = wrapText(optLine, tw, roomOpt, OPT_PT, false, M);
        P.grid = gridGeom(I, H, M);
        P.gridBox = bp::clampIn(bp::mk(P.box.l, P.box.t, P.box.l + P.grid.W, P.box.t + H), area);
        return P;
    }
    P.num = numGeom(I, area.h(), M);
    P.box = bp::placeBox(area, std::min(P.num.W, area.w()), P.num.H, place);
    // the read beside the box: in the top band, the wider free side of it; never past the last bar
    int L = area.l, R = area.r; const int y = paneTop + 2;
    if (P.box.t < y + reserve && P.box.b > y) { const int lw = P.box.l - 4 - L, rw = R - (P.box.r + 4); if (rw >= lw) L = P.box.r + 4; else R = P.box.l - 4; }
    if (R - L < 20) return P;
    const int lines = msgLinesFor(reserve);
    if (lines >= 2) P.read = wrapRead(line1, R - L, 2, M);
    else if (lines == 1) { const std::string t = fitMessage(line1, R - L, [&](const std::string& u) { return M(u, READ_PT, true); }); if (!t.empty()) P.read.push_back(t); }
    int yy = y;
    for (size_t k = 0; k < P.read.size(); ++k) { const int w = M(P.read[k], READ_PT, true), x0 = (L + R) / 2 - w / 2; P.textRects.push_back(bp::mk(x0, yy, x0 + w + 2, yy + READ_LH)); yy += READ_LH; }
    if (!optLine.empty() && reserve >= (int)P.read.size() * READ_LH + OPT_LH) {
        const std::string t = nl::fitText(optLine, R - L, [&](const std::string& u) { return M(u, OPT_PT, false); });
        if (!t.empty()) { P.opt.push_back(t); const int w = M(t, OPT_PT, false), x0 = (L + R) / 2 - w / 2; P.textRects.push_back(bp::mk(x0, yy, x0 + w + 2, yy + OPT_LH)); }
    }
    return P;
}
// (1.5.0) THE READ LINE (Box shows: Read): one line at the box's Position / Offset, sized to its text, only when the pane has a read;
// the mouse over it -> the detail box (the hover lines wrapped to the pane, below the line, else above it), drawn over the bars
struct LinePlan { bool ok = false, show = false; bp::Box box, hoverBox; std::string text, pre; int impl = 0, preW = 0; std::vector<std::string> hover; };
// (1.6.1, Rassul 2026-10-15 10:31 "in the indicator pane, add the name of the indicator and the questions like Who is arriving?")
// "Pull Stats - Who is Leaving" -> "Pull Stats \xB7 Who is leaving?" - drawn grey in front of the read line, always shown
inline std::string paneQuestion(const char* t) {
    std::string s = t ? t : ""; const size_t k = s.find(" - ");
    if (k == std::string::npos) return s;
    std::string q = s.substr(k + 3);
    if (q.compare(0, 7, "Who is ") == 0 && q.size() > 7 && q[7] >= 'A' && q[7] <= 'Z') q[7] = (char)(q[7] - 'A' + 'a');   // "Who is leaving"
    return s.substr(0, k) + " \xB7 " + q + "?";
}
static const int HOVER_W_MAX = 700, HOVER_PT = 10, HOVER_LH = 14;
inline int implOf(const zread::Line& L) { return L.impl > 0 ? BULL : L.impl < 0 ? BEAR : NEUTRAL; }
inline LinePlan planLine(const bp::Box& area, const bp::Place& place, const zread::Line& L, bool hovering, const TextW& M, const std::string& pre = std::string()) {
    LinePlan P; P.ok = area.w() >= 40 && area.h() >= READ_LH;
    const bool hasText = L.show && !L.text.empty();
    if (!P.ok || (!hasText && pre.empty())) return P;
    if (!pre.empty() && M(pre, READ_PT, true) + 12 < area.w()) { P.pre = pre; P.preW = M(pre + "   ", READ_PT, true); }   // (1.6.1) the name + question
    if (hasText) P.text = nl::fitText(L.text, area.w() - 8 - P.preW, [&](const std::string& u) { return M(u, READ_PT, true); });
    if (P.text.empty() && P.pre.empty()) return P;
    P.show = true; P.impl = P.text.empty() ? 0 : implOf(L);
    P.box = bp::placeBox(area, P.preW + (P.text.empty() ? 0 : M(P.text, READ_PT, true)) + 8, READ_LH + 2, place);
    if (!hovering || P.text.empty()) return P;
    const int W = std::min(HOVER_W_MAX, area.w()), tw = W - 2 * BOX_PAD;
    std::vector<std::string> lines;
    for (size_t i = 0; i < L.hover.size(); ++i) { const std::vector<std::string> w = wrapText(L.hover[i], tw, 4, HOVER_PT, false, M); lines.insert(lines.end(), w.begin(), w.end()); }
    const int maxLines = std::max(1, (area.h() - 2 * BOX_PAD - 2) / HOVER_LH);
    if ((int)lines.size() > maxLines) lines.resize((size_t)maxLines);
    int mw = 0; for (size_t i = 0; i < lines.size(); ++i) mw = std::max(mw, M(lines[i], HOVER_PT, false));
    const int H = (int)lines.size() * HOVER_LH + 2 * BOX_PAD, BW = std::min(W, mw + 2 * BOX_PAD + 2);
    int t = P.box.b + 2; if (t + H > area.b) t = P.box.t - 2 - H; if (t < area.t) t = area.t;
    P.hoverBox = bp::clampIn(bp::mk(P.box.l, t, P.box.l + BW, t + H), area);
    P.hover = lines;
    return P;
}
// (1.3.0) ONE spike label per candle (Rassul 07:02 "they overlap"): the side with the bigger ratio (no ratio: the bigger value)
inline int spikeSideOf(const bool spk[2], const bool ratioOk[2], const double ratio[2], const long long v[2]) {
    if (spk[0] && !spk[1]) return 0;
    if (spk[1] && !spk[0]) return 1;
    if (!spk[0] && !spk[1]) return -1;
    const double a = ratioOk[0] ? ratio[0] : -1, b = ratioOk[1] ? ratio[1] : -1;
    if (a != b) return a > b ? 0 : 1;
    return v[0] >= v[1] ? 0 : 1;
}
// (1.6.0) the pane title IRT shows in the pane's title row (parmsTitle): "<title> v<version>", cut to the buffer, always terminated
inline std::string titleText(const char* title, const char* version) { return std::string(title ? title : "") + " v" + (version ? version : ""); }
inline bool copyTitle(char* out, int size, const char* title, const char* version) {
    if (!out || size <= 1) return false;
    const std::string t = titleText(title, version);
    const size_t n = t.size() < (size_t)(size - 1) ? t.size() : (size_t)(size - 1);
    std::memcpy(out, t.data(), n); out[n] = 0; return true;
}
// (1.6.0, Rassul 22:37 CT) ONE BAR PER CANDLE. bull = the pane's buyer side (BidAskStats 1 = offers lifted; Stack / Pull 0 = bids);
// the other side is the red one. dir = the candle's close direction (+1 up, -1 down, 0 flat / unknown).
struct OneBar { int side = -1; long long v = 0, other = 0; bool bright = false, tick = false; };
inline int oneBarWinner(const long long v[2], int dir, int bull) {
    if (bull != 0 && bull != 1) bull = 1;
    if (v[0] <= 0 && v[1] <= 0) return -1;                              // nothing on either side: no bar
    if (v[0] != v[1]) return v[0] > v[1] ? 0 : 1;                        // the bigger side
    return dir > 0 ? bull : 1 - bull;                                    // a tie: the close direction's side, else red
}
inline OneBar oneBarOf(const long long v[2], const bool spike[2], int dir, int bull) {
    OneBar o; o.side = oneBarWinner(v, dir, bull); if (o.side < 0) return o;
    o.v = v[o.side]; o.other = v[1 - o.side] > 0 ? v[1 - o.side] : 0;
    o.bright = spike[o.side];                                            // bright = the WINNER is a spike
    o.tick = spike[0] && spike[1];                                       // white tick = BOTH sides met their own spike rule
    return o;
}
// the single bar spans both old bars' room (the slot's inner 80%); the tick is 1 px wider each side (inside the slot) so it reads as a
// mark across the bar, at the smaller side's height, 2 px tall (drawn t .. t + 2, the bottom edge unpainted; the box is kept t .. t + 2
// for the overlap checks)
inline void oneBarSpan(int xc, int slot, int& l, int& r) { const Pair p = pairIn(xc, slot); l = p.l[0]; r = p.r[1]; }
inline LBox oneTickBox(int xc, int slot, int yBase, int hSmall, int hBar) {
    int l = 0, r = 0; oneBarSpan(xc, slot, l, r);
    const int sl = xc - slot / 2, sr = sl + (slot > 2 ? slot - 1 : 1);
    if (l - 1 >= sl) --l;
    if (r + 1 <= sr) ++r;
    int h = hSmall < 1 ? 1 : hSmall; if (h > hBar) h = hBar;
    const int t = yBase - h;
    return LBox{ l, t, r, t + 2, '-', -1 };
}
// the letters of a one-bar candle: the winner's at the bottom of the bar; the loser's stacked in the same column (above them) only on
// a both-spiked candle; never over the tick (then the column starts above it). side in the boxes = the REAL side (0 / 1).
inline std::vector<LBox> oneBarLetters(int slotL, int slotR, int cx, int yBottom, int yTop, const OneBar& ob, const std::string ls[2], int bw, int bh,
                                       const LBox* tick) {
    std::vector<LBox> out; if (ob.side < 0) return out;
    const int w = ob.side;
    const std::string lose = ob.tick ? ls[1 - w] : std::string();
    out = letterBoxes(slotL, slotR, cx, cx, yBottom, yTop, ls[w], lose, bw, bh);
    if (tick) {
        bool hit = false; for (size_t k = 0; k < out.size() && !hit; ++k) hit = overlaps(out[k], *tick);
        if (hit) out = letterBoxes(slotL, slotR, cx, cx, tick->t - 2, yTop, ls[w], lose, bw, bh);
    }
    for (size_t k = 0; k < out.size(); ++k) out[k].side = out[k].side == 0 ? w : 1 - w;
    return out;
}

// =====================================================================================================================
// TIME: the 17:00 Central trading day of a bar END stamp (local clock fields), as yyyymmdd; at / after 17:00 = the next day
// =====================================================================================================================
inline long long civilDays(long long y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
inline std::string ymdOf(long long days) {
    const long long z = days + 719468, era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153, day = doy - (153 * mp + 2) / 5 + 1, mon = mp < 10 ? mp + 3 : mp - 9; const long long yr = (long long)yoe + era * 400 + (mon <= 2);
    char b[48]; std::snprintf(b, sizeof(b), "%04lld%02u%02u", yr, mon % 100u, day % 100u); return b;
}
inline std::string sessionOf(int year, int mon, int mday, int hour) {
    if (mon < 1 || mon > 12 || mday < 1 || mday > 31 || hour < 0 || hour > 23) return std::string();
    return ymdOf(civilDays(year, (unsigned)mon, (unsigned)mday) + (hour >= 17 ? 1 : 0));
}
// the candle an event belongs to: IRT stamps bar ENDS, so the first bar end AFTER the stamp; past the forming bar (the chart has not
// made the new bar yet) = the predicted end of a later bar (at most 3 bars ahead); -1 = no bar (a gap, too old)
inline long long eventKey(const std::map<long long, int>& barEnds, long long formingKey, int spb, long long st) {
    if (barEnds.empty() || st <= 0) return -1;
    std::map<long long, int>::const_iterator i = barEnds.upper_bound(st);
    if (i != barEnds.end()) { if (spb > 0 && i->first - st > spb) return -1; return i->first; }
    if (spb <= 0) return formingKey;
    const long long k = formingKey + (long long)spb * ((st - formingKey) / spb + 1);
    return k - formingKey > 3LL * spb ? -1 : k;
}

// =====================================================================================================================
// FILES (no SDK): the lsFlexLevels folder, atomic publish, bounded reads, per-market names
// =====================================================================================================================
inline std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels";
}
inline void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    (void)p;
#endif
}
inline long long fileStamp(const std::string& p) { return nl::fileSig(p); }   // -1 = no regular file
inline bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
// <path>.tmp, the old file to .bak, rename into place; on failure the .bak goes back: a reader never sees a partial file
inline bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp", bak = path + ".bak";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    const bool hadOld = fileStamp(path) >= 0;
    if (hadOld) { std::remove(bak.c_str()); if (!moveOver(path, bak)) { std::remove(tmp.c_str()); return false; } }
    if (!moveOver(tmp, path)) { if (hadOld) moveOver(bak, path); std::remove(tmp.c_str()); return false; }
    if (hadOld) std::remove(bak.c_str());
    return true;
}
inline std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) {
        const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_');
    }
    return out.empty() ? "UNKNOWN" : out;
}
inline std::string marketName(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return fileKey(r.substr(0, 8));
}
// the params file of a market, re-read only when its stamp changes (checked at most every 5 s)
// (a same-second, same-size rewrite keeps the stamp: the file is also re-read once a minute; true only when the result changed)
struct ParamCache { long long stamp = -3, checked = -1, readMs = -1; std::string text = "\x01"; ParamLoad load; };
inline bool refreshParams(ParamCache& C, const std::string& dir, const std::string& market, long long nowMs) {
    if (C.checked >= 0 && nowMs >= C.checked && nowMs - C.checked < 5000) return false;
    C.checked = nowMs;
    const std::string p = dir.empty() || market.empty() ? std::string() : dir + "\\LRA-OrderFlow-" + market + ".csv";
    const long long st = p.empty() ? -1 : fileStamp(p);
    if (st == C.stamp && C.readMs >= 0 && nowMs >= C.readMs && nowMs - C.readMs < 60000) return false;
    C.stamp = st; C.readMs = nowMs;
    std::vector<std::string> lines; bool ok = false;
    if (st >= 0) ok = nl::readBoundedLines(p, lines, 64 * 1024, 400, 512);
    ParamLoad L = parseParams(st >= 0, ok, lines, market);
    std::string t = L.source + "|" + paramsText(L.p); for (size_t i = 0; i < L.notes.size(); ++i) t += "|" + L.notes[i];
    if (t == C.text) return false;
    C.text = t; C.load = L;
    return true;
}

// =====================================================================================================================
// THE SESSION RECORD: lsFlexLevels\<Indicator>\<MKT>-<CONTRACT>-<spb>-<session>.csv, one line per closed candle (written once,
// the whole file published atomically), re-read on load so a restart within the session shows the same history:
//   bar_time|ct_time|dbo|<numbers...>|letters_left|letters_right|spike_left|spike_right|dash_left|dash_right|implication|level|message|numbers
// and the session size distributions in <stem>-<session>-sizes.txt (AGGR|s:n,... / REST|s:n,...) for the large thresholds.
// =====================================================================================================================
struct Row {
    long long t = 0; std::string ct; int dbo = 0; std::vector<long long> nums;
    std::string let[2]; int spike[2] = {0, 0}; long long dash[2] = {-1, -1};
    int impl = NEUTRAL; std::string level, line1, line2;
};
inline std::string clean(const std::string& s, size_t maxLen) { std::string o; for (char c : s) { if (o.size() >= maxLen) break; o += (c == '|' || c == '\n' || c == '\r') ? ' ' : c; } return o; }
inline const char* implText(int i) { return i == BULL ? "bull" : i == BEAR ? "bear" : "neutral"; }
inline std::string rowLine(const Row& r) {
    std::string o = std::to_string(r.t) + "|" + clean(r.ct, 12) + "|" + (r.dbo ? "1" : "0");
    for (size_t i = 0; i < r.nums.size(); ++i) o += "|" + std::to_string(r.nums[i] < 0 ? 0 : r.nums[i]);
    for (int s = 0; s < 2; ++s) o += "|" + clean(r.let[s], 2);
    for (int s = 0; s < 2; ++s) o += std::string("|") + (r.spike[s] ? "1" : "0");
    for (int s = 0; s < 2; ++s) o += "|" + (r.dash[s] < 0 ? std::string("-") : std::to_string(r.dash[s]));
    o += std::string("|") + implText(r.impl) + "|" + clean(r.level, 40) + "|" + clean(r.line1, 300) + "|" + clean(r.line2, 300);
    return o;
}
inline bool allDigits(const std::string& f, size_t maxLen) { if (f.empty() || f.size() > maxLen) return false; for (char c : f) if (c < '0' || c > '9') return false; return true; }
// strict: exactly 3 + nNums + 10 fields, each checked; anything else = not a record. (1.2.0) a record written by 1.0 / 1.1 has
// REC_NEW fewer numbers: it is read with those (the last ones) as 0.
inline bool parseRow(const std::string& ln, size_t nNums, Row& out) {
    if (ln.empty() || ln.size() > 4096) return false;
    const std::vector<std::string> c = nl::fields(ln);
    size_t have = nNums;
    if (c.size() != 13 + nNums) { if (nNums >= (size_t)REC_NEW && c.size() == 13 + nNums - REC_NEW) have = nNums - REC_NEW; else return false; }
    Row r;
    if (!allDigits(c[0], 13) || c[1].size() > 12 || (c[2] != "0" && c[2] != "1")) return false;
    r.t = std::strtoll(c[0].c_str(), nullptr, 10); if (r.t <= 0) return false;
    r.ct = c[1]; r.dbo = c[2] == "1" ? 1 : 0;
    for (size_t i = 0; i < have; ++i) { if (!allDigits(c[3 + i], 15)) return false; r.nums.push_back(std::strtoll(c[3 + i].c_str(), nullptr, 10)); }
    while (r.nums.size() < nNums) r.nums.push_back(0);                     // (1.2.0) legacy record: the new numbers read 0
    size_t k = 3 + have;
    for (int s = 0; s < 2; ++s) { const std::string& L = c[k++]; if (L.size() > 2) return false; std::string o; for (char ch : L) { if (ch == 'R') ch = 'I'; if (ch != 'S' && ch != 'L' && ch != 'I') return false; o += ch; } r.let[s] = o; }   // 1.0.1: legacy R = I
    for (int s = 0; s < 2; ++s) { const std::string& f = c[k++]; if (f != "0" && f != "1") return false; r.spike[s] = f == "1"; }
    for (int s = 0; s < 2; ++s) { const std::string& f = c[k++]; if (f == "-") r.dash[s] = -1; else if (allDigits(f, 12)) r.dash[s] = std::strtoll(f.c_str(), nullptr, 10); else return false; }
    const std::string& im = c[k++];
    if (im == "bull") r.impl = BULL; else if (im == "bear") r.impl = BEAR; else if (im == "neutral") r.impl = NEUTRAL; else return false;
    if (c[k].size() > 40 || c[k + 1].size() > 300 || c[k + 2].size() > 300) return false;
    r.level = c[k]; r.line1 = c[k + 1]; r.line2 = c[k + 2];
    out = r; return true;
}
static const long long SESSION_MAX_BYTES = 6 * 1024 * 1024; static const size_t SESSION_MAX_LINES = 6000;
struct SessionFile {
    std::string session, path, sizesPath, head; size_t nNums = 0;
    std::map<long long, Row> rows;
    bool dirty = false; long long writes = 0, fails = 0, loaded = 0, rejected = 0;
    std::string sizesText; bool sizesDirty = false;
    // 1.1.0: extra event lines kept after the candle rows (prefix extraTag, e.g. "SR|"), bounded; extraHead is written once above them
    std::string extraTag, extraHead; std::vector<std::string> extra;
    bool putExtra(const std::string& ln) {
        if (extraTag.empty() || ln.compare(0, extraTag.size(), extraTag) != 0 || ln.size() > 1024 || extra.size() >= 2000) return false;
        if (std::find(extra.begin(), extra.end(), ln) != extra.end()) return false;
        extra.push_back(ln); dirty = true; return true;
    }
    void open(const std::string& sess, const std::string& p, const std::string& sp, const std::string& header, size_t n) {
        session = sess; path = p; sizesPath = sp; head = header; nNums = n; rows.clear(); dirty = false; sizesText.clear(); sizesDirty = false; extra.clear();
    }
    // this session's rows (bounded: 6 MB, 6,000 lines of <= 4 KB), every line validated; false = no file / unreadable
    bool load() {
        if (path.empty()) return false;
        std::vector<std::string> lines;
        if (!nl::readBoundedLines(path, lines, (size_t)SESSION_MAX_BYTES, SESSION_MAX_LINES, 4096)) return false;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].empty() || lines[i][0] == '#' || lines[i].compare(0, 9, "bar_time|") == 0) continue;
            if (!extraTag.empty() && lines[i].compare(0, extraTag.size(), extraTag) == 0) { if (extra.size() < 2000 && lines[i].size() <= 1024) extra.push_back(lines[i]); continue; }
            Row r; if (parseRow(lines[i], nNums, r)) { rows[r.t] = r; loaded++; } else rejected++;
        }
        return true;
    }
    bool loadSizes(SizeDist& aggr, SizeDist& rest) {
        std::vector<std::string> lines;
        if (sizesPath.empty() || !nl::readBoundedLines(sizesPath, lines, 512 * 1024, 4, 300000)) return false;
        SizeDist a, r; bool ga = false, gr = false;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i].compare(0, 5, "AGGR|") == 0) ga = a.load(lines[i].substr(5));
            else if (lines[i].compare(0, 5, "REST|") == 0) gr = r.load(lines[i].substr(5));
        }
        if (!ga || !gr) return false;
        aggr = a; rest = r; return true;
    }
    void put(const Row& r) { if (rows.count(r.t)) return; if (rows.size() >= SESSION_MAX_LINES - 4) return; rows[r.t] = r; dirty = true; }   // once per candle
    void sizes(const SizeDist& aggr, const SizeDist& rest) { const std::string t = "AGGR|" + aggr.save() + "\nREST|" + rest.save() + "\n"; if (t != sizesText) { sizesText = t; sizesDirty = true; } }
    void save() {
        if (path.empty()) return;
        if (dirty) {
            std::string o = head;
            for (std::map<long long, Row>::const_iterator it = rows.begin(); it != rows.end(); ++it) o += rowLine(it->second) + "\n";
            if (!extra.empty()) { if (!extraHead.empty()) o += extraHead + "\n"; for (size_t i = 0; i < extra.size(); ++i) o += extra[i] + "\n"; }
            if (publishFile(path, o)) { writes++; dirty = false; } else fails++;
        }
        if (sizesDirty && !sizesPath.empty()) { if (publishFile(sizesPath, sizesText)) sizesDirty = false; else fails++; }
    }
};

// events per minute: 60 one-second buckets by arrival
struct Rolling {
    long long n[60]; long long sec[60];
    Rolling() { for (int i = 0; i < 60; ++i) { n[i] = 0; sec[i] = -1; } }
    void add(long long ms, long long k = 1) { if (ms < 0) return; const long long s = ms / 1000; const int i = (int)(s % 60); if (sec[i] != s) { sec[i] = s; n[i] = 0; } n[i] += k; }
    long long perMin(long long nowMs) const { const long long s = nowMs / 1000; long long t = 0; for (int i = 0; i < 60; ++i) if (sec[i] >= 0 && s >= sec[i] && s - sec[i] < 60) t += n[i]; return t; }
};

// (1.4.0, 2026-10-14 LIQ-DAY review: "captured fills only 30-45% of real volume in busy periods") COVERAGE SELF-CHECK - how much of
// the traded volume the order feed actually delivered: the FILL lots read (resting side, by the event's own second) over each closed
// candle's bar volume. Pre-market on 10-14 this was 96-100%; in busy RTH 10-45% (the drain ran only when IRT drew the chart, every
// 6-10 s, and IRT keeps only the last few thousand events per subscriber). Bars evaluated once, >= 5 s after they closed.
struct Coverage {
    std::map<long long, long long> fillsAt;          // event second (RTDATE) -> FILL lots read
    std::map<long long, long long> pend;             // closed bar end -> steady ms first seen closed (waits 5 s for late reads)
    std::set<long long> done;                        // bar ends evaluated
    struct Row { long long end = 0, fills = 0, vol = 0; int pct = -1; };
    std::vector<Row> last;                           // the most recent evaluations (<= 10)
    Row low; long long checked = 0, sumFills = 0, sumVol = 0;
    void addFill(long long stamp, long long lots) {
        if (stamp <= 0 || lots <= 0 || lots > 10000000) return;
        fillsAt[stamp] += lots;
        if (fillsAt.size() > 20000) fillsAt.erase(fillsAt.begin());   // ~5.5 h of busy seconds; older bars are long evaluated
    }
    long long fillsIn(long long from, long long to) const {   // [from, to)
        long long t = 0;
        for (std::map<long long, long long>::const_iterator i = fillsAt.lower_bound(from); i != fillsAt.end() && i->first < to; ++i) t += i->second;
        return t;
    }
    static int pctOf(long long fills, long long vol) { if (vol <= 0) return -1; const long long p = (fills * 100 + vol / 2) / vol; return (int)(p > 999 ? 999 : p); }
    // a bar ending at `end` (covers [end - spb, end)) with `vol` lots, seen closed now; returns true when it was evaluated now
    bool offer(long long end, int spb, long long vol, long long nowMs, long long waitMs = 5000) {
        if (end <= 0 || spb <= 0 || done.count(end)) return false;
        std::map<long long, long long>::iterator p = pend.find(end);
        if (p == pend.end()) { pend[end] = nowMs; if (pend.size() > 50) pend.erase(pend.begin()); return false; }
        if (nowMs >= p->second && nowMs - p->second < waitMs) return false;
        pend.erase(p);
        done.insert(end); if (done.size() > 3000) done.erase(done.begin());
        Row r; r.end = end; r.vol = vol; r.fills = fillsIn(end - spb, end); r.pct = pctOf(r.fills, vol);
        if (r.pct < 0) return true;                                   // no volume: nothing to compare
        last.push_back(r); if (last.size() > 10) last.erase(last.begin());
        ++checked; sumFills += r.fills; sumVol += vol;
        if (low.pct < 0 || (r.pct < low.pct && vol >= 20)) low = r;  // a 3-lot bar at 0% is not a meaningful "lowest"
        return true;
    }
    int recentPct() const { long long f = 0, v = 0; for (size_t i = 0; i < last.size(); ++i) { f += last[i].fills; v += last[i].vol; } return pctOf(f, v); }
    void reset() { fillsAt.clear(); pend.clear(); done.clear(); last.clear(); low = Row(); checked = sumFills = sumVol = 0; }
};

// (1.2.0) the options file of a market, re-read at most every 5 s when its stamp changes; each new grid joins the history on the bar
// it ARRIVED on = the bar during which the file was written (its modification time, mapped to a bar end by the host's barOf; a time
// past the forming bar = the forming bar), so a candle's line never changes later, and a restart shows the same lines
struct OptCache { long long sig = -2, checked = -1, readMs = -1; ofo::Snap last; ofo::History hist; std::string why = "no file"; long long reads = 0; };
inline bool refreshOptions(OptCache& C, const std::string& dir, const std::string& market, long long nowMs, const std::function<long long(long long)>& barOf) {
    if (C.checked >= 0 && nowMs >= C.checked && nowMs - C.checked < 5000) return false;
    C.checked = nowMs;
    const std::string p = dir.empty() || market.empty() ? std::string() : dir + "\\LRA-Options-" + market + ".csv";
    const long long sig = p.empty() ? -1 : nl::fileSig(p);
    if (sig == C.sig && (sig < 0 || (C.readMs >= 0 && nowMs >= C.readMs && nowMs - C.readMs < 60000))) return false;   // a same-second, same-size rewrite: re-read once a minute
    C.sig = sig;
    if (sig < 0) { C.last = ofo::Snap(); C.why = "no file"; return true; }
    C.readMs = nowMs;
    std::vector<std::string> lines;
    if (!nl::readBoundedLines(p, lines, 16 * 1024, 200, 300)) { C.last = ofo::Snap(); C.why = "unreadable or too big"; return true; }
    C.reads++;
    C.last = ofo::parse(lines, market);
    C.why = C.last.ok ? std::string("ok") : (C.last.notes.empty() ? std::string("garbled") : C.last.notes[0]);
    C.hist.add(C.last, barOf(nl::fileMtime(p)));
    return true;
}

static const int SLICE_MAX = 5000;                 // events per call at most
static const long long SLICE_BUDGET_MS = 8;        // and about this many ms
static const long long WAIT_MS = 60000;            // no event for 60 s after the 1st open: one 2nd try with NULL
static const long long DECIDE_DELAY_MS = 500;      // a closed candle is decided 0.5 s after it closed with the drain caught up
static const long long FINAL_GRACE_MS = 5000;      // ... or 5 s after it closed whatever the backlog (late events then counted, not used)

} // namespace off

// =====================================================================================================================
// PART 2 - host side (Investor/RT SDK). Define OFF_SDK after including irtsdk.h.
// =====================================================================================================================
#ifdef OFF_SDK
#include "HostSlot.h"
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG in MboPane: the settings window never faults, never opens blank
#include <chrono>
#include <climits>
#include <cstdint>
#include <new>
#include <sstream>

namespace off {

// bDboSubscribed is private: read through a pointer-to-member from an explicit instantiation ([temp.explicit] lets it name private
// members; no layout assumptions) - as lsMboCheck 1.1.0
template <class Tag, typename Tag::type M> struct Rob { friend typename Tag::type robbed(Tag) { return M; } };
struct DboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type robbed(DboFlag); };
template struct Rob<DboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;

inline short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }

// (1.4.0) one getNext() behind a structured-exception guard: 1 = an event, 0 = none queued, -1 = the native call faulted (IRT's
// C0000005 would otherwise unwind past our state). No C++ object with a destructor lives in this frame (MSVC C2712).
inline int safeNext(cppExtension::DEPTH_BY_ORDER* d) {
#if defined(_MSC_VER)
    __try { return d->getNext() == RTX_OK ? 1 : 0; } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#else
    try { return d->getNext() == RTX_OK ? 1 : 0; } catch (...) { return -1; }
#endif
}
static const long long TIMER_MS = 250;              // (1.4.0) the order-feed clock: IRT keeps only the last few thousand events
static const int TIMER_MAX = 50000;                 //   per subscriber, so it is read 4x a second whatever the chart is doing,
static const long long TIMER_BUDGET_MS = 20;        //   up to 50,000 events or ~20 ms per tick
static const size_t QUEUE_MAX = 400000;             //   events read by the clock wait here (<= ~20 MB) for the chart's next calc

// ---- the order feed of one chart (lsMboCheck 1.1.0 pattern) --------------------------------------------------------
struct Conn {
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr;
    int attempt = 0, flag = -1; bool blocked = false, dead = false;
    long long openMs = -1, lastResubMs = -1, lastEventMs = -1;
    long long faults = 0, slices = 0, capHits = 0, backlogRun = 0, longestBacklog = 0, events = 0;
    bool lastCapped = false; std::string via; Rolling roll;
    // (1.4.0) the clock path: events read by the order-feed timer wait in q (with their arrival ms) until the chart's calc / draw
    // routes them, in order, before reading more. `raw` sees every event exactly once, as it is read (the recorder).
    struct QEv { DEPTH_ORDER o; long long ms; };
    std::vector<QEv> q; size_t qPeak = 0;
    long long qDropped = 0, timerTicks = 0, timerEvents = 0, timerFaults = 0, pumpEvents = 0;
    bool timerOff = false;                           // a faulting timer read: the clock stops reading (never re-enter it)
    // (1.17.0) the watchdog: the chart's last volume key and when it changed, the dropout being handled, reconnects made
    long long tradeKey = -1, lastTradeMs = -1, silentFromMs = -1, silentFromWall = 0, reconnects = 0, lastReconnectMs = -1, dropouts = 0;
    bool silentNow(long long now) const { return silentFromMs >= 0 && (dbo == nullptr || lastEventMs <= silentFromMs || now - lastEventMs >= FEED_SILENT_MS); }
    // chart context, before open(): volKey changes whenever the chart's last bar trades; a silent feed is closed so open() reopens it
    void watch(long long now, long long volKey) {
        if (volKey != tradeKey) { if (tradeKey != -1) lastTradeMs = now; tradeKey = volKey; }
        if (silentFromMs >= 0 && dbo && lastEventMs > silentFromMs && now - lastEventMs < FEED_SILENT_MS) silentFromMs = -1;   // events again: recovered
        if (!feedSilent(dbo != nullptr, flag, lastEventMs, lastTradeMs, now)) return;
        if (silentFromMs < 0) { silentFromMs = lastEventMs; silentFromWall = (long long)std::time(NULL) - (now - lastEventMs) / 1000; ++dropouts; }
        if (!reconnectDue(lastReconnectMs, now)) return;
        delete dbo; dbo = nullptr; attempt = 0; flag = -1; dead = false; timerOff = false; openMs = -1; lastResubMs = -1; via.clear();
        ++reconnects; lastReconnectMs = now;
    }
    std::function<void(const DEPTH_ORDER&)> raw;
    Coverage cov;
    Conn() {}
    Conn(const Conn&) = delete; Conn& operator=(const Conn&) = delete;
    ~Conn() { delete dbo; }
    void reset() { delete dbo; dbo = nullptr; attempt = 0; flag = -1; blocked = dead = false; openMs = lastResubMs = lastEventMs = -1; lastCapped = false; via.clear();
                   q.clear(); timerOff = false; cov.reset(); }
    bool named() const { return dbo && via.compare(0, 13, "chart symbol ") == 0; }   // opened with the chart symbol spelled out:
                                                                                         // every SDK call names it, so readable off the chart
    void took(const DEPTH_ORDER& o) {                               // every event read, on either path
        if (o.action == 3) cov.addFill((long long)o.timestamp, (long long)o.size);
        if (raw) raw(o);
    }
    // (1.4.0) the clock path (timer, any chart context): only a stream opened with the chart symbol spelled out is read here
    template <class Clock> int prefetch(long long now, Clock clock) {
        if (!dbo || dead || timerOff || !named() || flag != 1) return 0;
        const long long t0 = clock(); int n = 0;
        ++timerTicks;
        for (;;) {
            if (n >= TIMER_MAX || ((n & 1023) == 1023 && clock() - t0 > TIMER_BUDGET_MS)) { capHits++; break; }
            const int r = safeNext(dbo);
            if (r < 0) { timerFaults++; faults++; timerOff = true; break; }
            if (r == 0) break;
            ++n; took(dbo->dbo);
            if (q.size() < QUEUE_MAX) { QEv e; e.o = dbo->dbo; e.ms = now; q.push_back(e); } else qDropped++;
        }
        if (q.size() > qPeak) qPeak = q.size();
        if (n > 0) { lastEventMs = now; slices++; events += n; timerEvents += n; roll.add(now, n); }
        return n;
    }
    // the queued events, in order, to the engine (chart context); `route(const DEPTH_ORDER&, long long arrivalMs)`
    template <class Route> size_t flushQueue(Route route) {
        const size_t n = q.size();
        for (size_t i = 0; i < n; ++i) route(q[i].o, q[i].ms);
        q.clear();
        if (q.capacity() > 65536) std::vector<QEv>().swap(q);        // give a burst's memory back
        return n;
    }
    bool due(long long now) const {
        if (blocked || dead || attempt >= 2) return false;
        if (attempt == 1) return lastEventMs < 0 && openMs >= 0 && now - openMs >= WAIT_MS;
        return true;
    }
    // chart context only (calc / draw). 1st: the chart symbol spelled out; after 60 s with no event, one 2nd try with NULL.
    // A crash-guard file marks each open; if IRT dies inside it, the next start does not open it again (delete it to retry).
    void open(const std::string& symbol, const std::string& guard, long long now, std::string& state) {
        if (!due(now)) return;
        if (attempt == 0 && !guard.empty() && fileStamp(guard) >= 0) { blocked = true; attempt = 2; state = "IRT closed while opening the order feed last time: delete " + guard + " to retry"; return; }
        if (!guard.empty()) { std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER try " << (attempt + 1) << "\n"; }
        try {
            if (attempt == 0) {
                const bool named = !symbol.empty();
                dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(named ? symbol.c_str() : NULL);
                via = named ? "chart symbol " + symbol : "default (NULL = this chart)";
                attempt = named ? 1 : 2;
            } else {
                delete dbo; dbo = nullptr;                                 // its destructor unsubscribes
                dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(NULL);
                via = "default (NULL = this chart), 2nd try";
                attempt = 2;
            }
        } catch (...) { faults++; dead = true; }
        flag = -1; openMs = now;
        if (!dbo && !dead) { dead = true; state = "order feed unavailable (out of memory)"; }
        if (!guard.empty()) std::remove(guard.c_str());
    }
    // at most SLICE_MAX events or ~SLICE_BUDGET_MS; the rest waits for the next call. Re-subscribe asked at most once a second.
    template <class Clock, class Sink> int drain(long long now, Clock clock, Sink sink) {
        lastCapped = false;
        if (!dbo || dead) return 0;
        if (flag != 1 && lastResubMs >= 0 && now - lastResubMs < 1000 && now >= lastResubMs) return 0;
        const long long t0 = clock(); int n = 0; bool cap = false;
        try {
            if (!(dbo->*robbed(DboFlag()))) lastResubMs = now;
            for (;;) {
                if (n >= SLICE_MAX) { cap = true; break; }
                if ((n & 511) == 511 && clock() - t0 > SLICE_BUDGET_MS) { cap = true; break; }
                if (dbo->getNext() != RTX_OK) break;
                ++n; took(dbo->dbo); sink(dbo->dbo);                        // (1.4.0) took(): recorder + coverage, once per event
            }
            flag = (dbo->*robbed(DboFlag())) ? 1 : 0;                      // after the reads: getNext may have just subscribed
        } catch (...) { faults++; dead = true; }                            // never re-enter a faulting call
        if (n > 0) { lastEventMs = now; slices++; events += n; pumpEvents += n; roll.add(now, n); }
        if (cap) { capHits++; backlogRun++; if (backlogRun > longestBacklog) longestBacklog = backlogRun; } else backlogRun = 0;
        lastCapped = cap;
        return n;
    }
    std::string stateText(long long now) const {
        if (blocked) return std::string();
        if (dead) return "order feed fault: reload indicator";
        if (flag == 0) return "MBO: not subscribed";
        if (lastEventMs < 0) return openMs >= 0 && now - openMs >= WAIT_MS && attempt >= 2 ? "MBO: no events" : "MBO: waiting";
        return "ready";
    }
};

// ---- drawing ----------------------------------------------------------------------------------------------------------------
static const COLOR C_WHITE = 0x00FFFFFF, C_BLACKC = 0x00000000, C_NUMS = 0x009CA3AF, C_NEUTRAL = 0x00D8DDE3;
static const COLOR C_GREEN_BAR = 0x001F9D55, C_RED_BAR = 0x00962D2D, C_GREEN_SPIKE = 0x0000FF00, C_RED_SPIKE = 0x00FF3B3B;
static const COLOR C_LBL_RATIO = 0x00FFE14D, C_LBL_LOTS = 0x00FFFFFF, C_LBL_PACE = 0x00B8BEC7;   // (1.9.0) label colours: ratio yellow, lots white, pace grey
inline std::string noX(std::string s) { if (!s.empty() && s[s.size() - 1] == 'x') s.erase(s.size() - 1); return s; }   // "2.8x" -> "2.8"
inline COLOR implColor(int impl) { return impl == BULL ? C_GREEN_SPIKE : impl == BEAR ? C_RED_SPIKE : C_NEUTRAL; }
inline void fillRect(int l, int t, int r, int b, COLOR border, COLOR fill, int w = 0) {
    cppExtension::RCT rc; rc.set(coord(l), coord(t), coord(r), coord(b)); rc.draw((short)w, border, fill, cppExtension::DRAW_OPAQUE, PAT_SOLID);
}
// 1.0.1 (Rassul 1:43 PM: the white outline on the bars and the letter boxes): a filled rectangle with NO visible border - the pen
// in effect is set to the fill colour first (IRT draws a rectangle's perimeter with the pen in effect, which another draw may
// have left white), and the border colour is the fill colour too. Bars, letter boxes and the refill dash all use this.
inline void fillFlat(cppExtension* x, int l, int t, int r, int b, COLOR fill) {
    x->setPen(fill, 1, P_SOLID);
    fillRect(l, t, r, b, fill, fill, 0);
}
struct BarLook { bool ok = false; long long v[2] = {0, 0}; bool spike[2] = {false, false}; std::string let[2]; long long dash[2] = {-1, -1}; bool ratioOk[2] = {false, false}; double ratio[2] = {0, 0};
                 std::string mark[2]; COLOR markCol[2] = {0, 0};      // 1.1.0: the stop-run mark ("R" / "R?") per side, its colour
                 long long p100[2] = {0, 0};                           // 1.2.0: the pace (x100, 0 = none) for the spike label's middle row
                 int dir = 0;                                          // 1.6.0: the candle's close direction (one-bar ties)
                 long long q[2] = {0, 0};                               // 1.7.0 (BookFlow): the lots on the spike label (net bars: the added or the pulled part)
                 std::string lab1[2], lab2[2]; };                      // (1.18.0) pair bars: ready-made label rows (Book Flow: ratio, R refilled); empty = from ratio / pace
inline bool hiOf(const BarLook& b, int s) { return b.spike[s] || (b.ratioOk[s] && b.ratio[s] >= HIGH_X); }   // (1.9.0) a side at 2.0x+ its average (or a spike)
struct PaneGeo { int left = 0, top = 0, right = 0, bottom = 0, yBase = 0, px = 1, reserve = 0, edge = 0; };
// the pane geometry: bars from yBase = bottom - 1 up px pixels; the text area ends at the last bar (never the empty space right
// of price) and never over the price axis
inline PaneGeo paneGeo(cppExtension* x, int mode) {
    PaneGeo g; cppExtension::RCT pane; pane.getPaneRect(false);
    g.left = pane.left; g.top = pane.top; g.right = pane.right; g.bottom = pane.bottom;
    g.reserve = textReserve(pane.bottom - pane.top, mode);                // (1.3.0) by the Box shows mode (scale() uses the same)
    g.yBase = pane.bottom - 1; g.px = drawPx(pane.bottom - pane.top, g.reserve);
    cppExtension::RCT scl; scl.getScaleRect();
    int edge = pane.right;
    if (scl.left > pane.left && scl.left < edge) edge = scl.left - 2;
    const long n = x->getBarCount();
    if (n > 0) { cppExtension::PNT e; if (e.set((int)n - 1, 0.0f, cppExtension::kBarRight) == RTX_OK && e.h > pane.left + 120 && e.h < edge) edge = e.h; }
    g.edge = edge;
    return g;
}
// the candles: two bars each (left / right), spike colours, white refill dashes, letters - all with no outline (1.0.1: the hovered
// candle is no longer outlined white; the info box and the message line show which candle is read). (1.6.0) oneBull = 0 / 1 (the pane's
// buyer side): ONE bar per candle (oneBarOf); -1 (default) = the two bars as before. marks = false: no letters / R marks / refill dash
// (StackStats, Rassul 23:00 CT; the hover and the records keep them).
inline void drawBars(cppExtension* x, const PaneGeo& g, int v0, int v1, int slot, long long top, int hov, const COLOR normal[2], const COLOR spike[2],
                     const std::function<int(int)>& xOf, const std::function<BarLook(int)>& look, const std::vector<LBox>& avoid = std::vector<LBox>(),
                     int oneBull = -1, bool marks = true, bool onlyHigh = false) {   // (1.9.0) onlyHigh: only bars at 2.0x+ their average are bright and labelled (Aggression); the rest are drawn dim
    if (top <= 0) return;
    const bool one = oneBull == 0 || oneBull == 1;
    cppExtension::FONT lf; lf.id = cppExtension::HELVETICA; lf.size = 8; lf.style = cppExtension::BOLD;   // (1.3.0) 8 pt white text (was 7 pt in white boxes)
    std::vector<std::pair<int, BarLook> > lets;
    for (int i = v0; i <= v1; ++i) {
        const BarLook b = look(i); if (!b.ok) continue;
        const int xc = xOf(i); if (xc < g.left || xc > g.right) continue;
        const Pair p = pairIn(xc, slot);
        (void)hov;                                                          // 1.0.1: no hover outline
        if (one) {                                                          // (1.6.0) the bigger side's bar, its dash, the both-spiked tick
            const OneBar ob = oneBarOf(b.v, b.spike, b.dir, oneBull);
            if (ob.side >= 0) {
                const int w = ob.side; int l = 0, r = 0; oneBarSpan(xc, slot, l, r);
                const int hb = barHeight(ob.v, top, g.px);
                if (hb > 0) fillFlat(x, l, g.yBase - hb, r, g.yBase, (ob.bright || (onlyHigh && hiOf(b, w))) ? spike[w] : normal[w]);
                if (marks && b.dash[w] >= 0 && hb > 0) {                   // the winner's refill dash (marks off = StackStats: none)
                    int dh = barHeight(b.dash[w] < ob.v ? b.dash[w] : ob.v, top, g.px); if (dh < 2) dh = 2; if (dh > hb) dh = hb;
                    fillFlat(x, l, g.yBase - dh, r, g.yBase - dh + 2, C_WHITE);
                }
                if (ob.tick && hb > 0 && !onlyHigh) { const LBox t = oneTickBox(xc, slot, g.yBase, barHeight(ob.other, top, g.px), hb); fillFlat(x, t.l, t.t, t.r, t.b, C_WHITE); }
            }
            if (marks && (!b.let[0].empty() || !b.let[1].empty() || !b.mark[0].empty() || !b.mark[1].empty())) lets.push_back(std::make_pair(i, b));
            continue;
        }
        int h[2] = {0, 0};
        for (int s = 0; s < 2; ++s) {
            h[s] = barHeight(b.v[s], top, g.px);
            const COLOR c = b.spike[s] ? spike[s] : normal[s];
            if (h[s] > 0) fillFlat(x, p.l[s], g.yBase - h[s], p.r[s], g.yBase, c);
            if (b.dash[s] >= 0 && h[s] > 0) {                               // the refill dash: inside the bar at min(refills, bar)
                int dh = barHeight(b.dash[s] < b.v[s] ? b.dash[s] : b.v[s], top, g.px); if (dh < 2) dh = 2; if (dh > h[s]) dh = h[s];
                fillFlat(x, p.l[s], g.yBase - dh, p.r[s], g.yBase - dh + 2, C_WHITE);
            }
        }
        if (!b.let[0].empty() || !b.let[1].empty() || !b.mark[0].empty() || !b.mark[1].empty()) lets.push_back(std::make_pair(i, b));
    }
    x->setFont(lf);
    std::vector<LBox> allLets(avoid);                 // the info box / message line: spike labels never go over them
    if (!lets.empty()) {
        const int bw = std::max(9, (int)x->getTextWidth("W", -1) + 3), bh = 11;
        for (size_t k = 0; k < lets.size(); ++k) {
            const int xc = xOf(lets[k].first); const Pair p = pairIn(xc, slot);
            const int sl = xc - slot / 2, sr = sl + (slot > 2 ? slot - 1 : 1);
            const BarLook& lb = lets[k].second;
            std::string ls[2];
            for (int s = 0; s < 2; ++s) ls[s] = lb.mark[s].empty() ? lb.let[s] : lb.let[s].substr(0, 1) + "R";   // 1.1.0: the mark lowest
            std::vector<LBox> bx;
            if (one) {                                                      // (1.6.0) on the single bar: the winner's (+ the loser's when both spiked)
                const OneBar ob = oneBarOf(lb.v, lb.spike, lb.dir, oneBull);
                if (ob.side < 0) continue;
                const int hb = barHeight(ob.v, top, g.px);
                const LBox t = oneTickBox(xc, slot, g.yBase, barHeight(ob.other, top, g.px), hb);
                bx = oneBarLetters(sl, sr, (p.l[0] + p.r[1]) / 2, g.yBase - 1, g.top + g.reserve, ob, ls, bw, bh, ob.tick && hb > 0 ? &t : nullptr);
            } else bx = letterBoxes(sl, sr, (p.l[0] + p.r[0]) / 2, (p.l[1] + p.r[1]) / 2, g.yBase - 1, g.top + g.reserve, ls[0], ls[1], bw, bh);
            for (size_t j = 0; j < bx.size(); ++j) {
                const int sd = bx[j].side == 1 ? 1 : 0;
                const bool isMark = bx[j].c == 'R' && !lb.mark[sd].empty();
                std::string txt(1, bx[j].c);
                if (bx[j].c == 'I' && !isMark) {                                // (1.5.0 audit) the refill / iceberg letter is inferred: "I?"
                    const int need = (int)x->getTextWidth("I?", -1) + 3;
                    LBox w = bx[j]; const int cx = (bx[j].l + bx[j].r) / 2; w.l = std::max(sl, cx - need / 2); w.r = w.l + need - 1;
                    if (w.r > sr) { w.r = sr; w.l = sr - need + 1; }
                    bool hit = w.l < sl; for (size_t o = 0; o < bx.size() && !hit; ++o) if (o != j) hit = overlaps(w, bx[o]);
                    if (!hit) { bx[j] = w; txt = "I?"; }                       // zoomed far out: "I" alone (the hover / record say "?")
                }
                if (isMark) {                                                   // "R?" may need a wider box: centred, else slid inside the slot
                    txt = lb.mark[sd];
                    const int need = (int)x->getTextWidth(txt.c_str(), -1) + 3;
                    if (need > bx[j].r - bx[j].l + 1) {
                        bool placed = false; const int cx = (bx[j].l + bx[j].r) / 2;
                        const int tries[3] = { cx - need / 2, sl, sr - need + 1 };
                        for (int q = 0; q < 3 && !placed; ++q) {
                            LBox w = bx[j]; w.l = tries[q]; w.r = w.l + need - 1;
                            if (w.l < sl || w.r > sr) continue;
                            bool hit = false; for (size_t o = 0; o < bx.size() && !hit; ++o) if (o != j) hit = overlaps(w, bx[o]);
                            if (!hit) { bx[j] = w; placed = true; }
                        }
                        if (!placed) txt = "R";                                 // zoomed far out: "R" (the hover box / message still say "?")
                    }
                }
                allLets.push_back(bx[j]);
                // (1.3.0, Rassul 07:16 "remove the white boxes, just keep the signals and use white font instead"): the letter only -
                // no box, no border; S / L / I white, the stop-run R in its red / green
                x->setTextColor(isMark ? lb.markCol[sd] : C_WHITE);
                cppExtension::RCT r; r.set(coord(bx[j].l), coord(bx[j].t), coord(bx[j].r + 1), coord(bx[j].b + 1)); r.drawText(txt.c_str(), true, false);
            }
        }
    }
    // (1.9.0, Rassul 2026-10-16 18:04-18:06) spike labels: the RATIO above the bar (yellow); the LOTS, then the PACE, INSIDE the bar near its
    // top (white / grey, no "p"); tight rows. Biggest bars first; a label that would sit on another is skipped, never moved. Laid out from
    // labelLead candles left of the view (never drawn) so a scroll does not re-decide a label still in view.
    struct L9 { int cx = 0, hb = 0; long long mag = 0; std::string x, q, p; bool view = true; };
    std::vector<L9> sp;
    for (int i = std::max(0, v0 - labelLead(slot)); i <= v1; ++i) {
        const BarLook b = look(i); if (!b.ok) continue;
        const int xc = xOf(i); if (xc > g.right) continue;
        const Pair p = pairIn(xc, slot);
        int lab = -1;
        if (one) { const OneBar ob = oneBarOf(b.v, b.spike, b.dir, oneBull); lab = onlyHigh ? ob.side : (ob.bright ? ob.side : -1); }
        else lab = spikeSideOf(b.spike, b.ratioOk, b.ratio, b.v);
        if (lab < 0) continue;
        if (onlyHigh ? !hiOf(b, lab) : !b.spike[lab]) continue;
        L9 s9; s9.cx = one ? (p.l[0] + p.r[1]) / 2 : (p.l[lab] + p.r[lab]) / 2; s9.hb = barHeight(b.v[lab], top, g.px); s9.mag = b.v[lab];
        s9.x = b.ratioOk[lab] ? noX(ratioText(b.ratio[lab])) : std::string(); s9.q = std::to_string(b.v[lab]); s9.p = b.p100[lab] > 0 ? noX(paceX(b.p100[lab])) : std::string();
        s9.view = i >= v0 && xc >= g.left;
        sp.push_back(s9);
    }
    if (sp.empty()) return;
    std::stable_sort(sp.begin(), sp.end(), [](const L9& a, const L9& b) { return a.mag > b.mag; });
    cppExtension::FONT sf; sf.id = cppExtension::HELVETICA; sf.size = LABEL_PT; sf.style = cppExtension::PLAIN; x->setFont(sf);
    std::vector<LBox> placed(allLets);
    for (size_t k = 0; k < sp.size(); ++k) {
        const L9& c = sp[k];
        std::string in[2]; COLOR ic[2] = { C_LBL_LOTS, C_LBL_LOTS }; int nIn = 0;
        if (!c.q.empty()) { in[nIn] = c.q; ic[nIn++] = C_LBL_LOTS; }
        if (!c.p.empty()) { in[nIn] = c.p; ic[nIn++] = C_LBL_PACE; }
        while (nIn > 0 && c.hb < nIn * LABEL_LH + 2) --nIn;                       // only the rows that fit inside the bar
        // (1.9.1, Rassul 2026-10-16 18:32) Aggression (onlyHigh): the RATIO (yellow) with the PACE (grey) under it, both ABOVE the bar; no lots
        std::string ab[2]; COLOR ac[2] = { C_LBL_RATIO, C_LBL_PACE }; int nA = 0;
        if (onlyHigh) { nIn = 0; if (!c.x.empty()) { ab[nA] = c.x; ac[nA++] = C_LBL_RATIO; } if (!c.p.empty()) { ab[nA] = c.p; ac[nA++] = C_LBL_PACE; } }
        else if (!c.x.empty()) { ab[nA] = c.x; ac[nA++] = C_LBL_RATIO; }
        if (nA == 0 && nIn == 0) continue;
        const int yTop = g.yBase - c.hb;
        const int tr = yTop - 2 - nA * LABEL_LH;                                  // the stack sits just above the bar
        const int tIn = yTop + 1;                                                 // the inside rows start under its top edge
        const int t0 = nA == 0 ? tIn : tr, b0 = nIn > 0 ? tIn + nIn * LABEL_LH : yTop + 1;
        if (t0 < g.top + g.reserve || b0 > g.yBase + 1) continue;
        int w = 0;
        for (int r = 0; r < nA; ++r) w = std::max(w, (int)x->getTextWidth(ab[r].c_str(), -1));
        for (int r = 0; r < nIn; ++r) w = std::max(w, (int)x->getTextWidth(in[r].c_str(), -1));
        const LBox me = { c.cx - w / 2 - 1, t0, c.cx - w / 2 + w + 1, b0 - 1, 0, 0 };
        if (me.l < g.left + 1 || me.r > g.edge - 1) continue;
        bool hit = false; for (size_t o = 0; o < placed.size() && !hit; ++o) hit = overlaps(me, placed[o]);
        if (hit) continue;
        placed.push_back(me);
        if (!c.view) continue;                                                    // a candle left of the view: laid out, not drawn
        auto put = [&](const std::string& s, int y, COLOR col) {
            if (s.empty()) return; x->setTextColor(col); const int tw = (int)x->getTextWidth(s.c_str(), -1);
            cppExtension::RCT rc; rc.set(coord(c.cx - tw / 2), coord(y), coord(c.cx - tw / 2 + tw + 2), coord(y + LABEL_LH)); rc.drawText(s.c_str(), false, false); };
        for (int r = 0; r < nA; ++r) put(ab[r], tr + r * LABEL_LH, ac[r]);
        for (int r = 0; r < nIn; ++r) put(in[r], tIn + r * LABEL_LH, ic[r]);
    }
}
// (1.18.0) pair bars: look.v[s] per side; left = the side drawn on the left (Aggression: 1 = buyers, Book Flow: 0 = bids); green / red by side
// (normal / spike colours indexed by side). top = the capped scale (a taller bar stops at the top with a white tip); a negative value
// (Book Flow: more pulled than added) is a 2 px stub. Bright (hiOf) bars carry the ratio (yellow) and pace / R (grey) above them.
// IRT's mapping of this pane, read at candle `bar`: the rows of value 0 and value `top` (inside a draw)
inline ValY paneValY(int bar, long long top, const PaneGeo& g) {
    if (top <= 0) return ValY();
    cppExtension::PNT a; a.set(bar, 0.0f); cppExtension::PNT b; b.set(bar, (float)top);
    return valYOf((double)a.v, (double)b.v, top, g.top, g.bottom);
}
// a bar's height and its zero row the same way drawPairBars places it (for things drawn on top of a bar: marks, caps)
inline int pairBarH(const ValY* vy, const PaneGeo& g, long long top, long long v, int& yb) {
    if (vy && vy->ok) { yb = std::min(g.bottom - 1, vy->yOf(0)); int a = 0, b = 0; bool c = false; return v > 0 ? axisBar(*vy, g.top + g.reserve + 1, g.bottom - 1, v, a, b, c) : 0; }
    yb = g.bottom - 1; return v > 0 ? barHeight(v, top, pairPx(g.bottom - g.top, g.reserve)) : 0;
}
// (Effort Reward 1.24.0 / Defense 1.14.0, Rassul 2026-10-20 14:33 "get rid of the labels like 2.4 etc and R1 2, keep percentages instead")
// no ratio / Refilled labels over the bright bars (the bright colour still marks the spike; the numbers stay in the info box)
static const bool PAIR_LABELS = false;
inline void drawPairBars(cppExtension* x, const PaneGeo& g, int v0, int v1, int slot, long long top, const COLOR normal[2], const COLOR spike[2],
                         const std::function<int(int)>& xOf, const std::function<BarLook(int)>& look, const std::vector<LBox>& avoid, int left, const ValY* vy = nullptr) {
    if (top <= 0) return;
    const bool ax = vy && vy->ok;
    const int px = pairPx(g.bottom - g.top, g.reserve), yb = ax ? std::min(g.bottom - 1, vy->yOf(0)) : g.bottom - 1;
    struct Lb { int cx = 0, hb = 0; long long mag = 0; std::string a, b; bool view = true; };
    std::vector<Lb> labs;
    for (int i = std::max(0, v0 - labelLead(slot)); i <= v1; ++i) {
        const BarLook b = look(i); if (!b.ok) continue;
        const int xc = xOf(i); if (xc > g.right) continue;
        const bool view = i >= v0 && xc >= g.left;
        const Pair p = pairIn(xc, slot);
        for (int pos = 0; pos < 2; ++pos) {
            const int s = pos == 0 ? left : 1 - left; const long long v = b.v[s];
            if (v == 0) continue;
            const int l = p.l[pos], r = p.r[pos];
            if (v < 0) { if (view && yb > g.top + g.reserve) fillFlat(x, l, yb - 1, r, yb, normal[s]); continue; }   // more pulled than added: a stub, never below zero
            int h = 0; bool cap = false;
            if (ax) { int yt = 0, ybb = 0; h = axisBar(*vy, g.top + g.reserve + 1, g.bottom - 1, v, yt, ybb, cap); }
            else { h = barHeight(v, top, px); cap = v > top; }
            if (h <= 0) continue;
            const bool hi = hiOf(b, s);
            if (view) {
                fillFlat(x, l, yb - h + 1, r, yb, hi ? spike[s] : normal[s]);
                if (cap && h > CAP_TIP_PX) fillFlat(x, l, yb - h + 1, r, yb - h + CAP_TIP_PX, C_WHITE);   // capped: the white tip
            }
            if (!hi || !PAIR_LABELS) continue;
            Lb q; q.cx = (l + r) / 2; q.hb = h; q.mag = v; q.view = view;
            q.a = b.ratioOk[s] ? noX(ratioText(b.ratio[s])) : std::string(); q.b = b.p100[s] > 0 ? noX(paceX(b.p100[s])) : std::string();
            if (!b.lab1[s].empty() || !b.lab2[s].empty()) { q.a = b.lab1[s]; q.b = b.lab2[s]; }   // ready-made rows (Book Flow)
            if (!q.a.empty() || !q.b.empty()) labs.push_back(q);
        }
    }
    if (labs.empty()) return;
    std::stable_sort(labs.begin(), labs.end(), [](const Lb& a, const Lb& b) { return a.mag > b.mag; });
    cppExtension::FONT sf; sf.id = cppExtension::HELVETICA; sf.size = LABEL_PT; sf.style = cppExtension::PLAIN; x->setFont(sf);
    std::vector<LBox> placed(avoid);
    for (size_t k = 0; k < labs.size(); ++k) {
        const Lb& c = labs[k];
        std::string t[2]; COLOR tc[2] = { C_LBL_RATIO, C_LBL_PACE }; int nr = 0;
        if (!c.a.empty()) { t[nr] = c.a; tc[nr++] = C_LBL_RATIO; }
        if (!c.b.empty()) { t[nr] = c.b; tc[nr++] = C_LBL_PACE; }
        const int t0 = yb - c.hb - 2 - nr * LABEL_LH, b0 = t0 + nr * LABEL_LH;
        if (t0 < g.top + g.reserve) continue;
        int w = 0; for (int r = 0; r < nr; ++r) w = std::max(w, (int)x->getTextWidth(t[r].c_str(), -1));
        const LBox me = { c.cx - w / 2 - 1, t0, c.cx - w / 2 + w + 1, b0 - 1, 0, 0 };
        if (me.l < g.left + 1 || me.r > g.edge - 1) continue;
        bool hit = false; for (size_t o = 0; o < placed.size() && !hit; ++o) hit = overlaps(me, placed[o]);
        if (hit) continue;
        placed.push_back(me);
        if (!c.view) continue;
        for (int r = 0; r < nr; ++r) {
            x->setTextColor(tc[r]); const int tw = (int)x->getTextWidth(t[r].c_str(), -1);
            cppExtension::RCT rc; rc.set(coord(c.cx - tw / 2), coord(t0 + r * LABEL_LH), coord(c.cx - tw / 2 + tw + 2), coord(t0 + r * LABEL_LH + LABEL_LH)); rc.drawText(t[r].c_str(), false, false);
        }
    }
}
// (1.18.0) the pane's question, yellow bold, centred at the top (moved right of the box if they would meet)
// (Effort Reward 1.22.0 / Defense 1.10.0, Rassul 2026-10-19 23:23 "move it more to the left so its aligned with the info box right side",
// "an info icon .. when i hover over it this description with a little more detail on how its calculated as well as an example";
// info-icon-mockup.png, "build" 23:29) the question starts just right of the info box (left of the pane when the box is not beside it),
// then a small grey info icon ("i" in a dot); its rectangle goes back in `icon` (pane pixels) for the mouse hover. Returns false = nothing drawn.
struct IconRect { int l = 0, t = 0, r = -1, b = -1; bool has(int h, int v) const { return r > l && h >= l - 2 && h <= r + 2 && v >= t - 2 && v <= b + 2; } };
static const int INFO_ICON_PX = 12;
static const COLOR C_INFO_ICON = 0x009CA3AF;                               // light grey dot, black "i"
// (2026-10-21, Rassul 11:30 "move the question and its info icon inside the info box, to the upper left") the question as the box's
// FIRST line (Info::topLines = 1 keeps it free), upper left inside the box, the info icon right of it; questionW = its width for Info::topW
inline int questionW(const TextW& M, const std::string& q) { return M(q, 10, true) + INFO_ICON_PX + 8; }
inline bool drawQuestionIn(cppExtension* x, const bp::Box& box, const std::string& q, IconRect* icon) {
    if (icon) *icon = IconRect();
    if (q.empty() || box.r <= box.l) return false;
    cppExtension::FONT f; f.id = cppExtension::HELVETICA; f.size = 10; f.style = cppExtension::BOLD; x->setFont(f);
    const int w = (int)x->getTextWidth(q.c_str(), -1), l = box.l + BOX_PAD, t = box.t + BOX_PAD - 2;
    if (l + w + INFO_ICON_PX + 8 > box.r - 2) return false;
    x->setTextColor(C_LBL_RATIO);
    cppExtension::RCT rc; rc.set(coord(l), coord(t), coord(l + w + 2), coord(t + QUESTION_H)); rc.drawText(q.c_str(), false, false);
    if (icon) {
        const int il = l + w + 8, it = t + (QUESTION_H - INFO_ICON_PX) / 2;
        x->setPen(C_INFO_ICON, 1, P_SOLID); cppExtension::CBRUSH br(C_INFO_ICON, PAT_SOLID); br.set();
        cppExtension::RCT d; d.set(coord(il), coord(it), coord(il + INFO_ICON_PX), coord(it + INFO_ICON_PX)); d.drawOval(cppExtension::DRAW_OPAQUE);
        cppExtension::FONT fi; fi.id = cppExtension::HELVETICA; fi.size = 9; fi.style = cppExtension::BOLD; x->setFont(fi);
        x->setTextColor(0x00000000);
        const int iw = (int)x->getTextWidth("i", -1);
        cppExtension::RCT ti; ti.set(coord(il + (INFO_ICON_PX - iw) / 2), coord(it - 1), coord(il + INFO_ICON_PX), coord(it + INFO_ICON_PX)); ti.drawText("i", false, false);
        icon->l = il; icon->t = it; icon->r = il + INFO_ICON_PX; icon->b = it + INFO_ICON_PX;
    }
    return true;
}
inline bool drawQuestion(cppExtension* x, const PaneGeo& g, const std::string& q, const bp::Box* box, IconRect* icon = nullptr) {
    if (icon) *icon = IconRect();
    if (q.empty() || g.edge - g.left < 120) return false;
    cppExtension::FONT f; f.id = cppExtension::HELVETICA; f.size = 10; f.style = cppExtension::BOLD; x->setFont(f);
    const int w = (int)x->getTextWidth(q.c_str(), -1);
    const int t = g.top + 1;
    const bool beside = box && box->r > box->l && t + QUESTION_H > box->t && t < box->b;
    int l = beside ? box->r + 10 : g.left + 4;
    const int need = w + (icon ? INFO_ICON_PX + 8 : 0);
    if (l + need > g.edge - 2) return false;                               // no room beside the box: leave it out (never over the box)
    x->setTextColor(C_LBL_RATIO);                                         // yellow, as the ratio labels
    cppExtension::RCT rc; rc.set(coord(l), coord(t), coord(l + w + 2), coord(t + QUESTION_H)); rc.drawText(q.c_str(), false, false);
    if (icon) {
        const int il = l + w + 8, it = t + (QUESTION_H - INFO_ICON_PX) / 2;
        x->setPen(C_INFO_ICON, 1, P_SOLID); cppExtension::CBRUSH br(C_INFO_ICON, PAT_SOLID); br.set();
        cppExtension::RCT d; d.set(coord(il), coord(it), coord(il + INFO_ICON_PX), coord(it + INFO_ICON_PX)); d.drawOval(cppExtension::DRAW_OPAQUE);
        cppExtension::FONT fi; fi.id = cppExtension::HELVETICA; fi.size = 9; fi.style = cppExtension::BOLD; x->setFont(fi);
        x->setTextColor(0x00000000);
        const int iw = (int)x->getTextWidth("i", -1);
        cppExtension::RCT ti; ti.set(coord(il + (INFO_ICON_PX - iw) / 2), coord(it - 1), coord(il + INFO_ICON_PX), coord(it + INFO_ICON_PX)); ti.drawText("i", false, false);
        icon->l = il; icon->t = it; icon->r = il + INFO_ICON_PX; icon->b = it + INFO_ICON_PX;
    }
    return true;
}
// the info icon's hover: true while the mouse is on the icon. Call from mouse() with the event's pane position; true = it changed (redraw).
// (1.23.0 / 1.11.0, Rassul 2026-10-20 07:44 "it shows ... one long line, instead of a multiline box") IRT's help tip shows one line,
// so the description is DRAWN as a box in the pane (drawInfoTip), last, over everything.
inline bool infoTip(const IconRect& r, bool& on, int h, int v, const char* /*text*/) {
    const bool in = r.has(h, v);
    if (in == on) return false;
    on = in; return true;
}
// the tip box: the text's lines (split at '\n'), the first in bold; as many columns as the pane height needs (a short pane gets a wide
// box, never cut); it starts at the icon and moves left to stay inside the pane; dark grey with light text, a grey border.
struct TipCol { size_t from = 0, to = 0; int w = 0; };
inline std::vector<TipCol> tipColumns(const std::vector<std::string>& lines, const std::vector<int>& widths, int rows) {
    std::vector<TipCol> cols; if (rows < 1) rows = 1;
    size_t i = 0;
    while (i < lines.size()) {
        while (i < lines.size() && lines[i].empty() && !cols.empty()) ++i;   // no blank line at the top of a column
        if (i >= lines.size()) break;
        TipCol c; c.from = i;
        size_t end = std::min(lines.size(), i + (size_t)rows);
        if (end < lines.size()) {                                          // break at a blank line when one is in the lower half
            for (size_t k = end; k > i + (size_t)rows / 2; --k) if (lines[k - 1].empty()) { end = k; break; }
        }
        c.to = end; for (size_t k = c.from; k < c.to; ++k) c.w = std::max(c.w, widths[k]);
        cols.push_back(c); i = end;
    }
    return cols;
}
static const COLOR C_TIP_BG = 0x00111827, C_TIP_BORDER = 0x009CA3AF, C_TIP_TEXT = 0x00E5E7EB;
inline void drawInfoTip(cppExtension* x, const PaneGeo& g, const IconRect& icon, const char* text) {
    if (!text || icon.r <= icon.l) return;
    std::vector<std::string> lines; { std::string cur; for (const char* p = text; *p; ++p) { if (*p == '\n') { lines.push_back(cur); cur.clear(); } else cur += *p; } lines.push_back(cur); }
    cppExtension::FONT fb; fb.id = cppExtension::HELVETICA; fb.size = 9; fb.style = cppExtension::BOLD;
    cppExtension::FONT fp = fb; fp.style = cppExtension::PLAIN;
    std::vector<int> w(lines.size(), 0);
    for (size_t k = 0; k < lines.size(); ++k) { x->setFont(k == 0 ? fb : fp); w[k] = lines[k].empty() ? 0 : (int)x->getTextWidth(lines[k].c_str(), -1); }
    const int lh = 13, pad = 6, gap = 18;
    const int top = icon.b + 3, avail = g.bottom - 2 - top;
    const int rows = std::max(1, (avail - 2 * pad) / lh);
    const std::vector<TipCol> cols = tipColumns(lines, w, rows);
    int boxW = 2 * pad, used = 0;
    for (size_t c = 0; c < cols.size(); ++c) { boxW += cols[c].w + (c ? gap : 0); used = std::max(used, (int)(cols[c].to - cols[c].from)); }
    const int boxH = 2 * pad + used * lh;
    int l = icon.l; if (l + boxW > g.right - 2) l = std::max(g.left + 2, g.right - 2 - boxW);
    const int r = std::min(g.right - 2, l + boxW), b = std::min(g.bottom - 2, top + boxH);
    x->setPen(C_TIP_BORDER, 1, P_SOLID);
    fillRect(l, top, r, b, C_TIP_BORDER, C_TIP_BG, 1);
    x->setTextColor(C_TIP_TEXT);
    int cx = l + pad;
    for (size_t c = 0; c < cols.size(); ++c) {
        int y = top + pad;
        for (size_t k = cols[c].from; k < cols[c].to; ++k, y += lh) {
            if (lines[k].empty()) continue;
            x->setFont(k == 0 ? fb : fp);
            cppExtension::RCT t; t.set(coord(cx), coord(y), coord(std::min(r - 2, cx + cols[c].w + 4)), coord(y + lh)); t.drawText(lines[k].c_str(), false, false);
        }
        cx += cols[c].w + gap;
    }
}
// ---- (Aggression 1.16.0, Rassul 2026-10-19 17:04 "i want it to show both bid and ask and i want the selling side to be on the bottom";
// mockup bidask-mirror-mockup.png, "build it already" 17:42) MIRROR BARS: every candle shows BOTH sides - buyers (side 1, offers lifted)
// green ABOVE the grey zero line, sellers (side 0, bids hit) red BELOW it, each the full slot wide, one scale both ways. The zero line sits
// where the data needs it (netGeoFor with the biggest buy / sell on screen). Bright = 2.0x+ (hiOf); its RATIO (yellow) and PACE (grey)
// sit just outside the bar (above a buy bar, below a sell bar); biggest first, a label that would sit on another is skipped, never moved.
struct MirrorGeo { NetGeo n; int y0 = 0; };
inline MirrorGeo mirrorGeo(const PaneGeo& g, long long up, long long down) { MirrorGeo m; m.n = netGeoFor(g.bottom - g.top, g.reserve, up, down); m.y0 = g.top + m.n.y0; return m; }
inline int mirrorH(long long v, const NetGeo& n, bool upSide) { return upSide ? barHeight(v, n.upTop, n.maxPx) : barHeight(v, n.downTop, n.downPx); }
inline void drawMirrorBars(cppExtension* x, const PaneGeo& g, int v0, int v1, int slot, long long up, long long down, const COLOR normal[2], const COLOR spike[2],
                           const std::function<int(int)>& xOf, const std::function<BarLook(int)>& look, const std::vector<LBox>& avoid) {
    if (up <= 0 && down <= 0) return;
    const MirrorGeo M = mirrorGeo(g, up, down); const NetGeo& n = M.n; const int y0 = M.y0;
    if (g.edge > g.left + 4) fillFlat(x, g.left, y0, g.edge, y0 + 1, (COLOR)0x00A0A0A0);   // the grey zero line (C_NET_ZERO)
    struct Lb { int cx = 0, hb = 0; bool upSide = true; long long mag = 0; std::string a, b; bool view = true; };
    std::vector<Lb> labs;
    for (int i = std::max(0, v0 - labelLead(slot)); i <= v1; ++i) {
        const BarLook b = look(i); if (!b.ok) continue;
        const int xc = xOf(i); if (xc > g.right) continue;
        const bool view = i >= v0 && xc >= g.left;
        int l = 0, r = 0; oneBarSpan(xc, slot, l, r);
        for (int s = 0; s < 2; ++s) {
            const bool upSide = s == 1; const long long v = b.v[s]; if (v <= 0) continue;
            const int h = mirrorH(v, n, upSide); if (h <= 0) continue;
            const bool hi = hiOf(b, s);
            if (view) { if (upSide) fillFlat(x, l, y0 - h, r, y0 - 1, hi ? spike[s] : normal[s]); else fillFlat(x, l, y0 + 2, r, y0 + 1 + h, hi ? spike[s] : normal[s]); }
            if (view && v > (upSide ? n.upTop : n.downTop) && h > CAP_TIP_PX) {   // a capped bar: the white tip at its outer end
                if (upSide) fillFlat(x, l, y0 - h, r, y0 - h + CAP_TIP_PX - 1, C_WHITE); else fillFlat(x, l, y0 + 2 + h - CAP_TIP_PX, r, y0 + 1 + h, C_WHITE);
            }
            if (!hi) continue;
            Lb q; q.cx = (l + r) / 2; q.hb = h; q.upSide = upSide; q.mag = v; q.view = view;
            q.a = b.ratioOk[s] ? noX(ratioText(b.ratio[s])) : std::string(); q.b = b.p100[s] > 0 ? noX(paceX(b.p100[s])) : std::string();
            if (!q.a.empty() || !q.b.empty()) labs.push_back(q);
        }
    }
    if (labs.empty()) return;
    std::stable_sort(labs.begin(), labs.end(), [](const Lb& a, const Lb& b) { return a.mag > b.mag; });
    cppExtension::FONT sf; sf.id = cppExtension::HELVETICA; sf.size = LABEL_PT; sf.style = cppExtension::PLAIN; x->setFont(sf);
    std::vector<LBox> placed(avoid);
    for (size_t k = 0; k < labs.size(); ++k) {
        const Lb& c = labs[k];
        std::string t[2]; COLOR tc[2] = { C_LBL_RATIO, C_LBL_PACE }; int nr = 0;
        if (!c.a.empty()) { t[nr] = c.a; tc[nr++] = C_LBL_RATIO; }
        if (!c.b.empty()) { t[nr] = c.b; tc[nr++] = C_LBL_PACE; }
        const int t0 = c.upSide ? y0 - c.hb - 2 - nr * LABEL_LH : y0 + 2 + c.hb + 2, b0 = t0 + nr * LABEL_LH;
        if (t0 < g.top + g.reserve || b0 > g.bottom) continue;
        int w = 0; for (int r = 0; r < nr; ++r) w = std::max(w, (int)x->getTextWidth(t[r].c_str(), -1));
        const LBox me = { c.cx - w / 2 - 1, t0, c.cx - w / 2 + w + 1, b0 - 1, 0, 0 };
        if (me.l < g.left + 1 || me.r > g.edge - 1) continue;
        bool hit = false; for (size_t o = 0; o < placed.size() && !hit; ++o) hit = overlaps(me, placed[o]);
        if (hit) continue;
        placed.push_back(me);
        if (!c.view) continue;
        for (int r = 0; r < nr; ++r) {
            x->setTextColor(tc[r]); const int tw = (int)x->getTextWidth(t[r].c_str(), -1);
            cppExtension::RCT rc; rc.set(coord(c.cx - tw / 2), coord(t0 + r * LABEL_LH), coord(c.cx - tw / 2 + tw + 2), coord(t0 + r * LABEL_LH + LABEL_LH)); rc.drawText(t[r].c_str(), false, false);
        }
    }
}
// ---- (1.7.0, BookFlow 1.0.0; Rassul 2026-10-16 07:49 "1 net bar. make the zero line gray so i can clearly see the separation") NET BARS ----
// One bar per side per candle = lots ADDED minus lots PULLED near price: above the zero line = size arriving (stacking), below = size leaving
// (pulling). Bids green (left), offers red (right) - the buyer / seller colours never change. The zero line is grey and 2 px.
static const COLOR C_NET_ZERO = 0x00A0A0A0;
inline void drawNetBars(cppExtension* x, const PaneGeo& g, int v0, int v1, int slot, long long top, const COLOR normal[2], const COLOR spike[2],
                        const std::function<int(int)>& xOf, const std::function<BarLook(int)>& look, const std::vector<LBox>& avoid) {
    if (top <= 0) return;
    const NetGeo n = netGeoFor(g.bottom - g.top, g.reserve);
    const int y0 = g.top + n.y0;
    if (g.edge > g.left + 4) fillFlat(x, g.left, y0, g.edge, y0 + 1, C_NET_ZERO);        // the grey zero line, drawn first; bars sit clear of it
    struct Cand { int cx = 0, s = 0, hb = 0; bool up = true; long long mag = 0; std::string a, b, c; };
    std::vector<Cand> cands;
    for (int i = v0; i <= v1; ++i) {
        const BarLook b = look(i); if (!b.ok) continue;
        const int xc = xOf(i); if (xc < g.left || xc > g.right) continue;
        const Pair p = pairIn(xc, slot);
        for (int s = 0; s < 2; ++s) {
            const long long v = b.v[s]; if (v == 0) continue;
            const bool up = v > 0; const long long mg = up ? v : -v;
            const int h = barHeight(mg, top, n.maxPx);
            const int ci = ((s == BID) == up) ? 0 : 1;                              // (1.9.0, Rassul 2026-10-16 18:16 "support arriving (green), support leaving (red)"): by what it means for price - bids arriving / offers leaving = green; bids leaving / offers arriving = red
            const COLOR c = b.spike[s] ? spike[ci] : normal[ci];
            if (up) fillFlat(x, p.l[s], y0 - h, p.r[s], y0 - 1, c); else fillFlat(x, p.l[s], y0 + 2, p.r[s], y0 + 1 + h, c);
            if (b.spike[s]) {
                Cand k; k.cx = (p.l[s] + p.r[s]) / 2; k.s = s; k.hb = h; k.up = up; k.mag = mg;
                k.a = b.ratioOk[s] ? ratioText(b.ratio[s]) : std::string(); k.b = paceLabel(b.p100[s]); k.c = b.q[s] > 0 ? std::to_string(b.q[s]) : std::string();
                cands.push_back(k);
            }
        }
    }
    if (cands.empty()) return;
    std::stable_sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.mag > b.mag; });   // the biggest spikes get their label first
    cppExtension::FONT sf; sf.id = cppExtension::HELVETICA; sf.size = LABEL_PT; sf.style = cppExtension::PLAIN; x->setFont(sf);
    std::vector<LBox> placed(avoid);
    for (size_t k = 0; k < cands.size(); ++k) {
        const Cand& c = cands[k];
        // (1.10.0, Rassul 2026-10-16 22:00) like Aggression: the RATIO (yellow) on top and the PACE (grey, added size only) under it, both outside the bar's far end; no lots
        const std::string rx = noX(c.a), pc = c.up ? noX(c.b) : std::string();
        std::string lr[2]; COLOR lc[2] = { C_LBL_RATIO, C_LBL_PACE }; int nR = 0;
        if (!rx.empty()) { lr[nR] = rx; lc[nR++] = C_LBL_RATIO; }
        if (!pc.empty()) { lr[nR] = pc; lc[nR++] = C_LBL_PACE; }
        if (nR == 0) continue;
        const int yEdge = c.up ? y0 - c.hb : y0 + 1 + c.hb;
        const int t0 = c.up ? yEdge - 2 - nR * LABEL_LH : yEdge + 2, b0 = t0 + nR * LABEL_LH;
        if (t0 < g.top + g.reserve || b0 > g.bottom) continue;                          // no room (zoomed / short pane): the bar is still drawn
        int w = 0; for (int r = 0; r < nR; ++r) w = std::max(w, (int)x->getTextWidth(lr[r].c_str(), -1));
        const LBox me = { c.cx - w / 2 - 1, t0, c.cx - w / 2 + w + 1, b0 - 1, 0, 0 };
        if (me.l < g.left + 1 || me.r > g.edge - 1) continue;
        bool hit = false; for (size_t o = 0; o < placed.size() && !hit; ++o) hit = overlaps(me, placed[o]);
        if (hit) continue;
        placed.push_back(me);
        auto put = [&](const std::string& s, int y, COLOR col) {
            if (s.empty()) return; x->setTextColor(col); const int tw = (int)x->getTextWidth(s.c_str(), -1);
            cppExtension::RCT rc; rc.set(coord(c.cx - tw / 2), coord(y), coord(c.cx - tw / 2 + tw + 2), coord(y + LABEL_LH)); rc.drawText(s.c_str(), false, false); };
        for (int r = 0; r < nR; ++r) put(lr[r], t0 + r * LABEL_LH, lc[r]);
    }
}
// ---- (12:05) THE INFO BOX - Rassul "all values go in the info box, which I should be able to see as I move my mouse over different
// bars": solid border, black, the hovered candle's time (12-hour CT) or "Live" centred on top, white labels, values in the side's colour,
// FIXED size (measured on a template value, values cut to fit), 9-spot Position (default Top left), never past the last bar.
// Two-column boxes (PullStats / StackStats): a column header row (Bids / Offers), then label | left value | right value.
static const COLOR C_BOX_BORDER = 0x009CA3AF;
// (1.2.0) the BLUE options stat line (SPEC: only when the data is present and fresh); (1.3.0) drawn by drawPanel
static const COLOR C_OPT_BLUE = 0x0058A6FF;
// ---- (1.3.0) THE PANEL: draws a PanelPlan (planPanel) with the host's font - the read box (+ its hover grid) or the numbers box + read
inline TextW measureWith(cppExtension* x) {
    return [x](const std::string& t, int pt, bool bold) {
        if (t.empty()) return 0;
        cppExtension::FONT f; f.id = cppExtension::HELVETICA; f.size = (short)pt; f.style = bold ? cppExtension::BOLD : cppExtension::PLAIN; x->setFont(f);
        return (int)x->getTextWidth(t.c_str(), -1);
    };
}
inline void textAt(cppExtension* x, int l, int t, int h, const std::string& str, COLOR c, int pt, bool bold) {
    if (str.empty()) return;
    cppExtension::FONT f; f.id = cppExtension::HELVETICA; f.size = (short)pt; f.style = bold ? cppExtension::BOLD : cppExtension::PLAIN; x->setFont(f);
    x->setTextColor(c); const int w = (int)x->getTextWidth(str.c_str(), -1);
    cppExtension::RCT r; r.set(coord(l), coord(t), coord(l + w + 2), coord(t + h)); r.drawText(str.c_str(), false, false);
}
inline void boxAt(cppExtension* x, const bp::Box& b) { x->setPen(C_BOX_BORDER, 1, P_SOLID); fillRect(b.l, b.t, b.r, b.b, C_BOX_BORDER, C_BLACKC, 1); }
// the numbers (label | left | right) of rows [from, from + n) in a column group starting at x0; values cut to the column, never wider
inline void numbersAt(cppExtension* x, int x0, int y0, int lw, int vw, const Info& I, size_t from, size_t n, const COLOR col[2], bool head) {
    const TextW M = measureWith(x);
    const int rc[2] = { x0 + lw + 8 + vw, x0 + lw + 8 + vw + 8 + vw };
    int y = y0;
    if (head && I.twoCol) { for (int s = 0; s < 2; ++s) textAt(x, rc[s] - M(I.colHead[s], NUM_PT, true), y, NUM_LH - 1, I.colHead[s], col[s], NUM_PT, true); y += NUM_LH; }
    for (size_t k = from; k < from + n && k < I.rows.size(); ++k, y += NUM_LH) {
        textAt(x, x0, y, NUM_LH - 1, I.rows[k].label, C_WHITE, NUM_PT, true);
        for (int s = 0; s < (I.twoCol ? 2 : 1); ++s) {
            const std::string v = nl::fitText(I.rows[k].v[s], vw, [&](const std::string& u) { return M(u, NUM_PT, true); }); if (v.empty()) continue;
            const int sd = I.rows[k].side[s];
            textAt(x, (I.twoCol ? rc[s] : x0 + lw + 10 + vw) - M(v, NUM_PT, true), y, NUM_LH - 1, v, sd < 0 ? C_WHITE : col[sd], NUM_PT, true);
        }
    }
}
inline void drawPanel(cppExtension* x, const PanelPlan& P, const Info& I, const COLOR col[2], int impl, bool hover) {
    if (!P.ok) return;
    if (P.mode == MODE_READ) {
        if (hover) {                                                       // the numbers: every row, in column groups
            const bp::Box& b = P.gridBox; boxAt(x, b);
            textAt(x, b.l + BOX_PAD, b.t + BOX_PAD - 1, NUM_LH - 1, I.head, C_WHITE, NUM_PT, true);
            for (size_t k = 0; k < P.grid.g.size(); ++k) {
                const GridGeom::G& g = P.grid.g[k]; if (b.l + g.x + g.lw > b.r) break;
                numbersAt(x, b.l + g.x, b.t + BOX_PAD - 1, g.lw, g.vw, I, g.from, g.n, col, true);
            }
            return;
        }
        boxAt(x, P.box);
        int y = P.box.t + BOX_PAD - 1;
        for (size_t k = 0; k < P.read.size(); ++k, y += READ_LH) textAt(x, P.box.l + 6, y, READ_LH, P.read[k], implColor(impl), READ_PT, true);
        for (size_t k = 0; k < P.opt.size(); ++k, y += OPT_LH) textAt(x, P.box.l + 6, y, OPT_LH, P.opt[k], C_OPT_BLUE, OPT_PT, false);
        return;
    }
    boxAt(x, P.box);                                                       // numbers mode: the compact box + the read beside it
    const int top = NUM_LH * I.topLines;                                    // (2026-10-21) the question line(s) first, drawn by the pane
    { const TextW M = measureWith(x); const int w = M(I.head, NUM_PT, true); textAt(x, P.box.l + (P.box.w() - w) / 2, P.box.t + BOX_PAD - 1 + top, NUM_LH - 1, I.head, C_WHITE, NUM_PT, true); }
    numbersAt(x, P.box.l + BOX_PAD, P.box.t + BOX_PAD - 1 + NUM_LH + top, P.num.lw, P.num.vw, I, 0, (size_t)P.num.rows, col, true);
    size_t r = 0;
    for (size_t k = 0; k < P.read.size(); ++k, ++r) textAt(x, P.textRects[r].l, P.textRects[r].t, READ_LH, P.read[k], implColor(impl), READ_PT, true);
    for (size_t k = 0; k < P.opt.size() && r < P.textRects.size(); ++k, ++r) textAt(x, P.textRects[r].l, P.textRects[r].t, OPT_LH, P.opt[k], C_OPT_BLUE, OPT_PT, false);
}
inline bool inBox(const bp::Box& b, int h, int v) { return b.w() > 0 && h >= b.l && h <= b.r && v >= b.t && v <= b.b; }
// (1.5.0) the read line (no box, no border) and, with the mouse on it, its detail box (solid grey border, black, white text)
inline void drawLine(cppExtension* x, const LinePlan& P) {
    if (!P.ok || !P.show) return;
    if (!P.pre.empty()) textAt(x, P.box.l + 4, P.box.t + 1, READ_LH, P.pre, C_NUMS, READ_PT, true);   // (1.6.1) the pane's name + question, grey
    if (!P.text.empty()) textAt(x, P.box.l + 4 + P.preW, P.box.t + 1, READ_LH, P.text, implColor(P.impl), READ_PT, true);
    if (P.hover.empty() || P.hoverBox.w() <= 0) return;
    boxAt(x, P.hoverBox);
    int y = P.hoverBox.t + BOX_PAD - 1;
    for (size_t k = 0; k < P.hover.size(); ++k, y += HOVER_LH) textAt(x, P.hoverBox.l + BOX_PAD, y, HOVER_LH, P.hover[k], k == 0 ? implColor(P.impl) : C_WHITE, HOVER_PT, k == 0);
}

// ---- (1.7.0, Rassul 2026-10-15 13:25 "when i hover over a indicator value for pulling stacking etc, i should get the answer for the
// node i am hovering over", mockup-pane-bar-hover-v1.png) the BAR HOVER: the hovered candle's answer in a small box beside it, in
// every order-flow pane (the chart-wide hover moves all three together). Line 1 = time + the pane's question; then the buyers' line
// (green) and the sellers' line (red), the side that helps buyers first; then the extra rows that have something (Sweep, Large ...).
// The numbers are the info box's own (same source as the Numbers mode and the record). Pure geometry: planned here, drawn by drawBarHover.
struct BarHov { bool ok = false; bp::Box box; std::vector<std::string> rows; std::vector<COLOR> cols; std::vector<bool> bold; };
inline bool hovUseful(const std::string& v) { return !v.empty() && v != "-" && v != "n/a" && v != "no" && v != "0"; }
inline BarHov planBarHover(const bp::Box& area, int barX, const Info& I, const std::string& question, const char* const verb[2], int bull,
                           const TextW& M) {
    BarHov H; if (bull != 0 && bull != 1) bull = 1;
    if (I.rows.empty() || area.w() < 120 || area.h() < HOVER_LH * 2) return H;
    H.rows.push_back(I.head + " \xB7 " + question); H.cols.push_back(C_WHITE); H.bold.push_back(true);
    std::string pace[2];
    for (size_t k = 1; k < I.rows.size(); ++k) if (I.rows[k].label == "Pace") { pace[0] = I.rows[k].v[0]; pace[1] = I.rows[k].v[1]; }
    const int order[2] = { bull, 1 - bull };
    for (int o = 0; o < 2; ++o) {
        const int sd = order[o];
        std::string t = std::string(verb[sd]) + " " + I.rows[0].v[sd];
        if (hovUseful(pace[sd])) t += " \xB7 pace " + pace[sd];
        H.rows.push_back(t); H.cols.push_back(sd == bull ? (COLOR)0x0022C55E : (COLOR)0x00EF4444); H.bold.push_back(false);
    }
    std::string extra;
    for (size_t k = 1; k < I.rows.size(); ++k) {
        const InfoRow& r = I.rows[k];
        if (r.label == "Pace" || r.label == "Crowd") continue;
        for (int o = 0; o < 2; ++o) {
            const int sd = order[o];
            if (!hovUseful(r.v[sd])) continue;
            extra += (extra.empty() ? std::string() : std::string(" \xB7 ")) + r.label + ": " + I.colHead[sd] + " " + r.v[sd];
        }
    }
    if (!extra.empty()) { H.rows.push_back(extra); H.cols.push_back(C_NUMS); H.bold.push_back(false); }
    const int maxRows = std::max(2, (area.h() - 2 * BOX_PAD) / HOVER_LH);
    if ((int)H.rows.size() > maxRows) { H.rows.resize((size_t)maxRows); H.cols.resize((size_t)maxRows); H.bold.resize((size_t)maxRows); }
    int w = 0; for (size_t k = 0; k < H.rows.size(); ++k) w = std::max(w, M(H.rows[k], HOVER_PT, H.bold[k]));
    w = std::min(w + 2 * BOX_PAD + 2, area.w());
    const int h = (int)H.rows.size() * HOVER_LH + 2 * BOX_PAD;
    int l = barX + 10; if (l + w > area.r) l = barX - 10 - w;              // right of the candle, else left of it
    if (l < area.l) l = area.l;
    H.box = bp::clampIn(bp::mk(l, area.t, l + w, area.t + h), area);
    H.ok = true; return H;
}
inline void drawBarHover(cppExtension* x, const BarHov& H) {
    if (!H.ok) return;
    boxAt(x, H.box);
    int y = H.box.t + BOX_PAD - 1;
    for (size_t k = 0; k < H.rows.size(); ++k, y += HOVER_LH) textAt(x, H.box.l + BOX_PAD, y, HOVER_LH, H.rows[k], H.cols[k], HOVER_PT, H.bold[k]);
}

#ifdef OFF_TEST_CLOCK
extern long long off_test_now_ms;
extern void* off_test_cursor_wnd;
inline long long steadyMs() { return off_test_now_ms; }
inline void* cursorWindow() { return off_test_cursor_wnd; }
#else
inline long long steadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
#if defined(_WIN32)
#pragma comment(lib, "user32.lib")   // (2026-10-13) GetCursorPos / WindowFromPoint - link fix
inline void* cursorWindow() { POINT p; if (!GetCursorPos(&p)) return NULL; return (void*)WindowFromPoint(p); }
#else
inline void* cursorWindow() { return NULL; }
#endif
#endif

// ---- (1.5.0) the zone read, once per pump, for any pane host: levels / normals (files, bounded, re-read on change), the lagged ADR
// and the closes (the chart's bars, on a new bar only), this session's record (read back on load), the price when no fill came for
// 5 s (the chart's close), then the clock (12-hour CT minute -> the normal's 30-min slot; wall seconds -> the record)
inline long long zoneClockSec(cppExtension* e, long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !e->getLocaltime((RTDATE)t, &x)) return LLONG_MIN; return civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec; }
inline void zoneTickFor(cppExtension* x, zread::Host& zr, long& zrCount, long long& zrSession, int pane, double tick, const std::string& market, const nl::Zone& zone,
                        double atr, const std::string& recPath, long count, long long now) {
    zr.pane = pane; zr.e.tick = tick;
    const std::string fd = flexDir();
    zr.files(fd.empty() ? fd : fd + "\\", market, now, (long long)std::time(NULL));
    if (count != zrCount && count >= 2) {
        cppExtension::RTARRAYI times(barDateTime); cppExtension::RTARRAY hi(barHigh), lo(barLow), cl(barClose);
        if (times.count >= count && hi.count >= count && lo.count >= count && cl.count >= count) {
            zrCount = count;
            std::vector<zread::BarRow> rows; rows.reserve((size_t)std::min(count, 12000L));
            for (long i = std::max(0L, count - 12000L); i < count - 1; ++i) {
                const long long cs = zoneClockSec(x, (long long)times[(int)i]); if (cs == LLONG_MIN || cs <= 0) continue;
                zread::BarRow r; r.session = zrule::sessionKey(cs); r.endSec = cs; r.h = (double)hi[(int)i]; r.l = (double)lo[(int)i]; r.c = (double)cl[(int)i]; rows.push_back(r);
            }
            const long long cs = zoneClockSec(x, (long long)times[(int)count - 1]);
            if (cs != LLONG_MIN && cs > 0) { zrSession = zrule::sessionKey(cs); zr.bars(rows, zrSession, nl::halfWidth(zone, atr, tick)); }
        }
    }
    long long wall = zoneClockSec(x, (long long)x->currentDate()); if (wall == LLONG_MIN) wall = 0;
    int minute = -1; { struct tm t; std::memset(&t, 0, sizeof(t)); if (x->getLocaltime(x->currentDate(), &t)) minute = t.tm_hour * 60 + t.tm_min; }
    if (zrSession >= 0 && !recPath.empty()) zr.newSession(zrSession, recPath, now, wall);
    if (zr.e.lastTradeMs < 0 || now - zr.e.lastTradeMs > 5000 || now < zr.e.lastTradeMs) {
        cppExtension::RTARRAY cl(barClose); Tick px = 0;
        if (cl.count >= count && count > 0 && toTick((double)cl[(int)count - 1], tick, px)) zr.e.onPrice(now, px);
    }
    zr.e.step(now, wall, minute);
    zr.flush();
}
// ---- the windows the chart's mouse events came from (hover off the chart -> Live), as lsBidAskStats 1.1.0 ----------------------
struct HoverTrack {
    Hover hover; std::vector<void*> wnds; long long redraws = 0;
    void seen() {
        void* w = cursorWindow(); if (!w) return;
        std::vector<void*>::iterator it = std::find(wnds.begin(), wnds.end(), w);
        if (it != wnds.end()) wnds.erase(it);
        wnds.push_back(w);
        if (wnds.size() > 8) wnds.erase(wnds.begin());
    }
    bool left() {
        if (hover.bar < 0 || wnds.empty()) return false;
        void* w = cursorWindow(); if (!w) return false;
        if (std::find(wnds.begin(), wnds.end(), w) != wnds.end()) return false;
        return hover.set(-1);
    }
};

// =====================================================================================================================
// MboPane<L>: the whole host of lsPullStats / lsStackStats (L = ps::Logic / st::Logic). Own pane, two bars per candle from the
// order feed near price, letters, spike labels, the info box + message line, the session record, the per-market params file.
// =====================================================================================================================
struct PaneSettings { int ratioBars = RATIO_DEFAULT, pos = POS_DEFAULT, nearTicks = 0; SpikeCfg spike; int record = -1; int dx = 0, dy = 0, mode = MODE_READ; };   // (1.3.0) + Offset X / Y, Box shows   // (1.2.2) record: -1 = not a recorder pane (6 fields), 0 / 1 = the "Record order feed" box
inline bool parsePaneSettings(const std::string& line, PaneSettings& out) {
    if (line.empty() || line.size() > 64) return false;
    std::vector<std::string> f; size_t a = 0;
    for (;;) { const size_t c = line.find(',', a); f.push_back(line.substr(a, c == std::string::npos ? std::string::npos : c - a)); if (c == std::string::npos) break; a = c + 1; }
    // (1.3.0) 9 / 10 fields = the 6 / 7 of 1.2.x + Offset X, Offset Y, Box shows (0 Read, 1 Numbers)
    int dx = 0, dy = 0, mode = MODE_READ;
    if (f.size() == 9 || f.size() == 10) {
        const size_t n = f.size();
        if (!bp::intTok(f[n - 3], dx) || !bp::intTok(f[n - 2], dy) || !bp::offOk(dx) || !bp::offOk(dy) || (f[n - 1] != "0" && f[n - 1] != "1")) return false;
        mode = f[n - 1] == "1" ? MODE_NUMBERS : MODE_READ;
        f.resize(n - 3);
    }
    if (f.size() != 6 && f.size() != 7) return false;                   // (1.2.2) 7th = Record order feed (PullStats)
    for (size_t k = 0; k < f.size(); ++k) if (!allDigits(f[k], 5)) return false;
    if (f.size() == 7 && f[6] != "0" && f[6] != "1") return false;
    PaneSettings s; s.ratioBars = std::atoi(f[0].c_str()); s.pos = std::atoi(f[1].c_str());
    s.spike.avgT = std::atoi(f[2].c_str()); s.spike.jumpT = std::atoi(f[3].c_str()); s.spike.lookback = std::atoi(f[4].c_str()); s.nearTicks = std::atoi(f[5].c_str());
    if (s.ratioBars < RATIO_LO || s.ratioBars > RATIO_HI || s.pos < 0 || s.pos > 8 || s.spike.avgT > 100 || s.spike.jumpT > 100 || s.spike.lookback > SPIKE_LOOKBACK_MAX || s.nearTicks > nl::NEAR_MAX) return false;
    if (f.size() == 7) s.record = f[6] == "1" ? 1 : 0;
    s.dx = dx; s.dy = dy; s.mode = mode;
    out = s; return true;
}
inline std::string paneSettingsLine(const PaneSettings& s) {
    return std::to_string(s.ratioBars) + "," + std::to_string(s.pos) + "," + std::to_string(s.spike.avgT) + "," + std::to_string(s.spike.jumpT) + "," + std::to_string(s.spike.lookback) + "," + std::to_string(s.nearTicks)
         + (s.record >= 0 ? std::string(",") + (s.record ? "1" : "0") : std::string())
         + "," + std::to_string(bp::offOk(s.dx) ? s.dx : 0) + "," + std::to_string(bp::offOk(s.dy) ? s.dy : 0) + "," + (s.mode == MODE_NUMBERS ? "1" : "0");
}
// (1.2.2) a pane's extra per-chart state (PullStats' order-feed recorder); deleted with the pane's state
struct PaneExt { virtual ~PaneExt() {} };
inline int tenthsOf(float v) { if (!(v >= 0) || v > 10.04f) return -1; return (int)(v * 10.0f + .5f); }

template <class L> struct PaneState {
    bool busy = false;
    std::string identity, market, symbol, state = "starting", lastStatusText;
    int spb = 0; double tick = 0;
    Conn feed;
    Engine e; Acc scratch; nl::Zone zone; double atr = 0; long atrCount = -1;
    std::map<long long, Acc> accs;                   // bar end -> what it collected (open bars only)
    std::map<long long, long long> closedAt;         // bar end -> steady ms it was first seen closed (not decided yet)
    PaneSeries<L> ser;                               // decided bars
    std::map<long long, int> barEnds;                // chart bar end -> index
    long long formingKey = 0, firstBarKey = -1, lastClosedKey = 0;
    long long lateDropped = 0, noBar = 0, orphans = 0, finalized = 0, decidedLate = 0;
    std::string curSession; SessionFile file; bool loaded = false; long long sessionResets = 0; int histSessions = 0;   // (1.8.0) earlier sessions read for the slot yardstick
    nl::Cache lv; std::vector<nl::Level> levels;
    ParamCache params; PaneSettings set; SpikeCfg dlgSpike; long long settingsCheckMs = -1, settingsStamp = -2;
    HoverTrack hov; long long axisTop = 0, axisUp = -1, axisDown = -1, netUp = -1, netDown = -1; int paneH = 0;   // (1.5.1) axisUp / axisDown: the biggest + / - net on screen (net panes)
    long long statusMs = -1, changed = 0;
    OptCache opt; ResetWatch rw;                     // (1.2.0) the options file (blue line), PullStats' reset watch
    std::unique_ptr<PaneExt> ext;                    // (1.2.2) PullStats' order-feed recorder (null in the other panes)
    bp::Box lastBox; bool inBox = false; long long boxHovers = 0;   // (1.3.0) the read box last drawn; the mouse over it (numbers hover)
    zread::Host zr; long zrCount = -1; long long zrSession = -1;    // (1.5.0) the zone read line
    int timerId = 0; bool timerOn = false, timerTried = false;       // (1.4.0) the order-feed clock of this chart's pane
};

template <class L> class MboPane : public cppExtension {
public:
    SS_SAFE_DIALOG(MboPane<L>, "OrderFlowPane")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    typedef PaneState<L> S_;
    MboPane() : cppExtension() {}
    // (1.2.2) hooks for a pane that does more with the raw feed (PullStats' "Record order feed"); the defaults do nothing
    virtual bool recorderPane() const { return false; }
    virtual void onRaw(S_&, const DEPTH_ORDER&) {}
    virtual void afterPump(S_&, long long, bool) {}
    virtual void statusExtra(S_&, std::ostream&) {}
    virtual void afterTimer(S_&, long long) {}                            // (1.4.0) e.g. the recorder's once-a-second flush
    // (1.4.0, 2026-10-14 LIQ-DAY review: ES mbo_HH.csv held 30-45% of the traded volume in busy periods, 96-100% pre-market; events came
    // in 1-3 s bursts with 5-9 s holes) ROOT CAUSE: the feed was read only from this pane's calc / draw, which IRT ran every ~6-10 s on
    // the busy 3-min chart (and not at all while its layout tab is hidden), at most 5,000 events / ~8 ms per call, while IRT keeps
    // only the last few thousand events per subscriber - the rest were overwritten unread. Now a 250 ms clock per pane reads the
    // stream (opened with the chart symbol spelled out, so no chart context is needed) into a queue; calc / draw route the queue in
    // order before reading more. A fault in a clock read stops the clock (calc / draw keep reading, as before).
    virtual int timer(RTX_EVENT* e) {
        if (!e) return RTX_FAIL;
        S_* S = nullptr;
        { typename std::map<int, S_*>::const_iterator it = timers().find(e->v.timer.id); if (it != timers().end()) S = it->second; }
        if (!S) return RTX_FAIL;                                           // not ours: let IRT pass it on
        if (S->busy) return RTX_OK;
        S->busy = true;
        struct Unbusy { S_* s; ~Unbusy() { s->busy = false; } } unbusy = { S };
        try {
            const long long now = steadyMs();
            S->feed.prefetch(now, steadyMs);
            if (S->feed.timerOff) stopTimer(*S);
            afterTimer(*S, now);
        } catch (...) {}
        return RTX_OK;
    }
    virtual int draw(void) {
        pump(true);                                                        // (1.2.2) from the draw: a recorder buffers, never writes
        S_* S = slot_.get(this, false);
        if (S) { S->hov.left(); followBoard(*S); boxLeft(*S); }          // (1.2.1) the candle under the mouse ANYWHERE on this chart
        if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
        return RTX_OK;
    }
    // (1.2.1, Rassul 9:25 PM "when i move the mouse the tape reader changes but the other reads don't change") HoverBoard.h: the chart's
    // first / last visible bar times (the time axis every pane of one chart shares) key the chart-wide hover
    bool visRange(unsigned long long& t0, unsigned long long& t1) {
        t0 = t1 = 0; int a = 0, b = 0; const long n = getBarCount();
        if (n < 1 || getVisibleBars(&a, &b) != RTX_OK) return false;
        RTARRAYI times(barDateTime); if ((long)times.count < n) return false;
        if (a < 0) a = 0;
        if (b > (int)n - 1) b = (int)n - 1;
        if (b < a) return false;
        t0 = (unsigned long long)times[a]; t1 = (unsigned long long)times[b]; return t0 > 0;
    }
    bool followBoard(S_& S) {                                              // true = the hovered candle changed
        unsigned long long t0 = 0, t1 = 0; if (S.symbol.empty() || !visRange(t0, t1)) return false;
        unsigned long long t = 0; if (!hb::hovered(S.symbol, S.spb, t0, t1, t)) return false;
        RTARRAYI times(barDateTime); const long n = getBarCount(); if ((long)times.count < n) return false;
        if (S.hov.hover.set(hb::indexOf(times, n, t))) { ++S.hov.redraws; return true; }
        return false;
    }
    void publishBoard(S_& S, long bar) {
        unsigned long long t0 = 0, t1 = 0; if (S.symbol.empty() || !visRange(t0, t1)) return;
        RTARRAYI times(barDateTime); const long n = getBarCount(); if ((long)times.count < n) return;
        hb::publish(S.symbol, S.spb, bar >= 0 && bar < n - 1 ? (unsigned long long)times[(int)bar] : 0ULL, t0, t1);
    }
    // (1.3.0) the settings dialog: filled from the per-market file when it opens (loading never saves); OK / Apply save it, a control
    // the user did not touch keeping the file's value (bp::DlgSnap). On the chart IRT hands lists 0 / boxes unchecked - draw() reads the file.
    int base() const { return recorderPane() ? 7 : 6; }                   // Offset X, Offset Y, Box shows follow the 1.2.x rows
    bool inDialog() { const char* pl = getPeriodicityLabel(); return !(pl && pl[0]); }
    std::string chartMarket() { char b[32]; std::memset(b, 0, sizeof(b)); const char* rs = getRootSymbol(b); const char* sy = getSymbol(); const std::string root = rs ? rs : ""; return marketName(root.empty() ? (sy ? sy : "") : root); }
    PaneSettings fileSettings(const std::string& mkt) {
        PaneSettings ns; const std::string p = placePath(mkt); if (p.empty()) return ns;
        std::ifstream f(p.c_str(), std::ios::binary); std::string ln;
        if (f && std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); PaneSettings c; if (parsePaneSettings(ln, c)) ns = c; }
        if (recorderPane()) ns.record = 1;   // (settings 2026-10-18) recording always on: a saved 0 can no longer stop it
        ns.dx = 0; ns.dy = 0;                // (settings 2026-10-18, his decision) Offset X / Y removed: the 9-spot Position only, a saved offset is ignored
        return ns;
    }
    std::vector<long long> dialogNow() {
        std::vector<long long> v;
        v.push_back(getIntegerValue(0)); v.push_back(getListIndex(1)); v.push_back(tenthsOf(getFloatValue(2))); v.push_back(tenthsOf(getFloatValue(3)));
        v.push_back(getIntegerValue(4)); v.push_back(getIntegerValue(5)); v.push_back(recorderPane() ? (this->isBoxChecked(6) ? 1 : 0) : -1);
        v.push_back(getIntegerValue(base())); v.push_back(getIntegerValue(base() + 1)); v.push_back(getListIndex(base() + 2));
        return v;
    }
    int parmsLoadBody(void) {
        if (!inDialog()) return RTX_OK;
        const PaneSettings f = fileSettings(chartMarket());
        this->setIntegerValue(0, f.ratioBars); this->setListIndex(1, f.pos); this->setFloatValue(2, (float)f.spike.avgT / 10.0f); this->setFloatValue(3, (float)f.spike.jumpT / 10.0f);
        this->setIntegerValue(4, f.spike.lookback); this->setIntegerValue(5, f.nearTicks); if (recorderPane()) this->checkBox(6, f.record != 0);
        this->setIntegerValue(base(), f.dx); this->setIntegerValue(base() + 1, f.dy); this->setListIndex(base() + 2, f.mode);
        snap_.take(dialogNow());                                           // what the dialog shows now (filled, or IRT's own values)
        return RTX_OK;
    }
    int parmsApplyBody(void) { saveSettings(); return RTX_OK; }
    int parmsUpdtBody(unsigned int) { saveSettings(); return RTX_OK; }
    void boxLeft(S_& S) {                                                  // the mouse is no longer over this chart: no box hover
        if (!S.inBox) return;
        void* w = cursorWindow();
        if (!w) return;
        if (std::find(S.hov.wnds.begin(), S.hov.wnds.end(), w) == S.hov.wnds.end()) { S.inBox = false; invalidateChart(false); }
    }
    virtual int mouse(RTX_EVENT* ev) {
        if (!ev) return RTX_FAIL;
        S_* S = slot_.get(this, false); if (!S) return RTX_FAIL;
        if (ev->type != E_MOUSE_MOVE && ev->type != E_MOUSE_HOVER) return RTX_FAIL;
        S->hov.seen();
        {   // (1.3.0) over the read box: its numbers show (hover), and the candle it reads stays (the bars under the box are not picked)
            const bool in = S->set.mode == MODE_READ && inBox(S->lastBox, ev->v.mouse.h, ev->v.mouse.v);
            if (in != S->inBox) { S->inBox = in; if (in) ++S->boxHovers; invalidateChart(false); }
            if (in) return RTX_FAIL;
        }
        publishBoard(*S, ev->v.mouse.bar);                                 // (1.2.1) every pane of this chart follows this candle
        if (S->hov.hover.set(hoverIndex(ev->v.mouse.bar, getBarCount()))) { ++S->hov.redraws; invalidateChart(false); }
        return RTX_FAIL;                                                   // never swallow the event
    }
    virtual int oneBarBull() const { return -1; }                         // (1.6.0) the buyer side for one bar per candle; -1 = two bars
    virtual bool barMarks() const { return true; }                        // (1.6.0) false = no letters / refill dash drawn (StackStats)
    virtual bool textOff() const { return false; }                        // (1.9.0) true = no read / numbers / options lines at all (Book Flow)
    int emode(int m) const { return textOff() ? MODE_NONE : m; }
    virtual std::string boxKeep() const { return std::string(); }          // (1.10.1) box-only panes: the rows kept, ";"-separated (empty = all)
    virtual bool boxOnly() const { return false; }                        // (1.10.0, Rassul 2026-10-16) true = just the left-justified numbers box in the pane (no read line, no options line, no top text band)
    int gmode(int m) const { return boxOnly() ? MODE_NONE : emode(m); }   // the geometry mode: a box-only pane keeps the whole height for bars
    // (1.7.0, BookFlow 1.0.0) net bars (added - pulled per side, signed, a grey zero line) instead of the one-bar / two-bar columns; the pane's own hooks
    virtual bool netBars() const { return false; }
    virtual long long netOf(const Rec& r, int s) const { return L::value(r, 0, s); }   // the drawn value (signed when netBars)
    virtual void netLook(const PaneBar&, int, BarLook&) const {}           // the pane's label numbers for a net bar (ratio, pace, lots)
    virtual int zonePane() const { return std::string(L::name()) == "StackStats" ? zread::P_STACK : zread::P_PULLSTATS; }   // which zone read this pane shows
    virtual std::string zoneDirName() const { return std::string(L::name()); }                                              // whose folder holds its zone-read record
    virtual void onLoad(S_&, const std::string&, const std::string&) {}   // (flexDir, stem) a session's record was opened: open any extra record
    virtual void onDecide(S_&, long long, const PaneBar&, const Row&) {}  // a closed candle was decided and recorded: write any extra record row
    virtual void onAccClosed(S_&, long long, const Acc&) {}             // (DefenseTurn 2026-10-20) a decided candle's raw accumulator, just before it is dropped
    virtual void onSave(S_&) {}                                           // the session record was saved: save any extra record
    long long magOf(const Rec& r) const {                                 // the tallest bar of a candle: net bars = the bigger |net|, else the bigger side
        if (netBars()) { const long long a = netOf(r, BID), b = netOf(r, ASK); return std::max(a < 0 ? -a : a, b < 0 ? -b : b); }
        return std::max(L::value(r, 0, BID), L::value(r, 0, ASK));
    }
    virtual int scale(int iStartBar, int iEndBar, double* dMin, double* dMax) {
        double mn = 0, mx = 10;
        S_* S = slot_.get(this, false);
        if (S) {
            const long count = getBarCount();
            RTARRAYI times(barDateTime);
            long long top = 0, up = 0, down = 0; std::vector<long long> mags;
            if (count > 0 && times.count >= count) {
                const int last = (int)count - 1;
                int a = std::max(0, iStartBar), b = std::min(last, iEndBar);
                if (b < a) { a = std::max(0, last - 200); b = last; }
                if (b - a > 20000) a = b - 20000;
                PaneBar lb; const bool live = liveBar(*S, lb);
                for (int i = a; i <= b; ++i) {
                    const long long bt = (long long)times[i];
                    const Rec* rr = nullptr;
                    if (i == last) { if (live && bt == S->formingKey) rr = &lb.r; }
                    else { typename std::map<long long, PaneBar>::const_iterator it = S->ser.bars.find(bt); if (it != S->ser.bars.end() && it->second.ok) rr = &it->second.r; }
                    if (!rr) continue;
                    top = std::max(top, magOf(*rr));
                    if (netBars()) for (int s = 0; s < 2; ++s) { const long long v = netOf(*rr, s); if (v > up) up = v; if (-v > down) down = -v; mags.push_back(v < 0 ? -v : v); }
                }
            }
            if (netBars()) { const long long cp = capTop(mags); up = capped(up, cp); down = capped(down, cp); top = capped(top, cp); }   // (BookFlow 1.7.0) the scale cap
            S->axisTop = top; S->axisUp = up; S->axisDown = down;
            if (netBars()) axisFor(top, S->paneH, pairPx(S->paneH, textReserve(S->paneH, gmode(S->set.mode)) + QUESTION_H), mn, mx);   // (BookFlow 1.8.0) pair bars above zero; the question line on top
            else axisFor(top, S->paneH, drawPx(S->paneH, textReserve(S->paneH, gmode(S->set.mode))), mn, mx);
        }
        if (dMin) *dMin = mn;
        if (dMax) *dMax = mx;
        return RTX_OK;
    }
    void calcHook() {
        pump(); S_* S = slot_.get(this, false); if (!S) return;
        bool ch = S->hov.left(); if (ch) ++S->hov.redraws;
        if (followBoard(*S)) ch = true;                                    // (1.2.1) the mouse moved over another pane of this chart
        if (ch) invalidateChart(false);                                    // only on a change: no redraw cost otherwise
    }
    void release() { S_* S = slot_.get(this, false); if (S) stopTimer(*S); slot_.release(this); }
    S_* testState() { return slot_.get(this, false); }
    // (1.4.0) timer id -> the pane state it reads (IRT may deliver a timer in another chart's context: never trust the user-data slot)
    static std::map<int, S_*>& timers() { static std::map<int, S_*> m; return m; }
    void stopTimer(S_& S) {
        if (S.timerOn) { destroyTimer(S.timerId); S.timerOn = false; }
        if (S.timerId) timers().erase(S.timerId);
        S.timerId = 0;
    }
    void startTimer(S_& S) {                                               // chart context, once the stream is subscribed
        if (S.timerTried || S.timerOn || !S.feed.named() || S.feed.flag != 1 || S.feed.timerOff) return;
        S.timerTried = true;
        int id = 30000 + (int)(((uintptr_t)&S >> 4) % 900000);
        while (timers().count(id)) ++id;
        if (createTimer(id, (int)TIMER_MS) == RTX_OK) { S.timerId = id; S.timerOn = true; timers()[id] = &S; }
    }
    // (1.4.0) closed candles >= 5 s old: FILL lots read vs the bar volume (the MBO_COVERAGE status line)
    void coverage(S_& S, long count, long long now) {
        if (count < 3 || S.firstBarKey < 0 || S.spb <= 0) return;
        RTARRAYI times(barDateTime), vols(barVolume);
        if (times.count < count || vols.count < count) return;
        for (long i = std::max(0L, count - 12); i < count - 1; ++i) {
            const long long end = (long long)times[(int)i];
            if (end <= S.firstBarKey) continue;                            // the bar the stream opened in is partial
            S.feed.cov.offer(end, S.spb, (long long)vols[(int)i], now);
        }
    }
    void pump(bool fromDraw = false) {
        S_* P = slot_.get(this, true);
        if (!P || P->busy) return;
        S_& S = *P; S.busy = true;
        struct Unbusy { S_* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
        try {
            identify(S);
            const long long now = steadyMs();
            settings(S, now);
            if (refreshParams(S.params, flexDir(), S.market, now)) applyParams(S);
            const long count = getBarCount();
            if (S.formingKey > 0) refreshOptions(S.opt, flexDir(), S.market, now, [&](long long mt) { return arrivalBar(S, mt); });   // (1.2.0)
            if (S.tick <= 0) S.state = "native tick size unavailable";
            else if (count < 2) S.state = "waiting for bars";
            else {
                bars(S, count, now);
                S.levels = nl::levels(S.lv, flexDir().empty() ? std::string() : flexDir() + "\\", S.market, S.symbol, now, (long long)std::time(NULL));
                load(S);
                std::string st;
                {   RTARRAYI vt(barDateTime), vv(barVolume);                    // (1.17.0) the watchdog: is the chart trading?
                    if (vt.count >= count && vv.count >= count) S.feed.watch(now, (long long)vt[(int)count - 1] * 1000003LL + (long long)vv[(int)count - 1]); }
                S.feed.open(S.symbol, dir().empty() ? std::string() : dir() + "\\_opening-" + stem(S) + ".txt", now, st);
                if (!st.empty()) S.state = st;
                const double tick = S.tick;
                if (!S.feed.raw) { S_* ps = &S; S.feed.raw = [this, ps](const DEPTH_ORDER& o) { onRaw(*ps, o); }; }   // (1.4.0) once per event read
                S.feed.flushQueue([&](const DEPTH_ORDER& o, long long ms) { route(S, evOf(o, tick, ms)); });          // (1.4.0) the clock's reads first
                S.feed.drain(now, steadyMs, [&](const DEPTH_ORDER& o) { route(S, evOf(o, tick, now)); });
                if (S.feed.lastEventMs >= 0 && S.firstBarKey < 0) S.firstBarKey = S.formingKey;
                startTimer(S);
                if (S.feed.timerOff) stopTimer(S);
                coverage(S, count, now);
                decide(S, now);
                zoneTick(S, count, now);                                   // (1.5.0) the zone read line
                const std::string fs = S.feed.stateText(now);
                if (!fs.empty()) S.state = fs;
            }
            try { afterPump(S, now, fromDraw); } catch (...) {}            // (1.2.2) e.g. the recorder's once-a-second flush
            status(S, now);
        } catch (...) { S.state = "calculation error"; }
    }
private:
    HostSlot<S_> slot_;
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + "\\" + L::name(); }
    std::string stem(const S_& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
    std::string placePath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\" + L::name() + ".place-" + mkt + ".txt"; }
    std::string ctOf(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return std::string(); return clockShort(x.tm_hour, x.tm_min); }
    // (1.2.0) the CT minute of the day of a bar stamp (-1 = unreadable) and its unix time (from the wall clock and the chart's own clock,
    // so no time-zone assumption): unix(t) = now - (chart clock now - chart clock t)
    int minuteOf(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return -1; return x.tm_hour * 60 + x.tm_min; }
    long long clockSec(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return LLONG_MIN; return civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec; }
    long long unixOf(long long t) { const long long a = clockSec((long long)currentDate()), b = clockSec(t); if (a == LLONG_MIN || b == LLONG_MIN) return 0; return (long long)std::time(NULL) - (a - b); }
    // (1.2.0) the bar a file written at unix time mt arrived on: the first bar whose end is at / after it (else the forming bar)
    long long arrivalBar(S_& S, long long mt) {
        const long long a = clockSec((long long)currentDate()); if (a == LLONG_MIN || mt <= 0) return S.formingKey;
        const long long target = a - ((long long)std::time(NULL) - mt);  // mt on the chart's clock
        long long best = S.formingKey;
        for (std::map<long long, int>::const_reverse_iterator it = S.barEnds.rbegin(); it != S.barEnds.rend(); ++it) { const long long c = clockSec(it->first); if (c == LLONG_MIN || c < target) break; best = it->first; }
        return best;
    }
    std::string sessionKey(long long t) { struct tm x; std::memset(&x, 0, sizeof(x)); if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return std::string(); return sessionOf(x.tm_year + 1900, x.tm_mon + 1, x.tm_mday, x.tm_hour); }
    std::string stamp(long long t) {
        struct tm x; std::memset(&x, 0, sizeof(x));
        if (t <= 0 || !getLocaltime((RTDATE)t, &x)) return "n/a";
        char b[64]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d", x.tm_year + 1900, x.tm_mon + 1, x.tm_mday, x.tm_hour, x.tm_min, x.tm_sec); return b;
    }
    void identify(S_& S) {
        char rb[32]; std::memset(rb, 0, sizeof(rb));
        const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
        const std::string root = rs ? rs : "", sym = sy ? sy : "";
        const int spb = getSecondsPerBar();
        double tick = (double)getProperty(SYM_TICKINCR); if (!finite(tick) || tick <= 0 || tick >= 1000) tick = 0;
        const std::string id = root + "|" + sym + "|" + std::to_string(spb);
        if (id == S.identity && tick == S.tick) return;
        stopTimer(S); S.timerTried = false;                                // (1.4.0) the clock goes with the stream
        S.feed.reset();                                                    // a new symbol / bar size: start clean (unsubscribes)
        S.e = Engine(); S.accs.clear(); S.closedAt.clear(); S.ser.bars.clear(); S.barEnds.clear();
        S.formingKey = 0; S.firstBarKey = -1; S.lastClosedKey = 0; S.atr = 0; S.atrCount = -1; S.loaded = false; S.curSession.clear(); S.file = SessionFile();
        S.identity = id; S.symbol = sym; S.spb = spb; S.tick = tick; S.market = marketName(root.empty() ? sym : root);
        S.zone = nl::zoneFor(S.market); S.params = ParamCache(); S.opt = OptCache(); S.rw = ResetWatch();
        S.settingsStamp = -2; S.settingsCheckMs = -1; S.hov.hover.set(-1); S.hov.wnds.clear(); S.lastStatusText.clear();
        S.zr = zread::Host(); S.zrCount = -1; S.zrSession = -1;            // (1.5.0)
    }
    void applyParams(S_& S) {
        const Params& P = S.params.load.p;
        S.e.refillMs = P.refillWindowMs; S.e.closeTicks = P.closeTicks;
        S.ser.spike = effectiveSpike(S.dlgSpike, P);                       // new bars only: a closed bar's colour is final
    }
    void settings(S_& S, long long now) {
        if (S.settingsCheckMs >= 0 && now >= S.settingsCheckMs && now - S.settingsCheckMs < 1000) return;
        S.settingsCheckMs = now;
        const std::string p = placePath(S.market);
        const long long st = p.empty() ? -1 : fileStamp(p);
        if (st == S.settingsStamp) return;
        const bool first = S.settingsStamp == -2;
        S.settingsStamp = st;
        PaneSettings ns;
        if (st >= 0) {
            std::ifstream f(p.c_str(), std::ios::binary); std::string ln;
            if (f && std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); PaneSettings c; if (parsePaneSettings(ln, c)) ns = c; }
        }
        ns.dx = 0; ns.dy = 0; if (recorderPane()) ns.record = 1;          // (settings 2026-10-18, his decision) recording always on; the 9-spot Position only (a saved offset ignored)
        const bool renear = ns.nearTicks != S.set.nearTicks;
        if (ns.pos != S.set.pos || ns.dx != S.set.dx || ns.dy != S.set.dy || ns.mode != S.set.mode) { S.lastBox = bp::Box(); S.inBox = false; }   // (1.3.0) moved: no stale hover rect
        S.set = ns; S.dlgSpike = ns.spike;
        const SpikeCfg eff = effectiveSpike(S.dlgSpike, S.params.load.p);
        if (S.ser.ratioBars != ns.ratioBars || S.ser.spike != eff) {      // a dialog change: every recorded bar re-decided
            S.ser.ratioBars = ns.ratioBars; S.ser.spike = eff;
            if (!first) { const Params P = S.params.load.p; S.ser.recompute([&](PaneBar& b) { L::decide(b, P, false); }); }
        }
        if (renear) S.atrCount = -1;
    }
    void saveSettings() {
        if (!inDialog()) return;                                           // (1.3.0) only the settings window's values are real
        const std::string mkt = chartMarket();
        const PaneSettings fs = fileSettings(mkt);
        const std::vector<long long> now = dialogNow();
        const long long fv[10] = { fs.ratioBars, fs.pos, fs.spike.avgT, fs.spike.jumpT, fs.spike.lookback, fs.nearTicks, fs.record, fs.dx, fs.dy, fs.mode };
        long long v[10]; for (size_t k = 0; k < 10; ++k) v[k] = snap_.pick(k, now[k], fv[k]);
        PaneSettings s;
        s.ratioBars = (int)v[0]; s.pos = (int)v[1]; s.spike.avgT = (int)v[2]; s.spike.jumpT = (int)v[3]; s.spike.lookback = (int)v[4]; s.nearTicks = (int)v[5];
        if (recorderPane()) s.record = v[6] ? 1 : 0;                       // (1.2.2) PullStats: Record order feed
        // (1.3.0) a garbage / out-of-range dialog value never reaches the file: the file's (validated) value stays
        s.dx = bp::offOk(v[7]) ? (int)v[7] : fs.dx; s.dy = bp::offOk(v[8]) ? (int)v[8] : fs.dy; s.mode = v[9] == MODE_READ || v[9] == MODE_NUMBERS ? (int)v[9] : fs.mode;
        if (!bp::posOk(s.pos)) s.pos = fs.pos;
        PaneSettings chk;
        if (s.spike.avgT < 0 || s.spike.jumpT < 0 || s.spike.lookback < 0 || s.nearTicks < 0 || !parsePaneSettings(paneSettingsLine(s), chk)) return;   // never overwrite with garbage
        const std::string p = placePath(mkt); if (p.empty()) return;
        makeDir(flexDir());
        publishFile(p, paneSettingsLine(s) + "\n");
        snap_.take(now);                                                   // the next Apply compares with what the dialog shows now
        S_* S = slot_.get(this, false);
        if (S) { S->settingsCheckMs = -1; S->settingsStamp = -1 - 3; }    // re-read on the next pass (not "first")
    }
    // chart bars: bar ends -> index, the forming bar, ATR of closed bars and the near band, the fallback last price
    void bars(S_& S, long count, long long now) {
        RTARRAYI times(barDateTime); RTARRAY high(barHigh), low(barLow), close(barClose);
        if (times.count < count || high.count < count || low.count < count || close.count < count) { S.state = "chart data arrays unavailable"; return; }
        const long first = std::max(0L, count - 1500L);
        if (S.barEnds.empty() || S.barEnds.rbegin()->first != (long long)times[(int)count - 1] || (long)S.barEnds.size() != count - first) {
            S.barEnds.clear();
            for (long i = first; i < count; ++i) { const long long bt = (long long)times[(int)i]; if (bt > 0) S.barEnds[bt] = (int)i; }
        }
        S.formingKey = (long long)times[(int)count - 1];
        if (count != S.atrCount) {                                         // ATR from CLOSED bars only (newest first)
            S.atrCount = count;
            std::vector<double> rg;
            for (int i = (int)count - 2; i >= 0 && rg.size() < 20; --i) rg.push_back((double)high[i] - (double)low[i]);
            S.atr = nl::atrOf(rg, S.spb);
            S.e.near = nl::nearBand(S.atr, S.tick, S.zone, S.set.nearTicks);
        }
        Tick px = 0;
        if (toTick((double)close[(int)count - 1], S.tick, px)) S.e.fallbackRef(px, now);
        { std::map<long long, int>::const_reverse_iterator r = S.barEnds.rbegin(); if (r != S.barEnds.rend()) { ++r; if (r != S.barEnds.rend()) S.lastClosedKey = r->first; } }
        // a new trading session: this session's file, the size distributions start over
        const std::string ss = sessionKey(S.formingKey);
        if (!ss.empty() && ss != S.curSession) {
            if (!S.curSession.empty()) { S.file.save(); S.e.newSession(); S.sessionResets++; }
            S.curSession = ss; S.loaded = false;
        }
    }
    void zoneTick(S_& S, long count, long long now) {                     // (1.5.0) the zone read line (zoneTickFor)
        const std::string d = dir();
        const std::string zdir = flexDir().empty() ? std::string() : flexDir() + "\\" + zoneDirName();
        zoneTickFor(this, S.zr, S.zrCount, S.zrSession, zonePane(), S.tick, S.market, S.zone, S.atr,
                    d.empty() || zdir.empty() || S.curSession.empty() ? std::string() : zdir + "\\" + stem(S) + "-zoneread-" + S.curSession + ".csv", count, now);
    }
    // an event to the candle containing its own stamp; one that cannot be counted (no bar, a decided bar, a bar before the stream
    // opened - e.g. a snapshot of old orders) still teaches the engine its order (a scratch candle, thrown away)
    void route(S_& S, const Ev& x) {
        S.zr.e.onEvent(x.arrMs, x.action, x.side, x.id, x.px, x.size);   // (1.5.0) every event, in order, to the zone read (its own book)
        const long long key = eventKey(S.barEnds, S.formingKey, S.spb, x.stamp);
        const bool noBar = key < 0, decided = !noBar && S.ser.bars.count(key) > 0, before = !noBar && S.firstBarKey > 0 && key < S.firstBarKey;
        const bool early = !noBar && S.firstBarKey < 0 && key < S.formingKey;      // before the first event of this load
        if (noBar || decided || before || early) {
            if (noBar) S.noBar++; else S.lateDropped++;
            S.e.apply(S.scratch, x);
            if (S.scratch.events > 20000) S.scratch = Acc();
            return;
        }
        S.e.curKey = key; S.e.apply(S.accs[key], x);
    }
    std::string levelAt(S_& S, long long key) {
        std::map<long long, int>::const_iterator i = S.barEnds.find(key);
        if (i == S.barEnds.end() || S.e.near <= 0) return std::string();
        RTARRAY close(barClose); if (close.count <= i->second) return std::string();
        return nl::nearestLevel(S.levels, (double)close[i->second], (double)S.e.near * S.tick);
    }
    Row rowOf(long long key, const PaneBar& b) {
        Row r; r.t = key; r.ct = ctOf(key); r.dbo = 1; recNums(b.r, r.nums);
        for (int s = 0; s < 2; ++s) { r.let[s] = b.let[s]; r.spike[s] = b.spike[s] ? 1 : 0; r.dash[s] = b.dash[s]; }
        r.impl = b.msg.impl; r.level = b.level; r.line1 = b.msg.line1; r.line2 = b.msg.line2;
        return r;
    }
    // closed candles are decided once: 0.5 s after they closed with the drain caught up, or 5 s after they closed
    void decide(S_& S, long long now) {
        bool any = false;
        for (typename std::map<long long, Acc>::iterator it = S.accs.begin(); it != S.accs.end() && it->first < S.formingKey;) {
            const long long key = it->first;
            if (!S.barEnds.count(key)) { S.closedAt.erase(key); S.accs.erase(it++); S.orphans++; continue; }   // a predicted bar the chart never made
            std::map<long long, long long>::iterator c = S.closedAt.find(key);
            if (c == S.closedAt.end()) c = S.closedAt.insert(std::make_pair(key, now)).first;
            const long long age = now >= c->second ? now - c->second : 0;
            if (age < FINAL_GRACE_MS && (S.feed.lastCapped || age < DECIDE_DELAY_MS)) { ++it; continue; }
            if (S.feed.lastCapped) S.decidedLate++;
            if (!S.ser.bars.count(key)) {
                const Params& P = S.params.load.p;
                PaneBar nb = S.ser.make(key, S.e.close(it->second, key == S.firstBarKey, P));
                nb.level = levelAt(S, key);                                 // decided once, at the close
                panePace(nb, S.ser, key, S.spb, minuteOf(key), P);          // (1.2.0) pace vs the norm, decided once
                L::decide(nb, P, false);
                if (L::resets()) {                                          // (1.2.0) the later "reset" message, on the candle it completed in
                    const int ws = S.rw.side;
                    const long long sec = resetStep(S.rw, it->second.addT[ws == ASK ? ASK : BID], key, P);
                    if (sec >= 0 && nb.ok && nb.msgSide < 0) { nb.msg.line1 = resetMessage(ws, sec, S.rw.pullCt, S.rw.level); nb.msg.impl = NEUTRAL; nb.kind = 3; }
                    if (nb.ok && nb.kind == 1 && nb.msgSide >= 0) resetStart(S.rw, nb.msgSide, key, nb.r.pullC[nb.msgSide], P, ctOf(key), nb.level);
                }
                S.ser.bars[key] = nb; S.finalized++; any = true;
                if (nb.ok) onAccClosed(S, key, it->second);                // (DefenseTurn) before onDecide, so the turn sees this candle's refill / extreme
                if (nb.ok && sessionKey(key) == S.curSession) { const Row rw0 = rowOf(key, nb); S.file.put(rw0); onDecide(S, key, nb, rw0); }
            }
            S.closedAt.erase(c); S.accs.erase(it++);
        }
        if (any) { S.file.sizes(S.e.aggrDist, S.e.restDist); S.file.save(); onSave(S); S.changed++; invalidateChart(false); }
        if (S.ser.bars.size() > 5000) S.ser.trim(5000);
        while (S.accs.size() > 16) S.accs.erase(S.accs.begin());          // bounded: never more than a few open bars
    }
    // this session's record, once per session: its candles come back exactly as they were decided (bars, letters, message)
    void load(S_& S) {
        if (S.loaded || S.curSession.empty()) return;
        S.loaded = true;
        const std::string d = dir(); if (d.empty()) return;
        makeDir(flexDir()); makeDir(d);
        const std::string base = d + "\\" + stem(S) + "-" + S.curSession;
        S.file.open(S.curSession, base + ".csv", base + "-sizes.txt",
                     std::string("# ") + L::name() + " " + L::version() + " " + S.market + " " + S.symbol + " " + std::to_string(S.spb) + "\nbar_time|ct_time|dbo|" + REC_COLS + "|letters_left|letters_right|spike_left|spike_right|dash_left|dash_right|implication|level|message|numbers\n",
                     (size_t)REC_NUMS);
        S.file.load();
        S.file.loadSizes(S.e.aggrDist, S.e.restDist);
        onLoad(S, flexDir(), stem(S));
        for (std::map<long long, Row>::const_iterator it = S.file.rows.begin(); it != S.file.rows.end(); ++it) {
            const Row& r = it->second; if (r.t >= S.formingKey || S.ser.bars.count(r.t)) continue;   // never the forming bar
            PaneBar b; if (!recFrom(r.nums, 0, b.r)) continue;
            S.e.barFrom(b.r);                                              // (1.3.0) the rare-L history
            b.ok = true; b.fromFile = true; b.level = r.level;
            for (int s = 0; s < 2; ++s) { b.let[s] = r.let[s]; b.spike[s] = r.spike[s] != 0; b.dash[s] = r.dash[s]; }
            b.msg.impl = r.impl; b.msg.line1 = r.line1; b.msg.line2 = r.line2;
            S.ser.bars[r.t] = b;
        }
        // (1.8.0, audit #1) the earlier sessions' records (up to 4 within 10 calendar days): only their numbers, for the same-slot yardstick
        {   const std::vector<std::string> back = daysBefore(S.curSession, 10); int got = 0; S.histSessions = 0; S.ser.hist.clear();
            for (size_t i = 0; i < back.size() && got < 4; ++i) {
                SessionFile hf; hf.open(back[i], d + "\\" + stem(S) + "-" + back[i] + ".csv", std::string(), std::string(), (size_t)REC_NUMS);
                if (!hf.load() || hf.rows.empty()) continue;
                ++got;
                for (std::map<long long, Row>::const_iterator it = hf.rows.begin(); it != hf.rows.end(); ++it) {
                    Rec rr; if (!recFrom(it->second.nums, 0, rr) || rr.partial) continue;
                    std::vector<long long> v((size_t)(L::metrics() * 2), 0);
                    for (int k = 0; k < L::metrics(); ++k) for (int s = 0; s < 2; ++s) v[(size_t)(k * 2 + s)] = L::value(rr, k, s);
                    S.ser.hist[it->first] = v;
                }
            }
            S.histSessions = got;
        }
        for (typename std::map<long long, PaneBar>::iterator it = S.ser.bars.begin(); it != S.ser.bars.end(); ++it) if (it->second.fromFile) S.ser.stats(it->first, it->second);
    }
    bool liveBar(const S_& S, PaneBar& b) {
        typename std::map<long long, Acc>::const_iterator it = S.accs.find(S.formingKey);
        if (it == S.accs.end() || it->second.events <= 0) return false;
        b = S.ser.make(S.formingKey, S.e.numbers(it->second, S.formingKey == S.firstBarKey, S.params.load.p));   // as of now, nothing recorded
        L::decide(b, S.params.load.p, true);
        return b.ok;
    }
    void status(S_& S, long long now) {
        if (S.statusMs >= 0 && now >= S.statusMs && now - S.statusMs < 15000) return;
        const Engine& e = S.e; const Conn& f = S.feed;
        std::ostringstream o; o.imbue(std::locale::classic());
        o << "VERSION," << L::version() << "\nINDICATOR," << L::name() << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
          << "\nFEED_WATCH,dropouts " << f.dropouts << ",reconnects " << f.reconnects << ",silent now " << (f.silentFromMs >= 0 ? "yes" : "no")
          << "\nSUBSCRIBED," << (f.flag < 0 ? "unknown" : f.flag ? "1" : "0") << "\nOPENED_WITH," << (f.via.empty() ? "not opened" : f.via)
          << "\nEVENTS_PER_MIN," << f.roll.perMin(now) << "\nEVENTS_TOTAL," << e.events
          << "\nACTIONS,NEW " << e.perAction[0] << ",MOD " << e.perAction[1] << ",DEL " << e.perAction[2] << ",FILL " << e.perAction[3]
          << "\nUNKNOWN,del " << e.unknownDel << ",mod " << e.unknownMod << ",fill " << e.unknownFill << ",dup_new " << e.dupNew << ",garbage " << e.garbage
          << "\nDROPPED,late " << S.lateDropped << ",no_bar " << S.noBar << ",orphan_bars " << S.orphans << ",decided_with_backlog " << S.decidedLate
          << "\nBOOK,orders " << e.book.size() << ",drops " << e.bookDrops << ",resets " << e.bookResets
          << "\nNEAR_TICKS," << e.near << (S.set.nearTicks > 0 ? " (setting)" : " (auto)") << "\nATR," << S.atr << "\nLAST_PRICE_TICKS," << e.ref
          << "\nREADS,slices " << f.slices << ",cap_hits " << f.capHits << ",longest_backlog " << f.longestBacklog << ",faults " << f.faults
          << "\nFEED_CLOCK," << (S.timerOn ? "on" : f.timerOff ? "off (a clock read faulted - calc / draw only)" : S.timerTried ? "refused by IRT (calc / draw only)" : "not started")
          << "," << TIMER_MS << " ms,ticks " << f.timerTicks << ",events clock " << f.timerEvents << " / chart " << f.pumpEvents << ",queue_peak " << f.qPeak
          << ",queue_dropped " << f.qDropped << ",clock_faults " << f.timerFaults
          << "\nMBO_COVERAGE," << coverageText(S)
          << "\nSETTINGS," << paneSettingsLine(S.set) << "\nSPIKE_IN_FORCE,avg " << tenthsText(S.ser.spike.avgT) << ",jump " << tenthsText(S.ser.spike.jumpT) << ",lookback " << S.ser.spike.lookback
          << "\nPARAMS_SOURCE," << S.params.load.source << "\nPARAMS," << paramsText(S.params.load.p);
        for (size_t i = 0; i < S.params.load.notes.size() && i < 20; ++i) o << "\nPARAMS_NOTE," << S.params.load.notes[i];
        o << "\nLARGE_NOW,aggressor " << largeThreshold(e.aggrDist, S.params.load.p.largeLotAggr, S.params.load.p.largeLotPct, S.params.load.p.largeLotMin) << " (" << e.aggrDist.total << " orders)"
          << ",resting " << largeThreshold(e.restDist, S.params.load.p.largeLotResting, S.params.load.p.largeLotPct, S.params.load.p.largeLotMin) << " (" << e.restDist.total << " orders)"
          << "\nSESSION," << S.curSession << "\nSLOT_YARDSTICK,earlier sessions " << S.histSessions << ",their bars " << S.ser.hist.size() << ",same 30-min slot from 10 bars (else the 20-bar rule)" << "\nSESSION_FILE,rows " << S.file.rows.size() << ",loaded " << S.file.loaded << ",rejected " << S.file.rejected << ",writes " << S.file.writes << ",fails " << S.file.fails
          << "\nBARS_DECIDED," << S.finalized << "\nLEVELS," << S.levels.size() << "," << S.lv.note
          << "\nOPTIONS," << S.opt.why << ",asof " << S.opt.last.asof << ",reads " << S.opt.reads << ",arrived " << S.opt.hist.v.size()
          << "\nRESET_WATCH," << (S.rw.on ? "on" : "off") << ",side " << (S.rw.side == ASK ? "offers" : "bids") << ",target " << S.rw.target << ",back " << S.rw.cum << ",resets " << S.rw.resets;
        typename std::map<long long, PaneBar>::const_iterator it = S.ser.bars.find(S.lastClosedKey);
        o << "\nLAST_CLOSED_BAR," << S.lastClosedKey << "," << stamp(S.lastClosedKey);
        if (it != S.ser.bars.end()) o << "\nMESSAGE," << implText(it->second.msg.impl) << "," << it->second.msg.line1 << "\nNUMBERS," << it->second.msg.line2 << "\nLETTERS," << it->second.let[0] << "," << it->second.let[1];
        statusExtra(S, o);                                                 // (1.2.2)
        o << "\n" << S.zr.status() << "\n";                               // (1.5.0) the zone read
        const std::string text = o.str(); if (text == S.lastStatusText) return;
        const std::string d = dir(); if (d.empty()) return;
        makeDir(flexDir()); makeDir(d);
        S.statusMs = now;
        if (publishFile(d + "\\status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
    }
    // (1.4.0) "last bar 3:24 PM 97% (fills 1234 / volume 1271),last 10 bars 95%,lowest 31% at 9:03 AM,bars 120,session 88%"
    std::string coverageText(S_& S) {
        const Coverage& c = S.feed.cov;
        if (c.last.empty()) return "no closed bar checked yet (fills read vs bar volume, each closed bar >= 5 s after it closes)";
        const Coverage::Row& r = c.last.back();
        std::ostringstream o; o.imbue(std::locale::classic());
        o << "last bar " << ctOf(r.end) << " " << r.pct << "% (fills " << r.fills << " / volume " << r.vol << "),last 10 bars " << c.recentPct() << "%";
        if (c.low.pct >= 0) o << ",lowest " << c.low.pct << "% at " << ctOf(c.low.end) << " (fills " << c.low.fills << " / volume " << c.low.vol << ")";
        o << ",bars " << c.checked << ",since load " << Coverage::pctOf(c.sumFills, c.sumVol) << "%";
        return o.str();
    }
    void render(S_& S) {
        RCT pane; pane.getPaneRect(false);
        const long count = getBarCount();
        if (count < 2 || pane.bottom - pane.top < 24 || pane.right - pane.left < 60) return;
        RTARRAYI times(barDateTime);
        if (times.count < count) return;
        const int last = (int)count - 1;
        auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
        int v0 = 0, v1 = last;
        if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0 || v1 > last) { v1 = last; v0 = last; while (v0 > 0 && xOf(v0 - 1) >= pane.left) --v0; }
        int slot = (int)getPixelsPerBar();
        if (slot <= 0 || slot > 400) slot = v1 > v0 ? std::max(2, std::abs(xOf(v1) - xOf(v1 - 1))) : 8;
        PaneBar lb; const bool haveLive = liveBar(S, lb) && (long long)times[last] == S.formingKey;
        auto barOf = [&](int i) -> const PaneBar* {
            if (i == last) return haveLive ? &lb : nullptr;
            typename std::map<long long, PaneBar>::const_iterator it = S.ser.bars.find((long long)times[i]);
            return it != S.ser.bars.end() && it->second.ok ? &it->second : nullptr;
        };
        long long top = 0;
        for (int i = v0; i <= v1; ++i) { const PaneBar* b = barOf(i); if (b) top = std::max(top, magOf(b->r)); }
        if (S.axisTop > top) top = S.axisTop;
        S.paneH = pane.bottom - pane.top;
        const PaneGeo g = paneGeo(this, gmode(S.set.mode));
        const int hov = S.hov.hover.bar >= 0 && S.hov.hover.bar < last ? S.hov.hover.bar : -1;
        const COLOR normal[2] = { C_GREEN_BAR, C_RED_BAR }, spike[2] = { C_GREEN_SPIKE, C_RED_SPIKE };   // left = bids (green), right = offers (red)
        RTARRAY op(barOpen), cl(barClose);                                 // (1.6.0) the close direction (one-bar ties)
        // (12:05) the info box (hovered candle, else the last closed) and the message line move together
        const int mb = hov >= 0 ? hov : last - 1;
        const long long mt = (long long)times[mb];
        typename std::map<long long, PaneBar>::const_iterator mi = S.ser.bars.find(mt);
        const PaneBar* mbar = mi != S.ser.bars.end() && mi->second.ok ? &mi->second : nullptr;
        Info info = L::info(mbar, S.params.load.p);
        if (boxOnly() && !boxKeep().empty()) {   // (1.10.1, Rassul 2026-10-16 22:54) only the rows asked for
            const std::string keep = ";" + boxKeep() + ";"; std::vector<InfoRow> kept;
            for (size_t k = 0; k < info.rows.size(); ++k) if (keep.find(";" + info.rows[k].label + ";") != std::string::npos) kept.push_back(info.rows[k]);
            info.rows.swap(kept);
        }
        { struct tm x; std::memset(&x, 0, sizeof(x)); info.head = hov < 0 ? std::string("Live") : (mt > 0 && getLocaltime((RTDATE)mt, &x) ? clock12(x.tm_hour, x.tm_min) : std::string("n/a")); }
        Msg m;
        if (mbar) m = mbar->msg;
        else {
            const bool pending = S.accs.count(mt) > 0;
            m.line1 = pending ? "Deciding this candle..." : S.firstBarKey < 0 || mt < S.firstBarKey ? "No order data for this candle (before the indicator loaded)" : "No order data for this candle";
            if (S.state != "ready") m.line1 += " - " + S.state;
        }
        // (1.2.0) the blue options line: the snapshot that had ARRIVED by the candle read (hovered: fresh at its end; live: now)
        const ofo::Snap* os = S.opt.hist.latest(hov >= 0 ? mt : S.formingKey);
        const long long ref = hov >= 0 ? unixOf(mt) : (long long)std::time(NULL);
        const std::string optL = os && ofo::fresh(*os, ref) ? L::optLine(*os, S.params.load.p.gate[L::optGate()], S.market) : std::string();
        // (1.3.0) the panel: the READ box (numbers on hover) or the compact numbers box + the read beside it
        bp::Place pl; pl.pos = S.set.pos; pl.dx = S.set.dx; pl.dy = S.set.dy;
        const bp::Box area = bp::mk(g.left + 3, g.top + 3, g.edge - 3, g.bottom - 3);
        // (1.5.0) Read = the ONE zone read line (only inside a key / gamma zone); Numbers = the compact numbers box + that line beside it
        const zread::Line zl = S.zr.e.line(S.zr.pane, steadyMs());
        const bool noText = !boxOnly() && emode(S.set.mode) == MODE_NONE;                   // (1.9.0) Book Flow: no text lines
        const bool readMode = !boxOnly() && clampMode(S.set.mode) == MODE_READ;
        PanelPlan P; LinePlan LP;
        if (noText) { S.lastBox = bp::Box(); S.inBox = false; }
        else if (readMode) { LP = planLine(area, pl, zl, false, measureWith(this), paneQuestion(L::title())); S.lastBox = LP.show ? LP.box : bp::Box(); if (!LP.show) S.inBox = false; }
        else { P = planPanel(area, g.top, boxOnly() ? MODE_NUMBERS : S.set.mode, pl, info, boxOnly() || !zl.show ? std::string() : zl.text, boxOnly() ? std::string() : optL, g.reserve, measureWith(this)); S.lastBox = P.ok ? P.box : bp::Box(); }
        (void)m;
        std::vector<LBox> avoid;
        if (LP.show) avoid.push_back(LBox{ LP.box.l, LP.box.t, LP.box.r, LP.box.b, 0, 0 });
        if (P.ok) avoid.push_back(LBox{ P.box.l, P.box.t, P.box.r, P.box.b, 0, 0 });
        // (1.3.1) the hover grid is NOT an obstacle: it is drawn over the bars while the mouse is in the box; labels never re-decide for it
        for (size_t k = 0; k < P.textRects.size(); ++k) avoid.push_back(LBox{ P.textRects[k].l, P.textRects[k].t, P.textRects[k].r, P.textRects[k].b, 0, 0 });
        const std::function<BarLook(int)> lookFn = [&](int i) {
            BarLook k; const PaneBar* b = barOf(i); if (!b) return k;
            k.ok = true;
            for (int s = 0; s < 2; ++s) { k.v[s] = netBars() ? netOf(b->r, s) : L::value(b->r, 0, s); k.spike[s] = b->spike[s]; k.dash[s] = b->dash[s]; k.ratioOk[s] = b->m[0].ratioOk[s]; k.ratio[s] = b->m[0].ratio[s]; k.p100[s] = b->r.pace100[s]; if (i != last) k.let[s] = b->let[s]; if (netBars()) netLook(*b, s, k); }
            if (i < op.count && i < cl.count) { const float o = op[i], c = cl[i]; if (std::isfinite(o) && std::isfinite(c)) k.dir = c > o ? 1 : c < o ? -1 : 0; }   // (1.6.0)
            return k;
        };
        if (netBars()) drawNetBars(this, g, v0, v1, slot, top, normal, spike, xOf, lookFn, avoid);
        else drawBars(this, g, v0, v1, slot, top, hov, normal, spike, xOf, lookFn, avoid, oneBarBull(), barMarks());
        const COLOR vcol[2] = { 0x0022C55E, 0x00EF4444 };                 // values: bids (buyers) green, offers (sellers) red
        if (noText) {} else if (readMode) drawLine(this, LP); else drawPanel(this, P, info, vcol, implOf(zl), false);
        (void)hov;   // (1.8.1, Rassul 18:57 "get rid of the hoverover ... they just take up too much space") no bar hover, no read-line hover
    }
    bp::DlgSnap snap_;                                                     // (1.3.0) the dialog as it opened (this object = the dialog's copy)
};

} // namespace off
#endif // OFF_SDK
#endif
