# lsWavePOC 1.9.1 (2026-11-06 12:58-13:01 CT)

Rassul looked at the NQ 3-min Nov 6 down wave (11:24 -> 11:54) and asked "why are the 2 H levels not displayed ?". He then gave the rule:
- "it should be the top 2 nodes above the breakdown low and vice versa"
- "basically that are in their own distributions above"
- "isnt there a second hvn in the circle below that is above the breakdown low"

## Cause
1.9.0 looked for HVNs across the whole wave and required each to be at least 2x the median row. It then kept only those on the right side of the break level.

On this wave:
- The BD low is 27,154.50.
- 27,219 (270 lots) passed.
- 27,162 (197 lots, its own distribution) failed the 2x floor of 198 by 1 lot, so only 1 H was drawn.

## Fix (wpc::hvnRows)
- The node finder (pickNodes) now runs ONLY on the rows at or below the break level (down waves: at or above).
- Its rules are unchanged: smoothed peaks, separation, a valley <= 0.7 x the smaller peak, and the biggest raw row of each distribution.
- The 2x-median floor is gone. A node only has to beat the median row, so flat noise is never a node.
- The 2 biggest nodes are taken. The POC (and its distribution) is never one of them.

The NQ wave now shows H at 27,219 and H at 27,162; P stays at 27,049.50.

## Tests
| Test | Result |
|---|---|
| test_wavepoc_190 (+ the NQ Nov 6 rows from the record; a sub-2x bump is now an HVN; flat rows are not) | 39 passed |
| test_wavepoc_150 | 49 passed |
| test_wavepoc_logic | 187 passed |
| test_wavepoc_host | 109 passed |
| MinGW against the real irtsdk.h | 0 errors, 0 warnings |
