# PROCESS — the HOW: the machinery that turns data into the READ, and how it improves itself

> Companion to `design/PURPOSE.md` (the WHAT). Operator, 2026-09-03: *"i need you to ensure you never
> forget my objective (the what) and the machinery and process that is supposed to do the analysis,
> testing and feedback into the application to help me reach my objective (the how). make sure you
> update it wherever, so future contexts are aware of it. I also need you to be able to tighten and
> harden the machinery and process in order to improve it over time."*
>
> Registered in `skills/gex/SKILL.md` (1a-00b) and pinned by `test_process.js`. A context that skips
> it fails the suite. **Read PURPOSE first, then this, before proposing, prioritising or cutting anything.**

> **THE PROCESS HAS A NAME: the Data Analysis process — `design/DATA-ANALYSIS-PROCESS.md` (2026-09-04).** Its seven links —
> CAPTURE → ANALYSIS → TESTING → LEARNING → REC → DASHBOARD → SCORE — are the structure; the eleven stages below are how
> the machinery runs them, night by night. Since v15.71 there is no manual step at the close — the panel writes the day
> itself (after the close, and any missed day outside market hours); the 💾 is the override. His ✓ on the Rec tab is the
> only path to the face.

> **The complete architecture — every component, integration (Skylit · InsiderFinance · Yahoo · ForexFactory · GitHub),
> the daily HOD/LOD statistics pipeline and every storage key — is `design/ARCHITECTURE.md`** (v15.67, generated from
> `tools/plan-seed.py`, the same data the ⚙ Architecture tab renders as ⑥–⑨). This file is the LOOP.

## 0 · The WHAT, in one line (from PURPOSE.md)

Find the day's HOD and LOD early enough to trade the move between them; secondarily find the
pullback turning points that resume a trend. The mechanism: **a gamma node deflects price, and the
deflection is the turning point.** Everything below exists to measure that claim and to say, at the
tap, what the measurements support — with n, or "unmeasured".

## 1 · The loop, stage by stage

```
 ①  RECORD   the panel records every bar's book and price (snaps), every tap (defl ledger), every
             scored feature (feat) and what he asked to be tracked (requests)          ← the browser
 ②  EXPORT   (v15.71) THE PANEL WRITES THE DAY ITSELF — rule 1+2: after the close (15:01 CT and
             later, weekdays, no upper bound) any tick that finds the day not confirmed in the repo
             folder writes it and retries every 10 minutes until it lands; rule 3: outside market
             hours (before 08:30, after 15:00, weekends) every earlier day still in IndexedDB with
             bars and no file is written, WRITE-IF-ABSENT, and marked once (kv dayWritten:<day>).
             It writes data/<day>.json AND (v15.66) the tape — data/tape/<day>/SPXW·SPY·QQQ·VIX.json,
             the whole book every bar — into the folder picked once with 📁 (pick the DATA folder).
             SAVED means confirmed in that folder and nothing else: the silent download fallback is
             gone. A timer may ASK Chrome for the grant, never REQUEST it (v14.53), so when the grant
             is missing the footer's 💾 turns into DUE and his one click carries the permission inside
             the gesture — where Chrome 122+ offers "Allow on every visit"; chosen once, no rule ever
             needs him again. The 💾 remains as the override. A day with no recorded bars is never
             written on any path (the 08-29 / 08-30 weekend files were the old auto-export's).   ← the browser
 ③  PUSH     the GEX sync task (tools/gex-sync.bat, a Windows task every 2 minutes, installed
             2026-09-03 by setup-gex-sync.bat) commits and pushes ANYTHING new in the repo: the
             day file, the review's files written over the desktop bridge, a Drive drop. The old
             daily "data: daily export" task still runs at ~15:30. The cloud can fetch, never push  ← his machine
 ④  NIGHTLY  (v15.68) HIS MACHINE — the "GEX nightly" Windows task (setup-gex-nightly.bat, run once):
             every 10 minutes, hidden, tools\gex-nightly.bat → tools/nightly/tick.py, which runs the nightly
             ONLY when the newest data/<day>.json is newer than learning/log/<day>.json — i.e. within ~10
             minutes of the day file's write — the panel's own after the close, or his 💾 — once (a
             second write earns one more run; nothing new → exit 3, silent).
             TRIGGER: the day file's mtime. If the task is not installed or python is missing, this stage
             does NOT happen and the ⚙ tab's NIGHTLY box stays red — it never silently waits for a session.
             tools/nightly/run.py, in this order (v15.90): the futures corpus — every day file's couriered
             minutes → data/futures/<MK>/<day>.csv (RTH) and <day>-night.csv (17:00 → 08:27), then ES / NQ
             BASERATES rebuilt (study-hodlod) and the sweep tables rebuilt from the vendor file + every CSV
             (study-sweeps → SWEEPS.json, appending like the base rates; SWEEPS-BOOK.json from the day files);
             the record counted → learning/coverage.json (every day file's signals, the corpora, and every
             corpus the registry names: have · per-session rate); every day file + the register: verdicts per
             hypothesis (read ONCE at minN, sessions from the register date; H5 by its join; a withdrawn row
             said, never counted), the pattern table (patterns.py), the tape coverage (tape.py), the feature
             outcomes (dir.kingRoll, gateHour), TRACK requests and items copied, learning/log/<day>.json
             written (ranOn: his machine / cloud) — and then THE REGISTRY: results.py writes every study
             whose number the log can answer (result · status · by:'nightly' · asOf), composes the scripted
             rows' sentences from SWEEPS.json and the feature outcomes, and sets WAITING (have / need / an
             ETA) or READY (data on hand, no reader) on every other row; then Rec (recommend.py: the machine
             rows, his ✓ / ✗ from the day files). The sync task pushes it all within two minutes.
             The cloud runs the same script when I am in a session (over the desktop bridge, or from GitHub).
 ⑤  REVIEW   Claude reads the log, the tables, the requests and the day file; turns a READ into a
             register row (predict + refuteIf fixed BEFORE the next session), a request into a study
             row (studies.json, req:<id>), a refuted rule into a retirement; writes FINDINGS      ← the cloud
 ⑥  REGISTRY studies.json (subjects → subsections → studies, status + result WITH n) and
             register.json (hypotheses) are the two files the tabs render                          ← the cloud
 ⑦  BUILD    code + tests (mutation-tested) + docs + chat history; the installer carries EVERY file
             the panel fetches (test_installer_manifest.js); one install file, with the links       ← the cloud
 ⑧  INSTALL  he runs the .bat (pushes) and clicks the Tampermonkey links; the panel fetches the
             registry, the register, the tables, the log, the suite stamp                            ← his machine
 ⑨  GATE     featGated: a feature whose rate does not move between its predicted bands cannot
             promote and its rate cannot render                                                       ← the browser
 ⑩  DASHBOARD the ladder renders a rate only from an earned tier that cleared the gate; THE READ
             FROM THE STATS turns today's tape into sentences from the tables, every rate with n     ← the browser
 ⑪  SCORE    tomorrow's export carries what the READ said and what printed — the nightly scores it
             (the READ is a feature like any other; a READ that cannot be wrong is not a read)        ← next day
```

