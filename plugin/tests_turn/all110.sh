#!/bin/bash
# lsTurn 1.1.0: rebuild and run every check (research machine: needs the MBO store and Friday's event files)
set -e
E=/home/claude/eff; W=/home/claude/turn/work; P=$E/plugin; T=$P/tests_turn
STORE=/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store
export TMPDIR=/tmp/claude-0; mkdir -p $TMPDIR
CXX="g++ -std=c++17 -Wall -Wextra -Werror"; SAN="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined"
echo "== 0 syntax against the REAL irtsdk.h"
for std in c++14 c++17; do g++ -std=$std -fsyntax-only -Wall -Wextra -I/mnt/user-data/uploads/sdk/c++/include -I$P $T/wrap_real_sdk.cpp > $TMPDIR/realsdk.log 2>&1; echo "$std exit $? errors $(grep -c error $TMPDIR/realsdk.log) warnings outside the SDK $(grep warning $TMPDIR/realsdk.log | grep -vc 'irtsdk.h:')"; done
g++ -std=c++14 -DLSPERF_FAKE -Wall -Wextra -Werror -I$T/mock -I$P $T/compile_only.cpp -o $TMPDIR/turn_compile && echo "mock SDK compile C++14 -Werror ok"
echo "== 1 logic unit tests (ASan / UBSan), C++17 and C++14"; $CXX $SAN $T/test_turn.cpp -o $T/t_logic && $T/t_logic | tail -2; g++ -std=c++14 -Wall -Wextra -Werror -O2 $T/test_turn.cpp -o $TMPDIR/t_logic14 && $TMPDIR/t_logic14 | tail -1
echo "== 3 the plugin parses every params file"; $CXX -O2 $T/replay.cpp -o $T/replay; echo "1 3 1 100 1 1 2" > $TMPDIR/one.txt
for m in ES NQ CL GC HG NG EU; do $T/replay $TMPDIR/one.txt --params $W/flex/LRA-TurnPane-$m.csv $m | tail -1; done
echo "== 4 golden candles through the C++"; python3 $T/golden.py $W/an $W/repo/data/turn_pane $STORE $TMPDIR/turn_golden | tail -2 | cut -c1-200
echo "== 5 the whole plugin on the mock SDK (live morning, ASan / UBSan / LSan)"
(cd $T && $CXX $SAN -DLSPERF_FAKE -Imock -I.. test_turn_host.cpp -o t_host && TURN_TEST_DATA=$W/hostdata TURN_DUMP=$E/out/dump ./t_host 2>&1 | grep -a "plain candles\|mouse on 8:\|checks\|FAIL\|overlap\|LeakSanitizer\|ERROR")
echo "== 6 soak: 40 cycles plain, 8 under ASan / LSan"
(cd $T && $CXX -O2 -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_plain && TURN_TEST_DATA=$W/hostdata ./soak_plain 40 | tail -2 && $CXX $SAN -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_asan && TURN_TEST_DATA=$W/hostdata ./soak_asan 8 2>&1 | grep -a "checks\|rss\|LeakSanitizer\|ERROR")
echo "== 7 1.0.1 against 1.1.0 on Friday's ES morning, 8:24 - 10:33 AM (44 candles), five zooms; restart; draw cost"
H=$W/hostdata; g++ -std=c++17 -O2 -DLSPERF_FAKE -w -I$T/mock -I$P $E/mock/effmock.cpp -o $E/mock/effmock_new; g++ -std=c++17 -O2 -DLSPERF_FAKE -w -I$T/mock -I$E/base $E/mock/effmock.cpp -o $E/mock/effmock_old
(cd $E && for z in 60 22 12 6 3; do for v in old new; do EFF_RESTART=1 ./mock/effmock_$v data/ev_ES.txt $H out/z/${v}_$z $z 170 | grep -a restart | sed "s/^/$v $z px /"; done; done | sort | uniq -c -f3 | head; python3 mock/diff.py
 for cfg in "300 22" "1000 6" "3000 3"; do set -- $cfg; EFF_PERF=$1 ./mock/effmock_new data/ev_ES.txt $H out/z/perf $2 170 | grep perf; done)
echo ALL DONE
