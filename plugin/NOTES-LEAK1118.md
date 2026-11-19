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

## After the close (15:10-15:20 CT): what the permem logs show

Source: `Perf\permem-*-20261118.csv`, 13:19-15:10 (IRT restarted about 14:00).

- In the steady stretch 14:40-15:10, IRT private bytes grew 1,887 MB (63 MB a minute). The plugins' own calc / draw calls account for 1,913 MB of it, so practically all the growth happens inside OUR calls, mostly inside draws.
- Per plugin, 14:40-15:10 (MB; per draw):
  - DeltaProfile 661 MB (68 KB a draw, all in draw, none in calc)
  - BookFlow 432 MB (61 KB a draw)
  - DealerProfile (Limit Profile) 330 MB (34 KB a draw)
  - ZzoCombo 279 MB (124 KB a draw)
  - WavePOC 74, Breakout1 64, ChartView 34, RevRead 22; SDMO, VolRead, RangeRead, HodLod, IRTReader about 0.
- The four big ones are the heavy drawers (profile columns, panes full of bars). They share no header except PerfGuard / SettingsGuard / irtsdk.h, and the light plugins with the same PerfGuard grow about 0, so PerfGuard is not the cause.
- Still open: is it memory the plugin's own code keeps (a container that grows), or IRT's own memory used while it carries out the plugin's drawing (drawing primitives / text / VAP arrays)? Private bytes cannot tell the two apart.

## PerfGuard 1.2.0: own heap

- Every plugin DLL now counts the bytes its own code holds: its operator new / delete (malloc / free underneath, exactly like the default, no header, so a block freed by IRT or another module still frees correctly).
- Every calc / draw also records how much the DLL's own heap grew. Output: `Perf\permem2-<plugin>-<day>.csv` with own_calc_mb, own_draw_mb, own_live_mb added.
- Read tomorrow: own_live_mb rising with private bytes = that plugin keeps the memory (fix it in the plugin). own_live_mb flat while its draws add private bytes = IRT keeps it while drawing for that plugin (then: draw less per pass, e.g. cache the drawn picture or cut primitives).
- Tests: test_permem 7/7 (own heap columns), PerfGuard suite 58/0, macro simulation, e2e writer, real-SDK syntax; MinGW syntax of DeltaProfile, BookFlow, ZzoCombo, SDMO, RevRead with 1.2.0: 0 errors; the operator block links (MinGW) and counts / releases correctly (Linux run).
- SDMO.cpp: the unused sdmLetters function removed (MSVC C5051 warning); SDMO host tests 14/14.

## 2026-11-19 10:00 CT - PerfGuard 1.2.0 own-heap counter: the growth is DeltaProfile's OWN heap
Source: `Perf\permem2-*-20261118.csv` (19:37-22:41) and `-20261119.csv` (07:24-09:53); own_live_mb = bytes the DLL holds through its own
operator new (all its charts), priv_mb = IRT's private memory.
- **DeltaProfile: own_live tracks IRT's private memory almost 1:1** (priv - own_live stays about 0.85-1.8 GB):
  - 11-18 evening: 5 MB (19:37) -> 4,124 MB (20:57), dropped at the 21:5x restart, then 30 MB -> 2,958 MB by 22:41 (priv 4.2 GB).
  - 11-19: 5 MB (07:24) -> 641 MB (08:27) -> 2,252 MB (08:54) -> 3,526 MB (09:30, priv 5.3 GB). About 65 MB a minute in RTH.
    After the 09:38 restart: 3 MB -> 170 MB by 09:53.
  - Rows at 09:12 / 09:21 show draw_n 0 (IRT not drawing) with priv at 4.6 GB; the 09:41 restart hang came after priv reached 5.3 GB.
    Memory pressure from this leak is a likely contributor to the hang (not proven).
- **Not their own heap** (own_live flat, so the process growth logged against their draws is IRT's, or another DLL's growth
  landing during their draw): DealerProfile / LimitProfile own_live 16-27 MB (process draw net 729-1,388 MB); ZzoCombo 9-23 MB
  (736-1,008 MB); BookFlow 7-39 MB; Breakout1 9-23 MB.
- Next: find what DeltaProfile keeps allocating in draw (own_draw adds 30-140 MB per 9-minute slice and is never freed) - a
  per-draw cache / vector that is appended to but never cleared. Fix needs a DeltaProfile build (his yes; trading hours rule).
