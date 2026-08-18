# GEX SIGNAL TAPEREADER — RESTART-SAFE MASTER SPEC

## Consolidated Handoff for Any LLM
**Version target:** v9.1 live-capture hardening  
**Prepared:** 2026-08-09  
**Purpose:** this document is meant to let any LLM resume the project safely, accurately, and without inventing missing code or assumptions.

## 1. Project intent
This project is a descriptive market-structure reader for Skylit Atlas. It reads GEX-related structure, accumulation, regime context, and setup lifecycle, then presents that information in a structural/observational panel.

It is not a trading bot, signal service, or execution assistant.

The tool may describe:
- node structure
- King / Gatekeeper / polarity context
- accumulation / dissipation
- trend and path condition
- target ladder / lifecycle state
- calibrated confidence once base rates exist

The tool must never provide:
- entries
- stops
- position sizing
- R:R
- P&L framing
- trade recommendations

The system is measure-first and descriptive-only.

## 2. Non-negotiable operating rules
These rules override everything else.

### 2.1 Code-source rule
Never reconstruct the userscript from memory, summaries, or this spec.  
This document describes behavior and architecture, not authoritative code.

First action on any resume: the user must paste the current Tampermonkey source.

No coding, editing, or replacement script generation should occur until the real source is in hand.

### 2.2 Paste protocol
The assistant cannot access the Tampermonkey editor directly.  
Workflow is always:

user pastes source → assistant edits/replaces → user saves → user hard-reloads

### 2.3 Delivery format rule
When producing code, provide either:
- one complete replacement block, or
- clean sequential parts

Never split one logical section across multiple disconnected blocks such as “main script + swap this function in.” That previously caused truncation and parse failures.

### 2.4 File-shape rule
The userscript must contain:
- exactly one render()
- exactly one closing `})();`
- the final line must be `})();`

### 2.5 Verification rule
Do not trust console boot logs alone.  
Every reload must be verified by DOM/probe evidence.

Preferred checks:
- panel visible
- expected version/footer text visible
- expected feature labels visible
- `window.__gptsDebug` present
- feed hook present
- probe returns hydrated data

### 2.6 Function-risk rule
Before asking the user to paste a risky logic change, smoke-test pure logic against persisted data where possible.

### 2.7 Scope rule
The panel remains observational/structural only.  
No drift into recommendation logic.

## 3. Source-of-truth hierarchy
Any future LLM should use this precedence order.

### Tier 1 — authoritative
1. The actual current userscript source pasted by the user
2. Live probes from the running page
3. Persisted localStorage state and recorder outputs

### Tier 2 — operational guidance
4. This master spec
5. Previous handoff notes
6. Teaching/specification docs
7. Heatseeker manual concepts

### Tier 3 — non-authoritative
8. Model memory
9. Model priors about markets, GEX, time of day, or microstructure

If a conflict exists, Tier 1 wins.

## 3.1 Persistent project storage and workflow
The persistent project now lives in AI Drive under `/GEX-Signal-Tapereader/`.

Canonical files:
- `master-spec.md`
- `teaching-spec.md`
- `current/gex-signal-tapereader.user.js`
- `session-state/latest-resume-note.md`
- `changelog/CHANGELOG.md`
- `design/architecture-design.md`

Supporting files/folders:
- `developer-kickoff.md`
- `workflow.md`
- `releases/`
- `probes/`
- `design/`

Operating rule: AI Drive is the source of project continuity; Tampermonkey is the deployment/runtime target. Future LLM sessions should load the canonical AI Drive files first, make edits against `current/gex-signal-tapereader.user.js`, save back into AI Drive, and only then prepare a deployable script for Tampermonkey.

### 3.1.1 Load-command completion rule
For this project, shorthand commands such as `load gex`, `open gex`, `continue gex`, `retrieve gex`, and `get gex` are not advisory; they are blocking bootstrap commands. This applies in all Genspark surfaces, including the Chrome extension.

A load command is complete only when the assistant has actually read all canonical files AND platform docs required for startup:

**Project files (7):**
- `master-spec.md`
- `teaching-spec.md`
- `current/gex-signal-tapereader.user.js` (FULL FILE — understand code architecture: Layer 0-7, render flow, key algorithms)
- `session-state/latest-resume-note.md`
- `changelog/CHANGELOG.md`
- `design/architecture-design.md`
- `skylit-docs/README.md` (doc archive overview)

