# Retired: lsDayModel and lsDayStats (2026-10-09, MT100)

Both indicators are retired. The HOD / LOD indicator and the HOD / LOD studies replace them. Nothing has been deleted. The sources, tests, build scripts and old builds now sit in `plugin\retired\` (moved by `MT100_retire_move.bat`), and `MOVE-LOG.txt` lists every file that moved.

## What replaces each output

| Retired output | What it showed | Where it is now |
|---|---|---|
| **lsDayModel**: expected "ghost" day candle (DAYEXP: E-open / E-HOD / E-LOD / E-close) | The day the model expected | **lsHodLod**: the E row of its table (1st time, reclaim, 2nd time), using the hle 2.0 model (`HodLodExpectedV2.h`: range band, room left, P(close > open), close location, timing). **lsSessionInfo 1.7.0**: E-HOD / E-LOD ticks on the expected-move slider |
| **lsDayModel**: actual developing candle (DAYACT), with HOD / LOD tips (value, time, duration), MUD box and swept-level ticks | The day so far | **lsHodLod**: the session candle at real prices on the left edge. It shows the HOD and LOD with their times, the key level each one swept (PFH / PDH / ONH / LonHI ...), took, size, gap, range and reclaim |
| **lsDayModel**: weekday header | Day of week | Not needed. The hle model includes day of week where the studies showed it helps |
| **lsDayStats**: A over E stats strip (DAYSA / DAYSE: 1ST, took, BOP, wick, W.End, wick %, MUD, 2ND, HL gap, HL range) | Actual against base rate | **lsHodLod** table (E over A: 1st Time, Reclaim, 2nd Time). Its status file `HodLod.status-<MKT>-<secs>.txt` holds every other value (took, size, rec. took, gap, room, range, close) |
| **lsDayStats**: the conditional E row (hodlodCondE, study-daystats-cond.py) | Expected clocks from the morning | **lra.hodlod_studies** (studies.py + fit_expected.py): these run nightly with a BH-controlled trial log and are forward-scored, and a refit is deployed only when it is no worse |

## Dependency inventory (checked 2026-10-09)

| Where | Reference | Action |
|---|---|---|
| `tools\gex-build.bat` | `call :one DayModel ...`, `call :one DayStats ...`, and both names in the `:status` list | **Removed** (MT100_gex-build.bat) |
| `plugin\compile-all.bat` | `call build-daymodel.bat`, `call build-daystats.bat` | **Removed** |
| `plugin\run-logic-tests.bat` | daymodel / daystats suites | **Removed** (the tests are kept in `plugin\retired\`) |
| `tools\regress.py` | daymodel / daystats in `ORDER`, versions read from `plugin\` | **Retired**: skipped with a note. The panel day-row suites still run, as **panelday** |
| `test_plugin_settings.js` | reads `plugin/DayStats.cpp` | Now reads `plugin/retired/DayStats.cpp` when the file has moved |
| `testing\REGRESSION.md` | map rows | Marked RETIRED |
| LRA nightly (`ri_nightly.py`, `self_improve_audit.py`, `hodlod_nightly.py`, `mq_nightly.py`, all of `analytics\lra`) | none | No change needed |
| Bridge (`analytics\mq_bridge.py`) and the LRA Reader (`reader.py` and the rest) | none | No change needed |
| `data\self_improve\parts.json` | none (never enrolled) | A `retired` list was added with the date. The audit reads only `parts`, so retired entries are never run or audited |
| GEX nightly (`tools\nightly\*.py`, `gex-nightly.bat`, `gex-sync.bat`, `gex-pull.bat`) | none | No change needed |
| `research-artifacts\frozen` (registry.json, log.jsonl) | none | Untouched. No frozen decision path depends on DayModel or DayStats |
| Userscript `current\gex-signal-tapereader.user.js` (16.52) | still writes the DAYACT / DAYEXP / DAYSA / DAYSE / ... rows into `lsFlexLevels\GammaProfile.csv` | **Left as is.** The rows are harmless: lsGammaProfile and lsKingTracker ignore them, and that file has not been written since 2026-09-23. They can be removed in a later panel version |
| `tools\gp-regress.py`, `tools\day-derive.js`, `testing\day-model\`, `testing\day-stats\` | manual same-moment tools and fixtures | Kept as history. They are not run automatically |
| `%USERPROFILE%\InvestorRT\dllx64\lsDayModel.dll`, `lsDayStats.dll` | IRT loads them at start (RTXReport: 0.2 / 0.18) | Your steps below. Use `MT100_park_irt_dlls.bat` last |

## Your steps in Investor/RT (do these yourself)

The indicator may be on any chart page, so check every market's page (ES, NQ, CL, GC, HG, NG, EU).

Step 1. Open Investor/RT and go to the first chart page.

Step 2. On each chart, look for the **Day model candle** (a ghost candle and a solid candle in a side margin, sometimes with a STALE badge) and the **Day stats strip** (a two-row A / E text panel in a corner).

Step 3. Right-click the chart and open its indicator list (Edit Chart / chart settings).

Step 4. In the list, select **lsDayModel** (description "Day model candle (expected + actual) ...") and remove it. Do the same for **lsDayStats** ("Day model stats strip ..."). Click OK.

Step 5. Save the page (File > Save, or the page's save button) so the indicator stays off when IRT reopens.

Step 6. Repeat steps 2 to 5 on every chart page and every market.

Step 7. Close Investor/RT completely.

Step 8. Double-click **`MT100_park_irt_dlls.bat`**. It moves the two DLLs (it does not delete them) from `InvestorRT\dllx64` into `InvestorRT\retired-dllx64`, so IRT stops loading them. If it says a DLL "could not move", IRT is still open: close it and run the file again.

Step 9. Reopen Investor/RT. Check that no chart shows a missing-indicator message. If one does, that chart still had the indicator: do steps 3 to 5 on it, or move the DLL back from `retired-dllx64` to `dllx64` and remove the indicator first.

## Undo

To undo, move the files back from `plugin\retired\` (and `plugin\retired\out\`) to `plugin\` (and `plugin\out\`), restore the two `call :one` lines in `tools\gex-build.bat`, and move the DLLs back from `retired-dllx64` to `dllx64`.
