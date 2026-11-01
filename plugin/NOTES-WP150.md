# lsWavePOC 1.5.0 (2026-11-01) - delta side, letters, dotted node borders, POC pullback, new wave rule

Rassul 2026-10-31 20:33-23:10 CT and 2026-11-01 05:50 CT ("implement what we have discussed", "dotted border around the distributions for the top 4 nodes").

## What changed on the chart
- Each profile has two sides of 30 px at the wave start (was 40 px, one side):
  - LEFT: the wave's volume profile as a grey outline; HVN rows light grey, WPOC amber; liquidity nodes (LPOC) grouped here.
  - RIGHT: the wave's same-price delta (IRT VAP buy - sell volume): green buyers, red sellers; rows >= 2x the median bright, others faint; the delta POC outlined in white.
- Letters (top 4 delta rows, each >= 2x the median |delta|; then location; then the class), centred on the node, "?" = unproven:
  - I? with the wave, first third of the wave - wave colour
  - A? against the wave, before the last quarter - wave colour
  - T? with the wave, last quarter, AND a bar closes back through it within 5 bars after the pivot - opposite colour, one per wave
  - cluster A? = an 8-tick middle band (25-75%) against the wave >= 2.0x the median of the prior 20 waves' best band (needs 10 waves)
  - the same letter within max(8 ticks, 10% of the wave) is drawn once
- A dotted white border round each top-4 node's distribution (contiguous rows, same sign, >= the median |delta|) and the cluster band.
- Pb (POC pullback): after an up wave, any of the next 3 pullback lows whose bar trades at / below the wave's volume POC while its high stays above; mirror after a down wave. "Pb" + an amber dotted line from the POC.
- Which waves get a profile: REVERSAL (14-bar extreme AND > the direction's average AND > 3 ATR) OR BREAKOUT (> 3 ATR and a bar closes beyond the previous swing in its direction) OR RALLY / DECLINE (> 4 ATR). CoB legs unchanged.
- Status: QUALIFIED_BY reversal / breakout / rally/decline (older records AVG / ATR); PROFILE line (letters / Pb drawn now).
- No settings change. No getCustomIndicator. Decided on closed bars; letters fixed once 5 bars after the pivot have closed (tested: no repaint).

## Tests (Linux, -Wall -Wextra -Werror, ASan + UBSan clean)
- test_wavepoc_logic 193/0 (legacy checks pinned to the 1.3.3 rule via Engine::rule = 133; drawing checks moved to the 1.5.0 layout)
- test_wavepoc_150 30/0 (new: letters / location / T close-back / cluster A / distributions / Pb / ES 10-29+10-30 fixture with delta, no repaint, delta sums)
- test_wavepoc_host 109/0 (built with -DWPC_DEFAULT_RULE=133 for the 10-22 fixture's wave counts; the plugin build uses 150; the mock VAP now splits buy / sell 60/40 so the delta read is tested)
- render: tests_wavepoc/render_dump_delta.cpp + render_check.py (ES 10-30, 1700x900)

## Open questions for Rassul
- Should Pb stop once price breaks the wave's start? Count near misses (e.g. 2:18, 1.50 pts short)?

## 2026-11-01 06:35 CT change (Rassul: "instead of 4 lets shorten it to 2 so the process is to identify 2 largest nodes, box them (solid box), classify them")
- LET_TOP 4 -> 2: the 2 biggest delta rows (each >= 2x the median) are boxed, then classified (I? / A? / T?); cluster A? unchanged.
- The border is a SOLID 1 px white box around each node's distribution (was dotted); the separate delta POC box is not drawn when the
  delta POC is one of the 2 (its solid box marks it).
- Pb moved one text row further from the bar (PB_GAP 16 px) so lsCandleAbsorb's code on the same pivot bar never overlaps it.
- Tests: logic 193/0, 1.5.0 31/0, host 109/0 (ASan + UBSan).
