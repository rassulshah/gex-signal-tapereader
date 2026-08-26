# ISSUES — compiled live 2026-08-27 (operator: "don't build, just compile issues for the next build")

Rules for this file: one entry per operator-raised issue, with what was OBSERVED, what the code
actually does (measured, not guessed), and the DECISION NEEDED before building. Nothing here is
approved for build until the operator picks an option. Numbered in the order raised.

## 1. 5M ROC "keeps changing" — rolling window vs bar-gated display
- OBSERVED (operator, 09:1x ET): the 5m ROC number changes continuously; expected it to update
  every 5 minutes.
- WHAT THE CODE DOES: displays Skylit's `delta5Min` verbatim from the velocity fiber — a ROLLING
  trailing-5-minute delta, recomputed every harvest tick (~5s). Same for 15M/60M columns. This is
  vendor-native behaviour (vendor-verbatim doctrine), not a defect.
- DECISION NEEDED: keep rolling (early warning of an in-progress build, but "breathes"), or gate
  the DISPLAY to freeze at each 5m/15m/60m bar close (stable readings, loses intra-bar warning),
  or show both (frozen reading + small live drift indicator). Applies to the profile ROC matrix
  and the NODES section rows alike.

## 2. Flag-post label overlaps rail value labels
- OBSERVED (operator screenshot, 09:2x ET, ES chart): the yellow flag-post at 7677 draws its
  strike label directly ON TOP of the 7670 label below it — two rail numbers stacked/overlapping
  under the track (RRUG band area, near EL 7674 / EH 7679, price pill at 7680).
- WHAT THE CODE DOES: flag-post labels ("name on top of the flag so it doesn't get overlapped",
  v14.6) are placed per-post; the rail's neighbour-label THINNING (LBLOK, v14.5) thins rail strike
  labels against EACH OTHER but does not treat the flag-post label as an occupant of the same
  label lane — so a post label and a neighbouring strike label can collide when strikes are dense
  (7670/7677 ≈ 7pt apart at this zoom).
- FIX CANDIDATE (not approved): include flag-post labels in the same collision lane as rail strike
  labels (one shared LBLOK pass), preferring the post's label and thinning the neighbour, or
  offset the post label to a second row when a collision is detected.

## 3. The node price is IN PLAY but invisible on the rail — pill occludes the post
- OBSERVED (operator + zoom, 09:23 ET): the 7690 node ($20M, 74%K, DEFENCE ABANDONING — the most
  important strike on the board at that moment) has NO visible flag-post on the rail; the rail
  shows only EL 7688 / EH 7693 and the pill. The NODES section and gamma profile both show 7690
  fine, so the pile data exists — the rail mark is what's missing.
- WHAT THE CODE DOES: flag-posts render BEHIND the pill by design (v14.6 z-order). When price is
  ON the node (the exact moment that matters most), the pill (~40px wide at this rail scale, 1 ES
  pt ≈ 20px inside the band) fully covers the post — so the interaction state (breathe/sonar/
  fracture) is hidden precisely when it is happening.
- FIX CANDIDATE (not approved): let the post's top flag + label protrude ABOVE the pill (posts
  taller than the pill's lane), or render the in-play post's outline OVER the pill at reduced
  opacity, or shift the pill vertically off the post lane. Needs a mockup first.

## 4. Price pill stale — reads 7691 while ES trades 7689.25
- OBSERVED (operator, 09:24 ET): pill showed 7691 (red-left tip) with the Atlas chart at 7689.25;
  FUTMODE.futPx at the same moment was 7689.5 (live). ~2 pts / several bars behind.
- WHAT THE CODE DOES: to determine — which price source the pill binds (candle close vs futPx vs
  band-frame conversion) and its update cadence vs the render tick. futPx was current, so the lag
  is in the pill's source or its render path, not the capture.
- NEXT: measure the pill's update path before proposing a fix (do NOT guess a source swap — the
  band/pill frame rules in DECISIONS.md were hard-won).

## 5. THE BIG ONE — heavy nodes missing from the rail: piles are clipped to a ~5-pt EM band
- OBSERVED (operator, 09:2x-09:3x ET, ES chart): Atlas SPXW book had NINE nodes ≥20%K (King 7655
  100%, 7670 67%, 7685 −55, 7675 −50, …) — the rail drew ZERO g3pile posts at 09:23, and ONE
  (SPXW 7685 = ES ~7690 ACC) at 09:33. The King and every other heavy node were absent.
- MEASURED MECHANISM: `skPiles` (and `emPilesIF`) clip piles with `if(disp<B.low||disp>B.high)
  continue` — the band [B.low,B.high]. The live band read **EL 7688 / EH 7693 — FIVE ES POINTS
  WIDE** (yesterday: 7674/7679, also 5 pts). Only SPXW 7685 (ES 7690) fell inside; King 7655
  (ES ~7660) and 7670/7675 were clipped. The rail FRAME also narrowed to ~7675–7704 vs ~7631–7732
  the previous session.
- ROOT-CAUSE HYPOTHESES (measure before fixing):
  (a) the EM itself is wrong-scale — ±2.5 pts looks like a SPY-dollar straddle (~$2.4) applied to
      the ES frame without conversion; a real ES 0DTE EM is ~±30-50 pts. NOTE: the operator
      flagged "the EM is obviously incorrect" DAYS ago (flag-post mockup session) — it was to be
      addressed in the mockup recommendations and may never have landed.
  (b) even with a correct EM, clipping the PILES to the EM band is questionable in the flag-post
      era — the King is the anchor of every %K and can legitimately sit outside the band
      (skRoles already computes roles over the WHOLE ladder for exactly this reason, v11.81).
- IMPACT: the rail's whole purpose (fuel/friction between price and the rails) is blind to the
  King and every heavy node beyond ±2.5 pts; the "through the band with nothing below" reads are
  computed over the same clipped set, so the narrative is also wrong.
- NEXT BUILD: measure emBand's EM inputs live (straddle legs, units, conversion), fix the scale,
  and DECIDE whether piles should clip to the band at all or to the rail FRAME (with off-frame
  King pinned at the edge, arrowed). Do not touch without re-reading DECISIONS.md band entries.
- BLAST RADIUS (operator-observed 09:3x): the clip starves EVERY pile consumer, not just the rail
  posts — the NODES section rows, the profile's 5M/15M/60M ROC matrix, and the in-play/flag
  effects all draw from the same emPiles list, so price was visibly interacting with a node on
  Atlas while the rail, ROC rows and interaction effects showed NOTHING. Operator's directive on
  the fix direction: the rail must stay IN SYNC with the SPXW ladder (Atlas) — the nodes should
  come from SPXW regardless of the band's width; the band is a measuring stick on the rail, not
  an admission filter for which nodes exist.
