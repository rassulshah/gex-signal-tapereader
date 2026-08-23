# IS THE FRAME APPROACH SOUND? — review, 2026-08-23

**The approach under review.** Frame the day with the expected move → anchor it at the open to get an
expected high and low → track how much is used and how much remains on each side → mark gamma nodes on
the path as accelerators or brakes → read whether there is fuel to reach the target or friction that
turns it back.

**Verdict: 6.5 / 10 as built. The reasoning is better than the calibration.** The causal chain is right
and in two places it is better than what is published. The statistics underneath it are measuring the
wrong quantity, and nothing has been measured yet. Both are fixable and neither requires redesign.

---

## SCORECARD

| dimension | score | why |
|---|---|---|
| Conceptual frame | **9** | destination → path → obstacles is a causal chain, not a dashboard |
| Anchor choice | **9** | static open anchor beats the floating-spot convention for this purpose |
| Statistical calibration | **4** | a displacement statistic used as a range budget, no time normalisation |
| Epistemic discipline | **8** | refuses dollars→points, labels the sign assumption, keeps the books apart |
| Demonstrated edge | **2** | every scorecard is at zero records; every threshold is hand-set |

---

## WHAT IS RIGHT, AND WHY IT IS RIGHT

**1. The static open anchor is better than the published convention.** MenthorQ's guide anchors the band
to **current spot**, and most overlays do the same. A band recentred on price cannot express "used up" —
it is always centred on you, so it can never say you have gone too far. Anchoring once at the open and
holding it is what makes the used/remaining read possible at all. And it is *correct*, not merely useful:
a straddle captured at the open prices the move from the open to expiry, so the open is the reference the
number actually belongs to.

**2. "Acceleration point" is the vendor's own vocabulary, not an invention.** SpotGamma describes a gamma
wall as something that *"can act as a magnet, resistance reference, or acceleration point depending on who
owns the options, time to expiry, and new flow"*. The accelerator/brake framing is mainstream.

**3. The discipline about what gamma can and cannot say is exactly right.** SpotGamma states that GEX
*"can therefore help answer, 'How sensitive might the market be to hedge rebalancing around this price?'
It cannot answer, by itself, 'Will the market go up or down?'"* — and that it cannot establish *"the timing
or size of an actual hedge trade"*. The panel already refuses to convert dollars of hedging into points of
movement, and says so in the hover. That is the single most commonly violated limit in this whole product
category, and this build does not violate it.

**4. Reading the day as one sentence beats a scoreboard.** Most tools show five gauges and leave the
synthesis to the trader at the worst possible moment. This one composes.

---

## FLAW 1 — THE UNIT MISMATCH. THIS IS THE BIG ONE.

**The expected move is a DISPLACEMENT statistic. It is being used as a RANGE budget.** Those are
different distributions, and the difference is not small — it is exactly a factor of two.

For a driftless diffusion over a session:

    E |close − open|  =  σ√(2T/π)  ≈  0.798 σ√T        where price ENDS
    E [high − low]    =  σ√(8T/π)  ≈  1.596 σ√T        how far price TRAVELS
    ratio                                =  2.000 exactly

The band's half-width is the straddle, ≈ 0.80 σ√T, so the **full band is 1.60 σ√T** — and the **expected
daily range is 1.596 σ√T.**

> **On an average day the market traverses the entire width of the band.**

So "how much is left" reads near-exhausted on a completely ordinary session. The gauge is calibrated so
that *normal* looks like *extended* — which is the opposite of what an overextension read is for.

**The touch statistic says the same thing from the other side.** Probability of touching a level runs at
roughly twice the probability of finishing beyond it (a rule of thumb, and it skews). At 0.8σ:

| band | closes inside | closes beyond, per side | ≈ touches, per side |
|---|---|---|---|
| **straddle, 0.80σ (what ships)** | **57.6%** | **21.2%** | **~42%** |
| 1σ (straddle × 1.25) | 68.3% | 15.9% | ~32% |

A boundary that gets tagged four sessions in ten on each side is not a boundary.

**⚠ The honest counterweight.** The variance risk premium runs the other way: implied exceeds subsequent
realised roughly **79% of the time** at 30 DTE, so the straddle is systematically padded. The band is too
narrow for *range* and too wide for *displacement*, and the two errors partially cancel. **That is why the
tool still looks sensible in use — and exactly why it cannot be trusted quantitatively until it is
measured rather than reasoned about.** The recorder is already in place; this is a measurement question,
not an argument.

---

## FLAW 2 — 0.8σ IS SHIPPING UNDER THE NAME "EM"

