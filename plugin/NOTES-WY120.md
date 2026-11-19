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
