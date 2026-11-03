# ZZO Combo 2.1.1 + Dealer Profile 2.52.0 - 2026-11-03 (built ~15:00-16:00 CT, Rassul "build it all")

## Requests (approved)
- ZZO Combo: draw the wave COUNT ("24", "50+") INSIDE the wave's bars - centred on the wave's second-to-last bar, just below that bar's
  top, white with a 1 px black outline (reference picture gc_zz.png, the white 24 / 12 inside the lower pane's bars); if that bar is too
  short for the text, just above its top. The ratio label ("2.0x") stays centred above the wave's last bar.
- Dealer Profile: "the rank in front of the liquidity" - the LIQUIDITY rows' rank box moves from the END of the row to the FRONT (left of
  R S notP F), same box / colours, rest of the row unchanged, box width unchanged (approved mockup rank_front.png).

## What changed
- ZzoComboView.h (lsZzoCombo 2.1.1): count x = centred on bar max(s, e-1) (a one-bar wave: its only bar), y0 = that bar's top + 2 px
  (COUNT_PAD); too short (text bottom below the baseline) -> text bottom = bar top - 1; if that would touch the ratio label it goes above
  the label; no room under the header row -> dropped (counted in ComboScene::countsDropped). Outline = the same text drawn 8 times in
  black at +-1 px before the white text (no host change; 9 small text calls per count). The old "count needs bot - yPeak >= TH + 4" gate is
  gone (the above-the-bar fallback replaces it). Waves, counts, tags, ratio label: unchanged.
- ZzoDeltaLogic.h: COMBO_VERSION "2.1.1" only (engine unchanged; ZZO Delta 1.3.2 / Volume 1.2.2 / ZigZag 1.1.0 rebuild, same versions).
- ZzoCombo.cpp: header note, description "v2.1.1", setVersion("2.1.1").
- DealerProfile.cpp (lsDealerProfile 2.52.0): DP_LIQ_RANK_FRONT = true (DP_LIQ_RANK_RIGHT = !FRONT). Book-mode rows: rank box at the column's
  left edge + 4 (one aligned column), bar area / R S notP F letters start after it (BX = RKX + bh + 1), label still 4 px after the bar,
  right edge = column right - 4 (where the rank box used to be). Same rank box drawing call (border = side colour, K white box + dark
  letter, others dark box + white digit). Column width unchanged (Key Levels 170 + Liquidity 170). The level-badge rank chips unchanged.
  DP_VERSION / setVersion / DealerProfileSettings.h PLUGIN_VERSION = "2.52.0" (no settings change).

## Tests (g++ -std=c++17 -fsanitize=address,undefined)
- tests_zzodelta: test_zzodelta 116/0, test_zzodelta_ref 34/0, test_zzocombo 55/0, test_zzocombo_210 45/0 (was 46: the old
  "count right edge one bar left" checks replaced by: count centred on the second-to-last bar, just below its top or above a short bar,
  8-offset black outline under every count, never on a ratio label / tag / another count, hand-made short / tall / one-bar waves, real-data
  tally; ratio label unchanged). -Werror -Wall -Wextra.
- Real data (400-bar windows, 9 px bars, 260 px and 150 px panes): ES Oct 26 - Nov 2: 86 counts inside the bar, 1 above a short bar (1%),
  0 dropped; GC Oct 1 - 7: 64 inside, 1 above (2%), 0 dropped; 0 overlaps.
- test_dealerprofile_2520.cpp (new, real DealerProfile.cpp on the 2400 mocked host): 23/0 - rank box first on all 8 rows, aligned at the
  Liquidity column's left + 4, left of letters / bar / label, nothing overlapping, all inside the column, style / colours, label 4 px after
  the bar, R S notP F + 4/4 + 1:49 rows, versions agree.
- test_dealerprofile_2400.cpp updated: its "DP_LIQ_RANK_RIGHT" and "rank box right after its label" checks now follow the front rank
  (DP_LIQ_RANK_FRONT, rank box left of its label). Its 4 other failures are pre-existing (2.50.1 version literal; HELD line / word checks -
  HELD lines were removed in 2.51.0): 31/4 before and after.
- All 30 old test_dealerprofile_*.cpp (g++ -O0, real DealerProfile.cpp, mocked host): 885 passed / 603 failed BEFORE (2.51.0) and the same
  885 / 603 AFTER (2.52.0) - no new failure, none fixed; 6 files fully green (2120, 2170_host, 274, klc_twochart, labels, regression);
  2190 crashes (null Op) before and after; the rest fail on old version literals / layouts superseded by later versions.
- MinGW -fsyntax-only with the real irtsdk.h: ZzoCombo / ZzoZigZag / ZzoDelta / ZzoVolume 0 warnings outside the SDK; DealerProfile the
  same 6 pre-existing warnings before and after (none on changed lines).

## Not verified - needs his IRT
Close IRT -> gex-build installs lsZzoCombo 2.1.1 + lsDealerProfile 2.52.0 -> reopen; check the Combo header "ZZO Combo 2.1.1" and the
counts inside the bars (white, black edge), and "DP v2.52.0 LIQUIDITY" with the rank boxes at the front of each row.
