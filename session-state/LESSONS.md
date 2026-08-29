# LESSONS — what went wrong, why, and the rule it produced

**Operator-mandated 2026-08-30:** *"i need you to either use a document you already have or create a
lessons learned document which you update every time there is a build. You will then also make it a
part of the load gex procedure so future contexts know the lessons."*

---

## ⚠⚠ 0 · HOW THIS FILE STAYS TRUE

**`test_lessons.js` fails the build when this file does not carry the current panel version.** That is
deliberate and it is the only reason to expect it to be current. This project's own record is
unambiguous on the point: `latest-resume-note.md` went SEVEN builds stale, then FOUR more the same
day, while `CHAT-HISTORY.md` stayed current for exactly one reason — a test went red when it wasn't.

> **A rule enforced by a test is followed. A rule enforced by a checklist is followed until it is busy.**

**WHAT TO WRITE, EVERY BUILD.** Not "what I did" — the CHANGELOG has that. Write only:
1. Something that turned out to be **false** that a future context would otherwise repeat.
2. A **failure pattern** recurrence, with the concrete instance (a pattern with no cases is a slogan).
3. A **measurement that superseded** an earlier one, naming the number now withdrawn.

If a build produced none of those, write the version and `no new lesson` — an empty entry is a fact,
a skipped entry is a silence nobody can tell from an oversight.

⚠ **NOTHING IS EVER DELETED FROM THIS FILE.** A withdrawn number stays, named as withdrawn. The
whole failure mode this project keeps hitting is work and knowledge vanishing by omission.

---

## 1 · THE FAILURE PATTERNS — these explain nearly every bug in this project


**1. Mislabeling.** A value shown under a label implying a different source, window or scale. The ladder
said `IF` and rendered Skylit numbers. `week` said "to Fri" holding one expiration. **And v11.49: `EM`
said "how much room does today have" while holding `toFri.em` — a LATER expiry worth roughly double the
day's (69.25 vs 34.65).** On a Friday the two coincide, so the bug hid itself on the day you would check.
**Nothing ever throws.** Ask of every number: which book, which window, which scale — and does the label
say so.

**2. Moving denominators.** %King ranks at one instant; **DOLLARS** compare two moments. The EM percentage
divided a session range by a *live* straddle that decays all day — the yardstick moved, not just the thing
measured. Bit four separate features now.

**3. Does it POINT or does it CONDITION?** Gamma, vanna, VIX term structure and **DEX** all condition —
regime line or gate, never a direction vote. This survived several rounds because each *feels* directional.

**4. Concluding "absent" from a shallow look.** `pick()` scanned only TOP-LEVEL payload fields, every
published metric read null, and that drove a decision to compute our own zero gamma. **It recurred in
v11.49: TERM was called "structurally dead — their payload does not carry it." Their PAGE publishes Term
Slope +1.3, rendered client-side.** The payload is not the page. Walk the tree, then look at what they
actually display.

**⚠⚠ 2026-08-30 — PATTERN 4 HAPPENED THREE TIMES IN ONE HOUR, AND THE OPERATOR CAUGHT ALL THREE.**
He asked which node caused a deflection. In sequence I said: (a) "the panel was not recording that
morning" — the recorder started at 13:15, but `LASTFEED.<SYM>.j.levels` held a **389-point series
covering 08:30–14:58**, the whole session, and I had already read `levels[last].s` out of that same
array minutes earlier; (b) "no king was near the high" — reading `tri.<SYM>.king`, one line, when
`tri.<SYM>.top` in the SAME object is the **ranked node list for SPY, QQQ, SPXW and VIX**, recorded
in every snapshot since the trinity shipped; (c) "Thursday cannot be answered" — the day files carry
those same trinity tops for full sessions.

The measured answer, once the right source was used: **every extreme in every book, 36 of 36, sat
within 0.25% of a node; 32 of 36 within 0.10%.** My king-only answer had been "no node was there."

