# ROADMAP — incremental, one build at a time, each with a definition of done

_Generated from `tools/plan-seed.py` (the same data the 🗺 Roadmap tab renders) · written 2026-09-04 · the WHAT is `design/PURPOSE.md`, the HOW is `design/PROCESS.md`._

⚠ Edit `tools/plan-seed.py`, run it, re-splice `PLAN_SEED` — never this file by hand. Older roadmaps (`roadmap/PRODUCT-ROADMAP.md`, the v15.50 deflection roadmap) are history; this is the plan.

## Where we are

- **v15.55** (2026-09-03) — the Analysis tab by subject · TRACK · THE READ FROM THE STATS · serves: HOD/LOD · PULLBACK · the loop
- **v15.56** (2026-09-03) — the book's levels in the sweep read · the book table · the honest overnight (companion v1.18) · serves: HOD/LOD · H6
- **v15.57** (2026-09-03) — EM edges · VWAP + bands · developing profile · London · HVL/magnet · the two-line rule · serves: HOD/LOD · PULLBACK (interior levels are pullback candidates)
- **v15.58** (2026-09-03) — the READ ranks reclaimed sweeps first · the Testing tab as mocked · the installer manifest carries every fetched file · serves: the loop
- **v15.59** (2026-09-03) — the ⚙ Architecture and 🗺 Roadmap tabs — the WHAT, the HOW and the plan inside the app, as live status · serves: never forgetting the objective and the machinery
- **v15.60** (2026-09-03) — the 📌 Open Items tab (issues, questions) and enhancement requests on the Roadmap — one field, one store, the review answers in the file · serves: complete application management: requirements → design → open items
- **v15.61** (2026-09-03) — the ladder floor — never fewer than 8 strikes when the tape has them: sub-threshold strikes drawn dimmed as CONTEXT rows, display only (his 'only 3 strikes' ask) · serves: HOD/LOD · seeing the node around the King on a concentrated book
- **v15.62** (2026-09-03) — the mockups' look is the panel's look (one stylesheet, one skeleton, the scale control) · the 📚 Learn tab — the deflection learning doc, four taught examples checked against the record, eight rules, the 0–100 gauge · serves: PULLBACK · HOD/LOD · learning to identify and predict deflections
- **v15.63** (2026-09-03) — the dashboard conversation, feature by feature — the node row (NEW · ⇄ ROLL · ▲ GROWTH · SETUP per book in the patternpedia's colours), the King zone (three Kings as rows of one zone, the strip with growth / rolled / above-below), the per-book King tally, the SWEPT line, the DAY table and the taps list off the face, the growth window as a setting under test (S6.6) · serves: HOD/LOD · PULLBACK · 'what is the node DOING as price arrives'
- **v15.64** (2026-09-04) — the second dashboard conversation — NEW and the stacks calibrated on his taught days (NEW = not the opening book, crossed 20% within 20 bars, doubled or +20%/15m; a stack = adjacent same-sign nodes each ≥ 30% of the King, named once with a bracket; rug / reverse rug take the doctrine's price side), the King zone made obvious (NOW and King rows lit, Kings pulsing, the QQQ King live, ROLLED badge), the HOD/LOD line and the SWEPT line at the top in plain words (names only, making LOD / making HOD, details in the hover), the replay strip below, the tally off the face · the lost roadmap/ and archive/ recovered and carried by the installer · serves: HOD/LOD · PULLBACK · 'when price is going to a level, a new node pops up and deflects price'

## NEXT — the running build

### v15.65 — the PATTERN columns — one per book (SPX · SPY · QQQ): a PIKA / BARNEY block in black on yellow / purple with the book's own strikes in yellow under it, RUG / RRUG blocks, a SPY / QQQ pattern off its King placed on the ladder row nearest its converted price; the NOW row in a white hue; the three Kings in their own colours (SPX orange · SPY blue · QQQ cyan) so no King shares a node colour; the heatmap claim in the brain

Serves: HOD/LOD · PULLBACK · reading the three books' shapes at a glance.

Done when: a SPY or QQQ stack that does not sit on its King is visible on the ladder, and the render of 2026-08-28 13:12 shows every pattern in its book's column.

## AFTER THAT — in order

### v15.66 — the deflection candidate score — the L-rules (growth into the tap, fresh, stack, roll, King distance, time of day, level confluence) as a live 0–100 per node as price approaches it, measured by the deflection ledger's CONTINUED / STALLED outcomes — the gauge's predict part · the per-book King rows from the ledger (S0.1–S0.4) · the H5 join

Serves: PULLBACK · HOD/LOD · 'predict a deflection will occur once you see price is going to the node'.

Done when: 30 scored calls exist and the gauge's predict part shows a Wilson lower bound, not a hope.

### v15.67 — score THE READ (stage ⑪) · MARK / STATE / polarity hovers say 'descriptive' until measured

Serves: HOD/LOD · the loop closes.

Done when: a scored READ line exists in a nightly log and renders on Testing ③ with its n.

### v15.68 — the TAP record — one row per fresh tap with the node's condition, both zones, extent, wasSessionExtreme

Serves: HOD/LOD · PULLBACK · 94 OPEN studies · H6.

Done when: 40 taps recorded and the first F-study reads from them with n.

### v15.69 — the nightly reads one READ NEXT study per night and writes it back · TRACK → DRAFT study

Serves: the loop closes on its own.

Done when: a study changes status without a human editing the seed.

### v15.70 — one definitions file (Python + panel) · the shipped-artifact test

Serves: hardening.

Done when: changing a bin in one place fails the suite until the other side follows.

### v15.71 — the face manifest — every number on the face names its study, pinned by a test

Serves: data-driven, enforced.

Done when: a new number on the face without a study fails the suite.

### v15.72 — the pullback outcome — RESUME to a new extreme for VWAP/value-area and node taps inside a move

Serves: PULLBACK.

Done when: a resume rate with n renders on a tier-3 READ line.

## His decisions, still open

- the Skylit API backfill (~15–20k credits): years of taps in an afternoon — unblocks most OPEN studies
- NQ 1-minute beside ES for D5 (cross-book lead)
- fold the four low-value DAY columns (OF BAR ×2, PTN, BODY) into hovers, or keep them

## Constraints (operator-mandated)

- one install file per build, with the Tampermonkey links pasted as text; wait ~5 min, click, reload
- ✅ SAVE DONE naming the files saved; chat history regenerated last; every new assertion mutation-tested
- do not tune a parameter to make a number look good
- cloud push is policy-denied — the installer pushes
- the integrations are untouched: IRT export, the Yahoo courier, InsiderFinance

## The hardening backlog (PROCESS.md §5)

- the deflection candidate score — the L-rules as a live score per node, measured by the ledger's outcomes (the gauge's predict part)
- score THE READ (stage ⑪)
- one definitions file read by Python and the panel, pinned equal
- the nightly reads one READ NEXT study per night and writes it back
- TRACK → DRAFT study as a nightly step
- the face manifest: every number on the face names its study
- a shipped-artifact test (smoke-test the installer's payload)
- data-quality checks on the face (courier age, ratio drift, bar gaps)