**Skylit platform foundation (minimum 3):**
- `skylit-docs/core-concepts.md` (nodes, King, Gatekeeper, retest decay)
- `skylit-docs/read-the-heatmap/how-to-read-and-use-heatseeker.md` (5-step framework)
- `skylit-docs/learn/intro-to-gamma.md` (Pika/Barney, absolute value rule)

**Additional platform docs (recommended):**
- `skylit-docs/learn/node-lifecycle.md` (Fresh/Tested/Delivered/Decaying)
- `SOURCE-OF-TRUTH.md` (Skylit Academy as source of truth)

Until all required files are read, the assistant must not claim the project is loaded, must not continue with project reasoning or coding, and must not imply sync. After a successful load it must explicitly report:
1. which project + platform files were loaded
2. code architecture understanding (layers, render flow, key algorithms)
3. current baseline/version
4. approved patch state
5. next concrete step

If any required file is missing or unread, it must say the load is incomplete and stop.

### 3.1.2 Save-command completion rule
For this project, shorthand save commands such as `save`, `save all`, and `save everything` are blocking persistence commands. This applies in all Genspark surfaces, including the Chrome extension.

A save command is complete only when the required canonical writes actually happened. The assistant must not claim save success unless it updated every file required for that session or explicitly states why a given file did not need updating. After a successful save it must explicitly report updated files, release-snapshot status, current canonical version/label, intentionally unchanged files, and whether the user wants the deploy-ready Tampermonkey script next. If any required save step failed, it must say save is incomplete and stop.

### 3.1.3 Update/sync-command completion rule
For this project, shorthand sync commands such as `update` and `claude update` are blocking reconciliation commands. This applies in all Genspark surfaces, including the Chrome extension.

An update command is complete only when incoming external code was actually received, reconciled against the canonical project state, and written back to the canonical files. If the user invokes update without pasting actual incoming code, the assistant must stop and say update cannot proceed yet. After a successful update it must explicitly report the pre-sync baseline, post-sync baseline, changed canonical files, release-snapshot status, unresolved differences, and whether the user wants the deploy-ready Tampermonkey script next. If reconciliation or writes failed, it must say update is incomplete and stop.

### 3.1.4 Deployment-prep completion rule
For this project, deployment-prep commands such as `prepare the script`, `give me code`, `give me the tampermonkey script`, and similar variants are blocking output commands. This applies in all Genspark surfaces, including the Chrome extension.

A deployment-prep command is complete only when the assistant has verified the canonical source file and actually returned the deploy-ready script inline, unless the user explicitly requested a URL instead. It must not say “here is the code” without including the code inline unless the user asked for a URL. After a successful deployment-prep response it must explicitly report the canonical source used, current version/label, and whether the output is inline code or a user-requested URL. If canonical verification failed, it must say deployment-prep is incomplete and stop.

### 3.1.5 Design-document continuity rule
This project must maintain a persistent architecture/design document at `design/architecture-design.md`. The file may be a markdown document or another stable design artifact, but it must exist and explain the app architecture/design in a restart-safe way.

Save-rule implication: whenever `save`, `save all`, or `save everything` is invoked, the assistant must ensure that the design document exists and refresh it if the app's architecture, layering, recorder design, UI organization, or operating model changed. A save is not fully complete unless the assistant either updated this document or explicitly states that it was reviewed and remains current.

Accepted shorthand commands for future sessions are documented in `developer-kickoff.md` and `workflow.md`, including load/open/retrieve/continue/get forms for the project, save/save all/save everything, update/claude update for syncing externally edited code back into Genspark, and deployment-prep commands such as give me code / give me script / prepare script.

Deployment delivery rule: unless the user explicitly asks for a URL, link, or hosted file, deploy/code requests should default to returning the full userscript inline in chat for direct copy/paste into Tampermonkey.

### 3.2 Cross-assistant sync rule
If the user explicitly says `update` or `claude update` and pastes a newer userscript from Claude or another assistant, treat that pasted code as the authoritative incoming sync payload for that operation. Reconcile it against the current project state, then write the merged result back into the canonical AI Drive files. After the sync completes, AI Drive resumes its role as the long-term source of truth.

