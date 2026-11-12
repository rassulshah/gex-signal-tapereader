// ZzoCobFinder.h - the CoB finder of lsZzoCombo 2.4.0 (2026-11-12), SDK-free. Used by ZzoCombo.cpp / ZzoComboView.h only.
//
// Rassul 2026-11-12: "the wave zzo would be upgraded to show the waves with count labels but also identify cobs" / "cob should be near
// the extremes, near the hod or lod for the Rth session" / "basically a sweep of a key level first" / "make sure you have the zzo count
// like we do today and now you can add the lvp" / "a cob is a strong wave that typically leads to change from a down trend to an
// uptrend and vice versa" / 08:56 "we are just upgrading the zzo wave ... do not change anything beyond the indicator".
//
// RULE (bullish; bearish = mirror), decided ONCE on the bar that completes the wave (the 3-bar reversal), never repainted:
//   1. RTH: the wave's origin bar and its completing bar are RTH bars of the same session (svl::rthOf hours of the market).
//   2. Near the extreme: the origin low is at or below the HIGH of the bar that made the RTH low so far (his LOD-bar rule; the RTH low
//      itself qualifies).
//   3. Key level first: the origin went below a key low (ONL, AsiaLO, LonLO, PhrL, PFL, PDL) or the prior day's POC / VAH / VAL, the wave came back over it and the completing
//      bar CLOSED back above it. (PDL = the prior session's RTH low.)
//   4. A strong wave: ZZO count >= 10 (the pane's own count) and the completing bar has given back <= 0.46 of the wave. It does NOT have
//      to take out the prior pivot (Rassul 09:07 "it doesnt necessarily need to be the wave that breaks the downtrend, it only
//      kickstarts it"; measured: the break adds 2 pts of held and nothing on the 1.33 rule) - whether it did is logged (bo).
//   THE COUNT: the pane's own count (what is drawn) AND the count of the same wave with the market's RTH split (zzd::Engine M below)
//   must both be >= 10. On charts whose root has no session hours in the engine (QGC / QHG / QCL / NGX6 / EU6) the pane's count has
//   no overnight / RTH split and runs high: Cobs passing on the pane's count alone held 53-60% (GC / HG / CL / NG, 400 sessions)
//   against 66-85% for those passing both. On ES / NQ the two counts are the same number.
//   Lvp: among the first 3 pullback waves after a Cob that stay inside the Cob wave (none below its origin, no wave above its top), the
//   first with <= 1/3 of the Cob wave's volume; tagged on that pullback wave when it completes.
// MEASURED (lra study, 3-min bars 2025-04-28 .. 2026-11-11, 7 markets, ~2,700 market-sessions): 0.44 Cobs / session, the origin held to
//   the RTH close 71% (thirds 73 / 70 / 69), reached the 1.33 extension before a close beyond the origin 62%; with an Lvp 79% / 68%,
//   without one 49% / 49%; no key level 63% / 56%; Cobs that qualify ONLY on DPoc / VAH / VAL 76% / 67% (131);
//   his RTL Cob signal on the same bars 53% / 54%. About break-even after costs at the completing bar's close: a read, not an entry.
// Levels here are the finder's own (bars only): overnight = bar END in (17:00, RTH open], Asia (18:00, 01:00], London (02:00,
//   min(07:30, RTH open)], prior hour (open - 60 min, open], prior full session, prior session's RTH. The prior week does not qualify (56% reached 1.33).
// (1.2.0, lsZzoCombo 2.4.0, Rassul 2026-11-12 11:25 "Cob2 is fine" / 11:27 "I want a read box on the left of the indicator pane" / 11:32
//   "usually when a cob occurs it turns the hourly as well" / 11:38 "in the Lvl you can add SR+ in the beginning" / 11:40 "cob's can have
//   deep retracements" / 11:42 "there should be no requirement for giveback because cob can test the origin bar": the 0.46 limit of
//   rule 4 is REMOVED for the Cob (it stays the Cob2 limit). Measured: the 193 Cobs it adds (retraced > 0.46 at the completing bar)
//   held 38% (58% with an Lvp, 26% without) against 70% for the rest - shown by the read box's Hold row, HOLD_DEEP):
//   COB2 (the quiet turn; a wave that passed rules 1 and 2 and the 0.46 limit and is NOT a Cob - no count, no key level needed):
//     a. within 2 bars of the bar that made the RTH low (the pivot of the down-wave that made it) a bar closed above that bar's high,
//     b. the completing bar closes above the high of the 10 bars before that RTH-low bar,
//     c. the completing bar's close is >= 5 bar ranges above the RTH low (range = the median of the 20 bars before the RTH-low bar),
//     d. the completing bar is at least 60 minutes after the RTH open.
//     MEASURED (same history): 525 Cob2 that are not a Cob (0.19 / session), held 76% (thirds 72 / 82 / 73), reached 1.33 66%.
//   HR (the hourly turn, decided ONCE on the completing bar): its close is above the high of the clock hour BEFORE the hour of the
//     origin (below the low for a bearish wave). Cobs: held 80% with it (526), 61% without (577); it adds nothing to a Cob2 (76 / 77).
//   SR+ = a key level was swept and closed back over (rule 3); every Cob has it, a Cob2 only when it happened.
//   HOLD (the read box): held-to-the-RTH-close rate of the state, from the same history - HOLD_COB / HOLD_COB2 below
//     [Hr no / yes][Lvp still possible / Lvp printed / no Lvp].
// (1.3.1, lsZzoCombo 2.5.1, Rassul 2026-11-12 11:45 "score the quality of the bars in the cob ... the number of consecutive green bars,
//   the number of big green bars that are above the atr, the volume of the bars", 12:10 "you should also check for high end closes",
//   12:26 "get rid of count because i already see it on the wave, and keep Q and label it like 4/6. also the 8ATR or 14 or more candles
//   are excessive. that many bars and that much length could exhaust and makes it impractical to get in on a pullback ... even 5 or more
//   up candles bigger than ATR is excessive"): Q = the QUALITY of the wave's own bars (origin + 1 .. the top), 0..6, one point each, none
//   of them about the wave's size or length (bullish; bearish = mirror):
//     1. no dip inside the wave deeper than 0.30 of the wave          2. 4 or more up bars closing in their top third
//     3. 4 or more closes above the prior bar's high                  4. 2 or more of the first 3 bars close in their top third
//     5. efficient: the wave is at least half the sum of its bars' ranges      6. a run of 4 or more up bars
//   MEASURED (1,296 Cobs, same history): Q 4-6 held 78%, reached 1.33 65% (last third 75%); Q 0-2 48% / 47%. On waves under 14 bars
//   and under 8 ATR: Q 5-6 held 78%, reached 1.33 75% (81); Q 0-2 42%. (The 2.5.0 score - 14+ bars, 8+ ATR, 5+ big up bars - held more
//   often, 85%, but rewarded size; replaced on his instruction. Big waves did not exhaust in the data: 11+ ATR held 87%.)
//   Volume per bar, the share of up bars and the wick at the top did not separate. Q is logged on every C / Q line and drives Hold.
// (1.4.0, lsZzoCombo 2.6.0, Rassul 2026-11-12 12:43 "a cob bar, basically a big range bar or a heavy volume bar that is the first green bar
//   or close to it because its low is less than the high of the dn pivot bar high ... I want you to label it in the cob", 12:47 "I'll go
//   with your recommendation" = label and log, no quality point): CB = the COB BAR of a Cob / Cob2 - the first of the wave's first two up
//   bars (within 4 bars of the origin) whose low is below the origin bar's high and whose range is >= 1.5 ATR(14) or whose volume is
//   >= 1.5x the median of the 20 bars before the origin. Bearish = mirror. Logged at the end of every C / Q line (its bar time, 0 = none).
//   MEASURED: of 18,177 waves near the RTH extreme, 11.5% of those with a Cob bar are Cobs against 4.2% without (65% of Cobs have one,
//   40% of all waves) - it helps a wave BECOME a Cob; among Cobs it adds nothing (held 62% with, 70% without), so it scores no point.
// (1.5.0, lsZzoCombo 2.7.0, Rassul 2026-11-12 12:51 "usually the cob [bar] is the biggest green bar or the most green bars volume in over
//   35+ or even 99 green bars", 12:53 "you can add the stop run badge too like you showed me in the mock up"; his picks: CB and CB35 / CB99,
//   SR in the pane and on the price chart):
//   CB RANK = how many of the up bars straight before the Cob bar it beats on range, or on volume (the larger of the two; capped at 150).
//     The Cob bar is now the first of the wave's first two up bars (within 4 bars of the origin, low under the origin bar's high) that
//     has a rank of 35+ (1.5.1: the rank alone; the 1.5x rule of 1.4.0 no longer qualifies). Drawn "CB35" (rank 35-98) or "CB99" (99+).
//     MEASURED (18,177 waves near the extreme): rank 99+ -> 18.2% become a Cob, 35-98 -> 14%, under 10 -> 5.5%. Among Cobs it does not
//     help (rank 35+: held 63%, reached 1.33 51% vs 66% / 59%) - a label, never a quality point.
//   SR = the STOP RUN bar of a Cob / Cob2 that swept a key level: the first bar of the wave INTO the origin that traded through the
//     deepest key level swept (the lowest for a bullish Cob). Bars only (rule 3's sweep), not an order-flow stop run.
//   C / Q lines end with: ... | Hr | Q | Cob bar time | CB rank | SR bar time | SR level.
#pragma once
#include "ZzoDeltaLogic.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace zcf {
static const char* const COMBO_VERSION = "2.8.1";   // (2.8.1) the box shows Q as a percentage ("67% / 0.6"); (2.8.0) + the day range used: box row "Q / Rng", Hold adjusted by it; (2.7.3) a Cob bar must be the biggest / heaviest of 35+ up bars; (2.7.2) CB / SR on the price pane only; (2.7.1) price-pane SR under the bar; (2.7.0) + CB35 / CB99, the SR badge; (2.6.0) + the Cob bar label CB; (2.5.1) Q without size / length, box row "Q 4/6"; (2.5.0) + the quality score Q; (2.4.0) + Cob2, the hourly turn, the read box   // lsZzoCombo (the setVersion literal in ZzoCombo.cpp)
static const char* const VERSION = "1.6.0";         // the finder rule (1.6.0: + day range used, Hold by range, the stop-run level's price on the record line; 1.5.1: CB needs rank 35+; 1.5.0: + CB rank, SR bar; 1.4.0: + CB, the Cob bar; 1.3.1: Q without size / length; 1.3.0: + Q; 1.2.0: + Cob2, Hr; 1.1.0: + the prior full session's POC / VAH / VAL)
static const int MIN_CNT = 10;                       // ZZO count ("zzo 10+", as his CoB filter today)
static const int WATCH_CNT = 5;                      // logged only (challenger)
static const double MAX_RETR = 0.46;                 // (1.2.0) no longer a Cob requirement: beyond it a Cob is "deep" (its own Hold rate); still the Cob2 limit
static const int HOLD_DEEP[2][3] = { { 31, 46, 25 }, { 49, 70, 29 } };   // a Cob that had retraced > 0.46 at the completing bar [Q 0-2, 3-6][Lvp pending, yes, none]
static const int LVP_WAVES = 3;                      // pullback waves that can carry the Lvp
static const double COB2_DIST = 5.0;                 // Cob2: bar ranges from the RTH extreme to the completing close
static const int COB2_MIN_OPEN = 60;                 // Cob2: minutes after the RTH open
static const int COB2_PRE = 10, COB2_MR = 20;        // Cob2: bars before the extreme bar (the high to close beyond; the median range)
static const int Q_MAX = 6, Q_HEC = 4, Q_ABOVE = 4, Q_FIRST = 2, Q_RUN = 4, ATR_N = 14;   // the six Q rules (the dip limit 0.30 and efficiency 0.5 are in quality())
// held % [Q band 0-2, 3, 4-6][Hr no, yes][Lvp pending, yes, none]; a cell under 20 cases takes its band's rate
static const int HOLD_COB[3][2][3] = { { { 49, 59, 35 }, { 72, 81, 54 } }, { { 62, 71, 43 }, { 73, 83, 40 } }, { { 71, 78, 47 }, { 84, 88, 65 } } };
static const int HOLD_COB2[3][3] = { { 71, 85, 58 }, { 72, 82, 50 }, { 78, 84, 63 } };   // [Q band][Lvp]; the hourly turn adds nothing to a Cob2
// (1.6.0, Rassul 2026-11-12 15:21 "Build all") the day range used at the completing bar = (RTH high - RTH low so far) / the median RTH
// range of the last ADR_N sessions (ADR_MIN needed). 400 sessions, 1,821 Cobs / Cob2: under 0.5 held 45%, 0.5-0.7 66%, 0.7-0.9 75%,
// 0.9-1.2 78%, 1.2+ 91%; against the Hold tables above that is HOLD_RNG points (the same in both halves of the days and in the Cob /
// Cob2 groups), added to the table rate and kept within 5..95.
static const int ADR_N = 10, ADR_MIN = 3;
static const int HOLD_RNG[5] = { -12, -3, 3, 5, 16 };  // range used x100: < 50, 50-69, 70-89, 90-119, 120+
inline int rngBand(int r100) { return r100 < 50 ? 0 : r100 < 70 ? 1 : r100 < 90 ? 2 : r100 < 120 ? 3 : 4; }
inline int holdWithRng(int hold, int r100) { if (r100 < 0) return hold; const int h = hold + HOLD_RNG[rngBand(r100)]; return h < 5 ? 5 : h > 95 ? 95 : h; }
static const int NLV = 9;                              // 0..5 = a high / low pair; 6..8 = one price, swept from either side
static const char* const LV_HI[NLV] = { "ONH", "AsiaHI", "LonHI", "PhrH", "PFH", "PDH", "DPoc", "VAH", "VAL" };
static const char* const LV_LO[NLV] = { "ONL", "AsiaLO", "LonLO", "PhrL", "PFL", "PDL", "DPoc", "VAH", "VAL" };

