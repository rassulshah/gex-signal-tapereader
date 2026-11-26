# lsHodLod 2.54.0 - Last 5 days panel (2026-11-26)

Requests (CT): 14:59 "a side panel that shows the last 5 days and the hod and lod levels that were swept"; 15:01 vertical, left
justified, no black space on the left; 15:02 at most two levels; 15:05 narrower; 15:06 day above the date; 15:07 "vertically center
it", "center the level and hod lod box"; 15:08 more space around the day separators; 15:11 include weekly levels, VAH VAL POC (only in
this panel); 15:18 order: session, daily, volume, weekly, gamma; 15:16 "build". Mockup: LastFive-Vertical-mockup.html.

## What changed
- HodLodLast5.h 1.0.0 (new, SDK-free): L5 row parse, day / date / time, lsEventCal height from its status file, the panel layout.
- HodLodSweeps.h 1.4.0: reads the L5 rows (bad rows skipped, at most 5).
- HodLod.cpp 2.54.0: the panel flush with the pane's left edge, centred vertically between the Event Calendar (EventCal.status-<MKT>-<spb>.txt
  PLACE line, fresh = under 15 min old; none = under the title row) and the Level line / box; the HOD / LOD box and the Level line centred
  on the candles again. Status file line LAST5.
- hodlod_levels 1.15.0 (nightly): weekly VAH / VAL (prior-week profile) + L5 rows in LRA-HodLodSweeps-<MKT>.csv.

## Tests
t_hl2540 30 pass (real ES file, bad rows, day of the week, calendar status, centring, short band, no room, box centred); t_hl2510 81,
t_hl2500 52, t_hl2490 35, t_hl2480 51 pass; MinGW syntax check with the real irtsdk.h: 0 errors. Nightly tests 23 pass (1 environmental).

## Not verified - needs his IRT
The panel on his charts (ChartView export after the restart), the calendar height read on each chart.
