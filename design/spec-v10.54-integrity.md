# v10.54 — INTEGRITY RELEASE: close the fiat paths, make outcomes match claims, honest n, R:R, and a usable Analysis/Testing UI

Base v10.53. Version → 10.54 (3 spots). Invariants: one render(), `})();`, md5 parity, no key renames,
descriptive-only, enrollment intact. Source of the findings: the 2026-08-18 whole-system audit (25 findings).
Every fix below names its finding number. Where a green test asserted the WRONG behaviour, the test is
rewritten — never delete a test to make a fix pass without replacing it with the correct assertion.

## GROUP 1 — the learning loop can no longer be moved by fiat (audit 1, 4, 6, 8, 24)
1.1 `weights` in a fetched rules.json is INERT. `rulesApplyWeights` derives DIR_WEIGHTS ONLY from
    DIR_WEIGHTS_HAND + PROMO.applied (promoted proposals). Rewrite test_rules_v2 2f/2g accordingly.
1.2 `killActive` consults ONLY PROMO.kills (proposals that cleared the bar). A killList entry with
    promoted:true / tier:'measured' in the fetched doc is documentation, never behaviour.
1.3 `ruleTier` returns 📊 only when the rule is in PROMO (applied) AND its locally-derived effN >= RULE_UNLOCK_N.
    `decisionCell` re-wording ('skip (measured X%)') requires the same.
1.4 `proposalClearsBar` RE-DERIVES from local truth: n_local = effective n from featStats for the proposal's
    feature key; wf_local = count of distinct recorded session-days strictly AFTER p.madeOn on which the rate
    stayed over the bar. Refuse if |n_local − p.n| > 20% or wf_local < PROMO_WF_SESSIONS. The proposal's own
    numbers are advisory only.
1.5 `swap` for the trend window sets TREND_DOM = round(0.75*TREND_WINDOW) with it; the SMA-period swap
    targets CFG.trendMA. Persist swaps in gpts_promo_v1. Add a DEMOTION rule: a promoted item whose locally
    measured rate falls under the bar on 3 consecutive weekly checks is demoted (weight reverts to hand,
    tier → hand, event logged).
1.6 Stamp every feature record with `model:{rulesAsOf, weightsHash}`; run rulesApply at boot and after
    the close only (never mid-session).

## GROUP 2 — doctrine + data integrity (audit 2, 5, 15, 16, 17)
2.1 ABSOLUTE VALUE at the boundary: futureStructureSummary assigns row.pct = Math.abs(...); polarity lives
    ONLY in row.pos. pickEdge / isStrongMag / nmStrength / inPlayZone / zone sort therefore see magnitude.
    Test: a −85% Barney node becomes Ceil/Flr/★Mag and is recorded.
2.2 Export flag set ONLY on success callbacks (repoExportDay / repoDownload). Add saveState code
    'pending' (amber) and 'failed' (red) when the day is marked but no pipeNoteSave record exists.
2.3 Persist TREND_LAST per day (with gpts_acmday_v1 style key gpts_trendlast_v1) and rehydrate on boot.
2.4 The AUTO exporter (repoExportDay) writes buildDayExport(date) — full file: snaps, feat (with
    hit/mfe/mae/resolved), act, flrCeilHist, rules asOf, questions. Bump export version string to VERSION_STR().
2.5 Feature-record cap: cap by BARS (keep the last 160 bars for every feature), not by record count.

## GROUP 3 — outcomes measure the claims; honest n (audit 3, 9, 10, 11, 12, 13, 14)
3.1 Add a SECOND outcome to node/decision/dir records: `frame:{tgtHit, invalHit, first:'tgt'|'inval'|null,
    rr}` = did price reach the recorded tgt before the recorded inval within fwd (using candle H/L). Keep
    the 0.5-pt drift hit as `drift`.
3.2 `_fwdStats` uses candle high/low (same as resolveFeatureOutcomes) so snaps[].out5/out10 agree with feat.
3.3 EFFECTIVE N everywhere: effN = round(n / FEAT_FWD). RULE_UNLOCK_N and PROMO_MIN_N compare against effN.
    Every displayed n reads "n=200 bars → eff 20".
3.4 1-way: oneSided = vn>=10 && max(up,dn)/vn >= 0.90. Numeric fixture test (46/49 must flag).
3.5 De-duplicate: `dir.relation` scores LIFT over trend-only on matched bars (not the same hit); `decision`
    scores frame.first==='tgt' for take-labelled cells and "no |move|>=DIR_PTS either way" for
    skip/stand-aside; SIDE records segregated from directional in byGrade + gradeMonotone.
3.6 Late-session: resolve pending records against the NEXT session's opening bars (cross-day) OR record the
    actual fwd window used and mark `partial:true`; power-hour bucket must be able to accumulate.
3.7 `act` recorded ONCE, on the tap bar.
3.8 Add dir.struct / dir.kingRoll / netGamma / dir.trendFast to dirFactorGroups so they get vote-split,
    baseline, lift, 1-way, regime columns.

