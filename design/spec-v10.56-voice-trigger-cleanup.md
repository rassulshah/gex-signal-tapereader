# v10.56 — the READ voice (user-authored) · dissipation HANDOFF detection · latched ✓/✗ trigger · clean in-play card · drift arrows · steps centered · sync grace

Base v10.55. Version → 10.56 (3 spots). Invariants unchanged (one render(), `})();`, md5 parity, no key renames,
descriptive-only, enrollment intact, effN, no % without n).

## A. THE HANDOFF (priority — user: "the key is identifying the dissipating old ceiling and the new one forming")
The roll is a STRENGTH transfer before it is a strike change: the old ceiling's %King bleeds (Dec) while a lower
node above price builds (Acm). Detect it a bar or two BEFORE the new PB fully qualifies.
`legEngine` gains `handoff:{active, from:{k,pctNow,pctPeak,dec15m}, to:{k,pctNow,acm15m}, t}`:
- from = the current lastPB / nearest ceiling above (dn) whose accumCanon m15 is Dec (<= −HANDOFF_DEC, default −8%)
  or whose pct has fallen >= HANDOFF_DROP (default 25%) from its session peak;
- to   = a node above price, LOWER than `from`, with m15 Acm >= HANDOFF_ACM (default +8%) or pct >= PB_MIN_PCT;
- active while both hold; resolves to pbDetected when `to` qualifies as a PB (existing rule) → then `to` is the
  PB, `from` is rolledOff. Uptrend = mirror (floor: from below, to = higher floor building).
Recorded as `leg.handoff` (record {from,to,pcts}; outcome: did `to` become the PB within fwd AND did price deflect
off it toward the magnet? + MFE/MAE). Vote-split row in Direction factors.

