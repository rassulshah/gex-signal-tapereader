# GammaProfile RTX plugin — build & install

Phase 1 of the IRT indicator: a per-strike **gamma histogram + reference-level rail**,
drawn in a reserved right-edge strip on the price pane, aligned to the chart's own price
axis. Reads `...\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv` (the panel will write that
file in Phase 0; a hand-made sample ships so it draws immediately).

## Files
- `GammaProfile.cpp` — the plugin source.
- `GammaProfile.csv` — sample data (goes into the `lsFlexLevels` folder to test).
- `build.bat` — one-command compile + install.

## Build (no Visual Studio project needed)

1. **Start menu → type `x64 Native Tools Command Prompt for VS 2022`** and open it.
   (This is installed with Visual Studio. It puts `cl.exe` and the C runtime on PATH.
   If you only see "VS 2019" or another year, that prompt is fine too — the V143 lib
   needs VS2022; for VS2019 use the `V142` lib instead: edit `build.bat`'s `LIB143` to
   `irtsdkV142-x64.lib`.)

2. In that window:
   ```
   cd /d "C:\Dev\gex-signal-tapereader\plugin"
   build.bat
   ```

3. Expect `[BUILD OK] GammaProfile.dll created` then `[INSTALLED] ...\dllx64\GammaProfile.dll`.
   - If the **copy** step says *access denied*, close the prompt, reopen it via
     right-click → **Run as administrator**, and run `build.bat` again (or copy the DLL
     into `C:\Program Files\LinnSoft\InvestorRT\dllx64\` by hand).

## Put the sample data where the plugin reads it
Copy `GammaProfile.csv` into:
```
C:\Users\<you>\InvestorRT\rtx\lsFlexLevels\
```
(Same folder as `FlexLevelsExport.csv`.)

## Add it to the chart
Investor/RT → open the **EPU26 3-min** chart → add indicator → **RTX** → **GammaProfile**.
It draws on the instrument pane (OVERLAY + INSTRUMENT_SCALE), so the bars line up with price
with no calibration. The histogram sits in the right ~130px; the King/Call Wall/Put Wall/
Flip/EM-H/EM-L lines span the pane with labels above each line.

## Troubleshooting
- **Compiler error `cannot open irtsdk.h`** — the SDK path in `build.bat` is wrong; fix `SDK=`.
- **`LNK2038 RuntimeLibrary mismatch`** — change `/MD` to `/MDd` in `build.bat`, rebuild.
- **IRT loads the DLL but nothing draws** — confirm `GammaProfile.csv` is in the `lsFlexLevels`
  folder and has `STRIKE,` lines; prices must be in the **chart's** price space (EPU26/ES),
  not SPX. Repaint the chart (scroll one bar) to force a redraw.
- **IRT says the entry point / `cpp_main` is missing** — rebuild adding `/link /EXPORT:cpp_main`
  to the `cl` line in `build.bat`.
