# lsHodLod 2.55.0 - box re-arranged, Stops resting / Stops run, "Next" (2026-11-26)

Requests (CT): 15:51 "left justify the box again" (2.54.1); 15:53 a section right of the box with the levels already swept; 16:00 "my
intention is to identify highs and lows that have had their stops run"; 16:02 resting on top; 16:05 "make the 1st and 2nd vertical,
extreme in by will be over the hod lod swept section"; 16:07 "use 9:00 61% format ... to make the box shorter"; 16:08 "update Vob Watch to
Next and center it"; 16:09 "stops run should be below the divider"; 16:11 no held / back; 16:12 space for more levels; 16:13 DPoc / VA
after the price levels; 16:14 "build"; 16:18 PDL taken in London must show in Stops run. Mockup: StopsBusy-mockup.html.

## What changed
- HodLodLogic.h hsb::layout: left = 1st (Exp / Act), a line, 2nd (gap) (Exp / Act); a divider; right = "extreme in by" rows of "9:00 61%"
  cells on the same columns as the HOD / LOD swept table under them; swept columns PRE-HR, ON, PD, PF, then DPoc, VA.
- HodLodStops.h 1.0.0 (new): session / daily / weekly highs and lows from LRA-KeyLevels; run = 1 tick through after the level formed (Asia
  after 1:00, London after 7:30 or the open, PhrH / ON from the open, prior-day / weekly from 5:00 PM); resting = 2 nearest not run each
  side; 2 names a line, grows upward, never cut (not drawn when there is no room).
- NowLevels.h 1.8.0: "VOB(T) WATCH" reads "Next"; the host centres the Level line on the box.
- Status file line STOPS.

## Tests
t_hl2550 29 pass (real ES 11/26 keys + bars at 8:00 AM and 11:54 AM; windows; London through PDL; a touch is not a run; layout);
t_hl2540 30, t_hl2510 83, t_hl2500 54, t_hl2490 37, t_hl2480 53 pass; t_hl2470 / t_hl2440 only their old version-number checks fail
(as before); MinGW syntax check with the real irtsdk.h: 0 errors.

## Not verified - needs his IRT
The section on his charts (width before the last bar in each market), the arrows' font.
