# lsHodLod 2.21.0 - the HOD / LOD confirm badge (2026-11-07)

Rassul 2026-10-21: "badges marking where a HOD / LOD occurred, drawn only AFTER the reversal is confirmed - not while the extreme is
forming; confirmation = a certain amount of movement in the opposite direction (a CoB is not required)". 2026-11-07 19:36 CT
"hod lod is a go"; option B approved on the "HOD LOD Confirm Badge" mockup (https://claude.ai/artifact/9xBYgS885ebzNy1ni7Rr8g);
early badges that get taken out stay exactly as drawn (no dimming, never repainted). Supports his setups: it marks the 1st extreme once
it is real and feeds the big-range-day read (focus rule: support).

## The rule (HodLodConfirm.h 1.0.0, same for every market; ADR sets the amount per market)
- A running RTH HOD is confirmed on the first CLOSED bar whose CLOSE is >= 0.25 x ADR below it AND whose bar END is >= 30 min after the
  extreme bar's END. A higher high starts a new running HOD; ties keep the first bar. LOD = the mirror.
- RTH bar = lsHodLod's own rule (bar START >= open, END <= close). ADR = mean of the last 14 RTH ranges; a session cut by the first
  loaded bar never feeds it. Roll day (gap > 4 ADR) = no badges. Fewer than 14 prior sessions = no badges (status says why).
- The badge is computed from the chart's closed bars of the last 35 calendar days (a separate read; the box keeps its 21 days).

## Measured (3-min RTH, May 2025 - Nov 6 2026, ~370 sessions a market, roll days dropped)
| Mkt | real HOD/LOD gets a badge | taken-out badges / day | badges that mark the real extreme | lag median / q75 |
|---|---|---|---|---|
| ES | 80% | 0.99 | 62% | 30 / 42 min |
| NQ | 80% | 0.92 | 64% | 30 / 39 |
| CL | 80% | 1.06 | 60% | 30 / 39 |
| NG | 78% | 1.00 | 61% | 30 / 42 |
| GC | 84% | 0.94 | 64% | 30 / 36 |
| HG | 80% | 0.99 | 62% | 30 / 39 |
| EU | 82% | 1.05 | 61% | 30 / 48 |
Two thirds of the taken-out badges print in the first hour; build and check halves agree within ~3 points.

## Drawn
- A thin solid line from the extreme candle to the confirm bar at the extreme price, a 4 px dot at the confirm bar, and a filled pill
  "HOD" (red, above the high) / "LOD" (green, below the low) on the extreme candle, 9 pt bold white text, no border, no hover, no
  other text. The pill keeps off the table, the read strip, the 2nd-extreme target badge, the 1st-extreme marker, every text block and
  other indicators' boxes (steps away from price, then sideways toward the confirm bar); no clear spot = line + dot only (counted).
- No settings change (setParameterVersion unchanged; spec version string = 2.21.0).

## Files
- Record: lsFlexLevels\HodLod\Confirm-<MKT>-<sec>s.csv, one line per badge, written once:
  `B,<yyyymmdd>,HOD|LOD,<extreme bar end HH:MM>,<confirm bar end HH:MM>,<price>,<ADR>,lsHodLod 2.21.0`.
  What is drawn = the record + this pass, so a badge never moves or vanishes when the 35-day window slides or IRT restarts.
- Status (HodLod.status-<MKT>-<sec>.txt): `CONFIRM,1.0.0,rule close 0.25 ADR back + held 30 min,bars N,sessions N,last <day> adr X,
  today HOD hh:mm>hh:mm ...,record N (+n now, fails n),draw N (record n),pills n,no clear spot n`.

## Tests - tests_hodlod/t_hl2210.cpp (g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined): 29 passed, 0 failed
- [1] parity with the Python study: FULL history (all 7 markets, 1.19 M bars) every badge identical - ES 970, NQ 935, CL 764,
  GC 997, HG 958, NG 977, EU 1,011 (6,612 / 6,612). The repo fixture hl2210/ (real bars from 2026-09-20, 3.3 MB) gives
  ES 58, NQ 48, CL 55, GC 51, HG 35, NG 57, EU 39 - all identical.
- [2] no repaint: ES and GC replayed one closed bar at a time over the last 6 days - no badge ever changes or disappears; the last
  pass = the straight pass; a 34-day window = the full history's badges.
- [3] rule edges: needs both the 0.25 ADR close and the 30-min hold; a higher high replaces an unconfirmed HOD; ties keep the first bar;
  13 prior sessions = none; roll day = none; a cut first session is not in the ADR; LOD mirror.
- [4] the pill: above / below, steps off obstacles, none clear = no pill; 2,000 random scenes, none outside the pane or on an obstacle.
- [5] record line format; versions 2.21.0 / HodLodConfirm 1.0.0.
- Older tests: t_hl240 / t_hl2180 / t_hl2190 fail only their old version checks (2.20.0 / 2.19.x literals), as on every bump.

## Not verified - needs his IRT
- The host compile (cl, real SDK) - gex-build.bat compiles within 2 minutes (HodLod.compile.txt); the cloud has no SDK.
- On his charts after the restart: the CONFIRM status line, pills on 3-min charts in 2+ markets, nothing overlapping (ChartView export).
- 1-min charts use the same rule (bar END hold, close-based); the numbers above are from 3-min bars.
