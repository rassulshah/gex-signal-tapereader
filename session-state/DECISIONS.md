# DECISIONS — why the panel is the way it is

**Each entry is a decision that a later context would otherwise re-litigate or silently undo.** Some of
these record a decision I got WRONG and then corrected; those are the most valuable ones, because the
wrong version is the one that looks reasonable.

Format: **what was decided · when · why · what would change it.**

---

## D-1 · THE BAND IS ANCHORED AT THE OPEN AND HELD STATIC
**v11.49, reaffirmed v11.65.** `open ± dte0 ATM straddle`, both captured once and pinned.

Most published implementations (MenthorQ's guide among them) anchor the band to **current spot**. A band
that recentres on price is always centred on you, so **it can never show overextension** — which is the
whole reason this band exists. Anchoring at the open is also correct rather than merely useful: a straddle
captured at the open prices the move *from* the open.

⚠ The anchor pins the `rr` it was captured at. A live `rr` drifts with futures basis and walked the anchor
18 points across four separate causes (v11.59, v11.61, v11.63, v11.65) — one of which was two of my own
fixes running in sequence and double-scaling. **ONE scale, chosen once, applied once.**

**Would change it:** nothing short of abandoning the overextension read.

---

## D-2 · THE ROW SAYS "EXP LOW / EXP HIGH" THOUGH THE BAND IS 0.80σ
**v11.68 renamed it STRADDLE; v11.75 reverted to EXP at the user's instruction.**

The ATM straddle ≈ **0.80σ**, not 1σ (Brenner–Subrahmanyam; the standard conversion is ×1.25). A row
labelled EM/EXP implies ~68% containment; this band delivers ~**58%**.

The user chose the familiar label over the precise one. **The WIDTH was never changed** — every level sits
where it always did — and the 0.80σ caveat, the containment figure and the ×1.25 conversion all live in
the rail hovers.

⚠ Do not "fix" this by multiplying by 1.25 without asking. It moves every level the user has been reading
for weeks. The open question is whether the recorder eventually shows containment near 58% or near 68%.

---

## D-3 · THE NODES COME FROM SKYLIT (v11.77). THE ROUTE HERE WAS WRONG TWICE.
**v11.61 built them on Skylit; v11.64 moved to IF; v11.68 narrowed to `dte0`.**

Two justifications were recorded at v11.64 and **both were flawed**:

- *"113× magnitude difference proves the books are incompatible."* Skylit SPY totals ≈ $0.74B against IF
  SPX `toFri` $63.4B is **86×** — and SPY→SPX scales by **spot², which is (7674/765)² ≈ 100.6×**. The gap
  is the scaling plus a wider expiry window. **It was never evidence of incompatibility.**
- *"Skylit's legs are both positive, so it carries no polarity."* False. `callPut('SPY')` exposes a signed
  `net` per strike — SPY 765 net −131.2M, SPY 772 net +52.6M. I read the unsigned magnitude fields and
  concluded the sign was missing.

A third claim, made in a mockup on 2026-08-23 and also wrong: *"the two books disagree on the King by 16
points."* That compared **Skylit's SPY ladder** (765 → ~7702 chart) against **IF's SPX book**. Skylit also
publishes an **SPXW** ladder whose King is **7710**, one strike from IF's 7700. Apples to oranges again.

**RESOLVED v11.77 — the nodes moved to Skylit's SPXW tape**, on the user's instruction and the evidence
above. What forced it: SPX 7710 is **positive and small in all three IF windows** (+$88M/9%, +$219M/11%,
+$270M/4%) while Skylit calls it the **King at −100%**. Not a window artefact — all three checked. They
measure different things, as this codebase already said above `ifChain`: live accumulated positioning
versus open-interest gamma. **A stock beside a flow.**

Side effect that mattered more than expected: **the brakes came back.** Under IF every in-band node was
negative. Skylit shows +41% at 7650 and three more — the "hurdles" half of the feature was missing because
the wrong book was answering it.

⚠ Skylit gives ONE signed number per strike, so **the gross-vs-net trap is structurally impossible** on
this path. But per-strike dollars are DERIVED (`pct/100 × kingKd`), so **%King is the size** and the
dollar figure is secondary. IF's number was $/POINT; Skylit's is a node VALUE — never print Skylit
dollars with "/PT".

⚠ **Skylit's SPXW ladder has NO call/put legs** (`callPut('SPXW')` returns empty), so the SPXW route
cannot do the gross-vs-net split that removed an 84× overstatement at v11.68. Only the SPY route can.

**Would change it:** evidence that the two books disagree on polarity *within the same window*. See D-4.

---

## D-4 · TWO BOOKS ON ONE ROW — ✅ CLOSED v11.83
**Standing v11.64 → v11.77. LARGELY RESOLVED.**

    regime chip (−G −V) + BREAKS ...... SKYLIT
    everything else on ① FRAME ........ INSIDERFINANCE

v11.61 deliberately sourced the piles from Skylit *"so a pile can never contradict the regime chip"*. v11.64
moved them and **that guarantee was silently dropped** — the comment claiming it survived until v11.76.

They agree today (both negative gamma) **by coincidence, not by construction**. On a day where Skylit's SPY
tape reads +gamma while IF's 0DTE SPX book reads −gamma, the chip would read **FADES** above a rail of
purple accelerators and nothing would flag it.

**v11.77 took route 2.** The regime chip and the nodes now both read Skylit, so the guarantee is restored
by construction for the two elements that most needed it. The band, target, EM and FLIP stay on
InsiderFinance — which is the user's intended split: **IF prices the day, Skylit marks the levels.**
**v11.83 closed the last instance.** The flow chip stays on InsiderFinance — its per-point figure has no
Skylit equivalent, and recomputing it quietly would swap a disclosed mismatch for a hidden one — but it now
compares its own gamma sign against the regime chip's and **DECLARES a conflict**: red border, ⚠ prefix,
and a hover naming both answers. It hands the judgement back rather than pretending to settle it.
⚠ The rule this leaves behind: **when two books make the same claim, compare them and show the
disagreement. Never average them, never silently pick one.**

**The three routes, kept for the record:**
1. **Flag the disagreement** — keep both opinions, mark the chip when the signs conflict. Cheap, and the
   conflict itself becomes information.
2. **Move the piles to Skylit SPY dte0** — the user's intended architecture (IF for the expected move,
   Skylit for the nodes) and it restores the guarantee by construction. Requires verifying that Skylit's
   dte0 per-strike net agrees with IF's polarity at the same strikes first.
3. Move the regime chip to IF. Cleanest on paper, but the four-cell gamma/vanna doctrine was built on the
   Skylit tape, and **IF's payload carries no vega**, so the −V half would have no source.

⚠ Note the unexplained data point: at strike 7650 Skylit's SPXW ladder reads **+41 (positive)** while IF's
`dte0` reads **negative**. Most likely a window difference — IF's own sign flips by window — but it has
NOT been confirmed, and it is exactly the check option 2 depends on.

---

