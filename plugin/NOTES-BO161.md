# lsBreakout1 "BO 1-bar" 1.6.0 (2026-11-04 21:25-21:31 CT)

Rassul 21:25: "there needs to be a 1bar option for breakout indicator .. this will help when i put it on higher timeframes". He said "go" at 21:31, on the plan of a separate indicator, since the settings windows don't work.

## What it is
- `Breakout1.cpp` sets `BKO_ONE_BAR 1` and includes `Breakout.cpp`. It builds `lsBreakout1.dll`; gex-build has a new line for it after lsBreakout.
- **1-bar ZigZag:** the BO / BD pivots, the waves (one per wave, follow-through), Rly and the Pb pivots all come from it. The 3-bar build uses the 3-bar ZigZag for these.
- **Same rules as lsBreakout 1.6.0:**
  - The follow-through close may equal the BO close (tie).
  - The zigzag carries across the 17:00 CT break.
  - A Pb comes back to the broken pivot, or stops less than ATR(14) / 3 short of it.
  - No BI.
  - The tie and carry were limited to zzRev > 1. `Engine::refine1` turns them on for the 1-bar build. lsBigBidAsk and the old zzRev = 1 test path are unchanged.
- **Records** go to `lsFlexLevels\Breakout1` (record version `1.6.0-zz1`). The status file is `Breakout1.status-<MKT>-<spb>.txt`. This keeps the nightly data apart from the 3-bar build.
- Rly is a 3-min signal, so it stays at 0 on higher timeframes.
- **lsBreakout (3-bar) is unchanged.** Only its settings description was refreshed: it was still the 1.5.0 text, which mentioned BI.

## On 15-min bars (the ES / NQ / GC 3-min weeks rebuilt as 15-min)
| Market | 15-min bars | BO/BD fired | Pb |
|---|---|---|---|
| ES | 929 | 39 | 53 |
| NQ | 736 | 31 | 51 |
| GC | 736 | 28 | 27 |

## Tests
- **tests_bo100/test_bo161_onebar (new): 24 passed.**
  - The build settings: zzRev 1, tie and carry on, records go to Breakout1.
  - Engine pivots equal an independent 1-bar zzd walk.
  - Every Pb sits on a 1-bar pivot.
  - The ⅓ ATR rule holds.
  - A bar-by-bar replay equals one full pass (no repaint).
  - The build equals a default engine set to zzRev 1 + refine1.
  - The wrapper and its version line are correct.
- **The full 3-bar suite (run_tests.sh)** checks that lsBreakout is unchanged, then runs MinGW against the real irtsdk.h for both Breakout.cpp and Breakout1.cpp.
- **Not done:** no Python parity for 1-bar. bo_pb.py only models the 3-bar path.
