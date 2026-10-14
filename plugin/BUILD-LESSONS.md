# IRT indicator build lessons checklist

Every IRT plugin build must pass this checklist before it is staged. It comes from the external audit of 2026-10-08 (14 indicators plus Delta Profile, in `irt-indicators-optimized/reports/`) and from mistakes made on 2026-10-09. Each item names the test that proves it. Add a line whenever a new mistake is found.

## From the external audit (the other LLM, 2026-10-08)

| # | Mistake found before | Rule | Proof required |
|---|---|---|---|
| 1 | Mutable state in the extension object or in function statics, shared across charts by one DLL object | All state lives in the host's `getUserData()` slot. `destroy()` stops timers, persists, deletes, and calls `setUserData(NULL)`. | A mock with two charts on the same DLL (same market and different markets): no cross-talk; symbol/period change; chart removal. |
| 2 | `long` quantities (32-bit on Windows x64) overflow | Volumes, sums and times are `long long`; promote before arithmetic. | A test with volumes above 2^31. |
| 3 | Windows macro names (`far`, `near`, `min`, `max`, `BOOL`) break MSVC | No such identifiers. Never use `BOOL` (the SDK maps it to RTBOOL); Win32 callbacks are `int CALLBACK`. | MinGW `-Wall` with the MSVC windows.h macros simulated, on every wrapper. |
| 4 | `atoi` / `atof` silently turn bad input into 0 or NaN | Strict full-token `strtoll` / `strtod` with range, finite and sign checks on every external file. | Malformed, partial, non-finite and oversized input tests. |
| 5 | Unsafe file publish loses the only good copy | Write `.tmp`, move the old file to `.bak`, rename into place, restore `.bak` on failure. A reader never trusts a partial file. | A test where the rename fails. |
| 6 | Events marked written before the append succeeded | Mark only after a flushed, good write; reload on failure. | A test with a failing write. |
| 7 | Non-finite price converted to ticks | `toTicks` rejects non-finite and out-of-range values. | A NaN/Inf test. |
| 8 | Broad shared-header dependency (`DealerLogic.h`) | Include only what is used; list the real headers in `gex-build.bat` so edits trigger a rebuild. | Review of the build line. |
| 9 | Status files keyed only by market: last writer wins | Key status/record files by market **and** seconds per bar, or chart identity, where two charts can coexist. | Two-chart mock. |
| 10 | DST / midnight / roll / forming vs closed bar | Decide on closed bars only. Session is 17:00 CT; test the DST hour. | Time tests. |

## From 2026-10-09

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 11 | Repaint: A turned back into A?, and circles vanished when the top 3 changed | A signal, once drawn, never changes or disappears. Decide at bar close; record to a session file; redraw from the file. | Bar-by-bar replay with a restart after every bar equals one straight pass. |
| 12 | Off-by-one-bar time alignment | IRT bar time = bar **end**. A 3-min bar "13:54" covers [13:51:00, 13:54:00). | Per-minute volume sums equal the chart bar volume (median error 0). |
| 13 | Arrow over the wrong sub-bar (candle centre, not the absorption minute) | Marks sit exactly over the sub-bar (minute) they describe and inside their own candle's width at every zoom. | Pixel test at 3 zoom levels plus a camera picture after install. |
| 14 | Labels overlapping bars, headers and each other | At most one mark per bar; bigger wins; reserved top/bottom bands; header outside the bars. | Overlap test in the sim harness plus the camera picture. |
| 15 | Signals in the wrong place (bearish mid-pullback, bullish at the top) | Location rules: bearish at a high or resistance, bullish at a low or support; the zone must survive its bar. | Wrong-side rate in the all-market study. |
| 16 | Deploy sent stale content (same staged path reused) | A new unique staged file name per commit; re-stage from the PC and check the version string after every commit. | Version check in the deploy step. |
| 17 | Layout switch thought to install | Only File → Exit installs; check BUILD-STATUS "installed". | BUILD-STATUS check. |
| 18 | Heavy work during RTH froze IRT | Back-fills and exports only outside RTH, in ≤200 ms slices, with a crash guard. | A sim asserting nothing heavy runs inside RTH; the freeze probe. |
| 19 | GDI+ will not compile next to the SDK | No GDI+; GetDIBits plus our own PNG encoder. | Build. |
| 20 | Untested assumptions about IRT (parmsTitle refresh, periodicity label empty) | Every IRT behaviour we rely on has a fallback and is verified with the camera after install. | Camera picture plus status line. |
| 21 | Shipped before measuring if the signal works | New signals ship in TESTING mode until the all-market study passes (precision, recall, lateness, wrong side, per market, ≥20 per side). | Study report. |

