# Zone Defense 1.0.2 - progress checkpoint (2026-11-08)

Work copy: the cloud scratchpad (zdb/). The plugin folder is updated when 1.0.2 compiles.

| Item | Status | Note |
|---|---|---|
| Review fixes H1-H4 | DONE | Markers in ZoneDefense.cpp / ZoneDefenseLogic.h |
| Review fixes M1-M4, M6-M8 | DONE | Markers in code |
| Review fix M5 | TODO (verify) | No marker found |
| Review fixes L1, L2, L3, L5 | DONE | Markers in ZoneDefense.cpp |
| Review fixes L4, L6 | TODO (verify) | No marker found |
| Bar-stamp resolution | TODO | IRT bars are start-stamped (verified); engine side still to align |
| Live-style timing + jumpstart re-run | DONE | Whole-second clock, refillSameSecond, ghostTicks; 71 days x 7 markets |
| Zone states (10-bar window) | DONE | forming/testing/held/reclaimed/broken/broken->reclaimed in the engine |
| Sweep bug (GC Oct 26) | DONE | Close-based back-in on the sweep bar |
| Queue normal 0% in GC/HG | DONE | Same-distance normal (nearWindows) |
| Level tags | IN PROGRESS | Engine + jumpstart done; Read label and host level loading still to wire |
| Recording/backfill watchdog row | DONE | watchdog.py committed |
| All-7-market parity | IN PROGRESS | Last run: NQ mismatch, 2 lines with a sub-second arrival offset (ZE 1 s apart); re-run after 1.0.2 |
| Rithmic vs Databento day | TODO | |
| NQ memory / time | TODO | Last perf: 0.49 us/event, max book 2876 orders, 605k ended orders kept (trim needed) |
| Scorecard test fix | TODO | |
| OneDrive online-only step | TODO | |
| GC Nov 6 example mockup | WAITING | Needs Nov 6 MBO |

## Level-tag jumpstart (71 days, 7 markets)
- Strict holds (held 9 bars): ES 0.31, NQ 0.27, CL 0.38, GC 0.32, HG 0.42, NG 0.57, EU 0.55.
- About half of breaks are reclaimed within 10 bars (ES 46%, NQ 50%, CL 50%, GC 50%, HG 48%, NG 54%).
- Being near a level adds no hold edge (ES/NQ/CL/NG negative at all reaches; GC +0.015 at 1.5 ATR; HG ~0).
- So the tag stays informational; reach is a per-market parameter (default 1.0 ATR) for the nightly to tune.
