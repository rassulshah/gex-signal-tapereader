#!/usr/bin/env python3
"""
THE DEFLECTION LEARNING DOC — learning/deflections/examples.json (the data) and LEARNING.md (the doc).  (v15.62)

    "I also want a learn tab. I will be giving you screenshots in a way training you and you will put it into a
     deflection learning doc, which will be part of your training to get better at identifying deflections and you
     will put it in the learning tab, always updating it when i give you examples. and learning until you get really
     good at identifying deflections."                                                       - operator, 2026-09-03

ONE SOURCE, THREE PLACES. This file holds the examples, the rules and the protocol; running it writes
learning/deflections/examples.json (what the 📚 Learn tab renders) and learning/deflections/LEARNING.md (what a
context reads on load — the training). LEARN_SEED in the userscript is a copy of the json for the first render;
test_v1562.js pins seed == file, doc ids == json ids, and every example's image on disk.

THE PROTOCOL (the honest version of "training"):
  1. He gives a screenshot. BEFORE he says what it was, I write my BLIND CALL: kind (HOD/LOD turn · PULLBACK turn ·
     NONE), side, the node I think caused it, the factors I see (new node · growth into the tap · roll · rug /
     reverse rug · stack · King), confidence. That is `blind` on the example, with `blind.right` filled in after.
  2. He gives the answer (or gave it with the screenshot — then the example is TAUGHT, unscored: `blind` is null).
  3. I pull the panel's own node-event record for those strikes and times (gpts_nodeevents_v1 / the day file) and
     write what the DATA says (`data`), agreeing or disagreeing with the picture, with numbers.
  4. The example produces or touches a rule (L-n). A rule from one example is PROPOSED (n=1). It becomes CONFIRMED
     when three examples agree and none refutes; REFUTED when an example contradicts it — the rule stays, marked.
  5. The score is blind reads only: right / n, by kind. Under RATE_MIN_N it prints thin, like every rate here.

Edit HERE (tools/learn-add.py appends an example interactively), run this script, re-splice LEARN_SEED, build.
"""
import json, io, os

RATE_MIN_N = 15

FACTORS = [
    {"id": "new",    "name": "NEW NODE",            "what": "a strike OBSERVED below the 20% threshold earlier today that crossed it within the last 20 bars AND GREW — ×2 its size at the crossing, or +20% of itself over the growth window (v15.64; the panel's NEW chip). First sight is never a birth: the opening book and the ladder after a reload are not new. His words: 'when price is going to a level, a new node pops up and deflects price. It grew rapidly for this purpose.'"},
    {"id": "growth", "name": "GROWTH INTO THE TAP", "what": "the node's $ exposure rising in the 15 minutes before price reached it (the recorder's d15 > 0, the chart's marker getting brighter)"},
    {"id": "king",   "name": "KING DEFLECTION",     "what": "the tap is of THE King — and which book's: SPX (SPXW, the flow book), SPY, or QQQ. His bread-and-butter setup (studies S0.1–S0.7); tracked per book, religiously"},
    {"id": "heavy",  "name": "HEAVY",               "what": "a node ≥ 50% of the King that is not the King"},
    {"id": "roll",   "name": "ROLL",                "what": "mass moving strike to strike toward or away from price (the ⇄ column; ROLL events)"},
    {"id": "rug",    "name": "RUG / REVERSE RUG",   "what": "the patternpedia's rug setup: a yellow (+γ) node stacked above a purple (−γ) node with no floor in sight — when the yellow unwinds the drop is violent; the reverse rug is the mirror (purple above yellow, no ceiling). Called per book: SPX · SPY · QQQ"},
    {"id": "stack",  "name": "PIKA / BARNEY STACK",  "what": "a cluster of LARGE same-sign nodes on adjacent strikes acting as one wall — each member ≥ 30% of the King, the biggest ≥ 40%, named once on the biggest (v15.64) — PIKA when they are yellow (+γ), BARNEY when purple (−γ); called per book: SPX · SPY · QQQ. The docs: 'magnitude matters most here: thin cloud = soft/porous; dense king-level cloud can pin all session.'"},
    {"id": "magnet", "name": "MAGNET PULL",         "what": "a node growing hard while price sits away from it — price is drawn back to it"},
    {"id": "side",   "name": "SIDE FLIP",           "what": "a node that was resistance becoming support once price is above it (or the reverse)"},
]

