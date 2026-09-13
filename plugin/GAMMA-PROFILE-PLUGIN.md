# IRT Gamma-Profile Plugin — authoritative status & architecture

**Read this first to continue the plugin.** Self-contained: build, architecture, the
gotchas that cost time, the settings, the render design, and what's next.
Last updated 2026-09-13. Built and iterated to a working v0.2 (setDescription tag; ~5 build
iterations) this session.

---

## WHAT IT IS
An **Investor/RT RTX C++ extension** (`lsGammaProfile.dll`) that draws a **per-strike gamma
node profile** (histogram) + a **reference-level rail** on the EPU26 3-min price chart, aligned
to the instrument's own price axis (`INSTRUMENT_SCALE`). It reads its data from a CSV. It is a
NEW build, separate from (and eventually able to replace the on-chart lines of) the FlexLevels
export.

Status 2026-09-13: **compiles clean, installs, loads, and renders real SPX gamma from the tape,
with a full 34-control settings panel, persisting whether the settings dialog is open or closed.**

## FILES (in repo `plugin/`)
- `GammaProfile.cpp` — the extension source (the one file that matters).
- `GammaProfile.csv` — the data it reads (currently a hand-built real SPX book; see Data).
- `compile.bat` — **double-click to rebuild** (finds VS, sets up the compiler, builds, installs). No admin.
- `build.bat` — the compile+install script compile.bat calls (or run from an x64 Native Tools prompt).
- `README-BUILD.md`, `PHASE1-BUILT.md` — earlier notes (superseded by this file).

## BUILD & INSTALL
- **Compiler:** Visual Studio 2026 (Community, "Desktop development with C++"). The VS2026 latest
  toolset links the SDK's **V143** lib fine (binary compatible).
