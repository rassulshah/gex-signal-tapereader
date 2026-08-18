# v10.51 — Direction engine: SMA-50 PRIMARY, drift CONFIRMS or DIVERGES

Base v10.50.1. Version → 10.51 (3 spots). Invariants: one render(), final `})();`, v10.js==current/ (md5),
no key renames, descriptive-only, all FEATURES enrollment kept working.

## The model (replaces the v10.50 weighted-sum lean)
HIERARCHY, not a blend:
1. **SMA-50 five-state IS the direction.** `trendVerdict(sym).state` → up | dn | up-broken | dn-broken | flat | na.
2. **Drift CONFIRMS or DIVERGES.** driftRead(sym).dir (+1/-1/0) never chooses the direction; it grades confidence.
3. **Tentative mode (user choice):** when there is NO confirmed trend (flat / up-broken / dn-broken / na),
   drift may supply a PROVISIONAL lean — but the grade can never exceed **C**.

### directionGrade(sym) rewrite
```
tv = trendVerdict(sym).state ; dr = driftRead(sym)          // dr.dir, dr.verdict
confirmedTrend = (tv==='up' || tv==='dn')
trendDir = tv==='up' ? +1 : tv==='dn' ? -1 : 0
```
**A. Confirmed trend (tv up|dn):** `dir = trendDir` (ALWAYS — drift never overrides direction).
   - agreement: `agree = (dr.dir!==0 && dr.dir===trendDir)`, `diverge = (dr.dir!==0 && dr.dir!==trendDir)`
   - base score 3 (a confirmed trend alone is a B)
   - `+2` when agree AND dr.verdict is an AGREE-* (bands overlap) ; `+1` when agree without overlap (LEAN-*)
   - `-2` when diverge  → a diverging trend cannot grade above C (hard rule, mirrors the caution doctrine)
   - `relation = 'confirmed' | 'divergence' | 'trend-only'` (trend-only when dr.dir===0 / NONE)
**B. No confirmed trend (flat | up-broken | dn-broken | na) — TENTATIVE:**
   - `dir = dr.dir` (drift supplies the provisional lean; SIDE when dr.dir===0)
   - score = 1 (i.e. grade C) — **cap grade at C, always**, `relation='tentative'`
   - broken states are recorded distinctly but vote 0 for now (we do NOT yet know whether a broken
     uptrend continues or reverses — the recorder will answer it; see §Record)
**C. Hard caps (unchanged from v10.50):** mid-range → C + noOdds ; chop → C + dir SIDE + noOdds ;
   'SIDE' cannot be A ; session capOdds (power / open-drive).
Grade from score: A ≥5, B ≥3, else C.
Output adds: `out.trendState`, `out.relation`, `out.tentative` (bool), `out.inputs.trend={state,up,dn,win,slope}`,
`out.inputs.drift={verdict,dir,gvwap,vvwap}`. Keep `dirWeightsSource:'hand-set'`.

### READ wording (uses relation)
- confirmed : `At King 773. Support building, uptrend confirmed by GEX and VEX leaning up. Potential bounce to 776.`
- divergence: `At King 773. Uptrend, but GEX and VEX lean down — divergence, lower confidence. Watch 772.`
- tentative : `Mid-range 772–776. No trend; GEX and VEX lean up — tentative only.`
- trend-only: keep the v10.50 3-beat sentence, no drift clause.
Direction hover (question-first): "Which way, and how sure? SMA-50 sets the trend (15/20 closes); GEX/VEX
drift confirms it or diverges from it. Trend UP + drift UP = confirmed. Trend UP + drift DOWN = divergence,
caution. No trend = drift gives a tentative lean, never better than C."

## Record (feeds the future optimizer — nothing else changes behaviour)
Register/extend FEATURES so each is scored independently on forward outcome (fwd=FEAT_FWD, DIR_PTS):
- `dir.trend5` — record the FULL five-state value (NOT collapsed ±1) so up-broken / dn-broken get their own
  measured hit-rates. outcome: did price move DIR_PTS in the state's implied direction (up/up-broken→+1,
  dn/dn-broken→−1, flat→skip/null).
- `dir.drift` — vote + verdict (AGREE/LEAN/SPLIT/NONE).
- `dir.relation` — 'confirmed' | 'divergence' | 'tentative' | 'trend-only' + resulting dir; outcome measures
  whether the RELATION predicted better than trend alone (this is the whole point of the hierarchy).
- Keep recording (NOT voting): `dir.struct`, `dir.kingRoll`, `netGamma`, plus **faster SMA windows**
  `dir.trendFast` (10 and 20 period, same 15/20-style dominance) so we can later compare 10/20/50 empirically.
- Start FCHIST: sample nodeMapModel(sym).flr/.ceil strike per bar into a capped history (key
  gpts_flrceilhist_v1) so multi-session rolling becomes computable later. Do NOT compute/vote rolling yet
  (Academy: rolling is measured across map updates day-over-day; 2 consecutive = signal, 3 = confirmation).
All new features need {key,label,record,outcome,fwd,questions,rule} — test_feature_enrollment must pass.

## Analysis
"Direction factors" table gains: per five-state hit-rate (up / dn / up-broken / dn-broken), drift by verdict,
and a RELATION row set (confirmed vs divergence vs tentative vs trend-only) with n + hit-rate + avg MFE/MAE.
**Every row must also show the vote-direction split (UP votes / DOWN votes) and the period's baseline drift**,
so a one-directional factor on a trending day can never masquerade as edge (this artifact was caught on
2026-08-11 where structure voted DOWN 46/49 on a down day).

## Tests
test_dir_hierarchy.js: confirmed trend + agreeing drift → higher grade; confirmed trend + opposing drift →
grade C (divergence rule); flat trend + drift → dir from drift AND grade capped at C (tentative); broken
states → tentative; drift NEVER flips the direction of a confirmed trend; mid-range/chop caps still apply.
Update test_direction_grade.js to the new model. Full suite: only the 5 known-stale may fail.