RULES = [
    {"id": "L1", "rule": "A deflection comes off a node that is GROWING INTO THE TAP. Read the growth in the 15 minutes before the tap (d15), not the size alone; the sign of the gamma did not matter to whether it deflected.",
     "from": ["E001", "E002", "E003", "E004"], "status": "CONFIRMED", "n": 15, "agree": 13, "weak": 2, "refute": 0,
     "note": "13 of 15 measurable legs (4 legs fell in blind windows). Weak: E002 c2 (7670 −γ roughly flat, −13M → −17M) and E003 c2 (7775 grew +13–15M/15m until 09:57, then d15 +3 → 0 AT the high: the growth stalled at the tap). +γ and −γ nodes both deflected (E001 +γ King; E002 −γ King; E004 −γ King at 10:16 and +γ stack at 12:10)."},
    {"id": "L2", "rule": "A FRESH node at the extreme — just crossed the threshold and growing — is a turning-point candidate: the high or low of the leg prints against it.",
     "from": ["E001", "E002", "E003", "E004"], "status": "CONFIRMED", "n": 6, "agree": 6, "weak": 0, "refute": 0,
     "note": "E001 R3 (7755 born 12:48, the session high); E002 c1 (7665/7670 born 09:21–09:27 −γ, the session low); E003 c2 (7775 born 09:33, the session high), c4 (7720 born 12:39, the 13:12 high), c5 (7705 born 13:42 −γ, the 14:00 low); E004 c1 (7700 fresh 08:45–08:57, the morning support)."},
    {"id": "L3", "rule": "The King growing hard while price sits away from it PULLS price back to it (magnet) — as first written, without a sign condition.",
     "from": ["E001", "E002", "E003", "E004"], "status": "REFUTED", "n": 4, "agree": 3, "weak": 0, "refute": 1,
     "note": "Held for three +γ Kings — E001 (King $237M → $345M with price above; price back on it by 14:23), E003 c3 (the King rolled DOWN 7790 → 7715 onto price; price rose to it and traded on it 12:36–13:12), E004 c3 (after the 7755 high price returned to the King 7735 by 13:00) — and FAILED for the −γ King of E002 c4 (7675 grew −27M → −116M under price and price rallied AWAY into the 14:59 high). The rule stays here refuted; L8 is the conditioned version and waits for a new example."},
    {"id": "L8", "rule": "A +γ King growing hard while price sits away from it pulls price back to it (magnet): price above a +γ King adding $50–90M per 15 min is a pullback-to-the-King candidate, not a breakout. (L3 with the sign condition the refutation forced.)",
     "from": ["E001", "E003", "E004"], "status": "PROPOSED", "n": 3, "agree": 3, "weak": 0, "refute": 0,
     "note": "Born from L3's refutation, so by the protocol it does not count as confirmed until a NEW example agrees — a rule rewritten to fit its counter-example has not been tested yet. Register candidate H8 is the corpus version."},
    {"id": "L4", "rule": "A node's SIDE FLIPS when price crosses it: the earlier resistance node re-growing under price is the pullback support. Look for the re-growth (a TURN_UP after a decay), not for a new strike.",
     "from": ["E001", "E002"], "status": "PROPOSED", "n": 2, "agree": 2, "weak": 0, "refute": 0,
     "note": "E001 Pb (7740: 80% resistance at 10:16 → decayed → support from 10:42 → re-grew from 11:15 → held 12:36); E002 c4 (7675 pierced at 13:39 'above' then reclaimed 'below', then the King)."},
    {"id": "L5", "rule": "A −γ node growing next to price ACCELERATES price away from it — a −γ node under price is a launch pad, above price a lid; it does not hold price the way a +γ node does.",
     "from": ["E002", "E003", "E004"], "status": "PROPOSED", "n": 5, "agree": 4, "weak": 1, "refute": 0,
     "note": "E002 c2 and c4 (−γ under price → up), E003 c5 (−γ 7705 under the 14:00 low → up), E004 c2 (−γ King 7735 above the 10:16 high → down). Unclear: E003 c3 (−γ 7710 at ES 7718 grew after the 12:06 low while price sat on it — the +γ King above was pulling)."},
    {"id": "L6", "rule": "A STACK — two nodes within ~5 ES points, both growing — marks the extreme of the leg more reliably than a single node.",
     "from": ["E001", "E002", "E004"], "status": "CONFIRMED", "n": 3, "agree": 3, "weak": 0, "refute": 0,
     "note": "E001 Pb (7735 + 7740, $61M + $81M), E002 c1 (7665 + 7670 + 7675 −γ, the session low), E004 c3 (7745 + 7750, $73M + $46M, the session high 5 pts between them)."},
    {"id": "L9", "rule": "THE KING DEFLECTION is the bread-and-butter setup: a tap of the King — SPX, SPY or QQQ King, each its own row — is the base case every other setup is measured against. Track every King tap with its book, its growth into the tap (d15), its sign, and the outcome.",
     "from": ["E001", "E002", "E003", "E004"], "status": "PROPOSED", "n": 6, "agree": 6, "weak": 0, "refute": 0,
     "note": "His words, 2026-09-03: 'this is a bread and butter setup … track this religiously.' In the four taught days 6 of 16 circled legs were taps of the King and all six deflected (E001 R1, R2 — the SPX King 7750; E002 c4 — 7675 as it became the King; E003 c1 — 7750 at the open, c3 — the King rolled onto price; E004 c2 — the −γ King 7735). That is selection, not a rate: he circled deflections, not every tap. The rate comes from the ledger (S0.1–S0.4), per book, at n ≥ 15."},
    {"id": "L7", "rule": "TIME OF DAY: the taught deflections cluster in the first hour and in the noon window (12:00–13:15 CT) — 3 and 7 of 16 legs. An observation to measure on the 284-session corpus, not a rule to trade.",
     "from": ["E001", "E002", "E003", "E004"], "status": "PROPOSED", "n": 16, "agree": 10, "weak": 6, "refute": 0,
     "note": "First hour: E002 c1 09:27, E003 c1 08:42–09:00, E004 c1 08:45–09:10. Noon: E001 R2 11:57–12:06, Pb 12:20–12:36, R3 13:12; E002 c3 12:51–13:24; E003 c3 12:06, c4 13:12; E004 c3 12:10. The sweep corpus already says the first 30 minutes matter (27% vs 18%, n=180)."},
]

