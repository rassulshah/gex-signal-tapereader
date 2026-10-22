#!/bin/sh
# lsTurnOrder 1.0.0 tests: logic + mock host (ASan / UBSan, -Werror) + MinGW real-SDK syntax (-Dsmall=char). Replay: replay_tor <events.csv> <MKT>
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); P=$(CDPATH= cd -- "$D/.." && pwd); B=${BUILD:-/tmp/tor-build}; mkdir -p "$B"; rm -f "$B/t_tor" "$B/t_tor_host"
g++ -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined "$D/test_tor.cpp" -o "$B/t_tor" && "$B/t_tor"
g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -g -fsanitize=address,undefined -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I"$P" -I"$P/tests_aggbook/mock" "$D/test_tor_host.cpp" -o "$B/t_tor_host" && ASAN_OPTIONS=detect_leaks=0 "$B/t_tor_host"
g++ -std=c++17 -O2 -Wall -Wextra -Werror "$D/replay_tor.cpp" -o "$B/replay_tor"
SDK=${SDK:-/mnt/user-data/uploads/sdk/c++/include}
if [ -d "$SDK" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
 printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "TurnOrder.cpp"\n' > "$B/wrap_TurnOrder.cpp"
 x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Wno-misleading-indentation -Dsmall=char -I"$SDK" -I"$P" "$B/wrap_TurnOrder.cpp" > "$B/syntax.log" 2>&1
 if grep 'warning:' "$B/syntax.log" | grep -v -F "$SDK/"; then echo "FAILED non-SDK warning"; exit 1; fi
 echo "Real-SDK syntax passed: TurnOrder"
else echo "Real-SDK syntax NOT RUN (no SDK / MinGW)"; fi
