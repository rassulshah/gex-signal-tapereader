# LOCKED ITEMS — specified, agreed, NOT YET BUILT

**⚠ THIS FILE EXISTS BECAUSE THE RESUME NOTE LOST ITEM 18 AND COST THE OPERATOR A WHOLE SESSION.**

_Created 2026-08-27._

---

## ✅ BUILT 2026-09-19 (GP 0.77, "ok build") — the dialog regroup (design/gp-dialog-regroup-mockup.html)
Seven sections (Book & layout · Nodes · Colours · King line · Top nodes · IF levels · Read panel), shorter names, the three cut-off
labels fixed. Needs setParameterVersion(8) — the saved instance opens once on defaults, so the defaults become HIS settings from
his 09-19 dialog screenshot (Book Both, 90, Right, Medium, Top 5, 20, Grey out, King=100%, hide 0, font 10, rank badge on, name
inside on, Inside, Top 5, GPOC, King line on, CW/PW/Flip/EM on, extend off, King style Dot, labels Left, structure on, regime on,
Bottom-C, tape on, IF tags on, SPY width 120 (unreadable), Node bands OFF, band 3, King width 2, Bands, Dot, Fixed, Solid, Mag off).
Section-title rows (setLabelParameter) must be probed first: in 0.6x one shifted the numbering.

## ✅ BUILT 2026-09-19 (GP 0.76) — level tags ON TOP of the strip's price / %King (design/gp-tag-above-mockup.html)
Operator 09-18: "I want to move the position of the labels on top of the price and %king rail"; 09-19 (screenshot, WK / PW /
FLIP 0DTE still beside the bar tips): "didn't i explain that the labels should be on top of the rails". The mockup's three open
questions (above price+% or % only; keep role letters G/C/F/B; italic or small caps) were never answered and the item was
dropped from the builds — my miss. ANSWERED 2026-09-19: above price + % (left edge on the strike, 1 px above); IF levels + K only (G/C/F/B stay in the bars); plain bold, font-1. FLIP tag above its tick inside the strip; pill after the tag; K·CW when shared; SPY strip gets K only; fallback to beside-the-tip when rows are too tight. Mockup: design/gp-tags-on-strip-mockup.html. Built as GP 0.76. Also queued: the dialog regroup (design/gp-dialog-regroup-mockup.html).

## ✅ BUILT 2026-09-19 — GP 0.75: separate King / top-node / IF level line styles; top nodes as Bands or Lines (width Fixed or By %King); IF "None (labels only)"; Mag line
Standing requirement (his words): "I should have the ability to also only have labels without any lines" — any future line
family must keep a labels-only choice. CHANGELOG has the detail.

## ✅ BUILT 2026-09-19 (was parked) — GP 0.73 (tape spacing N = 2, labels centred on their price, the SPY strip's ES price in dim ink, per-strip widths) + DS 0.16 (Corner read on every draw, MUD = $ (points))
Applied from pending/ on his "build"; the patch retired in the same commit. CHANGELOG has the detail.

## 📌 OFFERED 2026-09-17, NOT YET AGREED — the E row's wick family (BOP · WICK · W.END · WICK% · MUD)
The strip's E cells for the wick family are weekday MEANS (E.wick.bop 20.8 vs the median 6) and do not compose (TOOK + BOP
≠ WICK) while the A row's identities are exact; MUD and MUDt are two durations of one leg (spec §10.2: MUD = the move in
pts · $). Proposal on the table: medians composed like the A row (WICK = TOOK + BOP → W.END; MUDt = HL GAP − BOP), MUD =
|2ND − open| pts · $ (E from the expected candle). He has not said yes; ask, one element at a time. Leaves this file when
built or when he cancels it.

## ✅ BUILT 2026-09-17/18 — the sanity-check builds (16.47 → 16.49, GP 0.70, DS 0.13 / 0.14)
_The EM 10× (16.47) · the panel centred + both clocks after the close (DS 0.13) · no +orclock, READ1 = one model on both
halves (16.48 / DS 0.14) · one label per IF level (GP 0.70) · one deflection per swing in the tap record (16.49). All
awaiting his eyes-on 2026-09-18 RTH._

## ✅ BUILT 2026-09-17 — TWO RAILS, THE ATLAS POOL (v16.40 / GP 0.59)

