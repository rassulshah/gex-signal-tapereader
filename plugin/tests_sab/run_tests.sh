#!/bin/sh
# SharedAbsorb.h 1.0.0 (2026-10-22): the shared absorption A - logic suite + lsBidAskStats 1.27.0 end to end on the mock host.
# The other plugins' shared-A host checks: tests_dp217/test_dp222.cpp (Delta Profile one circle per candle), tests_aggbook/test_aggbook_host.cpp
# -DBOOK (Defense pane, the attacker-colour candle), tests_bba115/test_bba121.cpp [1.22.0] (Big Bid / Ask circle). Run from plugin/tests_sab.
set -e
B=${BUILD:-/tmp/sab-build}; mkdir -p "$B"
CXX="g++ -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-sanitize-recover=all"
$CXX -I.. test_sab.cpp -o "$B/test_sab" && "$B/test_sab"
$CXX -fno-pie -no-pie -Wno-misleading-indentation -DBAS_TEST_CLOCK -DPC_TEST_CURSOR -I.. -I../tests_aggbook/mock test_sab_bas_host.cpp -o "$B/test_sab_bas_host" && "$B/test_sab_bas_host"
