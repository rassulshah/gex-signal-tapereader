# THE END-TO-END WORKFLOW — from a 1-minute bar to a rule the panel applies

_design/ARCHITECTURE-E2E-WORKFLOW.md · written 2026-09-03 at v15.53 · **the spec for v15.54** · judged
against `design/PURPOSE.md`; supersedes `roadmap/SIMPLIFICATION-PLAN.md §9`_

> **One objective:** call the HOD, the LOD, and the pullback turn, using gamma nodes.
> **One loop:** collect → score → analyse → test → nightly review → reimplement → collect.
> This document names every store, every producer, every consumer, and — honestly — every joint
> that is broken at v15.53 and what v15.54 does about it.

```
              DATA                    ENGINES                   FACE            LEARNING
  ┌─────────────────────┐   ┌───────────────────────┐   ┌────────────┐   ┌────────────────────┐
  │ Skylit feeds (fetch)│──▶│ 0 FEED+TAPE           │   │            │   │ 5 RECORD           │
  │ Skylit ladder (DOM) │──▶│ 1 NODE LEDGER  ───────┼──▶│ the ladder │──▶│   snaps · feat ·   │
  │ React-fiber candles │──▶│ 2 STRUCTURE (rolls…)  │   │  MARK      │   │   defl · events    │
  │ ES 1-min courier ───┼──▶│ HOD/LOD (⓪a)  ────────┼──▶│ ⓪a verdict │──▶│ 5 SCORE at horizon │
  │ 284-session file ───┼──▶│   HLTAB (baked prior) │   │ callout    │   │ 5 AGGREGATE (episodes)
  │ InsiderFinance chain│──▶│ 3 DIRECTION           │   │ ledger     │   │ 5 GATE ⑤b ─────────┼──▶ face
  │ VIX · FF calendar   │──▶│ 4 SETUP (deflect/break)│  │ Testing ①② │   │ 5 PROMOTE          │
  └─────────────────────┘   └───────────────────────┘   └────────────┘   └─────────┬──────────┘
                                                                                   │ data/<day>.json
                                                              ┌────────────────────▼──────────┐
                                                              │ 6 NIGHTLY  harness.py          │
                                                              │   LLM proposes · harness disposes
                                                              │   → learning/log/<day>.json    │
                                                              │   → rules.json (weights/kills) │
                                                              └────────────────────┬──────────┘
                                                                                   │ read back
                                                              Analysis ④ REVIEW ◀──┘   rulesApply()
```

---

## 1 · THE DATA — every store, who writes it, who reads it

| store | where | written by | read by | survives |
|---|---|---|---|---|
| **ES 1-min, 284 sessions** (`data/es-1min/ES TestingData.txt`) | repo | hand-imported, 2025-06-02 → 2026-08-21 | `tools/model-lodhod.py` → **`HLTAB`** (baked into the panel) and → `BASERATES.json` | git |
| **`BASERATES.json`** | repo `data/es-1min/` | `tools/model-lodhod.py` | the courier → `gpts_hodlod_base_v1` → `hodlodBase()` → ⓪a's E column | git + localStorage |
| **Yahoo ES/NQ/GC/CL 1-min, ^VIX** (`gpts_futbars_v1`) | localStorage | companion `gex-if-levels.user.js` v1.17 | `futBarsLoad()` → `measureBars()` → everything in ⓪a | courier refresh |
| **InsiderFinance chain** (`gpts_if_chain_v1`) | localStorage | companion | `ifChain()`/`ifLadder()` → walls, levels, `skPiles` fallback | courier refresh |
| **ForexFactory calendar** (`gpts_evcal_v1`) | localStorage | companion | `evCalActive()` → `dayTypeOf()` EVENT cap | courier refresh |
| **Skylit gamma/vanna/SPXW payloads** | memory (`LASTFEED`) + `gpts_slices_v7` | the fetch/XHR hook | `extractWalls`, `accumCanon`, `feedSeriesAll` | session |
| **Skylit ladder + velocity** | DOM / React fiber | `readLaddersByDollar`, `velHarvest` | `tapeMap`, `skPiles`, `rollScan`, `levelStateOf`, ROC column | live only |
| **snaps** (per bar, whole ladder + `tri.<SYM>.top` ranked book) | `gpts_recorder_v7` → IndexedDB `snaps` | `recordNodeSnapshot()` every closed bar | replay, `nodeLedger`, the day export, `study-*` | IDB, unbounded |
| **feat** (one record per feature per bar; ~48 → 28) | recorder → IDB `feat` | `featRecordAll`/`featEnqueue` | `resolveFeatureOutcomes` → `featStats` → tabs, `ruleTier`, the export | IDB |
| **defl** (ONE ROW PER FRESH TAP — the event-level ledger) | recorder → IDB `defl` (since v15.51) | `recordDeflections` / `labelDeflectionOutcomes` | ⓪a ledger, `__gptsDebug.deflArchive`, **H5** | IDB |
| **nodeEvents** (NEV: DEFLECT/PIN/BREAK/ROLL/ACCUM) | recorder | `nevScan` | the day export | localStorage window |
| **rules / promo** (`gpts_rules_v2`, `gpts_promo_v1`) | localStorage | `rulesIngest` (from `learning/rules.json`), `rulesApply` | `ruleTier`, `killCheck`, the grade caps | localStorage |
| **`data/<day>.json`** | repo | `repoExportDay` at the close (+ manual) | **the nightly** | git |
| **`learning/log/<day>.json`** | repo | **the nightly** | `pipeNightlyTry` → Analysis REVIEW | git |
| **`learning/rules.json`** | repo | the nightly (proposals that cleared) | `rulesLoad` → `rulesApply` at boot | git |

