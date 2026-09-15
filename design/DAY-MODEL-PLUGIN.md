# lsDayModel — the Day Model IRT plugin (status)

**A SEPARATE RTX indicator from lsGammaProfile** (built 2026-09-13/14). Kept separate on purpose
so its settings dialog / parameter numbering can never destabilise the mature gamma profile (the
parameter-scramble history). Both plugins read the same CSV; each ignores the other's rows.

- Source: `plugin/DayModel.cpp` · build: `plugin/build-daymodel.bat` (+ double-click
  `plugin/compile-daymodel.bat`) → `lsDayModel.dll` → `%USERPROFILE%\InvestorRT\dllx64`.
- Reads `%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv`. **LIVE since panel v16.00** — the
  panel writes it (`gammaProfileBuild`), in the current futures symbol's ES price space, and the contract
  auto-rolls (v16.01). No longer a hand-made fixture.
- Flags: `POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE`. Price-aligned to the IRT axis.

## What is BUILT (current version **0.13**)
The **candle** piece of the day model (design §10.3), both candles in a reserved margin:
- Expected (ghost, dashed) + Actual (solid developing), colour by close-vs-open, split wicks.
- Actual: HOD/LOD tips (value / time / duration), MUD box in the body (label on its own line above the
  points, v0.13), swept-level dashed ticks with two-line tags, day-of-week header.
- Expected: E-HOD / E-LOD tips + E-MUD box in dim ghost ink, and **a real body** (v16.18+).
  ⚠ **The expected candle is now the ADAPTIVE model — see `design/DAY-MODEL.md`, the one source of truth.**
  Its RANGE is predicted from today's opening range (`E.predict`, self-calibrating), NOT the weekday mean;
  its body leans with the opening drive. `hodlodBaseFor(weekday)` still supplies the CLOCKS and wick family.
  The `DAYEXP` close ≠ open now, so `DayModel` draws a real body automatically.
- Background panel (dark, bordered) so chart price labels don't bleed through.
- Settings (17 params, all explicit `pc++`, NO setLabelParameter): Side, Layout (Pair/Overlay),
  Candle width, Pair spacing, Margin gap, Background fill, show Expected/Actual, Up/Down colour,
  HOD/LOD tips, MUD box, Swept levels, Day header, E-HOD/E-LOD lines, Font, EXP/ACT tags.

## Version history
- v0.1 candle pair · v0.2 finished actual candle (HOD/LOD tips, MUD box, swept ticks) ·
  v0.3 background panel + adjustable spacing + E-HOD/E-LOD/E-C · v0.4 day header, E-lines toggle,
  swept-tag price · v0.5 two-line swept tags · v0.6 expected candle time/duration/MUD ·
  v0.7 (2026-09-14) `setParameterVersion(5)` reset to correct defaults, `annoL` fix, weekday+date header ·
  **v0.13 (2026-09-15) MUD label on its own line above the points (actual + E-MUD); the expected candle now
  carries a real directional body because `DAYEXP` close ≠ open (the adaptive model — see DAY-MODEL.md).**

## CSV schema (day-model rows; gamma plugin ignores them)
```
DAYEXP,o,h,l,c            DAYACT,o,h,l,c
DAYHOD,value,time,dur     DAYLOD,value,time,dur     DAYMUD,pts,time,dollar
DAYEHOD,value,time,dur    DAYELOD,value,time,dur    DAYEMUD,pts,time,dollar   (expected — ADAPTIVE)
EXPMODEL,basis,rngPts,driveSign   (v16.18: which stage drew the expected candle — exante|open30|open60)
READ,first,posr%,cellPct,cellN,call   (v16.17: the HLTAB READ — Layer 2, rendered on the DayStats title line)
SWEPT,name,price,time,R|B|T   (highs & lows only: PDH/PDL·ONH/ONL·PWH/PWL·PFH/PFL)   WEEKDAY,Mon..Fri,date
```

## THE STATS STRIP — lsDayStats (design §10.2) — BUILT v0.5 (2026-09-15)
The SECOND day-model piece, a SEPARATE plugin (`plugin/DayStats.cpp` → `lsDayStats.dll`). v0.4 added per-cell
colour (LOD red / HOD green / MUD by phase) and put the EXPECTED row on top; **v0.5 renders the READ
(Layer 2, the HLTAB `READ` row) center-justified on the DAY STATS title line — "LOD IN 84%", green=IN /
amber=NOT IN.** It reads two CSV rows and renders Block 1 of §10.2: two rows, ACTUAL (A) over EXPECTED (E,
now the ADAPTIVE model's numbers, marked `~`),
columns `1ST · [1st] · Took · BOP · Wick · W.End · Wick% · MUD · MUDt(=HL Gap−BOP) · 2ND · [2nd] · HL Gap ·
HL Rng`. A corner text panel (not price-aligned; no INSTRUMENT_SCALE). Robust settings (8 params, explicit
`pc++`, `setParameterVersion(1)`): Corner, Font, Actual row, Expected row, Column headers, Background panel,
X/Y inset. The two rows it consumes (ONE fixed 17-field order; raw sec-of-day clocks, raw minutes, 2dp prices,
empty = unknown — the plugin formats to 12h / "Xh Ym"):
```
DAYSA,first,firstPx,firstClkSo,tookMin,bopMin,wickMin,wendSo,wickPct,mudMin,second,secondPx,secondClkSo,gapMin,rngPts,rngUsd,,     (actual, from hodLod)
DAYSE,first,firstPx,firstClkSo,tookMin,bopMin,wickMin,wendSo,wickPct,mudMin,second,secondPx,secondClkSo,gapMin,rngPts,rngUsd,rngP25,rngP75   (expected, from hodlodBaseFor)
```
Build: `plugin/compile-daystats.bat` (double-click) or `build-daystats.bat`. **The READ (Block 3, the
AUC-0.879 classifier — Layer 2 in DAY-MODEL.md) now rides the title line as of v0.5. The full elapsed-time
ladder (Block 2) as its own strip is still later work.**

## What's NEXT
1. Compile lsDayStats, add it to the chart, calibrate column widths / corner position against the live pane.
2. See `design/KING-TRACKER.md` for lsKingTracker (also source-ready v0.1) — #4 on the roadmap.

## DONE (panel v16.00 / v16.01)
- **Phase 0 transport** — the panel WRITES `GammaProfile.csv` (per-strike gamma + levels + day-model),
  in the current futures symbol's ES price space; the contract auto-rolls (front-month). Replaced the
  fixture. See CHANGELOG v16.00/16.01.
- **Expected feed** — `DAYE*` from the ADAPTIVE model (v16.18+): range predicted from today's opening range
  via `E.predict` (self-calibrating, re-fit nightly), body from the opening drive; the weekday reference
  (`hodlodBaseFor`) supplies the clocks. See `design/DAY-MODEL.md` — the one source of truth for the model.

## Companion change, same session
`lsGammaProfile` v0.37 (display version bumped, **parameter version unchanged at 5** so existing
instances keep their settings): new DEFAULTS — Title header OFF, EM H/L OFF, Panel at = Bottom-C.
Defaults only; apply to the operator's existing chart by toggling the 3 manually or re-adding.

## Known / deferred
- The candle is PRICE-ALIGNED, so scrolling/zooming the price axis moves it off screen (by design).
- Device-bridge commit FLUSH LAG: after every `device_commit_files`, re-stage and verify BY CONTENT
  (bytes can read stale); `force:true` + a short wait, then confirm the version string / a marker.
