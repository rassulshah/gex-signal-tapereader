#!/bin/sh
# lsSweepRefill 1.1.0 tests: logic + host (mock SDK, ASan/UBSan, -Werror) + replay build + MinGW syntax check against the REAL irtsdk.h (-Dsmall=char).
# usage: tests_sr/run_tests.sh [path-to-real-sdk-include]
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); B=${BUILD:-/tmp/swr-tests}; mkdir -p "$B"; rm -f "$B"/t_sr_*
F='-std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-sanitize-recover=undefined'
g++ $F "$D/test_sr_logic.cpp" -o "$B/t_sr_logic" && "$B/t_sr_logic"
g++ $F -Wno-misleading-indentation -Wno-delete-non-virtual-dtor -DSWR_TEST_CLOCK -I"$D/mock" "$D/test_sr_host.cpp" -o "$B/t_sr_host" && "$B/t_sr_host"
g++ -std=c++17 -O2 -Wall -Wextra -Werror "$D/replay_sr.cpp" -o "$B/replay_sr"
if [ -n "${1:-}" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Dsmall=char -I"$1" -I"$D/.." "$D/wrap_real_sdk.cpp" > "$B/mingw.txt" 2>&1
  if grep -v irtsdk.h "$B/mingw.txt" | grep -qE "error|warning"; then cat "$B/mingw.txt"; exit 1; fi
  echo "MinGW real SDK: 0 errors, 0 warnings outside the SDK"
fi
echo "lsSweepRefill tests passed"
