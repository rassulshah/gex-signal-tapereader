# Dealer Profile 2.52.1 - 2026-11-03 (Rassul "go": rank boxes too small / off centre)

## Request
LIQUIDITY rows (rank box at the FRONT since 2.52.0) and the level-badge rank chips ("ONH/PHH [10]"): the rank box was a fixed one-character
square (bh-2 px), so two-digit ranks (10, 11 - level-linked extras keep their real board rank) spilled out, and every glyph sat right of
centre (the king's dark "K" half off its white box). Screenshot c5b3c216-image.png.

## Root cause (two parts)
1. Box width fixed at bh-2 whatever the text.
2. Centring: the rank text was drawn left-justified (textLJ -> RCT::drawText(s, false, false)) at centre - textW/2. On his chart IRT draws
   left-justified rect text ~4 px RIGHT of rect.left (pixel scan of the screenshot: "2" / "K" / "11" / "10" all 4-5 px right of the box centre
   whatever their width - so getTextWidth is right and the offset is constant; the badge price drawn at x+1 starts ~x+5; the "ONH/PHH" name sits
   ~4 px right in its chip too). The mock drew at rect.left, so no host test could see it.

## What changed (DealerProfile.cpp only, + versions)
- New textC(centerX, y, ...): same font / vertical math as textLJ, drawn with drawText(s, TRUE, false) in a symmetric rect (centerX +-300):
  IRT centres with its own metric, so its left inset cancels.
- LIQUIDITY rows: RKW = max(bh-2, widest shown rank label ("K" for the king in book mode, else the number) + 6), computed before the LOTW clamp
  (rk0 = RKW); RKX / BX (= RKX + RKW + 3) / TXR use it; rank rect rx .. rx+RKW; text textC at rx + RKW/2. One width for the column (aligned).
- Badge chips: cw = max(bh-2, textW(rk)+6); rl = min(x, rgt-1-cw), rr = rl+cw; text textC centred; the width pass adds cw+3.
- The no-'B'-producer fallback column: rank text centred with textC (box unchanged).
- Unchanged: colours / styles, which nodes show, the "#3 #4" off-screen marker, all other text.
- DP_VERSION / setVersion / DealerProfileSettings.h PLUGIN_VERSION = "2.52.1" (no settings change).

## Tests
- test_dealerprofile_2521.cpp (new; every 2520 check + new scene): 38 passed / 0 failed. New scene: ranks K 2 3 4 5 10 11 in one column; with
  the mock modelling IRT's 4 px left-justified inset: every label fully inside its box, centred within 1 px, one aligned column of one width
  (18 px vs 14), rows inside the column, nothing overlapping; badge chips "10" (VAH) / "11" (PFL/PDL) fully inside, centred, in the Key Levels
  column. Same test on the 2.52.0 code: 6 fail (version + "K" 1441-1449 in box 1434-1448, as on his screenshot; "10" spills the chip).
- test_dealerprofile_2520.cpp / 2400.cpp: the mock's drawText now honours the centred flag (the rank text is centred).
- MinGW -fsyntax-only with the real irtsdk.h: 0 errors; the same 6 pre-existing warnings outside the SDK before and after (none on changed lines).

## Not verified - needs his IRT
Close IRT -> gex-build installs lsDealerProfile 2.52.1 -> reopen; "DP v2.52.1 LIQUIDITY": K / 10 / 11 inside their boxes, centred.
Not changed (same inset, separate item): the level name text inside its chip and other left-justified labels also sit ~4 px right.
