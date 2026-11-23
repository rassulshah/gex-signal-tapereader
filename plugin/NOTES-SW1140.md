# LevelMarks 1.14.0 / lsRevRead 0.24.0 / NowLevels 1.4.0 / lsHodLod 2.44.0 - Sw on major levels, the 3h S / R, the Level line, the AR's ATR way (2026-11-23)

## Requests (Rassul, 2026-11-23 CT)
- 14:00 "for the AR badge ,   if the Green bar is bigger than the ATR and its high is above the cmx bar with the hevy volume , it is also valid AR.. do you understand"
- 14:03 "for SW signal it needs to cross a major level like a session or pd pf  weekly or 3h S1 S2 , R1 R2 level etc .. do you understand ?"
- 14:04 "no the 3h is  the S1 S2 R1 R2  levels"; 14:05 (GC 180-minute chart: red 3691.0, green 3599.9 and 3502.9) "the green levels are Support levels and red are the resitance levels"
- 14:06 "i also think you have implemented these S and R levles on the limit profile right ?" (yes: HtfSr.h, 2026-11-15)
- 14:09 (GC 3-minute chart, the line read "WATCH: PHL .. 2hL" with PFH 3638.1 / ONH 3636.4 right under price) "why isnt the level line catching the most obvious levels"
- 14:12 "no the bk and tst can also include hourly , what do they include now ?"; 14:14 "ok so you are just adding to bk right ?" (yes: the 3h levels)
- 14:15 "and for SW you are limiting to major levels right" (yes); "and for levels, you are also using the same levels as SW right" (yes)
- 14:16 "and what is the other range that is going in  for the AR tag" (answered: BRMax 10+ OR bigger than ATR(13) + beyond the Cx candle); 14:18 "build"
- Mockup sweep-levels-mockup.html (sent 14:27, before the install).

## What changed
1. **Sw only on a major level** (LevelMarks 1.14.0, `swMajor`): a Sw is kept only when the level it is LABELLED with is session (ON / London / Asia), prior day, prior full
   session, weekly (PWH / PWL / CWH / CWL) or a 3-hour S / R. The hour highs / lows (1h, 2h, 3h back), the pre-open hour (PhrH / PhrL) and VAH / VAL no longer name a Sw.
   What makes a sweep is unchanged: a level nobody traded through since it formed (any level of the list - 2026-11-20 16:44 "use strict rule"), the close back within
   3 candles, the candle's colour, BVMax / BRMax 20+, never the RTH opening candle or a release candle. The label must now be a major level the candle crossed.
   A sweep candle that crossed no major level is taken out BEFORE the Bk and Cx steps, so the candle can be the Bk / Cx it is (it used to block them).
2. **The 3-hour S / R (R1 R2 S1 S2) are levels of every mark** (Sw, Bk, Tst through its Bk, Cx's label): the pane walks HtfSr.h (the walk lsHtfSR draws and the Limit
   Profile uses) over the chart's closed candles and hands each session the levels standing BEFORE each candle (lvm::H3). Priority 45: after weekly, before the hour
   levels. A 3h level counts from the first candle that had it at that price; broken levels are not levels. Bk / Tst / Cx keep every level they had.
3. **The AR's second way in**: rules 1 - 4 unchanged (a drawn Cx; one of the 3 candles after the pivot candle; colour; overlap with the pivot candle). Rule 5 is now
   BRMax 10+ OR (range > ATR(13) of the candle before AND the high above the Cx CANDLE's high; bearish: the low below the Cx candle's low). Strength line: the V / R
   count when there is one, else the range against that ATR ("1.2x"). ATR(13), 1.0x and "the 3-candle and overlap rules stay" were stated to him at 14:00 / 14:16 as
   assumptions; he built without changing them.
4. **The Level (Now) line names major levels only** (NowLevels 1.4.0, `major`): WATCH / Next = the nearest major level each side; no hour level (PHH / PHL / 2h / 3h),
   no VAH / VAL. A Bk / Sw mark on a level that is not major gives no BREAK / SWEEP badge, no BkTst state and no Last Bk part (its badge stays on the candle - told
   14:15). lsHodLod 2.44.0 walks the same 3h S / R over its own bars (the walk over the closed bars is kept until a new bar closes) and hands R1 R2 S1 S2 in as keys.
- Cause of the 14:09 screenshot, confirmed: the old line on the same candles reads exactly "[WATCH: PHL (down) . 2hL (up) . Bk / Sw] | Last Bk (up) LHI" - the prior
  hour's low was the nearest level under price, PFH / ONH half a point and two points further.
- NOT changed: Bk, FT, Tst, Rev, Cx, I rules; the strength counts; the files' formats (the AR strength text may now be "1.2x"); frozen sessions (WyFreeze: a finished
  session's file is read back as written - Friday keeps its old Sw labels); VolRead / RangeRead / SweepVoice / lsTurn code (they read the marks file).

