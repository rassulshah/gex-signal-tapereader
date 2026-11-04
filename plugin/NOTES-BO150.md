# lsBreakout 1.5.0 - "BI" (BreakIn = a failed BO / BD) (2026-11-04)

## Request
Rassul, 2026-11-04 after 15:00 CT: approved the BreakIn mockup v0.3 (bi/breakin_mockup.html, ES) and said "build". Rule as approved:
BI DOWN = a failed BO, BI UP mirrors a failed BD.
1. Attempt = any BO / BD decision candle that traded through the BO pivot (higher of the last 2 pivot highs / lower of the last 2 pivot
   lows): unconfirmed (no_confirm or still waiting), fired, or any other reject that still traded through (wrong_colour,
   range_below_prior, not_10bar_extreme, ...).
2. BI candle = the attempt candle itself or one of the next 10 candles (a FIRED BO: the 10 after its follow-through candle, the BI after
   it) that is the opposite colour (red for BI down), has BREAKOUT RANGE (range >= the average range of the previous 10 candles AND
   (volume > the prior candle's OR range > the prior candle's)) and CLOSES back past the pivot (below it for BI down).
3. One BI per pivot (first qualifying). Decided at the candle close, never repaints. A BI while a BO / BD still waits for its
   follow-through cancels that BO / BD.
4. All sessions (the BO zigzag carries across 17:00 CT); the session is recorded.

## What changed
- `BreakoutLogic.h` 1.5.0: `Engine::biPass` (after boPass): attempts = every kept BO / BD decision + the ones still waiting at the last
  closed candle (`bba::BoRun::pend`, kept in `pendAtt`); each attempt's window scanned with `biQual` (whole ticks: colour, range x 10 >=
  sum of the previous 10 ranges, volume or range up, close back past the pivot); candidates taken in candle order (ties: the earlier
  attempt), the first per pivot + side (key = pivot ticks * 2 + side) is the BI, kept for the chart's life. Cancel: a fired BO / BD with a
  BI on its pivot at a candle in [t0, follow-through) goes into `boCancel` - never drawn (`shownBo`), no Pb (pbPass skips it), its BBO
  line becomes fired 0 / reason `bi_cancel`. `bba::` code and lsBigBidAsk untouched (BigBidAskLogic.h not changed).
  Layout: text "BI" only, BO font (7), red just above the high (BI DOWN) / green just below the low (BI UP), the BO row (+3 .. +17 px);
  a row taken moves it one row out (16 px): rows 0 + 1 when the candle is a 3-bar zigzag pivot on that side (wave-time label + the
  lsCandleAbsorb A / Ae row), row 1 on the candle after a pivot that re-tests it within 4 ticks (lsCandleAbsorb's next-bar A), row 3 (+51,
  LVP) never; and any of our own letters (BO / BD / Rly / Pb) box by box. No free row inside the price area = not drawn, counted "no room".
  One "BI" per candle and side (a candle can be the BI of two pivots: one letter). Posted in SignalBoard 'L' with the other letters.
- Records: BBI| lines in the existing `lsFlexLevels\Breakout\<MKT>-<SYM>-<spb>-breakouts-<daykey>.csv` (same dedupe: key `I|t_bi|dir|pivot`,
  read before each append), the file's first line adds `| bi 1`, a third line `# BBI|...` (comment) names the columns:
  `BBI|t_bi|dir|t0|t_fire|kind|waiting|pivot|stop|bi_high|bi_low|bi_close|range|avg10|prior_range|vol|prior_vol|vol_up|range_up|daykey|session|version`
  kind = unconfirmed / fired / other (the attempt as known at the BI close); waiting = 1 when the attempt still waited for its
  follow-through (it can never fire now); stop = the attempt's extreme from its candle to the BI candle; range / avg10 / prior_range in
  ticks; session = RTH / ON / POST; version `1.5.0-zz3`. Files existing from 1.4.0 keep their old first line.
- Status: `BI,bi_shown 1,label BI (plain - no ?),...,today BI down x,BI up y,BO / BD cancelled z,...,drawn n`; RECORDS line `,bi 1`.
- `Breakout.cpp` 1.5.0: version literals, description, status RECORDS. No settings, no hovers, no new includes (gex-build line unchanged).

## Parity and the one deliberate difference
- With `biCancel = false` (test switch) one full pass == the v0.3 prototype (run5.py AVGX 1.0, N 10, "close", regenerated): 271 / 271 BI,
  every (candle, side, pivot, attempt) equal.
