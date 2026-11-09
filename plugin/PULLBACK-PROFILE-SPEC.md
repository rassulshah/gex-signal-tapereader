# Wave POC - Pullback Profile - BUILT in Wave POC 1.10.0 (2026-11-09, held for the 15:00 install)
Mockup: claude.ai/artifact/5faySwa2NBJcWbwTrEGiAn (v5, real ES / NQ 11-09, his IRT zoom ~6.5 px per 3m bar).

## What
After a BO or Rly up wave (mirror for BD / Rly down), a volume-at-price profile of the PULLBACK, from the wave's top pivot on.
Purpose: a POC developing on the pullback = reaccumulation. No accumulation check (delta / A / ZD) for now.

## When it shows
- Nothing until the pullback has retraced >= 38.2% of the move (move = wave start pivot -> wave high).
- From then on, recomputed EVERY bar: POC + up to 2 HVNs (each the peak of its own distribution, Wave POC's separation rule).
  They may shift at any time (repaint OK while developing).
- Freezes (profile = the volume BEFORE the ending bar) at the FIRST of: a Rev fires; a separate new BO or Rly fires (not the wave's own);
  price trades above the wave high (pullback over); session end 16:00 CT; invalidation.
- Invalidation: a 3m CLOSE below the origin low (down waves: close above the origin high) -> the profile dims, no further updates.
  A one-tick trade through the origin that closes back (ES 11-09 08:31) stays valid.

## Look
- Anchored at the top pivot bar, drawn to the right. The longest row (the POC) reaches the right edge of the 2nd bar after the pivot,
  NEVER narrower than 20 px (at his zoom 2 bars ~13-18 px, so 20 px applies); other rows proportional; width scales with zoom.
- POC row WHITE, the 2 HVN rows ORANGE, both thicker than the plain rows. Plain rows 28% grey over the ground (the 1.9.6 fade),
  1 px thinner each side, drawn BEHIND the candles (pre-draw). No lines extended right, no price / text labels, no shift markers.
- Keep the existing Wave POC elements (wave profile, P, POC line, BO / Rly / Pb / Rev letters).

## Build notes (1.10.0)
- wpc::pullbackOf (WavePOCLogic.h): pure, deterministic from the closed bars + IRT VAP -> a restart recomputes the same frozen state (no
  journal needed; frozen / invalid profiles are never recomputed while the chart runs). Rows = 1 tick (the wave's row size), bars after
  the pivot bar, the ending bar excluded. Rev / BO / Rly from the same native lsBreakout engine (bko) the host already runs; a Rly ends a
  pullback at its confirmation bar (field 4 of its RLY| line), a BO at its fire bar; the wave's own BO / Rly never end it.
- Wave profile POC / HVN rows thicker (same colours: POC cyan, HVN green / red) - option A.
- Status: PULLBACK line (drawn / developing / frozen / invalid).
- Tests: tests_wavepoc/test_wavepoc_1100.cpp on the real 11-09 ES / NQ bars + VAP: ES final POC 7462.75 (= mockup) frozen at the 08:33 BO,
  75% deep; NQ final POC 27317.50 (= mockup) frozen at the 08:30 bar (traded above 27349.75); ES 08:31 one-tick stop run stays valid;
  a close below the origin -> invalid; CoB-only wave -> none; 16:00 end; width = 3 bars or 20 px; no text.

## 1.10.1 (Rassul 12:10, picked mockup v7 ABOVE)
- The lots of the wave POC row (POC colour, cyan) and of the pullback POC row (white; dimmed when invalid) as plain ASCII with commas
  ("2,542"), the host's small font, left-aligned with the row, its bottom 1 px above the (thick) POC row; never on an HVN. The developing
  pullback label follows its POC. A label on another Wave POC text (P / H letters, tags, another label) moves up one label height (max 3).
  Letters drawn by OTHER plugins (lsBreakout BO / Rly / Pb / Rev) are not visible to Wave POC, so they cannot be avoided from here.
