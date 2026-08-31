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

### v15.11 · 2026-08-31 · **I SHIPPED A REPLAY THAT DREW TODAY'S KINGS ON A PAST DAY**

He dragged the new slider to 13:33 and said the king lanes, statuses and nodes were missing. Nothing
was broken — the panel was in replay — but the measurement was damning: **1 ladder node bar against
4 live, 2 king pills against 5, 14 ROC values against 26.**

⚠⚠ **AND ONE OF THOSE WAS NOT THINNESS.** `ladderKings()` reads the SPXW crown from a latch keyed to
`ctTodayStr()` and the SPY/QQQ crowns from `LASTFEED`. **All three are TODAY.** On a replayed past
day that silently draws today's crowns over Friday's ladder — mislabelled, not missing, which is the
one class this project cannot detect afterwards. **I wired the book through `tapeMap` and assumed
everything downstream followed. It does not: several consumers reach around it to live stores.**
The check I should have run before shipping is mechanical — grep the render path for `LASTFEED`,
`ctTodayStr` and the latch keys, and ask of each whether replay has an answer for it.

⚠⚠ **THE FIX I WAS ABOUT TO SHIP WOULD HAVE BEEN A SECOND STATE RULE WEARING THE SAME WORDS.** I had
agreed to derive BUILDING/FADING from `d60`. The panel's real rule is `rawAccumMap`, which reads a
node's abs-sequence out of `slicesFor()` — and a run of frames IS that sequence. **Feeding the real
function real history beats reproducing its vocabulary with a proxy**, and it cost less code. When a
feature needs "the same thing but from another source", look for the SEAM the original reads through
before writing a parallel rule.

⚠ **TWO MORE FAKE ASSERTIONS, SAME FAMILY, CAUGHT ONLY BY MUTATION.** `k4` grepped for
`RTri.SPY.king` — so `if(false && RTri.SPY.king…)` stayed green. `k5` grepped for `RXm.QQQ.px` — and
turning QQQ from a proportional bearing into a raw level left that token sitting in a tip string.
**Sixth occurrence of "a grep cannot tell a live branch from a dead one."** Executing `ladderKings`
and checking the returned crowns then caught two further mutations that dropped the scale conversion
entirely — a crown drawn at its raw strike sits ~18 points off on an ES rail.

⚠ `val('ACC_WINDOW')` returned garbage because that declaration carries a trailing comment — the
landmine PROJECT-CONSTANTS records next to L-K, hit from the test side this time. The test got its
own numeric reader rather than the declaration being edited to suit it.

⚠ **AND HE NAMED SOMETHING THAT IS NOT IN LIVE EITHER.** "make it exactly like live, including …
gamma profile" — the gamma profile was REMOVED from the face at v14.81 at his own request. Exactly
like live means it stays absent. **When a request lists something the baseline does not have, say so
instead of building it**; the alternative is reinstating a feature he cut, under cover of a different
request.

### v15.10 · 2026-08-31 · **THE FEATURE WAS ALREADY HALF-BUILT AND I ALMOST WROTE A SECOND ONE**

He asked for a replay slider. My first instinct was to size a new renderer: something that reads the
repository and draws a past face. **`tapeMap(sym)` has had a stored-book branch since v14.55** — it
is how the panel already shows Friday after the close — and every consumer on the face reads its book
through that one door. Replay is that branch with a different source, and the diff is small because
of it.

⚠⚠ **THE TELL WAS IN THE FILE I HAD ALREADY READ.** `lastBookSave()` stores
`{king, kingKd, pct{}, vel{}}`, and the snapshot's `vend.rows` is `[k,cur,d5,d15,d60,d1d]` — the same
four fields under different names. **Two structures that carry the same fields are usually one
mechanism**, and noticing that is worth more than any amount of careful new code.

⚠ **AND I MEASURED HIS STORE BEFORE DESIGNING, WHICH SETTLED TWO THINGS I WOULD HAVE GUESSED WRONG.**
`gpts_repo_v1.snaps` holds 2,149 frames over 18 days and is NOT bounded by the localStorage budget —
which at that moment held 28 bars. Reasoning from the budget alone would have concluded replay could
reach back ninety minutes.

⚠⚠ **A NEW DEPENDENCY IN A HOT PATH BROKE NINE TEST FILES, AND THE SYMPTOM NAMED THE WRONG CAUSE.**
`measureBars` is eval'd in isolation by tests that do not define `replayOn`. The bare call threw
ReferenceError; its catch called an undefined `swallow()`; that throw escaped into `hodLod`'s outer
catch, which returns `{ok:false}` — **which the face reads as "the session has no bars."** Not a
crash, a plausible empty. **This is v15.08's lesson verbatim** ("a silent catch turns a missing stub
into a mystery") and it recurred inside the build whose comments quote it. Every replay call site now
carries `typeof replayOn==='function'`, which is the pattern `recorderSave` already used for `lsPut`.

⚠⚠ **MUTATION FOUND TWO FAKE ASSERTIONS OF MINE, AND ONE WAS THE FIXTURE, NOT THE RULE.**
- The snap-to-nearest-frame test passed with the snap *removed*, because three near-evenly-spaced
  frames make `round(p*(n-1))` land on the same indices as nearest-in-time. **The fixture was too
  regular to separate the two behaviours.** A gap was added — which is also the case the feature
  exists to handle honestly.
- The "it goes amber in replay" test grepped for the colour, and the colour also appears in the
  border and the badge, so deleting the background tint stayed green. Now it compares the strip's
  GROUND between the two modes.
**Twenty mutations, run one at a time; the suite being green caught neither.**

⚠ **AND A GUARD FIRED ON THE RECORD QUOTING ITS OWN TRIGGER WORD.** `test_chat_history` searched the
whole current entry for the generator's placeholder text — and the entry contains the transcript, so
a reply that QUOTED the placeholder made a correctly-filled file go red. Scoped to the section body.
**Ninth occurrence of the comment-contains-the-token family, arriving one level up: the RECORD is now
big enough to contain the strings its own guards look for.**

⚠⚠ **AND THE SAVE ITSELF WAS INCOMPLETE UNTIL HE ASKED.** I shipped the build, wrote CHANGELOG,
LESSONS, CHAT-HISTORY and the resume note, and printed ✅ SAVE DONE — and **skipped
`OPEN-QUESTIONS.md`, which is on the mustBeCurrent list**, on a day that produced real material for
it: his six circled deflections, 5 of 6 caught, and a structural miss. Worse, **Q2 had been answered
BY A BUILD eleven versions earlier** (TREND left the face at v14.90) and the entry never closed,
because nothing links "a build removed this" to "the question about it is settled".
**A checklist item that is usually a no-op is the one that gets skipped when it isn't.** The fix is
to read the list, not to remember it — and to check this file against the FACE every build.

