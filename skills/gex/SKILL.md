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
1a. **`session-state/PROJECT-CONSTANTS.md` — THE LANDMINES AND THE USER'S STANDING RULES.** Nine
   failure patterns that explain nearly every bug here, plus the process failure that ended the
   2026-08-24 session. ⚠ **The single most-violated rule in this project is ONE AT A TIME: discuss
   exactly ONE element per message, state its fix, ask, STOP.** Not optional, and not a style note —
   breaking it has cost more rework than any bug. Also standing: never build without asking, always
   show mockups first, never volunteer adjacent work.

1b. **`session-state/DECISIONS.md` — WHY the panel is the way it is.** Every entry is a decision a
   later context would otherwise re-litigate or silently undo, INCLUDING several I got wrong and
   corrected. Read it before proposing any change to the FRAME section, the band, or the pile
   sources — at least three "improvements" in this project's history were re-treads of a decision
   already made and recorded.
1c. **`session-state/INSIDERFINANCE.md` — THE SECOND BOOK.** ⚠ **The panel stands on TWO data
   sources and confusing them has caused four separate bugs.** Skylit (the Atlas page this
   userscript runs on) and InsiderFinance (a second site, read by a SECOND userscript,
   `current/gex-if-levels.user.js`). If you only understand Skylit you will make a fifth.
   The single most repeated error in this codebase is **comparing a number from one book, window
   or scale against a number from another** and declaring a discrepancy — it has produced a
   phantom "netGEX sign bug", a phantom "regime contradicts FLIP", a wrong 113x
   "incompatibility" verdict, and a wrong "the books disagree on the King" claim. **Establish
   which book, which expiry window and which scale a number is in BEFORE comparing it to
   anything.**
2. `changelog/CHANGELOG.md` — head only (latest 2–3 entries).
3. `master-spec.md` + `teaching-spec.md` — rules (file-shape rule 2.4 etc.).
4. **`current/gex-if-levels.user.js` — the InsiderFinance companion, FULL FILE.** It is short.
   `windows()` (dte0 / toFri / all, and the Friday roll), `levelsFor()`, `gexProf`,
   `gexProfCoverage`, `hdrText`/`hdrNum` and the `levelSane()` gate. Every number on ① FRAME
   except the regime chip originates here.
5. **`current/gex-signal-tapereader.user.js` — FULL FILE** (not summary). Understand:
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
   - **① FRAME specifically:** `emBand` / `emPiles` / `emPath` / `emRead` / `secFrame`. The
     comment block above `emPiles` states its source in bold — believe the code, and if a
     comment and the code disagree, FIX THE COMMENT IN THE SAME BUILD. One that claimed the
     wrong source survived eleven versions and misled two separate contexts.
