#!/bin/sh
# AskBoard.h 0.1.0 (the ASK capability: raw-data index, request / answer channel, question lists): SDK-free logic tests on Linux
# (HB_FAKE board) + a Windows (real board, no HB_FAKE) mingw syntax check of the header with an optional SDK include dir.
# usage: sh run_tests.sh [<sdk include dir>]     (AskBoard.h sits in .. ; MarketState.h / BannerBoard.h / ContextRead.h in .. or -I dirs below)
D=$(cd "$(dirname "$0")" && pwd); P="$D/.."; B="${TMPDIR:-/tmp}/tests_ask.$$"; mkdir -p "$B"
INC="-I$P"; for X in $ASK_INC; do INC="$INC -I$X"; done
rc=0
g++ -std=c++17 -Wall -Wextra -Werror -DHB_FAKE $INC "$D/test_ask.cpp" -o "$B/test_ask" && "$B/test_ask" || rc=1
if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
  S=""; [ -n "$1" ] && S="-I$1"
  W='#define NOMINMAX\n#include <windows.h>\n'; [ -n "$1" ] && W="$W"'#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n'
  printf "$W"'#include "AskBoard.h"\nint askboard_syntax() { return (int)rx::MAX_INDEX + (int)ask::MAX_Q; }\n' > "$B/wrap_ask.cpp"
  if x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Dsmall=char $S $INC "$B/wrap_ask.cpp" > "$B/syntax-ask.log" 2>&1 && ! grep 'warning:' "$B/syntax-ask.log" | grep -v -F "${1:-@none@}/" | grep -q .; then echo "Windows syntax AskBoard.h${1:+ + real SDK}: 0 errors, 0 own warnings"; else echo "Windows syntax AskBoard.h: FAILED"; head -20 "$B/syntax-ask.log"; rc=1; fi
fi
exit $rc