## From the other indicators' audits (DealerSummary, Delta Profile, SessionInfo, SessionVWAP, TradeManager, IRTReader, DayModel, DayStats, Gamma, King, DealerRead/Sig/Profile)

| # | Mistake found before (where) | Rule | Proof required |
|---|---|---|---|
| 22 | SDK read failure returned a partial prefix as success; fixed 4,000-row buffer clipped data (Delta) | Read exactly the count the SDK reports; any failure rejects the whole snapshot; check row totals against bar volume. | Test with a failure mid-read and with more than 4,000 rows. |
| 23 | A fixed bar cap silently shortened the window (Delta 3,000 bars) | Traverse back to the time cutoff, not a bar count; report short history. | Short-chart test. |
| 24 | Hard-coded tick sizes overrode the chart's (Delta) | `SYM_TICKINCR` is the only tick source; validate it is finite and positive. | Test with the QM/MHG/M6E variants. |
| 25 | Float prices and pixel matching to identify rows (Delta) | Integer tick keys, floor division; marks are matched by key and bar index, never by screen Y or X. | Boundary tests at tick/group edges. |
| 26 | Float totals lose one-contract changes above 16.7M (Delta) | Accumulate in double or long long; convert to float only when drawing. | Large-volume test. |
| 27 | A forming candle used as "closed"; bar counts guessed from two timestamps (Delta) | Closed = a later bar exists; time spans measured in seconds, not guessed bar counts. | Forming-bar test. |
| 28 | Look-ahead in signal resolution (Delta) | Sequential state only: a decision uses data up to that bar, never later bars. | Replay equals live (#11). |
| 29 | Opening the settings dialog overwrote saved settings (Delta) | Loading never saves; clamp every loaded value. | Dialog test. |
| 30 | Fault disabled the whole DLL permanently (Delta) | Fault quarantine per host, reset on symbol change or reload; never re-enter a faulting native call in a loop. | Fault test. |
| 31 | Files opened or rewritten on every repaint (Delta, DealerSummary, SessionVWAP) | Status writes only on change or every ≥30 s, atomic; external files parsed only when their stamp changes; missing files cached. | I/O count test. |
| 32 | Expensive recalculation on every repaint | Cache per host; recompute at most once per second unless the chart's count, stamp or tick changes; drawing reuses the snapshot. | Calc-count test. |
| 33 | Unbounded external input (DealerSummary, SessionInfo) | Cap input size (e.g. 64 KiB) and line counts; binary-search text fitting, not per-character. | Oversized-file test. |
| 34 | Unknown or negative age shown as live (DealerSummary) | Missing or invalid timestamp = OLD and labelled so. | Test. |
| 35 | Drawing beyond the pane when it is too short (DealerSummary) | Check geometry; say "pane too short" instead of forcing. | Small-pane test. |
| 36 | Stale display when the feed is idle (DealerSummary) | Use CALL_CONTINUOUSLY or a timer where staleness must show. | Idle test. |
| 37 | RTH admission by whole minutes; VWAP before the first trade; precision loss (SessionVWAP) | Session membership by seconds; no value before data; Welford-style running variance. | Session-edge tests. |
| 38 | Zero variance gave 100% odds; uninitialised features (SessionVWAP) | Guard zero and non-finite cases; initialise everything; bounds-check indexes. | Edge tests. |
| 39 | Malformed times accepted (`24:00`, `09:60`) (SessionInfo) | Strict HH:MM validation; windows that cross midnight are supported. | Parse tests. |
| 40 | Unbounded work per pass (IRTReader) | Cap the work per pass (bars, levels, events); continue in the next pass. | Sim. |
| 41 | Dead code, retired paths and stale badges kept around (Delta, SessionInfo) | Remove dead paths; the product text matches what the code does. | Review. |

## From the settings audit (2026-10-14)

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 42 | Settings window rows that the chart never reads: SessionVWAP 1.3.4-1.4.2 had no rows at all (OUTPUT_NO_UI), LiquidityProfile Width / Font and DealerRead / TradeManager Font only reached the copy the dialog ran in, LiquidityFlow showed a Width it never draws, DealerRead kept Clock offset / Background rows that did nothing, DealerSig used an Off / On list for a check box. IRT fills list / box / colour values only while the settings window is open. | Every row is read and used. Saved ONLY from parmsApply / parmsUpdt (opening never saves) to a per-market file in lsFlexLevels; every draw re-reads that file by its stamp (at most once a second); the dialog is filled from it in parmsLoad. Rows are only ever APPENDED (IRT keeps values by position); a removed or retyped row bumps setParameterVersion. Each drawn line has a show box + colour; unticking removes it (and anything attached to it). | A mock SDK that hides values outside the dialog (SessionVWAP_test_support/irtsdk.h); per setting: off -> nothing of it drawn, colour -> drawn in it (test_sessionvwap_settings, render_dump states A-D); a static contract that rows exist and are read (tests_settings/test_settings_contract.py). |
| 43 | Two recorders on one order feed write every event twice (lsPulling 0.2.x and PullStats 1.2.0 both write data\irt\<session>\<MKT>\mbo_HH.csv). | One writer per market per IRT (process registry); a recorder that sees its file grow by more than it wrote stops and drops (never duplicates) until the file has stood still 10 s. Writes are buffered, once a second from the calc pass, never per event and never from a draw; under 2 GB free = paused with a status note. | tests_of/test_mborecord.cpp (ownership, foreign writer, low disk, off, restart append) + lra.mbo_live's own parser on the written files (tests_of/check_mbolive.py). |
| 44 | lsHodLod 2.0.5 read its new checkboxes with isBoxChecked in draw(): IRT hands the chart copy every box UNCHECKED, so "Show table" read off and the table vanished on ES / CL (no TABLE_BOX in the status) - lesson 42 was known but not applied to a plugin built the same night. | Every setting of every plugin (lists, boxes, colours, offsets) is saved from the settings window only (inDialog = no bar size) and the chart reads ONLY the per-market file; the dialog is filled from the file in parmsLoad and a control the user did not touch keeps the file's value (BoxPlace.h DlgSnap). | hlw/tests/t_hl206.cpp [1]: the mock's chart copy has every box 0 / list 0 and the table must draw; [3] round trip incl. a dialog that ignores the fill. |
| 45 | "L = one large order" compared the candle's BIGGEST order with the per-ORDER 99th percentile: a candle with thousands of orders almost always has one above it - L on ~every candle (ES 46 of 46), white boxes everywhere. | A per-candle mark must be judged against the per-CANDLE distribution of the same statistic (the prior candles' biggest orders, top 10%, 20+ candles; no mark before). | tests_of/test_panel_logic.cpp "rare L" (no history = no L; on candles like the prior ones <= 10%). |
| 46 | A pane under 80 px got a text reserve of 0, so spike labels were placed over the read line (PullStats 75 px pane). Labels avoided the info box but not the read. | The panel (box + read lines) is laid out BEFORE the bars and all its rectangles go into the labels' avoid list; one label per candle. | tests_of/test_panel_logic.cpp (reserve per mode / pane height), test_panel_host.cpp. |

