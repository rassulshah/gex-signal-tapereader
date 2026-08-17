# v10.49 BUILD SPEC — mental-model dashboard (implements the approved mockup + layer spec)

Companions: `mockups/gex-v10.49-full-dashboard-mockup.html` (target UI),
`design/spec-v10.49-mental-model-layers.md` (layer contract), `design/spec-feature-enrollment.md`.
Base: v10.48 (`v10.js` == `current/`). Bump to **10.49** in the THREE spots (header @version ~L4,
part1 console.log ~L314, footer feed ~L7242). Keep ONE `render()`, final line `})();`, no key renames.
`v10.js` and `current/gex-signal-tapereader.user.js` must stay BYTE-IDENTICAL.

## A. AUTH FIX (blocking — v10.48 self-fetch 401s)
1. `var LASTAUTH = null;` next to LASTFEEDURL.
2. fetch hook: when url has `gex/levels`, capture the Authorization header from `init.headers`
   (Headers object → `.get('authorization')`; plain object → case-insensitive key) or from a Request
   `input.headers.get('authorization')`. XHR hook: wrap `setRequestHeader` to record `authorization`
   on the xhr, and in `send` when url matches, `LASTAUTH = xhr.__auth`.
3. `selfFetch`: `fetch(url, {credentials:'include', headers: LASTAUTH?{Authorization:LASTAUTH}:{}})`.
   If `res.status===401` → clear nothing, just return (the next real request refreshes LASTAUTH).
4. Footer: if `LASTAUTH` is null and self-fetch is needed, show `vex ⏳` (dim) instead of pretending.

## B. FEATURES REGISTRY + ENFORCEMENT (the enrollment mechanism)
`var FEATURES = [];` `function registerFeature(f){ FEATURES.push(f); }` Each: {key,label,phase,record(sym,ctx),outcome(rec,fwd),fwd,questions[],rule{id,tier:'hand',mechanism}}.
Consumers: (1) snapshot cycle → `snap.feat[key]=f.record(sym,ctx)`; (2) `resolveFeatureOutcomes(sym)` on
closed bar for pending recs older than fwd bars → writes {hit,mfe,mae}; (3) `analysisBlock()` iterates
FEATURES → scorecard per key; (4) `seedQuestions()` = FEATURES.flatMap(q); (5) `RULES` (rules.json)
seeded from FEATURES[].rule, loaded at boot via `rulesLoad()` (localStorage `gpts_rules_v1` +
optional raw-URL fetch of `learning/rules.json`, fail-soft).
Test `test_feature_enrollment.js`: every entry has all fields; keys ⊂ recorder export ∩ analysis ∩ rules.

## C. SPINE — direction grade, node grade, decision (computed once per render, cached per bar)
`function directionGrade(sym)` → {grade:'A'|'B'|'C', dir:'UP'|'DN'|'SIDE', score, inputs:{drift,structAsym,rangePos,regime}, tier}
- drift: from `driftRead(sym)` (below). conf agree = +2, split = 0, none = +1
- structAsym: accumulation/mass above vs below (reuse the S/R tilt logic ~L1855) → +2 aligned / 0 / −1 opposed
- rangePos: pos=(px−lo)/(hi−lo) from pickEdge Flr/Ceil; **mid (0.35–0.65) HARD-CAPS grade to C**
- regime: `regimeTag()` chop → HARD-CAP C and no odds; trend → +1
- A ≥5, B ≥3, else C. tier from RULES['dir.'+grade] (⚖ default).
`function nodeGrade(sym,L)` → {grade,score,inputs:{pol,tap,rocNow,rocDay,conf}}
- pol: +γ +1 (clean) / −γ 0 (sharp, flag) · tap: 0→+2,1→+1,2+→−1 · rocNow: Building +1 / Fading −1 ·
  rocDay: since-open growth ≥+15% +1 / ≤−15% −1 · conf: Q agree +1, V(drift) agree +1. A ≥5, B ≥3, else C.
`DECISION_MATRIX[dir][node]` 3×3 (labels from the mockup; NEVER entry/stop/size words):
 A/A take·follow-thru · A/B take·tight tgt · A/C wait fresher node · B/A bounce play · B/B scalp · B/C skip · C/A scalp only · C/B skip · C/C stand aside.
`function decisionCell(sym)` → {cell:'B×A', text}.

## D. DRIFT READ (v10.48-vetted math)
`function driftRead(sym)`: GVWAP/±σ from LASTFEED gamma nodes, VVWAP/±σ from LASTVEX; verdict AGREE-UP /
AGREE-DN when both centers same side of px AND bands overlap, else SPLIT; NONE if LASTVEX missing.
Render `driftLineHtml()` ONE line under kingHeaderBlock: `↗ Drift UP·conf · G773.9 V775.0` (hover: bands, px, overlap).

