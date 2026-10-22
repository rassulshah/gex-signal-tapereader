/********************************************************************************
 *  HodLod.cpp  --  Investor/RT RTX extension  lsHodLod  (1.0.0, 2026-10-09; 1.1.8 2026-10-12: the day's first IN call ledger; 1.1.9: "called h:mm", "Range used")
 *  2.6.0 (2026-10-22, Rassul 15:57 CT "yes hod lod box"; MBO Roadmap "What does the pre-open book say about the first extreme?", ticket
 *         T-20261022-29): the PRE-OPEN BOOK row "Pre" under Est (PreOpenLogic.h 1.0.0, SDK-free, tested + replayed on ES / NQ MBO): from
 *         30 min before the RTH open to 60 min after it, ONE line - "Bids 2.1x . 70% stay . open sell 1.8x . 1st LOW early ?" - the MBO
 *         book within 1 ATR of price in the last N minutes before the open (setting "Pre-open minutes", default 15), the share of the
 *         size that rested 60 s+ and STAYED into the open, the opening imbalance (first 3 min) and an untested lean ("?" until the nightly
 *         proves it). Live inside the window, frozen at the open, completed at open + 3 min; recorded to lsFlexLevels\HodLod\PreOpen-
 *         <MKT>-<spb>.csv (SNAP + IMB lines) and read back after a restart (never repainted). No mark on the price chart. Its own hover
 *         (HL_PRE_HOVER - the one hover on this box; question / what / why / how / example + the numbers); TRACK_MOUSE for it only.
 *         The order feed: ONE DEPTH_BY_ORDER per chart (hlpo::Feed, the lsMboCheck / OrderFlowFeed.h off::Conn pattern COPIED, not
 *         included), opened only from 45 min before the open to 4 min after it, then deleted (once a day), crash-guard file, 250 ms clock.
 *         Settings: "Show pre-open line" + "Pre-open minutes" (parameter version 8). Status: PREOPEN / PREFEED lines.
 *  2.4.2 (2026-10-18 settings audit, Rassul 15:13 CT "make sure every indicator always has its settings verified and tested so there
 *         are not bugs anymore"): the settings window from ONE table (HodLodSettings.h / SettingsSpec.h / SettingsHost.h). Fixed: the
 *         colour rows were filled / read with setIntegerValue / getIntegerValue (swatches BLACK, a picked colour never saved - the place
 *         files held -1 for every colour); Show boxes now grey their colours; "Up (to the HOD)" / "Down (to the LOD)" -> "Up (HOD)" /
 *         "Down (LOD)" so the row fits (~424 -> ~372 px of 400); file HodLod.settings-<MKT>.txt (keyed, migrated once from
 *         HodLod.place-<MKT>.txt, which is left in place); parameter version 6 (IRT resets the saved rows once; the file keeps them).
 *  2.4.1 (2026-10-18, Rassul approved 15:03 CT the compact Big Lot box "connected to the hod lod box ... so it doesn't overlap"):
 *         the table's rectangle (table + Q-Scores header + GEX row, after the hysteresis) is published on every draw for the other
 *         indicators on this chart (BoxDock.h, LRA_HLB_<symbol>_<spb>_<chart>; lsBigBidAsk 1.16.0 docks its box to it); cleared when the
 *         table is off / not drawn and when the indicator is removed. Nothing drawn here changes.
 *  2.4.0 (2026-10-18 night, Rassul approved the audit-C fixes; audit-C-gamma-hodlod-read findings 7, 8, 13 + the Market State):
 *         - Reg: from ONE MenthorQ source - the HVL0 strike of lsFlexLevels\GammaProfile-MQ-<MKT>.csv (the same snapshot whose
 *           "HVL 0D" line the Dealer Profile draws) against the last CLOSED bar's close: at / above = Pos, below = Neg. The bridge's
 *           LRA-Session GAM used the book's FLIP (MenthorQ's all-expiry HVL: ES 10-16 FLIP 7820 vs HVL0 7780) - two "flips" on one
 *           chart; it is parsed for the status line only. Aged by Fresh.h (one rule for every indicator): STALE -> "Reg: STALE" (grey),
 *           never a stale regime as live; PAUSED (MenthorQ's evening / weekend pause, the closing set) is shown as before.
 *         - Q-Scores aged (Fresh.h judgeDaily): stale -> "Q: STALE", the old scores only in the hover with their date.
 *         - Gex S/R row aged by Fresh.h also outside MenthorQ hours (older than the last close = "STALE 2d", dimmed).
 *         - the Tape Reader slot is reserved only while TapeReader.status-<MKT>.txt was written in the last 120 s (an old file from a
 *           removed Tape Reader no longer pushes the table).
 *         - MARKET STATE (MarketState.h, MARKET-STATE-SPEC.md 1.0): SRC "HODLOD" and "TREND" (this box owns the VWAP trend it shows),
 *           decided once per closed bar (slow state: regime / Q / band / age), re-posted every 10 s so readers see it fresh; cleared on
 *           removal. Nothing drawn changes for it. windows.h now comes first (MarketState.h -> BannerBoard.h); GetCursorPos from it.
 *  2.1.0 (2026-10-14 evening; Rassul 16:01-16:04 CT "a line in the HOD/LOD indicator box that identifies the key GEX levels where a
 *         reaction can occur so it is front and center - this will be where I look for trades"; "must be able to identify strikes like
 *         7750 ... see growth in the absolute gex, regardless of whether it is an identified gamma level"): a GEX row under the header -
 *         at most 3 strikes from lsFlexLevels\LRA-GexNodes-<MKT>.csv (the bridge's lra.gex_nodes, every MenthorQ grid that arrives):
 *         the top 3 abs GEX within 1 expected move or GROWING (up arrow: rank <= 5, share +30% vs its neighbours since the open / last
 *         hour), nearest the price first, one per zone, MenthorQ name in blue. Mouse over the row (TRACK_MOUSE) = the detail (rank,
 *         share, growth as ratios, WALL / FUEL = model, zone mates, what it answers / how to use it / an example). Stale > 20 min in
 *         MenthorQ hours = dimmed + "STALE 25m"; no file = no row. Read every 5 s by stamp, 64 KiB cap, strict parse (HodLodGex.h).
 *         Replay ES 10-14: 7750 G8 is on the line from the 12:46 PM grid (rank 3) through the test of the zone.
 *  2.0.6 (2026-10-14, CR4800; Rassul 07:13 "HOD/LOD table: move it DOWN, default Bottom centre", 07:07 Offset X / Y on every box, 07:18
 *         "test them thoroughly ... dont shake ... dont have garbage values"): default Position Bottom centre (7); a place file holding the
 *         2.0.5 default 1 becomes 7 ONCE per market (HodLod.migrate-206.txt), any other choice is kept. WHY THE TABLE WAS MISSING ON ES /
 *         CL THIS MORNING: 2.0.5 read its checkboxes on the chart (isBoxChecked in draw) - IRT hands the chart copy every box UNCHECKED
 *         (only the settings window's copy has the real values; Dealer Profile 2.5 found the same), so "Show table" read off, the table
 *         was never laid out and no TABLE_BOX was written. Now every setting (Position, Offset X / Y, the 3 checkboxes, the 5 colours)
 *         is saved per market from the settings window into HodLod.place-<MKT>.txt and the chart reads only that file (no file = every
 *         element on). The dialog is filled from the file when it opens; a control he did not touch keeps the file's value. The table's
 *         place has hysteresis (BoxPlace.h): a new spot only after 2 draws, never straight back. Status: PLACE / OPTS lines.
 *  2.0.5 (2026-10-13 night): the header reads "Regime: Neg / Uptrend" (Rassul 21:01; "Range" when there is no trend, 21:20) -
 *         the VWAP trend of the RTH shown, computed natively from the chart's CLOSED bars with VwapTrendLogic.h (vtl::fromChart, RTH:
 *         the same rule as the Tape Reader TREND vote; words = vtl::displayLabel: Uptrend / Dntrend / Strong ...). Default Position Top centre (16:24 "the table at the TOP
 *         of the price pane"; 21:04 "even the location of the hod lod is incorrect"): a place file holding the old default 7 (Bottom
 *         centre) becomes 1 ONCE per market (logged in HodLod.migrate-205.txt), any other choice is kept. The IRT title row (22 px) stays
 *         clear; the table and the projection label keep clear of the Dealer Profile header (top right) / banner (top left, when on) and
 *         the Tape Reader box (its published LAYOUT box, else its Position region, only when it runs on this market); the version
 *         next to the RTH title. Settings made real (his 10-13 rule): Position + a checkbox and a colour for every drawn element - Show
 *         table / Table background, Show projection / Up + Down colours, Show 1st-extreme mark / LOD + HOD colours (parameter version 4:
 *         IRT resets the saved list once, so the dialog shows Top centre).
 *  2.0.4 (2026-10-13): the projection callout ("HOD after 10:45 . 94.34") keeps out of the left level-label band (the Liquidity
 *         column's level names, e.g. "/PDH") - max(110 px, 15% of the pane); it moves right of the band, never over a level label.
 *  2.0.3 (2026-10-13 10:38): the table 33% smaller ("reduce the size of this by 33%. too big"): 9 pt instead of 13, padding x 2/3.
 *  2.4.4 (2026-10-18, Rassul approved the mockup 17:13 CT "build"; the table was ~790 px "way too wide"): the COMPACT table, columns
 *         sized to their text (~350 px): header "ES M2 O0 V2" + "Neg . S.Dn" (regime / trend colours as before); "1st / 2nd" headings;
 *         Exp (blue) / Act / Est rows; grey footer "Exp 3:00 PM . v2.4.4". The Gex S/R moves to its OWN one-line strip ABOVE the table
 *         (same width, 4 px gap, moves with it): green R + the levels above price, red S + the levels below; a strip wider than the
 *         table widens the table. Removed words (RTH hours, expiry countdown, full regime / trend words, the current price) are in the
 *         hovers (header row, strip). Publishes table + strip (LRA_HLB_) and the table alone (LRA_HLT_, the Big Lot box's height).
 *         Offset X / Y removed from Settings (the 9-spot Position only; parameter version 7 - his saved offsets were 0).
 *  2.0.2 (2026-10-13): Rassul's table redesign (mockups a-u): Regime / RTH / Expiry header aligned to the columns, Reclaim column gone,
 *         Expected (blue) / Actual ("LOD IN 8:39 (71%)") / Estimate rows, odds in (), bigger fonts, no 1st-extreme line, projection label on
 *         its arrowhead; Session Info retired (its Regime + Expiry are read here from LRA-Session-<MKT>.csv)
 *  2.0.1 (2026-10-13): audit fixes - RTH-completion and tick-admission checks (external audit 2026-10-12, HodLod 1.1.1)
 *  2.0.0 (2026-10-12 night): the 1st Extreme model (was lsFirstExtreme) merged in: a 3rd table line + the LOD? / HOD? marker, same shadow logs
 *
 *  THE SESSION'S HOD / LOD, on every market's chart (Rassul 2026-10-09, mockup v5 approved: "check everything then build it").
 *    PIECE 1  the session candle on the LEFT edge of the price pane, at real prices: body RTH open -> last close, wicks to the HOD
 *             and LOD, green when close >= open. Above / below: HOD and LOD with their time, the key level each extreme swept
 *             (PFH / PDH / ONH / LonHI ...), took / size for the first extreme and the gap for the second; inside the body the gap,
 *             the range in points and $, and at the open's edge the reclaim time and the Rec. Took (1st extreme -> reclaim). "pending" until the second
 *             extreme prints.
 *    PIECE 2  the table (mockup v15), bottom, centred-left and clear of the candle: "ES  10:00" + the read ("LOD IN  (model in
 *             test)  -  expect HOD after 11:00 83%"), then 1st Time | Reclaim | 2nd Time for E (expected, grey) and A (actual):
 *             E "8:51 +-30m  either" | "8:51 - 9:21  76%" | "after 11:00 83%  HOD";  A "8:51 LOD  in" | "9:09  in" | "pending".
 *             Every other value (took, size, rec. took, gap, room, range, close, the hle 2.0 model outputs) is in the status file.
 *  Every definition is in HodLodLogic.h (tested against an independent Python reference on every bar of 2026-10-09 ES).
 *  Native: the chart's own closed bars + two generated headers (HodLodExpected.h = the model candles, HodLodReadTables.h = the
 *  read tables, built in when present), HodLodExpectedV2.h (hle 2.0, the studies' conditional model) and HodLodMedians.h (the
 *  typical timing). No file is read at run time. No settings (one invisible output, OUTPUT_NO_UI).
 *  Writes: HodLod.status-<MKT>-<secs>.txt (every value drawn), HodLod.reads-<MKT>-<secs>.csv (the read every closed RTH bar +
 *  the day's OUTCOME line) and, on 3-min charts with the read built in, HodLodRead.reads-<MKT>.csv (the nightly scorer's format).
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>                 // (2.4.0) BEFORE irtsdk.h (MSVC C2371 RTBOOL clash otherwise): MarketState.h -> BannerBoard.h needs it
#endif
#include "irtsdk.h"
#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "HostSlot.h"
#include "BoxPlace.h"                // (2.0.6) Offset X / Y rules, hysteresis, the dialog snapshot
#include "HodLodLogic.h"
#include "HodLodSettings.h"         // (2.4.2) the ONE settings table (SettingsSpec.h)
#include "DealerProfileSettings.h"   // (2.4.3) the Dealer Profile banner key (its keyed per-market file)
#include "SettingsHost.h"           // (2.4.2) registration / window fill / read / group masters from the table
#include "HodLodGex.h"             // (2.1.0) the GEX line: LRA-GexNodes-<MKT>.csv parse / pick / hover text (SDK-free)
#include "HodLodQ.h"               // (2.3.0) MenthorQ Q-Scores: LRA-QScore-<MKT>.csv parse / header parts / hover (SDK-free)
// (2.4.6, Rassul 2026-10-18 20:28 "remove the hoverover on gext strikes and the hod lod box") both hovers (Gex S/R strip, header) off
static const bool HL_HOVERS = false;
// (2.6.0, Rassul 15:57 CT, the agreed PRE-OPEN BOOK design: "a hover on the line") the ONE hover on this box: the Pre row's detail
static const bool HL_PRE_HOVER = true;
#include "HodLodFx.h"            // (2.0.0) the 1st Extreme model, merged in (was the separate lsFirstExtreme)
#include "Fresh.h"               // (2.4.0) the one freshness / STALE rule (Reg / Q / Gex row / Tape Reader status)
#include "MarketState.h"         // (2.4.0) SRC HODLOD + TREND, once per closed bar
#include "BoxDock.h"             // (2.4.1) the table rectangle for the Big Lot box (lsBigBidAsk) to dock to
#include <chrono>
#include "PreOpenLogic.h"        // (2.6.0) the PRE-OPEN BOOK row (SDK-free)
#include "DayTypeLogic.h"        // (2.7.0) the day type in the header trend (SDK-free, price only)
#include "MarkParams.h"          // (2.7.0) per-market SHOW / LOG params (LRA-PreOpen / LRA-DayType-<MKT>.csv)
#include "NewsWindow.h"          // (2.7.0) the News / settle window and the Feed ? rows
#include <map>
#include "VwapTrendLogic.h"      // (2.0.5) the header's trend: the Tape Reader TREND vote's rule, computed here from the chart's closed bars
#include <sys/stat.h>
#include <fstream>
#include <sys/types.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <new>
#include <cstdint>
#include <cmath>

static const COLOR HL_GREY = 0x009CA3AF;

// ---- (2.6.0) the PRE-OPEN BOOK's order feed: ONE DEPTH_BY_ORDER per chart, open only while the pre-open read needs it (45 min before
// the open to 4 min after it; then deleted - its destructor unsubscribes - once a day), opened from calc / draw only behind a crash-guard
// file, read by a 250 ms clock (named symbol only) and by calc / draw, at most 20,000 events / ~15 ms per read. The lsMboCheck 1.1.0 /
// OrderFlowFeed.h off::Conn pattern, COPIED (not included) so this box never depends on the order-flow panes' header (BigLotTiers.h rule).
namespace hlpo {
template <class Tag, typename Tag::type M> struct Rob { friend typename Tag::type robbed(Tag) { return M; } };
struct DboFlag { typedef RTBOOL cppExtension::DEPTH_BY_ORDER::*type; friend type robbed(DboFlag); };
template struct Rob<DboFlag, &cppExtension::DEPTH_BY_ORDER::bDboSubscribed>;
inline int safeNext(cppExtension::DEPTH_BY_ORDER* d) {   // 1 = an event, 0 = none, -1 = the native call faulted (never re-entered)
#if defined(_MSC_VER)
    __try { return d->getNext() == RTX_OK ? 1 : 0; } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#else
    try { return d->getNext() == RTX_OK ? 1 : 0; } catch (...) { return -1; }
#endif
}
static const long long TIMER_MS = 250, BUDGET_MS = 15, WAIT_MS = 60000; static const int READ_MAX = 20000;
struct Feed {
    cppExtension::DEPTH_BY_ORDER* dbo = nullptr;
    int attempt = 0, flag = -1; bool blocked = false, dead = false, named = false;
    long long openMs = -1, lastEventMs = -1, events = 0, faults = 0, capHits = 0, opens = 0; std::string via;
    Feed() {}
    Feed(const Feed&) = delete; Feed& operator=(const Feed&) = delete;
    ~Feed() { delete dbo; }
    void close() { delete dbo; dbo = nullptr; attempt = 0; flag = -1; dead = false; named = false; openMs = lastEventMs = -1; via.clear(); }
    // chart context only: 1st the chart symbol spelled out; after 60 s with no event one 2nd try with NULL (= this chart)
    void open(const std::string& symbol, const std::string& guard, long long now, std::string& state) {
        if (blocked || dead || attempt >= 2) return;
        if (attempt == 1 && !(lastEventMs < 0 && openMs >= 0 && now - openMs >= WAIT_MS)) return;
        if (attempt == 0 && !guard.empty()) { std::ifstream g(guard.c_str()); if (g.is_open()) { blocked = true; state = "IRT closed while opening the order feed last time: delete " + guard + " to retry"; return; } }
        if (!guard.empty()) { std::ofstream g(guard.c_str(), std::ios::trunc); g << "opening DEPTH_BY_ORDER try " << (attempt + 1) << "\n"; }
        try {
            delete dbo; dbo = nullptr;
            if (attempt == 0 && !symbol.empty()) { dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(symbol.c_str()); via = "chart symbol " + symbol; named = true; attempt = 1; }
            else { dbo = new (std::nothrow) cppExtension::DEPTH_BY_ORDER(NULL); via = "default (NULL = this chart)"; named = false; attempt = 2; }
            ++opens;
        } catch (...) { faults++; dead = true; }
        flag = -1; openMs = now;
        if (!dbo && !dead) { dead = true; state = "order feed unavailable (out of memory)"; }
        if (!guard.empty()) std::remove(guard.c_str());
    }
    template <class Clock, class Sink> int read(long long now, Clock clock, Sink sink, bool timerPath) {
        if (!dbo || dead || (timerPath && (!named || flag != 1))) return 0;
        const long long t0 = clock(); int n = 0;
        for (;;) {
            if (n >= READ_MAX || ((n & 511) == 511 && clock() - t0 > BUDGET_MS)) { capHits++; break; }
            const int r = safeNext(dbo);
            if (r < 0) { faults++; dead = true; break; }
            if (r == 0) break;
            ++n; sink(dbo->dbo);
        }
        if (!dead) { try { flag = (dbo->*robbed(DboFlag())) ? 1 : 0; } catch (...) { faults++; dead = true; } }
        if (n > 0) { lastEventMs = now; events += n; }
        return n;
    }
    std::string stateText(long long now) const {
        if (blocked) return "blocked (crash guard)";
        if (dead) return "order feed fault: reload the indicator";
        if (!dbo) return "closed";
        if (flag == 0) return "MBO: not subscribed";
        if (lastEventMs < 0) return now - openMs >= WAIT_MS && attempt >= 2 ? "MBO: no events" : "MBO: waiting";
        return "ready";
    }
};
} // namespace hlpo

struct HLState {
    std::string root, mkt; int mi = -1;
    long nBars = -1; RTDATE lastStamp = 0, firstStamp = 0; float lastTick = 0;
    long long per = 0; double tick = 0;
    bool have = false;                 // a view was built
    hl::Tracker* T = nullptr;          // the last tracker (owned)
    hl::View view; hl::ReadOut rd;
    std::string state = "starting";
    hl::LogState mine, scorer; bool firstPass = true;
    time_t lastStatusWrite = 0; std::string lastStatus;
    bool faulted = false;
    long long recomputes = 0, statusWrites = 0;   // test counters (checklist #31 / #32)
    int projIdx = -1;                  // (1.1.0) the chart bar index of the 1st extreme (the projection line's x), -1 = none
    // (2.0.0) the 1st Extreme model (fx2), 3-min charts only: its view, the chart index of the called candle, its shadow logs
    fx2::View fxView; bool fxHave = false; std::string fxState = "not run"; int fxIdx = -1;
    long long fxLogDay = -1, fxCallDay = -1; int fxLogElapsed = -1; time_t fxStatusWrite = 0; std::string fxLastStatus;
    std::string fcSession;                 // (1.1.8) the session whose first IN call is already in HodLod.firstcall-<MKT>.csv
    std::string projLogged, projSession;   // (1.1.6) the targets already in HodLod.proj-<MKT>.csv for projSession (\n-separated keys)
    // (HL104) MenthorQ level snapshots of MenthorQLevels.csv (the one external input): one per distinct content, <= 32, newest last
    std::vector<hl::MqSnap> mq; long long mqStamp = -2; time_t mqChecked = 0; std::string mqWhy; long long mqLoads = 0;
    // (2.0.2, Rassul 05:52 / 06:08: Session Info retired, its Regime + options Expiry move into the table's header) the bridge's
    // LRA-Session-<MKT>.csv: GAM|POS|NEG ... and EXP|hh:mm ... (bounded read, re-read only when the file changes, at most every 5 s)
    long long sessStamp = -2; time_t sessChecked = 0; std::string sessGam, sessExp; int sessAge = -1; bool sessRec = true;
    // (2.0.5) other indicators' boxes on this pane: the Tape Reader's published box (TapeReader.status-<MKT>.txt LAYOUT), its Position
    // when that is stale, and whether the Dealer Profile banner is on (DealerProfile.settings.txt); re-read at most every 3 s
    time_t avoidChecked = 0; bool trBoxOk = false; hl::Box trBox = { 0, 0, 0, 0 }; int trPos = 0; bool trSeen = false; bool dpBanner = false;
    // (2.1.0) the GEX line: LRA-GexNodes-<MKT>.csv (re-read only when its stamp changes, at most every 5 s) and the row's hover
    long long gexStamp = -2; time_t gexChecked = 0; hlg::File gex; std::string gexWhy; long long gexLoads = 0;
    bool inGex = false; bool lastGexOk = false, lastHoverOk = false; hl::Box lastGexRow = { 0, 0, 0, 0 }, lastGexHover = { 0, 0, 0, 0 };
    int gexCurX = -100000, gexCurY = -100000; long long gexHovers = 0;
    // (2.3.0) the Q-Scores: LRA-QScore-<MKT>.csv (stamp, at most every 5 s) and their hover
    long long qStamp = -2; time_t qChecked = 0; hlq::File qf; std::string qWhy;
    bool inQ = false, lastQOk = false, lastQHoverOk = false; hl::Box lastQBox = { 0, 0, 0, 0 }, lastQHover = { 0, 0, 0, 0 }; int qCurX = -100000, qCurY = -100000; long long qHovers = 0;
    // (2.4.0) the regime's ONE source: GammaProfile-MQ-<MKT>.csv HVL0 (stamp, at most every 5 s) and its verdict (Fresh.h)
    long long hvlStamp = -2; time_t hvlChecked = 0; hlg::HvlBook hvl; std::string hvlWhy; int regSign = 0; bool regFromGex = false; double regGexHvl = 0;   /* (2.5.0) the regime from the Gex S/R file HVL0 */ fr::Verdict regV; fr::Verdict qV; bool qJudged = false;
    long long lastClosedAbs = 0; double lastClose = NAN;     // the last closed bar (CT wall-clock seconds of its END stamp, its close)
    // (2.4.0) the Market State posts: this chart's identity, the facts decided for the last closed bar, when they were last posted
    std::string hlbSym, hlbChart; long hlbSpb = 0; bool hlbPosted = false;   // (2.4.1) the published table rectangle (BoxDock.h)
    std::string mstSym, mstChart; long mstSpb = 0; bool mstPosted = false; long long mstBar = -1, mstMs = -1; std::string mstSig;
    std::map<std::string, std::string> mstKv, mstTrendKv;
    bp::Hyst hyst; long long hystPlaceSig = -3; std::string placeLine; hl::Box hystPane = { 0, 0, 0, 0 };                           // (2.0.6) the table's place: no ping-pong (reset when the settings change)
    // (2.6.0) the PRE-OPEN BOOK: the engine of today's open, its order feed (+ 250 ms clock), the record, the row's hover
    po::Engine pre; hlpo::Feed preFeed; int preTimerId = 0; bool preTimerOn = false, preTimerTried = false, preBusy = false;
    long long preDay = -1; bool preNearFrozen = false; bool preSnapWritten = false, preImbWritten = false; long long preWrites = 0, preWriteFails = 0, preRestored = 0;
    RTDATE preStampKey = 0; long long preStampMs = 0; long long preRouted = 0; std::string preState = "not started"; double preTick = 0; std::string preSym; long prePer = 0;
    po::Line preLine; long long preLineMs = -100000; bool preShow = true; int preMin = 15;
    bool inPre = false, lastPreOk = false, lastPreHoverOk = false; hl::Box lastPreRow = { 0, 0, 0, 0 }, lastPreHover = { 0, 0, 0, 0 }; int preCurX = -100000, preCurY = -100000; long long preHovers = 0;
    // (2.7.0) the hovered row: 0 = Pre, 1 = the day type, 2 + k = note row k; the drawn targets (kind, box)
    int hovWhich = 0; std::vector<std::pair<int, hl::Box> > lastTargets;
    dtl::Result day; dtl::Params dayP; std::string dayLine = "DAYTYPE,not run", dayParamWhy, newsLine = "NEWS,none", preParamWhy; int dayLogged = -99; std::string dayLoggedKey; long long dayLogs = 0, dayLogFails = 0; int preParamShow = 0;
    ~HLState() { delete T; }
};

