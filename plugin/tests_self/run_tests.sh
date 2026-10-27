#!/bin/sh
# self-awareness (SelfAware.h + the nightly lra.self_aware): SDK-free tests on Linux + e2e records C++ -> Python + optional real-SDK syntax check.
# usage: sh run_tests.sh <plugin dir> [<sdk include dir> <patched plugin dir>]
D=$(cd "$(dirname "$0")" && pwd); P="${1:-$D/..}"; B="${TMPDIR:-/tmp}/tests_self.$$"; mkdir -p "$B/e2e"; rc=0
g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE -I"$D" -I"$P" "$D/test_self.cpp" -o "$B/test_self" && "$B/test_self" || rc=1
g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE -I"$D" -I"$P" "$D/e2e_self.cpp" -o "$B/e2e_self" && LRA_FLEX_DIR="$B/e2e" "$B/e2e_self" || rc=1
T="${SELF_AWARE_PY:-$D/test_self_aware.py}"   # in the LRA repo: analytics/test_self_aware.py (set SELF_AWARE_PY); the e2e records are in $B/e2e
if [ -f "$T" ]; then python3 "$T" "$B/e2e" || rc=1; else echo "lra.self_aware test not here (set SELF_AWARE_PY): C++ side only"; fi
if [ -n "$2" ] && [ -n "$3" ] && command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
  printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "DeltaProfile.cpp"\n' > "$B/wrap.cpp"
  if x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Wno-misleading-indentation -Dsmall=char -I"$2" -I"$3" -I"$P" "$B/wrap.cpp" > "$B/syntax.log" 2>&1 && ! grep -v irtsdk.h "$B/syntax.log" | grep -qE 'error|warning'; then echo "real-SDK syntax DeltaProfile: 0 errors"; else echo "real-SDK syntax DeltaProfile: FAILED ($B/syntax.log)"; rc=1; fi
fi
exit $rc
