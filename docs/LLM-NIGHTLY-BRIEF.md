# LLM REVIEW CONTRACTS — NIGHTLY (light) and WEEKLY (heavy) · v11.0 LOCKDOWN

> **LOCKDOWN (v11.0, 2026-08-18).** No new features are being built. For the next ≥20 sessions the review's whole
> job is measurement and proposals: pool what the day files record, report it with n, propose (never apply), and
> say plainly what is still unproven. The stack the panel is organised around — and the order in which this brief
> asks you to review it — is master-spec §0: 0 FEED+TAPE · 1 NODE LEDGER · 2 STRUCTURE (Map, leg engine) ·
> 3 DIRECTION · 4 SETUP (trigger) · 5 OUTCOME+LEARNING · 6 REVIEW (you) · 7 VOICE. Contracts 1 and 2 below say
> WHEN and WHAT you write; the "BY LAYER" section after them says WHAT TO EVALUATE, layer by layer.

Two runs, two jobs, two output files. They are **not** interchangeable.

> **Why this is split.** A session produces ~67 bars on overlapping 10-bar forward windows —
> roughly **6.7 independent observations a day**. A nightly run has NO statistical power to
> conclude anything about a weight. The old single nightly process was mis-sized for the one job
> that mattered, and nothing closed the loop: findings landed in JSON and nobody acted on them.
> So: the nightly keeps a **logbook** and reports **health**; the weekly does the **learning**;
> and the panel — not the LLM — decides what gets promoted.