- As built (cancel on): 272 BI + 8 BO / BD cancelled on ES Oct 20 17:03 - Nov 3 19:45. vs the prototype: 3 BI lost, 4 added, all 7 on
  pivots whose BO / BD the BI cancelled. Why: the prototype knew, at every candle, whether a waiting BO would LATER fire, and for a fired
  BO only looked after its follow-through. The live chart cannot know that while the BO waits; with the approved cancel rule the candle
  that closes back past the pivot while the BO waits IS the BI (and the BO never fires). Equal to an independent Python model of the
  same rule (272 + 8, 0 differ) and to the LRA port (bo_pb 1.2.0).
  The 8 cancelled (file times): Oct 22 01:30 BD, Oct 22 06:15 BD, Oct 23 08:30 BD, Nov 1 23:12 BO, Nov 2 13:39 BD, Nov 2 21:36 BD,
  Nov 3 00:48 BD, Nov 3 11:51 BO. BO / BD letters drawn on ES: 152 -> 144. NQ week: 238 BI, 5 cancelled; GC: 259 BI, 13 cancelled.
- Reference cases (file times): Oct 21 13:18 BO -> BI DOWN 13:48 (not the tiny 13:24); Nov 2 08:33 / 08:39 BD -> BI UP 08:42; Oct 22
  09:06 one-candle BI DOWN (7742.50 through 7740.75, close 7734.75); Oct 22 08:48 BD -> BI UP 08:57 (the same candle is also the BI of
  the 08:54 attempt's pivot 7720.50 - one letter). ES BI by session: RTH 85, ON 175, POST 12.

## Tests (tests_bo100/run_tests.sh <real irtsdk.h folder>, ASan / UBSan -Werror)
- NEW test_bo150_bi 24 / 24: [1] prototype parity 271; [2] built rule == Python model (272 + 8), delta vs prototype 3 / 4 all on
  cancelled pivots; [3] the 4 reference cases; [4] fired-kind BI after the follow-through within 10, others within attempt + 10, each
  cancel (BI in [t0, follow-through), not drawn, no Pb, bi_cancel line), one per pivot; [5] bar-by-bar replay: no BI / cancel ever
  changed or vanished, none on an unclosed candle, replay == one pass for every decision, record lines identical; [6] labels: every BI in
  view drawn once, row offsets exact, never a reserved / LVP row, no overlaps, a forced letter moves the BI out, SignalBoard; [7] BBI
  lines / header; [8] NQ / GC replay == one pass.
- test_bo100_parity 9 / 9, test_bo110_pb 21 / 21, test_bo120_week 45 / 45 - run with biCancel = false (the 1.4.0 decisions unchanged).
- test_bo100_host 23 / 23 (+ BI drawn / colours / rows; a BI with no free row at the pane top is dropped and counted), test_bo130_rec
  21 / 21 (BBI lines once per BI through restart / recompute / two charts / bar-by-bar, `| bi 1` header, bi_cancel lines), render check.
- MinGW vs the real irtsdk.h: see the report line below.
- LRA: `lra/bo_pb.py` 1.2.0 `bi_decisions` / `apply_bi_cancel` / `read_bi_records` (+ `bo_decisions(..., pending=)`), equal to the
  C++ harness (tests_bo100/harness_bi150.cpp) on both rules: analytics/test_bo_pb_bi.py 5 / 5 (in run_tests.py MODULES), fixtures
  `bo_pb_es_bi_oracle_2026-10-20_11-03.txt.gz` / `..._proto_...`; test_book_level_combo 14 / 14 unchanged.
  `lra/orderflow_badges.py` 1.6.0: BI-D / BI-U scored nightly in breakout_reads (continuation in the BI direction until a close back
  beyond the attempt's extreme vs the failed side's attempts that got no BI; FAIR + extreme-holds, in LRA-Badges-<MKT>.csv, DRAWN so
  a failing BI opens a ticket); bi_cancel lines are no longer BO / BD controls. test_orderflow_badges 27 / 27.

## Risks / not done
- The label rows of OTHER indicators are inferred (3-bar zigzag pivots for the wave-time label and lsCandleAbsorb's A rows; IRT's own
  ZigZag can differ by a bar); the LVP row is always skipped. Check his chart.
- An attempt dropped later as a duplicate fired BO (same pivot) is still an attempt while it waits (none on the ES / NQ / GC weeks).
- book_level_combo's Pb zones for days WITHOUT records still come from bo_pb.bo_decisions (no cancel): a cancelled BO's Pb zone stays
  there (8 per two weeks on ES); record days are right (cancelled lines are fired 0). Follow-up: apply_bi_cancel there.
- BI shows as plain "BI" (approved label), not "?" - no study has passed yet; the nightly now scores BI-D / BI-U from the records.

## Not verified - needs his IRT
- The BI letters on his ES chart after the restart (first: tonight's evening session); no overlap with the wave-time labels / A / Pb / LVP.
- Status file: `VERSION,1.5.0`, the `BI,bi_shown 1,...` line; breakouts file: `| bi 1` + BBI lines (new files from the next daykey).