## 4. Current verified state
This section describes the last known good project state, not the code itself.

### 4.1 Live build status
The project reached a verified v9.0 structural-read build state:
- panel rendering works
- feed hook is installed
- structural read layer is integrated
- final diagnostic produced PASS
- current read can honestly return flat-market fallback states

### 4.2 Structural-read status
The structural-read migration is complete for the current phase.  
The panel now uses the new structural layer rather than relying on the old standalone READ commentary block.

Verified structural outputs include:
- structural warning row when applicable
- active node
- node quality
- path quality
- setup health
- narrative structural read
- compact chip-style Read/ACM status headers without duplicated inner module labels

### 4.3 Flat-market behavior
A full PASS can coexist with:
- `anchor.ok = false`
- `node.ok = false`
- `path.ok = false`
- `health.ok = false`
- `reader.ok = false`

That is valid when the market is flat or lacks directional structure.  
This is a correct fallback state, not a bug.

### 4.4 Recorder state
Recorder infrastructure exists and is working, but the statistical corpus is still shallow.  
Only a limited number of days/setups were recorded at the last handoff, so base-rate claims are not yet mature.

The immediate live-market goal is to improve the recorder's truth coverage rather than add narrative complexity first. Recorder priorities are:
- preserve node-map truth
- preserve accumulation-state changes over time
- preserve setup lifecycle truth
- add explicit resolution outcomes (hit T1 / hit T2 / failed / expired / unresolved)

### 4.4.1 Canonical minimum recorder schema
The canonical minimum recorder requirement is now fixed as a ten-group schema. This is the stripped-down mandatory set and it is the operative baseline for future reconciliation work.

Non-negotiable schema constraints:
- observational-only boundary remains absolute
- exactly two targets exist: `T1` and `T2`
- do not introduce `T3`
- preserve existing fields where possible and add missing fields additively

Mandatory groups:
1. **Time / session truth** — audit timestamp, stored event timestamp, feed timestamp, candle timestamp, UTC/display offset, time-base divergence, stale-feed flag, feed age, day-key, session day-key, mismatch flag, ordering sanity flag, overall time-coherence verdict, warnings list.
2. **Structure snapshot at event** — symbol, current price, active node (strike/role/absolute strength/% of King), opposing wall presence + strike + strength, distances from price, King node identity, Gatekeeper identity, in-play node, wall count, reshuffle flag, source tag.
3. **Trend / directional context** — trend state, trend MA value, slope, direction bias, trend-filter pass/fail.
4. **Accumulation context** — accumulation state, delta, absolute delta, sequence if available, strongest support below, strongest resistance above, in-play-supported flag.
5. **Setup identity** — setup key, symbol, strike, direction, attempt number, breakout strength share (`boPct`), creation timestamp, last-update timestamp.
6. **Setup lifecycle** — current stage, BO / FT / PB-TST / CONF / GO / VOID timestamps, token history, back-through count, void reason, setup age/duration.
7. **Target context** — T1 strike, T2 strike, target ladder at setup time, provenance flags for Gatekeeper-derived and accumulation-derived targets.
8. **Outcome / resolution** — outcome state, discrete flags + timestamps for T1/T2 reached, failed, expired, voided, resolution timestamp, duration from creation to resolution, terminal classification enum (`hit_T1_only`, `hit_T2`, `failed`, `expired`, `voided`).
9. **Structural context at resolution** — price, trend state, active node, opposing wall, in-play node, accumulation state, target ladder, reshuffle flag.
10. **Recorder integrity** — captured-flags for each major snapshot block, missing-fields list, and data-quality warnings.

Implementation order remains:
1. time/session truth capture and persistence
2. structure/trend/accumulation completeness
3. target + outcome truth
4. recorder-integrity block
5. only after recorder truth is durable, higher-order review/calibration work

### 4.4.2 Immediate code status after Step 1 / Step 2 patch
The current approved minimal code change set is:
- add and wire `runOutcome(sym, last)` so GO setups can resolve to T1 / T2 / FAILED / EXPIRED on closed bars
- enforce the two-target rule by changing `assignTargets()` from three targets to two

This is intentionally smaller than the full recorder schema above. It activates basic outcome tracking without rewriting the broader recorder, UI, or engine architecture.

