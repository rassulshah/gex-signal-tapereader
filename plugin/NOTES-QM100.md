# QUEUE METER - lsDealerProfile 2.20.0 + lsLiquidityProfile 0.49.0 (QueueMeter.h 1.0.0), 2026-10-22

## Request
- 13:51-13:52 CT: Rassul approved the queue meter, option A (mockup `docs\build-queue\mockups\queue-placement.png`, panel A).
- 15:20 CT: build ordered.
- Roadmap question: "Is the queue at the level running out?" (ticket T-20261022-16).

## Where it lives and why
- **The meter is drawn by lsDealerProfile.** That plugin draws the Levels column, but it has no order feed.
- **The numbers come from lsLiquidityProfile.** It already keeps the chart's MBO book (one DEPTH_BY_ORDER per chart).
  - It runs the engine `QueueMeter.h`.
  - About once a second while a test is live, it writes `lsFlexLevels\LRA-QueueMeter-<MKT>.csv`.
  - This is the same pattern as LRA-LiqLevels: the Liquidity Profile writes, the Dealer Profile reads.
- **Why this split.**
  - A second MBO subscription on the same chart was not needed.
  - The Levels column stays where it is.

## What it does
- **Test start.** A test begins when price trades into one of these zones from the price side:
  - a level zone (key levels + MenthorQ levels of this contract): anchor - 0.12 ATR .. anchor + 2 ticks for bids, mirrored for offers;
  - a recent pivot zone (1 candle left / 2 right; dropped once a candle closes through it);
  - a suddenly defended price: a new 2-min low or high into a zone holding 2.5x+ the typical lots per tick within 1 ATR.
- **Meter.** The queue left vs the queue at the start.
- **Originals.**
  - These are the orders resting within 2 ticks of the defended price at the start that had rested 1 s or more.
  - Younger orders are re-price flicker and do not count.
  - Each original counts up to its starting size while it rests in the zone.
- **Rates.** Over the last 10 s:
  - eaten (fills);
  - pulled (cancels of orders that rested 1 s+, counted only while price is in contact, so the clean-up after price leaves is not counted);
  - refill that stayed (adds still resting after 1 s+).
- **Time to empty** = queue / (eaten + pulled - refill that stayed). "Holding" when refills keep up.
- **Break risk HIGH** when any of these is true:
  - the zone broke;
  - it will empty within 10 s;
  - under 40% of the queue is left.
