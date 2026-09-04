#!/usr/bin/env python3
"""
THE STUDY REGISTRY, BY SUBJECT - the single source the Analysis tab, the Testing tab and the nightly
read. Written 2026-09-03 from the operator's ask:

    "Kings should have king deflections for each type of king like a setup. In fact I want you to
     further have subcategories for these subjects. Think like a trader. ... an extensive section
     covering types of sweeps under hod lod ... We want information to trade deflections."

Seven subjects, each split the way a trader thinks about it, each study a question whose answer
changes an action at the tap (`decides`: SIZE / SIDE / TARGET / STOP / SKIP / TIME / LEVEL / WAIT).
Statuses: SHIPPED (measured, on the tab) · READ (first read, exploratory) · THIN (data on hand, n
too small) · READ NEXT (data on hand, unread) · OPEN (needs the tap record or the API backfill) ·
REFUSED (measured, no lift) · REGISTERED / BLOCKED (in learning/register.json) · DRAFT (a register
entry proposed, not yet written).

Every result carries its n. Nothing in `result` is illustrative: a study with no number says so.
Run:  python3 tools/studies-seed.py   -> writes learning/studies.json
"""
import json, io

S = {"schema": 1, "written": "2026-09-03", "subjects": []}


def subject(key, name, strap, subs):
    S["subjects"].append(dict(key=key, name=name, strap=strap, subsections=subs))


def sub(key, name, decides, studies, note=None):
    d = dict(key=key, name=name, decides=decides, studies=studies)
    if note:
        d["note"] = note
    return d


def st(id, q, decides, claim, corpus, status, result=None, script=None, was=None):
    d = dict(id=id, q=q, decides=decides, claim=claim, corpus=corpus, status=status)
    if result: d["result"] = result
    if script: d["script"] = script
    if was: d["was"] = was
    return d


# ─────────────────────────────────────────────────────────────── K · KINGS
subject("K", "KINGS", "the crown: which book's, what role, when it moves, and what it does to price", [
    sub("K1", "King deflections by book — SPX King · SPY King · QQQ King", "SIDE · LEVEL",
        [st("K1.1", "When price taps the SPX King, how often does it hold, and what does the hold pay (MFE10)?", "SIZE", "C10", "tap record", "OPEN"),
         st("K1.2", "The same for the SPY King and the QQQ King — three rows, one table", "SIZE", "C10 C28", "tap record", "OPEN"),
         st("K1.3", "Which King leads: when the QQQ King moves, does SPX/SPY price follow within N minutes — and the reverse?", "TIME", "C28", "437 recs · book + tap record", "READ NEXT", "437 kingRoll records, never read"),
         st("K1.4", "Is the SPX King the better floor/ceiling (held rate) and the QQQ King the better direction tell (follow rate)?", "SIDE · LEVEL", "C28", "tap record", "OPEN"),
         st("K1.5", "Kings aligned across the three books (same strike-equivalent) vs split — held rate at the aligned King", "SIZE", "C28", "tap record", "OPEN"),
         st("K1.6", "King in Skylit's flow book vs the InsiderFinance structure King — do both sit at the same strike, and which one price respects?", "LEVEL", "ours", "tap record + IF book", "OPEN")],
        note="Two books are never averaged: Skylit = flow, InsiderFinance = structure. Each King is its own row."),
    sub("K2", "King deflections by role — floor · ceiling · pin", "SIDE · TARGET",
        [st("K2.1", "King BELOW spot (a floor): tap held rate and the bounce's extent", "SIZE · TARGET", "C10", "tap record", "OPEN"),
         st("K2.2", "King ABOVE spot (a ceiling): tap held rate and the rejection's extent", "SIZE · TARGET", "C10", "tap record", "OPEN"),
         st("K2.3", "Spot INSIDE the King band: dwell time, and which side it leaves by", "WAIT", "C10", "tap record", "OPEN"),
         st("K2.4", "King as the magnet: from how far does price get pulled to the King before the close (5–10 pt margin claim)?", "TARGET", "C10", "11d · book", "THIN", "11 sessions"),
         st("K2.5", "King sweep (beach ball at the King): overshoot then close back inside — what follows?", "STOP", "C9", "tap record", "OPEN")]),
    sub("K3", "King by the clock — drive-off early · pin late", "TIME",
        [st("K3.1", "Before 11:00 the King drives price off; after 14:00 it pins — held rate at the King by hour", "TIME", "C10", "tap record", "OPEN", was="S-A2"),
         st("K3.2", "Distance from the King at 15:00 CT — how often inside 5 pts / 10 pts (SPX), by regime", "TARGET", "C10", "11d · book", "THIN", "11 sessions"),
         st("K3.3", "A King tap in the first 30 minutes — is it the day's extreme more often than a later King tap?", "SIZE", "C10 C13", "tap record", "OPEN"),
         st("K3.4", "Last-hour King: does price close on the King's side of the last node it crossed?", "SIDE", "C10", "11d · book", "THIN")]),
    sub("K4", "King rolls — the crown migrates", "SIDE · TARGET",
        [st("K4.1", "After the crown moves, does price move toward the NEW King within 10 / 30 bars?", "SIDE", "C11", "437 recs · book", "READ NEXT", "recorded since v14; unread", was="S-C1"),
         st("K4.2", "A roll UP pulls price up, a roll DOWN pushes it down — follow rate by direction, and by distance of the move", "SIDE · TARGET", "C11", "437 recs · book", "READ NEXT"),
         st("K4.3", "How often the crown moves per session, and how long it dwells", "WAIT", "C11", "11d · book", "SHIPPED", "kingmoves / kingdwell on the tab", was="S-D1"),
         st("K4.4", "A roll in the last 90 minutes — does the close land at the new King?", "TARGET", "C11", "11d · book", "THIN"),
         st("K4.5", "Roll into a fresh node vs roll into an existing one — which does price follow?", "SIDE", "C11 C21", "tap record", "OPEN"),
         st("K4.6", "Does a roll AWAY from spot (the crown leaves) precede the day's extreme?", "SIZE", "C11 C13", "tap record", "OPEN")]),
    sub("K5", "King quality — dominance · freshness · the gate in front", "SIZE",
        [st("K5.1", "Dominance: the King's share of the book's gamma — does a dominant King hold more than a marginal one?", "SIZE", "C1", "tap record", "OPEN"),
         st("K5.2", "A FRESH King (untested today) vs a TESTED one — tap decay at the crown", "SIZE", "C3 C4", "tap record", "OPEN"),
         st("K5.3", "A King carried from yesterday (grew day-over-day = real) vs born today — which holds?", "SIZE", "C5", "tap record", "OPEN"),
         st("K5.4", "A gatekeeper in front of the King: does the gate take the tap, or does price reach the King?", "LEVEL", "C14", "tap record", "OPEN"),
         st("K5.5", "King under rapid unwinding (losing gamma fast) — break rate", "STOP", "C-unwind", "tap record", "OPEN")]),
    sub("K6", "The King and the day's extremes", "SIZE · TARGET",
        [st("K6.1", "How often the HOD/LOD prints AT the King, per book", "TARGET", "C13", "tap record", "OPEN", was="S-A1"),
         st("K6.2", "Which book's King owns the extremes — SPX, SPY or QQQ", "LEVEL", "C28", "tap record", "OPEN", was="S-A6"),
         st("K6.3", "King as the day's pin: close within the King band — rate, by OPEX vs non-OPEX", "TARGET", "C10 C-OPEX", "11d · book", "THIN"),
         st("K6.4", "A King tap that IS the extreme vs one that is not — what differed before the tap (growth, trinity, clock)?", "SIZE", "C10 C21 C28", "tap record", "OPEN")]),
])

