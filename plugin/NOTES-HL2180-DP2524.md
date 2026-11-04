# lsHodLod 2.18.0 + lsDealerProfile 2.52.4 (2026-11-04 17:00-17:10 CT)

Rassul 16:59 CT: "in the hodlod box, i want you to swap the LOD Sweeps with HOD sweeps"
Rassul 17:02 CT: "also update PHL and PHH to PhrL and PhrH"

## lsHodLod 2.18.0
- The box now reads SESSION | LOD SWEEPS | HOD SWEEPS (it was SESSION | HOD | LOD). Strip colours are unchanged: LOD is green, HOD is red.
- The prior-hour sweep rows are shown as PhrH / PhrL. The key in LRA-HodLodSweeps and the LRA-KeyLevels file name stay PHH / PHL.
- The version is 2.18.0 rather than 2.17.10 because the longer version text widened the header and dropped header extras on some narrow panes. "v2.18.0" is the same width as "v2.17.9".
- Includes everything in 2.17.9, which was never installed.
- Test `tests_hodlod/t_hl2180.cpp` (6305 checks, 0 failed):
  - Across all 1890 scenarios of the 2.17.9 grid, the box, the sweep form and every text are identical to 2.17.9. The baseline `grid_2179.txt` is written by `gen_2179.cpp` from the 2.17.9 header.
  - In every layout with sweep sections, the LOD title and rows sit in the left section and the HOD ones in the right.

## lsDealerProfile 2.52.4
- `klv::displayName` shows PHH / PHL as PhrH / PhrL, including in merged badges ("ONH · PhrH").
- Colour, key-level and description lookups accept both spellings.
- The file and record names stay PHH / PHL, so the nightly, lsCobSignal and lsHodLod still match on them.
- Tests:
  - `tests_hodlod/t_phr.cpp`: 8 passed.
  - `test_keylevels_logic`: 62 passed.
  - `test_dealerprofile_2523`: only its 2.52.3 version check fails, which is expected.
