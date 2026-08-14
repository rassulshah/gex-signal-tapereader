# Skylit-docs-derived design spec for the GEX Analysis tab (captured 2026-08-12)

Source: docs.skylit.ai — Core Concepts, How-to-Read (5-step), Best Practices,
Limitations, Common Pitfalls, FAQs, and Patternpedia (Whipsaw, Rainbow Road,
Gatekeeper, Speculative/Decoy case study, Trend, Rug Setup; TODO: Topping/
Bottoming, Ten Commandments).

## POSTURE (from Best Practices / Pitfalls / Limitations)
- Heatseeker is CONTEXT, not a signal generator. Tab MEASURES whether structure
  predicted price; it never emits buy/sell. Correct hierarchy: Price Action (1) >
  Heatseeker confluence (2, where our metrics live) > Asymmetric R:R (3).
- Win-rate != edge. Always show MFE/MAE payoff alongside hit-rate.
- Honest degradation: low sample / flawed-sig days => "insufficient", never fabricate.

## REGIME CLASSIFIER (the headline new layer — from Patternpedia)
Classify each day/regime, because the correct read differs per regime:
- TREND: King far from spot + small counter-skew + LEADING King rolls (King relocates
  ahead of price) + build-AHEAD / fade-BEHIND polarity. Docs' literal trend mechanic:
  "nodes away from price fade while values in the trend direction increase." -> trade pullbacks.
- WHIPSAW: 2-4 heavy EDGE nodes + low-value middle + King FLIP-FLOPS (rollUp~=rollDn,
  small net) -> fade edges, avoid middle. (8/11 = textbook Whipsaw: net drift +4 but
  7up/7dn rolls, avgRollSize 1.29, reach 36%, px 769-771.)
- RAINBOW ROAD: many heavy nodes scattered, NO clear edges (80-100pt span) -> stand aside.
Discriminator primitives (ALL from current DOM data):
  King net drift + roll direction + lead/lag (kingBehavior) ; build-ahead/fade-behind
  polarity (combinedEdge.netFlow) ; node dispersion (bimodal edges vs scattered).

## KING METRICS (v10.21, validated on 8/11)
- Path, distinct levels, net drift, rolls (up/dn, avg size), lead vs lag.
- Rolling floors/ceilings = docs' bullish/bearish evidence (headline framing).
- Reach rate + time-to-reach; PIN metric (close within ~0.5-1.0pt SPY zone; 0.25=dead-on)
  + reach TIMING (early=drive-off risk, late=settlement pin).
- Decoy/speculative discount on reach: down-weight a King that is far-OTM, gapped
  BEHIND a gatekeeper, with no supporting nodes, or after a large prior excursion.

## GATEKEEPER (v10.21 approx, v10.22 clean via API)
- Def: highest-value node BETWEEN price and the next larger node (King). Geometric =>
  computable now.
- Strength ratio = gatekeeper value / node-beyond. >>1 => stall/reversal at gatekeeper.
- Powers: reversal signal + the decoy discount above.

## EDGE METRICS (v10.21, validated on 8/11)
- accumEdge('accum'): building support-below->up, resistance-above->down. vs baseline.
- accumEdge('fade'): fading support-below->down, resistance-above->up (DISSIPATION is
  directional — 8/11 fade-support->down hit 64%, swing 73%, +19 over baseline).
- combinedEdge: trapdoor (res build + sup fade -> down), liftoff (sup build + res fade
  -> up), compression (both build -> range), netFlow polarity, dual vs single.
- ADD touch-count FRESHNESS weighting (1st touch strongest ~ / 2nd ~66% / 3rd+ ~33%).
- ADD session-phase segmentation (open / lunch=noisy / power-hour 3:30ET) + OPEX &
  macro-event (FOMC/CPI/NFP) flags -> caveat/segment the edges.
- ADD node significance weighting + hierarchy King > Gatekeeper > Cluster.
- Cluster (grouped heavy nodes -> pin/chop) & Double-stack (adjacent heavy -> bounce):
  new detectable structures.

