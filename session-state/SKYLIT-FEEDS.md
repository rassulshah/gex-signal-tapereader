# SKYLIT FEEDS — everything measured about the API, the books, and the DERIVED layer

Written 2026-08-26 after the operator's directive: "make sure you are writing down everything you
learn, like about derived levels, somewhere so the next context knows." Every claim below was
MEASURED on the live page, not inferred. Where a claim has a date, that is the day it was verified.

## The endpoint

    https://app.skylit.ai/tv/api/gex/levels
      ?symbol=SPY|QQQ|SPXW|...        one request per symbol
      &data_type=gamma|vanna          GEX book or VEX book (two separate requests)
      &nodes=500
      &exp_mode=week&exp_count=1      ⚠ the WINDOW — follows the Atlas chart's expiration selector
      &extended=false
      &include_derived=true           always sent by the page; brings the derived[] array along
      &dates=YYYY-MM-DD               a week-anchor date, NOT "today" (seen: 2026-08-19 on 08-26)
      &v=<timestamp>

- **Auth: cookies alone 401** (verified 2026-08-26 by direct fetch from page context). The app sends
  an Authorization header; the panel's self-fetch works only after capturing that header from a real
  request. Any future direct probing must reuse the captured header, or read what the page stored.
- Cadence: the page refreshes each (symbol × data_type) roughly every 5s per pane; the panel
  self-fetches SPY/QQQ gamma+vanna when stale (FEED_STALE_MS=12000).
- ⚠ **THE PANEL'S OWN SPY / QQQ BOOK IS ONE WINDOW AND ONE BREADTH (v15.84, F-22):** the self-fetch pins
  `exp_mode=current&exp_count=1&nodes=500` — the 0DTE chain, every strike — whatever URL came last (the app's
  three projection windows on an ES chart, or the panel's own expiry sets, whose responses come back through
  the same hook), and `onFeed` drops a multi-expiration gamma payload once a single-expiration book is held
  (`__gptsDebug.feedRejects().SPY.win`). Before v15.84 the book inherited window AND breadth from the last URL
  seen — the recorded node count per bar flipped 34–61 times a session since 08-26 (FINDINGS F-22).

## Payload shape (top level)

    { success, levels:[snapshots], level_count, expirations, derived:[...], strike_interval }

Each `levels` snapshot: `{ t, s (spot), l:[rows] }`, rows `{ k, v, d, net }`.

- **|net| ≡ v on every strike** (measured 2026-08-25 via callPutProbe, all 200+ strikes): `net` is
  just the signed magnitude. **A call/put split does NOT exist in this feed**, so a Call Wall is NOT
  computable from Skylit — InsiderFinance's chain is the only call/put source. Sign of net carries
  polarity (brake/accelerator) only.
- Snapshots are a time series; the LAST one is the live book. Historical/replay payloads can arrive
  and must never overwrite the live one (the observer's newest-timestamp guard, v11.55 era).

## THE DERIVED LAYER (the diamonds) — verified 2026-08-26

`derived` is an array of **sibling-book projections onto the requesting symbol's chart**:

    derived[i] = { source:'SPXW'|'SPY'|..., ratio, ratio_date, levels:[snapshots like the main book] }

Measured facts:
1. **On the SPY feed, derived[0].source === 'SPXW'** (86 rows, ratio 0.09977) — the SPXW book
   projected onto the SPY chart. By symmetry the SPXW feed's derived[] carries the sibling books
   (SPY at least) projected onto SPXW — that is what Atlas draws as the GREY DIAMONDS when the
   toolbar's **Derived** toggle is on. ⚠ The diamonds are NOT VEX — first misread as vanna until
   the operator toggled GEX-only and they stayed; VEX renders only in the VEX/GEX+VEX modes.
2. **Derived rows arrive PRE-CONVERTED to the HOST scale.** On the SPY feed the top row was
   k=768.7677 = SPXW 7705 × 0.09977 — already SPY-scale. **`ratio` is INFORMATIONAL** (host-per-
   source), not an instruction to multiply. v14.10 multiplied anyway and survived only via a decade
   sanity fallback; v14.11 made raw-k the primary path with ratio-multiply as the fallback.
3. **Normalisation:** derived rows carry raw gamma dollars (v). % figures must be computed against
   the payload's OWN largest strike — which equals that source book's %King (its King is always the
   payload max). ⚠ NEVER comparable to the native book's %King (the v11.4.3 lesson — two books, two
   rulers). Export labels carry the source for exactly this reason: `D-SPY 43%`.
