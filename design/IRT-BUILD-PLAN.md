# IRT INDICATOR — PHASED BUILD PLAN

**Status:** BUILDING, updated 2026-09-13. The design is in `design/GAMMA-PROFILE.md`.
**➡ For live plugin status/architecture/gotchas, read `plugin/GAMMA-PROFILE-PLUGIN.md` first.**

## BUILT SO FAR (2026-09-13)
- **Phases 1–3 BUILT and rendering** on the EPU26 3-min chart: the RTX skeleton + price alignment,
  the gamma histogram, and the level rail — all live as `lsGammaProfile.dll`.
- **Real data proven:** the profile is driven by the real SPX gamma book from the Skylit tape
  (King SPX 7675), not dummy data. Data flows via a hand-built `plugin/GammaProfile.csv` for now.
- **Tape-matched color** (diverging teal→gold/magenta heatmap), and the full label design:
  % outside · node type inside · rank bubble inside (centered on the strike).
- **Full 34-control settings panel BUILT** (Phase 8 brought forward): width, side, detach, node
  filter/threshold, color pickers, all label toggles, the line rail toggles, header/spot. Params are
  read via the parameter callbacks (parmsLoad/Apply/Updt) and cached — see the plugin doc gotcha #3.
- **Build tooling:** `plugin/compile.bat` (double-click, no admin) + `plugin/build.bat`.

## NEXT (in order)
1. **Phase 0** — wire the panel to write `GammaProfile.csv` live with the real SPX→ES conversion
   (fixes alignment + makes it self-updating). Spec: `design/spec-phase0-gamma-export.md`.
2. Visual tuning vs the mockup; decide FlexLevels-lines retirement + NQ coverage + secondary King.
3. Phase 6 patterns (doctrine-gated); Phase 7 delta (needs footprint feed).

---

## Guiding constraints (true for every phase)

- **RTX C++ plugin.** It is the only Investor/RT path that draws a true histogram at arbitrary price levels. RTL
  indicators are time-indexed; FlexLevels draws lines only; the native Volume Profile is single-color/volume-only.
  SDK installed at `C:\Program Files\LinnSoft\InvestorRT\sdk`.
- **Reads IRT's price axis.** As an RTX indicator every Y comes from the chart axis — no separate calibration.
- **Detached margins** (VolumeScope "Detach Last X Bars" model): profiles live in a reserved RIGHT margin, the model
  candles in a reserved LEFT margin, at a set pixel width; the price line never overlaps them.
- **The panel is `@grant none`** and cannot write files. The per-session data file rides the same transport as the
  FlexLevels export today (companion / IRT-export courier). Confirm before Phase 0 ships.
- **Match Skylit** (his standing rule): what we draw mirrors Skylit for the same setting; a deliberate difference is
  named, never silent.
- **Doctrine gate** (project rule): before any detector/pattern, name the Skylit article, quote the rule, and state
  FOLLOWING / DEVIATING (with data) / NO-DOCTRINE. Patterns are only *surfaced* here — detected + verified upstream.
- **Ship ceremony each phase:** one self-contained installer `.bat` (no PowerShell), snapshot + resume note +
  changelog in the same commit, probe the live panel/plugin after install.

---

## Phase 0 — DATA EXPORT & SCHEMA  ·  ✅ DONE (panel v16.00/16.01)
The panel writes `GammaProfile.csv` (compact CSV, NOT the proposed JSON — the plugins parse CSV) into the
`lsFlexLevels` folder, in the current futures symbol's ES price space, with front-month auto-roll. STRIKE/
KING/SPOT/BOOK + DAYACT/DAYHOD/DAYLOD/DAYMUD + DAYEXP/DAYEHOD/DAYELOD/DAYEMUD (from HODLOD_BASE) + SWEPT +
WEEKDAY. `gammaProfileBuild()` in the userscript. Remaining for the stats strip: add BOP/Wick/W.End/Wick%/
HLGap stat fields (ACT from measureBars, EXP from hodlodBaseFor). Original sketch (kept for reference):
- **per-strike gamma** (whole book, 5-pt): `strike, pctKing, signedGamma, rank, isKing`.
- **levels:** `callWall, putWall, flip, emHigh, emLow, em0dte, spxKing, spyKing`.
- **day-model:** EXP + ACT for every field, with `MUD_t = HL_Gap − BOP` computed, dynamic first-extreme flagged.
- **patterns:** `{pattern, strike}` for any pattern detected upstream.
Deliverable: a validated file on disk, schema pinned by a test. Confirm the transport (which userscript writes it).
Depends on: nothing. The panel already holds gamma, IF levels, and the 0DTE EM.

