# RESUME NOTE — 2026-08-22 — v11.48 / companion v1.8

## ⏱ START HERE — THE FIRST THING TO DO

**v11.48 is built, committed and pushed. The user was on v11.47 when the session ended.**

1. Check what is actually running:
   `__gptsDebug.renderErrors` — if the hook is ABSENT, v11.48 is not loaded yet.
   ⚠ If the panel is popped out, `#gpts-panel` is NOT in the Atlas document — read it from
   `documentPictureInPicture.window.document`.
2. **THE ONE UNVERIFIED THING: the NODES rows in ③ TRADE LOCATION have never been seen rendering.**
   Shipped three times — v11.46 (called a function that was never added), v11.47 (used `rr`, a variable
   from another scope), v11.48 (the fix, unconfirmed). Check `p.querySelectorAll('.g3node').length > 0`
   and `__gptsDebug.renderErrors()` returns `[]`. **Do not call this feature done until rows appear.**
3. Then: nothing is urgent. The user's own words — *"the next useful thing is watching v11.48 during an
   actual session rather than adding more surface to it."* Eight builds shipped on 2026-08-22 and the
   last three were fixing my own misses.

---

## ⚠ SIX FAILURE PATTERNS — THESE EXPLAIN NEARLY EVERY BUG IN THIS PROJECT

**1. Mislabeling.** A value shown under a label implying a different source, window or scale. The ladder
said `IF` and rendered Skylit numbers. `week` said "to Fri" holding one expiration. `DRIFT ✓` sat beside
"both books not in yet", and later beside a call it *disagreed with*. `FLRCEIL_FAR` was documented in
STRIKES and tested in PRICE POINTS. **Nothing ever throws.** Ask of every number: which book, which
window, which scale — and does the label say so.

**2. Moving denominators.** %King ranks at one instant; **DOLLARS** compare two moments. If the King
strike changes, every node rebases at once. Bit three separate features.

**3. Does it POINT or does it CONDITION?** Gamma, vanna and VIX term structure all condition — regime
line or gate, never a direction vote. This survived several rounds because each *feels* directional.

**4. Concluding "absent" from a shallow look.** `pick()` scanned only TOP-LEVEL payload fields, every
published metric read null, and that drove a decision to compute our own zero gamma. Walk the tree first.

**5. Defensive try/catch makes a missing reference INVISIBLE.** Twice in two builds a section rendered
empty because its own catch ate a ReferenceError — once a function never defined (`tradeNodes`), once a
variable from another scope (`rr`). Header emitted, rows gone, looks exactly like "nothing to show".
→ **`swallow(tag,e)` now records it, `__gptsDebug.renderErrors()` exposes it, `node tools/smoke.js`
FAILS on a non-empty list.** A regex scan for undeclared identifiers was tried and ABANDONED — it flagged
keywords and regex-literal contents, and a noisy check gets switched off. Instrument the catch; do not
try to out-parse JavaScript.

**6. When an edit script asserts, CHECK IT LANDED.** v11.46 shipped half a feature because a python edit
aborted on a failed assertion and the failure went unnoticed.

---

## THE TWO RULES THE USER HAS STATED

> **"I am big on 50 sma, everything else is supplementary."**

② BIAS is **not a tally**. The 50-SMA gives direction and explains itself; SKEW / ACCUM / PA **confirm or
they do not**, and the count is the confidence. Confirmers never outvote the SMA; three agreeing cannot
manufacture a direction when the SMA is flat. Both pinned by tests. DRIFT gates — and it gates **the
call**, not itself: books agreeing with each other *against* the SMA is ✗, not ✓.

> **"any time a trade occurs, it must be off a node. The levels give context but the trade is off a
> node, preferably a pullback node."**

⑤ EXECUTE refuses with `NO NODE — NO TRADE` and names the level as context. Stop sits beyond the NODE'S
OWN zone; target is the next structural stop; a node setting up against the SMA is `AGAINST THE CALL`.
③ shows nodes as rows beside the levels, tagged `@CR`/`@PS` where a node sits AT a level (the strongest
combination), `▸` for the one the pullback engine selected.

---

## WHAT THE PANEL IS NOW

**① FRAME** — `−G −V ⚠ → 7617` (regime AND target on one line) then `DEX · EM · TERM · ATR · phase`.
**② BIAS** — SMA verdict + confirmation chips + count; DRIFT gate.
**③ TRADE LOCATION** — IF ladder with depth diamonds ◆◆/◆, then NODES rows, then the three-zone chart.
**④ REACTION** — WATCH (the node) · NODE (defended or abandoned) · PRICE · DEPTH · PRESSURE.
**⑤ EXECUTE** — armed on a node, or a refusal that says why. Empty EXECUTE is a single dash.

**Deleted for space and kept deleted:** the "waiting on" line, the "NO SETUP" box, the phase box and
progress bar, the chart legend row, PAIN, P/C, CAGE. **Every hover opens with the QUESTION it answers**
— a test asserts this for all five step headers and pins ~20 more cells by name.

