# 4-step Reversal Stage revamp - 2026-10-23 (Rassul approved the design 05:47-05:50 CT, chose option B "build now, mockup after" 05:55 CT)

Built before RTH. The DLLs install at his next IRT restart (File -> Exit, wait for "installed" in plugin\out\BUILD-STATUS.txt, reopen).

## What changed

| Area | Change | Versions |
|---|---|---|
| The model | 6 stages -> a GATE + 4 steps + a RESULT line. **Gate**: an attempt (the new 5-candle low / high) is tracked only when it trades into a meaningful location: one of the last 3 intact swing lows / highs of today's RTH ZigZag (SwingRef.h rule: 2 back only if beyond 1 back, 3 back only if beyond both; confirmed before the candle; intact), a key level (ONL PDL PFL LonLO PWL AsiaLO / highs) or a MenthorQ level (LRA-KeyLevels / LRA-Dealer / LRA-Levels, the same files and 18 h freshness rule as lsSweepRefill), or a live queue zone (a typed held Q at the extreme). Zone = from max(4 ticks, 0.3 ATR) before the level to max(that, 0.5 ATR) through it. **1 Trap**: P stacking (defenders' book) or a Q held before contact. Fail: W / B before the floor was tested. **2 Collision**: an attack S / K / L at the zone AND a result A / Q held on the same candle or within 1 candle; Trap alone never confirms. Fail: Q! (the queue at the extreme broke / drained). **3 Trapped**: E, Tr, G, the new D (within 5 candles of the collision, within max(tol, 0.5 ATR) of the extreme), or the attackers underwater >= 0.25 ATR (close vs the collision's average fill). Fail: a new extreme beyond the floor. **4 Release**: H or R, then L re-test held (a flag, the step stays 4). Fail: X. **Result**: the close 1 ATR from the extreme after a release. | ReversalStageLogic.h / lsReversalStage **2.0.0** |
| Never repaint | Kept: decided candles are frozen; the RS3 record carries the full post-bar checkpoint (incl. the gate, the collision candle, the attack's average fill and lots, the step texts). New file RS3-... (the 1.x RS2 steps meant something else and are not adopted). | RS3 record |
| Now box | "LOW at 7,696.00 (swing low 8:48 AM)" / "Trap: bids 2.3x, Q 143" / "Collision: 1,840 sold, held (A, Q)" / "Trapped: sellers -6 ticks, 1,840 lots, D" / "Release: waiting for H / R" / evidence readiness / "Odds ?". 2 - 3 columns when the pane is too short (never cut). Hover: the gate + the 4 step texts + letters + use + example. | lsReversalStage 2.0.0 |
| Strips | Names only: attacker "Attack" / "Trapped" (was Running out); Defense "Floor" (was Holding) / "Release" (was Turn & re-test). New attacker letter **D** in Trapped (R > Tr > D > E > G). Letters keep their meanings. | LetterStrips.h **1.3.0** |
| D | MBO CVD divergence: the order feed's classified aggressive totals (buyers - sellers, implied fills excluded - the 1.29.0 rule), cumulative over this + the 5 candles before; LOW = the CVD makes a new low on this candle while its low is HIGHER than the window's lowest (mirror for highs). Any candle without complete order-feed totals = no call. Typed EV-ATK row (MBO, COMPLETE only). Hover on the strip explains it. | lsBidAskStats **1.32.0** |
| Underwater | At each finalized candle lsBidAskStats keeps the aggressive fills' average price per side (each aggressor order at the middle of its lowest / highest fill, size-weighted) and appends it as a hidden " vw<price x100 in ticks>" token to the S / K / L strip numbers; the stage engine takes it from the collision's attack. After an IRT restart the old candles have no token = no underwater claim (honest n/a). Shown in the Now box / hover, not as a letter. | lsBidAskStats 1.32.0 |
| Defense pane | Strip names only. | lsBookFlow **1.27.0** |
| gex-build.bat | ReversalStage now also hashes SwingRef.h and NearLevels.h (CRLF kept). | - |

## Not wired yet (said plainly)
- **Sr refill** (Collision result) and **Sta** stop run absorbed (Trapped): their records live in lsSweepRefill / lsStopRun files, not in the typed EV files the stage reads. Next step: emit typed EV rows from those two plugins.
- **Volume-profile gate levels** (POC / VAH / VAL / naked POCs): need VAP in the stage pane. Swings + key + MenthorQ + queue zones gate now.
- **Day type / news "?"**: the existing NewsWindow "?" on the steps is kept; no day-type flag added.
- The underwater price is an approximation per aggressor order (middle of its fill range), not a per-fill VWAP.
- Odds stay "?" (T-20261022-39). Nothing here is a proven edge; the nightly reads RS3.

## Tests (Linux, ASan/UBSan, -Werror; MinGW real irtsdk.h -Dsmall=char)
- tests_strips: test_strips 1301, **test_stage_core 117** (new: gate on/off, key / MenthorQ / swing / last-3 / intact / no look-ahead / queue-zone gate; Trap P / Q, Trap alone never confirms, W / B fail; Collision same / next candle, 2 candles apart = none, Q held after contact, Q at the own extreme only, Q! fail at Trap and Collision; Trapped E / D (5-candle window, at the gate, needs a collision) / underwater 3 ticks / no token = no claim / new-low fail; Release H / R / L held / X fail, release needs a collision; result 1 ATR; typed-evidence and data-gap rules; RS3 round trip; restart after every candle = one straight pass; Now box / hover text; LetterStrips names / D / cvdDiv), evidence transport 45, **stage pipeline 18** (plugin + mock SDK: gate from a key-level file, collision, release, re-test, restart, contract isolation), **ES 10-22 replay**: 88 swing gates, 32 gated attempts, prefix and restart replays 0 differences.
- tests_aggbook --full: all targets 0 failed (BidAskStats host 71, BookFlow host, Big Lot host 30, attack host, Defense Lines, trails, dependency checks 346).
- Settings contract 40,909 checks, 0 failures; settings host 1 old failure ("HodLod parameter version 7", already on his files).
- MinGW real SDK: ReversalStage, BidAskStats, BookFlow, DefenseLines - 0 errors, no warnings outside the SDK.

## What to watch live (after the IRT restart)
- The stage pane starts empty (new RS3 record). Status line GATE shows how many swings / key / MenthorQ levels gate.
- "No attempt at a level" in the Now box = a 5-candle extreme away from every gate location (by design).
- D letters on the attacker pane's Trapped strip; "sellers -N ticks" only for candles finalized after the restart.
- Not verified - needs his IRT: the Now box columns at his pane height, the strip labels, D frequency per market.
