#!/bin/sh
# lsStopRun 1.2.0 tests: logic + host (mock SDK, ASan/UBSan, -Werror) + MinGW syntax check against the REAL irtsdk.h (-Dsmall=char).
# usage: tests_st/run_tests.sh [path-to-real-sdk-include]
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); B=${BUILD:-/tmp/srm-tests}; mkdir -p "$B"; rm -f "$B"/t_st_*
F='-std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-sanitize-recover=undefined'
g++ $F "$D/test_st_logic.cpp" -o "$B/t_st_logic" && "$B/t_st_logic"
g++ $F -Wno-misleading-indentation -Wno-delete-non-virtual-dtor -DSRM_TEST_CLOCK -I"$D/mock" "$D/test_st_host.cpp" -o "$B/t_st_host" && "$B/t_st_host"
g++ -std=c++17 -O2 -Wall -Wextra -Werror "$D/replay_st.cpp" -o "$B/replay_st"
if [ -n "${1:-}" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Dsmall=char -I"$1" -I"$D/.." "$D/wrap_real_sdk.cpp" > "$B/mingw.txt" 2>&1
  if grep -v irtsdk.h "$B/mingw.txt" | grep -qE "error|warning"; then cat "$B/mingw.txt"; exit 1; fi
  echo "MinGW real SDK: 0 errors, 0 warnings outside the SDK"
fi
echo "lsStopRun tests passed"
