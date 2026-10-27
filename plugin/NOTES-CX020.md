# lsContext 0.2.0 (2026-10-26): awareness_plus, the Market Awareness Layer additions

Design: `research-artifacts/awareness_plus/DESIGN.md`. Rassul approved it at 13:46 CT.

lsContext still **draws nothing**. Links stay **OFF**. `PLUS,0` in `LRA-Context-<MKT>.csv` gives the 0.1.0 behaviour exactly.

## Headers (SDK-free, `plugin/tests_plus`)

| Header | Namespace | Version |
|---|---|---|
| AwarePlus.h | `ap` | 0.1.0 |
| MarketProfile.h | `apm` | 0.1.0 |
| Expectations.h | `apx` | 0.1.0 |
| NewsGate.h | `apn` | 0.1.0 |
| SourceHealth.h | `aph` | 0.1.0 |
| Memory.h | `apmem` | 0.1.0 |
| CrossTf.h | `aptf` | 0.1.0 |

The namespaces are unique because `ex` / `ng` / `mp` are already used by ExhaustionLogic.h, HodLodExpectedV2.h and MarkParams.h.

`ContextPump.h` is now 0.3.0 and calls `ap::Plus`.

## What each bar does now

- **Before CTX(t).** Source health is checked: ok / lagging / stale / gone / missing. Bad sources are left out of CTX and TEAM. The news window and shock state are set. Open expectations are checked against bar t.
- **CTX(t)** gets these METRIC / DATA fields:
  - `M.news*`, `M.shock*`, `M.conf_x`
  - `M.src_*`, `M.excluded`, `M.reader`, `M.mbo`
  - `M.exp_*`
  - `D.mkt_*`
  - `D.mem_*` / `M.mem_*`, `D.on_*` / `D.lon_*`
  - `tf<spb>_*` from the other timeframes (another chart's team side is a READ).
- **TEAM(t)** gets `conf_x`, `news`, `excluded`, and `tf_agree` / `tf_against`.
- **Expectations.** An expectation is registered from each aware read decided for t.
- **Board sources posted:**
  - `EXP`, `NEWS`, `HEALTH`;
  - `TF` on the chart-agnostic key (`chart "any"`).

## Files

| File | Direction | Contents |
|---|---|---|
| `LRA-MarketProfile-<MKT>.csv` | read | personality, slot normals, K / N, news windows, expected sources |
| `LRA-Memory-<MKT>.csv` | read | memory beyond today |
| `IRTReader.status-<MKT>.txt` | read | the reader heartbeat |
| `LRA-Recording.csv` | read | the MBO flag, written hourly by `lra.rec_watch` |
| `.rec` | append | `EXPREG` / `EXP` / `NEWS` / `HEALTH` lines |
| `.status.txt` | write | adds `PLUS` / `PROFILE` / `NEWS` / `HEALTH` / `EXP` / `MEMORY` |

The three read-side files are re-read every 30 s. A missing file means built-in defaults or `?`, never a crash.

## Tests

Run `sh tests_plus/run_tests.sh <sdk include>`:
- test_plus 66/0
- test_pump_plus 18/0
- test_next 20/0
- e2e: 280 verdicts
- real-SDK syntax: 0 errors

The smart_indicators suites on the patched pump: test_pump 40/0, test_context 43/0, test_hook 29/0.