_v16.46 + DS 0.12 (22:30 CT): the second extreme's read (READ2) — BUILT, tested OOF; nightly refit + courier floors. Next: the rolling-20 calibration in the testing tab; eyes-on tomorrow._
_v16.45 + DS 0.11 (21:40 CT): the READ line's second extreme rung — BUILT; awaiting his install + tomorrow's eyes-on. The hod/lod ladder rebake is DONE (test_hodlod green)._
_DS 0.10 (21:05 CT): A-row prices from the chart's own session extremes — the 'A-row drift after close' item CLOSED. OPEN: rebake the hod/lod ladder (BASERATES.json regenerated 15:16, test_hodlod b5/b6/s2)._
_v16.44 (19:20 CT): VERIFIED live — Atlas's five == IRT's five. Pooled nodes always stamped; runner + harness fixes._
_v16.43 (18:05 CT): BUILT — the pool ranks from Atlas's merged slice (poolSrc = atlas), own-% fallback. Awaiting his update + the runner's check on the first 16.43 export._
_v16.42 (17:45 CT): AU.atlas — Atlas's own five recorded every export; OPEN: correct the pool rule from it (his "IRT top 5 ≠ Atlas top 5")._
_GP 0.65 (16:50 CT): repair-on-load — the version bump did not reset his instance (scrambled 0.64 dialog)._
_v16.41 + GP 0.64 (15:40 CT): the reviewed dialog (v7), node bands from first-seen, both Kings, MAG, chip. BUILT; awaiting his install + same-minute Atlas compare. Chip centring: was the setting (panelpos=0), now default Bottom-C._
_GP 0.63 (12:45 CT): rail clear of the price scale, tape columns both rails + left background, GPSETTINGS/GPRECT in the status file. OPEN: the chip at Bottom-L vs "centered" — read the status file after his restart._
_GP 0.62 (12:25 CT): SPY rail at the SPX thickness, bubbles outside short bars, SPY rail drawn first — awaiting his screenshot._
_GP 0.61 (11:55 CT): Book = Both — one instance, two rails; awaiting his install + same-minute screenshot vs Atlas._
_GP 0.60 (11:35 CT): 0.59's Rank row sat mid-list → both saved instances scrambled; moved to the end, parameter version 6
(defaults), per-instance `GammaProfile.status-<Book>-<Side>.txt`. His "one or the other but not both" is OPEN until the
two status files are read after his restart. Offered, not asked for: a "Top 2" entry in Show (would be appended)._
Operator: "there should be two profiles, one for SPX and another for SPY, and the top 5 should be split between them"
(after measuring Atlas's ES1 derived merge on the live feed). Built as the SPY book file + a pooled rank on every STRIKE
row + `Rank = Atlas merge` in lsGammaProfile; SPY rail left, SPX right; day candle off for now. Awaiting his install and
the same-minute eyes-on against Atlas. Agreed in the mockup discussion: a rail with no pooled badge that minute draws grey.

## ✅ BUILT 2026-09-17 — lsKingTracker Source = Skylit | IF (v16.38 / KT 0.11)
Proposed 2026-09-16 17:30 (IF's Magnet for the King tracker), lost from the lists, raised by him 2026-09-17: "the king
tracker should be able to switch back and forth between IF and skylit". Built as a switch (default Skylit). Awaiting his
install + eyes-on. The other 17:30 proposals for IF in lsDayModel are BUILT (16.33 EM + placement); lsDayStats
realized-range/EM per day is NOT built and not agreed — ask before proposing again.

## ✅ BUILT 2026-09-17 — THE REGRESSION FOR ALL FOUR INDICATORS (operator, 2026-09-16 late: "extend the regression to the other indicators … keep updating the test cases … fire it collectively or separately")
`tools/regress.py [all|gamma|daymodel|daystats|kingtracker]` / `regress.bat`; `testing/REGRESSION.md`; Gate A/B/L per
indicator; logic headers for DM/DS/KT/offset; `AU.day` on the audit (16.37). **Standing rule from here (BUILD-CHECKLIST
2b): a build that touches an indicator updates its cases in the same commit.** Open under it: (i) `regress.bat` never run
on his MSVC; (ii) the day/King fixtures are synthetic until his first real 16.37 CSV+audit pair is staged in.

## ⭐⭐ ACTIVE (2026-09-15 EVENING): THREE AGREED ITEMS FOR TOMORROW — discussed, doctrine-checked, NOT BUILT

Operator's close: **"save everything, we will do it tomorrow."** All three were discussed one at a time and agreed.
**2026-09-16 11:xx CT: A and B BUILT (v16.27 / lsGammaProfile 0.42 / lsKingTracker 0.6), awaiting his install; C still open.** Doctrine gate was run for A (FOLLOWING on every rule). Build order agreed: A + B in ONE gamma
build (one userscript update + one recompile), C only after the study.

### ✅ A · PATTERN LABELS PER DOCTRINE — BUILT 2026-09-16 (lsGammaProfile 0.42 + panel v16.27; awaiting his install + eyes-on)
#### (spec as agreed, kept for the record)
### A · PATTERN LABELS PER DOCTRINE (lsGammaProfile) — operator: "G at multiple places… implement gatekeeper,
### rug, rrug, pika, airpocket correctly"
Current plugin rule tags EVERY node ≥10% between spot and the King "G" (the flood). Correct rules, all FOLLOWING:
- **Gatekeeper** (`patternpedia/pattern-the-gatekeeper`, `core-concepts` §Gatekeeper Nodes): the SINGLE dominant
  blocker — largest node strictly between spot and the King, only if high-value (≥30% King). One G max. (The
  panel's own `gatekeeper(sym)` v10.27 already encodes exactly this — magnitude-ranked, ratio vs the next node
  beyond; the plugin just never used it.)
- **Air pocket** (`learn/air-pockets-velocity`, `core-concepts` §Air Pockets): a thin zone BETWEEN nodes, a
  pathway. Band only a thin run BOUNDED by a significant node on both sides — never the outer far-OTM tail (that
  was the "top area shaded"). Colour by polarity (−γ pocket = violent).
- **Rug / Reverse Rug** (`learn/heatseeker-patterns`): +γ node ABOVE spot with −γ BELOW spot, spot below the +γ
  node → tag that +γ node R; exact mirror → RR. (Panel's rugDetect already requires price's side, v15.64.)
- **Pika / Barney** (`learn/heatseeker-patterns` + FINDINGS S6, measured): dense cluster of +γ (pika) / −γ
  (barney) nodes — member ≥30% of King, a thinner node breaks the run, biggest ≥40%, ≥2 members → tag P / B.
- **Rule zero** (`heatseeker-patterns`): "Magnitude overrides pattern" — thin nodes get NO tag.
All computable inside the plugin (it has spot=chartClose, King, every node's sign/size/rank + the new SPX strike).

### ✅ B · REGIME ROW + FLIP/CW/PW ON THE NODES — BUILT 2026-09-16 (panel v16.27 `gpRegime()` + rows; lsGammaProfile 0.42 displays; lines AND labels as options)
#### (spec as agreed, kept for the record; the dropdown finding and the reconciliation are in the AMENDED block below)
### B · REGIME FROM THE PANEL, NOT THE PLUGIN'S SUM (+ the missing FLIP/CW/PW rows) — operator asked "how are
### you calculating the regime" then "is it better from InsiderFinance" then "what is your recommendation" → "ok"
Plugin today: `net = Σ pct over all 100 nodes; <0 → FOLLOW else FADE` (GammaProfile.cpp L776). Defects: distance-
blind (a far-OTM node tips it), two regimes only (a WHIPSAW day reads "follow"), ignores velocity, doesn't use the
flip. Settled split (DECISIONS v11.77, INSIDERFINANCE.md L156-164): **IF prices the day** (FLIP = IF's Zero Gamma,
band, EM, walls); **Skylit reads the character** (regime chip = gamma+vanna −G/−V, velocity) — moving the chip to IF
was considered and REJECTED (IF carries no vega). Recommendation agreed:
1. Panel writes `REGIME,<sign>,<type>,<flip>,<velocity>,<conflict>`: sign = price vs IF Zero Gamma; type = the
   doctrine's THREE (`learn/gamma-regimes`: Range→fade / Trend→follow / Whipsaw→fade extremes only or sit out)
   from the panel's existing structure classifier (Rainbow Road/Mixed/skew, ~L10620) + velocity (King growing?
   floors rolling?); conflict flag when Skylit's gamma sign ≠ IF flip (surface, don't resolve — DECISIONS L98).
2. Plugin DISPLAYS that row instead of computing. Same words as the Dashboard.
3. Same pass: panel writes `FLIP` (IF Zero Gamma), `CW`, `PW` rows — the plugin already draws them (found missing
   2026-09-15). ⚠ Window landmine: IF's zero-gamma flips sign by window (dte0 −$6.86B / toFri −$16.41B / all
   +$13.34B, 2026-08-23, all correct) — pin the window, never compare across.