## E. DESCRIPTIVE TRADE FRAME (per in-play zone) — vocabulary locked: zone / inval / tgt / path
`function tradeFrame(sym,L,dir)` → {zone:[k−0.25,k+0.25], inval: next node beyond L against dir (or k∓0.5),
tgt: next node in dir capped at King, path:'air'|'wall'|'cluster' (from airpocket/cluster detectors)}.
Rendered on the in-play zone row 2 tail: `zone 773±.25 · inval <772 · tgt 776 (air)`; echoed in decision line.

## F. DEFLECTION-QUALITY ZONES (replaces the ladder body; keep column header row)
`deflZonesBlock()`: rows = in-play node (full: r1 identity+pol+tap+grade; r2 Acm day/now · Q/V · frame),
then top-N (N=3) other meaningful nodes (Flr/Ceil/Gate/Mag) one line: `◦ 776 Ceil 44% · +γ · 2nd · now▼  B`.
%King on rows from `feedStructMap` (feed), not DOM. Legend line (8px) as mockup. Section header `⚡ Deflection zones · px`.
Reaction quality (live, at the tap): `reactionQuality(sym,L)` → 'confirmed' (wick rejection + node not fading) |
'weak' | '—'; shows as a small chip on the in-play r1 when engaged (`⚡conf` / `⚡weak`).
Deflection anticipation: when px within 0.6 of a node with grade ≥B and approaching → r1 shows `▶ setup`.

## G. READ (two grades + decision) — `readBlock44` rewrite of the head + decision line
Head: `↑ UP B · Node 773 A− ⚖`. Body: Direction why (drift/struct/range/regime) + Node why (pol/tap/roc/conf).
Decision line: matrix text + `tgt 776 · inval <772`. Odds sentence ONLY from promoted 📊 rules. CHOP ⇒ SIDE/C.
Time awareness: `sessionBucket()` (open-drive <10:00 CT, morning, midday 11:30–13:30, afternoon, power 14:30+ CT;
OPEX flag 3rd Fri) → small badge in the header row; sessionBucket is an input to `dir` (power hour: cap odds) and
recorded on every feature rec.
Model-confidence meta: `modelHeat()` = rolling hit-rate of last 10 resolved dir/node grades; if <40% show
`model cold` badge (dim red) beside ⚖; if >60% `model warm`. Descriptive only.

## H. ACTION CAPTURE (TAKE / PASS)
Two tiny buttons on the in-play zone (`✓ take` `– pass`), one tap each; writes `{t, sym, k, cell, dirGrade,
nodeGrade, action}` into recorderDay(db).act[sym]. Recorded on the feature rec as `act`. No P&L.

## I. ACM CANONICAL (one source, labeled horizons)
`accumCanon(sym,k)` → {now:{pct,label}, day:{pct,label}} computed ONCE from LASTFEED history:
now = last ~6m (2 samples), day = vs first snapshot of the day (session store `ACMDAY[sym][k]={open:abs}` reset by day key).
Both the Node Map sentence and the zone rows read accumCanon → no Acm/Dec contradiction. Labels: `Acm▲/Dec▼/Steady`.

## J. PRE-OPEN BRIEF (descriptive)
Before 08:30 CT (or on demand `__gptsDebug.brief()`): one collapsible line under the header:
`Brief · King 773 · range 772–776 · regime chop? · OPEX no · yday: dir A 3/4 hit`. From current map + yesterday's day file summary.

## K. RECORDER / ANALYSIS / TESTING / RULES / LLM
- Register features: dir, drift, node, decision, acm, defl_ant, reaction, act (+ pass-through of rshuf/roll/gateHour
  when present). MFE/MAE in every outcome (`fwd=10` bars, `DIR_PTS=0.5`, `DRIFT_PTS=0.5`).
- `analysisBlock()`: prepend the FEATURES scorecards: rate% + n + MFE/MAE per key; `dir` and `node` also BY GRADE
  (A/B/C monotone check → flag if not); `decision` as the 3×3 with rate+n per cell; `act` = selection quality
  (took vs passed outcomes). `● recording n=x/20` under unlock.
- Seed questions per layer spec §3; miner factors += dirGrade,nodeGrade,decisionCell,driftVerdict,rocDay,tap,pol,rangePos,session.
- `RULES` seeded ⚖ per layer spec §4 incl. KILL LIST rules (`kill.tap3`, `kill.midrange`, `kill.noConf`, `kill.negGammaWide`);
  READ cites promoted only; tiers rendered ⚖/📊.
- LLM brief (nightly, proposals only) documented in `docs/LLM-NIGHTLY-BRIEF.md`: inputs (day file, rules.json, prior 3
  reviews, act log), asks (why/contradictions/calibration/kill-list/questions/thresholds), output `review/YYYY-MM-DD.json`.

## L. TESTS
New: test_feature_enrollment.js, test_direction_grade.js (A/B/C + midrange cap + chop cap), test_node_grade.js,
test_decision_matrix.js, test_drift_read.js (the live 773 numbers → UP·conf), test_trade_frame.js, test_accum_canon.js,
test_auth_capture.js (Headers/object/Request/XHR paths). Update test_read_v1047.js version pin → 10.49.
Full suite green except the 5 known-stale.
