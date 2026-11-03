# lsBreakout 1.2.0 - BO / BD and Rly on the 3-bar High / Low ZigZag (2026-11-03)

## Request
Rassul said "go" (2026-11-03): check BO / BD and Rly on the 3-bar High/Low ZigZag over a whole week, then switch the "BO" indicator
(lsBreakout) to 3-bar so BO / BD, Rly and Pb all use the same swings. Pb was already 3-bar (1.1.0).

## Week check (3-min from fp_bars, ES Oct 26 - Nov 3 = 7 sessions, NQ / GC Oct 26 - 30 = 5 sessions each)
Same engine both ways. The only change is the ZigZag turn rule, which now matches zzd::Engine::step N = 3 (the lsZzoCombo 2.1.0 / Pb rule).
Picture: bo3/week_1v3.png (scratch).
- BO / BD "works" = within 10 bars after the mark, price goes 1 ATR(14) beyond the breakout candle before trading back through the
  broken pivot.
  - RTH: 1-bar 58/94 = 62%, 3-bar 50/76 = 66%.
  - Full session: 173/315 = 55% both ways.
  - Per market (RTH): ES 31/44 -> 24/37, NQ 14/29 -> 14/23, GC 13/21 -> 12/16.
- Rly "works" = within 10 bars after the signal bar, price retraces 1 ATR from the extreme before making a new extreme.
  - Measured from the extreme: 52/65 = 80% (1-bar), 64/69 = 93% (3-bar).
  - Stricter, a FURTHER 1 ATR from the signal close: 28/65 = 43% -> 38/69 = 55%.
  - The 3-bar Rly comes about 1 bar later: at the signal, price is already 1.3 - 1.6 ATR off the extreme, against about 1.0 ATR for 1-bar.
- The 3-bar version drops about 15% of BO / BD (the breaks of swings that only the 1-bar walk sees). About 1 in 4 of the BO / BD that
  are kept get a different pivot or follow-through candle.
- The samples are small, so this is NOT conclusive. It only shows that 3-bar is not worse.

## What changed (rev = 1 keeps the old code path byte for byte: lsBigBidAsk, lsSweepRefill and lsStopRun are unchanged)
- `SwingRef.h`: ZigZag::rev (default 1). When rev > 1 a swing turns on the zzd High / Low rule, with no outside-candle exception.
- `BigBidAskLogic.h`: swingBreaksPx(..., rev = 1), zigDir(..., rev = 1) and boFromCandles(..., keep, rev = 1).
  bba::VERSION stays 1.32.0 because the behaviour for rev 1 does not change.
- `RallyLogic.h`: zigzag / judge / run(..., rev = 1). rly::VERSION stays 1.1.0.
- `BreakoutLogic.h` 1.2.0:
  - New: `ZZ_REV_BARS = zzd::COMBO_REV_BARS` (3).
  - New: Engine::zzRev (default 3; tests set 1 to replay lsBigBidAsk).
  - boPass and rlyPass pass zzRev.
  - The status RULES line says "on the 3-bar High/Low ZigZag".
- `Breakout.cpp` 1.2.0: version literals and description. The host code is unchanged, with no new includes, so the gex-build line is unchanged.
- Every other threshold is unchanged: 10-bar extreme, range / volume, colour, follow-through in the same wave, 20-bar counter-trend,
  and Rly 2.5 ATR / 1 SD or 30-bar / 20-bar / trend.

## Tests (tests_bo100/run_tests.sh, ASan / UBSan -Werror)
- test_bo100_parity: 9 / 9. With zzRev = 1 it still matches lsBigBidAsk's recorded ES Oct 30 decisions: BO / BD 20 / 20, Rly 7 / 7.
  No repaint, and a restart reproduces the same result.
- test_bo120_week (new): 21 / 21, ES / NQ / GC.
  - A bar-by-bar replay and one full pass are both identical to the independent week harness (REVN = 3 header copies). That covers every BO
    decision with its reason, pivot and mark candle, every Rly wave and every Pb.
  - No repaint, and a restart matches the replay.
  - zigDir with rev 1 equals the default 1-bar walk.
- test_bo110_pb: Pb parity with the 1.1.0 mockup runs on zzRev = 1. The Pb rule is unchanged, and the mockup was drawn against 1-bar BOs.
- test_bo100_host: the "VERSION,1.2.0" status line and the RULES text are checked, along with layout, Pb two rows out and no overlaps.
- Week parity check: the new code with zzRev = 1 gives exactly the 1-bar harness output (the original code) on ES, NQ and GC.
- MinGW syntax check against the real irtsdk.h for Breakout, BigBidAsk, SweepRefill, StopRunMark, ReversalStage and Trapped: 0 errors and
  0 warnings outside the SDK.
- Related suites (tests_bigba, tests_bba115, tests_bo128/129/130, tests_rly, tests_sr, tests_st, tests_hodlod t_hl244, tests_strips):
  each one gives the same result before and after the change. See the hand-back report.

## Not built - waiting for Rassul (his 3 "should have had BO" screenshots, Nov 3)
1. 9:42 AM: the candle broke the 9:21 swing high (7601.75) but got no_confirm, because the 1-bar wave ended on 9:48 before a follow-through.
   **The 3-bar switch fixes this:** BO on 9:42, shown at the 9:48 close.
2. 11:48 AM broke no swing. 11:51 broke the 11:30 high (7611.00) but got no_confirm, because the 1-bar wave ended on 11:57.
   **The 3-bar switch fixes this:** BO on 11:51, shown at the 12:03 close. 11:48 still gets nothing, because it broke no swing high.
3. 5:03 PM (Globex open) over the 3:48 swing high (7611.50) got nothing. The swing walk restarts at 5 PM, so the new session has no
   swing yet.
   - Proposed fix: keep the swings across the 4-5 PM break.
   - Week effect with 3-bar: +7 BO / BD, all between 5:21 and 6:27 PM, 4 of 7 work. Nothing is lost and RTH is unchanged.
   - NOT built until he says OK. Mockup: bo3/missed_bo.png.

## Risks / not verified - needs his IRT
- lsBigBidAsk still decides and records BO / BD and Rly on the 1-bar walk (its records, logs and nightly inputs). The letters on the chart
  (lsBreakout) now follow the 3-bar walk, so the drawn set and lsBigBidAsk's records differ on purpose, by about 15% fewer BO / BD.
- The 3-bar Rly appears about 1 bar later than before.
- Installs only after File -> Exit of IRT. BUILD-STATUS shows "built, not installed - PENDING" while IRT is open.
