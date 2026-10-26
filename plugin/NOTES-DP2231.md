# lsDeltaProfile 2.23.1 (2026-10-25, Rassul's chart spec "Delta Profile next-candle A coloured green / red like every A")

## Change (display only, records unchanged)
- A node A decided at the NEXT candle's close (conf=next) is now coloured by THAT candle: green when the confirmation candle closed green
  (bullish A), red when it closed red (bearish A). Before, the 2.22.0 defenders'-close rule looked at the peak candle, so almost every
  next-candle A showed as the gray circle.
- Same-candle A: unchanged (its own candle's close). Shared A: unchanged. Old-path (late) A: still gray unless "Draw late A" is ticked.
- Still ONE circle per candle; the circle stays on the peak candle (where the absorption happened).
- Files: DeltaProfile.cpp (DLT_VERSION / setVersion 2.23.1), DeltaProfileSettings.h (PLUGIN_VERSION 2.23.1, parameter version 17 unchanged).
  DeltaProfileCore.h unchanged (2.23.0).

## Tests (Linux mock, ASan/UBSan)
- tests_dp217/run_tests.sh: dp219 7, dp222 14/14, dp223 46/46. Updated expectations: dp223 "[host] next" now 1 coloured ring (was gray);
  dp222 "[red]" now one green ring (the node A confirmed by the next candle's green close).
- tests_delta/test_trap221: 99/99 (version pins 2.23.1).
- Settings contract: 43,100 checks, 0 failures. MinGW -fsyntax-only with the real irtsdk.h: 0 errors.

## Verify on his chart
- Version "2.23.1" next to the title. A next-candle A (D line conf=next) shows a green / red circle, not gray, when the next candle closed
  in the defenders' colour.
