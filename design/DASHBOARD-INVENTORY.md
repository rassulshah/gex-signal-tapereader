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

## 1 · The ladder (`secLoc` → `ladderHtml`)

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
| version, Save, folder | the build, the export, the repo folder | keep |
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
