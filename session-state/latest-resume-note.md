# RESUME NOTE — read this before anything else
_written 2026-09-01 · panel v15.21 · companion v1.17 · supersedes every earlier resume note_

---

## ⚠ 0 · THE CARRY-FORWARD CONTRACT

**This note is rewritten IN FULL every build. Anything not re-typed is GONE, silently.**

⚠⚠ **AND IT FAILED EXACTLY THAT WAY AGAIN.** The previous note carried `v15.09` in its header —
which is all `test_savedone` checks — while its body still described **v14.80**. Nine versions never
reached it and every guard stayed green. Snapshot kept as `session-state/2026-08-31_resume-v14.80.md`
so the failure is legible. **A version-keyed guard is satisfied by a stamp, not by content: when you
bump the header, rewrite the body in the same edit.**

Read in this order, in full, before anything else:

0. ⚠⚠ **`session-state/LESSONS.md`** — the failure-pattern register, then the per-build log newest
   first. It names results that have been **WITHDRAWN**; quoting one is the most expensive thing a
   fresh context can do, because it looks like knowledge.
1. **`session-state/CHAT-HISTORY.md`**, the CURRENT-CONTEXT entry — what was *said*.
2. **`session-state/LOCKED-ITEMS.md`** — agreed-but-unbuilt work. Check every build.
3. **`session-state/OPEN-QUESTIONS.md`** — so you do not re-ask what he already answered.
4. **`design/DATA-ARCHITECTURE.md`** — who can reach what.
5. **`skylit-docs/FINDINGS.md`** — F-1…F-16.

**THE LOAD CLONES FULL** (never `--depth 1`).
⚠⚠ **A COMMIT IS NOT A PUSH.** The cloud gets a 403 from the git proxy. `installvNNNN.bat`, run on
his machine, is the only route to GitHub — and the git record confirms every release on origin was
committed and pushed by **him**, never by a session.

He works **one item at a time** and expects you to **discuss before building**. He has caught more
real defects than the test suite has. **When he pushes back, he is usually right.**

---

## 1 · THE STANDING BUSINESS REQUIREMENT (his words)

> "I am a trader and need to know where to take trades from and where price is going, so basically I
> need to know potential support and resistance especially if it is weakening and new support and
> resistance is forming as well as where price is going."

And the frame for ⓪a, which he had to tell me and which reorganised the whole section (2026-08-30):

> "do you realize that i am taking the model of the daily bar and trying to measure the movements in
> it from open to close"

> "so the daily candle and its contstruction will be my mental model for daytrading using all of
> these measurements … its very important that this feature be a world class feature but it will
> require your help in constant refinement via the use of llm to identify additional datapoints and
> measurements to better prediction"

**What he trades, stated 2026-08-30 and now wired that way:**

    STRUCTURE    nodes · kings · walls · flip      SPXW / SPY / QQQ.  ES has no book of its own.
    MEASUREMENT  HOD · LOD · candle · EFF · GD/RD  ES's own 1-minute bars.  (`measureBars`)

> "its the es that i am trading but using spxw nodes" · "we are using other markets to get things
> like their kings because ES doesn't have its own book, so we use other tapes"

---

## 2 · WHERE WE ARE — v15.21, and what the face carries

**Panel v15.21 · companion v1.17.** Suite **133 green / 6 baseline red** (`expiry_profile`,
`node_map`, `sma_cont`, `tapeking` (needs jsdom), `trendbadge`, `v1126_process`).

### ⚠⚠ THE REPLAY SLIDER IS THE NEW THING, AND IT HAS NOT BEEN SEEN LIVE YET

A 30px strip under the tabs: day stepper, a track whose ticks are the frames that actually exist,
the clock, and a LIVE / ⟲ REPLAY badge. **Dragging it rewinds the whole panel** — ladder, kings,
nodes, the frame and ⓪a all read the book recorded at that minute. ◀ ▶ step the day, so a Saturday
reaches Friday.

**It is the existing stale-book path with a different source, not a second renderer.** `tapeMap(sym)`
has served a stored book since v14.55; replay points that branch at any frame in
`gpts_repo_v1.snaps` — **2,149 SPY frames over 18 days**, back to 2026-08-11, which is NOT bounded by
the 3,600 KB localStorage budget.

    tapeMap        replay -> the frame's book · stale -> the latch · else -> live
    measureBars    replay -> bars built FROM THE FRAMES, truncated at the parked one
    recorderBlind  replay -> TRUE.  Nine write paths inherit it. This is the D-10 guard.

**Four rules it is built on, and none may be quietly relaxed:**
1. a vendor row belongs to the book whose KING it sits nearest **in log space** — SPY 767 and QQQ 716
   are seven percent apart and no magnitude rule splits them;
2. **refuse, never fall through** — an empty book beats live numbers under a REPLAY badge;
3. **replay never writes**;
4. the handle **snaps to a frame that exists** — 13:01 gives you the 13:00 book, labelled 13:00.