struct FBar { long long t = 0, h = 0, l = 0, c = 0, sess = 0; bool rth = false; long long o = 0; double atr = 0; long long v = 0; };   // (1.3.0) o = the open, atr = Wilder ATR(14) through this bar (used by CB), v = volume (1.4.0)
// (1.1.0, Rassul 2026-11-12 09:32 "Add prior day poc vah Val also") the prior FULL session's volume profile, as KeyLevelsLogic.h profileOf
// (1-tick rows; POC = the biggest row, a tie to the row nearest the middle then the lower; 70% value area by the CBOT two-row rule) -
// but built from BARS (each bar's volume spread evenly over its ticks), the only data the 400-session test had; it can sit a few
// ticks from the VAP-built DPoc / VAH / VAL lines of lsKeyLevels.
struct VA { bool ok = false; long long poc = 0, vah = 0, val = 0; };
inline VA profileOf(const std::map<long long, double>& rows, double vaPct = 0.70) {
    VA P; std::vector<long long> key; std::vector<double> vol; double tot = 0;
    for (const auto& kv : rows) if (kv.second > 0) { key.push_back(kv.first); vol.push_back(kv.second); tot += kv.second; }
    if (key.empty()) return P;
    const double mid = 0.5 * ((double)key.front() + (double)key.back()); size_t pi = 0;
    for (size_t i = 1; i < vol.size(); ++i) {
        if (vol[i] > vol[pi] + 1e-9) pi = i;
        else if (std::fabs(vol[i] - vol[pi]) <= 1e-9 && std::fabs((double)key[i] - mid) < std::fabs((double)key[pi] - mid) - 1e-9) pi = i;
    }
    size_t lo = pi, hi = pi; double cum = vol[pi]; const double need = vaPct * tot;
    while (cum + 1e-9 < need && (lo > 0 || hi + 1 < vol.size())) {
        double up = 0, dn = 0; size_t nu = 0, nd = 0;
        for (size_t j = hi + 1; j < vol.size() && nu < 2; ++j, ++nu) up += vol[j];
        for (size_t j = lo; j > 0 && nd < 2; --j, ++nd) dn += vol[j - 1];
        if (nu == 0) { lo -= nd; cum += dn; continue; }
        if (nd == 0) { hi += nu; cum += up; continue; }
        if (up > dn + 1e-9) { hi += nu; cum += up; }
        else if (dn > up + 1e-9) { lo -= nd; cum += dn; }
        else { hi += nu; lo -= nd; cum += up + dn; }
    }
    P.poc = key[pi]; P.vah = key[hi]; P.val = key[lo]; P.ok = true; return P;
}
struct HL { bool any = false; long long h = 0, l = 0; void add(long long hi, long long lo) { if (!any || hi > h) h = hi; if (!any || lo < l) l = lo; any = true; } };
// one decision, keyed by the wave it sits on (its pivot bar's END time and its direction)
struct Tag { int cob = 0;            // +1 / -1 = a Cob on this wave
             int lvp = 0;            // +1 / -1 = the Lvp of a bullish / bearish Cob, on this (pullback) wave
             int watch = 0;          // logged only: 1 = no key level, 2 = count 5..9
             int cob2 = 0;           // (1.2.0) +1 / -1 = a Cob2 on this wave (never together with cob)
             bool hr = false;        // (1.2.0) the hourly turn, decided on the completing bar
             long long tCb = 0;      // (1.4.0) the bar END time of the Cob bar (0 = none)
             int cbRank = 0;         // (1.5.0) up bars in a row before the Cob bar that it beats on range or volume
             long long tSr = 0; int srLv = -1; long long srPx = 0;   // (1.6.0) srPx = that level's price in ticks (0 = none)
               // (1.5.0) the stop-run bar (0 = none) and the level it ran (index into LV_LO / LV_HI)
             int q = -1;             // (1.3.0) the quality score 0..6 (-1 = not scored)
             int rng = -1;           // (1.6.0) day range used x100 at the completing bar (-1 = under ADR_MIN sessions of history)
             int lvpSt = 0;          // (1.2.0) of a Cob / Cob2: 0 = an Lvp can still come, 1 = it printed, 2 = none
             int lv = 0, cnt = 0; bool atExt = false; double retr = 0;
             long long tWave = 0, tCf = 0, origin = 0, top = 0, close = 0, tCob = 0; int dir = 0; };

