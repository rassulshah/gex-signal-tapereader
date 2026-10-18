#!/bin/sh
# lsDeltaProfile 2.17.0 / SignalBoard 1.1.0 tests (Linux, run from plugin/tests_dp217). The old tests_delta suites compile with this folder's
# mock first (it adds getBlendedColor): g++ ... -I../tests_dp217 -I. -I.. <suite>.cpp (run from tests_delta).
set -e
g++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=all -g \
    -I. -I../tests_delta -I.. test_dp217.cpp -o /tmp/t_dp217 && /tmp/t_dp217
