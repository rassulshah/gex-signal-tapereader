# PROJECT CONSTANTS — GEX Signal Tapereader

Landmines, doctrine and mechanics that outlive any one build. **A load must read this AND
`latest-resume-note.md`** — that file says where we are, this one says what will bite you.
Touch this file only when one of these things actually changes.

---

## ⚠ EIGHT FAILURE PATTERNS — THESE EXPLAIN NEARLY EVERY BUG IN THIS PROJECT

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

**5. Defensive try/catch makes a missing reference INVISIBLE.** A function never defined (`tradeNodes`), a
variable from another scope (`rr`). Header emitted, rows gone, looks exactly like "nothing to show".
→ `swallow(tag,e)` records it, `__gptsDebug.renderErrors()` exposes it, `node tools/smoke.js` FAILS on a
non-empty list. A regex scan for undeclared identifiers was tried and ABANDONED — it flagged keywords and
regex-literal contents, and a noisy check gets switched off.

**6. When an edit script asserts, CHECK IT LANDED.** v11.46 shipped half a feature because a python edit
aborted on a failed assertion and the failure went unnoticed.

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

## 📦 THE INSTALLER FILENAME CARRIES THE VERSION — user-mandated 2026-08-24

**Deliver `install-v<VER>.bat`, never a bare `install.bat`.** `tools/build-installer.py` writes both;
the versioned copy is the one to send and is gitignored so it never enters the payload.

⚠ **Why:** eight installers went out in a single session all named `install.bat`. The user ran an
older one and reported bugs that had been fixed three builds earlier — and I spent a live-market turn
diagnosing a panel that was simply not the panel I had shipped. **A file whose name cannot be told
apart from seven others is a file that gets run out of order.**
⚠ Always confirm the RUNNING version from the panel footer before diagnosing anything: the first
`v11.x` in the markup can be stale hover text, so read the footer stamp specifically.

---

## 📎 ALWAYS SEND THE TAMPERMONKEY LINK — user-mandated, restated 2026-08-24

**Every message that ships a build ends with the output of `bash tools/release-links.sh`, pasted.**
Not a link typed from memory, not "the installer is attached" — the script's block, because it reads
both `@version` values and marks which script actually CHANGED against origin/main.

⚠ Linking a script that did NOT change makes Tampermonkey offer **Reinstall** instead of **Update**,
which reads exactly like a failed push. That is why the script exists and why its output is what gets
pasted. BUILD-CHECKLIST step 9 has said this since v11.9 and it has still been missed.

---

## ⛔ THE PROCESS FAILURE THAT ENDED THE 2026-08-24 SESSION

**The user stopped work with "we need to stop . you are making a mess." No bug caused it. Scope did.**

The user asked for small UI tweaks. I returned a full section redesign. They narrowed it; I returned
another redesign. They narrowed it again — *"just update the current price in the white box"* — and I
still added extra figures beside it. Their next message: *"no, you idiot."* **Four mockup revisions
burned, and every one had to be walked back.**

**The rule that would have prevented all of it already exists in the skill and is user-mandated:**

> **ONE AT A TIME. Discuss exactly ONE element per message. Never list all open items and their fixes
> in one reply. State the one item, its fix, ask, STOP.**

⚠ **The tell:** if a reply is taking shape with three headings, a comparison table, and a "which do
you prefer" at the bottom — that reply is the failure mode. Delete it. Answer the one thing that was
actually named.

⚠ Two more standing instructions from the same user, both repeated: **"dont make any build without
asking me"** and **"show me mockups first always."**

⚠ A related habit worth naming: **do not volunteer adjacent work.** Finding a real defect while
implementing something else is useful; folding it into the current build uninvited is not. Report it,
and let the user decide whether it becomes a build item.

---

## THE THREE RULES THE USER HAS STATED

> **"I am big on 50 sma, everything else is supplementary."**