## RUG SETUP (project namesake — v10.21 single-sym, v10.22 tri-confluence)
- Def (SIGN MATTERS — exception to abs-value rule): yellow(+gamma) node stacked ABOVE
  purple(-gamma) node, negative-gamma floors below, NO clear floor -> violent nosedive
  when the yellow unwinds. Targets = lit-up downside neg-gamma nodes.
- Confirm: downside growing + ~zero upside accumulation + tri-index agreement.
- Data: we DO retain gamma sign via node.tp ("tape %King signed", best-effort). Needs
  clean v10.20+ capture to trust. Recorder note: consider promoting a reliable per-node
  gamma-sign field (not just best-effort tp).

## VEX — CORRECTION: available from the SAME feed NOW (no API key needed)
- The feed hook ALREADY recognizes data_type=vanna and stores it in LASTVEX{SPY,QQQ}
  (v10.js line 250). Atlas toolbar has GEX / VEX / combined toggles (ref_26/27/28) +
  a "derived exposures from related symbols" toggle (ref_29 — cross-symbol relevant).
- GAP: LASTVEX is a dead-end (saved, never read); recorder persists GEX nodes only, so
  VEX is live-only and absent from daily exports. VEX only streams when VEX/combined
  mode is active in the UI.
- => PULL VEX FORWARD to v10.21 (DOM): (1) read LASTVEX in analytics for GEX/VEX overlap
  (Best Practices #5: aligned gamma+vanna zone = stronger); (2) persist a VEX node
  snapshot in the recorder alongside GEX each bar -> unlocks multi-day Topping/Bottoming
  read WITHOUT the API; (3) honest guard: "VEX not streaming — switch to combined mode."
- API (v10.22) still adds: SPX, formal node classification, velocity, /v1/historical.

## CROSS-SYMBOL (v10.22, API key tomorrow)
- Rule: SPX/SPY/QQQ must AGREE or stand aside. Divergence incl. "chop on one index."
- No-confluence example (Pitfalls): SPX downside acc / SPY chop / QQQ upside -> prob drops.
- API /v1/heatmap exposes node classification (king/gatekeeper/pika/barney/significant/
  normal) + live velocity + SPX/SPY/QQQ. Metered beta; key from Developer tab.

## BUILD SPLIT
- v10.21 (DOM data now): regime classifier (Trend/Whipsaw/Rainbow), King metrics +
  pin/timing + decoy discount, gatekeeper approx, acc/diss/combined edges + freshness +
  session-phase, cluster/double-stack, Rug (single-sym), structured review LOADER,
  coherence tooltips (with docs' reasons), smaller grade badge. Context-not-signals.
- v10.22 (API): gatekeeper/velocity/node-class clean, VEX confluence, SPX/SPY/QQQ
  tri-confluence gate, macro-event/Hedge-node flags, Rug tri-confluence form.

---
## INTENTIONAL DIVERGENCE FROM ACADEMY (logged 2026-08-13, user decision)

### Strongest Floor/Ceiling selection blends proximity (BY DESIGN — do not "fix")
Academy (reading-heatseeker): "strongest floor is the LARGEST exposure node below
spot, NOT the nearest." Pure magnitude.

OUR CHOICE: the ★ strongest-floor/ceiling pick ranks by nmStrength() = a blend of
%King strength + accumulation velocity + PROXIMITY (TS_W_PROXIMITY=0.8) + structural
role. So nearness DOES influence the pick.

RATIONALE (user, 2026-08-13): "ours is better because it's more practical." A huge
floor many strikes away that price won't reach this session is less actionable than
a strong floor right underfoot. For an intraday tool, the blended pick is more useful
than the Academy's pure-magnitude definition.

STATUS: KEEP AS-IS. This is a conscious, approved deviation — NOT a bug. Do not
revert to pure-magnitude selection without explicit user sign-off.