class HodLod : public cppExtension {
public:
    SS_SAFE_DIALOG(HodLod, "HodLod")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    HodLod() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);                                 // (2.1.0) the GEX row's hover; (2.6.0) the Pre row's hover
    virtual int timer(RTX_EVENT* e);                                 // (2.6.0) the pre-open order feed's 250 ms clock
    void preTick(HLState& S);                                        // (2.6.0) the PRE-OPEN BOOK: feed, engine, record (calc / draw)
    void preRoute(HLState& S, const DEPTH_ORDER& o);
    void preRestore(HLState& S);
    void preStopTimer(HLState& S) { if (S.preTimerOn) { destroyTimer(S.preTimerId); S.preTimerOn = false; } if (S.preTimerId) preTimers().erase(S.preTimerId); S.preTimerId = 0; }
    static std::map<int, HLState*>& preTimers() { static std::map<int, HLState*> m; return m; }
    void loadGex(HLState& S);
    void loadQ(HLState& S);                                          // (2.3.0)
    // (HL105, Rassul "you should have setting where to put them like upper left etc.") ONE setting: Position (9 spots), default
    // Bottom centre. IRT can return -1 for a LIST parameter outside its settings copy, so the choice is also kept per market in
    // HodLod.place-<MKT>.txt - written only from the dialog's callbacks (loading never saves, #29) and read back by stamp.
    virtual int parmsLoadBody(void);                                     // (2.0.6) fills the dialog from the place file (never saves)
    virtual int parmsApplyBody(void) { savePos(); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::onUpdate(this, hlset::spec(), hlIds(), (int)i, snap_); savePos(); return RTX_OK; }   // (2.4.2) a show box greys its colours
    static ss::Ids& hlIds() { static ss::Ids I; return I; }        // (2.4.2) the row ids IRT gave the table (one DLL = one parameter list, written once in setup)
    // (2.0.5) the checkboxes and colours (indices into the parameter list, set in setup); (2.0.6) after Offset X / Y
    ss::Snap snap_;                                                    // (2.0.6) the dialog as it opened (2.4.2: SettingsSpec.h)
    struct SetStore { std::string mkt; ss::Store st; };
    std::vector<SetStore> stores_;                                     // (2.4.2) per market, per object (no static chart state)
    ss::Store& storeFor(const std::string& mkt);
    // (2.0.6) the per-market settings: "pos,dx,dy,table,proj,mark,tableBg,projUp,projDn,markLod,markHod" (colours 0..0xFFFFFF, -1 =
    // default); a 2.0.5 file ("7") = that position with everything else default
    struct Place { int pos = hl::POS_DEFAULT; hl::Opts o; long long col[5] = { -1, -1, -1, -1, -1 }; std::string why; long long sig = -3; bool preShow = true; int preMin = 15; };   // (2.6.0) + the pre-open line
    Place readPlace(const std::string& mkt);
    static Place toPlace(const ss::Values& V);
    hl::Opts opts(const Place& P);
    void savePos();
    Place position(const std::string& mkt);
    std::vector<std::string> migChecked;                 // (2.0.5) markets whose place file was checked for the old default this run
    void migratePlace(const std::string& mkt);
    void loadAvoid(HLState& S);
    int priceEdge();
    HostSlot<HLState> slot_;
    HLState* state(bool create) { return slot_.get(this, create); }
    void clearState()
    {   // (2.4.0) this chart's Market State entries leave the board (removal, symbol / period change), then the state goes
        HLState* S = slot_.get(this, false);
        if (S && S->hlbPosted) { bdk::clearHodLod(S->hlbSym, S->hlbSpb, S->hlbChart); bdk::clearHodLodTable(S->hlbSym, S->hlbSpb, S->hlbChart); S->hlbPosted = false; }   // (2.4.1)
        if (S && S->mstPosted) { mst::clear("HODLOD", S->mstSym, S->mstSpb, S->mstChart); mst::clear("TREND", S->mstSym, S->mstSpb, S->mstChart); S->mstPosted = false; }
        if (S) { preStopTimer(*S); S->preFeed.close(); }            // (2.6.0) the clock stops before the state goes; the feed unsubscribes
        slot_.release(this);
    }
    void refresh(HLState& S);
    bool loadMq(HLState& S);
    void loadSession(HLState& S);
    void loadHvl(HLState& S);                                        // (2.4.0) the regime's HVL0 book
    void postMst(HLState& S);                                        // (2.4.0) the Market State
    void recompute(HLState& S, long n);
    void writeFiles(HLState& S, const std::vector<hl::ReadRow>& rows);
    void writeProj(HLState& S);
    void writeFirstCall(HLState& S, const std::vector<hl::ReadRow>& rows);
    void writeStatus(HLState& S, const hl::Layout* L);
    void computeFx(HLState& S, long n, long long per);
    void writeFxLogs(HLState& S);
    void writeFxStatus(HLState& S);
    std::string flexPath(const std::string& name);
    int textW(const std::string& s, int sz, bool bold) { FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); return (int)getTextWidth(s.c_str(), -1); }
    void text(int x, int yc, const std::string& s, COLOR col, int sz, bool bold)
    {   // as SessionInfo: the text's vertical centre on yc
        FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f); setTextColor(col);
        short yy = (short)(yc - (short)(sz * 0.45f + 0.5f)); int w = (int)getTextWidth(s.c_str(), -1);
        RCT rc; rc.set((short)x, (short)(yy - sz), (short)(x + w + 4), (short)(yy + sz)); rc.drawText(s.c_str(), false, false);
    }
    void line(int x1, int y1, int x2, int y2, COLOR c, int w)
    {
        setPen(c, (short)w, P_SOLID);
        PNT a; a.set(0, 0.0f); a.h = (short)x1; a.v = (short)y1; a.setDrawPosition();
        PNT b; b.set(0, 0.0f); b.h = (short)x2; b.v = (short)y2; b.drawLineTo();
    }
    void execute(const hl::Layout& L);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<HodLod*>(this)->clearState(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<HodLod*>(this)->clearState(); return RTX_OK; }
int cppExtension::calc(int)
{
    HodLod* x = static_cast<HodLod*>(this);
    HLState* S = x->state(true);
    if (S) x->refresh(*S);
    if (S && S->mi >= 0) x->preTick(*S);                               // (2.6.0) the pre-open book's feed is read on every calc too
    return RTX_OK;
}