② BIAS is **not a tally**. The 50-SMA gives direction; SKEW / ACCUM / PA confirm or they do not, and the
count is the confidence. Confirmers never outvote the SMA. DRIFT gates **the call**, not itself.

> **"any time a trade occurs, it must be off a node. The levels give context but the trade is off a
> node, preferably a pullback node."**

⑤ EXECUTE refuses with `NO NODE — NO TRADE`. Stop sits beyond the NODE'S OWN zone. ③ shows nodes as rows
beside the levels, tagged `@CR`/`@PS`, `▸` for the pullback engine's pick.

> **"get the data and calc from insider finance instead of calculating it ourselves"** (2026-08-22)
> and *"stop writing your own stuff when it is already there."*

**Use their published value wherever one exists — but match the WINDOW, or rule 1 bites.** Their header
walls are ALL-EXPIRY (CW 7900 / PW 7500 on 2026-08-22); our ladder's CR0/PS0 are 0DTE (7700/7665, verified
reproducing their 0DTE view exactly). Both are "their values" and they answer different questions.

---

## THEIR PAYLOAD — SETTLED, DO NOT RE-DERIVE

`initialData` = `ticker · tickerDetails{...} · spot · options[] · timestamp · isStale`. **Nothing computed.**
Each contract: `strike, expireYear, expireMonth, expireDay, cp, gamma, delta, openInterest, impliedVol,
bid, ask`. `c.pub` exists with 8 keys — `zeroGamma, callWall, putWall, skew, skewSlope, termSlope, atmIV,
pcRatio` — **all null**. ⚠ **Their PAGE publishes all eight anyway** (see the fetch-path section).

**What we compute:** DEX (⚠ **puts already carry NEGATIVE delta — do not flip the sign**), 25Δ SKEW (voted
against its OWN range), expected move (ATM straddle; refused unless both legs quote at ONE strike within 1%
of spot), gamma flip (FALLBACK ONLY → `FLIP*` tagged `calc`), level depth (GEX ∩ DEX, ◆◆/◆).
⚠ **Level depth aggregates across a ZONE (max, not sum)** — compare a level to the book it came from.

---

## BUILD/TEST MECHANICS

**`tools/BUILD-CHECKLIST.md` runs on EVERY build. The user should never have to say "save".**

- ⚠ **THE SANDBOX CONTAINER RESET TWICE ON 2026-08-22.** **The installer's push is the only durable copy.**
  Recover with `git clone https://github.com/rassulshah/gex-signal-tapereader.git`.
- **Cannot push from the sandbox** — github.com is blocked at the proxy. The .bat pushes from the user's machine.
- **npm cannot install in the sandbox either** (403). `jsdom` is absent, so `test_tapeking.js` cannot pass here.
- Harness reads **`./v10.js`** — `cp current/gex-signal-tapereader.user.js v10.js` FIRST.
- **`node tools/smoke.js`** — fails on anything a render catch swallowed.
- **`test_no_dupes.js`** fails on any NEW function-name collision. Four to date. **GREP BEFORE NAMING.**
- **Version pins** in `test_direction_grade.js`, `test_pipeline_indicator.js`, `test_rules_v2.js`,
  `test_read_v1047.js`. Bump every release, plus `GPTS_VERSION` and the `@version` header.
- **`test_rules_v2.js` pins the RULE COUNT** (now 63) and `test_feature_enrollment.js` requires every seeded
  rule to exist in `learning/rules.json` with the FULL shape — including `lastVerified` and
  `walkForward:{sessions,held}`. Adding a feature means adding its rule to that file, correctly shaped.
- Installer: header + base64; `more +<HDRLINES>` must equal the `exit /b 0` line. **NO POWERSHELL**
  (Avast IDP.HELU.PSE88) — `certutil` + `tar`. CRLF.
- **`eval()` inside a forEach callback declares into the CALLBACK's scope.** Join and eval once.

