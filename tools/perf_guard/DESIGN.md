# IRT Performance Guard (PerfGuard) — design

Rassul, 2026-10-26 13:47 CT: "do you have a way of optimizing irt and how it uses memory so there is no degredation". At 13:49 CT he replied "ok. do it" (estimate ~3–4 h, $0).

The guard goes **first** in the commit order, so the Market Awareness / self-awareness / ask layers are measured against a "before" number.

## 1. PerfGuard.h 1.0.0 (one header in every plugin DLL)

`LSPERF_SCOPE(LSPERF_PLUGIN, "calc"|"draw"|"preCalc")` is placed at the top of every real calc / draw / preCalc body. The functions it covers:

- `cppExtension::` methods;
- methods of a class derived from it;
- methods of a class reached through `static_cast<X*>(this)`;
- in-class definitions.

A bare `return RTX_OK;` or a one-statement forward is skipped, so nothing is timed twice.

| What | How | Where it goes |
|---|---|---|
| Time per calc / draw, per chart (the extension pointer) | `std::chrono::steady_clock` (QPC on MSVC). Per minute: n, mean, p95 from a 24-bucket histogram (capped at the max), max | Perf CSV row (one per chart per minute) |
| Calc gap (user-visible chart lag) | Time between calcs of one chart, p95 in seconds | CSV `gap_p95_s` |
| IRT memory | Working set + private bytes once a minute via `K32GetProcessMemoryInfo`, looked up at run time (no psapi link). Leak trend = least-squares MB/h over the last 60 minutes (needs 30+ samples) | CSV + board `M.ws_mb`, `M.priv_mb`, `M.mem_slope_mb_h` |
| Feed lag | `OrderFlowFeed.h` passes each event's exchange stamp to `lsperf::noteEvent` (one relaxed atomic store). Lag = wall clock − stamp. UTC and local readings are both tried; implausible = `?` | CSV `lag_s`, board `M.lag_s` |
| Board size | Bytes and count of the `LRA_*` process-environment entries | CSV `board_kb` / `board_n`, board `M.board_kb` |
| Perf-file write time | ms of the minute's single batched append | CSV `write_ms`, board `M.write_ms` |

**Files**

- `lsFlexLevels\Perf\perf-<plugin>-<yyyymmdd>.csv`: CRLF, header on a new file, one append a minute per DLL.
- A row that cannot be written is kept for the next minute (capped at 4,000).

**Board**

- `LRA_PERF_<plugin>` = `"<ms>|<minute>|M.calc_p95_ms=..;M.draw_p95_ms=..;...;over=ESZ6 NQZ6;v=1.0.0;"`, in the MarketState encoding.
- `LRA_PERF_OVER_<plugin>_<sym>` = `"<utc>|<why>"` while that indicator is over budget on that market.
- `LRA_PERF_MEMOVER` while IRT is over its memory budget.

**Budgets**

- File: `lsFlexLevels\LRA-PerfBudget.csv`, columns `plugin,calc_ms,draw_ms,mem_mb`. The `*` row is the default (20 / 30 / 0, where 0 means observe only).
- The file is re-read when its stamp changes, checked at most once a minute.
- A minute is over when its calc or draw p95 is above budget.
- Three over minutes in a row put the indicator **OVER**. It posts the flag, and the CSV gets an EVENT row "OVER … (smart features off)".
- Ten good minutes in a row **CLEAR** it.

**Auto-off**

- `lsperf::smartAllowed(plugin, sym)` returns false while that indicator is OVER on that market, or while `LRA_PERF_MEMOVER` is set.
- A flag older than 15 minutes is ignored, because its writer has stopped.
- The SMART layers must call it before their extra work: Market Awareness links (ContextPump / AwareHook), self-awareness, and ask.
- The basic indicator keeps running.
- `lsperf::statusOf(this)` gives "perf ok" or "perf OVER … (smart off)" for status lines.

**Safety**

- There is no thread and no timer. The minute close runs inside the call that crosses the minute, and its time is not counted in that call.
- Every entry point is wrapped in `try`. A fault switches the guard off for that DLL.
- The chart table is capped at 64, and charts unseen for 10 minutes are dropped.
- `LRA_PERF_DISABLE=1` turns the guard off.
- The state is a per-DLL function-static keyed by the extension pointer. This is a documented exception to BUILD-LESSONS #1, because it is instrumentation, not indicator state.
- `NOMINMAX` is set before any `windows.h`, and no macro-named identifiers are used.

## 2. apply_perf.py 1.0.0 (the patcher, run on fresh copies)

1. Inserts `#include "PerfGuard.h"` plus `LSPERF_PLUGIN "<Name>"` right after the top-level `#include "irtsdk.h"`, never inside an `#if`. A wrapper (LiquidityFlow.cpp, TrappedMarks.cpp) defines its own name before it includes the shared .cpp.
2. Inserts the scopes, one line each.
3. Bumps the version:
   - the `setVersion` literal, or the constant passed to it;
   - every equal literal in the same .cpp;
   - the plugin's own mirrors in its headers: `PLUGIN_VERSION` in `<Name>Settings.h`, `VERSION` in `<Name>Logic.h`, `BAS_VERSION` / `BF_VERSION` in OrderFlowPaneSettings.h, "keep equal" lines.

   It never bumps a coincidental equal string in an unrelated header.
