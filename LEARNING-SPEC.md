# GEX TAPEREADER — LEARNING SPEC (Skylit methodology → app implementation)
_Authoritative. This is WHY the app exists in its current form. Every metric/pattern/READ line must trace to a doc principle here. When adding a feature, find its principle below; if it isn't here, add it. Source: docs.skylit.ai (read 2026-08-12): Core Concepts, How-to-Read (5-step), Best Practices, Limitations, Common Pitfalls, FAQs, Patternpedia (Whipsaw, Rainbow Road, Gatekeeper, Speculative/Decoy case study, Trend, Rug). Not yet read: Ten Commandments, web-app nav._

## 0. GOVERNING POSTURE (the app's voice — from Best Practices / Pitfalls / Limitations)
- **Context, NOT a signal generator.** "Heatseeker is a lens, not an entry system." The app MEASURES whether structure predicted price and NAMES what's forming; it never emits buy/sell. Correct hierarchy to encode in the READ + synthesis: **Price Action (primary) → Heatseeker confluence (validates, where our metrics live) → Asymmetric R:R (worth taking?).**
- **Win-rate != edge.** Always show MFE/MAE payoff beside every hit-rate. A 55% hit with asymmetric R:R can be a good edge; a 60% hit with MAE>MFE is a trap.
- **Absolute value rules (color/sign usually irrelevant)** — EXCEPT the Rug (sign is the whole pattern). King = largest |value| regardless of yellow/purple.
- **Honest degradation.** Low sample -> "insufficient". Range day -> directional edges flagged low-signal. NEVER fabricate a number the data can't support. Show n and baseline on everything.
- **Think in zones, not lines.** ~5-10pt margin on SPX ~= ~0.5-1.0pt on SPY for "at a node"/pin.

## 1. NODES & MAGNETS (Core Concepts)
- Every node = a magnet; pull STRENGTHENS as price nears, WEAKENS with distance (encode nearer-node heavier weighting — already in netPositioning 1/(1+dist)).
- Positive gamma (yellow) = lower-vol, smooth. Negative gamma (purple) = wicky, violent, can overshoot & trap. -> gamma SIGN modulates expected volatility of a move (use for Rug + move-quality).
- **Retest decay (FRESHNESS):** 1st touch strongest / 2nd ~66% / 3rd+ ~33%. -> weight every node-event by touch-count; a fresh fading floor >> a thrice-tested one. (TODO: implement touch-count weighting in edge metrics.)
- **Rate of change:** rapid accumulation = magnet pulling in hard; rapid unwind = level suddenly weak -> volatility. (We have build-rate; add velocity when API gives it.)

## 2. KING NODE (Core Concepts / How-to-Read step 2 / FAQ)
- The EOD/EOW settlement target. **Early reach -> dealers DRIVE price off; late reach -> PIN.** (Encoded: A_kingBehavior pin metric + early/late timing.)
- Rejects within a few points — not cent-perfect (zone thinking).
- **Rolling floors/ceilings = directional evidence:** rolling ceilings DOWN = bearish thesis playing out; rolling floors UP = bullish. (Encoded: King rolls up/dn + net drift; A_regime Trend uses one-way LEADING rolls.)
- Node hierarchy (FAQ): **King > Gatekeeper > Clusters (pin/chop) > Double-stack (bounce).** -> significance weighting order.

## 3. GATEKEEPER (Core Concepts / Patternpedia: Gatekeeper)
- Def: a high-value node BETWEEN price and the next larger node (the King) that blocks continuation; becomes key S/R. **Geometric -> detectable from current node data.**
- **Strength ratio = gatekeeper value / node-beyond.** >>1 -> expect stall/REVERSAL at gatekeeper, not continuation.
- Failed test at gatekeeper -> map RESHUFFLE / trend shift; early-day rejections = powerful reversals.
- USES: (a) reversal setup; (b) DECOY DISCOUNT on King reach (a King gapped behind a strong gatekeeper is less reachable).
- STAT (to build): when price approached a strong gatekeeper, % reject/reverse vs break-through; and did rejection precede a reshuffle. Track BOTH "caused reversal" and "blocked King reach" columns (user cares about the reversal read — confirm weighting).

