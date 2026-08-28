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

## 3 · THE FOUR TRANSPORTS (all pre-existing; all move files INTO git)

| script | trigger | what it moves |
|---|---|---|
| `setup-gex-autopull.bat` → `tools/gex-pull.bat` | scheduled task **"GEX auto-pull"**, every 2 min | Drive `GEX-inbox`: `gex-patch-*.patch` / `gex-drop-*.tar.gz` → apply → commit → push |
| `tools/review-pull.bat` | scheduled | Drive `GEX-review-inbox` → `review/`, `learning/rules.json` → push |
| `pushdata.bat` / `tools/push-data.bat` | manual / scheduled | `Downloads\YYYY-MM-DD.json` → `data/` → push |
| `installvNNNN.bat` | operator double-click | a whole build → xcopy → commit → push |

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
        -> push-data.bat / GEX auto-pull -> commit + push        (existing pipe)
  CLOUD
        -> tools/append-futures.py   -> data/futures/<SYM>/YYYY-MM-DD.csv
        -> tools/study-hodlod.py --market ES --out data/es-1min/BASERATES.json
        -> Drive GEX-review-inbox -> review-pull.bat -> push     (existing pipe)
  BROWSER · companion v1.15                                       <-- THE RETURN (new)
        -> raw.githubusercontent.com/.../BASERATES.json
        -> localStorage gpts_hodlod_base_v1 -> panel prefers it over the baked-in HODLOD_BASE
```

Only the two marked legs are new. Everything else was already running.

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
| `data/es-1min/NQ TestingData.txt` | ENQU26 1-minute, **TAB**, **no** header, `Y-m-dTH:M` | 188 |

⚠⚠ **THE TWO FILES ARE NOT THE SAME FORMAT** — different delimiter, header and timestamp style.
Assuming they were cost a run. `tools/model-lodhod.py::load()` sniffs all three from the file itself.
⚠ **THE ES CORPUS WAS NEVER MISSING.** Tooling looked for `EPM26-1min.csv.gz`, did not find it, and
reported the corpus absent — **the operator supplied it twice because of that error.** When a file
is "missing", list the FOLDER before trusting the NAME.

**What the daily Yahoo tap is FOR:** these corpora are static exports. The tap keeps them growing so
the ⓪a base rates and the LOD/HOD table can be re-derived on current data instead of ageing.

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
