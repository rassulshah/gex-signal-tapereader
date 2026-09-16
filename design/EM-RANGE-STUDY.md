# Does the expected move improve the day model's expected high and low? — the study

_2026-09-16 evening. Operator: "what if you used the expected move to calculate them [the expected high / low]. is its
result better or worse" → "run the study. I want to know how good EM is and if it will improve the model expected
high and low." Runner: `tools/study-em-range.py`. Numbers: `data/es-1min/EM-RANGE-STUDY.json`. No code in the
model was changed — this is the measurement the decision is made on._

## 0. The answer in three lines

1. **As a range predictor the EM is the best single number we have measured.** Before the open it beats the model's
   prior-day-range stage by 19% (MAE 18.3 vs 22.6 ES points, n=299, out-of-fold), and it even beats the model's
   30-minute and 60-minute stages (19.4 / 19.0). Added to those stages it sharpens them a further 9%.
2. **But the expected HIGH and LOW barely move (about 1 point, ~5%),** because the candle is drawn symmetric round the
   open and the error that dominates the high/low is *where the open sits in the day's range*, not how wide the day is.
   With a *perfect* range and the same symmetric placement the high/low error is still 18.4 points — the floor no
   range model can get under.
3. **The lever for the high/low is placement, and it is large:** placing the candle by where the open sits in the first
   hour cuts the expected-high error from 17.4 to 12.8 and the expected-low error from 22.0 to 16.8 (−25%). That is
   the change that would actually improve the expected high and low; the EM is the change that improves the range.

## 1. What was measured

**Sessions.** The same corpus and tool grid as the day model: the ES 1-minute vendor file (EPM26) plus the Yahoo
dailies, `MIN_BARS` complete sessions only — **n = 300, 2025-06-02 → 2026-09-15** (299 with a prior day). 3-minute
bars stamped by end from the 08:30 CT open; range = RTH high − low. Mean range 63.9, median 55.5, sd 33.5.

**The EM series.** The panel's own straddle pin (`feat.emband.em`) exists on 17 days, 9 of them clean — nothing to
fit on. The proxy is **CBOE's VIX1D** (the 1-day implied-volatility index), daily OPEN and CLOSE, fetched through the
operator's browser (`data/vix1d-daily.txt`; the cloud cannot reach cdn.cboe.com). Converted to points as
`ES open × VIX1D/100 / √252`; only proportionality matters because every model fits `a + b·x`.
Two readings were tested:
- **VIX1D open** — the first print of the session, driven by the SPX options expiring *today*; it is the 0DTE straddle
  the panel pins, seen through CBOE's formula. Available at the 08:30 CT open.
- **VIX1D prior close** — frozen at 3 PM ET the day before, driven by the *next-day* options; known before the open
  but pricing a window that includes the overnight.

**Proxy check.** On the 9 clean straddle pins the VIX1D-open proxy sits within 0.91–1.36× of the straddle (mean 1.08×),
so the level is right; the correlation over those 9 days is only 0.33 because the straddles all sit in a 31–42 band
(no dispersion to correlate). The proxy is used *for* the study; the live model would use the real straddle the
companion already carries. Nine days is not a validation of the straddle itself — that is the open item in §5.

**Scoring.** 10 contiguous chronological folds (fit on nine, score the tenth; no day scores itself). MAE of the
predicted **range**; MAE of the predicted **high** (open + range/2) against the actual HOD and of the **low** against the
LOD, because the question is about the candle; and the share of days whose whole range fell *inside* the candle.

## 2. Results — range models (ES points, out-of-fold, n=299)

| model | MAE range | median | MAE high | MAE low | inside | fit on all |
|---|---|---|---|---|---|---|
| constant (mean range) | 25.43 | 21.64 | 19.77 | 23.51 | 29% | 63.9 |
| **EXANTE — prior-day range (the model today, pre-open)** | **22.64** | 17.75 | 18.41 | 23.06 | 30% | 41.7 + 0.346·pdr |
| EM, VIX1D prior close | 20.26 | 15.45 | 18.27 | 22.82 | 24% | 14.6 + 0.806·EM |
| **EM, VIX1D open** | **18.31** | **13.21** | 17.53 | 22.59 | 20% | 1.7 + 1.329·EM |
| EM open + prior-day range | 18.39 | 13.51 | 17.49 | 22.62 | 20% | pdr coefficient 0.007 — the prior day adds nothing once the EM is in |
| **OPEN30 — opening 30-min range (the model today, 30 min in)** | **19.42** | 15.82 | 17.94 | 21.96 | 22% | 26.3 + 1.438·or30 |
| OPEN30 + EM open | 17.61 | 14.12 | 17.43 | 22.21 | 22% | 3.5 + 0.791·or30 + 0.848·EM |
| **OPEN60 — initial balance (the model today, 60 min in)** | **19.02** | 15.31 | 17.90 | 21.50 | 20% | 25.8 + 1.095·or60 |
| OPEN60 + EM open | **17.25** | 13.54 | 17.40 | 21.95 | 20% | 2.4 + 0.661·or60 + 0.824·EM |
| ORACLE — perfect range, symmetric placement | 0 | – | **18.44** | **18.44** | – | the placement error alone |