# ─────────────────────────────────────────────────────────────── S · SETUPS
subject("S", "SETUPS", "the patternpedia as trades: trigger · invalidation · target — each measured, not admired", [
    # (2026-09-03) operator: "you did not have king deflection in the setups. this is a bread and butter setup so make sure
    # you have a king deflection setup and also individual king deflection setups like qqq king, spy king and spx king
    # and track this religiously." The King deflection is the FIRST setup, one row per book, and one for the King that
    # rolls onto price. The deflection ledger (51 events on 2026-09-03) carries the node's role and the book, so S0.1–S0.4
    # read from it as it fills; the Learn tab's examples are its qualitative side (E001 R1/R2, E002 c4, E003 c1/c3, E004 c2).
    sub("S0", "King deflection — the bread-and-butter setup", "SIZE · SIDE · LEVEL",
        [st("S0.1", "A tap of THE King (whichever book owns it): held rate, MFE10, bars held — the base row every other setup is measured against", "SIZE", "C10", "defl ledger · tap record", "THIN", "51 ledger events on 2026-09-03; the join with the King's role is the review's next job"),
         st("S0.2", "SPX King deflection (SPXW, the flow book): held rate and extent", "SIZE · LEVEL", "C10 C28", "defl ledger · tap record", "OPEN"),
         st("S0.3", "SPY King deflection: held rate and extent — and does it lead or lag the SPX King's?", "SIZE · SIDE", "C10 C28", "defl ledger · tap record", "OPEN"),
         st("S0.4", "QQQ King deflection: held rate and extent — the cross-book tell (K1.3)", "SIDE", "C28", "defl ledger · tap record", "OPEN"),
         st("S0.5", "King growing INTO the tap vs flat/decaying at the tap — held rate by d15 (the Learn tab's L1, measured)", "SIZE", "C21 ours", "defl ledger · tap record", "OPEN"),
         st("S0.6", "The King that ROLLS onto price (E003 c3): is the first tap after a roll a better hold than a tap of a King that stood still?", "SIZE · LEVEL", "C19 C10", "defl ledger · tap record", "OPEN"),
         st("S0.7", "+γ King vs −γ King deflection: same hold rate (H3 says polarity does not discriminate) — but does price return to a +γ King and run from a −γ one (L3/L5/L8)?", "SIDE · TARGET", "C8 ours", "defl ledger · tap record", "OPEN")],
        note="Tracked religiously: every King tap is a ledger row with the book, the role, the growth into the tap and the outcome; the per-book rows print a rate only at n ≥ 15 (thin below), like every rate here."),
    sub("S1", "Rug · reverse rug", "SIZE · TARGET",
        [st("S1.1", "Rug: a rejection with acceleration — is it faster and deeper than a plain rejection (MFE10, bars to target)?", "TARGET", "C23", "tap record", "OPEN", was="S-B6"),
         st("S1.2", "Reverse rug (the upside mirror) — same measure", "TARGET", "C23", "tap record", "OPEN"),
         st("S1.3", "Rug at the King vs rug at a lesser node", "SIZE", "C23 C10", "tap record", "OPEN"),
         st("S1.4", "Rug configuration detected (rugDetect) vs Skylit's nodeType — agreement rate", "SKIP", "ours", "API · 1 call/bar", "OPEN", was="S-D6"),
         st("S1.5", "After a rug, does price return to the node (revisit within 20 bars)?", "STOP", "C6 C23", "tap record", "OPEN")]),
    sub("S2", "Gatekeeper rejection", "SIDE · LEVEL",
        [st("S2.1", "An EARLY gatekeeper rejection (before 10:30) marks the day's extreme — rate vs a late one", "SIZE", "C14", "671 recs · book", "READ NEXT", "gatekeeper records unread", was="S-A3"),
         st("S2.2", "Gatekeeper ratio (its value ÷ the node behind it): hold vs break by ratio band", "LEVEL", "C14", "tap record", "OPEN", was="S-B9"),
         st("S2.3", "When the gate breaks, does price reach the node behind it — and how fast?", "TARGET", "C14 C15", "tap record", "OPEN"),
         st("S2.4", "A gate that held: is the next tap of the node behind it a better hold?", "SIZE", "C14 C3", "tap record", "OPEN")]),
    sub("S3", "Beach ball — overshoot, close back inside", "STOP · TARGET",
        [st("S3.1", "How often a tap overshoots the band and closes back inside (wick triggers, 3m close decides)", "STOP", "C9", "8 sess · book", "SHIPPED", "the deflection engine's trigger geometry"),
         st("S3.2", "After the overshoot, the reversion's extent — does a deeper overshoot pay more?", "TARGET", "C9", "tap record", "OPEN", was="S-B7"),
         st("S3.3", "Overshoot at a −γ node vs +γ — wickier at −γ?", "STOP", "C8", "94 ep · book", "REFUSED", "H3 null: 52.2% vs 52.1%, n=46/48", was="S-B8"),
         st("S3.4", "Overshoot depth in ATR: where to put the stop so the beach ball does not take it", "STOP", "C9 ours", "tap record", "OPEN")]),
    sub("S4", "Air pocket — the gap in the ladder", "TARGET · SIZE",
        [st("S4.1", "A pocket on the far side of the tap: is the deflection larger (MFE10 by pocket × velocity × polarity)?", "TARGET", "C15 C20", "tap record", "OPEN", was="S-B10"),
         st("S4.2", "When price enters a pocket, does it reach the next node — reach rate and speed", "TARGET", "C15", "tap record", "OPEN"),
         st("S4.3", "Entry on the break into the pocket vs on the retest of the node — which pays?", "TIME", "C15", "tap record", "OPEN"),
         st("S4.4", "Pocket width in points vs realised travel — is the pocket the target, or does price stop short?", "TARGET", "C15", "tap record", "OPEN")]),
    sub("S5", "Rolling floors and ceilings", "SIDE · TARGET",
        [st("S5.1", "Two migrations vs three — does price follow within the session?", "SIDE", "C19", "11d · book", "THIN", was="S-C2"),
         st("S5.2", "Stairstep: after delivery, is the next fresh growing node reached?", "TARGET", "C22", "11d · book", "THIN", was="S-C3"),
         st("S5.3", "Succession: motion vs stasis — measured", "WAIT", "C22", "11d · book", "SHIPPED", "three passes on the tab", was="S-D2"),
         st("S5.4", "A floor that rolls UP under a pullback — does the pullback turn at the new floor?", "LEVEL", "C19", "tap record", "OPEN")]),
    sub("S6", "New node in the path · node fed gamma · node that moved", "LEVEL · SIZE",
        [st("S6.1", "A node BORN during the move (crossed the display threshold) — does the pullback deflect at it?", "LEVEL", "C21", "tap record", "OPEN", was="S-B5"),
         st("S6.2", "An existing node that had gamma ADDED into the tap (15-min accumulation) vs one left flat", "SIZE", "C21", "tap record", "OPEN", was="S-B4"),
         st("S6.3", "A node whose strike SHIFTED toward price vs away — which one deflects?", "LEVEL", "C21 C22", "tap record", "OPEN"),
         st("S6.4", "Reshuffle: do pre-reshuffle nodes stop holding?", "SKIP", "C21", "tap record", "OPEN", was="S-D4"),
         st("S6.5", "Rollsupport read: growth mattered, pairing did not — re-read as events", "SIZE", "C21", "11d · book", "READ", "exploratory; direction only"),
         st("S6.6", "THE GROWTH WINDOW: is growth INTO the tap measured over 5, 15 or 30 minutes the better tell for the hold? (his ask 2026-09-03: \"I'm not sure about the timeframe … we should test this\")", "SIZE · TIME", "ours", "defl ledger · day files (d5 · d15 · d60 recorded per bar)", "OPEN", "the dashboard shows 15 min until this reads; the row's window follows the winner")]),
    sub("S7", "Pika cloud · cluster", "WAIT · SIZE",
        [st("S7.1", "Does a pika cloud pin — dwell inside vs at a single node", "WAIT", "C24", "tap record", "OPEN", was="S-B11"),
         st("S7.2", "Cluster mass vs held rate — does magnitude decide?", "SIZE", "C24 C1", "tap record", "OPEN"),
         st("S7.3", "clusterDetect vs Skylit's pika/barney nodeType — agreement", "SKIP", "ours", "API · 1 call/bar", "OPEN")]),
    sub("S8", "Rapid unwinding · hedge bleed · decoys", "STOP · SKIP",
        [st("S8.1", "A node losing gamma fast (rapid unwinding) — break rate on the next tap", "STOP", "C-unwind", "tap record", "OPEN"),
         st("S8.2", "Hedge bleed: nodes that shrank day-over-day — how often they vanish; do they ever hold?", "SKIP", "C5", "tap record", "OPEN", was="S-D3"),
         st("S8.3", "Decoys: far, isolated, oversized nodes — reach rate by distance × gatekeeper count", "SKIP", "C18", "tap record", "OPEN", was="S-D5"),
         st("S8.4", "A node that vanished and came back — does it deflect like a fresh one?", "SIZE", "C4 C21", "tap record", "OPEN")]),
])

