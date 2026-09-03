# The Analysis and Testing tabs, by subject — design (2026-09-03)

**Decision (his):** of the five alternatives, *A · by subject* — "it provides a subject which can then
be added to." This document is the design that follows from it, and it supersedes Part 4 of
`SKYLIT-DOCTRINE-CLAIMS-AND-ANALYSIS-DESIGN.md` (the old S-A/S-B/S-C/S-D ids are carried as `was`).

**North star (his words):** identify the high and low of the day, and the pullback deflections, using
gamma nodes — to trade the move. *"We don't want information for the sake of information."* Every
study below is therefore a question whose answer changes an action at the tap, and carries that
action as a field: **SIZE · SIDE · TARGET · STOP · SKIP · TIME · LEVEL · WAIT**.

## 1 · The taxonomy

Seven subjects, 45 subsections, 170 studies — `learning/studies.json`, written by
`tools/studies-seed.py`, rendered by `tools/mockup-from-studies.py`. The mockup and the registry are
the same file; they cannot drift.

| subject | subsections | what a trader asks |
|---|---|---|
| **K · KINGS** | K1 by book (SPX/SPY/QQQ King) · K2 by role (floor/ceiling/pin) · K3 by the clock · K4 rolls · K5 quality · K6 the extremes | which King, doing what, when — and does it print the extreme |
| **S · SETUPS** | S1 rug/reverse rug · S2 gatekeeper rejection · S3 beach ball · S4 air pocket · S5 rolling floors · S6 new node / fed node / moved node · S7 pika cloud · S8 unwinding, bleed, decoys | the patternpedia as trades: trigger, invalidation, target |
| **D · DIRECTION** | D1 the day's direction · D2 after the deflection · D3 King and spot · D4 structure → direction · D5 cross-book lead | which way after the turn, how far |
| **F · DEFLECTION MECHANICS** | F1 zone & trigger · F2 tap decay · F3 lifecycle · F4 growth into the tap · F5 magnitude & rank · F6 polarity & regime · F7 trinity · F8 velocity | what a tap is, what makes it hold — the engine both objectives run on |
| **P · PULLBACK DEFLECTIONS** | P1 which node holds · P2 depth · P3 timing · P4 node condition · P5 confluence · P6 deflection or reversal · P7 payoff | inside a move, will this retracement turn here |
| **H · HOD / LOD** | H1 is the extreme in · **H2 sweeps (12 studies)** · H3 which node printed it · H4 the clock of extremes · H5 range & EM · H6 the other side · H7 event days | is the extreme in, what printed it, what the other side pays |
| **X · CONTEXT** | X1 regime · X2 calendar · X3 volatility · X4 the three books | the modifiers every row above is split by |

Status vocabulary, in order of maturity: OPEN (needs the TAP record / API backfill) → READ NEXT (data
on hand, unread) → THIN → READ (first read, exploratory) → DRAFT (register entry proposed) → REGISTERED
/ BLOCKED → SHIPPED (measured, on the tab) — and REFUSED, which stays visible. Today: 93 open, 17
read next, 22 thin, 15 read, 13 shipped, 4 registered, 1 blocked, 2 draft, 3 refused.

**Every result carries its n.** Checked by the seed script (a `%` in `result` without `n=` fails) and
by the Playwright render check.

## 2 · What was measured today — the sweeps (H2)

`tools/study-sweeps.py` on the 284-session ES file (full Globex, so ONH/ONL come from the bars).
Findings in `roadmap/FINDINGS-sweeps-first-read.md`. In one line: **the level's name does not matter
(ON, PD, IB ≈ a bounce off any fresh low); the clock, the depth and the speed of the reclaim do;
the node is unmeasured and is H6.** The sweep table renders inside H2 on the tab, from `SWEEPS.json`,
refreshed by the nightly.

## 3 · The flow, and the Testing tab

```
ANALYSIS (studies.json)  →  REGISTER (register.json)  →  GATE (featGated)  →  DASHBOARD (rules.json tiers)
        ↑                                                                              ↓
   NIGHTLY (run.py: verdicts · SWEEPS/BASERATES refresh · read-next · digest)  ←  RECORD (feat · defl · TAP)
```

- **Fed by Analysis.** A study reaches READ (a number, its n, a control). Testing turns it into a
  register row — `predict` and `refuteIf` fixed, `minN` set — read once on sessions the read never
  saw. Today: H6 (sweep × node, minN 40, predict > 40% vs base 24% n=453, refute ≤ 30%) and H7 (the
  early sweep, minN 60, predict > 24%, refute ≤ 18%, sessions after 2026-08-21 only). Drafts until the
  build writes them.
