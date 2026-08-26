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

### REQUIRED POSTURE for the panel's capture (the four rules)
**RTH · READ AS %King · VELOCITY All · LOW NODES Dim or Fade (never Hide).** Session ETH, Value
display, Selected velocity, or Hidden low nodes each degrade or break a measured pipeline.


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
