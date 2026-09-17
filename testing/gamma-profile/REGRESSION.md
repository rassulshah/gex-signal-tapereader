# lsGammaProfile — the regression suite

_Created 2026-09-16 on the operator's mandate: "a thorough regression build for the gamma profile. It must also have
testing to ensure that Skylit tape and Atlas chart match IRT. In fact you should get datapoints from Atlas as well as
screenshots and ensure everything is consistent. The regression should be documented to keep a trail so it can be
referenced in the future and results and notes should also be documented."_

Results trail: **`RESULTS.md`** (one line per run) · per-run detail + screenshots: **`runs/<date>/`**.

_2026-09-17: this suite is one of four — the master document is **`testing/REGRESSION.md`**, the runner for all of them
`tools/regress.py [all|gamma|daymodel|daystats|kingtracker]` (`regress.bat` on his machine), the all-indicator trail
`testing/RESULTS.md`. The pinned fixture pairs here (A-0947, IF-1020, 1325, 1426) run as Gate L on every build._

---

## 0. What "consistent" means here — the pipeline and its three gates

```
 ATLAS (Skylit)  ──(panel reads the DOM tape + the ES1 payload)──▶  GammaProfile.csv  ──(lsGammaProfile parses + draws)──▶  IRT chart
      │                        GATE A                               + .audit.json                     GATE B
      └────────────── ATLAS CROSS-CHECK: a screenshot of the tape, transcribed, vs what the panel read ───────────────┘
```

A gap lives in exactly one place, and naming it is half the fix:

| gap | lives in | found by |
|---|---|---|
| Atlas shows X, the CSV says Y | the PANEL (`gammaProfileBuild`, tape reader, ratio, IF chain) | Gate A tests + the audit diff |
| the CSV says Y, IRT draws Z | the PLUGIN (parse, offset, roles, tags, colours, layout) | Gate B logic test + the eyes-on checklist |
| the panel read X′ but Atlas's screen shows X | the tape PARSER (DOM read, King cell, sign) | the Atlas cross-check |

**The golden rule (from `design/IRT-VS-SKYLIT-TESTING-PLAN.md`): same moment or it is not a comparison.** The audit
sidecar carries the panel's read at the CSV's own timestamp, so Gate A is always same-moment. Screenshots must be
taken within ~60 s of the CSV's `ASOF`; the run file records both.