**The chart, three zones:** LEFT = InsiderFinance STRUCTURE (net GEX column, net DEX column — BOTH from
their chain). RIGHT = Skylit FLOW, bars growing INWARD from the right edge as growth segments (dim =
held over an hour, mid = added 60m→15m ago, **bright leading edge = last 15 minutes**), with lost ground
marked when a node bleeds. MIDDLE = price, candles, node markers, roll arrows, the 50-SMA, centred level
labels, price and time axes.

**Ladder = InsiderFinance, SPX, NO fallback.** Basis ~1.0023 live from THEIR spot vs the futures print.
⚠ **NEVER restate an SPX strike as a SPY strike** — 5-point vs 1-point grids, separate chains.
⚠ **THREE SCALES per row**: `k` (their SPX strike), `disp` (chart/ES), `und` (underlying/SPY, **for the
candle reads** — `closedCandles` divides futures candles back down). ATR, zone width and the rejection
detector all run on `und`.
⚠ `dispNum()` formats values ALREADY on chart scale — never pass them to `fmtLvl`/`fmtSpan`.
(`ifNum` is the unrelated manual-entry PARSER. Do not confuse them again — that collision shipped broken
for nine releases.)

**Verified against IF:** our SPX CR0 7700 / PS0 7665 reproduced their published 0DTE Call Wall 7700 /
Put Wall 7665 exactly.

---

## THEIR PAYLOAD — SETTLED, DO NOT RE-DERIVE

`__gptsDebug.ifShape()` returned the WHOLE of `initialData`:

    ticker · tickerDetails{...} · spot · options[] · timestamp · isStale

**No zeroGamma, no walls, no skew — nothing computed at all.** Their entire page renders client-side
from `options[]` and `spot`. So computing is not a shortcut around something they publish; it is the
only route. Each contract carries:

    strike, expireYear, expireMonth, expireDay, cp, gamma, delta, openInterest, impliedVol, bid, ask

**What we compute from it:** DEX (⚠ **puts already carry NEGATIVE delta — do not flip the sign**),
25Δ SKEW (put IV − call IV, nearest-delta match, refused when nothing is within 0.08 of 25 delta, voted
against its OWN recent range because index skew is permanently put-heavy), expected move (ATM straddle,
refused unless both legs quote at ONE strike within 1% of spot), gamma flip (FALLBACK ONLY → `HVL*`
tagged `calc`), level depth (GEX ∩ DEX, ◆◆/◆).
⚠ **Level depth aggregates across a ZONE (max, not sum)** — the two books sit on different strike grids,
and v11.42 scored IF levels against SKYLIT gamma, making every level read 0.02–0.20. **Compare a level
to the book it came from.**
**Charm was REMOVED** — computed, never displayed, and no decision changed by a `CHEX −$1.2B/day` cell.

---

## 📋 TABLED — THE BACKLOG, NOTHING STARTED

- **VIX/price divergence** in ④ — price at new highs while VIX ticks up = exhaustion. Research says VIX
  in every form FAILS the direction test (term structure is a regime filter, VIX change is tautological,
  divergence is a warning). VIX is already captured per bar in `xm`.
- **Daily DEX snapshot** — positioning change at a FIXED spot across days is a real signal; intraday
  ΔDEX is mostly greeks re-weighting. Cheap to start, useless without history, **so start it early.**
- **Term slope** — `TERM —` is permanently blank; their published value is null. Near-dated ATM IV
  against longer-dated is the same shape of calculation as the skew we already compute.
- **Prior-day levels + opening range** — PDH/PDC/PDL are on the user's Atlas chart and absent from ours.
- **Session-history reads** from the 390 snapshots — prior reaction at a level, node lifecycle.
- **NDX for NQ** — same argument that moved SPY→SPX.
- **Value area / POC / VAH / VAL** — canvas-painted (9 canvases, 0 SVG text). Only route is the React
  **fiber walk** `readFiberCandles` already uses. The one truly independent data source we lack.
- **Per-strike dollars in the DAILY EXPORT** — prerequisite for multi-day roll validation.
- **Debug hook for an expSet's per-strike rows** — `callPutRows` reads LASTFEED only.
- **PiP background throttling** — never verified with the feed running.
- **Coiling · read-level invalidation · map-reshuffle · regime-split scorecard · PATH waypoints ·
  entry reachability (EM exists now) · test counts · voids.**
- **Weekly review file naming** (`review_<FRIDAY>.json`), trigger `trig_01T8kd4kS3nBR7TuQMSSXNX6` — overdue.
- **Ledger `infl` counters not per node.**

