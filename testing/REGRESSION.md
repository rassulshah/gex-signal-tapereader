# The regression — all four RTX indicators

_Operator, 2026-09-16: "I had told you to make a regression and you did it for gamma profile. This regression needs to be
extended to the other indicators and you need to keep updating the test cases, so you can fire the regression either
collectively or separately for the indicators."_  Built 2026-09-17 (panel 16.37 · GP 0.56 · DM 0.17 · DS 0.8 · KT 0.10).

**Run it:** `python3 tools/regress.py` (cloud, every build) · `regress.bat` (his machine, double-click) — both take
`all | gamma | daymodel | daystats | kingtracker` (several may be listed), `-v` for every suite's output, `--no-record`
to leave the trail alone. **Results trail:** `testing/RESULTS.md` (one block per run, all indicators) plus each indicator's
own `RESULTS.md` for its live same-moment runs.

---

## 0. The rule that keeps it alive

**A build that touches an indicator updates that indicator's cases in the same commit — and the build is not done until
`tools/regress.py <indicator>` is green.** `tools/regress.py` names, per indicator, the source files it is built from
(`INDICATORS[...]['sources']`); if a diff touches one of them, its Gate A / Gate B suites and, when a row's grammar or a
decision changes, its fixtures are part of the change. A new row, a new field, a new decision → a new assertion, first.
The mutation blocks (every suite has them) are what make a green run mean something: they prove the assertions bite.

`tools/BUILD-CHECKLIST.md` carries this as a checkbox; `test_savedone.js` does not check it — the person building does.

## 1. The three gates, the same for every indicator

```
 the panel's READ (Atlas tape · IF chain · the chart's bars)
        │  GATE A — node tests EXECUTE the panel's builders with stubs and pin every CSV row
        ▼
 GammaProfile.csv / GammaProfile-IF.csv  +  GammaProfile.audit.json (what was read, at the CSV's own second)
        │  GATE L — tools/gp-regress.py re-derives the rows from the audit on pinned fixture pairs (and on today's export, by hand)
        ▼
 the RTX plugin parses, decides, draws
        │  GATE B — C++ tests pin every decision in plugin/<Name>Logic.h (no IRT SDK; g++ here, MSVC there)
        ▼
 the IRT chart — the eyes-on CHECKLIST.md per indicator, with the runner's printed "expected picture"
```

A gap lives in exactly one gate, and naming it is half the fix: Atlas shows X / the CSV says Y → the panel (Gate A);
the CSV says Y / IRT draws Z → the plugin (Gate B); the CSV and the audit disagree → the export's wiring (Gate L).
**Same moment or it is not a comparison**; **book / window / scale pinned before every comparison** (the project's #1
error — `session-state/INSIDERFINANCE.md`).

## 2. The map