#### B — AMENDED 2026-09-16 ~10:15 CT (operator, live, with a screenshot of IF's GEX page)
- ⚠ **IF's header numbers FOLLOW THE EXPIRY DROPDOWN.** With "Today (0DTE)" selected the header reads Net GEX −$1.5B ·
  Call Wall 7675 · Put Wall 7600 · **Zero Gamma 7617.03** (spot 7612.44). INSIDERFINANCE.md §4 says the header is
  all-expiry — that was only the DEFAULT dropdown state at capture. The companion scrapes whatever window the dropdown is
  on, and the panel tags the walls `wallsAreAllExpiry` regardless. Fix INSIDERFINANCE.md §4 in the same build.
- Operator: "I had to select 0 gamma, it was all expirations by default, so make sure you select 0 DTE (Today) so you
  implement it right, then you will have the walls and the zero level too." → the COMPANION must ensure the dropdown is
  on **Today (0DTE)** (check the label at every poll; select it if not; wait for the header to re-render; then scrape) and
  tag every header value with the window it was read under. Regime sign = spot vs THEIR 0DTE Zero Gamma (+ buffer); our
  `gammaFlip()` on the 0DTE chain is the backstop and must be reconciled against their number once (like net GEX was).
- Operator: "since I have a lot of levels, I want the call wall, put wall, zero gamma (flip) called out ON THE NODES
  THEMSELVES in the bar/histogram." → lsGammaProfile tags the node whose SPX strike = Call Wall "CW", = Put Wall "PW",
  and marks the Zero Gamma price on the rail as the flip (it is a price, not a strike — a tick + label at 7617.03, not a
  node tag). Panel writes `FLIP` / `CW` / `PW` rows with the window (ES scale via esOfSpx, raw SPX strike beside).
  ANSWERED (operator, 10:40): "keep them as options, so you can have an option of lines or labels" → existing line
  toggles untouched + one new bool "Level labels on nodes (CW / PW / FLIP)" appended LAST. Built without a mockup on
  his "build" — first eyes-on is the acceptance test.
- ⚠ THE DROPDOWN CANNOT BE SELECTED BY US: the companion fetches the page from Atlas (server default = all-expiry).
  0DTE flip/walls = our filter over their chain (`dte0.gf.flip`, `dte0.lv`), RECONCILED against his 0DTE screenshot
  2026-09-16 (walls exact, flip 7620.7 vs 7617.0 minutes apart). INSIDERFINANCE.md §4 updated.

### ✅ D · THE GAMMA-PROFILE REGRESSION SUITE — BUILT 2026-09-16 (v16.29 / lsGammaProfile 0.45); FIRST LIVE RUN PENDING
Operator: "a thorough regression build for the gamma profile … Skylit tape and Atlas chart match IRT … datapoints from
Atlas as well as screenshots … documented to keep a trail … results and notes documented." Built: Gate A test, Gate B
logic test + header, the live runner with the audit sidecar, REGRESSION.md / RESULTS.md / CHECKLIST.md / runs/.
**Open inside D:** (1) the first live run with both screenshots after he installs 16.29 + 0.45 (needs computer-use
approval on the desktop); (2) `run-logic-tests.bat` has not yet been run on his MSVC; (3) upgrade the scheduled T1–T5
checkpoints to run `gp-regress.py` from the audit (no screens) instead of `check-gammaprofile.py` alone; (4) a fixture
for a Trend day and an air-pocket day once one is recorded (fixture A has neither).