# ─────────────────────────────────────────────────────────────── D · DIRECTION
subject("D", "DIRECTION", "which way after the turn, and how far — the leg between the two extremes is the trade", [
    sub("D1", "The day's direction", "SIDE",
        [st("D1.1", "GREEN / RED — does the session close above its own open?", "SIDE", "—", "284d · price", "SHIPPED", "77% one-liner, n=196; opens / value area no better than sham", "study-greenred.py", was="S-C5"),
         st("D1.2", "LOD first or HOD first — which extreme prints first?", "SIDE · TIME", "—", "284d · price", "SHIPPED", "LOD first 146 / HOD first 138 = 51% · n=284", "BASERATES.json"),
         st("D1.3", "Day type: trend / range / whipsaw / rainbow vs realised range and efficiency", "SIZE", "C26 C27", "11d · book", "THIN", was="S-C4"),
         st("D1.4", "Open location vs the overnight range (above / inside / below) → GREEN rate and sweep rate", "SIDE", "lore", "284d · price", "READ", "open inside ON: 253 of 275 ON sweeps reclaimed; direction split not yet read", "study-sweeps.py")]),
    sub("D2", "After the deflection — which way, how far", "TARGET · STOP",
        [st("D2.1", "The hold's extent: MFE10 / MAE, distance to the next node, bars to get there", "TARGET", "ours", "94 ep · book", "READ", "median MFE10 on the tab; by node role thin"),
         st("D2.2", "+γ fade vs −γ follow: does the rejection travel further at −γ?", "TARGET", "C30", "94 ep · book", "THIN", "H3 says held rate is flat; extent unread", was="S-B14"),
         st("D2.3", "Continuation to the next node vs failure back through the tapped one — the R:R actually realised", "STOP", "ours", "tap record", "OPEN"),
         st("D2.4", "After a HOD/LOD deflection, how far to the far extreme — the prize", "TARGET", "ours", "284d · price", "READ", "when a sweep is the extreme: median 60–89 pts to the far extreme, by level", "study-sweeps.py")]),
    sub("D3", "The King and spot", "SIDE",
        [st("D3.1", "Spot above the King at the open vs below — drift direction by 15:00", "SIDE", "C10", "11d · book", "THIN"),
         st("D3.2", "Distance to the King at the open vs realised drift toward it (the magnet in points)", "TARGET", "C10", "11d · book", "THIN"),
         st("D3.3", "dir.kingRoll — the 437 records: follow rate", "SIDE", "C11", "437 recs · book", "READ NEXT")]),
    sub("D4", "Structure → direction", "SIDE",
        [st("D4.1", "Floors migrating up during the morning → GREEN close rate", "SIDE", "C19", "11d · book", "THIN"),
         st("D4.2", "Ceilings migrating down → RED close rate", "SIDE", "C19", "11d · book", "THIN"),
         st("D4.3", "The book's skew (gamma above vs below spot) at 09:00 → close side", "SIDE", "C-regime", "11d · book", "THIN")]),
    sub("D5", "Cross-book lead — does QQQ lead SPX?", "TIME · SIDE",
        [st("D5.1", "Cross-market divergence at a new extreme (this book made a new low, the sibling did not) — is the standing extreme final?", "SIZE", "lore", "284d ES + NQ · price", "READ", "in model-lodhod as xdiv; event-level read pending", "model-lodhod.py"),
         st("D5.2", "NQ prints its extreme before ES — how many minutes, how often?", "TIME", "lore", "284d ES + NQ · price", "READ NEXT"),
         st("D5.3", "QQQ King moves first, SPX follows — lead time in minutes", "TIME", "C28", "437 recs · book", "READ NEXT")]),
])

