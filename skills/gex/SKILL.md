---
name: gex
description: >
  Load/save/status procedures for the GEX Signal Tapereader project (Skylit SPY
  options dealer-exposure Tampermonkey panel). The GitHub repo
  github.com/rassulshah/gex-signal-tapereader is the ONE source of truth. Use when the
  user says "load gex" / "load tapereader" / "open gex" / "continue gex" / "get gex"
  (restore full project context at session start), "save" / "save gex" (persist session
  state), "status" / "where are we", or asks to resume, deploy, or continue the tapereader.
---

# GEX Tapereader — load & save procedures

## ⚠️ SOURCE OF TRUTH: GIT, NOT DRIVE

The ONLY canonical project store is the GitHub repo:
`https://github.com/rassulshah/gex-signal-tapereader` (branch `main`).

A Google Drive folder named `GEX-Tapereader` / `GEX-Signal-Tapereader` also exists and
will surface if a Drive connector is enabled. **It is a STALE MIRROR — on 2026-08-17 it was
ten versions behind git (v10.37 vs v10.47). NEVER read it as canonical, never load from it,
never trust its version numbers.** If Drive is the only thing you can reach, say so and ask
the user for the repo rather than silently using Drive.

## LOAD ("load gex")

Goal: full project context + code understanding + platform understanding before touching
anything. Get the files from git, in this priority order:

1. **git clone the repo into the cloud sandbox** (works headless, no credentials for a public
   repo):
   `git clone --depth 1 https://github.com/rassulshah/gex-signal-tapereader.git`
   Then read files from the clone. Re-clone (or `git pull`) each session — never reuse an old
   copy.
2. **User's machine via device bridge** (Claude desktop app connected): repo at
   `C:\Dev\gex-signal-tapereader` — stage files into the workspace if the clone is unavailable.
3. **GitHub raw URLs** (if only WebFetch is available), e.g.
   `https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/<path>`.
4. **Attached files / zip** in the conversation (uploads directory) — use if the user pasted them.

**Read `.gex-config.json` at the repo root FIRST** — it lists the canonical files and the exact
load instruction. Then read, in order:

1. `session-state/latest-resume-note.md` — the single source of "where we are": locked scope for
   the next version, phase status, open threads, standing workflow agreements.
2. `changelog/CHANGELOG.md` — head only (latest 2–3 entries).
3. `master-spec.md` + `teaching-spec.md` — rules (file-shape rule 2.4 etc.).
4. **`current/gex-signal-tapereader.user.js` — FULL FILE** (not summary). Understand:
   - Layer 0 (feed intake): `installFeedObserver()`, `extractWalls()`, fiber candles, the tape
     `<table>` reader (`findTapeTable`/`validKingRow` — validate per row: strike in td[0], King $K
     in a later td).
   - Layer 1 (state): `STATE[sym]`, persistence keys, `LASTFEED`.
   - Layer 2 (trend): `trendVerdict()` 5-state SMA-50 machine + sanity gates.
   - Layer 3 (setup): `newSetup()`, `runMachine()`, BO/FT/PB/CONF/GO lifecycle.
   - Layer 4 (accumulation): Building/Steady/Fading detection.
   - Layer 5 (recorder): IndexedDB store, daily `data/YYYY-MM-DD.json` export.
   - Layer 6–7 (render): one `render()`, block fns (`kingHeaderBlock`, `syncBannerHtml`,
     `readBlock44`, `nodeMapSentence`, `pickEdge`, `deflectionBlock`).
   - VERIFY integrity: exactly one `render()`, final line `})();`, `@version` consistent in all
     THREE spots (header ~L4, `part1 loaded` console.log, footer feed), byte count sane.
5. **Skylit platform docs** (the framework the code implements):
   - `skylit-docs/core-concepts/core-concepts.md` — nodes as magnets, King, Gatekeeper, retest decay.
   - `skylit-docs/read-the-heatmap/how-to-read-and-use-heatseeker.md` — the 5-step framework.
   - `skylit-docs/learn/intro-to-gamma.md` — Pika/Barney polarity, absolute-value rule.
   - `skylit-docs/learn/node-lifecycle.md` — Fresh/Tested/Delivered/Decaying, real-vs-hedge.
   - `SOURCE-OF-TRUTH.md` — the Academy (`skylit-docs/learn/`) wins all doctrine conflicts.
6. DATA (from v10.44): `data/YYYY-MM-DD.json` — one file per session day. Read the coverage
   summary first (days · bars · symbols · fields-since); never run a study on missing fields.

Then report the config's 6 points:
- files loaded
- code architecture understanding (layers, render flow, key algorithms)
- platform understanding (5-step framework, magnets, node lifecycle)
- current version (with the 3-spot @version integrity check)
- patch state (which phase is built/candidate/planned; test status)
- next concrete step

WAIT for the user's direction. Do not start building on load.

