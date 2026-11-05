#ifndef LS_BREAKOUT_LOGIC_H
#define LS_BREAKOUT_LOGIC_H
// BreakoutLogic.h 1.6.0 (2026-11-04 19:22 CT) -- lsBreakout 1.6.0 (Rassul 19:06 "lets remove the BI signal and require the PB which
//   tests the BO to pullback to the pivot with less than 1/2 atr remaining", 19:21 "make it 1/3", "go" on mockup pb_mock v4):
//   1. BI REMOVED: SHOW_BI = false - no BI pass, no BI letters, no BBI| records, and no BO / BD is cancelled by a BI any more (a waiting
//      BO / BD fires as in 1.4.0). The BI code stays below, unused.
//   2. Pb = a pullback pivot after a BO that comes back TO the broken pivot (the BO's bba::BoDec::piv) or stops less than PB_ATR_FRAC
//      (1/3) of the 14-bar Wilder ATR (the Rly ATR, rly::atrWilder, at the BO candle) short of it: pivot LOW <= piv + ATR / 3 (BD mirror:
//      pivot HIGH >= piv - ATR / 3). This REPLACES 1.1.0's "the pivot bar overlaps the BO / BD candle". Unchanged: the 3-bar pivots, one Pb
//      per pivot, looked for until the next fired BO / BD or a trade beyond the wave's origin, decided once, kept (no repaint).
//      ES Oct 20 - Nov 3: 160 Pb (1.5.0) -> 181 (145 the same, 15 dropped as > 1/3 ATR short, 36 new: tested the level below the candle).
// was BreakoutLogic.h 1.5.0 (2026-11-04) -- lsBreakout 1.5.0 (Rassul approved the BreakIn mockup v0.3 and said "build"): + "BI" (BreakIn =
//   a FAILED breakout / breakdown). BI DOWN (a failed BO; BI UP mirrors a failed BD):
//   1. attempt = any BO / BD decision candle that traded through the BO pivot (the swing bba::breakoutAt broke) - unconfirmed
//      (no_confirm / still waiting), fired, or any other reject (wrong_colour, range_below_prior, not_10bar_extreme, ...);
//   2. the BI candle = the attempt candle itself or one of the next BI_N = 10 candles (a FIRED BO: the 10 after its follow-through
//      candle, the BI after it) that is the opposite colour (red for BI down), has BREAKOUT RANGE (range >= the average range of the
//      previous 10 candles AND (volume > the prior candle's OR range > the prior candle's range)) and CLOSES back past the pivot;
//   3. one BI per pivot (first qualifying; key = pivot in ticks + side). Decided at the candle's close, kept for the chart's life (no
//      repaint). A BI while the attempt still WAITS for its follow-through CANCELS that BO / BD: it is never drawn, gets no Pb, and its
//      BBO record says fired 0 / reason bi_cancel (Engine::biCancel; false = the v0.3 prototype's one-pass rule, tests only).
//   4. every session (the BO zigzag carries across 17:00 CT); the record says the session.
//   Drawn as the text "BI" only (red just above the high for BI down, green just below the low for BI up, BO / BD font, the BO row 3 px
//   from the wick); a row taken by the zigzag wave-time label (pivot bar) / lsCandleAbsorb's A row (pivot bar, its re-test bar) / our
//   own letters moves it one row out (16 px), the LVP row (+51) skipped. Records: BBI| lines in the breakouts file (key t_bi + side +
//   pivot), first line "| bi 1".
// was BreakoutLogic.h 1.4.0 (2026-11-04) -- lsBreakout 1.4.0 (Rassul "BO 5PM - yes"): the BO pivot ZigZag CARRIES across the 17:00 CT
//   Globex session break (and the Sunday open) instead of starting again each session (bba::swingBreaksPx carry / swgCarry, passed only
//   here: Engine::zzCarry with zzRev > 1; lsBigBidAsk and the zzRev = 1 path keep the per-session walk). The evening's first candles can
//   now break the prior session's intact swing highs / lows. Everything else unchanged (BO rules incl. the 20-bar window clipped at the
//   session start, follow-through tie, Rly = RTH waves, Pb on the chart-wide 3-bar pivots, record layout); records say "zz_carry 1" in
//   the file's first line and "1.4.0-zz3" in every line. ES Oct 20 - Nov 3 19:45 one pass: fired 144 -> 152 (+8, all 17:03 - 18:27 CT
//   evening candles, 0 lost); replay bar by bar == one pass.
// was BreakoutLogic.h 1.3.0 (2026-11-03 evening) -- lsBreakout 1.3.0 (Rassul removed lsBigBidAsk from his charts at 19:45 CT, "do it"):
//   (1) RECORDS: lsBreakout now writes the decision records the nightly scores, from ITS OWN 3-bar decisions, in the same line layout
//       lsBigBidAsk used (bba::boLine 27-field BBO|, rly::line 17-field RLY|) plus a Pb record:
//         lsFlexLevels\Breakout\<MKT>-<SYM>-<spb>-breakouts-<daykey>.csv   BBO| one line per decided candidate (fired / no_confirm / reason)
//         lsFlexLevels\Breakout\Rly\<MKT>-<SYM>-<spb>-<daykey>.csv         RLY| one line per judged wave (3-min charts)
//         lsFlexLevels\Breakout\<MKT>-<SYM>-<spb>-pb-<daykey>.csv          BPB| one line per Pb (pivot bar, side, BO / BD candle, prices)
//       <MKT> / <SYM> / <daykey> exactly as lsBigBidAsk named them (marketName, fileKey, sessionOf: 17:00 CT starts the next day).
//       The ZigZag setting is in every line (version field "1.3.0-zz3") and in the file's first line ("# lsBreakout 1.3.0 | ... |
//       zz_rev_bars 3 | ft_tie 1"), so the scorer can tell these from lsBigBidAsk's 1-bar lines. ONE line per decision: a key per line
//       (BO t0 + dir, Rly t_e + dir, Pb t_pivot + side); the file is read before an append and a key already in it is never written
//       again (restart, recompute, two charts of the same market / contract / period) - lsBigBidAsk's Oct 30 file held most fired lines
//       8 - 10 times because it appended without looking. Only the last 3 sessions are written.
//   (2) FOLLOW-THROUGH TIE: the rule-5 follow-through close may EQUAL the breakout candle's close (BO: a later green candle of the wave
//       with a higher high AND close >= the BO close; BD: red, lower low AND close <= the BD close) - bba::boWaves ftTie, passed only
//       here (Engine::ftTie with zzRev > 1); lsBigBidAsk and the zzRev = 1 replay keep the strict rule. ES Oct 20 - Nov 3: fired 142 -> 144
//       (+ Oct 23 01:48 BO, + Nov 3 19:06 BD shown at the 19:21 close).
// was BreakoutLogic.h 1.2.0 (2026-11-03) -- lsBreakout 1.2.0: BO / BD and Rly on the 3-bar High / Low ZigZag (Rassul "go": check BO / BD +
//   Rly on the 3-bar zigzag over a week, then switch "BO" to 3-bar). The swing walk that gives the BO pivots (SwingRef.h ZigZag::rev),
//   the wave direction for one-per-wave / follow-through (bba::zigDir) and the Rly waves (rly::run) now use the SAME turn rule as the Pb
//   pivots and lsZzoCombo 2.1.0 (zzd::Engine::step, COMBO_REV_BARS = 3: an up wave turns down on a LOW below the previous 3 bars' lows,
//   a down wave turns up on a HIGH above their highs) - so BO / BD, Rly and Pb all refer to the same swings. Every other threshold
//   unchanged (10-bar extreme, range / volume, colour, follow-through, 20-bar counter-trend, Rly 2.5 ATR / 1 SD or 30-bar / 20-bar /
//   trend). lsBigBidAsk keeps the 1-bar rule (rev default 1: its decisions and records are unchanged). Week check ES / NQ / GC
//   Oct 26 - Nov 3 (week_1v3.png): BO / BD RTH works 58/94 -> 50/76, session 173/315 -> 146/267; Rly 52/65 -> 64/69 (later by ~1 bar).
// was BreakoutLogic.h 1.1.1 (2026-11-03) -- lsBreakout 1.1.1: Pb moved TWO rows out from the wick (Rassul's screenshot: the green "Pb" sat on
//   the zigzag wave-time labels "9m" / "16m" drawn right at the pivot's wick, ~14 px tall from 3 px out; lsCandleAbsorb's A / Ae uses the
//   next row, ROW_OUT 16 px further). Pb now: below a pivot LOW the letter's top = Y(low) + 3 + 32; above a pivot HIGH its bottom =
//   Y(high) - 3 - 32 (PB_ROWS = 2 rows of ROW_OUT 16 px). Collision rule unchanged (another BO / BD / Rly letter there: one more row out,
//   up to 4 rows; no room = dropped and counted). BO / BD / Rly positions and every decision unchanged.
// was BreakoutLogic.h 1.1.0 (2026-11-03) -- lsBreakout 1.1.0: + "Pb" (Rassul approved the Pb mockup - ES Oct 30 - and said "build"):
//   Pb = a pullback that TESTS the BO / BD candle. Pivots = the 3-bar High / Low ZigZag (zzd::Engine::step with COMBO_REV_BARS = 3,
//        the engine of lsZzoCombo 2.1.0 / lsZzoZigZag: a down wave turns up on a bar whose HIGH is above the highs of the previous 3
//        bars, an up wave turns down on a LOW below the previous 3 bars' lows; a strictly higher high / lower low moves the extreme).
//        After a fired BO on bar i: every confirmed pivot LOW p > i whose bar overlaps the BO candle (low <= BO high AND high >= BO low)
//        gets a green "Pb" just below its low, until EITHER the next fired BO / BD (any side, t0 in (i, p]) OR a bar in (i, p] trades
//        below the BO wave's origin (the last pivot LOW before i). BD mirrored (pivot HIGHs, red "Pb" just above the high, origin = the
//        last pivot HIGH before i).
//        Decided when BOTH are known: the pivot is confirmed (its ZigZag turn bar closed) and the BO / BD has fired (its follow-through
//        closed) = decision bar d = max(confirm, fire). Only BO / BD letters fired by d block (the chart cannot know a later one), so a
//        restart (one full pass) gives the same Pb as the live chart. A Pb, once made, is kept for the chart's life (no repaint; a later
//        invalidation never removes it). Same small font as BO / BD, 3 px from the wick; never on another letter (one row further out).
// was BreakoutLogic.h 1.0.0 (2026-11-03) -- the SDK-free part of lsBreakout ("BO", Breakout.cpp). Rassul approved the mockup
// (bo_mockup.html, ES Oct 30: "BO / BD sit small, right at the breakout candle ... letters only") and said "build".
// The chart shows ONLY letters:
//   "BO" green just ABOVE the breakout candle's high, "BD" red just BELOW the breakdown candle's low (GAP px from the wick), small font;
//        the decision is lsBigBidAsk's, by the SAME code: bba::boFromCandles (BigBidAskLogic.h 1.32.0) = an intact swing of the last 3
//        traded through, a 10-candle high / low (20-bar extreme in the wave when against his vwapTrend), range >= the prior candle's,
//        volume or range up, green / red, one per wave, and a FOLLOW-THROUGH candle later in the same wave (green, higher high and higher
//        close; BD mirrored). The letter sits on the breakout candle and appears when the follow-through candle closes.
//   "Rly" green above (end of a rally) / red below (end of a decline) on the wave's extreme bar, from RallyLogic.h (rly::run, the
//        rly2.py v0.2 port lsBigBidAsk uses), 3-min charts only, one row further out than BO / BD so the two never collide.
// No numbers, no prints, no hovers, no boxes. Decided on closed candles only; a decision once made is kept for the chart's life and
// never changes or disappears (keyed by the bar time; a later recompute never removes or alters one). Tested: tests_bo100.
#include "BigBidAskLogic.h"
#include "VwapTrendLogic.h"
#include "RallyLogic.h"
#include "ZzoDeltaLogic.h"   // (1.1.0) zzd::Engine::step = the 3-bar High / Low ZigZag of lsZzoCombo 2.1.0 (COMBO_REV_BARS)
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <cctype>
#include <climits>
#include <cstdio>
#if defined(_WIN32)
#define BKO_REC_SEP "\\"
#else
#define BKO_REC_SEP "/"
#endif

