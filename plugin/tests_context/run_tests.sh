#!/bin/sh
# lsContext 0.1.0 + the smart-read hook (AwareHook.h): SDK-free logic tests on Linux (HB_FAKE board) + the real-SDK mingw syntax check.
# usage: sh run_tests.sh [<sdk include dir>]
D=$(cd "$(dirname "$0")" && pwd); P="$D/.."; B="${TMPDIR:-/tmp}/tests_context.$$"; mkdir -p "$B"; export LRA_FLEX_DIR="$B/flex"; mkdir -p "$LRA_FLEX_DIR"
rc=0
for T in test_context test_pump test_hook; do
  g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE -I"$P" "$D/$T.cpp" -o "$B/$T" && "$B/$T" || rc=1
done
# the v2 records the nightly reads (lra.context_links / lra.team_health test them: analytics/test_e2e_v2.py + fixtures)
E="$B/e2e"; mkdir -p "$E"
g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE -I"$P" "$D/e2e_writer.cpp" -o "$B/e2e_writer" && LRA_FLEX_DIR="$E" "$B/e2e_writer" >/dev/null || rc=1
n=$(cat "$E"/Context-AWR-*.rec 2>/dev/null | grep -c '^AWR|2|'); c=$(cat "$E"/Context-ES-*.rec 2>/dev/null | grep -c '^CTX|.*cls_v=2')
if [ "$n" = 390 ] && [ "$c" = 130 ]; then echo "e2e records: 130 class-tagged CTX, 390 AWR v2"; else echo "e2e records: FAILED ($c CTX, $n AWR)"; rc=1; fi
if [ -n "$1" ] && command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
  for N in Context DeltaProfile BidAskStats HodLod ReversalStage; do
    printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "%s.cpp"\n' "$N" > "$B/wrap_$N.cpp"
    if x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Wno-misleading-indentation -Dsmall=char -I"$1" -I"$P" "$B/wrap_$N.cpp" > "$B/syntax-$N.log" 2>&1; then echo "real-SDK syntax $N: 0 errors"; else echo "real-SDK syntax $N: FAILED (see $B/syntax-$N.log)"; rc=1; fi
  done
fi
exit $rc
