#!/bin/sh
# lsBigBidAsk 1.15.0 tests (Linux, run from plugin/tests_bba115): tier logic + host on this folder's mock SDK, then the 1.14 suites.
set -e
CXX="g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -g -O1"
$CXX -I. -I.. test_bba115.cpp -o /tmp/t_bba115 && /tmp/t_bba115
(cd ../tests_bigba && $CXX -I. -I.. test_bigba_114.cpp -o /tmp/t_bba114 && /tmp/t_bba114 | tail -1 && $CXX -I. -I.. test_bigba_swing114.cpp -o /tmp/t_bbasw && /tmp/t_bbasw | tail -1)