⚠ **A REVERTED `v10.js` MADE THREE SAVE GUARDS REPORT FAILURE ON A CORRECT BUILD** — the harness copy
had been restored to the tracked v11.48 artefact to keep the tree clean, so every version-keyed test
compared against v11.48. **This is v15.09c's own lesson, hit again in the same session that wrote
it.** Regenerate before believing any version-keyed result.

⚠ **HE REJECTED THE MOCKUP FORMAT BEFORE THE DESIGN.** An artifact page of six states came back as
"i'm not interested in a replay card" — the trimmed face I drew to fit six panels side by side read
as a widget, not as the panel rewinding. **What worked was injecting the strip into his LIVE panel
and screenshotting it there.** For a change to an existing dense UI, a drawing of the change competes
with his memory of the real thing; the real thing with the change in it does not.

### v15.09c · 2026-08-31 · **THE RESUME NOTE PASSED ITS GUARD ON A STAMP WHILE ITS BODY WAS NINE VERSIONS OLD**

No code shipped. The previous context's window closed before the save finished, and what that
exposed is worth more than the tidy-up: **`latest-resume-note.md` carried `v15.09` in its header —
which is the entire content of `test_savedone`'s check — while its body still described v14.80.**
v14.81 through v15.09 never reached it. The guard was green throughout.

⚠⚠ **THIS IS THE SAME DEFECT CLASS AS v14.88b, ONE LEVEL UP.** That entry established "a
version-keyed check is blind to every commit that does not bump the version" and fixed it with
`test_recordcurrent`. This is the complement: **a version-keyed check is also blind to a file that
carries the version and nothing else.** The stamp is not the content. `test_recordcurrent` DID fire
here — it is commit-keyed — which is the argument for that test in one line.

⚠ **AND THE PREVIOUS NOTE IS KEPT, NOT OVERWRITTEN** (`2026-08-31_resume-v14.80.md`), with a banner
saying what it is. A note that failed this way is the case study for its own §0 warning, and deleting
it would leave only the abstract rule.

