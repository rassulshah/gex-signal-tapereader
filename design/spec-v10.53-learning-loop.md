# v10.53 — THE LEARNING LOOP: nightly logbook · weekly learning run · auto-promotion · challengers

Base v10.52. Version → 10.53 (3 spots). Invariants unchanged (one render(), `})();`, md5 parity, no key
renames, descriptive-only, enrollment intact). New keys allowed: gpts_rules_v2 (rules cache), gpts_promo_v1.

## User decisions (2026-08-18)
1. AUTO-APPLY, MARK IT: proposals that clear the promotion bar auto-apply at boot, flip ⚖→📊, and show a
   "promoted" marker. Fully hands-off.
2. SPARSE DATA: say so, keep hand-set. No provisional nudging. "insufficient — n=X, need 20" per rule.
3. CHALLENGER MODEL: parked factors are evaluated weekly against live ones; a challenger that beats an
   incumbent over the bar is proposed (then auto-applied) as a swap.

## Why the reshape
~6.7 independent observations/day → a nightly run has NO power to conclude anything about weights. One
process was mis-sized for the important job, and nothing closed the loop (findings landed in JSON, nobody
acted). Fix: split cadence, add regime tags, make proposals machine-applicable, let the panel promote.

## PART A — rules.json v2 (the mental model, machine-applicable)
`learning/rules.json`:
```
{ "schema":"gex-rules/v2", "asOf":"YYYY-MM-DD",
  "weights": { "dir": {"drift":2,"trend":2,...}, ... },          // LIVE weights the panel loads
  "rules": { "<id>": { "tier":"hand"|"measured", "rate":..,"n":..,"mfe":..,"mae":..,
                        "regime": {"trend":{rate,n},"chop":{rate,n},"opex":{rate,n}},
                        "mechanism":"..", "promotedOn":null|"date", "lastVerified":"date",
                        "walkForward": {"sessions":0,"held":false} } },
  "proposals": [ { "id":"..","kind":"weight"|"swap"|"kill"|"threshold", "target":"dir.weights.trend",
                   "current":2,"proposed":3,"n":..,"lift":..,"regime":"..","wf":{"sessions":3,"held":true},
                   "clearsBar":true|false,"reason":"..","madeOn":"date" } ],
  "promoted": [ { "id":"..","from":..,"to":..,"on":"date","evidence":{n,lift,wf} } ],
  "challengers": { "<challengerId>": { "vs":"<incumbentId>", "chalRate":..,"incRate":..,"n":..,"lift":.. } },
  "killList": [ { "id":"kill.tap3","condition":"tap>=3","rate":..,"n":..,"tier":".." } ] }
```
The panel already has rulesLoad(); extend it to read v2 (fallback to v1 shape), and to READ `weights.dir`
into DIR_WEIGHTS / the trend-primary scoring constants at boot. `dirWeightsSource` becomes 'measured' when
any dir weight came from a promoted proposal.

## PART B — the panel promotes (rulesApply at boot + on review read-back)
`applyProposals(rules)`: for each proposal with `clearsBar===true` and not yet in `promoted`:
- kind 'weight'  → set the target weight; record in `promoted`; flip that rule's tier to 'measured'.
- kind 'swap'    → challenger replaces incumbent in the voting set (e.g. dir.trend window 50→20); record.
- kind 'kill'    → add to killList (the READ/decision consult it: a matching condition caps the grade to C
                    and the decision line says why, descriptively).
- kind 'threshold' → e.g. grade cut points; apply; record.
PROMOTION BAR (hard, in code, not the LLM's call): n>=20 AND walkForward.held (>=3 NEW sessions after the
proposal was first made, rate still over the bar) AND not regime-flipping (rate does not invert between
trend and chop). The LLM computes and asserts `clearsBar`; the panel RE-CHECKS n and wf from the numbers in
the proposal before applying — the LLM cannot promote by fiat.
Promoted items render a small `📊 promoted <date>` marker wherever that weight/rule surfaces (READ grade
hover, Analysis). `__gptsDebug.promotions()` lists them. Persist applied state in gpts_promo_v1 so a boot
without network still uses the last-applied weights.

## PART C — regime tag on every outcome
Every FEATURES record already carries session bucket; ADD `regime` = {tag: trend|chop|na (from regimeTag),
opex:bool, event:bool (event tag if set)}. Outcomes are then aggregable per regime. The Analysis factor
table gains a regime split column set. (This is what stops a rule that works in trend and fails in chop from
averaging to "meh".)