⚠ **THE PER-BAR OPEN IS RECONSTRUCTED** (previous close); high/low/close are recorded and exact.
`approxOpen` carries it, so WICK%, BODY and the GREEN/RED call inherit the approximation knowingly.
⚠ **A replayed ladder is as deep as the frame was stored.** `VEND_MAX_ROWS` went 40 → 90 for that
reason, but it **cannot enrich the 18 days already recorded** — at 40 rows those hold ~19 SPXW
strikes, down to 4% of King, which covers everything the ladder draws and not the grey minors.

**v15.11 fixed the two things his first drag exposed.** All three crowns now come from the frame
(`ladderKings` was reading TODAY's latch and TODAY's `LASTFEED` — mislabelling, not thinness), and
the whole accumulation layer replays through `slicesFor()`, so BUILDING/STEADY/FADING, the day peak
and the DEFENDING/ABANDONING marks come from the REAL rule fed a sequence rebuilt from the frames.

⚠ **STILL CANNOT REPLAY, and both are stated on purpose:** the **roll arrows** (`ROLL_LATCH` is a
stateful RTH accumulator, not in a frame — replaying it is its own build), and the **gamma profile**,
which is not on the live face either (removed v14.81 at his request; do not reinstate it under cover
of "make replay like live").

⚠⚠ **v15.12 — THE EM PIN IS PER CHART FAMILY, AND THAT IS WHY ES WORKS AGAIN.** His pin was captured
on the SPY chart (`rr:1`, `em:3.49` in SPY points) and on an ES chart was judged against an ES-scale
floor of 7.7, healed away as implausible, and fell back to an expired $1.70 straddle — so the band
refused and **the ladder, which lives inside that section, went with it.** The record is now keyed
`sym|cash` / `sym|fut`, and a family with no pin SEEDS from the other using `emK`, the straddle in
the BOOK's own points. ⚠ Pins written before v15.12 have no `emK` and use the ratio rescale, which
is flagged `seedApprox`. **A stored value in DISPLAY units is a trap whenever the display can change.**

⚠⚠ **v15.13 — THE LADDER WAS 105px WIDER THAN HIS PANEL AND HE HAD NEVER SEEN THE RIGHT EDGE.**
`.g3ladwrap` measured 640 against 535 with scrollLeft 0, and the roll lane is at x 620-640 — so the
arrows shipped at v15.09 into the one strip he could not see, with the ROC column and most node bars
beside them. `ladderFit()` now grows the panel by exactly the overflow. ⚠ **The width was logged as
"his call" since v14.54; that applies to WHICH COLUMNS MATTER, never to whether the panel can show
the columns that exist.** The arrows also REPLAY now, by re-running the live `rollScan` over the
frames, and `rollsLive()`/`velOk()` no longer exclude a replayed bar.

⚠⚠ **v15.14 — THE KING LANE NOW DRAWS THE CROWN'S JOURNEY.** Its renderer was always complete; it
had nothing to draw. The track recorded MIGRATIONS but never an ORIGIN, so a crown holding one strike
all day gave an empty array and the "no migration recorded" placeholder — **an empty series and a
series with one long run mean opposite things.** The first observation is seeded now, and in replay
the journey is rebuilt from `tri.<book>.king` under the same `KT_DWELL` rule. Measured 2026-08-31:
**8 SPXW migrations, 10 SPY.** ⚠ That is the RECORDED trinity crown at dwell 2 — a DIFFERENT
instrument from the latched crown measured at "SPXW 0 durable moves" on 08-28 over a truncated
window. Neither number refutes the other; say which one you mean.

⚠⚠ **v15.15 — THE SEAM LESSON, AND IT IS THE MOST IMPORTANT THING IN THIS NOTE.** Every defect from
v15.10 to v15.15 was the same shape: a consumer reaching around a replayed seam to a LIVE source.
`tapeMap`, `ladderKings`, `slicesFor`, `velOk`, `rollsLive`, `ktOf`, `closedCandles`, `tapeSync`.
The last one cost four symptoms at once — `emBand` read LIVE candles while the nodes came from the
frame, and `emPiles` clips piles to that band, so a 13:12 book against a 21:00 band left ONE pile:
one node bar, no states, nothing for `rollScan`. **"I swapped the source" is not a finished thought
until every reader of that source is enumerated.** Before claiming replay works: grep the render path
for `LASTFEED`, `STATE[`, `VEL`, `ctTodayStr` and the latch keys.
⚠ Also: a missing `k` on the replayed velocity rows made `rollScan` compare `undefined===undefined`
and discard EVERY roll as "the same strike" — 2,406 sightings, zero drawn, nothing thrown.

⚠ **STILL LIVE-SOURCED IN REPLAY: the ladder's LEVELS.** `ifLadder` reads `ifChain` (the live IF
payload); the NODES replay, the LEVELS do not. Frames store `lev` and `deriv`, so it is buildable.

