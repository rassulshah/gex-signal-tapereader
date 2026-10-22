/********************************************************************************
 *  BigBidAsk.cpp  --  Investor/RT RTX extension  lsBigBidAsk  1.0.0  (2026-10-14 night)
 *  1.26.0 (2026-10-22 evening, Rassul 18:01 CT: "you only put the volume at the bid or ask like we currently do at the bo or the second
 *         high volume bid or ask if it exists near the pivot using the pivot rule, its as simple as that"): the 1.25.0 OB / BO
 *         RECTANGLES ARE GONE (no boxes, zones or extended bands - resting-limit zones come only from lsLiquidityProfile's LMT zones).
 *         BO: the breakout signal (bba::boWaves, unchanged rules: 10-candle extreme, range / volume, green / red, same wave) now breaks
 *         an INTACT SWING - one of the last 3 intact swings of his 1-bar ZigZag (bba::swingBreaksPx); its print as before; the letter
 *         is "BO" for both sides: green ABOVE a breakout candle, red BELOW a breakdown candle (was BD). BBO lines of the old pivot rule
 *         (before 1.26.0) are not read back. OB (OrderBlockLogic.h 2.0.0): for a strong wave (an Rly wave, or a confirmed wave of 3+
 *         ATR - unchanged), the highest-volume bid / ask cell on the pivot candle and the next 1-3 candles (setting, default 2); when
 *         the BO mark of one of those candles already shows that print, the 2nd highest (if any). Drawn like a print (lots in the side's
 *         box) + "OB" BELOW the candle of an up-wave / ABOVE for a down-wave; decided once when the wave is confirmed, recorded (OBX|,
 *         field rank), never repainted; visits to its price logged (OBT|) for the nightly. Hover on the OB letter. Settings: the "BO
 *         boxes" row is gone, "OB candles after the pivot" 1 - 3 (parameter version 13).
 *  1.25.0 (2026-10-22, agreed with Rassul 16:00-16:01 CT on his 13:55 sketch "a strong wave starts at a low (the ORDER BLOCK) and breaks
 *         through the prior pivot high (the BREAKOUT box) ... both should hold big prints"): OB + BO BOXES of STRONG WAVES on 3-min charts
 *         (OrderBlockLogic.h 1.0.0). Strong wave = (a) a wave that earned an Rly (normal strength) or (b) any confirmed wave >= 3.0 ATR
 *         (setting; lighter, never grey). When the wave is confirmed the host looks BACK: OB = its first 1-2 candles (setting) from the
 *         pivot to their high (up; mirror down, capped at 40% of the wave); BO = the candle through the prior ZigZag pivot, within the
 *         pivot +- 0.5 ATR (none if it overlaps the OB). Inside each box: the BIG PRINTS of the box candles (a cell >= the session's Big
 *         Lot 20 tier, or the candle's own Big Bid / Ask print) as dots sized by lots, buyers green / sellers red; no big print = FAINT
 *         (border only). Boxes extend right until a candle CLOSES through them (or the session ends) and are never repainted (OBX| record in
 *         lsFlexLevels\OrderBlock\<stem>-<session>.csv, the record wins after a restart); every visit is a test (held / swept / broke, OBT|
 *         lines, appended once) for the nightly; the box border brightens while price trades in it. Labels "OB" / "BO" at the box's left
 *         edge, never over a print, a BO / BD / Rly letter or a box of the chart. Hover on the box: wave stats, range, big prints, tests,
 *         question / what / why / how to use / example, "?" = not proven yet. Settings: OB / BO checkboxes, strong waves without trend,
 *         the x ATR threshold (3.0), origin candles (2), up / down colours (parameter version 12).
 *  1.24.0 (2026-10-22, Rassul approved the Rly mockup v0.2 at 15:10-15:15 CT: "keep 4 a day as it is now"): RLY (rally) on 3-min
 *         charts, RallyLogic.h 1.0.0 = an exact port of rly2.py v0.2 (tested: every ES / NQ session of the research bars, 0 differences).
 *         On the END bar of a CONFIRMED 1-bar ZigZag wave (decided on the bar where the ZigZag turns): 3+ bars, size >= 2.5 x ATR(14
 *         Wilder, chart bars incl. overnight), a bar of the wave tagged +1 SD (RTH session VWAP from the open), a 20-bar high, vwapTrend
 *         (from the 10th RTH bar, last up-to-20 bars) Uptrend / Strong Uptrend = "Rly" green ABOVE the bar; the mirror = "Rly" red
 *         BELOW. Every judged wave (fired or not, with the failed conditions) is recorded to lsFlexLevels\Rly\<MKT>-<CONTRACT>-<spb>-
 *         <session>.csv and never judged again (no repaint); the chart draws from the record. Short text, no box / border, never over a
 *         print, a BO / BD letter or a box. Hover on the Rly text (TRACK_MOUSE is on again ONLY for the Rly hover; the print / Big Lot
 *         box hovers stay off as 1.17.1). Settings: "Rally (Rly)" checkbox + Rly up / Rly down colours (parameter version 11).
 *  1.23.0 (2026-10-22, Rassul approved the BO / BD rules; built on his 11:00 CT "go restart"): BO only on a GREEN candle, BD only
 *         on a RED candle. SAME-WAVE rule: a wick-only break of the pivot is confirmed when the breakout candle OR any later candle of
 *         the same up-wave (BO) / down-wave (BD) CLOSES beyond the pivot ("as long as there is a close in the same upwave"); decided on
 *         closed candles, the mark goes ON the confirming candle and never repaints (bba::boWaves, a pure function of the closed
 *         candles + the record). No FB mark: a failed wick break (the wave ended first) is logged to the record only (wick_wave_ended).
 *         Record lines now 24 fields (t0 = breakout candle, t = mark candle); the 1.21 / 1.22 lines are ignored (not redrawn).
 *         Context, not an entry: the BO / BD test found close-beyond and same-wave breaks continue ~49%, the same as a random candle
 *         of the wave - "a confirmed break: don't fade against it". No other letter changed.
 *  1.21.0 (2026-10-21, Rassul approved breakout-mockup-v2 at 20:10 CT: "no dashed pivot line. yes build"): BREAKOUT / BREAKDOWN.
 *         A candle that trades THROUGH the higher of the last two pivot highs (bba::breakFlagsPx, his 1-bar ZigZag), makes the highest
 *         high of the last 10 candles, has range >= the prior candle's and (volume > prior OR range > prior) = BO; the mirror = BD.
 *         Decided once at the candle's close (bba::breakoutAt), never repainted; every decision (fired or not, with the reason) is kept
 *         in lsFlexLevels\BigBidAsk\<stem>-breakouts-<session>.csv (BBO| lines) for the nightly and reloaded on a restart.
 *         On the chart: "BO" green above / "BD" red below the candle; the breakout PRINT (the candle's biggest buy cell for BO / sell
 *         cell for BD, drawn only when bigger than the prior candle's biggest of that side) = its LOTS, white in a box outlined green /
 *         red, ON the candle at its price. A print that is already a drawn Big Bid/Ask print is drawn once (that print + the letter).
 *         No pivot line. Kept out of the HOD/LOD table / Big Lot box / Tape Reader banner like the prints; the skip first / last
 *         minutes apply. Setting "Breakout / breakdown (BO / BD)" (on) + BO / BD colours. Hovers stay off (1.17.1): the details of
 *         today's breakouts are on the status file (BREAKOUTS / BO_EVENT lines).
 *  1.17.0 (2026-10-18, Rassul approved the mockup 19:57 CT, "go"): NO USER SETTINGS - the settings window is empty ("the settings window
 *         would be empty" = yes). Fixed behaviour (what his chart showed, BigBidAsk.settings-ES.txt of 17:15): category 1 = Big Lot 5,
 *         all prints, cyan numbers; category 2 = Big Lot 10, Trend PB + BO; category 3 = Big Lot 20, Trend PB + BO (white numbers);
 *         category 4 (>= lots) REMOVED; font 8; boxes on (red at the bid EF4444, green at the ask 22C55E); no circle, no dim, no level
 *         filters; skip the first / last 15 min of each session (RTH + overnight); divider = the market default; the Big Lot box on,
 *         docked beside the HOD/LOD table (1.16.2). BigBidAsk.settings-<MKT>.txt is no longer read or written (old files are left alone).
 *         Parameter version 7 with no rows: IRT drops the 87 old rows. Status: "SETTINGS,fixed (1.17.0, no settings)".
 *  1.16.2 (2026-10-18, Rassul 17:13 CT "build"; "the BLS box should be the same height as the hod lod box"): docked beside the
 *         HOD/LOD TABLE (lsHodLod 2.4.4 publishes the table alone as LRA_HLT_, and table + Gex S/R strip as LRA_HLB_) the box takes the
 *         table's top and bottom, its 3 rows spread evenly; never over the strip / table / banner / price (BoxDock.h placeMatched).
 *         BLANK-SAVE GUARD: his BigBidAsk.settings-ES.txt was overwritten at 15:07:55 CT with every box off and every colour 0 ("where
 *         are all the big prints? they are all gone") - a window copy that read blank was saved. Now a blank read (SettingsHost.h 1.1.0
 *         blankRead) or a blank-shaped result (bba::looksBlank) is never written; the refusal is logged in BigBidAsk.dialog-<MKT>.txt.
 *         "Show the Big Lot box" stays checked by default (his restored file has it on).
 *  1.16.0 (2026-10-18, Rassul approved 15:03 CT): THE COMPACT BIG LOT BOX - "make it simple like BL 5 1434 and just have a compact box
 *         of the averages connected to the hod lod box when it is added to the chart so it doesn't overlap"; "no, just the avg". The
 *         6-line statistics box is replaced by 3 rows BL5 / BL10 / BL20 = the rolling average (latest 5 complete sessions, one outlier
 *         dropped) of the CURRENT session type (RTH during RTH, overnight overnight), right-aligned numbers, solid border, white text.
 *         Docked to the lsHodLod table (BoxDock.h: HodLod 2.4.1 publishes its table rectangle) beside / above / below it with a 4 px
 *         gap, moving with it, never over it, the Tape Reader banner, the profiles or right of the last bar; no table = its own
 *         9-spot Position. Prior session, 4-session avg, today's N of ~N, the other session type, the outlier, scoring -> its hover.
 *         + (Rassul 15:04-15:07 "cat 1 and 4 should be cyan", "i defined cat 4 because it is a user input") category order 1 > 4 > 2 / 3
 *         in the chart, the print log and the board events (bba::catOrder): a print >= his category-4 size takes category 4 unless it
 *         also reaches category 1.
 *         + SETTINGS WINDOW (Rassul 15:10 screenshot): colours are filled / read with setParameterColor / getParameterColor (the
 *         setIntegerValue / getIntegerValue of 1.11-1.15 left the Box bid / ask swatches black); the window is re-laid out to fit
 *         (each category on 2 lines: size / font, then Show / bid / ask colours; shorter labels) - setParameterVersion 6, IRT resets the
 *         saved rows once (stale rows from older layouts showed Divider 7, a red circle colour); Divider shows the real value (the
 *         market default when 0: ES / CL 10, others 1; typing the default back stores 0); a level group box unchecks / re-checks its
 *         levels and greys them out (enableParameter) while off, and an off group uses none of its levels on the chart; parmsLoad /
 *         save write lsFlexLevels\BigBidAsk.dialog-<MKT>.txt (what was filled and read back) so a window that ignores the fill shows.
 *
 *  BIGGEST BID / ASK ON EVERY CANDLE. Rassul approved the mockup at 20:41 CT 2026-10-14 (mockup-biggest-bid-ask.png v4; the thin
 *  absorption circle as drawn in v5). Put ONE on the PRICE pane of each chart.
 *    - RED number LEFT of each candle, at the price where the most lots traded AT THE BID in that candle (sellers hitting); the
 *      number = that price's bid lots. GREEN number RIGHT of the candle, at the price where the most lots traded AT THE ASK (buyers
 *      lifting). Ties go to the price nearest the close. No dash or tick mark, just the number (Font size, default 11).
 *    - BOLD = at least "Bold at" x (default 2.0x) the median of the same side's biggest over the prior 20 candles (past only).
 *    - ABSORPTION = a thin 1 px rounded outline round the number, in its own colour, decided once at the candle's close and never
 *      repainted: bullish = bold red at / below the halfway mark and the candle closes green above it; bearish mirror. The forming
 *      candle never gets one.
 *    - Hover a number: "Biggest bid 7,741.25: 410 lots sold at the bid (3.1x normal) . biggest ask 7,743.50: 160 lots (1.2x)" and
 *      what it answers / why / how to use it / an example; a circled number explains the absorption rule.
 *    - No overlap: own row, one row up, one row down, never on a candle, another number, the left Liquidity column or the right-side
 *      columns (ColumnBoard.h - read only, this plugin publishes no column); no room = not drawn, except bold ones.
 *  DATA: the chart's own volume at price per bar (barVolumeProfile, VAP_REQUIRED - the same source as lsDeltaProfile): per tick,
 *  never bucketed; bid lots = VOLPROFILE.sellVolume, ask lots = buyVolume; whole-tick keys (llround(price / SYM_TICKINCR)).
 *  NEVER REPAINTS: each closed candle's facts (prices, lots, ratios) are frozen at its close and recorded to
 *  lsFlexLevels\BigBidAsk\<MKT>-<CONTRACT>-<sec>-<session>.csv; a restart redraws from the record. Rules: BigBidAskLogic.h (tested).
 *  Settings (controls only, per market, lsFlexLevels\BigBidAsk.settings-<MKT>.txt, written only from OK / Apply): Font size,
 *  Bold at (x normal), Absorption circle, Show non-bold numbers. Status: lsFlexLevels\BigBidAsk.status-<MKT>-<CONTRACT>-<sec>.txt.
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
#include "BigBidAskLogic.h"
#include "BigBidAskSettings.h"         // (1.19.0) the settings table (Biggest in, skip first / last, font, colours)
#include "SettingsHost.h"
#include "SettingsGuard.h"         // (1.19.2) the window calls behind a fault guard; the market read safely              // (1.19.0) the window <-> file layer every indicator uses
#include "ColumnBoard.h"                 // where the price area ends (the right-side columns) and the left Liquidity column - read only
#include "HoverBoard.h"                  // hb::hbCursor (GetCursorPos + WindowFromPoint; carries the user32 pragma)
#include "HostSlot.h"
#include "BoxDock.h"                 // (1.16.0) the Big Lot box docks to the HOD/LOD table (lsHodLod 2.4.1 publishes it)
#include "AbsorbLogic.h"           // (1.20.0) the absorbed-print circle (order-book truth from lsBidAskStats, board kind O) - replaced 1.22.0
#include "SharedAbsorb.h"           // (1.22.0) the circle only on a SHARED A candle (one rule, one record for every plugin)
#include "SignalBoard.h"           // (1.2.1) posts its bold sizes / circles for the other indicators on this chart
#include "ZoneNear.h"              // (1.2.0) location first: bold / circles away from a zone are dimmed
#include "RallyLogic.h"             // (1.24.0) the Rly (rally) signal, rly2.py v0.2 port
#include "MarkParams.h"             // (1.25.1) LRA-OrderBlock-<MKT>.csv PARAM,SHOW - read for the status only since 1.26.0 (his 18:01 request shows the OB / BO letters)
#include "OrderBlockLogic.h"        // (1.26.0) the OB print of a strong wave (2.0.0; the 1.25.0 boxes are gone)
#include "VwapTrendLogic.h"         // (1.13.0) his vwapTrend, natively from the chart's bars (the pullback setup)
#include "KeyLevelsLogic.h"        // (1.11.0) the key levels from the chart's own bars (same code as lsDealerProfile), per candle
#if defined(_MSC_VER)
#pragma comment(lib, "user32.lib")      // HoverBoard.h's GetCursorPos / WindowFromPoint
#endif
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
#include <functional>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif

#define BBA_VERSION "1.26.0"          // keep equal to bba::VERSION and the setVersion literal below
// (1.17.1, Rassul 23:37 CT 2026-10-18 "get rid of the hover over on the gex levels and other places") the print-number and Big Lot box hovers are OFF (BBA_HOVERS; the code is kept), no TRACK_MOUSE
static const bool BBA_HOVERS = false;
// (1.15.0, 2026-10-18 audit B finding 2) NO SIZE REPAINT: the Big Lot tiers of a candle are those of ITS session, decided once from the
// complete sessions before it (bba::tiersFor) and only after the chart history before that session is fully read; the facts come from the
// last 8 sessions of the chart whatever is in view (and are recorded for 8 sessions); SignalBoard B events from the record for the last
// 120 closed candles (not the view), bit 7 = pivot confirmed; the 5-s log-normal fit is gone (SIZES = the S-tiers in force).

#if defined(_WIN32)
#define BBA_SEP "\\"
#else
#define BBA_SEP "/"
#endif

static const COLOR C_BUY = 0x0022C55E, C_SELL = 0x00EF4444;
static const COLOR C_BUY_DIM = 0x00157A3D, C_SELL_DIM = 0x00933030;   // (1.1.0, Rassul 10-15 "like the 96") normal-size numbers dim, the big ones full colour           // buyers green, sellers red (the Delta Profile's colours)
static const COLOR C_HOVBG = 0x00111827, C_HOVEDGE = 0x009CA3AF, C_INK = 0x00F3F4F6;
static const COLOR C_BLBG = 0x000F1520, C_BLEDGE = 0x00374151;   // (1.16.0) the Big Lot box = the HOD/LOD table's panel / border colours
static const int LEAD = 30;                 // candles laid out left of the view, so a scroll does not re-decide what stays in view
static const int READS_MAX = 900;           // VAP bars read per pass (the rest next pass)
static const int FAILS_FINAL = 5;           // a closed candle IRT has no VAP for after 5 passes is finalised as "no data"
static const size_t REC_FILE_MAX = 2 * 1024 * 1024, REC_LINES_MAX = 20000;

static long long steadyMs() {
    return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static short coord(int v) { return (short)(v < SHRT_MIN ? SHRT_MIN : v > SHRT_MAX ? SHRT_MAX : v); }
static std::string flexDir() {
    const char* up = std::getenv("USERPROFILE");
    if (!up || !up[0]) return std::string();
    return std::string(up) + BBA_SEP "InvestorRT" BBA_SEP "rtx" BBA_SEP "lsFlexLevels";
}
static void makeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0755);
#endif
}
// the file's stamp: last-write time to 100 ns / 1 ns + size (a second write within the same second still changes it)
static bool statFile(const std::string& p, long long& stamp) {
#if defined(_WIN32)
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(p.c_str(), GetFileExInfoStandard, &a) || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return false;
    const unsigned long long w = ((unsigned long long)a.ftLastWriteTime.dwHighDateTime << 32) | a.ftLastWriteTime.dwLowDateTime;
    stamp = (long long)(w & 0x3FFFFFFFFFFFULL) * 1009LL + (long long)a.nFileSizeLow; return true;
#else
    struct stat st; if (stat(p.c_str(), &st) != 0) return false;
    if ((st.st_mode & S_IFMT) != S_IFREG) return false;
    stamp = ((long long)st.st_mtim.tv_sec * 1000000000LL + (long long)st.st_mtim.tv_nsec) % 4000000000000000LL * 1009LL / 1000 + (long long)st.st_size; return true;
#endif
}
static bool moveOver(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
static bool publishFile(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp";
    { std::ofstream f(tmp.c_str(), std::ios::binary | std::ios::trunc); if (!f) return false;
      f.write(contents.data(), (std::streamsize)contents.size()); f.flush(); if (!f) { f.close(); std::remove(tmp.c_str()); return false; } }
    if (!moveOver(tmp, path)) { std::remove(tmp.c_str()); return false; }
    return true;
}
static bool readSmall(const std::string& path, size_t cap, std::string& out) {
    out.clear();
    std::ifstream in(path.c_str(), std::ios::binary); if (!in) return false;
    in.seekg(0, std::ios::end); const std::streamoff size = in.tellg();
    if (size < 0 || size > (std::streamoff)cap) return false;
    in.seekg(0, std::ios::beg); out.resize((size_t)size);
    if (size > 0) in.read(&out[0], size);
    return (bool)in || in.eof();
}
static std::string fileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) {
        const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_');
    }
    return out.empty() ? "UNKNOWN" : out;
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
    return fileKey(r.substr(0, 8));
}
static long long civilDays(int y, unsigned m, unsigned d) {
    y -= m <= 2; const long long era = (y >= 0 ? y : y - 399) / 400; const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}
// checklist #24: the chart's SYM_TICKINCR, made exact (it arrives as a float: 0.00005f = 4.99999987e-05)
static double cleanTick(float raw) {
    const double d = static_cast<double>(raw);
    if (!std::isfinite(d) || d <= 0 || d >= 1000) return 0;
    char b[32]; std::snprintf(b, sizeof(b), "%.6g", d);
    const double t = std::strtod(b, NULL);
    return std::isfinite(t) && t > 0 && t < 1000 ? t : 0;
}
static bool keyOf(double price, double tick, long long& key) {
    if (!std::isfinite(price) || tick <= 0) return false;
    const double q = price / tick;
    if (!std::isfinite(q) || std::fabs(q) >= 9.0e15) return false;
    key = (long long)std::llround(q); return true;
}

struct PickInfo { int cat = -1; std::string lv; double lvPx = 0; bool sweep = false, piv = false; int why = 0; int brk = 0; int trend = 0;
                  int how = 0, brkDir = 0; bool conf = true; };   // (1.14.0) how = 1 Trend PB / 3 Trend BO; brkDir +1 out / -1 down; conf = the pivot is final
// (1.14.0) what one pass knows about every bar: the swing (the candle's own pivot, may repaint until confirmed), the breakouts, the
// session VWAP and its SD at each bar (the Trend PB band rule)
struct PassCtx { bba::Swing sw; std::vector<signed char> bu, bd; std::vector<double> vw, sd; };   // (1.13.0) brk = the pivot broken (1 / 2), trend = vwapTrend level   // (1.12.0) why a print got its category
struct HoverT { bba::Box box; long long t = 0; int side = -1; PickInfo pi; long long lots = 0; double px = 0; };
struct BBState {
    sab::Host sab; long long sabCircles = 0, sabVeto = 0;   // (1.22.0) the shared A record (read only) + circles drawn / vetoed by the candle colour
    // (1.20.0, MBO Roadmap Top Defender #1) the ABSORBED-PRINT circle, decided once per closed candle and kept in
    // lsFlexLevels\BigBidAsk.abs-<MKT>-<spb>.txt ("t,mask": bit 0 the bid print / bit 1 the ask print absorbed; -1 = no order feed)
    std::map<long long, int> absC; std::map<long long, long long> absWait; bool absLoaded = false; std::string absPath;
    std::vector<sb::Ev> obEv; long long obReadMs = -1; std::vector<std::string> absJournal;
    long long absDecided = 0, absCircled = 0, absNoFeed = 0, absDrawn = 0;
    long long sdCheckMs = -1; std::string sdWhy = "learning";   // (1.15.0) the SIZES status line: the S-tiers in force (the 1.5 log-normal fit is gone)
    // (1.15.0) the Big Lot tiers per session (svl session key), decided ONCE (bba::tiersFor) and then fixed for every candle of that session;
    // covered = the chart's closed bars are all final from covSess on (covStart: back to the chart's first bar); bySess = prints per session
    std::map<long, bba::TierSet> tiers; long covSess = LONG_MAX; bool covStart = false;   // LONG_MAX = nothing final yet (never decide before compute ran)
    std::map<long, bba::SessPrints> bySess; size_t bySessN = (size_t)-1;
    long long boardLast = 0, boardMs = -1; int boardN = 0;   // (1.15.0) the SignalBoard B pass (from the record, not the view)
    bool busy = false;
    std::string identity, market, symbol, state = "starting", lastStatusText, settingsWhy = "defaults", chartKey;
    ss::Store setStore; ss::Values sv = ss::defaults(bbaset::spec(), -1); int topN = 10;   // (1.19.0) his settings
    int spb = 0; long long session = -1; double tick = 0;
    bba::Settings set; long long setStamp = -2, setCheckMs = -1; std::string setMarket;   // (1.17.0) fixed settings, set once per market
    std::map<long long, bba::Facts> fin;            // closed candles, final (from the record or decided here) - never changed
    std::map<long long, bba::Facts> raw;            // closed candles' prices / lots read from the chart (ratios not yet final)
    struct Fail { int n = 0; long long firstMs = 0; };
    std::map<long long, Fail> fails;                // closed candles with no VAP yet: passes failed, since when
    std::set<long long> recorded;                   // times already in the record
    std::vector<std::pair<long long, bba::Facts>> pending;   // (session, facts) to append
    bba::Facts forming; bool haveForming = false;
    long lastCount = -1; long long lastFirst = 0, lastLast = 0, builtMs = -1;
    long long missing = 0, mismatch = 0, recFails = 0, faults = 0, decided = 0; bool faulted = false;
    long long lastStatusMs = -1, lastRecordMs = -1;
    // draw
    cb::Peers peers; int priceRight = 0, leftLimit = 0; bb::Beat trBeat; bool trOn = false; bba::Box trBox;
    bba::LayoutStats lay; int clL = 0, clR = 0, clT = 0, clB = 0, clTr = 0, clBig = 0, clHl = 0, slidHl = 0, shifted = 0, areaFallback = 0; int droppedInView = 0; int circles = 0, circlesHidden = 0, zoneFull = 0, zoneDim = 0, hugeN = 0, catN[3] = {0, 0, 0}, pivotHidden = 0;
    int timeHidden = 0, levelHidden = 0, catN4[4] = {0, 0, 0, 0}, trendHidden = 0, unconfirmed = 0;
    // (1.11.0) market hours, the volume profiles, the levels per candle (frozen once the candle closed), the gamma file, the statistics
    int openMin = 510, closeMin = 900;
    struct VP { std::map<long long, long long> rth, full; int bars = 0; };
    std::map<long, VP> vp; long volNext = -1; long long volFirstT = 0; long volFirstSess = 0;
    std::map<long long, std::vector<bba::NamedLevel>> lvCache;
    std::vector<svl::Bar> svB; long svN = -1; long long svLast = 0; std::map<long long, int> trCache;
    long bandN = -2; std::vector<double> vw, sdv;   // (1.14.0) session VWAP / SD per bar (the Trend PB band rule)   // (1.13.0) the bars for vwapTrend, the trend per candle (frozen)
    std::vector<bba::NamedLevel> gam; long long gamCheckMs = -1; long gamSess = 0; std::string gamWhy = "not read";
    std::vector<std::string> stats; long long statsMs = -1; std::string statsWritten;
    // (1.16.0) the compact Big Lot box: both session types' averages, the box / table rectangles, the hover
    bba::TypeAvg blDay, blNight; std::string blDropDay, blDropNight, blTiers; bool blNightNow = false;
    bdk::Box blBox; bool blOn = false; bdk::Box hlBox; bb::Beat hlBeat; bool hlOn = false; bdk::Box hlTable; bool hlTableOn = false;   /* (1.16.2) the table alone (LRA_HLT_) */ std::string blWhy = "not drawn yet";
    std::set<std::string> logged; std::vector<std::pair<long long, std::string>> logPending; long long logMs = -1;   // (1.12.0) the print log
    std::string scoreLine = "Scoring: learning (needs the nightly run and 20+ prints per category).";
    // (1.21.0) BREAKOUT / BREAKDOWN: each closed candle decided once (candidates recorded as BBO| lines; dir 0 = no pivot traded through)
    std::map<long long, bba::BoDec> boDec; std::vector<std::pair<long long, std::string>> boPending; long long boLast = 0, boMs = -1, boFlushMs = -1, boRecFails = 0;
    bool boOn = true; unsigned boCol = 0x0022C55E, bdCol = 0x00EF4444;
    int boLetters = 0, boPrints = 0, boShared = 0, boKept = 0, boNoRoom = 0;
    // (1.23.0) the same-wave rule: boDec is keyed by the BREAKOUT candle (t0); the wick breaks still waiting for a close in their wave
    // ([1] BO / [0] BD, not recorded - rebuilt from the candles every pass); pre-1.23 record lines ignored (the old rule's letters)
    bool boWaitOn[2] = { false, false }; long long boWaitT0[2] = { 0, 0 }; double boWaitPiv[2] = { 0, 0 }; int boLegacy = 0, boDup = 0;
    // (1.24.0) RLY: every judged wave by key (bar-e time * 2 + up), from the record or decided once here; the drawn Rly boxes (hover)
    std::map<long long, rly::Rec> rlyRec; std::vector<std::pair<long long, std::string>> rlyPending; long long rlyLast = 0, rlyFlushMs = -1, rlyRecFails = 0; long rlyCount = -1;
    bool rlyOn = true; unsigned rlyUpCol = 0x0022C55E, rlyDnCol = 0x00EF4444; int rlyLetters = 0, rlyNoRoom = 0, rlyLoaded = 0, rlyBad = 0;
    struct RlyHov { bba::Box box; long long key = 0; }; std::vector<RlyHov> rlyTargets;
    // (1.25.0) OB / BO BOXES: every decided box by id (the record wins), its tests / end (a pure function of the closed bars, rebuilt on a
    // new bar), the boxes still waiting for their session's tiers or volume at price (id -> first try ms), the drawn boxes (hover)
    std::map<long long, obk::Box> obRec; std::map<long long, obk::Track> obTrk; std::map<long long, long long> obEndT; std::set<std::string> obTestDone;
    std::vector<std::pair<long long, std::string>> obPending; std::map<long long, long long> obWait; long long obLast = 0, obFlushMs = -1, obRecFails = 0, obRetryMs = -1; long obCount = -1;
    int obShowP = mp::LOG;   // (1.25.1) the nightly's verdict on the 1.25 OB / BO BOXES; 1.26.0 reports it, the letters do not depend on it
    bool obOn = true, obAny = true; double obAtr = 3.0; int obOrigin = 2; unsigned obUpCol = 0x0022C55E, obDnCol = 0x00EF4444;
    int obLabels = 0, obNoLabel = 0, obPrints = 0, obShared = 0, obLoaded = 0, obBad = 0, obLegacy = 0, obNoVap = 0;
    struct ObHov { bba::Box box; long long id = 0; }; std::vector<ObHov> obTargets;
    std::vector<HoverT> targets; bool msHas = false; int msH = 0, msV = 0; unsigned long long msWnd = 0; std::string hovKey;
};

