# lsRevRead / lsRevReadMarks 0.21.1 + LevelMarks 1.6.0 - Sw threshold 20 (2026-11-20)

Request: Rassul 07:50 CT 2026-11-20 "update the sweep threshold to 20". Built on the pending 0.21.0 (re-staged from the PC, identical to
outputs/rr0210).

## What changed
- LevelMarks.h 1.6.0: `SW_MIN` 10 -> 20 - a Sw (sweep) mark needs its attempt's BVMax or BRMax (max over the attempt candles) >= 20, else it
  is removed. `ST_MIN` (the strength line, shown from 10 up) is unchanged; a Bk is never removed by this rule.
- RevRead.cpp / RevReadMarks.cpp: version 0.21.0 -> 0.21.1 (RVS_VERSION, setVersion, description). No other code change.
- Nightly: analytics/lra/revread_letters.py 1.4.3 `SW_MIN = 20` (level_setups.sw_filter uses it); level_setups.py 1.0.1 (docstring + version).
  chart_signals.py reads Sw from the wy file - follows automatically. lsHodLod / lsVolRead / lsRangeRead read Sw from the marks file.

## Effect on 11-19 (his chart bars, tests_levelmarks fixtures)
- ES: 4 Sw drawn (9:15 PM 2hL max 32, 12:03 AM 2hH 58, 8:39 AM PHL 99, 9:39 AM PDL 99) - all 4 survive at 20 (the 2:18 AM PHH, max 5,
  was already dropped at 10).
- NQ: 6 Sw at 10 -> 5 at 20 (the 7:12 AM PHH, max 16, drops).

## In-sample (data/setups/level_setups_events.csv, 2026-09 .. 11-19, R_his; >= 20 approximated by the Sw candle's own strength `st`)
- ES: all Sw (>= 10) n 47, mean +0.13R -> st >= 20 n 26, -0.11R (the 21 dropped averaged +0.43R).
- NQ: n 50, +0.62R -> n 19, +0.56R (dropped 31: +0.66R).
- In this sample the stronger sweeps did NOT do better; the nightly level_setups run tonight scores the 20 rule properly (attempt max).

## Tests
- tests_levelmarks/t_levelmarks.cpp 67/67 (ASan/UBSan, -Werror) incl. 7 new 1.6.0 checks: SW_MIN 20 / ST_MIN 10; BVMax 15, 19 and BRMax 18
  -> no Sw; BVMax 20 -> Sw "V20"; BRMax 25 -> Sw "R25"; Bk with 15 keeps "V15"; weak Bk kept. 11-19 ES / NQ parity unchanged.
- tests_rr0210 45/45, tests_wymarks 42/42. tests_wyfreeze crashes under ASan here with the 0.21.0 headers too (pre-existing, WyFreeze.h
  does not include LevelMarks.h) - not caused by this change.
- pytest test_level_setups.py (+ test_sw_threshold_20) + lra/test_revread_letters_none.py: 8 passed.
- MinGW -fsyntax-only vs the real irtsdk.h (RevRead + RevReadMarks): 0 errors, 0 new warnings (7 / 3 pre-existing).

## Not verified - needs his IRT
The Sw marks on the live chart after he exits IRT and the build installs.
