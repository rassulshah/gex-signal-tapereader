# lsHodLod 2.39.1 - gamma pill at the far left, arrows clear of the text (2026-11-20)

## Request
- Rassul 09:32 CT 2026-11-20 (ES screenshot, HodLod 2.39.0 live): "the gamma badge should be more aligned to the left so you will have
  more space for the options line and fix the arrows because they are overlapping".

## What changed
- Gamma row: the regime pill ("-GAMMA · Trending · Bk / BkTst") + "0DTE Exp 3:00 PM" at the FAR LEFT, starting at the HOD / LOD box's
  left edge (as the 2.37.x tag did). The gamma line (badge | G V C | Next) no longer carries the pill: it stays centred on the box and
  gets the freed space. If the pill would come within 10 px of it, the line moves right only as far as needed; if it then runs past the
  pane, the next narrow level is used. Narrow order: 0DTE, the pill's setups, Next, G V C, the regime word, the pill; the badge stays.
- Arrows (Now line + gamma line, every segment: badges, Next, Last Bk, G V C): each arrow owns a slot 4 px | 9 px ink (shaft + heads
  incl. the 2-px pen) | 4 px, counted in the measured width. Same arrow size / strokes, vertically centred on the text row.
- Root cause of the overlap: IRT's left-justified drawText puts the text ~4 px right of rect.left (BUILD-LESSONS 2026-11-03, Dealer
  Profile 2.52.1), so each word ran 4 px into the arrow after it; the G V C slot had only 1 px. The Now / gamma line text is now drawn
  CENTRED in its measured box by IRT itself (host textIn, as DealerProfile textC) - no dependence on the inset.
- Status: GAMMA_LINE "narrow:" list follows the new order; NOW_ROW says the pill is at the far left of the gamma row.
- Versions: HL_VERSION = setVersion = PLUGIN_VERSION = 2.39.1 + header comment; GammaLine 1.1.0, NowLevels 1.3.1. No settings change.
  gex-build unchanged (its HodLod line already lists GammaLine.h + NowLevels.h).

## Tests
- tests_hodlod/t_hl2391.cpp 39 / 0 (ASan / UBSan, -Werror): his screenshot's strings ("GAMMA WATCH: 7,150↓ · 7,200↑ · Break │ G↑ V– C–",
  "Next↓ 2hH · ↑ 3hH", "Last Bk↑ ALO") with Liberation Sans (Arial metrics) widths at 9 pt and 10 pt: every arrow ink box >= 3 px clear
  of every glyph box (min 4), vertically centred, inside its badge fill, black on the gold badges / green / gold on dark; no glyph
  boxes overlap; pill + 0DTE at the box's left edge; widths 100..1600 px x every box centre: the pill never within 10 px of the line,
  the line shifted only as much as needed, drop order 0123456; host uses textIn for both lines; versions.
  Mutation check: arrow space 0 -> 4 checks fail (min gap 0 px).
- t_hl2390 75 / 0 (narrow test updated to the new order / the shift; badge-only line ~10 px wider), t_hl2380 35 / 0 (NowLevels 1.3.x).
- t_hl2372 / 2371 / 2370 / 2360 / 2350 / 240: identical failure sets to the untouched 2.39.0 sources (old version pins) - no new failure.
- MinGW -fsyntax-only vs the real irtsdk.h: 0 errors; only the old first_extreme_inference.cpp warning (same as 2.39.0).
- Render: /mnt/user-data/outputs/hl2391-lines.png (Now + gamma lines at 9 / 10 pt, drawn from the logic's draw list).

## Risks / not verified
- Not seen on his IRT yet: installs when he exits IRT; then a screenshot (ES + NQ) for the pill position and the arrows.
- IRT's centred drawText rounding may move a word by 1 px; the 4 px arrow spaces leave >= 3 px either way.