int cppExtension::setup(void)
{
    // (HL105) Position (9 spots; default Bottom centre since 2.0.6); (2.0.5, Rassul 2026-10-13 "settings must be real") a checkbox and a
    // colour for every drawn element; (2.0.6) Offset X / Y. Controls only, no how-to text. The value array stays hidden from the settings
    // window. The chart reads HodLod.place-<MKT>.txt only (IRT hands the chart copy every box unchecked - why the table vanished in 2.0.5).
    // (2.4.2, settings audit 2026-10-18) every row from hlset::spec(): colours filled / read with setParameterColor / getParameterColor
    // (2.0.5 - 2.4.1 used setIntegerValue / getIntegerValue on the colour rows: the swatches showed BLACK and a colour he picked was never
    // saved); a Show box greys its colours; projection labels shortened so the row fits the window (~400 px); parameter version 6.
    ss::registerAll(this, hlset::spec(), HodLod::hlIds());
    CPEN p(HL_GREY, 1, P_SOLID);
    setOutputParameter("HOD/LOD", DRAW_INVISIBLE, &p, HL_GREY, OUTPUT_NO_UI);
    setArrayDrawingFlags(0, (DRAW_FLAGS)(CONNECT_CNONZERO | NO_AUTOSCALE));
    return RTX_OK;
}

// (2.0.6) the chart's options come from the place file only (the chart copy's checkboxes always read unchecked in IRT)
hl::Opts HodLod::opts(const Place& P)
{
    hl::Opts O = P.o;
    O.tableBg = P.col[0] < 0 ? hl::C_PANEL : hl::optCol((unsigned)P.col[0], hl::C_PANEL); O.projUp = P.col[1] < 0 ? hl::C_GREEN : hl::optCol((unsigned)P.col[1], hl::C_GREEN);
    O.projDn = P.col[2] < 0 ? hl::C_RED : hl::optCol((unsigned)P.col[2], hl::C_RED); O.markLod = P.col[3] < 0 ? hl::C_GREEN : hl::optCol((unsigned)P.col[3], hl::C_GREEN);
    O.markHod = P.col[4] < 0 ? hl::C_RED : hl::optCol((unsigned)P.col[4], hl::C_RED);
    return O;
}
static bool hlColTok(const std::string& t, long long& v) { if (t == "-1") { v = -1; return true; } if (t.empty() || t.size() > 8) return false; for (char c : t) if (c < '0' || c > '9') return false; v = std::strtoll(t.c_str(), nullptr, 10); return v <= 0xFFFFFF; }
static bool hlParsePlace(const std::string& lnIn, HodLod::Place& P)
{
    std::string ln = lnIn; while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
    bp::Place pl; std::vector<std::string> rest; std::string why;
    if (!bp::parse(ln, hl::POS_DEFAULT, pl, &why, &rest)) { P = HodLod::Place(); P.why = "place file ignored (" + why + "): defaults"; return false; }
    HodLod::Place Q; Q.pos = pl.pos; Q.o.dx = pl.dx; Q.o.dy = pl.dy;
    if (!rest.empty()) {
        if (rest.size() != 8) { P = HodLod::Place(); P.pos = pl.pos; P.o.dx = pl.dx; P.o.dy = pl.dy; P.why = "options fields malformed: options default"; return false; }
        for (int k = 0; k < 3; ++k) if (rest[(size_t)k] != "0" && rest[(size_t)k] != "1") { P = HodLod::Place(); P.pos = pl.pos; P.o.dx = pl.dx; P.o.dy = pl.dy; P.why = "options fields malformed: options default"; return false; }
        Q.o.table = rest[0] == "1"; Q.o.proj = rest[1] == "1"; Q.o.mark = rest[2] == "1";
        for (int k = 0; k < 5; ++k) if (!hlColTok(rest[(size_t)(3 + k)], Q.col[k])) { P = HodLod::Place(); P.pos = pl.pos; P.o.dx = pl.dx; P.o.dy = pl.dy; P.why = "a colour field malformed: options default"; return false; }
    }
    P = Q; return true;
}
static std::string hlFormatPlace(const HodLod::Place& P)
{
    bp::Place pl; pl.pos = P.pos; pl.dx = P.o.dx; pl.dy = P.o.dy;
    std::string o = bp::format(pl) + "," + (P.o.table ? "1" : "0") + "," + (P.o.proj ? "1" : "0") + "," + (P.o.mark ? "1" : "0");
    for (int k = 0; k < 5; ++k) o += "," + std::to_string(P.col[k] < 0 || P.col[k] > 0xFFFFFF ? -1 : P.col[k]);
    return o;
}

std::string HodLod::flexPath(const std::string& name)
{
    const char* up = getenv("USERPROFILE");
    if (!up) return "";
#ifdef _WIN32
    return std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\" + name;
#else
    return std::string(up) + "/" + name;     // mock tests on Linux
#endif
}

// the chart -> the market; a symbol change resets everything for this chart (checklist #1 / #30)
void HodLod::refresh(HLState& S)
{
    char rb[32] = { 0 }; const char* rs = getRootSymbol(rb);
    std::string root = rs ? rs : "";
    if (root != S.root) {
        delete S.T; S.T = nullptr;
        S.root = root; S.mkt = hl::marketForRoot(root); S.mi = hl::mktIndex(S.mkt);
        S.mq.clear(); S.mqStamp = -2; S.mqChecked = 0; S.mqWhy.clear();
        S.nBars = -1; S.have = false; S.faulted = false; S.mine = hl::LogState(); S.scorer = hl::LogState(); S.firstPass = true;
        S.state = S.mi < 0 ? "unknown market (root " + root + ")" : "starting";
    }
    if (S.mi < 0 || S.faulted) return;
    if (loadMq(S)) S.nBars = -1;            // (HL104) new MenthorQ levels: rebuild once
    long n = getBarCount();
    if (n < 3) { S.state = "too few bars"; S.have = false; return; }
    RTARRAYI dt(barDateTime);
    if ((long)dt.count < n) { S.state = "timestamp array unavailable"; S.have = false; return; }
    RTDATE last = (RTDATE)dt[(int)n - 1], first = (RTDATE)dt[0];
    float tk = getProperty(SYM_TICKINCR);
    // recompute only when the bars or the tick change (a new bar = the forming bar closed); drawing reuses the result (#32)
    if (n == S.nBars && last == S.lastStamp && first == S.firstStamp && tk == S.lastTick) return;
    S.nBars = n; S.lastStamp = last; S.firstStamp = first; S.lastTick = tk;
    try { recompute(S, n); }
    catch (const std::exception& e) { S.faulted = true; S.have = false; S.state = std::string("fault: ") + e.what() + " (cleared on symbol change)"; }
    catch (...) { S.faulted = true; S.have = false; S.state = "fault (cleared on symbol change)"; }
}

// (HL104) MenthorQLevels.csv (lsFlexLevels, written by the MenthorQ bridge, read by lsDealerProfile too): stat at most every 5 s,
// read only when the stamp changes, at most 64 KiB, strict parse; a changed level set becomes a new snapshot stamped with the file's
// write time (local CT). Missing / oversized / garbage -> no new snapshot (a stale one from an earlier session is never used).
static long long hlLocalAbs(time_t t)
{
    struct tm lt; memset(&lt, 0, sizeof lt);
#ifdef _WIN32
    if (localtime_s(&lt, &t) != 0) return 0;
#else
    if (!localtime_r(&t, &lt)) return 0;
#endif
    return hl::daysFromCivil(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday) * 86400 + lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec;
}
bool HodLod::loadMq(HLState& S)
{
    time_t now = time(nullptr);
    if (S.mqStamp != -2 && now - S.mqChecked < 5) return false;
    S.mqChecked = now;
    std::string p = flexPath("MenthorQLevels.csv");
    if (p.empty()) { S.mqWhy = "MenthorQ levels: no profile folder"; return false; }
#ifdef _WIN32
    struct _stat64 st; bool ok = _stat64(p.c_str(), &st) == 0;
#else
    struct stat st; bool ok = stat(p.c_str(), &st) == 0;
#endif
    long long stamp = ok ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
    if (stamp == S.mqStamp) return false;
    S.mqStamp = stamp;
    if (!ok) { S.mqWhy = "MenthorQLevels.csv missing: key levels only"; return true; }
    if ((long long)st.st_size > (long long)hl::MQ_MAX_BYTES) { S.mqWhy = "MenthorQLevels.csv over 64 KiB: ignored"; return true; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) { S.mqWhy = "MenthorQLevels.csv unreadable"; return true; }
        text.resize(hl::MQ_MAX_BYTES + 1); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); }
    S.mqLoads++;
    std::vector<hl::MqLevel> lv; std::string why;
    double tick = (double)getProperty(SYM_TICKINCR); tick = std::round(tick * 1e9) / 1e9;
    if (!hl::parseMq(text, S.mkt, S.root, tick, lv, why)) { S.mqWhy = why + ": key levels only"; return true; }
    S.mqWhy = why;
    bool same = !S.mq.empty() && S.mq.back().lv.size() == lv.size();
    for (size_t i = 0; same && i < lv.size(); i++) same = lv[i].code == S.mq.back().lv[i].code && lv[i].px == S.mq.back().lv[i].px && lv[i].col == S.mq.back().lv[i].col;
    if (same) return false;
    hl::MqSnap sn; sn.atAbs = hlLocalAbs((time_t)st.st_mtime); sn.lv = lv;
    S.mq.push_back(sn);
    if (S.mq.size() > 32) S.mq.erase(S.mq.begin());
    return true;
}

// (2.1.0) the screen cursor (Win32 GetCursorPos, user32 - pragma below); the hover closes when the cursor has moved away from where
// the last mouse event over this pane put it (IRT sends mouse events only for the pane under the mouse)
#if defined(_WIN32) && !defined(HL_TEST_CURSOR)
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")
#endif
static bool hlCursor(int& x, int& y) { POINT p; if (!GetCursorPos(&p)) return false; x = (int)p.x; y = (int)p.y; return true; }   // (2.4.0) windows.h is included now (was a local declaration)
#else
static int hl_test_cx = 0, hl_test_cy = 0;
static bool hlCursor(int& x, int& y) { x = hl_test_cx; y = hl_test_cy; return true; }
#endif
void HodLod::loadGex(HLState& S)
{
    time_t now = time(nullptr);
    if (S.gexStamp != -2 && now - S.gexChecked < 5) return;
    S.gexChecked = now;
    std::string p = flexPath("LRA-GexNodes-" + S.mkt + ".csv");
    if (p.empty()) { S.gex = hlg::File(); S.gexWhy = "no profile folder"; S.gexStamp = -1; return; }
#ifdef _WIN32
    struct _stat64 st; bool ok = _stat64(p.c_str(), &st) == 0;
#else
    struct stat st; bool ok = stat(p.c_str(), &st) == 0;
#endif
    long long stamp = ok ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
    if (stamp == S.gexStamp) return;
    S.gexStamp = stamp;
    if (!ok) { S.gex = hlg::File(); S.gexWhy = "LRA-GexNodes-" + S.mkt + ".csv missing: no GEX line"; return; }
    if ((long long)st.st_size > (long long)hlg::MAX_BYTES) { S.gex = hlg::File(); S.gexWhy = "over 64 KiB: ignored"; return; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) { S.gexStamp = -2; S.gexWhy = "unreadable (kept the last copy)"; return; }
        text.resize(hlg::MAX_BYTES + 1); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); }
    hlg::File F = hlg::parse(text, S.mkt);
    S.gexLoads++;
    if (!F.ok) { S.gexWhy = "garbled (" + F.why + "): kept the last good copy"; S.gexStamp = -2; return; }   // a torn write: re-read next time
    S.gex = F; S.gexWhy = F.why;
}

// (2.3.0) MenthorQ Q-Scores, written by the bridge once a day (lra.gex_nodes.qscore_flex); same bounded, stamp-checked read as the GEX file
void HodLod::loadQ(HLState& S)
{
    time_t now = time(nullptr);
    if (S.qStamp != -2 && now - S.qChecked < 5) return;
    S.qChecked = now;
    std::string p = flexPath("LRA-QScore-" + S.mkt + ".csv");
    if (p.empty()) { S.qf = hlq::File(); S.qWhy = "no profile folder"; S.qStamp = -1; return; }
#ifdef _WIN32
    struct _stat64 st; bool ok = _stat64(p.c_str(), &st) == 0;
#else
    struct stat st; bool ok = stat(p.c_str(), &st) == 0;
#endif
    long long stamp = ok ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
    if (stamp == S.qStamp) return;
    S.qStamp = stamp;
    if (!ok) { S.qf = hlq::File(); S.qWhy = "LRA-QScore-" + S.mkt + ".csv missing: no Q-Scores"; return; }
    if ((long long)st.st_size > (long long)hlq::MAX_BYTES) { S.qf = hlq::File(); S.qWhy = "over 8 KiB: ignored"; return; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) { S.qStamp = -2; S.qWhy = "unreadable (kept the last copy)"; return; }
        text.resize(hlq::MAX_BYTES + 1); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); }
    hlq::File F = hlq::parse(text, S.mkt);
    if (!F.ok) { S.qWhy = "garbled (" + F.why + "): kept the last good copy"; S.qStamp = -2; return; }
    S.qf = F; S.qWhy.clear();
}

void HodLod::loadSession(HLState& S)
{
    time_t now = time(nullptr);
    if (S.sessStamp != -2 && now - S.sessChecked < 5) return;
    S.sessChecked = now;
    std::string p = flexPath("LRA-Session-" + S.mkt + ".csv");
    if (p.empty()) { S.sessGam.clear(); S.sessExp.clear(); S.sessStamp = -1; return; }
#ifdef _WIN32
    struct _stat64 st; bool ok = _stat64(p.c_str(), &st) == 0;
#else
    struct stat st; bool ok = stat(p.c_str(), &st) == 0;
#endif
    long long stamp = ok ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
    if (stamp == S.sessStamp) return;
    S.sessStamp = stamp; S.sessGam.clear(); S.sessExp.clear(); S.sessAge = -1; S.sessRec = true;
    if (!ok || (long long)st.st_size > 64 * 1024) return;
    std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) { S.sessStamp = -2; return; }
    std::string line; int nl = 0;
    while (nl++ < 400 && std::getline(f, line)) {
        if (line.size() > 512) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> c; size_t a = 0;
        for (;;) { size_t b = line.find('|', a); c.push_back(line.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
        if (c.size() >= 2 && c[0] == "GAM" && (c[1] == "POS" || c[1] == "NEG")) S.sessGam = c[1];
        else if (c.size() >= 2 && c[0] == "EXP" && c[1].size() == 5 && c[1][2] == ':') S.sessExp = c[1];
        else if (c.size() >= 3 && c[0] == "DAGE") { char* e = nullptr; long v = strtol(c[1].c_str(), &e, 10); if (e && *e == 0) S.sessAge = (int)v; S.sessRec = c[2].empty() || c[2] == "OK"; }
    }
}

// (2.4.0, audit C finding 8) the regime's ONE MenthorQ source: GammaProfile-MQ-<MKT>.csv (the bridge writes it with every grid; the
// HVL0 strike = the "HVL 0D" line the Dealer Profile draws from the same snapshot). Bounded read, re-read only when its stamp changes,
// at most every 5 s; a torn / garbled file keeps the last good book.
void HodLod::loadHvl(HLState& S)
{
    time_t now = time(nullptr);
    if (S.hvlStamp != -2 && now - S.hvlChecked < 5) return;
    S.hvlChecked = now;
    std::string p = flexPath("GammaProfile-MQ-" + S.mkt + ".csv");
    if (p.empty()) { S.hvl = hlg::HvlBook(); S.hvlWhy = "no profile folder"; S.hvlStamp = -1; return; }
#ifdef _WIN32
    struct _stat64 st; bool ok = _stat64(p.c_str(), &st) == 0;
#else
    struct stat st; bool ok = stat(p.c_str(), &st) == 0;
#endif
    long long stamp = ok ? (long long)st.st_mtime * 1000003LL + (long long)st.st_size : -1;
    if (stamp == S.hvlStamp) return;
    S.hvlStamp = stamp;
    if (!ok) { S.hvl = hlg::HvlBook(); S.hvlWhy = "GammaProfile-MQ-" + S.mkt + ".csv missing: no regime"; return; }
    if ((long long)st.st_size > (long long)hlg::MAX_BYTES) { S.hvl = hlg::HvlBook(); S.hvlWhy = "over 64 KiB: ignored"; return; }
    std::string text;
    {   std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) { S.hvlStamp = -2; S.hvlWhy = "unreadable (kept the last copy)"; return; }
        text.resize(hlg::MAX_BYTES + 1); f.read(&text[0], (std::streamsize)text.size()); text.resize((size_t)f.gcount()); }
    hlg::HvlBook B = hlg::parseHvl(text, S.mkt);
    if (!B.ok && S.hvl.ok && B.why != "no HVL0 strike in the book" && B.why != "book is for another market") { S.hvlWhy = "garbled (" + B.why + "): kept the last good book"; S.hvlStamp = -2; return; }
    S.hvl = B; S.hvlWhy = B.ok ? std::string() : B.why;
}
// the CT wall clock now (this PC's local time = Central, as everywhere in this plugin), as CT wall-clock seconds
static long long hlNowCt()
{
    time_t t = time(nullptr); return hlLocalAbs(t);
}