## 4. SPECULATIVE / DECOY NODES (Patternpedia case study)
- A big node is likely a DECOY (not a real target) when: far OTM; gapped BEHIND gatekeeper(s); lacks supporting nodes along the path; OR price already made a large prior run (exhaustion).
- -> King reach/pin must DISCOUNT such Kings so a correctly-ignored decoy isn't scored as a "miss". (TODO: implement decoy discount using distance + gatekeeper-in-path + prior excursion.)

## 5. REGIMES (Patternpedia — the day-type classifier, our headline layer)
- **TREND:** King far from spot + small counter-skew; price stair-steps toward King, King rolls to the NEXT node ahead (LEADING rolls). Mechanic: **nodes away from price FADE while nodes in the trend direction BUILD** (= build-ahead/fade-behind polarity = our netFlow; this is also Skylit's literal trend definition -> justifies polarity-complete netPositioning). Trade PULLBACKS.
- **WHIPSAW:** wide range whose edges are 2-4 high-value nodes with a low-value middle; King FLIP-FLOPS in a tight core, ~zero net. Trade = FADE THE EDGES, never the middle (theta/instability kill 0DTE mid-range). (8/11 = validated Whipsaw.)
- **RAINBOW ROAD:** MANY prominent nodes (both signs) scattered over a wide span (80-100pt), NO clear edges -> choppy/erratic -> STAND ASIDE.
- Discriminators (all from current DOM data): King net drift + roll one-way-ness + lead/lag; King core tightness (dwell-weighted, ignore 1-bar outliers); node dispersion (bimodal edges vs scattered). Encoded in A_regime.

## 6. RUG SETUP (Patternpedia — project namesake; SIGN MATTERS)
- Def: **yellow (+gamma) node stacked ABOVE a purple (-gamma) node, negative-gamma floors below, NO clear floor.** When the yellow ceiling UNWINDS, the negative gamma below ACCELERATES the drop ("gas on fire") -> violent nosedive. Targets = lit-up downside negative-gamma nodes.
- Confirm: downside nodes GROWING + ~zero upside accumulation + (best) TRI-INDEX agreement (SPY rug was the catalyst for QQQ/SPX in the 10/7 example).
- REVERSE RUG ("rip"): bullish mirror — purple ceiling above a yellow floor, no ceiling to cap -> violent up.
- Data: we retain gamma sign via node.tp ("tape %King signed", best-effort) -> Rug DETECTABLE in DOM data with a CLEAN v10.20+ capture. Tri-index form needs API/SSE (v10.22+).
- STAT (to build): of N bars a Rug/Reverse-Rug was present, % resolved the expected direction, avg MFE/MAE, time-to-break; pooled across days (single-day n too small).

## 7. CLUSTERS & DOUBLE-STACKS (FAQ)
- Cluster (several heavy nodes grouped) -> PIN/CHOP around that region (stronger form of compression).
- Double-stack (adjacent heavy strikes) -> strong BOUNCE off that level.
- STAT (to build): forward range-rate near clusters; forward MFE off double-stacks vs lone nodes.

## 8. FLOW / RESHUFFLES (How-to-Read step 5 / Pitfalls / Limitations)
- Accumulation = building magnets; Dissipation = weakening; **Reshuffle = new structure forming (NOT noise).** "Trade ahead of the reshuffle, not after." Reshuffles follow CPI/FOMC/Power-Hour.
- STAT (to build): reshuffle frequency, what preceded (gatekeeper rejection?), forward behavior after a reshuffle. (RESHUFFLE flag exists internally.)

## 9. CONFLUENCE — CROSS-SYMBOL (How-to-Read step 4 / Best Practices #6 / Pitfalls #2)
- **HARD RULE: SPX, SPY, QQQ must AGREE — if one diverges, STAND ASIDE.** Divergence includes "chop on one index." No-confluence example: SPX downside acc / SPY chop / QQQ upside -> probability drops drastically.
- Test to encode: "would my thesis hold on QQQ instead?" All-3 aligned = highest-probability outcomes.
- Data: Skylit Trinity already fetches SPY/QQQ/SPXW/VIX; "derived exposures from related symbols" toggle. -> tri-confluence achievable from DOM/streams; API adds SPX cleanly. (v10.22.)

## 10. GEX/VEX CONFLUENCE (Best Practices #5) + TOPPING/BOTTOMING (Patternpedia)
- GEX/VEX OVERLAP = stronger zone (dual-layer confirmation) for intraday fades/bounces.
- **Higher-timeframe (swing) top/bottom is read primarily via VEX**, not GEX: "little/no upside accumulation across indices = market TOP"; invalidation = higher accumulation at higher strikes + dissipation at lower + stronger near-money. Same polarity engine, multi-DAY window.
- Data CORRECTION: VEX is ALREADY hooked (data_type=vanna -> LASTVEX{SPY,QQQ}) but UNUSED + UNPERSISTED. -> (a) read LASTVEX for GEX/VEX overlap; (b) persist VEX snapshot in recorder -> unlocks multi-day topping/bottoming from DOM. (v10.22.)

## 11. LIMITATIONS -> HONEST GUARDRAILS (Limitations page)
- Not 100%; news/earnings alter flows -> event-day flag (FOMC/CPI/NFP) + Hedge-node awareness.
- Low-volume (lunch), OPEX weeks, short-squeeze/illiquid distort the map -> SESSION-PHASE segmentation (open / lunch=noisy / power-hour 3:30ET) + OPEX flag on edge metrics; show whether an edge concentrates in reliable windows vs lunch noise.
- Reshuffles can invalidate a valid thesis -> lead with R:R, not "being right".

## 12. TWO SURFACES, ONE DETECTOR (architecture — the coherence principle)
The pattern/regime detectors are written ONCE in the analytics core and serve BOTH surfaces:
- **Dashboard READ (LIVE, no forward outcomes):** NAMES the regime + flags forming patterns ("Whipsaw — fade edges"; "Rug forming — needs the yellow to unwind"; "Gatekeeper 771 blocking King 775"). Detection, not stats. Conservative naming + explicit "forming" qualifier for early state (per context-not-signals posture).
- **Analysis tab (RETROSPECTIVE, has out5/out10):** pattern STATS — occurrence counts + forward hit-rate vs baseline + MFE/MAE + n, pooled across days. Plus the King/edge/regime reads already shipped in v10.21.

## 13. WHAT'S DONE vs TODO (trace to sections above)
DONE (v10.21): regime classifier S5 (Trend/Whipsaw/Rainbow), King behavior + pin/timing S2, accumulation/dissipation/combined edges S1/S5, coherence tooltips + posture S0, loader, smaller grade chip.
TODO — TOMORROW (v10.22/23, must incorporate S3/S4/S6/S7/S8/S9/S10/S11):
1. SSE feed migration (Skylit -> /api/stream) so LIVE data is trustworthy (prereq for the live READ).
2. Detectors: Rug + Reverse-Rug S6, Gatekeeper S3 (+ strength ratio + decoy discount S4), Cluster/Double-stack S7 — written once, used by both surfaces S12.
3. Dashboard READ: name regime + flag Rug/Gatekeeper LIVE (conservative + "forming"). S12
4. Analysis tab: pattern STATS (Rug/Reverse-Rug/Gatekeeper/cluster) with hit-rate vs baseline + MFE/MAE + n, pooled. S6/S3/S7
5. VEX: persist from LASTVEX + GEX/VEX overlap S10; freshness/touch-count weighting S1; session-phase + OPEX/event flags S11.
6. Cross-symbol tri-confluence gate S9 (Trinity data / API).
