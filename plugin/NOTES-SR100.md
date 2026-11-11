# lsSR 1.0.0 (2026-11-10) - the SR indicator (stop runs)

## Requests (Rassul, 2026-11-10 CT)
- "ok.. lets just stick with this indicator, build it and lets call it stoprun. add it to the overnight process to automatically
  track and improve over time for all markets" -> "go" -> "so call the new indicator SR instead of StopRun".
- Design (mockup claude.ai/artifact/EctbmGWXrGd5cWxA7JSGKv, v10): one pane; "make the stop run bar gold with the ratio like 6.2 right
  outside the bar and just put the arrow right on top of the reversals bar"; "it should be at least a 5 bar high or low for it to be
  highlighted"; "add the ratio for the highlighted bars".

## What it shows (its own pane on the order feed, no settings)
| Element | Rule (decided on the closed candle, journalled, never repainted) |
|---|---|
| Bars up (green) / down (red) | the candle's strongest 1-second burst of small market orders (<= 5 lots) vs the median of the prior 10 candles |
| Bright bar + ratio | ratio >= 2.0x AND the candle makes a 5-candle high (buyers) / low (sellers); others are faded |
| GOLD bar + ratio | a stop run THROUGH a key / MenthorQ level: burst >= 2.0x, attackers >= 2.0x the other side, half of it inside 300 ms, 2+ ticks through, first 4 RTH visits of the level |
| Arrow | the candle that closes back across the level after a gold run that made a new RTH extreme, inside the market's window (GC 8, HG 6, CL 11, ES 6, NQ 7, NG 8, 6E 8 candles), <= 1.6 ATR deep. DRAWN on GC and HG (study: +0.44R / 39, +0.92R / 19); recorded only elsewhere until the nightly proves it |
| Gray diamond | on the arrow candle the attackers' side cancelled more than it added (pull) |

## Files
- SR.cpp (host, off::MboPane), SRLogic.h (sr::Engine, journal), SRParams.h (per market; written by the nightly lra.sr_score).
- Journal: lsFlexLevels\SR\<MKT>-<secondsPerBar>-<session>.csv (H / B / R / V / X lines, append-only).
- Levels: lsFlexLevels\LRA-KeyLevels-<MKT>.csv + the MenthorQ / dealer levels through NearLevels.h.
- tests_sr2\: t_sr.cpp (host, mock SDK), srrun.cpp (the engine as a CLI), parity.py / parity_v.py (vs the study), fx_export.py, fx\.
- Nightly: level-reversal-analytics analytics\lra\sr_score.py (part "sr_score:nightly" in data\self_improve\parts.json),
  research-artifacts\setups\SR.md, one line in the setup scorecard / morning email.

## Tests (2026-11-10)
- t_sr 19 / 19 on REAL order data (HG 2026-10-07 06:00-11:30, GC 2026-10-26 06:00-10:00): plugin journal = engine journal line for
  line; restart half way = journal prefix byte-identical and the same labels drawn; gold bars + ratios drawn, nothing outside the pane,
  no white outlines; versions. Also clean under -fsanitize=address,undefined -Werror (one signed overflow in the level hash found
  and fixed: unsigned arithmetic).
- Engine vs the study (parity.py): gold runs HG 21 / 21, GC and CL exact on key levels; arrows HG 19 / 19, GC 40 / 40.
- MinGW syntax check against the real irtsdk.h (wrap_real_sdk.cpp, -Dsmall=char): 0 errors, 0 warnings outside the shared headers.
- test_sr_score.py 5 / 5 (trade arithmetic, LEARNING changes nothing, arrow on / off needs 20+ trades and both halves, a challenger
  needs the check half, garbled lines / other files in the folder).

## Risks / not verified (needs his Investor/RT)
1. LIVE TIMING: live order stamps are whole seconds. The 200 ms / 300 ms / 1000 ms windows use the ARRIVAL clock inside the second.
   The study used exchange nanosecond stamps. Live ratios can differ from the study's; the nightly scores the LIVE journals, so the
   scorecard is the truth for what the chart draws.
2. Values exist only from the moment SR is on a chart (no historical order feed); after a restart the journal is redrawn.
3. Live key-level file may not carry every level of the study (Asia / PF / prior week): fewer gold runs than the study on such days.
4. Swing levels of the study are not used live.
5. The nightly can only TIGHTEN recBars / burstX from journals; loosening needs the order replay (srrun), not automated.
6. Display on his wide monitor not seen yet: screenshot after the first session.

## Lesson (add to BUILD-LESSONS at the next shared edit)
A study's sub-second rule cannot be promised live as measured: Rithmic stamps are whole seconds. Say it before the build and score
the live record separately (lsSR 1.0.0; t_sr feeds whole-second stamps for this reason).