namespace bko {

#ifndef BKO_RLY_ON_DEFAULT
#define BKO_RLY_ON_DEFAULT 1   // (1.7.0) the plugin judges the overnight Rly; the 1.6.0 parity suites compile with -DBKO_RLY_ON_DEFAULT=0
#endif
static const char* const VERSION = "1.7.0";   // (1.7.0, 2026-11-05 13:13 CT Rassul "this is a clear rly dn" -> "go") + overnight Rly (rly::run overnight = true: 17:00 CT to the RTH open on its own Globex VWAP / trend; records with version ...-on)   // was "1.6.0";   // (1.6.0) BI removed; Pb within 1/3 ATR of the broken pivot   // was "1.5.0":   // (1.5.0) + BI (BreakIn = failed BO / BD)   // (1.4.0) zigzag carries across the 17:00 CT break   // (1.3.0) records + follow-through tie   // (1.2.0) BO / BD + Rly on the 3-bar High / Low ZigZag   // was "1.1.1" (Pb two rows out), "1.1.0" (+ Pb), "1.0.0"
static const int GAP = 3;              // px between the wick end and the BO / BD letter
static const int RLY_ROW = 16;         // the Rly letter sits this many px further out than the BO / BD row
static const int ROW_OUT = 16;         // (1.1.1) one row = lsCandleAbsorb's ROW_OUT (its A / Ae sits one row beyond the wick label row)
static const int PB_ROWS = 2;          // (1.1.1) Pb sits this many rows beyond the wick row: clear of the wave-time label (row 0) and A / Ae (row 1)
static const int PB_OFF = GAP + PB_ROWS * ROW_OUT;   // (1.1.1) = 35 px from the wick end to the Pb letter's near edge
static const int FONT_BO = 7;          // ~9 px letters (Rassul: small, smaller than lsBigBidAsk's 10)
static const int FONT_RLY = 9;         // the mockup's Rly size
static const unsigned long C_UP = 0x0022C55E, C_DN = 0x00EF4444;   // green bullish / red bearish (his C_BUY / C_SELL)
static const int MAX_BARS = 20000;     // bars considered (as lsBigBidAsk's Rly pass)
static const int PB_REV_BARS = zzd::COMBO_REV_BARS;   // (1.1.0) the Pb pivots: 3-bar High / Low ZigZag (lsZzoCombo 2.1.0)
static const int ZZ_REV_BARS = zzd::COMBO_REV_BARS;   // (1.2.0) BO / BD pivots + wave and Rly waves: the same 3-bar High / Low ZigZag
#ifndef BKO_ONE_BAR
#define BKO_ONE_BAR 0                    // (1.6.1, 2026-11-04 Rassul "there needs to be a 1bar option ... for higher timeframes") 1 = the lsBreakout1 build
#endif
#if BKO_ONE_BAR
#define BKO_REC_DIR "Breakout1"           // (1.6.1) lsBreakout1 records apart from the 3-bar build's (nightly compares the two)
#else
#define BKO_REC_DIR "Breakout"
#endif
static const int DEF_REV_BARS = BKO_ONE_BAR ? 1 : ZZ_REV_BARS;   // (1.6.1) the build's ZigZag: 1-bar (lsBreakout1) or 3-bar (lsBreakout)

// one chart bar (the host fills it from IRT; t = the raw IRT stamp = the bar END, days / endSec = local CT civil day / second)
struct In { long long t = 0; bool ok = false; long days = 0; int endSec = 0; double o = 0, h = 0, l = 0, c = 0; long long vol = 0; };
struct BoMark { long long t0 = 0, t = 0; int dir = 0; double piv = 0; bool fired = false; int reason = 0; };
struct RlyMark { long long te = 0; int dir = 0; bool fired = false; };
struct RecL { long long sess = 0; std::string line; };   // (1.3.0) a record line, built once at the decision, and its file's session (daykey)
// (1.3.0) lsBigBidAsk's session key (BigBidAsk::sessionOf): the CT civil second, 17:00 CT starts the next day's file
inline long long sessOf(long days, int endSec) { const long long cs = (long long)days * 86400LL + endSec; return cs > 0 ? (cs - 17LL * 3600) / 86400LL + 1 : -1; }
// (1.1.0) one Pb: the pivot bar (tp), +1 = a pivot LOW after a BO (green, below) / -1 = a pivot HIGH after a BD (red, above), the BO / BD
// candle it tested (t0), the bar that confirmed the pivot (tc) and the decision bar (td = the later of tc and the BO / BD's fire bar)
struct PbMark { long long tp = 0, t0 = 0, tc = 0, td = 0; int dir = 0; };
struct Pivot { long long t = 0, tc = 0; int type = 0; double h = 0, l = 0; };   // type +1 = pivot HIGH, -1 = pivot LOW; tc = confirm bar
// (1.5.0) BI = BreakIn (a failed BO / BD). dir -1 = BI DOWN (a failed BO: red candle closing back BELOW the pivot, letter above the high),
// +1 = BI UP (a failed BD, letter below the low). kind = the attempt as known at the BI close: 0 unconfirmed (no_confirm or still waiting),
// 1 fired (the BI after its follow-through), 2 other reject. cancel = the attempt was still waiting (that BO / BD is cancelled).
static const bool SHOW_BI = false;    // (1.6.0, Rassul 2026-11-04 19:06 CT "lets remove the BI signal") no BI pass: no letters, records or BO / BD cancels
static const double PB_ATR_FRAC = 1.0 / 3.0;   // (1.6.0, 19:21 CT "make it 1/3") a Pb comes back to within this share of the ATR of the broken pivot
static const int BI_N = 10;            // the BI window: the attempt candle + the next 10 (fired: the 10 after the follow-through candle)
static const int BI_AVG = 10;          // breakout range = range >= the average range of the previous 10 candles (+ volume or range up)
enum { BI_UNCONF = 0, BI_FIRED = 1, BI_OTHER = 2 };
inline const char* biKindName(int k) { return k == BI_FIRED ? "fired" : k == BI_OTHER ? "other" : "unconfirmed"; }
struct BiMark {
    long long t = 0, t0 = 0, tf = 0;   // the BI candle, the attempt candle, the attempt's follow-through candle (fired; 0 = none)
    int dir = 0, kind = 0; bool cancel = false; int reason = 0;   // cancel = the attempt still WAITED for its follow-through at the BI close (BBI field "waiting")
    long long pk = 0; double piv = 0, stop = 0, h = 0, l = 0, c = 0;   // pivot (ticks / price), the attempt extreme (stop), the BI candle
    long long rng = 0, sum10 = 0, prng = 0, vol = 0, pvol = 0;          // ticks / lots (avg10 = sum10 / 10)
    long long sess = -1; int skind = 0;                                  // daykey (17:00 CT starts the next) + svl kind (RTH / ON / POST)
};
static const char* const BI_HEADER = "BBI|t_bi|dir|t0|t_fire|kind|waiting|pivot|stop|bi_high|bi_low|bi_close|range|avg10|prior_range|vol|prior_vol|vol_up|range_up|daykey|session|version";
inline const char* sessName(int k) { return k == svl::RTH ? "RTH" : k == svl::ON ? "ON" : k == svl::POST ? "POST" : "NONE"; }
inline std::string biLine(const BiMark& m, const std::string& ver) {
    char b[480]; std::snprintf(b, sizeof b, "BBI|%lld|%d|%lld|%lld|%s|%d|%.10g|%.10g|%.10g|%.10g|%.10g|%lld|%.1f|%lld|%lld|%lld|%d|%d|%lld|%s|%s",
        m.t, m.dir, m.t0, m.tf, biKindName(m.kind), m.cancel ? 1 : 0, m.piv, m.stop, m.h, m.l, m.c, m.rng, (double)m.sum10 / BI_AVG, m.prng,
        m.vol, m.pvol, m.vol > m.pvol ? 1 : 0, m.rng > m.prng ? 1 : 0, m.sess, sessName(m.skind), ver.c_str());
    return b;
}
// a BBO| line with fired 0 / reason bi_cancel (the BO / BD a BI cancelled while it waited for its follow-through)
inline std::string boCancelLine(const std::string& line) {
    std::vector<std::string> f; { size_t a = 0; for (;;) { const size_t q = line.find('|', a); f.push_back(line.substr(a, q == std::string::npos ? std::string::npos : q - a)); if (q == std::string::npos) break; a = q + 1; } }
    if (f.size() < 6 || f[0] != "BBO") return line;
    f[4] = "0"; f[5] = "bi_cancel";
    std::string o; for (size_t k = 0; k < f.size(); ++k) { if (k) o += "|"; o += f[k]; } return o;
}
// (1.3.0) the Pb record: one line per drawn Pb. side +1 = a pivot LOW after a BO (green, below), -1 = a pivot HIGH after a BD (red, above);
// t0 = the BO / BD candle it tested, t_fire = that BO / BD's follow-through close, t_conf = the bar that confirmed the pivot, t_dec = the
// Pb's decision bar (the later of the two); prices = the pivot bar's high / low, the BO / BD candle's high / low / close, the broken pivot.
static const char* const PB_HEADER = "BPB|t_pivot|side|t0|t_fire|t_conf|t_dec|pivot_high|pivot_low|bo_high|bo_low|bo_close|bo_pivot|version";
inline std::string pbLine(const PbMark& m, const bba::BoDec& D, const Pivot& p, const In& c, const std::string& ver) {
    char b[400]; std::snprintf(b, sizeof b, "BPB|%lld|%d|%lld|%lld|%lld|%lld|%.10g|%.10g|%.10g|%.10g|%.10g|%.10g|%s",
        m.tp, m.dir, m.t0, D.t, m.tc, m.td, p.h, p.l, c.h, c.l, c.c, D.piv, ver.c_str());
    return b;
}

struct Engine {
    // inputs of the last update
    std::vector<In> bars; double tick = 0; int spb = 0, openMin = 510, closeMin = 900;
    int zzRev = DEF_REV_BARS;                  // (1.6.1) = ZZ_REV_BARS (3) in lsBreakout, 1 in lsBreakout1; (1.2.0) ZigZag reversal bars for BO / BD + Rly (3; tests set 1 = lsBigBidAsk's path; set before the first update)
    bool ftTie = true;                         // (1.3.0) follow-through close may equal the BO close - only with zzRev > 1 (zzRev 1 = lsBigBidAsk's strict rule)
    std::map<long long, RecL> boL, rlyL, pbL;  // (1.3.0) the record line of each decision (same keys as bo / rly / pb)
    bool refine1 = BKO_ONE_BAR != 0;           // (1.6.1) lsBreakout1: the 3-bar build's refinements (follow-through tie, carry across 17:00 CT) also at zzRev 1
    bool tieOn() const { return ftTie && (zzRev > 1 || refine1); }
    bool zzCarry = true;                       // (1.4.0) the BO pivot zigzag carries across the 17:00 CT break - only with zzRev > 1 (zzRev 1 = lsBigBidAsk's per-session walk)
    bool carryOn() const { return zzCarry && (zzRev > 1 || refine1); }
    std::string recVer() const { return std::string(VERSION) + "-zz" + std::to_string(zzRev); }   // the version field of every record line
    // decisions (kept for the chart's life; never changed once made)
    std::map<long long, bba::BoDec> bo;        // t0 -> lsBigBidAsk's decision record (fired or not)
    std::map<long long, RlyMark> rly;          // te * 2 + (dir > 0) -> judged wave
    bool rlyOn = BKO_RLY_ON_DEFAULT != 0;      // (1.7.0) judge the overnight session too (false = the 1.6.0 RTH-only Rly, for the older parity tests)
    std::map<long long, int> trend;            // closed bar time -> vwapTrend level (cache)
    std::map<long long, PbMark> pb;            // (1.1.0) pivot bar time -> Pb (kept for the chart's life)
    std::vector<Pivot> piv;                    // (1.1.0) the confirmed 3-bar ZigZag pivots of the last pass (closed bars)
    long long curExtT = 0; int curExtType = 0; // (1.5.0) the zigzag's current (unconfirmed) extreme: bar time, +1 HIGH / -1 LOW (0 = none)
    // (1.5.0) BI: the decided BIs (key = pivot ticks * 2 + (dir > 0); kept for the chart's life), their record lines, the BO / BD
    // attempts still waiting for their follow-through at the last closed bar (bba::BoRun::pend, times filled), the cancelled BO / BD (t0)
    bool biCancel = true;                      // a BI while the attempt waits cancels that BO / BD (false = the v0.3 prototype's one-pass rule: tests only)
    std::map<long long, BiMark> bi; std::map<long long, RecL> biL; std::vector<bba::BoDec> pendAtt; std::set<long long> boCancel;
    long long biAtt = 0;                       // attempts looked at in the last BI pass (status)
    bool cancelled(long long t0) const { return boCancel.count(t0) != 0; }
    bool shownBo(const bba::BoDec& D) const { return D.fired && D.dir != 0 && !cancelled(D.t0); }   // a BO / BD letter that is drawn
    long long lastClosedT = 0, firstT = 0; long count = -1; bool full = true;   // firstT / count = the CHART's first bar stamp and bar count
    int runs = 0, fullRuns = 0; long long boDup = 0;
    void reset() { *this = Engine(); }