inline std::string lvText(int mask, int dir) {
    std::string t; for (int i = 0; i < NLV; ++i) if (mask & (1 << i)) { if (!t.empty()) t += "/"; t += dir > 0 ? LV_LO[i] : LV_HI[i]; }
    return t.empty() ? "-" : t;
}

struct Read { bool any = false; int kind = 0, dir = 0, lv = 0, cnt = 0, lvpSt = 0, hold = 0, q = -1, rng = -1; bool hr = false; double retr = 0; long long tCf = 0; };
struct HrBar { long long id = 0, h = 0, l = 0; };

struct Finder {
    int openMin = -1, closeMin = -1; bool ok = false;
    std::vector<FBar> F;                                   // aligned with zzd::Engine::B
    zzd::Engine M;                                         // the same walk with the MARKET's session hours (its count only)
    bool haveSess = false; long long curSess = 0;
    HL on, as, lon, phr, full, rth, prevFull, prevRth;
    std::map<long long, double> prof; VA prevVA;           // this session's bar-spread profile; the prior session's POC / VAH / VAL
    static const long long MAX_BAR_TICKS = 20000;          // a bar wider than this (bad data) is not spread
    std::vector<long long> rthRng;                         // (1.6.0) the RTH ranges of the last ADR_N finished sessions
    std::vector<HrBar> hrs;                                // (1.2.0) the last clock hours (id = hour of the bar END - 1 s)
    bool rthAny = false; long long rLo = 0, rHi = 0, rLoBarH = 0, rHiBarL = 0;   // RTH extremes so far; the high of the LOD bar, the low of the HOD bar
    std::map<long long, Tag> tags;
    struct Act { long long key = 0, origin = 0, top = 0, vol = 0, sess = 0; int dir = 0, n = 0; };
    std::vector<Act> act;
    std::vector<std::string> out;                          // record lines not yet taken by the host

