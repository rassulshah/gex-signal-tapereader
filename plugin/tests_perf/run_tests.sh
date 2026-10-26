#!/bin/sh
# plugin/tests_perf/run_tests.sh -- PerfGuard 1.0.0: the header in fake mode (ASan / UBSan, -Werror), the Windows-macro simulation,
# MinGW + the real SDK (a wrapper with the real irtsdk.h), the e2e writer, and the perf lint gate (new findings vs the baseline).
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd); P=$(CDPATH= cd -- "$D/.." && pwd); B=${BUILD:-/tmp/perf-build}; mkdir -p "$B"
g++ -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -DLSPERF_FAKE -I"$P" "$D/test_perfguard.cpp" -o "$B/t_perf" && "$B/t_perf"
printf '#define min(a,b) (((a)<(b))?(a):(b))\n#define max(a,b) (((a)>(b))?(a):(b))\n#define near\n#define far\n#include "PerfGuard.h"\nint main(){ return lsperf::smartAllowed("x","y") ? 0 : 1; }\n' > "$B/macro.cpp"
g++ -std=c++14 -Wall -Wextra -Werror -DLSPERF_FAKE -I"$P" "$B/macro.cpp" -o "$B/macro" && "$B/macro" && echo "Windows macro simulation passed"
g++ -std=c++17 -O1 -Wall -Wextra -Werror -DLSPERF_FAKE -I"$P" "$D/e2e_writer.cpp" -o "$B/e2e" && rm -rf "$B/flex" && mkdir -p "$B/flex" && "$B/e2e" "$B/flex" >/dev/null && test -s "$B/flex/Perf/perf-DeltaProfile-20261026.csv" && echo "e2e writer passed"
SDK=${SDK:-/mnt/user-data/uploads/sdk/c++/include}
if [ -d "$SDK" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
 printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "PerfGuard.h"\nstruct X : public cppExtension { int draw(void) { LSPERF_SCOPE("X", "draw"); return RTX_OK; } };\nint use(X* x) { return x->draw() + (lsperf::smartAllowed("X", "ESZ6") ? 0 : 1); }\n' > "$B/wrap_perf.cpp"
 x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Dsmall=char -I"$SDK" -I"$P" "$B/wrap_perf.cpp" > "$B/syntax.log" 2>&1 || { cat "$B/syntax.log"; exit 1; }
 if grep 'warning:' "$B/syntax.log" | grep -v -F "$SDK/"; then echo "FAILED non-SDK warning"; exit 1; fi
 echo "Real-SDK syntax passed: PerfGuard.h"
else echo "Real-SDK syntax NOT RUN (no SDK / MinGW)"; fi
T="$P/../tools/perf_guard"
if [ -f "$T/perf_lint.py" ]; then python3 "$T/perf_lint.py" --plugin "$P" --bat "$P/../tools/gex-build.bat" --baseline "$T/perf-lint-baseline.json" --gate; fi
echo "lsPerfGuard tests passed"
