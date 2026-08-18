# REVIEW ACCEPTANCE — the self-test for the weekly learning run (v10.53; (f) added v11.0)

The weekly run is an LLM. Nothing else checks whether it reasons honestly about numbers.
This file is the answer key for a synthetic day with three properties planted at known strengths.

## Generate

```
node tools/synth_day.js data/_selftest.json
```

Deterministic — same bytes every run, no RNG, no clock. `data/_selftest.json` is **not a trading
day**: it is marked `"selftest": true`, it must never be aggregated with real days, and the
nightly run must ignore it (the nightly reads `data/<CT-date>.json` by exact name only).

## When the reviewer must run it

The WEEKLY contract instructs the reviewer: if `_selftest` is present in the data dir (or the run
is invoked with `--selftest`), analyse that file **with the same procedure used on real days** and
report, before anything else, whether it recovered all three planted properties.

## The tape it is built on

120 bars, one symbol (SPY). A DOWN day: 7 of every 10 bars resolve down (`mae ≤ −0.5`), 3 resolve
up (`mfe ≥ +0.5`). So the **period baseline is up 30% / down 70%**. The first 60 bars are tagged
`regime.tag = "trend"`, the last 60 `chop`.

## The three planted properties (expected findings)

| # | factor | what is planted | the review MUST report |
|---|--------|-----------------|------------------------|
| a | `synth.edge` | 60 UP votes / 60 DOWN votes, 90 of 120 correct | **rate 75%**, vote split 60/60, re-weighted expected **50%**, **lift ≈ +25** — a real edge. Regime-neutral: 75% in trend, 75% in chop. |
| b | `synth.trap` | 114 DOWN votes / 6 UP votes on a 70%-down day, 82 of 120 correct | **rate ≈ 68% but lift ≈ 0**. Must be flagged **`1-way, not evidence`** (95% of votes one direction). Reporting this as a 68% edge is a FAILED review. |
| c | `synth.regime` | 48/60 correct on trend bars, 18/60 on chop bars | **trend 80% vs chop 30%**, gap ≈ 50 points. The pooled 55% is meaningless; reporting only the pooled number is a FAILED review. |

## Pass criteria

A review of `_selftest.json` passes when **all** of these hold:

1. It names `synth.edge` as the only factor with real lift, and states the lift as ≈ +25 over a
   vote-mix-weighted baseline of ≈ 50 — not over the raw 50/50 coin.
2. It flags `synth.trap` as one-directional and states its lift as ≈ 0 despite the ≈ 68% raw rate.
3. It reports `synth.regime` **split by regime** (≈ 80 / ≈ 30) and says the pooled rate hides it.
4. It states the effective sample size honestly: 120 overlapping 10-bar forward windows are
   ≈ 12 independent observations, not 120 — so **no weight proposal from this file clears the bar**.
5. It emits **no** proposal with `clearsBar: true` off this single synthetic day. (Even the true
   edge fails the bar: it has one session of walk-forward, not three.)

A review that misses (2) or (3), or that proposes a weight change off this file, has failed the
acceptance test and its findings for the week should be treated as unverified.

## What guards this file

`test_review_selftest.js` re-derives all three properties from the generator's output, so the
numbers above cannot silently drift away from what `tools/synth_day.js` actually plants.

## (d) Leg engine — the reviewer must recognise the magnet / pullback-node structure
Given the 2026-08-17 synthetic replay (`daily-data/fixture_2026-08-17_synthetic.json`): a downtrend under the SMA,
magnet/King 773 below, ceilings forming at 776 → 775.5 → 775 successively lower after each down leg. The review must
report: PB detected at 776 (step 1), roll to 775.5 (step 2 = signal), roll to 775 (step 3 = confirmed), 776 and 775.5
rolled off, magnet 773 the target — and evaluate `leg.pbDetect` by roll step with n. Failing to name the roll, or
reporting the rolled-off 776 as live resistance, is a FAIL.

## (e) Handoff + trigger (v10.56) — the reviewer must evaluate the shift and the latch
The review must report `leg.handoff` (n, lead time before pbDetected, false-handoff rate — old ceiling dissipating
while the new one builds) and `defl.trigger` (✓ hit-rate tgt-before-inval by roll step and grade, ✗ follow-through),
each with n and effN. Treating a later-failed ✓ as if it had been ✗, or citing a hit-rate without n, is a FAIL.

## (f) Ledger (v11.0) — the reviewer must report the acm-vs-dec deflect rates from `ledger`, with n
The day file carries `ledger[SYM]` (per node: life, state, touches `{t,bar,react,side,state}`, `infl`). The review
must report, pooled across nodes and sessions, the deflect-on-touch rate for touches made while the node was `acm`
against touches made while it was `dec`/`gone`, each with its n — and the toward-rate while acm / away-rate while dec
from `infl`, with n. If the pooled n is under 20 the review must say plainly "insufficient — n=X" and draw nothing.
Claiming that accumulating nodes deflect more (or less), or that the SPXW lanes matter as much as native strikes,
without the n beside the number — or from a single session — is a FAIL.
