# The Day Stats E row — conditional on the morning, measured against the weekday base

_2026-09-16 evening. Operator: "I am curious about the day stats, how can that model be improved?" → recommendation
(clocks and 1ST from the first 30/60 minutes instead of the weekday mean) → "I'll go with your recommendation, do what
you need to do but make sure you test and confirm that the model is better than base." Runner:
`tools/study-daystats-cond.py`; numbers `data/es-1min/DAYSTATS-COND-STUDY.json`; built as **panel v16.32**
(`hodlodCondE`, `gpOpenWindow`, the `CONDE` row; `study-hodlod.py` now emits `condstats` nightly)._

## 1. The base, scored honestly
The E row lsDayStats prints is `hodlodBaseFor(dow)`: the weekday's Tukey-trimmed mean of the first- and second-extreme
clocks (Took = 1ST − open, HL Gap = 2ND − 1ST) and the weekday's majority for 1ST = LOD/HOD. Scored out-of-fold on the
same 300 ES sessions (2025-06-02 → 2026-09-15, 10 chronological folds, the base re-derived inside each fold):

| model (out-of-fold) | 1ST clock MAE (min) | 2ND clock MAE | 1ST = LOD accuracy |
|---|---|---|---|
| **BASE — weekday trimmed mean / majority (the panel until 16.31)** | **38.4** | **93.1** | **0.483** |
| pre-open — pooled median / majority | 36.5 | 93.6 | 0.517 |
| 30 min — conditional on the open's position in the OR (terciles) | 36.6 | 95.5 | 0.593 |
| 30 min — position × drive | 36.4 | 95.7 | 0.573 |
| 30 min — READ input (close far from a running extreme) | 37.0 | 93.4 | 0.593 |
| 60 min — conditional on the open's position in the IB | 36.2 | 94.4 | 0.613 |
| 60 min — position × drive | 36.3 | 95.1 | 0.640 |
| **60 min — OR-extreme clock in the outer thirds, else the table** | **33.3** | 94.4 | 0.613 |
| 60 min — READ input | 33.6 | 93.5 | 0.643 |

## 2. Verdict — better than base where it claims to be, and honest where it is not
- **1ST = LOD/HOD**: base 48% (worse than a coin, because the weekday majority is 44–60% noise) → 59% at 30 min →
  61% at 60 min → a FACT once the READ says IN. **Better.**
- **1ST clock**: base 38.4 min → 36.5 pre-open (a median beats a trimmed mean on a distribution whose median is 24 and
  mean 45) → 33.3 at 60 min with the OR-extreme rule (an open in the bottom third of the first hour: the low it printed
  early is the day's first extreme 66% of the time, and its clock is known). **Better, by 13%.**
- **2ND clock**: base 93.1 min; nothing known in the morning predicts it (93–96 whichever feature). The pooled median is
  carried (93.6 — the same). **Not better, and no claim is made.** 39% of second extremes print in the last hour, 29%
  in the last 30 minutes; the honest display is the ladder, not a clock — a proposed lsDayStats change, not built.
- **Took / HL Gap** follow the clocks (Took = 1ST; Gap = 2ND − 1ST, so it inherits the 2ND's error).

## 3. The tables (fit on all 300; `condstats` in BASERATES.json, `GP_COND_FALLBACK` in the panel)
| window | open's position | n | 1ST median (min) | 2ND median | LOD-first |
|---|---|---|---|---|---|
| OR30 | bottom third | 114 | 21 | 286 | 59% |
| OR30 | middle | 81 | 33 | 294 | 58% |
| OR30 | top third | 105 | 21 | 282 | 39% |
| IB60 | bottom third | 108 | 16.5 (rule: the window low's clock) | 303 | 66% |
| IB60 | middle | 93 | 36 | 306 | 53% |
| IB60 | top third | 99 | 21 (rule: the window high's clock) | 267 | 35% |
Pooled: 1ST median 24, 2ND median 286.5, LOD-first 52%.

## 4. What shipped (v16.32) and what did not
Shipped: the E row's 1ST / 2ND clocks, Took, HL Gap and 1ST = LOD/HOD come from `hodlodCondE` by stage (pre-open →
30 min → 60 min → READ IN); the DAYEHOD / DAYELOD tips and DAYSE carry them; a `CONDE,<basis>,<t1>,<t2>,<lod%>,<n>` row
says which stage drew them; `study-hodlod.py` re-fits the tables nightly; `test_daystats_cond.js` (27 assertions, 4
mutations fire). The weekday still drives the wick family and the range's fallback; the range itself is the adaptive
model (v16.18) — the EM's entry into it is `design/EM-RANGE-STUDY.md`, not this.
Not shipped: the lsDayStats LADDER for the 2ND clock (a plugin change — mockup first); any regime / flip / King
conditioning (no history; needs the daily record row).
