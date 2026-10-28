# Declutter build 2026-10-28: lsDealerProfile 2.36.0 + lsWavePOC 1.2.0 + lsSessionVWAP 1.8.4

## Requests (Rassul)
- 16:26 CT (ES 3-min screenshot): "the chart is extremely messy, remove the drawing of levels across the chart and from the wave pocs"; also remove the Pivot VWAP line and the Liquidity HELD lines.
- 16:31 CT: "the wave poc lines are too large can you make them about 33% less".
- 16:40 CT: level badges get the LIQUIDITY RANK back (small chip right before the name chips).

## What changed
- DealerProfile 2.36.0: no level lines across the price area (badges stay in the Key Levels column, only the <= 6 px link tick at its edge);
  no faint liquidity order lines; no HELD line / word in the price area - HELD is now a tag ("HELD", bid green / offer red) in the liquidity
  column's tag lane for 645 s after the outcome (touch record file + outcome logic unchanged); off-screen level markers ("^ PWH 7812.25 (+51.5)")
  moved into the Key Levels column top / bottom (small; the badges start below / end above them); LIQUIDITY RANK CHIP on a level badge right
  before its name chips = the best rank among the column's shown zones from the level out to 1 ATR beyond it (one-sided, never inside), side
  colour ring, king (1) white with dark text; width counted before the odds-cut so the odds drop first. Status KEYS_COLUMN: liquidity rank chips N.
  Fix: a second tag-only row indexed topq with (size_t)-1 (heap overflow under ASan) - skipped now.
- WavePOC 1.2.0: no node lines (solid / dashed), no liquidity dotted lines, no touch circles / end ticks; right-edge tags only for nodes still
  OPEN (untouched incl. the forming bar, wave not replaced, origin not broken); profile max bar width x 0.67 (PROF_SCALE); liquidity nodes
  max 40 px (was 60). Stop rules / record file unchanged; zigzag unchanged (off).
- SessionVWAP 1.8.4: defaults = NO line (pivot VWAP off too, SVS_PIV_DEF); parameter version 10 -> 11 so every saved set (incl. 1.8.3's
  pivot-only) is reset once to all-off; window shows all unticked; any line can be ticked back.

## Tests (Linux, mock host)
- test_dealerprofile_2360.cpp (new, from 2350) 93/0 under ASan/UBSan: no line in the price area (no-B path and zone mode), off-screen markers in
  the column, HELD tag in the column / none in the price area, HELD after restart, rank chip (rank 1 king white / rank 2 bid, side ring, inside
  the right-justified block, no overlap, odds clear), no chip for a zone inside the level or > 1 ATR beyond. test_dealerprofile_290 52/11 = before.
- tests_wavepoc: logic 111/0 (no lines / ovals, tags only for OPEN nodes, profile width = 0.67 x the 1.1 width, LPOC bar 40 px), host 49/0.
- test_sessionvwap_184.cpp (new) 184/0: fresh = nothing drawn, v7-v10 sets reset once, v11 kept, ticking pivot = one connected line.
  160 31/0, 181 68/0, 182 80/0 (version literals bumped), settings 186/0, pivotvwap ALL PASS; settings_contract PASS (45759 checks).
- MinGW -fsyntax-only against the real irtsdk.h: 0 errors / 0 new warnings for all three.

## Not verified - needs his IRT
The look after he exits IRT and restarts (ES + NQ): no lines, rank chips, WavePOC widths, SessionVWAP blank until ticked.
