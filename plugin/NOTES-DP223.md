# lsDeltaProfile 2.23.0 - the drawn A is the researched A (2026-10-25)

Request (Sunday 2026-10-25 go, Iteration 6 parity, ITER6-HARDENING.md 2.4): "make the drawn A the researched A" - an A drawn only when
it is decided on its own candle or the next candle; late A's logged with their delay, never drawn by default; never repaint;
checkbox "Draw late A (not tested)" default off.

## Why the live A was late (diagnosis, ES / NQ RTH 10-19..10-22, 220 live A records on 104 candles)
1. The A rule (the node's candle closes beyond the node) was applied whenever a node FIRST reached the live top 3 (>= 2.0x), not at the
   candle's close. Median first-seen = 31 min after the candle (80% more than 1 candle late); the decision itself was then immediate
   (decided_by = the candle or the next one). 67% of these nodes kept growing after their candle: they only became top-3 once later
   selling / buying piled onto the same prices, or once older big rows left the sliding 90-min window.
2. The 90-min row grid is re-cut every build (ticks per row = span / 60, cut at fixed price steps, window including the forming candle),
   so one price area became new "nodes" with new edges: 53% of A records were a 2nd+ A on a candle that already had one
   (NQ 10-20 10:15 AM: 13 records between 10:42 AM and 11:42 AM).
3. The pivot wait (2.16.0) added little (16% decided > 1 candle after first seen) but drew a tentative A that could vanish.
4. Why the peak candles differ from the research detector: the research cuts rows from the 90-min LOW of closed candles only and
   requires the node to be top 3 AT the candle's (or next candle's) close; live counted any later top-3 node whose biggest candle was
   old. Only 17 of 104 live A candles were research A candles (and even those were drawn a median 15 min late).
5. The parity script shifted IRT stamps by +3 min; IRT peak_t is the bar END (183 / 183 live same-candle peaks close beyond their node
   with offset 0, 95 / 183 with +3).

## What changed
- DeltaProfileCore.h 2.23.0: `dp::researchA(window, tick)` = iter6 a6_indep.detect (60 rows from the window low, top 3, 2.0x, peak =
  this or the previous candle, close rule, 6-candle pivot rule, next-candle low / high vs the pivot bar).
- DeltaProfile.cpp 2.23.0: every closed candle of the session judged ONCE in order (W = 90 min / bar size; no A in the session's first
  90 min); RW|version|root|candle_end lines = the watermark (a restart never re-judges); A = D line, 29 fields:
  `...|conf=same|next|rule=research|delay_bars=0|1|delay_min=N|late=0`.
  The old path still runs AFTER the researched A has caught up; its A is code `AL`, `rule=old`, `delay_bars`, `delay_min`, `late=1`
  when > 1 candle; not drawn (unless the checkbox), never posted on the SignalBoard (even when drawn), no AE; shown as the gray circle it already was. No tentative A, no TA lines.
- Display unchanged from 2.22.0: one circle per candle, the shared A wins its candle, a coloured node A only on a candle closed in the
  defenders' colour - so a next-candle A (its candle closed in the attackers' colour) shows as the gray circle; its record is an A.
- Settings: "Draw late A (not tested)" (draw_late, default off), parameter version 17; old settings files load (missing key = off).
- Status file: RESEARCHED_A line (judged candles, watermark, A, AL logged, late, draw late on/off).
- Records of 2.22.0 and older (no rule field) count as old-path A's (drawn only with the checkbox).

## Tests (Linux, mock SDK, ASan/UBSan, -Werror)
- tests_dp217/test_dp223.cpp: 46 passed. run_tests.sh: dp219 7, dp222 14, dp223 46 - all pass. test_trap221 99 / 99 (version pins
  moved). test_dp217 / test_dp218 / tests_delta/test_repaint216 assert the retired tentative A and 25-field D lines: superseded, not run.
  tests_delta/test_rules285 already failed 19 / 36 on 2.22.0 (stale).
- Replay (tests_dp217/replay_dp223.cpp): research 3-min bars (MBO trade store) of ES + NQ 10-19..10-22 fed through the HOST candle by
  candle: Jaccard vs the research A on the decision candle 1.000 (35 / 35, RTH); 100% decided within 1 candle; a fresh engine after
  EVERY candle (record only) = the straight pass in 8 / 8 sessions. Core-only replay: 37 / 37 identical incl. zone edges and conf.
- MinGW -fsyntax-only with the real irtsdk.h: 0 errors; warnings only the 2 old TrapLogic.h misleading-indentation ones.

## Risks / not verified (needs his IRT)
- The replay feeds MBO-trade bars, not IRT's own VAP; live parity depends on IRT VAP = the exchange prints (the live peak candles'
  OHLC matched the research bars 183 / 183). Recheck after a session: live D lines rule=research vs the research detector.
- ETH candles are judged with the same rule (the research tested RTH only).
- First load per chart reads up to 30 VAP candles, then one per candle (cache); a fresh chart catches up 60 candles per pass.