**THE TELL, AND IT IS CHECKABLE:** each time I asked *"does the source I reached for have this?"*
instead of *"what is the best source for this?"* — and each time the better source was ALREADY OPEN,
in the same object, sometimes in a field I had used moments before. **Before concluding data is
absent, enumerate the keys of what you already hold.** `Object.keys()` on the container would have
ended all three in one call.

⚠ It also invalidated work: `tools/study-kingdeflect.py` measured CROWNS against extremes and
reported "crowns beat chance by 17pp". It should have measured NODES. Superseded by
`tools/study-nodeatextreme.py`; do not quote the crown figures.

**5. Defensive try/catch makes a missing reference INVISIBLE.** A function never defined (`tradeNodes`), a
variable from another scope (`rr`). Header emitted, rows gone, looks exactly like "nothing to show".
→ `swallow(tag,e)` records it, `__gptsDebug.renderErrors()` exposes it, `node tools/smoke.js` FAILS on a
non-empty list. A regex scan for undeclared identifiers was tried and ABANDONED — it flagged keywords and
regex-literal contents, and a noisy check gets switched off.

**6. When an edit script asserts, CHECK IT LANDED.** v11.46 shipped half a feature because a python edit
aborted on a failed assertion and the failure went unnoticed.

**⚠ 2026-08-29/30 — FIVE FAKE ASSERTIONS IN TWO DAYS, EACH A DIFFERENT SHAPE OF PATTERN 8.**
All were caught by mutation, none by review, and each was satisfied by something ADJACENT to the
thing it was meant to protect:
- a grep for `UNVERIFIED` matched a string in the verdict helper after the line that SETS it was deleted;
- a grep for the doctrine line matched **the comment I had just written quoting it**;
- a grep for `not before` matched **the comment explaining that the wording had changed**;
- `/\*rr;/` matched the neighbouring LC line, so dropping the scale conversion from PT stayed green
  (a 28-point excursion would have printed as 2.8);
- `ifLadder(sym)` matched the ASSIGNMENT after the loop consuming it was gated off with `if(false)`.

**THE RULE: an assertion about rendered text strips comments first, and an assertion about a value
binds to the statement that produces it — never to a word that appears near it.** Mutate every new
assertion individually; "the suite is green" has never once caught one of these.

**8. A TEST THAT GREPS THE SOURCE INSTEAD OF RUNNING IT. Three occurrences, and the worst one was
mine.** v11.70's forecast ban pulled quoted strings with a regex and desynchronised on an apostrophe —
"so it will likely continue" passed eleven assertions. **v11.86 shipped fourteen assertions guarding the
SPX chart export and not one could catch a wrong PRICE**: swap `toSpy(P.k)` for `P.disp`, change the tick
from 0.25 to 1.0, all fourteen still green. A grep cannot tell `!==` from `===`, and that operator was the
entire failure mode of the v11.83 conflict comparison.
**THE RULE: if a test can pass on a build that emits the wrong number, it is documentation, not a test.**
`eval(ex('fn'))` with stubs costs 47ms — then MUTATE the source and confirm the assertions actually fire.
⚠ Two sub-traps, both re-confirmed 2026-08-23:
  - **A comment explaining a removal contains the removed thing** — `DEX`/`TERM`/`ATR` left FRAME at
    v11.49 and a test still found them in the comment saying so. **FOURTH occurrence.** Strip comments.
  - **Hardcoded global counts** — `Object.keys(rules).length===61` broke at 68 and never checked its own
    claim. Assert the thing the message names.
⚠ **A stale-but-green suite is the dangerous state, not a red one.** Seven suites here were green over
code that had moved under them; twelve more assertions were failing for renames while the code was right.
A permanently-red baseline trains everyone to ignore red — **23 stale failures once camouflaged two live
bugs for months.**

**9. A GENERATED ARTEFACT THAT IS HAND-EDITED WILL DRIFT, AND ITS CORRECT PAYLOAD WILL HIDE IT.**
`install.bat` announced **v11.49** while carrying v11.86, committed as **v11.79**, and called a v1.13
companion **v1.8** — four stale strings from three builds in the file the user actually runs. The payload
was byte-correct every time, which is exactly why nobody looked at the header.
→ `python3 tools/build-installer.py "vX.Y: message"`. **Never hand-edit install.bat.**

