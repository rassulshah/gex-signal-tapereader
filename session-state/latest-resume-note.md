# RESUME NOTE — read this before anything else
_written 2026-09-02 · panel v15.48 · companion v1.17 · supersedes every earlier resume note_

---

## 0 · WHY THIS APPLICATION EXISTS — read `design/PURPOSE.md` BEFORE ANYTHING ELSE

**Find the day's HOD and LOD early enough to trade the move between them.** Secondarily, find the
**pullback** turning points that resume a trend rather than end it. (Operator, 2026-09-02, in full
and in his own words in `design/PURPOSE.md`; pinned by `test_purpose.js`.)

**The mechanism he is trading:** a **gamma node deflects price, and the deflection IS the turning
point** — either a trend reversal at the extreme (the HOD/LOD) or a pullback reversal that leads to
**continuation**. ⚠ **Confusing those two is the expensive error: they call for opposite trades.**

    ⓪a HOD/LOD          MEASURES the day — the turning points and their base rates
    the node ladder     watches the king, which ATTRACTS price as well as DEFLECTS it
    ⇄ · Δ15m · STATE · ROC   tracks gamma MOVEMENT: where · how much · what it means · as a rate

**Why the right-hand columns exist:** gamma **building** on a pullback may be the node that causes
the reversal and the continuation; gamma building at a high or low may be the node that causes the
HOD/LOD deflection. They answer **"is a deflection being built right now, and where?"** — the
leading signal for the turning points above.

⚠ **JUDGE EVERY PROPOSAL, PRIORITY AND CUT AGAINST THAT FILE.** If the roadmap, DECISIONS or
DEPENDENCIES conflict with it, **PURPOSE wins and the other is wrong.** ⚠ And the mechanism is a
**hypothesis he is trading, not a proven law** — keep it checkable (base rates, A over E), never
assert it.

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

## 2 · WHERE WE ARE — v15.48, and what the face carries

**Panel v15.48 · companion v1.17.** Suite **140 green / 6 baseline red** (`expiry_profile`,
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

⚠⚠⚠ **v15.48 — `sessionPhase(now)` TAKES A **DATE** AND I PASSED IT SECONDS, THREE TIMES.**
It does `new Date(now.toLocaleString(...))`. A NUMBER has `toLocaleString`, so `48000` → `"48,000"`
→ **Invalid Date** → every field NaN, **and nothing threw**. Measured 13:30 CT mid-RTH:
`deps.rthNow FALSE`, `idleMin null`.
⚠⚠ **SO BOTH GUARDS WERE INERT — INCLUDING THE ONE THAT MATTERED: the v15.45 replay stale-day guard
had NEVER ONCE FIRED.** The protection written the day he lost a morning of recording would not have
saved the next morning. ✅ One `liveSessionPhase()` = `sessionPhase(new Date())` serves all three.
⚠ **`new Date()` IS the wall clock** — no `ctOffsetSec`, no `%86400` arithmetic. Never rebuild it.
⚠⚠ **THE STUBS WERE KINDER THAN THE REAL FUNCTION — SECOND BUILD RUNNING** (v15.46 was the first).
They now THROW on a non-Date. **A double that accepts what the original refuses tests the double.**
⚠⚠ **THE PROOF IT WORKS WAS 65 FAILING ASSERTIONS** — the moment the clock was fixed the guard began
evicting both render harnesses, which park a past day on purpose. They now SATISFY the guard by
setting `RP_STALEGUARD` to today (the same state a real panel reaches), never patch it out.
⚠ **v15.47's RTH cut was "keep what I can verify" and deleted every CLOCKLESS bar**, emptying whole
series (the v15.24 blackout). Now "drop what I can **refute**" — only provably pre-open bars go. The
anchor guard still refuses a clockless bar; it is simply not ERASED on the way there.
⚠⚠⚠ **2026-09-02's EM BAND IS UNRECOVERABLE AND IT IS MY FAULT.** `capMin 299` — pinned at 13:29,
`em 9.66` against a 48.25-point day, price 22 above the expected high. My v15.46/47 faults stopped it
pinning at 08:30; by 13:29 the 0DTE straddle had decayed, so the pin is the REMAINDER, not the
EXPECTATION. Flagged `est`/`over` on the face. **Nothing recorded the open's straddle — it is gone.**
Tomorrow's is correct from the first bar.

⚠⚠⚠ **v15.47 — THE BAND'S SERIES BEGAN BEFORE THE OPEN, SO THE ANCHOR WAS NEVER THE OPEN.**
v15.46 fixed the units and the band STILL refused. Measured 13:20 CT: `MB.day '2026-9-2'` passed,
but **`cs[0].so 28800 = 08:00 CT, o 7640`** — the ES courier's window opens at 08:00 (`FUT_WIN_A`)
and `measureBars`' futures branch buckets by DAY **without cutting to RTH**. The first RTH bar was
index 30, 08:30, **o 7650.5**.
⚠⚠ **THE GUARD WAS RIGHT TO REFUSE.** `out.anchor` and `rec.openU` both read `cs[0]`, so relaxing
the check would have anchored the day on **7640 instead of 7650.5 — 10.5 points low, silently.**
✅ **CUT THE SERIES TO RTH WHERE IT IS BUILT**, so `cs[0]` IS the open and every reader (anchor,
openU, openSo, hiWater/loWater) shares one definition of the session. `emBand` now agrees with
`hodLod`, which had filtered `b.so<openSec` all along. Idempotent on the cash and replay paths;
pre-open it empties `cs`, which correctly falls to the prior-close anchor (v11.50).
⚠ **TWO INDEPENDENT FAULTS PRODUCED ONE MESSAGE** and I shipped a fix for the first. **RE-MEASURE
AFTER SHIPPING** — "it should work now" is not a measurement.
⚠ **ONLY A BEHAVIOURAL ASSERTION CAUGHT IT**: removing the cut survives every grep, because the line
is still there. `test_em_band` w4 runs a courier-shaped series and reads the anchor.
⚠⚠ **FOUND, NOT FIXED: `deps.rthNow` reported FALSE at 13:20 CT with `idleMin:null`** — the v15.43
wall-clock helper is not resolving the live phase, so the session-aware staleness is NOT engaging.
Harmless today (idle 0 grades as before). **Fix next.**

⚠⚠⚠ **v15.46 — THE WARM-UP GUARD READ `t` IN THE WRONG UNITS AND REFUSED THE BAND ALL DAY.**
He reloaded mid-session: *"nothing displayed, no ladder"*. `emBand.ok FALSE`, why *"warm-up: candle
window or ratio is not yet today's"* — **zero ladder rows**, with EVERY input healthy (FUTMODE.live,
futBars 1m old, tape 100 strikes, no render errors, `sessionBody` reading today correctly).
⚠⚠ **TWO PRODUCERS DISAGREE ABOUT WHAT `t` MEANS:** `closedCandles` stores `t: realMs` but passes
NAIVE SECONDS to `naiveDayStr`; `measureBars/ES` stores `t: r[0]*1000` (REAL ms). `naiveDayStr`
multiplies by 1000, so on a futures chart it got ms and returned a **year in the fifty-eight
thousands** — the comparison was STRUCTURALLY UNSATISFIABLE and `capOK` was permanently false.
⚠ **IT ONLY FIRES ON A FRESH CAPTURE** — a carried-over pin skips the branch — so it hid from v15.23
until the first new-day capture, then took the panel out on the first render.
⚠⚠⚠ **AND THE HARNESS WAS COVERING FOR IT:** `test_em_band` stubbed `naiveDayStr(ms)` while the real
one takes SECONDS. **A STUB KINDER THAN THE FUNCTION IT DOUBLES IS A COVER-UP** — 648 green
assertions on a panel that would not draw. The stub now matches; fixtures carry `day`.
✅ **ASK THE PRODUCER, NEVER RE-DERIVE.** No conversion works at the call site (right for one branch,
wrong for the other, and wrong for cash only after 19:00 CT when the UTC day rolls). `measureBars`
NAMES the day it selected; each bar carries the day it was kept for; compared NUMERICALLY because
formats differ (`2026-9-2` vs `2026-09-02`). No day named ⇒ decline to judge, never block.
⚠ The guard still refuses a stale ES day (the branch takes the LAST day present — v15.23's real
reason), a first bar before 08:30, and a bar with no clock.

