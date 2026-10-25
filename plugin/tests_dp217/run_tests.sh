#!/bin/sh
# lsDeltaProfile 2.17.0 / 2.18.0 / SignalBoard 1.1.0 tests (Linux, run from plugin/tests_dp217). The old tests_delta suites compile with this folder's
# mock first (it adds getBlendedColor): g++ ... -I../tests_dp217 -I. -I.. <suite>.cpp (run from tests_delta).
# (2.18.0) test_dp218: the same-or-next-candle close with his both-lows / both-highs pivot rule, conf=same|next records, header upgrade.
set -e
CXX="g++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=all -g -I. -I../tests_delta -I.."
# (2.23.0) test_dp217 / test_dp218 assert the RETIRED 2.17-2.18 tentative A (TA lines, board flag 2, 25-field D lines): superseded by test_dp223 (kept for history, not run)
$CXX test_dp219.cpp -o /tmp/t_dp219 && /tmp/t_dp219
$CXX test_dp222.cpp -o /tmp/t_dp222 && /tmp/t_dp222   # 2.22.0: the shared A, one circle per candle, the close required
$CXX test_dp223.cpp -o /tmp/t_dp223 && /tmp/t_dp223   # 2.23.0: the researched A (decided at its candle / the next candle), AL late log, Draw late A
# replay (real research bars through the host, straight + fresh engine after every candle): g++ -O2 ... replay_dp223.cpp -o /tmp/rp223 && /tmp/rp223 <ES_2026-10-22.txt> ES tag
