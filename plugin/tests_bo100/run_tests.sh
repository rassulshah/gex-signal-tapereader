#!/bin/sh
# lsBreakout 1.1.0 tests (1.0.0 + Pb: test_bo110_pb) (Linux, from plugin/tests_bo100): parity with lsBigBidAsk (ES Oct 30 records) + the host on the mock SDK under
# ASan / UBSan, then (with the real irtsdk.h folder as $1) the MinGW syntax check of Breakout.cpp against the REAL SDK, and the render.
set -e
CXX="g++ -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -Werror -Wall -Wextra -g"
$CXX -I.. -o /tmp/bo100_parity test_bo100_parity.cpp && /tmp/bo100_parity
$CXX -I.. -o /tmp/bo110_pb test_bo110_pb.cpp && /tmp/bo110_pb
$CXX -IBreakout_test_support -I.. -o /tmp/bo100_host test_bo100_host.cpp && /tmp/bo100_host
g++ -std=c++17 -IBreakout_test_support -I.. -o /tmp/bo100_rd render_dump.cpp && /tmp/bo100_rd > /tmp/bo100_dump.txt && python3 render_check.py /tmp/bo100_dump.txt
if [ -n "$1" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
  D=$(mktemp -d); cp "$1/irtsdk.h" wrap_Breakout.cpp "$D/"
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -I"$D" -I.. "$D/wrap_Breakout.cpp" > "$D/out.txt" 2>&1
  E=$(grep -c "error" "$D/out.txt" || true); W=$(grep "warning" "$D/out.txt" | grep -vc "irtsdk.h" || true)
  echo "MinGW vs real irtsdk.h: $E errors, $W warnings outside the SDK"; rm -rf "$D"; [ "$E" = "0" ] && [ "$W" = "0" ]
fi