5. **⚠️ THE DOCTRINE — NOT OPTIONAL, AND NOT A SKIM.** Two books, both mandatory:

   **5a. `SOURCE-OF-TRUTH.md` FIRST.** User mandate, 2026-08-13: the Skylit Academy is
   AUTHORITATIVE for all detector logic, terminology, regime rules and pattern definitions.
   **"No detector or UI label may use a term or rule the Academy does not support. Verify against
   `learn/` before building."** When the Academy and anything else disagree — patternpedia,
   core-concepts, older specs, prior code, or your own reasoning — **the Academy wins**.

   **5b. `skylit-docs/learn/00-INDEX.md` — READ THIS ONE PROPERLY.** It is the map of all 11
   Academy articles AND it carries a "KEY CALIBRATION TAKEAWAYS" section written for THIS project,
   listing which doctrine points are implemented and which are not. Its item 4 flagged REAL-vs-HEDGE
   (growth vs decay) as **"the biggest source of false patterns"** and unimplemented — a note that
   sat unread for twelve days while the panel shipped patterns built on single snapshots.

   **5c. All 11 Academy articles in `skylit-docs/learn/`.** They are 16–38 lines each; there is no
   excuse for summarising instead of reading:
   `charts-first` · `intro-to-gamma` · `reading-heatseeker` · `node-lifecycle` ·
   `rolling-floors-ceilings` · `air-pockets-velocity` · `gamma-regimes` · `heatseeker-patterns` ·
   `execution-doctrine` · `trinity-mode` · `graduate-from-trading-alerts`

   **5c-bis. `skylit-docs/FINDINGS.md` — OUR EVIDENCE, read alongside `learn/`, never merged into it.**
   Every doctrine article carries a banner pointing here. This file records where OUR measurements
   agree with the Academy, where they do not, and what is still unmeasured — with a status on every
   entry: OPEN / PROVISIONAL / CONFIRMED / SUPERSEDED. ⚠ A finding marked SUPERSEDED is kept, never
   deleted, so a later context does not rediscover a dead end. Before changing anything a finding
   covers, read the finding — it may already say the doctrine rule was too tight for our cadence.

   **5d. Supporting only** (valid where they do not contradict the Academy):
   `skylit-docs/core-concepts/core-concepts.md` (magnets, King margin 5–10 SPX, retest decay,
   price delivery, hedge nodes, air pockets, OPEX, Power Hour), `read-the-heatmap/`,
   `patternpedia/` (Rug, Gatekeeper, Whipsaw, Rainbow Road, Ten Commandments), `best-practices/`.

   **5e. THE SECOND BOOK — `session-state/INSIDERFINANCE.md`** (also listed at 1c). Skylit doctrine
   governs FLOW; IF governs STRUCTURE. A session that knows one book and not the other WILL mix
   scales and windows — that error has produced four separate phantom bugs.
6. DATA (from v10.44): `data/YYYY-MM-DD.json` — one file per session day. Read the coverage
   summary first (days · bars · symbols · fields-since); never run a study on missing fields.

Then report the config's 6 points:
- files loaded
- code architecture understanding (layers, render flow, key algorithms)
- platform understanding (5-step framework, magnets, node lifecycle)
- **doctrine STATUS** — for anything you are about to touch, say whether `skylit-docs/FINDINGS.md`
  marks it OPEN / PROVISIONAL / CONFIRMED / SUPERSEDED. "The Academy says X" is only half an answer
  if we have already measured X.
- **DOCTRINE CONSTANTS, RECITED FROM MEMORY** — this proves the docs were read rather than listed.
  State all of these; if you cannot, you have not loaded:
  · tap decay **80% / 66% / 33%** (fresh / tested / delivered) + DECAYING = quiet death
  · the two-question node gate: **is it FRESH? is it GROWING?** both yes or find another level
  · **REAL vs HEDGE** = grows (intent, target) vs fades (protection, fade)
  · rolling count rule: **1 migration = NOISE, 2 = SIGNAL, 3 = CONFIRMATION**; floor rolling UP is
    bullish, ceiling rolling DOWN is bearish; rolling ≠ breakout
  · **absolute value rules**, sign is secondary — polarity sets the CHARACTER of the interaction
    (+ = smooth/low-vol, − = wicky/violent/overshoot), NOT the strength of the pull
  · deflection zones **±5 SPX / ±0.50 SPY-QQQ**; King margin of interaction **5–10 SPX points**
  · air pocket = **PATHWAY not target** — trade through it, target the node on the far side
  · the loaded combo: **−gamma + air pocket + high rate of change + a trigger**
  · Trinity: **3/3 = A+ full size, 2/3 = size down, divergence = WAIT**
- current version (with the 3-spot @version integrity check)
- patch state (which phase is built/candidate/planned; test status)
- next concrete step

WAIT for the user's direction. Do not start building on load.

## ⚠️ PRE-BUILD DOCTRINE GATE (added 2026-08-25, after it was violated)

Before building or changing ANY detector, node rule, pattern, roll/lifecycle logic or UI label:

**Name the Academy file that bears on it and quote the rule** — so that whatever you do next is
deliberate rather than ignorant. Then state one of three things:

- **FOLLOWING** the doctrine rule, or
- **DEVIATING** from it, with what the data shows instead (numbers, dates) — this is LEGITIMATE, see
  the 2026-08-25 amendment in `SOURCE-OF-TRUTH.md`, or
- **NO DOCTRINE COVERS THIS** — say so and get the user's agreement before inventing.

