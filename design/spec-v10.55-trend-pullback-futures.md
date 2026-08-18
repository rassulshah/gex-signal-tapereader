# v10.55 — TREND / MAGNET / PULLBACK-NODE ENGINE · ROLLING · FUTURES MODE · ENGINE-READY DATA · QQQ parity · SPXW confluence

Base v10.54. Version → 10.55 (3 spots). Invariants unchanged (one render(), `})();`, md5 parity, no key renames,
descriptive-only, enrollment intact, effective-n everywhere, no % without n).

## THE MENTAL MODEL (user-taught, 2026-08-18 — this governs the code and the READ vocabulary)
A trend, in node terms, is an alternation:
- **MAGNET** — the node price is DRAWN TO. In a downtrend it is the heavy node BELOW; price "rallies down" to it (RLY).
- **PULLBACK NODE (PB)** — the node that FORMS on the counter-move and price DEFLECTS off. In a downtrend it forms
  ABOVE price, and it is the resistance a trader sells from. In an uptrend it forms BELOW = the support to buy from.
- Sequence in a downtrend: RLY down to magnet → PB node forms above → deflection off it → RLY to the NEXT magnet →
  a NEW PB node forms, LOWER than the last. i.e. lower-low (magnet) / lower-high (PB), each governed by a node.
  Uptrend = mirror (higher-low magnet / higher-high PB).