## B. THE READ VOICE (verbatim, user-authored 2026-08-18) — leg voice LEADS when a leg is active
Direction word/grade still from the spine (`↓ Downtrend C` / `↑ Uptrend B`), caps become a trailing caveat
("Chop, mid-range — low confidence."). Levels are plain numbers (no "magnet" word in the sentence).
DOWNTREND
1 rallying (no PB yet):        "Downtrend. Rallying down to 768. Expect pullback node to form from 771 ceiling rolling down."
2 HANDOFF (A active):          "Downtrend. Rallying down to 768. 771 ceiling dissipating and rolling down to form pullback node at 769."
3 PB formed:                   "Resistance pullback node formed at 769. Deflection expected to target 768."
4 pulling back into PB:        "Pulling back to resistance pullback node 769. Deflection expected to target 768 below."
5 deflected, next leg:         "Deflected off 769. Rallying down to 768. Expect pullback node to form from 769 ceiling rolling down."
6a stacking (old holds):       "Pullback node 769 holding. New resistance forming above at 770 — resistance stacking."
6b rolling up (old dissipated):"Pullback node 769 dissipated. New pullback node formed higher at 770 — ceiling rolling up."
7 target hit:                  "Rallied down to 768 target. On watch for a pullback."
UPTREND (mirror; note #2 uses "building", not "dissipating")
1 "Uptrend. Rallying up to 772. Expect pullback node to form from 768 floor rolling up."
2 "Uptrend. Rallying up to 772. 768 floor building and rolling up to form pullback node at 769."
3 "Support pullback node formed at 769. Deflection expected to target 772."
4 "Pulling back to support pullback node 769. Deflection expected to target 772 above."
5 "Deflected off 769. Rallying up to 772. Expect pullback node to form from 769 floor rolling up."
6a "Pullback node 769 holding. New support forming below at 768 — support stacking."
6b "Pullback node 769 dissipated. New pullback node formed lower at 768 — floor rolling down."
7 "Rallied up to 772 target. On watch for a pullback."
State→sentence: RLY&!handoff→1 · handoff.active→2 · pbDetected&price not in PB zone→3 · price in PB zone (approach/contact)→4 ·
just deflected (latched ✓) & new leg→5 · new PB higher(dn)/lower(up) with old holding→6a · with old dissipated→6b · magnet
reached→7. 6a/6b distinguished by the old PB's dissipation (same test as A). No leg active → the v10.54 3-beat voice.
Test: test_read_voice_leg.js pins all 15 sentences from state fixtures.

## C. THE LATCHED ✓/✗ TRIGGER (replaces the per-bar reactionQuality on the card)
`deflTrigger(sym, node)` is a per-setup LATCH keyed by (sym, node.k, legId):
- state ∈ {blank, '✓↑', '✓↓', '✗'}; evaluated ONLY on CLOSED bars (never intrabar).
- ✓ latches when: price touched the node zone (DEFLECT_ZONE) AND the closing bar shows rejection — for a resistance
  PB (dn): bar high >= k−zone AND close < k−zone AND close < open (wick rejection + close back away) → '✓↓'.
  Mirror for support → '✓↑'.
- ✗ latches when: a bar CLOSES through the node against the setup (dn: close > k+zone) before ✓ → '✗'.
- Once latched it does NOT re-evaluate. Reset ONLY on: new legId (leg engine phase change to a new leg / new PB),
  or price leaves the zone by > 2×zone for >= 3 closed bars with no ✓/✗ (setup abandoned).
- Rendered on the in-play row 1 as a bold `✓↓` / `✓↑` / `✗`; blank while unresolved. Hover: "Has the deflection
  occurred? ✓ = rejection candle closed away from the node (the go signal in that direction); ✗ = closed through it.
  Latched on bar close — does not flicker."
- Recorded: `defl.trigger` {k, state, dir, latchedBar}; outcome: after ✓, did price reach tgt before inval
  (frame) + MFE/MAE; after ✗, did the break follow through. This is the deflection hit-rate the loop measures.
- Keep reactionQuality as an internal input/hover ("reaction now"), NOT the visible mark.

## D. IN-PLAY CARD (visible fields — user-decided)
Row 1: `● (polarity colour) · strike · role · leg tag (MAG / PB · 2nd lower / rolled off) · TRIGGER (✓↓/✓↑/✗/blank) · GRADE`
Row 2: `S✓ Q✗ V✓ · decision · tgt X · inval Y` + TAKE/PASS — frame + buttons ONLY when tradeable (decision not
skip/stand-aside AND R:R gate passes AND in contact). On skip: `S✓ Q✗ V✓ · skip` only.
REMOVED from visible: R:R text (gate still applied silently; ratio in hover), %King, polarity text, tap, ▶ setup,
Acm, activity, entry — all in the row/grade hover (question-first). Other rows: dot · strike · role · leg tag · grade.

## E. Small UI
- Steps 1-5 line: text-align center.
- Drift: `G768.7↓ · V771.7↑` — arrow = that centre's side of price (↓ below / ↑ above), coloured red/green.
- Sync banner grace: show "⚠ Out of sync" only when tapeSync has failed on >= 2 consecutive checks (~30s); a
  single failed check is silent (still logged). Test: one failure → no banner; two → banner.

## F. Docs
master-spec §24 gains the handoff + the 15 sentences; LLM brief LEG section adds `leg.handoff` and `defl.trigger`
evaluation (✓ hit-rate by roll step, handoff lead time before pbDetected, ✗ follow-through).

## Tests
test_handoff.js (dec old + acm new → active; resolves to pbDetected; mirror), test_read_voice_leg.js (15 sentences),
test_defl_trigger.js (✓ on close only; never intrabar; ✗ on close-through; no re-eval once latched; reset on new
leg / abandonment; direction glyph), test_inplay_card.js (visible fields exactly; frame/buttons hidden on skip;
R:R absent from visible; S/Q/V present), test_sync_grace.js, drift arrows + centered steps assertions.
Version pins → 10.56. Full suite: only the 4 known-stale may fail.
