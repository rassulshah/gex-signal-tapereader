# Tst badge line 3 (A+E / E / A!) + P line always - 2026-11-24

Rassul 12:21 CT "use the third line which currently has the percentage .. show mockup" (tst-line3-mockup.html), 12:51 CT "2. do the test", "3. yes".

## What changed
- lsRevRead / lsRevReadMarks 0.28.0: the Tst badge's 3rd line = A+E / E / A! / blank instead of the retrace %.
  - A = absorption on the Tst pivot candle: effort without result (vol >= 1.5x the 20 candles before, range <= 1.3x, close in the far half)
    OR the Absorption bar (lots at the heaviest attacking price in the end quarter >= 2.0x the 20 before, close outside the quarter).
  - E = exhaustion: lsTurn's Ex? on the pivot candle (last minute <= 0.5x the first 2 minutes' pace) OR the next candle (attackers <= 0.5x,
    closed further the Tst's way, held within 1 tick).
  - Decided once the candle after the pivot closed; kept for good in RevRead\tstl3-<MKT>-<spb>.txt (no repaint across restarts).
  - Sessions already frozen (wymarks-<MKT>-<spb>-<date>.txt) keep their old % line.
- LevelMarks 1.19.0: tstL3(). WyRead 1.11.0 / RevReadLogic 0.17.0: quarter lots per candle.
- WyMarks 2.8.0: Abs badge P line always shown once known (P0 .. P12+); parseTeq / tl3Text / parseTl3.
- lsTurn 1.6.0: RevRead\turnexq-<MKT>-<spb>.txt = the sellers' / buyers' Ex? of every candle it read whole. Nothing drawn changed.

## Evidence (research TST-REVERSE 2026-11-24, 697 decided Tst, PB Rev win rule)
A+E 75% / +0.33R (n 60, +0.17 / +0.59 halves); E +0.02R; A! 55% / -0.19R (both halves); blank +0.07R. LEARNING (4-16 per market).

## Tests
- Parity: plugin tstL3 vs research labels (A, next-candle E) 943 / 943 Tst (100%).
- tests_levelmarks: t_tst119 25 checks + all suites pass (versions moved to 1.19.0).
- tests_turn all160.sh: real-SDK syntax 0 errors / 0 warnings outside the SDK, logic 3,287,189, golden 2,017, host 44,117 (turnexq file checked
  against the Abs badge's Ex?), soak no growth, read 136/262, abs 31 - all 0 failed.
- RevRead / RevReadMarks: real-SDK syntax 0 errors; only the pre-existing indentation warnings.

## Not verified - needs his IRT
Restart IRT; then check a Tst badge shows A+E / E / A! (or two lines when blank) and the turnexq / tstl3 files appear.
