# RESUME — 2026-09-14 · Phase 0 live feed + the day-model stats strip (session focus)

**This session wired the LIVE FEED for the IRT plugins and started the day-model stats strip.**
Panel is now **v16.01** (was 15.99). The plugins (lsGammaProfile v0.37, lsDayModel v0.6) are unchanged
this session — only the panel changed, plus docs. Read `design/DAY-MODEL-PLUGIN.md` and CHANGELOG
v16.00/16.01 first.

## WHERE WE LANDED (shipped, on origin)
- **v16.00** — the panel WRITES `GammaProfile.csv` (`gammaProfileBuild` / `gammaProfileExportNow`), the
  compact CSV the RTX plugins parse, into the SAME `lsFlexLevels` folder as `FlexLevelsExport.csv`
  (untouched), same 180s tick + ⇩ button, behind `CFG.irt.profileOn` (default on). Everything in the
  current futures symbol's ES price space → the profile + day-model candle now track live price on ANY
  contract (fixes "displayed then disappeared" on the EPZ26 roll).
- **v16.01** — front-month AUTO-ROLL. `esFrontSym()/nqFrontSym()` → `irtResolveAutoSym()` sets
  futSym/nqSym from the date (roll ~8d before the quarterly 3rd-Friday expiry). New **Auto** checkbox in
  the gear (default on); typing a symbol pins it + turns Auto off. Both files follow the front month.

## THE MODEL — CONFIRMED BEFORE WIRING (the operator insisted, twice)
The EXPECTED day candle (`DAYEXP/DAYEHOD/DAYELOD/DAYEMUD`) is fed by the **TESTED base-rate model**:
`hodlodBaseFor(weekday)` → **BASERATES.json / HODLOD_BASE** (297 ES sessions, 2025-06→2026-09, trimmed
mean, per weekday: firstClock/secondClock, took/gap, range pts/$, wick family). Anchored on today's RTH
open, range split symmetrically (v1 — "blended with the IF EM").
- ⚠ The heavily-tested model with **AUC 0.879** (grouped CV, calibration, ablation — reproduced from the
  corpus this session) is the **CLASSIFIER** (`model-lodhod.py`, HLTAB) — the READ ("LOD IN 84%"), a
  SEPARATE layer. It is NOT the candle's geometry and is NOT written to the CSV. The two evidence bases
  stay unfused (`mockups/hodlod-v2-SPEC.md`). Documented in ARCHITECTURE-E2E-WORKFLOW, DASHBOARD-INVENTORY
  (line ~411: "E row = expected from 284 sessions, BASERATES.json"), PROCESS.md.

## KEY IMPLEMENTATION FACTS (so they're not rediscovered)
- **The ruler.** SPX→ES uses Skylit's OWN SPXW ES ratio (`skylitFutPx('ES1','SPXW',k)` ~1.0003),
  persisted last-good — NOT `R.r` (the ~10× SPY ratio). DAY rows are ES-NATIVE (measureBars reads the ES
  chart's bars) — no conversion.
- The gamma STRIKE/KING rows need the **ES1 derived payload** (self-fetched once/min while on an ES
  chart). If absent, DAY rows still write (ES-native); the ladder waits for the ratio.
- Data fns: `tapeMap('SPXW')` (.pct/.king), `measureBars(activeSym())` (open/hod/lod/hodT/lodT/first/
  second/took/gap/bop/wick/wend/wickPct/mud/rngPts/rngUsd/scale), `sessionBody` (open/close/hi/lo),
  `sweepEventsShown` (level/px/at/status), `hodlodBaseFor(dow)`.
- Debug: `window.__gptsDebug.gp()` (preview rows) · `.gpExport()` (force write).

## NEXT ACTIONS (in order) — the operator wants the INDICATOR
1. **The stats strip** (§10.2) — the SECOND day-model piece, "the other indicator". Two rows EXP/ACT,
   field order `1ST · [1st] · Took · BOP · Wick · W.End · Wick% · MUD · MUD t · [2nd] · HL Gap · HL Rng`;
   dynamic first-extreme; MUD-t = HL Gap − BOP; weekday header; 12h times, Xh Ym durations. Operator
   words: "the other indicator which displays the stats on the top like in the mockup" → a SEPARATE
   plugin (lsDayStats), like the candle is separate from the gamma profile.
   - FIRST: add the stat fields to the CSV export (BOP/Wick/W.End/Wick%/HLGap for ACT and EXP). ACT from
     measureBars, EXP from `hodlodBaseFor().wick` + firstClock/secondClock/tookMin/gapMin/rngPts.
   - THEN: build `lsDayStats.cpp` (mirror lsDayModel's build: build-daystats.bat + compile-daystats.bat).
2. Later refinements: directional expected-close (candle body color), E-HOD/E-LOD price blend with IF EM,
   EMH/EML rows (skipped in v16.00 — off by default in the gamma profile anyway).

## HOW THE OPERATOR WORKS (unchanged)
ONE thing at a time; discuss/mockup before building; step-by-step instructions with the full
`cd /d "C:\Dev\gex-signal-tapereader\plugin"` path for plugin builds; Tampermonkey link as a CLICKABLE
link; wait 5 min after a push then reload Atlas. He verifies by version number — bump the display version
every build.
