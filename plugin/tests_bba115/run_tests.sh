#!/bin/sh
# lsBigBidAsk 1.15.0 tests (Linux, run from plugin/tests_bba115): tier logic + host on this folder's mock SDK, then the 1.14 suites.
set -e
CXX="g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -g -O1"
$CXX -I. -I.. test_bba115.cpp -o /tmp/t_bba115 && /tmp/t_bba115
$CXX -I. -I.. test_bba119.cpp -o /tmp/t_bba119 && /tmp/t_bba119   # 1.19.0: candles-back number + his settings (biggest in, skip, font, colours)
$CXX -I. -I.. test_bba116.cpp -o /tmp/t_bba116 && /tmp/t_bba116   # 1.16.0: Big Lot box, docking, category order, settings window
$CXX -I. -I.. test_bba121.cpp -o /tmp/t_bba121 && /tmp/t_bba121   # 1.21.0: breakout / breakdown (BO / BD + the print), NQ 2026-10-21 mockup replay
(cd ../tests_bigba && $CXX -I. -I.. test_bigba_114.cpp -o /tmp/t_bba114 && /tmp/t_bba114 | tail -1 && $CXX -I. -I.. test_bigba_swing114.cpp -o /tmp/t_bbasw && /tmp/t_bbasw | tail -1)
