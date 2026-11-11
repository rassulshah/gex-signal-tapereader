
## HodLod 2.26.0 / ReadBox 1.4.0 / BookFlow 1.40.0 (2026-11-11 afternoon CT) - the Book line
Rassul: "Can you include 4 because I want to use it to enter after a pullback when I am looking for a reversal"; "I am wondering if it makes sense
to have two reads because we have one at the top too"; "Ok build".
- Read box = two lines: "Flow: ..." and "Book: ..." (rbx::bookLine / applyBook). Book reads the side that matters: hourly trend Up -> bids, Down ->
  offers; price testing / sweeping a key level against the trend -> "Reversal at <level>:" and the side that would make the turn; no trend -> the
  level's side or the stronger side. "defending" = ratio >= 2.0x arriving, "pulling" = leaving >= 1.5x, else "not defending (B x, S x)".
  Same colour as Flow when they agree; Book white when they disagree. No lsBookFlow on the chart = no Book line.
- lsBookFlow 1.40.0: header is plain "B 2.3x . S 0.3x" (no "Defending?"); new SignalBoard fact F (bf::ratioFact) = the defend ratios of the last
  200 decided candles + the forming candle; ChainSpec.h lists F.
- HodLod re-reads fact F once a second at draw (the forming candle when there).
- Tests: tests_readbox/test_readbox_140.cpp 13 pass, test_readbox_130 19 pass, t_hl2250 13 pass; MinGW 0 errors; settings contract 45,949 pass.
- No BookFlow test rig locally: BookFlow 1.40.0 is compile + contract checked only. The 2.0x / 1.5x lines are not measured.