⚠⚠⚠ **v15.45 — A REPLAY PARKED ON YESTERDAY SAT THROUGH FOUR HOURS OF A LIVE SESSION AND THE PANEL
RECORDED NOTHING.** He asked, mid-morning: *"are you recording .. market is open"*. Measured: strip
`◀ Tue 1 Sep ▶ … 13:57 ↺ REPLAY` while the real clock was Wednesday 12:54 CT, RTH — and the store
held **ZERO frames for 2026-09-02**. `recorderBlind()` gates all nine write paths in replay, so the
whole morning was never captured. ⚠ **THE BADGE SAID WHICH MODE; IT NEVER SAID THE COST.**
⚠ **NOT persisted state** — `REPLAY` is in memory, so a reload would have cleared it. The tab simply
never closed. **A SESSION-BOUNDARY bug, not a storage one.**
✅ `replayStaleDayGuard()` — a replay of a **PREVIOUS** day, still open once a **live RTH** session
has begun, hands itself back **ONCE** and says so. Rewinding **TODAY** is deliberate and untouched.
✅ A red **⚠ NOT RECORDING** banner whenever replay is on during live RTH, naming the session being
missed, and **the banner itself is the click target** that returns to live.
⚠⚠ **WALL CLOCK, NOT `sessionPhase()`** — parked on yesterday it returns `rth:true` for a session
that ended, so the guard would have agreed with the state it exists to detect. Third build running
that this trap appeared (v15.43 deps, v15.45 guard ×2).
📏 **Recovered live and verified:** frames 0 → 1 → 2 at 12:55 / 12:57, 90 vend rows each.

⚠⚠ **v15.44 — THE LADDER COLUMNS RIGHT OF `NOW` ARE ONE NARRATIVE, IN HIS WORDS (2026-09-02):**
*"the arrow column shows the movement of gamma rolling from one strike to another, the delta profile
shows how much gamma is moving, the state says it in words by classifying it, the roc gives you a
percentage."* **WHERE → HOW MUCH → WHAT IT MEANS → AS A RATE.** Do not reorder these without him.
    S@2 · Y@28 · LEVEL@56 · PRICE@104 · NODE·%KING@140 · NOW@226 · MARK@294
      · Δ15m@400 · ⇄@452 · STATE@500 · ROC 15m@558          LAD_W 618 → 608
