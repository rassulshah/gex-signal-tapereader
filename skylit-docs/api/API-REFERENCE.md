# Skylit Public API — reference capture (for the GEX Signal Tapereader)

Captured 2026-08-18 (night, CT) from https://docs.skylit.ai — `api-reference/introduction`, `api-reference/authentication`,
`api-reference/heatmap/*`, `mcp/overview`, `mcp/quickstart`, `mcp/tools`. **Limited beta.** Re-capture when the docs change
(the load procedure reads this file; if it is older than ~30 days, re-check the live docs).

## 1. Basics
- Base URL: `https://api.skylit.ai` · MCP endpoint: `https://mcp.skylit.ai/mcp` (streamable HTTP, JSON-RPC; same key, same credits)
- Auth: `Authorization: Bearer <key>` (or `X-API-Key: <key>`). Keys are created in the **Developer tab of the Skylit app** ("account console").
  New accounts seeded with **5,000 credits**. NEVER commit a key to the repo; never paste one into a prompt; rotate if leaked.
- Envelope: success `{ data, meta }`, errors `{ error }`; camelCase. Every chargeable response carries `X-Credits-Remaining`.
- Rate ceiling 600 req/min (gateway safety; `X-RateLimit-*`, `Retry-After`). 401 bad key · 403 revoked/suspended/plan lacks API ·
  402 `insufficient_credits` · 429 rate/stream limit · 404 `no_data` (historical, nothing near that instant) · 503.
- `GET /v1/openapi.json` — full OpenAPI, 0 credits, **requires auth** (unauthenticated call returns `{"error":"Authorization field missing"}`).

## 2. Heatseeker endpoints (the ones that matter to us)
| Endpoint | Cost | What |
|---|---|---|
| `GET /v1/heatmap?symbols=SPY[,SPX,QQQ]&metric=gamma|vanna&maxStrikes=92 (1..400)` | 1 | LIVE per-strike heatmap at the latest snapshot, with `velocityPct` |
| `GET /v1/historical?symbols=…&at=<RFC3339>&metric=…&maxStrikes=…` | 5 | REPLAY: the snapshot nearest `at`, up to **365 days back**; same shape minus `velocityPct` |
| `GET /v1/stream?symbol=SPY&metric=…&maxStrikes=…` | 1 on connect + 1/min | SSE: `connected`, `initial_data`, `snapshot_update` (full heatmap on each change), `velocity_update`, `credits` (each minute), `closed`, `reconnect` (~1h, `max_duration`); keepalive comment every 30 s; max 5 concurrent streams |

MCP tools: `heat_heatmap` (1) and `heat_historical_heatmap` (5) wrap these 1:1. (The other 38 MCP tools are Flowseeker —
options flow, sweeps, dark pool, etc. — `flow-api.skylit.ai`; see the catalog page. Not part of this project yet.)

## 3. Response shape (both heatmap endpoints)
```
data.symbols[]            one element per requested symbol
  .symbol                 canonical ticker
  .asOf                   RFC3339 — the snapshot ACTUALLY returned (nearest to the requested instant)
  .spot                   spot at the snapshot
  .previousClose .priceChange .priceChangePercent
  .expirations[]          YYYY-MM-DD dates contributing to each strike's net value
  .strikes[]              ordered by strike ascending
     .strike              number
     .value               NET exposure for the metric at this strike (summed across the returned expirations); signed
     .nodeType            king | gatekeeper | pika | barney | significant | normal
     .velocityPct         LIVE only — % change of this strike's value over the velocity window (omitted on /v1/historical)
meta.metric gamma|vanna · meta.resolution "1m" · meta.mode live|historical · meta.cached
```
Example (live): `{ "strike":590, "value":1894300.4, "nodeType":"king", "velocityPct":12.4 }`.

## 4. What this means for the Tapereader (read before planning any use)
- **Historical backfill is possible but metered.** Resolution is 1 minute; one SPY replay = 5 credits. Rebuilding a session at our
  3-minute bar cadence = ~130 snapshots = **~650 credits/day**; at 1-minute = ~1,950/day. The 5,000 seed credits ≈ 7 session-days at
  3 m. Any backfill plan must state its credit budget first. Trinity (`SPY,SPX,QQQ` in one call) costs the same per call — one call,
  three symbols — so cross-index history is cheap relative to single-symbol.
- **What the API gives vs what the panel scrapes today:** the panel's Layer 0 intercepts the app's own `gex/levels` feed (per-strike
  values incl. the SPXW-derived lanes, the King, and the tape `<table>` with %King). The API gives the same per-strike NET value +
  Skylit's node class (`king`, `gatekeeper`, `pika` = positive/green, `barney` = negative/purple, `significant`, `normal`) and live
  `velocityPct`. It does NOT give: %King directly (derive: value / |king value|), the SPXW-derived lanes inside the SPY book (request
  SPX separately), accumulation history (derive from consecutive snapshots), or the TradingView candles (price comes as `spot` only —
  OHLC per bar must come from elsewhere or from a dense replay).
- **No velocity in historical** — the "rate of change / Building / Fading" read must be re-derived from consecutive replays; that is
  fine for the ledger (it already computes acm/dec from series) but it is NOT the same number the live `velocityPct` shows.
- **Node classes are the Academy vocabulary** (Pika = +γ, Barney = −γ) — the same ones master-spec uses. `significant` ≈ our "strong
  node" threshold; confirm the cutoff empirically before equating them.
- **Agent access:** the MCP server can be added to Claude Desktop as a custom connector (Settings → Connectors → Add custom connector →
  `https://mcp.skylit.ai/mcp`, bearer key). If the user connects it, a Claude session can call `heat_historical_heatmap` directly —
  the nightly/weekly reviews could then pull replays for verification (credits!). The cloud sandbox cannot hold the key; the user
  configures the connector themselves (Claude must never be handed the raw key).
- **Rule unchanged:** descriptive only. Historical data is for measuring the panel's claims over more sessions than the live tape
  has produced — it does not change the lockdown (≥20 sessions of OUR recorded data before any weight moves); but a backfilled
  ledger/Map over prior months is a legitimate MEASUREMENT source once the ingestion is spec'd and the credit budget approved.

## 5. Open decisions (ask the user, one at a time)
1. Get an API key (Developer tab) and decide the credit budget for history.
2. Connect the Skylit MCP to Claude Desktop (then the reviews can query history) — or run a local script with the key in an env var
   that writes `data/hist/YYYY-MM-DD.json` into the repo (same schema as the day files, minus what the API cannot provide).
3. Which questions history is allowed to answer first (the four node scenarios: fresh-node deflects, acm attracts, dec releases,
   rolled holds — all derivable from consecutive snapshots + spot).