## D-5 · `dte0` MEANS "NEAREST LIVE EXPIRY", NOT "TODAY" — ✅ CLOSED v11.83
**Discovered 2026-08-23.** IF drops an expiry from the payload once it has expired. Captured after the
Friday close, the chain's earliest expiry is Monday, so `dte0` selects **20260824** while `today` is
20260821 — `dte0_isToday = false`.

During live RTH on a weekday the row exists and `dte0` really is today. After the close and at weekends it
is the next session's book, and the band's "today's expected move" is then **Monday's** straddle laid over
Friday's price action.

**CLOSED v11.83.** The rails carry **`≠TODAY`** beside the label, and the hover names the expiry actually
being priced. ⚠ Three states, not two: `false` = today, a date = that expiry, **`null` = cannot tell** —
collapsing unknown into "fine" is how a silent wrong day gets drawn.

---

## D-6 · ABSENCE OF DATA READ AS ABSENCE OF OBSTACLES — ✅ CLOSED v11.77
**Found in the hardening audit 2026-08-23, fixed the same day.**

`emPiles` returns `[]` on **six** distinct failures — no chain, chain error, `spot<=0`, no `gexProf`, no
ladder/`dispScale`, `maxMag<=0` — and `[]` renders as:

> *"Nothing sizeable between 7708 and 7730."*

A **false all-clear**. Nothing on the face or in `__gptsDebug.piles()` (which returns `{ok:true, n:0}`
either way) distinguishes "the path is clear" from "we cannot see the book".

**FIXED.** Every refusal in `skPiles` now carries a reason, and the sentence prints it:

    No node book right now — SPXW tape unreadable. This is not a clear path, it is no reading.
    No node book right now — SPXW tape thin (3 strikes). This is not a clear path, it is no reading.

⚠ **A Skylit markup change lands in that second message** — `SK_MIN_STRIKES` (⚖ 20; a healthy ladder reads
100) catches a degraded read before it can pass as a quiet market. And when the tape is unreadable
entirely, the IF fallback **announces itself** in the sentence, because the units and the meaning both
change with it.

The two smaller ones are closed too: a non-finite price refuses outright, and two nodes rounding to one
whole point can no longer become each other's destination. **All five guards mutation-tested.**

---

