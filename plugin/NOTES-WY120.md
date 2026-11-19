# WyRead 1.2.0 / lsRevRead 0.18.5 - the I as a failed HTF breakout (2026-11-19)

Rassul 08:41 CT on the ES 3:36 AM I: "lets include this. so there is always some reclaim after a sweep of some htf level, in this case
it was an hourly level. build"; 08:42 "the key is that there is a htf breakout failure as indicated by the reclaim of a level".

What changed
- The high / low of each of the 3 completed clock hours before the pivot's hour is now a level the I can reclaim
  ("reclaimed the 2 AM hour low 7412.25 after the 3:33 low swept it"). Computed from the candles, so every session gets it
  (key / volume level files still feed only the current session). An hour the candles only partly cover is skipped.
- Same rules as 1.1.0 otherwise: swept 1+ tick, first close back, 2x delta, close through the prior candle, within 1 ATR.

Tests: tests_wyread/t_hourly.cpp 8/8 (ASan/UBSan -Werror), t_reclaim 6/6, t_wyread_102 19/19, t_nearpivot pass; MinGW syntax OK.

Measured (in-sample, 3-min, Aug 25 - Nov 19, 41-62 sessions per market, stop 1 tick past the pivot, 1.5R, net of costs):
- No I's added or removed on any market; the change re-labels which level the I failed at.
- I's whose furthest reclaimed level is a prior-hour high / low: n 504, 39% win, -0.15R (halves -0.17 / -0.13)
- I's that reclaimed only a local pivot: n 687, 36%, -0.26R (halves -0.20 / -0.33)
- Hourly better in NQ (-0.08 vs -0.27), HG, EU; no difference in ES (-0.30 vs -0.28) and GC.
- Still negative as a stand-alone trade with this stop / target.

Not verified: needs his IRT restart (File -> Exit, wait for "installed", reopen).

## 1.2.1 (08:59 CT "the 4 hour high and low is also a key level")
- The high / low of the 2 completed 4-hour candles before the pivot's 4-hour candle, anchored on the 5 PM CT session open
  (5 PM, 9 PM, 1 AM, 5 AM, 9 AM, 1 PM), are levels the I can reclaim ("the 1 AM 4h low"). On a tie with an hour level the 4h name wins.
- Tests: t_hourly 13/13; other WyRead suites unchanged and passing; MinGW OK. Ships inside lsRevRead 0.18.5 (not yet installed).
- Measured (same data / trade as above), by the furthest level reclaimed:
  4h 148, 34%, -0.28R (halves -0.33 / -0.23) | hour 435, 41%, -0.11R (-0.11 / -0.10) | local pivot only 672, 35%, -0.28R.
  The 4h group is no better than local pivots in this sample; the hourly group stays the best.

## 1.3.0 (09:02 CT "yes. you want some form of higher timeframe breakout failure")
- The I needs a failed higher-timeframe breakout: the level reclaimed must be a prior hour / 4-hour high or low, or a key / volume level
  (current session). A local pivot alone no longer makes an I. The strong-rejection I after Sp / Ut needs its spring / upthrust candle to
  have swept such a level and the rejection candle to close back through it; its read names the level.
- ES 11-18 fixture now uses real clock stamps; its 3:06 AM rejection I is gone (the 3:03 spring broke only a local floor, the 2 AM hour
  low 7387.00 held). Tests: t_hourly 13/13, t_reclaim 6/6, t_wyread_102 19/19, t_nearpivot pass, MinGW OK.
- Measured (no key-level files in the research run, so hour / 4h only): I's 1,239 -> 1,070 (-14%; most I's credited to a local pivot
  had also reclaimed an hour level). Hour 757, 40%, -0.15R | 4h 219, 30%, -0.38R. Best: CL hour +0.09R, NQ hour -0.04R. ES -0.31R.

## 1.3.1 (09:05 CT "then lets drop the 4 hour")
- 4-hour highs / lows no longer count; the I reclaims a prior hour high / low (3 hours back) or a key / volume level.
- Tests: t_hourly 13/13 (the 4h case now reads "the 4 AM hour low"), t_reclaim 6/6, t_wyread_102 19/19, MinGW OK.
- Measured: I's 1,070 -> 987 (-83 that only reclaimed a 4h level). All I's ~-0.21R (was ~-0.22R): the 4h I's that remain are
  credited to the hour level they also reclaimed, so the hour group is now 901, 38%, -0.20R.