# ─────────────────────────────────────────────────────────────── F · DEFLECTION MECHANICS
subject("F", "DEFLECTION MECHANICS", "what a tap is, and what makes it hold — the engine both objectives run on", [
    sub("F1", "The zone and the trigger", "STOP · LEVEL",
        [st("F1.1", "Skylit's zone (±0.50 SPY/QQQ, ±5 SPX) vs our ATR band — which definition is tighter, and do they disagree on taps?", "LEVEL", "C2", "tap record", "OPEN", was="S-A4"),
         st("F1.2", "Wick triggers, the 3-minute close decides — the trigger geometry as shipped", "STOP", "C9 ours", "8 sess · book", "SHIPPED", "DEFL_NEAR 1.0 / DEFL_THRU 1.5 ATR"),
         st("F1.3", "The midpoint control: reactions halfway between nodes — the rate every node cell is judged against", "SKIP", "C16", "tap record", "OPEN", was="S-B16"),
         st("F1.4", "Depth of the wick beyond the node: does deeper mean more likely to hold, or less?", "STOP", "ours", "970 rows · book", "REFUSED", "H4 null: 49.1% vs 48.6%, n=970 rows", was="S-B17")]),
    sub("F2", "Tap decay — 1st · 2nd · 3rd", "SIZE",
        [st("F2.1", "Skylit's 80 / 66 / 33 against our taps — held rate by tap number", "SIZE", "C3", "94 ep · book", "REGISTERED", "tap≥1 73% n=22 vs tap0 47% n=70 · H2 reads at 30", was="S-B1"),
         st("F2.2", "The third tap: kill.tap3 as a rule — does it earn its place?", "SKIP", "C3", "94 ep · book", "THIN", "n=2 third taps"),
         st("F2.3", "Decay by node role: does the King decay slower than a lesser node?", "SIZE", "C3 C10", "tap record", "OPEN")]),
    sub("F3", "Lifecycle state", "SIZE",
        [st("F3.1", "FRESH vs TESTED vs DELIVERED vs DECAYING — held rate by state", "SIZE", "C4", "tap record", "OPEN", was="S-B2"),
         st("F3.2", "Real vs hedge: grew day-over-day vs faded — held rate", "SIZE", "C5", "tap record", "OPEN", was="S-B3"),
         st("F3.3", "After a rejection, does the node's growth in the next 15 minutes predict the revisit?", "WAIT", "C6", "tap record", "OPEN", was="S-B15")]),
    sub("F4", "Growth into the tap", "SIZE",
        [st("F4.1", "Building vs flat vs shedding in the 15 minutes before the tap", "SIZE", "C21", "tap record", "OPEN", was="S-B4"),
         st("F4.2", "Growth measured in the flow book vs the structure book — do they agree on 'building'?", "SIZE", "ours", "tap record + IF book", "OPEN"),
         st("F4.3", "Accumulation canon (accumCanon) vs the archived variants — one definition, one number", "SIZE", "ours", "11d · book", "READ", "growth mattered, pairing did not (rollsupport)")]),
    sub("F5", "Magnitude and rank", "SIZE",
        [st("F5.1", "Top-5 node vs the rest — held rate", "SIZE", "C1", "94 ep · book", "THIN"),
         st("F5.2", "Grade A/B/C at fire — does the grade predict the hold, or invert it?", "SIZE", "ours", "94 ep · book", "REGISTERED", "exploratory 32% n=34 for A — the inverse · H1 reads at 40"),
         st("F5.3", "Node share of the book's gamma at the tap — a continuous magnitude read", "SIZE", "C1", "tap record", "OPEN")]),
    sub("F6", "Polarity and regime", "STOP · TARGET",
        [st("F6.1", "+γ vs −γ node — held rate (registered null)", "STOP", "C8", "94 ep · book", "REGISTERED", "H3: 52.2% vs 52.1% · n=46/48 · gap < 8 predicted"),
         st("F6.2", "+γ vs −γ — the EXTENT, not the rate", "TARGET", "C30", "94 ep · book", "THIN"),
         st("F6.3", "GEX / VEX overlap at the strike — held rate with vs without vanna", "SIZE", "C29", "tap record", "OPEN", was="S-B13")]),
    sub("F7", "Trinity", "SIZE",
        [st("F7.1", "3/3 vs 2/3 vs split at the tap — held rate", "SIZE", "C28", "tap record", "OPEN", was="S-B12"),
         st("F7.2", "The SPX outlier is the bad one — held rate when SPX disagrees vs when QQQ disagrees", "SIZE", "C28", "tap record", "OPEN"),
         st("F7.3", "Trinity alignment at the OPEN vs at the tap — which one predicts?", "TIME", "C28", "tap record", "OPEN")]),
    sub("F8", "Velocity into the tap", "STOP",
        [st("F8.1", "Fast approach vs grind (ROC at tap) — held rate and overshoot depth", "STOP", "C20", "tap record", "OPEN"),
         st("F8.2", "Velocity × air pocket behind — the setups that overshoot the most", "STOP", "C15 C20", "tap record", "OPEN")]),
])