## D-7 · THE READ SENTENCE IS A MECHANISM, NEVER A FORECAST
**v11.70, wording settled v11.72 and v11.74 (both the user's own).**

    $6M negative gamma accelerator at 7668 can take price lower to the $6M positive gamma node at 7665.

Polarity is an **adjective on the level**, the consequence is **its own sentence**, and it names where
price can get to — the next node, or the rail when nothing lies beyond.

⚠ **"CAN", NEVER "WILL".** That single word is the entire licence for the line to exist. Every scorecard is
still at zero records; SpotGamma's own documentation states GEX answers *how sensitive is the market to
hedging here* and cannot answer *which way*; and converting hedging dollars into points of movement needs a
market-impact coefficient no option chain contains. `test_em_band.js` §30 EXECUTES the function and greps
the emitted sentences for forecast and instruction vocabulary. **The ban targets forecast and instruction,
not vocabulary** — "they sell strength and buy weakness" is mechanism and stays.

---

## D-8 · THE SECTION IS THREE ROWS AND NOTHING IS PRINTED TWICE
**v11.66 → v11.75.** chips · the rail · the sentence.

Everything removed along the way was removed for the same reason — it was already said somewhere else:
the pace chip (the sentence said "slow for the hour"), the path clause (the sentence says it better), the
used/left figures (the rail draws both), the repeated target (row 1 shows it), the replay badge (user's
call). **Before adding anything to this section, check whether another element already says it.**

⚠ Removing the replay badge accepts a real risk, and the risk is **Monday 08:00, not Sunday**: pre-open,
replay engages, the whole face shows Friday, and nothing says so. If it comes back, the section header
(`① FRAME · 08/21`) is the home that costs no row.

---

## D-9 · ES PRICES ARE WHOLE POINTS
**v11.75.** `7730.48` is not a tradeable price — ES moves in quarter points. `frameNum()` rounds to the
nearest whole point **on futures charts only**; `dispNum` is untouched everywhere else, and on a SPY chart
(~764) whole points would be far too coarse. Nearest, not toward-price — the user's choice.

---

## D-10 · `sampleTapeHistory` RECORDS A REPLAY AS IF IT WERE TODAY — SPX FIXED, **SPY OPEN**
⚠ **This entry shipped mis-numbered as a SECOND D-7** (the first D-7 is the mechanism-not-forecast rule
above) and was renumbered on 2026-08-23. `trackSpxwNodes` in the userscript still carries the comment
*"Recorded in DECISIONS.md D-7 instead"* — that pointer now lands on the wrong entry and should read
D-10 at the next build. Nothing else references it.
**Found 2026-08-23 while verifying v11.84.**

`sampleTapeHistory` keys every sample by `todayKey()` — the wall-clock date — and is **not** replay-guarded.
On a Sunday showing Friday's tape it writes **Friday's node values under Sunday's key.** Not wrong data;
**mislabelled** data, which is worse, because nothing downstream can distinguish it.

**v11.85 fixed the SPX path** — `trackSpxwNodes` refuses in replay and records why, before sampling.

⚠ **THE SPY PATH IS STILL UNGUARDED, DELIBERATELY.** It has run this way for many versions and there is a
plausible reason: the node chart needs history to draw, so sampling during replay may be intentional.
Silently changing the keying of a long-running path on a hunch is its own risk.

**The likely correct fix, when someone takes it:** key by the session BEING SHOWN (`sessionDayStr()`)
rather than the wall clock. The chart keeps drawing in replay AND the data is labelled with the day it
actually came from. **Verify against the node chart before shipping it** — an empty chart in replay is the
symptom of getting this wrong.


---

## D-11 · THE LLM LAYER — END-OF-DAY FIRST, LIVE LATER, AND NOT FOR PREDICTION
**Discussed at length 2026-08-24. NOTHING BUILT.** The user wants "an AI Tapereader that gives a
read" — bullish or bearish, explicitly **unconstrained**, horizon session/swing rather than one bar.

**The honest finding, and it is the whole decision: for PREDICTION a live LLM is the weakest option
available.** GEX→direction is a weak-signal tabular problem — ~370 numeric columns, faint edge, heavy
noise. On that shape an LLM loses to a fitted model and to the existing `directionGrade`, which is at
least deterministic and scored. The recorder ALREADY produces the supervised dataset: `matrix`, one
row per bar, 370 columns, four labels.

Where a live LLM genuinely wins is **comprehension, not prediction**:
1. synthesis under time pressure (the FRAME review's own complaint about gauge-dashboards)
2. **"this day does not match the shape your rules were tuned on"** — a rule cannot report that its
   own preconditions are absent, and every threshold here is hand-set
3. interrogation — "why bearish?" against this bar's state
4. hypothesis generation → candidate rules to test and encode

**The only plausible live EDGE is information the panel does not already have** — econ calendar,
opex, overnight session, VIX term structure, cross-asset — or **retrieval over the user's own
recorded history** ("the last 14 times the book was >80% above price in −G−V..."). An LLM competing
with `directionGrade` on identical inputs is not adding anything.

⚠ **Measured, not assumed:** a day export is **5.9 MB**; the whole live tape state is **~6 KB**
(the full 100-strike SPXW ladder is 1.4 KB). The archive needs a digest. The live read does not.

⚠ **`@grant none`.** There is no `GM_xmlhttpRequest`, and a plain fetch to the API is CORS-blocked
and would expose the key to Skylit's page JS. The route is a **127.0.0.1 relay**, NOT a grant change:
any `@grant` moves the 1.3MB script into Tampermonkey's sandbox and out of page context, where it
currently reads Skylit's internals.

**Order agreed:** end-of-day reviewer first (no relay, no cost while trading, and it accumulates the
history a live layer would need); revisit live at ~20 recorded sessions.

**Would change it:** enough recorded sessions that a fitted model can be compared against an LLM
head-to-head on `dirHit`.

---

## D-12 · THE READ STRUCTURE, IF THE LLM LAYER IS EVER BUILT
**Agreed 2026-08-24.** Three tiers, because they are scored differently.

    CLAIM       verdict (bull/bear/neutral) · horizon (leg/session/swing) · conviction 1-3
    EVIDENCE    regime · balance · path · participation · location
    BOUNDARIES  gate (the price that arbitrates) · invalid · confirm

Plus **delta** (`changed` since last read, `standing` = bars this thesis has survived) and
**provenance** (`bar`, `promptVer`, `inputHash`, `model`, `books`, and `cited[]` validated against
the ladder it was given).

The three load-bearing fields: **`horizon`** (without it nothing can be graded and it defaults to
1-bar noise), **`invalid`** (separates *wrong* from *early*), **`balance`** (the only element that
cannot be got from a chart — make it a number, gamma above vs below plus distance to the nearest
opposing-polarity node, never an adjective).

⚠ A session-horizon read cannot share `dir`'s outcome window. `FEAT_FWD` scores at 5 and 10 bars;
grading a session thesis on 15 minutes grades noise. It needs its own horizon (~40 bars / to-close),
**which also means its scorecard takes ~3 weeks of sessions rather than 3 days.**
⚠ Conviction gets scored by level or it comes off. A confidence figure nobody checks is worse than
none, because it feels like information.
⚠ §30 of `test_em_band.js` executes the read composer and fails on forecast vocabulary. An AI read
making real directional calls will trip it — that test must be scoped to the MECHANISM sentence
before any such feature ships, or the build goes red for doing what was asked.

---

## D-13 · THE v11.88 UI SCOPE IS ONE CHANGE
**2026-08-24, after four rejected mockup revisions.**

    put the current price in the white circle. do not make it big.

That is the entire approved scope. Everything else offered was explicitly rejected: +50% then +33%
type, a node-role tier above the rail, staggered label shelves, a PACE chip, a SUCC chip, four
rewritten hovers, and used/remaining beside the price box.

⚠ **The failure here was mine and it was procedural, not technical.** The skill carries a standing
user-mandated rule — *"ONE AT A TIME: discuss exactly ONE element per message. Never list all open
items and their fixes in one reply. State the one item, its fix, ask, STOP."* I broke it in nearly
every reply and each redesign had to be walked back. **Four revisions burned.** The user ended the
session with "we need to stop . you are making a mess."

**Would change it:** the user asking for more. Not inference that more would be better.

---

## D-14 · ② BIAS — PA SHADOWED, CROSS AND ROLL VOTE, AND THE TALLY IS RECORDED
**v11.88, 2026-08-24.**

**PA out.** It reads the same price series the 50-SMA reads, so in a trend it agreed close to
mechanically — a confirm correlated with what it confirms inflates the count without adding evidence
(Pattern 7). ⚠ **Shadowed, not deleted:** still computed (④ REACTION's PRICE row needs `paRead`), now
recorded with `paWouldConfirm`, and `bias_pa_shadow` is the question that gives it its seat back if the
argument was wrong. **A removal you cannot measure is a preference, not a decision.**

**CROSS in.** SKEW, ACCUM and ROLL all read the SAME option book; two instruments agreeing is the only
independent evidence available. ⚠ **It is not `trendVerdict`** — QQQ has no candles at all, so both
sides are measured by one rule on one field (`j.levels[i].s`, ~390 points/389 min per feed) at horizons
matched to the SMA: 150-minute average, 60-minute window, 45-of-60 dominance = TREND_DOM's 15-of-20.
Comparing a candle trend against a snapshot trend would be the apples-to-oranges error again.

**ROLL in, overriding a prior decision.** `dir.kingRoll` has said `RECORDED not voted` since v11.0
because whether the King migrating LEADS price was an open measurement. It votes from v11.88 **at the
user's instruction**; `kingroll_leads_dir` still settles it.
⚠ Exposed a real bug: `kingRoll()` returns **0 for both "has not moved" and "no history"**. Fine for a
recorder, wrong for a vote — 0 counted as live-and-neutral. `kingRollRead()` separates them.

**The tally is recorded for the first time.** It never was: 224 bars, no `nConf`, no confirm directions,
while every NON-voting candidate carried an explicit `vote` field. The v11.36 premise this section was
rebuilt on was therefore untestable. Enrolled as `bias.confirm`.

⚠ **The denominator changed (3 → 4), so pre-v11.88 sessions cannot be pooled with later ones on the
confirm count.** Nothing was pooled before because nothing was recorded, which is why the change was
made now rather than after the data started.

⚠ **`confColour(nConf, nLive)` judges a FRACTION.** The old rule was `nConf>=3` for green — a hardcoded
denominator inside a renderer, which would have made green unreachable the moment the list changed
length. Extracted so it can be tested.

**Would change it:** `bias_pa_shadow` showing PA carried information after all, or `bias_cross_agrees`
showing CROSS adds nothing the symbol's own book did not already say.

---

## D-15 · THE TREND REVERSAL THRESHOLD IS 11, RAW
**v11.89/11.90, 2026-08-24, user-specified.**

A NEW trend confirms at 15 of 20; a REVERSAL out of a broken state confirms at 11. The break is itself
evidence and the extra 4 bars are lag. ⚠ **RAW — the user declined the slope gate.** The 50-SMA spans
150 minutes, so 11 bars below a still-rising average is a pullback, and raw calls DOWN there.

⚠⚠ **The cost is symmetric and was quantified before shipping:** once a trend has confirmed once, both
directions flip at 11, and the minimum gap between opposite flips falls from **10 new bars to 2**.
A test pins it so it is never "fixed" by accident. **If the data shows whipsaw the fix is a DWELL TIME,
not a retreat to 15.**

**Three machines are recorded, one ships** — loose, strict 15/15, and slope-gated — each with its own
`prior`. A shadow that shares `TREND_LAST` is an echo. `trend.machine` asks all three questions.

**Would change it:** `trend_loose_whipsaw` showing fast flips fail, or `trend_gate_would_have_helped`
showing the gate was right.

---

## D-16 · DRIFT IS A BADGE THAT DOES NOT VOTE
**v11.90.** It joins the confirm row for consistency and to save a row, drawn **outlined behind a
divider** and excluded from `nConf`.

⚠ **Two prior decisions must not be undone by this cosmetic change.** v11.44 made drift a gate because a
tick meant "the books agree with EACH OTHER" while they agreed on the wrong side; the `withCall` guard
is untouched. And the user shadowed drift on 2026-08-18 pending proof — measured 2026-08-24 at 25% on
effN 10 against a 21% baseline over 2 sessions, it has not earned promotion.

**Would change it:** drift clearing the promotion path it has always had — n≥20 effN, 3 sessions, both
up and down days.


## 2026-09-03 — the simplification (v15.53)

- **IRT export, Yahoo Finance, InsiderFinance: "i need the irt export it is very important, also the yahoo
  finance, insider finance so dont mess with the integrations."** Protected in full, including dormant lanes.
- **Dark-pool lifecycle: archive it.** Capture and the level reader stay.
- **Pop-out: keep the window only.** PiP archived.
- **Face hierarchy: keep the ladder on top.** ⓪a stays below it (my recommendation was the reverse; his call).
- **"perform the simplification"** — go, in the plan's order: defects + dead code first (v15.53), merges next.


## 2026-09-03 — the Analysis tab, by subject

- **Of five alternatives (by subject · doctrine scorecard · setup grader · conditions matrix · session timeline)
  he chose A · by subject: "it provides a subject which can then be added to."** My recommendation was the
  three-moment layout with the others folded in; his call, and the registry makes it extensible.
- **"Think like a trader … We don't want information for the sake of information."** Every study carries
  `decides` (SIZE / SIDE / TARGET / STOP / SKIP / TIME / LEVEL / WAIT). A study with no action does not go in.
- **Sweeps get their own extensive subsection (H2, 12 studies)** — ONL/ONH, PDL/PDH, IBL/IBH, by clock, depth,
  speed, acceptance, sweep × node (H6 draft), the early sweep re-read (H7 draft).
- **Testing is redesigned to the same subjects** — register, gate, dashboard, record, nightly, suite; ids shared
  with Analysis; a hand rule against a registered null is FLAGGED (kill.negGammaWide ↔ H3).

- **TRACK field (v15.55):** one per subject, not per subsection — "I want each section … a text field … click a
  button like add which will trigger you to do your part." The request rides in the day export → nightly →
  learning/requests.json → the review turns it into a study row carrying `req:<id>` → the row comes back to the
  field. The trigger IS the Save; nothing reaches the review without the day file.
- **THE READ FROM THE STATS (v15.55):** lives in ⓪a under the deflection ledger. It reads today's sweeps on the
  courier's ES bars against SWEEPS.json, the node clause from the deflection ledger, the register's word. The
  node-conditioned rate prints UNMEASURED until H6 fills — never a guess dressed as a rate.
- **Other sweeps (his question):** PDC, POC/VAH/VAL, PMH/PML, PWH/PWL, OR5/OR15 measured the same day (F-14):
  none beats the fresh-low control. CW0/PW0/King/VWAP sweeps need the book at the sweep — H6 / the tap record.
- **The book's levels as sweep levels (v15.56, his ask):** CW0 / PW0 / CW / PW / KING, valued as of the sweep bar,
  side by POSITION against the open (a PW0 above the open is a high-side level whatever its name). The node clause
  uses Skylit's own tap zone (±0.50 SPY = ±5 ES, C2). The book corpus is the panel's own day files at 3-minute
  resolution — one scale, no ratio — and it is thin (9 sessions) and says so; every export adds a session.
- **Companion v1.18 widens ES to the full Globex day.** Not "messing with the integration": the same courier, one
  more flag, so the live ONH/ONL are the overnight the corpus defined. NQ/GC/CL keep the trim.
- **RATE_MIN_N = 15:** a cell under 15 prints thin with its n everywhere the sweep machinery renders a rate.
- **Levels added (v15.57, his approval of my five):** EM edges, VWAP + bands, developing POC/VAH/VAL, London range,
  HVL/magnet. Skipped by agreement: midpoints, floor pivots, round numbers, weekly/monthly, naked POCs.
- **The two-line rule (his):** at most two sweep lines on ⓪a, the biggest / most popular levels first (tier 1 = PDH PDL
  ONH ONL VAH VAL POC KING EMH EML), then depth; the rest named in one trailer.

## 2026-09-03 — the WHAT and the HOW are both pinned; the plan is incremental

- **"never forget my objective (the what) and the machinery and process (the how)":** PURPOSE.md was already pinned
  (test_purpose.js); PROCESS.md now carries the HOW and is pinned (test_process.js); both sit at the top of the skill's
  load order, in `.gex-config.json` (`theWhatAndTheHow`) and in a standing block at the top of the resume note.
- **The roadmap is `roadmap/ROADMAP.md`**, versioned with a definition of done per item; older roadmaps are history.
- **The inventory** (`design/DASHBOARD-INVENTORY.md`): every face element mapped to the objective and its study;
  three elements (MARK, STATE, polarity colour) imply a claim they cannot back — their hovers will say so (v15.59).
- **A ⚙ Process tab: recommended, as live status, not prose** — v15.60 in the roadmap, pending his agreement.
- **The WHAT and the HOW live in the app (v15.59, his ask):** ⚙ Architecture (objective + the loop as live status) and
  🗺 Roadmap tabs, from `learning/plan.json`; the roadmap re-ordered so the tabs shipped first. A HOW tab is live status,
  not prose.

## 2026-09-07 — the IRT export mirrors Skylit's top-5: G2–G5, the King's slot is the gold line (v15.76, R-13)

- **His ask:** "exports the top 5 levels for the spx also. G1 - G5. all should be white. in addition to this, it already
  exports the kings, cw0, pw0 and the flip." **The question asked:** is G1 the King? Skylit's NODES setting is a top-N
  by |%King| (1/3/5/10/15/20 — measured, SKYLIT-FEEDS.md) and NODES=1 is the King alone, so its top-5 is King + 4.
  **His answer: "Mirror Skylit: G2–G5."** The King's slot on the ES chart is the SPXW KING line already exported (gold);
  the four that follow are G2..G5, white, width 1, solid. **No G1 row** — one level, one line.
- **One array, one conversion:** the ranking reads the same `T.pct` the SPXW KING row read (tapeMap, the rendered
  ladder strip — v13.2) and both go through one closure (`spxRow`: live ES chart → dispScale/R; else undScale; else
  dispScale/R). Sign is ignored in the ranking — polarity is a character, not a size (execution doctrine).
- **The strike dropped is the EXPORTED King's** (the latched crown), not the tape's 100%: during a flap the new crown
  shows as G2 rather than vanishing. A level under a lesser label beats a level absent from the chart.
- **Held like the Kings** (v14.74, same key, same day scope): a blind tick holds four lines instead of erasing them.
- **The v14.20 "only the kings" contract** is a ban on levels nobody asked for; each family since returned in his
  words (0DTE trio v14.79/80, QQQ bearing v14.75, G2–G5 v15.76). Nothing else may join without the same.

## 2026-09-07 — STANDING RULE: match Skylit, always — it is the source of truth the levels are compared against

- **His words (after v15.76 shipped):** *"We should match with skylit always in order to have a source of truth to
  compare against."* What the panel and the IRT export DRAW must mirror what Skylit itself shows for the same setting
  (the ladder strip, the NODES top-N, the overlay's conversion), so that his Skylit chart is the reference the IRT
  lines are checked against. This extends SOURCE-OF-TRUTH.md (the Academy governs the DOCTRINE) to the NUMBERS and the
  LEVELS: Skylit's own rendering governs what a level is and where it sits. A deliberate difference must be named in
  the file (a tag, a label) and in DECISIONS, never silent.
- **Where the export matches today:** the SPXW book is Skylit's own ladder strip (`tapeMap`), %King verbatim; G2–G5 are
  Skylit's NODES=5 ranks 2–5 by |%King|; the ES conversion is the same live basis Skylit's overlay uses on a futures
  chart (v14.13/14.14); on a closed day both stand on the last session's book.
- **The one designed difference, for his decision:** the King LATCH (`kingLatchTick`, `KING_LATCH_MS` = 2 minutes of
  continuous hold before the crown moves; v14.19) — during a crown flip the rail and the export keep the old King for
  up to two minutes while Skylit shows the new one. Kept so a mid-flap blip does not move the line on his chart; under
  this rule it is a named deviation. Open: keep it (with the difference explained) or follow Skylit's King instantly on
  both the rail and the export (they must agree with each other either way).

## 2026-09-08 — the E row per weekday: his seasonality (v15.77, R-14); the hold rates stay pooled

- **His ask and his choice:** the expected row on top of the HOD line, minus eleven fields (Rly · Done · PB · Num ·
  Ret · Risk · Ext · Tgt · Rwd · Dur · Time); of two mockups (all sessions n=284 · the weekday n=55) he chose the
  weekday: *"it compares friday with fridays in the past and mondays with past mondays … this is a type of
  seasonality."* **Every EXPECTED field is the shown day's weekday's**; the basis and its n are printed beside the E.
- **Measured before deciding (ES corpus, 55–60 sessions per weekday):** Fri ~19 m to the first extreme vs Tue ~46 m;
  Thu ~70 pts range vs Tue ~54; Wed LOD-first 42% vs Tue 60%. The weekday is real here. Conditioning on the weekday
  AND the first extreme leaves ~24 sessions — not honest — so the row conditions on the weekday only.
- **The LADDER (hold rates by age) and the lookup table stay POOLED.** Split five ways their rungs go thin, and they
  answer "has the extreme printed", not "when is it expected". The hover says which is which.
- **The weekday is the SHOWN session's** (a replay's or the closed state's day; the recorder's when live; a weekend
  never counts), so Friday's park on a Saturday reads Fridays.
- **The colour chip is a COUNT** (the last six sessions of that weekday, RTH close vs open — his tool's "last 6 per
  weekday"), never a rate, and it is marked STALE when the corpus has not been appended within 14 days of the shown
  day. ⚠ The nightly does not yet append the corpus (the plan's hodlod ③–④ are prose) — v15.78 wires it.
- **The read's timing prose reads the same base object as the row** and names the basis — one quantity, one source.
- **His standing principle, generalised:** track ranges and other statistics by weekday too (his two charts: red vs
  green by weekday, range by weekday vs the 10-week average) — v15.78, mockup first.

## 2026-09-08 — the day candle beside the ladder, with the sweep labels (v15.78, R-15)

- **His ask, on a screenshot of the space to the right of the ladder:** the developing daily candle there, with the
  labels the candle already had; then, on the mockup, "add the sweep labels."
- **The sweep labels are the SWEPT line's events** (`sweepEventsToday`, tier 1–2), in its colours and with its hover
  words — one source (v13.2). They sit RIGHT of the bar at the wick tip of their side; the reversal names (the levels
  the wick turned ON) sit LEFT. THROUGH and ON never share a column.
- **Room or nothing:** the slot is filled only after layout, when the grid's flex line leaves ≥110 px; the candle is
  the grid's rendered height; on a narrow panel the face is exactly what it was. The candle is capped at 260 px wide.
- **The small candle (the DAY table's) is unchanged** — `dayCandleSvg` without a frame argument draws what it drew.

## 2026-09-08 — the E row below the ladder, bigger; one door for "which day" (v15.79, R-16)

- **Placement (his):** under the ladder and the candle, above the SET line and the replay strip; the HOD line and
  SWEPT stay on top. **Size (his, from two mockups):** values 10.5 px, labels 8.6, chips 9, spread across the row.
- **The sweeps follow the shown day.** The candle and the SWEPT line read `sweepEventsShown` — today's events live,
  the replayed / parked day's in a replay or the closed state (inside the courier's 5-day window; none beyond it).
  A parked Friday face wears Friday's sweeps. Two readers of "which day" must go through one door.
- **The courier's day keys compare as numbers**, never as strings (the 10th-before-the-8th bug).
- **Mockups ride the installer by last commit, six of them** — the age that survives a clone.

## 2026-09-08 · v15.80 — the candle wears HIS key levels on the price axis; one source for the shown day

Operator, on the v15.79 candle: **"the levels that are swept are the only ones that should be indicated. EMH and EML are not levels
and they should be aligned based on the y axis which should be the price axis so the candle should show where it
swept the level."** Then: **"the king is not a key level. the key levels are PDH, PDL, ONL, ONH, WH, WL,
Prior day POC, VAH, VAL, Weekly Poc"** and **"I dont want IBL IBH PDC. you can keep CW0 and PW0. and POC is the prior
day poc. VAH and VAL is also prior day VAH and prior day VAL."**

Decided: `LEVEL_TIER` tier 1 = exactly that list (PWH/PWL/WPOC by name until the companion carries two weeks); the
King is tier 2 (structure — a sweep at the King is still a sweep, H6); EMH/EML, PDC and the IB are out of the sweep
set entirely (the band still draws as the band; the corpus keeps its IB/PDC cells as studies); the candle draws tier 1
only, each at its own price, with the minute it swept; the reversal names (v14.99) are gone from the candle; MUD and
the reclaim line (↩ W.END · BOP) sit beside the open tick, left of the bar. The candle, the A row and GREEN/RED measure
the shown day's ES 1-minute bars in a replay / the closed state (frames only when the courier has no such day);
`closedCandles` keeps the frames for the band, the ATR and the trend machine. The MUD dollars are converted once.
ONH/ONL are the full Globex night (17:00 → 08:29), FULL when both halves exist. A key with no RTH bars is not a
session (the prior session skips a holiday; the night before it does not). WH/WL/weekly POC wait for the companion's
window (asked separately, one at a time). LDNL/LDNH (mine, v15.57) stay as tier 2 until he says otherwise.

## 2026-09-08 · v15.80 — the IRT export: QQQ converts for NQ only

Operator: **"currently the irt export includes qqq converted for ES, remove this. qqq should only be converted for
nq."** Decided: the v14.75 projection of the QQQ King onto the ES symbol (a dashed `QQQ KING ~` row, the rail's own
bearing) is no longer written; the QQQ King writes on the NQ symbol only. The rail's `~ QQQ` pill on the face is
unchanged (a bearing, never a level). Supersedes the 2026-08-28 "qqq → NQ → ES" ask, by his own instruction.

## 2026-09-08 · v15.80 — the A row, MU / MD, the colours, the expectation chip; the IRT lines and the NQ G rows

Operator, in sequence: **"Under the Expected row add the Today (Actual) row."** · **"MUD should be dynamic, MU or
MD"** · **"Try to use common sense colors for the expected and actual rows. for example green or red. MU should be
green, MD should be red."** · **"there should also be the expectation for the day, which is either a Green day or a
Red day. you can place this before 1st HOD and dont need other badges that are there before 1st hod like 3/3 even, so
you can use this space instead"** · **"in the export for irt, i dont want dashed or dotted lines, make all solid
lines"** · **"Can you add additional levels for NQ as well, similar to how you have it for ES, so NQ would include
G2-G5 also"**.

Decided: the A row (today's actuals, the E row's columns) sits directly under the E row; MUD reads MU (green) to a
HOD and MD (red) to a LOD everywhere on the face — the record field stays `mud`; HOD cells green, LOD cells red, the
1ST chip in the extreme's colour; the E row's first badge is the day's expectation = the v14.91 green/red call
(GREEN DAY / RED DAY, `DAY ?` before it fires), the recent-six count lives in its hover and is no longer a badge;
the IRT export writes every line solid (PENSTYLE 0), and the NQ symbol carries G2–G5 from the QQQ book by the ES
rule (King dropped, |%King| rank, held day-scoped).

## 2026-09-08 · v15.80 — the E and A rows are one grid, tagged E · TUE / A · TUE

Operator, after the first v15.80 installer: **"the rows should be aligned. the firt badge can simply be E Tue and A Tue
or something like that so the entire row is symmetrical."** The two rows had been two flex-wrap boxes with tags of
different widths (`E · TUE n=60` / `A · TUE 8 SEP · so far`), so nothing below the tag lined up.

Decided: the two rows are the two rows of ONE thirteen-column grid (`.g3eag`; each row `display:contents`), so every
column is one track top and bottom; the tags are the letter and the weekday only (`E · TUE`, `A · TUE`; `E · ALL` when
the weekday block is missing) and everything else the tag used to say — the n, the basis, the date, "so far" / "as
it closed" — is in the tag's hover; a badge slot with nothing to say still holds a chip (`DAY ?`, `1ST ?`) so both
rows are always thirteen cells. A max-content grid cannot wrap, so below the width of the thirteen columns (~900 px;
his panel is 943) `eagFit()` returns the rows to wrapping — aligned where it fits, readable where it does not. Not
decided: a narrower set of columns for a narrow panel — he has not asked, and the panel he runs is wide enough.

## 2026-09-08 · v15.81 — the IRT export prints Skylit's own futures prices (R-23)

Operator, after the projection reading and one question: **"yes, i want them to match skylits own ES1 prices"** · **"why
weren't they done this way before?"**

Decided: the FlexLevels file's ES rows — SPXW KING, G2–G5, SPY KING, CW0 / PW0 / FLIP0 — and its NQ rows — QQQ KING and
its G rows — take their prices from the futures books Skylit itself publishes (`gex/levels?symbol=ES1` / `NQ1`, whose
`derived[]` carries every SPY / SPXW / QQQ strike already at the futures price by Skylit's live ratio: the derived orbs
on his chart); the panel's own basis (the ES/SPY EMA; the IF spot for SPX) is the FALLBACK only, tilde-tagged as before,
used when no payload has arrived or the held one is older than five minutes. One ruler in the file: the IF 0DTE rows
(an SPX chain) convert by the same SPXW ratio as the SPXW King, never by the IF spot beside a Skylit-priced King. The
read/record pipeline does not consume these payloads — the face's own basis machinery is untouched. Why: "match Skylit
always" (2026-09-07) — the two rulers were 0.5–1.5 pt apart at the same second, and his chart's orbs are Skylit's.
Not decided: the projection's own numbers (the target price, the cone, the regime) on the face or in the file — beta,
unmeasured; recorded in SKYLIT-FEEDS for when he asks.

## 2026-09-08 · v15.82 — the outcomes are exported from the archive; the queue is a window, not the record (R-22)

Operator, on the outstanding list: **"lets go with your recommendation"** (R-22 first).

Decided: the day file's `feat` is the localStorage queue ∪ the IndexedDB archive of every resolved record, merged by
`key|t` with the LS copy winning (it is the live one), and the file says what came from where (`featSource`); the same
union is what `featStats` — the live face's truth — reads for every day. The localStorage queue is a WINDOW the
shedder may trim; the archive is the record. Not done, on purpose: the snapshot mirror stays in localStorage (its
readers are synchronous and the archive already has every bar), and the shedder is not touched — the v14.76 shedder
went wrong once by being re-aimed, and with the queue's truth in the archive there is nothing left for it to break.
Why: F-20, mechanism found live this morning; every rate in the learning layer had been an afternoon-only sample since
08-25, and the far-side-on-gamma study is blocked on clean sessions that this alone can produce. Forward-only: the days
already written stay as they are.

## 2026-09-08 · v15.83 — the SPY book's top five in the file; every node line signed and coloured by polarity (R-24)

Operator: **"i currently have top 5 nodes for spx. Im thinking of having the top 5 for spy as well"** · **"i updated to 5
… is that showing me the top 5 from both spx and spy"** (it is — `nodes=5` per borrowed book) · **"ok lets go with this.
the spy gamma node lines can be beige"** · **"also add the % to each of the node lines except the king"** · **"add a + or
- too so i know the polarity"** · **"infact, i want you to redo the color schemes for the nodes, using yellow and its
derivatives for positive nodes and purple and its derivatives for negative gamma nodes."**

Decided: the file mirrors what NODES = 5 draws — the top five of EACH borrowed book: SPXW KING + G2–G5 (as since
v15.76) and SPY KING + **S2–S5** (new), the QQQ King + its G rows on NQ; no strength floor on the S rows (the chart has
none at NODES = 5). Every node line's label carries its **signed %King** (`G3 -56%`, `S2 +55%`); the Kings stay bare
(v14.73). **The colour of a node line is its polarity**: the yellow family for +gamma (the brake), the purple family for
−gamma (the accelerator); the shade tells the book — the SPXW / QQQ nodes one step lighter than their King, the SPY
nodes the palest, the SPY King the lighter pair it already was. White is gone from the node rows; beige (his first
word for the S rows) was superseded by the scheme in the same hour and is not to come back. The IF rows keep his
08-28 colours (walls red / green, the flip purple): a different book, a different question. Not decided: whether the
labels should also carry the strike (`S2 767 +55%`) — he asked for the % and the sign; the strike is in the price.

## 2026-09-08 · v15.84 — the panel's own SPY / QQQ book is the 0DTE chain with every strike, whatever the app asked last (F-22)

Operator: **"double check"** (the file against Skylit at the same second) — the SPX side exact, the SPY side two Kings
in two minutes. Found: the self-fetch inherited window and breadth from the last gex/levels URL seen (the app's three
projection windows; the panel's own expiry sets through the same hook), and had since v10.48.

Decided: **the book the pipeline stands on is ONE window and ONE breadth — `exp_mode=current&exp_count=1&nodes=500`,
pinned in the self-fetch, for gamma and vanna** — the window the ladder, the tape and the derived orbs already were.
`onFeed` refuses a multi-expiration gamma payload once a single-expiration book is held, counts it, and never refuses
with nothing held (blind at boot is worse than one wrong minute). "Match Skylit always" (v15.81) meant Skylit's PRICES;
this is Skylit's WINDOW — the front chain — which is what the Academy's tap decay, the King margin and the node lifecycle
are written about. Not decided (recorded): a breadth guard for a SPY / QQQ chart, where the app's own five-row payload
still arrives through the hook; and whether the week / wk7 expiry sets should feed anything at all now that nothing
reads them but the expiry profile. Forward-only: the sessions before v15.84 are a mixed-window sample for every
node-level question (F-21 flagged); the King-level series survive. **The level study's clean sample starts 2026-09-09.**

## 2026-09-08 · v15.85 — the node lines labelled by book: XG2–XG5 (SPX), SG2–SG5 (SPY); the QQQ King on NQ only (R-26)

Operator (evening): **"in the irt export, the spy gama level cand be SG2 SG3 etc., and the spx levels can be XG2 and XG3
etc.. make this change. also note the qqq king should only be on the nq not the es."**

Decided: the label names the book — **XG** = the SPX book's gamma node, **SG** = the SPY book's — the rank after it, the
signed % after that; the Kings keep their names; the colours stay the v15.83 scheme. The QQQ King is on the NQ symbol
only (already so since v15.80, R-18; pinned again). Not decided (asked, one word): the NQ symbol's QQQ lines — `G2–G5`
as today, or `QG2–QG5`. A hold latched under the old labels is dropped rather than translated: the next fresh book
re-latches within a tick, and a translated label is a label nobody asked for.

## 2026-09-08 · the tap record (R-25) — designed on paper, nothing built; the first hand read is one day

Operator: **"are you running a study on the gamma levels deflection. basically testing spx and spy and qqq top 5 gamma
levels and their ability to deflect and if they do deflect, under what conditions and if they dont deflect, under what
conditions."** · **"the purpose of the study is to identify what makes gamma levels work as deflectors of price for the
purpose of trading reversals from levels."** · **"yes"** (to the design) · **"I am also trying to determine if it is
sufficient to have the top 3 instead of the top 5. can you look at the deflections today and see if the top 3 is
enough?"**

Decided: the study is a CORPUS, not a new ledger — `design/TAP-RECORD.md`: one row per tap of an EXPORTED line (the
lines he trades from, by book and rank, at Skylit's price), two controls in the same row format (the rest of the 0DTE
book ≥ 20%; the midpoint between adjacent lines — the doctrine's "imaginary support", the honest zero), the conditions
at the tap, the outcome in trading terms (DEFLECT · PIN · BREAK; mfe · mae · reach · R), nightly by book × rank and by
condition, every cell with n beside its control; H11 (fresh + growing > tested or shedding > midpoint) registered the
day the record starts; the clean sample begins 2026-09-09 (F-22). Built one element at a time, on his word, in the
order the design lists; nothing on the face until the report's mockup. Why not the existing ledgers: the deflection
ledger holds no failures (no denominator); the node ledger is per bar (F-12) on a mixed-window book (F-22); neither
knows a line's book or rank. **The top-3 question:** today's hand read (34 taps) says ranks 1–3 carried 15 of 18
deflections and both extremes, ranks 4–5 carried all three breaks and two pullback turns — one day; the answer he was
given is "keep 5 for two weeks while the record accumulates, then decide with n".

## 2026-09-08 · v15.86 — his training: ten circles, the liquidity levels beside the gamma levels, the path to predictions

Operator (night): **"here are 6 deflections identified by the circles. each should be counted as 1 deflection. you should
analyze the time and price and gamma node with all its details. in the future, you should be able to identify them like
i am identifying them, and score them to predict the deflection in advance. see also if there were any type of patterns
as well as levels there like ONH ONL, PDH, PDL, Asia High Asia Low, London High and London Lo, prior day poc, vah, val.
i was not tracking sweeps of the asia and london highs and low, but we should do it. they can be ALO, AHI, LLO, LHI. you
will find that the liquidity levels and gamma levels may increase the probability of deflection, so you must look at
both and track both, so you are able to better score and predict a potential deflection will happen and give me a entry
or exit target in the future, but first you must get really good at identification of deflections and levels and both
as well as patterns like rug, rrug, piku stack, barney stack etc.. only after you put all of this together over time
will you start giving me valid predictions. Espeicallay after adding hod lod statistics as well as things like volume.
so make sure you track, score over time to get to this level of competence and reach my intended purpose of the
project, which you should always keep in mind."** · **"here are 4 examples from the NQ to also help you learn"** ·
**"ok.. i need you to record and let me know in about a week from now.. keep it as an open item."** (top 3 vs top 5)

Decided: (1) **a circle is one deflection** — E005 / E006 carry his ten as ten legs, each read against the ladder's
ranked lists, the node-event dollars, the bars and the day's levels; the taught examples are not scored (the protocol).
(2) **The liquidity levels are tracked beside the gamma levels at every deflection** — the `liq` factor; **AHI · ALO ·
LHI · LLO** join his key-level list (LOCKED-ITEMS; the session boundaries Asia 17:00–02:00 CT / London 02:00–08:30 CT
are proposed, his word awaited). (3) **The order of competence is his**: identification of deflections, levels and
patterns first; scoring over time; predictions with entries and targets only after — with the HOD/LOD statistics and
volume added along the way. The gauge on the Learn tab stays honest (blind reads only); the tap record (R-25) is the
scorer. (4) **Top 3 vs top 5 is an open item (Q12), his call on or about 2026-09-15** — he keeps 5 until then.
Recorded for his ruling, not decided: the first-hour turns he did not circle (ES 08:45 / 09:03 / 09:39 / 09:51; the
NQ LOD 09:09 at the PDL sweep) — whether they are deflections in his sense.

## 2026-09-09 · v15.87 — the A row on his tool's grid ("tools"); the corpus appends itself ("yes"); the nightly re-runs after an install

Operator, on the A row's 14% wick with the HOD "at 8:30", over three rounds: **"how is there a wick when the hod
occurred at 8:30. how is that possible. do you realize that the candles are session candles and that if the hod was at
8:30, then it never wen above the open, so how can there be a wick?"** · **"lookup the days candle using yahoo or any
online resource to double check"** · **"are you saying that the 14% of the session was done in 1 minute, creating a
wick"** · **"regarding the wick, i checked with other souce and this is what it shows. see pics. You must be doing
something wrong or there is a disconnect. figure it out."** — his tool's row for 2026-09-08: HOD 8:33 · Took 3m · BOP
3m · Wick 6m · W.End 8:36 · Wick% 6 · MUD 6h24 · LOD 3:00pm · HL Rng 45.5 · MD $2,138. Asked which open the row should
stand on — the 08:30 minute's (7711.50) or his tool's (the bar ending 08:30, open 7715.00) — **"tools"**. And on the
corpus, 2026-09-08: **"are you also saving the daily stats from yahoo … When will learning occur."** → **"yes"** to
wiring the append into the nightly.

Decided: (1) **The session is read on his tool's grid** — 3-minute bars stamped by their END; the session = the bars
ending 08:30 … 15:00 (minutes 08:27–14:59); **the OPEN = the bar ending 08:30's open** (the 08:27 minute); the
extremes' clocks are bar ends; took = the extreme's bar end − 08:30; **W.End = the first bar AFTER the extreme's bar
to close through the open**; wick% = |open − first extreme| / range; MU / MD = |second extreme − open| × $50. The
panel (`hlToolBars` before `hodLod` / `gdActual`) and the study (`study-hodlod.py`) fold the same way — one
definition, stated in the A row's hover and in `BASERATES.corpus.definition`. What would change it: his word; the
minute grid is one line (`HL_TOOL_BAR`) away, and every E field would move back with it (took 34.7 → 33.5 m).
(2) **The 14% was not a bug in the arithmetic** — it was |7711.50 − 7717.75| / 45.5 on the minute grid, and 6% is
|7715.00 − 7717.75| / 45.5 on his; two definitions under one label. A disagreement between the A row and his tool is
a DEFINITION question first, and the hover now carries the definition so it is read against, not re-derived.
(3) **The corpus appends itself every night** — `run.py` → `append-futures` (the last four day files; idempotent; the
newest day completes from the next day's window) → `study-hodlod` → `BASERATES` per market; **pooled with
provenance**: the vendor's 284 sessions (one contract, through 08-21) and the Yahoo days (the continuous front month,
from 08-24) with each session's source recorded, a day both hold coming from the vendor; the overlap is measured and
reported every night (none yet) — pooled, never averaged across. NQ stands on Yahoo alone until the vendor file
(tab / ISO) has a parser. (4) **The panel's boot literal is baked from the file at build time**, never by hand
(`tools/bake-hodlod.py`); the corpus moving without a build is the design, and the tests pin the file's fields, not
literals. (5) **Yahoo's live-quote row is not a bar** — the in-progress minute at the quote's own second (14:49:41,
o=h=l=c, volume 0) is dropped by the harvest; every session had ended on one. (6) **The nightly re-runs itself after an
install** — `tick.py` runs when an output is older than the log (the installer's mtime 0) or `results.asOf` is dated
before the log's day; on 2026-09-08 the 15:21 / 17:11 / 18:01 installers put the 09-07 outputs back over his machine's
15:05 run, on his disk and on GitHub, and I had said the opposite ("rode up with your push"). The origin guard cannot
see an install that comes after the build; the tick can. This build ships the 09-07 outputs as GitHub holds them and
his machine re-runs 09-08 within ten minutes of the install.

## 2026-09-09 · the NQ file's QQQ lines stay G2–G5; the ES file carries no QQQ line (agenda item 4, no build)

Operator: **"there should be no qqq lines in es. for NQ, just use G because we dont have to distinguish it like for the es
which has both spy and spx."**

Decided: the label names the book only where two books share a file — ES has SPX and SPY, so XG / SG; NQ has QQQ alone,
so **G2–G5** (the QQQ King keeps its name). The ES file carries **no QQQ line** — as it has since v15.80 (`xqWhy`,
pinned by test_v1580 §6 and test_v1585 §3). QG was proposed for consistency and declined. Closes I0908nq; R-26's one open
word is answered. What would change it: a second book on the NQ symbol.

## 2026-09-09 · v15.88 — the agenda batched: the standard hours, "match skylit", companion v1.19, the NQ corpus, the hour on every deflection, ⓪a per market, no patch tier, the tap record next; F-23

His method for the session: **"1 issue at a time"** — and, once item 5 was agreed, **"lets discuss other things also to put
more issues in the build"**; each item was still one message, one decision. His words, in order:

- **"just use whatever is standard"** (the Asia / London hours) → **Asia 17:00–02:00 CT, London 02:00–08:30 CT**, fixed
  Chicago hours as the session boxes on his other tools; the DST-mismatch weeks are not special-cased (nor there). A
  session counts from 150 bars (a late-starting holiday evening is a session; a stub is not). What would change it: his
  cuts. v15.57's LDNH / LDNL were the London range under my name — renamed to his, not duplicated.
- **"match skylit"** (the King latch) → the two-minute hold is OFF; the rail and the export follow Skylit's King on the same
  tick. DECISIONS 2026-09-07's "one designed difference" is closed. The hold is one number away (`KING_LATCH_MS`) and the
  flap information is still computed, if a day ever shows the flap was worth hiding.
- **"yes"** (companion v1.19) → NQ's full night; the weekly 5-minute bars for the prior ISO week (WH / WL / WPOC — named on
  his list since v15.80, never drawn because 1-minute history is 7 days); NQ's base rates couriered. A second Tampermonkey
  link at install time, once.
- **"i'll go with your recommendation"** (the NQ vendor corpus) → parsed; NQ BASERATES = the vendor's 188 + the Yahoo days,
  provenance per session; the four shared days read side by side (identical clocks and extremes — the same contract).
- **"i agree i think you should also tag other hours"** (the first-hour turns he had not circled) → they are deflections,
  tagged by the hour; EVERY leg carries its hour (H1–H7; first · midday · last as the roll-up); R-3 is implemented — the
  clock is a CLASS in the pattern table, not a rule (L7 rewritten).
- **"your recommendation"** (the tap record) → the NEXT build, on its own (v15.89): the recorder deserves its own
  verification day, and this build already carried six items.
- **"your recommendation"** (the patch-build tier) → none. The time is the suite, the records, the installer and the CDN
  wait — his standing rules; a patch tier skips exactly the checks that caught the 14% wick and the mixed-window feed.
  Batch instead. Closed.
- **"this build"** (⓪a per market), after my correction: I had said the NQ corpus would move "the NQ E row" — there was no
  NQ E row; the section read ES on every chart. Now the NQ chart measures NQ's bars, NQ's base, NQ's dollars, and says so;
  with **no baked NQ literal** — until the courier has delivered, the header says "no base for this market yet" and the E
  row does not draw. Absence of data must never render as data (D-6).

**F-23, found on the way (no decision of his; recorded so it is never re-derived):** the corpus's ONH / ONL included the
session's own post-close bars — a look-ahead that removed failed sweeps. Corrected: ONL 22% n=125 · ONH 16% n=154 (were
29% / 26%) — within chance like every level name; the first-30-minutes edge in F-14 is gone (21% n=249 vs 18%), the flush
(35% n=94 vs 23%) and the slow reclaim (32% n=96 vs 23%) stand at about +10pp. The number to quote is the file's, with its n.
