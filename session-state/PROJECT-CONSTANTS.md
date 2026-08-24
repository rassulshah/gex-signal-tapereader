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