# ─────────────────────────────────────────────────────────────── P · PULLBACK DEFLECTIONS
subject("P", "PULLBACK DEFLECTIONS", "inside a move, will this retracement turn at this node — the join-the-move trade", [
    sub("P1", "Which node holds the pullback", "LEVEL",
        [st("P1.1", "The NEAREST node, the LARGEST below/above, the King, or the node that just delivered — share of pullback turns by role", "LEVEL", "C13 C22", "tap record", "OPEN"),
         st("P1.2", "First node hit vs second — how often the pullback needs the second", "LEVEL", "ours", "tap record", "OPEN"),
         st("P1.3", "A gatekeeper in the pullback's path — does it take the turn before the bigger node?", "LEVEL", "C14", "tap record", "OPEN")]),
    sub("P2", "Pullback depth", "STOP · LEVEL",
        [st("P2.1", "Retracement of the leg at the turn (% of the leg) — distribution, by regime", "STOP", "ours", "tap record", "OPEN"),
         st("P2.2", "Turns inside the EM band vs beyond it", "LEVEL", "ours", "11d · book", "THIN"),
         st("P2.3", "Depth in ATR from the leg's extreme — where the stop must live", "STOP", "ours", "tap record", "OPEN")]),
    sub("P3", "Timing — the 10:00 · midday · the 14:00", "TIME",
        [st("P3.1", "The first pullback after the open drive (09:00–10:30): turn rate at the first node", "TIME", "lore", "tap record", "OPEN"),
         st("P3.2", "Midday pullbacks (11:30–13:30): turn rate — does lunch hold?", "TIME", "lore", "tap record", "OPEN"),
         st("P3.3", "The 14:00 pullback into the last 90 minutes: turn vs reversal", "TIME", "lore C10", "tap record", "OPEN")]),
    sub("P4", "Node condition at the pullback", "SIZE",
        [st("P4.1", "A node born during the move vs an established one", "SIZE", "C21", "tap record", "OPEN", was="S-B5"),
         st("P4.2", "A node fed gamma during the leg vs one left flat", "SIZE", "C21", "tap record", "OPEN"),
         st("P4.3", "A node that moved WITH the leg (rolled up under a rally) vs static", "SIZE", "C19 C22", "tap record", "OPEN"),
         st("P4.4", "Tap number of the pullback node: is a TESTED node a better pullback floor than a FRESH one (H2 says maybe)?", "SIZE", "C3", "94 ep · book", "REGISTERED", "H2 · tap≥1 73% n=22")]),
    sub("P5", "Confluence", "SIZE",
        [st("P5.1", "Node × VWAP within the band — turn rate with vs without; and the VWAP-band sweep as a pullback: does the move RESUME to a new extreme within 30 bars?", "SIZE", "lore", "284d · price + tap record", "READ NEXT", "H2.10h found VWAP/band sweeps are not the extreme (5–13%, n=124–207) — the right outcome for them is resume, unread"),
         st("P5.2", "Node × a prior level (IBH/IBL, ONH/ONL, PDH/PDL) — turn rate with vs without", "SIZE", "lore", "tap record + 284d price", "OPEN"),
         st("P5.3", "Node × EM band edge", "SIZE", "ours", "tap record", "OPEN"),
         st("P5.4", "Node in both books (Skylit and InsiderFinance) vs one", "SIZE", "ours", "tap record + IF book", "OPEN")]),
    sub("P6", "Deflection or reversal", "SIDE · STOP",
        [st("P6.1", "After the turn, did the move resume to a NEW extreme (continuation) — rate", "SIDE", "ours", "tap record", "OPEN"),
         st("P6.2", "The pullback that became the day's turn: what did it look like before it broke (growth, tap#, clock)?", "STOP", "ours", "tap record", "OPEN"),
         st("P6.3", "Failure cost: when the pullback node breaks, how far to the next node (the stop's distance)", "STOP", "C15", "tap record", "OPEN")]),
    sub("P7", "Payoff", "TARGET",
        [st("P7.1", "Resume-to-new-extreme distance and time, from the turn", "TARGET", "ours", "tap record", "OPEN"),
         st("P7.2", "MFE10 of pullback holds vs extreme holds — which pays more per point of stop?", "TARGET", "ours", "tap record", "OPEN"),
         st("P7.3", "With-trend pullback on a GREEN day vs counter-trend — turn rate and payoff", "SIDE", "lore", "tap record", "OPEN")]),
])

