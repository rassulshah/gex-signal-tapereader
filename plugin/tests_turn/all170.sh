#!/bin/bash
# lsTurn 1.4.1: rebuild and run every check (research machine; P = this plugin folder, H = hostdata, EV = ES events to 10:33 AM)
set -e
P=${P:-/home/claude/tst3}; T=$P/tests_turn; W=/home/claude/turn/work; H=$W/hostdata; EV=${EV:-/home/claude/eff/data/ev_ES.txt}
export TMPDIR=/tmp/claude-0; mkdir -p $TMPDIR
CXX="g++ -std=c++17 -Wall -Wextra -Werror"; SAN="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined"
for std in c++14 c++17; do g++ -std=$std -fsyntax-only -Wall -Wextra -I/mnt/user-data/uploads/sdk/c++/include -I$P $T/wrap_real_sdk.cpp > $TMPDIR/realsdk.log 2>&1; echo "$std exit $? errors $(grep -c error $TMPDIR/realsdk.log) warnings outside the SDK $(grep warning $TMPDIR/realsdk.log | grep -vc 'irtsdk.h:')"; done
g++ -std=c++14 -DLSPERF_FAKE -Wall -Wextra -Werror -I$T/mock -I$P $T/compile_only.cpp -o $TMPDIR/turn_compile && echo "mock SDK compile C++14 -Werror ok"
$CXX $SAN $T/test_turn.cpp -o $T/t_logic && $T/t_logic $T/cl_20261123_0947.txt | tail -1
$CXX -O2 $T/replay.cpp -o $T/replay; python3 $T/golden.py $W/an $W/repo/data/turn_pane /mnt/user-data/uploads/level-reversal-analytics/data/mbo/store $TMPDIR/turn_golden | tail -2 | head -1
(cd $T && $CXX $SAN -DLSPERF_FAKE -Imock -I.. test_turn_host_130.cpp -o t_host130 && TURN_TEST_DATA=$H TURN_EV=$EV ./t_host130 2>&1 | grep -a "Absorption bars\|checks\|FAIL\|overlap\|LeakSanitizer\|ERROR")
(cd $T && $CXX -O2 -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_plain && TURN_TEST_DATA=$H ./soak_plain 40 | tail -2 && $CXX $SAN -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_asan && TURN_TEST_DATA=$H ./soak_asan 8 2>&1 | grep -a "checks\|rss after\|LeakSanitizer\|ERROR")
(cd $T && $CXX $SAN t_read140.cpp -o t_read140 && ./t_read140 | tail -1)
(cd $T && $CXX $SAN t_abs150.cpp -o t_abs150 && ./t_abs150 | tail -1)
(cd $T && $CXX $SAN t_cr170.cpp -o t_cr170 && ./t_cr170 | tail -1)
echo ALL DONE 1.7.0