4. **The derived window follows the PAGE's expiration selector** (exp_mode/exp_count), not 0DTE.
   Measured 2026-08-26: derived-of-SPXW said 7705=100% while the live 0DTE ladder said 19% — both
   correct, different windows (week vs today). Anyone comparing derived %s to the 0DTE tape will
   see "wrong" numbers that are actually a window difference. Label and remember; never "fix" one
   book to match the other.
5. The panel's observer historically DROPPED all non-SPY/QQQ payloads (SYM_SEEN counts them). Since
   v14.10 it keeps the SPXW gamma feed in `LASTSPXW = {j, ts}` — SOLELY for derived[]; the
   read/record pipeline never consumes it. Freshness gate: ts within FEED_STALE_MS×3; stale ⇒ the
   export writes NO diamonds (absent, never old).

## THE ES1 BOOK AND THE PROJECTION FEATURE — measured live 2026-09-08 (operator turned PROJECTION on)

**ES1 has a gex/levels feed of its own, and it is ALL derived.** The Atlas page on ES1 requests
`/tv/api/gex/levels?symbol=ES1&data_type=gamma…&include_derived=true&with_snapshot=true&with_slices=true`
in three windows (`nodes=5&exp_mode=current&exp_count=1` · `nodes=5&exp_mode=next_n&exp_count=2` ·
`nodes=60&exp_mode=next_n&exp_count=4`). Measured at 10:0x–10:28 CT:

- `levels:[]`, `level_count:0` — ES has no option book here; `derived[]` carries THREE sources, each with
  its own `ratio` (host-per-source) and a 98–116-snapshot time series: **SPY** (ratio 10.0265 → 10.02604
  within 25 min), **SPXW** (1.000602 → 1.000578) and **SPX** (the monthly, same ratio). `k` arrives
  PRE-CONVERTED to ES (SPY 770 → 7720.41; SPXW 7700 → 7704.64; SPX 7750 → 7754.67), `v` raw dollars of the
  source book — the SPY rows agreed with our own SPY feed strike for strike within a minute's drift
  (770: 127.7M vs 133.7M sixty seconds later). This is exactly what Atlas draws as the DERIVED orbs on his
  ES chart — the ratio is Skylit's live one and it moves every minute (docs: "the wiggle").
