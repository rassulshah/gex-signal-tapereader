# GEX Signal Tapereader — Working Workflow

This document defines the standard development loop for maintaining the project with AI Drive as the persistent source of truth.

## 1. Canonical project files
The project lives under `/GEX-Signal-Tapereader/`.

Core files:
- `master-spec.md` — restart-safe operating spec
- `teaching-spec.md` — doctrine / terminology / rationale
- `current/gex-signal-tapereader.user.js` — authoritative current code
- `session-state/latest-resume-note.md` — what is done, what is next
- `changelog/CHANGELOG.md` — factual project history
- `design/architecture-design.md` — architecture / design explainer for restart-safe context

Supporting folders:
- `releases/` — dated code snapshots
- `probes/` — probe output, diagnostics, screenshots notes
- `design/` — architecture / design references

## 2. Source-of-truth rule
Use this precedence order:
1. current userscript source
2. live probes / runtime evidence
3. persisted recorder/state data
4. master spec
5. teaching spec
6. prior chat context

If there is a conflict, the current userscript wins.

## 3. Shorthand command contract
The following user phrases should be treated as operational commands for this project.

## 3.1 Command-proof rule across shorthand commands
For this project, shorthand commands are completion-sensitive operational commands, not informal intent hints. This applies in all Genspark environments, including the Chrome extension.

General rule:
- do not claim a shorthand command succeeded unless its required reads / writes / outputs actually happened
- if a required step failed or was skipped, say the command is incomplete and name the missing step
- after each shorthand command, explicitly report the proof-of-completion fields defined in that command section

### Load/open/retrieve/continue/get project
Any of these mean: load the canonical AI Drive project files and continue from the current project state.
- `load gex`
- `load gex project`
- `load tapereader`
- `load gex tapereader`
- `open gex`
- `open gex project`
- `open tapereader`
- `open gex tapereader`
- `retrieve gex`
- `retrieve gex project`
- `retrieve tapereader`
- `retrieve gex tapereader`
- `continue gex`
- `continue gex project`
- `continue tapereader`
- `continue gex tapereader`
- `get gex`
- `get gex project`
- `get tapereader`
- `get gex tapereader`

When used, the assistant should load:
1. `master-spec.md`
2. `teaching-spec.md`
3. `current/gex-signal-tapereader.user.js`
4. `session-state/latest-resume-note.md`
5. `changelog/CHANGELOG.md`
6. `design/architecture-design.md`

Blocking completion rule for **all Genspark environments including the Chrome extension**:
- `load gex` is not complete until all five canonical files above have actually been read.
- The assistant must not continue with project reasoning, patch planning, coding, or status claims until that read is finished.
- After reading, the assistant must explicitly report:
  1. which files were loaded
  2. current code baseline/version
  3. current approved patch state
  4. next concrete step
- If any required file was not read, the assistant must say the load is incomplete, name the missing file(s), and stop rather than pretending the project is synced.
- In the Chrome extension, `load gex` must be treated the same as `load gex and verify all canonical files were read before continuing`.

### Save project state
Any of these mean: run the full project save routine.
- `save`
- `save all`
- `save everything`

Default save routine:
1. overwrite `current/gex-signal-tapereader.user.js`
2. update `session-state/latest-resume-note.md`
3. append/update `changelog/CHANGELOG.md`
4. update `master-spec.md` if operating rules, architecture, roadmap, or verified state changed
5. update `teaching-spec.md` if doctrine, terminology, or conceptual mapping changed
6. save a dated release snapshot in `releases/` when the change is meaningful
7. create or update `design/architecture-design.md` whenever architecture/design understanding changed, or create it if missing
8. after saving, ask the user whether they also want the deploy-ready Tampermonkey script

Blocking completion rule for **all Genspark environments including the Chrome extension**:
- `save` is not complete until the canonical file writes actually happened.
- The assistant must not say the project was saved unless it updated every required file for that session or explicitly states why a given file did not need updating.
- After saving, the assistant must explicitly report:
  1. which files were updated
  2. whether a release snapshot was created
  3. the current canonical code version / label
  4. any files intentionally left unchanged
  5. whether the user wants the deploy-ready Tampermonkey script next
- If a requested save step failed, the assistant must say save is incomplete, name the failed step, and stop instead of claiming success.

### Update/sync from Claude or another assistant
Any of these mean: sync externally updated code back into the canonical Genspark project.
- `update`
- `claude update`

Expected input:
- updated Tampermonkey code, or
- updated code plus an accompanying spec / instructions

When used, the assistant should:
1. treat the newly supplied code as the incoming candidate source for this sync
2. compare it against the canonical AI Drive project state
3. adapt supporting files if the incoming update changes rules, architecture, roadmap, verified state, or doctrine
4. overwrite `current/gex-signal-tapereader.user.js` with the synced canonical version
5. update `session-state/latest-resume-note.md` to reflect what changed and what comes next
6. append a factual sync entry to `changelog/CHANGELOG.md`
7. save a dated release snapshot in `releases/` when the external update is meaningful
8. ask whether the user also wants the deploy-ready Tampermonkey script

