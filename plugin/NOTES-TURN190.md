# lsTurn 1.9.0 + lsVolRead / lsRangeRead 1.13.0 - the volume model's layers 4 and 6 (2026-11-24)
Requests: 19:59 "So have the indicators implemented the volume model"; 20:00 "Go"; 20:02 "Will the volume model be implemented".
- Layer 4 MAGNITUDE: the biggest market orders carry their Big Lot tier (BigTier.h, his agreed S20 / S10 / S5 per market, RTH vs overnight):
  Turn read "Biggest 41 (S20)" (How line) and "biggest print 60 (S5)" (Attack line); Volume read "Biggest orders: buyers 45 (S10), sellers 31."
- Layer 6 PRICE AGREEMENT: a "Price" line under the Turn read (not on a Quiet candle), the first of the next 10 closed candles:
  "agreed - went 1 ATR (8.3) up by 9:33 AM." / "failed - traded through 3527.7 at 9:36 AM." (a hold: Initiative, passive defense) /
  "failed - came back 1 ATR by ..." (the aggressor got through) / "held 3 candles so far." / "waiting for the next candle." / "neither ... in 10 candles."
  ATR = Wilder 13 of the candles before. A read text, decided from closed candles only.
- Tests: Turn logic 3,287,189; golden 2,017; host 34,312 (the Price line on the Initiative hover reads); soak OK; read 136 / 262; abs 31; cr 15; ini 31;
  t_agree190 19; t_bigtier 6. Volume: t_vr1100 37, t_vr1110 28, t_vr150_split 69, t_vr180 26, t_vr190 58, test_volrange_130 26, t_vr1120 25. Real SDK: 0 errors, 0 new warnings.
- Not verified: his charts (after the restart).