The tabs are the loop's faces. **Dashboard = act** (what the tables support right now). **🗄 Data = count**
(v15.89: what the record holds — every session on GitHub, the sources live, the browser's stores, what the
studies wait for with have / need / an ETA, the gaps and what Yahoo can fill; read-only). **Analysis = ask
and read** (the registry: every study, its status — the machine's since v15.90 — its result with n; the
TRACK field). **Testing = trust and promote** (the register, the gate, what the ladder renders and why, the
record, the nightly, the suite). **Learn = teach** (v15.62): his screenshots become examples checked
against the record, the examples become rules, the rules become the deflection scorer's features
(v15.63) — and the gauge says how good the identification has become, on blind reads only. **Rec = decide**
(v15.70): every change to the face passes his ✓ there.

## 2 · The files, and who writes each

| file | written by | read by |
|---|---|---|
| `data/<day>.json` | the panel (Save) | the nightly, the studies, the book corpus |
| `learning/studies.json` | `tools/studies-seed.py` (edited by the review — never the JSON by hand) | Analysis, Testing, the mockups |
| `learning/register.json` | the review (append only; a row is never edited after it is written) | Testing ①, the nightly, `PREREG_SEED` pinned equal |
| `learning/requests.json` | the nightly (`ingest_requests`) | the review |
| `learning/items.json` | the nightly (`ingest_items`); the review writes the answers | 📌 Open Items, 🗺 Roadmap (enhancements) |
| `learning/deflections/examples.json` + `LEARNING.md` + `img/` | `tools/learn-seed.py` (the review adds each taught screenshot, checked with `tools/node-lookup.py`) | 📚 Learn (the tab), every context on load (the doc) |
| `learning/log/<day>.json` | the nightly | Testing ⑤, the READ's register line |
| `learning/suite.json` | `tools/run-tests.sh` | Testing ⑥ |
| `learning/rules.json` | the weekly learning run | the ladder (tiers), Testing ③ |
| `data/es-1min/SWEEPS.json` | `tools/study-sweeps.py` (nightly) | Analysis H2, THE READ |
| `data/es-1min/SWEEPS-BOOK.json` | `tools/study-sweeps-book.py` (nightly) | Analysis H2, THE READ's node clause, H6 |
| `data/es-1min/BASERATES.json` | `tools/study-hodlod.py` | ⓪a DAY table (E row), Analysis H1 |
| `skylit-docs/FINDINGS.md` | the review | every context (the measured facts, F-1 … F-16) |

