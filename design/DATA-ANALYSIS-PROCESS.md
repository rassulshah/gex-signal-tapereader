# THE DATA ANALYSIS PROCESS — the one process this application runs, named 2026-09-04

**Operator, 2026-09-04, his words.** *"the idea is to have a trading decision support system that is data driven. I want
to have top quality insight and decision support. In order to do that we are building out data capture, analysis testing
back to dashboard for everything that is displayed on the dashboard including hod lod time, nodes, setups, directional
prediction, reads and more."* — *"the entire data, analysis and testing process results in learning and it is from the
learning that can know something and make a decision based on what you see on the dashboard."* — *"my expectation from
now on is to just click on the save button once a day probably eod, and from that point on you take over from data,
analysis, testing, learning all the way to the Rec tab, which is where we will discuss what to implement as needed."*
— *"lets call the process Data Analysis process to keep it simple."* — *"i want to fine tune everything so that we
don't have to play around changing the structure that we are deciding on now and that is solid."*

⚠ **THIS FILE IS THE STRUCTURE.** `test_data_analysis_process.js` fails the suite if any of the seven links below leaves
this file, the panel, or `design/PROCESS.md`; the `load gex` procedure reads it before anything else; the ⚙ Architecture
tab names it. Change it only with the operator, and record why in `session-state/LESSONS.md`.

---

## 1 · The seven links

Every number the dashboard shows travels this chain, in this order, and **one definition travels all seven** — the stamp
(what the face sees at a node: the King by book, a pika / barney stack, a rug, NEW, growth, polarity) is computed live for
the read, recorded at the tap, counted by the nightly, tested by the register, kept as knowledge, recommended, shown,
and scored. Two stages that use two definitions of the same thing produce numbers that mean nothing to each other.

| link | what happens | who | writes | tab |
|---|---|---|---|---|
| **1 · CAPTURE** | every bar's book (the whole tape, every market), every tap with its stamp, every read the face shows with its inputs, his items and requests — and the day file itself, written by the panel after the close (v15.71) | the panel, live | `data/<day>.json` · `data/tape/<day>/` · IndexedDB | Dashboard |
| **2 · ANALYSIS** | every tap counted by class against the three outcomes — held (10 bars) · TURN (the node was the session's HOD / LOD) · RESUME (the trend went on to a new extreme) — with n and a Wilson bound; every study whose number the count can answer gets it | the nightly, on his machine, within ~10 minutes of the day file (the panel's own write after the close, or his 💾) | `learning/log/<day>.json` · `results.json` · `studies.json` | Analysis · Testing ⑦ |
| **3 · TESTING** | a claim written BEFORE the data it will be judged on (predict + refuteIf), read ONCE at its minimum n, on sessions after its date, against the base rate and a shuffle null; the gate — can the scorer fail | the nightly (the check every night, the read once) · the review (writes the claims; the machine drafts them from reads that clear the bar) | `learning/register.json` · the log's verdicts | Testing ① ② |
| **4 · LEARNING** | what survived: the rules with the record's verdict beside them (agrees · contradicts · thin · not measured), the taught examples re-checked, the blind calls, the gauge | the nightly (the verdicts) · the review and the operator (the rules, the teaching) | `learning/deflections/examples.json` · `LEARNING.md` | Learn |
| **5 · REC** | proposals TO the operator — from the nightly (pre-registered conditions: a class clear of the base at n ≥ 15; a rule the record contradicts; a hypothesis cleared) and from the review — with their evidence; his ✓ / ✗ rides the next day file; the machine never changes a rule's status, the operator never has to write one | the nightly · the review · **the operator decides** | `learning/recommendations.json` · the day file's `reco` | Rec |
| **6 · DASHBOARD** | only what was approved reaches the face, with its knowledge degree and its n; a rule renders only from an earned tier that cleared the gate | a build (a rule: the file; a feature: code) | the face · `rules.json` | Dashboard |
| **7 · SCORE** | what the face said is recorded as it is shown and scored at the close; the reads' own accuracy becomes an Analysis row; a bad rule is refused by its own record and withdrawn on Rec | the panel (the record) · the nightly (the score) | `reads[]` · the log | Testing · Learn (the gauge) |

