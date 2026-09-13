# THE GAMMA PROFILE — a custom IRT indicator

**Status:** DESIGN, agreed one element at a time with the operator, 2026-09-12. Not built. Mockups:
`mockups/gamma-profile-*.html`. This document is the spec; it is edited as elements are settled.

**Operator's ask (2026-09-12):** *"help me build the gamma node profile custom indicator for IRT."* Then, when
asked the form: *the gamma profile can be similar to the Volume Profile.*

---

## 1 · WHAT IT IS

A horizontal profile drawn on the Investor/RT (Linnsoft) price chart, built exactly like IRT's **Volume
Profile** — but the bars are **gamma**, not volume. It renders the option-dealer gamma book (the same data the
Skylit Atlas ladder and the panel's IRT FlexLevels export already carry) as a profile beside price, so structure
is read as a *shape* rather than a stack of lines.

The Volume-Profile → Gamma-Profile mapping (the mental model):

| Volume Profile | Gamma Profile |
|---|---|
| bar length = shares at that price | bar length = the node's magnitude (\|%King\|) at that strike |
| POC (fattest bar) | **the King** — the highest-gamma strike |
| Value Area (~70% of volume) | the cluster of the biggest nodes — where dealer gamma sits |
| HVN / LVN | walls vs **air pockets** (a low-gamma gap = the pathway price travels through) |
| single color | **polarity color**: yellow +γ / purple −γ |

**Doctrine anchors** (Skylit Academy): air pocket = pathway, not target; King margin of interaction 5–10 SPX; polarity
sets the *character* of an interaction (+γ smooth/low-vol, −γ wicky/violent), not its strength.

---

## 2 · WHY RTX (the rendering mechanism)

Checked against Linnsoft's own docs (2026-09-12):
- **RTL** custom indicators are **time-indexed** (a value per bar) — they cannot draw bars at arbitrary price levels.
- **FlexLevels** draws full-width horizontal **lines only** — no bar length, so no histogram.
- IRT's **native Volume Profile** is single-color and volume-driven — it cannot be fed gamma or show +/− polarity.
- **RTX** (the C++ SDK, installed at `C:\Program Files\LinnSoft\InvestorRT\sdk`) supports custom drawing —
  "lines, shapes, histograms, bands" at arbitrary coordinates. **This is the only path** that draws a true gamma
  histogram with polarity colors.

**Data path:** the panel exports a per-strike gamma file (schema in §8) → an RTX plugin reads it and draws the
profile(s) with the settings below. (The panel is `@grant none` and cannot write files itself — the companion /
the existing IRT-export transport handles the file, same as the FlexLevels export today. To be confirmed at build.)

---

## 3 · PHASES

- **Phase 1 — GAMMA profiles.** Buildable now; the panel already holds every per-strike gamma value.
- **Phase 2 — DELTA profile.** Needs per-price bid/ask **footprint** data, which is not flowing yet (the RTX SDK
  footprint feed the operator wants to build separately). The design carries a Delta slot from day one so nothing
  has to be re-architected when the feed exists.

---

## 4 · THE PROFILE (form)

- **Own space.** Each profile occupies a **reserved column**; the price candles are NOT overlapped (operator,
  2026-09-12, with the IRT Volume-Profile dialog: *"the profile typically takes up its own space so price bars
  don't overlap it"*).
- **Faces price.** Bars grow toward the candles (a left profile grows right, a right profile grows left).
- **Layout A** (right-edge overlay) was picked over a separate pane — but "own space" governs: the bars live in
  their reserved column, they do not sit over the candles.

---

## 5 · CONTROLS (the settings panel)

**Placement & size**
- **Placement:** `Mirror L+R` (one profile each side, both facing price) · `Stack right` · `Stack left` (two
  profiles on one side, both facing price).
- **Width:** in **pixels** (as IRT's Volume Profile — the operator circled "Width 60 Pixels"). Per profile.
- **Up to TWO profiles.** Profile 2 = `None` for a single profile.

**Which book (per profile)**
- ES: `SPX γ` or `SPY γ`. NQ: `QQQ γ` (and `NDX γ`).
- Profile 2 may be the other book **or** a `Delta` profile (§7).
- **Both Kings always.** The SPX King and SPY King level lines display no matter which book is being profiled
  (operator: *"both king nodes displayed no matter what profile I select"*). SPX King gold, SPY King cyan.

**Which nodes (γ)**
- `Top 3` · `Top 5` · `≥ N%` · `All`.
- **`≥ N%` threshold is editable**, default **20%** (matches Atlas's node setting: top 3 / top 5 / 20% / more).
- **`All` greys out** the nodes **below** the threshold instead of hiding them (operator: *"display all the nodes,
  but if they are below threshold, then they will be greyed out"*) — so the whole book is visible but the
  qualifying nodes stand out.

**Labels & justification**
- **%King** label: always **inside the bar, justified RIGHT**.
- **Rank** (1..5): **inside the bar, justified LEFT**, for the top 5 (whatever set is selected).
- The King bar reads as **KING · POC**.

**Extend lines**
- **Extend King line(s):** toggle — draws a horizontal line across the chart at each King.
- **Extend HGN lines** (High-Gamma-Node): toggle — draws a line at each node in the selected set (Top 5, or the
  chosen %).
- **Lines are SOLID by default.**
- **Line-label position:** each extended line's label can be placed **left of the line, center, or right**
  (operator, 2026-09-12).

**Color hierarchy (γ)**
- King / POC: distinct (cream-gold, white border).
- Top 5: bright — gold (+γ) / purple (−γ).
- Below top 5 / sub-threshold: dimmer shade of the same polarity color (greyed when `All`).

---

## 6 · OVERLAID LEVELS & PATTERN LABELS

**Levels to display (toggle each):**
- From **InsiderFinance**: **Call Wall**, **Put Wall**, **Flip (zero-gamma)**. (The panel already reads these via
  the IF companion; the IRT export already carries CW0 / PW0 / FLIP0.)
- **Expected-Move High / Low** (the EM edges).

**Pattern labels (label on the profile when the pattern is present):**
- **Pika stack**, **Barney stack** (the stack patterns).
- **Gatekeeper**.
- **Rug**, **Rrug** (reverse rug).

⚠ **Doctrine gate (per the build rules).** Each pattern's *detection* follows the Skylit definition
(`skylit-docs/patternpedia/` — Rug, Gatekeeper; the stack patterns from the operator's training method). Some of
these the panel may already detect; any that it does not need a detector built and verified BEFORE the label can
be trusted. The profile only *surfaces* labels for patterns already detected upstream — it does not invent them.

---

## 6b · THE DAY CANDLE (a side option)

A column option (instead of a profile) that draws the **forming daily candle** — the same read the panel's day
candle already gives, brought onto the IRT chart. Operator, 2026-09-12: *"a side option to draw the daily forming
candle … it will not have the swept levels but will have the rest."*

- **The candle:** open, current, HOD (high wick), LOD (low wick), body green/red.
- **HOD:** value + **time + duration** (e.g. `1st HOD · 09:42 · 30m`). **LOD:** the same.
- **MU / MD:** move up to the HOD (green) and move down to the LOD (red), as the panel shows them.
- **Reclaim-open / BOP** (the ↩ line): optional.
- **Layout — Expected | candle | Actual** (operator sketch, 2026-09-12): the candle in the **middle**, a column of
  **EXPECTED** values on the **left** and the matching **ACTUAL** values on the **right** (the panel's E-row /
  A-row, flanking the candle, one metric per row so E vs A read across). Uses the **fuller value set the panel
  already shows** — Bias (Red/Green), 1st HOD, 1st LOD, **MU / MD**, **HL gap**, **Range**, **Wick %**,
  **W.End·BOP**, Close — *"and more"*. (Replaces the old left-side percentages like 59% / 8% / 33%, not wanted here.)
- **The HOD/LOD read line** incorporated at the top, **compact** — e.g. *"HOD 100% · LOD · 80% floor 4:01p (past
  close)"* (operator, 2026-09-12: *"incorporate the HOD in line"*, *"make the HOD header more compact"*).
- **Display conventions** (operator, 2026-09-12): headers abbreviated **Exp / Act** and **color-coded**; columns
  hug the candle (little gap); unknown values show **NA / TBD**; the candle's **HOD/LOD labels carry time +
  duration** (top for HOD, bottom for LOD). ⚠ **HL gap is a TIME** — the duration between the HOD and the LOD
  (e.g. exp 3h 13m vs act 48m), **not** a price gap. Nice-to-haves shown in the mockup: a beat/miss dot per row,
  and a "tracking vs expected" line under the candle.
- **NOT drawn:** the swept levels; the decay percentages.
- **Side:** left or right, like the profiles.
- **COMPACT** — a slim column so it can sit alongside profiles (operator, 2026-09-12): e.g. day candle on the
  left, and a γ profile + delta profile **stacked** on the right. Expected stats + HOD/LOD/MU/MD all fit in the
  narrow column.

## 7 · THE DELTA PROFILE (Phase 2)

A second profile whose bars are **order-flow delta** (net buy − sell volume) at price — the footprint read that
supports absorption detection (operator's standing goal). Green = buy-dominant, red = sell-dominant.

- **Timeframe:** selectable lookback — `5m delta`, `10m delta`, `15m delta`, … (how far back the delta accumulates).
- **Overlap ghost:** show a **second (higher) timeframe** at once as a ghost outline so they can be compared — the
  ghost is **selectable: 15m / 30m / 60m** (operator, 2026-09-12), not just 15m.
- **Growth coloring:** the nodes that **grew the most in the last X minutes** are drawn **brighter** (a
  fast-growing delta → a **light bright green**, not a yellow-lime — operator, 2026-09-12), so building pressure is
  visible at a glance. ⚠ OPEN: whether growth-brightening should keep direction (bright green for a growing buy,
  bright red/orange for a growing sell) or use one "growth" color regardless of side — pending operator.
- **Top-5 delta:** a mode that brightens the **five fastest-growing** nodes, the very top the brightest.

Delta needs the per-price footprint feed (not yet flowing) — Phase 2.

---

## 8 · DATA EXPORT (to be specced at build)

The panel writes a per-strike file the RTX plugin reads. Sketch (final schema at build time):

```
symbol, book, asof
strike, pctKing, signedGamma, rank, isKing
...          (every strike in the book, not just top-N — the plugin filters/greys client-side)
```
Plus a small levels block: `callWall, putWall, flip, emHigh, emLow, spxKing, spyKing` and any active pattern
labels `{pattern, strike}`. The Delta file (Phase 2) is per-strike signed delta per timeframe.

---

## 9 · OPEN / NEXT

1. Operator sign-off on the control set and the look (this doc + the mockups).
2. Confirm the file **transport** (which userscript writes the file, where IRT reads it — same plumbing as
   FlexLevels, or a new file).
3. Spec the per-strike file schema (§8) precisely.
4. Build the RTX plugin (C++), Phase 1 (gamma). Delta is Phase 2 behind the footprint feed.

---

## 10 · SETTLED DESIGN — 2026-09-12 session (the full IRT indicator)

Agreed one element at a time with the operator over a long mockup session. **Visual reference of record:
`mockups/irt-mockup.html`** (composited on his EPU26 3-min chart). This section supersedes earlier layout notes
where they conflict.

### 10.1 · The four zones (one indicator, composited on the IRT chart)
Modeled on IRT **VolumeScope "Detach Last X Bars"** — anything that would overlap the price bars is drawn in a
**reserved margin** instead, at a set pixel width, so the price line never runs through it. Verified against
Linnsoft docs (VolumeScope RTX / Profile). Left→right:

1. **Day Model** — a horizontal strip across the TOP (below the IRT title bar). Frees the sides for the candles.
2. **Model candles** — reserved LEFT margin: expected + actual day candles, price-aligned to the IRT axis.
3. **Price chart** — the operator's normal price in the center (the plugin does not redraw it).
4. **Profiles** — reserved RIGHT margin, detached, facing price: **γ nearest price, Δ to its right** (operator,
   2026-09-12: "the delta profile should be to the right of the gamma profile"). Two 60px columns by default.

**It is an RTX indicator, so it reads IRT's own price axis** — every candle/level/profile Y comes from the chart
axis (operator: "remember you will be using irt price axis because you are going to be an irt indicator"). No
separate calibration; alignment is automatic in the real plugin.

### 10.2 · The Day Model strip
- **Weekday name** (Mon/Tue/…) in the header, NOT a generic "DAY" (operator, 2026-09-12).
- Two rows: **EXP** and **ACT**, one value per field, color-coded field names.
- **Field order (from his tape-reader strip):**
  `1ST · [FIRST extreme] · Took · BOP · Wick · W.End · Wick% · MUD · MUD t · [SECOND extreme] · HL Gap · HL Rng`.
  ⚠ The two extremes are **NOT adjacent**: the FIRST extreme sits right after `1ST`; the intermediate fields run
  between; the SECOND extreme sits near the end, just before `HL Gap`/`HL Rng` (operator, 2026-09-12: "the second
  extremity is not suppose to come right after the first extremity").
- **Dynamic first-extreme:** whichever printed first leads. LOD-first → `1ST · LOD · … · MUD · HOD · HL Gap · HL Rng`;
  HOD-first flips it (operator: "first hod or lod is dynamically displayed").
- **MUD is TWO cells:** `MUD` = the move (points · $) and `MUD t` = its duration.
- ⚠⚠ **MUD time = HL Gap − BOP.** It runs from the **open-reclaim** (BOP done) to the **2nd extreme**, NOT
  extreme-to-extreme like HL Gap. Verified against his pic: actual HL Gap 48m − BOP 15m = MUD 33m ✓; expected
  1h37m − 24m = 1h13m ✓. So MUD t ≠ HL Gap by exactly BOP.
- **Formats:** clock times 12-hour am/pm (1:30pm, not 13:30); durations `Xh Ym` (1h 36m, not 96m).
- **Trade-management fields removed until defined** — Rly, Done, PB, Num, Ret, Risk, Ext, Tgt, Rwd, Dur, Time are
  NOT shown yet (operator, 2026-09-12: "remove the fields in the pic because you dont know what they are yet").
  They return only once each is defined and computed.

### 10.3 · The model candles (reserved left margin)
- **Expected** candle = ghost (dashed), body **green/red by expected close vs open**. **Actual** = solid developing.
- **Split wicks** — upper (high→body-top) and lower (body-bottom→low); the wick does NOT run through the body.
- **Body wide enough to hold the MUD box.**
- **HOD/LOD written on the wick tips** — the literal word `HOD`/`LOD` + value on the line nearest the tip, then
  **time and duration on separate lines** (value / time / duration stacked). Actual HOD carries a real duration,
  never just "so far".
- **MUD box inside the body:** `MUD ±pts` / MUD-time (= HL Gap − BOP) / dollar.
- **Swept levels** (PDC / OPN / ONL / …) as dashed ticks across the candle with tags **to the RIGHT** of the candle
  (no ⊠ icon).
- **E-HOD / E-LOD** = the model's expected extremes, drawn as level lines (see 10.5), blended with IF EM.

### 10.4 · The gamma profile (reserved right column, faces price)
- **Every 5-point strike** (SPX book), not sparse — a dense realistic ladder (operator, 2026-09-12).
- **King = POC** (cream, white border). **+γ gold / −γ purple**; sub-set nodes a dimmer shade.
- **Node filter:** Top 3 / Top 5 / **≥ N% (editable, default 20)** / All (greys sub-threshold, never hides).
- **Rank 1–5 inside the bar, left-justified; %King inside the bar, right-justified.** Value-area shading on the
  high-gamma cluster.
- **Pattern tags INSIDE the node bars** (operator, 2026-09-12): **Pika · Barney · Gatekeeper · Rug · Rrug**.
  ⚠ Doctrine gate: only surface a label for a pattern DETECTED + verified upstream; build/verify any missing
  detector first (§6).

### 10.5 · Reference levels (horizontal lines, labels ABOVE the line)
Every level label sits **on top of its line**, never crossed by it (operator, 2026-09-12). Toggle each:
- **InsiderFinance:** Call Wall, Put Wall, Flip (zero-gamma).
- **Expected Move:** EM High / EM Low (0DTE straddle — already computed by the companion `expectedMove()`).
- **Model:** E-HOD / E-LOD (blended with the IF EM).
- **King POC** line. Extend-King and Extend-HGN toggles; lines solid by default; label position left/center/right.

### 10.6 · The delta profile (reserved right column, right of gamma) — Phase 2, behind the footprint feed
- Per-price delta; **15m solid + selectable ghost 15/30/60m**; green buy / red sell.
- **Growth:** the fastest-growing nodes brightened (light bright green) AND their **5-minute growth amount shown**
  (`▲+28`), in a high-contrast font (operator, 2026-09-12). Top-5-delta mode.
- Needs the per-price **footprint feed** (RTX SDK) — not flowing yet.

### 10.7 · Open items carried forward
- Confirm the file transport + pin the export schema (§0 of the build plan).
- Value-area shading vs a VA-high/VA-low bracket (minor).
- The trade-management fields (Rly…Time) — define each, then re-add.
- Delta growth: keep direction on the brightening, or one "growth" color (still open, §7).

---

## §11 · BUILT DESIGN — 2026-09-13 (the RTX plugin, live)

The design below is what actually renders in `lsGammaProfile.dll`. See
`plugin/GAMMA-PROFILE-PLUGIN.md` for the code/build/architecture.

**Color = the Skylit tape's diverging heatmap.** Teal at gamma≈0, gold for strong +gamma, magenta
for strong −gamma, blended by |%King|. The King is colored by its own **polarity** (magenta when
negative) exactly like the tape — no special King color. Polarity is double-encoded by the **sign in
the % text**, so an important node's polarity is never color-alone. `Amplify polarity` (off by
default) reaches the pole colors faster for legibility (a deliberate, optional deviation from the
tape's muted mid-range).

**Node labels (operator-approved via mockups):**
- **% OUTSIDE** the bar tip, signed. Small bars keep their % (they'd be clipped inside). Hidden below
  an abs-% threshold to cut clutter.
- **Node TYPE inside** near the base — KING now; GK/RUG/RRUG/PIKA/BARNEY later (patterns, Phase 6).
- **Rank bubble INSIDE** at the leading tip (saves width) — dark circle, white numeral **centered on
  the strike** via font metrics. Top-5 only, prominent by contrast + size.

**Level rail:** the King **line** draws with NO label (the node labels it — kills the double "KING").
CW/PW/FLIP/EM-H/EM-L lines; labels on the **LEFT** so they don't collide with the node % on the right.

**Settings panel (34 controls)** — see the plugin doc for the full list and the parms-callback read
pattern. Notable additions beyond §10: `Detach` (VolumeScope-style, keep bars off the candles),
`Scale bars to King/Visible max`, `Amplify polarity`, `Rank position Inside/Outside`,
`Below threshold Grey/Hide`, per-line toggles, `Line style`, `Level label position`, `Header`,
`Spot marker`, editable color pickers for the three heatmap poles.

**Data-space note:** the live CSV is currently in **SPX price space** (King 7675) to match the tape's
numbers; the **SPX→ES basis ≈ +2** is applied when Phase 0 wires the panel's own conversion, which
will also align the profile to the panel's on-chart lines.
