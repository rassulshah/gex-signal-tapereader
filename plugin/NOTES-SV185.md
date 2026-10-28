# lsSessionVWAP 1.8.5 (2026-10-28) - Key Levels column keep-out

## Request
SessionVWAP lines ran across the whole pane, through lsDealerProfile's Key Levels column at the chart's LEFT edge.

## What changed
- Every SessionVWAP drawing (VWAP, +-1 / +-2 SD bands, overnight set, prior RTH, 3-bar pivot VWAP, the (hidden) odds labels) stays right of
  the Key Levels column: x >= cb::leftKeep(symbol, spb, chart, pane.left, steady ms) - the same ColumnBoard rule BigBidAsk / DeltaProfile /
  LiquidityProfile / HodLod use. A segment crossing the edge is cut AT the edge (the line still starts right next to the column); a segment
  wholly left of it is not drawn. Then the 1.6.3 HOD/LOD table cut (KeepOutDraw.h) applies as before.
- No column on the chart (or its stamp > 10 s old) = exactly as 1.8.4. Values, odds, Market State, records unchanged.
- Status file: LEFT_KEEP,<x> (ColumnBoard version). Settings unchanged: parameter version stays 11 (no saved set reset).

## Tests (Linux, mock host; the mock PNT now maps bar -> x / price -> y when H.pxBar > 0, lines record pixel ends)
- test_sessionvwap_185.cpp (new) 20/0 ASan/UBSan: clipLeft unit cases; no column = to the pane edge; 230 px column = nothing left of x 233,
  each crossing segment cut once and starting at 233, every other segment identical, pivot cut the same way; HOD/LOD table at 400-460 still
  cut with the column on; stale / cleared column and another chart's column = unchanged. Mutation (no cut) fails 4 checks.
- 184 184/0; 160 31/0, 181 68/0, 182 80/0, settings 186/0 (SVS_TEST_ALL_ON_DEFAULTS; version literals -> 1.8.5); pivotvwap ALL PASS; logic 47/0.
  (test_sessionvwap_183 already failed on 1.8.4 - superseded by 184's no-line defaults; unchanged.)
- settings_contract PASS (45759 checks, 24 tables).
- MinGW -fsyntax-only vs the real irtsdk.h (RT_LONG prefix): 0 errors, 25 warnings all inside irtsdk.h (same as 1.8.4).

## Not verified - needs his IRT
The look after IRT restarts: SessionVWAP lines (when ticked) begin at the Key Levels column's right edge.
