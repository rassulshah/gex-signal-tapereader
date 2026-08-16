# GEX Signal Tapereader — Developer Kickoff Prompt

Use this prompt at the start of any new Genspark development session.

---

Resuming the **Gex Signal Tapereader** project.

Project files are stored in AI Drive under:
- `/GEX-Signal-Tapereader/master-spec.md`
- `/GEX-Signal-Tapereader/teaching-spec.md`
- `/GEX-Signal-Tapereader/current/gex-signal-tapereader.user.js`
- `/GEX-Signal-Tapereader/session-state/latest-resume-note.md`
- `/GEX-Signal-Tapereader/changelog/CHANGELOG.md`
- `/GEX-Signal-Tapereader/design/architecture-design.md`
- `/GEX-Signal-Tapereader/workflow.md`

## Accepted shorthand commands
Treat these user phrases as project commands.

## Command-proof rule across shorthand commands
For this project, shorthand commands are blocking operational commands, not casual hints. This applies in all Genspark environments, including the Chrome extension.

General rule:
- do not claim a shorthand command succeeded unless its required reads / writes / outputs actually happened
- if a required step failed or was skipped, say the command is incomplete and name the missing step
- after each shorthand command, explicitly report the proof-of-completion fields defined in that command section

### Project load commands
Any of these mean: load the canonical AI Drive project files and continue from current project state.

Blocking rule for **all Genspark environments including the Chrome extension**:
- do not treat `load gex` as complete until every canonical file has actually been read
- do not continue with project reasoning, edits, or claims of sync until the read is complete
- after the read, explicitly report loaded files, current baseline/version, current approved patch state, and next concrete step
- if any required file was not read, say the load is incomplete, name the missing file(s), and stop
- in the Chrome extension, interpret `load gex` as `load gex and verify all canonical files were read before continuing`
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

### Save commands
Any of these mean: run the full project save routine.
- `save`
- `save all`
- `save everything`

Default save routine:
- overwrite `current/gex-signal-tapereader.user.js` with the newest canonical version
- update `session-state/latest-resume-note.md`
- append/update `changelog/CHANGELOG.md`
- update `master-spec.md` if rules, architecture, roadmap, or verified state changed
- update `teaching-spec.md` if doctrine, terminology, or conceptual mapping changed
- save a dated release snapshot in `releases/` when the change is meaningful
- create or refresh `design/architecture-design.md` when architecture/design understanding changed or if the file is missing
- after saving, ask whether the user also wants a deploy-ready script for Tampermonkey

Blocking rule for **all Genspark environments including the Chrome extension**:
- do not say save succeeded unless the canonical writes actually happened
- after saving, explicitly report updated files, snapshot status, current canonical version/label, any intentionally unchanged files, and whether the user wants deploy-ready code next
- if any required save step failed, say save is incomplete and stop

### Update/sync commands
Any of these mean: the user is pasting newer code from Claude or another assistant and wants Genspark updated to match.
- `update`
- `claude update`

Expected behavior:
- treat the pasted external code as the incoming candidate source
- compare it with the canonical AI Drive project state
- adapt supporting files when the external update changes rules, roadmap, verified state, or doctrine
- overwrite `current/gex-signal-tapereader.user.js` with the synced canonical version
- update `session-state/latest-resume-note.md`
- append a factual sync entry to `changelog/CHANGELOG.md`
- save a dated release snapshot in `releases/` when the sync is meaningful
- after syncing, ask whether the user also wants the deploy-ready script for Tampermonkey

Blocking rule for **all Genspark environments including the Chrome extension**:
- do not say update/sync succeeded unless incoming code was actually read, reconciled, and written to the canonical files
- if update was invoked without actual incoming code, stop and say update cannot proceed yet
- after syncing, explicitly report whether incoming code was received, baseline before/after sync, changed canonical files, snapshot status, unresolved differences, and whether deploy-ready code is wanted next
- if any required sync step failed, say update is incomplete and stop

### Deployment-prep commands
Any of these mean: prepare the latest canonical userscript for copy/paste deployment into Tampermonkey.
- `prepare the script`
- `prepare script`
- `give me script`
- `give me code`
- `give me the code`
- `code`
- `give me the tampermonkey script`
- `give me the javascript`
- `prepare deploy copy`

