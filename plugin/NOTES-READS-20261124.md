# Reads that conclude - lsTurn 1.11.0, lsRevRead 0.31.0 (BsRead 2.0.0), lsVolRead / lsRangeRead 1.15.0 (VRRead 1.6.0), TurnVRead.h 1.0.0

Rassul 2026-11-24 21:23 CT: "i dont want repetitive information in the read . for exampel in the volume it indicates in the read that the biggest
green bar in 99+ candles, which is something i already know by looking at the candle. instead it should be either an important point, a conclusion
or a verdict." Designed line by line 21:37-22:39 on reads-conclude-mockup.html (v30), then "everything else is a go".

## The rules
- No line repeats a bar, a bar label or a number drawn on the pane; a line shows only when it says something; nothing twice across the three reads.
- No "?", no odds lines, no time on the title, no "Did price agree?" line in Turn, no POC line. Grey question column; "5 How?" is now "5 Tactics?".

## 1. Volume read (title = the OVERALL CALL of all three panes)
- Turn decides the side (Initiative / passive defense = the end it held; aggressor = through it); buy / sell and the volume's own lean
  (effort vs result) confirm: both = "Strong bullish - buyers took the low on heavy volume", one = "Bullish - ...", none = "Mixed - ...".
  No turn: "Mixed - buyers trapped, no turn yet", else the volume's lean, else "Neutral - ordinary volume" / "Neutral - light volume".
- 1 How much?: "Effort matched result - price followed through." / "- price gave it back." / "Effort without result - but price broke the low."
  / "Result without effort." (price agreement = 1 ATR the expected way, or more than 1 tick through the held extreme, within 10 candles).
- 4 How big?: "Large buyers outnumbered sellers 6 to 1 - but large offers wait over the high." Large market order = top 1% of the order sizes
  (fills grouped by aggressor ID) of the 20 candles before, per market; large resting order = top 0.1% of new resting order sizes (GC about 15 lots;
  the top 1% was 5 lots - 100+ a candle). A S20 / S10 / S5 order is named ("; a 79-lot seller (S5)"). "Large buyers have led for 4 candles (20 to 12)."
- 5 Tactics?: "A buyer sliced 35 lots into 7-lot pieces." (same size 6+ lots, 4+ orders within 30 s, adding up to S20+) / "Offers pulled before the push."
- Where it comes from: lsTurn writes TurnPane\vread-<MKT>-<spb>.txt (each closed candle: Turn kind / end, large orders, sliced, pulled);
  lsRevRead writes RevRead\bsv-<MKT>-<spb>.txt (buy / sell verdict + trapped side). Without them the title comes from the volume alone.

## 2. Buy / sell read (BsRead 2.0.0)
- Title: "Buyers took over at the low" / "Buyers took over - the delta flipped 3.7x bullish" / "Sellers hit harder but could not keep the low" /
  "Two-sided - no one in control".
- "Delta and price agreed." or "Buyers hit hard but price fell - buyers trapped." (|delta| 1.5x+ normal, closed 0.3 ATR+ the other way).
- "Strong buyers flipped delta 3.2x bullish." "Selling dried up over 3 candles." The big-order share line moved to the Volume read.

## 3. Turn read (1.11.0)
- Title: "Initiative - buyers lifted the offer at the bottom" (+ " at LonLO" only when a key level is in that end quarter) / "Passive defense won -
  sellers absorbed at the bottom" / "Aggressor won - ..." / "No absorption or initiative at the ends".
- 3: "Buyers won the low about 5 to 1." / "- sellers barely there." / "Bids took the selling - the bottom held at the close."; exhaustion
  "Sellers ran out at the low." (the attackers' lots at the 2 extreme prices under 0.15x the candle's average per price).
- 5 Tactics?: "Urgent: one buy order swept 4 prices." / "Urgent: buyers fired at 11.0x their normal pace." / "Stops ran below LonLO, then bids
  absorbed them." (stop lots in the end quarter 1.0x+ the market's typical stop run) / "Iceberg: bids refilled 3 times (20 lots) at the low."

## Tests (all pass; ASan / UBSan, -Werror; real SDK syntax check 0 warnings outside the SDK for Turn / VolRead / RangeRead; RevRead same 6 old warnings)
- tests_turn: all1100.sh (3,287,189 logic checks, golden 2,017, host 36,639 incl. the vread file, soaks, t_read1110 23 new).
- tests_revread: t_bsread_200 (27, the mockup's real GC 9:12 / 9:21 / 9:30 / 10:06 candles); t_bsread_100 superseded (stub).
- tests_volrange: t_vr1150 (24, the mockup's candles); t_vr150_split / 180 / 190 / 1100 / 1110 / test_volrange_130 pass; t_vr1120 superseded (stub).

## Differences from the mockup, and risks (not verified - needs his IRT)
- The Iceberg line uses the real refills in the end quarter (count + lots), not the put-back number the mockup showed (243).
- 9:12 also reads "Selling dried up over 3 candles." (sellers 1,125 > 638 > 376 > 335 into a new low) - a real fact the mockup did not show.
- ES has 30-100 large market orders a candle by the top-1% rule (GC about 5); the line reads the same way.
- None of the new conclusions is measured yet (no "?" by his rule); the nightly measures them.
- After the restart the overall call needs lsTurn and lsRevRead on the same market's 3-min chart (they write the files the Volume read uses).
