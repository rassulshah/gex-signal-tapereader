#!/bin/sh
# lsEventCal 1.0.0 tests: logic + host (mock SDK, two charts on one DLL, HOD / LOD dock, file states; ASan/UBSan, -Werror) + MinGW syntax check
# against the REAL irtsdk.h (-Dsmall=char). usage: tests_eventcal/run_tests.sh [path-to-real-sdk-include] [LRA-Calendar-ALL.csv]
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); B=${BUILD:-/tmp/ecal-tests}; mkdir -p "$B"
F='-std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-sanitize-recover=undefined'
g++ $F "$D/t_eventcal_100.cpp" -o "$B/t_logic" && "$B/t_logic" ${2:-}
g++ $F -Wno-delete-non-virtual-dtor -I"$D/mock" -I"$D/.." "$D/t_eventcal_host.cpp" -o "$B/t_host" && "$B/t_host"
if [ -n "${1:-}" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
  printf '#ifndef NOMINMAX\n#define NOMINMAX\n#endif\n#include <windows.h>\n#include "irtsdk.h"\n#include "EventCal.cpp"\n' > "$B/wrap.cpp"
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Dsmall=char -I"$1" -I"$D/.." "$B/wrap.cpp" > "$B/mingw.txt" 2>&1 || true
  if grep -v irtsdk.h "$B/mingw.txt" | grep -qE "error|warning"; then cat "$B/mingw.txt"; exit 1; fi
  echo "MinGW real SDK: 0 errors, 0 warnings outside the SDK"
fi
echo "lsEventCal tests passed"