## SAVE ("save" / "save gex")

Persist everything a fresh session needs. The cloud workspace is EPHEMERAL — saving means
getting files to the USER (SendUserFile) and into the git repo (source of truth), NOT Drive.

1. Update `session-state/latest-resume-note.md` — rewrite fully (not append): baseline version +
   deploy status, one-paragraph session summary, LOCKED decisions for next build, background/
   data-layer scope, shelved items, OPEN THREADS (exactly where discussion stopped, in the user's
   "1 by 1" style), mockups list, standing workflow agreements, any live-DOM/parser findings.
2. Prepend a CHANGELOG entry (mark `PLANNED / NOT YET BUILT` if no code shipped).
3. DELIVERY RULE (user-mandated 2026-08-15): ship ONE self-contained installer `.bat` — files
   embedded as base64 inside the .bat (no separate zip). The bat decodes its own payload via
   PowerShell, xcopies into `C:\Dev\gex-signal-tapereader`, then commit+push using a GIT-FINDER
   probe (git is not on the user's PATH). CRLF line endings. User's only step: download, double-click.
4. SendUserFile the installer. Also send individually any file the user will READ (mockups render inline).
5. If code shipped: include the Tampermonkey update step with the raw GitHub URL as a clickable link.
6. Tell the user the resume phrase: new window, say "load gex".

## REVIEW — TWO RUNS (v10.53)

The review split into two differently-sized jobs. `docs/LLM-NIGHTLY-BRIEF.md` holds both contracts in
full; this section is the operating summary. **They are not interchangeable.**

Why: a session is ~67 bars on overlapping 10-bar forward windows ≈ **6.7 independent observations**. A
nightly run has no power to conclude anything about a weight, and the old single process both over-claimed
and closed no loop. So the nightly keeps a logbook, the weekly does the learning, and **the panel — not
the LLM — promotes**.

### REVIEW-NIGHTLY ("nightly review" / scheduled weekday run, after the close)

Produces `learning/log/YYYY-MM-DD.json`. **No weight proposals. No edits to `learning/rules.json`.**

**1. Read.** `git clone --depth 1 https://github.com/rassulshah/gex-signal-tapereader.git` (read works; the
cloud CANNOT push — github.com is blocked at the network proxy, and the claude.ai "GitHub Integration" is a
knowledge integration with NO write tools). Read `docs/LLM-NIGHTLY-BRIEF.md` (contract 1), today's
`data/<CT-date>.json`, and `learning/rules.json`.

**2. Data arrived?** `data/<CT-date>.json` present with `bars > 0`. If not: report which stage broke and
**STOP** — no review, no log file, no speculation about a day whose data never landed.

**3. Contradictions, per bar (this is the part one day CAN answer).** Bars where the READ verdict ≠ the
direction spine; drift flipping a confirmed trend; any grade A that resolved under 30% today. Name the bar,
the values, and the mechanism.

**4. Today's regime + vote split, TODAY ONLY.** The day's regime tag / OPEX / event (every FEATURES record
now carries `regime:{tag,opex,event}`), the baseline drift, and per factor the **vote-direction split**.
Flag ≥90% one-directional factors as `1-way, not evidence`.

**5. Write the logbook** — `learning/log/YYYY-MM-DD.json`, shape in `learning/log/README.md`. Append-only.

**6. Say the power out loud:** "one day = ~N independent observations; no weight conclusions from a single
day," with N = bars / forward-window.

**7. One-line pre-open brief.** Then deliver via the cascade below.

### REVIEW-WEEKLY ("weekly learning run" / scheduled Saturday run)

Produces `learning/rules.json` (v2) + `review/YYYY-MM-DD.json`. This is where learning happens.

**1. Read.** ALL `data/*.json`, **all** `learning/log/*.json`, `learning/rules.json` v2, and the prior
weekly reviews.

**2. Analyse — honesty rules that matter more than the findings:**
- Report every factor with n, hit-rate, avg MFE/MAE **and its VOTE-DIRECTION SPLIT** (how many UP vs DOWN
  votes) **plus the period baseline drift, re-weighted by that factor's own vote mix**. A one-directional
  factor on a trending day earns accuracy for free — flag it `1-way, not evidence`. This already fooled us
  once: 2026-08-11 structure voted DOWN 46/49 on a down day and looked like 71% edge.
- **Break every factor down PER REGIME** (trend / chop / opex). A rule that works in trend and fails in
  chop averages to "meh" and teaches nothing.
- State effective sample size. Overlapping forward windows mean effective n ≈ bars/10, NOT bars.
- Calibration: is A > B > C monotone? If not, the fusion is wrong — surface it.
- If there is not enough data to conclude anything, SAY THAT. Never invent findings or numbers.
- **Walk-forward** every open proposal on the sessions since it was made; update `wf.sessions` / `wf.held`.
- **Challengers**: score the parked factors (`dir.trendFast` 10/20 vs `dir.trend5`; `dir.struct`,
  `dir.kingRoll`, `netGamma`; floor/ceiling rolling only once FCHIST has ≥5 sessions) against the incumbents
  on the SAME bars. Emit `challengers`, and a `swap` proposal where the lift is real and over the bar.
- Propose KILL-LIST additions (conditions to avoid), not just what works.

**3. Emit proposals — and never apply them.** Kinds `weight` / `swap` / `kill` / `threshold`, each with
`clearsBar` computed against the HARD BAR: **n ≥ 20 AND walk-forward held over ≥3 NEW sessions AND no
regime flip**. Sparse → `clearsBar:false, reason:"insufficient — n=X, need 20"` and the hand-set value
stands; there is no provisional nudge. `clearsBar` is an assertion, not authority: `applyProposals()` in
`v10.js` re-derives n, the walk-forward hold and the regime flip from the proposal's own numbers and
refuses anything that does not survive. Write `rules` / `proposals` / `challengers` / `killList`; leave
`weights` and `promoted` **untouched** — the panel owns them.

**4. Self-test.** If `_selftest` is present in the data dir (`data/_selftest.json`, from
`node tools/synth_day.js`), analyse it with the same procedure and report FIRST whether you recovered all
three planted properties — the true edge, the 1-way trap, the regime split. Answer key and pass criteria:
`docs/REVIEW-ACCEPTANCE.md`. Never aggregate it with real days; never emit a proposal off it.

### DELIVERY CASCADE (both runs) — in order, stop at the first that succeeds

1. **Device bridge** (`mcp__remote-devices__*`, needs the Claude desktop app running): write the file
   directly into `C:\Dev\gex-signal-tapereader\learning\log\` (nightly) or `...\learning\` + `...\review\`
   (weekly). The user's local "GEX data push" task then commits it.
2. **Google Drive** (connector, always available): create the file in Drive folder `GEX-review-inbox`.
   The user's local task moves it into the repo. Drive is a TRANSPORT ONLY here — the repo remains the
   single source of truth (see the git-first rule above).
3. **Chat**: SendUserFile the JSON.

Always report WHICH path was used, so the panel's `review` pipeline stage can be interpreted.

**Summarise** in 5-10 plain lines: what is now measured, what got promotion candidates, what is still
unproven, what field was missing that would have answered a question (forward-only data can never be
back-filled). Plus a one-line "brief" the panel can show pre-open.

Descriptive only: never entries, stops, sizing, or trade recommendations.

## STATUS ("status" / "where are we" / "how complete is it")

Report completeness LAYER BY LAYER with an honest % and one sentence of what works vs what is
missing per layer (user-approved 2026-08-16): Sensing (L0–1) · Dashboard reader · Recording &
self-scoring · Analysis tab · Testing pipeline · Nightly review · Multi-symbol/Trinity. Close with
an overall one-liner. Never inflate; unverified-live code stays "candidate".

## STANDING PROJECT RULES (apply always)

- **FEATURE ENROLLMENT (user-mandated 2026-08-17): no feature ships un-enrolled.**
  Every feature — new ones included — must AUTOMATICALLY enter all three improvement layers and
  become part of the tape-reader mental model: DATA (recorded per bar in the day file), ANALYSIS
  (a "did it tell the truth?" scorecard with rate + n), and TESTING/LEARNING (a question in the
  queue + a rule in learning/rules.json, scored nightly, graduates at n>=20, decays if it
  underperforms). Mechanism = the FEATURES registry: each feature self-declares once
  {key,label,record,outcome,fwd,questions,rule} and the recorder / Analysis tab / testing pipeline
  iterate it. Adding a feature = one registry entry = auto-enrolled everywhere. A feature is NOT
  "done" until it is in FEATURES and the three layers consume it. See design/spec-feature-enrollment.md.

- Before coding: ASK first (user may have more fixes) and show MOCKUPS for review.
- **ONE AT A TIME (user-mandated, repeated 2026-08-15): discuss exactly ONE element per message.**
  Never list all open items and their fixes in one reply. State the one item, its fix, ask, STOP.
  Everything-at-once lists are the recurring failure mode — do not do it.
- Nodes are MAGNETS (attract/pull, repel/push). Every indicator is either DESCRIPTIVE or
  PREDICTIVE (⚖ hand-set / 📊 measured, nightly-scored, graduates at n≥20). Nothing vague between.
- The tool is DESCRIPTIVE/observational only: never entries, stops, sizing, R:R, or P&L.
- Hover/tooltip explanations on every element, written for a new reader.
- MODEL ROUTING (user-mandated 2026-08-15): delegate mechanical / well-specified work (code edits to
  a clear spec, running tests, packaging installers, search, scraping, formatting) to cheaper models
  via the Agent tool; reserve the main model for novel design, statistical interpretation, deciding
  WHAT to build, and reviewing another model's output.