class BigBidAsk : public cppExtension {
public:
    SS_SAFE_DIALOG(BigBidAsk, "BigBidAsk")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    BigBidAsk() : cppExtension() {}
    virtual int draw(void);
    virtual int mouse(RTX_EVENT* e);
    // (1.19.0, Rassul 21:51) the settings window: filled from / saved to lsFlexLevels\BigBidAsk.settings2-<MKT>.txt (SettingsHost.h)
    // (1.19.2) IRT faulted INSIDE these calls when the window copy had no chart ("???": adding / pasting the indicator - msglog
    // 2026-10-20 22:24 parmsLoad, 22:33 parmsApply): the window was never filled and OK saved nothing. Each call now runs behind
    // ss::sehRun (SettingsGuard.h); a caught fault is logged in BigBidAsk.dialog2-<MKT>.txt and the window keeps what it shows.
    virtual int parmsLoadBody(void) { guarded(&BigBidAsk::loadBody, "parmsLoad", 0); return RTX_OK; }
    virtual int parmsApplyBody(void) { guarded(&BigBidAsk::applyBody, "parmsApply", 0); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { guarded(&BigBidAsk::updtBody, "parmsUpdt", i); return RTX_OK; }
    struct GArg { BigBidAsk* me; void (BigBidAsk::*fn)(unsigned); unsigned i; };
    static void gThunk(void* p) { GArg* a = static_cast<GArg*>(p); (a->me->*(a->fn))(a->i); }
    static void gLog(void* p) { GArg* a = static_cast<GArg*>(p); a->me->rawLog("FAULT caught (IRT faulted inside the call; window kept as shown)"); }
    void guarded(void (BigBidAsk::*fn)(unsigned), const char* what, unsigned i) {
        GArg a{ this, fn, i };
        if (ss::sehRun(&BigBidAsk::gThunk, &a)) { (void)what; ss::sehRun(&BigBidAsk::gLog, &a); }
    }
    void loadBody(unsigned) {
        rawLog("parmsLoad");
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) { rawLog("parmsLoad: no market on this copy - the window keeps the defaults, OK saves nothing"); return; }
        const ss::Values F = st->load(ss::marketIndex(dlgMkt_), steadyMs(), true);
        ss::fillDialog(this, bbaset::spec(), bbaIds(), F, snap_);
        dlgLog("FILL", F, ss::dialogNow(this, bbaset::spec(), bbaIds(), F));   // (1.19.1) what was filled and what the window reads back
    }
    void applyBody(unsigned) { rawLog("parmsApply"); saveSettings(); }
    void updtBody(unsigned i) { rawLog(("parmsUpdt " + std::to_string(i)).c_str()); ss::onUpdate(this, bbaset::spec(), bbaIds(), (int)i, snap_); saveSettings(); }
    // (1.19.1) every settings-window call as IRT makes it: in the window or not, the bar size label, the ids and the raw control reads
    void rawLog(const char* what) {
        const std::string f = flexDir(); if (f.empty()) return;
        const std::string mkt = chartMarket(); const char* pl = getPeriodicityLabel();
        const std::string p = f + BBA_SEP "BigBidAsk.dialog2-" + (mkt.empty() ? std::string("XX") : mkt) + ".txt";
        std::vector<std::string> keep; { std::ifstream in(p.c_str()); std::string ln; while (std::getline(in, ln)) keep.push_back(ln); }
        if (keep.size() > 199) keep.erase(keep.begin(), keep.end() - 199);
        const ss::Ids& I = bbaIds(); std::string o = std::to_string(std::time(nullptr)) + ",CALL," + what + ",inDialog " + (ss::inDialog(this) ? "1" : "0") + ",period '" + (pl ? pl : "") + "',count " + std::to_string(getParameterCount()) + ",ids";
        for (int v : I.id) o += " " + std::to_string(v);
        const ss::Spec& SP = bbaset::spec();
        if ((int)I.id.size() == SP.n) { const ss::Values R = ss::dialogNow(this, SP, I, ss::defaults(SP, -1));
            for (int k = 0; k < SP.n; ++k) o += std::string(",") + SP.rows[k].key + "=" + ss::fmt(SP.rows[k], R.v[(size_t)k]); }
        keep.push_back(o);
        std::ofstream out(p.c_str(), std::ios::trunc); for (const auto& l : keep) out << l << "\n";
    }
    static ss::Ids& bbaIds() { static ss::Ids I; return I; }
    std::string chartMarket() { return ss::safeMarket(this); }   // (1.19.2) a copy with no chart = "" (never faults)
    static std::string setPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() || mkt.empty() ? std::string() : f + BBA_SEP "BigBidAsk.settings2-" + mkt + ".txt"; }
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = chartMarket(); const std::string p = setPath(mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &bbaset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values file = st->load(ss::marketIndex(dlgMkt_), steadyMs(), true);
        ss::Values V = ss::readDialog(this, bbaset::spec(), bbaIds(), file, snap_);
        // (1.19.1, Rassul 22:21 "i am trying to increase the font size and it doesn't work": his ES file was saved twice with font 7 while
        // the window showed 10) the NUMBER boxes are saved exactly as the window shows them when the value is valid - the "untouched
        // control keeps the file's value" rule lost his typed size when IRT re-filled / re-read the window around OK. An invalid number
        // (e.g. a blank or 0 Biggest in) keeps the file's value. Colours keep the layer's rule (a never-filled swatch must not save black).
        const ss::Values now = ss::dialogNow(this, bbaset::spec(), bbaIds(), file);
        const ss::Spec& SP = bbaset::spec();
        if (V.why.find("refused") == std::string::npos)
            for (int k = 0; k < SP.n; ++k) if (SP.rows[k].type == ss::T_INT && ss::valid(SP.rows[k], now.v[(size_t)k])) V.v[(size_t)k] = now.v[(size_t)k];
        makeDir(flexDir());
        const bool ok = st->save(V);
        dlgLog(ok ? "SAVED" : "NOT SAVED", V, now);
        if (ok) snap_.take(now);
    }
    // (1.19.1) lsFlexLevels\BigBidAsk.dialog2-<MKT>.txt: every fill / save with the window's read-back (the last 200 lines)
    void dlgLog(const char* what, const ss::Values& V, const ss::Values& shown) {
        const std::string f = flexDir(); if (f.empty() || dlgMkt_.empty()) return;
        const std::string p = f + BBA_SEP "BigBidAsk.dialog2-" + dlgMkt_ + ".txt";
        std::vector<std::string> keep; { std::ifstream in(p.c_str()); std::string ln; while (std::getline(in, ln)) keep.push_back(ln); }
        if (keep.size() > 199) keep.erase(keep.begin(), keep.end() - 199);
        const ss::Spec& SP = bbaset::spec(); std::string o = std::to_string(std::time(nullptr)) + "," + what + "," + BBA_VERSION;
        for (int k = 0; k < SP.n; ++k) { o += std::string(",") + SP.rows[k].key + "=" + ss::fmt(SP.rows[k], V.v[(size_t)k]) + "/shown " + ss::fmt(SP.rows[k], shown.v[(size_t)k]); }
        keep.push_back(o);
        std::ofstream out(p.c_str(), std::ios::trunc); for (const auto& l : keep) out << l << "\n";
    }
    void pump(bool fromDraw);
    void release() { BBState* S = slot_.get(this, false); if (S && !S->busy) { try { S->lastRecordMs = -1; flush(*S); } catch (...) {} } slot_.release(this); }   // what is decided is written before the state goes
    BBState* testState() { return slot_.get(this, false); }
