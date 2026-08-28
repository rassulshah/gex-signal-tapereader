# GEX Signal Tapereader — Architecture / Design Document

> ⚠⚠ **SUPERSEDED ON DATA SOURCES by `design/DATA-ARCHITECTURE.md` (2026-08-28).**
> This file's Layer 0 lists only the Skylit feed and the fiber candles. It predates the
> InsiderFinance companion, the two-book split, the futures corpus and the Yahoo tap, and it still
> names **AI Drive** as the persistent store — git is the source of truth. It remains correct on
> the layer model, the file-shape rules and the observational boundary.
> **Where the two disagree about where data comes from, DATA-ARCHITECTURE.md wins.**

## Purpose
This document exists so a brand-new context window can recover the app's architecture and design intent without inferring it from scattered notes. It is a structural design reference, not the authoritative code source. If this document conflicts with the current userscript, the current userscript wins.

## Product boundary
GEX Signal Tapereader is a Tampermonkey userscript for Skylit Atlas (`https://app.skylit.ai/atlas*`). It overlays a descriptive panel that reads market structure for symbols such as SPY and QQQ.

It is strictly observational. It must not emit:
- entries
- stops
- sizing
- risk/reward
- P&L framing
- probabilities or confidence values that are not backed by recorded outcomes
- trade recommendations

## Runtime environment
- Deployment target: Tampermonkey
- Persistent project store: AI Drive `/GEX-Signal-Tapereader/`
- Canonical code file: `current/gex-signal-tapereader.user.js`
- Core persistence keys: `gpts_inplay_v7`, `gpts_slices_v7`, `gpts_panelpos_v7`, `gpts_panelsize_v7`, `gpts_state_v7`, `gpts_cfg_v7`, `gpts_stats_v7`

## File-shape constraints
The userscript must preserve:
- exactly one `render()`
- a single IIFE
- final line exactly `})();`
- no storage-key renames without an explicit migration plan

## Source-of-truth hierarchy
1. current userscript source
2. live runtime / probes / localStorage evidence
3. persisted recorder output
4. master spec
5. this design doc
6. prior notes / chat context

## High-level architecture
The app is a compact layered system:

### Layer 0 — Intake / transport
Purpose: acquire raw market structure inputs.

Main sources:
- Skylit `gex/levels` feed intercepted through the fetch observer
- candle data read from the page's TradingView/React fiber structures
- optional derived feed information such as VEX-related context when available

Main responsibilities:
- intercept feed responses
- normalize feed payloads into walls / price / king structures
- convert candle time values into canonical runtime timestamps
- maintain latest feed snapshots per symbol

Representative functions:
- `installFeedObserver()`
- `onFeed(...)`
- `extractWalls(...)`
- `synthDerived(...)`
- `readFiberCandles()`
- `convertFiberCandles(...)`
- `applyCandles(...)`

### Layer 0A — Rendered tape / DOM truth bridge
Purpose: align structural percentages and king identity with the numbers Skylit actually renders on the heatmap tape, instead of relying only on feed-side re-derivation.

Main ideas:
- read the rendered tape table from the DOM
- cache tape percentages and king strike
- prefer tape-derived live `%King` when ACM / target / strength presentation needs to match what the operator sees
- maintain compatibility with feed/wall fallback logic when the tape cannot be read

Representative functions:
- `tapeMap(...)`
- `readTapeFromDOM(...)`
- `firstStrengthPct(...)`
- `tapeKingStrike(...)`

### Layer 1 — State model
Purpose: hold the current per-symbol structural state and recorder state.

Per-symbol state includes concepts such as:
- current price
- king
- wall map
- candles / current candle
- setups
- last processed closed bar
- trend / accumulation derived state

Global state also includes:
- config (`CFG`)
- latest feed caches (`LASTFEED`, `LASTVEX`)
- persistent log / slice caches
- UI position and size persistence

### Layer 1A — Short-horizon node trajectory memory
Purpose: keep a compact rolling memory of how individual node strengths and king identity are moving over the last several minutes.

Main ideas:
- one-minute `%King` history strips per strike
- king-strike history for roll detection
- visible strip and badge logic should be based on the same underlying samples so the operator never sees a badge/strip mismatch

Representative structures / functions:
- `HIST`
- `KINGHIST`
- `sampleTapeHistory(...)`
- `nodeHistory(...)`
- `kingRoll(...)`
- `histTrend(...)`

### Layer 2 — Setup engine
Purpose: detect and progress descriptive setup lifecycle states.

The setup engine is bar-driven and operates on closed candles. It creates and advances setups through structural lifecycle states such as:
- BO
- FT
- TST / PB
- CONF
- GO
- VOID

Representative functions:
- `newSetup(...)`
- `addToken(...)`
- `stageAnchor(...)`
- `runMachine(sym)`
- `assignTargets(...)`

Design intent:
- setup progression must remain descriptive
- current architecture favors small additive logic changes over engine rewrites
- setup state is persisted and surfaced for review, not turned into trading directives

