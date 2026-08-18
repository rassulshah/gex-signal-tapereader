# learning/log/ — the nightly logbook

One file per session day: `YYYY-MM-DD.json`, written by the **NIGHTLY (light)** run
(`docs/LLM-NIGHTLY-BRIEF.md`, contract 1, step 4). Append-only — a day file is written once and
never rewritten. This directory is the corpus the **WEEKLY (heavy)** run reads.

## Why it exists

A session is ~67 bars on overlapping 10-bar forward windows: about **6.7 independent
observations**. Nothing about a weight can be concluded from that. So the nightly run does not
try — it writes down what happened in a compact, machine-readable form, and Saturday's run
aggregates a week of them into something that can actually clear a bar.

## Shape

```json
{
  "schema": "gex-log/v1",
  "date": "2026-08-18",
  "bars": 67,
  "effectiveN": 6.7,
  "regime": { "tag": "trend", "opex": false, "event": false },
  "baseline": { "up": 31, "dn": 69, "n": 67 },
  "features": {
    "dir.drift": {
      "n": 54, "hits": 31, "up": 40, "dn": 14, "mfe": 0.51, "mae": -0.38,
      "byRegime": { "trend": { "n": 54, "hits": 31 }, "chop": { "n": 0, "hits": 0 } }
    }
  },
  "contradictions": [ { "bar": 31, "claim": "...", "evidence": "..." } ],
  "notes": "one day = ~6.7 independent observations; no weight conclusions from a single day."
}
```

Field notes:

- `effectiveN` = `bars / forward-window`, not `bars`. Overlapping windows are not independent
  samples, and reporting `n = 67` when it is really ~7 is the single easiest way to fool the loop.
- `up` / `dn` are the factor's **vote-direction split**. Without it, a factor that voted one way
  all day on a one-way day reads as edge. On 2026-08-11 structure voted DOWN on 46 of 49 bars of a
  down day and "scored" ~94%.
- `byRegime` mirrors the `regime.tag` that now rides on every FEATURES record (v10.53 C), so a
  rule that works in trend and fails in chop cannot average out to "meh".
- `baseline` is what the tape did on its own: the share of bars that travelled ±`DIR_PTS` inside
  the forward window, up and down. Every rate in the file is meaningless without it.

## What must NOT be here

- No weight proposals, no `clearsBar`, no promotions. The nightly proposes nothing; the weekly
  proposes; the panel promotes.
- No live verdicts, price targets, or trade language.
- No entry for a day whose data never arrived. A missing day file is honest; a fabricated one is
  not recoverable, because forward-only data can never be back-filled.
