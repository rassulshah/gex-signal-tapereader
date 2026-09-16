# Do the key levels improve the expected high / low and the range? — the study

_2026-09-16 evening. Operator: "I am particularly interested in whether key levels (which you have) like overnight high
and low and day high and low etc., can improve these numbers even more." Runner: `tools/study-key-levels.py`
(`EXTRA=1` for the controls). Same 279 sessions (2025-06-09 → 2026-08-21: the vendor 1-minute file, which the Globex
levels need), same folds and MAE as `EM-RANGE-STUDY.md`. Levels: ONH/ONL, PDH/PDL, PDC, AHI/ALO, LHI/LLO, PMH/PML,
PWH/PWL — the sweep study's definitions._

## 0. The answer
**No — not for these numbers.** None of the levels moves the range, the placement, or the expected high/low by more
than a fraction of a point, before the open or after the first hour. The day's extremes sit on a level only slightly
more often than chance. The one hint worth keeping — a level within ~5 points of the expected extreme seems to stop
the day more often than a plain price at the same distance — rests on 15–25 days per cell and is recorded, not built.

## 1. Range (A) — level features added to the EM model, out-of-fold MAE
| | pre-open | 60 min |
|---|---|---|
| EM model (reference) | 18.57 | 17.69 |
| + room to the nearest level up and down | +0.05 | +0.03 |
| + the ON ∪ PD envelope span | +0.18 | +0.19 |
| + both | +0.22 | +0.19 |
Nothing. The overnight range was already null in the second EM pass; the envelope and the rooms are the same
information in other clothes.

## 2. Placement (B) — the open's position among the levels → the day's upside share
| stage | model | MAE E-HOD | MAE E-LOD |
|---|---|---|---|
| pre-open | symmetric | 17.44 | 23.37 |
| pre-open | room share up/(up+down) | 17.52 | 23.52 (slope −0.03) |
| pre-open | position in the ON ∪ PD envelope | 17.54 | 23.26 (slope −0.09) |
| 60 min | pos60 (the built recommendation) | 12.83 | 17.55 |
| 60 min | pos60 + room share | 12.83 | 17.66 |
| 60 min | pos60 + envelope position | 12.85 | 17.57 |
Pre-open the slopes are ~0: where the open sits relative to last night's or yesterday's range says nothing about
which way today's range hangs. After the first hour the opening range already carries everything.

## 3. Magnet (C) — does the HOD / LOD land ON a level?
Within k points of a level above the open (HOD) / below it (LOD), real days vs a permutation control (the same day's
levels against an extreme of the same distance drawn from another day, 20 shuffles):
| k | HOD real | HOD permuted | LOD real | LOD permuted |
|---|---|---|---|---|
| 2 | 23% | 21% | 23% | 23% |
| 3 | 31% | 28% | 29% | 31% |
| 5 | 45% | 41% | 46% | 44% |
A few points above chance on the high side, none on the low side. When an extreme does sit on a level (k=3), the
levels it sits on most are the ones nearest the open — PML/PMH, LLO/LHI, ONL/ONH (28–33 each) — then PDH 18, PWH 16,
PDC 16, PDL 14. (First-pass "chance" printed by the script's section C is a span-coverage figure and overstates
chance because the levels cluster near the open; the permutation is the fair control.)

## 4. Snap and cap (D) — the practical rules, on the 60-minute placed model
Snapping E-HOD / E-LOD to the nearest level within X points: X=5 → 12.83 → 12.73 / 17.55 → 17.42; X=8 → 12.66 / 17.49;
X=12 → back to 12.81 / 17.59. A tenth of a point — noise.
Through-rate at a LEVEL vs at a PLAIN price the same distance inside the expected extreme (D′):
| distance inside E-HOD | level n | through | plain n | through |
|---|---|---|---|---|
| +5 | 22 | 32% | 213 | 39% |
| +8 | 26 | 54% | 217 | 54% |
| +12 | 11 | 73% | 235 | 70% |
| distance inside E-LOD | | | | |
| +5 | 15 | 20% | 219 | 43% |
| +8 | 23 | 61% | 227 | 46% |
| +12 | 14 | 29% | 231 | 71% |
The +5 rows (a level five points inside the expected extreme is got through 32% / 20% of the time vs 39% / 43% for a
plain price) are the hint; the +8 and +12 rows contradict it on one side each, and every level cell is 11–26 days.
Not enough to build on. Re-run when the corpus is twice the size, or on the recorded sweep events instead.

## 5. Where the levels DO belong
The sweep study (`tools/study-sweeps.py`, `data/es-1min/SWEEPS.json`, n=295) answers the level question at the
event level — when a sweep-and-reclaim of ONL / PDL / PML happens, how often that print is the LOD — and that is the
right use of a level: as the *place* an extreme prints when the tape does something there, read live by the SWEPT row,
not as a forecast input before the tape has moved. The forecast inputs that remain untested are the ones with no
history: the regime sign, the flip distance, the walls, the King (the daily record row).