private:
    HostSlot<BBState> slot_;
    long long ctSec(RTDATE d);
    int absMask(BBState& S, long long t, const bba::Facts& F);   // (1.20.0) (no longer drawn from 1.22.0)
    int sabMask(BBState& S, long long t, const bba::Facts& F);   // (1.22.0) the shared A on this candle: 1 = bullish (circle the bid print), 2 = bearish (the ask print)
    long long sessionOf(long long barTime) { const long long cs = ctSec((RTDATE)barTime); return cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1; }
    std::string clockOf(long long t);
    void identify(BBState& S);
    void settings(BBState& S, long long now);
    std::string market();
    void volPass(BBState& S);
    bool parts(long long t, long& days, int& endSec);
    const std::vector<bba::NamedLevel>& levelsFor(BBState& S, int i, bool closed);
    void gammaRead(BBState& S, long long now);
    void statsBuild(BBState& S, long long now);
    template <class H, class L, class C> PickInfo choose(BBState& S, int i, bool closed, int sd, const bba::Pick& p, int ph, const bba::TierSet& T, const PassCtx& X, H& hi, L& lo, C& cl);
    int trendAt(BBState& S, int i);
    template <class T> int backCount(BBState& S, T& times, int i, int sd, long long vol);   // (1.19.0) candles back it is the biggest
    void ensureBars(BBState& S);
    template <class H, class L> void passCtx(BBState& S, H& hi, L& lo, int n, PassCtx& X);
    void logPass(BBState& S, long long now);
    const bba::TierSet* tierFor(BBState& S, long sess);   // (1.15.0) the session's tiers once decidable (nullptr = still reading history)
    const bba::TierSet& tiersAt(BBState& S, long sess) { static const bba::TierSet learning; const bba::TierSet* T = tierFor(S, sess); return T ? *T : learning; }
    void tierStatus(BBState& S, long long now);
    void boardPass(BBState& S, long long now);           // (1.15.0) SignalBoard B from the record
    void boPass(BBState& S, long long now);              // (1.21.0) the breakout / breakdown decisions (once per closed candle)
    void rlyPass(BBState& S, long long now);             // (1.24.0) the Rly decisions (once per confirmed wave)
    void obPass(BBState& S, long long now);              // (1.25.0) the OB / BO boxes of strong waves (once per confirmed wave) + their tests
    std::string obPath(BBState& S, long long sess) { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "OrderBlock" BBA_SEP + stem(S) + "-" + std::to_string(sess) + ".csv"; }
    std::string rlyPath(BBState& S, long long sess) { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "Rly" BBA_SEP + stem(S) + "-" + std::to_string(sess) + ".csv"; }
    std::string boPath(BBState& S, long long sess) { const std::string d = dir(); return d.empty() ? d : d + BBA_SEP + stem(S) + "-breakouts-" + std::to_string(sess) + ".csv"; }
    std::string printsPath(BBState& S, long long sess) { const std::string d = dir(); return d.empty() ? d : d + BBA_SEP + stem(S) + "-prints-" + std::to_string(sess) + ".csv"; }
    std::string statsPath(const std::string& mkt) { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "BigBidAsk.stats-" + mkt + ".txt"; }
    int readStats(RTARRAYP* vap, int bar, BARSTATISTICS* stats);
    int readRows(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count);
    bool readBar(BBState& S, RTARRAYP& vap, int i, long long t, bool closed, float o, float h, float l, float c, bba::Facts& F);
    void compute(BBState& S, bool fromDraw);
    void load(BBState& S);
    bool flush(BBState& S);
    void status(BBState& S, long long now);
    void render(BBState& S);
    std::string dir() { const std::string f = flexDir(); return f.empty() ? f : f + BBA_SEP "BigBidAsk"; }
    std::string stem(BBState& S) { return S.market + "-" + fileKey(S.symbol) + "-" + std::to_string(S.spb); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<BigBidAsk*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BigBidAsk*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // (1.19.0, Rassul 21:51) the settings rows from the table (parameter version 8; 1.17 had none at version 7)
    ss::registerAll(this, bbaset::spec(), BigBidAsk::bbaIds());
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<BigBidAsk*>(this)->pump(false); return RTX_OK; }
int BigBidAsk::draw(void) {
    pump(true);
    BBState* S = slot_.get(this, false);
    if (S) { try { render(*S); } catch (...) { S->state = "draw error"; } }
    return RTX_OK;
}
long long BigBidAsk::ctSec(RTDATE d) {
    struct tm t; std::memset(&t, 0, sizeof(t));
    if (!getLocaltime(d, &t) || t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31) return 0;
    return civilDays(t.tm_year + 1900, (unsigned)(t.tm_mon + 1), (unsigned)t.tm_mday) * 86400LL + t.tm_hour * 3600LL + t.tm_min * 60LL + t.tm_sec;
}
// (1.20.0, MBO Roadmap Top Defender #1, Rassul approved mockup-absorption-v2-ES.png) is this closed candle's bid / ask print ABSORBED?
// The print sits in the candle's WICK (beyond its body, on its own side) at a price where the order book (lsBidAskStats, SignalBoard 'O')
// shows the defenders' resting orders traded >= 2x the most ever shown there and were reloaded >= 2 times (AbsorbLogic.h printAbsorbed).
// Decided ONCE, after lsBidAskStats decided the candle (its marker) or 90 s after first asked (then -1 = no order feed, never circled);
// kept in BigBidAsk.abs-<MKT>-<spb>.txt so a restart redraws exactly the same circles. The forming candle: 0.
int BigBidAsk::absMask(BBState& S, long long t, const bba::Facts& F) {
    if (!S.set.circle) return 0;
    if (!S.absLoaded) {
        S.absLoaded = true; const std::string f = flexDir();
        S.absPath = f.empty() || S.market.empty() ? std::string() : f + BBA_SEP "BigBidAsk.abs-" + S.market + "-" + std::to_string(S.spb) + ".txt";
        if (!S.absPath.empty()) { std::ifstream in(S.absPath.c_str()); std::string ln;
            while (std::getline(in, ln)) { long long tt = 0; int m = 0; if (std::sscanf(ln.c_str(), "%lld,%d", &tt, &m) == 2 && tt > 0 && m >= -1 && m <= 3) S.absC[tt] = m; } }
    }
    const auto it = S.absC.find(t); if (it != S.absC.end()) return it->second;
    if (!F.closed || !F.valid) return 0;
    const long long now = steadyMs();
    if (S.obReadMs < 0 || now - S.obReadMs > 1000) {   // the board once a second at most
        S.obReadMs = now; const char* sy = getSymbol(); const char* chl = getChartLabel();
        sb::readAll('O', sy ? sy : "", getSecondsPerBar(), chl ? chl : "", now, S.obEv);
    }
    const long long key = ctSec((RTDATE)t); if (key <= 0) return 0;
    bool present = false; std::vector<absl::ObEv> ev;
    for (const sb::Ev& e : S.obEv) {
        if (e.bar != key) continue;
        if (e.side == 0) { present = true; continue; }
        absl::ObEv o; o.px = e.px; o.side = e.side; o.traded = e.val; o.shown = sb::obShown(e.flag); o.refills = sb::obRefills(e.flag); ev.push_back(o);
    }
    auto w = S.absWait.find(t); if (w == S.absWait.end()) w = S.absWait.insert(std::make_pair(t, now)).first;
    if (!present && now - w->second < 90000) return 0;
    S.absWait.erase(w); if (S.absWait.size() > 400) S.absWait.clear();
    absl::Candle c; c.open = F.o; c.high = F.h; c.low = F.l; c.close = F.c;
    int m = 0;
    if (!present) { m = -1; ++S.absNoFeed; }
    else {
        if (F.bid.ok && absl::printAbsorbed(c, -1, F.bid.key, true, ev, S.tick)) m |= 1;
        if (F.ask.ok && absl::printAbsorbed(c, +1, F.ask.key, true, ev, S.tick)) m |= 2;
        if (m) ++S.absCircled;
    }
    ++S.absDecided; S.absC[t] = m;
    if (S.absC.size() > 6000) S.absC.erase(S.absC.begin());
    if (!S.absPath.empty()) { std::ofstream o(S.absPath.c_str(), std::ios::app); if (o.is_open()) o << t << ',' << m << '\n'; }
    return m;
}
// (1.22.0, 2026-10-22 Rassul 06:43 "then do it" / 06:49 "then require it", SHARED-ABSORPTION-STUDY: the circle "only when the shared A
// holds; the book test becomes the hover upgrade") a drawn print is circled only on a CLOSED candle that carries the SHARED A of its side in
// lsFlexLevels\SharedAbsorb\SAB-<MKT>-<spb>.csv (lsBidAskStats / lsDeltaProfile decide it) AND that closed in the defenders' colour on
// this chart: the bid print (sellers hitting) at a bullish A on a GREEN candle, the ask print at a bearish A on a RED candle. Read only.
int BigBidAsk::sabMask(BBState& S, long long t, const bba::Facts& F) {
    if (!F.closed || !F.valid || S.market.empty() || S.spb <= 0) return 0;
    const std::string f = flexDir(); if (f.empty()) return 0;
    S.sab.bind(S.market, (int)S.spb, f + BBA_SEP "SharedAbsorb" BBA_SEP + sab::fileName(S.market, (int)S.spb), f + BBA_SEP "LRA-Badges-" + sab::canon(S.market) + ".csv");
    S.sab.poll(steadyMs());
    int m = 0;
    if (S.sab.st.atRaw(sab::LOW, t)) { if (F.c > F.o) m |= 1; else ++S.sabVeto; }
    if (S.sab.st.atRaw(sab::HIGH, t)) { if (F.c < F.o) m |= 2; else ++S.sabVeto; }
    return m;
}
std::string BigBidAsk::clockOf(long long t) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return "?";
    return bba::clock12(m.tm_hour, m.tm_min);
}
std::string BigBidAsk::market() {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    return marketName(root.empty() ? sym : root);
}
void BigBidAsk::identify(BBState& S) {
    char rb[32]; std::memset(rb, 0, sizeof(rb));
    const char* rs = getRootSymbol(rb); const char* sy = getSymbol();
    const std::string root = rs ? rs : "", sym = sy ? sy : "";
    const int spb = getSecondsPerBar();
    const std::string id = root + "|" + sym + "|" + std::to_string(spb);
    if (id == S.identity) return;
    BBState fresh; fresh.identity = id; fresh.symbol = sym; fresh.spb = spb; fresh.market = marketName(root.empty() ? sym : root);
    fresh.msHas = S.msHas; fresh.msH = S.msH; fresh.msV = S.msV; fresh.msWnd = S.msWnd; fresh.busy = S.busy;
    { int o = 0, c = 0; if (svl::rthOf(fresh.market.c_str(), o, c)) { fresh.openMin = o; fresh.closeMin = c; } }   // (1.11.0)
    S = fresh;                                                       // nothing of another symbol / bar size is kept
}
static bool dayOf(const BBState& S, int m) { return m > S.openMin && m <= S.closeMin + 60; }   // (1.11.0) day = the RTH open to an hour after the RTH close (ES 8:30 AM - 4:00 PM)   // (1.10.0, Rassul 14:27) day = 8:30 AM - 4:00 PM CT (the 3 - 4 PM hour is no longer judged by night sizes)
// (1.15.0) the 1.5 - 1.14 printSd (a log-normal fit over the newest 2,300 candles every 5 s, and Big Lot N over a moving window that
// counted today's partial session and partly read sessions as whole ones) is gone: tiers are per session, decided once (tierFor).
// (1.15.0) a complete session: 3/4 of its day candles (the statistics' rule)
static int dayNeed(const BBState& S) { const int per = S.spb > 0 ? S.spb : 180; return std::max(20, (int)((S.closeMin + 60 - S.openMin) * 60 / per * 3 / 4)); }
// (1.15.0) the time filter of one candle (Skip first / last, RTH only) - the chart, the print log and the board use the same rule
static bool timeSkipped(const bba::Settings& set, const svl::Cls& cls, int endSec, int openMin, int closeMin) {
    if (set.rthOnly && cls.kind != svl::RTH) return true;
    if (set.skipFirst > 0 || set.skipLast > 0) {
        const int st = cls.startSec, en = endSec, o1 = 17 * 3600, o2 = openMin * 60, c1 = 16 * 3600, c2 = closeMin * 60, X = set.skipFirst * 60, Y = set.skipLast * 60;
        if ((X > 0 && ((st >= o1 && st < o1 + X) || (st >= o2 && st < o2 + X))) || (Y > 0 && ((en > c1 - Y && en <= c1) || (en > c2 - Y && en <= c2)))) return true;
    }
    return false;
}
void BigBidAsk::settings(BBState& S, long long now) {
    tierStatus(S, now);   // (1.15.0)
    // (1.17.0) fixed behaviour, no file: BigBidAsk.settings-<MKT>.txt is never read or written
    // (1.19.0) the fixed behaviour + his settings file (read at most every 2 s by the Store; a missing / bad file = the defaults)
    const std::string sp = setPath(ss::marketIndex(S.market) >= 0 ? ss::MARKETS[ss::marketIndex(S.market)] : std::string());
    if (S.setStore.path != sp) { S.setStore = ss::Store(); S.setStore.S = &bbaset::spec(); S.setStore.path = sp; }
    S.sv = sp.empty() ? ss::defaults(bbaset::spec(), -1) : S.setStore.load(ss::marketIndex(S.market), now);
    bba::Settings f = bba::fixedSettings(S.market);
    f.skipFirst = S.sv.i(bbaset::K_SKIP_FIRST); f.skipLast = S.sv.i(bbaset::K_SKIP_LAST);
    for (int k = 0; k < 3; ++k) { f.cat[k].font = S.sv.i(bbaset::K_FONT); f.cat[k].bid = f.cat[k].ask = (unsigned)S.sv.c(bbaset::K_COL_NUM); }
    f.boxBid = (unsigned)S.sv.c(bbaset::K_COL_BID); f.boxAsk = (unsigned)S.sv.c(bbaset::K_COL_ASK);
    f.circle = S.sv.b(bbaset::K_ABS_CIRCLE);   // (1.20.0) the absorbed-print circle (order book)
    S.boOn = S.sv.b(bbaset::K_BREAKOUT); S.boCol = (unsigned)S.sv.c(bbaset::K_COL_BO); S.bdCol = (unsigned)S.sv.c(bbaset::K_COL_BD);   // (1.21.0)
    S.rlyOn = S.sv.b(bbaset::K_RLY); S.rlyUpCol = (unsigned)S.sv.c(bbaset::K_COL_RLY_UP); S.rlyDnCol = (unsigned)S.sv.c(bbaset::K_COL_RLY_DN);   // (1.24.0)
    S.obOn = S.sv.b(bbaset::K_OB); S.obAny = S.sv.b(bbaset::K_OB_ANY);   // (1.25.0; 1.26.0 the BO boxes row is gone)
    {   const double a = S.sv.f(bbaset::K_OB_ATR); const double na = std::isfinite(a) ? std::max(2.0, std::min(10.0, a)) : 3.0;
        const int og = std::max(1, std::min(obk::ORIGIN_MAX, S.sv.i(bbaset::K_OB_ORIGIN)));   // (1.26.0) 1 - 3 candles after the pivot
        if (na != S.obAtr || og != S.obOrigin) { S.obCount = -1; }        // a new threshold / origin decides the NEXT waves (recorded boxes stay)
        S.obAtr = na; S.obOrigin = og; }
    S.obUpCol = (unsigned)S.sv.c(bbaset::K_COL_OB_UP); S.obDnCol = (unsigned)S.sv.c(bbaset::K_COL_OB_DN);
    S.set = bba::effective(f); S.topN = std::max(2, std::min(bbaset::CAP, S.sv.i(bbaset::K_BIGGEST_IN)));
    S.settingsWhy = S.sv.why.empty() ? "defaults" : S.sv.why; S.setStamp = 119; S.setMarket = S.market;
}
// ---- the chart's volume at price (only trivial values live in these MSVC SEH helpers) ---------------------------------------------
int BigBidAsk::readStats(RTARRAYP* vap, int bar, BARSTATISTICS* stats) {
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
int BigBidAsk::readRows(RTARRAYP* vap, int bar, VOLPROFILE* rows, int count) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!vap || count < 0 || (count > 0 && !rows)) return -2;
        for (int i = 0; i < count; ++i) {
            std::memset(&rows[i], 0, sizeof(rows[i]));
            if (vap->getVolumeProfile(bar, i, rows[i]) != RTX_OK) return -2;
        }
        return count;                                                  // any row failure rejects the whole bar, never a prefix
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
#endif
}
// one bar's facts from the chart; false = no usable VAP for it yet (F.valid false)
bool BigBidAsk::readBar(BBState& S, RTARRAYP& vap, int i, long long t, bool closed, float o, float h, float l, float c, bba::Facts& F) {
    long long ko = 0, kh = 0, kl = 0, kc = 0;
    F = bba::Facts(); F.t = t; F.closed = closed;
    if (!keyOf(o, S.tick, ko) || !keyOf(h, S.tick, kh) || !keyOf(l, S.tick, kl) || !keyOf(c, S.tick, kc)) return false;
    BARSTATISTICS st;
    const int n = readStats(&vap, i, &st);
    if (n == -1) { S.faulted = true; ++S.faults; return false; }
    if (n <= 0) return false;
    std::vector<VOLPROFILE> rows((size_t)n);
    const int got = readRows(&vap, i, &rows[0], n);
    if (got == -1) { S.faulted = true; ++S.faults; return false; }
    if (got != n) return false;
    std::vector<bba::Row> R; R.reserve(rows.size());
    long long bids = 0, asks = 0;
    for (const VOLPROFILE& v : rows) {
        long long k = 0;
        if (!keyOf((double)v.price, S.tick, k) || v.buyVolume < 0 || v.sellVolume < 0) { ++S.mismatch; continue; }
        bba::Row r; r.key = k; r.bid = (long long)v.sellVolume; r.ask = (long long)v.buyVolume;
        bids += r.bid; asks += r.ask; R.push_back(r);
    }
    if (bids != (long long)st.sellVolume || asks != (long long)st.buyVolume) ++S.mismatch;   // counted, never blanks the bar
    F = bba::facts(t, closed, ko, kh, kl, kc, bba::mergeRows(R), true);
    return true;
}
// the facts of the candles in view (and the 20 + LEAD before it): closed candles final once, the forming one live
void BigBidAsk::compute(BBState& S, bool fromDraw) {
    const long count = getBarCount();
    if (count < 2) { S.state = "waiting for bars"; return; }
    if (S.faulted) { S.state = "VAP fault: remove and re-add the indicator to retry"; return; }
    RTARRAYI times(barDateTime); RTARRAY op(barOpen), hi(barHigh), lo(barLow), cl(barClose);
    RTARRAYP vap(barVolumeProfile);
    if (times.count < count || op.count < count || hi.count < count || lo.count < count || cl.count < count || vap.count < count) { S.state = "chart data arrays unavailable"; return; }
    const long long first = (long long)times[0], last = (long long)times[(int)count - 1];
    const long long now = steadyMs();
    const bool changed = count != S.lastCount || first != S.lastFirst || last != S.lastLast;
    if (!changed && S.builtMs >= 0 && now >= S.builtMs && now - S.builtMs < 1000) return;   // at most once a second unless a bar came
    S.builtMs = now; S.lastCount = count; S.lastFirst = first; S.lastLast = last;
    int vs = 0, ve = -1;
    if (!fromDraw || getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0L, count - 400); ve = (int)count - 1; }
    // (1.15.0, audit B finding 2) the facts of the last 8 sessions are read whatever is in view (the tiers must never depend on how far he
    // scrolled): the read starts at the first closed candle of that span that is not final yet (900 reads a pass), or at the view if earlier
    const int histBars = (int)std::min<long long>(count, 8LL * 86400 / std::max(60, S.spb > 0 ? S.spb : 180) + 200);
    int firstOpen = (int)count - 1;
    // (1.19.1, Rassul 22:23 "when i copy and paste the indicator to other markets like nq it doesn't work") the LAST 400 candles first:
    // a new chart drew nothing until all 8 sessions were read (NQ: minutes of missing volume-at-price retries). The prints need only the
    // 50 candles before them; the older sessions (the Big Lot box) are read after the recent ones are final.
    for (int i = std::max(0, (int)count - 1 - 400); i < (int)count - 1; ++i) if (!S.fin.count((long long)times[i])) { firstOpen = i; break; }
    if (firstOpen == (int)count - 1)
        for (int i = std::max(0, (int)count - histBars); i < (int)count - 1; ++i) if (!S.fin.count((long long)times[i])) { firstOpen = i; break; }
    const int from = std::min(std::max(0, vs - LEAD), firstOpen), lo0 = std::max(0, from - 140);   // (1.3.0) priors of the same time of day reach further back
    std::vector<bba::Facts> seq; seq.reserve((size_t)(count - lo0));
    int reads = 0; bool capped = false;
    for (int i = lo0; i < (int)count; ++i) {
        const long long t = (long long)times[i];
        const bool closed = i < (int)count - 1;
        bba::Facts F;
        auto fi = S.fin.find(t);
        if (closed && fi != S.fin.end()) { seq.push_back(fi->second); continue; }
        auto ri = S.raw.find(t);
        if (closed && ri != S.raw.end()) { seq.push_back(ri->second); continue; }
        if (reads >= READS_MAX) { capped = true; F.t = t; F.closed = closed; seq.push_back(F); continue; }
        ++reads;
        const bool ok = readBar(S, vap, i, t, closed, op[i], hi[i], lo[i], cl[i], F);
        if (S.faulted) { S.state = "VAP fault: remove and re-add the indicator to retry"; return; }
        if (closed) {
            if (ok) { S.raw[t] = F; S.fails.erase(t); }
            else {
                ++S.missing;
                BBState::Fail& fl = S.fails[t]; if (fl.n++ == 0) fl.firstMs = now;
                if (fl.n >= FAILS_FINAL && now - fl.firstMs >= 60000) { F.valid = false; S.raw[t] = F; S.fails.erase(t); }   // no data from IRT for a minute: "no numbers" (not recorded)
            }
        }
        seq.push_back(F);
    }
    for (size_t k = 0; k < seq.size(); ++k) {                                       // (1.3.0) the time-of-day segment of each candle
        const long long cs = ctSec((RTDATE)seq[k].t); const int m = cs > 0 ? (int)((cs % 86400) / 60) : -1;
        seq[k].seg = m < 0 ? -1 : (m >= 510 && m < 900) ? 2 : (m >= 420 && m < 510) ? 1 : 0; }
    // ratios (past only) and the final facts of every closed candle whose 20 priors are all known
    for (size_t k = 0; k < seq.size(); ++k) {
        const int i = lo0 + (int)k;
        if (i < from) continue;
        const long long t = seq[k].t;
        if (seq[k].closed && S.fin.count(t)) continue;
        bool priorsKnown = true;
        for (size_t j = k > (size_t)bba::LOOKBACK ? k - bba::LOOKBACK : 0; j < k; ++j) if (!S.fin.count(seq[j].t) && !S.raw.count(seq[j].t)) priorsKnown = false;
        if (k < (size_t)bba::LOOKBACK && lo0 > 0) priorsKnown = false;          // the priors start before what was read
        bba::rate(seq, k);
        if (!seq[k].closed) { S.forming = seq[k]; S.haveForming = true; continue; }
        if (!S.raw.count(t) || !priorsKnown) continue;                           // not final yet: retried next pass
        S.fin[t] = seq[k]; S.raw.erase(t); ++S.decided;
        const long long sess = sessionOf(t);
        if (seq[k].valid && sess >= S.session - 7 && sess <= S.session && sess > 0 && !S.recorded.count(t) && S.pending.size() < REC_LINES_MAX) S.pending.push_back(std::make_pair(sess, seq[k]));   // (1.15.0) 8 sessions (the tiers' source survives a restart)
    }
    if (S.raw.size() > 50000) S.raw.clear();                                        // bounded (re-read if needed)
    {   // (1.15.0) how far back every closed chart bar is final (contiguous from the last closed bar): the tiers of a session are decided
        // only once the 7 days before it are covered (or the chart's first bar is reached)
        int c0 = (int)count - 1;
        while (c0 > 0 && S.fin.count((long long)times[c0 - 1])) --c0;
        S.covStart = c0 == 0;
        long d = 0; int e = 0; S.covSess = c0 < (int)count - 1 && parts((long long)times[c0], d, e) ? svl::classify(d, e, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin).sess : LONG_MAX;
        if (c0 >= (int)count - 1) S.covSess = LONG_MAX;                             // nothing final yet
    }
    S.state = capped ? "reading history (continues next pass)" : "ready";
}
void BigBidAsk::load(BBState& S) {
    const std::string d = dir(); if (d.empty()) return;
    for (long long k = S.session - 2; k <= S.session; ++k) {    // (1.12.0) the prints already logged (never logged twice)
        const std::string path = printsPath(S, k); std::string text; long long st = -1;
        if (!statFile(path, st) || !readSmall(path, REC_FILE_MAX, text)) continue;
        std::istringstream in(text); std::string line; bool head = true;
        while (std::getline(in, line)) {
            if (head) { head = false; continue; }
            const size_t a = line.find('|'), b = a == std::string::npos ? a : line.find('|', a + 1);
            if (b == std::string::npos) continue;
            S.logged.insert(line.substr(0, a) + (line.substr(a + 1, b - a - 1) == "-1" ? "b" : "a"));
        }
    }
    for (long long k = S.session - 2; k <= S.session; ++k) {    // (1.21.0) the breakout decisions already made: never decided again (no repaint)
        const std::string path = boPath(S, k); std::string text; long long st = -1;
        if (!statFile(path, st) || !readSmall(path, REC_FILE_MAX, text)) continue;
        std::istringstream in(text); std::string line;
        while (std::getline(in, line)) { bba::BoDec D;
            const size_t vb = line.find_last_of('|'); std::string lv = vb == std::string::npos ? std::string() : line.substr(vb + 1); while (!lv.empty() && (lv.back() == '\r' || lv.back() == '\n')) lv.pop_back();
            const bool oldRule = lv == "1.21.0" || lv == "1.22.0" || lv == "1.23.0" || lv == "1.24.0" || lv == "1.25.0";   // (1.26.0) the old pivot rule: decided again
            if (!oldRule && bba::parseBoLine(line, D)) { if (!S.boDec.count(D.t0)) S.boDec[D.t0] = D; }       // (1.23.0) first line per breakout candle wins
            else if (line.compare(0, 4, "BBO|") == 0 && line.compare(0, 6, "BBO|t|") != 0) ++S.boLegacy; }   // a 1.21 / 1.22 line: not redrawn
    }
    for (long long k = S.session - 3; k <= S.session; ++k) {    // (1.24.0) the Rly waves already judged: never judged again (no repaint)
        const std::string path = rlyPath(S, k); std::string text; long long st = -1;
        if (!statFile(path, st) || !readSmall(path, REC_FILE_MAX, text)) continue;
        std::istringstream in(text); std::string line;
        while (std::getline(in, line)) { rly::Rec R;
            if (rly::parseLine(line, R)) { const long long key = R.te * 2 + (R.dir > 0 ? 1 : 0); if (!S.rlyRec.count(key)) { S.rlyRec[key] = R; ++S.rlyLoaded; } }
            else if (line.compare(0, 4, "RLY|") == 0 && line.compare(0, 6, "RLY|t_") != 0) ++S.rlyBad; }
    }
    for (long long k = S.session - 3; k <= S.session; ++k) {    // (1.25.0) the OB / BO boxes already decided + the tests already logged (never again)
        const std::string path = obPath(S, k); std::string text; long long st = -1;
        if (!statFile(path, st) || !readSmall(path, REC_FILE_MAX, text)) continue;
        std::istringstream in(text); std::string line;
        while (std::getline(in, line)) { obk::Box b; bool legacy = false;
            if (obk::parseLine(line, b, &legacy)) { if (!S.obRec.count(b.id())) { S.obRec[b.id()] = b; ++S.obLoaded; } }
            else if (legacy) ++S.obLegacy;                                        // (1.26.0) a 1.25.0 box line: not drawn (the boxes are gone)
            else if (line.compare(0, 4, "OBT|") == 0 && line.compare(0, 7, "OBT|id|") != 0) { const std::vector<std::string> f = obk::split(line, '|'); if (f.size() >= 5) S.obTestDone.insert(f[1] + "|" + f[4]); }
            else if (line.compare(0, 4, "OBX|") == 0 && line.compare(0, 9, "OBX|kind|") != 0) ++S.obBad; }
    }
    for (long long k = S.session - 10; k <= S.session; ++k) {   // (1.11.0) back 10 days: 5 complete sessions for the statistics
        const std::string path = d + BBA_SEP + stem(S) + "-" + std::to_string(k) + ".csv";
        long long st = -1; if (!statFile(path, st)) continue;
        std::string text; if (!readSmall(path, REC_FILE_MAX, text)) { ++S.recFails; continue; }
        std::istringstream in(text); std::string line; bool head = true; size_t n = 0;
        while (std::getline(in, line) && ++n <= REC_LINES_MAX) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (head) { head = false; if (line != bba::REC_HEADER) break; continue; }
            bba::Facts F; if (!bba::parseRec(line, F)) continue;
            if (!S.fin.count(F.t)) S.fin[F.t] = F;                     // first wins: a candle's line never changes
            S.recorded.insert(F.t); S.raw.erase(F.t);
        }
    }
}
bool BigBidAsk::flush(BBState& S) {
    if (S.pending.empty()) return true;
    const long long now = steadyMs();
    if (S.lastRecordMs >= 0 && now >= S.lastRecordMs && now - S.lastRecordMs < 1000) return false;   // a failed publish is retried after 1 s
    const std::string d = dir(); if (d.empty()) { ++S.recFails; S.lastRecordMs = now; return false; }
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<size_t>> groups;
    for (size_t i = 0; i < S.pending.size(); ++i) groups[S.pending[i].first].push_back(i);
    std::vector<char> done(S.pending.size(), 0);
    for (const auto& g : groups) {
        const std::string path = d + BBA_SEP + stem(S) + "-" + std::to_string(g.first) + ".csv";
        std::string old; long long st = -1; std::vector<std::string> keep; std::set<long long> seen;
        if (statFile(path, st)) {
            if (!readSmall(path, REC_FILE_MAX, old)) { ++S.recFails; continue; }      // never rewrite a file we could not read
            std::istringstream in(old); std::string line; bool head = true, bad = false;
            while (std::getline(in, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (head) { head = false; if (line != bba::REC_HEADER) { bad = true; break; } continue; }
                bba::Facts F; if (bba::parseRec(line, F) && seen.insert(F.t).second) keep.push_back(bba::recLine(F));
            }
            if (bad) { ++S.recFails; continue; }
        }
        std::string text = std::string(bba::REC_HEADER) + "\n";
        for (const auto& k : keep) text += k + "\n";
        for (size_t j : g.second) { const bba::Facts& F = S.pending[j].second; if (seen.insert(F.t).second) text += bba::recLine(F) + "\n"; }
        if (text.size() > REC_FILE_MAX || !publishFile(path, text)) { ++S.recFails; continue; }
        for (size_t j : g.second) { S.recorded.insert(S.pending[j].second.t); done[j] = 1; }
    }
    std::vector<std::pair<long long, bba::Facts>> retry;
    for (size_t i = 0; i < S.pending.size(); ++i) if (!done[i]) retry.push_back(S.pending[i]);
    S.pending.swap(retry);
    S.lastRecordMs = S.pending.empty() ? -1 : now;
    return S.pending.empty();
}
void BigBidAsk::status(BBState& S, long long now) {
    if (S.lastStatusMs >= 0 && now >= S.lastStatusMs && now - S.lastStatusMs < 2000) return;
    std::ostringstream o; o.imbue(std::locale::classic());
    o << "VERSION," << BBA_VERSION << "\nMARKET," << S.market << "\nSYMBOL," << S.symbol << "\nSPB," << S.spb << "\nSTATE," << S.state
      << "\nTICK," << S.tick
      << "\nSETTINGS," << S.settingsWhy << ",biggest in " << S.topN << ",skip first " << S.set.skipFirst << " / last " << S.set.skipLast << " min,font " << S.set.cat[0].font
      << ",box " << (S.set.box ? 1 : 0) << ",circle (shared A) " << (S.set.circle ? 1 : 0) << "\n" << S.sab.statusLine() << ",circles on the bid / ask print of a shared A candle (vetoed by the chart candle colour " << S.sabVeto << ")" << "\nABSORBED,retired 1.22.0 (the order-book circle; the shared A decides),candles decided " << S.absDecided << ",with an absorbed print " << S.absCircled << ",no order feed " << S.absNoFeed << ",circles on screen " << S.circles << " (wick print where the defenders traded >= 2x shown + >= 2 refills; needs Aggression / lsBidAskStats on this chart)\nSETTINGS_CONT" << ",dim " << (S.set.dim ? 1 : 0) << ",divider " << bba::divFor(S.market) << " (market default)"
      << ",skip first / last " << S.set.skipFirst << " / " << S.set.skipLast << " min,levels off,Big Lot box " << (S.set.statsChart ? "on" : "off")
      << "\nLEVELS,gamma " << S.gamWhy << ",volume profiles " << S.vp.size() << " sessions (from bar " << S.volNext << "),candles with levels " << S.lvCache.size()
      << "\nCANDLES,final " << S.fin.size() << ",recorded " << S.recorded.size() << ",pending " << S.pending.size() << ",record fails " << S.recFails
      << "\nVAP,missing reads " << S.missing << ",bid/ask vs bar totals mismatch " << S.mismatch << ",faults " << S.faults
      << "\nSIZES," << S.sdWhy << "\nDRAWN,numbers " << S.lay.shown << ",nudged " << S.lay.nudged << ",bold without room " << S.lay.crowded << ",not drawn (no room) " << S.lay.dropped << ",of them in view " << S.droppedInView << " (the rest are the lead-in left of the view)" << ",under the HOD/LOD box " << S.clHl << ",slid off the HOD/LOD box " << S.slidHl << ",circles " << S.circles << ",circles without room " << S.circlesHidden << ",bold at a zone " << S.zoneFull << ",bold mid-range (dimmed) " << S.zoneDim << ",category 1 / 2 / 3 / 4 prints " << S.catN4[0] << " / " << S.catN4[1] << " / " << S.catN4[2] << " / " << S.catN4[3] << ",big but kept out by the pivot filter " << S.pivotHidden << ",by the level filter " << S.levelHidden << ",by Trend PB / BO " << S.trendHidden << ",shown before their pivot is confirmed (may still go) " << S.unconfirmed << ",by the time filter " << S.timeHidden
      << "\nCLASH,left " << S.clL << ",right " << S.clR << ",top " << S.clT << ",bottom " << S.clB << ",banner " << S.clTr << ",too big " << S.clBig << ",slid back in " << S.shifted << ",area fallback " << S.areaFallback
      << "\nAREA,left " << S.leftLimit << ",price right " << S.priceRight << ",columns levels " << (S.peers.on[cb::LEVELS] ? "on" : "off") << " delta " << (S.peers.on[cb::DELTA] ? "on" : "off")
      << " dealer " << (S.peers.on[cb::DEALER] ? "on" : "off") << " liq " << (S.peers.on[cb::LIQ] ? "on" : "off") << ",tape reader banner " << (S.trOn ? "avoided" : "none")
      << "\nBL_BOX," << S.blWhy << ",RTH avg " << bba::boxValue(S.blDay, 0) << " / " << bba::boxValue(S.blDay, 1) << " / " << bba::boxValue(S.blDay, 2)
      << ",overnight avg " << bba::boxValue(S.blNight, 0) << " / " << bba::boxValue(S.blNight, 1) << " / " << bba::boxValue(S.blNight, 2) << "\n";
    {   // (1.21.0) BREAKOUTS: today's counts (the session of the last bar) + the details of today's breakouts (the hovers are off)
        const long count = getBarCount(); long long today = -1;
        if (count > 0) { RTARRAYI times(barDateTime); if (times.count >= count) today = sessionOf((long long)times[(int)count - 1]); }
        int bo = 0, boP = 0, bd = 0, bdP = 0, cand = 0, tf = 0, nx = 0, nr = 0, nv = 0, wk = 0, fb = 0, sw = 0;
        std::vector<const bba::BoDec*> ev;
        for (const auto& kv : S.boDec) { const bba::BoDec& D = kv.second; if (D.dir == 0 || sessionOf(D.t) != today) continue;
            ++cand;
            if (D.fired) { if (D.dir > 0) { ++bo; if (D.print) ++boP; } else { ++bd; if (D.print) ++bdP; } if (D.wick) ++wk; ev.push_back(&D); }
            else if (D.reason == bba::BO_R_WAVE_ENDED) { ++fb; ev.push_back(&D); }
            else if (D.reason == bba::BO_R_SAME_WAVE) ++sw;
            else if (D.reason == bba::BO_R_TIME) ++tf; else if (D.reason == bba::BO_R_NOT_EXTREME) ++nx; else if (D.reason == bba::BO_R_RANGE) ++nr; else ++nv; }
        std::sort(ev.begin(), ev.end(), [](const bba::BoDec* x, const bba::BoDec* y) { return x->t != y->t ? x->t < y->t : x->t0 < y->t0; });
        o << "BREAKOUTS," << (S.boOn ? "on" : "off (unchecked in Settings)") << ",today BO " << bo << " (with a print " << boP << ") / BD " << bd << " (with a print " << bdP << ")"
          << ",candidates " << cand << ",not fired: time filter " << tf << " / not a 10-candle extreme " << nx << " / range below prior " << nr << " / no volume or range up " << nv
          << ",on screen letters " << S.boLetters << " / own prints " << S.boPrints << " / on a Big Bid/Ask print " << S.boShared << " / kept out of the HOD/LOD box " << S.boKept << " / no room " << S.boNoRoom
          << ",decided " << S.boDec.size() << ",pending " << S.boPending.size() << ",record fails " << S.boRecFails << "\n";
        {   // (1.24.0) RLY: today's marks (the session of the last bar) with the wave numbers
            int g = 0, r = 0, judged = 0; std::string ev;
            for (const auto& kv : S.rlyRec) { const rly::Rec& R = kv.second; long d = 0; int e = 0; if (!parts(R.te, d, e) || (long long)d != today) continue;
                ++judged; if (!R.fired) continue; if (R.dir > 0) ++g; else ++r;
                const int st = e - (S.spb > 0 ? S.spb : 180);
                ev += std::string(" | ") + bba::clock12(st / 3600, st / 60 % 60) + (R.dir > 0 ? " green " : " red ") + std::to_string(R.nb) + " bars " + rly::fmtD(R.ratio, 1) + " ATR " + rly::trendName(R.trend); }
            o << "RLY," << (S.spb != 180 ? "3-min charts only (this chart " + std::to_string(S.spb) + " s)" : (S.rlyOn ? std::string("on") : std::string("off (unchecked in Settings)")))
              << ",logic " << rly::VERSION << ",today Rly " << (g + r) << " (green " << g << " / red " << r << "),waves judged today " << judged
              << ",on screen " << S.rlyLetters << ",no room " << S.rlyNoRoom << ",record loaded " << S.rlyLoaded << " / bad lines " << S.rlyBad
              << ",pending " << S.rlyPending.size() << ",record fails " << S.rlyRecFails << ev << "\n";
        }
        {   // (1.26.0) ORDER_BLOCK: today's OB prints (the session of the last bar) and the visits to their price
            int nOB = 0, nA = 0, nB = 0, none = 0, second = 0, held = 0, swept = 0, broke = 0; std::string ev;
            for (const auto& kv : S.obRec) { const obk::Box& b = kv.second; long d = 0; int e = 0; if (!parts(b.te, d, e) || (long long)d != today) continue;
                ++nOB; (b.cls == obk::CLS_RLY ? nA : nB)++; if (!b.has()) ++none; if (b.rank == 2) ++second;
                const auto tk = S.obTrk.find(kv.first); std::string ts;
                if (tk != S.obTrk.end()) for (const obk::Test& t : tk->second.tests) { if (t.outcome == obk::T_HELD) ++held; else if (t.outcome == obk::T_SWEPT) ++swept; else if (t.outcome == obk::T_BROKE) ++broke; ts += std::string(ts.empty() ? "" : "/") + obk::outcomeName(t.outcome); }
                std::string pr = "no print";
                if (b.has()) { const obk::Print& q = b.prints.front(); long d2 = 0; int e2 = 0; std::string c2 = "?"; if (parts(q.t, d2, e2)) { const int s2 = e2 - (S.spb > 0 ? S.spb : 180); c2 = bba::clock12(s2 / 3600, s2 / 60 % 60); }
                    pr = std::to_string(q.lots) + (q.side > 0 ? " at the ask " : " at the bid ") + bba::price((long long)std::llround(q.px / S.tick), S.tick) + " " + c2 + (b.rank == 2 ? " (2nd)" : ""); }
                const int st = e - (S.spb > 0 ? S.spb : 180);
                ev += std::string(" | OB ") + (b.dir > 0 ? "up-wave to " : "down-wave to ") + bba::clock12(st / 3600, st / 60 % 60) + (b.cls == obk::CLS_RLY ? " Rly " : " 3ATR ") + pr + " tests " + (ts.empty() ? std::string("-") : ts); }
            o << "ORDER_BLOCK," << (S.spb != 180 ? "3-min charts only (this chart " + std::to_string(S.spb) + " s)" : std::string(S.obOn ? "OB on" : "OB off (unchecked in Settings)") + " (nightly param SHOW " + mp::modeName(S.obShowP) + " - the 1.25 boxes' verdict, not applied to the letters)" + (S.obAny ? " / 3+ ATR waves on" : " / 3+ ATR waves off"))
              << ",logic " << obk::VERSION << " (prints + letters only; no boxes),x ATR " << rly::fmtD(S.obAtr, 1) << ",candles after the pivot " << S.obOrigin << ",today OB " << nOB << " (Rly waves " << nA << " / 3+ ATR " << nB << "),2nd print (top = BO print) " << second << ",no print " << none
              << ",visits held " << held << " / swept " << swept << " / broke " << broke << ",on screen letters " << S.obLabels << " (no room " << S.obNoLabel << "), own prints " << S.obPrints << ", on a drawn print " << S.obShared
              << ",record loaded " << S.obLoaded << " / bad lines " << S.obBad << " / 1.25.0 box lines ignored " << S.obLegacy << ",waiting " << S.obWait.size() << ",no volume at price " << S.obNoVap << ",pending " << S.obPending.size() << ",record fails " << S.obRecFails << ev << "\n";
        }
        o << "BREAKOUT_RULES,1.26.0 the pivot = one of the last 3 intact swings (1-bar ZigZag); letter BO green above a breakout / red below a breakdown; 1.23.0 breakout only on a green candle / breakdown only on a red candle; a wick break is confirmed by a close beyond the pivot in the same wave (mark on the confirming candle); failed wick breaks logged only (no mark)"
          << ",today confirmed later in the same wave " << wk << ",failed wick breaks (wave ended first - logged only) " << fb << ",same wave already broken / waiting " << sw
          << ",waiting for a close: BO " << (S.boWaitOn[1] ? std::string("since ") + clockOf(S.boWaitT0[1]) + " pivot " + bba::price((long long)std::llround(S.boWaitPiv[1] / S.tick), S.tick) : std::string("none"))
          << " / BD " << (S.boWaitOn[0] ? std::string("since ") + clockOf(S.boWaitT0[0]) + " pivot " + bba::price((long long)std::llround(S.boWaitPiv[0] / S.tick), S.tick) : std::string("none"))
          << ",old-rule record lines ignored " << S.boLegacy << ",duplicates kept out " << S.boDup << "\n";
        const size_t a = ev.size() > 30 ? ev.size() - 30 : 0;
        for (size_t k = a; k < ev.size(); ++k) o << "BO_EVENT," << bba::boDetail(*ev[k], S.tick, clockOf(ev[k]->t)) << "\n";
    }
    const std::string text = o.str();
    if (text == S.lastStatusText && S.lastStatusMs >= 0 && now - S.lastStatusMs < 30000) return;
    S.lastStatusMs = now;
    const std::string f = flexDir(); if (f.empty()) return;
    if (publishFile(f + BBA_SEP "BigBidAsk.status-" + stem(S) + ".txt", text)) S.lastStatusText = text;
}
void BigBidAsk::pump(bool fromDraw) {
    BBState* P = slot_.get(this, true);
    if (!P || P->busy) return;
    BBState& S = *P; S.busy = true;
    struct Unbusy { BBState* s; ~Unbusy() { s->busy = false; } } unbusy = { P };
    try {
        identify(S);
        const double tick = cleanTick(getProperty(SYM_TICKINCR));
        if (tick <= 0) { S.state = "no tick size (SYM_TICKINCR)"; return; }
        if (tick != S.tick) { S.fin.clear(); S.raw.clear(); S.fails.clear(); S.recorded.clear(); S.pending.clear(); S.haveForming = false; S.session = -1; S.lastCount = -1; S.tick = tick;
                              S.tiers.clear(); S.bySess.clear(); S.bySessN = (size_t)-1; S.covSess = LONG_MAX; S.covStart = false; S.boDec.clear(); S.boPending.clear();
                              S.obRec.clear(); S.obTrk.clear(); S.obEndT.clear(); S.obWait.clear(); S.obCount = -1; }
        const long long now = steadyMs();
        const long long ct = ctSec(currentDate());
        if (ct > 0) { const long long key = (ct - 17LL * 3600) / 86400LL + 1; if (key != S.session) { S.session = key; load(S); } }
        settings(S, now);
        compute(S, fromDraw);
        volPass(S);                                                          // (1.11.0)
        gammaRead(S, now);
        statsBuild(S, now);
        logPass(S, now);                                                     // (1.12.0)
        boardPass(S, now);                                                   // (1.15.0) SignalBoard B from the record
        boPass(S, now);                                                      // (1.21.0) breakout / breakdown, once per closed candle
        rlyPass(S, now);                                                     // (1.24.0) Rly, once per confirmed wave
        obPass(S, now);                                                      // (1.25.0) OB / BO boxes, once per confirmed strong wave
        flush(S);
        status(S, now);
    } catch (...) { S.state = "calculation error"; }
}
template <class Times> static int indexOf(Times& times, long count, long long t) {
    long lo = 0, hi = count - 1;
    while (lo <= hi) {
        const long mid = lo + (hi - lo) / 2; const long long v = (long long)times[(int)mid];
        if (v == t) return (int)mid;
        if (v < t) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}
// ---- (1.11.0) the session clock of a bar stamp (IRT stamps = the bar END, local CT) ---------------------------------------------
bool BigBidAsk::parts(long long t, long& days, int& endSec) {
    struct tm m; std::memset(&m, 0, sizeof(m));
    if (t <= 0 || !getLocaltime((RTDATE)t, &m)) return false;
    days = svl::daysFromCivil(m.tm_year + 1900, m.tm_mon + 1, m.tm_mday); endSec = m.tm_hour * 3600 + m.tm_min * 60 + m.tm_sec; return true;
}
// ---- (1.11.0) the volume profiles per session (RTH and full) from the chart's volume at price: the last 15 days, 600 bars a pass ---
void BigBidAsk::volPass(BBState& S) {
    const long count = getBarCount(); if (count < 3 || S.tick <= 0 || S.faulted) return;
    RTARRAYI times(barDateTime); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || vap.count < count) return;
    const long long t0 = (long long)times[0];
    if (S.volNext < 0 || t0 != S.volFirstT) {
        S.vp.clear(); S.volFirstT = t0;
        const long per = std::max(60, S.spb > 0 ? S.spb : 180);
        S.volNext = std::max(0L, count - 1 - (long)(15LL * 86400 / per));
        long d = 0; int e = 0; S.volFirstSess = parts((long long)times[(int)S.volNext], d, e) ? svl::classify(d, e, (int)per, S.openMin, S.closeMin).sess : 0;
    }
    int reads = 0;
    while (S.volNext < count - 1 && reads < 600) {
        const int i = (int)S.volNext; ++S.volNext; ++reads;
        long days = 0; int endSec = 0; if (!parts((long long)times[i], days, endSec)) continue;
        const svl::Cls c = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
        if (c.kind == svl::NONE) continue;
        BARSTATISTICS st; const int n = readStats(&vap, i, &st);
        if (n == -1) { S.faulted = true; ++S.faults; return; }
        if (n <= 0 || n > 4000) continue;
        std::vector<VOLPROFILE> rows((size_t)n);
        if (readRows(&vap, i, &rows[0], n) != n) continue;
        BBState::VP& P = S.vp[c.sess]; ++P.bars;
        for (const VOLPROFILE& v : rows) { long long k = 0; if (!keyOf((double)v.price, S.tick, k)) continue;
            const long long tv = (long long)v.buyVolume + (long long)v.sellVolume; if (tv <= 0) continue;
            P.full[k] += tv; if (c.kind == svl::RTH) P.rth[k] += tv; }
    }
}
// ---- (1.11.0) the gamma levels: MenthorQLevels.csv, the chart's market and contract month; used for today's session only --------
void BigBidAsk::gammaRead(BBState& S, long long now) {
    if (S.gamCheckMs >= 0 && now >= S.gamCheckMs && now - S.gamCheckMs < 5000) return;
    S.gamCheckMs = now; S.gam.clear();
    const std::string f = flexDir(); std::string text;
    if (f.empty() || !readSmall(f + BBA_SEP "MenthorQLevels.csv", 64 * 1024, text)) { S.gamWhy = "MenthorQLevels.csv missing"; return; }
    S.gam = bba::parseGamma(text, S.market, S.symbol, S.tick);
    S.gamWhy = std::to_string(S.gam.size()) + " MenthorQ levels";
    const long count = getBarCount();
    if (count > 0) { RTARRAYI times(barDateTime); long d = 0; int e = 0; if (times.count >= count && parts((long long)times[(int)count - 1], d, e)) S.gamSess = svl::classify(d, e, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin).sess; }
}
// ---- (1.11.0) the levels known at candle i: key levels from the bars up to i (lsDealerProfile's code), the session high / low so far,
// the gamma levels (today's session only), the volume levels (prior day RTH, prior week). A closed candle's levels are kept (no repaint).
const std::vector<bba::NamedLevel>& BigBidAsk::levelsFor(BBState& S, int i, bool closed) {
    static std::vector<bba::NamedLevel> live;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    const long long t = (long long)times[i];
    if (closed) { auto it = S.lvCache.find(t); if (it != S.lvCache.end()) return it->second; }
    std::vector<bba::NamedLevel> out;
    const int per = S.spb > 0 ? S.spb : 180;
    long days = 0; int endSec = 0; parts(t, days, endSec);
    const svl::Cls me = svl::classify(days, endSec, per, S.openMin, S.closeMin);
    {   // key levels
        const int from = std::max(0, i - (int)(16LL * 86400 / per));
        std::vector<klv::Bar> B; B.reserve((size_t)(i - from + 1));
        for (int k = from; k <= i; ++k) { long d = 0; int e = 0; if (!parts((long long)times[k], d, e)) continue; B.push_back(klv::Bar{ d, e, hi[k], lo[k], cl[k] }); }
        int a0 = 18 * 60, a1 = 2 * 60; klv::asiaWindow(S.market.c_str(), a0, a1);
        const std::vector<klv::Level> L = klv::compute(B, per, S.openMin, S.closeMin, S.tick, a0, a1);
        for (const klv::Level& x : L) {
            std::string c = x.code; if (c == "AsiaHI") c = "AHI"; else if (c == "AsiaLO") c = "ALO";
            for (int j = 0; j < bba::NKEY; ++j) if (c == bba::KEY_NAMES[j]) { if ((S.set.keyMask >> j) & 1u) out.push_back(bba::NamedLevel{ c, x.price }); break; }
        }
    }
    if ((S.set.keyMask >> 10) & 3u) {   // HOD / LOD so far (RTH in the day, the Globex session at night), this candle included
        double h = -1e300, l = 1e300;
        for (int k = i; k >= 0 && k >= i - 600; --k) {
            long d = 0; int e = 0; if (!parts((long long)times[k], d, e)) break;
            const svl::Cls c = svl::classify(d, e, per, S.openMin, S.closeMin);
            if (c.sess != me.sess) break;
            if ((me.kind == svl::RTH || me.kind == svl::POST) && c.kind != svl::RTH && c.kind != svl::POST) break;
            h = std::max(h, (double)hi[k]); l = std::min(l, (double)lo[k]);
        }
        if (h > -1e299 && ((S.set.keyMask >> 10) & 1u)) out.push_back(bba::NamedLevel{ "HOD", h });
        if (l < 1e299 && ((S.set.keyMask >> 11) & 1u)) out.push_back(bba::NamedLevel{ "LOD", l });
    }
    if (me.sess == S.gamSess)
        for (const bba::NamedLevel& g : S.gam) for (int j = 0; j < bba::NGAM; ++j) if (g.name == bba::GAM_NAMES[j]) { if ((S.set.gamMask >> j) & 1u) out.push_back(g); break; }
    if (S.set.volMask) {   // (1.12.0) the ticked levels decide; a group box only ticks / clears its levels
        auto add = [&](int j, long long key) { if ((S.set.volMask >> j) & 1u) out.push_back(bba::NamedLevel{ bba::VOL_NAMES[j], key * S.tick }); };
        auto it = S.vp.find(me.sess);
        if (it != S.vp.end() && it != S.vp.begin()) {             // the prior day: the latest earlier session, fully loaded, its RTH profile
            auto p = std::prev(it);
            if (p->first > S.volFirstSess) { const bba::VA v = bba::valueArea(p->second.rth); if (v.ok) { add(0, v.poc); add(1, v.vah); add(2, v.val); } }
        }
        const int wd = svl::weekdayOf(me.sess); const long monday = me.sess - ((wd + 6) % 7);
        if (monday - 7 > S.volFirstSess) {                        // the prior week (Monday - Friday sessions), fully loaded
            std::map<long long, long long> wk;
            for (const auto& kv : S.vp) if (kv.first >= monday - 7 && kv.first <= monday - 3) for (const auto& r : kv.second.full) wk[r.first] += r.second;
            const bba::VA v = bba::valueArea(wk); if (v.ok) { add(3, v.poc); add(4, v.vah); add(5, v.val); }
        }
    }
    if (closed) { auto& slot = S.lvCache[t]; slot = out; if (S.lvCache.size() > 4000) S.lvCache.erase(S.lvCache.begin()); return slot; }
    live = out; return live;
}
// ---- (1.11.0) the statistics: prior day, the 5-session average (one outlier dropped), today so far - Big Lot 5 / 10 / 20 (day) --
void BigBidAsk::statsBuild(BBState& S, long long now) {
    if (S.statsMs >= 0 && now >= S.statsMs && now - S.statsMs < 5000) return;
    S.statsMs = now;
    std::map<long, std::vector<long long>> day, night; std::map<long, int> nCandles, nNight;   // (1.16.0) + the overnight prints
    const int per = S.spb > 0 ? S.spb : 180;
    long curSess = 0;
    for (const auto& kv : S.fin) {
        const bba::Facts& F = kv.second; if (!F.valid) continue;
        long d = 0; int e = 0; if (!parts(kv.first, d, e)) continue;
        const svl::Cls c = svl::classify(d, e, per, S.openMin, S.closeMin); if (c.kind == svl::NONE) continue;
        curSess = std::max(curSess, (long)c.sess);
        if (!dayOf(S, e / 60)) {   // (1.16.0) the overnight part of the session
            ++nNight[c.sess]; if (F.bid.ok) night[c.sess].push_back(F.bid.vol); if (F.ask.ok) night[c.sess].push_back(F.ask.vol); continue; }
        ++nCandles[c.sess];
        if (F.bid.ok) day[c.sess].push_back(F.bid.vol);
        if (F.ask.ok) day[c.sess].push_back(F.ask.vol);
    }
    const int need = std::max(20, (int)((S.closeMin + 60 - S.openMin) * 60 / per * 3 / 4));   // a complete day: 3/4 of its candles
    std::vector<long> done; for (const auto& kv : nCandles) if (kv.first < curSess && kv.second >= need) done.push_back(kv.first);
    std::vector<std::string> L;
    const int Ns[3] = { 5, 10, 20 };
    auto fmt = [](long long v) { std::string t = std::to_string(v); for (int k = (int)t.size() - 3; k > 0; k -= 3) t.insert((size_t)k, ","); return t; };
    auto dateOf = [](long s) { long z = s + 719468; const long era = (z >= 0 ? z : z - 146096) / 146097; const unsigned doe = (unsigned)(z - era * 146097);
        const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        const unsigned mp = (5 * doy + 2) / 153; const unsigned dd = doy - (153 * mp + 2) / 5 + 1; const unsigned mm = mp < 10 ? mp + 3 : mp - 9;
        static const char* M[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        return std::string(M[(mm - 1) % 12]) + " " + std::to_string(dd); };
    std::string dropNote;
    for (int q = 0; q < 3; ++q) {
        const int N = Ns[q];
        std::string line = "Big Lot " + std::to_string(N) + ":  prior day ";
        line += done.empty() ? std::string("learning") : fmt(bba::nth(day[done.back()], N));
        std::vector<long long> xs; std::vector<long> ss;
        for (int k = (int)done.size() - 1; k >= 0 && (int)xs.size() < 5; --k) { xs.push_back(bba::nth(day[done[k]], N)); ss.push_back(done[k]); }
        const bba::Avg a = bba::avgDropOutlier(xs);
        line += "  ·  " + std::to_string(xs.size()) + "-session avg " + (a.used ? fmt(a.avg) : std::string("learning"));
        if (a.dropped >= 0) { line += " (1 dropped)"; if (dropNote.empty()) { char r[16]; std::snprintf(r, sizeof(r), "%.1f", a.ratio); dropNote = "Dropped: " + dateOf(ss[a.dropped]) + " (" + r + "x the others)."; } }
        const long long thr = bba::bigLotOf(tiersAt(S, curSess), 1, N); int today = 0;   // (1.15.0) today's tiers (decided once)
        if (thr > 0) for (long long v : day[curSess]) if (v >= thr) ++today;
        line += "  ·  today " + std::to_string(today) + " of ~" + std::to_string(N);
        L.push_back(line);
    }
    {   const bba::TierSet& T = tiersAt(S, curSess);
        L.push_back("Night: Big Lot 5 / 10 / 20 = " + fmt(bba::bigLotOf(T, 0, 5)) + " / " + fmt(bba::bigLotOf(T, 0, 10)) + " / " + fmt(bba::bigLotOf(T, 0, 20)) + " lots.  Day now: "
                    + fmt(bba::bigLotOf(T, 1, 5)) + " / " + fmt(bba::bigLotOf(T, 1, 10)) + " / " + fmt(bba::bigLotOf(T, 1, 20)) + " lots (" + S.market + ", " + T.why + ")."); }
    L.push_back(dropNote.empty() ? std::string("No session dropped as an outlier.") : dropNote);
    {   // (1.12.0) the nightly score of his categories (lra.bba_prints -> LRA-BigPrints-<MKT>.csv): "SCORE|<text>"
        std::string text; const std::string f = flexDir();
        if (!f.empty() && readSmall(f + BBA_SEP "LRA-BigPrints-" + S.market + ".csv", 16 * 1024, text)) {
            std::istringstream in(text); std::string ln;
            while (std::getline(in, ln)) { if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (ln.compare(0, 6, "SCORE|") == 0 && ln.size() < 400) { S.scoreLine = ln.substr(6); break; } }
        }
        L.push_back(S.scoreLine);
    }
    S.stats = L;
    {   // (1.16.0) the compact box: both types' averages (the box shows the current type's, the hover both)
        const bba::TierSet& T = tiersAt(S, curSess);
        long long thrD[3], thrN[3]; for (int q = 0; q < 3; ++q) { thrD[q] = bba::bigLotOf(T, 1, bba::BOX_N[q]); thrN[q] = bba::bigLotOf(T, 0, bba::BOX_N[q]); }
        S.blDay = bba::typeAvg(day, nCandles, curSess, bba::typeNeed(false, S.openMin, S.closeMin, per), thrD, false);
        S.blNight = bba::typeAvg(night, nNight, curSess, bba::typeNeed(true, S.openMin, S.closeMin, per), thrN, true);
        S.blDropDay = S.blDay.droppedSess >= 0 ? dateOf(S.blDay.droppedSess) : std::string();
        S.blDropNight = S.blNight.droppedSess >= 0 ? dateOf(S.blNight.droppedSess) : std::string();
        S.blTiers = "Sizes the candles use this session: overnight " + fmt(bba::bigLotOf(T, 0, 5)) + " / " + fmt(bba::bigLotOf(T, 0, 10)) + " / " + fmt(bba::bigLotOf(T, 0, 20))
                  + ", RTH " + fmt(bba::bigLotOf(T, 1, 5)) + " / " + fmt(bba::bigLotOf(T, 1, 10)) + " / " + fmt(bba::bigLotOf(T, 1, 20)) + " lots (" + T.why + ").";
    }
    std::string text; for (const auto& x : L) text += x + "\n";
    if (text != S.statsWritten) { const std::string p = statsPath(S.market); if (!p.empty()) { makeDir(flexDir()); if (publishFile(p, text)) S.statsWritten = text; } }
}
// ---- (1.13.0) his vwapTrend at candle i (from the closed bars BEFORE it - what was known when the candle opened), kept per candle
void BigBidAsk::ensureBars(BBState& S) {
    RTARRAYI times(barDateTime);
    const long count = getBarCount(); const long long lastT = count > 0 ? (long long)times[(int)count - 1] : 0;
    if (S.svN == count && S.svLast == lastT) return;
    RTARRAY hi(barHigh), lo(barLow), cl(barClose); RTARRAYI vo(barVolume);
    S.svB.clear(); S.svB.reserve((size_t)count);
    for (int k = 0; k < (int)count; ++k) { long d = 0; int e = 0; parts((long long)times[k], d, e); svl::Bar b; b.days = d; b.endSec = e; b.h = hi[k]; b.l = lo[k]; b.c = cl[k]; b.v = (double)vo[k]; S.svB.push_back(b); }
    S.svN = count; S.svLast = lastT; S.bandN = -1;
}
// (1.14.0) one pass: the swing over ALL the bars (the candle's own pivot), the breakouts, the session VWAP / SD per bar
template <class H, class L>
void BigBidAsk::passCtx(BBState& S, H& hi, L& lo, int n, PassCtx& X) {
    X.sw = bba::swing(hi, lo, n);
    bba::breakFlags(hi, lo, n, X.bu, X.bd);
    ensureBars(S);
    if (S.bandN != S.svN) {
        std::vector<float> out; svl::run(S.svB, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin, out);
        S.vw.assign(S.svB.size(), 0.0); S.sdv.assign(S.svB.size(), 0.0);
        for (size_t k = 0; k < S.svB.size() && (k + 1) * 11 <= out.size(); ++k) {
            const svl::Cls c = svl::classify(S.svB[k].days, S.svB[k].endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
            const float* o = &out[k * 11] + (c.kind == svl::ON ? 5 : 0);
            if (c.kind != svl::NONE && o[0] > 0 && o[1] > o[0]) { S.vw[k] = o[0]; S.sdv[k] = (double)o[1] - (double)o[0]; }
        }
        S.bandN = S.svN;
    }
    X.vw = S.vw; X.sd = S.sdv;
}
// (1.19.0, Rassul 21:47 "if the bid is the biggest in 40 candles then show 40, but cap the max at 50") how many candles back the cell
// of side sd (-1 bid / +1 ask) at candle i is the biggest of its side: the run of earlier closed candles whose same-side biggest cell is
// SMALLER, up to bbaset::CAP. Read from the frozen record (never re-judged); a candle not read yet ends the run.
template <class T> int BigBidAsk::backCount(BBState& S, T& times, int i, int sd, long long vol) {
    if (vol <= 0) return 0;
    int n = 0;
    for (int k = i - 1; k >= 0 && n < bbaset::CAP; --k) {
        auto it = S.fin.find((long long)times[k]); if (it == S.fin.end()) break;
        const bba::Facts& F = it->second; const bba::Pick& q = sd < 0 ? F.bid : F.ask;
        if ((F.valid && q.ok ? q.vol : 0) >= vol) break;
        ++n;
    }
    return n;
}
int BigBidAsk::trendAt(BBState& S, int i) {
    RTARRAYI times(barDateTime); const long long t = (long long)times[i];
    auto it = S.trCache.find(t); if (it != S.trCache.end()) return it->second;
    const long count = getBarCount();
    ensureBars(S);
    const vtl::Trend tr = vtl::fromChart(S.svB, i, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
    const int lvl = tr.ready ? vtl::level(tr) : 0;
    if (i < (int)count - 1) { S.trCache[t] = lvl; if (S.trCache.size() > 6000) S.trCache.erase(S.trCache.begin()); }
    return lvl;
}
// ---- (1.12.0) the category of one print (the same rule for the chart and the print log): his categories, the biggest size first
// (= the rarest); the first ON category it reaches whose Show filter it passes. (1.14.0) one exclusive Show choice per category:
//   All prints; Near pivot = the candle passes his pivot rule in EITHER direction (its own swing's down or up pivot); Near a level;
//   Pivot near a level (both); Trend PB = trend up: the down-pivot rule + the band rule on the pullback (mirror down); Trend BO = trend
//   up: the candle went through the higher of the last 2 high pivots (mirror down); Trend PB + BO = either. Any print side.
//   Category 4 = every print of its size (only the time filter, applied before).
// why: 1 kept out by the pivot, 2 by the level, 4 by Trend PB / BO (no trend, wrong direction or the band rule)
template <class H, class L, class C>
PickInfo BigBidAsk::choose(BBState& S, int i, bool closed, int sd, const bba::Pick& p, int ph, const bba::TierSet& T, const PassCtx& X, H& hi, L& lo, C& cl) {
    PickInfo r; const bba::Settings& set = S.set; (void)sd;
    const bool inSw = i >= 0 && i < (int)X.sw.bull.size();
    const bool bull = inSw && X.sw.bull[i], bear = inSw && X.sw.bear[i], pivAny = bull || bear;
    r.conf = inSw ? X.sw.conf[i] != 0 : true; r.piv = pivAny;
    int ord[4] = { 0, 1, 2, 3 }; long long zz[4]; for (int k = 0; k < 4; ++k) zz[k] = set.cat[k].on ? bba::catSizeOf(T, ph, set.cat[k]) : 0;   // (1.15.0) the candle's session tiers
    bba::catOrder(zz, ord);   // (1.16.0, Rassul 15:04-15:07) category 1, then HIS category 4, then 2 / 3 (was: all four biggest size first)
    const double px = (double)p.key * S.tick;
    bool trendRead = false; int trend = 0;
    for (int q = 0; q < 4 && r.cat < 0; ++q) { const int k = ord[q]; const bba::Cat& c = set.cat[k];
        if (zz[k] <= 0 || p.vol < zz[k]) continue;
        if (k == 3) { r.cat = 3; break; }                              // (Rassul 15:53) category 4: wherever it is
        const int sh = c.show;
        if ((sh == bba::SH_PIVOT || sh == bba::SH_PIVLEVEL) && !pivAny) { r.why |= 1; continue; }
        if (sh == bba::SH_LEVEL || sh == bba::SH_PIVLEVEL) {
            const std::vector<bba::NamedLevel>& LV = levelsFor(S, i, closed);
            const double atr = bba::atr14(hi, lo, cl, i);
            const bba::NamedLevel* nlv = bba::nearLevel(hi, lo, cl, i, px, LV, atr, set.atrUpQ, set.atrDnQ);
            if (!nlv) { r.why |= 2; continue; }
            r.lv = nlv->name; r.lvPx = nlv->px;
            const double up = atr * set.atrUpQ / 100.0, dn = atr * set.atrDnQ / 100.0;
            r.sweep = !(atr > 0 && px >= nlv->px - dn && px <= nlv->px + up);   // only the sweep rule let it in
        }
        if (sh == bba::SH_PB || sh == bba::SH_BO || sh == bba::SH_PBBO) {
            if (!trendRead) { trend = trendAt(S, i); trendRead = true; r.trend = trend; }
            int how = 0;
            if ((sh == bba::SH_PB || sh == bba::SH_PBBO) && trend != 0 && (trend > 0 ? bull : bear) && bba::pbBands(hi, lo, X.vw, X.sd, X.sw, i, trend)) how = 1;
            if (!how && (sh == bba::SH_BO || sh == bba::SH_PBBO) && trend != 0) {
                const int w = i < (int)X.bu.size() && (trend > 0 ? bba::upWave(X.sw, i) : bba::downWave(X.sw, i)) ? (trend > 0 ? X.bu[i] : X.bd[i]) : 0;   // (1.14.1) in the wave
                if (w) { how = 3; r.brk = w; r.brkDir = trend > 0 ? 1 : -1; }
            }
            if (!how) { r.why |= 4; continue; }
            r.how = how;
        }
        r.cat = k; }
    if (r.cat >= 0 && r.lv.empty()) {   // the hover names the nearest selected level even when the category has no level filter
        const std::vector<bba::NamedLevel>& LV = levelsFor(S, i, closed);
        const bba::NamedLevel* nlv = bba::nearLevel(hi, lo, cl, i, px, LV, bba::atr14(hi, lo, cl, i), set.atrUpQ, set.atrDnQ);
        if (nlv) { r.lv = nlv->name; r.lvPx = nlv->px; }
    }
    return r;
}
// ---- (1.15.0, audit B finding 2) the tiers of ONE session, decided once: from the complete sessions before it (bba::tiersFor), and only
// when every chart bar of the 7 days before it is final (or the chart starts after that). Kept for the life of the chart state; the same
// data gives the same tiers after a restart (the facts are recorded for 8 sessions). nullptr = not decidable yet (history still reading).
const bba::TierSet* BigBidAsk::tierFor(BBState& S, long sess) {
    auto it = S.tiers.find(sess); if (it != S.tiers.end()) return &it->second;
    if (!(S.covStart || S.covSess < sess - 7)) return nullptr;
    if (S.bySessN != S.fin.size()) {                                             // the prints per session (rebuilt when facts were added)
        S.bySess.clear(); const int per = S.spb > 0 ? S.spb : 180;
        for (const auto& kv : S.fin) {
            const bba::Facts& F = kv.second; if (!F.valid || !F.closed) continue;
            long d = 0; int e = 0; if (!parts(kv.first, d, e)) continue;
            const svl::Cls c = svl::classify(d, e, per, S.openMin, S.closeMin); if (c.kind == svl::NONE) continue;
            bba::SessPrints& P = S.bySess[c.sess]; const int ph = dayOf(S, e / 60) ? 1 : 0;
            if (ph == 1) ++P.dayCandles;
            if (F.bid.ok) P.pv[ph].push_back(F.bid.vol);
            if (F.ask.ok) P.pv[ph].push_back(F.ask.vol);
        }
        S.bySessN = S.fin.size();
    }
    bba::TierSet& T = S.tiers[sess];
    T = bba::tiersFor(sess, S.bySess, dayNeed(S));
    if (S.tiers.size() > 400) S.tiers.erase(S.tiers.begin());
    return &S.tiers[sess];
}
// the SIZES status line: today's tiers in force and where they came from
void BigBidAsk::tierStatus(BBState& S, long long now) {
    if (S.sdCheckMs >= 0 && now >= S.sdCheckMs && now - S.sdCheckMs < 5000) return;
    S.sdCheckMs = now;
    long cur = 0;
    { const long count = getBarCount(); RTARRAYI times(barDateTime); long d = 0; int e = 0;
      if (count > 0 && times.count >= count && parts((long long)times[(int)count - 1], d, e)) cur = svl::classify(d, e, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin).sess; }
    const bba::TierSet* T = cur ? tierFor(S, cur) : nullptr;
    std::string t = "Big Lot tiers of session " + std::to_string(cur) + ": ";
    if (!T) t += "waiting for the chart history (covered from session " + (S.covSess == LONG_MAX ? std::string("-") : std::to_string(S.covSess)) + ")";
    else {
        t += T->why; if (!T->used.empty()) { t += " ("; for (size_t k = 0; k < T->used.size(); ++k) t += (k ? " " : "") + std::to_string(T->used[k]); t += ")"; }
        for (int k = 0; k < 3; ++k) { const bba::Cat& c = S.set.cat[k];
            t += std::string(k ? " |" : ";") + " Big Lot " + std::to_string(c.n) + (c.lots > 0 ? " min " + std::to_string(c.lots) : std::string()) + (c.on ? "" : " (off)") + ": day " + std::to_string(bba::catSizeOf(*T, 1, c)) + " / night " + std::to_string(bba::catSizeOf(*T, 0, c)) + " lots"; }
        t += "; decided once for the session (fixed for its candles)";
    }
    t += "; sessions decided " + std::to_string(S.tiers.size()) + "; board events " + std::to_string(S.boardN);
    S.sdWhy = t;
}
// ---- (1.15.0, audit B) the SignalBoard B events from the RECORD: the last 120 closed candles, whatever is on screen; the same filters
// and tiers as the chart. Bit 0 circle, 1-3 category, 4 at a level, 5 sweep, 6 pivot rule passed, 7 pivot CONFIRMED, 8-14 Big Lot N.
void BigBidAsk::boardPass(BBState& S, long long now) {
    const long count = getBarCount(); if (count < 3 || S.tick <= 0) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if (times.count < count || hi.count < count || lo.count < count || cl.count < count) return;
    const long long lastT = (long long)times[(int)count - 1];
    if (lastT == S.boardLast && S.boardMs >= 0 && now >= S.boardMs && now - S.boardMs < 1000) return;   // once a second or on a new bar
    S.boardLast = lastT; S.boardMs = now;
    PassCtx X; passCtx(S, hi, lo, (int)count, X);
    const bba::Settings& set = S.set;
    std::vector<sb::Ev> ev;
    for (int i = std::max(0, (int)count - 1 - 120); i < (int)count - 1; ++i) {
        const long long t = (long long)times[i];
        auto it = S.fin.find(t); if (it == S.fin.end() || !it->second.valid) continue;
        const bba::Facts& F = it->second;
        long days = 0; int endSec = 0; if (!parts(t, days, endSec)) continue;
        const svl::Cls cls = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin); if (cls.kind == svl::NONE) continue;
        if (timeSkipped(set, cls, endSec, S.openMin, S.closeMin)) continue;
        for (int sd = -1; sd <= 1; sd += 2) {   // (1.18.0) the chart's one rule: the biggest cell of its side in the last 10 candles
            const bba::Pick& p = sd < 0 ? F.bid : F.ask;
            if (!p.ok) continue;
            const int back = backCount(S, times, i, sd, p.vol); if (back < S.topN) continue;
            sb::Ev e; e.bar = ctSec((RTDATE)t); e.px = (double)p.key * S.tick; e.side = sd; e.val = (double)p.vol;
            e.flag = (1 << 1) | 128 | ((back & 0x7F) << 8);   // (1.19.0) category 1, final; n = the candles back (<= 50)
            { const int sm = (S.sab.bound ? ((S.sab.st.atRaw(sab::LOW, t) ? 1 : 0) | (S.sab.st.atRaw(sab::HIGH, t) ? 2 : 0)) : 0); if (sm & (sd < 0 ? 1 : 2)) e.flag |= 1; }   // (1.22.0) bit 0 = the print is on a shared A candle of its side
            if (e.bar > 0) ev.push_back(e);
        }
    }
    S.boardN = (int)ev.size();
    const char* sy = getSymbol(); const char* chl = getChartLabel();
    sb::publish('B', sy ? sy : "", getSecondsPerBar(), chl ? chl : "", steadyMs(), ev);
}
// ---- (1.21.0, Rassul approved breakout-mockup-v2 20:10 CT 2026-10-21) BREAKOUT / BREAKDOWN: each closed candle decided ONCE at its
// close from the chart's bars (his ZigZag over every bar, bba::breakFlagsPx) and the frozen facts (S.fin: the biggest cell per side);
// a decision waits until the candle, the one before it and the 50 before that are final. Candidates (a pivot traded through) are
// recorded as BBO| lines - fired or not, with the reason - and reloaded on a restart (load), so a candle is never judged twice.
void BigBidAsk::boPass(BBState& S, long long now) {
    const long count = getBarCount(); if (count < 12 || S.tick <= 0) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), op(barOpen), cl(barClose); RTARRAYI vo(barVolume);   // (1.23.0) open / close: the candle colour and the close beyond the pivot
    if (times.count < count || hi.count < count || lo.count < count || op.count < count || cl.count < count || vo.count < count) return;
    const long long lastT = (long long)times[(int)count - 1];
    if (!(lastT == S.boLast && S.boMs >= 0 && now >= S.boMs && now - S.boMs < 1000)) {   // once a second or on a new bar
        S.boLast = lastT; S.boMs = now;
        std::vector<signed char> up, dn, zd; std::vector<double> upPx, dnPx;
        bba::swingBreaksPx(hi, lo, (int)count, up, dn, upPx, dnPx);         // (1.26.0) the last 3 INTACT swings (was the higher of the last 2 pivots)
        bba::zigDir(hi, lo, (int)count, zd);                                  // (1.23.0) the wave of every candle (same walk)
        const int emitFrom = std::max(1, (int)count - 1 - 300), from = std::max(1, emitFrom - 60), fill0 = std::max(0, from - (bba::BO_CAP + 12));
        std::vector<bba::BoIn> in((size_t)count); std::vector<bba::BoBar> bars((size_t)count); std::vector<char> skip((size_t)count, 0);
        std::vector<int> rthOf((size_t)count, 0);
        for (int k = fill0; k < (int)count - 1; ++k) {
            const long long t = (long long)times[k];
            bba::BoBar& bb = bars[(size_t)k]; bb.ok = keyOf(op[k], S.tick, bb.o) && keyOf(cl[k], S.tick, bb.c);
            long days = 0; int endSec = 0; svl::Cls cls; cls.kind = svl::NONE;
            if (parts(t, days, endSec)) cls = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
            rthOf[(size_t)k] = cls.kind == svl::RTH ? 1 : 0; skip[(size_t)k] = (cls.kind == svl::NONE || timeSkipped(S.set, cls, endSec, S.openMin, S.closeMin)) ? 1 : 0;
            const auto it = S.fin.find(t); if (it == S.fin.end()) continue;
            bba::BoIn& b = in[(size_t)k];
            if (!keyOf(hi[k], S.tick, b.hk) || !keyOf(lo[k], S.tick, b.lk)) continue;
            b.vol = (long long)vo[k]; b.known = true;
            const bba::Facts& F = it->second;
            if (F.valid && F.bid.ok) { b.big[0] = F.bid.vol; b.bigKey[0] = F.bid.key; }
            if (F.valid && F.ask.ok) { b.big[1] = F.ask.vol; b.bigKey[1] = F.ask.key; }
        }
        // (1.23.0) green / red + the same-wave rule over the closed candles (a pure function of them); only decisions not yet recorded
        // are added - a breakout candle already decided (t0 in the record) is never decided again, and a fired break whose recorded mark
        // already covers this break (same side and pivot, marked at or after its breakout candle) is not drawn twice (no repaint)
        const bba::BoRun R = bba::boWaves(in, bars, zd, up, dn, upPx, dnPx, skip, S.tick, from, (int)count - 1, emitFrom);
        for (bba::BoDec D : R.ev) {
            if (D.i0 < 0 || D.im < 0 || D.im >= (int)count - 1) continue;
            D.t0 = (long long)times[D.i0]; D.t = (long long)times[D.im];
            if (S.boDec.count(D.t0)) continue;
            if (D.fired) { bool dup = false;
                for (const auto& kv : S.boDec) { const bba::BoDec& O = kv.second; if (O.fired && O.dir == D.dir && O.piv == D.piv && O.t >= D.t0) { dup = true; break; } }
                if (dup) { ++S.boDup; continue; } }
            S.boDec[D.t0] = D;
            const long long sess = sessionOf(D.t); if (sess > 0) S.boPending.push_back({ sess, bba::boLine(D, rthOf[(size_t)D.im], BBA_VERSION) });
        }
        for (int sd = 0; sd < 2; ++sd) { S.boWaitOn[sd] = R.pendOn[sd] && R.pend[sd].i0 >= 0; S.boWaitT0[sd] = S.boWaitOn[sd] ? (long long)times[R.pend[sd].i0] : 0; S.boWaitPiv[sd] = S.boWaitOn[sd] ? R.pend[sd].piv : 0; }
        while (S.boDec.size() > 20000) S.boDec.erase(S.boDec.begin());
    }
    if (S.boPending.empty() || (S.boFlushMs >= 0 && now >= S.boFlushMs && now - S.boFlushMs < 1000)) return;
    S.boFlushMs = now;
    const std::string d = dir(); if (d.empty()) return;
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<std::string>> by; for (const auto& x : S.boPending) by[x.first].push_back(x.second);
    std::vector<std::pair<long long, std::string>> retry;
    for (const auto& g : by) {
        const std::string path = boPath(S, g.first); std::string old; long long st = -1;
        if (statFile(path, st) && !readSmall(path, REC_FILE_MAX, old)) { ++S.boRecFails; for (const auto& l : g.second) retry.push_back({ g.first, l }); continue; }   // never rewrite what could not be read
        if (old.empty()) old = std::string(bba::BO_HEADER) + "\n";
        for (const auto& ln : g.second) old += ln + "\n";
        if (old.size() > REC_FILE_MAX || !publishFile(path, old)) { ++S.boRecFails; for (const auto& l : g.second) retry.push_back({ g.first, l }); }
    }
    S.boPending.swap(retry);
}
// ---- (1.24.0, Rassul approved the Rly mockup v0.2 15:10-15:15 CT 2026-10-22) RLY: on every new bar, the chart's CLOSED bars go
// through RallyLogic.h (rly2.py v0.2); each confirmed wave of the last 3 RTH sessions is judged ONCE - a wave already in the record
// (loaded at the session start or decided earlier) is never judged again, so a mark never changes or disappears. 3-min charts only.
void BigBidAsk::rlyPass(BBState& S, long long now) {
    if (S.spb != 180) return;                                            // the rule was built and tested on 3-min bars
    const long count = getBarCount(); if (count < 3) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), op(barOpen), cl(barClose); RTARRAYI vo(barVolume);
    if (times.count < count || hi.count < count || lo.count < count || op.count < count || cl.count < count || vo.count < count) return;
    const long long lastT = (long long)times[(int)count - 1];
    if (lastT != S.rlyLast || count != S.rlyCount) {
        S.rlyLast = lastT; S.rlyCount = count;
        const int first = std::max(0, (int)count - 1 - 20000);            // ATR settles in ~100 bars; cap the work
        std::vector<rly::Bar> B; std::vector<long long> T; B.reserve((size_t)(count - first)); T.reserve((size_t)(count - first));
        for (int k = first; k < (int)count - 1; ++k) {                   // closed bars only (the last bar is forming)
            const long long t = (long long)times[k]; long d = 0; int e = 0;
            const double o = op[k], h = hi[k], l = lo[k], c = cl[k];
            if (!parts(t, d, e) || !std::isfinite(o) || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c) || h < l) continue;
            rly::Bar b; b.days = d; b.endSec = e; b.o = o; b.h = h; b.l = l; b.c = c; b.v = (double)(long long)vo[k];
            B.push_back(b); T.push_back(t);
        }
        const int from = std::max(0, (int)B.size() - 600);
        const std::vector<rly::Wave> W = rly::run(B, S.spb, S.openMin, S.closeMin, from);
        for (const rly::Wave& w : W) {
            if (w.sess < S.session - 2 || w.ge < 0 || w.gp < 0 || w.gc < 0) continue;
            const long long te = T[(size_t)w.ge], key = te * 2 + (w.dir > 0 ? 1 : 0);
            if (S.rlyRec.count(key)) continue;                              // judged before: the record wins (never repaint)
            S.rlyRec[key] = rly::toRec(w, te, T[(size_t)w.gp], T[(size_t)w.gc]);
            S.rlyPending.push_back({ (long long)w.sess, rly::line(w, te, T[(size_t)w.gp], T[(size_t)w.gc], BBA_VERSION) });
        }
        while (S.rlyRec.size() > 5000) S.rlyRec.erase(S.rlyRec.begin());
    }
    if (S.rlyPending.empty() || (S.rlyFlushMs >= 0 && now >= S.rlyFlushMs && now - S.rlyFlushMs < 1000)) return;
    S.rlyFlushMs = now;
    const std::string f = flexDir(); if (f.empty()) return;
    makeDir(f); makeDir(f + BBA_SEP "Rly");
    std::map<long long, std::vector<std::string>> by; for (const auto& x : S.rlyPending) by[x.first].push_back(x.second);
    std::vector<std::pair<long long, std::string>> retry;
    for (const auto& g : by) {
        const std::string path = rlyPath(S, g.first); std::string old; long long st = -1;
        if (statFile(path, st) && !readSmall(path, REC_FILE_MAX, old)) { ++S.rlyRecFails; for (const auto& l : g.second) retry.push_back({ g.first, l }); continue; }   // never rewrite what could not be read
        if (old.empty()) old = std::string(rly::REC_HEADER) + "\n";
        for (const auto& ln : g.second) old += ln + "\n";
        if (old.size() > REC_FILE_MAX || !publishFile(path, old)) { ++S.rlyRecFails; for (const auto& l : g.second) retry.push_back({ g.first, l }); }
    }
    S.rlyPending.swap(retry);
}
// ---- (1.25.0, agreed with Rassul 16:00-16:01 CT 2026-10-22) OB / BO BOXES of STRONG WAVES: on every new bar the chart's CLOSED bars go
// through OrderBlockLogic.h (the RallyLogic waves); each box of a confirmed strong wave of the last 3 RTH sessions is decided ONCE - its
// geometry, and the big prints of its candles (volume at price, the session's Big Lot 20 tier or the candle's own Big Bid / Ask print) -
// and recorded (OBX|); a box already in the record is never decided again (no repaint). A box waits (at most 2 minutes) for its session's
// tiers and its candles' volume at price. Its tests / end are rebuilt from the closed bars (a pure function of them) and each resolved test
// is appended once (OBT|) for the nightly. 3-min charts only (the waves were built and tested on 3-min bars).
void BigBidAsk::obPass(BBState& S, long long now) {
    if (S.spb != 180 || S.tick <= 0 || S.faulted) return;
    const long count = getBarCount(); if (count < 3) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), op(barOpen), cl(barClose); RTARRAYI vo(barVolume); RTARRAYP vap(barVolumeProfile);
    if (times.count < count || hi.count < count || lo.count < count || op.count < count || cl.count < count || vo.count < count || vap.count < count) return;
    const long long lastT = (long long)times[(int)count - 1];
    const bool fresh = lastT != S.obLast || count != S.obCount;
    const bool retry = !S.obWait.empty() && (S.obRetryMs < 0 || now < S.obRetryMs || now - S.obRetryMs >= 2000);
    if (fresh || retry) {
        S.obLast = lastT; S.obCount = count; S.obRetryMs = now;
        const int first = std::max(0, (int)count - 1 - 20000);
        std::vector<rly::Bar> B; std::vector<long long> T; std::vector<int> G; B.reserve((size_t)(count - first)); T.reserve((size_t)(count - first)); G.reserve((size_t)(count - first));
        for (int k = first; k < (int)count - 1; ++k) {                   // closed bars only (the last bar is forming) - the same bars as rlyPass
            const long long t = (long long)times[k]; long d = 0; int e = 0;
            const double o = op[k], h = hi[k], l = lo[k], c = cl[k];
            if (!parts(t, d, e) || !std::isfinite(o) || !std::isfinite(h) || !std::isfinite(l) || !std::isfinite(c) || h < l) continue;
            rly::Bar b; b.days = d; b.endSec = e; b.o = o; b.h = h; b.l = l; b.c = c; b.v = (double)(long long)vo[k];
            B.push_back(b); T.push_back(t); G.push_back(k);
        }
        obk::Params P; P.atrB = S.obAtr; P.origin = S.obOrigin; P.tick = S.tick;
        const int from = std::max(0, (int)B.size() - 600);
        const std::vector<obk::Cand> C = obk::find(B, S.spb, S.openMin, S.closeMin, P, from);
        int reads = 0;
        for (const obk::Cand& c : C) {
            if (c.sess < S.session - 2 || c.i0 < 0 || c.i1 < c.i0 || c.ip < 0 || c.ie < 0 || c.ic < 0) continue;
            obk::Box b; b.dir = c.dir; b.cls = c.cls; b.tp = T[(size_t)c.ip]; b.te = T[(size_t)c.ie]; b.tc = T[(size_t)c.ic];
            b.pivot = c.pivot; b.nb = c.nb; b.ratio = c.ratio; b.atr = c.atr; b.rly = c.rly; b.origin = c.i1 - c.i0;
            const long long id = b.id();
            if (S.obRec.count(id)) { S.obWait.erase(id); continue; }             // decided before: the record wins (never repaint)
            auto w = S.obWait.find(id); if (w == S.obWait.end()) w = S.obWait.insert(std::make_pair(id, now)).first;
            const bool late = now < w->second || now - w->second >= 120000;
            // (1.26.0) the OB candles' volume at price: every bid / ask cell of the pivot candle and the next 1 - 3 candles
            std::vector<obk::CandleCells> cells; bool ok = true;
            for (int j = c.i0; j <= c.i1 && ok; ++j) {
                const int gi = G[(size_t)j]; if (++reads > 60) { ok = false; break; }
                BARSTATISTICS st; const int n = readStats(&vap, gi, &st);
                if (n == -1) { S.faulted = true; ++S.faults; return; }
                if (n <= 0 || n > 4000) { ok = false; break; }
                std::vector<VOLPROFILE> rows((size_t)n);
                const int got = readRows(&vap, gi, &rows[0], n);
                if (got == -1) { S.faulted = true; ++S.faults; return; }
                if (got != n) { ok = false; break; }
                obk::CandleCells cc; cc.t = T[(size_t)j];
                for (const VOLPROFILE& v : rows) { long long k = 0; if (!keyOf((double)v.price, S.tick, k) || v.buyVolume < 0 || v.sellVolume < 0) continue;
                    cc.px.push_back((double)k * S.tick); cc.bid.push_back((long long)v.sellVolume); cc.ask.push_back((long long)v.buyVolume); }
                cells.push_back(cc);
            }
            if (!ok && !late) continue;                                          // volume at price not there yet: next pass
            {   bool facts = true; for (int j = c.i0; j <= c.i1 + 1 && facts; ++j) if (j < (int)T.size() && !S.fin.count(T[(size_t)j])) facts = false;
                if (!facts && !late) continue; }                                  // the BO decisions of these candles need their facts first
            // the prints a BO mark already shows on these candles (the BO decisions of those candles are made: boPass runs first)
            std::vector<obk::Print> taken;
            for (const auto& kv : S.boDec) { const bba::BoDec& D = kv.second; if (!D.fired || !D.print || D.lots <= 0 || D.t < T[(size_t)c.i0] || D.t > T[(size_t)c.i1]) continue;
                obk::Print q; q.t = D.t; q.px = (double)D.key * S.tick; q.side = D.dir > 0 ? 1 : -1; q.lots = D.lots; taken.push_back(q); }
            int rank = 0; const obk::Print pk = ok ? obk::pick(cells, taken, S.tick, rank) : obk::Print();
            if (!ok) ++S.obNoVap;
            if (rank > 0) { b.prints.push_back(pk); b.rank = rank; b.t0 = b.t1 = pk.t; b.lo = b.hi = pk.px; }
            else { b.rank = 0; b.t0 = b.t1 = T[(size_t)c.i0]; b.lo = b.hi = c.pivot; }   // no print: logged, not drawn
            S.obRec[id] = b; S.obWait.erase(id);
            S.obPending.push_back(std::make_pair((long long)c.sess, obk::line(b, BBA_VERSION)));
        }
        for (auto it = S.obWait.begin(); it != S.obWait.end();) { if (now >= it->second && now - it->second > 600000) it = S.obWait.erase(it); else ++it; }
        while (S.obRec.size() > 3000) S.obRec.erase(S.obRec.begin());
        // the tests and the end of every recorded box: the closed bars of its session from its confirming bar (a pure function of them)
        S.obTrk.clear(); S.obEndT.clear();
        for (const auto& kv : S.obRec) {
            const obk::Box& b = kv.second; if (!b.has()) continue;              // (1.26.0) the visits to the OB print's price (log only)
            const auto k0 = std::lower_bound(T.begin(), T.end(), b.tc); if (k0 == T.end() || *k0 != b.tc) continue;
            size_t k = (size_t)(k0 - T.begin());
            const svl::Cls c0 = svl::classify(B[k].days, B[k].endSec, S.spb, S.openMin, S.closeMin);
            std::vector<obk::OHLC> bars; bool done = false; long long lastIn = 0;
            for (; k < B.size(); ++k) {
                const svl::Cls cx = svl::classify(B[k].days, B[k].endSec, S.spb, S.openMin, S.closeMin);
                if (cx.sess != c0.sess || cx.kind != svl::RTH) { done = true; break; }
                obk::OHLC x; x.t = T[k]; x.h = B[k].h; x.l = B[k].l; x.c = B[k].c; bars.push_back(x); lastIn = T[k];
            }
            const obk::Track tr = obk::track(b, bars);
            S.obTrk[kv.first] = tr;
            S.obEndT[kv.first] = tr.ended ? tr.tEnd : (done ? lastIn : 0);   // 0 = alive (extends to the last bar)
            for (const obk::Test& t : tr.tests) {
                if (t.outcome == obk::T_OPEN) continue;
                const std::string key = std::to_string(kv.first) + "|" + std::to_string(t.n);
                if (S.obTestDone.count(key)) continue;
                S.obTestDone.insert(key);
                S.obPending.push_back(std::make_pair((long long)c0.sess, obk::testLine(b, t, BBA_VERSION)));
            }
        }
    }
    if (S.obPending.empty() || (S.obFlushMs >= 0 && now >= S.obFlushMs && now - S.obFlushMs < 1000)) return;
    S.obFlushMs = now;
    const std::string f = flexDir(); if (f.empty()) return;
    makeDir(f); makeDir(f + BBA_SEP "OrderBlock");
    std::map<long long, std::vector<std::string>> by; for (const auto& x : S.obPending) by[x.first].push_back(x.second);
    std::vector<std::pair<long long, std::string>> retryL;
    for (const auto& g : by) {
        const std::string path = obPath(S, g.first); std::string old; long long st = -1;
        if (statFile(path, st) && !readSmall(path, REC_FILE_MAX, old)) { ++S.obRecFails; for (const auto& l : g.second) retryL.push_back(std::make_pair(g.first, l)); continue; }
        if (old.empty()) old = std::string(obk::REC_HEADER) + "\n" + obk::TEST_HEADER + "\n";
        for (const auto& ln : g.second) old += ln + "\n";
        if (old.size() > REC_FILE_MAX || !publishFile(path, old)) { ++S.obRecFails; for (const auto& l : g.second) retryL.push_back(std::make_pair(g.first, l)); }
    }
    S.obPending.swap(retryL);
}
// ---- (1.12.0, Rassul 15:10 "score every print") the print log: every closed candle's shown prints, decided once at the close with the
// settings of that moment, one file per session (<stem>-prints-<session>.csv) - the nightly lra.bba_prints scores what price did next
void BigBidAsk::logPass(BBState& S, long long now) {
    const long count = getBarCount(); if (count < 3 || S.tick <= 0) return;
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), cl(barClose);
    if (times.count < count || hi.count < count || lo.count < count || cl.count < count) return;
    const int from = std::max(0, (int)count - 1 - 300);
    // (1.14.0) the log keeps the FINAL decision: a candle is logged once its own swing's pivot is confirmed (the chart may show,
    // hide and show it again before that) - the nightly scores only what stayed
    PassCtx X; passCtx(S, hi, lo, (int)count, X);
    const bba::Settings& set = S.set;
    for (int i = from; i < (int)count - 1; ++i) {
        // (1.18.0) the one rule is final at the candle's close: no pivot wait
        const long long t = (long long)times[i];
        auto it = S.fin.find(t); if (it == S.fin.end() || !it->second.valid) continue;
        const bba::Facts& F = it->second;
        long days = 0; int endSec = 0; if (!parts(t, days, endSec)) continue;
        const svl::Cls cls = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin); if (cls.kind == svl::NONE) continue;
        const int ph = dayOf(S, endSec / 60) ? 1 : 0;
        const bba::TierSet* TS = tierFor(S, cls.sess);   // (1.15.0) not decided yet (history still reading): logged later, never as "nothing"
        if (!TS) continue;
        for (int sd = -1; sd <= 1; sd += 2) {
            const std::string key = std::to_string(t) + (sd < 0 ? "b" : "a");
            if (S.logged.count(key)) continue;
            const bba::Pick& p = sd < 0 ? F.bid : F.ask;
            bool skip = !p.ok || timeSkipped(set, cls, endSec, S.openMin, S.closeMin);   // (1.15.0) the shared time filter (1.13.0 RTH only)
            PickInfo pi; if (!skip && backCount(S, times, i, sd, p.vol) >= S.topN) { pi.cat = 0; pi.conf = true; }   // (1.19.0) Biggest in
            S.logged.insert(key);
            if (pi.cat < 0) continue;
            const bba::Cat& c = set.cat[pi.cat];
            char b[360]; std::snprintf(b, sizeof(b), "%lld|%d|%lld|%d|%d|%d|%d|%s|%.10g|%d|%d|%.10g|%d|%s|%d|%d|%d|%d",
                t, sd, p.vol, pi.cat + 1, c.n, c.lots, ph, pi.lv.empty() ? "-" : pi.lv.c_str(), pi.lvPx, pi.sweep ? 1 : 0, pi.piv ? 1 : 0, (double)p.key * S.tick,
                pi.cat == 3 ? 0 : (c.show == bba::SH_LEVEL ? 1 : c.show == bba::SH_PIVOT ? 2 : c.show == bba::SH_PIVLEVEL ? 3 : c.show == bba::SH_PB ? 4 : c.show == bba::SH_BO ? 8 : c.show == bba::SH_PBBO ? 12 : 0),
                BBA_VERSION, pi.how, pi.brk * (pi.brkDir < 0 ? -1 : 1), pi.trend, cls.kind == svl::RTH ? 1 : 0);
            S.logPending.push_back({ (long long)cls.sess, b });
        }
    }
    if (S.logPending.empty() || (S.logMs >= 0 && now - S.logMs < 2000)) return;
    S.logMs = now;
    const std::string d = dir(); if (d.empty()) return;
    makeDir(flexDir()); makeDir(d);
    std::map<long long, std::vector<std::string>> by; for (const auto& x : S.logPending) by[x.first].push_back(x.second);
    bool allOk = true;
    for (const auto& g : by) {
        const std::string path = printsPath(S, g.first); std::string old;
        long long st = -1; if (statFile(path, st) && !readSmall(path, REC_FILE_MAX, old)) { allOk = false; continue; }
        if (old.empty()) old = "t|side|lots|cat|n|min_lots|day|level|level_px|sweep|pivot|price|filters|version|setup|brk|trend|rth\n";
        for (const auto& ln : g.second) old += ln + "\n";
        if (old.size() > REC_FILE_MAX || !publishFile(path, old)) allOk = false;
    }
    if (allOk) S.logPending.clear();
}
void BigBidAsk::render(BBState& S) {
    S.targets.clear(); S.clL = S.clR = S.clT = S.clB = S.clTr = S.clBig = S.clHl = S.slidHl = S.shifted = S.areaFallback = 0; S.circles = 0; S.circlesHidden = 0; S.droppedInView = 0; S.zoneFull = 0; S.zoneDim = 0; S.hugeN = 0; S.catN4[0] = S.catN4[1] = S.catN4[2] = S.catN4[3] = 0; S.pivotHidden = 0; S.levelHidden = 0; S.timeHidden = 0; S.trendHidden = 0; S.unconfirmed = 0;
    S.boLetters = S.boPrints = S.boShared = S.boKept = S.boNoRoom = 0;   // (1.21.0)
    S.rlyLetters = S.rlyNoRoom = 0; S.rlyTargets.clear();                // (1.24.0)
    S.obLabels = S.obNoLabel = S.obPrints = S.obShared = 0; S.obTargets.clear();   // (1.26.0)
    const long count = getBarCount();
    if (count < 2 || S.tick <= 0) return;
    RCT pane, scale; pane.getPaneRect(false); scale.getScaleRect();
    int axisR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) axisR = scale.left - 2;
    if (pane.bottom - pane.top < 60 || axisR - pane.left < 120) { S.state = "pane too small: nothing drawn"; return; }
    // where the price area ends: the right-side columns (ColumnBoard.h, the same layout the profiles compute), the left Liquidity column
    {   const char* sy = getSymbol(); const char* cl = getChartLabel();
        const std::string sym = sy ? sy : "", chart = cl ? cl : "";
        S.chartKey = chart;
        S.peers.step(sym, getSecondsPerBar(), chart, steadyMs(), 0u);
        cb::In in; in.paneL = pane.left; in.paneT = pane.top; in.paneB = pane.bottom; in.axisR = axisR;
        in.levels = S.peers.on[cb::LEVELS]; in.levelsW = S.peers.w[cb::LEVELS]; in.delta = S.peers.on[cb::DELTA]; in.deltaW = S.peers.w[cb::DELTA];
        in.dealer = S.peers.on[cb::DEALER]; in.dealerW = S.peers.w[cb::DEALER];
        in.big = S.peers.on[cb::BIG]; in.bigW = S.peers.w[cb::BIG];   // (1.1.0) the BIG column
        const cb::Layout L = cb::layout(in);
        S.priceRight = (L.ok && L.margin > 0) ? L.priceRight : axisR;
        S.leftLimit = pane.left + (S.peers.on[cb::LIQ] ? S.peers.w[cb::LIQ] + 2 : 2);
        // the Tape Reader's banner (BannerBoard.h) - the one other indicator box this plugin can see: never covered
        bb::Rect r; long long ms = -1; const bool have = bb::peekBanner(sym, getSecondsPerBar(), chart, r, ms);
        S.trOn = S.trBeat.step(have, ms, steadyMs());
        if (S.trOn) { S.trBox.l = r.l; S.trBox.t = r.t; S.trBox.r = r.r; S.trBox.b = r.b; }
        // (1.16.0) the HOD/LOD table (lsHodLod 2.4.1+, BoxDock.h): present while its stamp keeps moving (same heartbeat as the banner)
        bdk::Box hb_; long long hms = -1; const bool hhave = bdk::peekHodLod(sym, getSecondsPerBar(), chart, hb_, hms);
        S.hlOn = S.hlBeat.step(hhave, hms, steadyMs()); if (S.hlOn) S.hlBox = hb_;
        {   bdk::Box tb_; long long tms = -1;   // (1.16.2) lsHodLod 2.4.4+: the table alone, published with the same stamp (an older lsHodLod: its box = the table)
            const bool tOk = S.hlOn && bdk::peekHodLodTable(sym, getSecondsPerBar(), chart, tb_, tms) && tms == hms && bdk::valid(tb_);
            S.hlTable = tOk ? tb_ : S.hlBox; S.hlTableOn = S.hlOn; }
    }
    RTARRAYI times(barDateTime); RTARRAY hi(barHigh), lo(barLow), op(barOpen), cl(barClose);
    if (times.count < count || hi.count < count || lo.count < count || op.count < count || cl.count < count) return;
    int vs = 0, ve = -1;
    if (getVisibleBars(&vs, &ve) != RTX_OK || vs < 0 || ve < vs) { vs = std::max(0L, count - 200); ve = (int)count - 1; }
    ve = std::min(ve, (int)count - 1);
    const int from = std::max(0, vs - LEAD);
    const bba::Settings set = S.set;
    // (1.2.0, Rassul approved mockup-location-first-v1 08:22 CT) LOCATION FIRST: bold numbers and circles at a high / low / pivot /
    // key or MenthorQ level are full strength; the same marks mid-range are dimmed (never hidden, never moved) - ZoneNear.h
    zn::Ctx Z; { const int zn0 = std::max(0, from - 600);
        for (int i = 0; i <= ve; ++i) { Z.hi.push_back(i < zn0 ? 0.0 : (double)hi[i]); Z.lo.push_back(i < zn0 ? 0.0 : (double)lo[i]); Z.sess.push_back(i < zn0 ? -1 : sessionOf((long long)times[i])); }
        Z.band = zn::bandFrom(Z.hi, Z.lo, std::max(0, (int)count - 2 <= ve ? (int)count - 2 : ve));
        const char* sy = getSymbol(); Z.levels = zn::levels(flexDir(), S.market, sy ? sy : "", steadyMs()); }
    std::vector<double> candPx, candVol;
    PassCtx X; passCtx(S, hi, lo, (int)count, X);   // (1.14.0) the candle's own swing (all bars: may repaint until confirmed), breakouts, bands
    FONT fp; fp.id = HELVETICA; fp.size = (short)set.cat[0].font; fp.style = PLAIN;
    FONT fb = fp; fb.style = BOLD;                                    // (1.9.0) every number the same size (his colours mark the bigger prints)
    int lead = 0, asc = 0, desc = 0; setFont(fp); getFontMetrics(&lead, &asc, &desc);
    const int th = std::max(fp.size + 2, asc + desc); (void)th;
    int leadB = 0, ascB = 0, descB = 0; setFont(fb); getFontMetrics(&leadB, &ascB, &descB); setFont(fp);
    const int thB = std::max(fp.size + 5, ascB + descB); (void)thB;
    bba::Box area; area.l = S.leftLimit; area.t = pane.top + 20; area.r = std::min(S.priceRight, axisR) - 1; area.b = pane.bottom - 1;   // the IRT title row stays clear
    // (1.6.1) an area that came out empty or tiny (columns not read yet, a restart) would drop EVERY number: fall back to the whole pane
    if (area.r - area.l < 120) { area.l = pane.left + 2; area.r = axisR - 1; ++S.areaFallback; }
    // (1.26.0) no OB / BO boxes any more (Rassul 18:01 CT 2026-10-22): the OB is a print + a letter (below, after the BO prints). The 1.25.1
    // LOG ONLY gate (LRA-OrderBlock-<MKT>.csv SHOW, approved 17:39 for the BOXES) is read for the status line only: his later 18:01 request
    // asks for the OB / BO prints + letters; the hover says "?" (not proven) and the nightly keeps scoring them.
    S.obShowP = mp::mode("OrderBlock", S.market, "SHOW", mp::LOG);
    std::vector<bba::Box> candles; std::vector<bba::Cand> cands; std::vector<const bba::Facts*> src; std::vector<std::string> texts; std::vector<PickInfo> picks;
    for (int i = from; i <= ve; ++i) {
        PNT a; a.set(i, hi[i], kBarLeft); PNT b; b.set(i, lo[i], kBarRight);
        bba::Box cb_; cb_.l = std::min(a.h, b.h); cb_.r = std::max(a.h, b.h); cb_.t = std::min(a.v, b.v); cb_.b = std::max(a.v, b.v);
        {   // (1.4.1, Rassul 13:18 "check if there are prints that are missing": 82 of 112 numbers were dropped for no room) a candle blocks
            // only what is drawn: its BODY (open..close) and its 1-2 px WICK (high..low at the centre), not the whole high-low box, so a
            // neighbour's number may sit beside the wick above / below its body
            PNT po; po.set(i, op[i], kBarLeft); PNT pc; pc.set(i, cl[i], kBarRight);
            bba::Box body; body.l = cb_.l; body.r = cb_.r; body.t = std::min(po.v, pc.v); body.b = std::max(po.v, pc.v);
            if (body.b - body.t < 1) body.b = body.t + 1;
            const int cx = (cb_.l + cb_.r) / 2;
            bba::Box wick; wick.l = cx - 1; wick.r = cx + 1; wick.t = cb_.t; wick.b = cb_.b;
            candles.push_back(body); candles.push_back(wick);
        }
        const long long t = (long long)times[i];
        const bba::Facts* F = nullptr;
        if (i < (int)count - 1) { auto it = S.fin.find(t); if (it != S.fin.end()) F = &it->second; }
        else if (S.haveForming && S.forming.t == t) F = &S.forming;
        if (!F || !F->valid) continue;
        // (1.5.0, Rassul 16:11-16:13 "the faded prints next to the candle have to be at least 1 SD ... use the distribution that we are
        // currently using") with the Liquidity Profile's big-lot sizes: nothing under 1SD, 1SD = faded, 2SD = bold, 3SD = bold + outline;
        // absorption circle from 1SD. Still learning there (no file / under 30 big orders): the 1.4 bold-ratio rule.
        // (1.5.0, Rassul 16:21 "for the candle just use the average large") on the candles the floor is Big mu: under it nothing; Big mu =
        // faded, Bigger (1SD) = bold, Huge (3SD+) = bold + white outline; absorption circle from Big mu
        // (1.9.0, Rassul 2026-10-17 13:14 / 13:27 "use your big lot methodology") a print shows when it reaches the chosen Big Lot tier of
        // its phase (S20 / S10 / S5 by rank); every number the same size, coloured by its tier and side; still learning (no tiers yet) = nothing
        // (1.11.0) the candle's session clock: its phase (day / night), the time filter, its levels
        long days = 0; int endSec = 0; if (!parts(t, days, endSec)) continue;
        const svl::Cls cls = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
        if (timeSkipped(set, cls, endSec, S.openMin, S.closeMin)) { ++S.timeHidden; continue; }   // (1.13.0 RTH only; Rassul 14:50 skip first / last x min)
        // (1.18.0, Rassul 2026-10-20 21:26 "show a print if it is the biggest print for that side in the last 10 bars", 21:30 "for now on
        // a print will be displayed every time it is the biggest print in the last 10 bars on any side") ONE rule, both sides on their own:
        // the candle's biggest cell at the bid (ask) is bigger than the biggest cell at the bid (ask) of EACH of the 10 candles before it.
        // No Big Lot tiers, pivot, level or trend filter; the first / last 15 minutes of every session stay skipped (21:31-21:32, all markets).
        for (int sd = -1; sd <= 1; sd += 2) {
            const bba::Pick& p = sd < 0 ? F->bid : F->ask;
            if (!p.ok) continue;
            const int back = backCount(S, times, i, sd, p.vol);
            if (back < S.topN) continue;                                    // (1.19.0) "Biggest in" (default 10)
            const double px = (double)p.key * S.tick;
            PickInfo pi; pi.cat = 0; pi.conf = true;
            const int got = 0;
            const std::string tx = std::to_string(back);                     // (1.19.0) the candles back, 10 - 50 (no lots / divider)
            FONT fc = fb; fc.size = (short)set.cat[got].font; setFont(fc);
            int lc = 0, ac = 0, dc = 0; getFontMetrics(&lc, &ac, &dc);
            PNT pt; pt.set(i, (float)px, kBarCenter);
            bba::Cand c; c.bar = i; c.side = sd; c.x = (cb_.l + cb_.r) / 2; c.y = pt.v; c.w = getTextWidth(tx.c_str(), -1) + 2; c.h = std::max(set.cat[got].font + 5, ac + dc);
            c.bold = true; c.huge = false; c.tier = got; c.circle = S.set.circle && (sabMask(S, t, *F) & (sd < 0 ? 1 : 2)) != 0; c.first = F->closed;   // (1.22.0) only at a shared A of its side (was the 1.20.0 order-book test)
            ++S.catN4[got];
            cands.push_back(c); src.push_back(F); texts.push_back(tx); candPx.push_back(px); candVol.push_back((double)p.vol); picks.push_back(pi);
        }
    }
    // (1.21.0) BREAKOUT / BREAKDOWN (decided in boPass, never here): the breakout print joins the numbers ON its candle at its price,
    // its LOTS (as the approved mockup) white in a box outlined green (BO) / red (BD); when that cell is already a Big Bid/Ask print
    // (same candle, same side) it is drawn ONCE - that box shows the breakout lots in the BO / BD colour, plus the letter. A breakout with no qualifying print gets the letter alone. No pivot line.
    std::vector<int> boDir(cands.size(), 0); std::vector<char> boOwn(cands.size(), 0);
    struct BoLetter { int bar = 0, dir = 0; int kind = 0; long long id = 0; };   // (1.26.0) kind 0 = BO, 1 = OB (id = its record)
    std::vector<BoLetter> letters;
    std::map<long long, const bba::BoDec*> boMark;                         // (1.23.0) the fired breaks by their MARK candle (t; keyed by t0 in boDec)
    if (S.boOn) for (const auto& kv : S.boDec) if (kv.second.fired && !boMark.count(kv.second.t)) boMark[kv.second.t] = &kv.second;
    if (S.boOn) for (int i = std::max(from, vs); i <= ve && i < (int)count - 1; ++i) {
        const long long t = (long long)times[i];
        const auto it = boMark.find(t); if (it == boMark.end()) continue;
        const bba::BoDec& D = *it->second; const int sd = D.dir > 0 ? +1 : -1;
        letters.push_back(BoLetter{ i, D.dir, 0, 0 });
        if (!D.print || D.lots <= 0) continue;
        size_t hit = cands.size();
        for (size_t k = 0; k < cands.size(); ++k) if (cands[k].bar == i && cands[k].side == sd) { hit = k; break; }
        if (hit < cands.size()) {                                            // already a Big Bid/Ask print: drawn ONCE, as the breakout print
            boDir[hit] = D.dir; boOwn[hit] = 2; texts[hit] = bba::lots(D.lots); candVol[hit] = (double)D.lots; continue; }
        const auto fi = S.fin.find(t); if (fi == S.fin.end()) continue;
        PNT a; a.set(i, hi[i], kBarLeft); PNT b; b.set(i, lo[i], kBarRight);
        const std::string tx = bba::lots(D.lots);
        FONT fc = fb; fc.size = (short)set.cat[0].font; setFont(fc);
        int lc = 0, ac = 0, dc = 0; getFontMetrics(&lc, &ac, &dc);
        const double px = (double)D.key * S.tick;
        PNT pt; pt.set(i, (float)px, kBarCenter);
        bba::Cand c; c.bar = i; c.side = sd; c.x = (std::min(a.h, b.h) + std::max(a.h, b.h)) / 2; c.y = pt.v; c.w = getTextWidth(tx.c_str(), -1) + 2; c.h = std::max(set.cat[0].font + 5, ac + dc);
        c.bold = true; c.huge = false; c.tier = 0; c.circle = false; c.first = true;
        cands.push_back(c); src.push_back(&fi->second); texts.push_back(tx); candPx.push_back(px); candVol.push_back((double)D.lots); picks.push_back(PickInfo());
        boDir.push_back(D.dir); boOwn.push_back(1);
    }
    // (1.26.0, Rassul 18:01 CT 2026-10-22) the OB PRINT of a strong wave (decided in obPass, never here): drawn like a print - its LOTS
    // in the side's box (red at the bid, green at the ask) ON its candle at its price; when that cell is already a drawn Big Bid/Ask print
    // (same candle, side and price) it is drawn ONCE (that box shows the OB lots). The letter "OB" goes BELOW the candle of an up-wave,
    // ABOVE for a down-wave (with the BO letters below). 3-min charts only; the "3+ ATR waves" checkbox hides the non-Rly ones.
    std::vector<char> obOwn(cands.size(), 0);
    if (S.obOn && S.spb == 180) for (const auto& kv : S.obRec) {
        const obk::Box& b = kv.second; if (!b.has()) continue;
        if (b.cls == obk::CLS_ATR && !S.obAny) continue;
        const obk::Print& q = b.prints.front();
        const int i = indexOf(times, count, q.t); if (i < std::max(from, vs) || i > ve || i >= (int)count - 1) continue;
        {   long dd = 0; int es = 0; if (!parts(q.t, dd, es)) continue;           // the skip first / last minutes apply as to every print
            const svl::Cls qc = svl::classify(dd, es, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
            if (timeSkipped(set, qc, es, S.openMin, S.closeMin)) continue; }
        letters.push_back(BoLetter{ i, b.dir, 1, kv.first });
        long long qk = 0; if (!keyOf(q.px, S.tick, qk)) continue;
        size_t hit = cands.size();
        for (size_t k = 0; k < cands.size(); ++k) { long long ck = 0; if (cands[k].bar == i && cands[k].side == q.side && keyOf(candPx[k], S.tick, ck) && ck == qk) { hit = k; break; } }
        if (hit < cands.size()) { if (!boOwn[hit]) { obOwn[hit] = 2; texts[hit] = bba::lots(q.lots); candVol[hit] = (double)q.lots; } continue; }
        const auto fi = S.fin.find(q.t); if (fi == S.fin.end()) continue;
        PNT a; a.set(i, hi[i], kBarLeft); PNT bb; bb.set(i, lo[i], kBarRight);
        const std::string tx = bba::lots(q.lots);
        FONT fc = fb; fc.size = (short)set.cat[0].font; setFont(fc);
        int lc = 0, ac = 0, dc = 0; getFontMetrics(&lc, &ac, &dc);
        const double px = (double)qk * S.tick;
        PNT pt; pt.set(i, (float)px, kBarCenter);
        bba::Cand c; c.bar = i; c.side = q.side; c.x = (std::min(a.h, bb.h) + std::max(a.h, bb.h)) / 2; c.y = pt.v; c.w = getTextWidth(tx.c_str(), -1) + 2; c.h = std::max(set.cat[0].font + 5, ac + dc);
        c.bold = true; c.huge = false; c.tier = 0; c.circle = false; c.first = true;
        cands.push_back(c); src.push_back(&fi->second); texts.push_back(tx); candPx.push_back(px); candVol.push_back((double)q.lots); picks.push_back(PickInfo());
        boDir.push_back(0); boOwn.push_back(0); obOwn.push_back(1);
    }
    // (1.15.0) the SignalBoard B events are no longer posted from the laid-out (visible) candles: boardPass posts them from the record
    // (1.6.0, Rassul 18:26-18:40 mockup footprint-white-box-circle-v1 row B) ON THE CANDLE: each number is drawn ON its own candle at the
    // price row it traded (never nudged, so it can only belong to that candle - a print is always inside the candle's high-low), WHITE with
    // a 1 px black edge; a big print = a rounded box, an absorption print = a circle, both in the SIDE colour (red = sold at the bid,
    // green = bought at the ask: the side that got trapped); 3SD+ = a bigger number, box and circle. Bid and ask on the same row of one
    // candle sit side by side. Away from a zone the box / circle / number are dimmed (location first, unchanged).
    FONT fH = fb; fH.size = (short)(fp.size + 5);                   // 3SD+: bigger
    int leadH = 0, ascH = 0, descH = 0; setFont(fH); getFontMetrics(&leadH, &ascH, &descH); setFont(fp);
    S.lay = bba::LayoutStats();
    std::vector<bba::Box> boxes(cands.size());
    for (size_t k = 0; k < cands.size(); ++k) {
        const bba::Cand& c = cands[k];
        FONT fc = fb; fc.size = (short)set.cat[c.tier].font; setFont(fc);    // (1.11.0) the category's own font size
        const int tw = getTextWidth(texts[k].c_str(), -1), thx = c.h;
        const bool rg = c.circle && set.circle; const int padX = rg ? 5 : set.box ? 3 : 1, padY = rg ? 2 : set.box ? 1 : 0;
        bba::Box b; b.l = c.x - tw / 2 - padX; b.r = b.l + tw + 2 * padX; b.t = c.y - thx / 2 - padY; b.b = b.t + thx + 2 * padY;
        boxes[k] = b;
    }
    for (size_t k = 0; k < cands.size(); ++k)                           // bid and ask on the same row of one candle: side by side
        for (size_t j = k + 1; j < cands.size(); ++j) {
            if (cands[j].bar != cands[k].bar || cands[j].side == cands[k].side) continue;
            if (boxes[j].t > boxes[k].b || boxes[k].t > boxes[j].b) continue;
            bba::Box& L = cands[k].side < 0 ? boxes[k] : boxes[j]; bba::Box& Rb = cands[k].side < 0 ? boxes[j] : boxes[k];
            const int cx = cands[k].x, wl = L.r - L.l, wr = Rb.r - Rb.l;
            L.r = cx - 1; L.l = L.r - wl; Rb.l = cx + 1; Rb.r = Rb.l + wr;
        }
    // (1.6.0) two numbers on NEIGHBOURING candles can touch when zoomed out (a number is wider than a candle): the absorption print
    // wins, then the bigger print; the other is not drawn (counted on the status file) - nothing is ever moved off its candle
    std::vector<char> lose(cands.size(), 0);
    {   std::vector<size_t> ord(cands.size()); for (size_t k = 0; k < ord.size(); ++k) ord[k] = k;
        std::stable_sort(ord.begin(), ord.end(), [&](size_t x, size_t y) {
            if (cands[x].circle != cands[y].circle) return cands[x].circle > cands[y].circle;
            if ((boDir[x] != 0) != (boDir[y] != 0)) return boDir[x] != 0;   // (1.21.0) a breakout print next
            if ((obOwn[x] != 0) != (obOwn[y] != 0)) return obOwn[x] != 0;   // (1.26.0) then an OB print
            return candVol[x] > candVol[y]; });
        std::vector<size_t> kept;
        for (size_t q : ord) {
            bool hit = false;
            for (size_t w : kept) { if (cands[w].bar == cands[q].bar) continue;
                const bba::Box& A = boxes[q]; const bba::Box& B = boxes[w];
                if (!(A.r < B.l || A.l > B.r || A.b < B.t || A.t > B.b)) { hit = true; break; } }
            if (hit) lose[q] = 1; else kept.push_back(q);
        }
    }
    std::vector<bba::Box> ko;   // (1.17.0) the HOD/LOD table + strip (as published) and the Big Lot box (as last drawn): keep-out
    if (S.hlOn) { bba::Box x; x.l = S.hlBox.l - 2; x.t = S.hlBox.t - 2; x.r = S.hlBox.r + 2; x.b = S.hlBox.b + 2; ko.push_back(x); }
    if (S.blOn && set.statsChart) { bba::Box x; x.l = S.blBox.l - 2; x.t = S.blBox.t - 2; x.r = S.blBox.r + 2; x.b = S.blBox.b + 2; ko.push_back(x); }
    std::vector<bba::Box> drawnBox; std::vector<int> drawnBar;          // (1.21.0) what was drawn: the letters never cover it
    for (size_t k = 0; k < cands.size(); ++k) {
        const bba::Cand& c = cands[k]; bba::Box f = boxes[k];
        if (c.bar < vs) continue;                                       // left of the view
        if (lose[k]) { ++S.droppedInView; ++S.lay.crowded; if (c.circle) ++S.circlesHidden; continue; }
        // (1.6.1) a number that pokes out of the area is SLID back in (up to 40 px, it stays on its candle's price row within a few ticks);
        // only a number far outside, wider than the area, or under the Tape Reader banner is dropped - and the reason is counted
        bool clash = false;
        { int dx = 0, dy = 0;
          if (f.r - f.l > area.r - area.l || f.b - f.t > area.b - area.t) { clash = true; ++S.clBig; }
          else {
              if (f.l < area.l) dx = area.l - f.l; else if (f.r > area.r) dx = area.r - f.r;
              if (f.t < area.t) dy = area.t - f.t; else if (f.b > area.b) dy = area.b - f.b;
              if (std::abs(dx) > 40) { clash = true; if (dx > 0) ++S.clL; else ++S.clR; }
              else if (std::abs(dy) > 40) { clash = true; if (dy > 0) ++S.clT; else ++S.clB; }
              else if (dx || dy) { f.l += dx; f.r += dx; f.t += dy; f.b += dy; ++S.shifted; }
          } }
        if (!clash && S.trOn && !(f.r < S.trBox.l || f.l > S.trBox.r || f.b < S.trBox.t || f.t > S.trBox.b)) { clash = true; ++S.clTr; }   // the Tape Reader banner
        if (!clash && !ko.empty()) { const int kr = bba::keepOut(f, ko, area); if (kr < 0) { clash = true; ++S.clHl; } else if (kr > 0) ++S.slidHl; }   // (1.17.0) never through the HOD/LOD table / strip or the Big Lot box
        if (clash) { ++S.droppedInView; ++S.lay.dropped; if (c.circle) ++S.circlesHidden; continue; }
        ++S.lay.shown;
        const bool zone = zn::atZone(Z, c.bar, candPx[k]);
        // (1.9.0) the colour = his pick for the print's tier and side (default red at the bid, green at the ask); dimmed away from a zone
        // only when "Dim away from a level" is on; a box and / or the absorption circle only when switched on
        const bool dimIt = set.dim && !zone;
        const bba::Cat& cc = set.cat[c.tier < 0 || c.tier > 3 ? 0 : c.tier]; const COLOR base = (COLOR)(c.side < 0 ? cc.bid : cc.ask);
        const COLOR ink = dimIt ? getBlendedColor(base, 0x00000000, 45.0f) : base;
        if (zone) ++S.zoneFull; else ++S.zoneDim;
        const bool ring = c.circle && set.circle, box = set.box && !ring;
        if (ring || box) {   // the black edge first (so it reads on a candle of its own colour), then the colour
            const COLOR boxC = boOwn[k] ? (COLOR)(boDir[k] > 0 ? S.boCol : S.bdCol) : (COLOR)(c.side < 0 ? set.boxBid : set.boxAsk);   // (1.14.0) the box has its own colour by side; (1.21.0) a breakout print: BO / BD colour
            const COLOR winC = (COLOR)(c.side < 0 ? set.boxAsk : set.boxBid);   // (1.20.0) the WINNER's colour: an absorbed bid print = buyers won (green), an absorbed ask print = sellers won (red)
            const COLOR lc = ring ? (dimIt ? getBlendedColor(winC, 0x00000000, 45.0f) : winC) : (dimIt ? getBlendedColor(boxC, 0x00000000, 45.0f) : boxC);
            CBRUSH nb(0x00000000, PAT_HOLLOW); nb.set();
            RCT e; e.set(coord(f.l), coord(f.t), coord(f.r), coord(f.b));
            setPen(0x00000000, 3, P_SOLID);
            if (ring) e.drawRounded(f.b - f.t, f.b - f.t); else e.drawRounded(4, 4);
            setPen(lc, 1, P_SOLID); CBRUSH br(lc, PAT_HOLLOW); br.set();
            if (ring) { e.drawRounded(f.b - f.t, f.b - f.t); ++S.circles; } else e.drawRounded(4, 4);
        }
        FONT fc = fb; fc.size = (short)set.cat[c.tier].font; setFont(fc);
        int lc = 0, ac = 0, dc = 0; getFontMetrics(&lc, &ac, &dc);
        const int lift = std::max(1, (lc + dc + 1) / 2);   // digits in the vertical middle (1.4.1)
        const int tl = f.l, tt = f.t - lift, tr = f.r, tb = f.b - lift;
        setTextColor(0x00000000);                                        // 1 px black edge round the digits
        for (int d = 0; d < 4; ++d) { const int dx = d == 0 ? -1 : d == 1 ? 1 : 0, dy = d == 2 ? -1 : d == 3 ? 1 : 0;
            RCT r; r.set(coord(tl + dx), coord(tt + dy), coord(tr + dx), coord(tb + dy)); r.drawText(texts[k].c_str(), true, false); }
        setTextColor(ink);
        { RCT r; r.set(coord(tl), coord(tt), coord(tr), coord(tb)); r.drawText(texts[k].c_str(), true, false); }
        HoverT H; H.box = f; H.t = src[k]->t; H.side = c.side; H.pi = picks[k]; H.lots = (long long)candVol[k]; H.px = candPx[k]; S.targets.push_back(H);
        drawnBox.push_back(f); drawnBar.push_back(c.bar);
        if (boOwn[k] == 1) ++S.boPrints; else if (boOwn[k] == 2) ++S.boShared;
        if (obOwn[k] == 1) ++S.obPrints; else if (obOwn[k] == 2) ++S.obShared;   // (1.26.0)
    }
    // (1.21.0) the BO / BD letters: "BO" green ABOVE the candle (and above its drawn prints), "BD" red BELOW (the footprint-signal
    // convention); never over a drawn print or another letter (moved further out), the Tape Reader banner, or the HOD/LOD table / Big
    // Lot box (keep-out: not drawn); outside the price area = not drawn. Counted on the status file.
    {   FONT lf = fb; lf.size = (short)set.cat[0].font; setFont(lf);
        int ll = 0, la = 0, ld = 0; getFontMetrics(&ll, &la, &ld);
        const int lh = std::max(set.cat[0].font + 5, la + ld);
        std::vector<bba::Box> placedL;
        for (const BoLetter& L : letters) {
            // (1.26.0) "BO" for both sides: green ABOVE a breakout / red BELOW a breakdown; "OB" BELOW an up-wave's candle / ABOVE a down-wave's
            const char* tx = L.kind == 1 ? "OB" : "BO";
            const bool above = L.kind == 1 ? L.dir < 0 : L.dir > 0;
            const int w = getTextWidth(tx, -1) + 2;
            PNT a; a.set(L.bar, hi[L.bar], kBarLeft); PNT b; b.set(L.bar, lo[L.bar], kBarRight);
            const int cx = (std::min(a.h, b.h) + std::max(a.h, b.h)) / 2, top = std::min(a.v, b.v), bot = std::max(a.v, b.v);
            int edge = above ? top : bot;
            for (size_t q = 0; q < drawnBox.size(); ++q) if (drawnBar[q] == L.bar) edge = above ? std::min(edge, drawnBox[q].t) : std::max(edge, drawnBox[q].b);
            bba::Box f; f.l = cx - w / 2; f.r = f.l + w;
            if (above) { f.b = edge - 2; f.t = f.b - lh; } else { f.t = edge + 2; f.b = f.t + lh; }
            bool ok = false;
            for (int tries = 0; tries < 4 && !ok; ++tries) {
                const bba::Box* clash = nullptr;
                for (const bba::Box& o : drawnBox) if (bba::hits(f, o)) { clash = &o; break; }
                if (!clash) for (const bba::Box& o : placedL) if (bba::hits(f, o)) { clash = &o; break; }
                if (!clash) { ok = true; break; }
                if (above) { f.b = clash->t - 2; f.t = f.b - lh; } else { f.t = clash->b + 2; f.b = f.t + lh; }   // further out
            }
            if (!ok || !bba::inside(f, area) || (S.trOn && bba::hits(f, S.trBox))) { if (L.kind == 1) ++S.obNoLabel; else ++S.boNoRoom; continue; }
            bool kept = false; for (const bba::Box& k2 : ko) if (k2.r > k2.l && k2.b > k2.t && bba::hits(f, k2)) kept = true;
            if (kept) { if (L.kind == 1) ++S.obNoLabel; else ++S.boKept; continue; }
            placedL.push_back(f); if (L.kind == 1) { ++S.obLabels; BBState::ObHov H; H.box = f; H.id = L.id; S.obTargets.push_back(H); } else ++S.boLetters;
            const COLOR col = L.kind == 1 ? (COLOR)(L.dir > 0 ? S.obUpCol : S.obDnCol) : (COLOR)(L.dir > 0 ? S.boCol : S.bdCol);
            const int lift = std::max(1, (ll + ld + 1) / 2);
            setTextColor(0x00000000);
            for (int d = 0; d < 4; ++d) { const int dx = d == 0 ? -1 : d == 1 ? 1 : 0, dy = d == 2 ? -1 : d == 3 ? 1 : 0;
                RCT r; r.set(coord(f.l + dx), coord(f.t - lift + dy), coord(f.r + dx), coord(f.b - lift + dy)); r.drawText(tx, true, false); }
            setTextColor(col);
            { RCT r; r.set(coord(f.l), coord(f.t - lift), coord(f.r), coord(f.b - lift)); r.drawText(tx, true, false); }
        }
        // (1.24.0) RLY on the wave's end bar: green ABOVE the high (up) / red BELOW the low (down), outside the prints and the BO / BD
        // letters of that bar (moved further out), never over the banner, the HOD/LOD table / Big Lot box, or outside the price area
        if (S.rlyOn && S.spb == 180 && !S.rlyRec.empty()) {
            std::map<long long, int> idx; for (int i = vs; i <= ve && i < (int)count - 1; ++i) idx[(long long)times[i]] = i;
            for (const auto& kv : S.rlyRec) {
                const rly::Rec& R = kv.second; if (!R.fired) continue;
                const auto it = idx.find(R.te); if (it == idx.end()) continue;
                const int bar = it->second; const char* tx = "Rly";
                const int w = getTextWidth(tx, -1) + 2;
                PNT a; a.set(bar, hi[bar], kBarLeft); PNT b; b.set(bar, lo[bar], kBarRight);
                const int cx = (std::min(a.h, b.h) + std::max(a.h, b.h)) / 2, top = std::min(a.v, b.v), bot = std::max(a.v, b.v);
                int edge = R.dir > 0 ? top : bot;
                for (size_t q = 0; q < drawnBox.size(); ++q) if (drawnBar[q] == bar) edge = R.dir > 0 ? std::min(edge, drawnBox[q].t) : std::max(edge, drawnBox[q].b);
                bba::Box f; f.l = cx - w / 2; f.r = f.l + w;
                if (R.dir > 0) { f.b = edge - 2; f.t = f.b - lh; } else { f.t = edge + 2; f.b = f.t + lh; }
                bool ok = false;
                for (int tries = 0; tries < 5 && !ok; ++tries) {
                    const bba::Box* clash = nullptr;
                    for (const bba::Box& o : drawnBox) if (bba::hits(f, o)) { clash = &o; break; }
                    if (!clash) for (const bba::Box& o : placedL) if (bba::hits(f, o)) { clash = &o; break; }
                    if (!clash) { ok = true; break; }
                    if (R.dir > 0) { f.b = clash->t - 2; f.t = f.b - lh; } else { f.t = clash->b + 2; f.b = f.t + lh; }
                }
                if (!ok || !bba::inside(f, area) || (S.trOn && bba::hits(f, S.trBox))) { ++S.rlyNoRoom; continue; }
                bool kept = false; for (const bba::Box& k2 : ko) if (k2.r > k2.l && k2.b > k2.t && bba::hits(f, k2)) kept = true;
                if (kept) { ++S.rlyNoRoom; continue; }
                placedL.push_back(f); ++S.rlyLetters;
                const int lift = std::max(1, (ll + ld + 1) / 2);
                setTextColor(0x00000000);
                for (int d = 0; d < 4; ++d) { const int dx = d == 0 ? -1 : d == 1 ? 1 : 0, dy = d == 2 ? -1 : d == 3 ? 1 : 0;
                    RCT r; r.set(coord(f.l + dx), coord(f.t - lift + dy), coord(f.r + dx), coord(f.b - lift + dy)); r.drawText(tx, true, false); }
                setTextColor((COLOR)(R.dir > 0 ? S.rlyUpCol : S.rlyDnCol));
                { RCT r; r.set(coord(f.l), coord(f.t - lift), coord(f.r), coord(f.b - lift)); r.drawText(tx, true, false); }
                BBState::RlyHov H; H.box = f; H.key = kv.first; S.rlyTargets.push_back(H);
            }
        }
    }
    // (1.16.0, Rassul 15:03 CT) the compact Big Lot box: BL5 / BL10 / BL20 = the rolling average of the CURRENT session type, docked to
    // the HOD/LOD table when it is on this chart (BoxDock.h), else his 9-spot Position; only up to the LAST BAR, never over another box
    S.blOn = false;
    {   long dd = 0; int ee = 0; S.blNightNow = parts((long long)times[(int)count - 1], dd, ee) ? !dayOf(S, ee / 60) : false; }
    if (set.statsChart) {
        const bba::TypeAvg& A = S.blNightNow ? S.blNight : S.blDay;
        const std::vector<std::pair<std::string, std::string>> rows = bba::boxRows(A);
        FONT lf; lf.id = HELVETICA; lf.size = 10; lf.style = BOLD; FONT vf = lf; vf.style = PLAIN;
        setFont(lf); int lw = getTextWidth("BL20", -1); for (const auto& r : rows) lw = std::max(lw, getTextWidth(r.first.c_str(), -1));
        setFont(vf); int vw = getTextWidth("00,000", -1); for (const auto& r : rows) vw = std::max(vw, getTextWidth(r.second.c_str(), -1));
        int la = 0, aa = 0, da = 0; getFontMetrics(&la, &aa, &da);
        const int lh0 = std::max(15, aa + da + 2), padX = 7, padY = 4;
        const int w = padX + lw + 14 + vw + padX, h0 = padY + 3 * lh0 + padY;         // fixed width (fits 5-digit sizes); the smallest height
        int lastX = -1; { PNT pe; if (pe.set((int)count - 1, cl[(int)count - 1], kBarRight) == RTX_OK && pe.h > 0) lastX = pe.h; }
        bdk::Box ar = bdk::mk(area.l, area.t, std::min(area.r, lastX > area.l ? lastX : area.r), area.b);   // his rule: up to where price ends
        std::vector<bdk::Box> avoid; if (S.trOn) avoid.push_back(bdk::mk(S.trBox.l, S.trBox.t, S.trBox.r, S.trBox.b));
        // (1.16.2, Rassul 10-18 "the BLS box should be the same height as the hod lod box") beside the TABLE, its top and bottom (not the
        // Gex S/R strip above it), the rows spread evenly over that height; no room beside it = the 1.16 placement at the smallest height
        int h = h0; const bdk::Result R = bdk::placeMatched(S.hlTableOn, S.hlBox, S.hlTable, w, h0, ar, set.statsPos, avoid, &h);
        const int lh = std::max(lh0, (h - 2 * padY) / 3);
        S.blWhy = R.why + (S.hlOn ? ",hodlod table " + std::to_string(S.hlBox.l) + "," + std::to_string(S.hlBox.t) + "," + std::to_string(S.hlBox.r) + "," + std::to_string(S.hlBox.b) : std::string(",no hodlod table"))
                + ",type " + (S.blNightNow ? "overnight" : "RTH") + ",sessions " + std::to_string(A.nSess);
        if (R.mode != bdk::NONE) {
            const bdk::Box& B = R.b; S.blBox = B; S.blOn = true;
            S.blWhy += ",box " + std::to_string(B.l) + "," + std::to_string(B.t) + "," + std::to_string(B.r) + "," + std::to_string(B.b);
            RCT bg; bg.set(coord(B.l), coord(B.t), coord(B.r), coord(B.b)); bg.draw(1, C_BLEDGE, C_BLBG, DRAW_OPAQUE, PAT_SOLID);
            setTextColor(C_INK);
            for (size_t k = 0; k < rows.size(); ++k) {
                const int y0 = B.t + padY + (int)k * lh + (B.b - B.t - 2 * padY - 3 * lh) / 2;   // (1.16.2) evenly over the box
                setFont(lf); { RCT tr; tr.set(coord(B.l + padX), coord(y0), coord(B.l + padX + lw + 4), coord(y0 + lh)); tr.drawText(rows[k].first.c_str(), false, false); }
                setFont(vf); { const int tw = getTextWidth(rows[k].second.c_str(), -1); RCT tr; tr.set(coord(B.r - padX - tw), coord(y0), coord(B.r - padX + 2), coord(y0 + lh)); tr.drawText(rows[k].second.c_str(), false, false); }
            }
        }
    } else S.blWhy = "off (Big Lot box unchecked in Settings)";
    // (1.24.0) the Rly hover (the only hover on this indicator): question / what / why / how to use / example with the wave numbers
    if (S.msHas && S.msWnd != 0 && !S.rlyTargets.empty() && hb::hbCursor() == S.msWnd) {
        const BBState::RlyHov* T = nullptr;
        for (const auto& h : S.rlyTargets) if (S.msH >= h.box.l - 2 && S.msH <= h.box.r + 2 && S.msV >= h.box.t - 2 && S.msV <= h.box.b + 2) { T = &h; break; }
        const auto it = T ? S.rlyRec.find(T->key) : S.rlyRec.end();
        if (it != S.rlyRec.end()) {
            const rly::Rec& R = it->second; S.hovKey = "RLY" + std::to_string(T->key);
            long d = 0; int e = 0; std::string clk = "?"; if (parts(R.te, d, e)) { const int st = e - (S.spb > 0 ? S.spb : 180); clk = bba::clock12(st / 3600, st / 60 % 60); }
            auto px = [&](double v) { return bba::price((long long)std::llround(v / S.tick), S.tick); };
            const std::vector<std::string> L = rly::hover(R, clk, px(R.from), px(R.to), px(R.size));
            FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
            int w = 0; for (const auto& s2 : L) w = std::max(w, getTextWidth(s2.c_str(), -1));
            w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
            int l = S.msH + 14, t = S.msV + 14;
            if (l + w > axisR - 2) l = S.msH - 14 - w;
            if (l < pane.left + 2) l = pane.left + 2;
            if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
            if (t < pane.top + 2) t = pane.top + 2;
            RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
            for (size_t i = 0; i < L.size(); ++i) {
                setTextColor(i == 0 ? (COLOR)(R.dir > 0 ? S.rlyUpCol : S.rlyDnCol) : C_INK);
                RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false);
            }
            return;
        }
    }
    // (1.26.0) the OB hover: on the OB letter (the smallest target under the cursor), after the Rly hover
    if (S.msHas && S.msWnd != 0 && !S.obTargets.empty() && hb::hbCursor() == S.msWnd) {
        const BBState::ObHov* T = nullptr; long long bestA = -1;
        for (const auto& h : S.obTargets) if (S.msH >= h.box.l && S.msH <= h.box.r && S.msV >= h.box.t - 1 && S.msV <= h.box.b + 1) {
            const long long ar = (long long)(h.box.r - h.box.l) * (h.box.b - h.box.t); if (!T || ar < bestA) { T = &h; bestA = ar; } }
        const auto it = T ? S.obRec.find(T->id) : S.obRec.end();
        if (it != S.obRec.end()) {
            const obk::Box& b = it->second; S.hovKey = "OB" + std::to_string(T->id);
            static const obk::Track none; const auto tk = S.obTrk.find(T->id); const obk::Track& tr = tk != S.obTrk.end() ? tk->second : none;
            auto clk = [&](long long t) { long d = 0; int e = 0; if (!parts(t, d, e)) return std::string("?"); const int st = e - (S.spb > 0 ? S.spb : 180); return bba::clock12(st / 3600, st / 60 % 60); };
            auto px = [&](double v) { return bba::price((long long)std::llround(v / S.tick), S.tick); };
            const std::vector<std::string> L = obk::hover(b, tr, clk, px);
            FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
            int w = 0; for (const auto& s2 : L) w = std::max(w, getTextWidth(s2.c_str(), -1));
            w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
            int l = S.msH + 14, t = S.msV + 14;
            if (l + w > axisR - 2) l = S.msH - 14 - w;
            if (l < pane.left + 2) l = pane.left + 2;
            if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
            if (t < pane.top + 2) t = pane.top + 2;
            RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
            for (size_t i = 0; i < L.size(); ++i) {
                setTextColor(i == 0 ? (COLOR)(b.dir > 0 ? S.obUpCol : S.obDnCol) : C_INK);
                RCT tr2; tr2.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr2.drawText(L[i].c_str(), false, false);
            }
            return;
        }
    }
    // the hover: only while the cursor is over this chart's window and over a number
    if (!BBA_HOVERS || !S.msHas || S.msWnd == 0 || hb::hbCursor() != S.msWnd) { S.hovKey.clear(); return; }   // (1.17.1) BBA_HOVERS
    if (S.blOn && S.msH >= S.blBox.l && S.msH <= S.blBox.r && S.msV >= S.blBox.t && S.msV <= S.blBox.b) {   // (1.16.0) the Big Lot box's hover
        S.hovKey = "BLBOX";
        const bba::TypeAvg& A = S.blNightNow ? S.blNight : S.blDay; const bba::TypeAvg& O = S.blNightNow ? S.blDay : S.blNight;
        const std::vector<std::string> L = bba::boxHover(A, O, S.market, S.blTiers, S.blNightNow ? S.blDropNight : S.blDropDay, S.scoreLine);
        FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
        int w = 0; for (const auto& s2 : L) w = std::max(w, getTextWidth(s2.c_str(), -1));
        w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
        int l = S.msH + 14, t = S.msV + 14;
        if (l + w > axisR - 2) l = S.msH - 14 - w;
        if (l < pane.left + 2) l = pane.left + 2;
        if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
        if (t < pane.top + 2) t = pane.top + 2;
        RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
        setTextColor(C_INK);
        for (size_t i = 0; i < L.size(); ++i) { RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false); }
        return;
    }
    const HoverT* T = nullptr;
    for (const auto& h : S.targets) if (S.msH >= h.box.l && S.msH <= h.box.r && S.msV >= h.box.t && S.msV <= h.box.b) { T = &h; break; }
    S.hovKey = T ? std::to_string(T->t) + (T->side < 0 ? "b" : "a") : std::string();
    if (!T) return;
    const bba::Facts* F = nullptr;
    { auto it = S.fin.find(T->t); if (it != S.fin.end()) F = &it->second; else if (S.haveForming && S.forming.t == T->t) F = &S.forming; }
    if (!F) return;
    std::vector<std::string> L;
    {   // (1.12.0, Rassul 15:10 "lets go with your recommendations") what the print is, why it is shown, where it is, how big today
        const std::string mid = " \xB7 ";
        const bba::Cat& c = set.cat[T->pi.cat < 0 || T->pi.cat > 3 ? 0 : T->pi.cat];
        long days = 0; int endSec = 0; parts(T->t, days, endSec);
        const svl::Cls me = svl::classify(days, endSec, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin);
        int bigger = 0;                                               // same side, same session, bigger prints (rank today)
        for (const auto& kv : S.fin) { long d2 = 0; int e2 = 0; if (!parts(kv.first, d2, e2)) continue;
            if (svl::classify(d2, e2, S.spb > 0 ? S.spb : 180, S.openMin, S.closeMin).sess != me.sess) continue;
            const bba::Pick& q = T->side < 0 ? kv.second.bid : kv.second.ask; if (q.ok && q.vol > T->lots) ++bigger; }
        const int rank = bigger + 1;
        const std::string nth = rank == 1 ? "the biggest" : std::to_string(rank) + (rank % 10 == 2 && rank != 12 ? "nd" : rank % 10 == 3 && rank != 13 ? "rd" : rank == 1 || (rank % 10 == 1 && rank != 11) ? "st" : "th") + " biggest";
        const std::string catTxt = c.n > 0 ? "Big Lot " + std::to_string(c.n) + " (category " + std::to_string(T->pi.cat + 1) + ")" : ">= " + bba::commas(c.lots) + " lots (category 4)";
        std::string catWhy;   // (1.16.0) which category won and why: 1 > your category 4 > 2 / 3
        if (T->pi.cat == 3) catWhy = " - your own size: it ranks above Big Lot categories 2 / 3 (only category 1 ranks higher)";
        else if (T->pi.cat == 0 && set.cat[3].on && set.cat[3].lots > 0 && T->lots >= set.cat[3].lots) catWhy = " - also over your category 4 (" + bba::commas(set.cat[3].lots) + "); category 1 ranks first";
        L.push_back(catTxt + catWhy + mid + bba::commas(T->lots) + (T->side < 0 ? " lots SOLD AT THE BID" : " lots BOUGHT AT THE ASK") + " at " + bba::price((long long)std::llround(T->px / S.tick), S.tick)
                    + mid + nth + (T->side < 0 ? " bid" : " ask") + " print this session");
        std::string where = "Candle " + clockOf(F->t) + (F->closed ? " (closed)" : " (forming - may still change)");
        if (!T->pi.lv.empty()) where += mid + "at " + T->pi.lv + " " + bba::price((long long)std::llround(T->pi.lvPx / S.tick), S.tick) + (T->pi.sweep ? " (sweep)" : "");
        const std::string trTxt = T->pi.trend >= 2 ? "Strong Uptrend" : T->pi.trend == 1 ? "Uptrend" : T->pi.trend == -1 ? "Dntrend" : T->pi.trend <= -2 ? "Strong Dntrend" : "no trend";
        if (T->pi.piv) where += mid + "on a pivot" + (T->pi.conf ? "" : " (not confirmed yet - may still go)");
        if (T->pi.how == 1) where += mid + "Trend PB: " + trTxt + " pullback";
        if (T->pi.how == 3) where += mid + "Trend BO: " + trTxt + (T->pi.brkDir > 0 ? ", broke out through the " : ", broke down through the ") + (T->pi.brk == 2 ? "higher pivot 2 back" : "prior pivot");
        L.push_back(where);
        const int ab = set.circle ? bba::absorptionMin(*F, T->lots) : 0;
        if ((ab > 0 && T->side < 0) || (ab < 0 && T->side > 0))
            L.push_back(T->side < 0 ? "ABSORPTION PRINT: sellers hit at/below the halfway mark and the candle closed GREEN above it - buyers absorbed them."
                                    : "ABSORPTION PRINT: buyers lifted at/above the halfway mark and the candle closed RED below it - sellers absorbed them.");
        L.push_back(T->side < 0 ? "What: the price in this candle where the most lots were sold at the bid (a big seller hitting)."
                                : "What: the price in this candle where the most lots were bought at the ask (a big buyer lifting).");
        L.push_back("Question: is a large trader acting here, and is it at a place that matters?");
        if (T->pi.cat < 3) L.push_back(std::string("Shown by: ") + (c.show == bba::SH_ALL ? "All prints" : c.show == bba::SH_PIVOT ? "Near pivot" : c.show == bba::SH_LEVEL ? "Near a level" : c.show == bba::SH_PIVLEVEL ? "Pivot near a level" : c.show == bba::SH_PB ? "Trend PB" : c.show == bba::SH_BO ? "Trend BO" : "Trend PB + BO"));
        if (c.n > 0) L.push_back("How: Big Lot " + std::to_string(c.n) + " = the size reached about " + std::to_string(c.n) + " times a session (" + (dayOf(S, endSec / 60) ? "day" : "night")
                                 + " this session " + bba::commas(bba::bigLotOf(tiersAt(S, me.sess), dayOf(S, endSec / 60) ? 1 : 0, c.n)) + " lots" + (c.lots > 0 ? ", min " + bba::commas(c.lots) : std::string("")) + "); this print reached it.");
        else L.push_back("How: category 4 shows every print of " + bba::commas(c.lots) + " lots or more.");
        L.push_back("Why: size at a level that does not move price is absorption; size that breaks a level and holds is initiative.");
        L.push_back(T->side < 0 ? "Use: a big seller at a low / support that fails to push lower - watch for the close back up and a turn."
                                : "Use: a big buyer at a high / resistance that fails to push higher - watch for the close back down and a turn.");
        L.push_back("Example: 1,318 sold at PDH after a sweep, the candle closes back below PDH: the breakout buyers above are trapped.");
    }
    FONT hf; hf.id = HELVETICA; hf.size = 10; hf.style = PLAIN; setFont(hf);
    int w = 0; for (const auto& s : L) w = std::max(w, getTextWidth(s.c_str(), -1));
    w += 16; const int lh = 15, h = (int)L.size() * lh + 10;
    int l = S.msH + 14, t = S.msV + 14;
    if (l + w > axisR - 2) l = S.msH - 14 - w;
    if (l < pane.left + 2) l = pane.left + 2;
    if (t + h > pane.bottom - 2) t = pane.bottom - 2 - h;
    if (t < pane.top + 2) t = pane.top + 2;
    RCT bg; bg.set(coord(l), coord(t), coord(l + w), coord(t + h)); bg.draw(1, C_HOVEDGE, C_HOVBG, DRAW_OPAQUE, PAT_SOLID);
    for (size_t i = 0; i < L.size(); ++i) {
        setTextColor(i == 0 ? (T->side < 0 ? C_SELL : C_BUY) : C_INK);
        RCT tr; tr.set(coord(l + 8), coord(t + 5 + (int)i * lh), coord(l + w - 4), coord(t + 5 + (int)(i + 1) * lh)); tr.drawText(L[i].c_str(), false, false);
    }
}
// the mouse: remember where it is; redraw only when the hovered number changes. Never swallows the event.
int BigBidAsk::mouse(RTX_EVENT* e) {
    if (!e || (e->type != E_MOUSE_MOVE && e->type != E_MOUSE_HOVER)) return RTX_FAIL;
    BBState* S = slot_.get(this, false); if (!S) return RTX_FAIL;
    if (!BBA_HOVERS && S->rlyTargets.empty() && S->obTargets.empty() && S->hovKey.empty()) return RTX_FAIL;   // (1.24.0) only the Rly / (1.25.0) OB / BO hovers use the mouse
    S->msHas = true; S->msH = e->v.mouse.h; S->msV = e->v.mouse.v; S->msWnd = hb::hbCursor();
    std::string k;
    for (const auto& h : S->rlyTargets) if (S->msH >= h.box.l - 2 && S->msH <= h.box.r + 2 && S->msV >= h.box.t - 2 && S->msV <= h.box.b + 2) { k = "RLY" + std::to_string(h.key); break; }   // (1.24.0)
    if (k.empty()) { long long bestA = -1;   // (1.25.0) the smallest OB / BO box under the cursor
        for (const auto& h : S->obTargets) if (S->msH >= h.box.l && S->msH <= h.box.r && S->msV >= h.box.t - 1 && S->msV <= h.box.b + 1) {
            const long long ar = (long long)(h.box.r - h.box.l) * (h.box.b - h.box.t); if (bestA < 0 || ar < bestA) { bestA = ar; k = "OB" + std::to_string(h.id); } } }
    if (!k.empty() || !BBA_HOVERS) {}
    else if (S->blOn && S->msH >= S->blBox.l && S->msH <= S->blBox.r && S->msV >= S->blBox.t && S->msV <= S->blBox.b) k = "BLBOX";   // (1.16.0)
    else for (const auto& t : S->targets) if (S->msH >= t.box.l && S->msH <= t.box.r && S->msV >= t.box.t && S->msV <= t.box.b) { k = std::to_string(t.t) + (t.side < 0 ? "b" : "a"); break; }
    if (k != S->hovKey) { S->hovKey = k; invalidateChart(false); }
    return RTX_FAIL;
}

