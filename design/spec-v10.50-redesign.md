# v10.50 BUILD — dashboard redesign (implements the full review + mockup)

Target UI: `mockups/gex-v10.50-full-redesign-mockup.html`. Base: v10.49.1.
Version → **10.50** in the THREE spots (header @version ~L4, part1 console.log, footer `feed v10.50`).
Invariants: one `render()`, final line `})();`, no key renames, v10.js == current/ (md5). Keep ALL feature
enrollment (recorder/analysis/testing/rules) intact — this is a DISPLAY redesign; recording continues.
**Nothing that produces a grade/verdict may be un-enrolled.** Question-first hovers on EVERY element.

## GLOBAL: question-first hovers
Every tooltip LEADS with the question it answers, then the answer. Rewrite all touched hovers this way.
Examples: Drift "Which way do GEX & VEX lean? …", Reaction "Is the bounce working right now? …",
Grade "How good is a deflection here? …", Footer "Is the tool healthy? …".

## READ (single direction voice) — rewrite readBlock44 output
- One block. Verdict word + Direction grade inline: `↑ BULLISH  B` (grade chip, NO ⚖).
- New sentence style (locked): three beats — WHERE · STATE+LEAN · POTENTIAL MOVE:
  - Bounce:  `At King 773. Support building with GEX and VEX leaning up. Potential bounce to 776.`
  - Reject:  `At Ceil 776. Resistance building with GEX and VEX leaning down. Potential drop to 772.`
  - Cont:    `Through Gate 774. GEX and VEX leaning up. Potential run to King 776.`
  - Split:   `Mid-range 772–776. GEX and VEX split — no clean lean, rotation likely.`
  - Fields from: engaged/nearest node (WHERE), Flr/Ceil build-state + driftRead (STATE+LEAN), tgt = next
    node in dir capped at King (POTENTIAL). Verdict word from directionGrade().dir (UP→BULLISH etc).
- NO invalidation clause in the READ (it lives on the decision line). NO regime chip. NO "King behind" line.
- Badges after the grade: session badge (dim normally; HIGHLIGHT only power/opex), model-heat (show only
  `model cold`, hidden otherwise). Human hover.
- REMOVE: standalone readHeadHtml direction line as a separate element (merge into this), readWhyHtml as a
  separate block (its content is the sentence), the legacy BULLISH/BEARISH body verdict (superseded).
- directionGrade() unchanged as the engine (still 4 inputs, hard caps); regime lives ONLY in its hover.

## DRIFT — one line + thin bar (driftLineHtml)
- Line: `↗ Drift UP·conf · G773.9 V775.0` (keep). Below it a THIN (~7px) bar: gold band = GVWAP±σ,
  purple band = VVWAP±σ, WHITE 2px price line. Map the band/px positions from the Flr..Ceil range.
- Hover (simple, no confidence clause): both above price → "Which way do GEX & VEX lean? Both above price
  — supporting higher prices."; both below → "…pressuring lower prices."; split → "…disagree, no clean
  lean, expect chop."; none → "Waiting on VEX."

## DEFLECTION ZONES — the SINGLE ladder (deflZonesBlock), retire the legacy ladder
- Section header `⚡ Deflection zones` — DROP the `· px` (price is the divider).
- IN-PLAY node (full, 3 rows):
  - r1: dot(polarity-colored) · strike · role · `%King · g · tap · ✓/✗` · GRADE (letter only, no ⚖).
       polarity = a colored `g`: YELLOW g (+γ, PAL.gold), PURPLE g (−γ, #a371f7). Reaction = `✓` (confirmed)
       / `✗` (weak) from reactionQuality; only when in contact. `▶ setup` chip when approaching (unchanged).
  - r2: `Acm 15m▲ session▲` · `S✓ Q✓ V✓` · ACTIVITY tag.
       Acm horizons RENAMED: now→**15m** (keep the ~5-bar window), day→**session** (since open). accumCanon
       returns {m15:{pct,label}, session:{pct,label}}. Confluence adds **S = SPXW** agrees (from SPXW walls
       if captured, else `S–`); order S Q V. ACTIVITY = the node's lifecycle tag (Pull/Push/Defl/BO·FT/BOw)
       from the setup/attraction state (port from nodeMapBlock's ACTIVITY column).
  - r3 (DECISION, folded in): `bounce play · entry 773 · tgt 776(air) · inval <772` + TAKE/PASS buttons.
       decision text = DECISION_MATRIX cell; entry = the node (descriptive trigger); tgt/inval/path from
       tradeFrame (air tag on tgt AND inval). TAKE/PASS gated: show only when grade≥B AND cell≠'stand aside'.
- OTHER nodes (one line each): dot · strike · role · `%King · g · tap` · ACTIVITY tag · grade. NO decision.
- Price divider `— SPY 773.4 —` between above/below. %King from feedStructMap (feed). DROP the sparkline.
- DROP the gray "Dir grade = … | Node grade = …" legend line.

## RETIRE (delete from the live render; keep functions defined if a test needs them)
- Node Map sentence (nodeMapSentence / its render). Legacy ladder ROWS in nodeMapBlock. The in-map
  strong-magnet ★SUP/👑/★RES header (top cluster is the only one). Step icons ①②③④⑤ everywhere (move each
  step's doctrine into the relevant element hover). Regime chip. The legacy "Deflections" section
  (deflectionBlock) — its history/scoring stays in Analysis only. The "↩ King behind" snapback line.
- Range chip → replace with an EXCEPTION flag: show `⚠ OUT · range redefining` ONLY when price is outside
  Flr..Ceil; nothing when inside.
- Air-pocket standalone line → REMOVE; ensure tradeFrame tags `(air)` on tgt and inval when a pocket lies
  in that direction (the concept is preserved in the frame).

## KEEP (as exceptions / health)
- Session badge (power/opex highlighted), model-heat (cold-only), pre-open brief (one line, pre-open only),
  footer = THREE health dots + version: `● feed  ● vex  ● rec   v10.50` (green live / amber waiting), hover
  "Is the tool healthy? feed live · vex capturing · recording on — any red = the read above may be stale."

## ENROLLMENT (unchanged, verify intact)
- All FEATURES still record + resolve with MFE/MAE. `dir` records the merged verdict; `node` records grade
  + the new inputs (incl. dominance, S/Q/V, m15/session). `decision` records the cell. `act` records take/pass.
- Analysis scorecards + rules.json + LLM brief unchanged. test_feature_enrollment must still pass.

## TESTS
- Update tests whose pinned strings change (readBlock44 sentence, drift, accumCanon key rename m15/session,
  zone rows). Add: test_read_voice.js (the 3-beat sentence per state), test_accum_canon.js update (m15/session),
  test_zone_row.js (g color by polarity, ✓/✗, S/Q/V, ACTIVITY, decision-fold, take/pass gate). Keep the 5
  known-stale as-is. Every other test 0 FAIL; new tests all PASS. Version pin in test_read_v1047 → 10.50
  (or retire that test if its sentence assertions are fully superseded — prefer update).
