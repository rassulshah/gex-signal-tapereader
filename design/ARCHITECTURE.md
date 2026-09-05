# THE ARCHITECTURE — the complete design of the application, as built

**The process this machinery runs is the Data Analysis process — `design/DATA-ANALYSIS-PROCESS.md` (named 2026-09-04): CAPTURE → ANALYSIS → TESTING → LEARNING → REC → DASHBOARD → SCORE, one definition end to end, no manual step at the close since v15.71 (the panel writes the day itself; the 💾 is the override), his ✓ on Rec the only path to the face.**

_Generated from `tools/plan-seed.py` (the same data the ⚙ Architecture tab renders) · written 2026-09-04 · the WHAT is `design/PURPOSE.md`, the loop is `design/PROCESS.md`, who-can-reach-what and the corpora are `design/DATA-ARCHITECTURE.md`, the second book is `session-state/INSIDERFINANCE.md`, the page itself is `session-state/SKYLIT-FEEDS.md`._

⚠ Edit `tools/plan-seed.py`, run it, re-splice `PLAN_SEED` — never this file by hand. `test_v1567.js` pins the seed, the file and this document equal.

## 1 · The loop (stages ①–⑪)

- **① RECORD** (browser) — every bar's book and price (snaps), the WHOLE tape every bar for SPXW · SPY · QQQ · VIX (v15.66), every tap (defl), every scored feature (feat), what he asked to track (requests), his issues, questions and enhancement requests (items) → writes `recorder store · IDB (snaps, tape, defl, feat)` · probe `rec`
- **② EXPORT** (browser) — (v15.71) the panel writes the day itself: after the close (15:01 CT, then every 10 minutes until the file is confirmed in the repo folder) and, outside market hours, any earlier day still in IndexedDB with bars and no file (write-if-absent); data/<day>.json and (v15.66) the tape files data/tape/<day>/<BOOK>.json go into the folder picked once with 📁; the 💾 is the override and the one click Chrome may need for the folder permission after a reload (choose Allow on every visit once) — the footer chip says 💾 DUE when that click is needed → writes `data/<day>.json · data/tape/<day>/` · probe `saved`
- **③ PUSH** (his machine) — the GEX sync task (tools/gex-sync.bat, every 2 minutes since 2026-09-03) commits and pushes anything new — the day file, the review's files written over the desktop bridge; the cloud can fetch but never push → writes `GitHub main` · probe `pushed`
- **④ NIGHTLY** (his machine (the GEX nightly task) · the cloud) — (v15.68) the 'GEX nightly' task on his machine, every 10 minutes, hidden: when the day file (the panel's after-close write, or his 💾) is newer than its log it runs tools/nightly/run.py — verdicts per hypothesis (read ONCE at minN), the pattern table, the tape coverage, the tables refreshed, TRACK requests and open items copied, the log written, and the study registry patched with every number the log can answer (tools/nightly/results.py → learning/results.json + studies.json) — and the sync task pushes it; the cloud runs the same script when I am in a session; no other model, no API → writes `learning/log/<day>.json · results.json · studies.json (patched) · SWEEPS*.json · requests.json · items.json` · probe `nightly`
- **⑤ REVIEW** (cloud (LLM)) — a READ becomes a hypothesis (predict + refuteIf fixed before the data), a request becomes a study row, an issue/question/enhancement gets its answer in items.json, a refuted rule retires, a taught screenshot becomes a learning example with the record's numbers (tools/node-lookup.py) and a rule L-n, FINDINGS written — and (v15.70) the review's recommendations go onto the Rec tab (tools/rec-seed.py) beside the nightly's, for his ✓ / ✗ → writes `register.json · studies.json · items.json · deflections/examples.json · recommendations.json · FINDINGS.md` · probe `review`
- **⑥ REGISTRY** (cloud) — studies.json (subjects → subsections → studies, result WITH n) and register.json are what the tabs render → writes `learning/studies.json · register.json` · probe `registry`
- **⑦ BUILD** (cloud) — what he APPROVED on Rec, and nothing else on the face: code + mutation-tested assertions + docs + chat history; the installer carries EVERY file the panel fetches; the build marks the Rec row IMPLEMENTED with its version → writes `installvNNNN.bat · recommendations.json (implemented)` · probe `suite`
- **⑧ INSTALL** (his machine) — run the .bat (it pushes), click the Tampermonkey links, reload; the panel fetches the registry, the tables, the log → writes `the running panel` · probe `version`
- **⑨ GATE** (browser) — a feature whose rate does not move between its predicted bands cannot promote and cannot render a rate → writes `featGated` · probe `gate`
- **⑩ DASHBOARD** (browser) — a rate renders only from an earned tier that cleared the gate; THE READ turns today's tape into sentences from the tables → writes `the face · THE READ` · probe `dashboard`
- **⑪ SCORE** (next day) — what THE READ said is written to the day file and scored at the close — a read that cannot be wrong cannot improve → writes `reads[] · readScore` · probe `score`

## 2 · The components

| component | where | does | cadence | owner |
|---|---|---|---|---|
| **THE PANEL — current/gex-signal-tapereader.user.js** | Tampermonkey on app.skylit.ai/atlas, @grant none (same-origin only: it can read the page and localStorage, fetch raw.githubusercontent.com, and nothing else) | reads Skylit's tape and Trinity panes off the DOM and the gex/levels feed, builds the ladder (nodes · NEW · ⇄ ROLL · ▲ GROWTH · PATTERN per book · the King zone), ⓪a (HOD/LOD read, SWEPT), records every bar and every tap, exports the day, renders the seven tabs | a tick every few seconds; one record per closed 3-minute bar | browser |
| **THE COMPANION — current/gex-if-levels.user.js (v1.18)** | Tampermonkey, GM_xmlhttpRequest (the only participant that may fetch cross-origin) | the couriers: the InsiderFinance chain and levels, the Yahoo 1-minute bars (ES full Globex · NQ · GC · CL), VIX daily, the ForexFactory calendar, and the base-rate tables from GitHub — each dropped into a localStorage key the panel reads | IF every 5 min · bars every 5 min in RTH, hourly off-hours (range=5d, so every poll backfills) · VIX every 12 h | browser |
| **THE STORES — localStorage + IndexedDB (gpts_repo_v1, db v4)** | the browser profile | localStorage: config, the day's recorder, node events, the courier drops (10 MB quota, bounded writes, F-10); IndexedDB: snaps · tape (v15.66) · defl · feat · kv (the folder handles) | per bar | browser |
| **THE DAY FILE — data/<day>.json (+ data/tape/<day>/<BOOK>.json from v15.66)** | his repo folder, written by the panel (v15.71: itself, after the close; the 💾 as override) through the File System Access API into the folder picked with 📁 | everything the browser recorded: snaps per bar (book, price, levels, IF chain, trinity, velocity, futBars), node events, the deflection ledger, features, requests, items — and the whole tape per market | after the close, by the panel (15:01 CT, retried every 10 min until confirmed); a missed day outside market hours; the 💾 any time | browser → his disk |
| **THE COURIERS TO GIT — tools/gex-sync.bat (task 'GEX sync') · installvNNNN.bat · the desktop bridge** | his machine (the cloud can fetch, never push) | the sync task commits and pushes anything new every 2 minutes — the day file and the tape files the panel wrote after the close (or his 💾), the log and the registry the GEX nightly task wrote, the review's files written over the bridge; the installer carries a build (every file the panel fetches) and pushes it; the bridge writes the cloud's results straight into the repo folder when the session is linked | 2 min · per build · per session | his machine |
| **THE NIGHTLY — tools/nightly/run.py (+ tick.py, patterns.py, results.py, tape.py)** | his machine — the 'GEX nightly' Windows task (setup-gex-nightly.bat → tools/gex-nightly.bat, hidden, every 10 minutes; tick.py runs it only when the newest day file is newer than its log; Python 3 on the PATH) — and the cloud when I am in a session; no other model, no API | reads the register and every day file: verdicts per hypothesis read ONCE at minN, the pattern table (held rate by setup × book, and from v15.69 the objective outcomes turn / resume from the day's own bars), the tape coverage, the refreshed sweep tables, new TRACK requests and items copied; writes learning/log/<day>.json (ranOn: his machine / cloud) and then the study registry — every study whose number the log can answer gets it (results.py → learning/results.json, studies.json patched in place: result · status · by:'nightly' · asOf; a thin row keeps the review's sentence and shows the count so far) and the Learn tab's rules (each rule's class, its numbers, the record's verdict — agrees / contradicts / thin) | within ~10 minutes of the day file (the task); the sync pushes it ≤ 2 minutes later; the panel re-fetches the registry and the log on its 10-minute check or a reload | his machine · cloud |
| **THE REVIEW — the LLM over the nightly's output** | the cloud, in a session (the one part of the loop that still waits for one) | a READ becomes a hypothesis (predict + refuteIf before the data), a request a study row, an item its answer, a taught screenshot a learning example checked against the record, FINDINGS written, the registry's QUESTIONS and the roadmap updated (the NUMBERS are the nightly's from v15.68 — studies-seed.py merges learning/results.json so a review never erases them), the build shipped | per session | cloud |
| **THE CORPORA — data/es-1min/ (ES 284 sessions · NQ 188) + data/futures/<SYM>/ (the daily tap, created on the first append)** | the repo | the static exports the HOD/LOD tables were built on, growing by one file per session from the courier's bars (tools/append-futures.py) | daily | repo |
| **THE SUITE — test_*.js (145 files) · tools/run-tests.sh · tools/smoke.js · tools/render-face.js** | the cloud before every build; his machine after an install | executes the panel's functions with fixtures (never grep where it can run), mutation-tested; the smoke loads the whole script; the render draws the shipped face on a recorded day in Chromium | every build | cloud |

## 3 · The integrations

### Skylit Atlas (the page)

- **how:** DOM + the page's own gex/levels feed (installFeedObserver)
- **what:** the SPXW tape (strike, %King, the King's $K), the velocity table (every strike's size and d5/d15/d60/d1d for today's expiry), the Trinity panes (SPY · QQQ · SPXW · VIX: 100 strikes as %King + the King's $K), the headers (prices), the chart's 3-minute candles
- **keys / functions:** LASTFEED · VEL · TAPE_CACHE · LADDER_CACHE
- **notes:** the panel runs ONLY on /atlas*; never open a second /atlas tab; the required chart posture is RTH · READ AS %King · VELOCITY All · LOW NODES never Hide (SKYLIT-FEEDS.md)

### InsiderFinance (the second book — STRUCTURE)

- **how:** the companion, GM_xmlhttpRequest to insiderfinance.io/gamma-exposure, every 5 min
- **what:** the SPX / SPY / QQQ option chains (open interest × gamma per strike) in three windows — dte0 · toFri · all — and the levels the panel names: CW / PW (call and put walls), CW0 / PW0 (0DTE walls), FLIP, Mag / HVL, the expected-move band (emBand) that the ladder's scale is built on (ifLadder: dispScale, px, undPx)
- **keys / functions:** gpts_if_chain_v1 → ifChain() · ifLadder() · gLevels()
- **notes:** STOCK, refreshed once a day at the source; never averaged with Skylit's FLOW; every number carries its book, window and scale (INSIDERFINANCE.md — four phantom bugs came from mixing them)

### Yahoo Finance (price bars)

- **how:** the companion only (the panel is @grant none and never touches Yahoo — test_futbars f30): Yahoo's v8 chart endpoint, <SYM>=F?interval=1m&range=5d — every 5 min in RTH, hourly off-hours; ^VIX interval=1d&range=2y every 12 h
- **what:** ES (the whole Globex day, so ONH/ONL are the real overnight 17:00→08:29 CT), NQ, GC, CL 1-minute bars; VIX daily closes
- **keys / functions:** gpts_futbars_v1 → futBarsLoad() (the day file's futBars) · gpts_vix_daily_v1
- **notes:** 1-minute history is ≤ 7 days at the source — a gap longer than that can never be recovered; the day file carries the bars so the corpus grows even when the courier missed a poll

### ForexFactory (the calendar)

- **how:** the companion
- **what:** the day's economic events, for the event tag and the calendar context (X2)
- **keys / functions:** gpts_evcal_v1
- **notes:** context only; never a direction vote

### GitHub raw (the repo, read back)

- **how:** the panel's own fetch (raw.githubusercontent.com is CORS-open) and the companion for the two base-rate tables
- **what:** learning/log/<day>.json (the nightly), studies.json · register.json · plan.json · items.json · requests.json · deflections/examples.json · suite.json (the tabs), review/<day>.json, data/es-1min/BASERATES.json + FARSIDE.json (couriered into gpts_hodlod_base_v1 / gpts_farside_v1 and VALIDATED before use)
- **keys / functions:** pipeFetch() · the *_SEED copies in the script render before the first fetch
- **notes:** raw is CDN-cached ~5 minutes; every file the panel fetches rides the installer (test_installer_manifest)

## 4 · THE HOD/LOD STATISTICS, DAILY — from Yahoo bars to the ⓪a line

① the companion fetches ES 1-minute bars (full Globex) every 5 minutes in RTH → gpts_futbars_v1
② the panel carries them in the day file (futBars) → the after-close write (v15.71; the 💾 as override) → the sync task → GitHub
③ tools/append-futures.py writes data/futures/ES/<day>.csv — the corpus grows by one session a day beside the static 284-session export (data/es-1min/)
④ tools/study-hodlod.py re-derives BASERATES.json: the first-extreme and second-extreme clocks, the LOD-first share, the gap, the range, the wick family (trimmed means, no-wick days and Tukey outliers excluded), and the lookup table itself — 8 posr rows × 7 forty-five-minute blocks (9:15–13:45 CT), 284 sessions, 38,054 observations, AUC 0.879 (a 5-feature logistic scored the same 0.8795, so the table ships: F-5); every cell carries its n and a cell under 25 observations is never shown
⑤ tools/study-farside.py writes FARSIDE.json (does the far side trade before the close × when — validated ≥ 120 sessions, every rated cell n ≥ 60, monotone); tools/study-sweeps.py writes SWEEPS.json / SWEEPS-BOOK.json (which swept level printed the extreme — by flush, clock and speed, not by name: F-14/F-16)
⑥ the companion couriers BASERATES.json and FARSIDE.json into gpts_hodlod_base_v1 / gpts_farside_v1, validated before use; the panel fetches SWEEPS*.json itself
⑦ live, ⓪a reads today's bars (measureBars: the chart live, the frames in replay): posr (how far price has travelled off the extreme) × the clock → the table cell → 'HOD IN 84%' / 'LOD after 11:02 — 80%' (the one-sided floor from FARSIDE), the SWEPT line (names only; the rates in the hover)
⑧ what THE READ said is written to the day file and scored at the close (stage ⑪; the live scorer can fail — F-11); the decision rates on the face are the corrected ones: IN 63% (n=284, F-12), NOT-IN 85% (n=230, F-11/F-12) — never the withdrawn 92%

## 5 · Storage

| key / store | what |
|---|---|
| `gpts_cfg_v8` | the gear: thresholds, toggles (ladderGrid, dayRead, motion), the growth window |
| `gpts_recorder_v7` | today's recorder: snaps, events, the deflection ledger (defl), features — bounded to 3.6 MB, evicted oldest-day-first; mirrored to IndexedDB |
| `gpts_nodeevents_v1` | node events (ACCUM · ROLL · taps with their why-vector and three outcomes) — bounded to 1.2 MB |
| `gpts_nodeborn_v2` | the day's births and the below-set for NEW |
| `gpts_futbars_v1 · gpts_vix_daily_v1 · gpts_if_chain_v1 · gpts_evcal_v1 · gpts_hodlod_base_v1 · gpts_farside_v1` | the courier drops (companion → panel) |
| `gpts_items_v1 · gpts_requests_v1 · gpts_lastbook_v1` | his items and TRACK requests until the day export copies them; the close-of-session book |
| `IndexedDB gpts_repo_v1 (v4): snaps · tape · defl · feat · kv` | every bar of every day (snaps), the whole tape per bar per book (tape, 5 days retained; the files are the record), every tap (defl, the ledger H5 waits on), resolved features, the folder handles (dataDir, irtDir) |
| `learning/results.json → learning/studies.json (patched)` | (v15.68) the nightly's numbers by study id, and the registry the Analysis tab renders with them applied — result · status · by:'nightly' · asOf on the rows the nightly can answer; `nightly` (the count so far) on the rows it is still counting toward |
| `learning/recommendations.json · gpts_rec_v1 · gpts_reco_v1 · the day file's `reco`` | (v15.70) the Rec tab: the file (the nightly's and the review's rows with their status), its fetched copy, his ✓ / ✗ on this machine, and the decisions riding the day file to the nightly |
| `learning/markets.json` | (v15.70) every market-specific number in one place — books, chart, bar length, the tolerances, the corpus — SPY live; NQ · GC · CL price only |
| `data/<day>.json · data/tape/<day>/ · learning/* · review/*` | the repo — the one source of truth; the cloud clones it, the panel fetches from it, the installer and the sync task push to it |

## 6 · The rules the machinery enforces

- a % is never rendered without its n — _test_analysis_tabs 5a/5b · bareP · RATE_MIN_N_
- a scorer must be able to fail before its rate means anything — _featGated · test_v1554 7*_
- a hypothesis is written with predict + refuteIf BEFORE the data and read ONCE at minN — _register ↔ seed pin · the nightly's judge_
- a first read is never a verdict — it becomes a register row and is read again on unseen sessions — _H7.since · the ledger line_
- every level named live is made the way the corpus made it — _overnightHL().full · companion v1.18 · test_v1556_
- every new assertion is mutation-tested — _BUILD-CHECKLIST §2_
- every file the panel fetches rides the installer — _test_installer_manifest.js_
- after an install, probe the live panel — never assume — _this tab_
- the mockup's look is the panel's look — one stylesheet, one skeleton — _test_v1562 1a · 2e_
- a deflection is never called from the picture alone — the record is looked up; the gauge cannot flatter — _tools/node-lookup.py · test_v1562 3f/3g/3l_

## 7 · The tabs

- **Dashboard** (ACT) — what the node is DOING as price arrives — NEW · ⇄ ROLL · ▲ GROWTH · SETUP per book, the King zone (three Kings, lit), the HOD/LOD line and the SWEPT line at the top in plain words; every rate with its n, in the hover
- **Analysis** (ASK AND READ) — the registry: every study by subject, its status, its result with n; the TRACK field
- **Testing** (TRUST AND PROMOTE) — the register, the gate, what the ladder renders and why, the record, the nightly, the suite
- **Architecture** (THE HOW) — the objective and the loop, as live status — is the machinery working today
- **Roadmap** (THE PLAN) — what shipped, what is next, in order, each with the objective it serves; your enhancement requests
- **Open Items** (PROJECT MANAGEMENT) — your issues and questions, with the review's answer under each
- **Learn** (TEACH) — the deflection learning doc: your screenshots, my blind calls, what the record says, the rules, and the 0–100 gauge
- **Rec** (DECIDE) — (v15.70) proposals to you from the nightly (pre-registered conditions) and the review, with their evidence; your ✓ / ✗ rides the next day file; nothing on the face changes except through here
