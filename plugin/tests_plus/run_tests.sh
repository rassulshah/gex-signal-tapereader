#!/bin/sh
# awareness_plus (lsContext 0.2.0 / ContextPump.h 0.3.0): SDK-free logic tests on the HB_FAKE board + the real-SDK mingw syntax check.
# usage: sh run_tests.sh [<sdk include dir>]     (run from plugin/tests_plus; the headers are in plugin/)
D=$(cd "$(dirname "$0")" && pwd); P="$D/.."; B="${TMPDIR:-/tmp}/tests_plus.$$"; mkdir -p "$B"
rc=0
for T in test_plus test_pump_plus test_next; do
  g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE -I"$P" "$D/$T.cpp" -o "$B/$T" && "$B/$T" || rc=1
done
g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE -I"$P" "$D/e2e_plus_writer.cpp" -o "$B/w" && mkdir -p "$B/flex" && "$B/w" "$B/flex" >/dev/null || rc=1
n=$(cat "$B"/flex/Context-ES-*.rec 2>/dev/null | grep -c '^EXP|1|'); if [ "$n" -gt 200 ]; then echo "e2e records: $n EXP verdicts"; else echo "e2e records: FAILED ($n)"; rc=1; fi
if [ -n "$1" ] && command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
  printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "Context.cpp"\n' > "$B/wrap.cpp"
  if x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Wno-misleading-indentation -Dsmall=char -I"$1" -I"$P" "$B/wrap.cpp" > "$B/syntax.log" 2>&1; then echo "real-SDK syntax Context: 0 errors"; else echo "real-SDK syntax Context: FAILED (see $B/syntax.log)"; rc=1; fi
fi
exit $rc
