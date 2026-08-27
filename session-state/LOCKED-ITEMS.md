# LOCKED ITEMS — specified, agreed, NOT YET BUILT

**⚠ THIS FILE EXISTS BECAUSE THE RESUME NOTE LOST ITEM 18 AND COST THE OPERATOR A WHOLE SESSION.**

_Created 2026-08-27._

---

## WHY THIS FILE EXISTS — read this before you skip it

On 2026-08-27 the operator said, three separate times, that a Yahoo Finance solution already existed
and that **files in git were being updated to get the data**. Three times I said I could not find it.
I searched the repo HEAD, GitHub, Drive and the skills directory, reported "it was never pushed",
proposed rebuilding it, and found a different project's pipeline and misreported that as the answer.

It was in **this repo's own history the entire time** — `session-state/latest-resume-note.md` at
commit `72e820e`, item 18, locked 2026-08-16, with a complete specification.

**Two failures caused that, and both are now fixed in `.gex-config.json`:**

1. **THE LOAD PROCEDURE SAID `git clone --depth 1`.** A shallow clone has no history. Every resume
   note ever written was one command away and structurally invisible. **The load now clones full.**
2. **THE RESUME NOTE IS REWRITTEN IN FULL EACH BUILD.** That is deliberate and correct — but it means
   any item not re-typed is *gone*, silently, with no deletion to notice. Item 18 appears in every
   note from 2026-08-15 (`72e820e`) through v11.23 on 2026-08-20 (`04f6f80`), and in **none** after.
   Nobody cancelled it. A handoff simply did not carry it, ~24 versions ago.

**THE RULE, FROM NOW ON: an item is only allowed to leave this file by being BUILT or by being
EXPLICITLY CANCELLED BY THE OPERATOR, with the date and the reason written in.** Dropping one by
omission is the failure this file prevents. `BUILD-CHECKLIST.md` must check it every build.

This is the same disease that ate `skylit-docs/FINDINGS.md`, `BUILD-PLAN.md`,
`garma/V2-PHASE-PLAN.md`, the "Mockup hodlod v2" HTML and the "Hodlod stats2" JSON — all referenced
by name in live code comments or config, all absent. **Work that exists only in a resume note is work
that is one handoff away from never having happened.**

---

## OPEN — LOCKED, NOT BUILT

### ITEM 18 · Yahoo Finance HTF/ITF data
**Locked 2026-08-16. Lost 2026-08-20. Recovered 2026-08-27. NOT BUILT.**

Full specification and the recovered verbatim text: **`session-state/YAHOO-PIPELINE.md`**.

Verified absent four ways on 2026-08-27:

| check | result |
|---|---|
| `htfFeed` / `gpts_htf_v1` / `.htf` / `yahoo` / `query1` in either userscript | **zero hits** |
| `"htf"` key in any of the 8 exported day files (08-17 → 08-26) | **none** |
| Yahoo or HTF in `skills/gex/SKILL.md` | **no** |
| `.github/` or any workflow yml across all 171 commits | **none** |

**The one unresolved constraint:** the spec says `@grant GM_xmlhttpRequest`, which **cannot** go in
the main panel — `@grant none` is load-bearing there (the feed hooks patch `window.fetch` and
`XMLHttpRequest` in page context; any grant sandboxes the script and kills the tape). The spec's own
hedge, *"verify unsafeWindow access still OK"*, is that doubt, and the console check it waited on was
never done. **The IF companion already carries `GM_xmlhttpRequest` and already couriers a foreign
site — the ForexFactory calendar at v1.14, writing `gpts_evcal_v1`.** Yahoo is that template with a
different URL.

⚠ **DEADLINE: 2026-09-16.** Yahoo serves intraday bars ≤60 days. The corpus gap starts 2026-07-18;
sixty days from it is 09-16. After that those 20 sessions leave the window permanently.

### ITEM 14 (partial) · premarket high/low never recorded
Item 14 specified per-bar chart levels: VWAP, PDH/PDL/PDC, **PMH/PML**, IB30 H/L, POC/VAH/VAL. All
are present in the current build **except premarket H/L — `PMH`/`PML`/`premarket` return 0 hits.**
The operator's Skylit session-levels setting includes Premarket H/L, so the level is on his chart and
absent from our record. Not confirmed cancelled; treat as open until he says otherwise.

