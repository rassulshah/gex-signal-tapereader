# GEX SIGNAL TAPEREADER — LEARNING / TEACHING SPEC

Purpose: Bring any context window up to speed on the domain doctrine behind the Gex Signal Tapereader. This is the "why." It pairs with the Handoff Spec (the "where we are"). Source of truth for doctrine is the Skylit Heatseeker Master Trading System v6.1 (and the Heatseeker Trading Manual v13 read this session); this distills them. Do not treat any win-rate or R:R figure as ground truth to hardcode — they are the doc's claims, used only to shape behavior/labels, never asserted as fact to the user.

## 1. What the system is

A GEX (Gamma Exposure) node-map reading system. Options dealer positioning creates "nodes" at strikes; nodes act as magnets, support, and resistance for price. The whole method is: read where the dominant nodes are relative to price, in the direction price is trending, and anticipate where price is being pulled (targets) and where it will bounce/reverse (support/resistance).

Positive GEX ⇒ dealers buy into weakness ⇒ acts as support. Negative GEX ⇒ dealers sell into strength ⇒ acts as resistance. Magnitude (absolute size, doc uses $ thresholds — >$1M strong, >$500K minimum, <$500K = ignore as speculative/decoy) matters more than sign for importance; sign tells you the node's role.

## 2. Core vocabulary (must use this language)

**Node** — a strike with significant dealer exposure. A price magnet. Multiple touches weaken it.

**King Node** — the single highest-magnitude node in the direction of price movement. The session/weekly settlement target. Uptrend ⇒ the highest node above; downtrend ⇒ the lowest (most negative) node below. Price typically tests the King once per session; King rejection often signals reversal.

**Gatekeeper** — the second-highest node between current price and the King, measured in the trend direction. It blocks price from reaching the King until accumulation shifts. It is the most reliable pullback-termination point — i.e. the best entry/bounce zone. Red flags that invalidate a Gatekeeper: not actually 2nd-highest; more than ~5 points from price; magnitude too weak; no accumulation.

**Floor / Ceiling** — Floor = positive-GEX node where price bounces in downtrends (support). Ceiling = negative-GEX node where price bounces in uptrends (resistance). They grow as price approaches — growth confirms the level is a real termination zone.

**Accumulation vs. Dissipation** — Accumulation = a node's magnitude increasing (dealer conviction building ⇒ stronger bounce/reversal probability). Dissipation = magnitude shrinking (node weakening ⇒ likely to break). Rate of Change (ROC) matters: rapid accumulation = strongest level; rapid unwinding = exit, the node is collapsing. This is the single most important real-time signal.

**Air Pocket** — a gap in the profile with no significant node over 10+ points. Price moves fast through it; no support/resistance; whipsaw risk. It is a pathway, not a target — avoid initiating inside one.

**Double Stack / Node Cluster** — two or more adjacent strong nodes forming a "fortress." Strongest reversal zones; produce many bounces (6–10/day); highest win rate and R:R.

**Midpoint** — the 50% zone between two nodes. ~1:1 R:R, no magnet — a trap. Never target or trade it.

**Map Reshuffle** — the profile changes materially (nodes vanish/appear, magnitudes shift, polarity flips). Pause and re-map; update King/Gatekeeper and targets.

**Speculative / Decoy / Hedge nodes** — weak (<$500K) or mixed-signal nodes. Ignore or treat with caution.

**Node personalities** (manual v13 layer, adopted for naming). On top of Floor/Ceiling, nodes carry a polarity-plus-behavior identity: Pika = yellow / positive-GEX, a stabilizer or "pillow" that cushions and reverts price; Barney = purple / negative-GEX, an accelerator or "fuel" that speeds price through. This is the same positive-supports / negative-resists doctrine of §1, given personality names so the READ and lexicon can speak about node character, not just sign. Polarity-aware King: a yellow King is a reversion magnet, a purple King is an acceleration zone.

**Ghost / Dormant node** (manual v13). A node that has decayed to insignificance but was historically meaningful; it can "resuscitate" — re-expand rapidly as price nears — using the historical-by-date endpoint to keep it on a watchlist. A resuscitating ghost is a warning of a re-forming level, not a dead one.

## 3. Node lifecycle & retest decay

Bounce probability falls with each touch of the same node: 1st touch ~80%, 2nd ~66%, 3rd ~50%, 4th+ <33%. Implication for anything we display: a node's freshness and its accumulation trajectory are both quality signals; a heavily-tested or dissipating node is low-conviction.

**Hedge exhaustion / wash-off** (manual v13). A related lifecycle read: as a node is tapped repeatedly, dealer hedges deplete and its magnitude shrinks per tap; a level that has been tapped several times and is bleeding is "washed off" — spent, low-conviction, likely to give way. Tap-count and per-tap shrinkage are the observable signals.

**Bearish divergence** (manual v13). When a node's strength changes against the direction of price — e.g. a support node grows stronger while price rises away from it — the move is suspect (a "fake bounce" / artificial advance). Divergence between price direction and node-strength direction is a caution flag surfaced in the READ.