### 4.5 Trinity / symbol environment
Trinity symbols are active for the Atlas tab context:
- SPY
- QQQ
- SPXW
- VIX

Cross-symbol logic is conceptually planned, but live wiring still depends on feed-path verification.

## 5. Storage keys — do not rename
These keys are part of the continuity contract.

### Active keys
- `gpts_inplay_v7`
- `gpts_slices_v7`
- `gpts_panelpos_v7`
- `gpts_panelsize_v7`
- `gpts_state_v7`
- `gpts_cfg_v7`
- `gpts_stats_v7`

### Legacy present / unused
- `gpts_inplay_v6`
- `gpts_ttsize_v7`
- `gpts_ttpos_v7`

No future LLM should rename these without an explicit migration plan.

## 6. Core principle — what makes the read valuable
The read is not valuable because an LLM sounds smart.  
It becomes valuable only when it is grounded in measured outcomes.

The priority chain is:

recorded context > resolution tracking > base-rate computation > calibrated confidence > narration

That means:
1. The system captures live setup context.
2. It records what later happened.
3. It computes conditional success/failure rates.
4. It converts that into calibrated confidence with sample-size honesty.
5. Only then does a language layer narrate the result.

The narration layer is the last and thinnest link.

## 7. Contamination rule for any LLM
A model may help with:
- wording
- synthesis
- hypothesis generation
- organization
- review framing

A model may not be the source of:
- factual claims about current market structure
- success probabilities
- confidence
- setup validity
- target logic
- regime truth

If a model has a strong prior like “midday is choppy” or “QQQ divergence matters,” that is only a hypothesis until the recorder/base-rate engine confirms it from project data.

LLM priors may shape vocabulary and suggested experiments.  
They may never override measurement.

## 8. High-level architecture
The system is a layered stack.  
Each layer should stay conceptually separate so a future LLM can modify one layer without corrupting the others.

### L0 — Data / feed
Passive intake from Skylit sources:
- GEX levels feed
- VEX / vanna if available
- candles from React fiber
- canonical UTC storage under the hood
- market-session-aware grouping / bucket logic
- user-local timezone only at display
- historical-by-date endpoint access if available

### 8.1 Current intake/design emphasis
The current synced v9.1 file does more than consume the raw feed. It also reads parts of Skylit's rendered heatmap tape from the DOM so that live `%King` display, king identity, and short-horizon node history can stay aligned with what the operator sees on screen. This DOM-tape bridge is a presentation/truth-alignment layer, not a replacement for the canonical userscript source.

Current architectural emphasis also includes:
- one-minute node-history strips derived from the rendered tape
- king-roll tracking over the short horizon
- accumulation classification that uses a more explicit absolute-value / dip-tolerant detector while still showing tape-matching `%King` values

### L1 — Structure
Snapshot-level structural facts:
- node identity
- polarity
- absolute size
- role
- King
- Gatekeeper
- clusters
- flip zones
- path density
- source tagging
- price-vs-node state

### L2 — Sensing / accumulation
How structure changes over time:
- build / bleed / steady
- accumulation / dissipation
- support vs resistance tilt
- migration
- freshness / tap-count
- divergence between structure and price
- dormant / resuscitating
- hedge exhaustion / wash-off states

### L3 — Regime / context
Cross-cutting conditioning layer:
- VIX regime bands
- VIX velocity
- volatility veto
- Trinity confluence
- SPX / SPY / QQQ relationship
- VEX / vanna swing regime

### L4 — Signals / lifecycle
Single lifecycle:
- BO
- FT
- TST
- CONF
- GO

No sprawling pattern explosion.  
Named patterns may be labels, but not separate unmanaged state machines.

### L5 — READ
Observational synthesis layer:
- describes structural state
- uses regime-appropriate verbs
- highlights caveats
- stays descriptive

### L6 — Analytics / base rates
Measured statistics:
- conditional success rates
- bucketed by recorded features
- sample-size-aware
- research engine for validating hypotheses

### L7 — Lexicon
Canonical vocabulary and operational definitions:
- Pika
- Barney
- King
- Gatekeeper
- Flip Zone
- Air Pocket
- Cluster
- Dormant
- Washed-off
- etc.