extern "C" cppExtension* CreateExtension(void) {
    BigBidAsk* p = new BigBidAsk();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE | TRACK_MOUSE | VAP_REQUIRED);   // (1.24.0) TRACK_MOUSE for the Rly hover only (print hovers stay off: BBA_HOVERS)
    p->setDescription("LRA Big Bid / Ask 1.26.0: OB (3-min) = the order block print of a strong wave (an Rly wave, or any wave of 3+ ATR): the biggest bid / ask volume on the pivot candle and the next 1 - 3 candles (the 2nd biggest when the BO already shows the biggest), its lots in the side box + OB below the candle of an up-wave / above for a down-wave; decided when the wave is confirmed, never repainted (not proven yet). Rly (3-min) = the end of a confirmed trend wave (3+ bars, 2.5+ ATR, tagged VWAP +/-1 SD, a 20-bar high / low, vwapTrend Uptrend / Dntrend): green above / red below the bar - don't chase it, wait for the pullback to VWAP / 1 SD and a trigger, trend only. BO = context, not an entry: a confirmed break - don't fade against it (in the test it continued ~49%, like any candle in the wave). A breakout (breakdown) candle through one of the last 3 intact swing highs (lows) of the 1-bar ZigZag with a 10-candle high (low), range and volume up; BO green above a green candle closing above the swing, BO red below a red candle closing below it - a wick-only break counts when a later candle of the same wave closes beyond the pivot (the letter goes on that candle); failed wick breaks are logged only. Its biggest buy (sell) print in lots on the candle. on each candle, the biggest print AT THE BID (left box) and AT THE ASK (right box); the number = how many candles back it is the biggest of its side (capped at 50), shown from the Biggest in setting (default 10). Settings: Biggest in, Skip first / last minutes of each session (default 15 / 15), font size, number colour, bid / ask box colours. The Big Lot box (BL5 / BL10 / BL20) sits beside the HOD/LOD table.");
    p->setVersion("1.26.0");   // BBA_VERSION
    return p;
}