    static long long key(long long tWave, int dir) { return tWave * 2 + (dir > 0 ? 1 : 0); }
    void reset() { M.reset(); F.clear(); haveSess = false; on = as = lon = phr = full = rth = prevFull = prevRth = HL(); prof.clear(); prevVA = VA(); rthAny = false; tags.clear(); act.clear(); out.clear(); hrs.clear(); rthRng.clear(); }
    bool setMarket(const std::string& root, int spb = 180) { const std::string m = mkey::norm(root); ok = svl::rthOf(m.c_str(), openMin, closeMin);
        M.revBars = zzd::COMBO_REV_BARS; M.setMarket(m, spb); return ok; }
    // the trading session of a bar END stamp (local secs): bars after 17:00 belong to the next day
    static long long sessOf(long long t) { const long long days = zzd::floorDiv(t - 1, 86400); const long long sec = (t - 1) - days * 86400; return sec >= 17 * 3600 ? days + 1 : days; }

    // o = the bar's open (LLONG_MIN = not given: the previous close)
    void addBar(const zzd::Bar& eb, long long c, long long o = LLONG_MIN) {
        const long long t = eb.t, h = eb.h, l = eb.l;
        M.addBar(eb); M.trimmed = 0;                        // trims itself at the same bar counts as the pane's engine
        FBar b; b.t = t; b.h = h; b.l = l; b.c = c; b.sess = sessOf(t); b.v = eb.vol;
        b.o = o != LLONG_MIN ? o : (F.empty() ? c : F.back().c); if (b.o > h) b.o = h; if (b.o < l) b.o = l;
        if (F.empty()) b.atr = (double)(h - l);
        else { const long long pc = F.back().c; const double tr = (double)std::max(h - l, std::max(std::llabs(h - pc), std::llabs(l - pc))); b.atr = F.back().atr + (tr - F.back().atr) / (double)ATR_N; }
        const long long days = zzd::floorDiv(t, 86400); const int mn = (int)((t - days * 86400) / 60);
        if (!haveSess || b.sess != curSess) {
            if (haveSess) { prevFull = full; prevRth = rth; prevVA = profileOf(prof);
                if (rth.any) { rthRng.push_back(rth.h - rth.l); if ((int)rthRng.size() > ADR_N) rthRng.erase(rthRng.begin()); } }
            for (const Act& A : act) closeAct(A, 2);
            haveSess = true; curSess = b.sess; on = as = lon = phr = full = rth = HL(); prof.clear(); rthAny = false; act.clear();
        }
        full.add(h, l);
        { const long long id = zzd::floorDiv(t - 1, 3600);
          if (hrs.empty() || hrs.back().id != id) { HrBar x; x.id = id; x.h = h; x.l = l; hrs.push_back(x); if (hrs.size() > 96) hrs.erase(hrs.begin(), hrs.begin() + 48); }
          else { if (h > hrs.back().h) hrs.back().h = h; if (l < hrs.back().l) hrs.back().l = l; } }
        if (eb.vol > 0 && h >= l && h - l < MAX_BAR_TICKS) { const double v = (double)eb.vol / (double)(h - l + 1); for (long long p = l; p <= h; ++p) prof[p] += v; }
        if (ok) {
            if (mn > 1020 || mn <= openMin) on.add(h, l);
            if (mn > 1080 || mn <= 60) as.add(h, l);
            if (mn > 120 && mn <= std::min(450, openMin)) lon.add(h, l);
            if (mn > openMin - 60 && mn <= openMin) phr.add(h, l);
            b.rth = mn > openMin && mn <= closeMin;
            if (b.rth) {
                rth.add(h, l);
                if (!rthAny || l < rLo) { rLo = l; rLoBarH = h; }
                if (!rthAny || h > rHi) { rHi = h; rHiBarL = l; }
                rthAny = true;
            }
        }
        F.push_back(b);
    }
    // (1.6.0) the middle RTH range of the last sessions (0 = not enough history) and the day range used x100 now
    double adr() const { if ((int)rthRng.size() < ADR_MIN) return 0; std::vector<long long> v(rthRng); std::sort(v.begin(), v.end()); return (double)v[v.size() / 2]; }   // the upper middle value, as the study
    int rngUsed() const { const double a = adr(); if (!(a > 0) || !rthAny) return -1; const double r = 100.0 * (double)(rHi - rLo) / a; return r > 9999 ? 9999 : (int)(r + 0.5); }
    void trim(long long k) { if (k > 0) { const long long n = std::min<long long>(k, (long long)F.size()); F.erase(F.begin(), F.begin() + (size_t)n); } }
    void closeAct(const Act& A, int st) { auto it = tags.find(A.key); if (it != tags.end() && it->second.lvpSt == 0) it->second.lvpSt = st; }
    // (1.2.0) the hourly turn: the close against the clock hour before the hour of the origin bar
    bool hourTurn(long long tOrigin, int d, long long c0) const {
        const long long id = zzd::floorDiv(tOrigin - 1, 3600);
        for (size_t i = hrs.size(); i-- > 1;) if (hrs[i].id == id) return d > 0 ? c0 > hrs[i - 1].h : c0 < hrs[i - 1].l;
        return false;
    }
    // (1.3.0) Q: the quality of the wave's own bars io + 1 .. e (io = the origin bar, e = the top bar); -1 when it cannot be scored
    int quality(int d, int io, int e) const {
        if (io < 0 || e <= io || e >= (int)F.size()) return -1;
        auto Hm = [&](int i) { return d > 0 ? F[(size_t)i].h : -F[(size_t)i].l; }; auto Lm = [&](int i) { return d > 0 ? F[(size_t)i].l : -F[(size_t)i].h; };
        const long long leg = Hm(e) - Lm(io); if (leg <= 0) return -1;
        int run = 0, best = 0, hec = 0, above = 0, first = 0; long long mx = Hm(io + 1), dip = 0, sum = 0;
        for (int i = io + 1; i <= e; ++i) {
            const long long hh = Hm(i), ll = Lm(i), r = hh - ll, cc = (long long)d * F[(size_t)i].c; const bool up = (long long)d * (F[(size_t)i].c - F[(size_t)i].o) > 0;
            const bool hi = r > 0 && 100 * (cc - ll) >= 67 * r;                 // closes in the top third
            run = up ? run + 1 : 0; if (run > best) best = run;
            if (up && hi) ++hec;
            if (hi && i <= io + 3) ++first;
            if (cc > Hm(i - 1)) ++above;
            sum += r;
            if (hh > mx) mx = hh;
            if (mx - ll > dip) dip = mx - ll;
        }
        return (10 * dip < 3 * leg) + (hec >= Q_HEC) + (above >= Q_ABOVE) + (first >= Q_FIRST) + (2 * leg >= sum) + (best >= Q_RUN);
    }
    // (1.5.0) how many same-colour bars straight before bar j it beats on range, or on volume (the larger; capped at CB_RANK_CAP)
    static const int CB_RANK_CAP = 150, CB_MIN_RANK = 35;
    int cbRankOf(int d, int j) const {
        const FBar& b = F[(size_t)j]; const long long r = b.h - b.l; int nr = 0, nv = 0; bool openR = true, openV = true; int seen = 0;
        for (int i = j - 1; i >= 0 && seen < CB_RANK_CAP && (openR || openV); --i) {
            const FBar& x = F[(size_t)i]; if ((long long)d * (x.c - x.o) <= 0) continue;
            ++seen;
            if (openR) { if (x.h - x.l >= r) openR = false; else ++nr; }
            if (openV) { if (x.v >= b.v) openV = false; else ++nv; }
        }
        return std::max(nr, nv);
    }
    // (1.4.0) CB: the index of the Cob bar of the wave io + 1 .. e, or -1; (1.5.0) also by rank 35+, the rank returned in *rank
    int cobBar(int d, int io, int e, int* rank = nullptr) const {
        if (rank) *rank = 0;
        if (io < 10 || e <= io || e >= (int)F.size()) return -1;
        const long long ph = d > 0 ? F[(size_t)io].h : -F[(size_t)io].l; int greens = 0;
        for (int i = io + 1; i <= e && i <= io + 4 && greens < 2; ++i) {
            const FBar& b = F[(size_t)i]; if ((long long)d * (b.c - b.o) <= 0) continue;
            ++greens; const long long lo = d > 0 ? b.l : -b.h; if (!(lo < ph)) continue;
            const int rk = cbRankOf(d, i);
            // (1.5.1, Rassul 2026-11-12 13:04 "it really should be the biggest green bar in at least 35 green bars to even qualify") the rank
            // alone decides: the biggest (range) or the heaviest (volume) of the 35+ up bars before it. The 1.5x rule no longer qualifies.
            if (rk >= CB_MIN_RANK) { if (rank) *rank = rk; return i; }
        }
        return -1;
    }
    // (1.5.0) SR: the first bar of the wave into the origin (bars s0 .. io) that traded through the deepest swept level of the mask
    int stopRunBar(int d, int s0, int io, int mask, int* lvOut) const {
        if (lvOut) *lvOut = -1;
        if (!mask || io < 0 || io >= (int)F.size()) return -1;
        const HL* L[6] = { &on, &as, &lon, &phr, &prevFull, &prevRth }; const long long V[3] = { prevVA.poc, prevVA.vah, prevVA.val };
        int best = -1, bestLv = -1; long long bestP = 0;
        for (int i = 0; i < NLV; ++i) {
            if (!(mask & (1 << i))) continue;
            const long long p = i < 6 ? (d > 0 ? L[i]->l : L[i]->h) : V[i - 6];
            if (best >= 0 && (long long)d * (p - bestP) >= 0) continue;                      // keep the deepest (lowest for a low)
            for (int j = std::max(0, s0); j <= io; ++j) { const long long x = d > 0 ? F[(size_t)j].l : F[(size_t)j].h; if ((long long)d * (p - x) > 0) { best = j; bestLv = i; bestP = p; break; } }
        }
        if (lvOut) *lvOut = bestLv;
        return best;
    }
    // (1.2.0) Cob2 rules a..d (the caller has checked RTH, near the extreme and the 0.46 limit); j = the completing bar
    // The "bar that made the RTH low" = the pivot bar of the nearest of the last 5 down-waves whose low IS the RTH low so far (as
    // tested; an RTH low that is not such a pivot was not in the test and gets no Cob2).
    bool cob2Test(int d, int j, const std::vector<zzd::Wave>& W) const {
        const FBar& cf = F[(size_t)j]; long long ix = -1; const long long ext = d > 0 ? rLo : rHi;
        for (int k = 1; k <= 9 && k < (int)W.size(); k += 2) {
            const zzd::Wave& x = W[W.size() - 1 - (size_t)k];
            if (x.dir != -d || x.e < 0 || x.e > j) break;
            if (x.ext == ext && F[(size_t)x.e].rth && F[(size_t)x.e].sess == cf.sess) { ix = x.e; break; }
        }
        if (ix < COB2_MR || ix >= j) return false;
        const long long days = zzd::floorDiv(cf.t, 86400); const int mn = (int)((cf.t - days * 86400) / 60);
        if (mn - openMin < COB2_MIN_OPEN) return false;
        auto Hm = [&](long long i) { return d > 0 ? F[(size_t)i].h : -F[(size_t)i].l; }; auto Lm = [&](long long i) { return d > 0 ? F[(size_t)i].l : -F[(size_t)i].h; };
        auto Cm = [&](long long i) { return (long long)d * F[(size_t)i].c; };
        bool ft = false; for (long long i = ix + 1; i <= ix + 2 && i <= j; ++i) if (Cm(i) > Hm(ix)) ft = true;
        if (!ft) return false;
        long long pre = Hm(ix - 1); for (long long i = ix - COB2_PRE; i < ix; ++i) if (Hm(i) > pre) pre = Hm(i);
        if (!(Cm(j) > pre)) return false;
        std::vector<long long> rg; for (long long i = ix - COB2_MR; i < ix; ++i) rg.push_back(F[(size_t)i].h - F[(size_t)i].l);
        std::sort(rg.begin(), rg.end()); const double mr = 0.5 * (double)(rg[COB2_MR / 2 - 1] + rg[COB2_MR / 2]);
        return mr > 0 && (double)(Cm(j) - Lm(ix)) / mr >= COB2_DIST;
    }

