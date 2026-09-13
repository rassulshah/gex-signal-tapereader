# CHANGELOG — IRT Gamma-Profile Plugin (separate from the 1 MB panel CHANGELOG.md)

## 2026-09-13 — lsGammaProfile v0.2 (working, full settings)
**Built the RTX gamma-profile plugin end to end.** New C++ Investor/RT extension
`lsGammaProfile.dll` that draws a per-strike gamma histogram + level rail on the EPU26 chart.

- **NEW:** `plugin/GammaProfile.cpp` — the extension. Reads `GammaProfile.csv`, draws the histogram
  (diverging tape colors), the level rail, labels (% outside, type inside, rank bubble inside centered
  on the strike), spot marker, header.
- **NEW: 34-control settings panel** (layout, node filter/threshold, color pickers, label toggles,
  line toggles, context). Params read via `parmsLoad/parmsApply/parmsUpdt` and cached (NOT in draw/
  calc — the getters read live dialog controls). Constructor seeds defaults so it renders with the
  dialog closed.
- **NEW: real data** — profile driven by the real SPX gamma book from the Skylit tape (King 7675),
  hand-built into `GammaProfile.csv` (SPX price space; ES basis +2 pending Phase 0).
- **NEW: build tooling** — `compile.bat` (double-click, no admin, installs to per-user folder) +
  `build.bat`. **NEW docs:** `plugin/GAMMA-PROFILE-PLUGIN.md` (authoritative).
- **REASONING / lessons:** DLL must be named `ls*`; install to `%USERPROFILE%\InvestorRT\dllx64`
  (not Program Files); read params in the parms callbacks only; mirror `lsSampleRTX` structure; the
  header's Mac branch makes `sprintf_s`/`RT_LONG` false-positives in a Linux clang syntax-check;
  device-bridge commits lag ~1–2 min (verify before rebuild). Full detail in the plugin doc.
- **NOT YET:** Phase 0 (panel writes the CSV live), visual tuning, patterns, delta. See build plan.
