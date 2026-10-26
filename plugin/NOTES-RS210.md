# lsReversalStage 2.1.1 (StageBox.h 1.0.0) - 2026-10-25, Rassul approved the Reversal Stage info box 22:51 CT (stage-box-mockup.html)

## What changed (display only - the 4-step engine and the RS3 record are unchanged)
- The pane shows the letters **1 2 3 C** on each pivot bar (bottom row of the pane) instead of the staircase: 1 Trap, 2 Collision,
  3 Turn, C Confirm - only the stages that were seen; green = a low, red = a high; an amber **F?** above them when the Defense pane
  recorded F? on the pivot bar.
- The **STAGE INFO BOX** for the latest pivot (9-spot Position as before). Each row: stage # . name . what to look for . check / dash .
  value (threshold):
  - 1 Trap - the engine's Trap step (P stacking / Q held before contact), its own words ("bids 7,752 > offers 6,316").
  - 2 Collision, absorption - the engine's Collision step (attack + A / Q held), its own words.
  - 2 Collision, exhaustion - an E letter with pace <= 0.5x ("pace 0.4x (0.5x)"), else "not here . look for pace <= 0.5x".
  - 3 Turn - CHECK = the HELD trigger: from the pivot bar to 3 bars after it, the first bar closing GREEN for a low (RED for a high)
    with no new extreme before it. "trapped <x> (thr)" = sells below the close / the 20-bar average bar volume (buys above it for a high),
    from IRT's own volume at price: DESCRIPTION ONLY (never a check). The wick volume ratio is in the hover only (description).
  - C Confirm - H (hit back) ratio >= 1.25x AND the bar after the trigger makes a higher high (low) / lower low (high). Check only when both.
  - F? - amber warning row when F? is on the pivot bar (lsBookFlow writes it once F? is built; until then the row never shows).
- Thresholds in lsFlexLevels\LRA-StageBox-<MKT>.csv ("PARAM,KEY,VALUE": TRAP_THR, WICK_THR, HB_MIN, PACE_MAX, TURN_BARS), re-read
  every 60 s; built-in defaults (TRAPPED.md top third): trapped ES 0.30x, NQ 0.31x, CL 0.33x, GC 0.33x, HG 0.26x, NG 0.28x, 6E 0.22x;
  wick ES 0.37, NQ 0.43, CL 0.50, GC 0.48, HG 0.35, NG 0.33, 6E 0.24.
- Hover a pivot's letters (or the box): his format - Question / Answer / How to use / Example + each row.
- Settings (parameter version 2): "Stage letters (1 2 3 C)" (+ Low / High pivot colours), "Stage legend", "Stage info box" (+ Position,
  "Dies-at-once warning (amber)" colour). Status: STAGE_BOX,<version>,<n> pivots,latest <letters>,params <file/defaults> ...
- No repaint: only closed candles and the append-only letter records are used; a box is decided once its Turn window / follow-through
  bar has closed and never changes after.

## Tests (Linux, mock SDK, ASan/UBSan, -Werror)
- tests_strips/test_stagebox.cpp (logic): 26/26 - market defaults, params parsing, VAP sums, rows 1/2/2/3/C, trapped 0.40x, wick,
  trapped high but no green close = no check, new low first = no turn, Hb 1.1x = no C, F? row, nothing recorded, high mirror,
  no repaint (decided box identical with later bars).
- tests_strips/test_stagebox_host.cpp (plugin on the mock host): 18/18 - letters drawn green on the pivot bar, box title / names /
  values / thresholds, drawn check marks, legend, status line, hover in his format, params file, F? amber row, settings on / off,
  colour setting, parameter version 2.
- tests_strips: strips 1303, core 119, transport 45, test_stage_pipeline 18/18 (its display check now reads the stage box), replay 0 repaint differences.
- 2.1.1: the C row reads the H ratio as lots / attack when the evidence carries no ratio.
- Settings contract 43,674 checks, 0 failures. MinGW with the real irtsdk.h: 0 errors. tools/gex-build.bat hashes StageBox.h
  (dependency check 349 passed).

## Not done / open
- Row 1 shows the engine's Trap evidence (P / Q), not a separate "bids added vs offers added" count (the Defense pane's net bars are
  not in the typed records yet). Exhaustion "tip prints" are not in any record yet - pace only.
- F? itself (lsBookFlow) is not built yet; the row appears once DEF records carry F?.
