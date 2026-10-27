# lsBookFlow 1.30.0 + lsBidAskStats 1.36.0 (2026-10-26 evening, Rassul 18:33 CT "remove all the signal letters")

- Defense pane ("Are they defending?", lsBookFlow): the two letter strips (Q, Q!, L?, Ab, B, Hb?, W, P, X?, F? ...) are not drawn; their
  rows go to the bars (topReserve 0). The turn % marks over the bars stay (unchanged rule: on a turn candle, how much bigger the
  defending side's Defense bar is). Letters still decided, written to lsFlexLevels\Strips\DEF-<MKT>-<spb>.csv, the LT / EV records
  and the status file.
- Aggression pane ("Who is aggressive?", lsBidAskStats): the S strip, the bar letters and the stop-run R / R? mark are not drawn; the
  strip row goes to the bars (BAS_BADGE_H 0); the "S letter" settings row hidden (parameter version 8, file field kept). Letters still
  decided and written (ATK strips file, records, stop-run record).
- The strip hovers went with the strips (nothing to hover); the numbers stay in the records and status files.
- NOT in this build: the proposed aggression % (on hold for Rassul's pick).
