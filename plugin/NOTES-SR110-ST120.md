# lsSweepRefill 1.1.0 + lsStopRun 1.2.0 (SwingRef.h 1.0.0) - 2026-10-22 evening

## Requests (Rassul, CT)
- 17:39 approved the Sr mockup v4 rule (sweep-refill-mockup-v4.png) and "the SAME swing rule on St / Sta".
- 18:05 St / Sta use the SAME reference as Sr: an intact swing among the last 3 ZigZag swings, or a key / MenthorQ level (not "most recent only").
- 18:07 St, Sta and Sr also count VOLUME PROFILE levels: POC / VAH / VAL of the current session and the prior day, plus naked prior POCs.
- Validation verdicts (VALIDATE-STOPRUN-SWEEP-2026-10-22.md, applied per market from lsFlexLevels\LRA-StopSweep-<MKT>.csv):
  Sr SHOW NQ bull + GC bull, "?" NQ bear; Sta "?" NQ bull; GC grey St "?"; everything else LOG (recorded, not drawn).

## What changed
- SwingRef.h (new, SDK-free, shared): the 1-bar ZigZag from the RTH open (rly2.py rules, outside-candle tie); the reference pick
  (last 3 swings with the beyond rule, intact before the last candle, starts <= 0.5 ATR past, deepest; else key / MenthorQ /
  volume-profile level intact over the 5 pre candles); ATR = mean true range of 14; the volume profile (klv::profileOf rule, 70% VA);
  the params file parser (strict; missing / garbled = every mark LOG).
- lsSweepRefill 1.1.0: refill = 50% (orders resting 1 s+) at some check within 5 s AND still there at 5 + stay s (NQ 5, GC 2);
  min prices per market (NQ 8, GC 3, ES 4, CL 6); the 15-min local-extreme gate and the min-lots gate are gone (not in the locked
  rule); green Sr UNDER the wick, red OVER; "Sr?" for "?"; short line to the swing; the ZigZag line (setting + colour); hover names the
  swing / level; the verdict is stamped on each mark (record field `show`) so a later params change never repaints; VAP_REQUIRED.
  Settings: min prices / lots / refill / window removed (params file); added Show ZigZag + colour, Line to the swing taken out.
- lsStopRun 1.2.0: the same reference (+ at most near-ticks before it); burst >= 3.0x; Sta LIVE at 10 s = last trade back across +
  a resting order >= staBigLots filled at / beyond (NQ 5); candle-close mode for staDecideLive 0; "50% eaten" and "no new extreme"
  removed; St? / Sta? text; verdicts stamped (ST `show`, SA `show`); a candle with a drawn Sr draws only the Sr (reads the Sr record);
  VAP_REQUIRED. Settings: the detection numbers and level switches moved to the params file (min orders, colours, font, show kept).

## Tests
- tests_sr: logic 62 / host 32, MinGW real irtsdk.h -Dsmall=char 0 errors / 0 warnings; replay_sr with a restart check after every
  candle (fresh ZigZag + profile == running, record reads back byte-identical).
- tests_st: logic 20 / host 24, MinGW real SDK clean; replay_st on NQ 10-09.
- Settings contract: SweepRefill + StopRunMark tables PASS (StopRunMark added to the contract).

## Risks / not verified (needs his IRT)
- VAP read for up to 2,000 history candles at the first pass (two plugins) - check Calc Time after the restart.
- The 2-lot pre-sweep case (NQ 10-09 8:38:22 through HVL0, 1500%) passes the locked rule (no min size in it) - flagged to research.
- Screenshots in 2+ markets after his restart (ChartView export).
