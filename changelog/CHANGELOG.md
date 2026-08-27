## v14.56 — the chute has two kinds of occupant, and the nudge only knew about one

Found by rendering the mockup the operator asked to see, not by reading the code. The pairwise
overlap audit on `mockups/lastbook-v1455-MOCKUP.png` reported:

    empill "EL 7708"  x  king "~7716 QQQ"

**v14.54 moved the EM edges AND the three crowns into the same 66px chute** — that was the point of
the change, it is what emptied the left gutter — **but it left the nudge loop comparing crowns
against CROWNS ONLY.** `used=[]` started empty and only ever collected king rows, so an expected-move
pill and a king within ~15px drew straight through each other. The crowns had been nudging apart
correctly since v14.48; what changed is that they acquired neighbours.

Fixed by collecting the EM pill rows into `CHUTEY` as they are drawn and **seeding the nudge list
with them** (`used=CHUTEY.slice()`), so a crown avoids an expected-move edge exactly as it avoids
another crown — same column, same collision. `test_ladder` k11b/k11c pin both halves and the
empty-seed mutation goes red.

⚠ **THIS IS THE THIRD TIME THE OVERLAP AUDIT HAS BEATEN A SCREENSHOT**, and the second time in two
builds: v14.54 found a role label six tenths of a pixel inside the chute, then found that the clamp
written to fix it had stacked the label on its own bar. None of the three was visible by reading, and
the first two were invisible in a screenshot too. `PROJECT-CONSTANTS` L-D is carrying its weight.

⚠ **THE MOCKUP WAS THE DIAGNOSTIC, NOT THE DELIVERABLE.** The operator asked to see the design after
the build rather than before, because he had said "I'll go with your recommendation so I can continue
working". The picture then found a defect in code he had already installed. The standing rule is
mockups FIRST; a feature of this size gets its picture before its code next time, go-ahead or not.

**Version bumped without shipping.** The fix changed the userscript while it still read v14.55 — the
version already installed on his machine. Two different files claiming one version is failure
pattern #9, so the repo moved to 14.56 immediately even though delivery is his call.

Suite 117 green, 6 documented baseline reds. test_ladder 108 -> 110 asserts.

## v14.55 — the close-of-session book, so the panel is not flat after the close

Operator, an hour after the close: *"we are suppose to have a rule to show the last day so i can
continue working."*

**⚠ THE RULE WAS NOT BROKEN, AND MUST NOT BE "FIXED".** Measured on the live panel at 17:11 CT:

    session   showing 2026-08-27 · replay:false · rth:false      <- pickSessionDay was CORRECT
    velocity strikes by expiry:  2026-08-28 -> 256 · 2026-09-16 -> 70 · 2026-08-27 -> 0
    strikes with a non-zero 15m delta: 0 of 326

`pickSessionDay` answers **which day's PRICE BARS to draw**, and it answered right — today had a
session, so there was nothing to substitute. The gap is one layer down: **Skylit drops the expired
chain at the close**, so the ladder becomes TOMORROW's book with every rate of change at zero.
Nothing answered *which expiry's NODES to draw*. The panel was **flat, not blank** — states all fell
to HOLDING, the ROC column read zeros, rolls stopped, and v14.54's delta profile drew nothing.

**THE FIX.** During RTH the panel latches the last healthy SPXW reading (`gpts_lastbook_v1`: %King
map, king, kingKd, and each strike's velocity object). After the close, once the live front expiry
has rolled away from the latched one, `tapeMap('SPXW')` and `velAt()` serve the latch instead. Every
consumer — ladder, states, ROC, rolls, profile — works unchanged, because they all read those two.

**⚠⚠ AND THE RECORDER IS BLIND TO IT, WHICH IS THE WHOLE RISK OF THE FEATURE.** Serving latched
numbers through the two shared readers reaches the recorder too, and a latched book written into
`data/*.json` as though it were live would poison every base rate the learning layer computes,
permanently and undetectably. That is DECISIONS D-10 exactly. So the nine recorder guards no longer
test `inReplay()` directly — they call **`recorderBlind()` = `inReplay() || showingStaleBook()`**, one
place that cannot be half-updated. Adding a tenth write path means calling `recorderBlind()`.
⚠ The DISPLAY guard in `gammaProfileHtml` deliberately still tests `inReplay()` only, or the profile
would blank itself in exactly the mode built to render it.

**FOUR CONDITIONS, ALL REQUIRED, AND ALL FAILING CLOSED.** setting on · **never during RTH** · the
latch belongs to the session being shown · the live front expiry has actually rolled.
`showingStaleBook` contains **no bare `return true`** — the only true is the rolled comparison itself,
every other exit returns false, back to the live book. That assertion exists because the first six
mutations left one path silent: turning the "no live book to compare" exit into `return true` would
have served the latch on a freshly loaded tab with an empty VEL, i.e. stale numbers during a live
session, which is the one thing this must never become.

**`tapeMapLive` / `tapeMap` SPLIT.** The original reader survives untouched as `tapeMapLive`;
`tapeMap` is a thin front door. `lastBookSave` reads the RAW reader — through the front door it would
re-latch its own output every tick and the book would never age out.

**A MODE YOU CANNOT SEE IS A MODE THAT LIES.** The footer carries `● 2026-08-27 book — frozen 15:00`,
naming the session and the clock time, with a hover that says nothing is being recorded. ⚠ The first
draft of that badge appended to `out` from inside the IRT branch — syntactically valid, silently
dead, swallowed by the catch, and invisible to `node --check`. Failure pattern #5.

⚠ **A DEGRADED BOOK IS NEVER LATCHED** (`n >= SK_MIN_STRIKES`, the same floor `skPiles` refuses
below). A thin latch would be served for hours as though it were the close, and nothing on the face
would distinguish it from a quiet day.

⚠⚠ **IT CANNOT WORK TONIGHT.** The latch is written during RTH only, and there has never been one.
`__gptsDebug.lastBook()` will report `no latch yet (it is written during RTH only)` — which is
correct behaviour, not a failure. **The first time this can engage is after tomorrow's close.**

test_lastbook.js — 38 asserts, **seven mutation-tested**: revert one recorder path to `inReplay`,
drop stale-book from `recorderBlind`, latch outside RTH, latch a degraded book, let the latch feed
itself through the front door, and serve when there is no live book to compare.
**Six existing suites broke correctly and were fixed in the same commit** rather than left red —
they stub the guard to prove the recorder stays silent, and the guard changed name.
Suite 117 green, 6 documented baseline reds. smoke clean.

**ALSO IN THIS BUILD — THE ONE-FILE DELIVERY RULE.** *"you are supposed to just give me an install
file."* His rule since 2026-08-15, and it broke because three documents disagreed:
`build-installer.py` printed "DELIVER THESE TWO FILES (primary)", `PROJECT-CONSTANTS` called that
pair "the primary delivery", and the skill said "ship ONE". I followed the banner and sent three
attachments. All six sources a load reads now carry his exact words; `test_delivery.js` (21 asserts,
4 mutation-tested) fails the build if one drifts; the builder prints the single filename to send.
The chat-history rule is stated beside it, including that it runs **last**.
⚠ `installv*.bat` was NOT gitignored — only `install-v*.bat` was — so v14.54's dash-free name would
have been swept into history by `git add -A`. That is the v13.8 failure that put 28MB in `mockups/`.

## v14.54 — the ladder re-laid to the approved mockup, and the 24px nobody could explain

Operator: *"well lets implement the fix for all of the inssues incldueing in place"* — v14.53 shipped
only the IRT half. This is the layout half, built to `mockups/mockup-ladder-v11.html`, the spec he
already approved, rather than to anything invented here.

**⚠ FIRST, THE THING THAT WAS OPEN: v14.52 IS VERIFIED.** Measured on his live panel at 16:41 CT,
`__gptsDebug.irt()` returned `inPlace:true, how:"file", err:null`, last write 2.9 minutes old on a
180s cadence. The atomic-replace diagnosis was right and the in-place write is running. That closes
the question v14.52 and v14.53 were both unable to answer.

**FOUR CHANGES, AND EACH DELETES WIDTH RATHER THAN TUNING IT.**

1. **%King is left-justified INSIDE its own bar**, the type right-justified at the tip, so the whole
   `LAD_KPCT` column is gone. Biggest single saving, and it also removes a read where the eye had to
   travel from a bar to a distant figure to learn how long the bar was.
2. **The roll lane moved LEFT of the prices**, into the gap that already existed between the level
   names and the price column. Both ends of an arrow now meet the price column's own edge, so each
   end points at the price it belongs to — and the old right-hand slot is reclaimed outright.
3. **The mirrored %King profile became the SIGNED DELTA PROFILE IN DOLLARS** — Skylit's own delta15
   hanging left off a zero line, green building and red draining, the figure immediately right.
   The mirror drew the fact the node bar already drew, only backwards. Dollars earn the space: the
   column beside it already prints percentages, and dollars make a roll checkable by eye ($4M leaves
   one strike, $4M arrives at another).
4. **The three King pills and the EM edges moved INTO the chute**, which is why it widens 44 to 66.
   That is what empties the left gutter he photographed, where `CW·CW0-1` was sitting across
   `~EH 7750`. The chute now carries every price that matters and nothing else may enter it.

Plus: the SPXW crown carries its **test counter** (absent at zero — an untested crown is the ~80%
state, and "0×" would read as weakness); the **price pill takes the tested king's colour** within 2
points, with STRETCHED still winning because two states cannot share one background; and the
**day-peak outline only draws after a 12-point giveback**, because outlining every node a point off
its high was wallpaper.

**⚠⚠ UNITS, MEASURED AND NOT ASSUMED.** The delta profile needed the King's dollar mass. Measured on
the live tape 16:52 CT: SPXW King 7690, `tapeMap.kingKd` **12680**, `velocity.cur` **−12,680,083.27**.
So `kingKd` is THOUSANDS and `cur`/`d15` are DOLLARS, and the King's mass is `kingKd × 1000`. Full
scale is 45% of it, so the bar self-scales to the instrument and a quiet day stays quiet. This is
exactly the pairing PROJECT-CONSTANTS L-F names as the most common real bug in this file, so the
measurement that settles it is written at the site.

**⚠⚠ AND THE 24px LOCKED-ITEMS COULD NOT ACCOUNT FOR.** It recorded `.g3lad scrollWidth 656 /
clientWidth 632 — 24px over its own LAD_W` as unexplained. **`LAD_ROCW` was 56 for a column that
needs 84**: the widest ROC string, `−100% −100% ▼99%`, measures 83px at 8.4px/800, so the last
column has been overflowing its declared width since v14.46 and `LAD_W` has been telling the truth
about every column except the one on the end. Found by the bounding-box audit, not by reading.
`test_ladder` now asserts `LAD_ROC + LAD_ROCW === LAD_W`, so the constant cannot lie again.

**THE TRUE WIDTH GOES 657 → 618.** Not 632 → 588, which is what the old constants would have
claimed. ⚠ **618 still does not fit a 454px panel body, and no arrangement of nine columns of 8.4px
text does — the approved mockup is itself drawn on a 544px panel.** The container scrolls rather than
clips, so nothing is ever silently dropped. The last stretch is one drag of the panel edge, and it is
his call, not something to close by deleting a column.

**THE RENDER AUDIT EARNED ITS KEEP — TWICE, AND THE SECOND TIME AGAINST MY OWN FIX.** Pairwise
bounding-box audit over 53 elements at the real fonts and worst-case strings:

- It found `roleOut "RUG"` at `[206.8, 224.6]` against a chute starting at 224 — **over the wall by
  six tenths of a pixel**, on a 74% node. Cause: the fit test charged the role for a *maximum-width*
  %King instead of measuring the actual one, so a role that fit inside its bar was pushed out.
- The clamp I wrote to fix that then put the label back **on top of its own bar**. There are three
  states, not two: it fits where it wants, it fits clamped, or **it does not fit and is not drawn**.
  `fitL` now floors at the bar's tip and returns null, and the hover still carries the role.

Neither collision was visible in a screenshot and neither would have been caught by reading.

test_ladder 84 → 108 asserts, **eleven of them mutation-tested** (narrow the state cell, divide
kingKd instead of multiplying, flip the delta bars, suppress the fallback, restore the wallpaper
peak, count king taps at the converted price, push the marker into the chute, clamp onto the bar,
restore LAD_ROCW=56 — every one fires). Suite 114 green, the 6 documented baseline reds unchanged.
`tools/smoke.js` clean.

## v14.53 — the IRT export had been silently dead every morning, and v14.52 never once ran

**MEASURED ON THE LIVE PANEL, 2026-08-27 10:27 CT**, after the operator asked whether last night's
in-place fix had deployed. It had — the tab reported v14.52 — but:

    irtDir handle : SET, "lsFlexLevels", a real FileSystemDirectoryHandle
    permission    : "prompt"          ← not "granted"
    IRT_LAST      : frozen 54 minutes stale, on an unrelated "no levels" error
    inPlace       : ABSENT            ← the write path had never been reached

**v14.52's entire change lives inside that write path, so it had never executed even once.** It was
diagnosed, shipped and reasoned about against a write that was not happening.

**TWO FAULTS, STACKED.** (1) **Chrome resets File System Access permission to `"prompt"` on every
page load** — the handle survives in IndexedDB, the grant does not, so this happens EVERY MORNING.
(2) **`requestPermission()` requires a user gesture**, so from the 180-second `irtTick` it REJECTS
rather than resolving — and the inner promise had **no `.catch`**. The rejection vanished, `IRT_LAST`
was never written, and the face kept showing a stale error that looked like a DATA problem. It was a
permissions problem wearing a data problem's clothes.

⚠ **THE FIX IS NOT TO RETRY.** `requestPermission` cannot succeed from a timer, and calling it every
three minutes buys nothing but an unhandled rejection. The panel now checks
`navigator.userActivation.isActive` FIRST, and with no gesture it refuses to attempt the call and
says so in words that name the remedy. The next real click carries it — `irtPickFolder` and the
gear's Export-now button both run inside one.

**AND IT SAYS SO ON THE FACE.** The old error was only visible inside the config drawer, which nobody
opens while trading — which is how 54 minutes went by. `feedStatusHtml` now prints **⚠ IRT needs a
click** in the footer whenever `needsGesture` is set, with the four-word fix in its hover.

⚠ **THE IN-PLACE WRITE OF v14.52 IS STILL UNVERIFIED.** This build is what finally lets it run. Once
the permission is granted, `__gptsDebug.irt()` will report `inPlace:true` and only then can we say
whether the atomic-replace diagnosis was right.

**Also measured, NOT fixed here:** the ladder is **656px wide inside a 460px panel — 196px of the
right-hand side is clipped.** `g3ldst` (12), `g3ldtap` (6) and `g3ldroc` (12) are all present in the
DOM and rendering; they are simply off-screen. The missing-columns appearance is a WIDTH problem, not
a build gap. Recorded in `session-state/LOCKED-ITEMS.md`; it needs a mockup and an overlap audit
before any render code moves.

test_irt_export.js 44 → 52 asserts, mutation-tested (removing the `.catch` fires the assertion).
Suite: 115 green, the 6 documented baseline reds unchanged.

## v14.52 — the FlexLevels CSV is written IN PLACE, not replaced

Operator-reported: *"it has problems reading from a local file unless i refresh — only after i
refresh will the lines be displayed."*

**Cause, and it was ours.** `createWritable()` defaults to `keepExistingData:false`, which Chromium
implements as an **ATOMIC REPLACE**: it writes a swap file and RENAMES IT OVER the original on
close(). The CSV contents were always correct — but the **file identity changed on every export**.
IRT's FlexLevels extension opens `FlexLevelsExport.csv` once and polls it every minute, so from our
first write onward it was polling a file that no longer received updates. Hitting refresh forced it
to re-open by path, the levels appeared, and the next export orphaned its handle again.

`keepExistingData:true` opens the EXISTING file rather than a swap, so writing at position 0 and
truncating to the new length mutates that same file and its identity survives the write.

⚠ **TRUNCATE IS NOT OPTIONAL.** Without it a shorter export leaves the tail of the previous, longer
one behind and IRT reads valid rows followed by stale ones. ⚠ **BYTES, NOT CHARACTERS** —
`truncate()` takes bytes, so the length is measured with `Blob.size`.

If a browser ever refuses the in-place path it falls back to the replacing write rather than
exporting nothing: levels that need a refresh beat no levels at all. `IRT_LAST.inPlace` records which
path ran, so the two can be told apart from `__gptsDebug.irt()`.

**Unverified against IRT itself** — the diagnosis fits the symptom exactly, but it is a hypothesis
until the operator watches the lines update tomorrow without touching refresh. The fallback to a
local HTTP server (`http://localhost:8000/FlexLevelsExport.csv` via "Remote File") remains the
option if this does not do it; GitHub raw is NOT suitable, being CDN-cached ~5 minutes against a
1-minute poll.

test_irt_export.js 37 → 44 asserts. Suite green, 6 baseline reds unchanged.

## v14.51 — the export that was losing whole sessions, and a real handoff

**⚠⚠ THE PIPELINE WAS SILENTLY LOSING DAYS.** `buildDayExport` used `dateKey || TODAY` — the
wall-clock day. Run before the open (2026-08-26 at 01:56 ET) "today" was a day with no bars in it
yet, so the file written was EMPTY and stamped with that date. Nothing ran again after the session,
so the **182 snapshots and 59 deflections that day actually recorded never reached a file**, and the
repo held a blank that was indistinguishable from "nothing happened". We spent this session quoting
statistics at each other while the pipeline feeding them was dropping sessions.

Two guards, because either alone would have failed here: with no explicit date the export now takes
**the most recent day that HAS DATA**, not the calendar day; and an empty day is **REFUSED loudly**,
by both export paths — `saveDayToFile` builds first, checks, and only then chooses repo or download,
where before it handed `TODAY` straight to `repoExportDay` and reintroduced the very bug.

**THE HANDOFF.** `session-state/latest-resume-note.md` rewritten from scratch as a BRIEFING rather
than a changelog: the standing requirement in his own words, what is shipped vs mockup-only vs
agreed-unbuilt, the settled level vocabulary and why it is settled, **what the data actually says
with n and date beside every rate**, how this operator works, and the next actions in order. The
previous note is kept as a dated snapshot.

The `gex` skill gains a **HANDOFF CONTRACT**: a save is not complete when files are committed, it is
complete when a fresh context can answer six named questions without asking the user. And the load
procedure now says to read the resume note IN FULL, first, and never to quote a rate from this
project without its n and its date.

`PROJECT-CONSTANTS.md` gains seven landmines from this session — the two drifting version strings,
multi-edit scripts that abort and write nothing, literal unicode in the source, rendering mockups
before sending them, asserting widths and not just offsets, the three unit bugs, and this export.

test_states.js 49 → 60 asserts. Suite green, 6 baseline reds unchanged.

## v14.50 — an independent review found seven defects in v14.49; all seven fixed

The operator asked for a review. An independent reviewer read only the new code and found seven real
faults, every one invisible to the 33 asserts that had just passed. Two were severe.

**1 ⚠⚠ ATTRACTING FIRED ON EVERY DOMINANT-PULL NODE, ALWAYS.** The closing-distance test converted a
past close with `ifDispScale()` — the SPX→ES BASIS (~1.002) — but `closedCandles()` returns the
UNDERLYING book's bars (SPY, ~765). So a 765 close became 767 and was compared against a chart-space
level near 7650: the historic gap was always ~6900, always larger than the current one, so "the
distance is closing" was true on every render. The ◂T branch was unreachable dead code. A state built
specifically to be falsifiable could not be falsified. Now uses `EB.scaleUsed`, the underlying→display
ratio the SPY King already used, passed in explicitly so the scale cannot be guessed again.

**2 ⚠⚠ THE STRIKE-STEP FIX RESTORED THE BUG IT WAS WRITTEN TO FIX.** `strikeStepOf` measured the gap
between whatever strikes `tapeMap` returned — but that falls back to the FEED-derived map when the DOM
tape is unreadable, and that map holds only strikes above a strength floor. Median gap of a sparse
subset is 3 or 4 on a book whose strikes are 1 apart, inflating the tap tolerances several-fold; and
with under three surviving strikes it returned 1, silently reinstating the ten-times-too-tight SPXW
behaviour. Now only a DENSE ladder (12+ strikes) may set the step, the last good reading is cached per
book, and the fallback is an honest per-book default.

**3** Zero was treated as a sign in the TURN test: `(a>0)===(b>0)` puts 0 in the negative bucket, so a
flat 5m "agreed" with a falling 15m and a flat hour was "flipped against". TURN fired on rows where
the ROC column — which guards zero correctly — showed no 60m badge at all. The two surfaces disagreed
on the same row. **4** A dead ternary on `rollsCtx.srcTo`, never populated by either caller.
**5** `Math.round(-0.4)` is `-0`, which prints as "0" — the hover read "0%/15m draining".
**6** The STATE cell was 44px with `nowrap`; "WEAKENING" renders ~50px and printed over the ROC
numbers. The test asserted the OFFSETS were ordered, which they were — proving nothing about what is
drawn. Every column now declares a WIDTH and the test asserts `offset+width <= next offset`.
**7** The pull contest mixed units: real dollars for nodes carrying `usdK`, and `pct*1e6` — an invented
dollar figure — for those without. An 80%King node with no dollar reading scored 8e7 against a genuine
$50M node's 5e7 and won on a number nobody measured. Such a node now sits the contest out.

**AND A SEMANTIC CORRECTION FROM THE OPERATOR'S OWN CHART.** He marked three tests of a level that
held and one that failed, and said the badge should have read **1×** at the failed test — one prior
test is the fact you decide on. Counting on CONTACT would have shown 2× while that second test was
still under way: reporting a test you are currently inside. **A tap now completes when price LEAVES,
not when it arrives**, so during a test the count reads the tests that came before it. Consecutive
bars on the level remain one test, as before.

test_states.js 33 → 49 asserts, test_ladder 56 → 76, test_lifecycle rewritten for the new semantics.
Suite green, 6 baseline reds unchanged.

## v14.49 — THE LEVEL LIFECYCLE, settled

Worked out with the operator one state at a time, then checked against Skylit Academy doctrine
before building.

**THREE ORTHOGONAL FACTS, THREE PLACES TO SAY THEM.** The old engine crushed them into one word and
the collisions showed: a level receiving size while worn out could only be one or the other.

- **STATE** — the level's own condition: `BUILDING · HOLDING · TURN UP/DN · WEAKENING · SPENT`
- **MARKER** — its relationship to price: `BREAKING · DEFENDING · ATTRACTING · ◂T`
- **COUNTER** — how many times price tested it, absent at zero

A row can now read **`BREAKING · WEAKENING · 3×`** — price is on it, it is failing, it was already
bleeding, and it had been tested three times. The panel could not say that at all before.

**AGAINST SKYLIT'S OWN LIFECYCLE** (FRESH → TESTED → DELIVERED → DECAYING):
- FRESH/TESTED/DELIVERED is the TAP axis and is exactly our counter — same taps, same ~80/66/33.
- Their DECAYING means "weakens with NO interaction". WEAKENING does not test for that and does not
  need to: WEAKENING with **0 taps** IS their DECAYING; with 2 taps it is worn by testing. Splitting
  the axes made the distinction finer, not coarser.
- ⚠ **SPENT IS NOT THEIR "DELIVERED"** — theirs is tap-exhaustion, ours is mass. Said explicitly in
  the hover, because two neighbouring vocabularies quietly meaning different things is how a panel
  starts lying.
- Their **HALO** fires when the multi-window rates AGREE; **TURN** fires when they DISAGREE.

**BUILDING AND SPENT NO LONGER REQUIRE A ROLL PAIRING.** The old FORMING/DOOR only fired on a paired
roll destination/source, but the pairing describes how we DETECTED mass moving — it says nothing
about what the level now IS. A level gaining from fresh flow is building; a level that evaporated
with no identifiable destination is just as empty and price passes through it identically.

**⚠ ATTRACTING REQUIRES EVIDENCE, NOT POTENTIAL.** Pull = size ÷ distance is a property of the
geometry: a node can hold the most pull all session while price walks away from it, and the old ◂T
would have sat there looking authoritative the whole time. ATTRACTING additionally requires the
DISTANCE TO BE CLOSING, which makes it falsifiable. ◂T survives for dominant-pull-without-evidence,
and the two are now different claims.

**⚠ BREAKING IS THE LEVEL FAILING, NOT PRICE BREAKING THROUGH.** It means the node is being ABANDONED
while price tests it. Whether price then passes is a separate question this panel does not answer —
Beach Ball doctrine is explicit that an overshoot is not confirmation.

**TWO TAP-COUNTING BUGS, both found by checking the code against the operator's own rule:**
1. **The tolerances were SPY units applied to SPXW strikes.** `TAP_TOL`/`TAP_AWAY` were written when
   only SPY was tracked, where strikes sit 1 point apart — so 0.20 and 0.60 MEANT 20% and 60% of a
   strike gap. On SPXW, whose strikes are 5 apart, they became 4% and 12%: ten times too tight. The
   consequence was specific — a bounce that came within a point of the King and turned away, a
   textbook test, never registered, so a defended King read as untested. Now scaled by the book's own
   measured strike step (median gap, immune to a missing strike).
2. **The re-arm was tick-based while the touch was bar-based.** A single tick past the away-distance
   re-armed the counter, so choppy sessions inflated it. A whole CLOSED BAR must now be clear.

⚠ Every threshold here is HAND-SET, not measured, and labelled as such in the source.

New `test_states.js` (33 asserts). test_garma_v2 rewritten where it asserted the retired vocabulary —
it keeps the doctrine, test_states owns the mechanics. Suite green, 6 baseline reds unchanged.

## v14.48 — THE THREE KINGS, and the expected move as pills

**All three crowns on one scale**, as pills in the price column: SPXW's, SPY's and QQQ's, each with a
crown to its left, the ES price, and a small tag saying which book it belongs to. Kings landing on
the same line are NUDGED apart rather than stacked — two crowns on one row is the dual-king confusion
the operator caught in the profile, and the point of drawing three is being able to tell them apart.

⚠⚠ **THE TWO CONVERSIONS ARE NOT THE SAME KIND OF THING, AND THE PANEL MUST NOT PRETEND THEY ARE.**
- **SPXW → ES is a BASIS.** ES is a future ON the index SPXW prices; the ratio is real, live and
  self-correcting (ifLadder's dispScale, anchored on Skylit's own spot).
- **SPY → ES** is that same basis one step removed — what the SPY King flag has always used.
- **QQQ → ES is NEITHER.** QQQ tracks the Nasdaq-100 and ES tracks the S&P 500: different indices,
  no basis between them. The only honest mapping is PROPORTIONAL — "if QQQ travelled from here to
  its King and ES moved the same percentage, ES would be here" — and that assumes a correlation of
  one, which is false on exactly the days it matters most, a tech-led move. It is drawn because the
  operator asked to see all three against price, and it is marked three ways: a TILDE on the price, a
  DASHED pill so the eye is told before the hover is read, and a hover that says plainly it is **a
  bearing, never a level**. The `kind` field carries `basis` vs `proportional` in the data itself so
  a later version cannot quietly promote it.

**The expected move is now a pill** at each extreme of the price column, and the old rail's over/under
behaviour is preserved exactly: when price runs PAST a boundary the frame grows to hold it and the
boundary stays drawn where it always was — the expected move is a PRICED level, never redefined as
wherever the drawing happens to end. When that happens the rail end is marked separately, so "the
band ended here" and "the drawing ends here" stay distinct. The 0.80-sigma caveat and its 1.25
conversion survive the move.

The SPY King leaves the ladder's levels column, since it is a king pill now rather than a level name.

test_ladder.js 56 -> 73 asserts. Suite green, 6 baseline reds unchanged.

## v14.47 — the two bugs the ladder's first live render found

Both invisible to a 48-assert suite, both obvious the second the panel drew itself.

**⚠ THE LADDER WAS CLIPPING TWO COLUMNS.** It was laid out at 646px inside a 486px body with
`overflow:hidden`, so STATE and ROC were simply gone — live data absent from the face with nothing
to say it was missing. Silently dropping data is the worst failure this panel can have. Columns are
compressed to 520 and the container now SCROLLS rather than clips, so a narrow panel costs a
scrollbar instead of costing information. (The panel is resizable; widening it shows all of it.)

**⚠ EVERY DAY-PEAK OUTLINE WAS MAXED OUT.** `PEAK.m[k]` stores `|velocity.cur|` — the space the
Level Engine divides in (`|vv.cur| / pk`). The ladder divided it by `P.usdK`, which is THOUSANDS of
dollars: the ratio came out ~1000x, clamped to 100, and drew a full-width outline behind every
single node. The "what a level HAD versus what it holds" signal was therefore meaningless on all of
them. Peak-as-%King is now today's %King scaled by peak/now, both in `|cur|` space, and a test pins
that it is never divided by `usdK` again.

test_ladder.js 48 -> 56 asserts. Suite green, 6 baseline reds unchanged.

## v14.46 — THE LADDER (and the version string that had been lying since v14.40)

**⚠ GPTS_VERSION WAS STUCK AT 14.39.** It is a separate constant from the `@version` header and its
own comment calls it "THE ONE VERSION STRING — header, footer, export, logs all read this". From
v14.40 to v14.45 only the header was bumped, so the panel footer told the operator it was running
v14.39 through six installs — every one of which had in fact landed. The existing tests pinned the
header, and pinned that the footer USES GPTS_VERSION, but never that the two AGREE; that is the crack
this fell through, and `test_ladder` now asserts equality.

**THE LADDER.** The horizontal rail, the gamma profile and the percentages section, rotated 90° onto
ONE vertical price axis. Ten operator-reviewed drafts; spec is `mockups/mockup-ladder-v10.html`.

Why: the operator caught it as "there are more flags than strikes" — the rail drew 15 posts and could
label only 9, because two neighbouring strikes sit ~23px apart while a two-line label is ~22px wide.
Vertically those strikes are ~15px apart and a label is ~9px TALL. The crowding does not get managed,
it stops existing, and every node now carries its price, size, type, state and rate of change.

Columns, the operator's order, each fact with exactly one home (7692 used to appear three times):
`levels · price · nodes ▸ | NOW | ◂ profile · rolls · state · roc`. Nodes reach INWARD toward price
and the profile mirrors them, so bar length reads as how far a level reaches toward price.

⚠ **THE CHUTE IS PRICE'S ALONE** — a walled column no other element may enter, which is what makes
"price is overlapped" structurally impossible rather than merely unlikely. test_ladder asserts the
arithmetic: a 100% bar stops before it, the %King label stops before it, the profile stops before it.

⚠ **A TEST CAUGHT A LAYOUT BUG NO SCREENSHOT WOULD HAVE EXPLAINED.** The first column layout gave the
level/price connector a width of MINUS four pixels. Column offsets are now named constants with
asserted arithmetic between them.

ROLL ARROWS to the operator's sketch: filled circle at the source, OUT to the right, along the ladder,
back IN past the origin's own x, and only then the arrowhead. The landing is a SEPARATE SOLID
sub-path — a dashed stroke ends wherever the pattern falls, and a final gap at the head left it
floating. The amount sits at the ORIGIN, so two rolls into one destination cannot stack their labels.

Carried over intact: the expected-move band with its 0.80-sigma caveat, the day's range, the open,
the stretched pill, off-frame disclosure, the destination (shared from the READ via RAILTGT — one
computation, never a second opinion), dark-pool lifecycle, level-to-node row snapping, and a hover on
every element including the column headers.

**REVERSIBLE BY ONE CLASS.** The old rail and profile are HIDDEN, not deleted, for one release:
`CFG.ladder=false` (or removing `g3ladon`) restores the previous surface instantly, so the two can be
compared live and a fault is a toggle rather than a rebuild.

New `test_ladder.js` (48 asserts). Suite green, 6 baseline reds unchanged.

## v14.45 — GARMA V2 PHASE B: ROLLING STRUCTURE (a four-month refusal, unblocked)

v10.51 built FCHIST to sample the dominant floor and ceiling every closed bar and then DELIBERATELY
computed nothing from it, on the stated belief that "Flr/Ceil ROLLING is a DAY-OVER-DAY measurement"
and that "a single intraday session cannot produce a rolling verdict". Months of samples have been
banked under that refusal — 7 sessions on the operator's machine right now.

GARMA V2 (GM-ROLL-001/2/3) states the official rule precisely: rolling is migration of the dominant
node ACROSS MAP UPDATES — one print is noise, two consistent migrations are signal, three are
confirmation. Map updates happen all session. The day-over-day reading was one valid sampling of
that rule, never the only one. So the refusal lifts, and the banked data becomes usable.

**INTRADAY ROLLING**, bucketed. FCHIST holds one row per 3-minute bar and consecutive 3-minute
samples are far too fine — a floor that ticks one strike and back would read as a migration every
few bars. Samples are therefore bucketed and the run counted across BUCKETS, each bucket taking the
LAST sample in it (the map as it stood at the end of that window).

⚠ **THE BUCKET IS 15 MINUTES, AND A TEST CHOSE THE NUMBER.** One migration per bucket means the
bucket size sets how long the reading takes to EXIST: the first draft used 30 minutes, and the
regression fixture showed that meant 90 minutes before the panel could say "rolling" and two hours
before it could confirm — most of a daytrading session spent silent. At 15 minutes a signal is 45
minutes old and a confirmation an hour. Not lower: below ~10m the 3-minute sampling noise starts
producing runs on its own, which is the exact failure v10.51 refused to risk.

**IT REACHES THE FACE.** The S&R clause now answers the operator's standing requirement directly —
"the floor is ROLLING UP 3 sessions — support migrating higher (confirmed)"; "the ceiling is ROLLING
DOWN 2 buckets — upside compressing (signal)". That IS new support and resistance forming, in his
own words, and the panel has never said it despite measuring it since v10.51.

⚠ **GM-ROLL-003 IS A TARGET RULE, NOT AN ENTRY RULE.** A ceiling rolling UP expands where price MAY
go and is explicitly NOT a reason to be long: it reads "more room above; a wider target, not a
reason to be long", and casts no bullish vote. Tested.

Same count rule as everywhere (2 signal, 3 confirm) — no second threshold vocabulary. Intraday means
TODAY; yesterday is never stitched in. `__gptsDebug.rolling()` added. test_garma_v2.js 81 -> 102
asserts. Suite green, 6 baseline reds unchanged.

## v14.44 — GARMA V2 PHASE A2: THE DARK-POOL LIFECYCLE

fresh / holding / retesting / broken / reclaimed / flipped / unknown, computed from bars we actually
watched. Built only after the first live capture proved the payload shape — designing this against a
guessed payload was the thing worth waiting one version for.

**The axis is the operator's own** ("it sounds like it hasn't broken"): crossed or not. Not-crossed
splits on whether price ever came to test it, because an untested level is the STRONG one (Academy
~80% first tap) and a level defended four times is nearly spent — the same tap doctrine v14.41 wired
into the gamma levels, applied here.

- **FRESH** never touched in our window · **HOLDING** tested and rejected, never closed through
  (tap count = wear) · **RETESTING** price is on it now, outranks everything · **BROKEN** closed
  through and still on the far side · **RECLAIMED** closed through then closed back — the break
  failed · **FLIPPED** broke, then tested from the NEW side and held (support became resistance).

⚠ **BROKEN NEEDS A CLOSE, NOT A WICK.** One spike through a $1.8B print is not a break, and wicks
would retire every level on its first volatile hour. A bar is a decision — the standard the price
pill and the deflection tests already use. There is a regression test for exactly this.

⚠ **THE WHOLE MACHINE RUNS IN THE UNDERLYING'S SCALE.** Prints arrive as SPY prices and
`futRawCandles` returns SPY bars, so the comparison is like-for-like and NO RATIO ENTERS THE STATE
MACHINE. The conversion to ES happens once, at draw time. A scale inside a state machine is a bug
waiting for a volatile day.

⚠ **WE ONLY CLAIM WHAT WE WATCHED.** Their lookback is 45 days; our raw window is the chart's. A
print older than our earliest bar has a gap we did not see, and the hover DISCLOSES it ("judged only
on bars we actually watched, since 2026-08-21") rather than papering over it. No bars after the
print at all = UNKNOWN, full stop.

The state rides the level's NAME on the levels line ("DP held", "DP broke"), the hover carries the
full reasoning, and the S&R clause names it too — because "on the dark pool" and "on a dark pool
that already broke" are opposite pieces of advice. GM-DP-003 is honoured: BROKEN is not yet
resistance; FLIPPED is.

`__gptsDebug.dp()` now returns the lifecycle. test_garma_v2.js 63 -> 81 asserts. Suite green,
6 baseline reds unchanged.

## v14.43 — WHAT THE FIRST DARK-POOL CAPTURE CAUGHT

The capture worked on the operator's first reload, and the live payload immediately found two bugs
that no amount of designing against a guessed shape would have found.

**Their `ts` is in SECONDS.** The real payload is a bare array of
`{price, notional, size, ts:1784147283}`, and that decodes to 2026-07-15 — 42 days back, exactly
inside the 45-day lookback. Read as milliseconds it lands in 1970 and the hover would have read
"Printed 20000d ago". Converted at the parse, so nothing downstream ever has to know which unit it
got; a millisecond timestamp still survives untouched in case they change it.

**A CLAMPED POSITION IS A FALSE POSITION.** `emPosRail` clamps to 0..100 by design — it protects the
rail's own drawing — so a level outside the frame lands ON THE EDGE. The three captured dark pools
sit 115-220 pts below the rail, so all three pinned at 0% and merged with the SPY King (also
off-frame) into a single stack reading **"SPY K·DP·DP·DP 7632"**: four levels named, one price
shown, and that price belonging to none of the dark pools. This was a latent bug in the levels line
from v14.40 — the SPY King has been capable of pinning to the edge since it moved there — and the
dark pools are simply what made it visible.

A level drawn where it is not is worse than a level not drawn. Off-frame levels now LEAVE THE LINE
and are DISCLOSED instead: the count and the nearest few ride the bar's hover ("3 levels OFF THIS
FRAME, not drawn because a clamped position would be a false one: DP 7577 (-115)…"), so the
information is kept without faking a position. An all-off-frame set still draws the bar and explains
itself rather than vanishing.

test_garma_v2.js 52 -> 63 asserts, including the verbatim live payload as a fixture. Suite green,
6 baseline reds unchanged.

⚠ PROCESS NOTE, THIRD TIME THIS PROJECT HAS PAID FOR IT: a multi-replace script that `sys.exit()`s
on a failed assert writes NOTHING, silently discarding the edits that succeeded before it. v14.43
was briefly in a state where the render function referenced `offTxt` that no longer existed — a
ReferenceError that `node -c` cannot see, because it is scope, not syntax. Edits are now applied and
VERIFIED one at a time, and a scope check runs over the render function before packaging.

## v14.42 — GARMA V2 PHASE A1: THE DARK POOLS ARRIVE

The most recurrent concept in the entire corpus — **11 videos out of 11**, tied with the King node
and the rug — and the one with nothing built. Scouted live: Atlas has its own **Dark Pool**
indicator, the operator runs it ON, and it is fed by
`GET /fs/api/dark-pool/top-prints?ticker=SPY&top_n=3&lookback_days=45`.

**Their definition, not ours.** A Skylit dark-pool level is the TOP N PRINTS OVER A 45-DAY LOOKBACK
— a standing level set, not an intraday tape. We store their numbers verbatim and do no clustering
of our own, the same rule that governs every Skylit-sourced figure here.

**The prints are CASH-EQUITY prints, and that is why `ticker=SPY`.** SPX is an index; it has no dark
pool prints at all. SPY and QQQ are the only books that can carry them, and they reach the ES rail
through the same `EB.scaleUsed` the SPY King flag already uses.

**Capture is passive, by necessity.** A cold re-fetch of our own returns 401 ("Provide a valid API
key") because the page holds the auth — so we read the page's OWN response, in both the fetch and
XHR hooks, exactly as with every other feed. No companion courier, no CSP problem, and no credential
ever passes through our hands. The endpoint fires on chart mount rather than on a timer, so the
store is PERSISTED — otherwise a quiet afternoon would silently empty the levels.

On the face: dark pools hang from THE LEVELS LINE in their own colour (teal — SPXW yellow/purple,
SPY lighter, IF italic red/green, confluence blue, SPY King purple, dark pools teal), with the
print's size and age in the hover, and the S&R clause can now say "on the dark pool 7688" the way it
says "on the IB low".

⚠ **NO LIFECYCLE STATE IS CLAIMED.** fresh / holding / broken / retesting / flipped / reclaimed is
phase A2, and it is deliberately not built yet: the payload shape is still unknown (401 on cold
fetch, mount-driven), so the parser is tolerant BY DESIGN — it accepts the array bare, under `data`,
`prints`, `top_prints`, `levels`, or one level nested, reads price/size/notional/time from any of
the obvious field names, and KEEPS A RAW SAMPLE (`__gptsDebug.dp()`) so the real shape can be read
off a live capture. An unrecognised shape fails LOUDLY into `DP_STATE.err` rather than quietly
producing zero levels that would look exactly like "no dark pools today". Designing a lifecycle
against a guessed payload is how you ship a state machine that silently never advances.

test_garma_v2.js 28 -> 52 asserts. Suite green, 6 baseline reds unchanged.

## v14.41 — GARMA V2 PHASE 0: the corrections, and three refusals

The V2 package (59 rules, up from 42; `garma/claude_package_v2`) says v1 underweighted the named
patterns and never separated official doctrine from Garma shorthand. Phase 0 closes the corrections
to what ALREADY ships — one new thing appears on the face, and one phase deliberately builds nothing.

**GM-MAP-004 — the air pocket is a PATHWAY, never a destination.** Structurally we were already
safe: the destination is picked from RAILPS by measured pull, and RAILPS holds nodes, so a gap can
never be selected as one. That guarantee is now ASSERTED rather than assumed. And the pocket finally
becomes visible again (its standalone line was retired at v10.50) in the place doctrine puts it —
the PATH clause: "the path up is CLEAR, through an AIR POCKET 7692–7706 (thin exposure — a pathway,
not a target)". `railPockets()` computes it on the rail's own display scale from the Academy's
verbatim thresholds (2.5x median spacing = pocket, 4.0x = LIQUIDITY VACUUM, absolute floor honoured).

**GM-MAP-007 — a used level is a weaker level.** `updateTaps()` has counted distinct taps per SPXW
strike since v11.84 and the Level Engine never asked. It asks now: 1 tap and 2 taps annotate the
state with the Academy's own odds for the NEXT test (~66%, ~33%), and a THIRD tap earns its own
state — **USED**, the graveyard, dimmest word on the rail and the only state that fades its own post.
Ranked BELOW the live states on purpose: a level receiving a roll reads FORMING even when tapped
out, because fresh size arriving is the newer fact. ⚠ TAP_PROB is indexed by taps ALREADY TAKEN, not
by which tap this is — a test pins that, because getting it backwards overstates every used level.

**GM-EVENT-001 / GM-REG-002 — event mode DOWNGRADES, it never suspends.** V2 explicitly rejects the
idea that one FOMC lotto example licenses special event logic. The clause now reads "confidence
capped, normal rules NOT suspended", and tests assert the King / S&R / Destination clauses still
compose on an event day and that nothing short-circuits the read.

**GM-TOOL-001/002 — three features deliberately NOT built.** Dim is display provenance, Gamma VWAP
is optional experimental confluence, Falcon is not defined in official doctrine at all. None may
create, upgrade or invalidate a level. Recorded in the file itself, because the risk is a future
version quietly wiring one into a verdict on the strength of a video mention.

New: `test_garma_v2.js` (28 asserts). Suite green, 6 baseline reds unchanged.

## v14.40 — THE LEVELS LINE (operator sketch) + the IF levels leave the main rail

The v14.39 chip strip is replaced wholesale — its FORM was rejected ("the idea was to have a
line with small arrowheads... a separate rail or line above the current one and the levels
should be below it"). THE LEVELS LINE is its own rail directly under the read: the LINE runs on
top, and each level's name, price and a small arrowhead hang BELOW it, the head pointing DOWN at
the exact spot on the gamma rail where that price lives — the two share the x-frame by the roll
lane's own overlay trick (anchored left:0/right:0 inside .g3emt; g3haslvl buys the space above).
Carries session structure (30-min IB, prior day), the SPY King (purple), confluence in blue with
"·node" (Garma r22), a white price-now notch, and overlap-merged stacks (names joined, nearest
price shown, every exact price in the hover).

AND — operator-directed — the InsiderFinance levels moved OFF the main rail onto the line: CW,
PW, the 0DTE walls, FLIP and T (the magnet), in ITALICS because they are THEIR numbers (the same
voice the old panel gave them). The rail's floating T span is gone; the row-1 T chip stays and
its hover now points at the line. MP* stays off the line — the read already names max pain on an
OPEX day. Mockup: mockups/mockup-levels-line.html (v2, corrected layout).

## v14.39 — GARMA 1b (max pain on OPEX) + THE LEVELS STRIP (approved mockup)

1b, the operator\'s "simplest thing that is useful": an OPEX day\'s lead clause names the pin
target — "OPEX day — expect pinning; max pain 7594" — one clause, pulling the MP the IF chain
already computes. QUAD-split and EM-caution options ledgered, not built.

THE LEVELS STRIP (mockups/mockup-levels-strip.html, operator-approved): session structure gets
its own row under the read — chips nearest-first, \u25b2 overhead / \u25bc beneath with signed
distance, BLUE + "· node" when a gamma level sits on it (Garma r22 made unmissable), SPY K in its
purple. The rail\'s session TICKS are RETIRED in its favour ("the rail seems cluttered") — the
rail keeps only what moves; sessionLevels() and the read\'s confluence naming live on unchanged.

## v14.38 + companion v1.14 — GARMA 1a: the economic calendar (operator-approved)

The EVENT day now stamps itself. Source: ForexFactory\'s free weekly feed (no key, impact-rated),
filtered to USD + High. FOMC-family events stamp the WHOLE day; everything else is active ±90
minutes around its release. The read opens "EVENT day — CPI 8:30 — normal-rule confidence
downgraded (Garma r41)". VERIFIED LIVE FIRST: the page cannot fetch the feed (Skylit\'s CSP +
@grant-none, both load-bearing) — so the fetch rides THE COMPANION (v1.14), which is privileged
past CORS/CSP and already exists precisely for this pattern; it couriers the feed once per day
into the shared localStorage cache the tapereader\'s evCalLoad() reads. Failure is disclosed in
the companion log, and the manual event tag stays as fallback — never a silent no-events.
⚠ THIS INSTALL UPDATES BOTH SCRIPTS — the companion changes for the first time since v1.13.
test_garma_p1 +6 executed (window logic, FOMC whole-day, stale-cache refusal, USD/High filter).

## v14.37 — one crown, a centred flag, and THE LIVE AUDIT (operator: "do a better job at testing")

The dual-crown was an INTERACTION defect — the profile crowned both the LATCHED King (role) and
the tape's momentary 100% row (isKing), two features each correct alone, wrong together, visible
only in a live contested crown no unit fixture reproduced. Three responses:

1. **One crown**: ♛ = the latched role only. A challenger that has out-massed the King shows its
   honest 100% wearing ⚔ (purple) — the contest is visible without premature coronation. Trinity
   now reads the LATCHED crown on the SPXW side too, killing the clause-vs-clause wording clash.
2. **THE LIVE AUDIT** (__gptsDebug.audit): face invariants checked on the REAL DOM — exactly one
   crown, no "undefined"/"NaN" printed, the read present and opening with the day, the pill
   numeric, no export error, one SPY K flag. Run after every install; violations name themselves.
   This is the standing answer to the class of bug unit tests cannot see.
3. **SPY K flag centred** (operator-directed): name above and price below both centre on the
   dashed line instead of hanging off its left edge into neighbouring labels.

test_garma_p1 +6 (dual-crown regression, challenger mark, audit presence, Trinity-latch). The
levels strip mockup is APPROVED-PENDING the operator's Phase-1 rediscussion — not yet built.

## v14.36 — the session ticks actually draw (data-shape fix, operator's check caught it)

v14.35's sessionLevels checked a `day` field the live candles never carry, and looked for
prior-day bars in a store that is today-only by construction — so every value computed null and
zero ticks drew. The check on the live panel exposed it within minutes. Rewritten against the
REAL shapes: IB from the today-only closed store (`so` gate alone), prior-day H/L/C from the raw
fiber window (spans days, carries timestamps; RTH bars only). Tests rewritten with true-shape
stubs including a pre-RTH decoy bar that must be ignored (20 executed). Also logged for polish:
during a crown contest the Trinity clause (tape crown) and the King clause (latched crown) can
read as disagreeing — same data, two stages of the latch; wording alignment queued.

## v14.35 — GARMA item 1: session-structure levels (30-minute IB + prior day)

The context layer he maps before touching Skylit (10/11 videos). Corpus-verbatim answer to the
operator\'s question: the IB is THIRTY MINUTES ("waits for the IB high and low to be set 30
minutes after the open", video 10) — which is also Skylit\'s own IB30H/IB30L badge, so the chart
verifies our numbers for free. sessionLevels() computes IB H/L (today, first-30-min bars only,
NOT reported until the window closes — he waits, so do we) and prior-day H/L/C (from the candle
window\'s prior-session bars; absent honestly when the window rolls past them), all in the chart
frame. The rail wears them as dim structure TICKS (IBH/IBL/PDH/PDL/PDC — the skeleton, clearly
not gamma, off-frame ticks skipped rather than edge-piled). The read names CONFLUENCE (his rule
22: structure + node stacked beats either alone): "support 7678 (67% brake) FORMING — on the IB
low" (2-pt zone, his rule 29: nodes are zones). VWAP + Asia/London/Midnight DEFERRED pending a
volume/ETH data check; Fibonacci skipped by his own rule 21. test_garma_p1 now 20 executed.

## v14.34 — GARMA PHASE 1: the day, the Trinity, and the gatekeeper block

The operator supplied an evidence-based reconstruction of Garma\'s decision model (the Skylit
analyst; garma/claude_package — 42 weighted rules from 11 videos, recurrence-audited). Reviewed
against our doctrine: NO CONFLICTS — he teaches the system the Academy documents. ~60% was
already built (kings, gatekeepers, rugs, node quality = our Level states, deflection triggers,
refusal honesty). Phase 1 lands the three gaps buildable with data in hand:

1. **Day classification** (his rule 2: classify BEFORE executing): dayTypeOf → EVENT / OPEX /
   WHIPSAW (both sides ran >35% of the EM — fade edges) / TREND UP·DOWN (one-sided EM use + the
   SMA machine agreeing) / RANGE. Leads the read.
2. **Trinity conviction** (rules 10-12): trinityRead — which side of ITS OWN spot each book\'s
   King sits (frame-free); 3-of-3 = aligned, 2-of-3 = "reduced conviction", <2 = "WAIT per
   doctrine", the dissenting book NAMED ("QQQ dissents").
3. **The gatekeeper block** (rule 5): a BRAKED path whose stalling gatekeeper (>=1.8×) sits
   between price and the destination now says so: "target BLOCKED by uncleared gatekeeper 7684 —
   do not target through it."
Day type + Trinity ride the levelstate record so the nightly review conditions every state\'s
hold-rate on them. test_garma_p1: 14 executed asserts. Phases 2-5 queued (session levels /
dark pools / SMH-NVDA / conviction composer).

## v14.33 — the "check everything" pass: two blemishes

The operator's full-system check verified the two-section face, the states voice, the ST row, the
door ghosts and the king-class arrows all correct — and caught two leftovers: (1) the "Node Map —
waiting on node data…" line had survived the NODES retirement (accumBlock now gates on
LOC_SHOW_NODES; nodeMapModel/recordDeflections keep running for Analysis + features), and (2) the
rail label state words used the full word (WEAKENING) where the profile uses four letters — dense
clusters mashed again; the rail now speaks WEAK/FORM/TURN/DOOR like its sibling.

## v14.32 — the face slims to TREND + LOCATION (operator-directed) + the consistency review

Operator: "step back and review the entire feature… i don't think i need the nodes section below,
so it can be removed along with the Reaction and Execute sections."

**The review's verdict**: with the Level Engine live, the NODES list, ③ REACTION and ④ EXECUTE
had become restatements — the rail + profile + read now carry roles, states, ROC, rolls, walls
and the defence verdicts (the READ's reaction clauses use the SAME reactDefence the REACTION
section displayed; one computation, no drift possible). The one thing the retired sections had
that the face keeps needing — the in-play defence verdict — lives in the read and the effects.

**The removals, done the safe way**: the NODES list is HIDDEN, NOT REMOVED (LOC_SHOW_NODES=false,
the LOC_SHOW_CHART precedent) — every computation behind it still runs because the read, rail,
profile and enrolled features drink from the same wells. REACTION/EXECUTE stop rendering
(secs=[secBias,secLoc]); secReact/secExec survive as functions for recorders/debug, marked for
proper deletion after a clean week. The step bar shows the two real steps.

test_layout_v13 updated for the two-step face (32 pass). Suite at the baseline six.

## v14.31 — the state row lands where it belongs (operator-caught layout)

The v14.30 state row was appended at a lexical point still INSIDE the bar track's container, so
the state words floated over the bars' tops instead of sitting under the ROC matrix. It is now a
proper fourth matrix row (capped "ST"), built where the data is and appended after the 60M row —
5M / 15M / 60M are the evidence, ST is the conclusion, in that order on screen.

## v14.30 — THE LEVEL ENGINE (operator's business requirement, made structural)

"I need to know potential support and resistance, especially if it is weakening and new support
and resistance is forming, as well as where price is going." One build, five pieces:

1. **Five states per rail level**, computed from what is already measured (levelStateOf):
   FORMING (roll destination, still building — the 74%-held shelf pattern caught early),
   WEAKENING (own-mass draining, or a roll source), TURNING (5m+15m against the hour — earliest
   hand-change warning), DOOR (drained source <50% of its own day peak — 19/19 pass-throughs),
   HOLDING (else). Thresholds ⚖ hand-set and enrolled for calibration.
2. **The face wears them**: state-coloured post outlines, a state word under the label (silent
   when HOLDING), DOOR ghosts drawn where vacated posts used to stand (with the stop-placement
   warning in the hover), and a STATE ROW under the profile's ROC matrix — the three percentage
   rows are the evidence, the word beneath is the conclusion.
3. **The read trades in states**: support/resistance clauses lead with the state + its why; a
   DOOR in reach gets its own clause; the destination adds CONCENTRATION (% of the near book at
   the target) and CROWN MARGIN (100 − successor%, flagged UNSTABLE past the 60% succession
   threshold that ran 76% within 20 bars, n=148).
4. **Two-tier arrows**: KING-CLASS (crown involved) draw bold — the succession chain is the roll
   that moves the destination itself; field arrows under $5M are latched but not drawn; pairing
   quality (% of shed mass the destination accounts for) rides every hover — low pairing is
   evaporation in a roll costume.
5. **Tracked from day one** (operator: "i want it tracked"): the `levelstate` feature records
   every level's state per bar + the nearest actionable level, outcomes score hold/break against
   the forward window, three questions enrolled (forming_holds / weak_leads / door_breaks),
   rules.json seeded (73 rules).

## v14.29 — the read speaks ES, the flag prices itself, the arrows stop lying about motion

Three operator catches in one look (while still on v14.25 — the reminder that a build not
TM-updated is a build not shipped):
1. **Every number in the read is ES now** — "KING 7670" was the SPXW strike; the King and any
   challenger print at their chart price, the strike stays on the rail labels and hovers.
2. **The SPY K flag carries its price BELOW the line**, ES-mapped, with the name alone on top —
   neither end collides with the rail labels.
3. **A moving arrow now means LIVE, full stop.** The animation keyed off (conf && !live), so a
   signal-level roll (2 bars) that had ALREADY stopped flowing still animated — motion asserted
   where none existed. Dashes = mass moving this window; solid = latched structure, whatever the
   count. The grammar the lane always claimed, now enforced.

## v14.28 — the third pillar is THE DESTINATION (operator-rephrased)

"King, Support and Resistance, Destination." The direction-lean of v14.27 becomes the doctrine\'s
own question — WHERE is the flow pulling price: the dominant magnet by measured pull
(size/distance, the attract metric recorded since v13.4), gated at the tractor\'s own 2\u00d7
dominance ("no dominant magnet — the field is contested" when it fails), the path to it priced by
the same sums the target uses (CLEAR / FUELLED / BRAKED, with what the words mean spelled out),
and rolls feeding the destination named with their size. The hover trimmed under the FRAME
600-char guard (it caught the first draft at 642). A destination is where the flow points, not a
promise.

## v14.27 — the read is KING-FOCUSED, on three things (operator-directed)

"The King, support and resistance, and direction." The flow read restructures around exactly
those pillars: (1) THE KING — held or contested (latch seconds), its polarity, and where its
magnet sits against price; (2) SUPPORT AND RESISTANCE — the nearest meaningful level each side
with its live build/drain (>$1M/15m) and any defence verdict when price is on it; the SPY KING
competes as a level on its own side (the operator\'s bounce lesson made structural); (3)
DIRECTION — a measured LEAN summed from countable votes (the King\'s pull, the latched roll bias,
build/drain geometry per side, defence verdicts), with the dissenting votes NAMED ("leaning UP —
resistance draining, support building; against it, the King pulls from below") and MIXED when the
votes cancel. The hover says a lean is context, not a signal. Same shared arrays throughout.

## v14.26 — two flow-read blemishes from the first live sentence

Sentence capitalization (each clause now capitalizes), and the roll-intent branch keyed off the
wrong direction token — r.dir's down value is not the literal 'down', so the first live roll-down
(7680→7655, $20M, destination UNDER price) read "lifting support toward price" instead of
"planting support under price". Intent now keys off the same test as the printed word.

## v14.25 — the flow read speaks in sentences (operator: "more reader friendly and coherent")

The v14.24 fragments become narrated prose — what is happening AND what the flow is trying to do,
every clause still strictly measured: "Price is on 7694 (an 80% accelerator) and the node is being
ABANDONED — a break is forming. The crown is being CONTESTED — 7675 has out-massed King 7655 for
80 of the 120s it needs. Dealers are rolling DOWN, 7685→7665 ($6M in flight) — planting support
under price. 7670 is the fastest BUILD (+$12M/15m) — support thickening below. The SPY King waits
overhead at 7682." Intent phrasing is the doctrine's, keyed to measured geometry: a roll-down whose
destination sits UNDER price is planting support (the 2026-08-25 lesson); a build below price is
support thickening; a drain above is the ceiling thinning. The line wraps as prose now (no
ellipsis). Same shared inputs, no second derivation.

## v14.24 — the roll's source dot and THE FLOW READ

1. **Source circle** (operator-directed): every roll arrow now anchors BOTH ends — a small circle
   at the rise to match the destination's arrowhead, in the roll's colour.
2. **THE FLOW READ** (operator: "a read area above the arrows analyzing the tape — the flow as
   described by the arrows, rolls, king movement and more, basically all what you have learned"):
   one line above the rail, measured facts only, priority-ordered, fragments present only when
   their fact exists: ⚡ the in-play node with its reactDefence verdict · the crown (held, or
   CONTESTED with the challenger's seconds on the latch clock) · the freshest latched roll (dir,
   from→to, in flight/stuck, $) · the strongest 15-minute flow move on the rail (>$1M) · the SPY
   King's position when within 30 pts. Every fragment reads from the SAME shared arrays the rail
   draws (RAILPS/RAILROLLS/velAt/reactDefence/latch stores) — no second derivation (the
   one-computation rule). Hover explains the grammar. Class g3tread (g3flow* is a banned prefix —
   the v11.95 FEEDS-chip guard; and the king-projection ellipsis guard forced a property reorder).

## v14.23 — the SPY King flag, the crown, and two arrow-audit fixes

1. **THE SPY KING FLAG** (operator standing requirement: "i must know where the spy king is even
   on spxw — sometimes there are bounces based on the spy king"): a full-height DASHED line at the
   SPY crown's chart price, light purple when its crown is negative / light yellow when positive,
   tag on top clear of the SPXW labels — deliberately not a post, because it is the OTHER book's
   crown. Live from the self-fetched SPY feed; settings toggle (SPY King flag, default on).
   Proven the day it was asked for: price bounced on ES 7682 = SPY 765 in an SPXW gap.
2. **The profile King wears the CROWN** (♛), not "100%" — a percentage that is 100 by definition
   carried no information; one royal mark across the panel.
3. **Roll arrows: the source names itself** (operator: "one of the red arrows shows a blank
   source"): a completed roll EMPTIES its origin below the node threshold, so no post stood at the
   rise and the arrow came from blank track. A small source tag now draws at the origin — only
   when no post stands there, keeping the v14.0 no-lane-text rule everywhere else.
4. **The rail frame includes the LIVE print**: v14.19 made the pill live but left emRailBounds on
   closed-bar extremes, so a fast move past the rail clipped the pill in half at the edge (found
   in the arrow audit). The frame now stretches for nowLive too.
5. **Ledger #1 CLOSED, measured**: Skylit's own 5M/15M/60M velocity fields were sampled 25s apart
   and MOVED — Skylit updates them as a rolling window (~5s refresh), not per-bar. The panel
   displays those fields verbatim, so it is already consistent with Skylit; rolling stands.
Also recorded: CW/PW provenance re-confirmed for the operator — InsiderFinance 0DTE only (Skylit
carries no call/put split); IF walls update every few minutes intraday on a once-daily OI base.

## v14.22 — the cross-book candle leak (operator: "the tape reader is messed up")

The v14.21 guard covered FOREIGN charts; this was one layer deeper, between two RECOGNIZED books.
While Atlas sat on the NQ chart, refreshSym('SPY') still ran — and futRawCandles' futures
fallback served the CHART's candles divided by the CHART's ratio to ANY caller: NQ bars / 41.17 =
QQQ-SCALE prices (~708) written into the SPY book. The session pin then captured 708.49 as SPY's
open and the whole rail anchored at 7114 (measured live; healed by hand: emopen cleared, both
books flushed, correct re-capture verified at openU 764.64).

Two fixes: (1) THE PAIRING GATE — the futures fallback serves ONLY the symbol the chart pairs
with (FUT_UNDERLYING[chart]===sym); chart candles are the chart's own underlying, full stop.
(2) THE FEED ANCHOR in applyCandles — a batch that is wrong-scale THROUGHOUT defeats a
median-only sweep (the bars agree with each other), so the batch's last close is compared to the
self-fetched feed's own spot: the right book disagrees by basis noise even on a crash day; the
QQQ-on-SPY leak reads 7.4% off; >5% drops the batch whole.

test_futures_mode +2, both EXECUTED (the pairing-gate token and a planted 708-scale batch
rejected against a 765.2 feed anchor). Suite at the baseline six.

## v14.21 — the chart-flip guard lands (queued since the GLD/USO corruption path was verified)

Operator, after browsing symbols: "make sure that doesn't corrupt anything… and clear the data
also." Two halves:

**The guard**: an unrecognized chart symbol (GLD, USO, a stock — anything without a pairing in
FUT_UNDERLYING) marks FUTMODE.foreign. The single candle-ingestion gate (refreshSym) then
DISCARDS the fiber candles instead of writing the foreign instrument's bars into the SPY book
through the 'SPY' default — the verified corruption path — and the last-resort price fallback is
gated too. The face says so: "RECORDING PAUSED, foreign data discarded". Feed-side state (walls,
levels, ladders) is symbol-keyed and unaffected. The recognized set (ES/MES/NQ/MNQ/SPX/SPXW/
SPY/QQQ) flips freely, as before.

**The sweep ("clear the data")**: applyCandles now drops any bar whose close sits >15% from the
batch median — a foreign bar is on a different price SCALE entirely (GLD ~428 vs SPY ~765; a real
intraday range is <2%), so history self-cleans even if something ever slipped in. Verified live
first: today's books were CLEAN (SPY 50 bars, 764.57–766.93, zero out-of-range).

test_futures_mode +5 (foreign flag, face message, both ingestion gates, and the sweep EXECUTED:
a planted 428 bar dropped from six real ones). Suite at the baseline six.

## v14.20 — the export steps back to THE THREE KINGS (operator-directed)

"Too many levels — let's step back to only exporting the kings. SPY king, SPX king and QQQ king.
SPY and SPX convert to ES levels and QQQ converts to NQ levels." The file is now three lines:
`SPXW KING 100%` + `SPY KING 100%` on EPU26, `QQQ KING 100%` on ENQU26. The rail's node set,
SUCC, the IF walls and every percentage row leave the FILE — the panel keeps all of them on
screen, where density is cheap.

Every hard-won mechanism rides along: the locked grammar (v14.12), RGB colours (v14.14) — SPXW
full yellow/purple, SPY the lighter shade, QQQ full — the chart-frame-independent SPXW conversion
(v14.14) on the Atlas-anchored basis (v14.17/18), the LATCHED crown (v14.19 — the exported SPXW
king is the rail's, never a mid-flap blip), the SPY king from the self-fetched book (v14.19,
always fresh), the QQQ king from the rendered 0DTE ladder with the weekly fallback still rejected
(v14.15), and both ratio chains with honest '~' tags (v11.4.1, v14.13). Any king alone still
writes; all dark writes nothing.

test_irt_export rewritten to the three-line contract (37 asserts incl. latch-holds-through-flap,
per-book scale behaviour, absence of every trimmed lane); em_band §40 rewritten to execute the
kings-only build (609). Suite at the baseline six.

## v14.19 — the compiled ledger lands: six operator-driven fixes in one build

1. **Flag posts no longer bury the price labels** (#2, caught twice): .g3emt 66→86px — a post
   (top 19 + up to 44 tall) now always ends above the bottom-anchored label block.
2. **The pill is LIVE** (#4): the number and dot ride the chart's live print (futPx, else live
   underlying × the pinned scale); EB.now stays the last CLOSED bar for every recorded
   measurement, and the arrow keeps its bar meaning. Hover updated; hook surfaces nowLive.
3. **Warm-up guard** (#7a): the session pin is REFUSED until the candle window's opening bar is
   TODAY's and, on futures charts, the ratio is live — the 09:50 capture had pinned YESTERDAY's
   8:30 bar and a contaminated early EMA (+20 pts all morning until hand-cleared).
4. **The rr self-heal clock survives reloads** (#7c): gpts_emrrbad_v1 — it was in-memory and
   every install reload reset the 5-minute timer, so the v13.9 heal never fired when needed.
5. **The crown is latched** (#8a): kingLatchTick — the KING label moves only after the new top
   strike holds 120s continuously (day-keyed, persisted); the tape's crown flapped 3× in an hour.
   Integrity gates stay on the TAPE king; roles anchor on the latched one.
6. **RUG/RRUG tag the anchor only** (#8b): the ceiling carries the pattern name; the floor keeps
   its polarity role — two simultaneous RRUGs (one on a positive node) confused the read.
7. **SPY rows persist + the SPY King is guaranteed** (#9, operator: "i must always have the spy
   and spxw king"): derived rows ship up to 5 min old wearing ' ~' past the fresh window; when
   the projection is dead the KING alone comes from the self-fetched SPY book (<=36s) directly.

Tests: em_band 622 (hook parity incl nowLive, latch fixtures, height + anchor-only asserts
redesigned), irt_export 71 (persistence, age tag, guaranteed-King path). Suite at baseline six.

## v14.18 — the basis chain gains a rung that actually fires

v14.17's Skylit-spot override was correct and nearly useless: it required a fresh SPXW feed, and
the page only fetches that book every ~2 minutes (measured: 4 payloads in 10 min) — so the basis
fell back to IF's delayed spot almost every render and the rail stayed ~6 pts off Atlas. (The same
sparseness is why the export's SPY diamond rows were absent all morning — their freshness gate is
honest, the feed is just slow.) New second rung: the SELF-FETCHED SPY feed (<=12s fresh) carries
Skylit's own measured SPY<->SPXW ratio in derived[] — SPX spot = SPYspot/ratio, both factors
theirs, both live, 2% sanity vs IF. spotSrc: 'skylit' | 'skylit-spy' | 'if'.

## v14.17 — the basis anchors on Atlas's own spot (rail was ~6 pts high vs Atlas)

Operator: "compare atlas with the rails — it's different." Measured: rail King 7674, Atlas ~7668;
dispScale 1.00248 vs true live basis ~1.0017. Cause: ifLadder's SPX->chart scale divided by
INSIDERFINANCE's spot — a delayed, slow-refreshing quote — so in a moving tape every node placed
from it drifted off Atlas by the lag. Fix: when the Skylit feed is fresh, its OWN spot
(levels[last].s, ~5s cadence — the number Atlas positions its overlay with) is the denominator,
with a 2% cross-source sanity gate; IF's spot remains the disclosed fallback (spotSrc:'skylit'|
'if' on the ladder output). One function, so the band, taps, profile, export and rail all move
together — no second path. Same lesson as the QQQ-window fix: pair Skylit with Skylit.

## v14.16 — the rail syncs with Atlas: EM floor + the band clip is gone (ledger issue #5)

Operator, watching price fight the 7690 accelerator with an empty rail: "the heavy vol nodes
aren't even shown on the rail… the rail is not in sync… the band is a measuring stick, not an
admission filter." Root cause, measured live: the once-per-session EM capture fired just past
midnight while the IF dte0 chain still held the EXPIRED Aug-26 book — an ATM straddle on expired
options is ~$2.5 of residue, and that was PINNED as the whole day's expected move (EL 7688/EH
7693, FIVE ES points). skPiles then clipped every pile to that band: King and 8 of 9 nodes >=20%K
deleted from the rail, the NODES section, the ROC matrix, the in-play effects AND the export,
while Atlas drew all of them. The rr poisoned-pin lesson (v13.9), one field over.

Three changes:
1. **EM sanity floor** (EM_MIN_FRAC=0.001 of the anchor, ⚖ hand-set): a stored EM below the floor
   is discarded (out.emHealed) and an implausible fresh straddle is REFUSED at capture — no band,
   disclosed, beats a 5-pt band silently governing the day. EMOPEN_SCHEMA 3→4 purges any pre-floor
   pin on install.
2. **The band clip is GONE** from skPiles AND emPilesIF: every node >= nodeThresh ships regardless
   of band width. Roles were already whole-ladder (v11.81); emPath still range-filters its own
   sums; emPos stays clamped (recorded measurement, untouched).
3. **The rail frame holds every node**: emRailBounds takes the pile list and widens to the
   outermost pile + half-pad, so un-clipped nodes are drawable instead of stacking at 0%/100%.
   over/under stay PRICE-driven — a far King must not fake "price ran past the band". RAILPS now
   computes before RB; one RB serves rail + profile.

test_em_band: clip asserts REVERSED (absence asserted both sources), schema/floor asserts added
(622). Suite at the six known pre-v13.8 reds.

## v14.15 — NQ rows come from the Atlas QQQ ladder, not the weekly feed

Operator, comparing his IRT NQ chart against Atlas on QQQ: "it has less levels than my nq chart —
something is wrong… use atlas as the source of truth." He was right about the mismatch and about
the standard: v14.12–v14.14 sourced the NQ rows from LASTFEED.QQQ, the WEEKLY window (12+ strikes
>=20%), while Atlas draws the FRONT/0DTE book (a handful). Same data family, different question —
and the chart must ask Atlas's question.

NQ source is now `tapeMap('QQQ')` — the rendered QQQ ladder, front expiry, the exact strikes,
%King and polarity signs Atlas shows — with the gamma-feed fallback REJECTED (`fromFeed` ⇒ absent),
because that fallback IS the weekly book and would silently change windows. Polarity now rides the
tape's signed pct (negative = accelerator purple). Consequences, by design: the NQ rows expire with
the 0DTE book exactly like the SPXW rail (the weekly rows' midnight survival was the tell), and an
unreadable ladder writes NOTHING — absent, never weekly. Tests: QQQ stub moved to a sym-aware
tapeMap, feed-fallback-rejection asserted (66).

## v14.14 — two operator-caught defects: byte-swapped colours, chart-frame prices

**"Why is it blue?" — IRT reads PENCOLOR as plain RGB, not Windows COLORREF.** The King's yellow
(227,195,65), written BGR as 0x41C3E3, rendered sky blue (65,195,227) on the operator's chart.
Every colour since v11.4 (the first FlexLevels build) was byte-swapped; nobody had questioned a hue
until now. irtColor is now (r<<16)+(g<<8)+b, token-asserted.

**"I see SPXW KING at 7655 on the ES — is that right?" — no.** The rail rows' conversion rode on
`dispScale`, which maps SPX -> the CHARTED symbol's frame: perfect on an ES chart (basis included),
but ~1.0 on the SPXW CASH chart — so a file written while Atlas showed SPXW carried raw SPX prices
on the EPU26 symbol, ~15 pts low (ES 7691 / SPX 7677 that moment). Fix, rail + SPY-derived blocks:
prefer dispScale only when FUTMODE says an ES chart is live; otherwise spxK x undScale (SPX->SPY,
chart-free) x the persisted live ES basis. The export price no longer depends on what Atlas shows.
(+4 asserts, 65; em_band fixture pinned to the ES-chart path.)

## v14.13 — the NQ ratio measures itself (operator: "do a proper ratio — use Skylit's conversion")

The guessed 41.9 default had the NQ lines ~480 points off-screen (ENQU26 29,233 ÷ QQQ 709.19 =
41.22, measured the moment the operator reported no levels). The real fix: the panel's futures
machinery ALREADY pairs an NQ chart with the QQQ feed (FUT_FAMILY NQ→'NQ', underlying QQQ) and
EMAs the live basis futPx/undPx — the same conversion Skylit's own overlay uses to place QQQ nodes
on an NQ chart. Verified live on NQ1: FUTMODE {fam:'NQ', r:41.1911, live:true, futPx:29270.25,
undPx:710.505}. An NQ chart is a RECOGNIZED pairing — it records into the QQQ book, so the
chart-flip corruption risk (GLD/USO) does not apply.

**`irtNqRatio()` — the ES chain, mirrored:** live FUTMODE (persisted to gpts_irt_nqratio_v1) →
last-good ≤14 days → the settings' manual number → NQ_RATIO const. `~` on NQ labels now means
"not measured live" (it is no longer permanent — visit the NQ chart on Atlas and the ratio
self-captures, exactly like ES). Settings ratio field re-titled: FALLBACK ONLY. Live store seeded
with the measured 41.196 so the fix works before the first future NQ visit. +5 asserts (61).

## v14.12 — the export speaks one grammar, gains NQ, and stops forgetting it was on

Operator-locked spec (2026-08-26): **one label grammar for every line — SOURCE + ROLE + STRENGTH.**
`SPXW KING 100%` / `SPXW GK 36%` / `SPXW SUCC` (no % — its meaning is "next King", not size) ·
`SPY KING 100%` (the 100% row is arithmetic) / `SPY 51%` (any other role would be a guess — the
rail machine is SPXW-only) · `IF CW0 / PW0 / MAG0 / MP0` (renamed from CR/PS/Mag/MP to the panel's
own vocabulary; the 0 = the 0DTE window, leaving room for un-suffixed weekly rows later). Row
order: SPXW block → SPY block → IF block. Colours (operator's scheme): SPXW yellow(+γ)/purple(−γ);
SPY the SAME language LIGHTER; IF CW0 red / PW0 green / MAG0+MP0 NEUTRAL grey-blue (a magnet is
not a wall — red/green stays reserved for walls). Derived sources other than SPY are now ignored
on the ES chart.

**NQ, on by default, same file.** FlexLevels routes rows by SYMBOL, so ENQU26 rows ride in
FlexLevelsExport.csv beside EPU26. Source: the panel's own self-fetched QQQ gamma book — own-King
%, node threshold, KING named, polarity colours, stale ⇒ absent. The QQQ→NQ ratio is MANUAL by
construction (Skylit carries no NQ price anywhere), so every NQ label wears `~` permanently;
settings gained NQ on/off + symbol + ratio (default 41.9). **CQG symbology throughout: EPU26 = ES
Sep-26, ENQU26 = NQ Sep-26 — quarterly rolls, user edits in settings.**

**THE RELOAD BUG (why the export "turned itself off"):** `loadCfg()` never merged `o.irt` back —
so "Export levels" silently reverted to OFF on EVERY page reload since the v8 config rewrite, not
just across updates. Caught live the night v14.11 went in (file 5+ minutes stale, cfg.on=false,
folder handle intact). Fixed: the irt block (incl. the new NQ fields) is merged like every other
persisted setting, and token-asserted in test_irt_export so a future rewrite cannot drop it again.

test_irt_export rewritten to the new spec (56 asserts: grammar, order, colours, NQ block, ratio
chain, absence asserts); em_band §40 labels updated; suite back to the six known pre-v13.8 reds.

## v14.11 — the derived payload's real semantics, measured, fixed, and WRITTEN DOWN

Operator: "make sure the export is done right and the percentages are accurate… and write down
everything you learn about derived levels so the next context knows." Both done, and the
verification caught v14.10 half-wrong:

**MEASURED on the live SPY feed's derived entry (source SPXW, 86 rows): the rows arrive
PRE-CONVERTED to the HOST's scale** (k=768.7677 = SPXW 7705 × 0.09977 — already SPY-scale). The
`ratio` field is informational, not an instruction. v14.10 multiplied by ratio anyway and only
survived through its decade sanity fallback. v14.11 flips the semantics: raw k is the primary path,
ratio-multiply is the fallback for a source-grid payload — both asserted in test_irt_export.

**The percentages: accurate, with two truths to hold.** A derived % is computed against the
payload's OWN largest strike, which equals that source book's %King — so `D-SPY 43%` = 43% of SPY's
own King, all strikes ≥ the node threshold (CFG.nodeThresh, default 20), NOT King-only. And the
derived window follows the PAGE's expiration selector, not 0DTE — measured: derived-of-SPXW said
7705=100% while the 0DTE ladder said 19%, both correct, different windows. Comparing them is a
window mismatch, not an error.

**NEW: session-state/SKYLIT-FEEDS.md** — the durable write-up of everything measured about the
feed: endpoint anatomy, auth (cookies alone 401), |net|≡v non-decomposability, snapshot series,
the derived schema and its pre-converted-k semantics, the window caveat, the velocity-fiber capture
and its cross-book pollution trap, and the consumers map. A load should read it beside
PROJECT-CONSTANTS whenever feed work is on the table.

## v14.10 — the Derived diamonds join the export

Operator: "since i want everything on the chart, im going to need the derived levels as well." The
diamonds on the Atlas chart are Skylit's DERIVED overlay — sibling books (SPY/SPX) projected onto
the chart's scale — which I first misread as VEX until the operator toggled GEX-only and the
diamonds stayed; the toolbar's Derived switch was the tell. Verified via the live network: every
feed ships `include_derived=true`, and each derived entry carries source, ratio and its own levels
snapshots (on the SPY feed: source SPXW, ratio 0.0998 — ratio maps SOURCE grid to HOST grid).

The observer, which used to DROP non-SPY/QQQ payloads, now keeps the SPXW gamma feed solely for its
derived array (`LASTSPXW`, freshness- and replay-guarded; the read/record pipeline still never
touches it). The export emits each derived row at source-strike × ratio → chart → ES, normalised to
the SOURCE's own strongest strike (the v11.4.3 lesson — never comparable to the native %King),
floored at the node threshold, and drawn exactly as Skylit draws the diamonds: slate, thin, dotted,
no polarity claim — labelled with its source: `D-SPY 43% ~`. A decade sanity-check falls back to
the raw strike if a payload ever arrives pre-converted; a stale SPXW feed exports NO diamonds
(absent, never old). test_irt_export grew the 5.5 block covering all of it.

## v14.9 — the FlexLevels file carries the rail, the 0DTE walls, and nothing else

Operator-directed, verbatim: "I need the ES levels which you get from the conversion from SPXW
which is what is displayed on the rail and atlas. I want the 0dte IF levels as well. I think that
should be all." So the export is now EXACTLY that: the rail's SPXW→ES nodes (KING/GK/BRK/ACC with
%King, polarity colours, King triple-weight, SUCC riding along — it is part of what the rail
displays) and InsiderFinance's 0DTE window (IF CR / IF PS / IF Mag / IF MP — switched from
to-Friday; a companion-suppressed side stays absent). REMOVED: the SPY-book role rows, the derived
lanes, our CR/CR0/PS/PS0/FLIP set, NextStop and PBentry. The nodeMap gate went with its rows — the
rail is the gate now, and one source down never kills the other (either exports alone; both down
writes nothing). test_irt_export was rewritten around the new contract, including ABSENCE asserts
so removed groups cannot creep back silently.

## v14.8 — FlexLevels lines say what they are, in the panel's colours

Operator-directed, from the first live IRT chart carrying the export: the labels repeated the price
("7680.00 | SPX 7680 BRK 43%") because IRT already prints the level on the line. Labels are now
NODETYPE + %King only — "KING 100% ~", "BRK 43% ~", "ACC 21%" — with the ~ ratio-honesty tag intact,
and node lines wear the PANEL'S polarity colours on the chart too: yellow = +gamma brake, purple =
-gamma accelerator (the rail's own hexes, in COLORREF). SUCC drops its strike the same way. King
keeps width 3 so it reads at a glance. IF/NextStop/PBentry rows unchanged — different objects,
different colours, still unambiguous about their book. The multi-market settings page (mockup
approved-pending) remains queued as the next build.

## v14.7 — ATTRACT: the fourth behaviour, drawn and measured

The operator closed the loop on the node lifecycle: **attract → in play → deflect | break**. Attract
had a metric all along — `w.pull = |cur| / distance`, recorded on every event since v13.4 "so it can
be TESTED and replaced" — and today it finally was tested: taking the single top-pull node each bar,
price moved TOWARD it within 30 minutes in **36/47 samples (77%)** on 2026-08-25. One day, and a
magnet day flatters the number — which is exactly why this ships as a RECORDED feature, not a vote.

**THE TRACTOR** on the rail: when one node's pull is at least 2× every other's, a dashed flow draws
on the track from price toward it, drifting INTO the node, in the node's colour — the fourth effect
beside breathe (in play), sonar rings (deflecting) and fracture (breaking). No dominant magnet, no
tractor: a flat book stays quiet. Hover carries the metric, the doctrine, and the one-day caveat.

**THE `attract` FEATURE** records per bar: the top node, its pull, its dominance ratio over the
runner-up, side and distance — with a scale-free toward/away outcome (net forward movement favours
the magnet\'s side). Two questions enrolled: does price go toward the magnet (vs the day\'s drift),
and is the ≥2× dominance gate (the tractor\'s own threshold) actually earning its keep. rules.json
is at 72 ids.

Verification that fed this build (2026-08-25 ledger, 624 events): the rolls were caught — 66 into
7665 built the operator\'s circled shelf, 23/22 into 7680/7685 built the closing wall; deflection
anatomy per touch showed 7665 holding 74% while GROWING (26 defl vs 19 brk) and 7675 while DRAINING
breaking **19 of 19** — the rate-of-change arbiter, measured, on one strike, same day.

## v14.6 — flag-posts, the directional pill, live-only rolls, and the after-hours rail

**1. FLAG-POSTS replace the discs** (operator-approved mockup). Each node is a vertical lozenge
BEHIND the price pill — z-order, not piercing — tops aligned under the role tier so the role name
can never be overlapped, height = sqrt(%King), colour = role, and the King alone is thicker and
wears the lone diamond cap. A 3px post taller than the pill means price can sit ON a node and the
node stays visible above and below the white body.

**2. THE THREE INTERACTION STATES**, each wired to what is MEASURED, never to an opinion: the
nearest post inside the deflection zone is IN PLAY and *breathes* (slow halo); if the node is
DEFENDING (reactDefence), it *rings* — fast green sonar off the contact point, a wall holding; if
ABANDONING, it *fractures* — the post renders as separating dashes with a red gutter-flicker, a
break forming. Three unrelated grammars, impossible to confuse. The disc-era motion grammar
(recv/grow/fade rings) rides the posts unchanged.

**3. THE PILL'S TIP IS ITS SILHOUETTE** (operator-directed): green tip on the RIGHT = last bar
closed up, red tip on the LEFT = down. Inside, only the rounded price.

**4. ROLLS ARE AN RTH STORY** ("there is no roll now" — caught live at night). `rollsLive()` gates
every DISPLAY of the latch — rail lane, profile RU/RD badges and bias chip, NODES connector — on
sessionPhase().rth. The latch itself keeps recording exactly as before.

**5. THE AFTER-HOURS RAIL tells the truth.** Post-close, the band is YESTERDAY'S expected move — a
straddle that expired at 15:00 forecasts nothing. One amber chip (AFTER HOURS · EM EXPIRED —
re-anchors at the open), EL/EH dimmed and struck through, the target, budget labels and roll lane
retired, the used-range fill faded. Pre-open keeps its own prevClose-anchor behaviour.

**⚠ THE SUITE FINDING, bigger than this build: 28 of ~40 test files never set an exit code.** They
printed FAIL and exited zero — the runner saw green. That is how the v13.9 connector rewrite broke
9 assertions in test_velocity_policy, v14.2's spacers broke the g3emk property checks in
test_depth_axes and test_node_rule, and nobody noticed for five versions. Every file now exits
nonzero on failure, and the stale assertions were rewritten for the designs that deliberately
replaced them (posts, gutter connector, textless lane, profile-header bias, AH-dim spans). The
suite's green is honest again; the six pre-v13.8 reds remain the known burn-down list.

## v14.5 — v14.4 shipped HALF-WRITTEN, and the operator caught it on the live face

The v14.4 edit script raised an assertion on its LAST replacement and exited WITHOUT SAVING — and I
read the traceback as "one edit failed" when it meant "every edit in that script was discarded."
The suite stayed green because none of the lost changes had test coverage; the LIVE FACE showed the
truth: %K still inside the bars, no grey minors, ROLL BIAS gone entirely (removed from NODES by a
separate script that DID save, never added to the header by the one that did not), and the price
pill clipped to a 10px circle showing "7" — the old .g3emn rule's width survived under my new rule.

v14.5 re-lands the three lost fixes (minors bound to the rail's own expiry; %K above the bar; ROLL
BIAS chip in the profile header) and adds two: `width:auto` on the pill so "7703▼" renders whole,
and NEIGHBOUR LABEL THINNING on the rail — 7655/7660 sit 2.5% apart tonight and their labels mashed
into "76557660"; the bigger pile keeps its label, a smaller one within 4% yields the text only.

⚠ PROCESS LESSONS, permanent: (1) a multi-replace script writes NOTHING when any replace fails —
apply-and-verify each, and grep the FILE for the new tokens after, never trust "it printed ok".
(2) A UI build is not verified by the suite alone: after install, the live DOM gets checked for the
specific elements the build claims (querySelector counts), the way this failure was actually found
— by the operator, which is the wrong person to be the test.

## v14.4 — five operator-caught refinements on the live v14.3 face

**1. The grey minors had vanished.** After the close Skylit rolls the front expiry, so the minors'
`exp === today` filter went false for every non-rail strike and the grey bars silently disappeared —
while the rail bars, exempt from the check, stayed. Minors now bind to THE RAIL'S OWN EXPIRY (read
from the rail strikes' harvested rows): one book on the face, one reference, whatever the ladder is
actually showing.

**2. %K moved back ON TOP of the bar.** v14.3 put it inside with the badges; the operator wants the
number above (King gold). The in-bar stack now holds only true badges — which also buys short bars
one more badge slot.

**3. ROLL BIAS moved to the GAMMA PROFILE header** (right-aligned chip, same red treatment, same
hover). The NODES copy is retired — the profile is becoming the primary read, and the one whole-book
line belongs on it. One home, not two.

**4. THE PRICE PILL.** The white dot on the rail is now a pill carrying the rounded ES price with an
arrowhead for direction — ▲ green / ▼ red from the LAST CLOSED BAR (a per-tick wiggle would flicker;
a bar is a decision). Stretched still turns it red (g3str). Same top row, so the money-label
geometry contract in test_em_band holds unchanged; edge-clamped like every label.

**5. The floating wall flag** (a CW/PW with no qualifying bar to live in) now draws INSIDE the band
at its strike instead of floating up beside the header.

NOT changed, by design: on the rail's roll lane one arrow animates and one sits still — that is the
grammar, not a bug: MOVING DASHES = the roll is in flight this bar; SOLID = latched and STUCK (the
window slid past but the destination holds). The profile bars use the same distinction.

## v14.3 — badges move INSIDE the bars, the ROC matrix, and a $ axis

The operator approved the mockups and supplied the letter codes; this build is that design.

**1. THE BADGE SYSTEM, IN-BAR.** Every marker now lives inside its bar as a 1-2 letter pill, so
nothing can overlap anything (the % labels and CW/PW flags used to share airspace above the bars).
The codes are the operator's: **S / R** support/resistance building, **SF / RF** failing, **TU / TD**
turning, **H** holding — colour keeps the nodeChip rule (what it means for PRICE: a failing ceiling
is green). **RU / RD** are latched rolls (roll-lane colours, dashed border on the way down) — bare R
is always the state, any two-letter R-code is always a roll, so the collision is structural, not
visual. **CW / PW** (0DTE walls) badge into the nearest rail bar within 3 ES points; a wall with no
qualifying bar keeps a floating flag, the one exception. ▶ watch (with a blue ring on the bar) and
the amber aged dot sit at the base. **THE FIT RULE:** the stack draws top-down in priority order
(%K, wall, state, roll) only while it fits inside the bar's height; everything is ALWAYS in the
bar's hover, so fitting hides pixels, never information. Grey minors never carry badges. Bars
widened 11→15px for two-letter pills; the band grew to 72px.

**2. THE ROC MATRIX** replaces the strike labels under the bars (the rail above already names every
strike). Three rows — 5M / 15M / 60M, captions in the left margin — one column per rail bar,
Skylit's own published percentages in the NODES green/red grammar. Reading a column top-to-bottom
is reading that node's momentum; this is the piece that lets NODES eventually retire.

**3. THE $ AXIS** sits in the left margin (inside a wrapper around the hidden EL clone, so the
frame's width still matches the rail exactly) with dashed gridlines at round dollar values —
SQRT-SPACED, because the bar heights are, and a linear axis on sqrt bars would lie. The ⓘ says so.

**4. Price** is a thin vertical white line with ▾ through the profile, replacing the bottom marker.

Legend row at the section's foot; header ⓘ rewritten for the new grammar. NODES is untouched this
build — it retires only after the operator has lived with the profile.

## v14.2 — the profile earns the rail's frame, %King on top, and clean books underneath

The operator approved the corrected mockup ("lets go with the skylit version") with one change of
meaning: **the % above each bar is %KING**, not %-of-own-peak — one size ruler across the whole
face, same as the NODES column and the rail. The peak story stays VISUAL (fill vs outline) and the
exact peak-% lives in the hover. The optional IF OI-structure row was declined.

**1. The frame.** v14.0/14.1 hung the profile on the full row width while the rail's percentages
live inside the track, which is flanked by the EL/EH labels — same numbers, different coordinate
frame, bars drifting up to ~20px right of their dots (measured on a screenshot). The profile and
axis rows are now flex clones of the rail row: hidden copies of the EL/EH label spans flank the
drawing area, so its x-space is pixel-identical to the track's. The gap is 6px because .g3emw's is.

**2. Linear fill.** The cross-strike sqrt (correct — the King must not flatten a 30% strike) had
been applied to the fill too, so a 63% bar drew 79% full. Fill is now hPk × (cur/peak): 56% means
the bar is 56% full.

**3. Today's SPXW expiry only — at every layer.** Pulling the peak table by hand found 225 of 315
peak-store keys were OFF-BOOK: SPY 765 at $900M, QQQ 711, a September row — the harvest holds every
ladder ever opened, and the tracker swept it all. Now: peakTick writes only rows whose harvested
`exp` equals today; the PROFILE draws non-rail strikes only with today's exp; `snap.vend` RECORDS
only today's exp (so future day files are clean); recorder SEEDING drops strikes below half the
largest recorded (the books sit a decade apart — self-scaling, no hard-coded index level); and the
peak store is schema-stamped v2 so every polluted v1 store is discarded, not trusted.
⚠ DATA CAVEAT: vend rows recorded BEFORE v14.2 may carry cross-book rows — the nightly must
range-filter them the same way.

**4. Axis in the rail's format.** frameNum, so the axis and the label above the dot cannot differ
("7664" both places; the NODES list keeps its tick precision).

**5. ⓘ info icons** (user-directed): on the GAMMA PROFILE header (the full how-to-read explainer),
the WALLS line (structure-vs-defence, the no-averaging rule, the F6 caveat), and the NODES header
(the three-axis node model: size = magnet, polarity = character on contact, rate-of-change = the
arbiter; rolls relocate S/R to the destination's side of price).

**6. Replay blank.** Today's peaks under Friday's rail would be a cross-window lie; the profile now
returns nothing in replay.

test_peak_profile grew to 20 assertions and caught a real bug before ship: the day-roll reset
dropped the schema stamp, so every post-roll save would have been discarded at the next load.

## v14.1 — the profile drew everything grey, off the rail's grid, from the wrong window

Three operator-caught defects in v14.0's profile, one build after it shipped.

**1. Every bar was grey.** The colour read `rail.cls` off emPiles rows — which do not CARRY `cls` —
so `NODE_COL[undefined]` fell back to grey for every strike, King included, and the deliberate
grey-means-minor distinction was invisible because everything wore it. Colours (and `role`/`isKing`)
now come from tradeNodes, the rail's own list. Minor strikes stay grey BY DESIGN; the rest wear
their role colours as the approved mockup showed.

**2. Bars could sit off the rail's grid.** v14.0 positioned every bar at `k × ifDispScale()` while
the dots above use the pile's own `disp`. Same intended number, two derivations — the exact
two-derivations-drift failure PROJECT-CONSTANTS documents. Rail strikes now take `es` straight from
tradeNodes (identical to the dot AND to the NODES list, to the tick); only off-rail grey strikes
still convert for themselves.

**3. CW/PW read the WEEK, not the day.** The flags took CR/PS from ifLadder — the to-Friday
structural set — when the operator wants the 0DTE walls. They now read `ifChain(...).dte0.lv.cr/.ps`:
InsiderFinance's call/put split in TODAY'S expiry only, with the companion's own suppression rule
(a side holding almost none of the book names no wall — their page prints N/A in that case, and a
missing flag is honest, never back-filled from a wider window). Hover names the window, the SPX
strike, and the ES conversion.

## v14.0 — the gamma profile under the rail, day peaks, and a quieter rail

**1. THE GAMMA PROFILE** (approved mockup, operator-directed). A volume-profile of the FLOW book
hanging directly under the rail, on the rail's own x-scale — every bar exactly beneath its node's
dot, at the ES price (SPX strike × live basis). Outline = the strike's DAY PEAK; fill = held NOW;
the hollow part is the position that CLOSED, live from Skylit — the thing IF's once-daily open
interest cannot show intraday. Rail strikes wear their role colours with a %-of-own-peak label
(green ≥90, red <50); every other harvested strike inside the rail draws GREY and thin — present,
visibly less important (user-directed). X-axis of ES strikes beneath (trim rule applies), price ▾.

**CW/PW flags ride the profile** at the IF ladder's CR and PS — the ONLY book with a call/put split.
Measured live before building: Skylit's feed has |net|≡v on every strike, so a call wall is NOT
computable from their book; IF's CR/PS rule previously reproduced a published table's tagged Call
Wall and Put Wall exactly. One descriptive WALLS line beneath states structure (IF) beside whether
the flow still defends it (Skylit %-of-peak at that strike) — the two books are stated side by side,
never averaged. This is the SpotGamma decomposition: their walls = OI (here IF), their live flow =
Skylit; the operator's question "which book is right?" had the answer "each owns half".

**2. THE DAY-PEAK TRACKER** behind it: running max of harvested |cur| per SPXW strike
(`gpts_peak_v1`, day-keyed, bar-gated writes, replay-guarded), SEEDED at boot from today's recorded
`snap.vend` rows so an install mid-session still knows the morning's high-water marks.

**3. Rail roll arrows lost their text** (operator: "it gets overlapped and i get the idea from just
looking at the arrows"). The v13.7 edge-clamp fought label collisions; v14.0 removes the label
rather than repositioning it again. Every number moves into the arrowhead's hover (padded to a
14×12px target), age included.

**4. Trailing .00 trimmed everywhere** esTick renders: 7709.00 → 7709; real ticks keep decimals
(7706.50, −22.75). One rule, one function, so the NODES list and the profile axis cannot disagree.

**5. The operator's support-roll hypothesis is now MEASURED**, not just believed. Stated verbatim
intent: "sometimes they roll up to create support and sometimes they roll down to create support,
as long as they are under price and price deflects." Both halves happened on the recording day:
morning roll-DOWNS built 7665, afternoon roll-UPS (13:33–14:00) stair-stepped 7675/7680 under
rising price. The `rolllatch` feature now records side-of-price per destination (`below`,
`destBelowConf`) and carries question `rolllatch_dest_below`: do latched below-price destinations
deflect better than below-price nodes that are merely growing (the F6 control)?

Also: the "GEX auto-pull" scheduled task (every 2 minutes, legacy Drive-inbox delivery, idle since
08-22) was identified as the cmd-window annoyance; removal is one command on the user's machine —
`schtasks /Delete /TN "GEX auto-pull" /F`. The daily 15:30 data-push task stays.

## v13.9 — rolls that STICK, a connector with its own gutter, and a poisoned scale-pin

Three fixes, all caught live on 2026-08-25 by the operator.

**1. The face showed rolls only while mid-flight — the doctrine count was never enforced.**
`rollScan` re-derives from Skylit's sliding velocity window every look. Between 10:00 and 10:23 four
nodes shed into 7665 ($14M/$21M/$16M/$14M on consecutive bars) and built the shelf price later
bounced from — and by the time price used it, the window had slid past and the face showed NOTHING.
Meanwhile one-scan flickers DID draw. That is backwards, and the counting rule that fixes it has been
in doctrine all along (rolling-floors-ceilings.md): 1 = noise, 2 = signal, 3 = confirmation — the
known open defect that the detector ignored it.

New: the ROLL LATCH (`rollLatchTick`/`rollLatched`, bar-gated like nevScan). One sighting never
draws; two consecutive bars draw it as SIGNAL (dashed, in flight); three LATCH it — it then outlives
the window, carries its age, and stays while the destination holds ≥60% of its at-confirmation mass.
When the destination bleeds below that, the chip says GAVE BACK for three bars and retires — a
support that returned its mass is not support, and deleting it silently would hide the reversal.
The NODES list, the rail lane, roll bias and the verdict all read the LATCH now — never the raw scan
(shared-array rule). Latch state survives reload via `gpts_rolllatch_v1`, today-keyed. Enrolled as
feature `rolllatch` (recorded, not voted — F6 stands until STUCK vs GAVE BACK is measured; rules.json
is at 71 ids, test pin updated).

**2. The v13.8 connector was drawn where it could not live.**
Its segments hugged `left:2px` — inside the watch row's inset bar and the marker glyphs — and only
node rows drew segments, so the line BROKE at the ES price row sitting mid-span. The badges
(`→ up 7684.00`, `⇢ in`) then re-said what the arrow failed to say.

New: the connector owns a gutter COLUMN (`--g3gut`, 26px with rolls, 0 on a quiet tape), part of the
shared grid, so it can never touch text. Dot = source; the line travels the gutter THROUGH the ES row
(which now draws pass-through segments); an elbow drives into the destination row and the arrowhead
lands at the gutter's right edge, pointing at the node it fed. In-flight = the rail's moving dashes;
STUCK = solid and calm; GAVE BACK = grey. Two arrivals on one row land high/low and never touch. One
chip on the source row names the roll in the rail's words — `ROLL ↑ 7684 · $1M · 47m` — and when the
SOURCE has vacated the list (the dissipated-node case) the chip moves to the destination and says
`from`. The receiver badge is retired: the arrowhead marks the receiver.

**3. The session scale-pin rode a poisoned capture for two hours.**
The v11.59 pin (one rr for the session, so the rails do not wobble) captured 10.0676 eleven minutes
in — a stale SPY leg against a live ES title — while the true ratio was 10.0436. Every rr-scaled
value sat +0.24% ≈ +18 ES points ALL MORNING: the dot rendered above the 7709 node while the chart
printed 7690, and the nodes themselves (scaled by dispScale, a different path) were correct, which is
exactly how the operator caught it. Patched live at ~11:35 CT by re-pinning from the live ratio.

New: SCALE-PIN SELF-HEAL in `emBand` — live ratio disagreeing with the pin by >0.1% SUSTAINED for
five minutes, from a live-trusted futMode, re-pins once and flags `rrHealed`. A real basis move is a
hair (0.003% measured 2026-08-22); only a poisoned capture holds a 0.1% offset for five minutes.
⚠ DATA CAVEAT for the nightly review: 2026-08-25 records BEFORE ~11:35 CT carry band-relative fields
(emPct, band edges) and node-distance fields (`w.dist`, from `B.now`) skewed ≈ +18 ES points. Node
velocities, sizes and the roll ledger are UNAFFECTED (vendor-verbatim, different path).

**4. The v13.9 installer itself hung — the v13.8 push had swept ~20 old downloaded installers
(~28MB) into `mockups/`, and the builder packaged all of `mockups/` blindly: a 30MB .bat whose
`more +HDRLINES` extraction ground through 390K lines and read as a hang.** The builder now excludes
`.bat` from every payload directory (an installer must never contain installers), refuses any payload
over 6MB with the offender list printed, and the emitted installer deletes `mockups/install*.bat`
from the repo before committing, so the push shrinks the repo back. They stay recoverable from git
history.

## v12.5 — the pop-out grow bug was a BOX bug, not a DRAG bug

`pipCopyStyles` told `#gpts-panel` to be `height:100% !important` while `html,body` had no height at
all. A percentage height against an auto-height parent computes to `auto`, so the panel silently took
its CONTENT height and the `100%` did nothing.

Measured live in the user's own pop-out: window `innerHeight` **598**, panel box **1104**.

`makeResizable` captures `oh` from the panel's rect at pointerdown, so every drag began from 1104:

- drag DOWN 1px -> target 1105 -> `min(maxOuter, 1105+95)` -> hits the 820 screen clamp on pixel one
- drag UP 500px -> target 604 -> 699 outer -> shrinks proportionally, fine

Shrink worked, grow was dead on the first pixel. That is the exact symptom reported for five versions.

Fix: `height:100%` on `html,body`. Verified live in the same window — panel measured **597** against a
598px window, and the drag then maps 1:1 in both directions.

⚠ Five versions (v11.93/95/96/97, v12.3, v12.4) attacked the DRAG HANDLER for a bug that lived in the
BOX. v12.4's pointer capture is a genuine fix for a genuine edge case and stays, but it was never this.

## v12.0 — the read line was naming the same level twice

Caught on the live face within minutes of v11.99 going in:

    "75% positive gamma brake at 7669 can stop price there.
     Next up: 7674 (32% brake, 4 away). Next down: 7669 (75% brake, 1 away)."

**7669 stated twice in one sentence.** The mechanism clause already names the node in the direction of
travel, and that node IS one of the two sides — so v11.99 printed it again. D-8 exists for exactly this:
*before adding anything to this section, check whether another element already says it.* The side the
clause has spoken for is now dropped, and the wording shortens to `Then up …, down …`.
⚠ An absent side still says **"nothing above" / "nothing below"** rather than being silently omitted —
an unmentioned side reads as a claim that nothing is there.

⚠ **My first test for this passed a build that still printed the duplicate.** It asserted "does not
match X…X" with a loose regex; the fix was to COUNT occurrences of the named level and require exactly
one. Restating a bug as a regex is not the same as measuring it.

### version pins are numeric now

Bumping to 12.0 broke three unrelated suites at once — `test_magnet_v1044`, `test_reco_deriv` and
`test_testing_tab` each pinned the version with a regex alternation listing every allowed major
(`10.4x|10.5x|11.x`). That is a tax charged on every major bump for no benefit. They compare numerically
against a floor now, which is what the assertion always meant.

### verified live on v11.99 before this fix

`renderErrors []` · sanity 0 failed · regime `−G +V` / BREAKS · badges `SKEW → · ACCUM ↑ · CROSS → ·
ROLL —` with `DRIFT ✗` · BIAS `↓ DNTREND BRK · 11 of 20 bars below the 50-SMA — lost 15, reversal needs
11` · **the rail rescale live**: price had run below the expected low, the rail read `7639 RAIL` against
`7730 EH` with the gold boundary marker drawn · SPXW ledger `n:8, bars:87` · every debug hook returning.
**ACCUM is answering again at n=7**, up from the starved n=4 that made it flip.

⚠ **I nearly reported a bug that was my own measurement artifact.** A first DOM read showed the regime
chip and three badges as dashes while the debug hooks returned healthy data. Reading the face and the
data in ONE synchronous pass showed them agreeing exactly — the first capture had simply landed between
renders. **When the face and the data disagree, suspect the sampling before the code.**

---

## v11.99 — the rail grows past a boundary, and the read line names both sides

**THE RAIL RESCALES WHEN PRICE RUNS PAST THE EXPECTED MOVE.** Until now a price beyond the expected
high pinned the dot at 100% and sat there. The band is a **priced level, not a barrier** — running past
it is frequent, and *how far past* is precisely what an overextension read exists to show. Pinned at
the rail it showed nothing at all.

The track now extends to hold price plus a quarter of the band's width, and the ORIGINAL boundary
becomes a marked gold line labelled `EH 7730` / `EL 7661`, while the rail END relabels itself **RAIL**.
⚠ **Otherwise the rescale would quietly redefine the rail end AS the expected move**, which is the one
thing that band must never be allowed to become.

⚠ **`emPos` STAYS CLAMPED, DELIBERATELY.** It is the MEASUREMENT — `pct` is recorded every bar as "how
much of the expected move is used", and letting it exceed 100 would silently change what months of
recorded data mean. Drawing moved to a separate coordinate space (`emRailBounds` / `emPosRail`) instead.
Two spaces on purpose: one is a number, the other is a pixel.

⚠ **The extreme is what widens the rail, not the current price.** A rescale that reverted the moment
price stepped back inside would flicker the day's high away exactly when it mattered most.

**THE READ LINE NAMES WHAT IS COMING EACH WAY.** It described only the direction of travel; a trader at
a level wants both — the next thing overhead and the next beneath, with distances, because that is the
shape of the decision. It names polarity, never a call: which of a brake or an accelerator helps you
depends on which way you are going, and the line does not decide that. D-7 stands.

⚠ **A real bug the first test caught: `undefined% accelerator`.** Skylit piles carry %King; the
InsiderFinance fallback carries DOLLARS and has no `pct` at all. Printing it blindly produced a number
that did not exist. Same split the pile hover already makes — one vocabulary never covered both books.

### the tests

Seven mutations on the rail fire 8/4/2/5/3/2/2. The one that fired ZERO first is worth keeping in mind:
`42t` originally asserted that *a* pile drew in rail space, and a mutation reverting a different mark
passed it. **A partial migration is the dangerous state** — some marks in rail space and some in band
space means the rail disagrees with itself the moment a boundary is run, and each mark looks
individually correct. It now asserts that NOTHING is still positioned by `P.pos`.

---

## v11.98 — the dead-code sweep, and two tests that only passed because of it

**`nodeBreadth` was declared twice.** The later one-argument version won for the whole file; the earlier
two-argument one was unreachable while reading like live code. **That is the exact shape of the `ifNum`
collision that shipped broken for nine releases.** Gone. `trendBadgeHtml` was declared twice with no
caller on either copy — also gone, and `test_no_dupes` now demands the collision list stay **empty**.

**21 uncalled functions removed, ~188 lines.** ⚠ Callers were counted with strings and comments MASKED
OUT. Counting prose as a call site has fooled six assertions in this project; here it would have fooled
a *deletion*, where the cost is a live function removed rather than a test left red.
⚠ The 17 functions carrying an explicit `(v11.0 audit) PARKED` marker were **left alone** — equally
uncalled, but someone deliberately kept them and that is not mine to reverse.

**13 CSS rules retired** — each fully overridden by a later duplicate. The check required the later rule
to be a property SUPERSET, which correctly spared `.g3cf`, where the second rule is additive.

### two tests were passing on the dead code

**`.g3tgt` cyan.** v11.75 gave the target chip a DIRECTION colour — green above price, red below — so
`.g3tgt` stopped being cyan. An older cyan rule sat earlier in the stylesheet, fully overridden and
invisible, and the assertion had been finding **that** one for 23 versions. Removing the dead rule
exposed it. It now asserts the rule that applies, and that the chip and the rail marker share one test.

**And I broke `feedStatusHtml` doing this.** The audit reported `txt`, `col` and `vexTxt` as computed
and never returned. `txt` and `col` were; **`vexTxt` was not** — it carries the dim `vex ⏳` marker that
exists so the footer never pretends VEX is live. My heuristic only checked the final `return` string and
missed the concatenation feeding it. `test_auth_capture` caught it immediately and the function was
restored whole from the v11.97 installer payload — **the third time that round-trip has been the backup
that mattered.**

⚠ **The lesson: an audit finding is a lead, not a licence.** Two of the twelve dead-code findings I
acted on were wrong, and both were caught only by tests that already existed.

---

## v11.96 — the pop-out height, and it was a regression I caused at v11.93

Measured live rather than guessed: **PiP window 598px, panel frozen at 581px, `#gpts-body` content
1021px.** Nearly half the panel was cut off and dragging moved it a few pixels before stopping.

**Two causes, both mine.**

The original pop-out rule was `height:auto !important; min-height:100vh` — the panel took its CONTENT
height and the window scrolled. v11.93 rewrote that whole rule to un-pin the panel for the grip and
**dropped `height:auto`**. So the inline height `restoreSize()` writes for the IN-PAGE panel (580.977px)
applied inside the pop-out and froze it at a height that has nothing to do with that window.

And v11.95's ceiling was **the window height** — `innerHeight − 8 = 590` against a panel already at 581.
That is the "it stops me": six pixels of travel. **The pop-out body already scrolls, so a panel taller
than its window is the normal case, not an error.** Ceiling is generous now; only the in-page panel
clamps at 2000, because nothing scrolls behind it.

Entering the pop-out now clears the in-page inline size so the stylesheet default can apply, and
restoring puts it back. ⚠ **`height` carries NO `!important`** — the stylesheet supplies the default and
an inline height from a grip drag has to be able to win, or the grip stops working again, which is the
loop this has been going round.

⚠ **Three attempts on one bug.** v11.93 unhid the grip; v11.95 pinned it to the window and capped the
height at the window; only v11.96 measured the actual numbers first. The panel/window/content heights
were available from the live PiP document the whole time.

---

## v11.95 — charting SPXW blanked the panel, and the gatekeeper never knew where price was

**SPXW blanked the whole dashboard, and the banner it showed was FALSE.** SPXW was absent from
`FUT_UNDERLYING`, so `futModeCompute` took the `!und` branch, set `ok:false`, and `render()` replaced
everything with *"SPXW has a Skylit options tape, but this panel is mapped to SPY/QQQ only — nothing
here is read or recorded for SPXW."* That sentence was written at v11.4.2 and **has been wrong since
v11.77**, when the FRAME rail moved onto Skylit's SPXW ladder. The SPXW tape is what the rail is
built from.
⚠ **DATA CAPTURE WAS NEVER AFFECTED.** `recordNodeSnapshot('SPY')` and `('QQQ')` are called with
hardcoded symbols every tick and `activeSym()` only ever returns SPY or QQQ. A display outage, not a
data gap. SPXW now maps to underlying SPY with its own ratio family, and the banner only fires for a
symbol genuinely unmapped.

**THE GATEKEEPER NEVER KNEW WHERE PRICE WAS.** `skRoles(pct, kingK)` was never given price, so it could
not apply the one clause that defines a gatekeeper. It took the strongest significant neighbour within
6 strikes on EITHER side of the King:

    King 7670 · 7665 below at 25% · 7675 above at 63%  ->  picked 7675, because it was stronger

With price at SPX 7665 the King was BELOW price and the "gatekeeper" was above both — it gatekept
nothing, and the rail labelled it GK anyway. **Reported by the user from the live face.** Only nodes
strictly between price and the King are candidates now, and with no price it refuses rather than
guessing a side.

**SKEW WAS PERMANENTLY GREY, FROM TWO FAULTS STACKED.** Live: `v=0.11`, `lo=2.2`, `hi=2.2`, n=240.
v11.37 switched the reading from their published header skew (~2.2) to a computed 25-delta IV spread
(~0.11) and **kept appending to the same store** — the current value was being ranked against a range
built from a quantity it is not. And the direction test is gated on `if(hi>lo)`, so a collapsed range
silently returned neutral for ever. The key is versioned by source now, and a dead range says so.

**ACCUM's direction was right; its sample was starved.** `dUp +$90M`, `dDn +$5M`, `dir −1`, "upside
building faster" — all coherent, because building ABOVE price is resistance stacking. The flipping was
`n=4` against a hard minimum of 4: only four strikes fell within `ACCUM_REACH=5` of price, so one
strike moving flipped the share. Reach 5 → 12.
**ROLL was never broken** — `{ok:true, dir:0}`, the King genuinely had not moved. It only looked broken
because a neutral rendered as a dash, identical to unavailable.

### FRAME and BIAS

FEEDS, ES/ct and the session-phase tag are off row 1 — ⚠ **the measurements behind them are not**, and
section 4z asserts that, because a removed badge that also removed its feature would empty
`flow_decays_when_books_fight` silently. Rails read **EL / EH**. The SPX strike now matches the ES price
in size and reads gray-white. Track 53 → 66px for air under the amounts.

BIAS names the machine's five states — **UPTREND · DNTREND · UPTREND BRK · DNTREND BRK · FLAT** — and
the grey line is the bar count **on the side the state is on** (the old line always read `w.up` and only
flipped the word, so a DNTREND could read "17 of 20 above"). A broken trend also states what it lost and
what a reversal needs. Badges read **↑ / ↓ / →** for their own direction; the COLOUR still carries
agreement, so the confirm count is unchanged.

### the pop-out limit was an unreachable handle

Measured live: panel **581px inside a 598px window**. The grip sits at the panel's bottom-right, so the
moment the panel grew past the window the HANDLE left the viewport and the drag stopped with no message.
Unhiding it at v11.93 was not enough. It is pinned to the window now and the ceiling is the window's own
height rather than an arbitrary 2000.

⚠ **I deleted ~400 assertions from `test_em_band.js` with a careless span edit and recovered them from
the v11.94 installer payload.** That is the second time the installer's round-trip has been the backup
that mattered. Assertions are now retired by INVERTING them in place, never by deleting lines.

---

## v11.94 — five refusals that rendered as readings, found by audit

Two agents swept the file for dead code and for silent failures. **Four of the findings were live and
one was mine from v11.88.** Every one has the same shape: a function returns its NEUTRAL value for
"no data", "not applicable" and "it threw", and a renderer turns that neutral into a confident
sentence. The project already fixed this at `emPiles` (D-6) and `kingRoll` (v11.88); these survived.

**1 · `mapStateOf` fabricated a measurement.** It answered `'hold'` for no nodeFlow, for a THROWN
error, and for a strike simply absent from the set — and the chip then printed **"holding"** with the
tooltip *"no 15m change beyond ±8% and near its session peak."* A measurement sentence with no
measurement behind it, on a node the trader is deciding at.
⚠ **The fix needed BOTH halves.** `mapChipHtml` rendered `!state` and `'hold'` identically, so
returning null alone would have changed nothing — and with the chip guard removed the label reads
**`dec`**, claiming DISSIPATING. A *stronger* false claim than the one being fixed.

**2 · `futModeCompute` could ship a half-built futures scale.** Defaults are `ok:true, r:1`, and
`out.fam` is assigned BEFORE the ratio. A throw in between left `{fam:'ES', ok:true, r:1}` — so
`dispIsFut()` was true, `dispR()` returned 1, `futMark()` was empty, and **every level rendered as a
SPY-scale number wearing an ES label. Off by ~10× with no approximation marker.** A failed conversion
is not a conversion; it now falls back to honest cash mode and says so.

**3 · `gatekeeper()` had five refusals and one all-clear, rendered identically.** No price, no walls,
no King, price AT the King, and a genuine clear path all returned `ok:false`, and both callers printed
**"No gatekeeper — clear path to the King."** Only the branch that actually walked the path now sets
`clear:true`; every other exit carries its own `why`.

**4 · A confirm that CRASHED looked exactly like one that abstained.** Each read in `biasVotes` has its
own try/catch and degrades to null — which is correct, one broken input must not take the tally down.
But both rendered as the same grey dash, so a permanently-crashing confirm could sit there for weeks.
`errs` records which threw; the chip marks it `!` and the hover says it is broken rather than quiet.
The structural backstop stays: a genuinely truncated assembly is zeroed rather than rendered short,
because `confColour` paints green on `nConf===nLive` and the denominator is the array's own length.

**5 · `kingRoll` raw call sites.** v11.88 fixed the vote path; the audit flagged two more. ⚠ **The
audit was wrong and my own test proved it** — stripping comments first shows the only raw call is
inside `kingRollRead`. The v11.88 fix was complete; the "finding" was prose counted as code, which is
now the sixth time that has fooled something in this project.

### the mutation harness itself had a hole

Four safety fixes fired **zero** assertions on the first sweep. Writing real tests exposed that the
`mapChipHtml` mutant **crashed** on an unstubbed `MAP_DROP` — and `grep -c '^FAIL'` counts a crash as
**zero failures**, so a mutation that killed the process read as "caught nothing".
⚠ **A mutation runner must treat a crash as a failure, not a pass.** It now checks for
ReferenceError / SyntaxError / TypeError before counting.

### also found, not yet fixed

`nodeBreadth` is declared twice (L1546 shadowed by L15846 — the `ifNum` failure mode, and the dead one
is unreachable), `feedStatusHtml` computes `txt`/`col`/`vexTxt` and returns none of them, 14 CSS rules
are silently overridden by a later duplicate, and ~710 lines across 24 functions have no caller.
Catalogued for the next pass rather than deleted mid-session.

---

## v11.93 — the node role moves above the rail, and resize follows the panel into the pop-out

**THE ROLE TIER.** `KING / GK / ACC / BRK / RUG` moves out of the label under the rail and into its own
tier at **11–17px**, between the money amounts and the track. Below the rail is now ES price over SPX
strike only. Sharing one line with the strike meant that on a crowded ladder `7645 ACC` and `7665 KING`
competed for width against their neighbours, and the role — the word that changes what you do — was the
half that got clipped.

Rail 14 → 19, dot 11 → 16, track 48 → 53px. ⚠ **The v11.67 contract it protects is unchanged**: the
money amounts own 0–9 alone. That bug was the dot's *ring* cutting the amounts, and
`getBoundingClientRect` does not include `box-shadow` — so the test asserts the dot's PAINTED top
(`top − 2`) clears 9, not merely that the numbers differ. The role tier is held to the same line.

**RESIZE IN THE POP-OUT.** ⚠ **The hidden grip was only half the reason it was dead there.**
`mousemove`/`mouseup` were bound to the **Atlas** document, but once the panel is appended into the PiP
window it lives in a *different* document and the pointer events happen there. The grip's own mousedown
still fired — an element listener travels with the element — so the drag ARMED and then nothing ever
moved. **Unhiding the grip alone would have shipped a handle that visibly does nothing**, which is worse
than no handle.

Now: the press binds move/up to `PANEL.ownerDocument`, whichever that is, and unbinds on release.
Vertical-only in the pop-out, because there the panel's width IS the window's width and a narrower
panel just leaves a dead strip. A pop-out height never overwrites the in-page size — that window has
its own.

### the removal comment trap, fifth occurrence

The test for `min-height:100vh` being gone failed **because the comment explaining its removal contains
the string**. Same trap as `DEX`/`TERM`/`ATR` at v11.90. The assertion strips comments first now, like
the others learned to.

---

## v11.92 — four things the first live session found that no test could

### 1. THE TOUCH STATE WAS ALWAYS THE CURRENT STATE

The feed stamps snapshots in **seconds** (`1787578200`); candles are in **milliseconds**
(`1787578200000`). The touch-state lookup is `T[q] <= c.t`, so every sample compared true and `si`
landed on the **last index every time**. Every touch was labelled with the node’s state *now*.

    763.00   32 touches   ALL "acm"     (current: acm)
    763.50   33 touches   ALL "dec"     (current: dec)
    764.00   20 touches   ALL "gone"    (current: gone)

⚠⚠ **This silently emptied `ledger.touch`**, whose entire question is whether a node ACCUMULATING as
price arrives deflects better than one bleeding. It was comparing a constant against itself.
**Every record written before v11.92 is void for that question** and the rule now says so.

Detection compares the TWO clocks, never the magnitude of one — the first attempt read
`T[0] < 1e11 -> seconds`, which is also true of any synthetic fixture using small integers, and
`test_node_ledger` caught it immediately by breaking on `t=940`.

### 2. THE FORMING BAR WAS COUNTED AS IF IT HAD REACTED

`closedCandles()` returns `STATE[sym].candles`, which includes the bar still forming. Its o/h/l/c keep
moving, so a touch counted `deflect` becomes `through` when the bar finally closes the other side.
**That is why 764’s deflect count was observed going 4 → 3 — a count of completed events must never
decrease.** The forming bar is excluded and reported as `pendingBar`.

### 3. SPXW HAD NO LEDGER AT ALL — ON THE LEVELS ACTUALLY BEING TRADED

    ledger('SPY')    bars 33   n 24     full touch/deflect/stall/through
    ledger('SPXW')   bars  1   n  0     EMPTY, all session

Two causes. `feedSeriesAll` reads `LASTFEED[sym]` and only SPY and QQQ have an entry — so `all` was
null. And `spxwCandlesFromSPY()` returned a **one-element array**, because its only consumer was
`updateTaps()`, which reads `cs[cs.length-1]`.

Fixed both: the candle builder converts the whole series (carrying `t` and `b`, without which nothing
can be matched to the tape history), and `tapeSeriesAll()` shapes `HIST[sym]` — which
`sampleTapeHistory` has been filling since v11.84 — into what `ledgerBuild` expects.
⚠ Samples are appended in LOCKSTEP across strikes, so a strike that appeared later has a shorter seq
and is **RIGHT-aligned**. Left-aligning would slide every reading of a late strike backwards in time.

### 4. CROSS WAS SILENT UNTIL 13:00 EVERY DAY

The horizons were matched to the SMA that owns the call — correct — but `trendVerdict` gets
`contCloses`, which reaches back days, while the spot series behind CROSS **starts empty at the open**.
Live at 09:56: *"SPY series too short (27 of 210 min)"*. 210 minutes puts the first reading near 13:00.

A short horizon (50-minute average, 20-minute window) now takes over, live about an hour in.
⚠ **The two are never blended.** Every reading carries `horizon`, the record carries it, and if the
pair cannot reach the SAME horizon the read ABSTAINS — one side on 210 minutes against the other on 70
is not like-for-like. `cross_short_horizon_holds` asks whether the short read is worth the same.

---

## v11.91 — a debug hook that threw on the live page, and the test that could never have caught it

`__gptsDebug.trendRec('SPY')` threw **`trendMachineRecord is not defined`** on the tab, minutes after
v11.90 shipped with a full green suite.

`trendMachineRecord` and `biasConfirmRecord` were declared **inside `registerCoreFeatures()`**. The
feature closures are created in that same scope, so recording worked perfectly — but the `__gptsDebug`
hooks are declared at top level and cannot see a nested declaration.

⚠ **No amount of executing the function would have found this.** `eval(ex('trendMachineRecord'))` gives
the function a scope the real file never gives it — the harness hands it exactly the visibility it is
missing. Extraction-based tests verify BEHAVIOUR and are structurally blind to PLACEMENT.

Both lifted to top level, and `test_trend_machine.js` §10 now parses the span of
`registerCoreFeatures()` and asserts that **no function called by name from a `__gptsDebug` hook is
declared inside it**. Re-nesting `trendMachineRecord` fires 2 assertions.

**The lesson to carry:** the harness is not the runtime. Executing a function proves what it computes;
it proves nothing about whether the caller can reach it. Only the live page, or a check on where the
declaration sits, can say that.

### verified live on v11.90 before this fix
`renderErrors: []` · DRIFT renders as an outlined badge, the old gate row is gone · `CROSS` reads QQQ
up 54 of 60 against SPY up 59 of 60, `same: true` · `ROLL` correctly abstains as `null` (fewer than 3
King samples) so `nLive` is 3 and not 4 — the v11.88 null-vs-zero split working as intended ·
`revThresh: 11`, `domThresh: 15`, all three trend machines agreeing at `up`.

---

## v11.90 — the trend machine loosens, DRIFT becomes a badge, every hover leads with a question

### THE TREND MACHINE — a reversal now needs 11, not 15

A NEW trend still confirms at 15 of 20. A **reversal out of a broken state** confirms at **11**, on the
user's reasoning that the break is itself evidence and waiting the full 15 costs 4 bars after the turn
is already visible. From a 20/20 uptrend rolling over: `up-broken` at bar 6 (unchanged), DOWN at bar 11
instead of bar 15 — **12 minutes earlier**.

⚠ **RAW, no slope gate — the user's explicit choice.** The 50-SMA spans 150 minutes, so 11 bars below a
still-RISING average is a pullback rather than a downtrend, and raw calls DOWN there. A slope-gated
variant is computed and recorded on every bar so the choice is measurable instead of arguable.

⚠⚠ **THE COST, MEASURED, NOT FEARED.** Once a trend has confirmed once, **both** directions flip at 11 —
after reversing, the prior becomes the new direction and the mirror rule reverses back at 11. The
minimum gap between two OPPOSITE flips falls from 10 new bars (30 min) to **2 (6 min)**. There is a test
asserting exactly that, so nobody "fixes" it by accident. **If the recording shows whipsaw the answer is
a DWELL TIME — a minimum number of bars in a state before a reversal may fire — not a retreat to 15,
which would simply undo what this was for.**

**One machine ships, three are recorded**: `state` (loose), `stateStrict` (the old 15/15), `stateGated`
(11 with the slope gate), plus `flip`/`sinceFlip` for the whipsaw question. Each shadow keeps its OWN
`prior` — a shadow sharing `TREND_LAST` is an echo, not a shadow, and there is a test for that too.
Precedent: `trendWindowRead` has recorded 10- and 20-period MAs unvoted since v10.51 for this reason.

### DRIFT IS A BADGE, AND IT STILL DOES NOT VOTE

It moves onto the confirm row and the full-width gate row is gone — but **outlined, never filled, behind
a divider, and excluded from `nConf`**. Two decisions had to survive that:

- **v11.44** pulled drift out of the confirm row because its tick meant *the two books agree with EACH
  OTHER* — the face read `↑ BULLISH` beside `DRIFT ✓ DN·conf`: they agreed on DOWN, against an up call.
  The `withCall` test that fixed it is untouched.
- **The user shadowed drift on 2026-08-18** — *"remove it until it is tested and proven"* — and
  `DRIFT_LIVE` is still false. Measured 2026-08-24: **AGREE-UP 25% on effN 10 against a 21% baseline,
  2 sessions.** It has not earned promotion.

Drawn identically to the confirms it sits beside, it would read as a fifth one and inflate the very
count that only started being recorded at v11.88.

### SEVEN HOVERS REWRITTEN

Question first, one plain sentence, then only the caveats that change what you would do. The count
hover still said *"three of three agreeing"* with four confirms on the row, and still claimed the count
was "doing real work" — a claim with no recorded bar behind it until v11.88. Both corrected.

### Test rot, three more instances

- `test_export_full.js` grepped for the literal line `TREND_LAST[sym]='up'; trendLastSave()` — the v11.89
  refactor to one resolver broke it while the behaviour was intact.
- `test_drift_gate.js` grepped for a comment HEADING that moved.
- Its replacement then failed because **prose in a comment WRAPS** and a wrapped phrase does not match a
  one-line regex. It now strips comment markers and collapses whitespace before looking.

⚠ Three of the first eight trend mutations fired ZERO assertions: a reversal firing straight out of
FLAT, the strict shadow writing the live memory, and the neutral band being deleted. The band one is
instructive — a "bar sitting on the average" fixture was not sitting on the average, because a mixed
series makes the SMA drift. **Only a flat series can prove that band exists.** All eight fire now.

---

## v11.88 — ② BIAS: PA out, CROSS and ROLL in, and the tally finally recorded

**PA no longer votes.** It reads where recent bars close inside their own range — the *same price
series* the 50-SMA above it reads. In an uptrend bars close near their highs more or less mechanically,
so PA agreed for free: a confirm CORRELATED with the thing it confirms inflates the count without
adding evidence. That is Pattern 7, a one-directional factor earning accuracy for free.
⚠ **It is still computed and now RECORDED.** `paRead` also feeds ④ REACTION's PRICE row, and shadowing
rather than deleting is what makes *"was removing it right?"* answerable — `bias_pa_shadow` asks exactly
that, and PA earns its seat back on evidence if bars where it would have confirmed score better.

**CROSS — the only confirm that is not another reading of SPY's own book.** SKEW, ACCUM and ROLL all
come off the same option chain; two INSTRUMENTS agreeing is the only independent evidence available.
Both feeds already carry a 390-point spot series at `j.levels[i].s` — 389 minutes of coverage, measured
live. ⚠ **Not `trendVerdict`:** QQQ has NO candles (`STATE.QQQ.candles` is 0 — the chart only builds
them for the symbol it is on), so a candle-vs-snapshot comparison would be the apples-to-oranges error
this project keeps making. Both sides are measured by the SAME rule on the SAME field, at horizons
matched to the SMA that owns the call: 150 minutes of average, a 60-minute window, and TREND_DOM's
15-of-20 becoming 45-of-60.

**ROLL votes** — the settlement magnet migrating. ⚠ It was deliberately `RECORDED not voted` since
v11.0 because whether it LEADS price was an open measurement; it votes now at the user's instruction
and `kingroll_leads_dir` remains the question that settles it.

### A real bug the change exposed

`kingRoll()` returns **0 for two different things** — "the King has not moved" and "there is not enough
King history to say." Harmless while it only fed a recorder; **not harmless once it votes**, because 0
counts as a live-but-neutral confirm and inflates `nLive` while an absent read must abstain.
`kingRollRead()` separates them. Same rule as everywhere else here: absence of data is not a reading.

### THE TALLY WAS NEVER RECORDED

`biasVotes` computed SKEW / ACCUM / PA and `nConf`, the face printed "1 of 3 confirm", and **none of it
reached the recorder — not one of 224 recorded bars carried it.** Meanwhile every candidate that does
NOT vote is richly recorded (`dir.drift.vote`, `dir.kingRoll.vote`, `dir.struct.vote`, `dir.trend5.vote`,
`dir.trendFast.vote10/20`). **The three factors that actually voted were the only ones with no data**,
so the v11.36 premise this section was rebuilt on — *"TREND with 3 of 3 confirming is a different
proposition from TREND with 0 of 3"* — has never been testable, while its own hover claims the count is
doing real work. Now enrolled as `bias.confirm` with four questions and a rule.

### The colour rule had a hardcoded denominator

`nConf>=3` for green, `<=1` for red. The moment the list stopped being three long, green became
unreachable and every 1-of-4 would have read red. Extracted to `confColour(nConf, nLive)` and judged as
a FRACTION of what is live. **A hardcoded denominator in a renderer is a silent bug the first time the
list changes length.**

### Testing

`confColour` and `biasConfirmRecord` were extracted from a renderer and an anonymous registry callback
specifically so tests could EXECUTE them — three mutations (cross comparing a symbol against itself,
the colour rule reverting to `>=3`, the tally dropping PA) each fired **zero** assertions before that.
Eight mutations now fire 11 / 2 / 3 / 2 / 2 / 1 / 1 / 1.
⚠ Two traps hit while writing them, both recorded in the test file: `eval(ex('crossRead'))` creates a
LOCAL binding that **silently shadows** the stub every other test depends on — that block must run last;
and the first dominance fixture was monotonic, so 60-of-60 satisfied both the 75% rule and a plain
`up>dn` and the threshold could have been deleted undetected. The fixture now sits at 58%.

---

## v11.88 — PLANNED / NOT YET BUILT

**Approved scope is ONE change:** put the current price inside the white circle on the band rail
(`.g3emn`), small — 7.5px text, 11px tall, one pixel taller than the 10px dot it replaces, so it stays
inside the existing geometry tier. Mockup: `mockups/frame_v1188_dot.html`.

⚠ **Six other mockup revisions were produced and REJECTED** — +50% then +33% level type, a node-role
tier above the rail, staggered label shelves, a PACE chip, a SUCC chip, four rewritten hovers, and
used/remaining figures beside the price box. Do not resurrect them. See DECISIONS.md D-13.

### Found this session, real, NOT fixed

**Two of the four used/remaining figures silently disappear.** `secFrame`'s money tier draws used and
remaining on each side of the open; `seg()` returns '' when its segment is under 9% of rail width.
Measured live 2026-08-24: down-remaining $1,684 and up-used $1,814 render, while down-used $53 (1.5%
wide) and up-remaining −$77 (0% wide, and NEGATIVE because the high of day exceeded the expected high)
both vanish. **A figure that disappears exactly when the day has run past its own rail is the case you
most want to see, and it renders identically to "nothing to report."** Same disease as D-6.

**Two dead computations.** `gp = gexPath(sym)` in `secFrame` is assigned and never read.
`PA = emPath(B, sym, B.now)` in the `piles` recorder is computed and never returned — that one reads
as though the path is being recorded every bar, and it is not.

### Corrected: a flaw I reported that was already fixed

**`pathStrictly` / "the target is inside its own path sum" is NOT open — v11.68 closed it.** `emPath`
explicitly skips the target and reports it separately as `atTargetPerPt`; the 82% / $107M / $23M
figures are inside that function as the fix's rationale. I read `FRAME-APPROACH-REVIEW.md`'s fix list
instead of the code. **The review is a snapshot of 2026-08-23 and later builds closed items without
updating it — verify every review item against the code before calling it open.**

### The LLM layer

Discussed at length, nothing built. For PREDICTION a live LLM is the weakest option available and
will not beat `directionGrade` on the same inputs; its real value is comprehension, anomaly flagging
and hypothesis generation. The only plausible live edge is data the panel does not have, or retrieval
over the user's own recorded history. Measured: a day export is 5.9 MB, the live tape state is 6 KB.
`@grant none` means a 127.0.0.1 relay, never a grant change. See DECISIONS.md D-11 and D-12.

---

## v11.87 — the disagreement was declared but never written down

**v11.83 taught the flow chip to say "the two books disagree." Nothing recorded it.** The face could
carry that warning every session for a month and the scorecard would still be unable to answer the only
question that makes a disagreement worth showing: **which book was right.** `conflict:` appeared exactly
zero times in any recorder. The project's own FEATURE ENROLLMENT rule is DATA + ANALYSIS + TESTING; this
had none of the three, and it went four builds that way.

Now the per-bar `flow` record carries `conflict` and `regimeG`, and the feature asks the question on
**both** sides of it:

    flow_holds_when_books_agree   conflict=false -> reachEM     the control arm
    flow_decays_when_books_fight  conflict=true  -> reachEM     the claim

⚠ **The conflicted arm means nothing without the control arm.** A hit rate under conflict compared
against a POOLED rate is comparing a slice to a mixture that contains it. If the agreed reading scores
and the conflicted one does not, the conflict flag is a filter worth trading on; if they score the same,
it stays a disclosure and nothing more. `flow.perPoint`'s rule now says so where it will be read.

⚠ **`conflict` is a TRI-STATE and is stored as one.** `null` means the regime was unreadable, which is
not "the books agree" — collapsing unknown into fine is the same mistake D-5 fixed on the expiry.

**`__gptsDebug.flow` is a RESHAPE, not a passthrough**, and it silently dropped every field `hedgeFlow`
gained after v11.62. The one place you would go to check whether the books disagree could not tell you.
It now reports `conflict`, `regimeG`, and a `books:` sentence in words, so the answer does not depend on
reading a boolean the right way round.

### THE TESTS GUARDING v11.86 WERE NOT TESTING ANYTHING

Section 40 of `test_em_band.js` shipped as **fourteen source-greps** — every assertion asked whether the
file CONTAINED a string. Swap `toSpy(P.k)` for `P.disp`, or the tick from 0.25 to 1.0, and all fourteen
still passed. **This is the same trap as the v11.70 forecast-ban test, the third time in this project.**
It now builds the CSV and reads the prices out:

    SPX 7710 KING -> ES 7727.75      SPX 7700 GK -> ES 7717.75
    SPX 7650 BRK  -> ES 7667.50      SPX SUCC 7630 -> ES 7647.50

and proves the scale property by building **twice at different futures ratios** and demanding identical
prices — `R.r` cancels through the `dispScale` route and does not through `undScale`, which is what makes
the choice load-bearing rather than cosmetic. Mutation-tested: forgetting the SPY-space division fires 6
assertions, no scale at all 7, wrong tick 7, dropping the Skylit-source guard 3.

The same rot was found in **six more suites** — 12 assertions failing against code that was correct,
each grepping for a name that had changed (`cell('EM'`, `/momentum/` where the chip now says `BREAKS`,
`MP` where v11.75 added the load-bearing asterisk). One of them believed `DEX`/`TERM`/`ATR` were still on
FRAME **because they appear inside the comment explaining their removal** — the fourth time that specific
trap has bitten this project. The repair strips comments before looking. `test_node_ledger` asserted
`Object.keys(rules).length === 61`, a count of every rule in the project, which broke at 68 and never once
checked what its own message claimed.

⚠ **A test that can pass on a build emitting the wrong number is documentation, not a test.**

### `install.bat` IS GENERATED NOW, NOT HAND-EDITED

The v11.86 installer announced **"GEX Tapereader installer - v11.49"**, named its temp files
`gex-v1149-payload`, committed with the message **"v11.79 ..."**, and told the user the companion was
"@version 1.8, unchanged" when it was **1.13**. Four stale strings from three different builds, in the one
artefact the user actually runs. **The payload was correct every time, which is exactly why nobody
noticed.** `tools/build-installer.py` now reads every version from the files, computes `HDRLINES` to a
fixed point rather than guessing it, round-trips its own payload against the working tree before writing,
and refuses to emit a file containing a stale `v11.x` in the header or the word PowerShell.

### also

`DECISIONS.md` had **two D-7 entries**. The replay-keying one is now **D-10**, and `trackSpxwNodes`'
comment points at it.

---

## v11.86 — the SPX levels reach the chart, in ES, on the tick

**The rail has drawn Skylit's SPXW nodes since v11.77 and the chart has never carried them.** The levels
actually being traded off were the ones missing from the chart.

Every node now exports with its role and colour — King gold, Gatekeeper white, Rug red, accelerators
purple, brakes green — labelled `SPX 7710 KING 100%` so nothing on the chart is ambiguous about where a
line came from.

⚠ **THE SCALE MATTERS MORE THAN IT LOOKS.** Rows are collected in SPY strike space and the export already
multiplies by the SPY→ES ratio and snaps to the tick. So an SPX strike has to arrive as

    kSpy = (spxStrike * dispScale) / R.r        NOT   spxStrike * undScale

Both are legitimate conversions and **they differ by about 0.9 points** — the two-path slack measured at
v11.82 (7691.75 via SPX, 7691.67 via SPY). Going through `dispScale` guarantees the chart line lands on the
SAME price the rail shows. **A chart that disagrees with the panel by a point is worse than a chart with
fewer lines on it**, because you would trust it and it would be quietly wrong.

⚠ **ES TRADES IN 0.25 INCREMENTS, AND DISPLAY ROUNDING IS NOT CHART ROUNDING.** `irtRound(k*mul, 0.25)`
already existed and does the right thing — these are tradeable prices. The FRAME row rounds to WHOLE
points because v11.75 asked it to; the chart must not borrow that. **Two different jobs, and a test now
asserts `frameNum` never appears in the export builder.**

**SUCCESSION IS DRAWN, AND ITS PROVENANCE IS LABELLED.** The strongest non-King strike goes on the chart
once it clears **60%** — the same cut the project's own backtest uses, where the crown rolls to it 76% of
the time within 20 bars (n=148).
⚠⚠ **THAT 76% WAS MEASURED ON SPY.** It is a label on a line, never asserted as an SPX probability, and
`spx.nodes` exists precisely to find out whether it transfers. Live on the SPX book right now: King 7710
at 100%, successor **7630 at 85%** — well past the threshold, and previously invisible.

⚠ Nodes are exported ONLY when they came from the Skylit book. If the rail fell back to InsiderFinance,
the chart draws nothing rather than drawing levels from a different measurement under the same labels.

All three guards mutation-tested: converting via `undScale` fires 3, dropping the succession threshold
fires 2, exporting on the fallback book fires 2.

## v11.85 — the tracking was recording a REPLAY as if it were today

**v11.84 shipped SPX node tracking and I verified it was running. It was — and it was recording the wrong
day.** `sampleTapeHistory` keys its samples by `todayKey()`, the WALL-CLOCK date, and is **not**
replay-guarded. On a Sunday showing Friday's tape that writes **Friday's node values under Sunday's key**.

⚠ **THAT IS NOT WRONG DATA, IT IS MISLABELLED DATA, WHICH IS WORSE** — nothing downstream can tell. The
entire point of tracking these nodes is to feed the scorecards and the end-of-day review, and a history
seeded with replayed sessions presented as live ones would poison exactly the loop it exists to serve.
Caught only because "check" prompted a look at whether tracking ran, and it ran on a weekend.

`trackSpxwNodes` now refuses in replay and records why. **The check happens BEFORE the sample, not after.**

⚠ **THE SAME HOLE EXISTS ON THE SPY PATH AND IS NOT FIXED HERE.** `sampleTapeHistory` has been unguarded
for many versions, and there is a plausible reason — the node chart needs history to draw at all, so
sampling during replay may have been deliberate. **Silently changing the keying of a long-running path
without evidence is its own risk**, so it is recorded as D-7 rather than patched on a hunch. The fix, when
taken, is probably to key by the session BEING SHOWN rather than the wall clock, which keeps the chart
drawing and labels the data correctly.

**AND THE TRACKER GOT AN INSTRUMENT.** I built it with no way to ask whether it was running and had to
infer from `nodeChart('SPXW')` returning a strike count. **Third time this session a new read shipped
without a hook.** `__gptsDebug.spxNodes()` now answers in one call: the tracker's own verdict, history
strike and point counts, tap counts, replay state, and the tape's King and source.

⚠ **A COMMENT-POSITION TRAP, AGAIN.** The test asserted the replay check comes BEFORE the sampling call by
comparing `indexOf`. The comment ABOVE the guard mentions `sampleTapeHistory`, so `indexOf` found the
comment first and the assertion failed on correct code. Comments stripped before comparing — the same trap
as v11.69 and v11.76, now three times.

## v11.84 — the SPX nodes are tracked now, not just drawn

**The rail has drawn Skylit's SPXW nodes since v11.77 and thrown every reading away.** No history, no
accumulation, no peak, no tap counts, nothing in the daily export, nothing the end-of-day review could
read. **You cannot build a mental model from data that is not kept**, and the LLM cannot suggest anything
about levels it never sees.

⚠ **THE GAP WAS SMALLER THAN I MADE IT LOOK, AND THE USER HAD TO SAY SO.** I answered "SPXW is DOM-scraped,
there is no feed, this needs a timer and new plumbing" — then went further and turned a side discovery
about feed lanes into a blocker. The user's reply was simply *"we clearly have a spxw tape"*, and they
were right. `sampleTapeHistory(sym)` needs only `tapeMap(sym)`, which already worked for SPXW (100
strikes, King dollar-anchored), and every store it touches auto-creates:

    var store = HIST[sym] || (HIST[sym]={});

**The history side was ONE CALL.** I spent a round theorising before checking that. The lesson is the same
one this project keeps re-learning: **check what the code already does before describing what it would
take to make it do that.**

**WHAT IS NOW KEPT, on the same cadence as SPY:**
- per-strike %King history (`HIST.SPXW`) — accumulation, Building / Steady / Fading, session peak
- King history, so a migrating King is visible rather than inferred
- **tap counters** — the 1st / 2nd / 3rd-tap lifecycle, now available on the SPX book

⚠ **TAPS WERE THE ONLY REAL WORK, AND THEY ARE SKIPPED RATHER THAN GUESSED.** `updateTaps` asks "did price
touch this strike" from `STATE[sym].candles`, and there is no SPXW price series — the chart is ES, the feed
is SPY. The candles are synthesised by converting SPY's with the ladder's own `undScale`, the same number
the rail already uses to place the nodes. **If that scale is unavailable the taps are skipped and the
reason recorded.** A tap counted against the wrong strike is worse than no tap count: the lifecycle read
would be grading the wrong level and would look perfectly normal doing it.

**ENROLLED AS `spx.nodes`, AND IT RECORDS THE LEVEL, NOT A COUNT OF LEVELS.** Per bar: which strike is next
in the direction of travel, its %King, its role, how far away, how many times it has been tapped, and the
King it is measured against. The existing `piles` feature recorded *counts* — how many accelerators, how
many brakes — which can never answer "did 7710 hold".

⚠ **NON-VOTING, AND THE QUESTIONS SAY WHY.** The tap rates this project quotes (1st ~80%, 2nd ~66%,
3rd ~33%) were measured on **Skylit's SPY backtest**. `spx_first_tap_holds` asks whether they transfer.
**Assuming they do is precisely the borrowed-number mistake this feature exists to avoid.**

⚠ It refuses to record when the rail fell back to InsiderFinance — a record whose source silently changed
mid-session is worse than a gap, because the gap is visible.

All three guards mutation-tested: removing the thin-tape gate fires 2, counting taps without a scale
fires 2, recording on the fallback book fires 2.

## v11.83 — the last two open items, closed

**D-5 · `dte0` MEANS "NEAREST LIVE EXPIRY", NOT "TODAY".** InsiderFinance drop an expiry from the payload
the moment it expires, so a chain captured after the close prices the NEXT session while the chart still
shows this one. Measured 2026-08-21 20:04Z: today 20260821, earliest expiry 20260824, and their own page
printed `0DTE Exp 0.0%`.

The rails now carry **`≠TODAY`** beside the label when that happens, and the hover names the expiry the
band is actually pricing. ⚠ **A visible marker, not only a hover** — a caveat that lives only in a tooltip
is a caveat nobody reads, which is the same reasoning that put `~EST` on the rails at v11.75.

⚠ **THREE STATES, NOT TWO.** `false` = genuinely today · a date = that expiry · **`null` = cannot tell**.
Collapsing "unknown" into "fine" is how a silent wrong day would get drawn, so the detector returns null
and the marker stays off rather than asserting either way.

**D-4 · THE FLOW CHIP WAS THE LAST ELEMENT READING A DIFFERENT BOOK FROM THE CHIP BESIDE IT.**
`FEEDS $214 M / PT` is InsiderFinance; the regime chip and now the nodes are Skylit. Both make the SAME
claim — the sign of gamma — and nothing compared them. On a day where Skylit reads short gamma and IF
reads long, the row would say `FIGHTS` next to `−G ⚠` over a rail of purple accelerators, and nothing
would say which to believe.

**IT DOES NOT SWITCH SOURCE.** IF's number is the only per-point figure available, and quietly recomputing
it from Skylit would replace a disclosed mismatch with a hidden one. The chip **declares** the conflict:
a red border, a ⚠ prefix, and a hover naming both answers and what each is measuring.

⚠ **AND IT HANDS THE JUDGEMENT BACK.** *"Neither is a check on the other — a stock beside a flow — but
when they conflict, trust the one whose window matches your horizon and treat the level structure as
contested."* Picking a winner would hide the one thing worth seeing.

⚠ **THE SELF-DERIVING HOOK GUARD CAUGHT `notToday` ON THE SAME BUILD IT WAS INTRODUCED** — the face read a
field the hook did not return, and the test said so before it could cost anything. That is the second time
that guard has paid for itself since v11.71.

All three mutation-tested: making `null` return `false` fires 3, removing the visible marker fires 3,
dropping the regime comparison fires 2.

## v11.82 — the piles hook was still IF-shaped, one build after I wrote a guard for exactly this

**THE FACE SAID `7710 KING`. THE HOOK SAID `ACC`.** The piles hook reported `gross` / `net` / `netFrac` /
`perPt` — fields a SKYLIT node does not have — and omitted `role`, which the face renders. So a live check
of the roles was impossible, and the two disagreed about the same node.

⚠ **THIS IS THE v11.71 FAILURE, REPEATED ELEVEN BUILDS LATER.** v11.71 found the face reading `pace` while
the hook did not return it, and shipped a self-deriving guard so it could not happen again. **That guard
only covered `emBand`.** A guard that covers one hook teaches the wrong lesson: it feels like the class of
bug is closed when only one instance is. The guard now covers the piles hook too, and asserts the hook
BRANCHES on `P.src` rather than reporting fields the node does not have — IF-only figures come back `null`
for a Skylit node instead of silently absent.

**VERIFIED LIVE ON v11.81 BEFORE THE FIX**, which is how it was found:

    labels on the face   7728/7710 KING   7718/7700 GK   7668/7650 BRK
    roles.byK            {7700:"GK", 7710:"KING"}     gkRatio 0.79  passable
    hook per-node keys   k, disp, pct, pos, gross, net, netFrac, perPt, shown, accel
                         ^ no `role`, and five IF-only fields on Skylit data

⚠ **The regime chip read `—` in a raw innerText grab and `−G −V ⚠` across five sampled reads. Third time
this session.** A single snapshot of a live face is not evidence — and the rule keeps earning its place.

## v11.81 — the node ROLES, which v11.79 shipped as polarity and called done

**The user asked for KING / GATE / RUG / REVERSE-RUG. v11.79 shipped ACC / BRK / BAL.** That is POLARITY —
how hedging behaves at a node — not ROLE, which is what shape the book has built around it. Both matter,
they are not the same label, and I mocked the roles twice without building them. Same failure as the
labels one build earlier.

**They run on Skylit's own doctrine, with the existing constants reused verbatim rather than re-invented:**

    RUG_ANCHOR_PCT 40    the yellow ceiling and the purple node must each be genuinely strong
    RUG_ADJ 3            and sit within this many LADDER STEPS of each other — index, not points, so a
                         5-point SPXW grid and a 1-point SPY grid mean the same thing
    RUG_SIG_PCT 20       what counts as a significant floor at all
    GK_RATIO_STRONG 1.8  |gatekeeper| / |King| at or above this = expect a STALL

**On the user's live ladder** (2026-08-23, 5-point SPXW strikes):

    KING  7710   -100%
    GK    7700    -79%   ratio 0.79 vs the King -> PASSABLE, below the 1.8x that says stall
    RUG   none
    RRUG  none

⚠ **AND THE NEAR-MISS IS THE INTERESTING PART.** 7650 (+41) sits over 7630 (−85) — a textbook rug pair,
both well past the 40% anchor, with no significant positive floor beneath. It does not fire because they
are **FOUR ladder steps apart and RUG_ADJ is 3.** A detector that fired there would be loosening the
doctrine to manufacture a hit. It stays at 3, and the near-miss is recorded here instead.

**ROLE BEATS POLARITY ON THE LABEL.** A King is still an accelerator; "KING" is the word that changes what
you do and "ACC" is not. The hover keeps both, and explains the role: what a gatekeeper's ratio means,
what a rug does when the cap unwinds.

⚠ **A REAL BUG THE TESTS CAUGHT: the reverse-rug pass OVERWROTE the rug tag.** A purple-yellow-purple stack
satisfies BOTH shapes — the middle node is a rug's ceiling AND a reverse rug's floor — and the second pass
silently relabelled it, **asserting the opposite direction on the same level.** Tags are no longer
overwritten, and a stack that is both is flagged `contested`, because two opposing shapes around one node
is itself a reading and hiding one of them would be the dishonest option.

⚠ Also removed: an empty `for(){}` loop left in the reverse-rug scan from drafting. It did nothing, and
dead code that looks deliberate is how a real loop goes missing later.

## v11.80 — the target distance was the one price still carrying decimals

`T: 7718+16.37` — two decimals on a DISTANCE, in a section where v11.75 made every other price a whole ES
point, and no space before it. False precision on top of an inconsistency. Now `T: 7718 +16`.

**VERIFIED LIVE FIRST, WHICH IS THE WHOLE POINT.** v11.79 was confirmed running on the tab before anything
was reported. Piles `src:"skylit"`, King **7710** via `kingSrc:"dollar"`, `kingKd` 17241, and the three
nodes on the rail:

    7710 -> 7727.7   100%  ACC     the King the user could see on the tape
    7700 -> 7717.7    79%  ACC
    7650 -> 7667.6    41%  BRK     yellow

Labels rendering ES-over-SPXW. Sentence: *"79% negative gamma accelerator at 7718 can take price higher to
the 100% negative gamma node at 7728."* The chain names the King as the destination.

⚠ **The regime chip read `—` on the FIRST sample and `−G −V ⚠` on the next five.** A single snapshot of a
live face is not evidence — the rule held, and re-reading before reporting avoided a second false alarm in
one session.

## v11.79 — the labels I was asked for twice, mocked twice, and did not build

**Owning this one: the user asked for ES-over-SPXW labels two sessions running. I produced two mockups and
shipped neither.** They are built now.

    7661            ▮              ▮      ▮          7730
    ~EXP LOW      7668           7718   7728       ~EXP HIGH
                7650 BRK      7700 ACC 7710 ACC

**ES price on top — the number that is traded — SPXW strike and the node's role beneath it.** Labelled only
above `PLAB_MIN_PCT` (⚖ 20, the same cut as the node itself) so a busy expiry cannot smear the tier.

**THE HOVER SPEAKS THE VOCABULARY OF WHICHEVER BOOK PRODUCED THE NODE.** Skylit nodes read `%King` with the
dollar figure stated as a VALUE; the InsiderFinance fallback keeps `$/PT`, and says it is the fallback.
⚠ Printing Skylit's node value with "/PT" would silently swap units, so a test forbids it.

**THE RAILS QUOTE THEIR INDEX EQUIVALENT.** The band has always been in ES — 7661 and 7730 are chart
prices — but nothing said so, and the strikes behind them are SPX. Both hovers now add *"This is an ES
price; the index equivalent is SPX 7643.4"*, via a named `ifDispScale()` inverse.

**AND THE TARGET CARRIES ITS DISTANCE.** `T: 7718 +6`. "Is it close" needs the GAP, not two prices the
reader subtracts — and the gap is the number that moves.

⚠ **GEOMETRY CONTRACT EXTENDED TO FOUR TIERS, AND MEASURED RATHER THAN GUESSED.** At 42px the piles
occupied rows 22-32 and the two-line labels 28-43 — **four rows of overlap.** The labels did not move; the
piles were LIFTED and the box grew to 48. Verified in a real render: 0 pile-over-label, 0 label-over-label.

    0-9    money labels     14-18  the rail
    19-29  the gamma piles  34-48  the node labels

⚠ **THREE WORDINGS FOR ONE CAVEAT.** "no option chain contains" / "no option chain carries" / "the chain
does not contain" were all live at once for the same standing rule. **A rule with three phrasings is on its
way to becoming a suggestion.** Normalised, and a test now requires it to read identically on every branch.

⚠ **A TEST THAT COULD NOT MATCH.** The geometry checks built their pattern through python → JS → regex and
the nested escaping produced a regex that never matched — `px()` returned `null` and every geometry
assertion passed vacuously. Same family as the `/'[^']*'/` failure of v11.70. **Build patterns from pieces,
and assert the extractor found something before asserting on what it found.**

---

⚠⚠ **AND A PROCESS FAILURE THAT COST THE USER TIME.** They reported 7710 still missing from the rail. It
was not the code: **they were running v11.76**, and the Skylit switch shipped in v11.77. I had shipped
three installers in a row without once confirming any of them reached the browser, and two earlier builds
(11.68, 11.72) never got their own commits either. The tape was being read perfectly the whole time —
`count 100, king 7710, kingKd 17241, kingSrc "dollar"`, and 7710 → ES 7727.73, inside the band.

**FROM NOW: verify the RUNNING version on the live tab before reporting any build as done.** A build that
is not installed is not shipped, and only the browser can say which it is.

## v11.78 — prove the King before trusting anything measured against it

**THE USER'S REQUIREMENT: survive Skylit changing their markup.** The right place to defend that is the
King, and the reason is worth stating precisely.

**Every %King on the SPXW tape is a RATIO to the King.** Get the King wrong and all 100 strikes are wrong
TOGETHER, in the same direction, by the same factor — and the rail still looks entirely plausible. Nothing
about it reads as broken. That is the worst failure available on this path, and it is exactly the one a
markup change produces.

**Skylit's signed `$K` cell is the strongest fingerprint they publish**, because it is DATA rather than
markup: it survives restyling, class renames, container swaps and table-to-div rewrites. `readTapeFromDOM`
has preferred it since v11.2 (`kingSrc:'dollar'`). **What was missing was checking it.** Now, three gates:

    1. the dollar anchor exists at all
    2. the percent ladder INDEPENDENTLY crowns the same strike  (|pct| == 100)
    3. the reader is not already flagging a conflict between its own two methods

⚠ **GATE 2 IS THE ONE THAT MATTERS.** Two methods, one answer required. If the $-anchor says 7710 and the
ratios say 7700, the ladder is being misread and **every level drawn from it would be wrong by the same
ratio while looking completely normal.** It now refuses and names both answers.

**A LOST DOLLAR ANCHOR DEGRADES, IT DOES NOT FAIL.** If the $K cell disappears but the ratios stay
self-consistent, the nodes still render — the ratios are what sizes them — and the sentence prints
*"⚠ no King $ value on the tape — ratios only, dollar anchor lost."* That is the early warning that the
strongest fingerprint has gone and the NEXT markup change may not be survivable.

**Nine simulated tape shapes, each naming itself:**

    healthy tape                        3 nodes
    tape gone (markup change)           SPXW tape unreadable
    thin tape (partial parse)           SPXW tape thin (4 strikes)
    no King                             no King on the SPXW tape
    $ says 7710, ratios say 7700        King disagrees: $-anchor says 7710, ratios say 7700
    nothing reaches 100%                no strike reaches 100% of King (top 64%)
    reader flags a conflict             the tape reader flags a King conflict
    dollar anchor lost                  3 nodes  ⚠ ratios only, dollar anchor lost
    no strike map                       SPXW tape has no strike map

**And the provenance is one hook call away.** `__gptsDebug.piles()` now returns `king`, `kingSrc`,
`kingKd` and `degraded`. ⚠ **If `kingSrc` ever stops saying `'dollar'`, the strongest fingerprint is gone**
— that is the thing to check first when levels look wrong.

⚠ **Every gate mutation-tested.** Removing the cross-check fires 3 assertions (including the silent
catastrophe itself), ignoring the conflict flag fires 4, making a lost anchor fatal fires 5, dropping the
100% requirement fires 4.

## v11.77 — the nodes come from Skylit, and a failure to read can no longer look like a clear path

**THE USER'S ARCHITECTURE, AND IT WAS RIGHT: InsiderFinance prices the DAY, Skylit marks the LEVELS.**
Both were coming from IF, which is why a King the user could plainly see on the SPXW tape at 7710 was
missing from the rail entirely.

⚠ **NOT A SCALING FIX AND NOT A BUG.** SPX 7710 in ALL THREE IF windows: `dte0` +$88M (9% of King),
`toFri` +$219M (11%), `all` +$270M (4%) — **positive and small in every one.** Skylit calls the same
strike its **King at −100%**. I ruled out a window artefact by checking all three. They measure different
things, and this file already said so above `ifChain`: *"live dealer positioning that accumulates and
dissipates intraday ... This is open interest x gamma: where exposure SITS. A stock beside a flow."*
**The trader watches the flow. The rail now draws the flow.**

**AND IT PUT THE BRAKES BACK.** Under IF every in-band node was negative — four purple piles, no yellow,
nothing structural leaning against a move in either direction. Skylit's book shows **+41% at 7650** and
three more above the noise. The "hurdles" half of the question was missing because the wrong book was
answering it.

    7710  −100%  $17.2M     the King the user saw
    7700   −79%  $13.6M
    7650   +41%   $7.1M     a BRAKE
    7690   −17%      7680  +13%      7670  +12%      7640  +9%

**THE GROSS-VS-NET TRAP IS NOW STRUCTURALLY IMPOSSIBLE.** Skylit gives ONE signed number per strike, so
there is no second magnitude to size by. The 84x overstatement of v11.68 cannot recur on this path.

⚠ **UNITS CHANGED AND THE SENTENCE CHANGED WITH THEM.** IF's figure was hedging per POINT; Skylit's is the
node's VALUE. Same-looking number, different meaning. **%King is now the size** — what the tape shows —
and the dollar value is derived (`pct/100 x kingKd`) and lives in the hover. Printing Skylit dollars with
"/PT" would have been the worst kind of wrong, so a test forbids a `$` in a Skylit node sentence.

---

**⚠⚠ FAILURE CAN NO LONGER LOOK LIKE AN EMPTY BAND — DECISIONS.md D-6 IS CLOSED.**

`"Nothing sizeable between 7708 and 7730"` is a claim about the MARKET. It was being made from a failure
to read the DATA: six distinct failures returned an empty array and an empty array rendered as a clear
path. Now every refusal carries its reason and the sentence prints it:

    No node book right now — SPXW tape unreadable. This is not a clear path, it is no reading.
    No node book right now — SPXW tape thin (3 strikes). This is not a clear path, it is no reading.

**A Skylit markup change lands in exactly that second message.** `SK_MIN_STRIKES` (⚖ 20; a healthy ladder
reads 100) catches a degraded read before it can masquerade as a quiet market.

**AND THE FALLBACK ANNOUNCES ITSELF.** If the SPXW tape cannot be read at all we would rather draw IF's
static book than draw nothing — but the units and the meaning both change, so the sentence says so:
*"⚠ Skylit tape unreadable — these are InsiderFinance open-interest levels, not live positioning."*
**A rail that silently changes what it measures is worse than one that admits it.**

Also closed from the same audit: a non-finite price now refuses (`"Price or band not readable right now"`)
instead of rendering `"between NaN and 7730"`, and two nodes rounding to one whole point can no longer
become each other's destination.

⚠ **WHAT THIS COSTS, STATED PLAINLY.** SPXW has **no JSON feed** — `feedShape('SPXW')` returns *"no feed
captured yet"*. The nodes are read from the rendered tape table. That reader has already adapted to four
Skylit markup changes (v10.44, v10.47, v11.1.3), validates per ROW, rejects the flow popup and the heatmap
by fingerprint, and falls back through the gamma feed and a stale cache before giving up — but it IS the
fragile edge, and it is now load-bearing for the levels. **Every guard above was mutation-tested:**
removing the thin-tape floor fires 3, collapsing the noBook branch fires 4, dropping the fallback
disclosure fires 3, pointing the reader at SPY fires 2, letting NaN through fires 3.

## v11.76 — documentation, because the code has been lying to its own author

**A COMMENT ASSERTING A SAFETY GUARANTEE IT NO LONGER PROVIDES IS WORSE THAN NO COMMENT.** The block above
`emPiles` said:

    Read from SKYLIT's book (cpRows), which is the SAME source regime2D reads
    — so a pile can never contradict the regime chip above it.

**Both halves have been false since v11.64.** The piles read InsiderFinance. The guarantee does not exist.
It sat there for **eleven versions** and misled two separate contexts — including me, twice, in one
session. Corrected, and `test_em_band.js` §34 now reads what `emPiles` ACTUALLY calls and fails the build
if the comment names a different book. Mutation-tested: restoring the old claim fires three assertions.

**`session-state/INSIDERFINANCE.md` — THE SECOND BOOK, WRITTEN DOWN.** Verified against the live page:
the Next.js payload shape, the eleven fields on an option row, all 55 expiries, the one GEX formula and
the reconciliation that licenses recomputing it (`ours call +263.83B / put −250.49B` against their
published `$263.8B / −$250.5B`), the three windows and the Friday roll, what we scrape from their header
versus what we recompute, the companion's `gexProf` and its ~5% trim, the full source-by-source map of
every element on ① FRAME, and the failure modes with how to confirm each.

⚠ **AND A FINDING FROM WRITING IT: `dte0` IS NOT ALWAYS TODAY.** InsiderFinance drops an expiry from the
payload once it has expired. Captured after the Friday close, the chain's earliest expiry is **Monday**:

    payload ts   2026-08-21T20:04:55Z   (after the 16:00 ET close)
    today        20260821
    earliest     20260824               <- Friday's chain already gone
    dte0 selects 20260824               dte0_isToday = FALSE

So on a weekend replay of Friday, the band's "today's expected move" is **Monday's** straddle over Friday's
price action. The arithmetic is right; the label over-promises. Recorded as D-5, not yet on the face.

**`session-state/DECISIONS.md` — NINE DECISIONS, INCLUDING THE ONES I GOT WRONG.** The wrong versions are
the valuable entries, because the wrong version is the one that looks reasonable:
- **D-3** the two reasons I moved the piles off Skylit at v11.64 were BOTH flawed. The "113× magnitude
  difference" is **spot² scaling — (7674/765)² = 100.6×** — plus a wider window, never incompatibility.
  And "Skylit carries no polarity" was me reading the unsigned magnitude fields while a signed `net` sat
  right there. A third claim, made in a mockup this session, compared Skylit's **SPY** ladder against IF's
  **SPX** book and announced a 16-point disagreement; Skylit's **SPXW** King is 7710, one strike from IF's
  7700.
- **D-4** one chip from one book and everything else from the other, unresolved, with the three ways out.
- **D-6** the false all-clear, still open.

**The skill's LOAD now names both books first.** New steps 1b and 1c put DECISIONS and INSIDERFINANCE
ahead of the code, and the companion script is now a full-file read rather than an afterthought.
`.gex-config.json` carries an `insiderFinance` block — payload path, row fields, formula, windows — so a
context that reads only the config still learns the second book exists.

⚠ **THE SINGLE MOST REPEATED ERROR IN THIS PROJECT, now stated at the top of LOAD:** comparing a number
from one book, window or scale against a number from another and declaring a discrepancy. It has produced
a phantom netGEX sign bug, a phantom regime-contradicts-FLIP, a wrong incompatibility verdict, and a wrong
King disagreement. **Establish book, window and scale BEFORE comparing.**

## v11.75 — the section is three rows, and it answers one question

**LINE 3 IS DELETED.** It carried `47% USED · 18.35 LEFT ↑ · ↑ clear — 5.6 pts · $17M at target · ▮ REPLAY`
and every part of it was said better somewhere else: the rail DRAWS position and remaining room as a
picture, and the sentence below now names what is ahead and where it leads. Four rows became three.
⚠ The one thing that mattered and left with it — `~EST`, which flags a band captured late and therefore
narrower than the open's would have been — moved onto the rail labels as a leading tilde. **A caveat that
only lives in a hover is a caveat nobody reads.**

**THE SENTENCE STARTS AT THE LEVEL AND ENDS AT WHERE PRICE CAN GET TO.** It used to open "Up 16.38, 47% of
the straddle but slow for the hour." The user's word for that was nonsense and they were right: the rail
above draws every one of those facts, so the sentence spent its first eight words reading the graph aloud.

    $6M negative gamma accelerator at 7668 can take price lower to the $6M positive gamma node at 7665.
    $17M negative gamma accelerator at 7718 can take price higher to 7730, with nothing else in the way.
    $31M positive gamma brake at 7727 can stop price there.
    Flip at 7666, 8.6 below. Through it hedging flips from amplifying to damping.

Polarity reads **negative / positive gamma**, not short / long. The destination is the NEXT node beyond the
level, falling back to the rail when nothing lies past it. ⚠ Still "can", never "will".

**ES PRICES ARE WHOLE POINTS.** `7730.48` and `7661.02` are not prices anyone can trade — ES moves in
quarter points, so those decimals were noise wearing precision. `frameNum()` rounds to the nearest whole
point **only on a futures chart**; `dispNum` is untouched everywhere else, and on a SPY chart (~764) whole
points would be far too coarse.

**Row 1 reordered.** BREAKS moved next to the regime chip it restates — it was sitting four chips away at
the far end. The target took that end instead: **`T: 7718`, right-aligned, GREEN above price and RED below**,
and the T on the rail reads the SAME test, so the chip and the mark can never disagree. They were cyan and
gold before, which said nothing about direction and did not match each other. The contract chip went 7px →
8px; it was a full step below every other chip and read as a footnote.

**Rails reverted to EXP LOW / EXP HIGH** at the user's instruction. The 0.80σ caveat did not go with the
label — it stays in the hover, because the label is what they want to read and the arithmetic is what they
need to be able to check.

**THE REPLAY BADGE IS REMOVED**, at the user's instruction: *"i know its sunday so i know that i will see
friday already."*
⚠ **THE RISK THIS ACCEPTS IS MONDAY 08:00, NOT SUNDAY.** Pre-open, replay engages, the whole panel shows
Friday, and nothing on the face says so. v11.55 called this "the one label whose absence would let a whole
stale face read as live" and that reasoning has NOT changed — only the decision has. `test_last_session`
GUARD 3 now asserts the ABSENCE and documents why, and the mode is still one `__gptsDebug.session()` call
away. If it is ever wanted back, the least intrusive home is the section header: `① FRAME · 08/21`.

---

⚠ **I INSERTED THE TARGET BLOCK INTO THE WRONG FUNCTION.** The anchor I matched on was `h+='</div>';` and
the scorecard renderer ends with the same line, so the chip landed there — syntactically valid, silently
absent from the face. **`node --check` passing is not evidence that code went where you meant.** Caught
because the test asserted the target renders inside `secFrame`, not merely that it exists.

⚠ **DELETING THE LAST TEST SECTION TOOK THE SUMMARY LINE WITH IT.** The section-bounds helper returns
end-of-file for the final section, so removing §33 removed `console.log(pass/fail)` — and the suite then
reported nothing at all rather than reporting failure. **A test file that prints no verdict must be treated
as a failing one.**

⚠ **`indexOf('__gptsDebug.session')` MATCHED `sessionRoll` AGAIN** — fourth time. It is in the resume note
and I still wrote it. Use the full marker `__gptsDebug.session = function`.

## v11.74 — the polarity is an adjective, not a follow-up clause

**The user's wording again, and again it is better.**

    was   $17M accelerator at 7717.71 — short gamma there, so a hedge can push price higher.
    now   $17M short gamma accelerator at 7717.71. A hedge there can push price higher.

The old line classifies the level, then **doubles back** to say what kind of level it was, then explains —
three moves where one would do, with the reader holding a comma-clause open across a dash the whole way.
Front-loading the polarity classifies **once**, and the consequence becomes its own short sentence instead
of a subordinate clause. Two words shorter, and it reads at a glance instead of in sequence.

Applied to every branch, so the section has one grammar rather than one per case:

    $17M short gamma accelerator at 7717.71.  A hedge there can push price higher.
    $6M short gamma accelerator at 7667.59.   A hedge there can take price lower.
    $31M long gamma brake at 7727.            A hedge there can cap it.
    $31M long gamma brake at 7670.            A hedge there can lift it.
    Flip 8.64 below at 7665.56.               Through it hedging flips from amplifying to damping.
    Air to 7730.48.                           Nothing in between to lean on.
    7717.71 above is BALANCED.                Size with no side to lean on.
    Through the band with nothing above.      Out here the book stops resisting.

Longest sentence 26 words → **24**.

⚠ **"CAN" IS STILL THE ONLY THING HOLDING THE LINE.** Mutation-tested again after the rewrite: replacing
`A hedge there can` with `Price will go there` fires four assertions; dropping the polarity adjective
fires two. The wording changed, the contract did not.

## v11.73 — the row showed what was spent and never what was left

**BOTH OF THE USER'S POINTS WERE RIGHT, AND THE SECOND ONE BROKE A RULE I WROTE MYSELF.**

**1. REMAINING WAS MISSING.** Line 3 printed `53% OF STRAD` — how much of the straddle had been SPENT —
and nothing anywhere said what was LEFT, which is the half the decision actually turns on. The rail's four
dollar segments were supposed to carry it, except **a segment narrower than 9% of the rail is suppressed**,
so on a day that has run to one side only two of the four render and "remaining" disappears from the
section entirely. Measured live 2026-08-23: **53% used, 16.49 points and $825 still ahead, and not one
element on the face said so.**

    was   53%  OF STRAD ~EST   0.77x COILED   ↑ clear — 3.7 pts · $17M at target
    now   53%  USED ~EST   16.49  LEFT ↑      ↑ clear — 3.7 pts · $17M at target

Room is reported in the DIRECTION OF TRAVEL because that is the side the question is about; both sides
stay in the hover, in points and in dollars per contract.

**2. THE PACE CHIP WAS THE SAME FACT TWICE.** v11.69 wrote down *nothing is printed twice on the section* —
and then v11.68's chip said `0.77x COILED` while the read line two rows below said **"but slow for the
hour"** about the same number, in better words, for free. The chip is gone and the space went to
`LEFT`. **The pace is not lost:** it still gates that clause of the sentence, and the arithmetic behind it
(what fraction of the move is due by this hour) is in the percentage hover. The read line also gave up its
own `, X to the rail` clause for the same reason — line 3 owns remaining now.

---

⚠ **A FIXED +3000 CHARACTER SLICE, AND IT IS THE THIRD OF THESE IN TWO DAYS.** The v11.51 hook test read
`src.slice(indexOf(hook), indexOf(hook)+3000)`. This build grew the hook past 3000 characters, so
`targetInPlay` fell off the end and an assertion failed on a field that was present and correct. Same
disease as the 40-record dedupe scan and the `session`/`sessionRoll` marker: **a magic-number window that
silently stops covering what it is meant to cover.** Now bounded by the NEXT hook, which cannot drift.

⚠ **THE SELF-DERIVING HOOK GUARD EARNED ITSELF ON THE VERY NEXT BUILD.** §32 shipped in v11.71 and caught
`dir` here — a field the face started reading and the hook did not return — before it cost anything.

⚠ **A COMMENT EXPLAINING A REMOVAL CONTAINS THE THING REMOVED.** Asserting `!/COILED/` over `secFrame`
failed on the comment that explains why COILED went away. Strip comments before asserting an absence.
**Third time this session** — it is now in the resume note as a standing rule rather than a lesson.

⚠ **A MUTATION TEST THAT FOUND MY TEST WANTING.** Replacing the LEFT figure's gate with `if(false)` left
every string in place and fired **nothing** — the assertions were checking source presence, not behaviour,
and `secFrame` cannot be executed without the whole closure. The gate is now asserted explicitly and the
real check is the offline render, which is run every build and is in the repo as
`mockups/frame_v1173_check.html`. **Structural assertions must say so out loud.**

## v11.72 — say what the hedge DOES to price, not what the hedging IS

**The user's own wording, and it is better than mine:**

    was   $6M accelerator at 7707.69 — short gamma there, so a push through is hedged WITH it;
                                        nothing behind it before 7730.48.
    now   $6M accelerator at 7707.69 — short gamma there, so a hedge can take price lower.

"A push through is hedged WITH it" describes the **mechanic** and leaves the reader to finish the thought.
"A hedge can take price lower" **is** the thought. The pile ahead is by construction in the direction of
travel, so the consequence has a direction and there was never a reason to make the reader supply it.

    accelerator, moving up     short gamma there, so a hedge can push price higher
    accelerator, moving down   short gamma there, so a hedge can take price lower
    brake, moving up           long gamma there, so a hedge can cap it
    brake, moving down         long gamma there, so a hedge can lift it

⚠ **"CAN", NEVER "WILL" — that word is the entire licence for the line to exist.** It says what the book
DOES if price reaches the level, not that price reaches it. Mutation-tested: swapping in "so price will
push higher" fires four assertions; deleting the direction fires two.

**The trailing clause is gone.** `; nothing behind it before 7730.48` restated the rail that is printed at
the end of the band one row above. Fluff, correctly called.

**And a give-back past 100% now reads as English.** "turned once and 109% back" is arithmetically right and
takes a beat to parse; it means the whole excursion came back and then some, so it says
**"turned once and fully retraced"**.

Every branch re-voiced and shortened — longest sentence 30 words → **26**.

⚠ **A FIXTURE SET THAT NEVER WENT DOWN.** The new assertions check both directional forms, and the seven
stub states they run against were all `dir:1` — so "take price lower" and "lift it" could never appear and
two assertions failed on code that was correct. Fixtures must span the axis the assertion is about, or the
test is checking the fixture rather than the function.

## v11.71 — I broke the v11.51 rule again, so the guard now derives itself

**THE PACE FIELDS WERE ON THE FACE AND NOT IN THE HOOK.** v11.68 added `pace`, `elapsed`, `dueFrac`,
`paceOk` and `nowSo` to `emBand()` and to the rendered chip — and not to `__gptsDebug.emBand`, whose
return object is a hand-maintained field list. So verifying the chip live meant reading two DOM strings
and inverting the arithmetic to recover elapsed:

    chip 0.59x, pct 39%  ->  elapsed = (0.39/0.59)^2 = 0.437  ->  11:20 CT

That is exactly the reconstruction the v11.51 rule exists to prevent, and I did it **twice in one session**
before noticing the hook was the problem rather than my patience.

**THE GUARD NO LONGER TRUSTS ANYONE TO UPDATE A LIST.** `test_em_band.js` §32 extracts every `EB.<field>`
that `secFrame` actually reads, extracts every `B.<field>` the hook actually returns, and fails on the
difference. It found three; it will find the next one without anyone remembering it exists.

⚠ **The same shape of bug is available anywhere a debug hook enumerates fields by hand.** `piles`, `flow`
and `session` all do. If one of them drifts, the symptom is not an error — it is a field that reads
`undefined` in the console while rendering perfectly on the face, which looks like the FEATURE is broken
rather than the instrument.

---

**VERIFIED LIVE ON v11.70 BEFORE THIS FIX** (replay of 2026-08-21, clock at 11:20 CT):

    piles window          dte0                          the band's own book
    7700  net -$1,314M    netFrac 39.5%   ->  $17M      was $107M gross under 11.67
    7650  net   -$444M    netFrac 35.8%   ->   $6M      was $59M on a 1.2% residual
    path                  clear - 8.6 pts, $17M at target
    read                  "Up 13.35, turned once and 52% back. $17M accelerator at 7717.71 -
                           short gamma there, so a push through is hedged WITH it; nothing
                           behind it before 7730.48."

The window fix, the net fix, the strict path, the pace chip and the read line are all doing exactly what
they were built to do, on live InsiderFinance data, with `renderErrors: []`.

⚠ **The Tampermonkey update showed RE-INSTALL rather than UPDATE.** The push and the CDN were both fine —
GitHub and raw.githubusercontent were serving 11.70 while the tab ran 11.67 — so the userscript manager
simply had not taken the new version. Re-installing fixed it. Worth watching: if it recurs, the script's
`@version` line uses irregular whitespace (`// @version    11.71`) compared with the other metadata keys,
which is legal but is the first thing to normalise if TM's update check keeps missing.

## v11.70 — the read, and a duplicate-record bug that had been one feature away for months

**THE READ.** One line under the band that composes the section into a mechanism. Two clauses: where the
day is, then the next level that changes how hedging behaves and what it changes to.

    Up 19.4, 56% of the straddle. $17M accelerator at 7717.71 — short gamma there,
    so a push through is hedged WITH it; nothing behind it before 7730.48.

    Down 21.55, 62% of the straddle. Flip 8.6 away at 7665.56 — through it hedging
    stops amplifying and starts damping, so the character changes there, not the direction.

**FLIP OUTRANKS EVERYTHING WITHIN 12 POINTS**, because it is the one level where the mechanism INVERTS
rather than strengthening or weakening. Branches: `flip` · `accel` · `brake` · `balanced` · `past` · `air`.
It speaks on quiet tape too — a blank line reads as a broken one.

⚠ **IT IS A MECHANISM, NEVER A FORECAST.** No *likely*, no *will*, no *should*, no probability, no trade.
Not caution for its own sake: every scorecard here is still at zero, gamma says HOW price moves and never
WHICH WAY, and "likely to reach 7730" would be inventing the market-impact coefficient no option chain
contains — while looking quantitative doing it. The ban is TESTED, and the test bans forecast and
instruction, NOT vocabulary: *"they sell strength and buy weakness into it"* stays, because that is what
dealers do at a long-gamma strike and deleting the verb would delete the explanation.

⚠ **THE BAN TEST DID NOT WORK, AND MUTATION TESTING IS THE ONLY REASON I KNOW.** It read the source and
pulled quoted strings with `/'[^']*'/`. That desynchronises on the first apostrophe inside a COMMENT —
"InsiderFinance's published Zero Gamma" — after which every captured string is mis-paired and the joined
text is garbage. **Inserting "so it will likely continue" passed all eleven assertions.** The check now
EXECUTES `emRead` against seven stub states and reads the sentences it actually emits. Same for flip
precedence: the old assertion compared source positions and passed even with the whole branch disabled by
`if(false && flipNear)`.

---

**AND A REAL BUG IN LIVE CODE, FOUND BY AN EXISTING TEST.** `featEnqueue` scanned back a hardcoded **40
records** looking for a duplicate. One bar writes **one record per enrolled feature** — so the moment the
registry reached 40, the look-back could no longer span a single bar and re-enqueueing the same bar
duplicated **every record**. featEnqueue runs repeatedly per bar, so live this would have doubled and
re-doubled the exact data every scorecard is computed from, silently, and worse the more features existed.

    39 features   look-back 40   spans the bar    fine
    40 features   look-back 40   does NOT span it  every record duplicates

**This build added the 40th feature.** `test_feature_enrollment` 9d caught it on the first run. Records for
one bar are contiguous at the tail, so the scan now walks back WHILE the bar matches and stops at the first
record from an earlier one — correct for any number of features, forever. Verified behaviourally at 60.

## v11.69 — the row said the same thing three times

**"momentum — breaks not fades · widen stops".** Forty-three characters of prose on the busiest row of the
panel, sitting next to a chip that already read **−G −V ⚠**. Three restatements of one fact: *momentum* and
*breaks not fades* are the same claim, and *widen stops* is its consequence. **The playbook is BREAKS or
FADES. Everything else about it is a sentence, and sentences live in hovers** — where the widen-stops
advice now is, along with the compounding-vanna warning and the standing "HOW price moves, never WHICH WAY".

**Line 3 was printing the destination that line 1 already showed.** `path ↑ to 7717.71` directly beneath a
chip reading `→ 7717.71`. The arrow carries the direction; the number did not need saying twice. A test now
counts `dispNum(ifMagEarly)` and fails if the target renders more than **once** in the whole section.

**And the contract chip still said EM.** After v11.68 renamed the row STRADDLE it was the one place left on
the face calling a 0.80σ band an expected move. Now `ES $1,736/ct` — the symbol and the money, nothing else.

    before   −G −V ⚠   → 7717.71   FEEDS $214 M / PT   ES · EM $1,736/ct   momentum — breaks not fades · widen stops
    after    −G −V ⚠   → 7717.71   FEEDS $214 M / PT   ES $1,736/ct        BREAKS

⚠ **A GUARD THAT SEARCHED ITS OWN EXPLANATION.** The first version asserted `pins hold` had gone from
`regime2D` — and it fails, because `out.why` in that same function still carries the full sentences and
MUST: those are the hover text. The assertion now extracts the `out.play=` lines and checks only those,
and a companion assertion checks the why-text still HAS them. **Trimmed, not deleted, is the whole point
of this build, so the test has to be able to tell the difference.**

## v11.68 — the piles were reading a different book than the band they were drawn on

**TWO FAULTS COMPOSING INTO ONE CONFIDENT WRONG NUMBER.** The band asks *how much room does TODAY have*
and correctly reads the 0DTE straddle. The piles then answered *what is in the way* from `toFri` — and on
a **Friday** the roll makes that window today PLUS AN ENTIRE EXTRA WEEK. Measured live on 2026-08-21:
**only 29.2% of the toFri gross gamma expired that day.** Seven tenths of the obstacles drawn on today's
band belonged to other days.

Separately, `perPt` was computed from **gross** (|call| + |put|) while ACCELERATOR/BRAKE was decided by
**net** sign — so a strike could carry the dollar weight of its whole book and the direction of a rounding
error. Overstatement measured at every in-band strike: 7700 **3.4×**, 7675 **3.0×**, 7690 **1.5×**, and
7650 by **EIGHTY-FOUR TIMES**.

**The two faults hid inside each other.** At 7650, today's book was decisively short gamma (net −$443.8M
on $1,240M gross, 35.8% surviving) and next week's was long. Blended, they cancelled to a **1.2% residual**
— and the face reported that residual as a **$59M ACCELERATOR**. Fixing either one alone still leaves it
wrong; the window fix makes the strike honest, the net fix makes its size honest.

    GROSS  -> pile HEIGHT and the 20% cut. Skylit's own node convention, so the rail keeps agreeing
              with their heatmap.
    |NET|  -> the DOLLARS and the path sums. What the dealer actually has to hedge.
    thin net -> BALANCED. Drawn hollow, votes in neither sum. It has size but no side.

**PACE.** The band said HOW FAR and nothing said whether that was a lot FOR THIS HOUR. Price diffuses with
√T, so half the clock elapsed means **71%** of the move is due, not 50%:

    10:00   8% of the clock ->  28% due  ->  40% used reads STRETCHED
    12:45  50% of the clock ->  71% due  ->  40% used reads COILED
    14:30  77% of the clock ->  88% due  ->  40% used reads dead

The face printed all three identically, which made its headline number close to uninterpretable unless the
reader silently did the division. `pct ÷ √elapsed`, no new data, floored below 4% elapsed because √elapsed
explodes in the first bars.

**THE ROW IS NOW NAMED FOR WHAT IT IS.** The ATM straddle is ~0.80σ, not 1.00 — a row labelled EM implies
~68% containment and this band delivers ~58%. **The width is unchanged and every level sits exactly where
it did**; only the claim was corrected. STRAD LOW / STRAD HIGH / OF STRADDLE, with the ×1.25 conversion in
the hover for anyone who wants the real one-sigma boundary.

**Also:** the target no longer sits inside its own path sum (it IS the heaviest strike, so `clear` was
nearly unreachable — measured, 82% of one verdict was the destination); `$/pt` divides by 1% in CHART
points rather than SPX points; and our recomputed max pain is now **MP\*** because InsiderFinance publish
one of their own and it is a different number (7350 against our 7712.70).

⚠ **A TERNARY WITH THE SAME VALUE ON BOTH SIDES.** The path hover named its window via
`(PA.nAcc+PA.nBrk)?'toFri':'toFri'` — it said toFri unconditionally and would have gone on saying toFri
after the piles moved to dte0. It now reads the window back off the piles themselves.

⚠ **`var` HOISTS THE BINDING, NOT THE VALUE.** `SESS_OPEN_SEC` was declared beside the band, ten thousand
lines below `sessPhaseCT`, which now uses it. Any caller running before that assignment would have got
`undefined` and produced NaN silently. Caught because `test_king_analyzer` extracts `sessPhaseCT` alone —
an "artificial" test surfacing a real ordering hazard. Constants moved above their first use.

⚠ **THE SEED IS THE SOURCE OF TRUTH FOR RULES, NOT THE JSON.** I added three rules to `learning/rules.json`
by hand; `test_feature_enrollment` correctly refused them, because `rulesSeed()` is built from
`FEATURES[].rule` and the file must match it exactly. Registered `empace` and `piles` as real features —
record, outcome, regime-split questions, rule — and regenerated the file from the seed. **Two rules, not
three: the window and the gross/net halves are one defect and are enrolled as one.**

⚠ **LINE 3 WRAPPED AT FULL WIDTH.** Rendered offline with every optional clause present at once it went to
two rows (31px against 14) — the exact vertical space the last two builds reclaimed. `at target` now prints
only in the CLEAR case where it IS the story, and the balanced count moved to the hover.

⚠ **A TEST THAT COULD NOT FAIL.** One assertion I wrote read `X === false || true`. Removed. Every one of
the four substantive fixes was instead **mutation-tested** — reverted in `v10.js`, the suite run, between
2 and 4 assertions confirmed to fire, then restored.

## v11.67 — the dot's ring was cutting through the money labels, and only sometimes

**FOUND BY RENDERING THE SECTION, NOT BY READING IT.** With the Chrome bridge down I could not check the
live face, so I extracted the panel's REAL stylesheet out of the script, rebuilt the FRAME markup with the
live values measured earlier, and rendered it in headless Chromium at true panel width. The layout change
was fine. Something else was not.

    money labels   top:0, 9px line box        -> rows 0..9
    price dot      top:6, 10px + 2px ring     -> PAINTED rows 4..18
                                                  ^^^^ five rows inside the label band

Whenever price happened to sit near the middle of a spent span, the dot's dark ring cut straight through
that span's figure — **`$1,394` rendered as `$1̶,394`**. It is value-dependent: invisible in any screenshot
where the dot is not on a label, which is why it shipped with the segments at v11.64 and survived every
look since.

**GEOMETRY CONTRACT, WRITTEN DOWN AND TESTED.** The rail moves to `top:14` — the lowest value that puts the
dot's PAINTED top at or below row 9 — and everything hangs off it: dot `rail-3`, notch and T `rail-4`,
piles stay `bottom:2`, box grows 26 -> 31px. `test_em_band.js` §21 parses those numbers back out of the
stylesheet and does the arithmetic, so the next person to nudge the rail gets told.

⚠ **The guard was MUTATION-TESTED** — the old geometry put back, the suite run, the two assertions
confirmed to fire. A guard nobody has seen fail is a guard nobody knows works.

⚠ **`top:0` CARRIES NO UNIT.** The first version of that test matched `(\d+)px`, read `top:0` as absent,
and did its arithmetic on `null` — every assertion passed by not running. Caught only because the same run
also reported a stale `v10.js`.

⚠ **THE TEST HARNESS READS `v10.js`, NOT `current/`.** Two assertions failed on values that were already
correct in the source, because the CSS had been edited after the last `cp`. Step 2 of the checklist exists
for exactly this and I skipped it mid-session. **`cp current/... v10.js` before EVERY test run, not once
per build.**

⚠ Pre-existing and harmless: `.g3emw2` and `.g3shape b.warn` are each defined twice in the stylesheet.
Later wins, both later definitions are the intended ones. Not touched — noting it so the duplicates are not
mistaken for the bug next time.

## v11.66 — the rail was being squeezed by its own restatement

**THE BAND ROW WAS RENDERING FIVE THINGS AND ONLY THREE OF THEM BELONGED THERE.** Low, rail, high — then
the percentage, then the session chip. So the one element on this face that is a MEASUREMENT was laid out
into whatever width was left over after two labels, and the piles, the ticks and the dot all had to fit
in it. **The percentage was the worst of the two, because it is the SAME FACT the dot already draws** —
the rail was giving up space to a restatement of itself.

Both moved to line 3. The band row now holds the low, the rail and the high, and nothing else. Line 3
reads left to right as measurement -> path -> which session, with the session chip pushed to the far
right so it can never reflow the two that carry numbers.

**THE FLOW CHIP BREATHES.** `$214M/pt` is nine glyphs with no gap in them; on a chip at 8px the eye has to
parse it as one token instead of taking three facts at a glance. Now `$214 M / PT`. `usdBigSp` DELEGATES
to `usdBig` and only inserts the space, so there is one rounding rule rather than two — and the hover
sentences keep the tight form, because a space before the unit reads as a typo mid-sentence.

**THE PILES FINALLY HAVE A HOOK.** v11.51 wrote the rule down — every read on the face has one, because
the alternative is counting DOM nodes and inferring from pixels — and the piles shipped at v11.61 without
one anyway. Verifying them this session meant rebuilding `emPiles()` by hand in the console against
`ifChain` + `ifLadder`. `__gptsDebug.piles()` returns the raw legs beside the derived figures: `gross` is
what sizes the pile, `net` is the dealer's actual residual, `netFrac` is how much survives the
cancellation, and `pathStrictly` lists what lies between price and the target WITHOUT counting the target
itself.

⚠ **Those last three fields exist because the hook answers a question the face currently gets wrong** —
see the two open items in the resume note. `perPt` is computed from GROSS gamma while ACCELERATOR/BRAKE
is decided by NET sign, so strike 7650 (call +$2,243M, put −$2,297M, net −$53.8M on $4,540M gross) is
drawn as a decisive accelerator carrying $59M/pt when the net requirement is $0.70M/pt. **Not fixed in
this build — it is a design decision, not a defect to patch quietly.**

⚠ **Two test lessons.** `sed 's/11\.65/...'` silently matched nothing, because the pins are written
`11\.65` WITH the backslash — the suite caught four stale pins that a "successful" sed had left behind.
And a slice ending at `__gptsDebug.session` matched `sessionRoll` ~9,000 lines earlier, came back empty,
and failed five assertions on a hook that was present and correct.

## v11.65 / companion v1.13 — my own two fixes were fighting, and the symptom looked identical to the bug

**THE ANCHOR WAS BEING SCALED TWICE.** v11.59 corrected `open`/`now` from live `rr` back to the captured
`rr`. v11.63 then set `open` **from the record** — already at the captured scale — and left v11.59's
correction running immediately after it. So the pinned anchor was multiplied by `rec.rr/rr` a SECOND time,
and drifted with live `rr` all over again.

    stored   openU 764.4076 x rr 10.0676  =  7695.6   correct
    rendered 7709.4 -> 7710.7 -> 7711.43            drifting, again

**The symptom was indistinguishable from the bug the fixes were for**, which is exactly why it survived
two rounds of being called fixed. Three diagnoses of "the moving dot" and the third fix introduced a
fourth cause.

**ONE SCALE, CHOSEN ONCE, APPLIED ONCE.** `useRr` is picked up front; the anchor, `now`, and both water
marks are all built from it. **There is no correction step any more, because there is nothing to correct.**
The pre-scaling line that computed `open=openU*rr` with LIVE rr — immediately overwritten — is gone too:
it was the surface that made double-multiplication possible. A test now counts the multiplications and
fails if `openU` is scaled anywhere but the two branches.

**THE PROFILE TRIM NOW DECLARES WHAT IT COSTS.** The 1% tail cut removes ~5% of a live 780-strike chain's
gross gamma — small, but the changelog said the legs sum "EXACTLY", and on live data they do not. That was
true only for the small fixture the test used. `gexProfCoverage` now reports the kept fraction, so anyone
summing the profile against `callGEX`/`putGEX` can see the gap is the trim rather than a bug.

⚠ **Two lessons, both about my own work.** When a fix does not resolve a symptom, the next fix must be
checked against the previous one — I stacked a correction on top of a pin without asking whether the pin
made the correction wrong. And a test that asserts an exact spelling (`rec.openU * (`) breaks on a
reformat that changes nothing; it now asserts the behaviour and counts the operations instead.

## v11.64 / companion v1.12 — the band answers the trader's three questions, from ONE book

**Where are we going · what gets us there · what stops us.** The rail now answers all three, and the
sanity pass caught a bug that would have made two of the answers meaningless.

**⚠ THE PILES AND THE FLOW CHIP WERE READING DIFFERENT BOOKS.** `emPiles` read `cpRows` — **Skylit's**
feed. `hedgeFlow` read `ifChain` — **InsiderFinance's**. On the same nominal window:

    SKYLIT   60 strikes   call +$0.24B   put +$0.35B   gross  $0.58B   (both legs POSITIVE)
    IF      309 strikes   call +$24.71B  put -$41.12B  gross $65.80B   (puts NEGATIVE)

**~113x apart in magnitude and opposite in sign convention.** The per-strike figures quoted in
conversation ($2,007K/pt at 765) divided Skylit's gamma by IF's spot — two sources in one number. Nothing
on that rail composed, and it would have shipped looking perfectly quantitative.

**The band is already an InsiderFinance construct end to end** — the expected move is their straddle, the
rails follow from it, the target is their heaviest strike, the flow is their book. The piles were the lone
import. They now come from IF too, in the SAME near window as the flow chip, so every number on the rail
sums. Skylit's node map keeps its home in ③ TRADE LOCATION, where it belongs.

**COMPANION v1.12 — the per-strike gamma profile.** `levelsFor()` was already accumulating per-strike
gamma in `byK` and then summing it away. It now keeps the call and put legs separately and exports
`gexProf` — `[strike, call $M, put $M]`, puts negative, their convention. **Verified: the legs sum EXACTLY
to the book's `callGEX`/`putGEX`, and one strike equals `gamma x OI x 100 x spot^2 x 0.01` to the decimal.**
The near-zero tail is trimmed so a 780-strike chain does not bloat every payload, and a test proves what
survives still sums to the whole — so the piles can never lie about the book they came from.

**THE PATH LINE replaces the shape sentence**, which narrated what the rail already draws. It says the
thing the rail cannot: what stands BETWEEN price and the target.

    path ↑ to 7717.71 · $1.4M fuel · $2.0M brake · BRAKED

Every pile between price and target, summed by polarity, in dollars of hedging per point. That is the
breakout read the user asked for — is the way to the target fuelled or defended.

**USED AND REMAINING ARE ON THE RAIL** as four segments, both sides, defined by the day's EXTREMES so they
sit still and only move when a new high or low prints. A retrace does not hand budget back. A segment too
narrow to hold its label is dropped rather than overlapping.

**PILE HOVERS** give the strike's own gamma, its hedging per point, and which way it pushes:
*"767 — $46M of gamma, $599K of hedging per point. NEGATIVE gamma: dealers are short here, so crossing it
they must trade WITH the move — an ACCELERATOR."*

**⛔ WHAT I WILL NOT PUT ON THE FACE: "this node can accelerate us X points."** Converting dollars of
hedging into points of movement needs a market-impact coefficient — how far $1 of forced buying pushes ES —
and **no option chain contains it.** It depends on book depth at that moment. Any number would be invented
and would look quantitative while being so, which is the most dangerous kind of wrong on a trading face.
It IS measurable empirically: `flow.perPoint` records flow against realised range from now on, and once
there is enough the coefficient becomes observed rather than guessed. Both the hovers and the source say
this outright.

⚠ Four tests asserted behaviour deliberately changed here and were UPDATED, not deleted — each replaced by
an assertion of the new intent, so coverage follows the number to its new home rather than evaporating.
One more failed on a comment that legitimately quotes the retired wording; it strips comments now.

## v11.63 — the moving dot, third diagnosis and the correct one

**THE ANCHOR WAS BEING RECOMPUTED FROM A SLIDING WINDOW.** `closedCandles()` returns whatever bars the
chart currently holds, and that array SLIDES. As it slid, `cs[0]` became a LATER bar and the anchor walked
forward with it. Measured live:

    open  7695.75 -> 7711.66 -> 7713.26 -> 7713.71   in minutes, ~18 points
    em    34.73                                       perfectly still — because IT was captured

**The expected move never moved because it was captured once. The open moved because it was not.** The
first two fixes were both real and both minor: v11.59 pinned the SCALE (0.05 of drift), v11.61 added a
schema stamp so that pin actually fired on an existing session. **Neither touched the 18-point problem.**
Pinning the scale fixed 0.05 of an 18-point bug, and I called it done twice.

The open is now captured into the record alongside the expected move, and read FROM the record. That is
what "anchored" means, and it is what should have happened at v11.49.

**SELF-HEAL, ONE DIRECTION ONLY.** `openSo` stores the opening bar's seconds-of-day. If a later render
surfaces an EARLIER bar — the window slid the other way, or the panel started mid-session and the chart
back-filled — the earlier one wins. It can move BACKWARD toward the true open and never forward, which is
exactly the motion that caused the bug. Schema bumped to 3 so records without the open are re-taken.

**AND THE FLOW CHIP READ `$213,827,434/pt`.** Seventeen characters of false precision that blew line 1
apart. Nobody reads a hedging flow to the dollar; they read its ORDER. `usdBig()` gives `$214M`, `$8.3B`,
`$16B` — while contract sizes keep the exact form, because $1,736 per contract IS meaningful to the dollar.

**Two false alarms in the same check, worth recording.** I first read the regime chip as empty (`—`) and
the gamma piles as absent (0), and started diagnosing both. Re-reading showed `−G −V ⚠` and five piles
rendering correctly — the first read had caught a mid-refresh frame. **A single snapshot of a live face is
not evidence; read it twice before calling it a bug.** The genuine bug in that same check was the one I
nearly missed: the band bounds had moved 16 points between two reads seconds apart.

## v11.62 — there was never a sign bug, and the fuel chip is on

**VERIFIED AGAINST THEIR PUBLISHED HEADER, ALL EXPIRIES, TO THE DECIMAL:**

    ours   call +263.83B   put -250.49B   net +13.34B   ratio 1.053
    THEIR  call +263.8B    put -250.5B    net +13.3B    ratio 1.05

**Our GEX computation is correct.** No bug, no sign-convention difference. The "sign gap" I raised was me
comparing our to-Friday slice against their all-expiry total — a mistake in analysis, not in code.

**What is real is that the sign FLIPS with the window**, because the near book and the full book are
different markets:

    dte0    1 expiry    211 strikes   net  -6.86B   NEGATIVE gamma
    toFri   5 expiries  309 strikes   net -16.41B   NEGATIVE gamma
    all    55 expiries  780 strikes   net +13.34B   POSITIVE gamma

Both true. Comparing across windows without saying so produced **two false alarms in one session** — the
phantom sign bug, and a phantom "the regime chip contradicts FLIP". The regime reads the NEAR book; FLIP
is their ALL-EXPIRY zero gamma. Neither was wrong. That is failure pattern #1 committed in analysis
rather than in code, and the cure is identical: **name the window, every time.**

`GEX_WINDOW_LABEL` + `gexWindowNote()` now travel with any GEX-derived figure, and the FLIP row's hover
says outright that it is the all-expiry book and may legitimately disagree with the regime chip.

**THE FUEL CHIP IS ON** — the thing the user asked for two sessions ago. `FEEDS $214M/pt` or
`FIGHTS $214M/pt`: how much underlying dealers must trade per point to stay neutral, from the NEAR book,
because that is where gamma concentrates and where re-hedging actually happens. A 2031 LEAP contributes
almost nothing to today's flow, which is why it deliberately does not use the all-expiry headline.

Negative net gamma means the flow is traded WITH the move, so it feeds it; positive means it fights it.
**Identical dollar figure, opposite meaning — the regime decides the word, not the size.** FEEDS wears the
accelerator purple and FIGHTS the brake yellow, the same grammar as the piles below, so chip and rail
agree without a legend.

**ENROLLED, REGIME-SPLIT, VOTING ON NOTHING.** `flow.perPoint` records the flow, the window it came from,
and how much flow the remaining distance to the rail would require. Two questions scored SEPARATELY:
does a larger flow-per-point precede price actually reaching the expected move when hedging FEEDS, and
does it precede the day staying inside the band when hedging FIGHTS. Pooled, two opposite effects average
to "no edge" from data that contained one. `hit:null` until measured.

The enrolment guard caught the build again — the new rule was not seeded into `learning/rules.json`. It is
now, with full shape parity copied from an established rule rather than hand-written.

Housekeeping: `tools/AUTOPULL-TEST.txt` removed.

## v11.61 — FRAME finished: gamma piles, Skylit's colours, and the real fix for the drifting dot

**THE MOVING DOT — THE ACTUAL CAUSE.** v11.59 pinned the scale so the anchor would stop drifting. It never
fired. A capture written BEFORE v11.59 has no `rr`, the date key still matched, and the new code reused it
happily — so the band went straight back to riding the live scale. Measured on the live panel: open
**7695.86 → 7695.29 in minutes**, both rails with it.

The failure is general and worth stating as a rule: **persisted state written by an older version can
silently disable a newer guard.** No error, no log line, nothing on the face — the only symptom was a dot
that moved, which the user spotted before any test did. The record now carries `EMOPEN_SCHEMA`, and a
capture whose stamp does not match — or which lacks a field this version depends on — is **discarded and
re-taken**, never half-used.

**GAMMA PILES — the fuel and the friction, inside the band.** What actually stands between price and the
expected move. Read from **Skylit's** book via `cpRows`, the same source `regime2D` reads, so a pile can
never contradict the regime chip above it. InsiderFinance's FLIP stays a second opinion from a different
book, labelled as theirs.

**Threshold: `CFG.nodeThresh`** — the ⚙ slider that already exists, default 20% of King. No second cut
enters the codebase. I nearly shipped 10%, which on the live sample admitted **all seven** in-band strikes
(100/30/19/18/14/13/10) — a formality, not a filter, and it would have marked strikes that ③ refuses to
call nodes at all. ⚠ nodeThresh is hand-set (⚖), never measured.

Height is **√magnitude**: linear lets a 100% King flatten a 30% pile into invisibility.

**SKYLIT'S COLOUR CONVENTION, which my own mockup got wrong.** Their doctrine — already documented in this
file — is **purple = put-dominant = negative gamma = ACCELERATOR**, **yellow = call-dominant = positive
gamma = BRAKE**. I had drawn brakes in green. Fixed, and that forced two more moves:

- **The target is now CYAN** (`#4fd1e0`). Yellow means positive gamma now, so the target could not keep
  it. Cyan was the one hue the palette had not spent, and the **T** glyph carries identity without colour.
- **STRETCHED is now RED.** Amber sat beside the brake yellow meaning something different; red already
  means "this is against you" in BIAS, which is what STRETCHED says.

The grammar is now: **purple accelerates · yellow brakes · cyan is where it is trying to go · white is
where it is · red is a warning.**

**THREE TIERS THAT CANNOT COLLIDE.** Money above the rail, ticks and price on it, piles hanging below. The
combined mockup showed the alternative crowding badly — an accelerator four points from a dollar label on
a QUIET day with only two piles. Separated by tier, they can never overlap regardless of how busy the book
gets.

**THE RAW BOOK IS NOW EXPORTED.** `nodes` is what the model chose to call a node — about 6 strikes. The
feed delivers **60** (`nodes=60`, confirmed live; the Skylit `P20` control only changes what THEIR canvas
draws, and no UI setting widens our feed). Throwing 90% away made every threshold question unanswerable:
**a sweep over the surviving 6 measures our own filter, not the market.** `snap.book` now carries
`[strike, callGEX $M, putGEX $M]` for all 60, compactly. ⚠ **Forward-only** — earlier days cannot be
back-filled, which is exactly why it goes in now rather than when the calibration is wanted.

**THE GAMMA/VANNA HOVER, in the user's own words:** *"GAMMA = does dealer hedging fight the move or feed
it. VANNA = does a change in vol add to that or work against it."* Plus the project's own phrasing —
gamma tells you HOW price moves, not WHICH WAY — which is why both gate rather than vote.
⚠ There is no industry aphorism pairing gamma and vanna; the line the user half-remembered was this
codebase's own, and the real pairing is **gamma = how, delta = which way**.

**NOT BUILT, DELIBERATELY:** the `FEEDS/FIGHTS $214M per point` chip. Our `toFri` netGEX is **−$16.41B**
against their published **+$13.30B**, and aggregation differences do not explain a sign flip. A chip
reading FEEDS while hedging is braking is a wrong number with a DIRECTION attached. The piles deliver the
fuel read from a book we trust; the aggregate dollar figure waits for the reconciliation.

⚠ Two tests failed on their own history during this build and both were fixed properly rather than
loosened: one matched `EMOPEN_KEY` and `EM_FRESH_MIN` as an ADJACENT block and broke when the schema
constant landed between them — **a test that depends on the order of unrelated declarations fails on a
change that is none of its business** — and two pinned the old abstract hover wording that was
deliberately replaced.

## v11.60 — the band in dollars per contract

A band in index points makes you do the ×50 in your head every time you want to know what a move is
actually worth. On an ES chart the face now says it outright.

**A contract chip on line 1** — `ES · EM $1,737/ct` — states, once, what a WHOLE expected move is worth on
the instrument being charted. **The shape line's last field switches from points to money**:
`$200 used · $1,537 left`. Nothing else on the row moves, and it costs no extra height.

    ES 34.73 pts of expected move  =  $1,737 per contract
    travelled 4.00 pts             =  $200 used
    30.73 pts to the rail ahead    =  $1,537 left

**THE MULTIPLIERS ARE VERIFIED, NOT ASSUMED** — against CME and NinjaTrader contract specs. A wrong one
puts a wrong dollar figure on the face under the panel's own name, which is the same class of failure as
the truncated Zero Gamma:

    ES  $50/pt  (tick 0.25 = $12.50) · MES $5 · NQ $20 · MNQ $2      micros are 1/10 of the E-mini

Keyed by the CHART symbol, never the family — ES and MES share a family and differ by 10×. Futures charts
only: a SPY chart has no contract, and inventing a multiplier would be worse than saying nothing.

**A ONE-DOLLAR BUG THE TEST CAUGHT BEFORE IT SHIPPED.** `34.73 * 50` is `1736.4999999999998` in floating
point, not `1736.5`. Rounding that straight to dollars renders **$1,736** for a move genuinely worth
$1,736.50 — a visible error out of arithmetic that looks exact. `usd()` rounds through cents first, and
the regression is pinned. The mockup I sent said $1,737; without the test the shipped panel would have
said $1,736 and I would not have noticed.

**WHAT THIS IS NOT, AND THE TEST THAT KEEPS IT THAT WAY.** The standing rule is that this tool is
descriptive: never entries, stops, sizing, R:R or P&L. A dollar figure sits one step from all of them, so
the framing is deliberate — this is **the expected MOVE converted to dollars per contract**, a unit change
and nothing else. There is no position in it and no quantity. *"$200 used"* means the day has travelled
$200 worth of its priced move; **nobody made or lost $200.** A test scans the VISIBLE labels for
profit/loss/sizing language and fails the build on any of it.

⚠ That test failed on itself first, twice, and both were worth fixing rather than loosening: the
disclaimer legitimately uses the words "profit or loss" in order to DENY them, and the hover it lives in
cannot be stripped by regex because its argument nests `usd(...)` and `dispNum(...)`. It counts parens
now. **A test that cannot tell a denial from a claim fails on its own safeguard.**

⚠ **The multiplier follows the CHART, not what you trade.** Chart ES while trading MES and every figure is
ten times too big. The chip's hover states the micro equivalent for exactly that reason.

## v11.59 — the anchor was drifting, and the line now speaks like a trader

**THE ANCHOR WAS NOT ANCHORED.** `rr` is a LIVE ratio between the displayed instrument and the underlying,
and it moves with the cash/futures basis. Measured on the live panel: `undScale` went 0.099778 → 0.099775
in **twenty seconds**, and the whole band slid 0.05 with it — open, both rails, HOD and LOD.

**Friday's opening print cannot change.** The expected move was captured once and held still; the anchor
was recomputed from a live scale factor on every render. **Half-anchored is not anchored**, and a rail
that wobbles is not a reference. `rr` is now captured alongside the EM and reused for every candle-derived
value, so open, now, HOD and LOD all share ONE scale factor: the arithmetic is exact and the rails hold
still. Cost, stated: if the basis moves materially intraday the displayed band drifts a hair from the
chart. Basis moves are small; wobbling rails are not acceptable in a fixed reference.

Verified against the reference implementation — SyntaxGeek's 0DTE Anchored Expected Move is explicit that
an anchored band is "a snapshot of the EM value at open" that "remains static throughout the day". Both
halves static, not one.

**THE LINE NOW SPEAKS LIKE A TRADER.**

    was:  REVERSED · up 53% down 55% · gave back 107% of the down-move · 33.48 left to EXP HIGH
    now:  LOD −55% → HOD +53% · retraced 107% of the LOD drive · 33.48 pts to EXP HIGH left

HOD and LOD are the words. **The arrow carries the ORDER** — and that is new information, not a
restyling: "sold off then rallied" and "rallied then sold off" are different days and rendered
identically before. It costs one bar index per extreme. "Retraced" is what a retracement is called.

    one-sided:  HOD +107% · one-sided, LOD −11% barely tested · STRETCHED — +G compresses
    inside:     inside · HOD +7% LOD −8% · 32.20 pts to EXP HIGH left
    trend −G:   HOD +119% · one-sided, LOD −4% barely tested · past EM, but −G expands

**HOVERS CUT.** Every tooltip in FRAME is now under ~600 characters, down from paragraphs. A tooltip
nobody finishes reading is a tooltip nobody reads. A test pins the ceiling.

**AND THE DEBUG HOOK NOW SURFACES THE SHAPE FIELDS.** `shape`, `hiWater`, `loWater`, `upExc`, `dnExc`,
`giveBack`, `hiFirst`, `roomAhead`, `gamma`, `stretched` were added to `emBand()` at v11.57 and never
exposed — so the only way to check any of them was to read pixels. **That is exactly how the anchor drift
went unnoticed**: the numbers were never visible to compare across two moments.

⚠ One test caught itself: the "old phrasing is gone" assertion failed because the source comment
legitimately QUOTES the old wording. It strips `//` lines before checking now. A test that cannot tell
code from documentation fails on its own history.

## v11.58 — the regime chip explains all four cells, not just yours

The hover named the cell you were in and described only that one, so there was no way to see what the
other three would have meant — or that the thing is a 2x2 at all. It now lists all four, marks the active
one with `▸`, and says which number you are in:

    1. −G −V  ⚠ the self-reinforcing cell — hedging amplifies twice over
    2. −G +V  short gamma, hedge WITH the move — breaks over fades
    3. +G −V  pinning, but vanna leans the other way — fade, not with size
    4. +G +V  long gamma, hedge AGAINST the move — pins hold

Native `title` attributes render `\n`, and `g3esc` only touches `"` and `<`, so a numbered list survives
into the tooltip without any new markup.

It also states the thing a reader must not get backwards: **the regime decides what an extension MEANS on
the band below.** Past 100% of the expected move reads as STRETCHED only in cells 3 and 4, where range
compresses. In cells 1 and 2 ranges EXPAND, so running past the band is what a trend day does — not a fade
signal. And it repeats why neither gamma nor vanna votes in BIAS: both CONDITION, so they gate.

⚠ **The test for this block was appended AFTER `process.exit()` and silently never ran** — the file
reported the same 56 passes before and after, which is exactly what a test that does not execute looks
like. Caught by comparing the count across the change; the block now sits above the exit and the file
reports 76. **Compare assertion COUNTS across a change, not just pass/fail** — a test that never runs is
green.

## v11.57 — where the day has BEEN, and what an extension means in each regime

Three things the band could not answer, one per stated goal.

**1 · THE TARGET'S REACHABILITY.** The gold triangle is now a **T**, and it GREYS when the target falls
outside what today prices. A magnet beyond the expected high is not a place to expect price today, whatever
the structure says — and the band was silently clamping it to the rail as if it were in range.
**EXP LOW / EXP HIGH** are now the largest numbers on the row, captioned, because they are what gets read.

**2 · HOW MUCH IS LEFT.** Distance travelled was never the question. The shape line reports the **room
remaining toward the rail ahead**, in points, clamped at zero once price is through — a negative "room"
would read as room.

**3 · COULD THIS TURN.** Two additions.

**The excursion.** The band knew only where price IS, so a day that ran +91% of the expected move, reversed
through the open and sat at −92% rendered **identically to a day that had drifted quietly down**. The whole
reversal was invisible. High- and low-water marks since the anchor now draw as a dim span behind the live
marks, and a plain-words shape line reads `INSIDE` / `ONE-SIDED UP` / `ONE-SIDED DOWN` / `REVERSED`, with
the **give-back** — how much of the dominant excursion has been handed back.

**And the regime, which is the part that was actually wrong.** The band coloured red past 100% regardless.
One row above, the regime line said `−G · momentum — breaks not fades`. **The same face was arguing with
itself, one line apart, about the same price.** In positive gamma dealers hedge AGAINST the move: range
compresses, the edge is where reversion is plausible. In negative gamma they hedge WITH it: ranges expand
and the edge is where OVERSHOOT happens. Identical reading, opposite meaning. Past 100% now reads
**STRETCHED in +G** in amber, and in −G stays neutral and says `past EM — but −G expands`.

**WHAT IT DOES NOT CLAIM, AND THE SCORECARD THAT WILL DECIDE.** Every word is descriptive — *stretched*,
never *reversing*. A test fails the build on any predictive phrasing reaching the face.

The hypothesis, stated so it can be wrong: **past 100% of the expected move, price reverts toward the
anchor MORE OFTEN in positive gamma than in negative gamma.** It has never been tested once. Four archived
day-files, 492 bars, effective n ≈ 49 independent observations — and all four are DOWN days, so a
directional claim from them earns its accuracy for free. The band shipped yesterday, so there are ZERO
`emband` records anywhere.

⚠ **The scorecard is SPLIT BY REGIME, and that split is the point.** Scored together, two opposite effects
average to zero and the honest-looking conclusion "no edge" gets drawn from data that actually contained
one. Three questions, each non-voting until n≥20 on its own: does +G past 100% come back; does −G past 100%
keep going; and once a day has been meaningfully both sides of the anchor, does the newer direction hold or
does it rotate again. `shape`, `gamma`, `upExc`, `dnExc` and `giveBack` are recorded per bar so those
questions can ever be answered.

Also corrected in the containment question: our band is the RAW straddle = **0.8 sigma**, so the honest
expectation is **~58% of sessions inside**, not the ~68% that "expected move" implies. The width decision
(multiply by 1.25, or rename the row STRADDLE) is still open and deliberately not taken here.

## v11.56 / companion v1.11 — the companion could never be updated, and nothing said so

**The companion had no `@updateURL` and no `@downloadURL`.** Without those two lines Tampermonkey never
offers an update for a script — not on its schedule, not on a page reload, not ever. The tapereader has
carried them for releases; the companion never did.

**This is why the truncated Zero Gamma survived.** The repo went to v1.10, the browser stayed on v1.9, and
`zeroGamma: 764` kept rendering under THEIR name through a reload. The repo was right, the browser was
wrong, and **neither side said anything** — the panel reports the tapereader's version in its footer and
has never reported the companion's at all, so there was no visible symptom to chase.

Both headers added, and both scripts now have the pin asserted in tests (`test_if_published.js` for the
companion, `test_rules_v2.js` for the tapereader) so a future script cannot ship un-updatable.

**Worth separating two things that look identical from the user's side:** reloading the Atlas tab reloads
the PAGE, not the SCRIPT. Tampermonkey serves its own stored copy until it is told to update. A reload can
therefore look like a failed deploy when the deploy was fine — which is a fourth entry for the
install-failure table, and the one that just cost a round trip.

## v11.55 — the panel works on a weekend

**The app was inert every weekend and every holiday.** `convertFiberCandles` keeps TODAY only, so with no
bars today there was no chart, no SMA, no trend, no nodes — nothing to look at and nothing to develop
against. Skylit's fiber feed still carries Friday; we were simply filtering it out.

When today has no RTH bars, the panel now shows **the last session the feed carries, as if it were the
day** — chart, trend, nodes, levels, all of it. The EM band anchors on that session's real open rather
than the prior close, because in this mode that session IS the day.

**This is the most dangerous thing in the panel, so it has three guards.** A whole face of stale data read
as live is the largest possible version of failure pattern #1 — not one cell wrong, everything wrong at
once, all of it plausible.

1. **NEVER during RTH.** If `sessionPhase().rth` is true we stay on today even when today is empty. A
   quiet pre-open feed must read *"no bars yet"*, never yesterday dressed up as now.
2. **THE RECORDER WRITES NOTHING** — guarded at all five entry points (`recordNodeSnapshot`,
   `recordOutcomeEvent`, `recordDeflections`, `actRecord`, `resolveFeatureOutcomes`). Replayed bars in
   `data/*.json` would poison every base rate the learning layer computes, permanently and undetectably
   after the fact.
3. **THE FACE SAYS SO.** A purple `▮ 08/21 REPLAY` chip REPLACES the phase tag — deliberately not beside
   it, because two chips would let the eye take the wrong one, and the phase of a finished day is noise.
   `__gptsDebug.session()` reports the mode and whether recording is on.

**The guard is written to fail TOWARD recording, and that is deliberate.** `inReplay()` try/catches, and
every call site tests `typeof inReplay==='function'` first. A bare global reference inside the recorder's
try/catch bodies is exactly the swallowed-ReferenceError shape that has bitten this project twice — and it
bit again while building this feature: a python edit asserted, the `SESSION_DAY` declaration never landed,
and five files were left referencing it. Had that shipped, all five write paths would have thrown, been
swallowed, and **silently stopped recording** with nothing on the face to say so. A lost session is loud.
A dead recorder is not. `node tools/smoke.js` calls the session hook, so a missing declaration now fails
the build instead of going quiet.

Same mechanism caught it: **check that the edit landed.** `grep -c SESSION_DAY` returned 5 references and
0 declarations.

The EM capture is keyed on the session being SHOWN rather than the wall-clock date — in replay that date
is a weekend, and every replayed day would otherwise collide on one key.

## v11.54 / companion v1.10 — I shipped their Zero Gamma as 764, and the test that should have caught it passed

**v1.9 put a WRONG NUMBER on the face under THEIR name.** `zeroGamma 764` beside a spot of 7674, tagged
`IF·pub`, rendering on the ladder as `FLIP@765.76`. Every published level was truncated to three digits:
Call Wall 790, Put Wall 750, Max Pain 735.

**The proximate cause was an ordered alternation.** The parser tried a comma-grouped branch first —
`[0-9]{1,3}(?:,[0-9]{3})*(?:\.[0-9]+)?` — and fell back to a plain branch. On `7646.90`, which is how
their page ACTUALLY renders it with no comma, the first branch matched `764`, succeeded, and the plain
branch never ran. Commas are now optional INSIDE one number (`[0-9][0-9,]*(?:\.[0-9]+)?`), so there is
no branch left to pick wrong. Verified against the exact strings their page serves: all eight values.

**The real failure is that the unit test PASSED.** Its fixture wrote `$7,646.90` WITH a comma while the
page renders `$7646.90` without one. **A fixture invented to suit the parser tests the parser against
itself.** The fixture is now the page's own rendering, and both forms are asserted.

**And the defect underneath both: nothing checked the number against reality.** 764 sat next to a spot of
7674 and the code was content. A parser can fail in ways nobody predicted, so `levelSane()` now gates
every price LEVEL to a band around spot (0.5x–2x); outside it the value is refused and `pubSrc` records
`REJECTED:header=764` rather than dropping it silently. Ratios, IV and slopes are not gated — they are
not prices. **This gate, not the regex, is what makes the next unpredicted parser failure visible.**

It also caught a pre-existing bad fixture in `test_if_chain.js`, which paired spot 765.2 with
`zeroGammaLevel 7679.88` — an SPX value beside a SPY spot, the exact scale-mixing this project keeps
getting bitten by. Corrected, and the gate now has its own coverage both directions.

**What this cost, stated plainly:** the wrong value was live on the panel for the length of one reload
cycle, and it was found by reading the debug hook rather than by any test. The hook is why it was found
in minutes instead of on Monday.

## v11.53 / companion v1.9 — the suite is green, and their published numbers are finally read

**THE SUITE WAS NEVER GREEN.** Notes claimed "4239 pass / 0 fail" while six files failed on every run.
Five were **one bug in the shared `ex()` test harness, repeated five times**: `ex()` extracts a single
function BODY and `eval`s it, so any name that body closes over — a callee, a module-level constant —
throws `ReferenceError` inside the eval and the whole file scores zero. It reads exactly like a real
failure, which is why nobody could tell them apart. Fixed by pulling in what each extracted function
actually calls, and by extracting constants FROM SOURCE (`EP_*`, `FLRCEIL_*`, `NM_*`, `NODE_OPEN`) so a
retune moves the tests with the code instead of leaving them asserting a stale threshold.

⚠ While fixing them I walked straight into the landmine this project already documented: **`eval()`
inside a `forEach` callback declares into the CALLBACK's scope**, so eleven constants vanished the moment
it returned. Join and eval once. It is in the notes because it has now cost time twice.

**`rollDetect` returned bare `null` for a feed that EXISTS but is too short** — indistinguishable from
"no feed at all", which is the one case null is reserved for. It now says `not enough session yet`, which
is the refusal doctrine used everywhere else in this file. (`accumAsym` keeps its null deliberately: it
returns a value, not a read, and has no refusal channel.)

**`test_spx_levels.js` is RETIRED, not deleted.** `spxwLane()` and `spxLevels()` are absent from the
source; the file had been throwing from its own `ex()` helper for an unknown number of releases. SPX
levels no longer come from Skylit's SPXW lane at all — they come from the InsiderFinance chain via
`ifLadder`, a different source with a different shape, so its 25 assertions could not be retargeted.
Coverage lives in `test_if_ladder.js` (45), `test_levels_unified.js` (122) and `test_spxw_confluence.js`
(26). The file keeps its ground-truth table and now carries a **live guard**: if those two functions ever
return to source, it FAILS and tells you to un-retire it from git history.

**`test_tapeking.js` fails in the cloud sandbox ONLY** — `jsdom` cannot install there (npm 403). It
passes on the user's machine. That is environmental, not a defect, and it is the only red left.

---

**COMPANION v1.9 — THE PAYLOAD IS NOT THE PAGE, AND THAT MISTAKE HAS NOW BEEN MADE THREE TIMES.**

`pick()` walked `initialData`, every published metric came back null, and the conclusion drawn was
*"their page computes client-side, so we must compute too."* That conclusion drove real decisions: we
computed our own zero gamma, and TERM was called "structurally dead — their payload does not carry it."

**Verified 2026-08-22 by fetching their raw HTML with NO JavaScript running:**

    Zero Gamma 7646.90 · Call Wall 7900 · Put Wall 7500 · ATM IV 6.2 · Put/Call 1.36 · Term Slope 1.3

The numbers are in the server-rendered MARKUP, outside `__NEXT_DATA__`. The payload was never the whole
page. `pub` now resolves through **three sources in order — payload field, tree walk, then rendered
header** — and `pubSrc` records which one won for each value, so anything on our face can be traced to
where it came from. Anchored on the LABELS their page prints, never class names: class names are exactly
what churns, and a moved label yields null while the computed path still runs.

**What this changes on the face, for free:** `ifLadder` already preferred `c.pub.zeroGamma` and fell
through to a computed flip tagged `FLIP*`/`calc` because it was always null. It will now get their
published 7646.90 and render as **`FLIP`** sourced `IF·pub`. Their value, their name, no substitution.

⚠ **Their header walls are ALL-EXPIRY** (CW 7900 / PW 7500) while our ladder's CR0/PS0 are **0DTE**
(7700/7665, verified reproducing their 0DTE view exactly). Both are "their values" and they answer
DIFFERENT questions. `pub.wallsAreAllExpiry` travels with them so the caveat cannot get separated from
the data. Substituting one for the other would be failure pattern #1 wearing a "use their numbers" badge.

**AND THE CORRECTION I OWE ON THE FETCH PATH.** Earlier this session I recommended turning the
tapereader's dormant `ifFetch` on. **That was wrong and it would have killed the tape.** The source says
so explicitly: the tapereader runs `@grant none`, which is load-bearing — it keeps the script in PAGE
context so its `window.fetch` / `XMLHttpRequest` hooks capture Skylit's feed. Any `@grant` moves it into
Tampermonkey's sandbox where `window` is a wrapper and the hooks patch the wrapper instead of the page.
The companion exists precisely to hold that grant. The published values belong there, and now they are.

**Housekeeping:** `install.bat` (10,382 lines of base64), `v10.js` and `tools/gex-pull.log` are untracked
and gitignored — the installer bloated every single diff and is superseded by the Drive pipeline.
`tools/AUTOPULL-TEST.txt` removed. Drop messages must stay ASCII: cmd reads `.gex-drop-msg` as ANSI, so
an em-dash arrives as mojibake in the commit subject.

## v11.50 — the band was absent exactly when you were preparing

**v11.49 shipped a band you could not see.** `closedCandles()` is filtered to TODAY, so on a weekend, on
a holiday, and every morning before the first RTH bar closes at 08:33 CT, there were no bars — no open —
and the band refused with *"no closed bars yet — no open to measure from"*. Which is precisely the window
a trader is sitting there planning the session. The refusal was technically honest and practically useless.

**The anchor now falls back to the PRIOR SESSION'S CLOSE.** Quoting an expected move from the prior close
is the standard reference when today has not started, and it is a FIXED anchor, not a live one chasing
spot — the whole reason the band is anchored at all. `S.contCloses` already carries prior sessions with a
`day` field, so the last close of the last completed day is available without new plumbing.

**It re-anchors to the real open the moment the first RTH bar closes**, and a test pins that transition.
The two references can never be silently confused: the face appends **`· FROM PREV CLOSE`** beside the
percentage, and the hover says the day has not started and that it will re-anchor.

Refusal is now reserved for genuinely having nothing — no bars today AND no prior session — and it says
which of the two is missing.

Landed alongside v11.49's own reason for existing, which is worth restating: the EM on the face was
`toFri.em`, a LATER expiry worth roughly double the day's (69.25 against 34.65), sitting under a hover
asking "how much room does today have". `dte0` only, forever.
## v11.49 — the EM on the face was a WEEK's move, and FRAME stopped reporting instruments

**The expected move shown under "how much room does today have" was answering about a different week.**
`secFrame` read `ec.toFri.em` and only fell back to `dte0`. But `toFri.em` is not a five-day move — it is a
SINGLE expiry's straddle picked out of the Mon..Fri set, so on any day but Friday it lands on a LATER
expiry worth roughly double the day's. Measured 2026-08-22: **toFri 69.25 against dte0 34.65.** Every
"% of EM used" was computed against a yardstick about twice as long as the day it described, and any
target looked comfortably in range when it was not.

**On a Friday the two coincide.** That is why it survived — the bug hides itself once a week, on the day
someone is most likely to check. This is the same shape as the `week`/"to Fri" mislabel already in the
note: a value under a label implying a different window, and nothing ever throws.

**The band replaces the number.** `open ± dte0 EM`, captured ONCE at the open and held fixed all session.
That is the standard anchored-EM convention, and the anchoring is the whole point: a band recentred on
spot every render just follows price around and can never say "further than was priced" — by 3pm a live
band would claim the day's possible range is a handful of points. The rail IS the day's priced range, so
the dot's position and the percentage beside it are the same fact and cannot drift apart. Marks CLAMP at
the rail; the percentage is what says how far past.

**The open price was never the hard part.** `closedCandles()` is already filtered to today from 08:30 CT,
so `cs[0].o` is the opening print even when the panel is started at noon. Only the EM's freshness varies.
Captured late, the straddle has already decayed and the band is narrower than the open's truly was — that
is what `~EST` says. **No reconstruction is attempted.** Scaling a decayed straddle back up by √(T) assumes
IV has not moved since the open, and on the days that matter it has.

**Line 2 stopped being four naked measurements.** Line 1 was synthesised — a regime translated into what it
rewards. Line 2 reported instrument readings. A number there either feeds the sentence or it is not on the
face.

- **DEX removed.** Its SIGN is structurally pinned. Live SPX to-Friday netDex −$13.5B; per-strike
  decomposition +14,732 against −34,671, negatives 2.35x, and dte0 negative too. Index chains are put-OI
  dominant, so the sign essentially cannot flip — and a sign that never changes carries nothing. This is
  the same disease the skew read already cures by voting the level against its OWN recent range. DEX never
  got that treatment because **it was never recorded**, so there was no range to vote against. It is
  recorded from this build, with the spot beside it.
- **TERM removed** — but NOT for the reason first given. Their *payload* carries no term figure, and that
  was mistaken for the value being unavailable. Their *page* publishes **Term Slope +1.3**, computed
  client-side from `options[]`. It is uncomputed, not absent. It goes because term structure CONDITIONS
  and never points — a third regime axis beside gamma and vanna that would rarely change the sentence.
- **ATR removed from the FACE only.** It still sets the ladder's ± zone widths and still sizes the
  rejection detector. It stopped needing to be read.

**HVL is now FLIP.** HVL is SpotGamma's house term (High Volatility Level) for a related but not identical
level. The industry-standard names are Zero Gamma and Gamma Flip, and our own source InsiderFinance calls
it "Zero Gamma Level" — so the panel was wearing a third vendor's vocabulary for a number it takes from a
second. Display strings and rank-map keys only; `hvlSrc`, `LVL_COL.hvl`, `P.hvl`, `isHVL` and the recorded
field names are untouched, because renaming identifiers is churn with no reader on the other end.

**Both new numbers are ENROLLED, which the originals never were.** DEX and EM sat on the face for releases
with no per-bar record, no scorecard and no question — which is exactly how a cell nobody can check
survives. `emband` states a claim that can be WRONG: past 100% of the priced move, price comes back toward
the open. It scores `null` on any bar inside the band, because a feature that votes on every bar of a
trending day earns accuracy for free — the one-directional trap the nightly rules exist to catch. `dex`
scores `null` outright and does not vote until the level has a range behind it.

**The enrolment guard caught the build mid-flight**, twice: `test_feature_enrollment` failed because the two
new rules were not seeded into `learning/rules.json`, and `test_rules_v2` then failed because the seeds were
missing `lastVerified` and `walkForward`. Both were real structural gaps in what was being shipped, not
tests needing to be bent. `test_em_band.js` (41 assertions) pins the dte0 rule, the FORBIDDEN toFri
fallback, open-from-first-candle, capture-not-overwritten, the `~EST` flag and the clamps.

**A note for whoever runs the suite next: it is NOT green and has not been.** Six files fail before any of
this build's changes. One is environmental (`jsdom` cannot install in the cloud sandbox). The other five are
stale harness gaps — tests that `eval()` a function slice without pulling in the names it closes over, so
they throw `ReferenceError` on `fmtLvl`, `dexSkewFor`, `nodeSessChg` and friends, all of which exist in the
source. The new band test hit the identical trap and works around it by extracting the constants from
source. This is precisely the camouflage the note warns about and it deserves its own pass.

## v11.48 — stop the swallow being silent

The NODES block still rendered nothing at v11.47, and the cause was the same shape as the last one: it
used **`rr`**, the chart-scale multiplier, which is declared in `nodeChartHtml` and nowhere near
`secLoc`. ReferenceError on the first row. The block's own try/catch ate it, the header had already been
emitted, and the face showed a heading with nothing under it — which reads as "there were no nodes",
not as "this is broken".

**Twice in two builds, from the same mechanism.** A missing function first, a variable from another
scope second. Both invisible for the same reason: defensive try/catch is what makes a ReferenceError
look like an empty result.

A regex scan for undeclared identifiers was tried and abandoned. It flagged keywords, regex-literal
contents and inner-function parameters, and a check that drowns in false positives gets switched off —
which is worse than not having it. JavaScript needs a parser, not a pattern.

So the catch is instrumented instead. **`swallow(tag, e)` records what was eaten**, `__gptsDebug.renderErrors()`
exposes it, and the smoke test FAILS on a non-empty list. That catches the whole class, including the
cases nobody thought to predict — which is the point, since both of these were exactly that.

The smoke test also moved **into the repo** at `tools/smoke.js`. It had been living in `/tmp` and the
sandbox container has now reset twice mid-session; a check that only exists in scratch space is one you
rewrite from memory at the worst possible moment.

## v11.47 — the NODES block called a function that was never added

v11.46 shipped the TRADE LOCATION node block, and it never rendered. The block calls `tradeNodes()`.
That function was never added: the edit which would have added it aborted on a failed assertion, the
failure went unnoticed, and only the half that renders the block landed.

**The block's own try/catch swallowed the ReferenceError.** No error, no console line, no broken layout
— the NODES section simply was not there, which is indistinguishable from "there were no nodes". The
defensive try/catch wrapped around every section is exactly what made a missing function invisible.

`test_render_refs.js` is the counterweight: it walks every renderer and fails the build if any of them
calls a function the file does not declare. Building it took three passes — the first version flagged
`.test()` and `.trim()` (method calls), the second flagged `rgba(` (CSS), the third flagged `Fri(` and
`RULE(` from inside comments and string literals. A check that drowns in false positives gets switched
off, and then a real miss slips through, so it strips comments and strings and matches bare calls only.

Confirmed working live at v11.46 despite the miss: node markers on the chart went from **0 to 4** — the
live-edge fix landed. ④ REACTION now reads the node: *"WATCH node 7676.73 · leg.pb"*, *"the node is dec ·
book upside bleeding faster · −$1.1B vs −$552M/30m"*. The target sits beside the regime, CAGE is gone,
and the ladder carries its depth diamonds.

## v11.46 — the panel finally shows the thing you trade

**The panel spoke two vocabularies that never met.** LEVELS (CR, PS, Mag, HVL) come from
InsiderFinance — structure, context, daily. NODES come from Skylit — live, and per the node rule, where
the trade actually is. ③ TRADE LOCATION showed the levels and hid the tradeable object; ⑤ EXECUTE traded
a node nothing else on the face mentioned. The join lived in the trader's head.

**Nodes are now rows in the ladder, beside the levels**, carrying strength, polarity, state and distance
— and marked **@CR / @PS** when a node sits AT a level. That is the strongest thing this panel can find:
live positioning on top of structure that matters. A node in open space is tradeable with less behind
it; a level with no node is not a trade at all, and says so. The node the pullback engine has selected —
the one EXECUTE is armed against — is marked ▸ and highlighted, so the chain from location to execution
is visible rather than inferred.

**And the reason no nodes ever appeared on the chart.** Markers drew only where a history sample fell
inside the CANDLE window. After any reload that is a growing sliver; with the market closed it is
NOTHING, because candles stop at the last close while `sampleTapeHistory` keeps writing timestamps for
now. **Absence of history was rendering as absence of node** — the opposite of the truth, with 100
strikes and 317 samples sitting in memory. History still draws the lifecycle; the node's current
reading now always draws at the right edge.

**④ REACTION reads the NODE**, not the nearest level. Under the node rule "is it holding?" has to be
asked of the thing being traded; asked of a level it answered a question nobody was about to act on.
When there is no node it falls back to the level and labels it *context only, not a trigger*.

**The target sits beside the regime.** Together they answer one question — what kind of day is this, and
where is it trying to go. It was buried among the supporting numbers, reading as one more statistic
rather than the destination. **CAGE is gone**: a percentage nobody acts on, on a face with no space.

**Step headers are centred and highlighted** to match the step bar.

## v11.45 — the node rule, and EXECUTE could never arm

**THE NODE RULE (user-mandated 2026-08-22): a trade is off a NODE.** Levels give context — where
structure sits, what shape the day has — but the entry itself has to be at a node, preferably a
PULLBACK node. Price sitting at a level with no node behind it is information, not a setup. ⑤ EXECUTE
now refuses outright with **NO NODE — NO TRADE**, and names the level as context rather than implying
a trade might be there.

**And wiring that up exposed why EXECUTE has never armed.** It tested `pb.entry`. `pbEntryPick` has
never returned an `entry` — it returns `level`. So a valid node-based pullback entry sat in the object
while the face showed "no setup", from v11.26 until now. Reading an absent property is not an error in
JavaScript, so nothing ever threw. Measured live at the moment of the fix: `{ok: true, level: 764,
rule: 'leg.pb', state: 'dec'}` — a real entry the panel was hiding from itself.

The stop now sits beyond the node's OWN zone rather than a fixed pad, the target is the next structural
stop, and a node setting up against the SMA is refused as **AGAINST THE CALL**.

**Hover audit, measured rather than assumed.** 114 fields carried a tip and **41 did not**, and the gap
was systematic: the tiny LABEL held the explanation while the value beside it held none — so hovering
the number a trader is actually looking at gave nothing. FRAME cells now wrap label and value in one
tipped element, ladder ROWS carry their tip so name, price, distance and depth marker are all covered,
and the price row, verdict block and SET line gained the questions they answer.

## v11.44 — drift was confirming calls it disagreed with

The live face read **`↑ BULLISH`** beside **`DRIFT ✓ DN·conf`**.

Both halves were doing exactly what they were told. The two books DID agree — on DOWN — and the tick
only ever asked whether they agreed with *each other*. So a structural read pointing squarely against
the call was displayed as confirmation, with a green tick on it.

Agreement is only confirmation when it points the same way as the SMA. Books agreeing against the call
is now **✗ · "books agree DOWN — against the call"**. A lean with the call is a soft `~`; a lean against
it is still a cross, because softness does not excuse direction. With the SMA flat there is no call to
confirm, so it reads `·` rather than ticking.

This is the same failure as everything else this project keeps finding: a label asserting something the
number underneath did not support, with nothing throwing. It survived because "do the books agree?" and
"do the books agree with us?" sound like one question.

The depth fix from v11.43 is confirmed working on live data — markers went from none to five, and the
scores now discriminate: `PS·Mag g=1.00 d=0.45 → ◆◆`, `CR g=0.98 d=0.17 → ◆`, `HVL* g=0.31 d=0.10 → ·`.
Before it, every level measured 0.02–0.20 because they were being compared to the wrong book.

## v11.43 — one book per question

Two instances of the same error, both found by checking the live panel rather than trusting it.

**The left column was drawing Skylit gamma under a caption that says "IF · structure".** Two books, one
label — the exact mislabeling pattern this project keeps producing, one layer down. It now draws THEIR
gamma beside THEIR delta. Skylit gamma keeps its own job on the flow side, which is what it is for.

**And the depth score was comparing IF's levels to Skylit's gamma book.** Measured live, every ladder
level scored between 0.02 and 0.20 on gamma — not because the levels were weak, but because Skylit's
gamma peaks at spot while InsiderFinance's walls sit away from it. A level has to be compared to the
book it came from. The companion now emits a per-strike `gexProf` alongside `dexProf`, both from their
chain, and depth scores against those.

Without this the ◆ markers would have rendered almost never, and the one time they did it would have
been coincidence.

**The payload question is settled.** `ifShape()` on the live panel returned the whole of `initialData`:

    ticker · tickerDetails{...} · spot · options[] · timestamp · isStale

There is no zeroGamma, no callWall, no putWall, no skew — **nothing computed at all.** Their entire page
is rendered client-side from `options[]` and `spot`. So the derived gamma flip is not a shortcut around
a number they publish; it is the only way to have one, and `HVL*` tagged `calc` is the honest label.
Everything we compute uses their DATA, which is the principle intact.

## v11.42 — depth, expected move, and scales on the chart

**Every ladder row looked equally important. They are not.** A strike carrying heavy dealer GAMMA and
heavy dealer DELTA is a materially harder place for price to pass than one carrying gamma alone — gamma
decides how price behaves there, delta decides how much hedging must happen to get through. Rows now
carry a depth marker: ◆◆ when both books are loaded, ◆ when one is. Tiers rather than a score, because a
trader needs to know which two levels matter today, not that one rated 0.63.

The bug that made it work: the two books sit on DIFFERENT strike grids, so gamma lands at 766.00 and
delta at 766.06. Taking the nearest single key read one book and scored the other as zero — every level
came out gamma-only. A level is a zone, so both books' contributions inside it count, taking the max so
adjacent strikes are not double-counted.

**Expected move is on the FRAME line**, with how much of it the session has already spent. It is the
number that makes a target honest: a level beyond what remains is not in play today whatever the
structure says. Blank unless both straddle legs quote at one strike within 1% of spot.

**REACTION gained a DEPTH row** — how much delta and gamma stand behind the level being tested. That is
a different question from whether the node is growing, and a better one for whether a wall will hold.

**The chart has scales.** Price ticks down the inside of the plot's left edge on ROUND numbers chosen
from the range rather than the range divided by four, so they stay stable as the window scrolls. Time
along the bottom, landing on the half hour rather than on whenever the window happens to begin, with the
step widening on a longer window so labels cannot collide.

**And test_no_dupes earned itself on the very next build.** `confluence(sym)` already existed at ~15022
and is called at ~2943; the new function took the same name and would have silently shadowed it. Caught
before shipping rather than nine releases later, and renamed `levelDepth`. Fourth collision in this
project, first that never reached the user.

## v11.41 — take theirs, and drop what nobody acts on

Two corrections, both prompted by the right question being asked.

**The payload scan was shallow.** `pick()` only looked at TOP-LEVEL fields of `initialData`, so every
published metric read as null and that drove a decision to compute our own zero gamma. A nested object
would have looked identical to an absent one. It now walks the tree, and the payload's own shape is
recorded so "absent" is a finding rather than an assumption — `__gptsDebug.ifShape()`.

**Their zero gamma wins, always.** The derived Black-Scholes flip is a FALLBACK, used only when the
payload genuinely carries nothing, drawn as `HVL*` and tagged `calc` rather than `IF·pub`. When neither
exists there is no row at all. Computing a number they publish is the mistake this project keeps
repeating; the fix is a preference order that cannot silently invert.

**Charm is gone.** It was computed and never displayed, and asked what decision a `CHEX −$1.2B/day`
cell changes, the honest answer is none — the consequence that matters, pins weakening into an expiry
close, is already carried by the session phase tag. Carrying an unused computation is the same
accumulation problem as carrying unused text, so it was removed rather than left wired up for later.

**Expected move** stays: the ATM straddle from their bid/ask, refused unless both legs quote at the same
strike within 1% of spot. A one-sided straddle is not a straddle, and a far pair is not at the money.

## v11.40 — the suite is green, and it was hiding two live bugs

The 23 "known stale" failures had been carried as a baseline for the whole project. They were examined
properly for the first time. **Twenty-one were stale tests. Two were real bugs that had been shipping.**

**The locked voice lost a word.** `Rallied down to 768.` should read `Rallied down to 768 target.` The
voice was authored by the trader on 2026-08-18 and the note says the wording and punctuation are locked.
"Rallied down to 768" reads as a location; "to 768 target" says the leg ARRIVED at what it was aiming at,
which is why the next sentence watches for a pullback. Restored verbatim.

**A function-name collision broke the manual-InsiderFinance panel.** v11.31 added `ifNum(x)` as a display
formatter. `ifNum(txt, label)` already existed at line ~1646 as the parser for manually-entered IF
values. Two declarations, one name — the later one wins silently, so all five parser callers were handed
the formatter, passed it a string and got NaN. Broken for nine releases with nothing thrown. The
formatter is now `dispNum`, and `test_no_dupes.js` fails the build on any NEW collision in either script,
with the two inert pre-existing ones pinned so they stay visible.

**The other twenty-one were tests left behind by deliberate changes:**
- 13 in accum_canon — the v11.0 audit made `accumCanon` FEED-first; the harness only ever stubbed the
  tape path, so every call threw, was swallowed by the function's own try/catch, and returned nulls.
  Thirteen assertions reported as logic failures when the logic had never run.
- 5 in node_role_badge — the label extractor keyed off a `padding:0 6px` CSS declaration, so a styling
  tweak broke assertions that had nothing to do with styling. Plus two stale contracts: the badge reads
  `isFlr`/`isCeil` rather than `role`, and a role-less node falls through to `Mag` rather than blank.
- 2 in node_identity — v10.44 renamed `Diss` to `Dec` and said so in the source; the test never followed.
- 1 in tape_sync — v10.47 deliberately replaced the blocking gate with a banner, user-directed, so the
  app stays inspectable. The assertion pinned the pre-v10.47 behaviour.

**4109 pass, 0 fail.** The lesson is the point: a permanently-red baseline trains everyone to ignore red,
and two genuine regressions hid in it for months. A suite is only useful while it is green.

## v11.39 — flow bars point inward and show growth, and the chart stops spending space on a key

**The Skylit bars now grow LEFTWARD from the right edge**, so both profiles point in toward price and
the chart reads as a spine rather than two unrelated columns.

**Growth is the segments, not a tick mark.** Each bar is drawn in three parts: dim for what the node has
held for over an hour, mid for what it added between 60 and 15 minutes ago, and a bright leading edge
for the last 15 minutes. A node accumulating shows a bright front; one that is bleeding shows dim length
with nothing new on it, and the ground it lost is marked so decay is visible rather than merely absent.

Two bugs found by tests while building it. The historical widths were clamped to the current width,
which erased the only case the shrink marker exists for — a node smaller now than it was 15 minutes ago.
And the scale was normalised on current values alone, so a node that had shrunk had a past larger than
the axis: its history clipped to full width and the whole bar rendered as one dim block. Normalising
across current and historical values fixes both.

**The legend row is gone.** The panel has no vertical space to spend on a key, and everything it said is
in the chart hover, which is where a key belongs.

**The ladder is tighter** — line-height 1.6 to 1.28, and the ES price row lost its vertical margins and
padding entirely. It was the worst offender.

## v11.38 — three zones, centred labels, and hovers that ask before they tell

**The chart is now three zones.** LEFT is InsiderFinance STRUCTURE — net GEX, then net DEX, two narrow
columns sharing the price axis. It refreshes once a day and tells you where the walls are: gamma for how
price moves at them, delta for which way hedging pushes. RIGHT is Skylit FLOW — live node strength, with
tick marks on every bar showing where that node stood **60 minutes** and **15 minutes** ago. A bar
reaching well past both is accumulating; a bar behind them is bleeding. MIDDLE is price.

Structure left, flow right, is the whole convention, and both captions are on the face.

**Level labels are centred on their own lines**, and the line is drawn in two segments so it breaks
around the text rather than running under it. That is what frees both gutters for the profiles. Strong
node rows keep a bare price — the redesign briefly took those with it, and a band you cannot read the
price of is a shape, not a level.

**The 50-SMA is drawn**, from `contSMAAtTodayIdx` — the same continuous series `trendVerdict` reads, so
the line and the call cannot disagree. It is the primary read and it had never been on our chart at all.

**Hovers ask before they tell.** Every step header and every substantive cell now opens with the
question it answers: *which playbook is legal today · which way and how much should you trust it · where
would you actually trade · is the level doing something · do you take it or refuse it.* A test asserts a
question mark in the opening line of all five, and pins fifteen more by name.

**ACCUM no longer contradicts itself.** It printed `below −$553M` beside a BULLISH vote, because the
vote is relative — the upside book was bleeding faster. Both were true and the sentence still fought the
tick. When both sides shrink it now names the one shrinking faster.

**And a double-conversion caught by its own test.** The centred label re-derived an IF level's display
price through the SPY→ES ratio when `ifLadder` had already computed it from the live SPX→ES basis — two
paths, disagreeing by a point or two. The line position and the printed number are now passed
separately, and the printed one is the value the source gave.

## v11.37 — DEX is real, and the skew is measured rather than borrowed

`__gptsDebug.optKeys()` answered the question that had been blocking three features. Their payload
carries per contract:

    strike, expireYear, expireMonth, expireDay, cp, gamma, delta, openInterest, impliedVol, bid, ask

`delta`, `impliedVol`, `bid` and `ask` are all there — while every one of their PUBLISHED metrics came
back null, because their page computes them client-side. So the answer is not to take theirs; it is that
we can compute the same things properly.

**DEX** = `sum(delta x OI x 100 x spot)`, in FRAME as a number rather than a vote. Puts already carry
negative delta in their payload, so no sign flip is applied — a test pins that, because flipping it
twice would have produced a plausible number with the wrong sign. Negative DEX means dealers are short
delta and must BUY as price rises, so rallies get chased. It maps hedging pressure; it does not point,
which is exactly why it sits in FRAME and not in the BIAS row.

**SKEW** is now the real 25-delta metric — 25-delta put IV minus 25-delta call IV, the figure their page
prints as "25Δ Skew". Matched on nearest delta rather than interpolated, and refused entirely when
nothing sits within 0.08 of 25 delta, so one thin far-wing quote cannot invent a number. Still read
against its own recent range rather than as a level, because index skew is permanently put-heavy.

Also emitted for the next build: a near-spot **DEX profile** (the mirror of the GEX bars, and the second
panel in the user's screenshots) and **ATM IV** from the two 50-delta legs.

## v11.36 — the 50-SMA is the direction, and the GEX profile finally exists

**BIAS is no longer a tally.** Direction is the 50-SMA; SKEW, ACCUM and PA confirm it or they do not,
and the count is the confidence. The old six-vote tally printed NEUTRAL on a 2-2 split while the SMA was
plainly sloped — the panel arguing with the chart — and it let PATH, MASS and VEX vote on direction when
all three are gamma-family reads: conditional, not directional. Confirmation is the better job for the
supplementary reads anyway, since TREND alone measured 34% and "TREND with 3 of 3 confirming" is a
different proposition from "TREND with 0 of 3". The confirmers never outvote the SMA, and three of them
agreeing cannot manufacture a direction the SMA does not have.

**ACCUM rebuilt on dollars.** It read STATE.walls, which on a tape-derived book carry %King and a null
`abs`, so it abstained almost every session and printed a dash that read as broken. It now reads the
feed's own `levels` series — ~390 snapshots of dollars at one-minute cadence, the same source rollDetect
uses. Growth in dollars is flow; growth in %King is a moving denominator.

**SKEW taken from InsiderFinance**, read against its OWN recent range rather than as a level, because
index skew is permanently put-heavy and the raw number would print the same verdict every day.

**The GEX profile.** Asked for in the first session and never built — node bands shipped instead and I
never said I had substituted one for the other. Horizontal bars on the price axis, length by dollar
gamma, green where dealers are long gamma and purple where short: the same picture their Net GEX panel
draws, from data that had been arriving all along.

**FRAME is two lines.** `−G −V ⚠` then the playbook — "self-reinforcing" was a third description of one
condition sitting between them, and the playbook already says widen stops, which is the part you act on.
The second line is TGT, DEX, TERM, ATR, CAGE and a phase tag. PAIN and P/C are gone: max pain is dead
weight four days in five, and put/call is a once-a-day number with an ambiguous sign.

**Layout.** Section labels are inline and left-justified instead of five centred banner rows — about
110px back. Deleted: the "waiting on" line, which restated in small grey type what BIAS says two rows
below in large type, and the "NO SETUP · bias is neutral" box, which was the third time the panel said
one sentence on one screen. An empty EXECUTE is now a single dash.

**pickEdge unit mismatch, fixed after five offers.** `FLRCEIL_FAR` is documented and reasoned about in
STRIKES; `dist` is `Math.abs(k-px)` — price points. On SPY those coincide because its strikes are 1
point apart, which is why it never bit. On SPX they are 5 apart, so the same constant meant something
5x different depending on the book. The spacing is now measured from the map, by median gap so one wide
strike cannot drag it.

**Companion v1.4** reports `optKeys` — the field names on a single option — which decides whether DEX
and a computed skew are buildable at all, and extracts their published skew, skew slope, term slope,
ATM IV and put/call if the payload carries them.

## v11.35 — the pop-out is usable on a big monitor

Popping out opened a 369px window on a 2032px screen, because the size was taken from the panel's current
width. It now sizes from the available SCREEN — about a quarter of its width and most of its height, with
a 420px floor so it can never open unusably narrow again.

But a bigger window on its own only adds empty space; nothing in it grows. So there is a **scale control**
in the header — `− 100% +`, click the percentage to reset. It uses `zoom` rather than `transform:scale`
deliberately: zoom participates in layout, so a panel at `width:100%` still fits its container at any
scale instead of overflowing sideways and forcing a horizontal scrollbar.

The scale persists, is clamped to 70–220%, ignores a corrupt or out-of-range stored value rather than
applying it, and clears the property entirely at 100% — an inert `zoom:1` still creates a containing
block. It is re-applied on both legs of the pop-out, because the panel changes documents when it moves.

## v11.34 — rolls on the chart, and the panel pops out

**Roll detection.** A roll is mass moving between strikes: one node dissipating while another on the
same side accumulates. Measured in DOLLARS from the feed's own `levels` time series — ~390 snapshots at
one-minute cadence, already arriving with every fetch, so nothing needs recording and nothing is lost on
reload. %King could not be used: if the King strike changes every node rebases at once and a whole
cluster reads as dissipating together, which is the most convincing possible false positive.

Thresholds were measured on 2026-08-21 (390 snapshots x 209 strikes), not guessed. 30-minute window,
50% **relative to the session median**, $40M floor, within 5 strikes — nine events across the session.
35% gave 37 (noise), 70% gave 2 (silence). The median near-money strike grows 10-15% every 30 minutes as
the book builds, so an absolute threshold reports accumulation all morning and dissipation into every
close, worst of all on an expiry day where strikes lost 75-94% to decay alone. The window is walked by
TIMESTAMP, not by index, so a cadence change cannot quietly turn 30 minutes into five.

A node dissipating with nothing growing is a wall evaporating, not a roll — it draws no arrow, because an
arrow to nowhere is a claim we cannot support.

The label is two lines, placed beside their own rows rather than one string across the middle that the
curve ran through. **Destination bold, origin faded** — a ceiling rolling down emphasises the lower line,
a floor rolling up the upper one. Arrow colour is the implication, red down and green up, deliberately
unlike the purple/yellow of the bands, which mean put and call.

Marked **`shadow · not voting`** on the face. On the four archived sessions this pattern had no
measurable edge, and every one of those days was a down day with a 66% base rate, so nothing could be
settled either way. It draws and records; it does not push the BIAS call until it earns it.

**Pop out.** A ⇗ button in the header opens the panel in a Document Picture-in-Picture window — a real
always-on-top window that can be dragged to any monitor. It is a page API, so `@grant none` survives,
which matters because the feed hooks patch window.fetch in page context. Styles are copied into the PiP
document, cross-origin sheets are skipped rather than thrown on, and closing it puts the panel back
exactly where it came from.

The trap this had to avoid: the Atlas tab stays the only thing capturing the feed, and our own code
returned early whenever `document.visibilityState !== 'visible'`. Backgrounding Atlas — the entire point
of popping out — would have silently stopped both books refreshing while the panel went on displaying
its last values. Every visibility gate now runs through `panelVisible()`, which counts an open pop-out as
visible. Each call site falls back to the raw visibility check when the helper is absent rather than
skipping the gate, which would reverse the behaviour instead of preserving it.

## v11.33 — a band is a level you can read

Prices in the chart's right gutter, converted to the chart instrument. Price, the IF ladder levels and
the node rows are placed in ONE pass in that priority order, because three independent passes overprint
each other into an unreadable stack. A label that would collide with one already placed is dropped, but
its MARKERS STAY — nothing is hidden, only a duplicate number.

A node under 15% of King keeps its markers and loses its label: the gutter is for levels, not noise. And
a row whose samples all fall outside the window now gets no label either — a price in the gutter with no
markers beside it reads as a level that is not there.

## v11.32 — the node chart

Atlas draws dealer-exposure nodes as horizontal rows of markers at their strikes, and reading a
pullback off that picture is a different act from reading a list of numbers. The panel now draws the
same thing under the ladder in step ③.

**Colour is polarity, not position.** Purple points down and is put-dominant; yellow points up and is
call-dominant. %King as captured from Skylit's King cell is SIGNED, so the polarity comes from the
data rather than from where price happens to be sitting — a node keeps its colour when price crosses
it. Purple has meant negative gamma everywhere else in the panel and a put-dominant node is exactly
where dealers are short gamma, so the two readings agree instead of competing.

Brightness is %King, with a 3% noise floor so a faint node is not dressed as a level. Row length is
lifecycle: a row spanning the chart has held all session, a short one just appeared.

**Two books, two visual languages.** The bands are SKYLIT flow. The dashed labelled lines are the
INSIDER FINANCE levels from the ladder above. They are drawn differently and the legend names both,
because a chart where you have to remember which book a line came from is the same failure this
project spent the day fixing.

`HIST_MAX` 12 → 130, so the bands can span a session rather than 36 minutes. HIST is in-memory, so
after a reload the chart refills from empty and says so on the face instead of implying there are no
nodes.

Nodes and candles are both on the underlying scale, so the chart computes there and only the labels
are converted to the chart instrument.

## v11.31 — the ladder reads SPX, converted by the live basis

We were fetching SPY while the page being checked against was SPX. Two different chains on two
different underlyings, so "are we consistent with InsiderFinance" could not be answered by looking
at the screen. Worse, SPY levels reached ES through a ~10.05 multiplier, so half a point of
disagreement arrived on the chart as five ES points.

ES is a future ON SPX, so SPX is the book that governs it. The companion now fetches SPX and the
conversion is a basis near 1.003, taken live from THEIR spot against the futures print rather than
from a constant we maintain. A rounding difference stays a rounding difference.

This converts a PRICE SCALE, never a strike grid. An SPX wall is not restated as a SPY strike:
SPX strikes sit on a 5-point grid and SPY on a 1-point grid, and dividing 7700 by ten invents a SPY
level that has no open interest behind it. Each row keeps its original strike, plus a chart-scale
value for display and an underlying-scale value for the candle reads, which run on the underlying.

Not done here, and deliberately: QQQ still fetches QQQ. By the same argument NQ wants NDX, but that
is its own change.

## v11.30 — HVL is their published Zero Gamma

Their page prints Zero Gamma in its header — 766.48 on SPY, 7679.88 on SPX. v11.29 computed a
cumulative net-GEX crossing instead and was one step from putting it on the face labelled IF. That
is the same mislabeling as showing Skylit numbers in an IF ladder, one layer down.

The companion now takes their published Zero Gamma, Call Wall and Put Wall straight off the page.
Key names are read directly and then found by pattern, so a rename on their side self-heals rather
than silently dropping to null. When they publish nothing the field is null and the HVL row simply
does not appear — nothing is substituted.

Their header values belong to the expiration filter their page defaults to, which is not necessarily
either of this ladder's windows, so the HVL row is tagged `IF·pub` rather than `IF` and its hover
says to read it as a structural reference rather than a window-matched level.

## v11.29 — the ladder shows InsiderFinance's levels

The requirement from the first session, finally on the face. The companion has been fetching and
computing their levels correctly for two builds; v11.26 wired the ladder to lvlUnified — the SKYLIT
sets — so the panel displayed our own numbers while their chain sat unused in storage. Live on
2026-08-21 the face read CR 766 while their book said 770.

③ TRADE LOCATION now reads their chain and nothing else: CR, CR0, PS, PS0, Mag and Max Pain,
converted to the chart's instrument. The FRAME target reads the same book, so the target and the
levels can no longer come from two different sources.

**No fallback (user-directed).** If their chain is missing or older than 25 minutes the ladder says
so and shows nothing. Skylit's levels are one call away and that is exactly why they are not
substituted: Skylit measures flow, IF measures open-interest stock, and swapping one for the other
changes what the numbers mean without changing how they look.

Nothing is invented. There is no HVL row, because they do not hand us a zero-gamma level and a
homemade approximation labelled with their name would be the same mislabeling in a different place.
Every row on the ladder is a level they actually give us.

Skylit keeps the work their once-daily chain cannot do: growth, accumulation and node lifecycle.

## v11.28 — the roll was oscillating

The v11.27 roll worked once and then fought itself. `expSetRollCheck` re-tested the HELD week set
on every tick, so the moment the rolled window came back spanning six expirations it no longer
looked collapsed — the roll was dropped, the next fetch went out unrolled, collapsed onto 0DTE
again, and re-armed. Live on 2026-08-21 the panel was flipping between a healthy 288-strike set
with 146 mixed strikes and a degenerate 284-strike one with zero, every fetch cycle. Whether the
levels were right depended on when you happened to look.

A roll is a property of the DATE, not of the set we happen to be holding. Today either is the
week's last expiry or it is not, and no fetch result changes that. Once armed for a trading day
the roll now stands until the date changes, and repeated checks are a no-op — which also stops
the fetch throttle being reset in a loop.

Deciding from the date has a second benefit: a Friday arms the roll before the week set has even
arrived, saving one guaranteed-collapsed request. The control set is now the only genuinely
required input, since it is what sizes the rolled request. A midweek collapse — a holiday-shortened
week — is still caught by comparing the held sets.

## v11.27 — six defects the first live check found

Everything in v11.26 rendered, and the Friday roll fired for real: the week window went from
one expiration and 284 strikes to six expirations through 8/28 and 288 strikes, decomposition
went from 0 mixed strikes to 146, and HVL computed where it had been blank. Then the live face
showed six things wrong.

**PAIN was blank while the number was sitting right there.** The companion stores its levels at
`toFri.lv` and `dte0.lv`; the FRAME section read a `week` key that never existed. Max pain was
755 the whole time.

**The face contradicted itself.** `playbook: momentum — breaks over fades` sat directly above
`MIDDAY — fades work, breaks usually fail`. The phase line now describes CONDITIONS only —
energy, compression, expansion, pin strength — and fade-versus-break belongs to the regime line
and nowhere else. There is a test that greps every phase string for tactical language.

**An expiry day is an expiry day from the open.** The expiry phase only triggered in the last
90 minutes, so 0DTE Friday read as an ordinary MIDDAY. Expiry is now flagged all session, with
the final stretch saying charm is accelerating.

**`DRIFT ✓` next to "both books not in yet".** driftRead returns `verdict`/`label`/`overlap`
and no `line` field, so the tick and the sentence beside it were reporting different things.
AGREE now shows ✓, LEAN shows ~, SPLIT shows ✗ and says the books disagree, NONE shows ·.

**The companion carried the identical Friday collapse.** Its payload reported
`friday: 20260821, today: 20260821` — the same bug in the second script. It rolls forward too,
and sets a `rolled` flag the panel discloses.

**ACCUM abstained silently.** With a thin feed every wall is tape-derived and carries %King but
no dollars, and summing dollar mass off a moving percentage denominator is the exact error the
project already ruled out. So abstaining is correct — it now says why instead of printing a
bare dash that reads as broken.

Also: PA voted neutral on an LH/LL tape because close location alone was 0.42, mid-band.
Structure now breaks the tie when close location is inconclusive.

## v11.26 — the panel becomes the process

The five-step face we locked in design finally ships. The panel now renders
① FRAME → ② BIAS → ③ TRADE LOCATION → ④ REACTION → ⑤ EXECUTE, with a step bar that
lights the step you are actually on and a line saying what it is waiting for. Section
headers are centred and highlighted to match the bar. The old stack of independent cards
is still there behind `CFG.panelV3===false`, so a bad build reverts from the gear rather
than a reinstall.

**The Friday roll-forward.** `exp_mode=week` means "through Friday of the current week",
which ON a Friday is today — so the week window collapsed onto the 0DTE window and CR/CR0
printed the same number off the same 284 strikes. Seen live on 2026-08-21. When the collapse
is detected the window rolls to NEXT Friday, and the request is sized from the wk7 control
set's own expiration list rather than a guessed calendar. The roll is disclosed on the face.

**Regime is two-dimensional.** We were reading gamma and ignoring the vanna book we already
receive. GEX × VEX is a 2×2 and one cell — negative gamma with negative vanna — is the
self-reinforcing one where hedging amplifies the move. That cell is flagged, and the
playbook line switches to momentum with widened stops instead of fading edges.

**Session phase.** The panel was completely time-blind. It now knows the opening charm
window, the midday lull, the power hour, and the expiry-Friday charm phase where gamma
erodes and pins stop working.

**Price action, standing in for market internals.** We have no TICK, no advance-decline and
no cumulative delta, and pretending otherwise would be dishonest. What OHLC does give us is
close location — where each bar finishes inside its own range — which is the fair analogue
of a sustained TICK reading. That is now the PA vote, alongside a structure tag and a
rejection detector for step ④. A rejection requires price to have COME FROM the side it
closes back to; without that rule a bar straddling the level reads as a rejection when it
is actually a break.

**Not yet built, and shown as `—` rather than guessed:** DEX, CHEX and expected move from
the ATM straddle. Each needs chain fields we do not yet plumb through.

## v11.25 — 2026-08-21 — Their levels, computed from their own chain (companion script)

The user's original instinct, which I talked them out of three times: **get the levels from InsiderFinance
instead of deriving them.** It turns out to be better than either of us thought.

**Their page embeds the FULL option chain.** `__NEXT_DATA__` carries 13,210 contracts for SPY, each with
`{strike, expireYear, expireMonth, expireDay, cp, gamma, delta, openInterest, impliedVol, bid, ask}`. So we
do not scrape their rendered header at all — we recompute from source. Their CSS, layout and wording can
change freely; it breaks only if they stop embedding the chain.

**Verified live against their published figures, spot 761.14 — every value exact:**

| window | ours (computed) | theirs (published) |
|---|---|---|
| 0DTE | CR suppressed @ 2.18% call share · PS 760 · Mag 760 · r 0.02 | CR **N/A** · PS 760 · r 0.02 |
| **to Friday** | **CR 775 · PS 760 · Mag 760 · r 0.40** | **CR 775 · PS 760 · r 0.40** |
| All | CR 800 · Call 17.8B · Put −32.18B · r 0.55 | CR 800 · 17.8B · −32.2B · 0.55 |

Formula: `GEX = gamma × OI × 100 × spot² × 0.01`, puts negative. And because we compute rather than read,
the window is **ours** — through *this* Friday, not their rolling-7 preset.

**Everything previously ruled out is now free**, because the chain carries open interest and implied vol:
**Max Pain** (0DTE 763 · to-Friday 758 · all 753) and the **put/call OI ratio** (55.5 · 5.18 · 2.85).

**A SEPARATE userscript, and that is the whole design.** `current/gex-if-levels.user.js` takes
`@grant GM_xmlhttpRequest` + `@connect insiderfinance.io` — it needs the grant to get past CORS (their
server sends no `Access-Control-Allow-Origin`; verified live, `BLOCKED Failed to fetch`). The Tapereader
**keeps `@grant none`**, page context, feed hooks untouched, and only reads localStorage. Same origin, so
they share it. If the companion dies, the tape does not notice.

**They are a DIFFERENT MEASUREMENT and the card says so.** Skylit's node values are live dealer positioning
that accumulates intraday — their docs say it, and our own session data shows strike 763 growing 18M → 600M
in a day. This is open interest × gamma: where exposure *sits*. A stock beside a flow. Their lane renders
italic and dotted, never as a check on ours.

**Both lanes reach the chart and Investor/RT.** The gamma set (CR/CR0/PS/PS0/HVL/Mag) exports with the
0DTE variants thinner; their levels export tagged `IF` so nothing on the chart is ambiguous about which
measurement drew a line. Either lane can be absent without breaking the export.

New `test_if_chain.js` (38) and `test_irt_export.js` grows to 35; `test_levels_unified.js` 122. No new
suite failures.

## v11.24 — 2026-08-21 — THE defect: we were reading the still-forming bar

The user pushed back that being this far off was impossible and something was not being accounted for.
They were right, and there is a single cause.

**`levels` is a ~390-entry TIME SERIES and the last entry is the CURRENT, STILL-FORMING bar.** We read
`levels[levels.length-1]`. Counting strikes that carry a real call/put split (`|net| < v`):

| snapshot | strikes with a split |
|---|---|
| 0 | 84 |
| 195 | 78 |
| 388 | 82 |
| **389 — the one we read** | **0** |

During RTH that frame fills in properly. **After the close it degenerates to `net = ±v` on every strike.**
Confirmed on the self-fetched sets AND the passive feed: `callPut()` returned `lt:0, eq:208, gt:0,
decomposable:false`.

Everything we misdiagnosed this week follows from it. Every strike read as 100% call or 100% put, because
`call=(v−net)/2` collapses to `v` or `0`. Our call wall picked **765** — a strike their book shows as
heavily PUT-dominant (put 1300M vs call 342M). And values appeared to swing **20×** between fetches; they
never did — I was comparing a degenerate frame against a complete one and calling it instability.

`pickSnapshot()` now walks back to the most recent frame carrying a decomposition, bounded by
`SNAP_MAX_STEPBACK` (8). Past that bound it returns the newest frame **flagged** rather than reaching for
stale data — a frame from twenty minutes ago wearing a current timestamp is worse than admitting we have
nothing. The card shows **⚠ no split** and `gexSanity` gains `snapshotHasDecomposition`, so a provisional
level can never again look measured.

New `design/LEVELS-DERIVATION.md` records how every level is derived, the verified rules and the ones
ruled out, the inverted sign convention, all seven guards, and — explicitly — that **CR remains unproven
until re-checked during live hours**, because every check we ever ran on it used the bad frame.

New `test_snapshot_pick.js` (17). `test_levels_unified.js` 105. No new suite failures.

## v11.23 — 2026-08-21 — Node coverage: 250 → 500

The last open discrepancy from the live check. On the **rolling-7 control** — a like-for-like window against
theirs — we read **CR 765** where they read **775**, and our call/put ratio was **0.70** against their
**0.36**. Same window, so the definition difference that explains a mid-week CR gap does not apply here.

Suspected cause is coverage, and the two symptoms point the same way: `nodes=250` returned only **212**
strikes against the **590** their 1W view reports. A ranked top-N drops the many small strikes in the
tails, which are disproportionately puts — so the book reads less put-heavy than it is, and that bias moves
where the most call-dominant strike lands.

Raised to `nodes=500` across all three sets. **This is a test, not a fix:** `__gptsDebug.expSets()` reports
`n` per set. If 500 still returns ~212, the cap is the server's, coverage is NOT the explanation, and the
remaining gap is in the selection — which is worth knowing either way.

No new suite failures.

## v11.22 — 2026-08-21 — Nothing goes missing silently

User, off the live v11.21 card: *"i dont see CR0 and PS"*. Two different faults behind one complaint.

**PS was there — buried.** The magnet and the put wall were on the same strike, so the row merged them,
and the label read `Mag·PS`. The label a trader scans for was second and easy to miss. Merged labels now
lead with the **wall** — `PS·Mag` — because the walls are the trade-location levels and the magnet is the
destination.

**CR0 was legitimately absent, and the card said nothing.** The 0DTE book had no call-dominant strike above
spot at all (call/put ratio 0.00), which is the correct answer and matches the N/A a published GEX page
shows on the same book. But the row was simply omitted, so it read as broken rather than as a finding.

Every level on the roster is now **accounted for**. A level with no value gets a line naming it and the
reason, and the reasons are distinguished rather than lumped together:

| shown as | means |
|---|---|
| `CR0 none today — 0DTE book is all put` | there is genuinely no call wall in today's expiry |
| `CR0 same as CR` | it landed on the identical strike |
| `CR0 0DTE set not loaded` | the fetch has not landed yet — a different thing entirely |

And when a 0DTE wall coincides with the weekly one, the strike carries **both labels** (`CR·CR0`) rather
than either being dropped: one row, one price, two facts. That is strictly better than the old behaviour,
which silently hid the second.

`test_levels_unified.js` 98. No new suite failures.

## v11.21 — 2026-08-21 — The verified level rules, two day-trading windows, and a directional read

The build the user asked for: 0DTE and through-Friday, correct levels, a sanity check, a readable chart,
and — the part that matters — the levels folded into a claim about **where price is going**.

**THE RULES, VERIFIED — not inferred from prose.** I extracted InsiderFinance's own 585-strike SPX table
and tested every candidate definition against the rows *they* tag:

| rule | strike | their tag |
|---|---|---|
| most POSITIVE net GEX above spot | 7775 | **CALL WALL** ✓ |
| most NEGATIVE net GEX below spot | 7640 | **PUT WALL** ✓ |
| largest \|net GEX\| anywhere | 7645 | **Peak GEX** ✓ |
| max CALL gamma above spot | 7650 | — ✗ |
| max call OPEN INTEREST above spot | 8800 | — ✗ |

So the walls run on **signed net**, which our feed has carried from the start — the call/put decomposition
was aimed at the wrong target. Their own description mentions open interest; the tagged row does not follow
it. **The table is ground truth, the prose is not.**

**⚠ Our net is the opposite sign from theirs.** Their net is call−put (negative on a put-heavy strike);
ours is put−call. Every rule mirrors. Get it backwards and both walls swap sides while still looking like
plausible levels — which is why `gexSanity` checks that CR sits above spot and PS below, the one failure
nothing else would catch.

**Two windows, both near-dated** (user's call): `dte0` = today; `week` = every expiry through this Friday,
which is Skylit's native `exp_mode=week`. Deliberately NOT their "Next week", which is a rolling 7 days —
their 1W preset checked 0DTE plus 1/4/5/6/7DTE and excluded 8DTE. Ours shrinks as the week runs down;
theirs does not. Mid-week the two differ, so a CR gap is a definition difference and the sanity check does
not flag it. The rolling-7 window is still fetched slowly as **`wk7`, never displayed** — the control that
lets the scorecard eventually say which window price actually respected. Forward-only data, so it starts now.

Their expiry breakdown is what settled the windows: **0DTE alone was −$43.44B of a −$62.50B whole-chain
total.** About 70% of dealer gamma expires today.

**PATH — where price is going.** `gexPath` reads the structure as a direction rather than a list: Mag is the
destination (heaviest hedging, so price is drawn to it), PS and CR are the two edges where a deflection is
most likely, and the cage bar shows how much room is left before the next reversal zone. It states when the
recent tape *disagrees* with the pull, and when the magnet lies BEYOND the near wall — the case where a
floor to lean on and a magnet pulling through it conflict. Descriptive, non-voting, and it sits directly
under Next Stop because it is the same claim.

**Enrolled with six questions, all written so they can fail** — including `path_pinned_stays`, which scores
a pinned read on price *staying put* so the feature cannot win by calling everything pinned on a quiet tape,
and `path_roll_age` as the explicit control for the migration claim. **Wall migration is recorded per bar**
(`crRolled`/`psRolled`, from-strike, delta, how long the old one stood), which is the question the user
raised weeks ago and nobody has ever measured: does a floor that just rolled up beat one sitting still?

**The periodic third-party check the user asked for cannot run in the panel, and the reason is settled.**
Their server sends no CORS header — verified live in the console, `fetch(...)` returns "BLOCKED Failed to
fetch" — and the `@grant` that would bypass it sandboxes this script and kills the `window.fetch` hook the
entire tape depends on. So `gexSanity()` checks what can be checked without anyone else's data: walls
straddle spot, levels inside the strike range actually read, no duplicate strikes, nothing more than 10%
from price, the set is fresh, a magnet exists. **Every one of those fired at least once during this build.**
Where their numbers are hand-entered, the put-wall delta *is* enforced — it held at the same strike across
every one of their expiry filters, so a gap there has no window excuse.

**UI changes cannot break any of this.** The levels come from the network payload, not the DOM — unlike the
ladder scraper that has broken repeatedly. A Skylit redesign does not touch it.

New `test_gex_levels.js` (36) pins the verified rules against their real numbers and every sanity assertion.
`test_expset_symbol` and `test_levels_unified` updated for the new windows. No new suite failures.

## v11.20 — 2026-08-21 — Three faults found checking v11.19 on the live panel

v11.19's own fix worked: the live 0DTE book came back with a **0.2% call share** and the phantom call wall
at 792 was correctly withheld — matching the N/A a third-party page shows on the same book. But checking it
turned up three more.

**1. The card rendered BLANK right after a reload.** The full-chain set was still in flight and the passive
feed had not arrived, but the 0DTE set *had* landed — and `lvlUnified` only ever treated `dte0` as a source
of extra rows, never as a source of levels. So it returned null and the whole card disappeared. Precedence
is now full chain → 0DTE → passive feed, the face says **`0DTE only`** when the today-only set is carrying
the read, and the full chain takes over the moment it lands.

**2. The nearest-spot zero-gamma fix had only half-landed.** `cpLevels` carried its OWN copy of the
wall-and-crossing logic, so the v11.18 fix reached `cpFromPayload` and left the passive path still
returning the first crossing from the bottom — **479.7 against a spot of 762.57**, live, in this build.
`cpLevels` now delegates to `cpFromPayload`. One implementation of a rule, not two; two is how a fix
half-lands and nobody notices.

**3. An HVL nowhere near price is not a flip.** The live 0DTE set put its nearest crossing at **687.09,
ten percent below spot**, because an all-put book only crosses out in the tail. A gamma flip is a statement
about where price *is*. Beyond `HVL_MAX_DIST` (3%) the level is withheld and the card says *"nearest flip
9.9% away — tail, not a flip"*, keeping the strike for the explanation rather than dropping the row.

**Still open and NOT fixed here — our zero gamma may not be their zero gamma.** On the same 0DTE book their
page reports a flip at 762.42, essentially at spot, where our method finds nothing nearer than 687. Summing
gamma across strikes and re-evaluating net exposure at hypothetical spot prices are different calculations,
and the second is the one that yields a level at spot. Worth settling before the HVL is trusted, and not
something to paper over with a threshold.

`test_levels_unified.js` 83, `test_call_put.js` 48. No new suite failures.

## v11.19 — 2026-08-21 — Levels correct for every symbol, no invented walls, a readable chart

The user asked whether multi-symbol support had actually been confirmed. It had not, and it was not true.
Three fixes.

**1. THE EXPIRY SETS ARE KEYED BY SYMBOL — QQQ/NQ was showing SPY levels.** v11.17–11.18 kept ONE global
set and hardcoded `symbol=SPY` into the self-fetch and `STATE.SPY.price` into the read:

    url.replace(/symbol=[^&]*/, 'symbol=SPY')     // expSetFetch
    var px=(STATE.SPY||{}).price;                  // expSetLevels
    var EXPSET={ dte0:null, full:null };           // no symbol dimension

So selecting QQQ or NQ produced SPY walls priced against a QQQ spot — every level ~55 points from price,
presented with full confidence. Worse than a blank card. `EXPSET` is now keyed by symbol, the request asks
for the symbol it was given, throttling and back-off are per symbol+set (so one book cannot starve the
other), and the tick follows whatever chart is on screen — still two extra requests per cycle. SPY and ES
both resolve through `activeSym()` to the SPY book; QQQ and NQ to the QQQ book.

**2. A WALL NEEDS A SIDE WITH REAL GAMMA IN IT.** The live 0DTE book came back with a call/put ratio of
**0.00** — there is essentially no call gamma on expiry day — and we still named a call wall at **792**,
which then sorted ABOVE the full-chain CR at 765. A 0DTE wall further out than the all-expiration wall is
backwards on its face; it was the largest of a set of rounding artefacts. A side must now hold at least
`SIDE_MIN_SHARE` (5%) of the book's total gamma before its wall gets a name — a third-party page reports
N/A at a ~2% share, so 5% is comfortably inside territory they already treat as "no wall". The rule is
symmetric: a call-dominated book loses its PUT wall the same way. The strike that *would* have been named
is kept and shown on the card with the share that disqualified it, so the absence is explained rather than
silent.

**3. THE PRICE ACTION OWNS THE CHART SCALE.** That same 792 was 30 points above everything else, so the
y-range stretched to reach it and the price line plus every real level collapsed into the bottom fifth —
the "looks crazy" chart. The range is now driven by the price action, growing for a level only within a
bounded multiple of it; anything beyond is drawn as a labelled **edge marker** (`▲ CR0 7960`). Still
visible, still identified, unable to flatten the chart.

**SPX remains out of scope, deliberately.** We do not hold an SPX book — only the SPXW *derived* lane, and
their own SPX and SPY pages disagree by 13 points on the call wall, so a converted SPY level is not their
SPX level. Supported markets are SPY, ES, QQQ and NQ.

New `test_expset_symbol.js` (25) pins the per-symbol behaviour so the hardcoding cannot silently return;
`test_levels_unified.js` grows to 72 with the suppression and chart-clamp cases. No new suite failures.

## v11.18 — 2026-08-21 — Three bugs the live v11.17 card exposed

Checked v11.17 against the running panel. **The self-fetch works** — the full-chain set came back with
**34 expirations and 241 strikes spanning 345–1180**, against the passive feed's 4 expirations and 60
strikes, with the captured auth accepted and zero failures. And the put wall reads **760**, their number.
But the card showed `CR:765 · Mag:760 · HVL:479.7`, which exposed three faults.

**1. Zero gamma was a tail artefact.** Over a real chain the cumulative crosses zero several times, and the
scan took the FIRST crossing walking up from the deep OTM puts — **479.7 against a spot of 762.57**. The
flip that means anything is the one price is near, so all crossings are now collected and the nearest to
spot wins. `nCross` and `crossings` ride with the read so a multi-crossing book is visible.

**2. The put support row had vanished.** PS and Mag were both 760, and the de-duplication dropped whichever
was added second — so a card meant to show four levels showed three, missing the floor entirely. Coincident
levels now **merge their labels** (`Mag·PS 760`) instead of one disappearing. That reading is also better
than either label alone: the heaviest strike in the book is also the floor.

**3. The 0DTE set never populated.** `cpFromPayload` refused any set without at least one MIXED strike. On
an expiry-day book most strikes are entirely call or entirely put, so `|net| == v` on nearly all of them and
the guard rejected the whole thing — silently, since a refusal is not a failure. The total-and-net
convention is already established from the wider books (49 of 60 mixed there), so an all-pure set
decomposes fine. Only `|net| > v` still refuses, because that would mean `v` is not a total. `mixed` and
`pure` counts are now reported so an all-pure set is visible rather than assumed.

`test_levels_unified.js` grows to 51, with the 479.7 case and the merged-label case pinned as fixtures. No
new suite failures.

## v11.17 — 2026-08-20 — One block, one scale, correct signs, real expiry sets

Three things the user asked for, plus a sign error of mine that made the call/put work wrong in a way that
mattered.

**1. THE SIGN WAS BACKWARDS.** v11.16 used `call = (v+net)/2`. It is the opposite: **`put = (v+net)/2`,
`call = (v-net)/2`**. Skylit's `net` is POSITIVE on a put-dominated strike. Tested against their published
SPY page:

| | v11.16 | corrected | their page |
|---|---|---|---|
| Put wall | 756 ✗ | **760** ✓ | **760** (0DTE *and* next-week) |
| Book | call-heavy, 1.44 ✗ | put-heavy, 0.70 ✓ | put-heavy, 0.02 / 0.36 |

Corrected, our put wall lands exactly on theirs. That cross-check is now asserted in `test_call_put.js`.

**2. THE DERIVED LANE IS ALREADY ON THIS CHART'S SCALE.** Skylit converts it for us: on the SPY feed the
SPXW lane's strikes read 686–808 (SPY dollars), and on the ES1 feed the SPY-derived lane reads 4422–8041
(ES points). v11.14 treated `k` as the raw source strike and divided spot by the ratio — which is exactly
how "Mag 762 / PS 762 / HVL 750" with −6879 distances ended up under an SPX heading. `spxwLane` and
`spxLevels` are **deleted**, not patched: their premise was wrong and the unified block replaces them.

**3. ONE BLOCK, ONE SCALE.** `levelsHtmlV2` replaces three stacked blocks with one. Values are on the
chart's instrument only — no `SPY 765` tail on an ES chart. A level prints **once**: de-duplicated by
value, so a 0DTE wall identical to the structural one adds no row, and CR0/PS0 appear only when they
genuinely differ. Source precedence: self-fetched full chain → self-fetched 0DTE → passive feed → ladder,
and the card states which one answered.

**4. EXPIRY SETS BY SELF-FETCH — because no Skylit setting will do it.** Tested live: switching the overlay
EXPIRATIONS dropdown Front→Week moved only the `combined`/`p20` requests; switching the Heatmap preset
Normal→Wide (500 strikes, 50 expirations) moved nothing at all. The request we consume stayed
`nodes=60 & exp_mode=next_n & exp_count=4` throughout — it is hardcoded by whichever component feeds
Trinity. So `expSetFetch()` replays LASTFEEDURL with `exp_mode`/`exp_count`/`nodes` overridden, using the
Authorization header the panel has captured since v10.48. Results live in their **own cache** — `LASTFEED`
is never touched, so the tape is completely undisturbed. Backs off after three refusals; 401 is ignored
because the app re-captures the token shortly.

**Also tested and REJECTED: the Heatmap's `READ AS: Value` toggle.** It looked like a free fix for the
dollars-vs-%King problem. It destroys the ladder reader: SPY King jumped 760 → 812, `$278,444K` became
`15`, and every strike read ±100 across all four panes. Reverted immediately. Making that toggle usable is
parser work, not a setting.

New `test_levels_unified.js` (39). `test_spx_levels.js` deleted with the code it covered. No new suite
failures.

## v11.16 — 2026-08-20 — THE CALL WALL IS COMPUTABLE. I was wrong.

The user asked me to double-check the claim that we cannot compute a call wall. I checked against the live
payload, and the claim was wrong.

**What the feed actually sends.** A strike row is `{k, v, d, net}` — and I had only ever used `v` and `d`,
treating `net` as decoration. Captured live off the wire, 2026-08-20:

    {k:760, v:283651666.375,  d:1,  net:278399177.125}
    {k:765, v:174886547.5078, d:-1, net:-174886547.5078}
    {k:766, v:61620230.75,    d:-1, net:-51428766.25}

`v` is TOTAL gamma; `net` is NET gamma. Across the live 60-strike book: **49 strikes had |net| < v, 11 had
|net| == v (a strike that is entirely put or entirely call), and ZERO had |net| > v.** That is precisely the
signature of total-and-net, and it makes the split algebra:

    call = (v + net) / 2            put = (v - net) / 2

A call/put split never required a call/put feed. Two aggregates determine it — which is why a third-party GEX
page's own header satisfies the same identity (Call 8.3 + Put 51.7 = Total 60.0; Call 8.3 − Put 51.7 = Net
−43.4). They are deriving it the same way, from data we also have. **No scraping, no `@grant`, no hand entry.**

`cpRows()` / `cpLevels()` decompose every strike and place **Call Resistance on CALL gamma** and **Put Support
on PUT gamma** — their definitions, from our own feed. A new **CALL/PUT · derived** block on the card shows
them with the SPX equivalent, the gamma at each wall, and the summed Call/Put totals with their ratio.

**Why 765 makes the point.** It carries 175M of gamma — the second-heaviest strike in the sample — and
decomposes to **zero call**. A net-based "call wall" could pick it; a real one never can. That is the gap this
build closes, and it is asserted in the tests.

**One caveat kept on the card's face.** The app requests `nodes=60`, a RANKED TOP-60 subset of the chain, not
the whole book. The walls found inside it are real, but the TOTALS are subset totals and will not equal a
full-chain page's Call GEX / Put GEX. The ratio is displayed precisely so that gap stays visible instead of
being assumed away. Also learned from the wire: the API takes `exp_mode` / `exp_count` (`current&1` = 0DTE,
`next_n&4` = the front four) and returns `expirations` and `strike_interval` — so a true CR0-vs-CR split is
available at the source, which is the obvious next build.

`test_call_put.js` grows to 45, with fixtures taken verbatim from the captured payload rather than invented.
No new suite failures.

## v11.15 — 2026-08-20 — Double-checking the call-wall claim: the `net` field I ignored

User: *"can you double check why we cant get call wall values."* Good instinct — my claim had a hole in it.

I had been saying the call wall is unreachable because the feed carries no call/put split. That skipped
something the strike rows already carry: alongside `v` they carry **`net`**. And a call/put split does not
require a call/put feed — **total and net fully determine it:**

    call = (total + net) / 2        |put| = (total - net) / 2

Their own page header satisfies exactly this identity: Call 8.3 + |Put| 51.7 = Total 60.0, and
Call 8.3 − |Put| 51.7 = Net −43.4. So they are almost certainly deriving call and put the same way, from two
aggregates — not from privileged data. If Skylit's `v` is a TOTAL and `net` is a NET, the split is pure algebra
and the call wall is computable from the feed we already have, with no scraping and no `@grant`.

`__gptsDebug.callPut()` settles it against the live payload. Per strike it classifies:

| observation | meaning | consequence |
|---|---|---|
| `\|net\| < v` | v is total, net is net | **decomposable — call wall computable** |
| `\|net\| == v` | net is just the signed magnitude | no extra information; the split really is absent |
| `\|net\| > v` | the fields mean something else | report and stop; do not guess |

In the good case it derives call and put per strike, places the call wall on **call gamma** (their definition,
not our net proxy), and reports the summed call/put totals and their ratio — which is the cross-check that
proves the decomposition rather than assuming it: those totals should line up with their header.

The tests pin all three branches deliberately. A probe that only recognises the convenient outcome is worse
than no probe, so the "genuinely absent" and "means something else" paths are asserted to emit **no wall at
all** rather than a fabricated one.

New `test_call_put.js` (24). No new suite failures.

## v11.14 — 2026-08-20 — Levels computed natively on the SPXW lane, in SPX points

User: *"can you compute the same?"* — with clean labelled screenshots of their SPX page at three expiry
filters. This is the build that makes that comparison honest, by taking the conversion out of it.

Skylit's `derived` SPXW lane carries SPX-scale strikes with absolute dollar values and a sign. So CR/PS/HVL/Mag
can be computed **in SPX points directly** and placed beside an SPX gamma page with nothing in the middle —
previously any disagreement could be blamed on the ratio. `spxLevels()` does that; a new **OURS · SPX** block on
the card shows them, with the lane's spot, strike count and range on its face.

**Their page, 2026-08-20, spot 7642.21:**

| view | Call Wall | Put Wall | Zero Gamma | call/put ratio |
|---|---|---|---|---|
| 0DTE | 7760 | **7640** | 7695.17 | 0.16 |
| next week | 7775 | **7640** | 7672.98 | 0.44 |
| all expirations | 7900 | **7640** | 7660.22 | 0.82 |

**The put wall is 7640 in all three.** It does not move because it is driven by near-dated, near-money puts —
exactly the part of the book Skylit gives us in full. That is the number we should match, and the card now lets
you check it at a glance.

**The call wall is a different story, and the tests made the point better than the prose did.** Our column is
labelled **CR(net)**, never "Call Resistance". Building the fixture, a put strike above spot outweighed the call
strike above it — which is not a contrivance: at a 0.16 ratio, puts outweigh calls better than 6 to 1, and put
strikes above spot routinely carry more gamma than call strikes. So the heaviest *net* strike above spot came out
100 points away from the heaviest *call* strike. Ours is not a rough version of theirs; it is a different strike,
and it will stay that way until something decomposes calls from puts.

**Reach is reported too**, because it is the first thing that decides whether their number is computable at all:
`kMin`/`kMax` and the same as a percentage of spot. If the lane tops out below 7900, their full-chain call wall
is not in our data and no amount of computation recovers it. `__gptsDebug.spxDump()` prints the whole lane
compactly so the arithmetic can be checked by hand.

New `test_spx_levels.js` (25). No new suite failures.

## v11.13 — 2026-08-20 — Feed shape probe: can we compute their levels from the SPXW lane?

User: *"are you able to calculate from the spxw tape and match what they have?"* The honest answer is that it
depends on three facts about the payload that nobody has actually looked at, so this build adds the probe
instead of another argument.

`__gptsDebug.feedShape('SPY')` reports the SHAPE of the captured feed — never its contents. Per lane (native
and every `derived` lane, each on its own scale, unconverted): snapshot count, strike count, strike range,
strike step, the row's key names, and exactly two sample rows. Plus a key scan for anything matching
`call|put|cGex|pGex|cOi|pOi` and anything matching `exp|dte|maturity`, each with the full path to where it was
found.

**The decisive line is `verdict`.** InsiderFinance's Call Wall is defined on *call* gamma specifically. If the
payload never splits call from put, their Call Wall is not reproducible from our data at any effort — and if it
does, it becomes computable directly, with no scraping, no `@grant`, and no vendor dependency. The probe answers
that in one line rather than by inference.

**One defect the tests caught in the probe itself:** the key scan recursed into `array[0]` only. The feed's
`levels` array is a *time series* whose first snapshot is routinely empty — so a payload that carried call/put
fields in its latest snapshot would have been reported as having none, which is precisely the wrong answer to
get wrong here. It now scans the first *and last* element and names the index in the path.

New `test_feed_shape.js` (21). No new suite failures.

## v11.12 — 2026-08-20 — InsiderFinance levels ON the card (entered by hand, because a fetch is impossible)

**The question was settled empirically, in the live console on app.skylit.ai:**

    await fetch('https://www.insiderfinance.io/gamma-exposure/SPX')...  ->  "BLOCKED Failed to fetch"

Their server sends no `Access-Control-Allow-Origin`, so a script running on Skylit cannot read their page. The
standard fix — `@grant GM_xmlhttpRequest` — moves this script into Tampermonkey's sandbox, where our
`window.fetch` / `XMLHttpRequest` hooks would patch a wrapper instead of the page and the tape would go dark.
Both routes are therefore closed, and no amount of further argument was going to open one.

So the levels are typed in, once. That is less painful than it sounds: **their walls barely move.** On 2026-08-20
the SPX call wall read 7900 from early afternoon through the close while SPX fell 66 points. A number that static
does not need a five-minute refresh — it needs to be on the chart.

**What shipped.** An `IF` control in the LEVELS header opens a paste box; `ifManParse` takes what a person
actually pastes (`7900 7640 7660.22 7645`, dollar signs, commas, stray labels) and **detects the scale** — SPX
prints in thousands, SPY in hundreds — instead of asking which tab it came from. `ifManLevels()` converts onto
our cash scale using Skylit's own SPXW ratio, and if that ratio is not in the feed yet it says so rather than
mis-scaling SPX numbers as SPY. Their levels render as a separate block under ours, italic, dimmer, headed
INSIDERFINANCE with the source scale and an age stamp, and on the chart as fine dotted lines labelled `IF·CR`
etc. The chart SCALE stays ours — their walls sit far out by nature (SPX 7900 against 7642 spot) and including
them would squash a whole session into a few pixels, so an out-of-range level simply is not drawn while the row
list still carries the number and its distance.

**They never merge with ours.** Every row says whose number it is, they feed no direction, and they are not
scored as ours. This is a reference map drawn beside our read, which is the only honest way to show a number we
cannot reproduce: their Call Wall uses call gamma specifically, and neither Skylit's feed nor InsiderFinance's own
published per-strike table decomposes calls from puts.

**A correction to v11.11's note:** it claimed they do not publish call/put per strike. Their page has a
**Call/Put** toggle beside Net GEX that I never opened — I had only ever looked at the default Net view. The
split may well be available there; worth checking before assuming their call wall is unreachable.

22 new assertions in `test_levels.js` (107 total). No new suite failures.

## v11.11 — 2026-08-20 — The SPX scale was already in the feed

User, on the question of how to reach InsiderFinance's SPX numbers: *"you already get it when you do the initial
conversion."* Correct, and it makes the whole external-fetch question moot for scale. Skylit's payload ships a
`derived` array whose SPXW lane carries the SPX→SPY ratio it used — `irtRatio()` has been falling back to
`1/dd.ratio` for its ES estimate since v11.4. The same number gives the SPX equivalent of every level for free:
no `@grant`, no scraping, no relay, no risk to the feed hooks.

`spxRatio()` / `lvlSpx()` add the SPX figure to each level's tooltip, so a level can be put straight beside an
SPX gamma page without arithmetic. **It converts SCALE ONLY**, and the tooltip says so: "our 765 is SPX 7683" is
not "their SPX book has a wall at 7683". Their walls come from a different chain, a full expiry span and a
call/put split we do not have. Absent an SPXW lane there is no SPX figure at all rather than a guessed one.

Eight new assertions in `test_levels.js` (85 total). No new suite failures.

## v11.10 — 2026-08-20 — Both scales on every level · the HVL's absence is now information

**Both scales, always.** The user trades ES but thinks in SPY strikes — *"so I can quickly see what key wall it
is near."* Whichever instrument the chart is on is the primary number; the other sits beside it, dimmed and
labelled (`SPY 765` on an ES chart, `ES 7690` on a SPY chart). Cash mode gets the ES figure from `irtRatio()`'s
fallback chain, and a non-live ratio is marked `≈` rather than presented as exact. No ratio at all means no
second figure — never a guessed one.

**The HVL row no longer disappears.** It was absent from the live card and that read as "we forgot to compute
it". It is the opposite: the cumulative gamma curve in that book never crosses zero, so there is no flip to
place. The row now always renders, showing `—` plus the reason, and the tooltip states plainly that the HVL *is*
the zero-gamma / gamma-flip level — the same thing MenthorQ calls the High Vol Level and InsiderFinance labels
Support/Resistance with "dynamics change if breached".

**The regime says what answered it.** "+γ dampening" is a claim about where price sits relative to the flip. With
no flip, the regime fell back to the net-gamma *sign* — a weaker, different statement — and the card presented
both identically. It now appends `(net)` when the sign answered, and omits it when a real flip did.

21 new assertions in `test_levels.js` (77 total). No new suite failures.

## v11.9 — 2026-08-20 — Levels chart legibility + no invented precision on converted levels

Both user-reported off the live v11.8 card.

**Labels on the line, centred, legible.** The 7px tag in a right-hand gutter was unreadable and the gutter cost
chart width. Labels now sit ON their line at 10px bold, centred, on a card-coloured plate so they stay readable
where the price line crosses. Coincident levels stagger horizontally instead of stacking — CR and CR0 landing on
the same strike is a normal case, not a rare one, and the old layout hid one behind the other completely.

**"Just round it."** In futures mode every level is a CASH level multiplied by a live EMA ratio, so `7689.75` was
arithmetic residue, not a futures price anyone quotes — and the ratio itself is only good to a point or two, so the
decimals claimed precision the number never had. `lvlFmt` / `lvlSpanFmt` round to whole points on the futures
scale; the cash strike and the ratio used ride in the tooltip so the level stays traceable to what was actually read.
Cash mode is untouched.

Six new assertions in `test_levels.js` (56 total). No new suite failures.

## v11.8 — 2026-08-20 — THE LEVEL SET: CR0/CR · PS0/PS · HVL · Mag, computed from our own tape

User asked for the InsiderFinance "Signals" data, the MenthorQ 0DTE-vs-weekly split, an answer on the trigger
level, and a price chart with all of it drawn on. Three things came out of checking before building.

**1. The Signals section carries no numbers of its own.** Live pull of /gamma-exposure/SPY at spot 767.44 returned
four signals: volatility@767.44 (= spot, the regime read), resistance@769.53 (= Zero Gamma exactly — this is the
HVL), magnet@765.00 (= Put Wall), volatility@765.00 (= Put Wall again). Every value is spot, zero gamma, call wall
or put wall — all four already in the header block. "Get the Signals data" is three numbers plus wording we can
generate ourselves.

**2. The trigger level and the volatility level are different families, not two versions of one thing.** MenthorQ's
gamma set is CR, PS, HVL, CR0, PS0, HVL0, Gamma Wall 0DTE, GEX 1–10, 1D Max/Min, Blind Spots. There is no trigger
level in it. Their **Risk Trigger** belongs to the swing model — an upper boundary over the next five days used as a
profit target. The **HVL is the volatility level**: where cumulative GEX flips slope, positive to negative gamma.
Only the HVL belongs on a gamma chart, so only the HVL is drawn.

**3. Their expiration filter is the CR0/CR split — but it is client-side state.** URL parameters do not drive it;
the page ships the whole chain and filters in the browser. A scraper cannot ask for one bucket. Our own feed,
however, has carried the expiry columns since v11.6 — so the split is computable natively, and that is how it is built.

**What it does.** `gLevels(sym)` returns the MenthorQ vocabulary from our tape: **CR0/PS0** from the front expiry
column alone, **CR/PS** from the longest-dated bucket present, **HVL** = zero gamma on the native node set (absolute
dollar basis when the tape carries it), **Mag** = the heaviest strike. A LEVELS card renders them in price order with
distance and percent, over an inline SVG chart where the 0DTE pair draws dashed so today-only walls never read as
structure. Collapse and chart state persist.

**The honesty rule, on the card's face.** Our chain reaches about a week; MenthorQ and InsiderFinance aggregate
years. So every read carries `reach` (0dte / week / month / chain) and the card states in its tooltip that our CR/PS
is "the structural wall we can see, not an all-expiration wall". A one-week wall is never quoted as a full-chain one.

**Enrolled, non-voting.** `levels` registers with seven questions, and they are written so they can fail: does
realised 30m range actually contract above the HVL (if not, the regime line is decoration); do CR and PS reject on
touch; **when CR0 and CR disagree, which one does price respect**; does the magnet close half its distance more often
than chance. Nothing here touches direction, the grade or any setup until the scorecard earns it.

**InsiderFinance adapter: built, tested, deliberately dormant.** A cross-origin fetch needs `@grant
GM_xmlhttpRequest`. But this script hooks `window.fetch` and `XMLHttpRequest.prototype` to capture Skylit's feed, and
that only works under `@grant none`, which runs us in page context. Any grant moves us into Tampermonkey's sandbox
where those hooks patch a wrapper instead of the page — **the tape would go dark**. Trading a working tape for a
cross-check we score at zero weight is a bad trade, so the grant change ships on its own with unsafeWindow
re-targeting and its own test. The parser is finished and tested against real captured page text (33 assertions,
including comma-grouped SPX figures and sign preservation on Put GEX) so it is ready the day that lands.

Also in this build: **v11.7 deriveFactors, re-applied** (it was built, delivered as an installer, and never pushed —
the container that held it was reclaimed). Three defects: SPXW-derived lanes were pooled with native nodes on a
%King basis though each lane is normalised to its own King; every factor ran on %King, a ratio to a *moving*
denominator, rather than absolute dollars; and the call wall required `pos===true` while the put wall required
nothing, so a negative-gamma ceiling was unreportable and `cw` came back null for whole sessions. Walls are now
chosen symmetrically with polarity recorded rather than required. `test_reco_deriv`'s old expectation was updated
with the reasoning, not silently deleted — on its own fixture the old rule picks 780 (+30) over 777 (−72).

Two smaller things the tests found: a `var atr` local would have shadowed the `atr()` function for its whole scope
(hoisting), silently freezing the level tolerance at its default; and the IF parser's 500-character minimum
rejected perfectly readable short responses as "empty" — length is the wrong guard for validity, the label check is.

New: `test_levels.js` (50), `test_derive_factors.js` (24), `test_if_parse.js` (33, fixtures from real page text).
Full suite: no new failures against the v11.6 baseline.

## v11.6 — 2026-08-20 — THE EXPIRATION PROFILE: read every expiry column, not just the front one

User-directed after reviewing a competitor's GEX page ("consider how it can be incorporated"). Their Call Wall /
Put Wall cannot be reproduced from Skylit's feed — the payload carries NET exposure per strike with no call/put
decomposition — and the behaviour those walls proxy for (how dealers hedge at a strike) we already read directly as
polarity. What IS genuinely missing is the part their walls get from spanning ALL expirations: the outer structural
boundaries. That data has been on screen since 2026-08-19 and the reader used column 1 only.

**What it does.** The main-ladder reader now parses EVERY expiry column (each rescaled to its own King, since each
column is its own book) and keeps them as `cols[{exp,pct,n}]`. `expiryProfile(sym)` buckets them — `dte0` (front),
`near` (within EXP_NEAR_DAYS=7), `far` (beyond) — gives each bucket its own walls (strongest strike each side of
price at ≥ EXP_BREADTH_MIN=25), and computes `breadth[strike]` = how many columns carry that strike with real weight.
A node with breadth ≥ 2 is STRUCTURE; breadth 1 is a today-level that dies at the close.

**One thing the tests caught in the first cut:** pooling percentages across columns to find "the" walls is invalid —
each column is normalised to its OWN King, so a front node at 100 always outranks a monthly node at 58 even when the
monthly one is far larger in dollars, and the feed gives no per-column absolute scale to correct it. The structural
walls are therefore read INSIDE one bucket (far, else near), which is internally consistent; the pooled map is used
only for breadth, never for ranking. `structuralFrom` names which bucket answered.

**Enrolled** as `exp.breadth` (rules.json 59): per bar it records the in-play node's breadth, whether it is
structural, today's walls, the structural walls, whether they differ, and whether price sits inside the structural
cage. Two questions: does a node carried by 2+ expirations hold better than a front-only node, and does price respect
the structural walls. Non-voting, nothing on the face — forward-only data, so it ships before the sessions accumulate.
`__gptsDebug.expiry()`. Tests: test_expiry_profile (18), including a single-column ladder degrading to null rather
than inventing a structural wall. Suite green except the known-stale.

## v11.5 — 2026-08-20 — ACCUMULATION IN DOLLARS (shadow) · one record per roll STEP · the edge path

Spec: design/spec-v11.5-dollar-accumulation.md. User-directed after the live session: "see support and resistance
rolling up and down knowing their impact on price." **No face change** — this build only adds recording.

**Why.** %King is a ratio to a MOVING denominator. Live 2026-08-20: the King (765) grew **112,611 → 142,214 in
dollars (+26%)** between 10:18 and 10:54 while the Map printed `765 hold, m15 0` — the King's percentage is 100 by
definition, so it can never accumulate and a roll INTO it can never fire a transfer. Worse, while the King grows every
other node's percentage shrinks even at flat dollars: the Map read `766 dec −14 · 764 dec −19 · 763 dec −12` during a
session in which the floor was rolling UP into 765. Net effect for the day: **44 ceiling transfers to 1 floor transfer**.

**(a) The dollar basis, in shadow.** `feedSeriesAll` now carries the raw absolute exposure (`a[]`) beside the %King
series (`v[]`). `ledgerStateAtAbs` runs the SAME state machine (ACM_UP/ACM_DN/ACM_DROP via `mapNodeState`) on dollars.
Per node the ledger records `absState / absM15 / absFromPeak / absCur / absPeak`; `nodeFlow` adds `absState`/`absM15`
per node plus `transfersAbs` / `leanAbs` — the transfers the Map WOULD have seen. Nothing on the face reads any of it.

**(b) One record per STEP.** `map.transfer` fired every bar a roll stayed active — 19 distinct steps became 45 records
on 2026-08-20 (`ceil 767→768` counted 8 times), inflating n and making the observations dependent. Records now carry
`stepNew` / `stepT` / `barsActive`, the question is scored on `stepNew` only, and a second question (`transfer_basis`)
asks which basis called the roll price actually followed on the bars where the two disagree.

**(c) The edge path.** `fcHistSample` has recorded the floor/ceiling STRIKE every bar since v10.51 and nothing ever read
it. `edgePath()` collapses today's series to its changes — 763 → 764 → 765 with timestamps — plus step counts,
directions and a read (compression / expansion / drift-up / drift-dn). Rides on the `map.transfer` record as `edge`.

**Also:** the ledger's `firstT` returned 0 (epoch) for native strikes, which made PB Entry's `fresh` flag meaningless
for them — a missing stamp is now null (unknown), never 1970.

Tests: test_dollar_accum (15) — the King reading `hold` on %King and `acm` on dollars, phantom dissipation reproduced
and separated, real decay still `dec` on both, the floor roll the %King basis missed, 8 bars = 1 step, and the
763→764→765 collapse. test_node_ledger harness updated. Suite green except the known-stale.

## v11.4.4 — 2026-08-20 (live, 10:20 CT) — TWO REAL MISREADS IN THE TRINITY PANE: velocity read as %King, and colour used for polarity

Found live while checking an ES chart. The panel went out of sync with 767 reading **117%** against a $K-tagged King at 100% —
impossible by definition, and the parse invariant (INVARIANT 2, "no other strike may meet or exceed the King") caught it.
**Cause 1 — cell order guessed instead of known.** The Trinity pane writes VELOCITY first then %King ("+15%62%"); the main table
writes %King first then velocity ("31%-7%"). v11.2 guessed by "signed then unsigned = velocity first", which works until the
value itself is NEGATIVE — Skylit prints those with a UNICODE minus ("-76%−3%"), so both tokens looked signed and the fallback
took the VELOCITY as the value. Every −γ strike in the pane has been carrying its velocity as its %King since Tuesday. The
caller now STATES the order (`ladderCellParse(cell, valueFirst)`); nothing is inferred.
**Cause 2 — polarity inferred from the cell colour.** v11.2 read blue-ish cells as −γ, but Skylit's ramp is a viridis scale over
the VALUE (deep purple = most negative … blue/teal = small … yellow = the King), so a small POSITIVE strike is blue: 770 at
+19% was being recorded as −19%. Polarity now comes only from the rendered sign (ASCII '-' or Unicode '−'; absent = positive),
and the King row takes its sign from its own dollar figure ("−$247,657K" = a −γ King). This also removes a hidden dependency on
a palette Skylit can restyle at will.
Fixtures in test_ladder_dollar (23) are the live 10:18 CT SPY pane verbatim. NIGHTLY NOTE: `tri[SYM].top` percentages recorded
2026-08-18 → 2026-08-20 10:20 CT are unreliable for NEGATIVE strikes (velocity-as-value) and for small positives (colour flip);
`pct` on the native SPY/QQQ books (feed-derived) is unaffected, as are the ledger, Map, leg engine and every scored feature —
they read the FEED, not the pane. Suite green except the known-stale.

## v11.4.3 — 2026-08-20 (live, 09:35 CT) — IRT export: an SPXW-derived lane no longer poses as a SPY wall

First live look at the generated CSV (after v11.4.1 made it produce rows) showed "Ceil 100%" sitting two strikes from
"K 100%". Cause: v10.58 normalises each derived book to its OWN King, so an SPXW lane can read 100% on a scale that is not
the SPY scale — and the exporter was giving it a SPY role word and a wall's weight. Derived lanes are now labelled by their
book with their own percentage ("SPXW 100%"), drawn thin, dotted and slate, and are never called K/GK/Ceil/Flr; native SPY
strikes are unchanged. Verified against the live 09:31 CT board: King 767 ($77,015K), lanes at 767.5/768.5 tagged SPXW,
ratio source named in the gear. Tests: test_irt_export (22).

## v11.4.2 — 2026-08-20 — SYMBOL SCOPE: another instrument's book can no longer be recorded as SPY

Found while answering "does this work for USO / GLD?". `symFromUrl` returned `'SPY'` for ANYTHING that was not QQQ, so
charting any other optionable instrument fed that instrument's `gex/levels` book into `LASTFEED.SPY` — and
`recordNodeSnapshot('SPY')`, which runs every tick regardless of what is charted, wrote those levels into the SPY day file
under the SPY name. The face was honest (futMode showed "No options tape"); the RECORDING was not. Now the true symbol is
parsed from the URL and `onFeed`'s existing SPY/QQQ guard drops everything else; sightings are counted
(`__gptsDebug.symbolsSeen()`) and the unmapped-instrument banner says the honest thing: "<SYM> has a Skylit options tape,
but this panel is mapped to SPY/QQQ only — nothing here is read or recorded for <SYM>." Nothing is invented for the
unsupported symbol. Tests: test_symbol_scope (12). ANSWER TO THE QUESTION: no, the panel does not support USO/GLD today —
`RECORDER_SYMS`, `STATE`, `LASTFEED/LASTVEX/LASTDISP`, `TAPE_CACHE`, `RECON_STATE`, `FUT_UNDERLYING`, the ledger, the leg
engine and the day-file schema are all keyed to SPY/QQQ. Generalising them is a real build (a symbol registry + per-symbol
tick/ATR/session assumptions), NOT a config change, and it is a FEATURE — it waits for the lockdown or an explicit call.

## v11.4.1 — 2026-08-20 (live, 09:20 CT) — IRT export works on a CASH chart; the King-dollar keeps recording

Live check at the open found two gaps. (1) `irtBuildCsv` took its conversion from `FUTMODE.r`, which is 1 while a CASH
chart is up — so with the user on the SPY chart (the normal case) the export produced nothing. The ratio is now
independent of what is charted: live ES EMA when a future is charted (and persisted to `gpts_irt_ratio_v1`) → the last
persisted ratio (≤14 days) → the feed's own SPXW→SPY ratio → the ES constant; anything but live marks futures labels '~',
and the gear status line names the source. (2) Since v11.2 the Trinity pane wins the tape read, and only the main table
carries the `$K` in a later expiry column, so `snap.bk` (the King-dollar trend, v10.39's "strongest leading signal on the
board") had gone null again — the Trinity read now carries the main table's `bookKing` across, or falls back to its own
King $K. Live at 09:13 CT: sync unanimous 767 (tag/feed/tapemax), all four ladders read by their $K (SPY 767 $70,499K ·
QQQ 714 · SPXW 7705 · VIX 16), 0 feed rejects, 186 feature records over 6 bars with 58 already resolved, `tri` on every
snap, PB Entry 764 map.pb Acm with a 4-node stack, Next Stop 766.5. No panel errors in the console. Tests: test_irt_export
(18). Suite green except the known-stale.

## v11.4 — 2026-08-19 (evening) — IRT FLEXLEVELS EXPORT (user-directed): the gamma levels drawn on Linnsoft Investor/RT charts

User: "build a feature that exports the gamma levels so I can import them on Linnsoft IRT charts… flex levels… consider
exporting to my Google Drive desktop and Linnsoft FlexLevels picking it up from there… toggle in the settings gear…
and the timing 1m 3m 5m." Format pinned verbatim to the user's own FlexLevelsExport.csv sample (28 columns; PENCOLOR =
Windows COLORREF 0x00BBGGRR). Levels written: King (gold, w3) · Gatekeeper (white) · Ceil (red) / Flr (green) · strong
magnets and every node ≥ nodeThresh (gray; −γ purple, label "-g") · Next Stop (blue, dashed) · PB Entry (orange, dashed,
state carried). Symbols: a FUTURES symbol as IRT charts it (user-edited, rolls quarterly, e.g. EPU26) converted with the
live ES/SPY ratio (0.25-tick rounding; "~" marks a last-known ratio when the Skylit chart is not on the future) and an
optional ETF symbol at raw strikes; FlexLevels' "Auto" symbol matching picks the right rows per chart. Delivery: the
panel writes `FlexLevelsExport.csv` into a folder picked ONCE (File System Access API handle persisted in IDB `irtDir` —
same mechanism as the repo data folder). Best target: `C:\Users\<you>\InvestorRT\rtx\lsFlexLevels\` (IRT reads local
FlexLevels files from there, per linnsoft.com/techind/flexlevels-rtx); a Google-Drive-synced folder works for a second
machine; the documented alternative ("the other way") is FlexLevels' REMOTE PATH — a URL the indicator polls. Gear block:
Export levels toggle · cadence 1m/3m/5m/15m · Fut/ETF symbol fields · 📁 folder · ⟳ now · last-export status line.
`__gptsDebug.irt()` / `.irtExport()`. Export utility only — draws lines, makes no claim, nothing to enroll. Tests:
test_irt_export (13). Suite green except the 4 known-stale.

## v11.3.3 — 2026-08-19 (live, ~14:40 CT) — FEED FRESHNESS GUARD: a historical payload can no longer poison the live book

Found live: a 389-snapshot gex/levels payload ending 2026-08-13 (Skylit fetches history through the same endpoint for higher
timeframes / panning / Replay) replaced the live feed mid-session — STATE.price jumped to a 6-day-old 777.66 while the tape read
769, the sync gate went "unanimous" on the replayed King (all three votes drink from LASTFEED on non-GEX display), and poisoned
bars were RECORDED into today's file. Fix in `onFeed`: a payload may never replace a fresher one — its newest snapshot must be
≥ the held payload's newest minus 90 s (out-of-order minute jitter allowed); at boot with nothing held, anything is accepted as
the last-known view and the live feed takes over on arrival. Same guard on the vanna capture. Rejections are counted and
surfaced (`__gptsDebug.feedRejects()`, one console line per 20). NIGHTLY NOTE for 2026-08-19: bars recorded roughly 13:40–14:40 CT
carry px≈777.66 from the stale payload — the review must quarantine snaps whose px deviates >2% from neighbours (dataHealth).
Tests: test_feed_guard (8); test_mode_king harness updated. Suite green except the 4 known-stale.

## v11.3.2 — 2026-08-19 — PB Entry line wraps: gray context moved to a smaller second line (user-directed)

The deflection/zone/stack text ran off the panel. Line 1 = level · pts · state · grade (tight); line 2 = 8.5px gray,
wrapping: "· defl ↓ → 7710 (stack of 3: 7716–7717.5) ✓↓ · 54% @30m".

## v11.3.1 — 2026-08-19 — PULLBACK CONTEXT on every PB Entry record (user-directed): depth · SMA-50 frame · pullback speed · node freshness/roll · STACKED nodes

User: judge "the amount of pullback, the distance from the 50-SMA, whether the node is above or below it, the quality of the
pullback — speed/momentum or not," and the node's own story — "rolled, increasing or decreasing in accumulation, fairly new
where dealers had intentions of stopping the pullback"; plus "many times there are 2-3 pullback nodes stacked close together —
judge which is stronger, or the one with more retracement." All recorded per bar on the pbEntry record (non-voting, no face
change except the stack zone note): `pb{depth (retrace/leg swing), swing, retrace, bars, paceAtr (pts/bar in ATRs), counterShare,
fading}` · `sma{sma, d, dAtr, levelD, levelDAtr, nodeSide above|below|at (±0.2 ATR)}` · `nodeCtx{age, fresh (≤45m — dealers
stepping in), m15, fromPeak, rolledFrom, step}` · `stack{n, members[{k,pct,state,m15,depth}] strongest-first, strongest, deepest,
chosenRank, span}` when ≥2 qualifying nodes sit within 1.5 strikes on the pullback side. Face: "(stack of 3: 7716–7717.5)" names
the area. Five new questions: pbentry_depth (shallow ≤0.5 vs deep), pbentry_pace (grind vs impulse), pbentry_smaside (node AT the
50-SMA), pbentry_fresh, pbentry_stack (strongest vs deepest vs the zone — chosenRank grades the picker). Forward-only fields —
shipped before more sessions accumulate. Tests: test_pb_entry (30). Suite green except the 4 known-stale.

## v11.3 — 2026-08-19 (live, 12:30 CT) — PB ENTRY under Next Stop (user-directed): where to look for the pullback / deflection — enrolled from day one

User: "under Next Stop I want a PB Entry section… it tells me where to look for a pullback entry… trade location for a potential
reversal (deflection) — track this over time for improvement analysis and testing." Face: one line under Next Stop —
`PB Entry: ↑ 7716.25 · +2.9 pts · Acm · defl ↓ → 7710 · B` (level green above price / red below like Next Stop; node state chip;
deflection arrow + the Next Stop it points to; "(zone lo–hi, forming)" when only a predicted zone exists; the ✓/✗ latch when one
is on; grade right-justified). Hover = why this level, how it is scored, measured rate with n only once n≥20, descriptive.
Picker: ① the leg's detected PB (✗-latched = broken → skipped, and not re-offered as the last PB) → ② the predicted PB zone (far
edge, zone carried) → ③ the leg's last PB on the pullback side → ④ no leg: the accumulating node on the far side of price from
the Next Stop within PB_REACH → ⑤ the nearer wall/gate on that side; nothing qualifying = no line. Grade B = leg + Acm + SMA
agrees + within reach; chop / mid-range / Dec node → C; A by promotion only. ENROLLED: FEATURES `pbEntry` (30m) + `pbEntry.60`
(60m); outcome SEQUENCED on the new `fwd.path` (touch first, then DIR_PTS away before a close through by > DEFLECT_ZONE;
never-touched = null, not a miss); questions pbentry_30/60, pbentry_acm, pbentry_pol, pbentry_touched; rules.json 58 ids;
Analysis ① tiles "PB Entry 30m/60m"; LLM brief paragraph. `__gptsDebug.pbEntry()`. Tests: test_pb_entry (21); suite green
except the 4 known-stale. Label "PB Entry" is the user's word; the hover and the record speak of the deflection only — no
size/stop/side anywhere.

## v11.2 — 2026-08-19 (live, 11:30 CT) — THE $K-ANCHORED LADDER READER: find the King by its dollar first, then read the tape · Trinity SPY/QQQ/SPXW/VIX read for the first time

**The user's principle (2026-08-19):** "locate the King first by its dollar amount; with that you can read the tape no matter what
the UI does, because the King dollar is the one consistent thing." Built exactly that way. `readLaddersByDollar()` finds every
innermost element carrying a `$K` figure, walks up to its ROW (first ancestor with ≥2 children one of which is a bare strike), up
to the CONTAINER holding ≥15 such rows, learns the column shape FROM THAT ROW (which cell is the strike, which holds the $K), and
reads every sibling row the same way. No dependence on `<table>`, headers, class names or colour — those are hints. Cell grammar
learned from the live page: "+8%$92,931K" (King: velocity + dollars), "-1%65%" (velocity, then %King), "59%"/"−3%" (lone %King),
"24%-7%" (main table: value, then velocity); the %King is the token that is NOT a signed velocity chip; polarity falls back to the
cell colour (Skylit paints −γ purple/blue) when the value carries no sign. Symbol = nearest ancestor whose text starts "SPY$"/"QQQ$"/
"SPXW$"/"VIX$"; a ladder inside a `<table>` is the main table ('main', shows whichever symbol the chart is on).
**Result on the live page 11:20 CT:** Trinity SPY King 769 ($89,164K) · QQQ King 717 (−$247,657K, a NEGATIVE King, read −100) ·
SPXW King 7725 ($72,781K) · VIX 15.5 · main table King 769 (yellow) with bookKing 775/08-21 −$320M — all from their $K cells.
`readTapeFromDOM(sym)` now tries the Trinity pane for that symbol first (true %King, King by $K), then the main table, then the
legacy finders; `tapeMap('QQQ')` works for the first time (deferred item 6 pulled forward by the user's request). Recorded per bar:
`tri:{SYM:{king,kd,n,top8}}` for every pane. The out-of-sync banner hover now lists the ladders found by $K with their Kings.
`__gptsDebug.ladders()`. Tests: test_ladder_dollar (14) — Trinity div rows, main table, `<ul>/<li>` with the strike in the 2nd
cell, hover $K → flagged. Version pins → 11.2. Suite green except the 4 known-stale. LOCKDOWN: this is Layer-0 reading, no
face change; the SPXW/QQQ ladders are DATA from today (enrollment of a cross-index feature is a separate, later decision).

## v11.1.3.1 — 2026-08-19 (live, 10:45 CT) — the $K cell is back in the data: BOOK KING captured wherever it sits

User: "adapt to the UI changes by identifying the King with its dollar value." Probing the live ladder for 15 s showed the $K
cell IS permanent — it marks the largest ABSOLUTE cell of the whole displayed book and in the EXPIRATIONS view sits in a
LATER expiry column (775 on 2026-08-21, −$341,276K, the dark-purple cell) while the yellow cell is the nearest-expiry King
(774 = the feed's book = the Trinity badge). The reader now captures `bookKing {k, col, expiry, kd, neg}` wherever the $K
cell is, plus the expiry headers; in column 1 it is the legacy King tag exactly as before. Recorded per bar as `bk` next to
`kd` (the King-dollar trend — "the strongest leading signal on the board" in v10.39 — had gone dark when the layout changed;
it is data again, under its true name). Test 6a–6d in test_tape_v1113 (17). Version pins → 11.1.3.1.

## v11.1.3 — 2026-08-19 (live, 10:20 CT) — HOTFIX: tape sync red all morning — three faults in the King reconciliation

**What the user saw:** "⚠ Out of sync" from the open with EXPIRATIONS on (the view the user trades from — turning it off hides the
nodes on the chart). Recording continued; only the structural read was suppressed.
**Faults (all in Layer 0, found live from the DOM):** (1) `kingFromFeed` read `levels[0]` — the OPEN minute of a whole-session
payload — so the feed vote was frozen on the 09:30 King (772) while the live King had moved to 774; (2) Skylit's ladder no
longer prints a permanent `$K` King cell (the King cell shows a % with the yellow highlight; the $ figure appears only under the
mouse) and, in the EXPIRATIONS view, the column values are each strike's SHARE of the book (sum ≈ 100), not %King — so
`findTapeTable` rejected the table (no $K) and the panel fell back to `feedStructMap`; (3) `feedStructMap` included the
SPXW-derived lanes, which v10.58 normalises to their OWN King, so a lane at 770.5 = 100% out-voted the real King in
`kingFromTapeMax`. Three votes, three answers, permanent no-consensus.
**Fixes:** feed vote = latest minute (max t); a real `<table>` with a Strike header + ISO expiry headers + ≥15 strike rows is
the ladder even without a $K cell; column 1 (nearest expiry = the feed's book) is read as before, rescaled so max|v| = 100
(= value/King, the %King the panel has always meant), the yellow cell is the King tag (`kingSrc:'highlight'`), and a $K cell
that is not the yellow cell is treated as a hover read-out (unknown for that tick) instead of crowning a strike; the tape
stand-in from the feed carries native strikes only. Verified against the live 10:05 CT DOM: King 774 · 769 50% · 768 40% ·
771 −30% — matching the feed ($113M / $56M / $45M). Tests: test_tape_v1113 (13) new; version pins → 11.1.3. Suite green
except the 4 known-stale. LOCKDOWN unchanged (fix). The planned dot / ledger / scenario items move to v11.1.4.

## 2026-08-18 (night) — DOCS: Skylit Public API reference captured (`skylit-docs/api/API-REFERENCE.md`), added to the load protocol

REST (`api.skylit.ai/v1/heatmap` · `/v1/historical` · `/v1/stream`) + MCP (`mcp.skylit.ai/mcp`, `heat_*` tools), auth/credits/
rate limits, full response shape, and a project-impact section (backfill budget ≈ 650 credits per session at 3 m, what the API gives
vs what Layer 0 scrapes, MCP-connector option, open decisions). `.gex-config.json` and the skill load list now include it.

## 2026-08-18 (night) — REVIEW LOOP AUTOMATED (no code): Drive inbox · scheduled nightly + weekly · local pull task

Drive folder `GEX-review-inbox` created; Google Drive for desktop installed on the trading PC; `tools/review-pull.bat`
+ scheduled tasks "GEX review pull" (daily 16:05 CT, Sat 10:30) copy inbox files into `learning/log/` · `learning/rules.json`
· `review/` (append-only for logs/reviews) and push. Two scheduled tasks in the Claude app run the LLM reviews unattended:
nightly weekdays 15:45 CT (contract 1 → `learning/log/<date>.json`), weekly Saturday 10:00 CT (contract 2 → `rules.json` +
`review_<date>.json`). Verified end to end with the 08-18 log (skipped as already-in-repo = path resolves). Daily chain is now
15:01 export → 15:30 push → 15:45 review → 16:05 pull → panel ⑥ REVIEW by ~16:15, with no manual step. Code changes still
ship by installer .bat + Tampermonkey click, by design. Skill DELIVERY CASCADE updated.

## v11.1.2 — 2026-08-18 (evening) — FIX: the feature queue deleted itself every bar (nothing ever resolved since v10.54)

**Found by the first nightly (learning/log/2026-08-18.json):** the 08-18 export's resolved-outcome queue held 27 records —
one per feature, all from the 15:00 bar, none resolved. Root cause in `featEnqueue`: the v10.54 "cap by bars" cutoff was
`maxBar - FEAT_KEEP_BARS`, but `bar` is the candle's MILLISECOND timestamp, so the cutoff was maxBar minus 160 ms and every
enqueue deleted every record but the bar that had just landed. Consequences: `resolveFeatureOutcomes` never had a record old
enough to score, the IDB `FEAT_ARCHIVE` stayed empty, every Analysis/Testing scorecard sat at n=0, and the promotion bar could
never see local n. Fix: cut at the FEAT_KEEP_BARS-th most recent DISTINCT bar value. Coupled fix in `recorderSave`'s quota
fallback: it deleted the oldest day even when today was the only day (i.e. it deleted today — snaps and queue in one call);
now it drops the oldest NON-today day, or sheds the oldest half of today's feature queue, never today. Tests: test_feat_keep
(13) new — 200 bars enqueued → 160 distinct bars kept, resolution works once the window closes, quota never deletes today;
test_export_full 5b/5d re-pinned; version pins → 11.1.2. Suite green except the 4 known-stale. Nightly log dir now has its
first entry (08-18); rates in it were recomputed from snaps because of this bug. LOCKDOWN unchanged — this is a fix.

## v11.1.1 — 2026-08-18 — Next Stop styling: green above / red below, signed points, grades right-justified

User-directed: the Next Stop level is green (↑) when above price and red (↓) when below, with the signed distance in points beside it; its grade and the read's grade sit at the right edge, level with each other; the "Why this level?" hover unchanged.

## v11.1 — 2026-08-18 — NEXT STOP: the one forward call, above the read (user-directed, enrolled from day one)

"Next Stop: 7721.25 · 30–60m · B" — the level price is expected to reach next, with a confidence grade; hover
answers "Why this level?" (rule, distance, measured hit-rate with n once it exists). Picker, until data teaches
otherwise: leg magnet → pullback-node target when the ✓ is latched → the accumulating node in the Map's lean →
the next wall on the King's side → the nearer wall; grade B when leg and SMA-50 agree and the level is within
~2× a 30-minute leg, C for structure-only / chop / mid-range, A only by promotion. Never invents: no nodes = no
line. Recorded at two horizons (`nextStop` 30m, `nextStop.60` 60m — the first feature with its own 20-bar
window): hit = touched within the contact zone, approach 0..1, wrongFirst; Analysis ① tiles, ⊕ scorecard by
rule/grade, rules.json (56 ids), nightly brief asks for grading by rule and re-ordering proposals through the
promotion bar; master-spec §30. Tests: test_next_stop (20). Suite green except the 4 known-stale.

## v11.0.2 — 2026-08-18 — levels said once: "Ceiling rolling down from 7731.25 Dec to 7721.25 Acm."

User-directed wording fix to every rolling sentence: the levels carry their state the first time they are named
("from X Dec to Y Acm") instead of being repeated after a colon. test_one_read re-pinned.

## v11.0.1 — 2026-08-18 — ONE READ (voice fix inside the lockdown)

User-directed: the read and the structure text are one paragraph now — leg sentence first (the 15, unchanged),
then ONE structure sentence in the user's words, then the caveat; no "Map:" label, no separate ⚑ banner. On the
bar a pullback node lands the read itself takes the ⚑ style (red down / green up) and the sentence carries the
banner's detail: "Resistance pullback node formed at 7716.25 rolled down from 7726.25. Deflection expected to
target 7710." The roll-count words ("2nd step, signal") are gone from the face (hover only). Structure sentences
(`mapSentence`, twelve situations): "Ceiling rolling down from 7735 to 7730: 7735 Dec, 7730 Acm. Pullback node
likely at 7730." · ceiling rolling up ("Room above to …") · floor rolling up/down · both down/up ("structure
leaning …, Pullback node likely at …, Magnet …") · compression · expansion · converging ceilings/floors · middle
emptying · magnet moving · holding. Words are Acm (green) / Dec (red), never "accumulating/dissipating". A
pullback node or magnet the leg sentence already named is not repeated. Futures levels round to the instrument
tick (ES/NQ 0.25: 7716.36 → 7716.25). Nodes on watch: the PB tag is plain "PB" (roll step in the hover), the
SPXW tag is off the rows (hover only). Recorded: `dir.read.map` = the structure sentence, `structureId`.
Tests: test_one_read (24); test_leg_engine / test_futures_mode / test_node_map re-pinned. Suite green except
the 4 known-stale.

## v11.0 — 2026-08-18 — LOCKDOWN: the stack, the node ledger, the learning path made trustworthy, −1,170 lines of dead code

**Why.** A whole-file audit (design/ARCHITECTURE-AUDIT.md — four independent line-range passes over 14,369 lines
and 664 functions, every block's callers grepped) reached one verdict: the core is sound — feed capture, tape sync
gate, SMA five-state, node map, leg engine, latched trigger, Map, FEATURES registry, effN/promotion, exporter — but
the app had grown by accretion. ~1,600 lines were unreachable from `render()`/`tick()`; the same idea was computed
several ways (accumulation ≥6 readings, direction tilt 4, King movement 3, node identity ~10, handoff ≡ Map
transfer); and the learning loop had never closed end to end: no day file had reached `data/`, the nightly output
was never read back, two user-priority outcomes resolved as false misses after a reload, and the promotion bar was
structurally unclearable once repo history exceeded the localStorage window. Not a reason to rewrite — a reason to
fix the recording path first, add the missing base layer, collapse duplicates to one reading each, remove the dead
code, rewrite the docs around one stack, and lock.

**What.** The stack (master-spec §0, read first): 0 FEED+TAPE · 1 NODE LEDGER (new) · 2 STRUCTURE (Map, leg
engine, one threshold set) · 3 DIRECTION · 4 SETUP · 5 OUTCOME+LEARNING · 6 REVIEW · 7 VOICE. Every function
belongs to exactly one row. **The node ledger (§27):** `ledgerBuild` (pure) / `nodeLedger(sym)` from
`feedSeriesAll` — the feed's own per-minute session snapshots for SPY strikes AND the SPXW-derived lanes — plus
closed candles: per node its life (born / peak / now / gone, `life{build,after,goneFor}`), state acm|dec|gone|hold
from the same thresholds the Map uses, every touch with its reaction (deflect / through / stall, decided by the side
the bar OPENED on and the DEFLECT_ZONE; the node's state AT the touch kept), and influence (while acm did price come
toward it over 5 bars, while dec did it move away). Recorded as `ledger.touch` (question: do accumulating nodes
deflect more than dissipating ones — if yes the acm/dec chip is a filter, if not decoration), exported as `ledger`,
shown in Analysis ⑦ NODES (per node life · now · touch d/t/s · toward% (n) · away% (n), the two pooled questions
under the table, dashes until n≥5) and in the node hover. `accumCanon` reads the feed series first; the tape strip is
only a faster confirm.

**Fixes (the learning path, §28 — this is what "collect data" depends on).** `LEG_PB_LOG` persisted per day;
outcomes that depend on an absent log resolve **null**, never 0. Resolved feature records mirrored to IndexedDB
per day (`repoUpsertFeat` → `FEAT_ARCHIVE`, merged by `featStats`) so local truth outlives localStorage.
`proposalClearsBar` drops the "reviewer n within 20% of local n" self-report test — the bar is local eff n≥20,
3 walk-forward sessions, no regime flip. `dirFactorStats` falls back to `dirNum` for the vote (handoff / trigger /
pbDetect rows get a vote split); `leg.roll` records the vote the grade actually used. `ruleLocalRate` maps
`node.tap.*` / `node.pol.*` / `node.rocDay.*` to the ④ DEFLECTIONS slices and `kill.*` to its feature key
(`deflStats`). The READ sentence + voiceId + leg dir/phase + dirSrc + Map line are recorded on the `dir` feature
(`LAST_READ` → `read{}`), so READ-vs-direction is measurable per bar. `pipeCheck` also fetches
`learning/log/<day>.json`; Analysis ⑥ REVIEW renders the nightly (headline/brief, contradictions, factors,
questions) beside the weekly v2 fields. One export path (`data/<date>.json`; auto-export success flips the save
banner). One version constant `GPTS_VERSION` (header, footer, export, logs). `resolveFeatureOutcomes` once per
tick. Click delegation wired before the futures early-return — the Dashboard tab from Testing works again.

**Merges.** `drift` → `dir.drift`, `roll` → `dir.kingRoll` (one record each; `defl_ant` / `reaction` stay SHADOW).
One accumulation threshold set — `ACM_UP=8`, `ACM_DN=−8`, `ACM_DROP=25` — shared by ledger, Map, leg engine and
handoff (`ACM_BAND` / `HANDOFF_*` / `MAP_*` gone); the leg engine's `to` node must actually be building, not merely
≥PB_MIN_PCT. The handoff reads `mapNodeState`, so it fires on the same evidence as a Map transfer (`leg.handoff`
keeps the lead-time claim, `map.transfer` the price claim). `accumBlock` → `nodeMapBlock`; `_frameRecOf` once per
bar; `nodeMapModel` memoised per poll. **Analysis tab:** ① HEADLINE · ② WHAT CHANGED = PROMOTED only (proposals,
challengers, kill list live in Testing ②③④) · ③ DIRECTION FACTORS · ④ DEFLECTIONS · ⑤ YOUR CALLS once (out of ⊕) ·
⑥ REVIEW nightly + weekly · ⑦ NODES (ledger); the pipeline health moved to the footer dots + hover.

**Removed (−1,170 lines; data preserved where any; PARKED markers where a test still pins the code).**
setupGrade; the debug-only reader / setupHealth / pathQuality / nodeQuality / absAccumStateFor; gridFor / signalGrid /
combinedGrid + tips; readLine; structuralReadHtml + structuralBox / structuralWarn; readBlock; confluenceStrip /
confluenceThesis; accumData; recordSession / gpts_stats_v7; exKingInfo; the legacy deflStats; LOGCACHE / syncLog
reader; acmChipHtml; breakoutConviction, trendIsUpish/Dnish, trendCodeOf, saveLog/LOG, outOfSyncBlock,
parseKingDollarSign, firstStrengthPct, countBarsSince, stageTimeline, accTrajHtml / histDetail, gatekeeperBlock /
triIndexNote, nodeTypeTag, deflectionBlock, nodeMapSentence(+Html), _nmRole/_nmB/_nmAcc, studyCite / studyTag,
dispVal, dirColor, stepIcon, ruleGet, _escHtml, tapCol, gradeChipHtml, activityPillHtml, zoneGGlyph,
readOddsAllowed, the identity gradeDisp, timelineSvg, convergenceSvg, _kpi, _accBar, the analysisStats body,
testingInsights prose, projScorecardHtml / projRecs, the write-only SMA_CONT_FLAG / LASTNODEMAP / PREVKING / FUT +
lastCloseOf; the trendBadgeHtml duplicate. The kingBlock chain (kingSparkline, kingPathSigMoves, kingReadHtml,
projChartHtml, projTaperHalf, sessionBoundsCT) and the other test-pinned pieces are kept as PARKED, not live.
NODEHIST is now the ledger's touch source (no longer write-only).

**Docs.** master-spec §0 (the stack + standing rules + LOCKDOWN), §27 (ledger), §28 (learning-path fixes);
docs/LLM-NIGHTLY-BRIEF.md reorganised by layer with LAYER 1 — NODE LEDGER, the merges, `dir.read`, the nightly
read-back fields, the local bar; REVIEW-ACCEPTANCE (f); skill v11.0 line; resume note.

**LOCKDOWN.** No new features until ≥20 sessions of data exist; only fixes ship. The review's job for those
sessions is measurement and proposals.

**Tests:** test_node_ledger (20) new; suite green except the 4 known-stale.

## v10.58 — 2026-08-18 — THE MAP: node flow (dissipate → accumulate → influence), SPXW lanes on the board

**Why.** Two rolls the user circled today (768.5 dying while 769.5 and 767 built at 10:40; the ceiling handing off
769.5 → 769 → 767 from 12:05) were invisible to the engine: (1) the SPXW-derived lanes (the diamonds on the chart:
7715 → 769.5, 7690 → 767 at today's ratio) were scaled against the SPY King magnitude — 0.7% — and dropped under
MIN_STRENGTH; (2) a node that dissipates DROPS OUT of the feed's top-N list and "absent" was read as "no data";
(3) the handoff only ran inside a leg, and the leg only ran when the SMA-50 said trend.

**Fixes.** `extractWalls` normalises each derived book to its own King and tags rows `src:'SPXW'` (integer
strikes stay native). `feedSeriesAll` reads the whole session's per-minute snapshots straight off the feed for
every strike in both books, drop-out = 0; `accumCanon` falls back to it when there is no tape strip (all
SPXW lanes). `nodeFlow` (the Map): per-node acm / dec / gone / holding, transfers on both sides, widening, lean —
always on, independent of the SMA. **Face:** "Map:" line under the READ in the user's words (green acm / red dec,
no arrows), acm/dec/holding chip and SPXW tag on every Nodes-on-watch row. **Structure leads:** when the five-state
has no trend the Map lean drives the leg engine (dirSrc map) and the direction spine (relation `structure-leads`,
capped C); the READ adds "SMA-50 has no trend: structure leads, trend unconfirmed" — or "structure rolling against
the trend — caution" when they disagree. **Recorded:** `map.transfer` (did price move the roll's way) and
`map.lean` (does the lean predict / lead the SMA), rules.json 55, LLM brief, master-spec §26.
**Tests:** test_node_map (37). Suite green except the 4 known-stale.

## v10.57 — 2026-08-18 — NODES ON WATCH · drift to SHADOW mode · Dashboard-tab fix

**Nodes on watch** (was "Deflection zones"). User: "only nodes in play and relevant nodes." Under the in-play
card the ladder now lists only nodes with a job — `nodesOnWatch()`: the leg's PB, its TGT (magnet), the node
building to be the `next PB` while a handoff is active, the King, then the nearest meaningful `next wall` above
and below price. Rolled-off levels, thin strikes and anything beyond PB_REACH are out; no leg = King + nearest
ceiling + floor. Cap 4 rows. Header hover: "Why these nodes?".

**Drift → SHADOW mode.** User: "remove it until it is tested and proven." `DRIFT_LIVE=false`: the Drift row is
off the face and drift no longer votes in the direction hierarchy (relation is trend-only / tentative; no
divergence cap; no tentative lean — no trend = SIDE). It is STILL computed and recorded every bar: `dir.drift`
as before, and `dir.relation` now records the SHADOW relation/direction (what drift would have said) so the
"does drift lift the trend" question stays measurable. The READ drops the "GEX and VEX lean" beat; the
direction hover says why. Promotion path (n≥20 effN, 3 sessions, both up and down days) brings it back —
flip DRIFT_LIVE when the review clears it. Hierarchy tests pin the promoted behaviour with DRIFT_LIVE=true.

**Fix.** Dashboard tab from the Testing tab did nothing (it only cleared the Analysis flag) — now
`showDashboard()` clears both. **Tests:** test_nodes_on_watch (28). Suite green except the 4 known-stale.

## v10.56 — 2026-08-18 — the READ voice (user-authored) [10.56.1: tags TGT/PB, sentence 7 drops 'target'] · dissipation HANDOFF · latched ✓/✗ trigger · clean in-play card

**The handoff (user priority).** The roll is a STRENGTH transfer before it is a strike change. `legStep` now
detects `handoff {active, from, to, since, leadBars}`: the current pullback node / nearest ceiling BLEEDING
(m15 Dec ≤ −8% or ≥25% off its session peak) while a lower node above price BUILDS (m15 Acm ≥ +8% or ≥
PB_MIN_PCT — a still-thin building node is now kept aside for this search). Flagged a bar or two before the
new node qualifies; resolves into `pbDetected` (then `from` is rolled off) with the lead time recorded.
Uptrend = mirror (floor building higher). Recorded as `leg.handoff`, seeded in rules.json, in the LLM brief.

**The READ voice.** When a leg is active the leg speaks first, in the user's own 15 sentences verbatim
(numbers live): 1 rallying → 2 handoff ("771 ceiling dissipating and rolling down to form pullback node at
769") → 3 PB formed → 4 pulling back into it → 5 deflected, next leg → 6a stacking / 6b old node dissipated,
new one higher (ceiling rolling up) → 7 target hit ("On watch for a pullback"). Uptrend mirror uses
"building" in #2. Direction word/grade still from the spine; caps trail as a caveat. Pinned character-for-
character by test_read_voice_leg.js.

**The latched ✓/✗ trigger.** Per setup (sym, node, legId), CLOSED bars only: `✓↓`/`✓↑` latches on a rejection
close away from the node (wick into the zone, close back outside, close against the open); `✗` on a close
through it. Once latched it never re-evaluates ("make sure you dont toggle it back and forth"); resets only
on a new leg / node or abandonment (>2× zone away for 3 closed bars unresolved). Persisted in
`gpts_trigger_v1`. Recorded as `defl.trigger` = the deflection hit-rate the loop measures. reactionQuality
is now only the "reaction now" hover input.

**In-play card, user-decided fields.** r1: `● · strike · role · leg tag · ✓↓/✓↑/✗ · grade`. r2:
`S✓ Q✗ V✓ · decision · tgt · inval` + take/pass ONLY when tradeable; on skip just `S/Q/V · skip`; not in
contact: `watching — not in contact · nearest zone X away`. Removed from the face (kept in the question-first
row hover): R:R text, %King, polarity word, tap, ▶ setup, Acm, activity, entry.

**Small UI.** Steps 1-5 line centred. Drift `G768.7↓ · V771.7↑` — each centre says which side of price it
sits on (red ↓ below / green ↑ above; blank when a number is missing). Sync banner grace: "⚠ Out of sync"
needs 2 CONSECUTIVE failed reconciliations (a single 30s tag-vote dropout is silent but still logged).

**Docs.** master-spec §24.1-24.3 (handoff, 15 sentences, latch), LLM brief `leg.handoff` + `defl.trigger`
evaluation, REVIEW-ACCEPTANCE (e). **Tests:** test_handoff (31), test_read_voice_leg (18), test_defl_trigger
(15), test_inplay_card (22), test_sync_grace (15), drift arrows + centred steps in test_drift_read; rules.json
53 ids = rulesSeed(). Suite green except the 4 pre-existing stale.

## v10.55 — 2026-08-18 — trend / magnet / PULLBACK-NODE engine · rolling factor · FUTURES mode (ES/NQ)

`legEngine` (`legStep`, pure, replayable): dir/phase RLY|PB, magnet (strongest node with the trend, capped
at the King), predicted PB zone BEFORE the node exists, pbDetected when a meaningful node appears/grows on
the counter side, roll (2 = signal, 3 = confirmed), rolled-off levels lose target status (air), weakening
flag when a PB forms against the trend. Roll factor +1 confirmed inside the direction hierarchy (never flips
dir). Surfaced in the READ, zones, decision, ⚑ banner; recorded as `leg.pbPredict/pbDetect/roll/magnet`;
seeded in rules.json; LLM brief LEG ENGINE section; master-spec §24. **Futures mode:** chart symbol
detected from the header (ES/MES→SPY, NQ/MNQ→QQQ), live EMA ratio with `≈` fallback, only futures levels
shown, honest "unavailable" for GC/CL. QQQ parity, SPXW confluence (S✓/✗/–), engine-ready matrix.
Multi-session rolling recorded (needs 3 sessions). Tests: test_leg_engine (62, synthetic 08-17 fixture),
test_roll_factor, test_futures_mode, test_qqq_parity, test_spxw_confluence.

## v10.54 — 2026-08-18 — INTEGRITY release (whole-system audit)

Fixed: weights were fiat on ingest (now inert; only PROMO.applied moves them); signed pct blinded −γ nodes
(absolute value at the boundary); outcomes ≠ claims (frame outcome = tgt-before-inval); LLM-only bar numbers
(proposalClearsBar re-derives n / walk-forward locally: n≥20 effN, 3 sessions, no regime flip); green
"saved" on failure; n inflation (effN = n/FEAT_FWD); 1-way vote detector (≥90% flag); triple-count; take/pass
×10; R:R gate (3:1 floor, hidden <2:1); kill list; regime tag; in-play = in contact. Analysis/Testing tabs
redesigned. Tests: test_effn, test_frame_outcome, test_regime_tag, test_promotion_bar, test_zone_row,
test_trade_frame, test_analysis_tabs, test_testing_tab.

## v10.53 — 2026-08-18 — LEARNING LOOP with auto-promotion (user choices)

rules.json v2, FEATURES registry (every feature auto-enrolls: record → outcome → questions → rule),
challenger model, auto-apply + mark on promotion, sparse → keep hand-set, nightly/weekly review methodology
in the gex skill (REVIEW-NIGHTLY / REVIEW-WEEKLY), review read-back into Analysis ⑥/⑦, scheduled tasks
(evening primary 17:47 CT, morning catch-up). Tests: test_rules_v2, test_feature_enrollment,
test_review_selftest.

## v10.52 — 2026-08-18 — end-to-end PIPELINE INDICATOR + automatic review read-back

**Why.** There was no way to see whether a day's data reached the nightly review, or whether a review came
back. Two real breaks: v10.50's footer redesign silently DROPPED the old `saved ✓` text, and the panel
could only receive a review through a console call, so a review sitting on GitHub never appeared.

**Footer is now a 4-stage pipeline** (replaces the 3 health dots; feed/vex liveness folded into the `rec`
hover, nothing lost): `● rec · ● saved · ● pushed · ● review`, each a coloured dot with a question-first
hover. green ok · amber warn · red bad · grey unknown.
- `saved` distinguishes **repo-folder save** from **download-only** (`dl`, amber) — a download never reaches
  the repo, so the review would never see it. That state is now persisted so it survives a reload.
- `pushed` = today's `data/<date>.json` is on GitHub. `review` = last session's review exists AND parsed.

**CT-vs-UTC date bug fixed.** `saveState()` compares against the CHICAGO trading date. Comparing to the UTC
date produced a false "not exported today" on 2026-08-17 (8pm CT = next-day UTC).

**Automatic review read-back.** `pipeCheck()` (max one remote check per 10 min, cached in `gpts_pipe_v1`,
skipped when the tab is hidden, fail-soft) fetches `review/<lastTradingDay>.json` from the raw URL, parses
it into `ANALYSIS_REVIEW`, and re-renders — replacing the manual `__gptsDebug.setReview` call. Analysis ⑥/⑦
and the pre-open brief now fill themselves. 404 falls back one weekday.

**Also:** `gex` skill gains a **REVIEW** procedure (the nightly methodology + honesty rules + a delivery
cascade: device bridge → Google Drive transport → chat), so the scheduled task just invokes the skill and
the method lives in one versioned place. `review/` created with a README. Nightly rescheduled to 06:33 CT.

**Tests:** test_pipeline_indicator.js (105) — saveState classification incl. the CT/UTC case, pipeCheck
caching + fail-soft, stage colours, one-line render. Suite green except the 4 pre-existing stale.

## v10.51.2 — 2026-08-18 — Steps 1-5 restored (small clickable line above the King badge)

v10.50 retired the ①②③④⑤ icons and folded their doctrine "into element hovers" — but the STEP_TEXT
content (all five Skylit method pages) was left in the file with NO CALLER, i.e. unreachable dead code.
The per-element hovers explain individual elements; they do not carry the 5-step framework, which the
project docs call the app's governing workflow.

Restored as a tiny line ABOVE the SUP/King/RES cluster: `Steps 1 2 3 4 5`, 8px, each numeral separately
clickable and opening ITS OWN popover through the existing `.gs-ico` / `data-gstep` delegation in
`wireStepIcons()` (machinery was never removed). Deliberately on its own row so the pill alignment below
is untouched. Hover names the step (1 Magnets · 2 King · 3 Range · 4 Gatekeepers · 5 Flow).

## v10.51.1 — 2026-08-18 — drift bar FIX: correct scale + two readable lanes

**Bug (found live).** The bar drew both ±1σ bands across the **Flr..Ceil** domain — which does not contain
them. On 2026-08-18 (GEX band 769.04-771.68, σ 1.32; VEX centre 774.16; price 772.68) that clamped VEX to
0-100% and GEX to 66%, so the two overlapping 50%-opacity bands smeared into one wash and only the white
price tick was legible. The READ was correct (SPLIT — gamma centred below price, vanna above); the DRAWING
was wrong.

**Fix.** Domain is now the UNION of both bands and price, padded 8% — so the bands always fit and their
relative positions are true. Same live numbers now render GEX 6.9-39.8% (left of price), VEX 48.3-93.1%
(right), price 52.2% — a readable split picture. Bands moved into TWO STACKED LANES (gold GEX on top,
purple VEX below, 3px each) instead of overlaying in one lane, each with a brighter centre tick for its
VWAP, and the white price line spans both lanes. Per-lane hovers name the band and centre.

**Also confirmed live:** VEX capture IS working (VVWAP 774.16 with the v10.49 auth self-fetch) — the earlier
"LASTVEX null" reading was a probe artifact (LASTVEX is not exposed on __gptsDebug), not a capture failure.

Suite green except the 4 pre-existing stale (node_identity, node_role_badge, nodemap, tapeking/jsdom).

## v10.51 — 2026-08-18 — Direction engine: SMA-50 PRIMARY, GEX/VEX drift CONFIRMS or DIVERGES

Replaces the v10.50 weighted-sum lean with a HIERARCHY (user-directed). The 50-SMA five-state machine IS
the trend; drift never chooses direction — it grades confidence.

**Confirmed trend (`up`/`dn`):** direction = the trend, always. Base score 3 (a confirmed trend alone is a B).
Drift agreeing adds +2 (AGREE-*, bands overlap) or +1 (LEAN-*, same side no overlap) → `confirmed`.
Drift opposing subtracts 2 AND hard-caps the grade at **C** → `divergence` (price up while the book leans
down is a caution, not a strong read). Drift SPLIT/NONE → `trend-only`.
**No confirmed trend (`flat`/`up-broken`/`dn-broken`/`na`) — TENTATIVE (user choice):** drift supplies a
PROVISIONAL lean so the panel still reads on rangebound days, but the grade can never exceed **C**. The two
broken states vote 0 for now — we do not yet know whether a broken uptrend continues or reverses; the
recorder will answer that.
All v10.50 hard caps preserved: mid-range → C, chop → C + SIDE, SIDE can't be A, power/open-drive cap odds.

**READ** gains relation-aware wording: confirmed / divergence ("Uptrend, but GEX and VEX lean down —
divergence, lower confidence") / tentative / trend-only. Direction hover rewritten question-first.

**Recording (feeds the future weight optimizer, changes no behaviour):** `dir.trend5` records the FULL
five-state value (uncollapsed) so up-broken/dn-broken earn their own measured hit-rates; `dir.drift`;
`dir.relation` (measures whether the hierarchy beats trend alone). Recorded but NOT voting: `dir.struct`,
`dir.kingRoll`, `netGamma`, `dir.trendFast` (SMA 10 + 20, so windows can be compared empirically).
**FCHIST** (`gpts_flrceilhist_v1`) starts sampling Flr/Ceil strikes per bar so multi-session ROLLING becomes
computable later — rolling is NOT computed or voted yet (Academy: rolling is day-over-day across map
updates; 2 consecutive = signal, 3 = confirmation).

**Analysis — "Direction factors"** table: rows per five-state, per drift verdict, and per relation, each with
n · rate · **vote split ↑/↓** · baseline-adjusted expectation · lift · MFE/MAE, plus a `⚠1-way` flag. The vote
split is mandatory: on 2026-08-11 structure voted DOWN 46/49 on a down day and would otherwise have looked
like 71% edge. Verified against that artifact — the row reads 100% but lift +6 and flags one-way.

**Tests:** test_dir_hierarchy.js (80) — drift never flips a confirmed trend, divergence → C, tentative → C,
caps intact; test_direction_grade rewritten (68); test_feature_enrollment (613). Suite clean except the 2
pre-existing stale (node_identity, node_role_badge) + 2 environmental crashes (nodemap, tapeking/jsdom).

## v10.50.1 — 2026-08-18 — trend confirmation 16/20 → 15/20

`TREND_DOM` 16 → **15** (user). 15 of 20 closed 3-min bars on one side of the continuous SMA-50
(±0.25 ATR band) now confirms a directional trend. This also corrects a long-standing comment error:
the old constant was documented as ">=75%" but 16/20 is 80%; 15/20 is the true 75%. Slightly earlier
trend confirmation, so `up` / `dn` are reached sooner and the `up-broken` / `dn-broken` transition
states trigger on a 15-bar loss of dominance. Comments + test_sma_cont global synced. Suite green
except the 5 pre-existing stale.

## v10.50 — 2026-08-17 — DASHBOARD REDESIGN: one voice per decision, exceptions not defaults

Full element-by-element review of the whole dashboard, implemented. Display-only redesign — all
data/analysis/testing/learning enrollment intact (test_feature_enrollment 429✓).

**READ — single direction voice.** Direction grade merged inline with the verdict (`↑ BULLISH B`, no ⚖).
New 3-beat sentence style: WHERE · STATE+LEAN · POTENTIAL — e.g. "At King 773. Support building with GEX
and VEX leaning up. Potential bounce to 776." Templates for bounce/reject/cont/split (test_read_voice 14✓).
Invalidation off the READ (on the decision line). Regime no longer shown (input to the grade, in its hover
only). Removed: standalone direction line, readWhy block, legacy BULLISH/BEARISH body, "↩ King behind" line.

**DRIFT — one line + thin bar.** ~7px bar under the line: gold GVWAP±σ, purple VVWAP±σ, white price line.
Hover simplified ("Which way do GEX & VEX lean? Both above price — supporting higher prices.").

**DEFLECTION ZONES — the single ladder.** Legacy Node-Map ladder retired; zones are the one node list.
In-play card = 3 rows with the DECISION folded into row 3 (`bounce play · entry 773 · tgt 776(air) · inval
<772`) + TAKE/PASS gated to real setups (grade≥B, cell≠stand-aside). Polarity = colored `g` (yellow +γ /
purple −γ). Reaction = ✓/✗. Confluence = S/Q/V (SPXW added, display-only from the trinity header, honest S–
when absent). Acm horizons renamed 15m/session. ACTIVITY tag (Pull/Push/Defl/BO·FT) folded onto every row.
%King from the gamma feed. Dropped: sparkline, "· px", the gray Dir/Node-grade legend, grade tier ⚖ (→ hover).

**RETIRED:** node-map sentence, legacy ladder rows, duplicate in-map ★SUP/👑/★RES header, step icons ①–⑤
(doctrine folded into element hovers), regime chip, legacy "Deflections" list, snapback line, air-pocket
line (→ `(air)` tag on tgt AND inval), range chip (→ `⚠ OUT · range redefining` exception flag only).

**KEPT as exceptions/health:** session badge (highlight power/OPEX), model-heat (cold-only), pre-open brief
(one line), footer = 3 health dots (feed · vex · rec) + version. Question-first hovers on every element.

**Tests:** +test_read_voice (14), +test_zone_row (24); updated test_read_v1047/layout_2col/accum_canon
(m15/session)/node_grade/feature_enrollment. Suite green except the 5 pre-existing stale.
**Verify live (next open):** the 3-beat READ, drift bar, single graded ladder with folded decision + take/pass,
✓/✗ reaction on a tap, colored g, S/Q/V, footer health dots.

## v10.49.1 — 2026-08-17 — coherence fixes (live-verify caught 3)

**1 · Drift SPLIT bug.** `driftRead` required same-side-of-price AND band-overlap → tight/offset bands
forced a false SPLIT even with both centres above price. Now: SAME side = AGREE on direction (dir set);
band-overlap only decides conf (`UP·conf`) vs plain lean (`UP`). SPLIT reserved for opposite sides.
Regression pinned in test_drift_read (same-side disjoint → LEAN-UP, opposite → SPLIT).

**2 · Two-voice READ.** `readBlock44` computed its OWN BULLISH/BEARISH verdict, contradicting the spine
head (DN vs BULLISH on the same panel). It now takes the verdict word from `directionGrade` (the spine)
when available; legacy lean-based verdict only as fallback (unit-test scope). One direction voice.

**3 · King graded C.** `nodeGrade` ignored magnitude, so the dominant +γ King scored C on tap/roc alone.
Added a dominance input (Academy absolute-value rule): King / %King≥70 → +1, trivial <25 → −1.

Suite green except the 5 pre-existing stale. v10.49 → 10.49.1 (3 spots).

## v10.49 — 2026-08-17 — MENTAL-MODEL DASHBOARD: two-grade READ + decision · deflection-quality zones · full 5-layer enrollment (candidate — verify live)

**A · Auth fix (blocking).** v10.48 self-fetch 401'd: Skylit's `gex/levels` needs an in-memory `Authorization: Bearer <JWT>`.
Now captured off real requests (fetch Headers/object/Request + XHR setRequestHeader) into `LASTAUTH` and replayed by `selfFetch`.
VEX is now ACTUALLY captured continuously. Footer shows `vex ⏳` until first auth is seen.

**B · FEATURES registry (the enrollment mechanism, user rule 2026-08-17).** `registerFeature({key,label,record,outcome,fwd,questions,rule})`;
5 consumers: recorder (`snap.feat`, `recorderDay.feat[sym]`, `resolveFeatureOutcomes` idempotent/forward-only w/ MFE/MAE),
Analysis scorecards, question seeds, `RULES` (`learning/rules.json`, `gpts_rules_v1`, fail-soft), export. 11 features enrolled:
dir, drift, node, decision, acm, defl_ant, reaction, act, rshuf, roll, gateHour. `test_feature_enrollment.js` (429) enforces it.

**C · Spine.** `directionGrade` (drift·structure·range·regime; MID-RANGE and CHOP hard-cap to C) · `nodeGrade`
(polarity·tap·rocNow·rocDay·confluence) · `decisionCell` = 3×3 DECISION_MATRIX (descriptive words only). Cached per bar (`spineOf`).
Grades render ⚖ until RULES promote them 📊 (n≥20).

**D · Drift line** under the header: `↗ Drift UP·conf · G773.9 V775.0` (GVWAP/VVWAP ±σ, normalized per feed; AGREE when same
side of px AND bands overlap). Pinned to live 773 numbers in `test_drift_read.js`.

**E · Descriptive trade frame** per in-play zone: `zone 773±.25 · inval <772 · tgt 776 (air)` (vocabulary locked; tgt capped at King;
path from air-pocket/cluster). Echoed in the decision line. Nothing prescriptive.

**F · Deflection-quality zones** replace the ladder body: in-play node full (identity · +γ clean/−γ sharp · tap · GRADE; row2 =
Acm day/now · Q/V · frame), top-3 others one line each with grade; %King from the feed; `reactionQuality` chip (⚡conf/⚡weak) at the tap;
`▶ setup` anticipation when approaching a ≥B node; legend. Ladder retained as fallback when no node qualifies.

**G · READ** = head `↑ UP B · Node 773 A− ⚖` + why + DECISION line; `sessionBucket` badge (open-drive/morning/midday/afternoon/power,
OPEX) feeds dir; `modelHeat` badge (model warm/cold from last 10 resolved grades); odds ONLY from promoted rules; CHOP ⇒ SIDE/C.

**H · TAKE/PASS** buttons on the in-play zone → `recorderDay.act[sym]` (selection-quality stat in Analysis). No P&L.
**I · accumCanon** = ONE Acm source (now ≈6m, day = since open via `gpts_acmday_v1`) used by sentence, zones and nodeGrade —
Acm/Dec contradiction eliminated. **J · Pre-open brief** line (before 08:30 CT / `__gptsDebug.brief()`).
**K · Analysis** prepends FEATURES scorecards (rate·n·MFE/MAE; dir/node BY GRADE with A>B>C monotone check; 3×3 decision cells;
act selection quality); Testing seeds questions + 9 miner factors; `learning/rules.json` 38 ⚖ rules incl. KILL LIST
(kill.tap3 / kill.midrange / kill.noConf / kill.negGammaWide); `docs/LLM-NIGHTLY-BRIEF.md` (nightly, proposals only).

**Tests.** 8 new files, 696 assertions, all PASS. Suite green except the 5 pre-existing stale. `test_read_v1047` pin → 10.49.
**Verify live:** VEX captured while on GEX (`__gptsDebug.LASTVEX.SPY`); drift line; two-grade READ + decision; zones with grades;
TAKE/PASS writes; footer `feed v10.49`.

## v10.48 — 2026-08-17 — GEX/VEX dual-capture + mode-independent King & ladder

**Problem.** Skylit only sends the `data_type` for what you DISPLAY. `onFeed` routed `combined`
into the gamma cache (`LASTFEED`), and `tapeMap()` reads the DOM tape — which shows the displayed
book. So VEX display gave a false out-of-sync, and GEX+VEX display gave a unanimous-but-WRONG King
(the 780 from A.4). You could never hold clean GEX and clean VEX at once.

**Fix — capture is now decoupled from display.**
- `onFeed(sym,feed,j,viaSelf)`: `vanna`→`LASTVEX`, `gamma`→`LASTFEED`, **`combined` is ignored for
  the caches** (no more contamination). `LASTDISP[sym]` records what you're displaying (hook only).
- `selfFetch(sym,type)` + `ensureFeeds()` (5s interval, tab-visible + URL guard): self-fetches
  whichever mode the display ISN'T showing, off the last real `gex/levels` URL (same auth/query),
  throttled per (sym,type), 503s swallowed. Net ≤1 extra request/cycle. `LASTFEED` (gamma) and
  `LASTVEX` (vanna) now stay fresh continuously regardless of the toggle → VEX is always captured
  for analysis.
- `feedStructMap(sym)` builds a `kingResolve`-shaped map straight from `extractWalls(LASTFEED.j)`
  (King node 100%, others signed by polarity). `tapeMap()` returns it whenever the display isn't
  pure GEX (and as the unreadable-tape fallback), so the WHOLE panel — King, ladder %s, accumulation,
  Flr/Ceil — reads pure gamma in GEX, VEX, or GEX+VEX. The 3-vote reconciler then agrees cleanly
  (no false out-of-sync).
- Footer states it plainly when relevant: `SPY:gamma·feed (disp VEX)`.
- `FEED_STALE_MS` 60s → 12s (matches the 5s keep-alive; tightens the footer live/stale line).

**Tests.** `test_mode_king.js` (29): feedStructMap build, onFeed routing, King-from-gamma-feed under
combined AND vanna display (DOM used only in pure GEX), unreadable-tape fallback, ensureFeeds
stale-mode selection + guards. Full suite green except the 5 pre-existing stale (layout_2col,
node_identity, node_role_badge, nodemap, tapeking/jsdom). `test_read_v1047.js` version pin → 10.48.

**Verify live:** flip GEX → VEX → GEX+VEX; King stays 776 (or whatever GEX says) in all three, footer
shows `gamma·feed (disp …)` off-GEX, no out-of-sync banner. `__gptsDebug.LASTVEX.SPY` populated while
displaying GEX.

## v10.47 — 2026-08-16 — PHASE A SHIPPED (dashboard) · Phases B–D still planned

**Phase A.4 (LIVE FIX 2026-08-17 08:55 CT):** tape sync tripped all morning — Skylit now renders the strike ladder as a real `<table>`; `findTapeTable().validKingRow()` collapsed each `<tr>` to one token so the strike→$K adjacency never matched. Now validates per row (strike in td[0], King $K in a later td). Verified in-page: King 775, 50 rows, 776=97%. Also: feed was `combined` (GEX+VEX toggle) with King 780 — user must run the heatmap in **GEX** mode for the model; VEX is captured separately.

**Phase A.3 (approved mockup `mockups/gex-v10.47-dashboard-mockup.html`):** ladder STATE is plain colored text ("Acm ▲12%", no pill); LIFE is "● T2" (stage letter + taps, no circle). Mockup file added.

**Phase A.2 (same evening):** READ and Node Map sentence cut to BARE BONES per user — READ e.g. "BEARISH. Down to King 772. Gate 774.50 held 2×. Sup 771 steady, Res 773 steady. King steady, −γ. 50% at this distance 📊. Watch 773." Node Map e.g. "CONT thru Gate 774.5 → King 775.38: Dec ▼8%, King 775.38 Acm ▲12% pulling. Sup 773.25 Acm ▲19%." / "REV at Ceil 776.5: Acm ▲14%, held 1×, 777.75 Acm ▲9% behind. Sup 773.25 steady." / "DEFL ↑ off Gate 774.5: 2nd tap, Acm ▲9%." Verdict words abbreviated CONT / REV / DEFL / TBD. 3rd+ tap always flips to CONT with the warning (unless the node is already Dec).

**Phase A.1 (2026-08-15 evening, after first live look):** banner text → "⚠ Out of sync" (one short line); King pill falls back to the model King (no "Waiting on tape…" while the ladder has a King); ①②③ now tiny icons INSIDE the ★SUP / 👑 / ★RES pills, 5-STEP row removed; READ has no "READ ▸" label, compressed wording, 4-line clamp (full text + provenance in hover); Node Map sentence only renders when a node is engaged (no "NO NODE IN PLAY"), 3-line clamp, ⑤ icon rides on the column header; ladder = CSS grid 96/66/78/1fr, one line per node, nowrap, smaller pills; Flr/Ceil labels outrank Rug labels; price divider tighter; Deflections "none".

**Phase A built (candidate — verify live Monday):**
- `kingHeaderBlock()` restored above READ: ★SUP | 👑 King (+ castle-gate row) | ★RES + 5-STEP ①②③ (top slice of the unrendered `kingBlock()`; no charts, no verdict pill).
- Tape-sync gate → `syncBannerHtml()`: ONE red line "⚠ STRUCTURAL READ OUT OF SYNC WITH TAPE" (detail in hover), app renders normally underneath (was a blocking panel).
- `readBlock44()` rewritten: ONE plain paragraph. Verdict word (BULLISH/BEARISH/SIDEWAYS/TBD) from King side+strength, Flr/Ceil state, srBattle; destination + distance; gate in between + tap record (3rd-tap warning); Support/Resistance state; King "getting heavier — dealers pulling price up" / "bleeding"; ONE odds sentence (King pull by distance + hour, dropped in CHOP); ONE watch level; range-position wording (near the floor / midpoint / ceiling); "already reached once today"; purple-node notes. Regime gate: CHOP ⇒ SIDEWAYS.
- Node Map header = `nodeMapSentence()`: CONTINUATION / REVERSAL / DEFLECTED / TBD / NO NODE IN PLAY at the engaged node (fresh Defl > BOw > Push > Pull≤1.5) with the WHY in accumulating/decreasing vocabulary (locked templates), 3rd-tap flip, polarity note. Range/Trend chips + imbalance line + crossover banner removed from the header (range/pattern in hover).
- Flr/Ceil REDEFINED (Skylit): `pickEdge()` = LARGEST node per side (≥`FLRCEIL_EDGE_PCT`=40% ⚖, King excluded unless only strong node; far-edge guard `FLRCEIL_FAR`=6). Strong nodes beyond the edge = "★ Mag · next" (`isNext`).
- Gate icon everywhere = castle-gate svg (`gateSvgSm` in row pills; 🚪 removed). Pull/Push chips without toward-share % (in hover). Defl cards: "Defl · Gate/Ceil/Flr/King…", context chips = Acm/Dec %, ±γ, Nth tap (red at 3rd+).
- Tests: `test_read_v1047.js` (32) + 3 tests updated; suite green except 5 pre-existing stale tests (layout_2col, node_identity, node_role_badge, nodemap, tapeking/jsdom) that were already failing on v10.46.
- NOT in Phase A (Phase B+): reshuffle detector (RShuf chip/sentence), rolling, chart levels, MFE/MAE, gate-hour, GEX/VEX VWAP, Analysis rebuild, Testing pipeline, rules.json, nightly review.

**Design session (2026-08-16) — locked scope for the whole learning pipeline:**

Full ONE-AT-A-TIME design pass against Skylit's 5-step guide + doctrine docs. LOCKED scope, phased:
**A Dashboard**: header cluster restored (★SUP | 👑/gate | ★RES + ①②③), sync gate → one-line banner
(app renders), READ = one D-style paragraph (BULLISH/BEARISH/SIDEWAYS/TBD; locked texts), range
position as wording, regime gate (chop→SIDEWAYS, odds dropped), Flr/Ceil = largest bounding node
(Skylit) + Gate/next-target/cluster classes, Node Map header = one CONT/REVERSAL/RSHUF sentence
(accumulating/decreasing vocabulary), 3rd-tap warning, polarity in why, King-already-reached, Defl
cards with Step-5 context chips (abbrev.). **B Recorder**: reshuffle detector, rolling Flr/Ceil,
gate-defl hour, chart levels (VWAP, PDH/L/C, PM H/L, IB30, POC/VAH/VAL), MFE/MAE, regime on outcomes,
GEX/VEX VWAP computed internally. **C Analysis**: "did the dashboard tell the truth" — 6 sections in
dashboard order + Setup Performance bars; nightly LLM review OPTION 1 (no cloud token; review/ file
committed by local task). **D Testing**: question queue w/ lifecycle + auto conditional refinement +
walk-forward promotion; learning engine `learning/rules.json` read by the panel; confidence tiers;
event tag. Deferred: cross-index SPY/SPXW/QQQ(+VIX). Spec: session-state/latest-resume-note.md +
mockups/gex-v10.46-dashboard-mockup.html. Rules added: one-item-at-a-time, panel abbreviations.

## v10.46 — 2026-08-15 — derived GEX factors + Recommended-tests section (research-backed)

**Research (2 web passes, sourced):** Barbon–Buraschi gamma fragility (the one hard result: +gamma→
mean-revert/low-vol, −gamma→momentum/wide range), Ni–Pearson–Poteshman expiration pinning,
SpotGamma/MenthorQ level defs, Skylit VEX ("GEX governs range, VEX governs drift"). VEX/vanna
directional claims are mechanistic but UNBACKTESTED — flagged 📕 so the tab measures rather than trusts.

**Derived GEX factors (`deriveFactors`, recorded per bar as `snap.deriv`)** — computed from the
strike tape we already parse (pct + polarity): net-GEX sign & magnitude, zero-gamma level + regime
(pos/neg-gamma), abs gamma strength Σ|mass|, gamma concentration (HHI), above/below imbalance,
call wall (largest +γ above) / put wall (largest mass below), GEX ranks 1–6. Unit-tested. Feeds the
Testing miner as new factors; gamma-only, so it works today with no new tape reads.

**Testing tab ⑥ Recommended tests** (`RECO_TESTS` + `recoTestsHtml`): 22 curated hypotheses grouped
by theme (GEX regime, level, concentration, imbalance, accumulation, time-of-day, VEX, DTE gate,
confluence, expiration, end-of-day), each tagged 📗 evidenced / 📙 plausible / 📕 folklore, with the
data it needs and ✅ runnable-now vs ⏳ unlocks-at-open (VEX / multi-symbol / VIX-term). Sources noted.

**Tests:** `test_reco_deriv.js` (18) + suite updated — 7 files, 192 assertions, all green.
**Deferred to v10.47 (needs live/toggled tape, scheduled for Mon open):** VEX (vanna) capture from the
Skylit VEX toggle, multi-symbol full ladders (QQQ/SPXW/VIX), VIX term structure, regime GATE,
Analysis Insights block.

## v10.45 — 2026-08-15 — 🧪 Testing tab: hypothesis engine over the data repository

**Frame.** A third tab (**🧪 Testing**, `TESTING_VIEW`, `window.__gptsDebug.showTesting`) beside
Dashboard and Analysis. It reads the v10.44 IndexedDB repository (`gpts_repo_v1`) and does one thing:
measure the PAST. Everything carries an n; nothing predicts. Five blocks.

**① Question library.** The recurring hypotheses — King pulls 30m; King pull at 2 strikes; King pull
11am CT; non-King mass repels; contender ≥60% repels; Acm wall reached / leak rate; net-force —
each a rate bar with %, n, and a ⚖/📊 tag. Values come from the study store (`studyLoad`, run by
`studyRun`); 08-15 baseline until the local repo has run once.

**② Hypothesis builder.** Preset recipe chips ("King PULL zone · 11am → toward King", "Regime CHOP →
up?") each compose a WHEN→outcome query over the repo and show rate vs baseline + lift. Console API
`window.__gptsHypo({when:[{f:'kzone',v:'pull'}],outcome:'toward'})` for ad-hoc runs.

**③ Pattern miner (`studyMine`).** Auto-scans single + pairwise factor buckets (kzone, kside, hour,
regime, nearest-node state, nearest-node strong/weak) against the outcome, min n=30, ranked by lift,
with a combos-tested count and a multiple-testing caution — rows are leads ⚖ until they survive the
nightly re-run. Cached in `localStorage gpts_mine_v1`.

**④ Insights & recommendations.** A rule engine over the study emitting four buckets: what the data
says / change the product / improve the testing / hypotheses to test next.

**⑤ Data coverage strip (`repoCoverage`).** Days · bars · symbols · which fields exist and since when.

**Nightly.** The LLM review (day export → GitHub) consumes the same store, so page and review agree.

**Still pending (10.45/10.46):** the Analysis-tab **Insights** block and the **regime gate** (suppress
trend/confluence/King-verdict direction claims in chop) — both carried over, not yet built.

## v10.44.1 — 2026-08-15 — hotfix: tape finder (heatmap sidebar was stealing the match)

**Bug (user screenshot, 23:43 CT):** STRUCTURAL READ SUPPRESSED, "tape $K tag —", recurring.
`findTapeTable()` matched the `chart-heatmap-sidebar` container — it also carries "Strike" + a
$K TOTAL (e.g. `-$262,131K`) + 50+ rows — instead of the real tape column, so no King $K ROW was
found and the sync gate (correctly) refused to show a wrong anchor.
**Fix:** (a) reject `heatmap` containers by class; (b) `validKingRow()` requires the $K cell to sit
in a strike row (a strike token immediately precedes it in cell order) — a lone $K total no longer
qualifies; (c) accept the compact trinity per-symbol column fingerprint (`SPY$…King…`, no "Strike"
header) with an SPY preference. Safe for market hours (the classic ladder still matches via the
Strike+expiry branch; validKingRow passes on "775 $1,252,620K"). NOTE: after-hours Skylit renders a
collapsed column that may still not fully parse — the definitive check is at market open.

# CHANGELOG — GEX-Signal-Tapereader

## v10.44 — 2026-08-15 — MAGNET FRAME: single-column dashboard, Node Map rebuilt, data repository

**Doctrine (from the design session):** nodes are magnets — they PULL and PUSH; every indicator is
DESCRIPTIVE (field now) or PREDICTIVE (⚖ hand-set until n≥20 → 📊 measured, nightly-scored).

**UI (user-directed simplification).** King console + King path chart + projected chart REMOVED
from the Dashboard (all still computed/recorded; `kingBlock()` no longer rendered). Panel is ONE
column: **READ ▸ → ⚡ Deflections → ⑤ Node Map**; default width 690→440 (one-time migration).

**READ ▸ (`readBlock44`)** cites only measured/tagged magnet claims: King distance + ZONE
(ORBIT ≤1 / PULL 1.5–3 / OUT >3) with the toward-King rate at that distance and this hour;
Range Flr–Ceil inside/OUT with each boundary's state, episode and "walls like this held X%";
non-King mass lean (repels 📊 57%); regime tag (chop/mixed/trend, efficiency ratio). No legacy
King-verdict/confluence/Break-through claims (all ran contrarian on the 4 test days).

**Node Map (all 5 fields per the locked spec).** IDENTITY: 👑 King · 🚪 Gate · **▔ Ceil / ⛰ Flr**
= nearest strong magnet (≥`FLRCEIL_MIN_PCT`=15% of King) above/below = the live range (Step 3);
★ Mag / Mag for the rest; Sup/Res retired; roles stack (King · Flr); **−γ identity purple**
(Skylit convention) incl. a −γ King. STATE: **Acm / Dec / Steady** (Diss→Dec) + the node's ▲/▼%
vs its session open (persisted), ±15% bright; ±γ text tag dropped (color carries it). ACTIVITY:
**Pull tw% → BOw → BO·FT → Defl ↑/↓ → Push tw% → echoes** (priority fresh Defl > BO·FT > BOw >
Push > Pull > echo); only BOw/BO·FT chips ever shown (chain retired); Push off a node below price
= green. Header **Range chip**. Every chip carries the episode timeline in its hover.

**Episode engine (`nodeEpisode`, pure).** Per node: zone, toward-share (% of last 10 closes
moving nearer), last tag, crossings, state. Recorded per bar as `snap.ep`.

**FT redefined (both directions):** full-hold OR two consecutive directional closes beyond the
node with the 2nd progressing (`s.ftLenient`). Applies to the machine and the scorecard.

**Data repository.** IndexedDB `gpts_repo_v1` (unbounded; migrates the localStorage recorder once;
mirrors the last 12 snaps per bar so outcome back-fills land). New per-bar fields: `xm`
(cross-market headers SPY/QQQ/SPXW/VIX from the Skylit sidebar — confluence was untestable with
0 QQQ bars), `ep` (episodes), `rg` (regime tag). **Daily export** `data/YYYY-MM-DD.json` at 15:01
CT into the repo folder via File System Access API (📁 one-time pick) with download fallback;
`install.bat` registers a scheduled task (weekdays 15:30, run-if-missed) that commits+pushes
`data/`. Footer: `rec ● · saved hh:mm ✓ · 💾 · 📁`.

**Study module (`studyRun`)** = the 08-15 test battery as a nightly job over the repository
(King pull by distance/hour, others-repel, contender-repel, wall-reached by state, net-force,
episode Pull/Push scoring); cached for READ/hovers; 08-15 baseline (4d/391 bars) until first run.

**Fixes.** `hitKing` labeler (fired 1–2%): uses tape King strike, rejects magnitude-as-King,
counts range crossings. `parseKingDollarsK` explicit `Math.abs` (live signed $K seen);
`parseKingDollarSign` exposed as candidate polarity source. %KCH baseline persisted per
day+symbol (`gpts_kd_open_v1`) — survives reload.

**Tests:** `test_magnet_v1044.js` (36) + suite updated for BOw vocabulary — 5 files all green.
**Docs:** `docs/MAGNET-FIELD-GUIDE.html` (new). NEXT (10.45): 🧪 Testing tab, Analysis Insights,
regime gate.

## v10.44-PLAN — design record 2026-08-15 (superseded by the shipped entry above)

Design session outcome; full spec in `session-state/latest-resume-note.md`, build mockup
`design/nodemap_v1044_mockup.html`.

**UI (simplification, user-directed):** King console + King path chart + projected chart
REMOVED from Dashboard (recording continues silently, footer "rec ●"). Single column:
Deflections → Node Map. Node Map becomes the primary magnet surface:
- IDENTITY: 👑 King · 🚪 Gate · **▔ Ceil / ⛰ Flr** = nearest strong magnet (≥15–20% King
  mass) above/below price = the live range (Step 3). Roles stack. Others: ★ Mag / Mag.
  **Sup/Res vocabulary retired.** −γ nodes (incl. a −γ King) render PURPLE (Skylit convention).
- STATE: **Acm / Dec / Steady** (Diss→Dec; Acm kept — Step 5 doctrine) + per-node ▲/▼% vs
  session open (`Dec ▼29%`), threshold-colored. ±γ text DROPPED — purple carries polarity.
  STRIKE·% and LIFE unchanged.
- ACTIVITY: `Pull tw%` → `BOw` → `BO·FT` (only BO chip; chain display removed) →
  `Defl ↑/↓` → `Push tw%` → echoes (broke/held/FBO). Priority: Defl > BO·FT > BOw > Push >
  Pull > echo. **FT redefined (both directions): full-hold OR two consecutive progressing
  closes beyond.** Range chip + range-redefinition echo on Ceil/Flr break+FT.

**Data layer:** %KCH = King-$ %change vs TRUE open (persisted baseline, survives reload);
parseKingDollarsK → explicit Math.abs (LIVE-VERIFIED signed $K exists: −$27,399K; sign =
candidate polarity source). Episode engine per node (PULL → PIN/BREAKING/BLOCKED → Defl →
PUSH·after-tag/-break/-block), per-bar `snap.ep` with conditions-at-contact, nightly
`episodeScorecard()` incl. PREDICT-PUSH forecast arm, %KCh day-direction study
(checkpoints × King-position × polarity), LLM nightly review must answer why/what-preceded/
what-to-change. Everything ⚖ until n≥20 → 📊.

**Test battery (in-page, 4 days/391 bars):** net-force sum RETIRED (50%); King pulls 55% (69% at 2
strikes; 74% 11am CT, 66% 2pm), non-King mass repels 57%, contender ≥60% repels 58%, Acm walls
reached 15% vs Fading 23%; legacy King-verdict/conf/Break-through ran contrarian → regime gate
(10.45). hitKing labeler bug found (1-2% fires) → fix. ADDED to 10.44: DIST→ZONE (ORBIT/PULL/OUT)
+ hour gate; READ ▸ citing measured rates; IndexedDB data repository + daily JSON export + QQQ/VIX
recording; test battery as nightly module. v10.45: 🧪 Testing tab (question library, hypothesis
builder, pattern miner, insights/recs, coverage) + Analysis insights + regime gate.

**Shelved with return-spec:** ATTRACTION tile v2, %KCH tile flip, King charts (return validated
once scorecards mature). Net-force chip retired on evidence.

## v10.40 — 2026-08-14 — KING PATH v2 (Batch 2): analyzer + narrative-first layout + gutter

**LAYOUT (approved mockup).** The "KING PATH · today · drift · rolls · verdict" header row
is REMOVED. The narrative — old bottom verdict line, expanded into the King Analyzer read —
now leads the section. Drift + rolls moved INSIDE the chart (top-left overlay chip), the
verdict pill to the top-right overlay, session times to the bottom-left overlay. Net: one
full row saved, chart effectively taller.

**GUTTER (the label-collision fix).** `kingSparkline` reserves a 46px right gutter
(padR 4→46, W 236→262) with a divider. The price PILL and the 👑King label render in the
gutter at their own Y (anti-collision separation kept), with tick marks into the plot. The
dashed price line ends at the plot edge — it can never run under its label again. The
signed offset-vs-King stays (small line under the pill; test_kingpath enforced it).

**KING ANALYZER (`kingAnalyzer` + `kingReadHtml`).** Descriptive line: King strike,
polarity (+γ friction / −γ fuel dealer mechanics), K$ magnitude + session change, distance,
eVA value band + inside/outside. Predictive line, priority-ordered and TAGGED
(⚖ Academy / 📊 measured-with-n): Overshoot→Beach-Ball watch ⚖ · SUCCESSION WATCH
(contender ≥60% → 📊 76% King rolls to it within 20 bars, n=148) · gravity gate (≤3
strikes; beyond → pull explicitly "unsupported" 📊) · approach/ETA (📊 63% vs 47%) with
POWER-phase PIN WINDOW ⚖ · outside-value imbalance = don't-fade 📊 (n=25) · K$
bleed/build ⚖. Chips: phase+mins-to-close, taps·crossings, succession, K$Δ, QQQ King
alignment ✓/✗ (feed-derived). Provenance footer names the 4d/324-bar base.

**DRIFT DEMOTED.** 3-bar King drift tested 50.0% vs next-30m direction (n=68) — coin flip.
It survives as a DESCRIPTIVE overlay chip only, and says so in its tooltip.

New pure helpers (all unit-tested): `evaBandFromPct`, `successionFromPct`,
`kingTapsCross`, `sessPhaseCT`, `kingApproach`. `KD_TRACK` follows session King-$
(first/last/peak). Analyzer is null-safe: missing inputs suppress claims, never invent.

**Tests:** `test_king_analyzer.js` (26 assertions: helpers + layout/demotion guards).
test_kingpath caught the dropped offset label — restored. Full suite 25/26 green.
NEXT (Batch 3): Analysis-tab King stats recomputing the backtest tables nightly.

## v10.39 — 2026-08-14 — KING DATA LAYER (Batch 1) + indicator backtest results

**BACKTEST FIRST.** Before choosing which King indicators to build, all candidates were
tested in-page against the 4 recorded days (324 usable bars, out10 = next-30m outcome).
Small n — this RANKS candidates, it does not validate them. Results:

| Candidate | Result | Verdict |
|---|---|---|
| Contender >=60% of King | King rolled TO THAT STRIKE within 20 bars **112/148 (76%)**, median 4 bars | BUILD — headline ("King Succession Watch") |
| Convergence velocity | approaching -> **63%** continue toward King (n=161) vs receding 47% (n=148) | BUILD |
| Distance gravity | toward-King edge at <=3 strikes (54/59/60%), FLIPS beyond (47%, 0/3 at 5+) | BUILD as a <=3-strike gravity gate |
| eVA (70% exposure band) | inside -> 57% rotation (n=260); **outside -> continuation, NOT reversion** (revert 36%, n=25) | BUILD — outside-value = imbalance, do-not-fade read |
| King drift (3-bar) | next-30m direction agreement **50.0%** (n=68) — coin flip | DEMOTE to descriptive-only chart chip |
| Naked Kings / IB×King / one-way rolls | supportive anecdotes (n<=4 days) | TRACK, grade after data accumulates |
| K$ momentum / polarity | UNTESTABLE — never recorded | THIS BATCH fixes that |

**SHIPPED (data layer only, no UI change):**
- `recNode()` now records **`pos`** (gamma polarity) and **`abs`** (magnitude) per node —
  the long-committed KEYSTONE. Unblocks Academy Art.4 (net-gamma regime), Art.7
  (day-over-day rolling), Art.9 (real-vs-hedge), plus contender/K$ backtesting.
- New `parseKingDollarsK()`: the King row's dollar figure (e.g. `$996,886K`) is now
  PARSED, exposed as `tapeMap().kingKd`, and recorded per bar as snapshot **`kd`**.
  Live 2026-08-14: K$ bled $1,397,016K -> $996,886K (−29%) intraday while price stalled
  below — the strongest leading signal on the board, previously discarded.
- Captured in BOTH tape paths (tr/td and div-grid), null-safe, guarded.

Taps/crossings/dwell, IB, HOD/LOD, eVA and naked-King ledgers are DERIVABLE offline from
the already-recorded px/tking/nodes series — deliberately NOT duplicated into capture.

**Tests:** new `test_king_data.js` (14 assertions incl. sync-guards on both capture paths).
Full suite 24/25 green (test_tapeking needs jsdom). Batches ahead: B2 = King Path v2 UI +
analyzer (approved mockup, drift demoted, VIX-confirm chip pending ladder spike);
B3 = Analysis-tab King stats recomputing these tables as n grows.

## v10.38b — 2026-08-14 — L0B TAPE RECONCILIATION: consensus gate, fail-closed display

Follow-on to the v10.38 King fix. The parse defect was fixable; the ARCHITECTURE that
let it ship silently was not addressed by fixing it. One parser was the sole authority
on the King, so a single defect inverted the structural anchor with nothing to contradict it.

**THREE INDEPENDENT PATHS.** The King is now derived three ways that fail differently:
1. `kingFromTapeTag()` — Skylit's own `$K` marker in the rendered DOM
2. `kingFromFeed()` — largest `|v|` in the raw network payload
3. `kingFromTapeMax()` — largest `|%King|` in the parsed tape map

A parse bug breaks (3) but not (1) or (2). A stale feed breaks (2) but not (1) or (3).
A Skylit DOM change breaks (1) and (3) but not (2). **No single fault can take a majority.**

**CONSENSUS REQUIRED.** `reconcileVotes()` (pure, 47 assertions) needs >=2 agreeing paths.
Outcomes: unanimous / majority / no-consensus / single-source / no-source. A single source
is explicitly NOT consensus — it cannot corroborate itself.

**FAIL-CLOSED DISPLAY.** `render()` calls `tapeSync('SPY')` before any %King-derived block.
Without consensus it renders `outOfSyncBlock()` — naming the reason and showing all three
votes side by side — INSTEAD of the King badge and Node Map. Structural data is now
suppressed rather than shown wrong. Aligns with LEARNING-SPEC S0: "NEVER fabricate a
number the data can't support." A confident panel built on a wrong anchor is exactly that.

**PARSE INVARIANTS FEED THE GATE.** A `kingConflict` from `kingResolve()` forces the gate
closed even when the three votes agree — a flagged parse is never treated as healthy.

**RECURRENCE TRACKING.** `RECON_STATE` counts consecutive failures per symbol. At
`RECON_FAIL_ESCAL` (3) the fault is marked RECURRING and the panel says so. A bounded log
(20 records) retains reason + all three votes + timestamp per failure so a repeating fault
is diagnosable rather than guessed at. The streak resets on any healthy read.

**OPERATOR DIAGNOSTICS.** `__gptsDebug.syncReport()` returns verdict, all three paths,
agree/disagree lists, streak, recurring flag and the full failure log.
`__gptsDebug.setTapeGate(false)` reverts to legacy behaviour if the gate ever misfires;
`CFG.tapeGate` defaults true.

**PROOF.** `test_tape_sync.js` replays the real 2026-08-14 board — tag 780, feed 780,
broken parser 775 — and asserts the reconciler returns **780 with the parser still
broken**, flags the dissenter by name, and never crowns 775. This layer would have
caught the incident on the first render.

Full suite 23/24 green (test_tapeking skipped — needs jsdom).


## v10.38 — 2026-08-14 — CRITICAL: tape/tapereader King desync fixed + negative-gamma strikes recovered

**Found live during market hours (SPY, 09:30 CT).** The panel was reporting `King 775 · 42%`
while the Skylit tape tagged **780** with `$1,252,620K` and the feed independently showed 780
at 3.71e9 vs 775 at 8.49e8 — 780 dominant by 4.4x. The structural anchor was inverted, and
it was being RECORDED that way.

**ROOT CAUSE (two defects in the same function).**

1. *Cross-expiry read.* The King row prints the DOLLAR figure instead of `100%` (King ==
   largest absolute exposure == 100% by definition). `firstStrengthPct()` returned null on
   that cell, and the loop then fell back to `cells[2]` — a DIFFERENT EXPIRATION COLUMN —
   assigning the King the next expiry's 3-4%. `kingResolve()` saw 775 at 45% beating the
   "4%" King and fired `maxpct-override`, crowning the wrong strike. Every downstream read
   inherited it: node roles, %King normalisation, target ladder, gatekeeper geometry,
   regime, READ narrative.
2. *Signed %King discarded.* %King carries gamma POLARITY. `firstStrengthPct()` accepted
   only UNSIGNED values, treating every signed one as a change chip — so EVERY
   NEGATIVE-GAMMA STRIKE was silently dropped from the tape map (774 `-1%`, 777, others).

**FIX.**
- New `tapeCellPct()` replaces `firstStrengthPct()` in the tape reader. Takes the FIRST
  percentage in a cell as %King (sign preserved); any later percentage is the growth chip
  and is ignored. A cell containing `$K` returns 100 unconditionally.
- Path A never reads `cells[2]`. One column, one expiry, no crossing.
- `kingResolve()`: **the `$K` tag is authoritative and can no longer be demoted.** The
  `maxpct-override` branch is removed. If a parsed percentage disagrees with Skylit's own
  King tag, the PARSE is wrong, not the tag. `maxpct` remains only for the no-tag case.
- Two hard invariants, both flagged rather than silently absorbed:
  `king-not-100` (tagged King didn't parse to 100) and `rival-at-or-above-king`
  (some other strike met or exceeded 100). Invariant 2 scans every strike, not just the
  max — a tie leaves the max pointing at the King itself and would slip through.

**TESTS.** New `test_tape_king.js` — 37 assertions, fixtures captured verbatim from the
live Skylit DOM. Includes SYNC-GUARDS that fail if `cells[2]` fallback or `maxpct-override`
is ever reintroduced. Verified end-to-end against the real 2026-08-14 board:
King 780 @ 100%, 775 @ 45% (not King), 777 present, 774 retained at -1.
Full suite 22/22 green (test_tapeking skipped — needs jsdom).

**NOTE.** Recorded snapshots store both `king` (feed) and `tking` (tape), so days captured
before this fix are recoverable — the raw values were never lost, only the resolution.

**DOC GAP.** `design/architecture-design.md` Layer 0A described the tape bridge without ever
specifying the cell layout (two percentages: %King then growth; King row prints dollars).
That omission is why the bug survived. Should be documented there.


## v10.37 — 2026-08-14 — King badge carries gatekeeper · Deflections one-line strip · Gatekeeper section removed

**King badge redesign (kingBlock):** the single gold pill is now two stacked rows.
- TOP: 👑 crown + King strike + signed offset vs price ('+2↑' / '−3↓', green above / red below).
- BELOW: white gate icon + GATEKEEPER strike + its signed distance from price (was the SPY price).
  No gatekeeper => a dimmed gate + '–' placeholder (keeps row height; means clear path to King).

**Gatekeeper section REMOVED:** the standalone gatekeeperBlock() render call and the inline
'🚪 Gatekeeper …' Node-Map header line are both gone — that info now lives in the King badge.
(gatekeeper() detector + gatekeeperBlock() function are left defined but unused, for safety.)

**Deflections strip redesign (deflectionBlock):**
- Header collapsed to ONE line: '⚡ Deflections  N live' (saves vertical space for the Node Map).
- Removed the 'unlock n≥…' message entirely.
- Cards now sit in a HORIZONTAL scroll strip, NEWEST-LEFT (sorted by fewest bars-since-tap),
  strip starts scrolled fully left.
- Per-card data: node type (setup name) + strike + direction + confluence chips.
  Grade stays HIDDEN ('● rec nX') until the setup crosses its auto-tuned unlock sample size.

**Tests:** full suite 22/22 green. Mockup: mockups/king_badge_and_deflections_v10.37.html.

## v10.36 — 2026-08-14 — Deflection Signals section (above Node Map) + honest data-earned grading

**New section: ⚡ Deflection Signals** — rendered ABOVE the Node Map (accumBlock).
A deflection = price taps a node and reverses away (reuses deflectionAt, >=DEFLECT_CONFIRM bars).
Rows are sorted by setup priority and show: strike, direction, setup name, confluence chips, and a grade.

**Setup classification (classifyDeflection):** King(90) > Gate(85) > Rug/Reverse-Rug(80) >
Pika/Barney(60) > Floor/Ceiling(50). The BO·FT-retest flavor (breakout+follow-through, pullback
back to the node, then deflect) STACKS on top (+8 prio, ⭑ marker) — the user's marquee case.
FBO (false-breakout) flavor tagged via nodeOutcome.

**Honest grading (data-earned, not predicted):**
- recordDeflections(sym) — logs each NEW confirmed deflection into recorderDay.defl[sym],
  de-duped per (setupKey@strike) within the forward window. Wired into the snapshot cycle.
- labelDeflectionOutcomes — after DEFL_FWD_BARS (10) closed bars, marks whether price CONTINUED
  in the deflect direction by >= DEFL_CONT_PTS (0.30 strikes).
- deflStats — aggregates per-setup continuation rate across all recorded days.
- Grade is HIDDEN (dashed "● recording  n=x/N") until the setup crosses its unlock sample size.
  Unlock N is AUTO-TUNED from observed daily setup volume (deflUnlockN ~= 3 trading days, floor 5,
  cap 25) — recommended after ~2 weeks of real data rather than guessed up front.
- Grade thresholds: A+ >=75% · A >=68% · B >=58% · C >=45% · D <45%.

**Tests:** test_defl_signals.js added (28 assertions: unlock-N, grade thresholds, classification
priority + BO·FT/FBO stacking, forward continuation scoring, stats aggregation). Full suite 22/22 green.

**Mockup:** mockups/deflection_signals_mockup_v10.36.html (3-panel: day-1 hidden grade,
matured earned grade, Analysis performance bars).


Implements the mocked & approved Node Map redesign (mockups: nodemap_final_redesign,
nodemap_redesign_v2_with_bo_outcome).
• 4 fixed ZONES per row, column headers up top so rows align:
  1. IDENTITY — new nodeRolePill(): ONE pill merging role ICON (★/👑/🚪/🧶) + WORD
     (King/Gatekeeper/Rug/Pika/Barn/Flr/Ceil). Kills the old duplication (icon in strike
     column AND a separate word badge on the right).
  2. STRIKE·% — merged ("775 · 23%"), King strike gold.
  3. STATE — Acm/Diss/Steady + γ polarity grouped.
  4. ACTIVITY·LIFE — ONE event by priority: DEFLECTION (Defl ↑/↓) > live BO chain
     (BO·FT·TST·CONF·GO) > resolved outcome (broke↑/↓ / held / FBO) > attracting; PLUS the
     lifecycle DOT.
• Lifecycle tag → compact DOT (single letter T/U/D; Fresh shows nothing; % + note in hover).
• Tests: nodeRolePill assertions added to test_node_role_badge.js. Suite 21/21 green.

NEXT (user-requested, NOT yet built): a dedicated "DEFLECTION SIGNALS" SECTION — a
standalone panel aggregating all confirmed deflection events across detectors (King
deflection, Gatekeeper deflection, Rug/Reverse-Rug, Floor/Ceiling), each with the node
type, direction, and CONFLUENCE (e.g. BO + pullback + deflection at one strike = stronger).
The per-node deflection detection (v10.34) + this v10.35 activity zone are the inputs;
the section is the digest of "what just became tradeable."

## v10.35 — 2026-08-13 — Node Map row REDESIGN (4 zones)

## v10.34 — 2026-08-13 — DEFLECTION detector + FBO relabel

SOURCE OF TRUTH: Skylit Academy (execution-doctrine: "enter at the direct tap, deflection
plays out"; core-concepts: magnet/deflection).
• NEW deflectionAt(): a DETECTED reversal off a node — price taps within DEFLECT_ZONE
  (0.50 SPY/QQQ; docs ±0.50 / ±5 SPX), reverses away by >=DEFLECT_AWAY(0.45), sustained
  >=DEFLECT_CONFIRM(2) closed bars. Reports {dir:+1 bounce-off-floor / -1 reject-off-ceiling,
  awayPts, bars, pos}.
  - EVENT REPORT, NEVER a prediction — fires only AFTER the reversal confirms. Honors the
    locked honesty red line (the app never predicts deflect/break).
  - Multi-bar confirmation per BUILD-PLAN (a one-bar 'did it reject' test mis-calls break-then-reverse).
  - Polarity FLAVOR (not a gate): +gamma node = deflection expected; -gamma = counter-character (noted in hover).
  - Distinct from BO outcomes: deflection = clean bounce/rejection; 'held' = never broke; FBO = broke then reversed.
• UI: DEFLECTION takes PRECEDENCE in the Node Map Activity zone ("Defl ↑/↓" + hover naming
  the node: King/Gatekeeper/Floor/Ceiling). Rationale (user): a setup is "meaningless until
  there is a deflection" — so the confirmed deflection is the headline event on the row.
• FBO: false-breakout outcome marker relabeled 'false break' -> 'FBO' (user).
• Attached per-node as L.deflection in nodeMapModel.
• Tests: NEW test_deflection.js (11 assertions incl. up/down/-gamma/multi-bar-confirm/
  still-sitting/never-tapped); step5 + nodemap tests updated. Suite 21/21 green.
NOTE: this is the detection+report of deflection only. The Node Map UI ZONE REDESIGN
(4-zone layout, role-pill de-dup, lifecycle dot, BO/outcome consolidation) was MOCKED and
APPROVED but NOT yet built — it is the next UI build (mockups: nodemap_redesign_v2).

## v10.33 — 2026-08-13 — Node Lifecycle: Fresh/Tested/Delivered/Decaying + tap-probability (Academy pattern)

SOURCE OF TRUTH: Skylit Academy "Node Lifecycle" (marked KEY in the doctrine).
• NEW node tap counter (updateTaps/nodeTapCount): counts DISTINCT taps — a tap = wick
  within TAP_TOL(0.20) of the strike, then price LEAVES by >=TAP_AWAY(0.60), then RETURNS.
  One long sit != many taps. Persisted per trading day. (Fixes the previously dead,
  never-populated 'touches' field.)
• NEW nodeLifecycle(): FRESH (0 taps, ~80% 1st-tap) -> TESTED (1, ~66%) -> DELIVERED
  (2+, ~33%, graveyard) -> DECAYING (weakening with no interaction). Academy tap-reaction
  probabilities (TAP_PROB 80/66/33) surfaced as a FACTUAL annotation, not a trade call.
• UI: nodeLifecycleTag on Node Map rows — Fresh/Tested/Used/Decay + probability. Fresh
  shown only on King/Gatekeeper/strong nodes (avoids badge spam on minor untouched nodes).
• Charts-First safe: reports how many times price tested a level, never buy/sell.
• Tests: NEW test_lifecycle.js (16 assertions incl. distinct-tap + no-inflation-on-sit);
  nodemap stub added. Suite 20/20 green.
NOTE: Real-vs-Hedge (cross-session GROWTH vs decay) is the SEPARATE later half of Node
Lifecycle — needs the committed recorder-schema history; not in this build.

## v10.32 — 2026-08-13 — Air Pocket / Liquidity Vacuum detector (Academy pattern)

SOURCE OF TRUTH: Skylit Academy "Air Pockets, Liquidity Vacuums & Velocity Mode."
• NEW airPocketDetect(): a low-exposure GAP between two significant nodes = a fast
  PATHWAY (trade THROUGH it, target the node on the far side), never a target itself.
  - Air Pocket vs Liquidity Vacuum by gap width RELATIVE to the board's grid step
    (adapts SPY 1-strike vs QQQ/SPX wider). Sparse-board fallback to an absolute floor.
  - Flags the pocket ADJACENT to spot (Academy Velocity-checklist Q1) + reports the
    pathway's far-side target nodes. Structural read, NOT a buy/sell signal.
  - Tunables: AIRPOCKET_GAP_MULT=2.5, AIRPOCKET_VACUUM_MULT=4.0, AIRPOCKET_MIN_STRIKES=2.0.
• UI: compact "⚡ Air Pocket lo–hi · pathway → up/dn" note in the Node Map header,
  shown only when a pocket sits adjacent to spot; full detail in the hover.
• This completes 3 of 4 ingredients of the Academy's "dangerous combo" checklist
  (we already have polarity + velocity/rapid flags; air pocket was the missing brick).
• Tests: NEW test_airpocket.js (14 assertions incl. relative-spacing + sparse-board
  edge cases); nodemap stub added. Suite 19/19 green.
• CLEANUP: removed the v10.31-debug feed/candle field probes (audit complete — verified
  the scraped candle prop carries only OHLC; volume/CVD live on separate chart series,
  logged as a roadmap candidate to widen the fiber scraper).

## v10.31 — 2026-08-13 — Detector polarity hardening (Skylit Academy = source of truth)

SOURCE OF TRUTH recorded: Skylit Academy (skylit.ai/learn) is now the authoritative
reference for all detector logic; on conflict with FAQ/patternpedia/older code, the
Academy wins (see SOURCE-OF-TRUTH.md). Full Academy (11 articles) mirrored to skylit-docs/learn/.

• CLUSTER = PIKA CLOUD, positive-gamma only. Academy: "Pika = POSITIVE gamma specifically."
  clusterDetect now filters w.pos===true. Fixes clusters/stacks over-firing on -gamma days
  (root cause of the "too much double-stack on multiple strikes" report).
• NEW Barney detector: dense NEGATIVE-gamma region (Academy: "a different animal — Barney").
  Surfaced with its own 'Barn' node badge (instability/acceleration zone), not mislabeled as a pin/chop cluster.
• DOUBLE-STACK polarity-gated to +gamma (STACK_POS_ONLY=true, tunable) — a bounce shelf is a
  +gamma support behavior. Kept (verified a real Skylit concept via FAQ). Final rule pending live review.
• Badge vocabulary aligned to Academy: Cluster 'Clst' -> 'Pika'; added 'Barn'.
• Tests: test_cluster_stack.js now polarity-aware (+7 assertions incl. Barney), role-badge +2,
  nodemap stub added. Suite 18/18 green.

Note: Double-Stack final polarity rule to be locked after reviewing live before/after counts.

## 2026-08-13 — v10.30: DETECTOR HARDENING (Double-Stack over-fire fix + Rug adjacency/strength) + roadmap
- **DOUBLE-STACK over-fire FIXED (user-reported).** On SPY's dense 1-pt grid the old detector chained any run of consecutive >=25% strikes into one giant "double stack" and badged every strike. Now: a Double-Stack is EXACTLY a comparably-strong adjacent PAIR — (a) exactly 2 nodes (a run of 3+ is a Cluster, not a stack), (b) MUTUALLY EXCLUSIVE with Cluster (cluster members are skipped), (c) STRENGTH-BALANCED (weaker >= STACK_BALANCE=0.5 × stronger, so a dominant King next to a marginal node is not a "stack"). doubleStackDetect now takes the cluster result to enforce exclusivity.
- **Significance floor raised 25% → 40% (CLUSTER_SIG_PCT).** A "fortress" node must be substantial vs the King; 25% let ordinary mid-strength strikes all qualify on a dense board.
- **RUG / REVERSE-RUG hardened.** (a) The yellow ceiling must now sit DIRECTLY over the purple node — within RUG_ADJ=3.0 strikes — matching the doctrine's tight cap-over-accelerant stack (was: any purple anywhere below, which over-fired). (b) Both anchors must be STRONG (>=RUG_ANCHOR_PCT=40), not just clear the 20% cascade floor. (c) Fixed a latent bug: the Reverse-Rug mass test referenced purpleCeil.v which walls never carry (always undefined → silent pct fallback); now compares pct-scale masses correctly.
- **Thresholds (all tunable, sync-guarded by tests):** CLUSTER_SIG_PCT=40, CLUSTER_BAND=3.0, CLUSTER_MIN_N=3, STACK_GAP=1.0, STACK_BALANCE=0.5, RUG_SIG_PCT=20, RUG_ANCHOR_PCT=40, RUG_ADJ=3.0.
- **ROADMAP:** added the NOW-priority "Detector correctness pass (v10.30)" and the follow-on "Analysis-tab metric review (v10.31, AFTER detector hardening)" per user — harden the Dashboard detectors BEFORE scoring their metrics.
- **TESTS:** test_cluster_stack rewritten (20 checks incl. over-fire regression + mutual-exclusivity); test_rug rewritten (13 checks incl. far-purple, weak-anchor, weak-reverse-floor rejections). Full suite 18/18 green.
## 2026-08-13 — v10.29: Node Map decluttered — callout lines → badge hovers, "regime"→"Pattern", final abbreviations
- **REMOVED the Node Map header callout LINES** (they duplicated per-node badges / drifted off-doctrine):
  * **Pattern instruction line** ("Stand aside — no clean edge." / "Fade the edges" / "Enter on pullbacks") — the header Pattern BADGE already names it; stance lives in the badge hover.
  * **S/R Imbalance net-read + crossover banner** — NOT part of Skylit's Step-5 (which is per-node Acm/Diss/Reshuffling, already on the rows); the net bull/bear synthesis edged toward a deflect/break call the doctrine forbids. Removed (srBattle engine retained for other consumers).
  * **RUG callout** and **Double-Stack / Cluster callouts** — fully covered by the per-node RugC/RugF (RRugC/RRugF) / DStk / Clst badges.
- **DETAIL MOVED TO BADGE HOVERS.** The per-node pattern badges now carry the live detail in their tooltip: Rug → geometry (ceil over floor) + forming/candidate/unconfirmed state + targets; Double-Stack / Cluster → span + node-count + meaning. (New row fields rugDetail/stackDetail/clusterDetail feed nodeRoleBadge.)
- **"regime" → "Pattern"** — renamed to match Skylit's Patternpedia vocabulary (Trend / Whipsaw / Rainbow Road are Patterns, not "regime"). Badge unchanged; wording corrected.
- **FINAL ABBREVIATIONS** (full names in hovers): Double-Stack→**DStk**, Cluster→**Clst**, Rug ceiling/floor→**RugC/RugF**, Reverse-Rug→**RRugC/RRugF**, Ceiling→**Ceil**, Floor→**Flr**. (Acm/Diss node-status tags already shipped in v10.26.)
- **TESTS:** test_node_role_badge updated to the final labels (priority order unchanged). Full suite 18/18 green.
## 2026-08-13 — v10.28: step-number icons on Node Map (⑤) & Gatekeeper (④), + space-saving abbreviations
- **⑤ STEP-5 ICON in the Node Map header** — the Node Map header title now leads with the clickable ⑤ icon (opens the "Step 5 — Map the Flow" popover), matching ①②③ in the King header and ④ in the Gatekeeper section. The 5-step spine is now fully numbered end-to-end.
- **④ moved BEFORE the gate icon** in the Gatekeeper section (was gate+④, now ④+gate) so the step number leads the section like the others.
- **ABBREVIATIONS (space fix — full names kept in tooltips):**
  * per-node role/setup badges: Double-Stack → **2Stk**, Cluster → **Clu**, Rug-ceiling → **Rug-Ce**, Rug-floor → **Rug-Fl**, Reverse-Rug variants → **RRug-Ce / RRug-Fl**.
  * Node Map callouts: "⬛⬛ Double-Stack N–N — strong-bounce shelf" → "⬛⬛ 2Stk N–N · bounce shelf"; "▦ Cluster N–N (N nodes) — pin/chop zone" → "▦ Clu N–N · pin/chop" (node-count moved into the tooltip). Frees up the cramped text/badge columns in the ladder.
- **TESTS:** test_node_role_badge updated to the new abbreviated labels (priority ordering unchanged). Full suite 18/18 green.
## 2026-08-13 — v10.27.1: layout fixes on the v10.27 batch (leftover ladder, header fit, King Path legibility)
- **Node Map: REMOVED the leftover old two-sided ladder.** accumBlock() was still rendering the pre-Step-5 ladder ("778 Ceiling · 24% [Steady]", "776 King · 38%" + ↳ sparkline rows) BELOW nodeMapBlock() — a duplicate of the new Step-5 identity ladder. accumBlock is now just the Node Map wrapper (everything after the nodeMapBlock() call deleted).
- **King 3-magnet header: FIT FIX.** ★SUP / 👑King / ★RES badges overflowed the panel width. Shrunk fonts (side strike 14→12.5px, King strike 15→13px, labels 8→7.5px, offset arrow 14→12px), padding, radius, and gap; row now uses justify-content:space-between + width:100% + flex:0 1 auto;min-width:0 so the three badges spread inside the panel instead of spilling off both edges.
- **King Path: LEGIBILITY FIX.** (a) King & price labels stacked when King≈price — now they auto-separate vertically to a guaranteed ≥11px gap (split around their midpoint). (b) The price line was lost under the gold King staircase — restyled to stroke-width 1.4, dash 4 3, opacity 0.85 (stale 0.35), + a blue price marker on the left end, so the two lines read distinctly even a strike apart.
- **TESTS:** test_kingpath updated for the new price-line style (dash 4 3, stale opacity 0.35). Full suite 18/18 green.
## 2026-08-13 — v10.27: BO section removed→per-node BO tag + 14-bar breakout gate + Gatekeeper magnitude-driven + S/R Imbalance folded into Node Map + badge/chip cleanup
- **GATEKEEPER now MAGNITUDE-DRIVEN (definition fix).** Was: NEAREST significant node between price and King. Now: the DOMINANT blocker = largest |%King| on the path (the doc's "second-highest node between price and King" / "compare vs the 2nd highest-value node" / "far in excess of nodes beyond it"). Tiebreak on EXACT-equal magnitude only → the node nearer PRICE (doctrine's price-anchored ≤5-pt validity). Fixes the case where a nearer-but-weaker node wrongly won over the stronger blocker (e.g. picks 779@41% over 778@14%). test_gatekeeper +2 assertions (stronger-farther beats weaker-nearer; exact tie→nearer price).
- **BO / SPY Signals standalone section REMOVED → per-node BO tag.** The breakout-pullback lifecycle now rides on the Node Map row it belongs to as a compact chain tag (BO / BO·FT / BO·FT·TST·CONF·GO), colored by direction (long green / short red), tooltip explains each stage. State machine (runMachine/newSetup/STATE.setups) unchanged — only the grid rendering is gone. Reclaims vertical space. (nodeMapBlock → setupTagForNode)
- **NEW 14-BAR BREAKOUT QUALITY GATE.** A BO only fires if the breakout bar ALSO prints a new N-bar extreme: a 14-bar HIGH for upside breakouts / 14-bar LOW for downside breakdowns (window INCLUSIVE of the breakout bar). Symmetric (longs + shorts). Filters weak/noise pokes through a node not backed by genuine range expansion. Tunable const BO_HL_LOOKBACK=14. +test_bo_14bar (8 assertions incl. constant sync-guard).
- **S/R IMBALANCE standalone section REMOVED → folded into the Node Map header.** The Step-5 net-flow read ("Bearish/Bullish imbalance — resistance X building, support Y fading") + the tradeable crossover banner (▼ BEARS / ▲ BULLS TAKING OVER) now render as Node Map header lines. srBattle engine unchanged (render-cached; same value). Keeps the flow thesis, drops the section overhead.
- **Redundant strongest-Sup/Res chip row REMOVED from the Node Map** — duplicated the King 3-magnet header (★SUP ← 👑 → ★RES). Snapback "↩ King N behind" warning retained.
- **SUP/RES header badge overflow FIX** — the ★SUP/★RES side badges clipped their label + %King outside the pill (rigid height:42px + 3 stacked rows). Switched to min-height:42px + line-height:1 so content sizes cleanly inside the border. Matches the King badge behavior.
- **TESTS:** full suite **18/18 suites green** (17 prior + test_bo_14bar), no regression. symSignalsHdr/signalGrid left defined but unreferenced (dead, harmless).
- **NOTE ON NUMBERING:** this is the Step-5 consolidation release; the VEX/Analysis-tab/LLM bundle originally sketched as "10.26/10.27" is still unbuilt — renumber that to v10.28+ next session.
## 2026-08-13 — v10.26: Step 5 node identity (three-axis) + role/setup badge (pill removed) + Cluster/Double-Stack detectors
- **NODE STATUS (Step 5 flow, doc-vocab):** each node now shows Building/Fading/Steady mapped to **Acm** (green, strengthening) / **Diss** (red, weakening) / **Steady** (grey). The RAPID flag = the doc's **Reshuffling** state → 🔥 (rapid Acm) / ❄ (rapid Diss), tooltip ties to the doc word. (nodeStatusTag)
- **NODE TYPE (gamma polarity):** **+γ** positive-gamma (pinning/mean-revert, yellow) vs **−γ** negative-gamma (accelerant/breakout, purple) — surfaced per-node from the existing `pos` field. (nodeTypeTag)
- **REMOVED the predictive verdict pill (Bounce/Pullback/Break-through)** — it violated the attraction-only honesty rule (map makes NO deflect/break call). The `verdict` field still exists in the model but is NOT rendered.
- **ROLE/SETUP BADGE replaces it (factual, never predicted):** King > Gatekeeper > Rug-ceiling/floor > Double-Stack > Cluster > Floor/Ceiling (priority order; single top badge, secondary role in tooltip). (nodeRoleBadge)
- **TWO NEW DETECTORS (user's node-type list):**
  * **clusterDetect** — ≥3 significant nodes (≥25% King) within CLUSTER_BAND=3 strikes → PIN/CHOP region (blue). Callout ▦ + per-node badge.
  * **doubleStackDetect** — 2+ adjacent significant nodes within STACK_GAP=1 strike → strong-BOUNCE shelf (green). Callout ⬛⬛ + per-node badge.
  * Thresholds: CLUSTER_SIG_PCT=25, CLUSTER_BAND=3.0, CLUSTER_MIN_N=3, STACK_GAP=1.0 (all tunable; test has a sync-guard).
- **King bug fix (from v10.25) intact** — extractWalls returns the STRIKE of max |exposure|, not the magnitude.
- **TESTS:** +test_node_identity (17), +test_node_role_badge (19), +test_cluster_stack (15). Full suite **17/17 suites green**, no regression.
- **NOT YET DONE (next session):** live verification of v10.26 on real tape; Analysis-tab scoring layer (item 3); tape reconciliation pass; daily-file→LLM loop. See BUILD-PLAN v10.26 section + RESUME-NEXT-SESSION.md.
## 2026-08-12 — v10.25: 5-step posture layer (live panel) + CRITICAL King-selection bug fix
- **5-STEP INFO-ICON SYSTEM** (①–⑤): click-to-open popovers carrying the Skylit "how-to-read-heatseeker" step text, wired via delegated handler + re-appended popover on each render.
- **STEP 1 (header)**: 3-MAGNET cluster centered — green ★SUP (left) · 👑King (middle) · red RES (right). Old trend badge REMOVED (regime/instruction line carries the trend read).
- **STEP 2**: King framed as EOD settlement anchor only (EOW/swing wording dropped — this is a day-trading tool).
- **STEP 4 (gatekeeper area)**: own section below King header; white castle-gate (portcullis) SVG icon, no spelled-out "GATEKEEPERS" title; lists gatekeeper node(s) + strength ratio via existing v10.24 gatekeeper() engine.
- **STEP 5 (sharpened Node Map, ATTRACTION-ONLY)**: accumulation ONLY attracts — NO deflect/break prediction. Per-node stage: attracting → "at node · watch BO" handoff. Resolved OUTCOME echo (report, not prediction) from the BO state machine: broke ↑ · broke ↓ · held (clean reject) · false break (broke then reversed).
- **CRITICAL KING BUG FIX**: extractWalls() was returning the max exposure MAGNITUDE (e.g. 6.6e8) as the King instead of the STRIKE — leaked into S.king/PREVKING and shown as a nonsense King disconnected from the tape. FIXED: King = strike of largest ABSOLUTE |exposure| (so a dominant NEG-gamma node can be King). Confirmed against day_811.json: 67 snaps had magnitude-as-king; tking (correct source) was always a real strike.
- **TESTS**: +test_step5_attraction (16), +test_king_strike (9). Full suite 14/14 green, no regression.
## 2026-08-12 — v10.24.1: Rug polarity VERIFIED (after-hours) — Rug/Reverse-Rug flag now LIVE
One-time polarity verification done against the live ladder (after-hours; feed still populated). Confirmed sign(pos=n.d>0) == Skylit ladder color convention (positive=yellow, negative=purple), so the Rug detector's polarity basis is sound.
- EVIDENCE (SPY ladder DOM, signed %King = polarity): 772=−85% (purple/neg), 773=+90% / 779=+95% / 774=+77% (yellow/pos), 769=−17% (neg). Signs cluster exactly where color convention predicts; no inversion.
- CROSS-SYMBOL CONFIRM: QQQ 727=−$13,835K (large negative/purple) with positive nodes above = the documented yellow-over-purple Rug geometry appearing NATIVELY under our sign convention. An inverted sign would render this upside-down; it doesn't.
- ACTION: flipped RUG_POLARITY_VERIFIED=true. Rug/Reverse-Rug now SHOWS its flag (was computed-but-hidden in v10.24). All other detectors unchanged. test_rug updated to assert shown=true. 9/9 new suites + 3 existing green.
NOTE: raw feed JSON not fetchable out-of-tab (Authorization header, not just cookie) — verification done from the rendered ladder, which is the authoritative feed render.

## 2026-08-12 — v10.24: Node Map v1 + intraday detectors (regime / Gatekeeper / Rug) + effectiveness capture
Shipped as one release (user locked). Build order: Node Map structure -> regime -> Gatekeeper -> Rug. 9 new test suites + 3 existing green.
- **NODE MAP v1** (Issue I): reshapes PROJ into a two-sided, price-anchored dealer-positioning map. Consumer of futureStructureSummary rows (not a new engine). Marks strongest floor/ceiling (★, size+build-rate+nearness blend), the King (👑), per-level verdict (Bounce/Pullback/Break-through/Forming, directional-meaning colors), travel-emphasis side (trend badge + momentum, both sides always shown), and against-King "magnet behind" note. Renderer nodeMapBlock() replaces the abbreviated PROJ row. Layout + verdict wording user-approved (rendered mockup).
- **GEX-STRUCTURE REGIME** (gexRegime): whole-board Trend / Whipsaw / Rainbow Road on current node structure, reverse-engineered from Patternpedia + annotated examples. TREND = one-sided mass skew ≥1.8×; WHIPSAW = 2 edges + hollow middle (edge/mid ≥2×); RAINBOW = ≥4 prominent both-sign interleaved, full middle. DISTINCT from the SMA price-trend badge (kept). Surfaces as a header chip + regime-as-instruction line (fade edges / stand aside / enter on pullbacks). Tunable consts REGIME_*.
- **GATEKEEPER** (gatekeeper): nearest high-|value| node between price and King; strength ratio = |gk| / |next non-King node beyond|. Ratio ≥1.8× => Reversal-likely verdict + King decoy discount. Early-session rejections weighted higher. Absolute value ranks (polarity only flavors).
- **RUG / REVERSE-RUG** (rugDetect): the polarity-gated namesake. Yellow(pos)-over-purple(neg) stack, neg floors below, no positive floor => bearish nosedive (mirror = squeeze). Uses per-node pos (feed d). VERIFY-ONCE: RUG_POLARITY_VERIFIED=false gates the SHOWN flag until sign(pos)==ladder color is confirmed live; detector still computes + logs evidence. Confirms with build-rate (downside growing + ~zero upside).
- **EFFECTIVENESS CAPTURE** (Issue I5 mandate): recorder now logs sig.nodemap every 3m bar — regime, gatekeeper, rug, strongest levels, and per-level verdicts — so forward-outcome (out5/out10) hit-rate is computable later by the Analysis tab + LLM loop. Records from day one.
NEW TESTS: test_nodemap, test_gexregime, test_gatekeeper, test_rug.
NOTE: Rug flag stays hidden (shown=false) until the one-time polarity verification; everything else is live.

## 2026-08-12 — v10.23: batch fix (A/B/C/D/F/G/H) — King header rework + de-flicker + continuous SMA
FIXES-FIRST build order per user. All 5 new test suites + 3 existing suites green; no regression.
- **C SMA-50 continuous**: added multi-session close series (convertFiberCandlesCont / S.contCloses) + contSMA/contSMAAtTodayIdx. smaVal + trendVerdict now chart-aligned & populated from the open (kills "need more bars"). Option-A sanity: fall back to today-only + SMA_CONT_FLAG if continuous drifts >5% from spot.
- **D S/R Imbalance** (renamed from "S/R Bias"): reframed to DIVERGENCE of build-RATES near price (imbalanceMetric); badge names the mechanism ("Bearish imbalance — resistance 773 building, support 769 fading"); de-flickered via dead-band + asymmetric hysteresis + 3m bar-close commit (SRB_STATE). Crossover derived from committed state.
- **H node-ladder de-flicker**: accumulationStateFor now judges accumTrend on the CLOSED portion of absSeq (excludes live last point) with a rapid-override (ACC_RAPID_ROC) for genuine fast moves. Node badge COLOR corrected to directional-meaning: res-building=red, res-fading=green, sup-building=green, sup-fading=red, steady=grey (stateColor).
- **A King Path**: price line now ALWAYS renders (was silently dropped when price fell outside the strike-only window — the reported bug); folds price into the axis before padding, clamps+caret if off-range, last-known fallback (dimmed) on null px. Labels: spot-price "771.9 (-1.1)" at right end + King strike "👑773" on the gold dot. Canvas +33% taller (H 84→112).
- **B King badge offset**: format now SIGN left, number, ARROW right → "+1↑" / "−1↓" (offset axis only; drift stays in the King Path).
- **F READ removed**: deleted the readBlock() render call; King header auto-promotes to the top of the panel. readBlock/structuralReadHtml left defined as dead code.
- **G King header**: TREND BADGE (left) + King node (right) matched pair. Trend badge = King-matched pill, stacked (state code top / dominant "↓16/20" count below, state-colored arrow) + slope tick (↗/↘/→) centered right, colored by slope direction. "warming up" (no digits) on na.
NEW TESTS: test_sma_cont, test_sr_imbalance, test_node_flicker, test_kingpath, test_trendbadge.

## 2026-08-12 — v10.22: King table-selection fix (FLOW BUCKET popup decoy) + largest-|value| cross-check + year-header guard

DATA-INTEGRITY FIX (found in a live-market review, 2026-08-12 ~09:5x CT). The panel showed King 775 while the tape's true GEX King was 773 (96% %King, $207,395K, adjacent to spot 772). Root cause: findTapeTable() returned the FIRST DOM container carrying a "Strike" header + a "$...K" cell. The "FLOW BUCKET / Top contracts" flow popup ALSO matches that pattern, sits earlier in the DOM, and its $K dollar tag lands on 775 — so whenever that popup is open the parser latched onto the wrong table and crowned the wrong strike. Every King consumer (King header/path/journey, S/R bias, edges, regime classifier, AND the recorder that persists to daily-data) read from that corrupted map, so the bug silently poisoned exports too. This is the 2nd King-integrity bug of the same class (see v10.20 recorder fix) — the source of truth is the tape.

FIXES:
- findTapeTable(): (a) HARD-REJECT the flow popup (FLOW BUCKET / Top contracts / Pick range end markers); (b) REQUIRE the GEX-ladder fingerprint — ISO expiry-date column headers (20\d\d-\d\d-\d\d) OR a deep strike list (>=15 strike rows); (c) among survivors pick the one with the MOST strike rows (the real ladder can never lose to a small popup). Still keyed off rendered data, not CSS classes.
- kingResolve() cross-check: docs say King = largest ABSOLUTE dealer exposure. If the $K-tagged strike disagrees with the strike carrying the largest parsed |%King| by >=5, trust the DATA (override to max-|value|), set kingConflict + remember kingTagged for diagnostics. A single mis-placed $K cell can no longer silently crown the wrong strike. kingSrc reports 'dollar' | 'maxpct' | 'maxpct-override'.
- King row no longer clobbers its real %King with 100 (773 now reads its true 96, not 100) in both Path A (<tr>) and Path B (div grid); 100 kept only as a last-resort fallback when no % is readable.
- Year-header guard: a bare 4-digit year token (e.g. "2026" from an expiry column) can no longer be mistaken for a strike and pollute the node map.

TESTS: test_tapeking.js NEW — reproduces the exact 2-table scenario (flow popup King$K=775 + GEX ladder King$K=773) and the mis-tagged-$K case; 12/12 PASS (picks ladder, crowns 773, preserves 96%, no year pollution, cross-check override + flags). Regressions still green: test_analytics.js (real 8/11 edges incl. fade-support->down 64%/sw73%), test_render.js (Whipsaw regime + all reads, RENDER-OK).

Files: current/gex-signal-tapereader.user.js (v10.22), releases/2026-08-12_king-table-selection-fix_v10.22.user.js, releases/2026-08-12_pre-v1022_v10.21.user.js.

## 2026-08-12 — v10.21: Doc-derived coherent Analysis tab (regime classifier + King behavior + accumulation/dissipation/combined edges + loader + tooltips)

Rebuilt the Analysis tab around the Skylit Heatseeker methodology (read from docs.skylit.ai: Core Concepts, 5-step How-to-Read, Best Practices, Limitations, Pitfalls, FAQs, and Patternpedia). The tab now tells ONE causal argument top-to-bottom instead of scattered stats.

NEW analytics core (pure fns over a day-export, validated against the real 2026-08-11 capture; A_* namespace):
- A_kingBehavior: King path/core (dwell-weighted, ignores 1-bar outliers), net drift, rolls (up/dn, avg size, lead/lag), offset posture (pull up/down), reach rate + time-to-reach + gap convergence, and the PIN metric (close vs day King within ~1pt zone + early/late timing per the doc's drive-off-vs-pin rule).
- A_accumEdge('accum'/'fade'): does building support-below->up / resistance-above->dn (accumulation), and fading support-below->dn / resistance-above->up (DISSIPATION is directional). Reports dir-hit vs baseline (lift), swing-hit, and MFE/MAE payoff, split support vs resistance, with n.
- A_combinedEdge: trapdoor (res build + sup fade -> dn), liftoff (sup build + res fade -> up), compression (both build -> range), dual-vs-single, net-flow polarity.
- A_regime: composes the above into the docs' day-types — Trend (leading one-way rolls + drift) / Whipsaw (flip-flop King in a tight core) / Rainbow Road (scattered heavy nodes, no edges) / Mixed / Forming — each with plain-language why. 8/11 correctly classifies Whipsaw (769–771 core, 7↑/7↓ rolls).

Analysis-tab render:
- Regime chip headline (color by regime) + why. King-behavior step (anchor) with reach/pin. Accumulation, Dissipation, and Combined edge steps — REGIME-AWARE (range days show a "directional edges are low-signal here" caveat). Every item carries a coherence tooltip naming its role in the King->price->S/R->nodes story (context-not-signals posture per Best Practices).
- LOADER: __gptsDebug.loadDay(json)/loadReview(obj)/clearLoaded() + an in-tab "📂 Load day" button, so a past day's export renders here (all render fns routed through A_day()). Legacy 7-step signal scorecard + LLM review preserved below as "Signal scorecard & review".
- Day-grade badge shrunk (28px->18px, 84px->52px) and demoted below the regime chip.

Roadmap captured in DOCS-DERIVED-SPEC.md. v10.22 (needs API key, reminder set 8/12): VEX/GEX overlap + persist VEX from LASTVEX (already hooked, unused), SPX/SPY/QQQ tri-confluence gate, gatekeeper/velocity/node-class, Rug tri-confluence, SSE feed migration (Skylit moved to /api/stream). v10.23: live Dashboard regime banner + Rug-forming alert + multi-day topping/bottoming.

Files: current/gex-signal-tapereader.user.js (v10.21), releases/2026-08-12_regime-king-edges-loader-tooltips_v10.21.user.js, releases/2026-08-12_pre-v1021_v10.20.user.js. Tests: analytics_v1021.js, test_analytics.js (PASS), test_render.js (RENDER-OK, 10/10).

## 2026-08-12 — v10.20: recorder tapeKingStrike fix + King-arrow offset + BO/S-R reorder + Analysis scroll

(Backfilled changelog entry — this shipped but was never logged.)
- RECORDER FIX (the reason v10.20 exists): recorder + sig now use tapeKingStrike() instead of S.king, which was storing raw dealer EXPOSURE (e.g. 528,568,656) instead of the strike in 67/88 snaps on 8/11. Caught by the first end-of-day data-integrity review. hitKing labeling corrected by the same fix going forward.
- King header: arrow now shows the offset NUMBER (e.g. ↑2) with tabular-nums, not a bare arrow.
- Layout: BO / SPY Signals moved ABOVE the S/R Bias node ladder.
- Analysis tab wrapped in its own vertical scroll container so all steps are reachable in a short panel.

Files: current/gex-signal-tapereader.user.js (v10.20), releases/2026-08-12_kingarrow-bo-order-analysisscroll_v10.20.user.js, releases/2026-08-12_pre-v1020_v10.19.user.js

## 2026-08-12 — v10.19: Remove S/R Bias balance bar + King header redesign

- REMOVED the S/R Bias balance bar (the "◀ support 50% · no edge · resistance 50% ▶" block). Per spec, the srBattle read now lives in the READ synthesis at top; the tradeable crossover banner and the node ladder + PROJ strikes below are kept (the ladder IS the S/R nodes the READ refers to). Only the bar was removed.
- KING HEADER redesign: removed the "👑 King" title text; badge is now CENTERED; crown moved INSIDE the badge next to the King price. Offset shown as an ARROW only (↑ when King above price, ↓ when below), and NOTHING when King == price. Fixed the inverted color: King ABOVE price = green (magnet pulling up), BELOW = red (was showing red when above). Removed the "+N" plus-sign entirely.
- King Path hysteresis coloring confirmed live (from v10.18): line color only flips when the King reclaims the prior pivot by ≥2 strikes, so single-strike wiggles no longer recolor a clear trend (the mixed-segment screenshot was a pre-v10.18 build).

Files: current/gex-signal-tapereader.user.js (v10.19), releases/2026-08-12_srbar-removed-kingheader_v10.19.user.js, releases/2026-08-12_pre-v1019_v10.18.user.js

## 2026-08-12 — v10.18: The agreed Dashboard redesign (READ, 16/20 trend machine, trend-gated breakouts, King hysteresis)

Implemented the decisions we specced but hadn't yet coded:

1. **READ replaces MIXED/NO EDGE + 4 badges.** New readBlock() at the top: a plain-language synthesis of the THREE kept signals (King, Trend, S/R) — no vote, no badges, no MIXED/NO EDGE. Leads with the dominant piece (confirmed trend > broken trend > King lean > S/R), names conflict in plain words ("Signals mixed — …"), left border colored by net lean (green/red/amber). CONTEXT voter dropped (proved wrong on 2026-08-11). Old confluenceStrip() left defined but no longer rendered.
2. **Trend = 16/20 five-state machine.** trendVerdict rebuilt: Uptrend / Uptrend-broken / Downtrend / Downtrend-broken / No-trend / NA. 16 of 20 bars (>=75%) confirms a direction; "broken" is the mandatory middle step (uses TREND_LAST memory) styled amber/caution. TREND_DOM 15→16.
3. **Trend-gated breakouts + 50-MA filter.** trendOkFor upgraded: long-break allowed only in Uptrend OR Downtrend-broken; short-break only in Downtrend OR Uptrend-broken; AND the bar must close beyond the 50-MA. Wired into the BO state machine (runMachine). breakoutConviction() tags high (confirmed) vs early (broken).
4. **King sparkline hysteresis.** Line now colored by a running regime that only flips when the King reclaims the prior pivot by >=2 strikes (BUF=2) — single-strike wiggles no longer recolor a clear trend. Roll dots keep their own local up/down color.

Verified: 5-state machine (up17→up, break→up-broken, 16-below→dn, chop→flat), gating (short-ok/long-blocked in downtrend even above MA), hysteresis (1-wiggle stays colored, real reversal flips), READ has no MIXED/badges. Regressions: king 15, fallback/recorder PASS, confluence intact.

Files: current/gex-signal-tapereader.user.js (v10.18), releases/2026-08-12_read-trend16-gating-hysteresis_v10.18.user.js, releases/2026-08-12_pre-v1018_v10.17.user.js

## 2026-08-12 — v10.17: Analysis tab as trigger + narrow-view responsive fixes

- Removed the standalone 📥 Save Day button from the Dashboard footer. The Analysis tab is now the trigger: an in-tab banner shows "Today's data isn't saved yet · [📥 Save & prep review]" when unsaved, and "✓ Saved gex_DATE.json — drop into daily-data/" once saved this session (SAVED_TODAY flag). No surprise auto-download on tab open — the save is an explicit tap.
- Responsive SVGs: timeline + convergence now size to the actual panel width (_bodyW from elBody.clientWidth), render at width:100% when they fit and only horizontal-scroll when bars exceed the width. Fixes overflow in narrow sidebar.
- Timeline visibility fixes: S/R dominance band now a visible green/red tint (opacity 0.13) instead of near-black; price line rendered distinctly under the King (thicker, lighter); auto-padded Y-range so a flat King and 771.xx price no longer collapse onto one line; compact single-row legend (King/price/sup/res) that fits.
- Reduced Analysis step indent 30px → 14px for more room in narrow view.

Files: current/gex-signal-tapereader.user.js (v10.17), releases/2026-08-12_analysis-tab-trigger-responsive_v10.17.user.js, releases/2026-08-12_pre-v1017_v10.16.user.js

## 2026-08-12 — v10.16: ANALYSIS TAB — in-app end-of-day review dashboard

- NEW tab bar at top of panel: **Dashboard | 📊 Analysis** (toggle via window.__gptsDebug.showAnalysis()).
- NEW analysisStats(sym): computes review metrics LIVE from the recorder's labeled snapshots (sig vector + out5/out10) — no LLM needed for the numbers. Direction hit % (30m), reversal-catch %, King-target %, per-signal accuracy (King/Trend/S/R/Confluence), confluence-outcome matrix by alignment count, and multi-strike node lifecycle (net drift per strike).
- NEW analysisBlock(): the 7-step narrative tab — (1) relationship timeline SVG (King+price+S/R dominance band, horizontal scroll, crossover markers), (2) King↔price convergence SVG, (3) node lifecycle (vertical scroll), (4) confluence-outcome matrix, (5) per-signal scorecard, (6) worked/missed/why, (7) discoveries + ranked recs. All from real data.
- LLM narrative panels (steps 6–7 + lead/lag notes) show an honest "Awaiting review" state until a review is loaded via window.__gptsDebug.setReview(json); numeric panels always render from tape.
- Day Grade derived from direction-hit% until an LLM grade is supplied.
- Scrollbars: whole tab body scrolls; timeline + convergence scroll horizontally; node lifecycle scrolls vertically.

Design source: mockups/analysis_tab_mockup.html. Files: current/gex-signal-tapereader.user.js (v10.16), releases/2026-08-12_analysis-tab_v10.16.user.js, releases/2026-08-12_pre-v1016_v10.15b.user.js

## 2026-08-12 — v10.15b: DATA LAYER — signal-vector capture + forward-outcome auto-labeler + daily export

Foundation for the self-improving review loop (Stage 1 of 3).

- recordNodeSnapshot now also captures a per-bar SIGNAL VECTOR `sig`: trend{state,up,win,ma,slope}, king{cls,word,drift,score,magnet,offK}, srb{dom,cross,supF,resF,supPct,floor/ceil + fade flags}, breadth{net,dir,mag}, conf{dir,word,score,aligned,bull,bear,declared}. Each signal is guarded so one failure never blocks the snapshot.
- NEW labelForwardOutcomes(): once ≥5 / ≥10 newer bars exist, back-fills out5 (15m) and out10 (30m) forward outcomes on each older snapshot — {mfe, mae, net, pxEnd, hitKing, revUp, revDn, n}. Idempotent, only fills null slots. Storing BOTH horizons makes the ~1-bar bounce/pullback lag measurable rather than guessed.
- NEW buildDayExport()/saveDayToFile(): self-describing daily payload (schema gex-day-export/v1 + legend + horizons) exported as gex_YYYY-MM-DD.json.
- NEW footer "📥 Save Day" button → downloads today's labeled JSON. Drop into AI Drive /GEX-Signal-Tapereader/daily-data/ for the scheduled end-of-day review workflow.
- Created AI Drive folder /GEX-Signal-Tapereader/daily-data/ as the review workflow's inbox.

Files: current/gex-signal-tapereader.user.js (v10.15), releases/2026-08-12_datalayer-forwardlabels_v10.15b.user.js

## 2026-08-12 — v10.15: King header stacked King/price badge

- Replaced the 3-badge header cluster (gold King price + distance chip "↑3 above" + net-drift chip "↓4 net") with ONE stacked pill: King strike on top, current SPY price below it (thin divider between), and a signed strikes-apart offset (e.g. +2 / −6) to the right.
- Offset = round(King − price). Color: King ABOVE price = red (overhead resistance), BELOW = green (support beneath), equal = gold/neutral — same convention as the sparkline dots.
- King price is the large emphasis line (14px gold); SPY price is the muted second line (11px); offset sized 13px so +5/−5 reads clearly.
- Net-session-drift is still fully described in the KING PATH sparkline tooltip and the drift pill below, so no signal was lost by dropping the header net chip.

Files: current/gex-signal-tapereader.user.js (v10.15), releases/2026-08-12_king-stacked-badge_v10.15.user.js, releases/2026-08-12_pre-v1015_v10.14.user.js

## 2026-08-11 — v10.14: srBattle S/R force engine + crossover flag (validated on real tape)

Replaced the S/R Bias bar's engine (static netPositioning, which only summed
BUILDING nodes and stayed green all the way down a grind) with srBattle() — a
DISSIPATION-DOMINANT support-vs-resistance FORCE model, designed and validated
against the user's real recorder dump (2026-08-11 PM turns).

Core logic: supportForce/resistanceForce = Σ proximity-weighted node build-rate
per side, with a HEAVY extra penalty (rate*1.5) when the NEAREST level (|k-px|<=1)
is Fading — because the tape showed the real turn is marked by the in-play level
DISSIPATING, not by breadth of far building nodes (support keeps rebuilding one
strike lower in a grind, so breadth stays green). Symmetric: a bear-pullback high
reads resistance-dominant (floor giving way + ceiling building); a bounce low
reads support-dominant (ceiling fading + floor rebuilding).

Validation (baked into test_srbattle.js from the actual dump):
- 1:30-1:42 grind (floor rebuilding lower): stays SUPPORT — correctly NO false
  short into rebuilding support.
- 1:48 breakdown (770 floor net -52): flips RESISTANCE + "BEARS TAKE OVER".
- Circle-5 pullback high (~14:45): RESISTANCE throughout.
- Bounce low C3 (~13:27): SUPPORT (ceiling fading, floor rebuilding +47).

UI: the S/R Bias bar is now the FORCE split (green support% vs red resistance%),
headline = Support/Resistance-dominant, plus a CROSSOVER banner — "▼ BEARS TAKING
OVER (pullback-high short trigger)" on SUP→RES, "▲ BULLS TAKING OVER (bounce
starting)" on RES→SUP. Tooltip shows the raw forces + nearest floor/ceiling state.

Confluence BREADTH contributor now reads srBattle.dom (dissipation-aware) instead
of the crude Building/Fading count; a fresh crossover intensifies its weight.
netPositioning kept ONLY as the separate CONTEXT vote (whole-board static tilt)
and setupGrade. Per-render memo (RENDER_SEQ + SRB_CACHE) so srBattle's crossover
state isn't corrupted by being called twice (bar + confluence) per render.

Known limitation (documented in tooltip): bounce-low confirmation can lag the
exact low by ~1 bar (the floor often still reads Fading at the bottom tick, flips
to Building a bar later) — it's a confirmation tool, not a bottom-picker.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
17/17; confluence 4 scenarios OK; srBattle 4 real-tape turns reproduced.

Files: current/gex-signal-tapereader.user.js (v10.14), test_srbattle.js,
srbattle_findings.md, releases/2026-08-11_srbattle-crossover_v10.14.user.js,
releases/2026-08-11_pre-v1014_v10.13.user.js.

## 2026-08-11 — v10.13: CONFLUENCE engine — one coherent thesis across all reads

Added a top CONFLUENCE strip that integrates the four independent signals into a
single directional thesis, so the panel tells ONE story (King lean → price
trigger → S/R entry → context confirms) instead of four scattered readouts.

confluence(sym) scores four weighted contributors, each voting bear(-1)/bull(+1)/0:
- LEAN (King, w1.2): kingVerdict().dir — the structural anchor.
- TRIGGER (Price, w1.2, +15% if a live setup agrees): trendVerdict state + a
  live BO/short setup — price must confirm the move.
- CONTEXT (Board, w0.8): netPositioning().dir — net board S/R tilt.
- BREADTH (Nodes, w up to 1.2): MULTI-NODE — nodeBreadth() counts ALL nearby
  nodes: resistance building overhead + support dissipating below = bearish;
  support building below + resistance dissipating above = bullish. One node is
  weak; several agreeing across strikes scales the weight (net/3, capped).
A direction is DECLARED only at >=3/4 aligned (weighted sign); otherwise
"MIXED / NO EDGE" — the tool stays quiet when signals conflict.

Top strip shows: ▲/▼/◆ thesis word + "N/4 aligned" + per-contributor chips
(LEAN/TRIGGER/CONTEXT/BREADTH each ✓ agrees / ✗ disagrees / – neutral, a
disagreeing one lights red so you see WHY conviction is <4/4) + an action line
("Short rallies into overhead resistance; magnet target 771" / "Buy dips…" /
"Signals conflict — stand aside"). Full explanation in the hover.

Section order: CONFLUENCE strip → King (LEAN) → S/R Bias (CONTEXT+BREADTH) →
BO (TRIGGER).

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
17/17; new test_confluence.js drives 4 scenarios (full bearish 4/4 = the user's
example with 2 resistance building + support fading; breadth-alone stays mixed;
conflict; full bullish 4/4) — all behave correctly.

Files: current/gex-signal-tapereader.user.js (v10.13), test_confluence.js,
releases/2026-08-11_confluence-engine_v10.13.user.js,
releases/2026-08-11_pre-v1013_v10.12.user.js.

## 2026-08-11 — v10.12: remove Trend section, King on top

Section order is now King → S/R Bias → BO (Trend section removed). The Trend
read is now carried by the King verdict + sparkline (dealer positioning) at the
top; structural levels live in S/R Bias below. structuralReadHtml() is left
defined but no longer rendered (harmless; can be deleted later).

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
17/17. No orphaned state (structuralReadHtml was purely presentational).

Files: current/gex-signal-tapereader.user.js (v10.12),
releases/2026-08-11_remove-trend-king-top_v10.12.user.js,
releases/2026-08-11_pre-v1012_v10.11.user.js.

## 2026-08-11 — v10.11: fix King path painted one color; per-segment coloring

Bug: v10.10 stroked the ENTIRE King staircase in the single CURRENT verdict
color, so a King that was bearish (red) most of the session but currently flat
showed a fully yellow line — hiding the real history.

Fix: each staircase segment is now stroked by ITS OWN local direction — green
where the King stepped UP, red where it stepped DOWN, gray for flat baseline
holds. The trailing hold-to-now keeps the last move's color. The line now
genuinely shows "red for most of the day, flat recently". The under-fill was
changed to a neutral gray tint (direction-agnostic context, no longer implying
a single verdict). Roll dots + gold current-King dot unchanged.

The current-verdict color still drives the Bull/Bear/Neutral PILL and the
magnet-target line (those are point-in-time reads) — only the LINE is now
historical/per-segment. verdictCol kept in kingSparkline signature but ignored
for the stroke.

QA: node --check OK; ends })(); FALLBACK + RECORDER PASS; King render test 17/17
(now asserts red segments present + line NOT single-yellow); segment-color unit
test confirms bearish session => red strokes, uptrend => green, verdict color
ignored for stroke.

Files: current/gex-signal-tapereader.user.js (v10.11),
releases/2026-08-11_king-persegment-color_v10.11.user.js,
releases/2026-08-11_pre-v1011_v10.10.user.js.

## 2026-08-11 — v10.10: King bull/bear verdict + verdict-colored path + magnet read

New kingVerdict() combines three independent King signals into ONE directional
call, shown as a colored pill next to the drift label and echoed on a new
magnet-target read line; the sparkline is now colored by that verdict.

Signals combined (drift-dominant):
- DRIFT (primary): net first→current King migration. Dealers re-centering
  exposure higher = bullish structural pressure; lower = bearish. ±3 cap, 1.6x.
- MAGNET (secondary): King vs price. Price gravitates to the King into expiry,
  so King ABOVE price = upward pull (bullish lean, target above), BELOW =
  downward pull, AT = pinned/no pull. ±1x.
- STABILITY (conviction, not sign): pinned ≥20m + no drift + no rolls =
  range-pinned (magnet strong, fade extremes; conviction ×0.6); ≥2 rolls in 60m
  = trending (momentum, go with drift; ×1.15).
Verdict: Bullish / Bearish / Neutral, plus a regime word (trending /
range-pinned / settling). When drift and magnet disagree it flags "mixed" in the
tooltip (and calls Mixed if the net score is weak).

UI:
- Bull/Bear/Neutral pill next to "drift up/down · N rolls" in the King-path title.
- Sparkline LINE + a faint fill tint now colored by the verdict (green bull /
  red bear / yellow neutral-flat), not just raw slope.
- New magnet-target line under the chart: "◉ Bullish · King magnet pulls price
  up toward 771 (2 strikes above)" or "Price is AT the King — pinned…". This is
  the actionable price-prediction output.
- Full drift+magnet+stability reasoning in the pill / chart tooltips.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
render test 15/15; kingVerdict unit test across 5 scenarios (bull/bear/flat/
mixed/trending) behaves correctly.

Files: current/gex-signal-tapereader.user.js (v10.10),
releases/2026-08-11_king-verdict-color_v10.10.user.js,
releases/2026-08-11_pre-v1010_v10.9.user.js.

## 2026-08-11 — v10.9: trend counter conditional, King moved under Trend + taller

1. Trend bar counter (e.g. "14/20 bars") now shows ONLY for a directional trend
   (Uptrend / Downtrend). On Sideways or Trend N/A there is no bias, so the
   count was noise — it's omitted; the clause just reads "Sideways." / "Trend
   N/A.".
2. Section order changed to Trend → King → S/R → BO (King moved up directly
   under Trend so the two directional reads sit together at the top).
3. King sparkline given more height (H 46 → 84) and switched to
   width:100%/height:auto with xMidYMid-meet so it fills the panel width and
   renders ~2x taller, making the drift staircase far more legible. Dots stay
   round (no distortion).

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
test 11/11; trend-counter logic test confirms Up/Dn show the count, Side/NA hide it.

Files: current/gex-signal-tapereader.user.js (v10.9),
releases/2026-08-11_king-under-trend-taller_v10.9.user.js,
releases/2026-08-11_pre-v109_v10.8.user.js.

## 2026-08-11 — v10.8: King path redesigned as a stepped time sparkline

Replaced the King-path chip row with an inline SVG **staircase sparkline** of
King STRIKE (y) vs real CLOCK TIME (x) across the cash session (8:30–15:00 CT).

Why stepped-on-a-time-axis: the King `moves` array is event-based (one entry per
confirmed roll), so equal-spaced chips misrepresented pace — a 2h hold and a
30-second double-roll looked identical. Plotting against real time makes a long
hold a long flat segment (truth) and a roll burst a tight cluster (instability).
The King is a discrete strike, so a smoothed line would imply values that never
existed; a staircase (hold flat, vertical jump at the roll timestamp, flat to
'now') is the honest shape. The silhouette IS the trend: climbing = bullish
migration, descending = bearish, flat = pinned, sawtooth = chop.

Details:
- Line colored by net drift (green = net higher, red = net lower, gray = flat).
- Roll vertices = small dots (green up / red down); current King = gold dot.
- Faint dashed line = current price, for King-vs-price convergence at a glance.
- y-axis extreme labels (top/bottom strike) + x-axis labels (first-move/open
  time on the left, "now · pinned Nm" on the right).
- Title row shows "drift up/down/flat · N rolls". Full explanation in hover.
- Consecutive same-strike moves collapsed; left edge anchored to session open
  (or first move), right edge to now.
- Single-session only (moves resets daily); a multi-day King trend would need
  the recorder feed.

New helpers: sessionBoundsCT() (8:30/15:00 CT epoch ms today) and kingSparkline().
Removed the now-unused horizontal auto-scroll snippet + #gpts-kingpath scrollbar
CSS.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS; King
test rewritten (SVG present, stepped H/V path, gold dot, dashed price line, red
bearish line, drift-down label) 11/11. Standalone kingSparkline unit test also
passed (net drift, dup-collapse, flat-day).

Files: current/gex-signal-tapereader.user.js (v10.8),
releases/2026-08-11_king-sparkline_v10.8.user.js,
releases/2026-08-11_pre-v108_v10.7.user.js.

## 2026-08-11 — v10.7: drop the LOCAL tile from S/R Bias

Removed the LOCAL scope tile from the S/R Bias block. Its "nearest building
support vs resistance" read (relativeRead) duplicated information already shown
two ways: (a) the two S/R node rows straddling the price divider ARE those
nearest levels with their build states, and (b) the Trend sentence already
states the same near-term call with direction + targets. LOCAL was also the
noisiest scope (single-strike flicker) and read "Neutral" most of the time.

Also dropped the now-redundant BOARD tile: the board tilt is fully carried by
the header badge (Support-heavy N×) + the balance bar (green support% vs red
resistance%). The bar's label row now shows support% · dip-buy/fade bias ·
resistance% inline. Net effect: the block collapses to header badge → balance
bar, reclaiming a full tile row of height, with zero information loss.

relativeRead() is kept — it still feeds the "trap / clean forms" alert.

QA: node --check OK; ends })(); single render(); FALLBACK + RECORDER PASS;
King 10/10. No orphaned identifiers (locVerd/locTip/supTx/resTx removed).

Files: current/gex-signal-tapereader.user.js (v10.7),
releases/2026-08-11_drop-local-tile_v10.7.user.js,
releases/2026-08-11_pre-v107_v10.6.user.js.

## 2026-08-11 — v10.6: header cleanup, target color fix, S/R+Bias merge

1. **Removed the "D·S" setup-grade tag** from the panel header (grade element
   kept but permanently hidden so nothing referencing it breaks).
2. **Fixed T2 color.** Both T1 and T2 now use ONE rule: yellow if the target is
   a STRETCH (too far at the recent pace), else green if ABOVE price (long
   target), else red if BELOW price (short target). Previously T2 used a reach
   scale that painted an in-reach downside target (e.g. 769) green — now a
   downside target is red (or yellow if stretched). Exact distance stays in the
   badge tooltip.
3. **Trend body: abbreviations + trend-first.** The Read/Trend sentence now uses
   "Sup" / "Res" instead of Support/Resistance, and leads with a colored trend
   clause (Uptrend / Downtrend / Sideways (+bar count)) BEFORE the S/R text.
4. **Merged the two headers into one "S/R Bias".** The former separate "⚖ Bias"
   sub-header is gone; the board-tilt headline badge (Support-heavy /
   Resistance-heavy N×) now rides on the single S/R Bias header. The balance bar
   + BOARD/LOCAL tiles render below it unchanged.
5. **King path: subtle → connector between nodes.** Faint (PAL.sub, 55% opacity,
   8px) with zero horizontal padding + a slight negative margin so it sits in
   the existing gap and does NOT widen node spacing.
6. **Removed the per-day King roll-count badge** from the King header (knowing
   it's "42" carries no signal). Current King, distance, and net drift remain;
   the roll count still appears in the King-path hover tooltip.

QA: node --check OK; ends })(); single render(); undefined-id scan clean;
FALLBACK + RECORDER PASS; King test 10/10 (roll-count badge absent, → present).

Files: current/gex-signal-tapereader.user.js (v10.6),
releases/2026-08-11_header-target-color-srbias_v10.6.user.js,
releases/2026-08-11_pre-v106_v10.5.user.js.

## 2026-08-11 — v10.5: UI layout overhaul (Trend · S/R · King · BO)

Application structure is now **Trend → S/R → King → BO** (render order changed to match).

1. **Read → "Trend".** Section header renamed.
2. **T1/T2 are now color-coded, distance text row removed.** T1 keeps GEOMETRY
   color (green = above price / bullish, red = below / bearish). T2 now encodes
   REACH as color: green = in reach, yellow = stretch, red = far. Direction of
   T2 is inferred from T1's color (per user). Exact distance/pace lives in each
   badge's hover tooltip. The old "T1 +x/ystr | T2 ..." distance row is gone.
3. **Projected S/R moved into the S/R (ACM) section and abbreviated.** Now a
   single dashed "PROJ · Sup <k> <pct>% · Res <k> <pct>%" row (was the verbose
   two-line "STRONGEST ACCUMULATION" block + the Trend-section PROJECTED row).
   Same strongestAccumulator() data; just abbreviated and relocated.
4. **King section redesign.**
   - Removed the PINNED / SESSION / LAST HR quick-stat tiles.
   - Removed the "KING READ · Resistance at 770 is holding while 769…" sentence.
   - KING PATH reversed to **oldest → newest** with the current King on the
     RIGHT (gold), and the row auto-scrolls to the right end on every render so
     the latest is always in view.
   - Path chips now show the **price only**; time + roll direction moved to each
     chip's hover tooltip. Frees space for 5–6 chips per row.
5. **ACM → "S/R".**
   - Removed the in-play header badge (the "GK 772 / Accumulating" pill).
   - "Strikes that matter" filter: instead of a fixed 3-above/3-below ladder
     padded with idle Steady nodes, it now shows ONLY nodes that are actively
     Building or Fading, plus the in-play node and the King (structural
     anchors). Capped at 5/side with the new scrollbar for overflow.
6. Section order (above).
7. **Vertical scrollbar on the S/R ladder** (max-height 220px, thin dark
   theme). The SPY price divider is sticky so it stays in view while rows
   scroll.
8. **"SPY Signals" header → "BO"** (breakout-pullback tracker), Clear button
   retained.

QA: node --check OK; ends `})();`; single render(); undefined-identifier scan
clean; FALLBACK + RECORDER regressions PASS; King redesign test rewritten to
assert removed tiles absent, path oldest→current, price-only chips, scroll
container present — all pass.

Files: current/gex-signal-tapereader.user.js (v10.5),
releases/2026-08-11_trend-sr-king-bo-layout_v10.5.user.js,
releases/2026-08-11_pre-uilayout_v10.4.user.js.

## 2026-08-11 — v10.4: merged NET + Mixed into one "Bias" block

### Why
The "NET support-heavy 2.1x" line and the "Mixed" line were the SAME support-vs-
resistance question at two zoom levels (whole-board tilt vs nearest-levels edge),
rendered as two separate bordered rows. Redundant + "Mixed" was unclear. Merged them.

### New ⚖ Bias block (replaces the two rows)
- Section header "⚖ Bias" with a headline badge: "Support-heavy 2.1×" /
  "Resistance-heavy N×" / "Balanced" (colored by board tilt).
- Balance BAR: green (support/below) vs red (resistance/above), widths from
  netPositioning below/above totals (e.g. support 68% / resistance 32%).
- Two scope tiles:
  - BOARD = whole-board tilt -> "dip-buy bias" / "fade-short bias" + the N× ratio.
  - LOCAL ~N str = nearest building support vs resistance -> "Support edge" /
    "Resistance edge" / "Neutral" (relabeled from Long-friendly/Trap/**Mixed**),
    with the raw "sup 771 +0 · res 772 -1 /36m" build-rates.
- Full explanations kept in per-element tooltips.

### Relabels (display only; internal cls unchanged so nothing downstream breaks)
- Local verdict: Long-friendly -> "Support edge"; Trap risk -> "Resistance edge";
  Mixed -> "Neutral" (per user).

### Verification
- node --check OK; undeclared-identifier scan CLEAN; single render(); ends })();.
- Regression: DOM-tape parser PASS; recorder PASS.
- Bias-block logic test (live scenario): headline 2.1×, bar 68/32, BOARD dip-buy,
  LOCAL "Neutral", raw rates rendered, "Bias" label — PASS.
- @version 10.3 -> 10.4; banner + footer v10.4.

### Files
- current/gex-signal-tapereader.user.js  (v10.4)
- releases/2026-08-11_bias-merged_v10.4.user.js
- releases/2026-08-11_pre-biasmerge_v10.3.user.js  (prior, safety)

## 2026-08-11 — v10.3: UI cleanup + King section redesign/enhancement

### Header / Read (space savings)
- Grade badge "D · Short" -> "D·S" (one line; Long shows "X·L").
- Read target badges drop the T1/T2 prefixes -> value+role only (e.g. "771 K",
  "769 Fl"), keeping the trend badge. No more 2-line wrap.

### King section — full redesign + enhancement (was cluttered/confusing)
Header badges now on ONE line: value-only King (was "K 771" -> "771"); distance badge
fixed (the confusing "-> 0 below" now reads "at price" when distance is 0, else
"^ N above" / "v N below" with the arrow meaning SIDE, not roll dir); NEW net-drift
badge ("vN net" colored, = first King -> current); roll count is just the number
(was "23x today" -> "23").

NEW enhancements (all from the persisted KINGDAY roll timestamps):
- KING READ line: one-line structural interpretation, e.g. "Drifted down 775->771
  (bearish migration), now pinned 41m at price. Settlement gravity right here and
  holding — range-pin behavior; a confirmed roll away is what would start a trend leg."
  (descriptive + structural implication; no trade advice). Stable-pin logic takes
  precedence over "unstable" so the sentence never contradicts itself.
- Quick-stat tiles: PINNED (time held at current King) / SESSION (start->now drift) /
  LAST HR (roll velocity).
- Roll path redesigned: MOST-RECENT FIRST (left), older rolls scroll right, single
  non-wrapping row, CONSECUTIVE DUPLICATES COLLAPSED (kills the 775->788->775 clutter),
  current strike gold + "now·<pinned>" pinned, green ^ up / red v down + timestamps.

### QA hardening (after the v10.2 pkTip/pkMark runtime crash)
- Added an acorn-based undeclared-identifier scan to the verification pass; v10.3
  reports SCAN CLEAN (no runtime ReferenceError risks in the new King code).

### Verification
- node --check OK; single render(); ends })();.
- Undeclared-identifier scan: CLEAN.
- Regression: DOM-tape parser PASS; recorder PASS.
- New king-logic test (realistic KINGDAY dataset): net drift, at-price distance,
  pinned 41m, session 775->771, last-hr 2 rolls, dedup path, coherent READ line — PASS.
- @version 10.2 -> 10.3; banner + footer relabeled v10.3.

### Files
- current/gex-signal-tapereader.user.js  (v10.3)
- releases/2026-08-11_king-redesign-ui_v10.3.user.js
- releases/2026-08-11_pre-uicleanup_v10.2.user.js  (prior, safety)

## 2026-08-11 — v10.2: render crash fix (pkTip / pkMark undefined)

### Problem (why v10.1 was STILL idle)
Deployed v10.1 still showed idle. Live console revealed the REAL cause: a runtime
ReferenceError thrown every tick inside rowHtml -> accumBlock -> render:
  "ReferenceError: pkTip is not defined" (and a second, pkMark, on the compact path).
render() threw before it could draw King/ACM/Read, so the panel could never populate
regardless of feed/tape data. These were LATENT bugs in the Skylit-ACM base (variables
used but never declared); node --check can't catch them (runtime-only), which is why
they slipped through prior syntax checks.

### Fix
- Defined `pkTip` (the peak-drawdown tooltip fragment) BEFORE its use in the tip string,
  from the same r.state.fromPeak data that pkTxt uses.
- Defined `pkMark` (inline peak marker: green ★ when a Building node sits at/near its
  session peak, fromPeak<=3%) before its two uses (compact + full row). Defensive:
  empty string when data absent.
- Added a static undefined-identifier scan (acorn) to the QA pass; it now reports CLEAN
  (only false positive: the named IIFE 'walk' inside tapeCells, which is valid).

### Verification
- node --check OK; single render(); ends })();.
- acorn undeclared-identifier scan: CLEAN (no real undefined refs).
- Regression: DOM-tape parser snapshot PASS; recorder PASS; wallsFromTape fallback PASS.
- @version 10.1 -> 10.2; banner + footer relabeled v10.2.

### Note
This crash masked whether the v10.1 feed-hook/fallback works — with render fixed, the
panel should now populate AND the footer will reveal the data path (SPY:combined = XHR
hook caught the feed; SPY:tape = DOM fallback carrying it; SPY:idle = neither, needs
further diagnosis).

### Files
- current/gex-signal-tapereader.user.js  (v10.2)
- releases/2026-08-11_renderfix-pkTip-pkMark_v10.2.user.js
- releases/2026-08-11_pre-renderfix_v10.1.user.js  (prior, safety)

## 2026-08-11 — v10.1: feed-hook + DOM-tape fallback (fixes idle panel)

### Problem
v10.0 deployed correctly (title "Tapereader", "Clear", feed v10.0, 15× today) but the
panel went idle again: Read NA, King "Waiting on tape/K –", ACM empty, footer SPY:idle.
Diagnosis on live app: gex/levels API returns 200 (26 calls seen) AND the SPY heatmap
tape table IS present/readable ($388,977K king at 771) — yet LASTFEED stayed empty.
Root cause: (a) the feed hook only wrapped window.fetch, so Skylit's XHR-delivered (or
early/first) gex/levels responses were never captured; (b) refreshSym HARD-BAILED when
LASTFEED was empty, so even though the tape was readable the panel never populated.

### Fix (both, per user)
1. XHR hook: installFeedObserver now also wraps XMLHttpRequest.prototype open/send
   (captures the URL at open, parses responseText at load), alongside the existing
   fetch hook. Feed is captured regardless of transport / install timing.
2. DOM-tape fallback: new wallsFromTape(sym,px) synthesizes a walls[] structure from
   the visible Skylit tape (tapeMap): %King per node, King=100, pos=support/resistance
   by side vs price (abs/net null — not on the tape). refreshSym now: reads fiber price
   first; uses the network feed when live; ELSE falls back to wallsFromTape and still
   runs the machine. Panel populates off the visible tape even with no feed hook at all.
3. Footer status: shows "SPY:tape" (blue) when running off the DOM fallback, "SPY:<feed>"
   (green) on live feed, "SPY:stale"/"SPY:idle" otherwise — no longer misleading.

### Verification
- node --check OK; single render(); ends })();.
- Regression: DOM-tape parser BOTH live snapshots PASS; recorder PASS.
- New fallback test: wallsFromTape builds 8 walls from a shim tape, King=771 @100,
  below-price nodes = support, above-price = resistance. PASS.
- @version 10.0 -> 10.1; banner + footer relabeled v10.1.

### Files
- current/gex-signal-tapereader.user.js  (v10.1)
- releases/2026-08-11_xhrhook-tapefallback_v10.1.user.js
- releases/2026-08-11_pre-feedhook-fix_v10.0.user.js  (prior, safety)

### Next
- DEPLOY v10.1; expect panel to POPULATE (King/ACM/Read fill; footer "SPY:tape" if the
  network hook still misses, or "SPY:combined" if the XHR hook now catches it).
- If it now reads SPY:combined, the XHR hook was the fix; if SPY:tape, the fallback is
  carrying it — either way the panel is live.

## 2026-08-11 — v10.0: CONSOLIDATED MERGE (all versions unified)

### Why
Two divergent builds existed: (a) canonical current v9.2 — had the WORKING div/grid
DOM-tape reader + the DATA-layer recorder + side-aware badges, but was UI-lighter;
(b) /Skylit ACM Project/ — feature-rich (grade badge, King-journey timeline, N× today,
In-play + NET support-heavy read, Absorb state, Gatekeeper) but had the OLD broken
tr-only DOM reader and no recorder. A mockup showed the union of both plus a couple
of items. v10 UNIFIES everything into one file.

### Merge mechanics (chosen for lowest risk)
- Base = the feature-complete /Skylit ACM Project/ file (already has grade/king-journey/
  In-play/NET/Absorb/Gatekeeper and the VISIBLE King-tracker timeline).
- Transplanted my 3 self-contained v9.2 fixes INTO it:
  1. WORKING DOM-tape reader (div/grid Path B: tapeCells/leadTok/leadSignedPct,
     signed nearest-expiry %King, $K king detection) — replaces ACM's broken tr-only
     reader that would show idle on today's Skylit layout.
  2. DATA-layer recorder (gpts_recorder_v7): per-closed-bar node snapshots + setup
     outcome events for SPY+QQQ, 10-day rolling, quota-guarded; wired into tick() and
     runOutcome(); exports __gptsDebug.dumpRecorder()/dumpRecorderJSON()/clearRecorder().
  3. Side-aware ACM badges: building SUPPORT=green, building RESISTANCE=yellow (gold),
     Fading=red, Steady=blue — in stColor/stateColor, in-play header badge, and the
     STRONGEST callout.

### Mockup items
- Title set to "Tapereader" (compact, one line) per request — NOT "GEX Tape Reader".
  Grade badge (A·Long etc) sits beside it, exactly like the mockup.
- Header heights ~20% smaller (title bar 7->5px/13->12px; section headers 11->9px,
  padding 4->3px) — matches the v9.2 compaction.
- VISIBLE King-tracker timeline: already present in the ACM base (kingBlock renders the
  color-coded, time-stamped roll chain from the persisted KINGDAY journey — green up,
  red down, gray hold, each with fmtClock timestamp) + "N× today" move count. Confirmed
  included; no rebuild needed.

### Config / data safety
- Kept ACM's built-in v7->v8 config migration (loadCfg migrates gpts_cfg_v7 -> _v8 on
  first load, settings carry over). King journey persists under gpts_kingday_v1. My
  recorder adds gpts_recorder_v7. No existing keys wiped.

### Verification
- node --check OK; single render(); ends })();.
- DOM-tape parser regression: BOTH live snapshots ALL PASS in the merged file.
- Recorder regression: PASS (snapshot throttle, node fields, outcome event + context).
- Feature audit: title/Clear/grade/setupGrade/King-timeline/N×today/In-play/
  netPositioning/STRONGEST/absorptionAt/Gatekeeper/side-aware-gold/tapeCells/
  leadSignedPct/recordNodeSnapshot/recordOutcomeEvent/dumpRecorder all present.
- @version -> 10.0; banner + footer relabeled v10.0.

### Files
- current/gex-signal-tapereader.user.js  (v10.0 consolidated)
- releases/2026-08-11_consolidated-merge_v10.0.user.js
- releases/2026-08-11_pre-v10-merge_v9.2.user.js       (prior canonical, safety)
- releases/2026-08-11_skylit-acm-source_v9.1.user.js   (archived merge source)

### Next / open
- DEPLOY v10.0 to Tampermonkey; verify: "Tapereader" title + grade badge, compact
  headers, King-tracker timeline chain populates, green/yellow ACM badges, panel not
  idle (feed v10.0), and __gptsDebug.dumpRecorder() fills after a few bars.
- The old /Skylit ACM Project/ file is now SUPERSEDED — treat /GEX-Signal-Tapereader/
  as the single source of truth going forward.
- Analytics/prediction consumer of the recorder JSON = next project phase.

## 2026-08-11 — v9.2: UI compaction, side-aware ACM badges, DATA-layer recorder

### UI / space savings
- Title "Gex Signal Tapereader" -> "Tapereader" (single line, nowrap).
- Header button "Clear All" -> "Clear".
- Header heights reduced ~20%: top title bar padding 7px->5px, title font 13->12px;
  section headers (Read/King/ACM/SPY Signals) font 11->9px, padding 4px->3px,
  margins 5/3 -> 4/2. Applied to sectionHdr, sectionHdrRight, symSignalsHdr.

### ACM badge color now SIDE-AWARE (support vs resistance)
- Accumulation (Building) badge/strip/arrow color:
  - building SUPPORT (below price) = GREEN (unchanged, PAL.longAccent)
  - building RESISTANCE (above price) = YELLOW (PAL.gold)  [NEW]
  - Dissipating = red, Steady = blue (unchanged)
- stColor()/stateColor() made side-aware; in-play header badge and STRONGEST
  ACCUMULATION callout updated to match (resistance accumulator now yellow, not red).

### DATA layer recorder (feeds LLM analytics / prediction) — NEW, additive
- New storage key gpts_recorder_v7 (the 7 existing gpts_*_v7 keys untouched).
- Two streams per trading day, 10-day rolling retention:
  - snaps[sym]: once-per-closed-3m-bar snapshot of the whole node picture — price,
    King (feed + tape), in-play node, and every tracked node's strike/role/side/
    %King(feed)/tape-%King(signed)/state/net/rapid/roll/short-history. Throttled on
    lastClosedB so exactly one per closed bar. Hard cap 200/sym/day.
  - events[sym]: one row when a setup RESOLVES (T1/T2/FAILED/EXPIRED) via runOutcome,
    with setup facts (strike/dir/attempt/targets/bars/duration/boPct) + node context
    at resolution (in-play + nearest-strike node). _recorded guard logs each once.
    Hard cap 300/sym/day.
- Symbols: SPY + QQQ. All writes quota-guarded (drop oldest day on QuotaExceededError);
  wrapped in try/catch so the recorder can never break render or the state machine.
- Debug exports: __gptsDebug.dumpRecorder() (returns DB + logs summary),
  dumpRecorderJSON() (stringified), clearRecorder() (wipes recorder key only).
- Wired in tick(): recordNodeSnapshot('SPY'/'QQQ') after recordSession();
  recordOutcomeEvent() at the end of runOutcome().

### Verification
- node --check OK; file shape preserved (single render(), single IIFE, ends })();).
- All 7 legacy gpts_*_v7 keys intact; gpts_recorder_v7 added additively.
- Offline recorder test (localStorage+STATE+futureStructureSummary shims): no-op with
  no closed bar; 1 snapshot/closed bar/sym; same-bar throttle (no dupes); new bar ->
  new snapshot; full node fields captured; outcome event captured with context. PASS.
- Regression: both live DOM-tape parser snapshots still ALL PASS (no regression).
- @version 9.1 -> 9.2; footer + load banner relabeled v9.2.

### Files
- current/gex-signal-tapereader.user.js  (v9.2)
- releases/2026-08-11_pre-ui-recorder_v9.1.user.js   (pre-change snapshot)
- releases/2026-08-11_ui-badge-recorder_v9.2.user.js (this release)

### Next / open
- DEPLOY v9.2 to Tampermonkey; visually confirm compact header, one-line title,
  yellow resistance-accumulation badges, green support ones.
- Analytics: build the consumer that reads dumpRecorder() JSON and joins node
  pre-conditions to outcomes for the LLM prediction layer (out of scope this fix).
- Still pending from prior ladder: verify GO->T1/T2/FAILED/EXPIRED on live closed
  bars; no stray third target; same-tick persistence; failed never overrides a hit.

## 2026-08-11 — live verification (post-fix, no code change)

### Verified on live app.skylit.ai/atlas (SPY, 3m)
- Panel now POPULATES after a fresh reload: King "K 771", ACM nodes (775 Ceiling
  40%, 773.5 Cluster, 773 Cluster, King 771 = 100/100), footer "SPY:combined".
- Cross-checked against the live heatmap tape: King $-cell at 771 (matches K 771);
  775 first-% = 40% (matches ACM "Ceiling · 40%"). Consistent.
- DOM reader re-validated offline against TWO live snapshots (king top-of-table and
  king mid-table with heavy negatives): both ALL PASS (king + every checked strike,
  incl. negatives like 774=-18, 773=-82, 772=-7). count 23-24.
- No userscript runtime errors in console (only benign Chrome ext "message channel
  closed" noise, unrelated to GPTS).

### Architecture clarification (important for future debugging)
- runMachine() BAILS if there is no network feed: `LASTFEED[sym]` from the gex/levels
  XHR/fetch hook. King/ACM/walls/%King are built from that NETWORK feed (STATE.walls).
- The DOM tape reader (readTapeFromDOM, the div/grid fix) is used by livePctAt() to
  OVERRIDE %King so ACM matches the visible tape for real strikes, plus node-history
  and King-identity cross-check. It is a supplement, not the primary gate.
- Therefore the original "all idle" had TWO causes: (a) network feed not yet captured
  (primary gate), and (b) DOM reader broken for the div/grid layout (fixed this day).
  The panel recovering coincided with the feed arriving; the fix keeps ACM %s tape-
  accurate and King identity aligned.
- Note: for a real strike the ACM % can differ from the tape's first-column number,
  because walls use ABSOLUTE strength (n.v/king) while the tape first column is a
  SIGNED net-change. For cluster half-strikes (e.g. 773.5) there is no tape key, so
  the wall value is used. Both behaviors are by design.

## 2026-08-11 — DOM tape-reader fix (div/grid layout) — v9.1

### Problem
The Tampermonkey panel loaded correctly (feed v9.1) but every section sat idle:
Read "NA / Flat market", King "Waiting on tape… / K –", ACM "No nodes under
accumulation yet", SPY Signals "No active setups", footer "SPY:idle". Root cause:
Skylit Atlas now renders the heatmap strike table as a CSS-grid of <div> cells,
NOT <tr>/<td> rows. readTapeFromDOM() only ever handled the <tr> path (the promised
div fallback was a never-implemented stub), so table.querySelectorAll('tr') returned
0, no rows parsed, count stayed 0, tapeMap() returned null, and LASTFEED/content
never populated. This is validation task #1 ("live DOM-tape reading matches current
Skylit layout"): it no longer matched.

### Fix (readTapeFromDOM + helpers)
- Kept Path A (<tr>/<td>) unchanged for forward/backward compatibility.
- Added Path B for the current div/grid table:
  - findTapeTable() now also accepts a signed King dollar cell (−$…K / $…K, real or
    unicode minus) and a larger subtree budget (<800 nodes).
  - New tapeCells(): walks the grid and returns each TOP-MOST value cell's leading
    token, deliberately NOT descending into it. Skylit nests each column's change
    delta (e.g. "+3%") as a child INSIDE the value cell whose own text is the real
    value (e.g. "98%"), so the leading token = the true nearest-expiry %King and the
    nested chip is ignored.
  - New leadSignedPct(): reads the nearest-expiry %King WITH sign, because Skylit's
    first-column %King can legitimately be negative (e.g. 774 = −39%). The old
    unsigned firstStrengthPct() would have skipped the negative and mis-read a later
    column; leaf-flattening would have grabbed the nested chip.
  - King row detected by the $…K cell; row locked so a trailing chip can't overwrite
    the King's 100.

### Verification
- node --check: OK. File shape preserved: single render(), single IIFE, ends `})();`,
  all 7 gpts_*_v7 storage keys unchanged, @version 9.1.
- Offline end-to-end test of the REAL extracted functions against a DOM shim modeled
  on the live SPY table (own-text value + nested change-chip child): king=773,
  count=24, and every checked strike correct incl. negatives (774→−39, 772→−4) and
  King (773→100). Edge cases: no-table page returns null gracefully (idle, no crash).
- Live DOM used as ground truth: SPY heatmap table ref showed value cell "98%" with
  nested child "+3%", confirming the own-text-leads structure the parser relies on.

### Files
- current/gex-signal-tapereader.user.js  (overwritten with fix)
- releases/2026-08-11_pre-domreader-fix_v9.1.user.js   (old current, safety snapshot)
- releases/2026-08-11_domreader-div-grid-fix_v9.1.user.js  (fixed snapshot)

### Not changed / still open
- Path A (<tr>) still uses unsigned firstStrengthPct (pre-existing behavior; not
  exercised by live Skylit). Could be made sign-aware later for parity.
- LASTFEED (footer status) is fed by the gex/levels network hook, a SEPARATE path
  from the DOM reader; if the footer still shows idle after deploy while content
  populates, investigate the fetch/XHR hook timing next.
- Deployment to Tampermonkey remains a separate explicit follow-up (paste current/).

## 2026-08-11 — load + verification session (no code change)

### What happened
- New context window ran a full canonical load of all eight `/GEX-Signal-Tapereader/` files.
- Verified `current/gex-signal-tapereader.user.js` is BYTE-INTACT (md5 c6a54bbcce5a2896d863d79453d55286), `node --check` passes, final line is `})();`.
- Baseline confirmed: v9.1 uploaded-sync; `gpts_*_v7` keys intact; Trinity symbols (SPY, QQQ, SPXW, VIX) preserved.
- Confirmed approved patch state: `runOutcome(sym, last)` wired; `assignTargets()` restricted to T1/T2.

### Saved this session
- Refreshed `session-state/latest-resume-note.md` (added last-opened marker, md5 integrity note, explicit "ignore /Skylit ACM Project/" instruction; next-step list unchanged).
- Appended this changelog entry.

### Intentionally NOT changed
- `current/gex-signal-tapereader.user.js` (no edits — load/verify only)
- `master-spec.md`, `teaching-spec.md`, `design/architecture-design.md`, `workflow.md`, `developer-kickoff.md` (no rule/doctrine/architecture/contract change)
- No `releases/` snapshot (no meaningful code change to snapshot)

### Next
- Live-validation checklist per resume note (DOM-tape stability → GO outcome resolution → no third target → same-tick persistence → failed-never-overrides → accumulation regressions), then recorder-schema expansion beginning with time/session-truth capture.

---

## 2026-08-11 — uploaded userscript synced into canonical project state

### Sync result
- Synced the uploaded `gex-signal-tapereader.user.js` into `current/gex-signal-tapereader.user.js`
- Saved release snapshots before and after sync:
  - `releases/2026-08-11_pre-upload-sync_v9.1.user.js`
  - `releases/2026-08-11_uploaded-sync_v9.1.user.js`

### Notable code-state differences from the previous canonical baseline
- Preserved the Step 1 / Step 2 outcome patch (`runOutcome(...)` still wired; two-target rule still enforced)
- Added DOM-tape reading / tape-alignment logic for live `%King` and king identity
- Added short-horizon node-history strips and king-roll memory/tracking
- Added a more explicit dip-tolerant / absolute-value-based accumulation detector
- Included denser UI presentation changes such as combined signal-grid direction and broader drag handling

### Documentation updates
- Refreshed `session-state/latest-resume-note.md` for the uploaded-sync baseline
- Refreshed `design/architecture-design.md` to describe the DOM-tape / node-history / accumulation design direction
- Updated `master-spec.md` with the current intake/design emphasis note

### Next validation focus
- validate the DOM tape reader against the live Skylit layout
- validate live outcome resolution and persistence behavior
- then continue recorder-schema expansion from the new canonical baseline

---

## 2026-08-11 — design-document continuity added to save protocol

### Save-protocol enhancement
- Added a required persistent design document at `design/architecture-design.md`
- Updated the save routine so `save` / `save all` / `save everything` must ensure the design document exists and refresh it when architecture/design understanding changes
- Added the design document to startup/restart loading so a brand-new context window loads rules, code, state, history, and architecture intent together

### New artifact created
- Created `design/architecture-design.md` as the restart-safe app architecture/design explainer

### Purpose
- Make project restarts safer by preventing architecture/design intent from being trapped only in code or scattered notes

---

## 2026-08-10 — full shorthand-command hardening across Genspark surfaces

### Command-contract updates
- Hardened `save`, `update` / `claude update`, and deployment-prep commands in addition to the prior `load gex` hardening
- Applied the rules explicitly across all Genspark environments, including the Chrome extension
- Added blocking completion criteria and proof-of-completion reporting for each shorthand command family
- Added stop conditions so the assistant must say a command is incomplete rather than bluffing success when required reads, writes, or output delivery did not happen

### Required proof after each command family
- load: loaded files, baseline/version, approved patch state, next concrete step
- save: updated files, snapshot status, current canonical version/label, unchanged files, deploy-next prompt
- update: incoming code received, baseline before/after, changed files, snapshot status, unresolved diffs, deploy-next prompt
- deployment-prep: canonical source used, current version/label, inline code vs requested URL

---

## 2026-08-10 — command-contract hardening for Chrome extension

### Workflow / bootstrap rules updated
- Hardened `load gex` and related shorthand commands so they are blocking bootstrap commands, not loose intent phrases
- Extended the rule explicitly to all Genspark environments, including the Chrome extension
- Added mandatory post-load confirmation requirements: loaded files, current baseline/version, approved patch state, and next concrete step
- Added stop condition when any canonical startup file was not actually read
- Added `changelog/CHANGELOG.md` to the required startup read checklist everywhere the load procedure is defined

### Purpose
- Prevent partial project loads that sound correct but do not actually read the full canonical project state before continuing

---

## 2026-08-10 — v9.1 minimal recorder patch (candidate)

### Code changes applied to canonical current source
- Added `runOutcome(sym, last)` above `runMachine(sym)`
- Wired `runOutcome(sym, last)` into `runMachine(sym)` after setup progression and before `syncLog(sym)`
- Enforced the two-target rule in `assignTargets()` by changing `slice(0,3)` to `slice(0,2)`

### Behavioral intent
- Outcome engine is observational-only and closed-bar based
- Intrabar touch resolves T1 / T2
- Failed = close back through strike before any target hit
- Expired = no target hit by end of day (15:00 CT)
- Touch wins over same-bar reclaim
- Once a target is hit, failure no longer applies

### Documentation updates
- Updated `master-spec.md` with the canonical minimum ten-group recorder schema
- Updated `session-state/latest-resume-note.md` to reflect the new current baseline and next validation steps

---

# GEX Signal Tapereader — Changelog

## 2026-08-10

### v9.1 candidate save — header cleanup + live-market plan refresh
- Removed the duplicated inner `Read` and `ACM` labels from the status cards and then removed the left-side inner labels entirely so the chip headers are tighter and the outer section header alone names the module
- Preserved file shape (`render()` count, single IIFE, final `})();`) and all existing `gpts_*_v7` storage keys
- Updated the master spec to reflect the UTC-first timing model, the four-layer current-node / future-structure / state / output architecture emphasis, and the live-market capture mission
- Replaced the session-state note to reflect that UTC canonical timing is now architecturally fixed and should be validated live rather than treated as the main pending bug
- Added `live-market-plan-2026-08-10.md` as the latest Claude-ready live-market collection plan

### `code` alias added
- Added `code` as an accepted deployment-prep alias meaning: give the full deploy-ready userscript inline for copy/paste into Tampermonkey
- Updated `workflow.md` and `developer-kickoff.md` so `code` is treated the same as `give me code`

### Inline deploy-code preference added
- Set the default deployment behavior to paste the full userscript inline in chat for direct copy/paste into Tampermonkey unless the user explicitly requests a URL, link, or hosted file instead
- Updated `workflow.md`, `developer-kickoff.md`, and `master-spec.md` to document the inline-code-first rule for deployment-prep commands such as `give me code`

### External sync/update workflow added
- Added `update` and `claude update` as accepted project commands
- Defined a cross-assistant sync flow for pasting newer Claude-generated userscript code back into the Genspark project
- Clarified that externally supplied code can be treated as the authoritative incoming sync payload for that update operation, after which AI Drive resumes its role as the canonical store
- Updated `workflow.md` and `developer-kickoff.md` so syncs now overwrite `current/gex-signal-tapereader.user.js`, refresh `session-state/latest-resume-note.md`, append a factual changelog entry, and optionally save a release snapshot when the external update is meaningful
- Updated `master-spec.md` to document the cross-assistant sync rule

## 2026-08-09

### Shorthand command workflow added
- Added accepted project-load aliases to the operating docs:
  - `load`, `open`, `retrieve`, `continue`, and `get` forms for the GEX project / Tapereader
  - added the shorter alias `load gex` (and parallel short forms for open/retrieve/continue/get)
- Added accepted save aliases:
  - `save`, `save all`, `save everything`
- Added accepted deployment-prep aliases:
  - `prepare the script`, `prepare script`, `give me script`, `give me code`, `give me the code`, `give me the tampermonkey script`, `give me the javascript`, `prepare deploy copy`
- Defined default behavior for each shorthand command in `developer-kickoff.md` and `workflow.md`
- Clarified that `master-spec.md` and `teaching-spec.md` should also be updated when a session changes persistent rules, verified state, doctrine, or conceptual mapping
- Added the post-save behavior: after a save command, ask whether the user also wants the deploy-ready Tampermonkey script

### Project storage scaffold created
- Created persistent AI Drive project structure under `/GEX-Signal-Tapereader/`
- Added canonical folders:
  - `current/`
  - `session-state/`
  - `changelog/`
  - `releases/`
  - `probes/`

### Canonical project files imported
- Saved current userscript as `current/gex-signal-tapereader.user.js`
- Saved restart-safe handoff as `master-spec.md`
- Saved doctrine/teaching reference as `teaching-spec.md`
- Saved current resume note as `session-state/latest-resume-note.md`

### Verified current development baseline
- Current live code baseline: `v9.0`
- Structural-read layer integrated and previously verified live
- Current next engineering target: `v9.1`
- Immediate next coding task: **time-unit reconciliation**

### Operating model established
- AI Drive is now the persistent source of project continuity
- Tampermonkey remains the deployment/runtime target
- Future sessions should load from:
  1. `master-spec.md`
  2. `teaching-spec.md`
  3. `current/gex-signal-tapereader.user.js`
  4. `session-state/latest-resume-note.md`

---

## Changelog rules
- Append newest entries at the top of the file or as a new dated section
- Record only factual project changes
- Include version target when a code change is made
- Note whether a file is a candidate, verified build, or release snapshot
- Do not use the changelog as a substitute for the session-state note## v10.55 — 2026-08-18 — TREND / MAGNET / PULLBACK-NODE engine · rolling · FUTURES mode · engine-ready data · QQQ parity · SPXW confluence

**The mental model (user-taught).** A trend is an alternation of MAGNETS (the node price rallies TO) and PULLBACK NODES
(the node that forms on the counter-move and price DEFLECTS off — the level to sell from in a downtrend, buy from in
an uptrend). Lower-low (magnet) / lower-high (PB), each governed by a node; PB nodes APPEAR AFTER the move and ROLL
lower after each leg. The 50-SMA confirms the trend; rolling ceilings ARE the successive pullback nodes.

**Leg engine (`legEngine`)** per bar: dir from the SMA five-state · phase RLY/PB · magnet (capped at King) · PB ZONE
prediction while rallying ("expect a pullback node to form above, below 775.5 — sell level") · PB DETECTION the
moment a meaningful node appears/grows in the zone lower than the last · ROLL count (2 = signal, 3 = confirmed) ·
rolled-off levels lose target status, vacated zone tagged air · invalidation on close through the PB ("lower-high
broken") · a PB forming AGAINST the trend resets and flags weakening. Uptrend = mirror. NODEHIST records the nearest
ceiling/floor per bar so it is testable. `test_leg_engine` recovers 776 → 775.5 → 775 (synthetic replay of the
user's 08-17 sequence — no real 08-17 export exists in the repo).

**Surfaced** in the READ (RLY/PB/confirmed sentences in the user's vocabulary), the ⚑ "Pullback node formed" banner,
zone rows (`PB · 2nd lower`, `MAG · target`, dimmed `rolled off`), the decision line at a PB ("sell-side deflection ·
tgt magnet 773 · inval above PB 775.0", still behind contact + R:R gates), and the direction hover. The roll is a
score factor INSIDE the trend-primary hierarchy (+1 confirmed aligned / −1 against) — it never flips direction.
Multi-session rolling from FCHIST votes only at ≥3 sessions ("needs 3, have N" until then).

**FUTURES mode.** Chart symbol auto-detected each render (title/header). ES/MES → SPY tape · NQ/MNQ → QQQ tape ·
anything else → "No options tape for GC — levels unavailable" (never invented). Live EMA ratio from the futures
price ÷ underlying (footer `ES/SPY 10.068 (live)`); fallback last-good → constant with `≈` on EVERY converted level.
ALL displayed levels convert (King, SUP/RES, gate, zones, magnet/PB, drift, entry/tgt/inval, R:R, contact band);
only the futures value is shown. Underlying price + candles reconstructed by 1/r so trend/in-play/drift/R:R keep
working on a futures chart. Recording stays in underlying strikes. ⚙ override auto|SPY|ES|NQ.

**Engine-ready data.** `buildFeatureMatrix` → `matrix` in the export (one row per bar per sym, every feature +
regime + model stamp, four outcome labels + MFE/MAE). New non-voting predictors: timeToClose, barOfDay,
distToKing/Magnet, pbActive, rollCount, sessionRangePos, dayNet, PDC-rel, EVENT_TAG (⚙: FOMC/CPI/OPEX/half-day).
**QQQ parity** (spine/zones/leg/features run for the active underlying). **SPXW confluence** parsed from the trinity
ladder → `S` scored +1 when present, `S–` honestly otherwise.

Tests: leg_engine 57 · futures_mode 62 · roll_factor 54 · feature_matrix 42 · spxw_confluence 26 · qqq_parity 41;
feature_enrollment 772. Suite green except the 4 pre-existing stale.


