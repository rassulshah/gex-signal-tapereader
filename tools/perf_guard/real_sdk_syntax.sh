#!/bin/sh
# real_sdk_syntax.sh <plugin-dir> [names...] -- MinGW + the REAL IRT SDK syntax check of every plugin .cpp (the tests_tor wrapper
# recipe: NOMINMAX, windows.h, irtsdk.h with __GNUC__ hidden, -Dsmall=char). Prints one line per plugin: OK / WARN / FAIL.
# Used by PerfGuard to prove the instrumented sources compile against the real SDK with no new warnings (compare with the originals).
P=$(CDPATH= cd -- "$1" && pwd); shift
SDK=${SDK:-/mnt/user-data/uploads/sdk/c++/include}
B=${BUILD:-$(mktemp -d)}
NAMES="$*"
[ -z "$NAMES" ] && NAMES=$(cd "$P" && ls *.cpp | sed 's/\.cpp$//' | grep -v '^test_')
for n in $NAMES; do
  [ -f "$P/$n.cpp" ] || continue
  printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "%s.cpp"\n' "$n" > "$B/wrap_$n.cpp"
  if x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Wno-misleading-indentation -Dsmall=char -I"$SDK" -I"$P" "$B/wrap_$n.cpp" > "$B/syntax_$n.log" 2>&1; then
    w=$(grep 'warning:' "$B/syntax_$n.log" | grep -v -F "$SDK/" | wc -l)
    if [ "$w" -gt 0 ]; then echo "WARN $n $w"; else echo "OK $n"; fi
  else
    echo "FAIL $n $(grep -m1 'error:' "$B/syntax_$n.log" | cut -c1-200)"
  fi
done
