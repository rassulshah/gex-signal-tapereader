# lsDeltaProfile 2.33.0 - "Intelligent Delta Profile" (2026-11-07)

## What it does
- Every closed 3-min candle runs DeltaIntel.h (dintel::Engine, SDK-free) over the session: letters A (absorbed), F (fading),
  E (exhaustion), I (initiation), T (trapped), plus research letters AI (A then counter-I, char J) and T1 (T confirmed by the next
  candle, char U).
- Each letter gets 15 decision-time context bits (key level, VWAP, trend, pivot, breakout with / against, momentum, own history,
  first test, old swing, at the day's extreme, first hour).
- DeltaCtxParams.h (written by lra.dp_intel) holds a per-market rule per letter: SHOW, "?" or LOG. A letter is drawn only when its
  market's status is SHOW / "?" and its context holds.
- Drawn: the letter in FRONT of the ratio label at the node ("T 2.4x", "I? 3.1x"), green bullish / red bearish, only letters from
  the last 90 min; no hovers, no circles. No room = the ratio alone.
- No repaint: a letter is decided once at its candle's close and journaled (DI line + a DIW watermark per candle); a restart reloads
  the journal and never records twice.
- Status file lines INTELLIGENT (drawn / no-room counts) and INTELLIGENT_GATE (the active rules). Title "Intelligent DLT v2.33.0".

## A rule (default, Rassul 2026-11-07)
Own candle only; bullish = green candle, node at / below the open, below half the range, in the lower wick; 2.0x top-3 node; pivot on
the 3-bar ZigZag; bearish mirror. Old rules behind compile flags: DINTEL_A_MODE, DINTEL_A_OPEN_HALFWAY, DINTEL_A_PIVOT_ZZ3.

## Truth used for the gate
A, T, AI, T1: 9-min hold (3 candles, a 3-min close beyond = wrong). I, E, F: 30 min (10 candles).

## Shown today (accuracy.json 2026-11-07)
GC: I (bo_against+open_hr) SHOW, T (swing_old) SHOW. EU: T (at_ext) SHOW, T1 "?". ES: I (at_pivot) "?". NQ: I (mom_hi+swing_old)
"?", T1 "?". CL: T1 "?". HG: T "?". NG: nothing. All A, F, E: LOG.

## Nightly
self_improve part dp_intel:nightly re-runs the study after the close (skips Mon-Fri 08:30-15:00 CT), demotes a shown letter whose last
20 days fall under 70%, and rewrites DeltaCtxParams.h only when the rows change (then the next build picks it up).

## Tests
tests_dintel/t_dintel.cpp (fixtures tests_dintel/fx, 7 real sessions): parity vs Python 105/105 letters, no-repaint replays, gate,
record line - 306 passed. Full sweep earlier: 2899/2899 letters identical (9150 checks). lra: test_dp_intel.py 4/4.
