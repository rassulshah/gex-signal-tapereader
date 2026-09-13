# RESUME — 2026-09-13 · IRT Gamma-Profile Plugin (session focus)

**This session was NOT panel work** — it built the **IRT RTX C++ gamma-profile plugin**
end to end. The panel (v15.99) is unchanged. To continue the plugin, read
**`plugin/GAMMA-PROFILE-PLUGIN.md`** (authoritative: build, architecture, the 4 gotchas,
settings, render design, next steps). This note is the session briefing.

## WHERE WE LANDED
`lsGammaProfile.dll` **compiles, installs, loads, and renders** the real SPX gamma node profile +
level rail on the EPU26 3-min chart, with a **34-control settings panel**, and it **persists whether
the settings dialog is open or closed**. Rank badges show, numbers centered on the strike, tape-
matched colors. This is the working state at end of day.

## HOW IT WAS A JOURNEY (so it isn't repeated)
1. Wrote `GammaProfile.cpp` against the SDK header; installed VS 2026 to compile.
2. **Naming:** IRT only loads `ls*.dll` → renamed to `lsGammaProfile.dll`.
3. **Folder:** IRT scans `%USERPROFILE%\InvestorRT\dllx64`, NOT Program Files → build.bat fixed.
4. **Real data:** pulled the live book from `__gptsDebug.LASTFEED.SPY.j` (Skylit feed) via
   Claude-in-Chrome; the operator supplied the true **SPXW tape** (King 7675). Built the CSV from it.
5. **Design pass** (mockups first, approved): tape diverging colors, % outside, type inside, rank
   bubble inside centered on the strike.
6. **Settings panel** (34 controls) built and approved.
7. **The hard bug:** parameters read in draw() crashed when the dialog closed (getters read live
   controls); moved to calc() → read false; **final fix = read in parmsLoad/Apply/Updt, cache in a
   member, draw() uses the cache, constructor seeds defaults.** See plugin doc gotcha #3.
8. **Tooling:** `compile.bat` (double-click, no admin — installs to the per-user folder).

## THE STANDING BUSINESS REQUIREMENT (operator's words, this project)
A data-driven trading decision-support system; everything on the dashboard goes through
capture → analysis → testing → back to the dashboard, tied to the purpose (HOD/LOD turning points &
deflections). The gamma-profile plugin is the IRT-side visualization of the dealer-gamma structure
that the deflection study is built on. **Match Skylit** is a standing rule — the profile mirrors the
tape (that's why the colors and King match the tape).

## NEXT ACTIONS (in order) — start here tomorrow
1. **Phase 0** — wire the panel to write `GammaProfile.csv` live (real per-5pt SPX book + SPX→ES
   conversion, self-updating). Spec: `design/spec-phase0-gamma-export.md`. Fixes the alignment
   (currently the CSV is hand-built in SPX space, King 7675; ES basis ≈ +2 to apply).
2. Visual tuning vs `mockups/node-profile-v2.html`; decide whether to retire the FlexLevels on-chart
   lines (the profile's rail can carry them) and whether to cover NQ / draw the secondary SPY King.
3. Phase 6 patterns (doctrine-gated), Phase 7 delta (needs footprint feed).

## TRUST / DATA NOTES
- The SPX book used is **Friday 2026-09-12 close** (weekend; panel frozen). The SPY feed grabbed was
  a Thursday snapshot — the operator's SPXW tape screenshot (King 7675) is the authoritative book used.
- CW/PW/FLIP not currently drawn (InsiderFinance companion was toggled off) — King + EM band + nodes
  are the real, drawn data.

## HOW THE OPERATOR WORKS (unchanged, reinforced this session)
One thing at a time; mockups before UI; step-by-step build instructions **with the full `cd /d` path**
(now `compile.bat` double-click). Do not rewrite settled design without showing a mockup.

## SAVE NOTE
The 242 KB `latest-resume-note.md` and 1 MB `CHANGELOG.md` were **not rewritten** (too large to
safely edit through the device bridge). Plugin work is captured in: this file,
`plugin/GAMMA-PROFILE-PLUGIN.md`, `changelog/CHANGELOG-plugin.md`, `design/IRT-BUILD-PLAN.md`,
`design/GAMMA-PROFILE.md §11`, and a pointer at the top of `session-state/LOCKED-ITEMS.md` +
`design/DATA-ARCHITECTURE.md`.
