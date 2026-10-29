# lsExhaustion 0.3.0 - exhaustion AT PIVOTS, histogram (2026-10-29)

## Request (Rassul 05:11 CT)
"update the exhaustion indicator so it follows the pivot rule and only bars above 2x are shown and draw it like a histogram instead of connected lines"

## Pivot rule (ExhaustionPivots.h, copied from lsWavePOC 1.3.2 - not shared, so neither build depends on the other)
IRT's ZIGZAG, Min Price Change = 3 BARS, Use High/Low: the live leg's extreme moves only on a STRICTLY more extreme bar; a bar whose high
beats the previous 3 bars' highs reverses a down leg, a bar whose low is under the previous 3 bars' lows reverses an up leg - the old
extreme is then a final pivot, confirmed on that reversal bar. Closed bars only.

## What is measured (per confirmed pivot, over the zigzag leg that ENDS at it; leg start = the previous pivot)
- pivot HIGH -> BUYERS FADING = buyers' peak pace in the up leg / buyers' pace on the pivot bar   (red bar: bearish)
- pivot LOW  -> SELLERS FADING = sellers' peak pace in the down leg / sellers' pace on the pivot bar (green bar: bullish)
- pace = aggressive volume (buyers = at the ask, sellers = at the bid, from IRT's VAP) averaged over the market's S bars (ES/NQ 2, CL/GC 1,
  HG 2, NG/EU 1); denominator floored at 1 contract. Same pace as 0.2.x; only the leg window changed (0.2.x: since the 20-bar opposite extreme).
  First pivot of the window: leg start = the 0.2.x 20-bar opposite extreme.
- Example: 4.4x = at the pivot high the buyers traded at under a quarter of their peak pace in the push.

## Drawing
- One histogram bar on the pivot bar, from 0 up to the ratio (60% of bar spacing, 3-14 px), the value "2.9x" on top; ONLY ratios >= 2.0x.
  No connecting lines; the 2.0x level line stays. Drawn once the pivot is confirmed (the reversal bar closed) - never repainted:
  a pivot is accepted only when its confirmation bar is newer than the last processed bar (a sliding 1500-bar window cannot add or
  move an old pivot); restart redraws from the records.
- Header: "EXHAUSTION at pivots  Buyers fading: <last pivot high>x (2.0x)  Sellers fading: <last pivot low>x (2.0x)" + testing/confirmed.
- Colours kept as 0.2.x (buyers fading = red / bearish, sellers fading = green / bullish) so the header and bars match what he saw.
- E? no longer drawn; the E records are still written exactly as 0.2.4 (nightly lra.exh_study unchanged).

## Records / status
- NEW lsFlexLevels\Exhaustion\<MKT>-<CONTRACT>-<spb>-pivots-<session>.csv, header bar_time|side|fade|high|low|params; EVERY pivot (also < 2x):
  `pivot_time|top/bottom|ratio|high|low|pivot N3 S<S> conf <confirm bar time> leg <leg start time> shown|logged`.
- Status file: VERSION,0.3.0 + `PIVOTS,zigzag N 3,show 2,pivots n,shown n,last_high <time> <ratio>,last_low <time> <ratio>`.
- No settings; parameter version unchanged (1).

## Tests (Linux g++ -Wall -Wextra -Werror, ASan + UBSan)
- test_exhaustion_030.cpp 16/0: ES 10-28 real bars (wavepoc_es1028_bars.csv) pivots == an independent implementation of IRT's rule (50
  pivots), alternate H/L, each confirmed on a later bar breaking the prior 3 bars' opposite extreme, 13:18 L present; every prefix gives
  the same pivots + ratios (no repaint); NaN bar = none. Host: pivot high 4.44x (2000 / 450) drawn as ONE box + "4.4x" red, the 1.14x low
  logged not drawn, exactly 1 line (the 2.0x level), no E?; pivots file has shown/logged lines; status line; later bars do not change a
  pivot; restart = same pivots.
- test_exhaustion_host.cpp 9/0 (updated: contract-qualified record path, header text, E not drawn). Leak check off for this older test
  (it never deletes its extensions - test-side, pre-existing).
- MinGW x86_64 -fsyntax-only vs real irtsdk.h (RT_LONG = unsigned long long): clean, no warnings in plugin files.
- Mockup from real ES 10-27/28 IRT footprint (fp_1m -> 3 min): 59 pivots, 8 at >= 2x (e.g. 05:48 buyers fading 5.6x, 03:36 sellers 3.7x).
