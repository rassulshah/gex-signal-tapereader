# DATA ARCHITECTURE — who can reach what, and how every number gets here

**Written 2026-08-28. THIS FILE IS READ ON EVERY `load gex`** (`.gex-config.json` → `projectFiles`).

**Why it exists.** The operator asked, three times across two sessions, whether a process existed to
fetch ES data daily. Answering took a full-history search each time, because *every fact about who
fetches what lived in `session-state/`* — `YAHOO-PIPELINE.md`, `INSIDERFINANCE.md`, `SKYLIT-FEEDS.md`
and item 18 inside a resume note that is **rewritten in full every build**. Meanwhile
`design/architecture-design.md` — the file whose name says it should hold this — describes only the
Skylit feed and fiber candles, names AI Drive as the store, and predates InsiderFinance entirely.

> **`design/architecture-design.md` is SUPERSEDED on data sources by this file.** It remains correct
> on layers, the file-shape rules and the observational boundary. Where the two disagree about where
> data comes from, this one is right.

Storing architecture in a file designed to be overwritten is the same mechanism that lost ITEM 18 for
24 versions. **Data-source facts belong here. Add them here.**

---

## 1 · THE ONE TABLE THAT ANSWERS "CAN WE FETCH X?"

| participant | can reach | cannot reach | why |
|---|---|---|---|
| **The panel** (`gex-signal-tapereader.user.js`) | anything same-origin on `app.skylit.ai`; the page's own responses via its fetch/XHR hooks | **any cross-origin host** | `@grant none` is LOAD-BEARING — the feed hooks patch `window.fetch`/`XMLHttpRequest` in **page context**. Any `@grant` sandboxes the script and the tape goes dark. |
| **The companion** (`gex-if-levels.user.js`) | insiderfinance.io · nfs.faireconomy.media · query1.finance.yahoo.com · raw.githubusercontent.com | hosts not in `@connect` | It takes `@grant GM_xmlhttpRequest`, which is privileged past CORS **and** page CSP. It has no tape to break. |
| **The cloud session** (this sandbox) | github.com **read** (clone) · npm/pypi are 403 | **cannot push to git** · cannot reach Yahoo | The git proxy refuses this repo for writes: *"not in this session's authorized repository set."* |
| **The operator's machine** | everything | — | It is the ONLY thing that can push. |

**The three consequences, and every one has bitten this project:**

1. **A foreign fetch goes in the COMPANION.** Never the panel. Never "just add a grant".
2. **A cloud sandbox commit is not a push.** Work that exists only in a sandbox commit does not
   exist. That is how the ES corpus was lost once and `DATA-ARCHITECTURE.md` itself was lost once.
3. **Git is the transport between browser and cloud**, in both directions.

### MEASURED, not assumed (2026-08-27, on the live Atlas tab)

```
await fetch('https://query1.finance.yahoo.com/v8/finance/chart/ES=F?interval=1m&range=1d')
  -> BLOCKED: Failed to fetch
```

Item 18 hedged on exactly this in 2026-08-16 — *"try plain fetch first … verify unsafeWindow access
still OK"* — and the console check was never run for eleven days. It is run. **The page cannot reach
Yahoo.** `window.__gexif` being absent from page context proves nothing, by the way: the companion is
sandboxed, so its globals never reach the page. Test it by its localStorage instead.

---

## 2 · THE TWO BOOKS (never averaged, never compared across windows)

| | SKYLIT | INSIDERFINANCE |
|---|---|---|
| what | **FLOW** — live accumulated dealer positioning | **STRUCTURE** — open interest × gamma |
| route | the page's own `gex/levels` responses, hooked | companion → `GM_xmlhttpRequest` → `__NEXT_DATA__` |
| key fact | `\|net\| ≡ v` on every strike — **no call/put split**, so no Call Wall is computable | `GEX = γ×OI×100×spot²×0.01`, puts negative; verified to their published header |
| detail | `session-state/SKYLIT-FEEDS.md` | `session-state/INSIDERFINANCE.md` |