EXAMPLES = [
    {
        "id": "E001", "date": "2026-09-03", "chart": "ES1 · Skylit · 3-min · 09:52–15:00 CT", "tz": "CT",
        "img": "learning/deflections/img/E001.png",
        "given": "2026-09-03 15:2x CT, with the answer — TAUGHT, not blind",
        "coverage": "node events 08:39–15:00 CT (a blind gap 13:36–14:23); the picture and the record agree bar for bar",
        "his": "in this example you see price going the first resistance deflection, then pulling back and retesting it two times after which it went down to test a pb support node that created a deflection that eventually went to the third resistance deflection node at the high. can you see it in the picture and the data. learn from this example today. … the resistance nodes show they got brighter meaning they were growing when they created resistance. the pullback support node shows that it was new node growth that created the pullback that came out of nowhere just before 11 am. pay attention to these new nodes that come out of nowhere. the way i see it, typically new gamma node growth, increasing gamma growth and gamma rolling up or down are very important and may cause deflections as well as push and pull price.",
        "legs": [
            {"leg": "R1", "kind": "PULLBACK", "side": "resistance", "when": "10:42–11:12 CT", "px": "ES 7755–7758", "node": "SPXW 7750 (the King, ES 7758)",
             "data": "10:00 56% $17M decaying → 10:06 King $40M → 10:18 $77M → 10:33 $101M → $116M at 10:42, the first tap bar; d15 +$40–54M. Two taps, held.", "factors": ["king", "growth"]},
            {"leg": "R2", "kind": "PULLBACK", "side": "resistance", "when": "11:57–12:06 CT", "px": "ES 7757–7758", "node": "SPXW 7750 (the King)",
             "data": "$180–204M, still +$20–30M/15m through the retests. Held.", "factors": ["king", "growth"]},
            {"leg": "Pb", "kind": "PULLBACK", "side": "support", "when": "12:20–12:36 CT", "px": "low ES 7744–7746", "node": "SPXW 7740 (ES 7748) + 7735 (ES 7743)",
             "data": "7740: $57M at 12:00 → $72M 12:30 → $81M at 12:36 (the tap bar) → $90–94M by 12:51, d15 +$13–21M into the tap; 7735 $47M → $61M into 12:09. NOT born out of nowhere in the record: 7740 was an 80% resistance node at 10:16, decayed to ~40% by 10:33, flipped to support at 10:42, and re-grew from a TURN_UP at 11:15 — the re-growth is what looked new. (His 'just before 11 am' does not match the record; asked which node he meant.)", "factors": ["growth", "side", "stack"]},
            {"leg": "R3", "kind": "HOD", "side": "resistance", "when": "13:12–13:33 CT", "px": "ES 7763–7765 (the session high)", "node": "SPXW 7755 (ES 7763) — fresh — over the King 7750",
             "data": "7755 crossed the 20% threshold at 12:48 ($49–56M, d15 +$11–12M): new on the ladder, and the high printed at it. Under it the King went $237M → $345M between 13:00 and 13:33 (+$37…+92M/15m) with price above it; price was back at 7762 → 7756 by 14:23 ($568M).", "factors": ["new", "growth", "magnet", "heavy"]}
        ],
        "call": {"kind": "PULLBACK ×3 then HOD", "summary": "three deflections and a pullback, every one off a node growing into the tap; the HOD at a fresh node with the King pulling from below"},
        "blind": None,
        "rules": ["L1", "L2", "L3", "L4", "L6", "L8", "L9"],
        "open": "which node he meant by 'new node growth just before 11 am' — the record shows 7740 re-growing from 11:15, not a birth before 11:00"
    },
    {
        "id": "E002", "date": "2026-08-31", "chart": "ES1 · Skylit · 3-min · Aug 31 08:30 → Sep 1 (the PDH/PDC/PDL lines are Aug 31's own levels, drawn from Sep 1)", "tz": "CT",
        "img": "learning/deflections/img/E002.png",
        "given": "2026-09-03, with the answer ('3 at a support with one at resistance'; 'i think it is from the 31st') — TAUGHT",
        "coverage": "snaps 08:36–15:00 CT (the book per 3-min bar); node events only from 10:39; the courier's ES minute bars. Date confirmed Aug 31: RTH O 7701.25 · H 7708.25 (14:59) · L 7674.75 (09:27) · C 7697.75 = the chart's PDH 7708 / PDC 7698 / PDL 7674.",
        "his": "see these examples, remember each circle count as 1 deflection. this is from another day, i think it is from the 31st. double check and confirm, but most importantly you must learn so you can identify, qualify and understand and predict a deflection. here are 4 examples. 3 at a support with one at resistance.",
        "legs": [
            {"leg": "c1", "kind": "LOD", "side": "support", "when": "09:24–09:33 CT", "px": "low ES 7674.75 at 09:27 (the session low)", "node": "SPXW 7665 (ES 7673) + 7670 (ES 7678) + 7675 (ES 7683), all −γ",
             "data": "A fresh −γ stack under price: 7670 born 09:21 (−$6M → −$8M at 09:27, d15 −3), 7665 born 09:27 (−$5M), 7675 −$15M → −$20M (50% of the King) at 09:27, d15 −2. The King was 7700 / 7650 flipping. Bounce to 7688 by 09:42.", "factors": ["new", "growth", "stack"]},
            {"leg": "c2", "kind": "PULLBACK", "side": "support", "when": "11:00–11:09 CT", "px": "low ES 7677.25 at 11:06", "node": "SPXW 7670 (ES 7678, −γ)",
             "data": "7670: −$13M (11:00) → −$17M (11:03, 48%) → −$14M (11:06, side 'above' = price pierced it) → −$11M (11:09) → reclaimed ('below') at 11:12. Growth into the tap was weak (d15 −1 … −2); the node shrank as price pierced it and price came back up through it. The weakest of the four.", "factors": ["side"]},
            {"leg": "c3", "kind": "PULLBACK", "side": "resistance", "when": "12:51–13:24 CT", "px": "high ES 7694–7695.75 (his circle sits on the 7698 PDC line — the close, drawn in hindsight)", "node": "SPXW 7685 (ES 7693) + 7690 (ES 7698) + 7695 (ES 7703), +γ",
             "data": "7685 grew $12M → $23M into the tap (d15 +12 … +15M at 13:03–13:15, 16% → 21%); 7695 $33–39M flat; 7690 $21–28M slow. Price held 7692–7696 for 30 minutes and fell to 7680 by 13:42.", "factors": ["growth", "stack"]},
            {"leg": "c4", "kind": "PULLBACK", "side": "support", "when": "13:36–13:51 CT", "px": "lows ES 7679.75–7680 (13:42–13:51)", "node": "SPXW 7675 (ES 7683, −γ) — became the King at 13:39 — + 7670 (ES 7678)",
             "data": "7675: −$27M (13:33, 52%) → −$39M King (13:39) → −$58M (13:51) → −$65M (13:57) → −$116M by 14:33, d15 −$24 … −48M: the biggest growth of the day, under price; 7670 −$21M → −$40M (88–90%) 13:39–14:06. Price pierced 7683 ('above' at 13:39–13:48), reclaimed, then rallied AWAY from the growing −γ King into the 14:59 high 7708 — the −γ King did not pull price back (L3's sign condition).", "factors": ["king", "growth", "stack", "side"]}
        ],
        "call": {"kind": "LOD then PULLBACK ×3", "summary": "the LOD at a fresh −γ stack; two supports and one resistance off growing nodes; the afternoon −γ King grew under price and price ran away from it"},
        "blind": None,
        "rules": ["L1", "L2", "L3", "L4", "L5", "L6", "L9"],
        "open": ""
    },
    {
        "id": "E003", "date": "2026-08-28", "chart": "ES1 · Skylit · 3-min · Aug 28 (the levels PDH 7755.5 · PDC 7741.25 · PDL 7702.75 are Aug 27's; IB 7743.75–7756.25)", "tz": "CT",
        "img": "learning/deflections/img/E003.png",
        "given": "2026-09-03, with the circles (5) — TAUGHT",
        "coverage": "snaps 08:36–10:03 and 12:12–14:57 CT; node events 09:09–10:06 and 12:16–15:00 — the panel was blind 10:06–12:12. Date found from the levels: Aug 27 H 7755.5 · L 7702.75 · C 7741.25 → the chart is Aug 28: O 7746.25 · H 7782.5 (10:01) · L 7711.75 (12:10) · C 7722.",
        "his": "(five circles, no words)",
        "legs": [
            {"leg": "c1", "kind": "PULLBACK", "side": "resistance", "when": "08:42–09:00 CT", "px": "ES 7757–7760.5", "node": "SPXW 7750 (the King, ES 7758)",
             "data": "The King at the open (08:36 7750; briefly 7790 at 08:39; 7750 again 09:00). At 09:09, the first event: 93–100%, $14M, stage Delivered, 3 taps — the circle IS those three taps. Growth into the taps is in the blind minutes before 09:09; from 09:09 it grew $14M → $34M by 09:48 while price fell to 7732 and came back. Price went through it at 09:42 (side 'below' from 09:45) and never returned.", "factors": ["king"]},
            {"leg": "c2", "kind": "HOD", "side": "resistance", "when": "10:00 and 10:24–10:30 CT", "px": "high ES 7782.25 at 10:00, 7779.5 at 10:30 (the session high)", "node": "SPXW 7775 (ES 7783) fresh; the King had rolled UP to 7790 (ES 7798)",
             "data": "7775 born 09:33 (8%) → $9M → $25M by 09:57 (+$13–15M/15m, 46–60% of the King) and STOPPED at the high (10:00 d15 +3, 10:06 +0; 37% as the King outgrew it). The King rolled 7750 → 7790 at 09:51–09:54 ($39M → $57M, +$24–34M/15m), 15 pts above price. The high printed at the fresh node whose growth stalled, under a King price never reached. Then blind until 12:12.", "factors": ["new", "growth", "roll"]},
            {"leg": "c3", "kind": "PULLBACK", "side": "support", "when": "12:00–12:30 CT", "px": "low ES 7714.25 at 12:06", "node": "the King rolled DOWN 7790 → 7715 (ES 7723) onto price; 7710 (ES 7718, −γ) grew after",
             "data": "At 12:12 the King was 7715 at $52M (from 7790 during the blind gap) — 9 pts above the low. 7705 (ES 7713) was not on the board (not in the top 90) until 13:42; 7700 (ES 7708) +$9M small; 7710 (−γ) −$7M → −$19M over 12:18–12:42. The bounce went straight to the King's price: 12:36 7722–7723.5, then 7722–7726 for 40 minutes ON it — pulled up to the King (L3).", "factors": ["king", "roll", "magnet"]},
            {"leg": "c4", "kind": "PULLBACK", "side": "resistance", "when": "12:54–13:18 CT", "px": "high ES 7726.5 at 13:12 (at the SMA 50)", "node": "SPXW 7720 (ES 7728) fresh, over the King 7715 (ES 7723)",
             "data": "7720 born 12:39 (13%) → $13M (12:45, 20%) → $24M (13:15, 45%) → $29M (13:27, 54%), d15 +$9–11M into the tap; the King under it was DECAYING 13:03–13:21 (d15 −7 … −15M). Price fell from 7726 to 7714 by 14:00.", "factors": ["new", "growth"]},
            {"leg": "c5", "kind": "PULLBACK", "side": "support", "when": "13:54–14:06 CT", "px": "low ES 7713.75 at 14:00", "node": "SPXW 7705 (ES 7713, −γ) fresh; 7710 (ES 7718, −γ) dissipating above it",
             "data": "7705 appeared 13:42 (−$18M, 31%) → −$24M (13:45, 38%, d15 −14M) → −$20M (13:48); 7710 above it went −$36M (13:42) → −$10M (13:51) — dissipating as price fell into it; the King flickered to 7690 (13:48–13:51) then back to 7715. Bounce to 7722–7726 by 14:30–14:42 — back to the King again.", "factors": ["new", "growth", "rug", "magnet"]}
        ],
        "call": {"kind": "PULLBACK, HOD, PULLBACK ×3", "summary": "the high at a fresh node whose growth stalled under a King that had rolled up; the two afternoon lows at fresh −γ nodes with the King (rolled down onto price) pulling price back up each time"},
        "blind": None,
        "rules": ["L1", "L2", "L3", "L5", "L8", "L9"],
        "open": "the 10:06–12:12 gap hides how the King rolled from 7790 to 7715 — the roll itself may be the tell for the 12:06 low"
    },
    {
        "id": "E004", "date": "2026-08-27", "chart": "ES1 · Skylit · 3-min · Aug 27 08:30–13:15 CT (levels PDH 7705.5 · PDC 7692 · PDL 7671 are Aug 26's; IB 7702.75–7728.5)", "tz": "CT",
        "img": "learning/deflections/img/E004.png",
        "given": "2026-09-03, with the circles (3) — TAUGHT; 'I think you have enough to start with and analyze for now'",
        "coverage": "snaps 08:36–15:00 CT; node events 09:24–15:00; ES minute bars from the 08-28 day file. Aug 27: O 7716.25 · H 7755.5 (12:10) · L 7702.75 (08:35) · C 7741.25.",
        "his": "put an indicator on the learning tab like a scale from 0 to 100 which will measure your learning progress and how good you have become in identifying deflections and becoming a deflection expert with the ability to identify deflections and even predict a deflection will occur once you see price is going to the node.",
        "legs": [
            {"leg": "c1", "kind": "PULLBACK", "side": "support", "when": "08:45–09:10 CT", "px": "ES 7708–7714 (after the 08:35 low 7702.75)", "node": "SPXW 7700 (ES 7708), +γ, fresh; the King 7740/7745 (ES 7748/7753) far above",
             "data": "7700: $3M (08:45, 22%) → $4M → $7M at 08:57 (50%, d15 +8M) — a small fresh +γ node growing right under price; 7705 −γ tiny. The King sat 35–45 pts above at 7740/7745 — and price rallied to it (7743 at 10:16).", "factors": ["new", "growth", "magnet"]},
            {"leg": "c2", "kind": "PULLBACK", "side": "resistance", "when": "10:13–10:22 CT", "px": "high ES 7742.75 at 10:19", "node": "SPXW 7735 (the King, ES 7743, −γ)",
             "data": "The −γ King: −$30M (09:54) → −$37M (10:00) → −$44M at 10:18, the tap bar (d15 −9M: growing into the tap) → dissipating after (10:30 d15 +12M, 10:36 +15M, −$27M) as price fell to 7731. Price met the King's price to the point and turned.", "factors": ["king", "growth"]},
            {"leg": "c3", "kind": "HOD", "side": "resistance", "when": "11:55–12:20 CT", "px": "high ES 7755.5 at 12:10 (the session high)", "node": "SPXW 7745 (ES 7753) + 7750 (ES 7758), +γ stack, 10–15 pts above the King 7735",
             "data": "7745: $34M (11:30) → $51M (11:39, d15 +26M) → $72M (12:06, +19M) → $73M (12:15) — the biggest +γ node, growing into the tap; 7750 $26M → $46M then decaying from 12:15 (−7, −13M). The high printed between the two (7753 / 7758). Price fell back to the King's price (7733 by 13:00).", "factors": ["growth", "stack", "magnet"]}
        ],
        "call": {"kind": "PULLBACK ×2 then HOD", "summary": "a fresh support node under price with the King far above pulling; the King itself as the first resistance; the HOD at a growing +γ stack, and the return to the King"},
        "blind": None,
        "rules": ["L1", "L2", "L3", "L6", "L7", "L8", "L9"],
        "open": ""
    }
]

