# lsBreakout 1.1.0 (+ "Pb") + lsWavePOC 1.8.3 - 2026-11-03

## Request
Rassul approved the Pb mockup (`bo_mockup.html`, ES Oct 30: "Pb = any pullback pivot that TESTS (overlaps) the BO / BD candle, until
the next BO / BD or the wave origin breaks") and said **"build"**. Pb is now drawn only by lsBreakout (Wave POC's POC-pullback Pb goes).

## What was built
**lsBreakout 1.1.0** (`Breakout.cpp`, `BreakoutLogic.h` 1.1.0, namespace `bko`). BO / BD / Rly decisions unchanged (same code path).

Pb rule (`bko::Engine::pbPass`):
- Pivots = the 3-bar High / Low ZigZag: `zzd::Engine::step(..., COMBO_REV_BARS = 3)` from `ZzoDeltaLogic.h` - the same engine as
  lsZzoCombo 2.1.0 / lsZzoZigZag (a down wave turns up on a bar whose HIGH is above the previous 3 bars' highs; an up wave turns down
  on a LOW below the previous 3 bars' lows; a strictly higher high / lower low moves the extreme; highs / lows only). Run over the
  chart's closed bars; a pivot is confirmed by its turn bar.
- After a fired BO on candle i: every confirmed pivot LOW p > i whose bar overlaps the BO candle (pivot low <= BO high AND pivot
  high >= BO low) gets a green "Pb" just below its low. Stops at (a) the next fired BO / BD (any side) whose candle is in (i, p], or
  (b) any bar in (i, p] trading below the BO wave's origin = the last pivot LOW before i. BD = the mirror (pivot HIGHs, red "Pb" just
  above the high, origin = the last pivot HIGH before i).
- Decided at the decision bar d = max(pivot confirm bar, BO / BD fire bar = follow-through close). Only BO / BD letters fired by d
  block it (the chart cannot know a later one), so a restart (one full pass) gives exactly the live set. Kept for the chart's life
  (map by pivot bar time): never changed or removed (a later invalidation does not remove an earlier Pb).
- Drawing: "Pb", font 7 bold (as BO / BD), 1 px black edge, green 0x22C55E below the low at Y(low) + 3, red 0xEF4444 above the high
  (bottom = Y(high) - 3). If another letter (BO / BD / Rly) is already there, one row further out (row height + 1 px, up to 4 rows);
  no room = dropped and counted. No numbers, no hovers. SignalBoard 'L' now also carries Pb bars (one post per bar and side, px to
  the outermost letter) so lsLiquidityProfile's Ex stacks past it.
- Status file: TODAY adds "Pb after BO / Pb after BD", KEPT adds Pb + pivots, DRAWN adds Pb.

**lsWavePOC 1.8.3** (`WavePOC.cpp`, `WavePOCLogic.h`, `WavePOCSettings.h` PLUGIN_VERSION 1.8.3, setVersion "1.8.3"): the POC-pullback
"Pb" text AND its cyan dotted line from the WPOC are no longer drawn (`wpc::SHOW_PB = false`); `pullbacks()` is still computed and the
status file says "POC pullbacks drawn now 0 (1.8.3: Pb is lsBreakout's; found in view, not drawn N)". A Pb no longer keeps an
old wave "in view". Nothing else changed. Wave POC's Pb was never logged / recorded elsewhere (only drawn + the status count).

**lsCandleAbsorb: NOT changed (stays 1.1.1).** It never drew a Pb: its codes A / Ae / D / S sit one row (ROW_OUT 16 px) out to keep
the row at the extreme free for his ratio / time label; only a comment mentioned Wave POC's Pb. Bumping it would ship an identical DLL.

## Tests (Linux, ASan / UBSan, -Werror -Wall -Wextra)
`tests_bo100/run_tests.sh <real irtsdk.h folder>`:
- test_bo100_parity 9 / 9 (unchanged): BO / BD 20 / 20, Rly 7 / 7 vs lsBigBidAsk 1.31.3's ES Oct 30 records, no repaint, restart.
- **test_bo110_pb 18 / 18** (new):
  - [2] PARITY with the mockup (r4.py) on ES Oct 30 08:33 - 15:00: **9 / 9 Pb, 0 extra** - 09:00 BD(08:33), 09:51 / 10:03 / 10:24
    BD(09:12), 11:00 BD(10:33), 11:42 BD(11:03), 12:45 BD(12:33), 14:12 / 14:30 BD(13:15).
  - [1] the plugin's pivots (full session from Oct 27 17:21) == the mockup walk (started at 08:33) from the first shared pivot
    (09:00) on: the mockup's 08:33 H / 08:51 L are start-up artefacts of starting the walk at the first RTH bar.
  - [3] bar-by-bar replay Oct 28 - 30: 53 Pb, 0 changed / vanished; each appears exactly at its decision bar's close; a restart
    (one full pass) = the live set (53 / 53).
  - [4] hand-made bars: overlap, next BO / BD stops it, origin break stops it, a BO / BD fired AFTER the decision bar or never fired
    does not block, a made Pb is never removed, layout (green Y(low) + 3, red Y(high) - 3, font 7, one row out over a BD, 'L' post).
- test_bo100_host 22 / 22 (updated): only BO / BD / Rly / Pb texts; every kept Pb in view drawn once on its pivot bar; no overlaps;
  black edge; 'L' = one post per bar and side with a letter; status "VERSION,1.1.0"; bar-by-bar == one pass == restart; two charts;
  1-min chart; chart-less settings copy; no-room drop; NaN / Inf.
- MinGW vs the real irtsdk.h (15.1.6): Breakout.cpp and WavePOC.cpp 0 errors / 0 warnings outside the SDK.
- Render: `tests_bo100/render-bo110.png` (ES Oct 30 RTH, all letters from the mock draw calls).
- Speed: pbPass on 20,400 bars with 509 fired BO / BD = 1.6 ms per closed bar.
- tests_wavepoc: test_wavepoc_150 49 / 49, test_wavepoc_logic 187 / 187, test_wavepoc_host 109 / 109 (updated: no Pb text and no Pb
  dashes drawn, pullbacks still found; version 1.8.3; a mutation with SHOW_PB = true fails 2 checks).
- tests_candleabsorb host 35 / 35 (unchanged plugin).
- Settings contract: no WavePOC / Breakout failure (setVersion "1.8.3" = PLUGIN_VERSION). Pre-existing, unrelated: test_dialog_guard.py
  flags WavePOC.cpp (no SS_SAFE_DIALOG - it has a real settings window) and CandleAbsorb's gex-build line (SettingsGuard.h not hashed).

## Mockup vs plugin
- The mockup confirmed pivots in hindsight and started the walk at the first RTH bar; the plugin runs the walk over the whole chart
  (overnight included) and decides each Pb live at max(confirm, fire). On Oct 30 the sets are identical (9 / 9).
- Where they can differ: (1) a next BO / BD whose candle is before the pivot but whose follow-through closes AFTER the pivot is
  confirmed - the mockup blocks the Pb, the plugin keeps it (it could not know); (2) the first wave of the day: the plugin has an
  overnight origin pivot / extra overnight BO / BD (the mockup had none before 08:33); (3) overnight Pb exist on the chart (the
  mockup drew RTH only).

## Build
`tools\gex-build.bat`: Breakout line + `ZzoDeltaLogic.h ZzoParams.h` (new includes). lsBreakout and lsWavePOC rebuild.

## Not verified - needs his IRT
- The look on the wide Samsung monitor in 2+ markets: Pb sits in the row right at the wick (as BD / BO), which is the row lsCandleAbsorb
  leaves for his ratio / time label - check it does not sit on that label.
- Pb is not yet measured (no per-market study); it is drawn as approved.
