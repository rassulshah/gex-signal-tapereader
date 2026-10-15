#!/bin/sh
# lsBigBidAsk 1.0.0 tests (Linux): logic + host on the mock SDK under ASan / UBSan, then the MinGW syntax check against the REAL
# irtsdk.h (pass its folder as $1, e.g. a copy of C:\Program Files\LinnSoft\InvestorRT\sdk\c++\include). Run from plugin/tests_bigba.
set -e
CXX="g++ -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -Werror -Wall -Wextra -g"
$CXX -o /tmp/bba_logic test_bigba_logic.cpp && /tmp/bba_logic
$CXX -I. -I.. -o /tmp/bba_host test_bigba_host.cpp && /tmp/bba_host
if [ -n "$1" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
  D=$(mktemp -d); cp "$1/irtsdk.h" wrap_BigBidAsk.cpp "$D/"
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -I"$D" -I.. "$D/wrap_BigBidAsk.cpp" > "$D/out.txt" 2>&1
  E=$(grep -c "error" "$D/out.txt" || true); W=$(grep "warning" "$D/out.txt" | grep -vc "irtsdk.h" || true)
  echo "MinGW vs real irtsdk.h: $E errors, $W warnings outside the SDK"; rm -rf "$D"; [ "$E" = "0" ] && [ "$W" = "0" ]
fi
