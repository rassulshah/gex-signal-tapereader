# Feature Enrollment Contract + Registry (standing architecture, 2026-08-17)

## Standing rule (user mandate)
Every feature — INCLUDING every new feature, enhancement, or update — must AUTOMATICALLY
enter the improvement layers and become part of the tape-reader mental model. No feature is
ever shipped and forgotten. Each one is continuously under scrutiny for improvement.

A feature is not "done" until it is enrolled in ALL THREE layers:
1. **DATA (L5 recorder)** — its live context + output is recorded per bar into the day file
   (`data/YYYY-MM-DD.json`), forward-only.
2. **ANALYSIS (Phase C / L6)** — an Analysis-tab scorecard: "did this feature tell the truth?"
   with a measured hit-rate and sample size n, in dashboard order.
3. **TESTING + LEARNING (Phase D / L9)** — a hypothesis/question in the testing queue AND a rule
   in `learning/rules.json` {condition, rate, n, MFE/MAE, regime, mechanism note}, scored nightly,
   graduating at n≥20 + walk-forward, decaying (demoted) when it underperforms.

## The mechanism: one Feature Registry, three consumers
Instead of wiring each feature into three places by hand (which rots), features SELF-DECLARE once
in a single registry, and the three layers ITERATE the registry. Adding a feature = one entry =
auto-enrolled everywhere.

`var FEATURES = [ featureDecl, ... ];` where each `featureDecl` is:
```
{
  key:        'drift',                       // stable id (also the rules.json rule id root)
  label:      'GEX/VEX Drift',               // human label for Analysis
  phase:      'dashboard',                    // ordering bucket for the Analysis tab
  // DATA: pure fn -> the snapshot object recorded each bar under snap.feat[key]
  record:     function(sym, ctx){ return { verdict, gvwap, vvwap, gLo,gHi,vLo,vHi, overlap, px }; },
  // OUTCOME: given the recorded snap + forward bars, did it resolve right? -> {hit:bool|null, mfe, mae}
  //          null = not yet resolvable (keep pending). Forward window per feature.
  outcome:    function(rec, fwdBars){ ... },  // e.g. drift UP hit if px drifts up >= DRIFT_PTS in N bars
  fwd:        10,                             // forward bars to resolve
  // TESTING: the question(s) this feature raises for the miner/queue
  questions:  [ { id:'drift_conf_up', when:[{f:'driftVerdict',v:'UP·conf'}], outcome:'driftUp' }, ... ],
  // LEARNING: default rule seed (hand-set ⚖ until measured 📊)
  rule:       { id:'drift', tier:'hand', mechanism:'GEX range + VEX drift agree => higher-prob lean' }
}
```

### Consumer 1 — recorder (L5)
In the per-bar snapshot cycle: `FEATURES.forEach(f => snap.feat[f.key] = f.record(sym, ctx));`
Then, on each closed bar, resolve pending outcomes: for each recorded feature instance older than
`f.fwd` bars, call `f.outcome(rec, fwd)` and write `{hit, mfe, mae}` back. All forward-only.

### Consumer 2 — Analysis tab (Phase C)
`FEATURES.forEach(f => analysisBlock.addScorecard(f))` — reads the recorded hits for `f.key`,
shows rate% + n + MFE/MAE, sorted by `f.phase`. A feature with n<unlock shows "● recording n=x/N".

### Consumer 3 — Testing pipeline + rules.json (Phase D)
`FEATURES.flatMap(f => f.questions)` seeds the question library/miner. `learning/rules.json` is keyed
by `f.rule.id`; the nightly review writes measured {rate,n,mfe,mae,regime} back; the panel reads the
rule to tag the feature ⚖ hand-set vs 📊 measured, and to decay it.

## Enforcement (so it can't be skipped)
- A dev-time assertion (in `test_feature_enrollment.js`): every FEATURES entry has all of
  {key,label,record,outcome,fwd,questions,rule}; every rendered feature block whose output is a
  verdict/lean/level appears in FEATURES. CI-style: the suite fails if a new verdict-producing block
  is added without a registry entry.
- SKILL.md + master-spec carry the rule: "a feature is not done until it is in FEATURES and the three
  layers consume it." Applies to every `save`/deploy.

## Applies to v10.49 immediately
- `drift` (GEX/VEX drift read) ships AS a FEATURES entry — recorded, scored, questioned, ruled.
- The Phase-B recorder items (reshuffle, rolling Flr/Ceil, gate-hour, chart levels, MFE/MAE) each
  ship AS FEATURES entries too. MFE/MAE becomes the shared outcome-magnitude field on every entry.
- This pulls the FIRST slices of Phase C (a minimal scorecard iterator) and Phase D (question seeds +
  rules.json read) forward, because enrollment requires them. Full C/D remain later builds, but the
  registry + its three thin consumers land now so nothing ships un-enrolled.
