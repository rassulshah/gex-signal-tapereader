# lsBidAskStats 1.33.0 (2026-10-26) - BUILD-SPEC "Attacker pane (lsBidAskStats): bars + S only" (Rassul approved 22:29 CT 2026-10-25)

## What changed (display only)
- The pane draws the buy / sell aggressor bars (bright + ratio when 2x+ normal, unchanged) and ONE letter strip, "Attack", with the
  **S** letter only (one order swept 3+ prices). Band height 1 strip (was 2): the bars get 11 px more.
- K / L / R / Tr / D / E / G (and PB in the nightly) are still decided and recorded exactly as before (LT + EV records: the Reversal Stage
  pane, its stage box and the nightly read them) - they are just not drawn. The **E pace** is in the S hover ("E  Attack pace (hover only,
  not drawn): ..." + the "Hover only: E fading ..." line).
- New settings row "S letter (one order swept 3+ prices)" (checkbox, default on). The place file gets field 10 (0 / 1); a 9-field file
  = on. Settings parameter version 7. Colours: S uses the existing buyer / seller colours.

## Tests (Linux mock, ASan/UBSan, -Werror)
- tests_aggbook/test_aggbook_host (BidAskStats): 74/74 (10-field Apply, field 10 parse / default / bad value).
- tests_aggbook/test_attack_126_host: 44/44 - S drawn, K / L / R not drawn, one strip label, S hover with the E pace, no repaint, restart.
- tests_aggbook/test_aggbook_114_host (Big Lot): 30/30. tests_sab: 17/17 (stale 1.28.0 pin moved).
- Settings contract 43,863 checks, 0 failures. MinGW with the real irtsdk.h: 0 errors.
- tests_aggbook/test_aggbook_113: 182/182 (its stale version / settings pins moved with this build, + the S letter row).