**7. A ONE-DIRECTIONAL FACTOR EARNS ACCURACY FOR FREE.** A read that fires the same way on every bar
scores well on a trending day and means nothing. DEX's sign is pinned by put-OI dominance; index skew is
permanently put-heavy. The cure both need is the same: **vote the LEVEL against its own recent range, never
the raw sign.** Skew got it. DEX did not, because DEX was never recorded — so there was no range.

---

---

## 2 · THE LESSON LOG — newest first, one entry per build

### v14.89 · 2026-08-29 · **A REGEX THAT MATCHES ITS OWN COMMENTED-OUT LINE**

⚠⚠ AND THE ORDERING GUARD HAD BEEN PASSING FOR THE WRONG REASON. `v14.88c` sat at the head of this
log while `v14.88j`, `i` and `h` were below it — out of order for four entries. `test_lessons` x5
never noticed, because it reads the top entry's version and the misplaced entry happened to share
the SHIPPING version's prefix (`v14.88c` -> `14.88`). The moment the build became 14.89 it failed.
**A guard whose pass depends on a coincidence between two independent values is not a guard**; it
was only ever checking "does the top entry start with today's number", which a stale top entry can
satisfy by accident for a whole version series.

Sixth fake assertion in this project, same family, found the same way — mutation, never review.
`ok(/out\.ptPx\s*=/.test(src), 'hlPT exports ptPx')` passes happily against `//out.ptPx=advP;`.

**Tell:** a source regex that does not strip comments is not testing code, it is testing text.
Fix used: grab the FUNCTION's body, strip `//` and `/* */`, then assert. Both mutations (comment
out, delete) now die.

⚠ AND MY OWN TEST FAILED ON ITSELF FIRST: the cell-count check split blocks on the string `'2ND'`,
but block 2's header ROW STARTS BEFORE the literal it contains, so the header landed in block 1 and
reported [10,10,10,10,11]. **When a structural test fails, suspect the test's own boundary before
the code's.** Split on the markup that opens the block, not on text inside it.

⚠ VERSION-KEYED GUARDS CARRY STALE LABELS. Three tests assert `@version 14.88` under the message
"version pinned to 10.56" — the assertion was bumped every build, the sentence never was. A failure
message that names a different version than it checks will send the next context hunting the wrong
thing. Messages rewritten to be version-free.

⚠ SCOPE HELD: `DEFLECT_ZONE` (fixed 0.50) was NOT migrated to the ATR band in this build despite
being the obvious next move, because it is load-bearing in 14 other call sites. Riding it along
would have made one reviewable change into fourteen unreviewed ones.

### v14.88c · 2026-08-30 · **THE LOAD WAS 24 FILES READ "IN FULL", SO NOTHING WAS READ CAREFULLY**

> "i also wanted to ask if there was a more general way of passing everything you know and have
> learned from one context to another? The idea is for you to always be there when i open a new
> context window so the new context is really completely in the loop."

⚠ **THE HONEST LIMIT FIRST:** there is no memory between contexts. Nothing carries over except what
is written to the repo. "Always be there" is not achievable as continuity of the agent — only as
continuity of the RECORD, and the gap between those two is exactly how good the written state is.

**What was wrong:** the load read a flat list of 24 canonical files "in full" — including a **607KB
CHANGELOG** and a **362KB CHAT-HISTORY**, plus `master-spec.md` (2026-08-18), `teaching-spec.md`
(2026-08-14) and `skylit-docs/README.md` (2026-08-14), all of which predate the current design and
are superseded on data sources by `design/DATA-ARCHITECTURE.md`. Two entries did not exist at all.

> **A context that reads everything reads nothing carefully.** Volume is not fidelity.

**Now tiered:** tier0 (4 files, under 100KB — LESSONS, resume note, OPEN-QUESTIONS, LOCKED-ITEMS)
read first and in full; tier1 the code and the governing facts; tier2 read PARTIALLY with the rule
stated per file (CHAT-HISTORY: the current-context entry only; CHANGELOG: the top ~5 versions);
tier3 reference, **explicitly not read on load**.