### THREE INSTALL FAILURES, IDENTICAL FROM THE USER'S SIDE
| symptom | cause | one-call check |
|---|---|---|
| GitHub serves the old version | the installer did not push | clone and read `@version` |
| GitHub new, browser old | **the page was already open. RELOAD.** | reload the tab |
| raw stale ~5 min | CDN cache, `max-age=300` | read `cache-control`/`age` |

⚠ **CHECK THE RAW URL YOURSELF BEFORE TELLING THE USER TO CLICK.** This cost four cycles on 2026-08-22.

## DEBUG SURFACE
`__gptsDebug.` — `renderErrors` `ifShape` `optKeys` `skew` `accum` `rolls` `ifLadder` `nodeChart` `phase`
`regime2` `pa` `bias` `steps` `roll` `face` `expSets` `unified` `sanity` `ifChain` `callPutRows`
`feedShape` `pbEntry` `if` `ifFetch` `ifParse`

## STANDING INSTRUCTIONS
- **FEATURE ENROLLMENT (2026-08-17): no feature ships un-enrolled** — DATA (per-bar record), ANALYSIS
  (scorecard with rate + n), TESTING (question + rule in `learning/rules.json`, graduates at n≥20).
  Mechanism = the FEATURES registry. **DEX and EM shipped un-enrolled and sat on the face for releases with
  nothing checking them. That is how a cell nobody can check survives.**
- **Ask before giving code** — the user may have more requests. Exception: unambiguous bug reports.
- **Show MOCKUPS before coding.** `mockups/frame_em_band.html` is the EM band review (⚠ its numbers use the
  WRONG week-EM — it was drawn before the dte0 bug was found; the shape was approved, the values were not).
- **ALWAYS send the Tampermonkey link with every build** — and ONLY for the script that actually changed.
- **Do not over-build.** *"The requirement is just to display the levels from IF. why are you making things
  more complicated"*. The user pushed back on an over-designed EM band and was right: a √T reconstruction
  and a severity gradient were both cut, and **the pushback is what surfaced the dte0 bug.**
- **No useless text — there is no space.** If it is needed it goes in a hover.
- **Verify against real data before proposing thresholds.**
- **Flag substitutions.** If the design says X and X is not built, say so at the time.
- **MODEL ROUTING (2026-08-15):** delegate mechanical work (renames, packaging, search) to cheaper models
  via the Agent tool; reserve the main model for design, statistical interpretation and review.
- **ONE AT A TIME** when discussing open items — state one, ask, stop.

### Failure pattern: blaming the handler for a bug in the box (SECOND OCCURRENCE)

v12.2: the panel "wouldn't resize" — the panel box was 524px while its content was 1068px and
`overflow` was `visible`. The panel never contained its content. Four attempts had gone at the drag.

v12.5: the pop-out "could shrink but not grow" — the panel box was 1104px inside a 598px window,
because `height:100%` was set on a parent that had no height. `makeResizable` reads `oh` off that
rect, so the drag arithmetic was fed a number nearly double the window and hit the clamp instantly.
Five more versions had gone at the drag.

RULE: when a drag, resize, or hit-test misbehaves, MEASURE THE BOX FIRST —
`getBoundingClientRect()` on the element AND on the thing that is supposed to contain it, plus
`innerHeight`. Compare them. Only after those three numbers agree is the handler a suspect.

COROLLARY: a percentage height needs a parent WITH a height. `height:100%` against an auto-height
parent computes to `auto` — silently, with no error and no warning. If a rule sets a percentage
height, something above it must set a height too.

COROLLARY: an asymmetry IS a clue, but read it correctly. "Shrinks but won't grow" pointed me at the
pointer leaving the window (v12.4). The real cause was a starting value on the wrong side of the
clamp. Both explain the asymmetry; only measurement distinguishes them.

### HARD PLATFORM LIMIT: a Document PiP window has a MAXIMUM HEIGHT that cannot be exceeded

