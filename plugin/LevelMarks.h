// LevelMarks.h 1.0.0 (2026-11-19) -- Bk / Sw / Cx on the price candles, each with ONE level (lsRevRead / lsRevReadMarks 0.19.0). SDK-free, tested
// (tests_levelmarks). Rassul 2026-11-19 10:20 CT "lets show bk sw sr and cx and the level they apply to . but be mindfull of not overlapping one
// another and putting lettes on top of green and below red as well as indicating the key level they apply to .. show mockup"; 10:26 "if you showed
// it without the price could you put it in", "but on 2nd line", "and abbreviate like Op or LLO or LHI"; 10:31 "you can also mention other levels
// that it is breaking even the highs like PDH"; 10:34 "to simplify, we can mention only 1 level giving priority first to session levels like
// overnight, then pd levels, pf, then volume levels"; 10:37 "ok, cut repeats"; 10:40 "go" (mockup level-marks-mockup.html v8).
//   Bk  a candle CLOSES through a level (the close before was on the other side); (1.21.0) its volume OR its range bigger than the candle before
//       (every level). Not when the same level was crossed on a close in the 10 candles before (chop). dir = the close's way.
//   Sw  one failed ATTEMPT: from the first candle that trades through a level nobody broke since it formed, while the next candles keep trading
//       through it (3 candles at most); marked on the attempt's deepest candle when a candle at / after it (within 3) closes back on the original
//       side with no new extreme first; (1.8.0: no volume gate - the Sw candle's own BVMax / BRMax >= 20 instead). dir = up for a swept low.
//   Cx  a ZigZag (3-bar High / Low) pivot candle, or the candle before it, with the biggest volume in 20+; decided when the pivot is confirmed;
//       green at a pivot low, red at a pivot high; its levels = the levels inside the candle.
//   AR  (1.13.0) the bar that answers a climax: after a DRAWN Cx, the first of the 3 candles after the pivot candle that is green after a climax
//       in a down wave (Cx at a pivot low; red after one in an up wave), overlaps the pivot candle (green: its low below the pivot candle's high;
//       red: its high above the pivot candle's low) and has a BRMax of 10 or more; one per Cx; shown "AR" + the strength line; it may share its
//       candle with a Bk / Sw / Cx. Known when the pivot is confirmed and the AR candle has closed; never changes after.
//   One mark per candle (Sw, then Bk, then Cx - first come), one level per mark by priority: session (ON, London, Asia, Open) > prior day >
//   prior full session > volume > weekly > hours (prior hour PHL / PHH, then 2h / 3h back). MenthorQ / gamma names do not label a mark.
//   Repeats cut: a code shows at most once per level per session unless price went 1+ ATR(13) away from the level and came back.
// Pure function of the closed candles and the levels: the same candles give the same marks (no repaint).
#pragma once
#include "WyRead.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace lvm {
static const char* const VERSION = "1.28.0";   // (1.28.0, Rassul 2026-11-26 13:06-13:16 CT "its close would not close over the level", "in any buying climax, no bar closes above the level", "no" to also checking the bars that confirm the pivot) a Cx level must be one the push FAILED to close beyond: buying climax (pivot high) - the climax candle AND the pivot candle (when the climax is the candle before it) close at or under the level; selling climax (pivot low) - both close at or above it. A close beyond = a breakout, no Cx at that level
// was static const char* const VERSION = "1.27.0";   // (1.27.0, Rassul 2026-11-26 12:40 CT "currently cx doesn't require a level but i want to update it so that it requires a level, meaning the bar with the volume needs to be crossing a level", 12:45 "A" = every level a Cx can name) a Cx (and the live Cx?) needs a level INSIDE its candle (low <= level <= high): session, PD, PF, VAH / VAL, weekly, 3-hour S / R, PhrH / PhrL, hour highs / lows - the Cx level list (not the Bk-only hourly pivots / 4h / 3HrH). A heavy pivot candle with no level in it: no Cx (the candle before may still be the Cx if IT crosses a level)
// was static const char* const VERSION = "1.26.0";   // (1.26.0, Rassul 2026-11-26 12:23 CT "why is this not a cmx" - ES 10:15 AM HOD, 1,150 lots = the biggest in 32: the candle was a Sw CANDIDATE (closed back under ONH / LonHI / AsiaHI) that blocked the Cx step, then fillStrength dropped the Sw for BVMax < 99, leaving no mark) run() takes noSw = the candles whose own BVMax is under SW_MIN (swWeak(), the same count fillStrength uses), and drops them as Sw candidates BEFORE the Bk and Cx steps, so the candle can still be its Bk / Cx. Nothing else changes
// was static const char* const VERSION = "1.25.0";   // (1.25.0, Rassul 2026-11-25 16:46-16:59 CT "You know I'm looking for high volume rejection from an attempted break of a level", "In this case we should use 99+ bvmax", both sides (answered 17:00)) a Sw (VOB / VOT) = a high-volume rejection of an attempted break: the sweep candle's own BVMax must be 99+ (SW_MIN 99, was 20), and BOTH sides count again - any major level, either direction (1.23.0's highs-for-bearish / lows-for-bullish rule is off: SW_BOTH_SIDES; NQ 11-25 9:03 AM bearish at LonLO is a VOT again)
// was static const char* const VERSION = "1.24.2";   // (1.24.2, Rassul 2026-11-25 16:10 CT "thats a problem .. lets just use BVMax 20+") the Sw strength is BVMax again (the larger of BV and the same-colour count), as 1.24.0; BV alone (1.24.1) removed most sweeps (ES / NQ 11-25 had BV 4 / 3)
// was static const char* const VERSION = "1.24.1";   // (1.24.1, Rassul 2026-11-25 16:04 CT "the vob / vot strength requires BV 20+") the Sw strength = BV, the count over ALL candles before it (1.24.0 used BVMax = the larger of BV and the same-colour count BVG / BVR); the label shows that BV ("V24")
// was static const char* const VERSION = "1.24.0";   // (1.24.0, Rassul 2026-11-25 13:56 CT "tighten strength to reuire the bigest volume in at least 20 bars", 13:59 "there is one at 12:09 but now it would not work becasue it doesnt have BV of at least 20", 14:00 "go"): a Sw needs ITS OWN candle's BVMax >= 20 (volume only - a big range alone no longer makes a Sw: HG 11-25 12:09 LonHI R91 drops); the Sw's strength line shows the volume count ("V24")
// was static const char* const VERSION = "1.23.0";   // (1.23.0, Rassul 2026-11-25 13:11 CT "i want to update sweeps so bearish sweeps use highs and bullish sweeps use lows", 13:12 "correct .. go"): a bearish Sw is named only by a HIGH level (ONH LonHI AsiaHI PDH PFH PWH CWH R1 R2), a bullish Sw only by a LOW level (ONL LonLO AsiaLO PDL PFL PWL CWL S1 S2); a sweep candle that crossed no major level of its side is no Sw (it can still be its Bk / Cx). GC 11-25 11:24 AM (bearish, up through AsiaLO) is no longer a Sw
// was static const char* const VERSION = "1.22.0";   // (1.22.0, Rassul 2026-11-25 11:39 CT "so lets update the rule to current session" - GC 11:21 AM AsiaLO 3511.3: untouched in the day session, traded only before the open, so no Sw): a Sw's "nobody traded through it" check (and its one attempt) now looks back only to the start of the CURRENT session part - the RTH open candle for a candle in the day session (open .. 5 PM), the session start (5 PM) otherwise; Bk / Cx / AR / Tst / Abs unchanged
// was static const char* const VERSION = "1.21.0";   // (1.21.0, Rassul 2026-11-25 08:39-08:49 CT "the BK is not being displayed for 3h breaks of highs and lows", "the volume requirement of 5 needs to be changed, volume just needs to be increasing", "or the range needs to be increasing", "for all levels , not just minor or major", "go" - GC 11-25 ~8:27 red candle that closed below the 3-hour low): (a) the last completed 3-HOUR bar's high / low ( 3-hour bars from 5 PM CT as his 180-minute chart) join the Bk levels as 3HrH / 3HrL (Bk only, like 4hH / 4hL - kept; not "3hH": that name is already the high of the clock hour 3 hours back); (b) THE BK CANDLE RULE for every level = its volume OR its range bigger than the candle before (was: range >= the prior AND range or volume up; hour / pivot / 4h levels also needed the biggest volume in 5+ - gone)
// was static const char* const VERSION = "1.20.0";   // (1.20.0, Rassul 2026-11-24 16:01 CT "the bk here says it is a bullish break of a higher low ? bullish breaks are of highs and bearish breaks are of lows" - GC 13:15 Bk 2hL): a bullish Bk names / counts only a HIGH level (or a two-sided one: Open, POC, VWAP), a bearish Bk only a LOW one (levelSide)
// was static const char* const VERSION = "1.19.0";   // (1.19.0, Rassul 2026-11-24 12:51 CT "2. do the test" on tst-line3-mockup.html): tstL3() - the Tst badge's 3rd line A+E / E / A! / blank instead of the retrace % (research TST-REVERSE 2026-11-24, tst.py / tst2.py ported line by line). Everything else unchanged
// was static const char* const VERSION = "1.18.0";   // (1.18.0, Rassul 2026-11-24 11:12 CT "ok. build this" - the Abs badge, abs-badge-mockup.html v15): absMarks() - effort without result at the FIRST test of a Bk level (research ABS-ORDERBOOK 2026-11-24, ported line by line) with its position line, and the next-candle Ex (ABS-EXHAUSTION part 2). Bk / Sw / Cx / FT / Tst / Rev / AR / I unchanged
// was static const char* const VERSION = "1.17.0";   // (1.17.0, Rassul 2026-11-24 08:33 CT "go for hourly pivots and 4h high and lows", 08:34 "absorption levels should match bk levels", 09:01 "go for bk with hourly pivots and 4 hour highs and lows"): Bk (and the Tst / FT / Rev that follow it) gains two level groups - the HOURLY PIVOTS (HpH / HpL: his 1-bar High / Low ZigZag on 1-hour bars, R1 R2 / S1 S2 as HtfSr.h walks the 3-hour ones, standing only) and the last completed 4-HOUR bar's high / low (4hH / 4hL, 4-hour bars from 5 PM CT). Both rank after every older level (4h 65, pivots 70) so no earlier mark changes its name, and a Bk made only by them never takes a candle from a Cx, need the hour levels' rule (the Bk candle's volume the biggest in 5+) and never make or name a Sw, Cx or I (Sw stays major-only)
// was static const char* const VERSION = "1.16.0";   // (1.16.0, Rassul 2026-11-23 23:18 CT "the CX came late .. why ?" (HG 11:03 PM climax, drawn 11:12 PM when the 3-bar ZigZag confirmed the low), 23:20 "go" on the live Cx?): liveCx() below - the climax candle shows "Cx?" from its own close while it is still the low (high) of the running leg; the Cx itself is unchanged
// was static const char* const VERSION = "1.15.0";   // (1.15.0, Rassul 2026-11-23 19:54 CT "why isnt this a cmx ?" - GC 5:51 PM, the pivot low on 324 lots, the biggest in 35 candles: the Cx look-back counted only the session's own candles, so no Cx could come in the first hour after 5:00 PM; 20:16 "go"): the Cx's "biggest volume in 20+" reaches into the candles BEFORE the session (run() preV = their volumes, oldest first), as the Volume pane's BVMax does. Nothing else changes
// was VERSION = "1.14.0";   // (1.14.0, Rassul 2026-11-23 14:00-14:18 CT "for SW signal it needs to cross a major level like a session or pd pf  weekly or 3h S1 S2 , R1 R2 level etc", "no the 3h is  the S1 S2 R1 R2  levels", "no the bk and tst can also include hourly", "ok so you are just adding to bk right ?", "and for SW you are limiting to major levels right", "for the AR badge ,   if the Green bar is bigger than the ATR and its high is above the cmx bar with the hevy volume , it is also valid AR", "build") (a) + the 3-HOUR S / R (R1 R2 S1 S2 of HtfSr.h, the lines lsHtfSR draws) as levels of every mark: the host hands run() the levels standing before each candle (H3); (b) a Sw is kept only when the level it is LABELLED with is MAJOR (swMajor: session ON / London / Asia, PD, PF, weekly, the 3h S / R) - the hour highs / lows, the pre-open hour (PhrH / PhrL) and VAH / VAL no longer name a Sw; what makes a sweep (a level nobody traded through, the close back, colour, strength 20+, no opening / release candle) is unchanged; Bk / Tst / Cx keep every level they had and gain the 3h S / R; (c) the AR's second way in: range > ATR(13) of the candle before AND beyond the Cx candle's extreme (AR_ATR) - see THE AR
// was VERSION = "1.13.0";   // (1.13.0, Rassul 2026-11-23 09:25 CT "i want you to implement the AR which is an automatic rally bar that occurs after a cx . so if there is a climax, and there is a bar that has its low below the pivot high and is within 3 bars of the pivot and has a BRMax of at least 10. so if the cx was in a downwave , the ARbar would have to be green", 09:26 "the AR bar should also have a strength line", "here is a decent example" (NQ 11-23 6:51 AM, the green candle after the 6:48 pivot low; Cx 6:45), 09:34 "go"; mockup ar-mockup.html): the AR - arCands() / arPick() below; Bk / Sw / Cx / FT / Tst / Rev untouched
// was VERSION = "1.12.0";   // (1.12.0, Rassul 2026-11-21 05:51 CT "we can skip data release for sweeps, they shouldnt qualify" - 6E 2026-01-09 7:33 Sw LonLO was the 7:30 data-release candle, V99+ / R99+ from the news): no Sw on a candle that holds a scheduled release (the Event Calendar list: the nightly LRA-Calendar-ALL.csv + its weekly rules), any impact, any market row ALL or this market - newsCandle()
// was VERSION = "1.11.0";   // (1.11.0, Rassul 2026-11-21 05:40 CT "we need to remove opening candle as a candidate for sweeps" - CL 2026-01-02 8:03 Sw LonLO was the RTH opening candle, its V99+ / R40 came from the open, swept both ways and failed): no Sw on the RTH opening candle (the candle that ENDS one bar after the RTH open: ES / NQ 8:30, CL / NG 8:00, GC / 6E 7:20, HG 7:10 CT) - rthOpenMin(); Bk / Cx unchanged
// was VERSION = "1.10.1";   // (1.10.1, Rassul 2026-11-20 20:27 CT "first do manual testing": the hand check found EU 10-15 10:03 marked a high sweep of 1hL when the candle before closed exactly ON the level - a close on the level has no side, so no Sw from it)   // was "1.10.0";   // (1.10.0, Rassul 2026-11-20 16:44 CT "yes, use strict rule"): a Sw needs a level nobody TRADED through since it formed (wicks count, one attempt per level - the 1.8.0 rule), and the mark is LABELLED with the best key level its candle crossed (wick through, close back) even if that key level was touched earlier (GC 11-20 9:00: swept the untouched 1-hour high 3,582.8, crossed PhrH 3,582.4 -> "PhrH"); 1.9.0's formation start, combined names, repeat fallback and 1hH / PhrH names stay
// was VERSION = "1.9.0";   // (1.9.0, Rassul 2026-11-20 14:20 CT "the 9:09 was a lonHi" - NQ 9:00 swept LonHI 26,447.75 but was labelled PHH): a SESSION level (ON / London / Asia / the pre-open hour / Open) counts "unbroken" from the candle that FORMED it (its high / low = the level), not from 5 PM - Asia trading above the later LonHI hid it; combined names ("ONL/PHL") take the best part's priority and short name instead of being dropped
// was VERSION = "1.8.0";   // (1.8.0, Rassul 2026-11-20 10:34-10:37 CT "look at the sw signal, shouldnt it meet a threshold of 20", "the red sweep bar doesnt meet the threshold.. fix", "I should only be on key levels that are named like overnight etc.. and we should have a 20 threshold just like sweeps"; lsRevRead 0.21.3): the Sw needs the Sw CANDLE ITSELF at BVMax or BRMax >= 20 and its strength line is that candle's own (was the max over the attempt candles - HG 11-20 08:03 a weak red Sw got V99+ from the green candle before it); the attempt-volume gate is gone (11:00 CT "shouldnt this be a sweep", ES 09:00 ONH); iPick() names only session / PD / PF / volume / weekly levels - never an hour level (PHH / PHL / 2h / 3h); I_MIN 20 = the I candle's own BVMax / BRMax minimum
// was VERSION = "1.7.0";   // (1.7.0, Rassul 2026-11-20 09:20 CT "add [the I] ... if both sweep and i are true, only show sweep", 09:24 "build"; lsRevRead 0.21.2): iPick() = the ONE level an I names, by the Bk / Sw priority (session > PD > PF > volume > weekly > hours; a tie = the furthest reclaimed); iUnderSw() = the I candle is also a Sw of the same direction (only the Sw shows); no Bk / Sw / Cx rule change
// was VERSION = "1.6.0";   // (1.6.0, Rassul 2026-11-20 07:50 CT "update the sweep threshold to 20": a Sw needs its attempt's BVMax or BRMax >= 20 (SW_MIN), was 10; the strength line still shows from 10 up (ST_MIN unchanged))   // was "1.5.0"   // (1.5.0, Rassul 2026-11-19 18:29 CT "lets keep it at 10 so the BRmax or BVmax should be at least 10": a Sw needs its attempt's BVMax or BRMax >= 10 (SW_MIN), else no Sw)   // was "1.4.0"   // (1.4.0, Rassul 2026-11-19 18:19 CT "switch to 10": the strength line shows from 10 up, was 20)   // was "1.3.0"   // (1.3.0, Rassul 2026-11-19 15:35-16:58 CT, mockup price-pane-cleanup-mockup.html, "1. ok 2. ok 3. ok"): Bk needs the BO candle rule (range >= the prior candle's and range or volume up); a bullish Sw is a GREEN candle, a bearish Sw a RED one; DPoc / WPoc / Open are not levels any more; + seq(): FT / Tst / Rev after a Bk (the BO indicator's FT, Pb and PB Rev rules on the Bk)
// was VERSION = "1.2.0";   // (1.2.0, Rassul 2026-11-19 11:22-11:30 CT "sweeps of key levels on big volume or range are very meaningfull", "maybe consider sw and a second line like v50 or r50, same with breaks", "use the max numbers that we have ... biggest in green bars, red bars, consecutive", "ok.. good build it"): Bk / Sw carry a STRENGTH = his BVMax / BRMax (the Volume / Range panes' Max number) - "V<N>" / "R<N>" from 20 up, "99+" from 99; a Sw takes the max over its attempt's candles
// was VERSION = "1.1.0";   // (1.1.0, Rassul 10:50 CT "ok build": the BO indicator's Rly gets its level too - levelNear(); same priority / short names)