# ---- THE GAUGE, 0–100: "how good have you become" — defined so it cannot flatter --------------------------------
# identify (60): my BLIND-read accuracy, Wilson 95% lower bound × 60 — zero until 5 blind reads, thin under RATE_MIN_N.
# predict  (30): the live call "price is going to the node — will it deflect?" scored by the outcome (the deflection
#                engine's CONTINUED / STALLED); zero until the scorer exists (the roadmap's v15.63) and has 30 calls.
# breadth  (10): 5 × min(1, examples/20) + 5 × min(1, confirmed rules/6).
GAUGE = {
    "parts": [
        {"id": "identify", "weight": 60, "what": "blind-read accuracy (Wilson 95%% lower bound); 0 until 5 blind reads; thin under %d" % RATE_MIN_N},
        {"id": "predict", "weight": 30, "what": "live prediction accuracy — 'price is going to the node: will it deflect?' — scored by the outcome; 0 until the scorer exists (v15.64) and 30 calls are scored"},
        {"id": "breadth", "weight": 10, "what": "5 × min(1, examples/20) + 5 × min(1, confirmed rules/6)"}
    ],
    "predictCalls": 0, "predictRight": 0, "predictBuilt": False
}

PRIORS = [
    {"id": "P-sweep", "says": "The level's NAME does not matter to whether a sweep prints the extreme; the flush (depth > 8 pts), the clock (first 30 min) and a slow reclaim (6–30 bars) do. Interior levels (VWAP, value area) are pullback candidates, not extremes.", "from": "roadmap/FINDINGS-sweeps-first-read.md · 284 ES sessions"},
    {"id": "P-node", "says": "Whether a sweep AT a top-5 node / the King prints the extreme more often than one not at a node is UNMEASURED (H6, 9 book sessions, thin). The examples here are the qualitative side of that question; H6 is the quantitative one.", "from": "learning/register.json H6"},
    {"id": "P-tap", "says": "First taps hold more often than later taps (tap decay 73% n=22 vs 47% n=70, book corpus) — a third tap is where hand rule kill.tap3 lives.", "from": "skylit-docs/FINDINGS.md"},
    {"id": "P-new", "says": "TOLD by the docs, no number given: nodes that APPEAR and grow are dealer urgency — velocity mode tracks nodes 'growing, shrinking, appearing, disappearing' (learn/air-pockets-velocity); 'rapid accumulation acts like a magnet' (core-concepts); the 2025-10-09 SPX/SPY/QQQ case: 'downside nodes popped up and grew significantly … floors beginning to grow at 670 … wait for the floor to hit to play the bounce' (examples-and-case-studies); the TSLA case, a node appearing under price and price returning to it. The panel's NEW thresholds (20 bars · ×2 · +20%) are ⚖ hand-set against his four taught days (tools/study-gridtells.py), NOT measured on a corpus; the ledger scores them from here.", "from": "skylit-docs (2026-09-04 read-through for v15.64)"},
    {"id": "P-stack", "says": "TOLD by the docs: a pika cloud is a DENSE cluster of LARGE positive nodes — 'thin cloud = soft/porous; dense king-level cloud can pin all session' (learn/heatseeker-patterns); 'clusters of nodes: when multiple LARGE values group together price pins or chops'; 'double stacked nodes … a strong bounce' (faqs). No member threshold is given; 30% of the King per member and 40% for the biggest are ⚖ hand-set (S6 in tools/study-gridtells.py: median 1 stack per bar on his four days). Three of his circled legs (7755 09-03, 7705 08-28, 7665 08-31) sat at 12–18% of a grown King at the tap — the moving denominator — and no threshold on %King catches them; recorded, not solved.", "from": "skylit-docs (2026-09-04 read-through for v15.64)"},
    {"id": "P-heat", "says": "OPERATOR, 2026-09-04 (his words, a stated claim): a barney stack shows up on the Skylit tape as DENSE PURPLE, just as a pika stack is dense YELLOW; the tape is a heatmap, so a stack's |%King| is high by construction — the heat IS the magnitude. This is why the 30% member cut finds them ('the barney's seem to be detecting better'). Not measured by us; recorded as told.", "from": "operator, 2026-09-04"},
    {"id": "P-pol", "says": "Polarity alone (accelerator vs brake) did not discriminate hold rates (H3: 52.2% vs 52.1%, n=46/48). Colour is identity, not prediction — and L1/L5 above say the same about WHETHER it deflects while proposing that the sign decides WHICH WAY price goes after.", "from": "learning/register.json H3"},
]