## PART D — the two runs (rewrite docs/LLM-NIGHTLY-BRIEF.md into two contracts + update the gex skill REVIEW)
**NIGHTLY (light) — health + logbook**, after close:
1. Data arrived? (`data/<CT-date>.json` present, bars>0). If not: report + stop, no review.
2. Contradictions: any bar where READ verdict != direction spine? drift dir flipped a confirmed trend? grade
   A that hit <30% today? (descriptive, per-bar list).
3. Regime tag for the day (trend/chop/opex/event) + baseline drift + vote-split per factor for TODAY ONLY.
4. Append the day to `learning/log/YYYY-MM-DD.json` (compact: per-feature n/hits/mfe/mae/votes, regime).
5. Say plainly: "one day = ~N independent obs; no weight conclusions from a single day."
6. One-line brief for pre-open. NO weight proposals. NO rules.json edits.
**WEEKLY (heavy) — the learning run**, Saturday:
1. Load ALL day files + all `learning/log/*.json` + rules.json v2 + prior weekly reviews.
2. Per rule/factor: n, rate, MFE/MAE, vote-split, PER-REGIME breakdown, lift vs baseline (baseline re-
   weighted by that factor's own vote mix, so a 1-way factor cannot masquerade).
3. Calibration: A>B>C monotone per grade family; flag inversions.
4. Walk-forward: for every open proposal, re-check on the sessions since it was made; update wf.sessions/held.
5. CHALLENGERS: evaluate parked factors (dir.trendFast 10/20 vs trend5-50; dir.struct, dir.kingRoll, netGamma
   as additional/replacement voters; flr/ceil rolling once FCHIST has >=5 sessions) against incumbents on the
   SAME bars. Emit `challengers` and, where lift is real over the bar, a `swap` proposal.
6. Emit `proposals` (weight/swap/kill/threshold) with `clearsBar` computed against the hard bar. Sparse →
   `clearsBar:false, reason:"insufficient — n=X, need 20"`. NEVER nudge weights below the bar.
7. Write rules.json v2 (proposals/challengers/killList/rules updated; `weights` UNCHANGED by the LLM — only
   the panel's applyProposals moves weights, after re-checking the bar).
8. Deliver via the cascade (bridge → Drive → chat). Plain summary: what is now measured, what got promoted
   candidates, what is still unproven, next questions.
Both runs: descriptive only; the Academy is doctrine truth; never invent numbers.

## PART E — a test for the review itself (guards the LLM's rigor)
`tools/synth_day.py` (or .js) generates a synthetic day file with: (a) a PLANTED TRUE EDGE (e.g. factor X
votes UP and price then rises DIR_PTS 75% of the time, balanced up/down votes), (b) a PLANTED 1-WAY TRAP
(factor Y votes DOWN 95% of bars on a down day → high raw accuracy, ~zero re-weighted lift), (c) a regime
split where a factor works in trend and fails in chop. `docs/REVIEW-ACCEPTANCE.md` states the expected
findings. The weekly brief instructs the reviewer to run against the synthetic day when
`--selftest` is present in the data dir, and to report whether it recovered all three. test_review_selftest.js
asserts the synthetic generator produces the planted properties (so the acceptance file is truthful).

## PART F — Analysis tab additions
- "Promoted" strip: what was auto-applied, when, evidence.
- "Proposals" queue: open proposals with n / need / wf progress ("2 of 3 sessions held").
- "Challengers": incumbent vs challenger rates side by side.
- Factor table: + regime split columns; + `1-way` flag stays.

## Schedules (update the two scheduled tasks)
- Nightly LIGHT: keep 22:47 UTC Mon–Fri (17:47 CDT / 16:47 CST). Prompt → nightly contract.
- Weekly HEAVY: Saturday 15:00 UTC (10:00 CDT). Prompt → weekly contract + challengers + selftest note.
- Morning catch-up: keep, but it now only checks the NIGHTLY log exists (not a review).

## Tests
test_rules_v2.js (load v1+v2, weights read into DIR_WEIGHTS), test_promotion_bar.js (bar enforced in code:
n<20 never applies; wf not held never applies; regime-flip never applies; valid proposal applies once and
is idempotent; promoted marker set), test_regime_tag.js (every feature record carries regime),
test_review_selftest.js (synthetic day has the planted edge/trap/regime split), and version pins → 10.53.
Full suite: only the 4 known-stale may fail.
