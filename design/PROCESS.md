# PROCESS — the HOW: the machinery that turns data into the READ, and how it improves itself

> Companion to `design/PURPOSE.md` (the WHAT). Operator, 2026-09-03: *"i need you to ensure you never
> forget my objective (the what) and the machinery and process that is supposed to do the analysis,
> testing and feedback into the application to help me reach my objective (the how). make sure you
> update it wherever, so future contexts are aware of it. I also need you to be able to tighten and
> harden the machinery and process in order to improve it over time."*
>
> Registered in `skills/gex/SKILL.md` (1a-00b) and pinned by `test_process.js`. A context that skips
> it fails the suite. **Read PURPOSE first, then this, before proposing, prioritising or cutting anything.**

## 0 · The WHAT, in one line (from PURPOSE.md)

Find the day's HOD and LOD early enough to trade the move between them; secondarily find the
pullback turning points that resume a trend. The mechanism: **a gamma node deflects price, and the
deflection is the turning point.** Everything below exists to measure that claim and to say, at the
tap, what the measurements support — with n, or "unmeasured".

## 1 · The loop, stage by stage

```
 ①  RECORD   the panel records every bar's book and price (snaps), every tap (defl ledger), every
             scored feature (feat) and what he asked to be tracked (requests)          ← the browser
 ②  EXPORT   Save writes data/<day>.json — the day, the book, the ES bars, the requests   ← the browser
 ③  PUSH     push-data.bat / the installer push it to GitHub (the cloud cannot push)     ← his machine
 ④  NIGHTLY  tools/nightly/run.py reads every day file + the register:
             verdicts per hypothesis (read ONCE at minN, sessions from the register date),
             refreshes the tables (SWEEPS.json, SWEEPS-BOOK.json, BASERATES.json),
             copies TRACK requests into learning/requests.json, writes learning/log/<day>.json  ← the cloud
 ⑤  REVIEW   the LLM reads the log, the tables, the requests and the day file; turns a READ into a
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

The three tabs are the loop's three faces. **Dashboard = act** (what the tables support right now).
**Analysis = ask and read** (the registry: every study, its status, its result with n; the TRACK
field). **Testing = trust and promote** (the register, the gate, what the ladder renders and why, the
record, the nightly, the suite).

## 2 · The files, and who writes each

| file | written by | read by |
|---|---|---|
| `data/<day>.json` | the panel (Save) | the nightly, the studies, the book corpus |
| `learning/studies.json` | `tools/studies-seed.py` (edited by the review — never the JSON by hand) | Analysis, Testing, the mockups |
| `learning/register.json` | the review (append only; a row is never edited after it is written) | Testing ①, the nightly, `PREREG_SEED` pinned equal |
| `learning/requests.json` | the nightly (`ingest_requests`) | the review |
| `learning/log/<day>.json` | the nightly | Testing ⑤, the READ's register line |
| `learning/suite.json` | `tools/run-tests.sh` | Testing ⑥ |
| `learning/rules.json` | the weekly learning run | the ladder (tiers), Testing ③ |
| `data/es-1min/SWEEPS.json` | `tools/study-sweeps.py` (nightly) | Analysis H2, THE READ |
| `data/es-1min/SWEEPS-BOOK.json` | `tools/study-sweeps-book.py` (nightly) | Analysis H2, THE READ's node clause, H6 |
| `data/es-1min/BASERATES.json` | `tools/study-hodlod.py` | ⓪a DAY table (E row), Analysis H1 |
| `skylit-docs/FINDINGS.md` | the review | every context (the measured facts, F-1 … F-16) |

**Rule: a file the cloud writes exists only when the installer carries it.** `test_installer_manifest.js`
pins the builder's `--list` against every `pipeFetch` path in the panel.

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
| after an install, PROBE THE LIVE PANEL, never assume | the manifest bug was found only by reading his tab | the ⚙ Architecture tab (the loop as live status, v15.59) |

## 4 · What is thin, what is open, what is next — honestly

- The book corpus is **9 sessions**; every cell thin. It grows one session per export. H6 reads at 40.
- **94 studies are OPEN on the TAP record** (v15.61): the per-tap fields (trinity, gatekeeper ratio,
  growth into the tap, node state, both zones, extent, `wasSessionExtreme`) are not recorded yet.
- The READ is **not yet scored**: what it said about a sweep is not written to the day file and the
  nightly does not check whether the candidate printed the extreme. That is stage ⑪ and it is the
  next hardening step, because a READ that is never scored cannot improve.
- Only rules with an earned tier render a rate; **0 of 28 rules have earned one** — so the ladder's
  grades still print "gated / thin". That is correct, and it will stay correct until the record fills.

## 5 · How to tighten and harden it (the standing backlog, in order)

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
4. **The nightly reads one READ NEXT study per night** and writes its result back into the registry
   (the registry as the nightly's output, not only its input).
5. **TRACK → DRAFT study** as a nightly step, so a request appears on the Analysis tab the next
   morning as a draft row with a proposed measure, before the review words it.
6. **The face manifest** (`design/DASHBOARD-INVENTORY.md` §3): every dashboard element names the
   study its number comes from, in `studies.json`, pinned by a test — a field without a study is
   marked DESCRIPTIVE on the face or removed.
7. **A shipped-artifact test**: decode the .bat, load the userscript from the payload (not the
   working tree), run the smoke test on it — the suite reads the tree; the operator runs the payload.
8. **Data-quality checks on the face**: courier age, ratio drift, gaps in the ES bars, the book's
   age — a store that is stale says so beside the number it feeds.

## 6 · The standing process constraints (operator-mandated)

One install file per build, with the Tampermonkey links pasted as text; wait ~5 min, click the
link, reload. `✅ SAVE DONE` naming the files saved. No PowerShell in installers. Chat history
regenerated last. Every new assertion mutation-tested. "Do not tune a parameter to make a number
look good." Cloud push is policy-denied — report, do not retry. Do not touch the integrations
(IRT export, Yahoo courier, InsiderFinance). Dark-pool lifecycle archived (reader kept), PiP
archived (window pop-out kept), the ladder stays on top of ⓪a.
