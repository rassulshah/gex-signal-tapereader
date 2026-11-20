# lsHodLod 2.37.1 - gamma pill label (2026-11-19, 22:35 CT)

## Request
Rassul approved at 22:35 CT, in his own words, the rows of gamma-tag-mockup.html under "YOUR IDEA, once the forward test proves it",
shown WITHOUT "?". The gamma pill at the far left of the Now row gets a label:

- negative gamma: `-GAMMA · Trending · Bk / BkTst` (red outline and red text, as the pill is today)
- positive gamma: `+GAMMA · Range · Sw / Cob` (green)

The middle dot is cp1252 0xB7, the same as elsewhere in HodLod. "0DTE Exp 3:00 PM" follows as today. No other changes.

## What changed
- HodLodTop.h 1.1.0:
  - `gammaTag(pill)` returns the label forms, longest first: full, then `-GAMMA · Trending` / `+GAMMA · Range`, then the word only. "GAMMA STALE" and an unknown regime are unchanged.
  - `nowLeftTag(...)` draws the whole label inside the pill, then the expiry.
  - On a narrow pane it drops parts in this order: the expiry first, then ` · Bk / BkTst` / ` · Sw / Cob`, then ` · Trending` / ` · Range`. The +/-GAMMA word always stays. As in 2.37.0, it goes only when even the word would come within 10 px of the Now line.
  - The 1.0.0 `nowLeft` is unchanged; nowLeftTag uses it.
- HodLod.cpp (host): calls `nowLeftTag(gammaTag(pill.s), ...)`. The status line now reads `NOW_ROW,1.1.0,left <gamma tag + 0DTE expiry | gamma tag (expiry dropped) | gamma tag short | gamma word only | none | not drawn>,tag <text>`.
- Versions: HL_VERSION = PLUGIN_VERSION = setVersion = header comment = 2.37.1. The parameter list is unchanged, so setParameterVersion is unchanged too.
- gex-build.bat is unchanged: its HodLod line already lists HodLodTop.h.

Widths at 10 pt bold: the full negative label is 203 px, so the pill is 215 px. The full positive label is 181 px. The expiry is 117 px. With the expiry, the left part needs about 340 px left of the Now line.

## Tests
- tests_hodlod/t_hl2371.cpp: 37 checks, 0 failed (`-Wall -Wextra -Werror`, ASan/UBSan). It covers:
  - Both regimes render the exact strings: one 0xB7 byte, no UTF-8, no "?".
  - The whole label sits inside the pill, in the regime colour, and the expiry follows.
  - The drop order, swept pixel by pixel. All five steps are seen, the label never returns to a longer form as room shrinks, and nothing comes within 10 px of the Now line.
  - The price-axis limit.
  - The real centred Now line over box widths 260..1600 px, every 2 px, both regimes: no overlap, and the line stays centred in the mockup case.
  - No regime and GAMMA STALE are unchanged.
  - The host call, the status line, the versions, and the gex-build line.
- t_hl2370: 5,884 / 0 failed. Only its version strings were updated to 2.37.1.
- MinGW `-fsyntax-only` against the real irtsdk.h: 0 errors. The only warning outside the SDK is the existing one in first_extreme_inference.cpp.

## Differences from the mockup
- None intended. The label is inside one pill, as in the mockup's rows, with the expiry after it.
- The pill keeps today's 10 pt bold font and its dark fill.

## Not verified - needs his IRT
- The look on his charts after the restart. Screenshots are needed in 2+ markets.
