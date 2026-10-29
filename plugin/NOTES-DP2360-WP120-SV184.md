# Declutter build 2026-10-28: lsDealerProfile 2.36.0 + lsWavePOC 1.2.0 + lsSessionVWAP 1.8.4

## Requests (Rassul)
- 16:26 CT (ES 3-min screenshot): "the chart is extremely messy, remove the drawing of levels across the chart and from the wave pocs"; also remove the Pivot VWAP line and the Liquidity HELD lines.
- 16:31 CT: "the wave poc lines are too large can you make them about 33% less".
- 16:40 CT: level badges get the LIQUIDITY RANK back (small chip right before the name chips).
- 16:51 CT (approved): the same per-level liquidity match written to lsFlexLevels\DealerProfile\<MKT>-liqlevels.csv for a new MenthorQ Reader section.

## What changed
- DealerProfile 2.36.0: no level lines across the price area (badges stay in the Key Levels column, only the <= 6 px link tick at its edge);
  no faint liquidity order lines; no HELD line / word in the price area - HELD is now a tag ("HELD", bid green / offer red) in the liquidity
  column's tag lane for 645 s after the outcome (touch record file + outcome logic unchanged); off-screen level markers ("^ PWH 7812.25 (+51.5)")
  moved into the Key Levels column top / bottom (small; the badges start below / end above them); LIQUIDITY RANK CHIP on a level badge right
  before its name chips = the best rank among the column's shown zones from the level out to 1 ATR beyond it (one-sided, never inside), side
  colour ring, king (1) white with dark text; width counted before the odds-cut so the odds drop first. Status KEYS_COLUMN: liquidity rank chips N.
  LIQLEVELS FILE lsFlexLevels\DealerProfile\<MKT>-liqlevels.csv (overwrite, atomic .tmp -> replace, dir created): "VERSION,1", then one row
  per badge shown in the Key Levels column: t_ct_ms,market,level_names (merged, column order, '/'),price,side BID/ASK/NONE,lots,
  ratio_vs_median (zone lots / lsLiquidityProfile's median real level of that side, read from its status line LIQ_CANDIDATES at most every
  10 s; 0.00 when unknown),rank (1..6 or 0),king (0/1),zone_lo,zone_hi (zone price +-1 tick),dist_ticks (level -> zone price),atr. Same match
  as the chip (at the level to 1 ATR beyond, one-sided). No liquidity = NONE,0,0.00,0,0 and empty zone / distance. Written once a minute, or
  on a change at most every 5 s; a failed write also waits (never retried every repaint); a handful of rows, no blocking work. Status LIQLEVELS.
  Fix: a second tag-only row indexed topq with (size_t)-1 (heap overflow under ASan) - skipped now.
- WavePOC 1.2.0: no node lines (solid / dashed), no liquidity dotted lines, no touch circles / end ticks; right-edge tags only for nodes still
  OPEN (untouched incl. the forming bar, wave not replaced, origin not broken); profile max bar width x 0.67 (PROF_SCALE); liquidity nodes
  max 40 px (was 60). Stop rules / record file unchanged; zigzag unchanged (off).
- SessionVWAP 1.8.4: defaults = NO line (pivot VWAP off too, SVS_PIV_DEF); parameter version 10 -> 11 so every saved set (incl. 1.8.3's
  pivot-only) is reset once to all-off; window shows all unticked; any line can be ticked back.

## Tests (Linux, mock host)
- test_dealerprofile_2360.cpp (new, from 2350) 105/0 under ASan/UBSan: no line in the price area (no-B path and zone mode), off-screen markers in
  the column, HELD tag in the column / none in the price area, HELD after restart, rank chip (rank 1 king white / rank 2 bid, side ring, inside
  the right-justified block, no overlap, odds clear), no chip for a zone inside the level or > 1 ATR beyond; liqlevels.csv (105/0 total):
  VERSION + 13-field rows = the badges, ASK 900 3.00x rank 1 king zone +-1 tick distance, BID 300 3.00x rank 2, NONE rows, '/' names, no .tmp
  left, status line, > 1 ATR = NONE, throttle (no rewrite on repaint, a change waits 5 s, once a minute when unchanged). test_dealerprofile_290 52/11 = before.
- tests_wavepoc: logic 111/0 (no lines / ovals, tags only for OPEN nodes, profile width = 0.67 x the 1.1 width, LPOC bar 40 px), host 49/0.
- test_sessionvwap_184.cpp (new) 184/0: fresh = nothing drawn, v7-v10 sets reset once, v11 kept, ticking pivot = one connected line.
  160 31/0, 181 68/0, 182 80/0 (version literals bumped), settings 186/0, pivotvwap ALL PASS; settings_contract PASS (45759 checks).
- MinGW -fsyntax-only against the real irtsdk.h: 0 errors / 0 new warnings for all three.

## Not verified - needs his IRT
The look after he exits IRT and restarts (ES + NQ): no lines, rank chips, WavePOC widths, SessionVWAP blank until ticked.

## 2.36.2 (2026-10-29 04:53 CT Rassul "there was a held line on the chart, add that back")
- DealerProfile 2.36.2: the 2.32.0-2.35.0 HELD LINE + word restored in the price area, code identical to 2.35.0: a liquidity level that
  was SHOWN in the column when price touched it (latched) and whose outcome is HELD (lsLiquidityProfile: 0.5 ATR away before 0.25 ATR
  through, SignalBoard S code 11) gets a 2 px line in the side colour (bid green / offer red) at its price from the TOUCH candle to the
  HELD candle, and the word HELD right of the HELD candle (steps right a candle at a time, never over a candle / badge / the HOD/LOD table;
  no room = line only). Final, never repainted, read back from DealerProfile.touch-<MKT>-<spb>.csv after a restart; no time limit (kept
  as long as the touch record keeps it). Clipped to the price area: Key Levels column edge + 3 .. the right-side columns. BROKE / EXPIRED
  get nothing (no THRU line). Still OFF: level lines, faint order lines. The HELD tag in the liquidity column lane (2.36.0) kept.
  Status TOUCH_TAGS counts HELD lines / words again. No settings change (PLUGIN_VERSION literal only).
- Tests: test_dealerprofile_2362.cpp (from 2361) 120/0 under ASan/UBSan (detect_leaks=0; the exit leak is SelfAware Tracker, pre-existing):
  HELD line green 2 px + word right of it on the line, inside the price area, only bright line (no level / order / BROKE lines), column tag
  kept, same line + word after a restart, HOD/LOD table: no line part under it and the word steps right (live + restart), LEFT clip: a HELD
  whose touch candle sits under the Key Levels column starts at the column edge + 3, nothing past the right-side columns; zone mode: the red
  offer HELD line + word. settings_contract PASS (45759). MinGW -fsyntax-only vs real irtsdk.h (RT_LONG = unsigned long long): 0 errors.