Already open, now externally confirmed: *"the next-Friday at-the-money straddle equals roughly 0.8 σ √T S"*,
and the recommended conversion is **× 1.25**. A row labelled EM implies ~68% containment and delivers
~58%. Multiplying by 1.25 also widens the band to 2σ√T, at which point E[range]/band = **0.80** — the
average day uses 80% of it and the extremes start meaning something again.

---

## FLAW 3 — THERE IS NO TIME AXIS, AND THE MOVE IS A √T QUANTITY

This is the cheapest large win available and it is currently absent entirely. Expected displacement grows
with **√T**, not with the clock:

| time | % of session elapsed | % of expected displacement "due" |
|---|---|---|
| 10:00 | 8% | **28%** |
| 11:00 | 23% | **48%** |
| 12:45 | 50% | **71%** |
| 14:30 | 77% | **88%** |

**40% used at 10:00 is a stretched market. 40% used at 14:30 is a dead one.** The panel reports both as
`40% OF EM` and means opposite things by it. A `% of EM ÷ √(elapsed)` pace ratio turns the same number
into a real overextension read, and it needs no new data.

---

## FLAW 4 — THE TARGET IS INSIDE ITS OWN PATH CALCULATION

Target = Mag = heaviest strike = also the largest gamma pile, by construction. So "where are we going" and
"what is the biggest thing on the way" are the same object, and the verdict is close to a constant.
Measured live 2026-08-23: of `$131M fuel`, **$107M was the pile sitting exactly ON the target and only
$23M was genuinely between price and it.** 82% of the reading is the destination.

---

## FLAW 5 — PRECISION WITHOUT ACCURACY

GEX is *"a model output, not an exchange-published statistic"*; public OI *"does not identify dealer versus
customer ownership"*, so the sign is *"still an assumption"*, and providers legitimately disagree — this
project has already measured ~113× between Skylit's book and InsiderFinance's. The face prints `$107M/pt`
to three significant figures on top of that assumption. The hovers say so, which is the right mitigation,
but the visual weight still says "measured".

And the related one already logged: `perPt` is computed from **gross** gamma while accelerator/brake is
decided by **net** sign, so strike 7650 claims $59M/pt and a direction off a 1.2% residual.

---

## WHAT THE FRAME IS MISSING

**A range statistic beside the displacement statistic.** The established tool for "how much of the day is
used" is Average Day Range — and it measures **high − low**, which is the right quantity for a path
budget. The expected move is the right quantity for a *target*. Two bars, two questions:

    RANGE  budget   (ADR / Parkinson)   ->  how much travel is left in the day
    MOVE   target   (straddle × 1.25)   ->  where the day is priced to END

Right now one bar is being asked both questions and can only answer the second.

**Skew.** The band is symmetric; the distribution is not. Not urgent, but it is an unstated assumption.

---

## THE FIX LIST, IN ORDER OF VALUE PER UNIT OF WORK

1. **Time-of-day pace ratio** — `pct ÷ √(elapsed fraction)`. No new data. Turns the headline number from
   ambiguous into meaningful. Biggest win available.
2. **× 1.25, or rename the row STRADDLE.** One line. Ends a mislabel.
3. **Exclude the target from its own path sum** (`pathStrictly` already computes it).
4. **Net, not gross, for the pile dollars**; render low-netFrac strikes as BALANCED.
5. **A range budget alongside the move target** — the only item here that is genuinely new work.
6. **Measure before tuning anything else.** `nodeThresh` at 20% is hand-set; the scorecards are empty.

Items 1–4 are small and would move the calibration score from 4 to about 8.

---

## SOURCES

- MenthorQ, *From Straddle Price to Expected Move* — 0.8σ, ×1.25, spot anchoring
- MenthorQ, *Backtesting Results: 1D Move on the SPX Index* — 72.6% both-sides containment at the close,
  Nov 2019–Aug 2023
- SpotGamma, *Gamma Exposure (GEX)* — "acceleration point", cannot answer direction, sign is an assumption
- Options Trading IQ, *Probability of Touch* — touch ≈ 2 × delta
- LuxAlgo, *Average Day Range Indicator* — range-based budget, fractional levels as intraday zones
- MetricGate, *Parkinson Volatility Estimator* — range carries ~5× the information of close-to-close
- SharpeTwo, *Variance Risk Premium* — IV > RV ~79% of the time at 30 DTE

⚠ Not financial advice, and none of the above establishes that the approach is profitable — only whether
it is internally coherent and correctly calibrated. Profitability is a question for the recorder.