| indicator | Gate A (node, `v10.js` = `current/` copy) | Gate B (C++, `plugin/`) | Gate L fixtures | eyes-on |
|---|---|---|---|---|
| **gamma** — lsGammaProfile | `test_gammaprofile_build.js` (65 + 3 mutations; fixtures A 09:47 / B synthetic / IF 10:20) · `test_if_extras.js` (24: depth pill, slope word, level rows) | `test_gammaprofile_logic.cpp` (57 + 3 mutations) · `test_contractoffset_logic.cpp` (15: the SCALEREF-minute anchor, shared with KT) | `gamma-profile/fixtures/` fixtureA-0947 · fixtureIF-1020 · 1325 · 1426 (Gate A of the runner; the Atlas/IRT transcriptions of 1325/1426 are live-run records, not regressions) | `gamma-profile/CHECKLIST.md` |
| **daymodel** — lsDayModel | `test_daymodel_em.js` (34: EM blend, placement, pin, daily record) · `test_day_export.js` (38: the WHOLE day section end to end + the audit re-derivation + a placement mutation) · `test_hodlod.js` (176: the base rates) | `test_daymodel_logic.cpp` (23: bar stamp, windows, PDH/PDL/ONH/ONL, the actual day, offsets, stale) | `day-model/fixtures/synth-1033` (the 10:33 CT synthetic morning `test_day_export.js` writes with `GPTS_DUMP=`) | `day-model/CHECKLIST.md` |
| **daystats** — lsDayStats | `test_daystats_cond.js` (27: the conditional E row) · `test_day_export.js` · `test_hodlod.js` | `test_daystats_logic.cpp` (19: rows, clocks, the twelve cells, the ladder, tones) | `day-stats/fixtures/synth-1033` | `day-stats/CHECKLIST.md` |
| **kingtracker** — lsKingTracker | `test_kingtracker_rows.js` (32 + 2 mutations: the sampler's dwell, the anti-oscillation, the rows; 16.38 the IF / IFQ Magnet books — reduction, scale, dwell, stale / error / empty chains) | `test_kingtracker_logic.cpp` (21: grammar, the v0.6 re-derivation, anchor, clamp, stale; 0.11 the Source switch `bookDrawn`) · `test_contractoffset_logic.cpp` | `king-tracker/fixtures/synth-1033` | `king-tracker/CHECKLIST.md` |

Shared suites run once per invocation (`test_hodlod.js`, `test_day_export.js`, `test_contractoffset_logic.cpp`) and are
listed under every indicator they cover. `tools/regress.py --syntax` additionally compiles each plugin `.cpp` against the
SDK header with `tools/shim/shim.h` (cloud only; the header is Linn's and is not in the repo).

## 3. The live same-moment run (today's export, with screenshots)

```
python3 tools/gp-regress.py [--indicator gamma|daymodel|daystats|kingtracker] --csv GammaProfile.csv --audit GammaProfile.audit.json \
    [--atlas-read atlas.json] [--irt-read irt.json] [--shots irt.png ...] [--note "..."] [--label T2]
```
Inputs from `%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\` at the same minute (panel ≥ 16.37 for the day indicators —
the audit's `AU.day` block is what the day rows are re-derived from; the King rows use `AU.kingNow` / `AU.rolls`).
Each run writes `testing/<indicator>/runs/<date>/<HHMM>[-label].md` with every check and the **expected picture** for the
screenshot, and appends one line to that indicator's `RESULTS.md`. `--irt-read` formats:

- gamma: `{"tags":{"7660":"G.C"},"cw":"7675","pw":"7600","flip_between":[7620,7625],"top5":[...],"regime":"..."}`
- daymodel: `{"offset": 0.0 | "chart_close": 7702.25, "exp":{"o":..,"h":..,"l":..,"c":..}, "act":{...}, "basis":"em-open60"}` (chart prices; ±1 pt)
- daystats: `{"offset": 0.0, "A":[12 cells or null], "E":[12 cells or null]}` (spaces ignored; a null skips a cell)
- kingtracker: `{"offset": 0.0, "SPX":{"now": 7757.3, "steps":[[...]], "strike_label": 7685}, "SPY":{...}, "IF":{...}}` (transcribe the books the chart's Source draws: Skylit → SPX + SPY, IF → IF). **The toggle test (0.14):** add `--kt-status KingTracker.status.txt` (written by the plugin after every draw) — the runner checks the Source, that only that source's books are drawn, their step counts against the CSV and their KINGNOW against the panel's.

`--dry` runs the checks without writing anything — that is how `tools/regress.py` runs the fixture pairs (Gate L).

## 4. What the regression has already caught (keep adding)

- **2026-09-17, 16.37** — `test_day_export.js` re-deriving the rows from the audit found the pre-EM range stages using
  `b.so < open+30·60` (the 09:00 bar dropped) while `gpOpenWindow`, the EM model, the conditional E row and BOTH studies
  (`study-hodlod.py`, `study-em-range.py`: `r[0] <= RTH_A + 30*60`) use bars ending at or before the minute. One bar
  short of the coefficients' own definition since 16.18. Fixed: the stages and the opening drive now read `w30GP`/`w60GP`.
- **2026-09-16, 16.36** — the E row copying the actual 1ST after the READ (his "how can that be?"); pinned in
  `test_daystats_cond.js` and re-checked live by the daystats runner ("the E row does not copy the actual 1ST clock/took").
- **2026-09-16, 0.48 → 0.49** — the after-hours offset (Skylit's ES1 frozen at 14:26 on FOMC): the SCALEREF-minute anchor,
  pinned in `test_contractoffset_logic.cpp` for both plugins that use it.

## 5. Known limits (honest)

- Colours and pixel placement are checked by eye against each `CHECKLIST.md`, not by code (the 0.51 → 0.55 pill saga).
- The King tracker has REAL fixtures from 2026-09-17 (0942-ready / 0943-skylit / 0944-if, with the plugin's status file — the toggle test, all green). The day fixtures are SYNTHETIC (a constructed morning) until a real 16.37 export is staged; the first real pair goes
  into `day-model/fixtures/` and `day-stats/fixtures/` as `<HHMM>.csv/.audit.json` and into the `live` list in
  `tools/regress.py`.
- `run-logic-tests.bat` / `regress.bat` have not yet been run on his MSVC — the cloud runs the identical files with g++.
- `test_irt_export.js` (23) and `test_v1592.js` (11) are pre-existing failures elsewhere in the suite, unrelated; listed in
  `gamma-profile/RESULTS.md` so nobody rediscovers them.
