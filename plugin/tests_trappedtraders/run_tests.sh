#!/bin/sh
# lsTrappedTraders logic tests on real ES 10-26 + NQ 10-22 data (ASan + UBSan, warnings are errors)
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -Werror -Wall -Wextra -I.. test_tt_logic.cpp -o /tmp/tt_logic_test
/tmp/tt_logic_test fixtures