⚠ **OPEN QUESTION, ASKED NOT ASSUMED:** he said "move the roll arrows to the RIGHT of the delta
profile" and then described the group with the ARROW FIRST. The build follows the explicit spatial
instruction (⇄ right of Δ15m); if he meant the narrative order, it is two constants.
✅ **THE ROLL WORDS COLUMN IS RETIRED** — "the arrows are suppose to show the roll, the from node
(little circle) and the to node (arrow head)." It was ONE FACT TOLD TWICE, 32px apart.
⚠ **NOTHING LEFT THE RECORD:** every sentence the chip's hover carried (the mass rule, the distance
cap, the receive ratio, "not conservation of mass", "does not say price will go there") moved to the
lane's hover, and `test_replay_face` q5 fails the build if any goes missing.
✅ Lane 20px → **44px** (the width v15.09's sketch asked for, squeezed only to fit the 640 cap).
⚠⚠ **THE Δ BARS HANG *LEFT* FROM THEIR AXIS.** `LAD_DAX` is the axis, `DAX-DMAX` is where the ink
starts. My first cut placed it at 344 and a full-width negative bar reached 288, inside the chute at
292 — `test_ladder` g4 caught it. **A column that grows toward its neighbour cannot be placed by its
own left edge.**
✅ New guard `g1b`: every ladder constant must be **FINITE** — `v()` on a deleted constant returns
NaN and comparisons against NaN pass silently, so a removed column can keep certifying its own layout.

⚠⚠⚠ **v15.43 — THE DEPS DOT WAS RED ~17 HOURS A DAY.** At 04:52 CT: `if.SPX 386m · if.QQQ 385m ·
fut.courier 426m · irt.export 294m` all STALE and `if.usable` FAIL "running blind" — **every one the
correct overnight state.** ⚠ `if.usable`'s OWN comment forbids exactly this ("would cry wolf every
session"); it guarded the SPX pin and never the clock.
✅ **THE RULE IS "WAS IT FRESH WHEN THE SESSION ENDED"** — staleness measured back from the CLOSE.
A feed last seen 14:59 is healthy at 04:52; one last seen 10:00 still FAILS and still shows its TRUE
age. **The age is never hidden; only the clock it is judged against changes.**
⚠⚠⚠ **NEVER CALL BARE `sessionPhase()` FOR ANYTHING ABOUT THE LIVE WORLD** — it is REPLAY-AWARE by
design (v15.18). Measured while parked at 14:06 it returned `rth:true · POWER HOUR` at a real
04:52. `depsSessionIdleMin()` builds a second-of-day from `Date.now()` and passes it EXPLICITLY.
⚠ Pre-open it walks back to the previous TRADING day (Monday counts the weekend). **Holidays are not
modelled** — errs toward noise, never silence.
✅ **CONFIRMED LIVE THIS SWEEP** (all previously unverified): replayed `dispScale 1.0019` (not
0.0998) on a real futures chart — closes the v15.40 unknown; EM band all one ruler (v15.41 holding);
roll lane 11 arrows; king runs 2.1–10.2px; `1ST TP · HOD` / `2ND TP · LOD`; both candles red from one
call; zero render errors.
⚠ **RAISED, NOT FIXED:** a king run priced outside the drawn frame is SILENTLY dropped — the ladder
names what it drops on its "off frame" line, the king lane has no equivalent.

⚠⚠⚠ **v15.42 — THE KING LANE WAS NEVER EMPTY; IT WAS ONE PIXEL WIDE.** The axis ran open → WALL
CLOCK, so at 19:50 the crown merely *still there* took **57% of a 24px lane** and five real
migrations drew at **1.0–2.5px**. Measured widths `1.0 · 2.5 · 1.0 · 2.5 · 2.4 · 11.4`.
⚠⚠ **A DEFECT PROPORTIONAL TO ELAPSED TIME IS INVISIBLE WHEN YOU BUILD IT AND OBVIOUS IN THE
EVENING.** Test at more than one hour — the test checks 15:00, 19:50 AND midnight.
⚠ **WHEN HE SAYS "EMPTY", MEASURE THE ELEMENT GEOMETRY BEFORE THE DATA.** I went to the data first,
twice. The lanes were drawn, populated and correct — and unreadable.
✅ The axis now ends at the **last closed bar** (data, not clock arithmetic); shrinks only, only once
RTH is over. Long runs gain ~1.7×; the "still there" run drops 57% → 19%.
⚠ **RESIDUAL, NOT FIXED:** a 15-minute run out of 390 is still sub-pixel in 24px. Floored at 1px so
it is faint, never absent.
⚠⚠ **THE ROLL LANE: `rollsLive()` ASKED `rth` WHILE THE REST OF THE FACE ASKED `showingStaleBook()`.**
After the close the panel serves the close-of-session book — nodes, states, ROC — and blanked that
same book's ROLLS. **One face, two opinions about which session is on screen.** Now `rollsLive()`
also returns true for the frozen book; `rollLatched()` still refuses live arrows over a replayed bar.
✅ **AN EMPTY ROLL LANE NOW NAMES ITS SILENCE** — four distinct cases, four sentences. `return ''`
had made "no rolls today", "retired at the close" and "the latch is empty" identical.

⚠⚠⚠ **v15.41 — A REPLAY PIN OUTLIVED THE REPLAY AND FLATTENED THE LADDER.** Measured LIVE after he
rewound: pin `SPY|fut = {openU:761.79, rr:1, fam:'replay', replay:true}` → band `low 729.29 /
high 794.29` (SPY) against `now 7647.50 / hiWater 7673.75` (ES). `emRailBounds` STARTS the frame at
`B.low`, so the rail spanned **6,944 points** and every row collapsed onto one line.
⚠⚠ **ONE UNGUARDED WRITE, FOUR OVER-GUARDED READS.** `replayEmPin()` (v15.24) builds the pin; four
heals refuse to repair a `replay:true` pin (v15.26, correct but UNSCOPED); and the **ratio heal**
(v14.19) does `S.sym[emKey]=rec` with **no replay guard at all** — it PERSISTED the replay pin into
the LIVE key where nothing could repair it. Each rule individually correct; **none asked whether the
replay was still happening.**
✅ **`rpPin(r) = r.replay && replayOn()`** — *exempt RIGHT NOW*, not *born in replay*. **All FIVE**
sites call it, the write included. ⚠ **When one condition is restated at five call sites, the bug is
the five.**
✅ **AND A GUARD THAT TRUSTS NO FIELD:** an anchor and a price on one chart cannot be a factor of
two apart. Overrides every exemption, disclosed as `rulerOff`. **v11.65, v15.12, v15.24, v15.26 and
this one would all have tripped it.**
⚠ **COMPARE THE RAW SERIES VALUES** — my first cut scaled `rec.openU` and `test_em_band` went red
("openU is scaled in exactly TWO places"). It was right: scaling first compares two numbers AFTER
applying the very ratio in doubt.
⚠ **I ASKED HIM TO REWIND, AND THE REWIND WROTE THE PIN.** When a report follows an instruction I
gave, suspect the instruction.

⚠⚠⚠ **v15.40 — THE REPLAY LADDER WAS EMPTY BECAUSE OF A SCALE, NOT A CAPTURE.** He reported it
THREE TIMES as "you are not capturing the state". ⚠ **THE STATE WAS CAPTURED PERFECTLY EVERY
MINUTE.** Measured replaying 2026-09-01 14:21: the frame held 36 SPXW strikes, seven clearing 20% of
King, **ALL SEVEN inside the band** (7630 −100, 7625 +77, 7610 +52, 7635 −48, 7620 −44, 7615 +39,
7650 +21). `replayLadder` returned `dispScale 0.099775 === undScale 0.099775` — the CASH scale in
both slots — so SPXW 7630 drew at **761.28 on a ladder framed 7615..7680**, ~6,880 points below it.
`inFrame()` refused every node. ⚠ **`dispScale` IS THE CHART SCALE; `und/spx` IS THE UNDERLYING ONE.
Their EQUALITY is the alarm** — it is only correct on a cash chart.
⚠⚠ **THE FIX WAS ALREADY IN THE FILE, FORTY LINES BELOW**, v15.06, in capitals: "THE FIX IS ONE
SCALE, NOT A BETTER FALLBACK." `replayLadder` was written in v15.18, AFTER it. **A lesson written as
prose guards only the function it sits in.**
⚠ **BEFORE BELIEVING A CAPTURE COMPLAINT, READ BACK THE STORED RECORD** — one probe would have
settled this on any of the three occasions.
✅ Replayed basis = today's ES/SPY ratio (no frame records an ES print), DISCLOSED as
`scaleSrc:'replay:fut:ratio-today'`; no ratio ⇒ cash, and it says so. Level rows still refused.
✅ **1ST TP · LOD / 2ND TP · HOD** — the headings name the turn. They said "1ST HOD" until v15.33, so
the rename had silently dropped it. The second is named only once it has PRINTED.
📏 **MEASURED, NOT FIXED — the day does not fit.** Store holds 14:00→15:00, **24 frames**. Each is
**26,551 bytes, 64% of it `feat`** (the learning vector, which nothing that DRAWS reads); `vend` —
what the replay ladder is rebuilt from — is 10%. A full RTH day = 130 × 26.5KB = **3.44MB against a
3.6MB budget (96%)**, so any growth evicts the morning. Dropping `feat` from the frame ⇒ ~1.2MB.
**Two budgets is the clean form: the replay slice must not be evicted by the learning payload.**

⚠⚠⚠ **v15.39 — `emBand` MULTIPLIES EVERY PRICE IT RETURNS BY `emRr`, AND UNTIL NOW SAID NOTHING.**
Measured 2026-08-31: bars high **769.88**, `EB.hiWater` **772.28**, ratio **1.0031195570**. And
`scaleUsed` reads **1**, so a caller checking for a conversion is told there is none. ⚠ **THE LADDER
IS EM SPACE; `hodLod`/`sessionBody` ARE BAR SPACE.** Anything moving between them must multiply.
I walked into this INSIDE the fix for it — v15.39b put bar prices on the EM rail, the body hung
below its own wick (jsdom) and the expected move collapsed to 1% of the view (real Chromium), which
is v15.28's exact fault reintroduced. ⚠ **THE COLOUR IS SCALE-INVARIANT; THE COORDINATES ARE NOT** —
share the FACT, convert the COORDINATE. `emBand` now publishes `emRr`; use it, never derive it.
⚠⚠ **THE ORIGINAL DEFECT:** the NOW-column candle drew `EB.open`→LIVE TAPE (RED) while ⓪a drew
`hodLod.open`→last CLOSED bar (GREEN), on the same session. **The panel was FROZEN and the NOW
candle was still following the after-hours tape** — `recorderBlind()` gates every WRITE and gated no
READ. ⚠ The day was FLAT (+0.50 on 52.25) so **the disagreement was 6× the body**.
✅ `sessionBody(sym)` now owns the session's open/close/hi/lo; both candles read it. `__gptsDebug.sessionBody()`.
📏 **MEASURED over 284 sessions:** median body 43% of range; **13% of sessions have a body smaller
than the 3.25pt error** — one day in eight it decided the colour.
⚠ **FOUND, NOT FIXED:** the panel carries TWO session highs — `emBand.hiWater` and `hodLod.hod` —
consistent only through `emRr`. Now labelled, not yet unified.

⚠⚠⚠ **v15.38 — THE FUTURES-GAMMA WORK IS PARKED, NOT ABANDONED, AND NOT YOURS TO START.**
`design/spec-futures-gamma-markets.md` — gamma levels for **CL · NG · GC · E6 (· HG pending)** from
the real CME chains, free, into the IRT export. Operator, 2026-09-01: *"hold this implementation
detail somewhere... we will come back to it once the application with the current markets is
optimal."* ⚠ **DO NOT START IT, AND DO NOT RE-RESEARCH IT.** `load gex` step 1a-1 routes you there.
⚠ **v15.38 CHANGED NO PANEL CODE** — version string only; the installer is the delivery channel.
⚠ **THE THREE TRAPS THE SPEC EXISTS FOR** (each measured, each would ship a wrong number):
`<ROOT>*1` (nearest) vs `<ROOT>*0` (most active) — **`*1` puts GOLD and COPPER on SEPTEMBER**, months
he does not trade; the DTN root map is **hand-written, never derived** (copper is `CPE` not `HGE`,
euro is `E6` with no suffix and Barchart's root is `E6` so `6EU26` 404s); and **the chain is not in
the raw HTML** — 466,227 bytes, `"strike":` appears ZERO times, which is why delivery is a reader
userscript rather than a fetch.
⚠ **THE NEGATIVES, which cost the most to establish:** Skylit returns **zero snapshots** for every
futures symbol **including ES1** (SPY returns 390); InsiderFinance is equity/ETF-only; the ETF
conversion (FXE/GLD/USO/UNG) was **dropped and is explicitly NOT a fallback**.
⚠ **OI IS PUBLISHED ONCE A DAY BY THE EXCHANGE — there is nothing to poll.** True for the paid
vendors too. One pull after the open is correct, not a compromise. **Do not build a poller.**
✅ `test_parked_specs.js` (36) fails the build if any of it is deleted. 24 mutations, 24 caught —
**after a fix**: two passed initially because I asserted words appeared *somewhere* in a document
that deliberately states the warning twice.

⚠⚠⚠ **v15.37 — FOUR HEADER LAMPS, ONE PER EXTERNAL FEED: `IRT · IF · YF · FF`.** He asked for
indicators for **Yahoo Finance (YF)** and **ForexFactory (FF)** and added *"All of this integration
better be mentioned somewhere. check where it is mentioned."* ⚠ **THE AUDIT:** Yahoo was in
`DEPENDENCIES.md` §2 but the heading never said Yahoo, and it had no lamp. **ForexFactory was in TWO
CODE COMMENTS AND NOTHING ELSE** — no doc section, no `deps()` item, no lamp, since v14.38.
⚠⚠ **§0 of that very file says every dependency here fails silently and must be written down.**
Writing the warning is not obeying it; `test_deps.js` now enforces all four steps (item · lamp ·
test · SECTION HEADING) and is **76 assertions**, up from 36.
⚠⚠ **FF IS COUNTED, NOT AGED** — it is delivered once a day, so an age reads "FF 340m" on a healthy
calendar by mid-session. It shows the EVENT COUNT, and **`0ev` is a real, healthy answer**; the
green dot is what says the courier ran. `cal.ff` tests the `day` STAMP, never the count — a
count-based check calls every quiet day broken.
⚠ **BOTH FAIL INTO A PLAUSIBLE FACE:** stale Yahoo bars still have a high and a low (the ⓪a candle
draws a WRONG one), and a missing calendar removes a caveat rather than blanking a section.
📏 **MEASURED in real Chromium at 673px:** the four lamps take 185px, ending x=337; right-hand
controls start x=529 — **192px slack**, panel floor is 652. Labels pinned to ≤3 chars.

⚠⚠⚠ **v15.36 — THE KING LANE IS NOT A CENSUS AND I QUOTED IT AS ONE.** He asked "for each type of
king, how many rolls were there" and I read the numbers off `KTRACK`, the **king lane** — which is
dwell-filtered to 20 minutes **because he asked for it to be** (v15.23, "too erratic"). Throwing
changes away is the lane's JOB. ⚠ **Median ratio census : lane over his own 8 recorded sessions =
×3.0.** ⚠ And the count still CLIMBS as sampling gets finer (15m→6, 9m→7, 6m→9, 3m→12 for SPXW), so
**every number this file can produce is a FLOOR** and Atlas, recomputing continuously, reads at or
above all of them. ⚠ **THE HONEST FORM IS "AT LEAST N".** ⚠ MEASUREMENT WITHDRAWN: any earlier
per-king roll count — those were lane counts.
✅ **NOW SHIPPED:** `gpts_kingraw_v1` — one entry per crown CHANGE, ~15s cadence, **all three books**
(QQQ included: excluding it from the DRAWING had silently excluded it from the ANSWER).
`__gptsDebug.kingTrack()` returns `rolls` (census) *and* `migrations` (lane), and the lane tooltip
says both. `replayKingRaw()` rebuilds the same shape from 3m frames and **declares its coarser
basis**. ⚠ **AND A REAL BUG FELL OUT:** `ktTick` had NO book-depth floor — a half-loaded first paint
has a king and it is noise; the dwell was absorbing it. `krTick` reuses `SK_MIN_STRIKES`, the same
floor `skPiles` and the LASTBOOK latch use. ⚠ **A tolerant consumer makes an unguarded producer look
correct.**

⚠⚠⚠ **v15.35 — THE FREEZE BADGE PRINTED A 1970 TIMESTAMP AND IT LOOKED REAL.** `LB.ts` is epoch
MILLISECONDS, `fmtClock(ts)` does `new Date(ts)` (also ms), and the call site divided by 1000 in
between: `1788296340000 → 1788296340 → 1970-01-21 → **10:44 am CT**`. A book latched at **14:59:00**
displayed as "frozen 10:44 am".
**THE LATCH WAS ALWAYS CORRECT** — SPXW king 7630, 100 strikes, ts 14:59:00, exp 2026-09-01. Only the
label lied, and 10:44 am is plausible enough to be believed. ⚠ **A disclosure that lies is worse than
no disclosure**: silence prompts a question, a wrong number ends one. ⚠ And the comment two lines
above says the badge "names the SESSION and the CLOCK TIME" — it did, and still printed 1970.
✅ **EVERYTHING ELSE POST-CLOSE CHECKED CORRECT:** lamps in the header (`IRT 1m · IF 1m`), AFTER HOURS
chip, ⓪a below the ladder, 1ST TP / 2ND TP, brighter labels, zero render errors. `deps` has one
failure left — `if.SPY: missing expected move` — which is EXPECTED under the v15.06 SPX pin.

⚠⚠ **v15.34 — THE FEED LAMPS ARE IN THE HEADER** beside the version chip (they cost a 13px row on
the top strip). `render()` paints `#gpts-hdrlamps` each pass. ⚠ Their CSS is scoped to `#gpts-panel`,
NOT `#gpts-body` — a selector scoped to a parent encodes a LOCATION, and locations change.
✅ **THE CLOSE-OF-DAY FREEZE, VERIFIED LIVE at 14:57 on 2026-09-01:** `CFG.lastBook true`,
`gpts_lastbook_v1` holding SPXW king 7625 / 100 strikes stamped that second, recorder at 31 frames.
⚠ **TWO STORES, TWO ANSWERS ABOUT THE SAME DAY:** the RECORDER started at 13:39 (panel reloaded, and
it only runs while open) so the slider covers 13:39→close; the KING TRACK is a separate day-keyed
latch that SURVIVED the reload and starts at 08:30. Say which store when reporting coverage.
📌 **2026-09-01 KING ROLLS (his question):** SPXW 5 recorded migrations / 4 drawn after the 20-minute
dwell; SPY 3 recorded / 2 drawn; **QQQ NOT TRACKED AT ALL — `KT_BOOKS` is SPXW + SPY only**, though
the ladder draws a QQQ crown. Atlas itself publishes no roll count, only a live King-distance chip
per symbol, so there is nothing on their side to reconcile against.

⚠⚠⚠ **v15.33 — MY OWN `deps()` CHECK CALLED IRT BROKEN WHILE IRT WAS WORKING.** It reported
`irt.build: nothing to write` while `IRT_LAST` held `{rows:6, how:'file', inPlace:true, err:null}`.
v15.22 re-ran `irtBuildCsv()` AS A PROBE, and that rebuild depends on live inputs (the IF ladder, the
ES ratio, the latched crown), so one unlucky instant reads as a dead export.
**A HEALTH CHECK MUST OBSERVE THE SYSTEM, NOT PERTURB IT** — it reads `IRT_LAST.rows` now and probes
only when no export has run this session. ⚠ Found only because I read the live state before building
the lamp; shipping the lamp on that check would have glowed red at him all day over nothing.

⚠ **TWO FEED LAMPS ON THE TOP STRIP** (`feedLampsHtml`), both from the SAME `depsHealth()` the footer
dot and `__gptsDebug.deps()` read — never a second opinion:
    IRT 2m   the panel WRITING king levels to the file IRT polls
    IF 3m    InsiderFinance ARRIVING — the age of the FRESHEST usable chain
Each states its AGE: a green dot with no number is a claim you cannot check.

⚠ **LAYOUT:** ⓪a HOD/LOD is mounted BELOW the ladder (the mount moved, `secDay()` untouched);
the columns are headed **1ST TP / 2ND TP** with the extreme's identity moved to the hover; the ⓪a
labels went `#6c7889` → `#9fb0c4` (3.1:1 → ~7.4:1 on the card — his "dark grey" was a real number).

⚠⚠⚠ **v15.32 — EVERY BUILD MESSAGE CARRIES TWO BLOCKS THE BUILDER PRINTS. PASTE BOTH.**
Operator, twice: *"i dont see the tamper monkey links or save confirmations."* Scroll to the END of
`python3 tools/build-installer.py` output and copy:

    ==== PASTE THIS WITH THE INSTALL FILE ====     both Tampermonkey links, CHANGED/UNCHANGED
    ==== SAVE CONFIRMATION — PASTE THIS TOO ====   chat history · lessons · changelog · resume note

The save list is read from `git show --stat HEAD`, so it cannot be written from memory, and each
mandated file is marked `saved` or **`MISSING`**. `skills/gex/SKILL.md` step 0-bis requires both.
⚠⚠ **THE LINKS HAVE PRINTED SINCE v14.3 AND STILL WENT MISSING FOR SEVERAL BUILDS**, because pasting
them lived only in a context's head. **The only rules this project keeps are the ones something
prints or something fails on** — and I read that sentence in LESSONS §0 this session and still
let the step lapse.

⚠⚠ **v15.31 — THE STRIKES WERE NEVER MISSING; THEY WERE BELOW `nodeThresh` (20% of King).**
Measured: 100 SPXW strikes on the tape, ELEVEN drawn. The filter is right — a NODE is a strike with
mass — but a price axis with holes reads as data loss. Every in-frame strike now draws a 1px tick at
18% opacity, width = its own %King. ⚠ **A GAP IS INFORMATION**: grid drawn with no bar = no dealer
mass between two levels, the air pocket, previously invisible because the row was absent.
⚠ **THE DAY IS A CANDLE BEHIND THE NOW COLUMN** — wick = `hiWater`..`loWater`, body = `open`..now,
green when up. Every number is one the ⓪a section already measures and the band is anchored on, so
the candle and the band can never describe different sessions.
⚠ **THE VIEW MUST HOLD THE WHOLE DAY** as well as the band — asserted by c3 the way L7b asserts the band.
⚠ **IBH/IBL ARE OFF THE LEVEL RAIL** (both call sites) and still MEASURED by `sessionLevels`.

⚠⚠ **v15.30 — THE GRIP WAS CAPPED AT 560 WHILE HIS PANEL WAS 673.** The first pixel of drag snapped
it DOWN and pinned it, so widening was impossible and the gesture read as dead. 560 was right when
the ladder was 588px; `ladderFit()` has been growing the panel past it for builds. `panelWidthBounds()`
is a FUNCTION now — floor = the ladder's own width (narrower just hides columns), ceiling = the
viewport — so it can be executed instead of grepped. ⚠ **A rule the operator's hands touch is
behaviour and gets a test that runs it.**
⚠ **THE TAPS COLUMN IS RETIRED AND THE ROLL LANE HAS ITS SLOT (x 344).** `LAD_W` came DOWN 640 → 618.
The tap COUNT survives in the STATE hover — removing a badge must never remove the measurement.
⚠ **"AT ALL TIMES SHOW EH TO EL"** is asserted in a real browser as a SPAN (L7b), not as two separate
labels: checking each edge alone passes a view holding one and not the other.
⚠⚠ **THREE BUILDS RUNNING, A LITERAL IN A TEST WAS WHAT BROKE.** r11 pinned `LAD_ROLL=620`, r12
pinned `LAD_W=640`, and both failed on a change that honoured what they were protecting.
**A position is a decision; a property is a fact. Assert the property.**
⚠ And I tripped the documented `val()` landmine myself: `typeof LAD_W==='number'` earlier in the file
than the declaration IS read as the declaration. Write `'number'===typeof X`.

⚠⚠⚠ **v15.29 — THERE IS A REAL BROWSER IN THE CONTAINER. USE IT FOR ANY LAYOUT QUESTION.**

    node tools/render-face.js <day> <hh:mm> --page     # standalone doc: panel CSS + body
    node tools/measure-ladder.js                        # lays it out in Chromium and measures
    node test_ladder_layout.js                          # 7 assertions, all in a real browser

**jsdom has NO layout engine** — every box measures 0, `scrollTop` never moves, `max-height` does
nothing — so every layout property was a `[GREP]` and the greps were guarding a clamp THAT DID NOT
WORK. On the first real-browser run, v15.28's EL label rendered at **299..312 in a 300px window**.
Three compounding faults, none visible without layout:
1. the pill's `top` is its **CENTRE** (`height:13px; transform:translateY(-50%)`) and I had **guessed
   11** for a box the stylesheet declares as 13 — one line away in the same file;
2. the **frame is not the view** — the window opens on the band and is shorter than the content;
3. the **header row shares the scroll box**, so `max-height:viewH` gives the ladder `viewH − 12`.
⚠ **A container's height is the sum of what is IN it.** ⚠ **`[GREP]` is a debt, not a resolution.**

⚠⚠⚠ **v15.28 — THE LADDER OPENS ON THE EXPECTED MOVE AND SCROLLS TO THE REST. HIS SPEC, VERBATIM:**
"at the open the ladder should be drawn from the expected move low to the expected move high and then
from that point on should adjust its height based on price movement taking out either side as well as
allowing me to scroll up and down."
    content = every node at its true price, ONE coordinate system, nothing clipped (v15.04's lesson)
    window  = EL..EH, widened by price taking out either side, +4% air
    scroll  = the wrapper scrolls vertically; applied ONCE per row-set so it never fights a manual scroll
⚠⚠ **AND THE REASON IT WAS NEEDED: IN REPLAY THE BAND WAS A FIVE-PIXEL SLIVER.** `feat.emband` is
recorded in CHART units while the frame's `px` is the UNDERLYING price, and the replay pin carried
`rr:1` — so band 7661..7730 sat beside its own `now` of 764.49 and the frame spanned **6,986 points**.
**The ratio was in the frame all along: recorded anchor ÷ the series' own open.** Fourth build running
that a mixed ruler was the fault.
⚠ **THE EXPECTED LOW WAS NEVER MISSING** — it was drawn at `top:300px` in a 300px frame. And that is
the NORMAL case: `emRailBounds` starts the frame AT the band, so both labels land on an edge by
construction. The label is clamped inside by its own height; the RAIL stays on the true row.
✅ **AND THE ARITHMETIC, VERIFIED ON HIS LIVE v15.27:** ES open 7647 ± 32.5 → EH 7680 / EL 7615, drawn
exactly. Rows spread 30.7→239 with a 19px median gap.

⚠⚠⚠ **v15.27 — `EB.scaleUsed` HAS TWO MEANINGS. DO NOT CHANGE WHAT IT CONTAINS.**
TEN call sites multiply an UNDERLYING-book value by it to reach chart space (the SPY King flag, the
prior-day levels, the dark-pool prints, `levelMarkerOf`). The BAND needs the scale of the series IT
measures. Those were the same number only while the band measured the underlying book — v15.24 moved
it to the ES series (scale 1) and v15.26 made the pin agree, so **PDH drew at 768 on a ladder of ES
strikes** and twelve rows crushed into six pixels.

    out.scaleUsed   = UNDERLYING book → this chart (~10.0353)   ← the contract. Never repurpose it.
    out.seriesScale = the band's own series scale (1 for ES bars) ← use this inside emBand

⚠⚠ **AND v15.26's GEOMETRY GUARD PASSED THE FAULT IT WAS WRITTEN FOR.** y2 asserted min-to-max
spread; his ladder had twelve rows in 6px and one outlier at 636 → spread 635px → green.
**A RANGE IS NOT A DISTRIBUTION.** y3b (median gap ≥6px) and y3c (no tenth holds >70% of rows) are
pinned against his ACTUAL measured tops, with an assertion that the old test passes them.
⚠⚠ **TWO BUILDS IN A ROW WENT OUT LIVE-BROKEN (v15.24 blank, v15.26 scrambled), both with a green
suite, both found by him in one glance. For any change to a scale, a unit or a shared field: RENDER
THE FACE AND LOOK AT IT** — `node tools/render-face.js <day> <hh:mm>` exists and I did not use it.

⚠⚠⚠ **v15.26 — I SHIPPED A BLANK LADDER AT v15.24 AND THE WHOLE SUITE WAS GREEN. READ THIS BEFORE
CHANGING WHERE ANY NUMBER COMES FROM.** v15.24 moved the band's anchor to `measureBars()` (ES bars,
already chart-scale, rr 1) and left the STORED pin's `rr: 10.0353` from the derived SPY series.
`useRr` preferred the stored one, so `hiWater = 7673 × 10.0353 = 76,986`, the rail frame spanned
~69,000 points, and **all thirteen rows drew at `top:639.7px` of a 640px frame**. Audit ok, zero
render errors, 134 green, blank panel.
**A SCALE STORED IN ONE SERIES' UNITS IS MEANINGLESS AGAINST ANOTHER** — third instance (v11.65,
v15.12). The pin now records `src` (the series) and is REBUILT from `emK` when the series or scale
changes. ⚠ Replay pins are exempt, like the v15.24 heal — forgotten twice in three builds, caught
both times by the cross-examination against `feat.emband`.
⚠⚠ **AND THE SUITE COULD NOT SEE IT because every assertion checked PRESENCE.** `test_replay_face`
y1-y5 now assert the GEOMETRY: rows spread, at distinct heights, the band spanning points not tens
of thousands. **Presence is not legibility.**
⚠ **WHEN HE REPORTS A DISPLAY FAULT, SCREENSHOT FIRST.** Every debug surface reported healthy —
correctly — while the face was empty.

⚠⚠⚠ **v15.25a — THE DELTA PROFILE'S REAL EDGE, MEASURED. DO NOT OVERSTATE IT AGAIN.**
`tools/study-deltacadence.js`, 13 sessions (08-17..08-31): **BUILDING is 52.8% against a 50% coin**
at a 30-minute horizon, and it does NOT improve with a bigger move (+4-10% → 52.1%, +100%+ → 52.5%).
**The only differentiator is DISTANCE: within 25 points of spot 56.9% (n=1266) vs 51.7% further out
(n=3999).** The measured parts of this section are the ROLL and SPENT (19/19 pass-throughs), not the
15m change. ⚠ His business requirement is support/resistance prediction — say 53% when it is 53%.

⚠⚠ **v15.25b — THE STATE IS HELD FIVE MINUTES BEFORE IT CHANGES (`LVL_HOLD_MIN=5`).** Measured:
hold 0m → 14.1% of reads change state, 52.8% edge · **5m → 9.3%, 52.2%** · 10m → 5.6%, **50.6% (a
coin)**. Five removes a third of the churn for nothing; ten takes the signal. ⚠ Replay is EXEMPT —
the slider jumps between minutes and a per-strike cache would carry state across a two-hour leap.

⚠⚠ **v15.25c — THE ROLLS WERE ALWAYS DRAWN AND NOBODY COULD READ THEM.** Four real rolls
(7645→7665 $18M, 7650→7665 $16M …) as stepped paths in a 20px column at the far right, no strike
named. Each row now carries `⇢7675` (amber, leaving) or `⇠7650` (blue, arriving). ⚠ **"It renders"
is not "it is readable".** Its 32px came from the ROC column, which lost the 5m at v15.23.

⚠ **THE AMBER LINE crossing pills was the EM rail drawn INSIDE the pill chute** at the band edge's
true price, while its label steps away to clear the crowns. Two dashed segments now stop either side
of the chute. ⚠ The line was in the right place and the wrong column.

✅ **EH/EL ARITHMETIC VERIFIED on his pin:** `openU 759.5653 × rr 10.0353 = 7622.4`, `± em 32.31` →
**7655 / 7590**, matching the face exactly. The width IS InsiderFinance's 0DTE ATM straddle captured
at the open; it is added to and subtracted from the open. The ANCHOR was the fault (v15.24).

⚠⚠⚠ **v15.24a — THE CROSS-EXAMINATION IS THE STRONGEST TEST THIS PROJECT HAS. USE IT.** Every frame
records what the LIVE face was reading — `tri.<book>.top`, `tri.<book>.king`, `feat.emband` — so a
replayed render is checked against THE RECORDING, not against my expectation. `test_replay_face`
does this (x1-x6) and caught a regression on its first run: v15.23's own heal was overwriting the
replayed pin, 771.74 recorded vs 769.34 drawn. **Both plausible; only the recording knows.**

⚠⚠ **v15.24b — WHY REPLAY COULD NOT CAPTURE THE DAY.** Measured on his recording: 34 frames, and
**EIGHT carried no `tri`, no `vend`, no `px`** — empty shells the slider offered as seekable ticks,
so the handle landed on one and the face went blank. The recorder refuses to write them now, and
`replayLoadDay` drops the ones already on disk (`replayUsable`), reports the count, and distinguishes
"the frames carry no book" from "none were recorded".
⚠ His recording that day also STARTED AT 09:03, not 08:30 — the recorder only runs while the panel is
open. Not a defect; a limit to state when the slider will not go back further.

⚠⚠ **v15.24c — A RULE ENFORCED ONLY AT WRITE TIME CANNOT FIX WHAT IS ALREADY STORED.** Twice in one
build: the king lane's dwell (KTRACK already held the day's flickers — his lane still showed **23
runs** after v15.23) and the empty frames. Both now enforced where the data is READ.
⚠ And the replay rebuild NO LONGER filters as it builds: **one rule, one place, both paths.** Two
copies looked like belt and braces and were invisible to mutation.