| | NIGHTLY (light) | WEEKLY (heavy) |
|---|---|---|
| when | after the close, Mon–Fri | Saturday |
| reads | today's day file + rules.json | ALL day files + all logs + rules.json + prior weeklies |
| writes | `learning/log/YYYY-MM-DD.json` | `learning/rules.json` (v2) + `review/YYYY-MM-DD.json` |
| may touch weights | **never** | **never** (only the panel's `applyProposals` moves a weight) |
| may emit proposals | **no** | yes, with `clearsBar` computed against the hard bar |

Both runs are **descriptive only**. The Academy (`skylit-docs/learn/`) is doctrine truth.
Never invent a number. If there is not enough data to conclude anything, say exactly that.

---

# CONTRACT 1 — NIGHTLY (light): health + logbook

Runs after 15:00 CT, Mon–Fri. Its whole job is: *did the machinery work today, did anything
contradict itself, and what happened — written down so Saturday can use it.* **No weight
proposals. No edits to `learning/rules.json`.**

### Inputs
1. **The day file** — `data/<CT-date>.json`: per closed 3-minute bar it carries `snaps[SYM][].feat.*`
   (one record per FEATURES key), the resolved outcome queue at `feat[SYM][]`
   (`{key, t, bar, rec, hit, mfe, mae, resolved}`), and the structural context (`nodes`, `deriv`,
   `ep`, `rg`, `xm`, `proj`, `out5`, `out10`), and from v11.0 the node ledger at `ledger[SYM]` (LAYER 1 below).
2. **`learning/rules.json`** — the current mental model (read only; the nightly never writes it).
3. **The prior 3 reviews** — the last three `learning/log/*.json` entries, plus the most recent
   `review/*.json`, so tonight is judged against what was already said.
4. **The act log** — `act[SYM][]`, the operator's `take` / `pass` labels with the decision cell and
   both grades at the moment of the tap.

### 1. Data arrived?
`data/<CT-date>.json` present and `snaps.SPY.length > 0`?
**If not: report that, and STOP.** Do not review, do not write a log file, do not speculate about
a day whose data never landed. Say which stage broke (not exported / not pushed / empty).

### 2. Contradictions (descriptive, per bar)
List the specific bars where the panel disagreed with itself:
- any bar where the READ verdict ≠ the direction spine (`feat.dir.verdict`);
- any bar where drift flipped a **confirmed** trend (`relation:"divergence"` on a `up`/`dn` state);
- any grade **A** that resolved under 30% today.
Name the bar, the values, and the mechanism you think caused it. A list without mechanisms is not
an answer. This is the one section that can be conclusive on one day, because it is about
self-consistency, not about probability.

### 3. Today's regime + vote split — **TODAY ONLY**
- the day's regime tag (`trend` / `chop` / `mixed` / `na`), whether it was OPEX, whether an event
  was tagged — read from `snaps[].feat.*.regime` (every record carries `{tag, opex, event}`);
- the baseline drift: share of bars that travelled ±`DIR_PTS` inside the forward window, up and down;
- per factor: the **vote-direction split** (UP votes vs DOWN votes) and today's raw rate.
Flag any factor whose votes were ≥90% one-directional as `1-way, not evidence`.

### 4. Append the logbook entry
Write `learning/log/YYYY-MM-DD.json` (compact — see `learning/log/README.md`):

```json
{ "schema":"gex-log/v1", "date":"YYYY-MM-DD", "bars":67, "effectiveN":6.7,
  "regime":{ "tag":"trend", "opex":false, "event":false },
  "baseline":{ "up":31, "dn":69, "n":67 },
  "features":{ "dir.drift":{ "n":54,"hits":31,"up":40,"dn":14,"mfe":0.51,"mae":-0.38,
                             "byRegime":{"trend":{"n":54,"hits":31},"chop":{"n":0,"hits":0}} } },
  "contradictions":[ { "bar":31, "claim":"...", "evidence":"..." } ],
  "notes":"..." }
```

Append-only: one file per session day, never rewritten. This is the corpus the weekly run reads.

### 5. Say the power out loud
Every nightly output must contain, in plain words:
> "one day = ~N independent observations; no weight conclusions can be drawn from a single day."

with N computed as `bars / forward-window` for this specific day.

### 6. One-line pre-open brief
A single sentence the panel can show before the open. Descriptive. No forecast, no level to trade.

### Forbidden in the nightly
Weight proposals · `clearsBar` · any edit to `learning/rules.json` · live verdicts · price targets ·
confidence numbers presented as live probabilities · trade language (entry / stop / size / buy /
sell / long / short).

---

# CONTRACT 2 — WEEKLY (heavy): the learning run

Runs Saturday. This is where learning happens.

### 1. Load everything
All `data/*.json` day files, **all** `learning/log/*.json`, `learning/rules.json` (v2), and the
prior weekly reviews. Never aggregate `data/_selftest.json` with real days (see step 8).

### 2. Per rule / factor
`n`, rate, MFE/MAE, vote-split, **per-regime breakdown** (trend / chop / opex), and **lift vs
baseline where the baseline is re-weighted by that factor's own vote mix**. A factor that voted
DOWN on 95% of a down day must come out at ~0 lift — that is the whole point of the re-weighting.
Report effective n (`bars / forward-window`), never raw bar counts, as the sample size.

### 3. Calibration
Is A > B > C monotone inside every grade family (direction, node, decision cells)? Flag every
inversion explicitly. A non-monotone ladder means the fusion is wrong, not that the day was odd.

### 4. Walk-forward
For every open proposal in `rules.json`, re-check it on the sessions that happened **since it was
made**, and update `wf.sessions` / `wf.held`. A proposal that stopped holding gets `held:false` —
that is a finding, not a failure to hide.

### 5. Challengers
Evaluate the parked factors against the incumbents **on the same bars**:
- `dir.trendFast` (10/20) vs `dir.trend5` (SMA-50);
- `dir.struct`, `dir.kingRoll`, `netGamma` as additional or replacement voters;
- floor/ceiling rolling **only once `FCHIST` has ≥5 sessions** (it is a day-over-day measurement).
Emit the `challengers` map. Where a challenger's lift is real **and over the bar**, emit a `swap`
proposal for it.

Also revisit the **kill list**: confirm, refine or retire the four seeded conditions (`kill.tap3`,
`kill.midrange`, `kill.noConf`, `kill.negGammaWide`) with the evidence behind each, and propose new
ones as `kind:"kill"` proposals. A kill entry caps a grade at C **only once it has been promoted
past the bar** — a hand-set kill entry is documentation, not behaviour.

### 6. Emit proposals
Kinds: `weight` · `swap` · `kill` · `threshold`. Each one states `target`, `current`, `proposed`,
`n`, `lift`, `regimeSplit`, `wf:{sessions,held}`, `reason`, `madeOn`, and `clearsBar` computed
against the hard bar:

> **n ≥ 20** AND **`walkForward.held` over ≥ 3 NEW sessions** since the proposal was made AND
> **no regime flip** (the rate does not invert between trend and chop).

Sparse data → `clearsBar:false, reason:"insufficient — n=X, need 20"`, and the hand-set value
stands. **NEVER nudge a weight below the bar. There is no provisional adjustment.**

`clearsBar` is your assertion, not your authority: `applyProposals()` in the panel re-derives n,
the walk-forward hold and the regime flip from the numbers inside the proposal before applying
anything. A proposal marked `clearsBar:true` with `n:12` is refused in code.

### 7. Write `learning/rules.json` (v2)
Update `rules` (rate/n/mfe/mae/regime/lastVerified/walkForward), `proposals`, `challengers`,
`killList`. **Leave `weights` EXACTLY as you found it** — the panel owns that block, and it only
changes it through `applyProposals()` after re-checking the bar. Also leave `promoted` alone; the
panel appends to it.

### 8. Self-test (acceptance)
If `_selftest` is present in the data dir (`data/_selftest.json`, generated by
`node tools/synth_day.js`), analyse it with the SAME procedure and report **first** whether you
recovered all three planted properties: the true edge, the 1-way trap, and the regime split.
`docs/REVIEW-ACCEPTANCE.md` is the answer key and the pass criteria. Never aggregate that file
with real days; never emit a proposal off it.

### 9. Deliver via the cascade
Stop at the first that succeeds, and always report **which** path was used:
1. **Device bridge** (`mcp__remote-devices__*`, needs the desktop app running) — write into
   `C:\Dev\gex-signal-tapereader\learning\` and `...\review\`; the local push task commits it.
2. **Google Drive** (connector) — create the file in `GEX-review-inbox`; the local task moves it
   into the repo. Drive is TRANSPORT ONLY; the repo stays the single source of truth.
3. **Chat** — SendUserFile the JSON.

### 10. Summarise plainly
5–10 lines: what is now **measured**, what got **promotion candidates**, what is still **unproven**,
and the next questions worth recording for. Name what you could not answer, and list the
**missing fields** — what is *not* being recorded that would have answered a question this week.
Forward-only data can never be back-filled, so that is the highest-leverage item in the whole run.

### Forbidden in the weekly
Editing `weights` · promoting anything by fiat · live verdicts · price targets · confidence numbers
presented as live probabilities · trade language · inventing a number when the sample is empty.

**Both contracts, one line:** the review looks at the past and proposes. It **never instructs** — no
entry, stop, size, or side, in any run, in any field, ever.

---

## Output file: `review/YYYY-MM-DD.json` (weekly)

```json
{
  "schema": "gex-review/v2",
  "date": "YYYY-MM-DD",
  "span": { "from": "YYYY-MM-DD", "to": "YYYY-MM-DD", "sessions": 5, "effectiveN": 33 },
  "grade": "B",
  "features": [
    { "key": "dir", "rate": 0.61, "n": 38, "mfe": 0.72, "mae": -0.41,
      "votes": { "up": 20, "dn": 18 }, "expected": 0.52, "lift": 0.09,
      "byRegime": { "trend": {"rate":0.68,"n":24}, "chop": {"rate":0.48,"n":14} },
      "byGrade": { "A": {"rate":0.71,"n":14}, "B": {"rate":0.58,"n":19}, "C": {"rate":0.40,"n":5} },
      "monotone": true, "oneSided": false, "why": "..." }
  ],
  "contradictions": [ { "claim": "...", "evidence": "...", "proposal": "..." } ],
  "calibration":    [ { "id": "decision.B×A", "label": "bounce play", "rate": 0.38, "n": 21, "verdict": "re-word to skip" } ],
  "challengers":    { "dir.trendFast": { "vs": "dir.trend5", "chalRate": 58, "incRate": 61, "n": 44, "lift": -3 } },
  "killList":       [ { "id": "kill.tap3", "keep": true, "rate": 31, "n": 26, "evidence": "..." } ],
  "questions":      [ { "id": "...", "when": [{"f":"pol","v":"-"}], "outcome": "nodeHold", "note": "..." } ],
  "missingFields":  [ "..." ],
  "selftest":       { "ran": true, "edgeFound": true, "trapFlagged": true, "regimeSplitFound": true },
  "headline":       "one line the panel can show pre-open"
}
```

The panel reads `review/<day>.json` and `learning/rules.json` back from raw GitHub on the same
fail-soft 10-minute cadence (`pipeCheck`), and surfaces both in the Analysis tab. Promotion is
executed by `applyProposals()` in `v10.js`, persisted to `gpts_promo_v1`, and listed by
`__gptsDebug.promotions()`.

---

# WHAT TO EVALUATE — BY LAYER OF THE STACK (both runs; every rule below still binds)

The evaluation is ordered the way the panel is built (master-spec §0). Every claim in every layer is reported
with n · effN · vote-split · regime split · MFE/MAE, and NEVER without n. Nothing below may be claimed from one
session; the nightly records, the weekly pools.

## LAYER 1 — NODE LEDGER (v11.0; the base layer, evaluate every run)
The day export now carries `ledger[SYM]` — every node the feed showed this session, SPY strikes AND the SPXW-derived
lanes (`src`), whose session peak reached PB_MIN_PCT. Per node: `k, src, first, peak, peakT, cur, state
(acm|dec|gone|hold), m15, fromPeak, gone, life{build,after,goneFor}` (minutes first→peak, peak→last seen, last
seen→now), the counts `deflect / through / stall`, the last 12 touches `{t,bar,react,side,state}` (`state` = the
node's ledger state AT the touch; reaction decided by the side the bar opened on and the DEFLECT_ZONE — see
master-spec §27), and `infl{acmN,acmToward,decN,decAway}` (5-bar windows: while acm did |price−node| shrink, while
dec/gone did it grow). Report, with n, POOLED across nodes and across sessions:
- **deflect-on-touch rate, acm vs dec/gone** — do accumulating nodes deflect more than dissipating ones? (touches
  where `state:'acm'` vs `state:'dec'|'gone'`, `react:'deflect'` share of each);
- **toward-rate while acm** (`acmToward/acmN`) and **away-rate while dec** (`decAway/decN`) — does accumulation
  pull price and dissipation release it?;
- **SPXW-derived lanes vs native strikes** — split both of the above by `src`: do the lanes influence SPY price as
  much as the integer strikes, or less?;
- **lead time** between a node's `peakT` and the bar where price first respected it (deflect) or abandoned it
  (through / moved away) — does the ledger see the change before the tape does?
- `ledger.touch` (FEATURES) — the in-play node touched, with the ledger state at the touch; outcome tgt-before-inval.
  Evaluate its question `acm_deflects_more` by state, by src, by grade, with n and effN. It is non-voting; say
  whether it has earned the acm/dec chip a job on the face or whether the chip is decoration.
Never claim any of this from one session: a session gives a handful of touches per node. If pooled n is under 20
say "insufficient — n=X" and stop there (REVIEW-ACCEPTANCE (f)).

## NOTES FOR EVERY LAYER (v11.0 changes to what you read)
- Merges: `drift` is now `dir.drift` and `roll` is now `dir.kingRoll` (one record each; older day files may still
  carry the old keys — pool them under the new name and say so).
- `dir` records now carry `read{voiceId,sentence,legDir,legPhase,dirSrc,map}` — the READ sentence as rendered on
  that bar, its voice id, the leg's dir/phase, where the direction came from (trend / map) and the Map line. So
  contradiction #1 (READ vs direction spine) is measurable PER BAR: compare `read.sentence`/`read.legDir` with
  `dir.verdict` and name the bars.
- The nightly log at `learning/log/<day>.json` is READ BACK by the panel (Analysis ⑥ REVIEW): the `headline` (or
  `brief`) line, `contradictions[]`, `factors[]` (or `features`) and `questions[]` are rendered. Write those fields
  — a log without them shows as an empty tile.
- Proposals no longer need the reviewer's n to be within 20% of the local n. The bar is: local eff n ≥ 20, three
  walk-forward sessions, no regime flip. `applyProposals()` still re-derives all three.

## LAYER 2 — STRUCTURE · LEG ENGINE — magnets, pullback nodes, rolling (evaluate every run; user-critical)
The trader's core model: a trend alternates MAGNETS (the node price rallies TO) and PULLBACK NODES (the node that
forms on the counter-move and price DEFLECTS off — the level to sell from in a downtrend / buy from in an uptrend).
PB nodes APPEAR AFTER the move and ROLL lower (dn) / higher (up) after each leg; 2 consecutive rolls = signal,
3 = confirmed. The 50-SMA confirms the trend; rolling ceilings ARE the successive pullback nodes.
Report, with n · effN · vote-split · regime split · MFE/MAE, and NEVER without n:
- `leg.pbPredict` — when the engine predicted "a PB node will form in this zone", did one form within fwd? (prediction accuracy)
- `leg.pbDetect` — when a PB node was detected, did price DEFLECT off it toward the magnet (tgt-before-inval)? by roll
  step (1st / 2nd / 3rd+): does a confirmed roll deflect more reliably than a first PB?
- `leg.roll` — after a 2nd/3rd consecutive roll, did the trend continue DIR_PTS? Is 3-step confirmation actually
  more reliable than 2-step signal on THIS tape? (the doctrine claims so — measure it)
- `leg.magnet` — was the magnet reached? how often does price make it vs stall at an intermediate node?

## LAYER 2 — STRUCTURE · THE MAP
- THE MAP (v10.58, USER PRIORITY — "as nodes dissipate, other nodes accumulate and start influencing price").
  `map.transfer` = a dec/gone node with an acm neighbour on the same side (the ceiling/floor rolling; SPY strikes
  AND the SPXW-derived lanes, drop-out counted as dissipation); `map.lean` = both sides rolling the same way.
  Report: did price move the roll's way within fwd (n, effN, by side, by book), does the lean lead the SMA-50
  (smaAgrees split), false-transfer rate, and the widening cases (dec node between acm nodes both sides) — did
  the range actually widen? These are recorded, non-voting; say plainly whether they have earned a vote.

## LAYER 2 — STRUCTURE · HANDOFF
- `leg.handoff` (v10.56, USER PRIORITY) — when the engine flagged the old ceiling DISSIPATING while a lower node
  built (the handoff), did the `to` node become the PB within fwd, and did price deflect off it toward the magnet?
  Report the LEAD TIME (leadBars before pbDetected) and the false-handoff rate (flagged, never resolved). Mirror for floors.

## LAYER 3 — DIRECTION · DRIFT SHADOW
- DRIFT IS IN SHADOW MODE (v10.57): not shown, not voting. `dir.drift` and the SHADOW `dir.relation` (what drift
  WOULD have said) are still recorded every bar. Report whether the shadow relation lifts trend-only (lift, n,
  effN, both directions) — this is the evidence that decides whether drift returns to the face. Never claim it
  is proven from one session.

## LAYER 4 — SETUP · TRIGGER
- `defl.trigger` (v10.56) — the latched ✓/✗ IS the deflection hit-rate: after ✓↓/✓↑, tgt-before-inval + MFE/MAE, by
  roll step and by grade; after ✗, did the break follow through DIR_PTS? Never intrabar; latch cannot flip — a ✓ that
  later failed counts as a failed ✓, not a ✗.

## LAYER 7 — VOICE · contradictions and the weakening tell
- Cases where a PB formed AGAINST the trend (weakening flag) — did the trend then fail? (that is the early-reversal tell)
- Contradictions: PB detected while direction said SIDE/chop-capped — was the PB level still tradeable? (this
  happened live 2026-08-18 09:19 CT: dn structure, magnet 768 / PB 769, direction capped SIDE by mid-range+chop)
- Contradiction #1 per bar (v11.0): `dir.read.sentence` / `dir.read.legDir` vs `dir.verdict` — list the bars, the
  values, and the mechanism (voice led by leg while the spine was capped SIDE, structure-leads with no trend, etc.).

## LAYER 5 — OUTCOME + LEARNING · proposals
Propose (never apply): PB_MIN_PCT, the roll-step confirmation count, whether the leg structure should soften the
mid-range cap. Kill-list candidates: PB steps that never deflect (e.g. 1st PB in chop).
Also propose, if the ledger earns it: whether the acm/dec state should filter deflection grades, and whether the
SPXW lanes deserve the same weight as native strikes. Everything proposed goes through the bar; nothing is applied
by the review, and during LOCKDOWN nothing new is built off a proposal — it waits for ≥20 sessions.