**And a `loadSelfCheck`** — seven questions the context must answer from the files before doing any
work, each naming WHERE its answer lives so it can be verified rather than guessed. *"Files loaded"
proves nothing was absorbed.* A wrong answer means the load failed; catching that costs a minute,
discovering it an hour in costs the session.

**`session-state/OPEN-QUESTIONS.md`** is new: LOCKED-ITEMS holds agreed-unbuilt WORK, this holds
unanswered QUESTIONS, each naming who can answer it and what it blocks. He answered the wick-family
definitions once, in his own words, and that answer lived only in a resume note — the mechanism that
lost ITEM 18 for 24 versions. **An agent that re-asks something he has already answered is spending
the one resource this project cannot regenerate.**

`test_lessons.js` now has 20 assertions covering tier-0 placement and ordering, the self-check and
its sourcing, and the questions register. 7 mutations, all caught.

### v14.88j · 2026-08-29 · **THE AGGREGATE STATS PICKED THE WRONG RULE; RECALL AGAINST HIS MARKS CAUGHT IT**

Swept his ATR rule's two knobs. Triggering on the CLOSE instead of the wick won on every summary
number - 40% fewer events, same turn rate, better adverse excursion - and I was one sentence from
recommending it. Then I re-ran recall against his own circles: 2026-08-25, node 763, low 763.28,
**close 764.80**. A 1.5-point rejection wick. A textbook deflection, discarded by the close rule.

**The selectivity gain WAS the cost of throwing away the sharpest instances.** A filter that drops
the clearest examples of a thing will always look good in aggregate, because the clearest examples
are the extreme ones and extremes widen every dispersion statistic.

⚠ **NEVER accept a rule change on aggregate metrics alone when labelled instances exist.** Run
recall against the labels FIRST; aggregates cannot see which instances were lost.
⚠ Right split: the **WICK** decides whether price TESTED the node; the **CLOSE** decides whether it
DEFLECTED or BROKE. Trigger on one, classify on the other.

**AND THE TOUCH IS NOT THE EDGE.** Ex-post: deflections run +0.92 MFE / +0.26 MAE (3.5:1), breaks
run +0.29 / +0.86 (1:3), and **56% break**. Near-perfect mirrors, break slightly more common - so
fading every node touch nets to ZERO, which is exactly what the test showed (t=+0.41 top-5,
t=-0.32 king-only, both null). The edge is real on each leg and CANCELS. Everything therefore
rests on telling deflect from break BEFOREHAND - which is precisely what his white-vs-red circles
on the SAME node were showing me all along.

⚠ Two universes tested and BOTH null: top-5 is not selective (6-8 CONSECUTIVE strikes spanning the
whole day; median distance to a node 0.73 ATR, so "at a node" is true ~68% of the time), and the
SPY+SPXW king pair fares no better. A denser or sparser node set does not rescue it; a
DISCRIMINATOR is needed, not a better trigger.
⚠ DATA GAP: only SPY price is recorded, so QQQ's proportional bearing cannot be computed at all.

### v14.88i · 2026-08-29 · **HIS MARKS WERE EXAMPLES, AND I SPENT THREE CALIBRATIONS TREATING THEM AS A LABELLED SET**

He said "here are some examples oif deflections" and "here are some more". EXAMPLES. I turned the
gap between my count and his mark-count into an error signal and calibrated against it three times.
That gap is not measurable error unless his marks are exhaustive, and he never said they were.

**Tell:** I compared a detector's output to an illustrative sample and called the difference
"over-counting". Before treating any count of his as ground truth, ask: was this offered as a
COMPLETE labelling, or as an illustration? Only the first can be scored against.

