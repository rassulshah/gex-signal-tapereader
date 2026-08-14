# GEX Signal Tapereader — Product Roadmap

_Last updated: 2026-08-13 · Current shipped: v10.29_

Board format: **NOW** (in build) · **NEXT** (committed / requested) · **LATER** (candidates) · **EXPLORING** (infra research).


---

## NOW — PRIORITY (v10.30+): HARDEN THE DASHBOARD DETECTORS *before* metric review

### 🔬 Detector correctness pass — Double-Stack / Cluster / Rug / Reverse-Rug (v10.30)
**User priority 2026-08-13:** validate the Dashboard pattern DETECTORS are computing correctly BEFORE any Analysis-tab metric refinement — no point scoring a detector's hit-rate if the detector itself is wrong.
- **SYMPTOM (user-observed):** far too many **Double-Stack** flags — showing on multiple strikes at once when a real "double stack" should be rare (2 adjacent genuinely strong nodes). Suggests the DStk threshold (STACK_GAP / significance) is too loose or the adjacency test is wrong.
- **TASK:** audit each of clusterDetect / doubleStackDetect / rugDetect (Rug + Reverse-Rug) against the Skylit doctrine definitions + real tape; tighten thresholds; add/￼extend unit tests with realistic fixtures so over-firing is caught.
- Only AFTER detectors are trustworthy → proceed to the metric review below.

### 📊 Analysis-tab metric review (v10.31, AFTER detector hardening)
**User request 2026-08-13:** go over the Analysis tab and review/refine ALL its metrics — direction hit-rate, per-signal accuracy (King/Trend/SRB/Conf), King-target hit, reversal-catch, confluence-lift matrix, accum/combined edge, day-regime, MFE/MAE. Confirm each is measuring what we intend, on clean captured data, and refine wording/thresholds. Capture layer is verified complete (v10.29); this is the readback/scoring refinement.

---

## NOW — in build (v10.15 → v10.16)