struct Mark { char code = 0; int dir = 0; long long px = 0; std::string lv; long long t0 = 0; std::string st; };   // (1.2.0) t0 = the first candle the strength looks at (a Sw: its attempt's first candle; else the mark's own); st = "V45" / "R99+" / ""   // code 'B' Bk / 'S' Sw / 'C' Cx; lv = the short level name ("" = none)
inline const char* codeText(char c) { return c == 'B' ? "Bk" : c == 'S' ? "Sw" : c == 'C' ? "Cx" : ""; }

// the priority of a level name (lower = shown first); -1 = this name never labels a mark (MenthorQ, gamma, VWAP ...)
inline int pri(const std::string& n, int hb);
// (1.9.0) a combined name "ONL/PHL" / "LonHI/PFL": the best priority of its parts (a part that labels nothing is skipped); -1 = none
inline int priParts(const std::string& n, std::string* bestPart = nullptr) {
    int best = -1; size_t a = 0;
    for (;;) { const size_t p = n.find('/', a); const std::string part = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        const int q = part.empty() || part == n ? -1 : pri(part, 0); if (q >= 0 && (best < 0 || q < best)) { best = q; if (bestPart) *bestPart = part; }
        if (p == std::string::npos) break;
        a = p + 1; }
    return best;
}
// (1.9.0) a level formed INSIDE the session (ON, London, Asia, the pre-open hour PHH / PHL of the level file, Open), alone or in a combined name
inline bool sessFormed(const std::string& n) {
    static const char* const S[] = { "ONH", "ONL", "LonHI", "LonLO", "AsiaHI", "AsiaLO", "PHH", "PHL", "Open" };
    size_t a = 0; for (;;) { const size_t p = n.find('/', a); const std::string part = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        for (const char* x : S) { if (part == x) return true; }
        if (p == std::string::npos) break;
        a = p + 1; }
    return false;
}
// (1.9.0) the clock window a session level is printed in (candle END, local): Asia (6 PM, 1 AM], London (2 AM, 7:30 AM], ON (5 PM, 8:30 AM],
// the pre-open hour PHH / PHL (6:20 AM, 8:30 AM], Open (7:20 AM, 8:40 AM]; a combined name: any of its parts
inline bool formWindow(const std::string& n, long long tEnd) {
    long long s = tEnd % 86400; if (s < 0) s += 86400; const int m = (int)(s / 60);
    size_t a = 0;
    for (;;) { const size_t p = n.find('/', a); const std::string q = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        if ((q == "AsiaHI" || q == "AsiaLO") && (m > 18 * 60 || m <= 60)) return true;
        if ((q == "LonHI" || q == "LonLO") && m > 120 && m <= 450) return true;
        if ((q == "ONH" || q == "ONL") && (m > 17 * 60 || m <= 510)) return true;
        if ((q == "PHH" || q == "PHL") && m > 380 && m <= 510) return true;
        if (q == "Open" && m > 440 && m <= 520) return true;
        if (p == std::string::npos) break;
        a = p + 1; }
    return false;
}
// (1.9.0) the window's end (minutes after midnight, candle END) of a session level's best-known part; -1 = not a session level
inline int formEnd(const std::string& n) {
    int e = -1; size_t a = 0;
    for (;;) { const size_t p = n.find('/', a); const std::string q = n.substr(a, p == std::string::npos ? std::string::npos : p - a);
        const int x = (q == "AsiaHI" || q == "AsiaLO") ? 60 : (q == "LonHI" || q == "LonLO") ? 450 : (q == "ONH" || q == "ONL" || q == "PHH" || q == "PHL") ? 510 : q == "Open" ? 520 : -1;
        if (x > e) e = x;
        if (p == std::string::npos) break;
        a = p + 1; }
    return e;
}
inline int pri(const std::string& n, int hb) {
    if (hb > 0) return hb == 1 ? 50 : 60;
    static const char* const S0[] = { "ONL", "ONH", "ONH/LonHI", "ONL/LonLO", "LonLO", "LonHI", "AsiaLO", "AsiaHI" };   // (1.3.0, 16:34 "just remove it") no Open
    static const char* const S1[] = { "PDH", "PDL", "PFL/PDL", "PFH/PDH" };
    static const char* const S2[] = { "PFH", "PFL" };
    static const char* const S3[] = { "VAH", "VAL" };   // (1.3.0, 16:30 "remove poc") no DPoc / WPoc
    static const char* const S4[] = { "PWH", "PWL", "CWH", "CWL" };
    static const char* const S5[] = { "PHL", "PHH" };
    static const char* const S6[] = { "R1", "R2", "S1", "S2" };   // (1.14.0) the 3-hour S / R: after the weekly levels, before the hour levels
    for (const char* x : S0) if (n == x) return 0;
    for (const char* x : S1) if (n == x) return 10;
    for (const char* x : S2) if (n == x) return 20;
    for (const char* x : S3) if (n == x) return 30;
    for (const char* x : S4) if (n == x) return 40;
    for (const char* x : S6) if (n == x) return 45;
    for (const char* x : S5) if (n == x) return 50;
    if (n == "3HrH" || n == "3HrL") return 48;   // (1.21.0) the 3-hour bar high / low: after the 3h S / R (45), BEFORE the clock-hour levels (50 / 60) - the same price names the 3-hour bar (GC 11-25 8:30: 3465.1 = both the 3-hour low and the 7 AM hour low -> "3HrL")
    if (n == "4hH" || n == "4hL") return 65;   // (1.17.0) the 4-hour bar high / low: after every older level (the hour levels 50 / 60 keep their names)
    if (n == "HpH" || n == "HpL") return 70;   // (1.17.0) the hourly pivots: ranked last
    if (n.find('/') != std::string::npos) return priParts(n);   // (1.9.0) a combined name not listed above
    return -1;
}
inline std::string shortName(const std::string& n, int hb, bool high) {
    if (hb > 0) return std::to_string(hb) + "h" + (high ? "H" : "L");   // (1.9.0, Rassul 14:26 CT "the sweep label changes") the prior hour = 1hH / 1hL (was PHH / PHL)
    static const std::pair<const char*, const char*> M[] = { { "PHH", "PhrH" }, { "PHL", "PhrL" },   // (1.9.0) the level file's pre-open hour, as his chart names it
        { "Open", "Op" }, { "LonLO", "LLO" }, { "LonHI", "LHI" }, { "ONH/LonHI", "ONH" },
        { "ONL/LonLO", "ONL" }, { "AsiaLO", "ALO" }, { "AsiaHI", "AHI" }, { "PFL/PDL", "PDL" }, { "PFH/PDH", "PDH" }, { "DPoc", "DP" }, { "WPoc", "WP" } };
    for (const auto& m : M) if (n == m.first) return m.second;
    if (n.find('/') != std::string::npos) { std::string b; if (priParts(n, &b) >= 0 && !b.empty()) return shortName(b, 0, high); }   // (1.9.0) the best part's short name
    return n;
}