⚠ **THE OTHER HALF: `test_lessons` x5 IS PASSING ON A PREFIX AGAIN.** The top entry read `v15.09b`
against a shipping `15.09`, and the regex `/^### v[0-9.]+/` truncates to `### v15.09`, so the match
is a coincidence of naming — **exactly the v14.89 lesson** ("a guard whose pass depends on a
coincidence between two independent values is not a guard"). It has not been changed, because
narrowing it would break the `bNN` suffix convention this log uses for work that ships no version.
**Noted rather than silently fixed: the next context should decide deliberately.**

⚠⚠ **AND I REPEATED PATTERN 4 INSIDE THE VERY NOTE I WAS WRITING TO PREVENT IT.** I put "THE
STORAGE QUOTA FIX (F-10) — the highest-value UNBUILT item" at the top of the next-actions list,
sourced from `LOCKED-ITEMS.md`. **It has been built since v14.68.** `lsPut`, `LS_CAP_KB`,
`LS_BUDGET_KB`, `LS_HEALTH`, `lsTotalKB` and `__gptsDebug.storage` are all in the shipped panel and
`test_storage.js` was green in the same suite run I quoted. One grep would have ended it.

⚠ **THE TELL, AND IT IS NEW: A PARKED PATCH READS AS UNSHIPPED WORK.**
`session-state/pending/v14.68-bounded-writes.patch` sits against a **v14.67** base and no longer
applies — because its work landed 42 versions ago. The directory exists for "built but deliberately
not shipped yet", so nothing distinguishes *waiting* from *long since merged*. **A patch file is
evidence of an intention, never of a state.** Check the symbols in `current/`.

⚠⚠⚠ **AND THEN I GOT THE REPLACEMENT WRONG TOO, AND SHIPPED IT AS CONFIRMED.** The correction
above originally continued: "the budget is 3600 KB against a ~6 MB session, so the shedder trims today
oldest-first and the export keeps only the last 90 minutes — the ⓪a NOT-IN call fires at 08:40 and is
never in the record meant to score it." That went to GitHub as **FINDINGS F-10b, status CONFIRMED**.
**It is false. WITHDRAWN — see F-10c.**

    snaps[].feat   131 of 131 bars   08:30 -> 15:00 CT   ALL 48 feature keys
    day.feat        29 bars          13:36 -> 15:00 CT

**The reads were complete the whole time.** `day.feat` is a resolution QUEUE; the record lives inside
each snapshot, and resolved records are mirrored to IndexedDB where `featStats()` reads them.

⚠⚠ **THE DISPROOF WAS INSIDE THE EVIDENCE I CITED, IN THE SAME PARAGRAPH.** I wrote that snapshots
spanned 08:30-15:00 and the queue spanned 13:36-15:00 — and the shedder I blamed trims **snaps and
feat in the same pass**. Both cannot be true. **I noticed the asymmetry and did not follow it**, which
is the exact move v15.00 records as the whole diagnosis of another bug. **Cross-checking two fields
that must agree is free, and it is the cheapest disproof available.**

⚠⚠ **THREE TIMES IN ONE SESSION, EACH TIME FROM A SINGLE ARRAY.** The ledger said unbuilt so I said
unbuilt; the queue was short so I said shed; both times the answer was in a sibling field of the same
object — `current/` for the first, `snaps[].feat` for the second. **Pattern 4 is not about obscure
data. It is about asking "does the source I reached for have this?" instead of "what is the best
source for this?"**

⚠ **AND THE INSTRUMENT WAS LYING IN A WAY THAT FLATTERED THE WRONG ANSWER.** `day-digest.py`'s
`dataHealth` reads the QUEUE and prints "COLLAPSED — feature records cover almost no bars ... do NOT
compute rates over it". On a day whose reads are complete that verdict is wrong, and it is what I
quoted, twice, as evidence. **A verdict from a tool is a measurement of what the tool measures.**
⚠ F-9's historical table (`2026-08-27: 15 records / 1 bar`) rests on the same counter and should be
re-read against `snaps[].feat` before it is trusted.

⚠ **THE REAL DEFECT, STATED NO FURTHER THAN THE EVIDENCE GOES:** `buildDayExport` exports `day.feat`
and not `FEAT_ARCHIVE`, so resolved outcomes older than the queue never reach the repo. **What trims
the queue to 29 bars is UNKNOWN** — `FEAT_KEEP_BARS`=160 rules out the bar cap, intact snapshots rule
out the shedder. **Two mechanisms named confidently, two wrong. The third statement needs a
measurement, not another read of the source.**

⚠ REVERTING A GENERATED ARTEFACT MAKES A VERSION-KEYED TEST LIE. A stop-hook flagged `v10.js` as
uncommitted — it is the harness's scratch copy, regenerated by `tools/run-tests.sh` and tracked on
origin frozen at **v11.48**. Reverting it to satisfy the hook made `test_chat_history` report
"shipping 11.48" and fail. **A single test file run after a clean checkout is testing v11.48, not the
panel** — the stale-but-green state this project already warns about. Always regenerate before
running one file alone.

### v15.09b · 2026-08-31 · **A WIDTH GUARD CAUGHT ME DOING THE EXACT THING IT WAS WRITTEN FOR**

`test_ladder` w1b: "THE BUILD MUST NOT GET WIDER. 632 -> 588 was the whole point; a later change that
adds a column and pushes it back up has undone this build without anything else noticing." I added a
48px roll lane and tripped w1, w1b and w1c together.

⚠⚠ **THE GUARD'S PURPOSE IS NOT TO FORBID, IT IS TO MAKE THE DECISION VISIBLE.** Its own words are
"without anything else noticing" — so the correct response is neither to bump the number silently nor
to abandon the feature, but to argue for it in the open. The lane was squeezed 44 -> 20px to land on
w1's 640 ceiling, w1b was amended ONCE with the reason written into the assertion, and w1c was
repointed at the new last column so LAD_W still cannot drift from the layout.

⚠ **AND THE AMENDMENT CARRIES THE NEXT ARGUMENT.** The assertion now reads "640 IS NOW THE CAP. The
next column that wants width argues for it here, in these words." A guard that is relaxed without
leaving that sentence behind is a guard that will be relaxed again by someone who never saw the first
argument.

### v15.09 · 2026-08-31 · **I RE-BROKE A COUNTING RULE HE HAD ALREADY TAUGHT ME**

"each day will only have a few pullback opportunties and each pullback has 1 deflection so when you
have that many deflections in a day, its because the data was not classified correctly."

`study-rollsupport.py` counted every bar against every node: 481 events over 4 sessions, ~120 a day.
**He taught me this exact lesson on 29 Aug** with his circled charts — "in each circle you only count
it as 1 deflection" — and `study-deflect-atr.py` still carries the fix. I wrote a NEW study three
days later and reverted to per-bar counting without noticing.

⚠⚠ **A LESSON FIXED IN ONE FILE IS NOT A LESSON LEARNED.** The correction lived in the study it was
written for; nothing carried it into the next one. **When a counting rule is corrected, it belongs in
the LESSON LOG as a rule about counting, not as a comment in one script** — which is what this entry
is for.

⚠ THE TELL WAS AVAILABLE WITHOUT HIM: **120 pullbacks a day is not a plausible number** and I printed
it without pausing. A count that disagrees with how the thing behaves in life is a defect in the
counter, not a finding. **Sanity-check a rate against lived experience before reporting it.**

⚠ THE FIX IS A DEFINITION, NOT A THRESHOLD. A pullback is the extreme of its OWN 30-minute
neighbourhood — a structural property — rather than "moved more than X". Tightening a threshold would
have produced any number I wanted; the neighbourhood test produces 3 a session because that is how
many there are.

⚠ AND A CAVEAT SPLIT ACROSS A STRING CONCATENATION IS UNGUARDED. `'NOT A '+'CLAIM...'` reads
perfectly and cannot be grepped, so the test protecting it passed on a mutation that deleted the
phrase. **A guarded sentence that a guard cannot match is not guarded.** Kept whole on one line.

### v15.08 · 2026-08-30 · **THE PANEL MEASURED A PROXY WHILE THE REAL INSTRUMENT SAT IN STORAGE**

"its the es that i am trading but using spxw nodes." The structure side was right — ES has no book,
so kings and nodes come from other tapes. The MEASUREMENT side was not: HOD, LOD, the candle, EFF
and the GREEN/RED call were computed from SPY candles x an EMA ratio, while true ES 1-minute bars
sat unused in localStorage the whole time.

⚠⚠ **I VERIFIED AGAINST ATLAS BEFORE BUILDING, AND IT CHANGED THE ARGUMENT.** Atlas draws each book
in its own native scale, unconverted. Reading the real numbers off his page — ES 7722.50, SPX
7711.76, SPY 769.35 — showed SPX and ES are TEN POINTS apart while SPY is 10.04x away. **That is why
his pairing works and why SPY does not belong in the path.** I would have made a weaker version of
this change from reasoning alone; the page settled it in one call.

⚠ **A PROXY IS NOT WRONG BY ITS AVERAGE ERROR, IT IS WRONG BY WHAT IT CANNOT SEE.** SPY x ratio gives
the RANGE to within hundredths — a fact that would have justified leaving it. But SPY and ES do not
tick in step, so the MINUTE an extreme prints can differ, and TOOK / BOP / MUD / W.END / HL GAP are
all timestamps. And SPY has no overnight session while ONH/ONL already come from ES bars. **Judge a
proxy on the quantity you actually use it for, not the one that is easiest to check.**

⚠ THE TRAP INSIDE THE FIX: `hodLod` set `out.scale=rr` from `dispR()`. With ES bars that would have
multiplied ES prices by ten — the exact failure the change exists to remove, reintroduced by the
change itself. The scale now comes FROM the bar source. **When you swap a data source, every derived
constant of the old source is suspect.**

⚠ AND THE TEST HARNESS NEEDED THE NEW DEPENDENCY. `hodLod` threw into its own catch and every
extremity assertion failed with an EMPTY result rather than a wrong one — which reads like the
function is broken rather than un-stubbed. **A silent catch turns a missing stub into a mystery.**

### v15.07 · 2026-08-30 · **TRANSPOSING A TABLE FIXED A READING PROBLEM I HAD NOT NOTICED**

He asked for the three rows turned vertical to save space. What it actually fixed is that the E row
sat UNDER the A row, so comparing an actual with its expected meant tracking two rows. As a column
pair they sit side by side, and `TOOK 5h30 vs 34m` — a low that took TEN TIMES the normal time —
became visible at a glance. **A layout change requested for space returned a comprehension gain I
had not seen was missing.**

⚠⚠ **I RAN THE DESIGN PASS AGAINST A CATALOG, NOT MY TASTE, AND IT CAUGHT FOUR THINGS I HAD SHIPPED.**
Colour decorating values ($725 in amber — money is not a status); no `tabular-nums` (the actual
mechanism behind "aligned", which I had been approximating by hand); spine segments abutting instead
of separated by 2px of surface; a dashed axis, which reads as "projection". **"Make it look nicer"
is a checkable procedure, not an aesthetic judgement** — and my eye had passed all four.

⚠ **A DERIVED DIMENSION CANNOT DRIFT.** The candle's height was CHOSEN (216, then 137) and each time
it disagreed with the table beside it, leaving the void he pointed at twice. It is now
`DAYCOL_HD + DAYCOL_N * DAYCOL_ROW` — add a field and both move together. **When two elements must
match, derive one from the other rather than setting both.**

⚠ AND THE REDUNDANCY WAS THE WHOLE PROBLEM. The void under the candle held a session track that
restated its own labels and a dollar total that appeared 60px away in the DAY column. **A number
printed twice is worse than a number printed once**, and cutting both cost nothing.

⚠ SIX TEST FILES ASSERTED THE ROW FORM and had to be re-expressed for columns — "3 A rows" became
"one NOREAD guard per data cell", "SLvl is the column after 1ST" became "SLvl heads the 1ST column".
**When a layout is transposed, its tests describe the OLD axis** and rewriting them is where you
find out whether they asserted the CONTRACT or the SHAPE.

### v15.06 · 2026-08-30 · **THE PANEL FOLLOWED THE CHART WHEN EVERY NUMBER IN IT STANDS ON ONE BOOK**

`activeSym()` returned QQQ whenever a QQQ chart was open. But the 284-session HOD/LOD corpus, the
GREEN/RED rule, the deflection geometry and the last-session latch are ALL the SPX/SPY book. One
click onto another tab silently swapped the panel onto a book with none of that evidence behind it.

⚠⚠ **THAT IS THE ROOT OF THE WHOLE EVENING.** The nine-pixel ladder, the SPX King against a 716
price, the empty QQQ book — every one of them is this. I chased four symptoms across five builds
without asking why the panel was reading QQQ at all. **When several unrelated-looking faults share
one instrument, the instrument selection IS the fault.**

⚠ AND HE HAD ASKED BEFORE: "I thought i had this requirement in the past but it must have been in
another context." A standing requirement that lives only in a conversation is lost when the context
ends. **This is what PROJECT-CONSTANTS and the lesson log exist for** — the requirement is now code
plus a test, not a memory.

⚠ A PIN MUST ANNOUNCE ITSELF. Pinned to SPX while a QQQ chart is open, every number on the face is
SPX — which is correct AND is exactly how someone reads a price off the wrong instrument. The badge
`◉ SPY book (chart: QQQ)` costs nothing and removes the trap.

⚠ THE CANDLE COLLISION WAS TWO COLUMNS SHARING ONE X. Level names at cx+14, MUD centred on cx —
fine until the body sat low. **Two independently-positioned text blocks in one narrow frame will
eventually meet; give them separate columns rather than tuning offsets.**

### v15.05 · 2026-08-30 · **I HID REAL DATA TO AVOID DRAWING IT BADLY**

v15.04 hid the ladder when its frame did not match the chart price. The operator: "the requirement
was to see fridays informaiton ... and you just removed everything from the display."

⚠⚠ **THE ROWS WERE FRIDAY'S BOOK. THE DATA WAS NEVER THE PROBLEM — THE FRAME WAS.** I diagnosed a
frame fault and then suppressed the CONTENT, which is the one thing in the pipeline that was
correct. **Hiding a panel converts a rendering bug into a data-loss bug**, and for a user whose
whole request was "let me see Friday", that is strictly the more expensive failure.

⚠ THE REAL FAULT WAS FOUR LINES AWAY AND I DID NOT LOOK. `emRailBounds` widens the rail to hold
EVERY pile — correct for a far King, catastrophic for a node from another book. One SPX pile
dragged `hi` to 7760 against a 716 `lo`. **When a frame is wrong, read the code that BUILDS the
frame** before reaching for a guard downstream of it.

⚠ SKIP, DO NOT CLAMP. Clamping an off-scale pile to the edge stacks it invisibly there — the
original reason this loop widened at all. The pile belongs to another rail; omit it.

⚠⚠ AND THE PATTERN ACROSS THIS WHOLE SEQUENCE: v15.02 correct but routed onto a broken path, v15.03
explained the symptom, v15.04 suppressed the symptom, v15.05 fixed the cause. **Three builds of
treating a symptom because I never read the function that produced it.**

### v15.04 · 2026-08-30 · **I TURNED EMPTY INTO GARBLED, AND EMPTY WAS BETTER**

v15.02 made `showingStaleBook()` correctly return false on QQQ. That was right, and it dropped the
ladder off the stale path onto the LIVE path — where the SPX-scale fallback lives. Result: strikes
7611-7710 drawn against a 716 price, a 7,200-point span, every row inside NINE PIXELS.

⚠⚠ **A CORRECT CHANGE CAN ROUTE EXECUTION ONTO A BROKEN PATH THAT NOTHING ELSE WAS EXERCISING.** The
live QQQ path had presumably been wrong for a long time; nothing reached it because the stale path
swallowed the case. **When a fix changes WHICH branch runs, the newly-reachable branch is now yours
to verify** — I verified the branch I wrote and not the one I exposed.

⚠ **DEGRADE TOWARD SILENCE, NOT TOWARD NOISE.** Empty was a poor experience; garbled was a worse
one, because it destroyed his ability to read anything at all and looked like total breakage. When
a render cannot be correct, refusing with a reason beats attempting it.

⚠ THE GUARD IS ON THE FAILURE MODE, NOT THE CAUSE. The upstream scale fallback is still wrong and
the message says so. Fixing the symptom while NAMING it as a symptom is honest; fixing it silently
would have buried the real bug for another four builds.

⚠ AND `if(false) return ...` SURVIVED THE ASSERTION AGAIN — every message string stayed matchable.
Pinned the STATEMENT SHAPE instead: the return must be the first thing after the scale test.

### v15.03 · 2026-08-30 · **I SHIPPED A FIX THAT COULD NOT TAKE EFFECT UNTIL MONDAY AND DID NOT SAY SO**

v15.02 made the last-session latch per-book, correctly. But the latch is WRITTEN DURING RTH — so on
a Sunday it changed nothing, his QQQ ladder was still blank, and he read the build as broken. He was
right to: from where he sat, I had "fixed" it twice and it was still empty.

⚠⚠ **A FIX WHOSE EFFECT IS DEFERRED IS NOT A FIX UNTIL THE FACE SAYS WHEN.** I knew the write only
happens in RTH and did not think about what he would see before then. **State the latency of a fix
in the same breath as shipping it** — and when the code can explain an absence, put the explanation
where the absence is.

⚠ AND THE DIAGNOSIS WAS ONE STORAGE READ AWAY THE WHOLE TIME. The latch held SPXW 7465-7960 frozen
at 14:59 CT and no QQQ book at all. I had already read that key twice this session without asking
what was NOT in it. **Check what a store is missing, not only what it holds.**

⚠ I ALSO NEARLY ADDED A "FALLBACK" that served the SPXW book on a QQQ chart — which is precisely the
bug v15.02 removed, re-entering as a helpful-looking convenience. **A fallback that substitutes the
wrong object is the original bug wearing a different name.**

⚠ Multi-line emit deletion survived a mutation AGAIN (fourth time): removing the first of two lines
left the second matchable. **Delete the whole construct.**

### v15.02 · 2026-08-30 · **A PER-SYMBOL FEATURE STORED IN A SYMBOL-LESS KEY**

The last-session book latched SPXW into `gpts_lastbook_v1` — one key, no symbol dimension — while the
serve gate asked only "is there a latch?", never "for which chart?". Correct on SPY by coincidence,
because SPY's ladder is governed by SPX. **Empty on QQQ, whose book was never latched at all.**

⚠⚠ **THE BUG WAS INVISIBLE FOR AS LONG AS HE STAYED ON SPY.** A feature that is right for the
default case and absent for every other one looks finished. **When a value is per-symbol, the KEY
must be per-symbol** — storing it flat is the bug, and the missing dimension is what makes it
undetectable from the working case.

⚠ AND I ALMOST FIXED A PROBLEM THAT DID NOT EXIST. I diagnosed "the labels are cut off" as the
ladder overflowing the panel and wrote an `overflow-x:auto` rule — which was ALREADY THERE, 7 lines
above mine. The ladder scrolls; it was never clipped. **Read the existing CSS before adding to it**;
the real clipping was one fixed-width cell.

⚠ A LOOSE ADJACENCY WINDOW PASSED ON A NEIGHBOUR. Asserting `text-overflow:ellipsis` within 400
chars of `.g3ldlv` stayed green after the rule was deleted, because the file has EIGHT such
declarations and the next one was in range. Tightened to the exact fragment at <200. **When a token
is common in the file, proximity is not identity.**

### v15.01 · 2026-08-30 · **HE QUESTIONED THE OUTPUT AND THE OUTPUT WAS RIGHT; THE RULE BEHIND IT WAS NOT**

"are you sure this is correct because the candle shows the market took out both the prior day high
and the prior day low." Checked the arithmetic: PDH and PDL differ by 6.98 against a 7.0 range, so
price tagged both by hundredths and reversed. **The display was correct.**

⚠⚠ **AND THE CHALLENGE STILL FOUND A BUG.** v14.99 used a SYMMETRIC tolerance, when this project had
already settled that the band is ASYMMETRIC — 1 ATR short, 1.5 ATR through, calibrated against his
own circled charts at v14.91. A level price EXCEEDED must be judged by the through-tolerance. I had
built the same geometry twice and only one copy carried the calibration.

⚠ **A CHALLENGE TO AN OUTPUT IS WORTH INVESTIGATING EVEN WHEN THE OUTPUT SURVIVES IT.** The right
answer was "yes, and here is the arithmetic" — but stopping there would have left a rule that
disagrees with the one four builds of work went into. **Verify the output AND re-read the rule.**

⚠ TWO MORE CONDITION-MUTATIONS SURVIVED, same shape as v14.98: I mutated an ASSIGNMENT while the
assertion matched neighbouring arithmetic, and flipped a CONDITION while the assertion matched the
emit text. Fifth and sixth this session. **The assertion must name the thing the mutation changes.**

### v15.00 · 2026-08-30 · **AN ASYMMETRY BETWEEN THREE FIELDS WAS THE WHOLE DIAGNOSIS**

HodN and LodN read em-dash every day; PTN found a node. I noted that as two separate observations
for two builds before seeing it was ONE fact: `deflKings()` reads the CURRENT king, and the PT
extreme is the only one recent enough for the current king to still be near it. **A 10:00 high was
being measured against a 16:00 king.**

⚠⚠ **THE FEATURE FAILED IN THE SHAPE OF ITS OWN "NO DATA" STATE**, which is why it survived four
builds. Em-dash is a legitimate reading — no king within tolerance — so nothing looked wrong.
**When a field CAN legitimately be empty, empty is not evidence that it works; you need a case where
it MUST be non-empty.** PTN was that case sitting in plain sight.

⚠ THE FIX OMITS SPX RATHER THAN SUBSTITUTING. Only SPY and QQQ have journeys. Filling SPX with its
CURRENT king would have been the precise error being fixed, one field over. **When a fix has a gap,
leave the gap visible — do not patch it with the thing that caused the bug.**

⚠ AND A WEAK MUTATION LOOKED LIKE A FAKE ASSERTION. Replacing the FIRST "46.6%" left two more in the
file, so p1 stayed green and I nearly recorded it as a bad test. It was a bad MUTATION — the
assertion is "this evidence exists somewhere", and only removing every instance tests it. Fourth
mutation-technique error this session, all the same shape: **mutating something narrower than the
assertion covers.**

### v14.99 · 2026-08-30 · **THE EXCLUSION HE ASKED FOR FELL OUT OF THE INCLUSION RULE**

"levels around the edges not levels that the maket blew through and kept going." My first instinct
was two rules — one to admit edge levels, one to reject traded-through ones. **A level price traded
through is mid-range, and therefore near NEITHER extreme.** One rule does both, and the second rule
would have been a threshold that could drift out of agreement with the first.

⚠ IB IS EXCLUDED **BY NAME**, NOT BY THRESHOLD. He said "do not include ib". A distance test would
let it back in on any day the initial balance happened to sit on a wick — which is common. When an
exclusion is categorical, encode it categorically.

⚠ I DROPPED THE SHAPE SPINE AT v14.98 CHASING WIDTH, AND IT WAS THE WRONG THING TO CUT. The
wick/body/wick percentages are the only figure on the candle that SUMS TO 100 — the check that the
whole daily-bar decomposition is honest. It costs 3px. **When trimming for space, rank by evidence
carried per pixel, not by what is easiest to remove.**

⚠ TWO MUTATIONS SURVIVED BECAUSE I DELETED ONE LINE OF A THREE-LINE EMIT. The remaining lines still
matched. **A deletion mutation must remove the whole construct** — loop, statement, block — or it
tests nothing. Third mutation-technique error this session; the pattern is mutating something
NARROWER than the assertion covers.

### v14.98 · 2026-08-30 · **HIS SKETCH CARRIED THE LAYOUT PRINCIPLE, NOT JUST THE CONTENT**

"the candle is taking up too much horizontal space" — with a drawing. The drawing was not a list of
fields; it was the fix. **Stacking the annotations above and below the bar is what makes it narrow**,
because the width becomes the BAR rather than the labels. I had put labels in a right-hand column
and three percentage bars in a left-hand one, which is why it needed 150px.

⚠ AND THE SKETCH RESOLVED AN AMBIGUITY I WOULD HAVE GUESSED WRONG. His "2h 9m" under the HOD is not
TOOK — 08:30 to 10:03 is 1h33. It is 10:03 -> 12:12, the leg that FOLLOWS the extreme. Checking his
two numbers against the live panel (HL GAP 2h09, LC GAP 2h48) settled it in one step. **When a
sketch carries numbers, arithmetic against real data tells you what they mean.**

⚠ FOUR MUTATIONS SURVIVED AT FIRST because I mutated CONDITIONS (`if(x)` -> `if(false)`) while the
emitted text stayed matchable. **A condition mutation does not test a presence assertion; deletion
does.** Re-run by DELETING each line, three then died — and the fourth (n48) matched a variable
DECLARATION rather than its emitter. A value computed and never drawn is not a feature.

### v14.97 · 2026-08-30 · **I FIXED THE CONSUMERS TWICE WHEN THE SOURCE WAS WRONG**

v14.93 made two call sites agree on a scale. v14.94 gave them one function. His screen did not
change, because the fault was never between consumers: **their payload returns a spot on one scale
and strikes on another** — `{"cr":7735, ..., "ps":765}` in one object.

⚠⚠ **CONSISTENCY BETWEEN CONSUMERS CANNOT REPAIR AN INCONSISTENT SOURCE.** Both my fixes were
correct improvements and neither touched the bug. **When a fix does not move the symptom, the model
of the fault is wrong — stop refining the fix and re-derive the fault.** I refined twice.

⚠⚠ **AND I ONLY FOUND IT BY READING HIS LIVE PANEL.** The values that settled it — pill 7710.6,
T 773.34, stored SPXW king 769.849, ratio 10.0387 — are not derivable from source. Four builds of
reasoning about code lost to one look at the running thing.

⚠ THE FIX IS A SELF-CHECK, NOT A CONVERSION. A level near the money must land near the price the
chart draws; if the ladder is off by ~the futures ratio it is corrected once and NAMED `fixed:10x`.
**A silent correction would have re-hidden the same class of bug.**

⚠ AND s16 CAUGHT NOTHING AT FIRST: it asserted `function _sane()` EXISTS, so deleting the CALL left
it green. Second "declared but never invoked" gap this session (after "assigned but not returned").
**Assert the invocation, not the declaration.**

### v14.96 · 2026-08-30 · **A TEST THAT PASSED WHILE THE SCREEN WAS VISIBLY BROKEN**

He said "the alignment is messe up". The ⓪a section was THREE separate `display:table` divs, and a
table sizes its columns from its OWN content — so the three laid out independently and block 2 began
halfway across the row.

⚠⚠ **`test_nodeat` n29 ASSERTED THE BLOCKS HAD EQUAL CELL COUNTS, AND IT PASSED.** I wrote it in
v14.90 believing it enforced his alignment contract, and I said so in the changelog. **Equal cell
COUNTS do not produce equal cell WIDTHS across independent layout contexts.** The property he asked
for is a RENDERING property; counting cells cannot see it, and a green test made me stop looking.

⚠ **THE RULE: a test must assert the MECHANISM that delivers the property, not a proxy that
correlates with it.** The mechanism here is ONE TABLE. That is now what is asserted — one table, and
every row the same width — and splitting it back into three is caught.

⚠ AND THE HOVER ALMOST DIED WITH THE WRAPPER. The GREEN/RED tip lived on the block-3 div; removing
that div would have deleted the only place the model's n, CI, base rate and null results are
reachable. Fourth time this shape has appeared (ROLL BIAS, the INFERRED caveat, the NO CALL, now
this). **Before deleting a container, list what only lives on it.**

⚠ The candle wrapped above the table because I set `flex:1 1 380px` on a 560px panel — it could
never sit beside a 150px candle. I chose that number without checking his panel width, which is
recorded in localStorage and was two tool calls away.

### v14.95 · 2026-08-30 · **`SYM` vs `sym` — ONE TYPO, THREE BROKEN FEATURES, ZERO RED TESTS**

The operator said "look at my screen and check yourself". I did, and the entire ⓪a section was
missing from the DOM while the panel reported v14.94 loaded. `secDay(sym)` — and I had written
`SYM` in four places since v14.91.

⚠⚠ **`node --check` PASSES ON AN UNDECLARED IDENTIFIER.** It is not a syntax error; it is a runtime
ReferenceError. And the section is mounted inside `swallow()`, so the throw was caught and the block
silently disappeared. **A swallow around a whole section converts a typo into an invisible deletion.**

⚠⚠ **THE TWO INSIDE try/catch WERE WORSE THAN THE ONE THAT THREW.** `hlNodeAt(SYM,...)` and
`gdRead(SYM)` failed into their own catches and returned null forever. HodN/LodN/PTN and the
GREEN/RED call would have read em-dash for weeks and looked like "no data today". **A catch that
returns a neutral value turns a crash into a plausible reading.**

⚠⚠ **47 ASSERTIONS ABOUT THIS SECTION AND NONE COULD SEE IT**, because every one is a SOURCE GREP.
Greps prove text is present; they cannot prove it runs. That gap is now `test_undefined_ids.js`.

⚠ AND THE LINT ITSELF NEEDED THREE FIXES — comma-separated declarators, regex literals read as
identifiers, multi-line `var` statements — all FALSE ALARMS. Then loosening the parser to silence
them destroyed its ability to detect anything, and **only mutation testing caught that**. I had
already written "PASS" five times against a lint that detected nothing. **A lint's parser needs the
same scrutiny as the code it polices, and the only proof it works is a bug you inject on purpose.**

⚠ My first mutation was ALSO wrong: `dayCandleSvg(sym, D, PTL)` matches the function DEFINITION as
well as the call, so I renamed a parameter and concluded the lint was broken. **Check that a
mutation hit what you think it hit before believing what it tells you.**

### v14.94 · 2026-08-30 · **"DECLINE RATHER THAN DEFAULT" IS RIGHT FOR NUMBERS AND WRONG FOR SCALES**

v14.93's second fix made `sessionLevels` decline when the scale was unknown. I wrote, approvingly,
"an absent level is a gap you can see, a mis-scaled one is a lie you cannot" — and shipped a build
that **blanked his levels after hours**: "the problem is that i wont be able to work."

⚠⚠ **THE PRINCIPLE WAS RIGHT AND I APPLIED IT TO THE WRONG OBJECT.** Decline when the VALUE would be
wrong. Do not decline when the value is fine and only its SCALE is hard to reach — especially when
that scale is derivable, which this one always is, because `FUTMODE.r` is a persisted EMA that
survives the close. **"Correct or nothing" is a false choice whenever a third option is derivable.**

⚠ AND I ONLY HALF-FIXED THE ROOT. v14.93 made two call sites AGREE; it left two sites each DERIVING
a scale, which is the precise condition that caused the original divergence. `displayScale()` is now
the single source. **Making two implementations agree is not the same as having one implementation.**

⚠ The test assertions had to be REVERSED — s11/s12 pinned the declining behaviour I had just
shipped. A guard written in the same breath as a mistake will faithfully protect the mistake.

### v14.93 · 2026-08-30 · **ONE SWITCH, TWO CONSUMERS, DIFFERENT REQUIREMENTS — AND NOTHING ERRORED**

The 10x mismatch. `emBand()` honoured `dispIsFut()` via `dispR()` (needs only `FUTMODE.r`);
`ifLadder()` honoured it only if `FUTMODE.futPx` existed, and **silently fell back to the CASH
scale** without it. `r` is a persisted EMA, `futPx` is null after hours. So the panel had a
reachable state where one half read ES and the other read SPY, every individual number was correct,
and every DIFFERENCE between them was nonsense.

⚠⚠ **A SILENT FALLBACK ACROSS A UNIT BOUNDARY IS THE WORST KIND OF BUG.** Nothing throws, nothing
looks empty, and every value stays plausible on its own. It is only visible in a SUBTRACTION —
which is why it surfaced as "−6937.26" and not as a crash. **When two code paths honour the same
switch, they must not have different preconditions for honouring it.**

⚠ THE FIX IS ONE SCALE, NOT A SMARTER FALLBACK. Deriving the same ratio `emBand` uses means they
cannot diverge. And the third state is NAMED (`scaleSrc`) rather than inferred — an undiagnosable
state is how this survived four builds.

⚠⚠ **AND IT LIVED IN A SECOND PLACE, WHICH I ONLY FOUND BY AUDITING RATHER THAN STOPPING AT THE
REPRO.** `sessionLevels(sym, EB.scaleUsed : 1)`. Every other consumer of `scaleUsed` guards with
`>0` and skips; this one line fell back to cash. **When a bug has a shape, grep for the shape, not
the symptom** — the symptom I was handed was the T-distance, and it would have left the rail broken.

⚠ DECLINING BEATS DEFAULTING. `SESSL` is now simply not computed when the scale is unknown. An
absent level is a gap you can see; a mis-scaled one is a lie you cannot.

### v14.92b · 2026-08-30 · **THE INSTALLER SILENTLY EXCLUDED EVERY tools/ SUBDIRECTORY**

`os.listdir('tools')` + `isfile` skips directories. So `tools/nightly/` — the harness, the protocol,
the verdict ledger and the **pre-registered hypothesis bank** — was committed locally and would
never have reached GitHub. The build reported success every time.

⚠⚠ **THE HYPOTHESIS BANK IS THE WORST POSSIBLE THING TO LOSE**, because its entire value is that it
was demonstrably written BEFORE the data existed. A copy restored later proves nothing. This is the
same shape as the ES corpus loss recorded in `data/es-1min/README.md` — "anything that only exists
in a sandbox commit does not exist" — and it recurred within two days of that note being written.

⚠ **A MANIFEST THAT WALKS ONE LEVEL IS A MANIFEST WITH A SILENT FLOOR.** It never errors; it just
ships less than you think. Caught only because the payload was decoded and inspected after building
— which is now the third time post-build verification has caught something the build itself called
a success.

### v14.92 · 2026-08-30 · **THE REGIME FIELD WAS A HARDCODED CONSTANT, AND THE RECORDER BANKED IT FOR NINE SESSIONS**

He asked whether the positive/negative gamma regime could be used as a filter. It is recorded. It is
also worthless: `neg:false` was **hardcoded** on the trinity read path, so all **284 samples across
9 sessions** say "positive gamma" because that line says so.

⚠⚠ **A CONSTANT THAT LOOKS LIKE DATA IS WORSE THAN A MISSING FIELD.** A gap is visible and stops you.
A constant answers every query, passes every null check, and quietly makes any study built on it
meaningless. Nine sessions of recording produced zero usable regime data and nothing flagged it.

⚠⚠ **THE TELL WAS TWO RECORDED FIELDS DISAGREEING.** `bk.neg` claimed 100% positive gamma while
price sat BELOW the flip (`deriv.zg`) on 60% of the SAME snapshots. Both describe the same thing;
they cannot both be right. **Cross-check fields that should agree — the contradiction is free and it
is what exposed this.** I only looked because the 100%/0% split was implausible.

⚠ THE FIX IS `null`, NOT A DERIVED GUESS. The Trinity pane does not expose the sign, so there is
nothing honest to derive it from. Null makes the hole visible; anything else would re-bury it.

⚠ AND THE BETTER SIGNAL WAS ALREADY THERE: price vs the FLIP is real, varies (40% above / 60%
below), is continuous rather than binary, and is what the doctrine actually describes. Registered as
gx-010/gx-011. **The broken field was hiding a working one.**

⚠ NOTHING WAS TESTED, DELIBERATELY. Six gamma sessions and a broken regime field is not a study, and
running one anyway is how a number nobody can defend reaches the face.

### v14.91c · 2026-08-30 · **A FILTER INHERITS THE EDGE IT IS FILTERING, AND MY HARNESS COULD NOT SEE IT**

Testing his prior-day value-area question, eight combinations were scored and FOUR came back
PROVISIONAL at 82–84% against a 79.5% incumbent. They were mutually contradictory — "open ABOVE
VAH" and "open IN VALUE" are DISJOINT sets that scored 82.1% and 82.2%.

**That contradiction is the tell, and it is the cheapest one available.** When several partitions
that cannot all be true score the same, nothing is being discovered: the incumbent's accuracy is
just being resampled.

⚠⚠ **THE HOLE: the shuffle test cannot judge a FILTER.** Shuffling labels destroys the incumbent's
edge — but a filter is a SUBSET of the incumbent's days and inherits that edge, so it clears a bar
built on the assumption there is no edge. The right control is a RANDOM SUBSET OF THE SAME SIZE:
median 79.5%, **p95 87.2%** at n=39. All four sat inside it. `subset_null()` added.

⚠ AND I RE-JUDGED MY OWN RESULT WITH IT. gx-008 (90.2%, n=61) survived — p95 luck band 85.2% — but
I did not know that until I built the control, and I had already written it into the ledger as a
finding. **A new control must be run against the results already banked, not only against new ones.**

⚠ THE NEGATIVE RESULT IS THE DELIVERABLE HERE. Prior-day POC/VAH/VAL predict neither direction
(±4pp, |z|<1), nor range (46% big-range outside value against 50% expected), nor support/resistance
— and on TWO tests the distance-matched SHAM beat the real level (16% vs 21%, 61% vs 64%). Logged
as gx-009, CLOSED NEGATIVE, so an intuitive and widely-believed idea is not re-proposed monthly.

### v14.91b · 2026-08-29 · **THE NIGHTLY LOOP: SIX SESSIONS OF GAMMA, AND A HARNESS THAT CAUGHT ITSELF**

Designing the LLM refinement loop surfaced the constraint that governs it: **there are 6 sessions of
gamma book and 284 of price.** Every shipped model is price-only. Nothing using the gamma book can
be tested for months. So the loop starts by ACCUMULATING and PRE-REGISTERING — 8 hypotheses locked
before the data to test them exists, which is stronger pre-registration than anything obtainable
later.

⚠⚠ **THE HARNESS FOUND A HOLE IN ITSELF ON ITS FIRST RUN.** Two proposals cleared all four bars at
90% and 84% — and both were **subsets of the incumbent's firing days**. They select WHEN the shipped
rule works; they add no new signal. **A confidence modifier reported as a predictor double-counts
one edge.** `relation()` and `duplicate_of()` added. The two survivors were also 74% the same idea.

⚠ **THE SHUFFLE TEST IS THE ONLY BAR THAT SCALES WITH AMBITION.** Over just 4 proposals the
best-of-K noise band reached **64% at p95** against a 51% base. At twenty proposals a night — which
is what "have the LLM find datapoints" means — anything under ~70% is indistinguishable from noise.
Without it the loop manufactures a finding every night, forever.

⚠ And the division that makes it safe: **the LLM proposes, the harness disposes.** The LLM never
sees a result before its hypothesis is locked and has no vote on the verdict. Not distrust —
arithmetic.

### v14.91 · 2026-08-29 · **THE FRAME WAS THE FINDING, AND HE HAD TO TELL ME**

> "do you realize that i am taking the model of the daily bar and trying to measure the movements
> in it from open to close"

No. I had been treating ⓪a as a LIST OF TIMING STATISTICS that happened to share a section, for
eleven builds. They are the anatomy of ONE DAILY CANDLE. His own numbers prove it and I could have
run that check any day: 51% + 35% + 14% = 100%, and WICK% is not a ratio, it is where the OPEN sits
in the bar.

⚠⚠ **THE TELL: I could decode every field but never asked what they were FOR.** TOOK + BOP = WICK,
and 10:00 + 51m = 10:51 = W.END — I verified that arithmetic to the minute and STILL did not ask why
those quantities would be interesting together. **Fields that reconstruct one object are a model of
that object.** When several measurements close on an exact identity, the identity IS the design.

⚠ AND THE FRAME ANSWERED AN OPEN QUESTION FOR FREE. PTWICK sat as Q1 for four builds, "undefinable",
because I stopped at "reclaim would mean a different event". Once the anchor is named — WICK is
measured off the OPEN — the mirror is mechanical: PTWICK is the same span off the SECOND EXTREME.
**A question that will not close is often a framing problem, not a data problem.**

⚠⚠ **A ONE-LINE RULE BEAT THE LOGISTIC REGRESSION, 77% TO 74%.** I built the regression first,
because a model is what "predict" suggests. The rule ships. ⚠ And the negative results were the
valuable half: the open predicts NOTHING (gap AUC 0.479, day-of-week 0.447), and the PRIOR DAY is
AUC 0.500 — an exact coin flip. **He asked about the prior day expecting it to help; reporting that
it does not is worth more than finding something that "works".**

⚠ NINTH COMMENT-BLIND ASSERTION, and the first inside a test HELPER rather than an assertion — the
cell counter counted commas in a `//` comment and reported 11 cells where there were 10. I chased
the CODE first. **A helper that reads source must strip comments too.**

⚠ A TWO-LETTER CSS CLASS COLLIDED. `.g3ct` was already the contract chip; `test_em_band` caught it
in one run. In a 27k-line single file, short class names are a collision waiting to happen.

⚠ AND I SHIPPED A DUPLICATE FOR ONE BUILD: the v14.90 top strip AND row 3 both printed HL GAP /
HL RNG. **When a thing MOVES, delete the old home in the same edit** — n39 caught it only because
it asserted the strip's existence and the assertion had to be rewritten.

### v14.90 · 2026-08-29 · **THE RECORD SAID "REMOVED" AND THE CODE WENT ON RENDERING**

> "you did not implement all the changes we have been talking about"
> "even the dntend Brk area was also removed .. all those changes and you didn't implement any of them"

Four agreed changes were unshipped. **Two of them were written down as DONE.** The resume note's
agreed-layout block ends `FAR SIDE block: REMOVED` and has since v14.88 — while the block rendered
every session. A record that asserts a change which never shipped is worse than the unshipped
change, because it is the thing that stops anyone looking.

⚠ **AND I READ THAT NOTE THIS SESSION.** I quoted its column scheme back accurately and still did
not check the scheme against the code. **A spec read as documentation is not a spec CHECKED against
the build.** The check is mechanical and takes one grep per line.

⚠⚠ **THE ALIGNMENT WAS BROKEN BY A LEFTOVER, NOT A MISTAKE IN THE NEW WORK.** HL GAP / HL RNG /
LC·RNG were still living in block 2 — they were supposed to move to the top strip at v14.87 — and
they pushed PTWick% and PTMUD three columns off WICK% and MUD. Then v14.89 added HodN/LodN/PTN on
top, making rows of 10 and 11 that could not align at all. **When a layout looks wrong after an
addition, check what should have LEFT before blaming what arrived.** `test_nodeat` n29 now asserts
the two blocks are the SAME WIDTH — a per-block width check would never have caught this.

⚠ REMOVING A BLOCK DELETES WHATEVER LIVES ONLY THERE. The FAR SIDE table was the only home of the
NO CALL — the sharpest thing the model says. Re-homed to the read hover. Third time this exact
shape has appeared (ROLL BIAS v14.83, the INFERRED caveat, now this): **before deleting a block,
list what it is the only home of.**

⚠ TWO MORE COMMENT/CSS-BLIND ASSERTIONS, both mutation-caught: `/g3farhd/` and `/g3dayhl/` matched
STYLE RULES rather than emitters, and `/THE NO CALL/` matched the comment I had just written saying
it was re-homed. Seventh and eighth in this family. **Any assertion whose regex could match prose or
CSS must strip comments and target the emitter.**

⚠ Three of my OWN test assertions were wrong before the code was: the block splitter split on text
inside the block, and a scope check required `function secBias` inside `panelV3`'s body. **When a
new assertion fails, suspect the assertion first — it is younger than the code.**

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