    // every closed bar is final: recompute only on a new closed bar / changed history; a changed history (first bar moved, fewer bars)
    // = one FULL pass again (new decisions only are added; the kept ones stay as they are - no repaint)
    // B = the chart's last bars (the forming bar last); chartN / chartFirst = the chart's bar count and first stamp (history identity)
    bool update(const std::vector<In>& B, long chartN, long long chartFirst, double tk, int per, int om, int cm) {
        const long n = (long)B.size(); if (n < 3 || !(tk > 0) || !std::isfinite(tk)) return false;
        const long long lc = B[(size_t)n - 2].t;
        if (chartN == count && lc == lastClosedT && chartFirst == firstT && tk == tick && per == spb) return false;
        if (count >= 0 && (chartFirst != firstT || chartN < count || tk != tick || per != spb)) full = true;
        bars = B; tick = tk; spb = per; openMin = om; closeMin = cm; count = chartN; lastClosedT = lc; firstT = chartFirst;
        boPass(); if (SHOW_BI) biPass(); rlyPass(); pbPass(); full = false; ++runs; return true;
    }
    int trendAt(int k) {
        if (k <= 0 || k >= (int)bars.size()) return 0;
        const long long t = bars[(size_t)k].t; const auto it = trend.find(t); if (it != trend.end()) return it->second;
        std::vector<svl::Bar> sb; sb.reserve((size_t)k);
        const int a = std::max(0, k - 1500);                                         // a session is < 500 3-min bars; plenty of room
        for (int i = a; i < k; ++i) { const In& b = bars[(size_t)i]; sb.push_back(svl::Bar{ b.days, b.endSec, (float)b.h, (float)b.l, (float)b.c, (double)b.vol }); }
        const vtl::Trend tr = vtl::fromChart(sb, (int)sb.size(), spb > 0 ? spb : 180, openMin, closeMin);
        const int lvl = tr.ready ? vtl::level(tr) : 0;
        if (k < (int)bars.size() - 1) { trend[t] = lvl; if (trend.size() > 60000) trend.erase(trend.begin()); }
        return lvl;
    }
    void boPass() {
        const int n = (int)bars.size();
        std::vector<bba::BoCandle> C((size_t)n);
        for (int k = 0; k < n; ++k) { const In& b = bars[(size_t)k]; bba::BoCandle& c = C[(size_t)k]; c.ok = b.ok; c.days = b.days; c.endSec = b.endSec; c.o = b.o; c.h = b.h; c.l = b.l; c.c = b.c; c.vol = b.vol; }
        int from = 1, emitFrom = 1, fill0 = 0;
        if (!full) bba::boWindow(n, from, emitFrom, fill0);                          // lsBigBidAsk's live window
        else { fill0 = std::max(0, n - 1 - MAX_BARS); from = emitFrom = std::max(1, fill0); ++fullRuns; }
        const bba::BoRun R = bba::boFromCandles(C, tick, from, n - 1, emitFrom, fill0,
            [&](int k) { return trendAt(k); }, [](int) { return true; }, [](int, bba::BoIn&) {}, nullptr, zzRev, tieOn(), carryOn());
        const std::string ver = recVer();
        pendAtt.clear();                                                             // (1.5.0) the attempts still waiting (BI may cancel them)
        for (int sd = 0; sd < 2; ++sd) if (R.pendOn[sd] && R.pend[sd].i0 >= 0 && R.pend[sd].i0 < n - 1) { bba::BoDec P = R.pend[sd]; P.t0 = bars[(size_t)P.i0].t; P.t = 0; pendAtt.push_back(P); }
        for (bba::BoDec D : R.ev) {
            if (D.i0 < 0 || D.im < 0 || D.im >= n - 1) continue;
            D.t0 = bars[(size_t)D.i0].t; D.t = bars[(size_t)D.im].t;
            if (bo.count(D.t0)) continue;                                            // decided before: never again
            if (bba::boDuplicate(bo, D)) { ++boDup; continue; }
            bo[D.t0] = D;
            const In& m = bars[(size_t)D.im];                                        // (1.3.0) the record line, as lsBigBidAsk wrote it (rth / session of the decision candle)
            const svl::Cls cls = m.ok ? svl::classify(m.days, m.endSec, spb > 0 ? spb : 180, openMin, closeMin) : svl::Cls();
            RecL L; L.sess = m.ok ? sessOf(m.days, m.endSec) : -1; L.line = bba::boLine(D, cls.kind == svl::RTH ? 1 : 0, ver.c_str()); boL[D.t0] = L;
        }
        while (bo.size() > 40000) { boL.erase(bo.begin()->first); bo.erase(bo.begin()); }
    }
    // (1.5.0) BI: every attempt (each kept BO / BD decision + the ones still waiting) is checked over its window; the candidates are taken
    // in candle order (ties: the earlier attempt) and the FIRST per pivot + side becomes the BI. A pure function of the closed candles:
    // an attempt is known at its own close, a waiting one at every later close, so the live chart, a replay and one full pass agree.
    bool biQual(int j, int sg, long long pk, BiMark& m) const {
        const int n = (int)bars.size(); if (j < BI_AVG || j > n - 2) return false;
        long long k[4] = { 0, 0, 0, 0 }, sum = 0, ph = 0, pl = 0;
        auto key = [&](double px, long long& o) { if (!std::isfinite(px)) return false; const double q = px / tick; if (!std::isfinite(q) || std::fabs(q) >= 9.0e15) return false; o = (long long)std::llround(q); return true; };
        for (int i = j - BI_AVG; i < j; ++i) { const In& b = bars[(size_t)i]; long long h = 0, l = 0; if (!b.ok || !key(b.h, h) || !key(b.l, l) || h < l) return false; sum += h - l; ph = h; pl = l; }
        const In& b = bars[(size_t)j]; if (!b.ok || !key(b.o, k[0]) || !key(b.h, k[1]) || !key(b.l, k[2]) || !key(b.c, k[3]) || k[1] < k[2]) return false;
        if (!(sg > 0 ? k[3] < k[0] : k[3] > k[0])) return false;                   // the opposite colour: red after a BO, green after a BD
        const long long r = k[1] - k[2], pr = ph - pl; const long long v = b.vol, pv = bars[(size_t)(j - 1)].vol;
        if (!(r * BI_AVG >= sum && (v > pv || r > pr))) return false;              // breakout range
        if (!(sg > 0 ? k[3] < pk : k[3] > pk)) return false;                       // closes back past the pivot
        m.rng = r; m.sum10 = sum; m.prng = pr; m.vol = v; m.pvol = pv; m.h = b.h; m.l = b.l; m.c = b.c; m.t = b.t;
        return true;
    }
    void biPass() {
        const int n = (int)bars.size(); if (n < BI_AVG + 2 || !(tick > 0)) return;
        std::vector<long long> T((size_t)n); for (int k = 0; k < n; ++k) T[(size_t)k] = bars[(size_t)k].t;
        auto idxOf = [&](long long t) { const auto it = std::lower_bound(T.begin(), T.end(), t); return it != T.end() && *it == t ? (int)(it - T.begin()) : -1; };
        const int last = n - 2;                                                      // the last closed candle
        // attempts whose windows can still give a NEW BI: a full pass looks at all of them; a live pass at those from the last 1,500 candles
        const long long tFrom = full ? LLONG_MIN : T[(size_t)std::max(0, n - 1 - 1500)];
        struct Cand { int j; long long t0; int win; BiMark m; };
        std::vector<Cand> C; biAtt = 0;
        auto scan = [&](const bba::BoDec& D, bool waiting) {
            if (D.dir == 0 || !std::isfinite(D.piv)) return;
            const int i0 = idxOf(D.t0); if (i0 < 0) return;
            const int sg = D.dir > 0 ? 1 : -1; const double pq = D.piv / tick; if (!std::isfinite(pq)) return;
            const long long pk = (long long)std::llround(pq);
            const bool fired = !waiting && D.fired; const int im = fired ? idxOf(D.t) : -1; if (fired && im < 0) return;
            ++biAtt;
            int w0[2] = { i0, -1 }, w1[2] = { std::min(i0 + BI_N, last), -1 };
            if (fired) { if (biCancel) w1[0] = std::min(w1[0], im - 1); else w1[0] = -1;   // before the follow-through (cancels) / prototype: none
                         w0[1] = im + 1; w1[1] = std::min(im + BI_N, last); }
            if (waiting && !biCancel) return;
            for (int w = 0; w < 2; ++w) for (int j = w0[w]; w0[w] >= 0 && j <= w1[w]; ++j) {
                BiMark m; if (!biQual(j, sg, pk, m)) continue;
                m.dir = -sg; m.t0 = D.t0; m.pk = pk; m.piv = D.piv; m.reason = waiting ? bba::BO_R_NONE : D.reason;
                m.kind = waiting ? BI_UNCONF : (D.fired ? BI_FIRED : D.reason == bba::BO_R_NO_CONFIRM ? BI_UNCONF : BI_OTHER);
                // waiting = the attempt still waited for its follow-through at the BI close (a pending one, or a no_confirm / fired one
                // decided AFTER this candle): it can never fire now. The same from the live chart and from one full pass.
                const bool pendType = waiting || D.reason == bba::BO_R_NO_CONFIRM || D.fired;
                const int imAny = waiting ? INT_MAX : idxOf(D.t);
                m.cancel = pendType && j < imAny && w == 0; if (m.cancel) m.kind = BI_UNCONF;
                m.tf = (D.fired && w == 1) ? D.t : 0;
                double st = sg > 0 ? bars[(size_t)i0].h : bars[(size_t)i0].l;
                for (int i = i0 + 1; i <= j; ++i) { const In& b = bars[(size_t)i]; if (sg > 0 ? b.h > st : b.l < st) st = sg > 0 ? b.h : b.l; }
                m.stop = st;
                const In& bj = bars[(size_t)j]; if (bj.ok) { m.sess = sessOf(bj.days, bj.endSec); m.skind = svl::classify(bj.days, bj.endSec, spb > 0 ? spb : 180, openMin, closeMin).kind; }
                Cand c; c.j = j; c.t0 = D.t0; c.win = w; c.m = m; C.push_back(c); break;
            }
        };
        for (auto it = bo.lower_bound(tFrom == LLONG_MIN ? LLONG_MIN : tFrom); it != bo.end(); ++it) scan(it->second, false);
        for (const bba::BoDec& P : pendAtt) if (!bo.count(P.t0)) scan(P, true);
        std::stable_sort(C.begin(), C.end(), [](const Cand& a, const Cand& b) { return a.j != b.j ? a.j < b.j : a.t0 < b.t0; });
        const std::string ver = recVer();
        for (const Cand& c : C) {
            const long long key = c.m.pk * 2 + (c.m.dir > 0 ? 1 : 0);
            if (bi.count(key)) continue;                                             // one BI per pivot + side: decided before, never again
            bi[key] = c.m; RecL L; L.sess = c.m.sess; L.line = biLine(c.m, ver); biL[key] = L;
        }
        // a BO / BD that a BI on its pivot hit while it waited (BI candle in [t0, follow-through)) can never fire: cancelled for good
        if (biCancel) for (auto it = bo.lower_bound(tFrom == LLONG_MIN ? LLONG_MIN : tFrom); it != bo.end(); ++it) {
            const bba::BoDec& D = it->second; if (!D.fired || D.dir == 0 || cancelled(D.t0)) continue;
            const double pq = D.piv / tick; if (!std::isfinite(pq)) continue;
            const auto b = bi.find((long long)std::llround(pq) * 2 + (D.dir > 0 ? 0 : 1));
            if (b == bi.end() || b->second.t < D.t0 || b->second.t >= D.t) continue;
            boCancel.insert(D.t0);
            const auto l = boL.find(D.t0); if (l != boL.end()) l->second.line = boCancelLine(l->second.line);
        }
        while (bi.size() > 40000) { biL.erase(bi.begin()->first); bi.erase(bi.begin()); }
        while (boCancel.size() > 40000) boCancel.erase(boCancel.begin());
    }
    void rlyPass() {
        if (spb != 180) return;                                                      // the rule was built and tested on 3-min bars
        const int n = (int)bars.size(); const int first = std::max(0, n - 1 - MAX_BARS);
        std::vector<rly::Bar> B; std::vector<long long> T;
        for (int k = first; k < n - 1; ++k) {                                        // closed bars only
            const In& x = bars[(size_t)k];
            if (!x.ok || !std::isfinite(x.o) || !std::isfinite(x.h) || !std::isfinite(x.l) || !std::isfinite(x.c) || x.h < x.l) continue;
            rly::Bar b; b.days = x.days; b.endSec = x.endSec; b.o = x.o; b.h = x.h; b.l = x.l; b.c = x.c; b.v = (double)x.vol; B.push_back(b); T.push_back(x.t);
        }
        const int from = full ? 0 : std::max(0, (int)B.size() - 600);
        const std::vector<rly::Wave> W = rly::run(B, spb, openMin, closeMin, from, zzRev, rlyOn);   // (1.7.0) + the overnight session
        const std::string ver = recVer();
        for (const rly::Wave& w : W) {
            if (w.ge < 0 || w.gp < 0 || w.gc < 0 || w.ge >= (int)T.size() || w.gp >= (int)T.size() || w.gc >= (int)T.size()) continue;
            const long long te = T[(size_t)w.ge], key = te * 2 + (w.dir > 0 ? 1 : 0);
            if (rly.count(key)) continue;                                            // judged before: never again
            RlyMark m; m.te = te; m.dir = w.dir; m.fired = w.fired; rly[key] = m;
            RecL L; L.sess = w.sess; L.line = rly::line(w, te, T[(size_t)w.gp], T[(size_t)w.gc], (w.on ? ver + "-on" : ver).c_str()); rlyL[key] = L;   // (1.7.0) overnight waves: version ...-on   // (1.3.0) as lsBigBidAsk (w.sess = the RTH day)
        }
        while (rly.size() > 20000) { rlyL.erase(rly.begin()->first); rly.erase(rly.begin()); }
    }
    // (1.1.0) the 3-bar High / Low ZigZag over the closed bars (zzd::Engine::step, N = 3); bars with no valid price are skipped
    void pivots() {
        piv.clear(); const int n = (int)bars.size();
        std::vector<zzd::Bar> Z; std::vector<int> at; Z.reserve((size_t)n); at.reserve((size_t)n);
        for (int k = 0; k < n - 1; ++k) {
            const In& b = bars[(size_t)k]; zzd::Bar z; z.t = b.t;
            if (!b.ok || !(b.h >= b.l) || !zzd::toTicks(b.h, tick, z.h) || !zzd::toTicks(b.l, tick, z.l)) continue;
            Z.push_back(z); at.push_back(k);
        }
        zzd::Walk w;
        for (int j = 0; j < (int)Z.size(); ++j) {
            int s0 = 0, e = 0, d = 0;
            if (!zzd::Engine::step(Z, w, j, s0, e, d, BKO_ONE_BAR ? 1 : PB_REV_BARS)) continue;   // (1.6.1) lsBreakout1: 1-bar Pb pivots
            const In& b = bars[(size_t)at[(size_t)e]];
            Pivot p; p.t = b.t; p.tc = bars[(size_t)at[(size_t)j]].t; p.type = d > 0 ? 1 : -1; p.h = b.h; p.l = b.l; piv.push_back(p);
        }
        curExtT = 0; curExtType = 0;                                                 // (1.5.0) the leg still forming: its extreme so far
        if (Z.size() >= 2 && w.ext >= 0 && w.ext < (int)Z.size()) { curExtT = bars[(size_t)at[(size_t)w.ext]].t; curExtType = w.dirn > 0 ? 1 : -1; }
    }
    // (1.1.0) Pb: see the header. Every fired BO / BD with its candle on the chart is checked against the confirmed pivots after it.
    void pbPass() {
        pivots();
        const int n = (int)bars.size(); if (n < 3 || piv.empty()) return;
        std::vector<long long> T((size_t)n); for (int k = 0; k < n; ++k) T[(size_t)k] = bars[(size_t)k].t;
        auto idxOf = [&](long long t) { const auto it = std::lower_bound(T.begin(), T.end(), t); return it != T.end() && *it == t ? (int)(it - T.begin()) : -1; };
        std::vector<const bba::BoDec*> F; for (const auto& kv : bo) if (shownBo(kv.second)) F.push_back(&kv.second);
        // (1.6.0) the 14-bar Wilder ATR (rly::atrWilder's rule: tr[0] none, atr[1] = tr[1], then (13 * atr + tr) / 14) per bar
        std::vector<double> atr((size_t)n, NAN); { double y = 0; bool have = false;
            for (int k = 1; k < n; ++k) { const In& b = bars[(size_t)k]; const In& a = bars[(size_t)k - 1];
                if (!b.ok || !a.ok || !std::isfinite(b.h) || !std::isfinite(b.l) || !std::isfinite(a.c)) { atr[(size_t)k] = have ? y : NAN; continue; }
                const double tr = std::max(b.h - b.l, std::max(std::fabs(b.h - a.c), std::fabs(b.l - a.c)));
                y = have ? y + (tr - y) / 14.0 : tr; have = true; atr[(size_t)k] = y; } }   // t0 order (map); (1.5.0) a BO / BD a BI cancelled is not one
        for (size_t q = 0; q < F.size(); ++q) {
            const bba::BoDec& D = *F[q]; const int i = idxOf(D.t0); if (i < 0) continue;
            const In& c = bars[(size_t)i]; if (!std::isfinite(c.h) || !std::isfinite(c.l)) continue;
            const int dir = D.dir > 0 ? 1 : -1, want = dir > 0 ? -1 : 1;    // BO: pivot LOWs, BD: pivot HIGHs
            const double A = atr[(size_t)i]; if (!std::isfinite(A) || !(A > 0) || !std::isfinite(D.piv)) continue;   // (1.6.0) the ATR at the BO / BD candle
            const long long tf = D.t;                                         // the BO / BD's fire bar (follow-through closed)
            // the wave's origin: the last pivot of the wanted type before the BO / BD candle, known when the BO / BD fired
            bool haveOrg = false; double org = 0;
            for (const Pivot& p : piv) { if (p.t >= D.t0) break; if (p.type == want && p.tc <= tf) { haveOrg = true; org = dir > 0 ? p.l : p.h; } }
            int scan = i;                                                     // bars (i, scan] checked against the origin
            const auto p0 = std::upper_bound(piv.begin(), piv.end(), D.t0, [](long long t, const Pivot& p) { return t < p.t; });
            for (auto it = p0; it != piv.end(); ++it) {
                const Pivot& p = *it; const long long td = std::max(p.tc, tf);
                // the next BO / BD known by the decision bar, with its candle in (t0, tp]: this pivot and every later one are past it
                // (td and tp only grow along the pivots, so every later pivot is blocked too: stop)
                bool blocked = false;
                for (size_t r = q + 1; r < F.size(); ++r) { const bba::BoDec& X = *F[r]; if (X.t0 > p.t) break; if (X.t <= td) { blocked = true; break; } }
                if (blocked) break;
                // price traded beyond the origin on a bar in (i, p]: stop (no Pb on or after that)
                bool broke = false; const int pi = idxOf(p.t); if (pi < 0) break;
                if (haveOrg) for (int k = scan + 1; k <= pi; ++k) { const In& b = bars[(size_t)k]; if (b.ok && (dir > 0 ? b.l < org : b.h > org)) { broke = true; break; } scan = k; }
                if (broke) break;
                if (p.type != want) continue;
                // (1.6.0) the pullback comes back to the broken pivot or stops less than ATR / 3 short of it (was: overlaps the BO / BD candle)
                if (!(dir > 0 ? p.l <= D.piv + PB_ATR_FRAC * A : p.h >= D.piv - PB_ATR_FRAC * A)) continue;
                if (pb.count(p.t)) continue;                                  // decided before: never again
                PbMark m; m.tp = p.t; m.t0 = D.t0; m.tc = p.tc; m.td = td; m.dir = dir; pb[p.t] = m;
                RecL L; const int di = idxOf(td); L.sess = di >= 0 && bars[(size_t)di].ok ? sessOf(bars[(size_t)di].days, bars[(size_t)di].endSec) : -1;   // (1.3.0) the decision bar's day
                L.line = pbLine(m, D, p, c, recVer()); pbL[p.t] = L;
            }
        }
        while (pb.size() > 40000) { pbL.erase(pb.begin()->first); pb.erase(pb.begin()); }
    }
};

// ---- layout: what the host draws (one text per letter; the host adds the 1 px black edge as lsBigBidAsk did) ---------------------
struct Box { int l = 0, t = 0, r = 0, b = 0; };
inline bool hits(const Box& a, const Box& b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
inline bool inside(const Box& a, const Box& in) { return a.l >= in.l && a.r <= in.r && a.t >= in.t && a.b <= in.b; }
struct Txt { std::string s; Box f; unsigned long col = 0; int font = 0; int bar = 0; int dir = 0; int kind = 0; int row = 0; };   // kind 0 BO / BD, 1 Rly, 2 Pb (1.1.0), 3 BI (1.5.0; row = rows out from the wick)
struct View {
    int first = 0, last = -1;                       // visible chart bars (inclusive)
    Box area;                                       // where letters may go (price area, below the IRT title row)
    std::vector<Box> keepOut;                       // other indicators' boxes (Tape Reader banner, HOD/LOD table)
    std::function<int(int)> X;                      // bar -> centre x
    std::function<int(double)> Y;                   // price -> y
    std::function<int(const char*, int)> textW;     // text, font -> px
    int lh[2] = { 12, 14 };                         // letter row heights: BO / BD font, Rly font
};
struct Board { long long t = 0; int side = 0; int val = 0; };   // SignalBoard 'L': bar time, +1 above / -1 below, px from the extreme to the outer edge
struct Scene { std::vector<Txt> txt; std::vector<Board> board; int boShown = 0, rlyShown = 0, pbShown = 0, biShown = 0, noRoom = 0; };
// (1.5.0) the BI rows: row r sits GAP + r * ROW_OUT px from the wick (0 = the BO row +3..+17, 1 = +19..+33, 2 = +35 (Pb), 3 = +51 (LVP),
// 4 = +67, 5 = +83). Rows taken by OTHER indicators' labels at this candle and side: a zigzag pivot (the wave-time label, row 0, and
// lsCandleAbsorb's A / Ae, row 1); the candle after a pivot that re-tests it within 4 ticks (lsCandleAbsorb's A row 1); row 3 is
// left to the LVP label always. Our own letters are checked box by box.
static const int BI_ROWS = 6, BI_RETEST_TICKS = 4;
inline unsigned biReserved(const Engine& E, int bar, int side) {   // side +1 above the high / -1 below the low; bit r = row r taken
    unsigned m = 1u << 3; const int n = (int)E.bars.size(); if (bar < 0 || bar >= n) return m;
    const long long t = E.bars[(size_t)bar].t;
    auto isPiv = [&](long long tt) {
        if (E.curExtType == side && E.curExtT == tt) return true;
        const auto it = std::lower_bound(E.piv.begin(), E.piv.end(), tt, [](const Pivot& p, long long x) { return p.t < x; });
        for (auto q = it; q != E.piv.end() && q->t == tt; ++q) if (q->type == side) return true;
        return false; };
    if (isPiv(t)) m |= 3u;
    if (bar >= 1 && isPiv(E.bars[(size_t)(bar - 1)].t)) {
        const In& p = E.bars[(size_t)(bar - 1)]; const In& b = E.bars[(size_t)bar]; const double tol = BI_RETEST_TICKS * E.tick + 1e-9 * std::fabs(p.h);
        if (side > 0 ? b.h >= p.h - tol : b.l <= p.l + tol) m |= 2u;
    }
    return m;
}

inline Scene layout(const Engine& E, const View& v) {
    Scene S; const int n = (int)E.bars.size(); if (n < 2 || !v.X || !v.Y || !v.textW) return S;
    std::map<long long, int> idx; const int a = std::max(0, v.first), z = std::min(v.last, n - 2);   // closed bars in view
    for (int i = a; i <= z; ++i) idx[E.bars[(size_t)i].t] = i;
    std::map<std::pair<int, int>, int> reach;       // (bar, side) -> outer edge in px from the wick end
    auto put = [&](int bar, int dir, int kind) {
        const In& b = E.bars[(size_t)bar]; if (!std::isfinite(b.h) || !std::isfinite(b.l)) return;
        const char* tx = kind == 2 ? "Pb" : kind == 1 ? "Rly" : (dir > 0 ? "BO" : "BD"); const int font = kind == 1 ? FONT_RLY : FONT_BO;
        const int w = v.textW(tx, font) + 2, cx = v.X(bar), yh = v.Y(b.h), yl = v.Y(b.l), top = std::min(yh, yl), bot = std::max(yh, yl);
        const int lh = v.lh[kind == 1 ? 1 : 0], off = kind == 1 ? GAP + std::max(RLY_ROW, v.lh[0] + 3) : kind == 2 ? PB_OFF : GAP;   // (1.1.1) Pb two rows out
        Txt T; T.s = tx; T.font = font; T.bar = bar; T.dir = dir; T.kind = kind; T.col = dir > 0 ? C_UP : C_DN;
        T.f.l = cx - w / 2; T.f.r = T.f.l + w;
        if (dir > 0) { T.f.b = top - off; T.f.t = T.f.b - lh; } else { T.f.t = bot + off; T.f.b = T.f.t + lh; }
        if (kind == 2)   // (1.1.0) Pb never on another letter: one row further out (lh + 1 px) until it is clear (1.1.1: starting PB_OFF out)
            for (int tries = 0; tries < 4; ++tries) {
                bool clash = false; for (const Txt& o : S.txt) if (hits(T.f, o.f)) { clash = true; break; }
                if (!clash) break;
                const int st = lh + 1; if (dir > 0) { T.f.t -= st; T.f.b -= st; } else { T.f.t += st; T.f.b += st; }
            }
        bool ok = inside(T.f, v.area); for (const Box& k : v.keepOut) if (k.r > k.l && k.b > k.t && hits(T.f, k)) ok = false;
        if (kind == 2) for (const Txt& o : S.txt) if (hits(T.f, o.f)) ok = false;
        if (!ok) { ++S.noRoom; return; }
        S.txt.push_back(T); if (kind == 2) ++S.pbShown; else if (kind == 1) ++S.rlyShown; else ++S.boShown;
        int& r = reach[std::make_pair(bar, dir > 0 ? 1 : -1)]; r = std::max(r, dir > 0 ? top - T.f.t : T.f.b - bot);
    };
    std::vector<std::pair<int, int>> boBars;
    for (const auto& kv : E.bo) { const bba::BoDec& D = kv.second; if (!E.shownBo(D)) continue;   /* (1.5.0) not a BO / BD a BI cancelled */ const auto it = idx.find(D.t0); if (it == idx.end()) continue; put(it->second, D.dir, 0); boBars.push_back({ it->second, D.dir > 0 ? 1 : -1 }); }
    for (const auto& kv : E.rly) { const RlyMark& R = kv.second; if (!R.fired) continue; const auto it = idx.find(R.te); if (it == idx.end()) continue; put(it->second, R.dir, 1); }
    // (1.1.0) Pb: green below a pivot LOW after a BO (dir +1 -> drawn on the LOW side), red above a pivot HIGH after a BD
    for (const auto& kv : E.pb) { const PbMark& P = kv.second; const auto it = idx.find(P.tp); if (it == idx.end()) continue;
        const size_t before = S.txt.size(); put(it->second, -P.dir, 2);
        if (S.txt.size() > before) { Txt& T = S.txt.back(); T.dir = P.dir; T.col = P.dir > 0 ? C_UP : C_DN; boBars.push_back({ it->second, -P.dir > 0 ? 1 : -1 }); } }
    // (1.5.0) BI: "BI" red just above the high (BI DOWN) / green just below the low (BI UP), the BO row first, one row out (16 px) while
    // the row is taken (another indicator's label - biReserved - or one of our letters); one "BI" per candle and side; no room = dropped
    {   std::set<std::pair<int, int>> done;
        for (const auto& kv : E.bi) { const BiMark& M = kv.second; const auto it = idx.find(M.t); if (it == idx.end()) continue;
            const int bar = it->second, side = M.dir < 0 ? 1 : -1; if (!done.insert(std::make_pair(bar, side)).second) continue;
            const In& b = E.bars[(size_t)bar]; if (!std::isfinite(b.h) || !std::isfinite(b.l)) continue;
            const char* tx = "BI"; const int w = v.textW(tx, FONT_BO) + 2, cx = v.X(bar), yh = v.Y(b.h), yl = v.Y(b.l), top = std::min(yh, yl), bot = std::max(yh, yl), lh = v.lh[0];
            const unsigned res = biReserved(E, bar, side); bool placed = false;
            for (int r = 0; r < BI_ROWS && !placed; ++r) {
                if (res & (1u << r)) continue;
                Txt T; T.s = tx; T.font = FONT_BO; T.bar = bar; T.dir = M.dir; T.kind = 3; T.row = r; T.col = M.dir > 0 ? C_UP : C_DN;
                T.f.l = cx - w / 2; T.f.r = T.f.l + w; const int off = GAP + r * ROW_OUT;
                if (side > 0) { T.f.b = top - off; T.f.t = T.f.b - lh; } else { T.f.t = bot + off; T.f.b = T.f.t + lh; }
                bool clash = false; for (const Txt& o : S.txt) if (hits(T.f, o.f)) { clash = true; break; }
                if (clash) continue;
                bool ok = inside(T.f, v.area); for (const Box& k : v.keepOut) if (k.r > k.l && k.b > k.t && hits(T.f, k)) ok = false;
                if (!ok) break;                                                       // off the price area / under a box: further out is worse
                S.txt.push_back(T); ++S.biShown; placed = true;
                int& rr = reach[std::make_pair(bar, side)]; rr = std::max(rr, side > 0 ? top - T.f.t : T.f.b - bot); boBars.push_back({ bar, side });
            }
            if (!placed) ++S.noRoom; } }
    std::sort(boBars.begin(), boBars.end()); boBars.erase(std::unique(boBars.begin(), boBars.end()), boBars.end());   // (1.1.0) one post per bar and side
    for (const auto& p : boBars) { const auto it = reach.find(p); if (it == reach.end()) continue; Board B; B.t = E.bars[(size_t)p.first].t; B.side = p.second; B.val = it->second; S.board.push_back(B); }
    return S;
}

// the status file body (version first; counts of today's decisions by the last closed bar's CT day)
inline std::string statusText(const Engine& E, const std::string& mkt, const std::string& sym, int spb, const Scene& last) {
    long today = 0; if (E.bars.size() >= 2) today = E.bars[E.bars.size() - 2].days;
    int bo = 0, bd = 0, nf = 0, ru = 0, rd = 0;
    std::map<long long, long> dayOf; for (const In& b : E.bars) dayOf[b.t] = b.days;
    int bc = 0;   // (1.5.0) BO / BD cancelled by a BI
    for (const auto& kv : E.bo) { const auto it = dayOf.find(kv.first); if (it == dayOf.end() || it->second != today) continue; if (E.shownBo(kv.second)) (kv.second.dir > 0 ? bo : bd)++; else { ++nf; if (kv.second.fired) ++bc; } }
    int iu = 0, idn = 0; for (const auto& kv : E.bi) { const auto it = dayOf.find(kv.second.t); if (it == dayOf.end() || it->second != today) continue; (kv.second.dir > 0 ? iu : idn)++; }
    for (const auto& kv : E.rly) { const auto it = dayOf.find(kv.second.te); if (it == dayOf.end() || it->second != today || !kv.second.fired) continue; (kv.second.dir > 0 ? ru : rd)++; }
    int pu = 0, pd = 0; for (const auto& kv : E.pb) { const auto it = dayOf.find(kv.first); if (it == dayOf.end() || it->second != today) continue; (kv.second.dir > 0 ? pu : pd)++; }   // (1.1.0)
    std::string s = std::string("VERSION,") + VERSION + "\nMARKET," + mkt + "\nSYMBOL," + sym + "\nSPB," + std::to_string(spb) + "\n";
    s += "RULES,BO / BD = lsBigBidAsk " + std::string(bba::VERSION) + " bba::boFromCandles (rule " + std::to_string(bba::BO_RULE) + ", follow-through in the same wave) on the " + std::to_string(E.zzRev) + "-bar High/Low ZigZag; Rly = RallyLogic " + rly::VERSION + " on the same ZigZag" + (spb == 180 ? "" : " (3-min charts only: off on this chart)") + "; Pb = a confirmed 3-bar High/Low ZigZag pivot back at the broken pivot or < ATR / 3 short of it (14-bar ATR at the BO / BD candle; until the next BO / BD or the wave origin breaks)\n";
    s += "TODAY,BO " + std::to_string(bo) + ",BD " + std::to_string(bd) + ",not fired " + std::to_string(nf) + ",Rly up " + std::to_string(ru) + ",Rly down " + std::to_string(rd) + ",Pb after BO " + std::to_string(pu) + ",Pb after BD " + std::to_string(pd) + "\n";
    if (SHOW_BI) s += "BI,bi_shown 1,label BI (plain - no ?),window " + std::to_string(BI_N) + " candles,range >= avg of " + std::to_string(BI_AVG) + " + (volume or range up),close back past the pivot,one per pivot,cancel waiting BO " + (E.biCancel ? "1" : "0")
       + ",today BI down " + std::to_string(idn) + ",BI up " + std::to_string(iu) + ",BO / BD cancelled " + std::to_string(bc) + ",kept " + std::to_string(E.bi.size()) + ",cancelled kept " + std::to_string(E.boCancel.size()) + ",attempts last pass " + std::to_string(E.biAtt) + ",drawn " + std::to_string(last.biShown) + "\n";   // (1.5.0)
    if (!SHOW_BI) s += "BI,removed (1.6.0, Rassul 2026-11-04): no BI letters, no BBI records, no BO / BD cancelled\n";
    s += "KEPT,breakout decisions " + std::to_string(E.bo.size()) + ",rally waves " + std::to_string(E.rly.size()) + ",Pb " + std::to_string(E.pb.size()) + ",pivots " + std::to_string(E.piv.size()) + ",duplicates skipped " + std::to_string(E.boDup) + ",runs " + std::to_string(E.runs) + " (full " + std::to_string(E.fullRuns) + ")\n";
    s += "DRAWN,BO/BD " + std::to_string(last.boShown) + ",Rly " + std::to_string(last.rlyShown) + ",Pb " + std::to_string(last.pbShown) + ",no room " + std::to_string(last.noRoom) + "\n";
    return s;
}

// ---- (1.3.0) RECORDS (the host reads / writes the files; everything that decides WHAT is written lives here and is tested) ------------
// lsBigBidAsk's file names: marketName (root -> ES / NQ / GC / CL / HG / NG / EU) and fileKey (the contract), copied from BigBidAsk.cpp
inline std::string recFileKey(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size() && out.size() < 24; ++i) { const unsigned char c = (unsigned char)s[i];
        out.push_back((std::isalnum(c) || c == '-' || c == '_') ? (char)std::toupper(c) : '_'); }
    return out.empty() ? "UNKNOWN" : out;
}
inline std::string recMarket(std::string r) {
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::toupper((unsigned char)r[i]);
    const auto has = [&r](const char* p) { return r.find(p) != std::string::npos; };
    if (has("NQ")) return "NQ";
    if (has("GC")) return "GC";
    if (has("CL") || r == "QM") return "CL";
    if (has("HG") || has("CP")) return "HG";
    if (has("NG") || r == "QG") return "NG";
    if (has("EU") || has("6E") || has("E6")) return "EU";
    if (has("ES") || has("EP")) return "ES";
    return recFileKey(r.substr(0, 8));
}
static const int REC_SESSIONS = 3;                 // the last 3 sessions (daykeys) are written; older decisions stay in memory only
enum { REC_BO = 0, REC_RLY = 1, REC_PB = 2 };
// the dedupe key of a record line ("" = not a record line: the header, a comment, garbage)
inline std::string recKey(const std::string& line0) {
    std::string line = line0; while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    std::vector<std::string> f; { size_t a = 0; for (;;) { const size_t q = line.find('|', a); f.push_back(line.substr(a, q == std::string::npos ? std::string::npos : q - a)); if (q == std::string::npos) break; a = q + 1; } }
    auto num = [](const std::string& x) { if (x.empty() || x.size() > 20) return false; size_t k = x[0] == '-' ? 1 : 0; if (k >= x.size()) return false; for (; k < x.size(); ++k) if (x[k] < '0' || x[k] > '9') return false; return true; };
    if (f[0] == "BBO" && f.size() == 27 && num(f[2]) && num(f[3])) return "B|" + f[2] + "|" + f[3];     // t0 + dir
    if (f[0] == "RLY" && f.size() == 17 && num(f[1]) && num(f[4])) return "R|" + f[1] + "|" + f[4];     // t_e + dir
    if (f[0] == "BPB" && f.size() == 14 && num(f[1]) && num(f[2])) return "P|" + f[1] + "|" + f[2];     // t_pivot + side
    if (f[0] == "BBI" && f.size() == 22 && num(f[1]) && num(f[2]) && !f[7].empty() && f[7].size() <= 24) return "I|" + f[1] + "|" + f[2] + "|" + f[7];   // (1.5.0) t_bi + dir + pivot
    return std::string();
}
// one file's worth of lines: dir = the folder under lsFlexLevels, file = the file name, head = what a NEW file starts with
struct RecFile { int kind = 0; long long sess = 0; std::string dir, file, head; std::vector<std::string> lines; };
inline std::string recHead(const Engine& E, int kind, const std::string& mkt, const std::string& sym, int spb) {
    std::string h = std::string("# lsBreakout ") + VERSION + " | " + mkt + " " + sym + " " + std::to_string(spb) + " | zz_rev_bars " + std::to_string(E.zzRev)
                  + " | ft_tie " + (E.tieOn() ? "1" : "0") + " | zz_carry " + (E.carryOn() ? "1" : "0") + (kind == REC_BO ? (SHOW_BI ? " | bi 1" : " | bi 0") : "") + " | " + (kind == REC_BO ? "BO / BD decisions (bba::boLine, 27 fields) + BI (BBI|, 22 fields)" : kind == REC_RLY ? "Rly waves (rly::line, 17 fields)" : "Pb (14 fields)") + "\n";
    return h + (kind == REC_BO ? bba::BO_HEADER : kind == REC_RLY ? rly::REC_HEADER : PB_HEADER) + "\n" + (kind == REC_BO && SHOW_BI ? std::string("# ") + BI_HEADER + "\n" : std::string());   /* (1.6.0) no BBI header while BI is removed */   // (1.5.0) the BI columns as a comment line
}
// every record line of the last REC_SESSIONS sessions (by the last closed bar's session), grouped per file, in decision-key order
inline std::vector<RecFile> recPlan(const Engine& E, const std::string& mkt, const std::string& sym, int spb) {
    std::vector<RecFile> out; if (E.bars.size() < 2) return out;
    const In& lc = E.bars[E.bars.size() - 2]; if (!lc.ok) return out;
    const long long minS = sessOf(lc.days, lc.endSec) - (REC_SESSIONS - 1);
    const std::string stem = mkt + "-" + sym + "-" + std::to_string(spb);
    std::map<std::pair<int, long long>, size_t> at;
    auto add = [&](int kind, const RecL& L) {
        if (L.sess < minS || L.line.empty()) return;
        const auto k = std::make_pair(kind, L.sess); auto it = at.find(k);
        if (it == at.end()) { RecFile F; F.kind = kind; F.sess = L.sess; F.head = recHead(E, kind, mkt, sym, spb);
            F.dir = kind == REC_RLY ? BKO_REC_DIR BKO_REC_SEP "Rly" : BKO_REC_DIR;   // (1.6.1) Breakout1 for lsBreakout1
            F.file = stem + (kind == REC_BO ? "-breakouts-" : kind == REC_PB ? "-pb-" : "-") + std::to_string(L.sess) + ".csv";
            it = at.emplace(k, out.size()).first; out.push_back(F); }
        out[it->second].lines.push_back(L.line); };
    for (const auto& kv : E.boL) add(REC_BO, kv.second);
    for (const auto& kv : E.rlyL) add(REC_RLY, kv.second);
    for (const auto& kv : E.pbL) add(REC_PB, kv.second);
    for (const auto& kv : E.biL) add(REC_BO, kv.second);   // (1.5.0) BBI| lines go to the breakouts file
    return out;
}
// the merge: old = the file as it is now ("" = no file yet). Returns the new text = old (or head) + every line whose key is NOT already in
// the file (nor earlier in `lines`); inFile gets every key now in the file text returned; added = lines appended (0 = nothing to write).
inline std::string recMerge(const std::string& old, const std::string& head, const std::vector<std::string>& lines, std::set<std::string>& inFile, int& added) {
    added = 0; inFile.clear();
    { size_t a = 0; while (a < old.size()) { size_t q = old.find('\n', a); if (q == std::string::npos) q = old.size();
        const std::string k = recKey(old.substr(a, q - a)); if (!k.empty()) inFile.insert(k); a = q + 1; } }
    std::string text = old.empty() ? head : old; if (!text.empty() && text.back() != '\n') text += "\n";
    for (const std::string& l : lines) { const std::string k = recKey(l); if (k.empty() || !inFile.insert(k).second) continue; text += l + "\n"; ++added; }
    return text;
}

} // namespace bko
#endif