- PB nodes APPEAR AFTER the move, not before. So the engine PREDICTS the ZONE ("a PB node will form above, below the
  last one — sell level"), then DETECTS the strike the moment it lands, then flags the ROLL.
- The 50-SMA confirms the trend; ROLLING CEILINGS (downtrend) ARE the successive PB nodes; rolling floors (uptrend)
  are the successive PB supports. Rolling detection and PB detection are ONE mechanism seen from two angles.
- Doctrine: ceiling rolling down = strong presumptive evidence of a bearish thesis; floor rolling up = bullish;
  2 consecutive migrations = signal, 3 = confirmation; a rolled-off level LOSES target status (the new one has it);
  a rolling floor leaves an AIR POCKET behind (fast travel through the vacated zone).

## PART A — TREND/MAGNET/PB ENGINE (`legEngine(sym)`), computed once per bar, cached with spineOf
Inputs: trendVerdict five-state (direction owner), nodeMapModel (levels with pct/pos/state/dist), FCHIST + a
new per-bar node-cluster history NODEHIST[sym] (nearest meaningful ceiling ABOVE and floor BELOW price, per bar).
State machine per sym: `{ dir:'dn'|'up'|'none', phase:'RLY'|'PB'|'none', magnet:{k,pct}, lastPB:{k,pct,t},
  pbZone:{lo,hi} , predictedPB:bool, pbDetected:{k,t,rolledFrom}|null, roll:{side:'ceil'|'flr', steps:[k...],
  count, confirmed:bool}, invalidations:{trendBreak, pbBreak} }`
Rules:
- dir from trendVerdict: 'dn' when state dn or dn-broken(with prior dn), 'up' mirror, else 'none' (engine idles).
- MAGNET (dn) = the strongest meaningful node BELOW price within reach (largest |pct| among nodes below, prefer
  King if below; cap at the King). RLY phase while price is moving toward it and no PB node has formed above since
  the last leg.
- PB ZONE (dn) = between price and the last PB node (or the last ceiling); predictedPB=true while in RLY and the
  zone is empty of a meaningful node. Rendered as a prediction: "expect a PB node to form above, below <lastPB>".
- PB DETECTED (dn) = a meaningful ceiling (>= PB_MIN_PCT of King, default 20) APPEARS or GROWS (rocNow Building)
  inside the pbZone after a down leg, AND it is LOWER than lastPB (or lastPB is null). Set pbDetected, phase='PB',
  lastPB=that node, roll.steps.push(k). Emit alert.
- ROLL: roll.count = number of consecutive LOWER PB nodes (dn) / HIGHER (up); count>=2 → 'signal', >=3 →
  'confirmed'. A PB node that forms HIGHER than lastPB in a downtrend breaks the roll (count resets) and is a
  caution ("PB rolled UP — trend weakening").
- INVALIDATION: trend flips (SMA state) or price closes THROUGH the current PB node in the trend direction's
  counter-side (dn: close above the PB node) → phase 'none', pbBreak flag; a break above the last PB node in a
  downtrend is a descriptive "lower-high broken".
- Rolled-off levels: any ceiling above the current PB node (in dn) is tagged `roledOff:true` — it loses tgt/inval
  status; the CURRENT PB node is the resistance; the magnet is the tgt. Vacated zone between the old and new PB
  node is tagged air (fast travel) per doctrine.
Uptrend = exact mirror with floors/supports.

## PART B — SURFACING (READ · zones · decision · direction) — user's vocabulary
- READ (3-beat, unchanged style) gains the leg language when the engine is active:
  dn/RLY:  "Downtrend (SMA). Rallying down to magnet 773. Expect a pullback node to form above, below 775.5 — sell level."
  dn/PB:   "Pullback node formed at 775.0 — rolled lower from 775.5 (2nd step, signal). Resistance to sell from;
           deflection expected. Potential drop to magnet 773."
  roll confirmed: "...(3rd step — confirmed downtrend)."
  up = mirror ("buy level", "support to buy from", magnet above).
- Direction grade: add `roll` as a factor INSIDE the trend-primary hierarchy: a confirmed roll in the trend
  direction = +1 to score (never changes direction, which the SMA owns); a PB that rolls AGAINST the trend = −1
  and relation gets a 'weakening' note. Rendered in the direction hover.
- Deflection zones: the current PB node row is marked `PB` (pullback node) with its roll step ("PB · 2nd lower");
  the magnet row is marked `MAG · target`; rolled-off rows are dimmed with "rolled off". Zone in-play logic
  prefers the PB node when price is retracing to it.
- Decision at a PB node in dn: "sell-side deflection · tgt magnet 773 · inval above PB 775.0" (descriptive words
  only — "sell level"/"buy level" are LEVEL DESCRIPTIONS matching the user's vocabulary, never an order; keep the
  matrix labels as-is; the R:R gate applies).
- Alert: when pbDetected flips true → a one-line highlighted banner under the READ for that bar
  ("⚑ Pullback node formed 775.0 — rolled lower") + optional sound if CFG allows (reuse existing alert plumbing).

## PART C — RECORD + SCORE (auto-enrolled)
New FEATURES: `leg.dir` (dir/phase), `leg.pbPredict` (predictedPB with zone; outcome: did a PB node form in the
zone within fwd? = hit), `leg.pbDetect` (k, rolledFrom, step; outcome: did price deflect off it (frame tgt-before-inval
toward magnet)? + MFE/MAE), `leg.roll` (count, side; outcome: did trend continue DIR_PTS after a 2nd/3rd step?),
`leg.magnet` (k; outcome: was it reached?). All with regime + effN, and vote-split rows in the Direction-factors table.
Backtest against FCHIST/day files for 2026-08-17 in a test fixture (the user's circled 776 → 775.5 → 775 sequence must
be recovered by the engine from recorded data: test_leg_engine.js).

## PART D — ROLLING as a first-class factor (multi-session + intraday)
- Intraday: as in PART A (PB roll steps).
- Multi-session: from FCHIST — Flr/Ceil strike today vs prior sessions; 2 consecutive sessions higher floor =
  bullish signal, 3 = confirmed (mirror for ceilings). Recorded `roll.session`. Voting: same +1/−1 inside the
  hierarchy once FCHIST has >=3 sessions; before that record only ("needs 3 sessions, have N" in Testing coverage).

## PART E — FUTURES MODE (auto-detect from the chart; only that instrument's levels)
- Detect the chart symbol from the page (title `ES1 $...`, header ticker) each render: SPY | QQQ | ES/MES | NQ/MNQ | other.
- Mapping: ES,MES → underlying SPY tape · NQ,MNQ → underlying QQQ tape · SPY→SPY · QQQ→QQQ · anything else →
  panel shows "No options tape for <SYM> — levels unavailable" (NEVER invented levels).
- Live ratio: r = futPrice / underlyingPrice, futPrice from the header ticker/title, underlying from the feed's
  price field (`levels[].s`) or last known; EMA-smoothed; if the fut price is unreadable fall back to the last
  good ratio then the constant (ES 10.0676 / NQ from last known) and mark every converted level with `≈`.
- ALL displayed levels convert: King, SUP/RES, gate, zone strikes, magnet/PB, drift centres, frame entry/tgt/inval,
  R:R and distances in futures points; zone widths scale by r (0.50 SPY → 5.0 ES). SHOW ONLY the futures value
  (no SPY strike alongside — user decision).
- Underlying price/candles when the chart is a future: reconstruct SPY(QQQ) price = futPrice / r and convert the
  futures chart candles by 1/r so trendVerdict, in-play, drift, R:R keep working. Recording stays in the
  underlying's strikes (learning unaffected). Footer shows `ES/SPY 10.071 (live)` or `(≈ const)`.
- ⚙ gets a "Futures: auto | force SPY | force ES | force NQ" override.

## PART F — ENGINE-READY DATA + extra predictors
- `buildFeatureMatrix(day)`: one row per bar per sym, columns = every recorded feature vote/grade/input + regime +
  session + model stamp; labels = the four outcomes (dirHit, deflHit, contHit, tgtHit) + MFE/MAE. Written into
  the day export as `matrix` (CSV-able) so a model is one script away.
- New recorded predictors (non-voting): timeToClose (mins), barOfDay, distToKing (pts), distToMagnet, pbActive,
  rollCount, sessionRangePos, dayNet, prevDayClose rel (PDC above/below), and `event` from an EVENT_TAG the user
  can set in ⚙ (FOMC/CPI/OPEX/half-day) — replaces the always-false event.

## PART G — QQQ parity + SPXW confluence
- Everything computed for SPY (spine, zones, leg engine, features) runs for QQQ too when QQQ is the active
  underlying (Futures NQ or chart QQQ). Recording/analysis keyed by sym everywhere.
- SPXW: parse the SPXW tape from the trinity sidebar (strike ladder) into a wall map like SPY/QQQ; `S` confluence
  becomes SCORED (+1 in nodeGrade like Q) when available; honest `S–` otherwise. Test with a synthetic ladder.

## Tests
test_leg_engine.js (state machine on the 08-17 fixture: RLY→PB detect at 775.5→roll to 775.0→confirmed; mirror
uptrend; PB rolling against trend resets; invalidation on close through PB; magnet capped at King),
test_futures_mode.js (detect ES/NQ/other, mapping, ratio live/fallback with ≈, all levels convert, unavailable
state, candles reconstructed, recording stays in underlying), test_roll_factor.js, test_feature_matrix.js,
test_spxw_confluence.js, test_qqq_parity.js. Version pins → 10.55. Full suite: only the 4 known-stale may fail.
