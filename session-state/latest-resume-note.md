# Resume note — end of the v13.x session

## Where the build is
**v13.3 shipped and confirmed running live** (panel reports v13.3, harvest timer running,
327 strikes captured, `velMeta.ok:true`).

## The big change this session: SKYLIT VELOCITY CAPTURE
Skylit hands every RENDERED ladder row a `velocity` object on its React fiber:
`{strike, currentValue, delta1Min, delta5Min, delta10Min, delta15Min, delta1Hour, delta4Hour,
delta1Day, percent*, trend}`. Clicking a strike fires **no network request** — it is all already in
the client. `velHarvest()` walks `__reactFiber$` and captures it verbatim.

Confirmed live sample (strike 7650): cur $18,619,834 · 5m +68,245 · 15m −1,237,568 · 60m −345,508 ·
Day +11,750,083.

**This makes Skylit's own UI a test oracle**: click a strike in their ladder and the popup must match
our columns to the dollar. Any disagreement is a real bug.

## Source-of-truth policy (user-directed, now in PROJECT-CONSTANTS)
1. Vendor numbers captured verbatim — Skylit for FLOW, InsiderFinance for STRUCTURE.
2. Derive only what they do not publish.
3. Derived values are tagged and marked on the face.
4. The recorder stores vendor raw AND our derived read — never derived alone.
   `snap.vend` (their numbers) + `snap.srcs` (provenance map) added in v13.1.
   The audit finding: we were recording conclusions without inputs — training on our own opinions.

## STILL TO VERIFY AT THE OPEN (nothing below has been seen with a live band)
- Every node on the rail appears in NODES, same colour, same order.
- Our 5m/15m/60m/Day match Skylit's strike popup exactly.
- Roll detection fires at a sane rate ($40K over 15m, ≤25 SPX pts, ratio ≥0.40).
- TURNING chip appears and is not noise.
- REACTION shows DEFENDING/ABANDONING with the hour caveat.
- The verdict line reads sensibly against the tape.

## Known limitation, by design
NODES reads `emPiles()` — the rail's own array. With no expected-move anchor (market closed) BOTH are
empty. Before v13.1 the list came from the tape and would show rows regardless. This is the cost of
the single-source rule; watch it at the open and decide if it is right.

## NOT built (deliberately deferred)
- **Roll ARROWS on the rail.** Needs a vertical re-layout of the rail's tiers, which is exactly where
  overlap regressions have come from. Rolls are visible in NODES meanwhile.
- **The event causality system.** Still the biggest outstanding request.

## Lessons recorded this session (PROJECT-CONSTANTS)
- Chrome CAPS Document PiP height; `resizeTo` needs user activation, CONSUMES it, and reverts past the
  cap. Six versions built a handle for an operation the platform forbids.
- A second computation of the same thing always drifts (rail vs NODES colour + membership).
- An assumption written in the voice of a measurement — THREE occurrences now.
- Removal-comment-contains-removed-string — SEVEN occurrences; `noc()` strips comments in tests.
- Writing the right requirement in a comment and wiring the call to the wrong place (v13.1 harvest
  inside `if(haveFeed)`).

## Standing user rules
- Never build without asking. Show mockups first, always.
- Always paste the `release-links.sh` output — run the .bat FIRST, click the link a few minutes later
  (raw.githubusercontent caches ~5 min; clicking early gives "Reinstall" instead of "Update").
- Deliver `install-v<VER>.bat`, never a bare install.bat.
- Terse. One thing at a time.
