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

# lsWavePOC 1.1.0 - liquidity nodes (2026-10-28, Rassul approved 11:27 CT for tonight's restart)

- WavePOCLiq.h (new, SDK-free): IRT's live order-by-order book (DEPTH_BY_ORDER, one per chart, kept alive, crash-guard file
  lsFlexLevels\WavePOC\_opening-<MKT>-<spb>.txt, drained <= 5,000 events / 8 ms per call). Market-maker orders (re-priced >= 2x in
  5 min) left out. Sampled once a second (+-300 ticks of the book's middle) into the forming bar.
- A wave's liquidity = average resting lots per volume-profile row over its bars (needs samples on >= 90% of its bars; waves formed
  before the indicator loaded have none - no error, status LIQUIDITY_NODES "waves without book").
- Nodes on the RAW rows (>= 2.0x median, a row >= both neighbours), separated like the volume nodes, max 5, biggest = LPOC; side =
  bids if bids >= half the row.
- Drawn: nodes only, bars pointing LEFT from the wave start (max 60 px), LPOC bright green (bids) / red (offers), others 60%; faint
  dotted line to the right (same stop rules); no tags, no outline boxes (none on the volume nodes either - Rassul 11:21 CT).
- Record: LNODE,t0,t1,rank,price,lots,ratio,bid|offer,near_volume_node|alone (within 2 ticks of a volume node) + LSTOP lines; recorded
  liquidity nodes are redrawn after a restart (the book itself is not historical).
- Tests: logic 107/107 (wp8.py reference on the Oct 22 MBO snapshots, wavepoc_es1022_liq.csv: wave 1 LPOC 7688.25 2.31x bids +
  7707.25 2.01x near the volume node; wave 2 LPOC 7685.25 4.53x, 7680.25 4.01x, 7689.25 2.39x - all match), book / MM / sampler /
  coverage; host 48/48 incl. a live mock book (LPOC = the 400-lot bid, the re-pricing 900-lot market maker left out, feed destroyed
  with the chart). Run: `g++ -std=c++17 -I.. test_wavepoc_logic.cpp -o t && ./t wavepoc_es1022.csv wavepoc_es1022_liq.csv`.
- Not verified on his IRT: the order feed on the price chart (status MBO line: open / events / orders / samples).

# lsWavePOC 1.1.1 - liquidity nodes: 1.9x + raw-row separation (2026-10-28, small approved change)
- Liquidity nodes only (volume nodes unchanged): threshold 2.0x -> 1.9x of the wave's median non-zero liquidity row.
- Separation on the RAW rows (no smoothing): two liquidity nodes are separate when >= 3 rows apart AND the minimum raw row between
  them <= 0.7x the smaller node's raw value. 1.1.0 used the 3-row smoothed valley, which merged single-tick orders on a flat ES book.
  Reference: wp9.py pick_raw.
- Oct 22 ES fixture: wave 11:18-11:48 LPOC 7688.25 (2.31x), 7707.25 (2.01x), 7710.25 (1.92x); wave 12:33-13:03 LPOC 7685.25 (4.53x),
  7680.25 (4.01x), 7682.25 (2.85x), 7689.25 (2.39x).
- Tests: test_wavepoc_logic 109/109, test_wavepoc_host 48/48, settings contract PASS (24 tables). Version 1.1.1 everywhere
  (WP_VERSION, setVersion literal, wpc::VERSION, wpl::VERSION, PLUGIN_VERSION).
