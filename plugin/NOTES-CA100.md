# lsCandleAbsorb 1.0.0 (2026-11-01) - candle absorption marks + the delta POC dash (VS ES chart)

Rassul 2026-10-31: "apply the absorbtion candle stick rules now that we disucessed"; "instead of R , just put D for Demand ( S for supply
in opposite case). get rid of F. i just want A, D,S. if there is exhasutionstion just put Ae"; "i also want a small dash that represents
the delta poc on everybar"; "the dash is supposed to be only on the candles". Overnight 23:10 CT: "implement waht we have disuccessed".

## Rules (CandleAbsorbLogic.h)
- Candidates: IRT ZigZag pivots (3 bars, High/Low) + the next bar when it re-tests the extreme within 4 ticks.
- Delta POC = the row with the biggest |buy - sell| volume (IRT VAP). Must be in the extreme end (low: at/below open AND close, lower
  half of the bar; high mirrored) and >= 2.0x the bar's median |delta| row.
- Code (one per bar): A? attackers absorbed (sellers at a low / buyers at a high); Ae? = A + the next row past the POC <= 20% of it;
  D? buyers at a low; S? sellers at a high. Green under lows, red over highs. "?" = unproven until the nightly scores them.
- Decided at the bar that CONFIRMS the pivot (closed bars only); recorded once; never repainted (tested bar-by-bar = one pass).
- Dash on every candle (closed + forming): ON the candle at its delta POC, body width + 2 px a side, black 1 px edge, green = buyers
  made the POC, red = sellers. No VAP for a bar = no dash (never guessed).

## Files
- lsFlexLevels\CandleAbsorb\<MKT>-<spb>.csv - every decided mark once (MARK,bar end secs,time,LOW/HIGH,code,poc tick,delta,x,pivot,...)
- lsFlexLevels\CandleAbsorb.status-<MKT>-<spb>.txt - version, rules, counts, last marks, VAP state, dashes / marks drawn

## Tests
- tests_candleabsorb/test_candleabsorb_host.cpp 32/0 (-Wall -Wextra -Werror, ASan + UBSan clean): rules on hand-made bars, the ES
  10-29/10-30 fixture through the real host (= the approved mockup's afternoon: 1:42 S, 1:54 D, 2:21 D, 2:33 Ae, bar END times),
  dash on every candle, colours, no VAP read in draw, record once / restart, bar-by-bar = one pass, two charts, destroy.
- MinGW -fsyntax-only against the real irtsdk.h: 0 errors / 0 warnings.
- render: tests_candleabsorb/render_dump.cpp + render_check.py.

## Not verified - needs his IRT
- The dash size on his bar spacing; the label font (10 pt bold) next to other marks on the VS ES chart.
- No settings (NO_UI). Add "Candle Absorb" to the VS ES chart's price pane.