⚠⚠ **v15.16 — THE SINGLE MOST IMPORTANT THING TO UNDERSTAND ABOUT REPLAY.** `SK_MIN_STRIKES = 20`
is a LIVE-PARSE health floor ("below 20 the DOM changed"). His recorded frames hold a MEDIAN OF 17
SPXW strikes, so it refused **120 of 129 bars**, and a `skPiles` refusal returns NO PILES — no nodes,
no statuses, nothing for `rollScan`, one strike at 100%. Every symptom he reported, from one constant.
`SK_MIN_STRIKES_REPLAY = 5` now judges a recorded book; the live floor stays 20.

⚠⚠ **AND THE PATTERN BEHIND EVERY REPLAY BUG IN v15.10-v15.16:** `rollsLive` (RTH-only), `velOk`
(live harvest), `tapeSync` (live votes), `closedCandles` (live candles), `SK_MIN_STRIKES` (live-parse
health). **Every threshold carries an implicit claim about where its input came from, and replay
changed the provenance without changing the thresholds.** Before claiming any replayed surface works:
list every early return, floor and freshness check between the source and the pixels, and ask of each
what it assumes. Skipping that read cost six builds.

⚠⚠ **STILL LIVE IN A REPLAYED FACE — AND MEASURED, SO NOBODY PROMISES WHAT CANNOT BE DELIVERED.**
Traced 2026-09-01 against a real frame; **neither is fully recoverable from the days already stored:**

    dispScale   ⚠ WITHDRAWN AT v15.19 — THIS WAS WRONG. The basis IS in every frame: px / xm.SPXW.px
                (764.86 / 7677.55 = 0.099775). `ifLadder` reads it from the frame in replay now. The
                stated evidence ("a frame has no ES price") was true and did not bear on the claim.
    the LEVELS  the frame's `lev` holds cr/cr0/ps/ps0/mag at SPY SCALE (767/765) — the SPY book's
                walls, NOT the SPX chain rows the ladder draws (PDH, CW0, FLIP…). Those come from
                ifLadder.rows + sessionLevels and are not in a frame at all.

**So a replayed LEVEL set is not obtainable from the 18 recorded days.** Recording `ifLadder.rows`
per frame would fix it going forward. ⚠ Do not tell him the levels will replay on old days.

⚠⚠ **v15.17 — TWO MORE, AND BOTH WERE FOUND BY MEASURING RATHER THAN GUESSING.**
**The arrows were TRUE and wrong to show:** four genuine roll pairs between strikes the ladder does
not draw (7625->7650 on an $82K shed) while the KING's own 7675->7670 at $22.4M was missing. The live
latch scans `tradeNodes()`; replay scanned every stored strike. **Reusing a function is not
reproducing the call — match the INPUT UNIVERSE too.**
**"Cannot scroll" was not a scroll bug:** panel 1016px in a 557px window, top -307, and
`body.scrollHeight === clientHeight`. The content fits the panel; the panel does not fit the screen.
`panelFit()` clamps it. Third costume of the v12.2/v12.5 lesson.

⚠⚠ **v15.21 — `hlPT` WAS SUBTRACTING A SPY PRICE FROM AN ES PRICE, LIVE.** The face printed
**PT 6895.0pts** and **OF BAR 35818%** (and `LC RNG 6894.2`): `hodLod` has measured true ES bars
since v15.08 while `hlPT`, which consumes hodLod's own output, still read `closedCandles()` — the
SPY book. 7661 − 766 = 6895. Both now read `measureBars(sym).bars`.
⚠⚠ **AND THE TEST DEFENDED IT.** `test_hodlod` p7/p7b asserted in a comment that "closedCandles() is
the UNDERLYING book" — true before v15.08, false after — and passed throughout. **A source grep
freezes the model that was true when it was written; when a value's SOURCE changes, the tests that
describe that source are part of the change.**

⚠⚠ **THE LADDER'S COLUMN HEADERS WERE NEVER LOST — NEVER CARRIED OVER.** v14.46 replaced the rail +
node profile with the ladder and the profile's header row (`g3ndhd`) went with the old surface. Ten
columns unlabelled for seven builds, because **a missing label throws nothing and greps as nothing.**
Now: S · Y · LEVEL · PRICE · NODE %KING · NOW · MARK · TAPS · Δ15m · STATE · ROC. ⚠ Every x and width
is the column's own `LAD_*` constant, and the test asserts THAT, not just the words.

✅ **THE ⓪a CANDLE, ANSWERED:** it is **RTH-only** (`hodLod` skips `so < 08:30 CT`, clamps to 15:00),
and on a FUTURES chart it **backfills the whole session** — the courier asks `interval=1m&range=5d`,
so opening at noon still fills from 08:30. ⚠ It was up to an HOUR stale: `FUT_POLL_MS` was hourly,
chosen when these bars only fed a nightly corpus. **Companion v1.17 polls every 5 minutes inside RTH,
hourly outside.** ⚠ **The reason a constant was chosen can expire without the constant looking wrong.**
⚠ On a CASH chart there is NO backfill — `measureBars` falls back to `closedCandles()`, which holds
only what the panel has seen since it opened. Stated, not fixed; the courier carries no cash bars.

