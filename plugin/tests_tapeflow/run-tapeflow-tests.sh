#!/bin/bash
# runs every TapeFlow test suite with g++ (the cloud twin of run-logic-tests.bat). usage: run_tests.sh <plugin dir>
P=${1:-/tmp/gex/plugin}; B=${TFBIN:-/tmp/tapeflow-test-bin}; M=$P/tests_tapeflow/mock; SDK=${IRTSDK:-/tmp/claude-0/hdp/pkg/hourly-delta-profile-final/sdk/include}
mkdir -p $B; tot=0
one() { name=$1; shift; if g++ -std=c++17 -O1 -w -o $B/$name "$@" 2>$B/$name.err; then out=$($B/$name 2>&1); rc=$?; echo "== $name rc=$rc :: $(echo "$out" | grep -iE 'passed|failed|checks|ok|FAIL' | tail -3 | tr '\n' ' ')"; [ $rc -ne 0 ] && { tot=$((tot+1)); echo "$out" | grep FAIL | head -20; }; else echo "== $name COMPILE FAILED"; head -20 $B/$name.err; tot=$((tot+1)); fi; }
cd $P
one test_tapeflow_logic test_tapeflow_logic.cpp
one test_tapeflow_audit_regressions test_tapeflow_audit_regressions.cpp
one fuzz_tapeflow_logic fuzz_tapeflow_logic.cpp
one core_regression tests_tapeflow/core_regression.cpp
one support_regression tests_tapeflow/support_regression.cpp
[ -f tests_tapeflow/v115_regression.cpp ] && one v115_regression tests_tapeflow/v115_regression.cpp
[ -f tests_tapeflow/v200_signals.cpp ] && one v200_signals tests_tapeflow/v200_signals.cpp
for h in sim_irt_harness sim_backfill_v115 sim_v200 sim_v202 sim_v202node sim_v203 sim_v203r sim_marks; do [ -f tests_tapeflow/$h.cpp ] && one $h -I$M -I$P -I$SDK -include $M/shim.h tests_tapeflow/$h.cpp $M/mockirt.cpp; done
echo "SUITES FAILED: $tot"; exit $tot