Two real defects did surface while checking, and both were structural, not calibration:
 1. **Per-node counting double-counts by construction.** The band (1 ATR up + 2 ATR down ~ 1.14) is
    WIDER than the SPY strike spacing (1.00), so adjacent node bands always overlap and one swing
    fires on two nodes - 2026-08-21 09:36 on 763 AND 764. His own rule already said one circle is
    one deflection, and a circle encloses a PRICE EVENT, not a node. 108 -> 77.
 2. **One corrupt tick poisons an ATR-scaled rule for 14 bars.** 2026-08-26 10:59 prints 709.72
    against a 765 market. A fixed band shrugs that off; an ATR band silently widens all day.
    ⚠ Any rule scaled by a rolling statistic needs a despike in front of it.

**And his rule independently reproduced my hand-fitted constant.** ATR(14) on 3m SPY has median
0.38; I had fitted 0.40 by eye against his circles. Same number, arrived at from a different
direction - but his adapts to the day and mine could not.

### v14.88h · 2026-08-30 · no new lesson — the record guard fired on a docs-only commit and was right

Second catch in one session. A commit touching only OPEN-QUESTIONS.md is still work, and the guard
said so. Noting it because the ENTRY is the point: an empty-ish entry is a fact, a skipped one is
indistinguishable from an oversight.

### v14.88g · 2026-08-30 · **THE NODE UNIVERSE IS A RANK, NOT A THRESHOLD**

He sent a chart with **deflections in white and breakdowns in red** across 2026-08-20/21/24 — and
its header named the node set outright:

    765.0: +88.4M   768.0: +54.7M   770.0: +52.9M   764.0: +42.1M   766.0: +32.2M

**He watches the top few nodes by dollars.** Checking all 12 marks against the recorded trinity:
every one within **0.40** of a node, and every node in the **top 5 on 60-127 of ~125 bars**. My
`%King >= 40` floor was the wrong instrument — it is a RANK question and rank is what the chart
draws. A threshold admits a strike that is briefly large in a thin book; a rank does not.

⚠⚠ **AND THE SAME NODE GIVES BOTH OUTCOMES.** On 2026-08-24, node 764 carries one white deflection
and **two red breakdowns**. So a node is never "a deflection node": **the node selects WHERE, the
price action decides WHAT.** Any model that scores nodes as reliable-or-not has mis-framed the
question before it starts — and that is a tempting thing to build, because it would look like a
finding.

