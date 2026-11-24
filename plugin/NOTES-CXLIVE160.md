# The live Cx? - LevelMarks 1.16.0 / lsRevRead + lsRevReadMarks 0.25.0 / WyMarks 2.6.0 (2026-11-23)

Rassul 23:18 CT (HG 3-minute screenshot) "the CX came late .. why ?"; 23:20 "go" on the live Cx?.

## Why it came late
The Cx is decided when the 3-bar ZigZag confirms the pivot. HG 11:03 PM: the climax candle (46 lots, the biggest in 20+,
the 1hL sweep). The 11:06 PM candle's high (5.5085) went over the 3 candles before -> the low confirmed at the CLOSE of 11:06 PM ->
the Cx was drawn from then on: one candle (3 minutes) after the climax closed. (Replay of his candles: t_cxlive.)

## What changed
- lvm::liveCx(): on the last CLOSED candle, the running leg's extreme candle (or the one before it) with the biggest volume of the 20
  candles before it (the Cx test, preV included) and no mark yet -> "Cx?". Not decided: it moves / goes when the leg makes a new extreme
  that is not the climax; the real Cx replaces it when the pivot confirms. Never recorded.
- lsRevRead writes RevRead\wylive-<MKT>-<spb>.txt (WYL|1|... + "stamp|Cx?|dir" or nothing) once per closed candle. The wy / wymarks
  records and every other reader are unchanged (parseAll ignores "Cx?").
- lsRevReadMarks reads it every 3 s (WyMarks parseLive) and draws a "Cx?" badge like the Cx (no level, no strength), not where a Cx is.
- HG 11-23 replay: Cx? at the 11:03 PM close, Cx at the 11:06 PM close; the Cx? moved to another candle once in the session.

## Tests
t_cxlive 9, t_wylive 10, t_levelmarks 95, t_major 60, t_ar 94, t_cx115 15, t_wymarks 42, t_wymarks_ar 14; both plugins against the
real irtsdk.h C++14 / C++17: 0 errors (the 3 RevReadLogic.h indentation warnings are old).
