# lsBigBidAsk 1.26.0 - OB / BO as prints + letters (2026-10-22 evening)

Request (Rassul 18:01 CT): "you only put the volume at the bid or ask like we currently do at the bo or the second high volume bid or
ask if it exists near the pivot using the pivot rule, its as simple as that".

## What changed
- The 1.25.0 OB / BO RECTANGLES are gone (no boxes, zones, extended bands). Resting-limit zones come only from lsLiquidityProfile LMT zones.
- BO: the existing breakout signal (bba::boWaves: 10-candle extreme, range / volume, green / red, same-wave rule - unchanged) now breaks
  one of the LAST 3 INTACT swings of the 1-bar ZigZag (new bba::swingBreaksPx; was "the higher of the last 2 pivots"). Its print as
  before. Letter "BO" for both sides: green ABOVE a breakout candle, red BELOW a breakdown candle (no more "BD").
  BBO record lines written before 1.26.0 (old pivot rule) are not read back: those candles are decided again under the new rule.
- OB (OrderBlockLogic.h 2.0.0): for a strong wave (an Rly wave, or a confirmed wave >= 3.0 ATR - unchanged), the highest-volume bid / ask
  cell on the pivot candle + the next 1-3 candles (setting "OB candles after pivot", default 2); when a BO mark on those candles already
  shows that print, the 2nd highest (if any). Drawn like a print (lots in the side's box) + "OB" BELOW an up-wave candle / ABOVE a
  down-wave candle. Decided once when the wave is confirmed (OBX| line, new field `rank` 1 / 2 / 0), never repainted; visits to the
  OB price logged (OBT|) for the nightly. 1.25.0 box lines are ignored (counted on the status file). Skip first / last minutes apply.
- Hover on the OB letter: question / what / why / how / example, "?" = not proven.
- Settings: "Breakout (BO) boxes" row removed; "Order block (OB)" + "OB on 3+ ATR waves"; "OB candles after pivot" 1-3; BO up / down,
  OB up / down colours. Parameter version 13. Settings contract: PASS (39,730 checks, 0 failures).

## Tests
- tests_bba115/test_bba126.cpp: 55 / 0 - intact-swing rule (last 3 only, wick-consumed, mirror, prefix runs), OB pick (top / 2nd /
  none), record (rank, legacy refused), window 1-3; ES 2026-10-05 + NQ 2026-10-09 3-min with full bid / ask volume at price from MBO
  trades (tests_ob126/make_fixture.py): no rectangles in OB / BO colours, no BD, BO / OB letter sides, OB print box present, no
  overlaps, OB setting off, restart after EVERY candle 8:30-12:00 = identical OBX / BBO record, legacy 1.25.0 line ignored.
- test_bba115 / 116 / 119 + tests_bigba 114 pass; 116 / 119 version checks unpinned (they had been stuck on 1.21-1.23).
- test_bba121 / 123 host checks pin the old pivot rule + BD letter on the NQ 10-21 mockup: superseded, kept, not run.
- MinGW real irtsdk.h (-Dsmall=char -Dfar= -Dnear=): 0 errors, 0 warnings outside the SDK.

## Replay labels (RTH)
ES 10-05: OB 8:48 (Rly, 647 ask), 10:09 (585 bid), 11:24 (586 bid), 12:39 (3ATR, 590 ask), 1:06 (626 ask), 1:51 (727 bid), 2:15 (495 bid),
2:18 down (715 ask), 2:42 down (1198 bid); 8:33 recorded but in the skipped first 15 min. BO 9:57, 11:21, 12:42, 1:24, 1:57, 2:12 up;
2:39 down (8:27 skipped).
NQ 10-09: OB 11:48 up (78 ask), 11:51 down (70 bid), 1:33 up (57 ask), 2:21 down (63 ask); 8:36 / 8:39 skipped. BO 10:51, 1:09, 1:18,
1:48 up; 2:33 down.

## Risks / not verified
- Not seen on his IRT yet: check the 3-min ES / NQ charts after the restart (OB / BO letters, no boxes).
- The 1.25.0 study found the OB / BO areas a coin flip; the letters are shown on his request with "?" in the hover; the nightly scores them.
