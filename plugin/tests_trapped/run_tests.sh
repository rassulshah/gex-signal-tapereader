#!/bin/sh
# lsTrapped / lsTrappedMarks 1.0.0 tests: logic (real ES 10-05 / NQ 10-09 fixtures in tests_ob126 + unit cases; ASan/UBSan, -Werror)
# + MinGW syntax check of BOTH builds against the REAL irtsdk.h (-Dsmall=char, lesson 133).
# usage: tests_trapped/run_tests.sh [path-to-real-sdk-include]
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); B=${BUILD:-/tmp/trp-tests}; mkdir -p "$B"; rm -f "$B"/t_trp_*
F='-std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-sanitize-recover=undefined'
g++ $F "$D/test_trapped_logic.cpp" -o "$B/t_trp_logic" && "$B/t_trp_logic" "$D/.."
if [ -n "${1:-}" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
  for v in Trapped TrappedMarks; do
    x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Dsmall=char -I"$1" -I"$D/.." "$D/wrap_real_sdk_$v.cpp" > "$B/mingw_$v.txt" 2>&1 || true
    if grep -v irtsdk.h "$B/mingw_$v.txt" | grep -qE "error|warning"; then cat "$B/mingw_$v.txt"; exit 1; fi
    echo "MinGW real SDK ($v): 0 errors, 0 warnings outside the SDK"
  done
fi
echo "lsTrapped tests passed"