### ✅ F · THE IF BOOK OPTION — BUILT 2026-09-16 15:05 (v16.30 / lsGammaProfile 0.47); awaiting install + the first
### side-by-side run
Operator: replace Skylit with IF? → investigate, impact analysis, document, plan, build. Analysis + plan:
`design/IF-BOOK-OPTION.md` (verdict: ADD, don't replace). Built: `GammaProfile-IF.csv` from the companion's 0DTE chain,
`Book: IF` in the plugin, test §9, runner support. **Open inside F:** (1) his install (Tampermonkey 16.30, compile-
gammaprofile.bat, a second lsGammaProfile instance with Book=IF / Side=Left); (2) the first same-minute run of both
books → RESULTS.md; (3) the doctrine question — what IF's book is FOR on the rail (design doc §5 step 9) — one element,
discuss before any further build; (4) whether IF's book should carry pattern tags at all (default ON; per-instance off).

### E · IF PAYLOAD AGE ON THE FLIP ROW — proposed 2026-09-16 14:35 (run 2), NOT AGREED YET
Run 2 (FOMC, ES −100 pts/hour): the companion fetched IF 3 min earlier, but IF's own `payloadT` was 13:54 CT — their
numbers were ~30 min behind the tape. FLIP 7690.79 / sign NEG stood against a spot of 7510; `!CONFLICT` fired as designed.
Proposal: FLIP row carries the payload age (`FLIP,<es>,<spx>,0DTE,calc,<payloadAgeMin>`); the plugin greys the sign
when their payload is older than ~10 min and the regime line says "flip <n>m old". One element; discuss before build.

### ✅ C · THE DAY CANDLE MODEL — BUILT 2026-09-16 19:15 (panel v16.33): the IF 0DTE straddle in the range at every stage +
### placement by the opening range (points 1 + 2 of the diagnosis below, measured in design/EM-RANGE-STUDY.md); point 3
### (regime skew) now RECORDABLE — the daily record row ships in the same build; test at ~60 sessions
### C · THE DAY CANDLE MODEL — operator: "why is the day candle model so bad, look at the results today"
Diagnosed with today's numbers (Tue 15 Sep, Dec scale, from the open): expected HOD +31 / LOD −31 / close −15.5 /
range 62 (open60 stage) vs actual HOD **+2.8** (8:33a) / LOD −40.8 (9:54a) / close −27.5 / range 43.5. LOD and close
misses (10, 12) are INSIDE the documented MAE 18.5; the day was "so bad" because of ONE thing — **28 pts of upside
that never printed**, and that is STRUCTURAL, not calibration:
1. The range is placed SYMMETRICALLY ±½ about the open by construction; only the CLOSE leans (¼ range) with the
   68%-reliable opening drive. A trend day is one-sided → a centred range guarantees the miss on the wrong side.
2. Layer 2 printed `READ,HOD,…,100,IN` (HOD already printed) and Layer 1 kept drawing HOD +31 — DAY-MODEL.md's
   "do not merge them" kept the two questions apart but let the candle project an extreme the READ had ruled out.
3. Regime is not an input: today was textbook −γ + rapid accumulation + rolling floors = TREND day
   (`learn/gamma-regimes`), which should skew the range one way; the model treats every day as a range day.
Proposed (NOT agreed yet — operator said "we will do it tomorrow"): (a) let the READ CLAMP the geometry — once
HOD/LOD IN at high confidence, pin that side to the actual extreme, hand the remaining range to the other side (keep
the layers as separate QUESTIONS; let Layer 2's answer constrain Layer 1's drawing); (b) skew the range by drive /
regime (e.g. 80/20 on a trend regime) instead of only tilting the close; (c) **MEASURE FIRST** — today is n=1;
run `tools/study-hodlod.py` over the recorded sessions to see how much (a)+(b) cut HOD/LOD error before changing
the model. Recommendation on the table: run the study, then decide.
**MEASURED 2026-09-16 evening — `design/EM-RANGE-STUDY.md` (n=299 ES sessions, 2025-06 → 2026-09, out-of-fold):**
(i) the expected move (VIX1D-open proxy for the 0DTE straddle) is the best RANGE predictor at every stage — beats the
prior-day stage 18.3 vs 22.6 MAE (−19%), beats even OPEN30/OPEN60 (19.4/19.0) on its own, sharpens them 9% when
added; the prior-day range is dead beside it (coef 0.007). (ii) the expected HIGH/LOW barely move from the range
(~1 pt) — a PERFECT range placed symmetric round the open still leaves 18.4 pts/side: the error is PLACEMENT, exactly
point 1 above. (iii) placement by where the open sits in the opening range cuts the high/low error −26%/−24% at 60 min
(17.4/22.0 → 12.8/16.8). Proposal on the table (nothing built): placement first, EM into the range second, record the
pinned straddle daily to re-fit on the real series. Regime skew (point 3) still unmeasured — no regime history.

### ✅ G · THE DAY STATS E ROW CONDITIONAL ON THE MORNING — BUILT 2026-09-16 18:10 (panel v16.32); awaiting install
Operator: "how can [the day stats] model be improved?" → "I'll go with your recommendation … test and confirm that the
model is better than base." `design/DAYSTATS-COND-STUDY.md`: 1ST clock 38.4 → 33.3 min, 1ST = LOD/HOD 0.48 → 0.61,
2ND clock unchanged (not predictable — say so). OPEN under G: (i) the lsDayStats LADDER for the 2ND clock instead of a
clock — plugin change, mockup first; (ii) wick% conditional on the open's OR position — unmeasured; (iii) the daily
record row (regime sign at the open, flip distance, walls, King) so the doctrine inputs can be tested — panel change,
fields to agree.

### ✅ H · THE DAILY RECORD ROW — BUILT 2026-09-16 19:15 (panel v16.33): `dayRecord` in the day file (regime sign, flip,
walls, King, EM, event, open) taken in the first 15 min. OPEN under H: re-run study-em-range / a regime-skew study when
~60 records exist; re-fit `GP_EM_MODEL` on the real straddle pins then (VIX1D proxy until).

### ✅ I · THE IF EXTRAS (#1 wall-depth pill, #2 slope word) + THE 2ND-CLOCK LADDER — BUILT 2026-09-16 22:00 (v16.35 /
companion 1.21 / lsGammaProfile 0.50 / lsDayStats 0.7); awaiting install. OPEN under I: measure the depth thresholds
(0.70 / 0.70) and the slope threshold (0.10) once the rows have a history; the "wall at 5 pts inside the expected
extreme stops the day" hint (KEY-LEVELS-STUDY §4) could use the depth tag as a conditioning variable.

### DECIDED 2026-09-16 21:55 — ONE RAIL, SWITCHABLE, IF DEFAULT (lsGammaProfile 0.53). F step 9 ("what is IF's book FOR")
answered by use: IF is the rail he looks at; Skylit is one dropdown away. Still UNANSWERED (asked 21:50, no reply yet):
item E (IF payload-age flag), the volume / trend-day-flag study, the NQ cross-check.

### CARRIED OPEN (unchanged tonight)
- Q12 top-3-vs-top-5 gamma lines (per-rank deflection table, sessions since 09-09) — operator asked "would I have
  seen it with Top 5" → answered (yes, ranks 2/3 are in Top 5); the measured question is still open.
- Full same-moment RTH audit (T1 09:05 CT): confirm the King-roll history against Atlas's 0DTE ladder AS ROLLS
  HAPPEN (Atlas live shows only the current King; the "Rolling front + 3 · 3 Days" view is a DIFFERENT window).
- SCALEREF is ES-only: an NQ chart's QQQ/NDX King lines get no basis shift (clamp protects them; needs an NQ anchor).
- lsDayStats: still SPOT-anchored (its rows are ~Dec scale via SPY×D.scale, ~1% hot) — untouched, verify.

---

