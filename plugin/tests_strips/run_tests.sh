#!/bin/sh
# Typed reversal pipeline 2.0.0 (4-step model). Synthetic integrity tests, not a trading backtest.
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
P=$(CDPATH= cd -- "$D/.." && pwd)
B=${BUILD:-/tmp/strips-build}; mkdir -p "$B"
CXX=${CXX:-g++}
F='-std=c++17 -Wall -Wextra -Werror -O0 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie'
for N in test_strips test_stage_core test_evidence_transport test_stagebox; do   # (2.1.0) + the stage info box
    "$CXX" $F -I"$P" "$D/$N.cpp" -o "$B/$N"
    "$B/$N"
done
"$CXX" $F -DRS_TEST_CLOCK -I"$P" -I"$P/tests_aggbook/mock" "$D/test_stage_pipeline.cpp" -o "$B/test_stage_pipeline"
"$CXX" $F -DRS_TEST_CLOCK -I"$P" -I"$P/tests_aggbook/mock" "$D/test_stagebox_host.cpp" -o "$B/test_stagebox_host"   # (2.1.0) letters 1 2 3 C + the stage info box on the mock host
ASAN_OPTIONS=detect_leaks=0 "$B/test_stagebox_host"
"$B/test_stage_pipeline"
"$CXX" $F -I"$P" "$D/replay_stage.cpp" -o "$B/replay_stage"
( cd "$D" && "$B/replay_stage" replay/ES-180-20261022-bars.csv replay/ES-180-turns-20261022.csv )   # (2.0.0) gate + restart replay on his ES day: 0 differences
echo 'Typed strip/core/transport/host tests passed. Old untyped stage replay is not a live confirmation or performance test.'
