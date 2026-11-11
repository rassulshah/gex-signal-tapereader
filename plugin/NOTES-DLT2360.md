# lsDeltaProfile 2.36.0 (2026-11-11) - speed only

## What he asked (CT)
- 00:17 "double check why irt is slow ..."; 07:44 "its important that all the indicators and how they are processed are improved /enhanced as
  part of the nightly improvement process"; 07:46 "all" (incl. the Delta Profile + BookFlow audit); 10:10 "3. go".

## Measured on his ES chart (PerfGuard, 11-11 RTH, 6,580 candles)
- Delta Profile: 222 redraws a minute x 13.3 ms (p95 22 ms, max 150 ms) = about 3 s of every minute. The heaviest indicator.

## Cause (my code)
- render() copied the high, low and time of EVERY candle on the chart from IRT on every redraw to build the "location first" context
  (zn::Ctx, 2.9.0): about 20,000 array reads + 6,580 date conversions a redraw.
- buildNative() (once a second) passed IRT's arrays to dp::swingPivot, which walks all candles several times: about 50,000 reads a second.

## Fix
- ChartState keeps the highs / lows / session keys (refreshHL): unchanged history = only the last two known candles and any new ones are
  read; a full re-read when the first candle or the last-known candle moved, and every 60 s. render() and swingPivot use that copy.
- No rule, drawing, record or settings change.

## Tests
- tests_dp217/t_dlt_perf.cpp (new): PERF mode for tests_perf/check_draw_io.py - PASS; a plain redraw on 6,580 candles 0.276 -> 0.025 ms
  on the mock SDK (11x; no longer grows with the candles on screen), a rebuild 0.70 -> 0.27 ms.
- PARITY mode (same file): 600 steps on a growing chart (forming candle changes, candles close, forced full re-reads), ASan / UBSan:
  every draw call and the signal record file are identical to 2.35.0 (hash 826d5b1467ef1937, record 80,227 bytes).
- test_dp2321: 15 pass, 1 fail - the same as on 2.35.0 (its label-gap check was already failing). MinGW real SDK: 0 errors, 6 warnings (= 2.35.0).
  Settings contract passes.

## Not verified / not done
- The real saving in IRT is only known from tomorrow's PerfGuard numbers (the mock SDK's reads are free; IRT's are not).
- Still read from IRT every second: about 400 volume-at-price rows + 40 bar statistics per rebuild (closed candles could be kept too).
- BookFlow (recalc 59 ms x 12 a minute, max 116 ms) is not audited yet.