⚠⚠ **v15.24d — THE BAND ANCHORS ON `measureBars()`, NOT `closedCandles()`.** On a futures chart the
underlying series is DERIVED — rebuilt from ES through a moving basis — so the same 08:30 bar read
759.5653 and later 761.9526. The band drew 7590-7655 against a real ES open of 7647 with price at
7663, ABOVE the expected high. **A value recomputed from a moving input is not a record of anything.**

⚠⚠ **v15.23a — THE EM BAND WAS ANCHORED ON YESTERDAY'S OPEN AND NOBODY COULD SEE IT.** Measured
on his panel: pin `openU 768.6968 · rr 10.0353` → anchor **7714**, against a real ES open of **7647**
(courier) and a SPY first candle of **761.93**. Price sat below EL all day and EL wore the ⤓.
Three stacked failures: the warm-up guard tested **`cs[0].time`, a field the candles do not have**
(they carry `t`/`so`), so it never fired; the capture then ran at 08:30:08 against an array that had
not rolled to today and stored `openSo:null`; and the self-heal REQUIRES `openSo` to be a number, so
a badly-captured pin was permanent for the session. ⚠ The width was always InsiderFinance's 0DTE
straddle and the anchor was always meant to be the open — **the width was right, the open was
yesterday's.**
⚠ `_openSec` must stay at FUNCTION scope: inside the capture branch it hoists as `undefined` on
every render where a pin exists, and the heal silently cannot run.