### 🧠 Self-improving review loop (Stage 1 done)
The engine that makes the app get sharper over time instead of being eyeballed after the fact.
- **DONE** Signal-vector capture — every closed 3-min bar records the full derived read: trend {state, up/win, ma, slope}, King {cls, word, drift, score, magnet, offset}, srBattle {dom, cross, sup/res force, floor/ceil + fade flags}, node breadth, confluence.
- **DONE** Forward-outcome auto-labeler — stamps what price actually did over the next 5 bars (15 min) and 10 bars (30 min): MFE, MAE, net, ending price, King touch, reversal up/down. Both horizons stored so the ~1-bar reversal lag is measured, not guessed.
- **DONE** Daily export — self-describing `gex_YYYY-MM-DD.json` (schema + legend + horizons) + footer "📥 Save Day" button. AI Drive inbox: `/GEX-Signal-Tapereader/daily-data/`.
- **IN BUILD** Auto-save at session close (Option A: script auto-triggers the day's download near close; user confirms browser save → lands in Drive). Paste-to-chat path also supported (agent writes to Drive + runs review).
- **IN BUILD** Scheduled end-of-day LLM review → writes a dated scorecard to Drive.
- **IN BUILD** In-app **Analysis tab** rendering the review (7-step narrative — see design).

### 👑 King coherence pass (shipped in v10.15)
- Stacked King/price header badge (King strike over SPY price + signed offset).
- Per-segment sparkline coloring with trend hysteresis.
- Section coherence across King · Trend · S/R · nodes.

---

## NEXT — requested features

### 📏 HOD / LOD Tracker — target v10.17
Live high-of-day / low-of-day tracked as reference rails.
- Surface HOD/LOD as horizontal reference levels alongside King & walls.
- Feed the predictive/target logic: distance-to-HOD and distance-to-LOD as target candidates.
- Flags: failed breakout (poke above HOD then reject), range expansion (new HOD/LOD after compression), range-bound (price trapped between HOD/LOD with King mid).
- Mark HOD/LOD events on the King relationship timeline (Analysis tab Step 1).
- **Insight it adds:** is price extending or trapped? Where is the liquidity magnet (HOD/LOD) relative to the settlement magnet (King)?

### 🌐 Multi-Symbol Support — Crude (CL) & Gold (GC) — target v10.18
Run the same King / S-R / node engine on markets beyond SPY/QQQ.
- Per-symbol config: strike/level spacing (CL ~0.25, GC ~1.0), ATR band multipliers, trend MA length, session hours.
- Header **symbol switcher**; recorder + slices keyed per symbol (already partially keyed).
- Per-symbol calibration of node granularity so "Building/Fading" thresholds mean the same thing across products.
- **Dependency:** a per-symbol tape/data source for CL & GC (confirm feed availability in Skylit for these).
- **Insight it adds:** apply the whole coherence framework to crude & gold intraday.

### 📤 Export GEX Levels → IRT — target v10.19
Push computed GEX levels out to IRT (charting/trading platform) as drawn levels.
- Export set: King, walls, nearest floors/ceilings, HOD/LOD.
- Delivery: one-click clipboard copy and/or file export in IRT's level-import format; optional auto-refresh on King roll.
- **Open question:** IRT import format — CSV, clipboard paste, or API? This shapes the connector. (Confirm with user.)
- **Insight it adds:** trade directly off the levels on your existing chart without re-typing them.

---

## LATER — candidates
- **🔔 Alert push** — crossover ("bears/bulls taking over") and confluence-alignment alerts (desktop/sound/webhook).
- **📅 Multi-day backtest** — run the LLM review over N archived days for regime-level pattern discovery and threshold calibration.
- **⚙️ Tunable thresholds UI** — expose MA length, trend window, dominance %, band multipliers for live tuning.
- **🕯️ Session profiles** — RTH vs overnight handling (needed once futures like CL/GC are in).

---

## EXPLORING — infrastructure
- **Zero-touch Drive sync** — small authenticated relay endpoint so the browser script can push the daily archive with no click (replaces Option A). This is the real fix for the "userscript can't reach Drive" limitation.
- **Cross-device recorder history** — persist recorder beyond one browser's localStorage so history survives device changes and the 10-day rolling cap.

---

## Notes / decisions log
- Review horizons locked at 5-bar (15m) + 10-bar (30m), stored side-by-side.
- Review focus (per user): predict direction, S/R, buy/sell signals, targets, pullback reversals, trend reversals; explain the *why* behind wins/losses; improve the app & its features; discover trader-useful patterns; make ranked recommendations.
- Analysis tab must show the node / S-R / price / King *relationship* coherently (timeline, King↔price convergence, multi-node lifecycle, confluence-outcome matrix, lead/lag).

---
## ROADMAP CANDIDATE (added 2026-08-13, from Article-2 walkthrough — Intro to Gamma)

### Net Gamma Regime + Polarity-Cross point
Source of truth: Skylit Academy — "Intro to Gamma" (Absolute Value Rule; regime-
transition = best setups) and "Gamma Regime Awareness".

WHAT: Determine the CURRENT net gamma regime — are we in a POSITIVE-gamma regime
(dealers hedge contrarian → friction/mean-reversion/chop) or a NEGATIVE-gamma
regime (dealers hedge pro-cyclical → fuel/acceleration/overshoots)? Then mark the
POLARITY-CROSS point: the strike where the board flips from net-positive to
net-negative relative to price. Academy says crossing that point in advance = edge.

WHY: Article 2 Part 3 is currently NOT built. We already carry per-node polarity
(w.pos), so we have raw material to compute a net-gamma read and locate the cross,
but we do not surface either today. User's framing: this is essentially the
"which gamma regime are we in" question and should drive how the whole map is read.

OPEN QUESTION (verify later): does the Skylit feed actually expose the data needed
for a reliable NET gamma reading (e.g. signed exposure per strike, or an aggregate
GEX/zero-gamma/flip level)? We have per-node sign+magnitude; need to confirm whether
that is sufficient for a trustworthy net read or whether a dedicated feed field
(e.g. zero-gamma / gamma-flip level) is required. TABLED pending feed audit.

STATUS: tabled / not started. Relates to existing gexRegime() (structural regime
coloring) — decide whether this net-gamma read extends gexRegime or is a new signal.

### UPDATE (2026-08-13, from Article-4 walkthrough — Gamma Regime Awareness)
The scope of this item is BIGGER than first framed. Three parts, ALL gated behind
the feed audit below:

(a) RE-BASE the regime classifier on GAMMA SIGN. Academy defines regimes by dealer
    POSITIONING (+/- gamma), "not price direction." Our current gexRegime() classifies
    by NODE-MASS GEOMETRY (skew = Trend; hollow-middle = Whipsaw; interleaved = Rainbow).
    That is a PROXY, not the Academy's definition.
(b) ADD the missing "RANGE" label + fix a NAMING INVERSION. Academy's Range Day =
    positive-gamma / fade-the-edges. We emit no "Range" label at all, and our current
    "Whipsaw" label is closest to the Academy's RANGE day — confusing. Reconcile labels.
(c) POLARITY-CROSS point (original item) folds into the same workstream.

RISK NOTE: gexRegime() is LOAD-BEARING — it colors how the entire map is read. This is
a RE-BASING of shipping behavior, not an additive feature. Must be its own build batch
WITH tests and explicit user sign-off. Do NOT rewrite casually.

BLOCKING DEPENDENCY: the feed audit (does Skylit expose data for a reliable NET gamma
read?). If YES -> build the re-base. If NO -> keep the geometric proxy but RENAME labels
to stop contradicting the Academy (small, safe change).

### FEED AUDIT RESULT (2026-08-13) — de-risks the regime re-base
Endpoint: app.skylit.ai/tv/api/gex/levels. Per-strike raw fields: k (strike),
v (exposure value), d (DIRECTION/sign), net (change).

FINDINGS:
1. LIVE feed HAS sign (d -> our w.pos). Net-gamma read is FEASIBLE live. Not blocked.
2. RECORDED snapshots DROP sign. Captured node keys = k,role,side,pct,tp,st,net,
   rapid,rdir,roll,hist — NO pos/d. => cannot BACKTEST a gamma-sign regime on our
   history; and any LLM/Analysis polarity work is impossible until this is fixed.
3. UNVERIFIED ASSUMPTION: that sum(v*sign) across strikes = meaningful NET gamma /
   maps to the Academy's +/- gamma day. Feed may be normalized/top-N/scaled. A true
   net read may want an aggregate signed GEX or a zero-gamma/flip level we don't have.

DE-RISKED SEQUENCING (do in this order):
  STEP 1 (cheap, safe, HIGH VALUE): fix recorder schema to log pos/d per node.
          Starts accumulating sign-carrying history. No UI risk.
  STEP 2 (after ~days of clean capture): MEASURE whether summed net-gamma actually
          separates trend vs range days in OUR data. Empirical validation.
  STEP 3 (only if STEP 2 confirms): re-base gexRegime on gamma sign + add Range label.
          Its own batch, tests, sign-off. Else: keep geometric proxy, just rename labels.

RECOMMENDED IMMEDIATE ACTION: STEP 1 (recorder schema) — unblocks everything, near-zero risk.

---
## NEXT BUILD — COMMITTED (user-approved 2026-08-13)

### [COMMITTED] Recorder schema: log node polarity + signed value (unblocker)
Add per-node SIGN to the recorded daily snapshot so net-gamma work + backtesting
become possible going forward.
- Add `pos` (polarity, from w.pos / feed d) to each recorded node.
- Ensure the SIGNED per-strike value is captured (magnitude + sign), not just pct,
  so sum(signed exposure) = net-gamma can be tested later.
- DATA-LAYER ONLY: no Dashboard/detector/regime changes. Near-zero risk.
- Add a test asserting recorded nodes carry pos (and signed value).
- NOTE: backtest is FORWARD-ONLY — existing history (day_811 etc.) lacks sign and
  cannot be retro-tested. Clock starts the day this ships.
This is STEP 1 of the Net-Gamma-Regime workstream (see FEED AUDIT RESULT above).

### [COMMITTED] Beach Ball detector (Academy pattern — currently MISSING)
Source of truth: Skylit Academy — "Heatseeker Patterns / The Beach Ball."
Academy: price OVERSHOOTS a significant POSITIVE node, punches through, then REVERTS
("beach ball underwater"). NOT a breakout — a STRETCH, not a break; node is absorbing,
not failing. Common mistake = chasing the break. Trade = the REVERSION back through the
node after the overshoot exhausts.

WHY: it is a named Academy pattern with ZERO implementation today, and it is the
direct COUNTER-CASE to our existing 14-bar breakout (BO) gate — a poke through a strong
+gamma node that FAILS to post a real breakout and reverts.

BUILDABLE NOW (no new data source):
- We already have: closedCandles, wick/touch detection (ABS_TOUCH_TOL, wickTouch),
  nodeBuildRate, and the 14-bar extreme gate (isNBarExtreme / BO_HL_LOOKBACK).
- Detector sketch: a significant +gamma (Pika) node is overshot (price trades beyond it
  by > deflection tolerance) but the bar FAILS the 14-bar breakout gate (no new extreme /
  no follow-through) -> flag Beach Ball = expect reversion back through the node.
  Emit as a structural read (reversion risk), NOT a buy/sell signal (Charts-First posture).

KNOWN LIMITATION (be honest in the tooltip): the feed/candles carry NO VOLUME, so the
Academy's "volume spikes at overshoot then dries up" cue is NOT observable. We detect the
PRICE-STRUCTURE half (overshoot + failure-to-follow-through + reversion) only. Document
this so the pattern isn't oversold as full-confidence.

TESTS: add test_beachball.js — overshoot+fail -> Beach Ball; clean 14-bar breakout -> NOT
Beach Ball; overshoot of a -gamma (Barney) node -> NOT Beach Ball (Academy: +node only).

### FINDING (2026-08-13) — VOLUME/CVD reachable but not yet scraped
Verified via live DOM probe: the candle prop we scrape carries ONLY
time,open,high,low,close — volKeys (NONE). BUT the chart DOES render "Volume · Total"
and "CVD · Session · MA 20" as SEPARATE indicator series/panes. So volume + CVD
(buy/sell pressure) exist in the TradingView fiber tree; our scraper just stops at
the OHLC candles prop and never reaches them.
ROADMAP CANDIDATE: widen the fiber scraper to capture the volume + CVD series.
- Would enable the Beach Ball "volume dries up" cue (currently caveated as unobservable).
- CVD = cumulative buy-minus-sell delta = the order-flow signal we wanted.
- Caveat: named studies may be pre-computed indicator outputs, not raw per-bar volume.
STATUS: candidate, not committed.

### CLEANUP TODO (do before next real release)
Debug probes (__feedProbe/__candleProbe/__PROBE_RESULT + footer title + __gptsDebug.PROBE)
are still in current/. STRIP them before shipping the next clean build. current/ is
presently a DEBUG build, NOT clean v10.31.

---
## ACADEMY CONSISTENCY SCORECARD — all 11 articles (completed 2026-08-13)
Full article-by-article audit of the app vs Skylit Academy (SOURCE OF TRUTH).

BUILT & FAITHFUL:
- Art 1 Charts First .......... posture ✅ (confirmation-not-signal); ⚠ no chart-structure input
- Art 2 Intro to Gamma ........ ✅ polarity + Absolute Value Rule (core); ❌ polarity-cross point
- Art 5 Patterns ............. ✅ Rug / Reverse Rug / Pika / Barney; Beach Ball COMMITTED (next build)
- Art 8 Air Pockets/Velocity .. ✅ velocity (rapid flag) + Air Pocket BUILT (v10.32)
- Art 11 Graduate ............ ✅ consistent by design (tool = confirmation + review loop)

PARTIAL / DIVERGES:
- Art 3 Reading Map .......... ✅ King & Gatekeeper (best); ⚠ floor/ceiling proximity (INTENTIONAL, approved); Air Pocket now built
- Art 4 Gamma Regimes ........ ⚠ regime is GEOMETRY-based not GAMMA-SIGN-based; no "Range" label. Re-base candidate (gated on feed audit — DONE) + recorder history
- Art 6 Execution Doctrine ... ⚠ tap DETECTED but no tap-probability DECAY (80/66/33); deflection zone is 0.20 not ±0.50; no 3:1 R:R / A+ grading (largely out-of-scope by Charts-First posture)
- Art 7 Rolling Floors/Ceil .. ⚠ kingRoll = King INTRADAY drift, not floor/ceiling DAY-OVER-DAY (needs recorder history)

LARGELY NOT BUILT:
- Art 9 Node Lifecycle ....... ❌ Fresh/Tested/Delivered/Decaying + Real-vs-Hedge (grows/fades) + stairstepping. Academy marks this "KEY". Least consistent. (code self-admits: freshness 'no touch counter yet')
- Art 10 Trinity Mode ........ ❌ no cross-index SPX/SPY/QQQ alignment (3/3=A+, 2/3=size-down, divergent=wait). Single-index tool today.

KEYSTONE INSIGHT: Art 4 (net-gamma regime), Art 7 (day-over-day rolling), and Art 9
(Real-vs-Hedge lifecycle) ALL depend on the SAME foundation — cross-session recorded
history carrying polarity + signed magnitude. That is the COMMITTED recorder-schema fix.
One data-layer change unblocks the three biggest doctrine gaps.

SUGGESTED PRIORITY (post current committed build):
1. Recorder schema (COMMITTED) — keystone, unblocks Art 4/7/9.
2. Node Lifecycle freshness/tap-count (Art 9) + tap-probability decay (Art 6) — pure structural facts, high value, Charts-First safe.
3. Real-vs-Hedge growth classifier (Art 9) — after recorder accumulates days.
4. Trinity cross-index confluence (Art 10) — bigger lift, needs multi-index plumbing.
5. Net-gamma regime re-base + "Range" label (Art 4) — load-bearing, needs validation days.
6. Polarity-cross point (Art 2), day-over-day rolling (Art 7) — fold into the above.