PROTOCOL = [
    "You paste a screenshot (and, ideally, say nothing yet).",
    "I write my BLIND CALL first: kind (HOD/LOD turn · PULLBACK turn · NONE), side, the node I think caused it, the factors I see, confidence.",
    "You tell me what it was. If you told me with the screenshot, the example is TAUGHT (recorded, not scored).",
    "I pull the panel's own node-event record for those strikes and times and write what the DATA says, with numbers — agreeing or disagreeing with the picture.",
    "The example produces or touches a rule (L-n): PROPOSED at n=1, CONFIRMED when three agree and none refutes, REFUTED when one contradicts (it stays, marked). A rule rewritten to absorb its counter-example is a NEW proposed rule and waits for a new example — it is not confirmed by the cases that produced it.",
    "My score is blind reads only: right / n, by kind — thin under " + str(RATE_MIN_N) + ". When it is not thin and not embarrassing, I am getting good.",
]

DOC = {
    "schema": 1, "written": "2026-09-03", "rateMinN": RATE_MIN_N,
    "protocol": PROTOCOL, "factors": FACTORS, "rules": RULES, "examples": EXAMPLES, "priors": PRIORS, "gauge": GAUGE,
}


def wilson_low(right, n, z=1.96):
    if n <= 0:
        return 0.0
    p = float(right) / n
    d = 1 + z * z / n
    c = p + z * z / (2 * n)
    m = z * ((p * (1 - p) + z * z / (4 * n)) / n) ** 0.5
    return max(0.0, (c - m) / d)


