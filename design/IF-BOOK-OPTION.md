# The InsiderFinance book as a gamma-profile source — investigation, impact, plan

_2026-09-16 14:45 CT. Operator: "if I wanted to replace Skylit with InsiderFinance, would it be possible?" → "investigate
it properly and do a thorough impact analysis of using IF option. review the results. document them, create a step by
step plan of providing the option of using IF vs Skylit. and then build."_

## 1. What each source actually is

| | Skylit (Atlas tape, SPXW) | InsiderFinance (chain, 0DTE window) |
|---|---|---|
| what the number is | dealer gamma per strike from **live positioning** (Skylit's own model of the flow) | dealer gamma per strike = `gamma × OI × 100 × spot² × 0.01`, calls +, puts − (verified to the decimal against their header) |
| refresh | with the tape, every few seconds; the panel reads the DOM every export | OI once a day (exchange); gamma re-priced as spot/IV move; **their payload lags** — 30 min behind on FOMC 2026-09-16 (`payloadT` 13:54 vs fetched 14:23) |
| units on the rail | %King (the tape's own normalisation, King = 100) | $M per strike; the panel normalises to %King (King = the largest \|net\|) |
| sign | polarity (+γ pins / −γ accelerates) | net = call + put; positive = dealers long gamma (the same reading) |
| the King | the largest \|node\| **on the tape** — rolls intraday with flow (4 rolls today) | the largest \|net\| in the 0DTE chain — moves only as gamma re-prices |
| velocity / lifecycle / growth | yes (the doctrine's "is it FRESH? is it GROWING?") | none — OI is static intraday |
| Trinity (SPY / QQQ / SPXW agreement) | yes | no (SPX and QQQ chains only, separately) |
| flip / walls / expected move | not from the tape (our FLIP/CW/PW rows already come from IF) | yes — already the source of FLIP / CW / PW |
| where it reaches the panel | the Atlas DOM (`tapeMapLive`) | the companion (`gex-if-levels.user.js`) → `ifChain('SPX').dte0.lv.gexProf` — **already stored, per strike, trimmed at 1% of the max gross, coverage ~97%** |

## 2. What the two books say about the same day (measured)

IF 0DTE profile from the operator's console paste (10:20 CT) against Skylit's tape (fixture A, 09:47 CT; the nearest
stored tape). %King on each book's own King.

```
strike  IF%  SKY%
 7685    39   100     Skylit King
 7675   100    31     IF King
 7660    37    63
 7650    37   -18     sign differs
 7640    43     4
 7625   -88    -2     IF's put wall; nothing on the tape
 7620    41     8
 7615    16   -28     sign differs
 7610     1   -49
 7605     5   -54     the tape's barney stack 7600-7615 is absent from IF
 7600   -42   -40     agree
 7575   -33     3
 7550   -29     6
 7525   -29     7
 7500   -31    68     sign differs — the tape's +68 floor is a -31 put strike on IF
```
Top-10 strike overlap: 4 of 10. Among strikes prominent (≥20%) on BOTH books: 4 agree in sign, 2 disagree.
Gross: IF 0DTE call $14.0B / put −$13.4B; the tape is unit-less. (A same-minute comparison is the first thing the
build below will produce; this one is 33 minutes apart and the point stands regardless.)

**Reading.** They are different instruments. IF is the standing book (what is *open*); Skylit is the flow (what is
being *done*). On 2026-09-16 the tape's whole story — the King at 7685 then 7610 then 7500, the −γ stack under price,
the +68 floor at 7500 — is invisible on IF, and IF's put wall at 7625 (−88) is invisible on the tape. Neither is
"wrong"; they answer different questions.

## 3. Impact of REPLACING Skylit with IF

Everything in the pipeline that depends on flow stops working or changes meaning:

- **The King and its rolls** — `KINGTRACK`/`KINGNOW`, lsKingTracker, the "rolling floors/ceilings" count (velocity
  input to the regime type), the King journey on the Dashboard: all tape-derived. IF's King moves slowly and
  differently (7675 vs 7685 at 10:20; the tape rolled to 7610 and 7500 in the afternoon, IF did not).
- **The doctrine detectors** — pika/barney stacks (S6), rugs, gatekeeper, air pockets: defined on the Heatseeker
  tape (`learn/heatseeker-patterns`, FINDINGS S6 measured on Skylit's ladder). They *run* on any %King ladder, but their
  thresholds and their measured rates are Skylit's. On IF's smoother OI profile they would fire on different shapes.
- **The regime TYPE** — the structure read is source-agnostic, but its velocity term (King rolls) is tape-only.
- **The learning record** — every snapshot, deflection, tap and rate in `data/` and FINDINGS is on the tape. A switch
  starts the corpus at zero.
- **Trinity** — gone.
- **Latency** — IF's payload was 30 minutes behind on the fastest hour of the day; the tape was not.
- **The panel's host** — the panel is a userscript on `app.skylit.ai/atlas*`; without Skylit it has no page. The
  companion is a second userscript on the same page. Replacing Skylit means re-hosting both.
- **Unchanged** — FLIP / CW / PW / EM (already IF), the day model (Yahoo bars), lsDayStats, the SPX→ES mapping.

**Verdict: replacement is not a like-for-like swap; it is a different product with a smaller doctrine.** Not
recommended.

## 4. Impact of ADDING IF as a selectable second book — recommended

- **Panel**: one more builder writing `GammaProfile-IF.csv` beside `GammaProfile.csv` on every export, from
  `dte0.lv.gexProf` (already stored). Same row grammar, same ES mapping, same FLIP/CW/PW, a `BOOK,IF0DTE` row, its own
  `REGIME` type read on IF's structure (sign is the same flip). Pattern tags from the same `gridSetups()` — labelled
  as IF's, with the caveat in §3 stated in the doc, not hidden.
- **Plugin**: `Book` list gains `IF` → reads `GammaProfile-IF.csv`; the header names the book; nothing else changes
  (the CSV contract is identical, so roles/pockets/brackets/callouts/regime all work). A second lsGammaProfile
  instance on the same chart with Book=IF and Side=Left gives the two books side by side.
- **KingTracker / DayModel / DayStats**: untouched (they read `GammaProfile.csv`).
- **Regression**: Gate A gains an IF-builder fixture (the 10:20 paste); the live runner gains `--book IF` and
  derives the expected rows from the audit's `ifc.dte0.gexProf`; the eyes-on checklist gets one line ("Book: IF
  header present").
- **Risk**: low — additive, no existing row changes, the Skylit path is untouched; the IF file is absent when the chain
  is stale (same rule as the FLIP row: absent, never wrong).

## 5. The plan (step by step) — and the build status

1. ✅ Investigate the data the companion already carries (`gexProf`), its units, trim and coverage — §1.
2. ✅ Measure the two books against each other on today's data — §2.
3. ✅ Write the impact analysis for REPLACE vs ADD — §3–4; recommendation: ADD.
4. ✅ Panel v16.30: `gammaProfileBuildIF()` + write `GammaProfile-IF.csv` each export; audit carries `ifProf` (the
   normalised IF ladder) so the runner can check it.
5. ✅ lsGammaProfile 0.47: `Book: Auto;SPX;SPY;IF` → `GammaProfile-IF.csv`; header "IF 0DTE"; nothing else.
6. ✅ `test_gammaprofile_build.js` §9: the IF builder on the 10:20 fixture (King 7675, 7625 −88, sign convention, ES
   mapping, BOOK row, absent when stale). `tools/gp-regress.py --book IF`.
7. ⬜ Operator: Tampermonkey 16.30 · `compile-gammaprofile.bat` · add a second lsGammaProfile to the chart with
   Book = IF, Side = Left (or switch the existing one to IF to compare).
8. ⬜ First same-minute run of both books (`gp-regress.py --book IF` + the Skylit run) → `RESULTS.md`.
9. ⬜ Decide, with numbers, what IF's book is FOR on the rail (walls/put-wall confirmation? the standing book behind
   a flow node?) — a doctrine question, one element at a time.

## 6. Open questions (not decided here)
- Should IF's book carry pattern tags at all? They are Skylit-doctrine shapes on a non-Skylit ladder. Built ON, easy
  to turn off (`Node name inside` per instance).
- The IF payload lag (item E in LOCKED-ITEMS): carry `payloadT` age on the FLIP row and on the IF book's header.
- Window: 0DTE only for now (matches FLIP/CW/PW). A `toFri` book is one more filter if ever wanted.