⚠ **Two things about the 284-session file that the panel must keep saying:** it has **price only, no gamma
book** — so every model built on it (HOD/LOD, far side, GREEN/RED) is a *price* prior, and **nothing about
gamma has ever been tested against it.** The gamma corpus is the 11 days in IndexedDB and it grows one
session at a time. `tools/nightly/PROTOCOL.md` is built around exactly this asymmetry.

## 2 · THE WORKFLOW, STEP BY STEP — status at v15.53, and what v15.54 does

| # | step | what it is | v15.53 status | v15.54 |
|---|---|---|---|---|
| 1 | **PRIOR** | `HLTAB` — 8×9 posr×clock survival table from the 284 sessions, AUC 0.879; ⓪a reads the cell every bar and calls IN / NOT-IN | ✅ works | shown live vs backtest once the close-scorer has sessions |
| 2 | **NODES** | the ladder from Skylit's own numbers; `skPiles`/`skRoles` name King/Gate/Rug; `accumCanon` says building/decaying; `rollLatched` draws where mass moved | ✅ works | ONE accumulation reading (`accumCanon`); `histTrend`/`accumulationStateFor` folded |
| 3 | **TEST** | price at a node: `levelMarkerOf` (ATR band, v15.51), `reactDefence` DEFENDING/ABANDONING, `deflectionAt` (wick tests, close decides) | ✅ works, MARK populated | `nodesVerdict` line re-surfaced |
| 4 | **RECORD** | every bar: snaps + feat; every fresh tap: defl; every event: NEV | ✅ works; defl survives since v15.51 | registry 48 → 28, one record per claim; old keys mapped |
| 5 | **SCORE** | at the RIGHT horizon: 10 bars for a bar claim, **the close** for a session claim (`toClose`, v15.51); `hitNull` says why a score was declined | ✅ works | — |
| 6 | **AGGREGATE** | `featStats`: by **episode**, void rows out, low-band vs high-band (v15.52) | ✅ works | — |
| 7 | **GATE** | ⑤b CAN THE SCORER FAIL: flags a feature whose rate does not move between its own predicted bands | ⚠ **flags, does not gate** — a flagged number can still reach `ruleTier` and the face | **`featGated(key)`: a flagged feature cannot promote and its rate cannot render** |
| 8 | **REGISTER** | PREREGISTER H1–H5, read once at minimum n, sessions from 2026-09-03 only (Testing ⑧) | ⚠ two registers: `roadmap/PREREGISTER.md` and `tools/nightly/HYPOTHESES.md` | **one register**, read by the panel AND the nightly |
| 9 | **PROMOTE** | `proposalClearsBar` → `rulesApply`: eff n ≥ 20 **in sessions for a to-close feature**, 3 walk-forward sessions, no regime flip | ✅ works (never legitimately fired — correctly) | consults the gate |
| 10 | **EXPORT** | `data/<day>.json` at the close: snaps, feat, defl, nodeEvents, ledger, act — **minus `projReview`** (D7), **plus `read`** (D4) | ✅ since v15.53 | `defl` added to the export |
| 11 | **TRANSPORT** | git push from his machine via the installer | ✅ works | — |
| 12 | **NIGHTLY** | `tools/nightly/harness.py`: THE LLM PROPOSES, THE HARNESS DISPOSES — base rate, simplest rule, incumbent, shuffle test, walk-forward only | ❌ **designed, run twice** (`learning/log/` has 2 files) | **`tools/nightly/run.py`** — one command, reads the day file + base rates + the register, scores every registered hypothesis with the four bars, writes `learning/log/<day>.json` (schema 2) — and a **checklist step** |
| 13 | **READ BACK** | `pipeNightlyTry` fetches `learning/log/<day>.json` → `ANALYSIS_NIGHTLY` → Analysis REVIEW | ⚠ reads it, renders one line | **REVIEW renders the verdicts**: per hypothesis, cleared / refused / thin, with the bar it failed |
| 14 | **REIMPLEMENT** | a cleared proposal becomes a weight/swap/kill in `rules.json` → `rulesApply` at boot; a code change ships with a test that can fail | ✅ mechanism exists | the nightly writes `rules.json` proposals in the shape `rulesIngest` already accepts |