## From the Tape Reader banner / Liquidity hand-off (CR5000, 2026-10-14)

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 47 | lsTapeReader 1.1.3 (pre-release) passed the WHOLE banner box through the overlap hysteresis (bp::Hyst): when the Liquidity warning came (1 -> 2 lines) or the text got wider, the old-size box was drawn for one more draw. | Hysteresis holds only the spot's anchor point (left / centre / right x, top / middle / bottom y); the box is rebuilt from it with the current width / height on the same draw. | test_tr_banner jitter: 1,000 draws, height always matches the warning on the same draw (the old code fails it - mutation checked). |
| 48 | A cross-plugin "is it still there" check by entry age alone (60 s) drops the other plugin's part on the first repaint of a chart that sat idle, and keeps a removed plugin's part for a minute. | Heartbeat (BannerBoard 1.0.1 bb::Beat): present while the stamp moves between my draws, or under 5 s old, or for 3 of my draws; each plugin clears its own entry in done() / destroy() (the Flow pane, which never publishes, never clears). | test_tr_banner board + Liquidity profilehost: 40 / 30 repaints with 10-minute idle gaps in either draw order never drop; a stopped stamp drops after 3 draws; removal clears at once. |

## From the spike-label report (CR5300, 2026-10-14)

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 49 | OrderFlowFeed.h 1.2.1-1.3.0 spikeLabels: when ANY spike had no free label spot (under the read box / hover grid, the forming bar, a crowded spot) it shrank the NEAREST already-placed full labels to their ratio, one after another - Rassul's CLZ6 seller spike lost "p 1.3x" / "111" and they came back on a scroll or the next bar (all three order-flow panes, either side). | A drawn label / signal is decided from OLDER items and fixed obstacles only (time order); a newer item adapts or shows less - it never takes rows away from an earlier one. Transient overlays (hover grids) are drawn over, never fed into the layout. Layouts include a lead-in left of the view so a scroll does not re-decide what stays in view. | tests_of/test_label_persist.cpp: logic + 22k-layout fuzz (no earlier label changes when a spike is appended; seller / buyer mirror identical) + BidAskStats / PullStats / StackStats hosts (3 rows kept through N bars, every hover, mouse in box, scroll, restart, 17:00 CT); fails on 1.3.0 (logic 9, each host 4), passes on 1.3.1. |