    // call right after the engine completed a wave on the newest bar (W.back() = that wave; F is aligned with E.B)
    void onWave(const std::vector<zzd::Wave>& W) {
        if (!ok || W.empty() || F.empty()) return;
        const size_t nW = W.size(); const zzd::Wave& w = W[nW - 1]; const int j = (int)F.size() - 1;
        if (w.s < 0 || w.e < 0 || w.e > j) return;
        const FBar& cf = F[(size_t)j]; const long long tWave = F[(size_t)w.e].t; const int d = w.dir;
        // ---- Lvp of the Cobs still open
        for (size_t a = 0; a < act.size();) {
            Act& A = act[a]; bool drop = false;
            if (cf.sess != A.sess) { drop = true; closeAct(A, 2); }
            else if (d == -A.dir) {
                ++A.n;
                if ((long long)A.dir * (w.ext - A.origin) < 0) { drop = true; closeAct(A, 2); }  // below the Cob's origin
                else if (w.vol * 3 <= A.vol) {
                    Tag& t = tags[key(tWave, d)]; t.lvp = A.dir; t.tWave = tWave; t.tCf = cf.t; t.tCob = A.key / 2; if (!t.dir) t.dir = d;
                    out.push_back("L|" + std::string(VERSION) + "|" + std::to_string(tWave) + "|" + std::to_string(cf.t) + "|" + (A.dir > 0 ? "UP" : "DN") + "|" + std::to_string(A.key / 2));
                    drop = true; closeAct(A, 1);
                } else if (A.n >= LVP_WAVES) { drop = true; closeAct(A, 2); }
            } else if ((long long)A.dir * (w.ext - A.top) > 0) { drop = true; closeAct(A, 2); }   // a wave beyond the Cob's top
            if (drop) act.erase(act.begin() + (long)a); else ++a;
        }
        // ---- the Cob test
        if (nW < 2) return;
        const zzd::Wave& pw = W[nW - 2];
        if (pw.dir == d || pw.e < 0 || pw.e > j) return;
        const FBar& ob = F[(size_t)pw.e];
        const long long origin = pw.ext, top = w.ext, leg = (long long)d * (top - origin), c0 = cf.c;
        if (leg <= 0 || !cf.rth || !ob.rth || ob.sess != cf.sess || !rthAny) return;
        if ((long long)d * (c0 - origin) <= 0) return;                                      // already closed beyond the origin
        const bool nearExt = d > 0 ? origin <= rLoBarH : origin >= rHiBarL;
        if (!nearExt) return;
        const bool bo = nW >= 3 && (long long)d * (top - W[nW - 3].ext) > 0;
        const double retr = (double)((long long)d * (top - c0)) / (double)leg;
        const HL* L[6] = { &on, &as, &lon, &phr, &prevFull, &prevRth }; const long long V[3] = { prevVA.poc, prevVA.vah, prevVA.val }; int mask = 0;
        for (int i = 0; i < NLV; ++i) {
            if (i < 6 ? !L[i]->any : !prevVA.ok) continue;
            const long long p = i < 6 ? (d > 0 ? L[i]->l : L[i]->h) : V[i - 6];
            if ((long long)d * (p - origin) > 0 && (long long)d * (top - p) >= 0 && (long long)d * (c0 - p) > 0) mask |= 1 << i;
        }
        int cnt = w.cntCap ? zzd::CNT_CAP : w.cnt; const int cntPane = cnt; int cntMkt = -1;
        if (!M.W.empty() && M.W.back().e == w.e && M.W.back().dir == d && M.B.size() == F.size()) { const zzd::Wave& mw = M.W.back(); cntMkt = mw.cntCap ? zzd::CNT_CAP : mw.cnt; }
        const bool cntOk = cntMkt >= 0;                                                      // the two walks disagree: never guess a count
        cnt = cntOk ? std::min(cntPane, cntMkt) : 0;
        const bool cob = cntOk && mask != 0 && cnt >= MIN_CNT;
        const int watch = cob || !cntOk ? 0 : (mask == 0 && cnt >= MIN_CNT) ? 1 : (mask != 0 && cnt >= WATCH_CNT) ? 2 : 0;
        const bool cob2 = !cob && retr <= MAX_RETR && cob2Test(d, j, W);
        if (!cob && !watch && !cob2) return;
        const bool hr = hourTurn(ob.t, d, c0);
        Tag& t = tags[key(tWave, d)];
        t.dir = d; t.tWave = tWave; t.tCf = cf.t; t.origin = origin; t.top = top; t.close = c0; t.lv = mask; t.cnt = cnt; t.retr = retr;
        t.atExt = d > 0 ? origin == rLo : origin == rHi;
        t.hr = hr; t.q = quality(d, pw.e, w.e); t.rng = rngUsed();
        if (cob || cob2) {
            int rk = 0; const int cb = cobBar(d, pw.e, w.e, &rk); t.tCb = cb >= 0 ? F[(size_t)cb].t : 0; t.cbRank = cb >= 0 ? rk : 0;
            int lvI = -1; const int sr = stopRunBar(d, pw.s, pw.e, mask, &lvI); t.tSr = sr >= 0 ? F[(size_t)sr].t : 0; t.srLv = sr >= 0 ? lvI : -1;
            t.srPx = sr >= 0 && lvI >= 0 && lvI < NLV ? (lvI < 6 ? (d > 0 ? L[lvI]->l : L[lvI]->h) : V[lvI - 6]) : 0;
        }
        if (cob2) t.cob2 = d;
        if (cob || cob2) { if (cob) t.cob = d; Act A; A.key = key(tWave, d); A.origin = origin; A.top = top; A.vol = w.vol; A.sess = cf.sess; A.dir = d; act.push_back(A); }
        if (!cob) t.watch = watch;
        char b[96]; std::snprintf(b, sizeof b, "|%d|%.2f|%s|%s|%d|%d", cnt, retr, t.atExt ? "at" : "near", bo ? "bo" : "-", cntPane, cntMkt);
        const std::string body = std::string(VERSION) + "|" + std::to_string(tWave) + "|" + std::to_string(cf.t) + "|" + (d > 0 ? "UP" : "DN") + "|" + std::to_string(origin) + "|" +
                      std::to_string(top) + "|" + std::to_string(c0) + "|" + lvText(mask, d) + b;
        const std::string qs = "|" + std::to_string(t.q) + "|" + std::to_string(t.tCb) + "|" + std::to_string(t.cbRank) + "|" + std::to_string(t.tSr) + "|" + (t.srLv >= 0 ? (d > 0 ? LV_LO[t.srLv] : LV_HI[t.srLv]) : "-") + "|" + std::to_string(t.rng) + "|" + std::to_string(t.srPx);
        if (cob) out.push_back("C|" + body + (hr ? "|H" : "|-") + qs);
        else if (watch) out.push_back("W|" + body + (watch == 1 ? "|no level" : "|count under 10"));
        if (cob2) out.push_back("Q|" + body + (hr ? "|H" : "|-") + qs);
    }
    // (1.2.0) the read box: the newest Cob / Cob2 of the CURRENT session
    Read read() const {
        Read R; int n = 0;
        for (auto it = tags.rbegin(); it != tags.rend() && n < 400; ++it, ++n) {
            const Tag& t = it->second; if (!t.cob && !t.cob2) continue;
            if (!haveSess || sessOf(t.tCf) != curSess) { if (t.tCf < (F.empty() ? 0 : F.back().t) - 2 * 86400) break; continue; }
            if (R.any && t.tCf <= R.tCf) continue;
            R.any = true; R.kind = t.cob ? 1 : 2; R.dir = t.cob ? t.cob : t.cob2; R.lv = t.lv; R.cnt = t.cnt; R.lvpSt = t.lvpSt; R.hr = t.hr; R.retr = t.retr; R.tCf = t.tCf; R.q = t.q; R.rng = t.rng;
            const int ls = t.lvpSt < 0 || t.lvpSt > 2 ? 0 : t.lvpSt;
            const int qb = t.q >= 4 ? 2 : t.q == 3 ? 1 : 0;
            R.hold = t.cob && t.retr > MAX_RETR ? HOLD_DEEP[qb > 0 ? 1 : 0][ls] : t.cob ? HOLD_COB[qb][t.hr ? 1 : 0][ls] : HOLD_COB2[qb][ls];
            R.hold = holdWithRng(R.hold, t.rng);
        }
        return R;
    }
    const Tag* find(long long tWave, int dir) const { auto it = tags.find(key(tWave, dir)); return it == tags.end() ? nullptr : &it->second; }
    // keep the map bounded: drop tags older than the oldest bar held
    void prune() { if (F.empty() || tags.size() < 4000) return; const long long t0 = F.front().t; for (auto it = tags.begin(); it != tags.end();) { if (it->second.tWave < t0) it = tags.erase(it); else ++it; } }
    std::string statusText() const {
        std::string s = std::string("COBFINDER,") + VERSION + (ok ? ",RTH " + std::to_string(openMin) + "-" + std::to_string(closeMin) : ",off - no RTH hours for this market") +
            ",rule near RTH extreme + key level swept and closed back + count >= " + std::to_string(MIN_CNT) + " (no retracement limit); Cob2 = quiet turn (close back over the extreme bar in 2 bars + close beyond the 10 bars before it + 5 ranges + 60 min after the open)\n";
        int n = 0;
        for (auto it = tags.rbegin(); it != tags.rend() && n < 12; ++it) {
            const Tag& t = it->second; if (!t.cob && !t.lvp && !t.cob2) continue; ++n;
            if (t.cob2) { char b[200]; std::snprintf(b, sizeof b, "COB2,%lld,%s,%s,quality %d,range used %d%%,gave back %.2f,hourly %s\n", t.tWave, t.cob2 > 0 ? "UP" : "DN", lvText(t.lv, t.cob2).c_str(), t.q, t.rng, t.retr, t.hr ? "turned" : "not turned"); s += b; }
            if (t.cob) { char b[200]; std::snprintf(b, sizeof b, "COB,%lld,%s,%s,count %d,quality %d,range used %d%%,gave back %.2f,%s\n", t.tWave, t.cob > 0 ? "UP" : "DN", lvText(t.lv, t.cob).c_str(), t.cnt, t.q, t.rng, t.retr, t.atExt ? "at the extreme" : "near the extreme"); s += b; }
            if (t.lvp) { char b[120]; std::snprintf(b, sizeof b, "LVP,%lld,of the %s Cob at %lld\n", t.tWave, t.lvp > 0 ? "UP" : "DN", t.tCob); s += b; }
        }
        return s;
    }
};
}  // namespace zcf