## 4. Targets (directly relevant to our Target element)

Targets are a trend-directional ladder anchored to the node map, not arbitrary levels. Doc's Target Placement:

- T1 = next node in the trend direction — "typically King, or mid-level between Gatekeeper and King."
- T2 = the King Node — the primary target.
- T3 = beyond the King, only on volume break-through.

Our project decision: cap at the King — no beyond-King (no T3). So our ladder is T1 → King. And T1 is treated as the Gatekeeper (the strong intermediate node before the King), with an accumulation override: a node that is growing fast (dissipation's opposite — the doc's "rapid accumulation = strongest termination zone") can be promoted into the T1 slot even if it isn't the statically-strongest intermediate node. This is the "771 grew and rejected" case.

## 5. Trend (directly relevant to our Trend badge)

The doc identifies trend visually / from recent price action, then defines King relative to it. Our project operationalizes trend as a concrete, close-based, SMA-50 indicator so it "matches Skylit's SMA 50": Up = ≥75% of the last 20 closed 3-min bars close above the SMA 50 (outside a small ATR deadband) AND the SMA 50 slope is rising; Dn = the mirror; else Sideways. Trend direction drives which side the King/targets live on, and the Target element shows nothing when Sideways (doc: don't trade chop / wait for a real node).

## 6. The primary edge & 3-point validation (context, not something we execute)

The doc's headline edge is pullback termination at the Gatekeeper (claimed 78%+ when validated), plus King rejection + floor accumulation (claimed exceptional R:R). A setup is "valid" only when all three points fire: (1) Node — Gatekeeper is genuinely 2nd-highest, close (1–3 pts), strong (>$500K), top-5; (2) Reaction — sharp wick rejection, close back through the wick, volume spike, live GEX growth at the node, confirming candle; (3) Alignment — SPX/SPY/QQQ all show the same level/trend/accumulation, no divergence (QQQ divergence is a common trap). Validation win-rate progression (doc's claim): node-only ~55%, node+reaction ~65%, all three ~78%+.

Important scope boundary: our tool is descriptive/structural only. We surface structure (nodes, King, Gatekeeper, accumulation, trend, targets, and the state-machine's BO→FT→TST→CONF→GO progression). We do not give entries, stops, sizing, R:R, or P&L. Win-rate/R:R figures from the doc are never presented as our own guarantees.

## 7. Regime / volatility context (manual v13 — the "why" behind L3)

Doctrine holds that the node map's meaning is conditioned by the volatility regime, read primarily through VIX. There is a gamma-vs-vanna distinction: in a gamma-dominant regime nodes behave as deflectors (price rejects/repels off them — a ceiling is a "deflection wall," watch for rejection not breakthrough); in a vanna-dominant regime nodes behave as attractors/magnets (price is drawn toward them). VIX is read as a graduated multi-band scale rather than a single line (the handoff records the working bands: <17.55 / 17.55–18.8 / 18.8–20.11 / 20.11 pivot / >25 / >30), with the ~20.11 area treated as a pivot for our instrument (to be confirmed by our own data, not assumed).

**Volatility Veto:** a sharp VIX spike (>~3 points) overrides the prevailing gamma read — volatility itself takes precedence over structure until it settles. A 10-minute confirmation rule guards against reacting to transient regime flips. VIX's role in the metaphor set is the referee / reality-check: it says which rulebook (deflect vs attract) is in force. This is doctrine used to shape labels and verbs only; whether the specific bands hold for our instrument is a hypothesis for our base rates to confirm (contamination rule).

## 8. How the doctrine maps onto the tool's elements

- **Trend badge** ← §5 (trend definition, drives King direction).
- **Target line** ← §4 (King as T2; Gatekeeper/accumulation-promoted node as T1; capped at King).
- **SPY Signals (state machine)** ← the BO→FT→TST(PB)→CONF→GO lifecycle is the tool's own operationalization of "breakout → follow-through → pullback/test → confirmation → go," gated so a setup only displays once it has cleared BO + FT (see Handoff). Each stage's meaning is explained in per-stage tooltips using this vocabulary. The node-% "20% → 25% / 20% → NA" line expresses accumulation vs dissipation (§2) for the specific node backing that setup.
- **Accumulation section** ← §2 accumulation/ROC: tracks nodes gaining king-relative strength, hero-stars the ones being directly accumulated, shows trajectories. Regime verbs from §7 (deflect vs attract) select the READ's wording; node personalities from §2 (Pika/Barney) and lifecycle states (fresh/washed/ghost/resuscitating) supply the tags.

## 9. Golden principles to preserve in any feature

Structure = direction; liquidity (air pockets) = speed; rate-of-change (accumulation) = intensity; confluence (multi-index) = probability; regime (VIX gamma-vs-vanna) = which rulebook is in force. Magnitude over color for importance. Growth = conviction; decay = exit. Never target the midpoint; never treat an air pocket as a destination. Respect map reshuffles. Everything the tool says is a read of dealer structure, never a trade instruction.
