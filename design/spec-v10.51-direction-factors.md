# v10.51 — expand the Direction engine to multi-factor + record every factor for the optimizer

Base v10.50. Version → 10.51 (3 spots). Invariants: one render(), final `})();`, v10.js==current/ (md5),
no key renames, descriptive-only, keep all enrollment. New localStorage keys allowed: gpts_flrceilhist_v1.

## 1. NEW roll detectors (Flr/Ceil), mirror the existing kingRoll
- Track Flr and Ceil strike history per bar (like KINGHIST): `FCHIST = {SPY:{flr:[{t,k}],ceil:[{t,k}]}, QQQ:{...}}`,
  sampled in the snapshot cycle from nodeMapModel(sym).flr/.ceil, capped at HIST_MAX.
- `flrRoll(sym)` → +1 if the Flr strike is HIGHER than ~ROLL_LOOKBACK(=5) bars ago (rolling up = bullish),
  −1 if LOWER (floor giving way), 0 if flat. `ceilRoll(sym)` → −1 if Ceil strike LOWER than lookback ago
  (rolling down = bearish), +1 if HIGHER (ceiling lifting), 0 flat. kingRoll already exists (+1/−1/0).
- Doctrine, in the hovers: floor rolling up = bullish; ceiling rolling down = bearish (Skylit).

## 2. SMA-50 trend vote
- `trendVote(sym)` from trendVerdict(sym): its `up`/`dn` are counts of closes above/below the SMA-50.
  Return +1 when state is an up-trend (enough closes above + slope up), −1 for down, 0 otherwise. Reuse the
  existing state; do NOT re-implement the SMA. Expose the up/dn counts for the hover.

## 3. Direction engine = weighted multi-factor vote (directionGrade rewrite of section 1)
Replace `lean = driftD*2 + structD` with a factor table (each {key, vote∈{-1,0,1}, weight}):
```
FACTORS = [
  {key:'drift',    vote: drift.dir,        weight: 2},   // GEX/VEX VWAP lean
  {key:'trend',    vote: trendVote(sym),   weight: 2},   // SMA-50 closes above/below
  {key:'struct',   vote: structD,          weight: 1},   // net mass above/below
  {key:'kingRoll', vote: kingRoll(sym),    weight: 1},   // King migration
  {key:'flrRoll',  vote: flrRoll(sym),     weight: 1},   // floor rolling up=bull
  {key:'ceilRoll', vote: ceilRoll(sym),    weight: 1},   // ceiling rolling down=bear
];
lean = Σ vote*weight;  dirNum = sign(lean);  out.dir = UP/DN/SIDE.
```
- WEIGHTS are hand-set (⚖) — read them from a single `DIR_WEIGHTS` object (const now) so the future
  optimizer can overwrite it from rules.json without touching logic. Add `dirWeightsSource:'hand-set'` to
  the grade output; when rules.json later supplies measured weights, flip to 'measured' + tier 📊.
- SCORE (grade): base on agreement-weighted magnitude — score = round(|lean|) capped, PLUS +1 when ≥3
  factors agree with dirNum (consensus), −1 when a high-weight factor (drift/trend) OPPOSES dirNum
  (conflict). A ≥5, B ≥3, else C. Keep the mid-range and chop HARD CAPS exactly as v10.50. Keep
  "SIDE can't be A" and session capOdds.
- `out.inputs.factors = [{key, vote, weight, contribution}]` for the hover + recording. Direction hover
  (question-first): "Which way, and how sure? Votes: drift +1, trend +1, struct 0, kingRoll 0, flrRoll +1,
  ceilRoll 0 → lean +6 → UP. Weights hand-set until measured."

## 4. RECORD every factor (feed the future optimizer) — FEATURES
- Ensure a recorded feature per direction factor with its vote + forward outcome (fwd=FEAT_FWD, DIR_PTS):
  `dir.drift`, `dir.trend`, `dir.struct`, `dir.kingRoll`, `dir.flrRoll`, `dir.ceilRoll`. The existing `roll`
  feature stays; ADD flrRoll/ceilRoll/trend/struct/drift as their own recorded features (record: the vote;
  outcome: did price move DIR_PTS in the vote direction over fwd bars → hit + mfe/mae). Also record the
  combined `dir` (already exists) with its lean + factors array.
- ALSO record (candidate, non-gating, not yet in the lean): `netGamma` (from deriveFactors reg posGamma/
  negGamma + net sign) as a directional-ish factor for later evaluation. Note in code: VWAP-position and
  options-skew NOT available yet (no price VWAP / skew feed) — leave TODO markers, do not fake.
- test_feature_enrollment must still pass (every new feature has all fields).

## 5. Analysis — factor scorecard (thin, sets up the optimizer)
- In analysisBlock, add a "Direction factors" table: per factor {key, n, hit-rate, lift vs 50%, avg MFE/MAE}
  sorted by lift. This is the read the optimizer will later consume. `● recording n=x/20` until unlocked.
  Header note: "which factors actually predict — weights stay hand-set until these mature."

## 6. Tests
- test_dir_factors.js: flrRoll/ceilRoll/kingRoll/trendVote signs; the weighted lean sum; consensus/conflict
  score; hard caps still apply; SIDE≠A. test_roll_detectors.js: floor up→+1, ceiling down→−1, flat→0.
  Update any direction test whose lean math changed (test_direction_grade). Run full suite; only the 5
  known-stale may fail; new tests all PASS. Version pins → 10.51 where asserted.
