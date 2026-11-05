# lsBreakout 1.6.0 (2026-11-04 19:06-19:22 CT)

Rassul, 19:06: "lets remove the BI signal and require the PB which tests the BO to pullback to the pivot with less than 1/2 atr remaining". He sent two sketches: a pullback that tests the broken level, and a pullback that stops close to it. At 19:21 he said "make it 1/3", then "go".

## Changes
1. **BI removed** (`SHOW_BI = false`):
   - No BI pass, no BI letters and no BBI| records.
   - The breakouts file reads "| bi 0" and has no "# BBI" header.
   - No BO / BD is cancelled by a BI.
   - The status line reads "BI,removed (1.6.0 ...)".
   - The BI code stays in the source, unused.
2. **Pb rule.** A Pb is a pullback pivot after a BO whose low is at or through the broken pivot, or less than ATR / 3 above it (BD mirror). ATR is the 14-bar Wilder ATR at the BO / BD candle (the rly::atrWilder rule).
   - This replaces 1.1.0's rule that the pivot bar overlaps the BO / BD candle.
   - Unchanged: the 3-bar pivots, one Pb per pivot, the search ends at the next BO / BD or when price trades beyond the wave's origin, decided once, no repaint.

## Effect on two weeks of data
| Market | Pb before (1.5.0) | Pb after (1.6.0) |
|---|---|---|
| ES Oct 20 - Nov 3 | 160 | 181 (145 same, 15 dropped, 36 new) |
| NQ week | 125 | 136 |
| GC week | 122 | 140 |

## Python port
`analytics/lra/bo_pb.py` 1.3.0: `pb_marks(rule="atr")` + `atr_wilder`. The engine equals this port on ES, NQ and GC (test_bo160_pb [1]).

## Tests (tests_bo100/run_tests.sh, ASan/UBSan, all passed)
| Test | Result |
|---|---|
| parity | 9 |
| week (BO / BD + Rly unchanged, Pb lines skipped) | 45 |
| test_bo160_pb (new) | 12 |
| host | 23 |
| rec | 21 |
| render | ok |
| MinGW against the real irtsdk.h | 0 errors, 0 warnings |

test_bo110_pb (the overlap rule) and test_bo150_bi are retired: they stay in the folder but are not run.