- **Rebuild:** close Investor/RT, then **double-click `plugin\compile.bat`**. No admin needed.
  (A loaded DLL can't be replaced, hence closing IRT.)
- **Installs to:** `%USERPROFILE%\InvestorRT\dllx64\lsGammaProfile.dll`.
- SDK at `C:\Program Files\LinnSoft\InvestorRT\sdk\c++` (include + `lib\irtsdkV143-x64.lib`).

## THE 4 GOTCHAS THAT COST TIME (do not rediscover these)
1. **DLL name MUST start with `ls`.** IRT only loads `ls*.dll`. `GammaProfile.dll` was silently
   ignored; `lsGammaProfile.dll` works.
2. **Install to the PER-USER folder** `%USERPROFILE%\InvestorRT\dllx64\` — NOT
   `C:\Program Files\LinnSoft\InvestorRT\dllx64\`. IRT scans the per-user folder (it holds all ~44
   shipped `ls*.dll`); the Program Files one is not the scan location. Also: no admin needed for the
   per-user folder. After install, fully quit IRT (windows + tray + Task Manager) and relaunch — it
   scans only at startup. Then Add Indicator -> RTX Extensions -> lsGammaProfile.
3. **Parameters: read them ONLY in the parameter callbacks, never in draw() or calc().**
   The getters (`isBoxChecked`, `getListIndex`, `getIntegerValue`, ...) read the **live dialog
   controls**, which exist only while the settings window is OPEN. Reading them in `draw()` →
   the draw crashes/vanishes when the dialog is closed. Reading in `calc()` → returns false/zero
   (dialog closed) so everything turns off. **Solution (implemented):** the constructor seeds safe
   defaults; `parmsLoad`/`parmsApply`/`parmsUpdt` read + cache into a `Settings cfg` member while the
   dialog is open; `draw()` uses only `cfg`. `parmsLoad` is guarded (only accepts a read if a control
   reads a plausible value) so it never stomps the defaults.
4. **Structure mirrors LinnSoft's `lsSampleRTX.cpp`:** `init/setup/calc/done/destroy` are defined as
   `cppExtension::` members (the SDK base declares them with NO body — the DLL must supply them);
   only `draw` + the `parms*` callbacks are overridden in the derived class; `CreateExtension()`
   `new`s the class, sets flags/desc/version, and returns the pointer. **Do NOT define `pExtension`**
   (the lib owns it). `cpp_main` is exported from the lib automatically because the code references
   `cppExtension` methods (pulls that object in). Flags:
   `POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE` (removed `NO_PARMS` once params were added).

Two workflow gotchas:
- **Syntax-check on Linux** with `clang++ -std=c++17 -fsyntax-only -fdeclspec -DWIN32 -D_WIN32
  "-DRT_LONG=unsigned long long" -I<sdk>/c++/include gp.cpp` — the header takes its Mac branch under
  `__GNUC__`, so `sprintf_s`→`sprintf` errors and `RT_LONG` are **false positives**; grep them out
  (`| grep -v sprintf`). If nothing else errors, it compiles on MSVC.
- **Device-bridge commit lag:** `device_commit_files` reports "written" but the file often syncs
  1–2 min later. ALWAYS re-stage and verify the byte count/content before telling the user to rebuild.

## ARCHITECTURE / DATA PIPELINE
`Skylit tape (SPX per-strike %King book) -> GammaProfile.csv -> plugin reads in draw() -> renders`.
- Today the CSV is **hand-built** from the live tape (see below). **Phase 0** wires the panel to
  write it live (spec: `design/spec-phase0-gamma-export.md`).
- The plugin re-reads the CSV every `draw()` (cheap; the file is ~1 KB), so updating the file +
  repainting the chart refreshes the profile without a rebuild.

### Getting real data from the tape (how the current CSV was made)
The tapereader panel is frozen on weekends, but its debug hooks expose the live/last book. On the
Skylit Atlas tab (`app.skylit.ai/atlas`), via Claude-in-Chrome:
- `window.__gptsDebug` — the panel's debug object.
- `__gptsDebug.map('SPXW')` — returns `{nodes,...}` but was empty (frozen).
- `__gptsDebug.LASTFEED.SPY.j` — Skylit's raw SPY gamma feed: `{levels:[{t,s,l:[{k,v,d,net}]}], derived,
  strike_interval,...}`. `levels[last].l` is the per-strike book; `net` is signed gamma, `d` polarity.
- `__gptsDebug.emBand('SPY')` — gives `scaleUsed` (SPY→ES ≈ 10.0175) and the ES-space EM band.
- The **true SPX (SPXW) book** the operator wants is the on-screen tape (King 7675 = -$12,022K on
  2026-09-12), which he provided as a screenshot; the SPY feed is a different book (King ~765).
- **SPX→ES basis** near expiry ≈ **+2** (ES = SPX + 2). Current CSV is in **SPX price space** (King
  7675) per the operator's request to match the tape numbers; the +2 ES basis is to be applied when
  Phase 0 wires the panel's own conversion.

### CSV format
```
# comments start with #
STRIKE,<price>,<pctKing -100..100 signed>,<rank by |pct|>,<isKing 0|1>[,<type>]
KING,<p>   CW,<p>   PW,<p>   FLIP,<p>   EMH,<p>   EML,<p>   SPOT,<p>   BOOK,SPX
```
`type` (optional 6th field) = KING / GK / RUG / RRUG / PIKA / BARNEY (only KING is populated now).

## RENDER DESIGN (tape-matched; approved via mockups)
- **Color = diverging heatmap** matching the Skylit tape: teal at ~0, **gold** for strong +gamma,
  **magenta** for strong −gamma, blended by |%King|. The King is colored by its own **polarity**
  (magenta here, since it's negative gamma) like the tape — not a special color. `Amplify polarity`
  option reaches the pole colors faster. Polarity is double-encoded via the **sign in the % text**.
- **% OUTSIDE** the bar tip, signed (`−100%`, `+65%`) — readable even on tiny stubs; hidden below an
  abs-% threshold to cut clutter.
- **Node TYPE inside** the bar near the base (KING now; patterns later).
- **Rank bubble INSIDE** at the bar's leading tip — dark circle, white numeral **centered exactly on
  the strike** using `getFontMetrics`/`getTextWidth` (box-centering sat low). Top-5 only.
- **Level rail:** KING line drawn with NO label (the node labels it — avoids a double "KING");
  CW/PW/FLIP/EM-H/EM-L lines, labels on the **LEFT** (off the node labels on the right). Line style
  Solid/Dot/Dash; label position L/C/R.
- **Spot marker** + **header** (book · King).
- Bars grow toward price (invariant: profile always faces price). `Detach` reserves right-margin
  room so bars don't cross candles.

## SETTINGS PANEL (34 params, read via parms* callbacks)
1 Layout: Width(px) · Side(Right/Left) · Detach · Bar thickness(Auto/Thin/Med/Thick) · Rounded ends
2 Nodes: Filter(Top3/Top5/≥Threshold/All) · Threshold% · Below threshold(Grey/Hide) · Scale to(King/Visible max)
3 Color: +Gamma · −Gamma · Midpoint (color pickers) · King colored by(Polarity/Distinct) · Amplify polarity · Translucent bars
4 Labels: Show% · %position(Outside/Inside) · Hide % under(abs) · Rank badge · Rank position(Inside/Outside) · Rank scope(Top5/Top3) · Node type inside · Font size
5 Levels: King · Call Wall · Put Wall · Flip · EM High/Low (each toggle) · Line style · Label position · Extend King · Extend top-node lines
6 Context: Header · Spot marker
Color pickers read back correctly via `getIntegerValue` (guarded to defaults if 0). The enum
`P_*` order in the cpp MUST match the setup() declaration order exactly.

## NEXT STEPS (in order)
1. **Phase 0 — wire the panel to write `GammaProfile.csv`** live (real per-5pt SPX book +
   SPX→ES conversion so it aligns to the panel's own lines + auto-updates). Spec:
   `design/spec-phase0-gamma-export.md`. This also fixes the alignment (currently SPX-space hand-build).
2. **Visual tuning** vs the mockup (`mockups/node-profile-v2.html` render) — spacing, detach margin,
   any remaining label collisions with the operator's *other* indicators (FlexLevels lines, ONHI, DOP).
3. Decide whether the profile's rail replaces the **FlexLevels lines** (turn those off) and whether
   to also draw the secondary (SPY) King and cover the **NQ** chart (needs a 2nd instance/book).
4. **Patterns** (GK/RUG/RRUG/PIKA/BARNEY inside nodes) — doctrine-gated (Phase 6); needs detectors.
5. **Delta profile** (Phase 7) — needs the per-price footprint feed (not flowing yet).

## OPERATOR PREFERENCES that bit us (now in memory)
- Always give the full `cd /d "C:\Dev\gex-signal-tapereader\plugin"` line before `build.bat` (he
  opens a fresh cmd each time). Better: he uses `compile.bat` (double-click) now.
- Step-by-step instructions; mockups before UI changes; one element at a time.
