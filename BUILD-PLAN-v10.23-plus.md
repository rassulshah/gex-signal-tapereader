# GEX Signal Tapereader — BUILD PLAN (v10.23 → v10.27)
_Compiled 2026-08-12 from the ISSUES-BATCH-v10.23.md spec + the Skylit docs archive (skylit-docs/). NO code written yet — awaiting user "start development"._

## GOVERNING PRINCIPLES (from the docs, apply to every release)
- **Context, not signals** — the app names structure & posture; price action confirms. (Best Practices)
- **Absolute value ranks; polarity/sign only *flavors* behavior** (deflect vs accelerate). Exception: the Rug, where sign IS the pattern. (Core Concepts)
- **Asymmetric R:R > win-rate** — show MFE/MAE beside every hit-rate; honest "insufficient"/"forming" on low sample.
- **Source of truth = the tape.** Any app-vs-tape disagreement => the app is wrong.
- **The 5-step framework is the app's spine:** Magnets → King(destination) → Range(fade/avoid) → Gatekeeper(+tri-index agree) → Flow(reshuffle) ⇒ ONE anticipatory posture line.
- **Every new metric/detector logs its state + forward outcome from day one** (effectiveness tracker) so the LLM can refine thresholds against real data.

## THREE-PLUS-ONE MODULE ARCHITECTURE
1. King Node Tracker (direction/destination) · 2. Node Map (terrain) · 3. Signal Finder (triggers) · 4. (higher-TF) VEX Topping/Bottoming.

---

## RELEASE SEQUENCE

### v10.23 — FOUNDATION FIXES + KING-HEADER REWORK  (low-risk, self-contained, immediately visible)
_STATUS: CONFIRMED by user 2026-08-12. Scope locked as-is. READ removed now as planned (not deferred)._
Rationale: everything downstream depends on trustworthy build-rates, a correct SMA, and a de-flickered UI. Do first.
**LOCKED BUILD ORDER (fixes-before-visuals, safest — user delegated):** (1) shared bar-close-commit HELPER first (foundation reused by D/H/G); (2) continuous SMA-50 (C) — prereq for the trend badge; (3) flicker fixes (D+H) on the helper; (4) King-header visual rework as ONE unit: King Path (A) + badge offset (B) + trend badge (G) + remove READ (F); (5) info icon (N); (6) deploy ritual (node --check, regressions, version bump, snapshot, changelog, resume-note, install link). Every visual element sits on already-verified data.
- **C · SMA-50 continuous** — compute cross-session SMA from raw candle series (matches chart, exists from open); verify once vs Skylit's rendered SMA, fall back to it on divergence. Feeds trend badge + breakout MA filter.
- **D+H · Flicker family (shared bar-close-commit helper)** — dead-band + hysteresis + commit-on-3m-close for BOTH the S/R read (renamed → S/R Imbalance shell) and the per-node ladder badges; rapid-move override keeps genuine surges live. Crossovers derived from the committed state.
- **F · Remove the READ section** — delete the one call; King header auto-promotes to top.
- **A · King Path** — always-render price line (fix drop-off bug) + label price & signed offset "771.9 (-1.1)" + King-strike label on the gold dot + path +33% taller.
- **B · King badge offset** — "+1↑ / -1↓" (sign left, arrow right, offset-only).
- **G · Trend badge** — stacked pill matching the King badge (left of King node): state code top, state-colored count arrow + count, centered directional slope tick; "warming up" pre-window.
- **N · Info (i) icon** — the 5-step framework + how the app implements each step, in-panel.
- Ritual: node --check, regression tests (King/table + analytics + render), version bump, release snapshot + changelog, resume-note update, install link.

