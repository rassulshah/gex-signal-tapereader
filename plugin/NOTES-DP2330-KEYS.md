# Key Levels column (lsDealerProfile 2.33.0) - 2026-10-28

Request (Rassul 12:16-12:18 CT): "i want a column for levels and move all the level badges in the levels column because the chart is too
messy. also move the hod lod box aligned with the liquidity profile on the left" ... Levels column at the LEFT EDGE; "build everything and
do it now". Mockup: mock_full.png / mock_left_zoom.png (ES 3-min 28 Oct 08:06-11:33, real levels).

## Layout (left -> right)
[KEY LEVELS column: HOD/LOD table docked at its top, level badges below at their price rows] | candles + thin level lines |
LMT F | READ | Delta | Liquidity (unchanged) | axis

## What changed
- ColumnBoard 1.9.0: the Key Levels column's own board key LRA_COLV_ (publishKeys / peekKeys / keysFresh, width 120-460, default 230);
  leftKeep() keeps every plugin right of it (withKeys = false for lsHodLod, whose table docks inside it); cb::dockKeys() = where the
  table docks (top-left of the column, under the 22 px IRT title row; not when the table is taller than the pane).
- lsDealerProfile 2.33.0: every level badge (key / volume levels, merged names "PFH/PDH", price, odds "86% / 91%") in the column at its
  own price row, stacked in price order without overlap (cb::stack), a tick from a moved badge to its line; badges start below the
  docked HOD/LOD table (a level the table covers gets its badge right below it); the column is as wide as the table when the table is
  wider; the column ground is never painted over the table; level lines run from the column to LMT F; no badge in the price area; the
  liquidity rank chip on the badges is gone (the liquidity column has its own ranks). A badge too wide drops its odds first, then the
  last chips of a merged row. Status line KEYS_COLUMN. No settings change.
- lsHodLod 2.10.0: while the Key Levels column is on the chart, the table + strip / Q header / rows dock at its top-left (whatever the
  Position setting); otherwise the Position setting as before. Status line KEYS_DOCK in the PLACE block. No settings change.
- lsLiquidityProfile 0.66.1: the top READ banner and the order lines start right of the Key Levels column.
- lsBigBidAsk 1.31.3: nothing drawn under the Key Levels column (leftLimit).
- lsDeltaProfile 2.30.1: A circles / marks stay right of the Key Levels column.
- lsDefenseLines: no code change; rebuilt with ColumnBoard 1.9.0, so its lines / dots now also stop at the Key Levels column.

## Tests (Linux, mock host)
- test_columnboard_190.cpp (new), test_dealerprofile_2330.cpp (new, real DealerProfile.cpp), the existing ColumnBoard / DP / HodLod /
  keep-out / Delta / LP / BBA suites - counts in the build report.

## Not verified - needs his IRT
- Draw order between plugins: the HOD/LOD table and the Dealer Profile's column ground - the ground skips the table's rectangle, so
  either order shows the table.
- The Big Lot box docks beside the HOD/LOD table (BoxDock): with the table in the column it sits right of the table at the top.