// (2.0.2) the header's Regime / Expiry segments, rebuilt every draw so the countdown stays current (CT clock of this PC)
// (2.4.0) Regime = the HVL0 book vs the last closed close (one source with the Dealer Profile), aged by Fresh.h: STALE -> "Regime: STALE"
// (grey; the header shortens it to "Reg: STALE"); no book / no HVL0 -> no regime part ("Reg: --" with a trend, as before)
static void hlSessSegs(HLState& S)
{
    S.view.sessSegs.clear();
    S.regSign = 0; S.regV = fr::Verdict();
    if (S.hvl.ok) {
        S.regV = fr::judge(fr::K_MQ_LEVELS, S.hvl.asofCt, hlNowCt());
        const int sg = hlg::regimeSign(S.hvl, S.lastClose);
        if (S.regV.stale()) S.view.sessSegs.push_back(hl::Seg{ "Regime: STALE", hl::C_GREY });
        else if (sg != 0) {
            S.regSign = sg;
            S.view.sessSegs.push_back(hl::Seg{ sg < 0 ? "Regime: Negative" : "Regime: Positive", sg < 0 ? hl::C_RED : hl::C_GREEN });
        }
    }
    if (S.sessExp.size() == 5) {
        int e = atoi(S.sessExp.substr(0, 2).c_str()) * 60 + atoi(S.sessExp.substr(3, 2).c_str());
        time_t t = time(nullptr); struct tm lt;
#ifdef _WIN32
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        int now = lt.tm_hour * 60 + lt.tm_min, left = e - now; if (now >= 17 * 60) left = e + 1440 - now;
        char b[64]; int h12 = (e / 60) % 12 == 0 ? 12 : (e / 60) % 12;
        if (left > 0) snprintf(b, sizeof b, "Expiry %d:%02d %s (in %dh %02dm)", h12, e % 60, e >= 720 ? "PM" : "AM", left / 60, left % 60);
        else snprintf(b, sizeof b, "Expiry %d:%02d %s (expired)", h12, e % 60, e >= 720 ? "PM" : "AM");
        S.view.sessSegs.push_back(hl::Seg{ b, left > 0 && left <= 60 ? hl::C_AMBER : hl::C_GREY });
    }
}

// (2.0.5) the header's trend: vtl::fromChart on the CLOSED bars (B never holds the forming bar), RTH, of the latest RTH session among
// them - the live one, or the prior one while the table shows it (overnight / before the open). Same rule and VWAP arithmetic as the
// Tape Reader TREND vote and lsSessionVWAP; a chart without volume weights every bar equally (as the timing features do).
static void hlTrend(HLState& S, const std::vector<hl::Bar>& B, long long per)
{
    S.view.trendLvl = 0; S.view.trendReady = false; S.view.trendBars = 0; S.view.trendVia.clear();
    int om = 0, cm = 0;
    if (B.empty() || per <= 0 || !svl::rthOf(S.mkt.c_str(), om, cm)) return;
    std::vector<svl::Bar> vb; vb.reserve(B.size());
    for (size_t i = 0; i < B.size(); i++) {
        const long long a = B[i].absEnd; long d = (long)(a >= 0 ? a / 86400 : (a - 86399) / 86400);
        svl::Bar x; x.days = d; x.endSec = (int)(a - (long long)d * 86400); x.h = (float)B[i].h; x.l = (float)B[i].l; x.c = (float)B[i].c;
        x.v = std::isfinite(B[i].v) && B[i].v > 0 ? B[i].v : 1.0; vb.push_back(x);
    }
    int k = (int)vb.size();
    while (k > 0 && svl::classify(vb[(size_t)k - 1].days, vb[(size_t)k - 1].endSec, (int)per, om, cm).kind != svl::RTH) k--;
    if (k <= 0) return;
    const vtl::Trend t = vtl::fromChart(vb, k, (int)per, om, cm, svl::RTH);
    S.view.trendReady = t.ready; S.view.trendLvl = t.ready ? vtl::level(t) : 0; S.view.trendBars = t.bars;
    S.view.trendVia = t.ready ? hl::csvSafe(t.strength == vtl::STR_STRONG ? std::string(vtl::strengthLabel(t)) : std::string(vtl::viaLabel(t))) : std::string();
}

static bool hlAppend(const std::string& p, const std::string& header, const std::string& text);   // (2.7.0) defined with the pre-open record below
// (2.7.0) the DAY TYPE (DayTypeLogic.h): today's CLOSED RTH bars only (the live session; nothing outside RTH / before the open), the
// per-market params from LRA-DayType-<MKT>.csv (strict; missing = defaults), shown in the header trend while called and SHOW != LOG
static void hlDayType(HLState& S, const std::vector<hl::Bar>& B, long long per, long long nowCt)
{
    S.view.dayText.clear(); S.view.dayHover.clear(); S.day = dtl::Result();
    const mp::Params P = mp::get("DayType", S.mkt);
    dtl::Params dp; dp.ibMin = (int)mp::num(P, "IB_MIN", 60, 15, 120); dp.extX = mp::num(P, "EXT_X", 1.5, 1.0, 5.0); dp.vwapHold = mp::num(P, "VWAP_HOLD", 0.80, 0.5, 1.0);
    dp.closePct = mp::num(P, "CLOSE_PCT", 0.35, 0.05, 0.5); dp.rotX = mp::num(P, "ROT_X", 1.25, 1.0, 3.0); dp.rotCross = (int)mp::num(P, "ROT_CROSS", 4, 1, 50);
    dp.rotMin = (int)mp::num(P, "ROT_MIN", 120, 60, 400); dp.mboTells = (int)mp::num(P, "MBO_TELLS", 0, 0, 1); dp.show = mp::mode(P, "SHOW", mp::SHOWQ) != mp::LOG ? 1 : 0;
    S.dayP = dp; S.dayParamWhy = P.missing ? "LRA-DayType-" + S.mkt + ".csv missing - defaults" : P.why;
    int om = 0, cm = 0;
    if (B.empty() || per <= 0 || nowCt <= 0 || !svl::rthOf(S.mkt.c_str(), om, cm)) { S.dayLine = "DAYTYPE," + std::string(dtl::VERSION) + ",no RTH hours / bars"; return; }
    const long long today = nowCt / 86400;
    std::vector<dtl::Bar> v;
    for (size_t i = 0; i < B.size(); i++) {
        const long long a = B[i].absEnd; if (a / 86400 != today) continue;
        const int endMin = (int)((a % 86400) / 60) - om; const int startMin = endMin - (int)(per / 60);
        if (startMin < 0 || endMin > cm - om) continue;          // RTH bars only (bar END stamps)
        dtl::Bar b; b.h = B[i].h; b.l = B[i].l; b.c = B[i].c; b.v = std::isfinite(B[i].v) ? B[i].v : 0; b.endMin = endMin; v.push_back(b);
    }
    S.day = dtl::classify(v, dp);
    S.dayLine = dtl::statusLine(S.day, dp) + (S.dayParamWhy.empty() ? "" : "," + S.dayParamWhy);
    if (S.day.kind == dtl::NONE || !dp.show) return;
    S.view.dayText = S.day.text;
    S.view.dayCol = S.day.kind == dtl::TREND_UP ? hl::C_GREEN : S.day.kind == dtl::TREND_DN ? hl::C_RED : hl::C_SLATE;
    const std::vector<std::string> h = dtl::hover(S.day);
    for (size_t i = 0; i < h.size(); i++) S.view.dayHover.push_back(hl::Line{ hl::Seg{ h[i], i == 0 ? S.view.dayCol : (h[i].compare(0, 4, "  + ") == 0 ? hl::C_SLATE : hl::C_GREY) } });
}

void HodLod::recompute(HLState& S, long n)
{
    S.recomputes++;
    double tick = (double)getProperty(SYM_TICKINCR);
    if (!std::isfinite(tick) || !(tick > 0) || tick >= 1000) { S.state = "chart tick size unavailable"; S.have = false; return; }
    tick = std::round(tick * 1e9) / 1e9;      // the float property, back to its decimal value (0.0005 not 0.000500000024)
    RTARRAYI dt(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) { S.state = "price arrays unavailable"; S.have = false; return; }
    auto absOf = [&](int i) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        return hl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday) * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    };
    // bar size from the chart's own stamps (bar END): the smallest positive gap of the last 40 bars
    long long per = 0, prev = absOf((int)n - 1);
    for (int i = (int)n - 2; i >= 0 && i >= (int)n - 41; i--) { long long a = absOf(i), d = prev - a; if (d > 0 && (per == 0 || d < per)) per = d; prev = a; }
    if (per <= 0) { S.state = "bar size unknown"; S.have = false; return; }
    if (per > 900) { S.state = "needs bars of 15 minutes or less"; S.have = false; return; }
    // the closed bars (a later bar exists: all but the last) back to 21 calendar days (time cutoff, #23), at most 40,000 bars (#40)
    const int lastClosed = (int)n - 2;
    long long cutoff = absOf(lastClosed) - 21LL * 86400;
    int start = lastClosed;
    while (start > 0 && lastClosed - start < 40000) { long long a = absOf(start - 1); if (a < cutoff) break; start--; }
    std::vector<hl::Bar> B; B.reserve((size_t)(lastClosed - start + 1));
    RTARRAYI vo(barVolume);                   // (HL101) volume for hlr::timing's VWAP trend; missing -> equal weights, flagged
    const bool haveVol = (long)vo.count >= n;
    for (int i = start; i <= lastClosed; i++) {
        hl::Bar b; b.absEnd = absOf(i); b.o = op[i]; b.h = hi[i]; b.l = lo[i]; b.c = cl[i];
        b.v = haveVol ? (double)vo[i] : NAN;
        B.push_back(b);
    }
    if (!B.empty()) { S.lastClosedAbs = B.back().absEnd; S.lastClose = B.back().c; }   // (2.4.0) the regime's price + the Market State's bar
    hl::Cfg c; c.mkt = S.mi; c.per = per; c.tick = tick; c.readOpen = hl::readOpenFor(S.mi); c.readN = hl::readNFor(S.mi);
    std::vector<hl::ReadRow> rows;
    hl::ProjLock lock;   // (1.1.6) the target stays where it was set until the 1st extreme is taken out
    hl::Tracker T = hl::runBars(c, B, B.size(), &rows, &lock);
    hl::ReadOut rd = rows.empty() ? hl::ReadOut() : rows.back().out;
    S.view = hl::buildView(T, rd, &rows, &S.mq, &lock);
    hlTrend(S, B, per);                       // (2.0.5) "Regime: Neg / Uptrend" - the trend from the closed bars only (no repaint)
    hlDayType(S, B, per, hlNowCt());          // (2.7.0) the day type (header trend text while called), from the same closed bars
    {   // (2.7.0) the day-type log for the nightly: one line each time the call changes (a restart logs the current call once again)
        const std::string key = std::to_string(hlNowCt() / 86400) + "," + std::to_string(S.day.kind);
        if (S.day.ready && key != S.dayLoggedKey && !B.empty()) {
            const std::string p = flexPath("HodLod\\DayType-" + S.mkt + ".csv");
            const long long a = B.back().absEnd; char ts[32]; std::snprintf(ts, sizeof ts, "%lld,%02d:%02d", a / 86400, (int)((a % 86400) / 3600), (int)((a % 3600) / 60));
            if (!p.empty() && hlAppend(p, "# lsHodLod day type log: D,<CT day number>,<bar end HH:MM>,<status fields>", std::string("D,") + ts + "," + S.dayLine + "," + hl::HL_VERSION + "\n")) { S.dayLoggedKey = key; S.dayLogs++; }
            else S.dayLogFails++;
        }
    }
    computeFx(S, n, per);                     // (2.0.0) the 1st Extreme model on the same chart (this recompute's bar size)
    S.view.fxSegs = hlfx::headerLine(S.fxView, S.fxHave);
    S.projIdx = -1;                           // (1.1.0) the 1st extreme's bar on this chart, matched by its END stamp
    if (S.view.projOk) for (size_t j = B.size(); j-- > 0;) if (B[j].absEnd == S.view.projFromEnd) { S.projIdx = start + (int)j; break; }
    if (S.view.projOk && S.projIdx >= start && tick > 0) {   // (2.2.2) the first closed bar after the 1st extreme that traded through the target
        const double tgt = (double)S.view.projTo * tick;
        for (size_t j = (size_t)(S.projIdx - start) + 1; j < B.size(); ++j) {
            if (S.view.projUp ? B[j].h >= tgt - tick * 0.5 : B[j].l <= tgt + tick * 0.5) {
                S.view.projHit = true; S.view.projHitMod = (int)(((B[j].absEnd - per) % 86400 + 86400) % 86400 / 60);
                for (size_t q = 0; q < S.view.headLeftSegs.size(); q++)
                    if (S.view.headLeftSegs[q].s.find(S.view.projTxt) != std::string::npos) { S.view.headLeftSegs[q].s = "   " + S.view.projTxt + " hit " + hl::clk(S.view.projHitMod); S.view.headLeftSegs[q].col = hl::C_DIM; }
                break;
            }
        }
    }
    if (!S.mqWhy.empty()) S.view.mqWhy = S.mqWhy + (S.view.mqWhy.empty() ? "" : "; " + S.view.mqWhy);
    S.rd = rd; S.per = per; S.tick = tick; S.have = S.view.has || !S.view.header.empty();
    delete S.T; S.T = new hl::Tracker(T);
    S.state = S.view.has ? (S.view.prior ? "prior session shown" : (S.view.complete ? "RTH complete" : "RTH live")) : "waiting for the RTH open";
    if (start > 0 && lastClosed - start >= 40000) S.state += " (history capped at 40,000 bars)";
    writeFiles(S, rows);
    writeProj(S);
    writeFirstCall(S, rows);
    S.firstPass = false;
}

// the read log (every closed RTH bar's read, live or replayed after a restart) + the OUTCOME line; marks only after a good write (#6)
// (1.1.5, Rassul 2026-10-12 "add updated ... time ... error ... you should also be tracking it for improvement purposes"): one line per
// change of the projected target (side or price) on the live session, 3-min charts only - what the chart showed and when. The
// nightly loop scores each target against the day's final HOD / LOD (error, updates, side flips) to refine the range model.
void HodLod::writeProj(HLState& S)
{
    const hl::View& v = S.view;
    if (S.per != 180 || S.mi < 0 || v.prior || !v.projOk || !(S.tick > 0)) return;
    const int dec = hl::MKTS[S.mi].dec;
    std::string tgt = hl::trimNum((double)v.projTo * S.tick, dec);
    // (1.1.6) one line per locked target: session | side | from | target; every key already in the file for this session is
    // skipped, so a restart (or a replay of the day) never writes a line twice
    std::string key = "\n" + v.session + "|" + (v.projUp ? "HOD" : "LOD") + "|" + std::to_string(v.projFrom) + "|" + tgt + "\n";
    std::string p = flexPath("HodLod.proj-" + S.mkt + ".csv"); if (p.empty()) return;
    if (S.projSession != v.session) {
        S.projSession = v.session; S.projLogged = "\n";
        std::ifstream f(p.c_str()); std::string ln;
        while (std::getline(f, ln)) if (ln.compare(0, v.session.size() + 1, v.session + ",") == 0) {
            std::vector<std::string> c; size_t a = 0, b; while ((b = ln.find(',', a)) != std::string::npos) { c.push_back(ln.substr(a, b - a)); a = b + 1; } c.push_back(ln.substr(a));
            double fr = 0; if (c.size() >= 5 && std::sscanf(c[3].c_str(), "%lf", &fr) == 1)
                S.projLogged += c[0] + "|" + c[2] + "|" + std::to_string((long long)std::llround(fr / S.tick)) + "|" + c[4] + "\n";
        }
    }
    if (S.projLogged.find(key) != std::string::npos) return;
    hl::FileOps F;
    if (!F.exists(p)) F.write(p, "session,bar_end,side,from,target,exp_range,range_so_far,stage,model,version\n", true);
    char re[32]; snprintf(re, sizeof re, "%.6g", v.projRange);
    int at = v.projLockMod >= 0 ? v.projLockMod : v.lastMod;   // the bar the target was set on
    std::string ln = v.session + "," + hl::hhmm(at) + "," + (v.projUp ? "HOD" : "LOD") + "," + hl::trimNum((double)v.projFrom * S.tick, dec) + "," + tgt + "," + re + "," +
        hl::trimNum((double)v.rangeT * S.tick, dec) + "," + std::to_string(v.projStage) + "," + (v.projV2 ? "v2" : "v1") + "," + hl::HL_VERSION + "\n";
    if (F.write(p, ln, true)) S.projLogged += key.substr(1);
}