# ─────────────────────────────────────────────────────────────── H · HOD / LOD
subject("H", "HOD / LOD", "is the extreme in, what printed it, and what the other side pays — the move from low to high", [
    sub("H1", "Is the extreme in", "SIZE · WAIT",
        [st("H1.1", "The standing extreme is the day's — posr × clock, calibrated", "SIZE", "C17", "284d · price", "SHIPPED", "AUC 0.879 · NOT-IN cell 85% n=230", "model-lodhod.py", was="S-A5"),
         st("H1.2", "The time-only baseline every claim must beat: the standing low is final ~40% at 09:30, ~64% by noon", "WAIT", "—", "284d · price", "SHIPPED", "clock curve on the tab"),
         st("H1.3", "The standing extreme conditioned on a top-5 node at it — does the node move the cell?", "SIZE", "C17 C13", "defl ledger 0/50", "BLOCKED", "H5 · needs 50 labelled deflections"),
         st("H1.4", "IB60 break as an 'extreme is in' tell", "SIZE", "lore", "284d · price", "READ", "0.655 standalone; no lift over the clock in ablation")]),
    sub("H2", "SWEEPS — the levels that get run before the turn", "SIZE · STOP · TIME",
        [st("H2.1", "ONL sweep-and-reclaim → is the sweep's low the LOD? ONH → the HOD?", "SIZE", "lore", "284d · price", "READ", "ONL 29% n=113 · ONH 26% n=140 — vs a fresh-low bounce at nothing: 28% / 22% (+1 / +4pp)", "study-sweeps.py"),
         st("H2.2", "PDL / PDH sweep-and-reclaim → the extreme?", "SIZE", "lore", "284d · price", "READ", "PDL 23% n=87 · PDH 16% n=113 — control 24% / 18% (−1 / −2pp)", "study-sweeps.py"),
         st("H2.3", "IBL / IBH sweep-and-reclaim (after 09:30) → the extreme?", "SIZE", "lore", "284d · price", "READ", "IBL 20% n=143 · IBH 16% n=148 — control 26% / 19% (−6 / −3pp)", "study-sweeps.py"),
         st("H2.4", "Sweep by the clock: does an early sweep print the extreme more than a late one?", "TIME", "lore", "284d · price", "READ", "first 30 min: ONL 37% n=35 · ONH 27% n=41 · PDL 31% n=39 · PDH 20% n=65 — each +5 to +15pp over control; after 10:00 no better than nothing", "study-sweeps.py"),
         st("H2.5", "Depth and speed of the sweep — the flush vs the poke", "SIZE · STOP", "lore", "284d · price", "READ", "depth > 8 pts 40% n=86 (+16pp) · reclaim 6–30 bars 40% n=90 (+16pp) · shallow ≤3 pt quick poke 14% n=228 (−9pp): the poke is NOT the low 86% of the time", "study-sweeps.py"),
         st("H2.6", "Acceptance — the level breaks on first touch (no reclaim in 30 bars)", "STOP", "lore", "284d · price", "READ", "PD levels break on first touch 30% n=124 / 34% n=170 · ON 7% n=121 / 9% n=154 · IB 14% n=166 / 20% n=184", "study-sweeps.py"),
         st("H2.7", "Sweep × node: an ON/PD sweep-reclaim that lands in a top-5 node band or at the King — does the node lift the 26%?", "SIZE", "C13 C17", "tap record · API backfill", "DRAFT", "H6 proposed: > 40% vs the 24% base (n=453 ON/PD sweep-reclaims) · refute if ≤ 30%"),
         st("H2.8", "The early sweep as a register entry — re-read on sessions after 2026-08-21", "SIZE · TIME", "lore", "new sessions", "DRAFT", "H7 proposed: first-30-min ON/PD sweep-reclaim prints the extreme > 24% · refute if ≤ 18% (pooled 27% n=180 on the read corpus)"),
         st("H2.9", "Sweep of the SESSION extreme itself — the failed breakdown: a new low that fails within 30 bars", "SIZE", "lore", "284d · price", "READ NEXT"),
         st("H2.10", "PDC (prior close, the gap-fill level) — sweep-and-reclaim → the extreme?", "SIZE", "lore", "284d · price", "READ", "PDC− → LOD 30% n=120 (control 21%, +9pp) · PDC+ → HOD 19% n=105 (control 17%) · and PDC BREAKS on first touch 44% / 56% of the time — it is a magnet, not a wall", "study-sweeps.py"),
         st("H2.10b", "Prior-day profile — POC / VAH / VAL sweep-and-reclaim → the extreme?", "SIZE", "lore", "284d · price", "READ", "VAL 28% n=112 (+4pp) · VAH 19% n=118 (+2pp) · POC− 27% n=108 (+4pp) · POC+ 26% n=97 (+9pp) — all inside chance of the control; they break on first touch 35–57%", "study-sweeps.py"),
         st("H2.10c", "Pre-market high/low (07:00–08:30 CT) — PMH / PML", "SIZE", "lore", "284d · price", "READ", "PML 21% n=189 (control 21%) · PMH 15% n=197 (17%) — nothing; they reclaim 87% of the time and mean nothing when they do", "study-sweeps.py"),
         st("H2.10d", "Prior WEEK high/low — PWH / PWL", "SIZE", "lore", "284d · price", "READ", "PWL 25% n=36 (27%) · PWH 22% n=55 (19%) — thin and flat; PW levels break on first touch 38% / 57%", "study-sweeps.py"),
         st("H2.10e", "Opening range 5 / 15 min — OR5 / OR15 sweeps (after the range completes)", "SKIP", "lore", "284d · price", "READ", "OR5L 20% n=212 (−4pp) · OR5H 10% n=215 (−7pp) · OR15L 22% n=186 (−6pp) · OR15H 14% n=201 (−7pp) — an OR sweep-reclaim is slightly WORSE than a bounce at nothing: it is the poke by construction", "study-sweeps.py"),
         st("H2.10f", "Sweeps of the BOOK'S levels — CW0 / PW0 / CW / PW and the King as sweep levels, valued as of the sweep bar", "SIZE", "C13 ours", "book · day files", "THIN", "BOOK CORPUS 11 sessions (2026-08-18 → 2026-09-03, SPY 3-min, ±0.50 zone): price sweep AT a top-5 node / the King 25% n=12 vs NOT at a node 38% n=8 · AT the King 25% n=4 · AT a wall 43% n=7 · sweeps OF the book’s levels: KING↓ 20% n=5 · KING↑ 50% n=2 · PW0↓ 0% n=4 · CW0↑ 0% n=4 — every cell thin; the corpus grows one session per export (nightly 2026-09-03 evening: 11)"),
         st("H2.10g", "Sweep × NODE (H6 as a table): a price-level sweep whose extremum sits in the tap zone (±0.50 SPY) of a top-5 node / the King / a wall, vs one that lands at nothing", "SIZE", "C2 C13 C17", "book · day files", "REGISTERED", "H6 · judged by the nightly from the same table · reads at 40 at-node events on sessions from 2026-09-03", "study-sweeps-book.py"),
         st("H2.10h", "VWAP and its ±1σ / ±2σ bands as sweep levels (valued at the sweep bar, side = the side price came from)", "SKIP · LEVEL", "lore", "284d · price", "READ", "the interior levels are NOT the extreme: VWAP- 7% n=126 (control 21%) \u00b7 VWAP+ 5% n=124 (control 16%) \u00b7 VW1L 8% n=207 (control 28%) \u00b7 VW1H 8% n=200 (control 23%) \u00b7 VW2L 13% n=180 (control 28%) \u00b7 VW2H 11% n=178 (control 22%) \u2014 a VWAP-band sweep is a PULLBACK candidate, not the day\u2019s turn; the pullback outcome (resume to a new extreme) is P5.1", "study-sweeps.py"),
         st("H2.10i", "Today’s DEVELOPING profile — DPOC / DVAH / DVAL as sweep levels", "SKIP · LEVEL", "lore", "284d · price", "READ", "not the extreme, by construction inside the range: DPOC- 5% n=131 (control 28%) \u00b7 DPOC+ 3% n=121 (control 23%) \u00b7 DVAL 8% n=213 (control 28%) \u00b7 DVAH 7% n=207 (control 22%) \u2014 pullback levels, measured for the pullback objective under P5", "study-sweeps.py"),
         st("H2.10j", "The London range (02:00 CT → the open) — LDNH / LDNL", "SIZE", "lore", "284d · price", "READ", "LDNL 21% n=163 (control 24%) \u00b7 LDNH 16% n=170 (control 18%) \u2014 like every other level name: within chance of the fresh-low control", "study-sweeps.py"),
         st("H2.10k", "The expected-move edges (EMH / EML) as sweep levels — Skylit’s own band", "SIZE", "C-EM ours", "live only · needs the straddle in the corpus", "OPEN", "live in the READ from v15.57; the price corpus has no straddle, so no rate yet — the tap record / API backfill measures it"),
         st("H2.10l", "HVL (zero-gamma) and the magnet as sweep levels", "SIZE", "C30 ours", "book · day files", "THIN", "HVL↓ 1 of 2 · HVL↑ 1 of 2 · MAG↓ 1 of 3 · MAG↑ 1 of 2 over 9 sessions (n under 15 everywhere: thin) — grows with every export", "study-sweeps-book.py"),
         st("H2.11", "The payoff when the sweep IS the extreme — median distance to the far extreme", "TARGET", "ours", "284d · price", "READ", "ONL 74.5 · ONH 67.8 · PDL 84.2 · PDH 60.0 pts — the whole range, the prize", "study-sweeps.py"),
         st("H2.12", "The bar-level sweep feature inside the standing-extreme model — kept on the board as the refusal it earned", "SKIP", "—", "284d · price", "REFUSED", "swp 0.559 standalone; no lift over the clock", "model-lodhod.py")],
        note="First read, 79 cells over 20 level types on price, 20 more on the book corpus (thin), none pre-registered. The verdict that matters: the LEVEL'S NAME does not matter — ON, PD, PDC, POC/VAH/VAL, PM, PW, IB, OR are all within chance of a bounce off any fresh low (−7 to +9pp; the two +9s are what 79 cells produce by luck); the CLOCK, the DEPTH and the SPEED do. The node-conditioned version is H6 — the tapereader's whole reason to exist."),
    sub("H3", "Which node printed the extreme", "LEVEL · SIZE",
        [st("H3.1", "The King, the largest-beyond, the nearest, or a gatekeeper — share of extremes by role, per book", "LEVEL", "C13 C10", "tap record", "OPEN", was="S-A1"),
         st("H3.2", "State and tap number of the node at the extreme (FRESH? first tap?)", "SIZE", "C3 C4", "tap record", "OPEN"),
         st("H3.3", "Extremes with NO node within the band — how often the day turns at nothing (the control)", "SKIP", "C16", "tap record", "OPEN"),
         st("H3.4", "Which book's node — Skylit flow vs InsiderFinance structure", "LEVEL", "ours", "tap record + IF book", "OPEN")]),
    sub("H4", "The clock of extremes", "TIME · WAIT",
        [st("H4.1", "When the extremes print — the first extreme's median minute, the gap to the second", "TIME", "—", "284d · price", "SHIPPED", "first extreme median 33.5 min · gap 229.5 min", "BASERATES.json"),
         st("H4.2", "LOD-first vs HOD-first by day of week and by open location", "TIME", "lore", "284d · price", "READ NEXT"),
         st("H4.3", "An extreme printed in the first 30 minutes — survival rate by the minute it was set (the ladder)", "WAIT", "C17", "284d · price", "SHIPPED", "ladder 30/60/90/120/180 on the tab", "BASERATES.json"),
         st("H4.4", "The 14:00 turn: extremes printed after 13:30 — how often, and are they at the King?", "TIME", "C10", "284d + tap record", "READ NEXT")]),
    sub("H5", "Range and the expected move", "TARGET",
        [st("H5.1", "The day's range: median 61.4 pts, p25 41.8 — what a full low-to-high pays", "TARGET", "—", "284d · price", "SHIPPED", "rng_pts 61.4 · rng_usd 3,072", "BASERATES.json"),
         st("H5.2", "The extreme at the EM band edge vs inside — how often the band is the wall", "TARGET", "ours", "11d · book", "THIN"),
         st("H5.3", "Range vs ATR at the open — does a wide open range shrink the second leg?", "TARGET", "lore", "284d · price", "READ NEXT")]),
    sub("H6", "The other side — the move from low to high", "TARGET",
        [st("H6.1", "After the LOD holds, where does the HOD print — at the King, a ceiling, the EM edge?", "TARGET", "C13 ours", "tap record", "OPEN"),
         st("H6.2", "Time from the first extreme to the second — median 229 min; by the first extreme's clock", "TIME", "—", "284d · price", "SHIPPED", "gap_min 229.5", "BASERATES.json"),
         st("H6.3", "The first excursion (wick family): median 40 pts n=252 before the first close back through the open", "STOP", "ours", "284d · price", "SHIPPED", "wick 61.1 mean / 40 median · n=252 · zero-wick days print 0", "BASERATES.json"),
         st("H6.4", "Does the second extreme reach the far-side node most often named by the ladder at the first extreme?", "TARGET", "C15", "tap record", "OPEN"),
         st("H6.5", "The LID: after a big-node rejection from the LOD, how often does a ceiling node cap the bounce — at which node, how far up, how soon (his example for the READ)", "TARGET · STOP", "C10 C13 ours", "tap record", "OPEN")]),
    sub("H7", "Event and expiry days", "SIZE · SKIP",
        [st("H7.1", "OPEX: does the doctrine's discount show as a lower held rate at the extremes?", "SIZE", "C-OPEX", "tap record", "OPEN"),
         st("H7.2", "FOMC / CPI days (ForexFactory): extremes at the release vs after", "TIME", "lore", "284d + calendar", "READ NEXT"),
         st("H7.3", "VIX regime (low / mid / high) — range, and whether the extremes print at nodes more or less", "SIZE", "lore", "284d + ^VIX", "READ NEXT")]),
])