### IB60 · required by the approved HOD/LOD mockup, does not exist
`mockups/hodlod-v2-SPEC.md` shows `IB60` as one of the five confirmation chips.
`sessionLevels()` computes **IB30 only** (`IB_MIN_S=1800` → `ibH`/`ibL`/`ibSet`) plus PDH/PDL/PDC.
Net-new work, and the operator has explicitly asked for a sweep testing IB30 **and** IB60 breaks.

### THE LADDER WIDTH — ⚠ PARTLY CLOSED IN v14.54, ONE DECISION STILL OPEN

**BUILT v14.54:** the ladder was re-laid to `mockups/mockup-ladder-v11.html`, the approved spec.
%King moved inside its own bar (the `LAD_KPCT` column deleted), the roll lane moved left of the
prices, the mirrored profile became the signed dollar delta profile, and the King pills and EM edges
moved into the chute — which is what empties the left gutter the operator photographed.

**⚠ THE 24px THIS FILE COULD NOT EXPLAIN IS EXPLAINED.** It recorded `scrollWidth 656 / clientWidth
632` as a discrepancy. `LAD_ROCW` was **56 for a column that needs 84** — the widest ROC string,
`−100% −100% ▼99%`, is 83px — so the last column overflowed its declared width from v14.46 onward.
`test_ladder` now asserts `LAD_ROC + LAD_ROCW === LAD_W`, so the constant cannot lie again.

**TRUE WIDTH: 657 → 618.** (Not 632 → 588: the old constants understated the original.)

**⚠⚠ STILL OPEN, AND IT IS THE OPERATOR'S CALL: 618 DOES NOT FIT A 454px PANEL BODY.** Measured on
his live panel 2026-08-27. No arrangement of nine columns of 8.4px text fits 454, and the approved
mockup is itself drawn on a **544px** panel — so the design he signed off assumes a wider panel than
he is running. The container SCROLLS rather than clips, so nothing is silently dropped. Three ways
to close it and none may be taken without him:

1. **Widen the panel to ~620.** It is resizable, it is one drag, and it costs nothing. Cheapest.
2. **Abbreviate the STATE words** to the mockup's WEAK/FORM/TURN/DOOR/USED — but the vocabulary was
   SETTLED at v14.49 (BUILDING · HOLDING · TURN UP/DN · WEAKENING · SPENT) and the resume note says
   not to re-litigate it. Would save ~16px and reopen a closed decision for very little.
3. **Drop a column.** The marker and the tests counter are the two candidates, and both are load-
   bearing under the v14.49 three-orthogonal-facts rule. Not recommended without a measurement.

⚠ **DO NOT CLOSE THIS BY DELETING A COLUMN ON YOUR OWN INITIATIVE.** The width is a display
preference; the columns are the level lifecycle.

---

## AWAITING OPERATOR VERIFICATION (not a build item — a question)

- ✅ **CLOSED 2026-08-27 16:41 CT — the v14.52 in-place CSV write RUNS.** Measured on the live
  panel: `inPlace:true`, `how:"file"`, `err:null`, last write 2.9 minutes old on a 180s cadence.
  The atomic-replace diagnosis was correct. ⚠ What is confirmed is that OUR write path executes and
  reports success; whether IRT itself now shows new lines without a refresh is still the operator's
  observation to make. If it does not, the fallback is a local HTTP server in that folder
  (`python -m http.server 8000`, then IRT "Remote File"). GitHub raw is NOT an option — CDN-cached
  ~5 min against a 1-min poll.
- **The FlexLevels CSV had only 3 rows at 16:41** (header + SPXW KING + QQQ KING) where an earlier
  session logged 14 levels. After hours with velocities at 0, so probably expected — but it has not
  been checked during RTH and it is the kind of thinning that passes for normal. Look once at the
  open.

---

## HOW TO ADD TO THIS FILE

One entry per locked item: what was agreed, **when and by whom**, where the full spec lives, what is
verifiably built vs not (with the check that proves it), and any deadline. When it ships, move it to
a `BUILT` section with the version — do not delete it, so a later context can see it was finished
rather than wonder whether it was lost.