// (1.1.8) the day's FIRST IN call, once per session (3-min charts): immutable - a session already in the file is never written again,
// so a restart, a replay or a later stronger call cannot change it. The nightly scores it against the day's exact first extreme and
// counts the sessions with no call (coverage).
void HodLod::writeFirstCall(HLState& S, const std::vector<hl::ReadRow>& rows)
{
    if (S.per != 180 || S.mi < 0 || rows.empty() || !(S.tick > 0)) return;
    if (S.fcSession == rows[0].session) return;          // this session's call is already on file
    hl::FirstCall f = hl::firstCall(rows);
    if (!f.ok) return;
    std::string p = flexPath("HodLod.firstcall-" + S.mkt + ".csv"); if (p.empty()) return;
    hl::FileOps F;
    if (F.exists(p)) {
        std::ifstream in(p.c_str()); std::string ln;
        while (std::getline(in, ln)) if (ln.compare(0, f.session.size() + 1, f.session + ",") == 0) { S.fcSession = f.session; return; }
    } else if (!F.write(p, std::string(hl::FIRSTCALL_HEADER) + "\n", true)) return;
    if (F.write(p, hl::firstCallLine(f, S.tick, hl::MKTS[S.mi].dec), true)) S.fcSession = f.session;
}

void HodLod::writeFiles(HLState& S, const std::vector<hl::ReadRow>& rows)
{
    if (rows.empty() && !(S.view.complete && !S.view.prior)) return;
    hl::FileOps F;
    std::string session = rows.empty() ? S.view.session : rows[0].session;
    std::string mine = flexPath("HodLod.reads-" + S.mkt + "-" + std::to_string(S.per) + ".csv");
    if (mine.empty()) return;
    if (!S.mine.primed || S.mine.session != session) {
        if (!F.exists(mine)) F.write(mine, std::string(hl::HL_LOG_HEADER) + "\n", true);
        hl::primeFromFile(S.mine, mine, session, 4, 5, "OUTCOME");
    }
    int dec = hl::MKTS[S.mi].dec;
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i].barEnd <= S.mine.lastBarEnd) continue;
        if (!F.write(mine, hl::readLogLine(S.mkt, (int)S.per, rows[i], !S.firstPass, S.tick, dec) + "\n", true)) break;
        S.mine.lastBarEnd = rows[i].barEnd;
    }
    if (S.view.complete && !S.view.prior && S.view.session == session && !S.mine.outcomeDone)
        if (F.write(mine, hl::outcomeLine(S.view, (int)S.per, S.tick) + "\n", true)) { S.mine.outcomeDone = true; S.mine.lastBarEnd = S.view.lastMod; }
#if HL_HAVE_READ
    // the nightly scorer's own file (hodlod_read.py LOG_COLS) - the read model is fitted on 3-min bars, so only 3-min charts write it
    if (S.per == 180 && hl::readSessionMatches(S.mi)) {
        std::string sp = flexPath("HodLodRead.reads-" + S.mkt + ".csv");
        if (!S.scorer.primed || S.scorer.session != session) hl::primeFromFile(S.scorer, sp, session, 1, 2, nullptr);
        for (size_t i = 0; i < rows.size(); i++) {
            if (rows[i].barEnd <= S.scorer.lastBarEnd || rows[i].out.state != 1) continue;
            if (!F.write(sp, hl::scorerLine(S.mkt, rows[i]) + "\n", true)) break;
            S.scorer.lastBarEnd = rows[i].barEnd;
        }
    }
#endif
}

// the status file: every value drawn, keyed by market AND seconds per bar (#9); written on change, at most every 2 s, and
// every 30 s regardless; atomic (#5)

// ------------------------------------------------------------------------------------------------ (2.0.0) the 1st Extreme model
static double fxRound(double v) { return std::round(v * 1e7) / 1e7; }   // the chart's float prices back to their decimal values
static std::string fxDayTxt(long long day)
{
    int y, m, d; hl::civilFromDays(day, y, m, d); char b[16]; snprintf(b, sizeof b, "%04d-%02d-%02d", y, m, d); return b;
}
// what lsFirstExtreme 1.0.1 did: 3-min charts, the closed bars back to 45 calendar days (14 complete sessions + the overnight),
// prices on the instrument's ticks (1.0.1 rounded to 1e-7 only), strictly increasing stamps; fx2::buildView for the last closed bar's day; the shadow logs once per bar
void HodLod::computeFx(HLState& S, long n, long long per)
{
    S.fxHave = false; S.fxIdx = -1;
    const int m = hlfx::fxMarket(S.mkt);
    if (m < 0) { S.fxState = "market not in the 1st Extreme model"; return; }
    if (per != 180) { S.fxState = "3-minute charts only (the model's bars)"; return; }
    RTARRAYI dt(barDateTime), vol(barVolume); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    if ((long)dt.count < n || (long)op.count < n || (long)hi.count < n || (long)lo.count < n || (long)cl.count < n) { S.fxState = "price arrays unavailable"; return; }
    if ((long)vol.count < n) { S.fxState = "volume unavailable (the model needs volume)"; return; }
    auto absOf = [&](int i) {
        struct tm t; memset(&t, 0, sizeof(t)); getLocaltime((RTDATE)dt[i], &t);
        return fx2::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday) * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
    };
    const int lastClosed = (int)n - 2;
    if (lastClosed < 1) { S.fxState = "too few bars"; return; }
    // (2.0.0) prices back to the instrument's own ticks: the chart's float 4135.6 is 4135.60009765625 and a 1e-7 rounding keeps that
    // error (GC / HG / NG / 6E), which moved two NG calls in the parity replay; the model was fitted on tick prices
    const double tk = fx2::MKTS[m].tick;
    auto onTick = [&](double v) { return fxRound(std::round(v / tk) * tk); };
    long long cutoff = absOf(lastClosed) - 45LL * 86400;
    int start = lastClosed; while (start > 0 && lastClosed - start < 60000) { if (absOf(start - 1) < cutoff) break; start--; }
    std::vector<fx2::Bar> B; B.reserve((size_t)(lastClosed - start + 1));
    std::vector<long long> ends; ends.reserve((size_t)(lastClosed - start + 1));
    for (int i = start; i <= lastClosed; i++) {
        fx2::Bar b; b.tEnd = absOf(i); b.o = onTick(op[i]); b.h = onTick(hi[i]); b.l = onTick(lo[i]); b.c = onTick(cl[i]); b.v = (double)vol[i];
        if (!B.empty() && b.tEnd <= B.back().tEnd) { ends.push_back(0); continue; }
        B.push_back(b); ends.push_back(b.tEnd);
    }
    if (B.empty()) { S.fxState = "no closed bars"; return; }
    long long day = fx2::floorDiv(B.back().tEnd, 86400);
    S.fxView = fx2::buildView(m, B, day);
    S.fxHave = true;
    S.fxState = S.fxView.state == fx2::ST_UNAVAILABLE ? S.fxView.why : "ok";
    if (S.fxView.called) for (size_t j = ends.size(); j-- > 0;) if (ends[j] == S.fxView.formedEnd) { S.fxIdx = start + (int)j; break; }
    writeFxLogs(S);
}
static bool fxAppend(const std::string& p, const std::string& header, const std::string& text)
{
    bool fresh = false; { std::ifstream t(p.c_str()); fresh = !t.good(); }
    std::ofstream f(p.c_str(), std::ios::out | std::ios::app | std::ios::binary);
    if (!f.is_open()) return false;
    if (fresh) f << header;
    f << text; f.flush(); return f.good();
}
// the shadow logs the nightly scorer reads (same files and lines as lsFirstExtreme): each assessed bar once, the call once per day;
// primed from the file after a restart so the day's lines already there are never repeated
void HodLod::writeFxLogs(HLState& S)
{
    const fx2::View& V = S.fxView;
    if (V.decisions.empty() || V.day < 0) return;
    std::string dayTxt = fxDayTxt(V.day);
    std::string pd = flexPath("FirstExtreme.decisions-" + S.mkt + ".csv"), pc = flexPath("FirstExtreme.calls-" + S.mkt + ".csv");
    if (pd.empty()) return;
    if (S.fxLogDay != V.day) {
        S.fxLogDay = V.day; S.fxLogElapsed = -1; S.fxCallDay = -1;
        std::ifstream f(pd.c_str()); std::string ln;
        while (std::getline(f, ln)) if (ln.compare(0, 10, dayTxt) == 0 && ln.size() > 11) { int e = atoi(ln.c_str() + 11); if (e > S.fxLogElapsed) S.fxLogElapsed = e; }
        std::ifstream g(pc.c_str()); while (std::getline(g, ln)) if (ln.compare(0, 10, dayTxt) == 0) S.fxCallDay = V.day;
    }
    std::string add; int maxE = S.fxLogElapsed;
    for (const auto& D : V.decisions) if (D.elapsed > S.fxLogElapsed) { add += fx2::decisionLine(V, D, dayTxt); maxE = (std::max)(maxE, D.elapsed); }
    if (!add.empty() && fxAppend(pd, fx2::DECISION_HEADER, add)) S.fxLogElapsed = maxE;
    if (V.called && S.fxCallDay != V.day && fxAppend(pc, fx2::CALL_HEADER, fx2::callLine(V, dayTxt))) S.fxCallDay = V.day;
}
void HodLod::writeFxStatus(HLState& S)
{
    if (S.mi < 0) return;
    std::string t = std::string("VERSION,") + fx2::FX_VERSION + " (inside lsHodLod " + hl::HL_VERSION + ")\nMODEL," + fx2::FX_MODEL + "\nMARKET," + S.mkt + "\nSTATE," + S.fxState + "\n";
    if (S.fxHave) {
        const fx2::View& V = S.fxView; char b[512];
        snprintf(b, sizeof b, "VIEW,state=%d,elapsed=%d,p_low=%.4f,p_high=%.4f,p_in=%.4f,low=%.10g,high=%.10g,called=%d,side=%d,price=%.10g,prob=%.4f,formed=%d,declared=%d,failed=%d,failed_at=%d,verdict=%d\n",
                 (int)V.state, V.elapsed, V.pLow, V.pHigh, V.pIn, V.lo, V.hi, (int)V.called, V.side, V.price, V.prob, V.formed, V.declared, (int)V.failed, V.failedAt, V.verdictKnown ? V.verdict : 9);
        t += b;
        std::string line; for (const auto& g : S.view.fxSegs) line += g.s; t += "LINE," + line + "\n";
    }
    time_t now = time(nullptr);
    if (t == S.fxLastStatus && now - S.fxStatusWrite < 30) return;
    if (now - S.fxStatusWrite < 2) return;
    std::string p = flexPath("FirstExtreme.status-" + S.mkt + ".txt"); if (p.empty()) return;
    hl::FileOps F;
    if (hl::writeAtomic(F, p, t)) { S.fxLastStatus = t; S.fxStatusWrite = now; }
}

void HodLod::writeStatus(HLState& S, const hl::Layout* L)
{
    if (S.mi < 0) return;
    std::string text;
    if (S.T && L) text = hl::statusText(S.view, *L, *S.T, S.rd, (int)S.per, S.state) + "CHART," + S.root + "," + std::to_string(S.per) + "\n" + S.placeLine;   // (HL105) chart identity for SessionInfo; (2.0.6) PLACE / OPTS
    else text = std::string("VERSION,") + hl::HL_VERSION + "\nMARKET," + S.mkt + "\nPER," + std::to_string(S.per) + "\nSTATE," + S.state + "\n";
    time_t now = time(nullptr);
    bool changed = text != S.lastStatus;
    if (!(changed && now - S.lastStatusWrite >= 2) && now - S.lastStatusWrite < 30) return;
    std::string p = flexPath("HodLod.status-" + S.mkt + "-" + std::to_string(S.per) + ".txt");
    if (p.empty()) return;
    hl::FileOps F;
    if (hl::writeAtomic(F, p, text)) { S.lastStatus = text; S.lastStatusWrite = now; S.statusWrites++; }
}

void HodLod::execute(const hl::Layout& L)
{
    for (size_t i = 0; i < L.items.size(); i++) {
        const hl::Item& it = L.items[i];
        switch (it.k) {
            case hl::K_FILL: { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw(0, (COLOR)it.c1, (COLOR)it.c1, DRAW_OPAQUE, PAT_SOLID); break; }
            case hl::K_RECT:
                if (it.c2 == 0xFFFFFFFFu) {       // hollow (the candle body): four lines
                    line(it.l, it.t, it.r, it.t, (COLOR)it.c1, it.w); line(it.r, it.t, it.r, it.b, (COLOR)it.c1, it.w);
                    line(it.r, it.b, it.l, it.b, (COLOR)it.c1, it.w); line(it.l, it.b, it.l, it.t, (COLOR)it.c1, it.w);
                } else { RCT rc; rc.set((short)it.l, (short)it.t, (short)it.r, (short)it.b); rc.draw((short)it.w, (COLOR)it.c1, (COLOR)it.c2, DRAW_OPAQUE, PAT_SOLID); }
                break;
            case hl::K_LINE: line(it.l, it.t, it.r, it.b, (COLOR)it.c1, it.w); break;
            case hl::K_TEXT: text(it.l, (it.t + it.b) / 2, it.s, (COLOR)it.c1, it.fs, it.bold); break;
            default: break;
        }
    }
}