def gauge(examples, rules):
    sc = score(examples)
    ident = (60.0 * wilson_low(sc["right"], sc["n"])) if sc["n"] >= 5 else 0.0
    pred = (30.0 * wilson_low(GAUGE["predictRight"], GAUGE["predictCalls"])) if (GAUGE["predictBuilt"] and GAUGE["predictCalls"] >= 30) else 0.0
    conf = len([r for r in rules if r["status"] == "CONFIRMED"])
    breadth = 5.0 * min(1.0, len(examples) / 20.0) + 5.0 * min(1.0, conf / 6.0)
    return {"value": int(round(ident + pred + breadth)), "identify": round(ident, 1), "predict": round(pred, 1), "breadth": round(breadth, 1),
            "blindN": sc["n"], "blindRight": sc["right"], "examples": len(examples), "confirmed": conf,
            "identifyWhy": ("%d blind reads, %d right" % (sc["n"], sc["right"])) if sc["n"] else "no blind read yet — every example so far was taught with its answer",
            "predictWhy": ("%d calls, %d right" % (GAUGE["predictCalls"], GAUGE["predictRight"])) if GAUGE["predictBuilt"] else "the scorer is not built (roadmap v15.64)"}


def score(examples):
    """blind reads only: right / n, overall and by kind"""
    out = {"n": 0, "right": 0, "byKind": {}}
    for e in examples:
        b = e.get("blind")
        if not b or b.get("right") is None:
            continue
        out["n"] += 1
        out["right"] += 1 if b["right"] else 0
        k = (e.get("call") or {}).get("kind", "?")
        d = out["byKind"].setdefault(k, {"n": 0, "right": 0})
        d["n"] += 1
        d["right"] += 1 if b["right"] else 0
    return out