## From the CoB impulse tag (CR5500, 2026-10-14)

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 50 | The approved CoB-impulse mockup said "the 08:36 bar = biggest in 100" - read off his WAVE pane (CI Biggest_Wave_Price_Since_Bars1), while the approved rule is a single BAR's range: on NQ 10/14 the 08:36 bar (82.25 pts) was only the biggest in 21 bars (the 07:33 news bar was 232.5), so the real tag is Cob+V? (08:33 volume, biggest in 100), not Cob+R?. | Before a mockup quotes a number, compute it with the build's logic on the chart's own bars (ChartView export) and say which unit it is (bar vs wave). | test_cobimpulse_logic.cpp NQ 2026-10-14 fixture: 08:39 = V only, 08:36 range since = 21; 08:54 = plain. |
| 51 | A plugin that depends on his RTL by name can silently draw nothing when IRT cannot evaluate that name from a plugin. | Prove the name is readable first: add it to lsFlexLevels\ChartView.labels.txt and check ChartView's export (cob_bull_3m_1b read OK, flags at 08:39 / 08:54 matching the chart) before building; the plugin never advances its decided-through bar while the RTL is unreadable and says so in its status file. | test_cobimpulse_host.cpp "RTL missing: nothing decided" then decided on the next readable bar. |

## From the right-side columns (CR5200, 2026-10-14)

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 52 | Every right-side plugin guessed the others' room from shared status files (DealerProfile REACH = width + 100, DeltaProfile WIDTH / gap, LiquidityProfile.reach-<MKT>.txt): one file per DLL for every chart, so two charts with different widths overwrote each other, and text spilled from one column into the next (badges over the Delta labels, % pills in the Delta column). | ONE layout for all of them (ColumnBoard.h): each plugin publishes its own width for THIS chart on the process board (symbol + bar size + chart label), reads the others' with the bb::Beat heartbeat, and computes the same boxes from the axis; every draw clamps to its own box (labels shorten, else are not drawn). | test_columnboard_logic (min / max / mixed widths x 4 chart widths: no overlap, abutting, margin = gap + columns), test_dealerprofile_2110 + tests_delta/test_columns280 (no draw op of a plugin outside its column), test_lq_columns_host (the hover stops at the columns). |
| 53 | The right margin is a chart option the SDK lets a plugin set only in pre-calc / Apply calls, and its unit (px per the header comment, bars per the chart dialog) was never verified. | Exactly one owner (Dealer Profile; Delta only with no Dealer Profile for 5 s); set from calc / preCalc / parmsApply, never draw; asked in BARS (rounded up) until two observations of the measured gap (last bar on screen) say pixels; grows at once, shrinks after 60 s; never re-asked within 2 s; nothing when getChartOption fails. Status: MARGIN line (need, units, read, asked, set ok / fail, gap). | test_columnboard_logic (rounding never short, probe votes, no shake with a flipping need), test_dealerprofile_2110 [1, 3, 9] (52 bars at 5 px / bar, set once, px detected, option failure). NOT verified on his IRT yet: check the MARGIN line after his restart. |
| 54 | A badge design approved at a mockup font did not fit the real font: "LonLO | HL 1.5K" needs ~92 px at 9 pt bold (Liberation / Helvetica metrics), the column default is 78. | A text that must fit a fixed box has a fit cascade decided in the plugin (here: badge font, 1 pt smaller, the HL block without the word "HL" - colour + outline still say HL, then the name alone) and the case is in the tests; never cut a word, never overflow the box. | test_dealerprofile_2110 [6] (30 px column: name only; HL blocks inside the 78 px column), the render check (render-2110-cols.png). |
| 55 | test_dealer_logic failed in a fresh work tree: it reads LRA-Dealer-GC.sample.csv from the current directory and the tree was staged without it. | Stage a test's fixtures with it (the run script lists them); a missing fixture is a test-setup failure, not a code failure. | run_tests_cr5200.sh runs from plugin\ with the fixture present: 98 passed. |

