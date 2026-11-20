# lsHodLod 2.37.1 - gamma pill label + TESTING wording and rules (2026-11-19, 22:35-22:46 CT)

## Request
Rassul approved at 22:35 CT, in his own words, the rows of gamma-tag-mockup.html under "YOUR IDEA, once the forward test proves it",
shown WITHOUT "?". The gamma pill at the far left of the Now row gets a label:

- negative gamma: `-GAMMA · Trending · Bk / BkTst` (red outline and red text, as the pill is today)
- positive gamma: `+GAMMA · Range · Sw / Cob` (green)

The middle dot is cp1252 0xB7, the same as elsewhere in HodLod. "0DTE Exp 3:00 PM" follows as today. No other changes.

Added at 22:42 CT. Rassul sent a screenshot of ES overnight showing "TESTING: PFL · BkTst Rev ↓" and asked "how can you have a BkTst Rev when you are testing a level?". The TESTING badge must not name the Rev, because it has not printed yet. The new wording is `TESTING: <level> BkTst <arrow>`, e.g. `TESTING: PFL BkTst ↓`, with no "Rev" and no middle dot, in the same Level-then-Signal order as WATCH. WATCH stays `WATCH: <level> BkTst Rev ↓`, because it names what to look for.

Added at 22:46 CT. Rassul: "after it breaks, and continues, you can then have the test of the level after it has the minimum required retracement". The Now line's TESTING state must use the same Tst rules as the chart's Tst marks (LevelMarks.h seq):
- only after a Bk that had its FT;
- the pullback overlaps the Bk candle and has retraced 38.2% to 100% of the move (Bk move origin to the move's extreme);
- it does not go past the Bk candle's extreme;
- at most 2 Tsts per move;
- it is void on a close beyond the move origin, or on an opposite Bk.

Before 38.2% is reached, the line shows the gold `WATCH: <level> BkTst Rev <arrow>`. 2.36.0 had dropped the 38.2% minimum, so it showed TESTING as soon as price traded into the Bk candle.

## What changed
- HodLodTop.h 1.1.0:
  - `gammaTag(pill)` returns the label forms, longest first: full, then `-GAMMA · Trending` / `+GAMMA · Range`, then the word only. "GAMMA STALE" and an unknown regime are unchanged.
  - `nowLeftTag(...)` draws the whole label inside the pill, then the expiry.
  - On a narrow pane it drops parts in this order: the expiry first, then ` · Bk / BkTst` / ` · Sw / Cob`, then ` · Trending` / ` · Range`. The +/-GAMMA word always stays. As in 2.37.0, it goes only when even the word would come within 10 px of the Now line.
  - The 1.0.0 `nowLeft` is unchanged; nowLeftTag uses it.
- HodLod.cpp (host): calls `nowLeftTag(gammaTag(pill.s), ...)`. The status line now reads `NOW_ROW,1.1.0,left <gamma tag + 0DTE expiry | gamma tag (expiry dropped) | gamma tag short | gamma word only | none | not drawn>,tag <text>`.
- Versions: HL_VERSION = PLUGIN_VERSION = setVersion = header comment = 2.37.1. The parameter list is unchanged, so setParameterVersion is unchanged too.
- NowLevels.h 1.1.0: the TESTING badge text is `"TESTING: " + level + " BkTst"`, and its arrow is drawn after it as before. It is still red or green by side. WATCH is unchanged.
- NowLevels.h 1.1.0 rules: `TST_MIN = 0.382`, the same as LevelMarks.h.
  - In the Bk candle but under 38.2% back: state 1, the gold WATCH.
  - A move whose finished pullbacks already made 2 Tsts gets no more TESTING or WATCH ("2 Tsts already in this move"). Those Tsts are counted the chart's way: the first turn after the Bk is the top, each later pullback pivot must overlap the Bk candle and be 38.2% to 100% back, and the 2nd must reach further back than the 1st.
  - The FT, overlap, 100%, Bk-extreme, close-beyond-origin and opposite-Bk rules were already there in 1.0.0.
- gex-build.bat is unchanged: its HodLod line already lists HodLodTop.h.

Widths at 10 pt bold: the full negative label is 203 px, so the pill is 215 px. The full positive label is 181 px. The expiry is 117 px. With the expiry, the left part needs about 340 px left of the Now line.

## Tests
- tests_hodlod/t_hl2371.cpp: 46 checks, 0 failed (`-Wall -Wextra -Werror`, ASan/UBSan). It covers:
  - Both regimes render the exact strings: one 0xB7 byte, no UTF-8, no "?".
  - The whole label sits inside the pill, in the regime colour, and the expiry follows.
  - The drop order, swept pixel by pixel. All five steps are seen, the label never returns to a longer form as room shrinks, and nothing comes within 10 px of the Now line.
  - The price-axis limit.
  - The real centred Now line over box widths 260..1600 px, every 2 px, both regimes: no overlap, and the line stays centred in the mockup case.
  - TESTING reads `TESTING: PFL BkTst <arrow>` on both sides, red or green, with no Rev and no dot. WATCH still reads `WATCH: PFL BkTst Rev` in gold. NowLevels is at 1.1.0.
  - No regime and GAMMA STALE are unchanged.
  - The host call, the status line, the versions, and the gex-build line.
- t_hl2370: 5,884 / 0 failed. Its version strings and its sample TESTING text were updated.
- t_hl2360: 85 / 0 failed.
  - All of its TESTING expected strings are now in the new wording, and its version checks are now 2.37.1.
  - New 38.2% checks: a pullback into the Bk candle at 20% to 38% back stays WATCH (the old "25% back = TESTING" check now expects WATCH). At 38.9% or more, with the Bk candle overlapped, it is TESTING.
  - His real ES 11-19 PDL case still reaches TESTING at 11:24 AM, and the 11:33 AM line is unchanged.
  - Behaviour change on his real 11-19 data: at 11:21 AM the PHL pullback was in the Bk candle but under 38.2% back. 2.36.0 showed TESTING: PHL there; it now shows WATCH: PHL.
  - The "max 2 Tsts per move" rule mirrors LevelMarks.h, but no test exercises a third pullback yet.
- MinGW `-fsyntax-only` against the real irtsdk.h: 0 errors. The only warning outside the SDK is the existing one in first_extreme_inference.cpp.

## Differences from the mockup
- None intended. The label is inside one pill, as in the mockup's rows, with the expiry after it.
- The pill keeps today's 10 pt bold font and its dark fill.

## Not verified - needs his IRT
- The look on his charts after the restart. Screenshots are needed in 2+ markets.
