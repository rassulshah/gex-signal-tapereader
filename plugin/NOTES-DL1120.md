# lsDefenseLines 1.12.0 + lsLiquidityProfile 0.55.1 (2026-10-25, BUILD-SPEC "Final decisions 22:29": Defense Lines vs order lines at the same price)

## What changed
- Defense (refill / E) lines are THINNER: 1 px (was 2). New setting "Line width (px)" 1-3 under Refill lines. Still no label.
- The yellow dot on the candle a line refers to is NOT drawn when lsLiquidityProfile drew an order FILL DOT on that candle.
  LP 0.55.1 writes lsFlexLevels\LRA-OrderFills-<MKT>-<spb>.txt (VERSION line + one "F,<bar datetime>" per fill-dot candle, from the data,
  rewritten only when the set changes); DefenseLines reads it every 2 s. New setting "No dot where an order fill dot is" (default on).
  No file (no LP order lines on that market) = every dot drawn as before.
- The forming candle's line is solid 1 px (was dotted): no dotted / dashed lines anywhere (his rule).
- Order line on top: put lsLiquidityProfile AFTER lsDefenseLines in the chart's indicator list (IRT draws later indicators on top).
- Status: FILL_DOTS,<file>,<n> order-fill candles,yellow dots skipped <n>. DefenseLines settings parameter version 4 (old files load; new rows default).
- lsBookFlow and lsBidAskStats include DefenseLinesSettings.h (new rows appended at the end: no key moved) - they rebuild with it.

## Tests (Linux mock, ASan/UBSan, -Werror)
- tests_aggbook/test_defenselines_host: 56/56 (6 new: 1 px, dot skipped with the fill file, status, setting off = dot back, width 3, file gone = dots back, no P_DOT in the source). test_trails_host 32/32. DefenseTurn 81, G/Tr/W 77, EffortReward 28, build dependencies 348.
- tests_oln/test_oln_host (LP): 25/25 (new: LRA-OrderFills-ES-180.txt = one F line, the fill candle's key). LP host regressions unchanged (lmz 34, lqp044 21; lqp453 1 / lqp461 2 failures are the same as on 0.54.1).
- Settings contract: 43,482 checks, 0 failures. MinGW with the real irtsdk.h: 0 errors (DefenseLines, LiquidityProfile).
- Pre-existing, not from this build: tests_aggbook/test_aggbook_113 version pins (BookFlow 1.31.0 / BidAskStats 1.26.0).

## Verify on his chart
- Titles: lsDefenseLines 1.12.0, lsLiquidityProfile 0.55.1. A refill line at the same price as an order line: thin, no yellow dot on a candle
  that has the order fill dot, the order line drawn over it (indicator order).
