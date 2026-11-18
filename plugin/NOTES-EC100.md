# lsEventCal 1.0.0 - Event Calendar box (2026-11-17)

Requests: Rassul 18:09 CT "the calendar at the top center of important events that are coming up for the market that im trading ...
for crude it could include inventories. other events are like fomc etc ... after 30 min of the even it can be removed"; 18:22 "i only
want market moving events medium and high impact that can move the market. also since i have limited space, be carefull with the font
size and spacing"; 18:25 "build" (mockup event_calendar_box.html: no title row, one line per event).

What it does: on the price pane, the next 3 medium / high impact events of the chart's market within 2 days - "Wed 1:00 PM  FOMC Minutes
18h 35m". High impact bold, medium plain; time grey while later; amber inside the last hour; red "out 11m" for 30 min after the release,
then removed. Top centre (9-spot 2) inside the area up to the last bar and below the IRT title row; when lsHodLod's table is on the chart
(BoxDock.h LRA_HLB_) it docks beside it (right, else left / below) and never overlaps it. Helvetica 9. No settings, no hovers.

Input: lsFlexLevels\LRA-Calendar-ALL.csv from the nightly lra.event_calendar (read at most every 10 s, re-parsed on a stamp change,
torn-safe, 256 KiB cap; a garbled file never replaces a good copy). Missing / garbled / older than 8 days = the built-in list
(EventCalLogic.h: the dated releases known 2026-11-17 to Mar 2027 + weekly claims / EIA / API rules, holidays skipped).

Nightly (lra.event_calendar 1.0.0, part "EVENT CALENDAR"): fills 21 days from analytics/lra/data/calendar_2026_2027.csv (official
schedules with sources), events_2025_2026.csv and the weekly rules; scores each event type's real impact per market (15 min range after the
release vs the same 15 min on 20 sessions before) and demotes / drops / promotes on 20+ releases with both halves agreeing. Data gaps
listed in research-artifacts/setups/EVENT-CALENDAR.md.

Tests: tests_eventcal/run_tests.sh - logic 75 pass, host (mock SDK: two charts on one DLL, QCL -> CL, HOD / LOD dock no overlap, narrow
chart, garbled / missing file, removal) 30 pass, ASan/UBSan -Werror; MinGW against the real irtsdk.h 0 errors / 0 warnings outside the SDK.
analytics/test_event_calendar.py 5 pass.

Risks / not verified: needs his IRT (font widths, the dock next to the HOD / LOD table and the Big Lot box if lsBigBidAsk is also docked
there - the Big Lot box does not publish its rectangle). Not known yet: OPEC+ meeting time (set 6:00 AM CT), Lagarde speeches (no schedule
source), Jan-2027 CPI / NFP / PCE dates (the nightly flags them as gaps until added).
