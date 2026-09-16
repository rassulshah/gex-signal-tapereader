# lsGammaProfile regression — results trail

_One line per run, newest at the bottom (append-only — never edit a past line; add a note line under it instead).
Detail and screenshots in `runs/<date>/`. The suite itself is described in `REGRESSION.md`; the eyes-on part in
`CHECKLIST.md`._

## Automated suites (run on every build)

| date | suite | result | notes |
|---|---|---|---|
| 2026-09-16 | `test_gammaprofile_build.js` (Gate A) | 40 / 40 + 3 mutations fire | first version, fixture A (09:47 CT) + fixture B (synthetic rug/pika/RANGE) |
| 2026-09-16 | `plugin/test_gammaprofile_logic.cpp` (Gate B logic) | 57 / 57 + 3 mutations fire | g++ in the cloud; `run-logic-tests.bat` not yet run on the operator's MSVC |
| 2026-09-16 | `tools/smoke.js` | clean | — |
| 2026-09-16 | `test_irt_export.js` / `test_v1592.js` | 23 / 11 pre-existing failures | **unrelated to this work** — identical on the pre-change source; parked, not fixed |

## Live runs (same moment: CSV + audit + Atlas screenshot + IRT screenshot)

| date | CSV time | panel | verdict | checks | note | run |
|---|---|---|---|---|---|---|
| 2026-09-16 | 09:47:09 | 16.26 (pre-suite, manual) | PASS (manual) | rail vs CSV vs Atlas row-for-row; King 7685 = 7689.25 = 7755 Dec; SCALEREF 7614.06 vs ES1 7613.50 | done by hand in-session before the runner existed; became fixture A | — |
