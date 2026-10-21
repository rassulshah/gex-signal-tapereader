#!/bin/sh
# MBO Roadmap Top Defender #1 (2026-10-21): AbsorbLogic.h, SignalBoard kind O, OrderFlowFeed absorbedPx, DeltaProfile 2.20.0 wick-A record / settings
set -e
CXX="g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -g"
$CXX -fsanitize=address,undefined -fno-sanitize-recover=all -I. -I.. test_absorb.cpp -o /tmp/t_absorb && /tmp/t_absorb
$CXX -Wno-unused-function -I../tests_dp217 -I../tests_delta -I.. test_dp220_record.cpp -o /tmp/t_dp220 && /tmp/t_dp220
