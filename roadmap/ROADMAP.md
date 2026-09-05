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
- **v15.65** (2026-09-04) — the PATTERN columns — one per book (SPX · SPY · QQQ): a PIKA / BARNEY block in black on yellow / purple with the book's own strikes in yellow under it, RUG / RRUG blocks, a SPY / QQQ pattern off its King placed on the ladder row nearest its converted price; the NOW row in a white hue; the three Kings in their own colours (SPX orange · SPY blue · QQQ cyan) so no King shares a node colour; the heatmap claim in the brain · serves: HOD/LOD · PULLBACK · reading the three books' shapes at a glance
- **v15.66** (2026-09-04) — THE TAPE — the whole book, every closed bar, every market: SPXW's full velocity table (all ~286 strikes, Skylit's dollars and d5/d15/d60/d1d) and the whole Trinity ladders for SPY · QQQ · VIX (every strike as %King + the King's $K), captured into IndexedDB during the day and written ONCE at the close beside the day file as data/tape/<day>/<BOOK>.json; a captured day never written is written on the next boot or 💾; tools/nightly/tape.py reads it and run.py logs the coverage · serves: the end-of-day review on the WHOLE day — SPY/QQQ patterns scorable, NEW on SPY/QQQ, Q11's dollar axis
- **v15.67** (2026-09-04) — SCORE THE SETUPS AND PATTERNS — every tap in the deflection ledger is stamped at the moment of the tap with the patterns as the face defines them (per book: PIKA / BARNEY stack, named or member; RUG / RRUG; the King by book; gate; NEW; growth into the tap; floor / ceiling) and scored 30 minutes later; the held rate by setup × book with its n and Wilson bound on the Testing tab (⑦ THE PATTERNS) live, and from the nightly across every recorded day (tools/nightly/patterns.py → the log) · the complete architecture on the ⚙ tab and in design/ARCHITECTURE.md (components, integrations — Skylit, InsiderFinance, Yahoo, ForexFactory, GitHub — the daily HOD/LOD statistics pipeline, storage) · serves: PULLBACK · HOD/LOD · 'score these setups and patterns to get proper probabilities and insights'
- **v15.68** (2026-09-04) — THE LOOP CLOSES ON THE CLICK — his 💾 is the only manual step: the 'GEX nightly' Windows task (setup-gex-nightly.bat, run once) runs the nightly on his machine within 10 minutes of the save (tick.py: only when the day file is newer than its log), the nightly writes the Analysis tab's registry itself (results.py: every study whose number the log can answer — the King by book, the rugs, the stacks, the register's verdicts — result · status · by the nightly · asOf; a thin row keeps the review's sentence and shows the count so far), the sync pushes it, the panel re-fetches the registry on its 10-minute check; the log says where it ran · serves: the loop closes without a session — 'clicking on the save, the data getting saved and the analysis occurring and the analysis tab being updated'
- **v15.69** (2026-09-04) — THE OBJECTIVE OUTCOMES — every tap scored, from the day's own bars, on the two decisions PURPOSE names: TURN (the tap's extreme within 0.50 SPY of the session's HOD / LOD — the node WAS the turn) and RESUME (a new session extreme after the tap — the pullback ended, stay in), first tap per node per day; beside held on the nightly's pattern table and in the Analysis rows' lines · the Learn tab's rules carry THE RECORD: each rule that names a class gets the ledger's numbers for it and a verdict — agrees / contradicts at n ≥ 15 on both sides, thin until then, not measured where the ledger has no class yet (L3 / L8 the King path, L4 the side flip, L7 the clock); the seed merges it so a review never erases it · serves: PULLBACK · HOD/LOD — 'the entire data, analysis and testing process results in learning'
- **v15.70** (2026-09-04) — 💡 REC — the eighth and last tab: proposals TO him from the nightly (pre-registered conditions: a class clear of the base at n ≥ 15, a Learn rule the record contradicts, a hypothesis cleared) and from the review (tools/rec-seed.py), each with what it changes and its evidence; his ✓ / ✗ saved at once and riding the next day file (`reco`), the nightly setting the row's status; withdrawn when the record stops supporting it; nothing on the face changes except through here · THE DATA ANALYSIS PROCESS named and written (design/DATA-ANALYSIS-PROCESS.md: the seven links, the degrees of knowledge, the ten rules, the eight tabs) and pinned by a test · learning/markets.json — every market-specific number in one place, gold as a configuration entry when a gamma book exists · serves: 'from that point on you take over from data, analysis, testing, learning all the way to the Rec tab, which is where we will discuss what to implement'
- **v15.71** (2026-09-04) — THE SAVE RUNS ITSELF — no click at the close: after the close (15:01 CT and later, no upper bound) any tick that finds the day not confirmed in the repo folder writes it and retries every 10 minutes until it lands; outside market hours every earlier day still in IndexedDB with bars and no file is written, write-if-absent, and marked once; saved means confirmed in the folder and nothing else (the silent download fallback is gone); the 💾 becomes a chip — saved · pending · DUE — and its click carries the folder permission inside the gesture (Chrome 122+ offers Allow on every visit there), so after one such click no rule ever needs him again; a day with no recorded bars is never written on any path · serves: 'automatically have the application trigger the save button instead of me clicking it' — the Data Analysis process with zero steps at the close

## NEXT — the running build

### v15.72 — THE FACE, HIS THREE ASKS + ONE BUG + ONE READ — the AFTER HOURS chip leaves the King row for the bottom of the panel (a full-width bar above the footer, only after the close); the three King cards take the whole row and grow (price 12 → 16.5 px, titles 6.8 → 8.6, the lines 7 → 8.6, the pills 6.2 → 7.4); the ladder font 7.6 → 9 px with the columns widened ~15% (ladderFit grows the panel once); the amber sliver inside every RUG / RRUG / PIKA / BARNEY block was a dead .g3pb rule’s left border leaking into the v15.65 class of the same name — gone, pinned; THE ROLLING FLOOR / CEILING — his read ("when the king rolls up and is below price it may be creating a floor (support) and be bullish and vice versa") is the doctrine’s rolling floor on the biggest node: every tap is stamped with each book’s King roll today (kroll), four classes in both twins (king:floor:up · king:floor:dn · king:ceil:dn · king:ceil:up), K2.6 / K2.7 on Analysis, H8 / H9 in the register read once at n = 30 from the first stamped session against the floor / ceiling base; .gitattributes fixed (the task scripts stay CRLF), Claude outputs/ ignored

Serves: the face he reads at the tap · "i think there is something to this" — a read becomes a claim before the data.

Done when: the chip renders under the replay strip after the close and nowhere else; the King strip spans the row; no .g3pb block carries a left border; H8 / H9 read THIN with their base counts in the nightly log; the first stamped session counts a king:floor:up tap.

## AFTER THAT — in order

### v15.73 — the deflection candidate score — the L-rules (growth into the tap, fresh, stack, roll, King distance, time of day, level confluence) as a live 0–100 per node as price approaches it, measured by the deflection ledger's CONTINUED / STALLED outcomes — the gauge's predict part · the per-book King rows from the ledger (S0.1–S0.4) · the H5 join

Serves: PULLBACK · HOD/LOD · 'predict a deflection will occur once you see price is going to the node'.

Done when: 30 scored calls exist and the gauge's predict part shows a Wilson lower bound, not a hope.

### v15.74 — score THE READ (stage ⑪) · MARK / STATE / polarity hovers say 'descriptive' until measured

Serves: HOD/LOD · the loop closes.

Done when: a scored READ line exists in a nightly log and renders on Testing ③ with its n.

### v15.75 — the TAP record — one row per fresh tap with the node's condition, both zones, extent, wasSessionExtreme

Serves: HOD/LOD · PULLBACK · 94 OPEN studies · H6.

Done when: 40 taps recorded and the first F-study reads from them with n.

### v15.76 — the nightly reads one READ NEXT study per night and writes it back · TRACK → DRAFT study

Serves: the loop closes on its own.

Done when: a study changes status without a human editing the seed.

### v15.77 — one definitions file (Python + panel) · the shipped-artifact test

Serves: hardening.

Done when: changing a bin in one place fails the suite until the other side follows.

### v15.78 — the face manifest — every number on the face names its study, pinned by a test

Serves: data-driven, enforced.

Done when: a new number on the face without a study fails the suite.

### v15.79 — the pullback outcome — RESUME to a new extreme for VWAP/value-area and node taps inside a move

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
