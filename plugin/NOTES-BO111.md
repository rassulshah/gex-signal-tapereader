# lsBreakout 1.1.1 - Pb two rows out (2026-11-03)

## Request
Rassul's screenshot (ES 3-min, today): the green "Pb" sat on the zigzag wave-time labels ("9m", "16m") another indicator draws right at
the pivot bar's wick (just below a swing low / above a swing high, ~14 px tall, from ~3 px out). lsCandleAbsorb's A / Ae already uses the
next row (ROW_OUT 16 px beyond the wick label row). Fix: move Pb out of both.

## What changed
- `BreakoutLogic.h` 1.1.1 (`bko::VERSION`), `Breakout.cpp` (`BKO_VERSION`, setVersion "1.1.1", description v1.1.1).
- Pb now sits TWO rows beyond the wick: below a pivot LOW the letter's top = Y(low) + 3 + 32; above a pivot HIGH its bottom =
  Y(high) - 3 - 32 (`ROW_OUT 16`, `PB_ROWS 2`, `PB_OFF = GAP + 2 x ROW_OUT = 35`).
- Collision rule unchanged: another BO / BD / Rly letter in that row -> one more row out (row height + 1 px, up to 4 rows); no room =
  dropped and counted in the status file. SignalBoard 'L' carries the Pb's outer edge as before (so lsLiquidityProfile's Ex stacks past it).
- BO / BD / Rly positions, every decision, the Pb rule, files: unchanged. No new includes (gex-build line unchanged).

## BO / BD and the wave-time label
Checked ES Oct 26 - Nov 3 (3-min from fp_bars, 111 fired BO / BD): 0 sit on a pivot bar on their own letter side (1-bar and 3-bar
ZigZag) - a BO candle always has a later follow-through with a higher high in the same wave, so it can never be the swing high (BD
mirrored). 6 sit on a pivot of the OPPOSITE side (label under the candle, BO above it) - no overlap possible. BO / BD left where they are.

## Tests (tests_bo100/run_tests.sh, ASan / UBSan, -Werror -Wall -Wextra)
- test_bo100_parity 9 / 9 (BO / BD 20 / 20, Rly 7 / 7 vs lsBigBidAsk's ES Oct 30 records; no repaint; restart).
- test_bo110_pb 21 / 21 (+3): green Pb top at Y(low) + 35; red Pb bottom at Y(high) - 35; Pb clears a simulated 14-px wave-time label at
  the wick row (+3 .. +17) and the CandleAbsorb A row (+19 .. +33) on both sides; a BD on the same bar no longer pushes it; a (tall) Rly
  reaching into the Pb row pushes it one more row out with no overlap. Pb parity with the mockup 9 / 9 unchanged.
- test_bo100_host 22 / 22: every Pb drawn 35 px from the wick (17 / 17 exact), never in the wave-label or A rows, no overlaps, status
  "VERSION,1.1.1".
- Mutation: PB_ROWS = 0 (the 1.1.0 position) fails 6 logic + 2 host checks.
- MinGW vs the real irtsdk.h: 0 errors, 0 warnings outside the SDK. Render: tests_bo100/render-bo111.png.

## Risks / not verified - needs his IRT
- lsCandleAbsorb draws its A row from Y(low - 0.5 tick) (half a tick lower than the wick): at a tight zoom where half a tick > 2 px the
  A box and the Pb could touch (the glyphs are smaller than the 14-px box, so likely still clear). Check on the wide Samsung monitor.
- The wave-time labels belong to another (RTL) indicator; their real height on his chart was taken from the screenshot (~14 px).