**Rule: a file the cloud writes exists only when it reaches his machine.** Two roads: the installer
(`test_installer_manifest.js` pins the builder's `--list` against every `pipeFetch` path in the panel) for
builds, and — since 2026-09-03 — the desktop bridge for the nightly's files (written straight into
`C:\Dev\gex-signal-tapereader\…`, pushed by the GEX sync task). The bridge needs his desktop app open and
the folder approved for the session (`device_request_folder_access`, one prompt); when it is not, the files
wait for the next installer. Either way the cloud's git history never reaches GitHub — only files do.

## 3 · The rules the machinery enforces (and the test that enforces each)

| rule | why | enforced by |
|---|---|---|
| a % is never rendered without its n | a rate without its n is a feeling | `test_analysis_tabs 5a/5b`, `bareP` in every v155x test, `RATE_MIN_N` |
| a scorer must be able to fail before its rate means anything | lodhod read 100% in every cell (F-11) | `featGated`, `test_v1554 7*` |
| a hypothesis is written with predict + refuteIf BEFORE the data, read ONCE at minN | a search is not a test | `register.json` ↔ `PREREG_SEED` pin (`test_v1554 8b`), the nightly's `judge` |
| a first read is never a verdict — it becomes a register row and is read again on unseen sessions | 79 cells produce two +9pp by luck | H7's `since`, the ledger line on every table |
| every level named live must be made the way the corpus made it | ONH/ONL were a pre-market stub for a month (v15.56) | `overnightHL().full`, companion v1.18, `test_v1556 1*` |
| every new assertion is mutation-tested | an assertion that never fails buys false confidence | `BUILD-CHECKLIST §2`, the mutation pass in every v155x build |
| every file the panel fetches rides the installer | the cloud cannot push | `test_installer_manifest.js` |
| the suite, the resume note, the changelog, the lessons and the chat history move with the code | a stale note cost seven builds | `test_savedone`, `test_chat_history`, `test_lessons`, `test_recordcurrent` |
| after an install, PROBE THE LIVE PANEL, never assume | the manifest bug was found only by reading his tab; the three-bar ladder (v15.61) only by reading his tape | the ⚙ Architecture tab (the loop as live status, v15.59) |
| what the ladder DRAWS and what the engine COUNTS are two lists | a display floor must never change what is recorded or scored | `ladderSubPiles` feeds `RAILPS_DRAW` only; `test_v1561 1f/1g` |
| the mockup's look is the panel's look — one stylesheet, one skeleton | the tabs were re-typed "as mocked" twice and drifted twice | `tools/panel-css.py` == `PANEL_CSS`; the skeleton pin `test_v1562 2e` |
| a deflection is never called from the picture alone; the gauge cannot flatter | a remembered pattern is not a measured one | `tools/node-lookup.py`; identify = Wilson lower bound of BLIND reads (`test_v1562 3f/3g/3l`) |

## 4 · What is thin, what is open, what is next — honestly (as of 2026-09-09, v15.90; the Data tab and the
## registry carry the live numbers — this list is the shape, not the count)

- **104 of 184 studies WAIT on the tap record** (v15.91, agreed, not built): the per-tap fields (book · rank ·
  signed % · trinity, gatekeeper ratio, growth into the tap, node state, both zones, extent, `wasSessionExtreme`,
  the liquidity level) are not recorded. Meanwhile 12 of them read THIN on the ±0.50 wobble ledger.
- The book corpus is **13 sessions** (needs 30); it grows one session per export. H6 accrues about one
  at-node event per three sessions — it reads at 40, months away; the tap record is the faster road.