## From the peer-review claim gates (2026-10-14, Rassul 15:07 CT approval)

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 56 | Claims the peer review found unproven (Strength label, HVL "cushion", options causes as confirmed, Delta Profile A as a vote) were shown in several places at once - bridge rows, Reader, video, and inside two plugins - so hiding one producer would have left old files / other panes still showing them. | A claim that must be hidden is gated in ONE switch per claim, in BOTH layers: Python lra.claim_gates (config/claim_gates.json, default hidden) for every producer, and plugin/ClaimGates.h (compile-time, default 0) for every plugin that draws it - the plugin also filters older files. Never delete the code; every gate has a hidden-default test AND a restored test. | test_claim_gates.py (20, hidden + restored), trtests_of: test_tr_gates (default) + test_tr_logic built with -DLRA_SHOW_ABSORPTION_REASON=1 -DLRA_SHOW_HVL_CUSHION=1 (240/0), test_dealerread_460 hidden 9/0 + restored 4/0. |

## From the recording fix (CR5450, 2026-10-14 evening - LIQ-DAY-2026-10-14 review)

| # | Mistake | Rule | Proof required |
|---|---|---|---|
| 57 | lsIRTReader 0.6-0.7.1 read MARKET_DEPTH(NULL) / DEPTH_BY_ORDER(NULL) inside the TIMER pass: "NULL = this chart" has no chart there. msglog: C0000005 in lsIRTReader calling timer() at each reload; the fault unwound past `busy = true`, so the reader silently never ran again and its status file showed only the Footprint Reader's "DOM,off". | A NULL-ticker depth / order-feed object is opened and read only from calc / draw. Anything read from a timer is opened with the chart symbol spelled out, and every native call there sits behind a structured-exception guard (no C++ objects in that frame); a busy flag carries a start time and is cleared after 30 s. | tests_ir/test_ir_host.cpp (clock read faults -> that clock read off, chart keeps reading; a never-finished pass recovers); contract test_irtreader_state_contract.py. |
| 58 | Two DLLs (lsIRTReader + lsFootprintReaderES) on one chart wrote the SAME status file: the last writer won and hid the other's state for a whole day. | One status file per writer, or one writer per market with an owner/heartbeat file; a non-owner never writes the shared status. | test_ir_host step 7 (second reader opens nothing, writes nothing). |
| 59 | The MBO feed was drained only from calc / draw (IRT ran them every ~6-10 s on the busy 3-min chart, never on a hidden layout tab), <= 5,000 events / 8 ms per call, while IRT keeps only the last few thousand unread events per subscriber: ES mbo_HH.csv held 30-45% of the volume in busy periods (events in 1-3 s bursts, 5-9 s holes). | A recorder reads the feed on its own 250 ms clock into a queue (engine routing stays in calc / draw, in order), and reports COVERAGE = fills read / bar volume per closed bar so loss is visible. | tests_of/test_feedclock_host.cpp: a ring-buffer mock reproduces the loss (4% at calc-every-8 s) and proves 0 lost / 100% coverage with the clock, a 40k burst in one tick, a 2-minute hidden chart, hour / session boundaries. |
| 60 | Liquidity Profile 0.15.0 counted every NEW in the zone after the first fill as "rebuilt" and capped at 200%: in ES the queue churn is 4-5x the hit, so rebuilt_pct was 200 on every attempt. | A restore ratio credits adds only at the hit price, up to the lots hit there and not yet restored; ratios that cannot exceed 100% are capped at 100%. | tests_lq/test_liquidityhover_logic.cpp test_rebuilt_churn. |
| 61 | A footprint bar whose volume at price IRT never loads (HG 2026-10-09 16:00) stopped the cursor for 5 days; a market with no data (NG 10-14 after 08:26, IRT subscription aborted) was silent. | A bar that fails N passes and is old / on the 16:00-17:00 boundary is skipped and traced; 20+ minutes without a new bar is an ERROR line. | test_ir_host step 10. |
| 62 | A pytest run of mbo_study.run wrote the LIVE %USERPROFILE%\...\LRA-LiqNorms-ES.csv (liq_norms_export with no out folder). | Any test that runs a pipeline step redirects USERPROFILE (monkeypatch) and asserts the live file's mtime did not change. | analytics/test_mbo.py test_study_end_to_end_defender_holds. |
