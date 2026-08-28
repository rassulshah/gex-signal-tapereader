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

## BUILT 2026-08-28 — moved here rather than deleted, so nobody wonders if they were lost

### ⓪a HOD/LOD — BUILT v14.57, and the whole feature completed v14.60–v14.66
The design (`mockuphodlodv2.html`) was never lost. Shipped: the stats table, the wick family with the
operator's own definitions, trimmed-mean E row, and the LOD/HOD probability table with a three-state
verdict. Evidence in `skylit-docs/FINDINGS.md` F-1 … F-8.

### The wick family — DEFINED BY THE OPERATOR 2026-08-28, BUILT v14.60
BOP/WICK/W.END/WICK%/MUD had been printed as PENDING since v14.57 for want of a definition. He gave
them; they were confirmed bar-by-bar on the tape. **No longer pending.**

---

## OPEN — LOCKED, NOT BUILT

### ⚠ CONDITION THE LOD/HOD TABLE ON THE GAMMA SIDE — the highest-value unbuilt idea
**Raised and agreed 2026-08-28.** The table knows nothing about the panel it lives in. Does price
sitting on the King, or a low printing at a put wall, change the probability that the extreme holds?
**This is what would make the feature belong to THIS tool** rather than being generic price structure
any charting package could compute.
⚠⚠ **BLOCKED by the feature-record collapse** (below) — there is no usable recorded history pairing
node state with session extremes. Unblocking that is the prerequisite, not a side quest.

### ⚠ THE FEATURE-RECORD COLLAPSE — diagnosis, not a feature, and it gates everything
3,822 records on 08-20 against **15** on 08-27. `matrix` rows track exactly (108→3132, 122→3822,
0, 0, 23→990, 2→8, 1→15) against 133 SPY snaps, so it is **ONE upstream gate, not 46 features
failing**. It blocks the gamma conditioning AND the forward test of the LOD/HOD table.

### Overnight / globex context — untested, cheap
The ES/NQ corpora contain ETH bars that every study currently filters out. Does the overnight range
or direction inform where the RTH extreme lands? Genuinely unknown; one study answers it.

### A test that the resume note was updated
`test_chat_history.js` fails the build when the chat history goes stale. **Nothing guards the resume
note** — and on 2026-08-28 it went SEVEN builds stale while every other document was current, until
the operator asked. The same mechanism would close it.



### ITEM 18 · Yahoo Finance HTF/ITF data
**Locked 2026-08-16. Lost 2026-08-20. Recovered 2026-08-27. ⚠ HALF BUILT 2026-08-28 (v14.59).**

⚠⚠ **READ THIS BEFORE CLAIMING ITEM 18 IS DONE. IT IS NOT.** v14.59 built the *route* item 18
specified and used it for ONE purpose — the daily ES bar corpus behind ⓪a HOD/LOD:

| item 18 asked for | state |
|---|---|
| the Yahoo `chart` endpoint reached from the browser | **BUILT** — companion v1.15, `GM_xmlhttpRequest`, `@connect query1.finance.yahoo.com` |
| plain `fetch` first, `GM_xmlhttpRequest` as fallback | **SETTLED** — plain fetch measured **BLOCKED** from page context 2026-08-27; the grant route is the only one |
| a Layer-0 source feeding the day export | **BUILT** — `gpts_futbars_v1` → `futBars` in `data/YYYY-MM-DD.json` |
| **Tier 1: prior week/month H/L/C, 20/50/200 DMA, daily ATR(14), gap vs ATR, position in weekly range** | **NOT BUILT** |
| **Tier 2: 60m/15m 1h/4h trend + swings, 1h ATR** | **NOT BUILT** |
| **`STATE.htf` / `snap.htf` / "nearest chart level" per node; READ citing an HTF level** | **NOT BUILT** — zero hits for `htf` in either script |

**So the pipe and the tap exist and carry 1-minute bars; the HTF/ITF derived reads do not exist.**
Anyone who greps `yahoo` and finds the courier must not conclude item 18 shipped. The remaining work
is a consumer of bars we now already have, which is strictly easier than it was.

⚠ **THE 2026-09-16 DEADLINE STILL STANDS AND IS NOT ADDRESSED.** The daily tap prevents *new* gaps;
it does nothing for the existing **2026-07-18 → 08-14** hole, which is recoverable only at 2-minute
resolution and only until 09-16. That is a separate backfill run, still unbuilt.

### ITEM 18-OLD · the original entry, kept verbatim for the record
**Locked 2026-08-16. Lost 2026-08-20. Recovered 2026-08-27.**

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

### ⓪a HOD/LOD · APPROVED, NOT BUILT, AND BLOCKED ON DATA
**The design is `mockuphodlodv2.html` — in the repo ROOT, on GitHub, renders clean.** Two earlier
notes called it lost; both were wrong (failure pattern #4, not searching the repo root).
`mockups/hodlod-v2-SPEC.md` is the transcription and now carries the correction.

⚠⚠ **BLOCKED ON ONE FILE: `data/es-1min/EPM26-1min.csv.gz`,** 284 complete RTH sessions of ES 1-min.
Committed in the sandbox 2026-08-27 (`a26cdfd`), **never pushed**, sandbox copy gone. Every rate on
the mockup — 84%, n=45, 43/47, med +24, p25 +11, the −12 miss case, the 42/54/66/75/84 ladder — came
from a study over it. They cannot be re-derived without it, and nothing here ships a rate without its
n. **Ask the operator to re-supply the CSV; it is the only blocker.**
⚠ It cannot ride the installer back — 5.2MB gzipped against a 6MB payload cap. He drops it into
`C:\Dev\gex-signal-tapereader\data\es-1min\` and his next push carries it.

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
