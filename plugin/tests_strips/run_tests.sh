#!/bin/sh
# LetterStrips.h 1.0.0 + ReversalStageLogic.h 1.0.0 + lsReversalStage 1.0.0 (2026-10-22): the strip priority / colours / offsets / record,
# the stage state machine (forming / failed / complete / order gate), the plugin on the mock host (record, no repaint across restarts,
# bar by bar, settled, settings, hover, two charts) and the replay on Rassul's real ES 3-min day. The strip drawing on the two panes:
# tests_aggbook/test_attack_126_host.cpp (attacker) and test_aggbook_host.cpp -DBOOK (Defense). Run from plugin/tests_strips.
set -e
B=${BUILD:-/tmp/strips-build}; mkdir -p "$B"
CXX="g++ -std=c++17 -Wall -Wextra -Werror -O0 -fsanitize=address,undefined -fno-sanitize-recover=all"
$CXX -I.. -include array -include cstdlib test_strips.cpp -o "$B/test_strips" && "$B/test_strips"
$CXX -DRS_TEST_CLOCK -I.. -I../tests_aggbook/mock test_stage_host.cpp -o "$B/test_stage_host" && "$B/test_stage_host"
g++ -std=c++17 -Wall -Wextra -Werror -O1 -I.. replay_stage.cpp -o "$B/replay_stage" && "$B/replay_stage" replay/ES-180-20261022-bars.csv replay/ES-180-turns-20261022.csv
