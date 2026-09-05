# DASHBOARD INVENTORY — every field on the face, and how it serves the objective (2026-09-03)

> Operator: *"inventory all the fields on the dashboard and see how they all support my objective.
> Basically i want the dashboard, Analysis and Testing tabs working together … a data driven
> application that uses data, analysis and testing to drive the Dashboard, its reads and more."*

Method: every element the face renders (from `secLoc` / `ladderHtml` / `secDay` / the footer), what
it shows, the objective it serves (**HOD/LOD** = the day's turning points; **PULLBACK** = the
continuation turn; **MOVEMENT** = watching gamma move so a turn can be seen forming), the study it
maps to in `learning/studies.json`, whether its number is **measured** (a rate with n behind it),
**descriptive** (a fact about now, no claim) or **unmeasured** (implies a claim it cannot back), and
the verdict. The verdicts are proposals; his call.

## 0i · v15.73 — the day line (2026-09-05, small hours)

One element added, at the bottom, between the AFTER HOURS bar and the footer: **the day line** (`dayLineHtml`,
`dayLineState`; `.g3pline`). The date (9/5), then the links in the process order — **saved · analysis · testing ·
learning · rec** — each a dot and a few words of evidence: "saved 15:01 · the panel · 131 bars", "analysis 15:11 · your
machine · 81 taps counted", "testing 9 claims read · 1 ready · 8 thin", "learning 9 rules carry the record", "rec 6
awaiting your ✓ · 0 new". Colours: green done · amber expected but not yet ("analysis not yet — the task runs every 10
min") · red overdue with the cause ("analysis overdue — is the GEX nightly task installed?" 30 minutes after the save with
no log; "saved overdue — see the 💾 chip" past 15:15 with no confirmed write; "cannot write — click 💾" when the chip is
DUE) · grey not knowable yet. During the session the first segment reads **data · recording · N bars** and the rest say
what will happen; yesterday's completed line stays until today records its first bar; a day known only as a download
reads "N bars, not saved". The hover is the whole sentence, opening with *"What has the process done with 9/5?"*.
Sources: the 💾 chip's own save evidence (`saveState`, `AUTOSAVE.lastWrite`, `DAY_WRITTEN`), the pipeline record
(`pipeLoad`: saveDate / saveHow / pushed), the nightly log (`ANALYSIS_NIGHTLY`: date, ranOn, ranAt, hypotheses,
patterns.events), the Learn file (`learnLoad`: rules' verdict + asOf), the Rec file merged with his decisions
(`recMerged`). Degree: **descriptive** — every word is a fact about the record; no rate, no claim. Probe:
`__gptsDebug.dayLine()`. A segment never clips — the row wraps at a segment boundary, so a completed day is two rows at
760 px (REC on its own). Mockup `mockups/mockup-day-line.png`; render `design/render-v1573-face.png` (the day-line state
seeded from the real 09-04 record via `render-face.js --pre`, because the harness holds no save evidence and no nightly log).

## 0h · v15.72 — his three asks on the face, one bug, one read (2026-09-04, night)

Three elements moved or grew, one was repaired, none changed meaning. **The AFTER HOURS · EM EXPIRED bar** left the King
row (`secFrame`) for the bottom of the panel — a full-width bar between the replay strip and the footer (`afterHoursChipHtml`,
the same replay-aware predicate as the branch that retires the band; nothing inside RTH or pre-market). **The King cards**
(`.g3kz`) span their row — they sat as a flex item beside the chip, sized to content, 462 of 649 px measured — and grew: the
price 12 → 16.5 px, the KING titles 6.8 → 8.6, GROWTH / ROLLED 7 → 8.6, the ROLLED / ABOVE-BELOW pills 6.2 → 7.4. **The
ladder** rows 7.6 → 9 px (header 7.3, level rows 8) with the nine columns widened ~15% (`56 68 118 66 94 68 70 70 70`; the
node bar scales to 112); the panel widens itself once (ladderFit) to ~730 px. **The pattern blocks** (RUG / RRUG / PIKA /
BARNEY): the amber sliver inside their left edge was a dead `.g3pb` (pullback) rule's `border-left` leaking into the v15.65
class of the same name — the rule is gone and the block states `border-left:0`; one `.g3pb` rule is pinned. Degrees
unchanged: every element keeps the degree it had. Render: `design/render-v1572-face.png` (the 09-04 replay at 15:00 CT).

**Not on the face, in the record:** his read — *"when the king rolls up and is below price it may be creating a floor
(support) and be bullish and vice versa"* — is the doctrine's rolling floor / ceiling on the King. Every tap is now stamped
with each book's King roll today (`kroll`); four classes in both twins (`king:floor:up` · `king:floor:dn` · `king:ceil:dn` ·
`king:ceil:up`); K2.6 / K2.7 on Analysis (REGISTERED); H8 / H9 in the register, judged by the nightly from the pattern
table (`judge_pat`) once at n = 30 per class against the floor / ceiling base, from the first stamped session (2026-09-08).
Nothing renders on the face until the count clears — degree **doctrine** until then.

