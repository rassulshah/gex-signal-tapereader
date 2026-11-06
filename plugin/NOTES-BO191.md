# lsBreakout 1.9.1 (2026-11-06 12:15-12:35 CT)

## 1. Rev simplified
Rassul 12:15: "lets simplify the pb rev .. remove the stop line, t1 and t2 to simplify it and requite the green bar to be 1.2*ATR(13)"
- The Rev bar = the first green bar (BD: red) from the Pb pivot bar on whose low is below the pivot bar's high (BD: its high is above the pivot bar's low), with **range >= 1.2 × ATR(13)** (Wilder).
- One Rev per wave.
- The "biggest of the last 5 bars" alternative is removed.
- The Stop / T1 / 1.618 lines of 1.9.0 are removed (bko::revLines, Scene::lines and the host's line drawing). Nothing but letters is drawn again.
- BRV| records: 14 fields.

## 2. Pb only on the wave after the BO wave
Rassul 12:35, GC 3-min screenshot (BO 12:21, Pb 12:24 inside the same up wave): "the pullback is supposed to be on the dnwave after the rally upwave, it cannot be in the same wave as the bo wave"

The BO / BD wave ends at the first 3-bar High / Low ZigZag pivot HIGH (BD: LOW) at or after the BO / BD candle. This is Engine::wav, zzd::Engine::step with PB_REV_BARS = 3, the chart's waves.
- A Pb pivot must come AFTER that pivot.
- A Pb is decided no earlier than that pivot's confirmation.
- The green / red close pivot rule (1.8.0), the overlap with the BO / BD candle and the limit of 2 per BO / BD are unchanged.

Test weeks:
| Market | Pb in 1.9.0 | Pb in 1.9.1 | Rev in 1.9.1 |
|---|---|---|---|
| ES | 173 | 151 | 55 |
| NQ | 171 | 137 | 52 |
| GC | 160 | 125 | 40 |
| EU | 19 | 15 | 8 |

## Tests
| Test | Result |
|---|---|
| test_bo190_rev (ATR(13) re-check, no lines, 14-field records) | 25 passed |
| test_bo180_pb (+ [7]: every Pb after the BO / BD wave's end, independent zzd walk) | 19 passed |
| test_bo100_host (no lines drawn) | 24 passed |
| test_bo130_rec | 21 passed |
| test_bo161_onebar | 24 passed |
| MinGW against the real irtsdk.h, BO and BO 1-bar | 0 errors, 0 warnings |