⚠⚠ **v15.20 — THE READ IS OFF (`CFG.read=false`) AND THE FIRST REMOVAL TOOK THE WRONG LINE.**
He asked at v15.10 ("take out the read … where it say Range day - Trinity"); something else went and
`.g3tread` — *"EVENT day · Trinity 2-of-3 … KING 7664 (brake) holds"* — stayed, so he had to say it
again. It is a SETTING, not a deletion ("I might come back to it later"): every producer still runs
and the check is `===true`, so a config stored before the key existed leaves it OFF.
⚠ **When the instruction names what is ON SCREEN, confirm against the screen** — `render-face.js`
exists now, so there is no excuse for removing the wrong thing twice.

⚠⚠ **AND `__gptsDebug.audit()` WAS INVENTING A FAULT AND THEN CRASHING.** It read `body.innerText`,
a rendering-dependent property that a layout-free DOM returns `undefined` for, tested the string
"undefined" against itself, reported *"the face prints undefined somewhere"*, and threw on `.split`.
`itxt()` falls back to `textContent`. **The auditor was the broken thing, and only became visible
once the face could be rendered in a test.**

✅ **LIVE CHECK, v15.19 on his panel, 2026-09-01 08:58 CT, market open:** audit ok / 0 violations ·
0 render errors · velocity harvest ok, 1212 objects of 2678 scanned · 17 ladder rows · storage
2,753KB of 10,240 (27%), 0 shed, 0 quota hits · panel 651px in an 837px window, body scrollHeight
962 / client 621 — **it scrolls.** The replay strip and tab bar are present and the day selector
reads `Tue 1 Sep`.
✅ **The v15.18 arrow fix is doing live work:** on his own book that morning, 7615 (`cur −4,939,537`,
`d15 +77,796`) and 7655 (`cur −3,927,477`, `d15 +249,912`) both read as RECEIVING by sign while
their mass was FALLING. Exactly the inversion, live.

⚠⚠⚠ **v15.19 — READ THIS BEFORE TOUCHING ANY REPLAYED SURFACE. THERE IS NOW A TEST THAT DRAWS.**
`node tools/render-face.js 2026-08-31 14:12` renders the REAL userscript in jsdom, parked on a real
recorded minute, and prints what the body contains plus every swallowed error. `test_replay_face.js`
is the same harness with 36 assertions. **Run it before claiming any replayed surface works.** Nine
defects in a row were found by the operator instead of by me for one reason: every other test in this
project executes a FUNCTION, a refusing section is swallowed by design, and so a broken replay and a
quiet one are the same picture.

⚠⚠ **v15.19a — ONE REFUSAL UPSTREAM WAS HIDING SIX SURFACES.** `emBand` pins the day's expected move
once from the LIVE 0DTE straddle, keyed to the session shown; a replayed day has no such record, so it
fell through to today's chain, which after hours does not quote, and returned `no EM`. **The ladder,
the node states, the percentages, the king lanes, the roll arrows and the ROC column all live inside
that section.** The band IS recorded — `feat.emband` = `{ok, em, open, k, est}` — and replay now pins
from it. ⚠ When several unrelated surfaces vanish at once, **walk UP to the nearest thing they share.**

⚠⚠ **v15.19b — THE CLOCK IS A LEAK CLASS WITH NO SEAM TO CATCH IT.** `Date.now()` is not a feed, so
no freshness gate or provenance check sees it. `sessionPhase()` said **AFTER HOURS · EM EXPIRED** on a
14:12 Monday bar — and that branch RETIRES the target, the budget and **the roll arrows** — and the
king lane's axis ran from the replayed open to tonight, crushing a session's journey into 6.5px
(measured: run starts 4.0→10.5 instead of 4.0→21.3). Both now use **`clockNow()`**. ⚠ A wrong
position is not a missing element, and a test that COUNTS elements cannot tell the two apart.

⚠⚠ **v15.19c — CORRECTION TO THIS NOTE: `dispScale` IS RECOVERABLE. The earlier claim was wrong.**
A frame carries `px` (764.86) and `xm.SPXW.px` (7677.55); their ratio IS the basis (0.099775), stored
every minute since the recorder began. My evidence — "a frame has no ES price" — was true and
irrelevant to the question asked. ⚠ **A negative finding needs the same standard of proof as a
positive one**; this one sat in the file the next context is required to trust.
⚠ **THE LEVELS REMAIN ABSENT AND THAT IS DELIBERATE.** `ifLadder` in replay returns the frame's scale
and NO rows: today's PDH/CW0/FLIP drawn over a past session is the mislabelling this project keeps
paying for. Recording `ifLadder.rows` per frame fixes it going forward only.

⚠ **THE ROC COLUMN DRAWS IN REPLAY** as `rp*` — this panel's own change in MASS, italicised, with a
hover that says so. `p15` still means SKYLIT's `percent15Min` and nothing else may ever be written to
that name.

