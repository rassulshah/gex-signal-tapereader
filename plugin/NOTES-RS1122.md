# R red, S green (2026-11-22): lsCobSignal 1.11.1, lsHtfSR 1.0.2, lsLimitProfile 2.69.1

## Requests (Rassul, 2026-11-22 CT)
- 19:41, with his CL chart: "also the R1 R2 and S1 and S2 levels should be colored.  R should be red and S should be green ."
- 20:22, on the mockup (now-line-mockup.html, picture 2 "R / S colours") and the two options: "A" = names and lines red / green everywhere, including where R or S sits on a key level; the Key Levels column badges keep their colours.

## Why three plugins
The 3-hour R1 / R2 / S1 / S2 are painted by three plugins. On his chart both R levels sat on key levels (R2 = PFH / PDH, R1 = ONH / AsiaHI), so the line he saw was the Limit Profile's key-level line (teal, grey), not the R / S plugin's.
- lsCobSignal (his 3-minute charts): its own line across the chart and the name at the last candle. Was one orange (0x00FF9F1A).
- lsHtfSR (charts without lsCobSignal): the same line and name. Was the same orange.
- lsLimitProfile: the level line of a row in its Key Levels column. A row that is ONLY R / S draws no line there (the two above draw it); a key level that also carries R / S ("PFH/PDH/R2") drew its line in the key level's colour.

## What changed
- lsCobSignal 1.11.1 (CobSignalLogic.h htfLayout): R1 / R2 line and name red (C_BEAR 0x00EF4444), S1 / S2 green (C_BULL 0x0022C55E). Same 1 px line, same place.
- lsHtfSR 1.0.2 (HtfSr.h layout): the same. HtfSr.h also gets C_RES / C_SUP, rsSide(name) (+1 the name carries R1 / R2, -1 S1 / S2, 0 neither; the first R / S name decides) and rsColour(side, other).
- lsLimitProfile 2.69.1 (DealerProfile.cpp, the cross-chart level lines): a row whose names carry R1 / R2 draws its line red, S1 / S2 green - the row's own fade and width are kept (key rows 55% of the colour, 2 px). Rows with no R / S are unchanged. The badges are unchanged (a PDH/R2 badge stays PDH's colour). The short tick between the line and the badge keeps the badge's colour. Status: HTFSR line ends ",lines in the R / S colour (2.69.1) N".
- Not changed: every other level line, the liquidity stretches on the lines (blue / orange), the Key Levels column, any calculation. No setting.

## Tests (2026-11-22, research machine)
- Real irtsdk.h syntax check, C++14, -Wall -Wextra: lsCobSignal 0 errors 0 warnings; lsHtfSR 0 errors 0 warnings; lsLimitProfile 0 errors, the same 7 warnings as before the change.
- tests_htfsr/test_htfsr.cpp: 96 passed, 0 failed (62 before; +34: the colours of every line and name, rsSide on 22 names, rsColour), ASan / UBSan, -Werror.
- tests_cobsignal/test_cobsignal_htf.cpp on the recorded ES chart: 47 passed, 0 failed (44 before): every piece red or green, each name in its line's colour and the colour its letter says, no resistance line under a support line. One pinned check made version-agnostic ("VERSION,1.11.0").
- test_dealerprofile_2691.cpp (new host test: the real plugin on the mocked SDK calls of test_dealerprofile_2680.cpp, ASan / UBSan / LeakSanitizer clean): 22 passed, 0 failed - PDH/R2 line red and PFL/PDL/S1 line green at the key-line fade and 2 px, PFH and PWH unchanged, both badges keep their colours, a row that is only R / S draws no line here, "R2/PDH" (R first) red, a second draw gives the same picture and the counter does not grow, the status line.
- Old against new on one scene (the same levels drawn by the 2.69.0 and the 2.69.1 source): of 72 drawing calls exactly 3 differ - the header's version text and the two level lines (0x91610b -> 0x882c30, 0x045257 -> 0x17733e).
- test_dealerprofile_2680.cpp (the older host suite): 83 passed, 4 failed on the old AND on the new source - the same four (version pins 2.68.0 / 2.65.1 and a 2.68.0 line-brightness expectation that 2.69.0 changed). Not fixed here.

## Rebuilds
HtfSr.h, CobSignalLogic.h and DealerProfileSettings.h are listed by other plugins, so gex-build also rebuilds lsHodLod, lsBreakout, lsBreakout1, lsZzoCombo, lsZzoComboMarks, lsVolRead and lsRangeRead. None of them uses the two colour names that were removed (checked in every file that includes the headers); nothing changes in them.

## Not verified (needs his IRT)
- Seen on his chart: after his restart (status files, ChartView export).
- Where R / S shares a price with a key level, lsCobSignal's own 1 px line and the Limit Profile's 2 px line lie on each other: both are red (or green) now, the brighter 1 px inside the dimmer 2 px.
- VAH / VAL lines are also red (his 2026-10-19 rule) and the hourly trend steps are red / green: on a chart with all of them the R / S lines are told apart by their name at the last candle.
