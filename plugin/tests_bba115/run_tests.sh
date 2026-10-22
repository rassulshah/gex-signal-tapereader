#!/bin/sh
# lsBigBidAsk 1.15.0 tests (Linux, run from plugin/tests_bba115): tier logic + host on this folder's mock SDK, then the 1.14 suites.
set -e
CXX="g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -g -O1"
$CXX -I. -I.. test_bba115.cpp -o /tmp/t_bba115 && /tmp/t_bba115
$CXX -I. -I.. test_bba119.cpp -o /tmp/t_bba119 && /tmp/t_bba119   # 1.19.0: candles-back number + his settings (biggest in, skip, font, colours)
$CXX -I. -I.. test_bba116.cpp -o /tmp/t_bba116 && /tmp/t_bba116   # 1.16.0: Big Lot box, docking, category order, settings window
# (1.26.0) test_bba121 / test_bba123 pin the pre-1.26 pivot rule (the higher of the last 2 pivots) and the BD letter on the NQ 10-21 mockup:
# superseded by Rassul 18:01 CT 2026-10-22 (the last 3 INTACT swings, BO for both sides) - kept for reference, not run. Their boWaves
# logic (green / red, same wave, wick_wave_ended) is unchanged; test_bba126 covers the new pivot rule, the OB print and the no-repaint replay.
$CXX -I. -I.. test_bba126.cpp -o /tmp/t_bba126 && /tmp/t_bba126   # 1.26.0: intact-swing BO, OB print + letters (no boxes), ES 10-05 / NQ 10-09 replay, restart after every candle
(cd ../tests_bigba && $CXX -I. -I.. test_bigba_114.cpp -o /tmp/t_bba114 && /tmp/t_bba114 | tail -1 && $CXX -I. -I.. test_bigba_swing114.cpp -o /tmp/t_bbasw && /tmp/t_bbasw | tail -1)