Definitions may be adopted before win-rate claims are adopted.

### L8 — Narration / LLM
Deferred layer:
- converts measured stats into language
- useful for EOD review and hypothesis proposal
- never runs the live analytical core
- never invents confidence

### L9 — Learning / adaptation
Closed-loop improvement:
- resolution recording
- EOD review
- sample-gated auto-calibration
- change journaling
- reversible adaptation
- hypothesis promotion / retirement

## 9. Current roadmap
There are two notions of completion:
- engineering complete
- statistically mature

The engineering path can be completed sooner.  
The statistical path requires many more trading days.

### Phase 0 — Recorder foundation
Status: done

### Phase 1 — Rich recording
Status: next build target, equivalent to v9.1

This is the most important near-term step because later phases depend on it.

### Phase 2 — Base rates
Compute conditional success rates by recorded feature sets.

### Phase 3 — Contextual READ
READ layer surfaces the relevant base rate for the current live context.

### Phase 4 — Calibration
Blend base rates by recency / sample depth into calibrated confidence.

### Phase 5 — Solid signals
A signal becomes “solid” only when it clears:
- sufficient sample size
- calibrated-confidence threshold

### Phase 6 — LLM layer
Provider-agnostic narration layer and EOD review assistant, always downstream of measured stats.

## 10. Immediate build target: v9.1
v9.1 is the next required build step.

### 10.1 Blocking items
There are two blocking issues.

#### Blocking bug #1 — time-unit reconciliation
Setup timestamps and recorder timestamps are not fully aligned.

Known issue:
- some structures are stamped in real milliseconds
- others in feed-native seconds

Everything in resolution tracking and time-of-day analysis depends on fixing this first.

#### Blocking bug #2 — bucket refinement
`expired` must not be treated as `failed`.

Resolution outcomes need clean distinction:
- hit-T1
- hit-T2
- failed
- expired

### 10.2 Required v9.1 additions
After time reconciliation, v9.1 should add:
- resolution recorder
- refined outcome buckets
- recorded non-gating features
- drag-clamp / restore-pos hardening

### 10.3 Feature fields to record
These should be recorded even before they are used for gating.

#### Momentum / divergence
- `rsiAtBO`
- `divergence`
  - none
  - regular-bear
  - regular-bull
  - hidden-bear
  - hidden-bull
- `divLookback` default 10

RSI should be computed from closed 3-minute closes.

#### Cross-symbol context
- `qqqConfirm`
- `spxwConfirm`
- `vixRegime`

Definitions should go in even if live wiring is still awaiting feed-path verification.

#### Time-of-day
Record a time bucket from setup timestamp:
- open-drive
- morning
- midday
- afternoon
- close

This depends on the time-unit fix.

#### UI robustness
Fix drag/position behavior:
- draggable header clamp
- top minimum
- partial on-screen retention
- restore saved position safely

## 11. Doctrine from the manual that is safe to adopt now
The manual contributes language, categories, and hypotheses.  
It does not contribute truth claims until validated by project data.

### 11.1 Safe to adopt immediately
Operational definitions and labels such as:
- Mega / Major / Minor / Noise tiers
- Pika / Barney vocabulary
- King / Gatekeeper roles
- Flip Zone
- Air Pocket
- Cluster / Double-Stack
- Dormant / Resuscitating
- Building / Bleeding / Steady
- Gamma-style vs Vanna-style verbs

### 11.2 Not safe to adopt as truth yet
Any claim like:
- this pattern wins often
- this time bucket is better
- this VIX band is predictive
- this Trinity configuration is higher probability

Those are research hypotheses until measured.

## 12. Structure upgrades planned from the manual
These belong mainly to L1 and should be implemented as deterministic structure, not narrative.

### 12.1 Magnitude tiers
Use absolute magnitude tiers such as:
- Mega
- Major
- Minor
- Noise

Include a hard floor for trivial contract counts.

### 12.2 Polarity-aware King
King must encode not just size but polarity-aware meaning.

### 12.3 Gatekeeper precision
Gatekeeper should use a deterministic size relationship relative to King and carry a grade.

### 12.4 Role-position matrix
Support richer positional role assignment such as upper/lower contextual roles around spot.

### 12.5 Flip Zone
Detect adjacent major opposite-polarity pairs.

