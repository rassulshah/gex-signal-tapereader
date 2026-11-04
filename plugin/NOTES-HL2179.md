# lsHodLod 2.17.8 -> 2.17.9: the HOD/LOD box no longer collapses (2026-11-04)

## Request
Rassul, 11:24 CT, NQ 3-min: "sometimes the hodlod box collapses". The box showed only its header row and the Exp / Act rows. The
SESSION block and the HOD SWEEPS / LOD SWEEPS tables were gone, and the box did not reach the price axis.
- NQ status: TABLE_BOX 2,568,767,625 (57 px), AXIS_FILL off.
- ES status: 155 px, AXIS_FILL on.
- The same symptom was in the status files for GC (566-623, 57 px, AXIS_FILL off, pane 949) and HG (533-590, 57 px, off, pane 930).
- CL, NG and EU were full.
Committed during RTH because he asked for the work. It installs when he next closes IRT.

## Root cause (HodLodLogic.h layoutTable)
- The layout tries 10 pt first, then 9 pt. Inside the 10 pt pass, the sweep sections were dropped (`useSw = false`) as soon as
  SESSION + 2 x sweep width was wider than the pane. That pass then laid out the header-only 2.12.0 box, which fits at 10 pt, so the
  loop stopped there.
- The 9 pt sweep box, which is what ES draws (155 px = 9 pt rows), was never tried.
- A header wider than the pane also made the sweep box fail. In the sweep branch the header width was never dropped, so any extra
  header width did the same.
- stretchToAxis skips a box with no sweep sections. That is why AXIS_FILL was off.
- Why only "sometimes" and why NQ: the margin is thin. ES fits by about 7 px (962 of 969). NQ's 5-digit prices (ON3 range
  "28,512.25 - 28,560.00") and longer Exp text push it over. HG and GC have narrower panes (930 / 949). The text changes during the
  day (2nd-time text, News row, ranges), and so does the Key Levels left keep. So the same chart flips between the two forms.
- Not caused by the Dealer Profile 2.52.3 / ColumnBoard 1.10.0 change, LiquidityProfile 0.73.1, BookFlow 1.34.0, BoxDock / BannerBoard
  or KeepOutDraw.
  - Those only read the published rectangle (HLB_BOX / LRA_HLT_), or give the AVOID boxes.
  - The AVOID boxes are at the top (728,1,978,47) and only move the box.
  - cb::leftKeep is unchanged in 1.10.0 (NQ table left = 2).
  - Side effect now that the box is 155 px tall again: on NQ / GC / HG, Dealer Profile 2.52.3 sees a taller table rectangle. More key
    level badges behind it become "^ / v" edge markers, as on ES. Big Lot docks to the taller HLB_BOX, as on ES.

## Fix (2.17.9)
- The layout now tries these steps in order:
  1. 10 pt, full form
  2. 9 pt, full form
  3. 9 pt, compact 1: no "1/4 ADR . 1 yr" strip caption
  4. 9 pt, compact 2: % bars at half width, 8 px gaps
  5. 9 pt, compact 3: no % bars, 6 px gaps, ranges written "a-b"
- The header-only 2.12.0 box (10 then 9 pt) is used only when no sweep form fits the pane.
- Every level row keeps its name, today's price and its %.
- Compact 1-3 remove only the caption and the bars.
- A header wider than the pane drops its RTH % / bar-left extras (the 2.12.0 box already did this). It no longer drops the sweeps.
  When the stretch to the axis gives the room back, the extras return.
- A form that is too tall for the pane goes to the next step (was `break`, which gave the "pane too small" line).
- stretchToAxis keeps pass 1's form (Opts.fillTier / Layout.swTier). So a compact box also runs to the price axis.
- New status line: `SWEEPS,full|compact N (...)|off (...),font F`.
- Versions: HL_VERSION, hlset::PLUGIN_VERSION and setVersion are all 2.17.9. No settings change.

## Tests (Linux, ASan/UBSan, -Wall -Wextra -Werror)
- tests_hodlod/t_hl2179.cpp: 52,000 passed, 0 failed.
  - [1] His NQ geometry. Pane right 978, Position 7, price edge 735. The font metric is calibrated so the header-only box is about 765 px,
    like his TABLE_BOX.
    - Before: the 57 px box.
    - Now: 155 px with both sweep sections, in compact form. Every row is in its own section, inside the box, with no overlaps.
    - The stretch reaches the axis (978) in the same form, with the same height and texts.
    - GC (949) and HG (930) panes: the sections are drawn and run to the axis.
  - [2] Grid: 6 markets (ES / NQ / CL / NG / HG / GC price formats) x 7 pane widths (930-1900) x 5 font scales x 3 positions x 3 variants
    (his pane, no sweep rows, 300 px pane).
    - The sections are always drawn when rows exist.
    - Boxes stay inside the pane, nothing overlaps, and no item is past the box.
    - Every stretch reaches the axis in the same form.
  - [3] Unaffected: golden_2178.txt holds FNV hashes of 2.17.8's items for the same 1,890 scenarios (written by golden.cpp from the 2.17.8
    header).
    - All 1,596 scenarios where 2.17.8 drew the sections (or had no rows) are byte-identical in 2.17.9.
    - The 294 that 2.17.8 collapsed now have sections.
    - The forms used: full x972, compact 1 x162, compact 2 x114, compact 3 x12.
  - [4] The form order never goes back as the text widens: 10 pt full, then 9 pt full, then compact 1 / 2 / 3. Compact 1+ has no
    caption; compact 3 has no bars. A wide header drops its extras, never the sweeps.
  - [5] Versions, the host SWEEPS line written after the stretch, and stretchToAxis passing the form.
- Existing tests:
  - t_hl2177: 370 pass, 1 fail (its own 2.17.7 version pin).
  - t_hl2178: 16 pass, 1 fail (its own 2.17.8 version pin).
  - t_hl2130: 48 / 48.
  - Its 2.17.7 width grid (1200-2400) is unchanged.
- MinGW `-fsyntax-only -Wall -Wextra` (NOMINMAX + windows.h, then the real irtsdk.h 15.1.6, then HodLod.cpp): the same result as
  2.17.8. 0 errors outside the SDK. The 3 RT_LONG errors are irtsdk.h's Mac branch under GCC. 1 old warning in
  first_extreme_inference.cpp.

## Not verified - needs his IRT
- The real look after the restart on NQ / GC / HG / ES: the SWEEPS line in HodLod.status-<MKT>-180.txt, AXIS_FILL on, and TABLE_BOX
  about 155 px.
- The font metric in the test is calibrated, not IRT's. The exact form his NQ picks (full / compact 1-3) shows in the SWEEPS line.
- The Dealer Profile key-level edge markers and the Big Lot docking against the taller NQ / GC / HG table (the same as ES today).
