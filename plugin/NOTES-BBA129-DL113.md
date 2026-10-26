# lsBigBidAsk 1.29.0 + lsDefenseLines 1.13.0 (2026-10-26 evening, Rassul 17:55-18:20 CT)

## Removed from the chart (drawing only; decisions + records kept for the nightly)
- lsBigBidAsk: the OB print (the lots box OrderBlockLogic drew for an OB) and the "OB" letter. 6 OB settings rows removed (parameter
  version 14). obPass + OBX / OBT record lines unchanged (fixed 3.0 ATR, 2 candles after the pivot). Ordinary Big Bid/Ask prints and
  BO letters / prints unchanged.
- lsDefenseLines: the gray EXHAUSTION (E) line and its dot ("show_e" / "col_e" rows removed, parameter version 5); the PRESSED-IN /
  STEPPED-BACK move boxes (hollow box at the old price, solid box + lots at the new price; no settings row existed). X and M lines still
  read and counted. R lines, dots, trails unchanged.

## BO / BD rules (rule 3, confirmed 18:20 CT)
Decided at the BO candle's own close: green BO / red BD trading through one of the last 3 intact swings (full-session 1-bar ZigZag;
a swing stays intact until a candle CLOSES beyond it - a failed poke does not use it up), 10-candle extreme, range >= prior and
volume or range up; no close beyond the pivot and no confirming candle; every session (no RTH-only, no skip minutes); counter-trend
(vwapTrend) needs a 20-bar extreme in the wave (was 30); one per wave. 1.26.0 - 1.28.0 BBO lines are decided again.
SwingRef.h (lsStopRun / lsSweepRefill) unchanged - bba::swingBreaksPx only.

ES 2026-10-26 (tests_bo129): 09:12 BO fires (closed below 7704); 09:45 poke = range_below_prior, 09:48 BO through 7719.5 fires;
10:48 BD = counter_trend_no_20 (Strong Uptrend, no 20-bar low) - the 10:51 candle through the same 7726.75 swing fires instead.
Why 1.28.0 logged 09:12 no_confirm with later 0: the BO bar's own ZigZag walk still pointed down (zdir != BO side), so it was ruled
"not in its wave" at once.

## Tests
tests_bba115 (115 / 119 / 126: 58 / 0), tests_bigba 114 + swing114, tests_bo129 (29 / 0), tests_aggbook --full incl. real-SDK
syntax for BidAskStats / BookFlow / DefenseLines, MinGW + real irtsdk.h for BigBidAsk (0 errors, 0 warnings outside the SDK),
tests_settings contract (43,075 checks, 0 failures), tests_perf (58 / 0). tests_bo128 pins the retired rule 2 (kept, not run).
