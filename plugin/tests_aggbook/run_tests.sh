#!/bin/sh
# tests_aggbook (2026-10-18): lsBidAskStats 1.13.0 (Aggression) + lsBookFlow 1.3.0 - logic (SDK-free) + both plugins end to end on the
# mock host (mock/irtsdk.h = the tests_of test double) + the real-SDK syntax check (MinGW). Run from anywhere: sh tests_aggbook/run_tests.sh
D=$(cd "$(dirname "$0")" && pwd); P="$D/.."; B=${BUILD:-/tmp/aggbook-build}; mkdir -p "$B"
CXX="g++ -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-sanitize-recover=undefined"
step() { echo "== $1" >&2; shift; "$@" || { echo "FAILED: $*"; exit 1; }; }
step "logic" $CXX -I"$P" "$D/test_aggbook_113.cpp" -o "$B/tl"; step "run" "$B/tl" "$P"
# OrderFlowFeed.h 1.10.1 has two pre-existing -Wmisleading-indentation warnings (shared header, not ours)
step "BidAskStats host" $CXX -Wno-misleading-indentation -DBAS_TEST_CLOCK -I"$D/mock" -I"$P" "$D/test_aggbook_host.cpp" -o "$B/thb"; step "run" "$B/thb"
step "Big Lot logic (1.14.0)" $CXX -I"$P" "$D/test_aggbook_114.cpp" -o "$B/tbl"; step "run" "$B/tbl"
step "BidAskStats Big Lot host (1.14.0)" $CXX -Wno-misleading-indentation -DBAS_TEST_CLOCK -I"$D/mock" -I"$P" "$D/test_aggbook_114_host.cpp" -o "$B/tblh"; step "run" "$B/tblh"
step "BookFlow host" $CXX -Wno-misleading-indentation -DBOOK -DOFF_TEST_CLOCK -I"$D/mock" -I"$P" "$D/test_aggbook_host.cpp" -o "$B/thf"; step "run" "$B/thf"
SDK=${SDK:-/mnt/user-data/uploads/sdk/c++/include}
if [ -d "$SDK" ]; then for N in BidAskStats BookFlow; do
  printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "%s.cpp"\n' "$N" > "$B/wrap_$N.cpp"
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Wno-misleading-indentation -I"$SDK" -I"$P" "$B/wrap_$N.cpp" 2>"$B/syntax-$N.log"
  E=$(grep -c ' error' "$B/syntax-$N.log"); W=$(grep 'warning' "$B/syntax-$N.log" | grep -vc "$SDK")
  echo "syntax $N: $E errors, $W warnings outside the SDK"; [ "$E" = 0 ] && [ "$W" = 0 ] || { head -30 "$B/syntax-$N.log"; exit 1; }
done; else echo "real-SDK syntax check SKIPPED (no $SDK)"; fi
echo "tests_aggbook: all passed"