- The register: H1–H4 accrue about one event a session (5–7 weeks to their minN); **H5 is judged by its join
  and reads 0 of 3 extremes joined** (the day's extreme is the open more often than not — the tap record);
  **H7 is withdrawn** (F-23 removed its premise); H8 / H9 began 09-08; H10 is on pace (26 of 60).
- **15 studies are READY** — data on hand, no reader written (the King-roll rows, the gatekeeper rows, the
  price corpus's clock and weekday questions): the review writes readers; the nightly then owns them.
- The READ is **scored only for the HOD/LOD cells** (H10, close-scored rows); the sweep candidate the READ
  names is not yet scored (stage ⑪, v15.94).
- Only rules with an earned tier render a rate; **no rule has earned one** — the ladder's grades still
  print "gated / thin". Correct, and it stays correct until the record fills.
- **His ✓ / ✗ on Rec has never been exercised live** (approved 0 · declined 0 after 24 implemented rows —
  every decision was made in chat and the row marked at build). The mechanism is tested; the first real
  click (R-33) proves the day-file → nightly → status path on his machine.

## 5 · How to tighten and harden it (the standing backlog, in order)

0. **The deflection candidate score** (v15.63): the L-rules as features in the register, scored by the
   deflection ledger's outcomes — the gauge's predict part. *Definition of done:* 30 scored calls and a
   Wilson lower bound on the Learn tab.
1. **Score the READ** (⑪): write each sweep line's verdict candidate to the day file; the nightly
   labels it at the close; Testing ③ shows the READ's own hit rate with n. *Definition of done:* a
   cell "READ said candidate → printed the extreme x% (n)" on Testing.
2. **The ⚙ Architecture tab (the Process tab, shipped v15.59 with the 🗺 Roadmap tab)** — the loop as live status, not prose: each stage green/amber/red with its
   evidence (file fetched? age? 404? companion full? last nightly? installer manifest count? suite
   stamp? requests pending?). It is the probe of §3 made permanent, and it is where the WHAT and the
   HOW paragraph lives inside the app.
3. **One definitions file** (`learning/definitions.json`: reclaim window, bins, the tap zone, the
   fresh-low control) read by the Python studies AND the panel, pinned equal by a test — today the
   same constants are typed in two languages.
4. **The nightly reads one READY study per night** and writes its result back into the registry — half
   built in v15.90 (the registry IS the nightly's output: statuses, scripted sentences, feature reads); the
   half that remains is the READERS for the 15 READY rows (a study script each, then the machine owns it).
5. **TRACK → DRAFT study** as a nightly step, so a request appears on the Analysis tab the next
   morning as a draft row with a proposed measure, before the review words it.
6. **The face manifest** (`design/DASHBOARD-INVENTORY.md` §3): every dashboard element names the
   study its number comes from, in `studies.json`, pinned by a test — a field without a study is
   marked DESCRIPTIVE on the face or removed.
7. **A shipped-artifact test**: decode the .bat, load the userscript from the payload (not the
   working tree), run the smoke test on it — the suite reads the tree; the operator runs the payload.
8. **Data-quality checks on the face**: courier age, ratio drift, gaps in the ES bars, the book's
   age — a store that is stale says so beside the number it feeds. (v15.89: the 🗄 Data tab's ② and ③
   carry the ages and the stores; the per-number badge on the face is still open.)
9. (v15.90) **A reader per READY row**, written by the review, owned by the nightly — the backlog the
   registry itself now lists (READY = data on hand, no reader).

## 5b · The end of day, as it runs now (2026-09-04, v15.71: no click — the panel saves the day itself)

```
 15:01 CT   the panel writes the day itself (rule 1+2; retried every 10 min until confirmed; the 💾 is the override)
                                                               → data/<day>.json + data/tape/<day>/ in the repo folder
 +2 min     the GEX sync task commits + pushes them              → GitHub
 ≤ +10 min  the GEX nightly task (his machine) sees the day file newer than its log → tick.py → run.py
            → learning/log/<day>.json (ranOn 'his machine'), the pattern table, the tables, and the
            registry: results.py → learning/results.json + studies.json (the Analysis tab's rows)
 +2 min     the sync pushes those                                → GitHub
 ≤ +10 min  the panel's pipeline check re-fetches the log and the registry (or a reload does) — the
            ⚙ tab's NIGHTLY box says "ran on his machine", Analysis rows read "by the nightly, <day>"
 a session  Claude: the review — a READ into a hypothesis, items answered, FINDINGS, the seed's questions
            (studies-seed.py merges results.json: the machine's numbers survive) → over the bridge → pushed
```
His words: *"from now on i will just click the save button end of day and you can take care of everything
else."*, (2026-09-04) *"i envision clicking on the save, the data getting saved and the analysis occurring
and the analysis tab being updated."*, and then (2026-09-04, v15.71) *"the next step is to automatically have
the application trigger the save button instead of me clicking it … instead of 5pm can you just modify so it
is after market hours."* A day the tab was closed before 15:01 is written the next time the panel is open
outside market hours (rule 3) — the next morning, pre-market, so the nightly has run before the open. The
mechanical half now runs without a session OR a click; the review is the one
piece that still waits for one (a scheduled cloud session, bound to his computer, is the candidate). Today's stray: 💾 with the REPO root picked writes
`<root>\<day>.json` — pick the `data` folder (v15.63 makes the panel write into `data\` under either).

## 6 · The standing process constraints (operator-mandated)

One install file per build, with the Tampermonkey links pasted as text; wait ~5 min, click the
link, reload. `✅ SAVE DONE` naming the files saved. No PowerShell in installers. Chat history
regenerated last. Every new assertion mutation-tested. "Do not tune a parameter to make a number
look good." Cloud push is policy-denied — report, do not retry. Do not touch the integrations
(IRT export, Yahoo courier, InsiderFinance). Dark-pool lifecycle archived (reader kept), PiP
archived (window pop-out kept), the ladder stays on top of ⓪a.
