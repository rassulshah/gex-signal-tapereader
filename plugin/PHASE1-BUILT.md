# Gamma Profile RTX plugin — Phase 1 BUILT & RENDERING (2026-09-12)

First live render of the IRT gamma-profile indicator confirmed on the EPU26 3-min chart.
This note captures the hard-won build/install facts so a future session doesn't re-derive them.

## What's built
- `GammaProfile.cpp` — RTX extension. Reads a CSV, draws a per-strike gamma histogram
  (gold +γ / purple −γ / cream King) in the right margin + a level rail
  (KING / CALL WALL / PUT WALL / FLIP / EM-H / EM-L), aligned to the instrument price
  axis via `INSTRUMENT_SCALE | OVERLAY | POST_DRAWING | NO_UI | NO_PARMS`.
- Structure mirrors LinnSoft's `lsSampleRTX.cpp`: `init/setup/calc/done/destroy` defined as
  `cppExtension::` members (base declares them with NO body — must be supplied); only `draw()`
  overridden in the derived class; `CreateExtension()` news + sets flags/desc/version + returns
  the pointer (do NOT define `pExtension`; the lib owns it; `cpp_main` is exported from the lib).

## Build (worked, first try, no code errors)
- Compiler: **Visual Studio 2026 Community, "Desktop development with C++"** → the
  **"x64 Native Tools Command Prompt for VS"** (run **as Administrator** if installing to
  Program Files; not needed for the per-user folder).
- `build.bat` (in this folder): `cl /LD /EHsc /O2 /MD GammaProfile.cpp /I <sdk>\c++\include
  /Fe:lsGammaProfile.dll /link <sdk>\c++\lib\irtsdkV143-x64.lib`
- SDK: `C:\Program Files\LinnSoft\InvestorRT\sdk\c++` (include + lib). V143 = VS2022 toolset;
  VS2026's latest toolset links it fine (binary compatible).

## Two install gotchas that cost time (REMEMBER)
1. **The DLL name MUST start with `ls`** — Investor/RT only loads `ls*.dll` from its extension
   folder. `GammaProfile.dll` was silently ignored; `lsGammaProfile.dll` works.
2. **Install to the PER-USER folder, not Program Files** — IRT scans
   `C:\Users\<user>\InvestorRT\dllx64\` (holds all ~44 shipped `ls*.dll`). The
   `C:\Program Files\LinnSoft\InvestorRT\dllx64\` folder is NOT the scan location.
   `build.bat` now installs to `%USERPROFILE%\InvestorRT\dllx64`.
- After copying the DLL, **fully quit IRT** (windows + system tray + Task Manager) and relaunch —
  it scans the folder only at startup. Then Add Indicator → RTX Extensions → `lsGammaProfile`.

## Data
- Plugin reads `%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv` (same folder as
  `FlexLevelsExport.csv`). A hand-made sample is there now. CSV format:
  `STRIKE,<price>,<pctKing>,<rank>,<isKing 0|1>` plus `KING/CW/PW/FLIP/EMH/EML,<price>`.
  Prices are in the CHART's space (EPU26/ES), not SPX.

## Next
- Tune the render vs the mockup (detached margin so price doesn't overlap; label positions;
  bar thickness) — ONE thing at a time, per his workflow.
- Phase 0: wire the panel to write `GammaProfile.csv` (spec in design/spec-phase0-gamma-export.md).
