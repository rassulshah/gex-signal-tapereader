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

## D-7 · `sampleTapeHistory` RECORDS A REPLAY AS IF IT WERE TODAY — SPX FIXED, **SPY OPEN**
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

