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

## D-3 · THE PILES COME FROM INSIDERFINANCE, NOT SKYLIT — AND MY REASONS WERE HALF WRONG
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

**The piles stay on IF for now** because that path is verified end to end — right window, net-vs-gross,
96.3% coverage — not because Skylit was proven unusable. **Skylit's SPY `dte0` set is a legitimate
alternative**: 197 strikes, exps `["2026-08-21"]`, decomposable into call/put/net, and it is the same book
the regime chip reads.

⚠ **Skylit's SPXW ladder has NO call/put legs** (`callPut('SPXW')` returns empty), so the SPXW route
cannot do the gross-vs-net split that removed an 84× overstatement at v11.68. Only the SPY route can.

**Would change it:** evidence that the two books disagree on polarity *within the same window*. See D-4.

---

## D-4 · ONE CHIP FROM ONE BOOK, EVERYTHING ELSE FROM THE OTHER — UNRESOLVED
**Standing since v11.64.**

    regime chip (−G −V) + BREAKS ...... SKYLIT
    everything else on ① FRAME ........ INSIDERFINANCE

v11.61 deliberately sourced the piles from Skylit *"so a pile can never contradict the regime chip"*. v11.64
moved them and **that guarantee was silently dropped** — the comment claiming it survived until v11.76.

They agree today (both negative gamma) **by coincidence, not by construction**. On a day where Skylit's SPY
tape reads +gamma while IF's 0DTE SPX book reads −gamma, the chip would read **FADES** above a rail of
purple accelerators and nothing would flag it.

**Three ways out, in order of preference:**
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

## D-5 · `dte0` MEANS "NEAREST LIVE EXPIRY", NOT "TODAY"
**Discovered 2026-08-23.** IF drops an expiry from the payload once it has expired. Captured after the
Friday close, the chain's earliest expiry is Monday, so `dte0` selects **20260824** while `today` is
20260821 — `dte0_isToday = false`.

During live RTH on a weekday the row exists and `dte0` really is today. After the close and at weekends it
is the next session's book, and the band's "today's expected move" is then **Monday's** straddle laid over
Friday's price action.

The arithmetic is right; the wording over-promises. **Open** — not yet reflected on the face.

---

## D-6 · ABSENCE OF DATA CURRENTLY READS AS ABSENCE OF OBSTACLES — OPEN DEFECT
**Found in the hardening audit, 2026-08-23. NOT YET FIXED.**

`emPiles` returns `[]` on **six** distinct failures — no chain, chain error, `spot<=0`, no `gexProf`, no
ladder/`dispScale`, `maxMag<=0` — and `[]` renders as:

> *"Nothing sizeable between 7708 and 7730."*

A **false all-clear**. Nothing on the face or in `__gptsDebug.piles()` (which returns `{ok:true, n:0}`
either way) distinguishes "the path is clear" from "we cannot see the book".

Same disease as a render `try/catch` swallowing an error, which this project already has a rule about.
**Fix: a distinct "no gamma book" state that says so.**

Two smaller ones found in the same pass, also open: a non-finite `now` renders *"between NaN and 7730"*,
and two piles on one strike produce *"accelerator at 7718 … to the node at 7718"*.

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
