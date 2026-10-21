#!/bin/sh
# Complete Defense / Effort Reward audit runner. Syntax checks are not native DLL builds.
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
P=$(CDPATH= cd -- "$D/.." && pwd)
B=${BUILD:-/tmp/gex-audit-build}
mkdir -p "$B"
MODE=${1:---full}
case "$MODE" in --full|--standalone) ;; *) echo "Usage: $0 [--full|--standalone]" >&2; exit 2 ;; esac
CXX=${CXX:-g++}
FLAGS='-std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-pie -no-pie'
step() { echo "== $1" >&2; shift; "$@" || { echo "FAILED: $*" >&2; exit 1; }; }
compile_run() {
 name=$1; file=$2; shift 2
 step "$name compile" "$CXX" $FLAGS -I"$P" "$@" "$D/$file" -o "$B/$name"
 step "$name run" "$B/$name" "$P"
}
compile_run defense_turn test_defenseturn.cpp
compile_run give_up_trap_withdrew test_gtw.cpp -Wno-misleading-indentation
compile_run effort_reward_audit test_er_audit.cpp
step 'Native rebuild dependency source checks (not batch execution)' python3 "$D/test_build_dependencies.py"
if [ "$MODE" = --standalone ]; then
 echo 'Standalone checks passed; full host, original integration suites, and real-SDK syntax NOT RUN.'
 exit 0
fi
# Retain every target from the original full archive. No historical suite is dropped.
compile_run agg_logic test_aggbook_113.cpp
compile_run bidask_host test_aggbook_host.cpp -Wno-misleading-indentation -DBAS_TEST_CLOCK -DPC_TEST_CURSOR -I"$D/mock"
compile_run refill_logic test_aggbook_150.cpp
compile_run moves test_moves.cpp
compile_run scale_axis test_cap170.cpp -Wno-misleading-indentation -I"$D/mock"
compile_run effort_reward_original test_er100.cpp
compile_run zero_line test_aggbook_151.cpp -Wno-misleading-indentation -I"$D/mock"
compile_run read_rows test_aggbook_115.cpp
compile_run big_lot_logic test_aggbook_114.cpp
compile_run bidask_big_lot_host test_aggbook_114_host.cpp -Wno-misleading-indentation -DBAS_TEST_CLOCK -DPC_TEST_CURSOR -I"$D/mock"
compile_run bookflow_host test_aggbook_host.cpp -Wno-misleading-indentation -DBOOK -DOFF_TEST_CLOCK -DPC_TEST_CURSOR -I"$D/mock"
compile_run defense_lines_host test_defenselines_host.cpp -Wno-misleading-indentation -I"$D/mock"
compile_run defense_real_header_audit test_defense_114_audit.cpp -Wno-misleading-indentation -I"$D/mock"
echo 'All included logic and mock-host test targets passed.'
# Default relative to this script, not the caller's current directory.
SDK=${SDK:-$P/../sdk}
if [ ! -d "$SDK" ]; then
 echo "Real-SDK syntax NOT RUN: no SDK directory ($SDK). No complete validation claim."
 exit 0
fi
SDK=$(CDPATH= cd -- "$SDK" && pwd)
MINGW_CXX=${MINGW_CXX:-x86_64-w64-mingw32-g++}
command -v "$MINGW_CXX" >/dev/null 2>&1 || { echo "Real-SDK syntax BLOCKED: compiler $MINGW_CXX missing" >&2; exit 2; }
for N in BidAskStats BookFlow DefenseLines; do
 printf '#define NOMINMAX\n#include <windows.h>\n#pragma push_macro("__GNUC__")\n#undef __GNUC__\n#include "irtsdk.h"\n#pragma pop_macro("__GNUC__")\n#include "%s.cpp"\n' "$N" > "$B/wrap_$N.cpp"
 if ! "$MINGW_CXX" -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unknown-pragmas -Wno-misleading-indentation -I"$SDK" -I"$P" "$B/wrap_$N.cpp" >"$B/syntax-$N.log" 2>&1; then
  cat "$B/syntax-$N.log" >&2; echo "FAILED real-SDK syntax: $N" >&2; exit 1
 fi
 if grep 'warning:' "$B/syntax-$N.log" | grep -v -F "$SDK/" >"$B/warnings-$N.log"; then
  cat "$B/warnings-$N.log" >&2; echo "FAILED non-SDK warning: $N" >&2; exit 1
 fi
 echo "Real-SDK syntax passed: $N (not a linked native DLL or runtime test)"
done
echo 'Complete supplied aggbook targets and real-SDK syntax passed; native MSVC linking and IRT runtime remain unverified.'
