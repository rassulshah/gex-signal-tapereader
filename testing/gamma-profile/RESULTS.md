# lsGammaProfile regression — results trail

_One line per run, newest at the bottom (append-only — never edit a past line; add a note line under it instead).
Detail and screenshots in `runs/<date>/`. The suite itself is described in `REGRESSION.md`; the eyes-on part in
`CHECKLIST.md`._

## Automated suites (run on every build)

| date | suite | result | notes |
|---|---|---|---|
| 2026-09-16 | `test_gammaprofile_build.js` (Gate A) | 40 / 40 + 3 mutations fire | first version, fixture A (09:47 CT) + fixture B (synthetic rug/pika/RANGE) |
| 2026-09-16 | `plugin/test_gammaprofile_logic.cpp` (Gate B logic) | 57 / 57 + 3 mutations fire | g++ in the cloud; `run-logic-tests.bat` not yet run on the operator's MSVC |
| 2026-09-16 | `test_gammaprofile_build.js` §9 (IF book) + `gp-regress.py` on `fixtureIF-1020` | 16 / 16 + 19 / 19 | v16.30; the IF builder on the 10:20 CT chain paste |
| 2026-09-16 | `tools/smoke.js` | clean | — |
| 2026-09-16 | `test_irt_export.js` / `test_v1592.js` | 23 / 11 pre-existing failures | **unrelated to this work** — identical on the pre-change source; parked, not fixed |

## Live runs (same moment: CSV + audit + Atlas screenshot + IRT screenshot)

| date | CSV time | panel | verdict | checks | note | run |
|---|---|---|---|---|---|---|
| 2026-09-16 | 09:47:09 | 16.26 (pre-suite, manual) | PASS (manual) | rail vs CSV vs Atlas row-for-row; King 7685 = 7689.25 = 7755 Dec; SCALEREF 7614.06 vs ES1 7613.50 | done by hand in-session before the runner existed; became fixture A | — |
| 2026-09-16 | 13:25:02 | 16.29 (DLL 0.45) | FAIL 26/29 | 18/18 Gate A · Atlas King/ES1 match · FLIP/PW/F/K/bracket/regime seen | 3 fails: 7630 Atlas "−50" was the VELOCITY chip, value 0 — RESOLVED in run 2 (transcription error, parser right); C on 7685 off-pane; badge 5 snapshot gap | [1325-live1](runs/2026-09-16/1325-live1.md) |
| 2026-09-16 | 14:26:21 | 16.29 | FAIL | 23/28 | SECOND LIVE RUN, FOMC afternoon (ES -100 pts in the hour). CSV 14:26:23; Atlas z | [1426-live2](runs/2026-09-16/1426-live2.md) |
| 2026-09-16 | 14:26:23 | 16.29 + companion 1.20 | PASS-with-notes 23/28 | 18/18 Gate A · Atlas King == panel · 21/22 zoomed Atlas cells == tape · every visible IRT value == CSV · PW/badges/regime+conflict verbatim | 5 'fails' all resolution/timing: a 3% cell's sign by colour; ES1 vs SCALEREF 1.1 pt on a 100-pt/hour tape; K/F below the pane, C unreadable at ultrawide res. Finding: IF payloadT 30 min behind on FOMC — conflict flag fired as designed | [1426-live2](runs/2026-09-16/1426-live2.md) |
| 2026-09-17 | 11:18:47 | 16.40 | PASS | 20/20 | first 16.40 export: the pool on the SPX file | [1118-pool-SPX](runs/2026-09-17/1118-pool-SPX.md) |
| 2026-09-17 | 11:18:47 | 16.40 | FAIL | 18/21 | first 16.40 export: the SPY file | [1118-pool-SPY](runs/2026-09-17/1118-pool-SPY.md) |
| 2026-09-17 | 11:18:47 | 16.40 | PASS | 21/21 | first 16.40 export: the SPY file | [1118-pool-SPY](runs/2026-09-17/1118-pool-SPY.md) |
