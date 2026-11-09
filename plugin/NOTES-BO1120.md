# lsBreakout / lsBreakout1 1.12.0 - Pboc (2026-11-09)

Rassul 12:22 CT: "if a pb tags the wave poc with the downpivot at or below the wave poc but the high is above it, then instead of pb, show pboc".
Refined for Rly waves without a BO ("a halfway back + a test of the wave POC"). Mockup https://claude.ai/artifact/BGng3DGC4jx2LEiAdsuBrs approved, "build".

## Rule
- **Wave POC** = lsWavePOC's own rule, computed natively in lsBreakout on IRT's VAP (VAP_REQUIRED; lsWavePOC's SEH-guarded reader):
  the ZigZag leg's bars (start pivot .. end pivot), rows = wpc::rowTicksFor(range), nodes = wpc::pickNodes(rows, 2 x median), POC = node 0's row centre.
  Checked against lsWavePOC's recorded POC: ES 11-09 06:27-06:51 = 7453.75 both.
- **BO / BD wave Pb** (unchanged Pb rule): drawn **Pboc** (same place / colour) when the pivot bar's LOW <= POC < HIGH (BD: HIGH >= POC > LOW). No 50% condition.
- **Rly wave with no shown BO / BD of its direction inside it** (start pivot, wave end]: its first 2 confirmed 3-bar ZigZag pullback pivots (Rly up: pivot LOWs)
  after the wave end; none after a bar trades beyond the wave origin, or after the next BO / BD or Rly known by the pivot's decision bar.
  **Pboc** when the POC is at / below the wave's 50% (origin low + end high) / 2 (Rly down: at / above) AND the pivot bar straddles the POC. Otherwise no label.
  Confirmed pivots only (no provisional) - never repaints.
- **No VAP yet** for the wave (e.g. after a restart): NOTHING is drawn at that pivot until IRT has it; then Pb or Pboc once (never Pb -> Pboc). The BPB line waits too.
- An **Lvp** on the pivot still hides the letter (1.11.1).
- Pb / Rev decisions unchanged. The thrust off a Rly Pboc (the Rev rule) is recorded as **BRO|**, never drawn.

## Records (lsFlexLevels\Breakout\<MKT>-<SYM>-<spb>-pb-<daykey>.csv)
- `BPB|...|version|boc|wave_poc` - 16 fields (boc 1 Pboc / 0 Pb / na no POC). Older 14-field lines still keyed.
- `BPO|t_pivot|side|t_rly_end|t_rly_start|t_conf|t_dec|pivot_high|pivot_low|wave_poc|wave_mid|origin|version` - a Rly Pboc.
- `BRO|` = BRV layout - the thrust off a Rly Pboc.

## Status
`PBOC` (decided Pboc / Pb, waiting for VAP, no POC, drawn now, Pb not drawn waiting for VAP) and `RLY_PBOC` (judged, Pboc, waiting, thrusts).

## Tests (tests_bo100/test_bo1120_pboc.cpp, real ES footprint export: pboc_es1109 / es1021 / es1104.txt) - 24 / 24
- ES 11-09 (bar-by-bar replay): Pboc 23:24 (11-08), 00:33, 07:15; Pb 20:03 (11-08), 00:45, 07:45. 07:15 wave 06:27-06:51, POC 7453.75 = lsWavePOC's record.
- Late VAP: 07:15 drawn nothing until the VAP arrives, then Pboc once; BPB written once with boc 1.
- ES 10-21: no-BO Rly 07:45-08:03 -> Pboc 08:15 (also 03:27); BPO / BRO keys. ES 11-04: no-BO Rly 08:03-08:24, POC above 50% -> 08:30 / 08:36 no label.
- No confirmed label ever changed; Lvp hides a Pboc.
- NQ 11-09 (no NQ VAP export here): checked against lsWavePOC's recorded POCs - Pboc 00:24, 04:06, 07:24; HG none.
- Regression: every other tests_bo100 suite gives the same results as 1.11.1 (only the version pin in test_bo1111 updated). MinGW vs the real irtsdk.h: Breakout / Breakout1 / WavePOC 0 errors, 0 warnings.

## lsWavePOC 1.10.2
Rebuild only (it compiles BreakoutLogic.h for its BO / Rly / Rev inputs; it never hands VAP to it, so no Pboc there). Tests: t1100 26/26, host 108/1, t190 42/3 (the old failures).