Measured live 2026-08-24 on the user's machine (secondary monitor, `screen.availHeight` 820,
`screen.height` 858, `availWidth` 3049):

- `resizeTo()` on a Document PiP window throws `NotAllowedError: requires user activation`.
- User activation is CONSUMED by the call, so ONE gesture buys exactly ONE resize.
  Logged: `DOWN act=true` -> `move1 res=ok` -> `move2 THREW NotAllowedError`.
- Even a resize that returns `ok` is REVERTED by the browser if it exceeds the cap.
  `resizeTo(w, 800)` reported `immediate=800`, then Chrome snapped it back to 693 within 600ms.
- FIVE consecutive `+20px` attempts, each with fresh activation, ALL reverted:
  `was=693 asked=713 res=ok settled=693 (reverted)` x5.
- WIDTH is freely resizable (`outerW` moved 321 -> 341 -> 353 during testing). HEIGHT is not.

The ceiling on this machine is **693 outer / 598 inner** against an 820px work area. The pop-out
opens AT the cap, so there is no headroom to grow into, by grip or by window edge.

⚠ EVERY VERSION FROM v11.93 TO v12.5 WAS TRYING TO BUILD A HANDLE THE BROWSER WOULD NEVER HONOUR.
v12.5's `height:100%` fix is real and worth keeping — the panel now genuinely fills its window and
scrolls internally instead of overflowing. But vertical GROWTH of a PiP window is not available.

RULE: before building a control for a platform API, verify the platform PERMITS the operation —
one call and one measurement. Nine versions of handler work went into a capability that does not exist.

### Failure pattern: a popup window is QUIRKS MODE, and this stylesheet cannot survive it

`window.open('')` returns an about:blank document with NO doctype, so `compatMode` is `BackCompat`.
In quirks mode **CSS class selectors are case-insensitive**. This stylesheet contains classes that
differ only by case — `.g3emt` (FRAME rail container) and `.g3emT` (centred label carrying
`transform:translateX(-50%)`) — so the container matched the label's rule and slid 40px off the left
edge, wrecking the whole rail. Everything inline-styled looked perfect, which is what made it read
like a layout bug rather than a mode bug.

Measured live 2026-08-24: popup `BackCompat` / Atlas `CSS1Compat`; `el.matches('.g3emT')` returned
TRUE for a `.g3emt` element; computed transform `matrix(1,0,0,1,-39.9988,0)`.

FIX: `d.open(); d.write('<!doctype html>...'); d.close();` BEFORE copying styles.
A Document PiP window never showed this — `requestWindow()` creates a standards-mode document.

RULE: any window this code writes into must be given a doctype explicitly. And treat
case-only-distinct class names as a latent hazard — they are a silent collision waiting for any
quirks-mode context.

DIAGNOSTIC NOTE: the tell was that inline styles rendered correctly while class-based rules did not.
That split says "selector matching", not "missing stylesheet" — and the stylesheet was in fact present
(18 tags, 1.38MB). Checking `compatMode` and `el.matches()` took one call.

## SOURCE-OF-TRUTH POLICY (v13.1, user-directed)

**We capture the vendors' numbers. We do not invent our own.**

1. A number the vendor publishes is captured VERBATIM and displayed as theirs.
   Skylit is the source of truth for FLOW (nodes, node dollars, rate of change).
   InsiderFinance is the source of truth for STRUCTURE (levels, gamma/delta profiles, tiers).
2. We derive ONLY what the vendor does not publish.
3. Every derived value carries a source tag and is marked on the face. The `IF·pub` vs `calc`
   asterisk on FLIP is the pattern; it is now the rule, not a one-off.
4. **The recorder stores vendor raw AND our derived read — never the derived alone.**

### Why 4 exists — the v13.1 audit finding