**The operator's one step:** none, since v15.71 — the panel writes the day itself after the close (15:01 CT, then every
ten minutes until the file is confirmed in the repo folder) and any earlier day it missed outside market hours (write-if-
absent); the 💾 remains as the override and as the one click Chrome may need for the folder permission after a reload
(*"automatically have the application trigger the save button instead of me clicking it … after market hours"*, 2026-09-04).
**The one stage that still waits for a session:** the review inside links 3–5 (turning a read into a claim, answering
items, writing findings) — and the process says so, on the ⚙ tab and here, rather than implying otherwise.

## 2 · The degrees of knowledge (every number on the face carries one)

| degree | meaning | may the face show it? |
|---|---|---|
| **confirmed** | tested out of sample (link 3 cleared), approved (link 5), scored since (link 7) | yes, with its n and since-date |
| **provisional** | counted (link 2) at n ≥ 15, not yet tested | on the Analysis tab; on the face only marked provisional |
| **doctrine** | Skylit's word, not yet measured here | as a label, never as a rate |
| **descriptive** | a fact about now, no claim (a price, a size, a distance) | yes, as a fact |

An element without a degree is a bug: `design/DASHBOARD-INVENTORY.md` is the element-by-element ledger.

## 3 · The rules the machinery enforces

1. A % is never rendered without its n (`RATE_MIN_N` = 15; `bareP`).
2. A scorer must be able to fail before its rate means anything (the gate, `featGated`).
3. A hypothesis is written with predict + refuteIf BEFORE the data and read ONCE at its minimum n, on sessions after its date.
4. A first read is never a verdict — it becomes a register row and is read again on unseen sessions.
5. One definition end to end: the panel's `PAT_CLASSES` / `tapClasses` / `patternTable` and `tools/nightly/patterns.py` are pinned equal on one fixture (test_v1567 3l); `HYP_STUDY` in the panel and `results.py` likewise.
6. The machine writes the numbers; the review writes the questions; the operator approves the changes. Three writers, by id, and none erases another's field (`results.py`, `recommend.py`, the seeds' merges).
7. Every outcome and every machine condition is written in its file BEFORE the first number is read (`patterns.py`, `recommend.py`).
8. Every file the panel fetches rides the installer; every new assertion is mutation-tested; after an install, the live panel is probed.
9. Nothing on the face changes except through Rec — a ✓, then a build that marks the row IMPLEMENTED with its version.
10. Every market-specific number lives in `learning/markets.json`; adding a market is a configuration entry, never a restructure.

## 4 · The markets

The process is the same for every market; what differs is the sources (which ladder gives the gamma book, which bars
give price), the units (a tolerance is a number in the book's own points) and the doctrine priors — all in
`learning/markets.json`. SPY (the SPX complex) is **live**; NQ, GC, CL are **price only**: the companion couriers their
1-minute bars, so the HOD/LOD statistics can be built the way ES's were once a corpus accrues; the node ladder needs a
gamma book source (for gold: a GLD chain or a Skylit GLD ladder), to be confirmed before it is promised.

## 5 · The eight tabs — the final set

Dashboard (act) · 📊 Analysis (ask and read) · 🧪 Testing (trust and promote) · 📚 Learn (what we know) · 💡 Rec (what to
change, yours to approve) · ⚙ Architecture (the how) · 🗺 Roadmap (the plan) · 📌 Open Items (your issues and questions).
No ninth tab: a new need becomes a section of one of these.

## 6 · The files that carry the process, in one place

`design/DATA-ANALYSIS-PROCESS.md` (this) · `design/PURPOSE.md` (the what) · `design/PROCESS.md` (the loop, stage by stage,
as it runs) · `design/ARCHITECTURE.md` (the machinery, generated) · `design/DASHBOARD-INVENTORY.md` (the elements) ·
`learning/markets.json` (the markets) · `tools/nightly/run.py → patterns.py · results.py · recommend.py · tape.py · tick.py`
(the nightly) · `setup-gex-nightly.bat` (the task) · `tools/rec-seed.py` · `tools/studies-seed.py` · `tools/learn-seed.py`
(the review's seeds) · `learning/recommendations.json` (Rec).