Scale chain: `k` (SPX strike) → `disp` (chart/ES, ×~1.0023) → `und` (SPY, ×~0.0998).
⚠ `kingKd` is **thousands**; `velocity.cur`/`.d15` are **dollars**.
⚠ **Four separate phantom bugs** came from comparing a number in one book/window/scale against
another. Name both units out loud before comparing two numbers.

---

## 3 · THE TRANSPORTS (all move files INTO git; the GEX nightly task also computes)

| script | trigger | what it moves |
|---|---|---|
| `setup-gex-autopull.bat` → `tools/gex-pull.bat` | scheduled task **"GEX auto-pull"**, every 2 min | Drive `GEX-inbox`: `gex-patch-*.patch` / `gex-drop-*.tar.gz` → apply → commit → push |
| `tools/review-pull.bat` | scheduled | Drive `GEX-review-inbox` → `review/`, `learning/rules.json` → push |
| `pushdata.bat` / `tools/push-data.bat` | manual / scheduled (~15:30, `data/` only) | `Downloads\YYYY-MM-DD.json` → `data/` → push |
| `tools/gex-sync.bat` (task "GEX sync", every 2 min, since 2026-09-03) | scheduled | commits + pushes ANYTHING new in the repo: the day file, the nightly's files Claude writes over the desktop bridge, Drive drops; replaces the Drive-only auto-pull |
| the desktop bridge (`mcp__remote-devices__*`) | per session, folder approved once | Claude reads `data/<day>.json` from his machine and writes `learning/log/`, the tables, the brain back into `C:\Dev\gex-signal-tapereader\` — no installer, no API |
| `installvNNNN.bat` | operator double-click (from `C:\Dev\gex-signal-tapereader\`, written there over the desktop bridge when the session is linked) | a whole build → xcopy → commit → push |
| the 💾 (v15.66) | his click at the close, and the 15:01 auto-export | `data/<day>.json` AND `data/tape/<day>/<BOOK>.json` into the picked data folder → the sync task pushes |
| `setup-gex-nightly.bat` → `tools/gex-nightly.bat` → `tools/nightly/tick.py` (v15.68) | scheduled task **"GEX nightly"**, every 10 min, hidden; RUNS only when the newest `data/<day>.json` is newer than `learning/log/<day>.json` | the nightly ON HIS MACHINE: `run.py` → the log (`ranOn`), the pattern table, the tables, and the registry (`results.py` → `learning/results.json` + `studies.json` patched) → the sync task pushes. Not a courier of market data — the one transport that COMPUTES. Needs Python 3 on the PATH |

**None of them fetches market data.** They are couriers. Until v14.59 the only thing in this project
that ORIGINATED market data was the browser panel, and what it originated was the Skylit day file.

---

## 4 · THE ES CORPUS PIPELINE (v14.59 / companion v1.15) — the daily tap

**Purpose:** keep ⓪a HOD/LOD standing on current data without a rebuild.

```
  DAILY · browser · companion v1.15 · GM_xmlhttpRequest          <-- THE TAP (new)
     query1.finance.yahoo.com/v8/finance/chart/{ES=F,NQ=F,GC=F,CL=F}?interval=1m&range=5d
        -> localStorage  gpts_futbars_v1        (hourly poll, raw bars, no session logic)
  PANEL v14.59
        -> futBarsLoad() -> `futBars` key in data/YYYY-MM-DD.json
  OPERATOR'S MACHINE
        -> the panel writes the day file at 15:01 (v15.71) -> the GEX sync task commits + pushes
  OPERATOR'S MACHINE · THE GEX NIGHTLY (v15.87 — this leg used to say CLOUD, and nothing called it)
        -> tools/nightly/run.py refresh_futures(), after the sweeps:
        -> tools/append-futures.py   -> data/futures/<SYM>/YYYY-MM-DD.csv   (the last four day files; idempotent;
                                        the newest day completes from the next day's 5-day window;
                                        Yahoo's live-quote row — the in-progress minute at 14:49:41 — dropped)
        -> tools/study-hodlod.py     -> data/es-1min/BASERATES.json  (ES: the vendor's 284 + the Yahoo days,
                                        provenance per session, the overlap reported — none)
                                     -> data/futures/NQ/BASERATES.json (NQ: Yahoo only, 11 sessions on 09-08)
        -> the GEX sync task pushes them
  CLOUD · AT BUILD TIME
        -> tools/bake-hodlod.py      -> the panel's HODLOD_BASE literal re-baked from the file (never by hand)
  BROWSER · companion v1.15                                       <-- THE RETURN
        -> raw.githubusercontent.com/.../BASERATES.json
        -> localStorage gpts_hodlod_base_v1 -> panel prefers it over the baked-in HODLOD_BASE
```

(v15.87) **The chain no longer waits for a session.** From v14.59 to v15.86 the append leg existed as a tool and ran only
when a context ran it — his question on 2026-09-08, *"are you also saving the daily stats from yahoo … When will
learning occur"*, found the gap. The corpus now grows by one session a night on his machine; `BASERATES.corpus.last`
is the indicator (2026-09-08 at v15.87; tomorrow's nightly makes it 09-09). **The grid:** the study and the panel both
read the session on his tool's grid — 3-minute bars stamped by END, the open = the bar ending 08:30's open (`corpus.definition`).

### The rules this pipeline is built on

- **THE COURIER IS DUMB ON PURPOSE.** It does no timezone conversion and no RTH classification.
  All session logic happens **once**, in `tools/append-futures.py`, with a real tz database. A
  sandboxed userscript doing DST arithmetic is how a corpus goes quietly wrong for half the year —
  note the companion's own `ctToday()` hardcodes −5h and **is** wrong under CST. Do not copy it.
- **THE TRIM IS A UTC WINDOW, NOT AN RTH WINDOW.** 08:30–15:00 CT is 13:30–20:00 UTC under CDT and
  14:30–21:00 under CST, so the courier keeps **13:00–21:30 UTC** and lets Python decide. Verified
  live: that window retains **391 RTH bars, 08:30→15:00 CT exactly**, on both sessions tested —
  which is precisely the complete-session count `MIN_BARS=386` expects.
- **1-MINUTE DATA IS ≤7 DAYS.** `range=5d` on a daily poll survives a long weekend. **A gap longer
  than seven days cannot be recovered at this resolution, ever.** ⓪a shows corpus staleness on its
  face rather than averaging over a hole.
- **NULL BARS ARE DROPPED, NOT ZEROED.** Measured: **152 nulls in 2674 bars** over 2 days on `ES=F`.
  A null treated as zero becomes a low of 0 and a fake LOD.
- **IDEMPOTENT BY CONSTRUCTION.** The 5-day window re-sends the same minutes constantly; rows key on
  (market, minute). Re-running changes nothing — verified `+0` on a second pass.
- **PROVENANCE, ALWAYS.** `ES=F` is the **continuous front-month** quote; the vendor corpus is
  **`EPM26`**, ONE contract. They differ by the calendar spread across a roll — points, not ticks.
  For HOD/LOD the statistics are a CLOCK and a RANGE and a constant basis shifts neither, **but that
  is an argument, not a measurement.** So: the vendor corpus wins on any day present in both, every
  day records its source, and `BASERATES.json.corpus.sources` shows the mix. **Never averaged.**

### The base rates only travel if they clear a floor

`hlBaseNormalise()` refuses a couriered payload — and keeps the baked-in literal — when it is
malformed, when the ladder is **non-monotone** (the section's only predictive claim would be
unsupported), when the corpus is **< 120 sessions**, or when any rung has **n < 50**.

⚠ **Why the floors exist:** during this build, two synthetic sessions produced ladder rates
`57/80/100/100/100` — monotone, well-formed, and complete nonsense. **Monotonicity is not evidence.**
An old known-good rate beats a fresh unparseable one.

### (v14.72) TWO MORE COURIERS ON THE SAME PIPE — and one of them is the improvement loop

```
  companion v1.16 · GM_xmlhttpRequest
     raw.githubusercontent.com/.../data/es-1min/FARSIDE.json   -> gpts_farside_v1
        the far-side touch/timing tables. Re-derive with tools/study-farside.py, push, and the
        panel picks them up WITHOUT A BUILD - validated first (fsNormalise: >=120 sessions, every
        rated cell n>=60, monotone in distance) or the baked-in copy stands.
     query1.finance.yahoo.com/v8/finance/chart/^VIX?interval=1d&range=2y  -> gpts_vix_daily_v1
        two years of daily VIX. NOT wired into the panel - it exists so the cloud can test whether
        an IMPLIED sigma beats the realized one over the corpus. The daily ATR was measured and adds
        nothing (F-16); implied vol is the one volatility measure that is not a slower copy of what
        the panel already computes.
```

⚠ **The `farside` FEATURE RECORD is a data-collection instrument, not just telemetry.** Every bar it
writes the three nearest rated levels with their distance IN SIGMA **and their node identity**
(`kind`, %King, polarity, role). Nothing in this project can answer "does a put wall get traded more
often than empty air at the same distance" until those rows exist - which is why the record carries
the gamma fields even though nothing consumes them yet. See `docs/LLM-NIGHTLY-BRIEF.md` ⭐ section.

### Adding a market is ONE ROW

`FUT_MARKETS` in `current/gex-if-levels.user.js`: `{ k:'NQ', y:'NQ=F' }`. Contract multiplier, CQG
symbol and RTH window are the cloud's business (`tools/append-futures.py`, `tools/study-hodlod.py`)
because none of them is needed to fetch a bar. Present: **ES · NQ · GC · CL**.

⚠ **ND IS DELIBERATELY ABSENT.** The operator named "nd" on 2026-08-28 and it is not a contract this
context could identify. A guessed symbol puts the wrong series in the corpus under a right-looking
name. `test_futbars.js` f26 **fails the build if ND appears** without him having said what it is.

---

## 5 · WHERE EVERY NUMBER ⓪a SHOWS COMES FROM

| row | source |
|---|---|
| `A` row — today's HOD/LOD, clocks, TOOK, GAP, RNG | the panel's own candles, live, every bar |
| `E` row + the survival ladder | `BASERATES.json` via courier, else the baked-in `HODLOD_BASE` |
| corpus n / last date / "rates live\|baked in" | printed on the section's honesty line |
| BOP · WICK · W.END · WICK% · MUD | **PENDING the operator's definitions** — not invented |
| VWAP | **UNAVAILABLE** — the codebase has none; never a passing tick |

⚠ The `A` row is live and independent of the corpus: a stale corpus does not make today's reading
wrong, it makes the **yardstick** older. That distinction belongs on the face, and it is on it.

---

## 6 · HOW TO CHECK IT IS ALIVE

```js
__gptsDebug.futBars()     // per-market bar counts, per-market errors, courier age in minutes
__gptsDebug.hlBase()      // {src:'courier'|'baked', n, first, last, at, ladder}
__gexif.fut()             // raw courier payload (companion console)
__gexif.futPull('ES')     // force a pull now
```

```bash
python3 tools/append-futures.py                 # day files -> data/futures/<SYM>/
python3 tools/study-hodlod.py --market ES --out data/es-1min/BASERATES.json
GEX_FUTURES_OUT=/tmp/x python3 tools/append-futures.py tools/fixtures/futbars-day.json
```

⚠ **`GEX_FUTURES_OUT` exists because of a real near-miss.** The first fixture run wrote **synthetic
prices straight into `data/futures/ES/`** — same columns, same filenames, plausible numbers, nothing
to flag them. A test that writes into production storage is a corpus-poisoning bug waiting for the
one run nobody watches. **Always redirect the output root when testing.**

---

## 6a · THE CORPORA ON DISK (updated 2026-08-28)

| file | what | sessions |
|---|---|---|
| `data/es-1min/ES TestingData.txt` | EPM26 1-minute, CSV **with** a header, `Y-m-d H:M` | 284 |
| `data/es-1min/NQ TestingData.txt` | ENQU26 1-minute, **TAB**, **no** header, `Y-m-dTH:M` | 188 | — (v15.88) read by `study-hodlod.py` (sniffed from the first line); pooled into `data/futures/NQ/BASERATES.json` with the Yahoo days

⚠⚠ **THE TWO FILES ARE NOT THE SAME FORMAT** — different delimiter, header and timestamp style.
Assuming they were cost a run. `tools/model-lodhod.py::load()` sniffs all three from the file itself.
⚠ **THE ES CORPUS WAS NEVER MISSING.** Tooling looked for `EPM26-1min.csv.gz`, did not find it, and
reported the corpus absent — **the operator supplied it twice because of that error.** When a file
is "missing", list the FOLDER before trusting the NAME.

**What the daily Yahoo tap is FOR:** these corpora are static exports. The tap keeps them growing so
the ⓪a base rates and the LOD/HOD table can be re-derived on current data instead of ageing.

(v15.87) `data/futures/<ES|NQ|GC|CL>/<day>.csv` — the courier's Yahoo minutes per session from 2026-08-24, written by the
nightly; `data/es-1min/BASERATES.json` now pools the vendor's 284 with them (**295** sessions through 2026-09-08, each
session's source recorded); `data/futures/NQ/BASERATES.json` stands on Yahoo alone (the NQ vendor file above has no
parser in `study-hodlod.py` yet — tab / ISO). `data/futures/2026-09-08-tail.json`: the 14:44–14:59 minutes of 09-08 read
from the live companion store, because the day file was written at 15:01 with the ~14:45 poll — the 09-09 window
carries the same minutes and the append is idempotent.

(v15.88) **Companion v1.19**: NQ keeps the whole Globex day (`full:true`) — the NQ chart's ONH / ONL and AHI / ALO / LHI / LLO come
from it; a second fetch, `<SYM>=F?interval=5m&range=1mo` (RTH-only rows, ES and NQ, every 6 h) under `gpts_futweek_v1`, carries
the prior ISO week for **PWH / PWL / WPOC** (1-minute history is 7 days; the prior week is up to 12 back) — read by the panel's
`priorWeek()`, never written to the corpus (the corpus has its own weeks from its own minutes). NQ's BASERATES ride the same
courier as ES's under `gpts_hodlod_base_nq_v1`; the panel keeps **no baked NQ literal** — before the first delivery the NQ
chart's ⓪a says "no base for this market yet".

⚠ (v15.88, **F-23**) the sweep corpus's night is 17:00 of the evening before → the open. Until v15.88 `study-sweeps.py` filed a
session's own post-close bars (15:01–16:59) under its `on` list, so ONH / ONL could be set after the session they were measured
against — a look-ahead that removed failed sweeps (ONL 29% → 22%, ONH 26% → 16%). The panel's `overnightHL` never had those bars;
the two now agree. The Asia / London sessions cut the same night at 02:00 CT (his "standard": Asia 17:00–02:00, London 02:00–08:30).

## 6b · THE TAPE ON DISK (v15.66, 2026-09-04) — the whole book, every bar, every market

Operator: *"in order for you to do proper analysis for the day as part of the end of day review, you must store the
whole day … save the entire tape in daily files for each market."* Until v15.65 the record held the **90 biggest SPXW
strikes** per bar (`vend`) and, for SPY · QQQ · SPXW · VIX, the King, its $K and the **8 strongest strikes as %King**
(`tri`) — a SPY stack survived only if its members were in the top 8 that bar; SPY/QQQ growth per strike was not
measurable from the record; the 100-strike Trinity ladders existed only live.

| where | what | when |
|---|---|---|
| `TAPE` (memory) → `repo.tape` (IndexedDB, keyed `day|book|bar`, indexed by day) | **SPXW**: every row of today's expiry from the velocity harvest — `[cur, d5, d15, d60, d1d]`, Skylit's dollars unaltered (~286 strikes). **SPY · QQQ · VIX**: every Trinity strike — `[pct, vel]` — plus the King and its $K per bar (dollars = pct/100 × kd × 1000) | every closed 3-minute bar while the panel is open on a live page; blind in replay; restored from IndexedDB on a reload; 5 days retained there |
| `data/tape/<day>/SPXW.json · SPY.json · QQQ.json · VIX.json` | one file per book per day: `{schema, book, day, src, f, unit, strikes:[shared list], bars:[{t, bar, px, n, v:[aligned rows], king?, kd?}]}` — ~1.7 MB + 3 × ~0.2 MB a day | **written once, at the close**, by the same 💾 (and the 15:01 auto-export) that writes the day file, into `tape/<day>/` under the picked data folder; a day captured and never written is written on the next boot or the next 💾, once (`kv tapeWritten:<day>`) |
| `tools/nightly/tape.py` | `load(day)` · `dollars(book, bar, k)` · `coverage(day)` · `--selftest`; `run.py` prints the coverage and logs it (`tape`) | the nightly |

⚠ **NOT periodic.** Every write during the day would be a fresh ~2 MB blob for the GEX sync task to push two
minutes later — 26 autosaves is 50 MB of git history for one day. IndexedDB survives a reload and a crash; the file
is the courier, not the safety net. ⚠ **Forward-only:** days before v15.66 stay at 8 strikes. ⚠ **Vendor verbatim:**
nothing in these files is derived; d5/d15 are Skylit's, stored rather than recomputed. Probe: `__gptsDebug.tape()`
(bars per book today, the last write); `__gptsDebug.tapeExport()` writes today on demand.

## 6c · THE DEFLECTION LEDGER, STAMPED (v15.67, 2026-09-04) — one row per tap, in the book's own units

`day.defl.<SYM>[]` — one event per fresh tap (`recordDeflections`), **in the BOOK'S OWN units** (a SPY tap is 768,
`px0` 769.5; a QQQ tap 567): `{sig, key, name (the OLD node-map detector's call), strike, dir, awayPts, bars, px0,
tapBar, t, chips, kings:[books whose King the tap touched — chart-frame join through tapDisp, v15.67], pat:{spx, spy,
qqq} (v15.67: what the PATTERN columns showed at that strike at the tap — null = book unreadable · {node:false} ·
{node:true, k, pct, pos, st:'pika'|'barney', mem, rug:'rug'|'rrug', nw, g}), cont (1 held · 0 broke · null pending),
contBar}`. Mirrored to IndexedDB `defl` (`repoUpsertDefl`, id `sym|date|sig|tapBar`). Scored 10 bars after the tap by
`labelDeflectionOutcomes` (held = continued ≥ DEFL_CONT_PTS in the book's units). ⚠ `kings` was `[]` on every tap
from v15.63 to v15.66 (the scale bug, L-T) and absent before — a pre-v15.67 row says nothing about the King.
Readers: the panel's `patternTable` (Testing ⑦) and `tools/nightly/patterns.py` (the log's `patterns`), pinned equal.

## 6d · THE RECORD, COUNTED (v15.89, 2026-09-09) — `learning/coverage.json` for the 🗄 Data tab

The panel cannot read the repo: it fetches one file at a time from GitHub raw. So the machine that HAS the files
counts them — **`tools/nightly/coverage.py`** runs after the futures step in `run.py` and writes
`learning/coverage.json` (schema 1): `days[]` — one row per `data/<day>.json` (`mb`, `version`, `snaps`, `nodeEvents`,
`defl`, `feat`, `fut{ES,NQ,GC,CL:{n,full,err}}` from the day's couriered `futBars`, `csv{MK:{n,complete}}` from
`data/futures/<MK>/<day>.csv` (complete = 386+ bars, study-hodlod's MIN_BARS), `tape[]` = the books under
`data/tape/<day>/`, `log` = a nightly log exists); `corpora` (ES / NQ BASERATES sessions with vendor vs Yahoo provenance,
the sweep and book corpora, the Learn corpus); `studies` (the registry total, by status, by the corpus each waits on);
`nightly` (the last log's stamps); `tapeDays`; `ranOn` · `asOf` · `generatedAt`. The panel fetches it with the pipeline
check (`coverageFetch` → `gpts_coverage_v1`) and draws ① COVERAGE, ④ NEEDS and part of ⑤ GAPS from it; the live half
(today's tape, the couriers' ages, the browser stores) it reads itself. **A cloud build's copy is the CLOUD's count** —
his machine's next nightly replaces it (the installer carries `learning/*.json`); the guard fetches his machine's added
day files, tape folders and logs before a build so the cloud's count is not short (v15.89, LESSONS).
Readers: the panel's `dataBlock()` (Data ①–⑦); the mockup tool `tools/mockup-data.py` reads the same files directly.

**(v15.90) `counts` — every corpus the registry names, counted:** `price` (ES BASERATES sessions) · `nq` · `sweeps` (the sweep
corpus — the vendor + the couriered nights since v15.90) · `book` (the SPY 3-minute book from the day files) · `ledger` (the
±0.50 deflection taps) · `kingroll` / `gate` (the day files' `dir.kingRoll` / `gateHour` feature rows with an outcome) · `tap`
(data/taps/ — 0 until v15.91) · `vix` / `calendar` (0: no file in the repo; the browser stores hold them) — each with `have`,
`unit`, `perSession` (for the ETA) and `first`. `results.py` reads them to set WAITING / READY on every study whose
`needs.corpus` they name; the Data tab's ④ shows the same numbers.

**(v15.90) THE NIGHT FILES.** `data/futures/<MK>/<day>-night.csv` — the overnight session (17:00 CT the evening before → 08:27),
keyed by the SESSION day, same columns as the RTH file; written by `append-futures.py` (the nightly) only when the courier
holds a real night (200+ minutes: ES since companion v1.18, NQ since v1.19; GC / CL never). Read by `study-sweeps.py` (with the
RTH files and the vendor file) and by nothing else — `study-hodlod.py` skips them. Not on the installer (the nightly's writes).

**The complete architecture — components, integrations, the HOD/LOD statistics pipeline, storage — is
`design/ARCHITECTURE.md`**, generated from `tools/plan-seed.py` and rendered on ⚙ Architecture ⑥–⑨ (v15.67). This
file remains the authority on WHO CAN REACH WHAT and the corpora on disk.

## 7 · KNOWN GAPS

- **`data/es-1min/EPM26-1min.csv.gz` is NOT on GitHub.** 5.1MB against a 6MB installer payload cap —
  the one tracked file the `.bat` cannot carry. It must be dropped into
  `C:\Dev\gex-signal-tapereader\data\es-1min\` on the operator's machine so his next push carries it.
  Until then the study cannot re-derive the 284-session rates and the panel serves the baked-in copy.
- **Corpus gaps:** `2026-07-18 → 08-14` (20 weekdays) recoverable at 2-minute resolution **until
  2026-09-16**, after which it leaves Yahoo's window permanently. `2026-08-24 → now` recoverable at
  1-minute while inside the 7-day tier — which is what the daily tap now prevents from recurring.
- **`skylit-docs/FINDINGS.md` has NEVER EXISTED** in any commit — verified by
  `git log --all --diff-filter=A`. Three live hovers name it. It is not lost; it was never written.
- `origin/main` still tracks **`v10.js` at v11.48** and `install.bat`: the `git rm --cached` was made
  in a sandbox and never pushed. A fresh clone running one test file tests v11.48 and goes green.
