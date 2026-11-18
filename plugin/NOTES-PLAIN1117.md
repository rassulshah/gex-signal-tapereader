# Plain pane reads (2026-11-17 21:17 CT)

Rassul 21:15 "i wnat a more human readable and simplified format for these reads and data points. you can keep the datapoints silently in the background, but its not something that is usefulle for me because i need something practical"; 21:17 "i like it. lets do this" (mockup pane-reads-simple-mockup.html).

Every pane read is now three short lines, no numbers table:
1. who is winning (green / red / white mixed / grey quiet, "?" until proven)
2. why, in plain words
3. what to watch ("Watch: bids hold on the next dip -> long idea.")

| Plugin | Version | Change |
|---|---|---|
| lsRevRead / lsRevReadMarks | 0.16.0 | CombRead.h 1.2.0 cbr::toPlain; the Rev six-check sentence stays, the long "Story ..." climax sentence goes |
| lsZzoCombo (Cob Reader) | 3.3.0 | WaveRead.h 1.2.0 wvr::toPlain; the Cob line stays ("Cob: ...") |
| lsVolRead / lsRangeRead | 1.6.0 | VRRead.h 1.2.0 vrd::toPlain (VolRangeLogic.h 1.6.0) |
| PlainRead.h | 1.0.0 | shared helpers |

The read RULES did not change (verdicts are the same); the numbers are still in the status files and scored nightly (lra.pane_verdicts, lra.orderflow_reads).

Tests: tests_revread/t_plainread_100 19/19 (ASan/UBSan -Werror); t_combread_100 20/20; t_waveread_110 12/12; tests_volrange/t_vr150_split 69/69 (updated: no numbers table; the time prefix - it had 14 stale failures since 1.5.3); tests_zzocob/t_zc320_split 9/9 (updated); test_zzocob 140/18 and test_volrange_130 21/1 = the same failures as before the change. MinGW + real irtsdk.h syntax check: no errors outside the SDK for RevRead / RevReadMarks / VolRead / RangeRead / ZzoCombo.
Not verified: his IRT - after the restart, check the four panes in 2+ markets (ChartView).