## What it does today (2026-11-23, the 5 charts' own candles; the harness reproduces the live files' Bk / Sw / Cx 127 of 127)
| Market | Sw before | Sw after | Other marks |
|---|---|---|---|
| GC | 3 (PhrL 7:57, 1hH 8:06, 1hL 1:03 PM) | 0 | 7:57 AM becomes Bk ONH V99+ (it closed up through LonHI, AsiaHI, ONH, PFH) |
| ES | 3 (2hL, 1hL, 1hL) | 0 | 1:03 PM becomes Cx; the 8:42 PM Cx moves to its pivot candle 8:45 PM; 2 new AR |
| NQ | 2 (1hL, 1hL) | 0 | 1:03 PM becomes Cx; 3 new AR |
| HG | 5 (hour levels) | 0 | 7:03 PM becomes Bk 1hL; 11:33 PM becomes Cx; 5 new AR |
| CL | 2 (1hL, PDL 7:57 AM) | 2 (S1 2:54 AM new, PDL 7:57 AM) | 7:03 PM becomes Bk 1hH; 1 new AR |
- 3h levels at 2:15 PM, GC: R1 = R2 3691.0, S1 3599.9, S2 3502.9 = his 180-minute chart. Level line GC: "WATCH: PFH . R1 . Bk / Sw | Last Bk ONH".

## Files
LevelMarks.h, RevRead.cpp, RevReadMarks.cpp (0.24.0), NowLevels.h, HodLod.cpp, HodLodLogic.h, HodLodSettings.h (2.44.0 = the three version places). HtfSr.h is
included by both hosts, unchanged (1.0.2). gex-build.bat is NOT edited (shared, append-only; the sources themselves changed, so RevRead, RevReadMarks, HodLod and the
plugins that list LevelMarks.h - Breakout, Breakout1, VolRead, RangeRead - rebuild). tests_levelmarks/t_major.cpp (new), t_ar.cpp, gc1123.psv, wy_GC_1123.txt;
tests_hodlod/t_hl2440.cpp (new), gc1123.psv, wy_GC_1123.txt, wy_GC_1123_new.txt.

## Tests (research machine)
- Real irtsdk.h syntax check, C++14 and C++17, -Wall -Wextra: RevRead.cpp, RevReadMarks.cpp, HodLod.cpp, VolRead.cpp, RangeRead.cpp 0 errors; warnings outside the SDK
  6 / 3 / 1 / 0 / 0 - the same as before the change. Breakout.cpp could not be checked here (BigBidAskLogic.h is not on this machine); it calls only lvm::levelNear,
  whose signature is unchanged.
- tests_levelmarks/t_major.cpp (ASan / UBSan, -Werror, C++17 and C++14): 60 checks, 0 failed - the major list; hand-made sweeps of ONL / PDL / PFL / PWL / S1 / S2 kept,
  of VAL or hour levels alone dropped, a 3h level already traded through not swept again, a wrong-length 3h list ignored; Bk S1, Bk keeps VAL; the GC 7:57 AM shape
  in small; GC today: the three Sw gone, 7:57 AM = Bk ONH, the other 17 Bk / Cx of the live file identical; 8 sessions with no level file: every Sw on a 3h level; no
  repaint over 366 candle closes.
- tests_levelmarks/t_ar.cpp: 94 checks, 0 failed (59 before) - NQ 11-20 7:45 AM = AR "1.2x" after the 7:36 Cx, 7:39 / 7:42 passed over, his 6:51 AM example still R51;
  hand-made: bigger than the ATR + above the Cx candle = AR; high only AT the Cx candle's high, range not bigger than the ATR, no overlap, 4th candle, wrong colour = none;
  mirror; every real AR has BRMax 10+ or the ATR way, and an "x" line only without a V / R count and equal to range / ATR.
- t_levelmarks 95, t_ar_readers 7, t_wymarks 42, t_wymarks_ar 14, t_vr1110 28, t_vr1100 37, t_vr190 58: 0 failed.
- tests_hodlod/t_hl2440.cpp: 43 checks, 0 failed - versions; the major list; GC 2:15 PM line with 0.24.0's marks and with the live marks; over 366 candle closes no
  hour level / VAH / VAL / PhrH on the line and never an empty line; a Bk ONH mark = BREAK, a Sw PhrL mark = no SWEEP, a Sw ONL mark = SWEEP, a Bk 1hH mark = no
  BREAK and no Last Bk, a Bk R1 mark = BREAK.
- The older tests_hodlod suites, old tree against new: t_hl2390 73, t_hl2430 2,265, t_hl2400, t_hl2410, t_hl2420, t_hl240, t_hl2370, t_hl244, t_hl270 identical results.
  Changed on purpose: t_hl2360 (18 checks expect PHH / PHL / VAL as line levels), t_hl2380 and t_hl2391 (1 each: they pin NowLevels 1.3.x).
- Old code on the bar store's candles reproduces the live marks files: GC 20 of 20, ES 19 of 19, NQ 24 of 24, HG 31 of 31, CL 33 of 33.

## Not verified (needs his IRT)
- The build of the 7 plugins on the PC and the charts after his restart: today's Sw redrawn, R1 / S1 labels, the Level line, AR badges with "1.2x".
- The price arrays' rounding to ticks in the host against the bar store's (the harness uses the store's candles).
- lsHodLod's 3h walk reads up to 21 days of the chart's bars; a chart with less history than the 180-minute chart can show other pivots than his 180-minute chart
  (the same limit lsHtfSR and the Limit Profile have on that chart).
- The nightly's own sweep scoring (lra.turn_pane, sweep studies) still counts sweeps of hour levels; it reads its own definition, not this header. Needs a follow-up.