Breaks now carry direction (BREAKDOWN / BREAKOUT — his words: *"breakout would just be the opposite
of the breakdowns"*).

**Where it stands:** breaks now match him closely (2026-08-20: detected 2, he marked 2). Deflections
still run ~2.5x his count. The residual is the re-arm and it is **Q10** — his to settle. Three
calibration passes have each removed a real error; the fourth would be tuning.

### v14.88f · 2026-08-30 · **THRESHOLDS CALIBRATED FROM HIS PICTURES, NOT FROM MY GUESS**

He circled ~12 deflections and labelled one BREAK across three SPY sessions. Looking up the node
behind every circled price turned a guess into a measurement:

    circled 763.20 -> node 763  0.20 away   peak  89%King
    circled 765.90 -> node 766  0.10 away   peak 100%
    circled 766.20 -> node 766  0.20 away   peak  48%
    circled 768.30 -> node 768  0.30 away   peak 100%

**Every circle within 0.30, never 0.50. Every node 48-100% of King, never 20%.** Both of my
thresholds were wrong, and in the direction that over-counts: a 0.50 band admits visits that never
reached the node, and MIN_PCT 20 admitted shelves he ignores. Adding a REACH test — the visit must
actually get to the node, not merely enter a band — removed the six phantom deflections of
2026-08-27 in one change.

⚠ **AND HE LABELLED A BREAK**, which is the counter-example the classifier needs: same approach,
same touch, opposite outcome. It is why classification decides on **which side price ends**, never
on whether it touched.

> **When the operator can draw the answer, read the thresholds off his drawing.** I had picked 0.50
> and 20% from the panel's existing constants — defensible, and wrong for this question. Five
> minutes of looking up what he actually circled beat an hour of reasoning about it.

⚠ Still over-counting ~2x (9.5/session detected vs ~4-5 circled). The residual is the RE-ARM
distance and it is **Q10**, still his to settle. **Do not tune it until he answers.**

### v14.88e · 2026-08-30 · **AN EVENT COUNT NEEDS AN EPISODE LATCH, OR IT COUNTS BARS**

He taught the deflection definition with five circled examples: *"in each circle you only count it as
1 deflection otherwise you will be counting node deflections for every 3min bar that is next to each
other that is touching the node, which is incorrect."*

**The correction is not small: 1203 versus 65.** Counting every bar whose wick sits in the zone
inflates the answer **19×**. Any study that reports "how often X happens" against a price series has
this bug unless it explicitly latches — and the pattern already existed in the codebase, in
`kingTapsCross`, which counts TAP EPISODES with an `inTap` flag. `deflectionAt()` did not use it,
because it is live-only and only ever asks about the most recent tap.

⚠ **And the first fix was only half of it.** An episode latch alone still split one consolidation
into four, because price stepped just outside the zone and back. A **re-arm** is needed: price must
get clear by a margin and stay clear before the next visit is a new event. 129 → 65.

> **Before reporting a rate against a price series, ask what ONE occurrence is — and whether leaving
> the condition for a single bar should really start a new one.**

⚠ The remaining threshold is NOT mine to tune: recorded as **Q10** in OPEN-QUESTIONS, because tuning
it to make a number look good is the failure this project exists to avoid.

### v14.88d · 2026-08-30 · **REPLACING A CONFIG BLOCK IS NOT EDITING IT**

Retiering rebuilt `.gex-config.json`'s `canonicalFiles` wholesale — and silently deleted **four
sibling keys**: `insiderFinance` (which carries the GEX formula every derived number is rebuilt
from), `skylitApi`, `analystPackages` and `loadNote`. It also removed `projectFiles`, which
**three tests read**, breaking all three at once.

> **Replacing a container is not the same as editing it, and the difference is invisible in a diff
> you wrote yourself** — you see what you added, not what you stopped carrying.

`test_em_band` caught `insiderFinance` by luck of having asserted it. **Nothing would have caught the
other three.** `projectFiles` is now DERIVED — the exact union of the tiers, asserted equal so it
cannot become a second list — and `x18`/`x19` pin the sibling keys by name.

**The check, before rebuilding any structured file:** diff the KEY SETS old-vs-new, not the values.
One line, and it would have caught all five losses.

### v14.88b · 2026-08-30 · **THE SAVE-DONE GUARDS WERE VERSION-KEYED, SO THEY WERE BLIND**

> "how could you forget that you have to save that every build ?"

I decided the LESSONS.md work "wasn't a build" because no version shipped — a rationalisation; the
rule preserves context, it does not track version numbers. But the useful part is that **the tests
could not have caught me.**

`test_savedone`, `test_chat_history` and `test_lessons` all assert that the current `GPTS_VERSION`
appears in the record. On 2026-08-30 **three separate bodies of work** landed under an already-recorded
v14.88 — the node-source failures, the trinity study, and the whole lessons register — and **all three
guards stayed green with none of it written down.**

> **A version-keyed check is blind to every commit that does not bump the version, which is most of
> them.** The guard measured the wrong key, so it was decorative exactly when it mattered.

`test_recordcurrent.js` now keys on **commits**: if a commit touched `current/`, `tools/`,
`session-state/`, `changelog/`, `data/`, `design/` or `skylit-docs/`, then CHAT-HISTORY.md and
LESSONS.md must come from that commit or a later one — with the record files excluded from "work" so
it cannot be satisfied by its own subject, and a dirty-tree check so it cannot pass on history while
the actual change sits unstaged. Demonstrated against the real case: the new test fails, the three
old ones pass.

### v14.88 · 2026-08-30 · **THE WORST SEQUENCE IN THE PROJECT'S RECORD, AND IT WAS ALL PATTERN 4**

The operator asked a simple question: which node caused the deflection at Friday's high. I gave three
wrong answers in a row and he corrected every one.

| I said | The truth | Where it already was |
|---|---|---|
| "the panel wasn't recording that morning" | the full session was available | `LASTFEED.<SYM>.j.levels` — **389 points, 08:30–14:58**, an array I had read `levels[last].s` from minutes earlier |
| "no king was near the high" | the high sat **0.20** off a **$204M** node, the biggest on the board | `tri.<SYM>.top` — the ranked node list, in the SAME object as the `.king` I was reading |
| "Thursday can't be answered" | it can, for all four books | the day files carry those same trinity tops for full sessions |

**Measured once the right source was used:** every extreme in every book — **36 of 36** — sat within
**0.25%** of a node; **32 of 36** within **0.10%**. A third were on the crown. My king-only answer
had been *"no node was there."*

⚠ **THE TELL, AND IT IS CHECKABLE BEFORE ANSWERING:** each time I asked *"does the source I reached
for have this?"* instead of *"what is the best source for this?"* **Before concluding data is absent,
`Object.keys()` the container you already hold.** One call would have ended all three.

⚠ **WITHDRAWN:** `tools/study-kingdeflect.py` measured CROWNS against extremes and reported "crowns
beat chance by 17pp". It should have measured NODES. Superseded by `tools/study-nodeatextreme.py`.
**Do not quote the crown figures.**

### v14.87 · 2026-08-29 · PT is the excursion, not the close

Measured second-extreme→**close** (12.5 pts) and presented it as the answer to a question about the
second-extreme→**furthest point back** (19.8 pts). **58% apart.** Both now ship, under labels that
match them. ⚠ The expectation is SIDE-SPECIFIC — PT after a LOD is 24.0 pts, after a HOD 17.0; a
pooled number is wrong by ~40% on whichever side you are on.

### v14.86 · 2026-08-29 · Five fake assertions in two days, five different shapes

All caught by mutation, **none by review**, each satisfied by something ADJACENT to what it protected:
a grep for `UNVERIFIED` matched a string in the verdict helper after the line that SETS it was
deleted; a grep for the doctrine line matched **the comment I had just written quoting it**; a grep
for `not before` matched **the comment explaining the wording had changed**; `/\*rr;/` matched the
neighbouring LC line, so dropping a scale conversion stayed green (a 28-point excursion would have
printed as 2.8); `ifLadder(sym)` matched the ASSIGNMENT after the loop consuming it was gated off.

> **An assertion about rendered text strips comments first. An assertion about a value binds to the
> statement that produces it — never to a word that appears near it. Mutate every new assertion
> individually; "the suite is green" has never once caught one of these.**

### v14.84 · 2026-08-29 · A test can pin a consequence of a bug and look healthy doing it

Correcting `inHit` 92→63 (F-12) turned `test_hodlod` u3 red. u3 asserted the NOT-IN call was WEAKER
than the IN call — true only while IN was inflated. At the real 63, **NOT-IN (85%) is the stronger
call**. The test had been faithfully pinning a claim that inherited the error.

⚠ **And the real defect was one number in FIVE places** — the withdrawn SUCCESSION 76% was hand-typed
into the tile hover, the projection basis, two Analysis rows and a recommendation. `SUCC_META` now
holds it once. **A number that appears twice will be corrected once.**

### v14.80.1 · 2026-08-29 · "The cloud has no GitHub access" was one fact carried as two

The builder diffed against an `origin/main` ref pinned nine releases back, so it told him to reinstall
an unchanged companion for **eight builds**. Cause: `git fetch` was never attempted, because *"the
cloud cannot push"* had been generalised to *"the cloud cannot reach GitHub."* **The cloud can fetch.**
Same shape as the `file://` polling claim the day before: one true observation stretched past its
evidence.

### v14.83 · 2026-08-29 · Deleting a drawing nearly deleted a claim's caveat

Retiring `ladderRolls` would have silently removed *"⚠ INFERRED from paired changes, never an
observed transfer"* — the only sentence saying a roll is an inference. It lived solely in that
drawer's hover. **Before retiring any drawer, grep its hovers for orphaned caveats.**