# ─────────────────────────────────────────────────────────────── X · CONTEXT
subject("X", "CONTEXT", "the modifiers every row above is split by — regime, calendar, volatility, the three books", [
    sub("X1", "Gamma regime", "TARGET · STOP",
        [st("X1.1", "+γ fade vs −γ follow — style of the day: reversion vs extension", "TARGET", "C30", "94 ep · book", "THIN", "rate flat (H3); extent unread"),
         st("X1.2", "Regime flip intraday (the zero-gamma crossing) — what happens to the extremes on a flip day", "STOP", "C30", "11d · book", "THIN")]),
    sub("X2", "Calendar", "SIZE · SKIP",
        [st("X2.1", "OPEX vs non-OPEX — the discount", "SIZE", "C-OPEX", "tap record", "OPEN"),
         st("X2.2", "Monday / Friday effects on extremes and range", "SIZE", "lore", "284d · price", "READ NEXT"),
         st("X2.3", "Event days from ForexFactory — skip or size?", "SKIP", "lore", "284d + calendar", "READ NEXT")]),
    sub("X3", "Volatility", "SIZE",
        [st("X3.1", "VIX level and change vs range and held rates", "SIZE", "lore", "284d + ^VIX", "READ NEXT"),
         st("X3.2", "EM band width vs realised range — the band as a range forecast", "TARGET", "ours", "11d · book", "THIN")]),
    sub("X4", "The three books", "SIZE",
        [st("X4.1", "Trinity alignment at the open — a modifier for every held rate", "SIZE", "C28", "tap record", "OPEN"),
         st("X4.2", "ES / NQ divergence at the extremes — the cross-market read", "SIZE", "lore", "284d ES + NQ", "READ", "xdiv in the model; event read pending")]),
])

# counts
tot = 0; by = {}
for sj in S["subjects"]:
    for ss in sj["subsections"]:
        for x in ss["studies"]:
            tot += 1; by[x["status"]] = by.get(x["status"], 0) + 1
S["counts"] = dict(studies=tot, byStatus=by)

if __name__ == "__main__":
    io.open("learning/studies.json", "w", encoding="utf-8").write(json.dumps(S, ensure_ascii=False, indent=1))
    print("wrote learning/studies.json ·", tot, "studies ·", by)