⚠⚠ **"I CANNOT SCROLL" IS SETTLED IN CSS, NOT IN JS.** Reported at v12.2, v12.5 and v15.17; every time
the body scrolled correctly and the PANEL was taller than the window. The panel now carries
`max-height: calc(100vh - 16px)` in its own style, which holds with no code running. `panelFit()`
stays only for what CSS cannot do: pulling a panel dragged off the top edge back into view.

✅ **AND WHAT HE ASKED FIRST, ANSWERED BY MEASUREMENT:** HOD/LOD, the candle and the DAY columns DO
replay — 09:00 / 11:30 / 14:12 give LOD 08:48 / 09:30 / 09:30, HL RNG 4.4 / 5.1 / 5.1, EFF — / 34% /
23%, KING 763.28 / 768.10 / 765.77.

⚠⚠ **v15.18 — A THRESHOLD'S DENOMINATOR HAS A PROVENANCE TOO, AND THAT IS THE TENTH INSTANCE.**
Four of five levels read **SPENT** on a replayed bar. `levelStateOf` divides the frame's mass by
`peakOf(k)` — and `peakOf` returned the LIVE WHOLE-DAY peak, so 14:12 was being judged against a
high water mark set at 15:59. Measured on 2026-08-31 at 14:12:

    strike      |cur|    peak->14:12   ret     peak WHOLE DAY   ret
    7675     81988795      81988795   1.00          115827347   0.71
    7685     34840580      34840580   1.00          503965848   0.07   <- at its OWN peak, called SPENT

`replayPeakOf(k)` is the max `|cur|` across frames **up to `REPLAY.idx`**, memoised on `day|idx`; a
strike no earlier frame carried returns `null`, not a fabricated peak. ⚠ The seam covers not only
the DATA a computation reads but **the scale it is judged against** — walk the constants and the
accumulators, not just the feed.

⚠⚠ **v15.18b — THE ROLL ARROWS WERE POINTING BACKWARDS ON HALF THE BOOK, LIVE AND IN REPLAY.**
`rollScan` tested the SIGNED delta (`src.d15 < -$40K`), which is right on the positive side and
inverted on the negative one, where a strike GAINING mass carries a negative delta:

    7675  |cur| 59.6M -> 82.0M   d15 -22.4M   GAINED   <- was called the SOURCE
    7670  |cur| 40.2M -> 18.2M   d15 +22.0M   SHED     <- was called the RECEIVER

The face drew `7675 -> 7670`; the mass went `7670 -> 7675`, into the King. Both tests now measure
`|cur| - |cur - d15|`. Over 129 recorded frames: **446 old arrows, 310 new, 40 of them previously
drawn REVERSED**, 15 identical. ⚠ The v11.34 note "receivers gained 2.8x, 8.6x…" was measured under
the inverted rule — **do not quote it again as evidence about direction.**
⚠⚠ **AND THE STATES COULD ONLY EVER SAY TWO THINGS.** Every branch but SPENT is gated on `p5`/`p15`
— SKYLIT's percents — which a recorded row does not carry. So replay could reach only SPENT,
WEAKENING-via-roll-source and HOLDING: *"2 weakening and everything else spent"* was the complete
list of things it could say, not a reading of the market. Replayed rows now carry `rp5/rp15/rp60`,
**deliberately not `p5/p15/p60`** — the ROC column credits those to Skylit and their sign convention
is not observable from a recording. `rp15` is ours and its convention is stated: the change in MASS.

⚠⚠ **THE ONE RULE BEHIND ALL THREE OF THIS BUILD'S DEFECTS: `cur` IS SIGNED, `|cur|` IS MASS.**
SPENT's denominator, `rp15`, and the arrows were the same confusion in three places. When a quantity
is signed, decide once whether the code means the value or its magnitude — and say which in the name.

⚠ **`tools/audit-replay-face.js` (new)** prints, for any minute of any recorded day, each node's
mass, its peak-to-then, its state and the arrows, so a claim about the face can be CHECKED. Run it
before answering any "does this make sense" question:
`node tools/audit-replay-face.js 2026-08-31 14:12` ⚠ **its clock is CENTRAL, because the panel's is**
(`ctNow` uses America/Chicago; `replaySec` subtracts 5h) — an audit on ET silently compares 15:12's
book against 14:12's claim, which cost half an hour this build.

⚠⚠ **AND THE RULE THAT COMES OUT OF IT: AN ABSENT MEASUREMENT IS NOT A ZERO.** `nodeTapCount()`
returns 0 both for "never touched" — which the face reports as *a quiet death* — and for "this run
does not track taps". Replay scored every strike a confident zero and every one earned the clause.
Taps are now `null` with `tapsKnown=false`, DECAYING requires a zero that is actually KNOWN, and the
face says the count is not recorded per frame. **Whenever a default is the same token as a real
measurement, the code has no way to be honest.**
⚠ The assertion for this was written as a SOURCE GREP and **survived mutation** — `if(false){
tapsN=null; … }` leaves the text intact. Fourth recorded instance in this project. It now executes
`levelStateOf` in both modes.

