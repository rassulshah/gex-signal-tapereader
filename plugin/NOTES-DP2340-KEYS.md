# Levels column v2 (lsDealerProfile 2.34.0, lsHodLod 2.11.0, ColumnBoard 1.9.1) - 2026-10-28

Request (Rassul 12:50 CT): only key price levels + volume levels + ONE VWAP (the latest), no gamma; odds left-justified, name + price
right-justified; the HOD/LOD box right of the Levels column at the bottom-left (not in it); no S / R squares in the HOD/LOD strip; the white
spot price smaller so it does not run into the row below; Delta + Liquidity bars facing price.
Mockup: /mnt/user-data/outputs/levelscol2/mockup.html (ES 22 Oct 11:00-15:00 CT, real data).

## Changes
- lsDealerProfile 2.34.0: the Key Levels column holds key + volume levels + ONE VWAP badge (latest session VWAP computed natively from the
  chart bars with SessionVWAPLogic: RTH during / after RTH, else overnight; never the +-1 / +-2 bands; not written to LRA-KeyLevels-<MKT>.csv).
  Gamma-named levels, VWAP bands and a second VWAP are filtered (status KEYS_COLUMN: VWAP, badges, filtered). Odds text LEFT-justified at the
  column's left edge, name chips + price badge RIGHT-justified at its right edge (odds dropped first when a row is too narrow). The column keeps
  230 px and its full height (no longer widened / shortened by the HOD/LOD table). HELD (2.32.0) kept: the word steps right of the HOD/LOD table,
  the bright line skips the part under the table (status: line parts under the HOD/LOD table skipped).
- lsHodLod 2.11.0: docks bottom-left of the price area, immediately right of the Key Levels column (ColumnBoard 1.9.1 dockKeys: left = column
  right + 4, bottom = pane bottom - 6); the column is a keep-out again. Gex S/R ladder: no S / R squares; spot price font = strike font + 1
  (was + 3), drawn in its own row only (second row starts 1 px above the first row's bottom, was 4).
- ColumnBoard 1.9.1: dockKeys(..., keysW) bottom-left right of the column; DOCK_GAP 4.
- Delta Profile / Liquidity column: verified in code, no change - DeltaProfile bars anchored at delta.r - 1 growing left (DELTA_LEFT false, no
  mirror); DealerProfile liquidity column (outerCol) bars anchored at BAR_R growing left toward price.

## Tests (Linux, mock host)
- test_dealerprofile_2340.cpp (new): 73 passed, 0 failed (one VWAP, no bands / gamma, odds left / badges right, dpLatestVwap = SessionVWAPLogic,
  table docked right of the column, HELD line + word clear of the table, also after a restart).
- test_columnboard_191.cpp (new): 22 / 0. test_columnboard_190.cpp is superseded (old dockKeys signature).
- tests_hodlod/t_hl230 46 / 0, t_hl244 47 / 2 (the 2 version-literal failures are pre-existing); new checks: no S / R, spot = strike font + 1,
  spot box above the distance row.
- Unchanged vs before: keepout 44/0, cb180 10/0, t_hl240 43/2, t_hl270 316/1, t_po260 131/2, dp2301 / dp2291 pass, LP lmtf / lt_lp / candle
  keepout, BBA126 60/2 (all failures pre-existing).

## Not verified - needs his IRT
- Big Lot box (BoxDock beside the HOD/LOD table) now follows the table to the bottom-left.
- lsSessionVWAP still draws its own VWAP / band lines if it is on the chart (not touched by request).