## ⭐ ACTIVE (2026-09-13): IRT GAMMA-PROFILE PLUGIN — built & working
A separate C++ RTX Investor/RT extension (`lsGammaProfile.dll`) was built this session and now
renders the real SPX gamma node profile + level rail + a 34-control settings panel on the EPU26
chart. **To continue it, read `plugin/GAMMA-PROFILE-PLUGIN.md` (authoritative) and
`session-state/2026-09-13_resume-plugin.md`.** NOT YET BUILT for the plugin: **Phase 0** (wire the
panel to write `GammaProfile.csv` live with the real SPX→ES conversion — spec
`design/spec-phase0-gamma-export.md`), visual tuning, patterns (Phase 6), delta (Phase 7). The panel
(v15.99) itself was not changed this session.

### 📋 GAMMA-PROFILE TUNING BACKLOG — operator batch, 2026-09-13 (one-at-a-time, not yet built)
His words, 2026-09-13 morning (a 14-item batch given at once; being worked ONE AT A TIME per PROJECT-CONSTANTS).
Feasibility verified against `irtsdk.h` this session. An item leaves this list only by being BUILT (version)
or CANCELLED by him.

**BUGS (dead options / wrong behaviour):**
1. **Bars pulse when the chart is zoomed/scrolled** — "when I move the candles closer the bars go to their
   intended size, when I move them back they shrink." Root cause: `render()` shrinks `w` to `room = paneR - lastX - 6`,
   and `lastX` (right edge of last candle) moves as the chart scrolls → bar length rescales. FIX: keep bar width FIXED
   at `S.width`; detach reserves a fixed right-margin strip instead of adapting to available room. **← chosen first.**
2. **"Extend top-node lines" does nothing** — `P_EXTN`/`S.extn` is read but never used in `render()`. Top-N node
   horizontal lines are simply not drawn. Must implement, and make them customizable (color/style/width).
3. **"Extend King line" does nothing** — `P_EXTK`/`S.extk` read but never used. `drawLevel` stops at `lastBar`;
   extend should run the line to the pane's right edge / future margin. Verify it works after.
4. **% doesn't re-align on bar-thickness change** — "when I change bar thickness it should auto-update the
   percentages and align them properly." Needs the exact misalignment confirmed (screenshot) in its turn.

**SETTINGS-PANEL PLUMBING (feasible, mostly no mockup needed):**
5. **SPX/SPY instrument selector** — auto-select book by chart symbol (ES has no data → default SPX); allow manual
   SPX (default) / SPY. `setListParameter("Book","Auto;SPX;SPY")` → picks which CSV to read. (Second profile = add the
   indicator twice with different Book. Needs a SPY CSV — ties to Phase 0 data pipeline.)
6. **Delta profile** — the signed-dollar delta profile we discussed. NEEDS the footprint/delta feed (Phase 7) — not
   available yet. Deferred with a note, not dropped.
7. **Rename "King" label** — `setStringParameter("King label","KING")` so he can set e.g. "GPoc". Blank = default.
8. **Add Top 8 / Top 10** to the Filter list (currently Top 3;Top 5;>=Threshold;All). Generalize the hardcoded
   rank<=3/<=5 logic to top-N. Same for Rank scope.
9. **Font size** — already exists (`P_FONT`, default 12). He wants it usable/bigger range; consider native
   `setFontParameter` (family+size+style) instead of a plain int.
10. **Section headers** in the settings dialog — feasible via `setLabelParameter("— LAYOUT —")` etc. (LAYOUT / NODES /
    COLOR / LABELS / LEVELS / CONTEXT).
11. **Horizontal (side-by-side) checkboxes** — feasible via `kParmAppendSameLine` flag on `setBoolParameter`, to stop
    King/CallWall/PutWall/Flip etc. eating vertical space.
12. **King text centered more in the node bar** — node TYPE label currently sits at the bar base (`anchor-6`); move
    to the bar's horizontal center.
13. **Customizable top-node lines + lines for OTHER kings** (e.g. SPY King) — color/style/width controls, plus a
    line at a secondary book's King. Ties to #2 line implementation and #5 book selector.

**NOT POSSIBLE natively:**
14. **Tooltips on settings controls** — IRT auto-generates the parameter dialog and exposes NO tooltip/hint API
    (verified: no tooltip/hint/help method in `irtsdk.h`). Alternatives: a one-line help `setLabelParameter` per
    section, or a Help section at the dialog bottom. Told the operator; awaiting his pick.

---

## WHY THIS FILE EXISTS — read this before you skip it

On 2026-08-27 the operator said, three separate times, that a Yahoo Finance solution already existed
and that **files in git were being updated to get the data**. Three times I said I could not find it.
I searched the repo HEAD, GitHub, Drive and the skills directory, reported "it was never pushed",
proposed rebuilding it, and found a different project's pipeline and misreported that as the answer.

It was in **this repo's own history the entire time** — `session-state/latest-resume-note.md` at
commit `72e820e`, item 18, locked 2026-08-16, with a complete specification.

**Two failures caused that, and both are now fixed in `.gex-config.json`:**

1. **THE LOAD PROCEDURE SAID `git clone --depth 1`.** A shallow clone has no history. Every resume
   note ever written was one command away and structurally invisible. **The load now clones full.**
2. **THE RESUME NOTE IS REWRITTEN IN FULL EACH BUILD.** That is deliberate and correct — but it means
   any item not re-typed is *gone*, silently, with no deletion to notice. Item 18 appears in every
   note from 2026-08-15 (`72e820e`) through v11.23 on 2026-08-20 (`04f6f80`), and in **none** after.
   Nobody cancelled it. A handoff simply did not carry it, ~24 versions ago.

**THE RULE, FROM NOW ON: an item is only allowed to leave this file by being BUILT or by being
EXPLICITLY CANCELLED BY THE OPERATOR, with the date and the reason written in.** Dropping one by
omission is the failure this file prevents. `BUILD-CHECKLIST.md` must check it every build.

This is the same disease that ate `skylit-docs/FINDINGS.md`, `BUILD-PLAN.md`,
`garma/V2-PHASE-PLAN.md`, the "Mockup hodlod v2" HTML and the "Hodlod stats2" JSON — all referenced
by name in live code comments or config, all absent. **Work that exists only in a resume note is work
that is one handoff away from never having happened.**

---