4. Adds the `OrderFlowFeed.h` lag hook, guarded by `#ifdef LSPERF_H` so other builds and tests are unaffected.
5. Appends ` PerfGuard.h` to each patched plugin's `call :one` line. CRLF is kept, and an LF file is refused.

Any anchor not found is an error, and nothing is written. Running it a second time changes nothing.

**Dry run on the 13:14 staged sources**

- 34 plugins are instrumented, 0 warnings.
- The 16 build-list names whose source is not in plugin\ are skipped:
  - SessionInfo, FirstExtreme, Pulling;
  - FootprintReader (×7 markets);
  - TapeFlow (×7 markets).
- **All 10 plugin test suites pass** on both the original and the patched trees.
- **The real-SDK MinGW syntax check** gives identical results, original vs patched:
  - 28 OK and 3 WARN, the same warnings in both.
  - 3 pre-existing FAILs come from headers missing in the staged copy: DeltaProfileHourlyParams.h, OrderLinesLogic.h, and BO_R_NO_CONFIRM. The fresh re-stage must bring them.

## 3. perf_lint.py 1.0.0 (build-time checks)

The lint works on the plugin .cpp files plus the headers each one hashes. Checks:

- **P1:** a loop over ALL bars inside calc / draw.
- **P2:** file I/O directly in calc / draw.
- **P3:** a board write inside a loop.
- **P4:** a member container grown with no cap, erase or clear in the file.
- **P5:** loops in calc / draw with no cap word in the file.
- **P6:** a board writer with no size cap.

Each finding has a key that stays stable when lines move. `perf-lint-baseline.json` holds today's findings, which is the "before" number: 68 findings (P4 65, P5 2, P6 1, mostly review items). `--gate` fails only on **new** findings, and `plugin/tests_perf/run_tests.sh` runs it.

Honest limit: the lint is a heuristic that reads only the calc / draw bodies, not the helpers they call. The runtime timing is the real measure.

## 4. lra.irt_perf 1.0.0 (nightly part, engine `irt_performance`, item `irt_perf`)

**Input:** the day's perf CSVs, the budget file, `tools\gex-build.log` (the plugin version timeline) and the ChartView exports.

**Output** goes to `data/irt_perf/`:

- `scorecard_latest.json` / `scorecard_<day>.json`;
- `history.csv`, one row per indicator per day; a re-run replaces that day;
- `report_<day>.md`;
- `LRA-PerfBudget.suggested.csv`, which is a proposal only.

**Contents of the scorecard and report**

- Per indicator: calc / draw p95 (median of minute p95), the worst 5%, the worst minute, the budget, the % of minutes over, smart-off events, the worst minutes, and status OK / WATCH / OVER.
- Per chart: bars and days loaded, minutes, calls, p95, gap.
- Memory: start / end / max private MB, slope in MB/h, and a **leak flag** when the slope is ≥ 50 MB/h over ≥ 3 h with ≥ 300 MB growth.
- The open (08:30–08:45 CT): feed lag (median and max) and calc gap p95.
- Settings review:
  - intraday charts holding more than 30 days of bars;
  - **hidden charts**, which calculate for 30 minutes or more but never draw;
  - **duplicate** indicators (same plugin, symbol and bar size from two charts);
  - ChartView exports.
- **Regressions:** a p95 at least 1.5× the median of the prior 5 sessions and at least 2 ms worse. Each one carries the build version before and now, and a **ROLLBACK PROPOSAL** that needs Rassul's yes. Nothing is rolled back automatically.

**Status email rows** are `email_rows` in the scorecard.

**Learning and engine**

- One `lra.learn` record (stats, settings findings, regressions as misses).
- An `lra.engines` score for `irt_performance`: the share of indicators inside budget.

**Enrolment** is done by `apply_lra.py`, using anchors on fresh copies:

- engines.py 1.0.1 (+`irt_performance`);
- learn.py (+item `irt_perf`, patch +1);
- parts.json (+"IRT performance guard (PerfGuard)" → `irt_perf:nightly`).

The part always returns `ok: True`. The day's verdict (PASS / WARN / FAIL / NO DATA) is reported in the scorecard instead.

## 5. Tests

| Suite | Result |
|---|---|
| `tests/test_perfguard.cpp` (fake mode, ASan/UBSan, `-Werror`) | 58 checks. ASan found a real dangling-pointer bug in `flagFresh`, now fixed. |
| Windows-macro simulation (C++14) | pass |
| MinGW real-SDK wrapper | pass |
| MinGW of the Windows branch | builds |
| `tests/test_apply_perf.py` | 3 tests |
| `tests/test_perf_lint.py` | 4 tests |
| `lra/test_irt_perf.py` | 9 tests, on a REAL PerfGuard session written by `tests/e2e_writer.cpp`: 4 charts, 410 minutes, OVER/CLEAR, leak, lag, hidden chart, duplicate. Integration with the real learn.py / engines.py was checked: inbox record written, engine scored, review lists it. |

## 6. What the other layers must do

Their SMART work for indicator P on market S must be wrapped in `if (lsperf::smartAllowed(P, S)) { ... }`. This covers:

- ContextPump / AwareHook (Market Awareness links);
- the self-awareness hooks;
- the ask layer;
- awareness_plus.

PerfGuard.h is already on every patched build line, so including it costs nothing.