- `snapshot:{ t, s (ES spot), pc (previous close, 7715), slices:[{exp, l:[rows]}] }` — one merged node
  list PER EXPIRATION (09-08 · 09-09 · 09-10 · 09-11 · 09-14 for next_n 4), all sources together at their
  ES prices, 60 rows each. The app's parser names them `{spot, previous_close, timestamp, slices:[{expiration,
  levels}]}` and merges slices by expiration across payloads.
- The panel's feed observer SEES these payloads (`symbolsSeen().ES1.n` = 195 by 10:20 CT) and drops them as
  non-SPY/QQQ. Nothing reads them yet.

**THE MERGED SLICE, RESOLVED (v15.90 / v15.91, measured 2026-09-09 on his charts).** `snapshot.slices[exp].l[]` rows carry
`k · v · d · net` and no source; `v` is each book's OWN-KING % (two 100% rows on FRONT: one per book), so Atlas's labels on
the futures chart are per-book percentages, not one ruler. A row's book is the one whose `derived[].ratio` turns `k` back
into a strike: **ES1** — 7665.24 / 1.00071 = 7660 (SPXW), 7649.94 / 10.0256 = 763 (SPY), 7655.23 → 7650, 7639.92 → 762,
7620.21 → 7615; **NQ1** — three books, QQQ (41.11874) and NDXP + NDX (one ratio, 1.00087): 29355.41 → NDX 29330 (the NDX
book's King — "the derived King on the nq"), 29523.25 → QQQ 718, 29445.49 → NDX 29420, 29441.02 → QQQ 716, 29495.53 → NDX
29470. On FRONT the ES slice IS the two books' top rows (verified 5 for 5 against the chart's labels); the morning's
"7658 — 100%" with "7650 — 58%" was a single-ruler list from a different window (the chart was not on FRONT then). The
panel's `futDerFrontList` does this resolution; `__gptsDebug.futDerRows(sym)` prints it.

**(v15.92) A BOOK'S OWN KING FROM ITS derived[] ENTRY.** `futDerBookKing('NQ1', ['NDXP','NDX'])` reads the NDXP book's own row
list (`derived[i].levels[last].l[]`, top-N by the app's `nodes=` — 5 on his chart) and takes the largest |v| row as the book's
King: its `k` is already the NQ price by Skylit's ratio (29355.41 → strike 29330 = k / 1.00087 on the 5-grid), its polarity
`net` / `d`. NDXP (the weeklies — the SPXW analog) before NDX (the monthly): both carry one ratio, and the front window's
0DTE rows live in NDXP. That is the file's **NDX KING**; the merged slice is no longer needed for it. The self-fetched NQ1
payload (exp_mode=current, exp_count=1) is FRONT by construction; the app's own NQ1 request follows his selector.

**PROJECTION (beta) = "Forward-projected GEX zones past the last candle"** (its own toggle, sub-panel
"Projection & Label Opacity", horizon pill "3 Days" on his 3-minute chart — `projectionHorizon:"auto"`
follows the chart timeframe; "the projection always uses rolling expiration buckets"). The algorithm, read
from the app's chunk 988 (`function h({slices, lastCandleTime, intervalMinutes, displayColumns,
maxProjectedNodes=3, extendedHours, nowUnix, projectionHorizon="auto", basketExpirationMode="next_n",
basketExpirationCount})`):

1. Expirations resolve to their close time; only those past the last candle count. Columns step forward
   from the last candle to the horizon (or to the last expiry / the session cap), N minutes per step.
2. At each column time `r`: the alive expirations of the basket (the next N, `next_n`) are MERGED per strike
   across every source: contribution = `|value| × dissolve × 1/√(max(daysToExpiry, 0.5))`, where dissolve =
   1 until an expiry is within two steps of its close, then fades to 0 (`min(1, steps/2)`). So the nearest
   expiry dominates: 0DTE ×1.41, tomorrow ×0.91, +2 days ×0.67, +3 days ×0.56.
3. Per column: `targetPrice` = the |value|-weighted mean strike (the CENTRE PATH / TARGET DOT);
   `stdDev` = √(Σ|v|(k−target)²/Σ|v|) × (0.3 + 0.7×(1−conflict)) (the CONE / BANDS); `conflictRatio` =
   min(+mass, −mass)/max (sign = the strongest contributor's); `regime` = "unstable" if any alive expiry is
   within two steps, else "compressed" (conflict ≥ .7 and stdDev/target ≤ 1.5%), "balanced" (conflict ≥ .4)
   or "directional".
4. `projectedNodes` = the top `maxProjectedNodes` (3) merged strikes, each with **strength = |merged| / the
   column's largest** — THAT is the percentage in the label ("7721 — 100%"): the column's king reads 100%,
   the same reading as the node labels ("size relative to the king"; help text: "The projection region draws
   its projected nodes the same way"). Also `concentration` (share of the column's total), `persistence`
   (how many of the alive expirations carry the strike) and `confidence` = persistence × a regime factor
   (.95 compressed / .8 balanced / .7 directional / .55 unstable). Labels are drawn once, from the
   rightmost visible bar; strike labels are the ES price rounded to a whole point.

Verified against the drawn labels at 10:0x CT — 7721 — 100% (SPY 770), 7705 — 86% (SPXW 7700), 7710 — 82%
(SPXW 7705), 7691 — 72% (SPY 767): recomputing step 2 from the slices gives 100 / 86 / 81 / 76 — the SPY 767
gap is the label's older snapshot. ⚠ The projection's % is NOT the 0DTE %King: SPXW 7705 read 94% on the
0DTE tape and 82% in the projection because SPY 770 carries more week-gamma behind it. Two rulers again —
label which one you are quoting.

**THE LEVELS COMPARISON he asked for (10:25 CT, same second, both read from his tab).** Skylit's ES1-derived
0DTE rows (what Atlas draws) vs the panel's IRT export: SPXW 7700 King 7704.45 vs 7705.00 · 7705 (G2) 7709.46
vs 7710.00 · 7680 (G3) 7684.44 vs 7685.00 · 7695 (G4) 7699.45 vs 7700.00 · 7675 (G5) 7679.44 vs 7680.00 ·
SPY 770 King 7720.05 vs 7720.50. Same five SPX strikes, same order. Twenty-five minutes earlier the export
had the SPXW rows 1.4 pt LOWER than Skylit (7703.25 vs 7704.64): the panel's SPX→ES basis is `ifLadder.
dispScale` — the IF companion's SPX spot against ES, refreshed on its 3-minute cadence (+3.25 at 10:0x,
+5.0 at 10:25) — while Skylit's ratio is its own SPX spot each minute (+4.64 → +4.45). Not a computing
error; two spots on two clocks, and our lines wander up to ~1.5 pt around his chart's orbs. The fix that
"matches Skylit always": read the ES prices straight from the ES1 derived rows (the observer already
receives them) instead of converting with a basis of our own. Proposed 2026-09-08, not built — his call.

## The velocity objects (a different capture path entirely)

Every RENDERED ladder row carries a `velocity` object on its React fiber:
`{strike, currentValue, delta1Min/5/10/15Min, delta1Hour, delta4Hour, delta1Day, percent*, trend,
exp}` — captured verbatim by velHarvest() into VEL. ⚠ VEL holds EVERY ladder the user ever opened
(SPY ~765, QQQ ~711, far expiries — measured: 225 of 315 keys off-book on 2026-08-25). Every
consumer must filter by `exp` (today / the rail's reference expiry) or the half-of-max decade rule.
Clicking a strike fires NO network request — the popup is client-side; their UI is a free test
oracle for our numbers.

## Consumers map (who reads what)

- Rail / profile / NODES / rolls / peaks / export node rows → VEL (SPXW, expiry-filtered).
- Drift's vanna band → LASTVEX (self-fetched; magnitudes ~10× gamma, normalised within-feed).
- FlexLevels export → VEL-derived rail nodes + IF dte0 chain + LASTSPXW.derived (v14.11 semantics).
- Recording (`snap.vend`) → VEL, today's expiry only since v14.2.
- NOTHING consumes LASTSPXW except the export. Keep it that way unless deliberately changed.


## CHART SETTINGS MAP — explored live 2026-08-26 (observe-only; nothing was changed)

### Main Settings dialog (gear, top-right)
- SESSION: **RTH** | ETH · TZ: CT — scopes the tape's session. RTH is what every panel calibration
  (bar clocks, after-hours gating, day peaks) was measured under. ETH would change "today's book".
- LAYOUT: Default ▾ · Single ▾ · INTERVAL: 1m **3m** 5m 15m 30m 1H 2H 4H 1D 1W
- **OVERLAY** (master toggle ON): modes **GEX | VEX | GEX+VEX | Derived | Orbs | Orbs V2** —
  GEX+VEX and Derived and Orbs V2 can be active together. Derived = the grey diamonds (sibling
  books); Orbs V2 = the right-edge %King sidebar column.
  - **EXPIRATIONS**: **Front**✓, Week, 2 Weeks, Month, 2 Months, Quarter, 2 Quarters, All, 2, 3, 5.
    Front = 0DTE — matches the measured ladder (all rendered strikes expire today). ⚠ Yet the
    gex/levels REQUESTS carry exp_mode=week&exp_count=1 with a week-anchor date, and the derived
    payload behaves as the WEEK window — so the UI selector and the request params are NOT the same
    knob; do not assume one implies the other. Facts only: ladder rows = today; derived %s = weekly.
  - **NODES**: 1/3/5/10/15/20 (top-N) or **P15/P20✓/P25/P30/P40** (percent-of-King floors).
    P20 = draw overlay nodes ≥20%K — the same default as our CFG.nodeThresh. Governs what the CHART
    overlay draws; the LADDER pane still renders the full strip (harvest saw 100 strikes at P20).
  - COLOR: Mono ▾
- Feature toggles (state on 2026-08-26): HEATMAP on · TRINITY on · CROSSHAIR on · SMA 50 on ·
  SESSION LEVELS on · VOLUME PROFILE on · off: PROJECTION, WATCHLIST, ALERTS, TRADE DECK, GAP FILL,
  STD LEGS, TDO GAP FILL, GEX VWAP, VOLUME-SELL, CVD-SESSION, VWAP, FLOW-SINGLE, VOLUME-BUY.
  Each has its own slider sub-panel (not all explored — display/indicator configs).
- CANDLES: Classic ▾ · ADD button (add indicator).

### Overlay sub-panel (gear beside Orbs V2; captured from the operator's screenshot)
- **READ AS: Value | %King** (%King✓) — ⚠ the DOM tape reader parses rendered percentages;
  switching to Value could confuse tapeMap. Fibers are unaffected either way.
- SIGN ± | Abs · DECIMALS 0-3 · TEXT SIZE · BOLD
- **LOW NODES: Hide | Dim | Fade** — ⚠⚠ NEVER Hide: an unrendered row cannot be harvested; grey
  minors, day peaks and the small-node deflection stats all depend on low nodes existing in the DOM.
- PALETTE (Viridis colourblind-safe + alternatives) — display only.
- **VELOCITY: All | Selected** — ⚠⚠ MUST stay All: velocity objects exist only on rendered rows the
  feature covers; "Selected" would blind the harvest on unselected strikes.
- RAW HOVER: On — the strike card shows raw values; the free test oracle, keep On.
- **HALO · MUST AGREE** with window chips 1m/5m/10m/15m/1h/4h/1d — Skylit's NATIVE multi-window
  agreement signal (halo only when the chosen windows agree on direction): their in-house version
  of our defended/abandoned arbiter. Potentially harvestable as vendor-verbatim confirmation later.
- DOCK position controls.

### DARK POOL — a live Atlas indicator, and its endpoint (discovered 2026-08-26)
The controls bar carries a **Dark Pool** toggle, ON in the operator's live config. It is a chart
indicator like SMA 50 / Session Levels, and it is fed by its own same-origin API:

    GET /fs/api/dark-pool/top-prints?ticker=SPY&top_n=3&lookback_days=45

- **Skylit's own definition of a dark-pool LEVEL is therefore vendor-verbatim: the TOP N PRINTS
  over a 45-DAY LOOKBACK** (their default: top 3). Not an intraday tape — a standing 45-day level
  set. This is the number to reproduce; do not invent our own clustering.
- **ticker=SPY, and that is not an accident**: dark-pool prints are CASH-EQUITY prints. SPX is an
  index and has none, so SPY (and QQQ) are the only books that can carry them — they reach the ES
  rail through the scale conversion we already run, exactly like the SPY King flag.
- **Auth**: `Authorization` header, the same scheme as `/tv/api/gex/levels`. A cold re-fetch 401s
  ("Provide a valid API key"); the page attaches the header to its own request.
- **HARVEST PATH — passive, one line.** `installFeedObserver()` filters on
  `url.indexOf('gex/levels')`; adding `dark-pool/top-prints` captures the response the page already
  fetches. NO companion courier, NO CSP problem, NO credential handling — the same posture as every
  other feed. The request fires on chart mount / symbol change (not on a timer), so a reload
  produces one.
- Related but DIFFERENT: the `/flow` **DARK FEED** page is the raw 500-print intraday tape. The
  Atlas indicator above is the distilled level set and is what Garma's videos reference.

### The heat panel behind the dialog (fullscreen ladder view)
Renders **FOUR books side by side: SPXW strikes, SPY, QQQ, VIX** — each with its own King header —
plus the Orbs V2 %King sidebar and a bottom heat-grid strip. ⚠ THIS is why VEL held SPY (~765) and
QQQ (~709) and VIX rows beside SPXW: everything rendered is harvested; every consumer must filter
(the v14.2 expiry/decade rules).

### The strike card (hover, RAW HOVER on)
`Strike 7695 · 2026-08-26 · NEUTRAL | CURRENT VALUE −159.6K · Exposure Decreasing |
VALUE OVER TIME sparkline (≈10m of client-held history) | RATE OF CHANGE 1m/5m/10m/15m ($ and %) |
EXTENDED 1h/4h/1d | 1m Velocity`. Confirms: per-strike history exists CLIENT-SIDE beyond the
velocity deltas — a possible richer harvest target if ever needed.

### REQUIRED POSTURE for the panel's capture (now FIVE rules)
**RTH · READ AS %King · VELOCITY All · LOW NODES Dim or Fade (never Hide) · OVERLAY = GEX.**
Session ETH, Value display, Selected velocity, or Hidden low nodes each degrade or break a measured
pipeline.

⚠⚠ **OVERLAY = GEX IS NEW, MEASURED 2026-08-28, AND IT WAS NEVER WRITTEN DOWN.** The overlay mode
changes what the LADDER shows, not just the chart. Same instant, same strikes:

    GEX+VEX   King 7715  $K 57,714   7750 = 57%   7755 = 32%
    GEX only  King 7715  $K 60,552   7750 = 68%   7755 = 42%

The King STRIKE survived; its dollar value and every %King did not. An 11-point difference on a node
you would size against.

**What already defended against it, and what did not:**
- `tapeMapLive()` checks `LASTDISP[sym]` and, when the display is not pure gamma, reads structure
  from the **gamma feed** instead of the DOM. So the rail, the piles and the SPXW King stayed on
  clean gamma. ✅
- The **QQQ King in the IRT export** is barred from feed sourcing by the v14.15 decision (Atlas is
  the source of truth for QQQ), so in GEX+VEX it has NO source and the row silently vanishes from
  `FlexLevelsExport.csv` — and IRT, polling correctly, deletes the level. ❌ That is the whole
  symptom the operator reported as "I don't see the QQQ levels on NQ".
- ⚠ `__gptsDebug.tape(sym)` calls `readTapeFromDOM` DIRECTLY and bypasses that guard, so it reports
  the blended numbers. **A debug hook that skips the production path is not a measurement of the
  production path** — this cost one wrong alarm before it was caught.

⚠ **VENDOR DRIFT, same day:** the live requests now carry `data_type=combined`, `nodes=p20`,
`exp_mode=current` — this document said `gamma|vanna`, `nodes=500`, `exp_mode=week`. `combined` is
what trips the not-pure-gamma branch.


### View Controls (the HEATMAP/overlay sub-panel — the panel behind READ AS)
- QUICK PRESETS: Tight | Normal✓ | Wide
- **STRIKE RANGE: 92 strikes** (slider) — this IS the `max_strikes=92` seen in /api/data requests.
  ⚠ Governs how many strikes RENDER → directly governs harvest breadth. Narrowing it starves the
  minors and the peaks the same way LOW NODES: Hide would.
- **EXPIRATIONS: "5 of 50"** slider with presets 1W✓ | 1M | 3M | 6M | All — the HEATMAP pulls the
  WEEK strip (5 expirations; = `max_expirations=5`), while the strike LADDER is the front expiry.
  This reconciles the "week vs 0DTE" measurements: different panel elements, different windows.
- HIDE STRIKES: Empty | Under <threshold> (currently Off) — ⚠ another render-removal switch; hiding
  strikes removes their rows from the harvest.
- **Node % profiles**: PRESET Off | Focus | King · SAVED: Custom ▾ · "+ Save as" — named, saveable
  display profiles exist.

### The heat panel's SYMBOLS list (its own settings panel)
**SYMBOLS: SPY · QQQ · SPXW · VIX — plus "+ Add symbol".** The four rendered books are
USER-CONFIGURABLE. Strategic implication recorded 2026-08-26: adding GLD/USO HERE would render
their ladders → the fiber harvest gets their velocity objects natively — likely a cleaner data
path for the future multi-market FlexLevels build (GC/CL) than self-fetching, at the cost of the
cross-book VEL pollution the expiry/decade filters already handle. Each symbol row also has its
own STRIKES slider and Node % profile.

### Not explored (deliberately — display-only, and clicks risk nudging live config)
CANDLES styles, LAYOUT presets (Default/Single dropdowns), COLOR palettes beyond Mono, and the
per-indicator sub-panels of SMA 50 / SESSION LEVELS / VOLUME PROFILE / TRINITY / the off-toggled
indicators. Map them on demand with operator screenshots.


## FULL APPLICATION MAP — toured page-by-page 2026-08-26 (second tab; observe-only)

Operator directive: "i need you to know the entire application." Every page below was opened and
screenshotted live except where marked. **Only `/atlas*` routes run our panel** — every other page
is safe to browse in any tab; the standing rule is NEVER a second /atlas tab (double-writer).

### HOME
- **DASHBOARD** (root `/`): the Heatmaps landing — strike × expiration velocity heat GRID (not the
  ladder), GEX/VEX toggle, Movers count, King cell starred (seen: $20,359K★ at 7655). Same data
  family as Atlas but the week-strip window (5 expirations).
- **LIVE STAGES**: live/replay community trade rooms. No data, no pipeline impact.
- **PORTAL** (`/portal`): UNRESOLVED — two separate clicks both landed on Academy; likely an
  account/billing portal or a redirect. Re-check some idle day; zero pipeline relevance expected.
- **ACADEMY**: certification courseware — "Reading The Dark Pool Prints", "Atlas", **"Talon Prompt
  Guide"** (Talon = Skylit's own AI assistant; also surfaces as the "T" button in Trinity's
  toolbar), chapters on Market Structure, Dealer Positioning & Gamma Mechanics, Node Hierarchy,
  Gamma Regime Awareness & Day Forecasting, Heatseeker Pattern Recognition. Their curriculum IS our
  doctrine's vocabulary — useful for terminology alignment.

### HEATSEEKER (the dealer-positioning suite — our world)
- **HEATMAPS**: same grid as Dashboard.
- **ATLAS**: the chart the panel lives on — fully mapped in the settings sections above.
- **ALERTS**: Skylit-native NODE alerts — empty table with columns Ticker / Name / NODE / Exp /
  Trigger / Mode / Threshold, plus a "Smart Alerts" button and a Tracked tab. None configured.
  Their built-in version of our ▶ watch flags; if the operator ever configures these, they are
  vendor-verbatim confirmations of node events.
- **TRINITY MODE** (own route, NOT /atlas — safe): "TRINITY · 3 panels" — SPXW | SPY | QQQ full
  ladders side by side, each with its own King header, %King rows, roll badges (+7%/−19% chips),
  bottom heat strip; toolbar has GEX/VEX, sort, interval, palette, history, **T (Talon)**, Share,
  LIVE + Last-Synced clock. This is the doctrine's cross-index agreement view rendered natively.
  Kings seen 08/25 evening: SPXW 7655 $19,898K★ · SPY 765 −$222,420K★ · QQQ 709 $12,234K★.
  ⚠ It renders ladders → if opened while the main tab records, its fibers are NOT harvested (other
  tab, other DOM) — no conflict either way.

### FLOWSEEKER (the options-flow suite — a separate data family: prints, not positioning)
- **LIVE FEED** (`/flow/live`): the options aggressor tape. Header sentiment strip: net premium
  chip (seen "Bullish +$252.6M"), FIR %, bull/bear gauge, C/P contract counts + premium totals
  (C 32.9M $14.83B / P 21.1M $10.01B), P/C ratio, RVOL. Rows: Date/Time, Ticker (×N = multi-leg),
  Strike, C/P, %OTM, Exp, DTE, Fill, bid–ask Spread with fill-position dot, Side (ASK/BID/MID),
  **Flow Score −100..+100**, Contract Ratio (BID/ASK %), Size, Prem, Vol, OI, ΔOI. HISTORICAL
  mode, saved screeners, filters. SPXW 0DTE prints at our node strikes stream here — the natural
  future "flow at the node" confirm layer if we ever want one.
- **DARK FEED**: dark-pool print tape — Date/Time, Ticker, Price, Size, Notional, % AvgVol,
  Sector; Live + Leaders tabs; 500-print window.
- **FLOW SCANNER**: screener over the flow history — saved Screener tabs, columns Date/Time,
  Ticker, Contract, DTE, Spot, %OTM, Avg, Last, Chg%, Day%, Vol, OI, ΔOI, ΔOI%, Prem, IV, %Tot,
  Bull/Bear %, Chain bars; day selector; Filters/Columns pickers.
- **FLOW COMPASS**: sector-rotation dashboards — Breadth Heat Calendar (% of each sector above its
  20-day average, by session), Rotation Map (sector trend/momentum vs market, 3M/6M/1Y, replayable
  with Play), Net Impact (net premium NCP−NPP by ticker).
- **CONTRACT LOOKUP** (`/flow/lookup`): free-text OPRA contract search ("TSLA 6/20 135 C", any
  order); bare ticker → its top volume & OI contracts.
- **COMPANY EVENTS** (`/flow/events`): earnings-week calendar (Before Open / After Close chips
  sized by options premium, names >$10M premium over 20 sessions) + per-day **expected moves**
  table: Implied %, Expected Range, Typical %, VS Normal (×), option Expiry used, Last Reactions %.
  Honest blanking: names without near-the-money 0-ish-DTE options are left unpriced.
- **FLOW TRACKER**: Tracked Flow / Tracked Contracts — bookmarks made from Live Feed rows
  ("Track trade" on right-click). Empty until used.
- **FLOW ALERTS**: criteria alerts on the flow tape ("Get notified the instant a trade matches
  your criteria"), Alerts + History tabs, delivery via the app's notification settings.

### NEXUS (community/social — no pipeline relevance)
- **LEADERBOARD** (`/nexus/leaderboard`): ranked trader profiles — tiers Oracle/Omega/Ultima/
  Gamma/Delta/Theta (rank unlocks after 10 closed trades), Individual vs Guilds, Season/All-Time,
  seasonal event banner ("Arc 3: Convergence"), win rate / avg return / streak / specialty chips.
- **MY PROFILE · TRADES · IDEAS · NEW TRADE · SETTINGS**: not opened (browser link dropped at this
  point) — trade-journal/social pages by name; nothing suggests data or chart interaction.

### Cross-cutting observations
- Sidebar bottom icon bar: docs, calendar, keyboard-shortcuts, notification bell (unexplored).
- **Talon** (their AI assistant) appears twice: Academy course + "T" toolbar button. Unexplored.
- NOTHING outside `/atlas*` mounts the chart or touches gex/levels the way Atlas does; Flowseeker
  is a different backend family (OPRA prints), Nexus is social. Chart-flip risk (GLD/USO on the
  ATLAS chart corrupting STATE.SPY) remains an Atlas-only concern.
- STILL UNVISITED: Portal (redirect mystery), the five Nexus sub-pages, Talon itself, the bottom
  icon bar. All believed pipeline-inert; finish on any reconnected session.