**The loop closes when 7, 8, 12 and 13 are done. Everything else is in place.**

> **Status at v15.54 (2026-09-03):** 7 the gate ✅ · 8 one register ✅ · 10 `defl` exported ✅ · 12 `tools/nightly/run.py` ✅ (self-tested: finds a planted effect, refuses a planted nothing) · 13 verdicts rendered in Analysis ④ ✅ · the two tabs in §3 order ✅ · §4 memoisation ✅. **The loop is closed. Its first real turn is the first session exported on this build.** Not yet: the merges (accumulation → one, registry 48 → 28) — v15.55.

## 3 · THE TWO TABS, REBUILT AROUND THE WORKFLOW (v15.54)

Every section below is named by the step it serves. Anything not tied to a step is archived.

**ANALYSIS — "what has the data said"**

| § | title | serves step | shows | reads |
|---|---|---|---|---|
| ① | **HOD / LOD** | 1, 5 | today's call and its cell; **live rate at the close vs the table** (calibration, per cell, sessions as n); the far side; the NOT-IN call's record | `hodLod`, `lodhodCall`, `featStats.byKey.lodhod` (to-close rows only) |
| ② | **DEFLECTIONS AT NODES** | 3, 6 | **episode-level**: held/broke by grade · tap · polarity · session · role — Wilson interval on every cell, the multiple-comparison ledger printed beside the table | `deflStats` (rewritten on episodes), the `defl` store |
| ③ | **NODES** | 2 | the ledger: each node's life, touches, reaction rate | `ledgerSectionHtml` |
| ④ | **REVIEW** | 13 | the nightly's verdicts — per hypothesis: cleared / refused (and which bar) / thin; the calibration line; what changed in `rules.json` | `ANALYSIS_NIGHTLY` (schema 2), `promoEvents` |
| ⑤ | **DIRECTION** | 3 | the direction factors' lift table (compact) — the input to proposals | `dirFactorsHtml` |

*Archived:* the ① HEADLINE tiles (subsumed by ① and ②), ⑤ YOUR CALLS (folded into ② as a column), ⊕ the
feature scorecards (moved to Testing ⊕). Sections: **8 → 5.**

**TESTING — "can we trust it, and what is queued"**

| § | title | serves step | shows |
|---|---|---|---|
| ① | **CAN THE SCORER FAIL** | 7 | per feature: horizon, rows, eff n (sessions for to-close), low vs high band, verdict — **and the word GATED where it bites** |
| ② | **PRE-REGISTERED** | 8 | H1–H5 from the one register: n / min n, status, read-once; H5's ledger count |
| ③ | **PROPOSALS** | 9 | what is asking to change the model; challengers folded in as "vs incumbent"; clears-bar / applied — with the gate consulted |
| ④ | **KILL LIST** | 9 | active kills and their measured rate |
| ⑤ | **DATA COVERAGE** | 4 | what unlocks at what n — sessions, episodes, defl events |
| ⑥ | **SELF-TEST** | 6 | the synthetic day |
| ⊕ | **DETAIL** | — | feature scorecards + the pattern miner (shadow; cannot promote) |

*Archived:* ① QUESTION QUEUE (PREREGISTER replaces it: a question is a hypothesis with no n), ③
CHALLENGERS as a section (a challenger is a proposal). Sections: **9 → 7.**

## 4 · FASTER TO UPDATE — what that means concretely

- **Smaller file** — 31,063 → 26,830 at v15.53; the merges take it under 25,000.
- **One reading per idea** — a change to "accumulation" is one function, not three; a roll is one detector
  and one renderer.
- **One record per claim** — 28 features, so a new claim is one `registerFeature` with an `outcome` that can
  fail, and ⑤b grades it from day one.
- **`tools/archive-block.py`** — retiring a block is a spec file and one command; it refuses if anything
  still references the block.
- **Memoised per render** — `ifLadder`, `emBand`, `emPiles`, `measureBars`, `sessionBody` computed once per
  frame instead of 14–25×; a render is cheaper, so a change to it is cheaper to verify in Chromium.
- **The nightly is one command** — `python3 tools/nightly/run.py <day>` — so "what did last night say" is
  not a research project.

## 5 · WHAT THIS ARCHITECTURE REFUSES

- **No number on the face without a scorer that can fail.** ⑤b is a gate, not a report (step 7).
- **No hypothesis read before its minimum n**, and none added after seeing the data it is tested on.
- **No gamma claim tested against the 284-session file** — it has no gamma book; the panel says so.
- **No second register, no second accumulation engine, no second roll renderer.** One of each.
- **No deletion.** Retired code goes to `archive/<version>/` with its reason and its tests.