✅ **AND WHAT THE CHECK CONFIRMED AS CORRECT** — record confirmations, not only faults: the node
profile is exact at 14:12 strike for strike, and "only five nodes" is every SPXW strike at or above
the 20% threshold, the same filter the live ladder applies.

**FIRST THING TO DO: drag the slider on the live panel.** v15.19 was seen working LIVE (above); the
REPLAY side of v15.17-v15.19 has still never been dragged on his real panel. Check the ladder, kings and ⓪a move together, the
clock reads the parked bar, ◀ reaches Friday, and `__gptsDebug.storage()` shows no new writes.

### the rest of the face

**⓪a DAY is three columns beside the candle** — 1ST · 2ND · DAY, each `label / actual / expected`,
1px rules between. The candle is on the LEFT and its height is **DERIVED**:
`DAYCOL_HD(16) + DAYCOL_N(9) × DAYCOL_ROW(13)`.

    1ST   SLvl · HodN|LodN · TIME · TOOK · BOP · WICK · W.END · OF BAR · MUD
    2ND   TLvl · HodN|LodN · TIME · PT TOOK · PT · PTWICK · (blank) · OF BAR · PTMUD
    DAY   GD/RD · PTN · HL GAP · HL RNG · HL $ · LC GAP · LC RNG · EFF · BODY

⚠ The blank at index 6 of the 2ND column is HIS deliberate gap. Do not fill it.

**The version now sits in the header beside `Tapereader`**, reading `GPTS_VERSION`.
**The READ row is OFF the face** (v15.10, his call — "I might come back to it later").
⚠ `emRead()` is **still called**: `test_em_band` §30 executes it and greps for forecast vocabulary,
so deleting the call would leave that ban guarding nothing.
**TREND is off the face since v14.90**; `secBias()` is kept because `bias.confirm` still records.
**The GAMMA PROFILE was removed at his request (v14.81) — do not propose rebuilding it.**
**The panel is PINNED to the SPX book** (`CFG.mkt`), and says so when the pin disagrees with the
chart: `◉ SPY book (chart: QQQ)`.

## 3 · THE NUMBERS ON THE FACE — every one with its n and its date

⚠ **Never quote a rate from this project without its n and its date.** Several are one-day samples
and at least one was contradicted by a later day.

| what | value | n · window | status |
|---|---|---|---|
| ⓪a **cell** rates (`HLTAB`) | AUC 0.879, calibrated at every decile | 44,302 obs · 284 sessions · 2025-06-02→2026-08-21 | PROVISIONAL, F-4/F-11 |
| ⓪a **IN** decision (`inHit`) | **63%**, median 9:20 | n=284 | CONFIRMED, F-12 |
| ⓪a **NOT-IN** decision | **85%**, median 8:40 | n=230 | **the STRONGER of the two**, F-11/F-12 |
| far side · touch | AUC 0.826, Brier 0.147 | 388,494 obs · 197 sessions | PROVISIONAL, F-14 |
| far side · timing | first passage, AUC 0.692; `T ~ (d/σ)²` does 95% of it | 7,168 arrivals | PROVISIONAL, F-15 |
| **GREEN/RED** (`GD_META`) | **76%** on the **80%** of days it speaks, base 51%, z=8.8, CI 71–82 | n=282 · 2025-06-02→2026-08-21 | PROVISIONAL, no forward test |
| EFF expected (`EFF_META`) | **68%**, a **MEDIAN** | 284 sessions | a ratio takes a median |
| PT / LC (`PT_META`) | PT ~49m / 19.8pts · LC ~103m / 12.5pts | n=283 | **side-specific**: PT after a LOD 24.0pts, after a HOD 17.0 |
| deflections (`DEFL_META`) | 79 deflect / 25 breaks, **56% break** | 8 sessions · calibrated 2026-08-29 | detection only — see §5 |

⚠⚠ **THREE WITHDRAWN NUMBERS. NEVER QUOTE THEM:** the ⓪a IN call at **92%** (hindsight side
selection — `inHindsight:92` is kept only as a label); SUCCESSION **76% crowned within 20 bars** (does
not reproduce at any horizon; 23% at 30m against the DRAWN crown); "crowns beat chance by **17pp**"
(`study-kingdeflect.py` measured crowns where it should have measured nodes).

⚠ **DO NOT RE-PROPOSE**, all measured and recorded: sweeps (48%, below their own base), momentum
divergence (−0.0004), NQ divergence (−0.0014), IB30/IB60, the 50-SMA, open-reclaimed, the 60-minute
breakout, the daily ATR, the overnight range, volume, day-of-week, gap, prior-day and overnight level
identity, prior-day POC/VAH/VAL (gx-009, CLOSED NEGATIVE — the **sham beat the real level** twice),
the prior day's colour (AUC **0.500**), red/green from the open, a 5-feature logistic, or a narrow
high-probability timing box.

---

## 4 · WHAT IS SETTLED — do not re-litigate any of this

- **⓪a is one daily candle.** Rows are legs of it; `OF BAR` sums to 100% and that total is the check
  the decomposition is honest. **WICK% is not a ratio — it is where the OPEN sits in the bar.**