## GROUP 4 — trader usefulness (audit 19, 20, 21, 7, 25)
4.1 R:R on the in-play row 3: rr = |tgt−k| / |k−inval|; render `R:R 2.4:1`; below 2:1 the decision text
    becomes "skip · R:R 0.7:1 (below the 3:1 floor)" descriptively; TAKE/PASS hidden below 2:1.
4.2 In-play band = DEFLECT_ZONE (0.50). When nothing is in contact, the card is labelled "watching — not in
    contact", NO frame, NO buttons. Buttons gated on the DECISION CELL (not node grade).
4.3 READ: tentative head derives from rangePosOf zone (near Flr / mid / near Ceil), not a fixed "Mid-range";
    'cont' only when dirNum === -holdDir.
4.4 Separate NODE_WEIGHTS.gradeA/gradeB from direction; hovers read the live constants (no hardcoded
    "A≥5, B≥3").
4.5 Delete dead narrators (readHeadHtml, readWhyHtml, decisionLineHtml, frameTextOf/_escHtml) — retire
    test_trade_frame's frameTextOf assertions in favour of tradeFrame + the live row-3 render. Fix
    gradeDisp so a direction score at exactly the A threshold shows "A", not "A−". Unify version strings.

## GROUP 5 — ANALYSIS + TESTING TABS: user-friendly and effective (new)
Principle: the tabs answer a trader's questions in order, in plain language, with honest n on every number.
5.1 ANALYSIS tab layout (top → bottom):
    ① HEADLINE — "Did the dashboard tell the truth today?" 3 tiles: Direction (rate · eff n), Node grades
      (A/B/C rates side by side + monotone ✓/✗), Decisions (take cells rate). Each tile shows "eff n=X" and
      greys out with "recording — need 20" under the bar. NEVER a % without its n.
    ② WHAT CHANGED — the Promoted strip (what auto-applied, when, evidence) + a red "MODEL CHANGED" banner
      on any day a promotion or demotion happened.
    ③ DIRECTION FACTORS — the full table (all voting AND parked factors): rate · eff n · votes ↑/↓ ·
      expected · lift · ⚠1-way · trend/chop split · MFE/MAE. Sortable by lift. Row hover explains the row
      question-first ("Does this factor predict? …").
    ④ DEFLECTIONS — by grade, by tap #, by polarity (+γ/−γ), by session bucket, each with rate · eff n ·
      MFE/MAE, plus the frame outcome (tgt-before-inval %).
    ⑤ YOUR CALLS — takes vs passes: rate and MFE/MAE for each, and the gap ("your passes beat the tool's
      takes by X pts" / "not enough taps yet").
    ⑥ NIGHTLY REVIEW — the read-back review's summary + brief line, with its date and which delivery path.
    ⑦ PIPELINE — the same 4 dots as the footer + rules asOf + last promotion, so the whole chain is visible
      in one place.
    Every section collapsible; empty sections show ONE honest line ("no data yet"), never blank.
5.2 TESTING tab layout:
    ① QUESTION QUEUE — proposed → testing → answered(📊, eff n) → refined | parked, each with the exact
      condition, current rate, eff n, and "need N more sessions".
    ② PROPOSALS — open proposals: kind · target · current → proposed · eff n / need · walk-forward "2 of 3
      held" · regime check · the panel's own bar verdict (why it does/doesn't clear). Plain language.
    ③ CHALLENGERS — incumbent vs challenger side by side (rate · eff n · lift) with a one-line reading
      ("SMA-20 is ahead by 6 pts on 14 eff obs — not yet decisive").
    ④ KILL LIST — each condition, its measured rate, tier, and whether it is ACTIVE (promoted) or hand-set.
    ⑤ SELF-TEST — a button that runs the synthetic day through the local scorer and shows pass/fail on the
      three planted properties (edge found / trap flagged / regime split), so the trader can see the
      scorer works.
    ⑥ DATA COVERAGE — days · bars · features · fields-since · effective observations, plus "what unlocks
      when" (e.g. "rolling floors/ceilings: needs 5 sessions of FCHIST, have 2").
5.3 Both tabs: consistent typography with the dashboard, question-first hovers everywhere, no jargon
    without a hover, mobile-width safe (250px panel), and a top-right "?" that opens a one-screen guide
    to reading the tab.

## Tests
Rewrite/replace: test_rules_v2 (weights inert), test_promotion_bar (local re-derivation; n=12 refused even
with clearsBar:true AND matching self-report; wf from local session count), test_regime_tag (numeric 1-way
fixture 46/49 flags; 30/49 does not), test_review_selftest, test_pipeline_indicator (pending/failed states),
test_zone_row (abs pct: −85 node is Ceil; R:R render; buttons hidden below 2:1; watching state), new
test_frame_outcome (tgt-before-inval scoring), test_effn (effN math + unlock uses effN), test_export_full
(auto export contains feat/act/flrCeilHist), test_act_once, test_analysis_tabs (sections render, no % without
n, empty-state lines). Version pins → 10.54. Full suite: only the 4 known-stale may fail.