`recNode` stored `pct` (our %King normalisation), `st` ('Building'/'Fading', our label) and `hist`
(our own 15-second samples). Skylit's published dollar value and their own 5m/15m/60m/1d deltas were
stored NOWHERE. If one of our rules is wrong, every recorded day inherits the flaw, and the nightly
LLM can neither detect it nor re-derive a better answer, because the inputs were never kept.
**We would be training on our own opinions.** `snap.vend` now carries their numbers verbatim and
`snap.srcs` marks which fields are ours.

### Enforcement
`test_velocity_policy.js` — the harvest computes no delta of its own, the recorder emits `vend` and
`srcs`, derived fields are tagged 'derived', and a broken capture REFUSES rather than rendering zeroes.

### The cross-check this buys
Because the displayed deltas are Skylit's own, THEIR UI is a live oracle: click a strike in their
ladder and the popup must match this panel. A disagreement is a real bug, visible without any
instrumentation. That is the TESTING leg of feature enrollment, obtained for free.

### Failure pattern: an assumption written in the voice of a measurement (THIRD occurrence)
- v12.3 test comment: "resizeTo IS permitted inside a drag handler, because a drag is a user gesture."
  Only the first call is. Six versions trusted it.
- v11.95 CSS comment: "the SPX strike now matches the ES price in size." It did not — the strike was
  raised to 8.65px and the ES price left at 6.5px. Eighteen versions carried the false claim.
- v13.1 (mine): "a roll is mass moving from one strike to another" implied conservation. Measured:
  receivers gained 2.8x-16.5x what losers shed. The floor I proposed, $500K, was 12x too high and
  would have detected almost nothing.
RULE: a comment stating a fact must say how it was measured, or say that it was assumed.

### Failure pattern: a second computation of the same thing (v13.2)

The rail and the NODES list were both deciding "which nodes matter" and "what colour is this node",
independently. They disagreed within one session of shipping: a node drawn on the rail was absent from
the list, and a node yellow on the rail was purple in the list.

RULE: when two parts of the face must agree, they read the SAME ARRAY — not two arrays built from the
same source with matching rules. Matching rules drift; a shared reference cannot. If a filter is needed
for one view, filter the shared array at the point of use and say so.

### Failure pattern: rigour applied to an unverified premise (2026-08-25)

A roll detector was designed from first principles, measured against the live book, corrected when the
measurement disproved the first guess, mutation-tested and shipped. Every step was done well.

`skylit-docs/learn/rolling-floors-ceilings.md` was in the repo the entire time and says:
**ONE migration = NOISE. TWO consecutive same-direction = SIGNAL. THREE = CONFIRMATION.** It also
defines rolling as the LARGEST FLOOR and LARGEST CEILING migrating across updates — not transfers
between arbitrary neighbouring strikes.

The shipped detector fires on a SINGLE 15-minute observation between neighbouring strikes. It reports
as signal precisely what the doctrine calls noise. ⚠ OPEN DEFECT — ROLL BIAS is under-confirmed.

The same session invented a six-stage node lifecycle while `learn/node-lifecycle.md` defined four
stages (FRESH / TESTED / DELIVERED / DECAYING) on an axis orthogonal to REAL-vs-HEDGE.

RULE: **check the doctrine BEFORE reasoning, not after.** Careful method on a wrong premise produces a
confident wrong answer, and the care makes it harder to spot.

⚠ BUT NOT "OBEY THE DOCTRINE" — see the 2026-08-25 amendment in `SOURCE-OF-TRUTH.md`. Checking first
means the prior is known before you reason, so a deviation is a decision instead of an oversight. The
count rule may well be too tight at our one-minute cadence; the way to find out is to detect
permissively, label confidence honestly, score the outcomes, and let the evidence set the threshold.
A real event that does not fit a definition is a finding, not an error.

RULE: the repo is the first place to look, not the last. The Academy docs, the IF notes and a written
`design/spec-event-causality.md` were all present and unread while their subjects were discussed at length.