## 0g · v15.71 — the save runs itself (2026-09-04, night)

One element changed: the footer's **💾** is a chip. `autosaveState()` → `data-autosave`: **idle** (plain, the session is
running) · **saved** (green — written into the repo folder, by the panel after the close or by his click) · **pending**
(amber — the close has passed, the panel is writing, retried every 10 minutes) · **💾!** (amber — the folder permission
needs one click today, said BEFORE the close) · **💾 DUE** (red, bold — a save is due and the panel cannot write: the
grant is missing after a reload, no folder is picked, or the write failed; "+n" = earlier days waiting too) · **nodata**
(plain — nothing was recorded today). Every hover opens with the question *"Is today's data saved in the repo folder?"*
and ends with how the panel saves on its own. Degree: **descriptive** (a fact about the record, no claim). The pipeline
strip's `saved` dot is unchanged and agrees with the chip (both read `saveState()`); its "not yet" hover now names the
after-close write. There is no longer a manual step at the close; the 💾 is the override.

## 0f · v15.70 — 💡 Rec, the eighth and last tab (2026-09-04, night)

Nothing on the face changed. **💡 Rec** (new, `recBlock`): proposals to him from the nightly (`recommend.py`) and the review
(`rec-seed.py`), each with kind · what it changes · evidence · by · as of; ✓ / ✗ buttons on a proposed row, saved at
once and riding the next day file (`reco`); four sections (awaiting · approved · implemented · declined / withdrawn).
From here on, **nothing on the face changes except through a ✓ on Rec and a build that marks the row IMPLEMENTED** — the
inventory below gains its "knowledge degree" column with the read-recording build (R-1). Reference
`design/render-v1570-rec.png`; mockup `mockups/mockup-recommendations.png`. The eight tabs are the final set.

## 0e · v15.69 — the objective outcomes; the Learn rules carry the record (2026-09-04, night)

Nothing on the face changed. **Testing ⑦**: the nightly's table gains two columns, **turn** (the node WAS the session's
HOD / LOD, within 0.50 SPY) and **resume** (a new session extreme after the tap — stay in), first tap per node per day;
the live table stays held-only and says so. **Analysis rows** the nightly answers read "held · turn · resume". **Learn
②**: under each rule, THE RECORD — the ledger's numbers for the class the rule names and its verdict (agrees green ·
contradicts red · thin / measured blue · not measured, with the reason); the rule's status is untouched. First read
F-19: turn 1 of 19, resume 13 of 19 (2026-09-03, one day).

## 0d · v15.68 — the Analysis tab written by the nightly (2026-09-04, evening)

Nothing on the face changed. **Analysis tab rows** the nightly can answer (the King by book, the King as floor /
ceiling, growing / fading and ±γ at the King, the rugs, the stacks, the register's verdicts — `tools/nightly/results.py`)
now carry the machine's line as their result, tagged **"· by the nightly, <date>"** in blue, with the status the number
earned (READ at n ≥ 15 · THIN · REFUSED · READ NEXT); a row still being counted toward keeps the review's sentence and
shows **"⟳ H2 thin: n=1 of 30 · nightly <date>"** under it. The registry is re-fetched on the panel's 10-minute check.
The ⚙ tab's NIGHTLY box names where the last run happened ("ran on his machine"). The 💾 was the only manual step until v15.71 (§0g).

## 0c · v15.67 — the patterns SCORED; the architecture complete (2026-09-04, evening)

Nothing on the face changed. **Testing ⑦ THE PATTERNS** (new, `patternScoresHtml`): the held rate by setup × book from
the deflection ledger — every new tap is stamped at the moment of the tap with what the PATTERN columns showed at that
strike, per book (`pat`: pika / barney stack named or member, rug / reverse rug, NEW, growth, polarity), plus the
Kings it touched (`kings`, whose join was broken since v15.63 — LESSONS v15.67). Live from this browser's ledger (today
+ the IndexedDB archive) and from the nightly (`learning/log/<day>.json` `patterns`, `tools/nightly/patterns.py`). A rate
prints at n ≥ 15 with its Wilson lower bound; the old detector's names are classes of their own. Reference
`design/render-v1567-patterns.png` (the 2026-09-03 ledger: 53 taps, none stamped yet). **⚙ Architecture ⑥–⑨** (new,
`architectureSystemHtml`): the components, the integrations (Skylit · InsiderFinance · Yahoo · ForexFactory · GitHub),
the HOD/LOD statistics daily pipeline, storage — from `PLAN.system`, the same data as `design/ARCHITECTURE.md`.
Reference `design/render-v1567-architecture.png`.