- **Normal.** The nightly `LRA-QueueNorms-<MKT>.csv` gives eaten lots/s per 30-min CT slot and side. Without it, this session's own (lots per active second).
- **Decided Q at a candle close.** It is recorded once and never changed: `<stem>-queuemeter-<session>.csv`, read back after a restart.
  - Red Q: broke (a trade 2+ ticks past the zone before the close).
  - Green Q: held (price left the zone 0.25 ATR on the defenders' side before the close).
  - After 3 closes in contact: green ("refilled") at 50%+ left with the originals not all gone, else red ("drained").
- **Q letter: record only.**
  - No Defense-pane strip exists yet to host it.
  - The strips build (another agent) can draw it from the record.
- **Dealer Profile display.**
  - The level badge nearest a live test's anchor becomes the meter. It spans the Levels column; the fill is green for bids and red for offers.
  - Text "G1 Q76%".
  - Originals gone: 2 px yellow outline and yellow text "Q33% . orig 0%". The approved middle dot is drawn as " . " because IRT text is ASCII.
  - A test with no badge in its zone gets a small unlabeled badge at its price.
  - It is shown only within the reach of price (setting, default 1 ATR).
  - The text shrinks to 7 pt, then to shorter text. It never leaves the Levels column.
  - The hover has the numbers, then question / what it is / why it matters / how to use / example, with "?" (not proven yet) in the header.
  - A missing, stale (> 5 s), garbled or wrong-market file means normal badges, with the reason on the status line.
- **Settings (Dealer Profile, parameter version 9).** All appended rows:
  - Queue meter (checkbox);
  - Reach (ATR) 0.25-2.0;
  - Meter bids, Meter offers and Originals gone colours.

## Tests
- `tests_qm/test_qm.cpp` (logic): 75 passed.
- `tests_qm/replay_qm.cpp`: the real NQ 2026-10-09 full-depth MBO (`tests_qm/conv_qm.py`), 16 passed:

  | Time | Queue | Orig | Rates | Read |
  |---|---|---|---|---|
  | 8:55:39 G1 | 74% (76% at 8:55:15) | - | - | holding, risk LOW; green Q "held" at the 8:57 close |
  | 9:42:26 (9:24 pivot zone) | 33% | 0% | pulled 10.9 / eaten 4.9 lots/s | 1.7 s to empty, risk HIGH; 0% at 9:42:28 (the break); red Q |

  - A restart mid-test (9:42:10) and at 9:50: every Q written before is unchanged, none decided twice.
- `tests_qm/test_qm_lp_host.cpp` (LP mock host: feed, live file, record, status, restart): 12 passed.
- `test_dealerprofile_2200.cpp` (DP mock host, real draw code): 25 passed.
  - Covered: badge text, fill %, colours, yellow state, unlabeled badge, reach, stale / wrong market / torn, settings, narrow column, nothing outside the Levels column, hover, status.
- Updated suites:
  - test_dealerprofile_2110 (56) and 2190 (25): float / colour parameter mocks, versions compared instead of typed;
  - LP suites (tests_lmz, tests_lqp044).
- MinGW against the real irtsdk.h with -Dsmall=char: DealerProfile, LiquidityProfile and LiquidityFlow have 0 errors and no new warnings.

## Findings
- **The mockup's "orig 22%" at the G1 hold was an artifact.**
  - The research rebuild kept orders after their fill rows; for example, NQ order 6883242054741 was filled 08:44:38 and still counted.
  - On the corrected book the G1 originals were gone by 8:55:15 too: the hold came from NEW bids.
  - So the yellow state alone does not separate a hold from a break. Queue left + time to empty did (holding vs 1.7 s).
  - The nightly pivot-reading iterations should test this (T-20261022-16).

## Not verified - needs his IRT
- The live layout on his charts: screenshots / ChartView after the restart.
- The arrival-time clock of the feed.
- Per-market behaviour outside NQ.

## 1.1.0: the validation verdict (2026-10-22 evening)
lsDealerProfile 2.21.0 + lsLiquidityProfile 0.50.0, QueueMeter.h 1.1.0.

The verdict is in `research-artifacts\learning\VALIDATE-DEFENDER-SIZE-2026-10-22.md`.

- **The 10-s CALL.** It is decided once, 10 s after price first trades into the zone. A test that has already broken or left gets no call.
  - RED: queue left < 40%.
  - GREEN: 100% or more left, and holding. ES and CL only.
  - Each call is written as an append-only `C|t0|t_call|side|anchor|R/G/N|pct|orig|tte|kind|ver` line in the queue record.
- **Per-market rule.** It comes from `lsFlexLevels\LRA-QueueMeter-<MKT>.csv` (PARAM lines, strict; garbled or wrong market = defaults). The defaults equal the verdict:
  - ES / CL: SHOW, red + green.
  - NQ / GC: SHOW?, red only.
  - HG / NG / 6E: LOG.
- **File names.** The live file is now `LRA-QueueLive-<MKT>.csv`, because that params name was taken. Old 22-field rows are still read, with no call.
- **Dealer Profile.**
  - The badge outline follows the call: 2 px red / green.
  - With no call: grey 1 px, or yellow 2 px when the originals are gone.
  - "?" after the text in NQ / GC.
  - The hover gives the market's out-of-sample odds.
- **The candle-close Q** stays a record for the nightly (no edge after it is decided, 47-54%). It is also written to `Strips\QUEUE-<MKT>-<spb>.csv` for the Defense Holding strip (lsBookFlow 1.23.0).
- **Tests.**
  - test_qm: 92.
  - replay_qm: 17. NQ 10-09: red 24, green 0, none 184. The two mockup tests read 65% at 10 s, so neither got a call.
  - LP host: 16.
  - DP host 2200: 28.
  - MinGW real SDK: clean.
  - Settings contract: PASS.
