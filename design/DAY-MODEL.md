# THE DAY MODEL — the one source of truth

_Last updated 2026-09-15 (panel v16.19). If any other doc, comment, tab or mockup disagrees with this
file about the day model, THIS file wins and the other is stale — fix it._

The day model answers two DIFFERENT questions with two DIFFERENT, never-fused layers. Confusing them
has cost time before, so they are stated separately and explicitly.

---

## LAYER 1 — THE EXPECTED CANDLE (geometry): "what will today's range and shape be?"

This is the ghost/expected candle `lsDayModel` draws and the `E` row `lsDayStats` shows. It is written by
the panel (`gammaProfileBuild`, the `if(haveE)` block ~L6631) into `DAYEXP / DAYEHOD / DAYELOD / DAYEMUD /
DAYSE / EXPMODEL` rows of `GammaProfile.csv`.

### It is ADAPTIVE and PREDICTIVE (as of v16.18–16.19). It was NOT before.
The old model (≤ v16.16) drew the weekday trimmed-mean range, symmetric about the open, `close = open`
(no body). **Backtested, that is not predictive — R² 2.3%, MAE 24.2 pt.** It draws the same ~56-pt Monday
every Monday. The weekday mean is a REFERENCE, not a forecast. So the RANGE is now predicted from today's
own tape, in three stages, and the BODY leans with the opening drive:

| stage | when | RANGE predictor | out-of-fold |
|---|---|---|---|
| **exante** | pre-open / first 30 min | `a + b·priorDayRange` | MAE 23.0 (beats weekday 24.2) |
| **open30** | ≥ 30 min of RTH in | `a + b·openingRange30` | **R² 30%, MAE 19.5** |
| **open60** | ≥ 60 min of RTH in | `a + b·(60-min initial balance)` | R² 32%, MAE 18.5 |

**DIRECTION (the body):** neutral (close = open) until 30 min; from 30 min on, the body leans a modest
¼ of the range toward the sign of the **opening-30-min drive** (close₃₀ − open), which calls the day's
close **68%** of the time (base green rate 53%). This is a *lean*, not a confident arrow.

**The clock/timing fields (E-HOD/E-LOD times, took, gap, wick family) still come from the weekday
reference** — only the range and body direction are predicted. That split is deliberate: the opening
range predicts *magnitude*; the weekday climatology is the best estimate we have for *when*.

**Guardrail:** the predicted range is clamped to 0.4×–2.5× the weekday mean so one bad bar can't draw an
absurd candle. `EXPMODEL,<basis>,<rngPts>,<driveSign>` names which stage drew it (`exante`/`open30`/`open60`).

### What was TESTED and REJECTED (do not re-add — measured null over 283–298 ES sessions):
- Volume / relative opening volume → no MAE change (it's a proxy for the opening range).
- Momentum (|drive| magnitude), gap size, prior-day range on top of the opening range → no help or worse.
- Direction: open-location (50%), gap (46%), volume-gated drive (thin) → nothing beats the 68% opening drive.
- The v16.17 "recent-weekday green/red lean" body → 47%, worse than a coin flip. **Removed in v16.18.**
See FINDINGS F-4 (ship the table, not a regression) and F-6 (a predicted green/red close is overconfident).

### SELF-CALIBRATING (the coefficients are not hardcoded)
`tools/study-hodlod.py` → `_predict_block()` re-fits `exante / open30 / open60 / dir30` every night over the
growing corpus and writes them to `BASERATES.json → predict`. `tools/bake-hodlod.py` carries them into the
panel's `HODLOD_BASE.predict` literal at build time; `hlBaseNormalise` passes `predict` through so the
couriered live file overrides the literal. The panel's hardcoded coefficients are only the boot fallback.
As the corpus grows, the model re-fits itself. Baked 2026-09-15 (298 sessions):
`open30 a=25.9 b=1.44 R²0.30 · open60 a=25.5 b=1.10 R²0.32 · exante a=40.7 b=0.36 · dir30 0.674`.

### Scale note (a landmine — do not repeat)
The range features are computed from `measureBars(sym).bars` via `hlToolBars` — **ES-native on the ES chart
(scale 1)**, same tool grid + RTH open + ES points as the corpus, so the coefficients transfer. Do NOT use
`closedCandles(sym)` here — it returns the underlying SPY-scale book (×rr) and would poison the range.

---

## LAYER 2 — THE READ (classifier): "has the HOD or LOD already printed?"

Completely separate. `tools/model-lodhod.py` builds `HLTAB`, a 2-axis lookup (posr = how far price has
travelled off the standing extreme × minutes-since-open). **AUC 0.879**, regime-stable (better on volatile
days), transfers ES↔NQ (F-7). It answers "is the standing extreme the day's" — NOT the candle geometry, and
it is NOT one of the range predictors above. Two axes only — a 3rd made it worse (F-4).

- It runs live in the browser panel's READ box and, since v16.17, is written to the CSV as
  `READ,<first HOD|LOD>,<posr%>,<cellPct>,<cellN>,<call IN|NOTIN|HOLD>` and rendered center-justified on the
  `lsDayStats` title line.
- **NOT-IN is the stronger call** (85% vs the IN call's 63%); operating point P ≥ 0.70 (F-5).
- ⚠ The cell % is a per-bar CELL rate (how often an extreme in this state was the day's), re-read every bar —
  it is NOT a forecast of price. The DECISION rate is a different, lower number.

> If a future context is unsure which model the candle uses: **Layer 1**, the adaptive range from today's
> opening range (`E.predict.open30/open60`), body from the opening drive. If unsure what AUC 0.879 is:
> **Layer 2**, the READ, a different question. Do not merge them.

---

## THE ONE UNEXPLORED LEVER (where future modelling effort goes)
Classical indicators are exhausted (all null above). The input most likely to carry signal the opening range
does NOT already contain is the **GEX structure itself** — King distance, air-pocket width, node density at
the open. That is the tool's whole thesis (hypothesis **H5** in `learning/register.json`), currently
**blocked** on the event-level ledger the nightly harness is only starting to accrue. Measure structure when
the ledger exists; do not add classical indicators.

---

## FILE / FUNCTION MAP (so a fresh session finds it fast)
- Panel export of the expected candle: `current/gex-signal-tapereader.user.js`, `gammaProfileBuild()`, the
  `if(haveE)` block (~L6631) — the 3-stage range + drive body.
- Coefficients source of truth: `data/es-1min/BASERATES.json → predict`; re-fit by `tools/study-hodlod.py`
  `_predict_block()`; baked into `HODLOD_BASE.predict` by `tools/bake-hodlod.py`; read via `hodlodBaseFor(dow)`.
- The READ: `tools/model-lodhod.py` (HLTAB), `lodhodCall()` in the panel, `HLTAB_META`.
- Plugins that draw it: `plugin/DayModel.cpp` (candle, v0.13), `plugin/DayStats.cpp` (stats strip + READ line, v0.5).
- CSV schema: `design/DAY-MODEL-PLUGIN.md`.
- Evidence: `skylit-docs/FINDINGS.md` F-4/F-5/F-6/F-7; the backtests are reproducible from `study-hodlod.py`.