### 12.6 Path density / air pockets
Density-based path characterization should identify sparse “air pocket” zones vs dense structure.

### 12.7 Source tagging
Tag native vs derived sources cleanly.

### 12.8 Price-vs-node state
Represent at least:
- approaching
- at-node
- triggered
- through

## 13. Measurement priorities for sensing
The accumulation layer should focus on deterministic measurements, not prose.

Important primitives include:
- strike
- role / sign
- absolute strength
- distance
- rate of change
- freshness / tap-count
- migration
- total net gamma

Important derived fields include:
- support vs resistance tilt
- distance to opposing structure
- stack membership
- polarity shift context
- support-side accumulation
- resistance-side accumulation

Interpret change on smoothed windows with deadband and hysteresis.  
Do not let state tags flicker on raw noise.

Accumulation should be treated as an arc, not a one-bar slope.

## 14. Breakout-context tracking
At each breakout state, record context fields without gating on them.

This should include:
- RSI at BO
- divergence state
- VIX regime
- VIX velocity
- volatility-veto flag
- node trajectories
- tap-count
- price-delivery context
- flip-zone context
- air-pocket context

All of this must tie to outcome resolution later.

## 15. Learning / adaptation design
Adaptation is allowed, but only with safety rails.

### 15.1 Four jobs of the learning layer
1. Pair predictions with outcomes
2. Produce explanatory EOD review
3. Auto-calibrate slowly under sample gates
4. Promote or retire hypotheses

### 15.2 Guardrails
Adaptation must be:
- sample-gated
- day-gated
- reversible
- logged immutably
- compared against doctrine priors side-by-side

### 15.3 Required logging for changes
Every adaptive change should record:
- timestamp
- parameter changed
- old value
- new value
- evidence / N
- prior doctrine expectation if relevant

## 16. LLM layer design constraints
The LLM layer is deferred until measured analytics exist.  
It should sit behind a provider-agnostic interface.

### 16.1 Allowed uses
- READ narrative synthesizer
- end-of-day review
- hypothesis proposal

### 16.2 Forbidden uses
- live analytical truth engine
- setup validity determination
- confidence generation
- target generation
- recommendation layer

### 16.3 Security rule
No API key may live inside the userscript.

Hosted models must be called via the user’s own backend.  
Local Ollama is acceptable because it avoids the key problem entirely.

## 17. Known discoveries already established
These findings should be treated as established project context unless disproven later.

### 17.1 Replay is not a valid GEX corpus source
Replay animates price/candles from a client-side buffer but does not replay the GEX map frame-by-frame.

Therefore replay is not suitable for harvesting:
- live structural evolution
- accumulation
- resolution-linked node behavior

### 17.2 Historical endpoint access matters more than replay
If a date-addressable historical GEX endpoint exists, that is the correct path for backfill and corpus acceleration.

### 17.3 Wall-clock contamination is real
Replay confirmed that wall-clock time can diverge from session time.  
Therefore all capture and backfill logic must stamp data from the requested/true session clock, not the local wall clock.

### 17.4 VEX / vanna should be retained
The current project already captures some VEX/vanna-related data and discards it.  
That should be preserved in future versions for regime/swing analysis.

## 18. Open probes still required
These are unresolved and should be actively re-checked when work resumes.

### 18.1 Trinity feed-path probe
Determine whether QQQ / SPXW / VIX arrive through:
- the same feed hook, or
- separate requests

This decides cross-symbol wiring strategy.

### 18.2 Volume / SSE probe
Current candle arrays do not include volume.  
Check whether there is any authenticated live stream carrying volume fields.  
Buy/sell split is unlikely, but should not be assumed impossible without checking.

### 18.3 Polarity-definition chart match
Polarity semantics need final chart-match confirmation against live visuals and raw arrays.

### 18.4 Historical GEX backfill probe
Reverse the page-load GEX request and determine whether it accepts:
- a date
- an as-of parameter
- or another historical selector

If yes, that becomes the proper backfill engine.

## 19. Resume protocol for any future LLM
This section is the most important operational part of the document.

When a user says “start” or “resume”, the future LLM should follow this sequence.

### Step 0 — reconnect and verify environment
Reconfirm:
- Atlas tab context
- current live version visible
- panel present
- debug object present
- recorder stats days and counts

