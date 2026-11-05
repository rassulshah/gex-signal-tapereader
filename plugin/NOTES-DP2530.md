# lsDealerProfile 2.53.0 (2026-11-04 21:30 CT)

Rassul: "i also want to have levels across the chart for the key levels, volume levels, gamma levels just CR0 and PS0. However i want all the levels to be dim and thin so they dont look noisy". Then "go with proposal A" (lv_mock).

## Change (drawing only)
- Every level that has a badge in the Key Levels column now gets a line across the price area at its price:
  - which levels: key levels, volume levels (DPoc / VAH / VAL / WPoc ...) and the gamma levels CR0 / PS0
  - where: from the left edge of the pane to just before the Key Levels column
  - how it looks: 1 px, solid, about 30% of the level's colour (`fade(colour, 0.70)`, toward C_DARK)
- Rows that are VWAP only get no line. lsSessionVWAP already draws VWAP.
- The line is cut around the HOD/LOD table, so it never runs through the table.
- Flags: `DP_XLINES = true`, `DP_XLINE_FADE = 0.70f`.
- Status line: `XLINES,2.53.0,...` gives the count of lines drawn and of VWAP rows skipped.

## Tests (all passed)
| Test | Result |
|---|---|
| test_dealerprofile_2530 (new). Over 3 scales it checks that every badge has a line at its price, every line is dim, no line runs past the column or through the HOD/LOD table, and the status line and versions are right. | 18 |
| test_dealerprofile_2523 (version pin moved to 2.53.0) | 14 |
| MinGW against the real irtsdk.h | 0 errors |
