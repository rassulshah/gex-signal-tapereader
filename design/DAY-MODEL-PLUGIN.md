# lsDayModel — the Day Model IRT plugin (status)

**A SEPARATE RTX indicator from lsGammaProfile** (built 2026-09-13/14). Kept separate on purpose
so its settings dialog / parameter numbering can never destabilise the mature gamma profile (the
parameter-scramble history). Both plugins read the same CSV; each ignores the other's rows.

- Source: `plugin/DayModel.cpp` · build: `plugin/build-daymodel.bat` (+ double-click
  `plugin/compile-daymodel.bat`) → `lsDayModel.dll` → `%USERPROFILE%\InvestorRT\dllx64`.
- Reads `%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv` (the CSV is still a HAND-MADE
  fixture — Phase 0 "panel writes it" is NOT wired yet).
- Flags: `POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE`. Price-aligned to the IRT axis.

## What is BUILT (current version **0.6**)
The **candle** piece of the day model (design §10.3), both candles in a reserved margin:
- Expected (ghost, dashed) + Actual (solid developing), colour by close-vs-open, split wicks.
- Actual: HOD/LOD tips (value / time / duration), MUD box in the body, swept-level dashed ticks
  with two-line tags (**NAME PRICE** on top, **time** below), day-of-week header.
- Expected: E-HOD / E-LOD tips with expected time + duration, and an E-MUD box — mirrors the
  actual candle in dim ghost ink. **These expected values are SAMPLE data in the CSV** until the
  model feed (Phase 4b) writes real ones.
- Background panel (dark, bordered) so chart price labels don't bleed through.
- Settings (17 params, all explicit `pc++`, NO setLabelParameter): Side, Layout (Pair/Overlay),
  Candle width, Pair spacing, Margin gap, Background fill, show Expected/Actual, Up/Down colour,
  HOD/LOD tips, MUD box, Swept levels, Day header, E-HOD/E-LOD lines, Font, EXP/ACT tags.

## Version history
- v0.1 candle pair · v0.2 finished actual candle (HOD/LOD tips, MUD box, swept ticks) ·
  v0.3 background panel + adjustable spacing + E-HOD/E-LOD/E-C · v0.4 day header, E-lines toggle,
  swept-tag price · v0.5 two-line swept tags · **v0.6 expected candle time/duration/MUD**.

## CSV schema (day-model rows; gamma plugin ignores them)
```
DAYEXP,o,h,l,c            DAYACT,o,h,l,c
DAYHOD,value,time,dur     DAYLOD,value,time,dur     DAYMUD,pts,time,dollar
DAYEHOD,value,time,dur    DAYELOD,value,time,dur    DAYEMUD,pts,time,dollar   (expected — SAMPLE)
SWEPT,name,price,time,R|B|T        WEEKDAY,Mon..Fri
```

## What's NEXT
1. **Statistics strip** (design §10.2) — top EXP/ACT rows (BOP · Wick · W.End · Wick% · MUD ·
   MUD-t · HL Gap · HL Rng, dynamic first-extreme, MUD-t = HL Gap − BOP). ACT row + layout
   buildable now; EXP row needs the model feed.
2. **Phase 4b model feed** — the nightly base-rate fit exports real expected values (times,
   durations, MUD) into the CSV; the v0.6 expected rendering then shows real numbers, not samples.
3. **Phase 0 transport** — wire the panel to WRITE the CSV (per-strike gamma + levels + day-model),
   replacing the hand-made fixture.

## Companion change, same session
`lsGammaProfile` v0.37 (display version bumped, **parameter version unchanged at 5** so existing
instances keep their settings): new DEFAULTS — Title header OFF, EM H/L OFF, Panel at = Bottom-C.
Defaults only; apply to the operator's existing chart by toggling the 3 manually or re-adding.

## Known / deferred
- The candle is PRICE-ALIGNED, so scrolling/zooming the price axis moves it off screen (by design).
- Device-bridge commit FLUSH LAG: after every `device_commit_files`, re-stage and verify BY CONTENT
  (bytes can read stale); `force:true` + a short wait, then confirm the version string / a marker.