## 0 · v15.65 — the PATTERN columns (2026-09-04, afternoon)

SETUP became **PATTERN, one column per book (SPX · SPY · QQQ)**: a PIKA / BARNEY block (the word in black on yellow /
purple, the book's own strikes in yellow under it), RUG / RRUG blocks with their two strikes; a SPY / QQQ pattern off
its King sits on the ladder row nearest its converted price (within one SPXW strike). The NOW row is a white hue; the
King rows, chips and strip cells are SPX orange · SPY blue · QQQ cyan. Reference `design/render-v1565-face.png`,
mockup `mockups/mockup-pattern-columns.png`. Everything else below stands.

## 0a · v15.64 — the second conversation (2026-09-04): what the face carries now

Top to bottom, with `CFG.dayRead` / `CFG.ladderGrid` on (both default): the **READ line** — its two facts only
(`LOD IN 99% · HOD after 1:49pm — 80%`; the range clause and the timing prose in its hover) · the **SWEPT line** —
names only, grouped by the side price was working, the latest sweep's side first (`making LOD: POC · VAL · IBL ·
making HOD: PDH`), the name's colour = its state, time/price/status in the hover · the **King strip** (SPY · SPX ·
QQQ: price, `▲ ROLLED UP` / `▼ ROLLED DOWN` over ABOVE/BELOW, GROWTH, ROLLED; the King taps tally in the hover) · the
**grid ladder** — LEVEL · PRICE/STRIKE · NODE · **NEW** (`NEW 9b ×2.3`: observed below the threshold today, crossed
within 20 bars, grew ×2 or +20%) · **⇄ ROLL** · **▲ GROWTH** · **SETUP** (a stack named ONCE on its biggest member,
members ≥ 30% of the King bracketed; rugs on price's side); the NOW row bright, the King rows gold and pulsing (the
gear's motion switch and `prefers-reduced-motion` stop it) · the **replay strip** at the bottom (the NOT RECORDING
warning stays at the top). Off the face since this build: the tally line. Reference `design/render-v1564-face.png`.
Calibration: `tools/study-gridtells.py` (NEW rule C, stacks S6). The three legs no %King threshold catches (7755
09-03, 7705 08-28, 7665 08-31 — 12–18% of a grown King at the tap) are recorded in the constants' comment.

## 0b · v15.63 — what the first conversation changed (2026-09-03/04)

The face was re-decided feature by feature with him; the rows below describe the v15.62 face and stay as the record.
What renders now (`CFG.ladderGrid`, `CFG.dayRead`, both default on): the **King strip** (SPY · SPX · QQQ: price,
ABOVE/BELOW, GROWTH, ROLLED) · the **tally** (King taps per book, counts until n ≥ 15) · the **grid ladder** — LEVEL ·
PRICE/STRIKE · NODE · **NEW** · **⇄ ROLL** · **▲ GROWTH** · **SETUP** (per book, patternpedia colours) with the three
Kings as rows of one zone · the **READ box** (unchanged) · the **SWEPT line** (rates in the hover). Off the face: MARK,
STATE, Δ15m in dollars, ROC, the King tracks, the candle chute, the 22-column DAY table, the taps list. Every one of
those is still computed and recorded; the studies and the Testing tab read them. Reference `design/render-v1563-face.png`.

## 1 · The ladder (`secLoc` → `ladderHtml`) — the v15.62 face, kept behind the toggle

| element | shows | serves | study | status | verdict |
|---|---|---|---|---|---|
| **S / Y King tracks** | where the SPXW / SPY crown has sat today, by price and time | MOVEMENT · HOD/LOD (K4 rolls, K6 extremes) | K4.1–K4.6, K1.3 | descriptive (measured: kingmoves/kingdwell K4.3) | keep; feed K4.1 (437 kingRoll recs unread) |
| **LEVEL** | IB, prior day, the SPY King, IF levels (italic) | HOD/LOD (H2 sweeps), PULLBACK (P5 confluence) | H2.*, P5.2 | descriptive | keep; the sweep read now names which of these were run |
| **PRICE** | the strike on this chart's scale | — (axis) | — | descriptive | keep |
| **NODE · %KING** | dealer exposure at the strike as a share of the King's; colour = polarity | MOVEMENT · F5 magnitude · F6 polarity | F5.1, F5.3, F6.1 (H3 null) | descriptive; the polarity colour implies nothing measured (H3: 52.2 vs 52.1%) | keep the size; **stop implying polarity predicts** — keep the colour as identity only (hover says H3) |
| **context rows** (v15.61, dimmed outline bars) | the next-strongest strikes BELOW the node threshold, drawn only when fewer than 8 nodes clear it, with their real %King | MOVEMENT (where the rest of the book sits around the King) | — | descriptive; display only — not a node, not recorded, not scored | keep; the hover says what it is |
| **NOW** | price and the three crowns | HOD/LOD (K1 which King, F7 trinity) | K1.*, F7.* | descriptive | keep |
| **MARK** | IN PLAY / DEFENDING / BREAKING / ATTRACTING | PULLBACK · HOD/LOD (the tap) | F1.*, F3.* | **unmeasured** — the words assert a state whose hold rate is not on the face | keep the label, add the measured hold rate for that state when F3.1 reads; until then hover says "descriptive" |
| **Δ15m** | dollars gained / lost at the strike over 15 min | MOVEMENT · F4 growth into the tap | F4.1, S6.2 | descriptive (F4: growth mattered, pairing did not — exploratory) | keep; the tap record makes it a measured condition |
| **⇄ rolls** | mass moving strike to strike (source → destination) | MOVEMENT · S5 rolling floors · K4 | S5.1–S5.4, K4.5 | descriptive ("INFERRED from paired changes, never an observed transfer" — the caveat is in the hover) | keep; S5.1 thin (11d) |
| **STATE** | BUILDING / WEAKENING / TURN / SPENT | MOVEMENT · F3 lifecycle | F3.1 | **unmeasured** as a predictor | keep as identity; measure F3.1 from the tap record |
| **ROC 15m** | rate of change, Skylit's own % live | MOVEMENT · F8 velocity | F8.1 | descriptive | keep |
| **EM rails (EH / EL)** | the expected-move band edges | HOD/LOD · H5 range · H2.10k | H5.2, H2.10k | descriptive; **now a sweep level in the READ** (v15.57) | keep |
| **candle chute / open / range marks** | today's price path on the ladder's scale | context | — | descriptive | keep |
| **♛ King pill** | the crown, its moves and dwell | HOD/LOD · K | K4.3 (measured) | measured | keep |
| **regime badge** (posGamma / negGamma) | the sign of the book at spot | X1 regime | X1.1 (rate flat, extent unread) | descriptive; "fade / follow" is doctrine (C30), not yet measured here | keep; label it doctrine until X1.1 reads |
| **IF ladder rows** (CW0 / PW0 / CW / PW / FLIP / Mag / MP\*) | the structure book's levels | HOD/LOD · H2.10f/l, K5.4 gatekeeper | H2.10f, H2.10l (thin, 9 sessions) | descriptive; **now sweep levels in the READ** (v15.56–57) | keep; the book corpus grows per export |

## 2 · ⓪a — the DAY section (`secDay`)

The A row is today (actual), the E row is the expected value from 284 sessions (`BASERATES.json`).

| column | shows | serves | study | status | verdict |
|---|---|---|---|---|---|
| **SLvl / TLvl** | the levels the first extreme's wick took out; the levels the far side targets | HOD/LOD · H2 | H2.* (the sweep read supersedes SLvl for "which level was run") | descriptive | keep SLvl as the record; the READ carries the rate |
| **TIME · TOOK** | when the first extreme printed, how long it took | HOD/LOD · H4 clock | H4.1 (measured: median 33.5 min, n=284) | measured (E row) | keep |
| **BOP · WICK · W.END · WICK%** | the first excursion family | HOD/LOD · H6.3 | H6.3 (measured: median 40 pts, n=252) | measured (E row); A row today | keep |
| **OF BAR** | the extreme's position within its bar | HOD/LOD | — | descriptive | keep, low value; candidate to fold into the hover |
| **MUD / PTMUD** | the leg between the extremes (the "mud" the middle of the day is) | HOD/LOD · H6.2 | H6.2 (gap 229.5 min, n=284) | measured (E row) | keep |
| **PT TOOK · PT · PTWICK** | the far-side (second extreme) timing, points, wick | HOD/LOD · H6, D2.4 | H6.1, D2.4 | measured (E row) | keep |
| **GD/RD** | green or red day | D1.1 | D1.1 (77% one-liner, n=196) | measured | keep |
| **PTN** | (the far-side extreme's position) | HOD/LOD | — | descriptive | keep, low value |
| **HL GAP · HL RNG · HL $** | time between the extremes, the range in points and dollars | HOD/LOD · H5.1, H6.2 | H5.1 (61.4 pts, $3,072, n=284) | measured | keep — this IS the prize the objective names |
| **LC GAP · LC RNG** | the last extreme to the close | HOD/LOD · K3.2 / K6.3 (pin) | K3.2, K6.3 (thin) | descriptive today | keep; K6.3 measures the pin |
| **EFF** | efficiency of the day's travel | D1.3 day type | D1.3 (thin) | descriptive | keep |
| **BODY** | the day's body vs range | D1 | — | descriptive | candidate to fold |
| **the READ box** (HOD IN x% · LOD after …) | is the standing extreme the day's — the posr × clock cell | HOD/LOD · H1.1 | H1.1 (AUC 0.879, NOT-IN 85% n=230) | **measured, calibrated live since v15.51** | keep — the objective's primary instrument |
| **the far-side line** (one-sided floor, middle half, the NO call) | where and when the other extreme can print | HOD/LOD · H6.4, D2.4 | F-14/15 (197 sessions) | measured, PROVISIONAL (one instrument, no forward test) | keep; add the forward score (stage ⑪) |
| **deflections today** (`.g3dfl`) | one row per fresh tap: CONTINUED / STALLED / pending | PULLBACK · HOD/LOD · F1.2 | F1.2 (engine), F2.1 (H2), H1.3 (H5) | descriptive per event; the ledger H5 waits on | keep; label each row with the H2 tap number |
| **THE READ FROM THE STATS** (v15.55–58) | today's sweeps against the tables, the node clause, the register's word | HOD/LOD · H2, H6; PULLBACK (VWAP/value levels named as pullback candidates) | H2.1–H2.12, H6, F2.1 | **measured with n; node-conditioned = thin (9 sessions)** | keep; **score it nightly (stage ⑪)** |

## 3 · Footer and chrome

| element | shows | verdict |
|---|---|---|
| pipeline dots (rec · saved · pushed · review · deps) | the loop's stages ①–⑤ as dots | keep; the ⚙ Architecture tab (v15.59) is the expansion; the Analysis / Testing / Learn tabs repeat rec · saved · pushed in their foot (v15.62) |
| version, 💾 chip, folder | the build, whether the day is saved (v15.71: idle · saved · pending · ! · DUE · nodata — the panel saves itself; the click is the override and carries the folder permission), the repo folder | keep; descriptive |
| step tips (hover doctrine) | the five-step execution doctrine as hovers | keep (doctrine, C-numbered) |

## 4 · What the inventory says

1. **Nothing on the face contradicts the objective**, and the two instruments that carry the objective
   directly — the READ box (H1.1, calibrated) and THE READ FROM THE STATS (H2 with n) — are measured.
2. **Three elements imply a claim they cannot back**: the MARK words, the STATE words, and the polarity
   colour. None is wrong as a *description*; each reads as a *prediction*. Until F3.1 / F1.* read from
   the tap record, their hovers should say "descriptive — hold rate unmeasured (F3.1)"; when the record
   fills, the measured rate joins the word. (Build: hover text only, v15.59.)
3. **Four columns are low-value** (OF BAR ×2, PTN, BODY): candidates to fold into hovers to buy width
   for THE READ. His call.
4. **The face manifest**: every element above gets a `face` entry in `studies.json` naming the element
   and the study; `test_face_manifest.js` pins that every rendered label with a number has one. Then
   "data-driven" is enforced, not asserted: a number on the face without a study fails the suite.

## 5 · How the three tabs work together (the integration he asked for)

```
ANALYSIS  a study reads → its result (with n) lands in studies.json → the row shows READ
    ↓ the review turns a READ into a hypothesis (predict + refuteIf)
TESTING   ① the register holds it → the nightly judges it ONCE at minN → ② the gate checks the scorer
    ↓ cleared + earned tier
DASHBOARD the element that maps to that study renders its rate (with n); THE READ quotes it at the tap
    ↓ stage ⑪ (next): what the READ said is written to the day file and scored the next night
ANALYSIS  the score comes back as the study's live column (like H1.1's live-vs-table cells)
```

What is wired today: Analysis ↔ registry ↔ Testing ① (same ids), Testing ② → the ladder's grades
(gated/thin), the tables → THE READ, the register's verdicts → the READ's register line, the TRACK
field → the export → the nightly → requests.json. **What is not wired yet:** the READ's own score
(⑪), the face manifest (§4), the tap record (94 OPEN studies).