- **Feeds the Dashboard.** The ladder renders a rate only when the rule has an earned tier *and* its
  feature cleared the gate (≥ 30 per band, ≥ 10 points between the bands). Otherwise "⛔ gated" or
  "thin" — never a bare %. Today: 75 hand rules, 0 earned, three features at the gate.
- **The flag.** `kill.negGammaWide` (a hand rule that kills on −γ wide) sits against a registered null
  (H3: polarity does not discriminate, 52.2% vs 52.1%, n=46/48). When H3 clears at 40 the rule
  retires; if H3 is refused it stays. This is the loop changing the ladder.

Testing tab sections (replacing T_canfail · T_prereg · T_prop · T_chal · T_kill · T_cov · T_self ·
T_detail): **⓪ the loop strip · ① THE REGISTER by subject · ② THE GATE · ③ ON THE DASHBOARD (each
ladder element → the study its number comes from) · ④ THE RECORD (stores, fields present, fields the
OPEN studies need) · ⑤ THE NIGHTLY (last run, reads next, read-next queue, ledger, LLM review handoff)
· ⑥ THE SUITE.** Same subject strip as Analysis; ids shared.

Mockups: `design/mockup-analysis-by-subject(-standalone).html`, `design/mockup-testing-tab(-standalone).html`.

## 4 · The build — v15.55 (SHIPPED 2026-09-03) and v15.56

**v15.55 shipped:** ② the Analysis tab from `studies.json` (subject strip, subsections, rows, result
lines, live evidence under H1/F1/F5/D2, the H2 sweep table from `SWEEPS.json`); ③ the Testing tab in loop
order with the `kill.negGammaWide` flag; ④ the nightly refreshes `SWEEPS.json`, judges H6/H7 through
`study-sweeps`, and copies TRACK requests into `learning/requests.json`; ⑤ THE READ FROM THE STATS on ⓪a
(one line per excursion, one per side, the latest break, the node clause, the register's word); ⑥
`test_v1555.js` (68, 12/12 mutations). **The TRACK field** (his ask, added the same day): one per subject;
the request rides in the day export; the review answers with a study row carrying `req:<id>`.

**v15.56 shipped (his ask — "can you add them"):** the book's levels as sweep levels (CW0/PW0/CW/PW/KING, side
by position, `bookLevelsNow` → ES points), the node clause against the King / top-5 / walls inside ±0.50 SPY, the
book table from the day files (`study-sweeps-book.py` → `SWEEPS-BOOK.json`, H2 block, H6 judged from it), the
honest overnight (`full`, PMH/PML for a stub; companion v1.18 keeps the whole Globex day for ES), `RATE_MIN_N=15`.

**v15.57 shipped (his approval of the five):** EM edges, VWAP + bands, developing POC/VAH/VAL, London, HVL/magnet in
the read; `LEVEL_TIER` and the two-line rule; the corpus at 32 level types (F-16: interior levels are pullback
candidates, not extremes — P5.1 measures resume).

**v15.58 — next:**
1. **TAP record** (`recordTap`, IDB `taps`, `wasSessionExtreme` labelled at 15:00 CT) — the record
   95 studies wait on. Fields per Part 3 of the doctrine doc: identity · node · lifecycle · growth ·
   structure · configuration · trinity · both zones · extent outcomes. Unblocks H6.
2. **The nightly reads one READ NEXT study per night** in the registry's order (K4.1, S2.1, K1.3 …) and
   writes its result back into `studies.json` (status READ) — the registry as the nightly's output.
3. **TRACK → study** as a nightly step: a NEW request becomes a DRAFT study row under its subject with a
   proposed measure, for the review to accept or reword.
4. ~~Sweep × book levels~~ — shipped in v15.56 from the day files; the tap record adds the per-tap fields.

## 5 · Decisions that are his

(a) Skylit API key / credit budget for the historical backfill — it turns 93 OPEN studies into
readable ones in an afternoon. (b) The first READ NEXT to run (my order: K4.1 the King roll, S2.1 the
early gatekeeper, K1.3 which King leads). (c) The tap-zone definition (Skylit's ±0.50/±5 vs the ATR
band) — F1.1 measures both; one must be primary for the record. (d) Whether NQ 1-minute joins the ES
file for D5 (the cross-book lead) — the loader already sniffs both formats.