### v10.24 — NODE MAP v1 + INTRADAY DETECTORS  (the big new capability)
_STATUS: CONFIRMED by user 2026-08-12. SHIP AS ONE release (not split). Data-capture logging from day one CONFIRMED._
Depends on v10.23's de-flickered build-rates.
**LOCKED BUILD ORDER (user delegated):** (1) NODE MAP structure (container; reuses de-flickered build-rates; two-sided level list + verdict slots); (2) REGIME classifier (Trend/Whipsaw/Rainbow) — built early because regime GATES how every other read is interpreted (docs: correct read differs per regime); geometry-only. (3) GATEKEEPER — geometry+value only, extends the map's between-price-and-King ordering, feeds King decoy discount. (4) RUG/Reverse-Rug LAST — the ONLY polarity-gated detector; do the ONE-TIME pos/d-vs-ladder-color verify here so it doesn't block the geometry-only detectors. Fold in freshness + reversion with the Node Map verdicts; midpoint flag with the regime; air pockets (polarity-tagged) with the Rug/polarity step. Wire EFFECTIVENESS LOGGING as each detector lands (state+forward outcome from its first day).
- **I · Two-sided Node Map** — both sides always shown; strongest-Sup/Res headline (size+rate+nearness blend); per-level Bounce/Pullback/Break-through verdicts; direction emphasis; works toward AND away from the King.
- **S/R Imbalance (D core)** — divergence of the two sides' build-RATES near price, mechanism-named ("Bearish imbalance — resistance 772 building, support 769 fading").
- **Detectors (reverse-engineered from docs + examples, thresholds = starting points):**
  - **Gatekeeper** — nearest high-|value| node between price & King; strength ratio vs next node beyond; reversal when ≫1; reshuffle-anticipation on failed test; early-day weighting; feeds King decoy discount.
  - **Rug / Reverse-Rug** — yellow-over-purple, no floor below; uses confirmed per-node polarity (pos/d) — pending ONE-TIME verify sign matches ladder color; amplifies break-through verdict.
  - **Regime classifier (Trend / Whipsaw / Rainbow Road)** — exact geometric tests (1-sided ≥~1.8×skew / bimodal empty-middle ≥~2× / scattered both-sign no-middle); turns regime into an instruction (fade edge / stand aside).
  - **Midpoint flag**, **node freshness (touch-count decay)**, **air pockets (polarity-tagged)**, **reversion-rate read**.
- **Effectiveness data capture (MANDATE)** — recorder logs node-map/detector state + forward outcomes from day one (even before analysis UI exists).

### v10.25 — THE GUIDED 5-STEP POSTURE LAYER  ✅ BUILT+SHIPPED 2026-08-12 (live panel; King bug fixed)
_STATUS: user corrections 2026-08-12 — (1) tri-index = DESIRABLE confidence BOOSTER, NOT a hard gate; (2) SPXW IS in the tape (no API needed). PLACEMENT/TIMING confirmed below._
_PLACEMENT (user delegated -> agent rec): KEEP v10.25 as its own release AFTER v10.24, with a short v10.24 'soak' (run detectors raw, LOG effectiveness) BEFORE building the posture. Rationale: posture is a SYNTHESIS/assertion; per 'context not signals', don't assert 'bias=X act at Y' until the underlying detectors are validated on live tape. Principle = COMPUTE & LOG EARLY, ASSERT LATE._
_POSTURE WORDING: MOCK UP FIRST, iterate before coding (user)._
_TRI-INDEX TIMING (user delegated -> agent rec): COMPUTE + LOG the SPY/QQQ/SPXW(+VIX) confluence TAG starting in v10.24 (cheap; data already present; lets the effectiveness tracker measure how much confluence improves hit-rate). TURN it into the posture CONFIDENCE BOOSTER in v10.25. i.e. tag early, assert late._
- **K · King-as-destination** session-bias framing.
- **L · Regime-as-instruction** (fade-edge / stand-aside).
- **M · Flow "reshuffle-forming / trade-ahead"** read.
- **J · Tri-index confluence = CONFIDENCE MODIFIER (not a gate)** — when SPY/QQQ/SPXW AGREE, ELEVATE confidence ('tri-index aligned'); when they diverge, NOTE it as lower-confluence but DO NOT block/kill the setup. Softer, 'context not signals' reading (matches Best Practices 'confluence = confidence'), not the How-to-Read 'stand aside' hard rule. DATA: SPY + QQQ + SPXW are ALL in the DOM tape NOW (seen live: SPY/QQQ/SPXW/VIX panels) — NO API key needed. Use VIX as a bonus risk-on/off context input.
- **Posture output** — the single anticipatory line synthesizing all five steps (replaces the removed READ, now doc-grounded). Confluence shown as a confidence tag on the posture, not a veto.
- CORRECTION applied project-wide: earlier notes calling SPX/tri-index 'API-gated' are WRONG — SPXW is in the tape. Cross-symbol confluence (and the topping/bottoming VEX read) can use real SPY/QQQ/SPXW(+VIX) without the API. API key would only add velocity/node-class/historical polish, NOT the index data itself.