⚠⚠ **v15.23b — `KT_DWELL` WAS A COUNT READ BY TWO LOOPS AT DIFFERENT RATES.** Live it ticks per
render (seconds → ~6s of probation); in replay it walks 3-minute frames (→ 6 minutes). One name, two
rules, and the live king lane was effectively unfiltered — "too erratic". Now **`KT_DWELL_MIN=20`**,
a DURATION honoured by the clock in both paths. Measured over **11 sessions, 08-17 to 08-31**:

    dwell   SPXW median [min-max]   SPY
      0m    5 [0-15]                5 [0-11]     ← what he was seeing
     20m    2 [0-4]                 3 [0-5]      ← shipped
     30m    2 [0-4]                 2 [0-4]      ← starts erasing real moves

⚠ n=11, one instrument, one three-week window. ⚠ A duration also means a GAP in the recording cannot
promote a flicker, which a count could never notice.

⚠ **THE ROC COLUMN IS 15m ONLY** (matching the Δ column). The 5m is still COMPUTED and still decides:
TURN needs 5m and 15m to agree and both to have flipped against the hour. BUILDING/WEAKENING are 15m.
All three windows remain in the hover.

⚠⚠⚠ **v15.22a — THE DEPENDENCIES HAVE A LIVE CHECK, AND `load gex` MUST READ
`design/DEPENDENCIES.md`.** Operator-mandated: *"it is fundamental to the application."*
InsiderFinance (call wall, put wall, expected move), the ES 1-minute courier, the IRT export and the
recorder ALL live outside this script and ALL fail silently. **A green suite has never meant they
are up** — the suite runs in Node, where none of them exist. Before diagnosing any missing surface:

    __gptsDebug.deps()      ← run this FIRST. The `deps` dot on the footer carries the same verdict.