struct Lv { long long px = 0; std::string name; int from = 0; int hb = 0; bool high = false; bool bkOnly = false; };   // (1.17.0) bkOnly = an hourly pivot / 4-hour level: Bk only
// (1.14.0) the 3-hour S / R standing BEFORE a candle trades (ticks; 0 = none / broken): one per candle of B, from the host's HtfSr walk
struct H3 { long long r1 = 0, r2 = 0, s1 = 0, s2 = 0; bool has(long long px) const { return px > 0 && (px == r1 || px == r2 || px == s1 || px == s2); }
    // (1.17.0) the hourly pivots (standing) and the last completed 4-hour bar's high / low, before the candle (ticks; 0 = none)
    long long hr1 = 0, hr2 = 0, hs1 = 0, hs2 = 0, h4h = 0, h4l = 0, h3h = 0, h3l = 0;   // (1.21.0) + the last completed 3-hour bar's high / low
    bool hasHp(long long px) const { return px > 0 && (px == hr1 || px == hr2 || px == hs1 || px == hs2); }
    bool has4(long long px) const { return px > 0 && (px == h4h || px == h4l); }
    bool has3(long long px) const { return px > 0 && (px == h3h || px == h3l); } };
// (1.17.0) THE HOURLY PIVOTS and THE 4-HOUR HIGH / LOW, walked by the host over the chart's candles (any bar size dividing the block):
//   PivWalk = HtfSr.h's hsr::Walk with the block size as a parameter (1 hour, blocks on the clock hour): his 1-bar High / Low ZigZag, the
//   forming block counts, R1 = the last confirmed pivot high, R2 = the higher of the last two, S1 / S2 the same for lows, standing = no
//   COMPLETED block since the pivot has closed beyond it. Blk4 = the high / low of the last COMPLETED 4-hour block (17-21, 21-01, 01-05,
//   05-09, 09-13, 13-17 CT). t = the candle END, local seconds of an epoch that starts on a local midnight (IRT's own times).
struct PivSnap { long long r1 = 0, r2 = 0, s1 = 0, s2 = 0; };
struct PivWalk {
    long long secs = 3600, off = 0; struct HB { long long k = 0; long long h = 0, l = 0, c = 0; };
    std::vector<HB> H; int d = 0, ext = 0; std::vector<std::pair<int, int> > piv;
    static long long fdiv(long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
    static void step(const std::vector<HB>& V, int i, int& d, int& ext, std::vector<std::pair<int, int> >& out) {
        if (i < 1 || i >= (int)V.size()) return;
        if (d == 0) { d = V[1].h >= V[0].h ? 1 : -1; ext = 0; }
        const HB& b = V[(size_t)i]; const HB& p = V[(size_t)i - 1];
        if (d == 1) { if (b.h > V[(size_t)ext].h) ext = i; else if (b.l < p.l && i != ext) { out.push_back(std::make_pair(ext, 1)); ext = i; d = -1; } }
        else        { if (b.l < V[(size_t)ext].l) ext = i; else if (b.h > p.h && i != ext) { out.push_back(std::make_pair(ext, -1)); ext = i; d = 1; } } }
    PivSnap add(long long t, long long perSec, long long h, long long l, long long c) {
        PivSnap o; if (perSec <= 0 || perSec > secs || secs % perSec != 0 || h < l) return o;
        const long long k = fdiv(t - perSec - off, secs);
        if (H.empty() || k != H.back().k) { if (!H.empty()) step(H, (int)H.size() - 1, d, ext, piv); HB n; n.k = k; n.h = h; n.l = l; n.c = c; H.push_back(n); }
        else { HB& n = H.back(); n.h = std::max(n.h, h); n.l = std::min(n.l, l); n.c = c; }
        const int last = (int)H.size() - 1; int d2 = d, e2 = ext; std::vector<std::pair<int, int> > extra; step(H, last, d2, e2, extra);
        if (d2 == 0) return o;
        int hi[2] = { -1, -1 }, lo[2] = { -1, -1 }, nh = 0, nl = 0;
        auto take = [&](const std::pair<int, int>& q) { if (q.second > 0) { if (nh < 2) hi[nh++] = q.first; } else { if (nl < 2) lo[nl++] = q.first; } };
        for (size_t q = extra.size(); q-- > 0;) take(extra[q]);
        for (size_t q = piv.size(); q-- > 0 && (nh < 2 || nl < 2);) take(piv[q]);
        auto stand = [&](int pb, long long L, int sd) { for (int q = pb + 1; q < last; ++q) if (sd > 0 ? H[(size_t)q].c > L : H[(size_t)q].c < L) return false; return true; };
        if (nh >= 1) { const long long r1 = H[(size_t)hi[0]].h; int b2 = hi[0]; if (nh >= 2 && H[(size_t)hi[1]].h > r1) b2 = hi[1];
            if (stand(hi[0], r1, 1)) { o.r1 = r1; }
            const long long r2 = H[(size_t)b2].h; if (r2 != r1 && stand(b2, r2, 1)) o.r2 = r2; }
        if (nl >= 1) { const long long s1 = H[(size_t)lo[0]].l; int b2 = lo[0]; if (nl >= 2 && H[(size_t)lo[1]].l < s1) b2 = lo[1];
            if (stand(lo[0], s1, -1)) { o.s1 = s1; }
            const long long s2 = H[(size_t)b2].l; if (s2 != s1 && stand(b2, s2, -1)) o.s2 = s2; }
        return o; } };
struct Blk4 {
    long long k = 0, h = 0, l = 0, ph = 0, pl = 0; bool any = false;   // the forming block and the last completed one
    long long len = 14400, off = 3600;                                 // (1.21.0) the block: 4 hours from 5 PM (1, 5, 9 ...); 3 hours from 5 PM = len 10800, off 7200 (2, 5, 8 ...)
    // returns the last COMPLETED block's high / low as it stands BEFORE this candle, then takes the candle in
    std::pair<long long, long long> add(long long t, long long perSec, long long hi, long long lo) {
        const long long kk = PivWalk::fdiv(t - perSec - off, len);
        if (any && kk != k) { ph = h; pl = l; any = false; }
        const std::pair<long long, long long> before(ph, pl);
        if (!any) { k = kk; h = hi; l = lo; any = true; } else { h = std::max(h, hi); l = std::min(l, lo); }
        return before; } };
// (1.14.0, "for SW signal it needs to cross a major level like a session or pd pf  weekly or 3h S1 S2 , R1 R2 level") the levels that may NAME a Sw:
// session (0), prior day (10), prior full session (20), weekly (40), the 3h S / R (45). Not VAH / VAL (30), not the pre-open hour (50), no hour level.
inline bool swMajor(const std::string& name, int hb) { if (hb != 0) return false; const int p = pri(name, 0); return p == 0 || p == 10 || p == 20 || p == 40 || p == 45; }
inline int levelSide(const std::string& name, bool hourHigh, int hb);
// (1.23.0, Rassul 13:11 CT "bearish sweeps use highs and bullish sweeps use lows") a level may name a Sw of direction dir (+1 bullish / -1 bearish) only
// when it is a low (bullish) / a high (bearish); a shared name ("ONL/LonLO") counts when any of its names is of that side
static const bool SW_BOTH_SIDES = true;   // (1.25.0) a rejection of an attempted break counts at any major level, either direction (false = the 1.23.0 rule)
inline bool swSideOk(const std::string& name, int dir) {
    if (SW_BOTH_SIDES) return true;
    std::string t; for (size_t k = 0; k <= name.size(); ++k) { const char c = k < name.size() ? name[k] : '/';
        if (c == '/') { if (!t.empty() && levelSide(t, false, 0) == (dir > 0 ? -1 : 1)) return true; t.clear(); } else t += c; }
    return false; }
// the levels at candle i: the key / volume levels (from the session start) + the high / low of the 3 full clock hours before i's hour
inline std::vector<Lv> levelsAt(const std::vector<wyr::Bar>& B, int i, const std::vector<wyr::Lvl>& key, const std::vector<H3>* h3 = nullptr) {
    std::vector<Lv> v; for (const wyr::Lvl& z : key) { if (z.px <= 0 || pri(z.name, 0) < 0) continue;   // MenthorQ / gamma / VWAP never make or label a mark
        Lv a; a.px = z.px; a.name = z.name; a.from = 0;
        // (1.9.0, Rassul 14:20 CT "the 9:09 was a lonHi") a SESSION level (ON / London / Asia / the pre-open hour / Open) exists from the candle
        // that printed it (the first candle whose high or low is the level): before it, it is not a level (no Sw / Bk / Cx of it); its "unbroken"
        // check starts after that candle - trading beyond it earlier (Asia above the later LonHI) does not use it up
        if (sessFormed(z.name)) {
            int fk = -1; for (int k = 0; k < i && k < (int)B.size(); ++k) if ((B[(size_t)k].h == z.px || B[(size_t)k].l == z.px) && formWindow(z.name, B[(size_t)k].t)) { fk = k; break; }
            if (fk < 0) {   // no candle printed it exactly (another data source): it exists from the first candle after its window
                const int e = formEnd(z.name);
                for (int k = 0; k < i && k < (int)B.size(); ++k) { long long q = B[(size_t)k].t % 86400; if (q < 0) q += 86400; const int mm = (int)(q / 60); if (mm > e && mm < 17 * 60) { fk = k - 1; break; } }
                if (fk < -1 || fk == -1) { bool found = false; for (int k = 0; k < i && k < (int)B.size(); ++k) { long long q = B[(size_t)k].t % 86400; if (q < 0) q += 86400; const int mm = (int)(q / 60); if (mm > e && mm < 17 * 60) { found = true; break; } }
                    if (!found) continue; } }
            a.from = fk + 1; }
        v.push_back(a); }
    // (1.14.0) the 3-hour S / R as they stood before candle i: a level since the first candle that had it at this price (its "untraded" check starts there)
    if (h3 && i > 0 && (size_t)i < h3->size() && h3->size() == B.size()) {
        const H3& q = (*h3)[(size_t)i]; const long long px[4] = { q.r1, q.r2, q.s1, q.s2 }; static const char* const nm[4] = { "R1", "R2", "S1", "S2" };
        for (int s2 = 0; s2 < 4; ++s2) { if (px[s2] <= 0) continue;
            bool dup = false; for (int u = 0; u < s2; ++u) if (px[u] == px[s2]) dup = true;
            if (dup) continue;
            int from = i; while (from > 0 && (*h3)[(size_t)from - 1].has(px[s2])) --from;
            Lv a; a.px = px[s2]; a.name = nm[s2]; a.from = from; a.hb = 0; a.high = s2 < 2; v.push_back(a); } }
    // (1.17.0) the hourly pivots and the 4-hour high / low before candle i: Bk only (never a Sw / Cx / I level)
    if (h3 && i > 0 && (size_t)i < h3->size() && h3->size() == B.size()) {
        const H3& q = (*h3)[(size_t)i];
        const long long hp[4] = { q.hr1, q.hr2, q.hs1, q.hs2 };
        for (int s2 = 0; s2 < 4; ++s2) { if (hp[s2] <= 0) continue; bool dup = false; for (int u = 0; u < s2; ++u) if (hp[u] == hp[s2]) dup = true; if (dup) continue;
            int from = i; while (from > 0 && (*h3)[(size_t)from - 1].hasHp(hp[s2])) --from;
            Lv a; a.px = hp[s2]; a.name = s2 < 2 ? "HpH" : "HpL"; a.from = from; a.high = s2 < 2; a.bkOnly = true; v.push_back(a); }
        const long long f4[2] = { q.h4h, q.h4l };
        for (int s2 = 0; s2 < 2; ++s2) { if (f4[s2] <= 0) continue; int from = i; while (from > 0 && (*h3)[(size_t)from - 1].has4(f4[s2])) --from;
            Lv a; a.px = f4[s2]; a.name = s2 == 0 ? "4hH" : "4hL"; a.from = from; a.high = s2 == 0; a.bkOnly = true; v.push_back(a); }
        const long long f3[2] = { q.h3h, q.h3l };   // (1.21.0) the last completed 3-hour bar's high / low
        for (int s2 = 0; s2 < 2; ++s2) { if (f3[s2] <= 0) continue; int from = i; while (from > 0 && (*h3)[(size_t)from - 1].has3(f3[s2])) --from;
            Lv a; a.px = f3[s2]; a.name = s2 == 0 ? "3HrH" : "3HrL"; a.from = from; a.high = s2 == 0; a.bkOnly = true; v.push_back(a); } }
    if (B.empty() || i <= 0) return v;
    const long long hp = wyr::hourOf(B[(size_t)i].t), h0 = wyr::hourOf(B[0].t);
    for (long long hh = hp - 1; hh >= hp - 3; --hh) {
        if (hh <= h0) break;
        long long lo = 0, hi = 0; int last = -1;
        for (int k = 0; k < i; ++k) if (wyr::hourOf(B[(size_t)k].t) == hh) { if (last < 0) { lo = B[(size_t)k].l; hi = B[(size_t)k].h; } else { lo = std::min(lo, B[(size_t)k].l); hi = std::max(hi, B[(size_t)k].h); } last = k; }
        if (last < 0) continue;
        Lv a; a.px = lo; a.from = last + 1; a.hb = (int)(hp - hh); a.high = false; a.name = wyr::hourName(hh) + " hour low"; v.push_back(a);
        Lv b = a; b.px = hi; b.high = true; b.name = wyr::hourName(hh) + " hour high"; v.push_back(b);
    }
    return v;
}

// (1.20.0) which side a level is: +1 a high (ONH, PDH, LonHI, VAH, R1, HpH, 4hH, an hour high ...), -1 a low, 0 two-sided (Open, POC, VWAP ...).
// A merged name ("ONL/LonLO") takes its parts' side; mixed parts = two-sided.
inline int levelSide(const std::string& name, bool hourHigh, int hb) {
    if (hb > 0) return hourHigh ? 1 : -1;
    int hi = 0, lo = 0; std::string t;
    for (size_t k = 0; k <= name.size(); ++k) { const char c = k < name.size() ? name[k] : '/';
        if (c == '/' || c == ' ') { if (!t.empty()) { std::string u; for (char x : t) u += (char)std::toupper((unsigned char)x);
                const size_t L = u.size();
                if (u == "R1" || u == "R2" || u == "HPH" || u == "4HH" || u == "HIGH" || (L >= 2 && u.compare(L - 2, 2, "HI") == 0) || (L >= 3 && u.compare(L - 3, 3, "HRH") == 0) || (L >= 2 && u[L - 1] == 'H' && u.compare(0, 4, "OPEN") != 0)) ++hi;
                else if (u == "S1" || u == "S2" || u == "HPL" || u == "4HL" || u == "LOW" || (L >= 2 && u.compare(L - 2, 2, "LO") == 0) || (L >= 3 && u.compare(L - 3, 3, "HRL") == 0) || (L >= 2 && u[L - 1] == 'L' && u.compare(0, 4, "OPEN") != 0)) ++lo; }
            t.clear(); }
        else t += c; }
    if (name.find("hour high") != std::string::npos) return 1;
    if (name.find("hour low") != std::string::npos) return -1;
    return hi > 0 && lo == 0 ? 1 : lo > 0 && hi == 0 ? -1 : 0;
}
struct Cand { long long px = 0; std::string name; int hb = 0; bool high = false; bool bkOnly = false; };
// the one level a mark shows: the best priority among its candidates (ties: the first given); none labels -> empty
inline bool pick(const std::vector<Cand>& c, Cand& out) {
    int best = 1 << 30; bool ok = false;
    for (const Cand& z : c) { const int p = pri(z.name, z.hb); if (p < 0) continue; if (p < best) { best = p; out = z; ok = true; } }
    return ok;
}

// (1.11.0) the RTH open in Central minutes of the day for the market root, -1 = unknown (no candle excluded)
inline int rthOpenMin(const std::string& mkt) {
    if (mkt == "ES" || mkt == "NQ" || mkt == "MES" || mkt == "MNQ") return 8 * 60 + 30;
    if (mkt == "CL" || mkt == "NG" || mkt == "MCL") return 8 * 60;
    if (mkt == "GC" || mkt == "EU" || mkt == "6E" || mkt == "MGC") return 7 * 60 + 20;
    if (mkt == "HG") return 7 * 60 + 10;
    return -1; }
// (1.11.0) the candle stamped (bar END) at openMin + spb is the RTH opening candle
inline bool isOpenCandle(long long t, int openMin, long long spb) {
    if (openMin < 0 || spb <= 0) return false;
    long long q = t % 86400; if (q < 0) q += 86400;
    return q == (long long)openMin * 60 + spb; }
// (1.22.0, Rassul 2026-11-25 11:39 CT "update the rule to current session") the day session part of candle f: the first candle of the run of
// candles ending after the RTH open (openMin) and at / before 5 PM CT that holds f, i.e. the RTH opening candle of f's day; -1 when f is not in it
// (overnight / pre-open) or the market has no RTH open. Bar stamps = bar END, CT wall-clock seconds (any epoch that is a whole number of days)
inline int dayStart(const std::vector<wyr::Bar>& B, int f, int openMin) {
    if (openMin < 0 || f < 0 || f >= (int)B.size()) return -1;
    auto inDay = [&](int k) { long long q = B[(size_t)k].t % 86400; if (q < 0) q += 86400; const long long m = q / 60; return m > openMin && m <= 17 * 60 && q > (long long)openMin * 60; };
    if (!inDay(f)) return -1;
    int k = f; while (k > 0 && inDay(k - 1) && B[(size_t)k].t - B[(size_t)k - 1].t < 6 * 3600) --k;
    return k; }
// (1.12.0) the candle stamped t (bar END) holds a release r when t - spb <= r < t; rel = release times in the bars' own time unit (seconds), sorted
inline bool newsCandle(long long t, long long spb, const std::vector<long long>* rel) {
    if (!rel || rel->empty() || spb <= 0) return false;
    const auto it = std::lower_bound(rel->begin(), rel->end(), t - spb);
    return it != rel->end() && *it < t; }
// (1.28.0) the climax failed AT the level: every candle from the climax candle kk to the pivot candle k closed at or under it (buying climax, side -1)
// / at or above it (selling climax, side +1) - a close beyond is a breakout, not a climax
inline bool cxHeld(const std::vector<wyr::Bar>& B, int kk, int k, int side, long long px) {
    for (int j = kk; j <= k && j < (int)B.size(); ++j) { const long long c = B[(size_t)j].c; if (side < 0 ? c > px : c < px) return false; }
    return true;
}
inline std::map<long long, Mark> run(const std::vector<wyr::Bar>& B, const std::vector<wyr::Lvl>& key, int openMin = -1, long long spb = 0, const std::vector<long long>* rel = nullptr, const std::vector<H3>* h3 = nullptr, const std::vector<double>* preV = nullptr, const std::set<long long>* noSw = nullptr) {   // (1.26.0) noSw = candle stamps that cannot be a Sw (own BVMax < SW_MIN, swWeak()); (1.15.0) preV = the volume of the candles before B[0], oldest first (the Cx look-back only)
    std::map<long long, Mark> out; const int n = (int)B.size(); if (n < 6) return out;
    std::vector<double> v((size_t)n); for (int i = 0; i < n; ++i) v[(size_t)i] = (double)(B[(size_t)i].buy + B[(size_t)i].sell);
    auto maxV = [&](int a, int b) { double m = 0; for (int k = std::max(0, a); k < b; ++k) m = std::max(m, v[(size_t)k]); return m; };
    const std::vector<double> atr = wyr::atr13(B);
    std::map<int, std::pair<char, std::vector<Cand> > > raw;   // candle -> code + its candidate levels (mockup order: Sw, then Bk, then Cx)
    std::map<int, int> rawDir; std::map<int, int> rawA;   // (1.2.0) the mark candle -> the first candle of its Sw attempt
    // Sw: one attempt per level
    { std::set<std::pair<long long, std::string> > done;
      for (int f = 5; f < n; ++f) {
        const int ds = dayStart(B, f, openMin);   // (1.22.0) -1 = overnight part (look back to the level's formation, as before)
        const std::vector<Lv> L = levelsAt(B, f, key, h3);
        for (const Lv& z : L) {
            if (z.bkOnly) continue;   // (1.17.0) Sw: major levels only, never the hourly pivots / 4-hour levels
            const std::pair<long long, std::string> id(z.px, z.name + (ds >= 0 ? "|day" : "")); if (done.count(id)) continue;   // (1.22.0) one attempt per level per session part
            if (B[(size_t)f - 1].c == z.px) continue;   // (1.10.1) the close before sits ON the level: no side, so no sweep from it
            const bool sup = B[(size_t)f - 1].c > z.px;
            auto thr = [&](int k) { return sup ? B[(size_t)k].l < z.px : B[(size_t)k].h > z.px; };
            if (!thr(f)) continue;
            const int from = std::max(std::max(0, z.from), ds);   // (1.9.0) a session level: after the candle that printed it (levelsAt); (1.22.0) in the day session: from the RTH open candle
            // (1.10.0, "use strict rule") nobody TRADED through it since it formed (a wick counts); the first candle through is its one attempt
            bool before = false; for (int k = from; k < f; ++k) if (thr(k)) { before = true; break; }
            if (before) continue;
            done.insert(id);
            std::vector<int> att(1, f); while (att.back() + 1 < n && att.size() < 3 && thr(att.back() + 1)) att.push_back(att.back() + 1);
            int dpt = att[0]; for (int k : att) if (sup ? B[(size_t)k].l < B[(size_t)dpt].l : B[(size_t)k].h > B[(size_t)dpt].h) dpt = k;
            int back = -1; for (int k = dpt; k < std::min(n, dpt + 3); ++k) if (sup ? B[(size_t)k].c > z.px : B[(size_t)k].c < z.px) { back = k; break; }
            if (back < 0) continue;
            if (isOpenCandle(B[(size_t)dpt].t, openMin, spb)) continue;
            if (newsCandle(B[(size_t)dpt].t, spb, rel)) continue;   // (1.12.0, "we can skip data release for sweeps") a candle that holds a scheduled release is never a Sw   // (1.11.0, "remove opening candle as a candidate for sweeps") the RTH opening candle is never a Sw
            bool newExt = false; for (int k = dpt + 1; k <= back; ++k) if (sup ? B[(size_t)k].l < B[(size_t)dpt].l : B[(size_t)k].h > B[(size_t)dpt].h) newExt = true;
            if (newExt) continue;
            double am = 0; for (int k : att) am = std::max(am, v[(size_t)k]);
            (void)am;   // (1.8.0, Rassul 11:00 CT "shouldnt this be a sweep" - ES 11-20 09:00 ONH: the attempt's volume was under the 08:54 breakout candle's, so no Sw) no volume gate any more: his sweep rule has none ("no volume requirement", 2026-10-26); the Sw candle's own BVMax / BRMax >= 20 (fillStrength) is the strength test
            if (sup ? !(B[(size_t)dpt].c > B[(size_t)dpt].o) : !(B[(size_t)dpt].c < B[(size_t)dpt].o)) continue;   // (1.3.0, 16:42 "a bullish sweep should be green and vice versa")
            Cand c; c.px = z.px; c.name = z.name; c.hb = z.hb; c.high = z.high;
            auto it = raw.find(dpt);
            if (it != raw.end()) { if (it->second.first == 'S') { it->second.second.push_back(c); rawA[dpt] = std::min(rawA[dpt], att[0]); } continue; }
            raw[dpt] = std::make_pair('S', std::vector<Cand>(1, c)); rawDir[dpt] = sup ? 1 : -1; rawA[dpt] = att[0];
        } } }
    // (1.10.0) a Sw's label may be any KEY level its mark candle crossed (wick through it, close back on the other side), touched earlier or not:
    // the untouched level (often an hour high / low) decides THAT it is a sweep, the key level it crossed names it
    for (auto& kv : raw) { if (kv.second.first != 'S') continue; const int i = kv.first; const int d = rawDir[i]; const wyr::Bar& b = B[(size_t)i];
        for (const Lv& z : levelsAt(B, i, key, h3)) { if (z.hb != 0 || z.bkOnly || pri(z.name, 0) < 0) continue;
            const int a0 = rawA.count(i) ? rawA[i] : i; const long long pc = a0 > 0 ? B[(size_t)a0 - 1].c : b.o;   // the close before the attempt: still on the level's original side
            const bool crossed = d > 0 ? (pc > z.px && b.l < z.px && b.c > z.px) : (pc < z.px && b.h > z.px && b.c < z.px); if (!crossed) continue;
            bool have = false; for (const Cand& x : kv.second.second) if (x.px == z.px && x.name == z.name) have = true;
            if (!have) { Cand c; c.px = z.px; c.name = z.name; c.hb = 0; c.high = d < 0; kv.second.second.push_back(c); } } }
    // (1.14.0) a sweep candle that crossed NO major level is not a Sw any more: it is taken out HERE, before the Bk and Cx steps, so the candle can
    // still be the Bk / Cx it is (GC 2026-11-23 7:57 AM: a sweep of the pre-open hour's low that closed up through LonHI, AsiaHI, ONH and PFH - a Bk)
    for (auto it = raw.begin(); it != raw.end(); ) { bool mj = false; if (it->second.first == 'S') { for (const Cand& z : it->second.second) if (swMajor(z.name, z.hb) && swSideOk(z.name, rawDir[it->first])) mj = true; }   // (1.23.0) a major level OF ITS SIDE
        if (it->second.first == 'S' && noSw && noSw->count(B[(size_t)it->first].t)) mj = false;   // (1.26.0) too little volume to be a Sw (fillStrength would drop it AFTER the Cx step): free the candle now
        if (it->second.first == 'S' && !mj) { rawDir.erase(it->first); rawA.erase(it->first); it = raw.erase(it); } else ++it; }
    // Bk: closes through a level
    std::set<int> weakBk;   // (1.17.0) a Bk crossed ONLY hourly pivots / 4-hour levels: a Cx may still take its candle
    for (int i = 5; i < n; ++i) {
        { const long long r0 = B[(size_t)i].h - B[(size_t)i].l, r1 = B[(size_t)i - 1].h - B[(size_t)i - 1].l;   // (1.21.0, Rassul 2026-11-25 08:43-08:45 "volume just needs to be increasing", "or the range needs to be increasing", "for all levels") the break candle: its volume OR its range bigger than the candle before
          if (!(r0 > r1 || v[(size_t)i] > v[(size_t)i - 1])) continue; }
        std::vector<std::pair<long long, Cand> > crossed;   // distance from the close before -> level
        for (const Lv& z : levelsAt(B, i, key, h3)) {
            const long long pc = B[(size_t)i - 1].c, c = B[(size_t)i].c;
            if (!((pc > z.px && c < z.px) || (pc < z.px && c > z.px))) continue;
            { const int sd = levelSide(z.name, z.high, z.hb); const bool up = c > z.px; if ((up && sd < 0) || (!up && sd > 0)) continue; }   // (1.20.0) a bullish break is of a high, a bearish one of a low
            bool chop = false; for (int k = std::max(1, i - 10); k < i; ++k) if ((B[(size_t)k - 1].c > z.px) != (B[(size_t)k].c > z.px)) { chop = true; break; }
            if (chop) continue;
            Cand cd; cd.px = z.px; cd.name = z.name; cd.hb = z.hb; cd.high = z.high; cd.bkOnly = z.bkOnly; crossed.push_back(std::make_pair(std::llabs(z.px - pc), cd));
        }
        if (crossed.empty() || raw.count(i)) continue;
        std::stable_sort(crossed.begin(), crossed.end(), [](const std::pair<long long, Cand>& a, const std::pair<long long, Cand>& b) { return a.first > b.first; });
        std::vector<Cand> cs; for (const auto& x : crossed) { bool dup = false; for (const Cand& y : cs) if (y.name == x.second.name) dup = true; if (!dup) cs.push_back(x.second); }
        std::stable_partition(cs.begin(), cs.end(), [](const Cand& c) { return !c.bkOnly; });   // (1.17.0) the older levels first, so the cap of 4 never drops one for a new level
        if (cs.size() > 4) cs.resize(4);
        { bool weak = true; for (const Cand& c : cs) if (!c.bkOnly) weak = false; if (weak) weakBk.insert(i); }
        raw[i] = std::make_pair('B', cs); rawDir[i] = B[(size_t)i].c < B[(size_t)i - 1].c ? -1 : 1;
    }
    // Cx: pivots confirmed (ZigZag leg starts), the pivot candle or the one before, biggest volume in 20+
    { const std::vector<wyr::Leg> LG = wyr::legs(B); int prevA = -1;
      std::vector<std::pair<int, int> > piv;   // pivot candle, side (+1 low / -1 high), in order of confirmation
      for (int i = 0; i < n; ++i) { const int a = LG[(size_t)i].a; if (a != prevA && i > 0) { bool seen = false; for (const auto& p : piv) if (p.first == a) seen = true; if (!seen) piv.push_back(std::make_pair(a, LG[(size_t)i].dir == 1 ? 1 : -1)); } prevA = a; }
      for (const auto& p : piv) { const int k = p.first, side = p.second;
        for (int kk : { k, k - 1 }) {
            // (1.15.0) the 20 candles before kk: the session's own, then the ones before the session (preV)
            const int pn = preV ? (int)preV->size() : 0; if (kk < 0 || kk + pn < 20 || (raw.count(kk) && !weakBk.count(kk))) continue;   /* (1.17.0) a weak Bk (new levels only) gives way to the Cx */
            double mb = maxV(kk - 20, kk); for (int j = kk - 20; j < 0; ++j) { const int q = pn + j; if (q >= 0) mb = std::max(mb, (*preV)[(size_t)q]); }
            if (v[(size_t)kk] < mb) continue;
            const long long ext = side > 0 ? B[(size_t)k].l : B[(size_t)k].h;
            std::vector<std::pair<long long, Cand> > in;
            for (const Lv& z : levelsAt(B, kk, key, h3)) if (!z.bkOnly && B[(size_t)kk].l <= z.px && z.px <= B[(size_t)kk].h && cxHeld(B, kk, k, side, z.px)) { Cand cd; cd.px = z.px; cd.name = z.name; cd.hb = z.hb; cd.high = z.high; in.push_back(std::make_pair(std::llabs(z.px - ext), cd)); }
            std::stable_sort(in.begin(), in.end(), [](const std::pair<long long, Cand>& a, const std::pair<long long, Cand>& b) { return a.first < b.first; });
            if (in.empty()) continue;   // (1.27.0) no level inside the candle: no Cx
            std::vector<Cand> cs; for (size_t q = 0; q < in.size() && q < 4; ++q) cs.push_back(in[q].second);
            raw[kk] = std::make_pair('C', cs); rawDir[kk] = side; break; } } }
    // one level each, repeats cut (per code + level, in candle order)
    std::map<std::pair<char, std::string>, std::pair<int, long long> > last;
    for (const auto& kv : raw) {
        const int i = kv.first; Mark m; m.code = kv.second.first; m.dir = rawDir[i];
        std::vector<Cand> cs = kv.second.second;
        if (m.code == 'S') std::stable_sort(cs.begin(), cs.end(), [&](const Cand& x, const Cand& y) { if ((x.hb == 0) != (y.hb == 0)) return x.hb == 0; return m.dir > 0 ? x.px < y.px : x.px > y.px; });   // (1.9.0) a tie in priority: a named level before a candle-hour level (GC 9:00 PhrH over the prior hour), then the deepest swept (ES 9:00 ONH over AsiaHI)
        auto isRepeat = [&](const std::string& lv, long long px) {
            const auto it = last.find(std::make_pair(m.code, lv)); if (it == last.end()) return false;
            for (int k = it->second.first + 1; k < i; ++k) if (std::fabs((double)(B[(size_t)k].c - px)) >= atr[(size_t)k] && atr[(size_t)k] > 0) return false;   // went 1+ ATR away and came back
            return true; };
        if (m.code == 'S') {   // (1.9.0) the best level that is not a repeat: a sweep is never dropped because its best level was swept earlier (it names the next one)
            std::vector<Cand> ord; for (const Cand& z : cs) if (swMajor(z.name, z.hb) && swSideOk(z.name, m.dir)) ord.push_back(z);   /* (1.23.0) of its side */   // (1.14.0) only a MAJOR level names a Sw (was: any level of the list, pri >= 0); none crossed = no Sw
            std::stable_sort(ord.begin(), ord.end(), [](const Cand& x, const Cand& y) { return pri(x.name, x.hb) < pri(y.name, y.hb); });
            bool got = false; for (const Cand& z : ord) { const std::string lv = shortName(z.name, z.hb, z.high); if (isRepeat(lv, z.px)) continue; m.px = z.px; m.lv = lv; got = true; break; }
            if (!got) continue;
            last[std::make_pair(m.code, m.lv)] = std::make_pair(i, m.px);
        } else {
            Cand c; if (pick(cs, c)) { m.px = c.px; m.lv = shortName(c.name, c.hb, c.high); }
            if (!m.lv.empty()) { if (isRepeat(m.lv, m.px)) continue; last[std::make_pair(m.code, m.lv)] = std::make_pair(i, m.px); }
        }
        const auto ra = rawA.find(i); m.t0 = B[(size_t)(ra != rawA.end() ? ra->second : i)].t;   // (1.2.0)
        out[B[(size_t)i].t] = m;
    }
    return out;
}


// ---- (1.3.0) the Bk sequence: Bk -> FT -> Tst -> Rev (Rassul 2026-11-19 15:35-16:58 CT "just keep BK because it focuses on levels", "Bk and its
// Tst, which is currently the pb but for BO", "just keep it like the Pb because it works .. also have FT bar just like how we have it for BO",
// "the second tst for a bearish must be higher than the first", "the tst shouldnt take out the bearish breakout bar high", "no, it must overlap",
// "update the Tst so it also mentions the lvl"; mockup price-pane-cleanup-mockup.html). Bullish shown, bearish = mirror:
//   FT   within the 3 candles after the Bk, a GREEN candle closing above the Bk candle's high (or at it with a higher high).
//   Tst  a pivot LOW on the 3-bar High / Low ZigZag on the pullback after the Bk move topped (the first pivot high at / after the Bk); only after a
//        Bk that had its FT; the pivot candle OVERLAPS the Bk candle; it retraced 38.2% - 100% of the move (move = the last pivot low at / before
//        the Bk -> the top); it never takes out the Bk candle's LOW (if it does, that Bk gets no more Tsts); at most 2 per move and the 2nd reaches
//        further back (a lower low) than the 1st; none after a close below the move's origin or a Bk the other way; one Tst per pivot - the
//        earliest Bk of the move (the furthest-back break) keeps it and names its level. Shown "Tst / level / 62%".
//   Rev  from the Tst pivot candle on (12 candles at most), the first GREEN candle whose low is below the pivot candle's high, with range > 0.8 x
//        ATR(13) of the candle before and a close above the prior candle's high, or range >= 1.2 x ATR; when the prior candle has the same colour
//        its range must be bigger (rule 7) - the PB Rev candle rules.
// Pure function of the closed candles and the Bk marks: no repaint.
struct SMark { char code = 0; int dir = 0; std::string lv; long long px = 0; int pct = 0; long long bkT = 0; std::string st; };   // st = the Rev's strength line (filled by the host, as fillStrength)   // 'F' FT / 'T' Tst / 'R' Rev
inline const char* seqText(char c) { return c == 'F' ? "FT" : c == 'T' ? "Tst" : c == 'R' ? "Rev" : c == 'A' ? "AR" : ""; }   // (1.13.0) 'A' = the AR (arPick), written and drawn like a Rev: code + strength
inline std::map<long long, std::vector<SMark> > seq(const std::vector<wyr::Bar>& B, const std::map<long long, Mark>& marks) {
    std::map<long long, std::vector<SMark> > out; const int n = (int)B.size(); if (n < 6) return out;
    std::map<long long, int> at; for (int i = 0; i < n; ++i) at[B[(size_t)i].t] = i;
    const std::vector<double> atr = wyr::atr13(B);
    std::vector<std::pair<int, int> > piv;   // pivot candle, side (+1 low / -1 high), in order of confirmation
    { const std::vector<wyr::Leg> LG = wyr::legs(B); int prevA = -1;
      for (int i = 0; i < n; ++i) { const int a = LG[(size_t)i].a; if (a != prevA && i > 0) { bool seen = false; for (const auto& p : piv) if (p.first == a) seen = true; if (!seen) piv.push_back(std::make_pair(a, LG[(size_t)i].dir == 1 ? 1 : -1)); } prevA = a; } }
    std::vector<std::pair<int, const Mark*> > bks; for (const auto& kv : marks) if (kv.second.code == 'B') { const auto it = at.find(kv.first); if (it != at.end()) bks.push_back(std::make_pair(it->second, &kv.second)); }
    std::set<int> tstAt; std::set<std::pair<int, char> > put;
    auto add = [&](int i, char code, int dir, const std::string& lv, long long px, int pct, long long bkT) {
        if (put.count(std::make_pair(i, code))) return;
        put.insert(std::make_pair(i, code));
        SMark m; m.code = code; m.dir = dir; m.lv = lv; m.px = px; m.pct = pct; m.bkT = bkT; out[B[(size_t)i].t].push_back(m); };
    for (const auto& bk : bks) {
        const int k = bk.first, dr = bk.second->dir;
        bool ft = false;
        for (int j = k + 1; j < std::min(n, k + 4); ++j) { const wyr::Bar& b = B[(size_t)j];
            if (dr > 0 && b.c > b.o && (b.c > B[(size_t)k].h || (b.c == B[(size_t)k].h && b.h > B[(size_t)k].h))) { add(j, 'F', 1, "", 0, 0, B[(size_t)k].t); ft = true; break; }
            if (dr < 0 && b.c < b.o && (b.c < B[(size_t)k].l || (b.c == B[(size_t)k].l && b.l < B[(size_t)k].l))) { add(j, 'F', -1, "", 0, 0, B[(size_t)k].t); ft = true; break; } }
        if (!ft) continue;
        int oi = -1; for (const auto& p : piv) if (p.first <= k && p.second == dr) oi = p.first;
        if (oi < 0) continue;
        const long long origin = dr > 0 ? B[(size_t)oi].l : B[(size_t)oi].h;
        int ti = -1; for (const auto& p : piv) if (p.first >= k && p.second == -dr) { ti = p.first; break; }
        if (ti < 0) continue;
        const long long top = dr > 0 ? B[(size_t)ti].h : B[(size_t)ti].l;
        int nt = 0; long long firstExt = 0;
        for (const auto& p : piv) {
            if (p.first <= ti || p.second != dr) continue;
            if (nt >= 2) break;
            const int pi = p.first;
            bool stop = false; for (int j = k; j <= pi; ++j) if (dr > 0 ? B[(size_t)j].c < origin : B[(size_t)j].c > origin) { stop = true; break; }
            if (stop) break;
            for (const auto& o : bks) if (o.second->dir == -dr && o.first > k && o.first <= pi) { stop = true; break; }
            if (stop) break;
            const bool overl = B[(size_t)pi].l <= B[(size_t)k].h && B[(size_t)pi].h >= B[(size_t)k].l;
            const long long ext = dr > 0 ? B[(size_t)pi].l : B[(size_t)pi].h;
            const double ret = top != origin ? (double)(top - ext) / (double)(top - origin) : 0.0;
            if (!overl || ret < 0.382 || ret > 1.0) continue;
            if (dr < 0 ? B[(size_t)pi].h > B[(size_t)k].h : B[(size_t)pi].l < B[(size_t)k].l) break;   // took out the Bk candle's extreme: the break failed
            if (nt == 1 && !(dr < 0 ? ext > firstExt : ext < firstExt)) continue;                      // the 2nd reaches further back
            if (nt == 0) firstExt = ext;
            ++nt;
            if (tstAt.count(pi)) continue;                                                             // the earliest Bk of the move keeps it (still counted here)
            tstAt.insert(pi);
            add(pi, 'T', dr, bk.second->lv, bk.second->px, (int)std::lround(ret * 100.0), B[(size_t)k].t);
            for (int j = pi; j < std::min(n, pi + 12); ++j) { if (j < 1) continue; const wyr::Bar& b = B[(size_t)j]; const wyr::Bar& q = B[(size_t)j - 1];
                if (!(dr > 0 ? b.c > b.o : b.c < b.o)) continue;
                const bool back = dr > 0 ? b.l < B[(size_t)pi].h : b.h > B[(size_t)pi].l;
                const double rng = (double)(b.h - b.l), a = atr[(size_t)j - 1];
                const bool beyond = dr > 0 ? b.c > q.h : b.c < q.l;
                const bool same = (q.c > q.o) == (b.c > b.o); const bool r7 = !same || rng > (double)(q.h - q.l);
                if (back && r7 && ((rng > 0.8 * a && beyond) || rng >= 1.2 * a)) { add(j, 'R', dr, "", 0, 0, B[(size_t)k].t); break; } }
        }
    }
    return out;
}

// (1.2.0) the strength line: his BVMax / BRMax, exactly as the Volume / Range panes count it (VolRangeLogic.h Engine::statFor): for candle i,
// BV = how many candles in a row before it were SMALLER (stops at the first as big or bigger), BVG / BVR = the same over the candles of its own
// colour only; Max = the larger; counts stop at 99 ("99+"), look back at most 1,500 candles. V = volume, R = range in whole ticks.
static const int ST_LOOK = 1500, ST_CAP = 99, ST_MIN = 10;   // (1.4.0) (Rassul 2026-11-19 18:19 CT "switch to 10": the strength line shows from 10 up, was 20)
inline int stCount(const std::vector<double>& V, const std::vector<char>& G, int i) {
    if (i < 0 || i >= (int)V.size() || !(V[(size_t)i] > 0)) return 0;
    const double x = V[(size_t)i]; const bool g = G[(size_t)i] != 0; int all = 0, same = 0; bool da = false, ds = false;
    for (int j = i - 1; j >= std::max(0, i - ST_LOOK) && !(da && ds); --j) {
        const bool big = V[(size_t)j] >= x;
        if (!da) { if (big || all >= ST_CAP) da = true; else ++all; }
        if (!ds && (G[(size_t)j] != 0) == g) { if (big || same >= ST_CAP) ds = true; else ++same; }
    }
    return std::max(all, same);
}
// (1.24.1, Rassul 16:04 CT "the vob / vot strength requires BV 20+") BV = how many candles in a row before i had SMALLER volume, any colour (stops at the
// first as big or bigger; at most 99, look back at most ST_LOOK) - the volume reader's BV, not BVMax
inline int bvCount(const std::vector<double>& V, int i) {
    if (i < 0 || i >= (int)V.size() || !(V[(size_t)i] > 0)) return 0;
    int n = 0; for (int j = i - 1; j >= std::max(0, i - ST_LOOK) && n < ST_CAP; --j) { if (V[(size_t)j] >= V[(size_t)i]) break; ++n; }
    return n;
}
// the strength over candles a..b (a Sw's attempt; a Bk: a == b): "V<N>" / "R<N>" for the larger count (volume wins a tie), "" under 20
inline std::string strength(const std::vector<double>& vol, const std::vector<double>& rng, const std::vector<char>& green, int a, int b) {
    int nv = 0, nr = 0; if (a < 0) a = 0;
    for (int k = a; k <= b && k < (int)vol.size(); ++k) { nv = std::max(nv, stCount(vol, green, k)); if (k < (int)rng.size()) nr = std::max(nr, stCount(rng, green, k)); }
    const int n = std::max(nv, nr); if (n < ST_MIN) return std::string();
    return std::string(nv >= nr ? "V" : "R") + (n >= ST_CAP ? std::string("99+") : std::to_string(n));
}
// (1.2.0, Rassul 11:35 CT "will you add this strength line to the I also") the strength of ONE candle by its stamp (the I candle); "" = none / under 20
inline std::string strengthAt(const std::vector<long long>& T, const std::vector<double>& V, const std::vector<double>& R, const std::vector<char>& G, long long t) {
    const auto it = std::lower_bound(T.begin(), T.end(), t); if (it == T.end() || *it != t) return std::string();
    const int i = (int)(it - T.begin()); return strength(V, R, G, i, i);
}
// the range value as the Range pane counts it: high - low in whole ticks (0 = none)
inline double rangeTicks(double h, double l, double tick) { const double r = h - l; if (!std::isfinite(r) || r <= 0) return 0.0; return tick > 0 ? (double)std::llround(r / tick) : r; }
// fill the strength of every Bk / Sw in M from the chart's closed candles (T = candle stamps, ascending; V volume; R range ticks; G green)
// (1.5.0, Rassul 2026-11-19 18:29 CT "lets keep it at 10 so the BRmax or BVmax should be at least 10") a Sw only counts when its attempt's
// BVMax or BRMax (max over the attempt candles, the strength line's measure) is at least SW_MIN; a weaker Sw is removed here
// (1.6.0, Rassul 2026-11-20 07:50 CT "update the sweep threshold to 20") SW_MIN 10 -> 20; ST_MIN (the strength line) stays 10
static const int SW_MIN = 99;   // (1.25.0, "we should use 99+ bvmax") BVMax 99+ (the count is capped at 99 = "99+"); was 20
inline int strengthN(const std::vector<double>& vol, const std::vector<double>& rng, const std::vector<char>& green, int a, int b) {
    int nv = 0, nr = 0; if (a < 0) a = 0;
    for (int k = a; k <= b && k < (int)vol.size(); ++k) { nv = std::max(nv, stCount(vol, green, k)); if (k < (int)rng.size()) nr = std::max(nr, stCount(rng, green, k)); }
    return std::max(nv, nr);
}
// (1.26.0) the closed candles that can NOT be a Sw: own BVMax (stCount, volume) under SW_MIN - the same test fillStrength applies, made BEFORE run()
// so a failed sweep candle does not block its Cx / Bk. T ascending stamps, V volume, G green.
inline std::set<long long> swWeak(const std::vector<long long>& T, const std::vector<double>& V, const std::vector<char>& G) {
    std::set<long long> out; for (int i = 0; i < (int)T.size() && i < (int)V.size() && i < (int)G.size(); ++i) if (stCount(V, G, i) < SW_MIN) out.insert(T[(size_t)i]);
    return out;
}
inline int fillStrength(std::map<long long, Mark>& M, const std::vector<long long>& T, const std::vector<double>& V, const std::vector<double>& R, const std::vector<char>& G) {
    int n = 0;
    auto at = [&](long long t) { const auto it = std::lower_bound(T.begin(), T.end(), t); return it != T.end() && *it == t ? (int)(it - T.begin()) : -1; };
    for (auto it = M.begin(); it != M.end(); ) { Mark& m = it->second; m.st.clear(); if (m.code != 'B' && m.code != 'S') { ++it; continue; }
        const int b = at(it->first); if (b < 0) { ++it; continue; } int a = m.t0 > 0 ? at(m.t0) : b; if (a < 0 || a > b) a = b;
        if (m.code == 'S') a = b;   // (1.8.0, Rassul 10:35 CT "the red sweep bar doesnt meet the threshold.. fix") a Sw is judged and labelled by ITS OWN candle, not the attempt's biggest
        if (m.code == 'S') {   // (1.24.0, Rassul 13:56 CT "the bigest volume in at least 20 bars") VOLUME only: the Sw candle's BVMax >= SW_MIN (was BVMax or BRMax)
            const int nv = stCount(V, G, b); if (nv < SW_MIN) { it = M.erase(it); continue; }   // (1.24.2) BVMax again (1.24.1 used bvCount = BV)
            m.st = std::string("V") + (nv >= ST_CAP ? std::string("99+") : std::to_string(nv)); ++n; ++it; continue; }
        m.st = strength(V, R, G, a, b); if (!m.st.empty()) ++n; ++it; }
    return n;
}


// ---- (1.13.0) THE AR - the bar that answers a climax (Rassul 2026-11-23 09:25-09:34 CT, quoted at VERSION; mockup ar-mockup.html). Bullish shown, bearish = mirror:
//   1 there is a DRAWN Cx (a mark of run(), dir +1 = at a pivot low: the climax of a down wave);
//   2 the AR is one of the AR_BARS (3) candles after the PIVOT candle (the Cx sits on the pivot candle or on the candle before it; the pivot candle itself is
//     not an AR - asked 09:30 CT, not answered before the build);
//   3 it is GREEN (close above open);
//   4 it overlaps the pivot candle: its low is below the pivot candle's high;
//   5 its BRMax (the Range pane's count: stCount on the range in ticks) is AR_MIN (10) or more.
//   The first candle that passes is the AR; one per Cx; one AR per candle. Strength line = the Bk / Sw line (V / R with the larger count).
// Two steps because the candles' colour and overlap come from the session's own candles (WB, as run() / seq()) and BRMax from the chart's closed candles (the
// vectors of fillStrength). Both are pure functions of closed candles: a candidate's BRMax is fixed once it has closed, candidates are tried in order, so the
// AR appears once (when its candle has closed and the pivot is confirmed) and never changes - no repaint.
// (1.14.0, Rassul 2026-11-23 14:00 CT "if the Green bar is bigger than the ATR and its high is above the cmx bar with the hevy volume , it is also valid AR"; 14:16
// "and what is the other range that is going in  for the AR tag" - answered: either way is enough; "build") rule 5 has a SECOND way in: the candle's range is
// bigger than ATR(13) of the candle before it (wyr::atr13, the PB Rev's ATR) AND its high is above the Cx CANDLE's high (bearish: its low below the Cx candle's
// low). Rules 1 - 4 stay for both ways. Strength line: the V / R count when there is one (10+), else the range against that ATR ("1.4x").
static const int AR_MIN = 10, AR_BARS = 3;
struct ArCand { long long cxT = 0, pivT = 0; int dir = 0; std::vector<long long> c; std::vector<long long> x; };   // x (1.14.0) = per candidate: range / ATR x 1000 when the ATR way passes, else -1   // one Cx: its candle, its pivot candle, its side, and the candles after the pivot with the colour and the overlap (in order)
inline std::vector<ArCand> arCands(const std::vector<wyr::Bar>& B, const std::map<long long, Mark>& marks) {
    std::vector<ArCand> out; const int n = (int)B.size(); if (n < 3) return out;
    std::map<long long, int> at; for (int i = 0; i < n; ++i) at[B[(size_t)i].t] = i;
    std::vector<std::pair<int, int> > piv;   // pivot candle, side (+1 low / -1 high) - the pivots of run() and seq()
    { const std::vector<wyr::Leg> LG = wyr::legs(B); int prevA = -1;
      for (int i = 0; i < n; ++i) { const int a = LG[(size_t)i].a; if (a != prevA && i > 0) { bool seen = false; for (const auto& p : piv) if (p.first == a) seen = true; if (!seen) piv.push_back(std::make_pair(a, LG[(size_t)i].dir == 1 ? 1 : -1)); } prevA = a; } }
    const std::vector<double> atr = wyr::atr13(B);
    for (const auto& kv : marks) { if (kv.second.code != 'C' || (kv.second.dir != 1 && kv.second.dir != -1)) continue;
        const auto it = at.find(kv.first); if (it == at.end()) continue; const int kk = it->second, d = kv.second.dir;
        int k = -1; for (const auto& p : piv) { if (p.second != d) continue; if (p.first == kk) { k = kk; break; } if (p.first == kk + 1) k = kk + 1; }   // the Cx candle is the pivot, or the candle before it
        if (k < 0) continue;
        ArCand a; a.cxT = kv.first; a.pivT = B[(size_t)k].t; a.dir = d;
        for (int j = k + 1; j < n && j <= k + AR_BARS; ++j) { const wyr::Bar& b = B[(size_t)j];
            const bool colour = d > 0 ? b.c > b.o : b.c < b.o; const bool over = d > 0 ? b.l < B[(size_t)k].h : b.h > B[(size_t)k].l;
            if (!(colour && over)) continue;
            const double rng = (double)(b.h - b.l), a0 = j >= 1 && (size_t)j - 1 < atr.size() ? atr[(size_t)j - 1] : 0.0;
            const bool beyond = d > 0 ? b.h > B[(size_t)kk].h : b.l < B[(size_t)kk].l;
            a.c.push_back(b.t); a.x.push_back(std::isfinite(a0) && a0 > 0 && rng > a0 && beyond ? (long long)std::llround(std::min(99.0, rng / a0) * 1000.0) : -1LL); }
        out.push_back(a); }
    return out;
}
// the AR of every Cx still drawn in M (after fillStrength): candle stamp -> SMark 'A' (dir = the Cx's, bkT = the Cx candle, st = the strength line)
inline std::map<long long, SMark> arPick(const std::vector<ArCand>& cands, const std::map<long long, Mark>& M, const std::vector<long long>& T, const std::vector<double>& V, const std::vector<double>& R, const std::vector<char>& G) {
    std::map<long long, SMark> out; if (R.size() != T.size() || G.size() != T.size()) return out;
    for (const ArCand& a : cands) { const auto m = M.find(a.cxT); if (m == M.end() || m->second.code != 'C' || m->second.dir != a.dir) continue;
        for (size_t q = 0; q < a.c.size(); ++q) { const long long t = a.c[q]; const long long ax = q < a.x.size() ? a.x[q] : -1;
            const auto it = std::lower_bound(T.begin(), T.end(), t); if (it == T.end() || *it != t) continue; const int i = (int)(it - T.begin());
            if (stCount(R, G, i) < AR_MIN && ax < 0) continue;   // (1.14.0) BRMax 10+ OR the ATR way
            if (!out.count(t)) { SMark s; s.code = 'A'; s.dir = a.dir; s.bkT = a.cxT; s.st = strength(V, R, G, i, i);
                if (s.st.empty() && ax >= 0) { char b2[16]; std::snprintf(b2, sizeof b2, "%.1fx", (double)ax / 1000.0); s.st = b2; }
                out[t] = s; }
            break; } }
    return out;
}

// (1.1.0) the one level AT candle i for another indicator's tag (the BO indicator's Rly): a level inside the candle's range or within tol ticks of
// its extreme ext; same priority / short names as the marks; false = none
inline bool levelNear(const std::vector<wyr::Bar>& B, int i, const std::vector<wyr::Lvl>& key, long long ext, long long tol, std::string& shortOut, long long& pxOut) {
    if (i < 0 || i >= (int)B.size()) return false;
    std::vector<std::pair<long long, Cand> > in;
    for (const Lv& z : levelsAt(B, i, key)) {
        const bool inside = B[(size_t)i].l <= z.px && z.px <= B[(size_t)i].h, closeBy = std::llabs(z.px - ext) <= tol;   // (never "near": a Windows macro)
        if (!inside && !closeBy) continue;
        Cand c; c.px = z.px; c.name = z.name; c.hb = z.hb; c.high = z.high; in.push_back(std::make_pair(std::llabs(z.px - ext), c)); }
    std::stable_sort(in.begin(), in.end(), [](const std::pair<long long, Cand>& a, const std::pair<long long, Cand>& b) { return a.first < b.first; });
    std::vector<Cand> cs; for (const auto& x : in) cs.push_back(x.second);
    Cand c; if (!pick(cs, c)) return false;
    shortOut = shortName(c.name, c.hb, c.high); pxOut = c.px; return true;
}

// the I's level as a short name ("" when its level does not label marks)
// (1.7.0) a level the I (and Bk / Sw) may use: a NAMED key level (0.19.4 "I should not print if there is no level") - no VWAP / its bands (they
// move), no HBO line, no MenthorQ / gamma names, no merged "a|b" names; px > 0
inline bool keyForI(const std::string& name, long long px) { return px > 0 && name.compare(0, 4, "VWAP") != 0 && name != "the HBO test line" && pri(name, 0) >= 0 && name.find('|') == std::string::npos; }
// (1.7.0) the I's (and Sp / Ut's) level list leaves out the level FILE's PHH / PHL: the file holds the prior hour as of NOW, and the live session
// is re-read every candle, so an earlier I tied to it would vanish when the hour turns (a repaint). The prior hours' highs / lows still count -
// WyRead takes them from the candles themselves (no look-ahead), with the same short names (PHH / PHL / 2hH ...).
inline bool levelForI(const std::string& name, long long px) { return keyForI(name, px) && name != "PHH" && name != "PHL"; }
// (1.7.0) the level an I names: the reclaimed level with the best Bk / Sw priority (lower pri first); a tie = the furthest reclaimed (the
// highest for a bullish I, side +1). Unnamed levels (MenthorQ, gamma, VWAP ...) never label it. false = no named level (no I).
// (1.8.0, Rassul 10:37 CT "I should only be on key levels that are named like overnight etc..") an I's level: session (ON / London / Asia),
// PD, PF, VAH / VAL, weekly - priority 0..40; the hour levels (PHH / PHL, 2h / 3h back, priority 50 / 60) never name an I
inline bool iKey(int p) { return p >= 0 && p < 50; }
// (1.8.0, "we should have a 20 threshold just like sweeps") the I candle's own BVMax or BRMax must be at least I_MIN
static const int I_MIN = 20;
inline bool iPick(const std::vector<wyr::Lvl>& c, int side, wyr::Lvl& out) {
    int best = -1; bool got = false;
    for (const wyr::Lvl& z : c) { const int p = pri(z.name, z.hb); if (!iKey(p)) continue;   // (1.8.0) no hour level
        if (!got || p < best || (p == best && (side > 0 ? z.px > out.px : z.px < out.px))) { out = z; best = p; got = true; } }
    return got;
}
// (1.7.0) the I candle is also a Sw of the same direction: only the Sw is drawn (badge and strip); the I is still recorded (flagged)
inline bool iUnderSw(const std::map<long long, Mark>& lv, long long t, int dir) { const auto it = lv.find(t); return it != lv.end() && it->second.code == 'S' && it->second.dir == dir; }
inline std::string iLevel(const std::string& name, int hb, bool high) { if (pri(name, hb) < 0) return ""; return shortName(name, hb, high); }
// (1.16.0) THE LIVE Cx? - the candle that WILL be the Cx if the running leg's extreme holds: the same test as the Cx in run() (the leg's
// extreme candle or the one before it, the biggest volume of the 20 candles before it, preV reaching before the session), made on the
// last CLOSED candle instead of when the 3-bar ZigZag confirms the pivot. It is not a decided mark: it moves or goes when a later candle
// makes a new extreme or fails the test, and the real Cx replaces it once the pivot is confirmed (a candle that is already a mark of
// run() - Sw / Bk / Cx - gets no Cx?). Never recorded. Returns {candle stamp, side +1 at a low / -1 at a high} or {0, 0}.
inline std::pair<long long, int> liveCx(const std::vector<wyr::Bar>& B, const std::map<long long, Mark>& marks, const std::vector<double>* preV = nullptr, const std::vector<wyr::Lvl>* key = nullptr, const std::vector<H3>* h3 = nullptr) {   // (1.27.0) key / h3 given = the Cx? also needs a level inside its candle
    const int n = (int)B.size(); if (n < 6) return std::make_pair(0LL, 0);
    std::vector<double> v((size_t)n); for (int i = 0; i < n; ++i) v[(size_t)i] = (double)(B[(size_t)i].buy + B[(size_t)i].sell);
    int zd = B[1].h >= B[0].h ? 1 : -1, zext = 0;                      // = wyr::legs, to the last candle: the running leg and its extreme
    for (int i = 1; i < n; ++i) {
        long long pl = B[(size_t)i - 1].l, ph = B[(size_t)i - 1].h; for (int j = std::max(0, i - 3); j < i; ++j) { pl = std::min(pl, B[(size_t)j].l); ph = std::max(ph, B[(size_t)j].h); }
        if (zd == 1) { if (B[(size_t)i].h > B[(size_t)zext].h) zext = i; else if (B[(size_t)i].l < pl && i != zext) { zext = i; zd = -1; } }
        else         { if (B[(size_t)i].l < B[(size_t)zext].l) zext = i; else if (B[(size_t)i].h > ph && i != zext) { zext = i; zd = 1; } }
    }
    const int side = zd == 1 ? -1 : 1;                                  // an up leg ends at a high (side -1), a down leg at a low (+1)
    const int pn = preV ? (int)preV->size() : 0;
    for (int kk : { zext, zext - 1 }) {
        if (kk < 0 || kk + pn < 20 || marks.count(B[(size_t)kk].t)) continue;
        double mb = 0; for (int k = std::max(0, kk - 20); k < kk; ++k) mb = std::max(mb, v[(size_t)k]);
        for (int j = kk - 20; j < 0; ++j) { const int q = pn + j; if (q >= 0) mb = std::max(mb, (*preV)[(size_t)q]); }
        if (v[(size_t)kk] < mb || v[(size_t)kk] <= 0) continue;
        if (key) { bool lv = false; for (const Lv& z : levelsAt(B, kk, *key, h3)) if (!z.bkOnly && B[(size_t)kk].l <= z.px && z.px <= B[(size_t)kk].h && cxHeld(B, kk, zext, side, z.px)) { lv = true; break; } if (!lv) continue; }   // (1.27.0) + (1.28.0) closed at / inside the level up to the running extreme
        return std::make_pair(B[(size_t)kk].t, side);
    }
    return std::make_pair(0LL, 0);
}

// ---- (1.18.0) THE ABS BADGE - effort without result at the first test of a Bk level (Rassul 2026-11-24 09:00-11:12 CT: "absorption levels should
// match bk levels", "In the bade the 2nd line will be candle location and the third line will be biggest print", "The badge can be updated like
// ABS/Ex when there is evidence of exhaustion", "ok. build this"; research ABS-ORDERBOOK / ABS-EXHAUSTION 2026-11-24, mockup abs-badge-mockup.html).
// The research rule, ported line by line (absx/tests_bk.py + ways.py, 43 sessions x 7 markets: 57% vs 49% at 1R, +6 / +12 on both halves, n 187):
//   candle i in the research RTH window (candle END: ES / NQ 8:33-15:00, CL / NG 8:03-13:30, GC 7:23-12:30, HG 7:13-12:00, EU 7:23-14:00)
//   a level of levelsAt(i) (the Bk set) inside the end quarter widened by tol = max(1 tick, 0.15 x ATR13 of the candle before):
//     bullish (a low tested): lo - tol <= level <= lo + zone + tol and level <= close; bearish: hi - zone - tol <= level <= hi + tol and level >= close
//   the FIRST test of that level today: no earlier RTH candle of the day came within tol of it ([lo - tol, hi + tol])
//   effort without result: volume >= 1.5x the mean of the 20 candles before (10+ needed), range <= 1.3x theirs, closed in the half away
//     (bullish (c - l) / (h - l) > 0.5, bearish (h - c) / (h - l) > 0.5)
//   line 2 = where the heaviest price of that end quarter sits: "Low 21%" = (qBot - lo) / (hi - lo), "Top 21%" = (hi - qTop) / (hi - lo)
// THE NEXT-CANDLE Ex (ABS-EXHAUSTION part 2: 32% exhausted vs 13%, held 77% vs 59%, both halves, n 152): the candle after the Abs - the
//   attackers (sellers after a bullish Abs) traded <= 0.5x their lots in the Abs candle, it closed further away (beyond the Abs close) and did
//   not trade past the Abs extreme by more than 1 tick. Drawn "Ex" on that candle; decided at its close, never changed.
struct AbsMk { int dir = 0; std::string lv; long long lvPx = 0; int pos = -1; bool ex = false; long long exT = 0; };
inline bool absWindow(const std::string& mkt, long long t, int& endMin) {
    int a = -1, b = -1;
    if (mkt == "ES" || mkt == "NQ" || mkt == "MES" || mkt == "MNQ") { a = 8 * 60 + 33; b = 15 * 60; }
    else if (mkt == "CL" || mkt == "NG" || mkt == "MCL") { a = 8 * 60 + 3; b = 13 * 60 + 30; }
    else if (mkt == "GC" || mkt == "MGC") { a = 7 * 60 + 23; b = 12 * 60 + 30; }
    else if (mkt == "HG") { a = 7 * 60 + 13; b = 12 * 60; }
    else if (mkt == "EU" || mkt == "6E") { a = 7 * 60 + 23; b = 14 * 60; }
    endMin = b; if (a < 0) return false;
    long long q = t % 86400; if (q < 0) q += 86400; const int m = (int)(q / 60);
    return m >= a && m <= b;
}
inline std::map<long long, AbsMk> absMarks(const std::vector<wyr::Bar>& B, const std::vector<wyr::Lvl>& key, const std::string& mkt, const std::vector<H3>* h3 = nullptr, const std::vector<double>* preV = nullptr, const std::vector<double>* preR = nullptr) {
    std::map<long long, AbsMk> out; const int n = (int)B.size(); if (n < 2) return out;
    const std::vector<double> atr = wyr::atr13(B);
    std::vector<double> V, R; const size_t np = preV ? preV->size() : 0;   // the candles before the session first (look-back only), then B
    for (size_t k = 0; k < np; ++k) { V.push_back((*preV)[k]); R.push_back(preR && preR->size() == np ? (*preR)[k] : -1.0); }
    for (const wyr::Bar& b : B) { V.push_back((double)(b.buy + b.sell)); R.push_back((double)(b.h - b.l)); }
    auto mean20 = [](const std::vector<double>& x, size_t at, double& m) { double s = 0; int c = 0; for (size_t k = at; k > 0 && c < 20; --k) { const double v = x[k - 1]; if (v < 0) continue; s += v; ++c; } if (c < 10) return false; m = s / c; return true; };
    std::set<std::pair<long long, std::string> > touched; long long day = -1; int endMin = 0;
    for (int i = 1; i < n; ++i) {
        const wyr::Bar& b = B[(size_t)i]; long long d = b.t >= 0 ? b.t / 86400 : -((-b.t + 86399) / 86400);
        if (!absWindow(mkt, b.t, endMin)) continue;
        if (d != day) { day = d; touched.clear(); }
        const double a = atr[(size_t)i - 1]; const double tol = std::max(1.0, 0.15 * a);
        const std::vector<Lv> L = levelsAt(B, i, key, h3);
        const long long lv = b.h - b.l + 1; const long long z = lv >= 4 ? std::max(1LL, (lv + 2) / 4) : 0;
        if (z > 0 && a > 0) {
            const size_t at = np + (size_t)i; double mv = 0, mr = 0;
            const bool ok = mean20(V, at, mv) && mean20(R, at, mr) && mv > 0 && mr > 0;
            const double volr = ok ? V[at] / mv : 0, rngr = ok ? R[at] / mr : 99, rg = (double)(b.h - b.l);
            for (int side : { 1, -1 }) {
                const double cp = rg > 0 ? (side > 0 ? (double)(b.c - b.l) / rg : (double)(b.h - b.c) / rg) : 0.5;
                if (!(ok && volr >= 1.5 && rngr <= 1.3 && cp > 0.5)) continue;
                const double b0 = side > 0 ? (double)b.l - tol : (double)(b.h - z) - tol, b1 = side > 0 ? (double)(b.l + z) + tol : (double)b.h + tol;
                std::vector<Cand> cs;
                for (const Lv& q : L) { if (q.px <= 0) continue;
                    if (side > 0 ? q.px > b.c : q.px < b.c) continue;
                    if ((double)q.px < b0 || (double)q.px > b1) continue;
                    if (touched.count(std::make_pair(q.px, shortName(q.name, q.hb, q.high)))) continue;   // keyed by the SHORT name as the research was ("2hL": an hour level renamed by the clock is a new level)
                    Cand c; c.px = q.px; c.name = q.name; c.hb = q.hb; c.high = q.high; c.bkOnly = q.bkOnly; cs.push_back(c); }
                Cand best; if (!pick(cs, best)) continue;
                AbsMk m; m.dir = side; m.lvPx = best.px; m.lv = shortName(best.name, best.hb, best.high);
                if (rg > 0) { const long long qp = side > 0 ? b.qBot : b.qTop;
                    if (qp > 0 && qp >= b.l && qp <= b.h) m.pos = (int)std::llround(100.0 * (side > 0 ? (double)(qp - b.l) : (double)(b.h - qp)) / rg); }
                if (i + 1 < n) { const wyr::Bar& x = B[(size_t)i + 1];
                    const long long att0 = side > 0 ? b.sell : b.buy, att1 = side > 0 ? x.sell : x.buy;
                    const bool held = side > 0 ? x.l >= b.l - 1 : x.h <= b.h + 1;
                    if (att0 > 0 && 2 * att1 <= att0 && (x.c - b.c) * side > 0 && held) { m.ex = true; m.exT = x.t; } }
                out[b.t] = m; break; } }
        for (const Lv& q : L) if (q.px > 0 && (double)b.l - tol <= (double)q.px && (double)q.px <= (double)b.h + tol) touched.insert(std::make_pair(q.px, shortName(q.name, q.hb, q.high)));
    }
    (void)endMin; return out;
}
inline std::string absPos(const AbsMk& m) { if (m.pos < 0) return std::string(); return std::string(m.dir > 0 ? "Low " : "Top ") + std::to_string(m.pos) + "%"; }

// (1.19.0, Rassul 2026-11-24 12:21 CT "use the third line which currently has the percentage .. show mockup", 12:51 "2. do the test"; mockup
// tst-line3-mockup.html; research research-artifacts/setups/TST-REVERSE-2026-11-24.md, 943 Tst / 697 decided, his PB Rev win rule on the Tst):
// the Tst badge's 3rd line says whether the pullback's attackers were ABSORBED and then EXHAUSTED at the Tst pivot candle:
//   A = absorption on the pivot candle (the attackers = the pullback's side: sellers into a bullish Tst's low, buyers into a bearish Tst's high):
//       effort without result - volume >= 1.5x the 20 candles before (10+), range <= 1.3x, closed in the far half (> 0.5) - OR the Turn pane's
//       Absorption bar - the lots at the heaviest single attacking price inside the end quarter >= 2.0x the same over the 20 candles before
//       (10+ with 4+ price levels), with the close outside that quarter
//   E = exhaustion: the attackers' last minute <= 0.5x the pace of their first 2 minutes (lsTurn's Ex?, from the order feed: exq) OR the next
//       candle - attackers <= 0.5x the pivot candle's, closed further the Tst's way, traded no more than 1 tick past the pivot's extreme
//   "A+E" both (75% win, +0.33R, n 60, +0.17 / +0.59 on the two halves), "E" exhaustion only (+0.02R), "A!" absorption without exhaustion
//   = a warning (-0.19R on both halves), "" neither (+0.07R). LEARNING: 4-16 a market.
// Decided once the candle after the pivot has closed (the Tst is drawn only after its pivot is confirmed, so it always has); the host keeps
// the first decision (no repaint). exq: candle stamp -> {sellers' Ex?, buyers' Ex?} (0 / 1), absent = not known.
struct PreQ { long long top = -1, bot = -1; };   // the candles before the session: the lots at the quarter prices (-1 = not known)
inline std::string l3Text(bool a, bool e) { return a && e ? "A+E" : e ? "E" : a ? "A!" : std::string(); }
inline std::map<long long, std::string> tstL3(const std::vector<wyr::Bar>& B, const std::map<long long, std::vector<SMark> >& sq,
        const std::map<long long, std::pair<int, int> >* exq = nullptr, const std::vector<double>* preV = nullptr, const std::vector<double>* preR = nullptr,
        const std::vector<PreQ>* preQ = nullptr, bool decideLast = false) {
    std::map<long long, std::string> out; const int n = (int)B.size(); if (n < 2) return out;
    std::map<long long, int> at; for (int i = 0; i < n; ++i) at[B[(size_t)i].t] = i;
    std::vector<double> V, R; std::vector<long long> QT, QB; const size_t np = preV ? preV->size() : 0;
    for (size_t k = 0; k < np; ++k) { V.push_back((*preV)[k]); R.push_back(preR && preR->size() == np ? (*preR)[k] : -1.0);
        QT.push_back(preQ && preQ->size() == np ? (*preQ)[k].top : -1); QB.push_back(preQ && preQ->size() == np ? (*preQ)[k].bot : -1); }
    for (const wyr::Bar& b : B) { V.push_back((double)(b.buy + b.sell)); R.push_back((double)(b.h - b.l));
        const bool q4 = b.h - b.l + 1 >= 4; QT.push_back(q4 ? b.qTopLots : -1); QB.push_back(q4 ? b.qBotLots : -1); }
    auto mean20 = [](const std::vector<double>& x, size_t a, double& m) { double s = 0; int c = 0; for (size_t k = a; k > 0 && c < 20; --k) { const double v = x[k - 1]; if (v < 0) continue; s += v; ++c; } if (c < 10) return false; m = s / c; return true; };
    for (const auto& kv : sq) for (const SMark& m : kv.second) {
        if (m.code != 'T') continue;
        const auto it = at.find(kv.first); if (it == at.end()) continue;
        const int pi = it->second, dr = m.dir; if (pi + 1 >= n && !decideLast) continue;   // not decided before the candle after the pivot closed
        const wyr::Bar& b = B[(size_t)pi]; const size_t ai = np + (size_t)pi; const double rg = (double)(b.h - b.l);
        // A: effort without result (the absMarks rule)
        bool a = false; double mv = 0, mr = 0;
        if (mean20(V, ai, mv) && mean20(R, ai, mr) && mv > 0 && mr > 0) {
            const double cp = rg > 0 ? (dr > 0 ? (double)(b.c - b.l) / rg : (double)(b.h - b.c) / rg) : 0.5;
            a = V[ai] / mv >= 1.5 && R[ai] / mr <= 1.3 && cp > 0.5; }
        // A: the Absorption bar (heaviest attacking price in the end quarter vs the 20 candles before, 10+ known; closed outside the quarter)
        const std::vector<long long>& Q = dr > 0 ? QB : QT; const long long L = b.h - b.l + 1;
        if (!a && L >= 4 && Q[ai] >= 0) { double s = 0; int c = 0; for (size_t k = ai; k > 0 && k + 20 > ai; --k) { const long long v = Q[k - 1]; if (v < 0) continue; s += (double)v; ++c; }
            const long long z = std::max(1LL, (L + 2) / 4); const bool away = dr > 0 ? b.c >= b.l + z : b.c <= b.h - z;
            if (c >= 10 && s > 0 && (double)Q[ai] / (s / c) >= 2.0 && away) a = true; }
        // E: the attackers' last minute (lsTurn) or the next candle
        bool e = false;
        if (exq) { const auto q = exq->find(b.t); if (q != exq->end()) e = (dr > 0 ? q->second.first : q->second.second) == 1; }
        if (!e && pi + 1 < n) { const wyr::Bar& x = B[(size_t)pi + 1]; const long long a0 = dr > 0 ? b.sell : b.buy, a1 = dr > 0 ? x.sell : x.buy;
            const bool held = dr > 0 ? x.l >= b.l - 1 : x.h <= b.h + 1;
            if (a0 > 0 && 2 * a1 <= a0 && (x.c - b.c) * dr > 0 && held) e = true; }
        out[kv.first] = l3Text(a, e); }
    return out;
}
} // namespace lvm