- **PTWICK = PT TOOK + PT BOP** (Q1, answered 2026-08-30): the first-extreme shape anchored on the
  SECOND extreme, as WICK is anchored on the open. **PT BOP and PT W.END were dropped** — em-dash
  most days.
- **ENHANCE HIS LAYOUT, DO NOT REDESIGN IT.** *"i dont want to deviate too much from what i have."*
  A HIGH/LOW rewrite and a per-leg rewrite were both offered and both rejected.
- ⚠⚠ **HE HAS ASKED THREE TIMES FOR A TWO-SIDED TIME WINDOW AT A HIGH NUMBER** and it cannot be built
  honestly: ±15 min lands **15%**, ±30 lands **24%**, and a two-sided window must be **3.6 HOURS**
  wide to reach 80% (197 sessions, F-13/F-15). "after X — 80%" is a **one-sided floor**; the MIDDLE
  HALF is the honest two-sided answer and it is **50%**. Show him those three numbers rather than
  re-deriving the refusal.
- **Deflection geometry is FINAL:** approach **1.0 ATR**, penetration **1.5 ATR**, triggered on the
  **WICK**; the **CLOSE** classifies deflect vs break. **One price event is ONE deflection**, never
  one per node. A **pullback is the extreme of its own 30-minute neighbourhood** — median 3/session.
- **The node universe is a RANK (top few by dollars), not a %King threshold.**
- ⚠⚠ **The node selects WHERE; the price action decides WHAT.** Node 764 on 2026-08-24: one
  deflection, two breakdowns. **Never score nodes as reliable-or-not** — it would look like a finding.
- **Levels are excluded by being mid-range, and IB is excluded BY NAME.** A level price traded through
  is near neither extreme, so one rule does both jobs; a distance test would readmit IB on any day it
  sat on a wick.
- **Ladder:** `LAD_W=640` — raised once from 618 for the roll lane, argued in the open, and the
  assertion now says *"640 IS NOW THE CAP. The next column that wants width argues for it here."*
  The chute is **price's alone** (v14.82 put names in it and he rejected it). Name→price gap is 2px,
  deliberate. `ladderRolls` is retired; its lane is the King columns.
- **Two books, never averaged.** Skylit = FLOW; InsiderFinance = OI×gamma. **Name both units out loud
  before comparing two numbers.**
- **Scale:** SPX and ES are **ten points apart** — a basis, not a conversion. **SPY is 10.04× away and
  every scale failure of 2026-08-30 had SPY in the display path.** `displayScale()` is the single
  source; `hodLod` takes its scale FROM `measureBars`, never from the ratio.

---

## 5 · HOW IT IMPROVES — the part he cares most about

1. **DATA.** Every feature self-declares once in the FEATURES registry and is recorded per bar; the
   `farside` record carries each level's **node identity** (`kind`, %King, polarity, role) — the
   gamma dataset nobody has collected.
2. **ANALYSIS.** `docs/LLM-NIGHTLY-BRIEF.md` carries the ⭐ section: score the touch call by decile,
   score the ≤20% NO call separately, run the gamma test **with a DENSE distance control** (F-16 —
   a sparse control invented a +12-point effect that vanished), and **propose** a new `FARSIDE.json`,
   never apply one.
3. **ADOPTION WITHOUT A BUILD.** Companion v1.16 couriers `BASERATES.json` and `FARSIDE.json` from
   raw GitHub; the panel **validates** them (≥120 sessions, every rated cell n≥60, monotone) and keeps
   the baked-in copy if the payload fails. ⚠ **Monotonicity is not evidence** — two synthetic sessions
   once produced `57/80/100/100/100`.
4. **THE NIGHTLY LOOP** (`tools/nightly/`): **the LLM proposes, the harness disposes.** 8 hypotheses
   are **pre-registered** — locked before the data to test them exists, which is stronger
   pre-registration than anything obtainable later. `subset_null()` is the control that matters: it
   killed four false PROVISIONALs at 82–84% that were mutually contradictory, and it exists because a
   FILTER inherits the edge it filters.
5. **THE UNTESTED INPUT.** `gpts_vix_daily_v1` holds 503 daily ^VIX closes, wired into nothing.
   Implied vol is the one volatility measure that is not a slower copy of what the panel computes.

⚠⚠ **AND THE CONSTRAINT THAT GOVERNS ALL OF IT:** the ES corpus is **284 sessions of PRICE ONLY**.
The gamma book has **~10 recorded sessions**, several of them collapsed. **Every shipped model is
price-only. Nothing using the gamma book can be tested for months.** That is not a reason to wait —
it is why the loop starts by accumulating and pre-registering.

---

## 6 · WHAT TO DO NEXT, IN ORDER

1. ⚠⚠ **WATCH THE REPLAY SLIDER ON A LIVE SESSION.** It is unit-tested (73 assertions, 20 mutations)
   and smoke-clean, and it has never been dragged. The things to look at, in order: do the ladder,
   the kings and ⓪a move **together**; does the clock read the parked bar; does ◀ reach Friday; and
   does `__gptsDebug.storage()` show **no new writes** while it is engaged.