#### v10.25 MOCKUP DECISIONS (LOCKED 2026-08-12 — mock approved, ready to code)
_Mockup: magnet_5step_mockup_v10_25_rev6.html. All wording iterated with user._
- **HEADER = 3-MAGNET CLUSTER (centered):** green ★ strongest SUPPORT (left) · 👑 King (middle) · red ★ strongest RESISTANCE (right). Each side badge = the ★ STRONGEST (size+build-rate+nearness blend), shows strike + %King, stacked pill matching King-badge height. TREND BADGE REMOVED from header (regime/instruction line carries trend). Trend ENGINE stays (just not rendered).
- **5-STEP INFO ICONS:** small circular ①②③ in the header, ④ in the Gatekeeper area, ⑤ in the Flow area. CLICK opens a popover (not hover — text is long). Text = the Skylit doc methodology, quoted faithfully. Step-2 text = **EOD ONLY** (dropped EOW/swing project-wide). Step-4 text keeps the doc 'must agree / stand aside' rule of thumb VERBATIM even though our impl uses the soft booster (teaching stays doc-true; behavior is soft).
- **STEP 4 GATEKEEPER = OWN AREA below the King header.** No spelled-out 'GATEKEEPERS' title (space). Header line = WHITE castle-gate (portcullis) SVG icon + ④ icon. Lists JUST the PRIMARY gatekeeper (nearest blocker to the King): gate icon + strike + strength-ratio pill + verdict ('Reversal likely' / 'Watch'), an explanation line, and a tri-index confluence line (agree=boosts, diverge=noted; soft). White gate SVG (stroke #e6edf3) chosen for visibility. No-gatekeeper case: TBD (hide vs 'Clear path to King' note — confirm at build).
- **STEP 5 = SHARPEN THE NODE MAP into a two-stage MAGNET/DEFLECTION view (user reinterpretation 2026-08-12 — supersedes the single 'posture line' idea).** NOT a new area and NOT a one-line verdict: sharpen the EXISTING Node Map. WHICH SECTION (agent rec, locked): the NODE MAP — it already has price-approach + Bounce/Break-through, the closest fit. S/R Imbalance STAYS AS-IS (imbalance/divergence surface); NO relabel — resolves the old backward-ripple open item (keep the two surfaces distinct & complementary).
- **CORRECTED CAUSAL MODEL (user, 2026-08-12, w/ 2 chart examples — this SUPERSEDES the 'two-stage deflection' framing below):** ACCUMULATION ONLY ATTRACTS. Full stop. A strong Acm node pulls price toward itself and price often CONSOLIDATES on it. Accumulation does NOT predict the on-arrival outcome. Once price arrives it can DEFLECT, BREAK THROUGH, or BREAK-THEN-REVERSE (false break / trap) — and this is UNKNOWABLE in advance and CANNOT be called from a single bar. GROUND TRUTH (user chart 8/12): price got attracted to a node ~773.5 (11:45–12:15) and BROKE UP through it; and to ~773 (14:15–14:45) and BROKE DOWN through it — the node was even WEAKENING on arrival yet still attracted price first. So 'deflect' is NOT the expected outcome; break is just as likely.
- **STATE MACHINE OWNS THE OUTCOME:** the existing BO/breakout-pullback state machine (signals grid: BO -> FT -> follow-through/fail/reverse) is EXACTLY what resolves consolidate-on-node -> break -> hold/fail/reverse over time. Step 5 must NOT re-invent an outcome call; it HANDS OFF to the state machine.
- **STEP 5 = SHARPEN NODE MAP, ATTRACTION-ONLY (final):** the Node Map's ONLY claim is ATTRACTION. It makes NO deflect/break prediction.
  * price drifting toward a strong Acm node -> **'<k> Acm · attracting'**.
  * price consolidating ON that node -> **'<k> Acm · at node, watch BO'** (explicitly points to the breakout-pullback state machine; does NOT call the outcome).
  * ATTRACTION GATE: a node is an 'attracting' watch only when it is Acm/strong AND price is being pulled TOWARD it (approaching). A strong magnet price is moving AWAY from is not a watch yet.
- **OUTCOME ECHO (user, 2026-08-12, refined w/ Tuesday chart):** once the STATE MACHINE resolves, echo a small marker back onto the Node Map row. FOUR outcomes (a 3-way was too coarse — the Tuesday chart forced the 4th): **broke ↑** (clean break up) · **broke ↓** (clean break down) · **held** (clean reject, never broke) · **false break** (broke through THEN reversed back — the trap). The false break gets its OWN DISTINCT marker ('false break ↓→↑' or '↑→↓') because it's the HIGHEST-VALUE reversal tell. All four are REPORTS, not predictions.
- **GROUND TRUTH for the 4th outcome (user chart, Tue 8/11):** two circles = SAME setup (price attracted to a node, consolidating on it), DIFFERENT resolution — left circle (~12:00, ~771) broke DOWN clean; right circle (~13:30, ~770) broke down THEN reversed UP (false break / deflection higher). A naive one-bar 'did it reject' test would mis-call the right circle as 'broke ↓' and miss the long. => the state machine MUST watch the move unfold over multiple bars to catch break-then-reverse; the echo vocabulary MUST include it.
- **HONESTY LINE:** no part of the app claims to know the deflection/break in advance. Node Map says 'strong magnet pulling price here, watch the BO'; the state machine says what price actually did.
- **SUPERSEDED:** BOTH (a) the single compact 'posture line' (Watching '769 Acm · watch for LONG' -> Confirmed 'LONG off 769 ✓ · tgt 780') AND (b) the 'two-stage deflection ✓ / broke through' prediction model are NO LONGER the Step-5 deliverable. Step 5 = attraction-only Node Map + state-machine handoff + resolved-outcome echo. RE-MOCK (done: nodemap_attraction_v10_25). NOTE the old two-stage bullets left below are HISTORICAL — the CORRECTED model above governs.

#### v10.26-PREP — STEP 5 NODE IDENTITY (agent-designed, user delegated 2026-08-13; grounded in the authoritative Step-5 doc text the user pasted)
_The user confirmed the node-identification scheme is correct and asked to ADD the Step-5 flow status + a node TYPE classification. 'The key is implementing Step 5 — I'll let you decide the best way.' Agent decided the model below._
- **EACH NODE CARRIES THREE INDEPENDENT CLASSIFICATIONS (role · status · type):**
  1. **ROLE (structural, unchanged):** King / Gatekeeper / Rug-ceiling / Rug-floor / Floor / Ceiling. Shown as the leading marker+label on the row (glyphs: ★ strong, 👑 King, 🚪 Gatekeeper, 🧶 rug-target).
  2. **STATUS = the Step-5 FLOW (doc: Accumulation→stronger / Dissipation→weakening / Reshuffling→rapid change):** map the existing engine state Building/Fading/Steady to the DOC VOCAB **Acm** (green, strengthening) / **Diss** (red, weakening) / **Steady** (grey, holding). The existing RAPID flag = the doc's **Reshuffling** state, shown as **🔥** (rapid Acm / rapidly strengthening) or **❄** (rapid Diss / rapidly weakening) appended to the status tag; tooltip explicitly says 'Reshuffling — rapid exposure change (new structure forming)' to tie back to the doc word.
  3. **TYPE = gamma POLARITY (already on every node as `pos`):** **+γ** positive-gamma (pinning / mean-reverting, 'yellow') vs **−γ** negative-gamma (accelerant / breakout-prone, 'purple'). This is the SAME axis that drives Rug detection; surfacing it per-node makes the rug pattern legible at the node level (yellow ceiling over purple floor).
- **LAYOUT (agent decision):** role+strike remain the left identity; **status and type render as two small colored tags on the RIGHT** of the row (scannable, no cramped one-liner). Status tag colored by meaning (Acm green / Diss red / Steady grey); type tag colored yellow(+γ)/purple(−γ).
- **RELATION TO THE ATTRACTION MODEL:** status (Acm/Diss) is WHAT the node is doing to its own magnet strength; the attraction stage ('attracting' / 'at node · watch BO') is WHERE PRICE is relative to it; the outcome echo (broke/held/false) is the state-machine REPORT. All three coexist on the row without contradiction — none of them predicts the deflection.
- STATUS 2026-08-13: agent building into code now + fresh mockup. Vocabulary/rapid-icon/type-axis all agent-decided per user delegation.

#### v10.26-PREP — REMOVE PREDICTIVE VERDICT PILL; REPLACE WITH ROLE/SETUP BADGE (user 2026-08-13)
- **KILL THE VERDICT PILL (Bounce / Pullback / Break-through).** USER: 'this can't be determined — we can only determine accumulation/diss etc, so those badges are pointless.' The pill PREDICTED on-arrival behavior, which VIOLATES the locked attraction-only honesty rule (map makes NO deflect/break call). REMOVE it entirely from the node row.
- **REPLACE with a ROLE / SETUP badge** — factual, data-derivable, never predictive. This is what the node IS, not what it might DO.
- **AUTHORITATIVE ROLE/SETUP DEFINITIONS (user 2026-08-13, verbatim):**
  * **King Nodes** → Dealer settlement targets (highest-probability zones).  [detected: King selection]
  * **Gatekeeper Nodes** → Deflection zones where trend shifts often begin; can prevent price from easily reaching the King.  [detected: v10.24 gatekeeper()]
  * **Clusters of Nodes** → multiple large values GROUPED together → price often PINS or CHOPS around that region.  [NEW DETECTOR — not yet built]
  * **Double-Stacked Nodes** → multiple nodes STACKED together → price can have a STRONG BOUNCE from those levels.  [NEW DETECTOR — not yet built]
  * (plus the existing Rug-ceiling / Rug-floor from the v10.24 rug detector, and plain Floor / Ceiling by geometry.)
- **KEY BEHAVIORAL DISTINCTION to preserve in wording:** Cluster = PIN/CHOP (avoid, poor R:R) vs Double-Stack = strong BOUNCE (tradeable, fade into it). Both are FACTUAL structural reads, not outcome predictions.
- **TWO NEW DETECTORS REQUIRED:** clusterDetect (≥3 significant nodes within a tight strike band → chop region) and doubleStackDetect (2+ adjacent significant nodes same side → bounce shelf). Follow the same build+unit-test ritual as gatekeeper/rug.
- **COLOR NOTE:** losing the pill loses no real info — the STATUS color already carries directional meaning (Acm-green on a floor = bullish support; Acm-green on a ceiling = bearish resistance), and side is obvious from the price line. The pill was FAKE information; removing it is a correctness fix.
- STATUS 2026-08-13: ✅ SHIPPED in v10.26 — pill removed, role/setup badge live, Cluster + Double-Stack detectors built+tested (17/17 green). Deployed to current/. See RESUME-NEXT-SESSION.md.

### v10.26 — HIGHER-TIMEFRAME VEX + ANALYSIS TAB + LLM LOOP
_SEQUENCING (user LOCKED): ship all three bundled as ONE release (VEX topping/bottoming + polarity modifier everywhere + LLM loop + 5-step Analysis tab)._

- **VEX Topping/Bottoming** — wire in LASTVEX (already captured, never read) + persist a multi-day VEX matrix; little-upside-accum + gatekeeper wall + downside stair-step ⇒ top (mirror = bottom). No API key needed.
- **Polarity modifier everywhere** — deflect (pos) vs accelerate (neg) sharpening on all Node Map verdicts.
  - DECISION (user delegated -> agent rec, LOCKED): polarity is a GRADED CONFIDENCE MODIFIER + behavioral tag, NOT a verdict flipper. Structure (King / imbalance) owns the base verdict; polarity only shades it.
    - Verdict direction AGREES with node polarity behavior (e.g. bullish read into a NEG-gamma/accelerant node above) -> BOOST confidence (clean punch-through toward King likely).
    - Verdict direction OPPOSES (e.g. bullish read into a POS-gamma deflector wall) -> DAMP confidence + FLAG behaviorally ('772 is a +gamma deflector — expect stall/rejection, not a clean break').
    - Never let a sign flip the whole call (avoids polarity-driven whipsaw). Exception remains the Rug, where sign IS the pattern.
- **O · 5-step Analysis tab** — retrospective structured around the 5 steps with per-step effectiveness scoring (mirrors the live workflow).
- **LLM refinement loop** — pooled hit-rate-vs-baseline + MFE/MAE + n per detector/verdict/step ⇒ concrete threshold-tuning recommendations.
  - DECISION (user delegated -> agent rec, LOCKED): OFFLINE, HUMAN-IN-THE-LOOP, BATCH-REVIEWED — never a live auto-tuner.
    - Cadence: periodic (weekly or after N logged fires), NOT continuous.
    - Mechanism: export accumulated effectiveness log + saved skylit-docs archive to an LLM; it proposes threshold/rule adjustments WITH the specific misses/hits that justify each (e.g. 'Gatekeeper edge-to-middle 2.2 not 1.8, here are the 6 misses').
    - Gate: proposals return as a REVIEWABLE DIFF/changelog -> user approves -> applied as a normal versioned build (same test-and-ship ritual). LLM NEVER writes the live script and NEVER auto-adjusts thresholds in production.
    - Principle: compute/log early, assert late, HUMAN approves the change.

### v10.27+ — API-UNLOCKED REFINEMENTS (needs Skylit API key)
- Clean node classification/velocity, SPX in-app, /v1/historical, formal cross-symbol Trinity gate, GEX/VEX overlap zones, event/OPEX/Power-Hour flags.

---

## CROSS-CUTTING THEMES
- One shared **bar-close-commit** helper powers all stability (D, H, G).
- The **King-header region** (A+B+F+G) ships as one coherent layout unit in v10.23.
- **Per-node polarity (pos/d)** — confirmed present; ONE-TIME verify vs ladder color unlocks Rug + polarity verdicts.
- **Effectiveness tracker** is the backbone: capture in v10.24, analyze in v10.26; never assume a rule works — measure it.

## OPEN CONFIRMATIONS BEFORE CODING
- Mockups approved: King header (rev4), Node Map (rev2), full panel. Remaining sign-offs: verdict wording (Bounce/Pullback/Break-through), ★/emphasis markers, crown-on-dot label.
- Docs mining: Ten Commandments + Speculative/Decoy case study not yet folded into detector detail (decoy discount).
- AWAITING explicit "start development" — nothing built until then.
