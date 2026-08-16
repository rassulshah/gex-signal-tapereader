---
name: gex
description: >
  Load/save procedures for the GEX Signal Tapereader project (Skylit SPY options
  dealer-exposure panel, C:\Dev\gex-signal-tapereader). Use when the user says
  "load gex" (restore full project context at session start), "save" / "save gex"
  (persist session state so a new window can resume), or asks to resume, deploy,
  or continue the tapereader project.
---

# GEX Tapereader — load & save procedures

## LOAD ("load gex")

Goal: full project context + code understanding + platform understanding before touching anything. Sources in priority order:

1. **Attached files / zip** in this conversation (uploads directory) — use if present.
2. **User's machine via device bridge** (Claude desktop app connected):
   `C:\Dev\gex-signal-tapereader` — stage needed files into the workspace.
3. **GitHub raw URLs** (for Skylit docs + userscript):
   - `https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-signal-tapereader.user.js`
   - `https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/skylit-docs/...`

Read, in order:
1. `session-state/latest-resume-note.md` — the single source of "where we are":
   locked scope for the next version, open threads, standing workflow agreements.
2. `changelog/CHANGELOG.md` — head only (latest 2–3 entries).
3. `master-spec.md` + `teaching-spec.md` — rules (file-shape rule 2.4 etc.).
4. **`current/gex-signal-tapereader.user.js` — FULL FILE** (not summary). Understand:
   - Layer 0 (feed intake): `installFeedObserver()`, `extractWalls()`, fiber candle reading
   - Layer 1 (state): `STATE[sym]`, persistence keys, `LASTFEED`
   - Layer 2 (trend): `trendVerdict()` 5-state machine, continuous SMA, sanity gates
   - Layer 3 (setup): `newSetup()`, `runMachine()`, BO/FT/PB/CONF/GO lifecycle
   - Layer 4 (accumulation): `accumData()`, Building/Steady/Fading detection
   - Layer 5 (recorder): `repoWrite()`, IndexedDB storage, daily export
   - Layer 6–7 (render): `render()`, block functions (`readBlock44`, `deflectionBlock`, `nodeMapBlock`)
   - Verify: exactly one `render()`, final line `})();`, byte count matches version.
5. **Skylit platform docs** (minimum 3, understand the framework the code implements):
   - `skylit-docs/core-concepts.md` — nodes as magnets, King, Gatekeeper, retest decay
   - `skylit-docs/read-the-heatmap/how-to-read-and-use-heatseeker.md` — 5-step framework
   - `skylit-docs/learn/intro-to-gamma.md` — Pika/Barney polarity, absolute value rule
   - **Optional:** `skylit-docs/learn/node-lifecycle.md`, `SOURCE-OF-TRUTH.md`
6. `design/` mockups referenced by the resume note (build specs).
7. DATA (from v10.44): `data/YYYY-MM-DD.json` — one file per session day, written by the
   userscript at the close and auto-pushed by a scheduled task. Fetch via raw GitHub URL
   (same host TM updates from). Read the coverage summary first (days · bars · symbols ·
   fields-since) — never run a study on fields that don't exist for the days in question.

Then report: 
- baseline version
- code architecture understanding (layers, render flow, key algorithms)
- platform understanding (5-step framework, magnet concept, node lifecycle)
- next build target
- open threads

WAIT for the user's direction. Do not start building on load.

## SAVE ("save" / "save gex")

Persist everything a fresh session needs. The cloud workspace is EPHEMERAL —
saving means getting files to the USER (SendUserFile) and/or their repo/Drive.

1. Update `session-state/latest-resume-note.md` — rewrite fully (not append):
   baseline version + deploy status, one-paragraph session summary, LOCKED
   decisions for next build, background/data-layer scope, shelved items,
   OPEN THREADS (exactly where discussion stopped, in the user's "1 by 1" style),
   mockups list, standing workflow agreements, any live-DOM/parser findings.
2. Prepend a CHANGELOG entry (mark `PLANNED / NOT YET BUILT` if no code shipped).
3. DELIVERY RULE (user-mandated 2026-08-15): ship ONE self-contained installer
   .bat — files embedded as base64 inside the .bat itself (no zip, no extraction
   step). The bat must: decode its own payload via PowerShell
   ([Convert]::FromBase64String on everything after a :::PAYLOAD::: marker),
   Expand-Archive to %TEMP%, xcopy into C:\Dev\gex-signal-tapereader, then
   commit+push using the GIT-FINDER (git is NOT on the user's PATH — probe:
   where git → Program Files\Git\cmd\git.exe → Program Files (x86) →
   %LOCALAPPDATA%\Programs\Git → GitHub Desktop app-*\resources\app\git\cmd\git.exe;
   if none found, tell the user to use their usual tool). CRLF line endings.
   User's only step: download, double-click.
4. SendUserFile the installer (git, not Drive, holds the changelog — user rule).
   Also send individually any file the user will READ (mockups render inline).
5. If code shipped: include Tampermonkey update step with the raw GitHub URL as
   a clickable hyperlink.
6. Tell the user the exact resume phrase: new window, say "load gex" (attach the
   installer or repo files if the skill isn't saved to their account).


## STATUS ("status" / "where are we" / "how complete is it")

Report completeness LAYER BY LAYER with an honest % and one sentence of what works vs what is
missing per layer (user-approved format 2026-08-16): Sensing (L0–1) · Dashboard reader (READ/
header/Node Map/Defl) · Recording & self-scoring · Analysis tab · Testing pipeline · Nightly review
· Multi-symbol/Trinity. Close with an overall one-liner: usable-as-what today, and which phases turn
it into what. Never inflate; unverified-live code stays "candidate".

## STANDING PROJECT RULES (apply always)

- Before coding: ASK first (user may have more fixes) and show MOCKUPS for review.
- **ONE AT A TIME (user-mandated, repeated 2026-08-15): discuss exactly ONE element per
  message. Never list all open items and their fixes in one reply. State the one item,
  its fix, ask for the decision, STOP. Move to the next only after the user confirms.**
  Everything-at-once lists are the recurring failure mode — do not do it.
- Nodes are MAGNETS (attract/pull, repel/push): every indicator is either
  DESCRIPTIVE (what the field is doing) or PREDICTIVE (⚖ hand-set / 📊 measured,
  nightly-scored, graduates at n≥20). Nothing vague in between.
- Hover/tooltip explanations on every element, written for a new reader.
- Every deploy ships with step-by-step instructions.
- MODEL ROUTING (user-mandated 2026-08-15): use the CHEAPEST model that does the job.
  Delegate mechanical / well-specified work to subagents on Opus or lower (Agent tool
  `model: 'opus'|'sonnet'|'haiku'`): code edits to a clear spec, running tests, packaging
  installers, code search, data scraping, doc formatting. Reserve Fable (the main loop, or
  `model:'fable'`) for work that needs its judgment: novel design/architecture, statistical
  interpretation, deciding WHAT to build, ambiguous trade-offs, reviewing another model's
  output. When unsure whether a task needs Fable, it usually doesn't — delegate down, then
  Fable-review the result.