## ROLL DETECTION — built, SHADOW MODE, NEVER SEEN FIRING
`rollDetect` reads `LASTFEED[sym].j.levels` — ~390 snapshots of `{k,v,net}` in DOLLARS at ~1min. **`t` is
in SECONDS.** Window walked by TIMESTAMP, not index.
**Calibrated on real data: 30m · 50% RELATIVE TO THE SESSION MEDIAN · $40M · ±5 strikes** → 9
events/session. 35% gave 37 (noise), 70% gave 2. The median near-money strike **grows 10–15% per 30m**;
on expiry day strikes lost **75–94%** to decay alone — without the median subtraction it screams at both
ends of every session.
⚠ **Direction unproven — do NOT make it a vote.** Ceiling-down 4 events/1 correct/mean **+0.39**;
floor-up 4/1/**−0.22** — both OPPOSITE to doctrine. The archive cannot settle it: exports keep only ~5
nodes/bar with `abs` often null and `hist` in %King, and **all four archived days are DOWN days** (66%
base rate). The existing per-node `roll` flag **never once fires as ceiling-rolled-down** in four sessions.
**It has never been watched working** — it correctly refuses on a weekend where everything decays
together. Needs a live session, not a fix.

---

## BUILD/TEST MECHANICS

**`tools/BUILD-CHECKLIST.md` runs on EVERY build. The user should never have to say "save".**
A build is finished when a fresh context could pick it up — not when the code works.

- Repo **`/tmp/gexwork`**. ⚠ **THE SANDBOX CONTAINER RESET TWICE ON 2026-08-22** and reverted to v11.25
  both times. Nothing was lost only because everything had been pushed. Recover with
  `git clone https://github.com/rassulshah/gex-signal-tapereader.git`. **The installer's push is the
  only durable copy of the work.**
- **Cannot push from the sandbox** — the git proxy refuses credentials for this repo. The .bat pushes
  from the user's machine.
- Harness reads **`./v10.js`** — `cp current/gex-signal-tapereader.user.js v10.js` FIRST.
- **Suite 4239 pass / 0 fail. KEEP IT GREEN.** 23 "known stale" failures once camouflaged two live bugs
  for months (the locked voice line lost the word "target"; the `ifNum` collision). When a deliberate
  change breaks a test, fix the test IN THE SAME COMMIT or it becomes camouflage.
- **`node tools/smoke.js`** — loads the script in a DOM stub, calls every debug hook, and **fails on
  anything a render catch swallowed.**
- **`test_no_dupes.js`** fails the build on any NEW function-name collision. Four to date:
  `trendBadgeHtml`, `nodeBreadth` (both inert), `ifNum` (live-broken nine releases), `confluence`
  (caught pre-ship, renamed `levelDepth`). **GREP BEFORE NAMING A FUNCTION.**
- **Version pins** in `test_direction_grade.js`, `test_pipeline_indicator.js`, `test_rules_v2.js`,
  `test_read_v1047.js`. Bump every release, and bump `GPTS_VERSION`.
- Installer: header + base64; `more +<HDRLINES>` must equal the `exit /b 0` line. **NO POWERSHELL**
  (Avast IDP.HELU.PSE88) — `certutil` + `tar`.
- **`eval()` inside a forEach callback declares into the CALLBACK's scope.** Join and eval once.

### THREE INSTALL FAILURES, IDENTICAL FROM THE USER'S SIDE
| symptom | cause | one-call check |
|---|---|---|
| GitHub serves the old version | the installer did not push | clone and read `@version` |
| GitHub new, browser old | **the page was already open — installing does not affect it. RELOAD.** | reload the tab |
| raw stale ~5 min | CDN cache, `max-age=300` | read `cache-control`/`age` |

⚠ **CHECK THE RAW URL YOURSELF BEFORE TELLING THE USER TO CLICK.** This cost four cycles on 2026-08-22.
A cachebusting query string does NOT reliably defeat it.

## DEBUG SURFACE
`__gptsDebug.` — `renderErrors` `ifShape` `optKeys` `skew` `accum` `rolls` `ifLadder` `nodeChart` `phase`
`regime2` `pa` `bias` `steps` `roll` `face` `expSets` `unified` `sanity` `ifChain` `callPutRows`
`feedShape` `pbEntry`

## STANDING INSTRUCTIONS
- **Ask before giving code** — the user may have more requests. Exception: unambiguous bug reports.
- **ALWAYS send the Tampermonkey link with every build** — and ONLY for the script that actually
  changed. Sending both when one is unchanged manufactures a "Reinstall" that looks like a failure.
  (This was forgotten repeatedly; it is in the checklist as step 9 and still got missed.)
- **Do not over-build.** *"The requirement is just to display the levels from IF. why are you making
  things more complicated"*.
- **Use their values when they publish them.** *"stop writing your own stuff when it is already there."*
- **No useless text — there is no space.** If it is needed it goes in a hover.
- **Verify against real data before proposing thresholds.** It changed the answer twice.
- **Flag substitutions.** Node bands shipped where a gamma profile was asked for and it was never said
  out loud. If the design says X and X is not built, say so at the time.
- **ONE AT A TIME** when discussing open items — state one, ask, stop.
