# LevelMarks 1.15.0 / lsRevRead 0.24.1 - the Cx in the first hour of a session - 2026-11-23

## His request (CT)
- 19:54 (GC 3-minute screenshot, the 5:51 PM pivot low circled) "why isnt this a cmx ?"
- 20:16 "go" on the fix (the Cx look-back reaches into the prior session's candles, as BVMax does)

## Cause
The Cx rule ("a pivot candle, or the candle before it, with the biggest volume in 20+") counted only the session's own candles and
needed 20 of them BEFORE the candle (kk < 20 = skipped). The session starts at 5:00 PM: no Cx could appear before the 21st candle
(6:03 PM). GC 5:51 PM: 324 lots, the biggest in 35 candles, the pivot low, the 17th candle - skipped before its volume was looked at.

## What changed
- LevelMarks.h 1.15.0: run() takes preV = the volumes of the candles before B[0], oldest first. The Cx's 20-candle look-back uses the
  session's candles and then preV. Null / empty preV = 1.14.0 exactly. No other mark touches preV.
- RevRead.cpp 0.24.1: hands run() the volume of the 20 chart candles before the session's first candle. RevReadMarks.cpp 0.24.1 (version only).

## Tests (all pass)
- tests_levelmarks/t_cx115.cpp (new, gc1123_cx.csv = his GC candles 3:03 PM - 8:15 PM): 15 checks, ASan / UBSan - Cx at 5:51 PM (green,
  a pivot low), the 6:03 PM Cx stays, no other mark changes, a bigger candle within the 20 before = no Cx, one more than 20 back does
  not count, under 20 candles before = no Cx, no repaint over every prefix.
- t_levelmarks 95, t_major 60, t_ar 94 (version pins moved to 1.15.0), t_ar_readers 7.
- Real irtsdk.h syntax, RevRead.cpp and RevReadMarks.cpp: 0 errors, the same warning count as 0.24.0.

## Not verified / notes
- Not seen on his chart (needs his restart). Today's session is recomputed: the 5:51 PM Cx (and an AR if one answers it) appears
  after the restart; finished sessions keep their saved marks (WyFreeze).
- More Cx marks in the first hour after 5:00 PM on every market, by design.
