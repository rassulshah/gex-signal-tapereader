# ZZO Combo 2.1.0 / ZZO ZigZag 1.1.0 - 2026-11-03 (built 14:00-15:00 CT)

## Requests
- 13:59 CT "make it 3bar using high and low by default" (ZZO Combo); earlier "allow an option to select 1bar or 3 bar for the indicators like the combo zig zag"
- 14:02 CT remove the "weak" tag from the Combo pane
- 14:02 CT the count must compare a wave only with PRIOR waves of the SAME colour, on the measure the bar height shows ("why is there 50+ on green waves that are not bigger than the last 50+ green waves")
- 14:03 CT "move the count 1 more to the left"

## What changed
- ZzoDeltaLogic.h: `Engine::revBars` (1 = old 1-bar walk, default; `COMBO_REV_BARS` = 3). 3-bar rule = IRT's ZIGZAG "Min Price Change = 3 bars, Use High/Low",
  the same rule as lsWavePOC 1.3.2: an up wave turns down only on a bar whose LOW is below the lows of the previous 3 bars (down wave: HIGH above the
  previous 3 highs); a strictly higher high / lower low moves the extreme; equal does not. Only highs / lows are read.
- Count (`countBack`): walks back over the waves the engine holds (overnight and earlier days included), same colour only, skipping waves with no
  normal, counting the ones whose volume RATIO (wave volume / median of the last 5 same-way waves = the ramp height drawn) is below this wave's; stops at
  the first >= one; 50+ cap. Was raw volume (quiet overnight waves made ordinary RTH waves "50+"). History limit: after 6,000 bars the engine trims and
  keeps the last 40 older waves for normals, so after very long charts the walk reaches back ~40 + the kept waves (unchanged design).
- ZzoComboView.h: "weak" never drawn (still computed, in the status file as ",weak"); count right edge = wave's right edge - one bar width; ratio label
  unchanged (centred on the wave's last bar - a different anchor). Status file: new ZIGZAG line ("3-bar reversal, High/Low").
- ZzoCombo.cpp 2.1.0 and ZzoZigZag.cpp 1.1.0: set `revBars = COMBO_REV_BARS` (the ZigZag overlay draws Combo's waves, so it follows Combo).
  ZZO Delta 1.3.2 / ZZO Volume 1.2.2 unchanged (1-bar; rebuilt because the shared header changed, same behaviour, same versions).
- Option 1-bar / 3-bar: these plugins have NO settings dialog (SS_SAFE_DIALOG, no parameters), so no chart setting was added (a dialog would not work).
  The choice is the compiled constant `zzd::COMBO_REV_BARS` (3); the engine supports 1 or 3.
- Nightly: lra.zzo_model 1.2.0 (`run(..., rev_bars=1)`, `COMBO_REV_BARS`, same-colour ratio count), lra.zzo_score 1.2.0 replays Combo's 3-bar waves.

## Tests (g++ -fsanitize=address,undefined -Werror -Wall -Wextra)
- tests_zzodelta/test_zzodelta 116/0, test_zzodelta_ref 34/0, test_zzocombo 55/0 (ref_zzc_* regenerated from lra.zzo_model 1.2.0 - new count rule)
- tests_zzodelta/test_zzocombo_210 46/0: synthetic 1-/2-bar pullbacks do not turn the 3-bar wave, the bar under the previous 3 lows does (and mirror);
  1-bar engine still turns on the first lower low; highs/lows only; = lsWavePOC's IRT ZIGZAG rule pivot for pivot on ES (529/529); = Python replica
  wave for wave (530/530); count stops at an earlier bigger green, reds never count vs greens and back, ratio not raw volume, 50+ cap; no "weak"
  drawn; count x one bar left; ratio x unchanged; hosts set the 3-bar walk; version literals.
- analytics/lra/test_zzo_score.py 6/0 (new: 3-bar + count vs the C++ reference ref_zzc3_es_1103.txt)
- MinGW real irtsdk.h -fsyntax-only: ZzoCombo / ZzoZigZag / ZzoDelta / ZzoVolume 0 errors, 0 warnings
- No mock-SDK host test exists for the ZZO plugins (host changes are one line each).

## ES 2026-11-03 (fixture zzc_es_1103.csv)
  ES 2026-11-03 waves (3-bar High/Low ZigZag; pivot bar START CT; size pts; vol ratio; count):
    8:45 AM  UP  29.00  3.2x  
    9:00 AM  DN  16.25  0.8x   (weak, not drawn)
    9:15 AM  UP  18.25  1.9x  
    9:30 AM  DN  11.00  0.8x  
    10:15 AM UP  27.50  2.0x  1
    10:48 AM DN  21.25  1.7x  2 (weak, not drawn)
    10:57 AM UP  11.00  0.2x  
    11:03 AM DN   8.75  0.3x  
    11:09 AM UP   9.50  0.2x  
    11:21 AM DN   7.50  0.4x  1
    11:27 AM UP   9.75  0.1x  
    11:36 AM DN  12.00  0.3x  
    12:12 PM UP  22.25  3.5x  6
    12:21 PM DN   9.25  0.8x  3
    12:33 PM UP  11.75  1.3x  
    12:48 PM DN  12.00  1.6x  4
    1:00 PM  UP   5.75  0.8x  
    1:12 PM  DN  15.75  1.6x  5
    1:18 PM  UP  11.25  0.7x  
  Nov 3 session: 70 waves (19 RTH), average 11.46 pts
  same session with the 1-bar walk: 138 waves

## Not verified - needs his IRT
Close IRT -> gex-build installs lsZzoCombo 2.1.0 / lsZzoZigZag 1.1.0 -> reopen; check the Combo header shows "ZZO Combo 2.1.0" and
lsFlexLevels\ZzoCombo.status-ES-180.txt has "ZIGZAG,3-bar reversal, High/Low". Compare with IRT's own ZIGZAG (3 bars, High/Low) on the price chart.