static long long hlFileStamp(const std::string& p)
{
#ifdef _WIN32
    struct _stat64 st; if (_stat64(p.c_str(), &st) != 0) return -1;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return -1;
#endif
    return (long long)st.st_mtime * 1000003LL + (long long)st.st_size;
}
// (2.0.6) the settings window's copy has no bar size: only there are the dialog values real (on the chart every box reads unchecked)
static std::string hlMarket(cppExtension* x) { char rb[32] = { 0 }; const char* rs = x->getRootSymbol(rb); return hl::marketForRoot(rs ? rs : ""); }
// (2.4.2) the settings store of one market: HodLod.settings-<MKT>.txt (keyed), migrated once from HodLod.place-<MKT>.txt
ss::Store& HodLod::storeFor(const std::string& mkt)
{
    for (size_t i = 0; i < stores_.size(); i++) if (stores_[i].mkt == mkt) return stores_[i].st;
    if (stores_.size() >= 8) stores_.erase(stores_.begin());
    SetStore n; n.mkt = mkt; n.st.S = &hlset::spec(); n.st.path = flexPath("HodLod.settings-" + mkt + ".txt");
    n.st.legacyPaths.push_back(flexPath("HodLod.place-" + mkt + ".txt")); n.st.legacy = hlset::legacy;
    stores_.push_back(n); return stores_.back().st;
}
HodLod::Place HodLod::toPlace(const ss::Values& V)
{
    using namespace hlset;
    Place P; P.pos = V.i(K_POSITION); P.o.dx = 0; P.o.dy = 0;   // (2.4.4) Offset X / Y removed (the 9-spot Position only)
    P.o.table = V.b(K_SHOW_TABLE); P.o.proj = V.b(K_SHOW_PROJ); P.o.mark = V.b(K_SHOW_MARK);
    const int ck[5] = { K_TABLE_BG, K_PROJ_UP, K_PROJ_DN, K_MARK_LOD, K_MARK_HOD };
    for (int k = 0; k < 5; ++k) P.col[k] = (long long)V.c(ck[k]);
    P.preShow = V.b(K_SHOW_PRE); P.preMin = V.i(K_PRE_MIN); if (P.preMin < 5 || P.preMin > 60) P.preMin = 15;   // (2.6.0)
    P.why = V.why; return P;
}
static long long hlNowMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
int HodLod::parmsLoadBody(void)
{
    if (!ss::inDialog(this)) return RTX_OK;                              // loading never saves (#29)
    const std::string mkt = hlMarket(this); if (mkt.empty()) return RTX_OK;
    const ss::Values V = storeFor(mkt).load(ss::marketIndex(mkt), hlNowMs(), true);
    ss::fillDialog(this, hlset::spec(), hlIds(), V, snap_);
    return RTX_OK;
}
void HodLod::savePos()
{
    if (!ss::inDialog(this)) return;                                    // (2.0.6) never from the chart copy (its boxes read unchecked)
    const std::string mkt = hlMarket(this); if (mkt.empty()) return;
    ss::Store& st = storeFor(mkt);
    const ss::Values F = st.load(ss::marketIndex(mkt), hlNowMs(), true);
    const ss::Values V = ss::readDialog(this, hlset::spec(), hlIds(), F, snap_);   // untouched -> the file's value; invalid never saved
    if (st.save(V)) snap_.take(ss::dialogNow(this, hlset::spec(), hlIds(), V));
}
// (2.0.6) ONE-TIME migration (Rassul 07:13 "move it DOWN, default Bottom centre"): per market, the first time 2.0.6 sees its place file,
// a 2.0.5 default "1" (alone, or "1,0,0" ...) becomes 7 (Bottom centre); any other value is his choice and stays. HodLod.migrate-206.txt
// records every market checked (and what was done), so a later deliberate "Top centre" is never migrated again.
void HodLod::migratePlace(const std::string& mkt)
{
    for (size_t i = 0; i < migChecked.size(); i++) if (migChecked[i] == mkt) return;
    if (migChecked.size() >= 16) migChecked.erase(migChecked.begin());
    migChecked.push_back(mkt);
    const std::string logp = flexPath("HodLod.migrate-206.txt"), p = flexPath("HodLod.place-" + mkt + ".txt");
    if (logp.empty() || p.empty()) return;
    std::string log;
    {   std::ifstream f(logp.c_str(), std::ios::binary); std::string ln; int nl = 0;
        while (nl++ < 200 && std::getline(f, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            if (ln.compare(0, mkt.size() + 1, mkt + ",") == 0) return;                     // this market was already checked
            if (ln.size() < 200) log += ln + "\n"; } }
    std::string cur;
    {   std::ifstream f(p.c_str()); if (f.is_open() && std::getline(f, cur) && !cur.empty() && cur.back() == '\r') cur.pop_back(); }
    time_t t = time(nullptr); struct tm lt;
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    char when[32]; strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", &lt);
    hl::FileOps F; std::string what;
    Place P; const bool ok = !cur.empty() && hlParsePlace(cur, P);
    if (ok && P.pos == hl::POS_PREV_DEFAULT) {
        P.pos = hl::POS_DEFAULT;
        if (!hl::writeAtomic(F, p, hlFormatPlace(P) + "\n")) return;      // not written: try again on the next load
        what = "place 1 (2.0.5 default Top centre) -> 7 (Bottom centre)";
    } else what = cur.empty() ? "no place file: default 7 (Bottom centre)" : "place '" + cur.substr(0, 40) + "' kept (his choice)";
    hl::writeAtomic(F, logp, log + mkt + "," + when + "," + hl::HL_VERSION + "," + what + "\n");
}
HodLod::Place HodLod::readPlace(const std::string& mkt)
{
    if (mkt.empty()) return Place();
    return toPlace(storeFor(mkt).load(ss::marketIndex(mkt), hlNowMs()));
}
// the per-market settings file is the ONLY source while drawing (IRT hands the chart copy a stale list / unchecked boxes);
// (2.4.2) re-read at most once a second and compared by content (lesson 84), never by stamp alone
HodLod::Place HodLod::position(const std::string& mkt)
{
    if (!mkt.empty()) migratePlace(mkt);                                // the 2.0.6 one-time 1 -> 7 on the old place file, before it migrates
    return readPlace(mkt);
}
// (2.0.5) the boxes of other indicators the table keeps clear of: bounded reads, at most every 3 s
void HodLod::loadAvoid(HLState& S)
{
    time_t now = time(nullptr);
    if (S.avoidChecked != 0 && now - S.avoidChecked < 3) return;
    S.avoidChecked = now; S.trBoxOk = false; S.trSeen = false;
    {   // the Tape Reader's status: "POSITION,<0-8>" and "LAYOUT,drawn,<note>,box l t r b,region ..." (written >= every 30 s while it draws)
        const std::string p = flexPath("TapeReader.status-" + S.mkt + ".txt");
        const long long st = hlFileStamp(p);
        if (!p.empty() && st >= 0) {
#ifdef _WIN32
            struct _stat64 sb; bool ok = _stat64(p.c_str(), &sb) == 0;
#else
            struct stat sb; bool ok = stat(p.c_str(), &sb) == 0;
#endif
            // (2.4.0, audit C finding 13) the slot is reserved only while the Tape Reader is drawing (its status written in the last 120 s,
            // Fresh.h fileFresh); a file left by a removed Tape Reader (ES: written Wed) no longer pushes the table
            const bool fresh = ok && (long long)sb.st_size <= 64 * 1024 && fr::fileFresh((long long)sb.st_mtime, (long long)now, fr::TAPE_STATUS_MAX_SEC);
            S.trSeen = fresh;
            std::ifstream f(p.c_str(), std::ios::binary); std::string ln; int nl = 0;
            while (ok && nl++ < 200 && std::getline(f, ln)) {
                if (ln.size() > 2048) continue;
                if (!ln.empty() && ln.back() == '\r') ln.pop_back();
                if (ln.compare(0, 9, "POSITION,") == 0 && ln.size() == 10 && ln[9] >= '0' && ln[9] <= '8') S.trPos = ln[9] - '0';
                if (fresh && ln.compare(0, 13, "LAYOUT,drawn,") == 0) {
                    size_t k = ln.find(",box "); int l, t, r, b;
                    if (k != std::string::npos && std::sscanf(ln.c_str() + k + 5, "%d %d %d %d", &l, &t, &r, &b) == 4 && r > l && b > t && r - l < 4000 && b - t < 4000) {
                        S.trBox = hl::Box{ l, t, r, b }; S.trBoxOk = true; }
                }
            }
        }
    }
    {   // the Dealer Profile's whole-book banner (top left, off by default): (2.4.3) the "banner" key of DealerProfile.settings-<MKT>.txt
        // (lsDealerProfile 2.17.1, keyed per market); before the Dealer Profile has migrated, the old root-keyed DealerProfile.settings.txt
        // row of this market through the same legacy reader the Dealer Profile uses (dprset::legacy) - never a positional column here
        S.dpBanner = false;
        std::string text; ss::Values V;
        const std::string pk = flexPath("DealerProfile.settings-" + S.mkt + ".txt"), pl = flexPath("DealerProfile.settings.txt");
        const int mi = ss::marketIndex(S.mkt);
        if (!pk.empty() && ss::readSmall(pk, text) && ss::parse(dprset::spec(), mi, text, V)) S.dpBanner = V.b(dprset::K_BANNER);
        else if (!pl.empty() && ss::readSmall(pl, text)) { V = ss::defaults(dprset::spec(), mi); if (dprset::legacy(dprset::spec(), mi, text, V)) S.dpBanner = V.b(dprset::K_BANNER); }
    }
}
// (2.0.5) the avoid boxes for this pane: the Dealer Profile header (right-aligned at the scale edge, ~44 px tall, 2.9.0
// DealerProfile.cpp header()) and its banner when on (top left, ~30 px), the Tape Reader box (published, else its Position region)
static std::vector<hl::Box> hlAvoid(const HLState& S, const hl::Box& pane, int priceEdge)
{
    std::vector<hl::Box> v;
    const int DP_HDR_W = 250, DP_HDR_H = 46, DP_BANNER_W = 440, DP_BANNER_H = 32, TR_W = 700, TR_H = 48;   // (2.0.6) the Tape Reader 1.1.3 banner (2 lines, 11 pt)
    v.push_back(hl::Box{ pane.r - DP_HDR_W, pane.t, pane.r, pane.t + DP_HDR_H });
    if (S.dpBanner) v.push_back(hl::Box{ pane.l, pane.t, pane.l + DP_BANNER_W, pane.t + DP_BANNER_H });
    if (S.trBoxOk) v.push_back(hl::Box{ S.trBox.l - 2, S.trBox.t - 2, S.trBox.r + 2, S.trBox.b + 2 });
    else if (S.trSeen) {   // no fresh box (not drawn yet / another chart): reserve its Position spot at the 11 pt size, up to the last bar
        const int r = priceEdge > 0 ? priceEdge : pane.r - (pane.r - pane.l) * 15 / 100;
        const hl::Box area = { pane.l + 110, pane.t + 2, (std::max)(pane.l + 112, r), pane.b - 2 };
        const int W = (std::min)(TR_W, area.r - area.l), H = (std::min)(TR_H, area.b - area.t);
        if (W > 0 && H > 0) v.push_back(hl::placeIn(area, W, H, S.trPos));
    }
    return v;
}
// the last price bar's right edge (x): where the usable area ends (never beyond the last bar); -1 = unknown
int HodLod::priceEdge()
{
    long n = getBarCount(); if (n < 2) return -1;
    RTARRAY cl(barClose); if ((long)cl.count < n) return -1;
    PNT p; if (p.set((int)(n - 1), cl[(int)(n - 1)], kBarRight) != RTX_OK) return -1;
    return p.h > 0 ? (int)p.h : -1;
}

// ---- (2.6.0) the PRE-OPEN BOOK ----------------------------------------------------------------------------------------------------
// one order event -> the engine, on the Central clock of the event's own stamp (whole seconds; cached per stamp)
void HodLod::preRoute(HLState& S, const DEPTH_ORDER& o)
{
    S.preRouted++;
    if (!(S.preTick > 0) || !std::isfinite((double)o.price) || o.price <= 0) { S.pre.ignored++; return; }
    const double q = (double)o.price / S.preTick; if (!(q > 0 && q < 4e12)) { S.pre.ignored++; return; }
    const long long px = (long long)std::llround(q);
    long long ms = 0;
    if (o.timestamp != 0 && o.timestamp == S.preStampKey) ms = S.preStampMs;
    else if (o.timestamp != 0) {
        struct tm t; memset(&t, 0, sizeof t);
        if (getLocaltime((RTDATE)o.timestamp, &t) && t.tm_mon >= 0 && t.tm_mon <= 11 && t.tm_mday >= 1 && t.tm_mday <= 31) {
            ms = (hl::daysFromCivil(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec) * 1000LL;
            S.preStampKey = o.timestamp; S.preStampMs = ms;
        }
    }
    if (ms <= 0) { S.pre.ignored++; return; }
    const int side = o.buySell == 1 ? po::BID : o.buySell == 0 ? po::ASK : -1;
    S.pre.onEvent((int)o.action, side, px, (long long)o.size, (unsigned long long)(unsigned int)o.orderID, ms);
}
// the record of today's session (SNAP / IMB lines) -> the frozen read after a restart (no repaint); bounded read (256 KiB, 4,000 lines)
void HodLod::preRestore(HLState& S)
{
    const std::string p = flexPath("HodLod\\PreOpen-" + S.mkt + "-" + std::to_string(S.prePer) + ".csv"); if (p.empty()) return;
    std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) return;
    const std::string ses = po::sessionKey(S.pre.dayMs); std::string ln; int lines = 0; long long bytes = 0;
    bool gotSnap = false, gotImb = false;
    while (lines++ < 4000 && bytes < 262144 && std::getline(f, ln)) {
        bytes += (long long)ln.size() + 1;
        po::Snap sn; po::Imb im; const int k = po::parseRec(ln, ses, S.mkt, S.prePer, S.pre.dayMs, sn, im);
        if (k == 1 && !gotSnap) { S.pre.restoreSnap(sn); gotSnap = true; S.preRestored++; }
        else if (k == 2 && !gotImb) { if (!gotSnap) { S.pre.restoreSnap(sn); gotSnap = true; } S.pre.restoreImb(im); gotImb = true; S.preRestored++; }
    }
    S.preSnapWritten = gotSnap; S.preImbWritten = gotImb;
}
static bool hlAppend(const std::string& p, const std::string& header, const std::string& text)
{
    bool fresh = true; { std::ifstream t(p.c_str(), std::ios::binary); if (t.is_open()) { t.seekg(0, std::ios::end); fresh = t.tellg() <= 0; } }
    std::ofstream f(p.c_str(), std::ios::binary | std::ios::app); if (!f.is_open()) return false;
    if (fresh) f << header << "\n";
    f << text; f.flush(); return f.good();
}
// calc / draw (chart context): today's engine, the chart's near band, the feed while it is wanted, the decisions, the record
void HodLod::preTick(HLState& S)
{
    if (S.mi < 0 || S.preBusy) return;
    S.preBusy = true;
    struct Unbusy { HLState* s; ~Unbusy() { s->preBusy = false; } } unbusy = { &S };
    try {
        const long per = getSecondsPerBar();
        const float tk = getProperty(SYM_TICKINCR);
        if (!(std::isfinite(tk) && tk > 0)) { S.preState = "no tick size"; return; }
        if (S.pre.mkt != S.mkt || S.pre.openMin != hl::MKTS[S.mi].open || per != S.prePer || (double)tk != S.preTick) {   // a new market / bar size: start clean
            preStopTimer(S); S.preFeed.close(); S.pre = po::Engine(); S.pre.mkt = S.mkt; S.pre.openMin = hl::MKTS[S.mi].open; S.pre.s20 = po::s20Of(S.mkt);
            S.preDay = -1; S.prePer = per; S.preTick = (double)tk; S.preTimerTried = false; S.preStampKey = 0; S.preRouted = 0;
        }
        const long long nowCt = hlNowCt(); if (nowCt <= 0) { S.preState = "no clock"; return; }
        const long long nowMs = nowCt * 1000LL, dayMs = (nowCt / 86400LL) * 86400000LL;
        if (dayMs != S.preDay) {
            preStopTimer(S); S.preFeed.close(); S.preTimerTried = false;
            S.pre.setDay(dayMs); S.preDay = dayMs; S.preNearFrozen = false; S.preSnapWritten = S.preImbWritten = false; S.preLineMs = -100000;
            preRestore(S);
        }
        const int wday = (int)(((nowCt / 86400LL) + 4) % 7);   // 1970-01-01 was a Thursday: 0 = Sunday
        if (wday == 0 || wday == 6) { if (S.preFeed.dbo) { preStopTimer(S); S.preFeed.close(); } S.preState = "weekend"; return; }
        if (!S.preShow) { if (S.preFeed.dbo) { preStopTimer(S); S.preFeed.close(); } S.preState = "off (Show pre-open line unchecked)"; return; }
        if (!S.pre.snapDone && nowMs < S.pre.winStartMs() - 60000LL && S.pre.P.windowMin != S.preMin) S.pre.P.windowMin = S.preMin;   // a new N applies before the window only
        {   // the near band = 1 ATR(14) of the chart's closed candles (>= 4 ticks), frozen at the window's start; the fallback reference = the last close
            const long n = getBarCount(); RTARRAY hA(barHigh), lA(barLow), cA(barClose);
            if (n >= 3 && (long)hA.count >= n && (long)lA.count >= n && (long)cA.count >= n) {
                const double last = (double)cA[(int)n - 1]; if (std::isfinite(last) && last > 0) S.pre.refFallback = (long long)std::llround(last / (double)tk);
                if (!S.preNearFrozen) {
                    double sum = 0; int k = 0;
                    for (long i = n - 2; i >= 1 && k < 14; --i, ++k) {
                        const double h = hA[(int)i], l = lA[(int)i], pc = cA[(int)i - 1];
                        if (!(std::isfinite(h) && std::isfinite(l) && std::isfinite(pc)) || h < l) { k--; continue; }
                        sum += (std::max)(h - l, (std::max)(std::fabs(h - pc), std::fabs(l - pc)));
                    }
                    if (k > 0) S.pre.nearTicks = (std::max)(4, (int)std::lround(sum / k / (double)tk));
                    if (nowMs >= S.pre.winStartMs()) S.preNearFrozen = true;
                }
            }
        }
        const long long st = hlNowMs();
        const bool want = S.pre.wantFeed(nowMs) && !(S.pre.snapDone && S.pre.imbDone);
        if (want) {
            const char* sy = getSymbol(); S.preSym = sy ? sy : "";
            const std::string dir = flexPath("HodLod");
#if defined(_WIN32)
            if (!dir.empty()) CreateDirectoryA(dir.c_str(), NULL);
#endif
            S.preFeed.open(S.preSym, dir.empty() ? std::string() : flexPath("HodLod\\_opening-dbo-" + S.mkt + "-" + std::to_string(per) + ".txt"), st, S.preState);
            S.preFeed.read(st, hlNowMs, [&](const DEPTH_ORDER& o) { preRoute(S, o); }, false);
            if (!S.preTimerTried && !S.preTimerOn && S.preFeed.named && S.preFeed.flag == 1) {
                S.preTimerTried = true; int id = 7600 + (int)(((uintptr_t)&S >> 4) % 1000);
                while (preTimers().count(id)) ++id;
                if (createTimer(id, (int)hlpo::TIMER_MS) == RTX_OK) { S.preTimerId = id; S.preTimerOn = true; preTimers()[id] = &S; }
            }
            S.preState = "reading (" + S.preFeed.stateText(st) + ")";
        } else if (S.preFeed.dbo) { preStopTimer(S); S.preFeed.close(); S.preState = "feed closed (read done for today)"; }
        else S.preState = nowMs < S.pre.feedStartMs() ? "waiting for " + po::hm12(S.pre.openMin - S.pre.P.windowMin - S.pre.P.leadMin) : "idle";
        S.pre.tick(nowMs);
        // the record: SNAP once frozen at the open, IMB once the opening imbalance is in (first line per session + kind wins)
        if ((S.pre.snapDone && !S.preSnapWritten) || (S.pre.imbDone && !S.preImbWritten)) {
            const std::string p = flexPath("HodLod\\PreOpen-" + S.mkt + "-" + std::to_string(per) + ".csv");
            if (!p.empty()) {
                if (S.pre.snapDone && !S.preSnapWritten) { if (hlAppend(p, po::REC_HEADER, po::recLine(S.pre, per, false))) { S.preSnapWritten = true; S.preWrites++; } else S.preWriteFails++; }
                if (S.pre.imbDone && !S.preImbWritten && S.preSnapWritten) { if (hlAppend(p, po::REC_HEADER, po::recLine(S.pre, per, true))) { S.preImbWritten = true; S.preWrites++; } else S.preWriteFails++; }
            }
        }
    } catch (...) { S.preState = "pre-open error (kept the last read)"; }
}
// the 250 ms clock: reads the named stream between IRT's calc / draw calls (IRT keeps only the last few thousand events per subscriber)
int HodLod::timer(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    HLState* S = nullptr;
    { std::map<int, HLState*>::const_iterator it = preTimers().find(e->v.timer.id); if (it != preTimers().end()) S = it->second; }
    if (!S) return RTX_FAIL;                                                // not ours
    if (S->preBusy) return RTX_OK;
    S->preBusy = true;
    struct Unbusy { HLState* s; ~Unbusy() { s->preBusy = false; } } unbusy = { S };
    try { S->preFeed.read(hlNowMs(), hlNowMs, [&](const DEPTH_ORDER& o) { preRoute(*S, o); }, true); if (S->preFeed.dead) preStopTimer(*S); } catch (...) {}
    return RTX_OK;
}

int HodLod::draw(void)
{
    HLState* S = state(true);
    if (!S) return RTX_OK;
    refresh(*S);
    if (S->mi < 0) return RTX_OK;
    RCT pane; pane.getPaneRect(false);
    hl::Box pb = { pane.left, pane.top, pane.right, pane.bottom };
    if (!S->have || !S->T) {
        std::string s = "HOD/LOD: " + S->state;
        if (pb.r - pb.l > 200 && pb.b - pb.t > 40) text(pb.l + 8, pb.b - 14, s, HL_GREY, 8, false);
        writeStatus(*S, nullptr);
        writeFxStatus(*S);
        return RTX_OK;
    }
    const double tick = S->tick;
    auto yOf = [&](long long ticks) { return (int)getVerticalDrawPosition((float)((double)ticks * tick)); };
    hl::Measure M = [&](const std::string& s, int fs, bool bold) { return textW(s, fs, bold); };
    int projX = -1;                           // (1.1.0) the projection line's x: the 1st extreme's bar centre
    if (S->view.projOk && S->projIdx >= 0 && S->projIdx < (int)getBarCount()) {
        PNT p; if (p.set(S->projIdx, (float)((double)S->view.projFrom * tick), kBarCenter) == RTX_OK && p.h > 0) projX = (int)p.h;
    }
    loadSession(*S); loadHvl(*S); hlSessSegs(*S);   // (2.0.2) Regime / Expiry in the header (Session Info retired); (2.4.0) Regime from the HVL0 book
    loadAvoid(*S);                           // (2.0.5) the Dealer Profile header / banner and the Tape Reader box
    loadGex(*S);                             // (2.1.0) the GEX line, nearest the chart's last price first
    {   long n = getBarCount(); double px = NAN;
        if (n > 0) { RTARRAY cl(barClose); if ((long)cl.count >= n) px = (double)cl[(int)(n - 1)]; }
        time_t t = time(nullptr); struct tm lt; memset(&lt, 0, sizeof lt);
#ifdef _WIN32
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        hlg::toView(S->view, S->gex, px, (long long)t, lt.tm_wday, lt.tm_hour * 60 + lt.tm_min, hl::MKTS[S->mi < 0 ? 0 : S->mi].dec);
    }
    {   // (2.5.0, Rassul 10:40 "i also dont see the regime") the HVL book stale / missing but the Gex S/R file LIVE: the regime from ITS HVL0
        const double h = hlg::hvlFromGex(S->gex);
        S->regFromGex = false;
        if (S->regSign == 0 && S->view.gexStale.empty() && std::isfinite(h) && std::isfinite(S->lastClose) && S->lastClose > 0) {
            S->regSign = S->lastClose >= h ? 1 : -1; S->regFromGex = true; S->regGexHvl = h;
            for (size_t q = 0; q < S->view.sessSegs.size(); q++) if (S->view.sessSegs[q].s.compare(0, 6, "Regime") == 0) { S->view.sessSegs.erase(S->view.sessSegs.begin() + (long)q); break; }
            S->view.sessSegs.insert(S->view.sessSegs.begin(), hl::Seg{ S->regSign < 0 ? "Regime: Negative" : "Regime: Positive", S->regSign < 0 ? hl::C_RED : hl::C_GREEN });
        }
    }
    loadQ(*S);                               // (2.3.0) the Q-Scores top left of the header
    S->qV = hlq::fresh(S->qf, hlNowCt()); S->qJudged = S->qf.ok;   // (2.4.0) their age: stale -> "Q: STALE"
    hlq::toView(S->view, S->qf, S->view.trendReady ? hl::trendWord(S->view.trendLvl) : std::string(), S->view.trendLvl, &S->qV);
    const int pe = priceEdge();
    const std::vector<hl::Box> avoid = hlAvoid(*S, pb, pe);
    const Place PL = position(S->mkt);
    const hl::Opts O = opts(PL);
    {   // (2.6.0) the PRE-OPEN BOOK row: the read (cached 1 s - a live read walks the book), its hover only while the mouse is on it
        S->preShow = PL.preShow; S->preMin = PL.preMin;
        preTick(*S);
        const long long nowMs = hlNowCt() * 1000LL, st = hlNowMs();
        if (st - S->preLineMs >= 1000 || st < S->preLineMs) { S->preLine = po::lineAt(S->pre, nowMs, S->preFeed.lastEventMs >= 0 ? 1 : 0); S->preLineMs = st; }
        {   // (2.7.0) VALIDATE-CONTEXT-MARKS 2026-10-22: the Pre row is LOG ONLY until the nightly promotes it (LRA-PreOpen-<MKT>.csv PARAM,SHOW); it keeps recording
            const mp::Params PP = mp::get("PreOpen", S->mkt); S->preParamShow = mp::mode(PP, "SHOW", mp::LOG); S->preParamWhy = PP.missing ? "no file - LOG" : PP.why;
        }
        S->view.preOn = S->preShow && S->preParamShow != mp::LOG && S->preLine.show && S->pre.dayMs >= 0 && S->preState != "weekend";
        S->view.preText = S->preLine.s;
        S->view.preCol = S->preLine.col == po::COL_GREEN ? hl::C_GREEN : S->preLine.col == po::COL_RED ? hl::C_RED : hl::C_GREY;
        S->view.preHover.clear();
        if (HL_PRE_HOVER && S->inPre && S->hovWhich == 0 && S->view.preOn)
            for (const po::HLine& h : po::hover(S->pre, nowMs)) S->view.preHover.push_back(hl::Line{ hl::Seg{ h.s, h.col == po::COL_GREEN ? hl::C_GREEN : h.col == po::COL_RED ? hl::C_RED : (h.bold ? hl::C_SLATE : hl::C_GREY) } });
    }
    {   // (2.7.0) the NEWS / SETTLE window row and the Feed ? row (NewsWindow.h), checked every draw against the PC's CT clock
        S->view.notes.clear();
        const nw::Gate g = nw::atCtSec(S->mkt, hlNowCt());
        if (g.news) {
            hl::View::Note N; N.label = "News"; N.col = hl::C_AMBER;
            N.text = g.label + " \xB7 -" + std::to_string(g.pre) + " / +" + std::to_string(g.post) + " min \xB7 marks ?";
            const char* H[] = { "News / settle window ?  ", "Question: is a scheduled release or the settlement moving price right now?",
                "What it is: the window around a tier-1 release (CPI / NFP 7:30 AM, FOMC 1:00 PM), the EIA reports (CL Wed / NG Thu 9:30 AM) or this market's daily settlement.",
                "Why it matters: inside the window sweeps run wider and order-flow marks are not proven, so every mark drawn in it carries '?'.",
                "How to use: a level is a plan, not a trade, until the window ends; judge stops in R and prefer the CoB over the reversal bar.",
                "Example: ES CPI day 7:25 - 7:45 AM: an Sr at 7:31 shows as 'Sr?'." };
            for (int i = 0; i < 6; i++) N.hover.push_back(hl::Line{ hl::Seg{ i == 0 ? std::string(H[0]) + g.label : std::string(H[i]), i == 0 ? hl::C_AMBER : hl::C_SLATE } });
            S->view.notes.push_back(N);
        }
        if (g.feedBad) {
            hl::View::Note N; N.label = "Feed"; N.col = hl::C_AMBER; N.text = "? live MBO feed does not match \xB7 MBO marks record only";
            const char* H[] = { "Feed ?", "Question: can the order-by-order (MBO) marks on this market be trusted today?",
                "What it is: the nightly compares this PC's recorded MBO feed with the exchange data (LRA-FeedCheck file); here they did not match.",
                "Why it matters: Sr, St, turn orders, the queue-meter call and the reversal stages are built from that feed.",
                "How to use: those marks are recorded only (not drawn) on this market until the check passes again; price, options and levels are unaffected.",
                "Example: NQ feed dropped 3% of the book events overnight -> 'Feed ?' and no Sr / St on NQ." };
            for (int i = 0; i < 6; i++) N.hover.push_back(hl::Line{ hl::Seg{ i == 0 ? std::string(H[0]) + "  " + g.feedWhy : std::string(H[i]), i == 0 ? hl::C_AMBER : hl::C_SLATE } });
            S->view.notes.push_back(N);
        }
        S->newsLine = std::string("NEWS,") + nw::VERSION + "," + (g.news ? g.label : std::string("none")) + ",feed " + (g.feedBad ? "BAD (" + g.feedWhy + ")" : std::string("ok"));
    }
    S->placeLine = "PLACE,pos " + std::to_string(PL.pos) + ",dx " + std::to_string(O.dx) + ",dy " + std::to_string(O.dy) + (PL.why.empty() ? "" : "," + PL.why)
                 + "\nOPTS,table " + (O.table ? "on" : "off (Show table unchecked in Settings)") + ",projection " + (O.proj ? "on" : "off") + ",mark " + (O.mark ? "on" : "off") + ",moves " + std::to_string(S->hyst.moves) + "\n";
    hl::Layout L = hl::layoutAll(S->view, pb, yOf, M, pe, PL.pos, projX, &avoid, &O);
    {   // (2.0.6) the table's place has hysteresis: a new spot only after 2 draws, never straight back (reset when the settings change)
        if (PL.sig != S->hystPlaceSig || pb.l != S->hystPane.l || pb.t != S->hystPane.t || pb.r != S->hystPane.r || pb.b != S->hystPane.b) { S->hyst.reset(); S->hystPlaceSig = PL.sig; S->hystPane = pb; }   // settings or the pane changed: start clean
        if (L.tableOk) {
            const bp::Box want = bp::mk(L.table.l, L.table.t, L.table.l + 1, L.table.t + 1);
            bool blocked = false; if (S->hyst.have) for (const hl::Box& a : avoid) { const bp::Box c = bp::mk(S->hyst.cur.l, S->hyst.cur.t - (L.gexOk ? L.table.t - L.gexRow.t : 0), S->hyst.cur.l + (L.table.r - L.table.l), S->hyst.cur.t + (L.table.b - L.table.t));   /* (2.4.4) + the strip */ if (bp::overlap(c, bp::mk(a.l, a.t, a.r, a.b))) blocked = true; }
            bp::Box got = S->hyst.step(want, blocked);
            const int tw = L.table.r - L.table.l, th = L.table.b - L.table.t, sh = L.gexOk ? L.table.t - L.gexRow.t : 0;   // (2.4.4) + the Gex S/R strip above
            if (got.l < pb.l || got.t - sh < pb.t || got.l + tw > pb.r || got.t + th > pb.b) { S->hyst.reset(); got = S->hyst.step(want); }   // never kept outside the pane
            const int dx = got.l - L.table.l, dy = got.t - L.table.t;
            if (dx || dy) { for (size_t i = 0; i < L.items.size(); ++i) if (L.items[i].role == hl::R_TABLE || L.items[i].role == hl::R_NOTE) { L.items[i].l += dx; L.items[i].r += dx; L.items[i].t += dy; L.items[i].b += dy; }
                            L.table.l += dx; L.table.r += dx; L.table.t += dy; L.table.b += dy;
                            if (L.gexOk) { L.gexRow.l += dx; L.gexRow.r += dx; L.gexRow.t += dy; L.gexRow.b += dy; }
                            if (L.qOk) { L.qBox.l += dx; L.qBox.r += dx; L.qBox.t += dy; L.qBox.b += dy; }
                            if (L.preOk) { L.preRow.l += dx; L.preRow.r += dx; L.preRow.t += dy; L.preRow.b += dy; }   // (2.6.0)
                            if (L.dayOk) { L.dayBox.l += dx; L.dayBox.r += dx; L.dayBox.t += dy; L.dayBox.b += dy; }   // (2.7.0)
                            for (size_t q = 0; q < L.noteRows.size(); ++q) { L.noteRows[q].l += dx; L.noteRows[q].r += dx; L.noteRows[q].t += dy; L.noteRows[q].b += dy; } }
        }
    }
    if (S->fxHave && S->fxView.called && S->fxIdx >= 0 && S->fxIdx < (int)getBarCount()) {   // (2.0.0) the LOD? / HOD? marker on the called candle
        PNT p; int mx = -1;
        if (p.set(S->fxIdx, (float)S->fxView.price, kBarCenter) == RTX_OK && p.h > 0) mx = (int)p.h;
        std::function<int(double)> yPx = [&](double v) { return (int)getVerticalDrawPosition((float)v); };
        const size_t m0 = L.items.size();
        hlfx::marker(L, S->fxView, pb, mx, yPx, M);
        hl::tagElem(L, m0, hl::E_MARK);
    }
    {   // (2.1.0) the GEX row's hover: open while the mouse is on the row (or its detail); closed once the cursor left this pane
        if (S->inGex) { int cx = 0, cy = 0; if (hlCursor(cx, cy) && (std::abs(cx - S->gexCurX) > 8 || std::abs(cy - S->gexCurY) > 8)) S->inGex = false; }
        if (HL_HOVERS && S->inGex && L.gexOk) hl::layoutGexHover(L, S->view, pb, pe > 0 ? pe : pb.r, M);
        if (S->inQ) { int cx = 0, cy = 0; if (hlCursor(cx, cy) && (std::abs(cx - S->qCurX) > 8 || std::abs(cy - S->qCurY) > 8)) S->inQ = false; }   // (2.3.0) the Q-Scores' detail
        if (HL_HOVERS && S->inQ && L.qOk && !S->inGex) hl::layoutQHover(L, S->view, pb, pe > 0 ? pe : pb.r, M);
        if (S->inPre) { int cx = 0, cy = 0; if (hlCursor(cx, cy) && (std::abs(cx - S->preCurX) > 8 || std::abs(cy - S->preCurY) > 8)) S->inPre = false; }   // (2.6.0) the Pre row's detail
        if (HL_PRE_HOVER && S->inPre) {   // (2.7.0) the hovered row's detail: Pre / day type / a note row
            const std::vector<hl::Line>* hv = nullptr; const int w = S->hovWhich;
            if (w == 0 && L.preOk) hv = &S->view.preHover;
            else if (w == 1 && L.dayOk) hv = &S->view.dayHover;
            else if (w >= 2 && (size_t)(w - 2) < S->view.notes.size() && (size_t)(w - 2) < L.noteRows.size()) hv = &S->view.notes[(size_t)(w - 2)].hover;
            if (hv) hl::layoutRowHover(L, *hv, pb, pe > 0 ? pe : pb.r, M);
        }
    }
    hl::applyOpts(L, O);                      // (2.0.5) the Settings checkboxes and colours
    S->lastGexOk = L.gexOk && L.tableOk; S->lastGexRow = L.gexRow; S->lastHoverOk = L.gexHoverOk && L.tableOk; S->lastGexHover = L.gexHover;
    if (!S->lastGexOk) S->inGex = false;
    S->lastQOk = L.qOk && L.tableOk; S->lastQBox = L.qBox; S->lastQHoverOk = L.qHoverOk && L.tableOk; S->lastQHover = L.qHover;
    if (!S->lastQOk) S->inQ = false;
    S->lastPreOk = L.preOk && L.tableOk; S->lastPreRow = L.preRow; S->lastPreHoverOk = L.preHoverOk && L.tableOk; S->lastPreHover = L.preHover;   // (2.6.0)
    S->lastTargets.clear();                                                // (2.7.0) every row with a hover
    if (L.tableOk) {
        if (L.preOk) S->lastTargets.push_back(std::make_pair(0, L.preRow));
        if (L.dayOk) S->lastTargets.push_back(std::make_pair(1, L.dayBox));
        for (size_t q = 0; q < L.noteRows.size(); ++q) S->lastTargets.push_back(std::make_pair(2 + (int)q, L.noteRows[q]));
    }
    S->lastPreOk = !S->lastTargets.empty();
    if (!S->lastPreOk) S->inPre = false;
    S->placeLine += hlq::statusLine(S->qf, S->qWhy, S->qJudged ? &S->qV : nullptr) + "Q_BOX," + (S->lastQOk ? std::to_string(L.qBox.l) + "," + std::to_string(L.qBox.t) + "," + std::to_string(L.qBox.r) + "," + std::to_string(L.qBox.b) : std::string("none"))
                  + ",hover " + (S->lastQHoverOk ? std::to_string(L.qHover.l) + "," + std::to_string(L.qHover.t) + "," + std::to_string(L.qHover.r) + "," + std::to_string(L.qHover.b) : std::string("off")) + ",hovers " + std::to_string(S->qHovers) + "\n";
    S->placeLine += hlg::statusLine(S->gex, S->view, S->gexWhy) + "GEX_BOX," + (S->lastGexOk ? std::to_string(L.gexRow.l) + "," + std::to_string(L.gexRow.t) + "," + std::to_string(L.gexRow.r) + "," + std::to_string(L.gexRow.b) : std::string("none"))
                  + ",hover " + (S->lastHoverOk ? std::to_string(L.gexHover.l) + "," + std::to_string(L.gexHover.t) + "," + std::to_string(L.gexHover.r) + "," + std::to_string(L.gexHover.b) : std::string("off")) + ",hovers " + std::to_string(S->gexHovers) + "\n";
    {   // (2.4.0) the regime's source, for the status file (FLIP / the session GAM are shown only to prove they are not used)
        const int dec = hl::MKTS[S->mi < 0 ? 0 : S->mi].dec;
        S->placeLine += std::string("REGIME,") + (S->regFromGex ? std::string(S->regSign > 0 ? "Pos" : "Neg") + " (from Gex S/R HVL0 " + hl::trimNum(S->regGexHvl, dec) + ", LIVE)"
                                                 : std::string(S->regV.stale() && S->hvl.ok ? "STALE" : (S->regSign > 0 ? "Pos" : (S->regSign < 0 ? "Neg" : "none"))))
                      + ",src GammaProfile-MQ-" + S->mkt + ".csv HVL0 " + (S->hvl.ok ? S->hvl.hvlTxt : std::string("-"))
                      + ",close " + (std::isfinite(S->lastClose) ? hl::trimNum(S->lastClose, dec) : std::string("-"))
                      + ",data " + (S->hvl.ok ? std::string(fr::word(S->regV.st)) + " " + fr::ageShort(S->regV.ageSec) : std::string("-"))
                      + ",FLIP " + (S->hvl.flipTxt.empty() ? std::string("-") : S->hvl.flipTxt) + " not used,session GAM " + (S->sessGam.empty() ? std::string("-") : S->sessGam) + " not used"
                      + (S->hvlWhy.empty() ? "" : "," + S->hvlWhy) + "\n";
    }
    {   // (2.6.0) the PRE-OPEN BOOK's status
        const long long nowMs = hlNowCt() * 1000LL; const po::Engine& E = S->pre;
        S->placeLine += S->dayLine + ",logged " + std::to_string(S->dayLogs) + ",log fails " + std::to_string(S->dayLogFails) + "\n" + S->newsLine + "\n";   // (2.7.0)
        S->placeLine += "PREOPEN," + std::string(po::VERSION) + ",state " + S->preState + ",show " + (S->preShow ? "on" : "off") + ",param SHOW " + mp::modeName(S->preParamShow) + " (" + S->preParamWhy + ")" + ",window " + std::to_string(E.P.windowMin) + " min"
                      + (E.dayMs >= 0 ? ",session " + po::sessionKey(E.dayMs) + ",open " + po::hm12(E.openMin) : std::string()) + ",near " + std::to_string(E.nearTicks) + " ticks,s20 " + std::to_string(E.s20)
                      + ",snap " + (E.snapDone ? "done" : "no") + ",imb " + (E.imbDone ? "done" : "no") + ",restored " + std::to_string(S->preRestored) + ",written " + std::to_string(S->preWrites) + ",fails " + std::to_string(S->preWriteFails)
                      + ",line '" + S->preLine.s + "',row " + (S->lastPreOk ? std::to_string(S->lastPreRow.l) + "," + std::to_string(S->lastPreRow.t) + "," + std::to_string(S->lastPreRow.r) + "," + std::to_string(S->lastPreRow.b) : std::string("none")) + ",hovers " + std::to_string(S->preHovers) + "\n"
                      + "PREFEED," + S->preFeed.stateText(hlNowMs()) + ",via " + (S->preFeed.via.empty() ? std::string("-") : S->preFeed.via) + ",opens " + std::to_string(S->preFeed.opens) + ",events " + std::to_string(S->preFeed.events)
                      + ",routed " + std::to_string(S->preRouted) + ",book " + std::to_string(E.book.size()) + ",ignored " + std::to_string(E.ignored) + ",caps " + std::to_string(S->preFeed.capHits) + ",faults " + std::to_string(S->preFeed.faults)
                      + ",clock " + (S->preTimerOn ? "on" : "off") + ",want " + (E.wantFeed(nowMs) ? "yes" : "no") + "\n";
    }
    postMst(*S);                             // (2.4.0) the Market State (behind the scenes; nothing drawn changes)
    execute(L);
    {   // (2.4.1) the table's rectangle for the docked Big Lot box (BoxDock.h): table + Q header + GEX row, as drawn; none = cleared
        const char* sy = getSymbol(); const char* cl = getChartLabel();
        const std::string sym = sy ? sy : "", chart = cl ? cl : ""; const long spb = getSecondsPerBar();
        if (S->hlbPosted && (sym != S->hlbSym || spb != S->hlbSpb || chart != S->hlbChart)) { bdk::clearHodLod(S->hlbSym, S->hlbSpb, S->hlbChart); bdk::clearHodLodTable(S->hlbSym, S->hlbSpb, S->hlbChart); S->hlbPosted = false; }
        if (L.tableOk && L.table.r > L.table.l && L.table.b > L.table.t && !sym.empty()) {
            bdk::Box u = bdk::mk(L.table.l, L.table.t, L.table.r, L.table.b);
            auto add = [&](const hl::Box& b) { if (b.r > b.l && b.b > b.t) { u.l = (std::min)(u.l, b.l); u.t = (std::min)(u.t, b.t); u.r = (std::max)(u.r, b.r); u.b = (std::max)(u.b, b.b); } };
            if (S->lastQOk) add(L.qBox);
            if (S->lastGexOk) add(L.gexRow);
            const long long ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            bdk::publishHodLod(sym, spb, chart, ms, u);
            bdk::publishHodLodTable(sym, spb, chart, ms, bdk::mk(L.table.l, L.table.t, L.table.r, L.table.b));   // (2.4.4) the table alone: the Big Lot box takes its height
            S->hlbSym = sym; S->hlbSpb = spb; S->hlbChart = chart; S->hlbPosted = true;
            S->placeLine += "HLB_BOX," + std::to_string(u.l) + "," + std::to_string(u.t) + "," + std::to_string(u.r) + "," + std::to_string(u.b) + " (published for the Big Lot box)\n";
        } else if (S->hlbPosted) { bdk::clearHodLod(S->hlbSym, S->hlbSpb, S->hlbChart); bdk::clearHodLodTable(S->hlbSym, S->hlbSpb, S->hlbChart); S->hlbPosted = false; S->placeLine += "HLB_BOX,none\n"; }
    }
    writeStatus(*S, &L);
    writeFxStatus(*S);
    return RTX_OK;
}

// (2.4.0) MARKET STATE: SRC "HODLOD" + "TREND" for the last CLOSED bar. The facts are decided once for that bar; the slow state (regime,
// Q, band, age) may change them within the bar (MARKET-STATE-SPEC: "and when a slow state changes"). The same facts are re-posted every
// 10 s (MarketState.h ignores entries older than 60 s, so a 3-min bar must be refreshed to stay visible); cleared in clearState().
void HodLod::postMst(HLState& S)
{
    if (S.mi < 0 || S.lastClosedAbs <= 0) return;
    const char* sy = getSymbol(); const char* cl = getChartLabel();
    const std::string sym = sy ? sy : "", chart = cl ? cl : ""; const long spb = getSecondsPerBar();
    if (sym.empty()) return;
    if (S.mstPosted && (sym != S.mstSym || spb != S.mstSpb || chart != S.mstChart)) { mst::clear("HODLOD", S.mstSym, S.mstSpb, S.mstChart); mst::clear("TREND", S.mstSym, S.mstSpb, S.mstChart); S.mstPosted = false; }
    hl::MstIn in; in.tick = S.tick; in.dec = hl::MKTS[S.mi].dec; in.regime = S.regSign;
    in.dataAgeMin = (S.hvl.ok && S.regV.known) ? S.regV.ageSec / 60 : -1;
    in.qOk = S.qf.ok && S.qJudged && !S.qV.stale(); in.qm = S.qf.m; in.qo = S.qf.o; in.qv = S.qf.v;
    in.bandOk = S.gex.ok && S.gex.band && hlg::showable(S.gex) && S.view.gexStale.empty(); in.bandHi = S.gex.bandTop; in.bandLo = S.gex.bandBot;
    std::map<std::string, std::string> kv = hl::mstHodLod(S.view, in);
    std::string sig; for (const auto& p : kv) if (p.first != "data_age_min") sig += p.first + "=" + p.second + ";";
    const long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    const bool newBar = S.lastClosedAbs != S.mstBar, changed = sig != S.mstSig;
    if (newBar || changed) { S.mstKv = kv; S.mstTrendKv = hl::mstTrend(S.view); S.mstBar = S.lastClosedAbs; S.mstSig = sig; }
    if (!(newBar || changed || !S.mstPosted || S.mstMs < 0 || now - S.mstMs >= 10000 || now < S.mstMs)) return;
    mst::publish("HODLOD", sym, spb, chart, now, S.mstBar, S.mstKv);
    mst::publish("TREND", sym, spb, chart, now, S.mstBar, S.mstTrendKv);
    S.mstSym = sym; S.mstSpb = spb; S.mstChart = chart; S.mstPosted = true; S.mstMs = now;
}

// (2.1.0) over the GEX row (or its open detail, 6 px dead band so the edge never flickers): the detail shows. Never swallows the event.
int HodLod::mouse(RTX_EVENT* e)
{
    if (!e) return RTX_FAIL;
    HLState* S = state(false); if (!S) return RTX_FAIL;
    if (HL_PRE_HOVER && (e->type == E_MOUSE_MOVE || e->type == E_MOUSE_HOVER)) {   // (2.6.0) the Pre row (or its open detail): the one hover
        const int mh = e->v.mouse.h, mv = e->v.mouse.v, g = S->inPre ? 6 : 0;
        auto inB = [&](const hl::Box& b) { return mh >= b.l - g && mh <= b.r + g && mv >= b.t - g && mv <= b.b + g; };
        int which = -1; for (size_t q = 0; q < S->lastTargets.size() && which < 0; ++q) if (inB(S->lastTargets[q].second)) which = S->lastTargets[q].first;   // (2.7.0) Pre / day / notes
        const bool ip = S->lastPreOk && (which >= 0 || (S->inPre && S->lastPreHoverOk && inB(S->lastPreHover)));
        if (ip) hlCursor(S->preCurX, S->preCurY);
        if (which >= 0 && which != S->hovWhich) { S->hovWhich = which; if (S->inPre) invalidateChart(false); }
        if (ip != S->inPre) { S->inPre = ip; if (ip) ++S->preHovers; invalidateChart(false); }
    }
    if (!HL_HOVERS) return RTX_FAIL;                                         // (2.4.6) no hovers: the mouse is never used
    if (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER) return RTX_FAIL;
    const int mh = e->v.mouse.h, mv = e->v.mouse.v, g = S->inGex ? 6 : 0;
    auto inB = [&](const hl::Box& b) { return mh >= b.l - g && mh <= b.r + g && mv >= b.t - g && mv <= b.b + g; };
    const bool in = S->lastGexOk && (inB(S->lastGexRow) || (S->inGex && S->lastHoverOk && inB(S->lastGexHover)));
    if (in) hlCursor(S->gexCurX, S->gexCurY);
    if (in != S->inGex) { S->inGex = in; if (in) ++S->gexHovers; invalidateChart(false); }
    {   // (2.3.0) the Q-Scores (or their open detail)
        const int gq = S->inQ ? 6 : 0;
        auto inQB = [&](const hl::Box& b) { return mh >= b.l - gq && mh <= b.r + gq && mv >= b.t - gq && mv <= b.b + gq; };
        const bool iq = !in && S->lastQOk && (inQB(S->lastQBox) || (S->inQ && S->lastQHoverOk && inQB(S->lastQHover)));
        if (iq) hlCursor(S->qCurX, S->qCurY);
        if (iq != S->inQ) { S->inQ = iq; if (iq) ++S->qHovers; invalidateChart(false); }
    }
    return RTX_FAIL;
}

extern "C" cppExtension *CreateExtension(void)
{
    HodLod *p = new HodLod();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | INSTRUMENT_SCALE | ((HL_HOVERS || HL_PRE_HOVER) ? TRACK_MOUSE : 0));   // (2.6.0) TRACK_MOUSE for the Pre row's hover only   // (2.4.6) no TRACK_MOUSE while the hovers are off   // (2.1.0) TRACK_MOUSE: the GEX row's hover
    p->setDescription("LRA HOD / LOD: one compact table - Regime, the RTH hours and the options expiry; Expected (1st / 2nd time and their odds) and Actual (the extreme with its IN odds); Estimate (range used, the projected opposite extreme). GEX: the strikes where a reaction can occur (top 3 abs GEX within 1 expected move or growing, from LRA-GexNodes-<MKT>.csv) - mouse over the row for the detail. A projection line from the 1st extreme to the target with its label on the arrowhead. The 1st Extreme model runs in the background (its call box on the candle). Use Settings > Position to place it. 2.3.0: MenthorQ Q-Scores top left (mouse over for the meaning), Reg / trend top right; Gex S/R = the 3 biggest abs-GEX strikes from the RTH open up to 1.38x the 1D Max (red) and down to 1.38x the 1D Min (green). 2.4.0: Reg = price vs the MenthorQ HVL the Dealer Profile draws; stale data reads STALE (Reg / Q / Gex row), never as live. 2.6.0: the Pre row - the pre-open MBO book (bids vs offers near price in the last N minutes before the open, the share that stayed, the opening imbalance) and an untested lean for the day's first extreme, shown from 30 min before the open to 60 min after; mouse over it for the detail.");
    p->setVersion("2.7.0");   // = hl::HL_VERSION = hlset::PLUGIN_VERSION (gex-build reads this literal; t_hl240 checks they match)
    return p;
}