When one of these is used, the expected behavior is:
- make sure `current/gex-signal-tapereader.user.js` is the final canonical version
- present that script for deployment
- default to pasting the full deploy-ready code inline in chat for copy/paste unless the user explicitly asks for a URL instead
- if appropriate, remind the user to paste into Tampermonkey, save, and hard-refresh Atlas

Note: after a save command, deployment should not be assumed automatically; ask the user whether they want the deploy-ready Tampermonkey script next.

Inline-code preference rule: unless the user explicitly asks for a URL, link, or hosted file, deployment-prep requests should return the full userscript inline in chat.

Blocking rule for **all Genspark environments including the Chrome extension**:
- do not say deployment-prep is complete unless the canonical source file was verified and the actual deploy-ready code was returned (or a user-requested URL was provided)
- do not claim "here is the code" without actually including the code inline unless the user asked for a URL
- explicitly report the canonical source used, current version/label, and whether the output is inline code or a requested URL
- if canonical verification failed, say deployment-prep is incomplete and stop

## Operating rules
- **DISCUSS ONE ITEM AT A TIME.** One element per message; ask, wait for the decision, then move on. Never dump the full list of open items + fixes in a single reply (user rule, 2026-08-15).
- Do not reconstruct code from memory, summaries, or prior chat context.
- Treat the current userscript file as the authoritative codebase.
- Read the specs for constraints, doctrine, and roadmap, but do not let them override the pasted or stored source.
- Confirm the real source is in hand before proposing edits.
- Preserve file shape: exactly one `render()`, one IIFE, final line exactly `})();`.
- Do not rename storage keys.
- Keep the tool descriptive-only, never advisory.
- Claims and confidence must come only from recorded outcomes and measured base rates, never model priors.

## Required startup sequence
1. Load and read:
   - `master-spec.md`
   - `teaching-spec.md`
   - `current/gex-signal-tapereader.user.js` (full file, not summary) — understand the code architecture, layer structure, key functions (render, runMachine, extractWalls, trendVerdict, accumData, recorder layer)
   - `session-state/latest-resume-note.md`
   - `changelog/CHANGELOG.md`
   - `design/architecture-design.md`
   - Skylit platform docs (at minimum): `skylit-docs/core-concepts.md`, `how-to-read-and-use-heatseeker.md`, `intro-to-gamma.md` — understand the 5-step framework, magnet concept, Pika/Barney polarity, node lifecycle
2. If the user supplied external code from Claude or another assistant, treat that pasted code as the incoming update payload and reconcile it against the project files before continuing.
3. Explicitly confirm:
   - which canonical files were read (including userscript + Skylit docs)
   - baseline/version from `current/gex-signal-tapereader.user.js`
   - code architecture understanding (layers, render flow, key algorithms)
   - current approved patch state from the canonical notes
   - next concrete step
4. If any canonical file was not read, say the load is incomplete and stop.
5. Continue only from the next concrete step in the resume note, unless the user is explicitly doing an external sync/update.
6. Make only the requested change or the next roadmap step.
7. Save outputs back into AI Drive.

## Standard outputs after any coding session
Update all relevant files:
- overwrite `current/gex-signal-tapereader.user.js` with the newest canonical version
- save a dated snapshot in `releases/` if the change is meaningful
- update `session-state/latest-resume-note.md`
- append a factual entry to `changelog/CHANGELOG.md`
- update `master-spec.md` when project rules/state/architecture changed
- update `teaching-spec.md` when doctrine/lexicon mapping changed
- create or refresh `design/architecture-design.md` if architecture/design understanding changed or if the file is missing
- save probe outputs to `probes/` when useful

## If code is requested
- Default to giving the full deploy-ready code inline in one copy/paste block unless the user explicitly asks for a URL.
- If the file is too large to fit safely in one block, use a clean sequential split and label the continuation clearly.
- Never split one logical section across disconnected blocks.
- Smoke-test risky pure logic before asking for deployment.
- Verify post-change behavior by probe or visible panel evidence.

## Current default target
Until the session-state note says otherwise, the active build target is:
**v9.1 rich recording, starting with time-unit reconciliation.**
