# lsTrappedTraders 0.1.0 "LRA Trapped Traders" (2026-10-26 evening)

Rassul 20:14-20:15 CT: "this makes the other underwater trader better" / "lets just call underwater traders trapped traders. build it.
its also simple and complements defense better right ?" 20:35: marks at levels only (default), bars always.
New plugin (lsTrapped 1.0.x is untouched). Add it on the 3-min charts under the Defense pane.

## The pane: "Who is trapped?"
- Two upward bars per CLOSED candle. GREEN (left) = SELL aggressive lots traded BELOW the candle's close (sellers trapped).
  RED (right) = BUY aggressive lots traded ABOVE the close (buyers trapped). Green bigger = bullish, red bigger = bearish. No exceptions.
- Stage 3 TRIGGER: at his 1-bar ZigZag pivot (a new low that is the lowest of the down-wave; mirror for highs), the first candle from
  the pivot through 3 after it, with no new extreme and his pivot rule (j == pivot or low_j < high of the pivot candle), where the turn
  side's bar becomes the bigger one (green > red now, not before; mirror). Drawn as a small triangle over the winning bar + the
  ratio (2.0x, capped "10x+"). Decided once and recorded (TTT line); a restart reads the record, never repaints.
- Setting "Trigger marks": Levels only (default) / Everywhere / Off. Levels = his trade levels (ONH/ONL, PDH/PDL, PFH/PFL, LonHI/LonLO
  + gamma levels, NOT PW / VA / POC / DPoc / WPoc / VWAP) within 2 x the market's zone half-width of the extreme, with the direction rule
  (price came INTO the level: a low needs a high above level + band in the 10 candles before, mirror). Bars are always drawn.
- Live box (top-left, only on a pane >= 160 px): Sellers | Buyers - "Lots trapped" and "vs normal" (the lots vs half the median green +
  red of the same 30-min slot over up to 20 earlier sessions, shown 2.0x). No grey sub-text. Bars and marks live strictly below the
  title and the box (pure layout, tested on 3,000 random panes).
- Hover any candle: question / numbers / what / how / use / example (his format).
- Other settings: Show ratio, Show live box, sellers / buyers colours. All on the shared settings layer (contract tested).

## Data source (deviation from the spec text)
VAP (IRT volume at price per candle: sellVolume = sold at the bid, buyVolume = bought at the ask) instead of the OrderFlowFeed MBO feed:
the same aggressor lots, available for all history, no MBO dependency, same source as lsTrapped. The nightly part re-computes the
same bars from the Databento trades store and the quality check compares the two.

## Files and records
- plugin/TrappedTraders.cpp, TrappedTradersLogic.h (pure logic, namespace ttr), TrappedTradersSettings.h (ttrset).
- lsFlexLevels\TrappedTraders\<MKT>-<CONTRACT>-<spb>-<sess>.csv append-only: TTB|t|sess|g|r|norm per candle, TTT|t|kT|sess|side|ratio100|ext|at|level.
- TrappedTraders.status-<MKT>-<spb>.txt (incl. PERF and AWARENESS lines); TrappedTraders.settings-<MKT>.txt.
- Board post TRAPTR (MarketState, DATA class) only when lsperf::smartAllowed; calc / draw under LSPERF_SCOPE.
- Source tags: WINDOW 3 = RASSUL; APPROACH 10, NORMAL_SESSIONS 20, SLOT_MIN 30, band 2 x zone = PLACEHOLDER (measured nightly).

## Tests (all on real data where data matters)
- tests_trappedtraders/run_tests.sh (ASan + UBSan, -Werror): 21 / 21. Green / red equal the Python reference on every candle of
  ES 10-26 (his recording) and NQ 10-22 (Databento store); triggers equal the Python reference (22 ES, 25 NQ); a restart after EVERY
  candle gives the identical trigger list and record lines (no repaint); a decided trigger never changes as later candles arrive;
  levels + direction rule; ratio text; hover rows; bad record lines; trimming; settings defaults; no overlap on 3,000 random panes.
- Real-SDK MinGW syntax (wrap_TrappedTraders.cpp): 0 errors, 0 non-SDK warnings.
- Settings contract: PASS 0 failures (proved to catch a wrong setVersion literal in TrappedTraders.cpp).
- tests_perf: PASS; perf lint: 0 new vs baseline (usedT_ trimmed with the bars, mark placement capped at 2,000 boxes).
- analytics/lra/test_trapped_traders.py: 4 / 4 (trigger parity with the same fixtures, outcomes, a full night on a synthetic store
  with records + Defense, bad record lines).

## Nightly part trapped_traders:nightly (item 2e.3, before learn:nightly)
Champion B0 (the plugin's bars) vs challengers V5_3 (fuel left, 3 candles) and CMB3 (trapped fuel, 3 candles), same trigger rule,
outcome = 1 ATR from the trigger close before the extreme breaks, within 40 candles; context RTH / ON, Defense agreeing (BookFlow
records), ratio bucket; the plugin's own triggers (everywhere / at levels) as 'live'. First run on 2 store sessions per market
(10-21 / 10-22, everywhere, not levels-only): B0 42-47%, V5_3 49-52%, CMB3 41-49%; ES Defense agreeing 49% vs against 32%.
Promotion is a proposal only (20+ new-day cases, Wilson lower bound beats the champion).

## Mockup (ES 10-26, before the build)
Everywhere 22 marks: 8 right, 12 wrong, 2 open. Levels only 7 marks: 3 right, 3 wrong, 1 open. B0 green / red equal the
trapped_test B0 on all 56 candles.
