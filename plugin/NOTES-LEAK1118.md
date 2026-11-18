# Memory / freeze hunt, 2026-11-18

**Request (Rassul):** 09:53 CT "also consider the memory leaks because irt has been freezing alot", 09:54 "maybe an independent check to identify and fix", 10:14 "start now".

## What the logs show

Source: `Perf\perf-*.csv`, 11-03 .. 11-18.

- IRT private bytes grow only while the charts are being drawn.
  - In minutes with draws: 2-9 MB/min in early November, then 17 (11-11), 25 (11-12), 40 (11-16) and 46 MB/min (11-17).
  - In minutes with no draws (screen off overnight): about 0.
- Every restart resets the memory. Yesterday it reached 9.3 GB by 16:00.
- The growth is not explained by our status counters. At 10:14 the BookFlow book held 6,910 ES orders and the queue peak was 14k.
- So the source is still open: one of our draw/calc paths, or IRT itself.

## Independent audit

Full report: `scratchpad/leak-audit/INDEPENDENT-AUDIT.md`. High findings:

- **H1:** The order-feed queue routed up to 400k events in one go. Hidden tab = freeze.
- **H2:** The BookLevelBox book had no cap and no reset on reconnect.
- **H3:** Stale-order books (off::Engine 200k cap, the zone read 60k cap that refuses new orders).
- **H4/H5:** WavePOC sampler, duplicate feeds. The WavePOC book is not fed in 1.16.1, so it is not a live issue.

## What changed

- **PerfGuard 1.1.0:** every calc / draw measures private bytes before and after itself, so growth is pinned to a plugin and chart.
  - Output: `Perf\permem-<plugin>-<day>.csv` (calc_net_mb, draw_net_mb, calc_up_mb, draw_up_mb).
  - Scopes added to the plugins that had none: Breakout / Breakout1, CobSignal, WavePOC, StepTrend, ZzoCombo / Marks, VolRead / RangeRead, HtfSR, EventCal.
  - gex-build.next.bat lists PerfGuard.h on their lines.
- **OrderFlowFeed 1.17.0:** the queue is routed in slices (40k events or 25 ms per calc/draw, the rest kept in order). The direct read waits until the queue is empty.
- **BookLevelBox 1.2.0:** BOOK_CAP 120k. Far orders (more than 400 ticks) leave first, then the oldest; their depth is taken out. BookFlow 1.41.0 cold-starts the level box after a feed reconnect.
- **ZoneRead 1.2.0:** orders outside every level band leave when the book is half full after a level change. Orders filled to 0 leave before a new order is refused at the cap.

## Tests

- `tests_perf/test_permem.cpp` 5/5. PerfGuard suite 58/0, macro sim, e2e, real-SDK syntax.
- `tests_leak1118/test_leak1118.cpp` 15/15 (ASan/UBSan): slices in order, depth = book after a trim, far first, prune.
- Existing host suites: identical failures with the old and new headers (version pins only). blb logic 40/1 (version pin).
- MinGW syntax against the real irtsdk.h: 0 errors on every installed plugin touched. DeltaProfile was not checked because the workspace copy is out of date.

## Not verified (needs his IRT)

- Permem after his restart: which plugin's draws add the memory.
- Then fix that plugin. If none of ours shows growth, the growth is inside IRT.
