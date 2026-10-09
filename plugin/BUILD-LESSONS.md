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
