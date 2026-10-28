# lsWavePOC 1.0.0 "Wave POC" (2026-10-28)

Request: Rassul 2026-10-28 - Wave POC idea, mockups v1-v3, approved 10:59 CT ("just build now"); 11:21 CT "no outline boxes on the
volume nodes" (highlight = the bar fill only). Committed before 15:10 CT so the auto build installs it at the evening IRT restart.

## What it draws (price pane, add "lsWavePOC" to the price chart)
- Waves = legs of the N-bar reversal zigzag (Reversal bars, default 3), up and down. A wave counts when it ends at a 20-bar high/low and
  is bigger than the average of the last 20 earlier such waves in that direction (no look-ahead; >= 5 needed; per market + bar size,
  lsFlexLevels\WavePOC\<MKT>-<spb>.csv).
- Its own volume profile (IRT VAP) at the wave start, pointing right, muted slate, behind the candles (pre-draw; if IRT never pre-draws
  it is drawn in the post call and the status DRAWING line says so).
- Row size rule: rowTicks = max(1, ceil(range ticks / 80)) -> at most 80 rows (ES 37.5 pts -> 2-tick rows, 76 rows).
- Nodes: >= 2.0x the median non-zero row, local peak of the 3-row smoothed profile, valley <= 0.7x the smaller peak, >= 3 rows apart,
  max 5, biggest = WPOC. WPOC = bright amber row + 3 px amber line; others bright light-grey row + 1 px line. No outline boxes.
- Lines dashed from the extreme until confirmed (N bars later), solid after; the live provisional wave is all dashed and moves with each
  new extreme. Stop: touch (circle), next qualifying wave (end tick), close beyond the wave's start pivot (end tick).
- Right-edge tags "WPOC 7700.25 6.3x" / "7693.75 2.5x" only right of the last bar (hidden + counted when there is no room), stacked.
- Zigzag optional (off). Header "Wave POC v1.0.0" top-left below the IRT title row.

## Files
- WavePOCLogic.h (rules, store, record, layout - SDK-free), WavePOC.cpp (host), WavePOCSettings.h (2 rows).
- Settings read from IRT's own per-indicator parameters (getParameters "IB", SessionVWAP 1.8.1 pattern); refused / invalid = defaults.
- lsFlexLevels\WavePOC\<MKT>-<spb>-nodes.csv record (WAVE / NODE / STOP lines); recorded waves are redrawn as recorded after a restart.
- lsFlexLevels\WavePOC.status-<MKT>-<spb>.txt status.

## Tests (Linux, -Wall -Wextra -Werror, UBSan)
- plugin\tests_wavepoc\test_wavepoc_logic.cpp on wavepoc_es1022.csv (ES Oct 22, the mockup data): 81/81 - same waves / nodes / ratios / stops as wp2/wp3.py,
  no repaint bar by bar, restart after every bar identical, record round trip, N=5, provisional move + confirm, origin close, VAP pending
  + retry, store strict parse, layout (tags right of the last bar, no overlap, WPOC distinct, no boxes, max 5, zigzag off by default).
  `cd plugin/tests_wavepoc && g++ -std=c++17 -I.. test_wavepoc_logic.cpp -o t && ./t wavepoc_es1022.csv`
- test_wavepoc_host.cpp on the mock SDK (WavePOC_test_support): 40/40 - settings rows, native / refused / blank parameter sets, window
  calls, draw layers, files, two charts, N=5 per chart, symbol change, bar-by-bar = one pass, restart from record, VAP missing.
  `g++ -std=c++17 -IWavePOC_test_support -I.. test_wavepoc_host.cpp -o th && ./th wavepoc_es1022.csv`; render check: render_dump.cpp + render_check.py
- Syntax: real irtsdk.h (MinGW + windows.h, and g++ wrapper) - 0 errors / warnings outside the SDK. Settings contract with the new
  entry: PASS.

## Not verified - needs his IRT
- PRE_DRAWING (first plugin to use it) - check the status DRAWING line; native getParameters on his IRT; VAP history depth (waves whose
  VAP is missing stay pending and are retried every 10 min).
- Screenshots in 2+ markets after the restart.
