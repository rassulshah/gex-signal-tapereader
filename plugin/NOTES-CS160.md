# lsCobSignal 1.6.0 (2026-11-06 18:50 CT)

Rassul (GC / HG 3-min): "this is the cob for gold, you need to use 3bar", "why does the cob still not on the gold and hg charts",
and 2026-11-01 "the cob should be a completed wave, not mid way".

## Rule (CobSignalLogic.h checkWave, the same in lra.trade_def.cob_wave3 for the Reader / Trade Now)
- The CoB = a COMPLETED 3-bar High / Low ZigZag wave (the chart ZigZag / ZZO Combo wave, zzd::Engine, COMBO_REV_BARS).
- Bull: the wave's start pivot low is below a key level (ONL / PDL / PFL / LonLO / PWL / AsiaLO / PhrL) and its high is back above it. Bear is the mirror.
- Size >= 1.9 x the average of the previous 20 completed waves of its direction (>= 5); >= 3 bars; start pivot an RTH bar of the session;
  room |close - start| >= 0.1 ADR; decided on the bar that confirms the wave, by 11:00 CT.
- Drawn only when the wave's ZZO Combo count >= 10. The label sits on the confirming bar; Fib lines run from the start pivot (0) to the wave end (1.0).
- The 1-bar pause CoB is off (wave3 = true). LVP now starts from the completed CoB wave.

## Today (replay)
| Market | CoB | Wave | Count (chart) | Drawn |
|---|---|---|---|---|
| GC | 08:00 bull | 07:33 -> 07:54 | 9 | no |
| GC | 09:30 bull | 08:39 (ONL sweep) -> 09:18 | 50+ | yes |
| HG | 08:57 bull | 08:18 (PDL / PhrL) -> 08:48 | 31 | yes |

## Tests
- test_cobsignal_fib 29/29, test_cobsignal_lvp 30/30.
- test_cobsignal_host 31 passed / 3 expected fails: two encode the old 1-bar CoB set (PHH/PHL count, CL phOnly), one reads ../CobSignal.cpp from the build dir.
- MinGW against irtsdk.h: 0 errors.