## Target and outcome model
Current approved direction:
- exactly two targets exist: `T1` and `T2`
- no `T3`
- target hits are structural observations, not trade instructions

Current minimal outcome layer:
- `runOutcome(sym, last)` evaluates GO setups on closed bars
- touch-based target detection
- failure means close back through strike before any target hit
- expiration means no target hit by end of day

This is intentionally smaller than the full recorder schema. Richer persistence and integrity fields remain future work.

## Accumulation design direction
The current accumulation direction now blends two ideas:
- **display truth** should match the rendered tape (`%King`, king identity, visible node strips)
- **state classification** should use an absolute-value-based accumulation detector with dip tolerance, so a normal pullback does not instantly flip a genuinely building node to fading

This means the operator-facing tape and the internal accumulation badge are designed to stay semantically aligned while still honoring the project's structural interpretation logic.

## Recorder design
There are two recorder concepts:

### A. Lightweight historical stats / state persistence
Existing persistence covers:
- setup state via `persistState()` / `restoreState()`
- day-level stats in `gpts_stats_v7`
- node-map slices in `gpts_slices_v7`

### B. Canonical recorder schema (future-complete target)
The minimum long-term recorder requirement is the ten-group schema documented in the master spec. It includes:
- time/session truth
- structure snapshot
- trend context
- accumulation context
- setup identity
- setup lifecycle
- target context
- outcome / resolution
- structure at resolution
- recorder integrity flags

The recorder exists to support later evidence-based refinement of signals, accumulation interpretation, and the Read — not to justify intuition.

## Time model
Time handling is a critical architectural feature.

Design rule:
- store canonical runtime timestamps consistently
- separate stored truth from display formatting
- validate session grouping and bar ordering under live conditions

Implications:
- display timezone may vary by user
- internal capture must stay coherent across feed and candle time bases
- time-truth validation remains a first-class engineering concern

## Read / ACM / Signals relationship
The visible panel is a synchronized presentation of three related ideas:
- Signals = lifecycle / structural events
- ACM = accumulation / in-play structure state
- Read = descriptive synthesis of the current structural picture

Design intent:
- these layers should stay synchronized
- UI wording can be refined, but not at the cost of logic distortion
- the outer section header names the module; tighter badge/header composition is preferred over redundant internal labels

## UI architecture
The UI is a floating overlay panel with:
- draggable / resizable behavior
- a configuration drawer
- per-symbol sections
- signal/log views
- structural read and accumulation summaries
- feed-status / operational diagnostics

Representative UI functions:
- `buildPanel()`
- `render()`
- `structuralReadHtml()`
- `accumBlock()`
- `feedStatusHtml()`
- `stageTimeline(...)`

UI rules:
- keep presentation descriptive
- avoid duplicate labels / wasted vertical rhythm
- no architectural rewrites solely for cosmetic reasons
- prefer denser presentation when it reduces wasted space without hiding structure

Current notable UI direction in the synced file:
- combined long+short signal grid rather than separate stacked blocks
- header badges integrated into the section header row
- broader drag surface for the panel

## Debug / operator surface
The script intentionally exposes debug hooks under `window.__gptsDebug` so live sessions can inspect state without editing code.

Typical debug responsibilities include:
- state inspection
- anchor/path/quality checks
- manual time auditing
- manual refresh support

## Save / restart philosophy
A brand-new context window should be able to restart safely by reading the canonical project files from AI Drive. That restart package now consists of:
- `master-spec.md`
- `teaching-spec.md`
- `current/gex-signal-tapereader.user.js`
- `session-state/latest-resume-note.md`
- `changelog/CHANGELOG.md`
- `design/architecture-design.md`
- `workflow.md`
- `developer-kickoff.md`

The design doc is mandatory because the spec is rule-heavy and the code is implementation-heavy; this file bridges architecture and intent.

## Current verified baseline at time of writing
- v9.1 uploaded-sync baseline with the approved Step 1 / Step 2 recorder patch still present
- `runOutcome(sym, last)` exists and is wired into `runMachine(sym)`
- `assignTargets()` is restricted to two targets
- DOM tape reading and short-horizon node-history strips are present in the synced file
- accumulation logic now includes a more explicit absolute-value / dip-tolerant trend detector
- command hardening exists for load / save / update / deployment-prep across Genspark surfaces including the Chrome extension

## Current highest-priority live validation goals
1. verify GO setups resolve correctly to T1 / T2 / FAILED / EXPIRED on live closed bars
2. verify no stray third target appears anywhere
3. verify same-tick persistence after outcome updates
4. verify failed never overrides a prior target hit
5. expand persistence toward the full ten-group recorder schema, starting with time/session truth capture

## How to use this document
Use this file when a new assistant needs to understand:
- what the app is
- how the layers fit together
- why the recorder exists
- why time truth and observational boundaries matter

Do not use this file to override the literal source.
