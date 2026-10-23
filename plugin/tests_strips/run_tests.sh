#!/bin/sh
# Typed reversal pipeline 1.2.0. Synthetic integrity tests, not a trading backtest.
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
P=$(CDPATH= cd -- "$D/.." && pwd)
B=${BUILD:-/tmp/strips-build}; mkdir -p "$B"
CXX=${CXX:-g++}
F='-std=c++17 -Wall -Wextra -Werror -O0 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie'
for N in test_strips test_stage_core test_evidence_transport; do
    "$CXX" $F -I"$P" "$D/$N.cpp" -o "$B/$N"
    "$B/$N"
done
"$CXX" $F -DRS_TEST_CLOCK -I"$P" -I"$P/tests_aggbook/mock" "$D/test_stage_pipeline.cpp" -o "$B/test_stage_pipeline"
"$B/test_stage_pipeline"
echo 'Typed strip/core/transport/host tests passed. Old untyped stage replay is not a live confirmation or performance test.'
