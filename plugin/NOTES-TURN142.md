# lsTurn 1.4.2 - Absorption bars in the attackers' colour (2026-11-24)

Rassul 11:21 CT: "first color the bars differently , top should be green and bottom should be red".

## What changed
- The Absorption bars: the TOP bar (the heaviest price traded at the offer in the top 25%) is green, the BOTTOM bar (traded at the bid
  in the bottom 25%) is red, when they count (2.0x+ and closed outside that quarter); grey otherwise, as before. Was the winner's colour.
- Nothing else: the numbers, the Stop Run / Exhaustion / Initiative bars, the top strip and the read are unchanged.

## Tests
- test_turn 3,287,189 checks 0 failed; host 36,999 checks 0 failed (colour checks moved: green above the axis, red below);
  golden 2017 / 0 mismatches; soak plain + ASan no growth; t_read140 262; real irtsdk.h C++14 / C++17 0 errors.

## Not verified - needs his IRT
- The colours on the live chart after the restart.
