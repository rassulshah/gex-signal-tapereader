# lsHodLod 2.37.2 - signal tags expire after 60 minutes; Now line TESTING instead of a live BREAK / SWEEP

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
- Nothing else changed in the signal line. The Now line change is in the Addition below.

## Tests (Linux, g++ ASan/UBSan, -Werror)
- New t_hl2372, 44/44 passing:
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
- t_hl2371 46/0, t_hl2370 5884/0 and t_hl2360 102/0 pass. Their version strings were updated, t_hl2360 [4b] was rewritten for the Now line rule, and the t_hl2371 NowLevels version check is now 1.2.0.
- t_hl2350 has 48 failures and t_hl240 has 1 ("header versions"). Both were already failing on 2.37.1 before this change (the 2.35.0 right-aligned layout and an old header check). They are unchanged.
- The MinGW syntax check against the real irtsdk.h gives 0 errors. The only warning is the old one in first_extreme_inference.cpp.

## Addition (23:02 CT) - the Now line no longer calls BREAK / SWEEP live (NowLevels.h 1.2.0)
Rassul sent an NQ screenshot. The Now line showed "SWEEP: PHL ↑" while the forming candle was only trading at or through PHL. He wrote: "why does it say sweep when there is no sweep yet, its interacting with the level so its just testing it until there is a break or a sweep".
- While the FORMING candle trades at or through a watched level (the nearest named level above or below the last close), the line shows the gold badge `TESTING: <level> Bk / Sw`. It has no arrow, because the side is not known yet. It is never BREAK or SWEEP while the candle is forming.
- BREAK and SWEEP are decided only at the candle's close, and only from the chart's own marks on that closed candle (the wy file, now including Sw lines):
  - BREAK needs a Bk mark that passes the BO candle rule: range >= the prior candle's, and range or volume up.
  - SWEEP needs a Sw mark with strength 10 or more (V / R), in the right candle colour.
  - They show red or green by side until the next candle closes. If the candle closes without a mark, the line goes back to WATCH.
- Order of priority: TESTING <level> BkTst (unchanged) > BREAK / SWEEP (closed) > TESTING <level> Bk / Sw (live) > WATCH BkTst > WATCH Bk / Sw.
- The NOW_LEVELS status line now reads "closed Bk / closed Sw / testing Bk / Sw <level>", or the reason a Sw mark was not used.
- Tests are in t_hl2360 [4b], 102 / 0, rewritten from the 2.36 live BREAK / SWEEP expectations. They use his real ES 11-19 bars and marks:
  - The 9:42 PDL candle forming shows gold TESTING.
  - At its close with a V99+ Sw mark it shows `SWEEP: PDL ↑`, and that is gone at 9:45.
  - The same close with a V5 mark, or with a wrong-colour mark, gives no SWEEP.
  - His real file has no 9:42 Sw mark, so at that close the line goes back to WATCH.
  - The 10:54 PDL candle forming shows TESTING. At its close, his Bk mark gives `BREAK: PDL ↓`, settled by 10:57.
  - The 8:33 close gives `BREAK: PHH ↑` from his mark (2.36 had computed "2hL").
  - Synthetic cases cover a Bk mark that fails the BO rule (no BREAK), a close with no mark (WATCH), the next close clearing the badge, and BkTst TESTING outranking both.
- t_hl2372 [7] covers his NQ case: forming through a level and back under it gives TESTING, not SWEEP.

## Risks / not verified
- The closed BREAK / SWEEP depends on lsRevRead writing the mark into the wy file at the close. HodLod re-reads it every 3 s, so the badge can appear a few seconds after the close.
- Not yet verified on his IRT. After the restart, check the status line `expired` / `clock bar` and look at a chart with an old tag.
- On a 3-minute chart a tag can stay up to about one bar longer in real time, because the clock is the last closed bar's end.
