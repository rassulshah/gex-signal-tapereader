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

### THE LADDER IS 656px WIDE INSIDE A 460px PANEL — 196px IS CLIPPED
**MEASURED on the live panel 2026-08-27, and this SUPERSEDES the "five unbuilt mockup items" entry
that stood here earlier today. I was wrong: most of them are built.**

    .g3ladwrap   scrollWidth 656   clientWidth 460   →  196px cut off
    .g3lad       scrollWidth 656   clientWidth 632   →  24px over its own LAD_W

DOM element counts on the same render prove the columns exist and are rendering:

| column | class | elements |
|---|---|---|
| STATE | `g3ldst` | **12** |
| test counters | `g3ldtap` | **6** |
| ROC | `g3ldroc` | **12** |
| levels | `g3ldlv` | 5 |

**So the "missing columns" are a WIDTH problem, not a build gap.** The operator sees a horizontal
scrollbar and a ladder that appears to stop after the delta profile; everything past it is real,
rendered, and off-screen. Any plan that starts by *building* STATE/TAP/ROC is rebuilding what exists.

**What is genuinely still mockup-only** (`mockups/mockup-ladder-v11.html`, rendered at
`mockups/mockup-ladder-v11-RENDERED.png`): the three King pills and `EH`/`EL` sit in the LEFT gutter
beside the level names rather than inside the chute, and `%King` is its own column (`LAD_KPCT=226`)
rather than being left-justified inside the bar.

⚠ **`%King`-on-the-bar IS THE ONE THAT RECLAIMS WIDTH** — it deletes a whole column, ~40px of the
196px that need to go. Moving the pills into the chute fixes the gutter crowding but saves nothing
horizontally. **Neither alone closes a 196px gap**, so the real decision is width strategy:
tighten columns, widen the panel (it is resizable — 656px would show everything), or let the
right-hand block wrap under. **That is an operator decision and it needs a rendered mockup first.**

⚠ **DO NOT MOVE RENDER CODE WITHOUT THE OVERLAP AUDIT.** The chute is the walled column whose entire
purpose is that nothing may overlap price, and `test_ladder` asserts the arithmetic (`offset+width <=
next offset`, the v14.50 lesson — asserting offsets alone proved nothing about what is drawn).
Render headless, run the pairwise `getBoundingClientRect` audit, THEN send.

---

## AWAITING OPERATOR VERIFICATION (not a build item — a question)

- **v14.52 IRT in-place CSV write.** Does IRT show new lines without a manual refresh?
  `__gptsDebug.irt()` reports `IRT_LAST.inPlace`. Fallback if not: a local HTTP server in that
  folder. GitHub raw is NOT an option — CDN-cached ~5 min against a 1-min poll.

---

## HOW TO ADD TO THIS FILE

One entry per locked item: what was agreed, **when and by whom**, where the full spec lives, what is
verifiably built vs not (with the check that proves it), and any deadline. When it ships, move it to
a `BUILT` section with the version — do not delete it, so a later context can see it was finished
rather than wonder whether it was lost.