## Phase 1 — RTX SKELETON + PRICE ALIGNMENT  ·  *buildable now*
Minimal plugin: read the file, map price→Y off the IRT axis, draw one horizontal line at the King with its label
**above** the line. Proves the SDK build/install, file read, coordinate mapping, refresh cadence, label placement.
Deliverable: an aligned King line.  Depends on: Phase 0.

## Phase 2 — GAMMA PROFILE (the core)  ·  *buildable now*
5-pt strike histogram in the reserved right column, facing price. King=POC; +γ gold / −γ purple; node filter
(Top 3 / Top 5 / ≥N% default 20 / All-greyed); rank 1–5 inside-left, %King inside-right; value-area shading.
Deliverable: the filterable gamma profile.  Depends on: Phase 1.

## Phase 3 — REFERENCE LEVELS  ·  *buildable now*
Horizontal lines, labels ABOVE the line, each toggleable: IF Call Wall / Put Wall / Flip; EM High / EM Low (0DTE);
model E-HOD / E-LOD; King POC. Extend-King / Extend-HGN toggles; solid by default; label position L/C/R.
Deliverable: the level rail.  Depends on: Phase 1 (0DTE EM already computed by the companion).

## Phase 4 — MODEL CANDLES (reserved left margin)  ·  *buildable now (EXP needs 4b)*
Expected (ghost, green/red by expected close vs open) + Actual (developing), price-aligned. Split wicks; body wide
enough for the MUD box; HOD/LOD written on the wick tips (word+value / time / duration); MUD box in body
(move / MUD-time / dollar); swept-level tags to the RIGHT. Deliverable: the day-candle pair.
Depends on: Phase 1; EXP values from Phase 4b.

## Phase 4b — EXPECTED-CANDLE MODEL WIRING  ·  *model validated this session; parallel track*
Nightly fit (his machine) → coefficients/table exported → EXP values rendered. Fold in the **self-improvement loop**
(feature audit → adopt/weight/drop, leakage-gated, frozen-baseline guardrail) as part of the nightly refit.
Parallel: **EM capture** — latch the opening 0DTE EM into the day-file so the near-dated EM can be tested vs the
model in a few weeks (scoped: panel-only, surface `dte0.em` from `gpts_if_chain_v1` into the recorded snapshot).

## Phase 5 — DAY MODEL STRIP  ·  *needs 4b for EXP*
Top strip: weekday · EXP/ACT rows · field order `1ST · [1st] · Took · BOP · Wick · W.End · Wick% · MUD · MUD t ·
[2nd] · HL Gap · HL Rng` (dynamic first-extreme; MUD t = HL Gap − BOP; 12h times, Xh Ym durations). Trade fields
(Rly…Time) stay OUT until defined. Deliverable: the strip.  Depends on: Phase 4b.

## Phase 6 — PATTERN LABELS  ·  *doctrine-gated*
Pika / Barney / Gatekeeper / Rug / Rrug inside the node bars. Only surface a label for a pattern DETECTED + verified
upstream; build/verify any missing detector first (name the Academy rule, FOLLOWING/DEVIATING/NO-DOCTRINE).
Deliverable: pattern tags.  Depends on: Phase 2 + the detectors.

## Phase 7 — DELTA PROFILE  ·  *blocked on the footprint feed*
Second reserved right column, RIGHT of gamma. Per-price delta: 15m solid + ghost 15/30/60m; buy/sell colors;
growth-brightening (light bright green) + the 5-min growth amount (`▲+X`), high-contrast font; top-5-delta mode.
Depends on: the per-price footprint feed (RTX SDK) — not flowing yet.

## Phase 8 — CONTROLS / SETTINGS PANEL  ·  *incremental across phases*
Width in px per profile; placement (Mirror L+R / Stack right / Stack left); profile selection (SPX γ / SPY γ /
QQQ γ / Delta); both Kings always shown; label position; solid lines default; a toggle per layer (levels, patterns,
delta, candles, day-model). Build the relevant control as each feature lands, then a consolidated dialog.

---

## Sequencing summary

**Buildable now (no external blocker):** 0 → 1 → 2 → 3 → 4 (+4b) → 5, with 8 growing alongside.
**Blocked:** 6 (needs verified detectors), 7 (needs the footprint feed).
**Recommended first cut:** Phase 0 → 1 → 2 → 3 — a live, axis-aligned gamma profile + level rail on his chart. That
is the highest-value, lowest-risk slice and it stands on data the panel already has.
