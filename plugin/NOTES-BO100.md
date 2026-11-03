# lsBreakout 1.0.0 ("BO") + lsBigBidAsk 1.32.0 - 2026-11-03

## Request
Rassul approved the mockup `bo_mockup.html` (ES Oct 30, the real decisions lsBigBidAsk 1.31.3 recorded that day:
"BO / BD sit small, right at the breakout candle (BD just below its low, BO just above its high), letters only, shown once
the follow-through confirms") and said **"build"**.

## What was built
**New indicator lsBreakout 1.0.0** (`Breakout.cpp` host, `BreakoutLogic.h` SDK-free logic, namespace `bko`). The price chart
gets ONLY letters:

| Letter | Where | Colour | Font |
|---|---|---|---|
| BO | just above the breakout candle's high (bottom edge = Y(high) - 3 px) | green 0x22C55E | 7 (~9 px), bold |
| BD | just below the breakdown candle's low (top edge = Y(low) + 3 px) | red 0xEF4444 | 7, bold |
| Rly | end of a rally (above) / decline (below), at least 16 px further out than the BO / BD row | green / red | 9, bold |

- No prints, lots, numbers, boxes, hovers or info box. No settings: `SS_SAFE_DIALOG` + empty `parms*Body` (as lsCandleAbsorb), `setParameterVersion(1)`.
- 1 px black edge round each letter (lsBigBidAsk's letter drawing, kept so the letters read on any candle).
- Never under the left Liquidity / Key Levels column (`cb::leftKeep`), the IRT title row, the Tape Reader banner or the HOD/LOD table; a letter with no room is dropped (counted in the status), never drawn outside the pane.
- Posts SignalBoard kind `'L'` (the BO letters as drawn, px from the candle's extreme to the letter's outer edge - incl. a Rly on the same side) so lsLiquidityProfile's Ex keeps stacking against it, as lsBigBidAsk 1.31.1 did.
- Calculated natively from the chart's bars (OHLC + volume). No file is read at run time. Writes only `lsFlexLevels\Breakout.status-<MKT>-<spb>.txt` (version first, today's counts, drawn counts; on change / 30 s).
- Closed candles only; decisions are kept per chart (host slot) keyed by bar time and never changed or removed once made (no repaint). First pass = the whole chart (up to 20,000 bars); each new closed bar = lsBigBidAsk's live window (last 300 candles, state from 60 before).
- Rly on 3-min charts only (the rule was built and tested on 3-min bars), as in lsBigBidAsk.

**Same decision, one code path** (`BigBidAskLogic.h` 1.32.0): `bba::boFromCandles` builds every input `boWaves` needs (full-session
swings `swingBreaksPx`, wave `zigDir`, whole-tick keys, bar volume, vwapTrend at candidate candles, session keys) and runs `boWaves`;
`bba::boWindow` = lsBigBidAsk's live window; `bba::boDuplicate` = its "already covered" rule. lsBigBidAsk's `boPass` and lsBreakout
both call them, so the two cannot drift. Rly: both call `rly::run` (RallyLogic.h 1.1.0, unchanged). The prints never change a
decision (they only feed lsBigBidAsk's box number), so lsBreakout passes none.

**lsBigBidAsk 1.32.0** (`BigBidAsk.cpp`, `BigBidAskSettings.h` PLUGIN_VERSION 1.32.0, `bba::VERSION` 1.32.0, setVersion "1.32.0"):
- STOPS drawing the BO / BD and Rly letters (`BBA_LETTERS = false`) and stops posting SignalBoard 'L' (lsBreakout posts it).
- Unchanged: every BO / BD and Rly decision and record (`lsFlexLevels\BigBidAsk\...-breakouts-*.csv`, `lsFlexLevels\Rly\*.csv`),
  all prints / boxes (incl. the BO print box in the BO colour), settings (parameter version 14 - no rows changed).
- `boPass` now calls `bba::boFromCandles` (identical inputs; proven equal in test [1] below).
- Settings rows: "Breakout (BO / BD)" and the BO colours still switch / colour the BO print boxes. The **Rly checkbox and Rly up / down
  colours now do nothing** in lsBigBidAsk (the letters are lsBreakout's, which has no settings). Left in place to avoid a parameter-version
  bump today; remove them in the next lsBigBidAsk settings change.

## Tests (all on Linux, ASan / UBSan, -Werror -Wall -Wextra)
`tests_bo100/run_tests.sh <real irtsdk.h folder>`:
- **test_bo100_parity** 9 / 9:
  - [1] `boFromCandles` == the 1.31.x inline inputs + `boWaves` on ES Oct 26, Oct 27 and Oct 28-30, full run and live window,
    every 9th prefix: 58 / 58, 58 / 58, 294 / 294 runs identical (every decision field).
  - [2] REAL parity ES Oct 30 (ESZ6 3-min candles built from `data\irt\2026-10-28..30\ES\fp_bars.csv`, replayed bar by bar):
    **BO / BD 20 / 20** recorded fired decisions matched (t0 + side; follow-through candle equal on 20), 0 extra;
    **Rly 7 / 7** matched, 0 extra - against lsBigBidAsk 1.31.3's records `ES-ESZ6-180-breakouts-20756.csv` / `Rly\ES-ESZ6-180-20756.csv`.
  - [3] no repaint: 361 decisions kept over the replay, 0 changed / vanished; a restart (one full pass) keeps all 52 fired BO / BD
    and 10 Rly of the replay, 0 new.
- **test_bo100_host** 21 / 21 (Breakout.cpp on the mock SDK `Breakout_test_support/irtsdk.h`): only texts drawn, only BO / BD / Rly;
  BO at Y(high) - 3, BD at Y(low) + 3, font 7, Rly font 9 >= 16 px further out; nothing on the forming candle; every fired decision
  in view drawn and no other candle marked; no overlaps; the black edge; SignalBoard 'L'; status file; version literals;
  bar-by-bar feeding == one pass == a restart; two charts on one DLL; 1-min chart = no Rly; chart-less settings copy; no-room drop;
  NaN / Inf bars.
- **MinGW vs the real irtsdk.h**: Breakout.cpp 0 errors / 0 warnings; BigBidAsk.cpp 0 errors / 0 warnings.
- **Render**: `tests_bo100/render-bo100.png` (ES Oct 30 RTH from the mock draw calls).
- Existing suites after the change: tests_bigba test_bigba_114 6272 / 6272, test_bigba_swing114 1376348 / 1376348;
  tests_bo130 25 / 25 (Oct 26) and 22 / 22 (Oct 27); tests_rly ALL PASS. Pre-existing failures, identical before and after this
  change (they test retired rules): tests_bo129 5 pass / 15 fail, tests_bo128 31 pass / 3 fail; tests_bigba test_bigba_logic /
  test_bigba_host do not compile against the current code (stale since 1.18 / 1.19).
- Settings contract (tests_settings/settings_contract): no BigBidAsk failure (setVersion "1.32.0" = PLUGIN_VERSION).
  test_dialog_guard.py: Breakout.cpp passes (SS_SAFE_DIALOG, SettingsGuard.h included and hashed in gex-build.bat).

## Build
`tools\gex-build.bat`: `call :one Breakout lsBreakout BreakoutLogic.h BigBidAskLogic.h RallyLogic.h VwapTrendLogic.h
SessionVWAPLogic.h SwingRef.h HostSlot.h SettingsGuard.h SettingsHost.h SettingsSpec.h ColumnBoard.h BoxPlace.h BannerBoard.h
BoxDock.h SignalBoard.h`, and `Breakout` added to the `:status` list. lsBigBidAsk rebuilds (BigBidAsk.cpp / BigBidAskLogic.h changed).

## Findings (not changed here)
- lsBigBidAsk's breakout record repeats lines: Oct 30 has 753 rows and its 141 fired lines hold only 21 distinct ones (most fired breaks written 8 - 10 times),
  and the 11:03 BD appears with two different follow-through candles (11:12 first, 11:06 later). The letter (t0 + side) is the same,
  so nothing he sees differs; likely several chart instances / restarts appending to the same session file. Worth a look.

## Not verified - needs his IRT
- The real look (font 7 in IRT's units, the 3 px gap, the Rly row) on the wide Samsung monitor in 2+ markets after the install.
- That lsLiquidityProfile's Ex letter stacks against lsBreakout's 'L' posts like it did against lsBigBidAsk's.