⚠⚠ It found a fault on its first run: the stored **SPY chain was 15,328 minutes old** (payload
2026-08-21) with a **null expected move**, while SPX and QQQ were three minutes old — and its own
`stale` flag read **false**, because that flag is written once and never re-evaluated. **Freshness is
a question about the clock NOW.** ⚠ A stale SPY is EXPECTED under the v15.06 SPX pin, so the overall
verdict asks "is there a usable book", not "is every symbol fresh" — a check that is red every day is
a check nobody reads.

⚠⚠ **v15.22b — "LOD IN 74% · HOD after 1:30 — 80%" NOW DRAWS. IT NEVER HAD.** The far-side model
shipped at v14.72 gated on `D.secondT > D.clock` — but **`secondT` is the later of the two RUNNING
extremes**, and a session has a running high and low within two bars. Measured: at 08:45 on
2026-08-31, `secondT` was 08:39. The gate was false from the third bar of every session, so the
far-side line never drew, "both extremes in — the range is set" printed all day about an unfinished
range, and the "% of the range" clause never appeared. **Three clauses, one wrong idea, eight
builds** — and `test_hodlod` u8/u9 DEMANDED that gate, holding it in place.
All three are gated on the table's own IN call now, and the floor is anchored on `D.clock` rather
than `ctNowSecOfDay()` (with the wall clock, a replayed 10:00 and 14:12 gave the same floor).
⚠ A floor past the close prints as a refusal, not a time.

⚠ **THE COMPANION LINK SHIPS WITH EVERY BUILD NOW.** The panel's has printed since v14.3 and the
companion's never did, so every build that changed the companion told him to update it and handed him
no way to.

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