**Book / window / scale, pinned before every comparison** (the project's #1 error):
SPX book (SPXW tape) · 0DTE window (`exp_count=1`; the companion's `dte0`) · three scales — SPX strike (7685),
ES1 price (strike × ratio; 1.0006 on Sep, ~1.0094 on Dec after the 2026-09-16 roll), chart price (ES1 + basis to the
charted contract, which lsGammaProfile applies from `SCALEREF`). Never compare across.

---

## 1. The four layers

### 1a. Gate A — `test_gammaprofile_build.js` (runs the code, 40 assertions, mutation-checked)
`cp current/gex-signal-tapereader.user.js v10.js && node test_gammaprofile_build.js`

Feeds `gammaProfileBuild()` the pinned Atlas snapshot of **2026-09-16 09:47:09 CT** (fixture A — the ladder that was
verified row-for-row against Atlas and the IRT rail that morning) plus a synthetic fixture B (a rug, a pika stack, a
negative King, a RANGE/POS regime) and asserts every CSV row:

- 40 STRIKE rows: ES = round(SPX × Skylit's ratio, 0.25); the 7th field is the raw SPX strike; %King is the tape's
  signed value with the King forced to ±100 by its sign; ranks follow |%King|.
- KING = the flagged strike's ES; SCALEREF = the ES1 payload spot.
- Pattern tags (6th field) from the Dashboard's own `gridSetups()`: barney stack named on its biggest member, members
  marked, a 28% neighbour breaks the run (S6), 27/29 is not a pika stack, RUG only with price under the yellow.
- FLIP / CW / PW from the companion's 0DTE numbers on the ES scale; **absent** when the chain is stale — never an
  all-expiry substitute.
- REGIME: sign from spot vs the 0DTE flip with the 3-pt AT buffer; type from the ladder's structure; conflict flag;
  the roll count; NA (not "AT") when the spot is missing; the spot falls back Trinity → ladder → chain.
- The audit sidecar carries the tape, the King and polarity, both ratios, the spot source, the IF numbers, the regime.
- Three mutations (raw SPX instead of ES; King sign dropped; staleness ignored) prove the assertions fire.

Pre-existing failures elsewhere in the suite (`test_irt_export.js` 23, `test_v1592.js` 11) predate this work and are
unrelated; they are listed in RESULTS.md so nobody rediscovers them.

### 1b. Gate B (logic) — `plugin/test_gammaprofile_logic.cpp` (57 assertions, mutation-checked)
Cloud: `g++ -std=c++14 -o /tmp/gpl plugin/test_gammaprofile_logic.cpp && /tmp/gpl` · Windows: `plugin\run-logic-tests.bat`.

Every decision lsGammaProfile makes now lives in `plugin/GammaProfileLogic.h` (no SDK) and is pinned here:
- **Roles**: King; Ceiling / Floor = biggest |node| above / below spot, King excluded; **Gatekeeper = ONE node**, the
  largest strictly between spot and the King and ≥ 30% of it; `G·C` / `G·F` when it is also the ceiling / floor; no G
  without a spot; the path can point down (spot above the King).
- **Air pockets**: ≥ 3 thin strikes (< 8%) bounded on BOTH sides by a node ≥ 20%; a tail run is never banded; two thin
  strikes are not a pocket; a net-negative run is flagged (the violent tint). Fixture A has NO pocket — v0.41 shaded its
  whole tail.
- **Stacks**: the panel's tags → one bracket per run, named once; a second named member starts a new bracket; members
  print no letter (the bracket marks the run — the operator's choice over a letter on every member).
- **Level → node**: CW / PW tag by SPX strike (immune to the Dec basis); price within 1 pt only as a fallback; the FLIP
  is a price and tags no node.
- **Contract offset**: SCALEREF anchor (Sep→Dec +68.94 on 09-16 09:47; ~0 after Skylit's roll); SPOT fallback; the ±300
  clamp protects an NQ chart.
- **Top-N / primary**; **the regime line** text for every sign/type, ASCII only (the â€" lesson).
- Mutations (gatekeeper floor 30→10; pocket needing one edge; G without G·C) each fail the suite.

### 1c. Gate B (render) — the eyes-on checklist, `CHECKLIST.md`
Nothing here can see IRT's pixels. The runner prints the **expected chart** for the CSV under test (which node carries
K / C / F / G, the CW / PW tags, where the FLIP tick sits, the bracket runs, the regime line, the top-5 badges); the
operator or the session checks it against an IRT screenshot and transcribes what IRT shows into `--irt-read`. Colour
rules are on the checklist (they are not machine-checkable from here).

### 1d. Live — `tools/gp-regress.py` (the same-moment run with Atlas datapoints and screenshots)
```
python3 tools/gp-regress.py --csv GammaProfile.csv --audit GammaProfile.audit.json \
    --atlas-read atlas.json --irt-read irt.json --shots atlas.png irt.png --note "..." --label T2
```
Inputs, staged from the operator's machine at the same minute:
- `GammaProfile.csv` and `GammaProfile.audit.json` (`%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\`) — the CSV and the
  panel's Atlas read at that export (panel ≥ 16.29).
- `atlas.json` — the SPXW ladder transcribed from an **Atlas screenshot** (`{"7610": -49, ..., "king": 7685, "es1": 7613.5}`).
- `irt.json` — what the **IRT rail** shows, transcribed (`{"tags":{"7660":"G.C","7500":"F"},"cw":"7675","pw":"7600",
  "flip_between":[7620,7625],"top5":[...],"named":{"7605":"B"},"regime":"REGIME  -gamma | WHIPSAW | ..."}`).
- The screenshots themselves, copied into the run folder.

Checks: timing (CSV ASOF == audit ct), every Gate A derivation from the audit, the Atlas transcription vs the panel's
tape (±1, King, ES1 vs SCALEREF), and every Gate B expectation vs the IRT transcription. Writes
`runs/<date>/<HHMM>[-label].md` and appends to `RESULTS.md`. Exit code = failures.

---

### 1e. The IF book (v16.30)
`GammaProfile-IF.csv` (BOOK,IF0DTE) is the same grammar from InsiderFinance's 0DTE chain (`design/IF-BOOK-OPTION.md`).
Gate A §9 pins the builder on the 10:20 CT chain paste (`fixtures/fixtureIF-1020.*`); the runner recognises the BOOK row
and derives the expected rows from `audit.ifProf`; the Atlas cross-check does not apply (Atlas is not its source). Gate
B logic and the checklist are identical — the plugin does not know which book it is drawing.

## 2. When to run what

| event | run |
|---|---|
| any change to `gammaProfileBuild` / `gpRegime` / `gridSetups` / the tape reader | 1a |
| any change to `GammaProfile.cpp` / `GammaProfileLogic.h` | 1b (cloud g++ before the commit; `run-logic-tests.bat` after the DLL build) |
| any release of either | 1d once after install, with both screenshots — that run is the acceptance test |
| the scheduled T1–T5 checkpoints | `check-gammaprofile.py` (structure) today; 1d without screens is the planned upgrade |
| a question of the form "is IRT showing the right thing?" | 1d with screens, same minute, before any code is touched |

## 3. Known limits (honest)
- Colours and pixel placement are checked by eye against `CHECKLIST.md`, not by code.
- The Atlas transcription is a human/LLM read of a screenshot; ±1 is the tolerance for a value read across a snapshot gap.
- The scheduled cloud runs cannot take screenshots; they run Gate A from the audit only.
- `gridSetups`' rug rule needs the 5-pt SPX grid step (`gridStep('SPX')`); a different book's step is not covered here.
