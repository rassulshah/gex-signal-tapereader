# LLM NIGHTLY BRIEF — the review loop (v10.49)

The LLM runs **once, after the close, on recorded data only**. It never sees live tape and never
produces a live verdict. Its whole job is to explain what happened, find contradictions, and
**propose** changes that a human applies.

## When
After 15:00 CT, once `data/YYYY-MM-DD.json` has been exported and committed.

## Inputs (all four, every night)
1. **The day file** — `data/YYYY-MM-DD.json`. Per closed 3-minute bar it carries
   `snaps[SYM][].feat.*` (one record per FEATURES key: `dir`, `drift`, `node`, `decision`, `acm`,
   `defl_ant`, `reaction`, `act`, `rshuf`, `roll`, `gateHour`), plus the resolved outcome queue at
   `feat[SYM][]` (`{key, t, bar, rec, hit, mfe, mae, resolved}`) and the structural context already
   recorded since v10.44 (`nodes`, `deriv`, `ep`, `rg`, `xm`, `proj`, `out5`, `out10`).
2. **`learning/rules.json`** — the current mental model: every rule with `{condition, rate, n, mfe,
   mae, regime, mechanism, tier, promoted, lastVerified}`.
3. **The prior 3 reviews** — `review/YYYY-MM-DD.json` for the last three sessions, so proposals are
   judged against what was already proposed and whether it held.
4. **The act log** — `act[SYM][]`, the operator's `take` / `pass` labels with the decision cell and
   both grades at the moment of the tap.

## Asks (in this order)
1. **Worked / missed per feature, with the WHY.** For every FEATURES key: what its measured rate was
   today, and the *mechanism* — the market reason it worked or failed. A rate without a mechanism is
   not an answer.
2. **Contradictions between grades and outcomes.** Name them explicitly, e.g. *"A− nodes broke 3×,
   all three were −γ → the polarity weight is too low."* Flag any non-monotone grade ladder
   (A must beat B must beat C; if it does not, the fusion is wrong).
3. **Calibration.** Is a grade claiming more than it delivers? Compare each grade's hit-rate to its
   implied confidence, and compare each decision cell to its label.
4. **Kill list.** Which conditions should void a read outright. Confirm, refine or retire the four
   seeded ones (`kill.tap3`, `kill.midrange`, `kill.noConf`, `kill.negGammaWide`) and propose new ones
   with the evidence that motivates them.
5. **New questions to queue.** Concrete, testable, one factor at a time, in the same shape as
   `FEATURES[].questions` so they can be dropped straight into the miner.
6. **Threshold PROPOSALS.** Never applied automatically. Each must state: the constant, the current
   value, the proposed value, the sample size behind it, and what would falsify it.
7. **Missing fields.** What is *not* being recorded that would have answered a question tonight.
   Forward-only data can never be back-filled, so this is the highest-leverage item.

## Output
`review/YYYY-MM-DD.json`:

```json
{
  "schema": "gex-review/v1",
  "date": "YYYY-MM-DD",
  "grade": "B",
  "features": [
    { "key": "dir", "rate": 0.61, "n": 38, "mfe": 0.72, "mae": -0.41,
      "byGrade": { "A": {"rate":0.71,"n":14}, "B": {"rate":0.58,"n":19}, "C": {"rate":0.40,"n":5} },
      "monotone": true, "why": "..." }
  ],
  "contradictions": [ { "claim": "...", "evidence": "...", "proposal": "..." } ],
  "calibration":   [ { "id": "decision.B×A", "label": "bounce play", "rate": 0.38, "n": 21, "verdict": "re-word to skip" } ],
  "killList":      [ { "id": "kill.tap3", "keep": true, "evidence": "..." } ],
  "questions":     [ { "id": "...", "when": [{"f":"pol","v":"-"}], "outcome": "nodeHold", "note": "..." } ],
  "thresholdProposals": [ { "const": "FLRCEIL_EDGE_PCT", "current": 40, "proposed": 33, "n": 120, "falsifiedBy": "..." } ],
  "missingFields": [ "..." ],
  "rulesPatch":    { "dir.A": { "rate": 0.71, "n": 34, "mfe": 0.8, "mae": -0.35, "tier": "measured", "promoted": true, "lastVerified": "YYYY-MM-DD" } }
}
```

`rulesPatch` is merged into `learning/rules.json` by the nightly job. The panel picks it up at the
next boot via `rulesLoad()` (localStorage `gpts_rules_v1` first, then a fail-soft fetch of the raw
`learning/rules.json`). Promotion requires **n ≥ 20 AND 3 nightly re-runs AND a walk-forward hold on
3 NEW sessions**; three runs under the bar demote a rule back to ⚖.

The review is surfaced in the Analysis tab (section ⑥) via `__gptsDebug.loadReview(obj)`.

## Forbidden
The LLM may not emit: live verdicts, confidence numbers presented as live probabilities, price
targets, or trade language (entry / stop / size / buy / sell / long / short). It reviews the past and
proposes; it never instructs.
