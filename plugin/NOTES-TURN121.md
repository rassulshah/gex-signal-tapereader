# lsTurn 1.2.1 - the Stop Run fault: one old order stopped the order-ID clock (2026-11-23)

## Requests (Rassul, 2026-11-23 CT)
- Reported to him 7:51 AM (NQ counts 42% of its fills as stops), 10:29 AM and 11:02 AM (after the 9:38 AM restart NQ is normal, CL counts 54%, then 81%); the fix offered each time (45 - 60 min, $0).
- 11:47 "go.   can i reopen irt now"

## What was wrong (found in IRT's own feed, not guessed)
- The pane ages an aggressor by its order ID: IDs count up, so "this ID was first reached 60+ s ago and the order was never seen resting" = a stop. For that it keeps a clock: newest ID seen -> second.
- **IRT's order IDs are an 8-digit counter that starts again at 0.** CL: 89.96 M at 10:26 AM on Friday, 99.99 M at 2:27 PM, 27.5 M on Monday 9:00 AM (about 2.5 - 3 M an hour, shared by more than the one contract). So an order of the lap before can carry a HIGHER number than every new order.
- Such old orders show up among the new ones 15 - 35 times a session a market: a stop-limit that was triggered and now rests (a NEW order with its old ID), an order announced again, a triggered stop as an aggressor.
- 1.0.0 - 1.2.0 took ANY ID up to 16.7 M above the newest as the newest (only a bigger jump needed 8 in a row).
- **CL, 2026-11-23 9:49:52 AM: `NEW|80.29|1|44965432|S`** while new orders were at 30.4 M. The clock jumped to 44.97 M and stood still (the counter needs about 5 hours to get there). From then on every aggressor looked "first reached at 9:49:52", i.e. 60+ s old a minute later, and every one that had not rested was counted as a stop.
- Reproduced from lsBookFlow's recording of that feed (data\irt\2026-11-23\CL\mbo_09.csv, mbo_10.csv; tests_turn/idreplay.cpp): the 1.2.0 logic stops the clock at **431 points** - the number in his live status file all morning - and counts 15,192 of 18,925 fills as stops (80%; live 81%).
- NQ before the 9:20 AM restart (42%, clock standing at 3,097): the same picture; no NQ recording exists for today, so that one is not proven.
- Not affected: lsStopRun / lsSR (St / Sta / Sr go by bursts, not by ID age); the nightly (it reads the exchange's own IDs from the order store).
- What it spoiled: on the caught market the recorded Stop Run was far too high and the Answer's FAST mark (a counter order issued under 2 s before) never fired, from the moment of the old order until the next restart. Sweeps recorded live in that time carry those values (CL today 9:50 AM - the restart; NQ 6:29 - 9:20 AM): the nightly's own read of the same sweeps is right.

## The fix (TurnLogic.h IdClock) - the clock follows the RUNNING count only
- **near**: an ID above the newest by no more than 20,000 + 5,000 x (seconds since the newest moved) = the count went on; it moves the clock. Measured on IRT's feed (CL 11-20, CL 11-23, NQ 11-16; 3.1 M new-order IDs): a real step was never over 5,000 + 5,000 a second (largest: 11,195 after 3 s; 1.2 M after a 25-minute gap).
- **far above**: a candidate. It becomes the clock only after 32 rising IDs of its own lane with no ID of the running count (above it, or under 20,000 below it) in between - a real jump (the pane started on an old order of the book). One running ID drops it. The same old ID again (one stop order filling 20 resting orders) is no vote.
- **far below** (a NEW order more than 20,000 under the newest): nothing by itself, but 64 in a row with no running ID between them = the newest is not the count (the counter started again at 0; or the clock was caught after all). The clock is set back to where most of them are (the newest within 20,000 of their median); points above are dropped, the older ones kept.
- An old aggressor below the newest never counts for or against the count: that is what a stop looks like.
- After a lane change or a set-back the clock is "known since" that second: a candle that began less than 60 s later gets no Stop Run (not known, never a wrong number).
- Status file, LIVE line: `|id strays N|id lanes N|id set-backs N` (old orders left out / real jumps taken over / set-backs).
- NOT changed: everything drawn (the pane is just Effort), the display file, every other reading, the records' format, the params, the nightly.

## Files
Turn.cpp, TurnLogic.h (1.2.1 = TURN_VERSION = turn::VERSION = the one setVersion literal). tests_turn: test_turn.cpp, test_turn_host.cpp, soak.cpp (changed); cl_20261123_0947.txt (the real CL feed 9:47:00 - 9:52:30 AM, 51,315 events, the mock's clock), idreplay.cpp (IRT's recorded feed through turn::Live), all121.sh (new). gex-build.bat is NOT edited.

## Tests (2026-11-23, research machine)
- Real irtsdk.h syntax check, C++14 and C++17: 0 errors, 0 warnings outside the SDK. Mock SDK compile C++14 -Werror: ok.
- Logic (ASan / UBSan, -Werror, C++17; C++14 -O2): 3,285,096 checks, 0 failed (2,539,468 in 1.2.0). New: his case in small (the count at 30.4 M, one NEW at 44,965,432: not taken, the clock ticks on, ages real); one old stop's ID 20 times in a row; a handful of old neighbours; a candidate ended by an ID just below the newest; caught by 40 old IDs in a row and set back by the running count; an old order inside the set-back streak; the counter starting again at 0; 500 old aggressors (a stop cascade) and old NEW orders between running ones: no set-back; the 25-minute gap of this morning taken at once; a pane that starts on an old low / an old high order; 2,000 random streams with old orders above and below, alone and in clusters: the clock's newest ID is never off the running count; 2,000 panes that start on a whole announced book: on the count within 200 live orders.
- **The real feed** (cl_20261123_0947.txt through turn::Live): from 9:51 AM on 986 fills, 24 counted as stops (2%; 1.2.0: over 90%); the clock goes from 240 to 329 points in the last 90 s (1.2.0: stands); 3 old orders left out, no set-back.
- IRT's recordings through the old and the new logic (idreplay.cpp): CL 11-23 9:42 - 11:00 AM 15,192 -> 163 stop fills of 18,925 (80% -> 0.9%); CL 11-20 (4 h) 438 = 438 of 48,343; NQ 11-16 (overnight) 1,571 = 1,571 of 61,728. Where the old clock was not caught, nothing changes.
- Golden candles through the C++ (the exchange's own IDs): 110 sweeps + 45 mockup candles, 2,017 checks, 0 mismatches.
- Whole plugin on the mock SDK (ASan / UBSan / LSan): 8,520 checks, 0 failed. Friday's ES morning (44 candles) through 1.2.0 and 1.2.1, the default and the full setting: the draws, the draws under the mouse and the record file are byte-identical (`id strays 0|id lanes 0|id set-backs 0`).
- Soak in both display modes: 40 cycles plain, growth 0 KB, 680 checks, 0 failed; 8 cycles ASan / LSan, 136 checks, 0 failed, no leak.

## Not verified (needs his IRT)
- Live, after the restart: `status-<MKT>-180.txt` LIVE line - `stop fills` about 1 - 3% of `fills` on every market, `clock` climbing to 4096, `id strays` rising slowly, `id lanes` / `id set-backs` 0 or a few. To be read again in the afternoon, when the CL counter has not yet passed 44.97 M (the old order of this morning is still "ahead").
- A quiet market on a busy feed (HG, EU, NG): if the count there steps more than 5,000 a second between two of its own orders, the clock follows by lane (32 orders later) instead of at once - `id lanes` would show it. Not seen in the three recordings.
- NQ's fault of this morning (no recording to replay).
