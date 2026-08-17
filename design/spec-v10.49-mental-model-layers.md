# v10.49 — Tape-Reader Mental Model: layer-by-layer enrollment (2026-08-17)

Standing rule (user): every feature auto-enters DATA → ANALYSIS → TESTING → LEARNING → LLM and is
part of the mental model. This spec applies it to the whole v10.49 dashboard, and defines the
coherence spine so all pieces reason from the same facts.

## 0. THE SPINE (coherence)
One **Direction verdict** is computed once per bar (`directionGrade()`); everything else either
FEEDS it or RENDERS it. Nothing else opines on direction.
- Inputs (feeders): drift (GEX/VEX confluence), structure asymmetry (heavier side / accumulation
  above-vs-below), range position (mid-range HARD-CAPS grade to C — Skylit midpoint rule),
  regime (`regimeTag`; chop caps to C and drops odds).
- Renderers: READ head, DECISION line. Header pills / drift line show inputs, never a verdict.
One **Node grade** per meaningful node (`nodeGrade(k)`): polarity · tap-freshness · ROC(now +
since-open) · confluence(Q/V). Rendered on the zone rows; the in-play node's grade goes to the READ.
**DECISION** = matrix cell (Direction × in-play Node) — the only place an action word appears
(descriptive: "bounce play", "scalp", "stand aside"; NEVER entry/stop/size).
**Provisional flag:** every grade renders ⚖ (hand-set) until its measured record reaches n≥20 and
promotes → 📊. The panel must never assert an unearned probability.

## 1. DATA layer (L5 recorder) — `snap.feat.*` per 3m bar, forward-only, MFE/MAE on every outcome
FEATURES registry entries (each {key,label,record,outcome,fwd,questions,rule}):
- `dir`      record: {grade, verdict, drift, structAsym, rangePos, regime, tier:⚖|📊}
             outcome (fwd 10): hit = price moved ≥ DIR_PTS in verdict dir; mfe/mae; kingReached
- `drift`    record: {verdict, gvwap, vvwap, gLo,gHi,vLo,vHi, overlap, px}
             outcome (fwd 10): hit = drift direction realized ≥ DRIFT_PTS
- `node`     record per zone: {k, role, pol, tap, rocNow, rocDay, sinceOpenGrowth, confQ, confV, grade, inPlay}
             outcome (fwd 10): for in-play/tapped node → held? deflected? mfe/mae from tap; if broke → next-node reached
- `decision` record: {cell (e.g. 'B×A'), text}
             outcome: hit = the cell's expected behaviour held (bounce played out / trend followed) — the matrix's own scorecard
- `acm`      record per node: {rocNow, rocDay, label}; outcome: did rapid-accum nodes hold on tap more than fading ones
- `defl_ant` (deflection anticipation) record: {k, fired, gradeAtFire}; outcome: did the tap deflect
- Also unchanged Phase-B recorder items enrolled the same way: `rshuf`, `roll`, `gateHour`, `chartLevels`.
Export: all under `snap.feat` in `data/YYYY-MM-DD.json` (coverage summary lists which feat keys exist since when).

## 2. ANALYSIS layer (Phase C, thin slice now) — "did the dashboard tell the truth?"
`analysisBlock()` iterates FEATURES: one scorecard per key, dashboard order:
① Direction: grade → hit-rate by grade (A/B/C) + n + avg MFE/MAE. Expect A>B>C monotone; if not, the fusion is wrong (surface it).
② Drift: hit-rate when UP·conf / DN·conf / SPLIT + n.
③ Node/deflection: hit-rate by grade AND by each input (polarity, tap#, ROC state, confluence) — shows WHICH input carries the edge.
④ Decision matrix: 3×3 cells each with rate+n — the matrix earns or loses its cell labels.
⑤ Acm: since-open growers vs faders on tap (the real-vs-hedge test).
Every card: `● recording n=x/20` until unlocked; ⚖→📊 flip is shown here first.

## 3. TESTING layer (Phase D seeds now) — question queue + miner factors
Seed questions (from FEATURES[].questions):
- dir_A_follow: when dir grade=A → moves ≥DIR_PTS in dir? · dir_midrange_cap: mid-range verdicts fail more?
- drift_conf: UP·conf vs SPLIT realized drift · node_grade_hold: A vs C nodes hold on tap
- tap_decay: 1st/2nd/3rd tap hold rates (does 80/66/33 hold for SPY?)
- pol_char: +γ taps clean (low mae) vs −γ (high mae) · roc_day: since-open growers hold more
- matrix_cells: each Direction×Node cell's outcome vs its label
Miner factors += dirGrade, nodeGrade, decisionCell, driftVerdict, rocDay, tap, polarity, rangePos.
Lifecycle: proposed → testing → answered(📊,n) → refined (auto child questions adding one factor) | parked.
Promotion: n≥20 AND 3 nightly re-runs AND walk-forward hold on 3 NEW sessions.

## 4. LEARNING layer — `learning/rules.json` (the mental model, written nightly, read at boot)
Rule ids seeded ⚖: `dir`, `dir.A`, `dir.B`, `dir.C`, `drift.conf`, `node.grade.A/B/C`, `node.tap.1/2/3`,
`node.pol.pos/neg`, `node.rocDay.up/dn`, `decision.<cell>` (9), `acm.realVsHedge`.
Each: {condition, rate, n, mfe, mae, regime, mechanism (LLM note), tier ⚖|📊, promoted, lastVerified}.
Panel reads rules at boot: grade tiers ⚖/📊, READ odds sentence cites ONLY promoted rules, decision-cell
labels can be re-worded by evidence (e.g. a cell measured 40% gets "skip"). Decay: 3 runs under bar → demoted.
The grades themselves are re-tuned ONLY through this loop (never hand-edited to fit a day).

## 5. LLM layer (nightly review + question generation ONLY — never live)
Give the LLM, per day: the day file (feat records + outcomes), rules.json, prior 3 reviews. Ask for:
(a) worked/missed per feature with WHY (mechanism), (b) contradictions between grades and outcomes
(e.g. "A− nodes broke 3× — all −γ, refine polarity weight"), (c) new questions to queue, (d) threshold
PROPOSALS (never applied automatically), (e) missing fields to start tracking. Output → `review/YYYY-MM-DD.json`,
surfaced in Analysis ⑥. Forbidden: live verdicts, confidence, targets, trade language.

## 6. Enforcement
`test_feature_enrollment.js`: every verdict-/grade-producing block is in FEATURES with all fields; every
FEATURES key appears in the recorder export, the analysis iterator, and the rules.json seed. Suite fails otherwise.