If the recorder did not capture expected dates, debug that before new feature work.

### Step 1 — hard gate: get the real current source
Ask the user to paste the full current Tampermonkey source.

Do not generate code before this happens.

Once pasted, immediately verify:
- only one `render()`
- final line is `})();`
- no obvious truncation

### Step 2 — run live probes
If market conditions allow, verify:
- Trinity feed path
- volume/SSE presence
- relevant debug outputs
- hydration state

### Step 3 — build the next version
For v9.1, build in this order:
1. time-unit reconciliation
2. resolution recorder
3. outcome bucket refinement
4. new recorded features
5. drag/restore clamp hardening

### Step 4 — logic smoke test
Before paste-back, test pure logic where possible:
- RSI
- divergence classification
- time-of-day bucketing
- resolution logic
- timestamp normalization

### Step 5 — user paste/save/reload
User pastes, saves, hard-reloads.

Then verify by probe and by visible panel evidence.

### Step 6 — post-build confirmation
Confirm:
- new fields exist
- recorder is writing them
- no parse failure
- panel still renders
- debug hooks still work

### Step 7 — update handoff
At the end of the session, update this spec rather than relying on memory.

## 20. Definition of done
There are two separate “done” standards.

### 20.1 Engineering done
The project is engineering-complete for the intended phase when:
- recorder works
- resolution works
- base-rate engine works
- READ surfaces measured statistics
- confidence is calibrated
- panel remains descriptive-only
- LLM layer is optional and downstream only

### 20.2 Statistical done
The project is statistically mature only when:
- enough trading days exist
- sample sizes are credible
- calibration has stabilized
- hypotheses have survived testing
- adaptation has real evidence behind it

Engineering done can happen far earlier than statistical maturity.

## 21. Immediate next objective
The next correct build target is still v9.1 rich recording, not more wording polish.

The required priority order is:

time normalization → outcome recording → bucket refinement → context feature capture → base rates later

If future sessions drift into style or speculative inference before those are complete, they are off track.

## 22. Copy-paste instruction block for any new LLM session
Use the following at the top of any future chat:

You are continuing a Tampermonkey userscript project called GEX Signal Tapereader.  
Do not reconstruct code from memory or from my handoff notes.  
First ask me to paste the full current userscript source.  
Do not write code until the real source is pasted.  
The script must keep exactly one render() and end with exactly one final line: `})();`  
The tool is descriptive-only, never advisory.  
Your next build priority is v9.1 rich recording: fix time-unit reconciliation first, then add resolution recording, outcome bucket refinement, and non-gating context features.  
Claims and confidence must come only from recorded outcomes and measured base rates, never from model priors.  
If you understand, begin by asking for the current full source.

## 23. Short operator summary
If you only remember five things, remember these:
1. Never code without the real pasted source.
2. Do not let LLM priors become facts.
3. v9.1 is about recording and resolution, not narration.
4. Replay is not the corpus path; historical endpoint backfill is.
5. The tool must stay descriptive-only forever.

## 8.1 Current operational architecture emphasis
The clean working model for the reader is now:

1. Current node layer
2. Future structure layer
3. State / synchronization layer
4. Output layer

Read, ACM, and Signals should remain synchronized through shared structural truth.

- Current node layer = the active node and its quality
- Future structure layer = relevant nodes above and below, including clusters and stronger second-order barriers/supports
- State layer = directional context, accumulation meaning, setup context, warnings, reshuffles, and section coordination
- Output layer = user-facing Read sentence + ACM summary + signal presentation

This model is preferred over exposing abstract R1–R6 labels in the product UI.

## 8.2 Tomorrow's live-market mission
The next live session is primarily a data-capture session for product improvement. The app should be used to gather factual structural evidence, not just to observe the panel.

Tomorrow's live priorities are:
1. validate UTC-based sequencing under live market conditions
2. capture truthful node / accumulation / setup examples
3. verify directional target correctness when multiple meaningful nodes exist
4. identify what recorder fields are still missing for resolution tracking
5. prepare the next engineering brief from recorded evidence


