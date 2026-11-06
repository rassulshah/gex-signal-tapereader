# lsBreakout 1.9.0 "Rev" (2026-11-06 09:38-09:51 CT)

Rassul asked for a "PB Rev": "basically its a thrust after a pullback". He refined it:
- "or it can be the pivot bar also"
- "just a green Rev mark"
- "the biggest range"
- "only have 1 Rev per wave"

He reviewed mockup pb-rev v0.1 and said "yes build".

## Rule (bullish; bearish is the mirror)
After a Pb (a pullback pivot LOW after a BO), the Rev is the FIRST bar, counting from the pivot bar itself, that meets all of these:
1. It closes **green** (close > open).
2. Its **low is below the high of the pivot bar** (the down-pivot bar).
3. Its range (high − low) is **≥ 1.2 × ATR(14)**. ATR is the Wilder 14-bar ATR at the bar before. **Or** its range is **bigger than each of the last 5 green bars**.

The window is the up wave that starts at the pivot. It ends at the bar that confirms the next pivot HIGH (the first red close after the wave's high). It also ends early if a bar trades below the pivot low.

Other rules:
- One Rev per wave (per Pb).
- The Rev is decided at the later of the Rev bar's close and the Pb's confirmation, so a Rev on the pivot bar shows when the Pb shows.
- Once made, a Rev is kept for the chart's life (no repaint).
- Bearish mirror: after a Pb pivot HIGH, the first **red** bar whose **high is above the pivot bar's low**, with the same range rule. It draws a red "Rev" above the bar.

## Drawing
- "Rev" uses the BO font: green below the bar (after a BO), red above it (after a BD).
- It takes the first free row from the wick, using the BI row search (biReserved).
- It skips the pivot label rows, the LVP row and our own letters. On the pivot bar it sits below the Pb.

## Records and status
- Records: one `BRV|` line per Rev (20 fields) in the `-pb-` file. Key = t_rev + side.
- The `-pb-` file header gains a `# BRV|...` comment line.
- Status lines: RULES carries the Rev rule. KEPT and DRAWN add "Rev".

## Test week (from tests_bo100/test_bo190_rev)
| Market | Pb | Rev | Rev on the pivot bar |
|---|---|---|---|
| ES | 173 | 74 | 15 |
| NQ | 171 | 71 | 20 |
| GC | 160 | 58 | 21 |
| EU | 19 | 9 | 2 |

## Tests
- **test_bo190_rev (new): 29 passed.** It checks:
  - an independent re-check of every Rev (first qualifying bar of its wave, ATR / biggest-5 flags);
  - that no Pb without a Rev has a qualifying bar;
  - one Rev per Pb;
  - a bar-by-bar replay equals one full pass, with no Rev changed and none decided on the forming bar;
  - the layout (green below / red above, never on another letter);
  - BRV records.
- **Version pins moved to 1.9.0:** test_bo100_host, test_bo130_rec (which now also checks the BRV lines and the lines-written count), test_bo161_onebar, test_bo170_onrly, test_bo180_pb.
- **run_tests.sh** runs test_bo190_rev.

## Trade lines (10:10-10:14 CT: "lets add a stop line and target lines", "lets add 1.618 as the 2nd and final target"; mockup v0.3)
The levels are fixed when the Rev is decided (RevMark org / t1 / t2 / stop):
- **Stop:** 1 tick beyond the lower of the Pb pivot low and the BO candle's low (BD: the higher of the highs).
- **T1 "Rly":** the wave extreme from the BO / BD candle to the pivot bar. This is the high before the pullback.
- **T2 "1.618" (final):** origin + 1.618 × (T1 − origin).
  - origin = the last opposite pivot before the BO candle, confirmed by the BO's fire; otherwise the BO candle's low.

The lines (bko::revLines):
- They are 1 px, drawn from the Rev bar, under the letters. The Stop is in the other colour; Rly and 1.618 are in the Rev's colour.
- A small name sits right of each line's end, and is dropped if it would sit on a letter.
- A target line ends at the bar that trades to it.
- All lines end at a stop touch, at T2, at the next Rev (either side) or at the session's last bar.
- An open line runs to the live bar.

Test week, line ends (ES / NQ / GC / EU):
| Ended by | ES | NQ | GC | EU |
|---|---|---|---|---|
| T2 | 27 | 26 | 21 | 3 |
| Stop | 30 | 25 | 24 | 4 |
| Next Rev | 13 | 19 | 10 | 2 |
| Session end | 3 | 1 | 3 | 0 |

test_bo190_rev [7] checks the lines independently: the levels, every end, never past the session, no level change in the replay, and the line count / colours drawn. BRV records carry origin / t1 / t2 / stop (20 fields).
