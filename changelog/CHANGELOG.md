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