## 24. TREND / MAGNET / PULLBACK-NODE MODEL (user-taught 2026-08-18 — core of the mental model)
A trend, in node terms, alternates:
- **MAGNET** — the node price is DRAWN TO (rallies to). Downtrend: the heavy node BELOW.
- **PULLBACK NODE (PB)** — the node that FORMS on the counter-move and price DEFLECTS off. Downtrend: forms ABOVE =
  the resistance a trader sells from. Uptrend: forms BELOW = the support to buy from.
Sequence (dn): rally down to magnet → PB forms above → deflection → rally to the NEXT magnet → NEW PB forms LOWER.
Lower-low (magnet) / lower-high (PB), each governed by a node. Uptrend = mirror.
PB nodes APPEAR AFTER the move — so the engine PREDICTS the zone, DETECTS the strike when it lands, then flags the
ROLL. 2 consecutive rolls = signal, 3 = confirmed. The 50-SMA confirms the trend; ROLLING CEILINGS ARE the successive
pullback nodes (one mechanism, two angles). Rolled-off levels lose target status; the vacated zone is air.
Implemented as `legEngine` (v10.55), surfaced in the READ / zones / decision / ⚑ banner, recorded as `leg.*` features,
seeded in rules.json, evaluated by the nightly/weekly review (see docs/LLM-NIGHTLY-BRIEF.md § LEG ENGINE).

### 24.1 The HANDOFF (v10.56) — detect the shift, not just the strike
The roll is a STRENGTH transfer before it is a strike change: the old ceiling's %King bleeds (m15 Dec ≤ −8% or ≥25%
off its session peak) while a lower node above price builds (m15 Acm ≥ +8% or ≥ PB_MIN_PCT). `legEngine.handoff`
{active, from, to, since, leadBars} is ACTIVE while both hold and RESOLVES when `to` qualifies as the PB (then `from`
is rolled off). Uptrend = mirror (floor building higher). Recorded as `leg.handoff` (outcome: did `to` become the PB
within fwd and did price deflect off it toward the magnet; leadBars = how early the handoff called it).

### 24.2 The READ voice (user-authored 2026-08-18, verbatim; numbers live) — leg voice LEADS when a leg is active
Downtrend: 1 "Downtrend. Rallying down to 768. Expect pullback node to form from 771 ceiling rolling down." ·
2 (handoff) "Downtrend. Rallying down to 768. 771 ceiling dissipating and rolling down to form pullback node at 769." ·
3 "Resistance pullback node formed at 769. Deflection expected to target 768." ·
4 "Pulling back to resistance pullback node 769. Deflection expected to target 768 below." ·
5 "Deflected off 769. Rallying down to 768. Expect pullback node to form from 769 ceiling rolling down." ·
6a "Pullback node 769 holding. New resistance forming above at 770 — resistance stacking." ·
6b "Pullback node 769 dissipated. New pullback node formed higher at 770 — ceiling rolling up." ·
7 "Rallied down to 768 target. On watch for a pullback."
Uptrend mirror: 1 "Uptrend. Rallying up to 772. Expect pullback node to form from 768 floor rolling up." ·
2 "Uptrend. Rallying up to 772. 768 floor building and rolling up to form pullback node at 769." · 3 "Support pullback
node formed at 769. Deflection expected to target 772." · 4 "Pulling back to support pullback node 769. Deflection
expected to target 772 above." · 5 "Deflected off 769. Rallying up to 772. Expect pullback node to form from 769 floor
rolling up." · 6a "Pullback node 769 holding. New support forming below at 768 — support stacking." · 6b "Pullback node
769 dissipated. New pullback node formed lower at 768 — floor rolling down." · 7 "Rallied up to 772 target. On watch
for a pullback." Direction word/grade come from the spine; caps trail as a caveat. Pinned by test_read_voice_leg.js.

### 24.3 The latched ✓/✗ trigger (v10.56)
Per setup (sym, node, legId), CLOSED bars only: ✓↓/✓↑ latches on a rejection close away from the node (wick into the
zone, close back outside, close against the open); ✗ latches on a close through it. Once latched it never re-evaluates
("make sure you dont toggle it back and forth"); resets only on a new legId / node, or abandonment (>2× zone away for
3 closed bars unresolved). Shown bold on the in-play card row 1; recorded as `defl.trigger` (✓ hit-rate = tgt before
inval + MFE/MAE; ✗ follow-through). reactionQuality stays a hover input only.