2. **Q11 — the ex-ante deflect/break discriminator.** Still the only open question that matters.
   Detection is finished and recall-verified; the touch itself has no edge (mirror legs, 56% break).
   ⚠ 2026-08-31 gave the sharpest case yet: of his six circled deflections the rule caught **five**,
   and the miss (10:54) is structural — on a 5.15-point day the test band is 0.81 points and price
   sat inside it 95% of the time, so 28 contiguous bars collapse into ONE visit and swallow the turn.
   The pullback-first framing catches only four of six. **Neither unit reproduces his set.**
3. **`DEFLECT_ZONE` is still 0.50, fixed and symmetric, in 22 places.** The finalised ATR geometry
   governs only `hlNodeAt` and its hover. A deliberate, separately-tested change.
4. **The day-export gap (FINDINGS F-10c).** `buildDayExport` carries `day.feat` but not
   `FEAT_ARCHIVE`, so resolved outcomes older than the queue never reach the repo — the only thing
   the nightly review reads. ⚠ What trims the queue to ~29 bars is **still unknown**; two mechanisms
   were named confidently and both were wrong. Measure with `__gptsDebug.featHealth()` first.
5. **Ask him Q3** (sweep levels: furthest, or all?) and **Q4** (what is the "nd" contract?).
6. **The implied-vs-realized σ study** — `gpts_vix_daily_v1` holds 503 daily closes, wired to nothing.
7. **ITEM 18 Tier 1/2**, and the **2026-09-16** backfill deadline for the 2026-07-18→08-14 hole.

⚠⚠ **THE IRT PIPE IS SOLVED — DO NOT RE-LITIGATE IT.** IRT's Remote File field reads a `file://` URL
**once, on Apply — it does NOT poll.** Standing config, verified live: `irtserve.bat` running with
autostart, BOTH charts on `http://127.0.0.1:8000/FlexLevelsExport.csv`, Check Every 1 Minute, feed on
`gex`. ⚠ Never run that server against a panel older than v14.74.

## 7 · HOW TO WORK WITH HIM

1. **ONE AT A TIME.** State one item, its fix, ask, **STOP.** This is the most-violated rule in the
   project and breaking it has cost more rework than any bug. If a reply is taking shape with three
   headings and a "which do you prefer" — delete it.
2. **Do not build until he says build.** He says it plainly.
3. **SHOW MOCKUPS FIRST**, rendered headless at his panel width with the pairwise overlap audit.
   ⚠ **Publish design mockups as ARTIFACTS** — he comments on delivered file cards and those comments
   do not reach us; the only reason one set was ever read is that he screenshotted them.
4. **TEST BEFORE YOU BUILD.** A measurement has changed the build after it was already described.
5. ⚠⚠ **DELIVER EXACTLY ONE FILE.** His words, 2026-08-15 and restated 2026-08-27: *"you are
   supposed to just give me an install file."* One `installvNNNN.bat`, dash-free and dot-free, **plus the
   Tampermonkey links as text**, plus **tell him to click them**: run the .bat → wait ~5 min for the
   CDN → **CLICK THE LINK** → **reload the Atlas tab**. Tampermonkey's default update check is once a
   day, so the click is the reliable step. "Reinstall" means he already has it — that is correct.
6. **VERIFY THE INSTALLER BY DECODING IT** before sending. It has silently dropped whole directories
   three times, including `tools/nightly/` with the pre-registered hypothesis bank.
7. **Bump BOTH version strings** (`@version` and `GPTS_VERSION`) and the four test pins.
8. **One edit, one write, verify.** A multi-edit script that aborts writes nothing.
9. **MUTATE EVERY NEW ASSERTION INDIVIDUALLY.** "The suite is green" has never once caught a fake
   assertion; mutation has caught every one. Delete the whole construct — a condition mutation does
   not test a presence assertion, and a mutation narrower than the assertion tests nothing.
10. **End every build message with `✅ SAVE DONE`** naming what was updated.

---

## 8 · DOCTRINE THAT MUST NOT BE LOST

- **Absence of data is not a reading.** Thin cells refuse; they do not guess. And **degrade toward
  silence, not toward noise** — but **never hide real data to avoid drawing it badly** (v15.04/05).
- **A well-formed number is not a supported one.** Monotone ≠ evidence.
- **Measure the question the FACE actually puts**, and state WHEN each variable was read.
- **A matched control must SPAN its range densely**, or it invents an effect.
- **%King ranks at one instant; DOLLARS compare two moments.** A moving denominator cannot measure
  change.
- **Does it POINT or does it CONDITION?** Gamma, vanna, VIX term structure and DEX all condition.
- **Gamma tells you HOW price moves, never WHICH WAY.**
- **Before concluding data is absent, enumerate the keys of what you already hold.**
- **A count that disagrees with how the thing behaves in life is a defect in the counter.**
- **Anything unproven ships labelled unproven and scored nightly.**
