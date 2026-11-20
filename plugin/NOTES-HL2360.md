# lsHodLod 2.36.0 — the Now levels line (2026-11-19, 18:35–19:06 CT)

Rassul asked for the Now box to be about levels: the last level broken, the next levels, and whether price is looking to test a level it already broke. That tells him where to look for a BkTst reversal, or for a break or sweep. He has little vertical space, so it is one horizontal line. The mockup is `now-box-levels-mockup.html`.

## What is drawn
There is one line at the old Now box spot: centred on the HOD / LOD table, just above it (or above its Gex strip). Without a table it sits at the bottom centre of the pane. It is updated live, once a second, on the forming candle.

There is one badge at a time, first. Only signals at named levels are shown. Nothing comes from a Cob (19:02 "no lets remove anyting that is associated with a cob"); the line does not read CobSignal. There is no REV state (19:04 "remove for the Rev candle"): after a BkTst Rev the line moves on to the next WATCH. They are listed here in priority order:

| Badge | When | Colour |
|---|---|---|
| `TESTING: PDL · BkTst Rev ↓` | The pullback after a Bk that had its FT trades into the Bk candle's range. It is ≤ 100% back, never past the Bk candle's extreme, and has no close beyond the move origin. The furthest-back Bk it reached is named. | Red if bearish, green if bullish |
| `BREAK: PDL ↓` / `SWEEP: PDL ↑` | The forming candle goes through a watched level (the nearest above / below the last close). BREAK: it is closing through the level in the break's colour. SWEEP: it is back in the Sw colour (a swept low is green, a swept high is red). It settles at the candle's close. | Red / green by side |
| `WATCH: PHL BkTst Rev ↓` | The move topped (a closed candle since the extreme did not extend it), and the pullback is heading back but has not reached a Bk candle yet. The nearest Bk ahead (the latest) is named. It waits while the newest Bk's FT window (3 candles) is still open. | Gold |
| `WATCH: VAL ↓ · ONL ↑ · Bk / Sw` (19:06: the level below, the level above, then the signal) | Otherwise. It names the nearest levels below and above. | Gold |

After the badge, the line shows `│ Next ↓ <level> · ↑ <level>` (after BREAK / SWEEP, only the level in that direction), then `│ Last Bk ↓ <level>`. Each of these appears only when its level is not already named on the line, so no level is named twice.

The line has no numbers: no prices, distances, strength, % or time. When the pane is narrow, the Last Bk part drops first, then pieces drop off the right end.

## Inputs
- **Levels:** LRA-KeyLevels-<MKT>.csv in the marks' level set: ON / London / Asia, PDH / PDL, PFH / PFL, VAH / VAL, PWH / PWL / CWH / CWL. It excludes DPoc / WPoc / Open / VWAP / gamma. It also includes the prior 3 full clock hours' highs / lows (PHH / PHL, 2hH / 2hL, 3hH / 3hL), computed live from the chart's bars (the key file's PHH / PHL snapshot is not used), never the session's first hour.
- **Bk marks:** lsFlexLevels\RevRead\wy-<MKT>-<spb>.txt `Bk` lines, this session only. A mark also has to pass the marks' BO candle rule. The file is re-read at most every 3 s, by stamp, and is bounded to 512 KB.
- **FT:** the BO indicator's FT rule, computed here from the bars.

## Code
- NowLevels.h 1.0.0 (namespace nowl) is SDK-free. It holds parse / compute / segments / layout. Arrows are drawn as pen strokes and the separator as a line, because IRT drawText is cp1252. The text uses the cp1252 middle dot.
- HodLod.cpp:
  - recompute keeps this session's closed bars plus the forming bar.
  - The draw widens the forming bar's high / low with the live price, recomputes once a second, and draws the items.
  - Confirm badges keep off the line's box.
  - There is a status line, `NOW_LEVELS,1.0.0,box …,runs,marks,bars,<why>,line <text>`.
  - The 2.34.0 read box stays undrawn.
- No settings change. The version is 2.36.0 everywhere. NowLevels.h is on the gex-build HodLod line.

## Tests
- tests_hodlod/t_hl2360.cpp: 78 checks, `-Wall -Wextra -Werror` with ASan/UBSan. Fixtures:
  - nl_es_1119_3m.csv (the real ES 11-19 3-minute bars).
  - nl_keys_es_1119.csv.
  - nl_wy_es_1119.txt (the Bk marks as in the mockup's signals table).
  - nl_wy_es_1119_pdl.txt (the same marks without the 11:03 PHL break).
- MinGW `-fsyntax-only` against the real irtsdk.h: 0 errors. The one warning, in first_extreme_inference.cpp, was already there before this change.

## Real ES 11-19 results (with these bars and levels)
| Time | Line |
|---|---|
| 11:33 AM | `TESTING: PDL · BkTst Rev ↓ │ Next ↓ 2hL · ↑ VAL │ Last Bk ↓ PHL` |
| 10:45 AM | `WATCH: VAL ↓ · ONL ↑ · Bk / Sw` |
| 9:42 AM (forming) | `SWEEP: PDL ↑ │ Next ↑ VAL │ Last Bk ↓ ONL` |
| 10:54 AM (forming) | `BREAK: PDL ↓ │ Last Bk ↓ ONL` (no named level below PDL then) |
| 8:33 AM (forming) | `BREAK: 2hL ↑ │ Next ↑ 2hH`. The live hour levels from the bars are PHH 7,435.50 and 2hL 7,416.75. The mockup's "PHH 7,425.75" is the key file's stale snapshot. |

These transitions are tested:
- All marks: WATCH PHL (11:12) → TESTING PHL (11:21) → TESTING PDL (11:24) → the next WATCH (11:39, the pullback turned).
- PDL marks only: WATCH PDL (11:12) → TESTING PDL (11:24) → the next WATCH (11:39).

## Differences from the mockup
- The mockup uses Next PHL / VAL at 11:33 and PDL / ONL at 10:45. The real data has 2hL and VAL (live 7,296 sits between 2hL 7,293 and PDL; PDL is being tested, so it is skipped). At 10:45, VAL 7,333 is just under the price.
- The mockup's Last Bk at 11:33 is PDL. The real last Bk is PHL 11:03, so the line shows it.
- TESTING no longer needs 38.2% back (per the 18:52 rule: "into the Bk candle, ≤ 100%"). The LevelMarks Tst mark still uses 38.2%.
- The PC's wy file is still written by lsRevRead 0.19.4. Its level names come from that version (for example, the 10:54 Bk is named "PHL" there). The line shows whatever the installed RevRead writes.