⚠ THE GATE IS "DID YOU CHECK", NOT "DID YOU OBEY". Doctrine is the prior; the tape is the arbiter.
A measured event that does not fit a definition is real, and the definition is what is wrong —
detect it, display it with an honest confidence label, and record the outcome. Suppressing a finding
because it fails to match a document is how the system stops learning. Where there is no evidence
yet, doctrine is the default; where there is, evidence wins.

Why this gate exists: on 2026-08-25 a roll detector was designed from first principles, live-measured,
mutation-tested and SHIPPED — while `skylit-docs/learn/rolling-floors-ceilings.md` sat in the repo
stating the count rule (**1 = noise, 2 = signal, 3 = confirmation**) and defining rolling as the
LARGEST FLOOR and LARGEST CEILING migrating across updates. The shipped detector fires on a single
15-minute observation between arbitrary neighbouring strikes — i.e. it reports as signal exactly what
the doctrine calls noise. Every other step was done well, which is the point: rigour applied to an
unverified premise produces a confident wrong answer.

The same session also invented a six-stage node lifecycle while `node-lifecycle.md` defined four
stages on two orthogonal axes. **Check the doctrine before reasoning, not after.**

## SAVE — AUTOMATIC ON EVERY BUILD (not a command the user has to remember)

⚠️ **The user must never have to say "save".** Every build runs `tools/BUILD-CHECKLIST.md`, and a build
is NOT finished when the code works — it is finished when a fresh context could pick it up. If a build
shipped and `session-state/latest-resume-note.md` was not updated in the SAME COMMIT, the build is
incomplete. Record what was LEARNED this build — especially a wrong assumption and what corrected it —
not merely what changed. "save gex" remains valid as an explicit trigger, but it should be redundant.

Persist everything a fresh session needs. The cloud workspace is EPHEMERAL — saving means
getting files to the USER (SendUserFile) and into the git repo (source of truth), NOT Drive.

1. Update `session-state/latest-resume-note.md` — rewrite fully (not append): baseline version +
   deploy status, one-paragraph session summary, LOCKED decisions for next build, background/
   data-layer scope, shelved items, OPEN THREADS (exactly where discussion stopped, in the user's
   "1 by 1" style), mockups list, standing workflow agreements, any live-DOM/parser findings.
