# lsHodLod 2.37.2 - signal tags expire after 60 minutes

## Request
Rassul, 2026-11-19 22:53 CT. He sent a screenshot of NQ at 10:51 PM. The signal line in the HOD / LOD box still showed "NQ Sw PFL 5:39". He wrote: "remove signals after an hour". This is an approved small fix on top of 2.37.1, which was built and still waiting to install.

## What changed
- The signal line is the first line inside the HOD / LOD box: Cob on the left, Sw in the centre, BkTst on the right, with the latest of each per market. A tag now shows only for 60 minutes after its candle. At 59 minutes it still shows. At 60 minutes or more it is gone.
- Age is measured on the chart's newest bar time. That is the end of the last closed bar, which is also the start of the forming bar. This is the market's own clock, so an idle feed or a reload neither keeps tags nor drops them wrongly. The PC clock is used only when no bar time is known yet.
- Tags are aged on every draw. Before, they were filtered only when the record files were re-read every 5 seconds.
- When a tag expires it disappears. If a group has no tags left, its third of the line is empty. If all three groups are empty, the line stays in place, empty, so the box height does not jump. That rule was already in place in 2.37.0 and is now tested again.
- Status line: `TOP_SIGNALS,1.1.0,...,badges N,expired N,clock bar|pc`.
- TopSignals.h 1.1.0 adds `SHOW_SECS = 3600`, `ageClock`, `fresh` and `freshOnly`.
- Versions: HL_VERSION = setVersion = PLUGIN_VERSION = 2.37.2, plus the header comment.
- Nothing else changed.

## Tests (Linux, g++ ASan/UBSan, -Werror)
- New t_hl2372, 41/41 passing:
  - Tags show at 0, 1, 30 and 59 minutes, and at 59:59.
  - Tags are gone at 60 and 61 minutes, at 5 hours and at 1 day.
  - The bar clock wins over the PC clock, and the PC clock is the fallback.
  - Mixed groups: a Cob at 30 minutes and a BkTst at 10 minutes stay, and two Sw tags at 61 and 70 minutes go. The Sw third is empty.
  - A newer Sw replaces an older one.
  - Reload of his 5:39 PM NQ case at 10:51 PM: none of the old Cob, Sw or BkTst tags show, on either clock.
  - On that same record the 5:39 PM Sw shows at 6:38 PM and is gone at 6:39 PM.
  - Idle feed: the tag stays while the bar clock is frozen.
  - The box has one height with 0, 1 or 3 tags.
  - Host source checks and versions pass.
- t_hl2371 46/0, t_hl2370 5884/0 and t_hl2360 85/0 pass. Only their version strings were updated.
- t_hl2350 has 48 failures and t_hl240 has 1 ("header versions"). Both were already failing on 2.37.1 before this change (the 2.35.0 right-aligned layout and an old header check). They are unchanged.
- The MinGW syntax check against the real irtsdk.h gives 0 errors. The only warning is the old one in first_extreme_inference.cpp.

## Risks / not verified
- Not yet verified on his IRT. After the restart, check the status line `expired` / `clock bar` and look at a chart with an old tag.
- On a 3-minute chart a tag can stay up to about one bar longer in real time, because the clock is the last closed bar's end.
