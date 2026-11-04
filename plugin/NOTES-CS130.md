# lsCobSignal 1.3.0 - LVP (low-volume pullback after a CoB) (2026-11-04)

## Request
Rassul approved the LVP mockup (irt_lvp.png, GC 3-min Nov 3: "LVPs at 8:24-8:33 and 8:48-8:54") and said "build" (estimate ~1.5 h).

## What changed (CobSignalLogic.h `cobs::Lvp`, CobSignal.cpp records)
- ZigZag: `zzd::Engine::step` with `zzd::COMBO_REV_BARS` (= 3, High / Low) from ZzoDeltaLogic.h - the same walk as lsZzoCombo / lsZzoZigZag
  / lsBreakout's Pb, reused, not re-written. A wave = the bars after the previous pivot through its own pivot (= the ZZO Volume wave);
  its range = previous pivot .. pivot.
- CoB wave = the ZigZag wave in the CoB's direction holding the CoB leg's extreme bar (`Mark::legEnd` = the bar before the CoB bar).
  The CoB bar itself is often the first bar of the pullback (when the leg extreme is the pivot), so the leg extreme is the anchor. On GC
  Nov 3 both give the same wave (7:45-8:24). No such wave = no LVP for that CoB (status `LVPLOOK` says why).
- Pullbacks = the next waves the other way (bear CoB: up waves; bull CoB: down waves), the first 3 only, each while it stays inside the
  CoB wave's high .. low; the first one that leaves the range ends the look.
- LVP = pullback volume * 3 <= CoB wave volume (integer). Bear CoB: red "LVP" above the pullback's pivot high; bull CoB: green "LVP" below
  its pivot low. Helvetica 10 bold (as the Cob labels), row 3 out from the wick: 51 px (row 0 wave-time label +3..+17, row 1 lsCandleAbsorb
  A +19..+33, row 2 lsBreakout Pb +35..), one more 16 px row out if another lsCobSignal label is there, dropped if outside the pane.
- Decided on the closed bar that CONFIRMS the pullback wave (the ZigZag turns off it); never repainted; the forming bar is never read.
- An LVP of a recorded-only (LOG) CoB is recorded with LOG and not drawn.
- Record: `lsFlexLevels\CobSignal\<MKT>-<spb>.csv` gets one `LVP,<pivot bar end>,BULL|BEAR,<CoB bar end>,<pullback first bar end>,
  <confirming bar end>,<pullback vol>,<CoB wave vol>,<pct>,<nth pullback>,SHOW|LOG` line per LVP, once (keyed by the pivot bar end;
  restart / second chart re-reads the file). Status: `LVP_RULES`, `LVPS,<n>`, `LASTLVP,...` (last 8), `LVPLOOK,...` (last 4 CoB looks).
  The `-live.csv` stays CoB-only (unchanged for its readers).
- CoB detection unchanged. No settings (SS_SAFE_DIALOG), no hovers, no text on the chart beyond the letters.
- Versions: CS_VERSION = cobs::VERSION = setVersion = 1.3.0; setParameterVersion unchanged (no parameters).

## GC Nov 3 replay (tests_cobsignal/lvp_gc_1103.csv; bar END times, +3 min vs the mockup's START labels)
| Pullback (mockup labels) | Volume | CoB wave 7:45-8:24 | % | LVP |
|---|---|---|---|---|
| 8:24-8:33 | 3,575 | 16,402 | 21.8% | yes (decided 8:45 start bar) |
| 8:48-8:54 | 1,151 | 16,402 | 7.0% | yes (decided 8:55 start bar) |
| 9:25-9:50 | - | - | - | leaves the CoB range: look ends |
The mockup text said 18% / 6%: it summed the CoB wave including its start pivot bar (7:45, 3,459 lots) but the pullbacks without theirs.
The plugin uses the ZZO wave (after the previous pivot through its own) for both - same two LVPs.

## Tests
- tests_cobsignal/test_cobsignal_lvp.cpp (SDK-free, ASan/UBSan, -Werror): 30 passed, 0 failed - GC replay (both LVPs, volumes, decision
  bars, the 3rd pullback leaving the range), nothing before the wave confirms, bar-by-bar prefix-stable + restart after every 7th bar
  byte-identical, long CoB on mirrored bars (green, below), synthetic: max 3 pullbacks, a pullback above the CoB high ends the look,
  exactly 1/3 counts / 1/3 + 1 does not, layout 51 px out, no overlap with a Cob label, LOG not drawn, outside pane not drawn, record /
  status lines, Engine wiring, reset.
- tests_cobsignal/test_cobsignal_host.cpp (mock SDK, ES 40-session fixture): 32 passed, 0 failed - 22 CoBs, 9 LVPs; every LVP once in
  the record, restart appends nothing, bar-by-bar = one pass for LVPs, LVP drawn red above / green below >= 50 px from the wick, no label
  overlaps another, versions 1.3.0. (Fixed the test's own -Wmisleading-indentation so it builds with -Werror.)
- MinGW `-fsyntax-only -Wall -Wextra` against the real irtsdk.h (tests_cobsignal/wrap_real_sdk_CobSignal.cpp): 0 errors, 0 warnings.
- Render check: tests_cobsignal/render_lvp_dump.cpp (the same cobs::layout) - two red LVPs above the GC pullbacks, clear of the Cob label.
- No new Win32 calls (check_win32_libs unaffected).

## Build / install
gex-build.bat line for CobSignal now also lists ZzoDeltaLogic.h ZzoParams.h (so a ZigZag change rebuilds it). Builds on the next 2-min
run; installs only when IRT is closed (File -> Exit), then BUILD-STATUS shows `installed`.

## Not verified - needs his IRT
- The live chart: LVP placement vs his wave-time labels, Candle Absorb A and Pb on real bars; 51 px may be far on a compressed chart.
- LVP is descriptive (not yet scored): the nightly should score LVP vs no-LVP pullbacks after CoBs (20+ a side) before it is trusted.