Reading it:
- The **EM at the open beats every stage the model has**, including the two that wait for 30 and 60 minutes of tape.
  A number known at 08:30 is a better range forecast than the first hour's own range.
- The prior-day range is **dead once the EM is in** (coefficient 0.007). Implied vol subsumes yesterday's realized
  range, as the literature says it should.
- The **high/low columns hardly move** — 18.4 → 17.5 on the high, 23.1 → 22.6 on the low — and the ORACLE row says why:
  even a perfect range, placed symmetric round the open, leaves 18.4 points of error on each side. The range is not
  what is wrong with the expected high and low.
- **"Inside" falls** (30% → 20%) as the models improve. That is not a defect: the prior-day stage over-predicts quiet
  days by 12 points (§3), so its candle was wide by accident. A tighter, unbiased candle contains fewer whole days. If
  the candle is meant to *contain* the day rather than *estimate* it, that is a different objective (a quantile, not
  a mean) and a separate decision.

## 3. Where the EM helps — by the day's implied vol (VIX1D prior-close terciles)

| days | n | prior-day MAE | EM MAE | mean range | prior-day bias | EM bias |
|---|---|---|---|---|---|---|
| quiet (VIX1D < 11.1) | 99 | 18.25 | **13.91** | 46.3 | **+12.2** | +3.8 |
| normal (11.1–14.4) | 100 | 22.07 | 20.13 | 60.9 | +1.4 | −1.5 |
| event (≥ 14.4) | 100 | 27.54 | 26.69 | 84.3 | **−13.3** | −1.4 |

The prior-day stage is biased in both tails — too wide on quiet days, too narrow on event days — because yesterday's
range is only a loose guide to today's. The EM removes the bias in both tails (+3.8 / −1.4) and takes a quarter off
the quiet-day error. On event days the *absolute* error stays large for both (the tail of an FOMC day is not
forecastable), but the EM is at least not biased there.

The ten days where the two disagree most (`study-em-range.py`, last block) are all high-VIX1D days: on 2026-03-09
(range 177) the prior day under-called by 115 points, the EM by 45; on 2026-04-08 and 2026-03-16 the EM over-called
(+74, +73) after a vol spike that the day then did not deliver — the EM's own failure mode is the day *after* the
event, when implied vol is still high and the tape has calmed.

## 4. The high/low lever — placement (out-of-fold, n=299)

Predict the day's **upside share** `f = (HOD − open) / range` from where the open sits in the opening range
(`pos = (open − low) / (high − low)` of the first 30 / 60 minutes), then draw `eHi = open + f·range`,
`eLo = open − (1 − f)·range` with the range from the best model at that stage.

| stage | MAE high | MAE low | vs symmetric |
|---|---|---|---|
| 30 min in: OPEN30 + EM, placed by pos30 | **14.31** | **19.69** | 17.43 / 22.21 → −18% / −11% |
| 60 min in: OPEN60 + EM, placed by pos60 | **12.82** | **16.78** | 17.40 / 21.95 → −26% / −24% |

Fit: upside share ≈ 0.88 − 0.77·pos60 — an open at the bottom of the first hour's range (pos 0) leaves ~88% of the
day's range above it; an open at the top leaves ~11%. This is the same information the OPEN30/OPEN60 stages already
use (the first hour is inside the day), applied to *where* the candle sits instead of only *how tall* it is. Before
30 minutes there is no placement signal in this study; the candle stays symmetric pre-open.

## 5. What this does and does not establish

- Established (n=299, 2025-06 → 2026-09, out-of-fold): the EM is the best range predictor at every stage; the
  prior-day range is redundant beside it; the high/low error is placement-dominated; placement by the opening range
  cuts it by a quarter after the first hour.
- **Not** established: that the **live straddle EM** the companion pins predicts as well as the VIX1D-open proxy. The
  level agrees (9 clean days, 0.91–1.36×) but 9 days cannot validate it. The straddle is the *same* quantity CBOE
  folds into VIX1D at the open, so the expectation is that it does; the guard is to record the pinned EM once a day
  (`data/`) and re-run this study with the real series as it accumulates. The nightly re-fit (`BASERATES.predict`)
  should carry the EM coefficient only once ~60 real pins exist; until then the proxy coefficients are the fallback.
- Not measured: the regime sign as a skew on the candle (the operator's earlier item C); an EM-based *quantile* candle
  ("contains the day 80% of the time") if that is the objective; NQ.

## 6. Recommendation (a proposal — nothing built)

1. **EM into the range at every stage** — `EXANTE: 1.7 + 1.33·EM` (replacing the prior-day range, which is dead beside
   it), `OPEN30: 3.5 + 0.79·or30 + 0.85·EM`, `OPEN60: 2.4 + 0.66·or60 + 0.82·EM`; prior-day range stays only as the
   fallback when no EM is pinned. Gains: −19% / −9% / −9% on the range.
2. **Placement after 30 minutes** — upside share from `pos30`, then `pos60`; the candle stops being symmetric once the
   opening range exists. Gain: −18…−26% on the expected high and low — the larger effect, and the one the operator's
   question is actually about.
3. Record the pinned straddle EM daily so (1) is re-fit on the real series.

Order if built: (2) first — it is where the expected high/low improve; (1) second. One at a time, mockup first.
