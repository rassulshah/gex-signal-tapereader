# lsKingTracker — the eyes-on checklist (Gate B, the part no test can see)

Use with the **expected picture** `tools/gp-regress.py --indicator kingtracker` prints (per book: the steps as
`[clock, strike, chart price]`, the live King, the ratio). Screenshot within a minute of `ASOF`; transcribe into
`irt.json` (`{"SPX":{"now": <chart price>, "steps": [...], "strike_label": 7685}, "SPY":{...}}`).

## Placement (scale)
- [ ] Every step sits at **strike × (KINGNOW price / KINGNOW strike) + offset** (the v0.6 re-derivation: no phantom step at Skylit's mid-session roll); offset = chart close at the SCALEREF minute − SCALEREF, ~0 inside RTH on the charted contract.
- [ ] The live King line (right edge) sits beside the gamma profile's magenta King bar — the two indicators read the same `KINGNOW`.
- [ ] The SPY line at strike × the SPY→ES ratio (≈ SPY strike × 10), labelled `SPY KING <strike>`.

## Steps
- [ ] The number of steps drawn = the runner's count (rolls + 1); the clocks read as the runner lists them.
- [ ] A challenger the sampler is still dwelling on is NOT a step (KTRK_CONFIRM_N samples); the runner warns when KINGNOW differs from the last step.
- [ ] No **STALE** badge.

## What to write down
Screenshot file names, every unticked line with what was seen instead, the operator's words.
