# lsRevRead / lsRevReadMarks 0.19.3 (2026-11-19)

Rassul 13:29 CT: "its hard for me to see the white lines so lets make them black instead so bk and other badges will have black writing instead".

- The candle tags drawn by lsRevReadMarks (I / Bk / Sw / Cx: code, level, strength lines) are written in BLACK (rvh::C_TAG_TEXT 0x000000)
  instead of white. Fills unchanged: bright green #22C55E (bullish) / bright red #EF4444 (bearish) - both read clearly with black text.
- The BO indicator's Rly level line is coloured text with a black outline (not white-on-colour) - unchanged, no BO rebuild.
- MinGW syntax check vs the real SDK: RevRead.cpp / RevReadMarks.cpp clean. Logic headers unchanged (tests unaffected).
- Not verified on his chart - needs an IRT restart.