## BUILT 2026-08-28 — moved here rather than deleted, so nobody wonders if they were lost

### ✅ AHI · ALO · LHI · LLO — the Asia and London highs and lows as tracked levels — AGREED 2026-09-08, **BUILT v15.88 (2026-09-09)**
**His words (2026-09-08, evening):** *"i was not tracking sweeps of the asia and london highs and low, but we should do
it. they can be ALO, AHI, LLO, LHI. you will find that the liquidity levels and gamma levels may increase the probability
of deflection, so you must look at both and track both."*
**What is agreed:** four new key levels beside PDH/PDL/ONH/ONL/POC/VAH/VAL/WH/WL — the Asia session's high and low and
the London session's high and low — on the SWEPT line, the day candle's price axis, the sweep study
(`tools/study-sweeps.py` level types) and the tap record's `liq` column. Proposed session boundaries, awaiting his
word: **Asia 17:00–02:00 CT (the Globex open to the European open), London 02:00–08:30 CT** — ONH/ONL remain the
whole night 17:00–08:30. Measured 2026-09-08 from the ES courier's rows: AHI 7725.75 (23:46) · ALO 7694.75 (01:20) ·
LHI 7718.25 (05:20) · LLO 7687.50 (03:02) = ONL.
**Built (v15.88):** his word on the hours, 2026-09-09: *"just use whatever is standard"* — **Asia 17:00–02:00 CT, London 02:00–08:30
CT**. `sessionHL` in the panel (tier 1 by his names; v15.57's LDNH / LDNL renamed, not duplicated), the four on the candle's
labels, the SWEPT line, the sweeps read and the Analysis H2 table; `study-sweeps.py` carries the Asia window (ALO / AHI n=123 /
141 on the 284-session corpus — within chance of the fresh-low control, like every level name); the NQ night rides companion
v1.19 so the NQ chart draws them too. The tap record's `liq` column (v15.94, next — the 🗄 Data tab took v15.89, the hardening v15.90, the D rows v15.91, Kings + walls v15.92 and the RTH open v15.93 on his word, 2026-09-09) reads them by name.
**Leaves this file:** when the four levels draw on the SWEPT line and the candle (a version number here), or when he
cancels them by name.


### ⓪a HOD/LOD — BUILT v14.57, and the whole feature completed v14.60–v14.66
The design (`mockuphodlodv2.html`) was never lost. Shipped: the stats table, the wick family with the
operator's own definitions, trimmed-mean E row, and the LOD/HOD probability table with a three-state
verdict. Evidence in `skylit-docs/FINDINGS.md` F-1 … F-8.

### The wick family — DEFINED BY THE OPERATOR 2026-08-28, BUILT v14.60
BOP/WICK/W.END/WICK%/MUD had been printed as PENDING since v14.57 for want of a definition. He gave
them; they were confirmed bar-by-bar on the tape. **No longer pending.**

---

## OPEN — LOCKED, NOT BUILT

### ⚠ CONDITION THE FAR SIDE ON THE GAMMA BOOK — DESIGN BUILT, COLLECTION STARTED v14.72, TEST BLOCKED
**Updated 2026-08-28.** The question is unchanged and it is still the highest-value unbuilt idea.
What changed is that it is no longer an intention:
- **The experiment exists and is validated** — `tools/study-atrlevels.py`: train a distance-only
  touch model, then test whether a CLASS of level beats its own distance-matched expectation. Run
  on prior-day and overnight levels it returned **nothing** (F-16), which is the control working.
- ⚠⚠ **AND IT NEARLY RETURNED A PHANTOM.** With a sparse distance control the same script said
  prior-day levels were worth **+12 points**. Dense control: zero. **Any run of this test must span
  the distance range densely.**
- **The data is being collected from v14.72** — the `farside` feature records each level's `kind`,
  `pct` (%King), `sgn` (polarity) and `role` every bar.
- **Still blocked on ~40 clean sessions**, which is blocked on the storage fix (F-10).
- The nightly brief carries it as ⭐ item 3 with the method and the warning.

### ⚠ THE OLD FRAMING, kept because the answer changed: "condition the LOD/HOD TABLE on gamma"
**Raised and agreed 2026-08-28.** The table knows nothing about the panel it lives in. Does price
sitting on the King, or a low printing at a put wall, change the probability that the extreme holds?
**This is what would make the feature belong to THIS tool** rather than being generic price structure
any charting package could compute.
⚠⚠ **BLOCKED by the feature-record collapse** (below) — there is no usable recorded history pairing
node state with session extremes. Unblocking that is the prerequisite, not a side quest.

### ✅ STORAGE QUOTA — THE ROOT CAUSE, FOUND 2026-08-28. **FIX BUILT v14.68/v14.76.**
**⚠⚠ THIS ENTRY SAID "FIX NOT BUILT" UNTIL 2026-08-31 AND THAT WAS FALSE.** `lsPut`, `LS_CAP_KB`,
`LS_BUDGET_KB`, `LS_HEALTH`, `lsTotalKB` and `__gptsDebug.storage` are all in the shipped panel, and
`test_storage.js` is a live green test. What kept the entry looking open is
`session-state/pending/v14.68-bounded-writes.patch` — a parked patch against a **v14.67** base that
no longer applies, because the work it describes landed 42 versions ago. **A parked patch is not
evidence that its work is unbuilt.** Kept, not deleted, with this note on it.

**WHAT IS ACTUALLY OPEN IS A DIFFERENT PROBLEM — see the entry below.**
**localStorage was FULL at exactly 10 MB** (`gpts_recorder_v7` 5,957 KB for ONE day +
`gpts_nodeevents_v1` 3,228 KB). Every `setItem` in the system was throwing behind `catch(e){}`.
**This IS the feature-record collapse** — and the corpus tap, the base-rate courier and the 8.5-hour
stale IF chain. One fault, five symptoms. See FINDINGS **F-10**.

Cleared by hand on the live panel 2026-08-28 (9.5 MB freed; 08-27 was already exported to the repo
with MORE snapshots than storage held, so nothing was lost). **Everything came alive instantly**:
ES/NQ/GC/CL 1,905–1,916 bars each, base rates delivered, IF chain live.

All five parts of the original spec SHIPPED: bytes not counts (`lsPut` against `LS_BUDGET_KB`),
shedding within today oldest-first, a LOUD failure through `swallow()` into `renderErrors()` plus
`LS_HEALTH`, day pruning, and `__gptsDebug.storage()`.

### ⚠ THE DAY EXPORT CARRIES READS BUT NOT ARCHIVED OUTCOMES — and the digest mislabels it
**⚠⚠ THIS ENTRY REPLACES ONE WRITTEN AND WITHDRAWN THE SAME DAY.** For a few hours on 2026-08-31 it
read "THE BUDGET IS SMALLER THAN A SESSION — THE MORNING IS SHED", stated as CONFIRMED. **That was
false.** The disproof was inside the evidence it cited: snapshots covered the whole session while the
queue did not, and the shedder being blamed trims both together. See FINDINGS **F-10b** (withdrawn)
and **F-10c** (what is actually true).

**Measured on `data/2026-08-31.json`:**

    snaps[].feat   131 of 131 bars   08:30 -> 15:00 CT   ALL 48 feature keys
    day.feat        29 bars          13:36 -> 15:00 CT
    digest verdict  "COLLAPSED, 22% bar coverage"        <- reads the QUEUE, not the record

**The per-bar READS are complete and exported.** `day.feat` is a resolution queue; resolved records
are mirrored to IndexedDB (`repoUpsertFeat` — "so local truth outlives LS") and `featStats()` reads
localStorage **plus** that archive, so his machine holds the full history.

**THE ACTUAL DEFECT: `buildDayExport` exports `day.feat` and not `FEAT_ARCHIVE`.** Resolved outcomes
older than the queue window never reach the repo — which is the only thing the nightly review can
read. The fix is an export change, not a storage change.

⚠ **AND `tools/day-digest.py` MUST STOP CALLING THIS A COLLAPSE.** Its `dataHealth` measures queue
depth and prints "COLLAPSED — do NOT compute rates over it". On a day whose reads are complete that is
wrong, and it caused two false diagnoses in one session. ⚠ F-9's historical table (`2026-08-27: 15
records / 1 bar`) should be re-read against `snaps[].feat` before it is trusted.

⚠⚠ **WHAT TRIMS THE QUEUE TO 29 BARS IS UNKNOWN AND MUST NOT BE GUESSED AGAIN.** `FEAT_KEEP_BARS` is
**160** (above a 131-bar session) and the `lsPut` shedder is ruled out by the intact snapshots. **Two
mechanisms have been named confidently and both were wrong.** `__gptsDebug.featHealth()` and
`__gptsDebug.storage()` each answer it live on his panel in one call. Measure before writing.

### ⚠ THE FEATURE-RECORD COLLAPSE — ROOT CAUSE FOUND (see above); the v14.67 instrument was aimed wrong
3,822 records on 08-20 against **15** on 08-27. `matrix` rows track exactly (108→3132, 122→3822,
0, 0, 23→990, 2→8, 1→15) against 133 SPY snaps, so it is **ONE upstream gate, not 46 features
failing**. It blocks the gamma conditioning AND the forward test of the LOD/HOD table.

### Overnight / globex context — untested, cheap
The ES/NQ corpora contain ETH bars that every study currently filters out. Does the overnight range
or direction inform where the RTH extreme lands? Genuinely unknown; one study answers it.

### A test that the resume note was updated
`test_chat_history.js` fails the build when the chat history goes stale. **Nothing guards the resume
note** — and on 2026-08-28 it went SEVEN builds stale while every other document was current, until
the operator asked. The same mechanism would close it.



### ITEM 18 · Yahoo Finance HTF/ITF data
**Locked 2026-08-16. Lost 2026-08-20. Recovered 2026-08-27. ⚠ HALF BUILT 2026-08-28 (v14.59).**

⚠⚠ **READ THIS BEFORE CLAIMING ITEM 18 IS DONE. IT IS NOT.** v14.59 built the *route* item 18
specified and used it for ONE purpose — the daily ES bar corpus behind ⓪a HOD/LOD:

| item 18 asked for | state |
|---|---|
| the Yahoo `chart` endpoint reached from the browser | **BUILT** — companion v1.15, `GM_xmlhttpRequest`, `@connect query1.finance.yahoo.com` |
| plain `fetch` first, `GM_xmlhttpRequest` as fallback | **SETTLED** — plain fetch measured **BLOCKED** from page context 2026-08-27; the grant route is the only one |
| a Layer-0 source feeding the day export | **BUILT** — `gpts_futbars_v1` → `futBars` in `data/YYYY-MM-DD.json` |
| **Tier 1: prior week/month H/L/C, 20/50/200 DMA, daily ATR(14), gap vs ATR, position in weekly range** | **NOT BUILT** |
| **Tier 2: 60m/15m 1h/4h trend + swings, 1h ATR** | **NOT BUILT** |
| **`STATE.htf` / `snap.htf` / "nearest chart level" per node; READ citing an HTF level** | **NOT BUILT** — zero hits for `htf` in either script |

**So the pipe and the tap exist and carry 1-minute bars; the HTF/ITF derived reads do not exist.**
Anyone who greps `yahoo` and finds the courier must not conclude item 18 shipped. The remaining work
is a consumer of bars we now already have, which is strictly easier than it was.

⚠ **THE 2026-09-16 DEADLINE STILL STANDS AND IS NOT ADDRESSED.** The daily tap prevents *new* gaps;
it does nothing for the existing **2026-07-18 → 08-14** hole, which is recoverable only at 2-minute
resolution and only until 09-16. That is a separate backfill run, still unbuilt.

### ITEM 18-OLD · the original entry, kept verbatim for the record
**Locked 2026-08-16. Lost 2026-08-20. Recovered 2026-08-27.**

Full specification and the recovered verbatim text: **`session-state/YAHOO-PIPELINE.md`**.

Verified absent four ways on 2026-08-27:

| check | result |
|---|---|
| `htfFeed` / `gpts_htf_v1` / `.htf` / `yahoo` / `query1` in either userscript | **zero hits** |
| `"htf"` key in any of the 8 exported day files (08-17 → 08-26) | **none** |
| Yahoo or HTF in `skills/gex/SKILL.md` | **no** |
| `.github/` or any workflow yml across all 171 commits | **none** |

**The one unresolved constraint:** the spec says `@grant GM_xmlhttpRequest`, which **cannot** go in
the main panel — `@grant none` is load-bearing there (the feed hooks patch `window.fetch` and
`XMLHttpRequest` in page context; any grant sandboxes the script and kills the tape). The spec's own
hedge, *"verify unsafeWindow access still OK"*, is that doubt, and the console check it waited on was
never done. **The IF companion already carries `GM_xmlhttpRequest` and already couriers a foreign
site — the ForexFactory calendar at v1.14, writing `gpts_evcal_v1`.** Yahoo is that template with a
different URL.

⚠ **DEADLINE: 2026-09-16.** Yahoo serves intraday bars ≤60 days. The corpus gap starts 2026-07-18;
sixty days from it is 09-16. After that those 20 sessions leave the window permanently.

### ITEM 14 (partial) · premarket high/low never recorded
Item 14 specified per-bar chart levels: VWAP, PDH/PDL/PDC, **PMH/PML**, IB30 H/L, POC/VAH/VAL. All
are present in the current build **except premarket H/L — `PMH`/`PML`/`premarket` return 0 hits.**
The operator's Skylit session-levels setting includes Premarket H/L, so the level is on his chart and
absent from our record. Not confirmed cancelled; treat as open until he says otherwise.

### ✅ ⓪a HOD/LOD — BUILT (v14.57 → v14.66). Entry kept so nobody re-discovers it.
**Closed 2026-08-28.** This said "APPROVED, NOT BUILT, BLOCKED ON DATA" and every word of that is now
wrong: the corpus was never missing (it is `data/es-1min/ES TestingData.txt`, 284 sessions — the
tooling looked for `EPM26-1min.csv.gz` and reported it absent, which is why the operator supplied it
twice), and the section shipped complete with the wick family, trimmed-mean base rates and the
probability table. See FINDINGS F-1..F-8.

### ✅ IB60 — BUILT v14.57. `sessionLevels()` now computes IB30 AND IB60.
**Closed 2026-08-28.** Both are kept deliberately: the operator asked for a sweep testing IB30 *and*
IB60 breaks, which is impossible if one replaces the other.
⚠ Measured since (FINDINGS F-1/F-2): **IB60 scores AUC 0.655 on the LOD/HOD question and adds nothing
once `posr` is known** — it is a crude switch approximating distance-travelled. It stays computed and
recorded; it is NOT a confirmation and was removed from the ⓪a face at v14.65.

### THE LADDER WIDTH — ⚠ PARTLY CLOSED IN v14.54, ONE DECISION STILL OPEN

**BUILT v14.54:** the ladder was re-laid to `mockups/mockup-ladder-v11.html`, the approved spec.
%King moved inside its own bar (the `LAD_KPCT` column deleted), the roll lane moved left of the
prices, the mirrored profile became the signed dollar delta profile, and the King pills and EM edges
moved into the chute — which is what empties the left gutter the operator photographed.

**⚠ THE 24px THIS FILE COULD NOT EXPLAIN IS EXPLAINED.** It recorded `scrollWidth 656 / clientWidth
632` as a discrepancy. `LAD_ROCW` was **56 for a column that needs 84** — the widest ROC string,
`−100% −100% ▼99%`, is 83px — so the last column overflowed its declared width from v14.46 onward.
`test_ladder` now asserts `LAD_ROC + LAD_ROCW === LAD_W`, so the constant cannot lie again.

**TRUE WIDTH: 657 → 618.** (Not 632 → 588: the old constants understated the original.)

**⚠⚠ STILL OPEN, AND IT IS THE OPERATOR'S CALL: 618 DOES NOT FIT A 454px PANEL BODY.** Measured on
his live panel 2026-08-27. No arrangement of nine columns of 8.4px text fits 454, and the approved
mockup is itself drawn on a **544px** panel — so the design he signed off assumes a wider panel than
he is running. The container SCROLLS rather than clips, so nothing is silently dropped. Three ways
to close it and none may be taken without him:

1. **Widen the panel to ~620.** It is resizable, it is one drag, and it costs nothing. Cheapest.
2. **Abbreviate the STATE words** to the mockup's WEAK/FORM/TURN/DOOR/USED — but the vocabulary was
   SETTLED at v14.49 (BUILDING · HOLDING · TURN UP/DN · WEAKENING · SPENT) and the resume note says
   not to re-litigate it. Would save ~16px and reopen a closed decision for very little.
3. **Drop a column.** The marker and the tests counter are the two candidates, and both are load-
   bearing under the v14.49 three-orthogonal-facts rule. Not recommended without a measurement.

⚠ **DO NOT CLOSE THIS BY DELETING A COLUMN ON YOUR OWN INITIATIVE.** The width is a display
preference; the columns are the level lifecycle.

---

## AWAITING OPERATOR VERIFICATION (not a build item — a question)

- ✅ **CLOSED 2026-08-27 16:41 CT — the v14.52 in-place CSV write RUNS.** Measured on the live
  panel: `inPlace:true`, `how:"file"`, `err:null`, last write 2.9 minutes old on a 180s cadence.
  The atomic-replace diagnosis was correct. ⚠ What is confirmed is that OUR write path executes and
  reports success; whether IRT itself now shows new lines without a refresh is still the operator's
  observation to make. If it does not, the fallback is a local HTTP server in that folder
  (`python -m http.server 8000`, then IRT "Remote File"). GitHub raw is NOT an option — CDN-cached
  ~5 min against a 1-min poll.
- **The FlexLevels CSV had only 3 rows at 16:41** (header + SPXW KING + QQQ KING) where an earlier
  session logged 14 levels. After hours with velocities at 0, so probably expected — but it has not
  been checked during RTH and it is the kind of thinning that passes for normal. Look once at the
  open.

---

## HOW TO ADD TO THIS FILE

One entry per locked item: what was agreed, **when and by whom**, where the full spec lives, what is
verifiably built vs not (with the check that proves it), and any deadline. When it ships, move it to
a `BUILT` section with the version — do not delete it, so a later context can see it was finished
rather than wonder whether it was lost.
