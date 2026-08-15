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

Goal: full project context before touching anything. Sources in priority order:

1. **Attached files / zip** in this conversation (uploads directory) — use if present.
2. **User's machine via device bridge** (Claude desktop app connected):
   `C:\Dev\gex-signal-tapereader` — stage needed files into the workspace.
3. **Google Drive connector** (GEX-Tapereader folder). GOTCHAS (hard-won):
   - `read_file_content` returns MARKDOWN-ESCAPED, TRUNCATED text — NEVER use it
     for source files. Use `download_file_content` + base64 decode.
   - Downloads >10 MB refused (whole-project zip won't pull; use the folder tree).
   - `update_file` changes METADATA ONLY. In-place update pattern:
     create_file new → rename old to *-archive → rename new into canonical slot.

Read, in order:
1. `session-state/latest-resume-note.md` — the single source of "where we are":
   locked scope for the next version, open threads, standing workflow agreements.
2. `changelog/CHANGELOG.md` — head only (latest 2–3 entries).
3. `master-spec.md` + `teaching-spec.md` — rules (file-shape rule 2.4 etc.).
4. `current/gex-signal-tapereader.user.js` — the live baseline. VERIFY before
   trusting: exactly one `render()`, final line `})();`, byte count matches the
   versioned copy. If any check fails the copy is corrupt — re-pull.
5. `design/` mockups referenced by the resume note (build specs).
6. `gex/SKYLIT-LLM-GROUNDING.md` — Skylit domain vocabulary (King, ±γ, nodes,
   eVA, succession, episodes/magnet frame).

Then report: baseline version, next build target, open threads — and WAIT for
the user's direction. Do not start building on load.

## SAVE ("save" / "save gex")

Persist everything a fresh session needs. The cloud workspace is EPHEMERAL —
saving means getting files to the USER (SendUserFile) and/or their repo/Drive.

1. Update `session-state/latest-resume-note.md` — rewrite fully (not append):
   baseline version + deploy status, one-paragraph session summary, LOCKED
   decisions for next build, background/data-layer scope, shelved items,
   OPEN THREADS (exactly where discussion stopped, in the user's "1 by 1" style),
   mockups list, standing workflow agreements, any live-DOM/parser findings.
2. Prepend a CHANGELOG entry (mark `PLANNED / NOT YET BUILT` if no code shipped).
3. Zip: `session-state/` + `changelog/CHANGELOG.md` + `design/*.html` (+ code and
   `install.bat` if a build shipped — install.bat copies files to repo root to
   avoid the nested-extraction trap; make the Tampermonkey raw URL a clickable link).
4. SendUserFile the zip with commit instructions (git, not Drive, holds the
   changelog — user rule). Deliverables also individually if user will read them.
5. Tell the user the exact resume phrase: open a new window, attach the zip or
   connect the desktop app, and say "load gex".

## STANDING PROJECT RULES (apply always)

- Before coding: ASK first (user may have more fixes) and show MOCKUPS for review.
- Discuss one element at a time; confirm before moving on.
- Nodes are MAGNETS (attract/pull, repel/push): every indicator is either
  DESCRIPTIVE (what the field is doing) or PREDICTIVE (⚖ hand-set / 📊 measured,
  nightly-scored, graduates at n≥20). Nothing vague in between.
- Hover/tooltip explanations on every element, written for a new reader.
- Every deploy ships with step-by-step instructions.