def rate_txt(n, right):
    if n < RATE_MIN_N:
        return "thin (n=%d)" % n
    return "%d%% (n=%d)" % (round(100.0 * right / n), n)


def md():
    sc = score(EXAMPLES)
    L = []
    L.append("# THE DEFLECTION LEARNING DOC — his examples, my blind calls, the rules they produce")
    L.append("")
    L.append("> Operator, 2026-09-03: *\"I will be giving you screenshots in a way training you and you will put it into a deflection learning doc, which will be part of your training to get better at identifying deflections … always updating it when i give you examples. and learning until you get really good at identifying deflections.\"*")
    L.append(">")
    L.append("> **Read this on every load (skills/gex/SKILL.md 1a-00d).** Generated by `tools/learn-seed.py` from the same data the 📚 Learn tab renders (`learning/deflections/examples.json`); pinned by `test_v1562.js`. Never edit the .md by hand — edit the seed.")
    L.append("")
    L.append("## 0 · The protocol")
    L.append("")
    for i, p in enumerate(PROTOCOL, 1):
        L.append("%d. %s" % (i, p))
    L.append("")
    L.append("## 1 · The gauge and my score")
    L.append("")
    G = gauge(EXAMPLES, RULES)
    L.append("**Deflection expertise: %d / 100** — identify %g/60 (%s) · predict %g/30 (%s) · breadth %g/10 (%d examples, %d confirmed rules)." % (G["value"], G["identify"], G["identifyWhy"], G["predict"], G["predictWhy"], G["breadth"], G["examples"], G["confirmed"]))
    L.append("")
    L.append("Blind reads only: **%s**." % rate_txt(sc["n"], sc["right"]) + (" By kind: " + " · ".join("%s %s" % (k, rate_txt(v["n"], v["right"])) for k, v in sc["byKind"].items()) + "." if sc["byKind"] else " No blind read yet — every example so far was taught with its answer."))
    L.append("")
    L.append("The gauge is defined so it cannot flatter: identify is the Wilson lower bound of my blind-read accuracy (0 until 5 blind reads); predict is the live call scored by the outcome (0 until the scorer exists); breadth is the only part a taught example can move, and it is capped at 10.")
    L.append("")
    L.append("## 2 · The factors I check on every screenshot (his list, made a checklist)")
    L.append("")
    L.append("| factor | what it is |")
    L.append("|---|---|")
    for f in FACTORS:
        L.append("| **%s** | %s |" % (f["name"], f["what"]))
    L.append("")
    L.append("Which node caused it, first: look it up in the record (strike, %King, $ exposure, d15, stage, taps, side) before naming the factor.")
    L.append("")
    L.append("## 3 · The rules (what I have learned so far)")
    L.append("")
    for r in RULES:
        L.append("- **%s · %s (n=%d: %d agree · %d weak · %d refute; from %s).** %s *%s*" % (r["id"], r["status"], r["n"], r.get("agree", 0), r.get("weak", 0), r.get("refute", 0), ", ".join(r["from"]), r["rule"], r.get("note", "")))
        if r.get("verdict"):
            L.append("  - the record (nightly %s): **%s** — %s" % (r.get("asOf") or "?", r["verdict"], r.get("evidence") or ""))
    L.append("")
    L.append("## 4 · The examples")
    L.append("")
    for e in EXAMPLES:
        L.append("### %s — %s · %s" % (e["id"], e["date"], e["chart"]))
        L.append("")
        L.append("![%s](%s)" % (e["id"], os.path.relpath(e["img"], "learning/deflections")))
        L.append("")
        L.append("**Given:** %s." % e["given"])
        L.append("")
        L.append("**Coverage:** %s" % e.get("coverage", ""))
        L.append("")
        L.append("**His words:** *\"%s\"*" % e["his"])
        L.append("")
        L.append("| leg | kind | side | when | price | node | factors | what the record says |")
        L.append("|---|---|---|---|---|---|---|---|")
        for g in e["legs"]:
            L.append("| %s | %s | %s | %s | %s | %s | %s | %s |" % (g["leg"], g["kind"], g["side"], g["when"], g["px"], g["node"], ", ".join(g["factors"]), g["data"]))
        L.append("")
        L.append("**The call:** %s — %s." % (e["call"]["kind"], e["call"]["summary"]))
        L.append("")
        if e.get("blind"):
            b = e["blind"]
            L.append("**My blind call:** %s — %s." % (b.get("kind"), "RIGHT" if b.get("right") else "WRONG"))
            L.append("")
        else:
            L.append("**My blind call:** none — taught with the answer; not scored.")
            L.append("")
        L.append("**Rules touched:** %s." % ", ".join(e["rules"]))
        if e.get("open"):
            L.append("")
            L.append("**Open:** %s." % e["open"])
        L.append("")
    L.append("## 5 · What the corpus already says (my priors, before his first example)")
    L.append("")
    for p in PRIORS:
        L.append("- **%s.** %s *(%s)*" % (p["id"], p["says"], p["from"]))
    L.append("")
    return "\n".join(L)


if __name__ == "__main__":
    os.makedirs("learning/deflections/img", exist_ok=True)
    # (v15.69) THE RECORD'S VERDICT ON EACH RULE SURVIVES A REGENERATION: learning/results.json (tools/nightly/results.py)
    # carries `rules` — evidence · verdict · asOf per rule — merged over the seed's rules here, so a review editing this
    # seed never erases what the machine measured. The seed owns the rule and its taught legs; the nightly the record.
    try:
        import sys as _sys
        _sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "nightly"))
        import results as _results
        _rp = "learning/results.json"
        if os.path.exists(_rp):
            _rj = json.load(io.open(_rp, encoding="utf-8"))
            _n = _results.apply_rules(DOC, _rj.get("rules") or {})
            print("merged learning/results.json ·", _n, "rules carry the record's verdict")
    except Exception as e:
        print("results merge skipped:", e)
    io.open("learning/deflections/examples.json", "w", encoding="utf-8").write(json.dumps(DOC, ensure_ascii=False, indent=1))
    io.open("learning/deflections/LEARNING.md", "w", encoding="utf-8").write(md())
    print("wrote learning/deflections/examples.json ·", len(EXAMPLES), "examples ·", len(RULES), "rules · LEARNING.md")