2. Prepend a CHANGELOG entry (mark `PLANNED / NOT YET BUILT` if no code shipped).
3. DELIVERY RULE (user-mandated 2026-08-15): ship ONE self-contained installer `.bat` — files
   embedded as base64 inside the .bat (no separate zip). ⚠️ **NO POWERSHELL ANYWHERE — Avast flags it
   (IDP.HELU.PSE88).** Decode with `more +<HDRLINES>` then `certutil -f -decode` then `tar -xzf`;
   `<HDRLINES>` must equal the `exit /b 0` line number and be recomputed whenever the header changes.
   Then xcopy into `C:\Dev\gex-signal-tapereader` and commit+push using a GIT-FINDER probe (git is not
   on the user's PATH). **The push is the only durable copy of the work — the cloud sandbox can reset
   mid-session and has.** User's only step: download, double-click.
4. SendUserFile the installer. Also send individually any file the user will READ (mockups render inline).
5. If code shipped: send **BOTH** Tampermonkey links whenever the companion changed, and say to
   **wait FIVE minutes** (`raw.githubusercontent.com` returns `cache-control: max-age=300`; clicking
   sooner offers *Reinstall* instead of *Update*, which looks exactly like a failed push) and then to
   **RELOAD the Atlas tab** — installing a userscript does not affect an already-open page.
   THREE install failures look identical from the user's side: (a) the installer did not push — clone and
   read `@version`; (b) the page was already open — reload; (c) the raw CDN is serving stale — check the
   `cache-control`/`age` headers.
6. Tell the user the resume phrase: new window, say "load gex".

## REVIEW ("nightly review" / scheduled nightly run)

Runs unattended after a trading session. Produces `review/YYYY-MM-DD.json` and gets it back into the repo.

**1. Read.** `git clone --depth 1 https://github.com/rassulshah/gex-signal-tapereader.git` (read works;
the cloud **cannot push** — the git proxy refuses to inject credentials for this repo. The installer .bat
pushes from the user's machine instead). Read `docs/LLM-NIGHTLY-BRIEF.md` (the contract), the newest
`data/*.json`, `learning/rules.json`, and up to the last 3 `review/*.json`.

**2. Analyse — honesty rules that matter more than the findings:**
- Report every factor with n, hit-rate, avg MFE/MAE **and its VOTE-DIRECTION SPLIT** (how many UP vs DOWN
  votes) **plus the day's baseline drift**. A one-directional factor on a trending day earns accuracy for
  free — flag it `1-way, not evidence`. This already fooled us once: 2026-08-11 structure voted DOWN 46/49
  on a down day and looked like 71% edge.
- State effective sample size. Overlapping forward windows mean effective n ≈ bars/10, NOT bars. 67 bars is
  ~6.7 independent observations — say so plainly.
- Calibration: is A > B > C monotone? If not, the fusion is wrong — surface it.
- If there is not enough data to conclude anything, SAY THAT. Never invent findings or numbers.
- Propose thresholds/weights; never apply them. Weights stay hand-set (⚖) until measured (📊, n>=20).
- Propose KILL-LIST additions (conditions to avoid), not just what works.

**3. Deliver — cascade, in order, stop at the first that succeeds:**
1. **Device bridge** (`mcp__remote-devices__*`, needs the Claude desktop app running): write
   `review/YYYY-MM-DD.json` directly into `C:\Dev\gex-signal-tapereader\review\`. The user's local
   "GEX data push" task then commits it.
2. **Google Drive** (connector, always available): create the file in Drive folder `GEX-review-inbox`.
   The user's local task moves it into the repo. Drive is a TRANSPORT ONLY here — the repo remains the
   single source of truth (see the git-first rule above).
3. **Chat**: SendUserFile the JSON.
Always report WHICH path was used, so the panel's `review` pipeline stage can be interpreted.

**4. Summarise** in 5-10 plain lines: what actually mattered, what is still unproven. Plus a one-line
"brief" the panel can show pre-open.

Descriptive only: never entries, stops, sizing, or trade recommendations.

## STATUS ("status" / "where are we" / "how complete is it")

Report completeness LAYER BY LAYER with an honest % and one sentence of what works vs what is
missing per layer (user-approved 2026-08-16): Sensing (L0–1) · Dashboard reader · Recording &
self-scoring · Analysis tab · Testing pipeline · Nightly review · Multi-symbol/Trinity. Close with
an overall one-liner. Never inflate; unverified-live code stays "candidate".

## HARD-WON RULES (each of these cost a real bug)

- **Ask of every number: which book, which window, which scale — and does the label say so.** Nearly every
  defect in this project has been a value displayed under a label that implied something else, with
  nothing throwing.
- **%King ranks at one instant; DOLLARS compare two moments.** A moving denominator cannot measure change.
- **Does it POINT or does it CONDITION?** Gamma, vanna and VIX term structure all condition — they belong
  in the regime line or a gate, never in a direction tally.
- **Walk the tree before concluding anything is missing.** A shallow scan reported their published metrics
  as absent and drove a decision to recompute what they might already publish.
- **Compare a level to the book it came from.** Scoring InsiderFinance levels against Skylit gamma made
  every level read as thin.
- **GREP BEFORE NAMING A FUNCTION.** Four collisions; `ifNum` shipped broken for nine releases because the
  later declaration silently won. `test_no_dupes.js` fails the build on a new one.
- **Keep the suite green.** 23 "known stale" failures once camouflaged two live bugs for months. When a
  deliberate change breaks a test, fix the test in the same commit or it becomes camouflage.
- **Verify against real data before proposing thresholds.** It has changed the answer more than once.
- **Use their values when they publish them; compute only what they do not.** And say which is which.

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

- Before coding: ASK first (user may have more fixes) and show MOCKUPS for review. Exception, agreed
  2026-08-20: an unambiguous bug report gets fixed without a round trip.
- **No useless text on the panel — there is no space.** If it is needed, it goes in a hover, and every
  hover opens with the QUESTION it answers.
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