Blocking completion rule for **all Genspark environments including the Chrome extension**:
- `update` / `claude update` is not complete until the incoming code was actually read, reconciled, and written back to the canonical project files.
- If the user invoked update but did not provide actual incoming code, the assistant must stop and say update cannot proceed yet.
- After syncing, the assistant must explicitly report:
  1. whether incoming external code was received
  2. baseline before sync
  3. baseline after sync
  4. which canonical files were changed
  5. whether a release snapshot was created
  6. any unresolved differences or deferred follow-ups
  7. whether the user wants the deploy-ready Tampermonkey script next
- If reconciliation or file-write steps failed, the assistant must say update is incomplete, name the failure, and stop instead of implying sync succeeded.

Rule: when the user explicitly invokes an update/sync command and pastes newer code from Claude or another assistant, that pasted code becomes the authoritative incoming source for the purpose of syncing Genspark forward. AI Drive remains the long-term canonical store after the sync is completed.

### Prepare deployable script
Any of these mean: prepare the canonical script for deployment into Tampermonkey.
- `prepare the script`
- `prepare script`
- `give me script`
- `give me code`
- `give me the code`
- `code`
- `give me the tampermonkey script`
- `give me the javascript`
- `prepare deploy copy`

When used, the assistant should:
1. ensure `current/gex-signal-tapereader.user.js` is the latest canonical version
2. present the script for deployment
3. default to giving the full deploy-ready code inline in chat for copy/paste unless the user explicitly asks for a URL instead
4. if useful, remind the user to paste into Tampermonkey, save, hard-refresh Atlas, and verify

Blocking completion rule for **all Genspark environments including the Chrome extension**:
- deployment-prep commands are not complete until the assistant has verified the canonical source file it is presenting and actually returned the deploy-ready script or explicitly stated why it could not.
- The assistant must not claim "here is the code" without providing the actual code inline unless the user explicitly requested a URL.
- The assistant must explicitly report:
  1. the canonical source file used
  2. the current version / label of the script being presented
  3. whether the reply contains the full deploy-ready code inline or a user-requested URL
  4. any reason the code was split into multiple parts
- If the canonical code was not verified first, the assistant must stop and say deployment-prep is incomplete.

Deployment should not be assumed automatically just because the user said save; the assistant should ask after saving whether the deploy-ready script is also wanted.

Inline-code preference rule: unless the user explicitly requests a URL, link, or hosted file, the default delivery format for `give me code` / deployment-prep requests is the full userscript inline in chat so the user can copy/paste it directly into Tampermonkey.

## 4. Start-of-session procedure
At the beginning of any development session:
1. Read `master-spec.md`
2. Read `teaching-spec.md`
3. Read `current/gex-signal-tapereader.user.js`
4. Read `session-state/latest-resume-note.md`
5. Read `changelog/CHANGELOG.md`
6. Read `design/architecture-design.md`
7. Confirm the next concrete step before editing anything

Mandatory load-verification response:
- list the canonical files that were successfully read (including the design document)
- state the current baseline/version from the current userscript
- state the current approved patch state from the canonical notes
- state the next concrete step
- if any file was not read, stop and say the load is incomplete

If the user is performing an external sync/update and supplies fresh Claude-generated code, read that incoming code first as the update payload, then reconcile it against the canonical project files before saving.

Do not begin coding from memory.

## 5. Code update procedure
For any planned change:
1. Work against `current/gex-signal-tapereader.user.js`
2. Preserve file-shape constraints:
   - exactly one `render()`
   - one IIFE
   - final line exactly `})();`
3. Do not rename storage keys unless an explicit migration plan is being implemented
4. Keep the panel descriptive-only
5. Smoke-test risky pure logic before deployment when possible

## 6. Deployment model
AI Drive file = canonical source  
Tampermonkey editor = deployment target

Recommended sequence:
1. update canonical file in AI Drive
2. save release snapshot if meaningful
3. deploy updated code to Tampermonkey
4. hard refresh Atlas
5. verify with probe and visible panel evidence

## 7. End-of-session procedure
After each meaningful session:
1. overwrite `current/gex-signal-tapereader.user.js` with the latest canonical build
2. save a dated snapshot in `releases/` if the session changed code materially
3. update `session-state/latest-resume-note.md` with:
   - what is verified
   - what is not a bug
   - exact next concrete step
4. append a factual entry to `changelog/CHANGELOG.md`
5. update `master-spec.md` if project rules/state/architecture changed
6. update `teaching-spec.md` if doctrine or conceptual mapping changed
7. create or refresh `design/architecture-design.md` if architecture/design understanding changed or if the file is missing
8. save any useful diagnostics to `probes/`

## 8. Release naming convention
Suggested format:
- `YYYY-MM-DD_v9.0_verified.user.js`
- `YYYY-MM-DD_v9.1_candidate.user.js`
- `YYYY-MM-DD_v9.1_verified.user.js`

Status words should reflect reality:
- `candidate` = not fully verified yet
- `verified` = tested and confirmed

## 9. Resume-note template
Each resume note should include:
- what is already integrated and verified
- what output is correct but may look like a bug
- the current target version
- the exact next task
- any open questions
- any constraints on how the next change must be delivered

## 10. Scope guardrails
Never let the project drift into:
- trade recommendations
- entries/stops/position sizing
- unsupported probability claims
- model-prior-based confidence
- code reconstruction from summaries alone

## 11. Current default direction
Until replaced by a newer session-state note, the next build objective is:
**v9.1 rich recording beginning with time-unit reconciliation.**
